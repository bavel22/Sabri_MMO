# Kafra Storage, Cart & Weight -- Deep Research (Pre-Renewal)

> **Scope**: RO Classic (pre-Renewal) Kafra services, personal storage, guild storage, Pushcart system, inventory system, and weight mechanics.
> **Sources**: iRO Wiki Classic, rAthena source (mmo.hpp, storage.cpp), RateMyServer, Ragnarok Fandom Wiki, divine-pride, WarpPortal Forums, rAthena community boards.
> **Target**: Sabri_MMO implementation reference.

---

## KAFRA STORAGE

### Storage Capacity (Base, Expansion)

| Property | Pre-Renewal Value |
|----------|-------------------|
| **Base capacity** | 300 item slots |
| **Historical note** | Originally 100 slots, expanded to 300 in a 2004 patch |
| **Renewal expansion** | 600 slots (post-Renewal only -- NOT applicable to pre-RE) |
| **Weight limit** | None -- storage has NO weight cap |
| **Stack limit** | Same as inventory -- consumables/loot stack (typically 99/999), equipment does NOT stack |

In rAthena source, storage capacity is controlled by `MAX_STORAGE` in `mmo.hpp`. The default is 600 for modern builds, but pre-renewal servers typically configure this to 300. The value must not exceed 0xFFFF (65535) for packet transmission.

**Key detail**: Storage is purely slot-limited. There is no weight restriction on storage. A player can store 300 Yggdrasil Berries (weight 30 each = 9,000 total weight) without issue, since weight only applies to inventory and cart.

### Storage Access (Any Kafra NPC)

Storage is accessed through **Kafra Employee NPCs** found in most towns. Not every Kafra NPC offers every service, but storage is the most universally available service.

**Kafra NPC locations** (towns with Kafra storage access):
- Prontera, Geffen, Payon, Alberta, Izlude, Aldebaran, Al De Baran, Morroc, Comodo, Juno (Yuno), Hugel, Rachel, Lighthalzen, Einbroch, Einbech, Louyang, Amatsu, Gonryun, Ayothaya, Jawaii, Niflheim, Umbala

**Access restrictions**:
- Must have **Basic Skill Lv. 6** (Novice skill prerequisite)
- Cannot access storage while in a trade window
- Cannot access storage while dead
- Cannot access storage while vending

### Storage Pricing/Fees

| Service | Cost |
|---------|------|
| **Open storage** | 40 Zeny per access (standard iRO Classic) |
| **Variant pricing** | Some servers/towns charge 40-80 Zeny |
| **iRO Valkyrie / kRO F2P** | 500 Zeny per access (premium server pricing) |
| **Deposit item** | Free (included in the access fee) |
| **Withdraw item** | Free (included in the access fee) |

**Free Ticket for Kafra Storage** (Item ID 7059): A consumable item that bypasses the 40z storage fee for one access. Given to new characters as a starter item. Can also be obtained from quests and events. The ticket is consumed when storage is opened.

**Kafra Points**: For every 10 Zeny spent on Kafra services (storage, teleport, cart rental), 1 Kafra Point (Special Reserve Point) is earned. These can be redeemed for items in Al De Baran through 5 different lottery NPCs.

### Item Types Allowed in Storage

| Item Type | Storable? | Notes |
|-----------|-----------|-------|
| Consumables | Yes | Stacks normally |
| Equipment (weapons/armor) | Yes | Each piece occupies 1 slot (never stacks) |
| Cards | Yes | Stack in groups |
| Loot/Miscellaneous | Yes | Stacks normally |
| Ammunition (arrows, etc.) | Yes | Stacks normally |
| Pet Eggs | Yes | Each egg = 1 slot |
| Forged/refined equipment | Yes | Preserves refine level, cards, forge attributes |

**Items that CANNOT be stored** (controlled by `item_trade` flag 0x004):
- Quest items (flag: cannot store)
- Bound items (character-bound, account-bound varies)
- Rental items (time-limited equipment)
- Items with the `nostorage` flag set in the item database

Equipment stored in Kafra storage retains ALL attributes: refine level, compounded cards, forged element, star crumbs, and forged-by metadata. They are stored as individual unique items, never stacked.

### Shared Across Characters (Per Account)

