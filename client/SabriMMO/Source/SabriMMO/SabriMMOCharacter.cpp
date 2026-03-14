// Copyright Epic Games, Inc. All Rights Reserved.

#include "SabriMMOCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SabriMMO.h"
#include "MMOGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "UI/CombatStatsSubsystem.h"
#include "UI/InventorySubsystem.h"
#include "UI/EquipmentSubsystem.h"
#include "UI/HotbarSubsystem.h"
#include "UI/ShopSubsystem.h"
#include "ShopNPC.h"
#include "KafraNPC.h"
#include "UI/ZoneTransitionSubsystem.h"
#include "UI/PlayerInputSubsystem.h"
#include "UI/CameraSubsystem.h"
#include "InputModifiers.h"
#include "Framework/Application/SlateApplication.h"

ASabriMMOCharacter::ASabriMMOCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ASabriMMOCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Ensure Game+UI input mode so Slate widgets receive mouse events
	// and Enhanced Input keyboard actions (F5, F6, etc.) work properly.
	// BP_MMOCharacter Blueprint also sets this, but C++ ensures it as a safety net.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	// Immediately place pawn at the DB-loaded position during zone transitions / initial login.
	// BeginPlay runs before the first frame renders, so the player never sees the wrong position.
	// Ground-snap ensures the capsule sits on the walkable surface.
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetGameInstance()))
	{
		if (GI->bIsZoneTransitioning && !GI->PendingSpawnLocation.IsNearlyZero())
		{
			FVector Snapped = UZoneTransitionSubsystem::SnapLocationToGround(
				GetWorld(), GI->PendingSpawnLocation,
				GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			SetActorLocation(Snapped);
		}
	}
}

void ASabriMMOCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Look);

		// ---- Gameplay input (left-click, camera) — replaces BP event graph ----
		CreateGameplayActions();

		// ---- UI toggle keys (programmatic, no Blueprint asset needed) ----
		CreateUIToggleActions();

		// Register the UI toggle IMC with the Enhanced Input subsystem
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (UIToggleIMC)
				{
					Subsystem->AddMappingContext(UIToggleIMC, 1);
				}
			}
		}

		// Register gameplay IMC
		if (APlayerController* PC2 = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem2 = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC2->GetLocalPlayer()))
			{
				if (GameplayIMC)
				{
					Subsystem2->AddMappingContext(GameplayIMC, 2);
				}
			}
		}

		// Bind gameplay actions
		if (LeftClickAction)
		{
			EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::HandleLeftClick);
		}
		if (RightClickAction)
		{
			EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleRightClickStarted);
			EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Completed, this, &ASabriMMOCharacter::HandleRightClickCompleted);
		}
		if (MouseDeltaAction)
		{
			EnhancedInputComponent->BindAction(MouseDeltaAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::HandleMouseDelta);
		}
		if (ZoomAction)
		{
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::HandleZoom);
		}

		// Bind UI toggle actions
		if (ToggleCombatStatsAction)
		{
			EnhancedInputComponent->BindAction(ToggleCombatStatsAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleCombatStats);
		}
		if (ToggleInventoryAction)
		{
			EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleInventory);
		}
		if (ToggleEquipmentAction)
		{
			EnhancedInputComponent->BindAction(ToggleEquipmentAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleEquipment);
		}
		if (ToggleShopAction)
		{
			EnhancedInputComponent->BindAction(ToggleShopAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleShop);
		}
		if (CycleHotbarAction)
		{
			EnhancedInputComponent->BindAction(CycleHotbarAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleCycleHotbar);
		}

		// Bind hotbar slot keys 1-9
		// Each must be a separate named function for Enhanced Input binding
		typedef void (ASabriMMOCharacter::*SlotHandler)();
		static const SlotHandler SlotHandlers[] = {
			&ASabriMMOCharacter::HandleHotbarSlot1, &ASabriMMOCharacter::HandleHotbarSlot2,
			&ASabriMMOCharacter::HandleHotbarSlot3, &ASabriMMOCharacter::HandleHotbarSlot4,
			&ASabriMMOCharacter::HandleHotbarSlot5, &ASabriMMOCharacter::HandleHotbarSlot6,
			&ASabriMMOCharacter::HandleHotbarSlot7, &ASabriMMOCharacter::HandleHotbarSlot8,
			&ASabriMMOCharacter::HandleHotbarSlot9
		};
		for (int32 i = 0; i < 9; ++i)
		{
			if (HotbarSlotActions[i])
			{
				EnhancedInputComponent->BindAction(HotbarSlotActions[i], ETriggerEvent::Started, this, SlotHandlers[i]);
			}
		}
	}
	else
	{
		UE_LOG(LogSabriMMO, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

// ============================================================
// Programmatic UI toggle actions (Enhanced Input)
// ============================================================

// ============================================================
// Gameplay input actions (replaces BP event graph IA_Attack, IA_ClickToMove, camera)
// ============================================================

void ASabriMMOCharacter::CreateGameplayActions()
{
	GameplayIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_Gameplay"));

	// Left mouse button — click-to-move, click-to-attack, click-to-interact
	LeftClickAction = NewObject<UInputAction>(this, TEXT("IA_GameLeftClick"));
	LeftClickAction->ValueType = EInputActionValueType::Boolean;
	GameplayIMC->MapKey(LeftClickAction, EKeys::LeftMouseButton);

	// Right mouse button — camera rotation
	RightClickAction = NewObject<UInputAction>(this, TEXT("IA_GameRightClick"));
	RightClickAction->ValueType = EInputActionValueType::Boolean;
	GameplayIMC->MapKey(RightClickAction, EKeys::RightMouseButton);

	// Mouse delta — camera rotation amount
	MouseDeltaAction = NewObject<UInputAction>(this, TEXT("IA_GameMouseDelta"));
	MouseDeltaAction->ValueType = EInputActionValueType::Axis2D;
	GameplayIMC->MapKey(MouseDeltaAction, EKeys::Mouse2D);

	// Mouse wheel — camera zoom
	// Map both scroll directions explicitly for reliable firing
	ZoomAction = NewObject<UInputAction>(this, TEXT("IA_GameZoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;
	GameplayIMC->MapKey(ZoomAction, EKeys::MouseScrollUp);
	FEnhancedActionKeyMapping& ScrollDownMap = GameplayIMC->MapKey(ZoomAction, EKeys::MouseScrollDown);
	UInputModifierNegate* NegMod = NewObject<UInputModifierNegate>(this);
	ScrollDownMap.Modifiers.Add(NegMod);
}

void ASabriMMOCharacter::HandleLeftClick()
{
	if (UWorld* World = GetWorld())
	{
		if (UPlayerInputSubsystem* Sub = World->GetSubsystem<UPlayerInputSubsystem>())
		{
			Sub->OnLeftClickFromCharacter(this);
		}
	}
}

void ASabriMMOCharacter::HandleRightClickStarted()
{
	if (UWorld* World = GetWorld())
	{
		if (UCameraSubsystem* Sub = World->GetSubsystem<UCameraSubsystem>())
		{
			Sub->OnRightClickStarted();
		}
	}
}

void ASabriMMOCharacter::HandleRightClickCompleted()
{
	if (UWorld* World = GetWorld())
	{
		if (UCameraSubsystem* Sub = World->GetSubsystem<UCameraSubsystem>())
		{
			Sub->OnRightClickCompleted();
		}
	}
}

void ASabriMMOCharacter::HandleMouseDelta(const FInputActionValue& Value)
{
	if (UWorld* World = GetWorld())
	{
		if (UCameraSubsystem* Sub = World->GetSubsystem<UCameraSubsystem>())
		{
			Sub->OnMouseDelta(Value.Get<FVector2D>());
		}
	}
}

void ASabriMMOCharacter::HandleZoom(const FInputActionValue& Value)
{
	if (UWorld* World = GetWorld())
	{
		if (UCameraSubsystem* Sub = World->GetSubsystem<UCameraSubsystem>())
		{
			Sub->OnZoom(Value.Get<float>());
		}
	}
}

void ASabriMMOCharacter::CreateUIToggleActions()
{
	UIToggleIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_UIToggles"));

	// F8: Toggle Combat Stats
	ToggleCombatStatsAction = NewObject<UInputAction>(this, TEXT("IA_ToggleCombatStats"));
	ToggleCombatStatsAction->ValueType = EInputActionValueType::Boolean;
	UIToggleIMC->MapKey(ToggleCombatStatsAction, EKeys::F8);

	// F6: Toggle Inventory
	ToggleInventoryAction = NewObject<UInputAction>(this, TEXT("IA_ToggleInventory"));
	ToggleInventoryAction->ValueType = EInputActionValueType::Boolean;
	UIToggleIMC->MapKey(ToggleInventoryAction, EKeys::F6);

	// F7: Toggle Equipment
	ToggleEquipmentAction = NewObject<UInputAction>(this, TEXT("IA_ToggleEquipment"));
	ToggleEquipmentAction->ValueType = EInputActionValueType::Boolean;
	UIToggleIMC->MapKey(ToggleEquipmentAction, EKeys::F7);

	// F9: Toggle Shop
	ToggleShopAction = NewObject<UInputAction>(this, TEXT("IA_ToggleShop"));
	ToggleShopAction->ValueType = EInputActionValueType::Boolean;
	UIToggleIMC->MapKey(ToggleShopAction, EKeys::F9);

	// H: Cycle Hotbar visibility
	CycleHotbarAction = NewObject<UInputAction>(this, TEXT("IA_CycleHotbar"));
	CycleHotbarAction->ValueType = EInputActionValueType::Boolean;
	UIToggleIMC->MapKey(CycleHotbarAction, EKeys::H);

	// Keys 1-9: Hotbar slot activation
	static const FKey NumberKeys[] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
	};
	for (int32 i = 0; i < 9; ++i)
	{
		FString ActionName = FString::Printf(TEXT("IA_HotbarSlot%d"), i + 1);
		HotbarSlotActions[i] = NewObject<UInputAction>(this, *ActionName);
		HotbarSlotActions[i]->ValueType = EInputActionValueType::Boolean;
		UIToggleIMC->MapKey(HotbarSlotActions[i], NumberKeys[i]);
	}
}

void ASabriMMOCharacter::HandleToggleCombatStats()
{
	if (UWorld* World = GetWorld())
	{
		if (UCombatStatsSubsystem* Sub = World->GetSubsystem<UCombatStatsSubsystem>())
		{
			Sub->ToggleWidget();
		}
	}
}

void ASabriMMOCharacter::HandleToggleInventory()
{
	if (UWorld* World = GetWorld())
	{
		if (UInventorySubsystem* Sub = World->GetSubsystem<UInventorySubsystem>())
		{
			Sub->ToggleWidget();
		}
	}
}

void ASabriMMOCharacter::HandleToggleEquipment()
{
	if (UWorld* World = GetWorld())
	{
		if (UEquipmentSubsystem* Sub = World->GetSubsystem<UEquipmentSubsystem>())
		{
			Sub->ToggleWidget();
		}
	}
}

void ASabriMMOCharacter::HandleToggleShop()
{
	// F9: Close shop if open (like Escape in RO Classic).
	// Opening shops is done by clicking NPC actors in the world.
	if (UWorld* World = GetWorld())
	{
		if (UShopSubsystem* Sub = World->GetSubsystem<UShopSubsystem>())
		{
			if (Sub->IsWidgetVisible())
			{
				Sub->CloseShop();
			}
		}
	}
}

void ASabriMMOCharacter::HandleCycleHotbar()
{
	if (UWorld* World = GetWorld())
	{
		if (UHotbarSubsystem* Sub = World->GetSubsystem<UHotbarSubsystem>())
		{
			Sub->CycleVisibility();
		}
	}
}

void ASabriMMOCharacter::HandleHotbarSlot1() { HandleHotbarSlotInternal(1); }
void ASabriMMOCharacter::HandleHotbarSlot2() { HandleHotbarSlotInternal(2); }
void ASabriMMOCharacter::HandleHotbarSlot3() { HandleHotbarSlotInternal(3); }
void ASabriMMOCharacter::HandleHotbarSlot4() { HandleHotbarSlotInternal(4); }
void ASabriMMOCharacter::HandleHotbarSlot5() { HandleHotbarSlotInternal(5); }
void ASabriMMOCharacter::HandleHotbarSlot6() { HandleHotbarSlotInternal(6); }
void ASabriMMOCharacter::HandleHotbarSlot7() { HandleHotbarSlotInternal(7); }
void ASabriMMOCharacter::HandleHotbarSlot8() { HandleHotbarSlotInternal(8); }
void ASabriMMOCharacter::HandleHotbarSlot9() { HandleHotbarSlotInternal(9); }

void ASabriMMOCharacter::HandleHotbarSlotInternal(int32 KeyNumber)
{
	if (UWorld* World = GetWorld())
	{
		if (UHotbarSubsystem* Sub = World->GetSubsystem<UHotbarSubsystem>())
		{
			// Detect modifier keys at the time of the key press
			bool bAlt = FSlateApplication::Get().GetModifierKeys().IsAltDown();
			bool bCtrl = FSlateApplication::Get().GetModifierKeys().IsControlDown();
			bool bShift = FSlateApplication::Get().GetModifierKeys().IsShiftDown();

			Sub->HandleNumberKey(KeyNumber, bAlt, bCtrl, bShift);
		}
	}
}

bool ASabriMMOCharacter::TryInteractWithNPC(AActor* HitActor)
{
	if (!HitActor) return false;

	if (AShopNPC* ShopNPC = Cast<AShopNPC>(HitActor))
	{
		ShopNPC->Interact();
		return true;
	}

	if (AKafraNPC* KafraNPC = Cast<AKafraNPC>(HitActor))
	{
		KafraNPC->Interact();
		return true;
	}

	return false;
}

void ASabriMMOCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ASabriMMOCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ASabriMMOCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ASabriMMOCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ASabriMMOCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ASabriMMOCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
