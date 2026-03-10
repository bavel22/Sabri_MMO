# 10 -- Party, Guild, Chat, Trade & Social Systems: UE5 C++ Implementation Guide

> **Scope**: Complete server + client implementation for all social/multiplayer systems in Sabri_MMO.
> **Stack**: UE5.7 (C++ Slate) | Node.js + Socket.io | PostgreSQL
> **Pattern**: UWorldSubsystem + Slate widget per system, server-authoritative, event-driven (no Tick polling).

---

## Table of Contents

1. [Database Schema (All Social Tables)](#1-database-schema)
2. [Socket.io Event Protocol](#2-socketio-event-protocol)
3. [Party System -- Server](#3-party-system-server)
4. [Party System -- Client](#4-party-system-client)
5. [Guild System -- Server](#5-guild-system-server)
6. [Guild System -- Client](#6-guild-system-client)
7. [Chat System](#7-chat-system)
8. [Friend List](#8-friend-list)
9. [Trade System](#9-trade-system)
10. [Player Interaction Menu](#10-player-interaction-menu)

---

## 1. Database Schema

All tables go in a single migration file: `database/migrations/add_social_systems.sql`.

```sql
-- =============================================================
-- PARTY SYSTEM
-- =============================================================

CREATE TABLE IF NOT EXISTS parties (
    party_id SERIAL PRIMARY KEY,
    party_name VARCHAR(24) NOT NULL,
    leader_id INTEGER NOT NULL REFERENCES characters(character_id),
    exp_share_mode VARCHAR(10) NOT NULL DEFAULT 'each_take',  -- 'each_take' | 'even_share'
    item_share_mode VARCHAR(15) NOT NULL DEFAULT 'each_take', -- 'each_take' | 'party_share' | 'individual' | 'shared'
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS party_members (
    party_id INTEGER NOT NULL REFERENCES parties(party_id) ON DELETE CASCADE,
    character_id INTEGER NOT NULL REFERENCES characters(character_id),
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (party_id, character_id)
);

CREATE INDEX IF NOT EXISTS idx_party_members_char ON party_members(character_id);
CREATE INDEX IF NOT EXISTS idx_parties_leader ON parties(leader_id);

-- =============================================================
-- GUILD SYSTEM
-- =============================================================

CREATE TABLE IF NOT EXISTS guilds (
    guild_id SERIAL PRIMARY KEY,
    guild_name VARCHAR(24) NOT NULL UNIQUE,
    master_id INTEGER NOT NULL REFERENCES characters(character_id),
    guild_level INTEGER NOT NULL DEFAULT 1,
    guild_exp BIGINT NOT NULL DEFAULT 0,
    guild_exp_next BIGINT NOT NULL DEFAULT 10000,
    max_members INTEGER NOT NULL DEFAULT 16,
    skill_points INTEGER NOT NULL DEFAULT 0,
    notice_title VARCHAR(60) DEFAULT '',
    notice_body VARCHAR(240) DEFAULT '',
    emblem_data TEXT DEFAULT NULL,  -- base64-encoded 24x24 BMP
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS guild_positions (
    guild_id INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    position_id INTEGER NOT NULL CHECK (position_id >= 0 AND position_id <= 19),
    position_name VARCHAR(24) NOT NULL DEFAULT 'Member',
    can_invite BOOLEAN NOT NULL DEFAULT FALSE,
    can_kick BOOLEAN NOT NULL DEFAULT FALSE,
    can_storage BOOLEAN NOT NULL DEFAULT FALSE,
    tax_rate INTEGER NOT NULL DEFAULT 0 CHECK (tax_rate >= 0 AND tax_rate <= 50),
    PRIMARY KEY (guild_id, position_id)
);

CREATE TABLE IF NOT EXISTS guild_members (
    guild_id INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    character_id INTEGER NOT NULL REFERENCES characters(character_id),
    position_id INTEGER NOT NULL DEFAULT 19,  -- lowest rank by default
    contributed_exp BIGINT NOT NULL DEFAULT 0,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (guild_id, character_id)
);

CREATE INDEX IF NOT EXISTS idx_guild_members_char ON guild_members(character_id);

CREATE TABLE IF NOT EXISTS guild_skills (
    guild_id INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    skill_id VARCHAR(40) NOT NULL,  -- 'guild_extension', 'battle_orders', etc.
    skill_level INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (guild_id, skill_id)
);

CREATE TABLE IF NOT EXISTS guild_storage (
    storage_id SERIAL PRIMARY KEY,
    guild_id INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    item_id INTEGER NOT NULL REFERENCES items(item_id),
    quantity INTEGER NOT NULL DEFAULT 1,
    slot_index INTEGER NOT NULL DEFAULT -1,
    deposited_by INTEGER REFERENCES characters(character_id),
    deposited_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_guild_storage_guild ON guild_storage(guild_id);

CREATE TABLE IF NOT EXISTS guild_alliances (
    guild_id_1 INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    guild_id_2 INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    alliance_type VARCHAR(10) NOT NULL DEFAULT 'ally',  -- 'ally' | 'enemy'
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (guild_id_1, guild_id_2)
);

-- =============================================================
-- FRIEND LIST
-- =============================================================

CREATE TABLE IF NOT EXISTS friends (
    character_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    friend_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (character_id, friend_id)
);

-- =============================================================
-- BLOCK LIST
-- =============================================================

CREATE TABLE IF NOT EXISTS blocked_players (
    character_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    blocked_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (character_id, blocked_id)
);

-- =============================================================
-- TRADE LOG (audit trail, optional)
-- =============================================================

CREATE TABLE IF NOT EXISTS trade_log (
    trade_id SERIAL PRIMARY KEY,
    player_a_id INTEGER NOT NULL REFERENCES characters(character_id),
    player_b_id INTEGER NOT NULL REFERENCES characters(character_id),
    items_a JSONB NOT NULL DEFAULT '[]',  -- [{itemId, inventoryId, quantity}]
    items_b JSONB NOT NULL DEFAULT '[]',
    zeny_a INTEGER NOT NULL DEFAULT 0,
    zeny_b INTEGER NOT NULL DEFAULT 0,
    completed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =============================================================
-- Seed default guild positions (inserted per guild on creation)
-- =============================================================
-- Position 0 = Guild Master (created dynamically)
-- Positions 1-19 = configurable ranks
```

---

## 2. Socket.io Event Protocol

All events follow the existing project pattern: client emits `domain:action`, server responds with `domain:result` or broadcasts `domain:update`.

### 2.1 Party Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C->S | `party:create` | `{ partyName, itemShareMode }` |
| S->C | `party:created` | `{ partyId, partyName, leaderId, leaderName, expShareMode, itemShareMode, members[] }` |
| C->S | `party:invite` | `{ targetName }` |
| S->Target | `party:invite_received` | `{ partyId, partyName, inviterName, inviterId }` |
| C->S | `party:invite_respond` | `{ partyId, accept }` |
| S->Party | `party:member_joined` | `{ characterId, characterName, level, jobClass, hp, maxHp, sp, maxSp, zone, isOnline }` |
| C->S | `party:kick` | `{ targetId }` |
| S->Party | `party:member_left` | `{ characterId, characterName, reason }` (reason: 'kicked' or 'left') |
| C->S | `party:leave` | `{}` |
| C->S | `party:change_leader` | `{ targetId }` |
| S->Party | `party:leader_changed` | `{ newLeaderId, newLeaderName }` |
| C->S | `party:set_exp_mode` | `{ mode }` ('each_take' or 'even_share') |
| S->Party | `party:settings_changed` | `{ expShareMode, itemShareMode }` |
| S->Party | `party:member_update` | `{ characterId, hp, maxHp, sp, maxSp, zone, isOnline, level, jobClass }` |
| S->C | `party:data` | Full party state (sent on login if in party) |
| S->C | `party:error` | `{ message }` |
| S->C | `party:disbanded` | `{ partyId }` |

### 2.2 Guild Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C->S | `guild:create` | `{ guildName }` (requires Emperium in inventory) |
| S->C | `guild:created` | Full guild data object |
| C->S | `guild:invite` | `{ targetName }` |
| S->Target | `guild:invite_received` | `{ guildId, guildName, inviterName }` |
| C->S | `guild:invite_respond` | `{ guildId, accept }` |
| S->Guild | `guild:member_joined` | `{ characterId, characterName, level, jobClass, positionId, isOnline }` |
| C->S | `guild:kick` | `{ targetId }` |
| S->Guild | `guild:member_left` | `{ characterId, characterName, reason }` |
| C->S | `guild:leave` | `{}` |
| C->S | `guild:set_position` | `{ targetId, positionId }` |
| C->S | `guild:update_position` | `{ positionId, positionName, canInvite, canKick, canStorage, taxRate }` |
| C->S | `guild:set_notice` | `{ title, body }` |
| S->Guild | `guild:notice_updated` | `{ title, body }` |
| C->S | `guild:learn_skill` | `{ skillId }` |
| S->Guild | `guild:skill_updated` | `{ skillId, newLevel, remainingPoints }` |
| C->S | `guild:use_skill` | `{ skillId }` |
| S->Guild | `guild:skill_effect` | `{ skillId, casterId }` |
| C->S | `guild:deposit_storage` | `{ inventoryId, quantity }` |
| C->S | `guild:withdraw_storage` | `{ storageId, quantity }` |
| S->C | `guild:storage_data` | `{ items[] }` |
| S->C | `guild:data` | Full guild state (sent on login if in guild) |
| S->Guild | `guild:member_update` | `{ characterId, isOnline, level, jobClass, zone }` |
| S->Guild | `guild:level_up` | `{ guildLevel, guildExp, guildExpNext, skillPoints }` |
| C->S | `guild:change_master` | `{ targetId }` |
| C->S | `guild:disband` | `{}` |
| S->C | `guild:disbanded` | `{ guildId }` |
| C->S | `guild:set_emblem` | `{ emblemData }` (base64) |
| S->Guild | `guild:emblem_updated` | `{ emblemData }` |
| S->C | `guild:error` | `{ message }` |

### 2.3 Chat Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C->S | `chat:message` | `{ channel, message, targetName? }` |
| S->C | `chat:receive` | `{ channel, senderId, senderName, message, timestamp }` |
| S->C | `chat:error` | `{ message }` |

Channels: `PUBLIC`, `WHISPER`, `PARTY`, `GUILD`, `GLOBAL`, `SYSTEM`, `COMBAT`, `TRADE`.

### 2.4 Friend Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C->S | `friend:add` | `{ targetName }` |
| S->Target | `friend:request` | `{ fromId, fromName }` |
| C->S | `friend:respond` | `{ fromId, accept }` |
| S->C | `friend:added` | `{ friendId, friendName, isOnline }` |
| C->S | `friend:remove` | `{ friendId }` |
| S->C | `friend:removed` | `{ friendId }` |
| S->C | `friend:status` | `{ friendId, isOnline }` |
| S->C | `friend:list` | `{ friends[] }` (sent on login) |

### 2.5 Trade Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C->S | `trade:request` | `{ targetId }` |
| S->Target | `trade:request_received` | `{ fromId, fromName }` |
| C->S | `trade:respond` | `{ fromId, accept }` |
| S->Both | `trade:started` | `{ partnerId, partnerName }` |
| C->S | `trade:add_item` | `{ inventoryId, quantity }` |
| S->Both | `trade:item_added` | `{ side, slot, inventoryId, itemId, itemName, icon, quantity }` |
| C->S | `trade:remove_item` | `{ slot }` |
| S->Both | `trade:item_removed` | `{ side, slot }` |
| C->S | `trade:set_zeny` | `{ amount }` |
| S->Both | `trade:zeny_set` | `{ side, amount }` |
| C->S | `trade:lock` | `{}` |
| S->Both | `trade:locked` | `{ side }` |
| C->S | `trade:confirm` | `{}` |
| S->Both | `trade:completed` | `{ success }` |
| C->S | `trade:cancel` | `{}` |
| S->Both | `trade:cancelled` | `{ reason }` |
| S->C | `trade:error` | `{ message }` |

### 2.6 Block Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C->S | `block:add` | `{ targetName }` |
| S->C | `block:added` | `{ blockedId, blockedName }` |
| C->S | `block:remove` | `{ blockedId }` |
| S->C | `block:removed` | `{ blockedId }` |
| S->C | `block:list` | `{ blocked[] }` (sent on login) |

---

## 3. Party System -- Server

Add to `server/src/index.js`. All party state lives in memory with DB persistence on mutation.

### 3.1 In-Memory Data Structures

```javascript
// ============================================================
// PARTY SYSTEM — In-memory state
// ============================================================

const MAX_PARTY_SIZE = 12;
const EVEN_SHARE_LEVEL_RANGE = 15;

// partyId -> party object
const parties = new Map();

// characterId -> partyId (reverse lookup)
const playerPartyMap = new Map();

// characterId -> { partyId, inviterName, timestamp } (pending invites, expire in 30s)
const pendingPartyInvites = new Map();

let nextPartyId = 1; // incremented on create, synced from DB on startup

// Load existing parties from DB on server start
async function loadPartiesFromDB() {
    try {
        const partyRows = await pool.query('SELECT * FROM parties');
        for (const row of partyRows.rows) {
            const memberRows = await pool.query(
                'SELECT pm.character_id, c.name, c.level, c.class FROM party_members pm JOIN characters c ON c.character_id = pm.character_id WHERE pm.party_id = $1',
                [row.party_id]
            );
            const members = {};
            for (const m of memberRows.rows) {
                members[m.character_id] = {
                    characterId: m.character_id,
                    characterName: m.name,
                    level: m.level,
                    jobClass: m.class,
                    isOnline: connectedPlayers.has(m.character_id),
                    hp: 0, maxHp: 100, sp: 0, maxSp: 100, zone: ''
                };
                playerPartyMap.set(m.character_id, row.party_id);
            }
            parties.set(row.party_id, {
                partyId: row.party_id,
                partyName: row.party_name,
                leaderId: row.leader_id,
                expShareMode: row.exp_share_mode,
                itemShareMode: row.item_share_mode,
                members: members
            });
            if (row.party_id >= nextPartyId) nextPartyId = row.party_id + 1;
        }
        logger.info(`[PARTY] Loaded ${parties.size} parties from DB`);
    } catch (err) {
        logger.error(`[PARTY] Failed to load parties: ${err.message}`);
    }
}
```

### 3.2 Helper: Emit to All Party Members

```javascript
function emitToParty(partyId, event, payload) {
    const party = parties.get(partyId);
    if (!party) return;
    for (const charIdStr of Object.keys(party.members)) {
        const charId = parseInt(charIdStr);
        const player = connectedPlayers.get(charId);
        if (player && player.socketId) {
            const targetSocket = io.sockets.sockets.get(player.socketId);
            if (targetSocket) targetSocket.emit(event, payload);
        }
    }
}

function getPartyMemberList(party) {
    return Object.values(party.members).map(m => {
        const player = connectedPlayers.get(m.characterId);
        return {
            characterId: m.characterId,
            characterName: m.characterName,
            level: player ? player.baseLevel : m.level,
            jobClass: player ? player.jobClass : m.jobClass,
            hp: player ? player.health : 0,
            maxHp: player ? player.maxHealth : 100,
            sp: player ? player.mana : 0,
            maxSp: player ? player.maxMana : 100,
            zone: player ? (player.zone || '') : '',
            isOnline: !!player,
            isLeader: m.characterId === party.leaderId
        };
    });
}
```

### 3.3 Socket Event Handlers

```javascript
// Inside io.on('connection', (socket) => { ... })

// ---- PARTY: CREATE ----
socket.on('party:create', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('party:error', { message: 'Not logged in.' });
    const player = connectedPlayers.get(characterId);
    if (!player) return socket.emit('party:error', { message: 'Player not found.' });

    if (playerPartyMap.has(characterId)) {
        return socket.emit('party:error', { message: 'You are already in a party.' });
    }

    const partyName = (data.partyName || '').trim().substring(0, 24);
    if (partyName.length < 1) {
        return socket.emit('party:error', { message: 'Party name is required.' });
    }

    const itemShareMode = ['each_take', 'party_share', 'individual', 'shared']
        .includes(data.itemShareMode) ? data.itemShareMode : 'each_take';

    try {
        const result = await pool.query(
            'INSERT INTO parties (party_name, leader_id, item_share_mode) VALUES ($1, $2, $3) RETURNING party_id',
            [partyName, characterId, itemShareMode]
        );
        const partyId = result.rows[0].party_id;

        await pool.query(
            'INSERT INTO party_members (party_id, character_id) VALUES ($1, $2)',
            [partyId, characterId]
        );

        const party = {
            partyId,
            partyName,
            leaderId: characterId,
            expShareMode: 'each_take',
            itemShareMode,
            members: {
                [characterId]: {
                    characterId,
                    characterName: player.characterName,
                    level: player.baseLevel,
                    jobClass: player.jobClass,
                    isOnline: true,
                    hp: player.health, maxHp: player.maxHealth,
                    sp: player.mana, maxSp: player.maxMana,
                    zone: player.zone || ''
                }
            }
        };

        parties.set(partyId, party);
        playerPartyMap.set(characterId, partyId);
        if (partyId >= nextPartyId) nextPartyId = partyId + 1;

        socket.emit('party:created', {
            partyId, partyName,
            leaderId: characterId,
            leaderName: player.characterName,
            expShareMode: 'each_take',
            itemShareMode,
            members: getPartyMemberList(party)
        });

        logger.info(`[PARTY] ${player.characterName} created party "${partyName}" (ID: ${partyId})`);
    } catch (err) {
        logger.error(`[PARTY] Create failed: ${err.message}`);
        socket.emit('party:error', { message: 'Failed to create party.' });
    }
});

// ---- PARTY: INVITE ----
socket.on('party:invite', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('party:error', { message: 'Not logged in.' });

    const partyId = playerPartyMap.get(characterId);
    if (!partyId) return socket.emit('party:error', { message: 'You are not in a party.' });

    const party = parties.get(partyId);
    if (!party) return socket.emit('party:error', { message: 'Party not found.' });

    if (party.leaderId !== characterId) {
        return socket.emit('party:error', { message: 'Only the party leader can invite.' });
    }

    if (Object.keys(party.members).length >= MAX_PARTY_SIZE) {
        return socket.emit('party:error', { message: 'Party is full (max 12).' });
    }

    const targetName = (data.targetName || '').trim();
    let targetCharId = null;
    let targetPlayer = null;
    for (const [cid, p] of connectedPlayers.entries()) {
        if (p.characterName && p.characterName.toLowerCase() === targetName.toLowerCase()) {
            targetCharId = cid;
            targetPlayer = p;
            break;
        }
    }

    if (!targetCharId || !targetPlayer) {
        return socket.emit('party:error', { message: `Player "${targetName}" is not online.` });
    }
    if (playerPartyMap.has(targetCharId)) {
        return socket.emit('party:error', { message: `${targetPlayer.characterName} is already in a party.` });
    }

    // Store pending invite (expires in 30s)
    pendingPartyInvites.set(targetCharId, {
        partyId, inviterName: connectedPlayers.get(characterId).characterName, timestamp: Date.now()
    });
    setTimeout(() => {
        const inv = pendingPartyInvites.get(targetCharId);
        if (inv && inv.partyId === partyId) pendingPartyInvites.delete(targetCharId);
    }, 30000);

    const targetSocket = io.sockets.sockets.get(targetPlayer.socketId);
    if (targetSocket) {
        targetSocket.emit('party:invite_received', {
            partyId, partyName: party.partyName,
            inviterName: connectedPlayers.get(characterId).characterName,
            inviterId: characterId
        });
    }

    logger.info(`[PARTY] ${connectedPlayers.get(characterId).characterName} invited ${targetPlayer.characterName} to "${party.partyName}"`);
});

// ---- PARTY: INVITE RESPOND ----
socket.on('party:invite_respond', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('party:error', { message: 'Not logged in.' });

    const invite = pendingPartyInvites.get(characterId);
    if (!invite || invite.partyId !== data.partyId) {
        return socket.emit('party:error', { message: 'No pending invite found.' });
    }
    pendingPartyInvites.delete(characterId);

    if (!data.accept) return; // silently decline

    if (playerPartyMap.has(characterId)) {
        return socket.emit('party:error', { message: 'You are already in a party.' });
    }

    const party = parties.get(invite.partyId);
    if (!party) return socket.emit('party:error', { message: 'Party no longer exists.' });

    if (Object.keys(party.members).length >= MAX_PARTY_SIZE) {
        return socket.emit('party:error', { message: 'Party is full.' });
    }

    const player = connectedPlayers.get(characterId);
    if (!player) return;

    try {
        await pool.query(
            'INSERT INTO party_members (party_id, character_id) VALUES ($1, $2)',
            [invite.partyId, characterId]
        );

        party.members[characterId] = {
            characterId,
            characterName: player.characterName,
            level: player.baseLevel,
            jobClass: player.jobClass,
            isOnline: true,
            hp: player.health, maxHp: player.maxHealth,
            sp: player.mana, maxSp: player.maxMana,
            zone: player.zone || ''
        };
        playerPartyMap.set(characterId, invite.partyId);

        // Send full party data to the joiner
        socket.emit('party:data', {
            partyId: party.partyId,
            partyName: party.partyName,
            leaderId: party.leaderId,
            expShareMode: party.expShareMode,
            itemShareMode: party.itemShareMode,
            members: getPartyMemberList(party)
        });

        // Notify all other members
        emitToParty(invite.partyId, 'party:member_joined', {
            characterId, characterName: player.characterName,
            level: player.baseLevel, jobClass: player.jobClass,
            hp: player.health, maxHp: player.maxHealth,
            sp: player.mana, maxSp: player.maxMana,
            zone: player.zone || '', isOnline: true
        });

        logger.info(`[PARTY] ${player.characterName} joined "${party.partyName}"`);
    } catch (err) {
        logger.error(`[PARTY] Join failed: ${err.message}`);
        socket.emit('party:error', { message: 'Failed to join party.' });
    }
});

// ---- PARTY: LEAVE ----
socket.on('party:leave', async () => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;

    const partyId = playerPartyMap.get(characterId);
    if (!partyId) return socket.emit('party:error', { message: 'Not in a party.' });

    await removeFromParty(characterId, partyId, 'left');
});

// ---- PARTY: KICK ----
socket.on('party:kick', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;

    const partyId = playerPartyMap.get(characterId);
    if (!partyId) return socket.emit('party:error', { message: 'Not in a party.' });

    const party = parties.get(partyId);
    if (!party || party.leaderId !== characterId) {
        return socket.emit('party:error', { message: 'Only the leader can kick members.' });
    }

    const targetId = parseInt(data.targetId);
    if (targetId === characterId) {
        return socket.emit('party:error', { message: 'Cannot kick yourself. Use leave instead.' });
    }
    if (!party.members[targetId]) {
        return socket.emit('party:error', { message: 'Target is not in your party.' });
    }

    await removeFromParty(targetId, partyId, 'kicked');
});

// ---- PARTY: CHANGE LEADER ----
socket.on('party:change_leader', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;

    const partyId = playerPartyMap.get(characterId);
    if (!partyId) return socket.emit('party:error', { message: 'Not in a party.' });

    const party = parties.get(partyId);
    if (!party || party.leaderId !== characterId) {
        return socket.emit('party:error', { message: 'Only the leader can transfer leadership.' });
    }

    const targetId = parseInt(data.targetId);
    if (!party.members[targetId]) {
        return socket.emit('party:error', { message: 'Target is not in your party.' });
    }

    party.leaderId = targetId;
    await pool.query('UPDATE parties SET leader_id = $1 WHERE party_id = $2', [targetId, partyId]);

    const newLeaderName = party.members[targetId].characterName;
    emitToParty(partyId, 'party:leader_changed', { newLeaderId: targetId, newLeaderName });
    logger.info(`[PARTY] Leadership of "${party.partyName}" transferred to ${newLeaderName}`);
});

// ---- PARTY: SET EXP MODE ----
socket.on('party:set_exp_mode', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;

    const partyId = playerPartyMap.get(characterId);
    if (!partyId) return socket.emit('party:error', { message: 'Not in a party.' });

    const party = parties.get(partyId);
    if (!party || party.leaderId !== characterId) {
        return socket.emit('party:error', { message: 'Only the leader can change EXP mode.' });
    }

    const mode = data.mode === 'even_share' ? 'even_share' : 'each_take';

    // Even Share validation: all online members must be within 15 levels
    if (mode === 'even_share') {
        const onlineMembers = Object.values(party.members).filter(m => {
            const p = connectedPlayers.get(m.characterId);
            return !!p;
        });
        if (onlineMembers.length > 1) {
            const levels = onlineMembers.map(m => {
                const p = connectedPlayers.get(m.characterId);
                return p ? p.baseLevel : m.level;
            });
            const minLv = Math.min(...levels);
            const maxLv = Math.max(...levels);
            if (maxLv - minLv > EVEN_SHARE_LEVEL_RANGE) {
                return socket.emit('party:error', {
                    message: `Cannot enable Even Share: level gap is ${maxLv - minLv} (max ${EVEN_SHARE_LEVEL_RANGE}).`
                });
            }
        }
    }

    party.expShareMode = mode;
    await pool.query('UPDATE parties SET exp_share_mode = $1 WHERE party_id = $2', [mode, partyId]);

    emitToParty(partyId, 'party:settings_changed', {
        expShareMode: party.expShareMode,
        itemShareMode: party.itemShareMode
    });
    logger.info(`[PARTY] "${party.partyName}" EXP mode changed to ${mode}`);
});
```

### 3.4 Remove from Party Helper

```javascript
async function removeFromParty(characterId, partyId, reason) {
    const party = parties.get(partyId);
    if (!party) return;

    const memberName = party.members[characterId]
        ? party.members[characterId].characterName : 'Unknown';

    delete party.members[characterId];
    playerPartyMap.delete(characterId);

    await pool.query('DELETE FROM party_members WHERE party_id = $1 AND character_id = $2',
        [partyId, characterId]);

    // Notify the removed player
    const removedPlayer = connectedPlayers.get(characterId);
    if (removedPlayer) {
        const removedSocket = io.sockets.sockets.get(removedPlayer.socketId);
        if (removedSocket) {
            removedSocket.emit('party:disbanded', { partyId });
        }
    }

    const remainingCount = Object.keys(party.members).length;

    if (remainingCount === 0) {
        // Disband empty party
        parties.delete(partyId);
        await pool.query('DELETE FROM parties WHERE party_id = $1', [partyId]);
        logger.info(`[PARTY] "${party.partyName}" disbanded (empty)`);
        return;
    }

    // If leader left, auto-transfer
    if (characterId === party.leaderId) {
        const newLeaderId = parseInt(Object.keys(party.members)[0]);
        party.leaderId = newLeaderId;
        await pool.query('UPDATE parties SET leader_id = $1 WHERE party_id = $2', [newLeaderId, partyId]);
        emitToParty(partyId, 'party:leader_changed', {
            newLeaderId,
            newLeaderName: party.members[newLeaderId].characterName
        });
    }

    emitToParty(partyId, 'party:member_left', {
        characterId, characterName: memberName, reason
    });

    logger.info(`[PARTY] ${memberName} ${reason} "${party.partyName}" (${remainingCount} remaining)`);
}
```

### 3.5 Even Share EXP Distribution

Integrate into the existing `givePlayerExp` / enemy death flow:

```javascript
/**
 * RO Classic Even Share EXP formula:
 * 1. Get all party members on the same zone as the kill
 * 2. Filter to online members
 * 3. Bonus = 25% per member who participated in combat
 * 4. Total EXP = MonsterBaseEXP * (1 + 0.25 * (participantCount - 1))
 * 5. Per-member EXP = floor(Total EXP / eligibleCount)
 *
 * Called from the enemy death handler instead of giving EXP to the killer alone.
 */
function distributePartyExp(killerId, monsterBaseExp, monsterJobExp, killerZone) {
    const partyId = playerPartyMap.get(killerId);
    if (!partyId) return null; // not in party, use normal EXP flow

    const party = parties.get(partyId);
    if (!party || party.expShareMode !== 'even_share') return null;

    // Gather eligible members: online + same zone
    const eligible = [];
    for (const charIdStr of Object.keys(party.members)) {
        const charId = parseInt(charIdStr);
        const player = connectedPlayers.get(charId);
        if (player && player.zone === killerZone) {
            eligible.push(charId);
        }
    }

    if (eligible.length <= 1) return null; // solo, use normal flow

    // Party bonus: +25% per additional participating member
    const bonusMultiplier = 1 + 0.25 * (eligible.length - 1);
    const totalBaseExp = Math.floor(monsterBaseExp * bonusMultiplier);
    const totalJobExp = Math.floor(monsterJobExp * bonusMultiplier);
    const perMemberBase = Math.floor(totalBaseExp / eligible.length);
    const perMemberJob = Math.floor(totalJobExp / eligible.length);

    // Return array of { characterId, baseExp, jobExp } for the caller to distribute
    return eligible.map(charId => ({
        characterId: charId,
        baseExp: perMemberBase,
        jobExp: perMemberJob
    }));
}
```

### 3.6 Party Member HP Broadcast (50ms tick integration)

Add to the existing combat tick loop or create a 1-second party update timer:

```javascript
// Party member status updates (1s interval, not every tick)
setInterval(() => {
    for (const [partyId, party] of parties.entries()) {
        for (const charIdStr of Object.keys(party.members)) {
            const charId = parseInt(charIdStr);
            const player = connectedPlayers.get(charId);
            const member = party.members[charId];
            const wasOnline = member.isOnline;
            member.isOnline = !!player;

            if (player) {
                const hpChanged = member.hp !== player.health || member.maxHp !== player.maxHealth;
                const spChanged = member.sp !== player.mana || member.maxSp !== player.maxMana;
                const zoneChanged = member.zone !== (player.zone || '');

                if (hpChanged || spChanged || zoneChanged || wasOnline !== member.isOnline) {
                    member.hp = player.health;
                    member.maxHp = player.maxHealth;
                    member.sp = player.mana;
                    member.maxSp = player.maxMana;
                    member.zone = player.zone || '';
                    member.level = player.baseLevel;
                    member.jobClass = player.jobClass;

                    emitToParty(partyId, 'party:member_update', {
                        characterId: charId,
                        hp: member.hp, maxHp: member.maxHp,
                        sp: member.sp, maxSp: member.maxSp,
                        zone: member.zone, isOnline: true,
                        level: member.level, jobClass: member.jobClass
                    });
                }
            } else if (wasOnline) {
                emitToParty(partyId, 'party:member_update', {
                    characterId: charId,
                    hp: 0, maxHp: member.maxHp,
                    sp: 0, maxSp: member.maxSp,
                    zone: '', isOnline: false,
                    level: member.level, jobClass: member.jobClass
                });
            }
        }
    }
}, 1000);
```

### 3.7 Send Party Data on Login

Add to the `player:join` handler, after the player is added to `connectedPlayers`:

```javascript
// Inside player:join handler, after connectedPlayers.set(...)
const existingPartyId = playerPartyMap.get(characterId);
if (existingPartyId) {
    const party = parties.get(existingPartyId);
    if (party) {
        // Update member status
        if (party.members[characterId]) {
            party.members[characterId].isOnline = true;
            party.members[characterId].hp = player.health;
            party.members[characterId].maxHp = player.maxHealth;
            party.members[characterId].sp = player.mana;
            party.members[characterId].maxSp = player.maxMana;
            party.members[characterId].zone = player.zone || '';
        }
        socket.emit('party:data', {
            partyId: party.partyId,
            partyName: party.partyName,
            leaderId: party.leaderId,
            expShareMode: party.expShareMode,
            itemShareMode: party.itemShareMode,
            members: getPartyMemberList(party)
        });
        // Notify party that member came online
        emitToParty(existingPartyId, 'party:member_update', {
            characterId,
            hp: player.health, maxHp: player.maxHealth,
            sp: player.mana, maxSp: player.maxMana,
            zone: player.zone || '', isOnline: true,
            level: player.baseLevel, jobClass: player.jobClass
        });
    }
}
```

---

## 4. Party System -- Client

### 4.1 Data Structs (add to CharacterData.h)

```cpp
// ============================================================
// Party member — mirrors server party:data payload
// ============================================================

USTRUCT()
struct FPartyMember
{
    GENERATED_BODY()

    int32 CharacterId = 0;
    FString CharacterName;
    int32 Level = 1;
    FString JobClass;
    int32 HP = 0;
    int32 MaxHP = 100;
    int32 SP = 0;
    int32 MaxSP = 100;
    FString Zone;
    bool bIsOnline = false;
    bool bIsLeader = false;

    bool IsValid() const { return CharacterId > 0; }
};

USTRUCT()
struct FPartyData
{
    GENERATED_BODY()

    int32 PartyId = 0;
    FString PartyName;
    int32 LeaderId = 0;
    FString ExpShareMode;   // "each_take" or "even_share"
    FString ItemShareMode;  // "each_take", "party_share", "individual", "shared"
    TArray<FPartyMember> Members;

    bool IsValid() const { return PartyId > 0; }
};
```

### 4.2 UPartySubsystem Header

File: `client/SabriMMO/Source/SabriMMO/UI/PartySubsystem.h`

```cpp
// PartySubsystem.h -- UWorldSubsystem managing party state and the party Slate widget

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "PartySubsystem.generated.h"

class USocketIOClientComponent;
class SPartyWidget;

UCLASS()
class SABRIMMO_API UPartySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- public data ----
    FPartyData PartyState;
    int32 LocalCharacterId = 0;
    bool bHasPendingInvite = false;
    FString PendingInvitePartyName;
    FString PendingInviterName;
    int32 PendingInvitePartyId = 0;

    // ---- delegates (widget binds to these) ----
    DECLARE_MULTICAST_DELEGATE(FOnPartyUpdated);
    FOnPartyUpdated OnPartyUpdated;

    DECLARE_MULTICAST_DELEGATE(FOnPartyInviteReceived);
    FOnPartyInviteReceived OnPartyInviteReceived;

    // ---- lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- public actions (called by widget or chat commands) ----
    void CreateParty(const FString& PartyName, const FString& ItemShareMode);
    void InvitePlayer(const FString& TargetName);
    void RespondToInvite(bool bAccept);
    void LeaveParty();
    void KickMember(int32 TargetId);
    void ChangeLeader(int32 TargetId);
    void SetExpMode(const FString& Mode);

    // ---- widget ----
    void ToggleWidget();
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;
    bool IsInParty() const { return PartyState.IsValid(); }
    bool IsLeader() const { return PartyState.LeaderId == LocalCharacterId; }

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- event handlers ----
    void HandlePartyCreated(const TSharedPtr<FJsonValue>& Data);
    void HandlePartyData(const TSharedPtr<FJsonValue>& Data);
    void HandleMemberJoined(const TSharedPtr<FJsonValue>& Data);
    void HandleMemberLeft(const TSharedPtr<FJsonValue>& Data);
    void HandleMemberUpdate(const TSharedPtr<FJsonValue>& Data);
    void HandleLeaderChanged(const TSharedPtr<FJsonValue>& Data);
    void HandleSettingsChanged(const TSharedPtr<FJsonValue>& Data);
    void HandleInviteReceived(const TSharedPtr<FJsonValue>& Data);
    void HandlePartyDisbanded(const TSharedPtr<FJsonValue>& Data);
    void HandlePartyError(const TSharedPtr<FJsonValue>& Data);

    void ParseFullPartyData(const TSharedPtr<FJsonObject>& Obj);
    FPartyMember ParseMember(const TSharedPtr<FJsonObject>& MemberObj);

    // ---- emit helpers ----
    void EmitToServer(const FString& Event, const FString& JsonPayload);

    // ---- state ----
    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    TSharedPtr<SPartyWidget> PartyWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 4.3 UPartySubsystem Implementation

File: `client/SabriMMO/Source/SabriMMO/UI/PartySubsystem.cpp`

```cpp
#include "PartySubsystem.h"
#include "SPartyWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogParty, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UPartySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UPartySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
    {
        LocalCharacterId = GI->GetSelectedCharacter().CharacterId;
    }

    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UPartySubsystem::TryWrapSocketEvents),
        0.5f, true
    );
}

void UPartySubsystem::Deinitialize()
{
    HideWidget();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

// ============================================================
// Socket wrapping (same pattern as BasicInfoSubsystem)
// ============================================================

USocketIOClientComponent* UPartySubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
            return Comp;
    }
    return nullptr;
}

void UPartySubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;

    TSharedPtr<FSocketIONative> Native = SIOComp->GetNativeClient();
    if (!Native.IsValid() || !Native->bIsConnected) return;

    CachedSIOComponent = SIOComp;

    // Party events are new -- bind directly (no wrapping of existing BP events)
    auto BindEvent = [&](const FString& Event, TFunction<void(const TSharedPtr<FJsonValue>&)> Handler)
    {
        Native->OnEvent(Event,
            [Handler](const FString& E, const TSharedPtr<FJsonValue>& Msg) { Handler(Msg); },
            TEXT("/"), ESIOThreadOverrideOption::USE_GAME_THREAD);
    };

    BindEvent(TEXT("party:created"),          [this](auto D) { HandlePartyCreated(D); });
    BindEvent(TEXT("party:data"),             [this](auto D) { HandlePartyData(D); });
    BindEvent(TEXT("party:member_joined"),    [this](auto D) { HandleMemberJoined(D); });
    BindEvent(TEXT("party:member_left"),      [this](auto D) { HandleMemberLeft(D); });
    BindEvent(TEXT("party:member_update"),    [this](auto D) { HandleMemberUpdate(D); });
    BindEvent(TEXT("party:leader_changed"),   [this](auto D) { HandleLeaderChanged(D); });
    BindEvent(TEXT("party:settings_changed"), [this](auto D) { HandleSettingsChanged(D); });
    BindEvent(TEXT("party:invite_received"),  [this](auto D) { HandleInviteReceived(D); });
    BindEvent(TEXT("party:disbanded"),        [this](auto D) { HandlePartyDisbanded(D); });
    BindEvent(TEXT("party:error"),            [this](auto D) { HandlePartyError(D); });

    bEventsWrapped = true;
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(BindCheckTimer);

    UE_LOG(LogParty, Log, TEXT("PartySubsystem -- socket events bound."));
}

// ============================================================
// Emit helpers
// ============================================================

void UPartySubsystem::EmitToServer(const FString& Event, const FString& JsonPayload)
{
    if (CachedSIOComponent.IsValid())
    {
        CachedSIOComponent->EmitNative(Event, JsonPayload);
    }
}

// ============================================================
// Public actions
// ============================================================

void UPartySubsystem::CreateParty(const FString& PartyName, const FString& ItemShareMode)
{
    FString Payload = FString::Printf(
        TEXT("{\"partyName\":\"%s\",\"itemShareMode\":\"%s\"}"),
        *PartyName.Replace(TEXT("\""), TEXT("")),
        *ItemShareMode);
    EmitToServer(TEXT("party:create"), Payload);
}

void UPartySubsystem::InvitePlayer(const FString& TargetName)
{
    FString Payload = FString::Printf(TEXT("{\"targetName\":\"%s\"}"),
        *TargetName.Replace(TEXT("\""), TEXT("")));
    EmitToServer(TEXT("party:invite"), Payload);
}

void UPartySubsystem::RespondToInvite(bool bAccept)
{
    FString Payload = FString::Printf(TEXT("{\"partyId\":%d,\"accept\":%s}"),
        PendingInvitePartyId, bAccept ? TEXT("true") : TEXT("false"));
    EmitToServer(TEXT("party:invite_respond"), Payload);
    bHasPendingInvite = false;
}

void UPartySubsystem::LeaveParty()
{
    EmitToServer(TEXT("party:leave"), TEXT("{}"));
}

void UPartySubsystem::KickMember(int32 TargetId)
{
    FString Payload = FString::Printf(TEXT("{\"targetId\":%d}"), TargetId);
    EmitToServer(TEXT("party:kick"), Payload);
}

void UPartySubsystem::ChangeLeader(int32 TargetId)
{
    FString Payload = FString::Printf(TEXT("{\"targetId\":%d}"), TargetId);
    EmitToServer(TEXT("party:change_leader"), Payload);
}

void UPartySubsystem::SetExpMode(const FString& Mode)
{
    FString Payload = FString::Printf(TEXT("{\"mode\":\"%s\"}"), *Mode);
    EmitToServer(TEXT("party:set_exp_mode"), Payload);
}

// ============================================================
// Event handlers
// ============================================================

void UPartySubsystem::HandlePartyCreated(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    ParseFullPartyData(*ObjPtr);
    OnPartyUpdated.Broadcast();
    UE_LOG(LogParty, Log, TEXT("Party created: %s"), *PartyState.PartyName);
}

void UPartySubsystem::HandlePartyData(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    ParseFullPartyData(*ObjPtr);
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandleMemberJoined(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FPartyMember NewMember = ParseMember(*ObjPtr);
    // Avoid duplicates
    bool bFound = false;
    for (FPartyMember& M : PartyState.Members)
    {
        if (M.CharacterId == NewMember.CharacterId)
        {
            M = NewMember;
            bFound = true;
            break;
        }
    }
    if (!bFound) PartyState.Members.Add(NewMember);
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandleMemberLeft(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    double CharIdD = 0;
    (*ObjPtr)->TryGetNumberField(TEXT("characterId"), CharIdD);
    int32 CharId = (int32)CharIdD;

    PartyState.Members.RemoveAll([CharId](const FPartyMember& M) {
        return M.CharacterId == CharId;
    });
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandleMemberUpdate(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double CharIdD = 0;
    Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
    int32 CharId = (int32)CharIdD;

    for (FPartyMember& M : PartyState.Members)
    {
        if (M.CharacterId == CharId)
        {
            double V;
            if (Obj->TryGetNumberField(TEXT("hp"), V)) M.HP = (int32)V;
            if (Obj->TryGetNumberField(TEXT("maxHp"), V)) M.MaxHP = FMath::Max((int32)V, 1);
            if (Obj->TryGetNumberField(TEXT("sp"), V)) M.SP = (int32)V;
            if (Obj->TryGetNumberField(TEXT("maxSp"), V)) M.MaxSP = FMath::Max((int32)V, 1);
            if (Obj->TryGetNumberField(TEXT("level"), V)) M.Level = (int32)V;
            FString Str;
            if (Obj->TryGetStringField(TEXT("zone"), Str)) M.Zone = Str;
            if (Obj->TryGetStringField(TEXT("jobClass"), Str)) M.JobClass = Str;
            bool bOnline = false;
            if (Obj->TryGetBoolField(TEXT("isOnline"), bOnline)) M.bIsOnline = bOnline;
            break;
        }
    }
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandleLeaderChanged(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    double V = 0;
    (*ObjPtr)->TryGetNumberField(TEXT("newLeaderId"), V);
    PartyState.LeaderId = (int32)V;

    for (FPartyMember& M : PartyState.Members)
    {
        M.bIsLeader = (M.CharacterId == PartyState.LeaderId);
    }
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandleSettingsChanged(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FString Str;
    if ((*ObjPtr)->TryGetStringField(TEXT("expShareMode"), Str)) PartyState.ExpShareMode = Str;
    if ((*ObjPtr)->TryGetStringField(TEXT("itemShareMode"), Str)) PartyState.ItemShareMode = Str;
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandleInviteReceived(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    (*ObjPtr)->TryGetStringField(TEXT("partyName"), PendingInvitePartyName);
    (*ObjPtr)->TryGetStringField(TEXT("inviterName"), PendingInviterName);
    double V = 0;
    (*ObjPtr)->TryGetNumberField(TEXT("partyId"), V);
    PendingInvitePartyId = (int32)V;
    bHasPendingInvite = true;
    OnPartyInviteReceived.Broadcast();
}

void UPartySubsystem::HandlePartyDisbanded(const TSharedPtr<FJsonValue>& Data)
{
    PartyState = FPartyData();
    OnPartyUpdated.Broadcast();
}

void UPartySubsystem::HandlePartyError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    FString Msg;
    (*ObjPtr)->TryGetStringField(TEXT("message"), Msg);
    UE_LOG(LogParty, Warning, TEXT("Party error: %s"), *Msg);
    // TODO: route to chat subsystem as system message
}

// ============================================================
// Parsers
// ============================================================

void UPartySubsystem::ParseFullPartyData(const TSharedPtr<FJsonObject>& Obj)
{
    double V;
    Obj->TryGetNumberField(TEXT("partyId"), V);  PartyState.PartyId = (int32)V;
    Obj->TryGetNumberField(TEXT("leaderId"), V);  PartyState.LeaderId = (int32)V;
    Obj->TryGetStringField(TEXT("partyName"), PartyState.PartyName);
    Obj->TryGetStringField(TEXT("expShareMode"), PartyState.ExpShareMode);
    Obj->TryGetStringField(TEXT("itemShareMode"), PartyState.ItemShareMode);

    PartyState.Members.Empty();
    const TArray<TSharedPtr<FJsonValue>>* MembersArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("members"), MembersArr) && MembersArr)
    {
        for (const auto& MVal : *MembersArr)
        {
            const TSharedPtr<FJsonObject>* MObj = nullptr;
            if (MVal->TryGetObject(MObj) && MObj)
            {
                PartyState.Members.Add(ParseMember(*MObj));
            }
        }
    }
}

FPartyMember UPartySubsystem::ParseMember(const TSharedPtr<FJsonObject>& MemberObj)
{
    FPartyMember M;
    double V;
    MemberObj->TryGetNumberField(TEXT("characterId"), V); M.CharacterId = (int32)V;
    MemberObj->TryGetNumberField(TEXT("level"), V);       M.Level = (int32)V;
    MemberObj->TryGetNumberField(TEXT("hp"), V);          M.HP = (int32)V;
    MemberObj->TryGetNumberField(TEXT("maxHp"), V);       M.MaxHP = FMath::Max((int32)V, 1);
    MemberObj->TryGetNumberField(TEXT("sp"), V);          M.SP = (int32)V;
    MemberObj->TryGetNumberField(TEXT("maxSp"), V);       M.MaxSP = FMath::Max((int32)V, 1);

    MemberObj->TryGetStringField(TEXT("characterName"), M.CharacterName);
    MemberObj->TryGetStringField(TEXT("jobClass"), M.JobClass);
    MemberObj->TryGetStringField(TEXT("zone"), M.Zone);
    MemberObj->TryGetBoolField(TEXT("isOnline"), M.bIsOnline);
    MemberObj->TryGetBoolField(TEXT("isLeader"), M.bIsLeader);
    return M;
}

// ============================================================
// Widget
// ============================================================

void UPartySubsystem::ToggleWidget()
{
    if (bWidgetAdded) HideWidget();
    else ShowWidget();
}

void UPartySubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* VC = World->GetGameViewport();
    if (!VC) return;

    PartyWidget = SNew(SPartyWidget).Subsystem(this);
    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(PartyWidget);
    VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 11);
    bWidgetAdded = true;
}

void UPartySubsystem::HideWidget()
{
    if (!bWidgetAdded) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (ViewportOverlay.IsValid())
                VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
        }
    }
    PartyWidget.Reset();
    ViewportOverlay.Reset();
    bWidgetAdded = false;
}

bool UPartySubsystem::IsWidgetVisible() const
{
    return bWidgetAdded;
}
```

### 4.4 SPartyWidget (Slate)

File: `client/SabriMMO/Source/SabriMMO/UI/SPartyWidget.h`

```cpp
// SPartyWidget.h -- Draggable party window showing member list with HP bars

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UPartySubsystem;

class SPartyWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPartyWidget) {}
        SLATE_ARGUMENT(UPartySubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UPartySubsystem> OwningSubsystem;

    // Drag state
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D WidgetPosition = FVector2D(10.0, 200.0);

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;

    TSharedRef<SWidget> BuildMemberRow(int32 MemberIndex);
    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildInvitePopup();

    TSharedPtr<SBox> RootSizeBox;

    void ApplyLayout();
};
```

File: `client/SabriMMO/Source/SabriMMO/UI/SPartyWidget.cpp`

```cpp
#include "SPartyWidget.h"
#include "PartySubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"

void SPartyWidget::Construct(const FArguments& InArgs)
{
    OwningSubsystem = InArgs._Subsystem;

    ChildSlot
    [
        SNew(SBox)
        .WidthOverride(220.f)
        .Padding(0.f)
        [
            SNew(SBorder)
            .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.12f, 0.92f))
            .Padding(FMargin(4.f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [ BuildTitleBar() ]
                + SVerticalBox::Slot().FillHeight(1.f)
                [
                    SNew(SScrollBox)
                    // Members are dynamically built via OnPaint / Tick
                    // For Slate, we rebuild on delegate broadcast
                    + SScrollBox::Slot()
                    [
                        SNew(SVerticalBox)
                        // Populated dynamically; for the guide, 12 slots max
                        // The real implementation uses a lambda-driven SListView
                        // or rebuilds child slots when OnPartyUpdated fires.
                    ]
                ]
            ]
        ]
    ];

    ApplyLayout();
}

TSharedRef<SWidget> SPartyWidget::BuildTitleBar()
{
    return SNew(SBorder)
        .BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.25f, 1.f))
        .Padding(FMargin(6.f, 3.f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText {
                    if (!OwningSubsystem.IsValid()) return FText::FromString(TEXT("Party"));
                    const FString& Name = OwningSubsystem->PartyState.PartyName;
                    return FText::FromString(Name.IsEmpty() ? TEXT("Party") : Name);
                })
                .ColorAndOpacity(FLinearColor::White)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText {
                    if (!OwningSubsystem.IsValid()) return FText::GetEmpty();
                    int32 Count = OwningSubsystem->PartyState.Members.Num();
                    return FText::FromString(FString::Printf(TEXT("%d/12"), Count));
                })
                .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            ]
        ];
}

