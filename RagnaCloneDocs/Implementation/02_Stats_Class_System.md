# 02 - Stats, Leveling, and Class System -- Implementation Guide

> **Audience**: Claude Code (AI coding assistant) implementing features in the Sabri_MMO codebase.
> **Scope**: RO pre-renewal stat formulas, job/class system, leveling, EXP, stat allocation UI.
> **Source of truth**: `RagnaCloneDocs/01_Stats_Leveling_JobSystem.md` for formulas, this document for code.

---

## Table of Contents

1. [Server-Side Stat System (Node.js)](#1-server-side-stat-system)
2. [Server-Side Class/Job System](#2-server-side-classjob-system)
3. [Server-Side Leveling](#3-server-side-leveling)
4. [Client-Side: Stats Subsystem (UE5 C++)](#4-client-side-stats-subsystem)
5. [Client-Side: Stats UI (Slate)](#5-client-side-stats-ui)
6. [Level Up Effects, DB Schema, Testing Checklist](#6-level-up-effects-db-schema-testing)

---

## Current State of the Codebase

Before implementing anything, understand what already exists:

### Server (Already Implemented)
- **`server/src/ro_exp_tables.js`** -- Base EXP table (levels 1-98), Job EXP tables (Novice/First/Second), `JOB_CLASS_CONFIG` (21 classes), `FIRST_CLASSES`, `SECOND_CLASS_UPGRADES`, stat/skill point formulas
- **`server/src/ro_damage_formulas.js`** -- `calculateDerivedStats()` (SIMPLIFIED -- not true RO formulas), element table, size penalty, damage formulas
- **`server/src/index.js`** -- `processExpGain()`, `buildExpPayload()`, `buildFullStatsPayload()`, `getEffectiveStats()`, socket handlers for `player:allocate_stat`, `player:request_stats`, `job:change`

### Client (Already Implemented)
- **`CombatStatsSubsystem.h/.cpp`** -- F8 toggle, reads `player:stats`, has stat allocation via `AllocateStat()`, shows base stats with [+] buttons, derived stats (ATK, MATK, HIT, FLEE, CRI, ASPD, DEF, MDEF, P.Dodge)
- **`SCombatStatsWidget.h/.cpp`** -- Full Slate widget with gold/dark RO theme, draggable/resizable, `BuildAllocatableStatRow()` pattern
- **`BasicInfoSubsystem.h/.cpp`** -- HP/SP/EXP bars, reads `player:stats`, `exp:gain`, `exp:level_up`
- **`CharacterData.h`** -- `FCharacterData` with Str/Agi/Vit/IntStat/Dex/Luk/StatPoints fields

### What Needs Upgrading
1. **`ro_damage_formulas.js::calculateDerivedStats()`** -- Currently uses simplified formulas (MaxHP = 100 + VIT*8 + Level*10). Needs true RO class-aware HP/SP, proper ASPD with weapon delay tables, proper Soft DEF/MDEF.
2. **`ro_exp_tables.js`** -- Missing transcendent EXP tables, transcendent class configs, ASPD base delay tables, HP/SP class coefficients, job bonus stats.
3. **`CombatStatsSubsystem`** -- Missing job bonus stat display, job info, MATK min~max range. Could be extended or a new `StatsSubsystem` created.
4. **No `stat_calculator.js` module** exists yet -- all stat logic is inline. Should be extracted for testability.

---

## 1. Server-Side Stat System

### 1.1 Create `server/src/stat_calculator.js`

Extract all stat formulas into a dedicated, testable module. This module has NO side effects -- pure functions only.

```javascript
// server/src/stat_calculator.js
// ============================================================
// Ragnarok Online Pre-Renewal Stat Calculator
// Pure functions -- no DB, no socket, no side effects.
// All intermediate results use Math.floor() (integer math).
// ============================================================

'use strict';

// ============================================================
// HP/SP Class Coefficients (from 01_Stats_Leveling_JobSystem.md)
// ============================================================
const HP_SP_COEFFICIENTS = {
    novice:     { hpA: 0.0,  hpB: 5.0, spA: 0.0, spB: 2.0 },
    high_novice:{ hpA: 0.0,  hpB: 5.0, spA: 0.0, spB: 2.0 },
    super_novice:{ hpA: 0.0, hpB: 5.0, spA: 0.0, spB: 2.0 },
    swordsman:  { hpA: 0.7,  hpB: 5.0, spA: 0.2, spB: 2.0 },
    mage:       { hpA: 0.3,  hpB: 5.0, spA: 0.6, spB: 2.0 },
    archer:     { hpA: 0.5,  hpB: 5.0, spA: 0.4, spB: 2.0 },
    merchant:   { hpA: 0.4,  hpB: 5.0, spA: 0.3, spB: 2.0 },
    thief:      { hpA: 0.5,  hpB: 5.0, spA: 0.3, spB: 2.0 },
    acolyte:    { hpA: 0.4,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    knight:     { hpA: 1.5,  hpB: 5.0, spA: 0.4, spB: 2.0 },
    wizard:     { hpA: 0.55, hpB: 5.0, spA: 1.0, spB: 2.0 },
    hunter:     { hpA: 0.85, hpB: 5.0, spA: 0.6, spB: 2.0 },
    assassin:   { hpA: 1.1,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    blacksmith: { hpA: 0.9,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    priest:     { hpA: 0.75, hpB: 5.0, spA: 0.8, spB: 2.0 },
    crusader:   { hpA: 1.1,  hpB: 7.0, spA: 0.5, spB: 2.0 },
    sage:       { hpA: 0.75, hpB: 5.0, spA: 0.8, spB: 2.0 },
    bard:       { hpA: 0.75, hpB: 3.0, spA: 0.6, spB: 2.0 },
    dancer:     { hpA: 0.75, hpB: 3.0, spA: 0.6, spB: 2.0 },
    alchemist:  { hpA: 0.9,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    rogue:      { hpA: 0.85, hpB: 5.0, spA: 0.5, spB: 2.0 },
    monk:       { hpA: 0.9,  hpB: 6.5, spA: 0.5, spB: 2.0 },
    // Transcendent classes use SAME coefficients as their base class
    lord_knight:    { hpA: 1.5,  hpB: 5.0, spA: 0.4, spB: 2.0 },
    high_wizard:    { hpA: 0.55, hpB: 5.0, spA: 1.0, spB: 2.0 },
    sniper:         { hpA: 0.85, hpB: 5.0, spA: 0.6, spB: 2.0 },
    whitesmith:     { hpA: 0.9,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    assassin_cross: { hpA: 1.1,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    high_priest:    { hpA: 0.75, hpB: 5.0, spA: 0.8, spB: 2.0 },
    paladin:        { hpA: 1.1,  hpB: 7.0, spA: 0.5, spB: 2.0 },
    scholar:        { hpA: 0.75, hpB: 5.0, spA: 0.8, spB: 2.0 },
    minstrel:       { hpA: 0.75, hpB: 3.0, spA: 0.6, spB: 2.0 },
    gypsy:          { hpA: 0.75, hpB: 3.0, spA: 0.6, spB: 2.0 },
    biochemist:     { hpA: 0.9,  hpB: 5.0, spA: 0.5, spB: 2.0 },
    stalker:        { hpA: 0.85, hpB: 5.0, spA: 0.5, spB: 2.0 },
    champion:       { hpA: 0.9,  hpB: 6.5, spA: 0.5, spB: 2.0 }
};

// ============================================================
// Weight Bonus Per Job Class
// ============================================================
const WEIGHT_BONUS = {
    novice: 0, high_novice: 0, super_novice: 0,
    swordsman: 800, mage: 200, archer: 600, merchant: 800, thief: 400, acolyte: 400,
    knight: 800, wizard: 200, hunter: 600, assassin: 600, blacksmith: 800, priest: 600,
    crusader: 800, sage: 400, bard: 600, dancer: 600, alchemist: 800, rogue: 600, monk: 600,
    lord_knight: 800, high_wizard: 200, sniper: 600, whitesmith: 800, assassin_cross: 600,
    high_priest: 600, paladin: 800, scholar: 400, minstrel: 600, gypsy: 600,
    biochemist: 800, stalker: 600, champion: 600
};

// ============================================================
// ASPD Base Weapon Delays (BTBA in "delay units" = seconds * 100)
// Format: { weaponType: delay } per class
// Source: rAthena job_db pre-renewal
// ============================================================
const ASPD_BASE_DELAYS = {
    novice:     { bare_hand: 50, dagger: 55 },
    swordsman:  { bare_hand: 40, dagger: 65, one_hand_sword: 70, two_hand_sword: 60, spear: 65, two_hand_spear: 65, mace: 70, axe: 80, two_hand_axe: 80 },
    mage:       { bare_hand: 35, dagger: 60, staff: 65 },
    archer:     { bare_hand: 50, dagger: 55, bow: 70 },
    thief:      { bare_hand: 40, dagger: 50, one_hand_sword: 70, bow: 85 },
    merchant:   { bare_hand: 40, dagger: 65, one_hand_sword: 55, mace: 65, axe: 70, two_hand_axe: 70 },
    acolyte:    { bare_hand: 40, dagger: 60, mace: 70, staff: 65 },
    knight:     { bare_hand: 38, dagger: 60, one_hand_sword: 55, two_hand_sword: 50, spear: 55, two_hand_spear: 55, mace: 60, axe: 70, two_hand_axe: 70 },
    wizard:     { bare_hand: 35, dagger: 58, staff: 60 },
    hunter:     { bare_hand: 48, dagger: 55, bow: 60 },
    assassin:   { bare_hand: 38, dagger: 45, one_hand_sword: 65, katar: 42 },
    blacksmith: { bare_hand: 38, dagger: 60, one_hand_sword: 52, mace: 60, axe: 62, two_hand_axe: 62 },
    priest:     { bare_hand: 40, mace: 62, staff: 60, knuckle: 55 },
    crusader:   { bare_hand: 38, dagger: 62, one_hand_sword: 58, two_hand_sword: 55, spear: 58, two_hand_spear: 58, mace: 62 },
    sage:       { bare_hand: 35, dagger: 58, staff: 60, book: 58 },
    bard:       { bare_hand: 45, dagger: 55, bow: 62, instrument: 58 },
    dancer:     { bare_hand: 45, dagger: 55, bow: 62, whip: 58 },
    rogue:      { bare_hand: 38, dagger: 48, one_hand_sword: 62, bow: 75 },
    monk:       { bare_hand: 36, mace: 60, staff: 62, knuckle: 42 },
    alchemist:  { bare_hand: 38, dagger: 60, one_hand_sword: 52, mace: 60, axe: 62, two_hand_axe: 62 }
    // Transcendent classes use same delays as their base class
};

// Map transcendent classes to their base class for ASPD lookup
const TRANS_TO_BASE_CLASS = {
    lord_knight: 'knight', high_wizard: 'wizard', sniper: 'hunter',
    whitesmith: 'blacksmith', assassin_cross: 'assassin', high_priest: 'priest',
    paladin: 'crusader', scholar: 'sage', minstrel: 'bard', gypsy: 'dancer',
    biochemist: 'alchemist', stalker: 'rogue', champion: 'monk',
    high_novice: 'novice',
    // Also map first classes that use "high_" prefix
    high_swordsman: 'swordsman', high_mage: 'mage', high_archer: 'archer',
    high_thief: 'thief', high_merchant: 'merchant', high_acolyte: 'acolyte'
};

// Transcendent class set (for 1.25x HP/SP modifier)
const TRANSCENDENT_CLASSES = new Set([
    'lord_knight', 'high_wizard', 'sniper', 'whitesmith', 'assassin_cross',
    'high_priest', 'paladin', 'scholar', 'minstrel', 'gypsy',
    'biochemist', 'stalker', 'champion', 'high_novice',
    'high_swordsman', 'high_mage', 'high_archer',
    'high_thief', 'high_merchant', 'high_acolyte'
]);

// ============================================================
// Stat Point Formulas
// ============================================================

/**
 * Cost to raise a stat from currentValue to currentValue+1.
 * RO Classic: floor((currentStat - 1) / 10) + 2
 */
function getStatPointCost(currentValue) {
    return Math.floor((currentValue - 1) / 10) + 2;
}

/**
 * Stat points gained when reaching a specific base level.
 * RO Classic: floor(level / 5) + 3
 */
function getStatPointsForLevel(level) {
    return Math.floor(level / 5) + 3;
}

/**
 * Total cumulative stat points available at a given base level.
 * Novice: starts with 48 distributable (6 stats at 1 = base of 6, + 48).
 * Transcendent: starts with 100 distributable.
 */
function getTotalStatPoints(baseLevel, isTranscendent) {
    let total = isTranscendent ? 100 : 48;
    for (let lv = 2; lv <= baseLevel; lv++) {
        total += getStatPointsForLevel(lv);
    }
    return total;
}

/**
 * Total cost to raise a stat from 1 to targetValue.
 * Used for validation: sum of costs from 1 to targetValue-1.
 */
function getCumulativeStatCost(targetValue) {
    let total = 0;
    for (let v = 1; v < targetValue; v++) {
        total += getStatPointCost(v);
    }
    return total;
}

// ============================================================
// HP/SP Calculation (True RO Pre-Renewal)
// ============================================================

/**
 * Calculate Base HP for a given class and level.
 * BaseHP = 35 + (BaseLevel * HP_JOB_B)
 * for i = 2 to BaseLevel: BaseHP += round(HP_JOB_A * i)
 */
function calculateBaseHP(baseLevel, hpJobA, hpJobB) {
    let baseHP = 35 + Math.floor(baseLevel * hpJobB);
    for (let i = 2; i <= baseLevel; i++) {
        baseHP += Math.round(hpJobA * i);
    }
    return baseHP;
}

/**
 * Calculate Max HP with VIT scaling and transcendent modifier.
 * MaxHP = floor(BaseHP * (1 + VIT * 0.01) * TransMod)
 * Then apply additive bonuses, then multiplicative bonuses.
 */
function calculateMaxHP(baseLevel, vit, jobClass, isTranscendent, additiveMod, multiplicativeMod) {
    const coeff = HP_SP_COEFFICIENTS[jobClass] || HP_SP_COEFFICIENTS.novice;
    const baseHP = calculateBaseHP(baseLevel, coeff.hpA, coeff.hpB);
    const transMod = isTranscendent ? 1.25 : 1.0;
    let maxHP = Math.floor(baseHP * (1 + vit * 0.01) * transMod);
    maxHP += (additiveMod || 0);
    maxHP = Math.floor(maxHP * (1 + (multiplicativeMod || 0) * 0.01));
    return Math.max(1, maxHP);
}

/**
 * Calculate Base SP for a given class and level.
 * BaseSP = 10 + (BaseLevel * SP_JOB_B)
 * for i = 2 to BaseLevel: BaseSP += round(SP_JOB_A * i)
 */
function calculateBaseSP(baseLevel, spJobA, spJobB) {
    let baseSP = 10 + Math.floor(baseLevel * spJobB);
    for (let i = 2; i <= baseLevel; i++) {
        baseSP += Math.round(spJobA * i);
    }
    return baseSP;
}

/**
 * Calculate Max SP with INT scaling and transcendent modifier.
 * MaxSP = floor(BaseSP * (1 + INT * 0.01) * TransMod)
 */
function calculateMaxSP(baseLevel, intStat, jobClass, isTranscendent, additiveMod, multiplicativeMod) {
    const coeff = HP_SP_COEFFICIENTS[jobClass] || HP_SP_COEFFICIENTS.novice;
    const baseSP = calculateBaseSP(baseLevel, coeff.spA, coeff.spB);
    const transMod = isTranscendent ? 1.25 : 1.0;
    let maxSP = Math.floor(baseSP * (1 + intStat * 0.01) * transMod);
    maxSP += (additiveMod || 0);
    maxSP = Math.floor(maxSP * (1 + (multiplicativeMod || 0) * 0.01));
    return Math.max(1, maxSP);
}

// ============================================================
// ATK / MATK
// ============================================================

/**
 * StatusATK for melee weapons.
 * floor(BaseLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
 */
function calculateMeleeStatusATK(baseLevel, str, dex, luk) {
    return Math.floor(baseLevel / 4) + str + Math.floor(str / 10) ** 2
         + Math.floor(dex / 5) + Math.floor(luk / 3);
}

/**
 * StatusATK for ranged weapons.
 * floor(BaseLv/4) + floor(STR/5) + DEX + floor(DEX/10)^2 + floor(LUK/3)
 */
function calculateRangedStatusATK(baseLevel, str, dex, luk) {
    return Math.floor(baseLevel / 4) + Math.floor(str / 5) + dex
         + Math.floor(dex / 10) ** 2 + Math.floor(luk / 3);
}

/**
 * MATK range from INT.
 * Min: INT + floor(INT/7)^2
 * Max: INT + floor(INT/5)^2
 */
function calculateMATK(intStat) {
    return {
        min: intStat + Math.floor(intStat / 7) ** 2,
        max: intStat + Math.floor(intStat / 5) ** 2
    };
}

// ============================================================
// DEF / MDEF
// ============================================================

/**
 * Soft DEF (stat-based, flat subtraction after Hard DEF).
 * floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)
 */
function calculateSoftDEF(vit, agi, baseLevel) {
    return Math.floor(vit / 2) + Math.floor(agi / 5) + Math.floor(baseLevel / 2);
}

/**
 * Apply Hard DEF percentage reduction to damage.
 * DamageAfterHardDEF = Damage * (4000 + HardDEF) / (4000 + HardDEF * 10)
 */
function applyHardDEF(damage, hardDEF) {
    if (hardDEF <= 0) return damage;
    return Math.floor(damage * (4000 + hardDEF) / (4000 + hardDEF * 10));
}

/**
 * Soft MDEF (stat-based).
 * INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)
 */
function calculateSoftMDEF(intStat, vit, dex, baseLevel) {
    return intStat + Math.floor(vit / 5) + Math.floor(dex / 5) + Math.floor(baseLevel / 4);
}

/**
 * Apply Hard MDEF percentage reduction to magic damage.
 * DamageAfterHardMDEF = Damage * (1000 + MDEF) / (1000 + MDEF * 10)
 */
function applyHardMDEF(damage, hardMDEF) {
    if (hardMDEF <= 0) return damage;
    return Math.floor(damage * (1000 + hardMDEF) / (1000 + hardMDEF * 10));
}

// ============================================================
// HIT / FLEE / Perfect Dodge / Critical
// ============================================================

/**
 * HIT accuracy.
 * BaseLv + DEX + BonusHIT
 * (Some sources also add floor(LUK/3) -- we include it for completeness.)
 */
function calculateHIT(baseLevel, dex, bonusHIT) {
    return baseLevel + dex + (bonusHIT || 0);
}

/**
 * Base Flee (A component).
 * BaseLv + AGI + BonusFlee
 */
function calculateFlee(baseLevel, agi, bonusFlee) {
    return baseLevel + agi + (bonusFlee || 0);
}

/**
 * Perfect Dodge (B component of Flee).
 * floor(LUK / 10) + BonusPD
 */
function calculatePerfectDodge(luk, bonusPD) {
    return Math.floor(luk / 10) + (bonusPD || 0);
}

/**
 * Critical rate (percentage).
 * floor(LUK * 0.3) + 1 + BonusCrit
 */
function calculateCritRate(luk, bonusCrit) {
    return Math.floor(luk * 0.3) + 1 + (bonusCrit || 0);
}

/**
 * Hit rate against a target.
 * HitRate = 80 + HIT - TargetFlee, clamped [5, 100]
 */
function calculateHitRate(attackerHIT, targetFlee) {
    return Math.max(5, Math.min(100, 80 + attackerHIT - targetFlee));
}

// ============================================================
// ASPD
// ============================================================

/**
 * Get weapon delay for a class + weapon type combination.
 * Returns delay in "delay units" (BTBA * 100).
 * Falls back through transcendent -> base class -> bare_hand.
 */
function getWeaponDelay(jobClass, weaponType) {
    // Try direct class lookup
    let classDelays = ASPD_BASE_DELAYS[jobClass];

    // If transcendent, fall back to base class
    if (!classDelays) {
        const baseClass = TRANS_TO_BASE_CLASS[jobClass];
        if (baseClass) {
            classDelays = ASPD_BASE_DELAYS[baseClass];
        }
    }

    // Final fallback to novice
    if (!classDelays) {
        classDelays = ASPD_BASE_DELAYS.novice;
    }

    // Weapon type lookup with bare_hand fallback
    const wt = weaponType || 'bare_hand';
    return classDelays[wt] || classDelays.bare_hand || 50;
}

/**
 * Calculate ASPD (0-190, higher = faster).
 *
 * Pre-renewal formula:
 * ASPD = 200 - (WeaponDelay - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SpeedMod)
 *
 * @param {string} jobClass - Character's job class
 * @param {string} weaponType - Equipped weapon type
 * @param {number} agi - Effective AGI
 * @param {number} dex - Effective DEX
 * @param {number} speedModifier - Sum of ASPD% buffs (0.0 to 1.0)
 * @returns {number} ASPD value clamped to [0, 190]
 */
function calculateASPD(jobClass, weaponType, agi, dex, speedModifier) {
    const wd = getWeaponDelay(jobClass, weaponType);
    const agiReduction = Math.floor(wd * agi / 25);
    const dexReduction = Math.floor(wd * dex / 100);
    const totalReduction = Math.floor((agiReduction + dexReduction) / 10);
    const baseASPD = 200 - (wd - totalReduction) * (1 - (speedModifier || 0));
    return Math.min(190, Math.max(0, Math.floor(baseASPD)));
}

/**
 * Convert ASPD to attack delay in milliseconds.
 * AttackDelay_ms = (200 - ASPD) * 10
 */
function aspdToDelayMs(aspd) {
    return Math.max(100, (200 - aspd) * 10);
}

// ============================================================
// Regeneration
// ============================================================

/**
 * HP regen per tick (6 second tick).
 * max(1, floor(MaxHP/200)) + floor(VIT/5)
 */
function calculateHPRegen(maxHP, vit) {
    return Math.max(1, Math.floor(maxHP / 200)) + Math.floor(vit / 5);
}

/**
 * SP regen per tick (8 second tick).
 * floor(MaxSP/100) + floor(INT/6) + 1
 * If INT >= 120: additional bonus of floor(INT/2) - 56
 */
function calculateSPRegen(maxSP, intStat) {
    let regen = Math.floor(maxSP / 100) + Math.floor(intStat / 6) + 1;
    if (intStat >= 120) {
        regen += Math.floor(intStat / 2) - 56;
    }
    return regen;
}

// ============================================================
// Cast Time
// ============================================================

/**
 * Pre-renewal cast time.
 * FinalCastTime = BaseCastTime * (1 - DEX/150)
 * At 150 DEX: instant cast.
 */
function calculateCastTime(baseCastTimeMs, dex, bonusReductionPercent) {
    const dexReduction = Math.min(1.0, dex / 150);
    const totalReduction = Math.min(1.0, dexReduction + (bonusReductionPercent || 0) / 100);
    return Math.max(0, Math.floor(baseCastTimeMs * (1 - totalReduction)));
}

// ============================================================
// Weight Limit
// ============================================================

/**
 * Max carrying weight.
 * 2000 + STR * 30 + JobWeightBonus
 */
function calculateMaxWeight(str, jobClass) {
    return 2000 + str * 30 + (WEIGHT_BONUS[jobClass] || 0);
}

// ============================================================
// Full Derived Stats Recalculation
// ============================================================

/**
 * Calculate ALL derived stats for a character.
 * This is the single function called whenever stats, equipment, or buffs change.
 *
 * @param {Object} params
 * @param {number} params.baseLevel
 * @param {string} params.jobClass
 * @param {boolean} params.isTranscendent
 * @param {number} params.str - Effective STR (base + job bonus + equipment + buffs)
 * @param {number} params.agi - Effective AGI
 * @param {number} params.vit - Effective VIT
 * @param {number} params.int - Effective INT
 * @param {number} params.dex - Effective DEX
 * @param {number} params.luk - Effective LUK
 * @param {string} params.weaponType - 'bare_hand', 'dagger', 'bow', etc.
 * @param {number} params.weaponATK - Base weapon damage
 * @param {number} params.hardDEF - Equipment DEF total
 * @param {number} params.hardMDEF - Equipment MDEF total
 * @param {number} params.bonusHIT - Equipment HIT bonus
 * @param {number} params.bonusFlee - Equipment Flee bonus
 * @param {number} params.bonusCrit - Equipment Crit bonus
 * @param {number} params.bonusPD - Equipment Perfect Dodge bonus
 * @param {number} params.bonusMaxHP - Additive Max HP bonus
 * @param {number} params.bonusMaxSP - Additive Max SP bonus
 * @param {number} params.aspdSpeedMod - ASPD% buff total (0.0 to 1.0)
 * @param {number} params.hpMultiplier - Multiplicative HP% bonus
 * @param {number} params.spMultiplier - Multiplicative SP% bonus
 * @returns {Object} All derived stats
 */
function recalculateAllStats(params) {
    const {
        baseLevel = 1, jobClass = 'novice', isTranscendent = false,
        str = 1, agi = 1, vit = 1, int: intStat = 1, dex = 1, luk = 1,
        weaponType = 'bare_hand', weaponATK = 0,
        hardDEF = 0, hardMDEF = 0,
        bonusHIT = 0, bonusFlee = 0, bonusCrit = 0, bonusPD = 0,
        bonusMaxHP = 0, bonusMaxSP = 0,
        aspdSpeedMod = 0, hpMultiplier = 0, spMultiplier = 0
    } = params;

    const isRanged = ['bow', 'gun', 'instrument', 'whip'].includes(weaponType);

    // ATK
    const statusATK = isRanged
        ? calculateRangedStatusATK(baseLevel, str, dex, luk)
        : calculateMeleeStatusATK(baseLevel, str, dex, luk);

    // MATK
    const matk = calculateMATK(intStat);

    // DEF
    const softDEF = calculateSoftDEF(vit, agi, baseLevel);
    const softMDEF = calculateSoftMDEF(intStat, vit, dex, baseLevel);

    // HIT / Flee
    const hit = calculateHIT(baseLevel, dex, bonusHIT);
    const flee = calculateFlee(baseLevel, agi, bonusFlee);
    const perfectDodge = calculatePerfectDodge(luk, bonusPD);

    // Critical
    const critical = calculateCritRate(luk, bonusCrit);

    // ASPD
    const aspd = calculateASPD(jobClass, weaponType, agi, dex, aspdSpeedMod);
    const attackDelayMs = aspdToDelayMs(aspd);

    // HP / SP
    const maxHP = calculateMaxHP(baseLevel, vit, jobClass, isTranscendent, bonusMaxHP, hpMultiplier);
    const maxSP = calculateMaxSP(baseLevel, intStat, jobClass, isTranscendent, bonusMaxSP, spMultiplier);

    // Regen
    const hpRegen = calculateHPRegen(maxHP, vit);
    const spRegen = calculateSPRegen(maxSP, intStat);

    // Weight
    const maxWeight = calculateMaxWeight(str, jobClass);

    return {
        statusATK, weaponATK,
        statusMATK: matk.min,   // For status window display
        matkMin: matk.min, matkMax: matk.max,
        hardDEF, softDEF,
        hardMDEF, softMDEF,
        hit, flee, perfectDodge,
        critical,
        aspd, attackDelayMs,
        maxHP, maxSP,
        hpRegen, spRegen,
        maxWeight,
        castTimeReduction: Math.min(1.0, dex / 150)
    };
}

// ============================================================
// Stat Allocation Validation
// ============================================================

/**
 * Validate and apply a stat allocation request.
 * Returns { success, newValue, cost, error } without side effects.
 *
 * @param {number} currentValue - Current base stat value (1-99)
 * @param {number} availablePoints - Player's unspent stat points
 * @param {number} amount - Number of points to raise (default 1)
 * @returns {Object} { success, newValue, totalCost, error }
 */
function validateStatAllocation(currentValue, availablePoints, amount = 1) {
    if (currentValue >= 99) {
        return { success: false, newValue: currentValue, totalCost: 0, error: 'Stat already at maximum (99)' };
    }

    let totalCost = 0;
    let newValue = currentValue;

    for (let i = 0; i < amount; i++) {
        if (newValue >= 99) break;
        const cost = getStatPointCost(newValue);
        if (totalCost + cost > availablePoints) {
            if (i === 0) {
                return { success: false, newValue: currentValue, totalCost: 0, error: `Not enough stat points (need ${cost}, have ${availablePoints})` };
            }
            break; // Apply partial allocation
        }
        totalCost += cost;
        newValue++;
    }

    return { success: true, newValue, totalCost, error: null };
}

module.exports = {
    // Constants
    HP_SP_COEFFICIENTS,
    WEIGHT_BONUS,
    ASPD_BASE_DELAYS,
    TRANS_TO_BASE_CLASS,
    TRANSCENDENT_CLASSES,

    // Stat points
    getStatPointCost,
    getStatPointsForLevel,
    getTotalStatPoints,
    getCumulativeStatCost,

    // HP/SP
    calculateBaseHP,
    calculateMaxHP,
    calculateBaseSP,
    calculateMaxSP,

    // ATK/MATK
    calculateMeleeStatusATK,
    calculateRangedStatusATK,
    calculateMATK,

    // DEF/MDEF
    calculateSoftDEF,
    applyHardDEF,
    calculateSoftMDEF,
    applyHardMDEF,

    // HIT/Flee/Crit
    calculateHIT,
    calculateFlee,
    calculatePerfectDodge,
    calculateCritRate,
    calculateHitRate,

    // ASPD
    getWeaponDelay,
    calculateASPD,
    aspdToDelayMs,

    // Regen
    calculateHPRegen,
    calculateSPRegen,

    // Cast time
    calculateCastTime,

    // Weight
    calculateMaxWeight,

    // Full recalc
    recalculateAllStats,

    // Validation
    validateStatAllocation
};
```

### 1.2 Integrating `stat_calculator.js` into `index.js`

The existing `calculateDerivedStats()` in `index.js` delegates to `ro_damage_formulas.js::calculateDerivedStats()` which uses simplified formulas. To upgrade:

**Step 1**: Import the new module at the top of `index.js`:

```javascript
const {
    recalculateAllStats: roRecalculateAllStats,
    getStatPointCost: roGetStatPointCost,
    validateStatAllocation,
    calculateMaxHP: roCalculateMaxHP,
    calculateMaxSP: roCalculateMaxSP,
    TRANSCENDENT_CLASSES
} = require('./stat_calculator');
```

**Step 2**: Replace the `calculateDerivedStats()` wrapper in `index.js`:

```javascript
// BEFORE (simplified):
function calculateDerivedStats(stats) {
    return roDerivedStats(stats);
}

// AFTER (class-aware):
function calculateDerivedStats(effectiveStats, player) {
    const jobClass = player ? (player.jobClass || 'novice') : 'novice';
    const isTranscendent = player ? TRANSCENDENT_CLASSES.has(jobClass) : false;

    return roRecalculateAllStats({
        baseLevel: effectiveStats.level || 1,
        jobClass,
        isTranscendent,
        str: effectiveStats.str || 1,
        agi: effectiveStats.agi || 1,
        vit: effectiveStats.vit || 1,
        int: effectiveStats.int || 1,
        dex: effectiveStats.dex || 1,
        luk: effectiveStats.luk || 1,
        weaponType: player ? (player.weaponType || 'bare_hand') : 'bare_hand',
        weaponATK: effectiveStats.weaponATK || 0,
        hardDEF: player ? (player.hardDef || 0) : 0,
        hardMDEF: player ? (player.hardMdef || 0) : 0,
        bonusHIT: effectiveStats.bonusHit || 0,
        bonusFlee: effectiveStats.bonusFlee || 0,
        bonusCrit: effectiveStats.bonusCritical || 0,
        bonusMaxHP: effectiveStats.bonusMaxHp || 0,
        bonusMaxSP: effectiveStats.bonusMaxSp || 0,
        aspdSpeedMod: player ? (player.aspdSpeedMod || 0) : 0
    });
}
```

**CRITICAL**: Update ALL call sites of `calculateDerivedStats()` to pass the `player` object as the second argument. Search for `calculateDerivedStats(` in `index.js` and update each one:

```javascript
// BEFORE:
const derived = calculateDerivedStats(effectiveStats);

// AFTER:
const derived = calculateDerivedStats(effectiveStats, player);
```

**Step 3**: Update `buildFullStatsPayload()` to include MATK range and job info:

```javascript
function buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd) {
    const statCosts = {
        str: getStatPointCost(player.stats.str || 1),
        agi: getStatPointCost(player.stats.agi || 1),
        vit: getStatPointCost(player.stats.vit || 1),
        int: getStatPointCost(player.stats.int || 1),
        dex: getStatPointCost(player.stats.dex || 1),
        luk: getStatPointCost(player.stats.luk || 1)
    };

    const jobBonuses = getJobBonusStats(player.jobClass, player.jobLevel);

    return {
        characterId,
        stats: {
            ...player.stats,
            str: effectiveStats.str, agi: effectiveStats.agi, vit: effectiveStats.vit,
            int: effectiveStats.int, dex: effectiveStats.dex, luk: effectiveStats.luk,
            hardDef: player.hardDef || 0,
            weaponATK: effectiveStats.weaponATK || 0
        },
        baseStats: {
            str: player.stats.str || 1, agi: player.stats.agi || 1, vit: player.stats.vit || 1,
            int: player.stats.int || 1, dex: player.stats.dex || 1, luk: player.stats.luk || 1
        },
        jobBonus: {
            str: jobBonuses.str, agi: jobBonuses.agi, vit: jobBonuses.vit,
            int: jobBonuses.int, dex: jobBonuses.dex, luk: jobBonuses.luk
        },
        statCosts,
        derived: {
            ...derived,
            aspd: finalAspd,
            matkMin: derived.matkMin,
            matkMax: derived.matkMax
        },
        job: {
            jobClass: player.jobClass || 'novice',
            displayName: (JOB_CLASS_CONFIG[player.jobClass] || {}).displayName || player.jobClass,
            jobLevel: player.jobLevel || 1,
            maxJobLevel: getMaxJobLevel(player.jobClass || 'novice'),
            isTranscendent: TRANSCENDENT_CLASSES.has(player.jobClass || 'novice')
        },
        exp: buildExpPayload(player)
    };
}
```

### 1.3 Equipment Bonus Aggregation

Equipment bonuses are already aggregated in `getEffectiveStats()` via `player.equipmentBonuses`. The flow:

1. When equipment changes (equip/unequip), recalculate `player.equipmentBonuses` by summing all equipped item stat bonuses.
2. `getEffectiveStats()` adds those bonuses to base stats.
3. `calculateDerivedStats()` uses effective stats for all derived calculations.

**Key integration point** -- When an item is equipped/unequipped in the `inventory:equip` / `inventory:unequip` handler, always:

```javascript
// After equip/unequip:
player.equipmentBonuses = recalculateEquipmentBonuses(player);
player.hardDef = player.equipmentBonuses.totalDef || 0;
player.hardMdef = player.equipmentBonuses.totalMdef || 0;
player.weaponATK = player.equipmentBonuses.weaponATK || 0;
player.weaponType = player.equipmentBonuses.weaponType || 'bare_hand';
player.weaponLevel = player.equipmentBonuses.weaponLevel || 1;
player.weaponElement = player.equipmentBonuses.weaponElement || 'neutral';

const effectiveStats = getEffectiveStats(player);
const derived = calculateDerivedStats(effectiveStats, player);
player.maxHealth = derived.maxHP;
player.maxMana = derived.maxSP;
player.health = Math.min(player.health, player.maxHealth);
player.mana = Math.min(player.mana, player.maxMana);
player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));

socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, player.aspd));
```

### 1.4 Full Recalculation Triggers

The following events MUST trigger a full `recalculateAllStats`:

| Trigger | Socket Event | Notes |
|---------|-------------|-------|
| Stat allocation | `player:allocate_stat` | Already implemented |
| Equipment change | `inventory:equip` / `inventory:unequip` | Recalculate all |
| Level up | `processExpGain()` | Already recalculates inside loop |
| Job change | `job:change` | Needs recalc for new HP/SP coefficients |
| Buff applied/removed | `skill:buff_applied` / `skill:buff_removed` | ATK/DEF multipliers change |
| Card inserted/removed | Future | Card bonuses change equipment totals |
| Login | `player:join` | Initial calculation |

---

## 2. Server-Side Class/Job System

### 2.1 Job Class Data (Extend `ro_exp_tables.js`)

The existing `JOB_CLASS_CONFIG` needs expansion. Add transcendent classes and HP/SP/weight data:

```javascript
// Add to ro_exp_tables.js -- Transcendent Job EXP Table (levels 50-69)
const TRANS_SECOND_CLASS_JOB_EXP_TABLE = {
    // Levels 1-49 use SECOND_CLASS_JOB_EXP_TABLE
    // Level 50+ uses these values
    50: 2125053,
    51: 4488362,
    52: 5765950,
    53: 7246770,
    54: 9498810,
    55: 12658800,
    56: 17127600,
    57: 22275900,
    58: 29586000,
    59: 37170000,
    60: 52050000,
    61: 58821000,
    62: 68280000,
    63: 78750000,
    64: 89250000,
    65: 99750000,
    66: 110250000,
    67: 120750000,
    68: 131250000,
    69: 141750000
};

// Add transcendent classes to JOB_CLASS_CONFIG:
const TRANSCENDENT_JOB_CONFIG = {
    'lord_knight':    { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Lord Knight',     parentClass: 'knight' },
    'high_wizard':    { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'High Wizard',     parentClass: 'wizard' },
    'sniper':         { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Sniper',          parentClass: 'hunter' },
    'whitesmith':     { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Whitesmith',      parentClass: 'blacksmith' },
    'assassin_cross': { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Assassin Cross',  parentClass: 'assassin' },
    'high_priest':    { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'High Priest',     parentClass: 'priest' },
    'paladin':        { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Paladin',         parentClass: 'crusader' },
    'scholar':        { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Scholar',         parentClass: 'sage' },
    'minstrel':       { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Minstrel',        parentClass: 'bard' },
    'gypsy':          { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Gypsy',           parentClass: 'dancer' },
    'biochemist':     { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Biochemist',      parentClass: 'alchemist' },
    'stalker':        { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Stalker',         parentClass: 'rogue' },
    'champion':       { maxJobLevel: 70, jobExpTable: null, tier: 3, displayName: 'Champion',        parentClass: 'monk' }
};

// The jobExpTable for trans classes: levels 1-49 from SECOND_CLASS, 50-69 from TRANS
// Handled in getJobExpForNextLevel:
function getJobExpForNextLevel(jobClass, currentJobLevel) {
    const config = JOB_CLASS_CONFIG[jobClass];
    if (!config) return 0;
    if (currentJobLevel >= config.maxJobLevel) return 0;

    // Transcendent classes: use second class table for 1-49, trans table for 50-69
    if (config.tier === 3) {
        if (currentJobLevel < 50) {
            return SECOND_CLASS_JOB_EXP_TABLE[currentJobLevel] || 0;
        }
        return TRANS_SECOND_CLASS_JOB_EXP_TABLE[currentJobLevel] || 0;
    }

    return config.jobExpTable[currentJobLevel] || 0;
}
```

### 2.2 Job Bonus Stats (Add to `stat_calculator.js` or new `ro_job_bonus.js`)

```javascript
// server/src/ro_job_bonus.js
// Job bonus stats per class, cumulative at each job level.
// Format: JOB_BONUS[classId] = { jobLevel: { str, agi, vit, int, dex, luk } }
// Only milestone entries are stored; use getJobBonusStats() for interpolation.

'use strict';

const JOB_BONUS = {
    novice: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        2:  { str:0, agi:0, vit:0, int:0, dex:0, luk:1 },
        6:  { str:0, agi:1, vit:0, int:0, dex:1, luk:1 },
        8:  { str:1, agi:1, vit:1, int:0, dex:1, luk:1 },
        10: { str:1, agi:1, vit:1, int:1, dex:1, luk:1 }
    },
    swordsman: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        2:  { str:1, agi:0, vit:0, int:0, dex:0, luk:0 },
        6:  { str:1, agi:0, vit:1, int:0, dex:0, luk:0 },
        10: { str:1, agi:1, vit:1, int:0, dex:1, luk:0 },
        14: { str:2, agi:1, vit:1, int:0, dex:1, luk:0 },
        18: { str:2, agi:1, vit:2, int:0, dex:1, luk:0 },
        22: { str:2, agi:1, vit:2, int:0, dex:2, luk:0 },
        26: { str:3, agi:1, vit:2, int:0, dex:2, luk:0 },
        30: { str:3, agi:2, vit:3, int:0, dex:2, luk:0 },
        34: { str:3, agi:2, vit:3, int:0, dex:2, luk:0 },
        38: { str:4, agi:2, vit:4, int:0, dex:2, luk:0 },
        42: { str:4, agi:2, vit:4, int:0, dex:3, luk:0 },
        46: { str:4, agi:3, vit:4, int:0, dex:3, luk:0 },
        50: { str:5, agi:3, vit:5, int:0, dex:3, luk:0 }
    },
    mage: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        2:  { str:0, agi:0, vit:0, int:1, dex:0, luk:0 },
        6:  { str:0, agi:0, vit:0, int:1, dex:1, luk:0 },
        10: { str:0, agi:0, vit:0, int:2, dex:1, luk:0 },
        14: { str:0, agi:0, vit:0, int:2, dex:2, luk:0 },
        18: { str:0, agi:1, vit:0, int:3, dex:2, luk:0 },
        22: { str:0, agi:1, vit:0, int:3, dex:3, luk:0 },
        26: { str:0, agi:1, vit:0, int:4, dex:3, luk:0 },
        30: { str:0, agi:1, vit:0, int:4, dex:4, luk:0 },
        38: { str:0, agi:1, vit:0, int:5, dex:4, luk:0 },
        42: { str:0, agi:2, vit:0, int:5, dex:5, luk:0 },
        50: { str:0, agi:2, vit:0, int:6, dex:5, luk:0 }
    },
    archer: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:1, agi:5, vit:0, int:1, dex:5, luk:1 }
    },
    thief: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:3, agi:5, vit:1, int:0, dex:4, luk:2 }
    },
    merchant: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:4, agi:1, vit:4, int:1, dex:4, luk:1 }
    },
    acolyte: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:0, agi:2, vit:2, int:5, dex:3, luk:3 }
    },
    knight: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:8, agi:5, vit:7, int:1, dex:5, luk:1 }
    },
    wizard: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:0, agi:3, vit:0, int:9, dex:8, luk:0 }
    },
    hunter: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:3, agi:8, vit:1, int:2, dex:8, luk:2 }
    },
    assassin: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:5, agi:8, vit:2, int:0, dex:5, luk:4 }
    },
    blacksmith: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:6, agi:2, vit:6, int:2, dex:6, luk:2 }
    },
    priest: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:0, agi:3, vit:5, int:7, dex:5, luk:3 }
    },
    crusader: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:7, agi:3, vit:7, int:3, dex:5, luk:2 }
    },
    sage: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:0, agi:5, vit:0, int:8, dex:7, luk:0 }
    },
    bard: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:3, agi:6, vit:1, int:4, dex:6, luk:4 }
    },
    dancer: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:3, agi:6, vit:1, int:4, dex:6, luk:4 }
    },
    rogue: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:4, agi:7, vit:2, int:1, dex:6, luk:4 }
    },
    monk: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:6, agi:6, vit:3, int:3, dex:5, luk:4 }
    },
    alchemist: {
        1:  { str:0, agi:0, vit:0, int:0, dex:0, luk:0 },
        50: { str:4, agi:4, vit:4, int:4, dex:4, luk:4 }
    }
};

/**
 * Get job bonus stats for a class at a specific job level.
 * Uses the highest milestone <= jobLevel.
 * Returns { str, agi, vit, int, dex, luk }.
 */
function getJobBonusStats(jobClass, jobLevel) {
    const empty = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0 };
    const classData = JOB_BONUS[jobClass];
    if (!classData) return empty;

    // Find highest milestone <= jobLevel
    let bestLevel = 0;
    for (const lvStr of Object.keys(classData)) {
        const lv = parseInt(lvStr, 10);
        if (lv <= jobLevel && lv > bestLevel) {
            bestLevel = lv;
        }
    }

    return classData[bestLevel] || empty;
}

module.exports = { JOB_BONUS, getJobBonusStats };
```

### 2.3 Job Change Validation (Already Implemented)

The `job:change` socket handler in `index.js` (lines 2690-2793) already handles:
- Novice -> First Class (requires job level 10)
- First Class -> Second Class (requires job level 40+)
- Validation of valid upgrade paths via `SECOND_CLASS_UPGRADES`
- Resetting job level/exp on change
- Broadcasting to zone

**Missing**: Rebirth/Transcendent process. Add this handler:

```javascript
// Add to index.js after the job:change handler

socket.on('job:rebirth', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    const currentClass = player.jobClass || 'novice';
    const currentTier = getClassTier(currentClass);

    // Requirements: must be second class, base level 99, job level 50
    if (currentTier !== 2) {
        socket.emit('job:error', { message: 'Only second class characters can be reborn' });
        return;
    }
    if ((player.stats.level || 1) < 99) {
        socket.emit('job:error', { message: 'Must be Base Level 99 to be reborn' });
        return;
    }
    if ((player.jobLevel || 1) < 50) {
        socket.emit('job:error', { message: 'Must be Job Level 50 to be reborn' });
        return;
    }

    // Rebirth cost: 1,285,000 zeny
    const REBIRTH_COST = 1285000;
    if ((player.zuzucoin || 0) < REBIRTH_COST) {
        socket.emit('job:error', { message: `Need ${REBIRTH_COST.toLocaleString()} Zeny (have ${(player.zuzucoin || 0).toLocaleString()})` });
        return;
    }

    // Check weight < 100%, no skill points unspent
    if ((player.skillPoints || 0) > 0) {
        socket.emit('job:error', { message: 'All skill points must be allocated before rebirth' });
        return;
    }

    // Perform rebirth
    const oldClass = currentClass;
    player.zuzucoin -= REBIRTH_COST;
    player.jobClass = 'high_novice';
    player.jobLevel = 1;
    player.jobExp = 0;
    player.stats.level = 1;
    player.baseExp = 0;

    // Reset stats to 1, give 100 stat points (transcendent bonus)
    player.stats.str = 1;
    player.stats.agi = 1;
    player.stats.vit = 1;
    player.stats.int = 1;
    player.stats.dex = 1;
    player.stats.luk = 1;
    player.stats.statPoints = 100;

    player.skillPoints = 0;
    player.isTranscendent = true;

    // Recalculate all stats
    const effectiveStats = getEffectiveStats(player);
    const derived = calculateDerivedStats(effectiveStats, player);
    player.maxHealth = derived.maxHP;
    player.maxMana = derived.maxSP;
    player.health = player.maxHealth;
    player.mana = player.maxMana;

    // Save to DB
    try {
        await pool.query(
            `UPDATE characters SET
                job_class = $1, job_level = $2, job_exp = $3,
                level = $4, base_exp = $5,
                str = 1, agi = 1, vit = 1, int_stat = 1, dex = 1, luk = 1,
                stat_points = $6, skill_points = 0,
                is_transcendent = TRUE, rebirth_count = rebirth_count + 1,
                max_health = $7, max_mana = $8, health = $9, mana = $10,
                zuzucoin = $11
             WHERE character_id = $12`,
            [player.jobClass, 1, 0, 1, 0, 100,
             player.maxHealth, player.maxMana, player.health, player.mana,
             player.zuzucoin, characterId]
        );
    } catch (err) {
        logger.error(`[REBIRTH] DB save failed for char ${characterId}: ${err.message}`);
    }

    logger.info(`[REBIRTH] ${player.characterName} reborn from ${oldClass} -> high_novice`);

    socket.emit('job:reborn', {
        characterId,
        oldClass,
        newClass: 'high_novice',
        message: 'You have been reborn! Your journey begins anew...',
        exp: buildExpPayload(player)
    });

    const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
    socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd));
});
```

### 2.4 Transcendent Job Change Path

After rebirth, the player goes: High Novice -> High First Class -> Transcendent Second Class.

Add to `SECOND_CLASS_UPGRADES` and `FIRST_CLASSES`:

```javascript
// Add high first classes
const HIGH_FIRST_CLASSES = ['high_swordsman', 'high_mage', 'high_archer', 'high_acolyte', 'high_thief', 'high_merchant'];

// Map high first classes to their transcendent second classes
const TRANSCENDENT_UPGRADES = {
    'high_swordsman': ['lord_knight', 'paladin'],
    'high_mage':      ['high_wizard', 'scholar'],
    'high_archer':    ['sniper', 'minstrel', 'gypsy'],
    'high_acolyte':   ['high_priest', 'champion'],
    'high_thief':     ['assassin_cross', 'stalker'],
    'high_merchant':  ['whitesmith', 'biochemist']
};
```

### 2.5 Weapon Restrictions Per Class

Weapon restrictions are enforced on equipment. Add a lookup table:

```javascript
// In stat_calculator.js or a new ro_job_weapons.js
const CLASS_ALLOWED_WEAPONS = {
    novice:     ['bare_hand', 'dagger'],
    swordsman:  ['bare_hand', 'dagger', 'one_hand_sword', 'two_hand_sword', 'spear', 'two_hand_spear'],
    mage:       ['bare_hand', 'dagger', 'staff'],
    archer:     ['bare_hand', 'dagger', 'bow'],
    thief:      ['bare_hand', 'dagger', 'one_hand_sword', 'bow'],
    merchant:   ['bare_hand', 'dagger', 'one_hand_sword', 'axe', 'two_hand_axe', 'mace'],
    acolyte:    ['bare_hand', 'mace', 'staff'],
    knight:     ['bare_hand', 'dagger', 'one_hand_sword', 'two_hand_sword', 'spear', 'two_hand_spear', 'mace', 'axe'],
    wizard:     ['bare_hand', 'dagger', 'staff'],
    hunter:     ['bare_hand', 'dagger', 'bow'],
    assassin:   ['bare_hand', 'dagger', 'one_hand_sword', 'katar'],
    blacksmith: ['bare_hand', 'dagger', 'one_hand_sword', 'axe', 'two_hand_axe', 'mace'],
    priest:     ['bare_hand', 'mace', 'staff', 'knuckle', 'book'],
    crusader:   ['bare_hand', 'dagger', 'one_hand_sword', 'two_hand_sword', 'spear', 'two_hand_spear', 'mace'],
    sage:       ['bare_hand', 'dagger', 'staff', 'book'],
    bard:       ['bare_hand', 'dagger', 'bow', 'instrument'],
    dancer:     ['bare_hand', 'dagger', 'bow', 'whip'],
    rogue:      ['bare_hand', 'dagger', 'one_hand_sword', 'bow'],
    monk:       ['bare_hand', 'mace', 'staff', 'knuckle'],
    alchemist:  ['bare_hand', 'dagger', 'one_hand_sword', 'axe', 'two_hand_axe', 'mace']
    // Transcendent classes inherit from base class
};

function canEquipWeapon(jobClass, weaponType) {
    const baseClass = TRANS_TO_BASE_CLASS[jobClass] || jobClass;
    const allowed = CLASS_ALLOWED_WEAPONS[baseClass];
    if (!allowed) return true; // Unknown class, allow all
    return allowed.includes(weaponType);
}
```

---

## 3. Server-Side Leveling

### 3.1 EXP Gain from Monster Kills (Already Implemented)

The `processExpGain()` function in `index.js` (lines 718-814) already handles:
- Adding base EXP and job EXP
- Multi-level-up in a single kill
- Stat point awards per base level (using `getStatPointsForLevelUp`)
- Skill point awards per job level (always 1)
- Full heal on level up
- Recalculating derived stats after level up
- Saving to DB via `saveExpDataToDB()`

### 3.2 Monster Level Difference Modifiers (TO ADD)

Add this to `processExpGain()`:

```javascript
/**
 * Calculate EXP modifier based on level difference between player and monster.
 * @param {number} playerLevel - Player's base level
 * @param {number} monsterLevel - Monster's level
 * @returns {number} Modifier as a decimal (1.0 = 100%)
 */
function getExpLevelModifier(playerLevel, monsterLevel) {
    const diff = playerLevel - monsterLevel; // positive = player is higher

    if (diff <= -10) return 1.40;  // Monster 10+ above
    if (diff <= -6)  return 1.20;  // Monster 6-10 above
    if (diff <= -1)  return 1.10;  // Monster 1-5 above
    if (diff === 0)  return 1.00;  // Same level
    if (diff <= 5)   return 0.95;  // Monster 1-5 below
    if (diff <= 10)  return 0.80;  // Monster 6-10 below
    if (diff <= 15)  return 0.60;  // Monster 11-15 below
    if (diff <= 20)  return 0.35;  // Monster 16-20 below
    if (diff <= 25)  return 0.20;  // Monster 21-25 below
    if (diff <= 30)  return 0.15;  // Monster 26-30 below
    return 0.10;                   // Monster 31+ below
}

// Modify processExpGain to accept monsterLevel and apply modifier:
function processExpGain(player, baseExpReward, jobExpReward, monsterLevel, isMVP) {
    // MVP monsters are exempt from level difference penalty
    const modifier = isMVP ? 1.0 : getExpLevelModifier(player.stats.level || 1, monsterLevel || 1);
    const adjustedBaseExp = Math.floor(baseExpReward * modifier);
    const adjustedJobExp = Math.floor(jobExpReward * modifier);

    // ... rest of function uses adjustedBaseExp/adjustedJobExp instead of raw rewards
}
```

### 3.3 Death Penalty

Add to the death handler in `index.js`:

```javascript
/**
 * Apply death penalty: lose 1% of EXP required for current base level.
 * Job EXP is NOT lost. Cannot delevel. Novices have no death penalty.
 */
function applyDeathPenalty(player) {
    const jobClass = player.jobClass || 'novice';

    // Novices have no death penalty
    if (jobClass === 'novice' || jobClass === 'high_novice') {
        return 0;
    }

    const currentLevel = player.stats.level || 1;
    const expForThisLevel = getBaseExpForNextLevel(currentLevel);
    if (expForThisLevel <= 0) return 0;

    const penalty = Math.floor(expForThisLevel * 0.01); // 1% of EXP needed
    player.baseExp = Math.max(0, (player.baseExp || 0) - penalty);

    return penalty;
}
```

Call this from the existing `combat:death` handler after confirming the dead entity is a player.

### 3.4 Party EXP Sharing (Even Share)

```javascript
/**
 * Distribute EXP to party members using Even Share formula.
 * TotalEXP = BaseMonsterEXP * (1 + (PartySize - 1) * 0.20)
 * Each member gets TotalEXP / PartySize.
 *
 * Requirements: All members within 15 base levels of each other.
 *
 * @param {Array} partyMembers - Array of player objects in the party
 * @param {number} baseExpReward - Monster's base EXP
 * @param {number} jobExpReward - Monster's job EXP
 * @param {number} monsterLevel - Monster's level
 * @param {boolean} isMVP - Whether monster is an MVP
 * @returns {Array} Array of { player, baseExp, jobExp } per member
 */
function distributePartyEXP(partyMembers, baseExpReward, jobExpReward, monsterLevel, isMVP) {
    const size = partyMembers.length;
    if (size <= 1) {
        // Solo: full EXP to single member
        return partyMembers.map(p => ({
            player: p,
            baseExp: baseExpReward,
            jobExp: jobExpReward
        }));
    }

    // Check 15-level spread requirement
    const levels = partyMembers.map(p => p.stats.level || 1);
    const minLevel = Math.min(...levels);
    const maxLevel = Math.max(...levels);
    if (maxLevel - minLevel > 15) {
        // Fall back to Each Take (only killer gets EXP)
        return null; // Caller handles Each Take fallback
    }

    // Even Share bonus: +20% per additional member
    const bonusMultiplier = 1 + (size - 1) * 0.20;
    const totalBaseExp = Math.floor(baseExpReward * bonusMultiplier);
    const totalJobExp = Math.floor(jobExpReward * bonusMultiplier);
    const perMemberBase = Math.floor(totalBaseExp / size);
    const perMemberJob = Math.floor(totalJobExp / size);

    return partyMembers.map(p => ({
        player: p,
        baseExp: perMemberBase,
        jobExp: perMemberJob
    }));
}
```

---

## 4. Client-Side: Stats Subsystem (UE5 C++)

### 4.1 Architecture Decision: Extend CombatStatsSubsystem

The existing `CombatStatsSubsystem` already handles stat allocation and display. Rather than creating a duplicate subsystem, **extend** it with the additional fields needed for job info and MATK range.

### 4.2 Updated `CombatStatsSubsystem.h`

Add these fields to the existing header (file: `client/SabriMMO/Source/SabriMMO/UI/CombatStatsSubsystem.h`):

```cpp
// Add to CombatStatsSubsystem.h public section:

    // ---- Job information ----
    FString JobClass;           // Internal class ID: "knight", "wizard", etc.
    FString JobDisplayName;     // Display name: "Knight", "Wizard", etc.
    int32 JobLevel = 1;
    int32 MaxJobLevel = 50;
    bool bIsTranscendent = false;

    // ---- Job Bonus Stats (green text in RO stat window) ----
    int32 JobBonusSTR = 0;
    int32 JobBonusAGI = 0;
    int32 JobBonusVIT = 0;
    int32 JobBonusINT = 0;
    int32 JobBonusDEX = 0;
    int32 JobBonusLUK = 0;

    // ---- MATK range (not just single value) ----
    int32 MATKMin = 0;
    int32 MATKMax = 0;
    // Rename existing StatusMATK -> keep for backward compat but also add:
    int32 HardMDEF = 0;

    // ---- Weight ----
    int32 MaxWeight = 2000;
```

### 4.3 Updated `HandlePlayerStats` in `CombatStatsSubsystem.cpp`

Extend the event parser to read the new fields:

```cpp
void UCombatStatsSubsystem::HandlePlayerStats(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    // Check character ID
    double CharIdD = 0;
    Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
    if ((int32)CharIdD != LocalCharacterId && LocalCharacterId != 0) return;

    // ── Effective stats (base + equipment) ──
    const TSharedPtr<FJsonObject>* StatsObj = nullptr;
    if (Obj->TryGetObjectField(TEXT("stats"), StatsObj) && StatsObj)
    {
        double Val = 0;
        if ((*StatsObj)->TryGetNumberField(TEXT("str"), Val)) STR = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("agi"), Val)) AGI = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("vit"), Val)) VIT = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("int"), Val)) INT_Stat = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("dex"), Val)) DEX = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("luk"), Val)) LUK = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("level"), Val)) BaseLevel = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("weaponATK"), Val)) WeaponATK = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("hardDef"), Val)) HardDEF = (int32)Val;
        if ((*StatsObj)->TryGetNumberField(TEXT("statPoints"), Val)) StatPoints = (int32)Val;
    }

    // ── Base stats (without equipment) ──
    const TSharedPtr<FJsonObject>* BaseStatsObj = nullptr;
    if (Obj->TryGetObjectField(TEXT("baseStats"), BaseStatsObj) && BaseStatsObj)
    {
        double Val = 0;
        if ((*BaseStatsObj)->TryGetNumberField(TEXT("str"), Val)) BaseSTR = (int32)Val;
        if ((*BaseStatsObj)->TryGetNumberField(TEXT("agi"), Val)) BaseAGI = (int32)Val;
        if ((*BaseStatsObj)->TryGetNumberField(TEXT("vit"), Val)) BaseVIT = (int32)Val;
        if ((*BaseStatsObj)->TryGetNumberField(TEXT("int"), Val)) BaseINT = (int32)Val;
        if ((*BaseStatsObj)->TryGetNumberField(TEXT("dex"), Val)) BaseDEX = (int32)Val;
        if ((*BaseStatsObj)->TryGetNumberField(TEXT("luk"), Val)) BaseLUK = (int32)Val;
    }

    // ── Job Bonus Stats (NEW) ──
    const TSharedPtr<FJsonObject>* JobBonusObj = nullptr;
    if (Obj->TryGetObjectField(TEXT("jobBonus"), JobBonusObj) && JobBonusObj)
    {
        double Val = 0;
        if ((*JobBonusObj)->TryGetNumberField(TEXT("str"), Val)) JobBonusSTR = (int32)Val;
        if ((*JobBonusObj)->TryGetNumberField(TEXT("agi"), Val)) JobBonusAGI = (int32)Val;
        if ((*JobBonusObj)->TryGetNumberField(TEXT("vit"), Val)) JobBonusVIT = (int32)Val;
        if ((*JobBonusObj)->TryGetNumberField(TEXT("int"), Val)) JobBonusINT = (int32)Val;
        if ((*JobBonusObj)->TryGetNumberField(TEXT("dex"), Val)) JobBonusDEX = (int32)Val;
        if ((*JobBonusObj)->TryGetNumberField(TEXT("luk"), Val)) JobBonusLUK = (int32)Val;
    }

    // ── Stat costs ──
    const TSharedPtr<FJsonObject>* CostsObj = nullptr;
    if (Obj->TryGetObjectField(TEXT("statCosts"), CostsObj) && CostsObj)
    {
        double Val = 0;
        if ((*CostsObj)->TryGetNumberField(TEXT("str"), Val)) CostSTR = (int32)Val;
        if ((*CostsObj)->TryGetNumberField(TEXT("agi"), Val)) CostAGI = (int32)Val;
        if ((*CostsObj)->TryGetNumberField(TEXT("vit"), Val)) CostVIT = (int32)Val;
        if ((*CostsObj)->TryGetNumberField(TEXT("int"), Val)) CostINT = (int32)Val;
        if ((*CostsObj)->TryGetNumberField(TEXT("dex"), Val)) CostDEX = (int32)Val;
        if ((*CostsObj)->TryGetNumberField(TEXT("luk"), Val)) CostLUK = (int32)Val;
    }

    // ── Derived stats ──
    const TSharedPtr<FJsonObject>* DerivedObj = nullptr;
    if (Obj->TryGetObjectField(TEXT("derived"), DerivedObj) && DerivedObj)
    {
        double Val = 0;
        if ((*DerivedObj)->TryGetNumberField(TEXT("statusATK"), Val)) StatusATK = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("statusMATK"), Val)) StatusMATK = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("matkMin"), Val)) MATKMin = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("matkMax"), Val)) MATKMax = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("hit"), Val)) HIT = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("flee"), Val)) FLEE = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("critical"), Val)) Critical = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("perfectDodge"), Val)) PerfectDodge = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("softDEF"), Val)) SoftDEF = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("softMDEF"), Val)) SoftMDEF = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("hardMDEF"), Val)) HardMDEF = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("aspd"), Val)) ASPD = (int32)Val;
        if ((*DerivedObj)->TryGetNumberField(TEXT("maxWeight"), Val)) MaxWeight = (int32)Val;
    }

    // ── Job info (NEW) ──
    const TSharedPtr<FJsonObject>* JobObj = nullptr;
    if (Obj->TryGetObjectField(TEXT("job"), JobObj) && JobObj)
    {
        FString Str;
        double Val = 0;
        if ((*JobObj)->TryGetStringField(TEXT("jobClass"), Str)) JobClass = Str;
        if ((*JobObj)->TryGetStringField(TEXT("displayName"), Str)) JobDisplayName = Str;
        if ((*JobObj)->TryGetNumberField(TEXT("jobLevel"), Val)) JobLevel = (int32)Val;
        if ((*JobObj)->TryGetNumberField(TEXT("maxJobLevel"), Val)) MaxJobLevel = (int32)Val;
        (*JobObj)->TryGetBoolField(TEXT("isTranscendent"), bIsTranscendent);
    }

    UE_LOG(LogCombatStats, Log,
        TEXT("Stats updated: ATK=%d+%d MATK=%d~%d HIT=%d FLEE=%d CRI=%d PD=%d DEF=%d+%d MDEF=%d+%d ASPD=%d StatPts=%d Job=%s JLv%d"),
        StatusATK, WeaponATK, MATKMin, MATKMax, HIT, FLEE, Critical, PerfectDodge,
        HardDEF, SoftDEF, HardMDEF, SoftMDEF, ASPD, StatPoints,
        *JobDisplayName, JobLevel);
}
```

---

## 5. Client-Side: Stats UI (Slate)

### 5.1 Updated SCombatStatsWidget

The existing `SCombatStatsWidget` already has the full pattern for stat rows, allocatable stat rows, and the gold/dark RO theme. To add the new features, modify the `Construct` method to include:

1. **Job info header** (class name + job level)
2. **MATK as min~max range**
3. **MDEF as HardMDEF + SoftMDEF**
4. **Job bonus display in stat rows** (e.g., "STR 45 +5" where +5 is job bonus in green)

### 5.2 Enhanced Allocatable Stat Row with Job Bonus

Replace the `BuildAllocatableStatRow` to show base + job bonus:

```cpp
TSharedRef<SWidget> SCombatStatsWidget::BuildAllocatableStatRow(
    const FString& StatLabel,
    TAttribute<FText> BaseValue,     // Base stat value (player-allocated)
    TAttribute<FText> BonusValue,    // Job bonus (green text)
    TAttribute<FText> CostText,
    TAttribute<bool> ButtonEnabled,
    const FString& StatName)
{
    return SNew(SHorizontalBox)

        // Label
        + SHorizontalBox::Slot()
        .AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(32.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(StatLabel))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                .ColorAndOpacity(FSlateColor(CombatColors::LabelText))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(CombatColors::TextShadow)
            ]
        ]

        // Base value
        + SHorizontalBox::Slot()
        .AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(26.f)
            [
                SNew(STextBlock)
                .Text(BaseValue)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                .ColorAndOpacity(FSlateColor(CombatColors::ValueText))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(CombatColors::TextShadow)
            ]
        ]

        // Job bonus (green, e.g. "+5")
        + SHorizontalBox::Slot()
        .AutoWidth().VAlign(VAlign_Center)
        .Padding(1, 0, 0, 0)
        [
            SNew(STextBlock)
            .Text(BonusValue)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.85f, 0.3f, 1.0f))) // Green
            .ShadowOffset(FVector2D(1, 1))
            .ShadowColorAndOpacity(CombatColors::TextShadow)
            .Visibility_Lambda([BonusValue]() -> EVisibility {
                // Hide if bonus is "+0"
                FText T = BonusValue.Get();
                return T.ToString() == TEXT("+0") ? EVisibility::Collapsed : EVisibility::Visible;
            })
        ]

        // Cost display
        + SHorizontalBox::Slot()
        .AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
        [
            SNew(SBox).WidthOverride(26.f)
            .Visibility_Lambda([ButtonEnabled]() -> EVisibility {
                return ButtonEnabled.Get() ? EVisibility::Visible : EVisibility::Hidden;
            })
            [
                SNew(STextBlock)
                .Text(CostText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
                .ColorAndOpacity(FSlateColor(CombatColors::CostText))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(CombatColors::TextShadow)
            ]
        ]

        // [+] button
        + SHorizontalBox::Slot()
        .AutoWidth().VAlign(VAlign_Center).Padding(2, 0, 0, 0)
        [
            SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .ContentPadding(FMargin(2.f))
            .Visibility_Lambda([ButtonEnabled]() -> EVisibility {
                return ButtonEnabled.Get() ? EVisibility::Visible : EVisibility::Hidden;
            })
            .OnClicked(FOnClicked::CreateSP(this, &SCombatStatsWidget::OnAllocateStatClicked, StatName))
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("+")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FSlateColor(CombatColors::StatPtsGold))
            ]
        ];
}
```

### 5.3 MATK Display as Range

In `Construct()`, change the MATK row:

```cpp
// BEFORE:
BuildStatRow(
    FText::FromString(TEXT("MATK")),
    TAttribute<FText>::CreateLambda([this]() -> FText {
        if (!Subsystem) return FText::GetEmpty();
        return FText::FromString(FString::FromInt(Subsystem->StatusMATK));
    })
)

// AFTER:
BuildStatRow(
    FText::FromString(TEXT("MATK")),
    TAttribute<FText>::CreateLambda([this]() -> FText {
        if (!Subsystem) return FText::GetEmpty();
        if (Subsystem->MATKMin == Subsystem->MATKMax)
        {
            return FText::FromString(FString::FromInt(Subsystem->MATKMin));
        }
        return FText::FromString(FString::Printf(TEXT("%d ~ %d"),
            Subsystem->MATKMin, Subsystem->MATKMax));
    })
)
```

### 5.4 MDEF as Hard + Soft

```cpp
BuildStatRow(
    FText::FromString(TEXT("MDEF")),
    TAttribute<FText>::CreateLambda([this]() -> FText {
        if (!Subsystem) return FText::GetEmpty();
        return FText::FromString(FString::Printf(TEXT("%d + %d"),
            Subsystem->HardMDEF, Subsystem->SoftMDEF));
    })
)
```

### 5.5 Job Info Title Bar

Add a job info section to the title bar or as a subtitle:

```cpp
TSharedRef<SWidget> SCombatStatsWidget::BuildTitleBar()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(CombatColors::PanelDark)
        .Padding(FMargin(6.f, 3.f))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Status")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FSlateColor(CombatColors::HeaderText))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(CombatColors::TextShadow)
            ]

            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(TAttribute<FText>::CreateLambda([this]() -> FText {
                    if (!Subsystem) return FText::GetEmpty();
                    return FText::FromString(FString::Printf(TEXT("%s  Lv %d / %d"),
                        *Subsystem->JobDisplayName, Subsystem->JobLevel, Subsystem->MaxJobLevel));
                }))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
                .ColorAndOpacity(FSlateColor(CombatColors::LabelText))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(CombatColors::TextShadow)
            ]
        ];
}
```

### 5.6 Stat Row Invocation with Job Bonus

Each stat row in `Construct()` should pass the job bonus:

```cpp
+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
[
    BuildAllocatableStatRow(
        TEXT("STR"),
        // Base value (effective = base + equip)
        TAttribute<FText>::CreateLambda([this]() -> FText {
            if (!Subsystem) return FText::GetEmpty();
            return FText::FromString(FString::FromInt(Subsystem->STR));
        }),
        // Job bonus
        TAttribute<FText>::CreateLambda([this]() -> FText {
            if (!Subsystem) return FText::GetEmpty();
            return FText::FromString(FString::Printf(TEXT("+%d"), Subsystem->JobBonusSTR));
        }),
        // Cost
        TAttribute<FText>::CreateLambda([this]() -> FText {
            if (!Subsystem) return FText::GetEmpty();
            return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostSTR));
        }),
        // Button enabled
        TAttribute<bool>::CreateLambda([this]() -> bool {
            if (!Subsystem) return false;
            return Subsystem->StatPoints >= Subsystem->CostSTR && Subsystem->BaseSTR < 99;
        }),
        TEXT("str")
    )
]
```

Repeat for AGI, VIT, INT, DEX, LUK with respective `JobBonusAGI`, `JobBonusVIT`, etc.

### 5.7 Key Toggle: Alt+A

The stat window toggle is currently F8 (handled in `ASabriMMOCharacter::SetupPlayerInputComponent`). To add Alt+A as an alternative:

```cpp
// In ASabriMMOCharacter::SetupPlayerInputComponent or BP_MMOCharacter's Enhanced Input:
// Bind IA_ToggleStats to Alt+A
// The callback calls:
if (UWorld* World = GetWorld())
{
    if (UCombatStatsSubsystem* CSS = World->GetSubsystem<UCombatStatsSubsystem>())
    {
        CSS->ToggleWidget();
    }
}
```

This uses the existing `IA_ToggleStats` input action (already mapped to F8) -- add a second binding for Alt+A in `IMC_MMOCharacter`.

---

## 6. Level Up Effects, DB Schema, Testing

### 6.1 Level Up VFX

When a level up occurs, the client receives `exp:level_up`. The existing handler in `BasicInfoSubsystem` updates the EXP bars. Add visual effects:

```cpp
// In a new or existing subsystem that handles exp:level_up:
void HandleLevelUp(const TSharedPtr<FJsonValue>& Data)
{
    // Parse baseLevelUps array
    // For each base level up:
    //   1. Play level-up particle effect at player location
    //   2. Play level-up sound
    //   3. Show floating text "Base Level Up! Lv X"
    //   4. Full heal animation (health/mana bars flash)

    // For each job level up:
    //   1. Show floating text "Job Level Up! JLv X"

    // Implementation uses SkillVFXSubsystem pattern:
    // - Find local pawn via GameInstance
    // - Spawn Niagara system at pawn location
    // - Use a bright column-of-light effect (NS_HealAura or similar)
}
```

### 6.2 DB Schema Migration

Create migration file: `database/migrations/add_stat_job_system.sql`

```sql
-- ============================================================
-- Migration: add_stat_job_system.sql
-- Adds transcendent tracking and ensures stat columns exist
-- ============================================================

-- Add transcendent tracking columns
ALTER TABLE characters
    ADD COLUMN IF NOT EXISTS is_transcendent BOOLEAN DEFAULT FALSE,
    ADD COLUMN IF NOT EXISTS rebirth_count INTEGER DEFAULT 0,
    ADD COLUMN IF NOT EXISTS current_weight INTEGER DEFAULT 0;

-- Ensure all stat columns have correct defaults and constraints
DO $$
BEGIN
    -- These columns should already exist but ensure constraints
    ALTER TABLE characters ALTER COLUMN str SET DEFAULT 1;
    ALTER TABLE characters ALTER COLUMN agi SET DEFAULT 1;
    ALTER TABLE characters ALTER COLUMN vit SET DEFAULT 1;
    ALTER TABLE characters ALTER COLUMN int_stat SET DEFAULT 1;
    ALTER TABLE characters ALTER COLUMN dex SET DEFAULT 1;
    ALTER TABLE characters ALTER COLUMN luk SET DEFAULT 1;
    ALTER TABLE characters ALTER COLUMN stat_points SET DEFAULT 48;
    ALTER TABLE characters ALTER COLUMN skill_points SET DEFAULT 0;
EXCEPTION WHEN OTHERS THEN
    -- Columns may not exist yet; the server auto-creates them
    NULL;
END $$;

-- Add check constraints if not present
DO $$
BEGIN
    ALTER TABLE characters ADD CONSTRAINT chk_str CHECK (str BETWEEN 1 AND 99);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$
BEGIN
    ALTER TABLE characters ADD CONSTRAINT chk_agi CHECK (agi BETWEEN 1 AND 99);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$
BEGIN
    ALTER TABLE characters ADD CONSTRAINT chk_vit CHECK (vit BETWEEN 1 AND 99);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$
BEGIN
    ALTER TABLE characters ADD CONSTRAINT chk_int CHECK (int_stat BETWEEN 1 AND 99);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$
BEGIN
    ALTER TABLE characters ADD CONSTRAINT chk_dex CHECK (dex BETWEEN 1 AND 99);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$
BEGIN
    ALTER TABLE characters ADD CONSTRAINT chk_luk CHECK (luk BETWEEN 1 AND 99);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

-- Index for job class lookups
CREATE INDEX IF NOT EXISTS idx_characters_job_class ON characters(job_class);
```

### 6.3 Socket.io Event Reference

| Event | Direction | Payload | Trigger |
|-------|-----------|---------|---------|
| `player:allocate_stat` | Client -> Server | `{ statName: "str" }` | User clicks [+] button |
| `player:request_stats` | Client -> Server | `{}` | Zone change, reconnect |
| `player:stats` | Server -> Client | See `buildFullStatsPayload()` output | Stat change, equip, level up, login |
| `exp:gain` | Server -> Client | `{ characterId, baseExpGained, jobExpGained, enemyName, exp: {...} }` | Monster kill |
| `exp:level_up` | Server -> All in zone | `{ characterId, characterName, baseLevelUps, jobLevelUps, exp: {...} }` | Level up |
| `job:change` | Client -> Server | `{ targetClass: "knight" }` | Job change NPC |
| `job:changed` | Server -> Client + Zone | `{ characterId, oldClass, newClass, ... }` | Successful job change |
| `job:rebirth` | Client -> Server | `{}` | Valkyrie NPC |
| `job:reborn` | Server -> Client | `{ characterId, oldClass, newClass, ... }` | Successful rebirth |
| `job:error` | Server -> Client | `{ message: "..." }` | Invalid job change |
| `combat:error` | Server -> Client | `{ message: "..." }` | Invalid stat allocation |

### 6.4 `player:stats` Payload Structure (Complete)

```json
{
    "characterId": 123,
    "stats": {
        "str": 55, "agi": 40, "vit": 50, "int": 1, "dex": 30, "luk": 10,
        "level": 75, "weaponATK": 120, "hardDef": 45, "statPoints": 42
    },
    "baseStats": {
        "str": 50, "agi": 38, "vit": 48, "int": 1, "dex": 28, "luk": 10
    },
    "jobBonus": {
        "str": 5, "agi": 2, "vit": 2, "int": 0, "dex": 2, "luk": 0
    },
    "statCosts": {
        "str": 6, "agi": 5, "vit": 6, "int": 2, "dex": 4, "luk": 2
    },
    "derived": {
        "statusATK": 92, "weaponATK": 120, "statusMATK": 1, "matkMin": 1, "matkMax": 1,
        "hardDEF": 45, "softDEF": 62, "hardMDEF": 0, "softMDEF": 25,
        "hit": 105, "flee": 115, "perfectDodge": 1, "critical": 4,
        "aspd": 165, "attackDelayMs": 350,
        "maxHP": 5890, "maxSP": 245, "maxWeight": 3450,
        "hpRegen": 39, "spRegen": 3, "castTimeReduction": 0.2
    },
    "job": {
        "jobClass": "knight",
        "displayName": "Knight",
        "jobLevel": 42,
        "maxJobLevel": 50,
        "isTranscendent": false
    },
    "exp": {
        "baseLevel": 75, "jobLevel": 42,
        "baseExp": 1250000, "baseExpNext": 2230113,
        "jobExp": 450000, "jobExpNext": 793535,
        "jobClass": "knight", "jobClassDisplayName": "Knight",
        "maxBaseLevel": 99, "maxJobLevel": 50,
        "statPoints": 42, "skillPoints": 3
    }
}
```

### 6.5 Testing Checklist

**Server-Side Tests** (manual or automated with a test script):

- [ ] `getStatPointCost(1)` returns 2, `getStatPointCost(10)` returns 2, `getStatPointCost(11)` returns 3, `getStatPointCost(91)` returns 11
- [ ] `getStatPointsForLevel(1)` returns 3, `getStatPointsForLevel(5)` returns 4, `getStatPointsForLevel(99)` returns 22
- [ ] `getTotalStatPoints(99, false)` returns 1273
- [ ] `getCumulativeStatCost(99)` returns 628
- [ ] `calculateMaxHP(50, 50, 'swordsman', false, 0, 0)` returns approximately 1767 (per doc example)
- [ ] `calculateMaxHP(99, 80, 'lord_knight', true, 0, 0)` returns approximately 17899
- [ ] `calculateMATK(99)` returns `{ min: 299, max: 491 }`
- [ ] Stat allocation: allocating STR from 1 to 2 costs 2 points
- [ ] Stat allocation: cannot exceed 99
- [ ] Stat allocation: insufficient points returns error
- [ ] Level up: killing a monster that gives enough EXP triggers level up
- [ ] Level up: stat points are correctly awarded
- [ ] Level up: HP/SP are fully restored
- [ ] Job change: Novice at job 10 can become Swordsman
- [ ] Job change: Novice at job 9 CANNOT become Swordsman
- [ ] Job change: Swordsman at job 40+ can become Knight or Crusader
- [ ] Death penalty: 1% of current level EXP needed is deducted
- [ ] Death penalty: EXP cannot go below 0
- [ ] Death penalty: Novice has no penalty
- [ ] Transcendent rebirth: requires Base 99, Job 50, correct zeny

**Client-Side Tests** (manual in PIE):

- [ ] F8 opens/closes the stats window
- [ ] Stats window shows correct base stats from server
- [ ] Job bonus stats appear in green next to base stats
- [ ] [+] button only appears when affordable
- [ ] Clicking [+] sends `player:allocate_stat` and UI updates
- [ ] MATK shows as range (min ~ max) when INT > 1
- [ ] DEF shows as HardDEF + SoftDEF
- [ ] MDEF shows as HardMDEF + SoftMDEF
- [ ] Job info displays class name and job level
- [ ] Stat points remaining updates after allocation
- [ ] Widget is draggable by title bar
- [ ] Widget is resizable by edges
- [ ] Tested with 2 PIE instances: each player sees own stats only
- [ ] Zone transition: stats re-populate after changing zones

### 6.6 File Inventory (What to Create / Modify)

| File | Action | Description |
|------|--------|-------------|
| `server/src/stat_calculator.js` | **CREATE** | Pure stat formula module |
| `server/src/ro_job_bonus.js` | **CREATE** | Job bonus stat tables |
| `server/src/ro_exp_tables.js` | **MODIFY** | Add transcendent EXP tables, transcendent class configs |
| `server/src/index.js` | **MODIFY** | Import new modules, update `calculateDerivedStats()`, add `job:rebirth` handler, add death penalty, update `buildFullStatsPayload()` |
| `server/src/ro_damage_formulas.js` | **NO CHANGE** | Keep existing `calculateDerivedStats` for backward compat; `index.js` wrapper overrides it |
| `database/migrations/add_stat_job_system.sql` | **CREATE** | DB migration for transcendent columns |
| `client/.../UI/CombatStatsSubsystem.h` | **MODIFY** | Add job info, job bonus, MATK range fields |
| `client/.../UI/CombatStatsSubsystem.cpp` | **MODIFY** | Parse new fields from `player:stats` |
| `client/.../UI/SCombatStatsWidget.h` | **MODIFY** | Update `BuildAllocatableStatRow` signature |
| `client/.../UI/SCombatStatsWidget.cpp` | **MODIFY** | Job bonus display, MATK range, MDEF display, job title |

---

## Appendix A: Quick Formula Reference

| Formula | Expression |
|---------|-----------|
| Stat cost | `floor((current - 1) / 10) + 2` |
| Stat pts/level | `floor(level / 5) + 3` |
| StatusATK (melee) | `floor(BLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)` |
| StatusATK (ranged) | `floor(BLv/4) + floor(STR/5) + DEX + floor(DEX/10)^2 + floor(LUK/3)` |
| MATK Min | `INT + floor(INT/7)^2` |
| MATK Max | `INT + floor(INT/5)^2` |
| Soft DEF | `floor(VIT/2) + floor(AGI/5) + floor(BLv/2)` |
| Hard DEF reduction | `Dmg * (4000 + DEF) / (4000 + DEF * 10)` |
| Soft MDEF | `INT + floor(VIT/5) + floor(DEX/5) + floor(BLv/4)` |
| Hard MDEF reduction | `Dmg * (1000 + MDEF) / (1000 + MDEF * 10)` |
| HIT | `BLv + DEX + Bonus` |
| Flee | `BLv + AGI + Bonus` |
| Perfect Dodge | `floor(LUK/10) + Bonus` |
| Critical | `floor(LUK * 0.3) + 1 + Bonus` |
| ASPD | `200 - (WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SM)` |
| Max HP | `floor(BaseHP * (1 + VIT*0.01) * TransMod) + AddMod` |
| Max SP | `floor(BaseSP * (1 + INT*0.01) * TransMod) + AddMod` |
| Cast Time | `BaseCT * (1 - DEX/150)` |
| HP Regen/6s | `max(1, floor(MaxHP/200)) + floor(VIT/5)` |
| SP Regen/8s | `floor(MaxSP/100) + floor(INT/6) + 1` |
| Weight | `2000 + STR*30 + JobBonus` |
| Party EXP | `BaseEXP * (1 + (Size-1) * 0.20) / Size` |
| Death penalty | `floor(EXPToNextLevel * 0.01)` |
