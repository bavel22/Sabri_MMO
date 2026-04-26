// SpriteCharacterActor.cpp — Billboard sprite character with layered equipment
#include "SpriteCharacterActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionSceneColor.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "UI/EquipmentSubsystem.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"

// Global sprite LOD bias — driven by Options > Video > Sprite Quality.
// Read by FSingleAnimAtlasInfo::EnsureTextureLoaded() when textures load.
int32 FSingleAnimAtlasInfo::GlobalLODBias = 0;

// State name → enum mapping for JSON parsing
static const TMap<FString, ESpriteAnimState> StateNameMap = {
	{TEXT("idle"),         ESpriteAnimState::Idle},
	{TEXT("walk"),         ESpriteAnimState::Walk},
	{TEXT("attack"),       ESpriteAnimState::Attack},
	{TEXT("cast_single"),  ESpriteAnimState::CastSingle},
	{TEXT("cast_self"),    ESpriteAnimState::CastSelf},
	{TEXT("cast_ground"),  ESpriteAnimState::CastGround},
	{TEXT("cast_aoe"),     ESpriteAnimState::CastAoe},
	{TEXT("hit"),          ESpriteAnimState::Hit},
	{TEXT("death"),        ESpriteAnimState::Death},
	{TEXT("sit"),          ESpriteAnimState::Sit},
	{TEXT("pickup"),       ESpriteAnimState::Pickup},
	{TEXT("block"),        ESpriteAnimState::Block},
};

ASpriteCharacterActor::ASpriteCharacterActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);
}

void ASpriteCharacterActor::EnableClickCollision()
{
	// Enable visibility-trace collision on the body mesh so clicks register on the sprite
	int32 BodyIdx = static_cast<int32>(ESpriteLayer::Body);
	FSpriteLayerState& Body = Layers[BodyIdx];
	if (Body.MeshComp)
	{
		Body.MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Body.MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		Body.MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Body.MeshComp->SetGenerateOverlapEvents(false);
		Body.MeshComp->SetCanEverAffectNavigation(false);
	}
}

void ASpriteCharacterActor::DisableClickCollision()
{
	int32 BodyIdx = static_cast<int32>(ESpriteLayer::Body);
	FSpriteLayerState& Body = Layers[BodyIdx];
	if (Body.MeshComp)
	{
		Body.MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ASpriteCharacterActor::PlayHitFlash()
{
	if (bHitFlashing) return;
	bHitFlashing = true;
	HitFlashTimer = 0.0f;
	SavedLayerTints.Empty();

	// Save original tints and flash all active layers to bright white
	for (int32 i = 0; i < static_cast<int32>(ESpriteLayer::MAX); ++i)
	{
		if (Layers[i].bActive && Layers[i].MaterialInst)
		{
			SavedLayerTints.Add(i, Layers[i].TintColor);
			Layers[i].MaterialInst->SetVectorParameterValue(
				TEXT("TintColor"), FLinearColor(3.0f, 3.0f, 3.0f, 1.0f));
		}
	}
}

void ASpriteCharacterActor::BeginPlay()
{
	Super::BeginPlay();

	// Pre-create layer mesh components
	for (int32 i = 0; i < static_cast<int32>(ESpriteLayer::MAX); i++)
	{
		ESpriteLayer LayerType = static_cast<ESpriteLayer>(i);
		if (LayerType == ESpriteLayer::Shadow)
			continue;

		CreateLayerQuad(i);
	}

	// Blob shadow — circular decal projected downward onto the ground
	BlobShadow = NewObject<UDecalComponent>(this, TEXT("BlobShadow"));
	if (BlobShadow)
	{
		BlobShadow->SetupAttachment(RootComp);
		BlobShadow->RegisterComponent();
		BlobShadow->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // project downward
		BlobShadow->DecalSize = FVector(64.f, 48.f, 48.f);
		BlobShadow->SetRelativeLocation(FVector(0.f, 0.f, 5.f)); // just above ground

		// Try editor asset first, then create at runtime
		UMaterialInterface* ShadowMat = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(), nullptr,
			TEXT("/Game/SabriMMO/Materials/M_BlobShadow")));

		if (!ShadowMat)
		{
			// Create blob shadow material at runtime (deferred decal, radial gradient)
			UMaterial* Mat = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
			Mat->MaterialDomain = MD_DeferredDecal;
			Mat->BlendMode = BLEND_Translucent;
			Mat->DecalBlendMode = DBM_Translucent;

			// Custom HLSL: radial gradient from center, dark circle
			auto* Custom = NewObject<UMaterialExpressionCustom>(Mat);
			Custom->OutputType = CMOT_Float1;
			Custom->Code = TEXT(
				"float2 Centered = UV - float2(0.5, 0.5);\n"
				"float Dist = length(Centered) * 2.0;\n"
				"float Alpha = saturate(1.0 - Dist) * 0.5;\n"
				"// Smooth circle fade\n"
				"Alpha *= Alpha;\n"
				"return Alpha;\n"
			);

			// TextureCoordinate as UV input
			auto* TexCoord = NewObject<UMaterialExpressionTextureCoordinate>(Mat);
			Mat->GetExpressionCollection().AddExpression(TexCoord);

			FCustomInput& In0 = Custom->Inputs.AddDefaulted_GetRef();
			In0.InputName = TEXT("UV");
			In0.Input.Connect(0, TexCoord);

			Mat->GetExpressionCollection().AddExpression(Custom);
			Mat->GetEditorOnlyData()->Opacity.Connect(0, Custom);

			// Emissive = black (shadow)
			auto* BlackColor = NewObject<UMaterialExpressionVectorParameter>(Mat);
			BlackColor->ParameterName = TEXT("ShadowColor");
			BlackColor->DefaultValue = FLinearColor(0.f, 0.f, 0.f, 1.f);
			Mat->GetExpressionCollection().AddExpression(BlackColor);
			Mat->GetEditorOnlyData()->EmissiveColor.Connect(0, BlackColor);

			Mat->PreEditChange(nullptr);
			Mat->PostEditChange();
			ShadowMat = Mat;
		}

		BlobShadow->SetDecalMaterial(ShadowMat);
	}

	UE_LOG(LogTemp, Warning, TEXT("=== SpriteCharacter BeginPlay === Location: %s"),
		*GetActorLocation().ToString());

	// Body class is set by SpawnSpriteForClass() or SetBodyClass() — not here.
	// BeginPlay only creates the quad geometry; the caller assigns the atlas.
}

// ============================================================
// Procedural Quad — one per layer, UVs updated per frame
// ============================================================

// Use shared constant from SpriteAtlasData.h (GSpriteCameraDepthOffset)
// so GetSpriteScreenBounds projects from the same offset position
static constexpr float SpriteDepthOffset = GSpriteCameraDepthOffset;


void ASpriteCharacterActor::CreateLayerQuad(int32 LayerIndex)
{
	FSpriteLayerState& Layer = Layers[LayerIndex];

	FString CompName = FString::Printf(TEXT("SpriteLayer_%d"), LayerIndex);
	Layer.MeshComp = NewObject<UProceduralMeshComponent>(this, *CompName);
	Layer.MeshComp->SetupAttachment(RootComp);
	Layer.MeshComp->RegisterComponent();
	Layer.MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Layer.MeshComp->CastShadow = false;
	Layer.MeshComp->bReceivesDecals = false;
	Layer.MeshComp->SetVisibility(false);

	// Mark sprites with Custom Stencil so the post-process outline pass skips them
	Layer.MeshComp->SetRenderCustomDepth(true);
	Layer.MeshComp->SetCustomDepthStencilValue(1);

	float HalfW = SpriteSize.X * 0.5f;
	float HalfH = SpriteSize.Y * 0.5f;

	TArray<FVector> Vertices;
	Vertices.Add(FVector(SpriteDepthOffset, -HalfW, 0.f));       // Bottom-left
	Vertices.Add(FVector(SpriteDepthOffset, HalfW, 0.f));         // Bottom-right
	Vertices.Add(FVector(SpriteDepthOffset, HalfW, HalfH * 2.f)); // Top-right
	Vertices.Add(FVector(SpriteDepthOffset, -HalfW, HalfH * 2.f));// Top-left

	TArray<int32> Triangles;
	Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);
	Triangles.Add(0); Triangles.Add(2); Triangles.Add(3);

	TArray<FVector> Normals;
	Normals.Init(FVector(1, 0, 0), 4);

	TArray<FVector2D> UVs;
	UVs.Add(FVector2D(0, 1));
	UVs.Add(FVector2D(1, 1));
	UVs.Add(FVector2D(1, 0));
	UVs.Add(FVector2D(0, 0));

	TArray<FColor> Colors;
	Colors.Init(FColor::White, 4);

	TArray<FProcMeshTangent> Tangents;
	Tangents.Init(FProcMeshTangent(0, 1, 0), 4);

	// bCreateCollision=true so EnableClickCollision() has geometry to trace against
	Layer.MeshComp->bUseComplexAsSimpleCollision = true;
	Layer.MeshComp->CreateMeshSection(0, Vertices, Triangles, Normals,
	                                   UVs, Colors, Tangents, true);

	// Back-to-front layer ordering via translucent sort distance offset.
	// Higher layer index = more negative offset = appears closer = drawn LAST (on top).
	// All 9 layers share the actor position, so this is the only ordering discriminator.
	// Small magnitudes (-0.1 per layer) avoid corrupting cross-character sort.
	Layer.MeshComp->TranslucencySortPriority = 0;
	Layer.MeshComp->TranslucencySortDistanceOffset = -0.1f * static_cast<float>(LayerIndex);
}

void ASpriteCharacterActor::UpdateQuadUVs(FSpriteLayerState& Layer)
{
	if (!Layer.MeshComp || !Layer.bActive)
		return;

	FVector2D UVOffset, UVScale;

	if (Layer.bUsingLayerV2)
	{
		// Per-animation atlas path (equipment layers)
		if (!Layer.ActiveLayerAtlas || !Layer.ActiveLayerAtlas->AtlasTexture)
		{
			// Current animation not available — hide layer (e.g., weapon during sit)
			Layer.MeshComp->SetVisibility(false);
			return;
		}
		// Don't re-show hair if headgear is hiding it
		if (&Layer == &Layers[static_cast<int32>(ESpriteLayer::Hair)] && bHairHiddenByHeadgear)
		{
			Layer.MeshComp->SetVisibility(false);
			return;
		}
		Layer.MeshComp->SetVisibility(true);
		UVOffset = Layer.ActiveLayerAtlas->GetUVOffset(CurrentDirection, CurrentFrame);
		UVScale = Layer.ActiveLayerAtlas->GetUVScale();
	}
	else
	{
		// Legacy single-atlas path
		if (!Layer.AtlasInfo.HasAnimation(CurrentAnimState))
		{
			Layer.MeshComp->SetVisibility(false);
			return;
		}
		Layer.MeshComp->SetVisibility(true);

		int32 VariantIdx = 0;
		const int32* VIdx = ActiveVariants.Find(CurrentAnimState);
		if (VIdx) VariantIdx = *VIdx;

		UVOffset = Layer.AtlasInfo.GetUVOffset(
			CurrentDirection, CurrentAnimState, CurrentFrame, VariantIdx);
		UVScale = Layer.AtlasInfo.GetUVScale();
	}

	float U0 = UVOffset.X;
	float V0 = UVOffset.Y;
	float U1 = UVOffset.X + UVScale.X;
	float V1 = UVOffset.Y + UVScale.Y;

	// Swap U0/U1 to flip horizontally — compensates for billboard rotation
	// negating local Y axis (Yaw=180° makes +Y point screen-left instead of right)
	TArray<FVector2D> NewUVs;
	NewUVs.Add(FVector2D(U1, V1));
	NewUVs.Add(FVector2D(U0, V1));
	NewUVs.Add(FVector2D(U0, V0));
	NewUVs.Add(FVector2D(U1, V0));

	float HalfW = SpriteSize.X * 0.5f;
	float HalfH = SpriteSize.Y * 0.5f;

	// Sit Z-offset applies to ALL layers (matches body)
	float ZOffset = 0.f;
	if (CurrentAnimState == ESpriteAnimState::Sit)
	{
		ZOffset = -HalfH * 0.6f;
	}

	// Per-frame depth ordering for equipment layers via translucent sort distance
	if (Layer.bUsingLayerV2 && Layer.MeshComp)
	{
		bool bFront = true; // default: in front of body
		if (Layer.ActiveLayerAtlas && Layer.ActiveLayerAtlas->DepthFront.Num() > 0)
		{
			bFront = Layer.ActiveLayerAtlas->IsDepthFront(CurrentDirection, CurrentFrame);
		}
		// Base per-layer offset (-0.1 * index) ± equipment front/back adjustment (0.5)
		int32 LayerIdx = static_cast<int32>(&Layer - Layers);
		float BaseOffset = -0.1f * static_cast<float>(LayerIdx);
		float FrontBackAdj = bFront ? -0.5f : 0.5f;
		Layer.MeshComp->TranslucencySortDistanceOffset = BaseOffset + FrontBackAdj;
	}

	TArray<FVector> Vertices;
	Vertices.Add(FVector(SpriteDepthOffset, -HalfW, ZOffset));
	Vertices.Add(FVector(SpriteDepthOffset, HalfW, ZOffset));
	Vertices.Add(FVector(SpriteDepthOffset, HalfW, HalfH * 2.f + ZOffset));
	Vertices.Add(FVector(SpriteDepthOffset, -HalfW, HalfH * 2.f + ZOffset));

	TArray<FVector> Normals;
	Normals.Init(FVector(1, 0, 0), 4);

	Layer.MeshComp->UpdateMeshSection(0, Vertices, Normals, NewUVs,
	                                   TArray<FColor>(), TArray<FProcMeshTangent>());
}

