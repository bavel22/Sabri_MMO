# 04 — Skill System Implementation Guide

> UE5.7 C++ + Node.js server implementation for Ragnarok Online Classic (Pre-Renewal) skill system.
> All code is production-ready and achievable by Claude Code without manual Blueprint input.
> Server: `server/src/index.js`. Client: `client/SabriMMO/Source/SabriMMO/`.

---

## Table of Contents

1. [Server-Side Skill Engine](#1-server-side-skill-engine)
2. [Adding a New Skill (Step-by-Step Template)](#2-adding-a-new-skill)
3. [Client-Side: Skill Targeting System](#3-client-side-skill-targeting-system)
4. [Client-Side: Cast Bar System](#4-client-side-cast-bar-system)
5. [Client-Side: Skill Tree UI](#5-client-side-skill-tree-ui)
6. [Client-Side: Skill VFX Integration](#6-client-side-skill-vfx-integration)
7. [Client-Side: Hotbar Integration](#7-client-side-hotbar-integration)
8. [Implementing Each Skill Category](#8-implementing-each-skill-category)

---

## 1. Server-Side Skill Engine

### 1.1 Skill Data Structure (`server/src/ro_skill_data.js`)

Every skill in the system is defined as a single object in the `SKILL_DEFINITIONS` array. The schema is:

```js
{
    id: 200,                          // Unique numeric ID (project-internal)
    name: 'cold_bolt',                // Snake_case internal name (used in switch blocks)
    displayName: 'Cold Bolt',         // Human-readable name (sent to client)
    classId: 'mage',                  // Which class tree this belongs to
    maxLevel: 10,                     // Maximum learnable level
    type: 'active',                   // 'active' | 'passive' | 'toggle'
    targetType: 'single',            // 'none' | 'self' | 'single' | 'ground' | 'aoe'
    element: 'water',                 // 'neutral'|'fire'|'water'|'wind'|'earth'|'holy'|'ghost'|'poison'|'undead'
    range: 900,                       // Max cast range in UE units (1 RO cell ~ 50 UE units)
    description: 'Water bolt magic.', // Tooltip text
    icon: 'cold_bolt',               // Icon asset name (resolved client-side)
    treeRow: 0,                       // Position in skill tree grid (row)
    treeCol: 0,                       // Position in skill tree grid (column)
    prerequisites: [                  // Array of required skills
        { skillId: 200, level: 5 }    // e.g., Cold Bolt Lv5 required
    ],
    levels: [                         // Per-level data array (index 0 = level 1)
        {
            level: 1,
            spCost: 12,               // SP consumed on execution (NOT during cast)
            castTime: 700,            // Base cast time in ms (before DEX reduction)
            afterCastDelay: 1000,     // Global skill lockout in ms after execution
            cooldown: 0,              // Per-skill cooldown in ms (0 = no cooldown)
            effectValue: 100,         // Skill-specific value (damage %, heal amount, buff value)
            duration: 0               // Buff/debuff duration in ms (0 = instant)
        }
    ]
}
```

**Key design points:**
- `castTime` is the **base** value in milliseconds before DEX reduction
- `afterCastDelay` (ACD) is a global lockout — no skills can be used during ACD
- `cooldown` is per-skill — only THIS skill is locked
- `effectValue` meaning varies by skill type (ATK%, MATK%, heal amount, buff value)
- SP is deducted at **execution time**, not when casting begins
- `levels` array is 0-indexed: `levels[learnedLevel - 1]` gets current level data

**Helper function for repetitive level scaling:**

```js
// Generate level array for skills with simple formulas
function genLevels(count, fn) {
    return Array.from({ length: count }, (_, i) => fn(i));
}

// Usage: Cold Bolt — SP scales 12+2*level, cast = 700*level ms
levels: genLevels(10, i => ({
    level: i + 1,
    spCost: 12 + i * 2,
    castTime: 700 * (i + 1),
    afterCastDelay: 800 + (i + 1) * 200,
    cooldown: 0,
    effectValue: 100,
    duration: 0
}))
```

**Lookup maps (built at module load):**

```js
const SKILL_MAP = new Map();           // skillId -> skill definition
for (const s of ALL_SKILLS) SKILL_MAP.set(s.id, s);

const CLASS_SKILLS = {};               // classId -> skill[]
for (const s of ALL_SKILLS) {
    if (!CLASS_SKILLS[s.classId]) CLASS_SKILLS[s.classId] = [];
    CLASS_SKILLS[s.classId].push(s);
}
```

**Class progression chains:**

```js
const CLASS_PROGRESSION = {
    'novice': ['novice'],
    'swordsman': ['novice', 'swordsman'],
    'mage': ['novice', 'mage'],
    'knight': ['novice', 'swordsman', 'knight'],
    // ... etc
};
```

### 1.2 Skill Execution Flow

The complete server-side flow when a client sends `skill:use`:

```
Client sends skill:use { skillId, targetId?, isEnemy?, groundX?, groundY?, groundZ? }
    |
    v
[1] VALIDATION PHASE
    - Player exists and is alive?
    - Not frozen/stoned/silenced? (CC check via getBuffStatModifiers)
    - Not already casting? (activeCasts.has(characterId))
    - Not in After-Cast Delay? (afterCastDelayEnd check)
    - Skill exists in SKILL_MAP?
    - Player has learned this skill? (learnedSkills[skillId] > 0)
    - Not on per-skill cooldown? (isSkillOnCooldown)
    - Enough SP? (player.mana >= spCost)
    |
    v
[2] TARGET RESOLUTION
    - Single target: use explicit targetId or fall back to autoAttackState
    - Ground target: parse groundX/Y/Z from payload
    - Self/AoE: no target needed (uses caster position)
    - PvP check: block player-targeting if PVP_ENABLED = false
    |
    v
[3] RANGE CHECK (pre-cast, only if not _castComplete)
    - Get caster position from DB/cache
    - Calculate distance to target or ground position
    - If out of range: emit combat:out_of_range, return
    |
    v
[4] CAST TIME CHECK
    - Get baseCastTime from levelData
    - If baseCastTime > 0 and not _castComplete:
        actualCastTime = baseCastTime * (1 - DEX/150)
        If DEX >= 150: actualCastTime = 0 (instant)
        If actualCastTime > 0:
            Store in activeCasts Map
            Broadcast skill:cast_start to zone
            Return (combat tick handles completion)
    - If baseCastTime = 0 or actualCastTime = 0: fall through to execute
    |
    v
[5] SKILL EFFECT EXECUTION
    - Deduct SP: player.mana -= spCost
    - Apply delays: applySkillDelays(characterId, player, skillId, levelData, socket)
    - Execute skill-specific handler (switch on skill.name)
    - Broadcast results to zone
    - Emit skill:used to caster
    |
    v
[6] POST-EXECUTION
    - Enemy death processing (EXP, drops, despawn)
    - Buff application/refresh
    - Health update broadcasts
```

### 1.3 Cast Time System

**Formula (Pre-Renewal):**

```js
function calculateActualCastTime(baseCastTime, casterDex) {
    if (baseCastTime <= 0) return 0;
    if (casterDex >= 150) return 0;  // Instant cast at 150 DEX
    return Math.max(0, Math.floor(baseCastTime * (1 - casterDex / 150)));
}
```

**Cast state storage:**

```js
// activeCasts: Map<characterId, CastState>
activeCasts.set(characterId, {
    skillId, targetId, isEnemy, learnedLevel, levelData, skill,
    castStartTime: now, castEndTime: now + actualCastTime, actualCastTime,
    socketId: socket.id, casterName: player.characterName,
    spCost, cooldownMs, effectVal, duration,
    groundX, groundY, groundZ, hasGroundPos
});
```

**Cast completion (in combat tick loop, 50ms interval):**

```js
// Inside the 50ms combat tick:
for (const [charId, cast] of activeCasts.entries()) {
    if (Date.now() >= cast.castEndTime) {
        activeCasts.delete(charId);
        await executeCastComplete(charId, cast);
    }
}
```

**Cast interruption:**

```js
function interruptCast(characterId, reason) {
    const cast = activeCasts.get(characterId);
    if (!cast) return;
    activeCasts.delete(characterId);
    // Notify caster
    const sock = io.sockets.sockets.get(cast.socketId);
    if (sock) sock.emit('skill:cast_interrupted', { skillId: cast.skillId, reason });
    // Notify zone (removes cast bar on other clients)
    broadcastToZone(zone, 'skill:cast_interrupted_broadcast', {
        casterId: characterId, skillId: cast.skillId
    });
}
```

**Interruption triggers:**
- Movement: called from `player:position` handler when position delta > 5 UE units
- Damage: called from all damage paths (auto-attack, skill damage, enemy attack)
- NOT interrupted by idle position updates or healing

**executeCastComplete re-validation:**

When the cast timer finishes, `executeCastComplete()` re-validates before executing:
1. Player still alive?
2. Still not CC'd (frozen/stoned)?
3. Still has enough SP? (SP deducted here, not at cast start)
4. Target still valid and in range?
5. If all pass: broadcast `skill:cast_complete`, then re-trigger `skill:use` handler with `_castComplete: true` flag to skip the cast time check and execute immediately.

### 1.4 Skill Type Handlers

Each skill type follows a specific pattern. All handlers live inside the `socket.on('skill:use')` handler and are dispatched by `skill.name`.

#### Single Target Offensive (Physical) — e.g., Bash

```js
if (skill.name === 'bash') {
    if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

    // 1. Resolve target (enemy or player)
    let target, targetPos, targetStats, targetHardDef, targetName;
    if (isEnemy) {
        target = enemies.get(targetId);
        // ... validate alive, get stats
    } else {
        target = connectedPlayers.get(targetId);
        // ... validate alive, get stats
    }

    // 2. Range check (melee range for Bash)
    const dist = calculateDistance(attackerPos, targetPos);
    if (dist > skill.range + COMBAT.RANGE_TOLERANCE) {
        socket.emit('combat:out_of_range', { ... });
        return;
    }

    // 3. Deduct SP and apply delays
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // 4. Calculate damage (physical skill with multiplier)
    const result = calculateSkillDamage(
        getEffectiveStats(player), targetStats, targetHardDef,
        effectVal,  // ATK% from levels array
        getBuffStatModifiers(player), getBuffStatModifiers(target),
        getEnemyTargetInfo(target), getAttackerInfo(player)
    );

    // 5. Apply damage
    target.health = Math.max(0, target.health - result.damage);

    // 6. Trigger AI aggro
    if (isEnemy) setEnemyAggro(target, characterId, 'skill');

    // 7. Broadcast damage
    broadcastToZone(zone, 'skill:effect_damage', { ... });

    // 8. Check death
    if (target.health <= 0) await processEnemyDeathFromSkill(target, player, ...);

    // 9. Confirm to caster
    socket.emit('skill:used', { ... });
    socket.emit('combat:health_update', { ... });
}
```

#### Single Target Offensive (Magical, Multi-Hit) — e.g., Bolt Skills

```js
if (skill.name === 'cold_bolt' || skill.name === 'fire_bolt' || skill.name === 'lightning_bolt') {
    // 1. Resolve and validate target (same as above)
    // 2. Range check (magic range = 900)
    // 3. Deduct SP and apply delays

    // 4. Calculate per-hit damage (hits = skill level)
    const numHits = learnedLevel;
    const hitDamages = [];
    let totalDamage = 0;
    for (let h = 0; h < numHits; h++) {
        const hitResult = calculateMagicSkillDamage(
            getEffectiveStats(player), targetStats, targetHardMdef,
            100,  // 100% MATK per hit (effectValue)
            skill.element, magicTargetInfo
        );
        hitDamages.push(hitResult.damage);
        totalDamage += hitResult.damage;
    }

    // 5. Apply total damage at once (gameplay damage is instant)
    target.health = Math.max(0, target.health - totalDamage);

    // 6. Fire breaks Frozen status
    if (skill.element === 'fire' && targetBuffMods.isFrozen) {
        target.activeBuffs = target.activeBuffs.filter(b => b.name !== 'frozen');
        broadcastToZone(zone, 'skill:buff_removed', {
            targetId, isEnemy, buffName: 'frozen', reason: 'fire_damage'
        });
    }

    // 7. Summary event (for subsystems that track total damage)
    broadcastToZone(zone, 'skill:effect_damage', {
        damage: totalDamage, hits: numHits, ...
    });

    // 8. Per-hit damage numbers (staggered 200ms for RO-style display)
    const HIT_DELAY_MS = 200;
    for (let h = 0; h < numHits; h++) {
        setTimeout(() => {
            broadcastToZone(zone, 'skill:effect_damage', {
                damage: hitDamages[h], hitNumber: h + 1, totalHits: numHits, ...
            });
        }, h * HIT_DELAY_MS);
    }

    // 9. Death check, skill:used, health_update
}
```

#### Single Target Supportive — e.g., Heal

Heal targets self or ally. Against Undead element enemies, it deals Holy damage instead.

```js
if (skill.name === 'heal') {
    // Heal formula: floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)
    const effectiveStats = getEffectiveStats(player);
    const baseHeal = Math.floor((effectiveStats.level + effectiveStats.int) / 8)
                     * (4 + 8 * learnedLevel);

    if (isEnemy && targetIsUndead) {
        // Deal Holy damage = 50% of heal amount
        const damage = Math.floor(baseHeal * 0.5);
        target.health = Math.max(0, target.health - damage);
        broadcastToZone(zone, 'skill:effect_damage', {
            damage, hitType: 'magical', element: 'holy', ...
        });
    } else {
        // Heal target (self or ally)
        const healTarget = targetId ? connectedPlayers.get(targetId) : player;
        const healed = Math.min(baseHeal, healTarget.maxHealth - healTarget.health);
        healTarget.health = Math.min(healTarget.maxHealth, healTarget.health + baseHeal);
        broadcastToZone(zone, 'skill:effect_damage', {
            damage: 0, hitType: 'heal', healAmount: healed, ...
        });
    }
}
```

#### Self Buff — e.g., Endure

```js
if (skill.name === 'endure') {
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const buffDuration = duration || (10000 + learnedLevel * 3000);
    const buffDef = {
        skillId, name: 'endure',
        casterId: characterId, casterName: player.characterName,
        mdefBonus: effectVal, defReduction: 0, atkIncrease: 0,
        duration: buffDuration
    };
    applyBuff(player, buffDef);

    broadcastToZone(zone, 'skill:buff_applied', {
        targetId: characterId, targetName: player.characterName, isEnemy: false,
        casterId: characterId, casterName: player.characterName,
        skillId, buffName: 'Endure', duration: buffDuration,
        effects: { mdefBonus: effectVal }
    });

    socket.emit('skill:used', { ... });
}
```

#### Ground AoE — e.g., Thunderstorm

```js
if (skill.name === 'thunderstorm') {
    if (!hasGroundPos) {
        socket.emit('skill:error', { message: 'No ground position' });
        return;
    }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const AOE_RADIUS = 250;  // 5x5 cells
    const numHits = learnedLevel;

    // Find all enemies in AoE radius around ground position
    const targetsInRange = [];
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead) continue;
        const dist = calculateDistance({ x: groundX, y: groundY }, { x: enemy.x, y: enemy.y });
        if (dist <= AOE_RADIUS) targetsInRange.push({ id: eid, enemy });
    }

    // Calculate and apply damage per target
    for (const { id: eid, enemy } of targetsInRange) {
        let totalDmg = 0;
        const hitDamages = [];
        for (let h = 0; h < numHits; h++) {
            const result = calculateMagicSkillDamage(
                getEffectiveStats(player), enemy.stats, enemy.hardMdef || 0,
                80, 'wind', getEnemyTargetInfo(enemy)  // 80% MATK per hit
            );
            hitDamages.push(result.damage);
            totalDmg += result.damage;
        }

        enemy.health = Math.max(0, enemy.health - totalDmg);
        setEnemyAggro(enemy, characterId, 'skill');

        // Staggered per-hit damage events (300ms between strikes for AoE)
        for (let h = 0; h < numHits; h++) {
            setTimeout(() => {
                broadcastToZone(zone, 'skill:effect_damage', {
                    damage: hitDamages[h], hitNumber: h + 1, totalHits: numHits,
                    groundX, groundY, groundZ,
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z, ...
                });
            }, h * 300);
        }
    }

    socket.emit('skill:used', { ... });
}
```

#### Passive Skills — e.g., Sword Mastery

Passives have no handler in `skill:use`. They are read by `getPassiveSkillBonuses()`:

```js
function getPassiveSkillBonuses(player) {
    const bonuses = { bonusATK: 0, hpRegenBonus: 0, spRegenBonus: 0, bonusMDEF: 0 };
    const learned = player.learnedSkills || {};
    const wType = player.weaponType || null;

    // Sword Mastery (100): +4 ATK/level with daggers & 1H swords
    const smLv = learned[100] || 0;
    if (smLv > 0 && (wType === 'dagger' || wType === 'one_hand_sword')) {
        bonuses.bonusATK += smLv * 4;
    }

    // Double Attack (500): checked during auto-attack tick
    // Owl's Eye (300): +1 DEX/level (applied in stat calculation)
    // Improve Dodge (501): +3 FLEE/level (applied in stat calculation)

    return bonuses;
}
```

Passive bonuses are applied at different points:
- **Mastery ATK**: added in `calculatePhysicalDamage()` as flat bonus (bypasses DEF)
- **Stat bonuses** (Owl's Eye DEX): added in `getEffectiveStats()` before any calculation
- **Proc passives** (Double Attack): checked in the auto-attack combat tick loop
- **Regen bonuses**: added in the HP/SP regen tick interval

### 1.5 Buff/Debuff System

**Data structure:**

```js
// Each player/enemy has: target.activeBuffs = []
// Each buff entry:
{
    skillId: 104,                    // Which skill applied this
    name: 'provoke',                 // Internal buff name
    casterId: 12345,                 // Who applied it
    casterName: 'Player1',
    defReduction: 25,                // Skill-specific effect fields
    atkIncrease: 17,
    mdefBonus: 0,
    duration: 30000,                 // Duration in ms
    appliedAt: 1700000000000,        // Date.now() when applied
    expiresAt: 1700000030000         // Auto-calculated by applyBuff()
}
```

**Core buff functions:**

```js
// Apply or refresh a buff
function applyBuff(target, buffDef) {
    if (!target.activeBuffs) target.activeBuffs = [];
    // Remove existing buff of same skillId (refresh/overwrite)
    target.activeBuffs = target.activeBuffs.filter(b => b.skillId !== buffDef.skillId);
    target.activeBuffs.push({
        ...buffDef,
        appliedAt: Date.now(),
        expiresAt: Date.now() + buffDef.duration
    });
}

// Expire old buffs, returns array of expired entries
function expireBuffs(target) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) return [];
    const now = Date.now();
    const expired = target.activeBuffs.filter(b => now >= b.expiresAt);
    target.activeBuffs = target.activeBuffs.filter(b => now < b.expiresAt);
    return expired;
}

// Aggregate stat modifiers from all active buffs
function getBuffStatModifiers(target) {
    const mods = {
        defMultiplier: 1.0,
        atkMultiplier: 1.0,
        bonusMDEF: 0,
        isFrozen: false,
        isStoned: false,
        overrideElement: null
    };
    if (!target.activeBuffs) return mods;
    const now = Date.now();
    for (const buff of target.activeBuffs) {
        if (now >= buff.expiresAt) continue;
        if (buff.name === 'provoke') {
            mods.defMultiplier *= (1 - (buff.defReduction || 0) / 100);
            mods.atkMultiplier *= (1 + (buff.atkIncrease || 0) / 100);
        }
        if (buff.name === 'endure') {
            mods.bonusMDEF += buff.mdefBonus || 0;
        }
        if (buff.name === 'frozen') {
            mods.isFrozen = true;
            mods.overrideElement = { type: 'water', level: 1 };
        }
        if (buff.name === 'stone_curse') {
            mods.isStoned = true;
            mods.overrideElement = { type: 'earth', level: 1 };
        }
    }
    return mods;
}
```

**Buff expiration polling (1s interval in server tick):**

```js
// Runs every 1000ms for all connected players and active enemies
for (const [charId, player] of connectedPlayers.entries()) {
    const expired = expireBuffs(player);
    for (const buff of expired) {
        const zone = player.zone || 'prontera_south';
        broadcastToZone(zone, 'skill:buff_removed', {
            targetId: charId, isEnemy: false,
            buffName: buff.name, reason: 'expired'
        });
    }
}
for (const [eid, enemy] of enemies.entries()) {
    const expired = expireBuffs(enemy);
    for (const buff of expired) {
        broadcastToZone(enemy.zone, 'skill:buff_removed', {
            targetId: eid, isEnemy: true,
            buffName: buff.name, reason: 'expired'
        });
    }
}
```

**Socket events for buffs:**

| Event | Direction | Payload | When |
|-------|-----------|---------|------|
| `skill:buff_applied` | Server -> Zone | `{ targetId, targetName, isEnemy, casterId, casterName, skillId, buffName, duration, effects }` | Buff applied or refreshed |
| `skill:buff_removed` | Server -> Zone | `{ targetId, isEnemy, buffName, reason }` | Buff expired, dispelled, or broken (e.g., fire breaks frozen) |

### 1.6 Cooldown System

Two separate cooldown mechanisms:

**Per-skill cooldown** (only THIS skill is locked):

```js
function isSkillOnCooldown(player, skillId) {
    const cd = (player.skillCooldowns || {})[skillId];
    if (!cd) return false;
    return Date.now() < cd;
}

function setSkillCooldown(player, skillId, cooldownMs) {
    if (!player.skillCooldowns) player.skillCooldowns = {};
    player.skillCooldowns[skillId] = Date.now() + cooldownMs;
}
```

**After-Cast Delay (ACD)** — global skill lockout (ALL skills locked):

```js
// afterCastDelayEnd: Map<characterId, timestamp>
function applySkillDelays(characterId, player, skillId, levelData, socket) {
    const cooldownMs = levelData.cooldown || 0;
    const acdMs = levelData.afterCastDelay || 0;

    if (cooldownMs > 0) {
        setSkillCooldown(player, skillId, cooldownMs);
        if (socket) socket.emit('skill:cooldown_started', { skillId, cooldownMs });
    }

    if (acdMs > 0) {
        afterCastDelayEnd.set(characterId, Date.now() + acdMs);
        if (socket) socket.emit('skill:acd_started', { afterCastDelay: acdMs });
    }
}
```

**Client receives and tracks:**
- `skill:cooldown_started { skillId, cooldownMs }` — per-skill cooldown
- `skill:acd_started { afterCastDelay }` — global ACD

### 1.7 SP Consumption

SP is consumed at **execution time**, not at cast start. This is critical for cast-time skills:

```
Cast starts   → SP NOT deducted (player might get interrupted)
Cast finishes → executeCastComplete re-checks SP, THEN deducts
Instant skill → SP deducted immediately in the handler
```

```js
// In every skill handler:
player.mana = Math.max(0, player.mana - spCost);

// After execution, notify client:
socket.emit('skill:used', {
    skillId, skillName, level, spCost,
    remainingMana: player.mana, maxMana: player.maxMana
});
socket.emit('combat:health_update', {
    characterId, health: player.health, maxHealth: player.maxHealth,
    mana: player.mana, maxMana: player.maxMana
});
```

### 1.8 Gemstone/Catalyst Consumption

Some skills require consumable items (gemstones). Check inventory before execution:

```js
if (skill.name === 'stone_curse') {
    // Requires 1 Red Gemstone (item ID from ro_item_mapping.js)
    const RED_GEMSTONE_ID = 717;
    const invSlot = findInventoryItem(characterId, RED_GEMSTONE_ID);
    if (!invSlot || invSlot.quantity <= 0) {
        socket.emit('skill:error', { message: 'Requires Red Gemstone' });
        return;
    }

    // Lv1-5: consume regardless of success. Lv6-10: only on success
    if (learnedLevel <= 5 || stoneApplied) {
        invSlot.quantity -= 1;
        if (invSlot.quantity <= 0) removeInventoryItem(characterId, invSlot.inventoryId);
        socket.emit('inventory:data', buildInventoryPayload(characterId));
    }
}
```

### 1.9 Complete Socket Event Reference

| Event | Direction | Purpose |
|-------|-----------|---------|
| `skill:use` | Client -> Server | Request to use a skill |
| `skill:used` | Server -> Caster | Confirmation: SP spent, skill executed |
| `skill:error` | Server -> Caster | Validation failure message |
| `skill:cast_start` | Server -> Zone | Cast began (cast bar appears) |
| `skill:cast_complete` | Server -> Zone | Cast finished (cast bar removed) |
| `skill:cast_interrupted` | Server -> Caster | Cast interrupted (damage/movement) |
| `skill:cast_interrupted_broadcast` | Server -> Zone | Cast interrupted (others see bar vanish) |
| `skill:cast_failed` | Server -> Caster | Cast complete but execution failed (e.g., target died) |
| `skill:effect_damage` | Server -> Zone | Skill dealt damage/healed (VFX + damage numbers) |
| `skill:buff_applied` | Server -> Zone | Buff/debuff applied (VFX spawn) |
| `skill:buff_removed` | Server -> Zone | Buff/debuff expired or broken (VFX cleanup) |
| `skill:cooldown_started` | Server -> Caster | Per-skill cooldown started |
| `skill:acd_started` | Server -> Caster | Global after-cast delay started |
| `combat:out_of_range` | Server -> Caster | Skill target out of range |
| `combat:health_update` | Server -> Caster | HP/SP update after skill |

---

## 2. Adding a New Skill

### Step-by-Step Template (Under 30 Minutes)

#### Step 1: Define in `ro_skill_data.js` (or `ro_skill_data_2nd.js`)

Add to the `SKILL_DEFINITIONS` array:

```js
// Template — replace all CAPS values
{
    id: SKILL_ID,
    name: 'SKILL_NAME',
    displayName: 'DISPLAY_NAME',
    classId: 'CLASS_ID',
    maxLevel: MAX_LEVEL,
    type: 'active',              // 'active' | 'passive' | 'toggle'
    targetType: 'TARGET_TYPE',   // 'none' | 'self' | 'single' | 'ground' | 'aoe'
    element: 'ELEMENT',
    range: RANGE_UE_UNITS,       // 150 = melee, 450 = mid, 900 = magic
    description: 'DESCRIPTION',
    icon: 'ICON_NAME',
    treeRow: ROW,
    treeCol: COL,
    prerequisites: [
        // { skillId: PREREQ_ID, level: PREREQ_LEVEL }
    ],
    levels: genLevels(MAX_LEVEL, i => ({
        level: i + 1,
        spCost: SP_FORMULA(i),
        castTime: CAST_FORMULA(i),
        afterCastDelay: ACD_FORMULA(i),
        cooldown: COOLDOWN_FORMULA(i),
        effectValue: EFFECT_FORMULA(i),
        duration: DURATION_FORMULA(i)
    }))
},
```

#### Step 2: Add Handler in `index.js` Skill Switch

Place the handler inside `socket.on('skill:use')`, after the cast time check block. Choose the template that matches your skill type:

**Physical single-target template:**

```js
// --- SKILL_NAME (ID XXX) ---
if (skill.name === 'SKILL_NAME') {
    if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }
    const { target, targetPos, targetStats, targetHardDef, targetName }
        = resolveTarget(targetId, isEnemy);  // helper or inline
    if (!target) return;

    // Range check
    const attackerPos = await getPlayerPosition(characterId);
    if (!attackerPos) return;
    const dist = Math.sqrt((attackerPos.x - targetPos.x)**2 + (attackerPos.y - targetPos.y)**2);
    if (dist > (skill.range || 150) + COMBAT.RANGE_TOLERANCE) {
        socket.emit('combat:out_of_range', { targetId, isEnemy });
        return;
    }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const result = calculateSkillDamage(
        getEffectiveStats(player), targetStats, targetHardDef, effectVal,
        getBuffStatModifiers(player), getBuffStatModifiers(target),
        isEnemy ? getEnemyTargetInfo(target) : getPlayerTargetInfo(target, targetId),
        getAttackerInfo(player)
    );

    target.health = Math.max(0, target.health - result.damage);
    if (isEnemy) { target.lastDamageTime = Date.now(); setEnemyAggro(target, characterId, 'skill'); }

    const zone = player.zone || 'prontera_south';
    broadcastToZone(zone, 'skill:effect_damage', {
        attackerId: characterId, attackerName: player.characterName,
        targetId, targetName, isEnemy,
        skillId, skillName: skill.displayName, skillLevel: learnedLevel,
        element: skill.element, damage: result.damage,
        isCritical: result.isCritical, isMiss: result.isMiss,
        hitType: 'physical',
        attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
        targetHealth: target.health, targetMaxHealth: target.maxHealth,
        timestamp: Date.now()
    });

    if (isEnemy && target.health <= 0) {
        broadcastToZone(zone, 'enemy:health_update', { enemyId: targetId, health: 0, maxHealth: target.maxHealth });
        await processEnemyDeathFromSkill(target, player, characterId, io);
    }

    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
    return;
}
```

**Buff/supportive template:**

```js
if (skill.name === 'BUFF_NAME') {
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const buffTarget = (targetId && !isEnemy) ? connectedPlayers.get(targetId) : player;
    const buffTargetId = targetId || characterId;
    const buffTargetName = buffTarget.characterName || player.characterName;

    const buffDuration = duration || DEFAULT_DURATION;
    applyBuff(buffTarget, {
        skillId, name: 'BUFF_INTERNAL_NAME',
        casterId: characterId, casterName: player.characterName,
        // ... buff-specific fields
        duration: buffDuration
    });

    const zone = player.zone || 'prontera_south';
    broadcastToZone(zone, 'skill:buff_applied', {
        targetId: buffTargetId, targetName: buffTargetName, isEnemy: false,
        casterId: characterId, casterName: player.characterName,
        skillId, buffName: 'DISPLAY_NAME', duration: buffDuration,
        effects: { /* buff-specific */ }
    });

    socket.emit('skill:used', { ... });
    socket.emit('combat:health_update', { ... });
    return;
}
```

#### Step 3: Add VFX Config in `SkillVFXData.cpp`

Add inside `BuildSkillVFXConfigs()`:

```cpp
// SKILL_NAME — Brief description of the VFX
Add(SKILL_ID, ESkillVFXTemplate::TEMPLATE_TYPE,
    FLinearColor(R, G, B, 1.f), TEXT("ELEMENT"),
    bCastCircle, AoERadius, bLoop, ProjectileSpeed, BoltHeight,
    TEXT("NIAGARA_OR_CASCADE_ASSET_PATH"),
    bIsCascade, Scale, bSelfCenter, CascadeLife);
```

**Template type selection guide:**

| Skill Behavior | Template Enum | Example |
|---------------|---------------|---------|
| N bolts striking from sky | `BoltFromSky` | Cold/Fire/Lightning Bolt |
| Projectile player -> enemy | `Projectile` | Soul Strike, Fire Ball |
| Explosion/impact at location | `AoEImpact` | Bash, Napalm Beat, Magnum Break |
| Persistent ground effect | `GroundPersistent` | Fire Wall, Safety Wall |
| Rain of strikes in area | `GroundAoERain` | Thunderstorm |
| Aura on self | `SelfBuff` | Endure, Sight |
| Aura on target (debuff) | `TargetDebuff` | Provoke, Frost Diver, Stone Curse |
| Heal flash on target | `HealFlash` | First Aid, Heal |

#### Step 4: Add to Skill Tree UI Data

The skill tree data comes from the server via `skill:data` event. As long as the skill is in `ro_skill_data.js` with correct `classId`, `treeRow`, `treeCol`, and `prerequisites`, it automatically appears in the client skill tree widget. No additional client code needed.

#### Step 5: Test End-to-End

```
1. Server: `npm run dev` (auto-restarts on save)
2. Client: Live Coding (Ctrl+Alt+F11) picks up SkillVFXData.cpp changes
3. Learn the skill via skill:learn socket event
4. Use from hotbar or skill tree
5. Verify: cast bar appears, VFX plays, damage/buff applied, cooldown works
```

---

## 3. Client-Side: Skill Targeting System

### 3.1 Architecture

File: `client/SabriMMO/Source/SabriMMO/UI/SkillTreeSubsystem.h/.cpp`

The targeting system is owned by `USkillTreeSubsystem` and handles the RO-style "click-to-cast" flow:

```
UseSkill(SkillId)
    |
    v
Check targetType from cached FSkillEntry
    |
    +--> "self" / "aoe" --> UseSkillOnTarget(SkillId, LocalCharacterId, false)
    |                        (immediate, no targeting mode)
    |
    +--> "single" ---------> BeginTargeting(SkillId)
    |                        Set PendingTargetingMode = SingleTarget
    |                        Show targeting overlay (crosshair cursor)
    |                        Wait for click on enemy actor
    |                        --> UseSkillOnTarget(SkillId, EnemyId, true)
    |
    +--> "ground" ---------> BeginTargeting(SkillId)
                             Set PendingTargetingMode = GroundTarget
                             Show AoE indicator (Niagara ring on ground)
                             Wait for click on ground position
                             --> UseSkillOnGround(SkillId, GroundPos)
```

### 3.2 Target Types

```cpp
enum class ESkillTargetingMode : uint8
{
    None,
    SingleTarget,   // Click an enemy to cast
    GroundTarget    // Click ground to confirm position
};
```

| Server targetType | Client Mode | User Action |
|-------------------|-------------|-------------|
| `"none"` | N/A (passive) | Cannot be used actively |
| `"self"` | Immediate | Casts on self, no click needed |
| `"single"` | SingleTarget | Cursor changes, click enemy |
| `"ground"` | GroundTarget | AoE indicator follows cursor, click ground |
| `"aoe"` | Immediate | Casts at caster position, no click needed |

### 3.3 UseSkill Entry Point

```cpp
void USkillTreeSubsystem::UseSkill(int32 SkillId)
{
    // Find cached skill entry
    const FSkillEntry* Entry = FindSkillEntry(SkillId);
    if (!Entry) return;

    // Check if skill is on cooldown
    if (IsSkillOnCooldown(SkillId)) return;

    // Route by target type
    if (Entry->TargetType == TEXT("self") || Entry->TargetType == TEXT("aoe"))
    {
        // Immediate cast — no targeting needed
        UseSkillOnTarget(SkillId, LocalCharacterId, false);
    }
    else if (Entry->TargetType == TEXT("single"))
    {
        BeginTargeting(SkillId);
    }
    else if (Entry->TargetType == TEXT("ground"))
    {
        BeginTargeting(SkillId);
    }
}
```

### 3.4 UseSkillOnTarget (Single Target)

```cpp
void USkillTreeSubsystem::UseSkillOnTarget(int32 SkillId, int32 TargetId, bool bIsEnemy)
{
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetNumberField(TEXT("skillId"), SkillId);
    Payload->SetNumberField(TEXT("targetId"), TargetId);
    Payload->SetBoolField(TEXT("isEnemy"), bIsEnemy);

    SIO->EmitNative(TEXT("skill:use"), MakeShared<FJsonValueObject>(Payload));
}
```

### 3.5 UseSkillOnGround (Ground Target)

```cpp
void USkillTreeSubsystem::UseSkillOnGround(int32 SkillId, FVector GroundPosition)
{
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetNumberField(TEXT("skillId"), SkillId);
    Payload->SetNumberField(TEXT("groundX"), GroundPosition.X);
    Payload->SetNumberField(TEXT("groundY"), GroundPosition.Y);
    Payload->SetNumberField(TEXT("groundZ"), GroundPosition.Z);

    SIO->EmitNative(TEXT("skill:use"), MakeShared<FJsonValueObject>(Payload));
}
```

### 3.6 Targeting Mode Flow

**Enter targeting:**

```cpp
void USkillTreeSubsystem::BeginTargeting(int32 SkillId)
{
    const FSkillEntry* Entry = FindSkillEntry(SkillId);
    if (!Entry) return;

    bIsInTargetingMode = true;
    PendingSkillId = SkillId;
    PendingSkillName = Entry->DisplayName;

    if (Entry->TargetType == TEXT("single"))
    {
        PendingTargetingMode = ESkillTargetingMode::SingleTarget;
    }
    else if (Entry->TargetType == TEXT("ground"))
    {
        PendingTargetingMode = ESkillTargetingMode::GroundTarget;
        // Store AoE radius for indicator
        // Look up from VFX config or skill data
        SpawnGroundAoEIndicator();
    }

    ShowTargetingOverlay();
}
```

**Cancel targeting (right-click or ESC):**

```cpp
void USkillTreeSubsystem::CancelTargeting()
{
    bIsInTargetingMode = false;
    PendingSkillId = 0;
    PendingTargetingMode = ESkillTargetingMode::None;
    HideTargetingOverlay();
    DestroyGroundAoEIndicator();
}
```

**Handle click in targeting mode:**

```cpp
void USkillTreeSubsystem::HandleTargetingClick()
{
    if (!bIsInTargetingMode) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (PendingTargetingMode == ESkillTargetingMode::SingleTarget)
    {
        // Line trace from cursor to find enemy actor
        FHitResult Hit;
        PC->GetHitResultUnderCursor(ECC_Pawn, false, Hit);
        if (Hit.GetActor())
        {
            int32 EnemyId = GetEnemyIdFromActor(Hit.GetActor());
            if (EnemyId > 0)
            {
                UseSkillOnTarget(PendingSkillId, EnemyId, true);
                CancelTargeting();
            }
        }
    }
    else if (PendingTargetingMode == ESkillTargetingMode::GroundTarget)
    {
        // Line trace to find ground position
        FHitResult Hit;
        PC->GetHitResultUnderCursor(ECC_WorldStatic, false, Hit);
        if (Hit.bBlockingHit)
        {
            UseSkillOnGround(PendingSkillId, Hit.ImpactPoint);
            CancelTargeting();
        }
    }
}
```

### 3.7 Ground AoE Indicator

For ground-targeted skills, a Niagara ring follows the cursor to show the AoE radius:

```cpp
void USkillTreeSubsystem::SpawnGroundAoEIndicator()
{
    // Load a simple ring Niagara system (from Free_Magic pack)
    UNiagaraSystem* RingSystem = LoadObject<UNiagaraSystem>(nullptr,
        TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Buff.NS_Free_Magic_Buff"));
    if (!RingSystem) return;

    GroundAoEIndicator = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(), RingSystem, FVector::ZeroVector, FRotator::ZeroRotator,
        FVector(PendingAoERadius / 100.f), true, true, ENCPoolMethod::None);
}

void USkillTreeSubsystem::UpdateGroundAoEIndicatorPosition()
{
    if (!GroundAoEIndicator.IsValid()) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_WorldStatic, false, Hit);
    if (Hit.bBlockingHit)
    {
        GroundAoEIndicator->SetWorldLocation(Hit.ImpactPoint);
    }
}
```

The indicator position is updated every frame via the targeting overlay's `OnPaint` or a timer.

---

## 4. Client-Side: Cast Bar System

### 4.1 Architecture

Files:
- `UI/CastBarSubsystem.h/.cpp` — UWorldSubsystem, event handling, data
- `UI/SCastBarOverlay.h/.cpp` — Slate widget, world-projected rendering

### 4.2 Cast Bar Data

```cpp
struct FCastBarEntry
{
    int32 CasterId = 0;
    FString CasterName;
    FString SkillName;
    int32 SkillId = 0;
    double CastStartTime = 0.0;   // FPlatformTime::Seconds()
    float CastDuration = 0.0f;    // Total cast time in seconds
};

// In UCastBarSubsystem:
TMap<int32, FCastBarEntry> ActiveCasts;  // CasterId -> Cast data
```

### 4.3 Socket Event Handlers

```cpp
void UCastBarSubsystem::HandleCastStart(const TSharedPtr<FJsonValue>& Data)
{
    auto Obj = Data->AsObject();
    if (!Obj) return;

    FCastBarEntry Entry;
    Entry.CasterId = (int32)Obj->GetNumberField(TEXT("casterId"));
    Entry.CasterName = Obj->GetStringField(TEXT("casterName"));
    Entry.SkillName = Obj->GetStringField(TEXT("skillName"));
    Entry.SkillId = (int32)Obj->GetNumberField(TEXT("skillId"));
    Entry.CastDuration = (float)Obj->GetNumberField(TEXT("actualCastTime")) / 1000.f;
    Entry.CastStartTime = FPlatformTime::Seconds();

    ActiveCasts.Add(Entry.CasterId, Entry);
}

void UCastBarSubsystem::HandleCastComplete(const TSharedPtr<FJsonValue>& Data)
{
    auto Obj = Data->AsObject();
    int32 CasterId = (int32)Obj->GetNumberField(TEXT("casterId"));
    ActiveCasts.Remove(CasterId);
}

void UCastBarSubsystem::HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data)
{
    // Same as complete — remove the cast bar
    // Could add a brief red flash before removing
    auto Obj = Data->AsObject();
    int32 CasterId = (int32)Obj->GetNumberField(TEXT("casterId"));
    ActiveCasts.Remove(CasterId);
}
```

### 4.4 World-Projected Cast Bar Rendering

The `SCastBarOverlay` widget is a full-viewport Slate overlay (Z-order 25) that renders cast bars above each casting character using `OnPaint`:

```cpp
int32 SCastBarOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    UCastBarSubsystem* Sub = /* get from world */;
    if (!Sub) return LayerId;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return LayerId;

    const double Now = FPlatformTime::Seconds();

    for (auto& Pair : Sub->ActiveCasts)
    {
        const FCastBarEntry& Cast = Pair.Value;
        float Elapsed = (float)(Now - Cast.CastStartTime);
        float Progress = FMath::Clamp(Elapsed / Cast.CastDuration, 0.f, 1.f);

        // Find caster actor position (enemy or player)
        FVector WorldPos = GetActorWorldPosition(Cast.CasterId);
        WorldPos.Z += 120.f;  // Above character head

        // Project to screen
        FVector2D ScreenPos;
        if (!PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos)) continue;

        // Draw background bar
        const float BarWidth = 100.f;
        const float BarHeight = 12.f;
        FVector2D BarPos(ScreenPos.X - BarWidth * 0.5f, ScreenPos.Y);

        FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(BarPos, FVector2D(BarWidth, BarHeight)),
            &BackgroundBrush, ESlateDrawEffect::None,
            FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));

        // Draw progress fill
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(BarPos, FVector2D(BarWidth * Progress, BarHeight)),
            &FillBrush, ESlateDrawEffect::None,
            FLinearColor(0.2f, 0.6f, 1.0f, 0.9f));

        // Draw skill name text
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(ScreenPos.X, ScreenPos.Y - 16.f), FVector2D(200.f, 16.f)),
            Cast.SkillName, SmallFont, ESlateDrawEffect::None,
            FLinearColor::White);

        // Draw remaining time
        float Remaining = FMath::Max(0.f, Cast.CastDuration - Elapsed);
        FString TimeText = FString::Printf(TEXT("%.1fs"), Remaining);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(ScreenPos.X + BarWidth * 0.5f + 4.f, ScreenPos.Y), FVector2D(50.f, 12.f)),
            TimeText, TinyFont, ESlateDrawEffect::None,
            FLinearColor(0.8f, 0.8f, 0.8f, 1.f));
    }

    return LayerId + 3;
}
```

### 4.5 Interruption Feedback

When a cast is interrupted, the client can show a brief visual indicator:

```cpp
void UCastBarSubsystem::HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data)
{
    auto Obj = Data->AsObject();
    int32 CasterId = (int32)Obj->GetNumberField(TEXT("casterId"));
    FString Reason = Obj->GetStringField(TEXT("reason"));

    // If this is the local player, show feedback
    if (CasterId == LocalCharacterId)
    {
        // The SkillTreeSubsystem shows a brief "Interrupted!" text
        // Or flash the cast bar red before removing
    }

    ActiveCasts.Remove(CasterId);
}
```

---

## 5. Client-Side: Skill Tree UI

### 5.1 Architecture

Files:
- `UI/SkillTreeSubsystem.h/.cpp` — UWorldSubsystem, data management, socket events
- `UI/SSkillTreeWidget.h/.cpp` — Slate widget, hierarchical layout

### 5.2 Data Structures

```cpp
// Single skill entry from server
USTRUCT(BlueprintType)
struct FSkillEntry
{
    int32 SkillId;
    FString Name, DisplayName;
    int32 MaxLevel, CurrentLevel;
    FString Type;       // "active", "passive", "toggle"
    FString TargetType; // "none", "self", "single", "ground", "aoe"
    FString Element;
    int32 Range;
    FString Description, Icon, IconPath;
    int32 TreeRow, TreeCol;
    int32 SpCost, NextSpCost, CastTime, Cooldown, EffectValue;
    bool bCanLearn;
    TArray<FSkillPrerequisite> Prerequisites;
};

// Group of skills per class
USTRUCT(BlueprintType)
struct FSkillClassGroup
{
    FString ClassId, ClassDisplayName;
    TArray<FSkillEntry> Skills;
};
```

### 5.3 Socket Events

**Request skill data (on zone load):**

```cpp
void USkillTreeSubsystem::RequestSkillData()
{
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;
    SIO->EmitNative(TEXT("skills:request"), MakeShared<FJsonValueNull>());
}
```

**Receive skill data:**

```cpp
void USkillTreeSubsystem::HandleSkillData(const TSharedPtr<FJsonValue>& Data)
{
    auto Obj = Data->AsObject();
    SkillPoints = (int32)Obj->GetNumberField(TEXT("skillPoints"));
    JobClass = Obj->GetStringField(TEXT("jobClass"));

    // Parse skill groups
    SkillGroups.Empty();
    const TArray<TSharedPtr<FJsonValue>>& Groups = Obj->GetArrayField(TEXT("skillGroups"));
    for (auto& GroupVal : Groups)
    {
        auto GroupObj = GroupVal->AsObject();
        FSkillClassGroup Group;
        Group.ClassId = GroupObj->GetStringField(TEXT("classId"));
        Group.ClassDisplayName = GroupObj->GetStringField(TEXT("classDisplayName"));

        const TArray<TSharedPtr<FJsonValue>>& Skills = GroupObj->GetArrayField(TEXT("skills"));
        for (auto& SkillVal : Skills)
        {
            auto SkillObj = SkillVal->AsObject();
            FSkillEntry Entry;
            Entry.SkillId = (int32)SkillObj->GetNumberField(TEXT("id"));
            Entry.Name = SkillObj->GetStringField(TEXT("name"));
            Entry.DisplayName = SkillObj->GetStringField(TEXT("displayName"));
            Entry.MaxLevel = (int32)SkillObj->GetNumberField(TEXT("maxLevel"));
            Entry.CurrentLevel = (int32)SkillObj->GetNumberField(TEXT("currentLevel"));
            Entry.Type = SkillObj->GetStringField(TEXT("type"));
            Entry.TargetType = SkillObj->GetStringField(TEXT("targetType"));
            // ... parse all fields
            Entry.bCanLearn = SkillObj->GetBoolField(TEXT("canLearn"));
            Group.Skills.Add(Entry);
        }
        SkillGroups.Add(Group);
    }

    OnSkillDataUpdated.Broadcast();
}
```

**Learn a skill:**

```cpp
void USkillTreeSubsystem::LearnSkill(int32 SkillId)
{
    if (SkillPoints <= 0) return;

    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetNumberField(TEXT("skillId"), SkillId);
    SIO->EmitNative(TEXT("skills:learn"), MakeShared<FJsonValueObject>(Payload));
}
```

### 5.4 Skill Tree Widget Layout

The `SSkillTreeWidget` renders skills in a grid layout per class, with prerequisite lines:

```cpp
// Simplified layout structure:
// +----------------------------------+
// | [Class Name]    Skill Points: 5  |
// +----------------------------------+
// | Row 0: [Skill] [Skill] [Skill]   |
// |           |       |              |
// | Row 1: [Skill] [Skill] [Skill]   |
// |           |                      |
// | Row 2: [Skill] [Skill]           |
// +----------------------------------+

// Each skill cell contains:
// +------------------+
// | [Icon 32x32]     |
// | Skill Name       |
// | Lv 3/10  [+]     |
// | SP: 16            |
// +------------------+
```

**Skill icon with level display:**

```cpp
SNew(SOverlay)
+ SOverlay::Slot()
[
    SNew(SImage)
    .Image(GetOrCreateIconBrush(Entry.IconPath))
]
+ SOverlay::Slot()
.HAlign(HAlign_Right)
.VAlign(VAlign_Bottom)
[
    SNew(STextBlock)
    .Text(FText::FromString(FString::Printf(TEXT("%d/%d"), Entry.CurrentLevel, Entry.MaxLevel)))
    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
    .ColorAndOpacity(Entry.CurrentLevel > 0 ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f))
]
```

**Prerequisite lines:**

Lines are drawn between skill cells during `OnPaint` using `FSlateDrawElement::MakeLines`:

```cpp
// For each skill with prerequisites:
for (const FSkillPrerequisite& Prereq : Entry.Prerequisites)
{
    // Find the prerequisite skill's grid position
    FVector2D StartPos = GetSkillCellCenter(Prereq.RequiredSkillId);
    FVector2D EndPos = GetSkillCellCenter(Entry.SkillId);

    // Draw line (green if met, gray if not)
    bool bMet = LearnedSkills.Contains(Prereq.RequiredSkillId)
                && LearnedSkills[Prereq.RequiredSkillId] >= Prereq.RequiredLevel;
    FLinearColor LineColor = bMet ? FLinearColor(0.2f, 0.8f, 0.2f) : FLinearColor(0.4f, 0.4f, 0.4f);

    TArray<FVector2D> Points = { StartPos, EndPos };
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        Points, ESlateDrawEffect::None, LineColor, true, 2.0f);
}
```

**Tooltip:**

```cpp
SNew(SToolTip)
[
    SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(Entry.DisplayName))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
    ]
    + SVerticalBox::Slot().AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(Entry.Description))
        .AutoWrapText(true)
    ]
    + SVerticalBox::Slot().AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("SP Cost: %d | Cast: %.1fs | CD: %.1fs"),
            Entry.SpCost, Entry.CastTime / 1000.f, Entry.Cooldown / 1000.f)))
    ]
    + SVerticalBox::Slot().AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("Effect: %d%% | Element: %s"),
            Entry.EffectValue, *Entry.Element)))
    ]
]
```

### 5.5 Skill Drag to Hotbar

```cpp
void USkillTreeSubsystem::StartSkillDrag(int32 SkillId, const FString& Name, const FString& Icon)
{
    bSkillDragging = true;
    DraggedSkillId = SkillId;
    DraggedSkillName = Name;
    DraggedSkillIcon = Icon;
    ShowSkillDragCursor(Icon);
}

void USkillTreeSubsystem::CancelSkillDrag()
{
    bSkillDragging = false;
    DraggedSkillId = 0;
    HideSkillDragCursor();
}
```

The hotbar widget checks `SkillTreeSubsystem->bSkillDragging` on mouse up and calls `AssignSkill()` if a skill is being dragged over a slot.

---

## 6. Client-Side: Skill VFX Integration

### 6.1 Architecture

Files:
- `VFX/SkillVFXSubsystem.h/.cpp` — UWorldSubsystem, event handling, VFX spawning
- `VFX/SkillVFXData.h` — FSkillVFXConfig struct, ESkillVFXTemplate enum
- `VFX/SkillVFXData.cpp` — `BuildSkillVFXConfigs()` registry

### 6.2 Five VFX Behavior Patterns

#### Pattern A: Bolt From Sky

Used by Cold Bolt (200), Fire Bolt (201), Lightning Bolt (202).

N bolts strike from above the target, staggered by 0.15s. Each bolt is a separate particle spawned at `TargetLocation + (0, 0, BoltSpawnHeight)` and moved to `TargetLocation`.

```cpp
void USkillVFXSubsystem::SpawnBoltFromSky(FVector TargetLocation,
    const FSkillVFXConfig& Config, int32 TotalHits)
{
    for (int32 i = 0; i < TotalHits; i++)
    {
        FTimerHandle TimerHandle;
        float Delay = i * Config.BoltInterval;  // 0.15s default

        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, TargetLocation, Config, i]()
        {
            // Add slight random offset per bolt for visual variety
            FVector Offset(FMath::RandRange(-30.f, 30.f), FMath::RandRange(-30.f, 30.f), 0.f);
            FVector SpawnPos = TargetLocation + Offset + FVector(0, 0, Config.BoltSpawnHeight);

            SpawnVFXAtLocation(Config, SpawnPos, FRotator(-90, 0, 0),
                FVector(Config.Scale));
        }, Delay, false);
    }
}
```

#### Pattern B: AoE Projectile (Single Projectile + Explosion)

Used by Fire Ball (207). One projectile travels from caster to primary target, then spawns an explosion at the impact point. The `bSingleProjectile` flag prevents duplicate projectiles when the server emits multiple `skill:effect_damage` events for the AoE splash.

```cpp
// In HandleSkillEffectDamage:
if (Config.bSingleProjectile)
{
    int64 Key = (int64)AttackerId * 10000 + SkillId;
    double Now = FPlatformTime::Seconds();
    double* LastSpawn = SingleProjectileLastSpawnTime.Find(Key);
    if (LastSpawn && (Now - *LastSpawn) < 0.5)
        return;  // Dedup: skip duplicate within 500ms
    SingleProjectileLastSpawnTime.Add(Key, Now);
}

SpawnProjectileEffect(AttackerLocation, TargetLocation, Config);
```

The projectile spawns an impact explosion on arrival:

```cpp
void USkillVFXSubsystem::SpawnProjectileEffect(FVector Start, FVector End,
    const FSkillVFXConfig& Config)
{
    // Spawn projectile at start, moving toward end
    FVector Direction = (End - Start).GetSafeNormal();
    FRotator Rotation = Direction.Rotation();

    UNiagaraComponent* Proj = SpawnVFXAtLocation(Config, Start, Rotation);
    // ... set up movement timer to lerp toward End

    // On arrival: spawn impact effect
    if (!Config.ImpactOverridePath.IsEmpty())
    {
        UNiagaraSystem* ImpactSystem = GetOrLoadNiagaraOverride(Config.ImpactOverridePath);
        if (ImpactSystem)
        {
            SpawnNiagaraAtLocation(ImpactSystem, End, FRotator::ZeroRotator,
                FVector(Config.Scale));
        }
    }
}
```

#### Pattern C: Multi-Hit Projectile

Used by Soul Strike (210). N projectiles travel from player to enemy, staggered by 200ms (matching the server's per-hit delay).

```cpp
void USkillVFXSubsystem::SpawnMultiHitProjectile(FVector AttackerLocation,
    FVector TargetLocation, const FSkillVFXConfig& Config, int32 TotalHits)
{
    for (int32 i = 0; i < TotalHits; i++)
    {
        FTimerHandle TimerHandle;
        float Delay = i * 0.2f;  // 200ms stagger matching server

        GetWorld()->GetTimerManager().SetTimer(TimerHandle,
            [this, AttackerLocation, TargetLocation, Config]()
        {
            SpawnProjectileEffect(AttackerLocation, TargetLocation, Config);
        }, Delay, false);
    }
}
```

#### Pattern D: Persistent Buff

Used by Frost Diver (208), Provoke (104), Stone Curse (206). The effect is spawned on `skill:buff_applied` and attached to the target actor. It persists until `skill:buff_removed`.

```cpp
void USkillVFXSubsystem::HandleBuffApplied(const TSharedPtr<FJsonValue>& Data)
{
    // Parse targetId, isEnemy, skillId, duration
    AActor* TargetActor = isEnemy ? FindEnemyActorById(targetId) : FindPlayerActorById(targetId);
    if (!TargetActor) return;

    const FSkillVFXConfig& Config = SkillVFXDataHelper::GetSkillVFXConfig(skillId);
    if (Config.Template == ESkillVFXTemplate::TargetDebuff)
    {
        SpawnTargetDebuff(TargetActor, Config, skillId);
    }
    else if (Config.Template == ESkillVFXTemplate::SelfBuff)
    {
        SpawnSelfBuff(TargetActor, Config, skillId, targetId);
    }
}

void USkillVFXSubsystem::HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data)
{
    // Find and destroy the active buff VFX
    int64 Key = (int64)targetId * 10000 + skillId;

    // Check Niagara auras
    if (auto* Comp = ActiveBuffAuras.Find(Key))
    {
        if (Comp->IsValid()) Comp->Get()->DeactivateImmediate();
        ActiveBuffAuras.Remove(Key);
    }
    // Check Cascade auras
    if (auto* PSC = ActiveCascadeBuffs.Find(Key))
    {
        if (PSC->IsValid()) PSC->Get()->DeactivateImmediate();
        ActiveCascadeBuffs.Remove(Key);
    }
}
```

**CascadeLoopTimer (2s interval):**

Non-looping Cascade effects (like Frost Diver's ice) fade after playing once. The timer re-activates them every 2 seconds so they persist for the full buff duration:

```cpp
// Set up in OnWorldBeginPlay:
GetWorld()->GetTimerManager().SetTimer(CascadeLoopTimer, [this]()
{
    for (auto& Pair : ActiveCascadeBuffs)
    {
        if (Pair.Value.IsValid())
        {
            // Unconditionally re-activate — do NOT check IsActive()
            // (returns true even after particles have visually faded)
            Pair.Value->ActivateSystem(true);
        }
    }
}, 2.0f, true);
```

#### Pattern E: Ground AoE Rain

Used by Thunderstorm (212). N strikes at random positions within the AoE radius, staggered by 300ms.

```cpp
void USkillVFXSubsystem::SpawnGroundAoERain(FVector Center,
    const FSkillVFXConfig& Config, int32 NumStrikes)
{
    for (int32 i = 0; i < NumStrikes; i++)
    {
        FTimerHandle TimerHandle;
        float Delay = i * 0.3f;

        GetWorld()->GetTimerManager().SetTimer(TimerHandle,
            [this, Center, Config]()
        {
            // Random position within AoE radius
            float Angle = FMath::RandRange(0.f, 360.f);
            float Dist = FMath::RandRange(0.f, Config.AoERadius);
            FVector StrikePos = Center + FVector(
                FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
                FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist,
                0.f
            );

            SpawnVFXAtLocation(Config, StrikePos);
        }, Delay, false);
    }
}
```

### 6.3 VFX Config Data Structure

```cpp
// In SkillVFXData.cpp — BuildSkillVFXConfigs():
// Cold Bolt example:
Add(200,                                    // Skill ID
    ESkillVFXTemplate::BoltFromSky,         // Template type
    FLinearColor(0.3f, 0.8f, 1.f, 1.f),    // Primary color (ice blue)
    TEXT("water"),                           // Element
    true,                                   // bUseCastingCircle
    0.f,                                    // AoERadius (0 = no AoE indicator)
    false,                                  // bLooping
    2000.f,                                 // ProjectileSpeed
    500.f,                                  // BoltSpawnHeight
    TEXT("/Game/.../P_Elemental_Ice_Proj"),  // VFX asset path
    true,                                   // bIsCascade (true = Cascade, false = Niagara)
    1.0f,                                   // Scale
    false,                                  // bSelfCentered
    0.5f);                                  // CascadeLifetime
```

### 6.4 Mapping a New Skill to an Existing Pattern

To add VFX for a new skill, identify which of the 5 patterns matches, then add one `Add()` call:

| If the skill... | Use Pattern | Template Enum |
|-----------------|------------|---------------|
| Strikes from above | A (Bolt) | `BoltFromSky` |
| Shoots one projectile that explodes | B (AoE Projectile) | `Projectile` + `bSingleProjectile=true` |
| Shoots N projectiles at target | C (Multi-Hit) | `Projectile` |
| Applies persistent aura on target | D (Buff) | `TargetDebuff` or `SelfBuff` |
| Rains strikes in an area | E (Ground Rain) | `GroundAoERain` |
| Simple impact at target location | N/A | `AoEImpact` |
| Persistent ground object | N/A | `GroundPersistent` |
| Heal/support flash | N/A | `HealFlash` |

---

## 7. Client-Side: Hotbar Integration

### 7.1 Architecture

Files:
- `UI/HotbarSubsystem.h/.cpp` — 4 rows x 9 slots, keybinds, socket events
- `UI/SHotbarRowWidget.h/.cpp` — Single row Slate widget
- `UI/SHotbarKeybindWidget.h/.cpp` — Keybind configuration panel

### 7.2 Skill Slots on Hotbar

Skills are stored alongside items in the same slot system:

```cpp
USTRUCT()
struct FHotbarSlot
{
    int32 RowIndex = 0;
    int32 SlotIndex = 0;        // 0-8
    FString SlotType;           // "item" or "skill" or "" (empty)

    // Item fields
    int32 InventoryId = 0;
    int32 ItemId = 0;
    FString ItemName, ItemIcon;
    int32 Quantity = 0;

    // Skill fields
    int32 SkillId = 0;
    FString SkillName, SkillIcon;

    bool IsSkill() const { return SlotType == TEXT("skill") && SkillId > 0; }
};
```

### 7.3 Assigning Skills to Hotbar

```cpp
void UHotbarSubsystem::AssignSkill(int32 RowIndex, int32 SlotIndex,
    int32 SkillId, const FString& SkillName, const FString& SkillIcon)
{
    if (RowIndex < 0 || RowIndex >= NUM_ROWS) return;
    if (SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;

    FHotbarSlot& Slot = Slots[RowIndex][SlotIndex];
    Slot.Clear();
    Slot.RowIndex = RowIndex;
    Slot.SlotIndex = SlotIndex;
    Slot.SlotType = TEXT("skill");
    Slot.SkillId = SkillId;
    Slot.SkillName = SkillName;
    Slot.SkillIcon = SkillIcon;

    DataVersion++;
    EmitSaveSkill(RowIndex, SlotIndex, SkillId, SkillName);
    OnHotbarDataUpdated.Broadcast();
}
```

### 7.4 Activating a Skill Slot

```cpp
void UHotbarSubsystem::ActivateSlot(int32 RowIndex, int32 SlotIndex)
{
    const FHotbarSlot& Slot = GetSlot(RowIndex, SlotIndex);

    if (Slot.IsSkill())
    {
        // Delegate to SkillTreeSubsystem
        USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>();
        if (SkillSub)
        {
            SkillSub->UseSkill(Slot.SkillId);
        }
    }
    else if (Slot.IsItem())
    {
        // Delegate to InventorySubsystem for item usage
        // ...
    }
}
```

### 7.5 Cooldown Overlay on Hotbar Icons

The hotbar widget queries `SkillTreeSubsystem::IsSkillOnCooldown()` and `GetSkillCooldownRemaining()` to render a darkened overlay:

```cpp
// In SHotbarRowWidget::OnPaint, for each skill slot:
USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>();
if (SkillSub && Slot.IsSkill() && SkillSub->IsSkillOnCooldown(Slot.SkillId))
{
    float Remaining = SkillSub->GetSkillCooldownRemaining(Slot.SkillId);
    // Draw semi-transparent dark overlay
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
        SlotGeometry, &DarkBrush, ESlateDrawEffect::None,
        FLinearColor(0.f, 0.f, 0.f, 0.6f));

    // Draw cooldown text
    FString CdText = FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(Remaining));
    FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
        SlotGeometry, CdText, CooldownFont, ESlateDrawEffect::None,
        FLinearColor::White);
}
```

### 7.6 SP Cost Display

Show SP cost below the skill icon in the hotbar:

```cpp
// Below each skill icon:
if (Slot.IsSkill())
{
    const FSkillEntry* Entry = SkillSub->FindSkillEntry(Slot.SkillId);
    if (Entry)
    {
        FString SpText = FString::Printf(TEXT("SP:%d"), Entry->SpCost);
        // Render small text below slot
    }
}
```

### 7.7 Keybind Activation

```cpp
void UHotbarSubsystem::HandleNumberKey(int32 KeyNumber, bool bAlt, bool bCtrl, bool bShift)
{
    // KeyNumber: 1-9 maps to slot 0-8
    int32 SlotIndex = KeyNumber - 1;
    if (SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;

    // Find which row this keybind matches
    for (int32 Row = 0; Row < NUM_ROWS; Row++)
    {
        const FHotbarKeybind& KB = Keybinds[Row][SlotIndex];
        if (KB.PrimaryKey == EKeys::Invalid) continue;

        // Check modifier match
        if (KB.bRequiresAlt == bAlt && KB.bRequiresCtrl == bCtrl && KB.bRequiresShift == bShift)
        {
            ActivateSlot(Row, SlotIndex);
            return;
        }
    }
}
```

**Default keybinds:**
- Row 0: Keys 1-9 (no modifiers)
- Row 1: Alt + 1-9
- Rows 2-3: Unbound by default (configurable via keybind panel)

---

## 8. Implementing Each Skill Category

### 8.1 Offensive Single Target: Fire Bolt (ID 201)

A multi-hit bolt skill. N = skill level bolts, each dealing 100% MATK. Fire element.

#### Server (`index.js`):

**Already implemented.** The bolt handler covers Cold Bolt (200), Fire Bolt (201), and Lightning Bolt (202) in one block:

```js
// --- BOLT SKILLS: Cold Bolt (200), Fire Bolt (201), Lightning Bolt (202) ---
if (skill.name === 'cold_bolt' || skill.name === 'fire_bolt' || skill.name === 'lightning_bolt') {
    if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

    // Resolve target
    let target, targetPos, targetStats, targetHardMdef = 0, targetName = '';
    if (isEnemy) {
        target = enemies.get(targetId);
        if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
        targetPos = { x: target.x, y: target.y, z: target.z };
        targetStats = target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
        targetHardMdef = target.hardMdef || target.magicDefense || 0;
        targetName = target.name;
    } else {
        target = connectedPlayers.get(targetId);
        if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
        targetPos = await getPlayerPosition(targetId);
        if (!targetPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
        targetStats = getEffectiveStats(target);
        targetHardMdef = target.hardMdef || 0;
        targetName = target.characterName;
    }

    // Range check
    const attackerPos = await getPlayerPosition(characterId);
    if (!attackerPos) return;
    const dist = Math.sqrt((attackerPos.x - targetPos.x)**2 + (attackerPos.y - targetPos.y)**2);
    if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
        socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
        return;
    }

    // CC check
    const casterBuffs = getBuffStatModifiers(player);
    if (casterBuffs.isFrozen || casterBuffs.isStoned) {
        socket.emit('skill:error', { message: 'Cannot cast while incapacitated' });
        return;
    }

    // Deduct SP and apply delays
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Multi-hit damage: hits = skill level, 100% MATK per hit
    const numHits = learnedLevel;
    const targetBuffMods = getBuffStatModifiers(target);
    const magicTargetInfo = isEnemy ? getEnemyTargetInfo(target) : getPlayerTargetInfo(target, targetId);
    if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;
    magicTargetInfo.buffMods = targetBuffMods;

    const hitDamages = [];
    let totalDamage = 0;
    for (let h = 0; h < numHits; h++) {
        const hitResult = calculateMagicSkillDamage(
            getEffectiveStats(player), isEnemy ? targetStats : getEffectiveStats(target),
            targetHardMdef, 100, skill.element, magicTargetInfo
        );
        hitDamages.push(hitResult.damage);
        totalDamage += hitResult.damage;
    }

    // Apply total damage at once
    target.health = Math.max(0, target.health - totalDamage);
    if (!isEnemy && activeCasts.has(targetId)) interruptCast(targetId, 'damage');
    if (isEnemy) { target.lastDamageTime = Date.now(); setEnemyAggro(target, characterId, 'skill'); }

    // Fire breaks Frozen status
    if (skill.element === 'fire' && targetBuffMods.isFrozen && target.activeBuffs) {
        target.activeBuffs = target.activeBuffs.filter(b => b.name !== 'frozen');
        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy, buffName: 'frozen', reason: 'fire_damage' });
    }

    // Summary event
    const zone = player.zone || 'prontera_south';
    broadcastToZone(zone, 'skill:effect_damage', {
        attackerId: characterId, attackerName: player.characterName,
        targetId, targetName, isEnemy,
        skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
        damage: totalDamage, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
        targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
    });

    // Per-hit staggered damage numbers (200ms apart)
    for (let h = 0; h < numHits; h++) {
        setTimeout(() => {
            broadcastToZone(zone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
                damage: hitDamages[h], isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                hitNumber: h + 1, totalHits: numHits,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });
        }, h * 200);
    }

    // Death handling
    if (isEnemy) {
        broadcastToZone(zone, 'enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
        if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
    }

    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, hits: numHits, totalDamage });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
    return;
}
```

#### Client VFX (`SkillVFXData.cpp`):

**Already implemented:**

```cpp
// Fire Bolt — NS_Magma_Shot_Projectile, scale 3.0 (base is tiny)
Add(201, ESkillVFXTemplate::BoltFromSky, FLinearColor(1.f, 0.3f, 0.0f, 1.f), TEXT("fire"), true,
    0.f, false, 2000.f, 500.f,
    TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Projectile.NS_Magma_Shot_Projectile"), false, 3.0f);
```

**Skill data (`ro_skill_data.js`):**

```js
{ id: 201, name: 'fire_bolt', displayName: 'Fire Bolt', classId: 'mage', maxLevel: 10,
  type: 'active', targetType: 'single', element: 'fire', range: 900,
  description: 'Fire bolt magic. +1 bolt per level.', icon: 'fire_bolt',
  treeRow: 0, treeCol: 2, prerequisites: [],
  levels: genLevels(10, i => ({
      level: i+1, spCost: 12+i*2, castTime: 700*(i+1),
      afterCastDelay: 800+(i+1)*200, cooldown: 0, effectValue: 100, duration: 0
  }))
},
```

---

### 8.2 Offensive AoE: Thunderstorm (ID 212)

Ground-targeted, Wind element. N = skill level strikes in a 5x5 area.

#### Server (`index.js`):

**Already implemented.** Key points:

```js
if (skill.name === 'thunderstorm') {
    if (!hasGroundPos) { socket.emit('skill:error', { message: 'No ground position specified' }); return; }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const AOE_RADIUS = 250;
    const numHits = learnedLevel;  // 1 hit per level
    // effectValue = 80 (80% MATK per hit)

    // Find all enemies and players in AoE
    const targetsInRange = [];
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.zone !== playerZone) continue;
        const dist = Math.sqrt((groundX - enemy.x)**2 + (groundY - enemy.y)**2);
        if (dist <= AOE_RADIUS) targetsInRange.push({ id: eid, entity: enemy, isEnemy: true });
    }

    // Calculate and apply damage to each target
    for (const tgt of targetsInRange) {
        let totalDmg = 0;
        const hitDmgs = [];
        for (let h = 0; h < numHits; h++) {
            const res = calculateMagicSkillDamage(
                getEffectiveStats(player), tgt.entity.stats, tgt.entity.hardMdef || 0,
                effectVal, 'wind', getEnemyTargetInfo(tgt.entity)
            );
            hitDmgs.push(res.damage);
            totalDmg += res.damage;
        }
        tgt.entity.health = Math.max(0, tgt.entity.health - totalDmg);
        setEnemyAggro(tgt.entity, characterId, 'skill');

        // Staggered per-hit events (300ms apart for AoE rain pattern)
        for (let h = 0; h < numHits; h++) {
            setTimeout(() => {
                broadcastToZone(zone, 'skill:effect_damage', {
                    attackerId: characterId, targetId: tgt.id, isEnemy: true,
                    skillId, skillName: 'Thunderstorm', element: 'wind',
                    damage: hitDmgs[h], hitNumber: h+1, totalHits: numHits,
                    groundX, groundY, groundZ,
                    targetX: tgt.entity.x, targetY: tgt.entity.y, targetZ: tgt.entity.z,
                    timestamp: Date.now()
                });
            }, h * 300);
        }

        if (tgt.entity.health <= 0) await processEnemyDeathFromSkill(tgt.entity, player, characterId, io);
    }
}
```

#### Client VFX:

```cpp
// Thunderstorm — NS_Lightning_Strike, per-hit in AoE
Add(212, ESkillVFXTemplate::GroundAoERain, FLinearColor(1.f, 1.f, 0.5f, 1.f), TEXT("wind"),
    true, 500.f, false, 0.f, 0.f,
    TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Strike.NS_Lightning_Strike"), false, 1.0f);
```

---

### 8.3 Supportive: Heal (ID 400)

Single target (self or ally). Against Undead, deals Holy damage.

#### Server — NEW IMPLEMENTATION:

```js
// --- HEAL (ID 400) — Single target HP restore / Undead damage ---
if (skill.name === 'heal') {
    // Resolve target — default to self if no target specified
    let healTarget = player;
    let healTargetId = characterId;
    let healTargetName = player.characterName;
    let isHealEnemy = false;

    if (targetId && targetId !== characterId) {
        if (isEnemy) {
            // Check if target is Undead element — heal becomes Holy damage
            const enemy = enemies.get(targetId);
            if (!enemy || enemy.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
            const enemyInfo = getEnemyTargetInfo(enemy);
            const enemyElement = (enemyInfo.element && enemyInfo.element.type) || 'neutral';
            if (enemyElement !== 'undead') {
                socket.emit('skill:error', { message: 'Can only Heal undead enemies' });
                return;
            }
            isHealEnemy = true;
            healTarget = enemy;
            healTargetId = targetId;
            healTargetName = enemy.name;
        } else {
            const ally = connectedPlayers.get(targetId);
            if (!ally || ally.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
            healTarget = ally;
            healTargetId = targetId;
            healTargetName = ally.characterName;
        }
    }

    // Range check
    if (healTargetId !== characterId) {
        const attackerPos = await getPlayerPosition(characterId);
        const targetPos = isHealEnemy
            ? { x: healTarget.x, y: healTarget.y, z: healTarget.z }
            : await getPlayerPosition(healTargetId);
        if (!attackerPos || !targetPos) return;
        const dist = Math.sqrt((attackerPos.x - targetPos.x)**2 + (attackerPos.y - targetPos.y)**2);
        if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
            socket.emit('combat:out_of_range', { targetId: healTargetId, isEnemy: isHealEnemy });
            return;
        }
    }

    // Deduct SP and apply delays
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Heal formula: floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)
    const effectiveStats = getEffectiveStats(player);
    const baseLv = effectiveStats.level || player.baseLevel || 1;
    const healAmount = Math.floor((baseLv + effectiveStats.int) / 8) * (4 + 8 * learnedLevel);

    const zone = player.zone || 'prontera_south';
    const attackerPos = await getPlayerPosition(characterId);

    if (isHealEnemy) {
        // Deal Holy damage = 50% of heal amount to Undead
        const damage = Math.floor(healAmount * 0.5);
        healTarget.health = Math.max(0, healTarget.health - damage);
        healTarget.lastDamageTime = Date.now();
        setEnemyAggro(healTarget, characterId, 'skill');

        broadcastToZone(zone, 'skill:effect_damage', {
            attackerId: characterId, attackerName: player.characterName,
            targetId: healTargetId, targetName: healTargetName, isEnemy: true,
            skillId, skillName: 'Heal', skillLevel: learnedLevel, element: 'holy',
            damage, isCritical: false, isMiss: false, hitType: 'magical',
            attackerX: attackerPos?.x || 0, attackerY: attackerPos?.y || 0, attackerZ: attackerPos?.z || 0,
            targetX: healTarget.x, targetY: healTarget.y, targetZ: healTarget.z,
            targetHealth: healTarget.health, targetMaxHealth: healTarget.maxHealth,
            timestamp: Date.now()
        });

        broadcastToZone(zone, 'enemy:health_update', {
            enemyId: healTargetId, health: healTarget.health, maxHealth: healTarget.maxHealth, inCombat: true
        });
        if (healTarget.health <= 0) await processEnemyDeathFromSkill(healTarget, player, characterId, io);
    } else {
        // Heal ally or self
        const healed = Math.min(healAmount, healTarget.maxHealth - healTarget.health);
        healTarget.health = Math.min(healTarget.maxHealth, healTarget.health + healAmount);

        const targetPos = (healTargetId === characterId)
            ? attackerPos
            : await getPlayerPosition(healTargetId);

        broadcastToZone(zone, 'skill:effect_damage', {
            attackerId: characterId, attackerName: player.characterName,
            targetId: healTargetId, targetName: healTargetName, isEnemy: false,
            skillId, skillName: 'Heal', skillLevel: learnedLevel, element: 'holy',
            damage: 0, isCritical: false, isMiss: false, hitType: 'heal',
            healAmount: healed,
            attackerX: attackerPos?.x || 0, attackerY: attackerPos?.y || 0, attackerZ: attackerPos?.z || 0,
            targetX: targetPos?.x || 0, targetY: targetPos?.y || 0, targetZ: targetPos?.z || 0,
            targetHealth: healTarget.health, targetMaxHealth: healTarget.maxHealth,
            timestamp: Date.now()
        });

        // Update healed target's health
        const healTargetSocket = [...io.sockets.sockets.values()].find(s => {
            const p = findPlayerBySocketId(s.id);
            return p && p.characterId === healTargetId;
        });
        if (healTargetSocket) {
            healTargetSocket.emit('combat:health_update', {
                characterId: healTargetId, health: healTarget.health,
                maxHealth: healTarget.maxHealth, mana: healTarget.mana, maxMana: healTarget.maxMana,
                healAmount: healed
            });
        }
    }

    socket.emit('skill:used', { skillId, skillName: 'Heal', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
    return;
}
```

#### Client VFX:

```cpp
// Heal — NS_Potion green flash (same as First Aid but with holy element color)
Add(400, ESkillVFXTemplate::HealFlash, FLinearColor(0.9f, 1.0f, 0.3f, 1.f), TEXT("holy"),
    false, 0.f, false, 0.f, 0.f,
    TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion"), false, 3.0f);
```

#### Skill data (already in `ro_skill_data.js`):

```js
{ id: 400, name: 'heal', displayName: 'Heal', classId: 'acolyte', maxLevel: 10,
  type: 'active', targetType: 'single', element: 'holy', range: 450,
  description: 'Restore HP. Damages Undead.', icon: 'heal',
  treeRow: 0, treeCol: 0, prerequisites: [],
  levels: genLevels(10, i => ({
      level: i+1, spCost: 13+i*3, castTime: 0, cooldown: 1000,
      effectValue: 18*(i+1), duration: 0
  }))
},
```

---

### 8.4 Buff: Blessing (ID 402)

Increases STR/DEX/INT of target for a duration. Against Undead/Demon: reduces DEX/INT instead.

#### Server — NEW IMPLEMENTATION:

```js
// --- BLESSING (ID 402) — +STR/DEX/INT buff ---
if (skill.name === 'blessing') {
    // Resolve target — default to self
    let buffTarget = player;
    let buffTargetId = characterId;
    let buffTargetName = player.characterName;
    let buffIsEnemy = false;

    if (targetId && targetId !== characterId) {
        if (isEnemy) {
            const enemy = enemies.get(targetId);
            if (!enemy || enemy.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
            buffTarget = enemy;
            buffTargetId = targetId;
            buffTargetName = enemy.name;
            buffIsEnemy = true;
        } else {
            const ally = connectedPlayers.get(targetId);
            if (!ally || ally.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
            buffTarget = ally;
            buffTargetId = targetId;
            buffTargetName = ally.characterName;
        }
    }

    // Range check
    if (buffTargetId !== characterId) {
        const attackerPos = await getPlayerPosition(characterId);
        const targetPos = buffIsEnemy
            ? { x: buffTarget.x, y: buffTarget.y, z: buffTarget.z }
            : await getPlayerPosition(buffTargetId);
        if (!attackerPos || !targetPos) return;
        const dist = Math.sqrt((attackerPos.x - targetPos.x)**2 + (attackerPos.y - targetPos.y)**2);
        if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
            socket.emit('combat:out_of_range', { targetId: buffTargetId, isEnemy: buffIsEnemy });
            return;
        }
    }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const statBonus = effectVal;  // effectValue = level (1-10)
    const buffDuration = duration || (60000 + (learnedLevel - 1) * 20000);  // 60s + 20s per level

    // Check if target is Undead/Demon — curse instead of bless
    let isDebuff = false;
    if (buffIsEnemy) {
        const enemyInfo = getEnemyTargetInfo(buffTarget);
        const race = enemyInfo.race || 'formless';
        const element = (enemyInfo.element && enemyInfo.element.type) || 'neutral';
        if (race === 'undead' || race === 'demon' || element === 'undead') {
            isDebuff = true;
        }
    }

    const buffDef = {
        skillId, name: 'blessing',
        casterId: characterId, casterName: player.characterName,
        strBonus: isDebuff ? 0 : statBonus,
        dexBonus: isDebuff ? -Math.floor(statBonus * 0.5) : statBonus,
        intBonus: isDebuff ? -Math.floor(statBonus * 0.5) : statBonus,
        duration: buffDuration
    };
    applyBuff(buffTarget, buffDef);

    const zone = player.zone || 'prontera_south';
    logger.info(`[SKILL-COMBAT] ${player.characterName} BLESSING Lv${learnedLevel} → ${buffTargetName} (${isDebuff ? 'cursed' : '+' + statBonus + ' STR/DEX/INT'} for ${buffDuration / 1000}s)`);

    broadcastToZone(zone, 'skill:buff_applied', {
        targetId: buffTargetId, targetName: buffTargetName, isEnemy: buffIsEnemy,
        casterId: characterId, casterName: player.characterName,
        skillId, buffName: isDebuff ? 'Blessing (Curse)' : 'Blessing',
        duration: buffDuration,
        effects: {
            strBonus: buffDef.strBonus, dexBonus: buffDef.dexBonus, intBonus: buffDef.intBonus
        }
    });

    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
    return;
}
```

**Update `getBuffStatModifiers()` to include Blessing:**

```js
if (buff.name === 'blessing') {
    mods.bonusSTR = (mods.bonusSTR || 0) + (buff.strBonus || 0);
    mods.bonusDEX = (mods.bonusDEX || 0) + (buff.dexBonus || 0);
    mods.bonusINT = (mods.bonusINT || 0) + (buff.intBonus || 0);
}
```

**Update `getEffectiveStats()` to apply buff stat bonuses:**

```js
function getEffectiveStats(player) {
    const base = { /* ... existing stats ... */ };
    const buffMods = getBuffStatModifiers(player);
    base.str += buffMods.bonusSTR || 0;
    base.dex += buffMods.bonusDEX || 0;
    base.int += buffMods.bonusINT || 0;
    // Ensure stats don't go below 1
    base.str = Math.max(1, base.str);
    base.dex = Math.max(1, base.dex);
    base.int = Math.max(1, base.int);
    return base;
}
```

#### Client VFX:

```cpp
// Blessing — NS_Potion with holy yellow color, on target
Add(402, ESkillVFXTemplate::SelfBuff, FLinearColor(1.f, 1.f, 0.6f, 1.f), TEXT("holy"),
    false, 0.f, false, 0.f, 0.f,
    TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion"), false, 3.0f, false, 1.0f);
```

---

### 8.5 Debuff: Provoke (ID 104)

Single target enemy debuff. Reduces target DEF, increases target ATK. 30 second duration.

#### Server:

**Already implemented.** See section 1.4 above. Key code:

```js
if (skill.name === 'provoke') {
    // ... target resolution and range check ...

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const buffDef = {
        skillId, name: 'provoke', casterId: characterId, casterName: player.characterName,
        defReduction: effectVal,    // effectValue = 5 + level * 3 (e.g., Lv10 = 35%)
        atkIncrease: effectVal,
        duration: duration || 30000,
        mdefBonus: 0
    };
    applyBuff(target, buffDef);

    broadcastToZone(zone, 'skill:buff_applied', {
        targetId, targetName, isEnemy,
        casterId: characterId, casterName: player.characterName,
        skillId, buffName: 'Provoke', duration: duration || 30000,
        effects: { defReduction: effectVal, atkIncrease: effectVal }
    });
}
```

#### Client VFX:

```cpp
// Provoke — P_Enrage_Base (Cascade), above enemy head, 3s lifetime
Add(104, ESkillVFXTemplate::TargetDebuff, FLinearColor(1.f, 0.1f, 0.1f, 1.f), TEXT("neutral"),
    false, 0.f, false, 0.f, 0.f,
    TEXT("/Game/InfinityBladeEffects/Effects/FX_Archive/P_Enrage_Base.P_Enrage_Base"),
    true, 3.0f, false, 3.0f);
```

---

### 8.6 Passive: Sword Mastery (ID 100)

Flat +ATK bonus with 1H Swords and Daggers. No active handler needed.

#### Server:

**Already implemented** in `getPassiveSkillBonuses()`:

```js
// Sword Mastery (100): +4 ATK/level with daggers & 1H swords
const smLv = learned[100] || 0;
if (smLv > 0 && (wType === 'dagger' || wType === 'one_hand_sword')) {
    bonuses.bonusATK += smLv * 4;
}
```

The `bonusATK` is added to the damage calculation as flat mastery damage (bypasses DEF/element modifiers).

#### Skill data:

```js
{ id: 100, name: 'sword_mastery', displayName: 'Sword Mastery', classId: 'swordsman',
  maxLevel: 10, type: 'passive', targetType: 'none', element: 'neutral', range: 0,
  description: 'Increases ATK with 1H Swords and Daggers.',
  icon: 'sword_mastery', treeRow: 0, treeCol: 0, prerequisites: [],
  levels: genLevels(10, i => ({
      level: i+1, spCost: 0, castTime: 0, afterCastDelay: 0,
      cooldown: 0, effectValue: (i+1)*4, duration: 0
  }))
},
```

#### Client VFX:

None. Passive skills have no VFX. The icon shows in the skill tree with the current level.

---

### 8.7 Toggle: Endure (ID 106)

Self-buff that grants flinch immunity and bonus MDEF. 10-second cooldown between uses.

#### Server:

**Already implemented.** See section 1.4 above. Key behavior:
- Grants `mdefBonus = level` MDEF
- Duration: `10000 + level * 3000` ms (10s to 40s)
- 10-second cooldown between uses
- Flinch immunity tracked by buff name "endure" — when checking if a character should flinch from damage, the system checks `target.activeBuffs.some(b => b.name === 'endure')`

**Toggle behavior note:**

In RO Classic, Endure is technically a one-shot buff (not a toggle with on/off state). It is used once, grants the buff for a duration, and goes on cooldown. The "toggle" classification in this project means it can be "toggled off" by re-casting to cancel the buff early (future enhancement).

```js
// To add toggle-off behavior:
if (skill.name === 'endure') {
    // Check if buff already active — cancel it
    const existingBuff = (player.activeBuffs || []).find(b => b.name === 'endure');
    if (existingBuff && Date.now() < existingBuff.expiresAt) {
        // Remove the buff (toggle off)
        player.activeBuffs = player.activeBuffs.filter(b => b.name !== 'endure');
        broadcastToZone(zone, 'skill:buff_removed', {
            targetId: characterId, isEnemy: false,
            buffName: 'endure', reason: 'cancelled'
        });
        // No SP cost or cooldown for cancellation
        socket.emit('skill:used', { ... });
        return;
    }

    // Normal activation (apply buff)
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);
    // ... buff application as shown in section 1.4 ...
}
```

#### Client VFX:

```cpp
// Endure — P_Ice_Proj_charge_01, plays once, scale 2.0
Add(106, ESkillVFXTemplate::SelfBuff, FLinearColor(1.f, 0.9f, 0.3f, 1.f), TEXT("neutral"),
    false, 0.f, false, 0.f, 0.f,
    TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Spider/ICE/P_Ice_Proj_charge_01.P_Ice_Proj_charge_01"),
    true, 2.0f, false, 1.5f);
```

---

## Appendix: Quick Reference Tables

### A. Skill ID Ranges by Class

| Class | ID Range | Example Skills |
|-------|----------|----------------|
| Novice | 1-10 | Basic Skill (1), First Aid (2), Play Dead (3) |
| Swordsman | 100-119 | Sword Mastery (100), Bash (103), Provoke (104), Endure (106) |
| Mage | 200-229 | Cold Bolt (200), Fire Bolt (201), Thunderstorm (212) |
| Archer | 300-319 | Owl's Eye (300), Double Strafe (303), Arrow Shower (304) |
| Acolyte | 400-429 | Heal (400), Blessing (402), Increase AGI (403) |
| Thief | 500-519 | Double Attack (500), Steal (502), Hiding (503) |
| Merchant | 600-619 | Discount (601), Mammonite (603), Vending (605) |
| Knight | 700-719 | Pierce (701), Bowling Bash (707), Twohand Quicken (705) |

### B. VFX Template Quick Reference

| Template | Used For | Key Config Fields |
|----------|----------|-------------------|
| `BoltFromSky` | Bolt skills | `BoltSpawnHeight`, `BoltInterval`, `bIsCascade` |
| `Projectile` | Projectile skills | `ProjectileSpeed`, `bSingleProjectile`, `ImpactOverridePath` |
| `AoEImpact` | Impact effects | `AoERadius`, `bSelfCentered` |
| `GroundPersistent` | Fire/Safety Wall | `bLooping`, `CascadeLifetime` |
| `GroundAoERain` | Thunderstorm | `AoERadius` |
| `SelfBuff` | Self buffs | `bLooping`, `CascadeLifetime` |
| `TargetDebuff` | Debuffs on target | `bLooping`, `CascadeLifetime`, `bIsCascade` |
| `HealFlash` | Healing effects | `Scale` |

### C. Damage Formula Quick Reference

**Physical skill damage:**
```
calculateSkillDamage(attackerStats, targetStats, targetHardDef,
    skillMultiplier, attackerBuffMods, targetBuffMods,
    targetInfo, attackerInfo, { skillElement })
```

**Magical skill damage:**
```
calculateMagicSkillDamage(attackerStats, targetStats, targetHardMdef,
    skillMultiplier, skillElement, targetInfo)
```

**Heal amount:**
```
floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)
```

**Cast time:**
```
actualCastTime = baseCastTime * (1 - DEX / 150)
```
