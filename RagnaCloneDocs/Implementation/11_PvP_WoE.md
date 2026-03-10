# 11 -- PvP & War of Emperium: UE5 C++ Implementation Guide

> Complete UE5 C++ and Node.js implementation guide for PvP arenas, duel system, and War of Emperium (WoE 1 & WoE 2).
> Server-authoritative architecture: all damage, ownership, scoring, and state transitions happen server-side.
> Reference: `RagnaCloneDocs/08_PvP_Guild_WoE.md` for RO Classic game design specs.

---

## Table of Contents

1. [PvP System (Server)](#1-pvp-system-server)
2. [PvP Arenas](#2-pvp-arenas)
3. [WoE Server Architecture](#3-woe-server-architecture)
4. [WoE Client](#4-woe-client)
5. [Castle Levels](#5-castle-levels)
6. [Scalability](#6-scalability)
7. [DB Schema](#7-db-schema)

---

## 1. PvP System (Server)

### 1.1 PvP Map Flags

The existing zone registry (`server/src/ro_zone_data.js`) already supports a `pvp` flag on every zone. PvP arenas use this flag plus additional arena-specific flags.

```javascript
// server/src/ro_zone_data.js -- Add new PvP zone flags to the flags object
// Extended flags structure for PvP and WoE zones:
flags: {
    noteleport: boolean,   // Fly Wing blocked
    noreturn: boolean,     // Butterfly Wing blocked
    nosave: boolean,       // Cannot save here
    pvp: boolean,          // PvP damage enabled between all players
    pvpNightmare: boolean, // Nightmare mode (EXP loss + item drop on death)
    town: boolean,         // Town zone
    indoor: boolean,       // Indoor zone
    woe: boolean,          // War of Emperium castle zone
    woeSE: boolean,        // WoE Second Edition castle zone
    noDuel: boolean,       // Duels prohibited (inside WoE/BG maps)
    noKnockback: boolean,  // Knockback effects disabled (WoE)
    guildZone: boolean,    // Guild dungeon (owning guild members only)
}
```

**Example PvP arena zone entry:**

```javascript
// server/src/ro_zone_data.js -- PvP Arena zone definition
pvp_arena_prontera: {
    name: 'pvp_arena_prontera',
    displayName: 'Prontera PvP Arena',
    type: 'pvp_arena',
    flags: {
        noteleport: true,
        noreturn: false,     // Butterfly Wing allowed (exit method)
        nosave: true,
        pvp: true,
        pvpNightmare: false, // Yoyo mode (safe PvP)
        town: false,
        indoor: true,
        woe: false,
        noDuel: true,        // Already in PvP mode
        noKnockback: false,  // Knockback allowed in PvP (unlike WoE)
    },
    defaultSpawn: { x: 0, y: 0, z: 300 },
    levelName: 'L_PvPArenaProntera',
    warps: [],               // No warp portals inside PvP arenas
    kafraNpcs: [],
    enemySpawns: [],         // No monsters in PvP arenas
    pvpConfig: {
        maxPlayers: 40,
        startingPoints: 5,
        killPoints: 1,
        deathPenalty: 5,
        spawnInvulnerabilityMs: 5000,
        minBaseLevel: 31,
    }
}
```

### 1.2 PvP Damage Calculation

PvP damage reuses the existing `roPhysicalDamage()` and `roMagicalDamage()` formulas from `ro_damage_formulas.js`. The key difference is checking the zone `pvp` flag before allowing player-vs-player damage.

```javascript
// server/src/index.js -- PvP damage gate (modify existing combat tick)
// Replace the current PVP_ENABLED global toggle with zone-based PvP checking

// OLD: const PVP_ENABLED = false;
// NEW: Remove global toggle, check per-zone instead

function isPvPAllowed(attackerZone, targetCharId, attackerCharId) {
    const zone = getZone(attackerZone);
    if (!zone) return false;

    // PvP arenas: all players can attack each other
    if (zone.flags.pvp) return true;

    // WoE castle zones: guild-based PvP (handled separately in WoE logic)
    if (zone.flags.woe || zone.flags.woeSE) return true;

    // Duel system: check if both players are in an active duel
    const duel = getActiveDuel(attackerCharId);
    if (duel && duel.participants.has(targetCharId)) return true;

    // Guild enemy: declared enemies can fight anywhere
    const attacker = connectedPlayers.get(attackerCharId);
    const target = connectedPlayers.get(targetCharId);
    if (attacker?.guildId && target?.guildId) {
        if (isGuildEnemy(attacker.guildId, target.guildId)) return true;
    }

    return false;
}
```

**PvP damage reduction rules (Yoyo mode -- NO additional reductions):**

```javascript
// server/src/index.js -- PvP damage modifiers
// In Yoyo PvP mode, no special reduction is applied.
// WoE has its own reduction (see Section 3).
// This function wraps existing calculatePhysicalDamage for PvP context.

function calculatePvPDamage(attacker, target, options = {}) {
    const zone = getZone(attacker.zone);

    // Build attacker/target info same as existing processPlayerAutoAttack
    const attackerStats = getEffectiveStats(attacker);
    const targetStats = getEffectiveStats(target);
    const targetHardDef = targetStats.equipDEF || 0;

    const result = calculatePhysicalDamage(
        attackerStats, targetStats, targetHardDef,
        { element: { type: target.element || 'neutral', level: 1 },
          size: 'medium', race: 'demihuman',
          buffMods: { defMultiplier: targetStats.buffDefMultiplier || 1.0 } },
        { weaponType: attacker.weaponType || 'bare_hand',
          weaponElement: attacker.weaponElement || 'neutral',
          weaponLevel: attacker.weaponLevel || 1,
          buffMods: { atkMultiplier: attackerStats.buffAtkMultiplier || 1.0 } },
        options
    );

    // WoE damage reduction (applied in WoE zones only)
    if (zone && (zone.flags.woe || zone.flags.woeSE)) {
        result.damage = applyWoEDamageReduction(result.damage, options);
    }

    return result;
}
```

### 1.3 Kill/Death Tracking

```javascript
// server/src/index.js -- PvP kill/death tracking (in-memory + DB persistence)

// In-memory PvP session state per player (reset when leaving PvP map)
const pvpSessionState = new Map();
// Key: characterId, Value: { points, kills, deaths, streak, zone }

function initPvPSession(characterId, zone) {
    const config = getZone(zone)?.pvpConfig;
    if (!config) return;

    pvpSessionState.set(characterId, {
        points: config.startingPoints,    // Default: 5
        kills: 0,
        deaths: 0,
        streak: 0,
        zone: zone,
        spawnInvulnerableUntil: Date.now() + config.spawnInvulnerabilityMs,
    });
}

function onPvPKill(killerId, victimId, zone) {
    const config = getZone(zone)?.pvpConfig;
    if (!config) return;

    const killerSession = pvpSessionState.get(killerId);
    const victimSession = pvpSessionState.get(victimId);

    if (killerSession) {
        killerSession.kills++;
        killerSession.points += config.killPoints;  // +1
        killerSession.streak++;
    }

    if (victimSession) {
        victimSession.deaths++;
        victimSession.points -= config.deathPenalty; // -5
        victimSession.streak = 0;

        // Respawn with invulnerability
        victimSession.spawnInvulnerableUntil =
            Date.now() + config.spawnInvulnerabilityMs;
    }

    // Broadcast kill event to everyone in the PvP zone
    const killer = connectedPlayers.get(killerId);
    const victim = connectedPlayers.get(victimId);
    broadcastToZone(zone, 'pvp:kill', {
        killerId, killerName: killer?.characterName,
        victimId, victimName: victim?.characterName,
        killerPoints: killerSession?.points || 0,
        victimPoints: victimSession?.points || 0,
    });

    // Check if victim should be kicked (points <= 0)
    if (victimSession && victimSession.points <= 0) {
        kickFromPvP(victimId, 'Your PvP points reached zero.');
    }

    // Update rankings for this zone
    broadcastPvPRankings(zone);

    // Persist lifetime stats to DB (async, non-blocking)
    persistPvPStats(killerId, { kills: 1, deaths: 0 });
    persistPvPStats(victimId, { kills: 0, deaths: 1 });
}

// Persist cumulative PvP stats to PostgreSQL
async function persistPvPStats(characterId, delta) {
    try {
        await pool.query(`
            INSERT INTO pvp_stats (character_id, kills, deaths)
            VALUES ($1, $2, $3)
            ON CONFLICT (character_id)
            DO UPDATE SET
                kills = pvp_stats.kills + $2,
                deaths = pvp_stats.deaths + $3,
                highest_streak = GREATEST(pvp_stats.highest_streak, $4),
                last_pvp_at = NOW()
        `, [characterId, delta.kills, delta.deaths,
            pvpSessionState.get(characterId)?.streak || 0]);
    } catch (err) {
        logger.error(`[PVP] Failed to persist stats for ${characterId}: ${err.message}`);
    }
}
```

### 1.4 PvP Rankings

```javascript
// server/src/index.js -- PvP ranking system (per-zone, per-session)

function getPvPRankings(zone) {
    const rankings = [];
    for (const [charId, session] of pvpSessionState.entries()) {
        if (session.zone !== zone) continue;
        const player = connectedPlayers.get(charId);
        if (!player) continue;

        rankings.push({
            charId,
            name: player.characterName,
            points: session.points,
            kills: session.kills,
            deaths: session.deaths,
            streak: session.streak,
        });
    }

    // Sort by points descending, then kills descending
    rankings.sort((a, b) => b.points - a.points || b.kills - a.kills);

    // Assign rank numbers
    rankings.forEach((entry, idx) => { entry.rank = idx + 1; });
    return rankings;
}

function broadcastPvPRankings(zone) {
    const rankings = getPvPRankings(zone);
    broadcastToZone(zone, 'pvp:rankings_update', { rankings });
}

function kickFromPvP(characterId, reason) {
    const player = connectedPlayers.get(characterId);
    if (!player) return;

    const socket = getSocketForCharacter(characterId);
    if (socket) {
        socket.emit('pvp:kicked', { reason });
    }

    pvpSessionState.delete(characterId);

    // Warp player to their save point
    const saveZone = player.saveZone || 'prontera';
    const savePoint = getZone(saveZone)?.defaultSpawn || { x: 0, y: 0, z: 300 };
    handleZoneChange(characterId, saveZone, savePoint.x, savePoint.y, savePoint.z);
}
```

### 1.5 Duel System

```javascript
// server/src/index.js -- Duel system (1v1 or up to 3 players, anywhere)

const activeDuels = new Map();    // duelId -> { participants: Set<charId>, createdAt }
const duelRequests = new Map();   // requestId -> { challengerId, targetId, expiresAt }
const playerDuelMap = new Map();  // charId -> duelId (reverse lookup)
let nextDuelId = 1;
let nextDuelRequestId = 1;

function getActiveDuel(charId) {
    const duelId = playerDuelMap.get(charId);
    return duelId ? activeDuels.get(duelId) : null;
}

// Socket event handlers
socket.on('pvp:duel_request', ({ targetCharId }) => {
    const challengerId = socket.characterId;
    if (!challengerId) return;

    // Validate: not already in a duel
    if (playerDuelMap.has(challengerId)) {
        socket.emit('pvp:error', { message: 'You are already in a duel.' });
        return;
    }
    if (playerDuelMap.has(targetCharId)) {
        socket.emit('pvp:error', { message: 'That player is already in a duel.' });
        return;
    }

    // Validate: same zone
    const challenger = connectedPlayers.get(challengerId);
    const target = connectedPlayers.get(targetCharId);
    if (!challenger || !target) return;
    if (challenger.zone !== target.zone) {
        socket.emit('pvp:error', { message: 'Target must be on the same map.' });
        return;
    }

    // Validate: not in a WoE/PvP arena zone
    const zone = getZone(challenger.zone);
    if (zone?.flags.noDuel || zone?.flags.woe || zone?.flags.pvp) {
        socket.emit('pvp:error', { message: 'Duels are not allowed in this zone.' });
        return;
    }

    const requestId = nextDuelRequestId++;
    duelRequests.set(requestId, {
        challengerId,
        targetId: targetCharId,
        expiresAt: Date.now() + 30000, // 30s to accept
    });

    const targetSocket = getSocketForCharacter(targetCharId);
    if (targetSocket) {
        targetSocket.emit('pvp:duel_request_received', {
            requestId,
            challengerName: challenger.characterName,
        });
    }
});

socket.on('pvp:duel_accept', ({ requestId }) => {
    const request = duelRequests.get(requestId);
    if (!request) return;
    if (Date.now() > request.expiresAt) {
        duelRequests.delete(requestId);
        socket.emit('pvp:error', { message: 'Duel request has expired.' });
        return;
    }
    if (socket.characterId !== request.targetId) return;

    duelRequests.delete(requestId);

    // Create the duel
    const duelId = nextDuelId++;
    const participants = new Set([request.challengerId, request.targetId]);
    activeDuels.set(duelId, { participants, createdAt: Date.now() });

    for (const charId of participants) {
        playerDuelMap.set(charId, duelId);
        const s = getSocketForCharacter(charId);
        const opponent = connectedPlayers.get(
            charId === request.challengerId ? request.targetId : request.challengerId
        );
        if (s) {
            s.emit('pvp:duel_started', {
                opponentId: opponent?.characterId,
                opponentName: opponent?.characterName,
            });
        }
    }
});

socket.on('pvp:duel_reject', ({ requestId }) => {
    const request = duelRequests.get(requestId);
    if (!request) return;
    duelRequests.delete(requestId);

    const challengerSocket = getSocketForCharacter(request.challengerId);
    if (challengerSocket) {
        challengerSocket.emit('pvp:duel_ended', {
            winnerId: null,
            reason: 'Challenge was declined.',
        });
    }
});

socket.on('pvp:duel_leave', () => {
    endDuelForPlayer(socket.characterId, 'left');
});

function endDuelForPlayer(charId, reason) {
    const duelId = playerDuelMap.get(charId);
    if (!duelId) return;

    const duel = activeDuels.get(duelId);
    if (!duel) return;

    // Determine winner (the one who did NOT leave/die)
    let winnerId = null;
    for (const pid of duel.participants) {
        if (pid !== charId) winnerId = pid;
    }

    // Clean up all participants
    for (const pid of duel.participants) {
        playerDuelMap.delete(pid);
        const s = getSocketForCharacter(pid);
        if (s) {
            s.emit('pvp:duel_ended', { winnerId, reason });
        }
    }

    activeDuels.delete(duelId);
}
```

### 1.6 PvP Spawn Invulnerability

```javascript
// server/src/index.js -- Invulnerability check (integrate into damage pipeline)

function isInvulnerable(characterId) {
    const session = pvpSessionState.get(characterId);
    if (!session) return false;
    return Date.now() < session.spawnInvulnerableUntil;
}

function cancelInvulnerability(characterId) {
    const session = pvpSessionState.get(characterId);
    if (session) {
        session.spawnInvulnerableUntil = 0;
    }
}

// Call cancelInvulnerability(charId) when:
// 1. Player moves (on player:position event)
// 2. Player attacks (on combat:attack event)
// 3. Player uses a skill (on skill:use event)
```

### 1.7 Socket Events Summary (PvP)

```
// Client -> Server
pvp:enter_room        { roomId }                 // Enter PvP arena (pays 500z entry fee)
pvp:leave             {}                         // Voluntary exit from PvP arena
pvp:duel_request      { targetCharId }           // Challenge another player
pvp:duel_accept       { requestId }              // Accept duel challenge
pvp:duel_reject       { requestId }              // Decline duel challenge
pvp:duel_leave        {}                         // Leave active duel

// Server -> Client
pvp:entered           { roomId, points, rankings[] }
pvp:kill              { killerId, killerName, victimId, victimName,
                        killerPoints, victimPoints }
pvp:death             { killerId, newPoints }
pvp:kicked            { reason }                  // Points <= 0
pvp:rankings_update   { rankings[] }              // { charId, name, points, kills,
                                                  //   deaths, streak, rank }
pvp:duel_request_received { requestId, challengerName }
pvp:duel_started      { opponentId, opponentName }
pvp:duel_ended        { winnerId, reason }
pvp:error             { message }
pvp:invulnerable_end  {}                          // 5s invulnerability expired
```

---

## 2. PvP Arenas

### 2.1 Entry NPCs

PvP arenas are accessed through NPC actors placed in town maps. These NPCs follow the existing `KafraNPC` pattern (click-to-interact via `BPI_Interactable`).

**Server-side entry handler:**

```javascript
// server/src/index.js -- PvP room entry handler

socket.on('pvp:enter_room', ({ roomId }) => {
    const characterId = socket.characterId;
    const player = connectedPlayers.get(characterId);
    if (!player) return;

    // Validate: level requirement
    if ((player.stats?.level || 1) < 31) {
        socket.emit('pvp:error', { message: 'You must be Base Level 31 or higher.' });
        return;
    }

    // Validate: not in WoE or another PvP room
    const currentZone = getZone(player.zone);
    if (currentZone?.flags.pvp || currentZone?.flags.woe) {
        socket.emit('pvp:error', { message: 'You are already in a PvP zone.' });
        return;
    }

    // Validate: entry fee (500 Zeny)
    const ENTRY_FEE = 500;
    if ((player.stats?.zeny || 0) < ENTRY_FEE) {
        socket.emit('pvp:error', { message: `Entry fee: ${ENTRY_FEE} Zeny. You don't have enough.` });
        return;
    }

    // Validate: room exists and is not full
    const targetZone = getZone(roomId);
    if (!targetZone || !targetZone.flags.pvp) {
        socket.emit('pvp:error', { message: 'Invalid PvP room.' });
        return;
    }

    const maxPlayers = targetZone.pvpConfig?.maxPlayers || 40;
    const currentCount = countPlayersInZone(roomId);
    if (currentCount >= maxPlayers) {
        socket.emit('pvp:error', { message: 'PvP room is full.' });
        return;
    }

    // Deduct entry fee
    player.stats.zeny -= ENTRY_FEE;
    socket.emit('stats:zeny_update', { zeny: player.stats.zeny });

    // Initialize PvP session
    initPvPSession(characterId, roomId);

    // Zone transition to PvP arena
    const spawn = targetZone.defaultSpawn;
    handleZoneChange(characterId, roomId, spawn.x, spawn.y, spawn.z);

    // Notify client
    socket.emit('pvp:entered', {
        roomId,
        points: 5,
        rankings: getPvPRankings(roomId),
    });
});
```

### 2.2 UE5 Arena NPC Actor (C++ Header)

```cpp
// client/SabriMMO/Source/SabriMMO/PvPArenaNPC.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PvPArenaNPC.generated.h"

/**
 * Clickable NPC that opens the PvP arena entry dialog.
 * Follows the same BPI_Interactable pattern as KafraNPC.
 * Place in town levels near the Inn or main square.
 */
UCLASS()
class SABRIMMO_API APvPArenaNPC : public AActor
{
    GENERATED_BODY()

public:
    APvPArenaNPC();

    /** The PvP arena zone name this NPC sends players to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
    FString ArenaZoneName = TEXT("pvp_arena_prontera");

    /** NPC display name shown in the interaction prompt */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
    FString NPCName = TEXT("PvP Arena Guide");

    /** Minimum base level required (displayed in tooltip) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
    int32 MinimumLevel = 31;

    /** Entry fee in Zeny */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
    int32 EntryFee = 500;

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class USphereComponent> InteractionSphere;

    virtual void BeginPlay() override;

private:
    /** Called when player clicks this NPC (via BPI_Interactable) */
    void OnInteract(AActor* Interactor);
};
```

### 2.3 Arena Map Setup

PvP arena levels follow the standard zone creation process (see `RagnaCloneDocs/Implementation/08_Zone_World_System.md`). Key differences:

1. **Always duplicate** an existing level (e.g., `L_PrtSouth`) to preserve the Level Blueprint.
2. **Remove** all warp portals and Kafra NPCs from the duplicated level.
3. **Add spawn points** -- place multiple `PlayerStart` actors or define spawn positions in the zone config.
4. **Level naming:** `L_PvPArenaProntera`, `L_PvPArenaIzlude`, etc.
5. **No enemies** -- `enemySpawns` array must be empty.
6. **Enclosed arena** -- walled off to prevent players from walking out of bounds.

**Spawn point randomization (server-side):**

```javascript
// server/src/ro_zone_data.js -- PvP arena spawn points
pvp_arena_prontera: {
    // ... (flags, config as above)
    pvpConfig: {
        maxPlayers: 40,
        startingPoints: 5,
        killPoints: 1,
        deathPenalty: 5,
        spawnInvulnerabilityMs: 5000,
        minBaseLevel: 31,
        // Multiple spawn points -- player spawns at a random one
        spawnPoints: [
            { x: -500, y: -500, z: 300 },
            { x: 500, y: -500, z: 300 },
            { x: -500, y: 500, z: 300 },
            { x: 500, y: 500, z: 300 },
            { x: 0, y: 0, z: 300 },
            { x: -800, y: 0, z: 300 },
            { x: 800, y: 0, z: 300 },
            { x: 0, y: -800, z: 300 },
        ],
    },
}

// Helper: pick a random spawn point for a PvP zone
function getRandomPvPSpawn(zoneName) {
    const zone = getZone(zoneName);
    const points = zone?.pvpConfig?.spawnPoints;
    if (!points || points.length === 0) return zone?.defaultSpawn || { x: 0, y: 0, z: 300 };
    return points[Math.floor(Math.random() * points.length)];
}
```

### 2.4 PvP Score Tracking and Rewards

```javascript
// server/src/index.js -- PvP leave handler (cleanup + reward)

socket.on('pvp:leave', () => {
    const characterId = socket.characterId;
    const session = pvpSessionState.get(characterId);
    if (!session) return;

    // Clean up session
    pvpSessionState.delete(characterId);

    // Warp to save point
    const player = connectedPlayers.get(characterId);
    const saveZone = player?.saveZone || 'prontera';
    const spawn = getZone(saveZone)?.defaultSpawn || { x: 0, y: 0, z: 300 };
    handleZoneChange(characterId, saveZone, spawn.x, spawn.y, spawn.z);

    // Update rankings for remaining players
    broadcastPvPRankings(session.zone);
});
```

### 2.5 PvP Top-10 Glow Effect

The top 10 ranked players on a PvP map receive a visual glow effect. This is driven by the rankings broadcast and rendered client-side.

```
// Rankings payload includes rank number:
pvp:rankings_update {
    rankings: [
        { charId: 42, name: "Swordsman", points: 15, rank: 1 },
        { charId: 99, name: "Mage",      points: 12, rank: 2 },
        ...
    ]
}
```

The client `PvPSubsystem` checks if a nearby player's `charId` is in the top 10 and attaches a Niagara glow effect. Brightness increases linearly as rank approaches 1.

---

## 3. WoE Server Architecture

### 3.1 WoE Scheduler

The WoE scheduler runs as a `setInterval` check on the server, similar to the combat tick and enemy AI loops. It uses a configurable schedule and manages the Pre/Active/Post state machine.

```javascript
// server/src/woe_scheduler.js
// Import into index.js: const { initWoEScheduler } = require('./woe_scheduler');

const WOE_STATE = {
    INACTIVE: 'INACTIVE',
    PRE_WOE: 'PRE_WOE',       // 5 minutes before start (announcement phase)
    ACTIVE: 'ACTIVE',          // WoE in progress
    POST_WOE: 'POST_WOE',     // 5 minutes after end (cleanup phase)
};

// Configurable schedule -- typically 2 sessions per week
const WOE_SCHEDULE = [
    { day: 3, startHour: 19, startMin: 0, durationMin: 120 },  // Wed 7:00-9:00 PM
    { day: 6, startHour: 13, startMin: 0, durationMin: 120 },  // Sat 1:00-3:00 PM
];

const PRE_WOE_MINUTES = 5;     // Announcement window before WoE starts
const POST_WOE_MINUTES = 5;    // Cleanup window after WoE ends

let woeState = WOE_STATE.INACTIVE;
let woeStartTime = null;
let woeEndTime = null;
let woeScheduleCheckInterval = null;

// In-memory castle state during WoE (loaded from DB at PRE_WOE, persisted at POST_WOE)
const activeCastles = new Map();

function initWoEScheduler(io, pool, broadcastToZone, connectedPlayers) {
    // Store references for broadcasting
    const ctx = { io, pool, broadcastToZone, connectedPlayers };

    // Check every 15 seconds
    woeScheduleCheckInterval = setInterval(() => {
        checkWoESchedule(ctx);
    }, 15000);

    // Also check immediately on startup (server may restart mid-WoE)
    checkWoESchedule(ctx);

    return {
        getWoEState: () => woeState,
        isWoEActive: () => woeState === WOE_STATE.ACTIVE,
        getActiveCastles: () => activeCastles,
        getWoESchedule: () => WOE_SCHEDULE,
    };
}

function checkWoESchedule(ctx) {
    const now = new Date();
    const day = now.getDay();       // 0=Sun, 1=Mon, ..., 6=Sat
    const nowMin = now.getHours() * 60 + now.getMinutes();

    for (const slot of WOE_SCHEDULE) {
        if (day !== slot.day) continue;

        const startMin = slot.startHour * 60 + slot.startMin;
        const endMin = startMin + slot.durationMin;
        const preStartMin = startMin - PRE_WOE_MINUTES;
        const postEndMin = endMin + POST_WOE_MINUTES;

        // PRE_WOE: 5 minutes before start
        if (nowMin >= preStartMin && nowMin < startMin
            && woeState === WOE_STATE.INACTIVE) {
            transitionToPreWoE(ctx, slot);
        }
        // ACTIVE: during WoE window
        else if (nowMin >= startMin && nowMin < endMin
            && woeState !== WOE_STATE.ACTIVE) {
            transitionToActive(ctx, slot);
        }
        // POST_WOE: WoE time expired
        else if (nowMin >= endMin && nowMin < postEndMin
            && woeState === WOE_STATE.ACTIVE) {
            transitionToPostWoE(ctx);
        }
        // INACTIVE: cleanup window expired
        else if (nowMin >= postEndMin
            && woeState === WOE_STATE.POST_WOE) {
            transitionToInactive(ctx);
        }
    }

    // If no schedule matches today and we are somehow still active, end it
    const todayHasWoE = WOE_SCHEDULE.some(s => s.day === day);
    if (!todayHasWoE && woeState !== WOE_STATE.INACTIVE) {
        transitionToInactive(ctx);
    }
}

// ===== State Transitions =====

async function transitionToPreWoE(ctx, slot) {
    woeState = WOE_STATE.PRE_WOE;

    const startTime = new Date();
    startTime.setHours(slot.startHour, slot.startMin, 0, 0);
    woeStartTime = startTime.getTime();
    woeEndTime = woeStartTime + (slot.durationMin * 60 * 1000);

    // Load castle data from DB into memory
    await loadCastlesFromDB(ctx);

    // Server-wide announcement
    ctx.io.emit('woe:announcement', {
        message: `War of Emperium begins in ${PRE_WOE_MINUTES} minutes!`,
        woeStartTime: woeStartTime,
        woeEndTime: woeEndTime,
    });

    logger.info(`[WOE] Transitioning to PRE_WOE. Start: ${new Date(woeStartTime).toISOString()}`);
}

function transitionToActive(ctx, slot) {
    woeState = WOE_STATE.ACTIVE;

    if (!woeStartTime) {
        const startTime = new Date();
        startTime.setHours(slot.startHour, slot.startMin, 0, 0);
        woeStartTime = startTime.getTime();
        woeEndTime = woeStartTime + (slot.durationMin * 60 * 1000);
    }

    // Server-wide broadcast
    ctx.io.emit('woe:start', {
        message: 'The War of Emperium has begun!',
        endTime: woeEndTime,
    });

    // Warp non-owners out of all castle zones
    warpNonOwnersFromAllCastles(ctx);

    // Spawn guardians for all owned castles
    for (const [castleId, castle] of activeCastles.entries()) {
        if (castle.ownerGuildId) {
            spawnCastleGuardians(ctx, castleId);
        }
    }

    // Reset Emperium HP for all castles
    for (const [castleId, castle] of activeCastles.entries()) {
        castle.emperiumHp = castle.emperiumMaxHp;
    }

    logger.info(`[WOE] WoE is now ACTIVE. ${activeCastles.size} castles loaded.`);
}

async function transitionToPostWoE(ctx) {
    woeState = WOE_STATE.POST_WOE;

    // Broadcast end with results
    const results = [];
    for (const [castleId, castle] of activeCastles.entries()) {
        results.push({
            castleId,
            castleName: castle.name,
            guildId: castle.ownerGuildId,
            guildName: castle.ownerGuildName || 'Unoccupied',
        });
    }

    ctx.io.emit('woe:end', {
        message: 'The War of Emperium has ended.',
        results,
    });

    // Warp all non-owners out of castle zones
    warpNonOwnersFromAllCastles(ctx);

    // Despawn all guardians
    for (const [castleId] of activeCastles.entries()) {
        despawnCastleGuardians(ctx, castleId);
    }

    // Persist castle state to DB
    await saveCastlesToDB(ctx);

    // Generate treasure boxes for all owned castles
    await generateTreasureBoxes(ctx);

    logger.info(`[WOE] WoE ended. Results: ${JSON.stringify(results)}`);
}

function transitionToInactive(ctx) {
    woeState = WOE_STATE.INACTIVE;
    woeStartTime = null;
    woeEndTime = null;
    activeCastles.clear();
    logger.info('[WOE] Transitioning to INACTIVE. Castle data cleared from memory.');
}

module.exports = { initWoEScheduler, WOE_STATE, WOE_SCHEDULE };
```

### 3.2 Castle Data Structure (In-Memory)

```javascript
// server/src/woe_scheduler.js -- Castle data loaded from DB during PRE_WOE

async function loadCastlesFromDB(ctx) {
    activeCastles.clear();

    const { rows } = await ctx.pool.query(`
        SELECT
            wc.id, wc.castle_name, wc.realm, wc.map_name,
            wc.owner_guild_id, wc.economy_level, wc.defense_level,
            wc.emperium_hp,
            g.name AS guild_name
        FROM woe_castles wc
        LEFT JOIN guilds g ON g.id = wc.owner_guild_id
        ORDER BY wc.id
    `);

    for (const row of rows) {
        const baseEmperiumHp = 68430;
        const defenseBonus = (row.defense_level || 0) * 1000;
        const maxHp = baseEmperiumHp + defenseBonus;

        activeCastles.set(row.id, {
            id: row.id,
            name: row.castle_name,
            realm: row.realm,
            mapName: row.map_name,
            zoneName: `woe_${row.map_name}`,   // Zone registry key
            ownerGuildId: row.owner_guild_id,
            ownerGuildName: row.guild_name || null,
            economyLevel: row.economy_level || 0,
            defenseLevel: row.defense_level || 0,
            emperiumHp: maxHp,
            emperiumMaxHp: maxHp,

            // Runtime state (not persisted mid-WoE)
            guardians: [],           // Active guardian NPCs
            guardianStones: [        // WoE 2 only
                { alive: true, destroyedAt: null },
                { alive: true, destroyedAt: null },
            ],
            barricades: [            // WoE 2 only
                { installed: false, hp: 0 },
                { installed: false, hp: 0 },
                { installed: false, hp: 0 },
            ],
        });
    }
}

async function saveCastlesToDB(ctx) {
    for (const [castleId, castle] of activeCastles.entries()) {
        await ctx.pool.query(`
            UPDATE woe_castles SET
                owner_guild_id = $1,
                economy_level = $2,
                defense_level = $3,
                emperium_hp = $4,
                captured_at = CASE WHEN owner_guild_id != $1 THEN NOW()
                              ELSE captured_at END
            WHERE id = $5
        `, [
            castle.ownerGuildId,
            castle.economyLevel,
            castle.defenseLevel,
            castle.emperiumMaxHp,
            castleId,
        ]);

        // Log ownership history
        if (castle.ownerGuildId) {
            await ctx.pool.query(`
                INSERT INTO castle_ownership_history
                    (castle_id, guild_id, acquired_at, lost_at)
                VALUES ($1, $2, NOW(), NULL)
                ON CONFLICT (castle_id, guild_id, lost_at IS NULL)
                DO NOTHING
            `, [castleId, castle.ownerGuildId]).catch(() => {});
        }
    }
}
```

### 3.3 Emperium Mechanics

```javascript
// server/src/woe_scheduler.js -- Emperium damage + break logic

// Emperium stats (RO Classic reference)
const EMPERIUM_STATS = {
    level: 90,
    race: 'angel',
    element: { type: 'holy', level: 1 },
    size: 'small',
    softDef: 40,
    hardDef: 80,
    softMdef: 100,
    hardMdef: 90,
    flee: 107,
    baseHp: 68430,
};

function onEmperiumAttacked(ctx, castleId, attackerCharId) {
    if (woeState !== WOE_STATE.ACTIVE) return;

    const castle = activeCastles.get(castleId);
    if (!castle) return;

    const attacker = ctx.connectedPlayers.get(attackerCharId);
    if (!attacker) return;

    // --- Validation checks ---

    // Must have a guild
    if (!attacker.guildId) {
        const socket = getSocketForCharacter(attackerCharId);
        if (socket) socket.emit('woe:error', { message: 'You must be in a guild.' });
        return;
    }

    // Guild must have Official Guild Approval skill
    // (checked against guild_skills cache)
    if (!hasGuildSkill(attacker.guildId, 'official_approval')) {
        const socket = getSocketForCharacter(attackerCharId);
        if (socket) socket.emit('woe:error', {
            message: 'Your guild needs Official Guild Approval to attack Emperiums.'
        });
        return;
    }

    // Cannot attack your own guild's Emperium
    if (attacker.guildId === castle.ownerGuildId) return;

    // Cannot attack an allied guild's Emperium
    if (isAllied(attacker.guildId, castle.ownerGuildId)) return;

    // --- Damage calculation ---
    // ONLY normal (auto) attacks can damage the Emperium.
    // Skills always miss. This is enforced by the caller (combat tick),
    // which routes Emperium attacks through this function, not skill handlers.

    const attackerStats = getEffectiveStats(attacker);

    // Holy element check: Holy weapons deal 0% to Holy Emperium
    const weaponElement = attacker.weaponElement || 'neutral';
    if (weaponElement === 'holy') {
        // Holy vs Holy = 0 damage in RO element table
        ctx.broadcastToZone(castle.zoneName, 'woe:emperium_attack', {
            castleId,
            attackerId: attackerCharId,
            damage: 0,
            isMiss: true,
            reason: 'holy_vs_holy',
        });
        return;
    }

    // Calculate physical damage against Emperium using existing formulas
    const result = calculatePhysicalDamage(
        attackerStats,
        { // Target stats for Emperium
            def: EMPERIUM_STATS.softDef,
            vit: 0,
            agi: 1,
            luk: 0,
            level: EMPERIUM_STATS.level,
            flee: EMPERIUM_STATS.flee,
        },
        EMPERIUM_STATS.hardDef,
        { // Target info
            element: EMPERIUM_STATS.element,
            size: EMPERIUM_STATS.size,
            race: EMPERIUM_STATS.race,
            buffMods: { defMultiplier: 1.0 },
        },
        { // Attacker info
            weaponType: attacker.weaponType || 'bare_hand',
            weaponElement: weaponElement,
            weaponLevel: attacker.weaponLevel || 1,
            buffMods: { atkMultiplier: attackerStats.buffAtkMultiplier || 1.0 },
        },
        {} // options
    );

    if (result.isMiss) {
        ctx.broadcastToZone(castle.zoneName, 'woe:emperium_attack', {
            castleId,
            attackerId: attackerCharId,
            damage: 0,
            isMiss: true,
        });
        return;
    }

    // Apply damage
    const finalDamage = Math.max(1, result.damage);
    castle.emperiumHp = Math.max(0, castle.emperiumHp - finalDamage);

    // Broadcast HP update
    ctx.broadcastToZone(castle.zoneName, 'woe:emperium_hp', {
        castleId,
        hp: castle.emperiumHp,
        maxHp: castle.emperiumMaxHp,
        attackerId: attackerCharId,
        damage: finalDamage,
    });

    // Check for break
    if (castle.emperiumHp <= 0) {
        onCastleConquered(ctx, castleId, attacker.guildId, attackerCharId);
    }
}

function onCastleConquered(ctx, castleId, newGuildId, breakerCharId) {
    const castle = activeCastles.get(castleId);
    if (!castle) return;

    const oldGuildId = castle.ownerGuildId;
    const breaker = ctx.connectedPlayers.get(breakerCharId);
    const newGuildName = getGuildName(newGuildId);

    // Update ownership
    castle.ownerGuildId = newGuildId;
    castle.ownerGuildName = newGuildName;
    castle.emperiumHp = castle.emperiumMaxHp; // Reset Emperium for defense

    // Warp out ALL non-allied players
    warpNonAlliedFromCastle(ctx, castleId, newGuildId);

    // Despawn old guardians, spawn new ones for new owner
    despawnCastleGuardians(ctx, castleId);
    spawnCastleGuardians(ctx, castleId);

    // Server-wide announcement
    ctx.io.emit('woe:castle_captured', {
        castleId,
        castleName: castle.name,
        guildId: newGuildId,
        guildName: newGuildName,
        breakerName: breaker?.characterName || 'Unknown',
        previousGuildId: oldGuildId,
    });

    // Log to castle_ownership_history
    logOwnershipChange(ctx, castleId, oldGuildId, newGuildId);

    logger.info(`[WOE] ${castle.name} conquered by ${newGuildName} (breaker: ${breaker?.characterName})`);
}

async function logOwnershipChange(ctx, castleId, oldGuildId, newGuildId) {
    try {
        // Close old ownership record
        if (oldGuildId) {
            await ctx.pool.query(`
                UPDATE castle_ownership_history
                SET lost_at = NOW()
                WHERE castle_id = $1 AND guild_id = $2 AND lost_at IS NULL
            `, [castleId, oldGuildId]);
        }
        // Open new ownership record
        if (newGuildId) {
            await ctx.pool.query(`
                INSERT INTO castle_ownership_history (castle_id, guild_id, acquired_at)
                VALUES ($1, $2, NOW())
            `, [castleId, newGuildId]);
        }
    } catch (err) {
        logger.error(`[WOE] Failed to log ownership change: ${err.message}`);
    }
}
```

### 3.4 Castle Economy (Commerce/Defense Investment)

```javascript
// server/src/index.js -- Castle investment handler

socket.on('guild:invest', ({ castleId, type }) => {
    const characterId = socket.characterId;
    const player = connectedPlayers.get(characterId);
    if (!player || !player.guildId) return;

    const castle = activeCastles.get(castleId);
    if (!castle) {
        socket.emit('guild:error', { message: 'Castle not found.' });
        return;
    }

    // Must own the castle
    if (castle.ownerGuildId !== player.guildId) {
        socket.emit('guild:error', { message: 'Your guild does not own this castle.' });
        return;
    }

    // Must be guild master
    if (!isGuildMaster(player.guildId, characterId)) {
        socket.emit('guild:error', { message: 'Only the Guild Master can invest.' });
        return;
    }

    // Check daily investment limit (2 per day per type)
    const dailyKey = type === 'economy' ? 'economyInvestToday' : 'defenseInvestToday';
    if ((castle[dailyKey] || 0) >= 2) {
        socket.emit('guild:error', { message: `Daily ${type} investment limit reached (2/day).` });
        return;
    }

    // Calculate cost (each investment costs 2x the previous)
    const currentLevel = type === 'economy' ? castle.economyLevel : castle.defenseLevel;
    const baseCost = 5000;
    const cost = baseCost * Math.pow(2, castle[dailyKey] || 0);

    // Check guild funds (Zeny from guild master)
    if ((player.stats?.zeny || 0) < cost) {
        socket.emit('guild:error', { message: `Not enough Zeny. Cost: ${cost}z` });
        return;
    }

    // Deduct cost
    player.stats.zeny -= cost;

    // Apply investment
    let bonusPoint = 0;
    if (type === 'economy') {
        castle.economyLevel++;
        castle.economyInvestToday = (castle.economyInvestToday || 0) + 1;

        // Absolute Develop guild skill: 50% chance of free bonus economy point
        if (hasGuildSkill(player.guildId, 'absolute_develop')) {
            if (Math.random() < 0.5) {
                castle.economyLevel++;
                bonusPoint = 1;
            }
        }
    } else if (type === 'defense') {
        castle.defenseLevel++;
        castle.defenseInvestToday = (castle.defenseInvestToday || 0) + 1;

        // Recalculate Emperium max HP
        castle.emperiumMaxHp = EMPERIUM_STATS.baseHp + (castle.defenseLevel * 1000);
    }

    socket.emit('guild:invest_result', {
        castleId,
        type,
        newLevel: type === 'economy' ? castle.economyLevel : castle.defenseLevel,
        cost,
        bonusPoint,
    });

    socket.emit('stats:zeny_update', { zeny: player.stats.zeny });
});
```

### 3.5 Guardian NPC Spawning

```javascript
// server/src/woe_scheduler.js -- Guardian spawning and AI

// Guardian templates (simplified -- 2 types per castle)
const GUARDIAN_TEMPLATES = {
    melee_knight: {
        name: 'Castle Guardian Knight',
        hp: 50000,
        atk: 800,
        def: 60,
        aspd: 170,
        range: 150,      // melee
        walkSpeed: 300,
        chaseRange: 1500,
        respawnDelayMs: 30000,
    },
    ranged_archer: {
        name: 'Castle Guardian Archer',
        hp: 35000,
        atk: 600,
        def: 40,
        aspd: 175,
        range: 800,       // ranged
        walkSpeed: 250,
        chaseRange: 2000,
        respawnDelayMs: 30000,
    },
};

function spawnCastleGuardians(ctx, castleId) {
    const castle = activeCastles.get(castleId);
    if (!castle || !castle.ownerGuildId) return;

    // Check if guild has Guardian Research skill
    if (!hasGuildSkill(castle.ownerGuildId, 'guardian_research')) return;

    // Strengthen Guardian bonuses
    const strengthenLevel = getGuildSkillLevel(castle.ownerGuildId, 'strengthen_guardian');
    const hpBonus = 1.0 + (strengthenLevel * 0.1);    // +10% per level
    const atkBonus = 1.0 + (strengthenLevel * 0.05);   // +5% per level

    // Defense investment bonus: +1000 HP per defense point
    const defenseHpBonus = castle.defenseLevel * 1000;

    // Spawn positions defined per castle in zone config
    const guardianSlots = getCastleGuardianSlots(castleId);

    castle.guardians = [];
    for (const slot of guardianSlots) {
        const template = GUARDIAN_TEMPLATES[slot.type];
        if (!template) continue;

        const guardian = {
            id: `guardian_${castleId}_${slot.index}`,
            castleId,
            type: slot.type,
            name: template.name,
            hp: Math.floor(template.hp * hpBonus) + defenseHpBonus,
            maxHp: Math.floor(template.hp * hpBonus) + defenseHpBonus,
            atk: Math.floor(template.atk * atkBonus),
            def: template.def,
            aspd: template.aspd,
            range: template.range,
            walkSpeed: template.walkSpeed,
            chaseRange: template.chaseRange,
            x: slot.x, y: slot.y, z: slot.z,
            homeX: slot.x, homeY: slot.y, homeZ: slot.z,
            ownerGuildId: castle.ownerGuildId,
            targetCharId: null,
            state: 'idle',   // idle | chase | attack | dead
            lastAttackTime: 0,
            diedAt: null,
        };

        castle.guardians.push(guardian);

        // Broadcast spawn to castle zone
        ctx.broadcastToZone(castle.zoneName, 'woe:guardian_spawned', {
            castleId,
            guardianId: guardian.id,
            type: guardian.type,
            name: guardian.name,
            hp: guardian.hp,
            maxHp: guardian.maxHp,
            x: guardian.x, y: guardian.y, z: guardian.z,
        });
    }
}

function despawnCastleGuardians(ctx, castleId) {
    const castle = activeCastles.get(castleId);
    if (!castle) return;

    for (const guardian of castle.guardians) {
        ctx.broadcastToZone(castle.zoneName, 'woe:guardian_despawned', {
            castleId,
            guardianId: guardian.id,
        });
    }

    castle.guardians = [];
}
```

### 3.6 Treasure Room

```javascript
// server/src/woe_scheduler.js -- Treasure box generation (runs at POST_WOE)

async function generateTreasureBoxes(ctx) {
    for (const [castleId, castle] of activeCastles.entries()) {
        if (!castle.ownerGuildId) continue;

        // Base: 4 treasure boxes
        // Commerce bonus: +1 per 5 economy points
        const baseBoxes = 4;
        const commerceBonus = Math.floor(castle.economyLevel / 5);
        const totalBoxes = baseBoxes + commerceBonus;

        // Generate loot for each box
        const treasureItems = [];
        for (let i = 0; i < totalBoxes; i++) {
            const loot = rollTreasureBoxLoot(castle.realm);
            treasureItems.push(loot);
        }

        // Store in DB for guild master to collect
        await ctx.pool.query(`
            INSERT INTO woe_treasure_log (castle_id, guild_id, items_json, box_count)
            VALUES ($1, $2, $3, $4)
        `, [castleId, castle.ownerGuildId, JSON.stringify(treasureItems), totalBoxes]);
    }
}

function rollTreasureBoxLoot(realm) {
    // Each box rolls from a realm-specific loot table
    // God Item materials have ~0.4% (1/250) drop rate
    const GOD_ITEM_CHANCE = 0.004;
    const items = [];

    // Roll 3-5 items per box
    const numItems = 3 + Math.floor(Math.random() * 3);
    for (let i = 0; i < numItems; i++) {
        if (Math.random() < GOD_ITEM_CHANCE) {
            items.push({
                itemId: getGodItemMaterialForRealm(realm),
                amount: 1,
                isGodMaterial: true,
            });
        } else {
            items.push(rollNormalTreasureLoot(realm));
        }
    }

    return items;
}
```

### 3.7 WoE Combat Rules

```javascript
// server/src/woe_scheduler.js -- WoE-specific combat modifiers

// WoE damage reduction table
const WOE_DAMAGE_REDUCTION = {
    skillDamage: 0.40,          // -40% for skill-based damage (magic + physical skills)
    longRangeNormal: 0.25,      // -25% for long-range normal attacks
    shortRangeNormal: 0.0,      // No reduction for melee normal attacks
    fleeReduction: 0.20,        // -20% flee for all players
    trapDurationMultiplier: 4,  // 4x trap duration
    potionEffectiveness: 0.50,  // 50% potion effectiveness (server-configurable)
};

function applyWoEDamageReduction(baseDamage, options = {}) {
    let reduction = 0;

    if (options.isSkillDamage) {
        reduction = WOE_DAMAGE_REDUCTION.skillDamage;
    } else if (options.isLongRange) {
        reduction = WOE_DAMAGE_REDUCTION.longRangeNormal;
    }
    // Short-range normal attacks: no reduction

    return Math.max(1, Math.floor(baseDamage * (1 - reduction)));
}

// Disabled skills during WoE (checked in skill:use handler)
const WOE_DISABLED_SKILLS = new Set([
    26,     // Teleport
    27,     // Warp Portal
    14,     // Ice Wall
    316,    // Basilica
    353,    // Assumptio
    331,    // Back Slide (optional)
    // Plant Cultivation, Moonlit Water Mill, Intimidate, Hocus Pocus
]);

function isSkillBlockedInWoE(skillId, zone) {
    const zoneData = getZone(zone);
    if (!zoneData) return false;
    if (!zoneData.flags.woe && !zoneData.flags.woeSE) return false;
    return WOE_DISABLED_SKILLS.has(skillId);
}
```

### 3.8 Guardian Stone + Barricade (WoE 2)

```javascript
// server/src/woe_scheduler.js -- WoE 2 / SE defensive structures

const GUARDIAN_STONE_HP = 200000;
const GUARDIAN_STONE_REPAIR_COOLDOWN_MS = 8 * 60 * 1000; // 8 minutes
const BARRICADE_BLOCK_HP = 450000;

function onGuardianStoneDamaged(ctx, castleId, stoneIndex, attackerCharId, damage) {
    if (woeState !== WOE_STATE.ACTIVE) return;

    const castle = activeCastles.get(castleId);
    if (!castle) return;

    const stone = castle.guardianStones[stoneIndex];
    if (!stone || !stone.alive) return;

    // Validate attacker guild
    const attacker = ctx.connectedPlayers.get(attackerCharId);
    if (!attacker?.guildId) return;
    if (attacker.guildId === castle.ownerGuildId) return;
    if (isAllied(attacker.guildId, castle.ownerGuildId)) return;

    stone.hp = Math.max(0, (stone.hp || GUARDIAN_STONE_HP) - damage);

    ctx.broadcastToZone(castle.zoneName, 'woe:guardian_stone_hp', {
        castleId, stoneIndex,
        hp: stone.hp,
        maxHp: GUARDIAN_STONE_HP,
    });

    if (stone.hp <= 0) {
        stone.alive = false;
        stone.destroyedAt = Date.now();

        // Kill guardians spawned by this stone
        despawnGuardiansForStone(ctx, castleId, stoneIndex);

        ctx.broadcastToZone(castle.zoneName, 'woe:guardian_stone_destroyed', {
            castleId, stoneIndex,
            attackerId: attackerCharId,
            attackerName: attacker.characterName,
        });

        // Check if both stones destroyed -- open path to barricades
        const allDestroyed = castle.guardianStones.every(s => !s.alive);
        if (allDestroyed) {
            ctx.broadcastToZone(castle.zoneName, 'woe:barrier_opened', { castleId });
        }
    }
}

function onRepairGuardianStone(ctx, castleId, stoneIndex, repairerCharId) {
    const castle = activeCastles.get(castleId);
    if (!castle) return;

    const stone = castle.guardianStones[stoneIndex];
    if (!stone || stone.alive) return;

    // Check repair cooldown (8 minutes after destruction)
    if (stone.destroyedAt && Date.now() - stone.destroyedAt < GUARDIAN_STONE_REPAIR_COOLDOWN_MS) {
        const remaining = Math.ceil(
            (GUARDIAN_STONE_REPAIR_COOLDOWN_MS - (Date.now() - stone.destroyedAt)) / 1000
        );
        const socket = getSocketForCharacter(repairerCharId);
        if (socket) socket.emit('woe:error', {
            message: `Guardian Stone cannot be repaired for ${remaining} more seconds.`
        });
        return;
    }

    // Check repair materials: 30 Stone, 1 Oridecon, 1 Elunium,
    //                         5 Blue Gem, 5 Red Gem, 5 Yellow Gem
    // (inventory check omitted for brevity -- same pattern as existing item consumption)

    // Repair the stone
    stone.alive = true;
    stone.hp = GUARDIAN_STONE_HP;
    stone.destroyedAt = null;

    // Respawn guardians for this stone
    spawnGuardiansForStone(ctx, castleId, stoneIndex);

    ctx.broadcastToZone(castle.zoneName, 'woe:guardian_stone_repaired', {
        castleId, stoneIndex,
    });
}

function onBarricadeDamaged(ctx, castleId, barricadeIndex, blockIndex, damage) {
    const castle = activeCastles.get(castleId);
    if (!castle) return;

    // Both guardian stones must be destroyed to damage barricades
    if (castle.guardianStones.some(s => s.alive)) return;

    const barricade = castle.barricades[barricadeIndex];
    if (!barricade || !barricade.installed) return;

    if (!barricade.blocks) return;
    const block = barricade.blocks[blockIndex];
    if (!block || block.hp <= 0) return;

    block.hp = Math.max(0, block.hp - damage);

    ctx.broadcastToZone(castle.zoneName, 'woe:barricade_hp', {
        castleId, barricadeIndex, blockIndex,
        hp: block.hp,
        maxHp: BARRICADE_BLOCK_HP,
    });

    if (block.hp <= 0) {
        // Check if ALL blocks in this barricade set are destroyed
        const allBlocksDestroyed = barricade.blocks.every(b => b.hp <= 0);
        if (allBlocksDestroyed) {
            ctx.broadcastToZone(castle.zoneName, 'woe:barricade_destroyed', {
                castleId, barricadeIndex,
            });
        }
    }
}
```

---

## 4. WoE Client

### 4.1 WoESubsystem (UWorldSubsystem)

```cpp
// client/SabriMMO/Source/SabriMMO/UI/WoESubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "WoESubsystem.generated.h"

class USocketIOClientComponent;
class SWoEStatusWidget;

/** WoE state enum matching server WOE_STATE */
UENUM(BlueprintType)
enum class EWoEState : uint8
{
    Inactive    UMETA(DisplayName = "Inactive"),
    PreWoE      UMETA(DisplayName = "Pre-WoE"),
    Active      UMETA(DisplayName = "Active"),
    PostWoE     UMETA(DisplayName = "Post-WoE"),
};

/** Castle info received from server */
USTRUCT(BlueprintType)
struct FCastleInfo
{
    GENERATED_BODY()

    UPROPERTY() int32 CastleId = 0;
    UPROPERTY() FString CastleName;
    UPROPERTY() FString Realm;
    UPROPERTY() int32 OwnerGuildId = 0;
    UPROPERTY() FString OwnerGuildName;
    UPROPERTY() int32 EmperiumHp = 0;
    UPROPERTY() int32 EmperiumMaxHp = 0;
    UPROPERTY() int32 EconomyLevel = 0;
    UPROPERTY() int32 DefenseLevel = 0;
};

UCLASS()
class SABRIMMO_API UWoESubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Public state (read by widget) ----
    EWoEState CurrentState = EWoEState::Inactive;
    int64 WoEEndTimeEpochMs = 0;

    /** Castle the local player is currently inside (0 = not in a castle) */
    int32 CurrentCastleId = 0;

    /** All castle ownership data (populated on woe:start and woe:castle_captured) */
    TArray<FCastleInfo> CastleInfos;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Widget ----
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

    // ---- Delegates (for other subsystems/widgets to bind) ----
    DECLARE_MULTICAST_DELEGATE(FOnWoEStateChanged);
    FOnWoEStateChanged OnWoEStateChanged;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEmperiumHpChanged, int32 /*Hp*/, int32 /*MaxHp*/);
    FOnEmperiumHpChanged OnEmperiumHpChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnWoEAnnouncement, const FString& /*Message*/);
    FOnWoEAnnouncement OnWoEAnnouncement;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCastleCaptured, int32 /*CastleId*/, const FString& /*GuildName*/);
    FOnCastleCaptured OnCastleCaptured;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Socket event handlers ----
    void HandleWoEStart(const TSharedPtr<FJsonValue>& Data);
    void HandleWoEEnd(const TSharedPtr<FJsonValue>& Data);
    void HandleWoEAnnouncement(const TSharedPtr<FJsonValue>& Data);
    void HandleEmperiumHp(const TSharedPtr<FJsonValue>& Data);
    void HandleCastleCaptured(const TSharedPtr<FJsonValue>& Data);
    void HandleGuardianSpawned(const TSharedPtr<FJsonValue>& Data);
    void HandleGuardianKilled(const TSharedPtr<FJsonValue>& Data);

    // ---- Widget ----
    TSharedPtr<SWoEStatusWidget> WoEWidget;
    bool bEventsWrapped = false;
};
```

### 4.2 WoESubsystem Implementation (Skeleton)

```cpp
// client/SabriMMO/Source/SabriMMO/UI/WoESubsystem.cpp

