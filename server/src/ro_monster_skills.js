/**
 * ro_monster_skills.js — Monster Skill Database (Pre-Renewal)
 *
 * Equivalent to rAthena's db/pre-re/mob_skill_db.txt
 * Defines which skills each monster can use, when, and how often.
 *
 * FORMAT per entry:
 *   skillId     — Skill ID (NPC_* for monster-only, player skill IDs for copyable)
 *   skillName   — Readable name (cosmetic, for logging)
 *   level       — Skill level (can exceed player max for power skills)
 *   state       — AI state trigger: 'any','idle','attack','chase','angry','dead'
 *   rate        — Chance out of 10000 (10000 = 100%) per evaluation tick
 *   castTime    — Milliseconds before skill executes (0 = instant)
 *   delay       — Cooldown in ms before skill can be reused
 *   cancelable  — Can cast be interrupted by damage? (true/false)
 *   target      — 'target','self','friend','randomtarget','ground'
 *   condition   — Trigger condition (see CONDITION_TYPES below)
 *   conditionValue — Primary condition parameter
 *   val1        — Secondary condition parameter (e.g., upper HP% bound)
 *   isPlayerSkill — If true, skill is a player-class skill → copyable by Plagiarism
 *   element     — Override element for this skill (null = use monster element)
 *   damageType  — 'physical','magical','heal','status','utility','summon'
 *   effectMultiplier — Damage % (100 = 100% ATK)
 *   statusEffect — Status to apply on hit (for status_melee types)
 *   statusChance — % chance to apply status (0-100)
 *   statusDuration — Status duration in ms
 *   emotion     — Emote ID to display (-1 = none)
 *
 * CONDITION_TYPES:
 *   'always'           — No condition, just rate check
 *   'myhpltmaxrate'    — Monster HP < conditionValue% of max
 *   'myhpinrate'       — Monster HP between conditionValue% and val1%
 *   'mystatuson'       — Monster has status conditionValue active
 *   'mystatusoff'      — Monster does NOT have status conditionValue
 *   'friendhpltmaxrate'— Nearby ally HP < conditionValue%
 *   'closedattacked'   — Was hit by melee attack
 *   'longrangeattacked'— Was hit by ranged attack
 *   'skillused'        — Specific skill (conditionValue) was used on this monster
 *   'casttargeted'     — A player is casting a spell at this monster
 *   'rudeattacked'     — Monster attacked out of range / can't path to target
 *   'onspawn'          — Just spawned or respawned
 *   'slavelt'          — Slave count < conditionValue
 *   'afterskill'       — Just cast skill conditionValue
 *   'attackpcgt'       — More than conditionValue players attacking
 *
 * STATE MAPPING (rAthena → Our AI_STATE):
 *   'idle'      → AI_STATE.IDLE (wandering, no target)
 *   'attack'    → AI_STATE.ATTACK (in range, auto-attacking)
 *   'chase'     → AI_STATE.CHASE (pursuing target)
 *   'angry'     → AI_STATE.ATTACK (first aggro)
 *   'any'       → All states except DEAD
 *   'anytarget' → ATTACK + CHASE (any state with valid target)
 *   'dead'      → AI_STATE.DEAD (on-death skills)
 *
 * Sources:
 *   - rAthena pre-re/mob_skill_db.txt (https://github.com/rathena/rathena/blob/master/db/pre-re/mob_skill_db.txt)
 *   - rAthena DeepWiki Monster System (https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)
 *   - iRO Wiki Classic monster pages
 */

'use strict';

