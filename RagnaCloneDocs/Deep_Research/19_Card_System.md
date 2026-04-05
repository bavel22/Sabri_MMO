# Card System -- Deep Research (Pre-Renewal)

> **Sources**: iRO Wiki Classic, rAthena pre-re source (card_db, item_combos.yml, item_bonus.txt), RateMyServer Pre-RE DB, Ragnarok Wiki (Fandom), GameFAQs card guides, WarpPortal forums, divine-pride.net
> **Scope**: Ragnarok Online Classic (pre-Renewal, Episodes 1-13.2). All mechanics verified against rAthena pre-re branch.
> **Date**: 2026-03-22

---

## Overview

The card system is Ragnarok Online's primary equipment customization mechanic. Players insert monster-dropped cards into slotted equipment to permanently grant bonuses ranging from simple stat boosts to game-changing combat modifiers. Every card is named after a monster (e.g. "Poring Card", "Hydra Card") and belongs to exactly one of seven equipment categories.

**Core principles:**
- Cards are obtained as monster drops at very low rates (typically 0.01%-0.02%, boss cards even rarer)
- Each card has a fixed equipment category -- it can only be compounded into matching equipment types
- Compounding is **permanent and irreversible** in pre-renewal -- once inserted, a card cannot be removed
- Cards are **consumed** on compound (removed from inventory)
- If equipment is **destroyed** (refine failure, Socket Enchant failure), all compounded cards are **lost forever**
- Card compounding itself **never fails** -- the action always succeeds if the slot is available and the card type matches
- Equipment can have 0-4 card slots depending on the specific item
- The display name of equipment changes to include card prefix/suffix names upon compounding

**Card count**: Approximately 400+ unique cards exist in pre-renewal RO (IDs 4001-4499), covering all seven equipment categories.

---

## Card Compounding Mechanics

### How to Compound

The compounding process in classic RO is straightforward:

1. **Have both the card and slotted equipment in your inventory** (not in storage or cart)
2. **Double-click the card** in the inventory window
3. A filtered list appears showing **only compatible equipment** with available empty slots
4. **Select the target equipment** from the list
5. A **confirmation dialog** warns that this is permanent and irreversible
6. On confirmation:
   - The card is consumed (deleted from inventory)
   - The card's item_id is stored in the equipment's card slot data
   - The equipment's display name updates to include the card's prefix or suffix
   - If the equipment is currently worn, stat bonuses are immediately recalculated

There is **no NPC required** for compounding in pre-renewal RO. Players compound cards themselves directly through the inventory UI. Some private servers add NPC-based compounders, but this is not official behavior.

### Success and Failure Rules

**Card compounding always succeeds.** There is no failure chance, no material cost, and no zeny cost for the compound action itself. The only requirements are:

1. The card must be in the player's inventory (not equipped, not in storage)
2. The target equipment must be in the player's inventory
3. The card's `equip_locations` must match the equipment's `equip_slot` (see Card Categories below)
4. The target equipment must have at least one empty card slot
5. The specific slot index targeted must be empty

**Risk of card loss** comes from two other systems, not compounding:
- **Equipment Refine Failure**: If a weapon or armor is destroyed during a failed refine attempt (above safe limit), all compounded cards are destroyed along with the equipment. This is why experienced players always refine equipment to their desired level **before** compounding cards.
- **Socket Enchant Failure**: If the Socket Enchant NPC fails to add a slot and the equipment is destroyed, all existing cards are lost.

### Card Slot Counts by Equipment Type

Equipment slots are determined per-item in the item database. Not all equipment of the same type has the same number of slots.

| Equipment Type | Possible Slots | Typical Range | Examples |
|---------------|---------------|---------------|----------|
| Weapons | 0-4 | Most have 0-3 | Blade [3], Main Gauche [3], Composite Bow [3], Pike [3], Knife [0] |
| Body Armor | 0-1 | 0-1 | Saint's Robe [1], Mink Coat [1], Formal Suit [0] |
| Shield | 0-1 | 0-1 | Guard [1], Buckler [1], Shield [0] |
| Garment | 0-1 | 0-1 | Muffler [1], Hood [1], Manteau [0] |
| Footgear | 0-1 | 0-1 | Shoes [1], Boots [1], Crystal Pumps [0] |
| Headgear (Upper) | 0-1 | 0-1 | Cap [1], Helm [1], Crown [0] |
| Headgear (Mid) | 0-1 | 0-1 | Sunglasses [1], Glasses [0] |
| Headgear (Lower) | 0 | Rarely has slots | Most lower headgear has 0 slots |
| Accessory | 0-1 | 0-1 | Clip [1], Rosary [1], Ring [0] |

**Key design principle**: More card slots generally means lower base stats. A Blade [3] (ATK 53, 3 slots) has lower ATK than a Katana [0] (ATK 60, 0 slots), but three card slots make it far more versatile for endgame builds. Players constantly evaluate the tradeoff between base stats and customization potential.

**Slot display**: Equipment with slots shows the slot count in brackets: `Guard [1]`, `Blade [3]`, `Main Gauche [3]`. The number always represents **total slots**, not remaining empty slots. A fully-carded `Blade [3]` still displays as `[3]`.

### Socket Enchant (Slot Addition)

Socket Enchant is an NPC service that can add one card slot to certain equipment that originally has zero slots. This is separate from card compounding.

**NPCs:**
- **Seiyablem** (Prontera) -- adds slots to weapons
- **Leablem** (Prontera) -- adds slots to armor/accessories
- **Troy** (Payon) -- safer option, requires Slotting Advertisement item (no destruction on failure)

**Requirements per attempt:**
- The specific equipment item (it must be on the eligible list)
- Required materials (varies by item tier -- ores, specific items)
- Zeny cost (varies by item tier)

**Item tier system (affects success rate):**

| Tier | Success Rate | Examples |
|------|-------------|----------|
| C (Common) | ~40-50% | Guard, Buckler, basic weapons |
| B (Better) | ~25-40% | Muffler, Boots, mid-tier weapons |
| A (Advanced) | ~15-25% | Manteau, better headgear |
| S (Special) | ~5-15% | Rare/high-end equipment |

**On failure**: The equipment AND all currently compounded cards are **destroyed**. Materials and zeny are non-refundable. This is why Socket Enchant is typically done on un-carded, un-refined equipment.

**Troy exception**: If using a Slotting Advertisement item with Troy, the equipment is NOT destroyed on failure -- you just lose the materials and zeny.

---

## Card Categories

