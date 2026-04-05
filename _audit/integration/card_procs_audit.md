# Card Proc Hooks Cross-System Integration Audit

**Auditor**: Claude Opus 4.6 (automated)
**Date**: 2026-03-22
**File**: `server/src/index.js`
**Scope**: All 11 card proc hook functions across every damage path

---

## 1. Card Hook Function Definitions

| # | Function | Line | Signature | Purpose |
|---|----------|------|-----------|---------|
| 1 | `processCardKillHooks` | 3179 | `(attacker, enemy, attackerId, io)` | On enemy kill: HP/SP gain, Zeny, EXP bonus by race |
| 2 | `processCardDrainEffects` | 3220 | `(attacker, damage, attackerId)` | On hit: HP/SP drain (rate-based + flat) |
| 3 | `processCardStatusProcsOnAttack` | 3247 | `(attacker, target, targetIsEnemy)` | On attack: chance to inflict status (bAddEff, bAddEff2) |
| 4 | `processCardStatusProcsWhenHit` | 3283 | `(target, attacker, attackerIsEnemy)` | When hit: chance to inflict status on attacker (bAddEffWhenHit) |
| 5 | `processCardMeleeReflection` | 3306 | `(target, damage)` | When hit: reflect melee damage (Orc Lord, High Orc) |
| 6 | `processCardMagicReflection` | 3315 | `(target, damage)` | When hit: reflect magic damage (Maya Card) |
| 7 | `knockbackTarget` | 3386 | `(target, sourceX, sourceY, cells, zone, io)` | Knockback with card-based immunity (bNoKnockback) |
| 8 | `processAutobonusOnAttack` | 3450 | `(player, characterId)` | On attack: temporary stat buff proc (Vanberk, Isilla, Atroce) |
| 9 | `processAutobonusWhenHit` | 3473 | `(player, characterId)` | When hit: temporary stat buff proc (Hodremlin, Ice Titan) |
| 10 | `processCardAutoSpellOnAttack` | 3530 | `(attacker, target, targetIsEnemy, zone, io)` | On attack: auto-cast spells (bAutoSpell) |
| 11 | `processCardAutoSpellWhenHit` | 3543 | `(target, attacker, attackerIsEnemy, zone, io)` | When hit: auto-cast spells (bAutoSpellWhenHit) |
| 12 | `processCardDropBonuses` | 3718 | `(attacker, enemy)` | On kill: bonus item drops (bAddMonsterDropItem) |
| 13 | `executeAutoSpellEffect` | 3602 | `(caster, target, targetIsEnemy, skillName, level, zone, io)` | Execute auto-spell damage/heal (called by #10 and #11) |

---

## 2. Auto-Attack Paths

### 2A. PvE Auto-Attack (Player vs Enemy) -- Lines 24930-26030

**Location**: Combat tick loop, `atkState.isEnemy === true` branch

**ON-ATTACK hooks called** (lines 25319-25376, gated by `!isMiss && totalAutoAtkDmg > 0`):

| Hook | Called? | Line | Notes |
|------|---------|------|-------|
| `processCardDrainEffects` | YES | 25323 | `processCardDrainEffects(attacker, totalAutoAtkDmg, attackerId)` |
| `processCardStatusProcsOnAttack` | YES | 25325 | `processCardStatusProcsOnAttack(attacker, enemy, true)` |
| `processCardAutoSpellOnAttack` | YES | 25365 | `processCardAutoSpellOnAttack(attacker, enemy, true, atkEnemyZone, io)` |
| `processAutobonusOnAttack` | YES | 25367 | `processAutobonusOnAttack(attacker, attackerId)` |

**ON-KILL hooks called** (lines 25784-25788, inside `enemy.health <= 0` block):

| Hook | Called? | Line | Notes |
|------|---------|------|-------|
| `processCardKillHooks` | YES | 25785 | `processCardKillHooks(attacker, enemy, attackerId, io)` |
| `processCardDropBonuses` | YES | 25788 | `processCardDropBonuses(attacker, enemy)` |

**VERDICT: COMPLETE** -- All 6 applicable on-attack/on-kill hooks are called.

---

### 2B. PvP Auto-Attack (Player vs Player) -- Lines 26060-26280

**Location**: Combat tick loop, `!atkState.isEnemy` branch

| Hook | Called? | Notes |
|------|---------|-------|
| `processCardDrainEffects` | **NO** | MISSING -- drain should apply on PvP hits |
| `processCardStatusProcsOnAttack` | **NO** | MISSING -- status procs should apply on PvP hits |
| `processCardAutoSpellOnAttack` | **NO** | MISSING -- auto-spells should trigger on PvP hits |
| `processAutobonusOnAttack` | **NO** | MISSING -- autobonus should trigger on PvP hits |
| `processCardStatusProcsWhenHit` | **NO** | MISSING -- target's when-hit procs should trigger |
| `processCardAutoSpellWhenHit` | **NO** | MISSING -- target's when-hit auto-spells should trigger |
| `processAutobonusWhenHit` | **NO** | MISSING -- target's when-hit autobonus should trigger |
| `processCardMeleeReflection` | **NO** | MISSING -- melee reflection should work in PvP |

**VERDICT: 8 VIOLATIONS** -- Zero card hooks are called in PvP auto-attack path.

**MITIGATING FACTOR**: PvP is disabled (`PVP_ENABLED = false` at line 160). This is a latent bug that will manifest when PvP is enabled.

---

### 2C. Dual Wield Left-Hand Hit -- Lines 25173-25246

The left-hand hit in dual wield is part of the PvE auto-attack path. Card hooks are called ONCE after both hands deal damage (using `totalAutoAtkDmg = damage + damage2`). This is correct -- in RO Classic, card procs fire once per attack action, not per hand.

**VERDICT: CORRECT**

---

### 2D. Double Attack (Thief Passive) -- Lines 25547-25588

Double Attack deals an extra hit 200ms after the main attack. It uses `processEnemyDeathFromSkill` for death but does NOT call any card procs.

| Hook | Called? | Notes |
|------|---------|-------|
| `processCardDrainEffects` | **NO** | ACCEPTABLE -- DA is a passive proc, main hit already triggered drain |
| `processCardStatusProcsOnAttack` | **NO** | ACCEPTABLE -- RO Classic: DA is part of the same attack action |

**VERDICT: ACCEPTABLE** -- In RO Classic, Double Attack is part of the same attack action. Card procs fire once per action (already called for the main hit).

---

### 2E. Triple Attack (Monk Passive) -- Lines 25590-25657

Triple Attack deals 3 hits. Uses `processEnemyDeathFromSkill` for death. No card procs called.

**VERDICT: ACCEPTABLE** -- Same reasoning as Double Attack: passive proc within the same attack action. Main hit already triggered card procs.

---

### 2F. Auto-Blitz Beat (Hunter Passive) -- Lines 25659-25712

MISC-type damage. Uses `processEnemyDeathFromSkill` for death. No card procs called.

**VERDICT: ACCEPTABLE** -- MISC damage in RO Classic does not trigger card weapon procs (rAthena: only BF_WEAPON triggers bAutoSpell/bAddEff).

---

### 2G. Splash Auto-Attack (Baphomet Card) -- Lines 25254-25290

Splash targets get independent damage calculations. Uses `processEnemyDeathFromSkill` for death. No card procs called on splash targets.

| Hook | Called? | Notes |
|------|---------|-------|
| `processCardDrainEffects` | **NO** | **VIOLATION** -- Drain should apply per target hit |
| `processCardStatusProcsOnAttack` | **NO** | **VIOLATION** -- Status procs should apply to splash targets |

**VERDICT: 2 VIOLATIONS** -- Splash damage should trigger on-attack card procs per splash target (rAthena: splash is still a weapon hit).

---

## 3. Skill Damage Paths

All player skill damage routes through the `skill:use` socket handler (line 9122) which dispatches to per-skill handlers. Skills go through `processEnemyDeathFromSkill` (line 2138) for enemy kills, which calls `processCardKillHooks` and `processCardDropBonuses`.

### 3A. Kill hooks via `processEnemyDeathFromSkill` (line 2138)

This centralized function is called by ALL skill kill paths. It calls:
- `processCardKillHooks` (line 2195) -- YES
- `processCardDropBonuses` (line 2198) -- YES

**VERDICT: COMPLETE** -- All skill kills go through this function, which has both kill hooks.

### 3B. On-Attack hooks in skill handlers

**NO skill handler calls any of the following:**
- `processCardDrainEffects`
- `processCardStatusProcsOnAttack`
- `processCardAutoSpellOnAttack`
- `processAutobonusOnAttack`

Verified by grep: these four functions are ONLY called at lines 25323, 25325, 25365, 25367 (PvE auto-attack).

**Analysis by skill type:**

| Skill Type | Should trigger on-attack card procs? | RO Classic Reference |
|-----------|--------------------------------------|---------------------|
| Physical melee skills (Bash, Pierce, Bowling Bash, etc.) | CONDITIONALLY | rAthena: skills with BF_WEAPON flag trigger bAutoSpell. Most weapon skills do. |
| Physical ranged skills (Double Strafe, Arrow Shower, etc.) | CONDITIONALLY | Same as above |
| Magic skills (bolts, AoE magic) | NO | rAthena: BF_MAGIC does not trigger bAddEff or bAutoSpell (attack) |
| MISC damage (traps, Grand Cross) | NO | rAthena: BF_MISC does not trigger weapon card procs |

**RO Classic rule (rAthena `battle.cpp`)**:
- `bAddEff` (status on attack): Triggers on BF_WEAPON auto-attacks only (not skills) in pre-renewal
- `bAutoSpell`: Triggers on auto-attacks and SOME weapon skills (those with NK_NO_CARDFIX_ATK unset)
- `bHPDrainRate`/`bSPDrainRate`: Triggers on auto-attacks only in pre-renewal

**VERDICT: MOSTLY ACCEPTABLE** -- In pre-renewal RO, most card on-attack procs only trigger on auto-attacks, not skills. The current implementation is consistent with this. However:

| Hook | Auto-attack only? (pre-renewal) | Current impl | Status |
|------|--------------------------------|--------------|--------|
| `processCardDrainEffects` | YES (pre-renewal) | Auto-attack only | CORRECT |
| `processCardStatusProcsOnAttack` | YES (pre-renewal bAddEff) | Auto-attack only | CORRECT |
| `processCardAutoSpellOnAttack` | MOSTLY (auto-attack + some skills) | Auto-attack only | MINOR GAP |
| `processAutobonusOnAttack` | Auto-attack primary | Auto-attack only | ACCEPTABLE |

The only minor gap is `bAutoSpell` not triggering on weapon skills. In rAthena pre-renewal, bAutoSpell CAN trigger on weapon skills that don't have `NK_NO_CARDFIX_ATK`. This is a low-priority enhancement, not a bug.

---

## 4. Enemy-Attacks-Player Paths (When-Hit Hooks)

### 4A. Enemy Auto-Attack on Player -- Lines 30628-31167

**Location**: Enemy AI tick, `AI_STATE.ATTACK` branch

**WHEN-HIT hooks called** (lines 30960-30975, gated by `!isMiss && damage > 0`):

| Hook | Called? | Line | Notes |
|------|---------|------|-------|
| `processCardStatusProcsWhenHit` | YES | 30963 | Correct |
| `processCardAutoSpellWhenHit` | YES | 30965 | Correct |
| `processAutobonusWhenHit` | YES | 30967 | Correct |
| `processCardMeleeReflection` | YES | 30969 | Correct |
| `processCardMagicReflection` | N/A | -- | Enemy auto-attacks are physical, not magic |

**VERDICT: COMPLETE** -- All 4 applicable when-hit hooks are called.

---

### 4B. `executeMonsterPlayerSkill` (Monster uses player-class skill) -- Lines 29015-29240

Damage is dealt at line 29184. Player death checked at line 29215.

| Hook | Called? | Notes |
|------|---------|-------|
| `processCardStatusProcsWhenHit` | **NO** | MISSING |
| `processCardAutoSpellWhenHit` | **NO** | MISSING |
| `processAutobonusWhenHit` | **NO** | MISSING |
| `processCardMeleeReflection` | **NO** | MISSING (for physical monster skills) |
| `processCardMagicReflection` | **NO** | MISSING (for magical monster skills) |

**VERDICT: 5 VIOLATIONS** -- Monster skills targeting players trigger zero when-hit card procs.

---

### 4C. `executeNPCSkill` -- elemental_melee (Lines 29248-29284)

Damage at line 29269. No card when-hit hooks called.

| Hook | Called? | Notes |
|------|---------|-------|
| `processCardStatusProcsWhenHit` | **NO** | MISSING |
| `processCardAutoSpellWhenHit` | **NO** | MISSING |
| `processAutobonusWhenHit` | **NO** | MISSING |
| `processCardMeleeReflection` | **NO** | MISSING |

**VERDICT: 4 VIOLATIONS**

---

### 4D. `executeNPCSkill` -- status_melee (Lines 29287-29340)

Damage at line 29306. No card when-hit hooks called.

**VERDICT: 4 VIOLATIONS** (same 4 hooks missing as 4C)

---

### 4E. `executeNPCSkill` -- multi_hit (Lines 29342-29368)

Multiple damage instances. No card when-hit hooks called.

**VERDICT: 4 VIOLATIONS** (same 4 hooks missing)

---

### 4F. `executeNPCSkill` -- forced_crit (Lines 29370-29392)

Single crit damage. No card when-hit hooks called.

**VERDICT: 4 VIOLATIONS** (same 4 hooks missing)

---

### 4G. `executeNPCSkill` -- aoe_physical (Lines 29394-29432)

AoE damage to all players in radius. No card when-hit hooks called.

**VERDICT: 4 VIOLATIONS** (same 4 hooks missing per player hit)

---

### 4H. `executeNPCSkill` -- drain_hp (Lines 29457+)

HP drain attack on player. No card when-hit hooks called.

**VERDICT: 4 VIOLATIONS** (same 4 hooks missing)

---

### 4I. Ground Effect Damage on Players (Fire Wall PvP, Storm Gust PvP, etc.)

Ground AoE ticks (Storm Gust, LoV, Meteor Storm, etc.) deal damage to enemies via the ground effect tick loop. When these deal damage to PLAYERS (PvP mode or monster-cast ground effects), no when-hit card hooks are called.

**VERDICT: LATENT VIOLATIONS** -- These only affect PvP paths or monster-cast ground effects on players. Low priority since PvP is disabled and monster ground AoE is rare.

---

### 4J. Venom Splasher Detonation -- Lines 24817-24895

Deals AoE damage. Uses `processEnemyDeathFromSkill` for kills. No on-attack card procs called.

**VERDICT: ACCEPTABLE** -- Venom Splasher is a delayed MISC-type AoE (not a direct weapon attack). Card weapon procs should not trigger.

---

## 5. Kill Path Verification

### 5A. Auto-Attack Kill Path (Lines 25714-26030)

| Hook | Called? | Line |
|------|---------|------|
| `processCardKillHooks` | YES | 25785 |
| `processCardDropBonuses` | YES | 25788 |

**VERDICT: COMPLETE**

### 5B. `processEnemyDeathFromSkill` (Line 2138) -- Centralized Skill Kill

| Hook | Called? | Line |
|------|---------|------|
| `processCardKillHooks` | YES | 2195 |
| `processCardDropBonuses` | YES | 2198 |

**VERDICT: COMPLETE** -- Used by all ~80+ skill kill paths.

### 5C. PvP Kill Path (Lines 26222-26276)

Player kills are not enemy kills -- `processCardKillHooks` and `processCardDropBonuses` are designed for monster kills (race-based bonuses, item drops). Not applicable to PvP.

**VERDICT: N/A** (correct behavior)

### 5D. Sub-system Kill Paths

| Path | Kill Hook Called? | Via |
|------|------------------|-----|
| Splash auto-attack (Baphomet) | YES | `processEnemyDeathFromSkill` at 25286 |
| Double Attack kill | YES | `processEnemyDeathFromSkill` at 25584 |
| Triple Attack kill | YES | `processEnemyDeathFromSkill` at 25640 |
| Auto-Blitz Beat kill | YES | `processEnemyDeathFromSkill` at 25706 |
| Hindsight autocast kill | YES | `processEnemyDeathFromSkill` at 25443 |
| Ground effect kills (SG/LoV/MS/FW) | YES | `processEnemyDeathFromSkill` at 27795/27954/28027/28107 |
| Trap kills | YES | `processEnemyDeathFromSkill` at 31210/31240/31255/31271 |
| Dissonance kill | YES | `processEnemyDeathFromSkill` at 27263 |
| Plant/Homunculus kills | YES | `processEnemyDeathFromSkill` at 26359/26506 |
| Reflect Shield kill | YES | `processEnemyDeathFromSkill` at 30948 |
| Poison React counter kill | YES | `processEnemyDeathFromSkill` at 31029/31072 |
| Auto Counter kill | YES | `processEnemyDeathFromSkill` at 30675 |
| Marine Sphere detonation | YES | `processEnemyDeathFromSkill` at 26394 |

**VERDICT: COMPLETE** -- All enemy kill paths go through `processEnemyDeathFromSkill`.

---

## 6. Hook Call Order Verification

### 6A. PvE Auto-Attack Path

```
1. Calculate damage (line 25046)
2. Apply damage to enemy HP (line 25169)
3. Card on-attack hooks (lines 25319-25376)  <-- AFTER damage, BEFORE death
4. Status breaks (line 25506)
5. Arrow consumption (line 25516)
6. Enemy death check (line 25715)
7. Card kill hooks (line 25785)               <-- INSIDE death block
8. Card drop bonuses (line 25788)             <-- INSIDE death block
9. EXP/drops/loot
```

**VERDICT: CORRECT ORDER** -- On-attack hooks fire after damage but before death processing. Kill hooks fire inside the death block.

### 6B. Enemy Auto-Attack on Player Path

```
1. Calculate damage (line 30631)
2. Buff checks (Auto Counter, Blade Stop, Auto Guard, Defender, Kyrie)
3. Apply damage to player HP (line 30906)
4. Card when-hit hooks (lines 30960-30975)    <-- AFTER damage
5. Player death check (line 31114)
```

**VERDICT: CORRECT ORDER** -- When-hit hooks fire after damage and before death.

### 6C. `processEnemyDeathFromSkill`

```
1. Set isDead (line 2139)
2. Clean up root_lock, close_confine, slave (lines 2143-2178)
3. Card kill hooks (line 2195)
4. Card drop bonuses (line 2198)
5. EXP distribution (line 2225+)
6. Loot distribution (line 2358+)
```

**VERDICT: CORRECT ORDER** -- Kill hooks fire before EXP/drops.

---

## 7. Violation Summary

### CRITICAL (will cause incorrect behavior in active game paths)

| # | Path | Missing Hook(s) | Severity | Impact |
|---|------|-----------------|----------|--------|
| V1 | `executeMonsterPlayerSkill` (line 29015) | `processCardStatusProcsWhenHit`, `processCardAutoSpellWhenHit`, `processAutobonusWhenHit` | **HIGH** | Monster skills (Cold Bolt, Fire Bolt, Bash, etc. cast by monsters) will not trigger player's when-hit card procs. Affects cards like Orc Hero (stun attacker), Maya Purple (auto-cast on hit), Hodremlin (autobonus when hit). |
| V2 | `executeNPCSkill` -- elemental_melee (line 29248) | Same 4 when-hit hooks | **HIGH** | Element-forced melee attacks by monsters will not trigger when-hit procs. |
| V3 | `executeNPCSkill` -- status_melee (line 29287) | Same 4 when-hit hooks | **HIGH** | Status-inflicting melee attacks by monsters will not trigger when-hit procs. |
| V4 | `executeNPCSkill` -- multi_hit (line 29342) | Same 4 when-hit hooks | **MEDIUM** | Multi-hit NPC attacks will not trigger when-hit procs. |
| V5 | `executeNPCSkill` -- forced_crit (line 29370) | Same 4 when-hit hooks | **MEDIUM** | Forced critical NPC attacks will not trigger when-hit procs. |
| V6 | `executeNPCSkill` -- aoe_physical (line 29394) | Same 4 when-hit hooks | **MEDIUM** | AoE physical NPC attacks (NPC_EARTHQUAKE) will not trigger when-hit procs per hit player. |

### MODERATE (latent issues -- PvP currently disabled)

| # | Path | Missing Hook(s) | Severity | Impact |
|---|------|-----------------|----------|--------|
| V7 | PvP auto-attack (line 26060) | All 8: drain, status on attack, autospell on attack, autobonus on attack, status when hit, autospell when hit, autobonus when hit, melee reflection | **MODERATE** | PvP is disabled (`PVP_ENABLED = false`). When enabled, zero card procs will work in PvP auto-attacks. |
| V8 | Splash auto-attack (Baphomet, line 25254) | `processCardDrainEffects`, `processCardStatusProcsOnAttack` | **LOW** | Splash targets don't trigger drain or status procs. In rAthena, splash IS a weapon hit and DOES trigger on-attack procs. Low priority since Baphomet Card is rare. |

### INFORMATIONAL (correct behavior per RO Classic rules)

| # | Path | Status | Notes |
|---|------|--------|-------|
| I1 | Skill handlers (all) | No on-attack procs | CORRECT in pre-renewal: bAddEff/bHPDrainRate are auto-attack only |
| I2 | Double Attack / Triple Attack | No extra procs | CORRECT: part of same attack action |
| I3 | Auto-Blitz Beat | No weapon procs | CORRECT: MISC damage does not trigger weapon card procs |
| I4 | Traps | No weapon procs | CORRECT: MISC/Trap damage does not trigger weapon card procs |
| I5 | Venom Splasher | No weapon procs | CORRECT: delayed MISC AoE |
| I6 | Hindsight autocast | No weapon procs | CORRECT: magic autocast from melee, not a weapon hit itself |
| I7 | Ground effects on players | No when-hit procs | LOW PRIORITY: these are magic AoE ticks, rare in current content |
| I8 | PvP kills | No kill hooks | CORRECT: kill hooks are monster-only (race bonuses, drop bonuses) |

---

## 8. Recommendations

### Priority 1: Fix Monster Skill When-Hit Procs (V1-V6)

Add when-hit card hooks to `executeMonsterPlayerSkill` and all `executeNPCSkill` damage branches. Insert after damage application, before player death check:

```javascript
// After: target.health = Math.max(0, target.health - damage);
// Before: if (target.health <= 0) { ... death handling ... }

if (!isMiss && damage > 0) {
    processCardStatusProcsWhenHit(target, enemy, true);
    processCardAutoSpellWhenHit(target, enemy, true, zone, io);
    processAutobonusWhenHit(target, targetCharId);
    // Melee reflection only for physical (non-magic) attacks:
    if (!isMagic) {
        const reflDmg = processCardMeleeReflection(target, damage);
        if (reflDmg > 0 && !enemy.isDead) {
            enemy.health = Math.max(0, enemy.health - reflDmg);
            broadcastToZone(zone, 'enemy:health_update', {
                enemyId: enemy.enemyId, health: enemy.health,
                maxHealth: enemy.maxHealth, inCombat: true
            });
        }
    } else {
        const reflDmg = processCardMagicReflection(target, damage);
        if (reflDmg > 0 && !enemy.isDead) {
            enemy.health = Math.max(0, enemy.health - reflDmg);
            broadcastToZone(zone, 'enemy:health_update', {
                enemyId: enemy.enemyId, health: enemy.health,
                maxHealth: enemy.maxHealth, inCombat: true
            });
        }
    }
}
```

**Files to modify**: `server/src/index.js`
- `executeMonsterPlayerSkill` (line ~29183, after damage application)
- `executeNPCSkill` case `elemental_melee` (line ~29269)
- `executeNPCSkill` case `status_melee` (line ~29306)
- `executeNPCSkill` case `multi_hit` (line ~29351, inside hit loop)
- `executeNPCSkill` case `forced_crit` (line ~29377)
- `executeNPCSkill` case `aoe_physical` (line ~29404, inside player loop)
- `executeNPCSkill` case `drain_hp` (line ~29463)

### Priority 2: Fix PvP Auto-Attack Card Procs (V7)

When PvP is eventually enabled, add the same card hook block used in PvE auto-attack (lines 25319-25376) to the PvP auto-attack path (after line 26188):

```javascript
// ON-ATTACK hooks (attacker's cards)
if (!pvpMiss && pvpDmg > 0) {
    processCardDrainEffects(attacker, pvpDmg, attackerId);
    processCardStatusProcsOnAttack(attacker, target, false);
    processCardAutoSpellOnAttack(attacker, target, false, pvpAtkZone, io);
    processAutobonusOnAttack(attacker, attackerId);
}
// WHEN-HIT hooks (target's cards)
if (!pvpMiss && pvpDmg > 0) {
    processCardStatusProcsWhenHit(target, attacker, false);
    processCardAutoSpellWhenHit(target, attacker, false, pvpAtkZone, io);
    processAutobonusWhenHit(target, atkState.targetCharId);
    const reflDmg = processCardMeleeReflection(target, pvpDmg);
    if (reflDmg > 0) {
        attacker.health = Math.max(0, attacker.health - reflDmg);
        // Broadcast reflected damage + health update
    }
}
```

### Priority 3: Splash Auto-Attack Procs (V8)

Add `processCardDrainEffects` and `processCardStatusProcsOnAttack` inside the splash loop (line ~25269), after damage is applied to each splash target. Low priority.

---

## 9. Summary Statistics

| Category | Count |
|----------|-------|
| Card hook functions audited | 13 |
| Damage paths audited | 18+ |
| Kill paths verified | 15+ |
| Hooks correctly placed | 10 call sites |
| **CRITICAL violations** | **6** (V1-V6: monster skill when-hit procs) |
| **MODERATE violations** | **2** (V7-V8: PvP + splash, both latent) |
| Correct non-triggers (informational) | 8 (I1-I8) |
| Hook call order | CORRECT in all paths |
| Kill hook coverage | 100% (all kills route through `processEnemyDeathFromSkill`) |
