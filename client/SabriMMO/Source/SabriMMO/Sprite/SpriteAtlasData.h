// SpriteAtlasData.h — Structs for sprite atlas metadata and animation state
#pragma once

#include "CoreMinimal.h"
#include "SpriteAtlasData.generated.h"

/** Animation states matching RO Classic */
UENUM(BlueprintType)
enum class ESpriteAnimState : uint8
{
	Idle,
	Walk,
	Attack,
	CastSingle,   // targetType=single (e.g., Heal, Sonic Blow)
	CastSelf,     // targetType=self (e.g., Blessing, Adrenaline Rush)
	CastGround,   // targetType=ground (e.g., Storm Gust, traps)
	CastAoe,      // targetType=aoe (e.g., Grand Cross, Frost Nova)
	Hit,
	Death,
	Sit,
	Pickup,
	Block,
	MAX UMETA(Hidden)
};

/** Weapon mode — determines which weapon atlas is active */
UENUM(BlueprintType)
enum class ESpriteWeaponMode : uint8
{
	None = 0,    // Unarmed atlas
	OneHand,     // 1H weapon atlas
	TwoHand,     // 2H weapon atlas
	Bow,         // Bow atlas
	MAX UMETA(Hidden)
};

/** 8 directions matching RO Classic (S=facing camera) */
UENUM(BlueprintType)
enum class ESpriteDirection : uint8
{
	S = 0,   // South (facing camera)
	SW = 1,
	W = 2,
	NW = 3,
	N = 4,   // North (facing away)
	NE = 5,
	E = 6,
	SE = 7,
	MAX UMETA(Hidden)
};

/** Per-animation variant metadata within an atlas */
USTRUCT(BlueprintType)
struct FSpriteAnimInfo
{
	GENERATED_BODY()

	/** First row in the atlas grid for this animation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartRow = 0;

	/** Number of frames in this animation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FrameCount = 1;

	/** Seconds per frame (lower = faster) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FrameDuration = 0.2f;

	/** Whether this animation loops (idle, walk) or plays once (attack, death) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLoops = false;

	/** Source FBX name (informational, from JSON "source" field) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Source;
};

/** Per-animation entry with variant support — multiple animations for the same game state */
USTRUCT(BlueprintType)
struct FSpriteAnimVariants
{
	GENERATED_BODY()

	/** All variants for this animation state (index 0 is default) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSpriteAnimInfo> Variants;

	/** Get a specific variant (clamped to valid range) */
	const FSpriteAnimInfo& GetVariant(int32 Index) const
	{
		int32 Clamped = FMath::Clamp(Index, 0, FMath::Max(0, Variants.Num() - 1));
		return Variants[Clamped];
	}

	int32 NumVariants() const { return Variants.Num(); }
	bool HasVariants() const { return Variants.Num() > 1; }
};

/** Full atlas metadata — loaded from JSON or configured in editor */
USTRUCT(BlueprintType)
struct FSpriteAtlasInfo
{
	GENERATED_BODY()

	/** The atlas texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* AtlasTexture = nullptr;

	/** Grid dimensions (columns, rows) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint GridSize = FIntPoint(8, 21);

	/** Cell size in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint CellSize = FIntPoint(256, 256);

	/** Weapon group name (shared, unarmed, onehand, twohand) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString WeaponGroup;

	/** Per-animation metadata with variant support */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESpriteAnimState, FSpriteAnimVariants> Animations;

	/** Check if this atlas has a particular animation */
	bool HasAnimation(ESpriteAnimState Anim) const
	{
		const FSpriteAnimVariants* Vars = Animations.Find(Anim);
		return Vars && Vars->Variants.Num() > 0;
	}

	/** Get UV offset for a specific direction + animation + frame + variant */
	FVector2D GetUVOffset(ESpriteDirection Dir, ESpriteAnimState Anim,
	                      int32 Frame, int32 VariantIdx = 0) const
	{
		const FSpriteAnimVariants* Vars = Animations.Find(Anim);
		if (!Vars || Vars->Variants.Num() == 0)
			return FVector2D::ZeroVector;

		const FSpriteAnimInfo& Info = Vars->GetVariant(VariantIdx);
		int32 Col = static_cast<int32>(Dir);
		int32 Row = Info.StartRow + FMath::Clamp(Frame, 0, Info.FrameCount - 1);

		return FVector2D(
			static_cast<float>(Col) / GridSize.X,
			static_cast<float>(Row) / GridSize.Y
		);
	}

