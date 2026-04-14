// MinimapSubsystem.cpp — Minimap + World Map data management and widget lifecycle.

#include "MinimapSubsystem.h"
#include "SMinimapWidget.h"
#include "SWorldMapWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "ZoneTransitionSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "GameFramework/Character.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMinimap, Log, All);

// ============================================================
// Lifecycle
constexpr float UMinimapSubsystem::ZoomFactors[];

// ============================================================

bool UMinimapSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UMinimapSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Store local character ID for filtering
	LocalCharacterId = GI->GetSelectedCharacter().CharacterId;

	// Restore minimap preferences from GameInstance (persist across zone transitions)
	OpacityState = GI->MinimapOpacityState;
	ZoomLevel = GI->MinimapZoomLevel;

	// Read current zone from GameInstance.
	// During zone transitions, PendingZoneName is set BEFORE OpenLevel (by zone:change handler),
	// while CurrentZoneName is only updated AFTER transition completes (too late for us).
	// So prefer PendingZoneName if we're transitioning.
	if (!GI->PendingZoneName.IsEmpty())
	{
		CurrentZoneName = GI->PendingZoneName;
	}
	else
	{
		CurrentZoneName = GI->CurrentZoneName;
	}
	if (CurrentDisplayName.IsEmpty())
	{
		CurrentDisplayName = CurrentZoneName;
	}

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("map:world_data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleWorldMapData(D); });
		Router->RegisterHandler(TEXT("map:party_positions"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePartyPositions(D); });
		Router->RegisterHandler(TEXT("map:mark"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleMinimapMark(D); });

		// Listen for party updates to show party member dots on world map
		Router->RegisterHandler(TEXT("party:update"), this,
			[this](const TSharedPtr<FJsonValue>& D) {
				if (!D.IsValid()) return;
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (!D->TryGetObject(Obj) || !Obj) return;
				const TArray<TSharedPtr<FJsonValue>>* MembersArr = nullptr;
				if (!(*Obj)->TryGetArrayField(TEXT("members"), MembersArr)) return;
				PartyMemberZones.Empty();
				for (const auto& MVal : *MembersArr)
				{
					const TSharedPtr<FJsonObject>* MObj = nullptr;
					if (!MVal->TryGetObject(MObj)) continue;
					FPartyMemberZone PMZ;
					double CidD = 0;
					(*MObj)->TryGetNumberField(TEXT("characterId"), CidD);
					PMZ.CharacterId = (int32)CidD;
					(*MObj)->TryGetStringField(TEXT("characterName"), PMZ.PlayerName);
					(*MObj)->TryGetStringField(TEXT("map"), PMZ.ZoneName);
					if (PMZ.ZoneName.IsEmpty())
					{
						(*MObj)->TryGetStringField(TEXT("mapName"), PMZ.ZoneName);
					}
					PMZ.ZoneDisplayName = PMZ.ZoneName;
					if (PMZ.CharacterId > 0)
					{
						PartyMemberZones.Add(PMZ);
					}
				}
			});

		// Listen for zone changes to update current zone indicator
		Router->RegisterHandler(TEXT("zone:change"), this,
			[this](const TSharedPtr<FJsonValue>& D) {
				if (!D.IsValid()) return;
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (!D->TryGetObject(Obj) || !Obj) return;
				FString NewZone;
				(*Obj)->TryGetStringField(TEXT("zone"), NewZone);
				if (!NewZone.IsEmpty())
				{
					CurrentZoneName = NewZone;
					FString NewDisplay;
					(*Obj)->TryGetStringField(TEXT("displayName"), NewDisplay);
					CurrentDisplayName = NewDisplay.IsEmpty() ? NewZone : NewDisplay;
				}
			});
	}

	// Load world map texture
	LoadWorldMapTexture();

	// Only show widgets if socket is connected (game level, not login)
	if (!GI->IsSocketConnected()) return;

	ShowMinimap();
	SetupOverheadCapture();

	UE_LOG(LogMinimap, Log, TEXT("MinimapSubsystem started — zone: %s"), *CurrentZoneName);
}

