# UI Test Functions Implementation

## Shop UI Tests

### Test_Shop_OpenWindow
```cpp
// Static Function in BP_AutomationTestLibrary
// Test: Shop window opens and displays correctly

Test_Shop_OpenWindow():
    // Setup
    PlayerController = GetPlayerController(0)
    HUDManager = PlayerController.GetHUDManager()
    
    // Execute
    HUDManager.OpenShop(MOCK_SHOP_DATA_JSON)
    
    // Wait for widget creation
    Delay(0.1)
    
    // Verify
    ShopWindow = HUDManager.ShopWindowRef
    if (ShopWindow == None):
        LogError("ShopWindowRef is null")
        return false
    
    // Check visibility
    if (!ShopWindow.IsVisible()):
        LogError("Shop window not visible")
        return false
    
    // Check shop name
    ShopNameText = ShopWindow.ShopNameText
    if (ShopNameText.GetText() != "Test Weapon Shop"):
        LogError("Shop name incorrect")
        return false
    
    // Check zuzucoin display
    ZuzucoinText = ShopWindow.ZuzucoinText
    if (ZuzucoinText.GetText() != "5000 Z"):
        LogError("Zuzucoin display incorrect")
        return false
    
    // Check item list
    ItemsList = ShopWindow.ShopItemsList
    if (ItemsList.GetChildrenCount() == 0):
        LogError("No items in shop")
        return false
    
    LogSuccess("Shop window opened correctly")
    return true
```

### Test_Shop_BuyItem
```cpp
// Test: Complete shop purchase flow

Test_Shop_BuyItem():
    // Setup: Open shop first
    if (!Test_Shop_OpenWindow()):
        return false
    
    // Get first shop item
    ShopWindow = GetHUDManager().ShopWindowRef
    ItemsList = ShopWindow.ShopItemsList
    FirstItem = ItemsList.GetChildAt(0)
    
    if (FirstItem == None):
        LogError("No shop items found")
        return false
    
    // Get initial zuzucoin
    InitialZuzucoin = ShopWindow.ZuzucoinText.GetText()
    
    // Simulate buy button click
    BuyButton = FirstItem.BuyButton
    if (BuyButton == None):
        LogError("Buy button not found")
        return false
    
    // Mock server response
    MockSocketResponse("shop:bought", {
        "itemId": 1,
        "itemName": "Iron Sword",
        "quantity": 1,
        "totalCost": 1000,
        "newZuzucoin": 4000
    })
    
    // Click buy button
    BuyButton.OnClicked.Broadcast()
    
    // Wait for processing
    Delay(0.2)
    
    // Verify zuzucoin updated
    NewZuzucoin = ShopWindow.ZuzucoinText.GetText()
    if (NewZuzucoin != "4000 Z"):
        LogError("Zuzucoin not updated correctly")
        return false
    
    // Verify inventory updated (if open)
    InventoryWindow = GetHUDManager().InventoryWindowRef
    if (InventoryWindow != None and InventoryWindow.IsVisible()):
        // Check if item appears in inventory
        InventorySlots = InventoryWindow.InventoryGrid.GetChildren()
        FoundItem = false
        for each Slot in InventorySlots:
            if (Slot.ItemName == "Iron Sword":
                FoundItem = true
                break
        
        if (!FoundItem):
            LogError("Item not found in inventory")
            return false
    
    LogSuccess("Shop purchase completed correctly")
    return true
```

## Inventory UI Tests

### Test_Inventory_Toggle
```cpp
// Test: Inventory window open/close toggle

Test_Inventory_Toggle():
    // Setup
    HUDManager = GetPlayerController(0).GetHUDManager()
    InitialState = HUDManager.bIsInventoryOpen
    
    // Execute toggle
    HUDManager.ToggleInventory()
    
    // Wait for widget creation
    Delay(0.1)
    
    // Verify window opened
    InventoryWindow = HUDManager.InventoryWindowRef
    if (InventoryWindow == None):
        LogError("InventoryWindowRef is null after toggle")
        return false
    
    if (!InventoryWindow.IsVisible()):
        LogError("Inventory window not visible after toggle")
        return false
    
    if (!HUDManager.bIsInventoryOpen):
        LogError("bIsInventoryOpen flag not set")
        return false
    
    // Execute second toggle
    HUDManager.ToggleInventory()
    Delay(0.1)
    
    // Verify window closed
    if (InventoryWindow.IsVisible()):
        LogError("Inventory window still visible after second toggle")
        return false
    
    if (HUDManager.bIsInventoryOpen):
        LogError("bIsInventoryOpen flag not cleared")
        return false
    
    LogSuccess("Inventory toggle works correctly")
    return true
```

