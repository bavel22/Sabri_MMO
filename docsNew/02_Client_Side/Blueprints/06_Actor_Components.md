# Actor Components

## AC_HUDManager

**Path**: `/Game/SabriMMO/Components/AC_HUDManager`  
**Parent Class**: `ActorComponent`  
**Attached To**: `BP_MMOCharacter` (as component named `HUDManager`)  
**Purpose**: The most function-rich Blueprint in the project (~38 functions). Manages ALL HUD widgets: GameHUD, Chat, Inventory, Stats, Death Overlay, Loot Popup, Damage Numbers, Target Frame, Hotbar, and Shop. Acts as the bridge between server events (via BP_SocketManager) and UI display. Also stores global `PlayerZuzucoin` balance.

**Status**: Phase 1 stabilization complete. Added 17 null guards and removed unused Event Tick. See refactoring plan: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `SocketManagerRef` | object (BP_SocketManager) | Cached ref to socket manager |
| `GameHudRef` | object (WBP_GameHUD) | Main game HUD widget instance |
| `ChatWindowRef` | object (WBP_ChatWidget) | Chat widget instance |
| `InventoryWindowRef` | object (WBP_InventoryWindow) | Inventory window instance |
| `TooltipRef` | object (WBP_ItemTooltip) | Active item tooltip widget instance |
| `bIsInventoryOpen` | bool | Inventory toggle state |
| `StatAllocationWindowRef` | object (WBP_StatAllocation) | Stat window instance |
| `bIsStatWindowOpen` | bool | Stat window toggle state |
| `LootPopupRef` | object (WBP_LootPopup) | Loot notification popup |
| `DeathOverlayRef` | object (WBP_DeathOverlay) | Death screen overlay |
| `DamageNumberRef` | object (WBP_DamageNumber) | Damage number widget class |
| `HotBarRef` | object (WBP_Hotbar) | Hotbar widget instance |
| `ShopWindowRef` | object (WBP_Shop) | Active shop window instance (null when closed) |
| `PlayerZuzucoin` | int | Player's current Zuzucoin balance. Updated from `player:joined`, `shop:bought`, `shop:sold` |

**⚠️ Bug variable**: `"Game Hud Ref"` (with space) also exists in the Blueprint — appears to be a stale duplicate of `GameHudRef`. Do not use; use `GameHudRef` instead.

### Event Graph (7 nodes)

```
Event BeginPlay
    ↓
Get Actor Of Class(BP_SocketManager) → Set SocketManagerRef
    ↓
IsValid(SocketManagerRef)
    ├─ TRUE → (normal flow continues)
    └─ FALSE → Print String "ERROR: BP_SocketManager not found in level!"
```

**Note**: Unused Event Tick removed to eliminate per-frame overhead.

### Functions — HUD Lifecycle

| Function | Nodes | Description |
|----------|-------|-------------|
| `InitializeHUD` | 15 | Creates WBP_GameHUD and WBP_ChatWidget, adds both to viewport. Called by BP_SocketManager after connection established |
| `InitializeChatWindow` | 5 | Sets up chat widget references |

### Functions — Health Display

| Function | Nodes | Description |
|----------|-------|-------------|
| `UpdateLocalPlayerHealth` | 30 | Sets HP bar percent and text on WBP_GameHUD (CurrentHealth/MaxHealth). **Guarded**: IsValid(GameHudRef) check added |
| `UpdateLocalPlayerMana` | 30 | Sets MP bar percent and text on WBP_GameHUD (CurrentMana/MaxMana). **Guarded**: IsValid(GameHudRef) check added |
| `UpdateTargetHealth` | 35 | Sets target frame HP bar percent, name text, and HP text. **Guarded**: IsValid(GameHudRef) check added |
| `UpdateTargetMana` | 35 | Sets target frame MP bar (reserved for future PvP target mana display). **Guarded**: IsValid(GameHudRef) check added |
| `ShowTargetFrame` | 10 | Makes target frame visible on WBP_GameHUD. **Guarded**: IsValid(GameHudRef) check added |
| `HideTargetFrame` | 10 | Hides target frame on WBP_GameHUD. **Guarded**: IsValid(GameHudRef) check added |

### Functions — Combat UI

