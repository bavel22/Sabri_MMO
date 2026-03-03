# BP_AutomationTestLibrary

**Path**: `/Game/SabriMMO/Tests/BP_AutomationTestLibrary`  
**Parent Class**: `BlueprintFunctionLibrary`  
**Purpose**: Static functions for UI automation testing

## Functions to Create

### Shop UI Tests
```cpp
// Static Function: Test_Shop_OpenWindow
// Returns: bool (success/failure)
// Description: Tests if shop window opens correctly

Test_Shop_OpenWindow():
    1. Get Player Controller
    2. Get HUD Manager (AC_HUDManager)
    3. Call OpenShop with test data
    4. Verify ShopWindowRef is valid
    5. Check WBP_Shop is visible
    6. Verify shop name displays
    7. Check zuzucoin display shows correct amount
    8. Return true if all checks pass
```

### Inventory UI Tests
```cpp
// Static Function: Test_Inventory_Toggle
// Returns: bool
// Description: Tests inventory window open/close

Test_Inventory_Toggle():
    1. Get HUD Manager
    2. Call ToggleInventory()
    3. Verify InventoryWindowRef is valid
    4. Check WBP_InventoryWindow is visible
    5. Call ToggleInventory() again
    6. Verify window is hidden
    7. Return true if toggle works
```

### Widget State Tests
```cpp
// Static Function: Test_Widget_TextDisplay
// Parameters: WidgetName, ExpectedText
// Returns: bool
// Description: Tests if widget displays correct text

Test_Widget_TextDisplay(WidgetName, ExpectedText):
    1. Find widget by name
    2. Cast to correct widget type
    3. Get text block component
    4. Compare displayed text with expected
    5. Log result
    6. Return comparison result
```

### Button Interaction Tests
```cpp
// Static Function: Test_Button_ClickResponse
// Parameters: ButtonWidgetName, ExpectedEvent
// Returns: bool
// Description: Tests if button click triggers correct response

Test_Button_ClickResponse(ButtonWidgetName, ExpectedEvent):
    1. Find button widget
    2. Verify button is clickable
    3. Simulate button click
    4. Check if expected event fired
    5. Verify server received correct event
    6. Return true if flow works
```

### Data Display Tests
```cpp
// Static Function: Test_Zuzucoin_Update
// Returns: bool
// Description: Tests zuzucoin display updates after transaction

Test_Zuzucoin_Update():
    1. Get initial zuzucoin display
    2. Simulate shop purchase
    3. Wait for server response
    4. Check if zuzucoin text updated
    5. Verify new amount is correct
    6. Return true if update works
```

## Implementation Notes

### Finding Widgets
```cpp
// Use these patterns to find widgets:
GetWidgetOfClass(WBP_Shop)
GetWidgetByName("ShopWindow")
GetAllWidgetsOfClass(WBP_InventorySlot)
```

### Simulating Input
```cpp
// Simulate button clicks:
ButtonWidget->OnClicked.Broadcast()

// Simulate text input:
EditableText->SetText(FText::FromString("test"))

// Simulate list selection:
ListView->SetSelectedIndex(0)
```

### Verification Patterns
```cpp
// Check widget visibility:
Widget->IsVisible()

// Check text content:
TextBlock->GetText().ToString()

// Check list contents:
ListView->GetNumItems()
ListView->GetItemAt(0)
```

## Test Data Setup

### Mock Socket Events
```cpp
// Create mock socket responses for testing:
MockShopData = {
    "shopName": "Test Shop",
    "playerZuzucoin": 1000,
    "items": [
        {"item_id": 1, "name": "Test Item", "price": 100}
    ]
}
```

### Test Character Setup
```cpp
// Ensure test character has:
- Valid inventory
- Sufficient zuzucoin
- Required level for items
```

## Integration with UE5 Automation

### Test Registration
```cpp
// In AutomationTestManager:
IMPLEMENT_AUTOMATION_TEST(FShopOpenTest, "SabriMMO.UI.Shop.Open")
IMPLEMENT_AUTOMATION_TEST(FInventoryToggleTest, "SabriMMO.UI.Inventory.Toggle")
IMPLEMENT_AUTOMATION_TEST(FZuzucoinUpdateTest, "SabriMMO.UI.Currency.Update")
```

### Test Execution
```cpp
// Each test function should:
1. Set up test conditions
2. Execute test actions
3. Verify results
4. Clean up test state
5. Return pass/fail result
```

## Expected Test Results

### Success Criteria
- All widgets found and accessible
- UI elements respond to interactions
- Data displays update correctly
- Server events fire as expected
- No crashes or errors

### Failure Detection
- Widget references are null
- Text doesn't match expected values
- Buttons don't trigger events
- Server doesn't receive expected events
- Visual state doesn't update

## Usage in Editor

### Running Individual Tests
1. Open Blueprint Editor
2. Right-click test function
3. Select "Execute Function"
4. Check output log for results

### Running All Tests
1. Window → Developer Tools → Session Frontend
2. Go to Automation tab
3. Filter by "SabriMMO.UI"
4. Click "Run Tests"
5. Review results in log

## Next Steps

After implementing these tests:
1. Create test map with test environment
2. Add test data to database
3. Configure automation settings
4. Set up CI/CD integration
5. Add visual regression tests
