# Inventory Performance Analysis — Root Cause Report

**Date:** 2026-04-10
**Symptom:** Significant game lag when a player has many inventory items
**Verdict:** Multiple cascading bottlenecks on both server and client. The core issue is that **every inventory mutation sends the entire inventory (all items, all 62 columns, all descriptions) from scratch via a fresh DB query, and the client destroys and rebuilds every widget from scratch on every update.**

---

## Root Cause Summary

| # | Layer | Issue | Severity |
|---|-------|-------|----------|
| 1 | Server | `getPlayerInventory()` fetches ALL items (62 columns each) from DB on every single change | CRITICAL |
| 2 | Server | 53 places in index.js emit `inventory:data` with the full payload | CRITICAL |
| 3 | Server | `full_description` (500+ chars) sent for every item AND every compounded card, every time | CRITICAL |
| 4 | Server | `calculatePlayerCurrentWeight()` runs a fresh DB query (JOIN + SUM) on every change — no caching | HIGH |
| 5 | Client | `RebuildGrid()` destroys ALL widgets and recreates them from scratch on every `DataVersion` change | CRITICAL |
| 6 | Client | `GetItemAtSlot()` calls `GetFilteredItems()` which copies the entire items array — called 2x per slot in per-frame lambdas | CRITICAL |
| 7 | Client | `GetFilteredItems()` returns `TArray<FInventoryItem>` by value (deep copy of ~300 bytes per item) | HIGH |
| 8 | Client | `FindItemByInventoryId()` is O(N) linear search — no TMap lookup | HIGH |
| 9 | Client | Hotbar polls `RefreshItemQuantities()` every 1 second, doing 36 x O(N) linear searches | MEDIUM |
| 10 | Client | All tooltips pre-built during grid rebuild (not lazy on hover) | LOW |

---

## Detailed Findings

### SERVER FINDING 1: Full Inventory Fetch on Every Change

**File:** `server/src/index.js:5373-5428`

```javascript
async function getPlayerInventory(characterId) {
    const result = await pool.query(
        `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
                ci.equipped_position, ci.refine_level, ci.compounded_cards,
                ci.forged_by, ci.forged_element, ci.forged_star_crumbs, ci.identified,
                i.name, i.description, i.full_description, i.item_type, i.equip_slot,
                i.weight, i.price, i.buy_price, i.sell_price,
                i.atk, i.def, i.matk, i.mdef,
                i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus, i.perfect_dodge_bonus,
                i.max_hp_rate, i.max_sp_rate, i.aspd_rate,
                i.hp_regen_rate, i.sp_regen_rate, i.crit_atk_rate, i.cast_rate, i.use_sp_rate, i.heal_power,
                i.required_level, i.stackable, i.icon,
                i.weapon_type, i.aspd_modifier, i.weapon_range,
                i.slots, i.weapon_level, i.refineable, i.jobs_allowed,
                i.card_type, i.card_prefix, i.card_suffix,
                i.two_handed, i.element, i.view_sprite
         FROM character_inventory ci
         JOIN items i ON ci.item_id = i.item_id
         WHERE ci.character_id = $1
         ORDER BY ci.slot_index ASC, ci.created_at ASC`, [characterId]);
    // ... post-processing adds compounded_card_details (9 fields per card) ...
}
```

**Problem:** This queries ALL 62 columns for ALL items every time. For a player with 200 items, that's 12,400 column values fetched from PostgreSQL, serialized to JSON, and sent over the wire — just to reflect that one potion was consumed.

**Scale:** Called from **53+ locations** throughout the codebase. Every equip, unequip, use, buy, sell, drop, pickup, refine, card compound, trade, storage deposit/withdraw, craft, and quest reward triggers this.

---

### SERVER FINDING 2: Description Bloat in Payload

**File:** `server/src/index.js:5379, 5410-5411`

Every `inventory:data` emission includes:
- `i.description` — short description (50-100 chars per item)
- `i.full_description` — full tooltip text (200-800 chars per item)
- For each compounded card: `cardDef.description` + `cardDef.full_description`

**Payload estimate with 200 items (30 with cards):**
- Item descriptions: 200 items x ~400 chars avg = 80,000 chars
- Card descriptions: 30 items x 2 cards avg x ~400 chars = 24,000 chars
- Other fields: 200 items x ~300 chars = 60,000 chars
- **Total: ~164,000 characters (~164 KB) per `inventory:data` emission**

This data is static — descriptions never change at runtime. Yet they're re-fetched and re-sent 53 times per session.

---

### SERVER FINDING 3: Weight Recalculation via DB Query

**File:** `server/src/index.js:5081-5093`

```javascript
async function calculatePlayerCurrentWeight(characterId) {
    const result = await pool.query(
        `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as total_weight
         FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
         WHERE ci.character_id = $1`, [characterId]);
    return parseInt(result.rows[0].total_weight) || 0;
}
```

**Problem:** This is a separate DB query (with JOIN) that scans every row in `character_inventory` for the player. Called via `updatePlayerWeightCache()` on nearly every inventory mutation — often right before or after `getPlayerInventory()` which already has the weight data in its result set.

The `updatePlayerWeightCache()` function (line 5098) always calls this, then calls `emitWeightStatus()` which does another socket emit.

**Result:** 2 DB queries (getPlayerInventory + calculatePlayerCurrentWeight) + 2 socket emits (inventory:data + weight:status) for a single item change.

---

### CLIENT FINDING 4: Full Widget Rebuild Every Update

**File:** `SInventoryWidget.cpp:132-146, 324-373`