	/** Get UV scale (size of one cell in UV space) */
	FVector2D GetUVScale() const
	{
		return FVector2D(1.0f / GridSize.X, 1.0f / GridSize.Y);
	}
};

// ============================================================
// V2: Per-animation atlas (one atlas = one animation)
// ============================================================

/** Single-animation atlas metadata (v2: one atlas file per animation) */
USTRUCT(BlueprintType)
struct FSingleAnimAtlasInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* AtlasTexture = nullptr;

	/** UE5 asset path for lazy loading (e.g., /Game/SabriMMO/Sprites/...) */
	UPROPERTY()
	FString AssetPath;

	/** Grid is always (8, FrameCount) — 8 directions, N frames */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint GridSize = FIntPoint(8, 8);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FrameCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Source;

	/** Per-frame depth ordering: true = weapon in front of body, false = behind.
	 *  Indexed as [Frame * 8 + Direction]. Empty = always in front (default). */
	TArray<bool> DepthFront;

	/** Lazy-load the texture from AssetPath if not already loaded */
	void EnsureTextureLoaded()
	{
		if (!AtlasTexture && !AssetPath.IsEmpty())
		{
			AtlasTexture = Cast<UTexture2D>(
				StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath));
		}
	}

	/** Check if weapon should render in front of body for this frame+direction */
	bool IsDepthFront(ESpriteDirection Dir, int32 Frame) const
	{
		if (DepthFront.Num() == 0) return true; // No data = default front
		int32 Idx = Frame * 8 + static_cast<int32>(Dir);
		if (Idx < 0 || Idx >= DepthFront.Num()) return true;
		return DepthFront[Idx];
	}

	FVector2D GetUVOffset(ESpriteDirection Dir, int32 Frame) const
	{
		int32 Col = static_cast<int32>(Dir);
		int32 Row = FMath::Clamp(Frame, 0, FMath::Max(0, FrameCount - 1));
		return FVector2D(
			static_cast<float>(Col) / GridSize.X,
			static_cast<float>(Row) / GridSize.Y
		);
	}

	FVector2D GetUVScale() const
	{
		return FVector2D(1.0f / GridSize.X, 1.0f / GridSize.Y);
	}
};

/** Composite key for the v2 atlas registry: (weapon mode, anim state) */
struct FSpriteAtlasKey
{
	ESpriteWeaponMode WeaponMode;
	ESpriteAnimState AnimState;

	bool operator==(const FSpriteAtlasKey& Other) const
	{
		return WeaponMode == Other.WeaponMode && AnimState == Other.AnimState;
	}

	friend uint32 GetTypeHash(const FSpriteAtlasKey& Key)
	{
		return HashCombine(
			GetTypeHash(static_cast<uint8>(Key.WeaponMode)),
			GetTypeHash(static_cast<uint8>(Key.AnimState))
		);
	}
};

// Camera depth offset applied to sprite quads (must match SpriteCharacterActor.cpp)
// Zero because sprites now use BLEND_Translucent + bDisableDepthTest (always on top)
static constexpr float GSpriteCameraDepthOffset = 0.f;

/** Utility: compute sprite screen bounds using billboard up vector (not world up) */
static bool GetSpriteScreenBounds(APlayerController* PC, const FVector& ActorPos,
	float SpriteHeight, FVector2D& OutTop, FVector2D& OutBottom)
{
	if (!PC || !PC->PlayerCameraManager) return false;

	// Account for camera depth offset — quad is pushed toward camera in local X
	FVector CamFwd = PC->PlayerCameraManager->GetCameraRotation().Vector();
	FVector OffsetPos = ActorPos - CamFwd * GSpriteCameraDepthOffset;

	// Project sprite bottom (offset position where quad actually renders)
	if (!PC->ProjectWorldLocationToScreen(OffsetPos, OutBottom, true))
		return false;

	// Compute billboard up direction (matches SpriteCharacterActor::UpdateBillboard)
	FRotator BillboardRot = (-CamFwd).Rotation();
	BillboardRot.Roll = 0.f;
	FVector BillboardUp = BillboardRot.RotateVector(FVector(0.f, 0.f, 1.f));

	// Sprite top in world space = offset pos + billboard up * height
	FVector TopWorld = OffsetPos + BillboardUp * SpriteHeight;
	if (!PC->ProjectWorldLocationToScreen(TopWorld, OutTop, true))
		return false;

	return true;
}

/** Equipment layer types (rendering order, back to front) */
UENUM(BlueprintType)
enum class ESpriteLayer : uint8
{
	Shadow = 0,
	Body,
	Hair,
	HeadgearLow,
	HeadgearMid,
	HeadgearTop,
	Garment,
	Weapon,
	Shield,
	MAX UMETA(Hidden)
};
