# Implement Player-to-Player Trading System

## Task

Implement the complete direct player-to-player trading system for Sabri_MMO — RO Classic style with 10 item slots per side, Zeny exchange, two-step confirmation (OK → Trade), and full anti-scam protection.

## Required Context

Load these skills FIRST:
- `/sabrimmo-economy` — Full economy architecture, trade events, anti-dupe patterns
- `/sabrimmo-items` — Item structs, inventory schema, addFullItemToInventory helper, trade flags
- `/sabrimmo-ui` — Slate widget patterns, Z-order map, RO brown/gold theme
- `/sabrimmo-persistent-socket` — SocketEventRouter registration pattern
- `/full-stack` — Server architecture, PostgreSQL patterns
- `/sabrimmo-chat` — Chat system for /trade command and system messages

Then read the implementation plan:
- `docsNew/05_Development/Kafra_Storage_And_Trading_Implementation_Plan.md` — Section B (Player-to-Player Trading)

Read these existing files before modifying them:
- `client/SabriMMO/Source/SabriMMO/CharacterData.h` — Extend enums with Trade/TradeSlot
- `client/SabriMMO/Source/SabriMMO/UI/VendingSubsystem.h` and `.cpp` — CRITICAL reference: similar popup/transaction pattern
- `client/SabriMMO/Source/SabriMMO/UI/SVendingBrowsePopup.h` and `.cpp` — Reference for dual-panel popup
- `client/SabriMMO/Source/SabriMMO/UI/PlayerInputSubsystem.h` and `.cpp` — Add right-click context menu
- `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.h` — Player registry, GetPlayerIdFromActor
- `client/SabriMMO/Source/SabriMMO/UI/InventorySubsystem.h` and `.cpp` — Drag-drop integration
- `client/SabriMMO/Source/SabriMMO/UI/SInventoryWidget.h` and `.cpp` — Reference for drag-drop input
- `client/SabriMMO/Source/SabriMMO/UI/PartySubsystem.cpp` — Reference for right-click context menu pattern (search for FMenuBuilder)
- `client/SabriMMO/Source/SabriMMO/UI/ChatSubsystem.h` and `.cpp` — /trade command, system messages
- `server/src/index.js` — Search for `vending:start`, `addFullItemToInventory`, `calculatePlayerCurrentWeight`, `sendPlayerInventory`, `connectedPlayers`, disconnect handler, zone:change handler, death handler

## Implementation Steps

### Phase 1: Database (5 min)
1. Create `database/migrations/add_trade_system.sql` with the `trade_logs` table (see plan Section B2)
2. Add auto-create block in `server/src/index.js` near existing auto-create sections

### Phase 2: Server Infrastructure (15 min)
1. Add `TRADE` constants (MAX_ITEMS=10, MAX_ZENY=2147483647, DISTANCE=100)
2. Create `TradeSession` class (see plan Section B3) — tracks both players, items, zeny, lock states
3. Create `activeTrades` Map (characterId → TradeSession) and `pendingTradeRequests` Map
4. Add `player.isTrading = false`, `player.tradePartnerId = null` to player initialization
5. Create helpers: `cancelTrade()`, `cleanupTrade()`, `getPlayerByCharacterId()`, `getItemQuantity()`
6. Create `validatePlayerState(player, action)` helper for reusable state checks

### Phase 3: Server — Trade Request Flow (20 min)
1. `trade:request` handler — Validate self state, find target (online + same zone), validate target state, distance check (100 UE units), create pending request with 30s auto-expire, notify target
2. `trade:accept` handler — Find pending request, re-validate both states, re-check distance, create TradeSession, mark both as trading, open trade windows
3. `trade:decline` handler — Remove pending request, notify requester
4. `trade:cancel` handler — Call `cancelTrade()`, notify both parties

### Phase 4: Server — Trade Item/Zeny Management (20 min)
1. `trade:add_item` handler — Check not locked, validate slot count (max 10), query item from DB, check ownership/equipped/trade_flag, weight check on receiver, add to session, reset locks if either was locked (anti-scam), notify both
2. `trade:remove_item` handler — Check not locked, remove from session, reset locks, notify both
3. `trade:set_zeny` handler — Clamp to valid range, check sender has enough, check receiver overflow, set in session, reset locks, notify both
4. `trade:lock` handler — Set locked for this player, notify partner, check if both locked

### Phase 5: Server — Trade Execution (25 min)
1. `trade:confirm` handler — Check both locked, set confirmed, wait for both
2. When both confirmed, execute in atomic DB transaction:
   - Re-validate ALL items exist and are owned (FOR UPDATE locks)
   - Re-validate Zeny amounts
   - Transfer items A→B: DELETE/UPDATE from A's inventory, addFullItemToInventory to B
   - Transfer items B→A: same pattern
   - Transfer Zeny both directions
   - Log trade to `trade_logs`
   - COMMIT
3. Update weight caches for both players
4. Emit `trade:completed` to both with received/gave summaries
5. Clean up trade state
6. Refresh inventories for both via `sendPlayerInventory()`