```cpp
// Tick — every frame
void SInventoryWidget::Tick(...) {
    if (CurrentVersion != LastDataVersion || CurrentTabId != LastTabId) {
        RebuildGrid();  // Full rebuild triggered
    }
}

// RebuildGrid — destroys everything, creates everything
void SInventoryWidget::RebuildGrid() {
    GridContainer->ClearChildren();  // DESTROY all widgets
    TArray<FInventoryItem> FilteredItems = Sub->GetFilteredItems();  // O(N) copy
    for (Row...) {
        for (Col...) {
            RowBox->AddSlot()[ BuildItemSlot(ItemIndex) ];  // NEW widgets per slot
        }
    }
}
```

**Problem:** Every `DataVersion` increment (every `inventory:data` from server) triggers a complete teardown and rebuild of the entire inventory grid. With 50 visible slots:
- `ClearChildren()` destroys ~350 widgets (7 per slot)
- Creates ~350 new widgets via `BuildItemSlot()`
- Each `BuildItemSlot()` creates: SBox, SBorder, SBorder, SOverlay, SBox+SImage, STextBlock(qty), STextBlock(?), SToolTip = ~8 widgets

This causes a visible frame hitch every time the inventory updates.

---

### CLIENT FINDING 5: GetItemAtSlot() Copies Entire Filtered Array

**File:** `SInventoryWidget.cpp:655-664`

```cpp
FInventoryItem SInventoryWidget::GetItemAtSlot(int32 SlotIndex) const {
    TArray<FInventoryItem> Filtered = Sub->GetFilteredItems();  // FULL COPY every call
    if (SlotIndex >= 0 && SlotIndex < Filtered.Num())
        return Filtered[SlotIndex];  // Also copies the item
    return FInventoryItem();
}
```

**Called from:**
1. `BuildItemSlot()` line 382 — once per slot during rebuild
2. Quantity lambda line 449 — **per-frame** for each visible slot
3. Unidentified "?" lambda line 464 — **per-frame** for each visible slot

**Impact with 200 items, 50 visible slots:**
- Per rebuild: 50 calls x GetFilteredItems() = 50 x (200 items x ~300 bytes copied) = **3 MB of allocations**
- Per frame (lambdas): 100 calls x GetFilteredItems() = **6 MB of allocations per frame** at 60 FPS = **360 MB/sec of garbage**

This is the single biggest performance killer on the client.

---

### CLIENT FINDING 6: GetFilteredItems() Returns by Value

**File:** `InventorySubsystem.cpp:887-914`

```cpp
TArray<FInventoryItem> UInventorySubsystem::GetFilteredItems() const {
    TArray<FInventoryItem> Filtered;
    for (const FInventoryItem& Item : Items) {
        if (Item.bIsEquipped) continue;
        // tab + search filter...
        Filtered.Add(Item);  // Deep copy of ~300-byte struct with 13 FStrings
    }
    return Filtered;  // Returns by value — another copy
}
```

**FInventoryItem struct** (`CharacterData.h:63-121`) contains:
- 13 FString fields (Name, Description, FullDescription, ItemType, EquipSlot, EquippedPosition, Icon, WeaponType, JobsAllowed, CardType, CardPrefix, CardSuffix, Element)
- 2 TArray fields (CompoundedCards, CompoundedCardDetails)
- 25+ int32/bool fields
- **Estimated size: ~300+ bytes per item** (not counting heap-allocated FString data)

Every `GetFilteredItems()` call deep-copies every matching item including all string heap allocations.

---

### CLIENT FINDING 7: No TMap for Item Lookup

**File:** `InventorySubsystem.cpp:677-684, InventorySubsystem.h:42-47`

```cpp
FInventoryItem* UInventorySubsystem::FindItemByInventoryId(int32 InventoryId) {
    for (FInventoryItem& Item : Items) {  // O(N) linear scan
        if (Item.InventoryId == InventoryId) return &Item;
    }
    return nullptr;
}
```

The subsystem stores items only in `TArray<FInventoryItem> Items` — no `TMap<int32, int32>` index from InventoryId to array position. Every lookup is O(N).

---

### CLIENT FINDING 8: Hotbar Polling Pattern

**File:** `HotbarSubsystem.cpp:827-865` (called every 1.0s from SHotbarRowWidget::Tick)

```cpp
void UHotbarSubsystem::RefreshItemQuantities() {
    for (r = 0..NUM_ROWS) {
        for (s = 0..SLOTS_PER_ROW) {  // Up to 36 slots
            FInventoryItem* Item = InvSub->FindItemByInventoryId(Slot.InventoryId);  // O(N) each
        }
    }
}
```

With 200 items and 36 hotbar slots: 7,200 comparisons per second, continuously.

---

## Cascade Effect: What Happens When You Use 1 Potion

1. **Server:** `inventory:use` handler consumes the item
2. **Server:** `removeItemFromInventory()` — DB UPDATE to decrease quantity
3. **Server:** `getPlayerInventory(characterId)` — fresh DB query, 62 columns x ALL items
4. **Server:** Post-process compounded card details for every item
5. **Server:** JSON.stringify the entire result (~164 KB for 200 items)
6. **Server:** `socket.emit('inventory:data', { items: ALL_ITEMS, ... })` — sends 164 KB
7. **Server:** `updatePlayerWeightCache()` — ANOTHER DB query (SUM + JOIN)
8. **Server:** `emitWeightStatus()` — another socket emit
9. **Client:** JSON parsing of 164 KB on game thread
10. **Client:** `HandleInventoryData()` — `Items.Empty()` then re-parse ALL items
11. **Client:** `++DataVersion`
12. **Client:** Next Tick: `RebuildGrid()` — destroy 350 widgets, create 350 new ones
13. **Client:** During rebuild: 50 calls to `GetItemAtSlot()` = 50 copies of full filtered array
14. **Client:** Per frame after: 100 lambda calls to `GetItemAtSlot()` = 100 more copies

**Total cost for using 1 potion:**
- 2 DB queries
- ~164 KB network transfer
- 350 widget destructions + 350 widget creations
- Hundreds of MB in temporary allocations from array copying

---

## Optimization Plan

