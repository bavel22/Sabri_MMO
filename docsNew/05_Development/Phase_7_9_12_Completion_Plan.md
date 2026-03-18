# Phase 7, 9, 12 Completion Plan

**Created**: 2026-03-17
**Last Updated**: 2026-03-18
**Based on**: Full codebase audit of index.js (~27,000 lines), ro_monster_skills.js, ro_damage_formulas.js, ChatSubsystem.h/.cpp, MEMORY.md, RagnaCloneDocs, rAthena pre-re source cross-reference

## Implementation Progress

| Task | Status | Notes |
|------|--------|-------|
| 12.1 Death Penalty | **COMPLETE** | `applyDeathPenalty()`: 1% base + 1% job EXP loss, Novice exempt, wired into 5 PvE death paths. PvP excluded. Deep-research verified against rAthena `pc_dead()`. |
| 12.2 Non-elemental vs Neutral | **COMPLETE** | `isNonElemental` option in `calculatePhysicalDamage()`, passed from `calculateEnemyDamage()`. Monster skills still use element table. |
| 7.1 Server whisper routing | **COMPLETE** | `/w "Name" msg` parsing, target lookup, echo-back as WHISPER_SENT, auto-reply support |
| 7.2 Block/ignore system | **COMPLETE** | `/ex`, `/exall`, `/in`, `/inall` commands, in-memory block list per player |
| 7.3 Client whisper support | **COMPLETE** | `/w`, `/r`, `/ex`, `/exall`, `/in`, `/inall`, `/am` parsing in SendChatMessage, HandleChatError handler, "From/To Name : msg" display format |
| 7.4 Whisper color fix | **COMPLETE** | Changed from pink (1.0, 0.6, 0.8) to yellow/gold (1.0, 0.85, 0.0) |
| 9.1 Missing NPC skill handlers | **COMPLETE** | Added random_element, ranged_magic (NPC_DARKBREATH), status_ranged (NPC_DARKBLESSING/Coma). All 3 with death penalty + buff clear. |
| 9.2 MVP reward system | **COMPLETE** | `inCombatWith` converted Set→Map for damage tracking. MVP winner by highest damage. `mvpExp` awarded. 3 MVP drop rolls. Server-wide `[MVP]` announcement. `mvp:tombstone` event. |
| 9.3 Slave summoning | **COMPLETE** | NPC_SUMMONSLAVE spawns slaves with `_masterId`/`_isSlave` tracking. Slaves share master aggro, give no EXP/drops, despawn on master death. |
| 9.4 Monster skill DB expansion | **COMPLETE** | 8 MVPs/bosses added (GTB, Moonlight, Phreeoni, Orc Lord, Stormy Knight, Dark Lord, Abysmal Knight, Raydric). Total: 27 monsters with skills. rAthena→project ID translation via `RATHENA_TO_PROJECT_SKILL` map. |
| 9.5 Monster ground AoE | **COMPLETE** | `executeMonsterPlayerSkill` detects ground AoE skills and calls `createGroundEffect()`. Plagiarism hook fires. |
| 9.6 Cast sensor fix | **COMPLETE** | `_isCastTarget` set in skill:use handler. IDLE state aggros flagged monsters. Flag cleared after selection. |
| 9.x MVP combat data snapshot | **COMPLETE** | Fixed critical bug: `inCombatWith.clear()` was called BEFORE MVP check. Now snapshot via `new Map()` before clear. Added MVP rewards to auto-attack death path. |
| 9.x rAthena skill ID translation | **COMPLETE** | Fixed critical bug: player-class monster skills used rAthena IDs (26/28/89) that didn't match SKILL_MAP. Added `RATHENA_TO_PROJECT_SKILL` translation table (~28 entries). Plagiarism uses translated project IDs. |
| 9.x inCombatWith spread fix | **COMPLETE** | Fixed `[...enemy.inCombatWith]` → `[...enemy.inCombatWith.keys()]` for randomTarget AI. |
| 9.x MVP respawn variance | **COMPLETE** | MVP: 120-130min, Boss: 60-70min with random variance. Slaves don't respawn. |
| 9.x Death penalty in NPC skills | **COMPLETE** | Added death checks + `applyDeathPenalty()` to aoe_physical, self_destruct, random_element, ranged_magic, status DoT paths |

---

## Pre-Audit Findings: Systems Already Implemented

Two systems originally flagged as "not done" are **already fully implemented**:

### Boss/Normal Monster Card Category — IMPLEMENTED
- Every monster template has `monsterClass` field (`'normal'`, `'boss'`, `'mvp'`)
- `spawnEnemy()` sets `enemy.modeFlags.isBoss` and `enemy.modeFlags.mvp`
- `ro_damage_formulas.js` Step 6c (line 668-673): `cardAddClass` applies offensive bonus (e.g., Abysmal Knight Card +25% vs Boss)
- `ro_damage_formulas.js` Step 8d (line 815-821): `cardSubClass` applies defensive reduction (e.g., Alice Card -40% from Boss)
- DEF/MDEF bypass by class: `cardIgnoreDefClass`, `cardDefRatioAtkClass`, `cardIgnoreMdefClass`
- `ro_card_effects.js` parses `bAddClass` and `bSubClass` from card scripts
- Boss status immunity checked via `enemy.modeFlags.isBoss` in 10+ locations
- **No action needed.**

