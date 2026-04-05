# Inventory, Weight & Item Properties -- Deep Research (Pre-Renewal)

> Ragnarok Online Classic (pre-Renewal) reference for the Sabri_MMO project.
> Sources: iRO Wiki Classic, rAthena pre-re source code, RateMyServer Pre-RE DB, Ragnarok Fandom Wiki, divine-pride.net, StrategyWiki, community forums.

---

## INVENTORY SYSTEM

### Max Inventory Slots (100 Item Types)

In pre-renewal Ragnarok Online, the inventory system is primarily **weight-limited**, not slot-limited. However, there are practical slot limits enforced by the client and server:

- **rAthena default**: `MAX_INVENTORY` = 100 slots (defined as `INVENTORY_BASE_SIZE` = 100 in older pre-renewal clients). This means a character can hold up to 100 **distinct item stacks/entries** at once.
- **Later patches** increased `INVENTORY_BASE_SIZE` to 150 for newer packet versions, but classic pre-renewal clients used 100.
- Each stackable item type occupies 1 slot regardless of quantity (e.g., 50 Red Potions = 1 slot). Each individual piece of equipment occupies 1 slot (e.g., two +7 Blades = 2 slots).
- The 100-slot limit and weight limit work together: you can be weight-capped long before filling 100 slots, or you can fill 100 slots with lightweight items before hitting the weight cap.

**Implementation Note**: The rAthena source defines this in `src/common/mmo.hpp`:
```cpp
#define INVENTORY_BASE_SIZE 100
#define MAX_INVENTORY (INVENTORY_BASE_SIZE + INVENTORY_EXPANSION_SIZE)
```
For pre-renewal, `INVENTORY_EXPANSION_SIZE` = 0, so `MAX_INVENTORY` = 100.

### Item Stacking (Consumables Stack, Equipment Does Not)

Stacking rules are fundamental to how inventory space is managed:

| Item Category | Stackable? | Reason |
|--------------|------------|--------|
| Consumables (potions, food, herbs) | **Yes** | Identical items share one slot |
| Miscellaneous / Etc (loot, materials, ores) | **Yes** | Monster drops, crafting materials |
| Ammunition (arrows, bullets, spheres) | **Yes** | Per ammo type |
| Cards | **No** | Each card is a unique inventory entry |
| Equipment (weapons, armor, headgear) | **No** | Each piece has individual refine level, card slots, and enchantments |
| Pet Eggs | **No** | Each egg tracks a unique pet with its own stats |

**Why equipment never stacks**: Even two "identical" items (e.g., two Blades [3]) can differ in refine level (+0 vs +7), compounded cards, forged attributes, or damaged state. The server must track each instance separately.

