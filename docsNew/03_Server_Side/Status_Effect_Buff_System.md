# Status Effect & Buff System

## 1. Overview

The server implements two separate modifier systems that affect combat, movement, and stats:

| System | File | Storage | Purpose |
|--------|------|---------|---------|
| **Status Effects** | `ro_status_effects.js` | `target.activeStatusEffects` (Map) | 10 core CC/DoT conditions: stun, freeze, stone, sleep, poison, blind, silence, confusion, bleeding, curse |
| **Buffs** | `ro_buff_system.js` | `target.activeBuffs` (Array) | Stat-modifying skills with duration: provoke, endure, sight, blessing, increase AGI, etc. |

Both systems feed into `getCombinedModifiers(target)` in `index.js`, which merges all stat multipliers, flat bonuses, prevention flags, and special flags into a single object consumed by damage formulas, regen timers, movement validation, and AI logic.

---

## 2. ro_status_effects.js

### STATUS_EFFECTS Config Table

All 10 effects with their full configuration:

| Status | resistStat | baseDuration | breakOnDamage | preventsMovement | preventsCasting | preventsAttack | statMods | periodicDrain |
|--------|-----------|-------------|---------------|-----------------|----------------|---------------|----------|--------------|
| **stun** | `vit` (cap 97) | 5s | No | Yes | Yes | Yes | `fleeMultiplier: 0` | -- |
| **freeze** | `mdef` (no cap) | 12s | Yes | Yes | Yes | Yes | `defMul: 0.5, mdefMul: 1.25`, element override water/1 | -- |
| **stone** | `mdef` (no cap) | 20s | Yes | Yes | Yes | Yes | `defMul: 0.5, mdefMul: 1.25`, element override earth/1 | 3s delay, then 1% maxHP / 5s, min 1 HP |
| **sleep** | `int` (cap 97) | 30s | Yes | Yes | Yes | Yes | none | -- |
| **poison** | `vit` (cap 97) | 60s | No | No | No | No | `defMul: 0.75`, blocks SP regen | 1.5% maxHP + 2 flat / 1s, floor 25% HP |
| **blind** | `avg(int,vit)` (cap 193) | 30s | No | No | No | No | `hitMul: 0.75, fleeMul: 0.75` | -- |
| **silence** | `vit` (cap 97) | 30s | No | No | Yes | No | none | -- |
| **confusion** | `avg(str,int)` (cap 193) | 30s | Yes | No | No | No | none | -- |
| **bleeding** | `vit` (cap 97) | 120s | No | No | No | No | blocks HP + SP regen | 2% maxHP / 4s, can kill |
| **curse** | `luk` (cap 97) | 30s | No | No | No | No | `atkMul: 0.75, lukOverride: 0, moveSpdMul: 0.1` | -- |

Additional config flags per effect: `canKill`, `preventsItems`, `blocksHPRegen`, `blocksSPRegen`.

Damage-breakable statuses: `BREAKABLE_STATUSES = ['freeze', 'stone', 'sleep', 'confusion']`

### Resistance Formula

```
FinalChance = BaseChance - (BaseChance * ResistStat / 100) + srcLevel - tarLevel - tarLUK
```
- Clamped to `[5, 95]`
- LUK >= 300 grants full immunity
- Boss/MVP `statusImmune` flag grants full immunity
- No stacking: if the same status is already active, application is rejected

```
Duration = BaseDuration - (BaseDuration * ResistStat / 200) - 10 * tarLUK
```
- Minimum 1000ms

Resist stat resolution for composite stats:
- `avg_int_vit` = `floor((INT + VIT) / 2)`
- `avg_str_int` = `floor((STR + INT) / 2)`
- `mdef` = hard MDEF from equipment

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `calculateResistance` | `(source, target, statusType, baseChance)` | Returns `{ applied, duration, chance, reason }`. Checks boss immunity, duplicate, LUK cap, rolls vs resistance. |
| `applyStatusEffect` | `(source, target, statusType, baseChance, overrideDuration?)` | Calls `calculateResistance`, then creates entry in `target.activeStatusEffects` Map. Returns `{ applied, duration }`. |
| `forceApplyStatusEffect` | `(target, statusType, duration?)` | Bypasses resistance (for debug/special cases). |
| `removeStatusEffect` | `(target, statusType)` | Deletes from Map. Returns `boolean`. |
| `cleanse` | `(target, statusTypes[])` | Removes multiple statuses at once (for Cure, Recovery skills). Returns array of removed types. |
| `checkDamageBreakStatuses` | `(target)` | Removes all breakable statuses (freeze/stone/sleep/confusion). Called after ANY damage. Returns array of broken types. |
| `tickStatusEffects` | `(target, now)` | Called from 1s server tick. Handles expiry and periodic HP drains. Returns `{ expired[], drains[] }`. |
| `getStatusModifiers` | `(target)` | Aggregates all active status mods into a single object: prevention flags, stat multipliers, element overrides, individual `isFrozen`/`isStunned`/etc. flags. |
| `hasStatusEffect` | `(target, statusType)` | Returns `boolean`. |
| `getActiveStatusList` | `(target)` | Serializable array for `buff:list` event: `{ type, duration, remainingMs }`. |