TSharedRef<SWidget> SPartyWidget::BuildMemberRow(int32 MemberIndex)
{
    // Each member row: Name | HP bar | Level
    return SNew(SBorder)
        .BorderBackgroundColor_Lambda([this, MemberIndex]() -> FLinearColor {
            if (!OwningSubsystem.IsValid()) return FLinearColor(0.08f, 0.08f, 0.15f, 0.8f);
            if (MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                return FLinearColor(0.08f, 0.08f, 0.15f, 0.8f);
            const auto& M = OwningSubsystem->PartyState.Members[MemberIndex];
            if (!M.bIsOnline) return FLinearColor(0.05f, 0.05f, 0.05f, 0.6f);
            if (M.bIsLeader)  return FLinearColor(0.15f, 0.12f, 0.05f, 0.8f);
            return FLinearColor(0.08f, 0.08f, 0.15f, 0.8f);
        })
        .Padding(FMargin(4.f, 2.f))
        [
            SNew(SVerticalBox)
            // Name + Level row
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, MemberIndex]() -> FText {
                        if (!OwningSubsystem.IsValid() ||
                            MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                            return FText::GetEmpty();
                        const auto& M = OwningSubsystem->PartyState.Members[MemberIndex];
                        FString Prefix = M.bIsLeader ? TEXT("[L] ") : TEXT("");
                        return FText::FromString(Prefix + M.CharacterName);
                    })
                    .ColorAndOpacity_Lambda([this, MemberIndex]() -> FSlateColor {
                        if (!OwningSubsystem.IsValid() ||
                            MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                            return FLinearColor::White;
                        return OwningSubsystem->PartyState.Members[MemberIndex].bIsOnline
                            ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f);
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, MemberIndex]() -> FText {
                        if (!OwningSubsystem.IsValid() ||
                            MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                            return FText::GetEmpty();
                        return FText::FromString(FString::Printf(TEXT("Lv%d"),
                            OwningSubsystem->PartyState.Members[MemberIndex].Level));
                    })
                    .ColorAndOpacity(FLinearColor(0.6f, 0.8f, 0.6f))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                ]
            ]
            // HP bar
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f, 0.f, 0.f)
            [
                SNew(SBox).HeightOverride(6.f)
                [
                    SNew(SProgressBar)
                    .Percent_Lambda([this, MemberIndex]() -> TOptional<float> {
                        if (!OwningSubsystem.IsValid() ||
                            MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                            return 0.f;
                        const auto& M = OwningSubsystem->PartyState.Members[MemberIndex];
                        return M.MaxHP > 0 ? (float)M.HP / M.MaxHP : 0.f;
                    })
                    .FillColorAndOpacity_Lambda([this, MemberIndex]() -> FLinearColor {
                        if (!OwningSubsystem.IsValid() ||
                            MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                            return FLinearColor(0.1f, 0.6f, 0.1f);
                        const auto& M = OwningSubsystem->PartyState.Members[MemberIndex];
                        float Pct = M.MaxHP > 0 ? (float)M.HP / M.MaxHP : 0.f;
                        if (Pct > 0.5f) return FLinearColor(0.1f, 0.6f, 0.1f);
                        if (Pct > 0.25f) return FLinearColor(0.8f, 0.7f, 0.1f);
                        return FLinearColor(0.8f, 0.1f, 0.1f);
                    })
                    .BackgroundImage(FCoreStyle::Get().GetBrush("ProgressBar.Background"))
                ]
            ]
            // SP bar
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f, 0.f, 0.f)
            [
                SNew(SBox).HeightOverride(4.f)
                [
                    SNew(SProgressBar)
                    .Percent_Lambda([this, MemberIndex]() -> TOptional<float> {
                        if (!OwningSubsystem.IsValid() ||
                            MemberIndex >= OwningSubsystem->PartyState.Members.Num())
                            return 0.f;
                        const auto& M = OwningSubsystem->PartyState.Members[MemberIndex];
                        return M.MaxSP > 0 ? (float)M.SP / M.MaxSP : 0.f;
                    })
                    .FillColorAndOpacity(FLinearColor(0.1f, 0.2f, 0.7f))
                ]
            ]
        ];
}

