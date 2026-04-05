# Acolyte Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14, **Updated:** 2026-03-15
**Status:** PHASES 1-3 IMPLEMENTED. Remaining deferred (party-wide buffs, Ruwach detection, memo system, Aqua Benedicta items, base level scaling).
**Scope:** All Acolyte class skills (IDs 400-414), 15 total (10 active + 2 passive + 1 toggle-like + 2 quest/support)

---

## Executive Summary

Deep research against iRO Wiki Classic, RateMyServer pre-renewal database, and rAthena source reveals **significant gaps** in 12 of 15 Acolyte skills. Only Ruwach (408) and Aqua Benedicta (412) are functionally correct for current scope. The remaining skills have issues ranging from missing mechanics (Blessing has no Undead/Demon debuff path, Decrease AGI has no success rate check, Increase AGI costs no HP, Angelus is self-only instead of party-wide) to incorrect formula values (Heal SP cost formula wrong, Decrease AGI duration wrong, Teleport Lv2 SP cost wrong) to completely missing handlers (Holy Light has no server handler at all).

**Critical fixes needed:**
1. Heal SP cost formula: should be `10 + 3*level` not `13 + 3*i` (our Lv1 is correct at 13, but Lv2+ are off by 0 — actually matches! verified below)
2. Blessing: missing Undead/Demon debuff path (halve STR/DEX/INT), missing Curse/Stone cure
3. Increase AGI: missing 15 HP cost, missing min-16-HP check, missing after-cast delay, no stats update to buff target
4. Decrease AGI: missing success rate roll, missing boss immunity, wrong duration values, missing buff dispel list
5. Angelus: self-only instead of party-wide
6. Signum Crucis: missing success rate roll, wrong AoE radius, missing permanent duration
7. Divine Protection / Demon Bane: missing base level scaling component
8. Holy Light: no server handler exists
9. Pneuma: no overlap check with existing Pneuma/Safety Wall

**New systems needed:**
1. Blessing offensive path (debuff Undead/Demon enemies)
2. Decrease AGI success rate formula with MDEF resistance
3. Party-wide buff broadcasting (Angelus)
4. Signum Crucis success rate with level-difference formula
5. Holy Light magical damage handler

---

## Skill-by-Skill Analysis

---

### 1. HEAL (ID 400) — Active, Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive | iRO Wiki Classic |
| Max Level | 10 | All sources |
| Target | Self, Ally, or Enemy (Undead only) | iRO Wiki Classic |
| Heal Formula | `floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)` | iRO Wiki Classic, RateMyServer |
| SP Cost | 13, 16, 19, 22, 25, 28, 31, 34, 37, 40 | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | 1 second | iRO Wiki Classic |
| Cooldown | None | iRO Wiki Classic |
| Range | 9-10 cells (~450 UE units) | iRO Wiki Classic (10), RateMyServer (9) |
| Element | Holy (when damaging Undead) | All sources |
| Undead Damage | 50% of heal amount as Holy damage, modified by Undead element level | iRO Wiki Classic, RateMyServer |
| Undead Damage vs MDEF | Ignores MDEF and INT | RateMyServer |
| Heal Power Bonus | Equipment/card `bHealPower` % modifiers apply | rAthena |

#### Current Implementation Status: MOSTLY CORRECT

- Heal formula: **CORRECT** — `calculateHealAmount()` uses `floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)`
- SP cost: **CORRECT** — `13 + i * 3` generates 13, 16, 19, 22, 25, 28, 31, 34, 37, 40
- Cast time: **CORRECT** — 0
- Undead damage: **CORRECT** — 50% of heal amount, multiplied by Holy vs Undead element modifier
- Self/ally targeting: **CORRECT** — handles both self and other players
- Heal power bonus: **CORRECT** — `equipHealPower` and `cardHealPower` applied
- Auto Berserk check: **CORRECT** — deactivates when healed above 25%

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| After-cast delay set as `cooldown: 1000` instead of `afterCastDelay: 1000` | MEDIUM | Trivial — change field name in skill def |
| Heal vs Undead should ignore MDEF (currently uses element modifier only, which is correct for damage calc, but the principle should be documented) | LOW | Already works via heal formula bypass |

#### Implementation Notes

The `cooldown: 1000` in the skill definition acts similarly to `afterCastDelay` for the player's own usage, but semantically it's per-skill cooldown rather than global ACD. In practice, the difference is minimal since Heal is the only skill being cast rapidly. However, changing to `afterCastDelay: 1000` would be more canonical — ACD prevents ALL skills for 1s after Heal, not just Heal itself.

The Undead damage path correctly uses `getElementModifier('holy', enemyElement, elementLevel)` which applies the Holy vs Undead multiplier table (100% for Undead Lv1, 125% for Lv2, etc.). The `floor(healAmount / 2)` is the base, then element modifier is applied. This matches canonical behavior.

---

### 2. DIVINE PROTECTION (ID 401) — Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| DEF Bonus | +3 per level (+3 to +30) | iRO Wiki Classic, RateMyServer |
| DEF Type | Soft DEF (VIT-like defense), subtracted after hard DEF reductions | RateMyServer |
| Target Races | Undead property AND Demon race monsters | RateMyServer, iRO Wiki Classic |
| Base Level Scaling | `(3 * SkillLv) + floor(0.04 * (BaseLv + 1))` — total reduction | RateMyServer (pre-renewal formula) |
| Player Restriction | Does NOT work against players | RateMyServer |