#include "UI/WoESubsystem.h"
#include "SocketIOClientComponent.h"
#include "Engine/World.h"

bool UWoESubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Create in all levels (WoE announcements are global)
    UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UWoESubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    TryWrapSocketEvents();
}

void UWoESubsystem::Deinitialize()
{
    HideWidget();
    Super::Deinitialize();
}

void UWoESubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    WrapSingleEvent(TEXT("woe:start"), [this](const TSharedPtr<FJsonValue>& D) { HandleWoEStart(D); });
    WrapSingleEvent(TEXT("woe:end"), [this](const TSharedPtr<FJsonValue>& D) { HandleWoEEnd(D); });
    WrapSingleEvent(TEXT("woe:announcement"), [this](const TSharedPtr<FJsonValue>& D) { HandleWoEAnnouncement(D); });
    WrapSingleEvent(TEXT("woe:emperium_hp"), [this](const TSharedPtr<FJsonValue>& D) { HandleEmperiumHp(D); });
    WrapSingleEvent(TEXT("woe:castle_captured"), [this](const TSharedPtr<FJsonValue>& D) { HandleCastleCaptured(D); });
    WrapSingleEvent(TEXT("woe:guardian_spawned"), [this](const TSharedPtr<FJsonValue>& D) { HandleGuardianSpawned(D); });
    WrapSingleEvent(TEXT("woe:guardian_killed"), [this](const TSharedPtr<FJsonValue>& D) { HandleGuardianKilled(D); });

    bEventsWrapped = true;
}

