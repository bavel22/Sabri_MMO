# UEquipmentSubsystem

**Files**: `Source/SabriMMO/UI/EquipmentSubsystem.h` (106 lines), `EquipmentSubsystem.cpp` (290 lines)
**Parent**: `UWorldSubsystem`
**Purpose**: Equipment slot management with 10 RO Classic equip positions, F7 toggle, and dual-accessory support.

## Widget

`SEquipmentWidget` at Z-order 15. Created/destroyed on toggle (not persistent when hidden).

## Socket Events

### Incoming
| Event | Handler | Description |
|-------|---------|-------------|
| `inventory:data` | `RefreshEquippedSlots()` | Rebuilds equipment state from inventory data |

No outgoing events — delegates unequip to `InventorySubsystem::UnequipItem`.

## Equipment Slots (10 RO Classic positions)

| Slot | Constant |
|------|----------|
| Head Top | `head_top` |
| Head Mid | `head_mid` |
| Head Low | `head_low` |
| Armor | `armor` |
| Weapon | `weapon` |
| Shield | `shield` |
| Garment | `garment` |
| Footgear | `footgear` |
| Accessory 1 | `accessory_1` |
| Accessory 2 | `accessory_2` |

## Key Functions

| Function | Description |
|----------|-------------|
| `RefreshEquippedSlots()` | Rebuilds `EquippedSlots` map from InventorySubsystem items |
| `GetEquippedItem(SlotPosition)` | Returns item in a slot (or empty FInventoryItem) |
| `IsSlotOccupied(SlotPosition)` | Checks if slot has an item |
| `UnequipSlot(SlotPosition)` | Delegates to InventorySubsystem::UnequipItem |
| `CanEquipToSlot(ItemEquipSlot, SlotPosition)` | Validates slot compatibility (handles accessory dual-slot) |
| `ToggleWidget()` | F7 toggle, refreshes slots before showing |

## Data

`EquippedSlots`: `TMap<FString, FInventoryItem>` keyed by slot position string (e.g., `"weapon"`, `"accessory_1"`).

## Implementation Notes

- Does NOT parse inventory data directly — reads from `UInventorySubsystem::Items` when `inventory:data` fires
- Fallback: items without `EquippedPosition` fall back to `EquipSlot` (with "accessory" defaulting to `accessory_1`)
- `GetValidPositions("accessory")` returns both `accessory_1` and `accessory_2`
- `GetDisplayName(Position)` helper for UI-friendly slot names

---

**Last Updated**: 2026-03-09
