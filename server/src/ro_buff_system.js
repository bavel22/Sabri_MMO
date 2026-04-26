/**
 * ro_buff_system.js — Generic Buff System for Sabri_MMO
 *
 * Handles positive buffs (Blessing, Increase AGI, Endure, Sight, etc.)
 * and debuffs (Provoke) that modify stats via the activeBuffs array.
 *
 * Distinction from ro_status_effects.js:
 *   - Status effects: 10 core CC/DoT conditions (stun, freeze, poison, etc.)
 *     stored in target.activeStatusEffects Map
 *   - Buffs: Stat-modifying skills with duration stored in target.activeBuffs array
 *
 * Usage in index.js:
 *   const { applyBuff, expireBuffs, getBuffModifiers, ... } = require('./ro_buff_system');
 */

// ============================================================================
// BUFF TYPE DEFINITIONS
// ============================================================================

const BUFF_TYPES = {
    // -- Existing buffs (already in game) --
    provoke: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Provoke',
        abbrev: 'PRV'
    },
    endure: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Endure',
        abbrev: 'END'
    },
    sight: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Sight',
        abbrev: 'SGT'
    },

    // -- Future buffs (Phase 3: Acolyte/support skills) --
    blessing: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Blessing',
        abbrev: 'BLS'
    },
    blessing_debuff: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Blessing (Curse)',
        abbrev: 'BCR'
    },
    increase_agi: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Increase AGI',
        abbrev: 'AGI'
    },
    decrease_agi: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Decrease AGI',
        abbrev: 'DAG'
    },
    angelus: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Angelus',
        abbrev: 'ANG'
    },
    pneuma: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Pneuma',
        abbrev: 'PNE'
    },
    signum_crucis: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Signum Crucis',
        abbrev: 'SCR'
    },

    // -- Future buffs (Phase 3: other classes) --
    auto_berserk: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Auto Berserk',
        abbrev: 'BRK'
    },
    hiding: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Hiding',
        abbrev: 'HID'
    },
    play_dead: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Play Dead',
        abbrev: 'PDd'
    },
    improve_concentration: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Concentration',
        abbrev: 'CON'
    },

    // -- Future buffs (Phase 6: second classes) --
    two_hand_quicken: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Two-Hand Quicken',
        abbrev: 'THQ'
    },
    kyrie_eleison: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Kyrie Eleison',
        abbrev: 'KYR'
    },
    magnificat: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Magnificat',
        abbrev: 'MAG'
    },
    gloria: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Gloria',
        abbrev: 'GLO'
    },
    lex_aeterna: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Lex Aeterna',
        abbrev: 'LEX'
    },
    aspersio: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Aspersio',
        abbrev: 'ASP'
    },
    energy_coat: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Energy Coat',
        abbrev: 'ENC'
    },
    enchant_poison: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Enchant Poison',
        abbrev: 'EPO'
    },
    cloaking: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Cloaking',
        abbrev: 'CLK'
    },
    poison_react: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Poison React',
        abbrev: 'PRC'
    },
    quagmire: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Quagmire',
        abbrev: 'QUA'
    },
    loud_exclamation: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Loud Exclamation',
        abbrev: 'LXC'
    },
    ruwach: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Ruwach',
        abbrev: 'RUW'
    },
    magnum_break_fire: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Magnum Break Fire',
        abbrev: 'MBF'
    },

    // -- Priest buffs (Phase 6: Priest skills) --
    impositio_manus: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Impositio Manus',
        abbrev: 'IMP'
    },
    suffragium: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Suffragium',
        abbrev: 'SUF'
    },
    slow_poison: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Slow Poison',
        abbrev: 'SLP'
    },
    bs_sacramenti: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'B.S. Sacramenti',
        abbrev: 'BSS'
    },
    auto_counter: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Auto Counter',
        abbrev: 'ACN'
    },

    // -- Crusader buffs (Phase 2A) --
    auto_guard: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Auto Guard',
        abbrev: 'AGD'
    },
    reflect_shield: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Reflect Shield',
        abbrev: 'RFS'
    },
    defender: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Defender',
        abbrev: 'DEF'
    },
    spear_quicken: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Spear Quicken',
        abbrev: 'SQK'
    },
    providence: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Providence',
        abbrev: 'PRD'
    },
    shrink: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Shrink',
        abbrev: 'SHK'
    },
    devotion_protection: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Devotion',
        abbrev: 'DVT'
    },

    // -- Wizard buffs (Phase 2B) --
    sight_blaster: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Sight Blaster',
        abbrev: 'SBL'
    },

    // -- Sage buffs (Phase 2C) --
    endow_fire: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Endow Blaze',
        abbrev: 'EBL'
    },
    endow_water: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Endow Tsunami',
        abbrev: 'ETS'
    },
    endow_wind: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Endow Tornado',
        abbrev: 'ETN'
    },
    endow_earth: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Endow Quake',
        abbrev: 'EQK'
    },
    hindsight: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Hindsight',
        abbrev: 'HND'
    },
    magic_rod: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Magic Rod',
        abbrev: 'MRD'
    },
    volcano_zone: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Volcano',
        abbrev: 'VLC'
    },
    deluge_zone: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Deluge',
        abbrev: 'DLG'
    },
    violent_gale_zone: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Violent Gale',
        abbrev: 'VGL'
    },

    // -- Monk buffs (Phase 3A) --
    critical_explosion: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Fury',
        abbrev: 'FRY'
    },
    steel_body: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Steel Body',
        abbrev: 'STL'
    },
    asura_regen_lockout: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Asura Lockout',
        abbrev: 'ASR'
    },
    // Sitting system
    sitting: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Sitting',
        abbrev: 'SIT'
    },
    // Blade Stop catching stance + root lock
    blade_stop_catching: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Blade Stop',
        abbrev: 'BLS'
    },
    root_lock: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Root Lock',
        abbrev: 'RLK'
    },

    // -- Bard/Dancer Performance Buffs (Phase 4) --
    // Song buffs (applied to allies in AoE)
    song_whistle: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'A Whistle',
        abbrev: 'WHI'
    },
    song_assassin_cross: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Assassin Cross of Sunset',
        abbrev: 'ACS'
    },
    song_bragi: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Poem of Bragi',
        abbrev: 'BRG'
    },
    song_apple_of_idun: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Apple of Idun',
        abbrev: 'APL'
    },
    dance_humming: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Humming',
        abbrev: 'HUM'
    },
    dance_fortune_kiss: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: "Fortune's Kiss",
        abbrev: 'FTK'
    },
    dance_service_for_you: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Service for You',
        abbrev: 'SFY'
    },
    // Dance debuffs (applied to enemies in AoE)
    dance_pdfm: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: "Please Don't Forget Me",
        abbrev: 'PDF'
    },
    dance_ugly: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Ugly Dance',
        abbrev: 'UGD'
    },
    // Dissonance has no lingering buff — just damage ticks

    // --- Ensemble buffs (Phase B — Bard+Dancer cooperative) ---
    ensemble_drum: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'A Drum on the Battlefield',
        abbrev: 'DRM'
    },
    ensemble_nibelungen: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'The Ring of Nibelungen',
        abbrev: 'NIB'
    },
    ensemble_mr_kim: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Mr. Kim A Rich Man',
        abbrev: 'KIM'
    },
    ensemble_siegfried: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Invulnerable Siegfried',
        abbrev: 'SIG'
    },
    ensemble_into_abyss: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Into the Abyss',
        abbrev: 'ABY'
    },
    ensemble_aftermath: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Ensemble Aftermath',
        abbrev: 'AFT'
    },

    // --- Blacksmith buffs (Phase 5A) ---
    adrenaline_rush: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Adrenaline Rush',
        abbrev: 'ADR'
    },
    weapon_perfection: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Weapon Perfection',
        abbrev: 'WPF'
    },
    power_thrust: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Power Thrust',
        abbrev: 'PTH'
    },
    maximize_power: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Maximize Power',
        abbrev: 'MXP'
    },

    // --- Rogue buffs/debuffs (Phase 5B) ---
    strip_weapon: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Strip Weapon',
        abbrev: 'SWP'
    },
    strip_shield: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Strip Shield',
        abbrev: 'SSH'
    },
    strip_armor: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Strip Armor',
        abbrev: 'SAR'
    },
    strip_helm: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Strip Helm',
        abbrev: 'SHL'
    },
    raid_debuff: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Raid',
        abbrev: 'RAI'
    },
    close_confine: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Close Confine',
        abbrev: 'CCF'
    },

    // --- Item consumable buffs ---
    aspd_potion: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'ASPD Potion',
        abbrev: 'ASP'
    },
    str_food: { stackRule: 'refresh', category: 'buff', displayName: 'STR Food', abbrev: 'SFD' },
    agi_food: { stackRule: 'refresh', category: 'buff', displayName: 'AGI Food', abbrev: 'AFD' },
    vit_food: { stackRule: 'refresh', category: 'buff', displayName: 'VIT Food', abbrev: 'VFD' },
    int_food: { stackRule: 'refresh', category: 'buff', displayName: 'INT Food', abbrev: 'IFD' },
    dex_food: { stackRule: 'refresh', category: 'buff', displayName: 'DEX Food', abbrev: 'DFD' },
    luk_food: { stackRule: 'refresh', category: 'buff', displayName: 'LUK Food', abbrev: 'LFD' },
    hit_food: { stackRule: 'refresh', category: 'buff', displayName: 'HIT Food', abbrev: 'HFD' },
    flee_food: { stackRule: 'refresh', category: 'buff', displayName: 'FLEE Food', abbrev: 'FFD' },
    item_endow_fire: { stackRule: 'refresh', category: 'buff', displayName: 'Fire Endow', abbrev: 'EFI' },
    item_endow_water: { stackRule: 'refresh', category: 'buff', displayName: 'Water Endow', abbrev: 'EWA' },
    item_endow_wind: { stackRule: 'refresh', category: 'buff', displayName: 'Wind Endow', abbrev: 'EWI' },
    item_endow_earth: { stackRule: 'refresh', category: 'buff', displayName: 'Earth Endow', abbrev: 'EEA' },
    item_endow_dark: { stackRule: 'refresh', category: 'buff', displayName: 'Dark Endow', abbrev: 'EDA' },
};

