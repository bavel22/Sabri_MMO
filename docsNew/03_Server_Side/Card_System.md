# Card Compound & Bonus System

Complete documentation for the Sabri_MMO card system: compounding cards into equipment, stat bonus aggregation, offensive/defensive combat modifiers, armor element changes, and percentage-based HP/SP bonuses.

---

## Overview

The card system allows players to permanently insert monster cards into slotted equipment. Each card provides bonuses that range from simple stat boosts (STR +2) to powerful combat modifiers (+20% damage vs Demi-Human race). Cards are parsed from rAthena scripts at server startup and their effects are applied through the damage formula pipeline.

**Key characteristics:**
- **Server-authoritative** — all compound/bonus logic runs on the server
- **Permanent** — once compounded, cards cannot be removed
- **538 cards** at 100% coverage (IDs 4001-4499)
- **65+ bonus types** parsed from rAthena scripts into structured effects
- **13 game subsystems** hooked for card effects (damage, regen, casting, kills, drops, procs, etc.)
- Cards are consumed on compound (removed from inventory)

**Coverage breakdown:**
- 372 script-parsed by `CARD_EFFECTS` (complex combat/proc/flag effects)
- 148 DB-column-only (flat stat bonuses: STR, DEF, ATK, etc.)
- 18 no-script (stat-only cards)
- 22 conditional cards evaluated at runtime (refine/stat/class dependent)

**Files:**
- `server/src/ro_card_effects.js` — Parser (65+ bonus types, 600 lines)
- `server/src/ro_item_groups.js` — Item group definitions for heal/drop bonuses
- `server/src/index.js` — `rebuildCardBonuses()` (300 lines), 10+ hook functions, conditional engine, knockback, auto-cast, skill grants
- `server/src/ro_damage_formulas.js` — Physical Steps 2/5b/6c-6e/8/8d-8f, Magical GTB/MATK/race/MDEF
- `server/src/ro_status_effects.js` — `cardResEff` in `calculateResistance()`
- `database/migrations/add_equipment_break.sql` — Equipment breaking infrastructure

---

## Architecture

```
card:compound { cardInventoryId, equipInventoryId, slotIndex }
    |
    v
Server validates: card type, equipment slot match, empty slot, ownership
    |
    v
Insert cardId into compounded_cards JSONB → consume card from inventory
    |
    v
If equipment is currently equipped → rebuildCardBonuses(player, characterId)
    |
    +--> Flat stat bonuses    --> player.cardBonuses    --> getEffectiveStats()
    |                                                        --> derived stats
    |
    +--> Offensive mods       --> player.cardMods       --> getAttackerInfo()
    |    (race/ele/size %)                                   --> damage Step 6
    |
    +--> Defensive mods       --> player.cardDefMods    --> getPlayerTargetInfo()
    |    (race/ele/size %)                                   --> damage Step 8c
    |
    +--> Armor element        --> player.armorElement    --> getPlayerTargetInfo()
    |                                                        --> element table lookup
    |
    +--> HP/SP rate %         --> player.cardMaxHpRate   --> calculateDerivedStats()
                              --> player.cardMaxSpRate        --> maxHP/maxSP calc
```

---

## Key Files

| File | Purpose |
|------|---------|
| `server/src/ro_card_effects.js` | Card script parser. Parses rAthena `script` column into structured effects. Exports: `CARD_EFFECTS` Map, `buildCardEffects()`, `canCompoundCardOnEquipment()` |
| `server/src/index.js` | `rebuildCardBonuses()`, `card:compound` handler, `getEffectiveStats()` with cardBonuses, `getAttackerInfo()` with cardMods, `getPlayerTargetInfo()` with cardDefMods |
| `server/src/ro_damage_formulas.js` | `calculatePhysicalDamage()` Step 6 (offensive card mods) + Step 8c (defensive card mods). `calculateDerivedStats()` with bonusMaxHpRate/bonusMaxSpRate |
| `database/migrations/add_item_inspect_fields.sql` | `compounded_cards` JSONB column on `character_inventory` |
| `database/migrations/add_card_compound_support.sql` | Populates `card_type` column, creates index for equipped cards |

