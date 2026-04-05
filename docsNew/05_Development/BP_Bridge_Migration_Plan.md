# BP_SocketManager Bridge Migration Plan — 14→0

**Created:** 2026-03-14
**Status:** Phases A-E COMPLETE + Phase F cleanup done — 0 bridges remaining. BP_SocketManager is fully dead code.
**Goal:** Remove all remaining BP_SocketManager bridges from MultiplayerEventSubsystem, making BP_SocketManager fully dead code
**Prerequisite:** Phases 1-6 of Blueprint-to-C++ migration complete

---

## Table of Contents

1. [Current State](#1-current-state)
2. [Bridge Analysis](#2-bridge-analysis)
3. [Migration Phases](#3-migration-phases)
4. [Phase A: Remove 10 Redundant Bridges](#4-phase-a-remove-10-redundant-bridges)
5. [Phase B: Add hotbar:data Handler](#5-phase-b-add-hotbardata-handler)
6. [Phase C: Add inventory:used Handler](#6-phase-c-add-inventoryused-handler)
7. [Phase D: Add loot:drop Handler](#7-phase-d-add-lootdrop-handler)
8. [Phase E: Migrate chat:receive](#8-phase-e-migrate-chatreceive)
9. [Phase F: Final Cleanup](#9-phase-f-final-cleanup)
10. [Server Event Payloads](#10-server-event-payloads)
11. [Testing Checklists](#11-testing-checklists)
12. [How to Start Implementation](#12-how-to-start-implementation)

---

## 1. Current State

`MultiplayerEventSubsystem` bridges 14 socket events to `BP_SocketManager` via `ForwardToBPHandler()` → `ProcessEvent()`. These BP functions call into `AC_HUDManager` which updates UMG widgets. However, **all UMG widgets have been replaced by C++ Slate subsystems** (Phases 1-6). The BP functions are updating dead UMG widgets.

### Files Involved
| File | Role |
|------|------|
| `UI/MultiplayerEventSubsystem.h` | Bridge subsystem header |
| `UI/MultiplayerEventSubsystem.cpp` | 14 bridge registrations (lines 58-96) |
| `BP_SocketManager` (Blueprint) | Handler shell — 14 active functions, SocketIO component disconnected |
| `AC_HUDManager` (Blueprint component) | Dead code — updates UMG widgets that no longer exist |

---

## 2. Bridge Analysis

### Bridges That Can Be Immediately Removed (C++ fully handles)

| # | Event | BP Function | Nodes | C++ Handlers |
|---|-------|------------|-------|-------------|
| 1 | `inventory:data` | OnInventoryData | 21 | InventorySubsystem, EquipmentSubsystem, BasicInfoSubsystem |
| 2 | `inventory:equipped` | OnItemEquipped | 65 | InventorySubsystem, EquipmentSubsystem |
| 3 | `inventory:dropped` | OnItemDropped | 4 | InventorySubsystem |
| 4 | `inventory:error` | OnInventoryError | 6 | InventorySubsystem |
| 5 | `player:stats` | OnPlayerStats | 45 | CombatStatsSubsystem, BasicInfoSubsystem, WorldHealthBarSubsystem |
| 6 | `hotbar:alldata` | OnHotbarAllData | 6 | HotbarSubsystem, SkillTreeSubsystem |
| 7 | `shop:data` | OnShopData | 9 | ShopSubsystem |
| 8 | `shop:bought` | OnShopBought | 20 | ShopSubsystem, BasicInfoSubsystem |
| 9 | `shop:sold` | OnShopSold | 23 | ShopSubsystem, BasicInfoSubsystem |
| 10 | `shop:error` | OnShopError | 26 | ShopSubsystem |

### Bridges That Need New C++ Handlers

| # | Event | BP Function | Nodes | What It Does | Migration Target |
|---|-------|------------|-------|-------------|-----------------|
| 11 | `hotbar:data` | OnHotbarData | 6 | Single slot update | HotbarSubsystem |
| 12 | `inventory:used` | OnItemUsed | 17 | Item use notification (HP healed, SP restored) | InventorySubsystem |
| 13 | `loot:drop` | OnLootDrop | 9 | Loot pickup notification (items from killed enemy) | InventorySubsystem |
| 14 | `chat:receive` | OnChatReceived | 9 | Chat message display | New ChatSubsystem (deferred) |

---

## 3. Migration Phases

| Phase | Scope | Effort | Risk |
|-------|-------|--------|------|
| **A** | Remove 10 redundant bridges | 10 min | Zero — C++ already handles everything |
| **B** | Add `hotbar:data` handler to HotbarSubsystem | 30 min | Low — simple single-slot parsing |
| **C** | Add `inventory:used` handler to InventorySubsystem | 30 min | Low — notification display |
| **D** | Add `loot:drop` handler to InventorySubsystem | 45 min | Low — loot notification |
| **E** | Create ChatSubsystem for `chat:receive` | 1-2 hrs | Medium — new subsystem + UI |
| **F** | Final cleanup: delete BP_SocketManager functions, AC_HUDManager, MultiplayerEventSubsystem | 30 min | Low — delete dead code |

**Total bridges after each phase:** A→4, B→3, C→2, D→1, E→0, F→clean

---

## 4. Phase A: Remove 10 Redundant Bridges

### What to Do
Remove 10 event registrations from `MultiplayerEventSubsystem::OnWorldBeginPlay()`. These events are already fully handled by C++ subsystems. The BP functions update dead UMG widgets via AC_HUDManager.

### Files to Modify
| File | Change |
|------|--------|
| `UI/MultiplayerEventSubsystem.cpp` | Remove 10 `Router->RegisterHandler()` calls |
| `UI/MultiplayerEventSubsystem.h` | Update header comment (14→4 bridges) |

### Events to Remove
```
inventory:data       → OnInventoryData      (C++: InventorySubsystem + EquipmentSubsystem + BasicInfoSubsystem)
inventory:equipped   → OnItemEquipped       (C++: InventorySubsystem + EquipmentSubsystem)
inventory:dropped    → OnItemDropped        (C++: InventorySubsystem)
inventory:error      → OnInventoryError     (C++: InventorySubsystem)
player:stats         → OnPlayerStats        (C++: CombatStatsSubsystem + BasicInfoSubsystem + WorldHealthBarSubsystem)
hotbar:alldata       → OnHotbarAllData      (C++: HotbarSubsystem + SkillTreeSubsystem)
shop:data            → OnShopData           (C++: ShopSubsystem)
shop:bought          → OnShopBought         (C++: ShopSubsystem + BasicInfoSubsystem)
shop:sold            → OnShopSold           (C++: ShopSubsystem + BasicInfoSubsystem)
shop:error           → OnShopError          (C++: ShopSubsystem)
```

### Events Remaining After Phase A (4)
```
inventory:used   → OnItemUsed       (NO C++ handler)
loot:drop        → OnLootDrop       (NO C++ handler)
hotbar:data      → OnHotbarData     (NO C++ handler)
chat:receive     → OnChatReceived   (NO C++ handler)
```

### Testing
- [ ] Inventory opens (F6), shows items correctly
- [ ] Equipping/unequipping items works
- [ ] Dropping items works
- [ ] Stat panel (F8) updates correctly
- [ ] Hotbar shows all slots on login
- [ ] NPC shop buy/sell works
- [ ] HP/SP/EXP bars show correctly

---

## 5. Phase B: Add hotbar:data Handler

### What to Do
Add `hotbar:data` handler to `HotbarSubsystem`. This handles single-slot updates (when the server confirms a save/clear for one slot).

Currently `HotbarSubsystem` only handles `hotbar:alldata` (full refresh). The server sends `hotbar:data` for individual slot confirmations, but the C++ subsystem ignores it. The BP bridge forwards it to BP_SocketManager which updates the old UMG hotbar.

### Server Payload (hotbar:data)
```json
{
  "row_index": 0,
  "slot_index": 3,
  "slot_type": "skill",
  "skill_id": 28,
  "skill_name": "Heal"
}
```
Or for items:
```json
{
  "row_index": 0,
  "slot_index": 5,
  "slot_type": "item",
  "inventory_id": 123,
  "item_id": 501,
  "item_name": "Red Potion",
  "quantity": 10
}
```
Or for clear:
```json
{
  "row_index": 0,
  "slot_index": 3,
  "slot_type": "empty"
}
```

### Files to Modify
| File | Change |
|------|--------|
| `UI/HotbarSubsystem.cpp` | Add `HandleHotbarData()` handler + register for `hotbar:data` |
| `UI/MultiplayerEventSubsystem.cpp` | Remove `hotbar:data` bridge |

### Implementation Notes
- Parse `row_index`, `slot_index`, `slot_type` from JSON
- If `slot_type == "skill"`: update the slot with skill data
- If `slot_type == "item"`: update the slot with item data
- If `slot_type == "empty"`: clear the slot
- Follow existing `HandleHotbarAllData()` parsing pattern but for a single slot

### Testing
- [ ] Drag item to hotbar → slot updates
- [ ] Drag skill to hotbar → slot updates
- [ ] Clear hotbar slot → slot clears
- [ ] Hotbar persists across zone transitions

---

## 6. Phase C: Add inventory:used Handler

### What to Do
Add `inventory:used` handler to `InventorySubsystem`. This handles item consumption notifications (potions, Fly Wing, Butterfly Wing).

### Server Payload (inventory:used)
```json
{
  "inventoryId": 123,
  "itemId": 501,
  "itemName": "Red Potion",
  "healed": 75,
  "spRestored": 0,
  "health": 450,
  "maxHealth": 500,
  "mana": 200,
  "maxMana": 200
}
```
Sent for: consumable use (potions/food), Fly Wing, Butterfly Wing.

### Files to Modify
| File | Change |
|------|--------|
| `UI/InventorySubsystem.cpp` | Add `HandleItemUsed()` handler + register for `inventory:used` |
| `UI/MultiplayerEventSubsystem.cpp` | Remove `inventory:used` bridge |

### Implementation Notes
- Parse `inventoryId`, `itemName`, `healed`, `spRestored`
- Request a full inventory refresh (`inventory:load`) to update quantities
- Log the usage: `UE_LOG(LogInventory, Log, TEXT("Used %s — healed %d, SP restored %d"), ...)`
- Future: show floating text notification (can be added later)
- The `health`/`maxHealth`/`mana`/`maxMana` fields are redundant — `combat:health_update` is also sent by the server for HP/SP changes

### Testing
- [ ] Use Red Potion → HP restores, inventory quantity decreases
- [ ] Use Blue Potion → SP restores
- [ ] Use Fly Wing → teleport works, item consumed
- [ ] Use Butterfly Wing → teleport to save point

---

## 7. Phase D: Add loot:drop Handler

### What to Do
Add `loot:drop` handler to `InventorySubsystem`. This handles loot pickup notifications when an enemy dies.

### Server Payload (loot:drop)
```json
{
  "enemyId": 2000001,
  "enemyName": "Poring",
  "items": [
    {
      "itemId": 501,
      "itemName": "Red Potion",
      "quantity": 2,
      "icon": "red_potion",
      "itemType": "consumable"
    },
    {
      "itemId": 909,
      "itemName": "Jellopy",
      "quantity": 3,
      "icon": "jellopy",
      "itemType": "etc"
    }
  ]
}
```
Sent to: killer socket only (after enemy death, loot generation, items added to DB).
The server ALSO sends `inventory:data` immediately after `loot:drop`, so the inventory is already refreshed.

### Files to Modify
| File | Change |
|------|--------|
| `UI/InventorySubsystem.cpp` | Add `HandleLootDrop()` handler + register for `loot:drop` |
| `UI/MultiplayerEventSubsystem.cpp` | Remove `loot:drop` bridge |

### Implementation Notes
- Parse `enemyName` and `items` array
- Log each item: `UE_LOG(LogInventory, Log, TEXT("Looted %s x%d from %s"), ...)`
- Future: show loot popup widget (can be added later as a Slate widget)
- No need to refresh inventory — server already sends `inventory:data` after `loot:drop`

### Testing
- [ ] Kill monster → loot notification logged
- [ ] Looted items appear in inventory
- [ ] Multiple items from one kill all show up

---

## 8. Phase E: Migrate chat:receive — COMPLETE (2026-03-14)

### What Was Built
Created `UChatSubsystem` (UWorldSubsystem) with inline `SChatWidget` at Z=13.

### Server Payload (chat:receive)
```json
{
  "senderName": "player4",
  "message": "Hello world",
  "channel": "GLOBAL"
}
```
**Note:** Server uses `senderName` (NOT `characterName`). Channels are UPPERCASE (`GLOBAL`, `SYSTEM`, `COMBAT`).

### Architecture
| File | Purpose |
|------|---------|
| `UI/ChatSubsystem.h` | UWorldSubsystem — EChatChannel enum, FChatMessage struct, message storage |
| `UI/ChatSubsystem.cpp` | Handle `chat:receive` + 8 combat log events, SChatWidget (inline), SendChatMessage() |

### What Was Implemented
- RO brown/gold themed chat window, bottom-left, draggable
- 3 filter tabs: All, System, Combat
- Scrollable message log (SScrollBox), input field (SEditableTextBox)
- Enter to send, local echo for own messages
- Channel colors: Normal=cream, Party=blue, Guild=green, Whisper=pink, System=yellow, Combat=orange-red
- MAX_MESSAGES = 100, auto-scroll to bottom
- Full combat log: 8 event handlers (combat:damage, skill:effect_damage, status:applied/removed, skill:buff_applied/removed, combat:death, combat:respawn)
- Combat log filters by LocalCharacterId (only shows local player events)

### Testing
- [x] Send message → appears in chat window
- [x] Receive message from other player → appears in chat
- [x] Combat events appear in Combat tab
- [x] Tab filtering works (All/System/Combat)
- [ ] Chat bubbles above characters (future)
- [ ] Channel switching (future)

---

## 9. Phase F: Final Cleanup — COMPLETE (2026-03-14)

### What Was Done

#### Removed from C++
| File | Change |
|------|--------|
| `UI/MultiplayerEventSubsystem.h` | Removed `ForwardToBPHandler()`, `JsonValueToString()`, `SocketManagerActor`, `FindSocketManagerActor()`, `bReadyToForward` |
| `UI/MultiplayerEventSubsystem.cpp` | Removed all bridge infrastructure. Only `EmitCombatAttack()`, `EmitStopAttack()`, `RequestRespawn()`, `EmitChatMessage()` remain |

#### Blueprint Cleanup (user does in editor)
1. Delete remaining BP_SocketManager functions (OnItemUsed, OnLootDrop, OnHotbarData, OnChatReceived)
2. Delete AC_HUDManager component from BP_MMOCharacter
3. Delete BP_SocketManager actor from all levels (it's now fully dead)

#### Rename Subsystem (optional, deferred)
Consider renaming `MultiplayerEventSubsystem` to `SocketEmitSubsystem` since it only handles outbound emits, not inbound event bridging.

---

## 10. Server Event Payloads

### inventory:used
```json
{ "inventoryId": 123, "itemId": 501, "itemName": "Red Potion",
  "healed": 75, "spRestored": 0,
  "health": 450, "maxHealth": 500, "mana": 200, "maxMana": 200 }
```

### loot:drop
```json
{ "enemyId": 2000001, "enemyName": "Poring",
  "items": [
    { "itemId": 501, "itemName": "Red Potion", "quantity": 2, "icon": "red_potion", "itemType": "consumable" }
  ] }
```

### hotbar:data (single slot)
```json
{ "row_index": 0, "slot_index": 3, "slot_type": "skill",
  "skill_id": 28, "skill_name": "Heal" }
```

### chat:receive
```json
{ "characterId": 29, "characterName": "player4",
  "message": "Hello world", "channel": "normal" }
```

---

## 11. Testing Checklists

### After Phase A (remove 10 bridges)
- [ ] Login → enter game world (no crash)
- [ ] Inventory (F6) opens and shows items
- [ ] Equip/unequip weapon → equipment panel updates
- [ ] Drop item → inventory updates
- [ ] Stat panel (F8) shows correct values
- [ ] Hotbar loads all slots on login
- [ ] NPC shop: buy item → inventory + zeny update
- [ ] NPC shop: sell item → inventory + zeny update
- [ ] HP/SP/EXP bars correct on login and zone transition
- [ ] Zone transition → all UI rebuilds correctly

### After Phase B (hotbar:data)
- [ ] Drag item to hotbar slot → confirms
- [ ] Drag skill to hotbar slot → confirms
- [ ] Clear hotbar slot → clears
- [ ] Hotbar state persists across zone transitions

### After Phase C (inventory:used)
- [ ] Use potion → quantity decreases, HP/SP restores
- [ ] Use Fly Wing → teleports, item consumed
- [ ] Use Butterfly Wing → teleports to save point

### After Phase D (loot:drop)
- [ ] Kill enemy → log shows loot items
- [ ] Items appear in inventory after kill

### After Phase E (chat:receive)
- [ ] Chat messages appear in chat window
- [ ] Messages from other players visible
- [ ] Enter key opens/sends chat

---

## 12. How to Start Implementation

### Starting a New Conversation

Say this to Claude Code:

```
Load skills /sabrimmo-persistent-socket and /sabrimmo-ui then read the BP bridge migration plan at docsNew/05_Development/BP_Bridge_Migration_Plan.md. Implement Phase A (remove 10 redundant bridges from MultiplayerEventSubsystem). Build with SabriMMOEditor target. Then proceed to Phase B.
```

### If Continuing After Phase A+B

```
Read docsNew/05_Development/BP_Bridge_Migration_Plan.md. Phases A and B are complete. Implement Phase C (inventory:used handler) then Phase D (loot:drop handler). Build with SabriMMOEditor target.
```

### If Starting Phase E (Chat)

```
Load skills /sabrimmo-ui and /sabrimmo-persistent-socket then read docsNew/05_Development/BP_Bridge_Migration_Plan.md. Implement Phase E (ChatSubsystem for chat:receive). This requires a new UWorldSubsystem + Slate widget. Follow the RO Classic chat style.
```

### Key Rules for Implementation
1. **ALWAYS build with `SabriMMOEditor` target** (NOT `SabriMMO`)
2. **NEVER use InitializeStruct/DestroyStruct on FMemory_Alloca** — use Memzero + FString placement new
3. **Close editor before building** — Live Coding causes crashes on header changes
4. **Test after each phase** before proceeding to the next
5. **Update CLAUDE.md** bridge count after each phase

### Files Reference
| File | Location |
|------|----------|
| MultiplayerEventSubsystem | `client/SabriMMO/Source/SabriMMO/UI/MultiplayerEventSubsystem.h/.cpp` |
| HotbarSubsystem | `client/SabriMMO/Source/SabriMMO/UI/HotbarSubsystem.h/.cpp` |
| InventorySubsystem | `client/SabriMMO/Source/SabriMMO/UI/InventorySubsystem.h/.cpp` |
| ShopSubsystem | `client/SabriMMO/Source/SabriMMO/UI/ShopSubsystem.h/.cpp` |
| CombatStatsSubsystem | `client/SabriMMO/Source/SabriMMO/UI/CombatStatsSubsystem.h/.cpp` |
| BasicInfoSubsystem | `client/SabriMMO/Source/SabriMMO/UI/BasicInfoSubsystem.h/.cpp` |
| Build skill | `/sabrimmo-build-compile` |
| Migration status | `docsNew/05_Development/Blueprint_To_CPP_Migration/08_Migration_Status_And_Next_Phase.md` |
