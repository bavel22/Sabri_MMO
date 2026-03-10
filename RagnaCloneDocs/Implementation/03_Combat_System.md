# 03 -- Combat System Implementation Guide

> Complete UE5 C++ and Node.js implementation guide for the Ragnarok Online Classic (Pre-Renewal) combat system.
> All code is designed for Claude Code automation -- no manual Blueprint input required.
> Server-authoritative architecture: all damage, hit/miss, and state changes happen server-side.

---

## Table of Contents

1. [Server-Side Combat Engine (Node.js)](#1-server-side-combat-engine-nodejs)
2. [Status Effect System (Server)](#2-status-effect-system-server)
3. [Element System (Server)](#3-element-system-server)
4. [Client-Side: Combat Subsystem](#4-client-side-combat-subsystem)
5. [Client-Side: Damage Numbers](#5-client-side-damage-numbers)
6. [Client-Side: Attack Animations](#6-client-side-attack-animations)
7. [Client-Side: Status Effect Display](#7-client-side-status-effect-display)
8. [Auto-Attack Flow (End-to-End)](#8-auto-attack-flow-end-to-end)

---

## 1. Server-Side Combat Engine (Node.js)

### 1.1 Architecture Overview

The combat engine lives in two server files:

| File | Purpose |
|------|---------|
| `server/src/ro_damage_formulas.js` | Pure math: element table, size penalty, stat calculations, physical/magical damage pipelines |
| `server/src/index.js` | Combat tick loop, auto-attack state machine, skill execution, socket event handlers |

The combat tick runs at a 50ms interval (`COMBAT.COMBAT_TICK_MS = 50`). Each tick processes:
1. Cast completion checks (iterates `activeCasts` Map)
2. Auto-attack processing (iterates `autoAttackState` Map)

Enemy AI runs on a separate 200ms tick but uses the same damage formulas.

### 1.2 Combat Tick Loop

The combat tick is a `setInterval` at 50ms. It is the heartbeat of all combat processing.

```javascript
// server/src/index.js — Combat tick loop (already implemented)
// Runs every COMBAT_TICK_MS (50ms)
setInterval(async () => {
    const now = Date.now();

    // ── Phase 1: Cast Completion ──
    // Check if any active casts have finished their cast time
    for (const [charId, cast] of activeCasts.entries()) {
        if (now >= cast.castEndTime) {
            activeCasts.delete(charId);
            executeCastComplete(charId, cast);
        }
    }

    // ── Phase 2: Auto-Attack Processing ──
    for (const [attackerId, atkState] of autoAttackState.entries()) {
        const attacker = connectedPlayers.get(attackerId);
        if (!attacker) {
            autoAttackState.delete(attackerId);
            continue;
        }

        // Dead attackers can't attack
        if (attacker.isDead) {
            autoAttackState.delete(attackerId);
            notifyPlayer(attacker, 'combat:auto_attack_stopped', { reason: 'You died' });
            continue;
        }

        // CC check: frozen/stoned attackers skip their turn
        const ccMods = getBuffStatModifiers(attacker);
        if (ccMods.isFrozen || ccMods.isStoned) continue;

        if (atkState.isEnemy) {
            await processEnemyAutoAttack(attackerId, attacker, atkState, now);
        } else {
            await processPlayerAutoAttack(attackerId, attacker, atkState, now);
        }
    }
}, COMBAT.COMBAT_TICK_MS);
```

**Key design decisions:**
- The loop is `async` because `getPlayerPosition()` reads from the database/cache
- Each attacker is processed independently -- one error does not block others
- CC (crowd control) checks happen at the top, before any damage calculation
- Dead attackers are immediately cleaned out of the state map

### 1.3 Auto-Attack State Machine

The auto-attack system uses a `Map<characterId, AttackState>` to track who is attacking whom.

```
State Machine:
  IDLE ──(combat:attack)──> ATTACKING ──(target dies/leaves)──> IDLE
                               │
                               ├── Every tick: check ASPD timer
                               │     └── Timer elapsed? → Calculate damage → broadcast
                               │
                               ├── Out of range? → emit combat:out_of_range (client moves toward target)
                               │
                               └── (combat:stop) → IDLE
```

**State structure (per attacker):**

```javascript
// autoAttackState.set(attackerId, { ... })
{
    targetCharId: Number,    // Enemy ID or player character ID
    isEnemy: Boolean,        // true = PvE target, false = PvP target
    startTime: Number        // Date.now() when attack started
}
```

**Entry point -- `combat:attack` socket event handler:**

```javascript
socket.on('combat:attack', (data) => {
    const targetCharacterId = data.targetCharacterId != null
        ? parseInt(data.targetCharacterId) : undefined;
    const targetEnemyId = data.targetEnemyId != null
        ? parseInt(data.targetEnemyId) : undefined;

    const attackerInfo = findPlayerBySocketId(socket.id);
    if (!attackerInfo) return;
    const { characterId: attackerId, player: attacker } = attackerInfo;

    // Validation
    if (attacker.isDead) {
        socket.emit('combat:error', { message: 'You are dead' });
        return;
    }

    // Clean up previous target if switching
    if (autoAttackState.has(attackerId)) {
        const oldAtk = autoAttackState.get(attackerId);
        const newTargetId = targetEnemyId ?? targetCharacterId;
        const newIsEnemy = targetEnemyId != null;

        if (oldAtk.targetCharId !== newTargetId || oldAtk.isEnemy !== newIsEnemy) {
            if (oldAtk.isEnemy) {
                const oldEnemy = enemies.get(oldAtk.targetCharId);
                if (oldEnemy) oldEnemy.inCombatWith.delete(attackerId);
            }
            autoAttackState.delete(attackerId);
            socket.emit('combat:auto_attack_stopped', {
                reason: 'Switched target',
                oldTargetId: oldAtk.targetCharId,
                oldIsEnemy: oldAtk.isEnemy
            });
        }
    }

    // Register new auto-attack
    if (targetEnemyId != null) {
        const enemy = enemies.get(targetEnemyId);
        if (!enemy || enemy.isDead) {
            socket.emit('combat:error', { message: enemy ? 'Enemy is already dead' : 'Enemy not found' });
            return;
        }

        autoAttackState.set(attackerId, {
            targetCharId: targetEnemyId,
            isEnemy: true,
            startTime: Date.now()
        });

        enemy.inCombatWith.add(attackerId);
        enemy.isWandering = false;
        setEnemyAggro(enemy, attackerId, 'melee');

        socket.emit('combat:auto_attack_started', {
            targetId: targetEnemyId,
            targetName: enemy.name,
            isEnemy: true,
            attackRange: attacker.attackRange,
            aspd: attacker.aspd,
            attackIntervalMs: getAttackIntervalMs(attacker.aspd)
        });
    }
});
```

### 1.4 ASPD-Based Attack Timing

ASPD determines how often a character can swing. The conversion from ASPD to milliseconds:

```javascript
// server/src/index.js — ASPD to attack interval conversion
function getAttackIntervalMs(aspd) {
    // Pre-Renewal formula: delay = (200 - ASPD) / 50 seconds
    // Converted to ms: delay = (200 - ASPD) * 20
    // Clamped: minimum 200ms (ASPD 190), maximum 2000ms (ASPD 100)
    const delayMs = (200 - Math.min(190, Math.max(100, aspd))) * 20;
    return Math.max(200, delayMs);
}

// Reference table:
// ASPD 150 → 1000ms (1.0 hits/sec)
// ASPD 160 →  800ms (1.25 hits/sec)
// ASPD 170 →  600ms (1.67 hits/sec)
// ASPD 175 →  500ms (2.0 hits/sec)
// ASPD 180 →  400ms (2.5 hits/sec)
// ASPD 185 →  300ms (3.33 hits/sec)
// ASPD 190 →  200ms (5.0 hits/sec) — hard cap
```

### 1.5 Physical Damage Pipeline (16 Steps)

The complete physical damage calculation in `ro_damage_formulas.js`. This is the already-implemented version with inline comments mapping to each canonical step.

```javascript
// server/src/ro_damage_formulas.js — calculatePhysicalDamage()
// Full RO Pre-Renewal physical damage pipeline
function calculatePhysicalDamage(attacker, target, options = {}) {
    const {
        isSkill = false,
        skillMultiplier = 100,       // 100 = 1x damage
        skillHitBonus = 0,
        forceHit = false,            // Skills like Bash ignore FLEE
        forceCrit = false,
        skillElement = null
    } = options;

    const atkDerived = calculateDerivedStats(attacker.stats || attacker);
    const defDerived = calculateDerivedStats(target.stats || target);

    const result = {
        damage: 0, hitType: 'normal', isCritical: false, isMiss: false,
        element: attacker.weaponElement || 'neutral',
        sizePenalty: 100, elementModifier: 100
    };

    const atkElement = skillElement || attacker.weaponElement || 'neutral';
    result.element = atkElement;

    const targetElement = target.element?.type || 'neutral';
    const targetElementLevel = target.element?.level || 1;
    const targetSize = target.size || 'medium';
    const targetRace = target.race || 'formless';

    // ────────── STEP 1: Perfect Dodge ──────────
    // PD = 1 + floor(LUK/10). Each PD = 1% chance to evade.
    // Critical attacks bypass PD. Skills cannot be Perfect Dodged.
    if (!forceHit && !forceCrit && !isSkill) {
        const pd = defDerived.perfectDodge;
        if (Math.random() * 100 < pd) {
            result.hitType = 'perfectDodge';
            result.isMiss = true;
            return result;
        }
    }

    // ────────── STEP 2: Critical Check ──────────
    // CRI = 1 + floor(LUK*0.3) + equipBonus
    // CritShield = floor(targetLUK * 0.2)
    // Skills cannot crit in pre-Renewal (with rare exceptions)
    let isCritical = false;
    if (!isSkill) {
        if (forceCrit) {
            isCritical = true;
        } else {
            const targetLuk = (target.stats || target).luk || 1;
            const effectiveCrit = calculateCritRate(atkDerived.critical, targetLuk);
            if (Math.random() * 100 < effectiveCrit) {
                isCritical = true;
            }
        }
    }

    // ────────── STEP 3: HIT/FLEE Check ──────────
    // Skipped on critical hits (they always land)
    // HitRate = 80 + AttackerHIT - DefenderFLEE, clamped 5-95%
    // Multi-monster FLEE penalty: -10% FLEE per attacker beyond 2
    if (!isCritical && !forceHit) {
        const effectiveHit = atkDerived.hit + skillHitBonus;
        const numAttackers = target.numAttackers || 1;
        const hitRate = calculateHitRate(effectiveHit, defDerived.flee, numAttackers);
        if (Math.random() * 100 >= hitRate) {
            result.hitType = 'miss';
            result.isMiss = true;
            return result;
        }
    }

    // ────────── STEP 4: StatusATK ──────────
    // Melee: STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
    const statusATK = atkDerived.statusATK;

    // ────────── STEP 5: WeaponATK ──────────
    const weaponATK = attacker.weaponATK || 0;
    const passiveATK = attacker.passiveATK || 0;

    // ────────── STEP 6: Size Penalty ──────────
    // Applies to WeaponATK portion only (not StatusATK or MasteryATK)
    const weaponType = attacker.weaponType || 'bare_hand';
    const sizePenaltyPct = getSizePenalty(weaponType, targetSize);
    result.sizePenalty = sizePenaltyPct;
    const sizedWeaponATK = Math.floor(weaponATK * sizePenaltyPct / 100);

    // ────────── STEP 7: Damage Variance ──────────
    // Weapon level affects spread: L1=+/-5%, L2=+/-10%, L3=+/-15%, L4=+/-20%
    // Critical: always max ATK (no variance roll)
    const weaponLevel = attacker.weaponLevel || 1;
    let variancedWeaponATK;
    if (isCritical) {
        variancedWeaponATK = sizedWeaponATK;
    } else {
        const varianceMin = 1.0 - weaponLevel * 0.05;
        const varianceMul = varianceMin + Math.random() * (1.0 - varianceMin);
        variancedWeaponATK = Math.floor(sizedWeaponATK * varianceMul);
    }

    // ────────── STEP 8: Sum Base Damage ──────────
    let totalATK = statusATK + variancedWeaponATK + passiveATK;

    // ────────── STEP 9: Buff Multiplier ──────────
    // Provoke: +ATK%, Power Thrust: +5% per level, etc.
    const atkMultiplier = attacker.buffMods?.atkMultiplier || 1.0;
    totalATK = Math.floor(totalATK * atkMultiplier);

    // ────────── STEP 10: Skill Multiplier ──────────
    // Bash Lv10 = 400%, Bowling Bash Lv10 = 500%, etc.
    if (isSkill && skillMultiplier !== 100) {
        totalATK = Math.floor(totalATK * skillMultiplier / 100);
    }

    // ────────── STEP 11: Card Bonuses ──────────
    // Race%, Element%, Size% are additive with each other, then multiplicative
    let cardBonus = 0;
    if (attacker.cardMods) {
        cardBonus += attacker.cardMods[`race_${targetRace}`] || 0;
        cardBonus += attacker.cardMods[`ele_${targetElement}`] || 0;
        cardBonus += attacker.cardMods[`size_${targetSize}`] || 0;
    }
    if (cardBonus !== 0) {
        totalATK = Math.floor(totalATK * (100 + cardBonus) / 100);
    }

    // ────────── STEP 12: Element Modifier ──────────
    // 10x10x4 table lookup: ELEMENT_TABLE[atkElement][defElement][defLevel-1]
    const eleModifier = getElementModifier(atkElement, targetElement, targetElementLevel);
    result.elementModifier = eleModifier;
    if (eleModifier <= 0) {
        result.damage = 0;
        result.hitType = 'miss';
        result.isMiss = true;
        return result;
    }
    totalATK = Math.floor(totalATK * eleModifier / 100);

    // ────────── STEP 13: Hard DEF (percentage) ──────────
    // Equipment DEF, capped at 99%
    const hardDef = Math.min(99, target.hardDef || 0);
    const defMultiplier = target.buffMods?.defMultiplier || 1.0;
    if (hardDef > 0) {
        const effectiveHardDef = Math.floor(hardDef * defMultiplier);
        totalATK = Math.floor(totalATK * (100 - effectiveHardDef) / 100);
    }

    // ────────── STEP 14: Soft DEF (flat) ──────────
    // VIT-based: floor(VIT/2) + max(1, floor((VIT*2-1)/3))
    const effectiveSoftDef = Math.floor(defDerived.softDEF * defMultiplier);
    totalATK = totalATK - effectiveSoftDef;

    // ────────── STEP 15: MasteryATK (already in passiveATK, step 8) ──────────

    // ────────── STEP 16: Floor to minimum 1 ──────────
    result.damage = Math.max(1, totalATK);
    result.isCritical = isCritical;
    result.hitType = isCritical ? 'critical' : 'normal';

    // Critical damage bonus: 40% extra damage
    if (isCritical) {
        result.damage = Math.floor(result.damage * 1.4);
    }

    return result;
}
```

### 1.6 Magical Damage Pipeline (9 Steps)

```javascript
// server/src/ro_damage_formulas.js — calculateMagicalDamage()
function calculateMagicalDamage(attacker, target, options = {}) {
    const { skillMultiplier = 100, skillElement = 'neutral' } = options;

    const atkDerived = calculateDerivedStats(attacker.stats || attacker);
    const defDerived = calculateDerivedStats(target.stats || target);

    const result = {
        damage: 0, hitType: 'magical', isCritical: false, isMiss: false,
        element: skillElement, elementModifier: 100
    };

    // ── Step 1: StatusMATK = INT + floor(INT/7)^2 ──
    const weaponMATK = attacker.weaponMATK || 0;

    // ── Step 2: MATK range ──
    // Max = statusMATK + weaponMATK
    // Min = floor(statusMATK * 0.7) + floor(weaponMATK * 0.7)
    const matkMax = atkDerived.statusMATK + weaponMATK;
    const matkMin = Math.floor(atkDerived.statusMATK * 0.7) + Math.floor(weaponMATK * 0.7);

    // ── Step 3: Roll damage in range ──
    const matk = matkMin + Math.floor(Math.random() * (matkMax - matkMin + 1));

    // ── Step 4: Skill Multiplier ──
    // Fire Bolt Lv10 = 1000% (10 hits x 100%), Fire Ball Lv10 = 270%
    let totalDamage = Math.floor(matk * skillMultiplier / 100);

    // ── Step 5: Buff Modifiers ──
    const atkMultiplier = attacker.buffMods?.atkMultiplier || 1.0;
    totalDamage = Math.floor(totalDamage * atkMultiplier);

    // ── Step 6: Element Modifier ──
    const targetElement = target.element?.type || 'neutral';
    const targetElementLevel = target.element?.level || 1;
    const eleModifier = getElementModifier(skillElement, targetElement, targetElementLevel);
    result.elementModifier = eleModifier;

    if (eleModifier <= 0) {
        result.damage = 0;
        result.hitType = 'miss';
        result.isMiss = true;
        return result;
    }
    totalDamage = Math.floor(totalDamage * eleModifier / 100);

    // ── Step 7: Hard MDEF (percentage) ──
    const hardMdef = Math.min(99, target.hardMdef || target.magicDefense || 0);
    const defMultiplier = target.buffMods?.defMultiplier || 1.0;
    if (hardMdef > 0) {
        const effectiveHardMdef = Math.floor(hardMdef * defMultiplier);
        totalDamage = Math.floor(totalDamage * (100 - effectiveHardMdef) / 100);
    }

    // ── Step 8: Soft MDEF (flat) ──
    const effectiveSoftMdef = Math.floor(defDerived.softMDEF * defMultiplier);
    totalDamage = totalDamage - effectiveSoftMdef;

    const buffBonusMDEF = target.buffMods?.bonusMDEF || 0;
    totalDamage = totalDamage - buffBonusMDEF;

    // ── Step 9: Floor to minimum 1 ──
    result.damage = Math.max(1, totalDamage);
    return result;
}
```

### 1.7 HIT/FLEE Calculation

```javascript
// HIT = 175 + BaseLv + DEX + BonusHIT
// FLEE = 100 + BaseLv + AGI + BonusFLEE
// HitRate% = 80 + AttackerHIT - DefenderFLEE, clamped [5, 95]

function calculateHitRate(attackerHit, targetFlee, numAttackers = 1) {
    // Multi-monster FLEE penalty: -10 FLEE per attacker beyond 2
    // At 11+ attackers, effective FLEE = 0 (always hit)
    let effectiveFlee = targetFlee;
    if (numAttackers > 2) {
        const fleePenalty = (numAttackers - 2) * 10;
        effectiveFlee = Math.max(0, effectiveFlee - fleePenalty);
    }
    const hitRate = 80 + attackerHit - effectiveFlee;
    return Math.max(5, Math.min(95, hitRate));
}
```

### 1.8 Critical Hit System

```javascript
function calculateCritRate(attackerCri, targetLuk) {
    // CRI = 1 + floor(LUK * 0.3) + equipBonus  (calculated in derivedStats)
    // CritShield = floor(targetLUK * 0.2)
    // Katar weapons: double displayed crit rate
    const critShield = Math.floor(targetLuk * 0.2);
    return Math.max(0, attackerCri - critShield);
}

// Critical hit properties:
// - Always max WeaponATK (no variance)
// - Bypass FLEE check
// - Bypass Perfect Dodge
// - +40% damage bonus (damage * 1.4)
// - Skills cannot crit in pre-Renewal
```

### 1.9 Perfect Dodge

```javascript
// Perfect Dodge = 1 + floor(LUK / 10) + BonusPD
// Checked BEFORE HIT/FLEE
// Not affected by multi-monster FLEE penalty
// Critical hits BYPASS Perfect Dodge
// Only applies to auto-attacks (not skills)

// In the damage pipeline (Step 1):
if (!forceHit && !forceCrit && !isSkill) {
    const pd = defDerived.perfectDodge;  // 1 + floor(LUK/10)
    if (Math.random() * 100 < pd) {
        return { hitType: 'perfectDodge', isMiss: true, damage: 0 };
    }
}
```

### 1.10 Complete `ro_damage_formulas.js` Module

The module exports the following public API. This is the already-implemented file at `server/src/ro_damage_formulas.js`:

```javascript
module.exports = {
    // ── Tables ──
    ELEMENT_TABLE,         // 10x10x4 element effectiveness
    SIZE_PENALTY,          // 17 weapon types x 3 sizes

    // ── Stat calculations ──
    calculateDerivedStats, // { str, agi, vit, int, dex, luk, level } → derived stats
    getElementModifier,    // (atkEle, defEle, defLv) → percentage
    getSizePenalty,        // (weaponType, targetSize) → percentage

    // ── Hit/Crit checks ──
    calculateHitRate,      // (hit, flee, numAttackers) → 5-95%
    calculateCritRate,     // (cri, targetLuk) → effective crit %

    // ── Damage pipelines ──
    calculatePhysicalDamage,  // Full 16-step physical pipeline
    calculateMagicalDamage    // Full 9-step magical pipeline
};
```

### 1.11 Helper Functions Used by the Combat Tick

These utility functions support the combat tick loop in `index.js`:

```javascript
// Get player position from cache/database
async function getPlayerPosition(characterId) {
    const player = connectedPlayers.get(characterId);
    if (!player) return null;
    return { x: player.lastX, y: player.lastY, z: player.lastZ };
}

// Build attacker info object for damage formula
function getAttackerInfo(player) {
    return {
        stats: getEffectiveStats(player),
        weaponATK: player.weaponATK || 0,
        passiveATK: player.passiveATK || 0,
        weaponType: player.weaponType || 'bare_hand',
        weaponElement: player.weaponElement || 'neutral',
        weaponLevel: player.weaponLevel || 1,
        buffMods: getBuffStatModifiers(player),
        cardMods: player.cardMods || {}
    };
}

// Build enemy target info object for damage formula
function getEnemyTargetInfo(enemy) {
    return {
        stats: enemy.stats,
        hardDef: enemy.hardDef || 0,
        element: enemy.element || { type: 'neutral', level: 1 },
        size: enemy.size || 'medium',
        race: enemy.race || 'formless',
        numAttackers: enemy.inCombatWith ? enemy.inCombatWith.size : 1,
        buffMods: getBuffStatModifiers(enemy)
    };
}

// Get effective stats including buff modifications
function getEffectiveStats(entity) {
    const base = entity.stats || entity;
    const mods = getBuffStatModifiers(entity);
    return {
        str: (base.str || 1) + (mods.bonusSTR || 0),
        agi: (base.agi || 1) + (mods.bonusAGI || 0),
        vit: (base.vit || 1) + (mods.bonusVIT || 0),
        int: (base.int || 1) + (mods.bonusINT || 0),
        dex: (base.dex || 1) + (mods.bonusDEX || 0),
        luk: (base.luk || 1) + (mods.bonusLUK || 0),
        level: base.level || 1
    };
}
```

---

## 2. Status Effect System (Server)

### 2.1 Status Effect Data Structure

Each active status effect on an entity is stored in a `Map`:

```javascript
// server/src/index.js — Status effect storage
// Entity structure (both players and enemies):
// entity.activeStatusEffects = new Map();
// Key: statusType string, Value: StatusEffect object

/**
 * @typedef {Object} StatusEffect
 * @property {string} type       - 'stun'|'freeze'|'stone'|'sleep'|'poison'|'blind'|'silence'|'confusion'|'bleeding'|'curse'
 * @property {number} appliedAt  - Date.now() when applied
 * @property {number} duration   - Duration in ms
 * @property {number} expiresAt  - appliedAt + duration
 * @property {number} sourceId   - Character/enemy ID that inflicted it
 * @property {number} sourceLevel - Caster's base level (for resistance formula)
 * @property {Object} effects    - Stat modifiers while active
 */
```

### 2.2 Status Resistance Formula

All status effects follow a consistent resistance pattern:

```javascript
// server/src/ro_status_effects.js — NEW MODULE

/**
 * Calculate the chance to inflict a status effect (Pre-Renewal).
 *
 * Formula: FinalChance = BaseChance - (BaseChance * ResistStat / 100)
 *          + srcBaseLevel - tarBaseLevel - tarLUK
 *
 * @param {number} baseChance    - Skill's base infliction % (e.g., Bash Lv10 = 25%)
 * @param {number} resistStat    - Target's resistance stat value (VIT, INT, MDEF, etc.)
 * @param {number} srcLevel      - Attacker's base level
 * @param {number} tarLevel      - Target's base level
 * @param {number} tarLUK        - Target's LUK (universal status resistance)
 * @param {Object} [opts]        - { immunityThreshold, lukImmunity }
 * @returns {number} Final infliction chance (0-100), clamped
 */
function calculateStatusChance(baseChance, resistStat, srcLevel, tarLevel, tarLUK, opts = {}) {
    const { immunityThreshold = 97, lukImmunity = 300 } = opts;

    // Hard immunity checks
    if (resistStat >= immunityThreshold) return 0;
    if (tarLUK >= lukImmunity) return 0;

    const chance = baseChance
        - Math.floor(baseChance * resistStat / 100)
        + srcLevel - tarLevel
        - tarLUK;

    return Math.max(0, Math.min(100, chance));
}

/**
 * Calculate status effect duration after resistance.
 *
 * @param {number} baseDuration  - Base duration in ms
 * @param {number} resistStat    - Target's resistance stat
 * @param {number} tarLUK        - Target's LUK
 * @returns {number} Final duration in ms (minimum 1000ms)
 */
function calculateStatusDuration(baseDuration, resistStat, tarLUK) {
    const duration = baseDuration
        - Math.floor(baseDuration * resistStat / 100)
        - 10 * tarLUK;
    return Math.max(1000, duration);
}
```

### 2.3 All 10 Status Effects

```javascript
// server/src/ro_status_effects.js — STATUS_EFFECT_CONFIG

const STATUS_EFFECT_CONFIG = {
    stun: {
        // Cannot move, attack, use items, use skills. FLEE negated.
        baseDuration: 5000,
        resistStat: 'vit',
        immunityThreshold: 97,
        effects: {
            canMove: false,
            canAttack: false,
            canUseSkills: false,
            canUseItems: false,
            fleeOverride: 0
        },
        curedBy: ['status_recovery', 'dispell', 'battle_chant'],
        curedByDamage: false
    },

    freeze: {
        // Cannot move/attack/skill. DEF -50%, MDEF +25%. Armor becomes Water Lv1.
        baseDuration: 12000,
        resistStat: 'hardMDEF',  // Uses equipment MDEF, not INT
        immunityThreshold: 100,  // No hard VIT/INT threshold; MDEF-based
        effects: {
            canMove: false,
            canAttack: false,
            canUseSkills: false,
            canUseItems: false,
            defMultiplier: 0.5,    // DEF reduced 50%
            bonusMDEF: 25,         // MDEF +25%
            elementOverride: { type: 'water', level: 1 }  // Vulnerable to Wind
        },
        curedBy: ['status_recovery', 'provoke', 'battle_chant'],
        curedByDamage: true  // Any damage breaks freeze
    },

    stone: {
        // Two-phase: (1) Petrifying 3s: can move, can't attack/skill
        //            (2) Petrified: can't do anything, DEF -50%, MDEF +25%,
        //                armor becomes Earth Lv1, lose 1% HP/5s (min 25% HP)
        baseDuration: 20000,       // Fixed duration once fully petrified
        petrifyingDuration: 3000,  // Phase 1 duration
        resistStat: 'hardMDEF',
        immunityThreshold: 100,
        effects: {
            // Phase 2 (fully petrified) effects
            canMove: false,
            canAttack: false,
            canUseSkills: false,
            canUseItems: false,
            defMultiplier: 0.5,
            bonusMDEF: 25,
            elementOverride: { type: 'earth', level: 1 },  // Vulnerable to Fire
            hpDrainPercent: 1,     // 1% HP every 5s
            hpDrainInterval: 5000,
            hpDrainMinPercent: 25  // Cannot drain below 25% max HP
        },
        curedBy: ['status_recovery', 'blessing', 'battle_chant'],
        curedByDamage: true  // Breaks stone
    },

    sleep: {
        // Cannot move/attack/use items/skills. Always hit. 2x crit rate.
        baseDuration: 30000,
        resistStat: 'int',
        immunityThreshold: 97,
        effects: {
            canMove: false,
            canAttack: false,
            canUseSkills: false,
            canUseItems: false,
            incomingHitRateOverride: 100,  // Attacks always hit sleeping targets
            incomingCritMultiplier: 2       // Double crit rate vs sleeping targets
        },
        curedBy: ['battle_chant', 'dispell'],
        curedByDamage: true
    },

    poison: {
        // DEF -25%. HP drain: (HP * 1.5% + 2)/sec. Can't go below 25% HP.
        // SP regen disabled.
        baseDuration: 60000,  // 60s for players, 30s for monsters
        monsterDuration: 30000,
        resistStat: 'vit',
        immunityThreshold: 97,
        effects: {
            defMultiplier: 0.75,           // DEF -25%
            hpDrainPerSecond: true,        // Dynamic: (currentHP * 0.015 + 2)
            hpDrainMinPercent: 25,         // Cannot drain below 25% max HP
            spRegenDisabled: true
        },
        curedBy: ['green_herb', 'green_potion', 'panacea', 'royal_jelly',
                  'detoxify', 'battle_chant'],
        curedByDamage: false
    },

    blind: {
        // HIT -25%, FLEE -25%, screen darkens
        baseDuration: 30000,
        minimumDuration: 15000,
        resistStat: 'avg_int_vit',  // (INT + VIT) / 2
        immunityThreshold: 97,      // 193 combined (INT+VIT)
        effects: {
            hitMultiplier: 0.75,
            fleeMultiplier: 0.75,
            screenDarken: true
        },
        curedBy: ['green_potion', 'panacea', 'royal_jelly', 'cure', 'battle_chant'],
        curedByDamage: false
    },

    silence: {
        // Cannot use active skills. Can still move, auto-attack, use items.
        baseDuration: 30000,
        resistStat: 'vit',
        immunityThreshold: 97,
        effects: {
            canUseSkills: false
            // canMove: true, canAttack: true, canUseItems: true
        },
        curedBy: ['green_potion', 'panacea', 'royal_jelly', 'cure',
                  'lex_divina', 'battle_chant'],
        curedByDamage: false
    },

    confusion: {
        // Movement direction randomized
        baseDuration: 30000,
        resistStat: 'avg_str_int',  // (STR + INT) / 2
        immunityThreshold: 97,      // 193 combined
        effects: {
            movementRandomized: true
        },
        // NOTE: Confusion has INVERTED level difference in formula
        invertedLevelDiff: true,
        curedBy: ['cure', 'panacea', 'royal_jelly', 'battle_chant'],
        curedByDamage: true
    },

    bleeding: {
        // HP drain over time (CAN KILL). HP/SP regen disabled. Persists through relog.
        baseDuration: 120000,  // Longest base duration
        resistStat: 'vit',
        immunityThreshold: 97,
        effects: {
            hpDrainOverTime: true,      // Slower than poison but lethal
            hpRegenDisabled: true,
            spRegenDisabled: true,
            canKill: true               // Unlike poison, CAN reduce HP to 0
        },
        curedBy: ['battle_chant', 'dispell'],  // Very few cures
        curedByDamage: false
    },

    curse: {
        // ATK -25%, LUK = 0, movement speed drastically reduced
        baseDuration: 30000,
        resistStat: 'luk',        // LUK for chance (unusual)
        durationResistStat: 'vit', // VIT for duration
        immunityThreshold: 97,
        effects: {
            atkMultiplier: 0.75,
            lukOverride: 0,
            moveSpeedMultiplier: 0.3  // Drastic speed reduction
        },
        curedBy: ['blessing', 'holy_water', 'panacea', 'royal_jelly', 'battle_chant'],
        curedByDamage: false
    }
};
```

### 2.4 Status Effect Application Function

```javascript
/**
 * Attempt to apply a status effect to an entity.
 * Checks boss immunity, resistance, and existing effects.
 *
 * @param {Object} source   - Attacker entity
 * @param {Object} target   - Target entity (player or enemy)
 * @param {string} statusType - One of the 10 status types
 * @param {number} baseChance - Base infliction chance (0-100)
 * @param {Object} [overrides] - { duration, forceApply }
 * @returns {{ applied: boolean, reason?: string }}
 */
function applyStatusEffect(source, target, statusType, baseChance, overrides = {}) {
    const config = STATUS_EFFECT_CONFIG[statusType];
    if (!config) return { applied: false, reason: 'unknown_status' };

    // Boss immunity: bosses/MVPs are immune to all status effects
    if (target.modeFlags?.statusImmune) {
        return { applied: false, reason: 'boss_immune' };
    }

    // Already has this status? Don't stack (RO doesn't stack same status)
    if (!target.activeStatusEffects) target.activeStatusEffects = new Map();
    if (target.activeStatusEffects.has(statusType)) {
        return { applied: false, reason: 'already_active' };
    }

    // Calculate resistance stat value
    const stats = target.stats || target;
    let resistStatValue = 0;
    switch (config.resistStat) {
        case 'vit': resistStatValue = stats.vit || 0; break;
        case 'int': resistStatValue = stats.int || 0; break;
        case 'luk': resistStatValue = stats.luk || 0; break;
        case 'hardMDEF': resistStatValue = target.hardMdef || target.magicDefense || 0; break;
        case 'avg_int_vit': resistStatValue = Math.floor(((stats.int || 0) + (stats.vit || 0)) / 2); break;
        case 'avg_str_int': resistStatValue = Math.floor(((stats.str || 0) + (stats.int || 0)) / 2); break;
    }

    // Roll infliction chance
    if (!overrides.forceApply) {
        const srcLevel = (source.stats || source).level || 1;
        const tarLevel = (stats).level || 1;
        const tarLUK = stats.luk || 0;

        let chance;
        if (config.invertedLevelDiff) {
            // Confusion: inverted level difference
            chance = baseChance - Math.floor(baseChance * resistStatValue / 100)
                     - srcLevel + tarLevel + tarLUK;
        } else {
            chance = calculateStatusChance(baseChance, resistStatValue, srcLevel, tarLevel, tarLUK, {
                immunityThreshold: config.immunityThreshold
            });
        }

        if (Math.random() * 100 >= chance) {
            return { applied: false, reason: 'resisted' };
        }
    }

    // Calculate duration
    const tarLUK = stats.luk || 0;
    const durationResistStat = config.durationResistStat || config.resistStat;
    let dResistValue = resistStatValue;
    if (config.durationResistStat) {
        switch (config.durationResistStat) {
            case 'vit': dResistValue = stats.vit || 0; break;
        }
    }

    let duration = overrides.duration || config.baseDuration;
    // For non-fixed-duration statuses, apply resistance reduction
    if (statusType !== 'stone') {
        duration = calculateStatusDuration(duration, dResistValue, tarLUK);
    }
    if (config.minimumDuration) {
        duration = Math.max(config.minimumDuration, duration);
    }

    // Apply the status effect
    const now = Date.now();
    const effect = {
        type: statusType,
        appliedAt: now,
        duration,
        expiresAt: now + duration,
        sourceId: source.characterId || source.enemyId,
        sourceLevel: (source.stats || source).level || 1,
        effects: { ...config.effects }
    };

    target.activeStatusEffects.set(statusType, effect);

    return { applied: true, duration, effect };
}
```

### 2.5 Status Tick Loop (Server)

```javascript
// server/src/index.js — Status effect tick (runs every 1000ms)
setInterval(() => {
    const now = Date.now();

    // Process ALL entities with active status effects
    const allEntities = [...connectedPlayers.values(), ...enemies.values()];

    for (const entity of allEntities) {
        if (!entity.activeStatusEffects || entity.activeStatusEffects.size === 0) continue;

        for (const [statusType, effect] of entity.activeStatusEffects.entries()) {
            // Check expiry
            if (now >= effect.expiresAt) {
                entity.activeStatusEffects.delete(statusType);
                broadcastStatusRemoved(entity, statusType, 'expired');
                continue;
            }

            // Periodic damage effects
            if (statusType === 'poison') {
                // HP drain: (currentHP * 1.5% + 2) per second
                const drain = Math.floor(entity.currentHP * 0.015) + 2;
                const minHP = Math.floor(entity.maxHP * 0.25);
                entity.currentHP = Math.max(minHP, entity.currentHP - drain);
                broadcastHealthUpdate(entity);
            }

            if (statusType === 'bleeding') {
                // HP drain over time — CAN KILL
                const drain = Math.max(1, Math.floor(entity.maxHP * 0.005));
                entity.currentHP = Math.max(0, entity.currentHP - drain);
                broadcastHealthUpdate(entity);
                if (entity.currentHP <= 0) {
                    handleEntityDeath(entity, effect.sourceId, 'bleeding');
                }
            }

            if (statusType === 'stone' && effect.effects.hpDrainPercent) {
                // Stone curse HP drain: 1% every 5s (checked every 1s tick)
                if ((now - effect.appliedAt) % 5000 < 1000) {
                    const drain = Math.floor(entity.maxHP * 0.01);
                    const minHP = Math.floor(entity.maxHP * 0.25);
                    entity.currentHP = Math.max(minHP, entity.currentHP - drain);
                    broadcastHealthUpdate(entity);
                }
            }
        }
    }
}, 1000);  // Status tick every 1 second
```

### 2.6 Cure Mechanics

```javascript
/**
 * Remove a status effect from an entity.
 * Called by cure items, cure skills, or damage-on-break statuses.
 */
function removeStatusEffect(entity, statusType, reason = 'cured') {
    if (!entity.activeStatusEffects) return false;
    if (!entity.activeStatusEffects.has(statusType)) return false;

    entity.activeStatusEffects.delete(statusType);
    broadcastStatusRemoved(entity, statusType, reason);
    return true;
}

/**
 * Check and break damage-breakable statuses when entity takes damage.
 * Called from both physical and magical damage pipelines.
 */
function checkDamageBreakStatuses(target) {
    if (!target.activeStatusEffects) return;

    const breakable = ['freeze', 'stone', 'sleep', 'confusion'];
    for (const statusType of breakable) {
        if (target.activeStatusEffects.has(statusType)) {
            const config = STATUS_EFFECT_CONFIG[statusType];
            if (config.curedByDamage) {
                removeStatusEffect(target, statusType, 'damage_break');
            }
        }
    }
}
```

### 2.7 Boss Immunity Flags

```javascript
// Boss/MVP mode flags from ro_monster_ai_codes.js
// These are parsed from rAthena hex mode bitmasks

// Key immunity flags:
// modeFlags.statusImmune  → immune to ALL status effects
// modeFlags.knockbackImmune → immune to knockback (Jupitel Thunder, etc.)
// modeFlags.detector → can see hidden/cloaked players

// Applied automatically to boss/mvp class monsters:
if (monster.class === 'boss' || monster.class === 'mvp') {
    monster.modeFlags.statusImmune = true;
    monster.modeFlags.knockbackImmune = true;
    monster.modeFlags.detector = true;
}
```

### 2.8 Socket Events

```javascript
// Status applied — sent to all players in zone
function broadcastStatusApplied(entity, statusType, duration) {
    const entityId = entity.characterId || entity.enemyId;
    const isEnemy = !!entity.enemyId;
    const zone = entity.zone || 'prontera_south';

    broadcastToZone(zone, 'status:applied', {
        targetId: entityId,
        isEnemy,
        statusType,       // 'stun', 'freeze', etc.
        duration,          // Duration in ms
        timestamp: Date.now()
    });
}

// Status removed — sent to all players in zone
function broadcastStatusRemoved(entity, statusType, reason) {
    const entityId = entity.characterId || entity.enemyId;
    const isEnemy = !!entity.enemyId;
    const zone = entity.zone || 'prontera_south';

    broadcastToZone(zone, 'status:removed', {
        targetId: entityId,
        isEnemy,
        statusType,
        reason            // 'expired', 'cured', 'damage_break'
    });
}
```

---

## 3. Element System (Server)

### 3.1 Element Table (10x10x4)

The element table is already fully implemented in `server/src/ro_damage_formulas.js` as the `ELEMENT_TABLE` constant. It is a nested object:

```
ELEMENT_TABLE[attackElement][defendElement][defendLevel - 1]
```

**10 elements:** neutral, water, earth, fire, wind, poison, holy, shadow, ghost, undead

**4 element levels:** Level 1 through 4 (array index 0-3)

**Values:** Damage percentage where 100 = normal, 0 = immune, negative = heals target

The lookup function:

```javascript
function getElementModifier(atkElement, defElement, defElementLevel = 1) {
    const atkRow = ELEMENT_TABLE[atkElement] || ELEMENT_TABLE.neutral;
    const defCol = atkRow[defElement] || atkRow.neutral;
    const lvIdx = Math.max(0, Math.min(3, defElementLevel - 1));
    return defCol[lvIdx];
}
```

### 3.2 Element Assignment

```javascript
// ── Monster element ──
// Defined in ro_monster_templates.js per monster:
// { element: { type: 'fire', level: 2 } }

// ── Player armor element ──
// Default: neutral (level 1)
// Changed by armor cards:
// Swordfish Card → Water Lv1, Pasana Card → Fire Lv1, etc.
// Stored on player object:
// player.armorElement = { type: 'neutral', level: 1 }

// ── Weapon element ──
// Default: neutral
// Changed by Endow skills (Sage), weapon cards, forged weapons
// Stored on player object:
// player.weaponElement = 'neutral'

// ── Skill element ──
// Defined in ro_skill_data.js per skill:
// Fire Bolt → 'fire', Cold Bolt → 'water', Lightning Bolt → 'wind'
// Passed as options.skillElement to damage function
```

### 3.3 Endow Skills (Temporary Weapon Element)

```javascript
// Sage Endow skills apply temporary weapon element changes
// Duration: skill level dependent, typically 10min at max level

function applyEndow(player, element, duration) {
    player.weaponElement = element;
    player.endowExpiresAt = Date.now() + duration;

    // Broadcast buff for client VFX
    broadcastToZone(player.zone, 'skill:buff_applied', {
        targetId: player.characterId,
        buffType: `endow_${element}`,
        duration,
        isEnemy: false
    });
}

// Endow expiry checked in the status tick loop:
if (player.endowExpiresAt && Date.now() >= player.endowExpiresAt) {
    player.weaponElement = 'neutral';
    player.endowExpiresAt = null;
    broadcastToZone(player.zone, 'skill:buff_removed', {
        targetId: player.characterId,
        buffType: `endow_${player.weaponElement}`
    });
}
```

### 3.4 Element Checking in Damage Pipeline

Element is checked at **Step 12** of the physical pipeline and **Step 6** of the magical pipeline. In both cases:

```javascript
// Determine attack element: skill element > weapon element > neutral
const atkElement = options.skillElement || attacker.weaponElement || 'neutral';

// Determine target element: status override > armor element > monster element
let targetElement = target.element?.type || 'neutral';
let targetElementLevel = target.element?.level || 1;

// Status overrides: Freeze → Water Lv1, Stone → Earth Lv1
if (target.activeStatusEffects) {
    const freeze = target.activeStatusEffects.get('freeze');
    if (freeze) { targetElement = 'water'; targetElementLevel = 1; }
    const stone = target.activeStatusEffects.get('stone');
    if (stone && stone.phase === 'petrified') { targetElement = 'earth'; targetElementLevel = 1; }
}

const eleModifier = getElementModifier(atkElement, targetElement, targetElementLevel);
// eleModifier <= 0 → immune (return miss)
// eleModifier > 100 → super effective
// eleModifier < 100 → resistant
totalDamage = Math.floor(totalDamage * eleModifier / 100);
```

---

## 4. Client-Side: Combat Subsystem

### 4.1 Overview

The combat subsystem on the client is presentation-only. It receives socket events and translates them into visual feedback. The existing project already has working subsystems for this:

| Subsystem | File | Z-Order | Events Handled |
|-----------|------|---------|----------------|
| `UBasicInfoSubsystem` | `UI/BasicInfoSubsystem.h/.cpp` | 10 | `combat:damage`, `combat:death`, `combat:respawn`, `player:stats`, `combat:health_update` |
| `UDamageNumberSubsystem` | `UI/DamageNumberSubsystem.h/.cpp` | 20 | `combat:damage`, `skill:effect_damage` |
| `UWorldHealthBarSubsystem` | `UI/WorldHealthBarSubsystem.h/.cpp` | 8 | `combat:damage`, `skill:effect_damage`, `enemy:spawn`, `enemy:move`, `enemy:death` |
| `UCastBarSubsystem` | `UI/CastBarSubsystem.h/.cpp` | 25 | `skill:cast_start`, `skill:cast_complete`, `skill:cast_interrupted` |
| `USkillVFXSubsystem` | `VFX/SkillVFXSubsystem.h/.cpp` | N/A | `skill:effect_damage`, `skill:buff_applied`, `skill:buff_removed` |

### 4.2 CombatSubsystem (New — for centralized combat state)

This subsystem would serve as the central hub for combat state on the client. Currently, combat state is scattered. A dedicated `UCombatSubsystem` would own:
- Current attack target tracking
- Attack animation triggering
- Click-to-attack input forwarding

**Header:**

```cpp
// client/SabriMMO/Source/SabriMMO/UI/CombatSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CombatSubsystem.generated.h"

class USocketIOClientComponent;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
    Idle,
    Attacking,
    Casting
};

UCLASS()
class SABRIMMO_API UCombatSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public API ----

    /** Start auto-attacking an enemy by ID. Sends combat:attack to server. */
    void StartAttackEnemy(int32 EnemyId);

    /** Start auto-attacking a player by ID (PvP). */
    void StartAttackPlayer(int32 CharacterId);

    /** Stop auto-attacking. Sends combat:stop to server. */
    void StopAttack();

    /** Get the current combat state. */
    ECombatState GetCombatState() const { return CombatState; }

    /** Get the ID of the current attack target (0 if none). */
    int32 GetCurrentTargetId() const { return CurrentTargetId; }

    /** Is the current target an enemy? */
    bool IsTargetEnemy() const { return bTargetIsEnemy; }

private:
    // ---- Socket event wrapping ----
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event handlers ----
    void HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data);
    void HandleAutoAttackStopped(const TSharedPtr<FJsonValue>& Data);
    void HandleTargetLost(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
    void HandleOutOfRange(const TSharedPtr<FJsonValue>& Data);

    // ---- State ----
    ECombatState CombatState = ECombatState::Idle;
    int32 CurrentTargetId = 0;
    bool bTargetIsEnemy = false;
    int32 LocalCharacterId = 0;

    // ---- Attack timing (for client-side animation pacing) ----
    float AttackIntervalSeconds = 1.0f;  // From server ASPD
    float AttackRange = 200.0f;          // From server

    bool bEventsWrapped = false;
    FTimerHandle BindCheckTimer;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

**Implementation (key methods):**

```cpp
// client/SabriMMO/Source/SabriMMO/UI/CombatSubsystem.cpp

#include "CombatSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombat, Log, All);

bool UCombatSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UCombatSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
    {
        LocalCharacterId = GI->GetSelectedCharacter().CharacterId;
    }

    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UCombatSubsystem::TryWrapSocketEvents),
        0.5f, true
    );
}

void UCombatSubsystem::StartAttackEnemy(int32 EnemyId)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetNumberField(TEXT("targetEnemyId"), EnemyId);

    CachedSIOComponent->EmitNative(TEXT("combat:attack"),
        MakeShared<FJsonValueObject>(Payload));

    UE_LOG(LogCombat, Log, TEXT("Sent combat:attack for enemyId=%d"), EnemyId);
}

void UCombatSubsystem::StopAttack()
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
    CachedSIOComponent->EmitNative(TEXT("combat:stop"),
        MakeShared<FJsonValueObject>(Payload));

    CombatState = ECombatState::Idle;
    CurrentTargetId = 0;
    bTargetIsEnemy = false;

    UE_LOG(LogCombat, Log, TEXT("Sent combat:stop"));
}

void UCombatSubsystem::HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr)) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double TargetId = 0;
    Obj->TryGetNumberField(TEXT("targetId"), TargetId);
    CurrentTargetId = (int32)TargetId;

    Obj->TryGetBoolField(TEXT("isEnemy"), bTargetIsEnemy);

    double IntervalMs = 1000;
    Obj->TryGetNumberField(TEXT("attackIntervalMs"), IntervalMs);
    AttackIntervalSeconds = (float)IntervalMs / 1000.0f;

    double Range = 200;
    Obj->TryGetNumberField(TEXT("attackRange"), Range);
    AttackRange = (float)Range;

    CombatState = ECombatState::Attacking;

    UE_LOG(LogCombat, Log,
        TEXT("Auto-attack started: target=%d isEnemy=%d interval=%.2fs range=%.0f"),
        CurrentTargetId, bTargetIsEnemy ? 1 : 0, AttackIntervalSeconds, AttackRange);
}

void UCombatSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
    // This handler is for triggering attack animations on the local player's pawn.
    // Damage numbers and HP bar updates are handled by their respective subsystems.
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr)) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double AttackerId = 0;
    Obj->TryGetNumberField(TEXT("attackerId"), AttackerId);

    // Only trigger attack animation if WE are the attacker
    if ((int32)AttackerId == LocalCharacterId)
    {
        // The Blueprint-bound handler in BP_MMOCharacter handles
        // the actual montage triggering via the combat:damage event.
        // This subsystem just tracks state.
    }
}

void UCombatSubsystem::HandleTargetLost(const TSharedPtr<FJsonValue>& Data)
{
    CombatState = ECombatState::Idle;
    CurrentTargetId = 0;
    bTargetIsEnemy = false;

    UE_LOG(LogCombat, Log, TEXT("Target lost — returning to idle."));
}

void UCombatSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}
```

### 4.3 Click-to-Attack Input Handling

In the existing project, click-to-attack is handled in Blueprint (`BP_MMOCharacter`). The C++ `ASabriMMOCharacter` provides the input binding framework. Here is how the click-to-attack input flows:

```
1. Player left-clicks on an enemy actor in the viewport
2. BP_MMOCharacter's Enhanced Input action IA_LeftClick fires
3. Blueprint performs a line trace from camera through cursor
4. If hit actor implements BPI_Damageable → get enemy ID from actor tag/component
5. Call UCombatSubsystem::StartAttackEnemy(EnemyId)
   OR emit combat:attack directly via SocketIO Blueprint node