// ============================================================
// NPC_SKILLS — Monster-Only Skill Definitions
// These skills can ONLY be used by monsters.
// They are NOT in the player SKILL_MAP and NOT copyable by Plagiarism.
// ============================================================
const NPC_SKILLS = {
    // --- Elemental Melee Attacks ---
    // Replace one auto-attack with elemental damage (same ATK%, forced element)
    184: { name: 'NPC_WATERATTACK', type: 'elemental_melee', element: 'water', multiplier: 100 },
    185: { name: 'NPC_GROUNDATTACK', type: 'elemental_melee', element: 'earth', multiplier: 100 },
    186: { name: 'NPC_FIREATTACK', type: 'elemental_melee', element: 'fire', multiplier: 100 },
    187: { name: 'NPC_WINDATTACK', type: 'elemental_melee', element: 'wind', multiplier: 100 },
    188: { name: 'NPC_POISONATTACK', type: 'elemental_melee', element: 'poison', multiplier: 100 },
    189: { name: 'NPC_HOLYATTACK', type: 'elemental_melee', element: 'holy', multiplier: 100 },
    190: { name: 'NPC_DARKNESSATTACK', type: 'elemental_melee', element: 'dark', multiplier: 100 },
    191: { name: 'NPC_GHOSTATTACK', type: 'elemental_melee', element: 'ghost', multiplier: 100 },
    192: { name: 'NPC_UNDEADATTACK', type: 'elemental_melee', element: 'undead', multiplier: 100 },

    // --- Status Melee Attacks ---
    // Auto-attack + status effect chance on hit
    176: { name: 'NPC_POISON', type: 'status_melee', status: 'poison', chance: 20, duration: 60000, multiplier: 100 },
    177: { name: 'NPC_BLINDATTACK', type: 'status_melee', status: 'blind', chance: 20, duration: 30000, multiplier: 100 },
    178: { name: 'NPC_SILENCEATTACK', type: 'status_melee', status: 'silence', chance: 20, duration: 30000, multiplier: 100 },
    179: { name: 'NPC_STUNATTACK', type: 'status_melee', status: 'stun', chance: 20, duration: 5000, multiplier: 100 },
    180: { name: 'NPC_PETRIFYATTACK', type: 'status_melee', status: 'stone', chance: 5, duration: 20000, multiplier: 100 },
    181: { name: 'NPC_CURSEATTACK', type: 'status_melee', status: 'curse', chance: 20, duration: 30000, multiplier: 100 },
    182: { name: 'NPC_SLEEPATTACK', type: 'status_melee', status: 'sleep', chance: 20, duration: 30000, multiplier: 100 },
    183: { name: 'NPC_RANDOMATTACK', type: 'random_element', multiplier: 100 },

    // --- Multi-Hit / Special Physical ---
    171: { name: 'NPC_COMBOATTACK', type: 'multi_hit', hits: 2, multiplier: 100 },
    197: { name: 'NPC_CRITICALSLASH', type: 'forced_crit', multiplier: 100 },

    // --- Self Buffs ---
    196: { name: 'NPC_SPEEDUP', type: 'self_buff', buffName: 'speed_boost', speedMultiplier: 1.5, duration: 10000 },
    199: { name: 'NPC_AGIUP', type: 'self_buff', buffName: 'agi_boost', agiBonus: 20, duration: 30000 },

    // --- Area Attacks ---
    653: { name: 'NPC_EARTHQUAKE', type: 'aoe_physical', radius: 750, multiplier: 500, element: 'neutral', hits: 1 },
    656: { name: 'NPC_DARKBREATH', type: 'ranged_magic', element: 'dark', multiplier: 500 },
    657: { name: 'NPC_DARKBLESSING', type: 'status_ranged', status: 'curse', chance: 50, duration: 30000 },
    660: { name: 'NPC_WIDEBLEEDING', type: 'aoe_status', radius: 750, status: 'bleeding', chance: 100, duration: 60000 },
    661: { name: 'NPC_WIDESILENCE', type: 'aoe_status', radius: 750, status: 'silence', chance: 100, duration: 30000 },
    662: { name: 'NPC_WIDESTUN', type: 'aoe_status', radius: 750, status: 'stun', chance: 100, duration: 5000 },
    663: { name: 'NPC_WIDECURSE', type: 'aoe_status', radius: 750, status: 'curse', chance: 100, duration: 30000 },
    665: { name: 'NPC_WIDEFREEZE', type: 'aoe_status', radius: 750, status: 'freeze', chance: 100, duration: 10000 },
    669: { name: 'NPC_WIDESLEEP', type: 'aoe_status', radius: 750, status: 'sleep', chance: 100, duration: 30000 },

    // --- Drain / Recovery ---
    687: { name: 'NPC_BLOODDRAIN', type: 'drain_hp', drainPercent: 5, multiplier: 100 },
    688: { name: 'NPC_ENERGYDRAIN', type: 'drain_sp', drainPercent: 5, multiplier: 100 },

    // --- Utility ---
    198: { name: 'NPC_EMOTION', type: 'emote' },
    304: { name: 'NPC_METAMORPHOSIS', type: 'transform' },
    485: { name: 'NPC_SUMMONSLAVE', type: 'summon' },
    175: { name: 'NPC_SELFDESTRUCTION', type: 'self_destruct', multiplier: 400, element: 'neutral' },

    // --- Healing (monster self-heal) ---
    // Monsters can use AL_HEAL on themselves — this is a PLAYER skill so isPlayerSkill=true on the entry
};