### Phase 1: Client — Stop the Per-Frame Hemorrhaging (Highest Impact)

**1A. Cache filtered items in InventorySubsystem**
- Add `TArray<FInventoryItem> CachedFilteredItems` and `uint32 FilteredCacheVersion`
- Only recompute when `DataVersion`, `CurrentTab`, or `SearchFilter` change
- Change `GetFilteredItems()` to return `const TArray<FInventoryItem>&` (reference, not copy)
- **Impact:** Eliminates hundreds of MB/sec of garbage allocations
- **Risk:** None — pure optimization, same data

**1B. Fix GetItemAtSlot() to use cached array**
- Index into the cached filtered array directly instead of calling `GetFilteredItems()`
- Return `const FInventoryItem*` (pointer) instead of copying the whole struct
- Update lambdas to capture the index and read from cache
- **Impact:** Eliminates O(N) per-slot per-frame cost
- **Risk:** None — same behavior, fewer copies

**1C. Add TMap<int32, int32> InventoryIdToIndex**
- Build alongside Items array in `HandleInventoryData()`
- Change `FindItemByInventoryId()` to use TMap::Find — O(1) instead of O(N)
- **Impact:** Hotbar refresh goes from 7,200 comparisons/sec to 36 hash lookups/sec
- **Risk:** None — same lookup result

### Phase 2: Client — Reduce Widget Churn

**2A. Delta-update grid instead of full rebuild**
- Track which items changed (compare old vs new Items arrays by InventoryId)
- Only update changed slots (icon, quantity text, tooltip) instead of ClearChildren + rebuild
- Keep widget references in a `TArray<TSharedPtr<SBox>> SlotWidgets` for direct updates
- **Impact:** Eliminates 700 widget alloc/dealloc per update
- **Risk:** Low — need to handle slot count changes (add/remove rows)
- **Fallback:** If delta is too complex, at minimum avoid rebuilding slots whose item hasn't changed

**2B. Lazy tooltips**
- Don't create SToolTip widgets during BuildItemSlot()
- Create tooltip only on hover (SetToolTip in OnMouseEnter or OnHovered)
- **Impact:** Eliminates 50 tooltip widget allocations per rebuild
- **Risk:** None — tooltip appears same moment

### Phase 3: Server — Reduce Payload Size

**3A. Strip `description` and `full_description` from `inventory:data`**
- These are static per item_id — they never change at runtime
- Send them once on initial `inventory:load` (or let client cache by item_id)
- On subsequent emissions, only send: `inventory_id, item_id, quantity, is_equipped, equipped_position, slot_index, refine_level, compounded_cards, forged_*, identified`
- **Impact:** Reduces payload from ~164 KB to ~20 KB for 200 items (88% reduction)
- **Risk:** Low — client already has item definitions from first load. Need a separate `itemDefs:data` cache on client or send full only on `inventory:load`, delta on mutations.

**3B. Strip card `description`/`full_description` from compounded_card_details**
- Only send `item_id, name, card_prefix, card_suffix` per card
- Client can look up descriptions from its own item definition cache
- **Impact:** Further ~15% payload reduction
- **Risk:** None — client already has this data

**3C. Only send changed items (delta sync) — STRETCH GOAL**
- Instead of full inventory, send `{ added: [...], removed: [...], updated: [...] }`
- Client applies delta to its local Items array
- This is a larger change — keep full sync as fallback for `inventory:load`
- **Impact:** Reduces 164 KB to ~1 KB for single-item changes
- **Risk:** Medium — need to ensure client and server stay in sync; version numbers help

### Phase 4: Server — Reduce DB Query Overhead

**4A. Maintain weight incrementally**
- When adding an item: `player.currentWeight += quantity * itemDef.weight`
- When removing: `player.currentWeight -= quantity * itemDef.weight`
- Only call `calculatePlayerCurrentWeight()` on initial load and as a periodic sanity check
- **Impact:** Eliminates ~50 DB queries per session
- **Risk:** Low — item weights are known from `itemDefinitions` Map. Periodic full recalc catches drift.

**4B. Use `itemDefinitions` Map instead of JOIN for static fields**
- `getPlayerInventory()` can query only `character_inventory` columns
- Look up static item data (name, description, stats, etc.) from the in-memory `itemDefinitions` Map
- **Impact:** Simpler, faster query. The items table is already fully cached in memory.
- **Risk:** Low — `itemDefinitions` is loaded at startup and never changes

### Phase 5: Hotbar — Event-Driven Instead of Polling

**5A. Replace RefreshItemQuantities timer with DataVersion check**
- Hotbar already knows its slots' InventoryIds
- When InventorySubsystem's DataVersion changes, update quantities directly
- Remove the 1-second polling timer
- **Impact:** Eliminates 7,200 linear searches per second
- **Risk:** None — more responsive too (instant update vs 1s delay)

---

## Implementation Priority

| Priority | Change | Effort | Impact | Risk |
|----------|--------|--------|--------|------|
| P0 | 1A: Cache filtered items + return by ref | 30 min | MASSIVE | None |
| P0 | 1B: Fix GetItemAtSlot to use cache | 15 min | MASSIVE | None |
| P0 | 1C: Add TMap for FindItemByInventoryId | 15 min | HIGH | None |
| P1 | 3A: Strip descriptions from inventory:data | 45 min | HIGH | Low |
| P1 | 4A: Incremental weight tracking | 30 min | MEDIUM | Low |
| P1 | 5A: Event-driven hotbar refresh | 20 min | MEDIUM | None |
| P2 | 2A: Delta grid update | 2 hrs | HIGH | Low |
| P2 | 3B: Strip card descriptions | 15 min | LOW | None |
| P3 | 4B: Use itemDefinitions Map in getPlayerInventory | 1 hr | MEDIUM | Low |
| P3 | 3C: Delta sync protocol | 3 hrs | HIGH | Medium |