void UWoESubsystem::HandleWoEStart(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* Obj;
    if (!Data.IsValid() || !Data->TryGetObject(Obj)) return;

    CurrentState = EWoEState::Active;

    double EndTime = 0;
    (*Obj)->TryGetNumberField(TEXT("endTime"), EndTime);
    WoEEndTimeEpochMs = static_cast<int64>(EndTime);

    OnWoEStateChanged.Broadcast();
    ShowWidget();
}

void UWoESubsystem::HandleWoEEnd(const TSharedPtr<FJsonValue>& Data)
{
    CurrentState = EWoEState::Inactive;
    WoEEndTimeEpochMs = 0;
    OnWoEStateChanged.Broadcast();

    // Keep widget visible briefly to show results, then hide after 30s
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]() {
        HideWidget();
    }, 30.0f, false);
}

void UWoESubsystem::HandleEmperiumHp(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* Obj;
    if (!Data.IsValid() || !Data->TryGetObject(Obj)) return;

    int32 Hp = 0, MaxHp = 0;
    (*Obj)->TryGetNumberField(TEXT("hp"), Hp);
    (*Obj)->TryGetNumberField(TEXT("maxHp"), MaxHp);

    // Update local castle info
    int32 CastleId = 0;
    (*Obj)->TryGetNumberField(TEXT("castleId"), CastleId);
    for (FCastleInfo& Info : CastleInfos)
    {
        if (Info.CastleId == CastleId)
        {
            Info.EmperiumHp = Hp;
            Info.EmperiumMaxHp = MaxHp;
            break;
        }
    }

    OnEmperiumHpChanged.Broadcast(Hp, MaxHp);
}

