# Thief Skills Deep Audit Plan v2 (2026-03-16)

Comprehensive skill-by-skill audit of all 10 Thief skills (IDs 500-509) against canonical Ragnarok Online Pre-Renewal (rAthena) behavior. Deep-researched against rAthena source code, iRO Wiki Classic, RateMyServer pre-renewal DB.

---

## Audit Summary

| Skill | ID | Verdict | Issues Found |
|-------|----|---------|-------------|
| Double Attack | 500 | **3 BUGS** | DA extra hit can crit; main hit crit not suppressed on DA proc; missing +1 HIT/lv on proc |
| Improve Dodge | 501 | **CORRECT** | No issues |
| Steal | 502 | **2 BUGS, 1 DATA** | Wrong ACD (1000ms vs 0ms); per-player steal lock instead of global; effectValue mismatch |
| Hiding | 503 | **1 BUG** | Doesn't cancel auto-attack on activation (rAthena: `StopAttacking: true`) |
| Envenom | 504 | **1 BUG** | Flat bonus not affected by element table (mastery ATK should go through element) |
| Detoxify | 505 | **CORRECT** | No issues |
| Sand Attack | 506 | **CORRECT** | 130% ATK, Earth, 20% blind — all verified |
| Back Slide | 507 | **1 GAP** | Doesn't cancel auto-attack state |
| Throw Stone | 508 | **2 BUGS** | MISC damage should bypass ALL DEF (currently applies hard+soft DEF); missing Blind fallback |
| Pick Stone | 509 | **CORRECT** | No issues |

**Total: 9 bugs + 1 gap + 1 data fix = 11 changes needed**

---

## Sources

All findings verified against these authoritative sources:
- **rAthena source**: `battle.cpp`, `skill.cpp`, `pc.cpp`, `status.cpp`, `db/pre-re/skill_db.yml`, `db/pre-re/status.yml`
- **rAthena issues/PRs**: #4460 (DA crit), #4454 (DA fix), #3473 (Steal), #1914 (Back Slide walls), #419/#439 (hidden targeting)
- **rAthena commit**: `eb4658f` (Envenom element double-application)
- **iRO Wiki Classic**: Double Attack, Steal, Hiding, Envenom, Sand Attack, Back Slide, Stone Fling, Pick Stone, Detoxify, Attacks page
- **RateMyServer Pre-Re**: All 10 Thief skills + Status Resistance Formulas guide

---

## Detailed Findings Per Skill

### 1. Double Attack (ID 500) — 3 ISSUES

#### BUG A: DA extra hit can critically strike (WRONG)
- **File**: `server/src/index.js` ~line 18112-18132
- **Current**: DA extra hit uses `calculatePhysicalDamage()` which includes crit logic. Line 18132 sends `isCritical: daResult2.isCritical`.
- **Canonical (rAthena)**: Pre-renewal `db/pre-re/skill_db.yml` for TF_DOUBLE lacks `DamageFlags: Critical: true` (which Renewal adds). `is_attack_critical()` in `battle.cpp` returns `false` when attack is `DMG_MULTI_HIT` and skill lacks `NK_CRITICAL`. DA and crit are mutually exclusive — DA has higher priority.
- **Source**: rAthena Issue #4460 confirms this was a bug that was fixed in PR #4454.
- **Fix**: Force `isCritical = false` on DA hit result:
  ```javascript
  // After calculatePhysicalDamage for DA hit:
  daResult2.isCritical = false; // DA suppresses crits in pre-renewal
  ```

#### BUG B: Main hit crit not suppressed when DA procs
- **Current flow**: Main hit calculates (can crit) → broadcasts → DA rolls → if DA procs, extra hit added.
- **Canonical flow**: In rAthena, `battle_calc_multi_attack()` is called BEFORE `is_attack_critical()`. DA proc is determined first, and if DA procs, the crit check is skipped entirely for both hits.
- **Impact**: When DA procs AND main hit crits (DA% × CRIT%), player gets bonus crit damage they shouldn't.
- **Fix**: Pre-roll DA before main damage calculation:
  ```javascript
  // BEFORE main damage calculation:
  const willDA = doubleAttackChance > 0 && rightIsDagger && Math.random() * 100 < doubleAttackChance;
  // Pass noCrit option to main hit calculation if willDA is true
  // Then use willDA flag instead of re-rolling after main hit
  ```