| Function | Nodes | Description |
|----------|-------|-------------|
| `ShowDeathOverlay` | 10 | Creates WBP_DeathOverlay, adds to viewport. **Guarded**: Duplicate creation check prevents multiple overlays |
| `HideDeathOverlay` | 14 | Removes death overlay, restores game input mode. **Guarded**: IsValid(DeathOverlayRef) + clears reference after removal |
| `SendRespawnRequest` | 14 | Emits `combat:respawn` via SocketManager. **Guarded**: IsValid(SocketManagerRef) check added |
| `ShowDamageNumbers` | 32 | Creates WBP_DamageNumber at world position, projects to screen, sets damage text and color (red for damage taken, white for damage dealt) |

### Functions — Inventory

| Function | Nodes | Description |
|----------|-------|-------------|
| `ToggleInventory` | 25 | Creates or destroys WBP_InventoryWindow. Emits `inventory:load` when opening. Toggles `bIsInventoryOpen`. **On open**: emits inventory:load, zuzucoin display updated by PopulateInventory when data arrives |
| `PopulateInventory` | **176** | Parses JSON inventory data. **NEW**: Parses `zuzucoin` field from inventory:data → sets `PlayerZuzucoin` → calls `WBP_InventoryWindow.UpdateZuzucoin(PlayerZuzucoin)` to display balance. Then for each item: creates WBP_InventorySlot, sets all item properties (name, type, quantity, equipped status, ATK, DEF, etc.), calls `SetItemIcon` on each slot (DT_ItemIcons lookup), binds OnSlotAction delegate, adds to inventory grid. Also sets `EquippedWeaponInventoryId`/`EquippedArmorInventoryId` on WBP_InventoryWindow, sets equipped weapon/armor icon on WBP_InventoryWindow slots, and updates `WeaponInfoText`/`ArmorInfoText` text (shows item name or "No Weapon"/"No Armor"). Clears grid before loop. **Guarded**: IsValid(InventoryWindowRef) check added. |
| `HandleSlotAction` | 16 | Delegates from WBP_InventorySlot click. Routes to equip/unequip, use, or drop based on item type and equipped status |
| `SendEquipRequest` | 16 | Emits `inventory:equip` with {inventoryId, equip: true/false}. **Guarded**: IsValid(SocketManagerRef) check added |
| `SendUseItemRequest` | 14 | Emits `inventory:use` with {inventoryId}. **Guarded**: IsValid(SocketManagerRef) check added |
| `SendDropRequest` | 14 | Emits `inventory:drop` with {inventoryId}. **Guarded**: IsValid(SocketManagerRef) check added |
| `SendMoveItemRequest` | 22 | Emits `inventory:move` with {sourceInventoryId, targetInventoryId}. Called from `WBP_InventorySlot.On Drop`. **Guarded**: IsValid(SocketManagerRef) check added |
| `ShowItemTooltip` | — | Creates/positions WBP_ItemTooltip. Called by WBP_InventorySlot on mouse enter. Params: InventoryId, ItemName, ItemType, ATK, DEF, ItemDescription, WeaponRange, AspdModifier, RequiredLevel |
| `HideItemTooltip` | — | Removes WBP_ItemTooltip. Called by WBP_InventorySlot on mouse leave |

### Functions — Stats

| Function | Nodes | Description |
|----------|-------|-------------|
| `ToggleStats` | 21 | Creates or destroys WBP_StatAllocation. Emits `player:request_stats` when opening. Toggles `bIsStatWindowOpen` |
| `UpdateStats` | **160** | Parses stats JSON. Updates ALL stat text fields in WBP_StatAllocation: base stats (STR-LUK), derived stats (ATK=WeaponATK+StatusATK, Hit, Flee, SoftDEF, ASPD, **Critical**), maxHP, maxSP, stat points remaining. **Guarded**: IsValid(StatAllocationWindowRef) check added. ATK = WeaponATKLocal + StatusATKLocal |
| `SendStatIncreaseRequest` | 16 | Emits `player:allocate_stat` with {statName, amount: 1}. **Guarded**: IsValid(SocketManagerRef) check added |

### Functions — Chat

| Function | Nodes | Description |
|----------|-------|-------------|
| `ChatReceived` | 14 | Parses chat JSON (channel, senderName, message). Calls AddChatMessage |
| `AddChatMessage` | 26 | Creates WBP_ChatMessageLine, sets MessageText = "[Channel] SenderName: message", adds to chat scroll box, auto-scrolls to bottom. **Guarded**: IsValid(ChatWindowRef) check added |
| `SendChatMessageRequest` | 45 | Gets text from chat input field, constructs JSON {channel, message}, emits `chat:message`, clears input field. **Guarded**: IsValid(SocketManagerRef) check added |

### Functions — Loot

