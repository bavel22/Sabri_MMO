# Economy, Trading & Vending -- Deep Research (Pre-Renewal)

> **Scope**: Comprehensive reference for replicating RO Classic (pre-Renewal) economy mechanics -- currency, NPC shops, player trading, vending, buying stores, item drops, storage, and economy balance.
> **Sources**: iRO Wiki Classic, rAthena pre-renewal source code, RateMyServer, divine-pride, WarpPortal forums, rAthena GitHub (trade.cpp, vending.cpp, map.cpp, buyingstore.cpp), community guides.
> **Date**: 2026-03-22

---

## Economy Overview

The Ragnarok Online economy is player-driven and closely mirrors real-world supply/demand dynamics. Zeny is the sole currency. There is no auction house equivalent that matters in Classic (it was added later, was unpopular, and most servers disabled it). The economy revolves around three pillars:

1. **NPC shops** -- fixed-price floor for common items (potions, arrows, basic equipment)
2. **Player vending** -- Merchant class sets up shops with custom prices; backbone of the economy
3. **Direct player trading** -- peer-to-peer item/zeny exchange via trade window

Key design principle: SP recovery items (Blue Potions, Grape Juice) are NOT sold by NPCs. They must be hunted or crafted, making them a core player-economy commodity.

---

## 1. Zeny (Currency System)

### 1.1 Limits

| Property | Value | Notes |
|----------|-------|-------|
| Max zeny per character | **2,147,483,647** | 2^31 - 1 (signed 32-bit integer) |
| Max zeny per trade | **999,999,999** | Client-enforced limit on trade window zeny field |
| Max vending item price | **1,000,000,000** | 1 billion per unit |
| Min zeny | **0** | Cannot go negative |

**Overflow protection**: Any transaction that would push a character over 2,147,483,647z is rejected server-side. If a vending sale would overflow the vendor's zeny, the purchase is blocked and the item remains in the shop. If a trade would overflow either party's zeny, the trade is cancelled.

**Bank system**: Episode 10.2 added a Bank in Lighthalzen for depositing/withdrawing zeny to work around the per-character cap. However, the bank was disabled on many servers due to zeny disappearance bugs. For pre-renewal faithful implementation, the bank is optional and low-priority.

### 1.2 Zeny Sources (How Players Earn Zeny)

| Source | Description | Volume |
|--------|-------------|--------|
| Monster drops sold to NPC | Kill monsters, pick up loot, sell to NPC. Primary income for most players. | High (constant) |
| Selling items to NPC | Base sell price = floor(buyPrice / 2). Modified by Overcharge skill. | High (constant) |
| Player vending sales | Merchant sets up shop with custom prices. Revenue from other players. | High (endgame) |
| Direct player trades | Trade window exchange of items + zeny. | Medium |
| Quest rewards | Some quests give zeny directly. Amounts vary (100z to 100,000z+). | Low |
| Monster zeny drops | Some monsters drop zeny directly as a loot item (Gold, Platinum Bullion). These are items with high NPC sell price, not direct currency drops. | Low-Medium |

**Important**: In pre-renewal RO, monsters do NOT drop raw zeny coins directly. All zeny enters the economy through NPC sell transactions. Gold (NPC sell: 100,000z) and Platinum Bullion are items, not currency. This is a critical distinction -- every zeny in the economy was created by an NPC shop purchase.

### 1.3 Zeny Sinks (How Players Spend Zeny)

| Sink | Typical Cost | Frequency | Impact |
|------|-------------|-----------|--------|
| NPC shop purchases | 1z (Arrow) to 100,000z+ (equipment) | Constant | High |
| Kafra Storage access | 40z per access | Very frequent | Low per-use, high aggregate |
| Kafra Teleport | 600z - 3,000z per teleport | Frequent | Medium |
| Kafra Cart Rental | 600z - 1,200z (varies by town) | One-time per session | Low |
| Skill: Mammonite | 100z - 1,000z per use (100z * skill level) | Per-attack | Medium (Merchant mains) |
| Skill: Maximum Power-Thrust | 2,500 + (Lv * 500)z per use | Per-cast | Medium |
| Equipment Refinement | 50z - 20,000z per attempt + ore | Periodic | Very High (primary sink) |
| Equipment Repair | 5,000z per broken piece | Occasional | Low |
| Vending Tax | 5% on items priced >100,000,000z | Per-sale | Medium-High |
| Buying Store License | 200z per license | Per-shop-open | Negligible |

### 1.4 Inflation Control Mechanisms

| Mechanism | How It Works |
|-----------|-------------|
| Refinement destruction | Equipment destroyed on failure above safe limit. Consumes zeny + ore. Primary item/zeny sink. |
| Vending tax | 5% commission on high-value sales destroys zeny from circulation |
| Kafra services | Small but constant drain (storage 40z, teleport 600-3000z, cart 600-1200z) |
| Mammonite/MPT | Combat skills that consume zeny directly per use |
| NPC consumables | White Potions (1,200z), Fly Wings (60z), Butterfly Wings (300z) are constant drains |
| Repair costs | 5,000z per broken equipment piece |
| No direct zeny drops | All zeny originates from NPC sell, so the server rate for item drops indirectly controls zeny supply |
| Vendor search | @whosell command / search NPC lets players find cheapest vendor, suppressing price inflation |