void SPartyWidget::ApplyLayout()
{
    if (RootSizeBox.IsValid())
    {
        RootSizeBox->SetRenderTransform(FSlateRenderTransform(WidgetPosition));
    }
}

FReply SPartyWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsDragging = true;
        DragOffset = MouseEvent.GetScreenSpacePosition() - WidgetPosition;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    return FReply::Unhandled();
}

FReply SPartyWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        bIsDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SPartyWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        WidgetPosition = MouseEvent.GetScreenSpacePosition() - DragOffset;
        ApplyLayout();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}
```

---

## 5. Guild System -- Server

### 5.1 In-Memory Data Structures

```javascript
// ============================================================
// GUILD SYSTEM -- In-memory state
// ============================================================

const MAX_GUILD_BASE_CAPACITY = 16;
const MAX_GUILD_LEVEL = 50;
const GUILD_EXTENSION_PER_LEVEL = 6;

// Guild EXP table (level -> cumulative EXP needed for next level)
const GUILD_EXP_TABLE = [];
(function buildGuildExpTable() {
    GUILD_EXP_TABLE[1] = 10000;
    for (let i = 2; i <= MAX_GUILD_LEVEL; i++) {
        GUILD_EXP_TABLE[i] = Math.floor(GUILD_EXP_TABLE[i - 1] * 1.35);
    }
})();

function getGuildExpForLevel(level) {
    if (level <= 0 || level >= MAX_GUILD_LEVEL) return Infinity;
    return GUILD_EXP_TABLE[level] || Infinity;
}

// guildId -> guild object
const guilds = new Map();

// characterId -> guildId (reverse lookup)
const playerGuildMap = new Map();

// characterId -> { guildId, inviterName, timestamp } (pending, expire 30s)
const pendingGuildInvites = new Map();

// Guild skill definitions
const GUILD_SKILL_DEFS = {
    'official_guild_approval':  { maxLevel: 1,  prereq: null },
    'kafra_contract':           { maxLevel: 1,  prereq: 'official_guild_approval' },
    'guardian_research':        { maxLevel: 1,  prereq: 'kafra_contract' },
    'strengthen_guardian':      { maxLevel: 1,  prereq: 'guardian_research' },
    'guild_extension':          { maxLevel: 10, prereq: 'official_guild_approval' },
    'guilds_glory':             { maxLevel: 1,  prereq: null },
    'great_leadership':         { maxLevel: 1,  prereq: 'guilds_glory' },
    'wounds_of_glory':          { maxLevel: 1,  prereq: 'guilds_glory' },
    'soul_of_cold':             { maxLevel: 1,  prereq: 'guilds_glory' },
    'sharp_hawk_eyes':          { maxLevel: 1,  prereq: 'guilds_glory' },
    'guild_storage_expansion':  { maxLevel: 5,  prereq: null },
    'battle_orders':            { maxLevel: 1,  prereq: 'official_guild_approval' },
    'regeneration':             { maxLevel: 3,  prereq: 'official_guild_approval' },
    'restore':                  { maxLevel: 1,  prereq: 'official_guild_approval' },
    'emergency_call':           { maxLevel: 1,  prereq: 'official_guild_approval' }
};