void ASpriteCharacterActor::UpdateBodyQuadUVs()
{
	FSpriteLayerState& Body = Layers[static_cast<int32>(ESpriteLayer::Body)];
	if (!Body.MeshComp || !Body.bActive)
	{
		static bool bLoggedOnce = false;
		if (!bLoggedOnce)
		{
			UE_LOG(LogTemp, Warning, TEXT("UpdateBodyQuadUVs: SKIPPED — MeshComp=%d bActive=%d"),
				Body.MeshComp ? 1 : 0, Body.bActive ? 1 : 0);
			bLoggedOnce = true;
		}
		return;
	}

	FVector2D UVOffset, UVScale;

	if (bUsingV2Atlas && ActiveAtlas)
	{
		// V2: one atlas per animation — simple direct lookup
		UVOffset = ActiveAtlas->GetUVOffset(CurrentDirection, CurrentFrame);
		UVScale = ActiveAtlas->GetUVScale();
	}
	else
	{
		// V1: dual atlas system
		int32 VariantIdx = 0;
		const FSpriteAnimInfo* Info = ResolveBodyAnimation(CurrentAnimState, VariantIdx);

		if (Info)
		{
			FSpriteAtlasInfo* SourceAtlas = nullptr;
			if (ActiveWeaponAtlas && ActiveWeaponAtlas->HasAnimation(CurrentAnimState))
				SourceAtlas = ActiveWeaponAtlas;
			else if (bSharedAtlasLoaded && SharedAtlas.HasAnimation(CurrentAnimState))
				SourceAtlas = &SharedAtlas;
			else if (bWeaponAtlasLoaded[0] && WeaponAtlases[0].HasAnimation(CurrentAnimState))
				SourceAtlas = &WeaponAtlases[0];

			if (!SourceAtlas) { UpdateQuadUVs(Body); return; }

			UVOffset = SourceAtlas->GetUVOffset(
				CurrentDirection, CurrentAnimState, CurrentFrame, VariantIdx);
			UVScale = SourceAtlas->GetUVScale();
		}
		else if (Body.AtlasInfo.HasAnimation(CurrentAnimState))
		{
			UVOffset = Body.AtlasInfo.GetUVOffset(
				CurrentDirection, CurrentAnimState, CurrentFrame, 0);
			UVScale = Body.AtlasInfo.GetUVScale();
		}
		else
		{
			UpdateQuadUVs(Body);
			return;
		}
	}

	float U0 = UVOffset.X;
	float V0 = UVOffset.Y;
	float U1 = UVOffset.X + UVScale.X;
	float V1 = UVOffset.Y + UVScale.Y;

	// Swap U0/U1 to flip horizontally — compensates for billboard rotation
	// negating local Y axis (Yaw=180° makes +Y point screen-left instead of right)
	TArray<FVector2D> NewUVs;
	NewUVs.Add(FVector2D(U1, V1));
	NewUVs.Add(FVector2D(U0, V1));
	NewUVs.Add(FVector2D(U0, V0));
	NewUVs.Add(FVector2D(U1, V0));

	float HalfW = SpriteSize.X * 0.5f;
	float HalfH = SpriteSize.Y * 0.5f;

	// Shift quad down when sitting so the seated character rests on the ground
	// instead of floating at standing-height. The sitting sprite occupies less
	// vertical space, so we lower the quad by ~30% of sprite height.
	float ZOffset = 0.f;
	if (CurrentAnimState == ESpriteAnimState::Sit)
	{
		ZOffset = -HalfH * 0.6f;
	}

	TArray<FVector> Vertices;
	Vertices.Add(FVector(SpriteDepthOffset, -HalfW, ZOffset));
	Vertices.Add(FVector(SpriteDepthOffset, HalfW, ZOffset));
	Vertices.Add(FVector(SpriteDepthOffset, HalfW, HalfH * 2.f + ZOffset));
	Vertices.Add(FVector(SpriteDepthOffset, -HalfW, HalfH * 2.f + ZOffset));

	TArray<FVector> Normals;
	Normals.Init(FVector(1, 0, 0), 4);

	Body.MeshComp->UpdateMeshSection(0, Vertices, Normals, NewUVs,
	                                  TArray<FColor>(), TArray<FProcMeshTangent>());
}

// ============================================================
// Material
// ============================================================

UMaterialInstanceDynamic* ASpriteCharacterActor::CreateSpriteMaterial(UTexture2D* Texture)
{
	if (!Texture) return nullptr;

	UMaterial* Mat = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	Mat->MaterialDomain = MD_Surface;
	Mat->BlendMode = BLEND_Translucent;                  // Required for bDisableDepthTest
	Mat->SetShadingModel(MSM_Unlit);
	Mat->TwoSided = true;
	Mat->bDisableDepthTest = 1;                          // Ignore scene depth — always render on top
	Mat->AllowTranslucentCustomDepthWrites = 1;          // Keep custom depth stencil = 1 pipeline
	Mat->TranslucencyLightingMode = TLM_VolumetricNonDirectional;
	Mat->OpacityMaskClipValue = 0.333f;
	Mat->bEnableResponsiveAA = 1;                        // Reduce TAA smoothing on sharp alpha edges

	UMaterialExpressionTextureSampleParameter2D* TexSample =
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Mat);
	TexSample->ParameterName = TEXT("Atlas");
	TexSample->Texture = Texture;
	TexSample->SamplerType = SAMPLERTYPE_Color;
	Mat->GetExpressionCollection().AddExpression(TexSample);

	// TintColor parameter (default White = no tint, used by Hair layer for color)
	UMaterialExpressionVectorParameter* TintParam =
		NewObject<UMaterialExpressionVectorParameter>(Mat);
	TintParam->ParameterName = TEXT("TintColor");
	TintParam->DefaultValue = FLinearColor::White;
	Mat->GetExpressionCollection().AddExpression(TintParam);

	// Multiply texture RGB by TintColor
	UMaterialExpressionMultiply* Multiply =
		NewObject<UMaterialExpressionMultiply>(Mat);
	Multiply->A.Connect(0, TexSample);
	Multiply->B.Connect(0, TintParam);
	Mat->GetExpressionCollection().AddExpression(Multiply);

	// FeetOccluded — line trace from camera to actor feet. 1 = feet behind a wall.
	UMaterialExpressionScalarParameter* FeetOccluded =
		NewObject<UMaterialExpressionScalarParameter>(Mat);
	FeetOccluded->ParameterName = TEXT("FeetOccluded");
	FeetOccluded->DefaultValue = 0.f;
	Mat->GetExpressionCollection().AddExpression(FeetOccluded);

	// HeadOccluded — line trace from camera to top of sprite. 1 = head also behind a wall.
	UMaterialExpressionScalarParameter* HeadOccluded =
		NewObject<UMaterialExpressionScalarParameter>(Mat);
	HeadOccluded->ParameterName = TEXT("HeadOccluded");
	HeadOccluded->DefaultValue = 0.f;
	Mat->GetExpressionCollection().AddExpression(HeadOccluded);

	// SceneColor: opaque scene color at the current pixel UV (the wall behind sprite)
	UMaterialExpressionSceneColor* SceneColorNode =
		NewObject<UMaterialExpressionSceneColor>(Mat);
	Mat->GetExpressionCollection().AddExpression(SceneColorNode);

	// PixelDepth and SceneDepth for per-pixel "is this pixel behind a wall" check
	// (used in the partial-occlusion case where feet are blocked but head isn't)
	UMaterialExpressionPixelDepth* PixelDepthNode = NewObject<UMaterialExpressionPixelDepth>(Mat);
	Mat->GetExpressionCollection().AddExpression(PixelDepthNode);

	UMaterialExpressionSceneDepth* SceneDepthNode = NewObject<UMaterialExpressionSceneDepth>(Mat);
	Mat->GetExpressionCollection().AddExpression(SceneDepthNode);

	// Opacity: just the binary alpha. Always write the sprite stencil, regardless
	// of occlusion (so the post-process can read it).
	UMaterialExpressionCustom* OpacityCompute = NewObject<UMaterialExpressionCustom>(Mat);
	OpacityCompute->OutputType = CMOT_Float1;
	OpacityCompute->Code = TEXT("return InAlpha > 0.333 ? 1.0 : 0.0;");
	FCustomInput& AlphaIn = OpacityCompute->Inputs.AddDefaulted_GetRef();
	AlphaIn.InputName = TEXT("InAlpha");
	AlphaIn.Input.Connect(4, TexSample);
	Mat->GetExpressionCollection().AddExpression(OpacityCompute);

	// Emissive: combines feet/head trace results with per-pixel depth check.
	// - Both head and feet occluded → entire sprite is silhouette (scene color)
	// - Only feet occluded → per-pixel: pixels behind walls = silhouette, above = sprite art
	// - Neither occluded → full sprite color
	UMaterialExpressionCustom* EmissiveCompute = NewObject<UMaterialExpressionCustom>(Mat);
	EmissiveCompute->OutputType = CMOT_Float3;
	EmissiveCompute->Code = TEXT(
		"float Behind = InPixelDepth - InSceneDepth;\n"
		"float PixelBehindWall = (Behind > 50.0) ? 1.0 : 0.0;\n"
		"// FinalOccluded = HeadOccluded ? 1 : (FeetOccluded ? PixelBehindWall : 0)\n"
		"float FinalOccluded = max(InHeadOccluded, InFeetOccluded * PixelBehindWall);\n"
		"return lerp(InSpriteColor.rgb, InSceneColor.rgb, FinalOccluded);"
	);
	FCustomInput& SpriteColIn = EmissiveCompute->Inputs.AddDefaulted_GetRef();
	SpriteColIn.InputName = TEXT("InSpriteColor");
	SpriteColIn.Input.Connect(0, Multiply);
	FCustomInput& SceneColIn = EmissiveCompute->Inputs.AddDefaulted_GetRef();
	SceneColIn.InputName = TEXT("InSceneColor");
	SceneColIn.Input.Connect(0, SceneColorNode);
	FCustomInput& FeetIn = EmissiveCompute->Inputs.AddDefaulted_GetRef();
	FeetIn.InputName = TEXT("InFeetOccluded");
	FeetIn.Input.Connect(0, FeetOccluded);
	FCustomInput& HeadIn = EmissiveCompute->Inputs.AddDefaulted_GetRef();
	HeadIn.InputName = TEXT("InHeadOccluded");
	HeadIn.Input.Connect(0, HeadOccluded);
	FCustomInput& PixDepthIn = EmissiveCompute->Inputs.AddDefaulted_GetRef();
	PixDepthIn.InputName = TEXT("InPixelDepth");
	PixDepthIn.Input.Connect(0, PixelDepthNode);
	FCustomInput& SceneDepthIn = EmissiveCompute->Inputs.AddDefaulted_GetRef();
	SceneDepthIn.InputName = TEXT("InSceneDepth");
	SceneDepthIn.Input.Connect(0, SceneDepthNode);
	Mat->GetExpressionCollection().AddExpression(EmissiveCompute);

	Mat->GetEditorOnlyData()->EmissiveColor.Connect(0, EmissiveCompute);
	Mat->GetEditorOnlyData()->Opacity.Connect(0, OpacityCompute);

	Mat->PreEditChange(nullptr);
	Mat->PostEditChange();

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
	if (MID)
	{
		MID->SetTextureParameterValue(TEXT("Atlas"), Texture);
	}
	return MID;
}

