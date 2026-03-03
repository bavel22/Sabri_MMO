# Test Environment Setup for UI Testing

## Overview
Create a controlled environment for automated UI testing.

## Step 1: Create Test Map

### Map: `AutomationTestMap`
1. **Create New Level**: Empty level
2. **Add Test Elements**:
   - Player Start (PlayerController)
   - Test Shop NPC (BP_NPCShop)
   - Test Enemy (BP_EnemyCharacter)
   - Test Environment (basic floor, lighting)

### Map Configuration
```
Map Settings:
- Default Game Mode: GameMode (uses BP_MMOCharacter)
- Player Controller Class: BP_PlayerController
- HUD Class: BP_GameHUD
```

## Step 2: Test Character Setup

### Test Character Data
```sql
-- Add to database for testing
INSERT INTO characters (user_id, name, class, level, hp, max_hp, mp, max_mp,
    str, agi, vit, int_stat, dex, luk, stat_points, zuzucoin, x, y, z)
VALUES (1, 'TestPlayer_UI', 'Warrior', 10, 500, 500, 200, 200,
    20, 15, 18, 10, 12, 8, 20, 5000, 0, 0, 300);
```

### Test Inventory
```sql
-- Give test character items for testing
INSERT INTO inventory (character_id, item_id, quantity, equipped)
VALUES 
    (CHAR_ID, 1, 1, false),  -- Test weapon
    (CHAR_ID, 2, 5, false),  -- Test consumables
    (CHAR_ID, 3, 1, true);   -- Equipped armor
```

## Step 3: Mock Server Responses

### Create Mock Socket Manager
```cpp
// BP_MockSocketManager (for testing)
class BP_MockSocketManager : Actor
{
    // Mock Functions
    function MockShopData()
    function MockInventoryData()
    function MockCombatDamage()
    function MockZuzucoinUpdate()
}
```

### Mock Data Templates
```javascript
// Mock Shop Data
const MOCK_SHOP_DATA = {
    shopName: "Test Weapon Shop",
    shopId: 1,
    playerZuzucoin: 5000,
    items: [
        {
            item_id: 1,
            name: "Iron Sword",
            price: 1000,
            description: "Basic sword for beginners",
            item_type: "weapon",
            weapon_type: "sword",
            atk: 25,
            required_level: 5
        },
        {
            item_id: 2,
            name: "Health Potion",
            price: 50,
            description: "Restores 100 HP",
            item_type: "consumable",
            quantity: 1,
            required_level: 1
        }
    ]
};

// Mock Inventory Data
const MOCK_INVENTORY_DATA = {
    items: [
        {
            inventory_id: 1,
            item_id: 1,
            name: "Iron Sword",
            quantity: 1,
            equipped: false,
            item_type: "weapon",
            atk: 25
        },
        {
            inventory_id: 2,
            item_id: 2,
            name: "Health Potion",
            quantity: 5,
            equipped: false,
            item_type: "consumable"
        }
    ],
    zuzucoin: 5000
};
```

## Step 4: Test Configuration

### Project Settings
```
Project Settings → Engine → Automation:
✓ Enable Automation Tests
✓ Run Automation Tests on Startup (optional)
✓ Automation Test Name Filter: "SabriMMO.UI"
✓ Automation Test Class Filter: "BlueprintFunctionLibrary"
```

### Console Variables
```
// Add to DefaultEngine.ini
[/Script/Engine.AutomationTestManager]
DefaultAutomationTestNameFilter=SabriMMO.UI
DefaultAutomationTestClassFilter=BlueprintFunctionLibrary
```

## Step 5: Test Execution Setup

### In Editor Testing
1. Open `AutomationTestMap`
2. Press `Play` to start test session
3. Open `Session Frontend` → `Automation` tab
4. Filter tests by "SabriMMO.UI"
5. Click `Run Tests`

