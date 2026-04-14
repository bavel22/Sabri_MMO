#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GroundItemActor.generated.h"

class UBoxComponent;
class UWidgetComponent;

/**
 * World actor representing an item on the ground.
 * Two screen-space UWidgetComponents: icon + name label (hover-only).
 * BoxComponent provides click/hover detection via ECC_Visibility.
 */
UCLASS()
class SABRIMMO_API AGroundItemActor : public AActor
{
	GENERATED_BODY()

public:
	AGroundItemActor();
	virtual void BeginPlay() override;

	void InitGroundItem(int32 InGroundItemId, const FString& InItemName, const FString& InIcon,
	                     const FString& InTierColor, int32 InQuantity);

	void PlayDropArc(const FVector& SourcePos, const FVector& FinalPos, float Duration = 0.4f);
	void SnapToGround();
	void FadeOutAndDestroy(float Duration = 0.5f);

	int32 GetGroundItemId() const { return GroundItemId; }
	void SetClickable(bool bClickable);
	void SetNameVisible(bool bVisible);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere) UBoxComponent* ClickBox = nullptr;
	UPROPERTY(VisibleAnywhere) UWidgetComponent* IconWidget = nullptr;
	UPROPERTY(VisibleAnywhere) UWidgetComponent* NameWidget = nullptr;
	UPROPERTY() UTexture2D* IconTexture = nullptr;

	TSharedPtr<FSlateBrush> IconBrush;

	int32 GroundItemId = 0;

	// Drop arc animation
	bool bAnimatingArc = false;
	FVector ArcStart = FVector::ZeroVector;
	FVector ArcEnd = FVector::ZeroVector;
	float ArcDuration = 0.4f;
	float ArcElapsed = 0.f;
	float ArcHeight = 80.f;

	// Fade-out
	bool bFadingOut = false;
	float FadeDuration = 0.5f;
	float FadeElapsed = 0.f;
};
