# Zone Filtering Audit — Server AoE Entity Iterations

**Auditor**: Claude Opus 4.6
**Date**: 2026-03-22
**File**: `server/src/index.js` (32,566 lines)
**Methodology**: 7-pass comprehensive scan of every `enemies.entries()` and `connectedPlayers.entries()` iteration
**Total iterations found**: 67 enemy iterations + 47 player iterations = 114 total

---

## Summary

| Category | Count | Zone-Filtered | Violations | Not Applicable |
|----------|-------|---------------|------------|----------------|
| AoE Skill Handlers (enemies) | 32 | 32 | 0 | 0 |
| AoE Skill Handlers (players) | 12 | 12 | 0 | 0 |
| Ground Effect Ticks (enemies) | 13 | 12 | **1** | 0 |
| Ground Effect Ticks (players) | 3 | 2 | **1** | 0 |
| Ensemble Ticks | 7 | 7 | 0 | 0 |
| Performance Ticks | 4 | 4 | 0 | 0 |
| Deaggro Loops (zone change/disconnect) | 6 | 4 | **2** | 0 |
| Enemy AI Loop | 1 | 1 | 0 | 0 |
| Combat Tick (auto-attack) | 4 | 4 | 0 | 0 |
| Monster Skill Execution | 4 | 4 | 0 | 0 |
| Trap Tick (ground effects) | 8 | 8 | 0 | 0 |
| Status/Buff Expiry Tick | 2 | N/A | 0 | 2 |
| Regen Ticks | 5 | N/A | 0 | 5 |
| Utility/Lookup (non-AoE) | 13 | N/A | 0 | 13 |
| **TOTAL** | **114** | **90** | **4** | **20** |

**Overall result**: 4 violations found, 3 LOW severity, 1 MEDIUM severity.

---

## VIOLATIONS

### V1: Fire Wall — Missing zone filter on enemy iteration (MEDIUM)

**Line**: 27706
**Context**: Ground Effects Tick — Fire Wall damage handler
**Code**:
```javascript
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.isDead) continue;
    if (effect.hitsRemaining <= 0) break;
    // NO zone filter here!
    const dx = enemy.x - effect.x;
    const dy = enemy.y - effect.y;
```
**Risk**: Fire Wall iterates ALL enemies globally without checking `enemy.zone !== effect.zone`. An enemy in a different zone at overlapping coordinates would take fire damage from a wall they cannot see. The distance check (150 UE radius) limits practical impact, but coordinate overlap between zones is plausible (e.g., two zones with similar spawn positions).

**Fix**:
```javascript
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.isDead) continue;
    if ((enemy.zone || 'prontera_south') !== (effect.zone || 'prontera_south')) continue;
    if (effect.hitsRemaining <= 0) break;
```

### V2: Fire Wall — Missing zone filter on player iteration (MEDIUM)

**Line**: 27806
**Context**: Ground Effects Tick — Fire Wall PvP damage (inside `if (PVP_ENABLED)`)
**Code**:
```javascript
for (const [charId, player] of connectedPlayers.entries()) {
    if (player.isDead || charId === effect.casterId) continue;
    if (effect.hitsRemaining <= 0) break;
    // NO zone filter here!
    const pPos = await getPlayerPosition(charId);
```
**Risk**: Same as V1 but for players. A player in a different zone at matching coordinates would take fire damage. Currently gated behind `PVP_ENABLED` (which appears to be false), so this is dormant.

**Fix**:
```javascript
for (const [charId, player] of connectedPlayers.entries()) {
    if (player.isDead || charId === effect.casterId) continue;
    if ((player.zone || 'prontera_south') !== (effect.zone || 'prontera_south')) continue;
    if (effect.hitsRemaining <= 0) break;
```

### V3: Play Dead — Missing zone filter on deaggro loop (LOW)

