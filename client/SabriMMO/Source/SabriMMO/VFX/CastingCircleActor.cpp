// CastingCircleActor.cpp — Rotating ground decal for spell casting indicator.

#include "CastingCircleActor.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

ACastingCircleActor::ACastingCircleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CircleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("CircleDecal"));
	RootComponent = CircleDecal;

	// Face downward so the decal projects onto the ground
	CircleDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	// Default size — overridden in Initialize()
	CircleDecal->DecalSize = FVector(200.f, 200.f, 200.f);
}

void ACastingCircleActor::Initialize(FLinearColor ElementColor, float Radius, float Duration)
{
	// Scale decal to requested radius
	CircleDecal->DecalSize = FVector(Radius, Radius, 100.f);

	// Create dynamic material instance
	if (BaseMaterial)
	{
		DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		CircleDecal->SetDecalMaterial(DynMaterial);
	}
	else
	{
		// If no base material assigned, try to load the default one
		static UMaterialInterface* DefaultMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/SabriMMO/VFX/Materials/M_CastingCircle.M_CastingCircle"));
		if (DefaultMat)
		{
			DynMaterial = UMaterialInstanceDynamic::Create(DefaultMat, this);
			CircleDecal->SetDecalMaterial(DynMaterial);
		}
	}

	// Set element colour and start invisible (fade in)
	FadeAlpha = 0.f;
	FadeDirection = 1.f;
	FadeSpeed = 4.f; // reach full opacity in ~0.25s

	if (DynMaterial)
	{
		DynMaterial->SetVectorParameterValue(TEXT("ElementColor"), ElementColor);
		DynMaterial->SetScalarParameterValue(TEXT("FadeAlpha"), FadeAlpha);
	}

	// Start fade-in tick (every 16ms ≈ 60fps)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FadeTimerHandle,
			FTimerDelegate::CreateUObject(this, &ACastingCircleActor::TickFade),
			0.016f,
			true
		);
	}

	// Auto-destroy after cast duration + a small buffer for fade-out
	if (Duration > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoDestroyHandle,
			FTimerDelegate::CreateLambda([this]() { FadeOut(0.3f); }),
			Duration + 0.1f,
			false
		);
	}
}

void ACastingCircleActor::FadeOut(float FadeTime)
{
	// Cancel auto-destroy if it hasn't fired yet
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoDestroyHandle);
	}

	FadeDirection = -1.f;
	FadeSpeed = (FadeTime > 0.f) ? (1.f / FadeTime) : 10.f;

	// If the fade tick isn't running, start it
	if (!FadeTimerHandle.IsValid() && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			FadeTimerHandle,
			FTimerDelegate::CreateUObject(this, &ACastingCircleActor::TickFade),
			0.016f,
			true
		);
	}
}

void ACastingCircleActor::TickFade()
{
	FadeAlpha += FadeDirection * FadeSpeed * 0.016f;
	FadeAlpha = FMath::Clamp(FadeAlpha, 0.f, 1.f);

	if (DynMaterial)
	{
		DynMaterial->SetScalarParameterValue(TEXT("FadeAlpha"), FadeAlpha);
	}

	// If fully faded out, destroy self
	if (FadeAlpha <= 0.f && FadeDirection < 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FadeTimerHandle);
		}
		Destroy();
		return;
	}

	// If fully faded in, stop ticking (save CPU)
	if (FadeAlpha >= 1.f && FadeDirection > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FadeTimerHandle);
		}
		FadeTimerHandle.Invalidate();
	}
}