**Kafra storage is ACCOUNT-SHARED.** All characters on the same account access the same 300-slot storage pool. This is one of the primary methods of transferring items between characters on the same account.

| Property | Value |
|----------|-------|
| **Sharing scope** | All characters on the same account |
| **Cross-server** | No -- storage is per-server (each server has its own storage) |
| **Simultaneous access** | Only one character can have storage open at a time |

This means:
- Character A deposits a +7 Blade[4] into storage
- Character B (same account) can withdraw that +7 Blade[4]
- Character A cannot access storage while Character B has it open (only one character is logged in at a time per account)

---

## GUILD STORAGE

### Capacity

| Guild Skill Level | Slots |
|-------------------|-------|
| Guild Storage Extension Lv. 0 (no skill) | 0 (no access) |
| Guild Storage Extension Lv. 1 | 100 slots |
| Guild Storage Extension Lv. 2 | 200 slots |
| Guild Storage Extension Lv. 3 | 300 slots |
| Guild Storage Extension Lv. 4 | 400 slots |
| Guild Storage Extension Lv. 5 | 500 slots |

The **Guild Storage Extension** (GD_GUILD_STORAGE) is a guild skill that must be invested in by the Guild Master using guild skill points. Without at least Lv. 1, the guild has no storage access at all.

In rAthena, `MAX_GUILD_STORAGE` is capped at 600 by default (hard limit), though the skill only unlocks up to 500.

**Weight limit**: None -- same as personal storage, guild storage is purely slot-limited.

### Access Permissions (Rank-Based)

Guild storage access is controlled by the **Guild Master** through position/rank permissions:

| Permission | Description |
|------------|-------------|
| **Warehouse access** | Guild Master toggles this permission per guild title/position |
| **Default** | Only the Guild Master has access by default |
| **Delegation** | GM can enable storage access for specific ranks (e.g., "Officer", "Veteran") |

The Guild Master sets permissions through the guild management window. Each guild position (title) can independently have storage access enabled or disabled.

### Concurrent Access Rules

| Rule | Details |
|------|---------|
| **Simultaneous access** | Only **ONE** player can access guild storage at a time |
| **Lock mechanism** | When a player opens guild storage, it is locked for all other guild members |
| **Auto-close** | Storage closes when the player moves away from the Kafra NPC, logs out, or manually closes |
| **WoE restriction** | Guild storage is typically disabled during War of Emperium (server-configurable) |

**Transaction logging**: The 100 most recent guild storage transactions are logged and viewable, with records retained for up to 6 months. This helps the Guild Master track who deposited/withdrew items.

---

## CART SYSTEM

### Cart Rental (Where, Cost)

Carts are rented through **Kafra Employee NPCs** in towns. The rental cost varies by town:

| Town | Rental Cost (Zeny) |
|------|-------------------|
| Geffen | 750z |
| Prontera | 800z |
| Alberta | 850z |
| Other towns | 600z - 1,200z (varies) |

**Requirements to rent a cart**:
- Must be a Merchant-class character (Merchant, Blacksmith, Alchemist, or their transcendent versions)
- Must have the **Pushcart** skill (Skill ID 604) learned at Lv. 1 or higher
- Super Novices can also use Pushcart (rent from Aldebaran only)
- Must have sufficient Zeny to pay the rental fee

**Free Ticket for Kafra Transportation** can also be used to bypass cart rental fees (same ticket used for teleport fee bypass).

### Cart Capacity (Weight Limit, Item Slots)

| Property | Value |
|----------|-------|
| **Max item slots** | 100 |
| **Max weight capacity** | 8,000 |
| **Stacking** | Same as inventory -- consumables/loot stack, equipment does not |
| **Access hotkey** | Alt+W (opens cart inventory window) |

The cart is a separate inventory from the player's personal inventory. Items must be explicitly moved between inventory and cart using the move-to-cart and move-to-inventory commands.

**Important**: Cart weight and inventory weight are tracked separately. Items in the cart do NOT count toward the player's inventory weight limit. This is a key reason Merchants carry carts -- to free up personal weight capacity for farming.

### Cart Appearance (Visual)

The cart has a visual 3D model that follows the player character. The appearance can be changed using the **Change Cart** skill.

**Change Cart** (Skill ID 154):
- Quest skill (Platinum Skill) -- learned through a quest, not from the skill tree
- Requires Merchant Job Level 30+
- Allows choosing from multiple cart appearances based on Base Level

