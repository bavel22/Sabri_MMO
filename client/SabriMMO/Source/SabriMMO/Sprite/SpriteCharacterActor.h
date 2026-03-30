// SpriteCharacterActor.h — Billboard sprite character with layered equipment (RO Classic style)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "SpriteAtlasData.h"
#include "SpriteCharacterActor.generated.h"

class UMaterialInstanceDynamic;

/**
 * A single sprite layer (body, hair, weapon, etc.)
 * Rendered as a procedural quad with UV manipulation for atlas cell selection.
 */
USTRUCT()
struct FSpriteLayerState
{
	GENERATED_BODY()

	UPROPERTY()
	UProceduralMeshComponent* MeshComp = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MaterialInst = nullptr;

	FSpriteAtlasInfo AtlasInfo;  // legacy single-atlas (mage_f compat)
	bool bActive = false;
	FLinearColor TintColor = FLinearColor::White;

	/** Per-animation atlas registry for this layer (equipment layers) */
	TMap<FSpriteAtlasKey, TArray<FSingleAnimAtlasInfo>> LayerAtlasRegistry;
	FSingleAnimAtlasInfo* ActiveLayerAtlas = nullptr;
	UPROPERTY()
	UTexture2D* ActiveLayerTexture = nullptr;
	bool bUsingLayerV2 = false;
};

/**
 * Sprite character actor — RO Classic style billboard with layered equipment.
 *
 * Uses ProceduralMeshComponent per layer so we can update UVs per frame
 * to select the correct atlas cell. Supports dual-atlas system for body:
 * shared atlas (weapon-independent) + weapon atlas (weapon-specific).
 * Animation variants are randomly selected on loop restart.
 */
UCLASS()
class SABRIMMO_API ASpriteCharacterActor : public AActor
{
	GENERATED_BODY()

public:
	ASpriteCharacterActor();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// --- Layer Management ---

	/** Set atlas for a non-body layer. */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetLayerAtlas(ESpriteLayer Layer, UTexture2D* AtlasTexture,
	                   FIntPoint GridSize,
	                   const TMap<ESpriteAnimState, FSpriteAnimVariants>& Animations);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetLayerVisible(ESpriteLayer Layer, bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetLayerTint(ESpriteLayer Layer, FLinearColor Color);

	// --- Animation Control ---

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetAnimState(ESpriteAnimState NewState);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetFacingDirection(const FVector& WorldForward);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetWeaponMode(ESpriteWeaponMode Mode);

	ESpriteAnimState GetAnimState() const { return CurrentAnimState; }
	ESpriteWeaponMode GetWeaponMode() const { return CurrentWeaponMode; }
	bool IsBodyReady() const { return Layers[static_cast<int32>(ESpriteLayer::Body)].bActive; }

	// --- Quick Setup (loads atlases from content) ---

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetBodyClass(int32 ClassId, int32 Gender);

	/** Set body class by atlas name directly (e.g. "skeleton" for enemies) */
	void SetBodyClass(const FString& AtlasBaseName);

	/** Load equipment sprite layer from manifest (public for OtherPlayerSubsystem) */
	void LoadEquipmentLayer(ESpriteLayer Layer, int32 ViewSpriteId);

	/** Set hair style and color — loads hair atlas and applies tint */
	void SetHairStyle(int32 HairStyleId, int32 HairColorIndex);

	/** Reset hair hiding flag before equipment refresh (call before LoadEquipmentLayer loop) */
	void ResetHairHiding();

	/** Reconcile hair visibility after equipment refresh (call after LoadEquipmentLayer loop) */
	void ReconcileHairVisibility();

	/** Get hair color as FLinearColor from RO Classic 9-color palette (index 0-8) */
	static FLinearColor GetHairColor(int32 ColorIndex);

	/** Map equip slot name to sprite layer */
	static ESpriteLayer EquipSlotToSpriteLayer(const FString& Slot);

	/** Billboard size in world units (width, height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite")
	FVector2D SpriteSize = FVector2D(150.f, 150.f);

	// --- Owner Tracking (attach to a character) ---

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void AttachToOwnerActor(AActor* Actor, bool bIsLocalPlayer = false, int32 CharacterId = 0);

	static ASpriteCharacterActor* SpawnTestSpriteAt(UWorld* World, FVector Location);

	/** Spawn a sprite with a specific class (no hardcoded test atlas) */
	static ASpriteCharacterActor* SpawnSpriteForClass(UWorld* World, FVector Location,
	                                                   int32 ClassId, int32 Gender);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Enable click targeting on the body sprite mesh (visibility trace only) */
	void EnableClickCollision();

	/** Disable click targeting (corpse stays visible but not clickable) */
	void DisableClickCollision();

protected:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* RootComp;

	/** All equipment layers */
	FSpriteLayerState Layers[static_cast<int32>(ESpriteLayer::MAX)];

private:
	// --- Animation state ---
	ESpriteAnimState CurrentAnimState = ESpriteAnimState::Idle;
	ESpriteDirection CurrentDirection = ESpriteDirection::S;
	int32 CurrentFrame = 0;
	float FrameTimer = 0.0f;
	FVector FacingDir = FVector::ForwardVector;

	// --- Atlas system version flag ---
	bool bUsingV2Atlas = false;
	ESpriteWeaponMode CurrentWeaponMode = ESpriteWeaponMode::None;

	/** Gender for equipment atlas subfolder search: "male" or "female" */
	FString GenderSubDir = TEXT("male");

	// --- Hair layer state ---
	int32 CurrentHairStyle = 0;
	int32 CurrentHairColor = 0;
	bool bHairHiddenByHeadgear = false;

	// --- V2: Per-animation atlas registry ---
	TMap<FSpriteAtlasKey, TArray<FSingleAnimAtlasInfo>> AtlasRegistry;

	FSingleAnimAtlasInfo* ActiveAtlas = nullptr;
	int32 ActiveVariantIndex = 0;

	// --- V1 LEGACY: Dual atlas system ---
	FSpriteAtlasInfo SharedAtlas;
	bool bSharedAtlasLoaded = false;
	FSpriteAtlasInfo WeaponAtlases[static_cast<int32>(ESpriteWeaponMode::MAX)];
	bool bWeaponAtlasLoaded[static_cast<int32>(ESpriteWeaponMode::MAX)] = {};
	FSpriteAtlasInfo* ActiveWeaponAtlas = nullptr;
	TMap<ESpriteAnimState, int32> ActiveVariants;

	/** Currently active body texture (for texture swap tracking) */
	UPROPERTY()
	UTexture2D* ActiveBodyTexture = nullptr;

	// --- Owner tracking ---
	UPROPERTY()
	TWeakObjectPtr<AActor> OwnerActor;

public:
	/** Z offset to lower sprite from owner actor's location (set by EnemySubsystem for ground-snap) */
	float GroundZOffset = 0.f;

	/** C++ server-driven movement (replaces BP Tick interpolation for sprite enemies) */
	void SetServerTargetPosition(const FVector& Pos, bool bMoving, float Speed);
	FVector ServerTargetPos = FVector::ZeroVector;
	float ServerMoveSpeed = 200.f;
	bool bUseServerMovement = false;  // true for enemies, false for players
	int32 LocalCharacterId = 0;

	// --- Internal methods ---
	void UpdateOwnerTracking();
	void RegisterCombatEvents();
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);