// Active guild skill cooldowns: guildId -> timestamp of last cast
const guildSkillCooldowns = new Map();
const GUILD_SKILL_COOLDOWN_MS = 5 * 60 * 1000; // 5 minutes shared cooldown
```

### 5.2 Load Guilds from DB on Startup

```javascript
async function loadGuildsFromDB() {
    try {
        const guildRows = await pool.query('SELECT * FROM guilds');
        for (const row of guildRows.rows) {
            const memberRows = await pool.query(`
                SELECT gm.character_id, gm.position_id, gm.contributed_exp,
                       c.name, c.level, c.class
                FROM guild_members gm
                JOIN characters c ON c.character_id = gm.character_id
                WHERE gm.guild_id = $1
            `, [row.guild_id]);

            const positionRows = await pool.query(
                'SELECT * FROM guild_positions WHERE guild_id = $1 ORDER BY position_id',
                [row.guild_id]
            );

            const skillRows = await pool.query(
                'SELECT skill_id, skill_level FROM guild_skills WHERE guild_id = $1',
                [row.guild_id]
            );

            const members = {};
            for (const m of memberRows.rows) {
                members[m.character_id] = {
                    characterId: m.character_id,
                    characterName: m.name,
                    level: m.level,
                    jobClass: m.class,
                    positionId: m.position_id,
                    contributedExp: parseInt(m.contributed_exp) || 0,
                    isOnline: connectedPlayers.has(m.character_id)
                };
                playerGuildMap.set(m.character_id, row.guild_id);
            }

            const positions = {};
            for (const p of positionRows.rows) {
                positions[p.position_id] = {
                    positionId: p.position_id,
                    positionName: p.position_name,
                    canInvite: p.can_invite,
                    canKick: p.can_kick,
                    canStorage: p.can_storage,
                    taxRate: p.tax_rate
                };
            }

            const skills = {};
            for (const s of skillRows.rows) {
                skills[s.skill_id] = s.skill_level;
            }

            // Calculate max members from guild_extension skill
            const extensionLevel = skills['guild_extension'] || 0;
            const maxMembers = MAX_GUILD_BASE_CAPACITY + (extensionLevel * GUILD_EXTENSION_PER_LEVEL);

            guilds.set(row.guild_id, {
                guildId: row.guild_id,
                guildName: row.guild_name,
                masterId: row.master_id,
                guildLevel: row.guild_level,
                guildExp: parseInt(row.guild_exp) || 0,
                guildExpNext: parseInt(row.guild_exp_next) || getGuildExpForLevel(row.guild_level),
                maxMembers: maxMembers,
                skillPoints: row.skill_points,
                noticeTitle: row.notice_title || '',
                noticeBody: row.notice_body || '',
                emblemData: row.emblem_data || null,
                members, positions, skills
            });
        }
        logger.info(`[GUILD] Loaded ${guilds.size} guilds from DB`);
    } catch (err) {
        logger.error(`[GUILD] Failed to load guilds: ${err.message}`);
    }
}
```

### 5.3 Helper: Emit to All Guild Members

```javascript
function emitToGuild(guildId, event, payload) {
    const guild = guilds.get(guildId);
    if (!guild) return;
    for (const charIdStr of Object.keys(guild.members)) {
        const charId = parseInt(charIdStr);
        const player = connectedPlayers.get(charId);
        if (player && player.socketId) {
            const targetSocket = io.sockets.sockets.get(player.socketId);
            if (targetSocket) targetSocket.emit(event, payload);
        }
    }
}

function buildFullGuildPayload(guild) {
    const memberList = Object.values(guild.members).map(m => {
        const p = connectedPlayers.get(m.characterId);
        return {
            characterId: m.characterId,
            characterName: m.characterName,
            level: p ? p.baseLevel : m.level,
            jobClass: p ? p.jobClass : m.jobClass,
            positionId: m.positionId,
            contributedExp: m.contributedExp,
            isOnline: !!p,
            isMaster: m.characterId === guild.masterId
        };
    });

    return {
        guildId: guild.guildId,
        guildName: guild.guildName,
        masterId: guild.masterId,
        guildLevel: guild.guildLevel,
        guildExp: guild.guildExp,
        guildExpNext: guild.guildExpNext,
        maxMembers: guild.maxMembers,
        skillPoints: guild.skillPoints,
        noticeTitle: guild.noticeTitle,
        noticeBody: guild.noticeBody,
        emblemData: guild.emblemData,
        members: memberList,
        positions: Object.values(guild.positions),
        skills: guild.skills
    };
}
```

### 5.4 Guild Socket Event Handlers

```javascript
// ---- GUILD: CREATE ----
socket.on('guild:create', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('guild:error', { message: 'Not logged in.' });
    const player = connectedPlayers.get(characterId);
    if (!player) return socket.emit('guild:error', { message: 'Player not found.' });

    if (playerGuildMap.has(characterId)) {
        return socket.emit('guild:error', { message: 'You are already in a guild.' });
    }

    const guildName = (data.guildName || '').trim().substring(0, 24);
    if (guildName.length < 2) {
        return socket.emit('guild:error', { message: 'Guild name must be at least 2 characters.' });
    }

    // Check for Emperium in inventory (item_id for Emperium = 7210, configurable)
    const EMPERIUM_ITEM_ID = 7210;
    try {
        const invCheck = await pool.query(
            'SELECT inventory_id FROM character_inventory WHERE character_id = $1 AND item_id = $2 AND quantity >= 1 LIMIT 1',
            [characterId, EMPERIUM_ITEM_ID]
        );
        if (invCheck.rows.length === 0) {
            return socket.emit('guild:error', { message: 'You need an Emperium to create a guild.' });
        }

        // Check name uniqueness
        const nameCheck = await pool.query('SELECT guild_id FROM guilds WHERE guild_name = $1', [guildName]);
        if (nameCheck.rows.length > 0) {
            return socket.emit('guild:error', { message: 'Guild name is already taken.' });
        }

        // Consume Emperium
        await pool.query(
            'DELETE FROM character_inventory WHERE inventory_id = $1',
            [invCheck.rows[0].inventory_id]
        );

        // Create guild
        const result = await pool.query(`
            INSERT INTO guilds (guild_name, master_id, guild_level, guild_exp, guild_exp_next, max_members, skill_points)
            VALUES ($1, $2, 1, 0, $3, $4, 0) RETURNING guild_id
        `, [guildName, characterId, getGuildExpForLevel(1), MAX_GUILD_BASE_CAPACITY]);
        const guildId = result.rows[0].guild_id;

        // Create default positions
        const positionValues = [];
        const positionParams = [];
        let paramIdx = 1;
        for (let i = 0; i < 20; i++) {
            const posName = i === 0 ? 'Guild Master' : (i === 1 ? 'Officer' : 'Member');
            const canInvite = i <= 1;
            const canKick = i === 0;
            const canStorage = i <= 2;
            positionValues.push(`($${paramIdx++}, $${paramIdx++}, $${paramIdx++}, $${paramIdx++}, $${paramIdx++}, $${paramIdx++}, $${paramIdx++})`);
            positionParams.push(guildId, i, posName, canInvite, canKick, canStorage, 0);
        }
        await pool.query(
            `INSERT INTO guild_positions (guild_id, position_id, position_name, can_invite, can_kick, can_storage, tax_rate)
             VALUES ${positionValues.join(', ')}`,
            positionParams
        );

        // Add master as member at position 0
        await pool.query(
            'INSERT INTO guild_members (guild_id, character_id, position_id) VALUES ($1, $2, 0)',
            [guildId, characterId]
        );

        // Build in-memory objects
        const positions = {};
        for (let i = 0; i < 20; i++) {
            positions[i] = {
                positionId: i,
                positionName: i === 0 ? 'Guild Master' : (i === 1 ? 'Officer' : 'Member'),
                canInvite: i <= 1, canKick: i === 0, canStorage: i <= 2, taxRate: 0
            };
        }

        const guild = {
            guildId, guildName, masterId: characterId,
            guildLevel: 1, guildExp: 0, guildExpNext: getGuildExpForLevel(1),
            maxMembers: MAX_GUILD_BASE_CAPACITY, skillPoints: 0,
            noticeTitle: '', noticeBody: '', emblemData: null,
            members: {
                [characterId]: {
                    characterId, characterName: player.characterName,
                    level: player.baseLevel, jobClass: player.jobClass,
                    positionId: 0, contributedExp: 0, isOnline: true
                }
            },
            positions, skills: {}
        };

        guilds.set(guildId, guild);
        playerGuildMap.set(characterId, guildId);

        socket.emit('guild:created', buildFullGuildPayload(guild));
        logger.info(`[GUILD] ${player.characterName} created guild "${guildName}" (ID: ${guildId})`);
    } catch (err) {
        logger.error(`[GUILD] Create failed: ${err.message}`);
        socket.emit('guild:error', { message: 'Failed to create guild.' });
    }
});

// ---- GUILD: INVITE ----
socket.on('guild:invite', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('guild:error', { message: 'Not logged in.' });

    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return socket.emit('guild:error', { message: 'You are not in a guild.' });

    const guild = guilds.get(guildId);
    if (!guild) return socket.emit('guild:error', { message: 'Guild not found.' });

    // Check invite permission
    const memberData = guild.members[characterId];
    const position = guild.positions[memberData.positionId];
    if (!position || (!position.canInvite && characterId !== guild.masterId)) {
        return socket.emit('guild:error', { message: 'You do not have invite permission.' });
    }

    if (Object.keys(guild.members).length >= guild.maxMembers) {
        return socket.emit('guild:error', { message: `Guild is full (${guild.maxMembers} max).` });
    }

    const targetName = (data.targetName || '').trim();
    let targetCharId = null, targetPlayer = null;
    for (const [cid, p] of connectedPlayers.entries()) {
        if (p.characterName && p.characterName.toLowerCase() === targetName.toLowerCase()) {
            targetCharId = cid; targetPlayer = p; break;
        }
    }

    if (!targetCharId) return socket.emit('guild:error', { message: `"${targetName}" is not online.` });
    if (playerGuildMap.has(targetCharId)) return socket.emit('guild:error', { message: `${targetPlayer.characterName} is already in a guild.` });

    pendingGuildInvites.set(targetCharId, {
        guildId, inviterName: connectedPlayers.get(characterId).characterName, timestamp: Date.now()
    });
    setTimeout(() => {
        const inv = pendingGuildInvites.get(targetCharId);
        if (inv && inv.guildId === guildId) pendingGuildInvites.delete(targetCharId);
    }, 30000);

    const targetSocket = io.sockets.sockets.get(targetPlayer.socketId);
    if (targetSocket) {
        targetSocket.emit('guild:invite_received', {
            guildId, guildName: guild.guildName,
            inviterName: connectedPlayers.get(characterId).characterName
        });
    }
});

// ---- GUILD: INVITE RESPOND ----
socket.on('guild:invite_respond', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('guild:error', { message: 'Not logged in.' });

    const invite = pendingGuildInvites.get(characterId);
    if (!invite || invite.guildId !== data.guildId) {
        return socket.emit('guild:error', { message: 'No pending guild invite.' });
    }
    pendingGuildInvites.delete(characterId);
    if (!data.accept) return;

    if (playerGuildMap.has(characterId)) return socket.emit('guild:error', { message: 'Already in a guild.' });

    const guild = guilds.get(invite.guildId);
    if (!guild) return socket.emit('guild:error', { message: 'Guild no longer exists.' });
    if (Object.keys(guild.members).length >= guild.maxMembers) return socket.emit('guild:error', { message: 'Guild is full.' });

    const player = connectedPlayers.get(characterId);
    if (!player) return;

    try {
        await pool.query('INSERT INTO guild_members (guild_id, character_id, position_id) VALUES ($1, $2, 19)', [invite.guildId, characterId]);

        guild.members[characterId] = {
            characterId, characterName: player.characterName,
            level: player.baseLevel, jobClass: player.jobClass,
            positionId: 19, contributedExp: 0, isOnline: true
        };
        playerGuildMap.set(characterId, invite.guildId);

        socket.emit('guild:data', buildFullGuildPayload(guild));
        emitToGuild(invite.guildId, 'guild:member_joined', {
            characterId, characterName: player.characterName,
            level: player.baseLevel, jobClass: player.jobClass,
            positionId: 19, isOnline: true
        });
        logger.info(`[GUILD] ${player.characterName} joined "${guild.guildName}"`);
    } catch (err) {
        logger.error(`[GUILD] Join failed: ${err.message}`);
        socket.emit('guild:error', { message: 'Failed to join guild.' });
    }
});

// ---- GUILD: LEAVE ----
socket.on('guild:leave', async () => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return socket.emit('guild:error', { message: 'Not in a guild.' });
    const guild = guilds.get(guildId);
    if (!guild) return;

    if (characterId === guild.masterId) {
        return socket.emit('guild:error', { message: 'Guild Master cannot leave. Transfer leadership or disband.' });
    }

    await removeFromGuild(characterId, guildId, 'left');
});

// ---- GUILD: KICK ----
socket.on('guild:kick', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return socket.emit('guild:error', { message: 'Not in a guild.' });
    const guild = guilds.get(guildId);
    if (!guild) return;

    const memberData = guild.members[characterId];
    const position = guild.positions[memberData.positionId];
    if (!position || (!position.canKick && characterId !== guild.masterId)) {
        return socket.emit('guild:error', { message: 'No kick permission.' });
    }

    const targetId = parseInt(data.targetId);
    if (targetId === guild.masterId) return socket.emit('guild:error', { message: 'Cannot kick the Guild Master.' });
    if (!guild.members[targetId]) return socket.emit('guild:error', { message: 'Target not in guild.' });

    await removeFromGuild(targetId, guildId, 'kicked');
});

// ---- GUILD: LEARN SKILL ----
socket.on('guild:learn_skill', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return socket.emit('guild:error', { message: 'Not in a guild.' });
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) {
        return socket.emit('guild:error', { message: 'Only the Guild Master can learn skills.' });
    }

    const skillId = data.skillId;
    const skillDef = GUILD_SKILL_DEFS[skillId];
    if (!skillDef) return socket.emit('guild:error', { message: 'Unknown guild skill.' });

    const currentLevel = guild.skills[skillId] || 0;
    if (currentLevel >= skillDef.maxLevel) return socket.emit('guild:error', { message: 'Skill already at max level.' });
    if (guild.skillPoints < 1) return socket.emit('guild:error', { message: 'No skill points available.' });

    if (skillDef.prereq) {
        const prereqLevel = guild.skills[skillDef.prereq] || 0;
        if (prereqLevel < 1) return socket.emit('guild:error', { message: `Prerequisite "${skillDef.prereq}" not learned.` });
    }

    const newLevel = currentLevel + 1;
    guild.skills[skillId] = newLevel;
    guild.skillPoints -= 1;

    // Update max members if guild_extension
    if (skillId === 'guild_extension') {
        guild.maxMembers = MAX_GUILD_BASE_CAPACITY + (newLevel * GUILD_EXTENSION_PER_LEVEL);
        await pool.query('UPDATE guilds SET max_members = $1 WHERE guild_id = $2', [guild.maxMembers, guildId]);
    }

    await pool.query('UPDATE guilds SET skill_points = $1 WHERE guild_id = $2', [guild.skillPoints, guildId]);
    await pool.query(`
        INSERT INTO guild_skills (guild_id, skill_id, skill_level) VALUES ($1, $2, $3)
        ON CONFLICT (guild_id, skill_id) DO UPDATE SET skill_level = $3
    `, [guildId, skillId, newLevel]);

    emitToGuild(guildId, 'guild:skill_updated', {
        skillId, newLevel, remainingPoints: guild.skillPoints
    });
    logger.info(`[GUILD] "${guild.guildName}" learned ${skillId} Lv${newLevel}`);
});

// ---- GUILD: SET NOTICE ----
socket.on('guild:set_notice', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return;
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) return socket.emit('guild:error', { message: 'Only the master can set the notice.' });

    guild.noticeTitle = (data.title || '').substring(0, 60);
    guild.noticeBody = (data.body || '').substring(0, 240);

    await pool.query('UPDATE guilds SET notice_title = $1, notice_body = $2 WHERE guild_id = $3',
        [guild.noticeTitle, guild.noticeBody, guildId]);

    emitToGuild(guildId, 'guild:notice_updated', { title: guild.noticeTitle, body: guild.noticeBody });
});

// ---- GUILD: UPDATE POSITION ----
socket.on('guild:update_position', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return;
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) return socket.emit('guild:error', { message: 'Master only.' });

    const posId = parseInt(data.positionId);
    if (posId < 0 || posId > 19 || posId === 0) return socket.emit('guild:error', { message: 'Cannot edit Guild Master position.' });

    const pos = guild.positions[posId];
    if (!pos) return;

    pos.positionName = (data.positionName || pos.positionName).substring(0, 24);
    pos.canInvite = !!data.canInvite;
    pos.canKick = !!data.canKick;
    pos.canStorage = !!data.canStorage;
    pos.taxRate = Math.max(0, Math.min(50, parseInt(data.taxRate) || 0));

    await pool.query(`
        UPDATE guild_positions SET position_name=$1, can_invite=$2, can_kick=$3, can_storage=$4, tax_rate=$5
        WHERE guild_id=$6 AND position_id=$7
    `, [pos.positionName, pos.canInvite, pos.canKick, pos.canStorage, pos.taxRate, guildId, posId]);

    emitToGuild(guildId, 'guild:data', buildFullGuildPayload(guild));
});

// ---- GUILD: SET MEMBER POSITION ----
socket.on('guild:set_position', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return;
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) return socket.emit('guild:error', { message: 'Master only.' });

    const targetId = parseInt(data.targetId);
    const posId = parseInt(data.positionId);
    if (!guild.members[targetId]) return socket.emit('guild:error', { message: 'Not a member.' });
    if (posId < 1 || posId > 19) return socket.emit('guild:error', { message: 'Invalid position.' });

    guild.members[targetId].positionId = posId;
    await pool.query('UPDATE guild_members SET position_id=$1 WHERE guild_id=$2 AND character_id=$3',
        [posId, guildId, targetId]);

    emitToGuild(guildId, 'guild:data', buildFullGuildPayload(guild));
});

// ---- GUILD: DISBAND ----
socket.on('guild:disband', async () => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return;
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) return socket.emit('guild:error', { message: 'Master only.' });

    if (Object.keys(guild.members).length > 1) {
        return socket.emit('guild:error', { message: 'Kick all members before disbanding.' });
    }

    // Notify all (just master at this point)
    emitToGuild(guildId, 'guild:disbanded', { guildId });

    // Clean up memory
    for (const cid of Object.keys(guild.members)) playerGuildMap.delete(parseInt(cid));
    guilds.delete(guildId);

    // Clean up DB
    await pool.query('DELETE FROM guilds WHERE guild_id = $1', [guildId]);
    logger.info(`[GUILD] "${guild.guildName}" disbanded by ${connectedPlayers.get(characterId)?.characterName}`);
});

// ---- GUILD: CHANGE MASTER ----
socket.on('guild:change_master', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return;
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) return socket.emit('guild:error', { message: 'Master only.' });

    const targetId = parseInt(data.targetId);
    if (!guild.members[targetId]) return socket.emit('guild:error', { message: 'Target not in guild.' });

    // Swap positions
    guild.members[characterId].positionId = 1; // demote old master to Officer
    guild.members[targetId].positionId = 0;    // promote new master
    guild.masterId = targetId;

    await pool.query('UPDATE guilds SET master_id = $1 WHERE guild_id = $2', [targetId, guildId]);
    await pool.query('UPDATE guild_members SET position_id = 1 WHERE guild_id = $1 AND character_id = $2', [guildId, characterId]);
    await pool.query('UPDATE guild_members SET position_id = 0 WHERE guild_id = $1 AND character_id = $2', [guildId, targetId]);

    emitToGuild(guildId, 'guild:data', buildFullGuildPayload(guild));
    logger.info(`[GUILD] "${guild.guildName}" master transferred to ${guild.members[targetId].characterName}`);
});

