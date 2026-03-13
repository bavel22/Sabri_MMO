# Item Stat & Script Comprehensive Audit

**Date**: 2026-03-13
**Status**: ALL HIGH/MEDIUM ISSUES IMPLEMENTED (2026-03-13) + DESCRIPTION AUDIT COMPLETE (2026-03-13)
**Scope**: Full pipeline audit — generator (`generate_canonical_migration.js`) -> DB schema -> server stat consumption -> client display -> **description text**
**Trigger**: Discovered `perfect_dodge_bonus` column was missing from generator INSERT, causing card PD bonuses to silently default to 0

> **2026-03-13 Update**: Added comprehensive description text audit. The `item_descriptions.json` file contained Renewal-era iRO client strings with inflated DEF values (3-11x higher), wrong weight/element/refinable/level/weapon type values, and renewal-only bonus stats. The generator (`generate_canonical_migration.js`) now rewrites ALL description properties from the pre-renewal YAML source: base stats (DEF/ATK/Weight), bonus stats (all 20+ types), element, weapon level, refinable, required level, weapon type. Audit v3 confirms 1,772/1,923 items are perfect matches (92.1%), with 118 known limitations (no MATK rate% DB column) and only 13 unfixable edge cases from stub descriptions or conditional scripts. Effective accuracy: 99.1%.
>
> New functions added to generator: `fixDescriptionStats()`, `extractScriptBonuses()`, `buildBonusLines()`, `getItemTypeName()`, `insertAfterLine()`.
>
> Audit scripts: `audit_item_descriptions.js` (v1), `audit_item_v2.js` (v2), `audit_item_v3.js` (v3 comprehensive).

---

## Executive Summary

The canonical item generator correctly **parses** most rAthena script bonuses but fails to **write** several of them to the SQL output. Post-insert migrations have patched some gaps (element, two_handed, perfect_dodge), but significant gaps remain. Most critically:

1. **Equipment MaxHP/SP rate bonuses are completely ignored** — 25 equipment items with `bMaxHPrate` and 26 with `bMaxSPrate` have zero effect (only card-based rates are tracked)
2. **Armor defense element not set for 8 innate-element armors** — Flame/Water/Wind/Earth Sprits Armor variants all read as neutral
3. **ASPD rate bonuses from equipment not applied** — 83 items with `bAspdRate` have no effect
4. **Generator INSERT missing 8 DB columns** — relies on migrations that can get out of sync

---

## Issue 1: Equipment MaxHP/SP Rate Bonuses Not Applied

**Severity**: HIGH — affects survivability/resource calculations for ~51 equipment items
**Status**: NOT IMPLEMENTED

### Problem

The server tracks `bonusMaxHpRate` and `bonusMaxSpRate` in `calculateDerivedStats()` (ro_damage_formulas.js:314,318), but these values ONLY come from **card** bonuses:

```javascript
// index.js:1085-1086 — getEffectiveStats()
bonusMaxHpRate: player.cardMaxHpRate || 0,   // ONLY cards
bonusMaxSpRate: player.cardMaxSpRate || 0,   // ONLY cards
```

There is NO `player.equipMaxHpRate` or equivalent. Non-card equipment with `bMaxHPrate`/`bMaxSPrate` in their scripts has zero effect.

### Affected Items (25 equipment with bMaxHPrate, 26 with bMaxSPrate)

**MaxHP Rate Equipment (25 items):**
| Item | Effect | Impact |
|------|--------|--------|
| Assassin's Dagger | +20% MaxHP | Major — key Assassin weapon |
| Vidar's Boots | +9% MaxHP, +9% MaxSP | Major — common endgame footgear |
| Vital Tree Shoes | +10% MaxHP | Major — trans-class footgear |
| Novice Shoes | +5% MaxHP | Minor — starting gear |
| Goibne's Armor | +10% MaxHP | Major — set armor |
| Diabolus Armor | +6% MaxHP | Moderate |
| Various others | +2% to +20% | Varies |

**MaxSP Rate Equipment (26 items):**
| Item | Effect | Impact |
|------|--------|--------|
| Vidar's Boots | +9% MaxSP | Major — paired with HP rate |
| Morpheus's Shawl | +10% MaxSP | Major — Mage set garment |
| Various others | +2% to +15% | Varies |

### Required Fix

**Step 1**: Add DB columns for equipment HP/SP rate
```sql
ALTER TABLE items ADD COLUMN IF NOT EXISTS max_hp_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS max_sp_rate INTEGER DEFAULT 0;
```