void UMinimapSubsystem::Deinitialize()
{
	CleanupCapture();
	HideMinimap();
	HideWorldMap();

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

	Super::Deinitialize();
}

// ============================================================
// Widget management
// ============================================================

void UMinimapSubsystem::ShowMinimap()
{
	if (bMinimapAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	MinimapWidget = SNew(SMinimapWidget).Subsystem(this);

	// Pin to top-right corner
	MinimapAlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			MinimapWidget.ToSharedRef()
		];

	MinimapViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(MinimapAlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(MinimapViewportOverlay.ToSharedRef(), 5);
	bMinimapAdded = true;
}

void UMinimapSubsystem::HideMinimap()
{
	if (!bMinimapAdded) return;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (MinimapViewportOverlay.IsValid())
			{
				VC->RemoveViewportWidgetContent(MinimapViewportOverlay.ToSharedRef());
			}
		}
	}
	MinimapWidget.Reset();
	MinimapAlignmentWrapper.Reset();
	MinimapViewportOverlay.Reset();
	bMinimapAdded = false;
}

void UMinimapSubsystem::ShowWorldMap()
{
	if (bWorldMapAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	WorldMapWidgetPtr = SNew(SWorldMapWidget).Subsystem(this);

	// Wrap in HAlign_Fill/VAlign_Fill so the widget fills the entire viewport
	// and receives mouse hit-tests. Without this, the widget has 0x0 layout
	// geometry and OnMouseMove/OnMouseButtonDown never fire.
	WorldMapAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::Visible)
		[
			WorldMapWidgetPtr.ToSharedRef()
		];

	WorldMapViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(WorldMapAlignWrapper);
	ViewportClient->AddViewportWidgetContent(WorldMapViewportOverlay.ToSharedRef(), 50);
	bWorldMapAdded = true;
	bWorldMapOpen = true;
}

void UMinimapSubsystem::HideWorldMap()
{
	if (!bWorldMapAdded) return;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (WorldMapViewportOverlay.IsValid())
			{
				VC->RemoveViewportWidgetContent(WorldMapViewportOverlay.ToSharedRef());
			}
		}
	}
	WorldMapWidgetPtr.Reset();
	WorldMapAlignWrapper.Reset();
	WorldMapViewportOverlay.Reset();
	bWorldMapAdded = false;
	bWorldMapOpen = false;
}

// ============================================================
// Public API
// ============================================================

void UMinimapSubsystem::CycleMinimapOpacity()
{
	OpacityState = (OpacityState + 2) % 3;  // 2->1->0->2

	// Persist to GameInstance
	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			GI->MinimapOpacityState = OpacityState;
		}
	}
}

void UMinimapSubsystem::ZoomIn()
{
	ZoomLevel = FMath::Min(ZoomLevel + 1, 4);
	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
			GI->MinimapZoomLevel = ZoomLevel;
	}
}

void UMinimapSubsystem::ZoomOut()
{
	ZoomLevel = FMath::Max(ZoomLevel - 1, 0);
	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
			GI->MinimapZoomLevel = ZoomLevel;
	}
}

void UMinimapSubsystem::ToggleWorldMap()
{
	if (bWorldMapOpen)
	{
		CloseWorldMap();
	}
	else
	{
		ShowWorldMap();
	}
}

void UMinimapSubsystem::CloseWorldMap()
{
	HideWorldMap();
}

void UMinimapSubsystem::ToggleZoneNames()
{
	bShowZoneNames = !bShowZoneNames;
}

void UMinimapSubsystem::ToggleMonsterInfo()
{
	bShowMonsterInfo = !bShowMonsterInfo;
}

// ============================================================
// Overhead SceneCapture for live minimap
// ============================================================