// ---- GUILD: SET EMBLEM ----
socket.on('guild:set_emblem', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return;
    const guild = guilds.get(guildId);
    if (!guild || guild.masterId !== characterId) return socket.emit('guild:error', { message: 'Master only.' });

    const emblemData = (data.emblemData || '').substring(0, 10000); // safety cap
    guild.emblemData = emblemData;
    await pool.query('UPDATE guilds SET emblem_data = $1 WHERE guild_id = $2', [emblemData, guildId]);
    emitToGuild(guildId, 'guild:emblem_updated', { emblemData });
});
```

### 5.5 Remove from Guild Helper

```javascript
async function removeFromGuild(characterId, guildId, reason) {
    const guild = guilds.get(guildId);
    if (!guild) return;

    const memberName = guild.members[characterId]?.characterName || 'Unknown';
    delete guild.members[characterId];
    playerGuildMap.delete(characterId);

    await pool.query('DELETE FROM guild_members WHERE guild_id = $1 AND character_id = $2', [guildId, characterId]);

    const removedPlayer = connectedPlayers.get(characterId);
    if (removedPlayer) {
        const s = io.sockets.sockets.get(removedPlayer.socketId);
        if (s) s.emit('guild:disbanded', { guildId });
    }

    emitToGuild(guildId, 'guild:member_left', { characterId, characterName: memberName, reason });
    logger.info(`[GUILD] ${memberName} ${reason} "${guild.guildName}"`);
}
```

### 5.6 Guild EXP Tax Integration

Insert into the existing `givePlayerExp` function (or the enemy death EXP handler):

```javascript
/**
 * Apply guild EXP tax before giving base EXP to a player.
 * Returns the amount of baseExp the player actually receives.
 */
function applyGuildTax(characterId, baseExp) {
    const guildId = playerGuildMap.get(characterId);
    if (!guildId) return baseExp; // no guild, full EXP

    const guild = guilds.get(guildId);
    if (!guild) return baseExp;

    const member = guild.members[characterId];
    if (!member) return baseExp;

    const position = guild.positions[member.positionId];
    const taxRate = position ? position.taxRate : 0;
    if (taxRate <= 0) return baseExp;

    const taxedAmount = Math.floor(baseExp * (taxRate / 100));
    const playerExp = baseExp - taxedAmount;

    // Add taxed EXP to guild
    guild.guildExp += taxedAmount;
    member.contributedExp += taxedAmount;

    // Check for guild level up
    while (guild.guildLevel < MAX_GUILD_LEVEL && guild.guildExp >= guild.guildExpNext) {
        guild.guildExp -= guild.guildExpNext;
        guild.guildLevel += 1;
        guild.skillPoints += 1;
        guild.guildExpNext = getGuildExpForLevel(guild.guildLevel);

        emitToGuild(guildId, 'guild:level_up', {
            guildLevel: guild.guildLevel,
            guildExp: guild.guildExp,
            guildExpNext: guild.guildExpNext,
            skillPoints: guild.skillPoints
        });

        // Persist level up
        pool.query('UPDATE guilds SET guild_level=$1, guild_exp=$2, guild_exp_next=$3, skill_points=$4 WHERE guild_id=$5',
            [guild.guildLevel, guild.guildExp, guild.guildExpNext, guild.skillPoints, guildId]);

        logger.info(`[GUILD] "${guild.guildName}" leveled up to ${guild.guildLevel}!`);
    }

    // Periodic DB save (batched, not per-kill — use a 30s interval or on disconnect)
    return playerExp;
}
```

### 5.7 Send Guild Data on Login

```javascript
// Inside player:join handler, after connectedPlayers.set(...)
const existingGuildId = playerGuildMap.get(characterId);
if (existingGuildId) {
    const guild = guilds.get(existingGuildId);
    if (guild && guild.members[characterId]) {
        guild.members[characterId].isOnline = true;
        socket.emit('guild:data', buildFullGuildPayload(guild));
        emitToGuild(existingGuildId, 'guild:member_update', {
            characterId, isOnline: true,
            level: player.baseLevel, jobClass: player.jobClass,
            zone: player.zone || ''
        });
    }
}
```

---

## 6. Guild System -- Client

### 6.1 Data Structs (add to CharacterData.h)

```cpp
// ============================================================
// Guild data structs
// ============================================================

USTRUCT()
struct FGuildPosition
{
    GENERATED_BODY()

    int32 PositionId = 0;
    FString PositionName;
    bool bCanInvite = false;
    bool bCanKick = false;
    bool bCanStorage = false;
    int32 TaxRate = 0;
};

USTRUCT()
struct FGuildMember
{
    GENERATED_BODY()

    int32 CharacterId = 0;
    FString CharacterName;
    int32 Level = 1;
    FString JobClass;
    int32 PositionId = 19;
    int64 ContributedExp = 0;
    bool bIsOnline = false;
    bool bIsMaster = false;

    bool IsValid() const { return CharacterId > 0; }
};

USTRUCT()
struct FGuildSkillEntry
{
    GENERATED_BODY()

    FString SkillId;
    int32 SkillLevel = 0;
};

USTRUCT()
struct FGuildData
{
    GENERATED_BODY()

    int32 GuildId = 0;
    FString GuildName;
    int32 MasterId = 0;
    int32 GuildLevel = 1;
    int64 GuildExp = 0;
    int64 GuildExpNext = 10000;
    int32 MaxMembers = 16;
    int32 SkillPoints = 0;
    FString NoticeTitle;
    FString NoticeBody;
    FString EmblemData;   // base64
    TArray<FGuildMember> Members;
    TArray<FGuildPosition> Positions;
    TMap<FString, int32> Skills;

    bool IsValid() const { return GuildId > 0; }
};
```

### 6.2 UGuildSubsystem Header

File: `client/SabriMMO/Source/SabriMMO/UI/GuildSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "GuildSubsystem.generated.h"

class USocketIOClientComponent;
class SGuildWidget;

UCLASS()
class SABRIMMO_API UGuildSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    FGuildData GuildState;
    int32 LocalCharacterId = 0;

    bool bHasPendingInvite = false;
    FString PendingInviteGuildName;
    FString PendingInviterName;
    int32 PendingInviteGuildId = 0;

    DECLARE_MULTICAST_DELEGATE(FOnGuildUpdated);
    FOnGuildUpdated OnGuildUpdated;

    DECLARE_MULTICAST_DELEGATE(FOnGuildInviteReceived);
    FOnGuildInviteReceived OnGuildInviteReceived;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // Actions
    void CreateGuild(const FString& GuildName);
    void InvitePlayer(const FString& TargetName);
    void RespondToInvite(bool bAccept);
    void LeaveGuild();
    void KickMember(int32 TargetId);
    void SetMemberPosition(int32 TargetId, int32 PositionId);
    void UpdatePosition(int32 PositionId, const FString& Name, bool bInvite, bool bKick, bool bStorage, int32 Tax);
    void SetNotice(const FString& Title, const FString& Body);
    void LearnSkill(const FString& SkillId);
    void TransferMaster(int32 TargetId);
    void DisbandGuild();
    void SetEmblem(const FString& Base64Data);

    // Widget
    void ToggleWidget();
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;
    bool IsInGuild() const { return GuildState.IsValid(); }
    bool IsMaster() const { return GuildState.MasterId == LocalCharacterId; }

private:
    void TryWrapSocketEvents();
    USocketIOClientComponent* FindSocketIOComponent() const;
    void EmitToServer(const FString& Event, const FString& JsonPayload);

    void HandleGuildCreated(const TSharedPtr<FJsonValue>& Data);
    void HandleGuildData(const TSharedPtr<FJsonValue>& Data);
    void HandleMemberJoined(const TSharedPtr<FJsonValue>& Data);
    void HandleMemberLeft(const TSharedPtr<FJsonValue>& Data);
    void HandleMemberUpdate(const TSharedPtr<FJsonValue>& Data);
    void HandleSkillUpdated(const TSharedPtr<FJsonValue>& Data);
    void HandleLevelUp(const TSharedPtr<FJsonValue>& Data);
    void HandleNoticeUpdated(const TSharedPtr<FJsonValue>& Data);
    void HandleEmblemUpdated(const TSharedPtr<FJsonValue>& Data);
    void HandleGuildDisbanded(const TSharedPtr<FJsonValue>& Data);
    void HandleInviteReceived(const TSharedPtr<FJsonValue>& Data);
    void HandleGuildError(const TSharedPtr<FJsonValue>& Data);

    void ParseFullGuildData(const TSharedPtr<FJsonObject>& Obj);
    FGuildMember ParseGuildMember(const TSharedPtr<FJsonObject>& Obj);
    FGuildPosition ParseGuildPosition(const TSharedPtr<FJsonObject>& Obj);

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    TSharedPtr<SGuildWidget> GuildWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 6.3 UGuildSubsystem Implementation (key methods)

File: `client/SabriMMO/Source/SabriMMO/UI/GuildSubsystem.cpp`

```cpp
#include "GuildSubsystem.h"
#include "SGuildWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGuild, Log, All);

bool UGuildSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UGuildSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
        LocalCharacterId = GI->GetSelectedCharacter().CharacterId;

    InWorld.GetTimerManager().SetTimer(BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UGuildSubsystem::TryWrapSocketEvents),
        0.5f, true);
}

void UGuildSubsystem::Deinitialize()
{
    HideWidget();
    if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(BindCheckTimer);
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

USocketIOClientComponent* UGuildSubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (auto* C = It->FindComponentByClass<USocketIOClientComponent>()) return C;
    return nullptr;
}

void UGuildSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;
    TSharedPtr<FSocketIONative> Native = SIO->GetNativeClient();
    if (!Native.IsValid() || !Native->bIsConnected) return;
    CachedSIOComponent = SIO;

    auto Bind = [&](const FString& Ev, TFunction<void(const TSharedPtr<FJsonValue>&)> H) {
        Native->OnEvent(Ev,
            [H](const FString&, const TSharedPtr<FJsonValue>& M) { H(M); },
            TEXT("/"), ESIOThreadOverrideOption::USE_GAME_THREAD);
    };

    Bind(TEXT("guild:created"),        [this](auto D) { HandleGuildCreated(D); });
    Bind(TEXT("guild:data"),           [this](auto D) { HandleGuildData(D); });
    Bind(TEXT("guild:member_joined"),  [this](auto D) { HandleMemberJoined(D); });
    Bind(TEXT("guild:member_left"),    [this](auto D) { HandleMemberLeft(D); });
    Bind(TEXT("guild:member_update"),  [this](auto D) { HandleMemberUpdate(D); });
    Bind(TEXT("guild:skill_updated"),  [this](auto D) { HandleSkillUpdated(D); });
    Bind(TEXT("guild:level_up"),       [this](auto D) { HandleLevelUp(D); });
    Bind(TEXT("guild:notice_updated"), [this](auto D) { HandleNoticeUpdated(D); });
    Bind(TEXT("guild:emblem_updated"), [this](auto D) { HandleEmblemUpdated(D); });
    Bind(TEXT("guild:disbanded"),      [this](auto D) { HandleGuildDisbanded(D); });
    Bind(TEXT("guild:invite_received"),[this](auto D) { HandleInviteReceived(D); });
    Bind(TEXT("guild:error"),          [this](auto D) { HandleGuildError(D); });

    bEventsWrapped = true;
    if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(BindCheckTimer);
    UE_LOG(LogGuild, Log, TEXT("GuildSubsystem -- socket events bound."));
}

void UGuildSubsystem::EmitToServer(const FString& Event, const FString& Payload)
{
    if (CachedSIOComponent.IsValid()) CachedSIOComponent->EmitNative(Event, Payload);
}

// --- Actions (emit to server) ---

void UGuildSubsystem::CreateGuild(const FString& GuildName)
{
    EmitToServer(TEXT("guild:create"),
        FString::Printf(TEXT("{\"guildName\":\"%s\"}"), *GuildName.Replace(TEXT("\""), TEXT(""))));
}

void UGuildSubsystem::InvitePlayer(const FString& TargetName)
{
    EmitToServer(TEXT("guild:invite"),
        FString::Printf(TEXT("{\"targetName\":\"%s\"}"), *TargetName.Replace(TEXT("\""), TEXT(""))));
}

void UGuildSubsystem::RespondToInvite(bool bAccept)
{
    EmitToServer(TEXT("guild:invite_respond"),
        FString::Printf(TEXT("{\"guildId\":%d,\"accept\":%s}"), PendingInviteGuildId,
            bAccept ? TEXT("true") : TEXT("false")));
    bHasPendingInvite = false;
}

void UGuildSubsystem::LeaveGuild() { EmitToServer(TEXT("guild:leave"), TEXT("{}")); }
void UGuildSubsystem::KickMember(int32 Id) { EmitToServer(TEXT("guild:kick"), FString::Printf(TEXT("{\"targetId\":%d}"), Id)); }
void UGuildSubsystem::DisbandGuild() { EmitToServer(TEXT("guild:disband"), TEXT("{}")); }
void UGuildSubsystem::TransferMaster(int32 Id) { EmitToServer(TEXT("guild:change_master"), FString::Printf(TEXT("{\"targetId\":%d}"), Id)); }
void UGuildSubsystem::LearnSkill(const FString& SkillId) { EmitToServer(TEXT("guild:learn_skill"), FString::Printf(TEXT("{\"skillId\":\"%s\"}"), *SkillId)); }

void UGuildSubsystem::SetNotice(const FString& Title, const FString& Body)
{
    EmitToServer(TEXT("guild:set_notice"),
        FString::Printf(TEXT("{\"title\":\"%s\",\"body\":\"%s\"}"),
            *Title.Replace(TEXT("\""), TEXT("")), *Body.Replace(TEXT("\""), TEXT(""))));
}

void UGuildSubsystem::SetMemberPosition(int32 TargetId, int32 PosId)
{
    EmitToServer(TEXT("guild:set_position"),
        FString::Printf(TEXT("{\"targetId\":%d,\"positionId\":%d}"), TargetId, PosId));
}

void UGuildSubsystem::UpdatePosition(int32 PosId, const FString& Name, bool bInv, bool bKick, bool bStor, int32 Tax)
{
    EmitToServer(TEXT("guild:update_position"),
        FString::Printf(TEXT("{\"positionId\":%d,\"positionName\":\"%s\",\"canInvite\":%s,\"canKick\":%s,\"canStorage\":%s,\"taxRate\":%d}"),
            PosId, *Name.Replace(TEXT("\""), TEXT("")),
            bInv ? TEXT("true") : TEXT("false"),
            bKick ? TEXT("true") : TEXT("false"),
            bStor ? TEXT("true") : TEXT("false"), Tax));
}

void UGuildSubsystem::SetEmblem(const FString& B64) { EmitToServer(TEXT("guild:set_emblem"), FString::Printf(TEXT("{\"emblemData\":\"%s\"}"), *B64)); }

// --- Event handlers ---

void UGuildSubsystem::HandleGuildCreated(const TSharedPtr<FJsonValue>& D) { HandleGuildData(D); }

void UGuildSubsystem::HandleGuildData(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    ParseFullGuildData(*O);
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleMemberJoined(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    FGuildMember M = ParseGuildMember(*O);
    bool bFound = false;
    for (auto& Existing : GuildState.Members)
        if (Existing.CharacterId == M.CharacterId) { Existing = M; bFound = true; break; }
    if (!bFound) GuildState.Members.Add(M);
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleMemberLeft(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    double V = 0; (*O)->TryGetNumberField(TEXT("characterId"), V);
    int32 CId = (int32)V;
    GuildState.Members.RemoveAll([CId](const FGuildMember& M) { return M.CharacterId == CId; });
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleMemberUpdate(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    double V = 0; (*O)->TryGetNumberField(TEXT("characterId"), V);
    int32 CId = (int32)V;
    for (auto& M : GuildState.Members) {
        if (M.CharacterId == CId) {
            bool bOn = false; (*O)->TryGetBoolField(TEXT("isOnline"), bOn); M.bIsOnline = bOn;
            if ((*O)->TryGetNumberField(TEXT("level"), V)) M.Level = (int32)V;
            FString S; if ((*O)->TryGetStringField(TEXT("jobClass"), S)) M.JobClass = S;
            break;
        }
    }
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleSkillUpdated(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    FString SkId; (*O)->TryGetStringField(TEXT("skillId"), SkId);
    double V = 0; (*O)->TryGetNumberField(TEXT("newLevel"), V);
    GuildState.Skills.FindOrAdd(SkId) = (int32)V;
    (*O)->TryGetNumberField(TEXT("remainingPoints"), V); GuildState.SkillPoints = (int32)V;
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleLevelUp(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    double V;
    (*O)->TryGetNumberField(TEXT("guildLevel"), V); GuildState.GuildLevel = (int32)V;
    (*O)->TryGetNumberField(TEXT("guildExp"), V); GuildState.GuildExp = (int64)V;
    (*O)->TryGetNumberField(TEXT("guildExpNext"), V); GuildState.GuildExpNext = (int64)V;
    (*O)->TryGetNumberField(TEXT("skillPoints"), V); GuildState.SkillPoints = (int32)V;
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleNoticeUpdated(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    (*O)->TryGetStringField(TEXT("title"), GuildState.NoticeTitle);
    (*O)->TryGetStringField(TEXT("body"), GuildState.NoticeBody);
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleEmblemUpdated(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    (*O)->TryGetStringField(TEXT("emblemData"), GuildState.EmblemData);
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleGuildDisbanded(const TSharedPtr<FJsonValue>& Data)
{
    GuildState = FGuildData();
    OnGuildUpdated.Broadcast();
}

void UGuildSubsystem::HandleInviteReceived(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    (*O)->TryGetStringField(TEXT("guildName"), PendingInviteGuildName);
    (*O)->TryGetStringField(TEXT("inviterName"), PendingInviterName);
    double V = 0; (*O)->TryGetNumberField(TEXT("guildId"), V);
    PendingInviteGuildId = (int32)V;
    bHasPendingInvite = true;
    OnGuildInviteReceived.Broadcast();
}

void UGuildSubsystem::HandleGuildError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O = nullptr;
    if (!Data->TryGetObject(O) || !O) return;
    FString Msg; (*O)->TryGetStringField(TEXT("message"), Msg);
    UE_LOG(LogGuild, Warning, TEXT("Guild error: %s"), *Msg);
}

// --- Parsers ---

void UGuildSubsystem::ParseFullGuildData(const TSharedPtr<FJsonObject>& Obj)
{
    double V;
    Obj->TryGetNumberField(TEXT("guildId"), V); GuildState.GuildId = (int32)V;
    Obj->TryGetNumberField(TEXT("masterId"), V); GuildState.MasterId = (int32)V;
    Obj->TryGetNumberField(TEXT("guildLevel"), V); GuildState.GuildLevel = (int32)V;
    Obj->TryGetNumberField(TEXT("guildExp"), V); GuildState.GuildExp = (int64)V;
    Obj->TryGetNumberField(TEXT("guildExpNext"), V); GuildState.GuildExpNext = FMath::Max((int64)V, (int64)1);
    Obj->TryGetNumberField(TEXT("maxMembers"), V); GuildState.MaxMembers = (int32)V;
    Obj->TryGetNumberField(TEXT("skillPoints"), V); GuildState.SkillPoints = (int32)V;
    Obj->TryGetStringField(TEXT("guildName"), GuildState.GuildName);
    Obj->TryGetStringField(TEXT("noticeTitle"), GuildState.NoticeTitle);
    Obj->TryGetStringField(TEXT("noticeBody"), GuildState.NoticeBody);
    Obj->TryGetStringField(TEXT("emblemData"), GuildState.EmblemData);

    GuildState.Members.Empty();
    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (Obj->TryGetArrayField(TEXT("members"), Arr) && Arr)
        for (auto& MV : *Arr) { const TSharedPtr<FJsonObject>* MO; if (MV->TryGetObject(MO)) GuildState.Members.Add(ParseGuildMember(*MO)); }

    GuildState.Positions.Empty();
    if (Obj->TryGetArrayField(TEXT("positions"), Arr) && Arr)
        for (auto& PV : *Arr) { const TSharedPtr<FJsonObject>* PO; if (PV->TryGetObject(PO)) GuildState.Positions.Add(ParseGuildPosition(*PO)); }

    GuildState.Skills.Empty();
    const TSharedPtr<FJsonObject>* SkillsObj;
    if (Obj->TryGetObjectField(TEXT("skills"), SkillsObj) && SkillsObj)
        for (auto& Pair : (*SkillsObj)->Values)
            GuildState.Skills.Add(Pair.Key, (int32)Pair.Value->AsNumber());
}

FGuildMember UGuildSubsystem::ParseGuildMember(const TSharedPtr<FJsonObject>& O)
{
    FGuildMember M; double V;
    O->TryGetNumberField(TEXT("characterId"), V); M.CharacterId = (int32)V;
    O->TryGetNumberField(TEXT("level"), V); M.Level = (int32)V;
    O->TryGetNumberField(TEXT("positionId"), V); M.PositionId = (int32)V;
    O->TryGetNumberField(TEXT("contributedExp"), V); M.ContributedExp = (int64)V;
    O->TryGetStringField(TEXT("characterName"), M.CharacterName);
    O->TryGetStringField(TEXT("jobClass"), M.JobClass);
    O->TryGetBoolField(TEXT("isOnline"), M.bIsOnline);
    O->TryGetBoolField(TEXT("isMaster"), M.bIsMaster);
    return M;
}

FGuildPosition UGuildSubsystem::ParseGuildPosition(const TSharedPtr<FJsonObject>& O)
{
    FGuildPosition P; double V;
    O->TryGetNumberField(TEXT("positionId"), V); P.PositionId = (int32)V;
    O->TryGetNumberField(TEXT("taxRate"), V); P.TaxRate = (int32)V;
    O->TryGetStringField(TEXT("positionName"), P.PositionName);
    O->TryGetBoolField(TEXT("canInvite"), P.bCanInvite);
    O->TryGetBoolField(TEXT("canKick"), P.bCanKick);
    O->TryGetBoolField(TEXT("canStorage"), P.bCanStorage);
    return P;
}

// --- Widget ---

void UGuildSubsystem::ToggleWidget() { if (bWidgetAdded) HideWidget(); else ShowWidget(); }

void UGuildSubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* W = GetWorld(); if (!W) return;
    UGameViewportClient* VC = W->GetGameViewport(); if (!VC) return;
    GuildWidget = SNew(SGuildWidget).Subsystem(this);
    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(GuildWidget);
    VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 13);
    bWidgetAdded = true;
}

void UGuildSubsystem::HideWidget()
{
    if (!bWidgetAdded) return;
    if (UWorld* W = GetWorld())
        if (UGameViewportClient* VC = W->GetGameViewport())
            if (ViewportOverlay.IsValid()) VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
    GuildWidget.Reset(); ViewportOverlay.Reset(); bWidgetAdded = false;
}

bool UGuildSubsystem::IsWidgetVisible() const { return bWidgetAdded; }
```