// ============================================================
// Billboard — face camera every tick
// ============================================================

void ASpriteCharacterActor::UpdateBillboard()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
		return;

	FVector CamFwd = PC->PlayerCameraManager->GetCameraRotation().Vector();
	FRotator FaceCamera = (-CamFwd).Rotation();
	SetActorRotation(FRotator(FaceCamera.Pitch, FaceCamera.Yaw, 0.f));
}

// ============================================================
// Direction — 8-dir from facing vs camera
// ============================================================

void ASpriteCharacterActor::UpdateDirection()
{
	ESpriteDirection NewDir = CalculateDirection();
	if (NewDir != CurrentDirection)
	{
		CurrentDirection = NewDir;
		UpdateAllLayers();
	}
}

ESpriteDirection ASpriteCharacterActor::CalculateDirection() const
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
		return ESpriteDirection::S;

	FVector CamFwd = PC->PlayerCameraManager->GetCameraRotation().Vector();
	FVector2D Facing2D(FacingDir.X, FacingDir.Y);
	FVector2D CamFwd2D(CamFwd.X, CamFwd.Y);

	if (Facing2D.IsNearlyZero() || CamFwd2D.IsNearlyZero())
		return ESpriteDirection::S;

	Facing2D.Normalize();
	CamFwd2D.Normalize();

	float Dot = FVector2D::DotProduct(Facing2D, CamFwd2D);
	float Cross = Facing2D.X * CamFwd2D.Y - Facing2D.Y * CamFwd2D.X;
	// -Dot flips front/back (character facing camera = S, not N)
	// Cross is NOT negated — UV flip in UpdateQuadUVs handles the billboard mirror
	float Angle = FMath::Atan2(Cross, -Dot);

	int32 Dir = FMath::RoundToInt(Angle / (PI / 4.0f));
	Dir = ((Dir % 8) + 8) % 8;
	return static_cast<ESpriteDirection>(Dir);
}

void ASpriteCharacterActor::SetFacingDirection(const FVector& WorldForward)
{
	FacingDir = WorldForward;
	FacingDir.Z = 0.f;
	if (!FacingDir.IsNearlyZero())
		FacingDir.Normalize();
}

// ============================================================
// Animation state machine
// ============================================================

bool ASpriteCharacterActor::IsLoopingState(ESpriteAnimState S)
{
	return S == ESpriteAnimState::Idle ||
	       S == ESpriteAnimState::Walk ||
	       S == ESpriteAnimState::Sit;
}

bool ASpriteCharacterActor::IsRevertState(ESpriteAnimState S)
{
	return S == ESpriteAnimState::Attack     ||
	       S == ESpriteAnimState::CastSingle ||
	       S == ESpriteAnimState::CastSelf   ||
	       S == ESpriteAnimState::CastGround ||
	       S == ESpriteAnimState::CastAoe    ||
	       S == ESpriteAnimState::Hit        ||
	       S == ESpriteAnimState::Pickup     ||
	       S == ESpriteAnimState::Block;
}

void ASpriteCharacterActor::UpdateAnimation(float DeltaTime)
{
	float Duration = GetFrameDuration();
	FrameTimer += DeltaTime;

	if (FrameTimer < Duration)
		return;

	FrameTimer -= Duration;
	CurrentFrame++;

	int32 MaxFrames = GetFrameCount();
	if (CurrentFrame >= MaxFrames)
	{
		if (IsLoopingState(CurrentAnimState))
		{
			CurrentFrame = 0;
			if (bUsingV2Atlas)
			{
				SelectRandomV2Variant();
				ResolveActiveAtlas();
			}
			else
			{
				SelectRandomVariant(CurrentAnimState);
			}

			// Frame-sync hook: fire whenever a looping animation cycle wraps.
			// EnemySubsystem listens for this on Walk to play the per-hop monster move sound
			// (RO Classic ACT-style "boing" cadence) in exact sync with the visual hop.
			OnAnimCycleComplete.Broadcast(CurrentAnimState);
		}
		else
		{
			CurrentFrame = MaxFrames - 1;
			if (IsRevertState(CurrentAnimState))
			{
				SetAnimState(ESpriteAnimState::Idle);
				return;
			}
		}
	}

	UpdateAllLayers();
}

float ASpriteCharacterActor::GetFrameDuration() const
{
	switch (CurrentAnimState)
	{
	case ESpriteAnimState::Idle:       return 0.25f;
	case ESpriteAnimState::Walk:       return 0.10f;
	case ESpriteAnimState::Attack:     return 0.08f;
	case ESpriteAnimState::CastSingle: return 0.15f;
	case ESpriteAnimState::CastSelf:   return 0.15f;
	case ESpriteAnimState::CastGround: return 0.15f;
	case ESpriteAnimState::CastAoe:    return 0.15f;
	case ESpriteAnimState::Hit:        return 0.10f;
	case ESpriteAnimState::Death:      return 0.20f;
	case ESpriteAnimState::Sit:        return 0.30f;
	case ESpriteAnimState::Pickup:     return 0.12f;
	case ESpriteAnimState::Block:      return 0.10f;
	default:                           return 0.20f;
	}
}

int32 ASpriteCharacterActor::GetFrameCount() const
{
	// V2: per-animation atlas
	if (bUsingV2Atlas && ActiveAtlas)
		return ActiveAtlas->FrameCount;

	// V1: Try dual atlas system first
	int32 VariantIdx = 0;
	const FSpriteAnimInfo* Info = ResolveBodyAnimation(CurrentAnimState, VariantIdx);
	if (Info) return Info->FrameCount;

	// Try legacy single-atlas layer info
	const FSpriteLayerState& Body = Layers[static_cast<int32>(ESpriteLayer::Body)];
	const FSpriteAnimVariants* LegacyVars = Body.AtlasInfo.Animations.Find(CurrentAnimState);
	if (LegacyVars && LegacyVars->Variants.Num() > 0)
		return LegacyVars->GetVariant(0).FrameCount;

	// Hardcoded fallback defaults
	switch (CurrentAnimState)
	{
	case ESpriteAnimState::Idle:       return 4;
	case ESpriteAnimState::Walk:       return 8;
	case ESpriteAnimState::Attack:     return 6;
	case ESpriteAnimState::Death:      return 3;
	case ESpriteAnimState::CastSingle: return 4;
	case ESpriteAnimState::CastSelf:   return 4;
	case ESpriteAnimState::CastGround: return 4;
	case ESpriteAnimState::CastAoe:    return 4;
	case ESpriteAnimState::Hit:        return 2;
	case ESpriteAnimState::Sit:        return 2;
	case ESpriteAnimState::Pickup:     return 4;
	case ESpriteAnimState::Block:      return 3;
	default:                           return 4;
	}
}

void ASpriteCharacterActor::SetAnimState(ESpriteAnimState NewState)
{
	if (CurrentAnimState == ESpriteAnimState::Death && NewState != ESpriteAnimState::Idle)
		return;

	// Always restart + pick new variant, even if same state (rapid casting, etc.)
	CurrentAnimState = NewState;
	CurrentFrame = 0;
	FrameTimer = 0.0f;

	if (bUsingV2Atlas)
	{
		SelectRandomV2Variant();
		ResolveActiveAtlas();
	}
	else
	{
		SelectRandomVariant(NewState);
		UpdateBodyTexture();
	}

	// Resolve equipment layer atlases (texture swap for new animation state)
	ResolveAllEquipmentAtlases();

	UpdateAllLayers();
}

// ============================================================
// Dual Atlas System — resolve body animation from weapon/shared
// ============================================================

const FSpriteAnimInfo* ASpriteCharacterActor::ResolveBodyAnimation(
	ESpriteAnimState State, int32& OutVariantIdx) const
{
	// Try weapon atlas first
	if (ActiveWeaponAtlas)
	{
		const FSpriteAnimVariants* WV = ActiveWeaponAtlas->Animations.Find(State);
		if (WV && WV->Variants.Num() > 0)
		{
			const int32* VIdx = ActiveVariants.Find(State);
			OutVariantIdx = VIdx ? *VIdx : 0;
			return &WV->GetVariant(OutVariantIdx);
		}
	}
	// Fall back to shared
	if (bSharedAtlasLoaded)
	{
		const FSpriteAnimVariants* SV = SharedAtlas.Animations.Find(State);
		if (SV && SV->Variants.Num() > 0)
		{
			const int32* VIdx = ActiveVariants.Find(State);
			OutVariantIdx = VIdx ? *VIdx : 0;
			return &SV->GetVariant(OutVariantIdx);
		}
	}
	// Fall back to unarmed atlas (e.g., 1H mode has no idle — use unarmed idle)
	if (bWeaponAtlasLoaded[0] && ActiveWeaponAtlas != &WeaponAtlases[0])
	{
		const FSpriteAnimVariants* UV = WeaponAtlases[0].Animations.Find(State);
		if (UV && UV->Variants.Num() > 0)
		{
			const int32* VIdx = ActiveVariants.Find(State);
			OutVariantIdx = VIdx ? *VIdx : 0;
			return &UV->GetVariant(OutVariantIdx);
		}
	}
	OutVariantIdx = 0;
	return nullptr;
}

void ASpriteCharacterActor::SelectRandomVariant(ESpriteAnimState State)
{
	int32 NumVars = 0;

	// Check weapon atlas first
	if (ActiveWeaponAtlas)
	{
		const FSpriteAnimVariants* WV = ActiveWeaponAtlas->Animations.Find(State);
		if (WV) NumVars = WV->NumVariants();
	}
	// Fall back to shared
	if (NumVars == 0 && bSharedAtlasLoaded)
	{
		const FSpriteAnimVariants* SV = SharedAtlas.Animations.Find(State);
		if (SV) NumVars = SV->NumVariants();
	}
	// Fall back to unarmed
	if (NumVars == 0 && bWeaponAtlasLoaded[0] && ActiveWeaponAtlas != &WeaponAtlases[0])
	{
		const FSpriteAnimVariants* UV = WeaponAtlases[0].Animations.Find(State);
		if (UV) NumVars = UV->NumVariants();
	}

	if (NumVars > 1)
	{
		ActiveVariants.FindOrAdd(State) = FMath::RandRange(0, NumVars - 1);
	}
	else
	{
		ActiveVariants.FindOrAdd(State) = 0;
	}
}

void ASpriteCharacterActor::UpdateBodyTexture()
{
	FSpriteLayerState& Body = Layers[static_cast<int32>(ESpriteLayer::Body)];
	if (!Body.MeshComp || !Body.MaterialInst)
		return;

	// Determine which atlas provides the current animation
	// Must match the same fallback order as ResolveBodyAnimation()
	UTexture2D* NewTexture = nullptr;
	if (ActiveWeaponAtlas && ActiveWeaponAtlas->HasAnimation(CurrentAnimState))
	{
		NewTexture = ActiveWeaponAtlas->AtlasTexture;
	}
	else if (bSharedAtlasLoaded && SharedAtlas.HasAnimation(CurrentAnimState))
	{
		NewTexture = SharedAtlas.AtlasTexture;
	}
	else if (bWeaponAtlasLoaded[0] && WeaponAtlases[0].HasAnimation(CurrentAnimState))
	{
		// Unarmed fallback (e.g., 1H mode has no idle)
		NewTexture = WeaponAtlases[0].AtlasTexture;
	}

	if (NewTexture && NewTexture != ActiveBodyTexture)
	{
		Body.MaterialInst->SetTextureParameterValue(TEXT("Atlas"), NewTexture);
		ActiveBodyTexture = NewTexture;
	}
}