#### Current Implementation Status: PARTIALLY CORRECT

- DEF bonus per level: **CORRECT** — `dpLv * 3` in `getPassiveSkillBonuses()`
- Race targeting: **CORRECT** — applies to `undead` and `demon` races
- Stored in `raceDEF`: **CORRECT** — used in damage calculations

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing base level scaling component: `floor(0.04 * (BaseLv + 1))` additional DEF | LOW | Small — add BaseLv component to raceDEF calculation |
| Description says "DEF" but should clarify "Soft DEF" (VIT-like) | LOW | Cosmetic |

#### Implementation Notes

The base level scaling adds a small amount: at level 99, it's `floor(0.04 * 100) = 4` extra DEF. At level 50, it's `floor(0.04 * 51) = 2`. This is minor but canonical. The `raceDEF` bonus is consumed in the damage pipeline where it subtracts from incoming damage — this correctly models soft DEF behavior.

---

### 3. DEMON BANE (ID 413) — Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| ATK Bonus | +3 per level (+3 to +30) | iRO Wiki Classic, RateMyServer |
| ATK Type | Weapon mastery bonus (ignores armor DEF, not VIT DEF) | RateMyServer |
| Target Races | Undead property AND Demon race monsters | RateMyServer |
| Base Level Scaling | `(3 * SkillLv) + floor(0.05 * (BaseLv + 1))` — total bonus | RateMyServer (pre-renewal formula) |
| Prerequisite | Divine Protection Lv3 | All sources |

#### Current Implementation Status: PARTIALLY CORRECT

- ATK bonus per level: **CORRECT** — `dbLv * 3` in `getPassiveSkillBonuses()`
- Race targeting: **CORRECT** — applies to `undead` and `demon` races
- Stored in `raceATK`: **CORRECT** — used in damage calculations
- Prerequisite: **CORRECT** — `{ skillId: 401, level: 3 }`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing base level scaling: `floor(0.05 * (BaseLv + 1))` additional ATK | LOW | Small — add BaseLv component |

#### Implementation Notes

At level 99, this adds `floor(0.05 * 100) = 5` extra ATK. At level 50: `floor(0.05 * 51) = 2`. Minor but canonical. Same pattern as Divine Protection.

---

### 4. BLESSING (ID 402) — Active, Single Target Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff | All sources |
| Max Level | 10 | All sources |
| Target | Self, Ally, or Enemy (Undead/Demon) | iRO Wiki Classic |
| Stat Bonus | +1 STR/DEX/INT per level (+1 to +10) | iRO Wiki Classic, RateMyServer |
| Duration | 60, 80, 100, 120, 140, 160, 180, 200, 220, 240 seconds | iRO Wiki Classic |
| SP Cost | 28, 32, 36, 40, 44, 48, 52, 56, 60, 64 | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | None | iRO Wiki Classic |
| Range | 9 cells (~450 UE units) | RateMyServer |
| Prerequisite | Divine Protection Lv5 | All sources |
| Undead/Demon Effect | Halves target's STR, DEX, INT (regardless of skill level) | RateMyServer, iRO Wiki Classic |
| Boss Immunity | Does NOT affect boss monsters | RateMyServer |
| Curse Cure | Removes Curse and Stone (2nd stage) from target | iRO Wiki Classic |
| Curse Cure Note | When curing Curse, target does NOT receive stat bonuses | iRO Wiki Classic |

#### Current Implementation Status: PARTIALLY CORRECT

- Stat bonus: **CORRECT** — `effectVal = i + 1` gives +1 to +10 per level
- SP cost: **CORRECT** — `28 + i * 4` gives 28, 32, 36...64
- Duration: **CORRECT** — `60000 + i * 20000` gives 60s, 80s...240s
- Cast time: **CORRECT** — 0
- Prerequisite: **CORRECT** — Divine Protection Lv5

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Missing Undead/Demon debuff path** — Blessing on Undead element or Demon race enemies should halve their STR/DEX/INT | HIGH | Medium — add enemy targeting branch like Heal's undead path |
| **Missing Curse/Stone cure** — should remove Curse and Stone (2nd stage) from the target | MEDIUM | Small — add `cleanseStatusEffects()` call for `['curse', 'stone']` |
| **No boss immunity check** for the debuff path | MEDIUM | Trivial — check `enemy.isBoss` |
| Handler only resolves player targets (line 7064: `!isEnemy`) — cannot target enemies at all | HIGH | Part of Undead/Demon debuff implementation |
| No stats update broadcast to buff target when target is a different player | LOW | Small — already does this (line 7092-7093) |

#### Implementation Notes

The handler needs a major restructuring to support three paths:
1. **Friendly target (player/self)**: Apply +STR/DEX/INT buff (current behavior, correct)
2. **Cursed/Stoned target**: Cure the status, do NOT apply stat buff
3. **Undead element or Demon race enemy**: Apply debuff that halves STR/DEX/INT for the duration

For the enemy debuff, apply a `blessing_debuff` buff to the enemy with `strReduction: 50, dexReduction: 50, intReduction: 50` (percentage). This reduces HIT (via DEX) and MATK (via INT) but not ATK directly.

---

