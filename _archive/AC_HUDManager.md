# AC_HUDManager (Actor Component)

## Overview

Centralized HUD management component that handles all in-game widget creation, updates, and lifecycle. Part of the major refactor to use components and centralized widget management.

## Architecture

```
BP_MMOCharacter
    └── AC_HUDManager (Component)
            ├── Creates → WBP_GameHud
            ├── Creates → WBP_ChatBox
            ├── Creates → WBP_InventoryWindow
            ├── Creates → WBP_StatAllocation
            ├── Creates → WBP_DeathOverlay
            ├── Creates → WBP_LootPopup
            ├── Creates → WBP_DamageNumber
            └── Updates → WBP_GameHud (health/mana/target)
```

## Component Location

- **Type**: Actor Component
- **Owner**: BP_MMOCharacter
- **Name**: HUDManager

## Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `GameHUDRef` | WBP_GameHud (Object Reference) | Main HUD with health/mana bars |
| `ChatWidgetRef` | WBP_ChatBox (Object Reference) | Chat panel |
| `InventoryWindowRef` | WBP_InventoryWindow (Object Reference) | Inventory window |
| `StatAllocationRef` | WBP_StatAllocation (Object Reference) | Stat allocation panel |
| `DeathOverlayRef` | WBP_DeathOverlay (Object Reference) | Death screen overlay |
| `bIsInventoryOpen` | Boolean | Track inventory open state |
| `bIsStatWindowOpen` | Boolean | Track stat window open state |
| `SocketManagerRef` | BP_SocketManager (Object Reference) | Reference to socket manager for emitting events |

## Functions

### InitializeHUD

Creates and displays the main game HUD and chat widget on game start.

```
Function: InitializeHUD
    (No inputs)

    ↓
Create Widget (Class: WBP_GameHud)
    ↓
Add to Viewport
    ↓
Set GameHUDRef = Return Value
    ↓
Create Widget (Class: WBP_ChatBox)
    ↓
Add to Viewport (ZOrder: 10)
    ↓
Set ChatWidgetRef = Return Value
```

**Called From**: BP_MMOCharacter BeginPlay (after socket connection established)

---

### ShowDeathOverlay

Displays the death overlay when player dies.

```
Function: ShowDeathOverlay
    (No inputs)

    ↓
Create Widget (Class: WBP_DeathOverlay)
    ↓
Add to Viewport (ZOrder: -100)
    ↓
Set DeathOverlayRef = Return Value
```

**Called From**: BP_SocketManager OnCombatDeath handler (when local player dies)

---

### HideDeathOverlay

Hides/removes the death overlay.

```
Function: HideDeathOverlay
    (No inputs)

    ↓
Get DeathOverlayRef → Is Valid? → Branch
    ├─ TRUE:
    │   DeathOverlayRef → Remove from Parent
    │   Set DeathOverlayRef = None
    └─ FALSE: (do nothing)
```

---

### ToggleInventory

Toggles inventory window visibility. Creates window if not exists, toggles if exists.

```
Function: ToggleInventory
    (No inputs)

    ↓
Get bIsInventoryOpen → Branch
    ├─ TRUE (already open):
    │   Get InventoryWindowRef → Is Valid? → Branch
    │       ├─ TRUE:
    │       │   InventoryWindowRef → Remove from Parent
    │       │   Set InventoryWindowRef = None
    │       │   Set bIsInventoryOpen = false
    │       └─ FALSE: (do nothing)
    │
    └─ FALSE (not open):
        Create Widget (Class: WBP_InventoryWindow)
            ↓
        Add to Viewport (ZOrder: 100)
            ↓
        Set InventoryWindowRef = Return Value
            ↓
        Set bIsInventoryOpen = true
            ↓
        SocketManagerRef → Emit("inventory:load", {})
```

**Note**: Emits `inventory:load` to request inventory data from server when opening.

---

### ToggleStats

Toggles stat allocation window visibility.

```
Function: ToggleStats
    (No inputs)

    ↓
Get bIsStatWindowOpen → Branch
    ├─ TRUE (already open):
    │   Get StatAllocationRef → Is Valid? → Branch
    │       ├─ TRUE:
    │       │   StatAllocationRef → Remove from Parent
    │       │   Set StatAllocationRef = None
    │       │   Set bIsStatWindowOpen = false
    │       └─ FALSE: (do nothing)
    │
    └─ FALSE (not open):
        Create Widget (Class: WBP_StatAllocation)
            ↓
        Add to Viewport (ZOrder: 100)
            ↓
        Set StatAllocationRef = Return Value
            ↓
        Set bIsStatWindowOpen = true
            ↓
        SocketManagerRef → Emit("player:request_stats", {})
```

**Note**: Emits `player:request_stats` to request stat data from server when opening.

---

### ShowLootPopup

Creates a loot notification popup when enemies drop items.

```
Function: ShowLootPopup
    Input: Data (String) — JSON string with enemyName and items array

    ↓
Value From Json String (Data)
    ↓
As Object
    ↓
Try Get String Field ("enemyName") → EnemyName
Try Get Array Field ("items") → ItemsArray
    ↓
Create Widget (Class: WBP_LootPopup)
    ↓
Call ShowLoot (EnemyName, ItemsArray)
    ↓
Add to Viewport (ZOrder: 50)
```

**Called From**: BP_SocketManager OnLootDrop handler

---

### ShowDamageNumber

Creates floating damage text at target location.