void ASpriteCharacterActor::SetWeaponMode(ESpriteWeaponMode Mode)
{
	UE_LOG(LogTemp, Warning, TEXT("SetWeaponMode: requested=%d current=%d v2=%d charId=%d"),
		static_cast<int32>(Mode), static_cast<int32>(CurrentWeaponMode),
		bUsingV2Atlas ? 1 : 0, LocalCharacterId);

	if (Mode == CurrentWeaponMode)
		return;

	CurrentWeaponMode = Mode;

	if (bUsingV2Atlas)
	{
		SelectRandomV2Variant();
		ResolveActiveAtlas();
		UpdateAllLayers();
		UE_LOG(LogTemp, Warning, TEXT("SetWeaponMode: V2 resolved — mode=%d activeAtlas=%s"),
			static_cast<int32>(Mode), ActiveAtlas ? *ActiveAtlas->Source : TEXT("NULL"));
		return;
	}

	// V1 path
	int32 Idx = static_cast<int32>(Mode);

	if (Idx >= 0 && Idx < static_cast<int32>(ESpriteWeaponMode::MAX) && bWeaponAtlasLoaded[Idx])
	{
		ActiveWeaponAtlas = &WeaponAtlases[Idx];
	}
	else
	{
		ActiveWeaponAtlas = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: Weapon atlas not loaded for mode %d"), Idx);
	}

	ActiveVariants.Empty();
	UpdateBodyTexture();
	UpdateAllLayers();

	UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Weapon mode set to %d"), Idx);
}

// ============================================================
// Layer updates
// ============================================================

void ASpriteCharacterActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateOwnerTracking();
	UpdateBillboard();
	UpdateDirection();
	UpdateAnimation(DeltaTime);

	// Hit flash: restore original tints after flash duration
	if (bHitFlashing)
	{
		HitFlashTimer += DeltaTime;
		if (HitFlashTimer >= HIT_FLASH_DURATION)
		{
			for (auto& Pair : SavedLayerTints)
			{
				if (Pair.Key >= 0 && Pair.Key < static_cast<int32>(ESpriteLayer::MAX)
					&& Layers[Pair.Key].MaterialInst)
				{
					Layers[Pair.Key].MaterialInst->SetVectorParameterValue(
						TEXT("TintColor"), Pair.Value);
					Layers[Pair.Key].TintColor = Pair.Value;
				}
			}
			bHitFlashing = false;
		}
	}
}

void ASpriteCharacterActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetGameInstance()))
	{
		if (USocketEventRouter* Router = GI->GetEventRouter())
		{
			Router->UnregisterAllForOwner(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ASpriteCharacterActor::UpdateAllLayers()
{
	for (int32 i = 0; i < static_cast<int32>(ESpriteLayer::MAX); i++)
	{
		ESpriteLayer LayerType = static_cast<ESpriteLayer>(i);
		if (LayerType == ESpriteLayer::Body)
		{
			// Body uses dual atlas system
			UpdateBodyQuadUVs();
		}
		else if (Layers[i].bActive)
		{
			UpdateQuadUVs(Layers[i]);
		}
	}
}

// ============================================================
// Public API — set atlas, visibility, tint
// ============================================================

void ASpriteCharacterActor::SetLayerAtlas(ESpriteLayer Layer, UTexture2D* AtlasTexture,
                                          FIntPoint GridSize,
                                          const TMap<ESpriteAnimState, FSpriteAnimVariants>& Animations)
{
	int32 Idx = static_cast<int32>(Layer);
	if (Idx < 0 || Idx >= static_cast<int32>(ESpriteLayer::MAX))
		return;

	FSpriteLayerState& L = Layers[Idx];
	L.AtlasInfo.AtlasTexture = AtlasTexture;
	L.AtlasInfo.GridSize = GridSize;
	L.AtlasInfo.Animations = Animations;
	L.bActive = true;

	if (L.MeshComp)
	{
		L.MaterialInst = CreateSpriteMaterial(AtlasTexture);
		if (L.MaterialInst)
		{
			L.MeshComp->SetMaterial(0, L.MaterialInst);
		}
		L.MeshComp->SetVisibility(true);
	}

	if (Layer == ESpriteLayer::Body)
	{
		UpdateBodyQuadUVs();
	}
	else
	{
		UpdateQuadUVs(L);
	}
}

void ASpriteCharacterActor::SetLayerVisible(ESpriteLayer Layer, bool bVisible)
{
	int32 Idx = static_cast<int32>(Layer);
	if (Idx < 0 || Idx >= static_cast<int32>(ESpriteLayer::MAX))
		return;
	Layers[Idx].bActive = bVisible;
	if (Layers[Idx].MeshComp)
		Layers[Idx].MeshComp->SetVisibility(bVisible);
}

void ASpriteCharacterActor::SetLayerTint(ESpriteLayer Layer, FLinearColor Color)
{
	int32 Idx = static_cast<int32>(Layer);
	if (Idx < 0 || Idx >= static_cast<int32>(ESpriteLayer::MAX))
		return;
	Layers[Idx].TintColor = Color;
	if (Layers[Idx].MaterialInst)
		Layers[Idx].MaterialInst->SetVectorParameterValue(TEXT("TintColor"), Color);
}

// ============================================================
// Hair Style + Color
// ============================================================

FLinearColor ASpriteCharacterActor::GetHairColor(int32 ColorIndex)
{
	// RO Classic 9-color palette (indices match character create widget)
	static const FLinearColor Palette[] = {
		FLinearColor(0.10f, 0.07f, 0.05f, 1.f),  // 0 - Black
		FLinearColor(0.85f, 0.15f, 0.10f, 1.f),  // 1 - Scarlet
		FLinearColor(0.80f, 0.60f, 0.20f, 1.f),  // 2 - Lemon/Blonde
		FLinearColor(0.75f, 0.75f, 0.75f, 1.f),  // 3 - Silver/White
		FLinearColor(0.55f, 0.25f, 0.10f, 1.f),  // 4 - Orange/Brown
		FLinearColor(0.20f, 0.50f, 0.25f, 1.f),  // 5 - Green
		FLinearColor(0.35f, 0.35f, 0.65f, 1.f),  // 6 - Cobalt Blue
		FLinearColor(0.50f, 0.20f, 0.55f, 1.f),  // 7 - Violet/Purple
		FLinearColor(0.90f, 0.40f, 0.55f, 1.f),  // 8 - Pink
	};
	int32 Idx = FMath::Clamp(ColorIndex, 0, 8);
	return Palette[Idx];
}

void ASpriteCharacterActor::ResetHairHiding()
{
	bHairHiddenByHeadgear = false;
}

void ASpriteCharacterActor::ReconcileHairVisibility()
{
	if (CurrentHairStyle > 0)
	{
		int32 HairIdx = static_cast<int32>(ESpriteLayer::Hair);
		if (bHairHiddenByHeadgear)
		{
			SetLayerVisible(ESpriteLayer::Hair, false);
		}
		else if (Layers[HairIdx].bUsingLayerV2 && Layers[HairIdx].LayerAtlasRegistry.Num() > 0)
		{
			SetLayerVisible(ESpriteLayer::Hair, true);
			ResolveLayerAtlas(Layers[HairIdx]);
		}
	}
}

void ASpriteCharacterActor::SetHairStyle(int32 HairStyleId, int32 HairColorIndex)
{
	CurrentHairStyle = HairStyleId;
	CurrentHairColor = HairColorIndex;

	if (HairStyleId <= 0)
	{
		SetLayerVisible(ESpriteLayer::Hair, false);
		return;
	}

	LoadEquipmentLayer(ESpriteLayer::Hair, HairStyleId);

	// Apply tint color after material is created by LoadEquipmentLayer
	SetLayerTint(ESpriteLayer::Hair, GetHairColor(HairColorIndex));

	// Respect headgear hiding
	if (bHairHiddenByHeadgear)
	{
		SetLayerVisible(ESpriteLayer::Hair, false);
	}
}

// ============================================================
// JSON Atlas Loading
// ============================================================

FSpriteAtlasInfo ASpriteCharacterActor::ParseAtlasJSON(const FString& JsonStr, const FString& AtlasName)
{
	FSpriteAtlasInfo Result;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SpriteCharacter: Failed to parse JSON for %s"), *AtlasName);
		return Result;
	}

	// Grid size
	const TArray<TSharedPtr<FJsonValue>>* GridArr;
	if (Root->TryGetArrayField(TEXT("grid"), GridArr) && GridArr->Num() >= 2)
	{
		Result.GridSize.X = (*GridArr)[0]->AsNumber();
		Result.GridSize.Y = (*GridArr)[1]->AsNumber();
	}

	// Cell size
	const TArray<TSharedPtr<FJsonValue>>* CellArr;
	if (Root->TryGetArrayField(TEXT("cell_size"), CellArr) && CellArr->Num() >= 2)
	{
		Result.CellSize.X = (*CellArr)[0]->AsNumber();
		Result.CellSize.Y = (*CellArr)[1]->AsNumber();
	}

	// Weapon group
	Root->TryGetStringField(TEXT("weapon_group"), Result.WeaponGroup);

	// Load atlas texture from Content directory
	FString TexturePath = FString::Printf(
		TEXT("/Game/SabriMMO/Sprites/Atlases/Body/%s.%s"),
		*AtlasName, *AtlasName);
	Result.AtlasTexture = Cast<UTexture2D>(
		StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));

	if (!Result.AtlasTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: Atlas texture not found: %s"), *TexturePath);
	}

	// Parse animations with variant support
	const TSharedPtr<FJsonObject>* AnimsObj;
	if (Root->TryGetObjectField(TEXT("animations"), AnimsObj))
	{
		for (auto& Pair : (*AnimsObj)->Values)
		{
			const FString& StateName = Pair.Key;
			const ESpriteAnimState* StateEnum = StateNameMap.Find(StateName);
			if (!StateEnum)
			{
				UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: Unknown anim state '%s'"), *StateName);
				continue;
			}

			FSpriteAnimVariants Variants;

			// Read variants array
			const TSharedPtr<FJsonObject>& AnimObj = Pair.Value->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* VariantsArr;
			if (AnimObj.IsValid() && AnimObj->TryGetArrayField(TEXT("variants"), VariantsArr))
			{
				for (const auto& VarVal : *VariantsArr)
				{
					const TSharedPtr<FJsonObject>& VarObj = VarVal->AsObject();
					if (!VarObj.IsValid()) continue;

					FSpriteAnimInfo Info;
					Info.StartRow = VarObj->GetIntegerField(TEXT("start_row"));
					Info.FrameCount = VarObj->GetIntegerField(TEXT("frame_count"));
					VarObj->TryGetStringField(TEXT("source"), Info.Source);

					// Set default frame duration and loop from state type
					Info.bLoops = IsLoopingState(*StateEnum);
					Info.FrameDuration = GetFrameDuration();

					Variants.Variants.Add(Info);
				}
			}

			if (Variants.Variants.Num() > 0)
			{
				Result.Animations.Add(*StateEnum, Variants);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Parsed atlas '%s' — %d states, grid %dx%d"),
		*AtlasName, Result.Animations.Num(), Result.GridSize.X, Result.GridSize.Y);

	return Result;
}

FString ASpriteCharacterActor::GetClassNameFromId(int32 ClassId)
{
	switch (ClassId / 100)
	{
	case 0: return TEXT("novice");
	case 1: return TEXT("swordsman");
	case 2: return TEXT("mage");
	case 3: return TEXT("archer");
	case 4: return TEXT("acolyte");
	case 5: return TEXT("thief");
	case 6: return TEXT("merchant");
	case 7: return TEXT("knight");
	case 8: return TEXT("wizard");
	case 9: return TEXT("hunter");
	case 10: return TEXT("priest");
	case 11: return TEXT("assassin");
	case 12: return TEXT("blacksmith");
	case 13: return TEXT("crusader");
	case 14: return TEXT("sage");
	case 15: return TEXT("bard");
	case 16: return TEXT("monk");
	case 17: return TEXT("rogue");
	case 18: return TEXT("alchemist");
	case 19: return TEXT("dancer");
	default: return TEXT("novice");
	}
}

void ASpriteCharacterActor::SetBodyClass(int32 ClassId, int32 Gender)
{
	FString ClassName = GetClassNameFromId(ClassId);
	FString GenderSuffix = (Gender == 1) ? TEXT("f") : TEXT("m");
	FString BaseName = FString::Printf(TEXT("%s_%s"), *ClassName, *GenderSuffix);

	// Store gender for equipment layer subfolder search (male/ or female/)
	GenderSubDir = (Gender == 1) ? TEXT("female") : TEXT("male");

	FString BodyRoot = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases/Body");

	// --- Try V2 manifest first (per-animation atlases) ---
	// Try subfolder first (Body/{BaseName}/), then flat (Body/)
	FString JsonDir = BodyRoot / BaseName;
	FString ManifestPath = JsonDir / FString::Printf(TEXT("%s_manifest.json"), *BaseName);
	FString ManifestStr;
	if (!FFileHelper::LoadFileToString(ManifestStr, *ManifestPath))
	{
		// Fallback: flat structure
		JsonDir = BodyRoot;
		ManifestPath = JsonDir / FString::Printf(TEXT("%s_manifest.json"), *BaseName);
		FFileHelper::LoadFileToString(ManifestStr, *ManifestPath);
	}
	if (!ManifestStr.IsEmpty())
	{
		bUsingV2Atlas = true;
		LoadV2AtlasManifest(ManifestPath);
		return;
	}

	// --- V1: weapon-group atlases ---
	// Reset JsonDir to try subfolder for v1 too
	JsonDir = BodyRoot / BaseName;
	if (!FPaths::DirectoryExists(JsonDir))
		JsonDir = BodyRoot;

	static const TCHAR* Groups[] = { TEXT("shared"), TEXT("unarmed"), TEXT("onehand"), TEXT("twohand") };
	bool bJsonFound = false;

	for (int32 g = 0; g < 4; g++)
	{
		FString AtlasName = FString::Printf(TEXT("%s_%s"), *BaseName, Groups[g]);
		FString JsonPath = JsonDir / FString::Printf(TEXT("%s.json"), *AtlasName);

		FString JsonStr;
		if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: JSON not found: %s"), *JsonPath);
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Loaded JSON: %s (%d bytes)"),
			*JsonPath, JsonStr.Len());

		bJsonFound = true;
		FSpriteAtlasInfo Atlas = ParseAtlasJSON(JsonStr, AtlasName);

		// If texture didn't load from flat path, retry with subfolder path
		if (!Atlas.AtlasTexture)
		{
			FString SubfolderPath = FString::Printf(
				TEXT("/Game/SabriMMO/Sprites/Atlases/Body/%s/%s.%s"),
				*BaseName, *AtlasName, *AtlasName);
			Atlas.AtlasTexture = Cast<UTexture2D>(
				StaticLoadObject(UTexture2D::StaticClass(), nullptr, *SubfolderPath));
		}

		if (g == 0)
		{
			// Shared atlas
			SharedAtlas = Atlas;
			bSharedAtlasLoaded = Atlas.AtlasTexture != nullptr;
		}
		else
		{
			// Weapon atlases: 0=unarmed→None, 1=onehand→OneHand, 2=twohand→TwoHand
			int32 WeaponIdx = g - 1;
			WeaponAtlases[WeaponIdx] = Atlas;
			bWeaponAtlasLoaded[WeaponIdx] = Atlas.AtlasTexture != nullptr;
		}
	}

	if (bJsonFound)
	{
		// Set up body layer with initial material
		FSpriteLayerState& Body = Layers[static_cast<int32>(ESpriteLayer::Body)];

		// Force weapon mode setup (default is already None, so SetWeaponMode
		// would early-return — reset to MAX first to ensure it runs)
		CurrentWeaponMode = ESpriteWeaponMode::MAX;
		SetWeaponMode(ESpriteWeaponMode::None);

		// Create material from the first available atlas texture
		UTexture2D* InitialTexture = nullptr;
		if (ActiveWeaponAtlas && ActiveWeaponAtlas->AtlasTexture)
			InitialTexture = ActiveWeaponAtlas->AtlasTexture;
		else if (bSharedAtlasLoaded && SharedAtlas.AtlasTexture)
			InitialTexture = SharedAtlas.AtlasTexture;

		UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: InitialTexture=%s, ActiveWeaponAtlas=%d, SharedLoaded=%d"),
			InitialTexture ? TEXT("YES") : TEXT("NULL"),
			ActiveWeaponAtlas ? 1 : 0, bSharedAtlasLoaded ? 1 : 0);

		if (InitialTexture && Body.MeshComp)
		{
			Body.MaterialInst = CreateSpriteMaterial(InitialTexture);
			if (Body.MaterialInst)
			{
				Body.MeshComp->SetMaterial(0, Body.MaterialInst);
			}
			Body.MeshComp->SetVisibility(true);
			Body.bActive = true;
			ActiveBodyTexture = InitialTexture;

			UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: Body material created, bActive=true"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("SpriteCharacter: FAILED to create body — InitialTexture=%d MeshComp=%d"),
				InitialTexture ? 1 : 0, Body.MeshComp ? 1 : 0);
		}

		// Set initial UVs to frame 0 of the current animation state
		SelectRandomVariant(CurrentAnimState);
		UpdateAllLayers();

		// Log what the resolve produces
		{
			int32 DbgVariant = 0;
			const FSpriteAnimInfo* DbgInfo = ResolveBodyAnimation(CurrentAnimState, DbgVariant);
			UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: ResolveBodyAnimation(Idle) = %s, variant=%d, frameCount=%d"),
				DbgInfo ? TEXT("FOUND") : TEXT("NULL"), DbgVariant, DbgInfo ? DbgInfo->FrameCount : -1);
		}

		UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Body loaded from JSON: %s "
			"(shared=%d, unarmed=%d, 1h=%d, 2h=%d)"),
			*BaseName, bSharedAtlasLoaded ? 1 : 0,
			bWeaponAtlasLoaded[0] ? 1 : 0,
			bWeaponAtlasLoaded[1] ? 1 : 0,
			bWeaponAtlasLoaded[2] ? 1 : 0);
		return;
	}

	// Fallback: old single-atlas path (backward compat for mage_f)
	FString AtlasName = FString::Printf(TEXT("%s_body"), *BaseName);
	// Try subfolder first, then flat
	FString AtlasPath = FString::Printf(
		TEXT("/Game/SabriMMO/Sprites/Atlases/Body/%s/%s.%s"),
		*BaseName, *AtlasName, *AtlasName);
	UTexture2D* Atlas = Cast<UTexture2D>(
		StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AtlasPath));
	if (!Atlas)
	{
		AtlasPath = FString::Printf(
			TEXT("/Game/SabriMMO/Sprites/Atlases/Body/%s.%s"),
			*AtlasName, *AtlasName);
		Atlas = Cast<UTexture2D>(
			StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AtlasPath));
	}

	if (!Atlas)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: Atlas not found: %s"), *AtlasPath);
		return;
	}

	// Legacy hardcoded layout
	TMap<ESpriteAnimState, FSpriteAnimVariants> Anims;

	auto AddSingle = [&](ESpriteAnimState State, int32 Row, int32 Frames, bool bLoop)
	{
		FSpriteAnimVariants Vars;
		FSpriteAnimInfo Info;
		Info.StartRow = Row;
		Info.FrameCount = Frames;
		Info.bLoops = bLoop;
		Vars.Variants.Add(Info);
		Anims.Add(State, Vars);
	};

	AddSingle(ESpriteAnimState::Idle,   0,  4, true);
	AddSingle(ESpriteAnimState::Walk,   4,  8, true);
	AddSingle(ESpriteAnimState::Attack, 12, 6, false);
	AddSingle(ESpriteAnimState::Death,  18, 3, false);

	SetLayerAtlas(ESpriteLayer::Body, Atlas, FIntPoint(8, 21), Anims);
	UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Body set to %s (legacy single atlas)"), *AtlasName);
}