void UMinimapSubsystem::SetupOverheadCapture()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Create render target (256x256 — good quality for 128x128 display)
	MinimapRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	MinimapRenderTarget->InitAutoFormat(256, 256);
	MinimapRenderTarget->ClearColor = FLinearColor(0.08f, 0.12f, 0.06f, 1.f);
	MinimapRenderTarget->UpdateResourceImmediate(true);

	// Spawn a capture actor with a proper root component
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CaptureActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!CaptureActor) return;

	// Create a root scene component so SetActorLocation works
	USceneComponent* Root = NewObject<USceneComponent>(CaptureActor, TEXT("MinimapRoot"));
	Root->RegisterComponent();
	CaptureActor->SetRootComponent(Root);

	// Add SceneCaptureComponent2D attached to root
	CaptureComponent = NewObject<USceneCaptureComponent2D>(CaptureActor, TEXT("MinimapCapture"));
	CaptureComponent->SetupAttachment(Root);
	CaptureComponent->RegisterComponent();

	// Configure: orthographic top-down
	CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	CaptureComponent->OrthoWidth = CaptureOrthoWidth;
	CaptureComponent->TextureTarget = MinimapRenderTarget;
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->bAlwaysPersistRenderingState = true;

	// Look straight down with north (Y+) pointing UP on the minimap image.
	// Pitch -90 = looking down. Yaw -90 = rotates so that:
	//   image right = +X (east), image up = +Y (north)
	CaptureComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	// Reduce rendering cost
	CaptureComponent->ShowFlags.SetFog(false);
	CaptureComponent->ShowFlags.SetVolumetricFog(false);
	CaptureComponent->ShowFlags.SetMotionBlur(false);
	CaptureComponent->ShowFlags.SetBloom(false);
	CaptureComponent->ShowFlags.SetEyeAdaptation(false);
	CaptureComponent->ShowFlags.SetAntiAliasing(false);
	CaptureComponent->ShowFlags.SetAtmosphere(false);
	CaptureComponent->ShowFlags.SetDynamicShadows(false);

	// Disable post-process materials on the capture. The PostProcessSubsystem
	// pushes a cutout material into the unbound global PP volume that darkens
	// every pixel where CustomStencil != 1 (i.e. everything that isn't the
	// player sprite). The minimap camera looks straight down from above, so
	// the billboard sprite is edge-on and writes no stencil into the capture,
	// which causes the cutout to darken the entire minimap to ~61% brightness
	// and the captured scene effectively vanishes against the dark frame.
	CaptureComponent->ShowFlags.SetPostProcessMaterial(false);

	// Start a timer to update capture position + capture at ~16 FPS
	World->GetTimerManager().SetTimer(CaptureUpdateTimer, this,
		&UMinimapSubsystem::UpdateCapturePosition, 0.0625f, true);

	UE_LOG(LogMinimap, Log, TEXT("Minimap overhead capture initialized (256x256, 16 FPS)"));
}

void UMinimapSubsystem::UpdateCapturePosition()
{
	if (!CaptureComponent || !CaptureActor) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!PlayerChar) return;

	FVector PlayerPos = PlayerChar->GetActorLocation();

	// Move the entire actor (root + capture component) above the player
	CaptureActor->SetActorLocation(FVector(PlayerPos.X, PlayerPos.Y, PlayerPos.Z + CaptureHeight));

	// Apply zoom
	int32 ClampedZoom = FMath::Clamp(ZoomLevel, 0, 4);
	float ZoomedWidth = CaptureOrthoWidth / ZoomFactors[ClampedZoom];
	CaptureComponent->OrthoWidth = ZoomedWidth;

	// Capture the scene
	CaptureComponent->CaptureScene();
}

void UMinimapSubsystem::CleanupCapture()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureUpdateTimer);
	}

	if (CaptureActor)
	{
		CaptureActor->Destroy();
		CaptureActor = nullptr;
	}
	CaptureComponent = nullptr;
	MinimapRenderTarget = nullptr;
}

void UMinimapSubsystem::LoadWorldMapTexture()
{
	// Load the world map texture from Content/SabriMMO/Textures/UI/
	static const TCHAR* WorldMapPath = TEXT("/Game/SabriMMO/Textures/UI/T_WorldMap_SabriMMO.T_WorldMap_SabriMMO");
	WorldMapTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, WorldMapPath));

	if (WorldMapTexture)
	{
		UE_LOG(LogMinimap, Log, TEXT("World map texture loaded: %s (%dx%d)"),
			*WorldMapTexture->GetName(),
			WorldMapTexture->GetSizeX(), WorldMapTexture->GetSizeY());
	}
	else
	{
		UE_LOG(LogMinimap, Warning, TEXT("Failed to load world map texture from: %s"), WorldMapPath);
	}
}

