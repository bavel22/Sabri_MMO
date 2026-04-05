# Buff Naming Consistency Audit

**Date**: 2026-03-23
**Scope**: `server/src/index.js`, `server/src/ro_buff_system.js`, `server/src/ro_status_effects.js`, client C++ subsystems
**Method**: 7-pass cross-reference of all buff names across applyBuff/removeBuff/hasBuff calls, BUFF_TYPES definitions, BUFFS_SURVIVE_DEATH, UNDISPELLABLE, and client-side code

---

## Executive Summary

Found **15 naming mismatches** across 6 categories. Two are CRITICAL (buff logic silently fails), five are HIGH (UNDISPELLABLE/SURVIVE_DEATH contain phantom names), and the rest are MEDIUM (missing BUFF_TYPES entries, client-side name corruption).

---

## 1. Complete Buff Name Catalog

### 1A. BUFF_TYPES in ro_buff_system.js (82 entries, canonical names)

```
provoke, endure, sight, blessing, blessing_debuff, increase_agi, decrease_agi,
angelus, pneuma, signum_crucis, auto_berserk, hiding, play_dead,
improve_concentration, two_hand_quicken, kyrie_eleison, magnificat, gloria,
lex_aeterna, aspersio, energy_coat, enchant_poison, cloaking, poison_react,
quagmire, loud_exclamation, ruwach, magnum_break_fire, impositio_manus,
suffragium, slow_poison, bs_sacramenti, auto_counter, auto_guard,
reflect_shield, defender, spear_quicken, providence, shrink,
devotion_protection, sight_blaster, endow_fire, endow_water, endow_wind,
endow_earth, hindsight, magic_rod, volcano_zone, deluge_zone,
violent_gale_zone, critical_explosion, steel_body, asura_regen_lockout,
sitting, blade_stop_catching, root_lock, song_whistle, song_assassin_cross,
song_bragi, song_apple_of_idun, dance_humming, dance_fortune_kiss,
dance_service_for_you, dance_pdfm, dance_ugly, ensemble_drum,
ensemble_nibelungen, ensemble_mr_kim, ensemble_siegfried,
ensemble_into_abyss, ensemble_aftermath, adrenaline_rush, weapon_perfection,
power_thrust, maximize_power, strip_weapon, strip_shield, strip_armor,
strip_helm, raid_debuff, close_confine, aspd_potion, str_food, agi_food,
vit_food, int_food, dex_food, luk_food, hit_food, flee_food,
item_endow_fire, item_endow_water, item_endow_wind, item_endow_earth,
item_endow_dark
```

### 1B. STATUS_EFFECTS in ro_status_effects.js (12 entries)

```
stun, freeze, stone, sleep, ankle_snare, poison, blind, silence,
confusion, bleeding, curse, petrifying
```

### 1C. Buff names used in applyBuff() but NOT in BUFF_TYPES

| Name used in applyBuff | Where | Issue |
|------------------------|-------|-------|
| `armor_break` | line 20462 (Acid Terror) | Missing from BUFF_TYPES |
| `weapon_break` | line 28174 (Demonstration tick) | Missing from BUFF_TYPES |
| `urgent_escape` | line 21560 (Homunculus skill) | Missing from BUFF_TYPES |
| `amistr_bulwark` | line 21613 (Homunculus skill) | Missing from BUFF_TYPES |
| `chemical_protection_helm` | line 20721 (CP skills) | Missing from BUFF_TYPES |
| `chemical_protection_shield` | line 20721 | Missing from BUFF_TYPES |
| `chemical_protection_armor` | line 20721 | Missing from BUFF_TYPES |
| `chemical_protection_weapon` | line 20721 | Missing from BUFF_TYPES |

### 1D. Buff names used in hasBuff/removeBuff but NOT in BUFF_TYPES

