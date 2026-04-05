// PostProcessSubsystem.cpp — RO Classic visual style: auto-lighting, post-process,
// and runtime master environment material.
//
// Spawns DirectionalLight + SkyLight + HeightFog per zone automatically.
// Creates M_Environment_Stylized at runtime (fully rough, diffuse-only, warm tint).
// See: docsNew/05_Development/RO_Classic_Visual_Style_Research.md

#include "PostProcessSubsystem.h"
#include "MMOGameInstance.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogPostProcess, Log, All);

bool UPostProcessSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UPostProcessSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	CurrentZone = GI->bIsZoneTransitioning
		? GI->PendingZoneName
		: GI->CurrentZoneName;

	SetupPostProcessVolume();
	SetupSceneLighting(CurrentZone);
	// Environment material created but NOT auto-applied to avoid darkening scene.
	// Will be used when proper 3D assets are imported.
	CreateEnvironmentMaterial();
	ApplyZonePreset(CurrentZone);

	UE_LOG(LogPostProcess, Log, TEXT("PostProcessSubsystem initialized for zone: %s"), *CurrentZone);
}

void UPostProcessSubsystem::Deinitialize()
{
	PPVolume = nullptr;
	SunLight = nullptr;
	AmbientLight = nullptr;
	HeightFog = nullptr;
	Super::Deinitialize();
}

// ============================================================
// Auto-lighting — spawn DirectionalLight + SkyLight + HeightFog
// RO Classic: warm, flat, high ambient fill, minimal shadows
// ============================================================

void UPostProcessSubsystem::SetupSceneLighting(const FString& ZoneName)
{
	UWorld* World = GetWorld();
	if (!World) return;

	bool bIsDungeon = ZoneName.Contains(TEXT("dungeon"));

	// --- Directional Light (sun) ---
	// Find existing or spawn. Dungeons get no directional light.
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		SunLight = *It;
		break;
	}

	if (!bIsDungeon)
	{
		if (!SunLight)
		{
			FActorSpawnParameters Params;
			Params.Name = FName(TEXT("RO_SunLight"));
			SunLight = World->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		}

		if (SunLight)
		{
			UDirectionalLightComponent* LC = SunLight->GetComponent();
			if (LC)
			{
				// RO: warm directional, moderate intensity, afternoon angle
				LC->SetIntensity(3.14f);  // ~pi lux, natural sunlight
				LC->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));  // warm white
				LC->SetCastShadows(true);
				SunLight->SetActorRotation(FRotator(-50.f, 135.f, 0.f));  // afternoon angle

				UE_LOG(LogPostProcess, Log, TEXT("Directional light configured: intensity=3.14, warm white, pitch=-50"));
			}
		}
	}
	else if (SunLight)
	{
		// Dungeons: disable directional light
		if (UDirectionalLightComponent* LC = SunLight->GetComponent())
		{
			LC->SetIntensity(0.f);
		}
	}

	// --- Sky Light (ambient fill) ---
	// RO has very flat lighting — high sky light intensity fills shadows
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		AmbientLight = *It;
		break;
	}

	if (!AmbientLight)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(TEXT("RO_SkyLight"));
		AmbientLight = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(), FVector(0.f, 0.f, 500.f), FRotator::ZeroRotator, Params);
	}

	if (AmbientLight)
	{
		USkyLightComponent* SC = AmbientLight->GetLightComponent();
		if (SC)
		{
			SC->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			// RO: HIGH ambient fill = flat lighting, minimal shadow contrast
			SC->SetIntensity(bIsDungeon ? 0.3f : 2.5f);
			SC->SetLightColor(bIsDungeon
				? FLinearColor(0.4f, 0.4f, 0.6f)   // cool blue for dungeons
				: FLinearColor(0.95f, 0.92f, 0.85f)); // warm for outdoors
			SC->RecaptureSky();

			UE_LOG(LogPostProcess, Log, TEXT("Sky light configured: intensity=%.1f, %s"),
				bIsDungeon ? 0.3f : 2.5f, bIsDungeon ? TEXT("cool") : TEXT("warm"));
		}
	}

	// --- Exponential Height Fog ---
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		HeightFog = *It;
		break;
	}

	if (!HeightFog)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(TEXT("RO_HeightFog"));
		HeightFog = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	if (HeightFog)
	{
		UExponentialHeightFogComponent* FC = HeightFog->GetComponent();
		if (FC)
		{
			if (bIsDungeon)
			{
				// Dungeon: dense colored fog for atmosphere
				FC->SetFogDensity(0.03f);
				FC->SetFogHeightFalloff(2.0f);
				FC->SetFogInscatteringColor(FLinearColor(0.1f, 0.08f, 0.15f));  // dark purple
			}
			else
			{
				// Outdoor: light warm fog for depth
				FC->SetFogDensity(0.004f);
				FC->SetFogHeightFalloff(0.5f);
				FC->SetFogInscatteringColor(FLinearColor(0.7f, 0.65f, 0.5f));  // warm haze
			}

			UE_LOG(LogPostProcess, Log, TEXT("Height fog configured: density=%.3f, %s"),
				bIsDungeon ? 0.03f : 0.004f, bIsDungeon ? TEXT("dense purple") : TEXT("light warm"));
		}
	}
}

