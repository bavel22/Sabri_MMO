# Buff/Debuff Cross-System Integration Audit

**Auditor**: Claude Opus 4.6 (1M context)
**Date**: 2026-03-22
**Files Audited**:
- `server/src/ro_buff_system.js` (1180 lines)
- `server/src/ro_status_effects.js` (712 lines)
- `server/src/index.js` (~31000 lines) — searched for all buff/status patterns

---

## 1. BUFF TYPE CATALOG

### 1A. Status Effects (`ro_status_effects.js` — stored in `target.activeStatusEffects` Map)

| Type | Resist Stat | Base Duration | DoT | Break On Dmg | Prevents | Key Stat Mods |
|------|-------------|--------------|-----|--------------|----------|---------------|
| stun | VIT (cap 97) | 5s | No | No | Move/Cast/Atk/Items | Flee x0 |
| freeze | MDEF | 12s | No | Yes | Move/Cast/Atk | DEF x0.5, MDEF x1.25, Water Lv1 element |
| stone | MDEF | 20s | HP 1%/5s (floor 25% MaxHP) | Yes | Move/Cast/Atk | DEF x0.5, MDEF x1.25, Earth Lv1 element |
| petrifying | MDEF | 5s (Phase 1) | No | No | Cast/Atk | Transitions to stone on expiry |
| sleep | INT (cap 97) | 30s | No | Yes | Move/Cast/Atk | (100% hit, 2x crit vs sleeping) |
| poison | VIT (cap 97) | 60s | HP 1.5%+2/1s | No | (none) | DEF x0.75, blocks SP regen |
| blind | avg(INT,VIT) cap 193 | 30s | No | No | (none) | HIT x0.75, Flee x0.75 |
| silence | VIT (cap 97) | 30s | No | No | Cast | (none) |
| confusion | avg(STR,INT) cap 193 | 30s | No | Yes | (none) | (movement randomized) |
| bleeding | VIT (cap 97) | 120s | HP 2%/4s (can kill) | No | (none) | Blocks HP+SP regen |
| curse | LUK (cap 97) | 30s | No | No | (none) | ATK x0.75, LUK=0, MoveSpeed x0.1 |
| ankle_snare | none (always lands) | 20s | No | No | Move only | (none) |

**Breakable set**: `['freeze', 'stone', 'sleep', 'confusion']`

### 1B. Buffs (`ro_buff_system.js` — stored in `target.activeBuffs` array)

All buffs use `stackRule: 'refresh'` (reapplying same buff replaces existing).

#### Core Buffs (14 types)
| Name | Category | Display | Effect Summary |
|------|----------|---------|----------------|
| provoke | debuff | Provoke | -DEF% (soft), +ATK% |
| endure | buff | Endure | +MDEF flat |
| sight | buff | Sight | Reveal hidden |
| blessing | buff | Blessing | +STR/DEX/INT |
| blessing_debuff | debuff | Blessing (Curse) | -STR/DEX/INT (Undead/Demon) |
| increase_agi | buff | Increase AGI | +AGI, +25% moveSpeed |
| decrease_agi | debuff | Decrease AGI | -AGI, -25% moveSpeed |
| angelus | buff | Angelus | +DEF% |
| pneuma | buff | Pneuma | Block ranged |
| signum_crucis | debuff | Signum Crucis | -DEF% (hard) |
| auto_berserk | buff | Auto Berserk | +ATK% (conditional <25% HP), -55% soft DEF |
| hiding | buff | Hiding | isHidden, SP drain |
| play_dead | buff | Play Dead | isPlayDead, deaggro all |
| improve_concentration | buff | Concentration | +AGI, +DEX |

#### Priest Buffs (9 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| kyrie_eleison | buff | Damage barrier (handled in pipeline) |
| magnificat | buff | 2x HP/SP regen |
| gloria | buff | +LUK |
| lex_aeterna | debuff | Double next damage |
| aspersio | buff | Holy weapon element |
| impositio_manus | buff | +ATK flat |
| suffragium | buff | Cast time reduction |
| slow_poison | buff | Block poison HP drain |
| bs_sacramenti | buff | Holy armor element |