FVector2D UMinimapSubsystem::WorldToMinimapUV(const FVector& WorldPos) const
{
	// Convert UE5 world position to normalized 0-1 UV on the minimap
	const FVector Range = ZoneMaxBounds - ZoneMinBounds;
	if (Range.X < 1.f || Range.Y < 1.f) return FVector2D(0.5, 0.5);

	float U = (WorldPos.X - ZoneMinBounds.X) / Range.X;
	float V = 1.f - (WorldPos.Y - ZoneMinBounds.Y) / Range.Y;  // Flip Y: UE5 Y+ = north, minimap V+ = down

	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

const TArray<FZoneMapInfo::FWarpInfo>& UMinimapSubsystem::GetCurrentZoneWarps() const
{
	const FZoneMapInfo* Info = ZoneRegistry.Find(CurrentZoneName);
	if (Info)
	{
		return Info->Warps;
	}
	return EmptyWarps;
}

const FZoneMapInfo* UMinimapSubsystem::GetZoneInfo(const FString& ZoneName) const
{
	return ZoneRegistry.Find(ZoneName);
}

FString UMinimapSubsystem::GetGridZone(int32 Col, int32 Row) const
{
	if (Col < 0 || Col >= GridCols || Row < 0 || Row >= GridRows) return FString();
	int32 Idx = Row * GridCols + Col;
	if (Idx < 0 || Idx >= GridCells.Num()) return FString();
	return GridCells[Idx];
}

// ============================================================
// Event handlers
// ============================================================

void UMinimapSubsystem::HandleWorldMapData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* RootObj = nullptr;
	if (!Data->TryGetObject(RootObj) || !RootObj) return;

	// Parse grid dimensions
	double ColsD = 12, RowsD = 8;
	(*RootObj)->TryGetNumberField(TEXT("gridCols"), ColsD);
	(*RootObj)->TryGetNumberField(TEXT("gridRows"), RowsD);
	GridCols = (int32)ColsD;
	GridRows = (int32)RowsD;

	// Parse grid cells
	GridCells.Empty();
	GridCells.SetNum(GridCols * GridRows);
	const TArray<TSharedPtr<FJsonValue>>* GridArr = nullptr;
	if ((*RootObj)->TryGetArrayField(TEXT("grid"), GridArr))
	{
		for (int32 Row = 0; Row < GridArr->Num() && Row < GridRows; ++Row)
		{
			const TArray<TSharedPtr<FJsonValue>>* RowArr = nullptr;
			if ((*GridArr)[Row]->TryGetArray(RowArr))
			{
				for (int32 Col = 0; Col < RowArr->Num() && Col < GridCols; ++Col)
				{
					FString CellZone;
					if ((*RowArr)[Col]->TryGetString(CellZone))
					{
						GridCells[Row * GridCols + Col] = CellZone;
					}
					// null values remain empty string (ocean)
				}
			}
		}
	}

	// Parse zones
	ZoneRegistry.Empty();
	const TSharedPtr<FJsonObject>* ZonesObj = nullptr;
	if (!(*RootObj)->TryGetObjectField(TEXT("zones"), ZonesObj)) return;

	for (const auto& Pair : (*ZonesObj)->Values)
	{
		const FString& ZoneName = Pair.Key;
		const TSharedPtr<FJsonObject>* ZoneObj = nullptr;
		if (!Pair.Value->TryGetObject(ZoneObj) || !ZoneObj) continue;

		FZoneMapInfo Info;
		Info.Name = ZoneName;
		(*ZoneObj)->TryGetStringField(TEXT("displayName"), Info.DisplayName);
		(*ZoneObj)->TryGetStringField(TEXT("type"), Info.Type);
		(*ZoneObj)->TryGetStringField(TEXT("levelRange"), Info.LevelRange);
		(*ZoneObj)->TryGetStringField(TEXT("category"), Info.Category);

		// Bounds
		const TSharedPtr<FJsonObject>* BoundsObj = nullptr;
		if ((*ZoneObj)->TryGetObjectField(TEXT("bounds"), BoundsObj))
		{
			double Bx1 = 0, By1 = 0, Bx2 = 0, By2 = 0;
			(*BoundsObj)->TryGetNumberField(TEXT("x1"), Bx1);
			(*BoundsObj)->TryGetNumberField(TEXT("y1"), By1);
			(*BoundsObj)->TryGetNumberField(TEXT("x2"), Bx2);
			(*BoundsObj)->TryGetNumberField(TEXT("y2"), By2);
			Info.BoundsX1 = (float)Bx1;
			Info.BoundsY1 = (float)By1;
			Info.BoundsX2 = (float)Bx2;
			Info.BoundsY2 = (float)By2;
		}

		// Flags
		const TSharedPtr<FJsonObject>* FlagsObj = nullptr;
		if ((*ZoneObj)->TryGetObjectField(TEXT("flags"), FlagsObj))
		{
			(*FlagsObj)->TryGetBoolField(TEXT("town"), Info.bIsTown);
			bool bIndoor = false;
			(*FlagsObj)->TryGetBoolField(TEXT("indoor"), bIndoor);
			Info.bIsDungeon = bIndoor;
		}

		// Connections
		const TArray<TSharedPtr<FJsonValue>>* ConnsArr = nullptr;
		if ((*ZoneObj)->TryGetArrayField(TEXT("connections"), ConnsArr))
		{
			for (const auto& ConnVal : *ConnsArr)
			{
				FString Conn;
				if (ConnVal->TryGetString(Conn))
				{
					Info.Connections.Add(Conn);
				}
			}
		}

		// Monsters
		const TArray<TSharedPtr<FJsonValue>>* MonstersArr = nullptr;
		if ((*ZoneObj)->TryGetArrayField(TEXT("monsters"), MonstersArr))
		{
			for (const auto& MonVal : *MonstersArr)
			{
				const TSharedPtr<FJsonObject>* MonObj = nullptr;
				if (!MonVal->TryGetObject(MonObj)) continue;

				FZoneMapInfo::FMonsterInfo MonInfo;
				(*MonObj)->TryGetStringField(TEXT("name"), MonInfo.Name);
				double LevelD = 1;
				if ((*MonObj)->TryGetNumberField(TEXT("level"), LevelD))
				{
					MonInfo.Level = (int32)LevelD;
				}
				(*MonObj)->TryGetStringField(TEXT("element"), MonInfo.Element);
				Info.Monsters.Add(MonInfo);
			}
		}

		// Warps
		const TArray<TSharedPtr<FJsonValue>>* WarpsArr = nullptr;
		if ((*ZoneObj)->TryGetArrayField(TEXT("warps"), WarpsArr))
		{
			for (const auto& WarpVal : *WarpsArr)
			{
				const TSharedPtr<FJsonObject>* WarpObj = nullptr;
				if (!WarpVal->TryGetObject(WarpObj)) continue;

				FZoneMapInfo::FWarpInfo WarpInfo;
				(*WarpObj)->TryGetStringField(TEXT("id"), WarpInfo.Id);
				double X = 0, Y = 0, Z = 0;
				(*WarpObj)->TryGetNumberField(TEXT("x"), X);
				(*WarpObj)->TryGetNumberField(TEXT("y"), Y);
				(*WarpObj)->TryGetNumberField(TEXT("z"), Z);
				WarpInfo.X = (float)X;
				WarpInfo.Y = (float)Y;
				WarpInfo.Z = (float)Z;
				(*WarpObj)->TryGetStringField(TEXT("destZone"), WarpInfo.DestZone);
				(*WarpObj)->TryGetStringField(TEXT("destDisplayName"), WarpInfo.DestDisplayName);
				Info.Warps.Add(WarpInfo);
			}
		}

		ZoneRegistry.Add(ZoneName, Info);
	}

	// Update zone bounds for coordinate conversion from current zone's warp positions
	if (const FZoneMapInfo* CurrentInfo = ZoneRegistry.Find(CurrentZoneName))
	{
		// Estimate zone bounds from warp positions + padding
		float MinX = -2000.f, MaxX = 2000.f;
		float MinY = -2000.f, MaxY = 2000.f;
		for (const auto& Warp : CurrentInfo->Warps)
		{
			MinX = FMath::Min(MinX, Warp.X - 500.f);
			MaxX = FMath::Max(MaxX, Warp.X + 500.f);
			MinY = FMath::Min(MinY, Warp.Y - 500.f);
			MaxY = FMath::Max(MaxY, Warp.Y + 500.f);
		}
		ZoneMinBounds = FVector(MinX, MinY, 0.f);
		ZoneMaxBounds = FVector(MaxX, MaxY, 1000.f);
	}

	UE_LOG(LogMinimap, Log, TEXT("World map data loaded — %d zones"), ZoneRegistry.Num());
}

