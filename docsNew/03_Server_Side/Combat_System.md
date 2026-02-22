# Combat System — Server-Side Documentation

## Overview

The combat system is **server-authoritative** and inspired by Ragnarok Online. Players click a target once to initiate auto-attack; the server processes attacks at ASPD-based intervals via a 50ms tick loop. Supports both Player-vs-Player (PvP) and Player-vs-Enemy (PvE) combat.

## Architecture

```
Client clicks target
    → emit('combat:attack', {targetEnemyId OR targetCharacterId})
    → Server validates, sets autoAttackState
    → emit('combat:auto_attack_started') to attacker

Every 50ms (Combat Tick Loop):
    For each entry in autoAttackState:
        1. Check attacker alive & connected
        2. Check ASPD timing (enough time elapsed?)
        3. Get positions from Redis
        4. Check range (distance <= attackRange)
           - Out of range → emit('combat:out_of_range')
           - In range → calculate damage, apply, broadcast
        5. If target HP <= 0 → process death
```

## Constants

```javascript
const COMBAT = {
    BASE_DAMAGE: 1,           // Flat damage per hit
    DAMAGE_VARIANCE: 0,       // Random bonus (0 = no variance currently)
    MELEE_RANGE: 150,         // Default melee range (UE units)
    RANGED_RANGE: 800,        // Default ranged range (bow)
    DEFAULT_ASPD: 175,        // Default attack speed (0-195 scale, higher = faster)
    ASPD_CAP: 195,            // Hard cap — above this, diminishing returns apply up to 199
    RANGE_TOLERANCE: 50,      // Padding so client moves INSIDE range
    COMBAT_TICK_MS: 50,       // Tick loop interval
    RESPAWN_DELAY_MS: 5000,   // Not actively used (respawn is client-initiated)
    SPAWN_POSITION: { x: 0, y: 0, z: 300 }  // Respawn coordinates
};
```

## Attack Speed (ASPD) System

ASPD ranges from 170 (new player) to a theoretical maximum of 199. The **hard cap is 195** — above 195, diminishing returns apply, making 190+ an endgame progression goal.

### ASPD Calculation

```javascript
// In calculateDerivedStats() - Square root scaling
const agiContribution = Math.floor(Math.sqrt(agi) * 1.2);      // Slow sqrt growth
const dexContribution = Math.floor(Math.sqrt(dex) * 0.6);      // Half of AGI impact
const aspd = Math.min(COMBAT.ASPD_CAP, Math.floor(170 + agiContribution + dexContribution));

// Final player ASPD (with weapon modifier applied after)
player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + weaponAspdMod);

// IMPORTANT: player:stats event now sends final ASPD including weapon modifier
const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + weaponAspdMod);
socket.emit('player:stats', {
    characterId,
    stats: {
        ...baseStats,
        hardDef: player.hardDef || 0  // Include total hard DEF from armor
    },
    derived: { ...derived, aspd: finalAspd }  // Final ASPD with weapon bonus
});
```

**Defense (DEF) System:**
- **softDEF**: Calculated from VIT stat only (`VIT*0.5 + VIT²/150`) - included in `derived.softDEF`
- **hardDef**: Sum of DEF values from all equipped armor pieces - included in `stats.hardDef`
- **Total DEF**: Used in damage calculations as `target.hardDef` for damage reduction

**Square root scaling** - slow, consistent growth that makes 195 a true endgame goal:
- AGI uses `sqrt(agi) × 1.2` - gradual diminishing returns
- DEX uses `sqrt(dex) × 0.6` - half the impact of AGI (primary stat for ASPD)
- Each stat point provides less benefit as you invest more (prevents power creep)
- Requires significant stat investment + weapon bonuses to reach cap