**Recommended approach:** Start with P0 changes (1 hour total) — these alone should eliminate 90%+ of client-side lag. Then proceed to P1 for server-side gains.

---

## Verification Method

After each phase, measure:
1. **Frame time** during inventory operations (use `stat unit` in UE5)
2. **Payload size** of `inventory:data` events (add `JSON.stringify(payload).length` log)
3. **DB query count** per inventory operation (add counter in getPlayerInventory/calculatePlayerCurrentWeight)
4. **Widget count** before/after rebuild (log GridContainer->GetChildren().Num())

---

## Files That Will Be Modified

| File | Changes |
|------|---------|
| `client/.../UI/InventorySubsystem.h` | Add cached filtered items array, TMap index, cache version tracking |
| `client/.../UI/InventorySubsystem.cpp` | Cache GetFilteredItems, add TMap in HandleInventoryData, return by ref |
| `client/.../UI/SInventoryWidget.cpp` | Fix GetItemAtSlot, lazy tooltips, (P2: delta grid update) |
| `client/.../UI/HotbarSubsystem.cpp` | Event-driven refresh instead of polling |
| `server/src/index.js` | (P1: strip descriptions, incremental weight) |

No gameplay behavior changes. No new socket events. No DB schema changes. No Blueprint changes.

---

## Safety Review — Fix-By-Fix Dependency Analysis

Every system that touches inventory data was traced. This section documents
exactly what depends on what, and whether each proposed fix is truly safe.

### Systems That Consume Inventory Data

| System | How It Gets Data | What It Reads | Mutates Data? |
|--------|-----------------|---------------|---------------|
| **InventorySubsystem** | Registers `inventory:data` via EventRouter | ALL fields (ParseItemFromJson: 62 fields) | Yes — `Items.Empty()` + re-parse in HandleInventoryData |
| **EquipmentSubsystem** | Registers `inventory:data` via EventRouter | Reads `InvSub->Items` directly (line 113): bIsEquipped, EquippedPosition, EquipSlot | No — reads only, copies into own `EquippedSlots` TMap |
| **BasicInfoSubsystem** | Registers `inventory:data` via EventRouter | Nothing — handler is log-only (line 314) | No |
| **HotbarSubsystem** | Does NOT register for `inventory:data` | Reads via `FindItemByInventoryId()`: Icon, Quantity only | No — copies values into FHotbarSlot |
| **SInventoryWidget** | Polls `DataVersion` in Tick() | `GetFilteredItems()` and `GetItemAtSlot()` | No — reads only |
| **SItemInspectWidget** | Receives FInventoryItem by value | `FullDescription` (falls back to `Description`) | No |
| **CartSubsystem** | Own socket events (`cart:data`) | Own CartItems array (separate from inventory) | Own data only |
| **StorageSubsystem** | Own socket events (`storage:data`) | Own StorageItems array; own `GetFilteredItems()` | Own data only |
| **ShopSubsystem** | Own socket events (`shop:data`) | Own parsed item data | Own data only |
| **TradingSubsystem** | Own socket events (`trade:*`) | Own trade window data | Own data only |
| **CraftingSubsystem** | Own socket events | No inventory dependency | No |
| **AudioSubsystem** | Called from HandleItemUsed | None — just plays `PlayItemUseSound()` | No |
| **CombatActionSubsystem** | No inventory reads | Uses skill IDs, not inventory | No |
| **DamageNumberSubsystem** | No inventory reads | Combat events only | No |
| **MultiplayerEventSubsystem** | All bridges removed (comment lines 34-39) | Nothing | No |

**Key finding:** Only 3 subsystems register for `inventory:data` on the client:
InventorySubsystem, EquipmentSubsystem, BasicInfoSubsystem. Of those, only
InventorySubsystem actually parses and stores items. Equipment defers a tick
then reads from `InvSub->Items` directly.

---

### FIX 1A: Cache Filtered Items + Return by Const Reference

**What changes:** `GetFilteredItems()` caches its result and returns `const TArray<FInventoryItem>&`
instead of a fresh by-value copy every call.

**All callers verified:**

| Caller | File:Line | Mutates result? | Safe? |
|--------|-----------|-----------------|-------|
| `SInventoryWidget::RebuildGrid()` | SInventoryWidget.cpp:332 | No — iterates for Num() and slot indices | YES |
| `SInventoryWidget::GetItemAtSlot()` | SInventoryWidget.cpp:660 | No — reads by index, returns copy | YES |

**No other files call `UInventorySubsystem::GetFilteredItems()`.** StorageSubsystem and
CartSubsystem have their own separate `GetFilteredItems()` methods on their own subsystems.

**Cache invalidation requirement:** Must rebuild cache when ANY of these change:
- `DataVersion` (inventory data from server)
- `CurrentTab` (user switches tab)
- `SearchFilter` (user types in search box)

**Thread safety:** All inventory data handling runs on the game thread. Socket events
arrive via the SocketIOClient plugin tick, which runs on game thread. Widget Tick
also runs on game thread. No concurrent access is possible.

**Edge case: EquipmentSubsystem reads `InvSub->Items` directly (line 113).**
This is the raw `Items` array, NOT `GetFilteredItems()`. The cache change
does not affect this path at all. Equipment reads `Items` after a one-tick
defer (line 89), ensuring HandleInventoryData has already populated it.

**VERDICT: SAFE — zero functionality impact.**

---

### FIX 1B: Fix GetItemAtSlot() to Use Cached Array

**What changes:** Instead of calling `GetFilteredItems()` (which currently copies the
whole array), index directly into the cached array from Fix 1A.

**All callers verified:**

