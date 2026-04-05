# NPCs, Shops & Quests -- Deep Research (Pre-Renewal)

> Comprehensive reference for all NPC archetypes, shop economy, dialogue scripting, quest systems, Kafra services, and special NPCs in Ragnarok Online Classic (pre-renewal). Verified against rAthena pre-re source, iRO Wiki Classic, RateMyServer, and Ragnarok Fandom Wiki.

---

## Table of Contents

1. [NPC Types](#1-npc-types)
2. [Shop System](#2-shop-system)
3. [Kafra Services](#3-kafra-services)
4. [Quest System](#4-quest-system)
5. [NPC Interaction Mechanics](#5-npc-interaction-mechanics)
6. [Special NPCs](#6-special-npcs)
7. [Implementation Checklist](#7-implementation-checklist)
8. [Gap Analysis](#8-gap-analysis)

---

## 1. NPC Types

Ragnarok Online features dozens of NPC archetypes spread across every town. Each NPC has a fixed position on a map, a sprite ID (visual appearance), a facing direction (0-7), and a scripted behavior defined in the rAthena scripting engine. NPCs are defined using tab-separated definitions in `.txt` script files.

### rAthena NPC Definition Types

rAthena supports these top-level NPC type keywords:

| Keyword | Purpose | Syntax |
|---------|---------|--------|
| `script` | Interactive NPC with embedded logic | `map,x,y,dir [TAB] script [TAB] Name [TAB] spriteID,{code}` |
| `shop` | Item vendor accepting Zeny | `map,x,y,dir [TAB] shop [TAB] Name [TAB] spriteID,itemID:price,...` |
| `cashshop` | Item vendor accepting Cash Points | Same as shop but uses Cash Points |
| `warp` | Teleportation point | `map,x,y,dir [TAB] warp [TAB] Name [TAB] spanX,spanY,toMap,toX,toY` |
| `duplicate` | Copy another NPC's script/shop | `map,x,y,dir [TAB] duplicate(label) [TAB] Name [TAB] spriteID` |
| `function` | Reusable code block (no position) | `function [TAB] script [TAB] Name [TAB] {code}` |
| `trader` | Item exchange NPC | Extended barter system |

**Floating NPC** (no position, event-only):
```
- [TAB] script [TAB] Name [TAB] -1,{code}
```

**NPC Naming Convention**: `DisplayName::UniqueName` -- the display name shows to players (max 24 chars), the unique name is for internal script references. The `#` character hides everything after it from the client display (e.g., `Tool Dealer#prt1` shows as "Tool Dealer").

---

### 1.1 Shop NPCs

#### Tool Dealers

**Purpose**: Sell basic consumables -- potions, arrows, utility items, traps.
**Location**: Every major town (typically near the center or market district).
**Behavior**: Opens a buy/sell shop window. No dialogue beyond "Welcome!"

**Standard Core Inventory** (items appearing in most Tool Dealers):

| Item | Item ID | Buy Price | Notes |
|------|---------|-----------|-------|
| Arrow | 1750 | 1z | Basic ammo |
| Silver Arrow | 1751 | 3z | Undead/Shadow element |
| Fire Arrow | 1752 | 3z | Fire element |
| Red Potion | 501 | 50z | Heals ~45-65 HP |
| Orange Potion | 502 | 200z | Heals ~105-145 HP |
| Yellow Potion | 503 | 550z | Heals ~175-235 HP |
| White Potion | 504 | 1,200z | Heals ~325-405 HP |
| Green Potion | 506 | 40z | Cures Poison, Silence, Blind, Confusion |
| Concentration Potion | 645 | 800z | +HIT temporarily |
| Awakening Potion | 656 | 1,500z | +ASPD temporarily |
| Berserk Potion | 657 | 3,000z | +ASPD (stronger, Trans only) |
| Magnifier | 611 | 40z | Identifies unidentified items |
| Fly Wing | 601 | 60z | Random teleport on current map |
| Butterfly Wing | 602 | 300z | Teleport to save point |
| Trap | 1065 | 100z | Used by Hunter trap skills |
| Blue Gemstone | 717 | 600z | Spell reagent (Warp Portal, etc.) |
| Spectacles | 2201 | 3,000z | Headgear (low DEF) |
| Novice Armlet [1] | 2628 | 400z | Accessory for Novices |

**Town-Specific Variations** (from rAthena `npc/merchants/shops.txt`):

| Town | Extra/Notable Items | Sprite ID |
|------|-------------------|-----------|
| Prontera | Spectacles (2239), basic arrows | 53 |
| Geffen | Glasses (2241), arrows | 68 |
| Morroc | No arrows (no bow focus) | 58 |
| Payon | Eastern Arrows (1770) | 77 |
| Alberta | Fire/Silver/Normal arrows | 83 |
| Al De Baran | No arrows, no Green Potion | 83 |
| Comodo | Eastern Arrows (1770), Berserk Potion | 83 |
| Einbroch | All three arrow types, full set | 850 |
| Hugel | Standard + Trap | 53 |
| Juno | Standard (no traps) | 84 |
| Lighthalzen | Berserk Potion, Trap, no Red/Orange Potion | 90 |
| Amatsu | Eastern Arrows (1770), full set | 763 |
| Louyang | Eastern Arrows (1770), full set | 818 |
| Rachel | Trap, no arrows | 919 |
| Veins | Same as Rachel | 943 |

#### Weapon Dealers

**Purpose**: Sell class-appropriate weapons.
**Location**: Weapon Shops in each major town.
**Behavior**: Opens a buy/sell shop window. Inventory is town-specific, reflecting the classes that originate from that town.

**Prontera Weapon Dealer** (representative, from rAthena):

| Item | Item ID | Buy Price | Weapon Lv | Slots |
|------|---------|-----------|-----------|-------|
| Arrow | 1750 | 1z | - | - |
| Silver Arrow | 1751 | 3z | - | - |
| Bow [3] | 1701 | 1,000z | 1 | 3 |
| Knife [3] | 1201 | 50z | 1 | 3 |
| Cutter [3] | 1204 | 1,250z | 1 | 3 |
| Main Gauche [3] | 1207 | 2,400z | 1 | 3 |
| Rod [3] | 1601 | 50z | 1 | 3 |
| Sword [3] | 1101 | 100z | 1 | 3 |
| Falchion [3] | 1104 | 1,500z | 1 | 3 |
| Blade [3] | 1107 | 2,900z | 2 | 3 |
| Rapier [2] | 1110 | 10,000z | 2 | 2 |
| Scimiter [2] | 1113 | 17,000z | 2 | 2 |
| Ring Pommel Saber [2] | 1122 | 24,000z | 3 | 2 |
| Tsurugi [1] | 1119 | 51,000z | 3 | 1 |
| Haedonggum [1] | 1123 | 50,000z | 3 | 1 |
| Flamberge | 1129 | 60,000z | 3 | 0 |
| Katana [3] | 1116 | 2,000z | 2 | 3 |
| Axe [3] | 1301 | 500z | 1 | 3 |

**Town Weapon Focus** (from rAthena shop definitions):

| Town | Focus | Notable Items |
|------|-------|---------------|
| Prontera | Swords, bows, rods, axes | Full range 1H swords up to Flamberge |
| Geffen | Staves, rods, daggers | Rod/Wand/Staff chain, daggers |
| Payon | Bows, katars | Composite Bow [3], Gakkung [2], Cross Bow [2] |
| Morroc | Daggers, katars | Dagger chain, Katar [2], Jur [3] |
| Alberta | Spears, maces | Pike [3], Lance [2], Mace chain |
| Einbroch | Everything | Separate One-Hand and Two-Hand Weapon Dealers |
| Comodo | Instruments, whips | Musical instruments for Bards, whips for Dancers |
| Al De Baran | Daggers only | Full dagger chain through Baselard |

#### Armor Dealers

**Purpose**: Sell armor, shields, garments, shoes, headgear, and accessories.
**Location**: Armor Shops in each major town.

**Prontera Armor Dealer** (from rAthena):

| Item | Item ID | Buy Price | Type |
|------|---------|-----------|------|
| Guard [1] | 2101 | 500z | Shield |
| Buckler [1] | 2103 | 14,000z | Shield |
| Sandals [1] | 2401 | 400z | Footgear |
| Shoes [1] | 2403 | 3,500z | Footgear |
| Hood [1] | 2501 | 1,000z | Garment |
| Muffler [1] | 2503 | 5,000z | Garment |
| Hat | 2220 | 1,000z | Headgear |
| Cap | 2226 | 12,000z | Headgear |
| Cotton Shirt [1] | 2301 | 10z | Armor |
| Adventurer's Suit [1] | 2305 | 1,000z | Armor |
| Wooden Mail [1] | 2328 | 5,500z | Armor |
| Mantle [1] | 2307 | 10,000z | Armor |
| Coat [1] | 2309 | 22,000z | Armor |
| Padded Armor [1] | 2312 | 48,000z | Armor |
| Chain Mail [1] | 2314 | 65,000z | Armor |
| Belt [1] | 2627 | 20,000z | Accessory |
| Novice Armlet [1] | 2628 | 400z | Accessory |

#### Special Vendor Types

| Vendor | Items Sold | Location |
|--------|-----------|----------|
| Ore Merchant | Phracon (200z), Emveretarcon (1,000z) | Near refine NPCs |
| Arrow Crafter NPC | Converts items into specific arrow types | Payon, Morroc |
| Pet Food Merchant | Pet Incubator, class-specific pet food | Pet-focused areas |
| Milk Merchant | Milk (25z) | Prontera (76, 133) |
| Taming Merchant | Taming items for each pet type | Various |

---

### 1.2 Service NPCs

#### Kafra Employees

**Purpose**: Save point, storage, teleport, cart rental, location guide, Kafra Points.
**Location**: Central plazas and dungeon entrances in all towns.
**Visual**: Female NPCs in Kafra Corporation uniform (blue/white).
**Details**: See [Section 3 -- Kafra Services](#3-kafra-services).

#### Nurse / Healer NPCs

**Purpose**: Free HP/SP recovery.
**Location**: Training grounds, novice areas, some towns.
**Behavior**: Dialogue-based. Click to fully restore HP/SP at no cost.
**Restrictions**: Typically only available in beginner areas; some towns charge a fee (50-200z) through Inn NPCs instead.

#### Stylist / Dye NPCs

**Purpose**: Change hair style, hair color, and clothes color.

**Stylist NPC Locations:**

| NPC | Town | Coordinates |
|-----|------|-------------|
| Stylist Jovovich | Prontera | prt_in 243/168 |
| Stylist Maxi | Juno | yuno_in04 186/21 |
| Stylist Aesop | Rachel | ra_in01 186/148 |
| Prince Shammi | Lighthalzen | lhz_in02 100/143 |
| Stylist Veronica | Alberta | alberta 135/37 |

**Costs:**
- Hair style change: 100,000z
- Hair color change: 100,000z
- Premium styles (Lighthalzen): Base Level 70+, Hairstyle Coupon, 3 Counteragent, 3 Mixture, 100 each of Black/Glossy/Golden Hair + Daenggie + Short Daenggie, 99,800z
- Novice free styling: Criatura Academy in Izlude (styles 1-19 only)

**Hair Colors Available**: 8 standard colors (palette varies by class and gender)
**Clothes Dye**: Requires Omni Clothing Dye Coupon (Cash Shop item). Different palettes per class and gender.

**Dyestuff Creation (Java Dullihan NPC, Morroc):**
8 dye colors available: Scarlet, Lemon, White, Orange, Green, Cobalt Blue, Violet, Black. Costs range 3,000-7,000z per dye plus specific crafting ingredients.

#### Refine / Upgrade NPCs

**Purpose**: Upgrade weapons and armor by applying ores.
**Location**: Dedicated refine buildings in each major town (indicated by an Anvil icon on the minimap).

**Refine NPC Locations** (from iRO Wiki Classic):

| NPC | Map | Coordinates | Type |
|-----|-----|-------------|------|
| Hollegren | Prontera | prt_in 63/60 | Weapon + Armor |
| Refiner | Morroc | morocc_in 73/38 | Weapon + Armor |
| Refiner | Payon | payon 144/173 | Weapon + Armor |
| Refiner | Alberta | alberta_in 29/59 | Weapon + Armor |
| Refiner | Juno | yuno basement 119/137 | Weapon + Armor |
| Refiner | Einbroch | ein 2F 255/108 | Weapon + Armor |
| Refiner | Lighthalzen | lhz basement 194/35 | Weapon + Armor |

**Ore Processors**: Convert 5 Rough Oridecon into 1 Oridecon, or 5 Rough Elunium into 1 Elunium, at no Zeny cost.

**Refinement Materials:**

| Equipment Type | Ore Required | Ore Cost | Refine Fee |
|---------------|-------------|----------|------------|
| Lv1 Weapons | Phracon | 200z (shop) | 50z |
| Lv2 Weapons | Emveretarcon | 1,000z (shop) | 200z |
| Lv3 Weapons | Oridecon | Monster drop only | 5,000z |
| Lv4 Weapons | Oridecon | Monster drop only | 20,000z |
| All Armor | Elunium | Monster drop only | 2,000z |

**Safety Limits** (guaranteed success up to this level):

| Equipment | Safe Limit |
|-----------|-----------|
| Lv1 Weapon | +7 |
| Lv2 Weapon | +6 |
| Lv3 Weapon | +5 |
| Lv4 Weapon | +4 |
| All Armor | +4 |

**Success Rates (Normal Ores, Premium/Subscription Servers):**

| Refine Level | Lv1 Weapon | Lv2 Weapon | Lv3 Weapon | Lv4 Weapon | Armor |
|-------------|-----------|-----------|-----------|-----------|-------|
| +4 -> +5 | 100% | 100% | 100% | 60% | 60% |
| +5 -> +6 | 100% | 100% | 60% | 40% | 40% |
| +6 -> +7 | 100% | 60% | 50% | 40% | 40% |
| +7 -> +8 | 60% | 40% | 20% | 20% | 20% |
| +8 -> +9 | 40% | 20% | 20% | 20% | 20% |
| +9 -> +10 | 20% | 20% | 20% | 10% | 10% |

**Success Rates (Enriched Ores, Premium Servers):**

| Refine Level | Lv1 Weapon | Lv2 Weapon | Lv3 Weapon | Lv4 Weapon | Armor |
|-------------|-----------|-----------|-----------|-----------|-------|
| +4 -> +5 | 100% | 100% | 100% | 90% | 90% |
| +5 -> +6 | 100% | 100% | 90% | 60% | 60% |
| +6 -> +7 | 100% | 90% | 75% | 60% | 60% |
| +7 -> +8 | 90% | 60% | 30% | 30% | 30% |
| +8 -> +9 | 60% | 30% | 30% | 30% | 30% |
| +9 -> +10 | 20% | 20% | 20% | 10% | 10% |

**Failure Consequence**: Equipment is **permanently destroyed**, including all slotted cards and enchantments. Zeny and ore are consumed regardless of outcome.

**Weapon ATK Bonuses Per Refine:**

| Weapon Level | ATK Per Refine (Normal) | ATK Per Refine (Overupgrade) |
|-------------|------------------------|------------------------------|
| Lv1 | +2 | +3 |
| Lv2 | +3 | +5 |
| Lv3 | +5 | +7 |
| Lv4 | +7 | +13 |

**Armor DEF**: +1 Equipment DEF per refine level (effective ~0.66 DEF per level in damage reduction).

**Mastersmith Bonus**: The Blacksmith "Upgrade Weapon" skill adds +0.5% success rate per Job Level after 50 (up to +10% at Job 70).

#### Card Removal NPC (Card Desocketing)

**Purpose**: Remove cards from slotted equipment.

**Pre-Renewal (Classic) Method:**
- **NPC**: Card Remover (old wise woman)
- **Location**: Prontera forge / Prontera Inn (28, 73)
- **Cost**: 200,000z base + 25,000z per card in the equipment + 1 Yellow Gemstone + 1 Star Crumb
- **Success Rate**: 90%
- **On Success**: Both equipment and card(s) are returned intact
- **On Failure**: Nothing is lost (equipment and cards remain as-is)

**Post-Classic (Malangdo) Method** (later episodes):
- **NPC**: Richard at Malangdo (220/160)
- **Options**: 4 tiers using Zeny or Lubricant items (5%/40%/80%/100% success)
- **On Failure**: Card remains in item, neither destroyed

#### Stat/Skill Reset NPCs (Hypnotist)

**Purpose**: Reset allocated stat points or skill points.

**Locations:**

| Service | NPC | Location | Coordinates |
|---------|-----|----------|-------------|
| Skill Reset | Hypnotist | Prontera | 146/232 |
| Skill Reset | Hypnotist | Izlude | 127/175 |
| Stat Reset | Hypnotist | Izlude (alternate channels A/B/C/D) | 127/175 |

**Level Requirements:**
- **Stat Reset**: Base Level 50 and below (First Class Jobs only)
- **Skill Reset**: Base Level 99 and below (Main Classes), Base Level 50 and below (Expanded Classes)

**Cost**: Free (no Zeny required)

**Restrictions:**
- Character must have 0 Weight (empty inventory)
- No active Mount or Animal Companion
- Must remove Pushcart
- Basic Skill must be returned to Lv9 before allocating First Class skill points (on some servers)
- Repeatable as long as level requirements are met

**For Level 100+**: Cash Shop item "Neuralizer" required for skill resets.

#### Repairman NPC

**Purpose**: Repair equipment that has been broken by enemy skills (e.g., Power Thrust weapon break, Acid Terror armor break).
**Location**: Near refine NPCs in most towns.
**Cost**: Varies by item (typically 5,000-50,000z depending on item value).
**Note**: Weapon Repair skill (Blacksmith) can also repair equipment without an NPC.

#### Inn NPCs

**Purpose**: Rest and recover HP/SP for a small fee.
**Location**: Most major towns have at least one inn.
**Cost**: 50-200z for full HP/SP recovery.
**Behavior**: Dialogue-based interaction.

---

### 1.3 Quest NPCs

#### Job Change NPCs

**Purpose**: Guide players through job change quests and officially change their class.
**Location**: Class-specific guilds in various towns.

| Class | Guild Location | Key NPC Name |
|-------|---------------|-------------|
| Swordsman | Izlude -- Swordsman Guild | Master Swordsman |
| Mage | Geffen -- Magic Academy (NW building) | Expert Mage |
| Archer | Payon -- Archer Village | Archer Guildsman |
| Thief | Morroc -- Pyramid Basement (Thief Guild) | Thief Guild Member |
| Merchant | Alberta -- Merchant Guild (SW corner) | Chief Mahnsoo |
| Acolyte | Prontera -- Church | Father Mareusis |
| Knight | Prontera -- Chivalry (NW corner) | Chivalry Captain |
| Crusader | Prontera -- Chivalry | Crusader Guildsman |
| Wizard | Geffen -- Wizard Tower (top floor) | Catherine Medichi |
| Sage | Juno -- Sage Academy | Metheus Sylphe |
| Priest | Prontera -- Church | High Bishop |
| Monk | Prontera -- St. Capitolina Abbey | Sensei Moohae |
| Hunter | Hugel -- Hunter Guild | Hunter Guildsman (Sherin) |
| Bard/Dancer | Comodo -- Bard/Dancer Guild | Bard/Dancer Guildsman |
| Assassin | Morroc -- Assassin Guild (2S 2W) | Assassin Guildsman |
| Rogue | Comodo -- Rogue Guild | Rogue Guildsman |
| Blacksmith | Einbroch -- Blacksmith Guild (SE) | Altiregen |
| Alchemist | Al De Baran -- Alchemist Guild | Alchemist Guildsman |

See [Section 4.1 -- Job Change Quests](#41-job-change-quests) for detailed quest requirements.

#### Platinum Skill NPCs (Quest Skill NPCs)

**Purpose**: Teach quest-only skills not available through normal skill points.
**Location**: Inside respective class guild buildings, or via a centralized Platinum Skill NPC in Prontera Main Office.
**Behavior**: Multi-step quest involving item collection, then grants a bonus skill.
**Note**: Some servers consolidate all platinum skill quests into a single NPC for convenience.

See [Section 4.3 -- Quest Skills](#43-quest-skills-platinum-skills) for the complete list.

#### Headgear Quest NPCs

**Purpose**: Craft specific headgear from collected materials.
**Location**: Scattered throughout towns (often in shops, guilds, or inns).
**Note**: There are hundreds of headgear quests. See [Section 4.4 -- Equipment/Headgear Quests](#44-equipmentheadgear-quests) for representative examples.

#### Bounty Board NPCs

**Purpose**: Repeatable monster hunting quests for EXP and Zeny.
**Location**: Near tool shops in select towns.
**Mechanic**: Kill 150 of a specific monster from nearby fields/dungeons.
**Reward**: EXP + (Monster Level x 100) Zeny.
**Bonus**: Every 4th turn-in grants 1 Eden Merit Badge.

---

### 1.4 Warp NPCs

**Purpose**: Teleport players between maps/zones.
**rAthena Syntax**: `map,x,y,dir [TAB] warp [TAB] Name [TAB] spanX,spanY,toMap,toX,toY`

Warp NPCs are invisible trigger zones on the map. When a player walks into the trigger area (defined by `spanX` x `spanY` cells), they are teleported to the destination map at `(toX, toY)`.

**Types:**
- **Town Exit Warps**: At the edges of town maps, leading to adjacent fields
- **Dungeon Entrance Warps**: At dungeon entrances
- **Building Entrance/Exit Warps**: Indoor maps
- **Inter-Town Warps**: Some NPCs offer dialogue-based warping (Kafra, Guide NPCs)

**Town Warper NPC** (convenience, training grounds): Teleport directly between major towns. Free service.

**Dungeon Warper NPC** (convenience, training grounds): Teleport directly to dungeon entrances. Free service.

---

### 1.5 Information / Guide NPCs

**Purpose**: Provide information about the town, nearby areas, and points of interest.
**Location**: Near town entrances.
**Behavior**: Opens a multi-page dialogue with town maps, NPC locations, and directions to key facilities. Some Guide NPCs can mark locations on the minimap.
**Cost**: Free.

---

### 1.6 Guild NPCs

**Guild Creation:**
- Requires 1 Emperium item
- Type `/guild "guild name"` while having Emperium in inventory
- Creator becomes Guild Master permanently

**Guild Skill NPC**: Located in guild castles. Guild Master allocates guild skill points earned from guild EXP.

**War of Emperium (WoE) NPCs:**
- Castle gatekeepers (entry control during WoE)
- Treasure Room NPCs (guild leader access only, daily treasure chest spawns)
- Emperium guardian NPCs (guardian spawning)
- Castle Investment NPCs (Commerce/Defense investments)

---

### 1.7 Arena NPCs

**Purpose**: PvP and monster arena access.

| Arena Type | Location | Description |
|------------|----------|-------------|
| PvP Arena | Prontera | Teleports to PvP rooms (1v1, 5v5, free-for-all) |
| Monster Arena | Izlude | Survival challenge vs monster waves |
| Turbo Track | Prontera | Racing minigame |

---

## 2. Shop System

### 2.1 Currency

**Zeny** is the sole NPC currency. Max stack: 2,147,483,647z (signed 32-bit integer).

Sources of Zeny:
- Selling items to NPCs (primary source)
- Quest rewards
- Monster drops (Zeny drops from some monsters)
- Ore Discovery skill (Blacksmith passive)

Zeny sinks:
- NPC shop purchases
- Kafra services (storage, teleport, cart)
- Refining (ore + fee)
- Guild creation (Emperium cost)
- Job change quest fees
- Stylist fees

### 2.2 Buy/Sell Price Formula

Every item in the database has a `buy_price` (the NPC shop price).

**Sell-back price to NPC:**
```
sell_price = floor(buy_price / 2)
```

- Items with `buy_price = 0` cannot be sold to NPCs (quest items, etc.)
- Items with no `buy_price` but a defined `sell_price` use that value directly (monster loot, crafting materials)

**rAthena Shop Syntax:**
```
map,x,y,dir  shop  ShopName  spriteID,itemID:price,itemID:price,...
```
When `price` is `-1`, the system uses the item's default buy price from the item database.

### 2.3 Discount Skill (Buying from NPCs)

Merchant passive skill. Reduces NPC buy prices.
**Prerequisite**: Enlarge Weight Limit Lv3.

| Level | Discount % | Effective Buy |
|-------|-----------|--------------|
| 1 | 7% | 93% of price |
| 2 | 9% | 91% of price |
| 3 | 11% | 89% of price |
| 4 | 13% | 87% of price |
| 5 | 15% | 85% of price |
| 6 | 17% | 83% of price |
| 7 | 19% | 81% of price |
| 8 | 21% | 79% of price |
| 9 | 23% | 77% of price |
| 10 | 24% | 76% of price |

**Formula**: `discount_rate = 5 + (level * 2)` (capped at 24% for level 10).

**Example**: White Potion (1,200z) at Discount Lv10 = `1,200 * 0.76 = 912z`.

### 2.4 Overcharge Skill (Selling to NPCs)

Merchant passive skill. Increases NPC sell prices.
**Prerequisite**: Discount Lv3.

| Level | Overcharge % | Effective Sell |
|-------|-------------|---------------|
| 1 | 7% | 107% of sell price |
| 2 | 9% | 109% of sell price |
| 3 | 11% | 111% of sell price |
| 4 | 13% | 113% of sell price |
| 5 | 15% | 115% of sell price |
| 6 | 17% | 117% of sell price |
| 7 | 19% | 119% of sell price |
| 8 | 21% | 121% of sell price |
| 9 | 23% | 123% of sell price |
| 10 | 24% | 124% of sell price |

**Formula**: `overcharge_rate = 5 + (level * 2)` (capped at 24% for level 10).

### 2.5 Shop Inventories by Town

| Town | Tool Dealer | Weapon Focus | Armor Focus | Special Shops |
|------|------------|-------------|-------------|---------------|
| Prontera | Full set | Swords, bows, rods | Full armor range | Milk Merchant, Pet shops |
| Geffen | Standard | Staves, rods, daggers | Mage-oriented robes | Magic item vendor |
| Payon | Standard | Bows, katars | Eastern armor, garments | Arrow crafting NPC |
| Morroc | Standard | Daggers, katars | Light armor | Taming items |
| Alberta | Standard | Spears, maces | Shields, heavy armor | Merchant Guild supplies |
| Izlude | Standard | Beginner weapons | Beginner armor | Novice supplies |
| Al De Baran | Standard | Daggers only | Mixed heavy | Alchemist supplies |
| Einbroch | Full set | Everything (2 dealers) | Full heavy armor | Blacksmith supplies |
| Comodo | Standard | Instruments, whips | Mixed | Dancer/Bard supplies |
| Hugel | Standard | Mixed | Mixed | Hunter supplies |
| Juno | Standard | Mixed (Geffen-like) | Mixed (Geffen-like) | Sage Academy supplies |
| Lighthalzen | Reduced | Eastern mix | Mixed | Premium items |

### 2.6 Player Vending System

**Skill**: Vending (MC_VENDING)
**Max Level**: 10
**SP Cost**: 30
**Prerequisites**: Pushcart Lv3

**Items Per Level**: Max stacks = 2 + Skill Level (3 at Lv1, 12 at Lv10)

**Setup Process:**
1. Have Pushcart equipped (visible behind character)
2. Place items to sell in the Pushcart inventory (NOT main inventory)
3. Use the Vending skill
4. Set a shop title (displayed above character)
5. Set prices per item (max 1,000,000,000z per item)
6. Confirm -- character becomes stationary with "shop" sign

**Restrictions:**
- Must be at least 4 cells from any NPC
- Cannot move while vending
- Cannot use skills while vending
- Only Pushcart items can be sold

**Commission Fee:**
- Items priced at 10,000,000z or above: 5% commission deducted from sale
- Items under 10,000,000z: no commission

**Buying Store** (Open Buying Store skill):
- Prerequisites: Vending Lv1
- Learn from "Purchasing Team Mr. Huh" at Merchant Guild (Alberta 37/41) for 10,000z
- Requires Bulk Buyer Shop Licenses (200z each, up to 50 at once)
- Maximum 5 item types per shop (2 for non-merchants)
- Only Etc. items and non-brewed consumables can be purchased
- Must have at least 1 of desired item already in inventory

---

## 3. Kafra Services

### 3.1 Overview

Kafra Corporation is the primary service provider NPC network. Kafra employees are female NPCs in distinctive blue/white uniforms found in every major town. Not all services are available at every Kafra.

### 3.2 Save Point

- **Cost**: Free
- **Function**: Sets respawn location to the Kafra's position (or nearby designated save coordinates)
- **Usage**: Unlimited, repeatable
- **Mechanic**: On death, player respawns at last saved Kafra location

### 3.3 Storage

- **Cost**: 40z per access (500z on some servers like iRO Valkyrie and kRO F2P)
- **Capacity**: 600 unique item stacks (300 on Valkyrie)
- **Scope**: **Account-wide** -- all characters on the same account share one storage
- **Prerequisite**: Novice Basic Skill Level 6
- **Features**:
  - Deposit and withdraw items
  - Items persist across sessions and server restarts
  - Cannot deposit equipped items
  - Cannot deposit quest items (typically)
  - Can store Zeny (via storage Zeny deposit)

### 3.4 Teleportation Service

Teleport destinations and costs vary by the Kafra's town location. Costs scale with distance.

**Representative Teleport Costs (from Prontera Kafra):**

| Destination | Cost |
|------------|------|
| Izlude | 600z |
| Geffen | 1,200z |
| Payon | 1,200z |
| Morroc | 1,200z |
| Alberta | 1,200z |
| Al De Baran | 1,800z |
| Comodo | 2,400z |
| Juno | 1,800z |

**Important Notes:**
- Not all destinations are available from all Kafras
- Prontera serves as the central hub with the most destinations
- Some remote Kafras only offer teleport to Prontera
- Free Ticket for Kafra Transportation (item) waives the fee once per ticket
- Teleport service availability varies by server (limited to Prontera/Juno on iRO Valkyrie)

### 3.5 Cart Rental

Available only to Merchant class and Super Novice (with Pushcart skill).

**Cart Rental Costs by Town:**

| Town | Cost |
|------|------|
| Aldebaran | 600z |
| Amatsu, Kunlun, Louyang | 700z |
| Geffen | 750z |
| Prontera, Juno, Einbroch, Lighthalzen, Hugel, Rachel | 800z |
| Izlude | 820z |
| Alberta | 850z |
| Payon, Morroc | 930z |
| Comodo | 1,000z |
| Archer Village | 1,200z |

### 3.6 Kafra Points

- **Earning Rate**: 1 Kafra Point per 10z spent on Kafra services
- **Redemption**: Available at the Kafra Headquarters in Al De Baran
- **Lottery Tiers**: 1,000 / 3,000 / 5,000 / 7,000 / 10,000 points
- **Rewards**: Potions, seeds, and utility items (random from tier pool)

### 3.7 Location Guide Service

- **Cost**: Free
- **Function**: Shows positions of other Kafra employees on the same map
- **Behavior**: Text-based dialogue listing nearby Kafra locations

### 3.8 Kafra Locations by Town

| Town | Typical Positions |
|------|-------------------|
| Prontera | Central plaza, south gate |
| Geffen | Central area |
| Payon | Central village |
| Morroc | Near pyramid entrance, town center |
| Alberta | Near port, town center |
| Izlude | Near arena, central area |
| Al De Baran | Clock tower area, Kafra HQ |
| Einbroch | Factory district |
| Comodo | Beach area |
| Hugel | Town center |
| Juno | Central area |
| Rachel | Temple district |
| Lighthalzen | Corporate district |

---

## 4. Quest System

### 4.1 Job Change Quests

#### Quest Categories

| Category | Description | Repeatable? |
|----------|-------------|-------------|
| Job Change | Required to change class | No (once per character) |
| Platinum Skill | Grants bonus quest-only skills | No |
| Headgear/Equipment | Craft specific gear from materials | No (per item) |
| Access | Unlock dungeon/area access | No |
| Story/Narrative | Advance world lore | No |
| Bounty Board | Hunt N monsters for EXP/Zeny | Yes |
| Repeatable EXP | Turn in monster parts for EXP | Yes |
| Daily | Time-gated repeatable quests | Yes (per day) |
| Event | Seasonal/limited-time content | Varies |

#### First Job Change (Novice -> 1st Class)

**Universal Requirements:**
- Job Level 10 as Novice
- All 9 skill points allocated to Basic Skill (Basic Skill Lv9)

##### Swordsman (Izlude -- Swordsman Guild)

1. Talk to Master Swordsman in the Swordsman Guild
2. Talk to Test Guide in the left room to learn about the test
3. Talk to Test Hall Staff to enter the obstacle course
4. Complete a 3-room obstacle course (walking/jumping without falling)
5. Return to Master Swordsman to receive Swordsman class
- **Items Required**: None
- **Test Type**: Skill-based (obstacle course)

##### Mage (Geffen -- Magic Academy)

1. Talk to Expert Mage in the Magic Academy (geffen 63, 177)
2. Receive a solution number assignment
3. Read the bookshelf to learn required ingredients and mixing code
4. Collect items: varies by solution number (may include Milk from Prontera 76/133 at 25z, Payon Solution from Archer Village)
5. Mix the solution in the mixing machine (correct ingredients + catalyst + code)
6. Return the Unknown Solution to the Expert Mage
- **Items Required**: Varies by solution number (ingredients + Zeny)
- **Test Type**: Item collection + puzzle

##### Archer (Payon -- Archer Village)

1. Talk to Archer Guildsman in Payon Archer Village
2. Hunt Willows in Payon fields to collect Trunks
3. Return with enough Trunks (need 25+ points; Trunk = 1 point each)
- **Items Required**: Trunks (from Willows)
- **Test Type**: Monster hunting
- **Note**: Avoid Elder Willows (too strong at this level)

##### Thief (Morroc -- Pyramids)

1. Travel to the Pyramids (NW of Morroc)
2. Enter the Thief Guild in the pyramid basement
3. Talk to guide NPCs, answer questions
4. Find the "Irrelevant Man" outside the Pyramid
5. Go to the mushroom farm and collect mushrooms:
   - Orange Net Mushroom = 3 points each
   - Orange Gooey Mushroom = 1 point each
   - Need 25+ points total
6. Return to Comrade Brad in the Pyramid Dungeon
- **Items Required**: Mushrooms (farmed on-site)
- **Test Type**: Collection quest

##### Merchant (Alberta -- Merchant Guild)

1. Talk to Chief Mahnsoo at the Merchant Guild (SW Alberta)
2. Pay a 1,000z registration fee
3. Receive a parcel delivery assignment
4. Deliver the parcel to the designated NPC (varies)
5. Receive a Voucher as receipt
6. Return the Voucher to the Merchant Guildsman
7. Chief Mahnsoo officially admits you to the guild
- **Items Required**: 1,000z fee
- **Test Type**: Delivery quest

##### Acolyte (Prontera -- Church)

1. Talk to Father Mareusis in Prontera Church
2. He sends you to find one of 3 Ascetics (randomly chosen):
   - Mother Marthilda
   - Father Rubalkabara
   - Father Yosuke
3. Travel to the assigned Ascetic's location (varies)
4. Complete their task (varies by Ascetic -- usually a pilgrimage or item fetch)
5. Return to Father Mareusis
- **Items Required**: Varies by Ascetic
- **Test Type**: Pilgrimage / item fetch

#### Second Job Change (1st Class -> 2nd Class)

**Universal Requirements:**
- Job Level 40+ (can attempt quest)
- Job Level 50 (can skip some tests/item collections on certain servers)

Each 1st class can branch into one of two 2nd classes:

| 1st Class | 2nd Class Options |
|-----------|------------------|
| Swordsman | Knight / Crusader |
| Mage | Wizard / Sage |
| Archer | Hunter / Bard (M) or Dancer (F) |
| Thief | Assassin / Rogue |
| Merchant | Blacksmith / Alchemist |
| Acolyte | Priest / Monk |

##### Knight (Prontera Chivalry)

1. Speak to Chivalry Captain (prontera 48/343)
2. Talk to Sir Andrew -- item collection (skip at Job 50)
3. Talk to Sir Siracuse -- knowledge quiz about Knight class
4. Talk to Sir Windsor -- teleport to waiting room
5. Talk to Lady Amy -- another quiz
6. Talk to Sir Edmond -- teleported to a map with Porings, Lunatics, Chonchons. **Do NOT attack them.** Wait 5 minutes.
7. Talk to Sir Gray -- ethics/personality quiz (answer as friendly/altruistic person)
8. Return to Chivalry Captain for job change
- **Test Type**: Quiz + patience test
- **Items Required**: Varies (collection test at step 2)

##### Wizard (Geffen Tower)

1. Talk to Catherine Medichi at the top of Geffen Tower
2. Take Raul Expagarus's quiz (10 questions, 10 points each, need 90+)
3. Three timed combat rooms -- defeat elemental monsters within 3 minutes each
4. Return to Catherine Medichi for job change
- **Test Type**: Quiz + combat trial
- **Cost**: 70,000z registration fee (Sage quest from Metheus Sylphe)

##### Priest (Prontera Church)

1. Visit three NPCs: Father Rubalkabara, Mother Marthilda, Father Yosuke
2. **Zombie Exorcism**: Kill all zombies in a room
3. **Temptation Resistance**: Walk past evil beings and say "Devil, Be Gone"
4. Return to the High Bishop for job change
- **Test Type**: Combat + willpower test

##### Hunter (Hugel)

1. Talk to Hunter Guildsman (Sherin) in Hugel
2. Complete a trap-setting test in a maze
3. Pass a monster identification quiz
4. Hunt specific monsters to prove hunting skill
5. Return for job change
- **Test Type**: Trap skill + quiz + combat

##### Assassin (Morroc area)

1. Travel to the Assassin Guild (2 maps south, 2 maps west of Morroc)
2. Pass a stealth infiltration test (navigate a map without being detected)
3. Collect specific poisons or items
4. Return for job change
- **Test Type**: Stealth + collection

##### Blacksmith (Einbroch)

1. Talk to Altiregen at the Blacksmith Guild in SE Einbroch
2. Collect specific ores and materials
3. Pass a forging test (craft specific items)
4. Answer quiz about metals and smithing
5. Return for job change
- **Test Type**: Collection + crafting + quiz

##### Other Second Classes

Crusader, Sage, Rogue, Monk, Alchemist, Bard/Dancer each follow a similar pattern: guild visit -> multi-part testing (quiz, combat, collection) -> return for job change. Specific item requirements and test details vary per class.

#### Rebirth / Transcendent Process

**Requirements:**
- Base Level 99, Job Level 50
- Must be a 2-1 or 2-2 class
- All skill points and stat points used
- 0 weight (no items in inventory, nothing equipped)
- No Cart, Falcon, or Peco Peco
- Exactly 1,285,000z

**Process:**
1. Go to Juno -- Sage Job Change area
2. Pay ~1,285,000z to the Valkyrie NPC
3. Complete a mini-quest to find "the Heart of Ymir"
4. Transported to Valhalla
5. Reborn as High Novice (1/1)

**After Rebirth:**
- Higher base stats than regular Novice
- At Job Level 10: change to High 1st Class (High Swordsman, High Mage, etc.)
- At Job Level 40-50: change to Transcendent 2nd Class (Lord Knight, High Wizard, etc.)
- Transcendent classes gain access to powerful new skills

**Complete Job Tree:**
```
[Novice Lv10/Jlv10] -> 1st Class
[1st Class Jlv40-50] -> 2nd Class (2-1 or 2-2)
[2nd Class Blv99/Jlv50] -> Rebirth -> High Novice
[High Novice Jlv10] -> High 1st Class
[High 1st Class Jlv40-50] -> Transcendent 2nd Class

Swordsman -> Knight / Crusader -> Lord Knight / Paladin
Mage -> Wizard / Sage -> High Wizard / Scholar (Professor)
Archer -> Hunter / Bard or Dancer -> Sniper / Clown or Gypsy
Thief -> Assassin / Rogue -> Assassin Cross / Stalker
Merchant -> Blacksmith / Alchemist -> Whitesmith / Creator (Biochemist)
Acolyte -> Priest / Monk -> High Priest / Champion
```

---

### 4.3 Quest Skills (Platinum Skills)

Quest skills are special skills learned from NPCs through item collection quests, not through the normal skill point system. Each class has a set of quest skills.

#### Novice Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| First Aid | Nurse Officer | Prontera | 7 | Newbie Tag |
| Play Dead | ? | Training Grounds | 5 | (quest completion) |

#### Swordsman Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Fatal Blow | Leon Von Frich | prt_in 75/88 (Knight's Guild) | 25 | 10 Fire Arrow, 10 Silver Arrow, 1 Banana Juice, 30 Tentacle, 5 Royal Jelly |
| Moving HP Recovery | Knight De Thomas | izlude_in 175/130 | 35 | 200 Empty Bottle, 1 Moth Wings |
| Auto Berserk | Juan | prt_in 94/57 (Weapon Refine Shop) | 30 | 35 Powder of Butterfly, 10 Horrendous Mouth, 10 Decayed Nail, 10 Honey |

#### Mage Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Energy Coat | Great Wizard (Blizzardriss) | geffen_in 151/119 (Mage Guild) | 35 | 3 Glass Bead, 1 1-Carat Diamond, 5 Shell, 1 Solid Shell |

**Item Acquisition Tips**: Glass Beads can be bought from Gift Merchant in SW Prontera for 1,400z each. The 1-Carat Diamond can be purchased from a Jeweler at East Morroc Ruins.

#### Archer Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Arrow Crafting | Roberto | moc_ruins 118/99 (Morroc Ruins center) | 30 | 20 Resin, 7 Mushroom Spore, 41 Pointed Scale, 13 Trunk, 1 Red Potion |
| Charge Arrow (Arrow Repel) | Jason | payon 103/63 (SW Payon, near tree) | 35 | 2 Emerald, 3 Yoyo Tail, 10 Tentacle, 10 Bill of Birds, 36 Banana Juice |

#### Thief Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Sand Attack | Alcouskou | moc_prydb1 154/128 (Pyramids, opposite Thief Guild) | 25 | 5 Fine Grit, 1 Leather Bag of Infinity (sub-quest from RuRumuni in Payon 91/77: 1 Cobweb, 1 Cactus Needle, 1 Earthworm Peeling) |
| Back Sliding | Alcoukowski | moc_prydb1 154/128 | 35 | 20 Grasshopper's Leg |
| Pick Stone | Alcoukowski | moc_prydb1 154/128 | 20 | 1 Zargon, 1 Bear's Footskin, 5 Spawn |
| Throw Stone (Stone Fling) | Alcoukowski | moc_prydb1 154/128 | 15 | 2 Garlet, 2 Scell |

#### Merchant Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Cart Revolution | Gershuan | alberta 232/106 (east, near harbor) | 35 | 2 Grape Juice, 15-23 Iron, 25-32 Sticky Mucus, 5-20 Fly Wing, 5-6 Tentacle |
| Change Cart | Charlron | alberta 119/221 (outside Hotel, right side) | 30 | 50 Trunk, 10 Iron, 20 Animal Skin |
| Crazy Uproar | Necko | alberta 83/96 (middle of second street) | 15 | 7 Pearl, 1 Banana Juice, 50 Mushroom Spore |

**Note for Cart Revolution**: The item quantities have variable ranges; the NPC may request different amounts. Continue talking until he accepts your quantity.

#### Acolyte Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Holy Light | (Prontera Church NPC) | Prontera Church | 30 | (item collection quest) |

#### Knight Quest Skills

| Skill | NPC | Location | Job Level Req | Items Required |
|-------|-----|----------|--------------|----------------|
| Charge Attack | Essofeit | prt_in 85/99 (Knight's Guild) | 40 | 5 Candy Cane, 3 Witherless Rose |

**Charge Attack Quest Steps:**
1. Talk to Knight Essofeit inside the Knight's Guild
2. Exit and observe knights training in the garden between Prontera Castle and Knight's Guild
3. Interact with invisible NPC at prontera (69, 351), then Grand Master at prontera (72, 352)
4. Talk to three training knights in order: Gon (73,357), Jiya (78,357), Zabi (78,354)
5. Return to Essofeit, then speak with Gatack near a tree at prontera (66, 358)
6. Collect items and return to Essofeit for the skill reward

**Witherless Roses** can be purchased from a Trader near Morroc's East exit.

#### Second Class Quest Skills (Representative)

| Class | Skill | Notes |
|-------|-------|-------|
| Wizard | Sight Blaster | Quest from Geffen Tower |
| Priest | Redemptio | Quest from Prontera Church |
| Hunter | Phantasmic Arrow | Quest from Hunter Guild |
| Assassin | Sonic Acceleration | Quest from Assassin Guild |
| Blacksmith | Greed | Quest from Blacksmith Guild |
| Crusader | Shrink | Quest from Chivalry |
| Sage | Create Elemental Converter | Quest from Sage Academy |
| Monk | Ki Translation | Quest from St. Capitolina Abbey |
| Rogue | Close Confine | Quest from Rogue Guild |
| Bard | Pang Voice | Quest from Comodo |
| Dancer | Charming Wink | Quest from Comodo |
| Alchemist | Bioethics | Quest from Al De Baran |

**Note**: Job level requirement for 2nd class quest skills typically does not apply if the character has already changed to the 2nd class. Only the 1st class base requirement matters (e.g., Swordsman must be Job 25+ for Fatal Blow, but Knights/Crusaders have no level requirement).

---

### 4.4 Equipment/Headgear Quests

Headgear crafting is a major quest category in RO Classic. NPCs in various towns accept specific combinations of monster drops and materials to produce headgear items. There are hundreds of headgear quests.

**Representative Headgear Crafting Quests:**

| Headgear | NPC | Location | Required Materials |
|----------|-----|----------|--------------------|
| Alarm Mask | Muscle Man | Inside Al De Baran | 3,000 Needle of Alarm, 1 Mr. Scream |
| Angel Wing Ears | Old Blacksmith | Juno | 1 Angel Wing, 1 Elven Ears, 20,000z |
| Antlers | Cherokee | Inside Alberta | 20 Evil Horn |
| Baby Pacifier | Kid | Lighthalzen | 1 Royal Jelly, 1 Nursing Bottle, 1 Pacifier, 1 Nose Ring |
| Beanie | Hat Merchant | Lutie Tool Shop | 1 Cap, 500 Yarn |
| Black Cat Ears | Neko Neko | Payon | 1 Kitty Band, 200 Fluff, 1 Black Dyestuffs, 10,000z |
| Bunny Band | Kafra Employee | Alberta | 100 Feather, 1 Pearl, 1 Four Leaf Clover, 1 Kitty Band |
| Cake Hat | Vending Machine | Lutie Town | 10 Candy, 5 Candy Cane, 15 Cookie, 20 Piece of Cake, 10 Steel |
| Doctor Band | Trader | Inside Al De Baran | 1 Red Bandana, 50 Iron, 1 Cracked Diamond, 3,500z |
| Headset | Eric | Inside Geffen | 40 Steel, 1 Coal, 1 Alcohol, 1 Oridecon, 2,000z |
| Heart Hairpin | Stylish Merchant | Alberta | 1,200 Coral Reef |
| Sunglasses | Sunglasses Trader | Alberta | 1 One-Carat Diamond, 50 Feathers, 100,000z |

**Quest Pattern**: Talk to NPC -> Learn required items -> Collect items -> Return to NPC -> Receive headgear.

---

### 4.5 Access Quests (Dungeon Unlocks)

Some dungeons require quests to enter:

| Dungeon | Access Method |
|---------|--------------|
| Amatsu Dungeon | Talk to Lord of Palace (4th floor), cure his sick mother via Kitsune Mask shrine NPC |
| Turtle Island Dungeon | Alberta boat quest + entry fee |
| Thanatos Tower | Key item quest from nearby NPCs, solve puzzles at each level |
| Nameless Island | Multi-step quest chain from Rachel |
| Nifflheim | Quest or direct map walking |
| Glast Heim | No quest needed -- walk from Geffen (W, W, N, W) |
| Sphinx | No quest needed -- walk from Morroc (W, W) |
| Pyramids | No quest needed -- walk from Morroc (NW) |

---

### 4.6 Repeatable Quests

#### Bounty Board Quests
- **Location**: Near tool shops in select towns
- **Mechanic**: Kill 150 of a specific monster from nearby fields/dungeons
- **Reward**: EXP + (Monster Level x 100) Zeny
- **Bonus**: Every 4th turn-in grants 1 Eden Merit Badge

#### Repeatable EXP Quests
- **Options**: Choose to hunt 50, 100, or 150 monsters
- **Scaling**: EXP per kill is the same regardless of chosen amount (linear)
- **Cooldown**: Once per day (resets at midnight server time)

---

### 4.7 Quest Tracking

RO Classic has a basic quest log:
- **Quest Window** (Alt+U): Shows active quests, objectives, and progress
- **Quest Icons**: Yellow "!" above NPCs with available quests, grey "?" for in-progress
- **Map Markers**: Some quests add markers to the minimap
- **No automatic tracking**: Players must remember locations and requirements

---

## 5. NPC Interaction Mechanics

### 5.1 Dialogue Windows

The RO dialogue window has these elements:
1. **NPC Name** -- displayed at the top of the window
2. **Portrait** (optional) -- NPC face image
3. **Message Area** -- scrollable text field
4. **Next Button** -- advances to the next page (clears current text)
5. **Close Button** -- ends the conversation
6. **Choice Menu** -- numbered list of clickable options
7. **Input Box** -- text or number input field (rare)

### 5.2 rAthena Script Commands

#### Core Dialogue Commands

| Command | Function | Description |
|---------|----------|-------------|
| `mes "text"` | Display message line | Shows one line of text in dialogue window |
| `next` | Page break | Clears window, waits for "Next" click |
| `close` | End dialogue | Closes dialogue window, triggers OnEnd label |
| `close2` | Force close | Closes without triggering OnEnd label |
| `end` | Terminate script | Stops script execution |
| `menu "opt1",L1,"opt2",L2` | Choice menu (goto) | Shows options, jumps to label on selection |
| `select("opt1","opt2")` | Choice menu (return) | Shows options, returns selected index (1-based) |
| `input var` | Input box | Shows text/number input, stores in variable |
| `callshop "ShopName",1` | Open shop | Opens shop (1=buy, 2=sell) |

#### Color and Formatting

```
mes "^FF0000Red text^000000";      // Hex color codes with ^ prefix
mes "^0000FFBlue text^000000";
mes "[NPC Name]";                   // Convention: brackets = speaker name
```

#### Flow Control

```
if (condition) { ... }
for (set .@i, 0; .@i < 10; set .@i, .@i + 1) { ... }
switch (variable) { case 1: break; }
goto L_Label;
```

#### Variable Scopes

| Prefix | Scope | Persistence | Storage |
|--------|-------|-------------|---------|
| (none) | Character | Permanent | Database (char_reg_num/str) |
| `@` | Character | Temporary (session) | Memory |
| `$` | Global | Permanent | Database (mapreg) |
| `$@` | Global | Temporary | Memory |
| `.` | NPC | Temporary | Memory |
| `.@` | Local scope | Temporary | Memory |
| `'` | Instance | Temporary | Memory |
| `#` | Account (local) | Permanent | Database (acc_reg_num) |
| `##` | Account (global) | Permanent | Database (global_acc_reg_num) |

Append `$` for strings: `@name$`, `.@text$`, etc. All types support array indexing: `.@items[0]`.

#### Special Labels (Event Hooks)

| Label | Trigger |
|-------|---------|
| `OnInit` | Server startup or script reload |
| `OnTouch` | Player enters NPC trigger area |
| `OnTouchNPC` | Monster enters NPC trigger area |
| `OnPCLoginEvent` | Player logs in |
| `OnPCDieEvent` | Player dies |
| `OnPCBaseLvUpEvent` | Player gains a base level |
| `OnNPCKillEvent` | Player kills a monster |
| `OnTimer<ms>` | Countdown timer expires |
| `OnClock<HHMM>` | Real-world time trigger |

#### NPC Visibility System

| Command | Effect |
|---------|--------|
| `disablenpc "name"` | Hide NPC globally |
| `enablenpc "name"` | Show NPC globally |
| `hidenpc "name"` | Hide for specific player |
| `shownpc "name"` | Show for specific player |
| `cloaknpc "name"` | Toggle cloak state per player |

### 5.3 Shop Interaction

**Shop NPC Definition:**
```
map,x,y,dir  shop  ShopName  spriteID,itemID:price,itemID:price,...
```

**Dynamic Shop Modification:**
```
npcshopitem "ShopName",itemID,price;    // Change price of existing item
npcshopadditem "ShopName",itemID,price; // Add item to shop
npcshopdelitem "ShopName",itemID;       // Remove item from shop
```

**Opening Shop from Script:**
```
callshop "ShopName",1;   // 1=buy window, 2=sell window
```

### 5.4 Quest Integration Commands

| Command | Function |
|---------|----------|
| `setquest <quest_id>` | Start quest (add to quest log) |
| `completequest <quest_id>` | Mark quest as complete |
| `erasequest <quest_id>` | Remove quest from log |
| `checkquest <quest_id>` | Query status (-1=not started, 1=in progress, 2=completed) |

Quests can track monster kills against specific targets defined in `quest_db.yml` with filtering by race, size, element, level range, and location.

### 5.5 RID (Character ID) Attachment

Scripts require a RID (character ID) to operate on character data. Player-triggered events automatically attach the RID, but event-based scripts (`OnInit`, `OnTimer`, `OnClock`) receive RID = 0.

Manual attachment:
- `attachrid(<account_id>)` -- Attach specific player
- `detachrid` -- Detach current RID
- `playerattached()` -- Query current RID

### 5.6 Character Data Access

Scripts access character data through system variables:

| Variable | Description |
|----------|-------------|
| `Zeny` | Currency amount |
| `BaseLevel` / `JobLevel` | Character levels |
| `Hp` / `MaxHp`, `Sp` / `MaxSp` | Health/Mana |
| `Weight` / `MaxWeight` | Carry capacity |
| `Sex` | 0 = Female, 1 = Male |
| `Class` | Job ID |
| `Upper` | 0 = Normal, 1 = Advanced, 2 = Baby |

### 5.7 Multi-Page Dialogue Flow Example

A typical RO NPC conversation flow:
```
1. Player clicks NPC
2. mes "Welcome to Prontera!"       --> Page 1
3. mes "How can I help?"
4. next                              --> "Next" button appears
5. mes "Choose a service:"           --> Page 2
6. menu "Buy items",L_Buy,
        "Sell items",L_Sell,
        "Leave",L_Leave;            --> Choice menu appears
7. (player picks "Buy items")
8. goto L_Buy;
9. callshop "PrtToolShop",1;        --> Shop window opens
10. close;                           --> Dialogue closes
```

### 5.8 Input Box Usage

Used for numeric or text input in some quests:
```
// Numeric input
mes "How many potions do you want?";
input .@amount;
if (.@amount < 1 || .@amount > 100) {
    mes "Invalid amount.";
    close;
}

// Text input
mes "Enter the secret password:";
input .@password$;
if (.@password$ == "yggdrasil") {
    mes "Correct!";
}
```

---

## 6. Special NPCs

### 6.1 Dead Branch

- **Item ID**: 604
- **Type**: Consumable item (not an NPC)
- **Effect**: Summons a random monster when used
- **Restrictions**: Cannot summon renewal-only monsters on pre-renewal servers
- **Use Cases**: Card farming (specific boss monsters), fun events, leveling
- **Variants**:
  - **Dead Branch** (604): Summons random regular monster
  - **Bloody Branch** (12103): Summons random MVP/boss monster
  - **Poring Box**: Summons Porings specifically

### 6.2 Old Blue Box

- **Item ID**: 603
- **Type**: Consumable item
- **Effect**: Gives a random item when opened
- **Loot Pool**: Hundreds of items, weighted by rarity
- **Quest** (some servers): Old Man in Comodo (68, 195) starts the OBB quest
- **Note**: Not available on all servers; quest availability varies

### 6.3 Guild NPCs

| NPC Type | Location | Function |
|----------|----------|----------|
| Guild Storage NPC | Guild Castles | Access guild-shared storage (only during WoE ownership) |
| Guild Skill NPC | Guild Castles | Guild Master allocates guild skill points |
| Treasure Room NPC | Guild Castles | Guild leader access, daily treasure chest spawns |
| Castle Investment NPC | Guild Castles | Commerce Investment (+treasure quality), Defense Investment (+guardian strength) |
| Emperium Guardian NPC | Guild Castles | Spawn guardian monsters during WoE |
| WoE Gatekeeper NPC | Guild Castles | Control castle entry during WoE |
| Kafra (Guild) | Guild Castles | Specialized Kafra with guild storage access |

### 6.4 Resetter NPCs

See [Section 1.2 -- Stat/Skill Reset NPCs](#statskill-reset-npcs-hypnotist) for details.

Summary:
- **Skill Reset**: Free, Base Level 99 and below, Prontera (146/232) or Izlude (127/175)
- **Stat Reset**: Free, Base Level 50 and below, Izlude alternate channels
- **Cash Shop Reset**: Neuralizer item for Level 100+ characters

### 6.5 Pet-Related NPCs

| NPC | Function | Location |
|-----|----------|----------|
| Pet Groomer | Evolve pets | Various towns |
| Pet Incubator Merchant | Sell Pet Incubators | Near pet areas |
| Taming Gift Set Merchant | Sell taming items | Near pet areas |

### 6.6 Cool Event Corp. NPCs

- **Purpose**: Seasonal and limited-time event quests
- **Location**: Various towns (change per event)
- **Examples**: Halloween candy quests, Christmas gift quests, anniversary events
- **Rewards**: Event-exclusive headgear, costumes, consumables

### 6.7 Pushcart Dealer

- **Purpose**: Rent pushcarts for Merchant class characters
- **Location**: Alberta Merchant Guild
- **Note**: Can also rent from Kafra (see Kafra Cart Rental section)

---

## 7. Implementation Checklist

### Phase 1: Core NPC Infrastructure
- [ ] Server-side NPC Registry (`ro_npc_data.js`) with NPC_TYPES constants
- [ ] NPC data structure: npcId, type, name, spriteId, zone, position, facing, interactRadius, shopId, dialogueId, questId, isActive
- [ ] `npc:interact` socket handler with zone check and range check
- [ ] Client-side `AMMONPC` actor class (data-driven, set NpcId per instance)
- [ ] Click detection integration in `PlayerInputSubsystem` or `SabriMMOCharacter`
- [ ] NPC name tag rendering via `NameTagSubsystem`
- [ ] NPC spawn on `player:join` (send visible NPCs in zone)

### Phase 2: Shop System
- [ ] Shop Registry (`SHOP_REGISTRY` in `ro_npc_data.js` or `ro_shop_data.js`)
- [ ] `npc:shop_open` / `npc:shop_buy` / `npc:shop_sell` socket handlers
- [ ] Server-side buy/sell price calculation with Discount/Overcharge
- [ ] Zeny validation (sufficient funds check)
- [ ] Inventory weight check on buy
- [ ] Client-side Shop UI (`ShopSubsystem` + `SShopWidget`) -- buy/sell tabs
- [ ] Item quantity selection (numeric input or +/- buttons)
- [ ] NPC shop inventories for each town (per rAthena shops.txt)

### Phase 3: Dialogue System
- [ ] Dialogue Registry (`DIALOGUE_REGISTRY` in `ro_dialogue_data.js`)
- [ ] Dialogue page system with `next` / `close` flow
- [ ] Choice menu support (menu options -> label jumps)
- [ ] Dialogue state machine on socket (currentPage, variables, questContext)
- [ ] Client-side Dialogue UI (`DialogueSubsystem` + `SDialogueWidget`)
- [ ] NPC name header, message area, Next/Close buttons, choice list
- [ ] Text color support (`^RRGGBB` parsing)

### Phase 4: Kafra Services
- [ ] Kafra NPC type with service menu
- [ ] Save Point service (update character spawn location)
- [ ] Storage service (account-wide, 600 slots, 40z fee)
- [ ] Teleport service (destination list per Kafra location, distance-based pricing)
- [ ] Cart Rental service (Merchant class check, town-based pricing)
- [ ] Kafra Points tracking (1 point per 10z spent)
- [ ] Client-side Kafra UI (already partially implemented: `KafraSubsystem`)

### Phase 5: Quest System
- [ ] Quest Registry (`QUEST_REGISTRY` in `ro_quest_data.js`)
- [ ] Quest state machine: NOT_STARTED -> IN_PROGRESS -> COMPLETED
- [ ] Quest state persistence in database (`character_quests` table)
- [ ] Quest objective tracking (item collection, monster kills, NPC visits)
- [ ] Quest reward distribution (EXP, Zeny, items, skills)
- [ ] Client-side Quest Log UI (`QuestSubsystem` + `SQuestWidget`, Alt+U hotkey)

### Phase 6: Job Change System
- [ ] Job change NPC handlers for all 6 first classes
- [ ] Job level + Basic Skill prerequisite check
- [ ] Class change logic (update character class, reset job level/EXP, grant new skill tree)
- [ ] Second job change NPC handlers for all 12 second classes
- [ ] Job Level 40/50 branching logic
- [ ] Rebirth system (Blv99/Jlv50 -> High Novice)

### Phase 7: Quest Skills (Platinum Skills)
- [ ] Platinum skill NPC handlers for all classes
- [ ] Item requirement validation
- [ ] Skill grant logic (add quest skill to character's skill list)
- [ ] Centralized Platinum Skill NPC option (all-in-one convenience)

### Phase 8: Refine System
- [ ] Refine NPC handler (`refine:request` -- already partially implemented)
- [ ] Safety limit enforcement
- [ ] Success rate calculation (normal + enriched ores)
- [ ] Failure -> item destruction
- [ ] Ore Processor NPC (5 rough -> 1 refined)

### Phase 9: Specialized NPCs
- [ ] Stylist/Dye NPC (hair style, hair color, clothes color change)
- [ ] Stat/Skill Reset NPC (Hypnotist)
- [ ] Card Removal NPC
- [ ] Healer NPC (free HP/SP recovery)
- [ ] Guide NPC (town info dialogue)
- [ ] Repairman NPC

### Phase 10: Headgear Quests & Repeatable Content
- [ ] Headgear quest NPC handlers (representative 20+ headgear)
- [ ] Bounty Board NPC (repeatable monster hunting)
- [ ] Daily quest system with cooldown tracking
- [ ] Dungeon access quests (Amatsu, Turtle Island, Thanatos Tower)

---

## 8. Gap Analysis

### Currently Implemented in Sabri_MMO

Based on the codebase review:

| System | Status | Files |
|--------|--------|-------|
| Kafra NPC (basic) | Partial | `KafraSubsystem.h/cpp`, `SKafraWidget.h/cpp` |
| Shop NPC (basic) | Partial | `ShopSubsystem` referenced in docs |
| Refine System | Implemented | Server `refine:request` handler in `index.js` |
| Forge System | Implemented | Server `forge:request` handler in `index.js` |
| Card Compound | Implemented | Server card compound handlers |
| Vending | Implemented | `VendingSubsystem`, `SVendingSetupPopup`, `SVendingBrowsePopup` |
| Cart System | Implemented | `CartSubsystem`, `SCartWidget` |
| Identify System | Implemented | `SIdentifyPopup` |
| Job Change (server-side class check) | Basic gate | Quest skill NPC gates in server |

### Not Yet Implemented

| System | Priority | Complexity | Notes |
|--------|----------|------------|-------|
| Generic NPC Actor (`AMMONPC`) | High | Medium | Core infrastructure for all NPC types |
| Dialogue System | High | High | Multi-page dialogue, choices, input, state machine |
| Dialogue UI (`SDialogueWidget`) | High | Medium | Slate panel for NPC conversation |
| NPC Registry (server data) | High | Low | Data module with all NPC definitions |
| Shop Registry (server data) | High | Low | Per-NPC item inventories with prices |
| Full Kafra Teleport Service | Medium | Low | Destination list per town, pricing |
| Kafra Storage (account-wide) | Medium | Medium | New DB table, shared across characters |
| Kafra Save Point | Low | Low | Update spawn coordinates on save |
| Quest System (full) | High | Very High | State machine, DB persistence, objectives, rewards |
| Quest Log UI | Medium | Medium | Alt+U quest window |
| Job Change Quests | High | High | Class-specific multi-step quests |
| Platinum Skill Quests | Medium | Medium | Item collection -> skill grant |
| Stylist NPC | Low | Low | Hair/color change with Zeny cost |
| Stat/Skill Reset NPC | Low | Low | Level-gated free reset |
| Card Removal NPC | Low | Low | Fee + RNG success |
| Healer NPC | Low | Very Low | Instant HP/SP restore |
| Guide NPC | Very Low | Very Low | Static dialogue text |
| Headgear Quests | Low | Medium | Many individual recipes |
| Bounty Board | Low | Medium | Repeatable with cooldown |
| Dungeon Access Quests | Medium | Medium | One-time gating |
| NPC Spawn System | High | Medium | Server sends NPC list per zone to client |
| NPC Name Tags | Medium | Low | Reuse NameTagSubsystem |

### Key Architectural Decisions Needed

1. **Dialogue Engine**: Should dialogue trees be defined as JSON data or as a simple scripted state machine? JSON trees are more flexible but harder to author. A simplified state machine with page arrays + choice branches covers 95% of RO NPC interactions.

2. **Quest Persistence**: New `character_quests` table with columns: `character_id`, `quest_id`, `status` (not_started/in_progress/completed), `progress` (JSON blob for objective tracking), `started_at`, `completed_at`. Auto-create at server startup.

3. **NPC Spawn Protocol**: On `player:join`, the server should emit `npc:list` with all active NPCs in the player's current zone. On zone transition, emit the new zone's NPC list. NPCs are static (no movement/despawn), so a single load is sufficient.

4. **Shop Data Organization**: Either embed shop inventories in the NPC registry or create a separate `SHOP_REGISTRY`. Separate registry is cleaner since multiple NPCs can share the same shop (e.g., duplicate Tool Dealers).

5. **Kafra Storage Scope**: Account-wide storage requires a new `account_storage` table keyed by `user_id` (not `character_id`). This is architecturally different from character inventory.

---

## Sources

- [iRO Wiki Classic -- Kafra](https://irowiki.org/classic/Kafra)
- [iRO Wiki -- Refinement System](https://irowiki.org/wiki/Refinement_System)
- [iRO Wiki Classic -- Refinement System](https://irowiki.org/classic/Refinement_System)
- [iRO Wiki -- Card Desocketing](https://irowiki.org/wiki/Card_Desocketing)
- [iRO Wiki -- Beginner Stat & Skill Reset Hypnotist](https://irowiki.org/wiki/Beginner_Stat_%26_Skill_Reset_Hypnotist)
- [iRO Wiki -- Job Change Guides](https://irowiki.org/wiki/Category:Job_Change_Guide)
- [iRO Wiki Classic -- Headgear Ingredients](https://irowiki.org/classic/Headgear_Ingredients)
- [RateMyServer -- Platinum Skill Quests](https://ratemyserver.net/quest_db.php?type=50000)
- [RateMyServer -- Swordsman Quest Skills](https://ratemyserver.net/quest_db.php?type=50000&qid=50002)
- [RateMyServer -- Thief Quest Skills](https://ratemyserver.net/quest_db.php?type=50000&qid=50014)
- [RateMyServer -- Merchant Quest Skills](https://ratemyserver.net/quest_db.php?type=50000&qid=50006)
- [RateMyServer -- Archer Quest Skills](https://ratemyserver.net/quest_db.php?type=50000&qid=50004)
- [RateMyServer -- Mage Quest Skills](https://ratemyserver.net/quest_db.php?type=50000&qid=50003)
- [RateMyServer -- Knight Quest Skills](https://ratemyserver.net/quest_db.php?type=50000&qid=50007)
- [RateMyServer -- NPC Shop Search](https://ratemyserver.net/index.php?page=nsw_shop_search)
- [RateMyServer -- Refine Success Rates](https://ratemyserver.net/index.php?page=misc_table_refine)
- [rAthena -- NPC Shop Definitions (shops.txt)](https://github.com/rathena/rathena/blob/master/npc/merchants/shops.txt)
- [rAthena -- Basic Scripting Wiki](https://github.com/rathena/rathena/wiki/Basic-Scripting)
- [rAthena -- Script Commands Reference](https://github.com/rathena/rathena/blob/master/doc/script_commands.txt)
- [rAthena -- General Shop Creation](https://github.com/rathena/rathena/wiki/General_Shop_creation)
- [DeepWiki -- rAthena Monster and NPC System](https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)
- [Ragnarok Fandom Wiki -- Kafra Corporation](https://ragnarok.fandom.com/wiki/Kafra_Corporation)
- [Ragnarok Fandom Wiki -- Tool Dealer](https://ragnarok.fandom.com/wiki/Tool_Dealer)
- [Ragnarok Fandom Wiki -- Weapon Dealer](https://ragnarok.fandom.com/wiki/Weapon_Dealer)