void UWoESubsystem::HandleCastleCaptured(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* Obj;
    if (!Data.IsValid() || !Data->TryGetObject(Obj)) return;

    int32 CastleId = 0;
    FString GuildName;
    (*Obj)->TryGetNumberField(TEXT("castleId"), CastleId);
    (*Obj)->TryGetStringField(TEXT("guildName"), GuildName);

    OnCastleCaptured.Broadcast(CastleId, GuildName);
    OnWoEAnnouncement.Broadcast(
        FString::Printf(TEXT("The castle has been conquered by [%s]!"), *GuildName)
    );
}

// FindSocketIOComponent and WrapSingleEvent follow the same pattern
// as BasicInfoSubsystem (find SocketManager actor, iterate components)
USocketIOClientComponent* UWoESubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName().Contains(TEXT("SocketManager")))
        {
            return It->FindComponentByClass<USocketIOClientComponent>();
        }
    }
    return nullptr;
}

void UWoESubsystem::WrapSingleEvent(const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> Handler)
{
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    SIO->OnNativeEvent.AddLambda([EventName, Handler](const FString& Event, const TSharedPtr<FJsonValue>& Data)
    {
        if (Event == EventName)
        {
            Handler(Data);
        }
    });
}
```

### 4.3 PvPSubsystem (UWorldSubsystem)

```cpp
// client/SabriMMO/Source/SabriMMO/UI/PvPSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "PvPSubsystem.generated.h"