### activeStatusEffects Entry Shape

```js
{
    type: 'freeze',
    appliedAt: 1709000000000,
    duration: 8000,
    expiresAt: 1709000008000,
    sourceId: 42,
    sourceLevel: 25,
    lastDrainTime: null   // set on first drain tick for periodic effects
}
```

---

## 3. ro_buff_system.js

### BUFF_TYPES Config

Each entry defines metadata and stacking behavior. Currently registered types:

| Buff Name | Category | Abbrev | stackRule | Status |
|-----------|----------|--------|-----------|--------|
| `provoke` | debuff | PRV | refresh | Active |
| `endure` | buff | END | refresh | Active |
| `sight` | buff | SGT | refresh | Active |
| `blessing` | buff | BLS | refresh | Active (Phase 5) |
| `increase_agi` | buff | AGI | refresh | Active (Phase 5) |
| `decrease_agi` | debuff | DAG | refresh | Active (Phase 5) |
| `angelus` | buff | ANG | refresh | Active (Phase 5) |
| `pneuma` | buff | PNE | refresh | Active (Phase 5) |
| `signum_crucis` | debuff | SCR | refresh | Active (Phase 5) |
| `auto_berserk` | buff | BRK | refresh | Active (Phase 5) — +32% ATK / -25% DEF below 25% HP, emits `player:stats` on toggle |
| `hiding` | buff | HID | refresh | Active (Phase 5) — breaks on damage/skill use |
| `improve_concentration` | buff | CON | refresh | Active (Phase 5) |
| `loud_exclamation` | buff | LXC | refresh | Active (Phase 5) |
| `ruwach` | buff | RUW | refresh | Active (Phase 5) |
| `energy_coat` | buff | ENC | refresh | Active (Phase 5) |
| `two_hand_quicken` | buff | THQ | refresh | Future (2nd class) |
| `kyrie_eleison` | buff | KYR | refresh | Future (2nd class) |
| `magnificat` | buff | MAG | refresh | Future (2nd class) |
| `gloria` | buff | GLO | refresh | Future (2nd class) |
| `lex_aeterna` | debuff | LEX | refresh | Future (2nd class) |
| `aspersio` | buff | ASP | refresh | Future (2nd class) |
| `enchant_poison` | buff | EPO | refresh | Future (2nd class) |
| `cloaking` | buff | CLK | refresh | Future (2nd class) |
| `quagmire` | debuff | QUA | refresh | Future (2nd class) |

### stackRule Options

| Rule | Behavior |
|------|----------|
| `refresh` | Replace existing buff of same name (most common) |
| `stack` | Allow multiple instances (rare) |
| `reject` | Do not overwrite if already active |

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `applyBuff` | `(target, buffDef)` | Applies buff per stackRule. `buffDef` must include `{ name, skillId, casterId, casterName, duration }` plus optional stat fields (`defReduction`, `atkIncrease`, `mdefBonus`, etc.). Returns `boolean`. |
| `removeBuff` | `(target, buffName)` | Removes by name. Returns the removed buff object or `null`. |
| `expireBuffs` | `(target)` | Removes all expired buffs. Returns array of expired buff objects. Called from 1s tick. |
| `hasBuff` | `(target, buffName)` | Returns `boolean`. |
| `getBuffModifiers` | `(target)` | Aggregates all active buff stat mods: multipliers (`defMultiplier`, `atkMultiplier`, `aspdMultiplier`), flat bonuses (`strBonus`..`lukBonus`, `defPercent`, `moveSpeedBonus`, `bonusHit`, `bonusFlee`, `bonusCritical`), special flags (`weaponElement`, `isHidden`, `doubleNextDamage`, `blockRanged`, `energyCoatActive`). |
| `getActiveBuffList` | `(target)` | Serializable array for `buff:list` event: `{ name, skillId, duration, remainingMs, category, displayName, abbrev }`. |

### getBuffModifiers Switch Cases

The function applies per-buff-name logic:

- **provoke**: `defMultiplier *= (1 - defReduction/100)`, `atkMultiplier *= (1 + atkIncrease/100)`
- **endure**: `bonusMDEF += mdefBonus`
- **blessing**: `strBonus`, `dexBonus`, `intBonus`
- **increase_agi**: `agiBonus`, `moveSpeedBonus`
- **decrease_agi**: subtracts `agiReduction`, `moveSpeedReduction`
- **auto_berserk**: `atkMultiplier *= 1.32`, `defMultiplier *= 0.75` (when `atkIncrease > 0`)
- **angelus**: `defPercent` (increases VIT soft DEF by %. Applied in `ro_damage_formulas.js` as `softDEF * (1 + defPercent/100)`)
- **gloria**: `lukBonus`
- **improve_concentration**: `agiBonus`, `dexBonus`
- **two_hand_quicken**: `aspdMultiplier *= (1 + aspdIncrease/100)`
- **lex_aeterna**: `doubleNextDamage = true`
- **aspersio**: `weaponElement = 'holy'`
- **enchant_poison**: `weaponElement = 'poison'`
- **pneuma**: `blockRanged = true`
- **energy_coat**: `energyCoatActive = true` (dynamic reduction handled by `applyEnergyCoat()` at damage time, not a flat modifier)
- **hiding/cloaking**: `isHidden = true`
- **quagmire**: subtracts `agiReduction`, `dexReduction`, `moveSpeedReduction`
- **default**: generic fallback checks `defReduction`, `atkIncrease`, `mdefBonus`

---

## 4. index.js Integration

### Imports

```js
const { STATUS_EFFECTS, BREAKABLE_STATUSES, calculateResistance,
    applyStatusEffect, forceApplyStatusEffect, removeStatusEffect,
    cleanse, checkDamageBreakStatuses, tickStatusEffects,
    getStatusModifiers, hasStatusEffect, getActiveStatusList
} = require('./ro_status_effects');

const { BUFF_TYPES, applyBuff: applyBuffGeneric, removeBuff, hasBuff,
    getBuffModifiers, getActiveBuffList
} = require('./ro_buff_system');

const { expireBuffs: expireBuffsGeneric } = require('./ro_buff_system');
```

### getCombinedModifiers(target)

Merges `getStatusModifiers(target)` and `getBuffModifiers(target)` into a single object:

- **Multiplicative merge**: `defMultiplier = status.defMultiplier * buff.defMultiplier` (same for `atkMultiplier`)
- **Pass-through from status**: `mdefMultiplier`, `hitMultiplier`, `fleeMultiplier`, `moveSpeedMultiplier`, `lukOverride`, `overrideElement`, all prevention flags, all individual `is*` flags
- **Pass-through from buffs**: `aspdMultiplier`, `bonusMDEF`, all stat bonuses (`strBonus`..`lukBonus`), `defPercent`, `moveSpeedBonus`, `bonusHit`, `bonusFlee`, `bonusCritical`, `weaponElement`, `isHidden`, `doubleNextDamage`, `blockRanged`, `energyCoatActive`

### getBuffStatModifiers(target) (Alias)

```js
function getBuffStatModifiers(target) {
    return getCombinedModifiers(target);
}
```

Backward-compatible alias. All existing skill handlers call `getBuffStatModifiers()` for CC checks; internally this now returns merged status + buff modifiers.

### buildFullStatsPayload() — Buff Multipliers in Stats Panel

The `buildFullStatsPayload()` function applies `buffAtkMultiplier` and `buffDefMultiplier` from `getEffectiveStats()` to the derived stats sent via `player:stats`:

```js
statusATK: Math.floor(derived.statusATK * (effectiveStats.buffAtkMultiplier || 1)),
softDEF: Math.floor(derived.softDEF * (effectiveStats.buffDefMultiplier || 1)),
```

This means all buffs with ATK/DEF multipliers (Auto Berserk, Provoke, etc.) are reflected in the CombatStatsWidget in real-time.

### checkAutoBerserk() — Stats Emit on Toggle

`checkAutoBerserk(player, characterId, zone)` monitors the HP ratio. When HP drops below 25%, it activates the buff (+32% ATK, -25% DEF). When HP rises to >= 25%, it deactivates (sets `atkIncrease = 0`). On each toggle, it emits `player:stats` via `io.sockets.sockets.get(player.socketId)` so the CombatStatsWidget updates ATK/DEF immediately.

Called from all HP-changing paths: enemy damage, heal, HP regen, skill regen, First Aid, item/potion use, Fire Wall tick on players, PvP auto-attack.

