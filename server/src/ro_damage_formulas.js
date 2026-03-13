/**
 * ro_damage_formulas.js — Complete Ragnarok Online Pre-Renewal Damage System
 *
 * All formulas sourced from rAthena pre-renewal database and iROWiki.
 * Covers: Physical/Magical damage, HIT/FLEE, Critical, Perfect Dodge,
 *         Element modifiers (10×10×4), Size penalties, DEF/MDEF.
 */

'use strict';

const {
    HP_SP_COEFFICIENTS,
    ASPD_BASE_DELAYS,
    TRANS_TO_BASE_CLASS,
    TRANSCENDENT_CLASSES,
} = require('./ro_exp_tables');

// ============================================================
// Element Effectiveness Table (Pre-Renewal)
// ELEMENT_TABLE[attackElement][defendElement][defendLevel - 1]
// Values are damage percentage (100 = normal, 0 = immune, negative = heals)
// ============================================================
// Source: rAthena db/pre-re/attr_fix.yml (canonical pre-renewal)
// Cross-referenced with: iRO Wiki Classic, RateMyServer, MuhRO Wiki, Hercules pre-re
// Format: [Lv1, Lv2, Lv3, Lv4] for defense element level
const ELEMENT_TABLE = {
    neutral: {
        neutral: [100, 100, 100, 100],
        water:   [100, 100, 100, 100],
        earth:   [100, 100, 100, 100],
        fire:    [100, 100, 100, 100],
        wind:    [100, 100, 100, 100],
        poison:  [100, 100, 100, 100],
        holy:    [100, 100, 100, 100],
        shadow:  [100, 100, 100, 100],
        ghost:   [ 25,  25,   0,   0],
        undead:  [100, 100, 100, 100]
    },
    water: {
        neutral: [100, 100, 100, 100],
        water:   [ 25,   0, -25, -50],
        earth:   [100, 100, 100, 100],
        fire:    [150, 175, 200, 200],
        wind:    [ 50,  25,   0,   0],
        poison:  [100, 100, 100,  75],
        holy:    [ 75,  50,  25,   0],
        shadow:  [100,  75,  50,  25],
        ghost:   [100, 100, 100, 100],
        undead:  [100, 100, 125, 150]
    },
    earth: {
        neutral: [100, 100, 100, 100],
        water:   [100, 100, 100, 100],
        earth:   [100,  50,   0, -25],
        fire:    [ 50,  25,   0,   0],
        wind:    [150, 175, 200, 200],
        poison:  [100, 100, 100,  75],
        holy:    [ 75,  50,  25,   0],
        shadow:  [100,  75,  50,  25],
        ghost:   [100, 100, 100, 100],
        undead:  [100, 100,  75,  50]
    },
    fire: {
        neutral: [100, 100, 100, 100],
        water:   [ 50,  25,   0,   0],
        earth:   [150, 175, 200, 200],
        fire:    [ 25,   0, -25, -50],
        wind:    [100, 100, 100, 100],
        poison:  [100, 100, 100,  75],
        holy:    [ 75,  50,  25,   0],
        shadow:  [100,  75,  50,  25],
        ghost:   [100, 100, 100, 100],
        undead:  [125, 150, 175, 200]
    },
    wind: {
        neutral: [100, 100, 100, 100],
        water:   [175, 175, 200, 200],
        earth:   [ 50,  25,   0,   0],
        fire:    [100, 100, 100, 100],
        wind:    [ 25,   0, -25, -50],
        poison:  [100, 100, 100,  75],
        holy:    [ 75,  50,  25,   0],
        shadow:  [100,  75,  50,  25],
        ghost:   [100, 100, 100, 100],
        undead:  [100, 100, 100, 100]
    },
    poison: {
        neutral: [100, 100, 100, 100],
        water:   [100,  75,  50,  25],
        earth:   [125, 125, 100,  75],
        fire:    [125, 125, 100,  75],
        wind:    [125, 125, 100,  75],
        poison:  [  0,   0,   0,   0],
        holy:    [ 75,  50,  25,   0],
        shadow:  [ 50,  25,   0, -25],
        ghost:   [100,  75,  50,  25],
        undead:  [-25, -50, -75,-100]
    },
    holy: {
        neutral: [100, 100, 100, 100],
        water:   [100, 100, 100,  75],
        earth:   [100, 100, 100,  75],
        fire:    [100, 100, 100,  75],
        wind:    [100, 100, 100,  75],
        poison:  [100, 100, 125, 125],
        holy:    [  0, -25, -50,-100],
        shadow:  [125, 150, 175, 200],
        ghost:   [100, 100, 100, 100],
        undead:  [150, 175, 200, 200]
    },
    shadow: {
        neutral: [100, 100, 100, 100],
        water:   [100, 100, 100,  75],
        earth:   [100, 100, 100,  75],
        fire:    [100, 100, 100,  75],
        wind:    [100, 100, 100,  75],
        poison:  [ 50,  25,   0, -25],
        holy:    [125, 150, 175, 200],
        shadow:  [  0, -25, -50,-100],
        ghost:   [100, 100, 100, 100],
        undead:  [-25, -50, -75,-100]
    },
    ghost: {
        neutral: [ 25,   0,   0,   0],
        water:   [100,  75,  50,  25],
        earth:   [100,  75,  50,  25],
        fire:    [100,  75,  50,  25],
        wind:    [100,  75,  50,  25],
        poison:  [100,  75,  50,  25],
        holy:    [ 75,  50,  25,   0],
        shadow:  [ 75,  50,  25,   0],
        ghost:   [125, 150, 175, 200],
        undead:  [100, 125, 150, 175]
    },
    undead: {
        neutral: [100, 100, 100, 100],
        water:   [100,  75,  50,  25],
        earth:   [100,  75,  50,  25],
        fire:    [100,  75,  50,  25],
        wind:    [100,  75,  50,  25],
        poison:  [ 50,  25,   0, -25],
        holy:    [100, 125, 150, 175],
        shadow:  [  0,   0,   0,   0],
        ghost:   [100, 100, 100, 100],
        undead:  [  0,   0,   0,   0]
    }
};