---

## Card Effect Parsing (`ro_card_effects.js`)

### Startup Flow

At server startup, after `loadItemDefinitions()` loads all 6,169 items from the DB:

```javascript
buildCardEffects(itemDefinitions);
// [CARDS] Parsed 538 card effects from scripts: N offensive, N defensive, N armor element
```

This iterates all items where `item_type === 'card'` and parses their `script` column using regex matching. Results are stored in the `CARD_EFFECTS` Map.

### Parsed Script Patterns

| rAthena Script Pattern | Effect Type | Key Format | Example Card |
|------------------------|-------------|------------|--------------|
| `bonus2 bAddRace,RC_X,N;` | Offensive race % | `combat.race_demihuman: 20` | Hydra (4035) |
| `bonus2 bAddSize,Size_X,N;` | Offensive size % | `combat.size_large: 15` | Minorous (4126) |
| `bonus2 bAddEle,Ele_X,N;` | Offensive element % | `combat.ele_fire: 20` | Vadon (4044) |
| `bonus2 bSubRace,RC_X,N;` | Defensive race % | `defense.race_demihuman: 30` | Thara Frog (4028) |
| `bonus2 bSubSize,Size_X,N;` | Defensive size % | `defense.size_large: 25` | — |
| `bonus2 bSubEle,Ele_X,N;` | Defensive element % | `defense.ele_neutral: 20` | Raydric (4133) |
| `bonus bDefEle,Ele_X;` | Armor element change | `armorElement: 'ghost'` | Ghostring (4047) |
| `bonus bMaxHPrate,N;` | MaxHP rate % | `maxHpRate: -25` | Ghostring (4047) |
| `bonus bMaxSPrate,N;` | MaxSP rate % | `maxSpRate: 10` | — |

### Flat Stat Bonuses (DB Columns)

Cards with simple stat bonuses already have these populated in their item DB columns by the generator script. These are read directly from `itemDefinitions.get(cardId)` during `rebuildCardBonuses()`:

| DB Column | Example Cards |
|-----------|---------------|
| `str_bonus` | Santa Poring Card |
| `agi_bonus` | Thief Bug Card |
| `vit_bonus` | Fabre Card (4002): VIT +1 |
| `int_bonus` | Elder Willow Card |
| `dex_bonus` | Zerom Card |
| `luk_bonus` | Poring Card (4001): LUK +2 |
| `atk` | Andre Card (4013): ATK +20, Minorous (4126): ATK +5 |
| `def` | Thara Frog Card (4028) — flat DEF |
| `max_hp_bonus` | Fabre (4002): +100, Pupa (4003): +700 |
| `max_sp_bonus` | Vitata Card |
| `hit_bonus` | — |
| `flee_bonus` | — |
| `critical_bonus` | Lunatic Card (4006): CRIT +1 |
| `perfect_dodge_bonus` | Poring Card (4001): PD +1, Lunatic Card (4006): PD +1 |

### Constant Mappings

**Race constants** (`RC_` prefix):

| rAthena | Internal Key |
|---------|-------------|
| `RC_Formless` | `formless` |
| `RC_Undead` | `undead` |
| `RC_Brute` | `brute` |
| `RC_Plant` | `plant` |
| `RC_Insect` | `insect` |
| `RC_Fish` | `fish` |
| `RC_Demon` | `demon` |
| `RC_DemiHuman` | `demihuman` |
| `RC_Player_Human` | `demihuman` |
| `RC_Angel` | `angel` |
| `RC_Dragon` | `dragon` |

**Element constants** (`Ele_` prefix):