class USocketIOClientComponent;
class SPvPRankingWidget;

USTRUCT()
struct FPvPRankEntry
{
    GENERATED_BODY()

    int32 CharId = 0;
    FString Name;
    int32 Points = 0;
    int32 Kills = 0;
    int32 Deaths = 0;
    int32 Streak = 0;
    int32 Rank = 0;
};

UCLASS()
class SABRIMMO_API UPvPSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Public state ----
    bool bIsInPvPArena = false;
    int32 LocalPoints = 0;
    int32 LocalKills = 0;
    int32 LocalDeaths = 0;
    int32 LocalRank = 0;
    TArray<FPvPRankEntry> Rankings;

    // ---- Duel state ----
    bool bIsInDuel = false;
    int32 DuelOpponentId = 0;
    FString DuelOpponentName;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Widget ----
    void ShowRankingWidget();
    void HideRankingWidget();

    // ---- Delegates ----
    DECLARE_MULTICAST_DELEGATE(FOnPvPRankingsUpdated);
    FOnPvPRankingsUpdated OnPvPRankingsUpdated;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPvPKill, const FString& /*KillerName*/, const FString& /*VictimName*/);
    FOnPvPKill OnPvPKill;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnPvPKicked, const FString& /*Reason*/);
    FOnPvPKicked OnPvPKicked;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnDuelRequest, const FString& /*ChallengerName*/);
    FOnDuelRequest OnDuelRequest;