// ============================================================
// Weapon Size Penalty Table (Pre-Renewal)
// SIZE_PENALTY[weaponType][targetSize] → percentage (100 = full)
// ============================================================
const SIZE_PENALTY = {
    bare_hand:       { small: 100, medium: 100, large: 100 },
    dagger:          { small: 100, medium:  75, large:  50 },
    one_hand_sword:  { small:  75, medium: 100, large:  75 },
    two_hand_sword:  { small:  75, medium:  75, large: 100 },
    one_hand_spear:  { small:  75, medium:  75, large: 100 },
    two_hand_spear:  { small:  75, medium:  75, large: 100 },
    one_hand_axe:    { small:  50, medium:  75, large: 100 },
    two_hand_axe:    { small:  50, medium:  75, large: 100 },
    mace:            { small:  75, medium: 100, large: 100 },
    rod:             { small: 100, medium: 100, large: 100 },
    staff:           { small: 100, medium: 100, large: 100 },
    bow:             { small: 100, medium: 100, large:  75 },
    katar:           { small:  75, medium: 100, large:  75 },
    book:            { small: 100, medium: 100, large:  50 },
    knuckle:         { small: 100, medium:  75, large:  50 },
    fist:            { small: 100, medium:  75, large:  50 },
    instrument:      { small:  75, medium: 100, large:  75 },
    whip:            { small:  75, medium: 100, large:  50 },
    // Fallback for unrecognized types
    default:         { small: 100, medium: 100, large: 100 }
};

// ============================================================
// Class-Aware MaxHP (RO Pre-Renewal)
// BaseHP = 35 + (BaseLv * HP_JOB_B) + sum(round(HP_JOB_A * i) for i=2..BaseLv)
// MaxHP = floor(BaseHP * (1 + VIT * 0.01) * TransMod) + bonusMaxHp
// ============================================================
function calculateMaxHP(baseLevel, vit, jobClass, isTranscendent, bonusMaxHp) {
    const coeff = HP_SP_COEFFICIENTS[jobClass] || HP_SP_COEFFICIENTS['novice'];
    let baseHP = 35 + Math.floor(baseLevel * coeff.HP_JOB_B);
    for (let i = 2; i <= baseLevel; i++) {
        baseHP += Math.round(coeff.HP_JOB_A * i);
    }
    const transMod = isTranscendent ? 1.25 : 1.0;
    const maxHP = Math.floor(baseHP * (1 + vit * 0.01) * transMod) + (bonusMaxHp || 0);
    return Math.max(1, maxHP);
}

// ============================================================
// Class-Aware MaxSP (RO Pre-Renewal)
// Simple linear formula: BaseSP = 10 + BaseLv * SP_JOB
// MaxSP = floor(BaseSP * (1 + INT * 0.01) * TransMod) + bonusMaxSp
// ============================================================
function calculateMaxSP(baseLevel, intStat, jobClass, isTranscendent, bonusMaxSp) {
    const coeff = HP_SP_COEFFICIENTS[jobClass] || HP_SP_COEFFICIENTS['novice'];
    const baseSP = 10 + Math.floor(baseLevel * coeff.SP_JOB);
    const transMod = isTranscendent ? 1.25 : 1.0;
    const maxSP = Math.floor(baseSP * (1 + intStat * 0.01) * transMod) + (bonusMaxSp || 0);
    return Math.max(1, maxSP);
}

// ============================================================
// Weapon-Type-Aware ASPD (RO Pre-Renewal)
// ASPD = 200 - (WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SpeedMod)
// ============================================================
/**
 * Calculate ASPD for single weapon or dual wield.
 * @param {string} jobClass — player's job class
 * @param {string} weaponType — right-hand weapon type (or single weapon)
 * @param {number} agi — effective AGI stat
 * @param {number} dex — effective DEX stat
 * @param {number} buffAspdMultiplier — buff speed modifier (1.0 = none, 1.3 = +30%)
 * @param {string|null} weaponTypeLeft — left-hand weapon type (for dual wield, null otherwise)
 * @returns {number} ASPD value (100-190 range, capped at 190 for dual wield, 195 for single)
 */
