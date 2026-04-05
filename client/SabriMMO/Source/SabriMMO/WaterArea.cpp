// WaterArea.cpp — RO-style water: animated translucent plane + overlap trigger.
// Visual: runtime material with animated opacity ripple and warm teal color.
// Gameplay: emits water:enter/water:exit to server for skill gating.

#include "WaterArea.h"
#include "MMOGameInstance.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogWater, Log, All);

AWaterArea::AWaterArea()
{
	PrimaryActorTick.bCanEverTick = false;

	// Box trigger for overlap detection
	TriggerComp = CreateDefaultSubobject<UBoxComponent>(TEXT("WaterTrigger"));
	TriggerComp->SetBoxExtent(FVector(500.f, 500.f, 100.f));
	TriggerComp->SetCollisionProfileName(TEXT("OverlapAll"));
	TriggerComp->SetGenerateOverlapEvents(true);
	TriggerComp->SetCanEverAffectNavigation(false);
	RootComponent = TriggerComp;

	// Visual water plane mesh
	WaterPlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterPlane"));
	WaterPlaneMesh->SetupAttachment(RootComponent);
	WaterPlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterPlaneMesh->CastShadow = false;
	WaterPlaneMesh->SetCanEverAffectNavigation(false);

	// Load the engine plane mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		WaterPlaneMesh->SetStaticMesh(PlaneMesh.Object);
	}
}

void AWaterArea::BeginPlay()
{
	Super::BeginPlay();

	// Update trigger box extent from editable properties
	TriggerComp->SetBoxExtent(FVector(WaterExtent.X, WaterExtent.Y, 100.f));

	// Scale the plane mesh to match the water extent
	// Engine Plane is 100x100 UU, so scale = extent / 50 (half-extent to full size)
	float ScaleX = WaterExtent.X / 50.f;
	float ScaleY = WaterExtent.Y / 50.f;
	WaterPlaneMesh->SetRelativeScale3D(FVector(ScaleX, ScaleY, 1.f));

	// Create and apply the water material
	CreateWaterMaterial();

	// Bind overlap events
	TriggerComp->OnComponentBeginOverlap.AddDynamic(this, &AWaterArea::OnOverlapBegin);
	TriggerComp->OnComponentEndOverlap.AddDynamic(this, &AWaterArea::OnOverlapEnd);

	UE_LOG(LogWater, Log, TEXT("WaterArea '%s' ready: extent=(%.0f, %.0f), deep=%d"),
		*WaterAreaId, WaterExtent.X, WaterExtent.Y, bIsDeep);
}

void AWaterArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// If player was in water when leaving, notify server
	if (bPlayerInWater)
	{
		if (UWorld* World = GetWorld())
		{
			if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
			{
				if (GI->IsSocketConnected())
				{
					GI->EmitSocketEvent(TEXT("water:exit"), TEXT("{}"));
				}
			}
		}
		bPlayerInWater = false;
	}

	Super::EndPlay(EndPlayReason);
}

// ============================================================
// Water Material — runtime creation, animated translucent
// ============================================================