**Key insight from community analysis**: Low-rate servers (1x drops) have naturally lower inflation because zeny generation (via NPC sell) is slower. High-rate servers suffer inflation because players accumulate sellable loot faster, pumping more zeny into the economy. The most effective anti-inflation measures are increasing zeny sinks (refine costs, service fees) rather than reducing drops.

---

## 2. NPC Shop System

### 2.1 Buy/Sell Price Formulas

```
Base NPC Buy Price  = item.price  (fixed, defined in item database)
Base NPC Sell Price = floor(item.price / 2)

With Discount skill (buying from NPC):
  Final Price = floor(buyPrice * (100 - discountRate) / 100)

With Overcharge skill (selling to NPC):
  Final Price = floor(sellPrice * (100 + overchargeRate) / 100)
```

### 2.2 Discount Skill (Merchant Passive, ID 37 / MC_DISCOUNT)

Reduces NPC buy prices. Prerequisite: Enlarge Weight Limit Lv. 3.

| Level | Discount Rate | Formula |
|-------|--------------|---------|
| 1 | 7% | 5 + 1*2 = 7 |
| 2 | 9% | 5 + 2*2 = 9 |
| 3 | 11% | 5 + 3*2 = 11 |
| 4 | 13% | 5 + 4*2 = 13 |
| 5 | 15% | 5 + 5*2 = 15 |
| 6 | 17% | 5 + 6*2 = 17 |
| 7 | 19% | 5 + 7*2 = 19 |
| 8 | 21% | 5 + 8*2 = 21 |
| 9 | 23% | 5 + 9*2 = 23 |
| 10 | **24%** | Capped at 24 (formula would give 25) |

**Formula**: `discountRate = min(24, 5 + skillLevel * 2)`

Available to: Merchant, Blacksmith, Whitesmith, Alchemist, Creator, Super Novice.

**Example**: White Potion (NPC price 1,200z) with Discount Lv.10:
`floor(1200 * (100 - 24) / 100) = floor(1200 * 0.76) = 912z`

### 2.3 Overcharge Skill (Merchant Passive, ID 38 / MC_OVERCHARGE)

Increases NPC sell prices. Prerequisite: Discount Lv. 3.

| Level | Overcharge Rate | Formula |
|-------|----------------|---------|
| 1 | 7% | 5 + 1*2 = 7 |
| 2 | 9% | 5 + 2*2 = 9 |
| 3 | 11% | 5 + 3*2 = 11 |
| 4 | 13% | 5 + 4*2 = 13 |
| 5 | 15% | 5 + 5*2 = 15 |
| 6 | 17% | 5 + 6*2 = 17 |
| 7 | 19% | 5 + 7*2 = 19 |
| 8 | 21% | 5 + 8*2 = 21 |
| 9 | 23% | 5 + 9*2 = 23 |
| 10 | **24%** | Capped at 24 |

**Formula**: `overchargeRate = min(24, 5 + skillLevel * 2)`

Available to: Merchant, Blacksmith, Whitesmith, Alchemist, Creator, Super Novice.

**Example**: Tsurugi (buy price 21,000z, sell price 10,500z) with Overcharge Lv.10:
`floor(10500 * (100 + 24) / 100) = floor(10500 * 1.24) = 13,020z`

### 2.4 Rogue Haggle Skill (Compulsion Discount, ID 1716 / RG_COMPULSION)

Rogues get a version of Discount called Haggle (Compulsion Discount). It does NOT stack with Merchant Discount -- uses `Math.max()` to pick the higher rate.

| Level | Discount Rate |
|-------|--------------|
| 1 | 9% |
| 2 | 13% |
| 3 | 17% |
| 4 | 21% |
| 5 | **24%** |

**Formula**: `discountRate = 5 + skillLevel * 4` (capped at 24)

### 2.5 NPC Shop Types

#### Tool Dealers (every major town)
Sell consumables and basic supplies. Representative inventory:

| Item | Buy Price | Category |
|------|-----------|----------|
| Arrow | 1z | Ammunition |
| Red Potion | 10z | HP Recovery |
| Orange Potion | 50z | HP Recovery |
| Yellow Potion | 180z | HP Recovery |
| White Potion | 1,200z | HP Recovery |
| Green Potion | 40z | Status Cure |
| Concentration Potion | 800z | AGI buff |
| Awakening Potion | 1,500z | AGI buff |
| Magnifier | 40z | Item identification |
| Fly Wing | 60z | Random teleport |
| Butterfly Wing | 300z | Return to save point |
| Trap | 100z | Hunter skill material |

**Critical design**: Blue Potions (SP recovery) are NOT sold by NPCs. Must be hunted or crafted.

#### Weapon Dealers (town-specific)
Each town sells weapons matching local class culture. Prices range from 50z (Knife) to 60,000z (Flamberge, Lance).