void ASpriteCharacterActor::SetBodyClass(const FString& AtlasBaseName)
{
	FString BodyRoot = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases/Body");
	FString ManifestFile = FString::Printf(TEXT("%s_manifest.json"), *AtlasBaseName);

	// Search order: Body/{name}/, Body/enemies/{name}/, Body/
	TArray<FString> SearchDirs;
	SearchDirs.Add(BodyRoot / AtlasBaseName);
	SearchDirs.Add(BodyRoot / TEXT("enemies") / AtlasBaseName);
	SearchDirs.Add(BodyRoot);

	for (const FString& Dir : SearchDirs)
	{
		FString ManifestPath = Dir / ManifestFile;
		FString ManifestStr;
		if (FFileHelper::LoadFileToString(ManifestStr, *ManifestPath))
		{
			bUsingV2Atlas = true;
			LoadV2AtlasManifest(ManifestPath);
			UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Body set by name '%s' (v2 manifest at %s)"),
				*AtlasBaseName, *Dir);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SpriteCharacter: No manifest found for '%s'"), *AtlasBaseName);
}

// ============================================================
// Equipment Layer System
// ============================================================

FString ASpriteCharacterActor::GetLayerSubDir(ESpriteLayer Layer)
{
	switch (Layer)
	{
	case ESpriteLayer::Hair:        return TEXT("Hair");
	case ESpriteLayer::Weapon:      return TEXT("Weapon");
	case ESpriteLayer::Shield:      return TEXT("Shield");
	case ESpriteLayer::HeadgearTop: return TEXT("HeadgearTop");
	case ESpriteLayer::HeadgearMid: return TEXT("HeadgearMid");
	case ESpriteLayer::HeadgearLow: return TEXT("HeadgearLow");
	case ESpriteLayer::Garment:     return TEXT("Garment");
	default:                        return TEXT("");
	}
}

ESpriteLayer ASpriteCharacterActor::EquipSlotToSpriteLayer(const FString& Slot)
{
	if (Slot == TEXT("weapon") || Slot == TEXT("weapon_left")) return ESpriteLayer::Weapon;
	if (Slot == TEXT("shield"))    return ESpriteLayer::Shield;
	if (Slot == TEXT("head_top"))  return ESpriteLayer::HeadgearTop;
	if (Slot == TEXT("head_mid"))  return ESpriteLayer::HeadgearMid;
	if (Slot == TEXT("head_low"))  return ESpriteLayer::HeadgearLow;
	if (Slot == TEXT("garment"))   return ESpriteLayer::Garment;
	return ESpriteLayer::MAX;
}

void ASpriteCharacterActor::LoadEquipmentLayer(ESpriteLayer Layer, int32 ViewSpriteId)
{
	int32 LayerIdx = static_cast<int32>(Layer);
	if (LayerIdx < 0 || LayerIdx >= static_cast<int32>(ESpriteLayer::MAX))
		return;

	FSpriteLayerState& L = Layers[LayerIdx];

	if (ViewSpriteId <= 0)
	{
		SetLayerVisible(Layer, false);
		L.bUsingLayerV2 = false;
		L.LayerAtlasRegistry.Empty();
		L.ActiveLayerAtlas = nullptr;
		return;
	}

	FString SubDir = GetLayerSubDir(Layer);
	if (SubDir.IsEmpty()) return;

	FString BaseName = FString::Printf(TEXT("%s_%d"), *SubDir.ToLower(), ViewSpriteId);
	FString LayerRoot = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases") / SubDir;

	// Search for manifest with gender-aware priority:
	// 1. {LayerRoot}/{item_subdir}/{gender}/manifest  (e.g., Weapon/dagger/female/)
	// 2. {LayerRoot}/{item_subdir}/manifest            (e.g., Weapon/dagger/)
	// 3. {LayerRoot}/manifest                          (flat fallback)
	FString JsonDir;
	FString ManifestPath;
	FString ManifestStr;
	{
		FString ManifestFileName = FString::Printf(TEXT("%s_manifest.json"), *BaseName);

		// Try each item subfolder (e.g., dagger/, bow/, egg_shell_hat/)
		TArray<FString> SubDirs;
		IFileManager::Get().FindFiles(SubDirs, *(LayerRoot / TEXT("*")), false, true);
		for (const FString& SD : SubDirs)
		{
			FString ItemDir = LayerRoot / SD;

			// Priority 1: gender subfolder (e.g., Weapon/dagger/female/)
			if (!GenderSubDir.IsEmpty())
			{
				FString GenderDir = ItemDir / GenderSubDir;
				FString GenderPath = GenderDir / ManifestFileName;
				if (FFileHelper::LoadFileToString(ManifestStr, *GenderPath))
				{
					JsonDir = GenderDir;
					ManifestPath = GenderPath;
					break;
				}
			}

			// Priority 2: directly in item subfolder (e.g., Weapon/dagger/)
			FString CandidatePath = ItemDir / ManifestFileName;
			if (FFileHelper::LoadFileToString(ManifestStr, *CandidatePath))
			{
				JsonDir = ItemDir;
				ManifestPath = CandidatePath;
				break;
			}
		}

		// Priority 3: flat structure (e.g., Weapon/weapon_1_manifest.json)
		if (ManifestStr.IsEmpty())
		{
			JsonDir = LayerRoot;
			ManifestPath = LayerRoot / ManifestFileName;
			FFileHelper::LoadFileToString(ManifestStr, *ManifestPath);
		}
	}

	if (ManifestStr.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpriteEquip: Manifest not found: %s_%d in %s"),
			*SubDir.ToLower(), ViewSpriteId, *LayerRoot);
		SetLayerVisible(Layer, false);
		return;
	}

	// Parse manifest
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ManifestStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return;

	// Check if this headgear hides hair
	if (Layer == ESpriteLayer::HeadgearTop || Layer == ESpriteLayer::HeadgearMid || Layer == ESpriteLayer::HeadgearLow)
	{
		bool bHidesHair = false;
		Root->TryGetBoolField(TEXT("hides_hair"), bHidesHair);
		if (bHidesHair)
		{
			bHairHiddenByHeadgear = true;
			SetLayerVisible(ESpriteLayer::Hair, false);
		}
		else if (bHairHiddenByHeadgear)
		{
			// Swapping to a non-hiding headgear — restore hair
			bHairHiddenByHeadgear = false;
			int32 HairIdx = static_cast<int32>(ESpriteLayer::Hair);
			if (Layers[HairIdx].bUsingLayerV2 && Layers[HairIdx].LayerAtlasRegistry.Num() > 0)
			{
				SetLayerVisible(ESpriteLayer::Hair, true);
			}
		}
	}

	L.LayerAtlasRegistry.Empty();
	L.ActiveLayerAtlas = nullptr;
	L.bUsingLayerV2 = true;

	const TArray<TSharedPtr<FJsonValue>>* AtlasArr;
	if (!Root->TryGetArrayField(TEXT("atlases"), AtlasArr))
		return;

	int32 LoadedCount = 0;
	for (const auto& AtlasVal : *AtlasArr)
	{
		const TSharedPtr<FJsonObject>& AtlasObj = AtlasVal->AsObject();
		if (!AtlasObj.IsValid()) continue;

		FString FileName = AtlasObj->GetStringField(TEXT("file"));
		FString StateName = AtlasObj->GetStringField(TEXT("state"));
		FString GroupName = AtlasObj->GetStringField(TEXT("group"));

		// Parse individual atlas JSON
		FString AtlasJsonPath = JsonDir / FString::Printf(TEXT("%s.json"), *FileName);
		FString AtlasJsonStr;
		if (!FFileHelper::LoadFileToString(AtlasJsonStr, *AtlasJsonPath))
			continue;

		// Reuse body's ParseSingleAtlasJSON but with equipment texture path
		FSingleAnimAtlasInfo Info;
		{
			TSharedPtr<FJsonObject> ARoot;
			TSharedRef<TJsonReader<>> AReader = TJsonReaderFactory<>::Create(AtlasJsonStr);
			if (!FJsonSerializer::Deserialize(AReader, ARoot) || !ARoot.IsValid())
				continue;

			const TArray<TSharedPtr<FJsonValue>>* GridArr;
			if (ARoot->TryGetArrayField(TEXT("grid"), GridArr) && GridArr->Num() >= 2)
			{
				Info.GridSize.X = (*GridArr)[0]->AsNumber();
				Info.GridSize.Y = (*GridArr)[1]->AsNumber();
			}
			Info.FrameCount = ARoot->GetIntegerField(TEXT("frame_count"));
			ARoot->TryGetStringField(TEXT("source"), Info.Source);

			// Parse depth_front array (per-frame front/behind ordering)
			const TArray<TSharedPtr<FJsonValue>>* DepthArr;
			if (ARoot->TryGetArrayField(TEXT("depth_front"), DepthArr))
			{
				Info.DepthFront.Reserve(DepthArr->Num());
				for (const auto& Val : *DepthArr)
				{
					Info.DepthFront.Add(Val->AsNumber() > 0.5);
				}
			}

			// Store path for lazy loading — DON'T load texture now
			// Derive asset sub-path from JsonDir (e.g., Weapon/dagger)
			FString ContentBase = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases");
			FString EquipAssetSubPath = SubDir;
			if (JsonDir.StartsWith(ContentBase))
			{
				EquipAssetSubPath = JsonDir.Mid(ContentBase.Len() + 1);
			}
			Info.AssetPath = FString::Printf(
				TEXT("/Game/SabriMMO/Sprites/Atlases/%s/%s.%s"),
				*EquipAssetSubPath, *FileName, *FileName);
		}

		const ESpriteAnimState* StateEnum = StateNameMap.Find(StateName);
		if (!StateEnum) continue;

		// Respect weapon group — must match body's group registration
		// so equipment and body resolve to the SAME animation per mode
		static const TMap<FString, ESpriteWeaponMode> EquipGroupMap = {
			{TEXT("unarmed"), ESpriteWeaponMode::None},
			{TEXT("onehand"), ESpriteWeaponMode::OneHand},
			{TEXT("twohand"), ESpriteWeaponMode::TwoHand},
			{TEXT("bow"),     ESpriteWeaponMode::Bow},
		};

		TArray<FString> Groups;
		GroupName.ParseIntoArray(Groups, TEXT(","));

		for (const FString& G : Groups)
		{
			FString Trimmed = G.TrimStartAndEnd();
			if (Trimmed == TEXT("shared"))
			{
				for (int32 m = 0; m < static_cast<int32>(ESpriteWeaponMode::MAX); m++)
				{
					FSpriteAtlasKey Key{static_cast<ESpriteWeaponMode>(m), *StateEnum};
					L.LayerAtlasRegistry.FindOrAdd(Key).Add(Info);
				}
			}
			else
			{
				const ESpriteWeaponMode* Mode = EquipGroupMap.Find(Trimmed);
				if (Mode)
				{
					FSpriteAtlasKey Key{*Mode, *StateEnum};
					L.LayerAtlasRegistry.FindOrAdd(Key).Add(Info);
				}
			}
		}

		LoadedCount++;
	}

	if (LoadedCount > 0)
	{
		// Resolve initial atlas and create material
		ResolveLayerAtlas(L);

		if (L.ActiveLayerAtlas && L.ActiveLayerAtlas->AtlasTexture && L.MeshComp)
		{
			L.MaterialInst = CreateSpriteMaterial(L.ActiveLayerAtlas->AtlasTexture);
			if (L.MaterialInst)
			{
				L.MeshComp->SetMaterial(0, L.MaterialInst);
			}

			// Don't show hair layer if headgear is hiding it
			bool bShouldShow = !(Layer == ESpriteLayer::Hair && bHairHiddenByHeadgear);
			L.MeshComp->SetVisibility(bShouldShow);
			L.bActive = bShouldShow;
			L.ActiveLayerTexture = L.ActiveLayerAtlas->AtlasTexture;
		}

		UE_LOG(LogTemp, Log, TEXT("SpriteEquip: Loaded %s (viewSprite=%d, %d atlases)"),
			*SubDir, ViewSpriteId, LoadedCount);
	}
	else
	{
		SetLayerVisible(Layer, false);
		L.bUsingLayerV2 = false;
	}
}

