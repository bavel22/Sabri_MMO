// SpawnRegionExporter.cpp — see header.

#include "SpawnRegionExporter.h"
#include "SpawnRegionVolumes.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/World.h"

// Console command: ExportSpawnRegions <zone_name> [output_dir]
static FAutoConsoleCommandWithWorldAndArgs GExportSpawnRegionsCmd(
	TEXT("ExportSpawnRegions"),
	TEXT("Export spawn allow/deny volumes for the current level as JSON. Usage: ExportSpawnRegions <zone_name> [output_dir]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTemp, Error, TEXT("[SpawnRegionExport] Usage: ExportSpawnRegions <zone_name> [output_dir]"));
			UE_LOG(LogTemp, Error, TEXT("[SpawnRegionExport] Zone names: prontera, prontera_south, prontera_north, prt_dungeon_01"));
			return;
		}

		const FString ZoneName = Args[0];
		const FString OutputDir = Args.Num() >= 2 ? Args[1] : TEXT("");
		USpawnRegionExporter::ExportSpawnRegionsToJSON(World, ZoneName, OutputDir);
	})
);

static FAutoConsoleCommandWithWorld GExportCurrentLevelSpawnRegionsCmd(
	TEXT("ExportCurrentLevelSpawnRegions"),
	TEXT("Export spawn volumes auto-detecting zone name from current level."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		USpawnRegionExporter::ExportCurrentLevelSpawnRegions(World);
	})
);

FString USpawnRegionExporter::GetDefaultOutputDirectory()
{
	// ProjectDir is "C:/Sabri_MMO/client/SabriMMO/" — go up 2 directories ("../../")
	// to reach the repo root, then into server/spawn_regions/.
	// Note: NavMeshExporter has a longstanding bug here that drops files in
	// client/server/navmesh/ instead of <repo_root>/server/navmesh/. We compute the
	// path correctly so SpawnRegions JSON lands directly in the right place — no copy needed.
	const FString ProjectDir = FPaths::ProjectDir();
	const FString Resolved = FPaths::ConvertRelativePathToFull(ProjectDir / TEXT("../../server/spawn_regions"));
	return Resolved;
}

FString USpawnRegionExporter::MapNameToZone(const FString& MapName)
{
	FString Clean = MapName;
	if (Clean.StartsWith(TEXT("UEDPIE_")))
	{
		// Strip "UEDPIE_N_" prefix used in PIE mode.
		const FString Prefix = TEXT("UEDPIE_");
		FString Tail = Clean.Mid(Prefix.Len());
		int32 SecondUnderscore = INDEX_NONE;
		Tail.FindChar(TEXT('_'), SecondUnderscore);
		if (SecondUnderscore != INDEX_NONE)
		{
			Clean = Tail.Mid(SecondUnderscore + 1);
		}
	}

	if (Clean.Contains(TEXT("PrtSouth")))         return TEXT("prontera_south");
	if (Clean.Contains(TEXT("PrtNorth")))         return TEXT("prontera_north");
	if (Clean.Contains(TEXT("PrtDungeon01")))     return TEXT("prt_dungeon_01");
	if (Clean.Contains(TEXT("Prontera")))         return TEXT("prontera");
	return Clean.ToLower();
}

FString USpawnRegionExporter::EscapeJsonString(const FString& In)
{
	FString Out = In;
	Out.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	Out.ReplaceInline(TEXT("\""), TEXT("\\\""));
	return Out;
}

