# Rogue Final 3 Skills — Implementation Plan

**Date:** 2026-03-17
**Goal:** 19/19 Rogue skills at 100%
**Status:** COMPLETE — ALL 19/19 IMPLEMENTED AND VERIFIED AGAINST RO CLASSIC PRE-RENEWAL

---

## Summary

| Skill | ID | Effort | Blocked By | Priority |
|-------|-----|--------|------------|----------|
| Remove Trap (Rogue) | 1708 | 15 min | Nothing — Hunter version exists, clone + modify | HIGH |
| Gangster's Paradise | 1715 | 30 min | Needs sit/stand system (~20 min) | MEDIUM |
| Scribble | 1717 | 10 min | Nothing — ground effect + gemstone patterns exist | LOW |

**Total: ~55 minutes. No architectural blockers.**

---

## Skill 1: Remove Trap — Rogue (ID 1708)

### RO Classic Behavior
- Targets a ground trap placed by ANY player (own traps or enemy Hunter traps in WoE)
- Removes the trap and returns 1 Trap item to the **Rogue's** inventory (not the trap owner)
- Cannot remove triggered traps
- Range: 2 cells (100 UE search radius, matching Hunter version)
- SP: 5
- Quest skill prerequisite: Double Strafe Rogue Lv5

### What Exists
Hunter Remove Trap (ID 905) is **fully implemented** at `index.js:15295-15321`:
- Finds trap within 100 UE of ground target
- **Only removes OWN traps** (`eff.casterId !== characterId` check)
- Removes ground effect + broadcasts `trap:removed`
- Returns 1 Trap item (ID 1065) to inventory

### Implementation
Clone the Hunter Remove Trap handler with one change — remove the ownership check:

```javascript
// --- REMOVE TRAP ROGUE (ID 1708) --- Remove ANY trap, return item to Rogue
if (skill.name === 'remove_trap_rogue') {
    if (!hasGroundPos) { socket.emit('skill:error', { message: 'No ground position' }); return; }
    const zone = player.zone || 'prontera_south';

    // Find ANY untriggered trap near ground position (not just own — key Rogue difference)
    let foundTrap = null, foundId = null;
    for (const [eid, eff] of activeGroundEffects.entries()) {
        if (eff.category !== 'trap' || eff.zone !== zone) continue;
        if (eff.isTriggered) continue;
        const tdx = groundX - eff.x, tdy = groundY - eff.y;
        if (Math.sqrt(tdx * tdx + tdy * tdy) <= 100) {
            foundTrap = eff; foundId = eid; break;
        }
    }
    if (!foundTrap) { socket.emit('skill:error', { message: 'No trap found at that position' }); return; }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    removeGroundEffect(foundId);
    broadcastToZone(zone, 'trap:removed', { trapId: foundId, removedById: characterId });
    broadcastToZone(zone, 'skill:ground_effect_removed', { effectId: foundId, reason: 'remove_trap' });

    // Return 1 Trap item (ID 1065) to Rogue's inventory
    const existingTrap = await pool.query(
        `SELECT id FROM character_inventory WHERE character_id = $1 AND item_id = 1065 AND is_equipped = false LIMIT 1`,
        [characterId]
    );
    if (existingTrap.rows.length > 0) {
        await pool.query(`UPDATE character_inventory SET quantity = quantity + 1 WHERE id = $1`, [existingTrap.rows[0].id]);
    } else {
        await addItemToInventory(characterId, 1065, 1);
    }
    await updatePlayerWeightCache(characterId, player);

    socket.emit('chat:receive', { type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
        message: `Removed ${foundTrap.type.replace(/_/g, ' ')} trap.`, timestamp: Date.now() });
    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost,
        remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth,
        mana: player.mana, maxMana: player.maxMana });
    return;
}
```

### Changes Required
1. Replace the deferred stub for `remove_trap_rogue` with the handler above
2. Location: `index.js` line ~16885 (the deferred skill block)

---

## Skill 2: Gangster's Paradise (ID 1715)

### RO Classic Behavior
- **Passive** — no activation required, always active when learned
- When **2+ Rogues/Stalkers** with this skill sit within **3x3 cells** (150 UE) of each other, nearby **non-Boss** monsters will NOT aggro them
- **Breaks when:** any player in the group stands up or moves
- **Boss monsters:** NOT affected (still aggro normally)
- The sitting players can still be attacked if a monster is ALREADY aggro'd — Gangster's Paradise only prevents NEW aggro

### What's Needed: Sit/Stand System

#### A. Player sit/stand state
Add to player object on join:
```javascript
player.isSitting = false;
```

#### B. Socket events
```javascript
socket.on('player:sit', () => {
    if (!player || player.isDead) return;
    player.isSitting = true;
    broadcastToZone(player.zone, 'player:sit', { characterId, characterName: player.characterName });
});

socket.on('player:stand', () => {
    if (!player || player.isDead) return;
    player.isSitting = false;
    broadcastToZone(player.zone, 'player:stand', { characterId, characterName: player.characterName });
});
```

#### C. Sitting blocks movement
In `player:position` handler, add before existing checks:
```javascript
if (player.isSitting) {
    player.isSitting = false; // Auto-stand on movement
    broadcastToZone(playerZone, 'player:stand', { characterId, characterName: player.characterName });
}
```

