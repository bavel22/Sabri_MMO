# Refine ATK System + Party System — Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Combat_System](../03_Server_Side/Combat_System.md) | [Database_Architecture](../01_Architecture/Database_Architecture.md)
> **Status**: COMPLETED — Refine ATK/DEF in damage pipeline, party system with EXP share + party chat + HP/SP sync

**Date:** 2026-03-16 (plan), 2026-03-17 (implementation), 2026-03-22 (latest fixes)
**Sources:** rAthena pre-renewal source (battle.cpp, status.cpp, party.cpp, mob.cpp, refine.yml, exp.conf), iRO Wiki Classic, RateMyServer

---

## Part 1: Refine ATK System

### Current State

`getAttackerInfo()` (line 2826) computes `refineATK` correctly (`+2/+3/+5/+7` per refine for weapon Lv 1-4), but **no code ever reads it**. The `calculatePhysicalDamage` wrapper (line 1590) does not pass `refineATK` through. The damage formula in `ro_damage_formulas.js` mentions refine in a comment at line 777 but only adds `passiveATK`. Grand Cross has its own inline refine calc (line 11245) — the only place refine currently affects damage.

### rAthena Pre-Renewal Refine ATK Pipeline (Verified from battle.cpp)

**Damage pipeline order:**
```
1. Base weapon damage (WeaponATK + DEX variance) → size penalty applied
2. Overupgrade random bonus added (AFTER size penalty, inside base damage)
3. Skill ratio applied (ATK_RATE) — affects overupgrade but NOT flat refine
4. Card fix / SC bonuses
5. DEF reduction (hard DEF % + soft DEF flat)
6. ★ REFINE ATK (rhw.atk2) ADDED HERE — bypasses DEF entirely ★
7. Damage capped to minimum 1
8. ★ MASTERY ATK ADDED HERE — also bypasses DEF ★
9. Damage capped to minimum 1 again
10. Element modifier applied (affects refine + mastery)
```

### Properties of Each Refine Component

| Property | Flat Refine (`rhw.atk2`) | Overupgrade Random | Mastery ATK |
|----------|--------------------------|-------------------|-------------|
| When added | After DEF (step 6) | Inside base damage (step 2) | After DEF (step 8) |
| Bypasses hard DEF? | YES | NO (before DEF) | YES |
| Bypasses soft DEF? | YES | NO (before DEF) | YES |
| Affected by size penalty? | NO | NO (added after size fix) | NO |
| Affected by skill ratio? | NO | YES (part of base before ratio) | NO |
| Affected by element? | YES | YES | YES |
| Per-hit random? | NO (flat) | YES (1 to max) | NO (flat) |
| Shown in status window? | YES (right side "+X") | NO | NO |

### Flat Refine ATK Per Level

| Weapon Level | ATK per refine | Safe Limit |
|---|---|---|
| Lv1 | +2 | +7 |
| Lv2 | +3 | +6 |
| Lv3 | +5 | +5 |
| Lv4 | +7 | +4 |

### Overupgrade Random Bonus (above safe limit)

| Weapon Lv | +5 | +6 | +7 | +8 | +9 | +10 |
|---|---|---|---|---|---|---|
| Lv1 | — | — | — | 1~3 | 1~6 | 1~9 |
| Lv2 | — | — | 1~5 | 1~10 | 1~15 | 1~20 |
| Lv3 | — | 1~8 | 1~16 | 1~24 | 1~32 | 1~40 |
| Lv4 | 1~13 | 1~26 | 1~39 | 1~52 | 1~65 | 1~78 |

Formula: `rnd() % overrefineMax + 1`

### Skills That Exclude Refine Bonus

These skills do NOT add flat refine ATK (from `battle_skill_stacks_masteries_vvs`):
- Shield Boomerang (1305) — already uses custom damage
- Shield Chain (Paladin future)
- Acid Terror (1809)
- Investigate (1606)
- Asura Strike (1610)
- Sacrifice/Martyr's Reckoning (Paladin future)

All other physical skills (Bash, Double Strafe, Bowling Bash, Sonic Blow, Pierce, etc.) DO get refine ATK.

### Armor Refine DEF Bonus

Per-piece formula: `floor((3 + refineLevel) / 4)` hard DEF per piece.

| Refine | +1 | +2 | +3 | +4 | +5 | +6 | +7 | +8 | +9 | +10 |
|---|---|---|---|---|---|---|---|---|---|---|
| DEF bonus | +1 | +1 | +1 | +1 | +2 | +2 | +2 | +2 | +3 | +3 |

Currently not implemented. Armor refine goes through `item.def` already (added as equipment bonus), so this is a separate refine-specific bonus that should stack.

### Implementation Plan

#### R1. Wire refineATK into the damage pipeline (`ro_damage_formulas.js`)

In the `calculatePhysicalDamage` function, after DEF subtraction and before element modifier:

```javascript
// Step 6: Refine ATK (bypasses DEF entirely, added post-DEF)
// rAthena: rhw.atk2 added in battle_calc_attack_post_defense()
const refineATK = attacker.refineATK || 0;
if (refineATK > 0 && !REFINE_EXCLUDE_SKILLS.includes(skillId)) {
    damage += refineATK;
}

// Step 7: Cap damage to minimum 1
damage = Math.max(1, damage);

// Step 8: Mastery ATK (also bypasses DEF)
damage += passiveATK;
```