Cards are divided into seven categories based on the `equip_locations` field in the item database. Each category can only be compounded into matching equipment types.

### Weapon Cards

Weapon cards (`equip_locations: Right_Hand`) are compounded into weapons. They are the most diverse category, with effects focused on increasing damage output.

**Race damage cards** (+20% damage vs specific race):

| Card | ID | Race | Prefix | rAthena Script |
|------|----|------|--------|----------------|
| Hydra Card | 4035 | Demi-Human | Bloody | `bonus2 bAddRace,RC_DemiHuman,20;` |
| Goblin Card | 4034 | Brute | Clamorous | `bonus2 bAddRace,RC_Brute,20;` |
| Caramel Card | 4033 | Insect | Insecticide | `bonus2 bAddRace,RC_Insect,20;` |
| Strouf Card | 4036 | Demon | Decussate | `bonus2 bAddRace,RC_Demon,20;` |
| Flora Card | 4038 | Fish | Fisher | `bonus2 bAddRace,RC_Fish,20;` |
| Scorpion Card | 4048 | Plant | Chemical | `bonus2 bAddRace,RC_Plant,20;` |
| Earth Petite Card | 4122 | Dragon | Dragoon | `bonus2 bAddRace,RC_Dragon,20;` |
| Orc Skeleton Card | 4037 | Undead | Damned | `bonus2 bAddRace,RC_Undead,20;` |
| Santa Poring Card | 4198 | Angel | -- | `bonus2 bAddRace,RC_Angel,20;` |
| Skeleton Card | 4062 | Formless | -- | `bonus2 bAddRace,RC_Formless,20;` |

**Size damage cards** (+15% damage + ATK bonus vs specific size):

| Card | ID | Size | Prefix | rAthena Script |
|------|----|------|--------|----------------|
| Desert Wolf Card | 4060 | Small | Gigantic | `bonus2 bAddSize,Size_Small,15; bonus bBaseAtk,5;` |
| Skeleton Worker Card | 4038 | Medium | Boned | `bonus2 bAddSize,Size_Medium,15; bonus bBaseAtk,5;` |
| Minorous Card | 4126 | Large | Titan | `bonus2 bAddSize,Size_Large,15; bonus bBaseAtk,5;` |

**Element damage cards** (+20% damage vs specific element):

| Card | ID | Element | Prefix/Suffix |
|------|----|---------|---------------|
| Vadon Card | 4044 | Fire | Flammable |
| Drainliar Card | 4043 | Water | Saharic |
| Mandragora Card | 4045 | Wind | Windy |
| Kaho Card | 4046 | Earth | Underneath |
| Anacondaq Card | 4042 | Poison | Envenom |
| Mao Guai Card | 4184 | Ghost | of Exorcism |

**Status infliction on attack cards** (chance to inflict status on each physical hit):

| Card | ID | Status | Chance | Prefix |
|------|----|--------|--------|--------|
| Marina Card | 4047 | Freeze | 5% | Ice |
| Magnolia Card | 4050 | Curse | 5% | Cursing |
| Zenorc Card | 4049 | Poison | 4% | Venomer's |
| Skeleton Card | 4062 | Stun | 2% | Keen |
| Savage Babe Card | 4039 | Stun | 5% | -- |
| Metaller Card | 4048 | Silence | 5% | Silence |
| Familiar Card | 4041 | Blind | 5% | Blink |
| Plankton Card | 4040 | Sleep | 5% | Drowsy |
| Requiem Card | 4056 | Confusion | 5% | Ginnungagap |

Status infliction from cards is checked per physical auto-attack. Boss monsters are **immune** to most card-inflicted status effects. Status chance is modified by the target's status resistance (VIT for stun, INT for blind/sleep/silence, LUK for curse/stone, etc.).

**Flat ATK weapon cards:**

| Card | ID | Effect | Prefix |
|------|----|--------|--------|
| Andre Card | 4013 | ATK +20 | Mighty |
| Drops Card | 4004 | DEX +1, HIT +3 | Lucky |

**Auto-spell on attack cards** (chance to auto-cast a skill when physically attacking):

| Card | ID | Effect | Trigger |
|------|----|--------|---------|
| Turtle General Card | 4305 | ATK +20%, 3% chance auto-cast Magnum Break Lv10 | On attack |
| Sniper Card (Cecil) | 4357 | 5% chance auto-cast Blitz Beat Lv1 | On attack |
| Stormy Knight Card | 4318 | 2% chance auto-cast Storm Gust Lv1 | On attack |
| Phreeoni Card | 4121 | HIT +100 | Passive |
| Lord of the Dead Card | 4276 | 1% chance to Coma (HP to 1), +50% vs Undead | On attack |

**HP/SP drain on attack cards:**

| Card | ID | Effect |
|------|----|--------|
| Sniper Card (Skel Archer) | 4104 | 1% chance to drain 5% of damage as HP |
| Incubus Card | 4192 | INT -3, MaxSP +150, SP Recovery +20% |
| Succubus Card | 4193 | INT -3, MaxHP +1000, HP Recovery +20% |
| Hunter Fly Card | 4098 | 3% chance to drain 15% of damage as HP |

**Kill hook cards** (effect triggers when killing a monster):

| Card | ID | Effect |
|------|----|--------|
| Orc Lady Card | 4072 | Gain 10 Zeny per kill |
| Race-specific SP drain | Various | +5 SP when killing monster of specified race (weapon must stay equipped or drains 5 SP) |

**Special flag weapon cards:**

| Card | ID | Effect | rAthena Bonus |
|------|----|--------|---------------|
| Drake Card | 4137 | Nullify size penalty (100% damage vs all sizes) | `bNoSizeFix` |
| Baphomet Card | 4147 | Splash damage (3x3 cells), HIT -10 | `bSplashRange,1` |
| Turtle General Card | 4305 | +20% damage to all enemies, 3% Magnum Break auto-cast | `bAddRace,RC_All,20` |

### Armor Cards

Armor cards (`equip_locations: Armor`) are compounded into body armor. Their primary function is elemental defense, stat boosts, and HP/SP bonuses.

**Elemental property cards** (change armor element, fundamentally altering damage taken):

