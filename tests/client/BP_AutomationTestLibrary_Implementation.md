# BP_AutomationTestLibrary - Complete Implementation Guide

## Overview
This document provides the exact, step-by-step implementation of the UI testing Blueprint library following Sabri_MMO project standards.

## Step 1: Create the Blueprint Library

### 1.1 Create Blueprint
1. **Open UE5 Editor** with SabriMMO project
2. **Content Browser** → Right-click → **Blueprint Class**
3. **Select "Blueprint Function Library"**
4. **Name**: `BP_AutomationTestLibrary`
5. **Save location**: `/Game/SabriMMO/Tests/`
6. **Save** the Blueprint

### 1.2 Verify Creation
- Confirm Blueprint appears in `/Game/SabriMMO/Tests/`
- Open to verify it's a Function Library class
- Should have no components, only Functions panel

## Step 2: Add Test Functions

### 2.1 Test_Shop_OpenWindow Function

#### Function Setup:
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Shop_OpenWindow`
3. **Check "Static"** (CRITICAL - must be static for automation)
4. **Return type**: `Boolean`
5. **Add input parameters** (none needed)

#### Node Implementation (exact node sequence):

```
Function Entry: Test_Shop_OpenWindow
    ↓
[Get Player Controller] (index 0)
    ↓
[Cast To BP_MMOCharacter] → As BP MMO Character
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Player Controller invalid"] → [Return False]
    ↓
[Get HUD Manager] (from BP MMO Character)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: HUD Manager invalid"] → [Return False]
    ↓
[Format Text] "Test Weapon Shop" → ShopName
[Format Text] "5000" → PlayerZuzucoin
[Make Array] → Add shop items (see below)
[Make JSON Object] → {shopName: ShopName, playerZuzucoin: PlayerZuzucoin, items: ItemArray}
[Conv String to Object] → ShopDataObject
[Conv Object to String] → ShopDataString
    ↓
[Call OpenShop] (on HUD Manager) → Target: ShopDataString
    ↓
[Delay] 0.1 seconds
    ↓
[Get Shop Window Ref] (from HUD Manager)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Shop Window Ref invalid"] → [Return False]
    ↓
[Is Visible] (on Shop Window Ref)
    ↓
[Branch] Is Visible?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Shop window not visible"] → [Return False]
    ↓
[Get Shop Name Text] (from Shop Window Ref)
    ↓
[Get Text] (from Shop Name Text)
    ↓
[Equal] (String) → Compare with "Test Weapon Shop"
    ↓
[Branch] Equal?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Shop name incorrect"] → [Return False]
    ↓
[Get Zuzucoin Text] (from Shop Window Ref)
    ↓
[Get Text] (from Zuzucoin Text)
    ↓
[Equal] (String) → Compare with "5000 Z"
    ↓
[Branch] Equal?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Zuzucoin display incorrect"] → [Return False]
    ↓
[Get Shop Items List] (from Shop Window Ref)
    ↓
[Get Children Count] (from Shop Items List)
    ↓
[Greater] (Int) → Children Count > 0
    ↓
[Branch] Greater?
    ├─ TRUE → [Print String "SUCCESS: Shop window opened correctly"] → [Return True]
    └─ FALSE → [Print String "ERROR: No items in shop"] → [Return False]
```

#### Shop Items Array Creation:
```
[Make Array] → ItemArray
    ↓
[Add Item 1]:
    [Make JSON Object] → {
        item_id: 1,
        name: "Iron Sword",
        price: 1000,
        description: "Basic sword for beginners",
        item_type: "weapon",
        weapon_type: "sword",
        atk: 25,
        required_level: 5
    }
    ↓
[Add Item 2]:
    [Make JSON Object] → {
        item_id: 2,
        name: "Health Potion",
        price: 50,
        description: "Restores 100 HP",
        item_type: "consumable",
        quantity: 1,
        required_level: 1
    }
    ↓
[Add Item 3]:
    [Make JSON Object] → {
        item_id: 3,
        name: "Iron Armor",
        price: 1500,
        description: "Basic armor for warriors",
        item_type: "armor",
        def: 15,
        required_level: 8
    }
```

### 2.2 Test_Inventory_Toggle Function

#### Function Setup:
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Inventory_Toggle`
3. **Check "Static"**
4. **Return type**: `Boolean`