// stackRule options:
//   'refresh' — replace existing buff of same name (most common)
//   'stack'   — allow multiple instances (rare)
//   'reject'  — do not overwrite if already active

// ============================================================================
// APPLY / REMOVE / EXPIRE
// ============================================================================

/**
 * Apply a buff to a target.
 *
 * @param {Object} target - Player or enemy (needs .activeBuffs array)
 * @param {Object} buffDef - Buff definition with at minimum:
 *   { name, skillId, casterId, casterName, duration }
 *   Plus optional stat fields: defReduction, atkIncrease, mdefBonus, etc.
 * @returns {boolean} true if applied, false if rejected
 */
function applyBuff(target, buffDef) {
    if (!target.activeBuffs) target.activeBuffs = [];

    const config = BUFF_TYPES[buffDef.name];
    const stackRule = (config && config.stackRule) || 'refresh';

    if (stackRule === 'refresh') {
        // Remove existing buff of same name
        target.activeBuffs = target.activeBuffs.filter(b => b.name !== buffDef.name);
    } else if (stackRule === 'reject') {
        if (target.activeBuffs.some(b => b.name === buffDef.name)) {
            return false;
        }
    }
    // 'stack' allows duplicates — no filtering

    const now = Date.now();
    target.activeBuffs.push({
        ...buffDef,
        appliedAt: now,
        expiresAt: now + buffDef.duration
    });

    return true;
}