| rAthena | Internal Key |
|---------|-------------|
| `Ele_Neutral` | `neutral` |
| `Ele_Water` | `water` |
| `Ele_Earth` | `earth` |
| `Ele_Fire` | `fire` |
| `Ele_Wind` | `wind` |
| `Ele_Poison` | `poison` |
| `Ele_Holy` | `holy` |
| `Ele_Dark` | `dark` |
| `Ele_Ghost` | `ghost` |
| `Ele_Undead` | `undead` |

**Size constants** (`Size_` prefix):

| rAthena | Internal Key |
|---------|-------------|
| `Size_Small` | `small` |
| `Size_Medium` | `medium` |
| `Size_Large` | `large` |

---

## Card Compounding (`card:compound`)

### Socket Event Flow

| Event | Direction | Payload |
|-------|-----------|---------|
| `card:compound` | Client -> Server | `{ cardInventoryId: int, equipInventoryId: int, slotIndex: int }` |
| `card:result` | Server -> Client | `{ success: bool, cardName?: string, equipmentName?: string, slotIndex?: int, message: string }` |
| `inventory:data` | Server -> Client | Full inventory refresh (after successful compound) |
| `player:stats` | Server -> Client | Updated stats (if equipment was equipped) |

### Server Validation Pipeline

```
1. Card exists in player inventory (character_id match)
2. Card item_type === 'card'
3. Card is NOT currently equipped
4. Equipment exists in player inventory
5. Equipment has equip_slot (is equippable)
6. Card equip_locations matches equipment equip_slot
   (via canCompoundCardOnEquipment())
7. Equipment has available slots (slots > 0)
8. Target slotIndex is valid (0 to slots-1)
9. Target slot is empty (compounded_cards[slotIndex] === null)
```

### Compound Slot Validation

Cards can only be compounded into matching equipment types:

| Card `equip_locations` | Valid Equipment `equip_slot` |
|------------------------|------------------------------|
| `Right_Hand` | `weapon` |
| `Left_Hand` | `shield` |
| `Armor` | `armor` |
| `Garment` | `garment` |
| `Shoes` | `footgear` |
| `Head_Top` | `head_top`, `head_mid`, `head_low` |
| `Head_Mid` | `head_top`, `head_mid`, `head_low` |
| `Head_Low` | `head_top`, `head_mid`, `head_low` |
| `Both_Accessory` | `accessory` (including `accessory_1`, `accessory_2`) |

> Note: Headgear cards (Head_Top/Mid/Low) can go into ANY headgear piece, regardless of which head position the card originally targets.

### On Successful Compound

1. Card item_id inserted into `compounded_cards` JSONB array at `slotIndex`
2. Card consumed from inventory (row deleted or quantity decremented)
3. If equipment is currently equipped:
   - `rebuildCardBonuses(player, characterId)` called
   - Derived stats recalculated
   - `player:stats` emitted with updated values
4. `card:result` emitted with success message
5. `inventory:data` emitted with refreshed inventory

### Permanence Rules

- **Cards cannot be removed** once compounded. This is permanent.
- **No un-compound operation** exists in RO Classic pre-renewal.
- If equipment is **destroyed by refine failure**, all compounded cards are **lost**.
- Card compound **consumes** the card item from inventory.

---

## Card Bonus Aggregation (`rebuildCardBonuses`)

### When Called

| Trigger | Location |
|---------|----------|
| Player login | `player:join` handler, after equipment bonuses loaded |
| Equip/Unequip | `inventory:equip` handler, after item stat changes |
| Card compound | `card:compound` handler, if target equipment is equipped |

### What It Does

1. **Resets** all card data: `player.cardBonuses`, `player.cardMods`, `player.cardDefMods`, `player.cardMaxHpRate`, `player.cardMaxSpRate`
2. **Queries** all equipped items with non-empty `compounded_cards` JSONB:
   ```sql
   SELECT ci.compounded_cards, i.equip_slot
   FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
   WHERE ci.character_id = $1 AND ci.is_equipped = true
   AND ci.compounded_cards IS NOT NULL AND ci.compounded_cards != '[]'::jsonb
   ```