private:
    void TryWrapSocketEvents();
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandlePvPEntered(const TSharedPtr<FJsonValue>& Data);
    void HandlePvPKill(const TSharedPtr<FJsonValue>& Data);
    void HandlePvPKicked(const TSharedPtr<FJsonValue>& Data);
    void HandlePvPRankingsUpdate(const TSharedPtr<FJsonValue>& Data);
    void HandleDuelRequestReceived(const TSharedPtr<FJsonValue>& Data);
    void HandleDuelStarted(const TSharedPtr<FJsonValue>& Data);
    void HandleDuelEnded(const TSharedPtr<FJsonValue>& Data);

    TSharedPtr<SPvPRankingWidget> RankingWidget;
    bool bEventsWrapped = false;
    int32 PendingDuelRequestId = 0;
};
```

### 4.4 WoE Schedule Display Widget

```cpp
// client/SabriMMO/Source/SabriMMO/UI/SWoEStatusWidget.h
#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UWoESubsystem;

/**
 * Slate widget showing WoE status:
 * - Countdown timer (Pre-WoE / during WoE)
 * - Castle ownership overview (mini-map with guild emblems)
 * - Emperium HP bar (when inside a castle zone)
 * - WoE announcements feed
 *
 * Z-Order: 22 (above most HUD elements, below loading overlay)
 * Pattern: same OnPaint approach as WorldHealthBarSubsystem
 */
class SABRIMMO_API SWoEStatusWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWoEStatusWidget) {}
        SLATE_ARGUMENT(UWoESubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateClipRectangleList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    UWoESubsystem* Subsystem = nullptr;

    // Paint helpers
    void PaintCountdownTimer(const FGeometry& Geo, FSlateWindowElementList& OutElements,
        int32 LayerId) const;
    void PaintEmperiumHpBar(const FGeometry& Geo, FSlateWindowElementList& OutElements,
        int32 LayerId) const;
    void PaintCastleOwnershipMap(const FGeometry& Geo, FSlateWindowElementList& OutElements,
        int32 LayerId) const;
    void PaintAnnouncementFeed(const FGeometry& Geo, FSlateWindowElementList& OutElements,
        int32 LayerId) const;

    // Countdown
    FString FormatTimeRemaining() const;
};
```

### 4.5 Emergency Call UI

Emergency Call is a guild skill that teleports all online guild members to the Guild Master's location. The client shows a fullscreen teleport effect.

```cpp
// Handled within WoESubsystem -- on receiving 'guild:skill_used' with skillId for Emergency Call:

void UWoESubsystem::HandleGuildSkillUsed(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>* Obj;
    if (!Data.IsValid() || !Data->TryGetObject(Obj)) return;

    int32 SkillId = 0;
    (*Obj)->TryGetNumberField(TEXT("skillId"), SkillId);

    // Emergency Call (guild skill ID defined in ro_guild_skill_data)
    if (SkillId == GUILD_SKILL_EMERGENCY_CALL)
    {
        // Show a "Summoned by Guild Master" overlay
        // The server handles the actual teleport via zone:change event
        OnWoEAnnouncement.Broadcast(TEXT("Guild Master has used Emergency Call!"));

        // The zone transition is handled by ZoneTransitionSubsystem
        // when the server sends zone:change with the GM's location
    }
}
```

---

## 5. Castle Levels

### 5.1 Design Requirements

Each castle is a separate UE5 level following the standard zone conventions. Castle levels have unique layout requirements for WoE gameplay.

**Level naming:** `L_WoEPrtCas01` through `L_WoEPrtCas05` (Valkyrie realm), `L_WoEPayCas01` through `L_WoEPayCas05` (Balder realm), etc.

**Always duplicate** from an existing working level (e.g., `L_PrtDungeon01`) to preserve the Level Blueprint.

**Zone registry entries for castle zones:**

```javascript
// server/src/ro_zone_data.js -- Castle zone example
woe_prtg_cas01: {
    name: 'woe_prtg_cas01',
    displayName: 'Kriemhild Castle',
    type: 'castle',
    flags: {
        noteleport: true,
        noreturn: true,
        nosave: true,
        pvp: false,          // PvP is handled by WoE guild logic, not generic PvP
        town: false,
        indoor: true,
        woe: true,            // WoE 1 castle
        noKnockback: true,    // No knockback during WoE
        noDuel: true,
    },
    defaultSpawn: { x: 0, y: 0, z: 300 },
    levelName: 'L_WoEPrtCas01',
    warps: [
        {
            id: 'prtcas01_entrance',
            x: 0, y: -2000, z: 300,
            radius: 200,
            destZone: 'woe_prtg_entrance', // Shared realm entrance map
            destX: 0, destY: 0, destZ: 300,
        }
    ],
    kafraNpcs: [],
    enemySpawns: [],        // Guardians are spawned dynamically, not in enemySpawns
    castleConfig: {
        castleId: 1,         // Matches woe_castles.id in DB
        realm: 'valkyrie',
        emperiumPosition: { x: 0, y: 1500, z: 300 },  // Innermost room
        guardianSlots: [
            { index: 0, type: 'melee_knight', x: -200, y: 500, z: 300 },
            { index: 1, type: 'ranged_archer', x: 200, y: 500, z: 300 },
            { index: 2, type: 'melee_knight', x: 0, y: 1000, z: 300 },
        ],
        // WoE 2 only:
        guardianStonePositions: [
            { x: -500, y: -500, z: 300 },
            { x: 500, y: -500, z: 300 },
        ],
        barricadePositions: [
            { setIndex: 0, blocks: [
                { x: -200, y: 0, z: 300 }, { x: 0, y: 0, z: 300 },
                { x: 200, y: 0, z: 300 }, { x: 400, y: 0, z: 300 },
            ]},
            { setIndex: 1, blocks: [
                { x: -200, y: 500, z: 300 }, { x: 0, y: 500, z: 300 },
                { x: 200, y: 500, z: 300 }, { x: 400, y: 500, z: 300 },
            ]},
            { setIndex: 2, blocks: [
                { x: -200, y: 1000, z: 300 }, { x: 0, y: 1000, z: 300 },
                { x: 200, y: 1000, z: 300 }, { x: 400, y: 1000, z: 300 },
            ]},
        ],
        linkFlagPositions: [
            { x: -800, y: -800, z: 300 },
            { x: 800, y: -800, z: 300 },
            { x: -800, y: 500, z: 300 },
            { x: 800, y: 500, z: 300 },
            { x: 0, y: 1200, z: 300 },
            // ... up to 12 flags
        ],
    },
}
```

### 5.2 Castle Layout Requirements

Each WoE 1 castle level must include:

1. **Entrance portal** -- overlap trigger at the entrance, warp point for attackers.
2. **3-5 rooms** -- connected by narrow corridors for defensive choke points.
3. **Emperium Room** -- innermost room where BP_Emperium is placed. Position matches `castleConfig.emperiumPosition`.
4. **Guardian spawn points** -- marked with invisible actors at `castleConfig.guardianSlots` positions.
5. **Treasure Room** -- separate enclosed area accessible only via NPC interaction.
6. **Castle NPC positions** -- Investment NPC, Guardian Hire NPC, Kafra NPC (for owning guild).

WoE 2 (SE) castles additionally require:

7. **Guardian Stones** -- 2 destructible objects near the entrance area.
8. **Barrier wall** -- visual barrier between entrance and interior, removed when both stones are destroyed.
9. **Barricade positions** -- 3 sets of 4-8 block positions between stones and Emperium.
10. **Link Flags** -- 12 interactable flag positions for defender fast-travel.

### 5.3 Blueprint Actors for Castle Levels

| Actor | Purpose | Placement |
|-------|---------|-----------|
| `BP_Emperium` | Destructible target. Implements `BPI_Damageable`. Only accepts normal attacks (auto-attack). Holy element = 0 dmg. Server tracks HP. | Emperium Room center |
| `BP_CastleGuardian` | Server-spawned NPC. AI: chase nearest non-allied player. Despawns on WoE end. | Dynamic (server-controlled positions) |
| `BP_CastlePortal` | Entry portal. During WoE: open to all. Outside WoE: open to owning guild only. | Castle entrance |
| `BP_TreasureBox` | Interactable chest. Only Guild Master can open. Shows loot UI. | Treasure Room |
| `BP_GuardianStone` | Destructible object (WoE 2). 200k HP. 8min repair cooldown. | Near castle entrance |
| `BP_Barricade` | Destructible wall block (WoE 2). 450k HP. Cannot be repaired during active WoE. | Between stones and Emperium |
| `BP_LinkFlag` | Click-to-teleport flag (WoE 2). Defenders only. | Throughout castle interior |
| `BP_InvestmentNPC` | Guild Master interaction: Commerce/Defense investment. | Castle interior room |

---

## 6. Scalability

### 6.1 Interest Management (100+ Players)

WoE regularly involves 100-200+ players on a single castle map. The existing `broadcastToZone()` pattern sends every event to every player in the zone. This must be optimized.

```javascript
// server/src/index.js -- Interest management for WoE zones

const INTEREST_RADIUS = 2500;        // UE units (~50 RO cells)
const INTEREST_RADIUS_REDUCED = 1500; // For less important data (buffs, animations)

