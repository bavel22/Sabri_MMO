/**
 * Ragnarok Online Skill Data — Pre-Renewal Classic
 * Complete skill definitions for Novice, 6 First Classes, 12 Second Classes.
 * Sources: iRO Wiki, rAthena pre-renewal database
 */
'use strict';

// Helper: generate level array for simple scaling skills
function genLevels(count, fn) {
    return Array.from({ length: count }, (_, i) => fn(i));
}

// ============================================================
// SKILL DEFINITIONS — All classes
// ============================================================
const SKILL_DEFINITIONS = [
    // === NOVICE (IDs 1-10) ===
    { id: 1, name: 'basic_skill', displayName: 'Basic Skill', classId: 'novice', maxLevel: 9, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Enables basic commands. Required for class change at level 9.', icon: 'basic_skill', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(9, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 })) },
    { id: 2, name: 'first_aid', displayName: 'First Aid', classId: 'novice', maxLevel: 1, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Restore 5 HP.', icon: 'first_aid', treeRow: 0, treeCol: 1, prerequisites: [], levels: [{ level: 1, spCost: 3, castTime: 0, cooldown: 0, effectValue: 5, duration: 0 }] },
    { id: 3, name: 'play_dead', displayName: 'Play Dead', classId: 'novice', maxLevel: 1, type: 'toggle', targetType: 'self', element: 'neutral', range: 0, description: 'Pretend to be dead. Monsters ignore you.', icon: 'play_dead', treeRow: 1, treeCol: 0, prerequisites: [{ skillId: 1, level: 1 }], levels: [{ level: 1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 }] },

    // === SWORDSMAN (IDs 100-119) ===
    { id: 100, name: 'sword_mastery', displayName: 'Sword Mastery', classId: 'swordsman', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases ATK with 1H Swords and Daggers.', icon: 'sword_mastery', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: (i+1)*4, duration: 0 })) },
    { id: 101, name: 'two_handed_sword_mastery', displayName: 'Two-Handed Sword Mastery', classId: 'swordsman', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases ATK with 2H Swords.', icon: 'two_handed_sword_mastery', treeRow: 1, treeCol: 0, prerequisites: [{ skillId: 100, level: 1 }], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: (i+1)*4, duration: 0 })) },
    { id: 102, name: 'increase_hp_recovery', displayName: 'Increase HP Recovery', classId: 'swordsman', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases natural HP recovery and healing items.', icon: 'increase_hp_recovery', treeRow: 0, treeCol: 2, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: (i+1)*5, duration: 0 })) },
    { id: 103, name: 'bash', displayName: 'Bash', classId: 'swordsman', maxLevel: 10, type: 'active', targetType: 'single', element: 'neutral', range: 150, description: 'Smash a target for increased damage.', icon: 'bash', treeRow: 0, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: i < 5 ? 8 : 15, castTime: 0, afterCastDelay: 0, cooldown: 700, effectValue: 130 + i*30, duration: 0 })) },
    { id: 104, name: 'provoke', displayName: 'Provoke', classId: 'swordsman', maxLevel: 10, type: 'active', targetType: 'single', element: 'neutral', range: 450, description: 'Enrage target: -DEF, +ATK.', icon: 'provoke', treeRow: 1, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 4+i, castTime: 0, afterCastDelay: 0, cooldown: 500, effectValue: 5+i*3, duration: 30000 })) },
    { id: 105, name: 'magnum_break', displayName: 'Magnum Break', classId: 'swordsman', maxLevel: 10, type: 'active', targetType: 'ground', element: 'fire', range: 50, description: 'AoE fire attack centered on targeted ground.', icon: 'magnum_break', treeRow: 1, treeCol: 2, prerequisites: [{ skillId: 103, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: 30, castTime: 0, afterCastDelay: 0, cooldown: 2000, effectValue: 120+i*20, duration: 0 })) },
    { id: 106, name: 'endure', displayName: 'Endure', classId: 'swordsman', maxLevel: 10, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Move while being attacked. Grants MDEF.', icon: 'endure', treeRow: 2, treeCol: 1, prerequisites: [{ skillId: 104, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: 10, castTime: 0, afterCastDelay: 0, cooldown: 10000, effectValue: i+1, duration: 10000+i*3000 })) },

    // === MAGE (IDs 200-229) ===
    { id: 200, name: 'cold_bolt', displayName: 'Cold Bolt', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'water', range: 900, description: 'Water bolt magic. +1 bolt per level.', icon: 'cold_bolt', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 12+i*2, castTime: 700*(i+1), afterCastDelay: 800+(i+1)*200, cooldown: 0, effectValue: 100, duration: 0 })) },
    { id: 201, name: 'fire_bolt', displayName: 'Fire Bolt', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'fire', range: 900, description: 'Fire bolt magic. +1 bolt per level.', icon: 'fire_bolt', treeRow: 0, treeCol: 2, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 12+i*2, castTime: 700*(i+1), afterCastDelay: 800+(i+1)*200, cooldown: 0, effectValue: 100, duration: 0 })) },
    { id: 202, name: 'lightning_bolt', displayName: 'Lightning Bolt', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'wind', range: 900, description: 'Wind bolt magic. +1 bolt per level.', icon: 'lightning_bolt', treeRow: 0, treeCol: 4, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 12+i*2, castTime: 700*(i+1), afterCastDelay: 800+(i+1)*200, cooldown: 0, effectValue: 100, duration: 0 })) },
    { id: 203, name: 'napalm_beat', displayName: 'Napalm Beat', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'ghost', range: 900, description: 'Ghost property AoE spell. Damage split among targets.', icon: 'napalm_beat', treeRow: 0, treeCol: 3, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: [9,9,9,12,12,12,15,15,15,18][i], castTime: 1000, afterCastDelay: [1000,1000,1000,900,900,800,800,700,600,500][i], cooldown: 0, effectValue: 80+i*10, duration: 0 })) },
    { id: 204, name: 'increase_sp_recovery', displayName: 'Increase SP Recovery', classId: 'mage', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases natural SP recovery.', icon: 'increase_sp_recovery', treeRow: 0, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: (i+1)*3, duration: 0 })) },
    { id: 205, name: 'sight', displayName: 'Sight', classId: 'mage', maxLevel: 1, type: 'active', targetType: 'self', element: 'fire', range: 0, description: 'Reveal hidden enemies.', icon: 'sight', treeRow: 1, treeCol: 1, prerequisites: [], levels: [{ level: 1, spCost: 10, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: 0, duration: 10000 }] },
    { id: 206, name: 'stone_curse', displayName: 'Stone Curse', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'earth', range: 300, description: 'Petrify a target. Requires Red Gemstone.', icon: 'stone_curse', treeRow: 2, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 25-i, castTime: 1000, afterCastDelay: 0, cooldown: 0, effectValue: 24+i*4, duration: 20000 })) },
    { id: 207, name: 'fire_ball', displayName: 'Fire Ball', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'fire', range: 900, description: 'Fire AoE attack. 5x5 splash around target.', icon: 'fire_ball', treeRow: 1, treeCol: 2, prerequisites: [{ skillId: 201, level: 4 }], levels: genLevels(10, i => ({ level: i+1, spCost: 25, castTime: i < 5 ? 1500 : 1000, afterCastDelay: i < 5 ? 1500 : 1000, cooldown: 0, effectValue: 80+i*10, duration: 0 })) },
    { id: 208, name: 'frost_diver', displayName: 'Frost Diver', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'water', range: 900, description: 'Freeze a target with ice.', icon: 'frost_diver', treeRow: 1, treeCol: 0, prerequisites: [{ skillId: 200, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: 25-i, castTime: 800, afterCastDelay: 1500, cooldown: 0, effectValue: 110+i*10, duration: (i+1)*3000 })) },
    { id: 209, name: 'fire_wall', displayName: 'Fire Wall', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'ground', element: 'fire', range: 900, description: 'Create a fire barrier. Knocks enemies back.', icon: 'fire_wall', treeRow: 2, treeCol: 2, prerequisites: [{ skillId: 207, level: 5 }, { skillId: 205, level: 1 }], levels: genLevels(10, i => ({ level: i+1, spCost: 40, castTime: Math.floor(2150-(i+1)*150), afterCastDelay: 0, cooldown: 0, effectValue: i+3, duration: (5+i)*1000 })) },
    { id: 210, name: 'soul_strike', displayName: 'Soul Strike', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'single', element: 'ghost', range: 900, description: 'Ghost bolts. Extra damage vs Undead.', icon: 'soul_strike', treeRow: 1, treeCol: 3, prerequisites: [{ skillId: 203, level: 4 }], levels: [{ level: 1, spCost: 18, castTime: 500, afterCastDelay: 1200, cooldown: 0, effectValue: 100, duration: 0 },{ level: 2, spCost: 14, castTime: 500, afterCastDelay: 1400, cooldown: 0, effectValue: 100, duration: 0 },{ level: 3, spCost: 24, castTime: 500, afterCastDelay: 1600, cooldown: 0, effectValue: 100, duration: 0 },{ level: 4, spCost: 20, castTime: 500, afterCastDelay: 1800, cooldown: 0, effectValue: 100, duration: 0 },{ level: 5, spCost: 30, castTime: 500, afterCastDelay: 2000, cooldown: 0, effectValue: 100, duration: 0 },{ level: 6, spCost: 26, castTime: 500, afterCastDelay: 2200, cooldown: 0, effectValue: 100, duration: 0 },{ level: 7, spCost: 36, castTime: 500, afterCastDelay: 2400, cooldown: 0, effectValue: 100, duration: 0 },{ level: 8, spCost: 32, castTime: 500, afterCastDelay: 2600, cooldown: 0, effectValue: 100, duration: 0 },{ level: 9, spCost: 42, castTime: 500, afterCastDelay: 2800, cooldown: 0, effectValue: 100, duration: 0 },{ level: 10, spCost: 38, castTime: 500, afterCastDelay: 2700, cooldown: 0, effectValue: 100, duration: 0 }] },
    { id: 211, name: 'safety_wall', displayName: 'Safety Wall', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'ground', element: 'neutral', range: 900, description: 'Blocks melee physical attacks. Needs Blue Gem.', icon: 'safety_wall', treeRow: 2, treeCol: 3, prerequisites: [{ skillId: 203, level: 7 }, { skillId: 210, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: [30,30,30,35,35,35,40,40,40,40][i], castTime: [4000,3500,3500,2500,2000,1500,1000,1000,1000,1000][i], afterCastDelay: 0, cooldown: 0, effectValue: i+2, duration: (i+1)*5000 })) },
    { id: 212, name: 'thunderstorm', displayName: 'Thunderstorm', classId: 'mage', maxLevel: 10, type: 'active', targetType: 'ground', element: 'wind', range: 900, description: 'Wind AoE storm. +1 hit per level.', icon: 'thunderstorm', treeRow: 1, treeCol: 4, prerequisites: [{ skillId: 202, level: 4 }], levels: genLevels(10, i => ({ level: i+1, spCost: 24+(i+1)*5, castTime: (i+1)*1000, afterCastDelay: 2000, cooldown: 0, effectValue: 80, duration: 0 })) },

    // === ARCHER (IDs 300-319) ===
    { id: 300, name: 'owls_eye', displayName: "Owl's Eye", classId: 'archer', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases DEX.', icon: 'owls_eye', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: i+1, duration: 0 })) },
    { id: 301, name: 'vultures_eye', displayName: "Vulture's Eye", classId: 'archer', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases bow range and HIT.', icon: 'vultures_eye', treeRow: 1, treeCol: 0, prerequisites: [{ skillId: 300, level: 3 }], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: i+1, duration: 0 })) },
    { id: 302, name: 'improve_concentration', displayName: 'Improve Concentration', classId: 'archer', maxLevel: 10, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Increase AGI/DEX, reveal hidden.', icon: 'improve_concentration', treeRow: 2, treeCol: 0, prerequisites: [{ skillId: 301, level: 1 }], levels: genLevels(10, i => ({ level: i+1, spCost: 25+i*5, castTime: 0, cooldown: 0, effectValue: 3+i, duration: 60000 })) },
    { id: 303, name: 'double_strafe', displayName: 'Double Strafe', classId: 'archer', maxLevel: 10, type: 'active', targetType: 'single', element: 'neutral', range: 800, description: 'Fire two arrows at once.', icon: 'double_strafe', treeRow: 0, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 12, castTime: 0, cooldown: 300, effectValue: 100+i*10, duration: 0 })) },
    { id: 304, name: 'arrow_shower', displayName: 'Arrow Shower', classId: 'archer', maxLevel: 10, type: 'active', targetType: 'ground', element: 'neutral', range: 800, description: 'AoE arrow volley.', icon: 'arrow_shower', treeRow: 1, treeCol: 1, prerequisites: [{ skillId: 303, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: 15, castTime: 0, cooldown: 1000, effectValue: 75+i*5, duration: 0 })) },
    { id: 305, name: 'arrow_crafting', displayName: 'Arrow Crafting', classId: 'archer', maxLevel: 1, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Create arrows from items.', icon: 'arrow_crafting', treeRow: 0, treeCol: 2, prerequisites: [], levels: [{ level: 1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 }] },

    // === ACOLYTE (IDs 400-429) ===
    { id: 400, name: 'heal', displayName: 'Heal', classId: 'acolyte', maxLevel: 10, type: 'active', targetType: 'single', element: 'holy', range: 450, description: 'Restore HP. Damages Undead.', icon: 'heal', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 13+i*3, castTime: 0, cooldown: 1000, effectValue: 18*(i+1), duration: 0 })) },
    { id: 401, name: 'divine_protection', displayName: 'Divine Protection', classId: 'acolyte', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'DEF vs Undead/Demon.', icon: 'divine_protection', treeRow: 0, treeCol: 2, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: (i+1)*3, duration: 0 })) },
    { id: 402, name: 'blessing', displayName: 'Blessing', classId: 'acolyte', maxLevel: 10, type: 'active', targetType: 'single', element: 'holy', range: 450, description: 'Increase STR/DEX/INT.', icon: 'blessing', treeRow: 1, treeCol: 2, prerequisites: [{ skillId: 401, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: 28+i*4, castTime: 0, cooldown: 0, effectValue: i+1, duration: 60000+i*12000 })) },
    { id: 403, name: 'increase_agi', displayName: 'Increase AGI', classId: 'acolyte', maxLevel: 10, type: 'active', targetType: 'single', element: 'holy', range: 450, description: 'Increase AGI temporarily.', icon: 'increase_agi', treeRow: 0, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 18+i*3, castTime: 1000, cooldown: 0, effectValue: 3+i, duration: 60000+i*12000 })) },
    { id: 404, name: 'decrease_agi', displayName: 'Decrease AGI', classId: 'acolyte', maxLevel: 10, type: 'active', targetType: 'single', element: 'holy', range: 450, description: 'Decrease target AGI.', icon: 'decrease_agi', treeRow: 1, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 15+i*2, castTime: 1000, cooldown: 0, effectValue: 3+i, duration: 60000+i*6000 })) },
    { id: 405, name: 'cure', displayName: 'Cure', classId: 'acolyte', maxLevel: 1, type: 'active', targetType: 'single', element: 'holy', range: 450, description: 'Remove status effects.', icon: 'cure', treeRow: 1, treeCol: 0, prerequisites: [{ skillId: 400, level: 2 }], levels: [{ level: 1, spCost: 15, castTime: 0, cooldown: 1000, effectValue: 0, duration: 0 }] },
    { id: 406, name: 'angelus', displayName: 'Angelus', classId: 'acolyte', maxLevel: 10, type: 'active', targetType: 'self', element: 'holy', range: 0, description: 'Increase party VIT DEF.', icon: 'angelus', treeRow: 2, treeCol: 2, prerequisites: [{ skillId: 401, level: 3 }], levels: genLevels(10, i => ({ level: i+1, spCost: 23+i*4, castTime: 0, cooldown: 0, effectValue: 5+i*5, duration: 30000+i*30000 })) },
    { id: 407, name: 'signum_crucis', displayName: 'Signum Crucis', classId: 'acolyte', maxLevel: 10, type: 'active', targetType: 'aoe', element: 'holy', range: 0, description: 'Decrease DEF of Undead/Demon.', icon: 'signum_crucis', treeRow: 0, treeCol: 3, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 35, castTime: 0, cooldown: 3000, effectValue: 14+i*3, duration: 0 })) },
    { id: 408, name: 'ruwach', displayName: 'Ruwach', classId: 'acolyte', maxLevel: 1, type: 'active', targetType: 'self', element: 'holy', range: 0, description: 'Reveal hidden enemies.', icon: 'ruwach', treeRow: 2, treeCol: 0, prerequisites: [], levels: [{ level: 1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 145, duration: 10000 }] },
    { id: 409, name: 'teleport', displayName: 'Teleport', classId: 'acolyte', maxLevel: 2, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Teleport randomly or to save point.', icon: 'teleport', treeRow: 3, treeCol: 0, prerequisites: [{ skillId: 408, level: 1 }], levels: genLevels(2, i => ({ level: i+1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 })) },
    { id: 410, name: 'warp_portal', displayName: 'Warp Portal', classId: 'acolyte', maxLevel: 4, type: 'active', targetType: 'ground', element: 'neutral', range: 450, description: 'Create portal. Needs Blue Gem.', icon: 'warp_portal', treeRow: 4, treeCol: 0, prerequisites: [{ skillId: 409, level: 2 }], levels: genLevels(4, i => ({ level: i+1, spCost: 35, castTime: 1000, cooldown: 0, effectValue: i+1, duration: 10000 })) },
    { id: 411, name: 'pneuma', displayName: 'Pneuma', classId: 'acolyte', maxLevel: 1, type: 'active', targetType: 'ground', element: 'neutral', range: 450, description: 'Block all ranged attacks.', icon: 'pneuma', treeRow: 5, treeCol: 0, prerequisites: [{ skillId: 410, level: 4 }], levels: [{ level: 1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 0, duration: 10000 }] },
    { id: 412, name: 'aqua_benedicta', displayName: 'Aqua Benedicta', classId: 'acolyte', maxLevel: 1, type: 'active', targetType: 'self', element: 'holy', range: 0, description: 'Create Holy Water.', icon: 'aqua_benedicta', treeRow: 0, treeCol: 4, prerequisites: [], levels: [{ level: 1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 }] },

    // === THIEF (IDs 500-519) ===
    { id: 500, name: 'double_attack', displayName: 'Double Attack', classId: 'thief', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Chance to double hit with daggers.', icon: 'double_attack', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 5*(i+1), duration: 0 })) },
    { id: 501, name: 'improve_dodge', displayName: 'Improve Dodge', classId: 'thief', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increases Flee.', icon: 'improve_dodge', treeRow: 0, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: (i+1)*3, duration: 0 })) },
    { id: 502, name: 'steal', displayName: 'Steal', classId: 'thief', maxLevel: 10, type: 'active', targetType: 'single', element: 'neutral', range: 150, description: 'Steal items from monsters.', icon: 'steal', treeRow: 0, treeCol: 2, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 10, castTime: 0, cooldown: 1000, effectValue: 10+i*5, duration: 0 })) },
    { id: 503, name: 'hiding', displayName: 'Hiding', classId: 'thief', maxLevel: 10, type: 'toggle', targetType: 'self', element: 'neutral', range: 0, description: 'Hide from enemies.', icon: 'hiding', treeRow: 1, treeCol: 2, prerequisites: [{ skillId: 502, level: 5 }], levels: genLevels(10, i => ({ level: i+1, spCost: 10, castTime: 0, cooldown: 500, effectValue: 0, duration: 30000+i*6000 })) },
    { id: 504, name: 'envenom', displayName: 'Envenom', classId: 'thief', maxLevel: 10, type: 'active', targetType: 'single', element: 'poison', range: 150, description: 'Poison attack.', icon: 'envenom', treeRow: 1, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 12, castTime: 0, cooldown: 500, effectValue: 15*(i+1), duration: 0 })) },
    { id: 505, name: 'detoxify', displayName: 'Detoxify', classId: 'thief', maxLevel: 1, type: 'active', targetType: 'single', element: 'neutral', range: 450, description: 'Cure poison.', icon: 'detoxify', treeRow: 2, treeCol: 0, prerequisites: [{ skillId: 504, level: 3 }], levels: [{ level: 1, spCost: 10, castTime: 0, cooldown: 500, effectValue: 0, duration: 0 }] },

    // === MERCHANT (IDs 600-619) ===
    { id: 600, name: 'enlarge_weight_limit', displayName: 'Enlarge Weight Limit', classId: 'merchant', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increase max carry weight.', icon: 'enlarge_weight_limit', treeRow: 0, treeCol: 0, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 200*(i+1), duration: 0 })) },
    { id: 601, name: 'discount', displayName: 'Discount', classId: 'merchant', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Reduce NPC buy prices.', icon: 'discount', treeRow: 1, treeCol: 0, prerequisites: [{ skillId: 600, level: 3 }], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 7+i*2, duration: 0 })) },
    { id: 602, name: 'overcharge', displayName: 'Overcharge', classId: 'merchant', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Increase NPC sell prices.', icon: 'overcharge', treeRow: 2, treeCol: 0, prerequisites: [{ skillId: 601, level: 3 }], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 7+i*2, duration: 0 })) },
    { id: 603, name: 'mammonite', displayName: 'Mammonite', classId: 'merchant', maxLevel: 10, type: 'active', targetType: 'single', element: 'neutral', range: 150, description: 'Powerful attack costing Zeny.', icon: 'mammonite', treeRow: 0, treeCol: 1, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 5, castTime: 0, cooldown: 800, effectValue: 150+i*50, duration: 0 })) },
    { id: 604, name: 'pushcart', displayName: 'Pushcart', classId: 'merchant', maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0, description: 'Use a cart for storage.', icon: 'pushcart', treeRow: 0, treeCol: 2, prerequisites: [], levels: genLevels(10, i => ({ level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: (i+1)*5, duration: 0 })) },
    { id: 605, name: 'vending', displayName: 'Vending', classId: 'merchant', maxLevel: 10, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Open player shop.', icon: 'vending', treeRow: 1, treeCol: 2, prerequisites: [{ skillId: 604, level: 3 }], levels: genLevels(10, i => ({ level: i+1, spCost: 30, castTime: 0, cooldown: 0, effectValue: i+3, duration: 0 })) },
    { id: 606, name: 'item_appraisal', displayName: 'Item Appraisal', classId: 'merchant', maxLevel: 1, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Identify items.', icon: 'item_appraisal', treeRow: 1, treeCol: 1, prerequisites: [], levels: [{ level: 1, spCost: 10, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 }] },
    { id: 607, name: 'change_cart', displayName: 'Change Cart', classId: 'merchant', maxLevel: 1, type: 'active', targetType: 'self', element: 'neutral', range: 0, description: 'Change cart appearance.', icon: 'change_cart', treeRow: 2, treeCol: 2, prerequisites: [{ skillId: 604, level: 5 }], levels: [{ level: 1, spCost: 0, castTime: 0, cooldown: 0, effectValue: 0, duration: 0 }] },
];

// Second class skills loaded from separate file to keep this manageable
const SECOND_CLASS_SKILLS = require('./ro_skill_data_2nd');

// Merge all skills
const ALL_SKILLS = SKILL_DEFINITIONS.concat(SECOND_CLASS_SKILLS);

// Build lookup maps
const SKILL_MAP = new Map();
for (const s of ALL_SKILLS) SKILL_MAP.set(s.id, s);

const CLASS_SKILLS = {};
for (const s of ALL_SKILLS) {
    if (!CLASS_SKILLS[s.classId]) CLASS_SKILLS[s.classId] = [];
    CLASS_SKILLS[s.classId].push(s);
}

// Class progression chains
const CLASS_PROGRESSION = {
    'novice': ['novice'],
    'swordsman': ['novice', 'swordsman'],
    'mage': ['novice', 'mage'],
    'archer': ['novice', 'archer'],
    'acolyte': ['novice', 'acolyte'],
    'thief': ['novice', 'thief'],
    'merchant': ['novice', 'merchant'],
    'knight': ['novice', 'swordsman', 'knight'],
    'crusader': ['novice', 'swordsman', 'crusader'],
    'wizard': ['novice', 'mage', 'wizard'],
    'sage': ['novice', 'mage', 'sage'],
    'hunter': ['novice', 'archer', 'hunter'],
    'bard': ['novice', 'archer', 'bard'],
    'dancer': ['novice', 'archer', 'dancer'],
    'priest': ['novice', 'acolyte', 'priest'],
    'monk': ['novice', 'acolyte', 'monk'],
    'assassin': ['novice', 'thief', 'assassin'],
    'rogue': ['novice', 'thief', 'rogue'],
    'blacksmith': ['novice', 'merchant', 'blacksmith'],
    'alchemist': ['novice', 'merchant', 'alchemist'],
};

// Get all skills available to a player of a given class
function getAvailableSkills(jobClass) {
    const chain = CLASS_PROGRESSION[jobClass] || ['novice'];
    const skills = [];
    for (const cls of chain) {
        if (CLASS_SKILLS[cls]) skills.push(...CLASS_SKILLS[cls]);
    }
    return skills;
}

// Validate if player can learn a skill
function canLearnSkill(skillId, playerSkills, jobClass, skillPoints) {
    const skill = SKILL_MAP.get(skillId);
    if (!skill) return { ok: false, reason: 'Skill not found' };
    if (skillPoints < 1) return { ok: false, reason: 'No skill points available' };

    // Check class access
    const chain = CLASS_PROGRESSION[jobClass] || ['novice'];
    if (!chain.includes(skill.classId)) return { ok: false, reason: `${skill.displayName} is not available for your class` };

    // Check current level
    const currentLevel = playerSkills[skillId] || 0;
    if (currentLevel >= skill.maxLevel) return { ok: false, reason: `${skill.displayName} is already at max level` };

    // Check prerequisites
    for (const prereq of skill.prerequisites) {
        const prereqLevel = playerSkills[prereq.skillId] || 0;
        if (prereqLevel < prereq.level) {
            const prereqSkill = SKILL_MAP.get(prereq.skillId);
            const prereqName = prereqSkill ? prereqSkill.displayName : `Skill #${prereq.skillId}`;
            return { ok: false, reason: `Requires ${prereqName} level ${prereq.level} (current: ${prereqLevel})` };
        }
    }

    return { ok: true };
}

module.exports = {
    ALL_SKILLS,
    SKILL_MAP,
    CLASS_SKILLS,
    CLASS_PROGRESSION,
    getAvailableSkills,
    canLearnSkill,
    genLevels
};