#### Armor Dealers (town-specific)
Armor/shields/accessories. Prices range from 10z (Cotton Shirt) to 65,000z (Chain Mail).

#### Specialty Shops
- **Gemstone dealers**: Blue Gemstone (1,500z), Yellow Gemstone (1,500z) -- required for Warp Portal, Safety Wall
- **Arrow dealers**: 11-35 arrow types depending on implementation
- **Pet food shops**: Taming items and pet food
- **Elemental stone NPCs**: Star Crumb, stones for forging
- **Ore NPCs**: Phracon (200z), Emveretarcon (1,000z) for refinement

### 2.6 Town Specialization

| Town | Specialty | Notable Items |
|------|-----------|---------------|
| **Prontera** | General/Swordsman/Knight | Full weapon range, armor up to Chain Mail |
| **Geffen** | Mage | Rods, Wands, Blue/Yellow Gemstones |
| **Payon** | Archer/Thief | Bows, daggers, arrows, traps |
| **Morroc** | Thief/Assassin | Daggers, katars, desert supplies |
| **Alberta** | Merchant/Marine | Trading goods, merchant supplies, Buying Store NPC |
| **Aldebaran** | General | Transit hub, Kafra HQ |
| **Juno** | Sage | Books, sage equipment |
| **Rachel** | Crusader | Holy-themed equipment |

---

## 3. Player-to-Player Trading

### 3.1 Trade Window Mechanics

The trade window enables direct peer-to-peer exchange of items and zeny. It uses a two-phase confirmation system to prevent scams.

#### Constants (rAthena source: trade.cpp)

| Constant | Value | Notes |
|----------|-------|-------|
| MAX_DEAL | **10** | Maximum item slots per side in trade window |
| Max zeny per trade | **999,999,999** | Client-enforced |
| Trade distance | ~2-3 cells | Must be on same map, close proximity |

#### Initiating a Trade
1. Right-click another player character
2. Select "Deal" / "Request Deal" from context menu
3. Target player receives a trade request popup
4. Target clicks "Accept" to open trade window, or "Deny" to reject

#### Trade Window Layout (per side)
- **10 item slots** (drag items from inventory, or Alt+Right-click to send)
- **1 Zeny input field** (max 999,999,999z)
- **"OK" button** -- locks your side (items/zeny cannot be changed after clicking)
- **"Trade" button** -- finalizes the exchange (only available after both sides locked)
- **"Cancel" button** -- aborts the trade at any time

### 3.2 Trade Process (Step by Step)

```
Phase 1: SETUP
  1. Player A right-clicks Player B -> selects "Deal"
  2. Player B receives popup -> clicks "Accept"
  3. Trade window opens for both players
  4. Both players drag items from inventory to trade window
     - Stackable items: prompted for quantity
     - Alt+Right-click shortcut sends item to trade window
  5. Both players enter zeny amount (optional)

Phase 2: LOCK
  6. Player A clicks "OK" -> their side is LOCKED
     - Items/zeny on A's side cannot be changed
     - B can still modify their side
  7. Player B clicks "OK" -> their side is LOCKED
     - Both sides now show as "locked"
     - Neither side can modify anything

Phase 3: CONFIRM
  8. Either player clicks "Trade" to finalize
  9. Server validates all items/zeny atomically
  10. If valid: items/zeny transferred, trade window closes
  11. If invalid: trade cancelled with error message
```

### 3.3 Cancel/Abort Conditions

A trade auto-cancels if:
- Either player clicks "Cancel" at any point before final "Trade"
- Either player walks too far away (exceeds trade distance)
- Either player logs out or disconnects
- Either player dies
- Either player opens a vending shop
- Either player opens storage
- Server sends a map change

### 3.4 Trade Restrictions

| Restriction | Details |
|------------|---------|
| Bound items | Account-bound and character-bound items cannot be placed in trade window |
| Quest items | Most quest items cannot be traded |
| Weight check | Trade fails if receiving player would exceed weight limit |
| Zeny overflow | Trade fails if receiving player's zeny would exceed 2,147,483,647 |
| Distance | Must be on same map and within close proximity (2-3 cells) |
| Dead players | Cannot initiate or accept trades while dead |
| Vending | Cannot trade while a vending shop is open |
| Storage open | Cannot trade while storage window is open |
| Untradeable flag | Some items have a trade restriction flag in the item database |

### 3.5 Anti-Scam Protections (Server-Side)

The server MUST enforce these protections:

1. **Atomic transactions**: Both sides commit or neither does -- no partial trades. Use database transaction with `BEGIN/COMMIT/ROLLBACK`.
2. **Item ownership verification**: Confirm each item belongs to the trading player's inventory at commit time (not just at add time).
3. **Item locking**: When placed in trade window, items are flagged as "in trade" and cannot be used, equipped, sold, or placed in another trade simultaneously.
4. **Zeny validation**: Verify sufficient funds on both sides at commit time (not just at add time).
5. **Weight validation**: Check receiving player has capacity for all incoming items.
6. **Tradeable check**: Verify items are allowed to be traded (no bound/quest/untradeable items).
7. **Zeny cap check**: Prevent overflow past 2,147,483,647 for both parties.
8. **Re-validation at commit**: All checks must be re-run at commit time, not just when items are added. Items could have been consumed by another system between add and commit.
9. **Audit logging**: Record every completed trade with timestamps, player IDs, all items, all zeny amounts.

