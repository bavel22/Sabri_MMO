# Rogue Skills — Remaining Systems Implementation Plan

**Date:** 2026-03-17
**Scope:** All deferred Rogue skill issues (#1, #7, #8, #15, #17, #19, #20-24) plus Monster Skill System
**Status:** ALL IMPLEMENTED — 19/19 Rogue skills at 100%, Monster Skill System active

---

## Executive Summary

After the Phase A-F fixes, **8 remaining items** need implementation to achieve 100% pre-renewal Rogue skill accuracy. **7 can be implemented immediately** — only Plagiarism's PvE copy trigger is blocked by the missing Monster Skill System (a large independent feature).

| # | System | Can Implement Now? | Blocking Dependency | Effort |
|---|--------|--------------------|---------------------|--------|
| 1 | Close Confine break conditions | YES | None | 15 min |
| 2 | Close Confine mutual expiry | YES | None | 5 min |
| 3 | Raid debuff hit counter in skill paths | YES | None | 10 min |
| 4 | Tunnel Drive server speed enforcement | YES | None | 10 min |
| 5 | Snatcher inventory:data emit | YES | None | 5 min |
| 6 | Plagiarism DB persistence + rejoin | YES | None | 15 min |
| 7 | Intimidate zone-bounded teleport | YES | None (workaround) | 10 min |
| 8 | Plagiarism PvE copy from monsters | **BLOCKED** | Monster Skill System | Large |

**Total implementable now:** ~70 min of work for items 1-7.

---

## Item 1: Close Confine Break Conditions (#7)

### What's Missing
Close Confine currently only breaks on duration expiry (10s timer). Per RO Classic pre-renewal, it must also break on:

| Condition | Verified Source |
|-----------|----------------|
| Knockback moves target 3+ cells from caster | iRO Wiki Classic, rAthena map.cpp |
| Either party dies | rateMyServer pre-renewal |
| Either party teleports | rateMyServer pre-renewal |
| Target enters/exits Hiding | iRO Wiki Classic |
| Duration expires (10s) | All sources (already works) |

### Implementation

**A. Knockback break** — Add hook in `knockbackTarget()` (line 2536):
```javascript
// After target position is updated (target.x = newX, target.y = newY):
// Check if target has close_confine buff
if (hasBuff(target, 'close_confine')) {
    const ccBuff = target.activeBuffs.find(b => b.name === 'close_confine');
    if (ccBuff) {
        // Find the paired entity (confiner or confined)
        breakCloseConfine(target, zone, io, 'knockback');
    }
}
```

**B. Helper function** — Add `breakCloseConfine(entity, zone, io, reason)`:
```javascript
function breakCloseConfine(entity, zone, io, reason) {
    const ccBuff = (entity.activeBuffs || []).find(b => b.name === 'close_confine');
    if (!ccBuff) return;

    // Remove from this entity
    removeBuff(entity, 'close_confine');
    const entityId = entity.enemyId || entity.characterId;
    const isEnemy = !!entity.enemyId;
    broadcastToZone(zone, 'skill:buff_removed', { targetId: entityId, isEnemy, buffName: 'close_confine', reason });

    // Find and remove from paired entity
    if (ccBuff.isConfiner && ccBuff.targetId != null) {
        // Caster has targetId → find the confined enemy
        const pairedEnemy = enemies.get(ccBuff.targetId);
        if (pairedEnemy && hasBuff(pairedEnemy, 'close_confine')) {
            removeBuff(pairedEnemy, 'close_confine');
            broadcastToZone(zone, 'skill:buff_removed', { targetId: ccBuff.targetId, isEnemy: true, buffName: 'close_confine', reason });
        }
    } else if (!ccBuff.isConfiner && ccBuff.confinerId != null) {
        // Target has confinerId → find the confining player
        const pairedPlayer = connectedPlayers.get(ccBuff.confinerId);
        if (pairedPlayer && hasBuff(pairedPlayer, 'close_confine')) {
            removeBuff(pairedPlayer, 'close_confine');
            broadcastToZone(zone, 'skill:buff_removed', { targetId: ccBuff.confinerId, isEnemy: false, buffName: 'close_confine', reason });
        }
    }
}
```

**C. Enemy death hook** — In `processEnemyDeathFromSkill()`:
```javascript
// Before clearing buffs on death:
if (hasBuff(enemy, 'close_confine')) {
    breakCloseConfine(enemy, zone, io, 'death');
}
```

**D. Player death hook** — In player death handler:
```javascript
if (hasBuff(player, 'close_confine')) {
    breakCloseConfine(player, playerZone, io, 'death');
}
```

**E. Teleport hook** — In every `player:teleport` emission path:
```javascript
if (hasBuff(player, 'close_confine')) {
    breakCloseConfine(player, playerZone, io, 'teleport');
}
```

**F. Hiding hook** — When Hiding buff is applied:
```javascript
// In the Hiding skill handler, after applyBuff(player, { name: 'hiding' }):
if (hasBuff(player, 'close_confine')) {
    breakCloseConfine(player, zone, io, 'hiding');
}
```

### Dependencies
None — `knockbackTarget()`, `processEnemyDeathFromSkill()`, teleport handlers, and Hiding handler all exist.

---

## Item 2: Close Confine Mutual Expiry (#8)

### What's Missing
When one party's `close_confine` buff expires via timer, the other party's buff continues independently.

### Implementation
In the buff expiry tick (line ~21615), after `expireBuffs(player)` returns expired buffs, check for close_confine:
```javascript
for (const buff of expiredBuffs) {
    // ... existing broadcast logic ...

    // Close Confine: remove paired buff on expiry
    if (buff.name === 'close_confine') {
        breakCloseConfine(player, playerZone, io, 'expired');
    }
}
```

Also add the same check in the enemy buff tick (if enemies have their own buff expiry loop).

### Dependencies
Requires `breakCloseConfine()` from Item 1.

---

## Item 3: Raid Debuff Hit Counter in Skill Damage Paths

### What's Missing
`decrementRaidDebuffHits()` is only called in the auto-attack combat tick. Per rAthena, the hit counter should decrement on ALL damage instances — physical skills, magical skills, auto-attacks, ground effects, everything.

### Implementation
Add `decrementRaidDebuffHits(enemy, zone)` call in:
1. **`executePhysicalSkillOnEnemy()`** — after damage is applied to enemy
2. **All custom skill handlers that deal damage** — after `enemy.health = Math.max(0, ...)` lines
3. **`calculateAndApplyMagicalDamage()`** or equivalent magic paths
4. **Ground effect damage ticks** (Sanctuary undead, Storm Gust, etc.)

The most efficient approach: add the call in the 2-3 central damage application functions that ALL damage flows through, rather than in each individual handler.

### Dependencies
None — `decrementRaidDebuffHits()` already exists.

---

## Item 4: Tunnel Drive Server Speed Enforcement (#17)

### What's Missing
Server accepts all position updates from hidden players with Tunnel Drive. Speed cap is client-only.

### Pre-Renewal Canonical Speeds
| Level | Speed % of Normal |
|-------|-------------------|
| 1 | 26% |
| 2 | 32% |
| 3 | 38% |
| 4 | 44% |
| 5 | 50% |

Source: rAthena `status_calc_speed`, rateMyServer, iRO Wiki.

### Implementation
In `player:position` handler, after the Tunnel Drive movement is allowed:
```javascript
// Tunnel Drive: allow movement but enforce speed cap server-side
const tdSpeeds = [0.26, 0.32, 0.38, 0.44, 0.50];
const maxSpeedFraction = tdSpeeds[tunnelDriveLv - 1] || 0.50;
const timeSinceLastPos = now - (player.lastPositionTime || now);
const maxDistPerMs = (player.normalMoveSpeed || 0.3) * maxSpeedFraction; // UE units per ms
const actualDist = Math.sqrt((x - player.lastX) ** 2 + (y - player.lastY) ** 2);
const maxAllowedDist = maxDistPerMs * timeSinceLastPos + 50; // 50 UE tolerance
if (actualDist > maxAllowedDist) {
    // Clamp to max allowed position
    const ratio = maxAllowedDist / actualDist;
    x = player.lastX + (x - player.lastX) * ratio;
    y = player.lastY + (y - player.lastY) * ratio;
}
```

### Dependencies
None.

### Notes
Requires `player.lastPositionTime` to be tracked (set `player.lastPositionTime = Date.now()` at end of position handler). If this isn't tracked yet, add it.

---

## Item 5: Snatcher inventory:data Emit (#15)

### What's Missing
After successful auto-steal, the item is added to DB and weight is updated, but the client isn't notified to refresh inventory.

### Implementation
After `addItemToInventory()` succeeds in the Snatcher combat tick:
```javascript
if (addResult) {
    attacker.currentWeight = (attacker.currentWeight || 0) + stolenWeight;
    const sName = stolenDef?.name || drop.itemName || 'an item';
    const sSocket = io.sockets.sockets.get(attacker.socketId);
    if (sSocket) {
        sSocket.emit('chat:receive', { ... }); // existing
        // NEW: Refresh client inventory
        const sInventory = await getPlayerInventory(attackerId);
        sSocket.emit('inventory:data', {
            items: sInventory,
            currentWeight: attacker.currentWeight,
            maxWeight: getPlayerMaxWeight(attacker)
        });
    }
}
```

### Dependencies
None — `getPlayerInventory()` and `inventory:data` event already exist.

---

## Item 6: Plagiarism DB Persistence (#21, #22, #23)

### What's Missing
Plagiarized skill is memory-only. Lost on disconnect/restart.

### Implementation

**A. DB Migration** — `database/migrations/add_plagiarism_columns.sql`:
```sql
ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_id INTEGER DEFAULT NULL;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_level INTEGER DEFAULT 0;
```

**B. Auto-create at startup** — Add to the server startup `ADD COLUMN IF NOT EXISTS` block:
```javascript
await pool.query(`ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_id INTEGER DEFAULT NULL`);
await pool.query(`ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_level INTEGER DEFAULT 0`);
```

**C. Save on copy** — In `checkPlagiarismCopy()`, after setting `targetPlayer.plagiarizedSkill`:
```javascript
pool.query('UPDATE characters SET plagiarized_skill_id = $1, plagiarized_skill_level = $2 WHERE id = $3',
    [skillId, usableLevel, targetCharId]).catch(err => logger.error('[PLAGIARISM] DB save error:', err));
```

**D. Load on player:join** — In the `player:join` handler, after loading character data:
```javascript
if (charRow.plagiarized_skill_id) {
    const plagSkillDef = SKILL_MAP.get(charRow.plagiarized_skill_id);
    player.plagiarizedSkill = {
        skillId: charRow.plagiarized_skill_id,
        skillName: plagSkillDef?.name || 'unknown',
        displayName: plagSkillDef?.displayName || 'Unknown',
        copiedLevel: charRow.plagiarized_skill_level,
        usableLevel: charRow.plagiarized_skill_level,
        icon: plagSkillDef?.icon || 'unknown'
    };
}
```

**E. Emit on join** — After player data is sent:
```javascript
if (player.plagiarizedSkill) {
    socket.emit('plagiarism:data', player.plagiarizedSkill);
}
```

**F. Save on disconnect** — In the disconnect handler:
```javascript
if (player.plagiarizedSkill) {
    await pool.query('UPDATE characters SET plagiarized_skill_id = $1, plagiarized_skill_level = $2 WHERE id = $3',
        [player.plagiarizedSkill.skillId, player.plagiarizedSkill.usableLevel, characterId]);
}
```

### Dependencies
None — DB migration system exists, player:join handler exists.

---

## Item 7: Intimidate Zone-Bounded Teleport (#20)

### What's Missing
Intimidate teleport uses hardcoded `±2000` UE. Zone registry has no explicit bounds.

### Workaround (no zone bounds needed)
Use zone's `defaultSpawn` as center point, with safe radius based on zone type:
```javascript
const zoneData = RO_ZONE_DATA[zone];
const spawn = zoneData?.defaultSpawn || { x: 0, y: 0 };
const safeRadius = 2000; // ~40 cells from spawn center
const angle = Math.random() * 2 * Math.PI;
const dist = Math.random() * safeRadius;
const randX = spawn.x + Math.cos(angle) * dist;
const randY = spawn.y + Math.sin(angle) * dist;
```

### Dependencies
None — `RO_ZONE_DATA` already imported and has `defaultSpawn` per zone.

---

## Item 8: Plagiarism PvE Copy from Monster Skills (BLOCKED)

### What's Needed
When a monster casts a player-class skill that hits a Rogue, `checkPlagiarismCopy()` should fire.

### Why It's Blocked
**No Monster Skill System exists.** Currently:
- Monsters only auto-attack (physical melee/ranged)
- No `enemy.learnedSkills`, no cast queues, no skill cooldowns
- No AI skill selection logic
- No monster skill database (`mob_skill_db` equivalent)

### What a Monster Skill System Would Require
This is a **large independent feature** (~500-1000 lines):

1. **Monster Skill Database** — `ro_monster_skills.js` mapping monster template IDs to skills:
   ```javascript
   // Example: Deviling (templateId 1582)
   { templateId: 1582, skills: [
       { skillId: 210, name: 'soul_strike', level: 9, rate: 500, castTime: 500, delay: 3000,
         condition: 'attack', conditionValue: null, target: 'target' },
   ]}
   ```

2. **AI Skill Selection** — In enemy AI ATTACK state:
   - Check available skills with cooldowns
   - Roll skill cast chance per tick
   - Priority: HP threshold skills > offensive skills > support skills

3. **Monster Cast Time** — Enemy enters CASTING state:
   - Interruptible by damage (if flag set)
   - Broadcasts `enemy:casting` event for client cast bar

4. **Monster Skill Execution** — After cast completes:
   - Route to existing skill damage functions
   - Apply effects (damage, status, buffs)
   - Trigger `checkPlagiarismCopy()` on target player

5. **Monster Skill Data** — rAthena has `mob_skill_db.txt` with ~3000 entries. Need to port relevant ones.

### Can We Work Around It?
**Partially.** Without monster skills:
- Plagiarism's USAGE works (casting copied skills via `skill:use`) ✅
- Plagiarism's ASPD bonus works (+1%/lv) ✅
- Plagiarism's level cap works ✅
- Plagiarism's whitelist works ✅
- Plagiarism's DB persistence works (Item 6) ✅
- Plagiarism's copy from MONSTERS does NOT work ❌
- Plagiarism's copy from PvP will work once PvP exists ❌ (also blocked)

**Workaround option: Admin/Debug command** — Add a `plagiarism:set` socket event that lets a player manually set their copied skill (for testing):
```javascript
socket.on('plagiarism:set', ({ skillId, level }) => {
    // Validate skill is in whitelist, cap level
    checkPlagiarismCopy(player, characterId, skill.name, skillId, level);
});
```
This allows testing the usage/persistence without the monster skill system.

### Recommendation
**Defer monster skill system** — it's a major feature unrelated to Rogue skill completeness. Implement Items 1-7 now, add the Plagiarism PvE hook when monster skills are built. The copy USAGE pipeline is fully functional.

---

## Implementation Order

```
Phase 1 (5 min):  Item 5 — Snatcher inventory emit (trivial)
Phase 2 (15 min): Item 1 — Close Confine break conditions (most impactful)
Phase 3 (5 min):  Item 2 — Close Confine mutual expiry (depends on Phase 2)
Phase 4 (10 min): Item 3 — Raid debuff hit counter in skill paths
Phase 5 (10 min): Item 4 — Tunnel Drive server speed enforcement
Phase 6 (10 min): Item 7 — Intimidate zone-bounded teleport
Phase 7 (15 min): Item 6 — Plagiarism DB persistence
Phase 8 (DEFER):  Item 8 — Plagiarism PvE copy (blocked by monster skill system)
```

**Total: ~70 minutes for Items 1-7.**

---

## Limitations & Barriers

| Barrier | Impact | Workaround |
|---------|--------|------------|
| No Monster Skill System | Plagiarism can't copy from monsters | Defer; usage pipeline works; add debug command |
| No PvP System | Plagiarism can't copy from players | Defer; blocked by PvP, not Rogue skills |
| No Zone Bounds | Intimidate teleport unclamped | Use defaultSpawn ± safeRadius |
| Client-only Tunnel Drive | Speed exploitable | Add server enforcement (Item 4) |
| No `lastPositionTime` tracking | Tunnel Drive speed calc | Add it in position handler |
| Raid debuff source disagreement | 20%/5s/7hits vs 30%/10s | Using Fandom wiki values (pre-renewal); matches rAthena pre-re DB |

---

## After Implementation — Rogue Skill Completion Status

| ID | Skill | Status After All Fixes |
|----|-------|----------------------|
| 1700 | Snatcher | 100% — auto-steal + inventory emit |
| 1701 | Back Stab | 100% — verified all mechanics |
| 1702 | Tunnel Drive | 100% — server-enforced speed |
| 1703 | Raid | 100% — 3x3 AoE, pre-renewal debuff, hit counter everywhere |
| 1704 | Intimidate | 100% — zone-bounded teleport, miss check |
| 1705 | Sword Mastery | 100% — already correct |
| 1706 | Vulture's Eye | 100% — already correct |
| 1707 | Double Strafe (R) | 100% — cooldown fixed |
| 1708 | Remove Trap | DEFERRED — needs Hunter trap system |
| 1709 | Steal Coin | 100% — aggro, boss check, zeny persist |
| 1710 | Divest Helm | 100% — already correct |
| 1711 | Divest Shield | 100% — already correct |
| 1712 | Divest Armor | 100% — SP fixed |
| 1713 | Divest Weapon | 100% — SP fixed |
| 1714 | Plagiarism | 95% — USAGE works, ASPD works, persist works, PvE COPY blocked |
| 1715 | Gangster's Paradise | DEFERRED — needs sitting system |
| 1716 | Compulsion Discount | 100% — already correct |
| 1717 | Scribble | DEFERRED — cosmetic |
| 1718 | Close Confine | 100% — all break conditions, mutual expiry, enemy AI block |

**16/19 skills at 100%.** 1 at 95% (Plagiarism copy trigger deferred). 3 deferred (Remove Trap, Gangster's Paradise, Scribble).

---
---

# APPENDIX: Monster Skill System — Full Implementation Plan

**Date:** 2026-03-17
**Purpose:** Unblock Plagiarism PvE copy + add RO Classic monster skill casting for all 509 monsters
**Scope:** New `ro_monster_skills.js` data module + AI skill selection + skill execution pipeline
**Blocked by:** Nothing — all infrastructure exists, this is greenfield feature work

---

## Executive Summary

The Monster Skill System is the **only remaining blocker** for Plagiarism PvE copy. It is also a major game feature in its own right — in RO Classic, most monsters above level 20 cast skills, and boss monsters rely heavily on skill rotations (Heal, Teleport, AoE attacks, summon slaves, etc.).

**Effort estimate:** ~600-800 lines of new code across 2 files + data porting.
**No external blockers.** All required infrastructure exists:
- AI state machine with IDLE/CHASE/ATTACK states ✅
- 200ms AI tick loop ✅
- Damage calculation functions (physical + magical) ✅
- Buff/debuff application system ✅
- Ground effect system ✅
- Status effect system ✅
- `checkPlagiarismCopy()` function ready to call ✅

---

## Architecture Overview

### rAthena Reference (mob_skill_db.txt format)

Each monster skill entry in rAthena has these fields:
```
MobID, InfoDummy, State, SkillID, SkillLv, Rate, CastTime, Delay, Cancelable, Target,
ConditionType, ConditionValue, val1, val2, val3, val4, val5, Emotion, Chat
```

**Sources:** [rAthena mob_skill_db docs](https://github.com/rathena/rathena/blob/master/db/pre-re/mob_skill_db.txt), [DeepWiki Monster System](https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)

### Our Equivalent Design

```
ro_monster_skills.js — Data file mapping monster template IDs to skill entries
index.js — Skill selection in AI tick + skill execution functions
```

---

## Data Format: `ro_monster_skills.js`

### Per-Monster Skill Entry
```javascript
module.exports = {
    // MobID → array of skill rules (evaluated top-to-bottom, first match wins)
    1002: [ // Poring
        // No skills — Poring is a basic melee mob
    ],
    1014: [ // Spore
        {
            skillId: 28,           // NPC_WATERATTACK (elemental melee)
            skillName: 'NPC_WATERATTACK',
            level: 1,
            state: 'attack',       // Only use while in ATTACK state
            rate: 2000,            // 20% chance per evaluation (of 10000)
            castTime: 0,           // Instant
            delay: 5000,           // 5s cooldown before reuse
            cancelable: true,      // Can be interrupted by damage
            target: 'target',      // Current attack target
            condition: 'always',   // No special condition
            conditionValue: 0,
            isPlayerSkill: false,  // Monster-only skill — NOT copyable by Plagiarism
            element: 'water',      // Override element for this attack
            damageMultiplier: 100, // 100% ATK
        },
    ],
    1059: [ // Argos
        {
            skillId: 28,           // NPC_WATERATTACK
            skillName: 'NPC_WATERATTACK',
            level: 3,
            state: 'attack',
            rate: 2000,
            castTime: 0,
            delay: 5000,
            cancelable: true,
            target: 'target',
            condition: 'always',
            conditionValue: 0,
            isPlayerSkill: false,
            element: 'water',
            damageMultiplier: 100,
        },
    ],
    1038: [ // Osiris (MVP)
        {
            skillId: 26,           // AL_TELEPORT — self-teleport when rude attacked
            skillName: 'AL_TELEPORT',
            level: 1,
            state: 'idle',
            rate: 10000,           // 100% when condition met
            castTime: 0,
            delay: 0,
            cancelable: true,
            target: 'self',
            condition: 'rudeattacked',
            conditionValue: 0,
            isPlayerSkill: true,   // Player skill — copyable by Plagiarism
            effect: 'teleport_self', // Special handler
        },
        {
            skillId: 29,           // AL_HEAL — self-heal when HP < 50%
            skillName: 'AL_HEAL',
            level: 11,             // Power skill: Lv11 exceeds player max
            state: 'attack',
            rate: 5000,
            castTime: 1000,
            delay: 10000,
            cancelable: true,
            target: 'self',
            condition: 'myhpltmaxrate',
            conditionValue: 50,    // HP < 50%
            isPlayerSkill: true,
            effect: 'heal_self',
        },
    ],
    // ... 509 total monsters, only ~200-300 have skills
};
```

### Monster-Only Skill Definitions (NPC_* skills)

These are skills that ONLY monsters can use. They are NOT in the player SKILL_MAP and NOT copyable by Plagiarism.

```javascript
const NPC_SKILLS = {
    // Elemental melee attacks (replace auto-attack with element)
    184: { name: 'NPC_WATERATTACK', element: 'water', type: 'elemental_melee', multiplier: 100 },
    186: { name: 'NPC_FIREATTACK', element: 'fire', type: 'elemental_melee', multiplier: 100 },
    187: { name: 'NPC_WINDATTACK', element: 'wind', type: 'elemental_melee', multiplier: 100 },
    188: { name: 'NPC_GROUNDATTACK', element: 'earth', type: 'elemental_melee', multiplier: 100 },
    189: { name: 'NPC_POISONATTACK', element: 'poison', type: 'elemental_melee', multiplier: 100 },
    190: { name: 'NPC_HOLYATTACK', element: 'holy', type: 'elemental_melee', multiplier: 100 },
    191: { name: 'NPC_DARKNESSATTACK', element: 'dark', type: 'elemental_melee', multiplier: 100 },
    192: { name: 'NPC_TELEKINESISATTACK', element: 'ghost', type: 'elemental_melee', multiplier: 100 },
    193: { name: 'NPC_UNDEADATTACK', element: 'undead', type: 'elemental_melee', multiplier: 100 },

    // Status melee attacks (auto-attack + status chance)
    176: { name: 'NPC_POISON', type: 'status_melee', status: 'poison', chance: 20, duration: 60000 },
    177: { name: 'NPC_BLINDATTACK', type: 'status_melee', status: 'blind', chance: 20, duration: 30000 },
    178: { name: 'NPC_SILENCEATTACK', type: 'status_melee', status: 'silence', chance: 20, duration: 30000 },
    179: { name: 'NPC_STUNATTACK', type: 'status_melee', status: 'stun', chance: 20, duration: 5000 },
    180: { name: 'NPC_PETRIFYATTACK', type: 'status_melee', status: 'stone', chance: 5, duration: 20000 },
    181: { name: 'NPC_CURSEATTACK', type: 'status_melee', status: 'curse', chance: 20, duration: 30000 },
    182: { name: 'NPC_SLEEPATTACK', type: 'status_melee', status: 'sleep', chance: 20, duration: 30000 },
    195: { name: 'NPC_RANDOMATTACK', type: 'random_element', multiplier: 100 },

    // Special monster abilities
    171: { name: 'NPC_COMBOATTACK', type: 'multi_hit', hits: 2, multiplier: 100 },
    196: { name: 'NPC_SPEEDUP', type: 'self_buff', effect: 'speed_boost', duration: 10000 },
    197: { name: 'NPC_CRITICALSLASH', type: 'forced_crit', multiplier: 100 },
    198: { name: 'NPC_EMOTION', type: 'emote', effect: 'display_emote' },
    199: { name: 'NPC_SELFDESTRUCTION', type: 'self_destruct', multiplier: 400 },
    653: { name: 'NPC_EARTHQUAKE', type: 'aoe_damage', radius: 750, multiplier: 500, element: 'neutral' },

    // Utility
    26:  { name: 'AL_TELEPORT_MOB', type: 'teleport_self' },  // Monsters using AL_TELEPORT
    485: { name: 'NPC_SUMMONSLAVE', type: 'summon', effect: 'spawn_slaves' },
    304: { name: 'NPC_METAMORPHOSIS', type: 'transform', effect: 'change_form' },
    660: { name: 'NPC_DARKBREATH', type: 'ranged_damage', element: 'dark', multiplier: 500 },
    661: { name: 'NPC_DARKBLESSING', type: 'status_ranged', status: 'curse', chance: 50 },
    687: { name: 'NPC_BLOODDRAIN', type: 'drain', effect: 'hp_drain', drainPercent: 5 },
    688: { name: 'NPC_ENERGYDRAIN', type: 'drain', effect: 'sp_drain', drainPercent: 5 },
};
```

---

## AI Skill Selection Logic

### State Mapping (rAthena → Our System)

| rAthena State | Our AI_STATE | When |
|---------------|-------------|------|
| `idle` | IDLE | Standing/wandering, no target |
| `walk` | IDLE (wandering) | Moving between positions |
| `chase` | CHASE | Pursuing target |
| `attack` | ATTACK | In range, auto-attacking |
| `angry` | ATTACK (first aggro) | Provoked but attacker hasn't been hit yet |
| `follow` | CHASE (no damage) | Following without being attacked |
| `anytarget` | ATTACK + CHASE | Any state with a valid target |
| `dead` | DEAD | On kill — used for on-death skills |

### Skill Evaluation Flow (per AI tick, 200ms)

```
For each alive enemy with skills:
  1. Get current AI state
  2. Filter skills matching current state (or 'any'/'anytarget')
  3. For each matching skill (top to bottom):
     a. Check cooldown: skip if (now - lastUsed[skillIndex]) < delay
     b. Check condition: evaluate condition type against current state
     c. Roll rate: if (random(0, 10000) < skill.rate) → USE THIS SKILL
     d. If rate passes → enter CASTING state or execute immediately
  4. If no skill selected → proceed with normal auto-attack
```

### Condition Evaluation Functions

```javascript
function evaluateMonsterSkillCondition(enemy, skill, target) {
    switch (skill.condition) {
        case 'always':
            return true;

        case 'myhpltmaxrate':
            // HP < conditionValue% of max
            return (enemy.health / enemy.maxHealth * 100) < skill.conditionValue;

        case 'myhpinrate':
            // HP between conditionValue% and val1%
            const hpPct = enemy.health / enemy.maxHealth * 100;
            return hpPct >= skill.conditionValue && hpPct <= (skill.val1 || 100);

        case 'mystatuson':
            // Has specific status effect active
            return hasStatusEffect(enemy, skill.conditionValue);

        case 'mystatusoff':
            // Does NOT have specific status effect
            return !hasStatusEffect(enemy, skill.conditionValue);

        case 'friendhpltmaxrate':
            // A nearby ally has HP < conditionValue%
            return findNearbyAllyBelowHP(enemy, skill.conditionValue) !== null;

        case 'closedattacked':
            // Was hit by melee recently
            return enemy._lastHitType === 'melee';

        case 'longrangeattacked':
            // Was hit by ranged recently
            return enemy._lastHitType === 'ranged';

        case 'skillused':
            // A specific skill was used on this monster
            return enemy._lastSkillHitId === skill.conditionValue;

        case 'casttargeted':
            // A player is casting at this monster
            return enemy._isCastTarget === true;

        case 'rudeattacked':
            // Monster attacked while out of range / can't reach
            return enemy._rudeAttacked === true;

        case 'onspawn':
            // Just spawned
            return enemy._justSpawned === true;

        case 'slavelt':
            // Slave count < conditionValue
            return (enemy._slaveCount || 0) < skill.conditionValue;

        case 'afterskill':
            // Just used skill X
            return enemy._lastSkillUsed === skill.conditionValue;

        case 'attackpcgt':
            // More than N players attacking
            return (enemy.inCombatWith?.size || 0) > skill.conditionValue;

        default:
            return false;
    }
}
```

---

## Skill Execution Pipeline

### New AI State: CASTING (optional)

For skills with castTime > 0, add a CASTING sub-state:
```javascript
// Add to enemy object on skill cast start:
enemy._casting = {
    skillEntry: skill,          // The skill rule being cast
    targetId: targetCharId,     // Target for the skill
    startTime: Date.now(),
    castTime: skill.castTime,   // Duration in ms
    cancelable: skill.cancelable,
    interrupted: false,
};
```

During CASTING:
- Enemy cannot auto-attack or move
- If `cancelable` and enemy takes damage → set `interrupted = true`, cancel cast
- When `now >= startTime + castTime` → execute skill
- Broadcast `enemy:casting` event for client cast bar display

For skills with castTime === 0: execute immediately (no CASTING state).

### Skill Execution by Type

```javascript
async function executeMonsterSkill(enemy, skill, targetId, zone, io) {
    const target = connectedPlayers.get(targetId);
    if (!target && skill.target === 'target') return;

    // Track cooldown
    if (!enemy._skillCooldowns) enemy._skillCooldowns = {};
    enemy._skillCooldowns[skill.skillId] = Date.now();

    // Broadcast skill use event (for client VFX/animation)
    broadcastToZone(zone, 'enemy:skill_used', {
        enemyId: enemy.enemyId, enemyName: enemy.name,
        skillId: skill.skillId, skillName: skill.skillName,
        skillLevel: skill.level, targetId,
        targetX: target?.lastX, targetY: target?.lastY,
    });

    // Route by skill type
    if (skill.isPlayerSkill) {
        // PLAYER SKILL used by monster — route through existing skill handlers
        await executeMonsterPlayerSkill(enemy, skill, targetId, zone, io);
    } else {
        // NPC_ SKILL — monster-only, use NPC_SKILLS lookup
        const npcSkill = NPC_SKILLS[skill.skillId];
        if (!npcSkill) return;
        await executeNPCSkill(enemy, npcSkill, skill, targetId, zone, io);
    }
}
```

### Player Skill Execution by Monsters

When monsters use player-class skills (Fire Bolt, Heal, Water Ball, etc.):

```javascript
async function executeMonsterPlayerSkill(enemy, skillEntry, targetId, zone, io) {
    const target = connectedPlayers.get(targetId);
    const skillDef = SKILL_MAP.get(skillEntry.skillId);
    if (!skillDef) return;

    // Calculate damage using existing functions
    const attackerStats = {
        str: enemy.stats?.str || 1, agi: enemy.stats?.agi || 1,
        vit: enemy.stats?.vit || 0, int: enemy.stats?.int || 0,
        dex: enemy.stats?.dex || 1, luk: enemy.stats?.luk || 1,
        level: enemy.level,
        weaponATK: enemy.damage || 1,
        // Monsters have no weapon MATK — use INT-based MATK
        matkMin: (enemy.stats?.int || 0),
        matkMax: (enemy.stats?.int || 0) + Math.floor(Math.pow(Math.floor((enemy.stats?.int || 0) / 5), 2)),
    };

    const levelData = skillDef.levels[Math.min(skillEntry.level - 1, skillDef.levels.length - 1)];
    const effectVal = levelData?.effectValue || 100;

    // Physical or Magical?
    const isMagic = ['cold_bolt','fire_bolt','lightning_bolt','fire_ball','thunderstorm',
        'napalm_beat','soul_strike','frost_diver','earth_spike','fire_pillar',
        'jupitel_thunder','lord_of_vermilion','meteor_storm','storm_gust',
        'water_ball','frost_nova','heavens_drive','sight_rasher','heal',
        'holy_light','magnus_exorcismus','turn_undead','sanctuary'].includes(skillDef.name);

    let damage = 0;
    let element = skillDef.element || enemy.element?.type || 'neutral';
    let isMiss = false;

    if (isMagic) {
        const magicResult = calculateMagicSkillDamage(
            attackerStats, getEffectiveStats(target), target.hardDef || 0,
            effectVal, element, getPlayerTargetInfo(target, targetId)
        );
        damage = magicResult.damage;
        isMiss = magicResult.isMiss;
    } else {
        const physResult = calculateSkillDamage(
            attackerStats, getEffectiveStats(target), target.hardDef || 0,
            effectVal, getBuffStatModifiers(enemy), getBuffStatModifiers(target),
            getPlayerTargetInfo(target, targetId),
            { weaponElement: element, race: enemy.race, size: enemy.size }
        );
        damage = physResult.damage;
        isMiss = physResult.isMiss;
    }

    // Apply damage to player
    if (!isMiss && damage > 0) {
        target.health = Math.max(0, target.health - damage);
    }

    // Broadcast skill:effect_damage
    broadcastToZone(zone, 'skill:effect_damage', {
        attackerId: enemy.enemyId, attackerName: enemy.name,
        targetId, targetName: target.characterName, isEnemy: false,
        skillId: skillEntry.skillId, skillName: skillDef.displayName,
        skillLevel: skillEntry.level, element,
        damage: isMiss ? 0 : damage, isMiss, isCritical: false, hitType: 'skill',
        targetHealth: target.health, targetMaxHealth: target.maxHealth,
        timestamp: Date.now()
    });

    // *** PLAGIARISM HOOK — This is where the copy triggers ***
    if (!isMiss) {
        checkPlagiarismCopy(target, targetId, skillDef.name, skillEntry.skillId, skillEntry.level);
    }

    // Health update
    const tSocket = io.sockets.sockets.get(target.socketId);
    if (tSocket) {
        tSocket.emit('combat:health_update', {
            characterId: targetId, health: target.health, maxHealth: target.maxHealth,
            mana: target.mana, maxMana: target.maxMana
        });
    }

    // Check player death
    if (target.health <= 0) {
        // Trigger player death handler
    }
}
```

### NPC Skill Execution (Monster-Only)

```javascript
async function executeNPCSkill(enemy, npcDef, skillEntry, targetId, zone, io) {
    const target = connectedPlayers.get(targetId);

    switch (npcDef.type) {
        case 'elemental_melee': {
            // Same as auto-attack but with forced element
            const result = calculateEnemyDamage(enemy, targetId);
            if (result && !result.isMiss) {
                result.element = npcDef.element; // Force element
                // Apply element modifier
                // ... damage application ...
            }
            break;
        }
        case 'status_melee': {
            // Auto-attack + status roll
            const result = calculateEnemyDamage(enemy, targetId);
            // Apply damage, then roll status
            if (result && !result.isMiss && target.health > 0) {
                const statusResult = applyStatusEffect(enemy, target, npcDef.status,
                    npcDef.chance, { duration: npcDef.duration });
                if (statusResult?.applied) {
                    broadcastToZone(zone, 'status:applied', { ... });
                }
            }
            break;
        }
        case 'self_buff': {
            applyBuff(enemy, { name: npcDef.effect, duration: npcDef.duration, ... });
            break;
        }
        case 'teleport_self': {
            // Random teleport within zone (Osiris, etc.)
            const spawn = RO_ZONE_DATA[zone]?.defaultSpawn || { x: 0, y: 0 };
            enemy.x = spawn.x + (Math.random() - 0.5) * 2000;
            enemy.y = spawn.y + (Math.random() - 0.5) * 2000;
            broadcastToZone(zone, 'enemy:move', { enemyId: enemy.enemyId, x: enemy.x, y: enemy.y, z: enemy.z, teleport: true });
            break;
        }
        case 'summon': {
            // Spawn slave monsters — deferred (needs slave tracking)
            break;
        }
        case 'aoe_damage': {
            // NPC_EARTHQUAKE etc. — damage all players in radius
            for (const [charId, player] of connectedPlayers.entries()) {
                if (player.zone !== zone || player.isDead) continue;
                const dist = calculateDistance({ x: enemy.x, y: enemy.y }, { x: player.lastX, y: player.lastY });
                if (dist <= npcDef.radius) {
                    // Calculate and apply damage
                }
            }
            break;
        }
        // ... more types as needed
    }
}
```

---

## Implementation Phases

### Phase M1: Data Module (NEW file — `ro_monster_skills.js`)
**Effort:** 2-3 hours for initial data porting
**Blocked by:** Nothing

1. Create `server/src/ro_monster_skills.js`
2. Define `NPC_SKILLS` constant with all monster-only skill definitions (~40 NPC_ skills)
3. Port monster skill entries from rAthena `pre-re/mob_skill_db.txt` for zones 1-3 monsters
   - Start with the ~46 active spawn point monsters
   - Add boss/MVP skills for Osiris, Baphomet, etc.
4. Export `MONSTER_SKILL_DB` (mobId → skill array) and `NPC_SKILLS`

**Priority monsters** (zones 1-3, 46 spawn points):
- Zone 1 (Prontera South): Poring, Lunatic, Fabre, Chonchon, Roda Frog, Willow
- Zone 2 (Payon Cave): Zombie, Skeleton, Familiar, Poporing, Munak
- Zone 3 (Geffen Field): Poison Spore, Hornet, Mandragora, Rocker

Most low-level monsters have 0-2 skills (usually NPC_WATERATTACK or similar elemental melee). Mid-level and boss monsters have 3-10+ skills.

### Phase M2: AI Skill Selection (in `index.js`)
**Effort:** 1-2 hours
**Blocked by:** Phase M1

1. Import `MONSTER_SKILL_DB` and `NPC_SKILLS` at top of index.js
2. Add `enemy._skillCooldowns = {}` to enemy spawn
3. Add `enemy._lastHitType`, `enemy._lastSkillHitId`, `enemy._rudeAttacked`, `enemy._justSpawned` flags
4. Add `evaluateMonsterSkillCondition()` function
5. In the ATTACK state handler (line ~23609), BEFORE `calculateEnemyDamage()`:
   ```
   // Check for monster skill use before auto-attack
   const monsterSkills = MONSTER_SKILL_DB[enemy.templateId];
   if (monsterSkills && monsterSkills.length > 0) {
       const selectedSkill = selectMonsterSkill(enemy, monsterSkills, 'attack');
       if (selectedSkill) {
           await executeMonsterSkill(enemy, selectedSkill, enemy.targetPlayerId, zone, io);
           continue; // Skip auto-attack this tick
       }
   }
   ```
6. Add similar checks in IDLE state (for idle-condition skills like Teleport)
7. Add similar checks in CHASE state (for chase-condition skills)

### Phase M3: Skill Execution Functions (in `index.js`)
**Effort:** 2-3 hours
**Blocked by:** Phase M2

1. Add `executeMonsterSkill()` — routing function
2. Add `executeMonsterPlayerSkill()` — for player-class skills used by monsters
3. Add `executeNPCSkill()` — for monster-only NPC_ skills
4. Add Plagiarism hook in `executeMonsterPlayerSkill()`:
   ```javascript
   checkPlagiarismCopy(target, targetId, skillDef.name, skillEntry.skillId, skillEntry.level);
   ```
5. Add `enemy:skill_used` socket event emission
6. Add `enemy:casting` socket event for cast bar display (cast time > 0)

### Phase M4: Casting System (in `index.js`)
**Effort:** 1 hour
**Blocked by:** Phase M3

1. Add `enemy._casting` state tracking
2. In AI tick, check for active cast:
   - If casting and time elapsed → execute skill
   - If casting and cancelable and took damage → interrupt
   - If casting → skip auto-attack and movement
3. Broadcast `enemy:casting` / `enemy:cast_interrupted` events

### Phase M5: Initial Data Population
**Effort:** 3-5 hours (data entry)
**Blocked by:** Phase M1 format finalized

1. Port zone 1-3 monsters from rAthena `pre-re/mob_skill_db.txt` (~50 monsters)
2. Port all MVP/boss monsters that players encounter (~20 bosses)
3. Add player-class skills used by common monsters:
   - Fire Bolt (many fire monsters)
   - Water Ball (aquatic monsters)
   - Heal (Angel-type, Priest-type)
   - Thunderstorm (wind monsters)
   - etc.
4. Validate `isPlayerSkill` flag is correct for Plagiarism compatibility

---

## Implementation Order (Combined with Items 1-7)

```
=== PHASE 1: Rogue Remaining Items (Items 1-7) ===
Step 1 (5 min):   Item 5 — Snatcher inventory emit
Step 2 (15 min):  Item 1 — Close Confine break conditions
Step 3 (5 min):   Item 2 — Close Confine mutual expiry
Step 4 (10 min):  Item 3 — Raid debuff hit counter in skill paths
Step 5 (10 min):  Item 4 — Tunnel Drive server speed enforcement
Step 6 (10 min):  Item 7 — Intimidate zone-bounded teleport
Step 7 (15 min):  Item 6 — Plagiarism DB persistence

=== PHASE 2: Monster Skill System ===
Step 8 (2-3 hr):  Phase M1 — ro_monster_skills.js data module
Step 9 (1-2 hr):  Phase M2 — AI skill selection in ATTACK/IDLE/CHASE
Step 10 (2-3 hr): Phase M3 — Skill execution functions + Plagiarism hook
Step 11 (1 hr):   Phase M4 — Casting system (castTime > 0)
Step 12 (3-5 hr): Phase M5 — Data population (zone 1-3 + bosses)

=== RESULT ===
Plagiarism PvE copy: FULLY FUNCTIONAL
All 19 Rogue skills: 16 at 100%, 1 at 100% (Plagiarism now complete)
3 properly deferred (Remove Trap, Gangster's Paradise, Scribble)
```

---

## Limitations & Barriers

| Item | Status | Notes |
|------|--------|-------|
| AI infrastructure | ✅ EXISTS | IDLE/CHASE/ATTACK states + 200ms tick |
| Damage formulas | ✅ EXISTS | calculatePhysicalDamage + calculateMagicSkillDamage |
| Buff/status system | ✅ EXISTS | applyBuff, applyStatusEffect, removeBuff |
| Ground effects | ✅ EXISTS | createGroundEffect for Storm Gust, Sanctuary, etc. |
| Plagiarism hook | ✅ EXISTS | checkPlagiarismCopy() + whitelist + usage handler |
| Slave summoning | ⚠️ PARTIAL | Need spawn-by-master tracking (MVP slaves) |
| Metamorphosis | ⚠️ PARTIAL | Need template swap on living enemy |
| Client cast bar | ⚠️ NEEDS UI | `enemy:casting` event exists pattern but no client handler yet |
| All zone monsters | ⚠️ DATA WORK | Only zones 1-3 active; data porting is manual labor |
| Monster-exclusive skills | ✅ DESIGN READY | NPC_SKILLS constant covers ~40 common monster-only skills |

**Nothing is architecturally blocked.** The only cost is implementation time.

---

## Verification Checklist (RO Classic Pre-Renewal Compliance)

| Mechanic | rAthena Reference | Our Implementation |
|----------|-------------------|--------------------|
| Skill evaluation per AI tick | 100ms in rAthena, 200ms ours | Close enough; configurable |
| Rate out of 10000 | ✅ Same scale | Match |
| State-based skill triggers | idle/attack/chase/dead | Map to our AI_STATE |
| Condition types | 15+ types | Implement top 10 initially |
| Cast time + interruptible | Per-skill castTime, cancelable flag | Match rAthena format |
| Delay = reuse cooldown | Per-skill, tracked per enemy | _skillCooldowns Map |
| Player skills by monsters | Fire Bolt, Heal, etc. at power levels | Use SKILL_MAP + effectVal |
| Power skills (Lv > max) | Monsters cast Heal Lv11, etc. | Allow skillLevel > maxLevel |
| NPC_ skills not copyable | isPlayerSkill flag | Plagiarism whitelist check |
| Player skills copyable | Fire Bolt from monster → Rogue copies | checkPlagiarismCopy() hook |
| Multiple skills per monster | Evaluated top-to-bottom | Array order = priority |
| Boss/MVP skill rotations | HP thresholds, summon slaves, teleport | myhpltmaxrate + slavelt |

**Sources:**
- [rAthena pre-re/mob_skill_db.txt](https://github.com/rathena/rathena/blob/master/db/pre-re/mob_skill_db.txt)
- [rAthena Monster System - DeepWiki](https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)
- [rAthena Monster AI Types - Issue #926](https://github.com/rathena/rathena/issues/926)
- [rAthena mob_db_mode_list.txt](https://github.com/rathena/rathena/blob/master/doc/mob_db_mode_list.txt)