// ============================================================
// MONSTER_SKILL_DB — Per-Monster Skill Entries
// Key: Monster template ID (rAthena mob ID)
// Value: Array of skill rules, evaluated top-to-bottom (first match wins)
// ============================================================
const MONSTER_SKILL_DB = {

    // ================================================================
    // ZONE 1: PRONTERA SOUTH — Starter Monsters (Level 1-10)
    // Most low-level monsters have NO skills or 1 basic skill
    // ================================================================

    // Poring (1002) — Level 1, Water/1, Plant — NO SKILLS
    // Lunatic (1063) — Level 3, Neutral/3, Brute — NO SKILLS
    // Fabre (1007) — Level 2, Earth/1, Insect — NO SKILLS
    // Pupa (1008) — Level 2, Earth/1, Insect — IMMOBILE, NO SKILLS
    // Drops (1113) — Level 3, Fire/1, Plant — NO SKILLS
    // Chonchon (1011) — Level 4, Wind/1, Insect — NO SKILLS
    // Condor (1009) — Level 5, Wind/1, Brute — NO SKILLS
    // Willow (1010) — Level 4, Earth/1, Plant — NO SKILLS
    // Roda Frog (1012) — Level 5, Water/1, Fish — NO SKILLS
    // Savage Babe (1167) — Level 7, Earth/1, Brute — NO SKILLS
    // Rocker (1052) — Level 9, Earth/1, Insect — NO SKILLS

    // Hornet (1004) — Level 8, Wind/1, Insect — Poison on attack
    1004: [
        {
            skillId: 176, skillName: 'NPC_POISON', level: 1,
            state: 'attack', rate: 500, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // Familiar (1005) — Level 8, Shadow/1, Brute — Blind on attack
    1005: [
        {
            skillId: 177, skillName: 'NPC_BLINDATTACK', level: 1,
            state: 'attack', rate: 500, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // ================================================================
    // ZONE 2-3: MID-LEVEL MONSTERS (Level 10-21)
    // ================================================================

    // Spore (1014) — Level 16, Water/1, Plant — Water elemental attack
    1014: [
        {
            skillId: 184, skillName: 'NPC_WATERATTACK', level: 1,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
    ],

    // Zombie (1015) — Level 15, Undead/1, Undead — Dark attack + Stun
    1015: [
        {
            skillId: 190, skillName: 'NPC_DARKNESSATTACK', level: 1,
            state: 'attack', rate: 500, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
        {
            skillId: 179, skillName: 'NPC_STUNATTACK', level: 3,
            state: 'chase', rate: 500, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // Skeleton (1076) — Level 10, Undead/1, Undead — Stun attack
    1076: [
        {
            skillId: 179, skillName: 'NPC_STUNATTACK', level: 3,
            state: 'attack', rate: 500, castTime: 0, delay: 7000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // Creamy (1018) — Level 16, Wind/1, Insect — Teleport when rude attacked
    1018: [
        {
            skillId: 26, skillName: 'AL_TELEPORT', level: 1,
            state: 'idle', rate: 10000, castTime: 0, delay: 5000, cancelable: true,
            target: 'self', condition: 'rudeattacked', conditionValue: 0,
            isPlayerSkill: true, damageType: 'utility', emotion: -1,
        },
    ],

    // Poporing (1031) — Level 14, Poison/1, Plant — Poison elemental attack
    1031: [
        {
            skillId: 188, skillName: 'NPC_POISONATTACK', level: 1,
            state: 'attack', rate: 1000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
    ],

    // Pecopeco (1019) — Level 19, Fire/1, Brute — No special skills (basic aggressive melee)

    // Mandragora (1020) — Level 12, Earth/3, Plant — Immobile, no skills

    // Poison Spore (1077) — Level 19, Poison/1, Plant — Poison attack + Poison status
    1077: [
        {
            skillId: 176, skillName: 'NPC_POISON', level: 3,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
        {
            skillId: 188, skillName: 'NPC_POISONATTACK', level: 2,
            state: 'attack', rate: 500, castTime: 0, delay: 10000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
    ],

    // Smokie (1056) — Level 18, Earth/1, Brute — Hiding, combo attack
    1056: [
        {
            skillId: 171, skillName: 'NPC_COMBOATTACK', level: 1,
            state: 'attack', rate: 500, castTime: 0, delay: 10000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
        // Smokie uses Hiding — but this requires a special handler (deferred)
    ],

    // Yoyo (1057) — Level 21, Earth/1, Brute — Combo attack, throws banana
    1057: [
        {
            skillId: 171, skillName: 'NPC_COMBOATTACK', level: 2,
            state: 'attack', rate: 500, castTime: 0, delay: 8000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
    ],

    // ================================================================
    // BOSS / MVP MONSTERS (Future zones, included for completeness)
    // ================================================================

    // Osiris (1038) — MVP Level 78, Undead/4 — Heal self, Teleport, Curse, Coma
    1038: [
        {
            skillId: 26, skillName: 'AL_TELEPORT', level: 1,
            state: 'idle', rate: 10000, castTime: 0, delay: 0, cancelable: true,
            target: 'self', condition: 'rudeattacked', conditionValue: 0,
            isPlayerSkill: true, damageType: 'utility', emotion: -1,
        },
        {
            skillId: 28, skillName: 'AL_HEAL', level: 11,
            state: 'attack', rate: 5000, castTime: 1000, delay: 10000, cancelable: true,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 50,
            isPlayerSkill: true, damageType: 'heal', emotion: -1,
        },
        {
            skillId: 181, skillName: 'NPC_CURSEATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
        {
            skillId: 190, skillName: 'NPC_DARKNESSATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
    ],

    // Baphomet (1039) — MVP Level 81, Dark/3 — Earthquake, Meteor, Heal, Slaves
    1039: [
        {
            skillId: 653, skillName: 'NPC_EARTHQUAKE', level: 5,
            state: 'attack', rate: 5000, castTime: 2000, delay: 30000, cancelable: false,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 80,
            isPlayerSkill: false, damageType: 'physical', emotion: 6,
        },
        {
            skillId: 28, skillName: 'AL_HEAL', level: 11,
            state: 'attack', rate: 5000, castTime: 1500, delay: 15000, cancelable: true,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 40,
            isPlayerSkill: true, damageType: 'heal', emotion: -1,
        },
        {
            skillId: 190, skillName: 'NPC_DARKNESSATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
        {
            skillId: 485, skillName: 'NPC_SUMMONSLAVE', level: 1,
            state: 'attack', rate: 10000, castTime: 0, delay: 60000, cancelable: false,
            target: 'self', condition: 'slavelt', conditionValue: 2,
            isPlayerSkill: false, damageType: 'summon', emotion: -1,
            val1: 1101, // Slave monster ID (Bapho Jr.)
        },
    ],

    // Drake (1112) — MVP Level 84, Undead/1 — Water Ball, Curse, Stun
    1112: [
        {
            skillId: 86, skillName: 'MG_WATERBALL', level: 9,
            state: 'attack', rate: 3000, castTime: 500, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: true, damageType: 'magical', emotion: -1,
        },
        {
            skillId: 181, skillName: 'NPC_CURSEATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
        {
            skillId: 179, skillName: 'NPC_STUNATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // Angeling (1096) — Mini-Boss Level 99, Holy/4 — Heal self, Holy Light
    1096: [
        {
            skillId: 28, skillName: 'AL_HEAL', level: 9,
            state: 'any', rate: 5000, castTime: 1000, delay: 10000, cancelable: true,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 60,
            isPlayerSkill: true, damageType: 'heal', emotion: -1,
        },
        {
            skillId: 75, skillName: 'AL_HOLYLIGHT', level: 5,
            state: 'attack', rate: 3000, castTime: 500, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: true, damageType: 'magical', emotion: -1,
        },
    ],

    // Deviling (1582) — Mini-Boss Level 52, Dark/4 — Soul Strike, Sight, Teleport
    1582: [
        {
            skillId: 26, skillName: 'AL_TELEPORT', level: 1,
            state: 'idle', rate: 10000, castTime: 0, delay: 5000, cancelable: true,
            target: 'self', condition: 'rudeattacked', conditionValue: 0,
            isPlayerSkill: true, damageType: 'utility', emotion: -1,
        },
        {
            skillId: 13, skillName: 'MG_SOULSTRIKE', level: 9,
            state: 'attack', rate: 3000, castTime: 500, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: true, damageType: 'magical', emotion: -1,
        },
    ],

    // Orc Hero (1087) — MVP Level 90 — Power Thrust, Comboattack, Earthquake
    1087: [
        {
            skillId: 653, skillName: 'NPC_EARTHQUAKE', level: 3,
            state: 'attack', rate: 3000, castTime: 2000, delay: 20000, cancelable: false,
            target: 'self', condition: 'attackpcgt', conditionValue: 2,
            isPlayerSkill: false, damageType: 'physical', emotion: 6,
        },
        {
            skillId: 171, skillName: 'NPC_COMBOATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
        {
            skillId: 179, skillName: 'NPC_STUNATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // Mistress (1059) — MVP Level 75, Wind/4 — Jupitel Thunder, Stun, Silence
    1059: [
        {
            skillId: 84, skillName: 'MG_JUPITEL', level: 10,
            state: 'attack', rate: 3000, castTime: 1000, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: true, damageType: 'magical', emotion: -1,
        },
        {
            skillId: 662, skillName: 'NPC_WIDESTUN', level: 5,
            state: 'attack', rate: 5000, castTime: 0, delay: 15000, cancelable: false,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 60,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // Maya (1147) — MVP Level 86, Earth/4 — Heal, Fire Pillar, Earthquake
    1147: [
        {
            skillId: 28, skillName: 'AL_HEAL', level: 11,
            state: 'any', rate: 5000, castTime: 1000, delay: 10000, cancelable: true,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 50,
            isPlayerSkill: true, damageType: 'heal', emotion: -1,
        },
        {
            skillId: 80, skillName: 'MG_FIREPILLAR', level: 10,
            state: 'attack', rate: 3000, castTime: 1000, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: true, damageType: 'magical', emotion: -1,
        },
        {
            skillId: 653, skillName: 'NPC_EARTHQUAKE', level: 3,
            state: 'attack', rate: 3000, castTime: 2000, delay: 20000, cancelable: false,
            target: 'self', condition: 'myhpltmaxrate', conditionValue: 50,
            isPlayerSkill: false, damageType: 'physical', emotion: 6,
        },
    ],

    // Eddga (1115) — MVP Level 74, Fire/1 — Stun, Fire Attack, Power Up
    1115: [
        {
            skillId: 186, skillName: 'NPC_FIREATTACK', level: 3,
            state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'physical', emotion: -1,
        },
        {
            skillId: 179, skillName: 'NPC_STUNATTACK', level: 5,
            state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true,
            target: 'target', condition: 'always', conditionValue: 0,
            isPlayerSkill: false, damageType: 'status', emotion: -1,
        },
    ],

    // ============================================================
    // Key MVPs and Bosses (added 2026-03-18, rAthena pre-re mob_skill_db.txt verified)
    // ============================================================

    // Golden Thief Bug (1086) — Prontera Sewers MVP, Lv64
    1086: [
        { skillId: 28, skillName: 'AL_HEAL', level: 9, state: 'attack', rate: 5000, castTime: 2000, delay: 10000, cancelable: false, target: 'self', condition: 'myhpltmaxrate', conditionValue: 50, isPlayerSkill: true, damageType: 'heal', emotion: -1 },
        { skillId: 186, skillName: 'NPC_FIREATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 179, skillName: 'NPC_STUNATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'status', emotion: -1 },
    ],

    // Moonlight Flower (1150) — Payon Cave MVP, Lv67
    1150: [
        { skillId: 19, skillName: 'MG_FIREBOLT', level: 10, state: 'attack', rate: 3000, castTime: 1500, delay: 5000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
        { skillId: 20, skillName: 'MG_COLDBOLT', level: 10, state: 'attack', rate: 3000, castTime: 1500, delay: 5000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
        { skillId: 21, skillName: 'MG_LIGHTNINGBOLT', level: 10, state: 'attack', rate: 3000, castTime: 1500, delay: 5000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
        { skillId: 28, skillName: 'AL_HEAL', level: 9, state: 'attack', rate: 5000, castTime: 2000, delay: 10000, cancelable: false, target: 'self', condition: 'myhpltmaxrate', conditionValue: 50, isPlayerSkill: true, damageType: 'heal', emotion: -1 },
        { skillId: 41, skillName: 'MC_MAMMONITE', level: 10, state: 'attack', rate: 1000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'physical', emotion: -1 },
    ],

    // Phreeoni (1159) — Morroc Field MVP, Lv69
    1159: [
        { skillId: 171, skillName: 'NPC_COMBOATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 179, skillName: 'NPC_STUNATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'status', emotion: -1 },
        { skillId: 26, skillName: 'AL_TELEPORT', level: 1, state: 'idle', rate: 10000, castTime: 0, delay: 0, cancelable: true, target: 'self', condition: 'rudeattacked', conditionValue: 0, isPlayerSkill: true, damageType: 'utility', emotion: -1 },
    ],

    // Orc Lord (1190) — Orc Dungeon MVP, Lv74
    1190: [
        { skillId: 653, skillName: 'NPC_EARTHQUAKE', level: 5, state: 'attack', rate: 2000, castTime: 2000, delay: 15000, cancelable: false, target: 'target', condition: 'attackpcgt', conditionValue: 2, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 653, skillName: 'NPC_EARTHQUAKE', level: 3, state: 'attack', rate: 1000, castTime: 2000, delay: 15000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 171, skillName: 'NPC_COMBOATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 179, skillName: 'NPC_STUNATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'status', emotion: -1 },
    ],

    // Stormy Knight (1251) — Toy Factory MVP, Lv75
    1251: [
        { skillId: 89, skillName: 'WZ_STORMGUST', level: 10, state: 'attack', rate: 2000, castTime: 5000, delay: 15000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
        { skillId: 84, skillName: 'WZ_JUPITELTHUNDER', level: 10, state: 'attack', rate: 3000, castTime: 3000, delay: 5000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
        { skillId: 20, skillName: 'MG_COLDBOLT', level: 10, state: 'attack', rate: 3000, castTime: 1500, delay: 3000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
    ],

    // Dark Lord (1272) — Glast Heim MVP, Lv80
    1272: [
        { skillId: 656, skillName: 'NPC_DARKBREATH', level: 5, state: 'attack', rate: 2000, castTime: 2000, delay: 10000, cancelable: false, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'magical', emotion: -1 },
        { skillId: 83, skillName: 'MG_METEORSTORM', level: 10, state: 'attack', rate: 1500, castTime: 5000, delay: 20000, cancelable: false, target: 'target', condition: 'myhpltmaxrate', conditionValue: 70, isPlayerSkill: true, damageType: 'magical', emotion: -1 },
        { skillId: 190, skillName: 'NPC_DARKNESSATTACK', level: 5, state: 'attack', rate: 3000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 28, skillName: 'AL_HEAL', level: 11, state: 'attack', rate: 5000, castTime: 2000, delay: 10000, cancelable: false, target: 'self', condition: 'myhpltmaxrate', conditionValue: 30, isPlayerSkill: true, damageType: 'heal', emotion: -1 },
    ],

    // Abysmal Knight (1219) — Glast Heim Boss, Lv63
    1219: [
        { skillId: 179, skillName: 'NPC_STUNATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'status', emotion: -1 },
        { skillId: 190, skillName: 'NPC_DARKNESSATTACK', level: 5, state: 'attack', rate: 3000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 171, skillName: 'NPC_COMBOATTACK', level: 5, state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
    ],

    // Raydric (1163) — Glast Heim Boss, Lv52
    1163: [
        { skillId: 179, skillName: 'NPC_STUNATTACK', level: 3, state: 'attack', rate: 2000, castTime: 0, delay: 5000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'status', emotion: -1 },
        { skillId: 190, skillName: 'NPC_DARKNESSATTACK', level: 3, state: 'attack', rate: 3000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
        { skillId: 171, skillName: 'NPC_COMBOATTACK', level: 3, state: 'attack', rate: 2000, castTime: 0, delay: 3000, cancelable: true, target: 'target', condition: 'always', conditionValue: 0, isPlayerSkill: false, damageType: 'physical', emotion: -1 },
    ],
};

// ============================================================
// Helper: get skills for a monster by template ID
// ============================================================
function getMonsterSkills(templateId) {
    return MONSTER_SKILL_DB[templateId] || null;
}

// ============================================================
// Helper: check if a skill is a player-class skill (copyable by Plagiarism)
// ============================================================
function isPlayerClassSkill(skillId) {
    // If it's in NPC_SKILLS, it's monster-only (NOT player class)
    if (NPC_SKILLS[skillId]) return false;
    // Otherwise assume it's a player skill
    return true;
}

module.exports = {
    NPC_SKILLS,
    MONSTER_SKILL_DB,
    getMonsterSkills,
    isPlayerClassSkill,
};