3. **For each card** in each equipped item:
   - Adds flat stat bonuses from DB columns to `player.cardBonuses`
   - Adds offensive combat mods from `CARD_EFFECTS` to aggregated `combatMods`
   - Adds defensive mods from `CARD_EFFECTS` to aggregated `defenseMods`
   - If card has `armorElement` and is in armor slot, sets armor element
   - Accumulates percentage HP/SP bonuses
4. **Sets** player fields: `cardMods`, `cardDefMods`, `armorElement`, `cardMaxHpRate`, `cardMaxSpRate`

### Player Object Fields

```javascript
player.cardBonuses = {
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0, perfectDodge: 0,
    atk: 0, def: 0, matk: 0, mdef: 0
};

// Offensive card modifiers — applied in damage Step 6
player.cardMods = {
    race_demihuman: 40,  // 2x Hydra = +40% vs demihuman
    size_large: 15       // Minorous = +15% vs large
};  // or null if no offensive mods

// Defensive card modifiers — applied in damage Step 8c
player.cardDefMods = {
    race_demihuman: 30,  // Thara Frog = -30% from demihuman
    ele_neutral: 20      // Raydric = -20% from neutral element
};  // or null if no defensive mods

// Armor element from cards (only armor-slot cards)
player.armorElement = { type: 'ghost', level: 1 };  // Ghostring

// Percentage-based HP/SP bonuses
player.cardMaxHpRate = -25;  // Ghostring: -25% MaxHP
player.cardMaxSpRate = 0;
```

### Stacking Rules

**Within same category** (e.g. two Hydra Cards on a 4-slot weapon):

- **Additive**: 20% + 20% = 40% race bonus vs demihuman

**Between categories** (race x element x size):

- **Multiplicative**: Applied sequentially in the damage formula
- Formula: `cardfix = (1 + race/100) * (1 + ele/100) * (1 + size/100)`

**Example**: Hydra (+20% race) + Vadon (+20% element) on a weapon:
- vs Fire Demi-Human: `1.20 * 1.20 = 1.44x` (44% total bonus)

---

## Stat Integration (`getEffectiveStats`)

Card bonuses are merged alongside equipment bonuses, buff modifiers, and passive skill bonuses:

```javascript
function getEffectiveStats(player) {
    const bonuses = player.equipmentBonuses || {};
    const cardB = player.cardBonuses || {};
    const buffMods = getBuffStatModifiers(player);
    return {
        str: baseSTR + bonuses.str + cardB.str + buffMods.strBonus,
        agi: baseAGI + bonuses.agi + cardB.agi + buffMods.agiBonus,
        vit: baseVIT + bonuses.vit + cardB.vit + buffMods.vitBonus,
        int: baseINT + bonuses.int + cardB.int + buffMods.intBonus,
        dex: baseDEX + bonuses.dex + cardB.dex + buffMods.dexBonus + passive.bonusDEX,
        luk: baseLUK + bonuses.luk + cardB.luk + buffMods.lukBonus,
        weaponATK: player.stats.weaponATK + cardB.atk,
        bonusHit: bonuses.hit + cardB.hit + passive.bonusHIT + buffMods.bonusHit,
        bonusFlee: bonuses.flee + cardB.flee + passive.bonusFLEE + buffMods.bonusFlee,
        bonusCritical: bonuses.critical + cardB.critical + buffMods.bonusCritical,
        bonusPerfectDodge: bonuses.perfectDodge + cardB.perfectDodge,
        bonusMaxHp: bonuses.maxHp + cardB.maxHp + buffMods.bonusMaxHp,
        bonusMaxSp: bonuses.maxSp + cardB.maxSp + buffMods.bonusMaxSp,
        bonusMaxHpRate: player.cardMaxHpRate || 0,
        bonusMaxSpRate: player.cardMaxSpRate || 0,
        bonusHardDef: cardB.def || 0,
        bonusMATK: cardB.matk || 0,
        bonusHardMDEF: cardB.mdef || 0,
        // ... other fields unchanged
    };
}
```