| Name | Where | Issue |
|------|-------|-------|
| `adrenaline_rush_full` | lines 23241, 23242, 11641 | Missing from BUFF_TYPES |
| `one_hand_quicken` | lines 23227, 23228, 11641 | Missing from BUFF_TYPES |
| `preserve` | line 260 (Plagiarism check) | Missing from BUFF_TYPES |
| `chemical_protection_armor` | line 20461 (hasBuff check) | Missing from BUFF_TYPES |
| `chemical_protection_weapon` | line 28173 (hasBuff check) | Missing from BUFF_TYPES |

---

## 2. All Mismatches Found

### CRITICAL — Buff logic silently fails

#### C1: BUFFS_SURVIVE_DEATH contains `song_humming` (should be `dance_humming`)

**Location**: `server/src/index.js` line 2035
**Current code**:
```js
const BUFFS_SURVIVE_DEATH = new Set([
    'auto_berserk', 'endure', 'shrink',
    'song_whistle', 'song_assassin_cross', 'song_bragi', 'song_apple_of_idun',
    'song_humming', 'dance_fortune_kiss', 'dance_service_for_you', 'dance_pdfm'
]);
```

**Problem**: `song_humming` does not exist anywhere. The buff is applied as `dance_humming` (see `PERFORMANCE_BUFF_MAP` line 959 and `BUFF_TYPES` line 403). This means Humming (Dancer buff) is NOT surviving death when it should be.

**Pattern**: All Bard songs use `song_` prefix, all Dancer dances use `dance_` prefix. Humming is a Dancer skill, so it is `dance_humming`.

**Fix**: Change `'song_humming'` to `'dance_humming'`.

---

#### C2: UNDISPELLABLE contains `stripweapon/stripshield/striparmor/striphelm` (should be `strip_weapon/strip_shield/strip_armor/strip_helm`)

**Location**: `server/src/index.js` line 15320
**Current code**:
```js
const UNDISPELLABLE = new Set([
    'hindsight', 'play_dead', 'auto_berserk', 'devotion_protection',
    'steel_body', 'combo',
    'stripweapon', 'stripshield', 'striparmor', 'striphelm',
    ...
]);
```

**Problem**: The strip debuffs are applied as `strip_weapon`, `strip_shield`, `strip_armor`, `strip_helm` (see BUFF_TYPES lines 501-524 and applyBuff at line 19815 which uses `stripName` from `cpMap` which maps to `'strip_weapon'` etc.). The UNDISPELLABLE set uses no-underscore variants that will never match.

**Impact**: Dispell currently REMOVES strip debuffs when it should not (they should persist through Dispell per rAthena).

**Fix**: Change `'stripweapon'` to `'strip_weapon'`, `'stripshield'` to `'strip_shield'`, `'striparmor'` to `'strip_armor'`, `'striphelm'` to `'strip_helm'`.

---

### HIGH — Phantom names in sets (no runtime error but wrong behavior)

#### H1: UNDISPELLABLE contains `cp_weapon/cp_shield/cp_armor/cp_helm` (actual names are `chemical_protection_*`)

**Location**: `server/src/index.js` line 15321
**Current code**:
```js
'cp_weapon', 'cp_shield', 'cp_armor', 'cp_helm',
'chemical_protection_helm', 'chemical_protection_shield', 'chemical_protection_armor', 'chemical_protection_weapon',
```

**Problem**: The CP buffs are applied with `name: skill.name` at line 20721, where `skill.name` is `chemical_protection_helm/shield/armor/weapon`. The `cp_*` short names are never used as buff names anywhere. They are harmless phantom entries (the `chemical_protection_*` entries correctly protect these buffs).

**Fix**: Remove the redundant `cp_weapon`, `cp_shield`, `cp_armor`, `cp_helm` entries from UNDISPELLABLE. They serve no purpose and are confusing.

---

#### H2: UNDISPELLABLE contains `combo` (never used as a buff name)