| Function | Nodes | Description |
|----------|-------|-------------|
| `ShowLootPopup` | 51 | Parses loot JSON (enemyName, items array). Creates WBP_LootPopup, adds loot item texts showing "Qty x ItemName", sets fade timer, adds to viewport |

### Functions — Shop

| Function | Nodes | Description |
|----------|-------|-------------|
| `OpenShop` | ~30 | Creates WBP_Shop, stores in `ShopWindowRef`, parses `shop:data` JSON (shopName, items[], playerZuzucoin), populates WBP_ShopItem list, calls `UpdateZuzucoinDisplay(playerZuzucoin)`, stores `PlayerZuzucoin`. **Guarded**: duplicate-open check |
| `CloseShop` | ~10 | Removes WBP_Shop from viewport, clears `ShopWindowRef`. **Guarded**: IsValid(ShopWindowRef) check |
| `UpdateZuzucoinEverywhere` | 19 | Sets `PlayerZuzucoin = NewAmount`. Calls `WBP_Shop.UpdateZuzucoin(NewAmount)` if ShopWindowRef valid. Calls `WBP_InventoryWindow.UpdateZuzucoin(NewAmount)` if InventoryWindowRef valid. Called by BP_SocketManager on `shop:bought`/`shop:sold` to update both displays simultaneously |
| `SendPopulateShopSellRequest` | ~15 | Emits `inventory:data` request (or passes cached inventory) to populate the WBP_Shop sell tab with non-equipped items. **Guarded**: IsValid(SocketManagerRef) check |
| `ShowShopError` | ~8 | Displays error message in WBP_Shop's StatusText. **Guarded**: IsValid(ShopWindowRef) check |

### Functions — Hotbar

| Function | Nodes | Description |
|----------|-------|-------------|
| `InitializeHotbar` | 17 | Creates WBP_Hotbar widget, stores in `HotBarRef`, adds to viewport. Called by `InitializeHUD` during HUD setup. Passes self reference to WBP_Hotbar |
| `SyncHotbarWithInventory` | 6 | Triggers a hotbar sync pass — calls `WBP_Hotbar.SyncWithInventory` to refresh quantities after inventory changes. **Guarded**: IsValid(HotBarRef) check |
| `SendSaveHotbarSlotRequest` | 28 | Emits `hotbar:save` with `{slotIndex, inventoryId, itemId, itemName}`. Called from `WBP_HotbarSlot.OnDrop`. **Guarded**: IsValid(SocketManagerRef) check |
| `PopulateHotbarFromServer` | 60 | Parses `hotbar:data` JSON array. For each slot entry: calls `GetSlotAtIndex` on WBP_Hotbar → calls `SetSlotData` on the matching WBP_HotbarSlot (slotIndex, inventoryId, itemId, itemName, quantity). **Guarded**: IsValid(HotBarRef) check |
| `UseHotbarSlot` | 39 | Called by BP_MMOCharacter when `IA_Hotbar_1`–`IA_Hotbar_9` are pressed (passes slot index 0–8). Calls `GetSlotAtIndex` on WBP_Hotbar → checks if slot has valid `HotbarInventoryId` → calls `SendUseItemRequest(HotbarInventoryId)`. **Guarded**: IsValid(HotBarRef) + IsValid slot ref + HotbarInventoryId > 0 |

### PopulateInventory — Detailed Flow (111 nodes)

This is the second most complex function. Key logic:

```
PopulateInventory(Data: String)
    ↓
Parse JSON → Get Array Field "items"
Clear existing inventory grid children
    ↓
For Each item in array:
    Create WBP_InventorySlot widget
    ↓
    Parse item fields:
        inventory_id, item_id, name, description, item_type,
        equip_slot, quantity, is_equipped, atk, def,
        weapon_type, weapon_range, aspd_modifier
    ↓
    Set all variables on WBP_InventorySlot:
        InventoryId, ItemId, ItemName, ItemType, Quantity,
        bIsEquipped, EquipSlot, ItemDescription,
        ATK, DEF, WeaponType, WeaponRange, AspdModifier
    ↓
    Bind OnSlotAction delegate → HandleSlotAction
    ↓
    Add to InventoryGrid (WrapBox or UniformGridPanel)
```

### HandleSlotAction — Decision Tree

```
HandleSlotAction(InventoryId: int, ItemType: string, bEquip: bool)
    ↓
Branch: ItemType == "consumable"?
    ├─ YES → SendUseItemRequest(InventoryId)
    └─ NO → Branch: ItemType == "weapon" OR "armor"?
        ├─ YES → SendEquipRequest(InventoryId, bEquip)
        └─ NO → (etc items — no action currently)
```

