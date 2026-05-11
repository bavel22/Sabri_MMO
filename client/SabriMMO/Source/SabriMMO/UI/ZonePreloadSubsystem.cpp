// ZonePreloadSubsystem.cpp — see header for architecture overview.
#include "ZonePreloadSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "CharacterData.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogZonePreload, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UZonePreloadSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UZonePreloadSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UE_LOG(LogZonePreload, Log, TEXT("ZonePreloadSubsystem started (world: %s)"),
		*InWorld.GetName());

	// Pin the local player's class — they're rendered every frame, never evict.
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (GI)
	{
		const FCharacterData SelChar = GI->GetSelectedCharacter();
		if (!SelChar.JobClass.IsEmpty())
		{
			const FString LocalGender = (SelChar.Gender.ToLower() == TEXT("female"))
				? TEXT("f") : TEXT("m");
			const FString LocalClass = FString::Printf(TEXT("%s_%s"),
				*SelChar.JobClass.ToLower(), *LocalGender);
			SetLocalPlayerClass(LocalClass);

			UE_LOG(LogZonePreload, Log, TEXT("Pinned local player class: '%s'"), *LocalClass);
		}

		// Update zone tracking
		BeginZone(GI->CurrentZoneName);

		// Listen for predictive adjacent zone preload data from the server.
		if (USocketEventRouter* Router = GI->GetEventRouter())
		{
			Router->RegisterHandler(TEXT("zone:adjacent_classes"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleAdjacentClasses(D); });
		}
	}
}

void UZonePreloadSubsystem::HandleAdjacentClasses(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* RootObjPtr = nullptr;
	if (!Data->TryGetObject(RootObjPtr) || !RootObjPtr) return;
	const TSharedPtr<FJsonObject>& Root = *RootObjPtr;

	const TSharedPtr<FJsonObject>* ZonesObjPtr = nullptr;
	if (!Root->TryGetObjectField(TEXT("zones"), ZonesObjPtr) || !ZonesObjPtr) return;
	const TSharedPtr<FJsonObject>& ZonesObj = *ZonesObjPtr;

	int32 PreloadedCount = 0;
	for (const auto& Pair : ZonesObj->Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* ClassArr = nullptr;
		if (!Pair.Value->TryGetArray(ClassArr) || !ClassArr) continue;

		for (const TSharedPtr<FJsonValue>& Val : *ClassArr)
		{
			FString ClassName;
			if (!Val->TryGetString(ClassName) || ClassName.IsEmpty()) continue;
			RequestClassPreload(ClassName);
			++PreloadedCount;
		}
	}

	if (PreloadedCount > 0)
	{
		UE_LOG(LogZonePreload, Log,
			TEXT("Predictive preload: requested %d classes across %d adjacent zones"),
			PreloadedCount, ZonesObj->Values.Num());
	}
}