/**
 * Remove a specific buff by name.
 * @returns {Object|null} The removed buff object, or null if not found
 */
function removeBuff(target, buffName) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) return null;

    const idx = target.activeBuffs.findIndex(b => b.name === buffName);
    if (idx === -1) return null;

    const removed = target.activeBuffs[idx];
    target.activeBuffs.splice(idx, 1);
    return removed;
}

/**
 * Remove expired buffs and return them.
 * @returns {Object[]} Array of expired buff objects
 */
function expireBuffs(target) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) return [];

    const now = Date.now();
    const expired = target.activeBuffs.filter(b => now >= b.expiresAt);
    target.activeBuffs = target.activeBuffs.filter(b => now < b.expiresAt);
    return expired;
}

/**
 * Check if a target has a specific buff active.
 */
function hasBuff(target, buffName) {
    if (!target.activeBuffs) return false;
    const now = Date.now();
    return target.activeBuffs.some(b => b.name === buffName && now < b.expiresAt);
}

// ============================================================================
// BUFF MODIFIER ACCESSOR
// ============================================================================

/**
 * Get aggregate stat modifiers from ALL active buffs on a target.
 *
 * NOTE: This does NOT include status effects (freeze/stone/stun/etc.)
 *       Those are handled by getStatusModifiers() in ro_status_effects.js.
 *       Use getCombinedModifiers() in index.js for the merged result.
 *
 * @param {Object} target
 * @returns {Object} Modifier object with multipliers and flat bonuses
 */