| Caller | File:Line | Context | Safe? |
|--------|-----------|---------|-------|
| `BuildItemSlot()` | SInventoryWidget.cpp:382 | During grid build, copies item for icon/tooltip | YES |
| Quantity lambda | SInventoryWidget.cpp:449 | Per-frame, reads Quantity + bStackable | YES |
| Unidentified "?" lambda | SInventoryWidget.cpp:464 | Per-frame, reads bIdentified | YES |
| Mouse handlers | SInventoryWidget.cpp:866,891,999,1077,1132 | On click/drag, copies item data | YES |

**Potential concern: Lambda uses stale SlotIndex after inventory update.**
This already happens with the current code — lambdas captured during BuildItemSlot
use a fixed SlotIndex. If inventory changes between grid rebuilds, the slot-to-item
mapping can be one frame stale. This is the EXISTING behavior. Our change does not
make it worse. The grid rebuilds on the very next Tick after DataVersion changes.

**VERDICT: SAFE — identical behavior, just fewer copies.**

---

### FIX 1C: Add TMap<int32,int32> for FindItemByInventoryId

**What changes:** Add `TMap<int32, int32> InventoryIdToIndex` mapping InventoryId to
array index. Rebuilt in `HandleInventoryData()` alongside the Items array.

**All callers verified — every FindItemByInventoryId call:**

| Caller | File:Line | Pointer lifetime | Mutates item? | Safe? |
|--------|-----------|-----------------|---------------|-------|
| HotbarSubsystem::HandleHotbarAllData | HotbarSubsystem.cpp:240 | Immediate — copies Icon+Quantity to FHotbarSlot | No | YES |
| HotbarSubsystem::HandleHotbarData | HotbarSubsystem.cpp:323 | Immediate — copies Icon+Quantity to FHotbarSlot | No | YES |
| HotbarSubsystem::RefreshItemQuantities | HotbarSubsystem.cpp:840 | Immediate — reads Quantity, compares | No | YES |
| InventorySubsystem::CompleteDrop | InventorySubsystem.cpp:1126 | Immediate — dereferences *FullItem | No | YES |
| SHotbarRowWidget::OnDragEnter | SHotbarRowWidget.cpp:539 | Immediate — dereferences *FullItem for AssignItem | No | YES |
| SHotbarRowWidget::OnDragOver | SHotbarRowWidget.cpp:630 | Immediate — dereferences *FullItem for AssignItem | No | YES |
| SInventoryWidget::OnDragDropped | SInventoryWidget.cpp:1052 | Immediate — dereferences *Item for StartDrag | No | YES |

**Critical check: No caller stores the pointer across frames.** All callers
consume the pointer immediately (same frame, same function scope).

**Critical check: Items array is only modified in HandleInventoryData()** (line 437:
`Items.Empty()` then re-add). HandleInventoryEquipped and HandleInventoryDropped
are no-ops (log only). HandleItemUsed calls RequestInventoryRefresh but does not
modify Items directly. So the TMap stays valid between HandleInventoryData calls.

**The TMap index is rebuilt every time Items is rebuilt.** No stale entries possible.

**VERDICT: SAFE — O(1) lookup returns same pointer as O(N) scan.**

---

### FIX 2A: Delta Grid Update (DEFERRED — needs more design)

**What changes:** Instead of ClearChildren + rebuild all widgets, only update
changed slots.

**Risk analysis:**
- Tab changes still need full rebuild (item set changes completely)
- Search filter changes need full rebuild
- Adding/removing items changes slot count → need to add/remove rows
- Drag-and-drop relies on slot indices being current
- Tooltip content must update if item at slot changed (e.g., quantity)

**There is no hard safety concern** (won't break multiplayer/audio/combat), but
incorrect implementation could cause visual glitches (wrong icon, stale tooltip,
missing quantity update). The complexity is in the edge cases, not in the
core approach.

**VERDICT: DEFER — not needed for P0. Fixes 1A+1B already eliminate the
per-frame hemorrhaging. The grid rebuild only happens once per inventory change,
which is acceptable. The per-frame lambda cost is the actual killer.**

---

### FIX 2B: Lazy Tooltips

**What changes:** Don't create SToolTip widget in BuildItemSlot(). Create on hover.

**Dependency check:**
- `SItemInspectWidget` reads `FullDescription` from the FInventoryItem passed to it
- Currently: tooltip is created during BuildItemSlot using the item at grid-build time
- With lazy: tooltip would be created from cached filtered items at hover time

**Functional difference:** If inventory updates between grid rebuild and hover,
the tooltip shows the CURRENT item data (lazy) vs STALE item data (current).
The lazy approach is actually MORE correct.

**VERDICT: SAFE — functionally better, not worse. But LOW priority since
tooltips are only ~50 widgets and don't execute per-frame.**

---

### FIX 3A: Strip descriptions from inventory:data — REVISED: UNSAFE AS ORIGINALLY PROPOSED

**Original proposal:** Remove `description` and `full_description` from the server
payload. Send only dynamic fields.

**Dependency that BLOCKS this:**
- `SItemInspectWidget.cpp:242-243` reads `CurrentItem.FullDescription` to render
  the item tooltip popup. This data comes from `ParseItemFromJson()` line 361
  which parses it from the `inventory:data` payload.
- If we strip `full_description` from the payload, **item tooltips would show
  empty descriptions**. This is a visible functionality regression.
- Same issue for card descriptions (`compounded_card_details[].full_description`).

**What would be needed to make this safe:**
1. Client-side item definition cache keyed by `item_id` (built from first full load)
2. Server sends full data on `inventory:load`, stripped data on mutations
3. Client merges dynamic fields with cached static definitions
4. This is essentially Fix 3C (delta sync) — a larger change

**VERDICT: UNSAFE AS PROPOSED — would break item tooltips. Requires client-side
item definition cache first. REMOVED from P1. Move to P3 alongside 3C.**

---

### FIX 3B: Strip card descriptions — SAME ISSUE AS 3A

**Dependency:** `ParseItemFromJson()` line 407 parses card `full_description`.
Card descriptions are shown in item tooltips alongside the compound slot list.

**VERDICT: UNSAFE AS PROPOSED — same dependency as 3A. REMOVED from P2.**

---

### FIX 3C: Delta Sync Protocol — DEFERRED

**What changes:** Server sends only changed items instead of full inventory.

**This is the correct long-term solution for 3A+3B**, but requires:
- Server-side change tracking (which items were added/removed/updated)
- Client-side full state maintenance (apply deltas to local array)
- Version numbering for sync verification
- Full-sync fallback on version mismatch

**Risk:** If client and server get out of sync, inventory shows wrong data.
Mitigation: periodic full sync, version checks.

**VERDICT: MEDIUM RISK — needs careful design. DEFER to after P0+P1 prove stable.**

---

### FIX 4A: Incremental Weight Tracking

**What changes:** Instead of `calculatePlayerCurrentWeight()` (DB query every time),
maintain `player.currentWeight` by adding/subtracting known item weights.

**Dependency check:**
- `updatePlayerWeightCache()` (line 5098) calls `calculatePlayerCurrentWeight()`
  then emits `weight:status` if thresholds crossed
- Called from 50+ locations throughout index.js
- Weight thresholds (50%, 90%) gate HP/SP regen and attack ability
- Item weights are known from `itemDefinitions` Map (loaded at startup, static)

**Risk scenario: Weight drift.**
If a code path adds/removes items without updating weight, the cached weight
diverges from reality. This could cause:
- Player appears under 50% weight but is actually over → regen works when it shouldn't
- Player appears over 90% but is actually under → can't attack when they should be able to

**Mitigation:** Keep `calculatePlayerCurrentWeight()` as a periodic sanity check
(e.g., every 60 seconds, or on zone load). If drift detected, correct silently.

**Implementation safety:** The incremental approach uses `itemDefinitions.get(itemId).weight`
which is guaranteed to exist (items are validated before being added to inventory).
The weight value is an integer — no floating point drift.

**VERDICT: SAFE with periodic sanity check. No gameplay impact if sanity check
interval is <=60 seconds (drift would be corrected before player notices).**

---

### FIX 4B: Use itemDefinitions Map Instead of JOIN — DEFERRED

**What changes:** `getPlayerInventory()` queries only `character_inventory` columns,
looks up static item data from in-memory `itemDefinitions` Map.

**Risk:** If `itemDefinitions` doesn't have an item_id (should never happen — items
are static and loaded at startup). Would need a fallback or error log.