void ASpriteCharacterActor::ResolveLayerAtlas(FSpriteLayerState& Layer)
{
	FSingleAnimAtlasInfo* OldLayerAtlas = Layer.ActiveLayerAtlas;
	Layer.ActiveLayerAtlas = nullptr;

	FSpriteAtlasKey Key{CurrentWeaponMode, CurrentAnimState};
	TArray<FSingleAnimAtlasInfo>* Variants = Layer.LayerAtlasRegistry.Find(Key);

	// Fallback to unarmed
	if (!Variants || Variants->Num() == 0)
	{
		Key.WeaponMode = ESpriteWeaponMode::None;
		Variants = Layer.LayerAtlasRegistry.Find(Key);
	}

	if (!Variants || Variants->Num() == 0)
		return; // Animation not available — layer will hide in UpdateQuadUVs

	// Use same variant index as body so equipment matches the body's animation
	int32 Idx = FMath::Clamp(ActiveVariantIndex, 0, Variants->Num() - 1);
	Layer.ActiveLayerAtlas = &(*Variants)[Idx];

	// Lazy-load texture on demand
	Layer.ActiveLayerAtlas->EnsureTextureLoaded();

	UE_LOG(LogTemp, Log, TEXT("ResolveLayerAtlas: state=%d mode=%d source='%s' tex=%s path='%s'"),
		static_cast<int32>(CurrentAnimState), static_cast<int32>(CurrentWeaponMode),
		*Layer.ActiveLayerAtlas->Source,
		Layer.ActiveLayerAtlas->AtlasTexture ? TEXT("LOADED") : TEXT("NULL"),
		*Layer.ActiveLayerAtlas->AssetPath);

	// Release old texture (only if different from new)
	if (OldLayerAtlas && OldLayerAtlas != Layer.ActiveLayerAtlas && OldLayerAtlas->AtlasTexture)
	{
		OldLayerAtlas->AtlasTexture = nullptr;
	}

	// Swap texture if changed
	if (Layer.MaterialInst && Layer.ActiveLayerAtlas->AtlasTexture &&
	    Layer.ActiveLayerAtlas->AtlasTexture != Layer.ActiveLayerTexture)
	{
		Layer.MaterialInst->SetTextureParameterValue(TEXT("Atlas"), Layer.ActiveLayerAtlas->AtlasTexture);
		Layer.ActiveLayerTexture = Layer.ActiveLayerAtlas->AtlasTexture;
	}
}

void ASpriteCharacterActor::ResolveAllEquipmentAtlases()
{
	for (int32 i = 0; i < static_cast<int32>(ESpriteLayer::MAX); i++)
	{
		if (Layers[i].bUsingLayerV2 && Layers[i].bActive)
		{
			ResolveLayerAtlas(Layers[i]);
		}
	}
}

// ============================================================
// V2: Per-animation atlas system
// ============================================================

FSingleAnimAtlasInfo ASpriteCharacterActor::ParseSingleAtlasJSON(const FString& JsonStr, const FString& AtlasName,
                                                                  const FString& AssetSubPath)
{
	FSingleAnimAtlasInfo Result;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return Result;

	const TArray<TSharedPtr<FJsonValue>>* GridArr;
	if (Root->TryGetArrayField(TEXT("grid"), GridArr) && GridArr->Num() >= 2)
	{
		Result.GridSize.X = (*GridArr)[0]->AsNumber();
		Result.GridSize.Y = (*GridArr)[1]->AsNumber();
	}

	Result.FrameCount = Root->GetIntegerField(TEXT("frame_count"));
	Root->TryGetStringField(TEXT("source"), Result.Source);

	// Store path for lazy loading — DON'T load texture now
	// AssetSubPath is relative to /Game/SabriMMO/Sprites/Atlases/ (e.g., "Body/swordsman_m")
	Result.AssetPath = FString::Printf(
		TEXT("/Game/SabriMMO/Sprites/Atlases/%s/%s.%s"),
		*AssetSubPath, *AtlasName, *AtlasName);
	// AtlasTexture stays nullptr — loaded on demand in ResolveActiveAtlas()

	return Result;
}