### Class Weapon Restrictions — IMPLEMENTED
- `JOB_EQUIP_NAMES` at index.js:3731-3753 maps each job class to allowed job name strings
- `canJobEquip()` at index.js:3761-3767 validates item's `jobs_allowed` comma-separated list against player's class
- `inventory:equip` handler at index.js:18772 calls `canJobEquip()` and rejects with error message
- Data-driven via the `jobs_allowed` column on each item row in the `items` database table (same approach as rAthena `item_db.yml` `Jobs:` field)
- **No action needed** — correctness depends on DB item data having accurate `jobs_allowed` values (already populated from rAthena canonical import).

---

## Phase 7 Completion: Whisper System

### Current State
- `EChatChannel::Whisper` enum exists in ChatSubsystem.h (unused)
- `ParseChannel("WHISPER")` mapping exists (unused)
- Server `chat:message` handler has no whisper case (commented stub: `// case 'TELL':`)
- Color is pink (1.0, 0.6, 0.8) — should be yellow/gold for RO Classic
- No `/w` command parsing anywhere
- No block/ignore system

### RO Classic Whisper Reference (pre-renewal, rAthena-verified)

| Feature | RO Classic Behavior |
|---------|-------------------|
| Send command | `/w "CharacterName" message` (quotes required for names with spaces) |
| Send via UI | Name box (left side of chat input) + message in main input |
| Received format | `From CharacterName : message` |
| Sent format | `To CharacterName : message` |
| Color | Yellow/Gold (NOT pink — pink is Renewal/private server convention) |
| Offline target | Error: "The Character is not currently online or does not exist." |
| Self-whisper | Silently dropped (no delivery, no error) |
| Block commands | `/ex Name` (block one), `/exall` (block all), `/in Name` (unblock one), `/inall` (unblock all) |
| Blocked error | `"Name doesn't want to receive your messages."` |
| Reply shortcut | Last whisperer name auto-fill |
| Auto-reply | `/am message` — auto-responds to whispers (AFK message) |
| Overhead bubble | None — whispers never show as overhead text |
| Rate limit | General chat flood control (existing 5/sec is fine) |
| Message length | ~70 chars client, 255 server cap |

### Implementation Tasks

#### 7.1 Server: Whisper Routing (index.js)

Add whisper handling to the `chat:message` handler:

```
Location: index.js, chat:message handler (~line 7581)

1. Parse whisper from message text:
   - If message starts with `/w "` → extract target name (between quotes) and message body
   - If message starts with `/w ` (no quotes) → first word after /w is target name
   - Alternative: client sends { channel: 'WHISPER', targetName: 'Name', message: 'text' }

2. Whisper routing logic:
   a. Block self-whisper: if targetName === senderName → silently drop
   b. Find target in connectedPlayers by character name (case-insensitive)
   c. If not found → emit chat:error { message: "The Character is not currently online or does not exist." }
   d. Check sender's block list: if target has blocked sender → emit chat:error { message: "Name doesn't want to receive your messages." }
   e. Check target's exall flag: if target.whisperBlockAll → emit chat:error { message: "Name is not in the mood to talk with anyone." }
   f. Send to target: emit chat:receive { channel: 'WHISPER', senderName, message, fromName: senderName }
   g. Echo to sender: emit chat:receive { channel: 'WHISPER_SENT', targetName, message, toName: targetName }
   h. Check auto-reply: if target.autoReplyMessage → emit chat:receive { channel: 'WHISPER', senderName: targetName, message: target.autoReplyMessage, fromName: targetName, isAutoReply: true }
```

#### 7.2 Server: Block/Ignore System (index.js)

```
Add to player object on join:
  player.whisperBlockList = new Set()   // blocked character names
  player.whisperBlockAll = false         // /exall flag
  player.autoReplyMessage = null         // /am message
  player.lastWhisperer = null            // for reply shortcut

New socket events:
  'whisper:block'     { targetName }     → add to blockList, confirm
  'whisper:block_all' {}                 → set whisperBlockAll = true
  'whisper:unblock'   { targetName }     → remove from blockList
  'whisper:unblock_all' {}               → set whisperBlockAll = false
  'whisper:auto_reply' { message }       → set autoReplyMessage (null to clear)
  'whisper:block_list' {}                → return current block list

Block list is in-memory only (resets on disconnect) — matches rAthena default behavior.
```

#### 7.3 Client: ChatSubsystem Updates (ChatSubsystem.cpp)

```
1. Fix whisper color:
   - Change EChatChannel::Whisper color from (1.0, 0.6, 0.8) to (1.0, 0.85, 0.0) — yellow/gold

2. Add /w command parsing in SendChatMessage():
   - If message starts with "/w " → parse target name and message
   - Emit chat:message with { channel: 'WHISPER', targetName, message }

3. Add whisper display formatting:
   - WHISPER channel: format as "From {senderName} : {message}"
   - WHISPER_SENT channel: format as "To {targetName} : {message}"