	void CreateLayerQuad(int32 LayerIndex);
	void UpdateQuadUVs(FSpriteLayerState& Layer);
	void UpdateBodyQuadUVs();
	void UpdateBillboard();
	void UpdateDirection();
	void UpdateAnimation(float DeltaTime);
	void UpdateAllLayers();
	void UpdateBodyTexture();

	ESpriteDirection CalculateDirection() const;
	float GetFrameDuration() const;
	int32 GetFrameCount() const;

	/** V1: Resolve which atlas + variant provides the current body animation */
	const FSpriteAnimInfo* ResolveBodyAnimation(ESpriteAnimState State, int32& OutVariantIdx) const;

	/** V1: Pick a random variant for a looping animation */
	void SelectRandomVariant(ESpriteAnimState State);

	/** V2: Load atlas registry from manifest JSON */
	void LoadV2AtlasManifest(const FString& ManifestPath);

	/** V2: Parse a single-animation atlas JSON. AssetSubPath = UE5 path relative to /Game/SabriMMO/Sprites/Atlases/ */
	FSingleAnimAtlasInfo ParseSingleAtlasJSON(const FString& JsonStr, const FString& AtlasName,
	                                          const FString& AssetSubPath = TEXT("Body"));

	/** V2: Resolve the active atlas for current weapon mode + anim state, swap texture */
	void ResolveActiveAtlas();

	/** V2: Pick random variant for the current (mode, state) */
	void SelectRandomV2Variant();

	/** Equipment layer: resolve active atlas for a non-body layer based on current state */
	void ResolveLayerAtlas(FSpriteLayerState& Layer);

	/** Equipment layer: resolve all active equipment layer atlases */
	void ResolveAllEquipmentAtlases();

	/** Get atlas subdirectory for an equipment layer type */
	static FString GetLayerSubDir(ESpriteLayer Layer);

	/** Check if a state loops or plays once */
	static bool IsLoopingState(ESpriteAnimState S);
	static bool IsRevertState(ESpriteAnimState S);

	/** Get class name string from class ID */
	static FString GetClassNameFromId(int32 ClassId);

public:
	/** Convert job class string (e.g., "swordsman") to class ID (e.g., 100) */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	static int32 JobClassToId(const FString& JobClass);

private:

	/** Parse atlas JSON file into FSpriteAtlasInfo */
	FSpriteAtlasInfo ParseAtlasJSON(const FString& JsonStr, const FString& AtlasName);

	UMaterialInstanceDynamic* CreateSpriteMaterial(UTexture2D* Texture);
};
