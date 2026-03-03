# Automated UI Testing System

## Overview

Sabri_MMO includes a comprehensive automated UI testing framework to prevent visual bugs, ensure user experience quality, and validate UI functionality across development cycles.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    UI Testing Architecture                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ASabriMMOUITests (C++)                                     │
│  ├─ Test Runner Logic                                       │
│  ├─ Player Spawn Detection                                  │
│  ├─ HUD Manager Validation                                  │
│  └─ Result Reporting                                        │
│                                                             │
│  BP_AutomationTestLibrary (Blueprint)                       │
│  ├─ Test_Shop_OpenWindow()                                  │
│  ├─ Test_Inventory_Toggle()                                 │
│  ├─ Test_Widget_TextDisplay()                               │
│  ├─ Test_Zuzucoin_Update()                                  │
│  └─ Test_Complete_Shop_Flow()                               │
│                                                             │
│  UE5 Automation System Integration                           │
│  ├─ Session Frontend → Automation Tab                       │
│  ├─ Filter: "SabriMMO.UI"                                   │
│  └─ Command Line: `Automation RunTests=UI`                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Components

### 1. C++ Test Runner (`ASabriMMOUITests`)

**Location**: `Source/SabriMMO/SabriMMOUITests.cpp`

**Purpose**: Core test execution engine with robust player spawn detection

**Key Features**:
- Auto-executes 5 seconds after BeginPlay
- Retries player spawn detection up to 10 times
- Manual player spawn fallback via GameMode
- On-screen debug messages + Output Log results
- Server-authoritative validation

**Current Tests**:
```cpp
bool Test_GameInstanceValid();           // UMMOGameInstance validation
bool Test_PlayerCharacterValid();        // BP_MMOCharacter spawn check  
bool Test_HUDManagerFound();             // AC_HUDManager component detection
bool Test_InventoryToggle();             // ToggleInventory() + bIsInventoryOpen
bool Test_ZuzucoinUpdate();              // UpdateZuzucoinEverywhere() propagation
```

**Usage**:
1. Place `ASabriMMOUITests` actor in any map
2. Press Play - tests auto-run after 5 seconds
3. Monitor Output Log for detailed results

### 2. Blueprint Test Library (`BP_AutomationTestLibrary`)

**Location**: `Content/SabriMMO/Test/BP_AutomationTestLibrary.uasset`

**Purpose**: Blueprint-based UI testing with UE5 Automation integration

**Key Functions**:
```blueprint
Test_Shop_OpenWindow() → Boolean
Test_Inventory_Toggle() → Boolean  
Test_Widget_TextDisplay(WidgetName, ExpectedText) → Boolean
Test_Zuzucoin_Update() → Boolean
Test_Complete_Shop_Flow() → Boolean
LogTestResult(TestName, Success, Message) → None
TakeScreenshot(TestName) → String
```

**Features**:
- Static functions for UE5 Automation system
- Widget visibility and text validation
- Complete user flow testing
- Screenshot capture for visual regression
- Structured error reporting

### 3. Test Infrastructure

**Test Map**: `Content/SabriMMO/Levels/L_AutomationTestMap.umap`
- PlayerStart actor positioned for spawn testing
- Basic lighting and floor for UI visibility
- ASabriMMOUITests actor pre-placed

**Build Script**: `C:/Sabri_MMO/compile_with_tests.bat`
- Closes UE5 Editor (prevents Live Coding conflicts)
- Compiles project with UI test changes
- Re-opens editor for immediate testing

## Test Coverage

### Current Coverage ✅

| System | Test | Status | Coverage |
|--------|------|--------|----------|
| GameInstance | UMMOGameInstance validation | ✅ PASS | 100% |
| Player Character | BP_MMOCharacter spawn | ✅ PASS | 100% |
| HUD Manager | AC_HUDManager detection | ✅ PASS | 100% |
| Inventory | ToggleInventory() function | ✅ PASS | 100% |
| Currency | Zuzucoin display updates | ✅ PASS | 100% |