void UZonePreloadSubsystem::Deinitialize()
{
	// Unregister socket handler before tearing down.
	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	// Drop all handles → textures become GC-eligible.
	// Pinned and active textures are released on world teardown; the next world's
	// subsystem starts with fresh state.
	PinnedHandles.Empty();
	ActiveHandles.Empty();
	LruCache.Empty();
	ClassPathsCache.Empty();
	InFlightClasses.Empty();
	InFlightCount = 0;
	ApproxResidentBytes = 0;

	UE_LOG(LogZonePreload, Log, TEXT("ZonePreloadSubsystem shut down"));
	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

void UZonePreloadSubsystem::RequestClassPreload(const FString& SpriteClass)
{
	if (SpriteClass.IsEmpty()) return;

	// Already pinned? Done.
	if (PinnedClasses.Contains(SpriteClass)) return;

	// Already active in current zone? Done.
	if (ActiveZoneClasses.Contains(SpriteClass)) return;

	// In LRU cache? Promote to active without re-loading.
	if (TryPromoteFromLru(SpriteClass))
	{
		ActiveZoneClasses.Add(SpriteClass);
		UE_LOG(LogZonePreload, Verbose, TEXT("RequestClassPreload: '%s' promoted from LRU cache"),
			*SpriteClass);
		return;
	}

	// In flight? Don't queue duplicate.
	if (InFlightClasses.Contains(SpriteClass)) return;

	// Resolve asset paths (cached after first call for this class)
	FResolvedClass& Resolved = ClassPathsCache.FindOrAdd(SpriteClass);
	if (Resolved.AssetPaths.Num() == 0)
	{
		FString ManifestPath = FindClassManifestPath(SpriteClass);
		if (ManifestPath.IsEmpty())
		{
			UE_LOG(LogZonePreload, Warning, TEXT("No manifest found for class '%s'"),
				*SpriteClass);
			return;
		}
		if (!ResolveAssetPaths(ManifestPath, Resolved))
		{
			UE_LOG(LogZonePreload, Warning, TEXT("Failed to parse manifest for '%s'"),
				*SpriteClass);
			return;
		}
	}

	// Kick off async load
	TSharedPtr<FStreamableHandle> Handle = StartAsyncLoad(SpriteClass, Resolved);
	if (!Handle.IsValid()) return;

	ActiveHandles.Add(SpriteClass, Handle);
	ActiveZoneClasses.Add(SpriteClass);
	ApproxResidentBytes += Resolved.EstimatedBytes;

	UE_LOG(LogZonePreload, Log,
		TEXT("RequestClassPreload: '%s' (%d atlases, ~%lld MB) started — total in-flight: %d"),
		*SpriteClass, Resolved.AssetPaths.Num(),
		Resolved.EstimatedBytes / (1024 * 1024), InFlightCount);
}

void UZonePreloadSubsystem::RequestLayerPreload(ESpriteLayer Layer, int32 ViewSpriteId,
                                                 const FString& GenderSubDir)
{
	if (ViewSpriteId <= 0) return;

	const FString Key = MakeLayerKey(Layer, ViewSpriteId, GenderSubDir);

	if (PinnedClasses.Contains(Key)) return;
	if (ActiveZoneClasses.Contains(Key)) return;
	if (TryPromoteFromLru(Key))
	{
		ActiveZoneClasses.Add(Key);
		return;
	}
	if (InFlightClasses.Contains(Key)) return;

	FResolvedClass& Resolved = ClassPathsCache.FindOrAdd(Key);
	if (Resolved.AssetPaths.Num() == 0)
	{
		FString ManifestPath = FindLayerManifestPath(Layer, ViewSpriteId, GenderSubDir);
		if (ManifestPath.IsEmpty())
		{
			UE_LOG(LogZonePreload, Warning, TEXT("No layer manifest for %s view=%d gender='%s'"),
				*ASpriteCharacterActor::GetLayerSubDir(Layer), ViewSpriteId, *GenderSubDir);
			return;
		}
		if (!ResolveAssetPaths(ManifestPath, Resolved)) return;
	}

	TSharedPtr<FStreamableHandle> Handle = StartAsyncLoad(Key, Resolved);
	if (!Handle.IsValid()) return;

	ActiveHandles.Add(Key, Handle);
	ActiveZoneClasses.Add(Key);
	ApproxResidentBytes += Resolved.EstimatedBytes;
}

void UZonePreloadSubsystem::PinClass(const FString& SpriteClass)
{
	if (SpriteClass.IsEmpty()) return;
	if (PinnedClasses.Contains(SpriteClass)) return;

	// If already loaded as active, just transfer the handle to pinned tier.
	if (ActiveHandles.Contains(SpriteClass))
	{
		PinnedHandles.Add(SpriteClass, ActiveHandles[SpriteClass]);
		ActiveHandles.Remove(SpriteClass);
		ActiveZoneClasses.Remove(SpriteClass);
		PinnedClasses.Add(SpriteClass);
		UE_LOG(LogZonePreload, Log, TEXT("PinClass: '%s' (transferred from active)"), *SpriteClass);
		return;
	}

	// If in LRU, transfer.
	if (FCachedClassEntry* Cached = LruCache.Find(SpriteClass))
	{
		PinnedHandles.Add(SpriteClass, Cached->Handle);
		ApproxResidentBytes -= Cached->ApproxBytes;  // re-counted below
		ApproxResidentBytes += Cached->ApproxBytes;
		LruCache.Remove(SpriteClass);
		PinnedClasses.Add(SpriteClass);
		UE_LOG(LogZonePreload, Log, TEXT("PinClass: '%s' (transferred from LRU)"), *SpriteClass);
		return;
	}

	// Otherwise mark as pinned and trigger preload — handle will be added on completion.
	PinnedClasses.Add(SpriteClass);

	FResolvedClass& Resolved = ClassPathsCache.FindOrAdd(SpriteClass);
	if (Resolved.AssetPaths.Num() == 0)
	{
		FString ManifestPath = FindClassManifestPath(SpriteClass);
		if (ManifestPath.IsEmpty()) return;
		if (!ResolveAssetPaths(ManifestPath, Resolved)) return;
	}

	TSharedPtr<FStreamableHandle> Handle = StartAsyncLoad(SpriteClass, Resolved);
	if (Handle.IsValid())
	{
		PinnedHandles.Add(SpriteClass, Handle);
		ApproxResidentBytes += Resolved.EstimatedBytes;
		UE_LOG(LogZonePreload, Log, TEXT("PinClass: '%s' (new pinned load)"), *SpriteClass);
	}
}

void UZonePreloadSubsystem::SetLocalPlayerClass(const FString& SpriteClass)
{
	if (SpriteClass.IsEmpty()) return;
	if (LocalPlayerClass == SpriteClass) return;

	// Unpin the previous one (rare — only happens if user changes character mid-session)
	if (!LocalPlayerClass.IsEmpty() && PinnedClasses.Contains(LocalPlayerClass))
	{
		const int64 PrevBytes = ClassPathsCache.Contains(LocalPlayerClass)
			? ClassPathsCache[LocalPlayerClass].EstimatedBytes : 0;
		PinnedClasses.Remove(LocalPlayerClass);
		PinnedHandles.Remove(LocalPlayerClass);
		ApproxResidentBytes = FMath::Max<int64>(0, ApproxResidentBytes - PrevBytes);
	}

	LocalPlayerClass = SpriteClass;
	PinClass(SpriteClass);
}

void UZonePreloadSubsystem::BeginZone(const FString& NewZoneName)
{
	UE_LOG(LogZonePreload, Log,
		TEXT("BeginZone: '%s' -> '%s' (demoting %d active classes to LRU)"),
		*CurrentZoneName, *NewZoneName, ActiveZoneClasses.Num());

	// Demote every active class to LRU cache (still resident, but evictable).
	// Copy first — DemoteActiveToLru modifies ActiveZoneClasses during iteration.
	TArray<FString> ToMove = ActiveZoneClasses.Array();
	for (const FString& Key : ToMove)
	{
		// Local player's class is in PinnedClasses, not Active — don't touch.
		if (PinnedClasses.Contains(Key)) continue;
		DemoteActiveToLru(Key);
	}

	ActiveZoneClasses.Empty();
	ActiveHandles.Empty();
	CurrentZoneName = NewZoneName;

	// Trim the LRU cache if it's over budget after demotions.
	EvictLruIfOverBudget();
}

int32 UZonePreloadSubsystem::GetResidentClassCount() const
{
	return PinnedClasses.Num() + ActiveZoneClasses.Num() + LruCache.Num();
}

// ============================================================
// Manifest resolution
// ============================================================

FString UZonePreloadSubsystem::FindClassManifestPath(const FString& SpriteClass) const
{
	const FString BodyRoot = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases/Body");
	const FString ManifestFileName = FString::Printf(TEXT("%s_manifest.json"), *SpriteClass);

	// Search order matches SpriteCharacterActor::SetBodyClass(FString):
	//   1. Body/{class}/         (player classes)
	//   2. Body/enemies/{class}/ (enemies)
	//   3. Body/                 (legacy flat)
	const TArray<FString> CandidateDirs = {
		BodyRoot / SpriteClass,
		BodyRoot / TEXT("enemies") / SpriteClass,
		BodyRoot,
	};

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	for (const FString& Dir : CandidateDirs)
	{
		const FString Candidate = Dir / ManifestFileName;
		if (PF.FileExists(*Candidate))
			return Candidate;
	}
	return FString();
}

FString UZonePreloadSubsystem::FindLayerManifestPath(ESpriteLayer Layer, int32 ViewSpriteId,
                                                     const FString& GenderSubDir) const
{
	const FString SubDir = ASpriteCharacterActor::GetLayerSubDir(Layer);
	if (SubDir.IsEmpty()) return FString();

	const FString LayerRoot = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases") / SubDir;
	const FString BaseName = FString::Printf(TEXT("%s_%d"), *SubDir.ToLower(), ViewSpriteId);
	const FString ManifestFileName = FString::Printf(TEXT("%s_manifest.json"), *BaseName);

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

	// Same search order as SpriteCharacterActor::LoadEquipmentLayer:
	//   1. Each item subdir + gender subfolder (e.g., Weapon/dagger/female/)
	//   2. Item subdir directly (Weapon/dagger/)
	//   3. Layer root flat (Weapon/)
	TArray<FString> SubDirs;
	IFileManager::Get().FindFiles(SubDirs, *(LayerRoot / TEXT("*")), false, true);
	for (const FString& SD : SubDirs)
	{
		const FString ItemDir = LayerRoot / SD;

		if (!GenderSubDir.IsEmpty())
		{
			const FString GenderPath = ItemDir / GenderSubDir / ManifestFileName;
			if (PF.FileExists(*GenderPath)) return GenderPath;
		}

		const FString FlatPath = ItemDir / ManifestFileName;
		if (PF.FileExists(*FlatPath)) return FlatPath;
	}

	const FString RootPath = LayerRoot / ManifestFileName;
	if (PF.FileExists(*RootPath)) return RootPath;

	return FString();
}

bool UZonePreloadSubsystem::ResolveAssetPaths(const FString& ManifestPath,
                                              FResolvedClass& OutResolved) const
{
	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *ManifestPath))
		return false;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return false;

	const TArray<TSharedPtr<FJsonValue>>* AtlasArr;
	if (!Root->TryGetArrayField(TEXT("atlases"), AtlasArr))
		return false;

	const FString JsonDir = FPaths::GetPath(ManifestPath);
	const FString ContentBase = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases");
	FString AssetSubPath = TEXT("Body");
	if (JsonDir.StartsWith(ContentBase))
		AssetSubPath = JsonDir.Mid(ContentBase.Len() + 1);

	OutResolved.AssetPaths.Reserve(AtlasArr->Num());

	for (const TSharedPtr<FJsonValue>& Val : *AtlasArr)
	{
		const TSharedPtr<FJsonObject>& AtlasObj = Val->AsObject();
		if (!AtlasObj.IsValid()) continue;

		FString FileName = AtlasObj->GetStringField(TEXT("file"));
		if (FileName.IsEmpty()) continue;

		// /Game/SabriMMO/Sprites/Atlases/<sub>/<file>.<file>
		const FString PackagePath = FString::Printf(
			TEXT("/Game/SabriMMO/Sprites/Atlases/%s/%s.%s"),
			*AssetSubPath, *FileName, *FileName);

		OutResolved.AssetPaths.Add(FSoftObjectPath(PackagePath));
	}

	// Rough estimate: ~21 MB per atlas at current settings (BC7 + future mips).
	// This drives LRU eviction decisions; precise accounting isn't required.
	OutResolved.EstimatedBytes = static_cast<int64>(OutResolved.AssetPaths.Num()) * 21LL * 1024 * 1024;

	return OutResolved.AssetPaths.Num() > 0;
}