### Test_Inventory_Populate
```cpp
// Test: Inventory populates with correct data

Test_Inventory_Populate():
    // Setup: Open inventory
    HUDManager = GetPlayerController(0).GetHUDManager()
    HUDManager.ToggleInventory()
    Delay(0.1)
    
    // Mock inventory data
    MockSocketResponse("inventory:data", MOCK_INVENTORY_DATA_JSON)
    
    // Trigger populate
    InventoryWindow = HUDManager.InventoryWindowRef
    HUDManager.PopulateInventory(MOCK_INVENTORY_DATA_JSON)
    
    // Wait for processing
    Delay(0.2)
    
    // Verify zuzucoin display
    ZuzucoinText = InventoryWindow.ZuzucoinText
    if (ZuzucoinText.GetText() != "5000 Z"):
        LogError("Inventory zuzucoin display incorrect")
        return false
    
    // Verify item slots created
    InventoryGrid = InventoryWindow.InventoryGrid
    ItemSlots = InventoryGrid.GetChildren()
    
    if (ItemSlots.Count() != 2):
        LogError("Incorrect number of inventory slots")
        return false
    
    // Verify first item
    FirstSlot = ItemSlots[0]
    if (FirstSlot.ItemName != "Iron Sword"):
        LogError("First item name incorrect")
        return false
    
    if (FirstSlot.Quantity != "1"):
        LogError("First item quantity incorrect")
        return false
    
    // Verify second item
    SecondSlot = ItemSlots[1]
    if (SecondSlot.ItemName != "Health Potion"):
        LogError("Second item name incorrect")
        return false
    
    if (SecondSlot.Quantity != "5"):
        LogError("Second item quantity incorrect")
        return false
    
    LogSuccess("Inventory populated correctly")
    return true
```

## Widget State Tests

### Test_Widget_TextDisplay
```cpp
// Generic test for text display widgets

Test_Widget_TextDisplay(WidgetName, ExpectedText):
    // Find widget
    Widget = FindWidgetByName(WidgetName)
    if (Widget == None):
        LogError("Widget not found: " + WidgetName)
        return false
    
    // Get text block
    TextBlock = Widget.GetTextBlock()
    if (TextBlock == None):
        LogError("TextBlock not found in widget: " + WidgetName)
        return false
    
    // Compare text
    ActualText = TextBlock.GetText().ToString()
    if (ActualText != ExpectedText):
        LogError("Text mismatch in " + WidgetName)
        LogError("Expected: " + ExpectedText)
        LogError("Actual: " + ActualText)
        return false
    
    LogSuccess("Text display correct for " + WidgetName)
    return true
```

### Test_Button_ClickResponse
```cpp
// Generic test for button interactions

Test_Button_ClickResponse(ButtonWidgetName, ExpectedSocketEvent):
    // Find button
    ButtonWidget = FindWidgetByName(ButtonWidgetName)
    if (ButtonWidget == None:
        LogError("Button not found: " + ButtonWidgetName)
        return false
    
    // Setup socket event listener
    EventReceived = false
    BindSocketEvent(ExpectedSocketEvent, function():
        EventReceived = true
    )
    
    // Click button
    ButtonWidget.OnClicked.Broadcast()
    
    // Wait for event
    Delay(0.1)
    
    // Verify event fired
    if (!EventReceived):
        LogError("Expected socket event not fired: " + ExpectedSocketEvent)
        return false
    
    LogSuccess("Button click triggered correct event: " + ButtonWidgetName)
    return true
```

## Integration Tests