void ASpriteCharacterActor::LoadV2AtlasManifest(const FString& ManifestPath)
{
	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *ManifestPath))
		return;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return;

	FString JsonDir = FPaths::GetPath(ManifestPath);

	// Derive UE5 asset sub-path from manifest location
	// e.g., disk: .../Content/SabriMMO/Sprites/Atlases/Body/swordsman_m/ → asset: "Body/swordsman_m"
	FString ContentBase = FPaths::ProjectContentDir() / TEXT("SabriMMO/Sprites/Atlases");
	FString AssetSubPath;
	if (JsonDir.StartsWith(ContentBase))
	{
		AssetSubPath = JsonDir.Mid(ContentBase.Len() + 1); // +1 for the separator
	}
	else
	{
		AssetSubPath = TEXT("Body"); // fallback
	}

	static const TMap<FString, ESpriteWeaponMode> GroupMap = {
		{TEXT("unarmed"), ESpriteWeaponMode::None},
		{TEXT("onehand"), ESpriteWeaponMode::OneHand},
		{TEXT("twohand"), ESpriteWeaponMode::TwoHand},
		{TEXT("bow"),     ESpriteWeaponMode::Bow},
	};

	AtlasRegistry.Empty();
	int32 LoadedCount = 0;

	const TArray<TSharedPtr<FJsonValue>>* AtlasArr;
	if (!Root->TryGetArrayField(TEXT("atlases"), AtlasArr))
		return;

	for (const auto& AtlasVal : *AtlasArr)
	{
		const TSharedPtr<FJsonObject>& AtlasObj = AtlasVal->AsObject();
		if (!AtlasObj.IsValid()) continue;

		FString FileName = AtlasObj->GetStringField(TEXT("file"));
		FString StateName = AtlasObj->GetStringField(TEXT("state"));
		FString GroupName = AtlasObj->GetStringField(TEXT("group"));

		// Parse individual atlas JSON
		FString AtlasJsonPath = JsonDir / FString::Printf(TEXT("%s.json"), *FileName);
		FString AtlasJsonStr;
		if (!FFileHelper::LoadFileToString(AtlasJsonStr, *AtlasJsonPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("SpriteV2: Atlas JSON not found: %s"), *AtlasJsonPath);
			continue;
		}

		FSingleAnimAtlasInfo Info = ParseSingleAtlasJSON(AtlasJsonStr, FileName, AssetSubPath);
		if (Info.AssetPath.IsEmpty()) continue;  // No valid path = skip

		const ESpriteAnimState* StateEnum = StateNameMap.Find(StateName);
		if (!StateEnum) continue;

		// Parse comma-separated groups (e.g., "onehand,twohand")
		TArray<FString> Groups;
		GroupName.ParseIntoArray(Groups, TEXT(","));

		for (const FString& G : Groups)
		{
			FString Trimmed = G.TrimStartAndEnd();
			if (Trimmed == TEXT("shared"))
			{
				// Register under ALL weapon modes
				for (int32 m = 0; m < static_cast<int32>(ESpriteWeaponMode::MAX); m++)
				{
					FSpriteAtlasKey Key{static_cast<ESpriteWeaponMode>(m), *StateEnum};
					AtlasRegistry.FindOrAdd(Key).Add(Info);
				}
			}
			else
			{
				const ESpriteWeaponMode* Mode = GroupMap.Find(Trimmed);
				if (Mode)
				{
					FSpriteAtlasKey Key{*Mode, *StateEnum};
					AtlasRegistry.FindOrAdd(Key).Add(Info);
				}
			}
		}

		LoadedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("SpriteV2: Loaded %d atlases, %d registry entries"),
		LoadedCount, AtlasRegistry.Num());

	// Set up body layer material from first available atlas
	FSpriteLayerState& Body = Layers[static_cast<int32>(ESpriteLayer::Body)];
	CurrentWeaponMode = ESpriteWeaponMode::MAX; // force SetWeaponMode to run
	SetWeaponMode(ESpriteWeaponMode::None);

	// Create material from active atlas
	if (ActiveAtlas && ActiveAtlas->AtlasTexture && Body.MeshComp)
	{
		Body.MaterialInst = CreateSpriteMaterial(ActiveAtlas->AtlasTexture);
		if (Body.MaterialInst)
		{
			Body.MeshComp->SetMaterial(0, Body.MaterialInst);
		}
		Body.MeshComp->SetVisibility(true);
		Body.bActive = true;
		ActiveBodyTexture = ActiveAtlas->AtlasTexture;
	}

	SelectRandomV2Variant();
	ResolveActiveAtlas();
	UpdateAllLayers();
}

void ASpriteCharacterActor::ResolveActiveAtlas()
{
	FSingleAnimAtlasInfo* OldAtlas = ActiveAtlas;
	ActiveAtlas = nullptr;

	// Lookup: (current weapon mode, current state)
	FSpriteAtlasKey Key{CurrentWeaponMode, CurrentAnimState};
	TArray<FSingleAnimAtlasInfo>* Variants = AtlasRegistry.Find(Key);

	// Fallback: unarmed mode
	if (!Variants || Variants->Num() == 0)
	{
		Key.WeaponMode = ESpriteWeaponMode::None;
		Variants = AtlasRegistry.Find(Key);
	}

	if (!Variants || Variants->Num() == 0)
		return;

	int32 Idx = FMath::Clamp(ActiveVariantIndex, 0, Variants->Num() - 1);
	ActiveAtlas = &(*Variants)[Idx];

	// Lazy-load: only load the texture when this atlas becomes active
	ActiveAtlas->EnsureTextureLoaded();

	// Release old atlas texture to free memory (if switching to a different atlas)
	if (OldAtlas && OldAtlas != ActiveAtlas && OldAtlas->AtlasTexture)
	{
		OldAtlas->AtlasTexture = nullptr;
	}

	// Swap texture on body material
	int32 BodyIdx = static_cast<int32>(ESpriteLayer::Body);
	if (BodyIdx < 0 || BodyIdx >= static_cast<int32>(ESpriteLayer::MAX))
		return;

	FSpriteLayerState& Body = Layers[BodyIdx];
	if (!Body.bActive || !IsValid(Body.MeshComp) || !IsValid(Body.MaterialInst))
		return;
	if (!ActiveAtlas->AtlasTexture || !IsValid(ActiveAtlas->AtlasTexture))
	{
		UE_LOG(LogTemp, Warning, TEXT("ResolveActiveAtlas: TEXTURE LOAD FAILED for state=%d source='%s' path='%s'"),
			static_cast<int32>(CurrentAnimState), *ActiveAtlas->Source, *ActiveAtlas->AssetPath);
		return;
	}
	if (ActiveAtlas->AtlasTexture == ActiveBodyTexture)
		return;

	UE_LOG(LogTemp, Log, TEXT("ResolveActiveAtlas: Swapping body texture for state=%d source='%s'"),
		static_cast<int32>(CurrentAnimState), *ActiveAtlas->Source);
	Body.MaterialInst->SetTextureParameterValue(TEXT("Atlas"), ActiveAtlas->AtlasTexture);
	ActiveBodyTexture = ActiveAtlas->AtlasTexture;
}

void ASpriteCharacterActor::SelectRandomV2Variant()
{
	FSpriteAtlasKey Key{CurrentWeaponMode, CurrentAnimState};
	TArray<FSingleAnimAtlasInfo>* Variants = AtlasRegistry.Find(Key);

	// Fallback to unarmed
	if (!Variants || Variants->Num() == 0)
	{
		Key.WeaponMode = ESpriteWeaponMode::None;
		Variants = AtlasRegistry.Find(Key);
	}

	if (Variants && Variants->Num() > 1)
	{
		// Avoid repeating the same variant
		int32 OldIndex = ActiveVariantIndex;
		int32 NumVariants = Variants->Num();
		ActiveVariantIndex = FMath::RandRange(0, NumVariants - 1);
		if (ActiveVariantIndex == OldIndex && NumVariants > 1)
			ActiveVariantIndex = (ActiveVariantIndex + 1) % NumVariants;

		UE_LOG(LogTemp, Log, TEXT("SpriteV2: Picked variant %d/%d for state=%d mode=%d (source=%s)"),
			ActiveVariantIndex, NumVariants, (int32)CurrentAnimState, (int32)CurrentWeaponMode,
			*(*Variants)[ActiveVariantIndex].Source);
	}
	else
	{
		ActiveVariantIndex = 0;
	}
}

ASpriteCharacterActor* ASpriteCharacterActor::SpawnTestSpriteAt(UWorld* World, FVector Location)
{
	if (!World) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	ASpriteCharacterActor* Actor = World->SpawnActor<ASpriteCharacterActor>(
		ASpriteCharacterActor::StaticClass(), SpawnTransform, Params);

	if (Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== TEST SPRITE SPAWNED at %s ==="), *Location.ToString());
	}
	return Actor;
}

ASpriteCharacterActor* ASpriteCharacterActor::SpawnSpriteForClass(
	UWorld* World, FVector Location, int32 ClassId, int32 Gender)
{
	ASpriteCharacterActor* Actor = SpawnTestSpriteAt(World, Location);
	if (Actor)
	{
		Actor->SetBodyClass(ClassId, Gender);
		UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Spawned for classId=%d gender=%d"),
			ClassId, Gender);
	}
	return Actor;
}

int32 ASpriteCharacterActor::JobClassToId(const FString& JobClass)
{
	static const TMap<FString, int32> Map = {
		{TEXT("novice"),     0},
		{TEXT("swordsman"), 100},
		{TEXT("mage"),      200},
		{TEXT("archer"),    300},
		{TEXT("acolyte"),   400},
		{TEXT("thief"),     500},
		{TEXT("merchant"),  600},
		{TEXT("knight"),    700},
		{TEXT("wizard"),    800},
		{TEXT("hunter"),    900},
		{TEXT("priest"),   1000},
		{TEXT("assassin"), 1100},
		{TEXT("blacksmith"),1200},
		{TEXT("crusader"), 1300},
		{TEXT("sage"),     1400},
		{TEXT("bard"),     1500},
		{TEXT("monk"),     1600},
		{TEXT("rogue"),    1700},
		{TEXT("alchemist"),1800},
		{TEXT("dancer"),   1900},
	};
	const int32* Found = Map.Find(JobClass.ToLower());
	return Found ? *Found : 0;
}

// ============================================================
// Owner Tracking — follow an actor, detect movement
// ============================================================

void ASpriteCharacterActor::AttachToOwnerActor(AActor* Actor, bool bIsLocalPlayer, int32 CharacterId)
{
	OwnerActor = Actor;
	bIsLocalPlayerSprite = bIsLocalPlayer;
	if (Actor)
	{
		SetActorLocation(Actor->GetActorLocation());
	}

	// Set the character ID this sprite represents
	// Local player: passed from GameInstance. Remote player: passed from OtherPlayerSubsystem.
	if (bIsLocalPlayer)
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetGameInstance()))
		{
			LocalCharacterId = GI->GetSelectedCharacter().CharacterId;
		}
	}
	else
	{
		LocalCharacterId = CharacterId; // Remote player's character ID
	}

	// ALL sprites register for combat events (local AND remote)
	// Each sprite filters events by its own LocalCharacterId
	RegisterCombatEvents();

	// Only the LOCAL player subscribes to equipment changes
	// Remote players get weapon mode from player:moved / player:appearance events
	if (!bIsLocalPlayer)
		return;

	// Subscribe to equipment changes for weapon mode switching
	UWorld* World = GetWorld();
	if (World)
	{
		if (UEquipmentSubsystem* EquipSub = World->GetSubsystem<UEquipmentSubsystem>())
		{
			EquipSub->OnEquipmentChanged.AddLambda([this, WeakEquipSub = TWeakObjectPtr<UEquipmentSubsystem>(EquipSub)]()
			{
				if (!WeakEquipSub.IsValid()) return;

				// Determine weapon mode from equipped weapon
				FInventoryItem Weapon = WeakEquipSub->GetEquippedItem(TEXT("weapon"));
				ESpriteWeaponMode NewMode = ESpriteWeaponMode::None;

				if (!Weapon.Name.IsEmpty() && Weapon.EquipSlot == TEXT("weapon"))
				{
					if (Weapon.WeaponType == TEXT("bow"))
						NewMode = ESpriteWeaponMode::Bow;
					else if (Weapon.WeaponType == TEXT("knuckle"))
						NewMode = ESpriteWeaponMode::None;
					else if (Weapon.WeaponType == TEXT("katar") || !Weapon.bTwoHanded)
						NewMode = ESpriteWeaponMode::OneHand;
					else
						NewMode = ESpriteWeaponMode::TwoHand;
				}

				SetWeaponMode(NewMode);

				ResetHairHiding();

				// Update all equipment sprite layers
				static const TArray<TPair<FString, ESpriteLayer>> EquipLayerMap = {
					{TEXT("weapon"),   ESpriteLayer::Weapon},
					{TEXT("shield"),   ESpriteLayer::Shield},
					{TEXT("head_top"), ESpriteLayer::HeadgearTop},
					{TEXT("head_mid"), ESpriteLayer::HeadgearMid},
					{TEXT("head_low"), ESpriteLayer::HeadgearLow},
					{TEXT("garment"),  ESpriteLayer::Garment},
				};

				for (const auto& Pair : EquipLayerMap)
				{
					FInventoryItem Item = WeakEquipSub->GetEquippedItem(Pair.Key);
					if (!Item.Name.IsEmpty() && Item.ViewSprite > 0)
						LoadEquipmentLayer(Pair.Value, Item.ViewSprite);
					else
						LoadEquipmentLayer(Pair.Value, 0); // hide layer
				}

				ReconcileHairVisibility();

				UE_LOG(LogTemp, Log, TEXT("SpriteCharacter: Equipment changed — weapon='%s' mode=%d"),
					*Weapon.Name, static_cast<int32>(NewMode));
			});

			// Set initial weapon mode + equipment layers from current equipment
			FInventoryItem Weapon = EquipSub->GetEquippedItem(TEXT("weapon"));
			if (!Weapon.Name.IsEmpty() && Weapon.EquipSlot == TEXT("weapon"))
			{
				ESpriteWeaponMode InitMode = Weapon.WeaponType == TEXT("bow") ? ESpriteWeaponMode::Bow
				: Weapon.WeaponType == TEXT("knuckle") ? ESpriteWeaponMode::None
				: (Weapon.WeaponType == TEXT("katar") || !Weapon.bTwoHanded) ? ESpriteWeaponMode::OneHand : ESpriteWeaponMode::TwoHand;
				SetWeaponMode(InitMode);
			}

			// Load initial equipment layers immediately (don't wait for OnEquipmentChanged)
			static const TArray<TPair<FString, ESpriteLayer>> InitEquipMap = {
				{TEXT("weapon"),   ESpriteLayer::Weapon},
				{TEXT("shield"),   ESpriteLayer::Shield},
				{TEXT("head_top"), ESpriteLayer::HeadgearTop},
				{TEXT("head_mid"), ESpriteLayer::HeadgearMid},
				{TEXT("head_low"), ESpriteLayer::HeadgearLow},
				{TEXT("garment"),  ESpriteLayer::Garment},
			};
			for (const auto& Pair : InitEquipMap)
			{
				FInventoryItem Item = EquipSub->GetEquippedItem(Pair.Key);
				if (!Item.Name.IsEmpty() && Item.ViewSprite > 0)
					LoadEquipmentLayer(Pair.Value, Item.ViewSprite);
			}
		}
	}
}

