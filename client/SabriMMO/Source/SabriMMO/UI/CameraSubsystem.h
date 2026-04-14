// CameraSubsystem.h — RO Classic camera control (fully C++ owned).
// Creates its own SpringArm + Camera on the pawn. No BP dependency.
// Right-click hold rotates yaw, mouse wheel zooms, fixed -55° pitch.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CameraSubsystem.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPrimitiveComponent;

UCLASS()
class SABRIMMO_API UCameraSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// --- Public API (called by ASabriMMOCharacter input handlers) ---
	bool IsRotatingCamera() const { return bIsRotatingCamera; }
	bool DidRotateThisClick() const { return bDidRotateThisClick; }
	void OnRightClickStarted();
	void OnRightClickCompleted();
	void OnMouseDelta(const FVector2D& Delta);
	void OnZoom(float AxisValue);

	// Configurable via Options menu (set by OptionsSubsystem)
	float ZoomSpeed = 80.f;
	float RotationSensitivity = 0.6f;

private:
	bool bIsRotatingCamera = false;
	bool bDidRotateThisClick = false;
	float CurrentYaw = 0.f;

	// RO Classic camera defaults
	static constexpr float DefaultArmLength = 1200.f;
	static constexpr float MinArmLength = 200.f;
	static constexpr float MaxArmLength = 1500.f;
	static constexpr float FixedPitch = -55.f;

	TWeakObjectPtr<USpringArmComponent> CachedSpringArm;

	void SetupCamera();
	FTimerHandle SetupRetryTimer;
};
