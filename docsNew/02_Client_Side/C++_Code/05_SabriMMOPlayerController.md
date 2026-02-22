# ASabriMMOPlayerController

**Files**: `Source/SabriMMO/SabriMMOPlayerController.h` (53 lines), `SabriMMOPlayerController.cpp` (68 lines)  
**Parent**: `APlayerController`  
**UCLASS**: `abstract`  
**Purpose**: Manages input mapping contexts and optional mobile touch controls.

## Properties

| Property | Type | Category | Description |
|----------|------|----------|-------------|
| `DefaultMappingContexts` | TArray\<UInputMappingContext*\> | Input\|Input Mappings | IMCs added for all players |
| `MobileExcludedMappingContexts` | TArray\<UInputMappingContext*\> | Input\|Input Mappings | IMCs excluded on mobile |
| `MobileControlsWidgetClass` | TSubclassOf\<UUserWidget\> | Input\|Touch Controls | Widget class for mobile UI |
| `MobileControlsWidget` | TObjectPtr\<UUserWidget\> | — | Spawned mobile controls |
| `bForceTouchControls` | bool | Input\|Touch Controls, Config | Force mobile controls on desktop |

## Functions

### BeginPlay
1. If `ShouldUseTouchControls()` and `IsLocalPlayerController()`:
   - Creates `MobileControlsWidget` from `MobileControlsWidgetClass`
   - Adds to player screen at Z-order 0

### SetupInputComponent
1. For local player controllers:
   - Gets `UEnhancedInputLocalPlayerSubsystem`
   - Adds all `DefaultMappingContexts` at priority 0
   - If NOT using touch controls: also adds `MobileExcludedMappingContexts`

### ShouldUseTouchControls
```cpp
return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
```

---

**Last Updated**: 2026-02-17