4. Add /ex, /exall, /in, /inall, /am command parsing:
   - "/ex Name" → emit whisper:block
   - "/exall" → emit whisper:block_all
   - "/in Name" → emit whisper:unblock
   - "/inall" → emit whisper:unblock_all
   - "/am message" → emit whisper:auto_reply

5. Add chat:error handler:
   - Register for chat:error event
   - Display error message in System color (yellow)

6. Track last whisperer:
   - On receiving a WHISPER message, store senderName
   - Add reply shortcut: "/r message" → auto-fill last whisperer as target

7. Add message length cap:
   - Client-side: truncate message to 255 characters before sending
```

#### 7.4 Scope Exclusions (defer)

| Feature | Reason to Defer |
|---------|----------------|
| Separate whisper window per contact | Complex UI, not needed for core functionality |
| Recent contacts dropdown button | Nice-to-have, not core |
| Whisper tab in chat window | Can filter by channel in existing All tab |
| `/savechat` command | Client-only file I/O, low priority |
| DB-persistent block lists | In-memory is fine for now (matches rAthena default) |

### Estimated Effort: 1-2 days

---

## Phase 9 Completion: Monster Skills & MVP System

### Current State
- 19/509 monsters have skill definitions in `ro_monster_skills.js`
- 14/17 NPC skill type handlers implemented in `executeNPCSkill()`
- 3 NPC skill types defined but **no handler**: `random_element`, `ranged_magic`, `status_ranged`
- 2 NPC skill types are **stubs**: `summon` (NPC_SUMMONSLAVE), `transform` (NPC_METAMORPHOSIS)
- Monster skill execution pipeline is complete (selectMonsterSkill → executeMonsterSkill → executeMonsterSkillEffect → executeNPCSkill/executeMonsterPlayerSkill)
- Cast times, cooldowns, conditions all working
- Self-healing (AL_HEAL target:self) working
- MVP templates exist (43 MVPs, 54 bosses in ro_monster_templates.js) but NO MVP reward system
- `mvpExp` field stored on templates but never awarded
- No damage tracking per player (inCombatWith is a Set, not a damage Map)
- Cast sensor (`casttargeted` condition) never fires (`_isCastTarget` never set to true)

### RO Classic Monster Skill Reference (pre-renewal, rAthena-verified)

**Monster attack capabilities in RO Classic:**
- Elemental melee (9 types) — IMPLEMENTED
- Status-inflicting melee (7 types) — IMPLEMENTED
- Self-healing — IMPLEMENTED
- AoE physical (Earthquake) — IMPLEMENTED
- Wide status effects (6 types) — IMPLEMENTED
- Teleport — IMPLEMENTED
- Player-class skills (copyable via Plagiarism) — IMPLEMENTED
- HP/SP drain — IMPLEMENTED
- Multi-hit, forced crit — IMPLEMENTED
- Self-destruct — IMPLEMENTED
- Slave summoning — STUB (critical gap)
- Metamorphosis/transform — STUB
- Ground AoE skills (Storm Gust, Meteor Storm, Fire Wall) — MISSING
- Monster-to-monster healing — MISSING
- Self-buffs (POWERUP, DEFENSE) — MISSING handlers
- Flee behavior (cowardly mobs run at low HP) — MISSING
- Cast sensor aggro — MISSING (flag parsed but never set)

**MVP System in RO Classic:**
- Damage tracking determines MVP winner (highest total damage)
- MVP winner gets: bonus EXP (`mvpExp`), 3 rolls on MVP drop table, "MVP" announcement
- MVP tombstone at death location (shows name, killer, time)
- Respawn timers with random variance (e.g., 120 ± 10 minutes)
- Server-wide announcement on MVP death
- Ground loot with ownership phases (MVP gets first pick for 10s)

### Implementation Tasks — Prioritized

#### 9.1 Missing NPC Skill Handlers (HIGH — blocks existing monster data)

Add 3 missing case handlers in `executeNPCSkill()`:

```
1. NPC_RANDOMATTACK (random_element):
   - Pick random element from [fire, water, earth, wind, poison, holy, dark, ghost, undead]
   - Apply as elemental melee with random element modifier
   - Used by: currently no monster, but needed for future monsters

2. NPC_DARKBREATH (ranged_magic):
   - Ranged dark-element magic attack
   - Formula: ATK * skillLevel * (element modifier)
   - Apply as magical damage with Dark element
   - Used by: Dark Lord, Detardeurus

3. NPC_DARKBLESSING (status_ranged):
   - Inflicts Coma (HP=1, SP=1) with chance based on level
   - RO Classic: 50% chance at Lv5, reduced by target LUK
   - Boss targets immune
   - Used by: Osiris, Lord of Death
```

#### 9.2 MVP Reward System (HIGH — core endgame feature)

```
1. Damage tracking:
   - Change inCombatWith from Set<charId> to Map<charId, { totalDamage: number, name: string }>
   - Update ALL damage paths to accumulate:
     a. Auto-attack combat tick (physical + dual wield)
     b. executePhysicalSkillOnEnemy()
     c. calculateAndApplySkillDamage() (magical)
     d. trap damage
     e. ground effect damage (Storm Gust, LoV, etc.)
     f. Blitz Beat / falcon damage
     g. Reflected damage (Reflect Shield)
   - Reset on enemy respawn