void ASpriteCharacterActor::SetServerTargetPosition(const FVector& Pos, bool bMoving, float Speed)
{
	ServerTargetPos = Pos;
	ServerMoveSpeed = Speed;
	bUseServerMovement = true;
}

void ASpriteCharacterActor::UpdateOwnerTracking()
{
	// Standalone sprite enemies (no owner actor) — handle movement first
	// C++ server-driven movement for standalone sprite enemies (no BP actor)
	if (bUseServerMovement)
	{
		FVector Current = GetActorLocation();
		FVector Dir = ServerTargetPos - Current;
		Dir.Z = 0.f;
		float Dist = Dir.Size();

		if (Dist > 5.f)
		{
			float DT = GetWorld()->GetDeltaSeconds();
			float Step = ServerMoveSpeed * DT;
			FVector Move = Dir.GetSafeNormal() * FMath::Min(Step, Dist);
			FVector NewPos(Current.X + Move.X, Current.Y + Move.Y, Current.Z);
			SetActorLocation(NewPos);
		}

		// Ground snap via line trace (replaces CharacterMovementComponent gravity)
		{
			FVector Loc = GetActorLocation();
			FVector Start = FVector(Loc.X, Loc.Y, Loc.Z + 500.f);
			FVector End = FVector(Loc.X, Loc.Y, Loc.Z - 2000.f);
			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
			{
				FVector Snapped = Loc;
				Snapped.Z = Hit.ImpactPoint.Z;
				SetActorLocation(Snapped);
			}
		}

		// Animation + facing driven by EnemySubsystem (HandleEnemyMove), not velocity
		return;
	}

	// Player/remote player sprites: follow owner actor position
	if (!OwnerActor.IsValid())
		return;

	// Position sprite at ground level (capsule bottom)
	// All player positions are at capsule center (96 above ground)
	static constexpr float PlayerCapsuleHalfHeight = 96.f;
	FVector Loc = OwnerActor->GetActorLocation();
	Loc.Z -= PlayerCapsuleHalfHeight;
	Loc.Z -= GroundZOffset;
	SetActorLocation(Loc);

	// Two line traces from camera: one to the sprite's feet, one to the sprite's head.
	// - Both blocked → entire sprite renders as silhouette
	// - Only feet blocked → per-pixel depth check (parts above the wall stay sprite art)
	// - Neither blocked → full sprite art
	// The custom depth pass always writes stencil so the post-process can mask the area.
	{
		float FeetVal = 0.f;
		float HeadVal = 0.f;
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
				FVector FeetLoc = Loc + FVector(0, 0, 8.f);             // just above ground
				FVector HeadLoc = Loc + FVector(0, 0, SpriteSize.Y);    // top of sprite quad
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(SpriteOcclusionTrace), false, this);
				Params.AddIgnoredActor(OwnerActor.Get());
				if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, FeetLoc, ECC_Visibility, Params))
				{
					FeetVal = 1.f;
				}
				if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, HeadLoc, ECC_Visibility, Params))
				{
					HeadVal = 1.f;
				}
			}
		}
		for (int32 i = 0; i < static_cast<int32>(ESpriteLayer::MAX); i++)
		{
			if (Layers[i].MeshComp)
			{
				if (UMaterialInstanceDynamic* MID =
					Cast<UMaterialInstanceDynamic>(Layers[i].MeshComp->GetMaterial(0)))
				{
					MID->SetScalarParameterValue(TEXT("FeetOccluded"), FeetVal);
					MID->SetScalarParameterValue(TEXT("HeadOccluded"), HeadVal);
				}
			}
		}
	}

	FVector Velocity = OwnerActor->GetVelocity();
	float Speed = Velocity.Size();

	// Don't override one-shot animations or sitting (sit ends via player:stand event)
	if (CurrentAnimState == ESpriteAnimState::Attack     ||
	    CurrentAnimState == ESpriteAnimState::Death      ||
	    CurrentAnimState == ESpriteAnimState::Hit        ||
	    CurrentAnimState == ESpriteAnimState::CastSingle ||
	    CurrentAnimState == ESpriteAnimState::CastSelf   ||
	    CurrentAnimState == ESpriteAnimState::CastGround ||
	    CurrentAnimState == ESpriteAnimState::CastAoe    ||
	    CurrentAnimState == ESpriteAnimState::Pickup     ||
	    CurrentAnimState == ESpriteAnimState::Block      ||
	    CurrentAnimState == ESpriteAnimState::Sit)
	{
		return;
	}

	if (Speed > 10.f)
	{
		if (CurrentAnimState != ESpriteAnimState::Walk)
			SetAnimState(ESpriteAnimState::Walk);
		SetFacingDirection(Velocity);
	}
	else
	{
		if (CurrentAnimState == ESpriteAnimState::Walk)
			SetAnimState(ESpriteAnimState::Idle);
	}
}

// ============================================================
// Combat Events
// ============================================================

void ASpriteCharacterActor::RegisterCombatEvents()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetGameInstance());
	if (!GI) return;
	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	Router->RegisterHandler(TEXT("combat:damage"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });

	Router->RegisterHandler(TEXT("combat:death"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDeath(D); });

	Router->RegisterHandler(TEXT("combat:respawn"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatRespawn(D); });

	Router->RegisterHandler(TEXT("player:sit_state"), this,
		[this](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("characterId"), CId);
			if ((int32)CId != LocalCharacterId) return;
			bool bSitting = false;
			Obj->TryGetBoolField(TEXT("isSitting"), bSitting);
			SetAnimState(bSitting ? ESpriteAnimState::Sit : ESpriteAnimState::Idle);
		});

	// Hit reaction when enemy attacks this player
	Router->RegisterHandler(TEXT("enemy:attack"), this,
		[this](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double TId = 0;
			Obj->TryGetNumberField(TEXT("targetId"), TId);
			if ((int32)TId == LocalCharacterId)
			{
				if (CurrentAnimState != ESpriteAnimState::Death &&
				    CurrentAnimState != ESpriteAnimState::Attack)
				{
					SetAnimState(ESpriteAnimState::Hit);
				}
			}
		});

	// Helper: map targetType string to cast animation state
	auto GetCastStateFromTargetType = [](const FString& TargetType) -> ESpriteAnimState
	{
		if (TargetType == TEXT("self"))   return ESpriteAnimState::CastSelf;
		if (TargetType == TEXT("ground")) return ESpriteAnimState::CastGround;
		if (TargetType == TEXT("aoe"))    return ESpriteAnimState::CastAoe;
		return ESpriteAnimState::CastSingle; // default for "single" and unknown
	};

	auto IsCastingState = [](ESpriteAnimState S) -> bool
	{
		return S == ESpriteAnimState::CastSingle || S == ESpriteAnimState::CastSelf ||
		       S == ESpriteAnimState::CastGround || S == ESpriteAnimState::CastAoe;
	};

	// skill:cast_start — broadcast to zone with casterId
	// Triggers for skills with cast time during the cast bar
	Router->RegisterHandler(TEXT("skill:cast_start"), this,
		[this, GetCastStateFromTargetType](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("casterId"), CId);
			if ((int32)CId != LocalCharacterId) return;
			if (CurrentAnimState == ESpriteAnimState::Death) return;

			FString TargetType;
			Obj->TryGetStringField(TEXT("targetType"), TargetType);
			SetAnimState(GetCastStateFromTargetType(TargetType));
		});

	// skill:cast_complete — broadcast to zone with casterId
	// Triggers for instant skills and when cast-time skills finish
	Router->RegisterHandler(TEXT("skill:cast_complete"), this,
		[this, GetCastStateFromTargetType, IsCastingState](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("casterId"), CId);
			if ((int32)CId != LocalCharacterId) return;
			if (CurrentAnimState == ESpriteAnimState::Death) return;
			// Don't override if already casting from skill:cast_start
			if (IsCastingState(CurrentAnimState)) return;

			FString TargetType;
			Obj->TryGetStringField(TEXT("targetType"), TargetType);
			SetAnimState(GetCastStateFromTargetType(TargetType));
		});

	// skill:buff_applied — broadcast to zone with casterId, catches instant buff/debuff skills
	// that don't emit skill:cast_start or skill:cast_complete
	Router->RegisterHandler(TEXT("skill:buff_applied"), this,
		[this, GetCastStateFromTargetType, IsCastingState](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("casterId"), CId);
			if ((int32)CId != LocalCharacterId) return;
			if (CurrentAnimState == ESpriteAnimState::Death) return;
			if (CurrentAnimState == ESpriteAnimState::Sit) return; // sitting buff (skillId=0) must not override sit
			if (IsCastingState(CurrentAnimState)) return;

			double SkillId = 0;
			Obj->TryGetNumberField(TEXT("skillId"), SkillId);
			if ((int32)SkillId == 0) return; // skillId 0 = sitting internal state, not a real skill

			FString TargetType;
			Obj->TryGetStringField(TEXT("targetType"), TargetType);
			// Most buff_applied events don't include targetType — default to self-cast
			if (TargetType.IsEmpty()) TargetType = TEXT("self");
			SetAnimState(GetCastStateFromTargetType(TargetType));
		});

}

void ASpriteCharacterActor::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
	if (!Obj.IsValid()) return;

	int32 AttackerId = 0;
	Obj->TryGetNumberField(TEXT("attackerId"), AttackerId);

	if (AttackerId == LocalCharacterId)
	{
		if (CurrentAnimState != ESpriteAnimState::Attack)
		{
			SetAnimState(ESpriteAnimState::Attack);
		}
	}
}

void ASpriteCharacterActor::HandleCombatDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
	if (!Obj.IsValid()) return;

	int32 KilledId = 0;
	Obj->TryGetNumberField(TEXT("killedId"), KilledId);

	if (KilledId == LocalCharacterId)
	{
		SetAnimState(ESpriteAnimState::Death);
	}
}

void ASpriteCharacterActor::HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data)
{
	SetAnimState(ESpriteAnimState::Idle);
}