### Aggregation Order

```
Final Stat = Base Stat + Equipment Bonus + Card Bonus + Buff Bonus + Passive Bonus
```

Card bonuses are **separate** from equipment bonuses to avoid double-counting. Both are independently tracked and merged in `getEffectiveStats()`.

---

## Damage Formula Integration

### Step 6: Offensive Card Modifiers (in `calculatePhysicalDamage`)

Applied after buff ATK multiplier and skill multiplier, before element modifier:

```javascript
// Step 6: Card modifiers (race%, element%, size%)
// Within each category: additive (2x Hydra = 40% race bonus)
// Between categories: multiplicative (race * ele * size)
if (attacker.cardMods) {
    const raceBonus = attacker.cardMods[`race_${targetRace}`] || 0;
    const eleBonus = attacker.cardMods[`ele_${targetElement}`] || 0;
    const sizeBonus = attacker.cardMods[`size_${targetSize}`] || 0;
    if (raceBonus !== 0) totalATK = Math.floor(totalATK * (100 + raceBonus) / 100);
    if (eleBonus !== 0) totalATK = Math.floor(totalATK * (100 + eleBonus) / 100);
    if (sizeBonus !== 0) totalATK = Math.floor(totalATK * (100 + sizeBonus) / 100);
}
```

**Flow**: `player.cardMods` -> `getAttackerInfo()` -> `attacker.cardMods` -> Step 6

### Step 8c: Defensive Card Modifiers

Applied after hard/soft DEF and passive race DEF, before final result:

```javascript
// Step 8c: Defensive card modifiers (Thara Frog, Raydric, etc.)
if (target.cardDefMods) {
    const raceRed = target.cardDefMods[`race_${attacker.race || 'formless'}`] || 0;
    const eleRed = target.cardDefMods[`ele_${atkElement}`] || 0;
    const sizeRed = target.cardDefMods[`size_${attacker.size || 'medium'}`] || 0;
    if (raceRed !== 0) totalATK = Math.floor(totalATK * (100 - raceRed) / 100);
    if (eleRed !== 0) totalATK = Math.floor(totalATK * (100 - eleRed) / 100);
    if (sizeRed !== 0) totalATK = Math.floor(totalATK * (100 - sizeRed) / 100);
}
```

**Flow**: `player.cardDefMods` -> `getPlayerTargetInfo()` -> `target.cardDefMods` -> Step 8c

### Armor Element Cards

Cards like Ghostring change the player's armor element. This changes how incoming attacks are modified by the element table:

**Flow**: `player.armorElement` -> `getPlayerTargetInfo()` -> `target.element` -> element table lookup in Step 7

| Card | Armor Element | Effect on Neutral Attacks |
|------|---------------|--------------------------|
| Ghostring | Ghost | 75% damage (25% reduction) |
| Pasana | Fire | 100% damage (no change) |
| Swordfish | Water | 100% damage |
| Angeling | Holy | 0% damage (immune!) |
| Evil Druid | Undead | 100% damage from neutral, immune to heal |

### Percentage HP/SP Bonuses

Applied in `calculateDerivedStats()` after class-aware base MaxHP/MaxSP calculation:

```javascript
// MaxHP — class-aware iterative formula (HP_JOB_A/B from HP_SP_COEFFICIENTS)
let maxHP = calculateMaxHP(level, vit, jobClass, isTranscendent, bonusMaxHp);
if (bonusMaxHpRate !== 0) maxHP = Math.floor(maxHP * (100 + bonusMaxHpRate) / 100);

// MaxSP — class-aware linear formula (SP_JOB from HP_SP_COEFFICIENTS)
let maxSP = calculateMaxSP(level, intStat, jobClass, isTranscendent, bonusMaxSp);
if (bonusMaxSpRate !== 0) maxSP = Math.floor(maxSP * (100 + bonusMaxSpRate) / 100);
```

### Full Damage Pipeline With Cards