**Location**: `server/src/index.js` line 15319
**Problem**: The Monk combo system uses `player.comboState` (a plain object on the player), NOT a buff via `applyBuff`. No buff named `combo` is ever applied. This is a phantom entry.

**Fix**: Remove `'combo'` from UNDISPELLABLE.

---

#### H3: UNDISPELLABLE contains `dancing` (never used as a buff name)

**Location**: `server/src/index.js` line 15324
**Problem**: The performance system uses `player.performanceState` (a plain object), NOT a buff named `dancing`. No buff named `dancing` is ever applied. This is a phantom entry.

**Fix**: Remove `'dancing'` from UNDISPELLABLE.

---

#### H4: UNDISPELLABLE contains future/unimplemented names

**Location**: `server/src/index.js` lines 15323-15326
**Names**: `enchant_deadly_poison`, `cart_boost`, `meltdown`, `safety_wall`, `eternal_chaos`, `lokis_veil`, `into_the_abyss`

**Problem**: These buff names are not currently applied by any applyBuff call. They are forward-looking entries for future skill implementations:
- `enchant_deadly_poison` — Assassin Cross skill (not yet implemented)
- `cart_boost` — Blacksmith/Alchemist speed boost (not yet implemented)
- `meltdown` — Blacksmith attack skill (not yet implemented)
- `safety_wall` — Ground effect, not a buff (never applied via applyBuff)
- `eternal_chaos`, `lokis_veil`, `into_the_abyss` — Ensemble effects that work as ground AoE zones, not as named buffs on players

**Fix**: Keep `enchant_deadly_poison`, `cart_boost`, `meltdown` for future-proofing. Remove `safety_wall` (ground effect, not a buff). For `eternal_chaos`/`lokis_veil`/`into_the_abyss`, these are ensemble canonical names used in the ground effect system, not buff names. The actual ensemble buffs that ARE applied use `ensemble_into_abyss` etc. Replace with the correct buff names or remove.

---

#### H5: Abracadabra uses skill names as buff names (endow_blaze vs endow_fire)

**Location**: `server/src/index.js` lines 16333-16336
**Current code**:
```js
applyBuff(player, {
    skillId: picked.id, name: picked.name,  // <-- uses skill name directly
    casterId: characterId, casterName: player.characterName,
    duration: buffDuration
});
```

**Problem**: When Abracadabra randomly picks endow skills, `picked.name` is the SKILL name (`endow_blaze`, `endow_tsunami`, `endow_tornado`, `endow_quake`), but the BUFF_TYPES and the getBuffModifiers switch-case use `endow_fire`, `endow_water`, `endow_wind`, `endow_earth`. The Abracadabra-applied endow buffs:
1. Never match any `case` in `getBuffModifiers()` (fall through to generic default)
2. The `weaponElement` property is never set on the buff object (Abracadabra passes no `weaponElement` field)
3. Don't get removed by the endow mutual-exclusion logic (which checks for `endow_fire` etc.)

**Impact**: Abracadabra endow buffs silently do nothing. The weapon element is never changed.

**Fix**: Map skill names to buff names in the Abracadabra self-buff handler:
```js
const ABRA_SKILL_TO_BUFF = {
    'endow_blaze': 'endow_fire', 'endow_tsunami': 'endow_water',
    'endow_tornado': 'endow_wind', 'endow_quake': 'endow_earth',
    'summon_spirit_sphere': null, 'blade_stop': null,  // Not buff-type skills
};
const actualBuffName = ABRA_SKILL_TO_BUFF[picked.name] || picked.name;
```
Also add `weaponElement` to the buff definition for endow buffs.

---

### MEDIUM — Missing BUFF_TYPES entries

#### M1: `chemical_protection_weapon/shield/armor/helm` missing from BUFF_TYPES