Create a constant array for excluded skills:
```javascript
const REFINE_EXCLUDE_SKILLS = [1305, 1809, 1606, 1610]; // SB, Acid Terror, Investigate, Asura
```

#### R2. Pass refineATK through the wrapper (`index.js`)

In the `calculatePhysicalDamage` wrapper (line ~1590), add `refineATK` to the attacker object passed to the formula:

```javascript
refineATK: attackerInfo.refineATK || 0,
```

#### R3. Add overupgrade random bonus

In `getAttackerInfo()`, compute the overupgrade maximum alongside the flat refine:

```javascript
overrefineMax: (() => {
    const rl = player.equippedWeaponRight?.refineLevel || 0;
    const wl = player.weaponLevel || 1;
    const safeLimit = [0, 7, 6, 5, 4][wl] || 7;
    if (rl <= safeLimit) return 0;
    // Overupgrade random bonus per weapon level (from rAthena refine.yml)
    const OVER_BONUS = {
        1: [0,0,0,0,0,0,0,0,3,6,9],      // Lv1: +8=3, +9=6, +10=9
        2: [0,0,0,0,0,0,0,5,10,15,20],    // Lv2: +7=5, +8=10, +9=15, +10=20
        3: [0,0,0,0,0,0,8,16,24,32,40],   // Lv3: +6=8, +7=16, +8=24, +9=32, +10=40
        4: [0,0,0,0,0,13,26,39,52,65,78], // Lv4: +5=13, +6=26, +7=39, +8=52, +9=65, +10=78
    };
    return (OVER_BONUS[wl] && OVER_BONUS[wl][rl]) || 0;
})(),
```

In `calculatePhysicalDamage` (inside base damage, before DEF):
```javascript
// Overupgrade random bonus (added to base damage BEFORE DEF, after size penalty)
if (attacker.overrefineMax > 0) {
    baseDamage += Math.floor(Math.random() * attacker.overrefineMax) + 1;
}
```

#### R4. Dual-wield left-hand refine

For dual-wielding Assassins, left-hand refine bonus should also be computed:
```javascript
refineATKLeft: (() => {
    if (!player.equippedWeaponLeft) return 0;
    const REFINE_ATK_PER_LEVEL = [0, 2, 3, 5, 7];
    const rl = player.equippedWeaponLeft.refineLevel || 0;
    const wl = player.equippedWeaponLeft.weaponLevel || 1;
    return rl * (REFINE_ATK_PER_LEVEL[wl] || 2);
})(),
```

#### R5. Armor refine DEF bonus

When loading equipped items in `player:join` and in the equip handler, compute total armor refine DEF bonus from all equipped armor/shield/headgear:
```javascript
// Query all equipped armor-type items with refine
const armorRefineResult = await pool.query(
    `SELECT ci.refine_level FROM character_inventory ci
     JOIN items i ON ci.item_id = i.item_id
     WHERE ci.character_id = $1 AND ci.is_equipped = true
       AND i.equip_slot IN ('armor', 'shield', 'headgear_top', 'headgear_mid', 'garment', 'shoes')`,
    [characterId]
);
let armorRefineDef = 0;
for (const row of armorRefineResult.rows) {
    armorRefineDef += Math.floor((3 + (row.refine_level || 0)) / 4);
}
player.armorRefineDef = armorRefineDef; // Added to hardDef
```

#### R6. Status window display

When building the stats payload, show refine ATK separately:
```javascript
// ATK display: left side = StatusATK + WeaponBaseATK, right side = refineATK
atkDisplay: { base: statusATK + weaponATK, refine: refineATK }
```

---

## Part 2: Party System

### Current State (2026-03-22)

**FULLY IMPLEMENTED.** Server has 9 socket handlers, client has `PartySubsystem` + `SPartyWidget` with full UI, DB tables auto-created.

**Server** (`server/src/index.js`):
- 9 socket handlers: `party:load`, `party:create`, `party:invite`, `party:invite_respond`, `party:leave`, `party:kick`, `party:change_leader`, `party:change_exp_share`, `party:chat`
- `party:load` — client re-requests party state after subsystem init (fixes reconnect timing)
- `party:create` reads `data.name` (1-24 chars), requires Basic Skill Lv7, creates DB records
- `party:invite` reads `data.targetName`, 30s TTL pending invites
- `party:change_exp_share` reads `data.mode` (`even_share`/`each_take`), broadcasts PARTY chat on change
- `distributePartyEXP()` returns share metadata: `partyShared`, `originalBaseExp/Job`, `partyBonusPct`, `memberCount`
- `exp:gain` event includes party share details for client chat display
- HP/SP sync: 1-second periodic interval, includes self (no longer excludes), tracks SP alongside HP
- SP added to all member data: create, join, reconnect, `buildPartyPayload`, periodic sync
- `buildPartyPayload` sends: `name` (not partyName), `expShare`, members with `map` (not mapName), `online` (not isOnline), `sp`, `maxSp`

**Client** (6 files):
- `SabriMMOCharacter.h/.cpp` — P key toggle via `TogglePartyAction`
- `PartySubsystem.h` — `FPartyMember` with SP/MaxSP fields
- `PartySubsystem.cpp` — 8 socket handlers, `party:load` re-request, error routing to chat via `AddCombatLogMessage`
- `SPartyWidget.h/.cpp` — RO brown/gold frame, create/invite text inputs, settings popup, HP/SP proportional bars, invite popup, right-click context menu, drag-to-move
- `ChatSubsystem.h/.cpp` — `exp:gain` handler with solo vs party shared formatting, `AddCombatLogMessage` now public

