# 07 -- NPCs, Quests & Shops

> Ragnarok Online Classic reference document for Sabri_MMO implementation.
> Covers every NPC archetype, the shop economy, dialogue scripting, quest systems, Kafra services, player vending, and implementation notes for our Node.js + UE5 stack.

---

## Table of Contents

1. [NPC Types](#1-npc-types)
2. [Shop System](#2-shop-system)
3. [Dialogue System](#3-dialogue-system)
4. [Quest System](#4-quest-system)
5. [Key Quest Lines](#5-key-quest-lines)
6. [Kafra Services](#6-kafra-services)
7. [Player Vending](#7-player-vending)
8. [Implementation Plan](#8-implementation-plan)

---

## 1. NPC Types

Ragnarok Online features dozens of NPC archetypes spread across every town. Each NPC has a fixed position, a sprite, a facing direction, and a scripted behavior. Below is the full taxonomy.

### 1.1 Tool Dealers

**Purpose:** Sell basic consumables -- potions, arrows, utility items.
**Location:** Every major town (typically near the center or market district).
**Behavior:** Opens a buy/sell shop window. No dialogue beyond "Welcome!"

Standard inventory (Prontera Tool Dealer as reference):

| Item | ID | Buy Price |
|------|----|-----------|
| Arrow | 1750 | 1z |
| Red Potion | 501 | 10z |
| Orange Potion | 502 | 50z |
| Yellow Potion | 503 | 180z |
| White Potion | 504 | 1,200z |
| Green Potion | 506 | 40z |
| Concentration Potion | 645 | 800z |
| Awakening Potion | 656 | 1,500z |
| Magnifier | 611 | 40z |
| Fly Wing | 601 | 60z |
| Butterfly Wing | 602 | 300z |
| Trap | 1065 | 100z |

Inventories vary slightly by town. Morroc's Tool Dealer adds desert-specific items. Payon's adds Eastern-themed consumables. Alberta's adds ocean/fishing items.

### 1.2 Weapon Dealers

**Purpose:** Sell class-appropriate weapons.
**Location:** Weapon Shops in each major town.
**Behavior:** Opens a buy/sell shop window. Inventory is town-specific.

Prontera Weapon Dealer 1 (representative):

| Item | ID | Buy Price |
|------|----|-----------|
| Knife [3] | 1201 | 50z |
| Sword [3] | 1101 | 101z |
| Bow [3] | 1701 | 1,000z |
| Rod [3] | 1601 | 50z |
| Cutter [3] | 1204 | 1,250z |
| Falchion [3] | 1104 | 1,500z |
| Katana [3] | 1116 | 2,000z |
| Main Gauche [3] | 1207 | 2,400z |
| Blade [3] | 1107 | 2,900z |
| Rapier [2] | 1110 | 10,000z |
| Scimiter [2] | 1113 | 17,000z |
| Ring Pommel Saber [2] | 1122 | 24,000z |
| Saber [2] | 1126 | 49,000z |
| Haedonggum [1] | 1123 | 50,000z |
| Tsurugi [1] | 1119 | 51,000z |
| Flamberge | 1129 | 60,000z |
| Axe [3] | 1301 | 500z |

Geffen sells more staves and rods. Morroc sells daggers and katars. Payon sells bows. Alberta sells spears.

### 1.3 Armor Dealers

**Purpose:** Sell armor, shields, garments, shoes, headgear, and accessories.
**Location:** Armor Shops in each major town.

Prontera Armor Dealer (representative):

| Item | ID | Buy Price |
|------|----|-----------|
| Cotton Shirt | 2301 | 10z |
| Jacket | 2303 | 200z |
| Sandals | 2401 | 400z |
| Guard | 2101 | 500z |
| Hat | 2220 | 1,000z |
| Hood | 2501 | 1,000z |
| Adventurer's Suit | 2305 | 1,000z |
| Shoes | 2403 | 3,500z |
| Muffler | 2503 | 5,000z |
| Wooden Mail | 2328 | 5,500z |
| Mantle | 2307 | 10,000z |
| Cap | 2226 | 12,000z |
| Buckler | 2103 | 14,000z |
| Belt [1] | 2627 | 20,000z |
| Coat | 2309 | 22,000z |
| Padded Armor | 2312 | 48,000z |
| Chain Mail | 2314 | 65,000z |
| Novice Armlet [1] | 2628 | 400z |

### 1.4 Kafra Employees

**Purpose:** Save point, storage, teleport, cart rental.
**Location:** Central plazas and dungeon entrances in all towns.
**Visual:** Female NPCs in Kafra Corporation uniform (blue/white).
**Details:** See [Section 6 -- Kafra Services](#6-kafra-services).

### 1.5 Job Change NPCs

**Purpose:** Guide players through job change quests and officially change their class.
**Location:** Class-specific guilds in various towns.

| Class | Guild Location | Key NPC |
|-------|---------------|---------|
| Swordsman | Izlude -- Swordsman Guild | Master Swordsman |
| Mage | Geffen -- Magic Academy (NW building) | Expert Mage |
| Archer | Payon -- Archer Village | Archer Guildsman |
| Thief | Morroc -- Pyramid Basement (Thief Guild) | Thief Guild Member |
| Merchant | Alberta -- Merchant Guild (SW corner) | Chief Mahnsoo |
| Acolyte | Prontera -- Church | Father Mareusis |
| Knight | Prontera -- Chivalry (NW corner) | Chivalry Captain |
| Wizard | Geffen -- Wizard Tower (top floor) | Catherine Medichi |
| Priest | Prontera -- Church | High Bishop |
| Hunter | Hugel -- Hunter Guild | Hunter Guildsman |
| Assassin | Morroc -- Assassin Guild (2S 2W) | Assassin Guildsman |
| Blacksmith | Einbroch -- Blacksmith Guild (SE) | Altiregen |
| Crusader | Prontera -- Chivalry | Crusader Guildsman |
| Sage | Juno -- Sage Academy | Sage Guildsman |
| Rogue | Comodo -- Rogue Guild | Rogue Guildsman |
| Monk | Prontera -- St. Capitolina Abbey | Monk Guildsman |
| Alchemist | Al De Baran -- Alchemist Guild | Alchemist Guildsman |
| Bard/Dancer | Comodo -- Bard/Dancer Guild | Bard/Dancer Guildsman |

### 1.6 Skill NPCs (Platinum Skills)

**Purpose:** Teach quest-only skills not available through normal skill points.
**Location:** Inside respective class guild buildings, or via a centralized Platinum Skill NPC in Prontera Main Office.
**Behavior:** Multi-step quest involving item collection or monster hunting, then grants a bonus skill.

Examples:
- **Swordsman**: Moving HP Recovery (quest from Swordsman Guild)
- **Knight**: Charge Attack (Essofeit, Prontera Knight Guild prt_in 85,99)
- **Priest**: Redemptio (quest from Prontera Church)
- **Wizard**: Sight Blaster (quest from Geffen Tower)

Some servers consolidate all platinum skill quests into a single NPC for convenience.

### 1.7 Refine / Upgrade NPCs

**Purpose:** Upgrade weapons and armor by applying ores.
**Location:** Dedicated refine buildings in each major town (near weapon shops).

**Key NPCs:**
- **Hollegren** (weapon refiner) -- found in most towns
- **Suhnbi** (armor refiner) -- found in most towns
- **Ore Processors** -- convert 5 Rough Oridecon into 1 Oridecon, or 5 Rough Elunium into 1 Elunium

**Refinement Materials:**

| Equipment Type | Ore Required | Refine Cost |
|---------------|-------------|-------------|
| Level 1 Weapons | Phracon | 50z |
| Level 2 Weapons | Emveretarcon | 200z |
| Level 3 Weapons | Oridecon | 5,000z |
| Level 4 Weapons | Oridecon | 20,000z |
| All Armor | Elunium | 2,000z |

**Safety Limits** (guaranteed success up to):

| Equipment | Safe Limit |
|-----------|-----------|
| Lv1 Weapon | +7 |
| Lv2 Weapon | +6 |
| Lv3 Weapon | +5 |
| Lv4 Weapon | +4 |
| All Armor | +4 |

**Success Rates (Normal Ores, beyond safety limit):**

| Refine Level | Lv1 Weapon | Lv2 Weapon | Lv3 Weapon | Lv4 Weapon | Armor |
|-------------|-----------|-----------|-----------|-----------|-------|
| +5 | 100% | 100% | 100% | 60% | 60% |
| +6 | 100% | 100% | 60% | 40% | 40% |
| +7 | 100% | 60% | 50% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 10% | 20% |
| +10 | 20% | 20% | 10% | 10% | 10% |

**Failure consequence:** Equipment is permanently destroyed (including slotted cards). Enriched ores (Kafra Shop) offer improved rates. HD (High-Density) ores prevent destruction on failure but the refine level drops by 1.

### 1.8 Dye & Stylist NPCs

**Purpose:** Change hair style, hair color, and clothes color after character creation.

**Stylist NPC Locations:**

| NPC | Town | Coordinates |
|-----|------|------------|
| Stylist Jovovich | Prontera | prt_in 243/168 |
| Stylist Maxi | Juno | yuno_in04 186/21 |
| Stylist Aesop | Rachel | ra_in01 186/148 |
| Prince Shammi | Lighthalzen | lhz_in02 100/143 |
| Stylist Lonza | Lasagna | lasagna 134/113 |
| Stylist Veronica | Alberta | alberta 135/37 |

**Costs:**
- Hair style change: 100,000z
- Hair color change: 100,000z
- Lighthalzen premium styles (L1-L7): requires Base Level 70+, VIP or Hairstyle Coupon, 3 Counteragent, 3 Mixture, 100 each of Black/Glossy/Golden Hair + Daenggie + Short Daenggie, 99,800z
- Novice free styling: Criatura Academy in Izlude (styles 1-19)

**Dyestuff Creation (Java Dullihan, Morroc):**
8 dye colors: Scarlet, Lemon, White, Orange, Green, Cobalt Blue, Violet, Black. Costs range 3,000-7,000z per dye. Requires specific crafting ingredients.

**Clothes Dye:**
Requires Omni Clothing Dye Coupon (from Kafra Shop). Different palettes per class and gender.

### 1.9 Inn NPCs

**Purpose:** Rest and recover HP/SP. Some inns double as PvP arena access points.
**Location:** Most major towns have at least one inn.
**Behavior:** Dialogue-based. Pay a small fee (50-200z) to fully restore HP/SP. Prontera has 2 inns (one provides PvP access).

### 1.10 Guide NPCs

**Purpose:** Provide information about the town, nearby areas, and points of interest.
**Location:** Near town entrances.
**Behavior:** Opens a multi-page dialogue with town maps, NPC locations, and directions to key facilities. Some Guide NPCs can mark locations on the minimap.

### 1.11 Quest NPCs

**Purpose:** Give quests ranging from item collection to monster hunting to exploration.
**Location:** Scattered throughout towns and dungeons.
**Types:**
- **Headgear Quest NPCs** -- craft specific headgear from collected materials
- **Equipment Quest NPCs** -- upgrade or create special equipment
- **Access Quest NPCs** -- grant access to restricted areas
- **Bounty Board NPCs** -- repeatable hunt quests near tool shops
- **Story NPCs** -- advance narrative quest lines

### 1.12 Guild NPCs

**Purpose:** Guild management, War of Emperium, and guild skill NPCs.

**Guild Creation:**
- Requires 1 Emperium item
- Type `/guild "guild name"` while having Emperium in inventory
- Creator becomes Guild Master permanently

**Guild Skill NPC:** Located in guild castles. Guild Master allocates guild skill points.

**War of Emperium (WoE) NPCs:**
- Castle gatekeepers
- Treasure Room NPCs (guild leader access only)
- Emperium guardian NPCs

### 1.13 Arena NPCs

**Purpose:** PvP and monster arena access.
**Location:** Prontera (PvP Arena), Izlude (Monster Arena).
**Types:**
- **PvP Arena NPC** -- teleports to PvP rooms of various sizes (1v1, 5v5, free-for-all)
- **Monster Arena NPC** -- survival challenge against waves of monsters for EXP/items
- **Turbo Track NPC** -- racing minigame

### 1.14 Other Specialized NPCs

| NPC Type | Function | Location |
|----------|----------|----------|
| Healer | Free HP/SP recovery | Training grounds, novice areas |
| Town Warper | Teleport between towns | Training grounds (convenience) |
| Dungeon Warper | Teleport to dungeon entrances | Training grounds (convenience) |
| Resetter | Reset stats or skills | Various towns |
| Taming Merchant | Sell pet taming items | Pet-focused towns |
| Card Remover | Remove cards from equipment (destroys item) | Geffen, Payon |
| Repairman | Repair broken equipment | Near refine NPCs |
| Pushcart Dealer | Rent pushcarts for Merchants | Alberta Merchant Guild |
| Milk Merchant | Sell Milk (quest ingredient) | Prontera (76, 133), 25z |
| Cool Event Corp. | Seasonal event quests | Various |

---

## 2. Shop System

### 2.1 Currency

**Zeny** is the sole NPC currency. Max stack: 2,147,483,647z (signed 32-bit integer).
- Earned by: selling items to NPCs, completing quests, monster drops
- Spent on: NPC shops, Kafra services, refining, teleporting, guild creation

### 2.2 Buy/Sell Price Formula

Every item in the database has a `buy_price` (the NPC shop price).

**Sell-back price to NPC:**
```
sell_price = floor(buy_price / 2)
```

Items with `buy_price = 0` cannot be sold to NPCs (quest items, etc.).
Items with no `buy_price` but a defined `sell_price` use that value directly (monster loot, crafting materials).

### 2.3 Discount Skill (Buying from NPCs)

Merchant passive skill. Reduces NPC buy prices.

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

**Formula:** `discount_rate = 5 + (level * 2)` (capped at 24% for level 10).
**Prerequisite:** Enlarge Weight Limit Lv3.

**Example:** White Potion (1,200z) at Discount Lv10 = `1,200 * 0.76 = 912z`.

### 2.4 Overcharge Skill (Selling to NPCs)

Merchant passive skill. Increases NPC sell prices.

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

**Formula:** `overcharge_rate = 5 + (level * 2)` (capped at 24% for level 10).
**Prerequisite:** Discount Lv3.

**Example:** Item with sell price 10,000z at Overcharge Lv10 = `10,000 * 1.24 = 12,400z`.

### 2.5 Shop Inventories by Town

Each town has a distinct set of shops reflecting its culture and class focus:

| Town | Tool Dealer | Weapon Focus | Armor Focus | Special Shops |
|------|------------|-------------|-------------|---------------|
| Prontera | Full set | Swords, bows, rods | Full armor range | Milk Merchant, Pet shops |
| Geffen | Standard | Staves, rods, daggers | Mage-oriented robes | Magic item vendor |
| Payon | Standard | Bows, katars | Eastern armor | Arrow crafting NPC |
| Morroc | Standard | Daggers, katars | Light armor | Taming items, Black Market |
| Alberta | Standard | Spears, maces | Shields, heavy armor | Merchant Guild supplies |
| Izlude | Standard | Beginner weapons | Beginner armor | Novice supplies |
| Al De Baran | Standard | Mixed | Mixed | Alchemist supplies |
| Einbroch | Standard | Heavy weapons | Heavy armor | Blacksmith supplies |
| Comodo | Standard | Mixed | Mixed | Dancer/Bard supplies |
| Hugel | Standard | Mixed | Mixed | Hunter supplies |
| Juno | Standard | Mixed | Mixed | Sage Academy supplies |

### 2.6 Special Vendor Types

- **Ore Merchants:** Sell Phracon (200z) and Emveretarcon (1,000z) for basic refining
- **Arrow Crafters:** Convert items into specific arrow types (fire, silver, etc.)
- **Pet Food Merchants:** Sell class-specific pet food
- **Ammunition Dealers:** Sell bullets and spheres for Gunslinger/Ninja classes

---

## 3. Dialogue System

### 3.1 Dialogue Box Anatomy (RO Client)

The RO dialogue window has these elements:
1. **NPC Name** -- displayed at the top of the window
2. **Portrait** (optional) -- NPC face image
3. **Message Area** -- scrollable text field
4. **Next Button** -- advances to the next page
5. **Close Button** -- ends the conversation
6. **Choice Menu** -- numbered list of clickable options
7. **Input Box** -- text or number input field (rare)

### 3.2 rAthena Script Commands Reference

rAthena (the open-source RO server emulator) defines the scripting language used for all NPC behavior. Key commands relevant to our implementation:

#### NPC Definition
```
map,x,y,direction  script  NPCName  spriteID,{
    // script body
}
```
- `direction`: 0-8 (counterclockwise from south)
- `spriteID`: determines visual appearance
- Names must be unique (up to 24 characters)

#### Core Dialogue Commands

| Command | Function | Our Equivalent |
|---------|----------|---------------|
| `mes "text"` | Display message line | `dialogue:message` |
| `next` | Clear window, wait for "Next" click | `dialogue:next` page break |
| `close` | Close dialogue window | `dialogue:close` |
| `menu "opt1",L1,"opt2",L2` | Show choice menu, jump to label | `dialogue:choices` |
| `select("opt1","opt2")` | Show choice menu, return index | `dialogue:choices` (returns index) |
| `input var` | Show input box, store in variable | `dialogue:input` |
| `close2` | Close without triggering OnEnd label | `dialogue:force_close` |
| `end` | Terminate script execution | (implicit) |

#### Color and Formatting
```
mes "^FF0000Red text^000000";      // Hex color codes with ^ prefix
mes "^0000FFBlue text^000000";
mes "[NPC Name]";                   // Convention: brackets = speaker name
```

#### Shop Commands
```
// Define a shop NPC
map,x,y,dir  shop  ShopName  spriteID,itemID:price,itemID:price,...

// Open shop from script
callshop "ShopName",1;   // 1=buy, 2=sell

// Dynamic pricing
npcshopitem "ShopName",itemID,price;    // Change price
npcshopadditem "ShopName",itemID,price; // Add item
npcshopdelitem "ShopName",itemID;       // Remove item
```

#### Flow Control
```
if (condition) {
    // branch
}

for (set .@i, 0; .@i < 10; set .@i, .@i + 1) {
    // loop
}

switch (variable) {
    case 1: /* ... */ break;
    case 2: /* ... */ break;
}

goto L_Label;
```

#### Variable Scopes

| Prefix | Scope | Persistence | Example |
|--------|-------|-------------|---------|
| (none) | Character | Permanent | `questStep` |
| `@` | Character | Temporary (session) | `@tempVar` |
| `$` | Global | Permanent | `$globalCount` |
| `$@` | Global | Temporary | `$@eventActive` |
| `.` | NPC | Permanent | `.shopOpen` |
| `.@` | NPC | Temporary (instance) | `.@localCalc` |
| `#` | Account | Permanent | `#kafraPoints` |
| `##` | Account (global) | Permanent | `##crossServer` |

Append `$` for strings: `@name$`, `.@text$`, etc.

#### Special Labels
```
OnInit:           // Runs when NPC loads (server start)
OnTouch:          // Player enters NPC trigger area
OnTouchNPC:       // Another NPC enters trigger area
OnPCLoginEvent:   // Player logs in
OnPCDieEvent:     // Player dies
OnNPCKillEvent:   // Player kills a monster
```

### 3.3 Multi-Page Dialogue Flow

A typical RO NPC conversation:
```
1. Player clicks NPC
2. mes "Welcome to Prontera!"     --> Page 1
3. mes "How can I help?"
4. next                            --> "Next" button appears
5. mes "Choose a service:"         --> Page 2
6. menu "Buy items",L_Buy,
        "Sell items",L_Sell,
        "Leave",L_Leave;          --> Choice menu appears
7. (player picks "Buy items")
8. goto L_Buy;
9. callshop "PrtToolShop",1;      --> Shop window opens
10. close;                         --> Dialogue closes
```

### 3.4 Input Box Usage

Rare but used in some quests:
```
mes "Enter the secret password:";
input .@password$;
if (.@password$ == "yggdrasil") {
    mes "Correct!";
} else {
    mes "Wrong password.";
}
close;
```

Also used for numeric input:
```
mes "How many potions do you want?";
input .@amount;
if (.@amount < 1 || .@amount > 100) {
    mes "Invalid amount.";
    close;
}
```

---

## 4. Quest System

### 4.1 Quest Categories

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

### 4.2 First Job Change Quests (Novice -> 1st Class)

**Universal Requirements:**
- Job Level 10 as Novice
- All 9 skill points allocated to Basic Skill

#### Swordsman (Izlude)
1. Talk to Master Swordsman in the Swordsman Guild
2. Talk to Test Guide in the left room to learn about the test
3. Talk to Test Hall Staff to enter the obstacle course
4. Complete a 3-room obstacle course (walking/jumping without falling)
5. Return to Master Swordsman to receive Swordsman class
- **Items Required:** None
- **Test Type:** Skill-based (obstacle course)

#### Mage (Geffen)
1. Talk to Expert Mage in the Magic Academy (geffen 63, 177)
2. Receive a solution number assignment
3. Read the bookshelf to learn required ingredients and mixing code
4. Collect items: varies by solution number (may include Milk from Prontera 76/133 at 25z, Payon Solution from Archer Village)
5. Mix the solution in the mixing machine (correct ingredients + catalyst + code)
6. Return the Unknown Solution to the Expert Mage
- **Items Required:** Varies by solution number (ingredients + Zeny)
- **Test Type:** Item collection + puzzle

#### Archer (Payon)
1. Talk to Archer Guildsman in Payon Archer Village
2. Hunt Willows in Payon fields to collect Trunks
3. Return with enough Trunks (need 25+ points)
- **Items Required:** Trunks (from Willows)
- **Test Type:** Monster hunting
- **Note:** Avoid Elder Willows (too strong at this level)

#### Thief (Morroc)
1. Travel to the Pyramids (NW of Morroc)
2. Enter the Thief Guild in the pyramid basement
3. Talk to the guide NPCs, answer questions
4. Find the "Irrelevant Man" outside the Pyramid
5. Go to the mushroom farm and collect mushrooms:
   - Orange Net Mushroom = 3 points each
   - Orange Gooey Mushroom = 1 point each
   - Need 25+ points total
6. Return to Comrade Brad in the Pyramid Dungeon
- **Items Required:** Mushrooms (farmed on-site)
- **Test Type:** Collection quest

#### Merchant (Alberta)
1. Talk to Chief Mahnsoo at the Merchant Guild (SW Alberta)
2. Pay a 1,000z registration fee
3. Receive a parcel delivery assignment
4. Deliver the parcel to the designated NPC
5. Receive a Voucher as receipt
6. Return the Voucher to the Merchant Guildsman
7. Chief Mahnsoo officially admits you to the guild
- **Items Required:** 1,000z fee
- **Test Type:** Delivery quest

#### Acolyte (Prontera)
1. Talk to Father Mareusis in Prontera Church
2. He sends you to find one of 3 Ascetics (randomly chosen):
   - Mother Marthilda
   - Father Rubalkabara
   - Father Yosuke
3. Travel to the assigned Ascetic's location (varies)
4. Complete their task (varies by Ascetic -- usually a pilgrimage or item fetch)
5. Return to Father Mareusis
- **Items Required:** Varies by Ascetic
- **Test Type:** Pilgrimage / item fetch

### 4.3 Second Job Change Quests (1st Class -> 2nd Class)

**Universal Requirements:**
- Job Level 40+ (can attempt quest)
- Job Level 50 (can skip some tests on certain servers)

Each 1st class can branch into one of two 2nd classes:

| 1st Class | 2nd Class Options |
|-----------|------------------|
| Swordsman | Knight / Crusader |
| Mage | Wizard / Sage |
| Archer | Hunter / Bard (M) or Dancer (F) |
| Thief | Assassin / Rogue |
| Merchant | Blacksmith / Alchemist |
| Acolyte | Priest / Monk |

#### Knight (Prontera Chivalry)
1. Speak to Chivalry Captain (prontera 48/343)
2. Talk to Sir Andrew -- item collection (skip at Job 50)
3. Talk to Sir Siracuse -- knowledge quiz about Knight class
4. Talk to Sir Windsor -- teleport to waiting room
5. Talk to Lady Amy -- another quiz
6. Talk to Sir Edmond -- teleported to a map with Porings, Lunatics, Chonchons. **Do NOT attack them.** Wait 5 minutes.
7. Talk to Sir Gray -- ethics/personality quiz (answer as friendly/altruistic person)
8. Return to Chivalry Captain for job change
- **Test Type:** Quiz + patience test

#### Wizard (Geffen Tower)
1. Talk to Catherine Medichi at the top of Geffen Tower
2. Take Raul Expagarus's quiz (10 questions, 10 points each, need 90+)
3. Three timed combat rooms -- defeat elemental monsters within 3 minutes each
4. Return to Catherine Medichi for job change
- **Test Type:** Quiz + combat trial

#### Priest (Prontera Church)
1. Visit three NPCs in order: Father Rubalkabara, Mother Marthilda, Father Yosuke
2. **Zombie Exorcism:** Kill all zombies in a room
3. **Temptation Resistance:** Walk past evil beings and say "Devil, Be Gone" to resist temptation
4. Return to the High Bishop for job change
- **Test Type:** Combat + willpower test

#### Hunter (Hugel)
1. Talk to Hunter Guildsman in Hugel
2. Complete a trap-setting test in a maze
3. Pass a monster identification quiz
4. Hunt specific monsters to prove hunting skill
5. Return for job change
- **Test Type:** Trap skill + quiz + combat

#### Assassin (Morroc area)
1. Travel to the Assassin Guild (2 maps south, 2 maps west of Morroc)
2. Pass a stealth infiltration test (navigate a map without being detected)
3. Collect specific poisons or items
4. Return for job change
- **Test Type:** Stealth + collection

#### Blacksmith (Einbroch)
1. Talk to Altiregen at the Blacksmith Guild in SE Einbroch
2. Collect specific ores and materials
3. Pass a forging test (craft specific items)
4. Answer quiz about metals and smithing
5. Return for job change
- **Test Type:** Collection + crafting + quiz

#### Crusader, Sage, Rogue, Monk, Alchemist, Bard/Dancer
Each follows a similar pattern of guild visit, multi-part testing (quiz, combat, collection), and return for job change. Specific details vary per class.

### 4.4 Rebirth / Transcendent Process

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
- At Job Level 10, change to High 1st Class (High Swordsman, High Mage, etc.)
- At Job Level 40-50, change to Transcendent 2nd Class (Lord Knight, High Wizard, etc.)
- Transcendent classes gain access to powerful new skills

### 4.5 Equipment / Headgear Quests

Headgear crafting is a major quest category. NPCs in various towns accept specific combinations of monster drops and materials to produce headgear items.

**Examples:**
- **Sunglasses:** Sunglasses Trader (Alberta) -- 1 One-Carat Diamond, 50 Feathers, 100,000z
- **Angel Wing:** Angel Wing Quest NPC -- various rare materials
- **Deviruchi Hat:** Specific monster drops + Zeny fee
- **Crown:** Royal crafting quest with rare materials

There are hundreds of headgear quests. iRO Wiki catalogs the complete list with required ingredients.

### 4.6 Access Quests (Dungeon Unlocks)

Some dungeons require quests to enter:

| Dungeon | Access Method |
|---------|--------------|
| Amatsu Dungeon | Talk to Lord of Palace (4th floor), cure his sick mother via Kitsune Mask shrine NPC |
| Turtle Island Dungeon | Alberta boat quest + entry fee |
| Thanatos Tower | Key item quest from nearby NPCs |
| Nameless Island | Multi-step quest chain from Rachel |
| Nifflheim | Quest or direct map walking |
| Glast Heim | No quest needed -- walk from Geffen (W, W, N, W) |
| Sphinx | No quest needed -- walk from Morroc (W, W) |
| Pyramids | No quest needed -- walk from Morroc (NW) |

### 4.7 Repeatable Quests

#### Bounty Board Quests
- **Location:** Near tool shops in select towns
- **Mechanic:** Kill 150 of a specific monster from nearby fields/dungeons
- **Reward:** EXP + (Monster Level x 100) Zeny
- **Bonus:** Every 4th turn-in grants 1 Eden Merit Badge

#### Repeatable EXP Quests
- **Mechanic:** Choose to hunt 50, 100, or 150 monsters
- **Scaling:** EXP per kill is the same regardless of chosen amount (linear scaling)
- **Cooldown:** Once per day (resets at midnight server time)

### 4.8 Quest Tracking

RO Classic has a basic quest log:
- **Quest Window** (Alt+U): Shows active quests, objectives, and progress
- **Quest Icons:** Yellow "!" above NPCs with available quests, grey "?" for in-progress
- **Map Markers:** Some quests add markers to the minimap
- **No automatic tracking:** Players must remember locations and requirements

---

## 5. Key Quest Lines (Detailed)

### 5.1 Complete First Job Change Flow

```
[Novice Lv10/Jlv10]
    |
    +-- Swordsman Guild (Izlude) -----> Swordsman
    +-- Magic Academy (Geffen) -------> Mage
    +-- Archer Village (Payon) -------> Archer
    +-- Thief Guild (Morroc Pyramid) -> Thief
    +-- Merchant Guild (Alberta) -----> Merchant
    +-- Church (Prontera) ------------> Acolyte
```

### 5.2 Complete Second Job Change Flow

```
[1st Class Jlv40-50]
    |
    +-- Swordsman --> Knight (Prontera Chivalry)
    |             --> Crusader (Prontera Chivalry)
    +-- Mage ------> Wizard (Geffen Tower)
    |             --> Sage (Juno Academy)
    +-- Archer ----> Hunter (Hugel)
    |             --> Bard/Dancer (Comodo)
    +-- Thief -----> Assassin (Morroc area)
    |             --> Rogue (Comodo)
    +-- Merchant --> Blacksmith (Einbroch)
    |             --> Alchemist (Al De Baran)
    +-- Acolyte ---> Priest (Prontera Church)
                 --> Monk (St. Capitolina Abbey)
```

### 5.3 Complete Rebirth Flow

```
[2nd Class Blv99/Jlv50]
    |
    v
  Juno Sage Area --> Pay 1,285,000z --> Heart of Ymir quest
    |
    v
  Valhalla --> Reborn as High Novice (1/1)
    |
    v
  [High Novice Jlv10]
    |
    +-- High Swordsman --> Lord Knight
    |                  --> Paladin
    +-- High Mage ------> High Wizard
    |                  --> Scholar (Professor)
    +-- High Archer ----> Sniper
    |                  --> Clown/Gypsy
    +-- High Thief -----> Assassin Cross
    |                  --> Stalker
    +-- High Merchant --> Whitesmith
    |                  --> Creator (Biochemist)
    +-- High Acolyte ---> High Priest
                       --> Champion
```

### 5.4 Key Dungeon Access Quests

**Amatsu Dungeon Entrance:**
1. Go to Amatsu castle, 4th floor
2. Talk to Lord of Palace -- learn his mother is sick
3. Head north to the shrine
4. Talk to Kitsune Mask NPC
5. Learn the ritual to banish the ghost
6. Complete the ritual
7. Access granted to Amatsu Dungeon

**Turtle Island:**
1. Alberta port -- find the Sailor NPC
2. Pay boat fare to Turtle Island
3. Complete introductory quest on the island
4. Dungeon entrance unlocked

**Thanatos Tower:**
1. Collect key items from surrounding areas
2. Solve puzzles at each tower level
3. Defeat guardians to progress
4. Access to upper floors unlocked

---

## 6. Kafra Services

### 6.1 Overview

Kafra Corporation is the primary service provider NPC network. Kafra employees are female NPCs in distinctive blue/white uniforms found in every major town.

### 6.2 Save Point

- **Cost:** Free
- **Function:** Sets respawn location to the Kafra's position (or nearby)
- **Usage:** Unlimited

### 6.3 Storage

- **Cost:** 40z per access (500z on some servers)
- **Capacity:** 600 unique item stacks (300 on some servers)
- **Scope:** Account-wide (all characters on the same account share one storage)
- **Features:**
  - Deposit and withdraw items
  - Items persist across sessions
  - Cannot deposit equipped items
  - Cannot deposit quest items (typically)

### 6.4 Teleport Service

Teleport destinations and costs vary by the Kafra's town location. Costs scale with distance.

**Representative Teleport Costs (from Prontera):**

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

**Notes:**
- Not all destinations are available from all Kafras
- Prontera serves as the central hub with the most destinations
- Some remote Kafras only offer teleport to Prontera
- Free Ticket for Kafra Transportation (item) waives the fee once

### 6.5 Cart Rental

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

### 6.6 Kafra Points

- Earn 1 Kafra Point per 10z spent on Kafra services
- Redeemable at the Kafra Headquarters in Al De Baran
- Lottery tiers: 1,000 / 3,000 / 5,000 / 7,000 / 10,000 points
- Rewards include potions, seeds, and utility items

### 6.7 Kafra Locations by Town

| Town | Kafra Position(s) |
|------|-------------------|
| Prontera | Central plaza, south gate |
| Geffen | Central area |
| Payon | Central village |
| Morroc | Near pyramid entrance, town center |
| Alberta | Near port, town center |
| Izlude | Near arena, central area |
| Al De Baran | Clock tower area, HQ |
| Einbroch | Factory district |
| Comodo | Beach area |
| Hugel | Town center |
| Juno | Central area |
| Rachel | Temple district |
| Lighthalzen | Corporate district |

---

## 7. Player Vending

### 7.1 Vending Skill (Merchant)

**Skill:** Vending (MC_VENDING)
**Type:** Active
**Max Level:** 10
**SP Cost:** 30
**Prerequisites:** Pushcart Lv3

**Items Per Level:**

| Level | Max Item Stacks |
|-------|----------------|
| 1 | 3 |
| 2 | 4 |
| 3 | 5 |
| 4 | 6 |
| 5 | 7 |
| 6 | 8 |
| 7 | 9 |
| 8 | 10 |
| 9 | 11 |
| 10 | 12 |

**Formula:** Max stacks = 2 + Skill Level

### 7.2 Setting Up a Vend Shop

1. Have Pushcart equipped (visible behind character)
2. Place items to sell in the Pushcart inventory (NOT main inventory)
3. Use the Vending skill
4. Set a shop title (displayed above character)
5. Set prices for each item (max 1,000,000,000z per item)
6. Confirm -- character becomes stationary with "shop" sign

**Restrictions:**
- Must be at least 4 cells from any NPC
- Cannot move while vending
- Cannot use skills while vending
- Only Pushcart items can be sold (not inventory items, except offline vending)

### 7.3 Commission Fee

- Items priced at 10,000,000z or above: **5% commission fee** deducted from sale
- Items under 10,000,000z: no commission
- Vendor receives 95% of listed price for expensive items

### 7.4 Buying Store

**Skill:** Open Buying Store
**Type:** Active
**Prerequisites:** Vending Lv1

**Setup:**
1. Go to Merchant Guild (Alberta 37/41)
2. Talk to "Purchasing Team Mr. Huh"
3. Pay 10,000z to learn Open Buying Store
4. Receive 5 Bulk Buyer Shop Licenses
5. Additional licenses: 200z each (up to 50 at once)

**For Non-Merchants:**
- Buy Black Market Buyer Shop License (500z each) from the Black Marketeer in Morroc's pub
- Single-use consumable items
- Maximum 2 item types per shop (vs 5 for Merchants)

**Restrictions:**
- Maximum 5 items advertised simultaneously (2 for non-merchants)
- Only Etc. items and non-brewed consumables can be purchased
- Must have at least 1 of the desired item already in inventory
- Must have enough Zeny to cover purchases

### 7.5 Vending Search

Players can search for vending shops through:
- **@whosell** command (find shops selling a specific item)
- **Shopping Board NPC** (location-based search)
- Walking through vend areas and reading shop titles

---

## 8. Implementation Plan

### 8.1 NPC Data Structure

```javascript
// server/src/ro_npc_data.js

const NPC_TYPES = {
  SHOP_TOOL: 'shop_tool',
  SHOP_WEAPON: 'shop_weapon',
  SHOP_ARMOR: 'shop_armor',
  KAFRA: 'kafra',
  JOB_CHANGE: 'job_change',
  SKILL_QUEST: 'skill_quest',
  REFINE: 'refine',
  STYLIST: 'stylist',
  GUIDE: 'guide',
  QUEST: 'quest',
  GUILD: 'guild',
  ARENA: 'arena',
  HEALER: 'healer',
  INN: 'inn',
  GENERIC: 'generic'
};

const NPC_REGISTRY = {
  // Example NPC definition
  'prt_tool_dealer': {
    npcId: 1001,
    type: NPC_TYPES.SHOP_TOOL,
    name: 'Tool Dealer',
    spriteId: 83,               // RO NPC sprite ID
    zone: 'prt_fild08',         // zone_name from ZONE_REGISTRY
    position: { x: 134, y: 221, z: 0 },
    facing: 4,                  // 0-7 direction
    interactRadius: 200,        // UE units for click interaction
    shopId: 'prt_tool',         // references SHOP_REGISTRY
    dialogueId: null,           // references DIALOGUE_REGISTRY (if any)
    questId: null               // references QUEST_REGISTRY (if any)
  },
  // ... hundreds of NPCs
};
```

### 8.2 Database Schema

```sql
-- NPC definitions (static, loaded at server start)
CREATE TABLE npcs (
  npc_id        SERIAL PRIMARY KEY,
  npc_key       VARCHAR(64) UNIQUE NOT NULL,  -- 'prt_tool_dealer'
  npc_type      VARCHAR(32) NOT NULL,          -- 'shop_tool', 'kafra', etc.
  name          VARCHAR(64) NOT NULL,
  sprite_id     INTEGER NOT NULL DEFAULT 0,
  zone_name     VARCHAR(64) NOT NULL,
  pos_x         FLOAT NOT NULL DEFAULT 0,
  pos_y         FLOAT NOT NULL DEFAULT 0,
  pos_z         FLOAT NOT NULL DEFAULT 0,
  facing        INTEGER NOT NULL DEFAULT 0,
  interact_radius FLOAT NOT NULL DEFAULT 200,
  shop_id       VARCHAR(64),                   -- FK to shops
  dialogue_id   VARCHAR(64),                   -- FK to dialogues
  quest_id      VARCHAR(64),                   -- FK to quests
  is_active     BOOLEAN NOT NULL DEFAULT TRUE,
  created_at    TIMESTAMP DEFAULT NOW()
);

-- Shop inventories
CREATE TABLE shops (
  shop_id       VARCHAR(64) NOT NULL,
  item_id       INTEGER NOT NULL,
  buy_price     INTEGER,                       -- NULL = cannot buy
  sell_price    INTEGER,                       -- auto-calculated if NULL
  stock         INTEGER DEFAULT -1,            -- -1 = unlimited
  sort_order    INTEGER DEFAULT 0,
  PRIMARY KEY (shop_id, item_id)
);

-- Quest definitions
CREATE TABLE quests (
  quest_id      VARCHAR(64) PRIMARY KEY,
  quest_type    VARCHAR(32) NOT NULL,          -- 'job_change', 'headgear', 'access', 'bounty', 'story'
  name          VARCHAR(128) NOT NULL,
  description   TEXT,
  min_base_level INTEGER DEFAULT 0,
  min_job_level  INTEGER DEFAULT 0,
  required_class VARCHAR(32),                  -- 'novice', 'swordsman', etc.
  is_repeatable  BOOLEAN DEFAULT FALSE,
  cooldown_hours INTEGER DEFAULT 0,            -- for repeatable quests
  rewards_json   JSONB,                        -- { exp: 1000, job_exp: 500, zeny: 5000, items: [{id:501, qty:10}] }
  steps_json     JSONB                         -- array of quest step definitions
);

-- Per-character quest progress
CREATE TABLE quest_progress (
  character_id  INTEGER NOT NULL REFERENCES characters(id),
  quest_id      VARCHAR(64) NOT NULL REFERENCES quests(quest_id),
  status        VARCHAR(16) NOT NULL DEFAULT 'active',  -- 'active', 'completed', 'failed', 'abandoned'
  current_step  INTEGER NOT NULL DEFAULT 0,
  progress_json JSONB DEFAULT '{}',            -- step-specific progress { monstersKilled: 45, itemsCollected: 3 }
  started_at    TIMESTAMP DEFAULT NOW(),
  completed_at  TIMESTAMP,
  last_completed_at TIMESTAMP,                 -- for repeatable quests
  PRIMARY KEY (character_id, quest_id)
);

-- Dialogue trees (optional -- can also be in-memory JSON)
CREATE TABLE dialogues (
  dialogue_id   VARCHAR(64) NOT NULL,
  page_index    INTEGER NOT NULL,
  speaker       VARCHAR(64),                   -- NPC name or NULL for narration
  text          TEXT NOT NULL,
  choices_json  JSONB,                         -- [{ text: "Buy", next_page: 5 }, { text: "Leave", action: "close" }]
  action        VARCHAR(32),                   -- 'next', 'close', 'open_shop', 'start_quest', 'input'
  action_params JSONB,                         -- { shopId: 'prt_tool' } or { questId: 'swordsman_change' }
  conditions_json JSONB,                       -- { minLevel: 10, hasItem: [501, 5] }
  PRIMARY KEY (dialogue_id, page_index)
);

-- Kafra points (per-character)
ALTER TABLE characters ADD COLUMN kafra_points INTEGER DEFAULT 0;
```

### 8.3 Dialogue State Machine

```
                    +---------------+
                    |  IDLE         |
                    | (no dialogue) |
                    +-------+-------+
                            |
                    player clicks NPC
                            |
                    +-------v-------+
                    |  PAGE_DISPLAY |<----+
                    | (showing text)|     |
                    +-------+-------+     |
                            |             |
              +-------------+--------+    |
              |                      |    |
        [Next button]          [Choice menu]
              |                      |    |
              |             +--------v--+ |
              |             | CHOICE    | |
              |             | (waiting) | |
              |             +--------+--+ |
              |                      |    |
              +-------->  next page  +----+
                                |
                         [Close / Action]
                                |
                    +-------v-------+
                    |  ACTION       |
                    | (shop/quest)  |
                    +-------+-------+
                            |
                    +-------v-------+
                    |  IDLE         |
                    +---------------+
```

**Server-side dialogue state** is stored per-socket in memory (not DB):

```javascript
// In player socket state
socket.dialogueState = {
  npcId: 'prt_tool_dealer',
  dialogueId: 'prt_tool_welcome',
  currentPage: 0,
  variables: {},           // Script variables for this dialogue session
  questContext: null        // Active quest context if in quest dialogue
};
```

### 8.4 Socket.io Events

```javascript
// Client -> Server
'npc:interact'        // { npcId }              -- Player clicks an NPC
'npc:next'            // {}                      -- Player clicks "Next" button
'npc:choice'          // { choiceIndex }         -- Player selects a menu option
'npc:input'           // { value }               -- Player submits input
'npc:close'           // {}                      -- Player closes dialogue
'shop:buy'            // { npcId, items: [{itemId, quantity}] }
'shop:sell'           // { npcId, items: [{itemId, quantity}] }
'quest:accept'        // { questId }
'quest:abandon'       // { questId }
'quest:turn_in'       // { questId }

// Server -> Client
'npc:dialogue'        // { npcId, speaker, text, hasNext, choices?, inputType? }
'npc:close'           // {}
'shop:open'           // { npcId, shopType, items: [{itemId, name, price, stock}] }
'shop:buy_result'     // { success, items, totalCost, newZeny }
'shop:sell_result'    // { success, items, totalEarned, newZeny }
'quest:started'       // { questId, name, steps, currentStep }
'quest:progress'      // { questId, currentStep, progress }
'quest:completed'     // { questId, rewards }
'quest:failed'        // { questId, reason }
'quest:log'           // { quests: [...] }  -- Full quest log
```

### 8.5 Node.js NPC Scripting Engine

Rather than a full scripting language, we use a JSON-driven dialogue/action tree:

```javascript
// server/src/npc_scripts/prt_tool_dealer.js
module.exports = {
  dialogueId: 'prt_tool_welcome',
  pages: [
    {
      speaker: 'Tool Dealer',
      text: 'Welcome to the Prontera Tool Shop!\nHow may I help you?',
      choices: [
        { text: 'Buy items', action: 'open_shop', params: { shopId: 'prt_tool', mode: 'buy' } },
        { text: 'Sell items', action: 'open_shop', params: { shopId: 'prt_tool', mode: 'sell' } },
        { text: 'Leave', action: 'close' }
      ]
    }
  ]
};

// server/src/npc_scripts/swordsman_job_change.js
module.exports = {
  dialogueId: 'swordsman_job_change',
  pages: [
    {
      speaker: 'Master Swordsman',
      text: 'You wish to become a Swordsman?\nFirst, you must prove your worth.',
      conditions: { minJobLevel: 10, currentClass: 'novice' },
      failText: 'You are not yet ready. Come back when you have reached Job Level 10.',
      next: 1
    },
    {
      speaker: 'Master Swordsman',
      text: 'Very well. Speak with the Test Guide\nin the next room to begin your trial.',
      action: 'start_quest',
      params: { questId: 'swordsman_change' }
    }
  ]
};
```

### 8.6 Shop Transaction Logic (Server)

```javascript
// Buy from NPC shop
function handleShopBuy(socket, player, { npcId, items }) {
  const npc = NPC_REGISTRY[npcId];
  if (!npc || !npc.shopId) return socket.emit('shop:buy_result', { success: false, error: 'Invalid shop' });

  const shop = SHOP_REGISTRY[npc.shopId];
  let totalCost = 0;

  // Calculate Discount
  const discountLevel = getSkillLevel(player, 'MC_DISCOUNT');
  const discountRate = discountLevel > 0
    ? Math.min(24, 5 + discountLevel * 2) / 100
    : 0;

  for (const { itemId, quantity } of items) {
    const shopItem = shop.find(s => s.itemId === itemId);
    if (!shopItem || !shopItem.buyPrice) continue;

    const price = Math.floor(shopItem.buyPrice * (1 - discountRate));
    totalCost += price * quantity;
  }

  if (player.zeny < totalCost) {
    return socket.emit('shop:buy_result', { success: false, error: 'Not enough Zeny' });
  }

  // Deduct zeny, add items to inventory
  player.zeny -= totalCost;
  for (const { itemId, quantity } of items) {
    addItemToInventory(player, itemId, quantity);
  }

  socket.emit('shop:buy_result', { success: true, totalCost, newZeny: player.zeny });
}

// Sell to NPC shop
function handleShopSell(socket, player, { npcId, items }) {
  let totalEarned = 0;

  // Calculate Overcharge
  const overchargeLevel = getSkillLevel(player, 'MC_OVERCHARGE');
  const overchargeRate = overchargeLevel > 0
    ? Math.min(24, 5 + overchargeLevel * 2) / 100
    : 0;

  for (const { itemId, quantity } of items) {
    const itemData = ITEM_DB[itemId];
    if (!itemData || !itemData.sellPrice) continue;

    const price = Math.floor(itemData.sellPrice * (1 + overchargeRate));
    totalEarned += price * quantity;

    removeItemFromInventory(player, itemId, quantity);
  }

  player.zeny += totalEarned;
  socket.emit('shop:sell_result', { success: true, totalEarned, newZeny: player.zeny });
}
```

### 8.7 UE5 NPC Actors

**Base NPC Actor: `BP_NPC_Base`**
- Inherits from `AActor`
- Implements `BPI_Interactable` (left-click interaction)
- Components:
  - `USkeletalMeshComponent` -- NPC model
  - `UCapsuleComponent` -- collision for click detection
  - `UWidgetComponent` -- floating name plate
  - `USphereComponent` -- interaction radius trigger

**NPC Subtypes:**
```
BP_NPC_Base
  +-- BP_NPC_ShopKeeper     (opens shop UI)
  +-- BP_NPC_Kafra           (opens Kafra service menu)   [EXISTING: KafraNPC.h]
  +-- BP_NPC_QuestGiver      (opens dialogue, tracks quest)
  +-- BP_NPC_JobChange       (opens job change dialogue)
  +-- BP_NPC_Refiner         (opens refine UI)
  +-- BP_NPC_Stylist         (opens styling UI)
  +-- BP_NPC_Guide           (opens info dialogue)
  +-- BP_NPC_Generic         (opens simple dialogue)
```

**Interaction Flow (C++):**
```cpp
// Player left-clicks NPC -> BPI_Interactable::OnInteract()
void ABP_NPC_Base::OnInteract(AActor* Interactor)
{
    // Get SocketManager from world
    // Emit 'npc:interact' with NPC ID
    // Server responds with 'npc:dialogue' or 'shop:open'
}
```

**NPC Subsystem: `UNPCSubsystem` (UWorldSubsystem)**
- Manages all NPC dialogue state on the client
- Listens to `npc:dialogue`, `npc:close`, `shop:open` events
- Owns the dialogue widget and shop widget
- Handles "Next", "Choice", and "Input" player actions

### 8.8 Quest Tracker UI

**Widget:** `SQuestTrackerWidget` (Slate, Z=13)
- Persistent HUD element showing active quests
- Shows quest name, current objective, and progress (e.g., "Kill 45/150 Porings")
- Expandable/collapsible per quest
- Click to open full quest log

**Quest Log Widget:** `SQuestLogWidget` (Slate, Z=22)
- Full-screen quest journal
- Tabs: Active / Completed / Failed
- Quest detail panel with steps, rewards, and NPC locations
- Toggle with Alt+U keybind

### 8.9 Shop UI

**Widget:** `SShopWidget` (Slate, Z=18)
- Two modes: Buy and Sell
- Buy mode: shows NPC inventory with prices (adjusted for Discount skill)
- Sell mode: shows player inventory with sell prices (adjusted for Overcharge skill)
- Quantity selector per item
- Running total display
- "Confirm Purchase" / "Confirm Sale" button
- Zeny balance display

### 8.10 Implementation Priority

| Phase | Feature | Effort |
|-------|---------|--------|
| 1 | NPC base actor + BPI_Interactable click | Small |
| 1 | Basic dialogue system (mes/next/close/menu) | Medium |
| 1 | Tool/Weapon/Armor shop buy/sell | Medium |
| 1 | Discount/Overcharge skill effects | Small |
| 2 | Kafra storage expansion (beyond current) | Medium |
| 2 | Kafra teleport with Zeny cost | Small |
| 2 | Quest progress tracking (DB + socket events) | Large |
| 2 | Quest log UI | Medium |
| 3 | Job change quest scripts (6 first jobs) | Large |
| 3 | Refine NPC + refinement system | Large |
| 3 | Headgear quest system | Medium |
| 4 | Second job change quests (12 classes) | Very Large |
| 4 | Player Vending (Merchant skill) | Large |
| 4 | Buying Store system | Medium |
| 4 | Stylist/Dye system | Small |
| 5 | Rebirth/Transcendent process | Large |
| 5 | Bounty Board repeatable quests | Medium |
| 5 | Guild system + WoE NPCs | Very Large |
| 5 | Arena NPCs | Medium |

---

## Sources

- [iRO Wiki -- Kafra Services](https://irowiki.org/wiki/Kafra)
- [iRO Wiki Classic -- Kafra](https://irowiki.org/classic/Kafra)
- [iRO Wiki -- Refinement System](https://irowiki.org/wiki/Refinement_System)
- [iRO Wiki Classic -- Refinement System](https://irowiki.org/classic/Refinement_System)
- [iRO Wiki -- Vending](https://irowiki.org/wiki/Vending)
- [iRO Wiki -- Buying Store](https://irowiki.org/wiki/Buying_Store)
- [iRO Wiki -- Discount](https://irowiki.org/wiki/Discount)
- [iRO Wiki -- Overcharge](https://irowiki.org/wiki/Overcharge)
- [iRO Wiki -- Commerce](https://irowiki.org/wiki/Commerce)
- [iRO Wiki -- Hair Styling](https://irowiki.org/wiki/Hair_Styling)
- [iRO Wiki -- Clothes Dye](https://irowiki.org/wiki/Clothes_Dye)
- [iRO Wiki -- Classes](https://irowiki.org/wiki/Classes)
- [iRO Wiki -- Guild System](https://irowiki.org/wiki/Guild_System)
- [iRO Wiki -- Headgear Quests](https://irowiki.org/wiki/Headgear_Quests)
- [iRO Wiki -- Access Quests](https://irowiki.org/wiki/Access_Quests)
- [iRO Wiki -- Bounty Board Quests](https://irowiki.org/wiki/Bounty_Board_Quests)
- [iRO Wiki -- Knight Job Change Guide](https://irowiki.org/wiki/Knight_Job_Change_Guide)
- [iRO Wiki -- Wizard Job Change Guide](https://irowiki.org/wiki/Wizard_Job_Change_Guide)
- [iRO Wiki -- Priest Job Change Guide](https://irowiki.org/wiki/Priest_Job_Change_Guide)
- [iRO Wiki -- Swordman Job Change Guide](https://irowiki.org/wiki/Swordman_Job_Change_Guide)
- [iRO Wiki -- Mage Job Change Guide](https://irowiki.org/wiki/Mage_Job_Change_Guide)
- [iRO Wiki -- Archer Job Change Guide](https://irowiki.org/wiki/Archer_Job_Change_Guide)
- [iRO Wiki -- Thief Job Change Guide](https://irowiki.org/wiki/Thief_Job_Change_Guide)
- [iRO Wiki -- Merchant Job Change Guide](https://irowiki.org/wiki/Merchant_Job_Change_Guide)
- [iRO Wiki -- Blacksmith Job Change Guide](https://irowiki.org/wiki/Blacksmith_Job_Change_Guide)
- [iRO Wiki -- War of Emperium](https://irowiki.org/wiki/War_of_Emperium)
- [iRO Wiki Database -- Prontera Tool Dealer](https://db.irowiki.org/db/shop-info/prt_tool/)
- [iRO Wiki Database -- Prontera Weapon Dealer](https://db.irowiki.org/db/shop-info/prt_weapon1/)
- [iRO Wiki Database -- Prontera Armor Dealer](https://db.irowiki.org/db/shop-info/prt_armor/)
- [rAthena Wiki -- Basic Scripting](https://github.com/rathena/rathena/wiki/Basic-Scripting)
- [rAthena -- Script Commands Reference](https://github.com/rathena/rathena/blob/master/doc/script_commands.txt)
- [rAthena -- NPC Shop Scripts](https://github.com/rathena/rathena/blob/master/npc/merchants/shops.txt)
- [RateMyServer -- Quest Database](https://ratemyserver.net/quest_db.php)
- [RateMyServer -- Job Change Quest Guides](https://ratemyserver.net/quest_db.php?type=60000)
- [RateMyServer -- Refinement Rates](https://ratemyserver.net/index.php?page=misc_table_refine)
- [RateMyServer -- NPC Shop Search](https://ratemyserver.net/index.php?page=nsw_shop_search)
- [Ragnarok Wiki -- Vending System](https://ragnarok.fandom.com/wiki/Vending_System)
- [Ragnarok Wiki -- Kafra Corporation](https://ragnarok.fandom.com/wiki/Kafra_Corporation)