| Card | ID | Element | Suffix | Key Effect |
|------|----|---------|--------|------------|
| Pasana Card | 4099 | Fire 1 | of Ifrit | Fire damage -25%, immune to Freeze |
| Swordfish Card | 4068 | Water 1 | Aqua | Water damage -25% |
| Dokebi Card | 4100 | Wind 1 | of Zephyrus | Wind damage -25% |
| Sandman Card | 4098 | Earth 1 | of Gnome | Earth damage -25% |
| Argiope Card | 4091 | Poison 1 | -- | Poison damage -25%, immune to Poison status |
| Bathory Card | 4104 | Dark 1 | Evil | Shadow damage -100% (immune), Holy +25% |
| Evil Druid Card | 4110 | Undead 1 | -- | Immune to Freeze/Stone, Holy +50%, Heal damages you |
| Angeling Card | 4054 | Holy 1 | Holy | Neutral damage immune, Shadow +25% |
| Ghostring Card | 4047 | Ghost 1 | -- | Neutral damage -25%, MaxHP -25% |

Elemental armor is one of the most impactful card mechanics. Wearing Fire armor (Pasana) makes you immune to Freeze status. Wearing Undead armor (Evil Druid) makes you immune to Freeze AND Stone but makes healing skills damage you. These tradeoffs are central to PvP builds.

**Only one armor element card can be active** -- if two armor-element cards were somehow compounded (normally impossible since armor only has 1 slot), only one element applies.

**Stat/HP/SP armor cards:**

| Card | ID | Effect |
|------|----|--------|
| Pupa Card | 4003 | MaxHP +700 |
| Pecopeco Card | 4031 | MaxHP +10% (percentage) |
| Roda Frog Card | 4012 | MaxHP +400, MaxSP +50 |
| Poring Card | 4001 | LUK +2, Perfect Dodge +1 |
| Fabre Card | 4002 | VIT +1, MaxHP +100 |
| Tao Gunka Card | 4302 | MaxHP +100%, DEF -50, MDEF -50 (MVP card) |
| Orc Lord Card | 4135 | Reflect 30% physical melee damage (MVP card) |
| Marc Card | 4065 | Freeze immunity (without element change) |
| Steel Chonchon Card | 4005 | DEF +2 |

### Headgear Cards

Headgear cards (`equip_locations: Head_Top`, `Head_Mid`, or `Head_Low`) can be compounded into **any** headgear piece regardless of which specific head position the card's `equip_locations` targets. A `Head_Top` card can go into mid or lower headgear.

**Status immunity cards:**

| Card | ID | Immunity | Additional |
|------|----|----------|------------|
| Orc Hero Card | 4143 | Stun | VIT +3 (MVP card) |
| Marduk Card | 4073 | Silence | -- |
| Nightmare Card | 4060 | Sleep | AGI +1 |
| Deviruchi Card | 4071 | Blind | STR +1 |
| Stalactic Golem Card | 4080 | -- | DEF +3, Stun Resist +20% |
| Mao Guai Card | 4184 | -- | Shadow element resistance +25% |
| Gibbet Card | 4085 | -- | Shadow element resistance +15% |

**Stat/utility headgear cards:**

| Card | ID | Effect |
|------|----|--------|
| Elder Willow Card | 4028 | INT +2 |
| Willow Card | 4009 | MaxSP +80 |
| Pharaoh Card | 4168 | SP consumption -30% (MVP card) |
| Mistress Card | 4132 | No gemstone requirement for skills, SP cost +25% (MVP card) |
| Carat Card | 4064 | INT +2, MaxSP +150 at +9 refine |
| Kathryne Keyron Card | 4176 | Cast time -cast%, DEX bonus at refine |

**Auto-cast headgear cards:**

| Card | ID | Effect |
|------|----|--------|
| Incantation Samurai Card | 4306 | Ignore DEF on critical hits, CRIT +10, SP -1% per second |

### Shield Cards

Shield cards (`equip_locations: Left_Hand`) are compounded into shields. They focus on damage reduction from specific sources.

**Racial damage reduction cards** (-30% damage from specific race):

| Card | ID | Race | Prefix/Suffix |
|------|----|------|---------------|
| Thara Frog Card | 4028 | Demi-Human | Cranial |
| Orc Warrior Card | 4066 | Brute | Brutal |
| Bigfoot Card | 4067 | Insect | of Gargantua |
| Penomena Card | 4095 | Formless | Fire-Proof |
| Khalitzburg Card | 4090 | Demon | from Hell |
| Teddy Bear Card | 4340 | Undead | of Requiem |
| Rafflesia Card | 4151 | Fish | Homer's |
| Sky Petite Card | 4178 | Dragon | of Dragoon |
| Anubis Card | 4263 | Angel | Satanic |
| Flora Card | 4149 | Plant | -- |

**Size damage reduction cards** (-25% damage from specific size):

| Card | ID | Size | Suffix |
|------|----|------|--------|
| Mysteltainn Card | -- | Small | Anti-Small |
| Ogretooth Card | -- | Medium | Anti-Medium |
| Executioner Card | -- | Large | Anti-Large |

**Special shield cards:**

| Card | ID | Effect |
|------|----|--------|
| Golden Thief Bug Card | 4128 | Immune to ALL magic (including heals/buffs), MDEF x2 (MVP card) |
| Maya Card | 4148 | Reflect single-target magic back at caster (MVP card) |
| Horn Card | 4074 | Long-range physical damage -35% |
| Alice Card | 4076 | Demi-Human damage -40% but +40% from Boss monsters |

### Garment Cards

Garment cards (`equip_locations: Garment`) are compounded into capes, mantles, and hoods. They primarily provide damage reduction and Flee.

**Elemental damage reduction cards** (-30% damage from specific element):

| Card | ID | Element | Suffix |
|------|----|---------|--------|
| Jakk Card | 4083 | Fire | Flameguard |
| Marse Card | 4094 | Water | Genie's |
| Dustiness Card | 4092 | Wind | -- |
| Hode Card | 4095 | Earth | -- |

**General garment cards:**

| Card | ID | Effect | Suffix |
|------|----|--------|--------|
| Raydric Card | 4133 | Neutral damage -20% | Immune |
| Deviling Card | 4174 | Neutral damage -50%, all other elements +50% | Deviant |
| Whisper Card | 4102 | Flee +20, Ghost element damage +50% | -- |
| Noxious Card | 4333 | Long-range damage -10%, Neutral damage -10% | -- |
| Nine Tail Card | 4147 | AGI +2, Flee +20 (at +9 refine) | De Luxe |

Raydric Card is the single most popular garment card in the game. Neutral is the most common damage element (all physical attacks without elemental endow/weapon are Neutral), so -20% Neutral reduction is universally useful.

Deviling Card is powerful but risky: -50% Neutral is huge, but +50% to all other elements means elemental attacks do 50% MORE damage. It is typically paired with elemental armor or resist gear.

