# Implement Kafra Storage System

## Task

Implement the complete Kafra Storage system for Sabri_MMO — an account-shared 300-slot item storage accessible via Kafra NPCs for 40 Zeny per open.

## Required Context

Load these skills FIRST:
- `/sabrimmo-economy` — Full economy architecture, storage schema, anti-dupe patterns
- `/sabrimmo-items` — Item structs, inventory schema, addFullItemToInventory helper
- `/sabrimmo-ui` — Slate widget patterns, Z-order map, RO brown/gold theme
- `/sabrimmo-persistent-socket` — SocketEventRouter registration pattern
- `/full-stack` — Server architecture, PostgreSQL patterns

Then read the implementation plan:
- `docsNew/05_Development/Kafra_Storage_And_Trading_Implementation_Plan.md` — Section A (Kafra Storage)

Read these existing files before modifying them:
- `client/SabriMMO/Source/SabriMMO/CharacterData.h` — Add FStorageItem struct, extend enums
- `client/SabriMMO/Source/SabriMMO/UI/KafraSubsystem.h` and `.cpp` — Understand existing Kafra menu
- `client/SabriMMO/Source/SabriMMO/UI/SKafraWidget.h` and `.cpp` — Add "Use Storage" button
- `client/SabriMMO/Source/SabriMMO/UI/InventorySubsystem.h` and `.cpp` — Drag-drop integration
- `client/SabriMMO/Source/SabriMMO/UI/SInventoryWidget.h` and `.cpp` — Reference for grid layout pattern
- `client/SabriMMO/Source/SabriMMO/UI/CartSubsystem.h` and `.cpp` — Reference for similar subsystem (cart is very close to storage)
- `client/SabriMMO/Source/SabriMMO/UI/SCartWidget.h` and `.cpp` — Reference for grid widget pattern
- `server/src/index.js` — Search for `kafra:open`, `cart:move_to_cart`, `addFullItemToInventory`, `calculatePlayerCurrentWeight`, `sendPlayerInventory` to understand existing patterns

## Implementation Steps

### Phase 1: Database (5 min)
1. Create `database/migrations/add_account_storage.sql` with the `account_storage` table (see plan Section A2)
2. Add auto-create block in `server/src/index.js` near the existing `// Ensure stat columns exist` section

### Phase 2: Server Handlers (30 min)
1. Add `STORAGE` constants (MAX_SLOTS=300, ACCESS_FEE=40, FREE_TICKET_ID=7059, REQUIRED_BASIC_SKILL=6)
2. Add `player.isStorageOpen = false` to player initialization (search for where `isVending` is initialized)
3. Implement these socket handlers in order:
   - `storage:open` — Basic Skill check, free ticket OR 40z fee, load items from `account_storage` by `user_id`
   - `storage:close` — Set `isStorageOpen = false`
   - `storage:deposit` — Atomic: lock inventory row, check trade_flag & 0x004, check equipped, check 300 slot limit, stack if stackable, remove from inventory FIRST then add to storage
   - `storage:withdraw` — Atomic: lock storage row, remove from storage FIRST then add via `addFullItemToInventory()`
   - `storage:cart_deposit` — Same as deposit but source is `character_cart`, check `hasCart` and `!isVending`
   - `storage:cart_withdraw` — Same as withdraw but destination is cart, check cart weight limit
4. Create `sendStorageContents(socket, userId)` helper
5. Add auto-close: `isStorageOpen = false` in disconnect handler, zone:change handler
6. Add mutual exclusion: check `isStorageOpen` in `vending:start`, `trade:request` handlers; check `isVending`/`isTrading` in `storage:open`

### Phase 3: Client — Data Structures (10 min)
1. Add `Storage` to `EItemDragSource` enum in `CharacterData.h`
2. Add `StorageSlot` to `EItemDropTarget` enum in `CharacterData.h`
3. Add `FStorageItem` struct (see plan Section A4.2)

### Phase 4: Client — StorageSubsystem (30 min)
1. Create `StorageSubsystem.h` and `StorageSubsystem.cpp`
2. Follow the exact same lifecycle pattern as `CartSubsystem`:
   - `ShouldCreateSubsystem()` — check for game world
   - `OnWorldBeginPlay()` — delayed timer → register 4 socket handlers via Router
   - `Deinitialize()` — unregister all for owner, hide widget, clear timers
3. Socket handlers: `storage:opened`, `storage:closed`, `storage:updated`, `storage:error`
4. Public API: `RequestOpen()`, `RequestClose()`, `DepositItem()`, `WithdrawItem()`, `DepositFromCart()`, `WithdrawToCart()`
5. Widget lifecycle: `ShowWidget()` at Z=21, `HideWidget()`
6. Tab filtering: `GetFilteredItems()` (0=All, 1=Consumable, 2=Equipment, 3=Etc)

### Phase 5: Client — SStorageWidget (45 min)
1. Create `SStorageWidget.h` and `SStorageWidget.cpp`
2. Use `SCartWidget` as the PRIMARY reference (10-column grid, scrollable, RO theme)
3. Key differences from cart:
   - Shows slot count (X/300) instead of weight bar
   - No F-key toggle (storage opens via Kafra menu, not hotkey)
   - Supports Alt+Right-Click for quick deposit/withdraw
4. Layout: Title bar (draggable) → Tab bar → 10-column scrollable grid → Slot count bar
5. Drag-drop: Accept from Inventory and Cart, drag out to Inventory and Cart
6. Right-click: ItemInspect popup
7. Double-click: Quick-withdraw to inventory
8. Window resize support (min 200x200)

### Phase 6: Integration (10 min)
1. `SKafraWidget.cpp`: Add "Use Storage (40z)" button in `BuildMainMenuContent()` between Teleport and Cart buttons
2. `InventorySubsystem.cpp`: Add `StorageSlot` case in `CompleteDrop()` → calls `StorageSubsystem->DepositItem()`
3. `InventorySubsystem.cpp`: Add `Storage` source case in `CompleteDrop()` InventorySlot target → calls `StorageSubsystem->WithdrawItem()`
4. If CartSubsystem supports drop targets, add Storage handling there too

### Phase 7: Testing
Test each item from the checklist in plan Section A6.

## Key Patterns to Follow

- **Atomic transfers**: Remove from source BEFORE adding to destination (anti-dupe)
- **FOR UPDATE row locks**: Always lock rows before modifying
- **Socket event registration**: Use `Router->RegisterHandler()` in `OnWorldBeginPlay`, `Router->UnregisterAllForOwner(this)` in `Deinitialize`
- **Widget overlay**: Create with `SNew(SWeightedOverlay)`, add to viewport at correct Z-order
- **Movement NOT locked**: Unlike Kafra menu, storage does NOT lock movement (player can walk with storage open)
- **RO theme colors**: PanelBrown(0.43,0.29,0.17), GoldTrim(0.72,0.58,0.28), TextPrimary(0.96,0.90,0.78)
- **DataVersion polling**: Widget checks `DataVersion` in Tick to rebuild grid on changes

## Critical Rules

1. Storage is keyed on `user_id`, NOT `character_id` — account-shared
2. No Zeny storage — items only
3. Deposit allowed while overweight (90%+) — storage is the escape valve
4. Cannot deposit equipped items — must unequip first
5. Check `trade_flag & 0x004` for NoStorage items
6. Preserve ALL item attributes: refine_level, compounded_cards, forged_by, forged_element, forged_star_crumbs, identified
7. Stack stackable items in storage (same item_id + same identified state)
8. Max 300 slots — check before insert, not just on open
