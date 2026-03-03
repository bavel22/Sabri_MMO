# UI Testing Implementation Guide

## Quick Start: 5 Steps to UI Testing

### Step 1: Create Test Blueprint (5 minutes)
1. **Open UE5 Editor**
2. **Right-click Content Browser** → **Blueprint Class**
3. **Select "Blueprint Function Library"**
4. **Name it**: `BP_AutomationTestLibrary`
5. **Save in**: `/Game/SabriMMO/Tests/`

### Step 2: Add First Test Function (10 minutes)
1. **Open BP_AutomationTestLibrary**
2. **Click "Functions" panel**
3. **Click "+" → Add Function**
4. **Name it**: `Test_Shop_OpenWindow`
5. **Set "Static" checkbox**
6. **Add nodes** (see UI_Test_Functions.md)

### Step 3: Create Test Map (5 minutes)
1. **File → New Level**
2. **Name it**: `AutomationTestMap`
3. **Add Player Start**
4. **Add BP_NPCShop** (from your Blueprints)
5. **Save map**

### Step 4: Run Test (2 minutes)
1. **Open AutomationTestMap**
2. **Press "Play"**
3. **Window → Developer Tools → Session Frontend**
4. **Go to "Automation" tab**
5. **Filter by "SabriMMO"**
6. **Click "Run Tests"**

### Step 5: Review Results (1 minute)
1. **Check Output Log** for test results
2. **Look for "LogSuccess" or "LogError" messages
3. **Fix any issues found**

---

## Detailed Implementation

### Creating the Test Library

#### 1. Blueprint Setup
```
BP_AutomationTestLibrary
├── Functions (Static)
│   ├── Test_Shop_OpenWindow
│   ├── Test_Inventory_Toggle
│   ├── Test_Widget_TextDisplay
│   └── TakeScreenshot
├── Variables (None needed)
└── Components (None needed)
```

#### 2. Required Nodes for Test_Shop_OpenWindow
```cpp
// Node structure in Blueprint:
Event: Test_Shop_OpenWindow
    ↓
Get Player Controller (index 0)
    ↓
Cast To BP_MMOCharacter (as Player Character)
    ↓
Get HUD Manager (from character)
    ↓
Branch: IsValid HUD Manager?
    ├─ TRUE → Continue
    └─ FALSE → Log Error → Return False
    ↓
Call OpenShop (with mock data)
    ↓
Delay (0.1 seconds)
    ↓
Get Shop Window Ref
    ↓
Branch: IsValid Shop Window?
    ├─ TRUE → Check visibility and text
    └─ FALSE → Log Error → Return False
    ↓
Check Shop Name Text
    ↓
Check Zuzucoin Text
    ↓
Check Item List
    ↓
Log Success → Return True
```

### Mock Data Setup

#### 1. Create Mock Data Variables
```cpp
// In BP_AutomationTestLibrary, add these variables:
MockShopData (String) = JSON shop data
MockInventoryData (String) = JSON inventory data
MockCharacterData (String) = JSON character data
```

#### 2. Mock Data JSON Strings
```json
// MockShopData default value:
{
    "shopName": "Test Weapon Shop",
    "shopId": 1,
    "playerZuzucoin": 5000,
    "items": [
        {
            "item_id": 1,
            "name": "Iron Sword",
            "price": 1000,
            "description": "Basic sword",
            "item_type": "weapon",
            "weapon_type": "sword",
            "atk": 25,
            "required_level": 5
        }
    ]
}

// MockInventoryData default value:
{
    "items": [
        {
            "inventory_id": 1,
            "item_id": 1,
            "name": "Iron Sword",
            "quantity": 1,
            "equipped": false,
            "item_type": "weapon",
            "atk": 25
        }
    ],
    "zuzucoin": 5000
}
```

### Test Execution Setup

#### 1. Project Settings Configuration
```
Project Settings → Engine → Automation:
✓ Enable Automation System
✓ Automation Test Name Filter: "SabriMMO"
✓ Automation Test Class Filter: "BlueprintFunctionLibrary"
✓ Run Automation Tests on Startup (optional)
```

#### 2. Console Commands for Testing
```
// Run specific test
Automation RunTests=Test_Shop_OpenWindow

// Run all UI tests
Automation RunTests=UI

// Run tests and quit
Automation RunTests=UI,Quit

// List available tests
Automation ListTests
```

### Visual Testing Setup

#### 1. Screenshot Function
```cpp
// Add to BP_AutomationTestLibrary:
Function: TakeScreenshot (TestName: String) -> String
    ↓
Get Game Viewport Client
    ↓
Get Viewport
    ↓
Take Screenshot (File Path)
    ↓
Return File Path
```