| Base Level Threshold | Cart Designs Available |
|---------------------|----------------------|
| Lv. 1+ | Cart 1 (default wooden cart) |
| Lv. 40+ | Cart 2 |
| Lv. 65+ | Cart 3 |
| Lv. 80+ | Cart 4 |
| Lv. 100+ | Cart 5 (added later) |

**Notes on Change Cart**:
- Purely cosmetic -- no gameplay effect
- Does NOT apply once you job change to Blacksmith or Alchemist (they get their own cart styles automatically based on class)
- If a Pushcart is removed and re-equipped, the appearance resets to default
- Pre-selecting a cart style before equipping a Pushcart applies the selected style when equipped

### Cart Revolution Damage Formula (Based on Cart Weight)

**Cart Revolution** (Skill ID 153) -- Merchant active skill, Lv. 1 only.

```
Damage (ATK%) = 150 + floor(100 * CurrentCartWeight / MaxCartWeight)
```

| Cart Weight | Damage % |
|-------------|----------|
| 0 (empty) | 150% |
| 2,000 (25%) | 175% |
| 4,000 (50%) | 200% |
| 6,000 (75%) | 225% |
| 8,000 (100% full) | 250% |

**Skill mechanics**:
- Melee physical attack
- Hits all enemies in a 3x3 AoE around the target
- Pushes affected enemies back 2 cells
- Ignores the accuracy check (always hits, like Bash)
- Damage is pseudo-elemental (uses weapon element but applies as Neutral for element table)
- Requires a Pushcart to be equipped
- SP Cost: 12

**Cart Termination** (Whitesmith skill, Skill ID 485 -- transcendent only):
- Requires Cart Boost to be active
- Consumes Zeny per use (500z at Lv. 1, scaling up)
- Damage scales with cart weight (heavier = more damage)
- NOT increased by % damage cards (Hydra, etc.) -- only flat ATK cards (Andre, Zipper Bear)
- ASPD capped at 186

**Cart Boost** (Whitesmith skill, Skill ID 484 -- transcendent only):
- Removes cart speed penalty entirely
- Required for Cart Termination

### Change Cart (Visual Change Skill)

See "Cart Appearance" section above. Change Cart is a Platinum Skill (quest skill) for Merchants at Job Level 30+. It is a cosmetic-only skill with no combat or stat effects.

### Cart is Merchant-Class Only

| Class | Can Use Cart? |
|-------|--------------|
| Merchant | Yes (with Pushcart skill) |
| Blacksmith | Yes (inherits Pushcart) |
| Alchemist | Yes (inherits Pushcart) |
| Whitesmith | Yes (transcendent Blacksmith) |
| Creator/Biochemist | Yes (transcendent Alchemist) |
| Super Novice | Yes (with Pushcart skill, rents from Aldebaran) |
| All other classes | No |

### Cart Persistence (Survives Map Changes?)

| Scenario | Cart Persists? |
|----------|---------------|
| Map/zone change (walking) | Yes -- cart and all contents persist |
| Kafra teleport | Yes -- cart travels with the character |
| Fly Wing / Butterfly Wing | Yes |
| Logging out | Yes -- cart state (has_cart, cart_type) and items saved to DB |
| Character death | Yes -- cart is NOT lost on death |
| Manually removing cart | Items PERSIST in DB -- re-renting cart restores items |
| Server restart | Yes -- loaded from character_cart table |

**Critical implementation detail**: When a player "removes" their cart via `cart:remove`, the cart items remain in the `character_cart` database table. When the player re-rents a cart, those items are automatically loaded back. The cart is not destroyed -- only the visual/active state is toggled.

### Pushcart Speed Penalty

The Pushcart skill reduces movement speed. Higher skill levels reduce the penalty:

| Pushcart Level | Speed Penalty |
|----------------|---------------|
| Lv. 1 | -45% move speed |
| Lv. 2 | -40% |
| Lv. 3 | -35% |
| Lv. 4 | -30% |
| Lv. 5 | -25% |
| Lv. 6 | -20% |
| Lv. 7 | -15% |
| Lv. 8 | -10% |
| Lv. 9 | -5% |
| Lv. 10 | 0% (no penalty -- normal speed) |