**DB**: `parties` + `party_members` tables (auto-created at server startup)

### rAthena Pre-Renewal Party Mechanics

#### Core Rules

| Property | Value |
|----------|-------|
| Max members | 12 |
| Creation requirement | Basic Skill Lv7 |
| Name | Must be unique on server |
| Level gap for Even Share | ≤ 15 base levels (online members only) |
| EXP share distance | Same map only (no within-map distance limit) |
| Dead members | Excluded from share (don't reduce pool) |
| Offline members | Excluded from share, slot remains occupied |
| Party bonus | +20% per member from 2nd (`100 + 20*(N-1)`) |
| Item share | Configurable: Each Take or Party Share |
| Party chat prefix | `%` before message |

#### EXP Share Formula (Even Share)

```
eligible = count of alive + online + same_map members
bonus = 100 + 20 * (eligible - 1)
per_member_base = floor(monster_base_exp * bonus / 100 / eligible)
per_member_job = floor(monster_job_exp * bonus / 100 / eligible)
```

Each member receives `per_member_base` and `per_member_job`.

| Party Size | Total Pool | Per Member |
|-----------|-----------|------------|
| 1 | 100% | 100% |
| 2 | 120% | 60% |
| 3 | 140% | ~47% |
| 4 | 160% | 40% |
| 5 | 180% | 36% |
| 6 | 200% | ~33% |
| 7 | 220% | ~31% |
| 8 | 240% | 30% |
| 9 | 260% | ~29% |
| 10 | 280% | 28% |
| 11 | 300% | ~27% |
| 12 | 320% | ~27% |

#### EXP Tap Bonus (Multi-Attacker)

Separate from party — applies to total monster EXP BEFORE party division. +25% per additional player who dealt damage, max 12 attackers.

```
tap_bonus = 100 + 25 * (attacker_count - 1)
final_exp = floor(base_exp * tap_bonus / 100)
```

Applied before party share formula.

#### Item Share Modes

- **Each Take**: Drop priority to highest damage dealer, first-come-first-served after priority expires
- **Party Share + Individual**: Any member picks up, they keep it
- **Party Share + Shared**: Any member picks up, random distribution to same-map member

#### Leader Powers

- Invite members (by character name, remote — no proximity needed)
- Kick/expel members
- Change EXP share mode (Each Take ↔ Even Share) anytime, if level gap allows
- Delegate leadership (must be on same map as new leader)
- Cannot change Item share mode after creation

#### Party Window Display

Per member: character name, HP bar (real-time), map name, online/offline status, leader indicator (crown).

RO Classic does NOT show: SP, exact HP numbers, level, equipment, class icon (pre-renewal).

**SabriMMO divergence**: Our implementation shows SP bars alongside HP bars (blue proportional bar) and includes SP in the periodic sync. This is a QoL improvement over RO Classic.

### Implementation Plan

#### Database

##### P0. Create `parties` table

```sql
CREATE TABLE IF NOT EXISTS parties (
    party_id SERIAL PRIMARY KEY,
    name VARCHAR(24) UNIQUE NOT NULL,
    leader_id INTEGER NOT NULL REFERENCES characters(id),
    exp_share SMALLINT DEFAULT 0,    -- 0=each_take, 1=even_share
    item_share SMALLINT DEFAULT 0,   -- 0=each_take, 1=party_share
    item_distribute SMALLINT DEFAULT 0, -- 0=individual, 1=shared
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS party_members (
    party_id INTEGER NOT NULL REFERENCES parties(party_id) ON DELETE CASCADE,
    character_id INTEGER NOT NULL REFERENCES characters(id),
    joined_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (party_id, character_id)
);
```

#### Server Data Structures

##### P1. In-memory party state (IMPLEMENTED)

```javascript
const activeParties = new Map(); // partyId -> PartyState

// PartyState:
{
    partyId: number,
    name: string,
    leaderId: number,           // characterId of leader
    expShare: 'each_take' | 'even_share',
    itemShare: 'each_take' | 'party_share',
    itemDistribute: 'individual' | 'shared',
    members: Map<number, {      // characterId -> member info
        characterId: number,
        characterName: string,
        jobClass: string,
        level: number,
        map: string,
        hp: number,
        maxHp: number,
        sp: number,             // Added 2026-03-22
        maxSp: number,          // Added 2026-03-22
        online: boolean,
        socketId: string | null,
    }>
}

// On player object:
player.partyId = null;  // set when joining a party
```

**SP tracking**: SP was added to all member data paths — create, join, reconnect, `buildPartyPayload`, and the 1-second periodic HP/SP sync. The periodic sync now includes self (no longer excludes the sender).

#### Socket Events

##### P2. Party creation and management events (IMPLEMENTED)

| Event | Direction | Payload | Handler |
|-------|-----------|---------|---------|
| `party:load` | C→S | `{}` | Re-request party state after subsystem init (reconnect fix) |
| `party:create` | C→S | `{ name }` | Create party (requires Basic Skill Lv7, 1-24 chars) |
| `party:invite` | C→S | `{ targetName }` | Send invite to target (30s TTL) |
| `party:invite_received` | S→C | `{ partyId, partyName, inviterName }` | Show invite popup to target |
| `party:invite_respond` | C→S | `{ partyId, accept }` | Accept/decline invite |
| `party:kick` | C→S | `{ targetCharId }` | Leader kicks member |
| `party:leave` | C→S | `{}` | Leave current party |
| `party:change_leader` | C→S | `{ targetCharId }` | Delegate leadership |
| `party:change_exp_share` | C→S | `{ mode }` | Toggle `even_share` / `each_take`, broadcasts PARTY chat |
| `party:chat` | C→S | `{ message }` | Party chat message |
| `party:update` | S→C | `{ party }` | Full party state sync (uses `buildPartyPayload`) |
| `party:member_joined` | S→C | `{ member }` | New member joined |
| `party:member_left` | S→C | `{ characterId, reason }` | Member left/kicked |
| `party:member_update` | S→C | `{ characterId, hp, maxHp, sp, maxSp, map }` | HP/SP/map update (1s periodic) |
| `party:member_offline` | S→C | `{ characterId }` | Member went offline |
| `party:dissolved` | S→C | `{}` | Party disbanded |
| `party:chat_receive` | S→C | `{ senderName, message }` | Party chat broadcast |
| `party:error` | S→C | `{ message }` | Error messages (routed to chat on client) |

**Key differences from original plan:**
- `party:load` added — client re-requests after subsystem init to handle timing issues
- `party:create` only takes `name` (no `expShare`/`itemShare`/`itemDistribute` — defaults used)
- `party:member_update` includes `sp` and `maxSp` alongside HP fields
- `party:change_exp_share` broadcasts a PARTY chat message on mode change
- `buildPartyPayload` field names: `name` (not partyName), `expShare`, members have `map` (not mapName), `online` (not isOnline)
- `exp:gain` event includes party share metadata (`partyShared`, `originalBaseExp/Job`, `partyBonusPct`, `memberCount`)

##### P3. Party creation handler

```
Validation:
- Player not already in a party
- Player has Basic Skill Lv7 (learnedSkills[1] >= 7)
- Party name unique (check DB + activeParties)
- Name length 1-24 characters

Steps:
1. INSERT into parties table
2. INSERT into party_members table
3. Create PartyState in activeParties Map
4. Set player.partyId
5. Emit party:update to creator with full state
```

##### P4. Party invite handler

```
Validation:
- Inviter is party leader
- Party has < 12 members
- Target exists and is online
- Target not already in a party

Steps:
1. Find target by name in connectedPlayers
2. Emit party:invite_received to target
3. Store pending invite (with timeout ~30s)
```

##### P5. Party invite response handler

```
If accept:
1. INSERT into party_members
2. Add to PartyState.members
3. Set target.partyId
4. Check Even Share level gap (recalculate)
5. Emit party:member_joined to all members
6. Emit party:update to joiner

If decline:
1. Emit party:error to inviter "Invitation declined"
```

##### P6. Party leave/kick handler

```
Steps:
1. DELETE from party_members
2. Remove from PartyState.members
3. Clear player.partyId
4. If leader left:
   a. If other online members exist: delegate to first online member
   b. If no online members: disband party (DELETE from parties)
5. Emit party:member_left to remaining members
6. Recalculate Even Share level gap
```

##### P7. EXP share mode change

```
Validation:
- Requester is party leader
- If switching to Even Share: check all online members within 15 level gap

Steps:
1. UPDATE parties SET exp_share
2. Update PartyState.expShare
3. Emit party:update to all members
```

#### EXP Distribution Integration

##### P8. Modify `processEnemyDeathFromSkill()` / EXP grant section

Current: Killer gets all EXP directly.
New: Check if killer is in a party with Even Share.

```javascript
function distributePartyEXP(killer, baseExp, jobExp, enemyZone) {
    if (!killer.partyId) {
        // Solo: killer gets 100%
        grantEXP(killer, baseExp, jobExp);
        return;
    }

    const party = activeParties.get(killer.partyId);
    if (!party || party.expShare === 'each_take') {
        // Each Take: killer gets 100%
        grantEXP(killer, baseExp, jobExp);
        return;
    }

    // Even Share: find eligible members (alive + online + same map)
    const eligible = [];
    let minLv = Infinity, maxLv = 0;
    for (const [charId, member] of party.members) {
        if (!member.online) continue;
        minLv = Math.min(minLv, member.level);
        maxLv = Math.max(maxLv, member.level);
    }

    // Level gap check (online members only)
    if (maxLv - minLv > 15) {
        // Level gap too large: fallback to killer-only
        grantEXP(killer, baseExp, jobExp);
        return;
    }

    // Collect eligible members (alive + online + same map as enemy)
    for (const [charId, member] of party.members) {
        if (!member.online) continue;
        if (member.map !== enemyZone) continue;
        const memberPlayer = connectedPlayers.get(charId);
        if (!memberPlayer || memberPlayer.health <= 0) continue;
        eligible.push(memberPlayer);
    }

    if (eligible.length === 0) return;

    // Party bonus: 100 + 20 * (N-1)
    const bonus = 100 + 20 * (eligible.length - 1);
    const sharedBase = Math.floor(baseExp * bonus / 100 / eligible.length);
    const sharedJob = Math.floor(jobExp * bonus / 100 / eligible.length);

    for (const member of eligible) {
        grantEXP(member, sharedBase, sharedJob);
    }
}
```

##### P9. EXP Tap Bonus (Multi-Attacker)

In the enemy death handler, count unique players who dealt damage:

```javascript
const attackerCount = enemy.inCombatWith ? enemy.inCombatWith.size : 1;
const tapBonus = 100 + 25 * Math.min(11, Math.max(0, attackerCount - 1));
const adjustedBaseExp = Math.floor(baseExp * tapBonus / 100);
const adjustedJobExp = Math.floor(jobExp * tapBonus / 100);
// Then pass adjustedBaseExp/adjustedJobExp to distributePartyEXP
```

#### Party Member Updates

##### P10. HP/SP/Map broadcasts (IMPLEMENTED)

HP/SP sync uses a 1-second periodic interval that broadcasts to all party members (including self):
```javascript
// Periodic sync (1s interval) — includes self, tracks SP alongside HP
party:member_update { characterId, hp, maxHp, sp, maxSp, map }
```

On zone change:
```javascript
if (player.partyId) {
    updatePartyMemberMap(player.partyId, characterId, newZone);
    broadcastToParty(player.partyId, 'party:member_update', {
        characterId, map: newZone
    });
}
```

**Changes from original plan**: SP fields added, periodic sync includes self (previously excluded), 7 map sync paths ensure map changes propagate correctly.

##### P11. Party persistence on login/logout

On `player:join`:
```javascript
// Load party membership from DB
const partyResult = await pool.query(
    'SELECT pm.party_id FROM party_members pm WHERE pm.character_id = $1', [characterId]
);
if (partyResult.rows.length > 0) {
    player.partyId = partyResult.rows[0].party_id;
    // Update PartyState: mark member as online
    // Emit party:update to other members
}
```

On disconnect:
```javascript
if (player.partyId) {
    // Mark member as offline in PartyState
    broadcastToParty(player.partyId, 'party:member_offline', { characterId });
}
```

#### Party Chat

##### P12. Party chat handler

```javascript
socket.on('party:chat', ({ message }) => {
    if (!player.partyId) return;
    broadcastToParty(player.partyId, 'party:chat_receive', {
        senderName: player.characterName, message
    });
});
```

Also handle `%` prefix in existing chat handler — if message starts with `%`, route to party chat instead of global.

#### Party-Targeted Skills

##### P13. Update Devotion target validation

Currently Devotion checks "target a party member" but has no party system to validate against. Add:

```javascript
// Devotion: target must be in same party
if (!player.partyId || !targetPlayer.partyId || player.partyId !== targetPlayer.partyId) {
    socket.emit('skill:error', { message: 'Target must be a party member' });
    return;
}
```

##### P14. Party-only AoE buffs

The following skills should affect party members in range only (not all players):
- Magnificat, Gloria, Angelus, Impositio Manus, Suffragium

Currently these may not be implemented or may affect all players. When implementing:
```javascript
// Find party members in range
const buffTargets = [];
for (const [charId, member] of party.members) {
    if (!member.online || member.map !== player.zone) continue;
    const memberPlayer = connectedPlayers.get(charId);
    if (!memberPlayer) continue;
    const dist = getDistance(player, memberPlayer);
    if (dist <= SKILL_RANGE) buffTargets.push(memberPlayer);
}
```

#### Helper Functions

##### P15. Utility functions

```javascript
function broadcastToParty(partyId, event, data, excludePlayer = null) {
    const party = activeParties.get(partyId);
    if (!party) return;
    for (const [charId, member] of party.members) {
        if (!member.online || !member.socketId) continue;
        if (excludePlayer && charId === excludePlayer.characterId) continue;
        const sock = io.sockets.sockets.get(member.socketId);
        if (sock) sock.emit(event, data);
    }
}

function getPartyForPlayer(player) {
    if (!player.partyId) return null;
    return activeParties.get(player.partyId);
}

function updatePartyMemberMap(partyId, characterId, newMap) {
    const party = activeParties.get(partyId);
    if (!party) return;
    const member = party.members.get(characterId);
    if (member) member.map = newMap;
}

function recalcEvenShareEligibility(party) {
    let minLv = Infinity, maxLv = 0;
    for (const [, member] of party.members) {
        if (!member.online) continue;
        minLv = Math.min(minLv, member.level);
        maxLv = Math.max(maxLv, member.level);
    }
    return maxLv - minLv <= 15;
}
```

---

## Execution Order

### Phase 1: Refine ATK (quick win — 3 files, small changes)

```
R1. Wire refineATK into ro_damage_formulas.js (post-DEF, pre-element)
R2. Pass refineATK through calculatePhysicalDamage wrapper
R3. Add overupgrade random bonus to getAttackerInfo + base damage
R4. Dual-wield left-hand refine
R5. Armor refine DEF bonus (player:join + equip handler)
R6. Status window display (stats payload)
```

### Phase 2: Party System — ALL COMPLETE (except P14)

```
P0.  [DONE] Database migration (parties + party_members tables, auto-created at startup)
P1.  [DONE] In-memory data structures (activeParties Map, SP fields added)
P2.  [DONE] Socket event registration (9 C→S handlers + 7 S→C events)
P3.  [DONE] Party creation handler (Basic Skill Lv7, 1-24 chars)
P4.  [DONE] Party invite handler (30s TTL pending invites)
P5.  [DONE] Party invite response (accept/decline)
P6.  [DONE] Party leave/kick + leader delegation + disband
P7.  [DONE] EXP share mode change (broadcasts PARTY chat on change)
P8.  [DONE] EXP distribution integration (distributePartyEXP with share metadata)
P9.  [DONE] EXP tap bonus (multi-attacker, +25%/attacker)
P10. [DONE] HP/SP/Map broadcasts (1s periodic, includes self, SP added)
P11. [DONE] Party persistence on login/logout + party:load re-request
P12. [DONE] Party chat handler (% prefix in global chat)
P13. [DONE] Devotion target validation (party check)
P14. [DEFERRED] Party-only AoE buff targeting (implement per-skill as needed)
P15. [DONE] Helper functions (broadcastToParty, buildPartyPayload, etc.)
```

---

### Phase 3: Party UI (client-side, after server is done)

```
C1.  FPartyMember / FPartyState structs
C2.  PartySubsystem — socket events + state
C3.  SPartyWidget — window frame (title, nav, close, drag)
C4.  SPartyWidget — member list (name, HP bar, map, online/offline)
C5.  SPartyWidget — leader indicator + HP bar colors
C6.  SPartyWidget — right-click context menu
C7.  SPartyWidget — click-to-target for skills
C8.  Create Party / Invite / Invite Received dialogs
C9.  ChatSubsystem — party chat (% prefix + green channel)
C10. Minimap dots (deferred until minimap exists)
```

---

## Files Modified (ALL COMPLETE)

| File | Changes |
|------|---------|
| `server/src/ro_damage_formulas.js` | R1: refineATK post-DEF, R3: overupgrade in base damage |
| `server/src/index.js` | R2-R6: wrapper + overupgrade + armor DEF + display |
| `server/src/index.js` | P1-P15: 9 party socket handlers, distributePartyEXP, HP/SP sync, buildPartyPayload |
| `database/migrations/add_party_system.sql` | P0: parties + party_members tables (also auto-created at startup) |
| `client/.../SabriMMOCharacter.h/.cpp` | P key toggle for party window |
| `client/.../UI/PartySubsystem.h/.cpp` | C1-C2: party state (FPartyMember with SP/MaxSP), 8 socket handlers, party:load re-request |
| `client/.../UI/SPartyWidget.h/.cpp` | C3-C10: full party window (HP/SP bars, create/invite/settings, context menu, drag-to-move) |
| `client/.../UI/ChatSubsystem.h/.cpp` | C11: party:chat_receive, % prefix routing, exp:gain party formatting, AddCombatLogMessage public |

---

## Verification

### Refine ATK
- [ ] +10 Lv1 weapon adds +20 ATK after DEF (visible in damage numbers)
- [ ] +10 Lv4 weapon adds +70 ATK after DEF
- [ ] Overupgrade +10 Lv4 weapon adds 1~78 random per hit
- [ ] Refine ATK NOT affected by size penalty (same bonus vs small/medium/large)
- [ ] Refine ATK NOT affected by skill ratio (Bash 400% doesn't multiply refine)
- [ ] Refine ATK IS affected by element (fire refine damage reduced vs water target)
- [ ] Shield Boomerang does NOT add refine ATK (excluded)
- [ ] Acid Terror does NOT add refine ATK (excluded)
- [ ] Investigate does NOT add refine ATK (excluded)
- [ ] Dual-wield left-hand refine adds independently

### Party System
- [x] Create party via UI Create button (needs Basic Skill Lv7, 1-24 char name)
- [x] Invite player by name, they receive popup
- [x] Accept invite → join party, see party window
- [x] Party window shows: name, HP/SP bars, map, online status, leader crown
- [x] Even Share: all alive+online+same-map members get equal EXP with +20%/member bonus
- [x] Even Share fails if level gap > 15 (fallback to killer-only)
- [x] Dead members excluded from EXP share
- [x] Offline members excluded from EXP share
- [x] Leader can kick, change EXP mode, delegate leadership
- [x] Party persists across login/logout (+ party:load re-request)
- [x] Party chat via `%` prefix works cross-map
- [x] Devotion only targets party members
- [x] Tap bonus: +25% per additional attacker on same monster
- [x] Party window UI matches RO Classic (brown/gold theme)
- [ ] Minimap shows party member dots (DEFERRED — no minimap system yet)
- [x] Right-click context menu: Whisper, Kick (leader), Delegate Leader, Leave
- [x] Click member to target them with Heal/Blessing/etc.
- [x] Create party dialog: name input + settings popup for EXP share
- [x] Invite popup on target player with accept/decline
- [x] SP bars displayed alongside HP bars
- [x] EXP gain chat messages show party share details (original EXP, bonus %, member count)
- [x] party:error messages routed to chat via AddCombatLogMessage
- [x] P key toggle for party window

---

## Part 3: Client-Side Party UI (UE5 C++ Slate)

### Reference: RO Classic Party Window

Based on roBrowser source (`PartyFriends.js/.css`), rAthena packet structures, and iRO Wiki screenshots.

### Architecture

Following the project's established pattern (see CLAUDE.md "Persistent Socket Architecture"):

| File | Role |
|------|------|
| `UI/PartySubsystem.h/.cpp` | UWorldSubsystem — manages party state, socket events, member registry |
| `UI/SPartyWidget.h/.cpp` | Slate widget — renders the Party window UI |

`PartySubsystem` registers socket handlers via `SocketEventRouter` in `OnWorldBeginPlay`, stores party state in C++ structs, and drives `SPartyWidget` updates. Widget is gated behind `GI->IsSocketConnected()`.

### Data Structures (C++) — IMPLEMENTED

```cpp
// PartySubsystem.h

struct FPartyMember
{
    int32 CharacterId = 0;
    FString CharacterName;
    FString JobClass;
    int32 Level = 0;
    FString MapName;          // "prontera", "geffen", etc.
    int32 HP = 0;
    int32 MaxHP = 0;
    int32 SP = 0;             // Added 2026-03-22
    int32 MaxSP = 0;          // Added 2026-03-22
    bool bIsOnline = true;
    bool bIsLeader = false;
};
```

Note: `FPartyState` is not a separate struct — party state is stored as member variables directly on `UPartySubsystem` (`PartyId`, `PartyName`, `LeaderId`, `ExpShare`, `Members` TMap).

### Socket Events Handled (IMPLEMENTED — PartySubsystem registers 8 handlers)

| Event | Direction | Handler |
|-------|-----------|---------|
| `party:update` | S→C | Full party state sync (on join, settings change) |
| `party:member_joined` | S→C | Add member to TMap, refresh widget |
| `party:member_left` | S→C | Remove member, refresh widget |
| `party:member_update` | S→C | Update HP/SP/map, refresh HP/SP bars |
| `party:member_offline` | S→C | Mark offline, hide HP/map in widget |
| `party:dissolved` | S→C | Clear state, hide widget |
| `party:invite_received` | S→C | Show invite popup overlay |
| `party:error` | S→C | Route error message to chat via `AddCombatLogMessage` |

**Note**: `party:chat_receive` is handled by `ChatSubsystem`, not `PartySubsystem`. `party:load` is emitted by `PartySubsystem` during init to re-request party state (fixes timing when server sends data before subsystem is ready).

`ChatSubsystem` also handles `exp:gain` — formats EXP messages differently for solo vs party shared (shows original EXP, bonus %, member count).

### Outbound Emits (IMPLEMENTED)

| Action | Event | Payload |
|--------|-------|---------|
| Re-request party state | `party:load` | `{}` |
| Create party | `party:create` | `{ name }` |
| Invite player | `party:invite` | `{ targetName }` |
| Accept/decline invite | `party:invite_respond` | `{ partyId, accept }` |
| Leave party | `party:leave` | `{}` |
| Kick member | `party:kick` | `{ targetCharId }` |
| Delegate leader | `party:change_leader` | `{ targetCharId }` |
| Change EXP share | `party:change_exp_share` | `{ mode }` |
| Party chat message | `party:chat` | `{ message }` |

### Widget Layout (SPartyWidget) — IMPLEMENTED

```
┌─────────────────────────────────┐
│ ⊗  Party - "PartyName"         │  ← Title bar (draggable, close button)
├─────────────────────────────────┤
│ [Create] [Invite] [Leave] [⚙]  │  ← Nav buttons (context-sensitive)
├─────────────────────────────────┤
│ 👑 PlayerName1      1234/5678  │  ← Leader: crown icon, bold dark blue name
│    ████████░░  HP               │  ← HP bar (green/red)
│    ██████████  SP  (prontera)  │  ← SP bar (blue) + map name
│                                │
│ ● PlayerName2       890/2000   │  ← Online: green dot, teal name
│    ██████░░░░  HP               │
│    ████████░░  SP  (geffen)    │
│                                │
│   PlayerName3                  │  ← Offline: no dot, no HP/SP/map shown
│                                │
│ ● PlayerName4       450/1500   │
│    ███░░░░░░░  HP               │  ← HP bar red at ≤25%
│    █████░░░░░  SP  (prontera)  │
├─────────────────────────────────┤
│ ◉ Party  ○ Friends    ◫       │  ← Tab switch + resize handle
└─────────────────────────────────┘
```

**Additions beyond original plan**: SP proportional bars (blue), settings popup for EXP share toggle, invite popup with accept/decline, drag-to-move window, P key toggle via `SabriMMOCharacter`.

### Visual Specification

| Element | Value | RO Classic Reference |
|---------|-------|---------------------|
| **Window background** | White (`#FFFFFF` / `FLinearColor::White`) | roBrowser `PartyFriends.css` |
| **Title bar** | RO brown/gold theme, 17px height | Project standard |
| **Member name (normal)** | Teal `#007B7B` (`FLinearColor(0, 0.48, 0.48)`) | roBrowser `.node` color |
| **Member name (leader)** | Dark blue `#08317B`, bold | roBrowser `.node.leader` |
| **Member name (offline)** | Gray `#808080` | HP/map hidden per roBrowser `.online` gating |
| **HP bar border** | Dark blue `#10189C` | roBrowser `EntityLife.js` |
| **HP bar background** | Dark gray `#424242` | roBrowser |
| **HP bar fill (>25%)** | Green `#10EF21` | roBrowser |
| **HP bar fill (≤25%)** | Red `#FF0000` | roBrowser |
| **HP bar size** | 60×5 px (scaled to Slate units) | roBrowser canvas 60×5 |
| **HP numbers** | Black, `current/max` format | roBrowser text at `left:86px` |
| **Map name** | Parenthesized, same line as HP | roBrowser `(mapname)` |
| **Leader icon** | Crown sprite (left of name) | `grp_leader.bmp` |
| **Online icon** | Green dot (left of name) | `grp_online.bmp` |
| **Party chat color** | Green `#00FF00` | RO standard party chat |
| **Nav buttons** | 26×20 px each | roBrowser button specs |
| **Window Z-order** | Z=14 (above chat Z=13, below inspect Z=22) | Project Z-order convention |

### Interaction Behaviors

#### Click Member → Skill Target
When `SkillTreeSubsystem` is in targeting mode (`bIsTargetingSkill`), clicking a party member's entry calls:
```cpp
// PartySubsystem notifies SkillTreeSubsystem to target this characterId
SkillTreeSubsystem->OnTargetSelected(TargetCharacterId, /*bIsEnemy=*/false);
```

#### Right-Click Context Menu
Right-clicking a member entry shows an `SMenuAnchor` popup:

**On self:**
- "Leave Party" → emit `party:leave`

**On others (as leader):**
- "Whisper" → focus chat input with `/w MemberName `
- "Kick" → emit `party:kick { targetCharId }`
- "Delegate Leader" → emit `party:change_leader { targetCharId }`

**On others (not leader):**
- "Whisper" → focus chat input

#### Create Party Dialog
Modal overlay (Z=30) with:
- Party Name text input (max 24 chars)
- Item Sharing checkbox ("Party Share" vs default "Each Take")
- OK / Cancel buttons
- Emits `party:create` on OK

#### Invite Dialog
Simple text input popup: "Enter player name:" with OK/Cancel.
Emits `party:invite { targetName }`.

#### Invite Received Popup
Notification overlay (Z=30):
```
"[PartyName] invites you to join their party."
[Accept] [Decline]
```
Emits `party:invite_respond { partyId, accept: true/false }`.

### Minimap Integration

In `NameTagSubsystem` or a new `MinimapSubsystem`, render party member positions:
- Each member gets a unique random `FLinearColor` on join (stored in `FPartyMember::MinimapColor`)
- Render as 6×6 white square with 4×4 colored inner square
- Position updated via `party:member_update` (includes x, y when on same map)
- Only show members on the same map

### Party Chat Integration (IMPLEMENTED)

In `ChatSubsystem`:
- Register handler for `party:chat_receive`
- Display messages in green (`#00FF00`) with `[Party] SenderName: message` format
- In `SendChatMessage()`, check if message starts with `%` → strip prefix, emit `party:chat` instead of `chat:send`
- Add "Party" tab filter alongside existing "All" / "Combat" tabs
- `exp:gain` handler formats EXP differently for party vs solo:
  - Solo: `"Base EXP +X / Job EXP +Y"`
  - Party shared: `"Base EXP +X (originally Y, +Z% party bonus, N members) / Job EXP +..."`
- `AddCombatLogMessage` is now public (used by `PartySubsystem` to route `party:error` messages to chat)

### Files Created/Modified (Client) — ALL COMPLETE

| File | Action | Status |
|------|--------|--------|
| `UI/PartySubsystem.h` | NEW — UWorldSubsystem, FPartyMember (with SP/MaxSP), party state | DONE |
| `UI/PartySubsystem.cpp` | NEW — 8 socket handlers, party:load re-request, emit helpers | DONE |
| `UI/SPartyWidget.h` | NEW — Slate widget header | DONE |
| `UI/SPartyWidget.cpp` | NEW — full party window (HP/SP bars, create/invite/settings, context menu) | DONE |
| `SabriMMOCharacter.h` | MODIFY — P key TogglePartyAction | DONE |
| `SabriMMOCharacter.cpp` | MODIFY — P key toggle binding | DONE |
| `UI/ChatSubsystem.h` | MODIFY — AddCombatLogMessage made public | DONE |
| `UI/ChatSubsystem.cpp` | MODIFY — party:chat_receive handler, % prefix routing, exp:gain handler | DONE |

### Execution Order (Client) — ALL COMPLETE (except C12)

```
C1.  [DONE] FPartyMember (with SP/MaxSP) in PartySubsystem.h
C2.  [DONE] PartySubsystem — 8 socket handlers + party:load re-request
C3.  [DONE] SPartyWidget — RO brown/gold frame (title bar, nav buttons, close, drag-to-move)
C4.  [DONE] SPartyWidget — member list (name, HP/SP proportional bars, map, online/offline)
C5.  [DONE] SPartyWidget — leader indicator (crown icon, bold dark blue)
C6.  [DONE] SPartyWidget — HP bar colors (green > 25%, red ≤ 25%) + SP bar (blue)
C7.  [DONE] SPartyWidget — right-click context menu (whisper, kick, delegate, leave)
C8.  [DONE] SPartyWidget — click-to-target integration with SkillTreeSubsystem
C9.  [DONE] Create Party dialog (name input) + settings popup (EXP share toggle)
C10. [DONE] Invite dialog + Invite Received popup (accept/decline)
C11. [DONE] ChatSubsystem — party:chat_receive, % prefix routing, exp:gain party formatting
C12. [DEFERRED] Minimap party member dots (deferred until minimap system exists)
```