// ============================================================
// Runtime M_Environment_Stylized — diffuse-only, fully rough, warm tint
// Same pattern as SpriteCharacterActor::CreateSpriteMaterial()
// ============================================================

void UPostProcessSubsystem::CreateEnvironmentMaterial()
{
	if (EnvMaterial) return;  // already created

	EnvMaterial = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	EnvMaterial->MaterialDomain = MD_Surface;
	EnvMaterial->BlendMode = BLEND_Opaque;
	EnvMaterial->SetShadingModel(MSM_DefaultLit);
	EnvMaterial->TwoSided = false;

	// --- BaseColor: TintColor parameter (warm stone default) ---
	auto* TintParam = NewObject<UMaterialExpressionVectorParameter>(EnvMaterial);
	TintParam->ParameterName = TEXT("TintColor");
	TintParam->DefaultValue = FLinearColor(0.65f, 0.55f, 0.42f, 1.0f);  // warm stone
	EnvMaterial->GetExpressionCollection().AddExpression(TintParam);

	EnvMaterial->GetEditorOnlyData()->BaseColor.Connect(0, TintParam);

	// --- Roughness: fully rough (RO has no specular) ---
	auto* RoughnessParam = NewObject<UMaterialExpressionScalarParameter>(EnvMaterial);
	RoughnessParam->ParameterName = TEXT("Roughness");
	RoughnessParam->DefaultValue = 0.95f;
	EnvMaterial->GetExpressionCollection().AddExpression(RoughnessParam);

	EnvMaterial->GetEditorOnlyData()->Roughness.Connect(0, RoughnessParam);

	// --- Metallic: 0 (RO has no metallic surfaces) ---
	auto* MetallicConst = NewObject<UMaterialExpressionConstant>(EnvMaterial);
	MetallicConst->R = 0.0f;
	EnvMaterial->GetExpressionCollection().AddExpression(MetallicConst);

	EnvMaterial->GetEditorOnlyData()->Metallic.Connect(0, MetallicConst);

	EnvMaterial->PreEditChange(nullptr);
	EnvMaterial->PostEditChange();

	UE_LOG(LogPostProcess, Log, TEXT("Created runtime M_Environment_Stylized (diffuse-only, roughness=0.95)"));
}

// ============================================================
// Auto-apply environment material to all static meshes (except sprites)
// ============================================================