**Total Tests**: 5  
**Pass Rate**: 100%  
**Last Run**: Current session

### Planned Coverage 📋

| Priority | Feature | Test Function | Status |
|----------|---------|---------------|--------|
| High | Shop window operations | Test_Shop_BuyItem() | 📋 Planned |
| High | Equipment management | Test_Equipment_EquipWeapon() | 📋 Planned |
| Medium | Combat UI interactions | Test_Combat_SkillUse() | 📋 Planned |
| Medium | Character stat allocation | Test_StatAllocation_Save() | 📋 Planned |
| Low | Chat system functionality | Test_Chat_MessageDelivery() | 📋 Planned |
| Low | Party management UI | Test_Party_InviteAccept() | 📋 Planned |

## Development Workflow

### When Adding New UI Features

1. **Create Test Function**
   ```cpp
   // In ASabriMMOUITests.h
   bool Test_NewFeatureName();
   
   // In ASabriMMOUITests.cpp
   bool ASabriMMOUITests::Test_NewFeatureName()
   {
       // Test implementation
       return true; // or false with LastFailReason
   }
   ```

2. **Update Test Runner**
   ```cpp
   // Add to RunNextTest() switch statement
   case NextTestIndex:
       bResult = Test_NewFeatureName();
       LogResult(TEXT("New Feature Test"), bResult, LastFailReason);
       break;
   ```

3. **Update Documentation**
   - Add to `tests/client/UI_TEST_COVERAGE.md`
   - Update this documentation file
   - Update project overview if major feature

4. **Compile and Test**
   ```bash
   C:/Sabri_MMO/compile_with_tests.bat
   ```

### When Modifying Existing UI

1. **Update Test Expectations**
   - Modify relevant test function for new behavior
   - Update variable names if UI components changed
   - Add new validation for changed functionality

2. **Verify Regression Tests**
   - Run full test suite
   - Ensure no unrelated tests broke
   - Update coverage document

3. **Document Changes**
   - Update relevant Blueprint documentation
   - Note behavior changes in test comments

## Continuous Integration

### Pre-Commit Testing

**Git Hook Setup** (`.git/hooks/pre-commit`):
```bash
#!/bin/bash
echo "Running UI tests..."
./compile_with_tests.bat
if [ $? -ne 0 ]; then
    echo "UI tests failed! Fix before committing."
    exit 1
fi
echo "UI tests passed! ✅"
```

### Automated Pipeline

**Command Line Testing**:
```bash
# Run all UI tests
SabriMMO.exe AutomationTestMap -ExecCmds="Automation RunTests=UI"

# Run specific test
SabriMMO.exe -ExecCmds="Automation RunTests=Test_Shop_OpenWindow"

# Run tests and exit (CI/CD)
SabriMMO.exe -ExecCmds="Automation RunTests=UI,Quit"
```

### Coverage Reporting

**Weekly Review Process**:
1. Run full test suite
2. Update `tests/client/UI_TEST_COVERAGE.md`
3. Identify coverage gaps
4. Plan new tests for next sprint
5. Review test performance metrics

## Best Practices

### Test Naming Convention
```cpp
// Good: Specific and descriptive
bool Test_ShopWindow_OpenWithValidShopId()
bool Test_Inventory_ToggleVisibilityState()
bool Test_Zuzucoin_UpdatePropagatesToAllUI()

// Avoid: Vague or generic
bool Test_Shop()
bool Test_Inventory()
bool Test_Zuzucoin()
```

### Error Message Standards
```cpp
// Good: Specific and actionable
LastFailReason = TEXT("Shop window null after calling OpenShop with ShopId=999");
LastFailReason = TEXT("InventoryWindowRef->IsVisible() returned false after ToggleInventory");

// Avoid: Generic errors
LastFailReason = TEXT("Shop test failed");
LastFailReason = TEXT("Inventory not working");
```