/**
 * Broadcast to nearby players only (interest management).
 * Use for position updates, damage numbers, and animations.
 * Castle-wide events (Emperium break, announcements) still use broadcastToZone.
 */
function broadcastToNearby(zone, x, y, z, event, data, radius = INTEREST_RADIUS) {
    const room = io.sockets.adapter.rooms.get(zone);
    if (!room) return;

    for (const socketId of room) {
        const s = io.sockets.sockets.get(socketId);
        if (!s?.characterId) continue;

        const player = connectedPlayers.get(s.characterId);
        if (!player) continue;

        const dx = (player.lastX || 0) - x;
        const dy = (player.lastY || 0) - y;
        const dist = Math.sqrt(dx * dx + dy * dy);

        if (dist <= radius) {
            s.emit(event, data);
        }
    }
}

/**
 * During WoE, use reduced-frequency position broadcasts.
 * Non-WoE: every position update is broadcast (existing behavior).
 * WoE zones: batch and throttle to 200ms intervals.
 */
const woePositionBuffer = new Map(); // zone -> Map<charId, posData>

function bufferWoEPosition(zone, charId, posData) {
    if (!woePositionBuffer.has(zone)) {
        woePositionBuffer.set(zone, new Map());
    }
    woePositionBuffer.get(zone).set(charId, posData);
}

// Flush WoE position buffers every 200ms (instead of per-update)
setInterval(() => {
    for (const [zone, positions] of woePositionBuffer.entries()) {
        if (positions.size === 0) continue;

        // Batch all positions into a single event
        const batch = [];
        for (const [charId, pos] of positions.entries()) {
            batch.push({ charId, ...pos });
        }

        broadcastToZone(zone, 'player:batch_moved', { players: batch });
        positions.clear();
    }
}, 200);
```

### 6.2 Damage Throttling

```javascript
// server/src/index.js -- Damage aggregation for WoE

const woeDamageBuffer = new Map(); // zone -> Map<targetId, { totalDamage, hits, lastBroadcast }>
const WOE_DAMAGE_BROADCAST_INTERVAL = 100; // Aggregate over 100ms windows

function bufferWoEDamage(zone, targetId, damage, attackerId) {
    if (!woeDamageBuffer.has(zone)) {
        woeDamageBuffer.set(zone, new Map());
    }
    const zoneBuffer = woeDamageBuffer.get(zone);

    if (!zoneBuffer.has(targetId)) {
        zoneBuffer.set(targetId, { totalDamage: 0, hits: 0, attackers: new Set() });
    }
    const entry = zoneBuffer.get(targetId);
    entry.totalDamage += damage;
    entry.hits++;
    entry.attackers.add(attackerId);
}

// Flush damage buffer every 100ms
setInterval(() => {
    for (const [zone, targets] of woeDamageBuffer.entries()) {
        for (const [targetId, entry] of targets.entries()) {
            if (entry.hits === 0) continue;

            // Broadcast aggregated damage
            broadcastToZone(zone, 'combat:damage_batch', {
                targetId,
                totalDamage: entry.totalDamage,
                hits: entry.hits,
                attackerCount: entry.attackers.size,
            });

            // Reset
            entry.totalDamage = 0;
            entry.hits = 0;
            entry.attackers.clear();
        }
    }
}, WOE_DAMAGE_BROADCAST_INTERVAL);
```

### 6.3 Emperium Damage Queue

```javascript
// server/src/woe_scheduler.js -- Emperium damage queue (prevent race conditions)

const emperiumDamageQueue = new Map(); // castleId -> Array<{ attackerId, damage }>

function queueEmperiumDamage(castleId, attackerId, damage) {
    if (!emperiumDamageQueue.has(castleId)) {
        emperiumDamageQueue.set(castleId, []);
    }
    emperiumDamageQueue.get(castleId).push({ attackerId, damage });
}

// Process Emperium damage in batches (every 100ms)
setInterval(() => {
    for (const [castleId, queue] of emperiumDamageQueue.entries()) {
        if (queue.length === 0) continue;

        const castle = activeCastles.get(castleId);
        if (!castle || castle.emperiumHp <= 0) {
            queue.length = 0;
            continue;
        }

        // Process all queued damage
        let totalDamage = 0;
        let lastAttackerId = null;

        for (const entry of queue) {
            totalDamage += entry.damage;
            lastAttackerId = entry.attackerId;

            castle.emperiumHp -= entry.damage;
            if (castle.emperiumHp <= 0) {
                castle.emperiumHp = 0;
                // The LAST attacker in the queue who pushed HP to 0 gets credit
                const attacker = connectedPlayers.get(entry.attackerId);
                if (attacker?.guildId) {
                    onCastleConquered(ctx, castleId, attacker.guildId, entry.attackerId);
                }
                break;
            }
        }

        // Clear queue
        queue.length = 0;

        // Broadcast aggregated HP update
        if (castle.emperiumHp > 0) {
            broadcastToZone(castle.zoneName, 'woe:emperium_hp', {
                castleId,
                hp: castle.emperiumHp,
                maxHp: castle.emperiumMaxHp,
            });
        }
    }
}, 100);
```

### 6.4 Client-Side LOD (UE5)

```cpp
// client/SabriMMO/Source/SabriMMO/WoEPlayerLOD.h
// Component added to BP_OtherPlayer during WoE zones

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WoEPlayerLOD.generated.h"

/**
 * Manages Level of Detail for remote players during WoE.
 * Attached dynamically by BP_OtherPlayerManager when in a WoE zone.
 *
 * LOD tiers:
 *   0-25m:  Full detail (all mesh, VFX, nameplate)
 *   25-50m: Reduced mesh, no VFX, simple nameplate
 *   50m+:   Billboard sprite only, no nameplate
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SABRIMMO_API UWoEPlayerLOD : public UActorComponent
{
    GENERATED_BODY()

public:
    UWoEPlayerLOD();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, Category = "LOD")
    float HighDetailDistance = 2500.0f;   // ~25m

    UPROPERTY(EditAnywhere, Category = "LOD")
    float MediumDetailDistance = 5000.0f; // ~50m

private:
    void UpdateLOD();
    float GetDistanceToLocalPlayer() const;

    int32 CurrentLODTier = 0;
};
```

### 6.5 Effect Culling

```cpp
// In WoE zones, SkillVFXSubsystem should cull non-essential effects when
// the player count exceeds a threshold.

// In SkillVFXSubsystem.cpp, add a check:
void USkillVFXSubsystem::SpawnVFXEffect(/* params */)
{
    // During WoE with many players, skip non-critical VFX
    UWoESubsystem* WoE = GetWorld()->GetSubsystem<UWoESubsystem>();
    if (WoE && WoE->CurrentState == EWoEState::Active)
    {
        int32 NearbyPlayerCount = GetNearbyPlayerCount();
        if (NearbyPlayerCount > 30)
        {
            // Only render damage numbers and Emperium effects
            if (!bIsCriticalEffect)
            {
                return; // Skip this VFX
            }
        }
    }

    // ... normal VFX spawn logic
}
```

### 6.6 Redis Caching for Castle State

```javascript
// server/src/woe_scheduler.js -- Redis caching during active WoE

const CASTLE_STATE_KEY = 'woe:castle_state';

async function cacheActiveCastles(redis) {
    if (!redis) return;

    const stateObj = {};
    for (const [castleId, castle] of activeCastles.entries()) {
        stateObj[castleId] = {
            ownerGuildId: castle.ownerGuildId,
            emperiumHp: castle.emperiumHp,
            emperiumMaxHp: castle.emperiumMaxHp,
            guardianStones: castle.guardianStones,
        };
    }

    await redis.set(CASTLE_STATE_KEY, JSON.stringify(stateObj), 'EX', 300); // 5min TTL
}

// Cache every 10 seconds during active WoE
setInterval(async () => {
    if (woeState === WOE_STATE.ACTIVE) {
        await cacheActiveCastles(redis);
    }
}, 10000);

// On server restart during WoE, restore from Redis before DB
async function restoreCastleStateFromCache(redis) {
    if (!redis) return false;

    const cached = await redis.get(CASTLE_STATE_KEY);
    if (!cached) return false;

    const stateObj = JSON.parse(cached);
    for (const [castleId, state] of Object.entries(stateObj)) {
        const castle = activeCastles.get(parseInt(castleId));
        if (castle) {
            castle.ownerGuildId = state.ownerGuildId;
            castle.emperiumHp = state.emperiumHp;
            castle.emperiumMaxHp = state.emperiumMaxHp;
            castle.guardianStones = state.guardianStones;
        }
    }

    return true;
}
```

### 6.7 Server Optimization Summary

| Optimization | Normal Zone | WoE Zone |
|-------------|-------------|----------|
| Position broadcast | Per-update (~100ms) | Batched every 200ms |
| Damage broadcast | Per-hit | Aggregated over 100ms windows |
| Combat tick | 50ms | 100ms (WoE-specific) |
| Emperium damage | N/A | Queued, processed every 100ms |
| Interest radius | Unlimited (broadcastToZone) | 2500 UE units for detail, castle-wide for events |
| Castle state persist | N/A | Redis every 10s, PostgreSQL at WoE end |
| Guardian AI tick | N/A (uses enemy AI loop) | Separate 200ms loop, simplified aggro |

---

## 7. DB Schema

### 7.1 Full Migration Script

```sql
-- database/migrations/add_pvp_woe_system.sql
-- Run after guild system tables are created (see 08_PvP_Guild_WoE.md Section 8.1)