void AWaterArea::CreateWaterMaterial()
{
	UMaterial* Mat = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	Mat->MaterialDomain = MD_Surface;
	Mat->BlendMode = BLEND_Translucent;
	Mat->SetShadingModel(MSM_DefaultLit);  // Lit for reflections
	Mat->TwoSided = true;

	auto AddExpr = [&](UMaterialExpression* E) { Mat->GetExpressionCollection().AddExpression(E); };

	// === INPUTS ===

	// Water base color
	auto* BaseColor = NewObject<UMaterialExpressionVectorParameter>(Mat);
	BaseColor->ParameterName = TEXT("WaterColor");
	BaseColor->DefaultValue = bIsDeep ? DeepColor : ShallowColor;
	AddExpr(BaseColor);

	// Time node — this is the key to animation, updates every frame
	auto* TimeExpr = NewObject<UMaterialExpressionTime>(Mat);
	AddExpr(TimeExpr);

	// Speed constant
	auto* SpeedConst = NewObject<UMaterialExpressionConstant>(Mat);
	SpeedConst->R = WaveSpeed;
	AddExpr(SpeedConst);

	// === ANIMATION CHAIN: Time * Speed ===

	// AnimTime = Time * Speed
	auto* AnimTime = NewObject<UMaterialExpressionMultiply>(Mat);
	AnimTime->A.Connect(0, TimeExpr);
	AnimTime->B.Connect(0, SpeedConst);
	AddExpr(AnimTime);

	// === SPATIAL RIPPLE: Pure standard nodes (no Custom HLSL) ===
	// Pattern: Sine(UV.x * freq + AnimTime) creates a wave moving across the surface

	auto* TexCoord = NewObject<UMaterialExpressionTextureCoordinate>(Mat);
	AddExpr(TexCoord);

	// Extract UV.x and UV.y as separate floats
	auto* MaskX = NewObject<UMaterialExpressionComponentMask>(Mat);
	MaskX->R = true; MaskX->G = false; MaskX->B = false; MaskX->A = false;
	MaskX->Input.Connect(0, TexCoord);
	AddExpr(MaskX);

	auto* MaskY = NewObject<UMaterialExpressionComponentMask>(Mat);
	MaskY->R = false; MaskY->G = true; MaskY->B = false; MaskY->A = false;
	MaskY->Input.Connect(0, TexCoord);
	AddExpr(MaskY);

	// Helper lambda: create a single directional wave
	// Sine(UV.x * freqX + UV.y * freqY + AnimTime * speedMult)
	auto MakeWave = [&](float freqX, float freqY, float speedMult) -> UMaterialExpressionSine*
	{
		UMaterialExpression* SpatialTerm = nullptr;

		if (freqX != 0.f && freqY != 0.f)
		{
			// Diagonal: UV.x * freqX + UV.y * freqY
			auto* FX = NewObject<UMaterialExpressionConstant>(Mat); FX->R = freqX; AddExpr(FX);
			auto* MulX = NewObject<UMaterialExpressionMultiply>(Mat);
			MulX->A.Connect(0, MaskX); MulX->B.Connect(0, FX); AddExpr(MulX);

			auto* FY = NewObject<UMaterialExpressionConstant>(Mat); FY->R = freqY; AddExpr(FY);
			auto* MulY = NewObject<UMaterialExpressionMultiply>(Mat);
			MulY->A.Connect(0, MaskY); MulY->B.Connect(0, FY); AddExpr(MulY);

			auto* Sum = NewObject<UMaterialExpressionAdd>(Mat);
			Sum->A.Connect(0, MulX); Sum->B.Connect(0, MulY); AddExpr(Sum);
			SpatialTerm = Sum;
		}
		else if (freqX != 0.f)
		{
			// Horizontal: UV.x * freqX
			auto* FX = NewObject<UMaterialExpressionConstant>(Mat); FX->R = freqX; AddExpr(FX);
			auto* MulX = NewObject<UMaterialExpressionMultiply>(Mat);
			MulX->A.Connect(0, MaskX); MulX->B.Connect(0, FX); AddExpr(MulX);
			SpatialTerm = MulX;
		}
		else
		{
			// Vertical: UV.y * freqY
			auto* FY = NewObject<UMaterialExpressionConstant>(Mat); FY->R = freqY; AddExpr(FY);
			auto* MulY = NewObject<UMaterialExpressionMultiply>(Mat);
			MulY->A.Connect(0, MaskY); MulY->B.Connect(0, FY); AddExpr(MulY);
			SpatialTerm = MulY;
		}

		// Time component: AnimTime * speedMult
		auto* SM = NewObject<UMaterialExpressionConstant>(Mat); SM->R = speedMult; AddExpr(SM);
		auto* TimeTerm = NewObject<UMaterialExpressionMultiply>(Mat);
		TimeTerm->A.Connect(0, AnimTime); TimeTerm->B.Connect(0, SM); AddExpr(TimeTerm);

		// Spatial + Time
		auto* WaveIn = NewObject<UMaterialExpressionAdd>(Mat);
		WaveIn->A.Connect(0, SpatialTerm); WaveIn->B.Connect(0, TimeTerm); AddExpr(WaveIn);

		auto* WaveSine = NewObject<UMaterialExpressionSine>(Mat);
		WaveSine->Input.Connect(0, WaveIn); AddExpr(WaveSine);
		return WaveSine;
	};

	// --- Wave 1: Horizontal, fast, tight frequency ---
	//     Sine(UV.x * 10 + AnimTime * 1.0) → horizontal bands moving right
	auto* Wave1 = MakeWave(10.f, 0.f, 1.0f);

	// --- Wave 2: Vertical, slower, wider frequency ---
	//     Sine(UV.y * 7 + AnimTime * 0.6) → vertical bands moving down
	auto* Wave2 = MakeWave(0.f, 7.f, 0.6f);

	// --- Wave 3: Diagonal (45 deg), medium speed ---
	//     Sine(UV.x * 5 + UV.y * 5 + AnimTime * 0.8) → diagonal bands
	auto* Wave3 = MakeWave(5.f, 5.f, 0.8f);

	// --- Combine: each wave contributes equally, normalize to 0-1 ---
	auto* Sum12 = NewObject<UMaterialExpressionAdd>(Mat);
	Sum12->A.Connect(0, Wave1); Sum12->B.Connect(0, Wave2); AddExpr(Sum12);

	auto* Sum123 = NewObject<UMaterialExpressionAdd>(Mat);
	Sum123->A.Connect(0, Sum12); Sum123->B.Connect(0, Wave3); AddExpr(Sum123);

	// (W1+W2+W3) ranges from -3 to +3, scale to 0-1: * 0.15 + 0.5 (subtle waves)
	auto* NormScale = NewObject<UMaterialExpressionConstant>(Mat);
	NormScale->R = 0.15f; AddExpr(NormScale);

	auto* Scaled = NewObject<UMaterialExpressionMultiply>(Mat);
	Scaled->A.Connect(0, Sum123); Scaled->B.Connect(0, NormScale); AddExpr(Scaled);

	auto* HalfConst = NewObject<UMaterialExpressionConstant>(Mat);
	HalfConst->R = 0.5f; AddExpr(HalfConst);

	auto* RippleNorm = NewObject<UMaterialExpressionAdd>(Mat);
	RippleNorm->A.Connect(0, Scaled); RippleNorm->B.Connect(0, HalfConst); AddExpr(RippleNorm);

	// === EMISSIVE COLOR: Lerp(BaseColor, White highlight, RippleNorm) ===
	// Strong contrast so ripples are visible

	auto* WhiteHighlight = NewObject<UMaterialExpressionVectorParameter>(Mat);
	WhiteHighlight->ParameterName = TEXT("HighlightColor");
	WhiteHighlight->DefaultValue = FLinearColor(0.2f, 0.4f, 0.8f, 1.0f);  // brighter blue highlight
	AddExpr(WhiteHighlight);

	auto* ColorLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	ColorLerp->A.Connect(0, BaseColor);
	ColorLerp->B.Connect(0, WhiteHighlight);
	ColorLerp->Alpha.Connect(0, RippleNorm);
	AddExpr(ColorLerp);

	// === OPACITY: Spatial ripples drive transparency (most visible effect) ===
	// RippleNorm (0.25-0.75) scaled to opacity range: darker bands = more opaque, lighter = more transparent

	auto* OpacityBase = NewObject<UMaterialExpressionConstant>(Mat);
	OpacityBase->R = bIsDeep ? 0.6f : 0.45f;
	AddExpr(OpacityBase);

	auto* OpacityRange = NewObject<UMaterialExpressionConstant>(Mat);
	OpacityRange->R = bIsDeep ? 0.3f : 0.2f;  // moderate opacity variation
	AddExpr(OpacityRange);

	auto* OpacityWave = NewObject<UMaterialExpressionMultiply>(Mat);
	OpacityWave->A.Connect(0, RippleNorm);  // spatial wave pattern 0.25-0.75
	OpacityWave->B.Connect(0, OpacityRange);
	AddExpr(OpacityWave);

	auto* FinalOpacity = NewObject<UMaterialExpressionAdd>(Mat);
	FinalOpacity->A.Connect(0, OpacityBase);
	FinalOpacity->B.Connect(0, OpacityWave);
	AddExpr(FinalOpacity);

	// === ROUGHNESS: low = reflective surface ===
	auto* RoughnessVal = NewObject<UMaterialExpressionConstant>(Mat);
	RoughnessVal->R = 0.15f;  // very smooth = reflective
	AddExpr(RoughnessVal);

	// === SPECULAR: boost reflections ===
	auto* SpecularVal = NewObject<UMaterialExpressionConstant>(Mat);
	SpecularVal->R = 0.8f;  // strong specular
	AddExpr(SpecularVal);

	// === WIRE OUTPUTS ===
	Mat->GetEditorOnlyData()->BaseColor.Connect(0, ColorLerp);
	Mat->GetEditorOnlyData()->Opacity.Connect(0, FinalOpacity);
	Mat->GetEditorOnlyData()->Roughness.Connect(0, RoughnessVal);
	Mat->GetEditorOnlyData()->Specular.Connect(0, SpecularVal);

	Mat->PreEditChange(nullptr);
	Mat->PostEditChange();

	// Create MID for per-instance parameter tweaks
	WaterMID = UMaterialInstanceDynamic::Create(Mat, this);
	if (WaterMID && WaterPlaneMesh)
	{
		WaterPlaneMesh->SetMaterial(0, WaterMID);
	}

	UE_LOG(LogWater, Log, TEXT("Water material created: %s, opacity=%.2f"),
		bIsDeep ? TEXT("deep") : TEXT("shallow"),
		bIsDeep ? 0.8f : 0.55f);
}