#### Knight/Crusader Buffs (8 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| two_hand_quicken | buff | ASPD Haste2 group, +CRI, +HIT |
| auto_counter | buff | Active counter mode |
| auto_guard | buff | Block% physical attacks |
| reflect_shield | buff | Reflect% melee damage |
| defender | buff | Reduce% ranged, -33% moveSpeed, -ASPD |
| spear_quicken | buff | ASPD Haste2 group |
| providence | buff | Demon resist, Holy resist |
| shrink | buff | Knockback on block |
| devotion_protection | buff | Redirect damage |

#### Wizard/Sage Buffs (10 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| sight_blaster | buff | Reactive AoE trigger |
| endow_fire/water/wind/earth | buff | Weapon element override |
| hindsight | buff | Autocast on melee |
| magic_rod | buff | Absorb single-target magic |
| volcano_zone | buff | +ATK (fire armor), fire dmg boost |
| deluge_zone | buff | +MaxHP% (water armor), water dmg boost |
| violent_gale_zone | buff | +Flee (wind armor), wind dmg boost |

#### Monk Buffs (5 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| critical_explosion | buff | +CRI, disable SP regen |
| steel_body | buff | DEF/MDEF override 90, -25% ASPD, -25% moveSpeed, block skills |
| asura_regen_lockout | debuff | Disable SP regen |
| sitting | buff | 2x regen (handled externally) |
| blade_stop_catching / root_lock | buff/debuff | Paired immobilization |

#### Bard/Dancer Buffs (11 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| song_whistle | buff | +Flee, +PerfectDodge |
| song_assassin_cross | buff | ASPD Haste2 group |
| song_bragi | buff | Cast/ACD reduction |
| song_apple_of_idun | buff | +MaxHP% |
| dance_humming | buff | +HIT |
| dance_fortune_kiss | buff | +CRI |
| dance_service_for_you | buff | +MaxSP%, SP cost reduction |
| dance_pdfm | debuff | -ASPD%, -moveSpeed |
| dance_ugly | debuff | SP drain (tick only) |

#### Ensemble Buffs (6 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| ensemble_drum | buff | +ATK, +DEF |
| ensemble_nibelungen | buff | +ATK (Lv4 weapon) |
| ensemble_mr_kim | buff | +EXP% |
| ensemble_siegfried | buff | Element resist, status resist |
| ensemble_into_abyss | buff | No gemstone cost |
| ensemble_aftermath | debuff | Block skills, -50% moveSpeed |

#### Blacksmith Buffs (4 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| adrenaline_rush | buff | ASPD Haste2 group |
| weapon_perfection | buff | No size penalty |
| power_thrust | buff | +ATK% |
| maximize_power | buff | Max weapon variance |

#### Rogue Debuffs (6 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| strip_weapon / strip_shield / strip_armor / strip_helm | debuff | ATK/DEF/VIT/INT reduction |
| raid_debuff | debuff | +20% incoming damage (7 hits or 5s) |
| close_confine | debuff | Paired movement lock |

#### Item Consumable Buffs (13 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| aspd_potion | buff | ASPD reduction |
| str/agi/vit/int/dex/luk_food | buff | +stat |
| hit_food / flee_food | buff | +HIT / +Flee |
| item_endow_fire/water/wind/earth/dark | buff | Weapon element (converter) |

#### Misc Buffs (5 types)
| Name | Category | Effect Summary |
|------|----------|----------------|
| energy_coat | buff | Physical damage reduction by SP% |
| enchant_poison | buff | Poison weapon element |
| cloaking | buff | isHidden, SP drain |
| poison_react | buff | Counter poison attacks |
| loud_exclamation | buff | +STR |
| ruwach | buff | Reveal hidden |
| magnum_break_fire | buff | +20% fire bonus 10s |
| quagmire | debuff | -AGI, -DEX, -50% moveSpeed |

