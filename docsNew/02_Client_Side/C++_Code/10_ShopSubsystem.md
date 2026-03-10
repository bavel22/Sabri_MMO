# UShopSubsystem

**Files**: `Source/SabriMMO/UI/ShopSubsystem.h` (111 lines), `ShopSubsystem.cpp` (637 lines)
**Parent**: `UWorldSubsystem`
**Purpose**: NPC shop buy/sell interface with cart system, Discount/Overcharge skill integration, and inventory weight tracking.

## Widget

`SShopWidget` at Z-order 18. Created/destroyed on toggle. Locks player movement while open (RO Classic behavior).

## Socket Events

### Incoming (Server → Client)
| Event | Handler | Description |
|-------|---------|-------------|
| `shop:data` | Opens shop UI with item catalog | Populates `ShopItems` array, sets `PlayerZuzucoin`, mode → ModeSelect |
| `shop:bought` | Purchase confirmed | Updates zuzucoin, refreshes inventory, clears buy cart |
| `shop:sold` | Sale confirmed | Updates zuzucoin, refreshes inventory, clears sell cart |
| `shop:error` | Error with 4s TTL | Sets `LastErrorMessage` for transient display |

### Outgoing (Client → Server)
| Event | Payload | Description |
|-------|---------|-------------|
| `shop:open` | `{ shopId }` | Request to open an NPC shop |
| `shop:buy_batch` | `{ shopId, items: [{itemId, quantity}...] }` | Batch purchase |
| `shop:sell_batch` | `{ items: [{inventoryId, quantity}...] }` | Batch sell |

## Shop Modes

`EShopMode` enum: `Closed` → `ModeSelect` → `BuyMode` / `SellMode`

## Key Functions

| Function | Description |
|----------|-------------|
| `RequestOpenShop(ShopId)` | Emits `shop:open` to server |
| `SetMode(EShopMode)` | Transitions between modes, refreshes sellable items |
| `AddToBuyCart/RemoveFromBuyCart/ClearBuyCart` | Buy cart management (merges duplicates) |
| `AddToSellCart/RemoveFromSellCart/ClearSellCart` | Sell cart management (capped at owned quantity) |
| `SubmitBuyCart()` | Sends batch buy, validates weight/slots/zuzucoin |
| `SubmitSellCart()` | Sends batch sell |
| `CloseShop()` | Resets state, restores movement |
| `GetSellableItems()` | Filters inventory (excludes equipped, price=0, fully-carted items) |
| `GetSellPrice(Item)` | Applies Overcharge bonus percentage |

## State Tracking

| Property | Purpose |
|----------|---------|
| `PlayerZuzucoin` | Current currency |
| `DiscountPercent` / `OverchargePercent` | Skill bonuses (from Merchant skills 601/602) |
| `CurrentWeight` / `MaxWeight` | Inventory weight tracking |
| `UsedSlots` / `MaxSlots` | Inventory slot count (max 100) |
| `DataVersion` | Incremented on every state change for widget polling |
| `LastErrorMessage` + `ErrorExpireTime` | 4s TTL transient error display |

## Cross-Subsystem Dependencies

- Reads inventory from `UInventorySubsystem` for sellable items
- Uses `FShopItem` and `FCartItem` structs from `CharacterData.h`

---

**Last Updated**: 2026-03-09