#### GAP C: Missing +1 HIT per level on DA proc
- **Current**: No HIT bonus in DA logic.
- **Canonical (rAthena)**: `battle.cpp` — `hitrate += pc_checkskill(sd, TF_DOUBLE)` when `wd->div_ == 2`. This is +1 flat HIT per DA level, ONLY on the DA proc (not always-on). Pre-renewal = flat, Renewal = percentage.
- **Source**: iRO Wiki Classic: "Adds Hit equal to the Skill's Level. This Hit is only added in the instance of a Double Attack happening." RateMyServer Pre-Re: "+1 HIT per SkillLV (that only applies in double attacks)"
- **Fix**: Add HIT bonus to DA hit calculation (pass as option or temp-modify cached stats).

#### ADDITIONAL DA NOTES (verified correct):
- Each DA hit deals full ATK (real 2x damage, NOT a split) ✓
- Both hits happen within the same attack cycle (no ASPD change) ✓
- Daggers only (skill-based; equipment like Sidewinder Card bypasses) ✓
- Right hand only when dual wielding ✓
- Auto-attacks only, never on skills ✓
- DA checked before Triple Attack in `battle_calc_multi_attack()` ✓ (our code does this)
- Card status procs fire once per attack action (not per DA hit) — consistent with our implementation ✓

---

### 2. Improve Dodge (ID 501) — CORRECT

- **Effect**: +3 FLEE per level ✓ (line 564)
- **Scope**: Always-on passive ✓
- **Canonical**: Matches rAthena pre-renewal exactly
- **Deferred**: 2nd class scaling (+4/lv for Assassin/Rogue)

---

### 3. Steal (ID 502) — 2 BUGS + 1 DATA ISSUE

#### BUG A: afterCastDelay is 1000ms, should be 0ms
- **File**: `server/src/ro_skill_data.js`, Steal skill definition
- **Current**: All 10 levels have `afterCastDelay: 1000`
- **Canonical**: rAthena `db/pre-re/skill_db.yml` has NO AfterCastActDelay for TF_STEAL (defaults to 0). The "1 second delay" documented on iRO Wiki is the **client-side animation lock**, not a server-enforced ACD.
- **Fix**: Change `afterCastDelay: 1000` → `afterCastDelay: 0` for all 10 levels.

#### BUG B: Per-player steal lock instead of global lock
- **File**: `server/src/index.js` ~line 8919-8920
- **Current**: `enemy.stolenBy` is a `Set()` of characterIds. Each player can steal once, but multiple players can each steal from the same monster.
- **Canonical (rAthena)**: `pc_steal_item()` in `pc.cpp` — On success, `md->state.steal_flag = UCHAR_MAX` (a single flag on the monster). Once ANY player steals successfully, the monster is locked for ALL players globally.
- **Source**: rAthena forum code extract + Issue #3473 confirm: "Only one successful steal per monster, globally."
- **Fix**: Replace `Set` with a single boolean flag:
  ```javascript
  // Replace:
  if (!enemy.stolenBy) enemy.stolenBy = new Set();
  if (enemy.stolenBy.has(characterId)) { ... }
  // With:
  if (enemy.stealLocked) {
      socket.emit('chat:receive', { ... message: 'This monster has already been stolen from.' });
  } else {
      // ... attempt steal
      if (success) {
          enemy.stealLocked = true; // Global lock — no player can steal again
      }
  }
  ```

#### DATA: effectValue mismatch (cosmetic)
- **Current**: 16, 22, 28, 34, 40, 46, 52, 58, 64, 70
- **Canonical**: 10, 16, 22, 28, 34, 40, 46, 52, 58, 64
- **Impact**: NONE — handler hardcodes the canonical formula.
- **Fix (optional)**: Correct to `4 + 6*(i+1)`.

#### ADDITIONAL STEAL NOTES (verified correct):
- Boss immunity ✓
- Success formula: `(4 + 6*SkillLv) + (DEX - MonsterDEX) / 2` ✓
- Rate NOT capped at 100% — high DEX can exceed, making common drops guaranteed ✓
- No base level penalty — confirmed (only DEX-based) ✓
- Drop table rolled sequentially slot 0 to N — first match wins ✓
- Cards skipped (`stealProtected` / `IT_CARD`) ✓
- Cannot steal from players (monsters only) ✓
- Steal does NOT affect death drops ✓

---

### 4. Hiding (ID 503) — 1 BUG

#### BUG: Doesn't cancel auto-attack on activation
- **File**: `server/src/index.js` ~line 8991-9011
- **Current**: Hiding toggle ON applies buff but doesn't clear `autoAttackState`.
- **Canonical (rAthena)**: `db/pre-re/status.yml` for SC_HIDING includes `Flags: StopAttacking: true`. This calls `unit_stop_attack()` server-side when the status is applied. It is a SERVER-ENFORCED flag, not just client visual.
- **Fix**: Add after SP deduction (line 8992):
  ```javascript
  if (autoAttackState.has(characterId)) {
      autoAttackState.delete(characterId);
      socket.emit('combat:auto_attack_stopped', { reason: 'Hiding' });
  }
  ```

