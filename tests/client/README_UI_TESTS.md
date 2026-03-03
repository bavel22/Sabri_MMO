# UE5 UI Testing Setup for Sabri_MMO

## Overview
This guide shows how to set up automated UI testing using UE5's built-in Automation System.

## Testing Strategy

### 1. Functional Tests
- Widget visibility and state
- Button interactions
- Data display verification
- Socket.io event integration

### 2. Visual Tests  
- Screenshot comparison
- Layout verification
- Animation state checks

### 3. Integration Tests
- Complete user flows
- Multi-widget interactions
- Server-client communication

## Setup Steps

### Step 1: Create Test Blueprint
1. Create Blueprint Class: `BP_AutomationTests`
2. Parent: `BlueprintFunctionLibrary`
3. Add test functions as static methods

### Step 2: Create Test Maps
1. Create test map: `AutomationTestMap`
2. Add test environment (shop NPC, test character)
3. Set as default test map

### Step 3: Configure Automation
1. Project Settings → Engine → Automation
2. Enable "Run Automation Tests on Startup"
3. Configure test filters

## Test Categories

### UI Widget Tests
- Window open/close
- Button click responses
- Text display updates
- List population

### Gameplay Flow Tests
- Complete shop transaction
- Inventory management
- Character stat allocation
- Combat interactions

### Integration Tests
- Socket.io event handling
- Server response processing
- Data synchronization

## Running Tests

### In Editor
```
Window → Developer Tools → Session Frontend → Automation Tab
```

### Command Line
```
SabriMMO.exe -ExecCmds="Automation RunTests"
```

### Continuous Integration
```
SabriMMO.exe -ExecCmds="Automation RunTests=All,Quit"
```

## Test Structure

### Test Naming Convention
```
UI_Shop_OpenWindow
UI_Shop_BuyItem
UI_Inventory_EquipWeapon
UI_Combat_AttackEnemy
```

### Test Pattern
```cpp
// In Blueprint Function Library
IMPLEMENT_AUTOMATION_TEST(FShopOpenTest, "SabriMMO.UI.Shop.OpenWindow")

bool FShopOpenTest::RunTest(const FString& Parameters)
{
    // Test implementation
    return true;
}
```

## Expected Coverage

### Current Coverage: 70%
- Server logic ✅
- Data flow ✅
- Backend validation ✅

### Target Coverage: 95%
- UI widget state ✅
- User interactions ✅
- Visual verification ✅
- Integration flows ✅

## Benefits

- Prevent UI regression bugs
- Verify user experience
- Test complete player journeys
- Automated regression testing
- CI/CD integration