This speed penalty is one of the major tradeoffs of using a cart. Most Merchant-class characters aim for Pushcart Lv. 10 to eliminate the penalty.

---

## INVENTORY SYSTEM

### Max Item Slots

| Version | Slot Limit |
|---------|-----------|
| RO Classic (official) | **No hard slot limit** -- purely weight-limited |
| rAthena `INVENTORY_BASE_SIZE` | 100 slots (default) |
| rAthena expansion (packet 20181031+) | +100 additional = 200 total |
| rAthena hard cap | 480 slots maximum (technical limit) |
| Pre-renewal rAthena (older packets) | 100 slots (no expansion) |

In official RO Classic, inventory is purely weight-limited with no hard slot cap. You can carry 500 different items as long as the total weight stays under your weight limit. However, rAthena (the most common server emulator) implements a default of 100 slots for practical/packet reasons.

**Sabri_MMO uses**: 100 slots (matching rAthena default) + weight system for a hybrid approach.

### Item Stacking Rules

| Item Type | Stacks? | Typical Max Stack |
|-----------|---------|-------------------|
| Consumables (potions, food, gems) | Yes | 99 or as defined per item |
| Loot/Miscellaneous (Jellopy, etc.) | Yes | 999 (some items up to 30,000) |
| Ammunition (arrows, bullets) | Yes | 999 |
| Cards | Yes | 99 |
| Equipment (weapons, armor, headgear) | **No** -- each piece = 1 slot | 1 |
| Pet Eggs | **No** -- each egg = 1 slot | 1 |
| Forged/carded equipment | **No** -- always unique | 1 |

**Stacking rules**:
- Items stack only if they are identical (same item_id)
- Equipment NEVER stacks, even if identical -- each piece occupies its own slot because each can have different refine levels, cards, and forged attributes
- When picking up an item that already exists in inventory (same item_id, stackable), it adds to the existing stack
- When a stack hits max size, a new stack slot is created

### Weight System

See the dedicated Weight System section below.

---

## WEIGHT SYSTEM (Detailed)

### Weight Limit Formula

```
Max Weight = 2000 + (Base STR * 30) + Job Bonus + Skill Bonus + Mount Bonus
```

| Component | Value | Notes |
|-----------|-------|-------|
| **Base** | 2,000 | Universal for all classes |
| **STR bonus** | +30 per point of base STR | Only base STR counts (not buffed STR) |
| **Job class bonus** | Varies by class (see table) | Applied automatically |
| **Enlarge Weight Limit (600)** | +200 per skill level (max +2,000 at Lv. 10) | Merchant-class skill |
| **Gym Pass (Kafra Shop)** | +200 per level (max +2,000 at Lv. 10) | Separate from merchant skill, STACKS with it |
| **Peco Peco mount** | +1,000 | Knight/Crusader when mounted |

**Job Class Weight Bonuses**:

| Bonus | Classes |
|-------|---------|
| +0 | Novice, Super Novice |
| +200 | Mage, Wizard, Sage |
| +400 | Archer, Thief, Assassin, Acolyte |
| +600 | Hunter, Bard, Dancer, Rogue, Priest, Monk |
| +800 | Swordsman, Knight, Crusader, Merchant, Gunslinger, Taekwon |
| +1,000 | Blacksmith, Alchemist |

**Example calculations**:

1. **Novice (STR 1)**: 2000 + (1 * 30) + 0 = **2,030**
2. **Knight (STR 80, mounted)**: 2000 + (80 * 30) + 800 + 1000 = **6,200**
3. **Blacksmith (STR 90, EWL Lv10)**: 2000 + (90 * 30) + 1000 + 2000 = **7,700**
4. **Merchant (STR 50, EWL Lv10, Gym Pass Lv10)**: 2000 + (50 * 30) + 800 + 2000 + 2000 = **8,300**

### Gym Pass Bonus (+2,000)

The **Gym Pass** (Item ID 7776) is a Kafra Shop (cash shop) item. When used with the NPC **Ripped Cabus** in Payon, it teaches one level of a special version of Enlarge Weight Limit.

| Property | Value |
|----------|-------|
| **Max levels** | 10 |
| **Weight per level** | +200 |
| **Total bonus** | +2,000 at max |
| **Stacks with Merchant EWL** | Yes -- Merchant characters can get +4,000 total (+2,000 skill + +2,000 Gym Pass) |
| **Available to all classes** | Yes -- any class can use Gym Pass |