#### Node Implementation:
```
Function Entry: Test_Inventory_Toggle
    ↓
[Get Player Controller] (index 0)
    ↓
[Cast To BP_MMOCharacter] → As BP MMO Character
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Player Controller invalid"] → [Return False]
    ↓
[Get HUD Manager] (from BP MMO Character)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: HUD Manager invalid"] → [Return False]
    ↓
[Get bIsInventoryOpen] (from HUD Manager) → InitialState
    ↓
[Call ToggleInventory] (on HUD Manager)
    ↓
[Delay] 0.1 seconds
    ↓
[Get Inventory Window Ref] (from HUD Manager)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Inventory Window Ref invalid"] → [Return False]
    ↓
[Is Visible] (on Inventory Window Ref)
    ↓
[Branch] Is Visible?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Inventory window not visible after toggle"] → [Return False]
    ↓
[Get bIsInventoryOpen] (from HUD Manager) → ToggledState
    ↓
[Not Equal] (Boolean) → InitialState != ToggledState
    ↓
[Branch] Not Equal?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: bIsInventoryOpen flag not updated"] → [Return False]
    ↓
[Call ToggleInventory] (on HUD Manager) → Close it
    ↓
[Delay] 0.1 seconds
    ↓
[Is Visible] (on Inventory Window Ref)
    ↓
[Branch] Is Visible?
    ├─ FALSE → [Print String "SUCCESS: Inventory toggle works correctly"] → [Return True]
    └─ TRUE → [Print String "ERROR: Inventory window still visible after second toggle"] → [Return False]
```

### 2.3 Test_Widget_TextDisplay Function

#### Function Setup:
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Widget_TextDisplay`
3. **Check "Static"**
4. **Return type**: `Boolean`
5. **Input parameters**:
   - `WidgetName` (String)
   - `ExpectedText` (String)

#### Node Implementation:
```
Function Entry: Test_Widget_TextDisplay (WidgetName, ExpectedText)
    ↓
[Get All Widgets of Class] → Widget Class: UserWidget
    ↓
[For Each] WidgetArray → LoopWidget
    ↓
[Get Display Name] (from LoopWidget)
    ↓
[Equal] (String) → Compare with WidgetName
    ↓
[Branch] Equal?
    ├─ TRUE → FoundWidget = LoopWidget → Break
    └─ FALSE → Continue Loop
    ↓
[Branch] Is Valid FoundWidget?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Widget not found"] → [Return False]
    ↓
[Get All Children] (from FoundWidget)
    ↓
[For Each] ChildrenArray → LoopChild
    ↓
[Is A] → Cast To Text Block
    ↓
[Branch] Cast Succeeded?
    ├─ TRUE → TextBlock = Cast Result → Break
    └─ FALSE → Continue Loop
    ↓
[Branch] Is Valid TextBlock?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: TextBlock not found"] → [Return False]
    ↓
[Get Text] (from TextBlock)
    ↓
[Equal] (String) → Compare with ExpectedText
    ↓
[Branch] Equal?
    ├─ TRUE → [Print String "SUCCESS: Text display correct"] → [Return True]
    └─ FALSE → [Print String "ERROR: Text mismatch"] → [Return False]
```

### 2.4 Test_Zuzucoin_Update Function

#### Function Setup:
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Zuzucoin_Update`
3. **Check "Static"**
4. **Return type**: `Boolean`

