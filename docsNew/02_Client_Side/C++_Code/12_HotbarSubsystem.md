# UHotbarSubsystem

**Files**: `Source/SabriMMO/UI/HotbarSubsystem.h` (205 lines), `HotbarSubsystem.cpp` (908 lines)
**Parent**: `UWorldSubsystem`
**Purpose**: 4-row × 9-slot hotbar with item/skill assignment, customizable keybinds, and F5 visibility cycling.

## Widgets

- `SHotbarRowWidget` × 4 (one per row) at Z-order 16
- `SHotbarKeybindWidget` at Z-order 30 (keybind config panel)

## Socket Events

### Incoming
| Event | Handler | Description |
|-------|---------|-------------|
| `hotbar:alldata` | Populates all slots | Server sends full 4×9 hotbar state |

### Outgoing
| Event | Payload | Description |
|-------|---------|-------------|
| `hotbar:request` | `{ characterId }` | Request hotbar data (1s delayed after wrap) |
| `hotbar:save` | `{ characterId, slotIndex, inventoryId, itemId, itemName, rowIndex }` | Save item to slot |
| `hotbar:save_skill` | `{ characterId, slotIndex, skillId, skillName, rowIndex, zeroBased: true }` | Save skill to slot |
| `hotbar:clear` | `{ characterId, slotIndex, rowIndex }` | Clear a slot |

## Constants

- `NUM_ROWS`: 4
- `SLOTS_PER_ROW`: 9

## Visibility

`EHotbarVisibility` cycles via F5: `OneRow` → `TwoRows` → `ThreeRows` → `FourRows` → `Hidden` → `OneRow`

## Key Functions

| Function | Description |
|----------|-------------|
| `CycleVisibility()` | F5 toggle through visibility states |
| `AssignItem(Row, Slot, Item)` | Put inventory item in slot, emits `hotbar:save` |
| `AssignSkill(Row, Slot, SkillId, Name, Icon)` | Put skill in slot, emits `hotbar:save_skill` |
| `ClearSlot(Row, Slot)` | Clear slot, emits `hotbar:clear` |
| `ActivateSlot(Row, Slot)` | Uses the skill (via SkillTreeSubsystem) or item (via InventorySubsystem) |
| `HandleNumberKey(KeyNumber, bAlt, bCtrl, bShift)` | Called from SabriMMOCharacter, scans keybinds for match |
| `RefreshItemQuantities()` | Syncs quantities from InventorySubsystem, auto-clears removed items |
| `ToggleKeybindWidget()` | Opens/closes keybind config panel |
| `SaveKeybinds/LoadKeybinds/ResetKeybindsToDefaults` | Keybind persistence to GameUserSettings.ini |

## Data Structures

### FHotbarSlot
```
SlotType (item/skill), ItemId, InventoryId, SkillId, Name, Icon, Quantity
```

### FHotbarKeybind
```
Key (FKey), bAlt, bCtrl, bShift
```

## Default Keybinds

| Row | Keys |
|-----|------|
| 0 | 1, 2, 3, 4, 5, 6, 7, 8, 9 |
| 1 | Alt+1 through Alt+9 |
| 2-3 | Unbound |

## Cross-Subsystem Dependencies

- Icons from `InventorySubsystem` and `SkillTreeSubsystem`
- Activates via `InventorySubsystem::UseItem` / `SkillTreeSubsystem::UseSkill`
- PIE-safe: caches `UGameViewportClient*` via `World->GetGameViewport()`
- `DataVersion` counter + `FOnHotbarDataUpdated` delegate for widget refresh

---

**Last Updated**: 2026-03-09