6. Server validates and starts auto-attack loop
7. Server sends combat:auto_attack_started back
8. Client receives confirmation, begins visual feedback (target highlight, etc.)
```

For a pure C++ implementation path:

```cpp
// In ASabriMMOCharacter::SetupPlayerInputComponent or a dedicated input handler:

void ASabriMMOCharacter::OnLeftClickAction()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    // Perform line trace under cursor
    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (!Hit.bBlockingHit || !Hit.GetActor()) return;

    // Check if hit actor is an enemy (via interface or tag)
    AActor* HitActor = Hit.GetActor();

    // Try to get enemy ID from a component or actor tag
    // This depends on how BP_EnemyManager tags spawned enemy actors
    FString EnemyIdTag = HitActor->Tags.Num() > 0
        ? HitActor->Tags[0].ToString() : TEXT("");

    if (EnemyIdTag.StartsWith(TEXT("Enemy_")))
    {
        int32 EnemyId = FCString::Atoi(*EnemyIdTag.RightChop(6));
        if (UWorld* World = GetWorld())
        {
            if (UCombatSubsystem* Combat = World->GetSubsystem<UCombatSubsystem>())
            {
                Combat->StartAttackEnemy(EnemyId);
            }
        }
    }
}
```

### 4.4 Attack Range Validation (Client-Side Prediction)

The client does not authoritatively validate range -- the server does. However, the client can do a soft prediction to provide immediate feedback:

```cpp
// In CombatSubsystem — client-side range check (non-authoritative)
bool UCombatSubsystem::IsTargetInRange(const FVector& TargetPos) const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC || !PC->GetPawn()) return false;

    const float Distance = FVector::Dist2D(
        PC->GetPawn()->GetActorLocation(), TargetPos);

    // AttackRange comes from server (combat:auto_attack_started payload)
    return Distance <= AttackRange;
}
```

If the client detects out-of-range, it can begin moving the pawn toward the target. The server will independently emit `combat:out_of_range` with the target position for the client to path toward.

---

## 5. Client-Side: Damage Numbers

### 5.1 Architecture (Already Implemented)

The damage number system is fully implemented across two files:

- `UI/DamageNumberSubsystem.h/.cpp` -- UWorldSubsystem that wraps socket events and feeds the overlay
- `UI/SDamageNumberOverlay.h/.cpp` -- Slate widget that renders all active damage pop-ups via `OnPaint`

### 5.2 `SDamageNumberOverlay` Design

The overlay is a fullscreen transparent `SCompoundWidget` at Z-order 20. All rendering is done in `OnPaint` for maximum performance -- no child widgets are created per damage number.

**Key data structure:**

```cpp
// Circular buffer of damage pop entries (pool pattern)
struct FDamagePopEntry
{
    bool bActive = false;
    int32 Value = 0;
    EDamagePopType Type = EDamagePopType::NormalDamage;
    FVector2D ScreenAnchor = FVector2D::ZeroVector;
    double SpawnTime = 0.0;
    float RandomXBias = 0.0f;
    FString Element;  // For elemental tinting
};