**Total**: 12 status effects + ~77 buff types = **~89 total buff/debuff types**

---

## 2. MUTUAL EXCLUSION PAIRS

### 2A. Correctly Implemented

| Pair | Where | Behavior |
|------|-------|----------|
| **Increase AGI <-> Decrease AGI** | Increase AGI handler (line ~11551) | Removes `decrease_agi` before applying `increase_agi` |
| **Decrease AGI -> strips ASPD buffs** | Decrease AGI handler (line ~11640) | Strips: `adrenaline_rush`, `adrenaline_rush_full`, `two_hand_quicken`, `one_hand_quicken`, `spear_quicken`, `cart_boost` |
| **Increase AGI blocked by Quagmire** | Increase AGI handler (line ~11545) | Fails silently if target has `quagmire` |
| **Quagmire strips speed buffs** | Ground effect tick (line ~28218) | Strips: `two_hand_quicken`, `spear_quicken`, `increase_agi`, `adrenaline_rush`, `loud_exclamation`, `improve_concentration` |
| **ASPD Haste2 group** | `getBuffModifiers()` (line ~694) | `song_assassin_cross`, `adrenaline_rush`, `two_hand_quicken`, `spear_quicken` — strongest wins via `haste2Bonus` |
| **Endow mutual exclusion** | Endow handler (line ~15256) | Removes all existing endows (skill + item + Aspersio + Enchant Poison) before applying new one |
| **Weapon swap cancels buffs** | Equipment handler (line ~23222) | Cancels `two_hand_quicken` (non-2H sword), `spear_quicken` (non-2H spear), `adrenaline_rush` (non-axe/mace) |
| **Shield removal cancels buffs** | Equipment handler (line ~23246) | Cancels `auto_guard`, `reflect_shield`, `defender`, `shrink` |
| **Weapon swap cancels performance** | Equipment handler (line ~23212) | Cancels Bard performance if not instrument, Dancer if not whip |
| **Provoke breaks Play Dead** | Provoke handler (line ~9837) | Removes `play_dead` from target |
| **Sage zones mutual exclusion** | Volcano/Deluge/VG handlers | Each caster can only have one elemental zone active |

### 2B. VIOLATIONS — Missing Mutual Exclusions

| ID | Pair | Expected Behavior (rAthena) | Current State | Severity |
|----|------|---------------------------|---------------|----------|
| ME-1 | **Blessing -> should remove Curse status** | Blessing on Cursed target should cure Curse AND apply buff (rAthena: EndOnStart SC_CURSE) | Blessing cures Curse via `cleanseStatusEffects(['curse', 'stone'])` but then RETURNS without applying the stat buff (line ~11496-11499). Per rAthena, Blessing should cure Curse AND then apply +STR/DEX/INT | **HIGH** |
| ME-2 | **Decrease AGI -> should remove Blessing** | rAthena `EndOnStart` for SC_DECREASEAGI includes SC_INCREASEAGI, but NOT SC_BLESSING. However, in practice Dec AGI and Blessing can coexist in pre-renewal. | Currently Dec AGI does NOT remove Blessing. This is CORRECT for pre-renewal. | **OK** |
| ME-3 | **Quagmire -> missing enemy buff strip** | Quagmire ground tick strips buffs from PLAYERS (line ~28218) but does NOT strip buffs from ENEMIES (line ~28195-28206) | Enemy targets only get the quagmire debuff applied, but speed buffs are not stripped | **MEDIUM** |
| ME-4 | **Dispel -> should remove Quagmire** | Quagmire is not in UNDISPELLABLE set, so Dispel should remove it. Since Quagmire is a ground effect that reapplies, Dispel removes the buff but it gets reapplied next tick if still standing in zone. | Functionally correct — Dispel removes the buff, ground effect reapplies. No issue. | **OK** |

---

## 3. BUFFS_SURVIVE_DEATH