**Complexity:** Requires restructuring getPlayerInventory() to build the result
object from two sources instead of one SQL query. All 53 callers expect the
same shape — must produce identical output.

**VERDICT: SAFE but large refactor. DEFER — lower priority than client fixes.**

---

### FIX 5A: Event-Driven Hotbar Refresh

**What changes:** Replace the 1-second polling timer in `SHotbarRowWidget::Tick()`
with a `DataVersion` check tied to InventorySubsystem.

**Current code** (SHotbarRowWidget.cpp:382-386):
```cpp
QuantityRefreshTimer += InDeltaTime;
if (QuantityRefreshTimer > 1.0f) {
    QuantityRefreshTimer = 0.f;
    Sub->RefreshItemQuantities();
}
```

**Proposed change:**
```cpp
UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>();
if (InvSub && InvSub->DataVersion != LastInvDataVersion) {
    LastInvDataVersion = InvSub->DataVersion;
    Sub->RefreshItemQuantities();
}
```

**Why this is safe:**
1. Same function called (`RefreshItemQuantities`) — same logic
2. Runs in same context (Tick, game thread)
3. `DataVersion` is incremented AFTER `Items` is fully populated (line 449)
4. EquipmentSubsystem already uses a similar deferred pattern (line 89)
5. Hotbar stores InventoryId integers, not pointers — safe across array rebuilds

**What improves:**
- Refresh fires immediately when inventory changes (0 delay vs up to 1s delay)
- No CPU cost when inventory hasn't changed (0 work per frame vs 7,200 comparisons/sec)
- Combined with Fix 1C (TMap), RefreshItemQuantities itself becomes 36 hash lookups instead of 36 x O(N) scans

**What could go wrong:**
- If DataVersion somehow increments without Items being populated → RefreshItemQuantities
  would find no items → hotbar slots would clear. BUT DataVersion only increments at
  the end of HandleInventoryData (line 449), after Items is fully populated. This
  cannot happen.

**VERDICT: SAFE — strictly better behavior (faster response, less CPU).**

---

### Cross-Cutting Concerns

**Multiplayer:** Each player has their own UInventorySubsystem instance (it's a
UWorldSubsystem, one per world). Other players' inventory changes arrive via
different socket events (`player:equipment_changed`, `player:moved`). None of
these touch the local player's inventory data. No multiplayer impact.

**Audio:** `AudioSubsystem::PlayItemUseSound()` is called from `HandleItemUsed()`
(line 495-498). This reads NO inventory data — it just plays a sound. No audio impact.

**VFX/Visuals:** No VFX system reads inventory data. Equipment visuals are handled
by EquipmentSubsystem which reads `InvSub->Items` directly (not GetFilteredItems).
Our fixes don't change how Items is populated. No visual impact.

**Combat:** Server-authoritative. The client never reads inventory data for combat
calculations. Hotbar item use sends InventoryId to server via socket.
No combat impact.

**Sprite System:** Reads equipment data from EquipmentSubsystem and `player:moved`
events. No inventory dependency. No sprite impact.

**Trading/Storage/Cart:** Each has its own subsystem with its own item arrays
and its own socket events. No dependency on InventorySubsystem's GetFilteredItems
or FindItemByInventoryId. No trading/storage/cart impact.

---

### Revised Implementation Priority