### Command Line Testing
```bash
# Run all UI tests
SabriMMO.exe AutomationTestMap -ExecCmds="Automation RunTests=UI"

# Run specific test
SabriMMO.exe -ExecCmds="Automation RunTests=UI.Shop.Open"

# Run tests and exit
SabriMMO.exe -ExecCmds="Automation RunTests=UI,Quit"
```

### CI/CD Integration
```yaml
# GitHub Actions example
- name: Run UI Tests
  run: |
    ./SabriMMO.exe -ExecCmds="Automation RunTests=UI,Quit"
    # Check exit code for test results
```

## Step 6: Test Data Management

### Test Database
```sql
-- Create separate test database
CREATE DATABASE sabri_mmo_test;

-- Use test-specific schema
-- Allows destructive testing without affecting main database
```

### Test Cleanup
```cpp
// After each test:
function CleanupTest()
{
    // Reset character position
    // Clear inventory
    // Reset zuzucoin
    // Close all windows
    // Disconnect mock sockets
}
```

## Step 7: Visual Testing Setup

### Screenshot Comparison
```cpp
// Add to test library:
function TakeScreenshot(TestName)
{
    // Capture viewport
    // Save to Tests/Screenshots/TestName.png
    // Compare with reference image
}

function CompareScreenshots(Current, Reference)
{
    // Pixel-by-pixel comparison
    // Return similarity percentage
    // Log differences
}
```

### Reference Screenshots
```
Tests/Screenshots/Reference/
├── Shop_Open.png
├── Shop_AfterPurchase.png
├── Inventory_Open.png
├── Inventory_Equipped.png
└── Combat_Damage.png
```

## Step 8: Performance Testing

### Frame Rate Monitoring
```cpp
function MeasurePerformance(TestName)
{
    // Start FPS counter
    // Run test actions
    // Record min/max/average FPS
    // Log performance metrics
}
```

### Memory Usage
```cpp
function CheckMemoryUsage()
{
    // Get current memory usage
    // Compare with baseline
    // Log memory leaks
}
```

## Step 9: Test Reporting

### Results Format
```json
{
    "test_name": "UI.Shop.OpenWindow",
    "status": "passed",
    "duration": 2.3,
    "assertions": [
        {
            "description": "Shop window should be visible",
            "expected": true,
            "actual": true,
            "passed": true
        },
        {
            "description": "Zuzucoin should display 5000 Z",
            "expected": "5000 Z",
            "actual": "5000 Z",
            "passed": true
        }
    ],
    "screenshots": [
        "Shop_Open_Before.png",
        "Shop_Open_After.png"
    ],
    "performance": {
        "fps_min": 58,
        "fps_avg": 62,
        "fps_max": 75,
        "memory_mb": 1024
    }
}
```

### Test Dashboard
```html
<!-- Simple HTML report viewer -->
<!DOCTYPE html>
<html>
<head>
    <title>SabriMMO UI Test Results</title>
</head>
<body>
    <div id="test-results"></div>
    <script src="test-results.json"></script>
</body>
</html>
```

## Step 10: Maintenance

### Test Updates
- Update reference screenshots when UI changes
- Maintain test data with new features
- Update mock data for new shop items
- Add new tests for new UI elements

### Regression Prevention
- Run tests before each deployment
- Fail build if any UI test fails
- Monitor test execution time
- Track test success rate over time

## Expected Timeline

### Week 1: Setup
- Create test map and environment
- Set up automation framework
- Create basic test functions

### Week 2: Implementation
- Write shop UI tests
- Write inventory UI tests
- Add visual comparison tests

### Week 3: Integration
- Set up CI/CD pipeline
- Add performance monitoring
- Create test reporting dashboard

### Week 4: Expansion
- Add combat UI tests
- Add character creation tests
- Implement full user journey tests

## Success Metrics

- Test Coverage: Target 95% of UI elements
- Test Execution Time: < 5 minutes for full suite
- False Positive Rate: < 5%
- Test Reliability: > 98% consistent results