### Footgear (Shoes) Cards

Footgear cards (`equip_locations: Shoes`) are compounded into shoes, boots, and sandals. They focus on HP/SP bonuses and defensive stats.

| Card | ID | Effect | Suffix |
|------|----|--------|--------|
| Verit Card | 4117 | MaxHP +8%, MaxSP +8% | of Counter |
| Matyr Card | 4074 | AGI +1, MaxHP +10% | -- |
| Sohee Card | 4070 | MaxSP +15%, SP Recovery +3% | -- |
| Green Ferus Card | 4191 | VIT +1, MaxHP +10% | -- |
| Firelock Soldier Card | 4246 | STR +2, MaxHP +10%, MaxSP +10% (at +9 refine) | Superior |
| Eddga Card | 4123 | No flinch (Endure), MaxHP -25% (MVP card) | -- |
| Moonlight Flower Card | 4131 | No SP cost for movement skills (MVP card) | -- |
| Zombie Slaughter Card | -- | +2% HP regen per kill | -- |

**EXP bonus footgear cards** (+10% EXP from specific race, but +20% damage taken from that race):

| Card | ID | Race |
|------|----|------|
| Mini Demon Card | -- | Brute |
| Skeleton General Card | -- | Insect |
| Others | -- | One per race |

### Accessory Cards

Accessory cards (`equip_locations: Both_Accessory`, `Right_Accessory`, or `Left_Accessory`) are compounded into accessories (clips, rosaries, rings, belts). Both accessory slots can hold cards.

**Stat bonus accessory cards:**

| Card | ID | Stat | Amount |
|------|----|------|--------|
| Mantis Card | 4175 | STR | +3 |
| Kukre Card | 4070 | AGI | +2 |
| Vitata Card | 4069 | VIT | +3 |
| Zerom Card | 4069 | DEX | +3 |
| Spore Card | 4067 | VIT | +2 |

**Skill-granting accessory cards** (grant usable skills regardless of class):

| Card | ID | Granted Skill | Level |
|------|----|---------------|-------|
| Smokie Card | 4044 | Hiding | Lv1 |
| Creamy Card | 4045 | Teleport | Lv1 |
| Horong Card | 4103 | Sight | Lv1 |
| Poporing Card | 4050 | Detoxify | Lv1 |

These cards add a usable skill to the player's skill bar. The skill functions identically to the class version at that level. Smokie Card's Hiding Lv1 works just like Thief's Hiding Lv1.

**Special accessory cards:**

| Card | ID | Effect |
|------|----|--------|
| Osiris Card | 4144 | Full HP/SP restore on resurrection (MVP card) |
| Marine Sphere Card | 4067 | 3% chance auto-cast Magnum Break Lv3 when hit |
| Owl Baron Card | 4187 | 3% chance auto-cast Lex Aeterna on attack |

---

## Card Effect Types

### Flat Stat Bonuses

The simplest card effect type. Cards directly add to base stats, which are then included in all derived stat calculations.

**Stats affected**: STR, AGI, VIT, INT, DEX, LUK, ATK, DEF, MATK, MDEF, MaxHP, MaxSP, HIT, Flee, Critical, Perfect Dodge.

**Implementation**: Stored in the items database as column values (`str_bonus`, `agi_bonus`, `atk`, `def`, etc.). Aggregated during `rebuildCardBonuses()` from all equipped cards.

**Stacking**: Flat bonuses from multiple cards are **additive**. Two cards with ATK +20 gives ATK +40 total.

### Percentage Modifiers

Cards that modify values as a percentage of the base. Applied multiplicatively with base stats.

| Modifier | rAthena Bonus | Example Card | Effect |
|----------|--------------|--------------|--------|
| MaxHP % | `bMaxHPrate` | Pecopeco (4031) | MaxHP +10% |
| MaxSP % | `bMaxSPrate` | -- | MaxSP +N% |
| ASPD % | `bAspdRate` | -- | Attack speed +N% |
| Cast time % | `bCastrate` | Kathryne Keyron | Cast time -N% |
| HP Recovery % | `bHPrecovRate` | Succubus (4193) | HP regen +20% |
| SP Recovery % | `bSPrecovRate` | Incubus (4192) | SP regen +20% |
| SP Cost % | `bUseSPrate` | Pharaoh (4168) | SP cost -30% |
| MATK % | `bMatkRate` | -- | Magic damage +N% |
| Heal power % | `bHealPower` | -- | Heal effectiveness +N% |
| Ranged ATK % | `bLongAtkRate` | -- | Ranged damage +N% |
| Critical damage % | `bCritAtkRate` | -- | Crit damage +N% |

**MaxHP/MaxSP rate**: Applied in `calculateDerivedStats()` after the class-aware base MaxHP/MaxSP formula. Multiple MaxHP rate cards are additive with each other: Pecopeco (+10%) + Tao Gunka (+100%) = +110% MaxHP. Ghostring's -25% MaxHP also stacks: +100% -25% = +75% net.

### Auto-Spell on Attack / When Hit

Cards can trigger automatic spell casts under two conditions:

**On Attack (`bAutoSpell`)**: Chance to auto-cast a skill each time the player lands a physical attack.

| Card | Skill Cast | Level | Chance |
|------|-----------|-------|--------|
| Turtle General Card | Magnum Break | 10 | 3% |
| Stormy Knight Card | Storm Gust | 1 | 2% |
| Cecil Damon Card | Blitz Beat | 1 | 5% |
| Lord of the Dead Card | Coma | -- | 1% |
| Owl Baron Card | Lex Aeterna | -- | 3% |

**When Hit (`bAutoSpellWhenHit`)**: Chance to auto-cast a skill each time the player takes physical or magical damage.

| Card | Skill Cast | Level | Chance |
|------|-----------|-------|--------|
| Marine Sphere Card | Magnum Break | 3 | 3% |
| Picky Card | -- | -- | -- |

**Auto-spell mechanics (critical for implementation):**
- Auto-cast skills have **no cast time** (instant)
- Auto-cast skills have **no SP cost** (free)
- Auto-cast skills have **no catalyst/gemstone cost**
- Auto-cast skills DO incur the **skill's after-cast delay** (ACD)
- Cast delay does NOT prevent bolt-type spells from being triggered again via auto-spell (they can fire rapidly)
- Auto-spell level is fixed by the card, not the player's learned skill level
- Auto-spells CAN trigger even if the player has not learned the skill
- The target of the auto-spell is the entity being attacked (for on-attack) or the attacker (for when-hit)
- Some auto-spells target self (buffs like Kyrie Eleison)