### 3.6 Trade State Machine (rAthena reference)

```
TRADE_NONE        -> Player is not trading
TRADE_INITIATED   -> Trade request sent, waiting for response
TRADE_ACCEPTED    -> Both players have trade window open, adding items
TRADE_LOCKED      -> This player's side is locked (clicked OK)
TRADE_COMMITTED   -> Final "Trade" clicked, server processing
```

Both players must reach TRADE_LOCKED before either can advance to TRADE_COMMITTED.

---

## 4. Vending System (Merchant Skill)

### 4.1 Prerequisites

| Requirement | Details |
|------------|---------|
| Job class | Merchant, Blacksmith, Whitesmith, Alchemist, Creator, Super Novice |
| Skill: Pushcart | Lv. 3+ (prerequisite for Vending) |
| Skill: Vending (ID 41 / MC_VENDING) | Lv. 1-10 (determines max item stacks) |
| Cart equipped | Must have a Pushcart rented and active |
| Items in cart | Only items in Pushcart inventory can be vended |
| SP cost | 30 SP per activation |

### 4.2 Vending Skill Levels

| Level | Max Item Stacks | Formula |
|-------|----------------|---------|
| 1 | 3 | 2 + Lv |
| 2 | 4 | 2 + Lv |
| 3 | 5 | 2 + Lv |
| 4 | 6 | 2 + Lv |
| 5 | 7 | 2 + Lv |
| 6 | 8 | 2 + Lv |
| 7 | 9 | 2 + Lv |
| 8 | 10 | 2 + Lv |
| 9 | 11 | 2 + Lv |
| 10 | **12** | 2 + Lv |

Each "stack" = one item entry. For stackable items (potions, ores), a single stack can hold the full quantity of that item from the cart.

### 4.3 Setting Up a Vending Shop

1. Merchant must have Pushcart equipped with items to sell
2. Use Vending skill (hotkey or skill window)
3. **Shop Setup Window** appears:
   - **Shop Title**: Text input for the shop name (max ~80 characters, displayed above head)
   - **Item Selection**: List of all items currently in Pushcart
   - Player selects items to vend (up to max stacks for skill level)
   - **Set Price**: For each selected item, enter the per-unit price
   - **Price range**: 1z to 1,000,000,000z per unit
4. Click "OK" to open the shop
5. Character sits on the ground with shop title floating above head
6. Must be at least **4 cells from any NPC** (prevents obscuring NPC interaction)

### 4.4 Shop Title Display

- Shown as text banner above the vending character's head
- Visible to all players on the same map
- Players typically include item names and prices in the title
- Maximum length: ~80 characters (client-enforced)
- Examples: `Selling +7 Blade [3] 500k`, `CHEAP White Potions!!`, `Cards & Ores BEST PRICE`

### 4.5 Buying from a Vending Shop

1. Click on the vending character
2. **Vending Browse Window** appears showing:
   - Shop title at top
   - List of items: name, quantity available, price per unit
3. Select an item
4. Enter quantity to purchase (for stackable items)
5. Window shows calculated total price
6. Click "Buy" to confirm
7. Server validates: buyer has enough zeny, vendor has items, buyer has weight capacity, vendor won't overflow zeny
8. If valid: Zeny deducted from buyer, items added to buyer inventory, items removed from vendor's cart

### 4.6 Vending Tax (rAthena-verified)

The tax threshold is **100,000,000z** (100 million), not 10 million as some sources state. This was confirmed in rAthena PR #2620 which adjusted the tax to match official kRO behavior.

| Price Per Unit | Tax Rate | Vendor Receives |
|---------------|----------|-----------------|
| <= 100,000,000z | **0%** (no tax) | 100% of listed price |
| > 100,000,000z | **5%** | 95% of listed price |

The 5% tax is a zeny sink -- the taxed amount is destroyed, not given to anyone. This helps control inflation on high-value trades.

**Implementation note**: Tax is calculated per-unit price, not total. If unit price is 50,000,000z and buyer purchases 3, no tax applies (unit price < threshold).

### 4.7 Shop Closure Conditions

A vending shop closes when:
- All items are sold out (auto-close)
- The merchant manually cancels the shop
- The merchant disconnects or is kicked
- Server maintenance/restart
- The merchant dies (if attacked in PvP areas)

**Closing Report**: When a shop closes, the vendor sees a summary of:
- Number of items sold
- Buyer names
- Time of each sale
- Total zeny earned (after tax)

### 4.8 Vendor Behavior While Vending

| Action | Allowed? |
|--------|----------|
| Move | NO -- character is sitting and immobile |
| Attack | NO |
| Use skills | NO |
| Public chat | NO |
| Whisper (send/receive) | YES |
| Open inventory/storage | NO |
| Accept trades | NO |
| Use items | NO |