### Current Set (line 2032-2036)
```javascript
const BUFFS_SURVIVE_DEATH = new Set([
    'auto_berserk', 'endure', 'shrink',
    'song_whistle', 'song_assassin_cross', 'song_bragi', 'song_apple_of_idun',
    'song_humming', 'dance_fortune_kiss', 'dance_service_for_you', 'dance_pdfm'
]);
```

### Verification vs rAthena pre-re/status.yml NoRemoveOnDead

| Buff | In BUFFS_SURVIVE_DEATH | rAthena NoRemoveOnDead | Match |
|------|----------------------|----------------------|-------|
| auto_berserk | YES | YES | OK |
| endure | YES | YES | OK |
| shrink | YES | YES | OK |
| song_whistle | YES | YES (song buffs persist) | OK |
| song_assassin_cross | YES | YES | OK |
| song_bragi | YES | YES | OK |
| song_apple_of_idun | YES | YES | OK |
| **song_humming** | YES | **WRONG NAME** | **BUG** |
| dance_fortune_kiss | YES | YES | OK |
| dance_service_for_you | YES | YES | OK |
| dance_pdfm | YES | YES | OK |

### VIOLATIONS

| ID | Issue | Details | Severity |
|----|-------|---------|----------|
| BSD-1 | **`song_humming` is wrong name — should be `dance_humming`** | The buff type is defined as `dance_humming` in `ro_buff_system.js` (line 403). The performance system uses `'humming': 'dance_humming'` mapping (index.js line 959). `BUFFS_SURVIVE_DEATH` uses `song_humming` which will NEVER match, so Humming buff is incorrectly cleared on death. | **HIGH** |
| BSD-2 | **Status effects not cleared on death** | `clearBuffsOnDeath()` only clears `activeBuffs` array. It does NOT clear `activeStatusEffects` Map (stun, freeze, poison, bleeding, etc.). In rAthena, `status_change_clear(BL_DEAD)` clears most status effects on death. A dead player could retain poison/bleeding DoT in the tick loop (though the `isDead` check at line 26819 skips dead players). | **LOW** (mitigated by isDead guard) |
| BSD-3 | **Missing surviving buffs** | rAthena NoRemoveOnDead also includes: `SC_WEIGHT50`, `SC_WEIGHT90`, `SC_EDP` (Enchant Deadly Poison), `SC_MAXIMIZEPOWER` (but toggle state IS cleared at line 2047). Some may not apply to pre-renewal scope. | **LOW** |

### Additional Concerns
- `clearBuffsOnDeath()` is called at **18+ death sites** across index.js, covering: auto-attack PvP, skill PvP kills (Bash, Bolt, Soul Strike, Napalm Beat, FireBall, Frost Diver, Thunderstorm, Magnum Break), status DoT death, monster skill death, monster AoE death, monster melee death, monster self-destruct, random target death, Abracadabra SA_DEATH, and respawn.
- Performance state and ensemble state are correctly cleared in `clearBuffsOnDeath()` (lines 2049-2055).
- Blacksmith toggle states (`maximizePowerActive`, `weaponBroken`) are correctly cleared (lines 2046-2048).

---

## 4. UNDISPELLABLE SET

### Main Dispel Handler (line 15317-15328)
```javascript
const UNDISPELLABLE = new Set([
    'hindsight', 'play_dead', 'auto_berserk', 'devotion_protection',
    'steel_body', 'combo',
    'stripweapon', 'stripshield', 'striparmor', 'striphelm',
    'cp_weapon', 'cp_shield', 'cp_armor', 'cp_helm',
    'chemical_protection_helm', 'chemical_protection_shield',
    'chemical_protection_armor', 'chemical_protection_weapon',
    'enchant_deadly_poison', 'cart_boost', 'meltdown',
    'safety_wall', 'dancing',
    'blade_stop_catching', 'root_lock', 'sitting',
    'eternal_chaos', 'lokis_veil', 'into_the_abyss',
    'ensemble_aftermath'
]);
```

