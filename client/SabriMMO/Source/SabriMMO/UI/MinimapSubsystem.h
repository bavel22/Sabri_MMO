// MinimapSubsystem.h — UWorldSubsystem managing the minimap + world map.
// Registers for map:world_data and zone events via persistent EventRouter.
// Owns both SMinimapWidget (corner overlay) and SWorldMapWidget (fullscreen).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "MinimapSubsystem.generated.h"

class SMinimapWidget;
class SWorldMapWidget;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

// Zone data for minimap + world map display
struct FZoneMapInfo
{
	FString Name;
	FString DisplayName;
	FString Type;           // town, field, dungeon
	FString LevelRange;
	FString Category;

	// World map bounding box (normalized 0-1)
	float BoundsX1 = 0.f;
	float BoundsY1 = 0.f;
	float BoundsX2 = 0.f;
	float BoundsY2 = 0.f;

	// Connections to other zones
	TArray<FString> Connections;

	// Monster summary
	struct FMonsterInfo
	{
		FString Name;
		int32 Level = 1;
		FString Element;
	};
	TArray<FMonsterInfo> Monsters;

	// Warp positions (for minimap red dots)
	struct FWarpInfo
	{
		FString Id;
		float X = 0.f;
		float Y = 0.f;
		float Z = 0.f;
		FString DestZone;
		FString DestDisplayName;
	};
	TArray<FWarpInfo> Warps;

	// Map flags
	bool bIsTown = false;
	bool bIsDungeon = false;
};

// Party member zone location (for world map cross-zone tracking)
struct FPartyMemberZone
{
	int32 CharacterId = 0;
	FString PlayerName;
	FString ZoneName;
	FString ZoneDisplayName;
};

// Minimap mark (from Guide NPCs or server viewpoint command)
struct FMinimapMark
{
	int32 Id = 0;
	float X = 0.f;
	float Y = 0.f;
	FLinearColor Color = FLinearColor::White;
	float RemainingTime = -1.f;  // <0 = persistent
	bool bPersistent = false;
};

UCLASS()
class SABRIMMO_API UMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- Public data (read by widgets) ----

	// World map texture (loaded at startup)
	UPROPERTY()
	UTexture2D* WorldMapTexture = nullptr;

	// Minimap overhead camera render target (live top-down view)
	UPROPERTY()
	UTextureRenderTarget2D* MinimapRenderTarget = nullptr;

	// All zones in the game
	TMap<FString, FZoneMapInfo> ZoneRegistry;

	// Grid system: COLS x ROWS, each cell = zone name or empty
	int32 GridCols = 12;
	int32 GridRows = 8;
	TArray<FString> GridCells;  // Flat array: index = row * GridCols + col

	// Get zone name at grid position (empty string = ocean)
	FString GetGridZone(int32 Col, int32 Row) const;

	// Local player ID (for filtering self out of party dots)
	int32 LocalCharacterId = 0;

	// Current zone name
	FString CurrentZoneName;
	FString CurrentDisplayName;

	// Party member positions (cross-zone, for world map)
	TArray<FPartyMemberZone> PartyMemberZones;

	// Minimap marks (Guide NPC + server viewpoint)
	TArray<FMinimapMark> MinimapMarks;

	// Minimap state
	int32 OpacityState = 2;   // 0=hidden, 1=50%, 2=100%
	int32 ZoomLevel = 0;      // 0-4

	// Zoom factors (public so widget can access for projection)
	static constexpr float ZoomFactors[5] = { 1.f, 1.8f, 3.f, 5.f, 8.f };

	// World map state
	bool bWorldMapOpen = false;
	bool bShowZoneNames = false;
	bool bShowMonsterInfo = false;

	// ---- Public API ----
	void CycleMinimapOpacity();
	void ZoomIn();
	void ZoomOut();
	void ToggleWorldMap();
	void CloseWorldMap();
	void ToggleZoneNames();
	void ToggleMonsterInfo();

	// Coordinate conversion: world position -> minimap UV (0-1)
	FVector2D WorldToMinimapUV(const FVector& WorldPos) const;

	// Get warp positions for current zone
	const TArray<FZoneMapInfo::FWarpInfo>& GetCurrentZoneWarps() const;

	// Get the zone info for a given zone name
	const FZoneMapInfo* GetZoneInfo(const FString& ZoneName) const;

	// ---- Lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	// ---- Event handlers ----
	void HandleWorldMapData(const TSharedPtr<FJsonValue>& Data);
	void HandlePartyPositions(const TSharedPtr<FJsonValue>& Data);
	void HandleMinimapMark(const TSharedPtr<FJsonValue>& Data);

	void LoadWorldMapTexture();

	// ---- Widget management ----
	void ShowMinimap();
	void HideMinimap();
	void ShowWorldMap();
	void HideWorldMap();

	// ---- State ----
	bool bMinimapAdded = false;
	bool bWorldMapAdded = false;

	TSharedPtr<SMinimapWidget> MinimapWidget;
	TSharedPtr<SWidget> MinimapAlignmentWrapper;
	TSharedPtr<SWidget> MinimapViewportOverlay;

	TSharedPtr<SWorldMapWidget> WorldMapWidgetPtr;
	TSharedPtr<SWidget> WorldMapAlignWrapper;
	TSharedPtr<SWidget> WorldMapViewportOverlay;

	// Overhead capture camera (spawned as actor in the world)
	UPROPERTY()
	AActor* CaptureActor = nullptr;

	UPROPERTY()
	USceneCaptureComponent2D* CaptureComponent = nullptr;

	void SetupOverheadCapture();
	void UpdateCapturePosition();
	void CleanupCapture();

	FTimerHandle CaptureUpdateTimer;

	// Capture settings
	float CaptureHeight = 5000.f;       // How high above player
	float CaptureOrthoWidth = 4000.f;   // Orthographic width (zoom level 0 = widest)

	// Zone bounds for coordinate conversion (UE5 level bounds)
	FVector ZoneMinBounds = FVector(-3000.f, -3000.f, 0.f);
	FVector ZoneMaxBounds = FVector(3000.f, 3000.f, 1000.f);

	// Empty warps for default return
	TArray<FZoneMapInfo::FWarpInfo> EmptyWarps;
};