**AFK vending** is a core gameplay pattern. Merchants set up shop and go AFK for hours. Common vending locations:
- Near Kafra NPCs in major towns
- Town centers (Prontera south gate is iconic)
- Dungeon entrances
- Near popular farming spots

### 4.9 Pushcart Details

| Property | Value |
|----------|-------|
| Weight capacity | **8,000** |
| Item slots | **100** |
| Rental NPC | Kafra Employee |
| Required skill | Pushcart Lv. 1+ |
| Prerequisite skill | Increase Weight Limit Lv. 5 |
| Available to | Merchant class, Super Novice |

#### Cart Rental Costs by Town

| Town | Cost |
|------|------|
| Aldebaran | 600z |
| Prontera | 800z |
| Morroc | 800z |
| Geffen | 800z |
| Payon | 800z |
| Alberta | 800z |
| Archer Village | 1,200z |

#### Pushcart Speed Penalty by Skill Level

| Level | Speed Penalty |
|-------|--------------|
| 1 | -45% movement speed |
| 2 | -40% |
| 3 | -35% |
| 4 | -30% |
| 5 | -25% |
| 6 | -20% |
| 7 | -15% |
| 8 | -10% |
| 9 | -5% |
| 10 | **No penalty** (full speed) |

---

## 5. Buying Store System

### 5.1 Overview

The Buying Store is the reverse of Vending -- a player advertises items they WANT to buy, and other players sell TO the buying store. This was added in Episode 13.2+ and is considered a late-Classic / borderline-Renewal feature.

### 5.2 Requirements

| Requirement | Details |
|------------|---------|
| Skill | Open Buying Store (quest-learned Merchant skill, ID 2535 / ALL_BUYING_STORE) |
| Skill levels | 1 (single level in most implementations) |
| SP Cost | 30 |
| Catalyst | 1x Bulk Buyer Shop License (200z from Purchasing Team NPC in Alberta Merchant Guild) |
| Distance from NPC | Must be at least 4 cells away |
| Prerequisite skill | Vending Lv. 1 |
| Inventory requirement | Must already possess at least 1 of each item you want to buy |
| Quest | Must complete the Open Buying Store quest (10,000z + 1 Bulk Buyer Shop License) |

### 5.3 Mechanics

- **Maximum 5 item types** can be advertised per buying store
- **Eligible items**: Only Etc. (miscellaneous) items and non-brewed consumable items
  - NO equipment, NO quest items, NO brewed potions
- **Weight limit check**: Can only purchase up to `floor(0.9 * MaxWeight) - CurrentWeight - 1` in total weight
- Shop title displayed above character (same as vending)
- Other players click on the buying store character to see what they want
- Sellers drag items from their inventory and sell at the advertised price

### 5.4 Buying Store Tax

Same as vending: **5% commission on items priced over 100,000,000z** (100 million).

### 5.5 Implementation Priority

Buying Store is a LOWER priority than Vending and Trading for Sabri_MMO. It was added later in RO's lifecycle and is not essential for the core Classic experience. Implement after Vending is stable.

---

## 6. Item Drop System & Economy

### 6.1 Monster Drop Mechanics

Each monster can drop up to **8 different items** (plus 1 card slot), each with an individual drop rate. The item database defines these per-monster.

| Drop Type | Typical Rate Range | Notes |
|-----------|--------------------|-------|
| Common drops | 5% - 50% | Jellopy, Feather, basic materials |
| Uncommon drops | 1% - 5% | Ores, better materials |
| Rare drops | 0.1% - 1% | Equipment, special items |
| Card drops | **0.01% - 0.02%** | Cards; usually fixed at 0.01% (1 in 10,000) |
| MVP reward drops | 1% - 5% | Equipment/items given directly to MVP killer |

### 6.2 Drop Rate Modifiers

| Modifier | Effect | Source |
|----------|--------|--------|
| Server rate multiplier | Applied to all drop rates (e.g., 1x, 2x, 5x) | Server config |
| Bubble Gum | +100% drop rate for 30 minutes | Cash shop / event item |
| HE Bubble Gum | +200% drop rate for 15 minutes | Cash shop |
| Equipment bonuses | Certain headgear/accessories increase drop rate | Card/item effects |

**Drop Rate Cap**: Maximum effective drop rate is capped at 90% (prevents 100% guarantees through stacking modifiers). Cards are an exception -- their base rate is so low that even with modifiers they rarely exceed 1%.

### 6.3 Ground Item Mechanics

When a monster dies and drops items:

1. Items appear as sprites on the ground at the monster's death location
2. Item name label displayed below the sprite
3. Items are subject to **ownership priority** (see below)
4. Items persist on the ground for a configurable time (default: **60,000 ms / 60 seconds**)
5. After the timer expires, the item despawns permanently

### 6.4 Ownership Priority System (rAthena: first_get_charid)

Ground items have a tiered ownership system controlled by three character ID fields and corresponding tick timers:

#### Normal Monster Drops