2. MVP determination on death:
   - In processEnemyDeathFromSkill() and the auto-attack death path:
   - If enemy.monsterClass === 'mvp':
     a. Find player with highest totalDamage in inCombatWith
     b. Award mvpExp to that player (bonus, on top of normal kill EXP)
     c. Roll 3 MVP drops (each with independent chance), give to MVP winner
     d. Broadcast zone-wide: chat:receive { channel: 'SYSTEM', message: "MVP: [PlayerName] killed [BossName]!" }
     e. Broadcast server-wide: io.emit('mvp:announcement', { playerName, bossName, zone })
     f. Store lastMvpKiller, lastMvpTime on the spawn point for tombstone

3. MVP tombstone:
   - On MVP death, emit enemy:tombstone { x, y, z, bossName, killerName, time }
   - Client: show a visual marker at death location (can be simple text overlay initially)
   - Remove tombstone 5 seconds after MVP respawns

4. MVP respawn variance:
   - Change fixed respawnMs to: baseRespawnMs + Math.floor(Math.random() * varianceMs)
   - MVPs: 7200000 base (120 min) + random 0-600000 (0-10 min) = 120-130 min
   - Mini-bosses: 3600000 base (60 min) + random 0-600000 = 60-70 min
   - Normal bosses: keep current 30-min fixed (or add small variance)
```

#### 9.3 Slave Summoning (HIGH — required for many MVPs)

```
Replace NPC_SUMMONSLAVE stub with full implementation:

1. Slave spawn:
   - Read slaveIds from skill data (array of monster template IDs)
   - Spawn N slaves at random positions within 200 UE units of master
   - Slaves inherit master's zone
   - Mark slaves: slave._masterId = master.id, slave._isSlave = true
   - Track on master: master._slaves = Set<slaveId>, master._slaveCount

2. Slave behavior:
   - Slaves share master's aggro target (if master is attacking, slaves attack same target)
   - Slaves have their own AI tick (normal monster AI)
   - Slaves do NOT give EXP or drops when killed
   - Slaves do NOT count toward spawn limits

3. Slave lifecycle:
   - On master death: all slaves despawn immediately
   - On slave death: remove from master._slaves, do NOT respawn slave independently
   - Master can re-summon (NPC_CALLSLAVE) to refill slave count

4. Master-slave tracking:
   - master._maxSlaves (from skill data, typically 3-5)
   - NPC_SUMMONSLAVE only fires if master._slaveCount < master._maxSlaves
```

#### 9.4 Expand Monster Skill Database (MEDIUM — content, not systems)

Add skill entries for high-priority monsters that players will encounter:

```
Priority 1 — Active zone 1-3 monsters missing skills (should have at least 1):
  Check which of the 46 active spawns lack skills beyond the current 10.
  Many low-level monsters only auto-attack in RO Classic too, so not all need skills.

Priority 2 — Key MVPs (for when zones expand):
  Golden Thief Bug (1086): AL_HEAL, NPC_FIREATTACK, NPC_MAGICIMMUNE
  Moonlight Flower (1150): MG_FIREBOLT, MG_COLDBOLT, MG_LIGHTNINGBOLT, AL_HEAL, MC_MAMMONITE
  Orc Lord (1190): NPC_EARTHQUAKE, NPC_POWERUP, NPC_SUMMONSLAVE
  Dark Lord (1272): MG_METEORSTORM, MG_FIREWALL, NPC_DARKBREATH, NPC_SUMMONSLAVE
  Phreeoni (1159): NPC_COMBOATTACK, KN_BRANDISHSPEAR, AL_TELEPORT
  Stormy Knight (1251): WZ_STORMGUST, WZ_JUPITELTHUNDER, MG_COLDBOLT

Priority 3 — Notable boss monsters:
  Abysmal Knight (1219): KN_BRANDISHSPEAR, KN_PIERCE, CR_AUTOGUARD
  Raydric (1163): SM_BASH, SM_ENDURE, KN_TWOHANDQUICKEN
```

#### 9.5 Monster Ground AoE Skills (MEDIUM — needed for boss MVPs)

```
Currently executeMonsterPlayerSkill() only handles single-target offensive spells.
Need to add ground effect creation for monster-cast AoE:

1. Extend executeMonsterPlayerSkill() to detect ground AoE skills:
   - WZ_STORMGUST, WZ_METEORSTORM, MG_FIREWALL, WZ_VERMILION, PR_SANCTUARY, etc.
   - These should call createGroundEffect() with the monster as owner
   - Ground effect tick already handles damage to players in range

2. Monster targeting for ground AoE:
   - Target position = current target's position (cast at player location)
   - Cast time from skill data (interruptible based on cancelable flag)

3. Ground effects from monsters should:
   - Use the same createGroundEffect() as player skills
   - Set owner to the enemy object (need minor refactor — currently assumes player owner)
   - Apply damage/effects to players only (not other monsters)
```

#### 9.6 Cast Sensor Fix (LOW — behavioral accuracy)

```
The casttargeted condition always returns false because _isCastTarget is never set.