**Important**: The Gym Pass version is a SEPARATE skill from the Merchant's Enlarge Weight Limit. Both can be learned independently, and both bonuses apply simultaneously.

### 50% Threshold: No Natural HP/SP Regen

When a character's carried weight reaches **50% or more** of their maximum weight capacity, the following effects apply:

| Effect | Details |
|--------|---------|
| **Natural HP regeneration** | STOPS completely |
| **Natural SP regeneration** | STOPS completely |
| **HP Recovery skill** | Disabled |
| **SP Recovery skill** | Disabled |
| **Increase HP Recovery** | Disabled |
| **Increase SP Recovery** | Disabled |
| **Item-creation skills** | Disabled (Arrow Crafting, Find Stone, etc.) |
| **Combat skills** | Still work normally |
| **Movement** | Normal |
| **Potion/item use** | Still works |
| **Attacks** | Still work |

This threshold is a significant penalty that forces players to manage their weight carefully. Farming characters must periodically sell loot or store items to stay under 50%.

### 90% Threshold: Cannot Attack, Use Skills, Pick Up Items

When a character's carried weight reaches **90% or more** of their maximum weight capacity, ALL of the 50% penalties apply PLUS:

| Effect | Details |
|--------|---------|
| **Physical attacks** | BLOCKED -- cannot auto-attack |
| **ALL skills** | BLOCKED -- cannot use any skill |
| **Item pickup** | Effectively impossible (would push over 100%) |
| **Movement** | Still works -- character can walk |
| **Chat** | Still works |
| **Potion use** | Still works (but skills like Potion Pitcher cannot be used) |
| **NPC interaction** | Still works (can sell items to NPCs to reduce weight) |

At this threshold, the character is essentially helpless in combat. The only options are:
1. Sell items to an NPC
2. Drop items on the ground
3. Store items in Kafra storage
4. Move items to cart (if Merchant class)
5. Trade items to another player

### Weight Display in UI

The weight display is shown in the player's equipment/status window:

```
Current Weight / Max Weight
Example: 3,450 / 6,200
```

**Visual indicators**:
- Normal (< 50%): Standard display, no warning
- Minor Overweight (50-89%): Weight text turns yellow/orange, regen icon may be greyed out
- Major Overweight (90%+): Weight text turns red, additional warning message

The weight bar is also commonly displayed in the inventory panel as a horizontal bar showing the fill percentage.

---

## KAFRA ADDITIONAL SERVICES

### Save Point System

| Property | Value |
|----------|-------|
| **Cost** | Free |
| **Function** | Sets the character's respawn point (death return location) |
| **Persistence** | Saved to database, persists across sessions |
| **Usage limit** | Unlimited -- can be changed as often as desired |
| **Location** | Save point is set at/near the Kafra NPC's position |

When a character dies and respawns, they appear at their most recently saved Kafra save point. This is the primary method of choosing your "home town" in RO.

### Teleport Service

| Property | Value |
|----------|-------|
| **Cost** | 600z - 3,000z per teleport (distance-dependent) |
| **Destinations** | Varies per Kafra NPC -- usually nearby towns |
| **Free bypass** | Free Ticket for Kafra Transportation (Item ID 7060) |

Each Kafra NPC offers a fixed list of teleport destinations with set prices. Not all Kafra NPCs offer teleport to the same destinations.

---

## IMPLEMENTATION CHECKLIST

### Kafra Storage

| Feature | Status | Notes |
|---------|--------|-------|
| Storage table (character_storage) | Planned (in RagnaCloneDocs) | Account-level, keyed by user_id |
| 300-slot capacity | Not implemented | Need MAX_STORAGE = 300 |
| No weight limit on storage | Not implemented | Slot-only constraint |
| 40z access fee | Not implemented | Deduct on storage:open |
| Free Ticket bypass (7059) | Not implemented | Check inventory for ticket, consume on use |
| Account-shared access | Not implemented | Query by user_id, not character_id |
| Item deposit (inv -> storage) | Not implemented | Move item, recalculate weight |
| Item withdraw (storage -> inv) | Not implemented | Weight check before withdraw |
| Equipment preservation | Not implemented | Refine, cards, forge attrs must transfer |
| Nostorage flag check | Not implemented | Check item_trade flags |
| StorageSubsystem (client) | Not implemented | Reference code exists in 12_Economy_Vending.md |
| SStorageWidget (client) | Not implemented | Grid layout, Z=18 per plan |
| storage:open/close/deposit/withdraw events | Not implemented | Socket events defined in docs |
| Kafra Points earning | Not implemented | 1 point per 10z spent |