function getBuffModifiers(target) {
    const mods = {
        // Multiplicative modifiers
        defMultiplier: 1.0,      // Applied to equipment hard DEF (Signum Crucis, etc.)
        softDefMultiplier: 1.0,  // Applied to VIT soft DEF (Provoke, Auto Berserk)
        atkMultiplier: 1.0,
        aspdMultiplier: 1.0,

        // Flat additive bonuses
        bonusMDEF: 0,
        strBonus: 0,
        agiBonus: 0,
        vitBonus: 0,
        intBonus: 0,
        dexBonus: 0,
        lukBonus: 0,
        defPercent: 0,       // Angelus: +VIT% defense
        moveSpeedBonus: 0,   // Increase AGI, etc.
        bonusHit: 0,
        bonusFlee: 0,
        bonusCritical: 0,
        bonusMaxHp: 0,
        bonusMaxSp: 0,

        // Special flags
        weaponElement: null,  // Aspersio, Enchant Poison, etc.
        isHidden: false,      // Hiding, Cloaking
        isPlayDead: false,    // Play Dead (TrickDead) — deaggros ALL monsters including bosses
        doubleNextDamage: false, // Lex Aeterna
        blockRanged: false,   // Pneuma
        energyCoatActive: false, // Energy Coat: dynamic physical damage reduction based on SP%
    };

    if (!target.activeBuffs || target.activeBuffs.length === 0) {
        return mods;
    }

    // Haste2 exclusion group: only the strongest ASPD buff applies
    // (song_assassin_cross, adrenaline_rush, two_hand_quicken, spear_quicken)
    let haste2Bonus = 0;

    const now = Date.now();
    for (const buff of target.activeBuffs) {
        if (now >= buff.expiresAt) continue;

        switch (buff.name) {
            case 'provoke':
                // RO Classic (rAthena): Provoke reduces VIT soft DEF (def2), NOT equipment hard DEF
                mods.softDefMultiplier *= (1 - (buff.defReduction || 0) / 100);
                mods.atkMultiplier *= (1 + (buff.atkIncrease || 0) / 100);
                break;

            case 'endure':
                mods.bonusMDEF += (buff.mdefBonus || 0);
                break;

            case 'blessing':
                mods.strBonus += (buff.strBonus || 0);
                mods.dexBonus += (buff.dexBonus || 0);
                mods.intBonus += (buff.intBonus || 0);
                break;

            case 'blessing_debuff':
                mods.strBonus -= (buff.strReduction || 0);
                mods.dexBonus -= (buff.dexReduction || 0);
                mods.intBonus -= (buff.intReduction || 0);
                break;

            case 'increase_agi':
                mods.agiBonus += (buff.agiBonus || 0);
                mods.moveSpeedBonus += (buff.moveSpeedBonus || 0);
                break;

            case 'decrease_agi':
                mods.agiBonus -= (buff.agiReduction || 0);
                mods.moveSpeedBonus -= (buff.moveSpeedReduction || 0);
                break;

            case 'angelus':
                mods.defPercent += (buff.defPercent || 0);
                break;

            case 'gloria':
                mods.lukBonus += (buff.lukBonus || 0);
                break;

            case 'impositio_manus':
                mods.bonusATK = (mods.bonusATK || 0) + (buff.bonusATK || 0);
                break;

            case 'magnificat':
                // rAthena status.cpp SC_MAGNIFICAT: doubles SP regen ONLY (NOT HP).
                // Confirmed by rAthena issue #275 + Aegis testing.
                mods.spRegenMultiplier = buff.spRegenMultiplier || 2;
                break;

            case 'suffragium':
                mods.castTimeReduction = buff.castTimeReduction || 0;
                break;

            case 'kyrie_eleison':
                // No direct stat modification — handled in damage pipeline
                break;

            case 'slow_poison':
                // Blocks poison HP drain — checked in tickStatusEffects integration
                mods.slowPoisonActive = true;
                break;

            case 'bs_sacramenti':
                mods.armorElement = { type: 'holy', level: 1 };
                break;

            case 'improve_concentration':
                mods.agiBonus += (buff.agiBonus || 0);
                mods.dexBonus += (buff.dexBonus || 0);
                break;

            case 'two_hand_quicken':
                // Haste2 group: track highest bonus, apply once after loop
                haste2Bonus = Math.max(haste2Bonus, buff.aspdIncrease || 0);
                mods.bonusCritical += (buff.critBonus || 0);
                mods.bonusHit += (buff.hitBonus || 0);
                break;

            case 'lex_aeterna':
                mods.doubleNextDamage = true;
                break;

            case 'aspersio':
                mods.weaponElement = 'holy';
                break;

            case 'enchant_poison':
                mods.weaponElement = 'poison';
                break;

            case 'auto_counter':
                mods.autoCounterActive = true;
                break;

            case 'pneuma':
                mods.blockRanged = true;
                break;

            case 'hiding':
            case 'cloaking':
                mods.isHidden = true;
                break;

            case 'play_dead':
                mods.isPlayDead = true;
                break;

            case 'quagmire':
                mods.agiBonus -= (buff.agiReduction || 0);
                mods.dexBonus -= (buff.dexReduction || 0);
                mods.moveSpeedBonus -= (buff.moveSpeedReduction || 0);
                break;

            case 'auto_berserk':
                mods.atkMultiplier *= (1 + (buff.atkIncrease || 0) / 100);
                // RO Classic: Auto Berserk = self-Provoke Lv10 → -55% VIT soft DEF when active
                if ((buff.atkIncrease || 0) > 0) mods.softDefMultiplier *= 0.45;
                break;

            case 'energy_coat':
                mods.energyCoatActive = true;
                break;

            case 'loud_exclamation':
                mods.strBonus += (buff.strBonus || 0);
                break;

            case 'signum_crucis':
                mods.defMultiplier *= (1 - (buff.defReduction || 0) / 100);
                break;

            case 'ruwach':
                // Presence only — reveal check done separately
                break;

            case 'magnum_break_fire':
                // Magnum Break fire endow: +20% fire bonus on auto-attacks for 10s
                // Handled in auto-attack tick, not as a stat modifier
                break;

            // -- Crusader buffs (Phase 2A) --
            case 'auto_guard':
                // Block check handled in enemy attack pipeline, not as stat modifier
                mods.autoGuardChance = buff.blockChance || 0;
                mods.autoGuardMoveDelay = buff.moveDelay || 0;
                break;

            case 'reflect_shield':
                mods.reflectShieldPercent = (mods.reflectShieldPercent || 0) + (buff.reflectPercent || 0);
                break;

            case 'defender':
                mods.defenderRangedReduction = buff.rangedReduction || 0;
                mods.moveSpeedBonus -= (buff.moveSpeedPenalty || 0);
                if (buff.aspdPenalty > 0) {
                    mods.aspdMultiplier *= (1 - buff.aspdPenalty / 100);
                }
                break;

            case 'spear_quicken':
                // Haste2 group: track highest bonus, apply once after loop
                haste2Bonus = Math.max(haste2Bonus, buff.aspdIncrease || 0);
                break;

            case 'providence':
                mods.demonResist = (mods.demonResist || 0) + (buff.demonResist || 0);
                mods.holyResist = (mods.holyResist || 0) + (buff.holyResist || 0);
                break;

            case 'shrink':
                mods.shrinkActive = true;
                break;

            case 'devotion_protection':
                mods.devotedTo = buff.devotedTo || null;
                break;

            // -- Sage buffs (Phase 2C) --
            case 'endow_fire':
            case 'endow_water':
            case 'endow_wind':
            case 'endow_earth':
                mods.weaponElement = buff.weaponElement || null;
                break;

            case 'hindsight':
                mods.hindsightActive = true;
                mods.hindsightChance = buff.autocastChance || 0;
                mods.hindsightLevel = buff.hindsightLevel || 0;
                break;

            case 'magic_rod':
                mods.magicRodActive = true;
                mods.magicRodAbsorbPct = buff.absorbPct || 0;
                break;

            // -- Monk buffs (Phase 3A) --
            case 'critical_explosion':
                mods.bonusCritical += (buff.critBonus || 0);
                mods.disableSPRegen = true;
                mods.furyActive = true;
                break;

            case 'steel_body':
                mods.overrideHardDEF = buff.overrideHardDEF || 90;
                mods.overrideHardMDEF = buff.overrideHardMDEF || 90;
                mods.aspdMultiplier *= 0.75; // -25% ASPD
                mods.moveSpeedBonus -= 25;
                mods.blockActiveSkills = true;
                break;

            case 'asura_regen_lockout':
                mods.disableSPRegen = true;
                break;

            case 'sitting':
                mods.isSitting = true;
                break;

            case 'blade_stop_catching':
                mods.bladeStopCatching = true;
                break;

            case 'root_lock':
                mods.rootLockActive = true;
                if (buff.isPlayer) {
                    mods.rootLockEnemyId = buff.lockedEnemyId || null;
                    mods.rootLockBladeStopLevel = buff.bladeStopLevel || 0;
                } else {
                    mods.rootLockPlayerId = buff.lockedPlayerId || null;
                }
                break;

            // -- Bard/Dancer Performance Buffs (Phase 4) --
            case 'song_whistle':
                mods.bonusFlee += (buff.fleeBonus || 0);
                mods.bonusPerfectDodge = (mods.bonusPerfectDodge || 0) + (buff.perfectDodgeBonus || 0);
                break;

            case 'song_assassin_cross':
                // Haste2 group: track highest bonus, apply once after loop
                haste2Bonus = Math.max(haste2Bonus, buff.aspdBonus || 0);
                break;

            case 'song_bragi':
                // Cast time and after-cast delay reduction percentages
                mods.bragiCastReduction = (mods.bragiCastReduction || 0) + (buff.castReduction || 0);
                mods.bragiAcdReduction = (mods.bragiAcdReduction || 0) + (buff.acdReduction || 0);
                break;

            case 'song_apple_of_idun':
                // MaxHP percentage boost + HP regen per tick (regen handled in performance tick)
                mods.bonusMaxHpPercent = (mods.bonusMaxHpPercent || 0) + (buff.maxHpPercent || 0);
                break;

            case 'dance_humming':
                mods.bonusHit += (buff.hitBonus || 0);
                break;

            case 'dance_fortune_kiss':
                mods.bonusCritical += (buff.critBonus || 0);
                break;

            case 'dance_service_for_you':
                // MaxSP% boost + SP cost reduction (SP reduction checked in skill:use handler)
                mods.bonusMaxSpPercent = (mods.bonusMaxSpPercent || 0) + (buff.maxSpPercent || 0);
                mods.spCostReduction = (mods.spCostReduction || 0) + (buff.spCostReduction || 0);
                break;

            case 'dance_pdfm':
                // ASPD + move speed debuff on enemies
                mods.aspdMultiplier *= (1 - (buff.aspdReduction || 0) / 100);
                mods.moveSpeedBonus -= (buff.moveSpeedReduction || 0);
                break;

            case 'dance_ugly':
                // SP drain — handled in performance tick, no stat modifier
                break;

            // --- Ensemble buffs (Phase B — Bard+Dancer cooperative) ---
            case 'ensemble_drum':
                // Drum on the Battlefield: +ATK and +DEF for party members
                mods.bonusATK = (mods.bonusATK || 0) + (buff.atkBonus || 0);
                mods.bonusDEF = (mods.bonusDEF || 0) + (buff.defBonus || 0);
                break;
            case 'ensemble_nibelungen':
                // Ring of Nibelungen: +ATK for Lv4 weapon wielders (DEF-ignoring flat ATK)
                mods.bonusATK = (mods.bonusATK || 0) + (buff.atkBonus || 0);
                break;
            case 'ensemble_mr_kim':
                // Mr. Kim A Rich Man: +EXP bonus (checked in EXP distribution)
                mods.expBonusPct = (mods.expBonusPct || 0) + (buff.expBonusPct || 0);
                break;
            case 'ensemble_siegfried':
                // Invulnerable Siegfried: per-element resist (non-Neutral) + status resist
                // elementResist is an object { fire: 60, water: 60, ... } or a flat number (legacy)
                if (buff.elementResist && typeof buff.elementResist === 'object') {
                    if (!mods.elementResist || typeof mods.elementResist !== 'object') mods.elementResist = {};
                    for (const [el, pct] of Object.entries(buff.elementResist)) {
                        mods.elementResist[el] = (mods.elementResist[el] || 0) + pct;
                    }
                } else if (buff.elementResist) {
                    // Legacy flat number fallback — apply to all 9 non-Neutral elements
                    if (!mods.elementResist || typeof mods.elementResist !== 'object') mods.elementResist = {};
                    ['fire', 'water', 'earth', 'wind', 'poison', 'holy', 'dark', 'ghost', 'undead'].forEach(el => {
                        mods.elementResist[el] = (mods.elementResist[el] || 0) + buff.elementResist;
                    });
                }
                mods.statusResist = (mods.statusResist || 0) + (buff.statusResist || 0);
                break;
            case 'ensemble_into_abyss':
                // Into the Abyss: no gemstone cost (checked in skill:use handler)
                mods.noGemStone = true;
                break;
            case 'ensemble_aftermath':
                // Ensemble aftermath: no skills for 10s + 50% move speed reduction (checked in skill:use handler)
                mods.blockActiveSkills = true;
                mods.moveSpeedBonus -= (buff.moveSpeedReduction || 50);
                break;

            // --- Blacksmith buffs (Phase 5A) ---
            case 'adrenaline_rush':
                // Haste2 group: convert multiplier (e.g., 1.3) to bonus % (e.g., 30)
                haste2Bonus = Math.max(haste2Bonus, ((buff.aspdMultiplier || 1.0) - 1) * 100);
                break;
            case 'weapon_perfection':
                // Remove size penalty — all sizes take 100% weapon damage
                mods.noSizePenalty = true;
                break;
            case 'power_thrust':
                // +5-25% ATK for caster, +5% for party
                mods.atkMultiplier *= (1 + (buff.atkPercent || 0) / 100);
                break;
            case 'maximize_power':
                // Always use max weapon variance (no random roll)
                mods.maximizePower = true;
                break;

            // --- Rogue strip debuffs (Phase 5B) ---
            case 'strip_weapon':
                mods.atkMultiplier *= (1 - (buff.atkReduction || 0));  // 0.25 = 25% ATK reduction
                break;
            case 'strip_shield':
                mods.hardDefReduction = (mods.hardDefReduction || 0) + (buff.defReduction || 0); // 0.15 = 15% hard DEF reduction
                break;
            case 'strip_armor':
                mods.vitMultiplier = (mods.vitMultiplier || 1.0) * (1 - (buff.vitReduction || 0)); // 0.40 = 40% VIT reduction
                break;
            case 'strip_helm':
                mods.intMultiplier = (mods.intMultiplier || 1.0) * (1 - (buff.intReduction || 0)); // 0.40 = 40% INT reduction
                break;

            // --- Alchemist debuffs (Acid Terror armor break, Demonstration weapon break) ---
            case 'armor_break':
                // Pre-renewal: armor break removes hard DEF entirely (sets to 0).
                // Stored as fraction 0-1 (1.0 = 100% reduction → DEF effectively 0).
                mods.hardDefReduction = (mods.hardDefReduction || 0) + (buff.hardDefReduction || 0);
                break;
            case 'weapon_break':
                // Pre-renewal weapon break: equipped weapon disabled. In PvE we treat as a fractional
                // ATK debuff on the affected entity (same fraction format as strip_weapon).
                mods.atkMultiplier *= (1 - (buff.atkReduction || 0));
                break;
            case 'raid_debuff':
                // Pre-renewal: +20% incoming damage (bosses +10%), expires after 5s OR 7 hits
                // Applied as final damage multiplier, NOT a DEF reduction
                mods.raidDamageIncrease = (mods.raidDamageIncrease || 0) + (buff.incomingDamageIncrease || 0);
                mods.raidHitsRemaining = buff.raidHitsRemaining || 0;
                break;
            case 'close_confine':
                if (buff.isConfiner) {
                    mods.bonusFlee += (buff.bonusFlee || 0);
                }
                mods.closeConfineActive = true;
                break;

            // --- Item consumable buffs ---
            case 'aspd_potion':
                mods.aspdPotionReduction = buff.aspdPotionReduction || 0;
                break;

            case 'str_food':
                mods.strBonus += (buff.statBonus || 0);
                break;
            case 'agi_food':
                mods.agiBonus += (buff.statBonus || 0);
                break;
            case 'vit_food':
                mods.vitBonus += (buff.statBonus || 0);
                break;
            case 'int_food':
                mods.intBonus += (buff.statBonus || 0);
                break;
            case 'dex_food':
                mods.dexBonus += (buff.statBonus || 0);
                break;
            case 'luk_food':
                mods.lukBonus += (buff.statBonus || 0);
                break;
            case 'hit_food':
                mods.bonusHit += (buff.hitBonus || 0);
                break;
            case 'flee_food':
                mods.bonusFlee += (buff.fleeBonus || 0);
                break;

            // Item elemental converter endow buffs
            case 'item_endow_fire':
                mods.weaponElement = 'fire';
                break;
            case 'item_endow_water':
                mods.weaponElement = 'water';
                break;
            case 'item_endow_wind':
                mods.weaponElement = 'wind';
                break;
            case 'item_endow_earth':
                mods.weaponElement = 'earth';
                break;
            case 'item_endow_dark':
                mods.weaponElement = 'dark';
                break;

            // Sage elemental zone buffs (applied while standing in zone)
            case 'volcano_zone':
                mods.bonusATK = (mods.bonusATK || 0) + (buff.atkBonus || 0);
                mods.fireDmgBoost = (mods.fireDmgBoost || 0) + (buff.fireDmgBoost || 0);
                break;
            case 'deluge_zone':
                mods.bonusMaxHpPercent = (mods.bonusMaxHpPercent || 0) + (buff.maxHpBoost || 0);
                mods.waterDmgBoost = (mods.waterDmgBoost || 0) + (buff.waterDmgBoost || 0);
                break;
            case 'violent_gale_zone':
                mods.bonusFlee += (buff.fleeBonus || 0);
                mods.windDmgBoost = (mods.windDmgBoost || 0) + (buff.windDmgBoost || 0);
                break;

            // Generic fallback: check for common stat fields
            default:
                if (buff.defReduction) mods.defMultiplier *= (1 - buff.defReduction / 100);
                if (buff.atkIncrease) mods.atkMultiplier *= (1 + buff.atkIncrease / 100);
                if (buff.mdefBonus) mods.bonusMDEF += buff.mdefBonus;
                break;
        }
    }

    // Apply Haste2 exclusion group: only the strongest ASPD buff takes effect
    if (haste2Bonus > 0) {
        mods.aspdMultiplier *= (1 + haste2Bonus / 100);
    }

    return mods;
}

/**
 * Get all active buffs as a serializable array (for buff:list event).
 */
function getActiveBuffList(target) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) {
        return [];
    }
    const now = Date.now();
    return target.activeBuffs
        .filter(b => now < b.expiresAt)
        .map(b => {
            const config = BUFF_TYPES[b.name];
            return {
                name: b.name,
                skillId: b.skillId,
                duration: b.duration,
                remainingMs: b.expiresAt - now,
                category: (config && config.category) || 'buff',
                displayName: (config && config.displayName) || b.name,
                abbrev: (config && config.abbrev) || b.name.substring(0, 3).toUpperCase()
            };
        });
}

// ============================================================================
// MODULE EXPORTS
// ============================================================================

module.exports = {
    BUFF_TYPES,
    applyBuff,
    removeBuff,
    expireBuffs,
    hasBuff,
    getBuffModifiers,
    getActiveBuffList
};