```
1. Base StatusATK + WeaponATK (includes card ATK from cardBonuses.atk)
2. Size penalty
3. Variance
4. Buff ATK multiplier
5. Skill multiplier
6. *** Offensive card modifiers (race/ele/size %) ***
6b. Passive race ATK (Divine Protection)
7. Element modifier (uses target's armorElement — may be changed by card)
8. Hard DEF reduction
8b. Soft DEF reduction + passive race DEF
8c. *** Defensive card modifiers (race/ele/size reduction %) ***
9. Final result (min 1)
```

---

## Database Schema

### `character_inventory.compounded_cards`

```sql
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS compounded_cards JSONB DEFAULT '[]'::jsonb;
```

**Format**: JSONB array where index = slot position, value = card item_id or null

```json
[null, 4036, null, null]
```
- Slot 0: empty
- Slot 1: Hydra Card (4036)
- Slot 2: empty
- Slot 3: empty

### Card Items in `items` Table

| Column | Type | Purpose |
|--------|------|---------|
| `item_type` | VARCHAR(20) | Always `'card'` for card items |
| `equip_locations` | TEXT | Where card can be compounded (`Right_Hand`, `Armor`, etc.) |
| `card_type` | VARCHAR(20) | Category: `weapon`, `armor`, `shield`, `garment`, `footgear`, `headgear`, `accessory` |
| `card_prefix` | VARCHAR(50) | Display prefix (e.g. `Bloody` for Hydra) — shown BEFORE item name |
| `card_suffix` | VARCHAR(50) | Display suffix (e.g. `of Ghost` for Ghostring) — shown AFTER item name |
| `script` | TEXT | rAthena bonus script — parsed by `ro_card_effects.js` |
| `str_bonus`...`luk_bonus` | INTEGER | Flat stat bonuses |
| `atk`, `def`, `matk`, `mdef` | INTEGER | Flat combat stat bonuses |
| `max_hp_bonus`, `max_sp_bonus` | INTEGER | Flat MaxHP/MaxSP bonuses |

### Index for Performance

```sql
CREATE INDEX IF NOT EXISTS idx_ci_equipped_cards ON character_inventory (character_id)
    WHERE is_equipped = true AND compounded_cards IS NOT NULL AND compounded_cards != '[]'::jsonb;
```

---

## Notable Card Examples

### Offensive Weapon Cards

| Card | ID | Effect | rAthena Script |
|------|----|--------|----------------|
| Hydra | 4035 | +20% vs Demi-Human | `bonus2 bAddRace,RC_DemiHuman,20;` |
| Andre | 4013 | ATK +20 | (in `atk` column) |
| Minorous | 4126 | +15% vs Large, ATK +5 | `bonus2 bAddSize,Size_Large,15; bonus bBaseAtk,5;` |
| Desert Wolf | 4060 | +15% vs Small, ATK +5 | `bonus2 bAddSize,Size_Small,15;` |
| Vadon | 4044 | +20% vs Fire element | `bonus2 bAddEle,Ele_Fire,20;` |
| Drainliar | 4043 | +20% vs Water element | `bonus2 bAddEle,Ele_Water,20;` |
| Goblin | 4034 | +20% vs Brute race | `bonus2 bAddRace,RC_Brute,20;` |
| Orc Skeleton | 4037 | +20% vs Undead race | `bonus2 bAddRace,RC_Undead,20;` |
| Skeleton Worker | 4038 | +15% vs Medium, ATK +5 | `bonus2 bAddSize,Size_Medium,15;` |

### Defensive Cards

| Card | ID | Slot | Effect | rAthena Script |
|------|----|------|--------|----------------|
| Thara Frog | 4028 | Shield | -30% from Demi-Human | `bonus2 bSubRace,RC_DemiHuman,30;` |
| Raydric | 4133 | Garment | -20% Neutral damage | `bonus2 bSubEle,Ele_Neutral,20;` |
| Deviling | 4174 | Garment | -50% Neutral, +50% others | Complex multi-element |