### Drain HP/SP on Attack

Cards that steal HP or SP from targets on each physical attack.

**Percentage drain** (`bHPDrainRate` / `bSPDrainRate`):

| Card | Drain Type | Chance | Amount |
|------|-----------|--------|--------|
| Hunter Fly Card | HP | 3% | 15% of damage dealt |
| Sniper Card (Skel) | HP | 1% | 5% of damage dealt |

**Flat drain** (`bHPDrainValue` / `bSPDrainValue`):

| Card | Drain Type | Amount |
|------|-----------|--------|
| Various | SP | Fixed amount per hit |

**Kill-triggered drain** (not technically drain, but HP/SP on kill):

| Card | Effect | Condition |
|------|--------|-----------|
| Race SP cards | +5 SP | When monster of specified race dies to physical attack |
| Generic SP gain | +N SP | Per kill |
| Generic HP gain | +N HP | Per kill |

**Important**: Drain effects only work on physical attacks, not magical. Drain is applied after damage calculation, using the final damage dealt as the base for percentage drain.

### Status Infliction on Attack / When Hit

**On Attack (`bAddEff`)**: Chance to inflict a status effect on the target per physical hit.

| Status | Example Cards | Typical Chance | Boss Immune? |
|--------|--------------|----------------|--------------|
| Stun | Skeleton Card, Savage Babe Card | 2-5% | Yes |
| Freeze | Marina Card | 5% | Yes |
| Poison | Zenorc Card | 4% | Yes |
| Curse | Magnolia Card | 5% | Yes |
| Blind | Familiar Card | 5% | Yes |
| Silence | Metaller Card | 5% | Yes |
| Sleep | Plankton Card | 5% | Yes |
| Confusion | Requiem Card | 5% | Yes |
| Bleeding | Various | 1-2% | Varies |
| Stone | Various | Low | Yes |

**When Hit (`bAddEffWhenHit`)**: Chance to inflict a status effect on the **attacker** when the player is hit.

| Status | Example Cards | Typical Chance |
|--------|--------------|----------------|
| Stun | Various | Varies |
| Curse | Various | Varies |
| Freeze | Various | Varies |