### 6.4 SGuildWidget Header

File: `client/SabriMMO/Source/SabriMMO/UI/SGuildWidget.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UGuildSubsystem;

/** Tabbed guild window: Info | Members | Positions | Skills | Storage */
class SGuildWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGuildWidget) {}
        SLATE_ARGUMENT(UGuildSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UGuildSubsystem> OwningSubsystem;

    // Active tab index (0=Info, 1=Members, 2=Positions, 3=Skills, 4=Storage)
    int32 ActiveTab = 0;

    // Drag
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D WidgetPosition = FVector2D(300.0, 100.0);

    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent&) override;

    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildTabBar();
    TSharedRef<SWidget> BuildInfoTab();
    TSharedRef<SWidget> BuildMembersTab();
    TSharedRef<SWidget> BuildPositionsTab();
    TSharedRef<SWidget> BuildSkillsTab();
    TSharedRef<SWidget> BuildStorageTab();

    TSharedPtr<SBox> RootSizeBox;
    void ApplyLayout();
};
```

The SGuildWidget implementation follows the same pattern as SPartyWidget: lambda-driven text blocks, progress bars for guild EXP, a scrollable member list, and tab switching via `ActiveTab`. Each tab calls the appropriate `Build*Tab()` method, and the widget rebuilds its visible content when `OnGuildUpdated` fires.

---

## 7. Chat System

### 7.1 Server -- Expanded Chat Handler

Replace the existing minimal `chat:message` handler in `index.js`:

```javascript
// ============================================================
// CHAT SYSTEM -- Full multi-channel chat
// ============================================================

// Emote definitions
const EMOTES = {
    '/smile': ':)', '/laugh': 'XD', '/cry': 'T_T', '/angry': '>:(', '/wave': 'o/',
    '/sit': '(sits down)', '/stand': '(stands up)', '/bow': '(bows)', '/no': '(shakes head)',
    '/yes': '(nods)', '/gg': 'GG', '/thx': 'Thanks!', '/lol': 'LOL'
};

socket.on('chat:message', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return socket.emit('chat:error', { message: 'Not logged in.' });
    const player = connectedPlayers.get(characterId);
    if (!player) return;

    let { channel, message, targetName } = data;
    message = (message || '').trim().substring(0, 255);
    if (!message) return;

    // Parse slash commands
    if (message.startsWith('/')) {
        const parts = message.split(' ');
        const cmd = parts[0].toLowerCase();
        const rest = parts.slice(1).join(' ');

        // Emotes
        if (EMOTES[cmd]) {
            const emoteMsg = {
                type: 'chat:receive', channel: 'EMOTE',
                senderId: characterId, senderName: player.characterName,
                message: `${player.characterName} ${EMOTES[cmd]}`,
                timestamp: Date.now()
            };
            broadcastToZone(player.zone || 'prontera_south', 'chat:receive', emoteMsg);
            return;
        }

        // Whisper: /w "name" message  OR  /whisper name message
        if (cmd === '/w' || cmd === '/whisper') {
            channel = 'WHISPER';
            // Parse target name (quoted or first word)
            let whisperTarget, whisperMsg;
            if (rest.startsWith('"')) {
                const endQuote = rest.indexOf('"', 1);
                if (endQuote > 1) {
                    whisperTarget = rest.substring(1, endQuote);
                    whisperMsg = rest.substring(endQuote + 1).trim();
                }
            }
            if (!whisperTarget) {
                whisperTarget = parts[1] || '';
                whisperMsg = parts.slice(2).join(' ');
            }
            targetName = whisperTarget;
            message = whisperMsg || '';
            if (!message) return;
        }

        // Party chat: /p message
        if (cmd === '/p' || cmd === '/party') { channel = 'PARTY'; message = rest; }

        // Guild chat: /g message
        if (cmd === '/g' || cmd === '/guild') { channel = 'GUILD'; message = rest; }

        // Trade shout: /trade message
        if (cmd === '/trade') { channel = 'TRADE'; message = rest; }

        // Party creation: /organize "name"
        if (cmd === '/organize') {
            socket.emit('party:create', { partyName: rest.replace(/"/g, ''), itemShareMode: 'each_take' });
            return;
        }

        // Party invite: /invite name
        if (cmd === '/invite') {
            socket.emit('party:invite', { targetName: rest.trim() });
            return;
        }

        // Guild creation: /guild "name" (when not in guild context)
        if (cmd === '/guild' && !rest && !playerGuildMap.has(characterId)) {
            // This would clash with guild chat; handled by UI
            return;
        }

        if (!message) return;
    }

    channel = channel || 'PUBLIC';

    const chatMsg = {
        type: 'chat:receive',
        channel,
        senderId: characterId,
        senderName: player.characterName,
        message,
        timestamp: Date.now()
    };

    switch (channel) {
        case 'PUBLIC':
            // Zone-local chat (visible to same map only)
            broadcastToZone(player.zone || 'prontera_south', 'chat:receive', chatMsg);
            logger.info(`[CHAT PUBLIC] ${player.characterName}: ${message}`);
            break;

        case 'WHISPER': {
            const tn = (targetName || '').trim();
            let targetId = null, targetP = null;
            for (const [cid, p] of connectedPlayers.entries()) {
                if (p.characterName && p.characterName.toLowerCase() === tn.toLowerCase()) {
                    targetId = cid; targetP = p; break;
                }
            }
            if (!targetP) return socket.emit('chat:error', { message: `${tn} is not online.` });

            // Check block list (DB check or in-memory if loaded)
            // For now, skip block check (implemented in section 8)

            const targetSocket = io.sockets.sockets.get(targetP.socketId);
            if (targetSocket) targetSocket.emit('chat:receive', chatMsg);
            // Echo back to sender with channel WHISPER_SENT
            socket.emit('chat:receive', { ...chatMsg, channel: 'WHISPER_SENT', targetName: targetP.characterName });
            logger.info(`[CHAT WHISPER] ${player.characterName} -> ${targetP.characterName}: ${message}`);
            break;
        }

        case 'PARTY': {
            const pId = playerPartyMap.get(characterId);
            if (!pId) return socket.emit('chat:error', { message: 'Not in a party.' });
            emitToParty(pId, 'chat:receive', chatMsg);
            logger.info(`[CHAT PARTY] ${player.characterName}: ${message}`);
            break;
        }

        case 'GUILD': {
            const gId = playerGuildMap.get(characterId);
            if (!gId) return socket.emit('chat:error', { message: 'Not in a guild.' });
            emitToGuild(gId, 'chat:receive', chatMsg);
            logger.info(`[CHAT GUILD] ${player.characterName}: ${message}`);
            break;
        }

        case 'GLOBAL':
            io.emit('chat:receive', chatMsg);
            logger.info(`[CHAT GLOBAL] ${player.characterName}: ${message}`);
            break;

        case 'TRADE':
            io.emit('chat:receive', chatMsg);
            logger.info(`[CHAT TRADE] ${player.characterName}: ${message}`);
            break;

        default:
            broadcastToZone(player.zone || 'prontera_south', 'chat:receive', chatMsg);
    }
});
```

### 7.2 Client -- UChatSubsystem Header

File: `client/SabriMMO/Source/SabriMMO/UI/ChatSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "ChatSubsystem.generated.h"

class USocketIOClientComponent;
class SChatWidget;

UENUM()
enum class EChatChannel : uint8
{
    Public,
    Whisper,
    WhisperSent,
    Party,
    Guild,
    Global,
    System,
    Combat,
    Trade,
    Emote
};

USTRUCT()
struct FChatMessage
{
    GENERATED_BODY()

    EChatChannel Channel = EChatChannel::Public;
    int32 SenderId = 0;
    FString SenderName;
    FString Message;
    FString TargetName;   // for whisper
    int64 Timestamp = 0;

    FLinearColor GetChannelColor() const
    {
        switch (Channel)
        {
            case EChatChannel::Public:      return FLinearColor::White;
            case EChatChannel::Whisper:      return FLinearColor(1.f, 0.6f, 0.8f);      // pink
            case EChatChannel::WhisperSent:  return FLinearColor(0.8f, 0.5f, 0.7f);
            case EChatChannel::Party:        return FLinearColor(0.3f, 1.f, 0.3f);       // green
            case EChatChannel::Guild:        return FLinearColor(0.4f, 0.8f, 1.f);       // light blue
            case EChatChannel::Global:       return FLinearColor(1.f, 1.f, 0.4f);        // yellow
            case EChatChannel::System:       return FLinearColor(1.f, 0.85f, 0.f);       // gold
            case EChatChannel::Combat:       return FLinearColor(0.8f, 0.3f, 0.3f);      // red
            case EChatChannel::Trade:        return FLinearColor(0.7f, 0.9f, 0.5f);      // lime
            case EChatChannel::Emote:        return FLinearColor(0.9f, 0.7f, 0.3f);      // orange
            default: return FLinearColor::White;
        }
    }

    FString GetChannelPrefix() const
    {
        switch (Channel)
        {
            case EChatChannel::Whisper:      return FString::Printf(TEXT("[From %s] "), *SenderName);
            case EChatChannel::WhisperSent:  return FString::Printf(TEXT("[To %s] "), *TargetName);
            case EChatChannel::Party:        return TEXT("[Party] ");
            case EChatChannel::Guild:        return TEXT("[Guild] ");
            case EChatChannel::Global:       return TEXT("[Global] ");
            case EChatChannel::System:       return TEXT("[System] ");
            case EChatChannel::Combat:       return TEXT("");
            case EChatChannel::Trade:        return TEXT("[Trade] ");
            case EChatChannel::Emote:        return TEXT("");
            default: return TEXT("");
        }
    }
};

UCLASS()
class SABRIMMO_API UChatSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Chat history (ring buffer, max 200 messages)
    static constexpr int32 MAX_CHAT_HISTORY = 200;
    TArray<FChatMessage> ChatHistory;

    // Active channel for sending
    EChatChannel ActiveSendChannel = EChatChannel::Public;
    FString WhisperTargetName;

    // Channel visibility filters
    TMap<EChatChannel, bool> ChannelFilters;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&);
    FOnChatMessageReceived OnChatMessageReceived;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // Send a message using the active channel
    void SendMessage(const FString& Message);

    // Direct send to specific channel
    void SendWhisper(const FString& TargetName, const FString& Message);
    void SendToChannel(EChatChannel Channel, const FString& Message);

    // Add a local system message (not sent to server)
    void AddLocalMessage(const FString& Message, EChatChannel Channel = EChatChannel::System);

    // Widget
    void ToggleWidget();
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

private:
    void TryWrapSocketEvents();
    USocketIOClientComponent* FindSocketIOComponent() const;
    void EmitToServer(const FString& Event, const FString& Payload);

    void HandleChatReceive(const TSharedPtr<FJsonValue>& Data);

    EChatChannel ParseChannel(const FString& ChannelStr) const;

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;
    int32 LocalCharacterId = 0;

    TSharedPtr<SChatWidget> ChatWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 7.3 UChatSubsystem Implementation

File: `client/SabriMMO/Source/SabriMMO/UI/ChatSubsystem.cpp`

```cpp
#include "ChatSubsystem.h"
#include "SChatWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogChat, Log, All);

bool UChatSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* W = Cast<UWorld>(Outer);
    return W && W->IsGameWorld();
}

void UChatSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
        LocalCharacterId = GI->GetSelectedCharacter().CharacterId;

    // Default: all channels visible
    ChannelFilters.Add(EChatChannel::Public, true);
    ChannelFilters.Add(EChatChannel::Whisper, true);
    ChannelFilters.Add(EChatChannel::WhisperSent, true);
    ChannelFilters.Add(EChatChannel::Party, true);
    ChannelFilters.Add(EChatChannel::Guild, true);
    ChannelFilters.Add(EChatChannel::Global, true);
    ChannelFilters.Add(EChatChannel::System, true);
    ChannelFilters.Add(EChatChannel::Combat, true);
    ChannelFilters.Add(EChatChannel::Trade, true);
    ChannelFilters.Add(EChatChannel::Emote, true);

    InWorld.GetTimerManager().SetTimer(BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UChatSubsystem::TryWrapSocketEvents),
        0.5f, true);
}

void UChatSubsystem::Deinitialize()
{
    HideWidget();
    if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(BindCheckTimer);
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

USocketIOClientComponent* UChatSubsystem::FindSocketIOComponent() const
{
    UWorld* W = GetWorld(); if (!W) return nullptr;
    for (TActorIterator<AActor> It(W); It; ++It)
        if (auto* C = It->FindComponentByClass<USocketIOClientComponent>()) return C;
    return nullptr;
}

void UChatSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;
    TSharedPtr<FSocketIONative> Native = SIO->GetNativeClient();
    if (!Native.IsValid() || !Native->bIsConnected) return;
    CachedSIOComponent = SIO;

    // Wrap existing chat:receive (may have BP bindings from BP_SocketManager)
    // Use the WrapSingleEvent pattern from BasicInfoSubsystem
    TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OrigCallback;
    FSIOBoundEvent* Existing = Native->EventFunctionMap.Find(TEXT("chat:receive"));
    if (Existing) OrigCallback = Existing->Function;

    Native->OnEvent(TEXT("chat:receive"),
        [this, OrigCallback](const FString& Ev, const TSharedPtr<FJsonValue>& Msg) {
            if (OrigCallback) OrigCallback(Ev, Msg);
            HandleChatReceive(Msg);
        },
        TEXT("/"), ESIOThreadOverrideOption::USE_GAME_THREAD);

    // Also wrap chat:error
    Native->OnEvent(TEXT("chat:error"),
        [this](const FString&, const TSharedPtr<FJsonValue>& Msg) {
            if (!Msg.IsValid()) return;
            const TSharedPtr<FJsonObject>* O; if (!Msg->TryGetObject(O)) return;
            FString ErrMsg; (*O)->TryGetStringField(TEXT("message"), ErrMsg);
            AddLocalMessage(ErrMsg, EChatChannel::System);
        },
        TEXT("/"), ESIOThreadOverrideOption::USE_GAME_THREAD);

    bEventsWrapped = true;
    if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(BindCheckTimer);
    ShowWidget();  // Chat is always visible
    UE_LOG(LogChat, Log, TEXT("ChatSubsystem -- bound."));
}

void UChatSubsystem::EmitToServer(const FString& Event, const FString& Payload)
{
    if (CachedSIOComponent.IsValid()) CachedSIOComponent->EmitNative(Event, Payload);
}

void UChatSubsystem::HandleChatReceive(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* O; if (!Data->TryGetObject(O) || !O) return;
    const TSharedPtr<FJsonObject>& Obj = *O;

    FChatMessage Msg;
    FString ChStr; Obj->TryGetStringField(TEXT("channel"), ChStr);
    Msg.Channel = ParseChannel(ChStr);

    double V;
    Obj->TryGetNumberField(TEXT("senderId"), V); Msg.SenderId = (int32)V;
    Obj->TryGetStringField(TEXT("senderName"), Msg.SenderName);
    Obj->TryGetStringField(TEXT("message"), Msg.Message);
    Obj->TryGetStringField(TEXT("targetName"), Msg.TargetName);
    Obj->TryGetNumberField(TEXT("timestamp"), V); Msg.Timestamp = (int64)V;

    // Ring buffer
    if (ChatHistory.Num() >= MAX_CHAT_HISTORY) ChatHistory.RemoveAt(0);
    ChatHistory.Add(Msg);

    OnChatMessageReceived.Broadcast(Msg);
}