void UMinimapSubsystem::HandlePartyPositions(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data->TryGetObject(Obj) || !Obj) return;

	// Server sends { members: [{ characterId, characterName, map, x, y, z }] }
	const TArray<TSharedPtr<FJsonValue>>* MembersArr = nullptr;
	if ((*Obj)->TryGetArrayField(TEXT("members"), MembersArr))
	{
		for (const auto& MemberVal : *MembersArr)
		{
			const TSharedPtr<FJsonObject>* MemberObj = nullptr;
			if (!MemberVal->TryGetObject(MemberObj)) continue;

			FPartyMemberZone PMZ;
			double CharIdD = 0;
			if ((*MemberObj)->TryGetNumberField(TEXT("characterId"), CharIdD))
			{
				PMZ.CharacterId = (int32)CharIdD;
			}
			(*MemberObj)->TryGetStringField(TEXT("characterName"), PMZ.PlayerName);
			(*MemberObj)->TryGetStringField(TEXT("map"), PMZ.ZoneName);

			// Look up display name from zone registry
			if (const FZoneMapInfo* ZoneInfo = ZoneRegistry.Find(PMZ.ZoneName))
			{
				PMZ.ZoneDisplayName = ZoneInfo->DisplayName;
			}
			else
			{
				PMZ.ZoneDisplayName = PMZ.ZoneName;
			}

			// Update or add
			bool bFound = false;
			for (auto& Existing : PartyMemberZones)
			{
				if (Existing.CharacterId == PMZ.CharacterId)
				{
					Existing = PMZ;
					bFound = true;
					break;
				}
			}
			if (!bFound && PMZ.CharacterId > 0)
			{
				PartyMemberZones.Add(PMZ);
			}
		}
	}
}

