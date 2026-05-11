// WaterArea.cpp — RO-style water: animated translucent plane + overlap trigger.
// Visual: runtime material with depth-aware shallow/deep gradient (color, opacity, roughness)
//         and three crossing sine-wave ripples.
// Gameplay: emits water:enter/water:exit to server for skill gating.
// Mixed-mode: at construction, raycasts a downward grid to auto-detect deep cells, then
//             greedy-merges them into rectangles and spawns one nav-modifier UBoxComponent
//             per rect (each marked NavArea_Null so deep regions are cut out of the navmesh).

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
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "NavAreas/NavArea_Null.h"

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		WaterPlaneMesh->SetStaticMesh(PlaneMesh.Object);
	}
}

void AWaterArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyExtentToComponents();
	ClearDeepNavBoxes();
	PerformDepthScan();
	SpawnDeepNavBoxes();
}

void AWaterArea::BeginPlay()
{
	Super::BeginPlay();

	// Re-sync runtime visuals + nav boxes (covers runtime-spawned actors and editor sessions
	// where OnConstruction hasn't been re-run since terrain edits).
	ApplyExtentToComponents();
	ClearDeepNavBoxes();
	PerformDepthScan();
	SpawnDeepNavBoxes();

	CreateWaterMaterial();

	TriggerComp->OnComponentBeginOverlap.AddDynamic(this, &AWaterArea::OnOverlapBegin);
	TriggerComp->OnComponentEndOverlap.AddDynamic(this, &AWaterArea::OnOverlapEnd);

	int32 DeepCellCount = 0;
	for (bool b : DeepCells) if (b) DeepCellCount++;
	UE_LOG(LogWater, Log, TEXT("WaterArea '%s' ready: extent=(%.0f, %.0f), deep cells=%d/%d, nav boxes=%d"),
		*WaterAreaId, WaterExtent.X, WaterExtent.Y,
		DeepCellCount, DeepCells.Num(), DeepNavBoxes.Num());
}

void AWaterArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

void AWaterArea::ApplyExtentToComponents()
{
	if (TriggerComp)
	{
		TriggerComp->SetBoxExtent(FVector(WaterExtent.X, WaterExtent.Y, 100.f));
	}
	if (WaterPlaneMesh)
	{
		// Engine Plane is 100x100 UU, scale = half-extent / 50
		const float ScaleX = WaterExtent.X / 50.f;
		const float ScaleY = WaterExtent.Y / 50.f;
		WaterPlaneMesh->SetRelativeScale3D(FVector(ScaleX, ScaleY, 1.f));
	}
}

// ============================================================
// Depth scan — raycast grid below the water plane, mark deep cells
// ============================================================

void AWaterArea::PerformDepthScan()
{
	const int32 N = FMath::Clamp(DepthSampleResolution, 4, 64);
	DeepCells.Reset();
	DeepCells.SetNumZeroed(N * N);

	if (!bAutoDetectDeep)
	{
		// Manual override: whole area treated uniformly per bIsDeep
		if (bIsDeep)
		{
			for (int32 i = 0; i < N * N; i++) DeepCells[i] = true;
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	const FTransform Xform = GetActorTransform();
	const float Hx = WaterExtent.X;
	const float Hy = WaterExtent.Y;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WaterDepthScan), true);
	Params.AddIgnoredActor(this);

	const float TraceLength = 2000.f;

	for (int32 j = 0; j < N; j++)
	{
		for (int32 i = 0; i < N; i++)
		{
			// Sample at the cell center: ((i + 0.5)/N) maps to [0..1], rescale to [-Hx..+Hx]
			const float xLocal = ((float)i + 0.5f) * (2.f * Hx / (float)N) - Hx;
			const float yLocal = ((float)j + 0.5f) * (2.f * Hy / (float)N) - Hy;
			const FVector LocalPos(xLocal, yLocal, 0.f);
			const FVector WorldStart = Xform.TransformPosition(LocalPos);
			const FVector WorldEnd = WorldStart - FVector(0.f, 0.f, TraceLength);

			FHitResult Hit;
			const bool bHit = World->LineTraceSingleByChannel(
				Hit, WorldStart, WorldEnd, ECC_Visibility, Params);

			// No floor below = no walkable surface = treat as deep (block)
			if (!bHit)
			{
				DeepCells[j * N + i] = true;
				continue;
			}

			const float Depth = WorldStart.Z - Hit.ImpactPoint.Z;
			DeepCells[j * N + i] = (Depth >= DeepDepthThreshold);
		}
	}
}