**Impact**: `getBuffModifiers()` switch-case has no handler for these buffs, so they fall through to the generic default handler. However, the CP buffs don't modify stats — they are checked via `hasBuff()` calls only (e.g., `hasBuff(enemy, 'chemical_protection_armor')`). So `hasBuff()` still works (it just checks the activeBuffs array). But `getActiveBuffList()` returns no `displayName` or `abbrev` for these buffs — the client buff bar shows raw name with truncated abbreviation.

**Fix**: Add to BUFF_TYPES:
```js
chemical_protection_weapon: { stackRule: 'refresh', category: 'buff', displayName: 'Chemical Protection (Weapon)', abbrev: 'CPW' },
chemical_protection_shield: { stackRule: 'refresh', category: 'buff', displayName: 'Chemical Protection (Shield)', abbrev: 'CPS' },
chemical_protection_armor: { stackRule: 'refresh', category: 'buff', displayName: 'Chemical Protection (Armor)', abbrev: 'CPA' },
chemical_protection_helm: { stackRule: 'refresh', category: 'buff', displayName: 'Chemical Protection (Helm)', abbrev: 'CPH' },
```

---

#### M2: `armor_break` and `weapon_break` missing from BUFF_TYPES

**Impact**: Applied to enemies only (lines 20462, 28174). Fall through to generic default handler in `getBuffModifiers()`. `armor_break` has `hardDefReduction: 100` which the default handler catches (`if (buff.defReduction)`... but wait, the field is `hardDefReduction`, not `defReduction`). So armor break's DEF reduction is NOT being processed.

For `weapon_break`, `atkReduction: 50` — the default handler catches `if (buff.atkIncrease)` but NOT `atkReduction`. So weapon break ATK reduction is NOT being processed.

**Fix**: Add to BUFF_TYPES and add proper getBuffModifiers cases:
```js
armor_break: { stackRule: 'refresh', category: 'debuff', displayName: 'Armor Break', abbrev: 'ABK' },
weapon_break: { stackRule: 'refresh', category: 'debuff', displayName: 'Weapon Break', abbrev: 'WBK' },
```
Add switch cases in getBuffModifiers:
```js
case 'armor_break':
    mods.hardDefReduction = (mods.hardDefReduction || 0) + (buff.hardDefReduction || 0);
    break;
case 'weapon_break':
    mods.atkMultiplier *= (1 - (buff.atkReduction || 0) / 100);
    break;
```

---

#### M3: `adrenaline_rush_full` and `one_hand_quicken` missing from BUFF_TYPES

**Impact**: Referenced in hasBuff/removeBuff at lines 23241, 23227, 11641 (weapon swap cancel and Decrease AGI strip). Currently never applied (Adrenaline Rush Full and One-Hand Quicken skills not yet implemented). But when they are, these need BUFF_TYPES entries and getBuffModifiers cases.

**Fix**: Add to BUFF_TYPES (future-proofing):
```js
adrenaline_rush_full: { stackRule: 'refresh', category: 'buff', displayName: 'Adrenaline Rush Full', abbrev: 'ARF' },
one_hand_quicken: { stackRule: 'refresh', category: 'buff', displayName: 'One-Hand Quicken', abbrev: 'OHQ' },
```
And add Haste2 group case in getBuffModifiers:
```js
case 'adrenaline_rush_full':
    haste2Bonus = Math.max(haste2Bonus, ((buff.aspdMultiplier || 1.0) - 1) * 100);
    break;
case 'one_hand_quicken':
    haste2Bonus = Math.max(haste2Bonus, buff.aspdIncrease || 0);
    break;
```

---

#### M4: `preserve` missing from BUFF_TYPES

**Impact**: Only checked via `hasBuff(targetPlayer, 'preserve')` at line 260 (Plagiarism copy). Preserve skill not yet fully implemented (Rogue/Stalker Preserve), but the check exists. When implemented, it needs a BUFF_TYPES entry.

**Fix**: Add to BUFF_TYPES:
```js
preserve: { stackRule: 'refresh', category: 'buff', displayName: 'Preserve', abbrev: 'PRE' },
```

---