### Guild Storage

| Feature | Status | Notes |
|---------|--------|-------|
| Guild storage table (guild_storage) | Planned | Keyed by guild_id |
| Guild Storage Extension skill | Not implemented | Guild skill, 100 slots per level |
| Position-based access permissions | Not implemented | Requires guild system first |
| Single-user lock | Not implemented | Only one guild member at a time |
| Transaction logging | Not implemented | 100 recent entries |
| WoE lockout | Not implemented | Disable during siege |

### Cart System

| Feature | Status | Notes |
|---------|--------|-------|
| Cart rental (cart:rent) | IMPLEMENTED | 800z flat fee, Merchant-class check |
| Cart removal (cart:remove) | IMPLEMENTED | Items persist in DB |
| Cart items (character_cart table) | IMPLEMENTED | DB persistence |
| Cart capacity (100 slots, 8000 weight) | IMPLEMENTED | Enforced in cart:move_to_cart |
| Cart to/from inventory transfers | IMPLEMENTED | cart:move_to_cart, cart:move_to_inventory |
| Cart weight tracking | IMPLEMENTED | player.cartWeight recalculated |
| CartSubsystem (client) | IMPLEMENTED | F10 toggle, Z=14 |
| SCartWidget (client) | IMPLEMENTED | 10-col grid, weight bar, drag/drop |
| Cart visual broadcast | IMPLEMENTED | cart:equipped event to zone |
| Variable rental cost per town | NOT DONE | Currently flat 800z, should vary |
| Pushcart speed penalty by level | PARTIALLY | Speed formula references Pushcart level |
| Change Cart (cosmetic) | NOT DONE | cart_type stored but no visual selection UI |
| Cart Boost (Whitesmith) | NOT DONE | Transcendent skill |
| Cart Termination (Whitesmith) | NOT DONE | Transcendent skill |

### Weight System

| Feature | Status | Notes |
|---------|--------|-------|
| Base weight formula (2000 + STR*30) | IMPLEMENTED | getPlayerMaxWeight() |
| Enlarge Weight Limit (+200/lv) | IMPLEMENTED | Skill ID 600 |
| Mount bonus (+1000) | IMPLEMENTED | isMounted check |
| 50% overweight (regen stop) | IMPLEMENTED | OVERWEIGHT_50 threshold |
| 90% overweight (attack/skill block) | IMPLEMENTED | OVERWEIGHT_90 threshold |
| Weight status events | IMPLEMENTED | weight:status emitted on change |
| Inventory weight calculation | IMPLEMENTED | calculatePlayerCurrentWeight() |
| Gym Pass (+2000 all classes) | NOT DONE | Cash shop item, separate skill |
| Job class weight bonuses | NOT DONE | Currently missing from formula |
| Overweight UI indicators | PARTIAL | Weight bar exists, threshold colors TBD |

### Inventory System

| Feature | Status | Notes |
|---------|--------|-------|
| 100-slot limit | IMPLEMENTED | INVENTORY.MAX_SLOTS = 100 |
| Weight-based capacity | IMPLEMENTED | Full weight system in place |
| Item stacking | IMPLEMENTED | Stackable check in item handling |
| Equipment non-stacking | IMPLEMENTED | Each equipment = 1 slot |
| Drag-to-merge stacks | IMPLEMENTED | Inventory stack merging |

---

## GAP ANALYSIS

### Critical Gaps (Must Implement)

1. **Kafra Storage System**: The entire storage subsystem is not implemented. No `storage:open/close/deposit/withdraw` socket handlers exist on the server. No `StorageSubsystem` or `SStorageWidget` exists on the client. Reference implementation code exists in `RagnaCloneDocs/Implementation/12_Economy_Vending.md` (sections 4.4-4.5) but has not been built.

2. **Account-Level Storage Sharing**: Storage must be keyed by `user_id` (account), not `character_id`. The `character_storage` table schema exists in documentation but not in the actual database. All characters on an account share 300 slots.