**Cards do not stack** because each card is treated as a unique item instance (has `UniqueId` flag in rAthena's item_db), even though two Poring Cards are functionally identical. This is a deliberate design choice to prevent card duplication exploits.

### Stack Limits

Maximum stack sizes vary by item type. These are defined per-item in rAthena's `item_db.yml` under the `Stack:` property:

| Item Type | Typical Max Stack | Examples |
|-----------|------------------|----------|
| Healing potions | 100 | Red Potion, White Potion |
| SP recovery items | 100 | Blue Potion, Grape |
| Cure items | 100 | Green Potion, Panacea |
| Gemstones | 100 | Blue Gemstone, Yellow Gemstone |
| Teleport items | 100 | Fly Wing, Butterfly Wing |
| Monster loot (light) | 30,000 | Jellopy, Fluff, Shell |
| Monster loot (heavy) | 30,000 | Iron Ore, Elunium Stone |
| Upgrade ores | 30,000 | Phracon, Emveretarcon, Oridecon |
| Arrows (all types) | 30,000 | Arrow, Fire Arrow, Silver Arrow |
| Bullets | 30,000 | Silver Bullet, Bloody Shell |
| Stat foods | 100 | All cooking items |
| Elemental converters | 100 | Flame/Frost/Seismic/Lightning |
| Box items | 100 | Old Blue Box, Old Card Album |
| Dead Branch / Bloody Branch | 100 | Summon items |

**Stack configuration in rAthena** (item_db.yml format):
```yaml
Stack:
  Amount: 100          # Max stack size
  Inventory: true      # Stack applies in inventory
  Cart: true           # Stack applies in cart
  Storage: true        # Stack applies in storage
  GuildStorage: true   # Stack applies in guild storage
```

The `Amount` field defines the maximum number of items that can be in a single stack. When a stack reaches its limit, additional items of the same type create a new stack in a new slot.

### Inventory Window UI Layout

The pre-renewal inventory window (opened with **Alt+E**) has **three tabs**:

| Tab | Name | Contents |
|-----|------|----------|
| 1 | **Item** | Consumables, usable items (potions, food, wings, gems, scrolls, boxes) |
| 2 | **Equip** | Equipment items (weapons, armor, headgear, accessories, shields, garments, footgear) -- both equipped and unequipped |
| 3 | **Etc** | Miscellaneous items (monster loot, crafting materials, ores, quest items, cards, ammunition) |

**UI elements visible in the inventory window:**
- Item icon (32x32 sprite)
- Item name (text, colored for equipment rarity in some clients)
- Quantity (for stackable items, shown as a number overlay or beside the name)
- Weight indicator at the bottom showing `Current Weight / Max Weight`
- Equipped items are shown with a special border or highlighted state in the Equip tab

**Pre-renewal did NOT have:**
- A Favorites tab (added in Renewal)
- A Sort button (added post-Renewal, December 2016 patch)
- Item rarity color coding in the original classic client (some later classic clients added this)

**Interaction methods:**
- **Double-click**: Use a consumable or equip/unequip equipment
- **Right-click**: Show item description tooltip
- **Ctrl+Right-click**: Identify an unidentified item (requires Magnifier in inventory)
- **Drag-and-drop**: Move to trade window, storage window, cart window, or drop on ground
- **Drag to hotbar**: Place consumables or equipment on F1-F9 shortcut bar

### Item Sorting

Pre-renewal RO had **no automatic sorting** feature in the inventory UI. Items appeared in the order they were acquired (by slot_index). Players organized their inventory manually by:

1. Dropping items and picking them up in desired order
2. Moving items to/from storage to rearrange
3. Using the hotbar for frequently used items

**Post-Renewal** added a Sort button that could sort by:
- Item Type
- Item Name (alphabetical)
- Item Weight
- Item Price

For implementation purposes, server-side sorting by `slot_index` (acquisition order) is the authentic pre-renewal behavior. Client-side sorting options are a modern quality-of-life addition.

---

## ITEM PROPERTIES

### Item ID System

Every item in Ragnarok Online has a unique integer ID. The rAthena `item_db` uses these ID ranges:

| ID Range | Category | Examples |
|----------|----------|----------|
| 501-999 | Usable items (potions, food, wings) | Red Potion (501), Fly Wing (601), Butterfly Wing (602) |
| 1000-1099 | Arrows and ammunition | Arrow (1750), Silver Arrow (1751), Fire Arrow (1752) |
| 1100-1299 | Various usable items | Blue Gemstone (717), Yellow Gemstone (716) |
| 1201-1249 | Daggers | Knife (1201), Cutter (1202), Main Gauche (1208) |
| 1250-1299 | One-Handed Swords | Sword (1101), Falchion (1104), Blade (1107) |
| 1300-1399 | Two-Handed Swords | Slayer (1151), Claymore (1163) |
| 1400-1449 | One-Handed Axes | Axe (1301) |
| 1450-1499 | Two-Handed Axes | Battle Axe (1351) |
| 1500-1549 | One-Handed Spears | Javelin (1401), Pike (1407) |
| 1550-1599 | Two-Handed Spears | Lance (1460) |
| 1600-1649 | Maces | Club (1501), Mace (1502), Chain (1519) |
| 1700-1749 | Rods / Staves | Rod (1601), Wand (1602), Arc Wand (1610) |
| 1750-1799 | Bows | Bow (1701), Composite Bow (1702), Arbalest (1713) |
| 1800-1899 | Knuckles / Fists | Waghnak (1801) |
| 1900-1949 | Instruments | Violin (1901) |
| 1950-1999 | Whips | Rope (1950) |
| 2100-2199 | Shields | Guard (2101), Buckler (2103) |
| 2200-2299 | Headgear | Hat (2216), Helm (2228) |
| 2300-2399 | Body Armor | Cotton Shirt (2301), Coat (2306), Full Plate (2316) |
| 2400-2499 | Garments | Hood (2504), Muffler (2503), Manteau (2505) |
| 2500-2599 | Footgear | Sandals (2401), Shoes (2403), Boots (2405) |
| 2600-2699 | Accessories | Ring (2601), Clip (2607), Rosary (2615) |
| 2700-2799 | Katars | Katar (1250) |
| 4000-4999 | Cards | Poring Card (4001), Fabre Card (4002), Andre Card (4050) |
| 7000-7999 | Miscellaneous / Etc items | Jellopy (909), Iron Ore (1002), Elunium (985) |

**Note**: The actual rAthena IDs differ from Sabri_MMO's custom ID ranges. The project uses its own mapping (see `docsNew/06_Reference/ID_Reference.md`).

### Item Types (type field in item_db)

rAthena defines item types with numeric codes:

| Type Code | Name | Description |
|-----------|------|-------------|
| 0 | Healing | HP/SP recovery items (consumed immediately on use) |
| 2 | Usable | Items with delayed effect or targeting (Fly Wing, converters) |
| 3 | Etc | Non-usable, non-equippable miscellaneous items |
| 4 | Armor | All armor-type equipment (headgear, body, shield, garment, footgear, accessory) |
| 5 | Weapon | All weapon types |
| 6 | Card | Cards for compounding into equipment |
| 7 | Pet Egg | Captured monster eggs |
| 8 | Pet Armor | Equipment for pets |
| 10 | Ammo | Ammunition (arrows, bullets, spheres, shuriken, kunai) |
| 11 | Delayed Consumable | Usable items with use delay |

**Subtypes for weapons** (`subtype` field):
- `dagger`, `1hSword`, `2hSword`, `1hSpear`, `2hSpear`, `1hAxe`, `2hAxe`, `mace`, `staff`, `bow`, `katar`, `knuckle`, `book`, `instrument`, `whip`, `gun_handgun`, `gun_rifle`, `gun_shotgun`, `gun_gatling`, `gun_grenade`, `huuma`

**Subtypes for armor** (`subtype` field in Renewal, location bitmask in pre-re):
- Equipment positions use a location bitmask: `head_top` (256), `head_mid` (512), `head_low` (1), `armor` (16), `weapon` (2+32), `shield` (32), `garment` (4), `footgear` (64), `accessory` (8+128)

### Weight Per Item

Every item has a `Weight` field in the item database. Weight is stored as an integer with units of 0.1 (so Weight=10 means 1.0 actual weight). Common weight values:

| Item | DB Weight | Actual Weight | Notes |
|------|-----------|---------------|-------|
| Jellopy | 10 | 1.0 | Lightest loot |
| Red Potion | 70 | 7.0 | Light potion |
| White Potion | 150 | 15.0 | Standard endgame potion |
| Condensed White Potion | 30 | 3.0 | Why "slims" are preferred |
| Mastela Fruit | 30 | 3.0 | Best HP/weight ratio |
| Yggdrasil Berry | 300 | 30.0 | Very heavy |
| Blue Potion | 150 | 15.0 | Heavy SP recovery |
| Butterfly Wing | 50 | 5.0 | Light teleport |
| Fly Wing | 50 | 5.0 | Light teleport |
| Arrow (any type) | 1 | 0.1 | Nearly weightless |
| Card (any) | 10 | 1.0 | All cards weigh 1 |
| Iron Ore | 150 | 15.0 | Heavy material |
| Elunium | 100 | 10.0 | Upgrade material |
| Oridecon | 200 | 20.0 | Upgrade material |
| Knife (weapon) | 400 | 40.0 | Light weapon |
| Claymore (2H Sword) | 2500 | 250.0 | Very heavy weapon |
| Full Plate (armor) | 4500 | 450.0 | Heaviest armor |
| Cotton Shirt (armor) | 100 | 10.0 | Lightest armor |
| Magnifier | 50 | 5.0 | Identification tool |

**Important**: rAthena stores weight as integer * 10 (so 70 = 7.0 weight). Sabri_MMO may use direct integer weight values. Ensure consistent weight unit handling.

### NPC Buy/Sell Price

Every item has a buy price and a sell price:

- **Buy price**: What the item costs at an NPC shop
- **Sell price**: What the NPC pays when you sell the item
- **Standard ratio**: Sell price = Buy price / 2 (50% of buy price)
- **Overcharge skill** (Merchant): Increases sell price by up to +24% at Lv10
- **Discount skill** (Merchant): Decreases buy price by up to -24% at Lv10

| Item | NPC Buy | NPC Sell | Notes |
|------|---------|---------|-------|
| Red Potion | 50z | 25z | Starter healing |
| White Potion | 1,200z | 600z | Standard endgame |
| Fly Wing | 60z | 30z | Random teleport |
| Butterfly Wing | 300z | 150z | Save point teleport |
| Magnifier | 40z | 20z | Item identification |
| Blue Gemstone | 600z | 300z | Skill catalyst |
| Yellow Gemstone | 600z | 300z | Skill catalyst |
| Arrow | 1z | 0z | Basic ammo |
| Silver Arrow | 3z | 1z | Holy element ammo |

Items that cannot be bought from NPCs (drop-only) typically have a buy price of 0 and a sell price based on their rarity.

### Class Restrictions (Equip)

Every equipment item has an `equip_jobs` bitmask that defines which job classes can equip it. In rAthena, this is a 64-bit integer where each bit represents a class:

| Bit | Class | Bit | Class |
|-----|-------|-----|-------|
| 0 | Novice | 8 | Knight |
| 1 | Swordsman | 9 | Priest |
| 2 | Mage | 10 | Wizard |
| 3 | Archer | 11 | Blacksmith |
| 4 | Acolyte | 12 | Hunter |
| 5 | Merchant | 13 | Assassin |
| 6 | Thief | 14 | Crusader |
| 7 | -- | 15 | Monk |
| 16 | Sage | 17 | Rogue |
| 18 | Alchemist | 19 | Bard |
| 20 | Dancer | -- | -- |

- **-1 (all bits set)**: Equippable by all classes
- **0**: Not equippable (misc items, cards, etc.)

Common restriction patterns:
- **All classes**: Most basic armor (Cotton Shirt, Sandals)
- **Swordsman line only**: Heavy armor (Full Plate), Two-Handed Swords
- **Mage line only**: Robes, Rods/Staves
- **Assassin only**: Katars
- **Bard only**: Instruments
- **Dancer only**: Whips
- **Merchant line only**: Carts (skill-based, not equipment)

### Level Restrictions (Equip)

Equipment has a `required_level` (base level) that the character must meet to equip it. Additionally, weapon level implies minimum base level requirements in some implementations:

| Weapon Level | Implied Min BaseLv |
|-------------|-------------------|
| 1 | 1 |
| 2 | 12-18 (varies by weapon type) |
| 3 | 24-33 (varies by weapon type) |
| 4 | 36-48 (varies by weapon type) |

In practice, most items simply use the explicit `required_level` field. Some high-end equipment requires base level 50, 70, or even 99.

### Slot Count (for Equipment)

Equipment can have 0 to 4 card slots, shown in brackets after the item name (e.g., "Blade [3]"):

| Equipment Type | Possible Slot Range | Typical |
|---------------|-------------------|---------|
| Weapons | 0-4 | 0-3 most common |
| Body Armor | 0-1 | 0 or 1 |
| Shield | 0-1 | 0 or 1 |
| Garment | 0-1 | 0 or 1 |
| Footgear | 0-1 | 0 or 1 |
| Headgear (Upper) | 0-1 | 0 or 1 |
| Headgear (Mid) | 0 | Never has slots |
| Headgear (Lower) | 0 | Never has slots |
| Accessories | 0-1 | 0 or 1 |

**Trade-off**: More slots generally means lower base stats. A Blade [3] has 53 ATK with 3 card slots, while a Katana [0] has 60 ATK with no slots. Three Hydra Cards (+60% vs Demi-Human) in the Blade far outweigh the 7 ATK difference.

**Socket Enchant NPC**: Some equipment that originally has 0 slots can gain 1 slot through a special NPC service (costs zeny + materials). Not all items are eligible.

### Item Scripts/Effects

In rAthena, items can have three script fields:

| Script Field | When Executed | Example |
|-------------|--------------|---------|
| `Script` | When item is equipped (equipment) or used (consumable) | `{ bonus bStr,3; }` (STR+3 on equip) |
| `EquipScript` | When equipment is equipped (runs once) | `{ sc_start SC_ENDURE,60000,1; }` |
| `UnEquipScript` | When equipment is unequipped (cleanup) | `{ sc_end SC_ENDURE; }` |

**Common script commands:**
```
bonus bStr,N;           -- Add N to STR
bonus bAgi,N;           -- Add N to AGI
bonus bMaxHP,N;         -- Add N to Max HP
bonus bMaxHPrate,N;     -- Add N% to Max HP
bonus2 bAddRace,RC_DemiHuman,20;  -- +20% damage vs Demi-Human
bonus2 bSubRace,RC_DemiHuman,30;  -- -30% damage FROM Demi-Human
bonus2 bAddEle,Ele_Fire,20;       -- +20% damage vs Fire element
bonus bNoSizeFix;                  -- Ignore size penalty (Drake Card)
bonus bSplashRange,1;             -- 3x3 splash attack (Baphomet Card)
bonus bNoGemStone;                -- No gemstone cost (Mistress Card)
autobonus "{ ... }",rate,duration; -- Chance to trigger effect on attack
```

**In Sabri_MMO**: Item effects are handled through `ro_item_effects.js` (consumable use effects), `ro_card_effects.js` (card bonuses), and inline equipment bonus logic in `index.js`.

---

## WEIGHT SYSTEM

### Weight Limit Formula: 2000 + (STR * 30)

The base weight formula in pre-renewal RO:

```
Max Weight = 2000 + (Base_STR * 30) + Job_Bonus + Skill_Bonus + Mount_Bonus
```

| Component | Value | Source |
|-----------|-------|--------|
| Base | 2,000 | Universal for all classes |
| STR bonus | +30 per point of **base** STR | Only base stat counts, not buffed STR |
| Job class bonus | Varies by class (see below) | Built-in to class definition |
| Enlarge Weight Limit | +200 per skill level (max Lv10 = +2,000) | Merchant passive skill (ID 600) |
| Peco Peco riding | +1,000 | When mounted on Peco (Knight/Crusader) |

**Example calculations:**
- Novice with 1 STR: 2,000 + (1 * 30) + 0 = 2,030
- Knight with 80 STR: 2,000 + (80 * 30) + 800 = 5,200
- Blacksmith with 90 STR, EWL Lv10: 2,000 + (90 * 30) + 1,000 + 2,000 = 7,700
- Knight with 80 STR on Peco: 2,000 + (80 * 30) + 800 + 1,000 = 6,200

### Gym Pass: +2,000 Weight Capacity

The **Gym Pass** (item ID 7776) is a membership card for a training facility:

- **How it works**: Take the Gym Pass to NPC "Ripped Cabus" in Payon (173, 141)
- **Effect**: Teaches the Enlarge Weight Limit skill, gaining +1 level per Gym Pass used
- **Per level**: +200 weight capacity
- **Maximum**: Lv10 = +2,000 total weight capacity
- **Who can use**: All classes (not just Merchant)
- **Persistence**: Permanent passive skill, survives death and relog
- **Source**: Gym Pass items are obtained from quests, NPCs, or the cash shop (Gym Pass Box, item ID 13710)

**Merchant class skill**: Merchants learn Enlarge Weight Limit as a normal class skill (up to Lv10 = +2,000). This is the same skill the Gym Pass teaches, so the bonuses do not stack -- they share the same skill ID. A Merchant with Lv10 EWL cannot benefit from additional Gym Passes.

**Non-Merchant classes**: Must rely entirely on Gym Passes to get EWL levels, since it is not in their skill tree.

### Job Class Weight Bonuses

Different job classes have inherent weight capacity bonuses:

| Job Bonus | Classes |
|-----------|---------|
| +0 | Novice, Super Novice |
| +200 | Mage, Wizard, Sage |
| +400 | Archer, Thief, Assassin, Acolyte |
| +600 | Hunter, Bard, Dancer, Rogue, Priest, Monk |
| +800 | Swordsman, Knight, Crusader, Merchant, Gunslinger |
| +1,000 | Blacksmith, Alchemist |

**Note**: These values are approximate and derived from community observation. rAthena does not have a single "job weight bonus" field -- instead, the weight limit is computed from base + STR + skills. The job bonus values listed here may be embedded in the base weight constant or class-specific configuration files in different server implementations.

**Sabri_MMO implementation note**: The current `getPlayerMaxWeight()` function uses `2000 + str * 30 + EWL bonus`. Job-class bonuses are not yet implemented. If adding them, use a lookup table keyed by `player.stats.class`.

### 50% Overweight: Natural HP/SP Regen Stops

When a character's current weight reaches **50% or more** of their maximum weight capacity:

**Effects:**
- Natural HP regeneration stops completely (the passive tick-based HP recovery while standing still)
- Natural SP regeneration stops completely
- The **Increase HP Recovery** skill effect is suppressed
- The **Increase SP Recovery** skill effect is suppressed
- Item-creation skills are disabled: **Arrow Crafting**, **Find Stone**, **Pharmacy**, and similar
- Combat, movement, skill use (non-creation), and item use are all still allowed

**What still works at 50% overweight:**
- Attacking (melee and ranged)
- Using skills (offensive, defensive, supportive)
- Using consumable items (potions still heal)
- Moving and walking
- Picking up items (until 90%)
- Sitting (but regen from sitting is also blocked)

**Technical**: The check is `currentWeight >= maxWeight * 0.50`. It uses >= (greater-than-or-equal), so hitting exactly 50% triggers the penalty.

### 90% Overweight: Cannot Attack, Use Skills, Pick Up Items

When a character's current weight reaches **90% or more** of their maximum weight capacity:

**Effects (all of 50% penalties PLUS):**
- **Cannot attack** (melee or ranged auto-attack is blocked)
- **Cannot use any skills** (all skill types disabled)
- **Cannot pick up items from the ground** (pickup is refused)
- Movement speed is reduced (some implementations use ~60% speed)
- The character is essentially limited to walking and chatting

**What still works at 90% overweight:**
- Walking / moving (at reduced speed)
- Chatting
- Opening/closing windows
- Dropping items (to reduce weight)
- Using NPC services (shops, storage)
- Trading with other players

**How to recover from 90% overweight:**
1. Drop items on the ground
2. Sell items to a nearby NPC
3. Store items in Kafra Storage
4. Trade items to another player
5. Use items that are consumed (potions, which reduce weight when used)

**Technical**: The check is `currentWeight >= maxWeight * 0.90`.

### Weight Display in Status Window

The status window (**Alt+A**) shows weight information:

```
Weight: [current] / [maximum]
```

- Current weight updates in real-time as items are acquired, consumed, or dropped
- The display changes color based on overweight status:
  - **White/Normal**: Under 50% (no penalty)
  - **Yellow**: 50-89% (minor overweight -- regen disabled)
  - **Red**: 90%+ (major overweight -- combat disabled)
- Some clients show a weight bar/gauge in the status window

### Cart Weight (Separate from Character Weight)

Merchant-class characters (Merchant, Blacksmith, Alchemist, and their transcendent versions) can use a **Pushcart**:

| Property | Value |
|----------|-------|
| Max slots | 100 item entries |
| Max weight | 8,000 (base), increased by Cart slots improvements |
| Speed penalty | -40% movement speed (base) |
| Speed recovery | Change Cart skill reduces penalty |
| Access | Requires Pushcart skill (Merchant Lv5) |
| Persistence | Cart contents persist across logouts |

**Cart weight is completely separate from character weight.** Items in the cart do not count toward the character's weight limit and vice versa. This means a Merchant at 90% character weight can still use cart items.

**Cart weight formula**: `Cart Max Weight = 8,000` (flat, no STR scaling). Some implementations allow Cart skill improvements to increase this.

**Cart-specific operations:**
- Move items between inventory and cart
- Use cart items in skills (Cart Revolution uses cart weight for damage)
- Vend items directly from cart (Vending skill)
- Cart weight affects Cart Revolution damage: `Damage = 150% ATK * (100% + CartWeight/MaxCartWeight * 100%)`

---

## ITEM INTERACTION

### Pick Up Items (From Ground)

Ground items (dropped by monsters or players) can be picked up by clicking on them:

**Pickup mechanics:**
- Click on a ground item to walk to it and pick it up
- The item is added to the first available inventory slot
- A pickup animation plays and the character briefly stops
- If inventory is full (100 slots) or weight would exceed maximum, pickup is refused with a message: "Item is too heavy" or "Cannot carry more items"

**Loot protection system** (from rAthena `drops.conf`):

| Timer | Default (ms) | Who Can Loot |
|-------|-------------|--------------|
| `item_first_get_time` | 3,000 (3s) | Only the player who dealt the most damage |
| `item_second_get_time` | 2,000 (2s) | First and second highest damage dealers |
| `item_third_get_time` | 2,000 (2s) | First, second, and third highest damage dealers |
| After all timers expire | -- | Anyone can pick up |

**MVP-specific loot protection:**

| Timer | Default (ms) | Who Can Loot |
|-------|-------------|--------------|
| `mvp_item_first_get_time` | 10,000 (10s) | Only the MVP (most damage) |
| `mvp_item_second_get_time` | 10,000 (10s) | MVP + second highest |
| `mvp_item_third_get_time` | 2,000 (2s) | MVP + second + third highest |

**Party loot distribution** (when in a party):
- **Each Take**: Each player picks up their own loot individually (default)
- **Party Share**: Loot is automatically distributed among party members. The player who dealt the most damage has a higher chance of receiving each item.

### Drop Items (To Ground, Timed Despawn)

Players can drop items from their inventory to the ground:

**Drop mechanics:**
- Drag an item from the inventory window and drop it outside the window, or use the drop command
- A confirmation dialog appears: "Are you sure you want to drop [item name]?"
- For stackable items, a quantity selector appears to choose how many to drop
- The item appears on the ground at the character's feet

**Ground item lifetime** (from rAthena `drops.conf`):
- `flooritem_lifetime`: **60,000 ms (60 seconds)** default
- After this time, the item despawns permanently and is lost
- Monster-dropped items and player-dropped items use the same despawn timer
- The timer starts when the item hits the ground

**Items that cannot be dropped:**
- Quest items with the `NoDrop` trade flag
- Account-bound items (teleport back to inventory if dropped)
- Character-bound items
- Some cash shop items

### Use Items (Consumables)

Using a consumable item:

**How to use:**
1. Double-click the item in the Item tab of the inventory
2. Or press the assigned hotkey (F1-F9) if placed on the shortcut bar
3. Some items require a target (e.g., Fly Wing: no target needed; some scrolls: select target)

**Use mechanics:**
- The item is consumed (quantity reduced by 1)
- The effect applies immediately (HP heal, SP heal, buff, teleport, etc.)
- Weight is reduced by the item's weight
- A use animation may play (potion drinking animation)
- There is a global item use delay (~0.5s) to prevent spamming
- Some items have individual cooldowns

**Item use restrictions:**
- Cannot use items during certain status effects (Stun, Freeze, Stone Curse, Deep Sleep)
- Cannot use items at 90% overweight (major overweight)
- Some items have class restrictions on use
- Some items have level requirements
- Yggdrasil Berry has a 5-second cooldown in some implementations

### Equip/Unequip

Equipment is managed through the Equipment window (**Alt+Q**) or the Equip tab of the Inventory window:

**How to equip:**
1. Double-click an equipment item in the Equip tab of inventory
2. Or drag the item to the appropriate slot in the Equipment window
3. Or press the assigned hotkey (if placed on F1-F9 shortcut bar)

**Equip validation pipeline:**
1. **Class check**: Does the character's class match the item's `equip_jobs` bitmask?
2. **Level check**: Is the character's base level >= the item's `required_level`?
3. **Slot conflict**: Is the target slot already occupied? (auto-unequip the old item)
4. **Two-handed check**: If equipping a 2H weapon, auto-unequip shield. If equipping a shield while 2H weapon is equipped, reject.
5. **Headgear combo**: Multi-slot headgear (Upper+Mid) auto-unequips items in both slots
6. **Identification**: Unidentified items cannot be equipped

**On equip:**
- Item stat bonuses are added to the character (STR, AGI, DEF, etc.)
- Equipment scripts execute (`EquipScript` field)
- ASPD is recalculated based on weapon type
- Weapon ATK, range, and element are updated
- Visual appearance changes (visible to other players)
- `combat:health_update` is broadcast to the zone

**On unequip:**
- Item stat bonuses are removed
- Unequip scripts execute (`UnEquipScript` field)
- All derived stats are recalculated
- If weapon is unequipped, character reverts to bare-fist stats
- Equipment swap: equipping a new item in an occupied slot auto-unequips the old item first

**Hotkey equipment swap:** Players can place equipment on their F1-F9 hotbar. Pressing the hotkey equips the item (or unequips if already equipped). This enables fast weapon swapping in combat -- a common tactic for switching between a damage weapon and a carded utility weapon.

### Item Identification (Magnifier Item, Identify Skill)

Some equipment drops from monsters in an **unidentified** state:

**Unidentified items:**
- Display a generic name based on type (e.g., "Sword", "Armor", "Shoes") instead of their actual name
- Cannot be equipped
- Cannot be refined
- Cannot have cards compounded
- The actual stats, card slots, and special effects are hidden
- Can still be dropped, traded, stored, and sold to NPCs (at a reduced price)

**How to identify:**
1. **Magnifier** (item ID 611): A consumable item purchased from Tool Dealers for 40z, weight 5
   - Use: Double-click the Magnifier, then click the unidentified item
   - Alternative: Hold Ctrl + Right-click the unidentified item (if Magnifier is in inventory)
   - Consumes one Magnifier per identification
2. **Identify skill** (Merchant skill, ID 40 / Item Appraisal):
   - Identifies items without consuming a Magnifier
   - Lv1 passive skill for Merchants
   - Free and unlimited use
3. **Novice Magnifier**: Given to new characters, single-use

**Which items drop unidentified:**
- Most equipment dropped by monsters (weapons, armor, headgear)
- Cards always drop identified
- Consumables always drop identified
- Etc/misc items always drop identified
- NPC-purchased equipment is always identified
- Quest reward equipment is always identified

**In rAthena's item_db**: The `DropIdentified` flag (default: true for non-equipment, false for equipment) controls whether monster drops are pre-identified.

### Unidentified Items (Hidden Stats, Generic Name)

When an item is unidentified, the client displays substitute information:

| Item Type | Generic Name Shown |
|-----------|-------------------|
| Dagger | "Dagger" |
| One-Handed Sword | "Sword" |
| Two-Handed Sword | "Two-handed Sword" |
| Spear | "Spear" |
| Axe | "Axe" |
| Mace | "Mace" |
| Rod / Staff | "Rod" |
| Bow | "Bow" |
| Katar | "Katar" |
| Knuckle | "Knuckle" |
| Book | "Book" |
| Instrument | "Instrument" |
| Whip | "Whip" |
| Body Armor | "Armor" |
| Shield | "Shield" |
| Garment | "Garment" |
| Footgear | "Shoes" |
| Headgear (Upper) | "Headgear" |
| Headgear (Mid) | "Headgear" |
| Headgear (Lower) | "Headgear" |
| Accessory | "Accessory" |

**Tooltip for unidentified items:**
- Shows only the generic name and "Unidentified" label
- Weight is visible (weight is not hidden)
- All stat bonuses, card slots, element, and special effects are hidden
- Icon may show a generic item icon or the actual icon with a "?" overlay (varies by client)

**Visual indicator:** Unidentified items typically have an orange "?" overlay on their icon or a distinct border color to distinguish them from identified items.

---

## SPECIAL ITEM TYPES

### Bound Items (Account/Character Bound)

Bound items have trade restrictions that prevent them from being freely transferred:

**Account Bound items:**
- Cannot be traded to other players
- Cannot be dropped on the ground (attempting to drop teleports the item back to inventory)
- Cannot be vended (sold via player shops)
- Cannot be mailed to other players
- **CAN** be stored in Kafra Storage (shared between characters on the same account)
- **CAN** be sold to NPCs (destroyed for zeny)
- Source: Cash shop items, event rewards, some quest rewards

**Character Bound items:**
- All restrictions of Account Bound, PLUS:
- **Cannot** be stored in Kafra Storage
- Permanently tied to a single character
- Source: Some quest rewards, job change items

**In rAthena's item_db**, trade restrictions are defined with the `Trade` flag set:

```yaml
Trade:
  NoDrop: true         # Cannot be dropped
  NoTrade: true        # Cannot be traded to other players
  NoStorage: true      # Cannot be stored in Kafra Storage
  NoCart: true          # Cannot be put in cart
  NoGuildStorage: true  # Cannot be stored in guild storage
  NoMail: true          # Cannot be mailed
  NoAuction: true       # Cannot be auctioned
  Override: 100         # GM level required to bypass restrictions
```

**Pre-Renewal note**: Account/character binding was rare in original pre-renewal. Most items were freely tradeable. Binding was introduced primarily for cash shop items and became more widespread in Renewal and later updates. In classic pre-renewal, the main "untradeable" items were quest items.

### Rental Items (Time-Limited)

Rental items are equipment or consumables with a time-based expiration:

**Mechanics:**
- The item has a countdown timer from the moment it is acquired (not from first use)
- Timer counts down in real time, including while logged out
- When the timer reaches zero, the item is automatically deleted from inventory/storage
- Rental items typically cannot be traded, dropped, or stored
- The item tooltip shows remaining time

**Common rental items (pre-renewal era):**
- Rental weapons from NPCs (trial weapons for new players)
- Event-specific equipment
- Cash shop trial items

**In rAthena**: Rental items use the `RentalDuration` field:
```yaml
RentalDuration: 604800  # Duration in seconds (604800 = 7 days)
```

**Pre-Renewal note**: Rental items were uncommon in original pre-renewal. They became more prevalent with the cash shop system and event systems in later episodes. For a classic pre-renewal implementation, rental items are a low-priority feature.

### Quest Items (Cannot Trade/Drop)

Quest items are special miscellaneous items tied to quest progression:

**Properties:**
- Item type is typically `Etc` (type 3)
- Have the `NoDrop` and `NoTrade` flags set
- Often have `NoStorage` as well (cannot be banked)
- Weight varies (usually light, 1-10)
- Have no sell value or very low sell value
- Purpose: Collected during quests, turned in to NPCs
- Automatically removed when quest is completed (via NPC script)

**Examples:**
- Jellopy x10 (Novice training quest -- uses regular Jellopies, not a special quest item)
- Various monster parts for job change quests
- Proof of collecting items for unlock quests
- NPC letter/document items for delivery quests

**Implementation in rAthena:**
```yaml
Flags:
  BuyingStore: false    # Cannot be listed in buying stores
  DeadBranch: false     # Cannot be obtained from Dead Branch
  Container: false      # Cannot be obtained from boxes
  UniqueId: true        # Each instance is unique
Trade:
  NoDrop: true
  NoTrade: true
  NoStorage: true
  NoCart: true
  NoGuildStorage: true
  NoMail: true
  NoAuction: true
```

---

## Implementation Checklist

### Core Inventory System
- [x] Max inventory slots limit (100 unique entries) -- current Sabri_MMO has no hard slot limit; consider adding
- [x] Item stacking for consumables/etc -- implemented in `addItemToInventory()`
- [x] Equipment non-stacking -- each equipment is a unique inventory entry
- [x] Stack limits per item type -- `max_stack` column exists in items table
- [ ] Inventory full rejection message when 100 slots reached
- [ ] Item sorting (client-side, by type/name/weight)

### Item Properties
- [x] Item ID system -- custom ID ranges in `database/init.sql`
- [x] Item types (weapon, armor, card, consumable, etc) -- `item_type` column
- [x] Weight per item -- `weight` column in items table
- [x] NPC buy/sell price -- `price`/`buy_price`/`sell_price` columns
- [x] Class restrictions -- `class_restrictions` JSON column
- [x] Level restrictions -- `required_level` column
- [x] Slot count for equipment -- `slots` column (via migration)
- [x] Item effects/scripts -- handled in `ro_item_effects.js` and `ro_card_effects.js`

### Weight System
- [x] Weight limit formula (2000 + STR*30) -- `getPlayerMaxWeight()` in server
- [x] Enlarge Weight Limit skill bonus -- implemented for Merchant skill ID 600
- [ ] Gym Pass item for non-Merchant classes
- [ ] Job class weight bonuses (per-class +0 to +1000)
- [ ] Peco Peco riding +1000 weight bonus
- [x] 50% overweight: regen stops -- implemented in weight penalty system
- [x] 90% overweight: cannot attack/use skills -- implemented in weight penalty system
- [x] Weight display (sent to client in stats) -- `player:stats` includes weight info
- [x] Cart weight system (separate 8000 cap) -- implemented in CartSubsystem

### Item Interaction
- [x] Pick up items (from monster drops) -- drop system implemented
- [x] Drop items (to ground) -- `inventory:drop` socket event
- [x] Use items (consumables) -- `inventory:use` socket event
- [x] Equip/unequip -- `inventory:equip` socket event
- [x] Item identification (Magnifier) -- `identify:item_list/result` handlers
- [x] Unidentified items (hidden stats, generic name) -- `bIdentified` on FInventoryItem
- [ ] Ground item despawn timer (60s default)
- [ ] Loot protection timer (3s/2s/2s grace periods)
- [ ] Ground item visual (items visible on map floor)
- [ ] Party loot distribution (Each Take / Party Share)

### Special Item Types
- [ ] Account-bound items (NoDrop/NoTrade flags)
- [ ] Character-bound items (NoStorage added)
- [ ] Trade restriction flag system (NoDrop/NoTrade/NoStorage/NoCart/NoGuildStorage/NoMail)
- [ ] Rental items (time-limited with countdown)
- [ ] Quest items (NoDrop + NoTrade + NoStorage)
- [ ] Rental duration tracking and auto-deletion

### Storage Systems
- [ ] Kafra Storage (300 slots, account-shared, no weight limit)
- [ ] Guild Storage (100-500 slots, single-access lock)
- [x] Cart Storage (100 slots, 8000 weight) -- implemented

---

## Gap Analysis

### Critical Gaps (Gameplay-Affecting)

| Gap | Impact | Priority |
|-----|--------|----------|
| **No hard inventory slot limit** | Players can carry unlimited item types (weight is only limiter). Should add 100-slot cap. | HIGH |
| **No job class weight bonuses** | All classes have same base weight. Merchants/Blacksmiths should carry more. | MEDIUM |
| **No ground item system** | Dropped items vanish immediately. No ground visualization, no despawn timer, no loot protection. | HIGH |
| **No loot protection timers** | Anyone can pick up drops instantly. Should implement 3s/2s/2s grace period for kill credit. | HIGH |
| **No trade restriction flags** | All items can be freely traded/dropped. No NoDrop/NoTrade/NoStorage system. | MEDIUM |
| **No Gym Pass for non-Merchants** | Only Merchants can increase weight capacity via skill. Other classes need Gym Pass item. | LOW |

### Moderate Gaps (Quality of Life)

| Gap | Impact | Priority |
|-----|--------|----------|
| **No inventory sorting** | Items appear in acquisition order only. Client-side sorting would improve UX. | LOW |
| **No Kafra Storage** | Players have no shared storage between characters. | MEDIUM |
| **No Guild Storage** | Guilds cannot share items. | LOW (guild system not yet implemented) |
| **No rental item system** | Time-limited items not supported. | LOW |
| **No account-bound items** | Cash shop / event items not restricted. | LOW |
| **No party loot distribution** | Party members must manually coordinate item pickup. | MEDIUM |

### Already Implemented (No Gap)

| Feature | Status |
|---------|--------|
| Weight formula (2000 + STR*30 + EWL) | Implemented |
| 50% / 90% overweight penalties | Implemented |
| Item stacking (consumables vs equipment) | Implemented |
| Equip/unequip pipeline with stat recalc | Implemented |
| Item identification (Magnifier + Identify skill) | Implemented |
| Unidentified item display (generic names, hidden stats) | Implemented |
| Cart system (100 slots, 8000 weight) | Implemented |
| Item use (consumables) | Implemented |
| Item drop (from inventory) | Implemented |
| Stack merge (drag-to-merge) | Implemented |
| Stack split | Implemented |

---

## Sources

- [Inventory - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Inventory)
- [Weight Limit - iRO Wiki Classic](https://irowiki.org/classic/Weight_Limit)
- [Weight Limit - iRO Wiki](https://irowiki.org/wiki/Weight_Limit)
- [Drop System - iRO Wiki](https://irowiki.org/wiki/Drop_System)
- [Enlarge Weight Limit - iRO Wiki](https://irowiki.org/wiki/Enlarge_Weight_Limit)
- [Card System - iRO Wiki](https://irowiki.org/wiki/Card_System)
- [Equipment - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Equipment)
- [Hotkeys - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Hotkeys)
- [Basic Game Control - iRO Wiki](https://irowiki.org/wiki/Basic_Game_Control)
- [Magnifier - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Magnifier)
- [Magnifier - RateMyServer Pre-RE](https://ratemyserver.net/index.php?page=item_db&item_id=611)
- [Item Appraisal - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=40)
- [Party System - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Party_System)
- [Gym Pass - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Gym_Pass)
- [Gym Pass - RateMyServer Pre-RE](https://ratemyserver.net/index.php?page=item_db&item_id=7776)
- [SP Recovery - iRO Wiki Classic](https://irowiki.org/classic/SP_Recovery)
- [HP Recovery - iRO Wiki](https://irowiki.org/wiki/HP_Recovery)
- [rAthena item_db.txt documentation](https://github.com/rathena/rathena/blob/master/doc/item_db.txt)
- [rAthena item_db.yml source](https://github.com/rathena/rathena/blob/master/db/item_db.yml)
- [rAthena drops.conf (floor item lifetime, loot protection)](https://github.com/rathena/rathena/blob/master/conf/battle/drops.conf)
- [rAthena MAX_INVENTORY issue #7173](https://github.com/rathena/rathena/issues/7173)
- [rAthena increasing inventory capacity forum thread](https://rathena.org/board/topic/133352-increasing-inventory-capacity/)
- [rAthena item trade restriction flags](https://rathena.org/board/topic/99989-item-trade/)
- [WarpPortal Forums - Total Max Items on Inventory](https://forums.warpportal.com/index.php?%2Ftopic%2F221684-total-max-items-on-inventory%2F=)
- [rAthena item stack higher than 30000](https://rathena.org/board/topic/80562-item-stack-higher-than-30000-please/)
- [Account-Bound Items - rAthena](https://rathena.org/board/topic/71152-account-bound-items/)
- [Ragnarok Online/Inventory - StrategyWiki](https://strategywiki.org/wiki/Ragnarok_Online/Inventory)