### Abracadabra Dispel (line 16392-16394)
```javascript
const UNDISPELLABLE = new Set([
    'hindsight', 'play_dead', 'auto_berserk', 'wedding', 'riding', 'cart',
    'devotion_protection', 'steel_body', 'cp_weapon', 'cp_shield',
    'cp_armor', 'cp_helm',
    'chemical_protection_helm', 'chemical_protection_shield',
    'chemical_protection_armor', 'chemical_protection_weapon'
]);
```

### VIOLATIONS

| ID | Issue | Details | Severity |
|----|-------|---------|----------|
| UD-1 | **Strip buff names mismatch** | Main Dispel uses `stripweapon/stripshield/striparmor/striphelm` but `ro_buff_system.js` defines them as `strip_weapon/strip_shield/strip_armor/strip_helm` (with underscores). The UNDISPELLABLE check will NEVER match, so Dispel would incorrectly remove active strip debuffs. | **HIGH** |
| UD-2 | **Duplicate CP entries** | Both `cp_weapon` AND `chemical_protection_weapon` (etc.) are listed. Only one naming convention is actually used — this is harmless but adds confusion about which is canonical. Need to verify which name `applyBuff()` actually uses. | **LOW** |
| UD-3 | **Abracadabra UNDISPELLABLE is smaller** | Missing from Abracadabra's set: `combo`, `safety_wall`, `dancing`, `blade_stop_catching`, `root_lock`, `sitting`, `eternal_chaos`, `lokis_veil`, `into_the_abyss`, `ensemble_aftermath`, `meltdown`, `enchant_deadly_poison`, `cart_boost`. Abracadabra's Dispel can remove buffs that the main Dispel cannot. | **MEDIUM** |
| UD-4 | **`dancing` — never used as buff name** | The UNDISPELLABLE entry `'dancing'` does not correspond to any buff name in the system. Song buffs use names like `song_whistle`, `song_bragi`, etc. Performance cancellation is handled separately (line 15347). This entry has no effect. | **LOW** |
| UD-5 | **Dispel should cancel performance** | This IS handled correctly at line 15346-15354. Dispel cancels active performance AND ensemble via `cancelPerformance()` and `endEnsemble()`. | **OK** |
| UD-6 | **Song buffs missing from UNDISPELLABLE** | Active song buffs on OTHER players (from standing in a Bard/Dancer AoE) ARE dispellable — this is correct per rAthena. The song effect ground area remains and will reapply next tick. | **OK** |

---

## 5. BUFF TICK PROCESSING

### Tick Intervals

| Tick | Interval | Processes |
|------|----------|-----------|
| Status effect + buff expiry | 1000ms | Status expiry, DoT drains (poison/bleeding/stone), buff expiry, Hiding SP drain, Cloaking SP drain, spirit sphere expiry, combo window expiry |
| HP regen | 6000ms | Natural HP regen (VIT-based), equipment regen rate bonus |
| SP regen | 8000ms | Natural SP regen (INT-based), Magnificat 2x multiplier |
| Skill regen | 10000ms | Inc HP Recovery (+5/lv), Inc SP Recovery (+3/lv), card regen bonuses |
| Ground effect | 250ms | Fire Wall, Storm Gust, LoV, Meteor Storm waves, Quagmire debuff, Volcano/Deluge/VG zone buffs, Ice Wall decay |
| Combat auto-attack | 50ms (COMBAT_TICK_MS) | ASPD-based attack timing |
| Enemy AI | 500ms | Monster behavior, aggro, skill casting |
| Performance SP drain | (within performance tick) | 1 SP per (10-Lv) seconds per performance |

### DoT Processing Details

| Status | Interval | Damage | Floor | Kill? |
|--------|----------|--------|-------|-------|
| poison | 1s | 1.5% MaxHP + 2 flat | 1 HP | No |
| bleeding | 4s | 2% MaxHP | 0 HP | Yes |
| stone | 5s (after 3s delay) | 1% MaxHP | 25% MaxHP | No |

### Tick Processing Concerns

