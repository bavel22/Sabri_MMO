# Complete UI Testing Setup Guide

## Overview
This guide provides the complete, step-by-step setup for automated UI testing in Sabri_MMO. Everything that can be automated has been prepared for you.

## ✅ What's Already Done

### Server-Side Setup (COMPLETED)
- ✅ Test mode routes added to `server/src/index.js`
- ✅ Mock data endpoints at `/test/*`
- ✅ Database setup script created
- ✅ Test character and inventory data prepared

### Test Infrastructure (COMPLETED)
- ✅ Complete Blueprint implementation guide
- ✅ Node-by-node instructions
- ✅ Test map setup instructions
- ✅ Automation system configuration

## 🛠️ Manual Setup Required (30-45 minutes)

### Step 1: Database Setup (5 minutes)

#### 1.1 Run Test Setup Script
```bash
# In your terminal, navigate to database directory
cd C:/Sabri_MMO/database

# Run the test setup script
psql -d sabri_mmo -f test_setup.sql
```

#### 1.2 Verify Test Data
```sql
-- Check test character exists
SELECT character_id, name, class, level, zuzucoin 
FROM characters 
WHERE name = 'TestPlayer_UI';

-- Check test inventory
SELECT ci.inventory_id, it.name, ci.quantity, ci.is_equipped
FROM character_inventory ci
JOIN items it ON ci.item_id = it.item_id
WHERE ci.character_id = (SELECT character_id FROM characters WHERE name = 'TestPlayer_UI');
```

**Expected Results:**
- Test character with character_id, name "TestPlayer_UI", 5000 zuzucoin
- 3 inventory items (Iron Cleaver, 5x Crimson Vial, Linen Tunic equipped)

### Step 2: UE5 Blueprint Creation (20 minutes)

#### 2.1 Create BP_AutomationTestLibrary
1. **Open UE5 Editor** with SabriMMO project
2. **Content Browser** → Navigate to `/Game/SabriMMO/`
3. **Right-click** → **New Folder** → Name: `Tests`
4. **Open Tests folder** → **Right-click** → **Blueprint Class**
5. **Select "Blueprint Function Library"**
6. **Name**: `BP_AutomationTestLibrary`
7. **Save** the Blueprint

#### 2.2 Add Test Functions

**Function 1: Test_Shop_OpenWindow**
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Shop_OpenWindow`
3. ✅ **Check "Static"** (CRITICAL)
4. **Return type**: `Boolean`
5. **Implement nodes** following `BP_AutomationTestLibrary_Implementation.md`

**Function 2: Test_Inventory_Toggle**
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Inventory_Toggle`
3. ✅ **Check "Static"**
4. **Return type**: `Boolean`
5. **Implement nodes** following guide

**Function 3: Test_Widget_TextDisplay**
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Widget_TextDisplay`
3. ✅ **Check "Static"**
4. **Return type**: `Boolean`
5. **Input parameters**:
   - `WidgetName` (String)
   - `ExpectedText` (String)
6. **Implement nodes** following guide

**Function 4: Test_Zuzucoin_Update**
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Zuzucoin_Update`
3. ✅ **Check "Static"**
4. **Return type**: `Boolean`
5. **Implement nodes** following guide

**Function 5: Test_Complete_Shop_Flow**
1. **Functions panel** → **+** → **Add Function**
2. **Name**: `Test_Complete_Shop_Flow`
3. ✅ **Check "Static"**
4. **Return type**: `Boolean`
5. **Implement nodes** following guide

#### 2.3 Helper Functions

**Function 6: LogTestResult**
1. **Name**: `LogTestResult`
2. ✅ **Check "Static"**
3. **Return type**: `None`
4. **Input parameters**:
   - `TestName` (String)
   - `Success` (Boolean)
   - `Message` (String, default "")

**Function 7: TakeScreenshot**
1. **Name**: `TakeScreenshot`
2. ✅ **Check "Static"**
3. **Return type**: `String`
4. **Input parameters**:
   - `TestName` (String)

#### 2.4 Compile and Save
1. **Click "Compile"** in Blueprint editor
2. **Fix any compilation errors**
3. **Save** the Blueprint

### Step 3: Create Test Map (10 minutes)

#### 3.1 Create Map
1. **File → New Level**
2. **Select "Empty Level"**
3. **Name**: `AutomationTestMap`
4. **Save** in `/Game/SabriMMO/Maps/`

#### 3.2 Setup Test Environment
1. **Place "Player Start"** actor from Modes tab
2. **Place "BP_NPCShop"** actor (from your existing Blueprints)
3. **Select BP_NPCShop** → Details panel → Set `ShopId` to `999`
4. **Add "Directional Light"** for basic lighting
5. **Add "Cube"** or plane for floor surface
6. **Position NPC shop near player start**

#### 3.3 Configure Map Settings
1. **World Settings** (blue button in toolbar)
2. **Game Mode** section → **Default Game Mode**: `GameMode`
3. **Player Controller Class**: `BP_PlayerController`
4. **HUD Class**: `BP_GameHUD`
5. **Save** the map

### Step 4: Configure Automation System (5 minutes)