void UMinimapSubsystem::HandleMinimapMark(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data->TryGetObject(Obj) || !Obj) return;

	double TypeD = 0;
	(*Obj)->TryGetNumberField(TEXT("type"), TypeD);
	int32 Type = (int32)TypeD;

	double IdD = 0;
	(*Obj)->TryGetNumberField(TEXT("id"), IdD);
	int32 MarkId = (int32)IdD;

	if (Type == 2)
	{
		// Remove mark by ID
		MinimapMarks.RemoveAll([MarkId](const FMinimapMark& M) { return M.Id == MarkId; });
		return;
	}

	FMinimapMark Mark;
	Mark.Id = MarkId;
	double X = 0, Y = 0;
	(*Obj)->TryGetNumberField(TEXT("x"), X);
	(*Obj)->TryGetNumberField(TEXT("y"), Y);
	Mark.X = (float)X;
	Mark.Y = (float)Y;

	double ColorD = 0;
	if ((*Obj)->TryGetNumberField(TEXT("color"), ColorD))
	{
		uint32 ColorHex = (uint32)ColorD;
		float R = ((ColorHex >> 16) & 0xFF) / 255.f;
		float G = ((ColorHex >> 8) & 0xFF) / 255.f;
		float B = (ColorHex & 0xFF) / 255.f;
		Mark.Color = FLinearColor(R, G, B, 1.f);
	}

	double DurationD = -1;
	(*Obj)->TryGetNumberField(TEXT("duration"), DurationD);
	Mark.RemainingTime = (float)DurationD;
	Mark.bPersistent = (Type == 1);

	MinimapMarks.Add(Mark);
}