### Armor Element Cards

| Card | ID | Element | MaxHP Penalty | rAthena Script |
|------|----|---------|---------------|----------------|
| Ghostring | 4047 | Ghost | -25% | `bonus bDefEle,Ele_Ghost; bonus bMaxHPrate,-25;` |
| Pasana | 4099 | Fire | None | `bonus bDefEle,Ele_Fire;` |
| Swordfish | 4068 | Water | None | `bonus bDefEle,Ele_Water;` |
| Dokebi | 4034 | Wind | None | `bonus bDefEle,Ele_Wind;` |
| Sandman | 4098 | Earth | None | `bonus bDefEle,Ele_Earth;` |
| Angeling | 4054 | Holy | None | `bonus bDefEle,Ele_Holy;` |
| Argiope | 4091 | Poison | None | `bonus bDefEle,Ele_Poison;` |
| Bathory | 4104 | Dark | None | `bonus bDefEle,Ele_Dark;` |
| Evil Druid | 4110 | Undead | None | `bonus bDefEle,Ele_Undead;` |

### Stat Bonus Cards

| Card | ID | Slot | Effect |
|------|----|------|--------|
| Poring | 4001 | Armor | LUK +2, Perfect Dodge +1 |
| Fabre | 4002 | Weapon | VIT +1, MaxHP +100 |
| Pupa | 4003 | Armor | MaxHP +700 |
| Lunatic | 4006 | Weapon | LUK +1, CRIT +1, PD +1 |
| Pecopeco | 4031 | Armor | MaxHP +10% (rate bonus) |
| Tao Gunka | 4302 | Armor | MaxHP +100%, DEF/MDEF -50 (MVP) |

---

## Card Slots Per Equipment Type

| Equipment Type | Max Slots | Typical Range |
|---------------|-----------|---------------|
| Weapons | 4 | 0-4 (most: 0-3) |
| Body Armor | 1 | 0-1 |
| Shield | 1 | 0-1 |
| Garment | 1 | 0-1 |
| Footgear | 1 | 0-1 |
| Headgear | 1 | 0-1 |
| Accessories | 1 | 0-1 |

Equipment with 0 slots cannot have cards compounded into it. The `slots` column in the `items` table determines the max slots.

---

## Card Naming System (Display Names)

### Overview

When cards are compounded into equipment, the item's **display name** changes to reflect the compounded cards. This is **100% client-side visual** — the server never modifies item names.

**Data source**: `database/migrations/populate_card_naming.sql` — 441 cards (327 prefix, 114 suffix), sourced from eAthena `cardprefixnametable.txt` + `cardpostfixnametable.txt`.

**Reference module**: `server/src/ro_card_prefix_suffix.js` — lookup data + utility functions.

### Name Format

```
[+RefineLevel] [Multiplier Prefix1] [Multiplier Prefix2...] BaseName [Multiplier Suffix1...] [TotalSlots]
```

### Prefix vs Suffix

Each card has a **fixed** designation as prefix or suffix (per-card, NOT per-equipment-type):
- **Prefix cards** (`card_prefix` column): text goes BEFORE item name — "Bloody", "Cranial", "Immune"
- **Suffix cards** (`card_suffix` column): text goes AFTER item name — "of Counter", "of Hermes", "of Ghost"
- Determined by `card_prefix IS NOT NULL` (prefix) vs `card_suffix IS NOT NULL` (suffix) — mutually exclusive

### Multiplier System for Duplicate Cards

| Count | Multiplier | Example |
|-------|-----------|---------|
| 1× | *(none)* | `Bloody Blade [4]` |
| 2× | Double | `Double Bloody Blade [4]` |
| 3× | Triple | `Triple Bloody Blade [4]` |
| 4× | Quadruple | `Quadruple Bloody Blade [4]` |

Mixed cards each get their own multiplier: `Double Bloody Double Boned Blade [4]` (2× Hydra + 2× Skel Worker)

### Slot Count Display