| ID | Issue | Details | Severity |
|----|-------|---------|----------|
| TP-1 | **Slow Poison implementation** | Slow Poison works by UNDOING the drain after it happens (line 26839-26842: `player.health = Math.min(player.maxHealth, player.health + d.drain)`). This means the drain is applied then reversed, which could cause a 1-tick flicker of lower HP on the client since `tickStatusEffects()` already modified `player.health`. | **LOW** (cosmetic) |
| TP-2 | **Bleeding breaks Play Dead** | Correctly implemented at line 26856-26860. Bleeding HP drain removes Play Dead buff. | **OK** |
| TP-3 | **Death from DoT** | Correctly handled — `clearBuffsOnDeath()` called, death penalty applied, auto-attacks cleared (line 26863-26875). | **OK** |

---

## 6. STATS RECALCULATION AFTER BUFF APPLICATION

For each buff application, I checked whether `getEffectiveStats()` + `roDerivedStats()` + `player:stats` emission follows.

### Correctly Recalculated

| Buff | Stats Emitted | Notes |
|------|--------------|-------|
| Blessing | YES (line 11517-11518) | Full stats payload to buff target |
| Increase AGI | YES (line 11564-11568) | Full stats payload to buff target |
| Auto Berserk toggle | YES (line 1662-1666, 1680-1684) | Stats on activate AND deactivate |
| Auto Guard on/off | YES (lines 14088, 14116) | Stats for toggle on AND toggle off |
| Defender on/off | YES (lines 14169, 14197) | Stats for toggle on AND toggle off |
| THQ / Spear Quicken | YES (checked via equipment handler) | Stats emitted after equip changes |
| Buff expiry (ANY) | YES (line 26919-26933) | All expired buffs trigger stats recalc |
| Equipment change | YES (line 23255+) | After all buff cancellations |
| Endow application | Partial | Weapon element set directly on player |
| Mount toggle | YES (line 7909-7911) | ASPD recalc |

### VIOLATIONS

| ID | Issue | Details | Severity |
|----|-------|---------|----------|
| SR-1 | **Provoke on player — no stats emission** | When Provoke lands on a PLAYER target (PvP), no `player:stats` is emitted to the affected player. Their ATK/DEF changes are invisible to their stats window until next natural stats refresh. The Provoke handler ends at line 9862-9863 with only `skill:used` and `combat:health_update` to the CASTER, not the target. | **HIGH** |
| SR-2 | **Decrease AGI on player — no stats emission** | Decrease AGI on a player target does NOT emit `player:stats` to the debuffed player. AGI/moveSpeed/ASPD changes are invisible. Handler ends at line ~11675 with only caster emissions. | **HIGH** |
| SR-3 | **Angelus — no stats emission** | Angelus `applyBuff()` at line 11742 is followed only by `skill:buff_applied`, `skill:used`, and `combat:health_update`. No `player:stats` emission. DEF% change is invisible to stats window. | **MEDIUM** |
| SR-4 | **Endure — no stats emission** | Endure applies +MDEF but does not emit `player:stats`. MDEF change invisible to stats window. | **MEDIUM** |
| SR-5 | **Magnum Break fire buff — no stats emission** | The 10s fire endow buff does not trigger stats recalc. Minor since it's a temporary combat modifier, not a persistent stat change. | **LOW** |
| SR-6 | **Quagmire ground debuff — no stats emission** | When Quagmire applies its debuff to a player in the ground tick (line 28226-28231), no `player:stats` is emitted. AGI/DEX/moveSpeed changes are invisible. | **MEDIUM** |
| SR-7 | **Improve Concentration — no stats emission** | IC applies +AGI and +DEX but the handler at line 12316 does not emit `player:stats`. | **MEDIUM** |
| SR-8 | **Signum Crucis — no stats emission to targets** | AoE DEF reduction debuff applied to enemies + players, but no `player:stats` emitted to affected players. | **MEDIUM** |

---

## 7. PERFORMANCE INTERACTIONS

### Cancel Triggers (all verified present)

