/**
 * ro_damage_formulas.js — Complete Ragnarok Online Pre-Renewal Damage System
 *
 * All formulas sourced from rAthena pre-renewal database and iROWiki.
 * Covers: Physical/Magical damage, HIT/FLEE, Critical, Perfect Dodge,
 *         Element modifiers (10×10×4), Size penalties, DEF/MDEF.
 */

'use strict';

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
// Derived Stat Calculations (Pre-Renewal)
// ============================================================

/**
 * Calculate all derived combat stats from base stats + equipment.
 * @param {Object} stats — { str, agi, vit, int, dex, luk, level, bonusHit, bonusFlee, bonusCritical, bonusMaxHp, bonusMaxSp }
 * @returns {Object} derived stats
 */
function calculateDerivedStats(stats) {
    const {
        str = 1, agi = 1, vit = 1, int: intStat = 1, dex = 1, luk = 1, level = 1,
        bonusHit = 0, bonusFlee = 0, bonusCritical = 0, bonusMaxHp = 0, bonusMaxSp = 0
    } = stats;

    // ── Status ATK (Pre-Renewal) ──
    // STR + floor(STR/10)² + floor(DEX/5) + floor(LUK/3)
    const statusATK = str + Math.floor(str / 10) ** 2 + Math.floor(dex / 5) + Math.floor(luk / 3);

    // ── Status MATK (Pre-Renewal) ──
    // INT + floor(INT/7)²
    const statusMATK = intStat + Math.floor(intStat / 7) ** 2;

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
    // 1 + floor(LUK / 10)
    const perfectDodge = 1 + Math.floor(luk / 10);

    // ── Soft DEF from VIT (Pre-Renewal rAthena formula) ──
    // VIT/2 + max(1, (VIT*2 - 1) / 3)
    const softDEF = Math.floor(vit / 2) + Math.max(1, Math.floor((vit * 2 - 1) / 3));

    // ── Soft MDEF from INT (Pre-Renewal) ──
    // INT + floor(INT/2)... simplified
    const softMDEF = Math.floor(intStat / 2) + Math.max(0, Math.floor((intStat * 2 - 1) / 4));

    // ── ASPD (custom formula — sqrt scaling for smooth gameplay) ──
    const agiContribution = Math.floor(Math.sqrt(agi) * 1.2);
    const dexContribution = Math.floor(Math.sqrt(dex) * 0.6);
    const aspd = Math.min(195, Math.floor(170 + agiContribution + dexContribution));

    // ── Max HP (simplified, class-agnostic) ──
    const maxHP = 100 + vit * 8 + level * 10 + bonusMaxHp;

    // ── Max SP ──
    const maxSP = 50 + intStat * 5 + level * 5 + bonusMaxSp;

    return {
        statusATK, statusMATK, hit, flee, critical, perfectDodge,
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
        const effectiveCrit = calculateCritRate(atkDerived.critical, targetLuk);
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
    const weaponType = attacker.weaponType || 'bare_hand';
    const sizePenaltyPct = getSizePenalty(weaponType, targetSize);
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
    // Hard DEF (equipment): percentage reduction — NOT affected by buff defMultiplier
    //   (RO Classic: Provoke/Signum Crucis only reduce VIT soft DEF)
    //   damage = damage * (100 - hardDEF) / 100
    // Soft DEF (VIT-based): flat subtraction, affected by defMultiplier
    // ─────────────────────────────────────────────────────
    const hardDef = Math.min(99, target.hardDef || 0); // Cap at 99% reduction
    const defMultiplier = (target.buffMods && target.buffMods.defMultiplier) || 1.0;

    // Apply hard DEF (percentage reduction) — unmodified by buffs
    if (hardDef > 0) {
        totalATK = Math.floor(totalATK * (100 - hardDef) / 100);
    }

    // Apply soft DEF (flat reduction) — modified by Provoke/Signum Crucis defMultiplier
    // Angelus: increases VIT-based soft DEF by defPercent%
    const defPercentBonus = (target.buffMods && target.buffMods.defPercent) || 0;
    const angelusMultiplier = defPercentBonus > 0 ? (1 + defPercentBonus / 100) : 1.0;
    const effectiveSoftDef = Math.floor(defDerived.softDEF * defMultiplier * angelusMultiplier);
    totalATK = totalATK - effectiveSoftDef;

    // Passive race DEF bonuses (Divine Protection)
    if (target.passiveRaceDEF && attacker.race) {
        const raceDEFBonus = target.passiveRaceDEF[attacker.race] || 0;
        if (raceDEFBonus > 0) {
            totalATK = Math.max(1, totalATK - raceDEFBonus);
        }
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
        skillElement = 'neutral'
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

    // ── MATK calculation ──
    // statusMATK = INT + floor(INT/7)²
    // MATK varies between min and max:
    //   max = statusMATK + weaponMATK
    //   min = statusMATK * 0.7 + weaponMATK * 0.7
    const weaponMATK = attacker.weaponMATK || 0;
    const matkMax = atkDerived.statusMATK + weaponMATK;
    const matkMin = Math.floor(atkDerived.statusMATK * 0.7) + Math.floor(weaponMATK * 0.7);
    const matk = matkMin + Math.floor(Math.random() * (matkMax - matkMin + 1));

    // ── Apply skill multiplier ──
    let totalDamage = Math.floor(matk * skillMultiplier / 100);

    // ── Buff ATK modifier ──
    const atkMultiplier = (attacker.buffMods && attacker.buffMods.atkMultiplier) || 1.0;
    totalDamage = Math.floor(totalDamage * atkMultiplier);

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
    // Hard MDEF (equipment): percentage reduction — unmodified by buffs
    const hardMdef = Math.min(99, target.hardMdef || target.magicDefense || 0);

    if (hardMdef > 0) {
        totalDamage = Math.floor(totalDamage * (100 - hardMdef) / 100);
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

    // Damage calculations
    calculatePhysicalDamage,
    calculateMagicalDamage
};