| Phase | Duration | Who Can Loot | rAthena Field |
|-------|----------|-------------|---------------|
| 1st priority | 3,000 ms (3s) | Only the player who dealt the most damage | `first_get_charid` / `first_get_tick` |
| 2nd priority | 1,000 ms (1s) | Top 2 damage dealers | `second_get_charid` / `second_get_tick` |
| 3rd priority | 1,000 ms (1s) | Top 3 damage dealers | `third_get_charid` / `third_get_tick` |
| Free-for-all | Remaining lifetime | Anyone | All get fields cleared |

#### MVP Monster Drops

| Phase | Duration | Who Can Loot |
|-------|----------|-------------|
| 1st MVP | 10,000 ms (10s) | MVP killer only |
| 2nd MVP | 10,000 ms (10s) | Top 2 damage dealers |
| 3rd MVP | 2,000 ms (2s) | Top 3 damage dealers |
| Free-for-all | Remaining lifetime | Anyone |

#### MVP Reward Items
MVP reward items (the special drops given directly to the MVP killer) bypass the ground item system entirely -- they go directly into the MVP killer's inventory.

### 6.5 Pickup Range

Players must be within approximately **1-2 cells** (~50-100 UE units) of the ground item to pick it up. Click on the item to pick up.

### 6.6 Party Loot Distribution

Parties have two independent loot settings:

#### Who Can Loot (Item Share)
| Mode | Behavior |
|------|----------|
| Each Take | Only party members who dealt damage can pick up drops |
| Party Share | All party members in the area can pick up drops regardless of participation |

#### Where Loot Goes (Item Distribution)
| Mode | Behavior |
|------|----------|
| Individual | Items go to whoever physically picks them up |
| Shared | Items are randomly assigned to a party member when picked up |

### 6.7 Player-Dropped Items

Players can manually drop items:
- Drag item outside inventory window
- Stackable items: prompted for quantity
- Same 60-second ground lifetime
- Bound/quest items cannot be dropped

### 6.8 Item Value Tiers (Economy Reference)

| Tier | Examples | Typical Player-Market Value |
|------|---------|----------------------------|
| Trash | Jellopy, Sticky Mucus, Feather | 1z - 50z (NPC sell only) |
| Common loot | Iron, Steel, Oridecon, Elunium | 100z - 10,000z |
| Consumables | White Potion, Blue Potion, Yggdrasil Berry | 1,000z - 30,000z |
| Low equipment | Lv.1-2 weapons, basic armor | 500z - 50,000z |
| Mid equipment | Lv.3 weapons, slotted armor | 50,000z - 500,000z |
| High equipment | Lv.4 weapons, rare armor | 500,000z - 10,000,000z |
| Rare drops | MVP equipment, rare accessories | 1M - 100Mz |
| Cards (common) | Poring Card, Drops Card | 10,000z - 500,000z |
| Cards (useful) | Andre Card (+20 ATK), Peco Peco Card | 500,000z - 5,000,000z |
| Cards (rare) | Thara Frog, Marc, Bathory | 5M - 50Mz |
| Cards (MVP) | Baphomet Card, Osiris Card, Moonlight Flower | 50M - 2,000,000,000z |
| God items | Mjolnir, Sleipnir, Brisingamen | Priceless (quest-only) |

### 6.9 Key Economy Commodities

| Commodity | Why Valuable |
|-----------|-------------|
| Blue Potions | SP recovery; NOT sold by NPCs; must be hunted/crafted |
| Yggdrasil Berry/Leaf | Full HP/SP restore; rare drop only |
| Oridecon/Elunium | Required for weapon/armor refinement |
| Cards | Slot into equipment; extremely rare (0.01%) |
| Elemental weapons | Fire/Ice/Lightning/Earth for element advantage |
| Slotted equipment | More card slots = exponentially more valuable |
| +7/+8/+9/+10 refined equipment | Exponentially risky to create; very high demand |

---

## 7. Storage System

### 7.1 Kafra Storage (Personal)

| Property | Value |
|----------|-------|
| Max slots | **300** (Classic) / 600 (later patches) |
| Access fee | **40z** per access |
| Access NPC | Kafra Employee (most towns) |
| Prerequisite | Basic Skill Lv. 6 (Novice skill) |
| Shared across | All characters on the same account |
| Free Ticket | Consumable item that bypasses the 40z fee for one access |

### 7.2 Guild Storage

| Property | Value |
|----------|-------|
| Base slots | 100 (Guild Warehouse Expansion Lv.1) |
| Max slots | 500 (Lv.5) |
| Access | Guild members with permission set by Guild Master |
| Expansion per level | +100 slots per skill level (100/200/300/400/500) |

### 7.3 Kafra Teleport Service

| From/To | Typical Cost |
|---------|-------------|
| Same region (close) | 600z |
| Different region (mid) | 1,200z |
| Cross-continent (far) | 2,400z - 3,000z |

Save point service is free. Teleport destinations depend on Kafra location.

---

## 8. Refinement Costs (Zeny Sink)

Refinement is the primary zeny and item sink in the economy.

### 8.1 Materials Required