// ============================================================
// Greedy rectangle merge — coalesce contiguous deep cells into
// the smallest set of axis-aligned rectangles, then spawn one
// nav-modifier UBoxComponent per rectangle.
// ============================================================

void AWaterArea::SpawnDeepNavBoxes()
{
	const int32 N = FMath::Clamp(DepthSampleResolution, 4, 64);
	if (DeepCells.Num() != N * N) return;

	const float Hx = WaterExtent.X;
	const float Hy = WaterExtent.Y;
	const float CellW = 2.f * Hx / (float)N;
	const float CellH = 2.f * Hy / (float)N;

	TArray<bool> Mask = DeepCells;  // working copy

	for (int32 j = 0; j < N; j++)
	{
		for (int32 i = 0; i < N; i++)
		{
			if (!Mask[j * N + i]) continue;

			// Expand right along row j as long as cells are deep
			int32 RightI = i;
			while (RightI + 1 < N && Mask[j * N + (RightI + 1)]) RightI++;

			// Expand downward only while every cell across [i..RightI] in the next row is deep
			int32 BottomJ = j;
			while (BottomJ + 1 < N)
			{
				bool bRowAllDeep = true;
				for (int32 ii = i; ii <= RightI; ii++)
				{
					if (!Mask[(BottomJ + 1) * N + ii]) { bRowAllDeep = false; break; }
				}
				if (!bRowAllDeep) break;
				BottomJ++;
			}

			// Local-space rectangle bounds
			const float MinX = (float)i * CellW - Hx;
			const float MaxX = (float)(RightI + 1) * CellW - Hx;
			const float MinY = (float)j * CellH - Hy;
			const float MaxY = (float)(BottomJ + 1) * CellH - Hy;
			const FVector Center((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f, 0.f);
			const FVector Extent((MaxX - MinX) * 0.5f, (MaxY - MinY) * 0.5f, 200.f);

			UBoxComponent* Box = NewObject<UBoxComponent>(this);
			if (Box)
			{
				Box->SetupAttachment(RootComponent);
				Box->RegisterComponent();
				Box->SetRelativeLocation(Center);
				Box->SetBoxExtent(Extent);
				// QueryOnly + Ignore-all keeps the box from blocking pawns/projectiles —
				// it exists purely as a nav-modifier shape.
				Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				Box->SetCollisionResponseToAllChannels(ECR_Ignore);
				Box->SetCanEverAffectNavigation(true);
				Box->SetAreaClassOverride(UNavArea_Null::StaticClass());
				Box->SetHiddenInGame(true);
				Box->bDynamicObstacle = true;  // updates runtime navmesh if rebuilt
				DeepNavBoxes.Add(Box);
			}

			// Mark consumed cells so they aren't picked up by later iterations
			for (int32 jj = j; jj <= BottomJ; jj++)
			{
				for (int32 ii = i; ii <= RightI; ii++)
				{
					Mask[jj * N + ii] = false;
				}
			}
		}
	}
}

void AWaterArea::ClearDeepNavBoxes()
{
	for (UBoxComponent* Box : DeepNavBoxes)
	{
		if (IsValid(Box))
		{
			Box->DestroyComponent();
		}
	}
	DeepNavBoxes.Reset();
}

// ============================================================
// Water Material — depth-aware translucent gradient + 3 sine waves
// ============================================================

void AWaterArea::CreateWaterMaterial()
{
	UMaterial* Mat = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	Mat->MaterialDomain = MD_Surface;
	Mat->BlendMode = BLEND_Translucent;
	Mat->SetShadingModel(MSM_DefaultLit);
	Mat->TwoSided = true;

	auto AddExpr = [&](UMaterialExpression* E) { Mat->GetExpressionCollection().AddExpression(E); };

	// === DEPTH FACTOR — drives all gradients ===
	// WaterDepth = SceneDepth − PixelDepth (camera-space distance from water surface
	// to the opaque geometry behind/below it). At our isometric -55° camera this
	// approximates vertical depth closely enough.
	auto* SceneDepthExpr = NewObject<UMaterialExpressionSceneDepth>(Mat);
	AddExpr(SceneDepthExpr);

	auto* PixelDepthExpr = NewObject<UMaterialExpressionPixelDepth>(Mat);
	AddExpr(PixelDepthExpr);

	auto* WaterDepth = NewObject<UMaterialExpressionSubtract>(Mat);
	WaterDepth->A.Connect(0, SceneDepthExpr);
	WaterDepth->B.Connect(0, PixelDepthExpr);
	AddExpr(WaterDepth);

	auto* DepthFalloff = NewObject<UMaterialExpressionScalarParameter>(Mat);
	DepthFalloff->ParameterName = TEXT("DepthFalloff");
	DepthFalloff->DefaultValue = 250.f;  // depth at which water is "fully deep" looking
	AddExpr(DepthFalloff);

	auto* DepthDiv = NewObject<UMaterialExpressionDivide>(Mat);
	DepthDiv->A.Connect(0, WaterDepth);
	DepthDiv->B.Connect(0, DepthFalloff);
	AddExpr(DepthDiv);

	auto* DepthFactor = NewObject<UMaterialExpressionClamp>(Mat);
	DepthFactor->Input.Connect(0, DepthDiv);
	DepthFactor->MinDefault = 0.f;
	DepthFactor->MaxDefault = 1.f;
	AddExpr(DepthFactor);

	// === STAGE 2 DEPTH FACTOR — drives deep→abyss transition past DepthFalloff ===
	// Stage2Factor = saturate((WaterDepth - DepthFalloff) / (AbyssDepth - DepthFalloff))
	auto* AbyssDepthParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
	AbyssDepthParam->ParameterName = TEXT("AbyssDepth");
	AbyssDepthParam->DefaultValue = AbyssDepth;
	AddExpr(AbyssDepthParam);

	auto* Stage2Numerator = NewObject<UMaterialExpressionSubtract>(Mat);
	Stage2Numerator->A.Connect(0, WaterDepth);
	Stage2Numerator->B.Connect(0, DepthFalloff);
	AddExpr(Stage2Numerator);

	auto* Stage2Range = NewObject<UMaterialExpressionSubtract>(Mat);
	Stage2Range->A.Connect(0, AbyssDepthParam);
	Stage2Range->B.Connect(0, DepthFalloff);
	AddExpr(Stage2Range);

	auto* Stage2Div = NewObject<UMaterialExpressionDivide>(Mat);
	Stage2Div->A.Connect(0, Stage2Numerator);
	Stage2Div->B.Connect(0, Stage2Range);
	AddExpr(Stage2Div);

	auto* Stage2Factor = NewObject<UMaterialExpressionClamp>(Mat);
	Stage2Factor->Input.Connect(0, Stage2Div);
	Stage2Factor->MinDefault = 0.f;
	Stage2Factor->MaxDefault = 1.f;
	AddExpr(Stage2Factor);

	// === COLOR PARAMS ===
	auto* ShallowColorParam = NewObject<UMaterialExpressionVectorParameter>(Mat);
	ShallowColorParam->ParameterName = TEXT("ShallowColor");
	ShallowColorParam->DefaultValue = ShallowColor;
	AddExpr(ShallowColorParam);

	auto* DeepColorParam = NewObject<UMaterialExpressionVectorParameter>(Mat);
	DeepColorParam->ParameterName = TEXT("DeepColor");
	DeepColorParam->DefaultValue = DeepColor;
	AddExpr(DeepColorParam);

	auto* AbyssColorParam = NewObject<UMaterialExpressionVectorParameter>(Mat);
	AbyssColorParam->ParameterName = TEXT("AbyssColor");
	AbyssColorParam->DefaultValue = AbyssColor;
	AddExpr(AbyssColorParam);

	auto* HighlightColorParam = NewObject<UMaterialExpressionVectorParameter>(Mat);
	HighlightColorParam->ParameterName = TEXT("HighlightColor");
	HighlightColorParam->DefaultValue = FLinearColor(0.4f, 0.6f, 0.85f, 1.f);
	AddExpr(HighlightColorParam);

	// Stage 1: shallow → deep across [0, DepthFalloff]
	auto* DepthColorLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	DepthColorLerp->A.Connect(0, ShallowColorParam);
	DepthColorLerp->B.Connect(0, DeepColorParam);
	DepthColorLerp->Alpha.Connect(0, DepthFactor);
	AddExpr(DepthColorLerp);

	// Stage 2: deep → abyss across [DepthFalloff, AbyssDepth]
	auto* FinalDepthColor = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	FinalDepthColor->A.Connect(0, DepthColorLerp);
	FinalDepthColor->B.Connect(0, AbyssColorParam);
	FinalDepthColor->Alpha.Connect(0, Stage2Factor);
	AddExpr(FinalDepthColor);

	// === ANIMATION — 3 crossing sine waves ===
	auto* TimeExpr = NewObject<UMaterialExpressionTime>(Mat); AddExpr(TimeExpr);

	auto* SpeedConst = NewObject<UMaterialExpressionConstant>(Mat);
	SpeedConst->R = WaveSpeed; AddExpr(SpeedConst);

	auto* AnimTime = NewObject<UMaterialExpressionMultiply>(Mat);
	AnimTime->A.Connect(0, TimeExpr);
	AnimTime->B.Connect(0, SpeedConst);
	AddExpr(AnimTime);

	auto* TexCoord = NewObject<UMaterialExpressionTextureCoordinate>(Mat); AddExpr(TexCoord);

	auto* MaskX = NewObject<UMaterialExpressionComponentMask>(Mat);
	MaskX->R = true; MaskX->G = false; MaskX->B = false; MaskX->A = false;
	MaskX->Input.Connect(0, TexCoord); AddExpr(MaskX);

	auto* MaskY = NewObject<UMaterialExpressionComponentMask>(Mat);
	MaskY->R = false; MaskY->G = true; MaskY->B = false; MaskY->A = false;
	MaskY->Input.Connect(0, TexCoord); AddExpr(MaskY);

	auto MakeWave = [&](float freqX, float freqY, float speedMult) -> UMaterialExpressionSine*
	{
		UMaterialExpression* SpatialTerm = nullptr;

		if (freqX != 0.f && freqY != 0.f)
		{
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
			auto* FX = NewObject<UMaterialExpressionConstant>(Mat); FX->R = freqX; AddExpr(FX);
			auto* MulX = NewObject<UMaterialExpressionMultiply>(Mat);
			MulX->A.Connect(0, MaskX); MulX->B.Connect(0, FX); AddExpr(MulX);
			SpatialTerm = MulX;
		}
		else
		{
			auto* FY = NewObject<UMaterialExpressionConstant>(Mat); FY->R = freqY; AddExpr(FY);
			auto* MulY = NewObject<UMaterialExpressionMultiply>(Mat);
			MulY->A.Connect(0, MaskY); MulY->B.Connect(0, FY); AddExpr(MulY);
			SpatialTerm = MulY;
		}

		auto* SM = NewObject<UMaterialExpressionConstant>(Mat); SM->R = speedMult; AddExpr(SM);
		auto* TimeTerm = NewObject<UMaterialExpressionMultiply>(Mat);
		TimeTerm->A.Connect(0, AnimTime); TimeTerm->B.Connect(0, SM); AddExpr(TimeTerm);

		auto* WaveIn = NewObject<UMaterialExpressionAdd>(Mat);
		WaveIn->A.Connect(0, SpatialTerm); WaveIn->B.Connect(0, TimeTerm); AddExpr(WaveIn);

		auto* WaveSine = NewObject<UMaterialExpressionSine>(Mat);
		WaveSine->Input.Connect(0, WaveIn); AddExpr(WaveSine);
		return WaveSine;
	};

	auto* Wave1 = MakeWave(10.f, 0.f, 1.0f);
	auto* Wave2 = MakeWave(0.f, 7.f, 0.6f);
	auto* Wave3 = MakeWave(5.f, 5.f, 0.8f);

	auto* Sum12 = NewObject<UMaterialExpressionAdd>(Mat);
	Sum12->A.Connect(0, Wave1); Sum12->B.Connect(0, Wave2); AddExpr(Sum12);

	auto* Sum123 = NewObject<UMaterialExpressionAdd>(Mat);
	Sum123->A.Connect(0, Sum12); Sum123->B.Connect(0, Wave3); AddExpr(Sum123);

	auto* NormScale = NewObject<UMaterialExpressionConstant>(Mat);
	NormScale->R = 0.15f; AddExpr(NormScale);

	auto* Scaled = NewObject<UMaterialExpressionMultiply>(Mat);
	Scaled->A.Connect(0, Sum123); Scaled->B.Connect(0, NormScale); AddExpr(Scaled);

	auto* HalfConst = NewObject<UMaterialExpressionConstant>(Mat);
	HalfConst->R = 0.5f; AddExpr(HalfConst);

	auto* RippleNorm = NewObject<UMaterialExpressionAdd>(Mat);
	RippleNorm->A.Connect(0, Scaled); RippleNorm->B.Connect(0, HalfConst); AddExpr(RippleNorm);

	// === RIPPLE → COLOR (intensity falls off with depth) ===
	// RippleAmplitude = lerp(0.5, 0.15, DepthFactor) — calmer in deeper water
	auto* RippAmpHigh = NewObject<UMaterialExpressionConstant>(Mat); RippAmpHigh->R = 0.5f; AddExpr(RippAmpHigh);
	auto* RippAmpLow = NewObject<UMaterialExpressionConstant>(Mat); RippAmpLow->R = 0.15f; AddExpr(RippAmpLow);
	auto* RippAmpLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	RippAmpLerp->A.Connect(0, RippAmpHigh);
	RippAmpLerp->B.Connect(0, RippAmpLow);
	RippAmpLerp->Alpha.Connect(0, DepthFactor);
	AddExpr(RippAmpLerp);

	auto* RippleAlpha = NewObject<UMaterialExpressionMultiply>(Mat);
	RippleAlpha->A.Connect(0, RippleNorm);
	RippleAlpha->B.Connect(0, RippAmpLerp);
	AddExpr(RippleAlpha);

	auto* FinalColor = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	FinalColor->A.Connect(0, FinalDepthColor);
	FinalColor->B.Connect(0, HighlightColorParam);
	FinalColor->Alpha.Connect(0, RippleAlpha);
	AddExpr(FinalColor);

	// === OPACITY — depth-driven (shallow translucent → deep mostly opaque) ===
	auto* ShallowOpacityParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
	ShallowOpacityParam->ParameterName = TEXT("ShallowOpacity");
	ShallowOpacityParam->DefaultValue = 0.5f;
	AddExpr(ShallowOpacityParam);

	auto* DeepOpacityParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
	DeepOpacityParam->ParameterName = TEXT("DeepOpacity");
	DeepOpacityParam->DefaultValue = 0.92f;
	AddExpr(DeepOpacityParam);

	auto* AbyssOpacityParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
	AbyssOpacityParam->ParameterName = TEXT("AbyssOpacity");
	AbyssOpacityParam->DefaultValue = AbyssOpacity;
	AddExpr(AbyssOpacityParam);

	// Stage 1 opacity: shallow → deep
	auto* OpacityLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	OpacityLerp->A.Connect(0, ShallowOpacityParam);
	OpacityLerp->B.Connect(0, DeepOpacityParam);
	OpacityLerp->Alpha.Connect(0, DepthFactor);
	AddExpr(OpacityLerp);

	// Stage 2 opacity: deep → abyss (reaches AbyssOpacity, e.g. 1.0 = fully opaque)
	auto* FinalOpacityLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	FinalOpacityLerp->A.Connect(0, OpacityLerp);
	FinalOpacityLerp->B.Connect(0, AbyssOpacityParam);
	FinalOpacityLerp->Alpha.Connect(0, Stage2Factor);
	AddExpr(FinalOpacityLerp);

	// === ROUGHNESS — depth-driven (shallow more diffuse → deep glassier) ===
	auto* RoughShallow = NewObject<UMaterialExpressionConstant>(Mat); RoughShallow->R = 0.3f; AddExpr(RoughShallow);
	auto* RoughDeep = NewObject<UMaterialExpressionConstant>(Mat); RoughDeep->R = 0.08f; AddExpr(RoughDeep);
	auto* RoughLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	RoughLerp->A.Connect(0, RoughShallow);
	RoughLerp->B.Connect(0, RoughDeep);
	RoughLerp->Alpha.Connect(0, DepthFactor);
	AddExpr(RoughLerp);

	// === SPECULAR — depth-driven ===
	auto* SpecShallow = NewObject<UMaterialExpressionConstant>(Mat); SpecShallow->R = 0.6f; AddExpr(SpecShallow);
	auto* SpecDeep = NewObject<UMaterialExpressionConstant>(Mat); SpecDeep->R = 0.95f; AddExpr(SpecDeep);
	auto* SpecLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
	SpecLerp->A.Connect(0, SpecShallow);
	SpecLerp->B.Connect(0, SpecDeep);
	SpecLerp->Alpha.Connect(0, DepthFactor);
	AddExpr(SpecLerp);

	// === WIRE OUTPUTS ===
	Mat->GetEditorOnlyData()->BaseColor.Connect(0, FinalColor);
	Mat->GetEditorOnlyData()->Opacity.Connect(0, FinalOpacityLerp);
	Mat->GetEditorOnlyData()->Roughness.Connect(0, RoughLerp);
	Mat->GetEditorOnlyData()->Specular.Connect(0, SpecLerp);

	Mat->PreEditChange(nullptr);
	Mat->PostEditChange();

	WaterMID = UMaterialInstanceDynamic::Create(Mat, this);
	if (WaterMID && WaterPlaneMesh)
	{
		WaterPlaneMesh->SetMaterial(0, WaterMID);
	}
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

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || PC->GetPawn() != OtherActor) return;

	const double Now = FPlatformTime::Seconds();
	if (Now - LastWaterEventTime < 1.0) return;
	LastWaterEventTime = Now;

	bPlayerInWater = true;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (GI && GI->IsSocketConnected())
	{
		// Whole-area "is this water containing any deep cells" — used by server for
		// remote-player visual cues, not for skill gating (skills only care that the
		// player is in water at all).
		bool bAnyDeep = false;
		for (bool b : DeepCells) { if (b) { bAnyDeep = true; break; } }

		TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
		Payload->SetStringField(TEXT("waterAreaId"), WaterAreaId);
		Payload->SetBoolField(TEXT("isDeep"), bAnyDeep);
		GI->EmitSocketEvent(TEXT("water:enter"), Payload);

		UE_LOG(LogWater, Log, TEXT("Player entered water: %s (anyDeep=%d)"),
			*WaterAreaId, bAnyDeep ? 1 : 0);
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