#### ADDITIONAL HIDING NOTES (verified correct):
- Toggle ON/OFF ✓
- SP drain: 1 SP per `(4 + SkillLv)` seconds ✓
- SP=0 removal ✓
- Movement 100% blocked (rAthena: `NoMoveCond: true`) ✓
- `RemoveOnDamaged: true` — hiding breaks when damage from TargetHidden skills lands ✓
- Only Heaven's Drive and Quagmire have `TargetHidden: true` in pre-renewal ✓
- Sanctuary does NOT reveal hidden players ✓
- Normal AoE/splash from non-TargetHidden skills cannot hit hidden players ✓
- Detector monsters: Insect race, Demon race, Boss flag (includes MVPs) ✓
- 7-point enforcement all verified ✓

---

### 5. Envenom (ID 504) — 1 BUG

#### BUG: Flat bonus not affected by element table
- **File**: `server/src/index.js` ~line 9096-9107
- **Current**: `totalDamage = envResult.damage + flatBonus` — the flat bonus (15*lv) is added raw, not modified by element.
- **Canonical (rAthena)**: In `battle.cpp`, Envenom adds the flat bonus to `masteryAtk`:
  ```cpp
  if(skill_id == TF_POISON)
      ATK_ADD(wd->masteryAtk, wd->masteryAtk2, 15 * skill_lv);
  ```
  Mastery ATK is part of the total damage that goes through the element table. rAthena commit `eb4658f` specifically confirms: "Envenom applies the attribute table to the base damage and then again to the final damage." This means the element modifier applies to the flat bonus too.
- **Impact**: Against Poison-property monsters (Poison vs Poison = 0%), the flat bonus should deal 0 but currently deals full damage. Against Undead-property (Poison vs Undead = 50%), flat bonus should be halved. In most practical scenarios the impact is small, but it's technically wrong and matters for Poison-element enemies.
- **Fix**: Apply element modifier to the flat bonus:
  ```javascript
  const flatBonus = 15 * learnedLevel;
  // Apply element table to flat bonus (mastery ATK goes through element in rAthena)
  const enemyElemType = envTargetInfo.elementType || 'neutral';
  const enemyElemLevel = envTargetInfo.elementLevel || 1;
  const elemMod = getElementModifier('poison', enemyElemType, enemyElemLevel);
  const adjustedFlat = Math.max(0, Math.floor(flatBonus * elemMod));

  if (envResult.isMiss) {
      totalDamage = adjustedFlat; // Miss: flat only, still element-modified
  } else {
      totalDamage = envResult.damage + adjustedFlat;
  }
  ```

#### VERIFIED CORRECT:
- Custom handler (NOT executePhysicalSkillOnEnemy) ✓
- 100% ATK with forced poison element ✓
- Flat bonus bypasses ALL DEF (mastery damage added after DEF calc) ✓
- Flat bonus always hits on miss ✓
- Poison chance `(10 + 4*SkillLv)%` ✓ (confirmed in eAthena source: `(4*skilllv+10)`)
- Poison duration 60000ms base ✓ (VIT/LUK reduction handled by `applyStatusEffect`)
- Boss/Undead immunity ✓
- No weapon restriction — any weapon or bare hands ✓
- SP 12, range 100 ✓

---

### 6. Detoxify (ID 505) — CORRECT

- Self or ally target (TargetType: Friend) ✓
- Range 9 cells (450 UE) ✓
- Cleanses ONLY Poison (not bleeding, not deadly poison) ✓
- SP 10, prerequisite Envenom Lv3 ✓

---

### 7. Sand Attack (ID 506) — CORRECT

- 130% ATK ✓ (rAthena + RMS agree; iRO Wiki says 125% but is likely wrong)
- Earth element ✓ (all sources agree)
- 20% blind chance ✓ (rAthena source + RMS; iRO Wiki says 15% but emulator is authoritative)
- Blind base duration 30s, reduced by `(tarInt + tarVit) / 200` and `10 * tarLuk` ✓
- SP 9, range 150, quest skill Lv1 ✓

---

### 8. Back Slide (ID 507) — 1 GAP