| Trigger | Handler | Status |
|---------|---------|--------|
| Death | `clearBuffsOnDeath()` (line 2050-2051) | OK |
| Disconnect | `disconnect` handler (line 7262) | OK |
| Skill reset | `skills:reset` handler (line 8315-8316) | OK |
| Dispel | Dispel handler (line 15347-15354) | OK |
| Weapon swap (wrong type) | Equipment handler (line 23211-23213) | OK |
| Heavy damage (>25% MaxHP) | `checkPerformanceDamageCancel()` (line 1691-1694) | OK |
| Adaptation skill | Handled in skill:use | OK |
| SP depletion | Performance tick | OK |

### Performance Skill Blocking (rAthena AllowWhenPerforming)

| Allowed During Performance | Blocked |
|---------------------------|---------|
| Adaptation, Musical Strike, Slinging Arrow, Starting new performance | All other skills (Frost Joker, Scream, Pang Voice, etc.) |

This is correctly implemented at line 9216-9223.

### Caster Exclusion

Song/dance buff caster exclusion is handled in the performance tick — the caster's own buff has a separate check. Verified in ro_buff_system.js getBuffModifiers().

---

## 8. COMPREHENSIVE VIOLATION SUMMARY

### CRITICAL (data integrity / functionality broken)

| ID | Category | Description | Fix |
|----|----------|-------------|-----|
| **BSD-1** | BUFFS_SURVIVE_DEATH | `song_humming` should be `dance_humming` — Humming buff incorrectly cleared on death | Change `'song_humming'` to `'dance_humming'` in BUFFS_SURVIVE_DEATH set (line 2035) |
| **UD-1** | UNDISPELLABLE | Strip debuff names use no underscore (`stripweapon`) but actual buff names use underscore (`strip_weapon`) — Dispel can remove active strip debuffs when it should not | Change to `'strip_weapon', 'strip_shield', 'strip_armor', 'strip_helm'` in UNDISPELLABLE set (line 15320) |

### HIGH (stat display / combat accuracy)

| ID | Category | Description | Fix |
|----|----------|-------------|-----|
| **SR-1** | Stats Recalc | Provoke on player target — no `player:stats` emission | Add stats recalc + emit to provoked player after applyBuff |
| **SR-2** | Stats Recalc | Decrease AGI on player target — no `player:stats` emission | Add stats recalc + emit to debuffed player |
| **ME-1** | Mutual Exclusion | Blessing cures Curse/Stone but then returns WITHOUT applying the stat buff. Per rAthena, Blessing should cure Curse AND then apply +STR/DEX/INT. The current `return` at line 11499 prevents the buff from being applied. | Remove the early return after cleanse — let execution flow to PATH 3 buff application |

### MEDIUM

| ID | Category | Description | Fix |
|----|----------|-------------|-----|
| **ME-3** | Mutual Exclusion | Quagmire ground tick strips buffs from players but not enemies | Add same strip logic for enemies in quagmire tick |
| **UD-3** | UNDISPELLABLE | Abracadabra's UNDISPELLABLE set is missing 13 entries vs main Dispel | Unify into a shared constant |
| **SR-3** | Stats Recalc | Angelus — no stats emission after DEF% change | Add `player:stats` emission |
| **SR-4** | Stats Recalc | Endure — no stats emission after MDEF change | Add `player:stats` emission |
| **SR-6** | Stats Recalc | Quagmire debuff on player — no stats emission | Add `player:stats` emission in ground tick |
| **SR-7** | Stats Recalc | Improve Concentration — no stats emission after AGI/DEX change | Add `player:stats` emission |
| **SR-8** | Stats Recalc | Signum Crucis — no stats emission to affected players | Add `player:stats` emission per affected player |

### LOW

| ID | Category | Description |
|----|----------|-------------|
| BSD-2 | BUFFS_SURVIVE_DEATH | Status effects (activeStatusEffects) not cleared on death — mitigated by isDead guard |
| BSD-3 | BUFFS_SURVIVE_DEATH | Some rAthena surviving buffs not included (Weight, EDP) |
| UD-2 | UNDISPELLABLE | Duplicate CP naming (cp_X and chemical_protection_X) |
| UD-4 | UNDISPELLABLE | `dancing` entry never matches any buff name |
| TP-1 | Tick Processing | Slow Poison undo-drain may cause 1-tick HP flicker |
| SR-5 | Stats Recalc | Magnum Break fire buff — no stats emission (minor) |