#### M5: `urgent_escape` and `amistr_bulwark` missing from BUFF_TYPES

**Impact**: Applied by homunculus skills (lines 21560, 21613). Fall through to generic default in getBuffModifiers. `urgent_escape` has `moveSpeedPct` which the default handler does NOT catch. `amistr_bulwark` has `vit` which the default handler does NOT catch. Both buffs have NO stat effect.

**Fix**: Add to BUFF_TYPES and getBuffModifiers:
```js
urgent_escape: { stackRule: 'refresh', category: 'buff', displayName: 'Urgent Escape', abbrev: 'UES' },
amistr_bulwark: { stackRule: 'refresh', category: 'buff', displayName: 'Amistr Bulwark', abbrev: 'AMB' },
```
```js
case 'urgent_escape':
    mods.moveSpeedBonus += (buff.moveSpeedPct || 0);
    break;
case 'amistr_bulwark':
    mods.vitBonus += (buff.vit || 0);
    break;
```

---

### CLIENT — Buff name corruption

#### CL1: BuffBarSubsystem converts underscores to spaces, breaking HasBuff lookups

**Location**: `client/SabriMMO/Source/SabriMMO/UI/BuffBarSubsystem.cpp` lines 217, 225
**Current code** (HandleBuffApplied):
```cpp
FString BuffNameLower = BuffName.ToLower().Replace(TEXT("_"), TEXT(" "));
// ...
Info.Name = BuffNameLower;  // Stores "play dead", "auto berserk", etc.
```

**Location**: `client/SabriMMO/Source/SabriMMO/UI/PlayerInputSubsystem.cpp` line 73
```cpp
if (BBS->HasBuff(TEXT("play_dead")) || BBS->HasBuff(TEXT("hiding")) || BBS->HasBuff(TEXT("sitting")))
```

**Problem**: BuffBarSubsystem stores buff names with spaces (`"play dead"`, `"hiding"`, `"sitting"`), but PlayerInputSubsystem checks for underscored names (`"play_dead"`). The `HasBuff("play_dead")` call will never find `"play dead"` in the array.

**Impact**: Player can still use movement/attack inputs while Play Dead, Hiding, or Sitting are active (client-side check fails, server still blocks but client lets you try).

**Fix**: Either:
- (A) Stop converting underscores to spaces in BuffBarSubsystem (preferred — keep names as-is from server)
- (B) Have HasBuff() also convert its input parameter the same way
- (C) Change PlayerInputSubsystem to use space-separated names

Recommended: Option (A) — remove the `.Replace(TEXT("_"), TEXT(" "))` in both HandleBuffApplied and HandleBuffRemoved. The server sends snake_case names consistently. The client GetBuffAbbrev() already handles both forms with `||` checks. The `HandleBuffList` handler (line 290) ALSO does this conversion, so fix it in all three places.

---

#### CL2: OtherPlayerSubsystem compares raw server name (correct) vs BuffBarSubsystem (converted)

**Location**: `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.cpp` line 409
```cpp
if (BuffName != TEXT("hiding") && BuffName != TEXT("cloaking")) return;
```

**This is correct** — it compares against the raw server-sent name. But it means OtherPlayerSubsystem and BuffBarSubsystem use different name formats for the same buffs. If OtherPlayerSubsystem ever needs to call `BuffBarSubsystem::HasBuff()`, it would break.

**Fix**: Part of CL1 fix — once BuffBarSubsystem stops converting, all subsystems use the same format.

---

## 3. Second UNDISPELLABLE Set (Abracadabra Dispell)

**Location**: `server/src/index.js` line 16392
```js
const UNDISPELLABLE = new Set(['hindsight', 'play_dead', 'auto_berserk', 'wedding', 'riding', 'cart',
    'devotion_protection', 'steel_body', 'cp_weapon', 'cp_shield', 'cp_armor', 'cp_helm',
    'chemical_protection_helm', 'chemical_protection_shield', 'chemical_protection_armor', 'chemical_protection_weapon']);
```