void USpawnRegionExporter::ExportSpawnRegionsToJSON(
	UObject* WorldContextObject,
	const FString& ZoneName,
	const FString& OutputDirectory)
{
	if (ZoneName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnRegionExport] Zone name cannot be empty"));
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnRegionExport] No valid world context"));
		return;
	}

	TArray<FString> AllowEntries;
	TArray<FString> DenyEntries;

	for (TActorIterator<ASpawnRegionVolume> It(World); It; ++It)
	{
		ASpawnRegionVolume* Vol = *It;
		if (!Vol || !Vol->BoxComp) continue;

		// World-space AABB. UBoxComponent::Bounds already accounts for the actor's
		// transform, so a rotated volume is exported as the smallest axis-aligned box
		// that fully encloses the rotated volume.
		const FBoxSphereBounds B = Vol->BoxComp->Bounds;
		const FVector Min = B.Origin - B.BoxExtent;
		const FVector Max = B.Origin + B.BoxExtent;

		FString FilterJson = TEXT("[]");
		if (Vol->MonsterFilter.Num() > 0)
		{
			TArray<FString> Quoted;
			Quoted.Reserve(Vol->MonsterFilter.Num());
			for (const FString& F : Vol->MonsterFilter)
			{
				Quoted.Add(FString::Printf(TEXT("\"%s\""), *EscapeJsonString(F)));
			}
			FilterJson = FString::Printf(TEXT("[%s]"), *FString::Join(Quoted, TEXT(", ")));
		}

		const FString Tag = Vol->RegionTag.IsEmpty() ? Vol->GetName() : Vol->RegionTag;

		const FString Entry = FString::Printf(
			TEXT("    {\n      \"min\": [%.2f, %.2f, %.2f],\n      \"max\": [%.2f, %.2f, %.2f],\n      \"tag\": \"%s\",\n      \"filter\": %s\n    }"),
			Min.X, Min.Y, Min.Z,
			Max.X, Max.Y, Max.Z,
			*EscapeJsonString(Tag),
			*FilterJson
		);

		if (Vol->IsA(ASpawnAllowVolume::StaticClass()))
		{
			AllowEntries.Add(Entry);
		}
		else if (Vol->IsA(ASpawnDenyVolume::StaticClass()))
		{
			DenyEntries.Add(Entry);
		}
	}

	if (AllowEntries.Num() == 0 && DenyEntries.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnRegionExport] No SpawnAllowVolume or SpawnDenyVolume actors in this level — nothing to export"));
		return;
	}

	FString JsonContent;
	JsonContent.Reserve(256 + (AllowEntries.Num() + DenyEntries.Num()) * 200);
	JsonContent += TEXT("{\n");
	JsonContent += FString::Printf(TEXT("  \"version\": 1,\n"));
	JsonContent += FString::Printf(TEXT("  \"zone\": \"%s\",\n"), *EscapeJsonString(ZoneName));
	JsonContent += TEXT("  \"allow\": [\n");
	JsonContent += FString::Join(AllowEntries, TEXT(",\n"));
	JsonContent += TEXT("\n  ],\n");
	JsonContent += TEXT("  \"deny\": [\n");
	JsonContent += FString::Join(DenyEntries, TEXT(",\n"));
	JsonContent += TEXT("\n  ]\n");
	JsonContent += TEXT("}\n");

	const FString OutDir = OutputDirectory.IsEmpty() ? GetDefaultOutputDirectory() : OutputDirectory;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*OutDir))
	{
		PlatformFile.CreateDirectoryTree(*OutDir);
	}

	const FString FilePath = OutDir / FString::Printf(TEXT("%s.json"), *ZoneName);

	if (FFileHelper::SaveStringToFile(JsonContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Log, TEXT("[SpawnRegionExport] SUCCESS: %d allow + %d deny -> %s"),
			AllowEntries.Num(), DenyEntries.Num(), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnRegionExport] FAILED: Could not write to %s"), *FilePath);
	}
}

void USpawnRegionExporter::ExportCurrentLevelSpawnRegions(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	const FString MapName = World->GetMapName();
	const FString ZoneName = MapNameToZone(MapName);
	UE_LOG(LogTemp, Log, TEXT("[SpawnRegionExport] Auto-detected zone: %s (from level: %s)"), *ZoneName, *MapName);
	ExportSpawnRegionsToJSON(WorldContextObject, ZoneName);
}