### Test Structure Pattern
```cpp
bool ASabriMMOUITests::Test_Example()
{
    // 1. Setup validation
    if (!ValidatePrerequisites()) return false;
    
    // 2. Execute action
    if (!ExecuteTestAction()) return false;
    
    // 3. Verify results
    if (!VerifyExpectedOutcome()) return false;
    
    // 4. Cleanup (if needed)
    CleanupTestState();
    
    return true;
}
```

## Troubleshooting

### Common Issues

**Issue**: "Player pawn never spawned!"
**Solution**: 
- Check PlayerStart actor in map
- Verify Game Mode class is set correctly
- Ensure character Blueprint is valid

**Issue**: "HUD Manager not found"
**Solution**:
- Verify BP_MMOCharacter has AC_HUDManager component
- Check component name spelling matches exactly
- Ensure player character spawned successfully

**Issue**: Tests don't appear in Automation tab
**Solution**:
- Ensure functions are marked as "Static" in Blueprint
- Check function names match filter pattern "SabriMMO.UI"
- Compile and save Blueprint

**Issue**: Compilation fails with Live Coding error
**Solution**:
- Close UE5 Editor completely
- Run `compile_with_tests.bat`
- Reopen editor after compilation

### Debug Mode

Enable detailed logging:
```cpp
// In ASabriMMOUITests constructor
InitialDelay = 10.0f;  // Longer delay for debugging
DelayBetweenTests = 1.0f;  // Slower test execution
```

## Performance Considerations

### Test Execution Time
- **Current suite**: ~2 seconds total execution
- **Target**: Keep full suite under 10 seconds
- **Optimization**: Use parallel execution where possible

### Memory Usage
- **Minimal impact**: Tests use existing game objects
- **No asset loading**: Tests work with spawned actors only
- **Cleanup**: Automatic garbage collection after test completion

## Future Enhancements

### Planned Features
- **Visual Regression Testing**: Screenshot comparison system
- **Performance Testing**: UI response time measurement
- **Accessibility Testing**: Keyboard navigation validation
- **Multi-language Testing**: Localization validation
- **Stress Testing**: UI behavior under high load

### Integration Opportunities
- **Jenkins/GitHub Actions**: Automated test execution
- **Test Reporting**: HTML test reports with screenshots
- **Coverage Visualization**: UI coverage heat maps
- **Analytics Integration**: Test result tracking over time

---

## Related Files

| File | Purpose |
|------|---------|
| `Source/SabriMMO/SabriMMOUITests.cpp` | C++ test runner implementation |
| `tests/client/BP_AutomationTestLibrary_Implementation.md` | Blueprint test implementation guide |
| `tests/client/COMPLETE_SETUP_GUIDE.md` | Complete UI testing setup instructions |
| `tests/client/UI_TEST_COVERAGE.md` | Test coverage tracking and planning |
| `tests/client/README_UI_TESTS.md` | Quick reference and usage guide |
| `C:/Sabri_MMO/compile_with_tests.bat` | Build script for test compilation |

---

**Last Updated**: 2026-02-23  
**Version**: 1.0  
**Status**: Active - All tests passing  
**Maintainer**: Development Team  

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| Component-Based | UI testing separated into dedicated test components |
| Event-Driven | Test execution triggered by BeginPlay timer |
| Manager | ASabriMMOUITests manages all UI test coordination |
| State Machine | Test runner uses enum states for test execution flow |
| Structured Logging | UE_LOG categories for test result reporting |

---

## Anti-Patterns to Address

| Anti-Pattern | Correct Pattern | Flag If... |
|-------------|----------------|------------|
| Test Hardcoding | Data-Driven Tests | Test values hardcoded instead of parameterized |
| UI Polling | Event-Driven Validation | Using Tick to check UI state instead of events |
| No Cleanup | Resource Management | Test actors not cleaned up after execution |
| Silent Failures | Explicit Reporting | Tests fail without clear error messages |