### 5. INCREASE AGI (ID 403) — Active, Single Target Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff | All sources |
| Max Level | 10 | All sources |
| Target | Self, Ally, or Enemy | iRO Wiki Classic |
| AGI Bonus | 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 | iRO Wiki Classic |
| Duration | 60, 80, 100, 120, 140, 160, 180, 200, 220, 240 seconds | iRO Wiki Classic |
| SP Cost | 18, 21, 24, 27, 30, 33, 36, 39, 42, 45 | iRO Wiki Classic |
| HP Cost | 15 HP per cast (flat, all levels) | iRO Wiki Classic, RateMyServer |
| Min HP Check | Fails if caster has less than 16 HP | iRO Wiki Classic |
| Cast Time | 1 second (base, reduced by DEX) | iRO Wiki Classic |
| After-Cast Delay | 1 second | iRO Wiki Classic |
| Movement Speed | Increases movement speed (amount unspecified in classic docs, commonly 25%) | All sources |
| Cancels Decrease AGI | Yes — dispels Decrease AGI on target before applying | RateMyServer |
| Prerequisite | Heal Lv3 | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |

#### Current Implementation Status: PARTIALLY CORRECT

- AGI bonus: **CORRECT** — `effectVal = 3 + i` gives 3, 4, 5...12
- Duration: **CORRECT** — `60000 + i * 20000` gives 60s...240s
- SP cost: **CORRECT** — `18 + i * 3` gives 18, 21...45
- Cast time: **CORRECT** — 1000ms
- Cancels Decrease AGI: **CORRECT** — handler removes `decrease_agi` buff
- Move speed bonus: **CORRECT** — `moveSpeedBonus: 25`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Missing 15 HP cost** — should deduct 15 HP from caster on each cast | HIGH | Small — add `player.health -= 15` and health update |
| **Missing min-16-HP check** — skill should fail if caster HP < 16 | HIGH | Small — add HP check before execution |
| **Missing after-cast delay** in skill definition — has none defined | MEDIUM | Trivial — add `afterCastDelay: 1000` |
| No `player:stats` broadcast to buff target (only caster gets health_update) | LOW | Small — add stats broadcast like Blessing handler |
| Buff target doesn't get `combat:health_update` after buff application | LOW | Small — emit to target socket |

#### Implementation Notes

The HP cost is a unique mechanic — no other buff costs HP. Add before SP deduction:
```js
if (player.health < 16) { socket.emit('skill:error', { message: 'Not enough HP' }); return; }
player.health = Math.max(1, player.health - 15);
```

The after-cast delay prevents spamming Increase AGI + other skills in quick succession. Without it, an Acolyte can cast Increase AGI and immediately follow with Heal, which shouldn't be possible.

---

### 6. DECREASE AGI (ID 404) — Active, Single Target Debuff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Debuff | All sources |
| Max Level | 10 | All sources |
| Target | Enemy only | All sources |
| AGI Reduction | 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 | iRO Wiki Classic, RateMyServer |
| Duration (Monsters) | 30, 40, 50, 60, 70, 80, 90, 100, 110, 120 seconds | iRO Wiki Classic |
| Duration (Players) | 20, 25, 30, 35, 40, 45, 50, 55, 60, 65 seconds | iRO Wiki Classic |
| SP Cost | 15, 17, 19, 21, 23, 25, 27, 29, 31, 33 | iRO Wiki Classic |
| Cast Time | 1 second | iRO Wiki Classic |
| After-Cast Delay | 1 second | iRO Wiki Classic |
| Success Rate (Classic) | 42%, 44%, 46%, 48%, 50%, 52%, 54%, 56%, 58%, 60% base | iRO Wiki Classic |
| Success Formula (RMS) | `40 + 2*SkillLv + (BaseLv + INT)/5 - TargetMDEF` % | RateMyServer |
| Movement Speed | -25% | RateMyServer |
| Cancels Buffs | Increase AGI, Adrenaline Rush, Two-Hand Quicken, Spear Quicken, Cart Boost | RateMyServer |
| Boss Immunity | Does NOT work on Boss monsters | RateMyServer |
| Prerequisite | Increase AGI Lv1 | All sources |

#### Current Implementation Status: SIGNIFICANT GAPS