3. **Job Class Weight Bonuses**: The `getPlayerMaxWeight()` function currently only adds base 2000 + STR*30 + EWL skill + mount bonus. It is MISSING the job class weight bonuses (+0 to +1000 depending on class). This means all characters have lower max weight than they should.

### Important Gaps (Should Implement)

4. **Variable Cart Rental Cost by Town**: Currently a flat 800z. Should vary by town (Geffen 750z, Prontera 800z, Alberta 850z, etc.). The zone data already has `kafraNpcs` per zone -- just needs a `cartRentalCost` field.

5. **Gym Pass Weight Bonus**: The Gym Pass (all-class +2000 weight) is a cash shop convenience item. Should be a separate tracked skill/flag on the character, stacking with Merchant's EWL.

6. **Free Ticket for Kafra Storage**: Item ID 7059. Should check inventory for this item on storage:open, consume it to bypass the 40z fee.

7. **Guild Storage**: Requires the guild system to be implemented first. 100-500 slots based on guild skill level, position-based access permissions, single-user lock, transaction logging.

### Minor Gaps (Nice to Have)

8. **Change Cart Visual Selection**: `cart_type` is stored in DB but there is no UI for selecting cart appearance. Needs a Change Cart skill (Platinum Skill quest at Job Lv 30+) and a selection popup.

9. **Pushcart Speed Penalty**: The speed formula references Pushcart level but the per-level penalty table (-45% to 0%) should be verified against the actual move speed calculation.

10. **Kafra Points System**: Earning 1 point per 10z spent on Kafra services is a minor convenience feature. Low priority.

11. **Storage Tab Sorting**: Pre-renewal has no tabs (items are in a single grid). Renewal adds category tabs. For pre-RE compliance, a single grid with scrolling is correct.

### Already Implemented (No Action Needed)

- Cart system core (rent, remove, move items, weight/slot tracking, DB persistence)
- Weight system core (formula, thresholds, overweight penalties, UI events)
- Inventory system (100 slots, stacking, weight tracking)
- Kafra NPC framework (kafra:open, kafra:save, kafra:teleport handlers)
- Cart persistence across map changes and logouts
- Weight status events (weight:status with threshold flags)

---

## SOURCES

- [Kafra - iRO Wiki Classic](https://irowiki.org/classic/Kafra)
- [Storage - iRO Wiki / Ragnarok Fandom](https://ragnarok.fandom.com/wiki/Storage)
- [Pushcart - iRO Wiki](https://irowiki.org/wiki/Pushcart)
- [Cart Revolution - iRO Wiki Classic](https://irowiki.org/classic/Cart_Revolution)
- [Weight Limit - iRO Wiki Classic](https://irowiki.org/classic/Weight_Limit)
- [Guild Storage Extension - iRO Wiki](https://irowiki.org/wiki/Guild_Storage_Extension)
- [Enlarge Weight Limit - iRO Wiki](https://irowiki.org/wiki/Enlarge_Weight_Limit)
- [Gym Pass - Ragnarok Fandom Wiki](https://ragnarok.fandom.com/wiki/Gym_Pass)
- [Change Cart - iRO Wiki](https://irowiki.org/wiki/Change_Cart)
- [rAthena mmo.hpp - GitHub](https://github.com/rathena/rathena/blob/master/src/common/mmo.hpp)
- [rAthena storage.cpp - GitHub](https://github.com/rathena/rathena/blob/master/src/map/storage.cpp)
- [rAthena MAX_GUILD_STORAGE Issue #3591](https://github.com/rathena/rathena/issues/3591)
- [rAthena Inventory/Cart/Storage Limit Discussion](https://rathena.org/board/topic/110837-inventory-cart-inventory-and-storage-limit/)
- [Free Ticket for Kafra Storage - RateMyServer](https://ratemyserver.net/index.php?page=item_db&item_id=7059)
- [Kafra Corporation - Ragnarok Fandom](https://ragnarok.fandom.com/wiki/Kafra_Corporation)
- [Cart Termination - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=485)
- [Guild Warehouse - Ragnarok Fandom](https://ragnarok.fandom.com/wiki/Guild_Warehouse)
- [Inventory - Ragnarok Fandom](https://ragnarok.fandom.com/wiki/Inventory)