**On Self (`bAddEff2`)**: Chance to inflict a status effect on the **player themselves** when attacking. These are typically drawbacks paired with powerful effects (e.g. Muramasa's self-curse chance).

**Status chance calculation**: The card's listed chance is the base rate. The actual success depends on target resistance. VIT reduces Stun chance, INT reduces Blind/Sleep chance, LUK reduces Curse/Stone chance. Boss monsters have inherent immunity to most status effects from cards.

### Kill Hooks (Bonus EXP, Item Drop, Heal)

Effects that trigger when the player kills a monster.

**EXP bonus on kill** (`bExpAddRace`):

| Card | Race | EXP Bonus | Drawback |
|------|------|-----------|----------|
| Mini Demon Card | Brute | +10% | +20% damage from Brute |
| Skeleton General Card | Insect | +10% | +20% damage from Insect |
| (One per race) | Various | +10% | +20% damage from that race |

These cards are compounded into footgear. The EXP bonus only applies when the carded player deals the killing blow. The damage taken penalty applies at all times.

**Extra drop on kill** (`bAddMonsterDropItem` / `bAddMonsterDropItemGroup`):

| Card | Drop | Chance |
|------|------|--------|
| Ore Discovery (Blacksmith passive) | Rough Oridecon, Rough Elunium | Per kill |
| Various cards | Item groups (healing, misc) | Low chance |

**Zeny on kill** (`bGetZenyNum`):

| Card | Zeny | Condition |
|------|------|-----------|
| Orc Lady Card | 1-10 Zeny | Per kill |

### Autobonus (Temporary Buff)

Autobonus is a proc system where attacking or being attacked has a chance to grant a **temporary buff** that lasts for a set duration.

**rAthena syntax**:
```
autobonus "{ bonus script }",rate,duration{,flag};
autobonus2 "{ bonus script }",rate,duration{,flag};
autobonus3 "{ bonus script }",rate,duration,skill_id,skill_lv;
```

- `autobonus` -- triggers on attack
- `autobonus2` -- triggers when hit
- `autobonus3` -- triggers when using a specific skill
- `rate` -- chance out of 10000 (1000 = 10%)
- `duration` -- buff duration in milliseconds
- `flag` -- BF_WEAPON, BF_MAGIC, BF_MISC (what type of attack triggers it)

**Example**: A card with `autobonus "{ bonus bAspdRate,100; }",5,10000` grants 100% ASPD rate for 10 seconds with a 0.05% chance per attack.

**Key mechanics**:
- The temporary buff is re-applied fresh each time it procs (duration resets)
- Autobonus buffs ARE removed on stat recalculation events
- Visual effects can be attached via the optional "other script" parameter
- Autobonus and autobonus2 coexist -- a card can have both attack and defense procs

### Size/Race/Element Damage Modifiers

The most impactful card effects for combat. These provide percentage-based damage bonuses (offensive) or damage reductions (defensive) against specific target categories.

**Offensive modifiers** (applied in damage formula Step 6):

| Bonus | rAthena | Effect | Example |
|-------|---------|--------|---------|
| Race % | `bAddRace` | +N% damage vs race | Hydra: +20% vs Demi-Human |
| Element % | `bAddEle` | +N% damage vs element | Vadon: +20% vs Fire |
| Size % | `bAddSize` | +N% damage vs size | Minorous: +15% vs Large |
| Class % | `bAddClass` | +N% damage vs boss/normal | -- |
| Monster ID % | `bAddDamageClass` | +N% damage vs specific monster | -- |

**Defensive modifiers** (applied in damage formula Step 8c):

| Bonus | rAthena | Effect | Example |
|-------|---------|--------|---------|
| Race % | `bSubRace` | -N% damage from race | Thara Frog: -30% from Demi-Human |
| Element % | `bSubEle` | -N% damage from element | Raydric: -20% from Neutral |
| Size % | `bSubSize` | -N% damage from size | -- |
| Class % | `bSubClass` | -N% damage from boss/normal | -- |

**Stacking rules -- this is critical for correct implementation:**

**Within the same category** (e.g. two cards both giving race %): **ADDITIVE**
- 2x Hydra Card = +20% + +20% = +40% vs Demi-Human

**Between different categories** (race vs element vs size): **MULTIPLICATIVE**
- Hydra (+20% race) + Vadon (+20% element) vs Fire Demi-Human:
  - `cardfix = (1 + 0.20) * (1 + 0.20) = 1.44` = 44% total bonus
- NOT additive 40%

**Damage pipeline position**: Card modifiers are applied AFTER skill multiplier and buff multiplier, BEFORE element modifier. Defensive card modifiers are applied AFTER DEF reduction.

Full pipeline order:
```
1. Base ATK (StatusATK + WeaponATK, includes card flat ATK)
2. Size penalty
3. Variance
4. Buff ATK multiplier
5. Skill multiplier
6. OFFENSIVE CARD MODIFIERS (race %, then ele %, then size % -- multiplicative between categories)
7. Element modifier (target armor element -- may be changed by card)
8. Hard DEF, Soft DEF
8c. DEFENSIVE CARD MODIFIERS (race %, then ele %, then size % -- multiplicative between categories)
9. Final (min 1)
```

---

## Card Naming System (Prefix/Suffix)

### Overview

When cards are compounded into equipment, the equipment's **display name** changes to reflect the installed cards. Each card has a **pre-determined, fixed** designation as either a prefix or suffix -- this never varies by equipment type or slot position.

### Prefix vs Suffix

- **Prefix cards**: Text appears BEFORE the item name -- "Bloody", "Cranial", "Immune", "Mighty"
- **Suffix cards**: Text appears AFTER the item name, always starting with "of" -- "of Counter", "of Ghost", "of Hermes", "of Ifrit"
- Each card is exclusively one or the other, never both
- Approximately 327 cards have prefixes, 114 have suffixes (in the standard card naming table)

**Data source**: rAthena uses `cardprefixnametable.txt` and `cardpostfixnametable.txt` for this mapping.

### Display Name Format

```
[+RefineLevel] [Prefix1] [Prefix2...] BaseItemName [Suffix1] [Suffix2...] [TotalSlots]
```

Examples:
- `+7 Blade [3]` -- no cards, refine +7
- `Bloody Blade [3]` -- 1x Hydra Card (prefix "Bloody")
- `+7 Bloody Blade [3]` -- 1x Hydra + refine +7
- `Cranial Buckler [1]` -- 1x Thara Frog (prefix "Cranial")
- `Muffler of Immune [1]` -- 1x Raydric (prefix "Immune")
- `Shoes of Counter [1]` -- 1x Verit (suffix "of Counter")
- `Saint's Robe of Ifrit [1]` -- 1x Pasana (suffix "of Ifrit")

### Multiplier System for Duplicate Cards

When the same card is compounded multiple times (weapons with 2+ slots), a multiplier prefix is added:

| Count | Multiplier | Example |
|-------|-----------|---------|
| 1x | *(none)* | `Bloody Blade [4]` |
| 2x | Double | `Double Bloody Blade [4]` |
| 3x | Triple | `Triple Bloody Blade [4]` |
| 4x | Quadruple | `Quadruple Bloody Blade [4]` |

**Mixed card names**: Each unique card gets its own multiplier independently.
- 2x Hydra + 2x Skeleton Worker = `Double Bloody Double Boned Blade [4]`
- 1x Hydra + 1x Skeleton Worker = `Bloody Boned Blade [4]`
- 3x Hydra + 1x Vadon = `Triple Bloody Flammable Blade [4]`
- 1x Hydra (prefix) + 1x Verit (suffix) = `Bloody Blade of Counter [4]`

### Slot Count Display

The `[N]` in brackets always shows the **total number of slots** on the equipment, NOT remaining empty slots. A `Quadruple Bloody Blade [4]` has all 4 slots filled with Hydra cards.

---

## Card Combos (Set Bonuses)

### Overview

Card combos are special bonuses granted when multiple specific cards are equipped simultaneously in different equipment slots. The individual card effects still apply -- the combo bonus is an **additional** set bonus on top of normal card effects.

**Data source**: rAthena `db/pre-re/item_combos.yml`

### How Combos Work

- All cards in the combo must be **compounded into equipped equipment** simultaneously
- Cards can be in any valid equipment type for their category
- The combo bonus only applies while ALL required cards are equipped
- Equipping/unequipping any piece breaks or activates the combo
- Combo bonuses stack with individual card effects

### Notable Card Combos (Pre-Renewal)

**Merchant/Creator Set** (Holden + Raggler + Zipper Bear + Muka + Baby Leopard):
- STR +4
- MaxHP and MaxSP +7%
- +20% Mammonite damage
- Gain 1 SP per physical hit received

**Thief Set** (The Paper + Wanderer + Wild Rose + Shinobi + Dancing Dragon):
- AGI +5, STR +5
- ASPD +5%
- Movement Speed +5%
- Gain 1 SP per attack
- No gemstone requirement for Thief class skills

**Mage Set** (Bloody Butterfly + Loli Ruri + Parasite + Miyabi Doll + Evil Nymph + Harpy):
- DEF +5, MDEF +5, MaxHP +500
- Fire/Cold/Lightning Bolt damage +10%
- MATK +3%
- Cast time -15%

**Acolyte Set** (Geographer + Rideword + Enchanted Peach Tree + Cookie + Fur Seal):
- VIT +10
- Cast time -10%
- -30% damage from Demon and Undead
- +5% EXP from Demon and Undead (Acolyte class only)

**Monk Set** (Rideword + Cookie + Fur Seal + Waste Stove):
- ATK +25, STR +3, MaxSP +80
- 1% chance auto-cast Signum Crucis Lv5 on attack
- Asura Strike damage +10%
- SP consumption -10%, uninterruptible casting (Monk/Champion only)

**Injustice + Zherlthsh Combo:**
- ATK +20, LUK +3

**Dragon Tail + Merman + Anolian + Alligator + Cruiser Set:**
- DEX +5, AGI +5
- Arrow Shower damage +20%
- Double Strafe damage +20%
- Ranged damage +10%
- HIT +10

**Equipment Sets** (non-card combos that also appear in combo system):
- Morrigane's Set (Helm + Pendant + Belt + Manteau): CRIT +13, ATK +18, LUK +5, ASPD +5%
- Goibne's Set (Helm + Armor + Shoes + Spaulders): DEF +5, MDEF +5, VIT +5, MaxHP +15%
- Morpheus's Set (Hood + Bracelet + Ring + Shawl): INT +5, MaxSP +20%, no casting interruption

### Combo Activation

Card combos are checked during `rebuildCardBonuses()` or stat recalculation. The system iterates all known combo definitions and checks if the player has all required cards equipped. If yes, combo bonuses are added to the player's stats.

**Job-specific combos**: Some combo bonuses only apply to certain classes. The combo definition includes class checks (e.g. "Acolyte class only" for the Acolyte set EXP bonus).

---

## Card Removal

### Pre-Renewal Rules

In **strict pre-renewal** (original Episodes 1-13.2), card compounding is **permanent and irreversible**. There is no official mechanism to remove a card from equipment. This is by design -- it makes card placement decisions meaningful and creates demand for replacement equipment.

### Card Desocketing (Later Addition)

Card desocketing was introduced in later episodes and is present on many servers. It is technically post-renewal but commonly available on pre-renewal private servers:

**NPC**: Card desocketing NPC (varies by server)

**Cost**:
- Base: 200,000 Zeny
- +25,000 Zeny per card in the equipment
- 1x Yellow Gemstone + 1x Star Crumb per equipment piece

**Success rate**: ~90% (varies by server configuration)

**On success**: Cards are returned to inventory, equipment keeps its slots (now empty)

**On failure**: Nothing is destroyed -- equipment and cards remain unchanged. Only the zeny and materials are consumed.

**Note for implementation**: Whether to include card removal is a design decision. Strict pre-renewal does not have it. Including it significantly reduces the impact of card compounding decisions. Many classic-focused servers intentionally exclude it to maintain the original permanence mechanic.

---

## Notable Cards and Interactions

### MVP Cards (Game-Changing Effects)

MVP cards are the rarest drops in the game (0.01% or lower from boss monsters). They provide unique, powerful effects that fundamentally alter gameplay.

| Card | ID | Equip | Effect | Impact |
|------|----|-------|--------|--------|
| Baphomet Card | 4147 | Weapon | Splash damage 3x3, HIT -10 | Turns any melee class into AoE farmer |
| Drake Card | 4137 | Weapon | Nullify size penalty | 100% damage vs all sizes -- eliminates weapon weakness |
| Turtle General Card | 4305 | Weapon | +20% all damage, 3% Magnum Break Lv10 auto-cast | Best general-purpose weapon card |
| Lord of the Dead Card | 4276 | Weapon | 1% Coma (HP to 1), +50% vs Undead | PvP/WoE godly, kills any non-boss in one proc |
| Golden Thief Bug Card | 4128 | Shield | Immune to ALL magic (including heals) | Complete magic immunity but cannot be healed |
| Maya Card | 4148 | Shield | Reflect single-target magic | Counter-mage shield |
| Tao Gunka Card | 4302 | Armor | MaxHP +100%, DEF -50, MDEF -50 | Doubles HP pool, extremely fragile to piercing |
| Orc Lord Card | 4135 | Armor | Reflect 30% melee damage | Passive reflect, powerful in PvP |
| Angeling Card | 4054 | Armor | Holy armor (Neutral immune) | Immune to Neutral = immune to most physical |
| Ghostring Card | 4047 | Armor | Ghost armor, MaxHP -25% | Neutral damage -25%, but HP penalty |
| Pharaoh Card | 4168 | Headgear | SP cost -30% | Sustain for casters |
| Mistress Card | 4132 | Headgear | No gemstone cost, SP +25% | Free Sage/Priest/Wizard casting |
| Orc Hero Card | 4143 | Headgear | Stun immunity, VIT +3 | PvP essential |
| Eddga Card | 4123 | Footgear | No-flinch (Endure), MaxHP -25% | Walk without interruption |
| Moonlight Flower Card | 4131 | Footgear | No SP cost for movement skills | Free teleport/fly wing |
| Osiris Card | 4144 | Accessory | Full HP/SP on resurrect | Instant recovery from death |

### Critical Card Interactions

**GTB + Heal**: GTB Card makes the wearer immune to ALL magic. This includes beneficial magic like Heal, Blessing, Increase AGI, and Resurrection. A GTB user MUST rely on potions for healing.

**Evil Druid + Heal**: Undead armor means Heal deals DAMAGE to the wearer instead of healing. However, Undead armor grants immunity to Freeze and Stone Curse.

**Deviling + Raydric**: Deviling (-50% Neutral, +50% other elements) combined with Raydric (-20% Neutral) results in -70% Neutral reduction total but still +50% to other elements. The non-Neutral penalty remains dangerous.

**Angeling + Shadow attacks**: Holy armor (Angeling) is immune to Neutral attacks but takes +25% from Shadow element. Shadow-element weapons or endows counter Angeling users.

**Multiple race cards (PvP weapons)**: A 4-slot weapon with 2x Hydra + 2x Skeleton Worker = +40% vs Demi-Human + +30% vs Medium. Since most players are Demi-Human + Medium: `1.40 * 1.30 = 1.82x` = 82% total damage increase against other players. This is the classic "PvP weapon" build.

**Baphomet + Bowling Bash**: Splash from Baphomet Card stacks with AoE skills. Bowling Bash with Baphomet on a Knight can clear entire rooms.

**Drake + Dagger**: Daggers have 100%/75%/50% size penalty (Small/Medium/Large). Drake Card removes all size penalties, making daggers deal 100% to all sizes -- turning Assassins into viable Large-target killers.

---

## Implementation Checklist

### Currently Implemented (in Sabri_MMO)

Based on the existing `Card_System.md` and `ro_card_effects.js`:

- [x] Card compounding (`card:compound` socket event, server validation pipeline)
- [x] 538 cards at 100% coverage (IDs 4001-4499)
- [x] 65+ bonus types parsed from rAthena scripts
- [x] Flat stat bonuses (STR, AGI, VIT, INT, DEX, LUK, ATK, DEF, MATK, MDEF, MaxHP, MaxSP, HIT, Flee, CRIT, PD)
- [x] Offensive card modifiers (race/element/size % damage) in damage Step 6
- [x] Defensive card modifiers (race/element/size % reduction) in damage Step 8c
- [x] Armor element change (Ghostring, Pasana, etc.)
- [x] MaxHP/MaxSP rate percentage bonuses
- [x] Stacking rules: additive within category, multiplicative between categories
- [x] Card naming system (prefix/suffix with Double/Triple/Quadruple multipliers, 441 cards)
- [x] `rebuildCardBonuses()` on login, equip/unequip, compound
- [x] Card slot validation (`canCompoundCardOnEquipment()`)
- [x] Permanent compound (no removal)
- [x] Card consumed on compound
- [x] CARD_EFFECTS Map with full parser
- [x] Kill hooks (`processCardKillHooks()` -- SP gain, Zeny gain, EXP bonus)
- [x] HP/SP drain (`processCardDrainEffects()`)
- [x] Status procs on attack (`processCardStatusProcsOnAttack()`)
- [x] Status procs when hit (`processCardStatusProcsWhenHit()`)
- [x] Auto-spell on attack (`processCardAutoSpellOnAttack()`)
- [x] Auto-spell when hit (`processCardAutoSpellWhenHit()`)
- [x] Autobonus on attack (`processAutobonusOnAttack()`)
- [x] Autobonus when hit (`processAutobonusWhenHit()`)
- [x] Drop bonuses (`processCardDropBonuses()`)
- [x] Special flags: noSizeFix (Drake), splashRange (Baphomet), noMagicDamage (GTB), noCastCancel (Phen), noWalkDelay (Eddga), noGemStone (Mistress), noKnockback (RSX), intravision (Maya Purple), restartFullRecover (Osiris)
- [x] ASPD rate, Cast rate, MATK rate, Ranged ATK rate from cards
- [x] Critical damage rate from cards
- [x] HP/SP recovery rate from cards
- [x] Weapon/armor break chance from cards
- [x] Magic/melee reflect from cards
- [x] Class-based modifiers (boss/normal)
- [x] Sub-race modifiers (Goblin, Golem, Orc, Kobold)
- [x] Skill-specific ATK bonuses
- [x] Coma effect (Lord of the Dead)
- [x] Conditional card effects (refine-dependent, stat-dependent, class-dependent)
- [x] Compound UI (client-side `SCardCompoundPopup`)
- [x] Card prefix/suffix DB population (441 entries in `populate_card_naming.sql`)
- [x] `ro_card_prefix_suffix.js` server module (lookup data + utility functions)
- [x] Client `GetDisplayName()` with multiplier counting

### Not Yet Implemented / Deferred

- [ ] Card combo set bonuses (multiple specific cards granting additional bonus) -- requires `item_combos.yml` parsing
- [ ] Socket Enchant NPC (add slots to 0-slot equipment)
- [ ] Card removal NPC (optional -- not in strict pre-renewal)
- [ ] Skill-granting cards fully functional in hotbar (Smokie/Creamy/Horong/Poporing cards adding usable skills)
- [ ] Item group heal rate bonuses (`bAddItemGroupHealRate` affecting potion effectiveness per item group)
- [ ] Periodic effects (HP/SP loss/gain rate from cards like Incantation Samurai -1% SP/s)
- [ ] SP vanish rate (destroy target SP on hit)
- [ ] Class change (transform monster on hit)

---

## Gap Analysis

### Priority 1 -- High Impact, Gameplay-Affecting

| Feature | Status | Impact | Effort |
|---------|--------|--------|--------|
| Card combo set bonuses | Not implemented | Class-defining builds depend on combos (Mage set, Monk set, Thief set) | Medium -- parse `item_combos.yml`, check during stat rebuild |
| Skill-granting cards in hotbar | Partial | Smokie (Hiding), Creamy (Teleport), Horong (Sight), Poporing (Detoxify) are core utility | Medium -- needs skill hotbar integration |
| Socket Enchant NPC | Not implemented | Key equipment progression path (turn 0-slot gear into 1-slot) | Medium -- NPC + success rate + material costs |

### Priority 2 -- Important for Completeness

| Feature | Status | Impact | Effort |
|---------|--------|--------|--------|
| Periodic card effects | Not implemented | Incantation Samurai SP drain, HP/SP regen rate cards | Low -- tick-based system |
| SP vanish on hit | Not implemented | Rare card effect, niche PvP use | Low |
| Item group heal rate | Not implemented | Affects potion effectiveness per group | Low -- hook into heal calculation |

### Priority 3 -- Optional / Design Decision

| Feature | Status | Impact | Effort |
|---------|--------|--------|--------|
| Card removal NPC | Not implemented | Not in strict pre-renewal. Including reduces card impact. | Low -- NPC + zeny cost + success rate |
| Class change on hit | Not implemented | Very rare card effect, novelty | Low |

### Verified Correct in Current Implementation

The following critical mechanics have been verified as correctly implemented based on the existing codebase review:

- Card modifier stacking (additive within category, multiplicative between)
- Damage pipeline position (Step 6 offensive, Step 8c defensive)
- Armor element change (single element, armor-slot only)
- MaxHP/MaxSP rate applied after base calculation
- Card bonuses separate from equipment bonuses (no double-counting)
- rebuildCardBonuses triggered on all relevant events (login, equip, compound)
- All 13 card hook functions active in combat tick
- Conditional card evaluation at runtime (refine, stat, class checks)
- Card naming with multiplier system (Double/Triple/Quadruple)

---

## Sources

- [Card System - iRO Wiki Classic](https://irowiki.org/classic/Card_System)
- [Card Reference - iRO Wiki Classic](https://irowiki.org/classic/Card_Reference)
- [Card Sets - iRO Wiki Classic](https://irowiki.org/classic/Card_Sets)
- [Card Desocketing - iRO Wiki](https://irowiki.org/wiki/Card_Desocketing)
- [Autocast - iRO Wiki](https://irowiki.org/wiki/Autocast)
- [Socket Enchant - iRO Wiki Classic](https://irowiki.org/classic/Socket_Enchant)
- [Card Prefix/Suffix List - Revival-Ro Wiki](https://revivalro.fandom.com/wiki/Card_Prefix_/_Suffix_List)
- [RateMyServer Pre-RE Card Database](https://ratemyserver.net/index.php?page=link_card)
- [RateMyServer Item Combo Database](https://ratemyserver.net/index.php?page=item_combo)
- [RateMyServer Socket Enchant Guide](https://ratemyserver.net/socket_enchant.php)
- [rAthena item_bonus.txt](https://github.com/rathena/rathena/blob/master/doc/item_bonus.txt)
- [rAthena pre-re item_combos.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/item_combos.yml)
- [rAthena Autobonus Wiki](https://github.com/cydh/rathena-wiki/blob/master/Autobonus.md)
- [Skill Enabling Cards - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Skill_Enabling_Cards)
- [Attacking Special Status Chance Cards - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Attacking_Special_Status_Chance_Cards)
- [Job Specific Card Combinations - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Job_Specific_Card_Combinations)
- [Card Combo Guide - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/42099)
- [Ragnarok Online Card Combo Guide - Neoseeker](https://www.neoseeker.com/ragnarok-online/faqs/126634-card-combo.html)