**Issues**:
- `wedding`, `riding`, `cart` — never used as buff names (phantom entries for unimplemented systems)
- `cp_weapon`, `cp_shield`, `cp_armor`, `cp_helm` — same phantom entries as H1
- Missing entries compared to main UNDISPELLABLE: no `combo`, `stripweapon/shield/armor/helm`, `enchant_deadly_poison`, `cart_boost`, `meltdown`, `safety_wall`, `dancing`, `blade_stop_catching`, `root_lock`, `sitting`, `eternal_chaos`, `lokis_veil`, `into_the_abyss`, `ensemble_aftermath`

**Fix**: Sync both UNDISPELLABLE sets. Extract to a shared constant at module scope:
```js
const UNDISPELLABLE_BUFFS = new Set([
    'hindsight', 'play_dead', 'auto_berserk', 'devotion_protection',
    'steel_body', 'blade_stop_catching', 'root_lock', 'sitting',
    'ensemble_aftermath',
    'strip_weapon', 'strip_shield', 'strip_armor', 'strip_helm',
    'chemical_protection_helm', 'chemical_protection_shield',
    'chemical_protection_armor', 'chemical_protection_weapon',
    // Future (not yet implemented):
    'enchant_deadly_poison', 'cart_boost', 'meltdown',
]);
```

---

## 4. Recommended Canonical Naming Convention

All buff names MUST be **snake_case** and MUST match across:
1. The `name` field in `applyBuff()` calls
2. The key in `BUFF_TYPES` object
3. The string in `hasBuff()` / `removeBuff()` calls
4. The entries in `BUFFS_SURVIVE_DEATH` set
5. The entries in `UNDISPELLABLE` set(s)
6. The `buffName` field in socket events (`skill:buff_applied`, `skill:buff_removed`)

Skill names (used in `SKILL_DATA` / `SKILL_MAP`) should NEVER be used directly as buff names unless they happen to be identical. When they differ, map skill name to buff name explicitly.

---

## 5. Fix Specification (Priority Order)

### Fix 1 (CRITICAL): BUFFS_SURVIVE_DEATH — `song_humming` to `dance_humming`
**File**: `server/src/index.js` line 2035
**Change**: `'song_humming'` -> `'dance_humming'`

### Fix 2 (CRITICAL): UNDISPELLABLE — strip names
**File**: `server/src/index.js` line 15320
**Change**:
```
'stripweapon', 'stripshield', 'striparmor', 'striphelm'
```
to:
```
'strip_weapon', 'strip_shield', 'strip_armor', 'strip_helm'
```

### Fix 3 (HIGH): Clean up phantom entries in UNDISPELLABLE
**File**: `server/src/index.js` lines 15317-15328
**Remove**: `'combo'`, `'dancing'`, `'safety_wall'`, `'cp_weapon'`, `'cp_shield'`, `'cp_armor'`, `'cp_helm'`, `'eternal_chaos'`, `'lokis_veil'`, `'into_the_abyss'`

### Fix 4 (HIGH): Sync Abracadabra UNDISPELLABLE (line 16392)
**File**: `server/src/index.js` line 16392
**Remove phantom entries**: `'wedding'`, `'riding'`, `'cart'`, `'cp_weapon'`, `'cp_shield'`, `'cp_armor'`, `'cp_helm'`
**Add missing entries**: `'strip_weapon'`, `'strip_shield'`, `'strip_armor'`, `'strip_helm'`, `'blade_stop_catching'`, `'root_lock'`, `'sitting'`, `'ensemble_aftermath'`

### Fix 5 (HIGH): Abracadabra endow buff name mapping
**File**: `server/src/index.js` around line 16333
**Add mapping** from skill name to buff name for endow skills, and include `weaponElement` in buff definition.