-- ============================================================
-- WoE Castles — 20 WoE 1 castles + 10 WoE 2 castles
-- ============================================================
CREATE TABLE IF NOT EXISTS woe_castles (
    id                      SERIAL PRIMARY KEY,
    castle_name             VARCHAR(32) NOT NULL UNIQUE,
    realm                   VARCHAR(24) NOT NULL,
    map_name                VARCHAR(32) NOT NULL,
    woe_edition             INTEGER NOT NULL DEFAULT 1,          -- 1 = WoE 1, 2 = WoE SE
    owner_guild_id          INTEGER REFERENCES guilds(id) ON DELETE SET NULL,
    economy_level           INTEGER NOT NULL DEFAULT 0,
    defense_level           INTEGER NOT NULL DEFAULT 0,
    economy_invest_today    INTEGER NOT NULL DEFAULT 0,          -- resets daily, max 2
    defense_invest_today    INTEGER NOT NULL DEFAULT 0,          -- resets daily, max 2
    last_invest_reset       DATE NOT NULL DEFAULT CURRENT_DATE,
    emperium_hp             INTEGER NOT NULL DEFAULT 68430,
    guardian_stone_1_alive  BOOLEAN NOT NULL DEFAULT TRUE,       -- WoE 2 only
    guardian_stone_2_alive  BOOLEAN NOT NULL DEFAULT TRUE,       -- WoE 2 only
    barricade_1_installed   BOOLEAN NOT NULL DEFAULT FALSE,      -- WoE 2 only
    barricade_2_installed   BOOLEAN NOT NULL DEFAULT FALSE,      -- WoE 2 only
    barricade_3_installed   BOOLEAN NOT NULL DEFAULT FALSE,      -- WoE 2 only
    captured_at             TIMESTAMPTZ,
    created_at              TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_woe_castles_owner ON woe_castles(owner_guild_id);
CREATE INDEX idx_woe_castles_realm ON woe_castles(realm);

-- ============================================================
-- Castle Economy — Investment history and treasure log
-- ============================================================
CREATE TABLE IF NOT EXISTS castle_economy (
    id                  SERIAL PRIMARY KEY,
    castle_id           INTEGER NOT NULL REFERENCES woe_castles(id) ON DELETE CASCADE,
    guild_id            INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    invest_type         VARCHAR(10) NOT NULL,           -- 'economy' | 'defense'
    amount              INTEGER NOT NULL DEFAULT 1,
    cost_zeny           INTEGER NOT NULL DEFAULT 0,
    bonus_point         BOOLEAN NOT NULL DEFAULT FALSE,  -- Absolute Develop free point
    invested_at         TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_castle_economy_castle ON castle_economy(castle_id);
CREATE INDEX idx_castle_economy_guild ON castle_economy(guild_id);

-- ============================================================
-- Treasure Log — Daily treasure box generation records
-- ============================================================
CREATE TABLE IF NOT EXISTS woe_treasure_log (
    id              SERIAL PRIMARY KEY,
    castle_id       INTEGER NOT NULL REFERENCES woe_castles(id) ON DELETE CASCADE,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    box_count       INTEGER NOT NULL DEFAULT 4,
    items_json      JSONB NOT NULL DEFAULT '[]',
    collected       BOOLEAN NOT NULL DEFAULT FALSE,
    collected_by    INTEGER REFERENCES characters(id),
    generated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    collected_at    TIMESTAMPTZ
);

CREATE INDEX idx_woe_treasure_castle ON woe_treasure_log(castle_id);
CREATE INDEX idx_woe_treasure_guild ON woe_treasure_log(guild_id);

-- ============================================================
-- PvP Stats — Lifetime kill/death tracking per character
-- ============================================================
CREATE TABLE IF NOT EXISTS pvp_stats (
    id              SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    kills           INTEGER NOT NULL DEFAULT 0,
    deaths          INTEGER NOT NULL DEFAULT 0,
    highest_streak  INTEGER NOT NULL DEFAULT 0,
    total_points    INTEGER NOT NULL DEFAULT 0,          -- Cumulative session points earned
    last_pvp_at     TIMESTAMPTZ,
    UNIQUE(character_id)
);

CREATE INDEX idx_pvp_stats_kills ON pvp_stats(kills DESC);
CREATE INDEX idx_pvp_stats_character ON pvp_stats(character_id);

-- ============================================================
-- Castle Ownership History — Track every ownership change
-- ============================================================
CREATE TABLE IF NOT EXISTS castle_ownership_history (
    id              SERIAL PRIMARY KEY,
    castle_id       INTEGER NOT NULL REFERENCES woe_castles(id) ON DELETE CASCADE,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    acquired_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    lost_at         TIMESTAMPTZ                          -- NULL = currently owns
);

CREATE INDEX idx_ownership_history_castle ON castle_ownership_history(castle_id);
CREATE INDEX idx_ownership_history_guild ON castle_ownership_history(guild_id);
CREATE INDEX idx_ownership_history_current ON castle_ownership_history(castle_id)
    WHERE lost_at IS NULL;

-- ============================================================
-- Seed WoE 1 Castles (4 realms x 5 castles = 20)
-- ============================================================
INSERT INTO woe_castles (castle_name, realm, map_name, woe_edition) VALUES
    -- Valkyrie Realm (Prontera)
    ('Kriemhild',    'valkyrie',  'prtg_cas01', 1),
    ('Swanhild',     'valkyrie',  'prtg_cas02', 1),
    ('Fadhgrindh',   'valkyrie',  'prtg_cas03', 1),
    ('Skoegul',      'valkyrie',  'prtg_cas04', 1),
    ('Gondul',       'valkyrie',  'prtg_cas05', 1),
    -- Balder Realm (Payon)
    ('Bright Arbor',       'balder', 'payg_cas01', 1),
    ('Scarlet Palace',     'balder', 'payg_cas02', 1),
    ('Holy Shadow',        'balder', 'payg_cas03', 1),
    ('Sacred Altar',       'balder', 'payg_cas04', 1),
    ('Bamboo Grove Hill',  'balder', 'payg_cas05', 1),
    -- Britoniah Realm (Geffen)
    ('Repherion',     'britoniah', 'gefg_cas01', 1),
    ('Eeyorbriggar',  'britoniah', 'gefg_cas02', 1),
    ('Yesnelph',      'britoniah', 'gefg_cas03', 1),
    ('Bergel',        'britoniah', 'gefg_cas04', 1),
    ('Mersetzdeitz',  'britoniah', 'gefg_cas05', 1),
    -- Luina Realm (Al De Baran)
    ('Neuschwanstein',  'luina', 'aldeg_cas01', 1),
    ('Hohenschwangau',  'luina', 'aldeg_cas02', 1),
    ('Nuernberg',       'luina', 'aldeg_cas03', 1),
    ('Wuerzberg',       'luina', 'aldeg_cas04', 1),
    ('Rothenburg',      'luina', 'aldeg_cas05', 1)
ON CONFLICT (castle_name) DO NOTHING;

-- ============================================================
-- Seed WoE 2 / SE Castles (2 realms x 5 castles = 10)
-- ============================================================
INSERT INTO woe_castles (castle_name, realm, map_name, woe_edition) VALUES
    -- Nidhoggur Realm (Juno)
    ('Himinn',       'nidhoggur', 'schg_cas01', 2),
    ('Andlangr',     'nidhoggur', 'schg_cas02', 2),
    ('Viblainn',     'nidhoggur', 'schg_cas03', 2),
    ('Hljod',        'nidhoggur', 'schg_cas04', 2),
    ('Skidbladnir',  'nidhoggur', 'schg_cas05', 2),
    -- Valfreyja Realm (Rachel)
    ('Mardol',  'valfreyja', 'arug_cas01', 2),
    ('Cyr',     'valfreyja', 'arug_cas02', 2),
    ('Horn',    'valfreyja', 'arug_cas03', 2),
    ('Gefn',    'valfreyja', 'arug_cas04', 2),
    ('Syr',     'valfreyja', 'arug_cas05', 2)
ON CONFLICT (castle_name) DO NOTHING;

-- ============================================================
-- Daily investment reset function (call via cron or server startup)
-- ============================================================
CREATE OR REPLACE FUNCTION reset_daily_investments()
RETURNS void AS $$
BEGIN
    UPDATE woe_castles
    SET economy_invest_today = 0,
        defense_invest_today = 0,
        last_invest_reset = CURRENT_DATE
    WHERE last_invest_reset < CURRENT_DATE;
END;
$$ LANGUAGE plpgsql;

-- ============================================================
-- Convenience views
-- ============================================================
CREATE OR REPLACE VIEW v_castle_ownership AS
SELECT
    wc.id AS castle_id,
    wc.castle_name,
    wc.realm,
    wc.woe_edition,
    g.id AS guild_id,
    g.name AS guild_name,
    wc.economy_level,
    wc.defense_level,
    wc.captured_at
FROM woe_castles wc
LEFT JOIN guilds g ON g.id = wc.owner_guild_id;

CREATE OR REPLACE VIEW v_pvp_leaderboard AS
SELECT
    c.id AS character_id,
    c.name AS character_name,
    c.job_class,
    c.base_level,
    ps.kills,
    ps.deaths,
    ps.highest_streak,
    CASE WHEN ps.deaths > 0
         THEN ROUND(ps.kills::numeric / ps.deaths, 2)
         ELSE ps.kills END AS kd_ratio
FROM pvp_stats ps
JOIN characters c ON c.id = ps.character_id
WHERE c.deleted = FALSE
ORDER BY ps.kills DESC;
```

### 7.2 Schema Diagram

```
woe_castles
    id (PK)
    castle_name (UNIQUE)
    realm
    map_name
    woe_edition (1 or 2)
    owner_guild_id (FK -> guilds.id, SET NULL on delete)
    economy_level
    defense_level
    economy_invest_today
    defense_invest_today
    last_invest_reset
    emperium_hp
    guardian_stone_1_alive (WoE 2)
    guardian_stone_2_alive (WoE 2)
    barricade_1/2/3_installed (WoE 2)
    captured_at
        |
        +--- castle_economy (investment history)
        |       castle_id (FK)
        |       guild_id (FK)
        |       invest_type
        |       amount, cost_zeny, bonus_point
        |
        +--- woe_treasure_log (daily treasure generation)
        |       castle_id (FK)
        |       guild_id (FK)
        |       box_count, items_json
        |       collected, collected_by, collected_at
        |
        +--- castle_ownership_history (ownership changes)
                castle_id (FK)
                guild_id (FK)
                acquired_at, lost_at

pvp_stats
    character_id (FK -> characters.id, UNIQUE)
    kills, deaths, highest_streak
    total_points, last_pvp_at
```

### 7.3 Key Queries

```sql
-- Get current castle ownership for all castles
SELECT * FROM v_castle_ownership;

-- Get PvP leaderboard (top 50)
SELECT * FROM v_pvp_leaderboard LIMIT 50;

-- Get a guild's castle count
SELECT COUNT(*) FROM woe_castles WHERE owner_guild_id = $1;

-- Get uncollected treasure for a guild master
SELECT * FROM woe_treasure_log
WHERE guild_id = $1 AND collected = FALSE
ORDER BY generated_at DESC;

-- Get ownership history for a castle
SELECT coh.*, g.name AS guild_name
FROM castle_ownership_history coh
JOIN guilds g ON g.id = coh.guild_id
WHERE coh.castle_id = $1
ORDER BY coh.acquired_at DESC;

-- Reset daily investments (called at midnight or server startup)
SELECT reset_daily_investments();
```

---

## Appendix: Socket Events Summary (All PvP + WoE)

```
// ===== PvP Events =====

// Client -> Server
pvp:enter_room          { roomId }
pvp:leave               {}
pvp:duel_request        { targetCharId }
pvp:duel_accept         { requestId }
pvp:duel_reject         { requestId }
pvp:duel_leave          {}

// Server -> Client
pvp:entered             { roomId, points, rankings[] }
pvp:kill                { killerId, killerName, victimId, victimName,
                          killerPoints, victimPoints }
pvp:death               { killerId, newPoints }
pvp:kicked              { reason }
pvp:rankings_update     { rankings[] }
pvp:duel_request_received { requestId, challengerName }
pvp:duel_started        { opponentId, opponentName }
pvp:duel_ended          { winnerId, reason }
pvp:error               { message }
pvp:invulnerable_end    {}

// ===== WoE Events =====

// Server -> All Clients
woe:announcement        { message, woeStartTime?, woeEndTime? }
woe:start               { message, endTime }
woe:end                 { message, results[] }
woe:castle_captured     { castleId, castleName, guildId, guildName,
                          breakerName, previousGuildId }

// Server -> Castle Zone Clients
woe:emperium_hp         { castleId, hp, maxHp, attackerId?, damage? }
woe:emperium_attack     { castleId, attackerId, damage, isMiss, reason? }
woe:guardian_spawned    { castleId, guardianId, type, name, hp, maxHp,
                          x, y, z }
woe:guardian_despawned  { castleId, guardianId }
woe:guardian_killed     { castleId, guardianId, killerId }
woe:guardian_stone_hp   { castleId, stoneIndex, hp, maxHp }
woe:guardian_stone_destroyed { castleId, stoneIndex, attackerId, attackerName }
woe:guardian_stone_repaired  { castleId, stoneIndex }
woe:barrier_opened      { castleId }
woe:barricade_hp        { castleId, barricadeIndex, blockIndex, hp, maxHp }
woe:barricade_destroyed { castleId, barricadeIndex }

// Client -> Server
woe:attack_emperium     { castleId }
woe:enter_castle        { castleId }
woe:open_treasure       { castleId }
woe:hire_guardian       { castleId, guardianSlot }
woe:repair_guardian     { castleId, guardianSlot }
woe:repair_stone        { castleId, stoneIndex }
woe:install_barricade   { castleId, barricadeIndex }

// ===== WoE-zone combat (reuses existing events with WoE modifiers) =====
// combat:damage          -- WoE damage reduction applied server-side
// skill:effect_damage    -- WoE skill damage reduction applied server-side
// player:batch_moved     -- Batched position updates (WoE optimization)
// combat:damage_batch    -- Aggregated damage numbers (WoE optimization)

// ===== Guild Investment =====
guild:invest            { castleId, type }          // Client -> Server
guild:invest_result     { castleId, type, newLevel, cost, bonusPoint }  // Server -> Client
```