### Movement Lock (`player:position` handler)

```js
const ccMods = getCombinedModifiers(player);
if (ccMods.preventsMovement) {
    socket.emit('player:position_rejected', {
        x: player.lastX, y: player.lastY, z: player.lastZ,
        reason: 'cc_locked'
    });
    return; // rejects the position update
}
```

Active statuses that set `preventsMovement: true` (stun, freeze, stone, sleep) cause the server to reject any position updates and send the player back to their last known position.

### Regen Blocking

- **HP Regen (6s interval)**: Skipped when `getCombinedModifiers(player).blocksHPRegen` is true. Bleeding sets this flag.
- **SP Regen (8s interval)**: Skipped when `getCombinedModifiers(player).blocksSPRegen` is true. Poison and bleeding set this flag.

### AI Movement Lock (Enemy AI tick, 200ms)

```js
const enemyCCMods = getCombinedModifiers(enemy);
if (enemyCCMods.preventsMovement || enemyCCMods.preventsAttack) {
    // Stop wandering, stop chasing, skip attack
}
```

Enemies under full CC (stun, freeze, stone, sleep) are frozen in place and cannot act.

### Damage-Break-All-Statuses

Every damage path calls `checkDamageBreakStatuses(target)` after dealing damage:

- Auto-attack vs enemy
- Auto-attack vs player (PvP)
- Bolt skills (Cold Bolt, Fire Bolt, Lightning Bolt)
- Fire Ball (per-target in AoE)
- Fire Wall (per-tick)

For each broken status type, the server emits both `status:removed` (new system) and `skill:buff_removed` (backward compat for VFX).

### Combat Tick CC Check

In the 50ms combat tick loop, before processing each player's attack state:

```js
const attackerCCMods = getCombinedModifiers(attacker);
if (attackerCCMods.preventsAttack) continue; // skip this attacker's turn
```

### skill:use CC Check

Every skill handler checks CC before executing:

```js
const casterBuffs = getBuffStatModifiers(player); // alias for getCombinedModifiers
if (casterBuffs.preventsCasting) {
    socket.emit('skill:error', { message: 'Cannot cast while incapacitated' });
    return;
}
```

The initial `skill:use` handler also checks with a more detailed message:

```js
const ccMods = getCombinedModifiers(player);
if (ccMods.preventsCasting) {
    // Returns specific reason: "Cannot use skills while frozen/petrified/stunned/sleeping/silenced"
}
```

### combat:attack CC Check

```js
const attackCCMods = getCombinedModifiers(attacker);
if (attackCCMods.preventsAttack) {
    socket.emit('combat:error', { message: 'Cannot attack while incapacitated' });
    return;
}
```

### 1s Tick Loop (Status + Buff Expiry)

The 1-second server tick processes both systems for all connected players and active enemies:

```js
// Players
const { expired: statusExpired, drains: statusDrains } = tickStatusEffects(player, now);
// emit status:removed for each expired, status:tick for each drain
const expiredBuffs = expireBuffs(player);
// emit buff:removed for each expired buff, skill:buff_removed for VFX

// Enemies (same pattern)
const { expired: eStatusExpired, drains: eStatusDrains } = tickStatusEffects(enemy, now);
const eExpiredBuffs = expireBuffs(enemy);
```

---

## 5. Socket Events

### Status Effect Events

| Event | Direction | Payload | When Emitted |
|-------|-----------|---------|-------------|
| `status:applied` | Server -> Zone | `{ targetId, isEnemy, statusType, duration, sourceId, sourceName }` | Status effect lands on a target |
| `status:removed` | Server -> Zone | `{ targetId, isEnemy, statusType, reason }` | Status expires (`'expired'`), broken by damage (`'damage_break'`), or removed by debug (`'debug'`) |
| `status:tick` | Server -> Zone | `{ targetId, isEnemy, statusType, drain, newHealth, maxHealth }` | Periodic HP drain tick (poison, bleeding, stone) |

### Buff Events

| Event | Direction | Payload | When Emitted |
|-------|-----------|---------|-------------|
| `buff:list` | Server -> Client | `{ characterId, buffs[], statuses[] }` | On `player:join`, sends full current state. `buffs` from `getActiveBuffList()`, `statuses` from `getActiveStatusList()`. |
| `buff:removed` | Server -> Zone | `{ targetId, isEnemy, buffName, reason }` | Buff expires naturally |

### Movement Rejection

| Event | Direction | Payload | When Emitted |
|-------|-----------|---------|-------------|
| `player:position_rejected` | Server -> Client | `{ x, y, z, reason: 'cc_locked' }` | Player sends position update while CC'd |