static constexpr int32 MAX_ENTRIES = 64;  // Pool size
FDamagePopEntry Entries[MAX_ENTRIES];
int32 NextEntryIndex = 0;  // Circular write pointer
```

### 5.3 World-to-Screen Projection

```cpp
bool UDamageNumberSubsystem::ProjectWorldToScreen(
    const FVector& WorldPos, FVector2D& OutScreenPos) const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC) return false;

    // ProjectWorldLocationToScreen returns screen-pixel coordinates
    // bPlayerViewportRelative = false for absolute screen coords
    return PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, false);
}
```

### 5.4 Color Coding

```cpp
enum class EDamagePopType : uint8
{
    NormalDamage,    // Yellow/gold — auto-attack damage on enemies
    CriticalDamage,  // White, larger font — critical hit
    PlayerHit,       // Red — local player receiving damage
    PlayerCritHit,   // Bright red, larger — local player crit received
    SkillDamage,     // Orange — skill damage on enemies
    Miss,            // Light blue — "Miss" text
    Heal,            // Green — healing received
    Dodge,           // Green — FLEE dodge
    PerfectDodge     // Gold — Lucky Dodge (LUK-based)
};
```

**RO-faithful color palette:**

| Type | Fill Color | Notes |
|------|-----------|-------|
| NormalDamage | `(1.0, 0.92, 0.23)` Yellow/Gold | Auto-attack hits |
| CriticalDamage | `(1.0, 1.0, 0.95)` Warm White | Larger font (28pt) |
| PlayerHit | `(1.0, 0.2, 0.2)` Red | Damage received by local player |
| PlayerCritHit | `(1.0, 0.35, 0.55)` Magenta-Red | |
| SkillDamage | `(1.0, 0.65, 0.15)` Orange | Skill hits |
| Miss | `(0.5, 0.7, 1.0)` Light Blue | Renders "Miss" text |
| Heal | `(0.3, 1.0, 0.4)` Bright Green | |
| Dodge | `(0.4, 0.9, 0.5)` Green | Renders "Dodge" text |
| PerfectDodge | `(0.95, 0.85, 0.2)` Gold | Renders "Lucky Dodge" text |

**Element tinting:** Non-neutral elemental attacks blend the base color 60/40 toward an element color (water=blue, fire=orange-red, wind=green, etc.).

### 5.5 Float-Up Animation with Fade

All animation is driven by a per-frame `ActiveTimer` that calls `Invalidate(EInvalidateWidgetReason::Paint)`.

```cpp
// Animation constants
static constexpr float LIFETIME = 1.3f;           // Total pop-up duration
static constexpr float RISE_DISTANCE = 85.0f;     // Pixels to float upward
static constexpr float SPREAD_PER_DIGIT = 4.5f;   // Fan-out spread per digit
static constexpr float FADE_START = 0.65f;         // Start fading at 65%

