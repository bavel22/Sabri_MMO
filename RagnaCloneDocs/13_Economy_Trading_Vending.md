# 13. Economy, Trading, Vending & Zeny System

> **Scope**: RO Classic (pre-Renewal) economy mechanics -- currency, NPC shops, player trading, vending, buying stores, item drops, storage, mail, auction, refinement costs, and economy balance.
> **Target**: Sabri_MMO implementation using UE5 (C++), Node.js server, PostgreSQL, Redis.

---

## Table of Contents

1. [Currency System (Zeny)](#1-currency-system-zeny)
2. [NPC Shop System](#2-npc-shop-system)
3. [Player-to-Player Trading](#3-player-to-player-trading)
4. [Vending System](#4-vending-system-merchant-skill)
5. [Buying Store System](#5-buying-store-system)
6. [Item Drop System](#6-item-drop-system)
7. [Storage System](#7-storage-system)
8. [Mail System (RODEX)](#8-mail-system-rodex)
9. [Auction System](#9-auction-system)
10. [Refinement & Upgrade Costs](#10-refinement--upgrade-costs)
11. [Economy Balance & Design](#11-economy-balance--design)
12. [Implementation Reference](#12-implementation-reference)

---

## 1. Currency System (Zeny)

### 1.1 Overview

Zeny is the sole currency in Ragnarok Online. All NPC transactions, player trades, services, and skill costs use Zeny. When prices are denoted with "Z" (e.g., `1,200z`), that means Zeny.

### 1.2 Limits

| Property | Value |
|----------|-------|
| Max Zeny per character | **2,147,483,647** (2^31 - 1, signed 32-bit integer) |
| Max Zeny per trade | **999,999,999** |
| Max vending item price | **1,000,000,000** |
| Min Zeny | 0 (cannot go negative) |

**Overflow protection**: Any transaction that would push a character over the 2,147,483,647 limit is rejected. The server must validate this on every Zeny-modifying operation.

### 1.3 Zeny Sources (How Players Earn Zeny)

| Source | Description |
|--------|-------------|
| **Monster drops sold to NPC** | Kill monsters, pick up loot, sell to NPC shops. Primary income for most players. |
| **Selling items to NPC** | Base sell price = 50% of NPC buy price. Modified by Overcharge skill. |
| **Player-to-player sales (Vending)** | Merchant class sets up player shops with custom prices. |
| **Direct player trades** | Trade window exchanges of items + Zeny. |
| **Quest rewards** | Some quests give Zeny directly. Amounts vary (100z to 100,000z+). |
| **Monster Zeny drops** | Some monsters drop Zeny directly (separate from item drops). |

### 1.4 Zeny Sinks (How Players Spend Zeny)

| Sink | Typical Cost Range |
|------|-------------------|
| **NPC shop purchases** | 1z (Arrow) to 100,000z+ (high-tier equipment) |
| **Kafra Storage access** | 40z per access (Classic servers) |
| **Kafra Teleport** | 600z - 3,000z per teleport depending on distance |
| **Kafra Cart Rental** | 600z - 1,200z depending on town |
| **Skill: Mammonite** | 100z - 1,000z per use (scales with skill level) |
| **Skill: Maximum Power-Thrust** | 2,500 + (Level x 500)z per use |
| **Equipment Refinement** | 50z - 20,000z per attempt (plus ore cost) |
| **Equipment Repair** | 5,000z per broken equipment piece |
| **Vending Tax** | 5% commission on items priced over 10,000,000z |
| **Auction listing fee** | 12,000z per hour (up to 864,000z for 72h listing) |
| **Mail system fees** | 2,500z per item attached + 2% of Zeny sent |
| **Buying Store license** | 200z per Bulk Buyer Shop License |

---

## 2. NPC Shop System

### 2.1 Buy/Sell Price Formula

```
NPC Buy Price  = Item's base price (fixed, defined in item database)
NPC Sell Price = floor(Buy Price / 2)   -- 50% of buy price

With Discount skill (buying):
  Discounted Price = floor(Buy Price * (100 - DiscountRate) / 100)

With Overcharge skill (selling):
  Overcharged Price = floor(Sell Price * (100 + OverchargeRate) / 100)
```

### 2.2 Discount Skill (Merchant Passive)

Reduces NPC buy prices. Prerequisite: Enlarge Weight Limit Lv. 3.

| Level | Discount Rate |
|-------|--------------|
| 1 | 7% |
| 2 | 9% |
| 3 | 11% |
| 4 | 13% |
| 5 | 15% |
| 6 | 17% |
| 7 | 19% |
| 8 | 21% |
| 9 | 23% |
| 10 | **24%** |

Available to: Merchant, Super Novice.

**Example**: White Potion (NPC price 1,200z) with Discount Lv.10:
`floor(1200 * (100 - 24) / 100) = floor(1200 * 0.76) = 912z`

### 2.3 Overcharge Skill (Merchant Passive)

Increases NPC sell prices. Prerequisite: Discount Lv. 3.

| Level | Overcharge Rate |
|-------|----------------|
| 1 | 7% |
| 2 | 9% |
| 3 | 11% |
| 4 | 13% |
| 5 | 15% |
| 6 | 17% |
| 7 | 19% |
| 8 | 21% |
| 9 | 23% |
| 10 | **24%** |

Available to: Merchant, Super Novice.

**Example**: Tsurugi (buy price 21,000z, sell price 10,500z) with Overcharge Lv.10:
`floor(10500 * 1.24) = 13,020z`

### 2.4 NPC Shop Types

#### Tool Dealers
Found in every major town. Sell consumables and basic supplies.

**Prontera Tool Dealer** (representative inventory):

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
| Monocle | 10,000z | Headgear |

**Note**: SP recovery items (Grape Juice, Blue Potion) are NOT sold by NPCs. They must be hunted from monsters or crafted, making them a key player-economy commodity.

#### Weapon Dealers
Each town sells weapons matching the local class culture.

**Prontera Weapon Dealer** (sample items):

| Item | Buy Price | ATK | Weapon Lv |
|------|-----------|-----|-----------|
| Knife | 50z | 17 | 1 |
| Cutter | 1,250z | 30 | 1 |
| Main Gauche | 2,400z | 43 | 1 |
| Sword | 100z | 25 | 1 |
| Falchion | 1,500z | 39 | 1 |
| Blade | 2,900z | 53 | 1 |
| Rapier | 10,000z | 70 | 2 |
| Scimitar | 17,000z | 85 | 2 |
| Katana | 2,000z | 60 | 1 |
| Tsurugi | 21,000z | 130 | 3 |
| Flamberge | 60,000z | 150 | 3 |
| Haedonggum | 50,000z | 120 | 3 |
| Axe | 500z | 38 | 1 |
| Javelin | 150z | 28 | 1 |
| Spear | 1,700z | 44 | 1 |
| Pike | 3,450z | 60 | 1 |
| Halberd | 54,000z | 165 | 3 |
| Lance | 60,000z | 185 | 3 |
| Bow | 1,000z | 15 | 1 |

#### Armor Dealers
Each town sells armor/shields/accessories.

**Prontera Armor Dealer** (sample items):

| Item | Buy Price | DEF | Type |
|------|-----------|-----|------|
| Guard | 500z | 3 | Shield |
| Buckler | 14,000z | 4 | Shield |
| Cotton Shirt | 10z | 1 | Armor |
| Jacket | 200z | 2 | Armor |
| Adventurer's Suit | 1,000z | 3 | Armor |
| Wooden Mail | 5,200z | 4 | Armor |
| Mantle | 32,000z | 5 | Armor |
| Coat | 56,000z | 6 | Armor |
| Chain Mail | 65,000z | 7 | Armor |
| Sandals | 400z | 1 | Footgear |
| Shoes | 3,500z | 2 | Footgear |
| Hood | 1,000z | 1 | Garment |
| Muffler | 5,000z | 2 | Garment |
| Hat | 1,000z | 2 | Headgear |
| Cap | 6,000z | 3 | Headgear |

#### Specialty Shops
- **Gemstone dealers**: Blue Gemstone (1,500z), Yellow Gemstone (1,500z) -- required for skill casting (e.g., Warp Portal, Safety Wall)
- **Arrow crafting NPCs**: Convert items into specialized arrows
- **Pet food shops**: Taming items and pet food for the pet system
- **Cooking ingredient shops**: Materials for the cooking system
- **Elemental stone NPCs**: Star Crumb, various stones for forging

### 2.5 Shop Inventory Per Town

Each town has a distinct economy flavor:

| Town | Specialty | Notable Items |
|------|-----------|---------------|
| **Prontera** | General/Swordsman/Knight | Full weapon range, armor up to Chain Mail |
| **Geffen** | Mage | Rods, Wands, staffs, Blue/Yellow Gemstones |
| **Payon** | Archer/Thief | Bows, daggers, arrows, traps |
| **Morroc** | Thief/Assassin | Daggers, katars, desert supplies |
| **Alberta** | Merchant/Marine | Trading goods, merchant supplies |
| **Aldebaran** | General | Transit hub, Kafra HQ |
| **Juno** | Sage | Books, sage equipment |
| **Rachel** | Crusader | Holy-themed equipment |

---

## 3. Player-to-Player Trading

### 3.1 Trade Window Mechanics

The trade window allows two players to exchange items and Zeny directly.

#### Initiating a Trade
1. Right-click another player character
2. Select "Deal" / "Request Deal" from the context menu
3. Target player receives a trade request popup
4. Target player clicks "Accept" to open the trade window, or "Deny" to reject

#### Trade Window Layout
Each side of the trade window shows:
- **10 item slots** (confirmed via rAthena `MAX_DEAL = 10`)
- **1 Zeny input field** (max 999,999,999z per trade)
- **"OK" button** (lock/confirm your side)
- **"Trade" button** (finalize the exchange)
- **"Cancel" button** (abort the trade at any time)

#### Trade Process (Step by Step)
```
1. Player A right-clicks Player B -> selects "Deal"
2. Player B receives popup -> clicks "Accept"
3. Trade window opens for both players
4. Both players drag items from inventory to trade window
   - Alt+Right-click on an item in inventory sends it to the active trade window
   - Stackable items: prompted for quantity
5. Both players enter Zeny amount (optional)
6. Player A clicks "OK" (locks their side -- items/zeny cannot be changed)
7. Player B clicks "OK" (locks their side)
8. Both sides now show as "locked"
9. Either player clicks "Trade" to finalize
10. Server validates and executes atomic exchange
11. Trade window closes, items/zeny transferred
```

#### Cancel Behavior
- Either player can click "Cancel" at ANY point before the final "Trade" confirmation
- If either player walks too far away, the trade auto-cancels
- If either player logs out, the trade auto-cancels

### 3.2 Trade Restrictions

| Restriction | Details |
|------------|---------|
| **Bound items** | Account-bound and character-bound items cannot be placed in the trade window |
| **Quest items** | Most quest items cannot be traded |
| **Weight check** | Trade fails if receiving player would exceed weight limit |
| **Zeny overflow** | Trade fails if receiving player's Zeny would exceed 2,147,483,647 |
| **Distance** | Players must be on the same map and within close proximity |
| **Dead players** | Cannot trade while dead |
| **Vending** | Cannot trade while a vending shop is open |
| **Storage open** | Cannot trade while storage is open |

### 3.3 Trade Security (Server-Side)

The server MUST enforce:
1. **Atomic transactions**: Both sides commit or neither does -- no partial trades
2. **Item ownership verification**: Confirm each item belongs to the trading player
3. **Duplicate prevention**: Items are locked (removed from inventory) when placed in trade window
4. **Zeny validation**: Verify sufficient funds on both sides
5. **Weight validation**: Check receiving player has capacity
6. **Bound/tradeable check**: Verify items are allowed to be traded
7. **Zeny cap check**: Prevent overflow past 2,147,483,647
8. **Audit logging**: Record every completed trade with timestamps, player IDs, items, amounts

---

## 4. Vending System (Merchant Skill)

### 4.1 Prerequisites

| Requirement | Details |
|------------|---------|
| **Job class** | Merchant, Blacksmith, Whitesmith, Alchemist, Creator, Super Novice |
| **Skill: Pushcart** | Lv. 3 minimum (prerequisite for Vending) |
| **Skill: Vending** | Lv. 1-10 (determines max item stacks) |
| **Cart equipped** | Must have a Pushcart rented and active |
| **Items in cart** | Only items in the Pushcart inventory can be vended |
| **SP Cost** | 30 SP per activation |

### 4.2 Vending Skill Levels

| Level | Max Item Stacks | Prerequisite |
|-------|----------------|--------------|
| 1 | 3 | Pushcart Lv.3 |
| 2 | 4 | -- |
| 3 | 5 | -- |
| 4 | 6 | -- |
| 5 | 7 | -- |
| 6 | 8 | -- |
| 7 | 9 | -- |
| 8 | 10 | -- |
| 9 | 11 | -- |
| 10 | **12** | -- |

Each "stack" = one item entry. For stackable items (potions, ores), a single stack can hold multiple units of the same item.

### 4.3 Setting Up a Vending Shop

1. Merchant must have Pushcart equipped with items to sell
2. Use Vending skill (hotkey or skill window)
3. **Shop Setup Window** appears:
   - **Shop Title**: Text input for the shop name (displayed above character head)
   - **Item Selection**: List of all items in Pushcart
   - Select items to vend (up to max stacks for skill level)
   - **Set Price**: For each selected item, enter the per-unit price
   - **Price range**: 1z to 1,000,000,000z per unit
4. Click "OK" to open the shop
5. Character sits on the ground with shop title floating above head
6. Must be at least **4 cells away from any NPC** (prevents obscuring NPC interaction)

### 4.4 Shop Title Display

- Shown as a text bubble/banner above the vending character's head
- Visible to all players on the same map
- Players typically include item names and prices in the title
- Maximum title length: ~80 characters (client-enforced)
- Examples: `Selling +7 Blade [3] 500k`, `CHEAP White Potions!!`

### 4.5 Buying from a Vending Shop

1. Click on the vending character
2. **Vending Browse Window** appears showing:
   - Shop title at top
   - List of items with: Item name, quantity available, price per unit
3. Select an item
4. Enter quantity to purchase (for stackable items)
5. Window shows calculated total price
6. Click "Buy" to confirm purchase
7. Zeny deducted from buyer, items added to buyer inventory
8. Items removed from seller's Pushcart

### 4.6 Vending Tax

- **Items priced at or below 10,000,000z**: No tax (vendor receives 100%)
- **Items priced above 10,000,000z**: 5% commission fee
  - Vendor receives 95% of the listed price
  - The 5% is destroyed (Zeny sink)

### 4.7 Shop Closure

A vending shop closes when:
- All items are sold out (auto-close)
- The merchant manually cancels the shop
- The merchant disconnects or is kicked
- Server maintenance/restart

**Closing Report**: When a shop closes, the vendor sees a summary:
- Number of items sold
- Buyer names
- Time of each sale
- Total Zeny earned

### 4.8 Vending Behavior

- Vendor character is **sitting** and **immobile** while shop is open
- Vendor CANNOT move, attack, use skills, chat (publicly), or open other windows
- Vendor CAN whisper/receive whispers
- **AFK vending** is a core gameplay pattern -- merchants set up shop and go AFK
- Common vending locations: near Kafra NPCs, town centers, dungeon entrances

### 4.9 Pushcart Details

| Property | Value |
|----------|-------|
| Weight capacity | 8,000 |
| Item slots | 100 |
| Rental NPC | Kafra Employee |
| Rental cost | 600z - 1,200z (varies by town) |
| Required skill | Pushcart Lv. 1+ |
| Available to | Merchant class, Super Novice |

**Pushcart Speed Penalty by Skill Level**:

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
| 10 | **No penalty** |

**Cart Rental Costs by Town**:

| Town | Cost |
|------|------|
| Aldebaran | 600z |
| Prontera | 800z |
| Morroc | 800z |
| Geffen | 800z |
| Payon | 800z |
| Alberta | 800z |
| Archer Village | 1,200z |

---

## 5. Buying Store System

### 5.1 Overview

The Buying Store is the reverse of Vending -- a player advertises items they WANT to buy, and other players sell TO the buying store. This was added later in RO's lifecycle.

### 5.2 Requirements

| Requirement | Details |
|------------|---------|
| **Skill** | Open Buying Store (quest-learned Merchant skill) |
| **Skill levels** | 1-2 (not much difference) |
| **SP Cost** | 30 |
| **Catalyst** | 1x Bulk Buyer Shop License (200z from Purchasing Team NPC in Alberta) |
| **Distance from NPC** | Must be at least 4 cells away |
| **Inventory requirement** | Must already possess 1 of each item you want to buy |

### 5.3 Mechanics

- **Maximum 5 items** can be advertised per buying store
- **Item restrictions**: Only Etc. (miscellaneous) items and non-brewed consumable items are eligible
  - NO equipment, NO quest items, NO potion-creation items
- **Weight limit**: Can only purchase up to `(0.9 * MaxWeight) - CurrentWeight - 1` in weight
- Shop title displayed above character (same as vending)
- Other players click on the buying store character to see what they want
- Sellers drag items from their inventory to sell at the advertised price

### 5.4 Buying Store Tax

Same as vending: **5% commission on items priced over 10,000,000z**.

---

## 6. Item Drop System

### 6.1 Monster Drop Mechanics

- Each monster can drop up to **8 different items**, each with an individual drop rate
- Drop rates range from **0.01%** (rare cards) to **100%** (guaranteed drops)
- Most common drops: 5% - 50%
- Card drops: Typically **0.01% - 0.02%**
- MVP drops: Typically **1% - 5%** for equipment, higher for consumables

### 6.2 Drop Rate Modifiers

| Modifier | Effect |
|----------|--------|
| **Server rate** | Multiplier applied to all drop rates (e.g., 1x, 2x, 5x) |
| **Bubble Gum** | +100% drop rate for 30 minutes |
| **HE Bubble Gum** | +200% drop rate for 15 minutes |
| **Equipment bonuses** | Certain headgear/accessories increase drop rate |
| **VIP status** | Additional drop rate bonus (server-dependent) |

**Drop Rate Cap**: Maximum 90% effective drop rate (prevents 100% guarantees through stacking modifiers).

### 6.3 Ground Item Mechanics

When a monster dies and drops items:

1. Item sprites appear on the ground at the monster's death location
2. Item name label displayed below the sprite
3. Items are subject to **ownership priority** (see below)
4. Items persist on the ground for a limited time
5. Any player within pickup range can attempt to pick up (subject to ownership)

### 6.4 Ownership Priority (Drop Protection)

Items have a tiered ownership system based on damage dealt:

| Phase | Duration | Who Can Loot |
|-------|----------|--------------|
| **1st priority** | 3,000 ms (3 seconds) | Only the player who dealt the most damage |
| **2nd priority** | 1,000 ms (1 second) | Top 2 damage dealers |
| **3rd priority** | 1,000 ms (1 second) | Top 3 damage dealers |
| **Free-for-all** | Remaining lifetime | Anyone |

**MVP Items** have extended priority timers:

| Phase | Duration | Who Can Loot |
|-------|----------|--------------|
| **1st MVP** | 10,000 ms (10 seconds) | MVP killer only |
| **2nd MVP** | 10,000 ms (10 seconds) | Top 2 damage dealers |
| **3rd MVP** | 2,000 ms (2 seconds) | Top 3 damage dealers |
| **Free-for-all** | Remaining lifetime | Anyone |

### 6.5 Ground Item Lifetime

| Property | Default Value |
|----------|--------------|
| **Floor item lifetime** | 60,000 ms (60 seconds) |
| After 60 seconds | Item despawns and is permanently lost |

### 6.6 Pickup Range

Players must be within approximately **1-2 cells** (about 50-100 UE units) of the ground item to pick it up. Click on the item or use the `/pickall` command (if enabled).

### 6.7 Party Loot Distribution

Parties have two independent loot settings:

#### Who Can Loot (Item Share)
| Mode | Behavior |
|------|----------|
| **Each Take** | Only party members who dealt damage to the monster can pick up its drops |
| **Party Share** | All party members in the area can pick up the drops regardless of who killed the monster |

#### Where Loot Goes (Item Distribution)
| Mode | Behavior |
|------|----------|
| **Individual** | Items go to whoever physically picks them up |
| **Shared** | Items are randomly assigned to a party member when picked up |

### 6.8 Player-Dropped Items

Players can manually drop items from their inventory:
- Drag the item outside the inventory window
- For stackable items: prompted for quantity to drop
- Dropped items appear on the ground with the same 60-second lifetime
- **Bound items**: Cannot be dropped (teleport back to inventory if attempted)
- **Quest items**: Most cannot be dropped

---

## 7. Storage System

### 7.1 Kafra Storage (Personal)

| Property | Value |
|----------|-------|
| **Max slots** | 300 (Classic) / 600 (later patches) |
| **Access fee** | 40z per access (Classic) |
| **Access NPC** | Kafra Employee (most towns) |
| **Prerequisite** | Basic Skill Lv. 6 (Novice skill) |
| **Shared across** | All characters on the same account |
| **Restrictions** | Some bound/untradeable items cannot be stored |

**Free Ticket for Kafra Storage**: Consumable item that bypasses the storage fee for one access. Given to new characters.

### 7.2 Guild Storage

| Property | Value |
|----------|-------|
| **Base slots** | 100 (Guild Warehouse Expansion Lv.1) |
| **Max slots** | 500 (Guild Warehouse Expansion Lv.5) |
| **Access** | Guild members with permission |
| **Prerequisite** | Guild Warehouse Expansion skill (guild skill) |
| **Shared across** | All authorized guild members |

**Guild Warehouse Expansion Skill Levels**:

| Level | Slots |
|-------|-------|
| 1 | 100 |
| 2 | 200 |
| 3 | 300 |
| 4 | 400 |
| 5 | 500 |

Access permissions are set by the Guild Master.

### 7.3 Pushcart Storage (Merchant Only)

| Property | Value |
|----------|-------|
| **Max slots** | 100 |
| **Max weight** | 8,000 |
| **Available to** | Merchant class, Super Novice with Pushcart skill |
| **Access** | Anytime (inventory-like, but separate) |
| **Key feature** | Items in cart can be vended through Vending skill |

### 7.4 Player Inventory

| Property | Value |
|----------|-------|
| **Item slots** | Unlimited (weight-limited, not slot-limited) |
| **Weight limit** | Base: `(STR * 30) + (BaseJobLevel * 200) + 2000` |
| **70% weight** | "Minor overweight" -- HP/SP recovery disabled |
| **90% weight** | "Major overweight" -- cannot attack or use most skills |

**Enlarge Weight Limit** (Merchant skill): +200 weight per level, up to +2,000 at Lv.10.

---

## 8. Mail System (RODEX)

RODEX (Ragnarok Online Delivery Express) allows players to mail items and Zeny to other players.

### 8.1 Mechanics

| Property | Value |
|----------|-------|
| **Max items per mail** | 5 different items |
| **Max weight per mail** | 4,000 weight |
| **Item attachment fee** | 2,500z per item type attached (quantity does not affect fee) |
| **Zeny transfer fee** | 2% of Zeny amount sent |
| **Daily send limit** | 300 mails per day |
| **Mail expiry** | 15 days (unread mail returned to sender) |

### 8.2 Restrictions

- Cannot send mail to characters on the same account
- Cannot send mail while storage is open
- Bound items cannot be mailed
- Quest items cannot be mailed

### 8.3 Mail UI

- Inbox: Received mail
- Outbox: Sent mail
- Return box: Expired/returned mail
- Compose: Write new mail with text body, item attachments, Zeny amount
- Read: View mail, claim attached items/Zeny

**Note**: RODEX replaced the original simple Mail System. Offline vending proceeds are delivered via RODEX when applicable.

---

## 9. Auction System

### 9.1 Overview

The Auction System allows players to list items for bidding. It was implemented but remained largely unpopular and was eventually disabled on many servers due to bugs and high fee structure.

### 9.2 Mechanics

| Property | Value |
|----------|-------|
| **Min bid** | 10,000,000z |
| **Max price** | 990,000,000z |
| **Max duration** | 72 hours |
| **Listing fee** | 12,000z per hour |
| **Max listing cost** | 864,000z (72 hours) |
| **Categories** | Armor, Weaponry, Cards, Miscellaneous |

### 9.3 NPC Locations

Auction Hall Guide NPCs warp players to the Auction House:
- Prontera (217, 120)
- Juno (130, 116)
- Lighthalzen (206, 169)
- Morroc Ruins (78, 174)

**Important**: All auction halls share the same listing pool. An item listed in Juno is visible and biddable in Prontera.

### 9.4 Process

1. Speak to Auction Hall Guide NPC to enter the Auction House
2. **List an item**: Select item, set minimum bid, set "Buy Now" price, set duration
3. **Browse**: Search by item name or bid number; filter by category
4. **Bid**: Enter bid amount (must exceed current bid); Zeny held in escrow
5. **Buy Now**: Pay the "Buy Now" price to immediately win the auction
6. **Auction ends**: Winning bidder receives item via mail; seller receives Zeny
7. **No bids**: Unsold item returned to seller via mail

### 9.5 Restrictions

- Account-bound and character-bound items cannot be auctioned
- Items cannot be withdrawn once listed (must wait for auction to conclude)
- Listing fee is non-refundable

### 9.6 Implementation Recommendation

Given that the Auction System was unpopular and buggy even in official RO, consider it a **low-priority feature** for Sabri_MMO. Implement Trading and Vending first; add Auction only if there is player demand.

---

## 10. Refinement & Upgrade Costs

Refinement is a major Zeny sink and item sink. Players upgrade weapons and armor at Refine NPCs.

### 10.1 Required Materials

| Equipment | Material | Material Source |
|-----------|----------|----------------|
| Weapon Lv. 1 | Phracon | NPC shop (200z each) |
| Weapon Lv. 2 | Emveretarcon | NPC shop (1,000z each) |
| Weapon Lv. 3 | Oridecon | Monster drops (or refine from Rough Oridecon) |
| Weapon Lv. 4 | Oridecon | Monster drops (or refine from Rough Oridecon) |
| All Armor | Elunium | Monster drops (or refine from Rough Elunium) |

**Ore refining**: Rough Oridecon (5x) -> 1 Oridecon, Rough Elunium (5x) -> 1 Elunium (free NPC service).

### 10.2 Zeny Cost Per Refinement Attempt

| Equipment | Zeny per Attempt |
|-----------|-----------------|
| Weapon Lv. 1 | 50z |
| Weapon Lv. 2 | 200z |
| Weapon Lv. 3 | 5,000z |
| Weapon Lv. 4 | 20,000z |
| Armor | 2,000z |

### 10.3 Safe Limits (100% Success)

| Equipment | Safe Limit |
|-----------|-----------|
| Weapon Lv. 1 | +7 |
| Weapon Lv. 2 | +6 |
| Weapon Lv. 3 | +5 |
| Weapon Lv. 4 | +4 |
| All Armor | +4 |

Upgrades AT or BELOW the safe limit have a **100% success rate**.

### 10.4 Pre-Renewal Success Rates (Beyond Safe Limit)

| Upgrade | Weapon Lv.1 | Weapon Lv.2 | Weapon Lv.3 | Weapon Lv.4 | Armor |
|---------|-------------|-------------|-------------|-------------|-------|
| +1 to +4 | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 100% | 60% | 60% |
| +6 | 100% | 100% | 60% | 40% | 40% |
| +7 | 100% | 60% | 50% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 19% | 19% | 19% | 9% | 9% |

### 10.5 Failure Consequences (Pre-Renewal)

- **Equipment is DESTROYED** on failure
- All cards socketed in the equipment are also lost
- Zeny and ore are consumed regardless of success/failure
- There is NO downgrade mechanic in pre-Renewal -- it is all or nothing

### 10.6 ATK/DEF Bonus Per Upgrade Level

| Equipment | Bonus per +1 |
|-----------|-------------|
| Weapon Lv. 1 | +2 ATK |
| Weapon Lv. 2 | +3 ATK |
| Weapon Lv. 3 | +5 ATK |
| Weapon Lv. 4 | +7 ATK |
| Armor | +1 DEF per level |

**Over-upgrade bonus** (beyond +4 for armor, beyond safe limit for weapons): Additional ATK/DEF bonus applied.

---

## 11. Economy Balance & Design

### 11.1 Item Value Tiers

| Tier | Example Items | Typical Value Range |
|------|--------------|-------------------|
| **Trash** | Jellopy, Sticky Mucus, Feather | 1z - 50z (NPC sell only) |
| **Common loot** | Iron, Steel, Oridecon, Elunium | 100z - 10,000z |
| **Consumables** | White Potion, Blue Potion, Yggdrasil Berry | 1,000z - 30,000z |
| **Low equipment** | Lv.1-2 weapons, basic armor | 500z - 50,000z |
| **Mid equipment** | Lv.3 weapons, slotted armor | 50,000z - 500,000z |
| **High equipment** | Lv.4 weapons, rare armor | 500,000z - 10,000,000z |
| **Rare drops** | MVP equipment, rare accessories | 1,000,000z - 100,000,000z |
| **Cards (common)** | Poring Card, Drops Card | 10,000z - 500,000z |
| **Cards (useful)** | Andre Card (+20 ATK), Peco Peco Card | 500,000z - 5,000,000z |
| **Cards (rare)** | Thara Frog, Marc, Bathory | 5,000,000z - 50,000,000z |
| **Cards (MVP)** | Baphomet Card, Osiris Card, Moonlight Flower | 50,000,000z - 2,000,000,000z |
| **God items** | Mjolnir, Sleipnir, Brisingamen | Priceless (quest-only) |

### 11.2 Key Economy Commodities

Items that drive the player economy:

| Commodity | Why Valuable |
|-----------|-------------|
| **Blue Potions** | SP recovery; NOT sold by NPCs; must be hunted/crafted |
| **Yggdrasil Berry/Leaf** | Full HP/SP restore; rare drop only |
| **Oridecon/Elunium** | Required for weapon/armor refinement |
| **Cards** | Slot into equipment for permanent stat bonuses; extremely rare |
| **Elemental weapons** | Fire/Ice/Lightning/etc. weapons for element advantage |
| **Slotted equipment** | More card slots = more valuable |
| **+7/+8/+9/+10 refined equipment** | Exponentially risky to create; high demand |

### 11.3 Zeny Inflation Control

The primary mechanisms to control Zeny inflation:

| Mechanism | How It Works |
|-----------|-------------|
| **Refinement** | Huge Zeny sink; 20,000z per Lv.4 attempt, most fail above safe limit |
| **Vending tax** | 5% commission on high-value sales destroys Zeny |
| **Kafra services** | Small but constant drain (storage, teleport, cart rental) |
| **Mammonite** | Combat skill that consumes Zeny directly |
| **Mail fees** | 2% of Zeny transferred + per-item fees |
| **Auction fees** | 12,000z/hour for listing |
| **NPC-only consumables** | White Potions, Fly Wings, etc. drain Zeny constantly |
| **Repair costs** | 5,000z per broken equipment piece |

### 11.4 Vendor Search System

The vendor search system (where players can search all active vending shops by item name) is identified as "one of the greatest mechanisms for keeping an economy in check." It displays results sorted by lowest price first, naturally suppressing excessive pricing by making the cheapest vendor instantly discoverable.

### 11.5 Economy Design Principles for Sabri_MMO

1. **Balanced Zeny sinks**: Ensure outflow matches or slightly exceeds inflow
2. **SP items NOT from NPCs**: Blue Potion/Grape Juice must come from monster drops or crafting to maintain player economy
3. **Card rarity**: 0.01% drop rates create aspirational long-term goals
4. **Refinement risk**: Equipment destruction above safe limits is the primary item sink
5. **Vending creates markets**: Player shops are the backbone of the economy
6. **NPC prices anchor the economy**: Fixed NPC prices set floor values for common items
7. **Service fees everywhere**: Small constant drains (Kafra, teleport, repair) prevent hoarding

---

## 12. Implementation Reference

### 12.1 PostgreSQL Schema

```sql
-- === CHARACTER INVENTORY (already exists, extend as needed) ===
-- character_inventory table handles equipped items, inventory, and positions

-- === KAFRA STORAGE ===
CREATE TABLE character_storage (
    id              SERIAL PRIMARY KEY,
    account_id      INTEGER NOT NULL REFERENCES users(id),  -- shared across characters
    item_id         INTEGER NOT NULL,                        -- references items table
    amount          INTEGER NOT NULL DEFAULT 1,
    slot_position   INTEGER NOT NULL,                        -- 0-599
    refine_level    INTEGER NOT NULL DEFAULT 0,
    card_slot_1     INTEGER DEFAULT 0,
    card_slot_2     INTEGER DEFAULT 0,
    card_slot_3     INTEGER DEFAULT 0,
    card_slot_4     INTEGER DEFAULT 0,
    is_identified   BOOLEAN NOT NULL DEFAULT TRUE,
    is_damaged      BOOLEAN NOT NULL DEFAULT FALSE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (account_id, slot_position)
);
CREATE INDEX idx_storage_account ON character_storage(account_id);

-- === GUILD STORAGE ===
CREATE TABLE guild_storage (
    id              SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id),
    item_id         INTEGER NOT NULL,
    amount          INTEGER NOT NULL DEFAULT 1,
    slot_position   INTEGER NOT NULL,                        -- 0-499
    refine_level    INTEGER NOT NULL DEFAULT 0,
    card_slot_1     INTEGER DEFAULT 0,
    card_slot_2     INTEGER DEFAULT 0,
    card_slot_3     INTEGER DEFAULT 0,
    card_slot_4     INTEGER DEFAULT 0,
    is_identified   BOOLEAN NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (guild_id, slot_position)
);
CREATE INDEX idx_guild_storage_guild ON guild_storage(guild_id);

-- === TRADE LOG (Audit Trail) ===
CREATE TABLE trade_logs (
    id              SERIAL PRIMARY KEY,
    trade_type      VARCHAR(20) NOT NULL,                    -- 'player_trade', 'vend_purchase', 'npc_buy', 'npc_sell', 'storage', 'mail'
    player_a_id     INTEGER NOT NULL REFERENCES characters(id),
    player_b_id     INTEGER REFERENCES characters(id),       -- NULL for NPC transactions
    items_a         JSONB NOT NULL DEFAULT '[]',             -- items player A gave [{item_id, amount, refine, cards}]
    items_b         JSONB NOT NULL DEFAULT '[]',             -- items player B gave
    zeny_a          BIGINT NOT NULL DEFAULT 0,               -- zeny player A gave
    zeny_b          BIGINT NOT NULL DEFAULT 0,               -- zeny player B gave
    zone_name       VARCHAR(50),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_trade_logs_player_a ON trade_logs(player_a_id);
CREATE INDEX idx_trade_logs_player_b ON trade_logs(player_b_id);
CREATE INDEX idx_trade_logs_created ON trade_logs(created_at);

-- === ACTIVE VENDING SHOPS ===
CREATE TABLE vending_shops (
    id              SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    shop_title      VARCHAR(80) NOT NULL,
    zone_name       VARCHAR(50) NOT NULL,
    position_x      FLOAT NOT NULL,
    position_y      FLOAT NOT NULL,
    position_z      FLOAT NOT NULL,
    items           JSONB NOT NULL DEFAULT '[]',             -- [{cart_slot, item_id, amount, price_per_unit}]
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (character_id)                                    -- one shop per character
);
CREATE INDEX idx_vending_zone ON vending_shops(zone_name);

-- === BUYING STORES ===
CREATE TABLE buying_stores (
    id              SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    shop_title      VARCHAR(80) NOT NULL,
    zone_name       VARCHAR(50) NOT NULL,
    position_x      FLOAT NOT NULL,
    position_y      FLOAT NOT NULL,
    position_z      FLOAT NOT NULL,
    items           JSONB NOT NULL DEFAULT '[]',             -- [{item_id, buy_price, amount_wanted, amount_bought}]
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (character_id)
);

-- === GROUND ITEMS ===
CREATE TABLE ground_items (
    id              SERIAL PRIMARY KEY,
    item_id         INTEGER NOT NULL,
    amount          INTEGER NOT NULL DEFAULT 1,
    zone_name       VARCHAR(50) NOT NULL,
    position_x      FLOAT NOT NULL,
    position_y      FLOAT NOT NULL,
    position_z      FLOAT NOT NULL,
    refine_level    INTEGER NOT NULL DEFAULT 0,
    card_slot_1     INTEGER DEFAULT 0,
    card_slot_2     INTEGER DEFAULT 0,
    card_slot_3     INTEGER DEFAULT 0,
    card_slot_4     INTEGER DEFAULT 0,
    is_identified   BOOLEAN NOT NULL DEFAULT TRUE,
    dropped_by      INTEGER REFERENCES characters(id),       -- NULL for monster drops
    killer_id       INTEGER REFERENCES characters(id),       -- for ownership priority
    drop_time       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    despawn_time    TIMESTAMPTZ NOT NULL,                     -- drop_time + 60 seconds
    ownership_data  JSONB DEFAULT NULL                        -- {first: charId, second: charId, third: charId}
);
CREATE INDEX idx_ground_items_zone ON ground_items(zone_name);
CREATE INDEX idx_ground_items_despawn ON ground_items(despawn_time);

-- === MAIL SYSTEM ===
CREATE TABLE mail_messages (
    id              SERIAL PRIMARY KEY,
    sender_id       INTEGER NOT NULL REFERENCES characters(id),
    receiver_id     INTEGER NOT NULL REFERENCES characters(id),
    sender_name     VARCHAR(50) NOT NULL,
    receiver_name   VARCHAR(50) NOT NULL,
    title           VARCHAR(80) NOT NULL DEFAULT '',
    body            TEXT NOT NULL DEFAULT '',
    zeny_amount     BIGINT NOT NULL DEFAULT 0,
    items           JSONB NOT NULL DEFAULT '[]',             -- [{item_id, amount, refine, cards}]
    is_read         BOOLEAN NOT NULL DEFAULT FALSE,
    items_claimed   BOOLEAN NOT NULL DEFAULT FALSE,
    zeny_claimed    BOOLEAN NOT NULL DEFAULT FALSE,
    expires_at      TIMESTAMPTZ NOT NULL,                     -- created_at + 15 days
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_mail_receiver ON mail_messages(receiver_id);
CREATE INDEX idx_mail_sender ON mail_messages(sender_id);
CREATE INDEX idx_mail_expires ON mail_messages(expires_at);
```

### 12.2 Server-Side Architecture

#### Trade Engine
```
trade:request    { targetCharacterId }
trade:accept     { requesterId }
trade:deny       { requesterId }
trade:add_item   { inventorySlot, amount }
trade:remove_item { tradeSlot }
trade:set_zeny   { amount }
trade:lock       {}                          -- "OK" button
trade:confirm    {}                          -- "Trade" button
trade:cancel     {}

-- Server emits:
trade:requested  { fromCharacterId, fromName }
trade:opened     { partnerId, partnerName }
trade:partner_item { tradeSlot, itemData, amount }
trade:partner_zeny { amount }
trade:partner_locked {}
trade:completed  { receivedItems, receivedZeny }
trade:cancelled  { reason }
trade:error      { message }
```

#### Vending Events
```
vend:open_shop   { shopTitle, items: [{cartSlot, pricePerUnit}] }
vend:close_shop  {}
vend:browse      { vendorCharacterId }
vend:buy         { vendorCharacterId, vendItemIndex, amount }

-- Server emits:
vend:shop_opened  { characterId, shopTitle, position, zone }
vend:shop_closed  { characterId }
vend:shop_data    { vendorId, shopTitle, items: [{itemData, amount, pricePerUnit}] }
vend:buy_success  { itemData, amount, totalCost }
vend:buy_error    { message }
vend:item_sold    { buyerName, itemData, amount, revenue, timestamp }
vend:shop_report  { totalSold, totalRevenue, sales: [...] }
```

#### Storage Events
```
storage:open      {}                         -- request to open Kafra storage
storage:close     {}
storage:deposit   { inventorySlot, amount }
storage:withdraw  { storageSlot, amount }

-- Server emits:
storage:opened    { items: [...], maxSlots }
storage:closed    {}
storage:updated   { items: [...] }
storage:error     { message }
```

#### Ground Item Events
```
item:pickup       { groundItemId }
item:drop_ground  { inventorySlot, amount }

-- Server emits:
item:spawned      { groundItemId, itemId, amount, x, y, z, ownerId }
item:picked_up    { groundItemId, pickedUpByCharId }
item:despawned    { groundItemId }
item:pickup_error { message }
```

#### Mail Events
```
mail:send         { receiverName, title, body, zenyAmount, items: [{inventorySlot, amount}] }
mail:read         { mailId }
mail:claim_items  { mailId }
mail:claim_zeny   { mailId }
mail:delete       { mailId }
mail:list         {}

-- Server emits:
mail:inbox        { messages: [...] }
mail:sent         { mailId }
mail:received     { fromName, title }       -- notification
mail:claimed      { mailId, items, zeny }
mail:deleted      { mailId }
mail:error        { message }
```

### 12.3 Redis State Management

```
Active vending shops:     vend:shops:{zoneId}     -> Set of characterIds with active shops
Vending shop data:        vend:shop:{charId}      -> Hash with shop data (title, items, prices)
Ground items per zone:    ground:items:{zoneId}   -> Hash of groundItemId -> item data
Ground item ownership:    ground:owner:{itemId}   -> Hash with {killer, damageRanking, dropTime}
Active trades:            trade:session:{charId}  -> Hash with trade state
Trade locks:              trade:lock:{charId}     -> Boolean (prevents concurrent trades)
Buying stores:            buy:stores:{zoneId}     -> Set of characterIds with active buying stores
```

### 12.4 UE5 Client Widgets

| Widget | Purpose | Z-Order |
|--------|---------|---------|
| `STradeWidget` | Trade window with 10 slots per side, Zeny field, OK/Trade/Cancel | Z=22 |
| `SVendingSetupWidget` | Shop title input + item/price selection from cart | Z=22 |
| `SVendingBrowseWidget` | Browse another player's vending shop | Z=22 |
| `SBuyingStoreSetupWidget` | Set up buying store with wanted items/prices | Z=22 |
| `SBuyingStoreBrowseWidget` | View/sell to another player's buying store | Z=22 |
| `SStorageWidget` | Kafra storage grid (300-600 slots) | Z=18 |
| `SMailboxWidget` | Mail inbox/compose/read/claim | Z=22 |
| `SVendSearchWidget` | Search all active vending shops by item name | Z=22 |

### 12.5 UE5 Client Subsystems

| Subsystem | Role |
|-----------|------|
| `TradeSubsystem` | UWorldSubsystem: manages trade window state, socket events, item locking |
| `VendingSubsystem` | UWorldSubsystem: manages vending shop setup, browsing, purchasing |
| `StorageSubsystem` | UWorldSubsystem: manages Kafra storage open/close, deposit/withdraw |
| `GroundItemSubsystem` | UWorldSubsystem: manages ground item actors, pickup, despawn timers |
| `MailSubsystem` | UWorldSubsystem: manages mail inbox, compose, claim, notifications |

### 12.6 Anti-Cheat & Validation

#### Duplicate Item Prevention
- Items are **removed from source** (inventory/cart/storage) BEFORE being added to destination
- Trade items are "locked" in a pending state during the trade session
- If trade fails/cancels, locked items are restored to original inventory
- Server never trusts client-reported item data -- always validates against DB

#### Zeny Overflow Protection
```javascript
function canReceiveZeny(currentZeny, amount) {
    const MAX_ZENY = 2147483647;
    return (currentZeny + amount) <= MAX_ZENY && amount >= 0;
}
```

#### Trade Validation Checklist
1. Both players exist and are online
2. Both players are on the same map
3. Both players are within trade range
4. Neither player is vending, in storage, dead, or in another trade
5. All items belong to the trading player
6. All items are tradeable (not bound/quest)
7. Both players have sufficient Zeny
8. Neither player would exceed Zeny cap
9. Neither player would exceed weight limit
10. All item amounts are valid (>0, <= owned amount)
11. No more than 10 items per side

#### Vending Validation
1. Character is Merchant class with Vending skill
2. Character has Pushcart equipped
3. All vended items are in the Pushcart
4. Prices are within valid range (1z - 1,000,000,000z)
5. Number of item stacks does not exceed skill level cap
6. Character is at least 4 cells from any NPC
7. Buyer has sufficient Zeny
8. Buyer would not exceed weight limit
9. Apply 5% tax on items over 10,000,000z

### 12.7 Kafra Service Costs Reference

| Service | Cost |
|---------|------|
| Storage access | 40z |
| Save point | Free |
| Teleport (short distance) | 600z - 1,200z |
| Teleport (medium distance) | 1,200z - 2,000z |
| Teleport (long distance) | 2,000z - 3,000z |
| Cart rental (cheapest) | 600z (Aldebaran) |
| Cart rental (average) | 800z (most cities) |
| Cart rental (most expensive) | 1,200z (Archer Village) |

**Sample Teleport Costs from Prontera**:

| Destination | Cost |
|-------------|------|
| Izlude | 600z - 2,000z |
| Geffen | 2,000z |
| Payon | 2,000z |
| Morroc | 2,000z |
| Alberta | 2,000z |
| Aldebaran | 2,000z |
| Comodo | 3,000z |
| Umbala | 3,000z |

### 12.8 Mammonite Skill Cost Table

| Level | Zeny Cost | ATK% |
|-------|-----------|------|
| 1 | 100z | 150% |
| 2 | 200z | 200% |
| 3 | 300z | 250% |
| 4 | 400z | 300% |
| 5 | 500z | 350% |
| 6 | 600z | 400% |
| 7 | 700z | 450% |
| 8 | 800z | 500% |
| 9 | 900z | 550% |
| 10 | 1,000z | 600% |

**Dubious Salesmanship** (Blacksmith skill): Reduces Mammonite Zeny cost by 10%.

---

## Appendix A: NPC Shop Data Structure

For the server-side NPC shop system, each shop should be defined as a static data file:

```javascript
// server/src/ro_npc_shops.js
const NPC_SHOPS = {
    'prt_tool': {
        name: 'Prontera Tool Dealer',
        zone: 'prt_fild08',   // or zone ID
        type: 'tool',
        items: [
            { itemId: 1750, price: 1 },       // Arrow
            { itemId: 501,  price: 10 },      // Red Potion
            { itemId: 502,  price: 50 },      // Orange Potion
            { itemId: 503,  price: 180 },     // Yellow Potion
            { itemId: 504,  price: 1200 },    // White Potion
            { itemId: 505,  price: 40 },      // Green Potion
            { itemId: 645,  price: 800 },     // Concentration Potion
            { itemId: 656,  price: 1500 },    // Awakening Potion
            { itemId: 611,  price: 40 },      // Magnifier
            { itemId: 601,  price: 60 },      // Fly Wing
            { itemId: 602,  price: 300 },     // Butterfly Wing
            { itemId: 1065, price: 100 },     // Trap
        ]
    },
    'prt_weapon1': {
        name: 'Prontera Weapon Dealer',
        zone: 'prontera',
        type: 'weapon',
        items: [
            { itemId: 1201, price: 100 },     // Sword
            { itemId: 1202, price: 1500 },    // Falchion
            { itemId: 1203, price: 2900 },    // Blade
            // ... etc
        ]
    }
    // ... more shops
};
```

## Appendix B: Item Trade Flags

Each item in the database should have trade restriction flags:

```sql
ALTER TABLE items ADD COLUMN IF NOT EXISTS trade_flag INTEGER NOT NULL DEFAULT 0;
-- Bit flags:
-- 0x001 = Cannot be dropped
-- 0x002 = Cannot be traded (player-to-player)
-- 0x004 = Cannot be stored in Kafra storage
-- 0x008 = Cannot be stored in guild storage
-- 0x010 = Cannot be sold to NPC
-- 0x020 = Cannot be placed in cart
-- 0x040 = Cannot be mailed
-- 0x080 = Cannot be auctioned
-- 0x100 = Account-bound
-- 0x200 = Character-bound
```

## Appendix C: Zeny-Consuming Skills Reference

| Skill | Class | Zeny Cost | Effect |
|-------|-------|-----------|--------|
| Mammonite | Merchant | 100-1,000z | Physical attack (150-600% ATK) |
| Maximum Power-Thrust | Blacksmith | 3,000-5,000z | Party ATK buff (+50% for 150-180s) |
| Rapid Throw | Ninja | Up to 100,000z | Ranged damage based on Zeny spent |
| Cart Revolution | Merchant | 0z (uses cart weight) | AoE attack based on cart weight |

---

*Document generated for Sabri_MMO project. Based on Ragnarok Online Classic (pre-Renewal) mechanics as documented by iRO Wiki, RateMyServer, rAthena source code, and community resources.*