function calculateASPD(jobClass, weaponType, agi, dex, buffAspdMultiplier, weaponTypeLeft) {
    // Resolve transcendent class to base class for ASPD table lookup
    const baseClass = TRANS_TO_BASE_CLASS[jobClass] || jobClass;
    const classDelays = ASPD_BASE_DELAYS[baseClass] || ASPD_BASE_DELAYS['novice'];
    const wt = weaponType || 'bare_hand';
    const baseDelay = classDelays[wt] || classDelays['bare_hand'] || 50;

    let WD;
    if (weaponTypeLeft) {
        // Dual wield ASPD: combined weapon delay formula
        // WD_dual = floor((WD_right + WD_left) * 7 / 10)
        const leftDelay = classDelays[weaponTypeLeft] || classDelays['bare_hand'] || 50;
        WD = Math.floor((baseDelay + leftDelay) * 7 / 10);
    } else {
        WD = baseDelay;
    }

    const agiReduction = Math.floor(WD * agi / 25);
    const dexReduction = Math.floor(WD * dex / 100);
    const totalReduction = Math.floor((agiReduction + dexReduction) / 10);

    // Speed modifier from buffs (e.g., Two-Hand Quicken +0.30, Adrenaline Rush +0.30)
    const speedMod = Math.max(0, (buffAspdMultiplier || 1) - 1); // 1.3 → 0.3

    const rawASPD = 200 - Math.floor((WD - totalReduction) * (1 - speedMod));
    // Dual wield cap is 190 (harder to reach max); single weapon cap is 195
    const aspdCap = weaponTypeLeft ? 190 : 195;
    return Math.min(aspdCap, Math.max(100, rawASPD));
}

// ============================================================
// Derived Stat Calculations (Pre-Renewal)
// ============================================================

/**
 * Calculate all derived combat stats from base stats + equipment.
 * @param {Object} stats — all effective stats + equipment fields
 * @returns {Object} derived stats
 */
function calculateDerivedStats(stats) {
    const {
        str = 1, agi = 1, vit = 1, int: intStat = 1, dex = 1, luk = 1, level = 1,
        bonusHit = 0, bonusFlee = 0, bonusCritical = 0, bonusPerfectDodge = 0,
        bonusMaxHp = 0, bonusMaxSp = 0, bonusMaxHpRate = 0, bonusMaxSpRate = 0,
        bonusHardDef = 0, bonusMATK = 0, bonusHardMDEF = 0,
        // Class/weapon fields for HP/SP/ASPD formulas
        jobClass = 'novice',
        weaponType = 'bare_hand',
        weaponMATK = 0,
        buffAspdMultiplier = 1,
        equipAspdRate = 0,
        // Dual wield (Assassin/Assassin Cross)
        weaponTypeLeft = null,
    } = stats;

    // ── Status ATK (Pre-Renewal) ──
    // STR + floor(STR/10)² + floor(DEX/5) + floor(LUK/3)
    const statusATK = str + Math.floor(str / 10) ** 2 + Math.floor(dex / 5) + Math.floor(luk / 3);

    // ── Status MATK Min/Max (Pre-Renewal) ──
    // Min = INT + floor(INT/7)²   Max = INT + floor(INT/5)²
    const statusMATK = intStat + Math.floor(intStat / 7) ** 2;
    const statusMATKMax = intStat + Math.floor(intStat / 5) ** 2;
    const matkMin = statusMATK + Math.floor(weaponMATK * 0.7);
    const matkMax = statusMATKMax + weaponMATK;

    // ── HIT (Pre-Renewal) ──
    // 175 + BaseLv + DEX + bonuses
    const hit = 175 + level + dex + bonusHit;

    // ── FLEE (Pre-Renewal) ──
    // 100 + BaseLv + AGI + bonuses
    const flee = 100 + level + agi + bonusFlee;

    // ── Critical Rate (Pre-Renewal) ──
    // 1 + floor(LUK * 0.3) + equipment bonus
    const critical = 1 + Math.floor(luk * 0.3) + bonusCritical;

    // ── Perfect Dodge (Pre-Renewal) ──
    // 1 + floor(LUK / 10) + equipment bonus (bFlee2)
    const perfectDodge = 1 + Math.floor(luk / 10) + bonusPerfectDodge;

    // ── Soft DEF (Pre-Renewal) ──
    // floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)
    const softDEF = Math.floor(vit / 2) + Math.floor(agi / 5) + Math.floor(level / 2);

    // ── Soft MDEF (Pre-Renewal) ──
    // INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)
    const softMDEF = intStat + Math.floor(vit / 5) + Math.floor(dex / 5) + Math.floor(level / 4);

    // ── ASPD — weapon-type-aware (RO pre-renewal), dual wield aware ──
    // Combine buff ASPD multiplier with equipment ASPD rate (e.g., Muramasa +8%)
    const totalAspdMultiplier = (buffAspdMultiplier || 1) + (equipAspdRate / 100);
    const aspd = calculateASPD(jobClass, weaponType, agi, dex, totalAspdMultiplier, weaponTypeLeft);

    // ── MaxHP — class-aware (RO pre-renewal) ──
    const isTranscendent = TRANSCENDENT_CLASSES.has(jobClass);
    let maxHP = calculateMaxHP(level, vit, jobClass, isTranscendent, bonusMaxHp);
    if (bonusMaxHpRate !== 0) maxHP = Math.floor(maxHP * (100 + bonusMaxHpRate) / 100);

    // ── MaxSP — class-aware (RO pre-renewal) ──
    let maxSP = calculateMaxSP(level, intStat, jobClass, isTranscendent, bonusMaxSp);
    if (bonusMaxSpRate !== 0) maxSP = Math.floor(maxSP * (100 + bonusMaxSpRate) / 100);

    return {
        statusATK,
        statusMATK,     // kept for backward compat (= matkMin base component)
        matkMin,
        matkMax,
        hit, flee, critical, perfectDodge,
        softDEF, softMDEF, aspd, maxHP, maxSP
    };
}