// ============================================================
// Async loading
// ============================================================

TSharedPtr<FStreamableHandle> UZonePreloadSubsystem::StartAsyncLoad(
	const FString& ClassKey, const FResolvedClass& Resolved)
{
	if (Resolved.AssetPaths.Num() == 0) return nullptr;

	UAssetManager* AM = UAssetManager::GetIfInitialized();
	if (!AM)
	{
		UE_LOG(LogZonePreload, Warning, TEXT("UAssetManager not initialized"));
		return nullptr;
	}

	FStreamableManager& Streamable = AM->GetStreamableManager();

	InFlightClasses.Add(ClassKey);
	++InFlightCount;

	TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(
		Resolved.AssetPaths,
		FStreamableDelegate::CreateUObject(this, &UZonePreloadSubsystem::OnClassLoaded, ClassKey),
		PreloadPriority,
		false /* bManageActiveHandle */,
		false /* bStartStalled */,
		FString::Printf(TEXT("ZonePreload:%s"), *ClassKey)
	);

	return Handle;
}

void UZonePreloadSubsystem::OnClassLoaded(FString ClassKey)
{
	InFlightClasses.Remove(ClassKey);
	InFlightCount = FMath::Max(0, InFlightCount - 1);

	UE_LOG(LogZonePreload, Log, TEXT("Loaded '%s' — %d still in flight"),
		*ClassKey, InFlightCount);

	if (InFlightCount == 0)
	{
		OnAllPreloadsComplete.Broadcast();
	}
}