| Priority | Change | Effort | Impact | Risk | Notes |
|----------|--------|--------|--------|------|-------|
| **P0** | 1A: Cache filtered items + return by const ref | 30 min | MASSIVE | None | Only 2 callers, both verified safe |
| **P0** | 1B: Fix GetItemAtSlot to use cache | 15 min | MASSIVE | None | Eliminates per-frame array copying |
| **P0** | 1C: TMap for FindItemByInventoryId | 15 min | HIGH | None | 7 callers, all immediate consumption |
| **P0** | 5A: Event-driven hotbar refresh | 15 min | HIGH | None | DataVersion check, same function |
| **P1** | 4A: Incremental weight tracking | 30 min | MEDIUM | Low | Add periodic sanity check |
| **DEFER** | 2A: Delta grid update | 2 hrs | MEDIUM | Low | P0 fixes eliminate the worst cost already |
| **DEFER** | 2B: Lazy tooltips | 30 min | LOW | None | Minor savings, not urgent |
| ~~P1~~ | ~~3A: Strip descriptions~~ | — | — | **BLOCKED** | Breaks item tooltips — needs client cache first |
| ~~P2~~ | ~~3B: Strip card descriptions~~ | — | — | **BLOCKED** | Same as 3A |
| **DEFER** | 3C+3A+3B: Delta sync + definition cache | 4 hrs | HIGH | Medium | Correct long-term solution, do after P0 |
| **DEFER** | 4B: itemDefinitions Map in getPlayerInventory | 1 hr | MEDIUM | Low | Large refactor, lower priority |

**Summary of changes:** 3A and 3B were REMOVED because stripping descriptions
from the payload would break item tooltips (SItemInspectWidget reads FullDescription).
Fix 5A was promoted to P0 since it's trivial and zero-risk.
Fixes 2A, 2B, 3C, 4B deferred — P0 alone should eliminate 90%+ of client lag.

---

## Item Definition Cache Design (Unblocks 3A + 3B)

### The Insight

Every FInventoryItem field falls into one of two categories:

**STATIC (per item_id, identical for every player, never changes at runtime):**
```
name, description, full_description, item_type, equip_slot,
weight, price, buy_price, sell_price,
atk, def, matk, mdef,
str_bonus, agi_bonus, vit_bonus, int_bonus, dex_bonus, luk_bonus,
max_hp_bonus, max_sp_bonus, hit_bonus, flee_bonus, critical_bonus, perfect_dodge_bonus,
required_level, stackable, max_stack, icon, view_sprite,
weapon_type, aspd_modifier, weapon_range,
slots, weapon_level, refineable, jobs_allowed,
card_type, card_prefix, card_suffix,
two_handed, element
```
**Count: 43 fields.** These come from the `items` table which the server already has
fully cached in the `itemDefinitions` Map (loaded at startup, never modified).

**DYNAMIC (per inventory row, unique per player, changes on equip/use/refine/etc.):**
```
inventory_id, item_id, quantity, is_equipped, equipped_position,
slot_index, refine_level, compounded_cards, identified
```
**Count: 9 fields.** These come from the `character_inventory` table.

**FCompoundedCardInfo is 100% static** — every field (Name, Description, FullDescription,
Icon, CardType, CardPrefix, CardSuffix, Weight) is just the card's item definition
looked up by card item_id. The client can resolve this locally.

### Current Flow (wasteful)

```
Every inventory mutation:
  Server → DB: SELECT 12 ci.* + 50 i.* FROM character_inventory JOIN items (62 cols)
  Server → JSON: serialize 62 fields × N items + card details
  Server → Wire: ~164 KB for 200 items
  Client → Parse: 62 TryGetField calls × N items
```

### Proposed Flow

```
Once at login (inventory:load):
  Server → Client: itemDefs:data — definitions for all item_ids in player's inventory
  Client: stores in TMap<int32, FItemDefinition> ItemDefCache

Every inventory mutation (53 sites):
  Server → DB: SELECT 9 cols FROM character_inventory only (NO JOIN)
  Server → JSON: serialize 9 fields × N items (NO descriptions, NO stats)
  Server → Wire: ~15 KB for 200 items (91% reduction)
  Client → Parse: 9 TryGetField calls × N items
  Client → Merge: look up static fields from ItemDefCache[item_id]
  Client → Cards: resolve CompoundedCardDetails locally from ItemDefCache
```

### Server Changes

**1. New `getPlayerInventoryLight(characterId)` function:**
```javascript
async function getPlayerInventoryLight(characterId) {
    const result = await pool.query(
        `SELECT inventory_id, item_id, quantity, is_equipped, slot_index,
                equipped_position, refine_level, compounded_cards, identified
         FROM character_inventory
         WHERE character_id = $1
         ORDER BY slot_index ASC, created_at ASC`, [characterId]);
    return result.rows;
}
```
No JOIN. No item table columns. No compounded_card_details post-processing.

**2. New `itemDefs:data` emission in `inventory:load` handler (line 10843):**
```javascript
socket.on('inventory:load', async () => {
    const inventory = await getPlayerInventoryLight(characterId);

    // Collect unique item_ids (including card ids from compounded_cards)
    const neededIds = new Set();
    for (const item of inventory) {
        neededIds.add(item.item_id);
        if (item.compounded_cards) {
            for (const cid of item.compounded_cards) {
                if (cid && cid > 0) neededIds.add(cid);
            }
        }
    }

    // Build definitions payload from in-memory cache (no DB query)
    const defs = {};
    for (const id of neededIds) {
        const def = itemDefinitions.get(id);
        if (def) defs[id] = def;  // Full row from items table
    }

    socket.emit('itemDefs:data', { definitions: defs });
    socket.emit('inventory:data', { items: inventory, ... });
});
```

**3. All other 52 emission sites:** Replace `getPlayerInventory()` with
`getPlayerInventoryLight()`. No `compounded_card_details` enrichment needed.