// Per-frame calculations in OnPaint:
const float LifeAlpha = Elapsed / LIFETIME;  // 0.0 → 1.0

// Rise: ease-out (fast start, decelerates)
const float RiseY = -RISE_DISTANCE * FMath::InterpEaseOut(0, 1, LifeAlpha, 3.0f);

// Digit spread: ease-out, reaches max at ~50% lifetime
const float SpreadT = FMath::Clamp(LifeAlpha * 2.0f, 0.0f, 1.0f);
const float SpreadFactor = FMath::InterpEaseOut(0, 1, SpreadT, 2.0f);

// Alpha fade: full until 65%, then linear fade to 0
float Alpha = 1.0f;
if (LifeAlpha > FADE_START) {
    Alpha = 1.0f - (LifeAlpha - FADE_START) / (1.0f - FADE_START);
}
```

### 5.6 Widget Pooling

The overlay uses a fixed-size circular buffer (pool) of 64 entries. No heap allocation occurs when spawning damage numbers:

```cpp
void SDamageNumberOverlay::AddDamagePop(int32 Value, EDamagePopType Type,
    FVector2D ScreenPosition, const FString& Element)
{
    // Stacking detection: offset upward if multiple numbers near same position
    int32 StackCount = 0;
    for (int32 i = 0; i < MAX_ENTRIES; ++i) {
        // Count recent entries within STACK_CHECK_RADIUS pixels
        // within STACK_CHECK_TIME window
    }

    FDamagePopEntry& Entry = Entries[NextEntryIndex];
    Entry.bActive = true;
    Entry.Value = Value;
    Entry.Type = Type;
    Entry.ScreenAnchor = AdjustedPos;
    Entry.SpawnTime = FPlatformTime::Seconds();
    Entry.RandomXBias = FMath::FRandRange(-RANDOM_X_RANGE, RANDOM_X_RANGE);
    Entry.Element = Element;

    NextEntryIndex = (NextEntryIndex + 1) % MAX_ENTRIES;
}
```

### 5.7 Multiple Simultaneous Damage Numbers

The stacking system prevents overlapping numbers:

```cpp
static constexpr float STACK_CHECK_RADIUS = 60.0f;   // Pixel proximity check
static constexpr float STACK_CHECK_TIME = 0.8f;       // Time window
static constexpr float STACK_OFFSET_Y = -26.0f;       // Offset upward per stack