#### 4.1 Project Settings
1. **Edit → Project Settings**
2. **Engine → Automation**
3. **Enable Automation System**: ✅
4. **Automation Test Name Filter**: `SabriMMO.UI`
5. **Automation Test Class Filter**: `BlueprintFunctionLibrary`
6. **Run Automation Tests on Startup**: ❌ (leave unchecked)

#### 4.2 Create Screenshots Folder
1. **Windows Explorer** → Navigate to `C:/Sabri_MMO/Tests/`
2. **Create folder**: `Screenshots`
3. **Create subfolder**: `Reference`

### Step 5: Test Execution (5 minutes)

#### 5.1 Start Server
```bash
# In new terminal
cd C:/Sabri_MMO/server
npm start
```

#### 5.2 Run Tests in Editor
1. **Open UE5 Editor**
2. **Open AutomationTestMap**
3. **Press "Play"**
4. **Window → Developer Tools → Session Frontend**
5. **Go to "Automation" tab**
6. **Filter**: Type `SabriMMO` in search box
7. **Select all tests** (Ctrl+A)
8. **Click "Run Tests"**

#### 5.3 Verify Results
**Expected Output in Output Log:**
```
Log: ✅ PASS: Test_Shop_OpenWindow
Log: ✅ PASS: Test_Inventory_Toggle
Log: ✅ PASS: Test_Widget_TextDisplay
Log: ✅ PASS: Test_Zuzucoin_Update
Log: ✅ PASS: Test_Complete_Shop_Flow
```

## 🔧 Troubleshooting Guide

### Common Issues and Solutions

#### Issue 1: "Widget not found" Error
**Problem**: Test can't find UI widgets
**Solution**:
1. Ensure widgets have correct names in Blueprint
2. Check widget names match exactly (case-sensitive)
3. Add delay after widget creation (0.1-0.2 seconds)

#### Issue 2: "HUD Manager null" Error
**Problem**: Can't get HUD Manager reference
**Solution**:
1. Ensure player controller is valid
2. Add IsValid check before getting HUD Manager
3. Make sure game mode is set correctly

#### Issue 3: Tests Not Found in Automation Tab
**Problem**: Tests don't appear in automation list
**Solution**:
1. Ensure functions are marked as "Static"
2. Check function names match filter pattern
3. Compile and save Blueprint
4. Restart editor if needed

#### Issue 4: Compilation Errors
**Problem**: Blueprint won't compile
**Solution**:
1. Check all node connections are valid
2. Ensure variable types match
3. Verify function signatures match guide
4. Check for missing required nodes

#### Issue 5: Server Connection Issues
**Problem**: Tests fail due to server issues
**Solution**:
1. Ensure server is running on localhost:3001
2. Check test data exists in database
3. Verify test endpoints work: `http://localhost:3001/test/setup`

## 📊 Expected Results

### Successful Setup Indicators
- ✅ All 5 test functions appear in automation list
- ✅ Tests run without crashes
- ✅ Output log shows "✅ PASS" messages
- ✅ No "ERROR:" messages in output
- ✅ Screenshots saved to Tests/Screenshots/

### Test Coverage Achieved
- **Widget Visibility**: ✅ Shop and inventory windows
- **Button Interactions**: ✅ Toggle functionality
- **Text Display Updates**: ✅ Zuzucoin and item names
- **Data Synchronization**: ✅ Server to UI updates
- **Complete User Flows**: ✅ Shop transaction cycle

## 🚀 Next Steps

### Phase 1: Basic Tests (This Setup)
- ✅ Shop window operations
- ✅ Inventory toggle
- ✅ Text display verification
- ✅ Zuzucoin updates

### Phase 2: Advanced Tests (Future)
- Combat UI interactions
- Character stat allocation
- Equipment management
- Visual regression testing

### Phase 3: Integration (Future)
- CI/CD pipeline integration
- Performance testing
- Multi-client testing
- Automated reporting

## 📋 Quick Reference

### Essential Files Created
- `server/src/test_mode.js` - Mock data endpoints
- `database/test_setup.sql` - Test data setup
- `BP_AutomationTestLibrary` - UI test functions
- `AutomationTestMap` - Test environment

### Key Commands
```bash
# Database setup
psql -d sabri_mmo -f database/test_setup.sql

# Start server
cd server && npm start

# Run tests (in editor)
Window → Developer Tools → Session Frontend → Automation tab

# Run tests (command line)
SabriMMO.exe AutomationTestMap -ExecCmds="Automation RunTests=UI"
```

### Test Function Names
- `Test_Shop_OpenWindow`
- `Test_Inventory_Toggle`
- `Test_Widget_TextDisplay`
- `Test_Zuzucoin_Update`
- `Test_Complete_Shop_Flow`

## 🎯 Success Criteria

Your UI testing setup is successful when:
1. ✅ All 5 tests compile without errors
2. ✅ Tests appear in automation system
3. ✅ Tests run and show "✅ PASS" results
4. ✅ No server errors during tests
5. ✅ Screenshots are captured (if using TakeScreenshot)

This setup provides comprehensive UI testing that prevents visual bugs and ensures user experience quality for your MMO!