**4. When player acquires a NEW item_id** (loot, buy, trade, quest reward):
Before emitting `inventory:data`, check if the item_id is new to this player.
If so, include an `itemDefs` field in the payload:
```javascript
socket.emit('inventory:data', {
    items: lightInventory,
    newDefs: { [newItemId]: itemDefinitions.get(newItemId) },  // Only new ones
    ...
});
```

### Client Changes

**1. New struct `FItemDefinition`** (or reuse static fields from FInventoryItem):
Could be a separate lightweight struct, or we can keep FInventoryItem as-is and
just add the cache logic.

**Simplest approach:** No new struct needed. Cache entire FInventoryItem objects
by item_id. On light-load, create FInventoryItem with dynamic fields from server,
copy static fields from cache.

**2. New cache in InventorySubsystem.h:**
```cpp
TMap<int32, FInventoryItem> ItemDefCache;  // item_id → static definition
```

**3. New handler for `itemDefs:data`:**
```cpp
void HandleItemDefs(const TSharedPtr<FJsonValue>& Data);
// Parses definitions object, populates ItemDefCache
```

**4. Modified `HandleInventoryData()`:**
```cpp
void HandleInventoryData(const TSharedPtr<FJsonValue>& Data) {
    // Parse newDefs if present → add to ItemDefCache
    // Parse items array — only dynamic fields
    // For each item: merge with ItemDefCache[item.ItemId] for static fields
    // Resolve CompoundedCardDetails locally from ItemDefCache
    Items.Empty();
    for (each item in payload) {
        FInventoryItem Item = ParseLightItemFromJson(itemObj);  // 9 fields
        MergeWithDefinition(Item);  // Copy 43 static fields from cache
        ResolveCardDetails(Item);   // Build CompoundedCardDetails from ItemDefCache
        Items.Add(MoveTemp(Item));
    }
    ++DataVersion;
}
```

**5. CompoundedCardDetails resolved client-side:**
```cpp
void ResolveCardDetails(FInventoryItem& Item) {
    Item.CompoundedCardDetails.Empty();
    for (int32 CardId : Item.CompoundedCards) {
        if (CardId <= 0) {
            Item.CompoundedCardDetails.Add(FCompoundedCardInfo());
            continue;
        }
        const FInventoryItem* CardDef = ItemDefCache.Find(CardId);
        if (CardDef) {
            FCompoundedCardInfo Info;
            Info.ItemId = CardId;
            Info.Name = CardDef->Name;
            Info.Description = CardDef->Description;
            Info.FullDescription = CardDef->FullDescription;
            Info.Icon = CardDef->Icon;
            Info.CardType = CardDef->CardType;
            Info.CardPrefix = CardDef->CardPrefix;
            Info.CardSuffix = CardDef->CardSuffix;
            Info.Weight = CardDef->Weight;
            Item.CompoundedCardDetails.Add(MoveTemp(Info));
        }
    }
}
```

### Safety Analysis

**No FInventoryItem struct changes.** The struct stays identical — all existing
code that reads FInventoryItem fields works unchanged. The only change is WHERE
the static fields come from (cache instead of server payload).

**SItemInspectWidget:** Still reads `CurrentItem.FullDescription` — still populated,
just from cache instead of parsed from JSON. No change needed.

**EquipmentSubsystem:** Still reads `InvSub->Items` with bIsEquipped/EquippedPosition.
These are dynamic fields still sent by server. No change needed.

**Hotbar:** Still calls `FindItemByInventoryId()` which returns pointer to Items
array. Items still contains full FInventoryItem with all fields merged. No change.

**Cart/Storage/Shop:** These have their OWN socket events and parsers. They are
NOT affected by inventory:data changes. They can adopt the same pattern later
if desired, but are independent.

**Fallback safety:** If `ItemDefCache` doesn't have a definition for an item_id
(shouldn't happen, but defensive), fall back to sending a full `inventory:load`
request which re-sends definitions. No crash, just one extra round-trip.

### Payload Size Comparison (200 items, 30 with cards)

| | Current | With Cache |
|---|---------|-----------|
| Static fields per item | ~400 chars | 0 (cached) |
| Dynamic fields per item | ~80 chars | ~80 chars |
| Card details per item | ~800 chars (2 cards avg) | 0 (resolved client-side) |
| **Total per emission** | **~164 KB** | **~16 KB** |
| **Reduction** | — | **90%** |

### Updated Priority Table

| Priority | Change | Effort | Impact | Risk |
|----------|--------|--------|--------|------|
| **P0** | 1A: Cache filtered items + return by const ref | 30 min | MASSIVE | None |
| **P0** | 1B: Fix GetItemAtSlot to use cache | 15 min | MASSIVE | None |
| **P0** | 1C: TMap for FindItemByInventoryId | 15 min | HIGH | None |
| **P0** | 5A: Event-driven hotbar refresh | 15 min | HIGH | None |
| **P1** | Item definition cache (client) | 1 hr | HIGH | None — FInventoryItem unchanged |
| **P1** | getPlayerInventoryLight (server) | 30 min | HIGH | None — light query, same result shape |
| **P1** | itemDefs:data event + newDefs field | 30 min | HIGH | Low — additive event |
| **P1** | Client-side CompoundedCardDetails resolution | 20 min | MEDIUM | None — same data, local lookup |
| **P1** | 4A: Incremental weight tracking | 30 min | MEDIUM | Low — add sanity check |
| **DEFER** | 2A: Delta grid update | 2 hrs | MEDIUM | Low |
| **DEFER** | 2B: Lazy tooltips | 30 min | LOW | None |
| **DEFER** | 3C: Delta sync (only changed items) | 3 hrs | HIGH | Medium |

**P0 (1 hour):** Client-only fixes — zero server changes, eliminates per-frame lag.
**P1 (2.5 hours):** Item definition cache + light queries — 90% payload reduction.
Combined P0+P1 should make inventory performance a non-issue.