// ============================================================
// Element Modifier Lookup
// ============================================================

/**
 * Get element effectiveness percentage.
 * @param {string} atkElement — attacker's weapon/skill element
 * @param {string} defElement — target's armor/body element
 * @param {number} defElementLevel — target's element level (1-4)
 * @returns {number} percentage (100 = normal, 0 = immune, negative = heals)
 */
function getElementModifier(atkElement, defElement, defElementLevel = 1) {
    const atkRow = ELEMENT_TABLE[atkElement] || ELEMENT_TABLE.neutral;
    const defCol = atkRow[defElement] || atkRow.neutral;
    const lvIdx = Math.max(0, Math.min(3, defElementLevel - 1));
    return defCol[lvIdx];
}

// ============================================================
// Size Penalty Lookup
// ============================================================

/**
 * Get weapon size penalty percentage.
 * @param {string} weaponType — weapon type key
 * @param {string} targetSize — 'small', 'medium', or 'large'
 * @returns {number} percentage (100 = full damage)
 */
function getSizePenalty(weaponType, targetSize) {
    const entry = SIZE_PENALTY[weaponType] || SIZE_PENALTY.default;
    return entry[targetSize] || 100;
}

// ============================================================
// Hit/Miss Calculation (Pre-Renewal)
// ============================================================

/**
 * Calculate hit rate percentage.
 * @param {number} attackerHit — attacker's HIT stat
 * @param {number} targetFlee — target's FLEE stat
 * @param {number} numAttackers — number of entities attacking the target (for FLEE penalty)
 * @returns {number} hit chance percentage (clamped 5-95)
 */
function calculateHitRate(attackerHit, targetFlee, numAttackers = 1) {
    // Multiple attacker FLEE penalty: -10% FLEE per attacker beyond 2
    let effectiveFlee = targetFlee;
    if (numAttackers > 2) {
        const fleePenalty = (numAttackers - 2) * 10;
        effectiveFlee = Math.max(0, effectiveFlee - fleePenalty);
    }

    const hitRate = 80 + attackerHit - effectiveFlee;
    return Math.max(5, Math.min(95, hitRate));
}

// ============================================================
// Critical Rate Calculation (Pre-Renewal)
// ============================================================

/**
 * Calculate effective critical rate.
 * @param {number} attackerCri — attacker's critical stat
 * @param {number} targetLuk — target's LUK stat
 * @returns {number} effective crit rate (minimum 0)
 */
function calculateCritRate(attackerCri, targetLuk) {
    // Target's critical shield = floor(LUK * 0.2)
    const critShield = Math.floor(targetLuk * 0.2);
    return Math.max(0, attackerCri - critShield);
}

// ============================================================
// Physical Damage Calculation (Pre-Renewal, Complete)
// ============================================================

/**
 * Calculate physical auto-attack or skill damage with full RO mechanics.
 *
 * @param {Object} attacker — { stats (effective), weaponATK, passiveATK, weaponType, weaponElement, weaponLevel, buffMods, cardMods }
 * @param {Object} target — { stats (effective/enemy), hardDef, element: { type, level }, size, race, numAttackers, buffMods }
 * @param {Object} options — { isSkill, skillMultiplier, skillHitBonus, forceHit, forceCrit, skillElement }
 * @returns {Object} { damage, hitType, isCritical, isMiss, element, sizePenalty, elementModifier }
 */