### Fix 6 (MEDIUM): Add missing BUFF_TYPES entries
**File**: `server/src/ro_buff_system.js`
**Add**: `chemical_protection_weapon`, `chemical_protection_shield`, `chemical_protection_armor`, `chemical_protection_helm`, `armor_break`, `weapon_break`, `urgent_escape`, `amistr_bulwark`, `adrenaline_rush_full`, `one_hand_quicken`, `preserve`

### Fix 7 (MEDIUM): Add getBuffModifiers switch cases
**File**: `server/src/ro_buff_system.js`
**Add cases for**: `armor_break`, `weapon_break`, `urgent_escape`, `amistr_bulwark`, `chemical_protection_*` (no-op but explicit), `adrenaline_rush_full`, `one_hand_quicken`, `preserve`

### Fix 8 (MEDIUM): Client underscore-to-space removal
**File**: `client/SabriMMO/Source/SabriMMO/UI/BuffBarSubsystem.cpp`
**Change**: Remove `.Replace(TEXT("_"), TEXT(" "))` from lines 217, 254, and 290.
**Also update**: `GetBuffAbbrev()` to remove the dual-format `||` checks (only need snake_case).

---

## 6. Cross-Reference Matrix

| Buff Name | BUFF_TYPES | applyBuff | hasBuff | removeBuff | SURVIVE_DEATH | UNDISPELLABLE | UNDISPELLABLE2 |
|-----------|-----------|-----------|---------|------------|---------------|---------------|----------------|
| `dance_humming` | YES | YES (perf tick) | -- | -- | **NO (song_humming)** | -- | -- |
| `strip_weapon` | YES | YES (19815) | -- | -- | -- | **NO (stripweapon)** | -- |
| `strip_shield` | YES | YES (19815) | -- | -- | -- | **NO (stripshield)** | -- |
| `strip_armor` | YES | YES (19815) | -- | -- | -- | **NO (striparmor)** | -- |
| `strip_helm` | YES | YES (19815) | -- | -- | -- | **NO (striphelm)** | -- |
| `chemical_protection_*` | **NO** | YES (20721) | YES | -- | -- | YES (both) | YES (both) |
| `cp_weapon/shield/armor/helm` | **NO** | **NO** | **NO** | **NO** | -- | phantom | phantom |
| `combo` | **NO** | **NO** | **NO** | **NO** | -- | phantom | -- |
| `dancing` | **NO** | **NO** | **NO** | **NO** | -- | phantom | -- |
| `safety_wall` | **NO** | **NO** | **NO** | **NO** | -- | phantom | -- |
| `armor_break` | **NO** | YES (20462) | -- | -- | -- | -- | -- |
| `weapon_break` | **NO** | YES (28174) | -- | -- | -- | -- | -- |
| `urgent_escape` | **NO** | YES (21560) | -- | -- | -- | -- | -- |
| `amistr_bulwark` | **NO** | YES (21613) | -- | -- | -- | -- | -- |
| `adrenaline_rush_full` | **NO** | -- | YES (23241) | YES (23242) | -- | -- | -- |
| `one_hand_quicken` | **NO** | -- | YES (23227) | YES (23228) | -- | -- | -- |
| `preserve` | **NO** | -- | YES (260) | -- | -- | -- | -- |
| `endow_blaze/tsunami/tornado/quake` (Abra) | **NO** | YES (16334) | -- | -- | -- | -- | -- |

---

## 7. Note on Client-Side GetBuffAbbrev Coverage

The client `GetBuffAbbrev()` function (BuffBarSubsystem.cpp line 16) only maps 17 buff names. All other buffs fall through to the "first 3 chars uppercase" default. This means most buffs display abbreviated garbage. The server sends `abbrev` in `buff:list` responses (via `getActiveBuffList`), but `skill:buff_applied` events do NOT include `abbrev`. The client should either:
- Use the server-provided `abbrev` from `buff:list` when available
- Expand the client-side mapping table
- Have the server include `abbrev` in all `skill:buff_applied` events