// Each new number at a similar position is shifted up by STACK_OFFSET_Y
AdjustedPos.Y += StackCount * STACK_OFFSET_Y;
```

---

## 6. Client-Side: Attack Animations

### 6.1 Animation Montage System

Attack animations are triggered by the `combat:damage` event in Blueprint. The existing project handles this in `BP_MMOCharacter` via the Socket.io event callback. Here is the C++ equivalent for reference:

```cpp
// Attack animation triggering from combat:damage event
// This runs when the LOCAL player is the attacker

void ABP_MMOCharacter::OnCombatDamageReceived(int32 AttackerId, int32 TargetId,
    int32 Damage, bool bIsCritical, const FString& HitType)
{
    // Only play attack animation if WE are the attacker
    if (AttackerId != LocalCharacterId) return;

    USkeletalMeshComponent* Mesh = GetMesh();
    if (!Mesh) return;

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance) return;

    // Select montage based on weapon type
    UAnimMontage* AttackMontage = GetAttackMontageForWeapon(CurrentWeaponType);
    if (!AttackMontage) return;

    // Play rate scaled by ASPD
    // Higher ASPD = faster animation
    // At ASPD 150 (1 hit/sec): play at 1.0x
    // At ASPD 190 (5 hits/sec): play at ~2.5x (capped for visual clarity)
    float PlayRate = FMath::Clamp(
        AttackIntervalSeconds > 0 ? 1.0f / AttackIntervalSeconds : 1.0f,
        0.5f, 2.5f
    );

    AnimInstance->Montage_Play(AttackMontage, PlayRate);
}
```

### 6.2 ASPD-Based Playback Rate Scaling

```cpp
// ASPD → Animation speed mapping
// The attack interval from the server determines how fast animations play

// Server sends in combat:auto_attack_started:
//   attackIntervalMs: (200 - ASPD) * 20
//
// Examples:
//   ASPD 150 → 1000ms → PlayRate 1.0
//   ASPD 170 →  600ms → PlayRate 1.67
//   ASPD 185 →  300ms → PlayRate 2.0 (capped at 2.5)
//   ASPD 190 →  200ms → PlayRate 2.5 (cap)

float CalculateAnimPlayRate(float AttackIntervalMs)
{
    // Base animation is designed for 1-second attack speed
    const float BaseAnimDuration = 1.0f; // seconds
    const float TargetDuration = AttackIntervalMs / 1000.0f;

    float PlayRate = BaseAnimDuration / FMath::Max(0.1f, TargetDuration);
    return FMath::Clamp(PlayRate, 0.5f, 2.5f);
}
```

### 6.3 Hit Stun Animation (Damage Received)

When the local player or a visible character takes damage, a brief hit stun animation plays:

```cpp
// Triggered by combat:damage or skill:effect_damage when
// the LOCAL player is the TARGET