Fix: In the skill:use handler (when a player starts casting), iterate nearby enemies:
  - For each enemy within aggro range that has castSensorIdle or castSensorChase AI flag:
  - Set enemy._isCastTarget = true
  - This triggers casttargeted conditions in selectMonsterSkill()
  - Clear _isCastTarget after the monster reacts (or after 2 seconds)
```

#### 9.7 Additional NPC Skills (LOW — future expansion)

```
Add to NPC_SKILLS and executeNPCSkill() when needed:

NPC_POWERUP:     Self-buff, +ATK% for duration (used by Orc Lord, Pharaoh, etc.)
NPC_DEFENSE:     Self-buff, +DEF for duration
NPC_CALLSLAVE:   Recall existing slaves to master position
NPC_REBIRTH:     On death, transform into different monster (e.g., Mastering → Angeling)

These can be added incrementally as specific monsters are placed in new zones.
```

#### 9.8 Scope Exclusions (defer to later)

| Feature | Reason to Defer |
|---------|----------------|
| Ground loot with ownership phases | Major inventory system change, not needed for core MVP |
| NPC_BARRIER / NPC_INVINCIBLE | Very few monsters use these, add when needed |
| NPC_METAMORPHOSIS | Only a handful of monsters transform, low priority |
| Flee AI state (cowardly mobs) | Minor behavioral accuracy, most mobs fight to death |
| Monster phase transitions | Can be simulated with HP-threshold skill conditions (already working) |
| Breath attacks (fire/ice/thunder/acid) | Only dragons use these, add with dragon zones |
| Monster-to-monster healing | Only Osiris (heals other undead), defer to Osiris's zone |

### Estimated Effort: 5-7 days

---

## Phase 12 Completion: Remaining Items

### Current State

Already done:
- Card system (538/538 cards, 65 bonus types) — DONE
- Weight system (50%/90%/100% thresholds) — DONE
- Dual wield — DONE
- Item descriptions audit — DONE
- Item inspect UI — DONE
- Item icon generation — DONE
- Ammunition system — DONE
- Refine ATK — DONE
- Forging — DONE
- Cart/Vending/Identification — DONE
- Boss/Normal card category — DONE (verified in pipeline)
- Class weapon restrictions — DONE (via jobs_allowed)

Remaining:
1. **Death Penalty** — NOT IMPLEMENTED (verified: zero EXP deduction code exists)
2. **Non-elemental vs Neutral** — NOT IMPLEMENTED (monster auto-attacks incorrectly use element table)

### 12.1 Death Penalty (PvE Only)

#### RO Classic Reference (pre-renewal, rAthena-verified)

| Rule | Detail |
|------|--------|
| PvE death | Lose 1% of EXP needed for current base level |
| PvP death | No EXP loss |
| WoE death | No EXP loss |
| Minimum EXP | 0 (can't go below 0% — no deleveling) |
| Job EXP | NOT affected (only base EXP) |
| Resurrection skill | Does NOT reduce penalty in pre-renewal (all levels lose full 1%) |
| Token of Siegfried | Resurrects with no EXP penalty (item consumption) |

#### Implementation

```
Location: index.js — All player death paths

1. PvE death from auto-attack (enemy kills player, ~line 25635):
   Add before or after clearBuffsOnDeath():
     const expForLevel = getBaseExpForNextLevel(player.stats.level);
     const penalty = Math.floor(expForLevel * 0.01);  // 1%
     player.baseExp = Math.max(0, player.baseExp - penalty);
     // Save to DB
     await pool.query('UPDATE characters SET base_exp = $1 WHERE id = $2',
       [player.baseExp, player.id]);
     // Notify client of updated EXP
     socket.emit('player:stats', buildFullStatsPayload(player));

2. PvE death from monster skill (~line 24349):
   Same penalty logic as above.

3. PvE death from ground effects (DoT kills player):
   Same penalty logic.

4. PvP death (~line 21853):
   NO penalty. Already correct by omission.

5. Client display:
   No client changes needed — player:stats already updates EXP bars.
   Optionally: add system message "You lost X base EXP." in combat log.

Helper function:
  function applyDeathPenalty(player, socket) {
    const expForLevel = getBaseExpForNextLevel(player.stats.level);
    const penalty = Math.floor(expForLevel * 0.01);
    if (penalty > 0 && player.baseExp > 0) {
      player.baseExp = Math.max(0, player.baseExp - penalty);
      pool.query('UPDATE characters SET base_exp = $1 WHERE id = $2',
        [player.baseExp, player.id]);
      socket.emit('player:stats', buildFullStatsPayload(player));
    }
  }

Call from all PvE death paths. Do NOT call from PvP death paths.
```

### 12.2 Non-elemental vs Neutral Distinction

#### RO Classic Reference (pre-renewal, rAthena-verified)

| Attack Type | Element Handling |
|-------------|-----------------|
| Player auto-attack | Uses weapon element (default Neutral). Applies element table. Ghost armor = 25% from Neutral. |
| Player skill with forced element | Uses skill element (Fire Bolt = Fire). Applies element table. |
| Player skill with weapon element | Uses weapon element. Applies element table. |
| Monster auto-attack | **Non-elemental. Bypasses element table entirely. Always 100% damage.** |
| Monster skill (NPC_ elemental) | Uses skill element. Applies element table normally. |
| Monster player-class skill | Uses skill element. Applies element table normally. |

**Why this matters:** Ghostring Card (Ghost Lv1 armor) in RO Classic:
- Reduces Neutral player attacks to 25% — intended
- Does NOT reduce monster auto-attacks — they bypass the table
- Currently in Sabri_MMO: Ghostring would reduce BOTH to 25% (incorrect)

#### Implementation

```
Location: ro_damage_formulas.js — calculatePhysicalDamage()