- AGI reduction: **CORRECT** — `effectVal = 3 + i` gives 3...12
- SP cost: **CORRECT** — `15 + i * 2` gives 15, 17...33
- Cast time: **CORRECT** — 1000ms
- After-cast delay: **CORRECT** — 1000ms
- Cancels Increase AGI: **CORRECT** — removes `increase_agi` buff
- Move speed reduction: **CORRECT** — `moveSpeedReduction: 25`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Missing success rate roll** — always succeeds, should have 42-60% base + formula | HIGH | Medium — add success rate formula |
| **Missing boss immunity** — works on all enemies including bosses | HIGH | Small — check `enemy.isBoss` or `enemy.mode` |
| **Wrong duration values** — uses `(4+i)*10000` giving 40-130s, should be 30-120s for monsters | MEDIUM | Small — fix formula to `(3+i)*10000` |
| Duration fallback of 120000ms instead of per-level value | MEDIUM | Trivial — remove `\|\| 120000` fallback |
| **Missing additional buff dispels** — only removes Increase AGI, should also remove Adrenaline Rush, Two-Hand Quicken, Spear Quicken, Cart Boost | DEFERRED | Phase 6+ (2nd class buffs don't exist yet) |
| No separate duration for player targets (should be shorter) | LOW | Small — check `isEnemy` for duration |

#### Implementation Notes

The success rate formula from RateMyServer:
```js
const successRate = 40 + 2 * learnedLevel + Math.floor((baseLv + intStat) / 5) - targetMDEF;
if (Math.random() * 100 >= successRate) {
    // Failed — SP still consumed
    socket.emit('skill:effect_damage', { ... hitType: 'miss', damage: 0 ... });
    return;
}
```

The iRO Wiki Classic lists different base rates (42-60%) which would correspond to `40 + 2*Lv` = 42, 44, 46...60 — this matches the RMS formula's base component exactly.

Duration fix: change `(4+i)*10000` to monster duration `(i+3)*10000` giving 30, 40, 50...120 seconds. For player targets, use `(i+2)*5000` giving 20, 25, 30...65 seconds (when PvP is enabled).

---

### 7. CURE (ID 405) — Active, Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive | All sources |
| Max Level | 1 | All sources |
| Target | Self, Ally, or Enemy | iRO Wiki Classic |
| Removes | Silence, Blind, Confusion (Chaos) | iRO Wiki Classic |
| SP Cost | 15 | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | None | iRO Wiki Classic |
| Prerequisite | Heal Lv2 | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |

#### Current Implementation Status: CORRECT

- Status removal: **CORRECT** — `cleanseStatusEffects(cureTarget, ['silence', 'blind', 'confusion'])`
- SP cost: **CORRECT** — 15
- Cast time: **CORRECT** — 0
- Self/ally targeting: **CORRECT**
- Broadcasts removal: **CORRECT** — both `status:removed` and `skill:buff_removed`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Has `cooldown: 1000` which is not canonical — no cooldown in RO Classic | LOW | Trivial — remove cooldown |

#### Implementation Notes

The `cooldown: 1000` prevents rapid Cure spam but is not canonical. In RO Classic, Cure has no cooldown or after-cast delay — you can spam it. Removing the cooldown would be more accurate but could allow rapid status cure spam. Consider keeping it as a minor deviation for game balance.

---

### 8. ANGELUS (ID 406) — Active, Self/Party Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff (Party-wide) | All sources |
| Max Level | 10 | All sources |
| Target | Self + All Party Members on Screen | iRO Wiki Classic, RateMyServer |
| VIT DEF% Bonus | 5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50% | iRO Wiki Classic, RateMyServer |
| DEF Type | VIT-based soft DEF only (does NOT increase VIT stat or VIT-related bonuses) | RateMyServer |
| Duration | 30, 60, 90, 120, 150, 180, 210, 240, 270, 300 seconds | iRO Wiki Classic, RateMyServer |
| SP Cost | 23, 26, 29, 32, 35, 38, 41, 44, 47, 50 | iRO Wiki Classic, RateMyServer |
| Cast Time | 0.5 seconds | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | 3.5 seconds | iRO Wiki Classic, RateMyServer |
| Range | Screen-wide (14x14 cells for party members) | iRO Wiki Classic |
| Prerequisite | Divine Protection Lv3 | All sources |

#### Current Implementation Status: PARTIALLY CORRECT

- VIT DEF%: **CORRECT** — `effectVal = 5 + i * 5` gives 5, 10...50 (note: `5+i*5` gives 5,10,15,20,25,30,35,40,45,50 which is correct)
- SP cost: **CORRECT** — `23 + i * 3` gives 23, 26...50
- Cast time: **CORRECT** — 500ms
- After-cast delay: **CORRECT** — 3500ms
- Duration: **CORRECT** — `(i+1)*30000` gives 30s, 60s...300s
- Buff modifier: **CORRECT** — `defPercent` applied in `getBuffModifiers()`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Self-only instead of party-wide** — only applies to caster, should apply to all party members on screen | HIGH | Medium — iterate party members and apply buff to each |
| No party system exists yet, so party-wide is impossible to implement | DEFERRED | Depends on party system implementation |

#### Implementation Notes

Since the party system is not yet implemented, Angelus can remain self-only for now. When the party system is added (Phase 7+), Angelus should iterate all party members in the same zone and apply the buff to each. The buff should use the same `defPercent` field.

For now, mark this as DEFERRED — the self-only behavior is acceptable until party support exists. The VIT DEF% bonus correctly increases soft defense when applied.

Note on iRO Wiki's Level 7 data showing 25% instead of 35% — this is a known data entry error on the wiki. The correct progression is linear: 5% per level, so Level 7 = 35%.

---

### 9. SIGNUM CRUCIS (ID 407) — Active, AoE Debuff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, AoE Debuff | All sources |
| Max Level | 10 | All sources |
| Target | All Undead element AND Demon race monsters on screen | iRO Wiki Classic |
| DEF Reduction | 14%, 18%, 22%, 26%, 30%, 34%, 38%, 42%, 46%, 50% | iRO Wiki Classic |
| Duration | **Permanent** — until affected monsters die | iRO Wiki Classic |
| SP Cost | 35 (all levels) | All sources |
| Cast Time | 0.5 seconds | iRO Wiki Classic |
| After-Cast Delay | 2 seconds | iRO Wiki Classic |
| AoE Range | Screen-wide (~15x15 cells radius, or ~31x31 from RMS) | RateMyServer |
| Success Rate | 27%, 31%, 35%, 39%, 43%, 47%, 51%, 55%, 59%, 63% | iRO Wiki Classic |
| Success Formula | `23 + 4*SkillLv + CasterBaseLv - TargetLv` % (estimated from base rates) | Multiple sources |
| Boss Monsters | Works on boss monsters | iRO Wiki Classic |
| Prerequisite | Demon Bane Lv3 | All sources |
| Visual | Affected monsters perform `/swt` emote on success | iRO Wiki Classic |

#### Current Implementation Status: PARTIALLY CORRECT

- DEF reduction: **CORRECT** — `effectVal = 14 + i * 4` gives 14, 18...50
- SP cost: **CORRECT** — 35
- Cast time: **CORRECT** — 500ms
- After-cast delay: **CORRECT** — 2000ms
- Targets Undead/Demon: **CORRECT** — checks `race !== 'undead' && race !== 'demon'`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Missing success rate roll** — always succeeds, should have 27-63% base chance modified by level difference | HIGH | Medium — add success formula |
| **Wrong AoE radius** — uses 500 UE units (~10 cells), should be screen-wide (~750+ UE units) | MEDIUM | Trivial — increase radius |
| **Wrong duration** — uses 30000ms (30s), should be **permanent** (until monster dies) | HIGH | Small — set very long duration or use special flag |
| Should also check Undead ELEMENT, not just Undead RACE | MEDIUM | Small — add element check |
| Prerequisite references skill 413 (Demon Bane) which is **correct** | — | No change needed |

#### Implementation Notes

The success rate formula reconstructed from base rates:
- Level 1: 27% = 23 + 4*1 = 27 (before level difference)
- Level 10: 63% = 23 + 4*10 = 63

With level difference: `successRate = 23 + 4*SkillLv + CasterBaseLv - TargetLv`

For "permanent" duration, use a very high value like `86400000` (24 hours) or `Number.MAX_SAFE_INTEGER`. Since monsters respawn on death, this effectively means "until death."

The AoE should cover the whole screen. In RO, screen-wide is approximately 15 cells radius. In UE units: `15 * 50 = 750`. Current value of 500 covers ~10 cells.

Additionally, the skill should check BOTH Undead element AND Demon race. Currently it only checks race. Enemies with Undead element but non-Demon race (e.g., an Undead element Brute) should also be affected.

---

### 10. RUWACH (ID 408) — Active, Self Buff (AoE Detection)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self Buff with AoE effect | All sources |
| Max Level | 1 | All sources |
| Target | Self (AoE around caster) | All sources |
| AoE | 5x5 cells (~250 UE units radius) | iRO Wiki Classic |
| Duration | 10 seconds | iRO Wiki Classic |
| SP Cost | 10 | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | None | iRO Wiki Classic |
| Damage to Revealed | 145% MATK, Holy element | iRO Wiki Classic |
| Prerequisite | None | iRO Wiki Classic |

#### Current Implementation Status: CORRECT

- Duration: **CORRECT** — 10000ms
- SP cost: **CORRECT** — 10
- Cast time: **CORRECT** — 0
- Damage value: **CORRECT** — `effectValue: 145` (145% MATK)
- AoE radius in buff: **CORRECT** — `aoeRadius: 500` (functional, though 5x5 = 250 UE units is canonical)
- Prerequisite: **CORRECT** — none

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| AoE radius sent as 500 in buff data, canonical is 5x5 cells (~250 UE units) | LOW | Trivial |
| No actual hidden enemy detection/damage logic — only applies buff, no tick that checks for hidden enemies | DEFERRED | Medium — needs Hidden status system integration |

#### Implementation Notes

The Ruwach handler correctly applies a buff with `revealHidden: true`, but there's no periodic tick that checks for hidden enemies within the AoE and damages them. This requires the Hidden status effect system (from Thief's Hiding and Assassin's Cloaking) to be fully implemented first. Since Hiding is currently a toggle with no actual invisibility server-side logic, Ruwach's detection aspect is deferred.

When implemented, Ruwach's tick should:
1. Every ~200ms, check all enemies/players in range with `isHidden` flag
2. Force-unhide them (remove Hiding/Cloaking buff)
3. Deal 145% MATK Holy damage

---

### 11. TELEPORT (ID 409) — Active, Self

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self | All sources |
| Max Level | 2 | All sources |
| Lv1 Effect | Random teleport within same map | iRO Wiki Classic |
| Lv2 Effect | Teleport to save point | iRO Wiki Classic |
| SP Cost | Lv1: 10, Lv2: 9 | iRO Wiki Classic (formula: 11 - SkillLv) |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | ASPD-based | iRO Wiki Classic |
| Prerequisite | Ruwach Lv1 | All sources |
| Restrictions | Cannot be used in PvP or WoE maps | iRO Wiki Classic |

#### Current Implementation Status: MOSTLY CORRECT

- Lv1 random warp: **CORRECT** — randomizes position within zone
- Lv2 save point: **CORRECT** — reads from DB and teleports/zone changes
- SP cost: **PARTIALLY CORRECT** — both levels use 10 SP, but Lv2 should be 9

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Lv2 SP cost is 10 instead of 9 | LOW | Trivial — use manual levels array instead of genLevels |
| No PvP/WoE map restriction | DEFERRED | Phase 7+ (PvP not yet implemented) |
| Random warp range could be too small/large — uses ±1000 from spawn | LOW | Tuning issue |

#### Implementation Notes

The SP cost fix is trivial: change from `genLevels(2, ...)` with fixed `spCost: 10` to manual levels:
```js
levels: [
    { level: 1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 },
    { level: 2, spCost: 9, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 }
]
```

The random warp implementation is simplified (randomizes around spawn point) but functional. A more canonical approach would randomize across all walkable cells in the map, but this requires map data.

---

### 12. WARP PORTAL (ID 410) — Active, Ground

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Ground | All sources |
| Max Level | 4 | All sources |
| Catalyst | 1 Blue Gemstone per cast | iRO Wiki Classic |
| Memorized Locations | Lv1: save point only, Lv2: +1 memo, Lv3: +2 memo, Lv4: +3 memo | iRO Wiki Classic |
| Max Active Portals | 3 | iRO Wiki Classic |
| Portal Duration | 10, 15, 20, 25 seconds | iRO Wiki Classic |
| SP Cost | 35, 32, 29, 26 | iRO Wiki Classic |
| Cast Time | ~1 second | iRO Wiki Classic |
| After-Cast Delay | 1 second | iRO Wiki Classic |
| Prerequisite | Teleport Lv2 | All sources |
| Memorize System | `/memo` command to save locations (max 3 spots, FIFO overwrite) | iRO Wiki Classic |

#### Current Implementation Status: PARTIALLY CORRECT

- SP cost: **CORRECT** — 35, 32, 29, 26
- Duration: **CORRECT** — 10, 15, 20, 25 seconds
- Cast time: **CORRECT** — 1000ms
- Ground effect creation: **CORRECT** — uses `createGroundEffect()`
- Portal destination: **PARTIAL** — always goes to save point

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No Blue Gemstone consumption** — should consume 1 Blue Gem from inventory | HIGH | Medium — check inventory, remove item |
| **No memorize system** — always warps to save point, should support `/memo` and multiple destinations | MEDIUM | Large — new `/memo` command, DB table for memo spots, destination selection UI |
| **No max portal limit** — should allow max 3 concurrent portals | MEDIUM | Small — use `countGroundEffects()` like Fire Wall |
| **No after-cast delay** in skill definition | LOW | Trivial — add field |
| Portal stepping logic (walking into portal warps you) needs verification | MEDIUM | Need to verify ground effect overlap handler |

#### Implementation Notes

Blue Gemstone consumption: Check `character_inventory` for item ID 717 (Blue Gemstone), consume 1 on cast. If none available, emit `skill:error`.

The memorize system is a significant feature requiring:
1. DB table: `character_memo_points (character_id, slot, zone, x, y, z)`
2. Server handler: `memo:save` event (saves current position to next slot)
3. Client UI: Destination selection popup when casting Warp Portal at Lv2+
4. Slot count per level: Lv1=0 (save point only), Lv2=1, Lv3=2, Lv4=3

This is a large feature and should be its own implementation phase.

---

### 13. PNEUMA (ID 411) — Active, Ground

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Ground Persistent | All sources |
| Max Level | 1 | All sources |
| AoE | 3x3 cells (~150 UE units radius) | iRO Wiki Classic |
| Duration | 10 seconds | iRO Wiki Classic |
| SP Cost | 10 | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | None | iRO Wiki Classic |
| Effect | Blocks ALL ranged physical damage (range 4+ cells) | iRO Wiki Classic |
| Does NOT Block | Magic damage, melee attacks, status effects (stun/knockback still apply) | iRO Wiki Classic |
| Overlap Rule | Cannot overlap existing Pneuma, Safety Wall, or Magnetic Earth | iRO Wiki Classic |
| Casting Range | 9 cells (~450 UE units) | iRO Wiki |
| Prerequisite | Warp Portal Lv4 | All sources |
| Blocked Skills | Acid Bomb, Arrow Shower, Blitz Beat, Double Strafe, Shield Boomerang, Soul Destroyer (phys part), + many more | iRO Wiki |

#### Current Implementation Status: MOSTLY CORRECT

- Duration: **CORRECT** — 10000ms
- SP cost: **CORRECT** — 10
- Cast time: **CORRECT** — 0
- Ground effect creation: **CORRECT** — uses `createGroundEffect()`
- Combat tick integration: **CORRECT** — checks `getGroundEffectsAtPosition()` for Pneuma, blocks ranged auto-attacks

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No overlap check** — can stack multiple Pneuma on same cell, or overlap Safety Wall | MEDIUM | Small — check existing ground effects at position before creating |
| AoE radius uses 100 UE units, canonical 3x3 = ~150 UE units (3 cells * 50) | LOW | Trivial |
| Ranged skill block not implemented — only blocks ranged auto-attacks in combat tick | MEDIUM | Medium — add Pneuma check in skill damage paths |
| No max concurrent Pneuma limit checked | LOW | Small — add `countGroundEffects()` check |
| `blockRanged` buff modifier exists in buff system but Pneuma doesn't apply it as a buff — it's a ground effect only | — | Design choice, ground effect approach is correct |

#### Implementation Notes

The overlap check should verify no existing Pneuma, Safety Wall, or Magnetic Earth at the target position:
```js
const existing = getGroundEffectsAtPosition(groundX, groundY, groundZ, 50);
if (existing.some(e => e.type === 'pneuma' || e.type === 'safety_wall')) {
    socket.emit('skill:error', { message: 'Cannot place here' });
    return;
}
```

For ranged skill blocking, any `skill:effect_damage` with range >= 4 cells (200 UE units) should check if the target is standing in a Pneuma. This requires adding Pneuma checks to the skill execution paths for ranged skills (Double Strafe, Arrow Shower, etc.).

---

### 14. AQUA BENEDICTA (ID 412) — Active, Self

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self | All sources |
| Max Level | 1 | All sources |
| Effect | Creates 1 Holy Water | iRO Wiki Classic |
| SP Cost | 10 | iRO Wiki Classic |
| Cast Time | 1 second | iRO Wiki Classic |
| After-Cast Delay | 0.5 seconds | iRO Wiki Classic |
| Catalyst | 1 Empty Bottle consumed | iRO Wiki Classic |
| Water Requirement | Must stand on water cell / Deluge | iRO Wiki Classic |
| Prerequisite | None | iRO Wiki Classic |

#### Current Implementation Status: ACCEPTABLE (Simplified)

- SP cost: **CORRECT** — 10
- Cast time: **CORRECT** — 1000ms
- After-cast delay: **CORRECT** — 500ms
- System message: **CORRECT** — notifies player of Holy Water creation

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No Empty Bottle consumption** — should consume 1 Empty Bottle (item ID 713) | MEDIUM | Small — check inventory |
| **No Holy Water creation in inventory** — only sends chat message | MEDIUM | Medium — add Holy Water item (ID 523) to inventory |
| **No water cell check** — can be used anywhere | LOW | Hard — requires map tile data |
| No actual item creation flow (inventory:add_item event) | MEDIUM | Part of Holy Water inventory addition |

#### Implementation Notes

Simplified implementation is acceptable for now since:
1. Water cell detection requires map tile metadata (not available)
2. Holy Water is primarily used for Aspersio (Priest skill) and a few other skills

When implementing properly:
1. Check inventory for Empty Bottle (ID 713)
2. Consume 1 Empty Bottle
3. Add 1 Holy Water (ID 523) to inventory
4. Emit `inventory:data` update

---

### 15. HOLY LIGHT (ID 414) — Active, Single Target (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Offensive Magic (Quest Skill) | All sources |
| Max Level | 1 | All sources |
| Target | Single enemy | All sources |
| Damage | 125% MATK, Holy element | iRO Wiki Classic |
| SP Cost | 15 | iRO Wiki Classic |
| Cast Time | 2 seconds | iRO Wiki Classic |
| After-Cast Delay | None (ASPD-based) | iRO Wiki Classic |
| Element | Holy | All sources |
| Special | Immediately cancels Kyrie Eleison on target (even if it misses) | iRO Wiki Classic |
| Quest | Acolyte Skill Quest (1 Rosary, 1 Opal, 1 Crystal Blue) | RateMyServer |
| Range | Ranged magic | iRO Wiki Classic |

#### Current Implementation Status: NO HANDLER

- Skill definition: **EXISTS** — ID 414, 125% effectValue, 2s cast time, 15 SP
- Server handler: **MISSING** — no `if (skill.name === 'holy_light')` in index.js

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No server handler** — skill is defined but has no execution logic | HIGH | Medium — add magical damage handler |
| Quest skill learning — no quest system to learn it | DEFERRED | Phase 7+ (quest system) |
| Kyrie Eleison cancel on hit — 2nd class skill not yet implemented | DEFERRED | Phase 6+ |

#### Implementation Notes

Holy Light should follow the bolt/magical damage template:
```js
if (skill.name === 'holy_light') {
    if (!targetId || !isEnemy) { socket.emit('skill:error', { message: 'No enemy target' }); return; }
    const enemy = enemies.get(targetId);
    if (!enemy || enemy.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
    // Range check, SP deduction, apply delays
    // Calculate magical damage: 125% MATK, Holy element
    const hitResult = calculateMagicSkillDamage(
        getEffectiveStats(player), targetStats, targetHardMdef,
        125, 'holy', magicTargetInfo
    );
    enemy.health = Math.max(0, enemy.health - hitResult.damage);
    // Broadcast, death check, skill:used, health_update
}
```

For now, Holy Light can be added as a learnable skill (skip quest requirement) since the quest system doesn't exist yet.

---

## New Systems Required

| System | Skills Affected | Priority | Effort |
|--------|----------------|----------|--------|
| Blessing offensive path (debuff Undead/Demon enemies) | Blessing (402) | HIGH | Medium |
| Decrease AGI success rate formula | Decrease AGI (404) | HIGH | Small |
| Decrease AGI boss immunity | Decrease AGI (404) | HIGH | Trivial |
| Increase AGI HP cost + min HP check | Increase AGI (403) | HIGH | Small |
| Holy Light magical damage handler | Holy Light (414) | HIGH | Medium |
| Signum Crucis success rate with level formula | Signum Crucis (407) | HIGH | Small |
| Pneuma overlap prevention | Pneuma (411) | MEDIUM | Small |
| Blue Gemstone consumption (Warp Portal) | Warp Portal (410) | MEDIUM | Medium |
| Warp Portal memorize system (`/memo`) | Warp Portal (410) | LOW | Large |
| Party-wide buff broadcast (Angelus) | Angelus (406) | DEFERRED | Medium (needs party system) |
| Aqua Benedicta item creation flow | Aqua Benedicta (412) | LOW | Medium |
| Ruwach hidden enemy detection tick | Ruwach (408) | DEFERRED | Medium (needs Hidden system) |
| Divine Protection / Demon Bane base level scaling | DP (401), DB (413) | LOW | Small |

---

## Skill Definition Corrections

### Heal (ID 400)
```
CHANGE: cooldown: 1000 -> afterCastDelay: 1000, cooldown: 0
```

### Increase AGI (ID 403)
```
ADD: afterCastDelay: 1000
```

### Decrease AGI (ID 404)
```
CHANGE: duration formula from (4+i)*10000 to (i+3)*10000
         giving 30, 40, 50, 60, 70, 80, 90, 100, 110, 120 seconds (monsters)
```

### Cure (ID 405)
```
CHANGE: cooldown: 1000 -> cooldown: 0 (no cooldown in RO Classic)
```

### Signum Crucis (ID 407)
```
CHANGE: duration from 0 to 86400000 (24h = permanent until death)
```

### Teleport (ID 409)
```
CHANGE: levels to manual array with Lv1 SP=10, Lv2 SP=9
```

---

## Implementation Priority

### Phase 1: Critical Fixes (HIGH priority, small effort)
1. **Decrease AGI success rate** — add roll with formula `40 + 2*Lv + floor((BaseLv+INT)/5) - TargetMDEF`
2. **Decrease AGI boss immunity** — block on `enemy.isBoss` or boss mode flag
3. **Decrease AGI duration fix** — change formula to `(i+3)*10000`
4. **Increase AGI HP cost** — deduct 15 HP, check >= 16 HP before cast
5. **Increase AGI after-cast delay** — add `afterCastDelay: 1000` to skill def
6. **Signum Crucis success rate** — add roll with formula `23 + 4*Lv + CasterBaseLv - TargetLv`
7. **Signum Crucis permanent duration** — set to 86400000ms
8. **Heal afterCastDelay** — change from `cooldown` to `afterCastDelay`

### Phase 2: Medium Effort Additions (HIGH priority)
1. **Blessing Undead/Demon debuff** — add enemy targeting path that halves STR/DEX/INT
2. **Blessing Curse/Stone cure** — add `cleanseStatusEffects()` for curse/stone
3. **Holy Light handler** — add magical damage handler (125% MATK Holy)

### Phase 3: System Features (MEDIUM priority)
1. **Pneuma overlap check** — prevent stacking with existing Pneuma/Safety Wall
2. **Pneuma ranged skill block** — add Pneuma check to skill damage paths
3. **Warp Portal Blue Gemstone** — consume from inventory on cast
4. **Signum Crucis Undead element check** — add element check alongside race check
5. **Signum Crucis AoE radius** — increase from 500 to 750 UE units

### Phase 4: Deferred (LOW priority / needs other systems)
1. **Warp Portal memorize system** — needs DB table, `/memo` command, destination UI
2. **Angelus party-wide** — needs party system
3. **Ruwach detection tick** — needs Hidden status system
4. **Aqua Benedicta item flow** — needs Empty Bottle + Holy Water inventory integration
5. **Divine Protection / Demon Bane base level scaling** — minor accuracy improvement
6. **Cure cooldown removal** — minor accuracy
7. **Teleport Lv2 SP cost** — change from 10 to 9

---

## Sources

- [Heal - iRO Wiki Classic](https://irowiki.org/classic/Heal)
- [Heal - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=28)
- [Divine Protection - iRO Wiki Classic](https://irowiki.org/classic/Divine_Protection)
- [Divine Protection - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=22)
- [Demon Bane - iRO Wiki Classic](https://irowiki.org/classic/Demon_Bane)
- [Demon Bane - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=23)
- [Blessing - iRO Wiki Classic](https://irowiki.org/classic/Blessing)
- [Blessing - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=34)
- [Increase AGI - iRO Wiki Classic](https://irowiki.org/classic/Increase_AGI)
- [Increase AGI - iRO Wiki](https://irowiki.org/wiki/Increase_AGI)
- [Increase AGI - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=29)
- [Decrease AGI - iRO Wiki Classic](https://irowiki.org/classic/Decrease_AGI)
- [Decrease AGI - iRO Wiki](https://irowiki.org/wiki/Decrease_AGI)
- [Decrease AGI - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=30)
- [Cure - iRO Wiki Classic](https://irowiki.org/classic/Cure)
- [Angelus - iRO Wiki Classic](https://irowiki.org/classic/Angelus)
- [Angelus - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=33)
- [Signum Crucis - iRO Wiki Classic](https://irowiki.org/classic/Signum_Crusis)
- [Signum Crucis - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=32)
- [Signum Crucis - rAthena DB](https://db.pservero.com/skill/AL_CRUCIS)
- [Signum Crucis - Project Alfheim Wiki](https://projectalfheim.net/wiki/index.php/Signum_Crucis)
- [Ruwach - iRO Wiki Classic](https://irowiki.org/classic/Ruwach)
- [Teleport - iRO Wiki Classic](https://irowiki.org/classic/Teleport)
- [Warp Portal - iRO Wiki Classic](https://irowiki.org/classic/Warp_Portal)
- [Pneuma - iRO Wiki Classic](https://irowiki.org/classic/Pneuma)
- [Pneuma - iRO Wiki](https://irowiki.org/wiki/Pneuma)
- [Aqua Benedicta - iRO Wiki Classic](https://irowiki.org/classic/Aqua_Benedicta)
- [Holy Light - iRO Wiki Classic](https://irowiki.org/classic/Holy_Light)
- [Holy Light - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=156)