---

## 9. RECOMMENDATIONS

### Priority 1: Fix Critical Bugs (2 items)

1. **Fix `BUFFS_SURVIVE_DEATH` naming** (line 2035):
   ```javascript
   // BEFORE:
   'song_humming', 'dance_fortune_kiss', ...
   // AFTER:
   'dance_humming', 'dance_fortune_kiss', ...
   ```

2. **Fix `UNDISPELLABLE` strip buff names** (line 15320):
   ```javascript
   // BEFORE:
   'stripweapon', 'stripshield', 'striparmor', 'striphelm',
   // AFTER:
   'strip_weapon', 'strip_shield', 'strip_armor', 'strip_helm',
   ```

### Priority 2: Fix High-Severity Issues (3 items)

3. **Provoke player stats emission** — after `applyBuff(target, buffDef)` at line 9834, add:
   ```javascript
   if (!isEnemy) {
       const pvkEffStats = getEffectiveStats(target);
       const pvkDerived = roDerivedStats(pvkEffStats);
       const pvkSock = io.sockets.sockets.get(target.socketId);
       if (pvkSock) pvkSock.emit('player:stats', buildFullStatsPayload(targetId, target, pvkEffStats, pvkDerived, ...));
   }
   ```

4. **Decrease AGI player stats emission** — after debuff application, add same pattern for player target.

5. **Blessing cure + buff** — remove the early `return` at line 11499 so that after curing Curse/Stone, execution flows to PATH 3 and applies the stat buff. This matches rAthena behavior where Blessing both cures and buffs.

### Priority 3: Unify UNDISPELLABLE (1 item)

6. **Extract UNDISPELLABLE to module constant** — define once at top of file (or in `ro_buff_system.js`) and reuse in both main Dispel handler and Abracadabra Dispel, ensuring consistency.

### Priority 4: Add Missing Stats Emissions (5 items)

7. Add `player:stats` emission after: Angelus, Endure, Improve Concentration, Quagmire ground tick, Signum Crucis (for player targets).

### Priority 5: Minor Fixes

8. Add Quagmire buff stripping for enemies (parity with player stripping).
9. Clean up `dancing` entry in UNDISPELLABLE (dead entry).
10. Resolve CP buff naming convention (pick one: `cp_weapon` or `chemical_protection_weapon`).

---

## 10. ARCHITECTURE NOTES

### What Works Well
- **Dual-layer system** (`activeStatusEffects` Map + `activeBuffs` array) cleanly separates CC/DoT from stat buffs
- **getBuffModifiers() + getStatusModifiers() -> getCombinedModifiers()** provides clean modifier aggregation
- **Haste2 exclusion group** correctly implements strongest-wins for ASPD buffs
- **Performance cancel hooks** are comprehensive (7 triggers)
- **Endow mutual exclusion** covers all 11 endow types (skill + item + Aspersio + Enchant Poison)
- **Equipment change buff cancellation** covers weapon type + shield dependency
- **Buff expiry tick** correctly recalculates stats and emits `player:stats` (line 26919-26933)
- **clearBuffsOnDeath()** called at all 18+ death sites

### Systemic Pattern Concern
The stats recalculation violations (SR-1 through SR-8) reveal a **systemic pattern**: buff application code does not consistently emit `player:stats` after stat-affecting buffs. The expiry path handles this correctly (single handler at line 26919), but application paths are scattered across 30+ skill handlers with no enforced pattern. Consider wrapping buff application in a helper that auto-emits stats when needed.

---

*Audit complete. 2 CRITICAL + 3 HIGH + 7 MEDIUM + 6 LOW violations identified across 89 buff/debuff types.*
