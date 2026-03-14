// CameraSubsystem.h — RO-style camera control.
// Right-click hold rotates camera yaw, mouse wheel zooms.
// Fixed pitch angle (-55°). No Tick needed — rotation applied directly in handlers.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CameraSubsystem.generated.h"

class USpringArmComponent;

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
	void OnRightClickStarted();
	void OnRightClickCompleted();
	void OnMouseDelta(const FVector2D& Delta);
	void OnZoom(float AxisValue);

private:
	bool bIsRotatingCamera = false;
	float CurrentYaw = 0.f;
	float ZoomSpeed = 80.f;
	float MinArmLength = 200.f;
	float MaxArmLength = 1500.f;
	float FixedPitch = -55.f;
	float RotationSensitivity = 0.6f;

	TWeakObjectPtr<USpringArmComponent> CachedSpringArm;

	USpringArmComponent* FindSpringArm() const;
	void EnsureSpringArm();
};
