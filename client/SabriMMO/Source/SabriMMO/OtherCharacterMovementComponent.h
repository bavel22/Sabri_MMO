// OtherCharacterMovementComponent.h — Movement component for remote player characters.
// Extends CharacterMovementComponent with a per-tick floor snap so remote players
// always sit on the ground even when their Blueprint movement uses flat (XY-only) direction.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "OtherCharacterMovementComponent.generated.h"

UCLASS()
class SABRIMMO_API UOtherCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
};