void UPostProcessSubsystem::ApplyEnvironmentMaterial()
{
	if (!EnvMaterial) return;

	UWorld* World = GetWorld();
	if (!World) return;

	int32 AppliedCount = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		// Skip certain actor types that shouldn't get the environment material
		FString ClassName = Actor->GetClass()->GetName();
		if (ClassName.Contains(TEXT("SpriteCharacter")) ||
			ClassName.Contains(TEXT("MMOCharacter")) ||
			ClassName.Contains(TEXT("PlayerController")) ||
			ClassName.Contains(TEXT("GameMode")) ||
			ClassName.Contains(TEXT("Light")) ||
			ClassName.Contains(TEXT("Fog")) ||
			ClassName.Contains(TEXT("PostProcess")) ||
			ClassName.Contains(TEXT("Sky")) ||
			ClassName.Contains(TEXT("WarpPortal")) ||
			ClassName.Contains(TEXT("KafraNPC")) ||
			ClassName.Contains(TEXT("NavMesh")) ||
			ClassName.Contains(TEXT("SocketManager")))
		{
			continue;
		}

		// Find StaticMeshComponents and apply material
		TArray<UStaticMeshComponent*> MeshComps;
		Actor->GetComponents<UStaticMeshComponent>(MeshComps);

		for (UStaticMeshComponent* SMC : MeshComps)
		{
			if (!SMC || SMC->GetNumMaterials() == 0) continue;

			// Create MID from our environment material with per-mesh warm color variation
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(EnvMaterial, SMC);
			if (MID)
			{
				// Slight random color variation per mesh for visual interest
				float Variation = FMath::FRandRange(-0.05f, 0.05f);
				MID->SetVectorParameterValue(TEXT("TintColor"),
					FLinearColor(0.65f + Variation, 0.55f + Variation, 0.42f + Variation * 0.5f, 1.0f));

				for (int32 i = 0; i < SMC->GetNumMaterials(); i++)
				{
					SMC->SetMaterial(i, MID);
				}
				AppliedCount++;
			}
		}
	}

	UE_LOG(LogPostProcess, Log, TEXT("Applied M_Environment_Stylized to %d static mesh components"), AppliedCount);
}

// ============================================================
// PostProcessVolume setup
// ============================================================

void UPostProcessSubsystem::SetupPostProcessVolume()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		PPVolume = *It;
		break;
	}

	if (!PPVolume)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(TEXT("PPVolume_Global"));
		PPVolume = World->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	if (!PPVolume)
	{
		UE_LOG(LogPostProcess, Error, TEXT("Failed to create PostProcessVolume"));
		return;
	}

	PPVolume->bUnbound = true;
	PPVolume->Priority = 0.f;
}

// ============================================================
// Per-zone presets
// ============================================================

void UPostProcessSubsystem::ApplyZonePreset(const FString& ZoneName)
{
	if (!PPVolume) return;

	FPostProcessSettings& S = PPVolume->Settings;

	float Bloom = 0.35f;
	float BloomThreshold = 1.5f;
	float Vignette = 0.25f;
	float ExposureBias = 0.0f;
	float WhiteTemp = 6500.f;
	FVector4 GainHighlights(1.0f, 1.0f, 1.0f, 1.0f);

	if (ZoneName == TEXT("prontera"))
	{
		Bloom = 0.4f;
		Vignette = 0.25f;
		ExposureBias = 1.5f;
		WhiteTemp = 6800.f;
		GainHighlights = FVector4(1.03f, 1.0f, 0.96f, 1.0f);
	}
	else if (ZoneName == TEXT("prontera_south"))
	{
		Bloom = 0.35f;
		Vignette = 0.2f;
		ExposureBias = 1.5f;
		WhiteTemp = 6500.f;
		GainHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (ZoneName == TEXT("prontera_north"))
	{
		Bloom = 0.35f;
		Vignette = 0.2f;
		ExposureBias = 1.5f;
		WhiteTemp = 6200.f;
		GainHighlights = FVector4(0.98f, 0.98f, 1.02f, 1.0f);
	}
	else if (ZoneName == TEXT("prt_dungeon_01"))
	{
		Bloom = 0.2f;
		Vignette = 0.4f;
		ExposureBias = 0.0f;
		WhiteTemp = 5000.f;
		GainHighlights = FVector4(0.92f, 0.92f, 1.05f, 1.0f);
	}

	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = Bloom;
	S.bOverride_BloomThreshold = true;
	S.BloomThreshold = BloomThreshold;

	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = Vignette;

	S.bOverride_WhiteTemp = true;
	S.WhiteTemp = WhiteTemp;

	S.bOverride_ColorSaturation = false;
	S.bOverride_ColorContrast = false;

	S.bOverride_ColorGainHighlights = true;
	S.ColorGainHighlights = GainHighlights;

	S.bOverride_AutoExposureSpeedUp = true;
	S.bOverride_AutoExposureSpeedDown = true;
	S.AutoExposureSpeedUp = 20.0f;
	S.AutoExposureSpeedDown = 20.0f;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = ExposureBias;

	S.WeightedBlendables.Array.Empty();

	UE_LOG(LogPostProcess, Log,
		TEXT("Zone preset: %s — Bloom=%.2f, WhiteTemp=%.0f, ExpBias=%.1f"),
		*ZoneName, Bloom, WhiteTemp, ExposureBias);
}