#### D. Gangster's Paradise aggro check
In the enemy AI IDLE state aggro scan (where `findAggroTarget()` is called), add a Gangster's Paradise filter. The function `findAggroTarget()` already iterates nearby players. Add a check:

```javascript
function isProtectedByGangstersParadise(player, zone) {
    if (!player.isSitting) return false;
    const gpLv = (player.learnedSkills || {})[1715] || 0;
    if (gpLv <= 0) return false;

    // Count other sitting Rogues/Stalkers with GP within 150 UE
    let sittingRogueCount = 0;
    for (const [, other] of connectedPlayers.entries()) {
        if (other === player || other.isDead || other.zone !== zone || !other.isSitting) continue;
        const otherGP = (other.learnedSkills || {})[1715] || 0;
        if (otherGP <= 0) continue;
        const dist = calculateDistance(
            { x: player.lastX || 0, y: player.lastY || 0 },
            { x: other.lastX || 0, y: other.lastY || 0 }
        );
        if (dist <= 150) { sittingRogueCount++; break; } // Only need 1 other = 2 total
    }
    return sittingRogueCount > 0; // 2+ Rogues sitting together
}
```

In `findAggroTarget()`, after checking if a player is valid (alive, not hidden, not playing dead):
```javascript
// Gangster's Paradise: 2+ sitting Rogues with this skill = monsters ignore them
if (isProtectedByGangstersParadise(player, enemy.zone)) continue;
```

**Boss exception:** Boss monsters (`enemy.modeFlags?.boss` or `statusImmune`) should skip the GP check entirely.

#### E. Remove the deferred stub
Delete `gangsters_paradise` from the deferred skill block since it's passive (no handler needed — it's an AI check).

### Changes Required
1. Add `player.isSitting = false` to player:join initialization
2. Add `player:sit` and `player:stand` socket event handlers
3. Add auto-stand on movement in `player:position`
4. Add `isProtectedByGangstersParadise()` helper function
5. Add GP check in `findAggroTarget()`
6. Remove `gangsters_paradise` from deferred stub

---

## Skill 3: Scribble (ID 1717)

### RO Classic Behavior
- Quest skill — learned from NPC quest
- Ground target, range: self (0 cells)
- SP: 15
- Consumes: 1 Red Gemstone (item ID 716)
- Creates text graffiti on the ground for 180 seconds
- Purely cosmetic — no gameplay effect
- Text limited to ~20 characters

### Implementation
```javascript
if (skill.name === 'scribble') {
    if (!hasGroundPos) { socket.emit('skill:error', { message: 'No ground position' }); return; }
    const zone = player.zone || 'prontera_south';

    // Red Gemstone check + consumption
    const gemCheck = await pool.query(
        `SELECT id, quantity FROM character_inventory WHERE character_id = $1 AND item_id = 716 AND is_equipped = false AND quantity >= 1 LIMIT 1`,
        [characterId]
    );
    if (gemCheck.rows.length === 0) { socket.emit('skill:error', { message: 'Requires a Red Gemstone' }); return; }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Consume 1 Red Gemstone
    await pool.query(`UPDATE character_inventory SET quantity = quantity - 1 WHERE id = $1`, [gemCheck.rows[0].id]);
    await pool.query(`DELETE FROM character_inventory WHERE id = $1 AND quantity <= 0`, [gemCheck.rows[0].id]);
    await updatePlayerWeightCache(characterId, player);

    // Create cosmetic ground effect (180 seconds)
    const scribbleText = (data.text || 'Rogue was here').substring(0, 20);
    const scribbleDuration = 180000;
    const scribbleId = createGroundEffect({
        type: 'scribble', category: 'cosmetic',
        skillId, skillLevel: 1, casterId: characterId, casterName: player.characterName, zone,
        x: player.lastX || 0, y: player.lastY || 0, z: player.lastZ || 0,
        radius: 0, element: 'neutral', duration: scribbleDuration,
        text: scribbleText,
    });

    broadcastToZone(zone, 'skill:ground_effect_created', {
        effectId: scribbleId, type: 'scribble',
        casterId: characterId, casterName: player.characterName,
        x: player.lastX || 0, y: player.lastY || 0, z: player.lastZ || 0,
        duration: scribbleDuration, text: scribbleText,
    });

    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost,
        remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth,
        mana: player.mana, maxMana: player.maxMana });
    return;
}
```

### Client Side (Deferred)
The client would need to render text on the ground when receiving `skill:ground_effect_created` with `type: 'scribble'`. This is a cosmetic client feature and can be done later — the server handler is complete.

### Changes Required
1. Replace the deferred stub for `scribble` with the handler above
2. The `data.text` parameter comes from the client skill:use payload — client needs to send text

---

## Implementation Order

```
Step 1 (15 min): Remove Trap Rogue — clone Hunter handler, remove ownership check
Step 2 (30 min): Gangster's Paradise — sit/stand system + AI aggro filter
Step 3 (10 min): Scribble — ground effect + Red Gemstone consumption

Total: ~55 minutes
```

### After Implementation

| ID | Skill | Status |
|----|-------|--------|
| 1700-1718 | All 19 Rogue skills | **19/19 at 100%** |

**Caveats:**
- Scribble ground text rendering needs client-side work (server handler complete)
- Gangster's Paradise needs client sit animation (server logic complete)
- Remove Trap needs client ground-target UI if not already available for Rogue