### Backward-Compat VFX Events

| Event | Direction | Payload | When Emitted |
|-------|-----------|---------|-------------|
| `skill:buff_applied` | Server -> Zone | `{ targetId, targetName, isEnemy, casterId, casterName, skillId, buffName, duration, effects }` | Provoke, Endure, Sight applied; also Frost Diver freeze and Stone Curse petrify (for VFX system) |
| `skill:buff_removed` | Server -> Zone | `{ targetId, isEnemy, buffName, reason }` | Any status/buff removed (emitted alongside `status:removed` or `buff:removed`) |

### Debug Events (dev only)

| Event | Direction | Payload | Guard |
|-------|-----------|---------|-------|
| `debug:apply_status` | Client -> Server | `{ targetId, isEnemy, statusType, duration? }` | `NODE_ENV !== 'development'` returns early |
| `debug:remove_status` | Client -> Server | `{ targetId, isEnemy, statusType }` | Same guard |
| `debug:list_statuses` | Client -> Server | `{ targetId, isEnemy }` | Same guard; responds with `debug:status_list` |

---

## 6. How to Add a New Status Effect

**Step 1**: Add entry to `STATUS_EFFECTS` in `ro_status_effects.js`:

```js
new_status: {
    resistStat: 'vit',       // 'vit', 'int', 'luk', 'mdef', 'avg_int_vit', 'avg_str_int'
    resistCap: 97,            // null for no cap
    baseDuration: 10000,
    canKill: false,
    breakOnDamage: false,
    preventsMovement: false,
    preventsCasting: false,
    preventsAttack: false,
    preventsItems: false,
    blocksHPRegen: false,
    blocksSPRegen: false,
    statMods: { /* multipliers/overrides */ },
    // Optional: periodicDrain: { startDelay, interval, hpPercent, flatDrain, minHpPercent }
    // Optional: elementOverride: { type, level }
}
```

**Step 2**: Add individual flag to `getStatusModifiers()` if needed:

```js
// In the mods object initialization:
isNewStatus: false,
// In the switch:
case 'new_status': mods.isNewStatus = true; break;
```

**Step 3**: If breakable by damage, add to `BREAKABLE_STATUSES` array.

The skill handler in `index.js` calls `applyStatusEffect(source, target, 'new_status', baseChance)` and broadcasts `status:applied` on success. Everything else (tick processing, expiry, modifier aggregation) works automatically.

---

## 7. How to Add a New Buff

**Step 1**: Add entry to `BUFF_TYPES` in `ro_buff_system.js`:

```js
new_buff: {
    stackRule: 'refresh',
    category: 'buff',        // 'buff' or 'debuff'
    displayName: 'New Buff',
    abbrev: 'NBF'            // 3-char abbreviation for BuffBarWidget
}
```

**Step 2**: Add a case to the `getBuffModifiers()` switch:

```js
case 'new_buff':
    mods.someBonus += (buff.someBonus || 0);
    break;
```

**Step 3**: In the skill handler in `index.js`, call `applyBuff(target, buffDef)` where `buffDef` includes `{ name: 'new_buff', skillId, casterId, casterName, duration, ...statFields }`. Broadcast `skill:buff_applied` for VFX and optionally `buff:removed` is handled automatically by the 1s tick.

---

## 8. Backward Compatibility Notes

### skill:buff_applied Still Emits for VFX

The `SkillVFXSubsystem` (C++ client) listens to `skill:buff_applied` for Pattern D (persistent buff VFX). When a status effect like freeze or stone is applied, the server emits both:

1. `status:applied` (new system, consumed by `BuffBarSubsystem`)
2. `skill:buff_applied` (backward compat, consumed by `SkillVFXSubsystem`)

Similarly, when removed:

1. `status:removed` (new system)
2. `skill:buff_removed` (backward compat for VFX cleanup)

### getBuffStatModifiers Is an Alias

`getBuffStatModifiers(target)` is a direct alias for `getCombinedModifiers(target)`. All existing skill handlers use `getBuffStatModifiers` for CC checks. The function now returns merged status + buff modifiers instead of buff-only modifiers, which is safe because status effect prevention flags were not previously present in the return value (they default to `false`).

### Data Storage Separation

Despite using a unified modifier accessor, the two systems store data separately:

- Status effects: `target.activeStatusEffects` (Map, keyed by status type string)
- Buffs: `target.activeBuffs` (Array of buff objects)

This separation allows independent lifecycle management: status effects have resistance formulas and periodic drains; buffs have stack rules and skill-specific stat fields.