// ============================================================
// Overlap — detect player entering/leaving water
// ============================================================

void AWaterArea::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Only react to local player pawn
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || PC->GetPawn() != OtherActor) return;

	// Spam guard (1 second cooldown)
	const double Now = FPlatformTime::Seconds();
	if (Now - LastWaterEventTime < 1.0) return;
	LastWaterEventTime = Now;

	bPlayerInWater = true;

	// Emit to server
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (GI && GI->IsSocketConnected())
	{
		TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
		Payload->SetStringField(TEXT("waterAreaId"), WaterAreaId);
		Payload->SetBoolField(TEXT("isDeep"), bIsDeep);
		GI->EmitSocketEvent(TEXT("water:enter"), Payload);

		UE_LOG(LogWater, Log, TEXT("Player entered water: %s (%s)"),
			*WaterAreaId, bIsDeep ? TEXT("deep") : TEXT("shallow"));
	}
}

void AWaterArea::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || PC->GetPawn() != OtherActor) return;

	bPlayerInWater = false;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (GI && GI->IsSocketConnected())
	{
		TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
		Payload->SetStringField(TEXT("waterAreaId"), WaterAreaId);
		GI->EmitSocketEvent(TEXT("water:exit"), Payload);

		UE_LOG(LogWater, Log, TEXT("Player exited water: %s"), *WaterAreaId);
	}
}