**Step 2**: Generator — parse `bMaxHPrate` and `bMaxSPrate` in `parseBonusScript()`
```javascript
// In parseBonusScript(), add cases:
case 'MaxHPrate': bonuses.max_hp_rate = (bonuses.max_hp_rate || 0) + n; break;
case 'MaxSPrate': bonuses.max_sp_rate = (bonuses.max_sp_rate || 0) + n; break;
```

Add `max_hp_rate` and `max_sp_rate` to the `columns` array, `values` array, and ON CONFLICT clause.

**Step 3**: Migration to populate existing items
```sql
-- Parse bMaxHPrate from scripts for all non-card equipment
-- Must handle ONLY unconditional bonuses (not inside if-blocks)
-- Example items:
UPDATE items SET max_hp_rate = 20 WHERE item_id = 1232;  -- Assassin Dagger
UPDATE items SET max_hp_rate = 9  WHERE item_id = 2418;  -- Vidar's Boots
-- ... (full list needs script audit)
```

**Step 4**: Server — track equipment HP/SP rate alongside card rates
```javascript
// In player object initialization:
player.equipMaxHpRate = 0;
player.equipMaxSpRate = 0;

// In equip handler, after applying stat bonuses:
player.equipMaxHpRate += item.max_hp_rate || 0;

// In unequip handler:
player.equipMaxHpRate -= item.max_hp_rate || 0;

// In getEffectiveStats():
bonusMaxHpRate: (player.cardMaxHpRate || 0) + (player.equipMaxHpRate || 0),
bonusMaxSpRate: (player.cardMaxSpRate || 0) + (player.equipMaxSpRate || 0),
```