| Equipment | Material | Source | Material Cost |
|-----------|----------|--------|---------------|
| Weapon Lv. 1 | Phracon | NPC shop | 200z each |
| Weapon Lv. 2 | Emveretarcon | NPC shop | 1,000z each |
| Weapon Lv. 3 | Oridecon | Monster drop | ~5,000-15,000z (player market) |
| Weapon Lv. 4 | Oridecon | Monster drop | ~5,000-15,000z (player market) |
| All Armor | Elunium | Monster drop | ~5,000-15,000z (player market) |

Ore refining: 5x Rough Oridecon -> 1 Oridecon, 5x Rough Elunium -> 1 Elunium (free NPC service).

### 8.2 Zeny Cost Per Attempt

| Equipment | Zeny per Attempt |
|-----------|-----------------|
| Weapon Lv. 1 | 50z |
| Weapon Lv. 2 | 200z |
| Weapon Lv. 3 | 5,000z |
| Weapon Lv. 4 | 20,000z |
| Armor | 2,000z |

### 8.3 Safe Limits & Success Rates (Pre-Renewal)

| Upgrade | Wep Lv.1 | Wep Lv.2 | Wep Lv.3 | Wep Lv.4 | Armor |
|---------|----------|----------|----------|----------|-------|
| +1 to +4 | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 100% | 60% | 60% |
| +6 | 100% | 100% | 60% | 40% | 40% |
| +7 | 100% | 60% | 50% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 19% | 19% | 19% | 9% | 9% |

**Failure = DESTRUCTION**: Equipment is destroyed on failure. All socketed cards are also lost. Zeny and ore consumed regardless. No downgrade mechanic in pre-Renewal.

---

## 9. Vendor Search System

The `@whosell` command (or search NPC) allows players to search all active vending shops by item name. Results are sorted by lowest price first. This is critical for economy health:

- Naturally suppresses price gouging (cheapest vendor is instantly discoverable)
- Reduces information asymmetry between buyers and sellers
- Creates efficient price discovery
- Community consensus: "one of the greatest mechanisms for keeping an economy in check"

### Implementation

```
Server command: @whosell <item_name>
Response: List of vendors selling that item, sorted by price ascending
  - Vendor name
  - Map location (coordinates)
  - Price per unit
  - Quantity available

Also: @whobuy <item_name> for Buying Store searches
```

---

## 10. Implementation Checklist

### Phase 1: Core Economy (Must Have)
- [ ] Zeny storage as BIGINT (2^31-1 cap with overflow protection)
- [ ] Atomic `modifyZeny()` function with row locking (SELECT FOR UPDATE)
- [ ] Zeny transaction audit log table
- [ ] NPC buy/sell with base prices
- [ ] Discount skill integration (reduce NPC buy prices)
- [ ] Overcharge skill integration (increase NPC sell prices)
- [ ] Haggle/Compulsion Discount for Rogues (non-stacking with Discount)
- [ ] Weight limit validation on all item-receiving operations

### Phase 2: Player Trading (Must Have)
- [ ] Trade request/accept/deny flow
- [ ] Trade window: 10 item slots + zeny field per side
- [ ] Two-phase lock: OK (lock side) then Trade (commit)
- [ ] Cancel at any point before commit
- [ ] Auto-cancel on distance/disconnect/death/map change
- [ ] Server-side atomic transaction (BEGIN/COMMIT/ROLLBACK)
- [ ] Item ownership re-validation at commit time
- [ ] Weight validation for receiving player
- [ ] Zeny overflow protection for both parties
- [ ] Bound/quest/untradeable item check
- [ ] Trade audit logging
- [ ] Item locking (prevent use/equip/sell while in trade window)

### Phase 3: Vending System (Must Have)
- [x] Cart system (Pushcart inventory, 8000 weight, 100 slots)
- [x] Vending skill with level-based max stacks (3 + level - 1)
- [x] Shop setup window (title, item selection, price setting)
- [x] Shop title display above vendor's head
- [x] Browse window for buyers (item list, quantities, prices)
- [x] Purchase flow with server validation
- [x] Vending tax (5% on items priced > 100,000,000z)
- [x] Vendor immobility (sitting, cannot move/attack/skill)
- [x] Shop auto-close when all items sold
- [x] Manual shop close with sales summary
- [x] Vendor can send/receive whispers while vending
- [ ] Vendor search system (@whosell / search NPC)
- [ ] 4-cell NPC distance check for shop placement

### Phase 4: Ground Items (Should Have)
- [ ] Item sprites on ground at monster death location
- [ ] 60-second ground item lifetime with despawn
- [ ] Ownership priority system (3 tiers + FFA)
- [ ] MVP extended priority timers (10s/10s/2s)
- [ ] Pickup range validation (1-2 cells)
- [ ] Party loot distribution modes (Each Take / Party Share)
- [ ] Player item dropping (drag outside inventory)

### Phase 5: Storage (Should Have)
- [ ] Kafra storage (300 slots, 40z fee, account-shared)
- [ ] Free Ticket for Kafra Storage item
- [ ] Guild storage (100-500 slots based on guild skill)