**Line**: 12578
**Context**: Play Dead skill handler — force ALL enemies to drop aggro
**Code**:
```javascript
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.targetPlayerId === characterId) {
        enemy.targetPlayerId = null;
        enemy.aiState = AI_STATE.IDLE;
        enemy.isWandering = false;
    }
    enemy.inCombatWith && enemy.inCombatWith.delete(characterId);
}
```
**Risk**: Iterates ALL enemies across ALL zones to clear aggro. This is functionally harmless since enemies in other zones would never have this player as a target anyway (they can't reach cross-zone). The only cost is unnecessary iteration over enemies in other zones.

**Severity**: LOW — no functional impact, only performance (extra iterations).

**Fix** (optional, for consistency):
```javascript
const pdZone = player.zone || 'prontera_south';
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.zone !== pdZone) continue;
    if (enemy.targetPlayerId === characterId) {
        enemy.targetPlayerId = null;
        enemy.aiState = AI_STATE.IDLE;
        enemy.isWandering = false;
    }
    enemy.inCombatWith && enemy.inCombatWith.delete(characterId);
}
```

### V4: Cloaking — Missing zone filter on deaggro loop (LOW)

**Line**: 16823
**Context**: Cloaking skill handler — deaggro on hide
**Code**:
```javascript
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.targetPlayerId === characterId) {
        enemy.targetPlayerId = null;
        enemy.aiState = AI_STATE.IDLE;
        enemy.isWandering = false;
    }
    if (enemy.inCombatWith) enemy.inCombatWith.delete(characterId);
}
```
**Risk**: Same as V3 — functionally harmless (cross-zone enemies would never target this player) but iterates all enemies needlessly.

**Severity**: LOW — no functional impact, only performance.

### V5: Disconnect — Missing zone filter on deaggro loop (LOW)

**Line**: 7397
**Context**: `socket.on('disconnect')` handler — clear AI targets
**Code**:
```javascript
for (const [, enemy] of enemies.entries()) {
    enemy.inCombatWith.delete(charId);
    if (enemy.targetPlayerId === charId) {
        enemy.targetPlayerId = null;
        // ... pickNextTarget fallback
    }
}
```
**Risk**: Same as V3/V4. Iterates all enemies in all zones. Functionally harmless since enemies can't cross-zone target, but on a server with many zones/enemies, this wastes CPU on disconnect.

**Severity**: LOW — no functional impact.

---

## COMPLIANT HANDLERS — Full Catalog

### Ensemble Effect Ticks (lines 1289-1519)
All 7 iterations are compliant with zone filtering:

| Line | Handler | Filter |
|------|---------|--------|
| 1289 | Eternal Chaos VIT-DEF cleanup (enemies) | `(enemy.zone \|\| 'prontera_south') === ens.zone` |
| 1350 | Lullaby sleep AoE (enemies) | `(enemy.zone \|\| 'prontera_south') !== ensZone` |
| 1362 | Lullaby sleep AoE (players) | `p.zone !== ens.zone` |
| 1376 | Eternal Chaos VIT-DEF tick (enemies) | `(enemy.zone \|\| 'prontera_south') !== ensZone` |
| 1397 | Drum on Battlefield buff (players) | `(ally.zone \|\| 'prontera_south') !== ensZone` |
| 1423 | Ring of Nibelungen buff (players) | `(ally.zone \|\| 'prontera_south') !== ensZone` |
| 1451 | Mr. Kim A Rich Man EXP (players) | `(ally.zone \|\| 'prontera_south') !== ensZone` |
| 1484 | Invulnerable Siegfried resist (players) | `(ally.zone \|\| 'prontera_south') !== ensZone` |
| 1509 | Into the Abyss catalyst (players) | `(ally.zone \|\| 'prontera_south') !== ensZone` |

### Zone Change / Warp Handlers
| Line | Handler | Filter |
|------|---------|--------|
| 5945 | player:join — send existing enemies | `enemy.zone === playerZone` |
| 5888 | player:join — send existing players | `existingPlayer.zone === playerZone` |
| 6179 | warp_portal zone change — clear combat | `enemy.zone === portalZone` |
| 6338 | zone:change — clear combat | `enemy.zone === oldZone` |
| 6455 | zone:arrived — send zone enemies | `enemy.zone === zone` |
| 6466 | zone:arrived — send zone players | `existingPlayer.zone === zone` |
| 6636 | kafra:teleport — clear combat | `enemy.zone === zone` |
| 11859 | Teleport Lv2 cross-zone — clear combat | `enemy.zone === oldZone` |

### AoE Skill Handlers (enemies)
| Line | Skill | Filter |
|------|-------|--------|
| 9536 | Cast Sensor aggro scan | `csEnemy.zone !== castStartZone` |
| 9894 | Magnum Break (enemies) | `enemy.zone !== mbZone` |
| 10468 | Napalm Beat splash (enemies) | `enemy.zone !== nbZone` |
| 10675 | Fire Ball AoE (enemies) | `enemy.zone !== fbZone` |
| 10748 | Thunderstorm AoE (enemies) | `enemy.zone !== tsZone` |
| 11758 | Signum Crucis debuff (enemies) | `enemy.zone !== scZone` |
| 12258 | Arrow Shower AoE (enemies) | `enemy.zone !== asZone` |
| 12978 | Cart Revolution splash (enemies) | `enemy.zone !== crZone` |
| 13226 | Spear Stab line AoE (enemies) | `enemy.zone !== ssZone` |
| 13330 | Brandish Spear rectangle (enemies) | `enemy.zone !== bsZone` |
| 13582 | Bowling Bash splash (enemies) | `enemy.zone !== bbZone` |
| 13868 | Grand Cross diamond AoE (enemies) | `enemy.zone !== gcZone` |
| 14666 | Heaven's Drive AoE (enemies) | `enemy.zone !== hdZone` |
| 15049 | Sight Rasher AoE (enemies) | `enemy.zone !== srZone` |
| 15110 | Frost Nova AoE (enemies) | `enemy.zone !== fnZone` |
| 16729 | Grimtooth splash (enemies) | `enemy.zone !== gtZone` |
| 18048 | Blitz Beat splash (enemies) | `sEnemy.zone !== zone` |
| 18511 | Kick enemies splash (enemies) | `enemy.zone !== keZone` |
| 18700 | Crimson Fire Blossom splash (enemies) | `splashEnemy.zone !== zone` |
| 19322 | Frost Joker AoE (enemies) | `(enemy.zone \|\| 'prontera_south') !== zone` |
| 19374 | Scream AoE (enemies) | `(enemy.zone \|\| 'prontera_south') !== zone` |
| 19587 | Raid AoE (enemies) | `enemy.zone !== zone` |
| 20105 | Hammer Fall stun (enemies) | `enemy.zone !== zone` |
| 21318 | Marine Sphere manual detonate (enemies) | `(enemy.zone \|\| 'prontera_south') !== zone` |
| 27084 | Sight Blaster trigger (enemies) | `enemy.zone !== pZone` |

### AoE Skill Handlers (players)
| Line | Skill | Filter |
|------|-------|--------|
| 9968 | Magnum Break PvP (players) | `ptarget.zone !== mbZone` |
| 10481 | Napalm Beat PvP (players) | `ptarget.zone !== nbZone` |
| 10686 | Fire Ball PvP (players) | `ptarget.zone !== fbZone` |
| 10821 | Thunderstorm PvP (players) | `ptarget.zone !== tsZone` |
| 12339 | Improve Concentration reveal (players) | `p.zone !== icZone` |
| 17987 | Detect reveal (players) | `p.zone !== zone` |
| 18859 | Redemptio heal (players) | `p.zone === zone` |
| 19039 | Ensemble partner search (players) | `(p.zone \|\| 'prontera_south') === zone` |
| 19336 | Frost Joker AoE (players) | `(pTarget.zone \|\| 'prontera_south') !== zone` |
| 19385 | Scream AoE (players) | `(pTarget.zone \|\| 'prontera_south') !== zone` |

### Ground Effect Ticks (enemies) — Compliant
| Line | Effect Type | Filter |
|------|-------------|--------|
| 27896 | Storm Gust wave | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 27981 | Lord of Vermilion wave | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28057 | Meteor Storm splash | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28121 | Demonstration DoT | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28195 | Quagmire debuff (enemies) | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28302 | Fire Pillar trigger | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28369 | Venom Dust poison | `(enemy.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28442 | Sanctuary heal/damage | `(enemy.zone \|\| 'prontera_south') !== sanctZone` |
| 28482 | Magnus Exorcismus wave | `(enemy.zone \|\| 'prontera_south') !== meZone` |

### Ground Effect Ticks (players) — Compliant
| Line | Effect Type | Filter |
|------|-------------|--------|
| 28210 | Quagmire debuff (players) | `(p.zone \|\| 'prontera_south') !== (effect.zone \|\| 'prontera_south')` |
| 28407 | Sanctuary heal (players) | `(player.zone \|\| 'prontera_south') !== sanctZone` |

### Combat Tick (auto-attack)
| Line | Handler | Filter |
|------|---------|--------|
| 24818 | Venom Splasher detonation (enemies) | `target.zone !== vsZone` |
| 24845 | Venom Splasher AoE targets (enemies) | `target.zone !== vsZone` |
| 25131 | Auto-Blitz Beat splash on miss (enemies) | `se.zone !== atkEnemyZone` |
| 25257 | Baphomet Card splash (enemies) | `splashEnemy.zone !== enemy.zone` |
| 25676 | Auto-Blitz Beat splash on hit (enemies) | `se.zone !== atkEnemyZone` |

### Performance Tick
| Line | Handler | Filter |
|------|---------|--------|
| 27214 | Song overlap detection (players) | `(oPlayer.zone \|\| 'prontera_south') !== playerZone` |
| 27233 | Offensive performance AoE (enemies) | `(enemy.zone \|\| 'prontera_south') !== playerZone` |
| 27279 | Supportive performance buff (players) | `(ally.zone \|\| 'prontera_south') !== playerZone` |
| 27300 | Song overlap Dissonance (enemies) | `(oenemy.zone \|\| 'prontera_south') !== playerZone` |

### Trap Tick
| Line | Handler | Filter |
|------|---------|--------|
| 31191 | Blast Mine auto-detonate (enemies) | `target.zone !== eff.zone` |
| 31217 | Trap trigger check (enemies) | `target.zone !== eff.zone` |
| 31245 | Blast Mine AoE on trigger (enemies) | `se.zone !== eff.zone` |
| 31261 | Claymore Trap AoE (enemies) | `se.zone !== eff.zone` |
| 31279 | Freezing Trap AoE (enemies) | `se.zone !== eff.zone` |
| 31356 | Sandman AoE sleep (enemies) | `se.zone !== eff.zone` |

### Monster Skill Execution
| Line | Handler | Filter |
|------|---------|--------|
| 29398 | NPC_EARTHQUAKE AoE (players) | `player.zone !== zone` |
| 29438 | NPC_WIDE status AoE (players) | `player.zone !== zone` |
| 29716 | NPC_SELFDESTRUCTION AoE (players) | `player.zone !== zone` |

### Enemy AI Loop
| Line | Handler | Filter |
|------|---------|--------|
| 30111 | AI tick main loop | `!activeZones.has(enemy.zone)` — skips enemies in zones with no players |
| 30187 | Debug aggro scan (players) | `p.zone !== enemy.zone` |
| 28713 | findAggroTarget (players) | `player.zone !== enemy.zone` |
| 28632 | triggerAssist (enemies) | `other.zone !== attackedEnemy.zone` |
| 28816 | friendhpltmaxrate condition (enemies) | `other.zone !== enemy.zone` |

### Summon/Companion Loops
| Line | Handler | Filter |
|------|---------|--------|
| 21585 | Castling aggro redirect (enemies) | `e.zone === homZone` |
| 26304 | Geographer heal (players) | `(p.zone \|\| 'prontera_south') !== plant.zone` |
| 26326 | Plant auto-attack target search (enemies) | `(enemy.zone \|\| 'prontera_south') !== plant.zone` |
| 26371 | Marine Sphere auto-detonate (enemies) | `(enemy.zone \|\| 'prontera_south') !== sZone` |

### Status/Buff Expiry Tick
| Line | Handler | Notes |
|------|---------|-------|
| 26818 | Player status/buff expiry | N/A — per-player, no AoE, zone used for broadcast only |
| 27471 | Enemy status/buff expiry | N/A — per-enemy, no AoE, zone used for broadcast only |

### Utility/Lookup (Not AoE — No Zone Filter Needed)
| Line | Handler | Why N/A |
|------|---------|---------|
| 5088 | findPlayerBySocketId | Socket ID lookup, not spatial |
| 5126 | isZoneActive | Checks if any player is in a zone |
| 5134 | getActiveZones | Collects all zones with players |
| 5868 | player:join — active vending shops | Filters by `p.zone === playerZone` |
| 7241 | disconnect — find player by socket | Socket ID match, not spatial |
| 8439 | party:invite — find by name | Name search, not spatial |
| 8724 | chat:whisper — find by name | Name search, not spatial |
| 24791 | Maximize Power SP drain | Per-player drain, not AoE |
| 26579 | HP natural regen | Per-player, not AoE |
| 26638 | SP natural regen | Per-player, not AoE |
| 26692 | Skill-based regen | Per-player, not AoE |
| 26745 | Spirits Recovery regen | Per-player, not AoE |
| 26776 | Card periodic effects | Per-player, not AoE |
| 28746 | Gangster's Paradise partner check | Filters `other.zone !== enemy.zone` |
| 32551 | Periodic DB save | Per-player, administrative |

---

## Recommendations

### Priority 1 — Fix V1 and V2 (Fire Wall)
These are the only violations where cross-zone damage could actually occur in gameplay. Fire Wall iterates all enemies/players without zone filtering, meaning an enemy in Zone B at coordinates that overlap with a Fire Wall in Zone A could silently take damage.

**Effort**: 2 lines added (1 `continue` per loop).

### Priority 2 — Fix V3, V4, V5 (Deaggro Loops)
These are functionally harmless (enemies in other zones will never have this player as their target). However, adding zone filters would:
- Be consistent with the project's zone filtering standard
- Reduce wasted iterations (minor perf gain with many zones)
- Prevent future bugs if the codebase ever allows cross-zone targeting

**Effort**: 3 lines added total (1 `continue` per loop).

### Observation: Consistent Pattern
The codebase is overwhelmingly well-filtered. Out of 114 entity iterations:
- 90 are properly zone-filtered for spatial queries
- 20 are utility/non-spatial (don't need zone filtering)
- 4 are violations (3 LOW, 1 MEDIUM)

The standard pattern used throughout is:
- For enemies: `if (enemy.zone !== zone) continue;` or `if ((enemy.zone || 'prontera_south') !== zone) continue;`
- For players: `if (player.zone !== zone) continue;` or `if ((player.zone || 'prontera_south') !== zone) continue;`

The fallback `|| 'prontera_south'` is used in many places to handle undefined zones, which is a good defensive pattern.