`[N]` always shows **total slots**, NOT remaining empty. `Quadruple Bloody Blade [4]` = all 4 slots filled.

### Examples

| Compound | Display Name |
|----------|-------------|
| +7 Blade, 3× Hydra + 1× Skel Worker | `+7 Triple Bloody Boned Blade [4]` |
| Buckler + 1× Thara Frog | `Cranial Buckler [1]` |
| Muffler + 1× Raydric | `Immune Muffler [1]` |
| Shoes + 1× Verit (suffix) | `Shoes of Counter [1]` |
| +10 Bow, 2× Hydra + 2× Skel Worker | `+10 Double Bloody Double Boned Composite Bow [4]` |
| Blade + 1× Hydra + 1× Verit | `Bloody Blade of Counter [4]` |

### Implementation

- **DB**: `card_prefix`/`card_suffix` columns on `items` table (populated by migration)
- **Server**: `getPlayerInventory()` reads columns, sends in `compounded_card_details` payload
- **Client**: `FInventoryItem::GetDisplayName()` in `CharacterData.h` — counts unique cards, applies multipliers, assembles prefix/suffix in insertion order
- **All UI widgets** call `GetDisplayName()` — inventory, equipment, tooltip, inspect, shop sell, hotbar

### Key Files

| File | Purpose |
|------|---------|
| `database/migrations/populate_card_naming.sql` | 441 UPDATE statements for card prefix/suffix |
| `server/src/ro_card_prefix_suffix.js` | `CARD_NAMING_TEXT` (441 entries), `POSTFIX_CARD_IDS` (114 suffix cards) |
| `client/.../CharacterData.h` | `GetDisplayName()` — multiplier counting algorithm |

---

## Not Yet Implemented (Deferred)

| Feature | Status | Notes |
|---------|--------|-------|
| Status effect on-hit cards (Marina = freeze) | Deferred | Needs proc-on-hit system |
| Skill-granting cards (Poporing = Detoxify) | Deferred | Needs script engine |
| Drake Card (nullify size penalty) | Deferred | Needs special flag in damage calc |
| GTB (magic immunity) | Deferred | Needs magic damage path |
| Auto-cast cards (Turtle General = Magnum Break) | Deferred | Needs proc system |
| Card removal NPC service | Deferred | Not in pre-renewal Classic |
| Card set bonuses | Deferred | Phase 12+ |
| Boss/Normal card category | Deferred | Phase 12+ |
| ~~Card prefix/suffix display~~ | **COMPLETE** | 441 cards populated, `GetDisplayName()` with Double/Triple/Quadruple multipliers |

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Card bonuses not applying | `rebuildCardBonuses()` not called | Ensure called in equip handler and player:join |
| Damage mods not working | `player.cardMods` is null | Check `CARD_EFFECTS.get(cardId)` — script may not parse |
| Card compound fails | Card type mismatch | Check `canCompoundCardOnEquipment(equip_locations, equip_slot)` |
| No card slots | Equipment `slots = 0` | Check item's `slots` column in DB |
| Armor element not changing | Card not on armor slot | Only armor-slot cards change element |
| Double-counting stats | Overlap between equipmentBonuses and cardBonuses | Verify separate objects |
| MaxHP rate not applied | Missing `bonusMaxHpRate` in derived calc | Check `calculateDerivedStats()` destructuring |
| New card not parsed | Unsupported script pattern | Add regex to `parseCardScript()` |
| Server crash on compound | Missing column | Run migration `add_card_compound_support.sql` |

---

## Related Documentation

- [Inventory System](./Inventory_System.md) — Item types, equip handler, inventory events
- [Combat System](./Combat_System.md) — Full damage pipeline with card modifier steps
- [Event Reference](../06_Reference/Event_Reference.md) — Socket.io event payloads
- [Database Architecture](../01_Architecture/Database_Architecture.md) — Schema details

**Last Updated**: 2026-03-13 — Added Card Naming System (prefix/suffix display, multiplier logic)