EChatChannel UChatSubsystem::ParseChannel(const FString& S) const
{
    if (S == TEXT("PUBLIC"))       return EChatChannel::Public;
    if (S == TEXT("WHISPER"))      return EChatChannel::Whisper;
    if (S == TEXT("WHISPER_SENT")) return EChatChannel::WhisperSent;
    if (S == TEXT("PARTY"))        return EChatChannel::Party;
    if (S == TEXT("GUILD"))        return EChatChannel::Guild;
    if (S == TEXT("GLOBAL"))       return EChatChannel::Global;
    if (S == TEXT("SYSTEM"))       return EChatChannel::System;
    if (S == TEXT("COMBAT"))       return EChatChannel::Combat;
    if (S == TEXT("TRADE"))        return EChatChannel::Trade;
    if (S == TEXT("EMOTE"))        return EChatChannel::Emote;
    return EChatChannel::Public;
}

void UChatSubsystem::SendMessage(const FString& RawMessage)
{
    FString Message = RawMessage.TrimStartAndEnd();
    if (Message.IsEmpty()) return;

    // Slash command detection (let server handle parsing)
    if (Message.StartsWith(TEXT("/")))
    {
        // Detect /w for whisper shortcut on client side
        if (Message.StartsWith(TEXT("/w ")) || Message.StartsWith(TEXT("/whisper ")))
        {
            EmitToServer(TEXT("chat:message"), FString::Printf(
                TEXT("{\"channel\":\"WHISPER\",\"message\":\"%s\"}"),
                *Message.Replace(TEXT("\""), TEXT("\\\""))));
            return;
        }
        // Send raw slash command as PUBLIC channel; server parses it
        EmitToServer(TEXT("chat:message"), FString::Printf(
            TEXT("{\"channel\":\"PUBLIC\",\"message\":\"%s\"}"),
            *Message.Replace(TEXT("\""), TEXT("\\\""))));
        return;
    }

    FString ChannelStr;
    switch (ActiveSendChannel)
    {
        case EChatChannel::Public:  ChannelStr = TEXT("PUBLIC");  break;
        case EChatChannel::Party:   ChannelStr = TEXT("PARTY");   break;
        case EChatChannel::Guild:   ChannelStr = TEXT("GUILD");   break;
        case EChatChannel::Global:  ChannelStr = TEXT("GLOBAL");  break;
        case EChatChannel::Trade:   ChannelStr = TEXT("TRADE");   break;
        case EChatChannel::Whisper:
            ChannelStr = TEXT("WHISPER");
            EmitToServer(TEXT("chat:message"), FString::Printf(
                TEXT("{\"channel\":\"WHISPER\",\"message\":\"%s\",\"targetName\":\"%s\"}"),
                *Message.Replace(TEXT("\""), TEXT("\\\"")).Left(255),
                *WhisperTargetName.Replace(TEXT("\""), TEXT(""))));
            return;
        default: ChannelStr = TEXT("PUBLIC");
    }

    EmitToServer(TEXT("chat:message"), FString::Printf(
        TEXT("{\"channel\":\"%s\",\"message\":\"%s\"}"),
        *ChannelStr, *Message.Replace(TEXT("\""), TEXT("\\\"")).Left(255)));
}

void UChatSubsystem::SendWhisper(const FString& Target, const FString& Msg)
{
    EmitToServer(TEXT("chat:message"), FString::Printf(
        TEXT("{\"channel\":\"WHISPER\",\"message\":\"%s\",\"targetName\":\"%s\"}"),
        *Msg.Replace(TEXT("\""), TEXT("\\\"")).Left(255),
        *Target.Replace(TEXT("\""), TEXT(""))));
}

void UChatSubsystem::SendToChannel(EChatChannel Ch, const FString& Msg)
{
    EChatChannel Prev = ActiveSendChannel;
    ActiveSendChannel = Ch;
    SendMessage(Msg);
    ActiveSendChannel = Prev;
}

void UChatSubsystem::AddLocalMessage(const FString& Msg, EChatChannel Ch)
{
    FChatMessage M;
    M.Channel = Ch;
    M.SenderName = TEXT("System");
    M.Message = Msg;
    M.Timestamp = FDateTime::UtcNow().ToUnixTimestamp() * 1000;

    if (ChatHistory.Num() >= MAX_CHAT_HISTORY) ChatHistory.RemoveAt(0);
    ChatHistory.Add(M);
    OnChatMessageReceived.Broadcast(M);
}

// Widget management
void UChatSubsystem::ToggleWidget() { if (bWidgetAdded) HideWidget(); else ShowWidget(); }

void UChatSubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* W = GetWorld(); if (!W) return;
    UGameViewportClient* VC = W->GetGameViewport(); if (!VC) return;
    ChatWidget = SNew(SChatWidget).Subsystem(this);
    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(ChatWidget);
    VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 5);
    bWidgetAdded = true;
}

void UChatSubsystem::HideWidget()
{
    if (!bWidgetAdded) return;
    if (UWorld* W = GetWorld())
        if (UGameViewportClient* VC = W->GetGameViewport())
            if (ViewportOverlay.IsValid()) VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
    ChatWidget.Reset(); ViewportOverlay.Reset(); bWidgetAdded = false;
}

bool UChatSubsystem::IsWidgetVisible() const { return bWidgetAdded; }
```

### 7.4 SChatWidget Header

File: `client/SabriMMO/Source/SabriMMO/UI/SChatWidget.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UChatSubsystem;

class SChatWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SChatWidget) {}
        SLATE_ARGUMENT(UChatSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UChatSubsystem> OwningSubsystem;

    // Input field text
    FText CurrentInputText;

    // Drag
    bool bIsDragging = false;
    FVector2D DragOffset;
    FVector2D WidgetPosition = FVector2D(10.0, 500.0);

    // Scroll
    TSharedPtr<SScrollBox> MessageScrollBox;

    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent&) override;

    void OnInputCommitted(const FText& Text, ETextCommit::Type CommitType);
    void OnNewMessage(const struct FChatMessage& Msg);
    void AppendMessageToScroll(const struct FChatMessage& Msg);

    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildChannelTabs();
    TSharedRef<SWidget> BuildInputRow();

    FDelegateHandle MessageDelegateHandle;
    void ApplyLayout();
    TSharedPtr<SBox> RootSizeBox;
};
```

The SChatWidget layout is:
- Title bar (draggable, "Chat" label, minimize button)
- Channel filter tabs (Public / Party / Guild / Global / Trade / Whisper -- toggle visibility)
- Scrollable message area (each message colored by channel, with prefix)
- Input row: channel indicator dropdown + text input + send button
- When Enter is pressed, calls `UChatSubsystem::SendMessage()`
- New messages auto-scroll to bottom via `MessageScrollBox->ScrollToEnd()`

---

## 8. Friend List

### 8.1 Server -- Friend/Block Handlers

```javascript
// ============================================================
// FRIEND / BLOCK SYSTEM
// ============================================================

// In-memory: characterId -> Set<friendId>
const friendLists = new Map();
const blockLists = new Map();
const pendingFriendRequests = new Map(); // targetId -> { fromId, fromName, timestamp }

// Load on player:join
async function loadFriendsForPlayer(characterId) {
    const rows = await pool.query(
        'SELECT friend_id FROM friends WHERE character_id = $1', [characterId]);
    const set = new Set();
    for (const r of rows.rows) set.add(r.friend_id);
    friendLists.set(characterId, set);

    const bRows = await pool.query(
        'SELECT blocked_id FROM blocked_players WHERE character_id = $1', [characterId]);
    const bSet = new Set();
    for (const r of bRows.rows) bSet.add(r.blocked_id);
    blockLists.set(characterId, bSet);
}

// Send friend list on login
function sendFriendList(socket, characterId) {
    const friends = friendLists.get(characterId) || new Set();
    const list = [];
    for (const fid of friends) {
        const player = connectedPlayers.get(fid);
        list.push({
            friendId: fid,
            friendName: player ? player.characterName : '(offline)',
            isOnline: !!player
        });
    }
    socket.emit('friend:list', { friends: list });
}

function sendBlockList(socket, characterId) {
    const blocked = blockLists.get(characterId) || new Set();
    const list = [];
    for (const bid of blocked) {
        list.push({ blockedId: bid, blockedName: '(lookup)' }); // name lookup from DB on demand
    }
    socket.emit('block:list', { blocked: list });
}

// Socket handlers (inside io.on('connection'))

socket.on('friend:add', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const player = connectedPlayers.get(characterId);
    if (!player) return;

    const targetName = (data.targetName || '').trim();
    let targetId = null, targetP = null;
    for (const [cid, p] of connectedPlayers.entries()) {
        if (p.characterName?.toLowerCase() === targetName.toLowerCase()) {
            targetId = cid; targetP = p; break;
        }
    }
    if (!targetId) return socket.emit('chat:receive', {
        type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
        message: `${targetName} is not online.`, timestamp: Date.now()
    });

    const friends = friendLists.get(characterId) || new Set();
    if (friends.has(targetId)) return socket.emit('chat:receive', {
        type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
        message: `${targetP.characterName} is already your friend.`, timestamp: Date.now()
    });

    pendingFriendRequests.set(targetId, { fromId: characterId, fromName: player.characterName, timestamp: Date.now() });
    setTimeout(() => {
        const req = pendingFriendRequests.get(targetId);
        if (req && req.fromId === characterId) pendingFriendRequests.delete(targetId);
    }, 30000);

    const targetSocket = io.sockets.sockets.get(targetP.socketId);
    if (targetSocket) targetSocket.emit('friend:request', { fromId: characterId, fromName: player.characterName });
});

socket.on('friend:respond', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;

    const req = pendingFriendRequests.get(characterId);
    if (!req || req.fromId !== data.fromId) return;
    pendingFriendRequests.delete(characterId);
    if (!data.accept) return;

    const fromId = req.fromId;
    try {
        await pool.query('INSERT INTO friends (character_id, friend_id) VALUES ($1, $2), ($2, $1) ON CONFLICT DO NOTHING',
            [characterId, fromId]);

        // Update in-memory
        if (!friendLists.has(characterId)) friendLists.set(characterId, new Set());
        if (!friendLists.has(fromId)) friendLists.set(fromId, new Set());
        friendLists.get(characterId).add(fromId);
        friendLists.get(fromId).add(characterId);

        const player = connectedPlayers.get(characterId);
        const fromPlayer = connectedPlayers.get(fromId);

        socket.emit('friend:added', {
            friendId: fromId, friendName: fromPlayer?.characterName || req.fromName, isOnline: !!fromPlayer
        });

        if (fromPlayer) {
            const fromSocket = io.sockets.sockets.get(fromPlayer.socketId);
            if (fromSocket) fromSocket.emit('friend:added', {
                friendId: characterId, friendName: player?.characterName || '', isOnline: true
            });
        }
    } catch (err) {
        logger.error(`[FRIEND] Add failed: ${err.message}`);
    }
});

socket.on('friend:remove', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const friendId = parseInt(data.friendId);

    await pool.query('DELETE FROM friends WHERE (character_id=$1 AND friend_id=$2) OR (character_id=$2 AND friend_id=$1)',
        [characterId, friendId]);

    const myFriends = friendLists.get(characterId);
    if (myFriends) myFriends.delete(friendId);
    const theirFriends = friendLists.get(friendId);
    if (theirFriends) theirFriends.delete(characterId);

    socket.emit('friend:removed', { friendId });
    const other = connectedPlayers.get(friendId);
    if (other) {
        const os = io.sockets.sockets.get(other.socketId);
        if (os) os.emit('friend:removed', { friendId: characterId });
    }
});

// Block handlers
socket.on('block:add', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const targetName = (data.targetName || '').trim();

    // Look up character by name (may be offline)
    const result = await pool.query('SELECT character_id, name FROM characters WHERE LOWER(name) = LOWER($1) AND deleted = FALSE', [targetName]);
    if (result.rows.length === 0) return socket.emit('chat:error', { message: `Player "${targetName}" not found.` });

    const blockedId = result.rows[0].character_id;
    await pool.query('INSERT INTO blocked_players (character_id, blocked_id) VALUES ($1, $2) ON CONFLICT DO NOTHING',
        [characterId, blockedId]);

    if (!blockLists.has(characterId)) blockLists.set(characterId, new Set());
    blockLists.get(characterId).add(blockedId);

    socket.emit('block:added', { blockedId, blockedName: result.rows[0].name });
});

socket.on('block:remove', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const blockedId = parseInt(data.blockedId);

    await pool.query('DELETE FROM blocked_players WHERE character_id=$1 AND blocked_id=$2', [characterId, blockedId]);

    const bSet = blockLists.get(characterId);
    if (bSet) bSet.delete(blockedId);

    socket.emit('block:removed', { blockedId });
});

// Notify friends on login/logout (add to player:join / disconnect handlers)
function notifyFriendsOnlineStatus(characterId, isOnline) {
    const friends = friendLists.get(characterId);
    if (!friends) return;
    for (const fid of friends) {
        const fp = connectedPlayers.get(fid);
        if (fp) {
            const fs = io.sockets.sockets.get(fp.socketId);
            if (fs) fs.emit('friend:status', { friendId: characterId, isOnline });
        }
    }
}
```

### 8.2 Client -- Friend list is managed within the Chat subsystem or a small dedicated `UFriendSubsystem` following the identical pattern. The struct:

```cpp
USTRUCT()
struct FFriendEntry
{
    GENERATED_BODY()

    int32 FriendId = 0;
    FString FriendName;
    bool bIsOnline = false;
};
```

The friend list UI is either a tab within SChatWidget or a standalone panel toggled with a key.

---

## 9. Trade System

### 9.1 Server -- Trade State and Handlers

```javascript
// ============================================================
// TRADE SYSTEM
// ============================================================

const MAX_TRADE_SLOTS = 10;

// characterId -> trade session
const activeTrades = new Map();

// characterId -> { fromId, fromName, timestamp }
const pendingTradeRequests = new Map();

function getTradePartner(characterId) {
    const trade = activeTrades.get(characterId);
    if (!trade) return null;
    return trade.partnerId;
}

function emitToBothTraders(trade, event, payload) {
    for (const cid of [trade.playerA, trade.playerB]) {
        const p = connectedPlayers.get(cid);
        if (p) {
            const s = io.sockets.sockets.get(p.socketId);
            if (s) s.emit(event, payload);
        }
    }
}

function cancelTrade(characterId, reason) {
    const trade = activeTrades.get(characterId);
    if (!trade) return;
    const partnerId = trade.playerA === characterId ? trade.playerB : trade.playerA;

    emitToBothTraders(trade, 'trade:cancelled', { reason });
    activeTrades.delete(trade.playerA);
    activeTrades.delete(trade.playerB);
}

// Socket handlers

socket.on('trade:request', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    if (activeTrades.has(characterId)) return socket.emit('trade:error', { message: 'Already in a trade.' });

    const targetId = parseInt(data.targetId);
    const target = connectedPlayers.get(targetId);
    if (!target) return socket.emit('trade:error', { message: 'Player not found.' });
    if (activeTrades.has(targetId)) return socket.emit('trade:error', { message: 'That player is already trading.' });

    const player = connectedPlayers.get(characterId);
    if (player.zone !== target.zone) return socket.emit('trade:error', { message: 'Must be on the same map.' });

    pendingTradeRequests.set(targetId, { fromId: characterId, fromName: player.characterName, timestamp: Date.now() });
    setTimeout(() => {
        const req = pendingTradeRequests.get(targetId);
        if (req && req.fromId === characterId) pendingTradeRequests.delete(targetId);
    }, 15000);

    const ts = io.sockets.sockets.get(target.socketId);
    if (ts) ts.emit('trade:request_received', { fromId: characterId, fromName: player.characterName });
});

socket.on('trade:respond', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;

    const req = pendingTradeRequests.get(characterId);
    if (!req || req.fromId !== data.fromId) return socket.emit('trade:error', { message: 'No pending request.' });
    pendingTradeRequests.delete(characterId);

    if (!data.accept) {
        const fromP = connectedPlayers.get(req.fromId);
        if (fromP) {
            const fs = io.sockets.sockets.get(fromP.socketId);
            if (fs) fs.emit('trade:cancelled', { reason: 'Request declined.' });
        }
        return;
    }

    if (activeTrades.has(characterId) || activeTrades.has(req.fromId)) {
        return socket.emit('trade:error', { message: 'One of you is already in a trade.' });
    }

    const trade = {
        playerA: req.fromId,
        playerB: characterId,
        slotsA: {},  // slot (0-9) -> { inventoryId, itemId, itemName, icon, quantity }
        slotsB: {},
        zenyA: 0,
        zenyB: 0,
        lockedA: false,
        lockedB: false,
        confirmedA: false,
        confirmedB: false
    };

    activeTrades.set(req.fromId, trade);
    activeTrades.set(characterId, trade);

    const playerA = connectedPlayers.get(req.fromId);
    const playerB = connectedPlayers.get(characterId);

    const sA = io.sockets.sockets.get(playerA.socketId);
    const sB = io.sockets.sockets.get(playerB.socketId);
    if (sA) sA.emit('trade:started', { partnerId: characterId, partnerName: playerB.characterName });
    if (sB) sB.emit('trade:started', { partnerId: req.fromId, partnerName: playerA.characterName });
});

socket.on('trade:add_item', async (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const trade = activeTrades.get(characterId);
    if (!trade) return socket.emit('trade:error', { message: 'Not in a trade.' });

    const isA = characterId === trade.playerA;
    if ((isA && trade.lockedA) || (!isA && trade.lockedB)) {
        return socket.emit('trade:error', { message: 'Trade is locked.' });
    }

    // Reset confirms on item change
    trade.confirmedA = false; trade.confirmedB = false;
    trade.lockedA = false; trade.lockedB = false;

    const slots = isA ? trade.slotsA : trade.slotsB;
    const usedSlots = Object.keys(slots).length;
    if (usedSlots >= MAX_TRADE_SLOTS) return socket.emit('trade:error', { message: 'Trade slots full (max 10).' });

    const invId = parseInt(data.inventoryId);
    const qty = Math.max(1, parseInt(data.quantity) || 1);

    // Verify ownership
    const invRow = await pool.query(`
        SELECT ci.*, i.name, i.icon, i.stackable FROM character_inventory ci
        JOIN items i ON i.item_id = ci.item_id
        WHERE ci.inventory_id = $1 AND ci.character_id = $2
    `, [invId, characterId]);
    if (invRow.rows.length === 0) return socket.emit('trade:error', { message: 'Item not found.' });

    const item = invRow.rows[0];
    const actualQty = Math.min(qty, item.quantity);

    // Check not already in trade
    for (const s of Object.values(slots)) {
        if (s.inventoryId === invId) return socket.emit('trade:error', { message: 'Item already in trade.' });
    }

    const slot = usedSlots; // next available
    slots[slot] = {
        inventoryId: invId, itemId: item.item_id,
        itemName: item.name, icon: item.icon, quantity: actualQty
    };

    const side = isA ? 'A' : 'B';
    emitToBothTraders(trade, 'trade:item_added', {
        side, slot, inventoryId: invId, itemId: item.item_id,
        itemName: item.name, icon: item.icon, quantity: actualQty
    });
});