### Called By

| Caller | Functions Called |
|--------|----------------|
| `BP_SocketManager` (via MMOCharacterHudManager) | InitializeHUD, UpdateLocalPlayerHealth/Mana, UpdateTargetHealth, ShowTargetFrame, HideTargetFrame, ShowDeathOverlay, HideDeathOverlay, UpdateStats, PopulateInventory, PopulateHotbarFromServer, ShowLootPopup, ShowDamageNumbers, ChatReceived, Set PlayerZuzucoin, OpenShop, UpdateZuzucoinEverywhere, ShowShopError |
| `BP_MMOCharacter` | ToggleInventory (IA_ToggleInventory), ToggleStats, UseHotbarSlot (IA_Hotbar_1–9) |
| `WBP_StatAllocation` | SendStatIncreaseRequest, ToggleStats (close button) |
| `WBP_ChatWidget` | SendChatMessageRequest |
| `WBP_InventoryWindow` | ToggleInventory (close button) |
| `WBP_Shop` | CloseShop (close button), SendPopulateShopSellRequest (sell tab) |
| `WBP_DeathOverlay` | SendRespawnRequest |

---

## Safety Nets Added (Phase 1)

### Null Guards Implemented

| Function | Guard Type | Variable Checked | Reason |
|----------|------------|------------------|--------|
| UpdateLocalPlayerHealth | Widget Ref | GameHudRef | Prevents crash if HUD not initialized |
| UpdateLocalPlayerMana | Widget Ref | GameHudRef | Prevents crash if HUD not initialized |
| UpdateTargetHealth | Widget Ref | GameHudRef | Prevents crash if HUD not initialized |
| UpdateTargetMana | Widget Ref | GameHudRef | Prevents crash if HUD not initialized |
| ShowTargetFrame | Widget Ref | GameHudRef | Prevents crash if HUD not initialized |
| HideTargetFrame | Widget Ref | GameHudRef | Prevents crash if HUD not initialized |
| AddChatMessage | Widget Ref | ChatWindowRef | Prevents crash if chat not initialized |
| PopulateInventory | Widget Ref | InventoryWindowRef | Prevents crash if inventory not open |
| UpdateStats | Widget Ref | StatAllocationWindowRef | Prevents crash if stats not open |
| ShowDeathOverlay | Duplicate Guard | DeathOverlayRef | Prevents multiple death overlays |
| HideDeathOverlay | Widget Ref | DeathOverlayRef | Prevents crash if no overlay exists |
| SendRespawnRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |
| SendEquipRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |
| SendUseItemRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |
| SendDropRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |
| SendMoveItemRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |
| SendStatIncreaseRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |
| SendChatMessageRequest | Emit Guard | SocketManagerRef | Prevents crash if SocketManager missing |

### Event Graph Changes
- **Removed**: Unused Event Tick node (eliminates per-frame overhead)
- **Added**: SocketManagerRef validation with error print

### Performance Impact
- **Positive**: Eliminated unnecessary Event Tick calls
- **Positive**: Early returns prevent unnecessary processing when widgets are null
- **Neutral**: IsValid checks add minimal overhead (<0.01ms per call)