void ABP_MMOCharacter::OnDamageReceived(int32 Damage, bool bIsCritical)
{
    if (Damage <= 0) return;  // Miss — no stun

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    // Play hit reaction montage
    // Duration: DamageMotion from monster template (typically 300-500ms)
    UAnimMontage* HitMontage = HitReactionMontage;
    if (HitMontage)
    {
        AnimInstance->Montage_Play(HitMontage, 1.0f);
    }

    // Screen shake for heavy hits (crits or >20% of max HP)
    if (bIsCritical || Damage > MaxHP * 0.2f)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            PC->ClientStartCameraShake(HeavyHitCameraShake, 0.5f);
        }
    }
}
```

### 6.4 Death Animation

```cpp
void ABP_MMOCharacter::OnCharacterDeath()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    // Stop any current montage
    AnimInstance->Montage_Stop(0.2f);

    // Play death montage
    if (DeathMontage)
    {
        AnimInstance->Montage_Play(DeathMontage, 1.0f);
    }

    // Disable input
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
    }

    // Ragdoll after death montage completes (optional)
    // GetMesh()->SetSimulatePhysics(true);
}
```

### 6.5 Miss Effect

Miss does not trigger an attack animation on the target. The "Miss" text is displayed as a damage number pop-up. Additionally, a subtle whoosh sound can play:

```cpp
// In DamageNumberSubsystem — miss spawns a text pop-up
if (HitType == TEXT("miss"))
{
    PopType = EDamagePopType::Miss;  // Renders "Miss" text in light blue
}
else if (HitType == TEXT("dodge"))
{
    PopType = EDamagePopType::Dodge;  // Renders "Dodge" in green
}
else if (HitType == TEXT("perfectDodge"))
{
    PopType = EDamagePopType::PerfectDodge;  // Renders "Lucky Dodge" in gold
}
```

---

## 7. Client-Side: Status Effect Display

### 7.1 StatusEffectSubsystem (New)

A new `UStatusEffectSubsystem` manages visual feedback for all status effects:

```cpp
// client/SabriMMO/Source/SabriMMO/UI/StatusEffectSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "StatusEffectSubsystem.generated.h"

class USocketIOClientComponent;
class SStatusEffectOverlay;

UENUM(BlueprintType)
enum class EStatusEffectType : uint8
{
    Stun,
    Freeze,
    Stone,
    Sleep,
    Poison,
    Blind,
    Silence,
    Confusion,
    Bleeding,
    Curse,
    Count UMETA(Hidden)
};

// Active status effect on a tracked entity
struct FActiveStatusDisplay
{
    EStatusEffectType Type;
    double StartTime = 0.0;
    float Duration = 0.0f;  // seconds
    int32 TargetId = 0;
    bool bIsEnemy = false;
};

UCLASS()
class SABRIMMO_API UStatusEffectSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public query ----

    /** Get all active statuses on the local player. */
    TArray<FActiveStatusDisplay> GetLocalPlayerStatuses() const;

    /** Get all active statuses on a specific entity. */
    TArray<FActiveStatusDisplay> GetEntityStatuses(int32 EntityId, bool bIsEnemy) const;

    /** Check if local player has a specific status. */
    bool LocalPlayerHasStatus(EStatusEffectType Type) const;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // Event handlers
    void HandleStatusApplied(const TSharedPtr<FJsonValue>& Data);
    void HandleStatusRemoved(const TSharedPtr<FJsonValue>& Data);

    // Overlay management
    void ShowOverlay();
    void HideOverlay();

    // ---- State ----
    // Key: "entityId_isEnemy" → array of active statuses
    TMap<FString, TArray<FActiveStatusDisplay>> ActiveStatuses;

    int32 LocalCharacterId = 0;
    bool bEventsWrapped = false;
    bool bOverlayAdded = false;

    FTimerHandle BindCheckTimer;
    TSharedPtr<SStatusEffectOverlay> OverlayWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 7.2 Event Handlers

```cpp
// StatusEffectSubsystem.cpp — key event handlers

void UStatusEffectSubsystem::HandleStatusApplied(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr)) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double TargetId = 0;
    Obj->TryGetNumberField(TEXT("targetId"), TargetId);

    bool bIsEnemy = false;
    Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

    FString StatusTypeStr;
    Obj->TryGetStringField(TEXT("statusType"), StatusTypeStr);

    double DurationMs = 0;
    Obj->TryGetNumberField(TEXT("duration"), DurationMs);

    // Parse status type string to enum
    EStatusEffectType StatusType = ParseStatusType(StatusTypeStr);

    FActiveStatusDisplay Status;
    Status.Type = StatusType;
    Status.StartTime = FPlatformTime::Seconds();
    Status.Duration = (float)DurationMs / 1000.0f;
    Status.TargetId = (int32)TargetId;
    Status.bIsEnemy = bIsEnemy;

    // Store in map
    FString Key = FString::Printf(TEXT("%d_%d"), (int32)TargetId, bIsEnemy ? 1 : 0);
    ActiveStatuses.FindOrAdd(Key).Add(Status);

    // If this affects the local player, trigger screen effects
    if ((int32)TargetId == LocalCharacterId && !bIsEnemy)
    {
        ApplyLocalPlayerVisualEffect(StatusType, true);
    }
}

void UStatusEffectSubsystem::HandleStatusRemoved(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr)) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double TargetId = 0;
    Obj->TryGetNumberField(TEXT("targetId"), TargetId);

    bool bIsEnemy = false;
    Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

    FString StatusTypeStr;
    Obj->TryGetStringField(TEXT("statusType"), StatusTypeStr);

    EStatusEffectType StatusType = ParseStatusType(StatusTypeStr);

    FString Key = FString::Printf(TEXT("%d_%d"), (int32)TargetId, bIsEnemy ? 1 : 0);
    if (TArray<FActiveStatusDisplay>* Arr = ActiveStatuses.Find(Key))
    {
        Arr->RemoveAll([StatusType](const FActiveStatusDisplay& S) {
            return S.Type == StatusType;
        });
    }

    if ((int32)TargetId == LocalCharacterId && !bIsEnemy)
    {
        ApplyLocalPlayerVisualEffect(StatusType, false);
    }
}
```

### 7.3 Status Icons Widget

Status icons are rendered near the player's character frame (BasicInfo panel area):

```cpp
// SStatusEffectOverlay — renders status icons in a horizontal row
// near the top of the screen (below the BasicInfo widget)

// Icon layout:
// [Stun] [Freeze] [Poison] [Blind] ...
//  3.2s    8.1s     45s     22s      ← remaining time

// Each icon is 24x24 pixels with a dark background
// Duration text below in small font
// Icons pulse/glow when about to expire (<3s remaining)
```

### 7.4 Visual Effects on Character

For status effects that have visible world effects, the subsystem can spawn Cascade/Niagara particles on the affected actor:

```cpp
void UStatusEffectSubsystem::ApplyLocalPlayerVisualEffect(
    EStatusEffectType StatusType, bool bApply)
{
    // Get the local player pawn
    UWorld* World = GetWorld();
    if (!World) return;

    APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
    if (!Pawn) return;

    if (bApply)
    {
        switch (StatusType)
        {
        case EStatusEffectType::Freeze:
            // Spawn ice overlay particle effect attached to pawn
            // P_Ice_Proj_charge_01 Cascade at scale 2.0
            SpawnStatusVFX(Pawn, TEXT("/Game/VFX/Cascade/P_Ice_Proj_charge_01"),
                FVector::ZeroVector, FVector(2.0f));
            break;

        case EStatusEffectType::Poison:
            // Green poison bubbles rising from character
            SpawnStatusVFX(Pawn, TEXT("/Game/VFX/Niagara/NS_Poison_Bubbles"),
                FVector(0, 0, 50), FVector(1.0f));
            break;

        case EStatusEffectType::Stun:
            // Spinning stars above head
            SpawnStatusVFX(Pawn, TEXT("/Game/VFX/Niagara/NS_Stun_Stars"),
                FVector(0, 0, 120), FVector(1.0f));
            break;

        case EStatusEffectType::Blind:
            // Darken the screen via post-process material
            ApplyScreenDarkenEffect(true);
            break;

        case EStatusEffectType::Confusion:
            // Swirling effect around character
            SpawnStatusVFX(Pawn, TEXT("/Game/VFX/Niagara/NS_Confusion_Swirl"),
                FVector(0, 0, 60), FVector(1.0f));
            break;

        default:
            break;
        }
    }
    else
    {
        // Remove the VFX for this status type
        RemoveStatusVFX(StatusType);
    }
}
```

---

## 8. Auto-Attack Flow (End-to-End)

### 8.1 Complete Sequence Diagram

```
PLAYER                          CLIENT (UE5)                        SERVER (Node.js)
  |                                 |                                     |
  |  Left-click on enemy            |                                     |
  |  ─────────────────────>         |                                     |
  |                                 |  Line trace → hit enemy actor       |
  |                                 |  Extract enemyId from actor tag     |
  |                                 |                                     |
  |                                 |  combat:attack { targetEnemyId }    |
  |                                 |  ──────────────────────────────>    |
  |                                 |                                     |
  |                                 |                 Validate:           |
  |                                 |                 - Attacker alive?   |
  |                                 |                 - Enemy exists?     |
  |                                 |                 - Enemy alive?      |
  |                                 |                                     |
  |                                 |                 Store in            |
  |                                 |                 autoAttackState     |
  |                                 |                 Map                 |
  |                                 |                                     |
  |                                 |  combat:auto_attack_started         |
  |                                 |  { targetId, attackRange,           |
  |                                 |    aspd, attackIntervalMs }         |
  |                                 |  <──────────────────────────────    |
  |                                 |                                     |
  |  Target highlight shown         |  CombatState → Attacking           |
  |  <─────────────────────         |                                     |
  |                                 |                                     |
  |                                 |           ┌── Combat Tick (50ms) ──┐|
  |                                 |           │ Check ASPD timer       │|
  |                                 |           │ Check range            │|
  |                                 |           │ Roll PD → Crit → FLEE  │|
  |                                 |           │ Calculate 16-step dmg  │|
  |                                 |           │ Apply damage to enemy  │|
  |                                 |           └────────────────────────┘|
  |                                 |                                     |
  |                                 |  combat:damage                      |
  |                                 |  { attackerId, targetId, damage,    |
  |                                 |    isCritical, hitType, element,    |
  |                                 |    targetHealth, targetMaxHealth,   |
  |                                 |    targetX, targetY, targetZ }      |
  |                                 |  <──────────────────────────────    |
  |                                 |                                     |
  |                                 |  ┌─ DamageNumberSubsystem ─┐       |
  |                                 |  │ Project targetXYZ to     │       |
  |  Floating "1234"                |  │ screen, spawn pop-up     │       |
  |  ← shown above enemy           |  └──────────────────────────┘       |
  |                                 |                                     |
  |                                 |  ┌─ WorldHealthBarSubsystem ┐      |
  |  Enemy HP bar updates           |  │ Update EnemyHealthMap     │      |
  |  <─────────────────────         |  └───────────────────────────┘      |
  |                                 |                                     |
  |                                 |  ┌─ BasicInfoSubsystem ─────┐      |
  |                                 |  │ (Only if player is target)│      |
  |  Player HP bar updates          |  │ Update CurrentHP/MaxHP    │      |
  |  <─────────────────────         |  └───────────────────────────┘      |
  |                                 |                                     |
  |                                 |  ┌─ BP_MMOCharacter ────────┐      |
  |  Attack animation plays         |  │ Montage_Play(AttackAnim, │      |
  |  <─────────────────────         |  │ ASPD-scaled PlayRate)    │      |
  |                                 |  └───────────────────────────┘      |
  |                                 |                                     |
  |                                 |           ... (repeat each tick     |
  |                                 |            until target dies) ...   |
  |                                 |                                     |
  |                                 |           Enemy HP reaches 0        |
  |                                 |           ───────────────────       |
  |                                 |                                     |
  |                                 |  combat:damage (final hit)          |
  |                                 |  <──────────────────────────────    |
  |                                 |                                     |
  |                                 |  combat:death                       |
  |                                 |  { killedId, killedName,            |
  |                                 |    killerId, killerName,            |
  |                                 |    isEnemy, targetHealth: 0 }       |
  |                                 |  <──────────────────────────────    |
  |                                 |                                     |
  |  Enemy death animation          |  CombatState → Idle                |
  |  Enemy actor removed            |  CurrentTargetId → 0               |
  |  <─────────────────────         |                                     |
  |                                 |                                     |
  |                                 |  exp:gained { baseExp, jobExp }     |
  |  "+125 Base EXP" shown          |  <──────────────────────────────    |
  |  <─────────────────────         |                                     |
  |                                 |                                     |
  |                                 |  inventory:data (loot drops)        |
  |  Loot appears in inventory      |  <──────────────────────────────    |
  |  <─────────────────────         |                                     |
```