### Phase 6: Server — Auto-Cancel Guards (10 min)
1. Disconnect handler: cancel active trade, delete pending requests
2. Zone change handler: cancel active trade
3. Death handler: cancel active trade
4. Add `isTrading` check to: `vending:start`, `storage:open`
5. Add `isVending`/`isStorageOpen` check to `trade:request`
6. Block `inventory:equip`, `inventory:drop`, `inventory:use` if `isTrading`

### Phase 7: Client — Data Structures (5 min)
1. Add `Trade` to `EItemDragSource` in `CharacterData.h`
2. Add `TradeSlot` to `EItemDropTarget` in `CharacterData.h`

### Phase 8: Client — TradeSubsystem (40 min)
1. Create `TradeSubsystem.h` with `ETradeState` enum and `FTradeItem` struct
2. Create `TradeSubsystem.cpp`
3. Register 11 socket handlers via Router:
   - `trade:request_received` → show request popup
   - `trade:opened` → set state, show trade widget
   - `trade:item_added` → update MyItems or PartnerItems based on `side` field
   - `trade:item_removed` → remove from correct array
   - `trade:zeny_updated` → update MyZeny or PartnerZeny
   - `trade:partner_locked` → set bPartnerLocked
   - `trade:partner_unlocked` → clear all locks, update UI
   - `trade:both_locked` → enable Trade button
   - `trade:completed` → show in chat, close widget
   - `trade:cancelled` → show reason in chat, close widget
   - `trade:error` → show in chat
4. Public API: `RequestTrade()`, `AcceptRequest()`, `DeclineRequest()`, `AddItem()`, `RemoveItem()`, `SetZeny()`, `Lock()`, `Confirm()`, `Cancel()`
5. Widget management: `ShowTradeWidget()` at Z=22, `ShowRequestPopup()` at Z=30

### Phase 9: Client — STradeWidget (45 min)
1. Create `STradeWidget.h` and `STradeWidget.cpp`
2. Dual-panel layout:
   - LEFT: "Your Offer" — 5x2 item grid (10 slots), Zeny input (SEditableTextBox)
   - RIGHT: "Partner's Offer" — 5x2 item grid (10 slots), Zeny display (read-only)
3. Bottom buttons: OK / Trade / Cancel with state-dependent visibility
4. Drag-drop: Accept items from `EItemDragSource::Inventory` on LEFT panel
5. Right-click YOUR item → remove from trade
6. Right-click PARTNER item → ItemInspect popup (view full details)
7. Lock indicators: green badge or "LOCKED" text when a side is locked
8. Item display: icon, name (with refine+cards via GetDisplayName), quantity overlay
9. Tooltip on hover (same as inventory tooltip)

### Phase 10: Client — STradeRequestPopup (10 min)
1. Create small centered popup with "{PlayerName} wants to trade with you"
2. Accept and Decline buttons
3. 30-second auto-dismiss timer (match server timeout)
4. Z=30 (above most UI)

### Phase 11: Client — Integration (15 min)
1. **PlayerInputSubsystem**: Add right-click handler on other players → show FMenuBuilder context menu with "Trade" option. Reference: PartySubsystem's member context menu pattern.
2. **InventorySubsystem**: Add `TradeSlot` case in `CompleteDrop()` → `TradeSubsystem->AddItem()`
3. **ChatSubsystem**: Parse `/trade PlayerName` → look up character ID from OtherPlayerSubsystem → `TradeSubsystem->RequestTrade()`
4. **ChatSubsystem**: Add system messages for trade events (request sent, accepted, completed, cancelled)

### Phase 12: Testing
Test each item from the checklist in plan Section B6.

## Key Patterns to Follow

- **Two-step confirmation**: OK (lock) → Trade (finalize). BOTH must complete BOTH steps.
- **Lock reset on change**: If either player modifies items/zeny after one side locked, ALL locks reset. This is the core anti-scam mechanic.
- **Atomic transfer**: Wrap the entire trade in a single PostgreSQL `BEGIN`/`COMMIT` with `FOR UPDATE` row locks.
- **Remove before add**: Delete items from source inventory BEFORE adding to destination (anti-dupe).
- **Re-validate at commit**: ALL items re-checked for existence, ownership, equipped status at final commit time.
- **Socket event pattern**: Use `Router->RegisterHandler()` for all event binding.
- **Context menu pattern**: Follow PartySubsystem's `FMenuBuilder` + `FSlateApplication::Get().PushMenu()` pattern.
- **Chat notifications**: Use `ChatSubsystem->AddCombatLogMessage()` for trade system messages.

## Critical Rules

1. Max 10 items per side — hard limit, both server and client
2. Both sides can offer Zeny — not just one-way
3. Cannot trade equipped items — server strips them from deal
4. Check `trade_flag & 0x002` for NoTrade items
5. Weight check BEFORE item appears in partner's trade window AND at final commit
6. Zeny overflow check: partner's current + offered cannot exceed 2,147,483,647
7. Distance check (100 UE units) at request and acceptance only — not continuously
8. Auto-cancel on: disconnect, zone change, death
9. Block: cannot equip/drop/use items while trading
10. Block: cannot open storage or start vending while trading
11. Preserve ALL item attributes on transfer: refine_level, compounded_cards, forged_by, forged_element, forged_star_crumbs, identified
12. Log every completed trade to `trade_logs` table