#### 2. Reference Screenshots
```
Create folder: C:/Sabri_MMO/tests/client/Screenshots/Reference/
Add files:
- Shop_Open.png
- Shop_AfterPurchase.png
- Inventory_Open.png
- Combat_Damage.png
```

### Integration with Existing Tests

#### 1. Combine Server and UI Tests
```cpp
// Modified shop test that includes server validation:
Test_Shop_CompleteFlow():
    // 1. Test server logic (existing test)
    if (!RunServerShopTest()):
        return false
    
    // 2. Test UI response (new test)
    if (!Test_Shop_OpenWindow()):
        return false
    
    // 3. Test interaction
    if (!Test_Shop_BuyItem()):
        return false
    
    // 4. Test UI update
    if (!Test_Zuzucoin_Update()):
        return false
    
    return true
```

#### 2. Test Data Synchronization
```cpp
// Ensure UI tests use same data as server tests
Function: SyncTestData()
    ↓
Get Test Data from Server Tests
    ↓
Update Mock Variables
    ↓
Verify Data Consistency
```

### Automation Pipeline Integration

#### 1. GitHub Actions Setup
```yaml
# .github/workflows/ui-tests.yml
name: UI Tests
on: [push, pull_request]

jobs:
  ui-tests:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build Project
        run: |
          # Build UE5 project
      - name: Run UI Tests
        run: |
          SabriMMO.exe AutomationTestMap -ExecCmds="Automation RunTests=UI,Quit"
      - name: Upload Results
        uses: actions/upload-artifact@v3
        with:
          name: ui-test-results
          path: Tests/Results/
```

#### 2. Local Testing Script
```batch
@echo off
echo Running UI Tests...
cd C:/Sabri_MMO

echo Starting server...
start cmd /k "cd server && npm start"

echo Waiting for server...
timeout /t 5

echo Running UI tests...
SabriMMO.exe AutomationTestMap -ExecCmds="Automation RunTests=UI,Quit"

echo Tests completed!
pause
```

### Troubleshooting Guide

#### Common Issues and Solutions

##### Issue 1: Widget References Null
```
Problem: GetWidgetByName returns None
Solution: 
1. Ensure widget is created and added to viewport
2. Use correct widget names (case-sensitive)
3. Add delay after widget creation
```

##### Issue 2: Tests Not Found
```
Problem: Automation system can't find tests
Solution:
1. Ensure functions are marked as Static
2. Check function names match filter
3. Verify Blueprint is saved and compiled
```

##### Issue 3: Socket Manager Not Ready
```
Problem: SocketManagerRef is None during tests
Solution:
1. Add delay after player join
2. Create mock socket manager for testing
3. Check connection state before testing
```

##### Issue 4: Timing Issues
```
Problem: Tests fail due to timing
Solution:
1. Add Delay nodes after async operations
2. Use retry logic for flaky tests
3. Increase default delay times
```

### Performance Considerations

#### Test Optimization
```cpp
// Use object pooling for test objects
Function: GetTestCharacter():
    if (TestCharacterPool.IsEmpty()):
        return SpawnTestCharacter()
    else:
        return TestCharacterPool.Pop()

// Clean up after tests
Function: CleanupTest():
    // Return objects to pool
    // Reset widget states
    // Clear mock data
```

#### Parallel Testing
```cpp
// Run independent tests in parallel
Function: RunParallelTests():
    // Shop tests (independent)
    // Inventory tests (independent)
    // Combat tests (independent)
    // Visual tests (independent)
```

### Maintenance Guidelines

#### Weekly Tasks
1. Update reference screenshots if UI changes
2. Review test failure logs
3. Update mock data for new features
4. Check test coverage metrics

#### Monthly Tasks
1. Optimize slow tests
2. Add tests for new features
3. Review and update test documentation
4. Check automation pipeline health

#### When Adding New UI Features
1. Create corresponding test functions
2. Update mock data
3. Add new reference screenshots
4. Update test documentation

### Success Metrics

#### Coverage Targets
- Widget Visibility: 100%
- Button Interactions: 100%
- Text Display Updates: 95%
- Complete User Flows: 90%

#### Performance Targets
- Test Execution Time: < 5 minutes
- Memory Usage: < 2GB during tests
- FPS During Tests: > 30 FPS
- Test Reliability: > 98%

## Next Steps

1. **Implement Basic Tests** (Week 1)
   - Shop open/close
   - Inventory toggle
   - Basic widget verification

2. **Add Integration Tests** (Week 2)
   - Complete shop flow
   - Inventory management
   - Combat interactions

3. **Visual Testing** (Week 3)
   - Screenshot comparison
   - Layout verification
   - Animation states

4. **CI/CD Integration** (Week 4)
   - Automated pipeline
   - Performance monitoring
   - Test reporting dashboard

This setup gives you comprehensive UI testing that prevents visual bugs and ensures user experience quality!