**Example progression:**
| Build | AGI | DEX | ASPD (no weapon) | Status |
|-------|-----|-----|------------------|--------|
| New player | 1 | 1 | 170 | Baseline |
| Early game | 20 | 15 | 176 | Starting to feel faster |
| Mid game | 50 | 30 | 182 | Noticeable improvement |
| High mid | 80 | 65 | 186 | Significant investment required |
| Late game | 90 | 80 | 188 | Major stat commitment |
| Max stats | 99 | 99 | 190 | Endgame without gear |
| Max + Dagger | 99 | 99 | 195 | **Hard cap achieved** |
| Max + potions | 99 | 99 | 195+ | Diminishing returns range |

### Attack Interval Calculation

```javascript
function getAttackIntervalMs(aspd) {
    if (aspd <= COMBAT.ASPD_CAP) {
        // Linear formula up to hard cap (195)
        return (200 - aspd) * 50;
    } else {
        // Diminishing returns above 195: exponential decay
        const excessAspd = Math.min(aspd - COMBAT.ASPD_CAP, 9);
        const decayFactor = Math.exp(-excessAspd * 0.35);
        const maxBonus = 130;
        const actualBonus = Math.floor(maxBonus * (1 - decayFactor));
        return Math.max(217, 250 - actualBonus); // Floor ~217ms
    }
}
```

### ASPD Reference Table

**Linear range (below hard cap):**
| ASPD | Interval | Attacks/Second |
|------|----------|---------------|
| 170 | 1500ms | 0.67 |
| 175 | 1250ms | 0.80 |
| 180 | 1000ms | 1.00 |
| 185 | 750ms | 1.33 |
| 190 | 500ms | 2.00 |
| 195 | 250ms | 4.00 |

**Diminishing returns range (above hard cap):**
| ASPD | Interval | Attacks/Second | Gain vs 195 |
|------|----------|----------------|-------------|
| 195 | 250ms | 4.00 | baseline |
| 196 | 244ms | 4.10 | +0.10 |
| 197 | 237ms | 4.22 | +0.22 |
| 198 | 229ms | 4.37 | +0.37 |
| 199 | 217ms | 4.61 | +0.61 |

The square root formula makes 195 ASPD a true endgame achievement. Even with maximum base stats (99 AGI, 99 DEX), players still need weapon bonuses (+5 from dagger) to reach the hard cap. This creates meaningful long-term progression and prevents early-game power creep.

### Weapon ASPD Modifiers
- Dagger: +5
- One-hand sword: 0
- Bow: -3

### SPEED_TONICS (ASPD Consumables)

Temporary ASPD boosts applied at combat time. The `aspdBonus` is added to the player's current ASPD before `getAttackIntervalMs()` is called.

```javascript
const SPEED_TONICS = {
    'veil_draught':   { duration: 60000,  aspdBonus: 5 },  // Short burst, big bonus
    'dusk_tincture':  { duration: 180000, aspdBonus: 3 },  // Sustained, smaller bonus
    'ember_salve':    { duration: 30000,  aspdBonus: 8 },  // Very short, large bonus
    'grey_catalyst':  { duration: 300000, aspdBonus: 2 },  // Long duration, minor bonus
};
```

| Tonic | Duration | ASPD Bonus | Use Case |
|-------|----------|------------|----------|
| `veil_draught` | 60s | +5 | Boss burst window |
| `dusk_tincture` | 180s | +3 | Sustained farming |
| `ember_salve` | 30s | +8 | Emergency/clutch |
| `grey_catalyst` | 300s | +2 | Long dungeon runs |

> ⚠️ **Note**: SPEED_TONICS are defined but not yet wired to a consumption/inventory event. Integration with the inventory system is pending.

## Damage Calculation

### Player vs Enemy (Current: Simplified)
```javascript
const damage = COMBAT.BASE_DAMAGE + Math.floor(Math.random() * (COMBAT.DAMAGE_VARIANCE + 1));
// Currently: always 1 damage (BASE_DAMAGE=1, DAMAGE_VARIANCE=0)
```