#### Node Implementation:
```
Function Entry: Test_Zuzucoin_Update
    ↓
[Get Player Controller] (index 0)
    ↓
[Cast To BP_MMOCharacter] → As BP MMO Character
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: Player Controller invalid"] → [Return False]
    ↓
[Get HUD Manager] (from BP MMO Character)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: HUD Manager invalid"] → [Return False]
    ↓
[Get Player Zuzucoin] (from HUD Manager) → InitialZuzucoin
    ↓
[Set Player Zuzucoin] (on HUD Manager) → NewZuzucoin = 4000
    ↓
[Call UpdateZuzucoinEverywhere] (on HUD Manager) → NewZuzucoin
    ↓
[Delay] 0.1 seconds
    ↓
[Get Shop Window Ref] (from HUD Manager)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Check Shop Display
    └─ FALSE → Skip Shop Check
    ↓
[Check Shop Display]:
    [Get Zuzucoin Text] (from Shop Window Ref)
    ↓
[Get Text] (from Zuzucoin Text)
    ↓
[Equal] (String) → Compare with "4000 Z"
    ↓
[Branch] Equal?
    ├─ TRUE → ShopCorrect = True
    └─ FALSE → ShopCorrect = False
    ↓
[Get Inventory Window Ref] (from HUD Manager)
    ↓
[Branch] Is Valid?
    ├─ TRUE → Check Inventory Display
    └─ FALSE → Skip Inventory Check
    ↓
[Check Inventory Display]:
    [Get Zuzucoin Text] (from Inventory Window Ref)
    ↓
[Get Text] (from Zuzucoin Text)
    ↓
[Equal] (String) → Compare with "4000 Z"
    ↓
[Branch] Equal?
    ├─ TRUE → InventoryCorrect = True
    └─ FALSE → InventoryCorrect = False
    ↓
[Branch] (Shop Window Valid OR Inventory Window Valid)?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "ERROR: No UI windows open"] → [Return False]
    ↓
[Branch] (ShopCorrect OR InventoryCorrect)?
    ├─ TRUE → [Print String "SUCCESS: Zuzucoin display updated correctly"] → [Return True]
    └─ FALSE → [Print String "ERROR: Zuzucoin display not updated"] → [Return False]
```

## Step 3: Create Helper Functions

### 3.1 TakeScreenshot Function

#### Function Setup:
1. **Name**: `TakeScreenshot`
2. **Check "Static"**
3. **Return type**: `String`
4. **Input parameters**:
   - `TestName` (String)

#### Node Implementation:
```
Function Entry: TakeScreenshot (TestName)
    ↓
[Format Text] "Tests/Screenshots/{0}" → TestName → FilePath
    ↓
[Get Game Viewport Client]
    ↓
[Get Viewport]
    ↓
[Take Screenshot] → File Path: FilePath
    ↓
[Return Value] → FilePath
```

### 3.2 LogTestResult Function

#### Function Setup:
1. **Name**: `LogTestResult`
2. **Check "Static"**
3. **Return type**: `None`
4. **Input parameters**:
   - `TestName` (String)
   - `Success` (Boolean)
   - `Message` (String, default "")

#### Node Implementation:
```
Function Entry: LogTestResult (TestName, Success, Message = "")
    ↓
[Branch] Success?
    ├─ TRUE → [Format Text] "✅ PASS: {0}" → TestName → LogMessage
    └─ FALSE → [Format Text] "❌ FAIL: {0} - {1}" → TestName, Message → LogMessage
    ↓
[Print String] → LogMessage
```

## Step 4: Create Integration Test Functions

### 4.1 Test_Complete_Shop_Flow

#### Function Setup:
1. **Name**: `Test_Complete_Shop_Flow`
2. **Check "Static"**
3. **Return type**: `Boolean`

#### Node Implementation:
```
Function Entry: Test_Complete_Shop_Flow
    ↓
[Call Test_Shop_OpenWindow] → ShopOpenResult
    ↓
[Branch] ShopOpenResult?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "FAIL: Shop window failed to open"] → [Return False]
    ↓
[Call Test_Widget_TextDisplay] → WidgetName: "ShopNameText", ExpectedText: "Test Weapon Shop" → NameResult
    ↓
[Branch] NameResult?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "FAIL: Shop name incorrect"] → [Return False]
    ↓
[Call Test_Widget_TextDisplay] → WidgetName: "ZuzucoinText", ExpectedText: "5000 Z" → ZuzucoinResult
    ↓
[Branch] ZuzucoinResult?
    ├─ TRUE → Continue
    └─ FALSE → [Print String "FAIL: Zuzucoin display incorrect"] → [Return False]
    ↓
[Call Test_Zuzucoin_Update] → UpdateResult
    ↓
[Branch] UpdateResult?
    ├─ TRUE → [Print String "SUCCESS: Complete shop flow works correctly"] → [Return True]
    └─ FALSE → [Print String "FAIL: Zuzucoin update failed"] → [Return False]
```

## Step 5: Compile and Save

### 5.1 Compile Blueprint
1. **Click "Compile"** in Blueprint editor
2. **Fix any compilation errors**
3. **Save** the Blueprint