function calculatePhysicalDamage(attacker, target, options = {}) {
    const {
        isSkill = false,
        skillMultiplier = 100,
        skillHitBonus = 0,
        skillName = null,
        forceHit = false,
        forceCrit = false,
        skillElement = null
    } = options;

    const atkDerived = calculateDerivedStats(attacker.stats || attacker);
    const defDerived = calculateDerivedStats(target.stats || target);

    const result = {
        damage: 0,
        hitType: 'normal',
        isCritical: false,
        isMiss: false,
        element: attacker.weaponElement || 'neutral',
        sizePenalty: 100,
        elementModifier: 100
    };

    // Use skill element if specified, otherwise weapon element
    const atkElement = skillElement || attacker.weaponElement || 'neutral';
    result.element = atkElement;

    // ── Target element info ──
    const targetElement = (target.element && target.element.type) ? target.element.type : 'neutral';
    const targetElementLevel = (target.element && target.element.level) ? target.element.level : 1;

    // ── Target race/size for card modifiers ──
    const targetSize = target.size || 'medium';
    const targetRace = target.race || 'formless';

    // ─────────────────────────────────────────────────────
    // Step 1: Perfect Dodge check
    // PD = 1 + floor(LUK/10). Each PD = 1% chance.
    // Critical attacks bypass Perfect Dodge.
    // ─────────────────────────────────────────────────────
    if (!forceHit && !forceCrit) {
        const pd = defDerived.perfectDodge;
        if (Math.random() * 100 < pd) {
            result.hitType = 'perfectDodge';
            result.isMiss = true;
            return result;
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 2: Critical check
    // CRI = 1 + floor(LUK*0.3) + bonusCRI
    // CritShield = floor(targetLUK*0.2)
    // Critical → skip FLEE check, always max ATK
    // ─────────────────────────────────────────────────────
    let isCritical = false;
    if (forceCrit) {
        isCritical = true;
    } else {
        const targetLuk = (target.stats || target).luk || 1;
        // Phase 1: Card crit race bonus (bCriticalAddRace)
        let extraCrit = 0;
        if (attacker.cardCritRace) {
            extraCrit += attacker.cardCritRace[targetRace] || 0;
        }
        // Phase 1: Card ranged crit bonus (bCriticalLong)
        if (attacker.cardCriticalLong) {
            const wType = attacker.weaponType || 'bare_hand';
            if (wType === 'bow' || wType === 'gun') extraCrit += attacker.cardCriticalLong;
        }
        const effectiveCrit = calculateCritRate(atkDerived.critical + extraCrit, targetLuk);
        if (Math.random() * 100 < effectiveCrit) {
            isCritical = true;
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 3: Hit/Miss check (skipped for criticals)
    // HIT = 175 + BaseLv + DEX + bonuses
    // FLEE = 100 + BaseLv + AGI + bonuses
    // HitRate = 80 + HIT - FLEE, clamped 5-95%
    // Status effects (Blind) apply multiplicative HIT/FLEE modifiers
    // ─────────────────────────────────────────────────────
    if (!isCritical && !forceHit) {
        const atkHitMul = (attacker.buffMods && attacker.buffMods.hitMultiplier) || 1.0;
        const defFleeMul = (target.buffMods && target.buffMods.fleeMultiplier) || 1.0;
        const effectiveHit = Math.floor((atkDerived.hit + skillHitBonus) * atkHitMul);
        const effectiveFlee = Math.floor(defDerived.flee * defFleeMul);
        const numAttackers = target.numAttackers || 1;
        const hitRate = calculateHitRate(effectiveHit, effectiveFlee, numAttackers);

        if (Math.random() * 100 >= hitRate) {
            result.hitType = 'miss';
            result.isMiss = true;
            return result;
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 4: Calculate ATK
    // StatusATK = STR + floor(STR/10)² + floor(DEX/5) + floor(LUK/3)
    // WeaponATK from equipment
    // PassiveATK from skills (Sword Mastery, etc.)
    // ─────────────────────────────────────────────────────
    const statusATK = atkDerived.statusATK;
    const weaponATK = attacker.weaponATK || 0;
    const passiveATK = attacker.passiveATK || 0;

    // ── Size penalty (applies to weapon ATK portion only) ──
    // Phase 3: Drake Card (bNoSizeFix) nullifies size penalty
    const weaponType = attacker.weaponType || 'bare_hand';
    const sizePenaltyPct = attacker.cardNoSizeFix ? 100 : getSizePenalty(weaponType, targetSize);
    result.sizePenalty = sizePenaltyPct;

    // ── Weapon ATK with size penalty ──
    const sizedWeaponATK = Math.floor(weaponATK * sizePenaltyPct / 100);

    // ── Damage variance (weapon level affects range) ──
    // Critical: always max ATK (no variance)
    // Normal: random between (1 - weaponLevel * 0.05) and 1.0
    const weaponLevel = attacker.weaponLevel || 1;
    let variancedWeaponATK;
    if (isCritical) {
        variancedWeaponATK = sizedWeaponATK; // Max ATK
    } else {
        const varianceMin = 1.0 - weaponLevel * 0.05; // L1=0.95, L2=0.90, L3=0.85, L4=0.80
        const varianceMul = varianceMin + Math.random() * (1.0 - varianceMin);
        variancedWeaponATK = Math.floor(sizedWeaponATK * varianceMul);
    }

    // ── Total base ATK ──
    let totalATK = statusATK + variancedWeaponATK + passiveATK;

    // ── Critical damage bonus (+40% base, plus equipment bCritAtkRate) ──
    if (isCritical) {
        const baseCritBonus = 40; // RO pre-renewal: +40% on critical hits
        const equipCritAtkRate = attacker.critAtkRate || 0; // Equipment bCritAtkRate bonus
        totalATK = Math.floor(totalATK * (100 + baseCritBonus + equipCritAtkRate) / 100);
    }

    // ── Buff ATK modifier (Provoke etc.) ──
    const atkMultiplier = (attacker.buffMods && attacker.buffMods.atkMultiplier) || 1.0;
    totalATK = Math.floor(totalATK * atkMultiplier);

    // ─────────────────────────────────────────────────────
    // Step 5: Skill multiplier
    // ─────────────────────────────────────────────────────
    if (isSkill && skillMultiplier !== 100) {
        totalATK = Math.floor(totalATK * skillMultiplier / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 5b: Card bSkillAtk bonus (per-skill damage %)
    // ─────────────────────────────────────────────────────
    if (isSkill && skillName && attacker.cardSkillAtk) {
        const skillBonus = attacker.cardSkillAtk[skillName] || 0;
        if (skillBonus !== 0) totalATK = Math.floor(totalATK * (100 + skillBonus) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 6: Card modifiers (race%, element%, size%)
    // Within each category: additive (2x Hydra = 40% race bonus)
    // Between categories: multiplicative (race * ele * size)
    // rAthena: cardfix = (1+race/100) * (1+ele/100) * (1+size/100)
    // ─────────────────────────────────────────────────────
    if (attacker.cardMods) {
        const raceBonus = attacker.cardMods[`race_${targetRace}`] || 0;
        const eleBonus = attacker.cardMods[`ele_${targetElement}`] || 0;
        const sizeBonus = attacker.cardMods[`size_${targetSize}`] || 0;
        if (raceBonus !== 0) totalATK = Math.floor(totalATK * (100 + raceBonus) / 100);
        if (eleBonus !== 0) totalATK = Math.floor(totalATK * (100 + eleBonus) / 100);
        if (sizeBonus !== 0) totalATK = Math.floor(totalATK * (100 + sizeBonus) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 6b: Passive race ATK bonuses (Demon Bane, etc.)
    // ─────────────────────────────────────────────────────
    if (attacker.passiveRaceATK) {
        const raceBonus = attacker.passiveRaceATK[targetRace] || 0;
        if (raceBonus > 0) {
            totalATK += raceBonus;
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 6c: Card boss/normal class modifier (Abysmal Knight, Turtle General, etc.)
    // ─────────────────────────────────────────────────────
    if (attacker.cardAddClass) {
        const targetClass = (target.modeFlags && target.modeFlags.isBoss) ? 'boss' : 'normal';
        const classBonus = attacker.cardAddClass[targetClass] || 0;
        if (classBonus !== 0) totalATK = Math.floor(totalATK * (100 + classBonus) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 6c2: Card sub-race modifier (Goblin Leader, Orc Lady, etc.)
    // ─────────────────────────────────────────────────────
    if (attacker.cardAddRace2 && target.subRace) {
        const subRaceBonus = attacker.cardAddRace2[target.subRace] || 0;
        if (subRaceBonus !== 0) totalATK = Math.floor(totalATK * (100 + subRaceBonus) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 6d: Card damage vs specific monster (Crab Card, Aster Card, etc.)
    // ─────────────────────────────────────────────────────
    if (attacker.cardAddDamageClass && target.templateId) {
        const monsterBonus = attacker.cardAddDamageClass[target.templateId] || 0;
        if (monsterBonus !== 0) totalATK = Math.floor(totalATK * (100 + monsterBonus) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 6e: Ranged ATK bonus (Archer Skeleton Card)
    // ─────────────────────────────────────────────────────
    if (attacker.cardLongAtkRate && attacker.cardLongAtkRate !== 0) {
        const isRanged = weaponType === 'bow' || weaponType === 'gun' || weaponType === 'instrument' || weaponType === 'whip';
        if (isRanged) {
            totalATK = Math.floor(totalATK * (100 + attacker.cardLongAtkRate) / 100);
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 7: Element modifier
    // ─────────────────────────────────────────────────────
    const eleModifier = getElementModifier(atkElement, targetElement, targetElementLevel);
    result.elementModifier = eleModifier;

    if (eleModifier <= 0) {
        // 0% = immune, negative = would heal target (treat as immune for damage)
        result.damage = 0;
        result.hitType = eleModifier < 0 ? 'elementHeal' : 'elementImmune';
        result.isMiss = true;
        return result;
    }
    totalATK = Math.floor(totalATK * eleModifier / 100);

    // ─────────────────────────────────────────────────────
    // Step 8: DEF reduction
    // System D: bIgnoreDefClass (Samurai Spector) skips both hard+soft DEF
    // System D: bDefRatioAtkClass (Thanatos/Ice Pick) converts DEF into bonus damage
    // ─────────────────────────────────────────────────────
    const rawHardDef = Math.min(99, target.hardDef || 0);
    const defMultiplier = (target.buffMods && target.buffMods.defMultiplier) || 1.0;
    const defPercentBonus = (target.buffMods && target.buffMods.defPercent) || 0;
    const angelusMultiplier = defPercentBonus > 0 ? (1 + defPercentBonus / 100) : 1.0;
    const effectiveSoftDef = Math.floor(defDerived.softDEF * angelusMultiplier);

    // Check DEF bypass cards
    let skipDEF = false;
    const targetMonsterClass = (target.modeFlags && target.modeFlags.isBoss) ? 'boss' : 'normal';

    // bIgnoreDefClass: skip both hard DEF and soft DEF entirely
    if (attacker.cardIgnoreDefClass) {
        if (attacker.cardIgnoreDefClass === targetMonsterClass || attacker.cardIgnoreDefClass === 'all') {
            skipDEF = true;
        }
    }

    // bDefRatioAtkClass: DEF becomes bonus damage instead of reduction
    // Formula: ATK * (hardDEF + softDEF) / 100 — replaces normal DEF step
    if (!skipDEF && attacker.cardDefRatioAtkClass) {
        if (attacker.cardDefRatioAtkClass === targetMonsterClass || attacker.cardDefRatioAtkClass === 'all') {
            const combinedDEF = rawHardDef + effectiveSoftDef;
            totalATK = Math.floor(totalATK * combinedDEF / 100);
            skipDEF = true;
        }
    }

    if (!skipDEF) {
        // Normal DEF reduction path
        const hardDef = Math.floor(rawHardDef * defMultiplier);
        if (hardDef > 0) {
            totalATK = Math.floor(totalATK * (100 - hardDef) / 100);
        }
        totalATK = totalATK - effectiveSoftDef;
    }

    // Passive race DEF bonuses (Divine Protection)
    if (target.passiveRaceDEF && attacker.race) {
        const raceDEFBonus = target.passiveRaceDEF[attacker.race] || 0;
        if (raceDEFBonus > 0) {
            totalATK = Math.max(1, totalATK - raceDEFBonus);
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 8c: Defensive card modifiers (Thara Frog, Raydric, etc.)
    // Percentage reduction from target's compounded cards
    // Applied as: damage * (100 - reduction%) / 100
    // ─────────────────────────────────────────────────────
    if (target.cardDefMods) {
        const raceRed = target.cardDefMods[`race_${attacker.race || 'formless'}`] || 0;
        const eleRed = target.cardDefMods[`ele_${atkElement}`] || 0;
        const sizeRed = target.cardDefMods[`size_${attacker.size || 'medium'}`] || 0;
        if (raceRed !== 0) totalATK = Math.floor(totalATK * (100 - raceRed) / 100);
        if (eleRed !== 0) totalATK = Math.floor(totalATK * (100 - eleRed) / 100);
        if (sizeRed !== 0) totalATK = Math.floor(totalATK * (100 - sizeRed) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 8d: Defensive boss/normal class card (Alice Card)
    // ─────────────────────────────────────────────────────
    if (target.cardSubClass) {
        const atkClass = (attacker.modeFlags && attacker.modeFlags.isBoss) ? 'boss' : 'normal';
        const classRed = target.cardSubClass[atkClass] || 0;
        if (classRed !== 0) totalATK = Math.floor(totalATK * (100 - classRed) / 100);
    }

    // ─────────────────────────────────────────────────────
    // Step 8e: Ranged damage reduction (Horn Card, Alligator Card, etc.)
    // ─────────────────────────────────────────────────────
    if (target.cardLongAtkDef && target.cardLongAtkDef > 0) {
        const atkWeaponType = attacker.weaponType || 'bare_hand';
        const isRanged = atkWeaponType === 'bow' || atkWeaponType === 'gun' || atkWeaponType === 'instrument' || atkWeaponType === 'whip';
        if (isRanged) {
            totalATK = Math.floor(totalATK * (100 - target.cardLongAtkDef) / 100);
        }
    }

    // ─────────────────────────────────────────────────────
    // Step 8f: Monster-specific DEF card (Bongun, Mi Gao cards)
    // ─────────────────────────────────────────────────────
    if (target.cardAddDefMonster && attacker.templateId) {
        const monsterDef = target.cardAddDefMonster[attacker.templateId] || 0;
        if (monsterDef !== 0) totalATK = totalATK - monsterDef;
    }

    // ─────────────────────────────────────────────────────
    // Step 9: Final result
    // ─────────────────────────────────────────────────────
    result.damage = Math.max(1, totalATK);
    result.isCritical = isCritical;
    result.hitType = isCritical ? 'critical' : 'normal';

    return result;
}

// ============================================================
// Magical Damage Calculation (Pre-Renewal)
// ============================================================

/**
 * Calculate magical damage.
 *
 * @param {Object} attacker — { stats (effective), weaponMATK, buffMods }
 * @param {Object} target — { stats (effective/enemy), hardMdef, element: { type, level }, buffMods }
 * @param {Object} options — { skillMultiplier, skillElement }
 * @returns {Object} { damage, hitType, element, elementModifier }
 */
function calculateMagicalDamage(attacker, target, options = {}) {
    const {
        skillMultiplier = 100,
        skillElement = 'neutral',
        skillName = null
    } = options;

    const atkDerived = calculateDerivedStats(attacker.stats || attacker);
    const defDerived = calculateDerivedStats(target.stats || target);

    const result = {
        damage: 0,
        hitType: 'magical',
        isCritical: false,
        isMiss: false,
        element: skillElement,
        elementModifier: 100
    };

    // ── Phase 3: GTB Card — magic immunity (bNoMagicDamage) ──
    if (target.cardNoMagicDamage && target.cardNoMagicDamage >= 100) {
        result.damage = 0;
        result.hitType = 'magicImmune';
        result.isMiss = true;
        return result;
    }

    // ── MATK calculation ──
    const matkMin = atkDerived.matkMin || atkDerived.statusMATK;
    const matkMax = atkDerived.matkMax || atkDerived.statusMATK;
    const matk = matkMin + Math.floor(Math.random() * (matkMax - matkMin + 1));

    // ── Apply skill multiplier ──
    let totalDamage = Math.floor(matk * skillMultiplier / 100);

    // ── Phase 1: Card MATK rate bonus (bMatkRate) ──
    if (attacker.cardMatkRate && attacker.cardMatkRate !== 0) {
        totalDamage = Math.floor(totalDamage * (100 + attacker.cardMatkRate) / 100);
    }

    // ── Buff ATK modifier ──
    const atkMultiplier = (attacker.buffMods && attacker.buffMods.atkMultiplier) || 1.0;
    totalDamage = Math.floor(totalDamage * atkMultiplier);

    // ── Phase 1: Card bSkillAtk — per-skill damage bonus ──
    if (skillName && attacker.cardSkillAtk) {
        const skillBonus = attacker.cardSkillAtk[skillName] || 0;
        if (skillBonus !== 0) totalDamage = Math.floor(totalDamage * (100 + skillBonus) / 100);
    }

    // ── Phase 1: Card magic race bonus (bMagicAddRace) ──
    const targetRace = target.race || 'formless';
    if (attacker.cardMagicRace) {
        const magicRaceBonus = attacker.cardMagicRace[targetRace] || 0;
        if (magicRaceBonus !== 0) totalDamage = Math.floor(totalDamage * (100 + magicRaceBonus) / 100);
    }

    // ── Element modifier ──
    const targetElement = (target.element && target.element.type) ? target.element.type : 'neutral';
    const targetElementLevel = (target.element && target.element.level) ? target.element.level : 1;
    const eleModifier = getElementModifier(skillElement, targetElement, targetElementLevel);
    result.elementModifier = eleModifier;

    if (eleModifier <= 0) {
        result.damage = 0;
        result.hitType = eleModifier < 0 ? 'elementHeal' : 'elementImmune';
        result.isMiss = true;
        return result;
    }
    totalDamage = Math.floor(totalDamage * eleModifier / 100);

    // ── MDEF reduction ──
    // Phase 1: Card ignore MDEF (Vesper, High Wizard Card)
    let effectiveHardMdef = Math.min(99, target.hardMdef || target.magicDefense || 0);
    if (attacker.cardIgnoreMdefClass) {
        const targetClass = (target.modeFlags && target.modeFlags.isBoss) ? 'boss' : 'normal';
        const ignorePercent = attacker.cardIgnoreMdefClass[targetClass] || 0;
        if (ignorePercent > 0) {
            effectiveHardMdef = Math.floor(effectiveHardMdef * (100 - Math.min(100, ignorePercent)) / 100);
        }
    }

    if (effectiveHardMdef > 0) {
        totalDamage = Math.floor(totalDamage * (100 - effectiveHardMdef) / 100);
    }

    // Soft MDEF (INT-based): flat subtraction
    const effectiveSoftMdef = defDerived.softMDEF;
    totalDamage = totalDamage - effectiveSoftMdef;

    // ── Buff MDEF bonus (Endure etc.) ──
    const buffBonusMDEF = (target.buffMods && target.buffMods.bonusMDEF) || 0;
    totalDamage = totalDamage - buffBonusMDEF;

    // ── Status MDEF multiplier (freeze/stone = 1.25 → take 125% magic damage) ──
    // Applied as a final damage multiplier, not an MDEF modifier
    const mdefMultiplier = (target.buffMods && target.buffMods.mdefMultiplier) || 1.0;
    if (mdefMultiplier !== 1.0) {
        totalDamage = Math.floor(totalDamage * mdefMultiplier);
    }

    result.damage = Math.max(1, totalDamage);
    return result;
}

// ============================================================
// Exports
// ============================================================
module.exports = {
    // Tables
    ELEMENT_TABLE,
    SIZE_PENALTY,

    // Core functions
    calculateDerivedStats,
    getElementModifier,
    getSizePenalty,
    calculateHitRate,
    calculateCritRate,

    // Class-aware stat helpers
    calculateMaxHP,
    calculateMaxSP,
    calculateASPD,

    // Damage calculations
    calculatePhysicalDamage,
    calculateMagicalDamage
};