1. Add option flag:
   In the options parameter of calculatePhysicalDamage(), add:
     isNonElemental: false  // default

2. Skip element modifier when flag is set:
   At the element modifier step (~line 706):
     if (!options.isNonElemental) {
       elementModifier = getElementModifier(atkElement, targetElement, targetElementLevel);
     } else {
       elementModifier = 1.0;  // always 100%, bypass table
     }

3. Pass flag from monster auto-attack:
   In calculateEnemyDamage() (~line 24710-24744):
     Pass { isNonElemental: true } to calculatePhysicalDamage()

4. Do NOT pass flag from monster skills:
   In executeNPCSkill() and executeMonsterPlayerSkill():
     These should NOT set isNonElemental — monster skills use the element table normally.
     Elemental melee skills (NPC_FIREATTACK etc.) use their specified element through the table.

5. Monster auto-attack element:
   When isNonElemental is true, the attackElement value doesn't matter because the table is bypassed.
   But for card effects and other checks that reference atkElement, still pass the monster's body element.
```

### Estimated Effort: 1 day

---

## Combined Priority Order

| # | Task | Phase | Priority | Effort | Impact |
|---|------|-------|----------|--------|--------|
| 1 | Death penalty (PvE 1% base EXP loss) | 12 | HIGH | 2 hours | Core RO Classic mechanic, affects risk/reward |
| 2 | Non-elemental vs Neutral distinction | 12 | HIGH | 2 hours | Ghostring Card and Ghost armor broken without this |
| 3 | Whisper system (server routing + client parsing) | 7 | HIGH | 1 day | Core social feature, only missing chat channel |
| 4 | Block/ignore system (/ex, /exall, /in, /inall) | 7 | MEDIUM | 3 hours | Part of whisper, standard RO feature |
| 5 | Whisper color fix (pink → yellow/gold) | 7 | LOW | 5 min | Visual accuracy |
| 6 | MVP reward system (damage tracking + EXP + drops) | 9 | HIGH | 2 days | Core endgame feature |
| 7 | Missing NPC skill handlers (3 types) | 9 | HIGH | 3 hours | Blocks existing monster data |
| 8 | Slave summoning system | 9 | HIGH | 1 day | Required for Baphomet, Orc Lord, Dark Lord |
| 9 | MVP tombstone + announcement | 9 | MEDIUM | 3 hours | Endgame polish |
| 10 | MVP respawn variance | 9 | MEDIUM | 30 min | Prevents camping predictability |
| 11 | Expand monster skill DB (priority MVPs) | 9 | MEDIUM | 1 day | Content, not systems |
| 12 | Monster ground AoE skills | 9 | MEDIUM | 1 day | Needed for Storm Gust/Meteor Storm MVPs |
| 13 | Cast sensor fix | 9 | LOW | 1 hour | Behavioral accuracy for AI codes 9/21 |
| 14 | Auto-reply (/am) | 7 | LOW | 30 min | Nice-to-have AFK feature |
| 15 | Reply shortcut (/r) | 7 | LOW | 30 min | Convenience |
| 16 | Additional NPC skills (POWERUP, etc.) | 9 | LOW | Incremental | Add with specific monsters |

### Total Estimated Effort: 8-10 days

---

## Execution Order

### Week 1: Phase 12 + Phase 7

**Day 1 (Phase 12 — quick wins):**
- [ ] 12.1: Death penalty — `applyDeathPenalty()` helper, wire into all PvE death paths
- [ ] 12.2: Non-elemental flag — add `isNonElemental` to calculatePhysicalDamage(), pass from calculateEnemyDamage()
- [ ] Verify: Ghostring Card reduces player Neutral attacks but NOT monster auto-attacks

**Day 2-3 (Phase 7 — whisper):**
- [ ] 7.1: Server whisper routing in chat:message handler
- [ ] 7.2: Server block/ignore system (whisper:block, whisper:block_all, etc.)
- [ ] 7.3: Client `/w`, `/ex`, `/exall`, `/in`, `/inall`, `/r` command parsing
- [ ] 7.3: Client whisper display format ("From Name : msg" / "To Name : msg")
- [ ] 7.3: Fix whisper color to yellow/gold
- [ ] 7.3: Client chat:error handler for offline/blocked messages

### Week 2: Phase 9

**Day 4-5 (Phase 9 — MVP system):**
- [ ] 9.1: Add 3 missing NPC skill handlers (random_element, ranged_magic, status_ranged)
- [ ] 9.2: Convert inCombatWith from Set to Map for damage tracking
- [ ] 9.2: Wire damage accumulation into all damage paths
- [ ] 9.2: MVP determination on death (highest damage = MVP winner)
- [ ] 9.2: Award mvpExp, roll MVP drops, broadcast announcement
- [ ] 9.2: MVP tombstone emit + respawn variance

**Day 6-7 (Phase 9 — slave summoning + content):**
- [ ] 9.3: Implement NPC_SUMMONSLAVE (spawn slaves, master-slave tracking, despawn on master death)
- [ ] 9.4: Add skill entries for key MVPs (Golden Thief Bug, Moonlight Flower, Orc Lord, Dark Lord, Phreeoni, Stormy Knight)
- [ ] 9.5: Monster ground AoE — extend executeMonsterPlayerSkill() for createGroundEffect()
- [ ] 9.6: Cast sensor fix (_isCastTarget in skill:use handler)

---

## Verification Checklist

### Phase 7 Verification
- [ ] `/w "PlayerName" hello` sends whisper to target
- [ ] `/w PlayerName hello` works without quotes for single-word names
- [ ] Sent whisper shows "To PlayerName : hello" in yellow
- [ ] Received whisper shows "From PlayerName : hello" in yellow
- [ ] Whisper to offline player shows error message
- [ ] Self-whisper is silently dropped
- [ ] `/ex PlayerName` blocks whispers from that player
- [ ] Blocked player sees "PlayerName doesn't want to receive your messages."
- [ ] `/exall` blocks all whispers; senders see appropriate error
- [ ] `/in PlayerName` unblocks; `/inall` unblocks all
- [ ] `/r message` replies to last whisperer
- [ ] Party chat via `%` still works (no regression)
- [ ] Global chat still works (no regression)

### Phase 9 Verification
- [ ] Existing monster skills (Hornet poison, Zombie stun, etc.) still work
- [ ] NPC_DARKBREATH deals dark magic damage
- [ ] NPC_DARKBLESSING inflicts Coma (HP=1, SP=1)
- [ ] NPC_RANDOMATTACK picks random element each use
- [ ] MVP death awards bonus EXP to highest-damage player
- [ ] MVP death rolls 3 MVP drops for winner
- [ ] MVP death broadcasts zone-wide announcement
- [ ] MVP tombstone appears at death location
- [ ] MVP respawns within correct variance window (120-130 min)
- [ ] NPC_SUMMONSLAVE spawns slave monsters
- [ ] Slaves attack master's target
- [ ] Slaves despawn when master dies
- [ ] Slaves give no EXP or drops
- [ ] Damage tracking accumulates correctly across all damage sources
- [ ] inCombatWith Map resets on enemy respawn

### Phase 12 Verification
- [ ] PvE death deducts 1% base EXP for current level
- [ ] EXP cannot go below 0 (no deleveling)
- [ ] PvP death does NOT deduct EXP
- [ ] Client EXP bar updates immediately after death
- [ ] Monster auto-attacks bypass element table (always 100% damage)
- [ ] Player Neutral attacks still use element table (Ghost armor = 25%)
- [ ] Monster elemental skills (NPC_FIREATTACK etc.) still use element table
- [ ] Ghostring Card: reduces player attacks, does NOT reduce monster auto-attacks

---

## Systems Confirmed Working (No Action Needed)

These were audited and confirmed fully implemented:

| System | Evidence |
|--------|---------|
| Boss/Normal card category | `cardAddClass`/`cardSubClass` in damage pipeline, `modeFlags.isBoss` |
| Class weapon restrictions | `canJobEquip()` + `jobs_allowed` DB column |
| Monster skill execution pipeline | selectMonsterSkill → executeMonsterSkill → executeMonsterSkillEffect |
| Monster cast times + cooldowns | `_casting` system with `processMonsterCasting()` |
| Monster self-healing (AL_HEAL) | Handler in executeMonsterPlayerSkill() |
| Plagiarism copy from monsters | `checkPlagiarismCopy()` in executeMonsterPlayerSkill() |
| Boss status immunity | Checked via `modeFlags.isBoss`/`statusImmune` in 10+ locations |
| Boss knockback immunity | `knockbackImmune` flag set on spawn |
| Monster AI conditions (15 types) | HP thresholds, target count, status checks, etc. |
| HP/SP drain attacks | NPC_BLOODDRAIN, NPC_ENERGYDRAIN |
| Self-destruct | NPC_SELFDESTRUCTION |
| Card system (538/538) | Full pipeline with 65 bonus types |
| Weight thresholds (50%/90%/100%) | Cached weight, 12 mutation points |
| Refine ATK (+2/+3/+5/+7) | Post-DEF in damage pipeline, overupgrade random bonus |
| Ammunition system | Element priority, consumption, Arrow Crafting |
| Cart/Vending/Identification | Full DB-backed system |
| Forging system | FORGE_RECIPES, element stones, star crumbs |

---

## Deep-Research Corrections Applied (2026-03-18)

Post-implementation audit against rAthena source code revealed 3 gaps. All fixed:

| Issue | Source | Fix Applied |
|-------|--------|-------------|
| Death penalty missing job EXP loss | rAthena `death_penalty_job: 100` in exp.conf | Added 1% job EXP deduction to `applyDeathPenalty()` |
| Novice class not exempt from death penalty | rAthena `(sd->class_&MAPID_SECONDMASK) != MAPID_NOVICE` | Added `jobClass === 'novice'` early return |
| Whisper offline error message text wrong | rAthena msgstringtable: "There is no such character name or the user is offline." | Fixed string in chat:message handler |
| Whisper display format missing parentheses | Official client msgstringtable: "( From Name: ) message" | Updated ChatSubsystem.cpp format strings |

### Skills Created
- `/sabrimmo-death` — Death penalty system reference (rules, exemptions, formula, death paths)
- `/sabrimmo-mvp` — MVP reward system + monster skills + slave summoning reference

---

## Remaining Content Plan: 9.4, 9.5, 9.6

### 9.4 Monster Skill Database Expansion

**Scope:** Add skill entries to `ro_monster_skills.js` for key monsters. This is pure content (data entry), not systems work — all handlers are already implemented.

**Priority 1 — Active zone 1-3 monsters (add when verified missing skills):**
Most Lv1-20 monsters only auto-attack in RO Classic. The current 19 monsters with skills cover the right ones.

**Priority 2 — Key MVPs (add when zones expand to include their areas):**

| Monster | ID | Level | Skills to Add | Zone Needed |
|---------|-----|-------|--------------|-------------|
| Golden Thief Bug | 1086 | 64 | AL_HEAL, NPC_FIREATTACK | Prontera Sewers B4 |
| Moonlight Flower | 1150 | 67 | MG_FIREBOLT/COLDBOLT/LIGHTNINGBOLT, AL_HEAL, MC_MAMMONITE | Payon Cave |
| Phreeoni | 1159 | 69 | NPC_COMBOATTACK, KN_BRANDISHSPEAR, AL_TELEPORT | Morroc Field |
| Orc Lord | 1190 | 74 | NPC_EARTHQUAKE, NPC_SUMMONSLAVE(Orc Warrior) | Orc Dungeon |
| Dark Lord | 1272 | 80 | MG_METEORSTORM, MG_FIREWALL, NPC_DARKBREATH, NPC_SUMMONSLAVE | Glast Heim |
| Stormy Knight | 1251 | 75 | WZ_STORMGUST, WZ_JUPITELTHUNDER, MG_COLDBOLT | Toy Factory |

**Priority 3 — Notable boss monsters:**

| Monster | ID | Level | Skills to Add |
|---------|-----|-------|--------------|
| Abysmal Knight | 1219 | 63 | KN_BRANDISHSPEAR, KN_PIERCE, CR_AUTOGUARD |
| Raydric | 1163 | 52 | SM_BASH, SM_ENDURE, KN_TWOHANDQUICKEN |
| Stormy Knight | 1251 | 75 | WZ_STORMGUST, WZ_JUPITELTHUNDER |

**Effort:** ~1 day per priority tier (data entry + testing)

### 9.5 Monster Ground AoE Skills

**What:** Enable monsters to cast ground-effect skills (Storm Gust, Meteor Storm, Fire Wall, Sanctuary).

**Current gap:** `executeMonsterPlayerSkill()` only handles single-target offensive magic. Ground AoE skills need `createGroundEffect()` with the monster as owner.

**Implementation:**
1. In `executeMonsterPlayerSkill()`, detect ground AoE skill IDs:
   - WZ_STORMGUST (808), WZ_METEORSTORM (807), MG_FIREWALL (209)
   - WZ_VERMILION (805), PR_SANCTUARY (1003), MG_FIREPILLAR (810)
2. For these skills, call `createGroundEffect()` with:
   - Position: current target player's location
   - Owner: enemy object (need minor refactor — owner is currently assumed to be player)
   - Duration/ticks from existing ground effect config
3. Refactor `createGroundEffect()` owner to accept either player or enemy object
4. Ground effect tick damage to players already works (existing handler)

**Blocked by:** Need to verify `createGroundEffect()` can handle non-player owners. May need an `ownerType` field.

**Effort:** ~1 day

### 9.6 Cast Sensor Fix

**What:** Enable `casttargeted` condition so monsters with AI codes 9/21 (Isis, Raydric, MVPs) aggro when players cast spells nearby.

**Current gap:** `_isCastTarget` is never set to `true` by any code path, so the condition always returns `false`.

**Implementation:**
1. In the `skill:use` handler (when a player starts casting), after cast time begins:
   - Iterate enemies in the player's zone within aggro range
   - For each enemy with `castSensorIdle` or `castSensorChase` AI flag:
     - Set `enemy._isCastTarget = true`
     - Set `enemy._castTargetPlayerId = characterId`
2. In `selectMonsterSkill()`, the `casttargeted` condition already checks `enemy._isCastTarget`
3. After the monster reacts (selects a skill or 2 seconds pass), clear `_isCastTarget`

**Effort:** ~2 hours

### Implementation Order for 9.4-9.6

These are all **content-gated** — they only matter when specific monsters are placed in specific zones:

1. **9.6 Cast sensor** (2 hours) — Do first, benefits all future monsters
2. **9.5 Monster ground AoE** (1 day) — Do second, unblocks Storm Gust/Meteor Storm MVPs
3. **9.4 Monster skill DB** (1+ days) — Do when adding zones that contain these monsters

**Total remaining effort: 2-3 days** (can be done incrementally as zones expand)