#### GAP: Doesn't cancel auto-attack state
- **Current**: Teleports player backward but doesn't clear `autoAttackState`.
- **Canonical**: rAthena Back Slide handler does not explicitly call `unit_stop_attack()`, but repositioning 5 cells backward breaks melee range, causing the client to drop the target lock. The server should mirror this.
- **Fix**: Add before teleport (after line 9199):
  ```javascript
  if (autoAttackState.has(characterId)) {
      autoAttackState.delete(characterId);
      socket.emit('combat:auto_attack_stopped', { reason: 'Back Slide' });
  }
  ```

#### VERIFIED CORRECT:
- Direction from `player.lastDirX/lastDirY` ✓
- Distance 250 UE (5 cells) backward ✓
- Server position update + Redis cache ✓
- Zone broadcast ✓
- Stops at walls (doesn't fail) — our current impl doesn't check walls but moves max distance ✓ (obstacle check is deferred)
- SP 7, no ACD, no cooldown ✓

---

### 9. Throw Stone (ID 508) — 2 BUGS

#### BUG A: MISC damage type — should bypass ALL DEF (MAJOR)
- **File**: `server/src/index.js` ~lines 9275-9280
- **Current**: Applies both hard DEF and soft DEF reduction to the 50 flat damage:
  ```javascript
  let stoneDamage = Math.floor(50 * (100 - Math.min(99, enemyHardDef)) / 100);
  stoneDamage = Math.max(1, stoneDamage - enemySoftDef);
  ```
- **Canonical (rAthena)**: Throw Stone has `Type: Misc` in `skill_db.yml`. MISC damage type **ignores both hard DEF and soft DEF entirely**. Additionally, `nk` flags include `IgnoreFlee` (always hits). iRO Wiki Classic confirms: "50 Attack Strength which pierces the target's Defense." RMS Pre-Re confirms: "always does 50 points of damage."
- **Impact**: Against armored enemies (e.g., 50 hard DEF + 30 soft DEF), our implementation deals ~12 damage instead of 50. This is a 4x undercount.
- **Fix**: Remove all DEF calculation:
  ```javascript
  // Replace the DEF reduction block with:
  const stoneDamage = 50; // MISC damage — bypasses ALL defense
  ```

#### BUG B: Missing Blind fallback when Stun fails
- **File**: `server/src/index.js` ~lines 9308-9313
- **Current**: Only applies 3% stun.
- **Canonical (rAthena)**: `TF_THROWSTONE` handler — try 3% stun, if stun fails → try 3% blind as fallback. Not independent rolls.
- **Source**: rAthena source (confirmed by both eAthena stable and current rAthena):
  ```cpp
  if (!sc_start(src, bl, SC_STUN, 3, ...))
      sc_start(src, bl, SC_BLIND, 3, ...);
  ```
- **Fix**: Replace stun-only block:
  ```javascript
  if (enemy.health > 0) {
      const stunResult = applyStatusEffect(player, enemy, 'stun', 3);
      if (stunResult && stunResult.applied) {
          broadcastToZone(tsZone, 'status:applied', { targetId, isEnemy: true, targetName: enemy.name, statusType: 'stun', duration: stunResult.duration, sourceId: characterId, sourceName: player.characterName });
      } else {
          // Blind fallback (canonical rAthena): only attempted if stun fails
          const blindResult = applyStatusEffect(player, enemy, 'blind', 3);
          if (blindResult && blindResult.applied) {
              broadcastToZone(tsZone, 'status:applied', { targetId, isEnemy: true, targetName: enemy.name, statusType: 'blind', duration: blindResult.duration, sourceId: characterId, sourceName: player.characterName });
          }
      }
  }
  ```

#### VERIFIED CORRECT:
- Stone (7049) inventory check + consumption ✓
- Weight update (-3) ✓
- Fixed 50 damage (no STR bonus) ✓
- Always hits (MISC type ignores Flee) ✓
- Neutral element (cannot be overridden by arrows — MISC type) ✓
- `checkDamageBreakStatuses` called ✓
- Inventory update after consumption ✓
- SP 2, range 350 ✓

---

### 10. Pick Stone (ID 509) — CORRECT

- Cast time 500ms (fixed) ✓
- Weight check <50% ✓
- Stone item (7049) creation ✓
- Weight update (+3) ✓
- Can hold multiple stones (stack up to 30,000) ✓
- SP 3 ✓

---

## Implementation Plan

### Phase 1: Critical Damage Fixes (3 changes)

| # | Skill | Fix | Severity | File |
|---|-------|-----|----------|------|
| 1 | **Throw Stone (508)** | Remove ALL DEF calculation — MISC damage bypasses defense entirely. `stoneDamage = 50` | **CRITICAL** | `index.js` ~9275-9280 |
| 2 | **Throw Stone (508)** | Add blind 3% fallback when stun fails | MEDIUM | `index.js` ~9308-9313 |
| 3 | **Envenom (504)** | Apply element modifier to flat bonus (mastery ATK goes through element table) | MEDIUM | `index.js` ~9096-9107 |

### Phase 2: Steal Fixes (3 changes)

| # | Skill | Fix | Severity | File |
|---|-------|-----|----------|------|
| 4 | **Steal (502)** | Change `afterCastDelay: 1000` → `0` for all 10 levels | HIGH | `ro_skill_data.js` |
| 5 | **Steal (502)** | Replace per-player `stolenBy` Set with global `stealLocked` boolean | HIGH | `index.js` ~8919-8930 |
| 6 | **Steal (502)** | Fix effectValue to `4 + 6*(i+1)` (cosmetic) | LOW | `ro_skill_data.js` |

### Phase 3: Auto-Attack Cancellation (2 changes)

| # | Skill | Fix | Severity | File |
|---|-------|-----|----------|------|
| 7 | **Hiding (503)** | Add `autoAttackState.delete()` + emit stop on toggle ON | HIGH | `index.js` ~8992 |
| 8 | **Back Slide (507)** | Add `autoAttackState.delete()` + emit stop before teleport | MEDIUM | `index.js` ~9199 |

### Phase 4: Double Attack Refactor (3 changes)

| # | Skill | Fix | Severity | File |
|---|-------|-----|----------|------|
| 9 | **Double Attack (500)** | Force DA extra hit `isCritical = false` | HIGH | `index.js` ~18117 |
| 10 | **Double Attack (500)** | Pre-roll DA before main hit calc; suppress crit on main hit if DA procs | MEDIUM | `index.js` combat tick |
| 11 | **Double Attack (500)** | Add +learnedLevel flat HIT bonus to DA proc hits | LOW | `index.js` ~18112 |

---

## What's Already Correct (No Changes Needed)

| Skill | Verified Details |
|-------|-----------------|
| Improve Dodge (501) | +3 FLEE/lv passive — exact match |
| Detoxify (505) | Cleanse poison only, self/ally target, range 450 — exact match |
| Sand Attack (506) | 130% ATK, Earth element, 20% blind — exact match (rAthena authoritative over iRO Wiki's 125%/15%) |
| Pick Stone (509) | 500ms cast, <50% weight, Stone creation — exact match |
| Hiding SP drain | 1 SP per (4+level) seconds, SP=0 removal — exact match |
| Hiding 7-point enforcement | Move/attack/items/HP regen/SP regen/skill regen/SP drain — all verified |
| Hiding break conditions | Manual cancel, duration, SP=0, skill use, Sight/Ruwach/IC, detector mobs — all verified |
| Hiding detection | Sanctuary does NOT reveal. Only Heaven's Drive + Quagmire have TargetHidden. Insect/Demon/Boss detect. |
| Steal formula | `(4+6*lv) + (DEX-MonDEX)/2`, boss immunity, sequential drop table, cards skipped — exact match |
| Envenom core mechanics | Custom handler, 100% ATK + flat bypass, miss behavior, poison formula, immunities — correct |
| Throw Stone core | Flat 50, stone consumption, always hits, neutral, weight update — correct |
| Back Slide mechanics | 5 cells backward, server pos update, zone broadcast — correct |

---

## Source Disagreements Resolved

| Question | rAthena Source | iRO Wiki Classic | RMS Pre-Re | **Our Decision** |
|----------|---------------|-----------------|------------|-------------------|
| Sand Attack blind % | 20% | 15% | 20% | **20%** (rAthena authoritative) |
| Sand Attack damage | 130% | 125% | 130% | **130%** (rAthena authoritative) |
| Throw Stone stun % | 3% | "5% stun or blind" | 3% | **3%** (rAthena authoritative) |
| Steal ACD | 0ms (server) | "1s cast delay" | — | **0ms** (iRO Wiki refers to client animation) |

---

## Risk Assessment

| Change | Risk | Notes |
|--------|------|-------|
| Throw Stone DEF removal | LOW | Simple code deletion, makes skill stronger (correct) |
| Throw Stone blind fallback | LOW | Additive behavior, no existing change |
| Envenom element on flat bonus | LOW | Small damage delta in most cases |
| Steal ACD removal | LOW | Limited by animation in practice |
| Steal global lock | LOW | Simple flag change |
| Hiding auto-attack cancel | LOW | Prevents broken state |
| Back Slide auto-attack cancel | LOW | Prevents broken state |
| DA crit suppression (extra hit) | LOW | Simple flag override |
| DA pre-roll refactor | **MEDIUM** | Touches combat tick hot path |
| DA HIT bonus | LOW | Minor accuracy change |