### Future Phases
See complete refactoring plan: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`

---

## AC_CameraController

**Path**: `/Game/SabriMMO/Components/AC_CameraController`  
**Parent Class**: `ActorComponent`  
**Attached To**: `BP_MMOCharacter` (as component named `CameraController`)  
**Purpose**: Handles top-down camera rotation (right-click hold + drag) and mouse wheel zoom.

### Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `SpringArmRef` | object (SpringArmComponent) | — | Reference to character's SpringArm |
| `bIsRotatingCamera` | bool | false | Whether right mouse button is held |
| `CurrentYaw` | real | — | Current camera yaw angle |
| `CurrentPitch` | real | — | Current camera pitch angle |
| `CurrentRoll` | real | — | Current camera roll angle |
| `LookInput` | struct (Vector2D) | — | Current mouse delta input |
| `ZoomSpeed` | real | — | Zoom sensitivity |
| `NewArmLength` | real | — | Computed arm length after zoom |
| `ClampedLength` | real | — | Arm length clamped to min/max |

### Functions

| Function | Nodes | Params | Description |
|----------|-------|--------|-------------|
| `HandleCameraRotation` | 20 | `DeltaTime (float)` | If `bIsRotatingCamera`: applies LookInput to yaw/pitch rotation on SpringArm. Uses `SetWorldRotation` with interpolated values |
| `HandleZoom` | 11 | `AxisValue (float)` | Adjusts `SpringArmRef.TargetArmLength` by `AxisValue * ZoomSpeed`. Clamps between 200 and 1500 units |
| `SetLookInput` | 2 | `Input (Vector2D)` | Stores mouse delta for rotation |
| `SetRotating` | 2 | `bRotating (bool)` | Sets `bIsRotatingCamera` flag |

### Event Graph (2 nodes)
Empty — BeginPlay and Tick stubs only. All logic is called from BP_MMOCharacter.

---

## AC_TargetingSystem

**Path**: `/Game/SabriMMO/Components/AC_TargetingSystem`  
**Parent Class**: `ActorComponent`  
**Attached To**: `BP_MMOCharacter` (as component named `TargetingSystem`)  
**Purpose**: Handles mouse hover detection over other players and enemies, and checks whether the local player is within attack range of the current target.

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `HoveredTargetPlayerRef` | object | Currently hovered remote player |
| `HoeveredTargetEnemyRef` | object | Currently hovered enemy (note: typo in var name) |
| `CurrentTargetEnemyRef` | object | Current attack target (enemy) |
| `CurrentTargetPlayerRef` | object | Current attack target (player) |
| `CurrentTargetId` | int | ID of current target |
| `bHasTarget` | bool | Whether any target is selected |
| `bIsAutoAttacking` | bool | Auto-attack state |
| `AttackRange` | real | Current attack range (from server) |
| `LastCursorScreenPos` | struct (Vector2D) | Last cursor position |
| `OnTargetPlayerSelected` | Multicast Delegate | Fired when a player is selected |
| `OnTargetEnemySelected` | Multicast Delegate | Fired when an enemy is selected |
| `TargetLocation` | struct (Vector) | Target's world position |
| `LocalPlayerLocation` | struct (Vector) | Local player's world position |

### Functions

| Function | Nodes | Params | Returns | Description |
|----------|-------|--------|---------|-------------|
| `UpdateHoverDetection` | **59** | — | — | Runs on Tick. Performs cursor trace, checks if hovering over BP_OtherPlayerCharacter or BP_EnemyCharacter, shows/hides HoverOverIndicator widgets |
| `CheckIfInAttackRangeOfTargetPlayer` | 22 | `MyLocation (Vector), TargetPlayer (BP_OtherPlayerCharacter)` | Sets `bIsInAutoAttackRangeOfPlayer` on owner | Calculates 3D distance, compares to AttackRange |
| `CheckIfInAttackRangeOfEnemy` | 22 | `MyLocation (Vector), TargetEnemy (BP_EnemyCharacter)` | Sets `bIsInAutoAttackRangeOfEnemy` on owner | Calculates 3D distance, compares to AttackRange |

### UpdateHoverDetection — Flow

```
UpdateHoverDetection()
    ↓
Get Player Controller → Get Hit Result Under Cursor by Channel
    ↓ Break Hit Result → Get Hit Actor
    ↓
[Try cast to BP_OtherPlayerCharacter]
    ├─ SUCCESS:
    │   Set HoveredTargetPlayerRef = result
    │   Show HoverOverIndicator on hovered player
    │   (Hide previous hover if different actor)
    └─ FAIL:
        [Try cast to BP_EnemyCharacter]
        ├─ SUCCESS:
        │   Set HoeveredTargetEnemyRef = result
        │   Show HoverOverIndicator on hovered enemy
        └─ FAIL:
            Hide all HoverOverIndicators
            Clear hovered refs
```

---

**Last Updated**: 2026-02-22 (Feature #20 NPC Shop: added ShopWindowRef + PlayerZuzucoin variables; added Shop functions: OpenShop, CloseShop, UpdateZuzucoinEverywhere, SendPopulateShopSellRequest, ShowShopError; ToggleInventory now calls UpdateZuzucoinDisplay on open; total functions ~38. ⚠️ unrealMCP scan unavailable — node counts approximate)

**Previous**: 2026-02-20 (Feature #19 Hotbar complete: HotBarRef variable added; 5 new hotbar functions added (InitializeHotbar, SyncHotbarWithInventory, SendSaveHotbarSlotRequest, PopulateHotbarFromServer, UseHotbarSlot); total functions now 33; PopulateInventory updated to 168 nodes; verified via unrealMCP)
**Source**: Read via unrealMCP `read_blueprint_content`  
**Related**: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`