### 8.2 Server-Side Kill Processing

When an enemy reaches 0 HP in the combat tick:

```javascript
// server/src/index.js — Inside the combat tick, after damage applied
if (enemy.health <= 0) {
    enemy.isDead = true;
    enemy.health = 0;

    // ── Broadcast death to zone ──
    const deathPayload = {
        killedId: enemy.enemyId,
        killedName: enemy.name,
        killerId: attackerId,
        killerName: attacker.characterName,
        isEnemy: true,
        targetHealth: 0,
        targetMaxHealth: enemy.maxHealth
    };
    broadcastToZone(zone, 'combat:death', deathPayload);

    // ── Award EXP ──
    // EXP divided among all players who damaged the enemy
    const expShare = calculateExpShare(enemy, attacker);
    for (const [playerId, share] of expShare.entries()) {
        const player = connectedPlayers.get(playerId);
        if (!player) continue;

        player.baseExp += share.baseExp;
        player.jobExp += share.jobExp;

        const playerSocket = io.sockets.sockets.get(player.socketId);
        if (playerSocket) {
            playerSocket.emit('exp:gained', {
                baseExp: share.baseExp,
                jobExp: share.jobExp,
                totalBaseExp: player.baseExp,
                totalJobExp: player.jobExp,
                sourceId: enemy.enemyId,
                sourceName: enemy.name
            });
        }

        // Check level up
        checkLevelUp(player, playerId);
    }

    // ── Generate loot drops ──
    const drops = generateLootDrops(enemy, attackerId);
    if (drops.length > 0) {
        const playerSocket = io.sockets.sockets.get(attacker.socketId);
        if (playerSocket) {
            // Add items to inventory
            for (const drop of drops) {
                await addItemToInventory(attackerId, drop.itemId, drop.amount);
            }
            // Send updated inventory
            const inventory = await getPlayerInventory(attackerId);
            playerSocket.emit('inventory:data', { items: inventory });
        }
    }

    // ── Clean up combat state ──
    // Remove all auto-attack entries targeting this enemy
    for (const [otherId, otherAtk] of autoAttackState.entries()) {
        if (otherAtk.targetCharId === enemy.enemyId && otherAtk.isEnemy) {
            autoAttackState.delete(otherId);
            const otherPlayer = connectedPlayers.get(otherId);
            if (otherPlayer) {
                const otherSocket = io.sockets.sockets.get(otherPlayer.socketId);
                if (otherSocket) {
                    otherSocket.emit('combat:target_lost', {
                        reason: 'Enemy died',
                        isEnemy: true
                    });
                }
            }
        }
    }

    // ── Schedule respawn ──
    scheduleEnemyRespawn(enemy);
}
```

### 8.3 Client-Side Event Processing Order

When the client receives `combat:damage`, multiple subsystems process it in their wrap order:

```
combat:damage received
    │
    ├── 1. Blueprint handler (BP_SocketManager)
    │       └── Updates BP variables, triggers attack animation montage
    │
    ├── 2. BasicInfoSubsystem
    │       └── Updates local player HP/SP if target is local player
    │
    ├── 3. WorldHealthBarSubsystem
    │       └── Updates enemy HP bar data, triggers bar visibility
    │
    └── 4. DamageNumberSubsystem
            └── Projects world position, spawns damage pop-up
```

Each subsystem independently wraps the same event. The `WrapSingleEvent` pattern preserves the callback chain so all handlers fire in sequence. This architecture is critical -- do not replace it with a single handler.

### 8.4 Socket Event Reference (Complete)

| Event | Direction | Trigger | Key Payload Fields |
|-------|-----------|---------|-------------------|
| `combat:attack` | Client -> Server | Player clicks enemy | `targetEnemyId` or `targetCharacterId` |
| `combat:stop` | Client -> Server | Player clicks ground / presses Escape | `{}` |
| `combat:auto_attack_started` | Server -> Client | Attack validated | `targetId, isEnemy, attackRange, aspd, attackIntervalMs` |
| `combat:auto_attack_stopped` | Server -> Client | Attack ended | `reason` |
| `combat:out_of_range` | Server -> Client | Target too far | `targetId, targetX/Y/Z, distance, requiredRange` |
| `combat:target_lost` | Server -> Client | Target died/left | `reason, isEnemy` |
| `combat:damage` | Server -> Zone | Auto-attack hit/miss | `attackerId, targetId, damage, isCritical, isMiss, hitType, element, targetHealth, targetMaxHealth, targetX/Y/Z` |
| `combat:death` | Server -> Zone | Entity killed | `killedId, killedName, killerId, killerName, isEnemy` |
| `combat:error` | Server -> Client | Invalid action | `message` |
| `skill:effect_damage` | Server -> Zone | Skill damage per-hit | `attackerId, targetId, skillId, damage, hitType, element, hitNumber, totalHits, targetX/Y/Z` |
| `skill:cast_start` | Server -> Zone | Cast begins | `characterId, skillId, castTime, targetId` |
| `skill:cast_complete` | Server -> Zone | Cast finished | `characterId, skillId` |
| `skill:cast_interrupted` | Server -> Zone | Cast broken | `characterId, reason` |
| `skill:buff_applied` | Server -> Zone | Buff/debuff applied | `targetId, buffType, duration` |
| `skill:buff_removed` | Server -> Zone | Buff/debuff removed | `targetId, buffType` |
| `status:applied` | Server -> Zone | Status effect applied | `targetId, isEnemy, statusType, duration` |
| `status:removed` | Server -> Zone | Status effect removed | `targetId, isEnemy, statusType, reason` |
| `enemy:attack` | Server -> Zone | Monster attacks player | `enemyId, targetId, attackMotion` |
| `exp:gained` | Server -> Client | EXP awarded | `baseExp, jobExp, sourceId, sourceName` |

---

## Appendix A: File Inventory

### Server Files

| File | Status | Purpose |
|------|--------|---------|
| `server/src/ro_damage_formulas.js` | Implemented | Element table, size penalty, physical/magical damage pipelines |
| `server/src/index.js` | Implemented | Combat tick loop, auto-attack state, socket handlers |
| `server/src/ro_monster_templates.js` | Implemented | 509 monster stat templates |
| `server/src/ro_skill_data.js` | Implemented | Skill definitions, SP costs, multipliers |
| `server/src/ro_status_effects.js` | **To implement** | Status effect config, resistance formulas, application logic |

### Client Files

| File | Status | Purpose |
|------|--------|---------|
| `UI/DamageNumberSubsystem.h/.cpp` | Implemented | Wraps combat events, feeds overlay |
| `UI/SDamageNumberOverlay.h/.cpp` | Implemented | OnPaint rendering, pool, animation |
| `UI/WorldHealthBarSubsystem.h/.cpp` | Implemented | Floating HP/SP bars |
| `UI/BasicInfoSubsystem.h/.cpp` | Implemented | Player HP/SP/EXP panel |
| `UI/CastBarSubsystem.h/.cpp` | Implemented | Cast time bars |
| `VFX/SkillVFXSubsystem.h/.cpp` | Implemented | Skill particle effects |
| `UI/CombatSubsystem.h/.cpp` | **To implement** | Centralized combat state, attack commands |
| `UI/StatusEffectSubsystem.h/.cpp` | **To implement** | Status effect icons, VFX, duration tracking |
| `UI/SStatusEffectOverlay.h/.cpp` | **To implement** | Status icon rendering widget |

---

## Appendix B: Implementation Priority Order

1. **Status Effect Module** (`ro_status_effects.js`) -- Server-side module with all 10 status configs, resistance formulas, application/removal logic. Required before any CC skills work correctly.

2. **CombatSubsystem** (`CombatSubsystem.h/.cpp`) -- Client-side centralized combat state. Reduces scattered combat logic and provides a clean API for Blueprint and C++ callers.

3. **StatusEffectSubsystem** (`StatusEffectSubsystem.h/.cpp`) -- Client-side status effect tracking and visual feedback. Depends on `status:applied`/`status:removed` events being emitted by the server.

4. **Element Table Audit** -- Compare `ELEMENT_TABLE` in `ro_damage_formulas.js` against the canonical RateMyServer table (documented in `02_Combat_System.md` Section 7.2). Fix discrepancies.

5. **Animation Integration** -- Wire attack/hit/death montages through C++ or Blueprint. Currently handled in Blueprint; a C++ path provides better performance for high-frequency combat.