### Phase 6: Buying Store (Nice to Have)
- [ ] Open Buying Store skill quest
- [ ] Bulk Buyer Shop License catalyst (200z)
- [ ] Max 5 item types, misc/consumable only
- [ ] Weight limit check
- [ ] Same tax rules as vending
- [ ] @whobuy search command

### Phase 7: Economy Polish (Nice to Have)
- [ ] Vendor search NPC (alternative to @whosell)
- [ ] Zeny transaction history viewer (player command)
- [ ] Economy metrics dashboard (admin)
- [ ] Bot detection for suspicious trade patterns

---

## 11. Gap Analysis (vs. Current Sabri_MMO Implementation)

### Already Implemented
- Cart system (CartSubsystem + SCartWidget, F10 toggle)
- Vending system (VendingSubsystem + SVendingSetupPopup + SVendingBrowsePopup)
- Vending tax (currently using 10M threshold -- should be updated to 100M per rAthena verification)
- Vendor immobility and shop sign via NameTagSubsystem
- Live sale log in vendor self-view
- Buyer quantity input
- Click-to-browse via PlayerInputSubsystem
- Item identification (SIdentifyPopup)
- NPC shop buy/sell
- Discount and Overcharge skills
- Basic zeny tracking (zuzucoin column on characters table)
- Kafra storage (basic implementation)

### Needs Implementation
1. **Player-to-Player Trading**: No trade window system exists yet. This is a high-priority gap.
2. **Vending Tax Threshold**: Currently set at 10,000,000z (10M). Should be **100,000,000z** (100M) per rAthena-verified official behavior.
3. **Ground Item System**: No ground items exist. Items are currently given directly to inventory on monster death.
4. **Item Ownership/Priority**: No loot priority system.
5. **Vendor Search**: No @whosell or search NPC functionality.
6. **Buying Store**: Not implemented.
7. **Zeny Transaction Logging**: No audit trail for zeny changes.
8. **Atomic Zeny Operations**: Current zeny modifications may not use row locking / transaction safety.
9. **Guild Storage**: Not implemented (guild system not yet built).
10. **Party Loot Distribution**: Party exists but loot modes not implemented.

### Critical Fixes Needed
- **Vending tax threshold**: Change from 10,000,000 to 100,000,000 in server constants
- **Zeny column type**: Verify characters.zuzucoin is BIGINT (not INTEGER) for overflow safety
- **Zeny overflow checks**: Ensure all zeny-modifying operations check against 2,147,483,647 cap

---

## Sources

- [Zeny - iRO Wiki](https://irowiki.org/wiki/Zeny)
- [Commerce - iRO Wiki](https://irowiki.org/wiki/Commerce)
- [Vending - iRO Wiki Classic](https://irowiki.org/classic/Vending)
- [Vending System - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Vending_System)
- [Buying Store - iRO Wiki](https://irowiki.org/wiki/Buying_Store)
- [Open Buying Store - iRO Wiki](https://irowiki.org/wiki/Open_Buying_Store)
- [Drop System - iRO Wiki](https://irowiki.org/wiki/Drop_System)
- [MVP - iRO Wiki](https://irowiki.org/wiki/MVP)
- [Overcharge - iRO Wiki](https://irowiki.org/wiki/Overcharge)
- [Discount - iRO Wiki](https://irowiki.org/wiki/Discount)
- [Kafra - iRO Wiki Classic](https://irowiki.org/classic/Kafra)
- [Pushcart - iRO Wiki](https://irowiki.org/wiki/Pushcart)
- [Merchant - iRO Wiki Classic](https://irowiki.org/classic/Merchant)
- [Making Zeny - iRO Wiki](https://irowiki.org/wiki/Making_Zeny)
- [Guide:Secure Trading - iRO Wiki](https://irowiki.org/wiki/Guide:Secure_Trading)
- [rAthena trade.cpp source](https://github.com/rathena/rathena/blob/master/src/map/trade.cpp)
- [rAthena vending.cpp source](https://github.com/rathena/rathena/blob/master/src/map/vending.cpp)
- [rAthena vending tax PR #2620](https://github.com/rathena/rathena/pull/2620/files/6c5f1b1f194890a17ffa21374da1122540ef556e)
- [rAthena map.cpp floor item ownership](https://github.com/rathena/rathena/blob/master/src/map/map.cpp)
- [rAthena buyingstore.cpp](https://github.com/rathena/rathena/blob/master/src/map/buyingstore.cpp)
- [rAthena drops.conf](https://github.com/rathena/rathena/blob/master/conf/battle/drops.conf)
- [Economy and Inflation in RO - RateMyServer](https://write.ratemyserver.net/ragnoark-online-ro-related-writings/economy-and-inflation-in-ragnarok-online/)
- [Bank - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Bank)
- [Overcharge - RateMyServer Skill DB](https://ratemyserver.net/index.php?page=skill_db&skid=38)
- [Discount - RateMyServer Skill DB](https://ratemyserver.net/index.php?page=skill_db&skid=37)
- [Open Buying Store - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=2535)
