// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SabriMMOCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UHotbarSubsystem;
class ASpriteCharacterActor;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ASabriMMOCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	ASabriMMOCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

private:

	// ---- Programmatic UI toggle input (Enhanced Input) ----
	// Add new UI toggle keys here. Create the action in CreateUIToggleActions(),
	// map it to a key, and bind in SetupPlayerInputComponent().

	UPROPERTY()
	UInputMappingContext* UIToggleIMC = nullptr;

	UPROPERTY()
	UInputAction* ToggleCombatStatsAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleInventoryAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleEquipmentAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleShopAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleCartAction = nullptr;

	UPROPERTY()
	UInputAction* FocusChatAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleSitAction = nullptr;

	UPROPERTY()
	UInputAction* TogglePartyAction = nullptr;

	UPROPERTY()
	UInputAction* CycleHotbarAction = nullptr;

	UPROPERTY()
	UInputAction* CycleMinimapAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleWorldMapAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleZoneNamesAction = nullptr;

	UPROPERTY()
	UInputAction* ToggleEscapeMenuAction = nullptr;

	UPROPERTY()
	UInputAction* HotbarSlotActions[9];

	void CreateUIToggleActions();
	void CreateGameplayActions();

	// ---- Gameplay input (left-click, camera) ----
	UPROPERTY()
	UInputMappingContext* GameplayIMC = nullptr;

	UPROPERTY()
	UInputAction* LeftClickAction = nullptr;

	UPROPERTY()
	UInputAction* RightClickAction = nullptr;

	UPROPERTY()
	UInputAction* MouseDeltaAction = nullptr;

	UPROPERTY()
	UInputAction* ZoomAction = nullptr;

	void HandleLeftClick();
	void HandleRightClickStarted();
	void HandleRightClickCompleted();
	void HandleMouseDelta(const FInputActionValue& Value);
	void HandleZoom(const FInputActionValue& Value);

	void HandleFocusChat();
	void HandleToggleSit();
	void HandleToggleCombatStats();
	void HandleToggleInventory();
	void HandleToggleEquipment();
	void HandleToggleShop();
	void HandleToggleCart();
	void HandleToggleParty();
	void HandleCycleHotbar();
	void HandleCycleMinimap();
	void HandleToggleWorldMap();
	void HandleToggleZoneNames();
	void HandleToggleEscapeMenu();
	void HandleHotbarSlot1();
	void HandleHotbarSlot2();
	void HandleHotbarSlot3();
	void HandleHotbarSlot4();
	void HandleHotbarSlot5();
	void HandleHotbarSlot6();
	void HandleHotbarSlot7();
	void HandleHotbarSlot8();
	void HandleHotbarSlot9();
	void HandleHotbarSlotInternal(int32 KeyNumber);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Try to interact with a clicked actor (e.g. AShopNPC). Returns true if handled. */
	UFUNCTION(BlueprintCallable, Category="NPC Interaction")
	bool TryInteractWithNPC(AActor* HitActor);

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** The local player's sprite actor (for UI position tracking) */
	UPROPERTY()
	TWeakObjectPtr<ASpriteCharacterActor> LocalSprite;
};