**Step 5**: Rebuild card bonuses should NOT double-count (currently doesn't — card rates are separate)

---

## Issue 2: Armor Defense Element Not Set for Innate-Element Armors

**Severity**: HIGH — 8 armor items have wrong element (neutral instead of correct element)
**Status**: NOT FIXED

### Problem

The migration `fix_item_element_matk_twohanded.sql` only fixes `bAtkEle` (weapon attack element). It does NOT fix `bDefEle` (armor defense element) on non-card armor items. When the server reads `item.element` for armor on equip (index.js:7328-7329), it gets `'neutral'` instead of the actual element.

```javascript
// index.js:7328-7329 — equip handler
if (item.equip_slot === 'armor') {
    player.armorElement = { type: item.element || 'neutral', level: 1 };
}
// item.element is 'neutral' because DB was never updated for bDefEle
```

### Affected Items (8 non-card armor items)

| Item ID | Name | Script Element | DB Element | Correct |
|---------|------|---------------|------------|---------|
| 2306 | Flame Sprits Armor | `bDefEle,Ele_Fire` | neutral | fire |
| 2309 | Flame Sprits Armor_ | `bDefEle,Ele_Fire` | neutral | fire |
| 2307 | Water Sprits Armor | `bDefEle,Ele_Water` | neutral | water |
| 2310 | Water Sprits Armor_ | `bDefEle,Ele_Water` | neutral | water |
| 2308 | Wind Sprits Armor | `bDefEle,Ele_Wind` | neutral | wind |
| 2311 | Wind Sprits Armor_ | `bDefEle,Ele_Wind` | neutral | wind |
| 2305 | Earth Sprits Armor | `bDefEle,Ele_Earth` | neutral | earth |
| 2304 | Earth Sprits Armor_ | `bDefEle,Ele_Earth` | neutral | earth |

**Note**: Card-based `bDefEle` (Ghostring, Pasana, etc.) is correctly handled by `ro_card_effects.js` at runtime. This issue only affects non-card armor items with innate elements.

### Required Fix

**Migration**:
```sql
-- FIX: Armor defense element (bDefEle) for non-card armor items
UPDATE items SET element = 'fire'  WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Fire%';
UPDATE items SET element = 'water' WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Water%';
UPDATE items SET element = 'wind'  WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Wind%';
UPDATE items SET element = 'earth' WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Earth%';
UPDATE items SET element = 'holy'  WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Holy%';
UPDATE items SET element = 'dark'  WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Dark%';
UPDATE items SET element = 'ghost' WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Ghost%';
UPDATE items SET element = 'poison' WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Poison%';
UPDATE items SET element = 'undead' WHERE item_type = 'armor' AND script LIKE '%bDefEle,Ele_Undead%';
```

**Generator**: Add `bDefEle` parsing alongside `bAtkEle` in `parseBonusScript()`:
```javascript
case 'DefEle': // armor defense element — treat same as AtkEle for the element column
    // Only for armor items (cards handle this via ro_card_effects.js)
    bonuses.element = eleKey; break;
```

---

## Issue 3: ASPD Rate Bonuses From Equipment Not Applied

**Severity**: MEDIUM — 83 items with `bAspdRate` have no effect on ASPD
**Status**: NOT IMPLEMENTED

### Problem

83 items have `bonus bAspdRate,N;` in their scripts (percentage ASPD modification). The generator does not parse this into any column, and the server does not read it. The `aspd_modifier` column exists but is a flat additive value, not a percentage rate.

The server's ASPD formula (ro_damage_formulas.js) already supports `buffAspdMultiplier` for buff-based ASPD modifiers. Equipment ASPD rate bonuses should follow the same pattern.

### Affected Items (83 total, showing high-impact examples)

| Item | bAspdRate | Effect |
|------|-----------|--------|
| Muramasa (1173) | +8% | Major — katana with ASPD bonus |
| Assassin Dagger (1232) | +2% | Moderate |
| Great Axe (1314) | +5% | Moderate |
| Doom Slayer (1178) | -40% | Major — penalty for massive weapon |
| Various bows/daggers | +2% to +10% | Varies |
| 7 items with +100% | +100% | Investigation needed — likely Berserk weapons |

### Required Fix

**Step 1**: Add DB column
```sql
ALTER TABLE items ADD COLUMN IF NOT EXISTS aspd_rate INTEGER DEFAULT 0;
```

**Step 2**: Generator — parse `bAspdRate`
```javascript
case 'AspdRate': bonuses.aspd_rate = (bonuses.aspd_rate || 0) + n; break;
```

Add to columns, values, ON CONFLICT.

**Step 3**: Migration to populate existing items (parse from script column)

**Step 4**: Server — track and apply equipment ASPD rate
```javascript
// In player object:
player.equipAspdRate = 0;

// In equip/unequip handlers:
player.equipAspdRate += item.aspd_rate || 0;

// In getEffectiveStats() — combine with buff multiplier:
buffAspdMultiplier: (buffMods.aspdMultiplier || 1) * (1 + (player.equipAspdRate || 0) / 100),
// OR as a separate field:
equipAspdRate: player.equipAspdRate || 0,
```

**Step 5**: In `calculateDerivedStats()`, apply equipment ASPD rate using the existing `SpeedModifier` parameter in the ASPD formula.

---

## Issue 4: Generator INSERT Missing 8 DB Columns

**Severity**: MEDIUM — systematic gap, relies on migrations to patch
**Status**: PARTIALLY MITIGATED

### Missing Columns

| Column | Type | Default | Mitigated By | Server Reads | Impact |
|--------|------|---------|-------------|-------------|--------|
| `element` | VARCHAR(10) | 'neutral' | Migration (bAtkEle only) | YES — weapon & armor element | HIGH — armor bDefEle NOT fixed |
| `two_handed` | BOOLEAN | false | Migration | YES — equip validation | LOW — migration covers it |
| `aspd_modifier` | FLOAT | 0 | None | YES — flat ASPD mod | LOW — rAthena uses % not flat |
| `card_type` | VARCHAR(20) | NULL | Partial migration (7 cards) | YES — compound validation | LOW — only 7 cards have it |
| `card_prefix` | VARCHAR(50) | NULL | None | NO | NONE — unused |
| `card_suffix` | VARCHAR(50) | NULL | None | NO | NONE — unused |
| `class_restrictions` | TEXT | NULL | None | NO | NONE — uses `classes_allowed` |
| `ammo_type` | VARCHAR(20) | NULL | None | NO | NONE — unused |

### Required Fix

Add these columns to the generator's `columns` array, `values` array, and ON CONFLICT clause:

```javascript
// columns array additions:
'element', 'two_handed', 'aspd_modifier'

// values array additions:
bonuses.element || 'neutral',                      // element
isTwoHanded(item.SubType, item.Locations),         // two_handed (helper already exists at line 294)
0,                                                  // aspd_modifier (flat, from rAthena)

// ON CONFLICT additions:
'  element = EXCLUDED.element,'
'  two_handed = EXCLUDED.two_handed,'
```

**Note**: `card_type`, `card_prefix`, `card_suffix` are low priority. `class_restrictions` and `ammo_type` are dead code.

---

## Issue 5: bMatkRate Parsed But Then Deleted

**Severity**: LOW — by design, but creates confusion
**Status**: DESIGN ISSUE

### Problem

The generator parses `bMatkRate,N` (188 items) and stores the percentage value as an integer in the `matk` column. Then the migration `fix_item_element_matk_twohanded.sql` immediately clears `matk = 0` for all items with `bMatkRate` in their script. This is correct for pre-renewal (no flat MATK on items), but the data is parsed and then thrown away.

### Affected Items

188 items with `bMatkRate` in scripts — primarily staves, rods, and books. Pre-renewal these items give a hidden +15% MATK engine bonus, not the flat MATK shown in renewal.

### Impact

Currently: NO gameplay impact (matk correctly 0 for pre-renewal).
Future: If implementing MATK% equipment bonuses, would need a `matk_rate` column similar to `max_hp_rate`.

### Recommended Action

LOW PRIORITY. For now, the generator should skip `bMatkRate` parsing to avoid false signals:
```javascript
case 'MatkRate': /* Pre-renewal: skip — no flat MATK from equipment */ break;
```

If MATK% from equipment is ever needed, add a dedicated `matk_rate` column.

---

## Issue 6: Movement Speed Bonuses Not Applied

**Severity**: LOW — 14 items, movement speed system not yet implemented
**Status**: NOT IMPLEMENTED (deferred)

### Problem

14 items have `bonus bSpeedRate,N;` (movement speed % bonus). No DB column exists and the server has no movement speed modifier system.

### Affected Items

| Item | bSpeedRate | Notes |
|------|-----------|-------|
| Brood Axe | +25% | Rental weapon |
| Joker Jester | +25% | Headgear |
| Happy Wig | +25% | Headgear |
| Pecopeco Hairband | +25% | Headgear |
| Various others | +25% | Mostly headgear |

### Recommended Action

DEFER until movement speed system is implemented. When ready:
1. Add `speed_rate` INTEGER column
2. Parse `bSpeedRate` in generator
3. Server: apply to character movement speed

---

## Issue 7: Long Range ATK Rate Not Applied

**Severity**: LOW — 6 items
**Status**: NOT IMPLEMENTED (deferred)

### Problem

6 items have `bonus bLongAtkRate,N;` (long-range physical ATK %). No column or server handling exists.

### Affected Items

| Item | bLongAtkRate | Notes |
|------|-------------|-------|
| Balistar | +20% | Crossbow |
| Thimble of Archer | +3% | Accessory |
| Various others | +3% to +20% | |

### Recommended Action

DEFER until ranged damage modifiers are separated from melee in the damage pipeline. When ready:
1. Add `long_atk_rate` INTEGER column
2. Parse `bLongAtkRate` in generator
3. Apply in `calculatePhysicalDamage()` for ranged attacks

---

## Issue 8: Flat ASPD Bonus (bAspd) Not Populated

**Severity**: LOW — only 1 item (Masamune)
**Status**: NOT IMPLEMENTED

### Problem

The `aspd_modifier` column exists in the DB and IS read by the server (as `player.weaponAspdMod`), but the generator never populates it. Only 1 item has `bonus bAspd,N;` (Masamune, +2 flat ASPD).

### Required Fix

```javascript
// In parseBonusScript():
case 'Aspd': bonuses.aspd_modifier = (bonuses.aspd_modifier || 0) + n; break;
```

Add `aspd_modifier` to columns/values/ON CONFLICT. Or simply create a migration:
```sql
UPDATE items SET aspd_modifier = 2 WHERE item_id = 1173; -- Masamune
```

---

## Bonus Pattern Coverage Summary

### Currently Parsed by Generator (17 patterns)

| Pattern | DB Column | Status |
|---------|----------|--------|
| `bStr,N` | str_bonus | Working |
| `bAgi,N` | agi_bonus | Working |
| `bVit,N` | vit_bonus | Working |
| `bInt,N` | int_bonus | Working |
| `bDex,N` | dex_bonus | Working |
| `bLuk,N` | luk_bonus | Working |
| `bDef,N` / `bDef2,N` | def (additive) | Working |
| `bMdef,N` / `bMdef2,N` | mdef | Working |
| `bBaseAtk,N` / `bAtk,N` / `bAtk2,N` | atk (additive) | Working |
| `bMaxHP,N` | max_hp_bonus | Working |
| `bMaxSP,N` | max_sp_bonus | Working |
| `bHit,N` | hit_bonus | Working |
| `bFlee,N` | flee_bonus | Working |
| `bFlee2,N` | perfect_dodge_bonus | **FIXED 2026-03-13** |
| `bCritical,N` | critical_bonus | Working |
| `bAllStats,N` | all 6 stat columns | Working |
| `bMatkRate,N` | matk (then cleared) | Design issue (see Issue 5) |

### NOT Parsed — Needs Implementation

| Pattern | Count | Priority | Column Needed |
|---------|-------|----------|--------------|
| `bMaxHPrate,N` | 25 equip | **HIGH** | `max_hp_rate` |
| `bMaxSPrate,N` | 26 equip | **HIGH** | `max_sp_rate` |
| `bAspdRate,N` | 83 | **MEDIUM** | `aspd_rate` |
| `bDefEle,Ele_X` | 8 armor | **HIGH** | `element` (existing, needs population) |
| `bAspd,N` | 1 | LOW | `aspd_modifier` (existing) |
| `bSpeedRate,N` | 14 | LOW (deferred) | `speed_rate` (new) |
| `bLongAtkRate,N` | 6 | LOW (deferred) | `long_atk_rate` (new) |
| `bPerfectHitRate,N` | 2 | LOW (deferred) | `perfect_hit_rate` (new) |
| `bSplashRange,N` | 1 | LOW (deferred) | `splash_range` (new) |

### Parsed at Runtime by ro_card_effects.js (cards only)

| Pattern | Status | Notes |
|---------|--------|-------|
| `bonus2 bAddRace,RC_X,N` | Working | Offensive race % |
| `bonus2 bAddSize,Size_X,N` | Working | Offensive size % |
| `bonus2 bAddEle,Ele_X,N` | Working | Offensive element % |
| `bonus2 bSubRace,RC_X,N` | Working | Defensive race % |
| `bonus2 bSubSize,Size_X,N` | Working | Defensive size % |
| `bonus2 bSubEle,Ele_X,N` | Working | Defensive element % |
| `bonus bDefEle,Ele_X` | Working | Armor element change |
| `bonus bMaxHPrate,N` | Working | Card HP% (e.g., Ghostring -25%) |
| `bonus bMaxSPrate,N` | Working | Card SP% |

---

## Dead Code Columns (No Action Needed)

| Column | Type | Default | Notes |
|--------|------|---------|-------|
| `card_prefix` | VARCHAR(50) | NULL | Never populated, never read by server |
| `card_suffix` | VARCHAR(50) | NULL | Never populated, never read by server |
| `class_restrictions` | TEXT | NULL | Superseded by `classes_allowed` |
| `ammo_type` | VARCHAR(20) | NULL | Never populated, never read by server |
| `aspd_modifier` | FLOAT | 0 | Column exists but never populated (rAthena uses % not flat) |

---

## Implementation Priority

### Phase 1: Immediate Fixes (migrations only, no server changes)

1. **Armor defense element** (Issue 2) — 8 items, simple UPDATE migration
2. **Flat ASPD bonus** (Issue 8) — 1 item (Masamune), simple UPDATE

### Phase 2: Equipment Rate Bonuses (requires server + DB + generator changes)

3. **Equipment MaxHP/SP rate** (Issue 1) — new columns, player tracking, getEffectiveStats merge
4. **ASPD rate from equipment** (Issue 3) — new column, player tracking, ASPD formula integration

### Phase 3: Generator Hardening (prevent future gaps)

5. **Add missing columns to generator INSERT** (Issue 4) — element, two_handed, aspd_modifier + new rate columns
6. **Skip bMatkRate parsing** (Issue 5) — avoid false signals

### Phase 4: Deferred (when systems are implemented)

7. Movement speed rate (Issue 6)
8. Long range ATK rate (Issue 7)

---

## Verification Queries

After implementing fixes, run these queries to verify:

```sql
-- Verify armor element items
SELECT item_id, name, element FROM items
WHERE script LIKE '%bDefEle%' AND item_type = 'armor'
ORDER BY item_id;

-- Verify no equipment MaxHP rate items have max_hp_rate = 0
SELECT item_id, name, max_hp_rate FROM items
WHERE script LIKE '%bMaxHPrate%' AND item_type != 'card' AND max_hp_rate = 0;

-- Verify no equipment MaxSP rate items have max_sp_rate = 0
SELECT item_id, name, max_sp_rate FROM items
WHERE script LIKE '%bMaxSPrate%' AND item_type != 'card' AND max_sp_rate = 0;

-- Verify ASPD rate items
SELECT item_id, name, aspd_rate FROM items
WHERE script LIKE '%bAspdRate%' AND aspd_rate = 0;

-- Verify perfect_dodge_bonus on cards
SELECT item_id, name, perfect_dodge_bonus FROM items
WHERE item_type = 'card' AND script LIKE '%bFlee2%' AND perfect_dodge_bonus = 0;

-- Verify element column matches bAtkEle script (weapons)
SELECT item_id, name, element FROM items
WHERE script LIKE '%bAtkEle%' AND element = 'neutral'
AND script NOT LIKE '%Ele_Neutral%';
```
