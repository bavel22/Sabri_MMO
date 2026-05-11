// SpawnRegionVolumes.h — Editor-placeable box volumes that mark spawn allow/deny areas.
// Place ASpawnAllowVolume actors where enemies SHOULD spawn, ASpawnDenyVolume actors where they should NOT.
// Drag, scale, and (optionally) tag them in the editor, then run console command "ExportSpawnRegions <zone_name>".
// The exported JSON is consumed server-side by ro_spawn_regions.js to generate random spawn points.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnRegionVolumes.generated.h"

class UBoxComponent;

/**
 * Abstract base for spawn region volumes. Wraps a UBoxComponent for editor visualization.
 * The box is editor-only (hidden in game). Rotation is ignored on export — the volume's
 * world-space axis-aligned bounding box is what gets serialized to JSON.
 */
// Base is Abstract (cannot be instantiated directly). Do NOT add NotPlaceable here —
// in UE5 NotPlaceable is inherited by subclasses unless they explicitly override with
// Placeable, which would hide ASpawnAllowVolume / ASpawnDenyVolume from the Place Actors panel.
UCLASS(Abstract)
class SABRIMMO_API ASpawnRegionVolume : public AActor
{
	GENERATED_BODY()

public:
	ASpawnRegionVolume();

	UPROPERTY(VisibleAnywhere, Category = "Spawn Region")
	UBoxComponent* BoxComp;

	/**
	 * Optional whitelist of monster template names that may spawn in this region.
	 * Empty array = all templates allowed. Only meaningful for ASpawnAllowVolume;
	 * ASpawnDenyVolume rejects all templates regardless of filter.
	 *
	 * Example: ["poring", "fabre"] — only Porings and Fabres spawn in this allow box.
	 */
	UPROPERTY(EditAnywhere, Category = "Spawn Region")
	TArray<FString> MonsterFilter;

	/** Optional free-form tag written to the JSON export (useful for debugging logs). */
	UPROPERTY(EditAnywhere, Category = "Spawn Region")
	FString RegionTag;
};

/**
 * Allow region — random spawn points are generated inside this volume.
 * Multiple allow volumes are supported per level; selection is weighted by their XY area.
 * Wireframe is green in editor.
 */
UCLASS(Placeable)
class SABRIMMO_API ASpawnAllowVolume : public ASpawnRegionVolume
{
	GENERATED_BODY()

public:
	ASpawnAllowVolume();
};

/**
 * Deny region — any random point that lands inside this volume is rejected.
 * Used to carve exclusions out of allow regions (warp portal, NPC spot, quest area).
 * Wireframe is red in editor.
 */
UCLASS(Placeable)
class SABRIMMO_API ASpawnDenyVolume : public ASpawnRegionVolume
{
	GENERATED_BODY()

public:
	ASpawnDenyVolume();
};