### 5.2 Verify Functions
1. **Functions panel** should show all test functions
2. Each function should have "Static" icon
3. Return types should be Boolean (except helpers)

## Step 6: Create Test Map

### 6.1 Create Map
1. **File → New Level**
2. **Select "Empty Level"**
3. **Name**: `AutomationTestMap`
4. **Save** in `/Game/SabriMMO/Maps/`

### 6.2 Setup Test Environment
1. **Place "Player Start"** actor
2. **Place "BP_NPCShop"** actor (from your Blueprints)
3. **Set Shop ID** on NPC to 999 (test shop)
4. **Add basic lighting** (Directional Light)
5. **Add floor** (Brush or plane)

### 6.3 Configure Map Settings
1. **World Settings** → **Game Mode**
2. **Default Game Mode**: `GameMode` (your existing game mode)
3. **Player Controller Class**: `BP_PlayerController`
4. **HUD Class**: `BP_GameHUD`

## Step 7: Configure Automation System

### 7.1 Project Settings
1. **Edit → Project Settings**
2. **Engine → Automation**
3. **Enable Automation System**: ✅
4. **Automation Test Name Filter**: `SabriMMO.UI`
5. **Automation Test Class Filter**: `BlueprintFunctionLibrary`
6. **Run Automation Tests on Startup**: ❌ (optional)

### 7.2 Console Variables
Add to `DefaultEngine.ini`:
```ini
[/Script/Engine.AutomationTestManager]
DefaultAutomationTestNameFilter=SabriMMO.UI
DefaultAutomationTestClassFilter=BlueprintFunctionLibrary
```

## Step 8: Test Execution

### 8.1 In Editor Testing
1. **Open AutomationTestMap**
2. **Press "Play"**
3. **Window → Developer Tools → Session Frontend**
4. **Go to "Automation" tab**
5. **Filter tests** by typing "SabriMMO"
6. **Select tests** to run
7. **Click "Run Tests"**

### 8.2 Command Line Testing
```bash
# Run all UI tests
SabriMMO.exe AutomationTestMap -ExecCmds="Automation RunTests=UI"

# Run specific test
SabriMMO.exe -ExecCmds="Automation RunTests=Test_Shop_OpenWindow"

# Run tests and exit
SabriMMO.exe -ExecCmds="Automation RunTests=UI,Quit"
```

## Step 9: Verify Results

### 9.1 Expected Output
```
Log: ✅ PASS: Test_Shop_OpenWindow
Log: ✅ PASS: Test_Inventory_Toggle  
Log: ✅ PASS: Test_Widget_TextDisplay
Log: ✅ PASS: Test_Zuzucoin_Update
Log: ✅ PASS: Test_Complete_Shop_Flow
```

### 9.2 Troubleshooting
- **Widget not found**: Check widget names match exactly
- **HUD Manager null**: Ensure player controller is valid
- **Shop window null**: Call OpenShop before checking window
- **Text mismatch**: Check formatting (spaces, " Z" suffix)

## Step 10: Integration with Server Tests

### 10.1 Server Setup
1. **Run database setup**: `psql -d sabri_mmo -f database/test_setup.sql`
2. **Start server**: `npm start` in server directory
3. **Verify test endpoints**: `http://localhost:3001/test/setup`

### 10.2 Combined Testing
```javascript
// Run server tests first
node tests/unit/combat.test.js

// Then run UI tests
SabriMMO.exe -ExecCmds="Automation RunTests=UI"
```

## Naming Conventions and Standards

### Function Names
- Prefix: `Test_`
- PascalCase: `Test_Shop_OpenWindow`
- Descriptive: What is being tested

### Variable Names
- camelCase for local variables
- Descriptive: `shopWindowRef`, `initialZuzucoin`
- No abbreviations: `widgetName` not `wgtNm`

### Error Messages
- Prefix: `"ERROR: "`
- Descriptive: `"ERROR: Shop window not visible"`
- Include context: `"ERROR: Widget not found: ShopNameText"`

### Success Messages
- Prefix: `"SUCCESS: "`
- Clear: `"SUCCESS: Shop window opened correctly"`

### Comments
- Add comment boxes for complex logic
- Group related nodes
- Explain non-obvious operations

This implementation follows all Sabri_MMO project standards and provides comprehensive UI testing capability!
