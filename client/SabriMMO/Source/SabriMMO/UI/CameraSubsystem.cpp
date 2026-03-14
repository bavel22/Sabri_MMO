// CameraSubsystem.cpp — Camera rotation and zoom applied directly in input handlers.
// No Tick/FTickableGameObject — rotation happens immediately when mouse moves.

#include "CameraSubsystem.h"
#include "MMOGameInstance.h"
#include "SabriMMOCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

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
	UE_LOG(LogCamera, Log, TEXT("CameraSubsystem started"));
}

void UCameraSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ============================================================
// Public API — called directly from character input handlers
// ============================================================

void UCameraSubsystem::OnRightClickStarted()
{
	bIsRotatingCamera = true;
	EnsureSpringArm();
}

void UCameraSubsystem::OnRightClickCompleted()
{
	bIsRotatingCamera = false;
}

void UCameraSubsystem::OnMouseDelta(const FVector2D& Delta)
{
	if (!bIsRotatingCamera) return;

	USpringArmComponent* SA = CachedSpringArm.Get();
	if (!SA) { EnsureSpringArm(); SA = CachedSpringArm.Get(); }
	if (!SA) return;

	// Apply yaw rotation immediately (no Tick delay)
	CurrentYaw += Delta.X * RotationSensitivity;
	SA->SetRelativeRotation(FRotator(FixedPitch, CurrentYaw, 0.f));
}

void UCameraSubsystem::OnZoom(float AxisValue)
{
	USpringArmComponent* SA = CachedSpringArm.Get();
	if (!SA) { EnsureSpringArm(); SA = CachedSpringArm.Get(); }
	if (!SA) return;

	float NewLength = SA->TargetArmLength - (AxisValue * ZoomSpeed);
	SA->TargetArmLength = FMath::Clamp(NewLength, MinArmLength, MaxArmLength);
}

// ============================================================
// Helpers
// ============================================================

void UCameraSubsystem::EnsureSpringArm()
{
	if (CachedSpringArm.IsValid()) return;

	USpringArmComponent* SA = FindSpringArm();
	if (!SA) return;

	CachedSpringArm = SA;
	FRotator Rot = SA->GetRelativeRotation();
	CurrentYaw = Rot.Yaw;
	SA->bUsePawnControlRotation = false;
	SA->bInheritPitch = false;
	SA->bInheritYaw = false;
	SA->bInheritRoll = false;
	SA->SetRelativeRotation(FRotator(FixedPitch, CurrentYaw, 0.f));
	UE_LOG(LogCamera, Log, TEXT("SpringArm configured (pitch=%.0f, yaw=%.0f, arm=%.0f)"),
		FixedPitch, CurrentYaw, SA->TargetArmLength);
}

USpringArmComponent* UCameraSubsystem::FindSpringArm() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return nullptr;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return nullptr;

	// BP_MMOCharacter has TWO SpringArms:
	//   1. C++ "CameraBoom" (TargetArmLength=400, NOT used for game camera)
	//   2. BP "SpringArm" (TargetArmLength=1200, the ACTUAL game camera)
	// We must find the BP one, not the C++ one.
	TArray<USpringArmComponent*> SpringArms;
	Pawn->GetComponents<USpringArmComponent>(SpringArms);

	// Prefer the BP SpringArm named "SpringArm" (the game camera)
	for (USpringArmComponent* SA : SpringArms)
	{
		if (SA->GetName() == TEXT("SpringArm"))
			return SA;
	}

	// Fallback: return the one with largest arm length (the actual camera)
	USpringArmComponent* Best = nullptr;
	float BestLength = 0.f;
	for (USpringArmComponent* SA : SpringArms)
	{
		if (SA->TargetArmLength > BestLength)
		{
			BestLength = SA->TargetArmLength;
			Best = SA;
		}
	}
	return Best ? Best : (SpringArms.Num() > 0 ? SpringArms[0] : nullptr);
}
