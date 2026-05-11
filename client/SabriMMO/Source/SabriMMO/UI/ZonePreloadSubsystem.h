// ZonePreloadSubsystem.h — Async preload of sprite atlases for the current zone.
//
// Holds FStreamableHandles to keep loaded UTexture2D atlases resident, releases
// them on zone change. Uses an LRU cache to avoid reloading recently-visited
// zones. Pins the local player's class so it never reloads. Coordinates with
// ZoneTransitionSubsystem to keep the loading screen up until preload completes.
//
// Public API is invoked from EnemySubsystem (enemy:spawn) and OtherPlayerSubsystem
// (player:moved) — they tell us "this class is needed in the current zone" and we
// async-load all 17 atlases for that class on background threads.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "UObject/SoftObjectPath.h"
#include "Sprite/SpriteAtlasData.h"
#include "Dom/JsonValue.h"
#include "ZonePreloadSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnZonePreloadComplete);

/**
 * UZonePreloadSubsystem
 *
 * Per-world subsystem that batch-preloads sprite atlas textures for the current
 * zone. Sprites can spawn before atlases finish loading — atlas paths are
 * registered immediately, but actual texture loading happens asynchronously on
 * worker threads. The first state-change in a sprite hits a fast cache lookup
 * once preload completes (no GameThread blocking).
 */
UCLASS()
class SABRIMMO_API UZonePreloadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Public API ----

	/** Request preload of every atlas for a body class (e.g., "priest_f", "skeleton").
	 *  Idempotent — if already loaded or in-flight, no-op. Async, returns immediately. */
	void RequestClassPreload(const FString& SpriteClass);

	/** Request preload of every atlas for an equipment layer (Weapon, Hair, etc).
	 *  GenderSubDir is "male" or "female" or "" (empty = legacy/genderless). */
	void RequestLayerPreload(ESpriteLayer Layer, int32 ViewSpriteId,
	                         const FString& GenderSubDir);

	/** Pin a class so it's never evicted (used for local player). */
	void PinClass(const FString& SpriteClass);

	/** Called when entering a new zone: drop active classes, demote them to LRU cache. */
	void BeginZone(const FString& NewZoneName);

	/** Called when entering a new zone with the local player's class — pins it. */
	void SetLocalPlayerClass(const FString& SpriteClass);

	/** True if any preload is still in flight (used by ZoneTransitionSubsystem to
	 *  keep the loading screen up until preload completes). */
	bool IsLoadingInProgress() const { return InFlightCount > 0; }

	/** Fires once when InFlightCount transitions to 0 (all preloads done). */
	FOnZonePreloadComplete OnAllPreloadsComplete;

	/** Total in-flight load count (for debug overlay). */
	int32 GetInFlightCount() const { return InFlightCount; }

	/** Number of classes currently resident (active + pinned + LRU). */
	int32 GetResidentClassCount() const;

	/** Approximate VRAM usage estimate in bytes. */
	int64 GetApproxResidentBytes() const { return ApproxResidentBytes; }

private:
	// ---- Tier 1: Pinned (never evicted) ----
	UPROPERTY()
	TSet<FString> PinnedClasses;
	TMap<FString, TSharedPtr<FStreamableHandle>> PinnedHandles;

	// ---- Tier 2: Active zone (released on BeginZone) ----
	UPROPERTY()
	TSet<FString> ActiveZoneClasses;
	TMap<FString, TSharedPtr<FStreamableHandle>> ActiveHandles;

	// ---- Tier 3: LRU cache (evicted on memory pressure) ----
	struct FCachedClassEntry
	{
		TSharedPtr<FStreamableHandle> Handle;
		double LastUsedTime = 0.0;
		int64 ApproxBytes = 0;
	};
	TMap<FString, FCachedClassEntry> LruCache;
	int64 ApproxResidentBytes = 0;

	// LRU cache budget — defaults to 4 GB. Eviction happens when total exceeds.
	static constexpr int64 LruCacheBudgetBytes = 4LL * 1024 * 1024 * 1024;

	// ---- In-flight loads (avoid duplicate requests) ----
	TSet<FString> InFlightClasses;
	int32 InFlightCount = 0;

	// ---- Cached parsed manifests (avoid re-parsing JSON for same class) ----
	struct FResolvedClass
	{
		TArray<FSoftObjectPath> AssetPaths;
		int64 EstimatedBytes = 0;
	};
	TMap<FString, FResolvedClass> ClassPathsCache;

	// ---- Internal helpers ----

	/** Locate the manifest file for a sprite class (Body subdirs + enemies/). */
	FString FindClassManifestPath(const FString& SpriteClass) const;

	/** Locate the manifest for an equipment layer (Weapon/<item>/<gender>/). */
	FString FindLayerManifestPath(ESpriteLayer Layer, int32 ViewSpriteId,
	                              const FString& GenderSubDir) const;

	/** Parse manifest JSON, derive list of FSoftObjectPath for every atlas. */
	bool ResolveAssetPaths(const FString& ManifestPath, FResolvedClass& OutResolved) const;

	/** Start the async load for a resolved class. Returns the handle. */
	TSharedPtr<FStreamableHandle> StartAsyncLoad(const FString& ClassKey,
	                                              const FResolvedClass& Resolved);

	/** Fires when a class's load completes — moves to active tier, decrements counter. */
	void OnClassLoaded(FString ClassKey);

	/** Move a class's handle from one tier to another. */
	void DemoteActiveToLru(const FString& SpriteClass);
	bool TryPromoteFromLru(const FString& SpriteClass);

	/** Evict LRU entries until total bytes drops below budget. Pinned + Active never evicted. */
	void EvictLruIfOverBudget();

	/** Get appropriate prefix for an equipment layer cache key. */
	FString MakeLayerKey(ESpriteLayer Layer, int32 ViewSpriteId,
	                     const FString& GenderSubDir) const;

	/** Track current zone name for logging / future predictive preload. */
	FString CurrentZoneName;

	/** Local player's class (always pinned). */
	FString LocalPlayerClass;

	/** Server event handler — receives adjacent zone class lists for predictive preload. */
	void HandleAdjacentClasses(const TSharedPtr<FJsonValue>& Data);

	/** Async load priority — kept high so zone preload finishes quickly. */
	static constexpr TAsyncLoadPriority PreloadPriority = FStreamableManager::AsyncLoadHighPriority;
};