### Full RO-Style Formula (Available but not used in PvE tick)
```javascript
function calculatePhysicalDamage(attackerStats, targetStats) {
    const atkDerived = calculateDerivedStats(attackerStats);
    const defDerived = calculateDerivedStats(targetStats);
    const weaponATK = attackerStats.weaponATK || 0;
    const totalATK = atkDerived.statusATK + weaponATK;
    const variance = 0.8 + Math.random() * 0.2;        // 80-100% damage variance
    const rawDamage = Math.floor(totalATK * variance);
    const damage = Math.max(1, rawDamage - defDerived.softDEF);
    const isCritical = Math.random() * 100 < atkDerived.critical;
    const finalDamage = isCritical ? Math.floor(damage * 1.4) : damage;
    return { damage: Math.max(1, finalDamage), isCritical };
}
```

## Range Checking

Range is checked using Redis-cached positions:

```javascript
const attackerPos = await getPlayerPosition(attackerId);
const dx = attackerPos.x - target.x;
const dy = attackerPos.y - target.y;
const distance = Math.sqrt(dx * dx + dy * dy);

if (distance > attacker.attackRange) {
    // Out of range — emit with padded range
    socket.emit('combat:out_of_range', {
        targetId, isEnemy, targetX, targetY, targetZ,
        distance,
        requiredRange: Math.max(0, attacker.attackRange - COMBAT.RANGE_TOLERANCE)
    });
}
```

`requiredRange` uses `attackRange - RANGE_TOLERANCE (50)` so the client moves INSIDE the attack range rather than stopping at the boundary (which could cause repeated out-of-range failures).

## Target Switching

When a player sends `combat:attack` while already attacking:

1. Compare new target to old target
2. If different target:
   - Remove attacker from old enemy's `inCombatWith` set
   - Delete old `autoAttackState`
   - Emit `combat:auto_attack_stopped` with reason "Switched target"
3. Set new `autoAttackState`
4. Emit `combat:auto_attack_started` with new target info

## Death Processing

### Player Death
```
1. Set target.isDead = true
2. Clear ALL auto-attackers targeting this player (without sending combat:target_lost)
3. Broadcast combat:death to all
4. Broadcast kill message in COMBAT chat channel
5. Save target health (0) to DB
```

**Critical**: `combat:target_lost` is NOT sent alongside `combat:death`. Sending both causes a Blueprint race condition where `target_lost` nullifies the player reference before `combat:death` tries to access it → EXCEPTION_ACCESS_VIOLATION crash.

### Enemy Death
```
1. Set enemy.isDead = true
2. Clear ALL auto-attackers for this enemy (without combat:target_lost)
3. Clear enemy.inCombatWith set
4. Broadcast enemy:death to all
5. Roll loot drops → emit loot:drop to killer only
6. Schedule respawn timer (enemy.respawnMs)
```

## Respawn

Player-initiated via `combat:respawn` event:

```
1. Verify player is dead
2. Stop all attackers targeting this player (emit combat:target_lost)
3. Restore health = maxHealth, mana = maxMana, isDead = false
4. Save to DB
5. Update Redis position to SPAWN_POSITION (0, 0, 300)
6. Broadcast combat:respawn with teleport=true
```

## Auto-Attack State Cleanup

Auto-attack entries are removed when:
- Attacker disconnects
- Attacker dies
- Target dies / disconnects / respawns
- Player sends `combat:stop_attack`
- Player switches to different target
- Enemy dies

## Event Flow Diagram

```
                    combat:attack
Client ──────────────────────────────► Server
                                         │
                                    Validate target
                                    Set autoAttackState
                                         │
              combat:auto_attack_started  │
Client ◄─────────────────────────────────┘
                                         │
                                   Combat Tick (50ms)
                                    ┌────┴────┐
                                    │ In Range?│
                                    └────┬────┘
                               No   │         │  Yes
                                    ▼         ▼
                          combat:out_of_range  Calculate damage
                                              Apply to target
                                              │
                          combat:damage        │
All Clients ◄──────────────────────────────────┘
                                              │
                                         [HP <= 0?]
                                              │ Yes
                          combat:death / enemy:death
All Clients ◄──────────────────────────────────┘
                                              │
                          loot:drop (killer)   │ (enemy only)
Killer ◄──────────────────────────────────────┘
```

---

**Last Updated**: 2026-02-18