### Test_Complete_Shop_Flow
```cpp
// Test complete shop interaction flow

Test_Complete_Shop_Flow():
    // Step 1: Join game
    if (!Test_Player_JoinGame()):
        return false
    
    // Step 2: Open shop
    if (!Test_Shop_OpenWindow()):
        return false
    
    // Step 3: Browse items
    if (!Test_Shop_BrowseItems()):
        return false
    
    // Step 4: Buy item
    if (!Test_Shop_BuyItem()):
        return false
    
    // Step 5: Check inventory
    if (!Test_Inventory_VerifyPurchase()):
        return false
    
    // Step 6: Close shop
    if (!Test_Shop_CloseWindow()):
        return false
    
    LogSuccess("Complete shop flow works correctly")
    return true
```

### Test_Combat_UI_Flow
```cpp
// Test combat UI interactions

Test_Combat_UI_Flow():
    // Setup: Spawn test enemy
    Enemy = SpawnTestEnemy()
    
    // Step 1: Target enemy
    if (!Test_Combat_TargetEnemy(Enemy)):
        return false
    
    // Step 2: Attack enemy
    if (!Test_Combat_Attack(Enemy)):
        return false
    
    // Step 3: Receive damage
    if (!Test_Combat_ReceiveDamage()):
        return false
    
    // Step 4: Enemy death
    if (!Test_Combat_EnemyDeath(Enemy)):
        return false
    
    // Step 5: Loot popup
    if (!Test_Combat_LootPopup()):
        return false
    
    LogSuccess("Combat UI flow works correctly")
    return true
```

## Helper Functions

### FindWidgetByName
```cpp
// Helper to find widgets by name

FindWidgetByName(WidgetName):
    // Search in viewport
    Viewport = GetGameViewportClient().GetGameWidget()
    return Viewport.GetWidgetFromName(WidgetName)
```

### MockSocketResponse
```cpp
// Helper to mock server responses

MockSocketResponse(EventName, Data):
    SocketManager = GetSocketManager()
    if (SocketManager != None:
        // Simulate receiving server event
        SocketManager.ProcessMockEvent(EventName, Data)
    else:
        LogError("SocketManager not found for mocking")
```

### TakeScreenshot
```cpp
// Helper for visual testing

TakeScreenshot(TestName):
    // Capture current viewport
    ScreenshotPath = "Tests/Screenshots/" + TestName + ".png"
    GetGameViewportClient().Viewport.TakeScreenshot(ScreenshotPath)
    return ScreenshotPath
```

### CompareScreenshots
```cpp
// Helper for visual regression

CompareScreenshots(CurrentPath, ReferencePath):
    CurrentImage = LoadImage(CurrentPath)
    ReferenceImage = LoadImage(ReferencePath)
    
    if (CurrentImage == None or ReferenceImage == None):
        return 0.0
    
    // Pixel-by-pixel comparison
    Difference = CalculateImageDifference(CurrentImage, ReferenceImage)
    Similarity = 1.0 - Difference
    
    return Similarity
```

## Test Execution Order

### Recommended Test Sequence
1. **Setup Tests** (player join, character load)
2. **Widget Tests** (individual widgets)
3. **Integration Tests** (complete flows)
4. **Visual Tests** (screenshot comparison)
5. **Performance Tests** (FPS, memory)
6. **Cleanup Tests** (reset state)

### Test Dependencies
```
Setup Tests → Widget Tests → Integration Tests → Visual Tests
     ↓              ↓              ↓              ↓
Player Ready → UI Elements Work → Complete Flows Work → Visual Correctness
```

## Error Handling

### Common Failure Points
- Widget references are null
- Socket manager not ready
- Mock data invalid
- Timing issues (need delays)
- Blueprint compilation errors

### Recovery Strategies
```cpp
// Add retry logic for timing-sensitive tests
RetryTest(TestFunction, MaxRetries=3):
    for i = 0 to MaxRetries:
        if (TestFunction()):
            return true
        Delay(0.1)  // Wait before retry
    
    return false
```

### Debug Logging
```cpp
// Detailed logging for troubleshooting
LogTestStart(TestName)
LogTestStep(StepDescription, StepData)
LogTestPass(TestName)
LogTestFail(TestName, ErrorReason)
```
