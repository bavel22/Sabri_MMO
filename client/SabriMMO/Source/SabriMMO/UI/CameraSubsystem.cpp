// CameraSubsystem.cpp — RO Classic camera. Fully C++ owned — finds or creates
// SpringArm on the pawn, forces all settings. No dependency on BP components.

#include "CameraSubsystem.h"
#include "MMOGameInstance.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCamera, Log, All);

bool UCameraSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCameraSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	// Pawn may not exist yet on BeginPlay — retry until found
	InWorld.GetTimerManager().SetTimer(SetupRetryTimer,
		FTimerDelegate::CreateLambda([this]()
		{
			SetupCamera();
		}),
		0.1f, true);

	UE_LOG(LogCamera, Log, TEXT("CameraSubsystem started — waiting for pawn"));
}

void UCameraSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SetupRetryTimer);
	}

	CachedSpringArm = nullptr;
	Super::Deinitialize();
}

// ============================================================
// SetupCamera — find or configure the SpringArm on the pawn
// ============================================================

void UCameraSubsystem::SetupCamera()
{
	if (CachedSpringArm.IsValid()) return; // Already set up

	UWorld* World = GetWorld();
	if (!World) return;
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return; // Pawn not spawned yet — timer will retry

	// Stop retrying
	World->GetTimerManager().ClearTimer(SetupRetryTimer);

	// Find all SpringArms on the pawn
	TArray<USpringArmComponent*> SpringArms;
	Pawn->GetComponents<USpringArmComponent>(SpringArms);

	USpringArmComponent* SA = nullptr;

	// Prefer the BP "SpringArm" (the game camera arm)
	for (USpringArmComponent* Candidate : SpringArms)
	{
		if (Candidate->GetName() == TEXT("SpringArm"))
		{
			SA = Candidate;
			break;
		}
	}

	// Fallback: use any SpringArm that has a camera attached
	if (!SA)
	{
		for (USpringArmComponent* Candidate : SpringArms)
		{
			TArray<USceneComponent*> Children;
			Candidate->GetChildrenComponents(false, Children);
			for (USceneComponent* Child : Children)
			{
				if (Cast<UCameraComponent>(Child))
				{
					SA = Candidate;
					break;
				}
			}
			if (SA) break;
		}
	}

	// Last fallback: first SpringArm
	if (!SA && SpringArms.Num() > 0)
	{
		SA = SpringArms[0];
	}

	if (!SA)
	{
		UE_LOG(LogCamera, Error, TEXT("No SpringArmComponent found on pawn %s!"), *Pawn->GetName());
		return;
	}

	// Force ALL settings — don't trust BP defaults
	CachedSpringArm = SA;
	CurrentYaw = 0.f;

	SA->TargetArmLength = DefaultArmLength;
	SA->SetRelativeRotation(FRotator(FixedPitch, CurrentYaw, 0.f));
	SA->bUsePawnControlRotation = false;
	SA->bInheritPitch = false;
	SA->bInheritYaw = false;
	SA->bInheritRoll = false;
	SA->bDoCollisionTest = false;
	SA->bEnableCameraLag = false;
	SA->bEnableCameraRotationLag = false;

	UE_LOG(LogCamera, Log, TEXT("RO Classic camera configured: pitch=%.0f, yaw=%.0f, arm=%.0f on %s"),
		FixedPitch, CurrentYaw, SA->TargetArmLength, *SA->GetName());
}

// ============================================================
// Public API — called directly from character input handlers
// ============================================================

void UCameraSubsystem::OnRightClickStarted()
{
	bIsRotatingCamera = true;
	bDidRotateThisClick = false;
}

void UCameraSubsystem::OnRightClickCompleted()
{
	bIsRotatingCamera = false;
}

void UCameraSubsystem::OnMouseDelta(const FVector2D& Delta)
{
	if (!bIsRotatingCamera) return;

	USpringArmComponent* SA = CachedSpringArm.Get();
	if (!SA) return;

	bDidRotateThisClick = true;
	CurrentYaw += Delta.X * RotationSensitivity;
	SA->SetRelativeRotation(FRotator(FixedPitch, CurrentYaw, 0.f));
}

void UCameraSubsystem::OnZoom(float AxisValue)
{
	USpringArmComponent* SA = CachedSpringArm.Get();
	if (!SA) return;

	float NewLength = SA->TargetArmLength - (AxisValue * ZoomSpeed);
	SA->TargetArmLength = FMath::Clamp(NewLength, MinArmLength, MaxArmLength);
}

// Occlusion handling removed (Option A): the sprite material uses
// BLEND_Translucent + bDisableDepthTest, so the character renders on top of
// any wall/landscape between camera and player. No need to hide world geometry.