socket.on('trade:remove_item', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const trade = activeTrades.get(characterId);
    if (!trade) return;

    const isA = characterId === trade.playerA;
    if ((isA && trade.lockedA) || (!isA && trade.lockedB)) return;

    trade.confirmedA = false; trade.confirmedB = false;
    trade.lockedA = false; trade.lockedB = false;

    const slots = isA ? trade.slotsA : trade.slotsB;
    const slot = parseInt(data.slot);
    delete slots[slot];

    emitToBothTraders(trade, 'trade:item_removed', { side: isA ? 'A' : 'B', slot });
});

socket.on('trade:set_zeny', (data) => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const trade = activeTrades.get(characterId);
    if (!trade) return;

    const isA = characterId === trade.playerA;
    if ((isA && trade.lockedA) || (!isA && trade.lockedB)) return;

    trade.confirmedA = false; trade.confirmedB = false;
    trade.lockedA = false; trade.lockedB = false;

    const player = connectedPlayers.get(characterId);
    const amount = Math.max(0, Math.min(parseInt(data.amount) || 0, player.zuzucoin || 0));

    if (isA) trade.zenyA = amount; else trade.zenyB = amount;

    emitToBothTraders(trade, 'trade:zeny_set', { side: isA ? 'A' : 'B', amount });
});

socket.on('trade:lock', () => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const trade = activeTrades.get(characterId);
    if (!trade) return;

    const isA = characterId === trade.playerA;
    if (isA) trade.lockedA = true; else trade.lockedB = true;

    emitToBothTraders(trade, 'trade:locked', { side: isA ? 'A' : 'B' });
});

socket.on('trade:confirm', async () => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    const trade = activeTrades.get(characterId);
    if (!trade) return;

    const isA = characterId === trade.playerA;
    // Must be locked first
    if ((isA && !trade.lockedA) || (!isA && !trade.lockedB)) {
        return socket.emit('trade:error', { message: 'Lock your trade first.' });
    }
    if (!trade.lockedA || !trade.lockedB) {
        return socket.emit('trade:error', { message: 'Both players must lock before confirming.' });
    }

    if (isA) trade.confirmedA = true; else trade.confirmedB = true;

    if (trade.confirmedA && trade.confirmedB) {
        // Execute atomic trade
        await executeAtomicTrade(trade);
    }
});

socket.on('trade:cancel', () => {
    const characterId = findCharacterIdBySocket(socket.id);
    if (!characterId) return;
    cancelTrade(characterId, 'Cancelled by player.');
});

// Clean up trade on disconnect (add to disconnect handler)
// cancelTrade(characterId, 'Player disconnected.');
```

### 9.2 Atomic Trade Execution

```javascript
async function executeAtomicTrade(trade) {
    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // Transfer items A -> B
        for (const slot of Object.values(trade.slotsA)) {
            await client.query(
                'UPDATE character_inventory SET character_id = $1 WHERE inventory_id = $2 AND character_id = $3',
                [trade.playerB, slot.inventoryId, trade.playerA]
            );
        }

        // Transfer items B -> A
        for (const slot of Object.values(trade.slotsB)) {
            await client.query(
                'UPDATE character_inventory SET character_id = $1 WHERE inventory_id = $2 AND character_id = $3',
                [trade.playerA, slot.inventoryId, trade.playerB]
            );
        }

        // Transfer Zeny
        if (trade.zenyA > 0) {
            await client.query('UPDATE characters SET zuzucoin = zuzucoin - $1 WHERE character_id = $2', [trade.zenyA, trade.playerA]);
            await client.query('UPDATE characters SET zuzucoin = zuzucoin + $1 WHERE character_id = $2', [trade.zenyA, trade.playerB]);
        }
        if (trade.zenyB > 0) {
            await client.query('UPDATE characters SET zuzucoin = zuzucoin - $1 WHERE character_id = $2', [trade.zenyB, trade.playerB]);
            await client.query('UPDATE characters SET zuzucoin = zuzucoin + $1 WHERE character_id = $2', [trade.zenyB, trade.playerA]);
        }

        // Audit log
        await client.query(`
            INSERT INTO trade_log (player_a_id, player_b_id, items_a, items_b, zeny_a, zeny_b)
            VALUES ($1, $2, $3, $4, $5, $6)
        `, [
            trade.playerA, trade.playerB,
            JSON.stringify(Object.values(trade.slotsA)),
            JSON.stringify(Object.values(trade.slotsB)),
            trade.zenyA, trade.zenyB
        ]);

        await client.query('COMMIT');

        // Update in-memory zeny
        const pA = connectedPlayers.get(trade.playerA);
        const pB = connectedPlayers.get(trade.playerB);
        if (pA) pA.zuzucoin = (pA.zuzucoin || 0) - trade.zenyA + trade.zenyB;
        if (pB) pB.zuzucoin = (pB.zuzucoin || 0) - trade.zenyB + trade.zenyA;

        emitToBothTraders(trade, 'trade:completed', { success: true });

        // Trigger inventory reload for both players
        const sA = pA ? io.sockets.sockets.get(pA.socketId) : null;
        const sB = pB ? io.sockets.sockets.get(pB.socketId) : null;
        // Re-emit inventory:data for both (reuse existing inventory load logic)
        if (sA) sA.emit('inventory:reload', {});
        if (sB) sB.emit('inventory:reload', {});

        activeTrades.delete(trade.playerA);
        activeTrades.delete(trade.playerB);

        logger.info(`[TRADE] Completed: ${pA?.characterName} <-> ${pB?.characterName}`);
    } catch (err) {
        await client.query('ROLLBACK');
        logger.error(`[TRADE] Failed: ${err.message}`);
        emitToBothTraders(trade, 'trade:cancelled', { reason: 'Trade failed. Items returned.' });
        activeTrades.delete(trade.playerA);
        activeTrades.delete(trade.playerB);
    } finally {
        client.release();
    }
}
```

### 9.3 Client -- UTradeSubsystem Header

File: `client/SabriMMO/Source/SabriMMO/UI/TradeSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "TradeSubsystem.generated.h"

class USocketIOClientComponent;
class STradeWidget;

USTRUCT()
struct FTradeSlot
{
    GENERATED_BODY()

    int32 Slot = -1;
    int32 InventoryId = 0;
    int32 ItemId = 0;
    FString ItemName;
    FString Icon;
    int32 Quantity = 0;
    bool IsValid() const { return InventoryId > 0; }
};

UCLASS()
class SABRIMMO_API UTradeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Trade state
    bool bIsTrading = false;
    int32 PartnerId = 0;
    FString PartnerName;

    TArray<FTradeSlot> MySlots;    // 10 max
    TArray<FTradeSlot> TheirSlots; // 10 max
    int32 MyZeny = 0;
    int32 TheirZeny = 0;
    bool bMyLocked = false;
    bool bTheirLocked = false;

    // Pending request
    bool bHasPendingRequest = false;
    int32 PendingFromId = 0;
    FString PendingFromName;

    DECLARE_MULTICAST_DELEGATE(FOnTradeUpdated);
    FOnTradeUpdated OnTradeUpdated;

    DECLARE_MULTICAST_DELEGATE(FOnTradeRequestReceived);
    FOnTradeRequestReceived OnTradeRequestReceived;

    DECLARE_MULTICAST_DELEGATE(FOnTradeEnded);
    FOnTradeEnded OnTradeEnded;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void RequestTrade(int32 TargetId);
    void RespondToRequest(bool bAccept);
    void AddItem(int32 InventoryId, int32 Quantity);
    void RemoveItem(int32 Slot);
    void SetZeny(int32 Amount);
    void LockTrade();
    void ConfirmTrade();
    void CancelTrade();

    void ShowWidget();
    void HideWidget();

private:
    void TryWrapSocketEvents();
    USocketIOClientComponent* FindSocketIOComponent() const;
    void EmitToServer(const FString& Event, const FString& Payload);

    void HandleTradeStarted(const TSharedPtr<FJsonValue>& Data);
    void HandleItemAdded(const TSharedPtr<FJsonValue>& Data);
    void HandleItemRemoved(const TSharedPtr<FJsonValue>& Data);
    void HandleZenySet(const TSharedPtr<FJsonValue>& Data);
    void HandleLocked(const TSharedPtr<FJsonValue>& Data);
    void HandleCompleted(const TSharedPtr<FJsonValue>& Data);
    void HandleCancelled(const TSharedPtr<FJsonValue>& Data);
    void HandleRequestReceived(const TSharedPtr<FJsonValue>& Data);
    void HandleTradeError(const TSharedPtr<FJsonValue>& Data);

    void ResetTradeState();

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;
    int32 LocalCharacterId = 0;

    TSharedPtr<STradeWidget> TradeWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

The UTradeSubsystem follows the same lifecycle and socket wrapping as UPartySubsystem. The STradeWidget layout:

- Two columns: "Your Offer" (left) | "Their Offer" (right)
- 10 item slots per side (grid of 5x2), each slot shows icon + name + quantity
- Zeny input row below each column
- Lock button (disables item changes when pressed)
- Confirm button (only enabled when both locked)
- Cancel button
- Anti-scam: any item/zeny change resets both locks and confirms (server-enforced)

---

## 10. Player Interaction Menu

### 10.1 Server -- No Additional Handlers Needed

The context menu is purely a client UI that invokes existing actions:
- "Trade" calls `UTradeSubsystem::RequestTrade(targetId)`
- "Party Invite" calls `UPartySubsystem::InvitePlayer(targetName)`
- "Guild Invite" calls `UGuildSubsystem::InvitePlayer(targetName)`
- "Whisper" sets `UChatSubsystem::ActiveSendChannel = Whisper` and `WhisperTargetName`
- "Add Friend" calls `friend:add` via chat subsystem or direct emit
- "Block" calls `block:add` via direct emit
- "Duel" reserved for future PvP duel system

### 10.2 Client -- SPlayerContextMenu

File: `client/SabriMMO/Source/SabriMMO/UI/SPlayerContextMenu.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SPlayerContextMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPlayerContextMenu) {}
        SLATE_ARGUMENT(int32, TargetCharacterId)
        SLATE_ARGUMENT(FString, TargetCharacterName)
        SLATE_ARGUMENT(FVector2D, ScreenPosition)
        SLATE_ARGUMENT(UWorld*, World)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    int32 TargetId = 0;
    FString TargetName;
    TWeakObjectPtr<UWorld> OwningWorld;
    FVector2D MenuPosition;

    void OnTradeClicked();
    void OnPartyInviteClicked();
    void OnGuildInviteClicked();
    void OnWhisperClicked();
    void OnAddFriendClicked();
    void OnBlockClicked();
    void OnDuelClicked();

    void CloseMenu();

    TSharedRef<SWidget> BuildMenuItem(const FText& Label, FSimpleDelegate OnClicked);

    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
};
```

File: `client/SabriMMO/Source/SabriMMO/UI/SPlayerContextMenu.cpp`

```cpp
#include "SPlayerContextMenu.h"
#include "PartySubsystem.h"
#include "GuildSubsystem.h"
#include "ChatSubsystem.h"
#include "TradeSubsystem.h"
#include "SocketIOClientComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

void SPlayerContextMenu::Construct(const FArguments& InArgs)
{
    TargetId = InArgs._TargetCharacterId;
    TargetName = InArgs._TargetCharacterName;
    MenuPosition = InArgs._ScreenPosition;
    OwningWorld = InArgs._World;

    ChildSlot
    [
        SNew(SBorder)
        .BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.15f, 0.95f))
        .Padding(FMargin(2.f))
        [
            SNew(SVerticalBox)
            // Header: target name
            + SVerticalBox::Slot().AutoHeight().Padding(6.f, 4.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TargetName))
                .ColorAndOpacity(FLinearColor(1.f, 0.9f, 0.5f))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            // Menu items
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Trade")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnTradeClicked)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Party Invite")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnPartyInviteClicked)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Guild Invite")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnGuildInviteClicked)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Whisper")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnWhisperClicked)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Add Friend")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnAddFriendClicked)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Block")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnBlockClicked)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ BuildMenuItem(FText::FromString(TEXT("Duel")),
                FSimpleDelegate::CreateSP(this, &SPlayerContextMenu::OnDuelClicked)) ]
        ]
    ];

    SetRenderTransform(FSlateRenderTransform(MenuPosition));
}

TSharedRef<SWidget> SPlayerContextMenu::BuildMenuItem(const FText& Label, FSimpleDelegate OnClicked)
{
    return SNew(SButton)
        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
        .ContentPadding(FMargin(8.f, 3.f))
        .OnClicked_Lambda([this, OnClicked]() -> FReply {
            OnClicked.ExecuteIfBound();
            CloseMenu();
            return FReply::Handled();
        })
        [
            SNew(STextBlock)
            .Text(Label)
            .ColorAndOpacity(FLinearColor::White)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
        ];
}

void SPlayerContextMenu::OnTradeClicked()
{
    if (!OwningWorld.IsValid()) return;
    if (UTradeSubsystem* Trade = OwningWorld->GetSubsystem<UTradeSubsystem>())
        Trade->RequestTrade(TargetId);
}

void SPlayerContextMenu::OnPartyInviteClicked()
{
    if (!OwningWorld.IsValid()) return;
    if (UPartySubsystem* Party = OwningWorld->GetSubsystem<UPartySubsystem>())
        Party->InvitePlayer(TargetName);
}

void SPlayerContextMenu::OnGuildInviteClicked()
{
    if (!OwningWorld.IsValid()) return;
    if (UGuildSubsystem* Guild = OwningWorld->GetSubsystem<UGuildSubsystem>())
        Guild->InvitePlayer(TargetName);
}

void SPlayerContextMenu::OnWhisperClicked()
{
    if (!OwningWorld.IsValid()) return;
    if (UChatSubsystem* Chat = OwningWorld->GetSubsystem<UChatSubsystem>())
    {
        Chat->ActiveSendChannel = EChatChannel::Whisper;
        Chat->WhisperTargetName = TargetName;
        Chat->ShowWidget();
    }
}

void SPlayerContextMenu::OnAddFriendClicked()
{
    if (!OwningWorld.IsValid()) return;
    // Emit friend:add directly
    for (TActorIterator<AActor> It(OwningWorld.Get()); It; ++It)
    {
        if (USocketIOClientComponent* SIO = It->FindComponentByClass<USocketIOClientComponent>())
        {
            SIO->EmitNative(TEXT("friend:add"),
                FString::Printf(TEXT("{\"targetName\":\"%s\"}"),
                    *TargetName.Replace(TEXT("\""), TEXT(""))));
            break;
        }
    }
}

void SPlayerContextMenu::OnBlockClicked()
{
    if (!OwningWorld.IsValid()) return;
    for (TActorIterator<AActor> It(OwningWorld.Get()); It; ++It)
    {
        if (USocketIOClientComponent* SIO = It->FindComponentByClass<USocketIOClientComponent>())
        {
            SIO->EmitNative(TEXT("block:add"),
                FString::Printf(TEXT("{\"targetName\":\"%s\"}"),
                    *TargetName.Replace(TEXT("\""), TEXT(""))));
            break;
        }
    }
}

void SPlayerContextMenu::OnDuelClicked()
{
    // Future: PvP duel system
    if (!OwningWorld.IsValid()) return;
    if (UChatSubsystem* Chat = OwningWorld->GetSubsystem<UChatSubsystem>())
        Chat->AddLocalMessage(TEXT("Duel system not yet implemented."), EChatChannel::System);
}

void SPlayerContextMenu::CloseMenu()
{
    if (!OwningWorld.IsValid()) return;
    UGameViewportClient* VC = OwningWorld->GetGameViewport();
    if (!VC) return;
    // Remove self from viewport (the caller manages the shared ptr)
    SetVisibility(EVisibility::Collapsed);
}

FReply SPlayerContextMenu::OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Event)
{
    // Close on any click outside
    if (!GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()))
    {
        CloseMenu();
        return FReply::Handled();
    }
    return SCompoundWidget::OnMouseButtonDown(Geo, Event);
}
```

### 10.3 Triggering the Context Menu

The context menu is opened from `ASabriMMOCharacter` or the PlayerController when the player right-clicks on another character. Add to the click handler (in the Blueprint or C++ input):

```cpp
// In the right-click handler (e.g., ASabriMMOCharacter or PC_MMOPlayerController)
void ShowPlayerContextMenu(AActor* ClickedActor, const FVector2D& ScreenPosition)
{
    // Determine if ClickedActor is another player (via BPI_Targetable or tag check)
    int32 TargetCharId = /* extract from actor */;
    FString TargetName = /* extract from actor */;

    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* VC = World->GetGameViewport();
    if (!VC) return;

    // Remove any existing context menu
    if (ActiveContextMenu.IsValid())
    {
        VC->RemoveViewportWidgetContent(ActiveContextMenuOverlay.ToSharedRef());
        ActiveContextMenu.Reset();
        ActiveContextMenuOverlay.Reset();
    }

    ActiveContextMenu = SNew(SPlayerContextMenu)
        .TargetCharacterId(TargetCharId)
        .TargetCharacterName(TargetName)
        .ScreenPosition(ScreenPosition)
        .World(World);

    ActiveContextMenuOverlay = SNew(SWeakWidget).PossiblyNullContent(ActiveContextMenu);
    VC->AddViewportWidgetContent(ActiveContextMenuOverlay.ToSharedRef(), 50);
}
```

---

## Summary: File Manifest

### Server Files
| File | Content |
|------|---------|
| `database/migrations/add_social_systems.sql` | All DB tables (parties, guilds, friends, blocks, trade_log) |
| `server/src/index.js` (additions) | Party, guild, chat, friend, block, trade socket handlers + in-memory state |

### Client C++ Files (new)
| File | Content |
|------|---------|
| `UI/PartySubsystem.h/.cpp` | UWorldSubsystem for party state + socket events |
| `UI/SPartyWidget.h/.cpp` | Slate draggable party window with HP/SP bars |
| `UI/GuildSubsystem.h/.cpp` | UWorldSubsystem for guild state + socket events |
| `UI/SGuildWidget.h/.cpp` | Slate tabbed guild window (Info/Members/Positions/Skills/Storage) |
| `UI/ChatSubsystem.h/.cpp` | UWorldSubsystem for all chat channels |
| `UI/SChatWidget.h/.cpp` | Slate chat window with channel tabs + input |
| `UI/TradeSubsystem.h/.cpp` | UWorldSubsystem for trade flow |
| `UI/STradeWidget.h/.cpp` | Slate trade window (2-column, 10 slots + zeny) |
| `UI/SPlayerContextMenu.h/.cpp` | Right-click context menu (Trade/Party/Guild/Whisper/Friend/Block/Duel) |

### Client C++ Files (modified)
| File | Change |
|------|--------|
| `CharacterData.h` | Add FPartyMember, FPartyData, FGuildMember, FGuildPosition, FGuildData, FTradeSlot, FFriendEntry structs |
| `SabriMMOCharacter.h/.cpp` | Add ToggleParty (Alt+Z), ToggleGuild (Alt+G) input actions; right-click handler for context menu |

### Input Key Bindings
| Key | Action |
|-----|--------|
| Alt+Z | Toggle Party Window |
| Alt+G | Toggle Guild Window |
| Enter | Focus chat input / Send message |
| Right-Click on player | Open context menu |
| Tab (in chat input) | Cycle send channel |

### Widget Z-Order
| Z | Widget |
|---|--------|
| 5 | SChatWidget |
| 11 | SPartyWidget |
| 13 | SGuildWidget |
| 15 | STradeWidget |
| 50 | SPlayerContextMenu |