// ============================================================
// LRU cache management
// ============================================================

void UZonePreloadSubsystem::DemoteActiveToLru(const FString& SpriteClass)
{
	TSharedPtr<FStreamableHandle>* HandlePtr = ActiveHandles.Find(SpriteClass);
	if (!HandlePtr) return;

	const int64 Bytes = ClassPathsCache.Contains(SpriteClass)
		? ClassPathsCache[SpriteClass].EstimatedBytes : 0;

	FCachedClassEntry Entry;
	Entry.Handle = *HandlePtr;
	Entry.LastUsedTime = FPlatformTime::Seconds();
	Entry.ApproxBytes = Bytes;
	LruCache.Add(SpriteClass, Entry);

	ActiveHandles.Remove(SpriteClass);
	ActiveZoneClasses.Remove(SpriteClass);
}

bool UZonePreloadSubsystem::TryPromoteFromLru(const FString& SpriteClass)
{
	FCachedClassEntry* Cached = LruCache.Find(SpriteClass);
	if (!Cached) return false;

	ActiveHandles.Add(SpriteClass, Cached->Handle);
	Cached->LastUsedTime = FPlatformTime::Seconds();
	LruCache.Remove(SpriteClass);
	return true;
}

void UZonePreloadSubsystem::EvictLruIfOverBudget()
{
	int64 LruBytes = 0;
	for (const auto& Pair : LruCache)
		LruBytes += Pair.Value.ApproxBytes;

	if (LruBytes <= LruCacheBudgetBytes) return;

	// Sort by LastUsedTime ascending (oldest first), evict until under budget.
	TArray<TPair<FString, double>> ByAge;
	ByAge.Reserve(LruCache.Num());
	for (const auto& Pair : LruCache)
		ByAge.Add({Pair.Key, Pair.Value.LastUsedTime});
	ByAge.Sort([](const TPair<FString, double>& A, const TPair<FString, double>& B)
	{
		return A.Value < B.Value;
	});

	for (const auto& Old : ByAge)
	{
		if (LruBytes <= LruCacheBudgetBytes) break;
		FCachedClassEntry* E = LruCache.Find(Old.Key);
		if (!E) continue;
		LruBytes -= E->ApproxBytes;
		ApproxResidentBytes = FMath::Max<int64>(0, ApproxResidentBytes - E->ApproxBytes);
		LruCache.Remove(Old.Key);
		UE_LOG(LogZonePreload, Log, TEXT("LRU evicted '%s' (%lld MB)"),
			*Old.Key, E->ApproxBytes / (1024 * 1024));
	}
}

// ============================================================
// Helpers
// ============================================================

FString UZonePreloadSubsystem::MakeLayerKey(ESpriteLayer Layer, int32 ViewSpriteId,
                                            const FString& GenderSubDir) const
{
	return FString::Printf(TEXT("layer:%d:%d:%s"),
		static_cast<int32>(Layer), ViewSpriteId, *GenderSubDir);
}