```
Function: ShowDamageNumber
    Input: Damage (Integer), WorldPosition (Vector)

    ↓
Create Widget (Class: WBP_DamageNumber)
    ↓
Set DamageText = Damage.ToString
    ↓
Set DamageColor = [White for damage dealt, Red for damage taken]
    ↓
Add to Viewport (ZOrder: 200)
    ↓
[Optional: Animate upward and fade out over 1.5 seconds]
    ↓
Remove from Parent (after animation)
```

**Called From**: BP_SocketManager OnCombatDamage handler

---

### UpdatePlayerHealth

Updates the local player's health bar.

```
Function: UpdatePlayerHealth
    Input: Health (Integer), MaxHealth (Integer)

    ↓
Get GameHUDRef → Is Valid? → Branch
    ├─ TRUE:
    │   GameHUDRef → UpdateHealth(Health, MaxHealth)
    └─ FALSE: (buffer or ignore)
```

---

### UpdatePlayerMana

Updates the local player's mana bar.

```
Function: UpdatePlayerMana
    Input: Mana (Integer), MaxMana (Integer)

    ↓
Get GameHUDRef → Is Valid? → Branch
    ├─ TRUE:
    │   GameHUDRef → UpdateMana(Mana, MaxMana)
    └─ FALSE: (buffer or ignore)
```

---

### UpdateTargetHealth

Updates the target frame (enemy or player) in the HUD.

```
Function: UpdateTargetHealth
    Input: TargetName (String), Health (Integer), MaxHealth (Integer), Mana (Integer), MaxMana (Integer)

    ↓
Get GameHUDRef → Is Valid? → Branch
    ├─ TRUE:
    │   GameHUDRef → UpdateTargetHealth(TargetName, Health, MaxHealth, Mana, MaxMana)
    └─ FALSE: (buffer or ignore)
```

---

### HideTargetFrame

Hides the target frame in the HUD.

```
Function: HideTargetFrame
    (No inputs)

    ↓
Get GameHUDRef → Is Valid? → Branch
    ├─ TRUE:
    │   GameHUDRef → HideTargetFrame()
    └─ FALSE: (do nothing)
```

---

### RefreshInventory

Called when inventory:data is received from server to repopulate inventory slots.

```
Function: RefreshInventory
    Input: Data (String) — JSON string with items array

    ↓
Get InventoryWindowRef → Is Valid? → Branch
    ├─ TRUE:
    │   InventoryWindowRef → PopulateSlots(Data)
    └─ FALSE: (inventory window not open, ignore)
```

---

### UpdateStats

Called when player:stats is received to update stat allocation window.

```
Function: UpdateStats
    Input: Data (String) — JSON string with stats and derived objects

    ↓
Get StatAllocationRef → Is Valid? → Branch
    ├─ TRUE:
    │   StatAllocationRef → UpdateStats(Data)
    └─ FALSE: (stat window not open, ignore)
```

---

## Data Flow

### Inventory Equip Flow
```
[Player clicks slot button in WBP_InventorySlot]
    ↓
[WBP_InventorySlot::OnSlotAction Event Dispatcher fires] (InventoryID, bWantToEquip)
    ↓
[WBP_InventoryWindow::HandleSlotAction receives it]
    ↓
[Get Owning Player Pawn → Get AC_HUDManager component]
    ↓
[AC_HUDManager::SendEquipRequest] (InventoryID, bEquip)
    ↓
[SocketManagerRef → Emit "inventory:equip" {inventoryId, equip}]
    ↓ (network)
[Server validates, updates DB, recalculates stats]
    ↓
[Server sends back: inventory:data]
    ↓
[BP_SocketManager::OnInventoryData fires]
    ↓
[AC_HUDManager::RefreshInventory]
    ↓
[WBP_InventoryWindow::PopulateSlots — recreates all slots with fresh data]
```

### Stat Allocation Flow
```
[Player clicks + button in WBP_StatAllocation]
    ↓
[WBP_StatAllocation::OnIncrease_[STAT] emits event or calls function]
    ↓
[AC_HUDManager::SendStatAllocationRequest] (statName, amount)
    ↓
[SocketManagerRef → Emit "player:allocate_stat" {statName, amount}]
    ↓ (network)
[Server validates, updates DB, recalculates derived stats]
    ↓
[Server sends back: player:stats]
    ↓
[BP_SocketManager::OnPlayerStats fires]
    ↓
[AC_HUDManager::UpdateStats]
    ↓
[WBP_StatAllocation::UpdateStats — refreshes displayed values]
```

---

## Design Patterns Applied

| Pattern | How Applied |
|---------|-------------|
| Component-Based | AC_HUDManager is an Actor Component on BP_MMOCharacter, separate from character logic |
| Manager | Single centralized manager for all widget creation and updates |
| Event-Driven | Widgets communicate through AC_HUDManager functions, not direct socket bindings |
| Dependency Injection | SocketManagerRef passed explicitly, not fetched via GetAllActorsOfClass |

---

## Related Files

| File | Purpose |
|------|---------|
| `Content/Blueprints/BP_MMOCharacter.uasset` | Owner of AC_HUDManager component |
| `Content/Blueprints/BP_SocketManager.uasset` | Emits events that AC_HUDManager responds to |
| `docs/UI_Widgets.md` | All widget documentation |
| `docs/SocketIO_RealTime_Multiplayer.md` | Socket.io event reference |

---

**Last Updated**: 2026-02-17
**Version**: 1.0
**Status**: Complete — Major refactor with centralized widget management
