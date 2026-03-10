# Sabri_MMO — Strategic Implementation Plan v3

**Created**: 2026-03-09
**Last Updated**: 2026-03-10
**Based on**: Full codebase audit, skills/VFX audit, all RagnaCloneDocs, all docsNew/, complete server + client C++ analysis, external architecture review, Persistent Socket Connection Plan

## Progress Tracker

| Phase | Status | Completed | Notes |
|-------|--------|-----------|-------|
| Phase 0: Critical Fixes | **COMPLETE** | 2026-03-09 | All 6 fixes verified: JWT auth, Butterfly Wing, shop transactions, SP timing, hotbar idempotent, socket rate limiting |
| Phase 1: Server Extraction | DEFERRED | — | New modules created instead (ro_status_effects.js, ro_buff_system.js); full extraction deferred — AI-assisted dev neutralizes monolith pain |
| Phase 2: Status Effects + Buffs | **COMPLETE** | 2026-03-09 | 10 status effects, buff system, BuffBarWidget, AI CC lock, reconnect cache |
| Phase 3: Element Table + Formula Audit | **COMPLETE** | 2026-03-09 | Replaced entire ELEMENT_TABLE (100+ wrong values), verified SIZE_PENALTY (all correct), fixed card modifier stacking, added element immunity/heal hit types, 537 tests passing |
| Phase 4: Persistent Socket Connection | **COMPLETE** | 2026-03-10 | Persistent socket on GameInstance, SocketEventRouter, MultiplayerEventSubsystem bridge, PositionBroadcastSubsystem, reconnectBuffCache removed, all 14+ subsystems migrated |
| Phase 5: Passive Skills + Classes | NOT STARTED | — | Passive engine + 4 new classes (Archer, Acolyte, Thief, Merchant) |
| Phase 6: Party System | NOT STARTED | — | Party create/invite/leave, EXP sharing, HP broadcasting |
| Phase 7: Chat Expansion | NOT STARTED | — | Whisper, party chat, chat channels |
| Phase 8: Second Classes | NOT STARTED | — | Knight, Priest, Wizard, Assassin, Hunter |
| Phase 9: Monster Skills | NOT STARTED | — | Monster skill AI, MVP bosses |
| Phase 10: World Expansion | NOT STARTED | — | 15-20 zones from current 4 |
| Phase 11: Quests | NOT STARTED | — | NPC dialogue engine, job change quests |
| Phase 12: Items Deep Dive | NOT STARTED | — | Refining, cards, weight, 500+ items |

---

## Executive Summary

The project has a **solid foundation**: auth, 15 C++ UI subsystems, 4 working zones, auto-attack + 17 skills with VFX, RO damage formulas, 509 monster templates, inventory/equipment/hotbar, NPC shops, and a complete status effect + buff system. The architecture (server-authoritative, UWorldSubsystem pattern, event wrapping) is sound.

**The core problem**: Only **10% of skills work**, only **2 of 13 classes are playable** (Swordsman, Mage), and **zero social systems exist** (no party, guild, chat channels, trading). The game is a tech demo, not an MMO.

**The strategy**: Fix the element table (combat math is wrong), then lay the permanent networking foundation (persistent socket), then build **all 6 first classes** with passive skills, then add party play. Infrastructure before content. Correct data before more features.

---

## Guiding Principles

1. **Correct data before more features** — The element table has confirmed errors affecting every damage calculation. No point adding 40 skills on top of wrong combat math.

2. **Infrastructure before content** — Persistent socket connection benefits every future phase. Build it once, then every new subsystem, zone change, buff, and party feature works on a solid foundation.

3. **Build generic systems before specific content** — A proper status effect engine takes 1 week but unblocks 100+ skills. Hardcoding each status effect individually would cost 10x more time and require refactoring.

4. **Prioritize what enables group play** — An MMO without parties, healers, and chat is a single-player game with spectators. Acolyte/Priest + Party system are the highest-impact features.

5. **Defer what doesn't block gameplay** — Art, audio, PvP/WoE, companions, and vending can wait. They don't block core progression or party play.

6. **Minimal server modularization, not full rewrite** — The monolith works. AI-assisted development neutralizes most arguments for splitting index.js. Extract when actual pain appears, not proactively.

---

## Phase 0: Critical Fixes (1-2 days) — COMPLETE ✓

> **Completed 2026-03-09.** All 6 fixes verified.

| Fix | Severity | Effort | Impact |
|-----|----------|--------|--------|
| Auth bypass — `player:join` proceeds without JWT | CRITICAL | 3 lines | Any socket client can impersonate any character |
| Butterfly Wing — `WHERE id =` instead of `WHERE character_id =` | CRITICAL | 2 lines | Crashes on every use AND consumes item |
| Butterfly Wing — missing z-coordinate in UPDATE | CRITICAL | 1 line | Position save incomplete |
| Shop buy/sell — no DB transactions | HIGH | ~20 lines | Item duplication possible |
| SP deduction timing — deduct on cast COMPLETE, not START | HIGH | ~10 lines | Players lose SP on interrupted casts (deviates from RO) |
| Hotbar normalization — not idempotent | LOW | ~5 lines | Runs unnecessary queries on every restart |
| Socket rate limiting — none on socket events | HIGH | ~30 lines | DoS via spam socket events |

**Deliverable**: Zero known critical/high security issues. Butterfly Wing works. SP timing matches RO Classic.

---

## Phase 1: Targeted Server Extraction — DEFERRED

**Original rationale**: The server is ~8,800 lines in one file. Extract skill handlers into separate modules.

**Why deferred**: AI-assisted development neutralizes the core arguments:
- "Can't find anything in 8,800 lines" — AI navigates any file size instantly
- "Merge conflicts" — solo developer + AI, no team contention
- "No unit testing possible" — test suite deferred anyway, tests written alongside features
- "Syntax error crashes everything" — real but mitigated by nodemon auto-restart (<2s)

**What was done instead**: New domain modules created as needed (`ro_status_effects.js`, `ro_buff_system.js`) without restructuring the monolith. This approach works — extract when the growth actually hurts, not proactively.

**Revisit when**: Adding 50+ skill handlers in Phase 8 (Second Classes) makes the skill:use section unwieldy.

---

## Phase 2: Generic Status Effect & Buff System (1.5-2 weeks) — COMPLETE ✓

> **Completed 2026-03-09.** All deliverables met. See `docsNew/03_Server_Side/Status_Effect_Buff_System.md` and `docsNew/02_Client_Side/C++_Code/14_BuffBarSubsystem.md` for implementation details.
>
> **What was built**: `ro_status_effects.js` (10 effects with resistance formulas, periodic drains), `ro_buff_system.js` (generic buff system with stacking rules), `getCombinedModifiers()` in index.js, `BuffBarSubsystem` + `SBuffBarWidget` (client UI), AI CC lock, movement lock, regen blocking, damage-break-all-statuses, debug test commands, `reconnectBuffCache` for zone change persistence, `buff:list`/`buff:request` socket events.
>
> **Skills created**: `/sabrimmo-buff`, `/sabrimmo-debuff`

**Server-side scope** (completed):
- Generic `StatusEffectEngine` — apply, tick, resist, cleanse, expire, stack rules
- 10 core status effects: Stun, Freeze, Petrify, Poison, Silence, Blind, Sleep, Curse, Bleeding, Coma
- Resistance formulas: STR-based Stun resist, VIT-based Poison resist, INT-based Silence resist, etc.
- Buff system: apply, duration, stack rules (overwrite vs stack vs refresh), stat modifications, cleanse
- Integration: refactored existing hardcoded effects (Frost Diver freeze, Stone Curse petrify, Provoke buff) into the generic system
- Socket events: `status:applied`, `status:removed`, `status:tick`, `buff:applied`, `buff:removed`, `buff:list`
- Player movement locked during Stun/Freeze/Petrify/Sleep
- Flee penalty for multiple attackers (RO Classic: -10% per additional attacker after first)

**Client-side scope** (completed):
- **Buff Bar UI** — `BuffBarSubsystem` (UWorldSubsystem + Slate widget, Z=11). Text abbreviation icons with countdown timers, 4-tick stability delay.
- Socket event wrapping for `status:*` and `buff:*` events
- Zone persistence via `reconnectBuffCache` (30s TTL)

---

## Phase 3: Element Table & Formula Audit (half day) — COMPLETE

> **Completed 2026-03-09.** All deliverables met. 537 tests passing.
>
> **What was done**:
> 1. Replaced entire `ELEMENT_TABLE` (100+ wrong values) with canonical rAthena `db/pre-re/attr_fix.yml` values, cross-referenced with iRO Wiki Classic, RateMyServer, MuhRO Wiki, and Hercules pre-re
> 2. Verified `SIZE_PENALTY` table — all 18 weapon types × 3 sizes match rAthena canonical (100% correct, no changes needed)
> 3. Fixed card modifier stacking: was all-additive, now per-category multiplicative (race × ele × size) matching rAthena `cardfix` formula
> 4. Added `elementImmune` and `elementHeal` hit types (replacing generic `miss` for element interactions)
> 5. Comprehensive test suite: `server/tests/test_element_table.js` — 537 tests covering all 100 Lv1 cells, all 100 Lv4 cells, key Lv2-3 interactions, size penalties, card stacking, edge cases
>
> **Key corrections at Level 1**: Fire/Water/Earth/Wind weak-side was 90% (correct: 50%), Ghost→Neutral was 100% (correct: 25%), Undead→Shadow was 100% (correct: 0%), Undead→Poison was -50% (correct: 50%), Poison→Earth/Fire/Wind was 100% (correct: 125%), Poison→Holy was 125% (correct: 75%), Ghost→Undead was 50% (correct: 100%), Undead→Holy was 125% (correct: 100%)
>
> **Known deferred items** (not blocking, documented in memory):
> - Weapon element vs forced element distinction for physical skills (Bash should use weapon element)
> - Heal-damages-Undead mechanic
> - Monster class type (Boss/Normal) card category
> - Endow system (Phase 5 — Sage class)
> - Non-elemental vs Neutral distinction for monster auto-attacks

---

## Phase 4: Persistent Socket Connection (1-1.5 weeks) — COMPLETE ✓

> **Completed 2026-03-10.** All deliverables met. See `CLAUDE.md` "Persistent Socket Architecture (Phase 4)" section and `memory/persistent-socket.md` for implementation details.
>
> **What was built**:
> - `MMOGameInstance` owns `TSharedPtr<FSocketIONative> NativeSocket` — survives `OpenLevel()`, no disconnect on zone change
> - `USocketEventRouter` — multi-handler dispatch (multiple subsystems can register for the same event)
> - All 14+ C++ subsystems migrated from `FindSocketIOComponent()` actor iteration to `Router->RegisterHandler()` / `Router->UnregisterAllForOwner(this)` pattern
> - `MultiplayerEventSubsystem` — bridges 30+ inbound socket events to BP_SocketManager handler functions via `ProcessEvent`
> - `PositionBroadcastSubsystem` — 30Hz position broadcasting via persistent socket (replaces BP_SocketManager timer)
> - Server `zone:ready` event sends zone data without requiring `player:join` re-emit
> - `reconnectBuffCache` removed — buffs persist naturally in server memory across zone transitions
> - All subsystem widgets gated behind `GI->IsSocketConnected()` (only show in game levels, not login screen)
>
> **New files**: `SocketEventRouter.h/.cpp`, `MultiplayerEventSubsystem.h/.cpp`, `PositionBroadcastSubsystem.h/.cpp`

**Why now**: Every zone change currently causes a full socket disconnect → reconnect → `player:join` cycle. This is wrong architecturally — every real MMO (WoW, FFXIV, rAthena/Hercules) uses persistent connections. Doing this BEFORE Phase 5 means every new subsystem built for the 4 new classes (Archer, Acolyte, Thief, Merchant) uses the clean GameInstance socket pattern from day one, instead of building with the old actor-search pattern and then migrating.

### The Problem

Currently, every zone change causes:

1. Player enters warp portal → server sends `zone:change`
2. Client calls `UGameplayStatics::OpenLevel()` → destroys current world
3. `BP_SocketManager` (a per-level actor) is destroyed → socket disconnects
4. Server disconnect handler runs: saves to DB, removes from `connectedPlayers`, broadcasts `player:left`
5. New level loads → new `BP_SocketManager` creates a fresh socket connection
6. Client sends `player:join` → server creates brand new player object from DB
7. All in-memory state is lost: buffs, status effects, auto-attack state, active casts, enemy aggro

### Consequences

| Issue | Impact Now (4 zones) | Impact Later (20+ zones) |
|-------|---------------------|--------------------------|
| **Buffs/statuses lost** | Mitigated by `reconnectBuffCache` (band-aid) | Band-aid compounds — party state, guild state, quest state all need caches too |
| **`player:left` + `player:joined` spam** | Barely noticeable | Visible and annoying with 20+ players per zone |
| **DB writes on every zone change** | ~100ms, invisible | Adds up at scale with frequent zone transitions |
| **Re-authentication overhead** | ~200ms per transition | Same — unnecessary JWT re-validation, DB reload |
| **Race conditions** | Mitigated by `bIsZoneTransitioning` | Same |
| **Party state on zone change** | N/A (no party yet) | **BLOCKS Phase 6** — party HP bars, chat, EXP sharing must persist |

### Current Architecture

```
UE5 Level (L_Prontera)
  └── BP_SocketManager (Actor)           ← DESTROYED on OpenLevel
       └── USocketIOClientComponent      ← Socket connection dies here
            └── Connected to server

UMMOGameInstance                          ← SURVIVES OpenLevel
  └── Auth token, character data, zone state
```

All 15 C++ subsystems find the socket via `FindSocketIOComponent()` which iterates world actors looking for a `USocketIOClientComponent`. When the level changes, the actor is gone.

### Target Architecture

```
UMMOGameInstance                          ← SURVIVES OpenLevel
  └── USocketIOClientComponent (UPROPERTY) ← Connection persists here
  └── Auth token, character data, zone state

UE5 Level (any)
  └── Subsystems find socket via GameInstance, not actor search
```

### Phase 4a: Move SocketIO to GameInstance (1-2 days)

1. **Add `USocketIOClientComponent` to `UMMOGameInstance`**
   - Create as a `UPROPERTY()` default subobject in constructor
   - Connect on login success (after JWT obtained)
   - Disconnect on logout / return to login screen
   - Never disconnect on zone change

2. **Update `FindSocketIOComponent()` in all 15 subsystems**
   - Change from actor iteration to: `Cast<UMMOGameInstance>(GetGameInstance())->GetSocketIOComponent()`
   - Single function in a shared header or on GameInstance itself

3. **Remove `BP_SocketManager` actor from all levels**
   - Currently in Level Blueprints — remove the actor spawn
   - Move any Blueprint event bindings to GameInstance or a new C++ subsystem

### Phase 4b: Refactor Zone Change Flow (2-3 days)

**Current flow:**
```
Client: warp portal → emit zone:warp
Server: validate → emit zone:change { zone, level, x, y, z }
Client: OpenLevel → socket dies → reconnect → player:join
Server: full DB reload, new player object
```

**Target flow:**
```
Client: warp portal → emit zone:warp
Server: validate → move player to new zone bucket → emit zone:change { zone, level, x, y, z }
Client: OpenLevel → socket stays alive
Client: new level loads → emit zone:ready
Server: send zone enemies, other players, zone metadata (same as now, but no player:join needed)
```

4. **Server: Add `zone:transition` event** (replaces disconnect/reconnect)
   - Server moves player between zone rooms/buckets without disconnect
   - `broadcastToZone(oldZone, 'player:left', ...)` — notify old zone
   - Update `player.zone` in memory
   - `broadcastToZone(newZone, 'player:moved', ...)` — notify new zone
   - No `connectedPlayers.delete()`, no DB save, no re-auth

5. **Server: Remove re-init from zone change path**
   - No re-loading equipment/skills/inventory from DB (already in memory)
   - No re-sending `player:stats` (already sent)
   - Only send: zone enemies, other players, zone metadata, `buff:list`

6. **Client: `ZoneTransitionSubsystem` update**
   - On `zone:change`, call `OpenLevel` but do NOT expect socket to die
   - After new level loads, subsystems re-wrap events on the same (still-alive) component
   - Emit `zone:ready` as before

### Phase 4c: Blueprint Event Migration (2-3 days)

7. **Move BP_SocketManager event bindings to C++**
   - The Blueprint SocketManager currently binds ~20 socket events (enemy:spawn, player:moved, etc.)
   - These need to move to C++ subsystems or a new `USocketEventRouter` subsystem
   - This is the largest single task — requires auditing all Blueprint event bindings

### Phase 4d: Cleanup (1 day)

8. **Remove `reconnectBuffCache`** — no longer needed (buffs persist naturally)
9. **Remove `player:join` from zone change path** — only used on initial login
10. **Remove per-level `BP_SocketManager`** actor from all Level Blueprints
11. **Update all documentation** — CLAUDE.md, docsNew, memory files

### Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Blueprint event bindings break | HIGH | Audit all BP_SocketManager bindings before removing. Migrate one at a time. |
| Subsystems can't find socket during level transition | MEDIUM | GameInstance component is always available. Add null checks during transition. |
| Socket events arrive during level load (no world yet) | MEDIUM | Queue events in GameInstance, replay when new world's subsystems are ready |
| Multiple `OpenLevel` calls while socket is alive | LOW | Already handled by `bIsZoneTransitioning` flag |

### Internal Dependency Graph

```
Phase 4a (Move SocketIO to GameInstance)
  │
  ├── Phase 4b (Refactor zone change flow)
  │     └── Phase 4d (Cleanup)
  │
  └── Phase 4c (Blueprint event migration)
        └── Phase 4d (Cleanup)
```

Phase 4a must come first. Phases 4b and 4c can run in parallel. Phase 4d comes last.

### Files Affected

**Must Change:**

| File | Change |
|------|--------|
| `MMOGameInstance.h/.cpp` | Add `USocketIOClientComponent*` UPROPERTY, connect/disconnect methods |
| All 15 UI subsystems (`*Subsystem.cpp`) | Change `FindSocketIOComponent()` to use GameInstance |
| `SkillVFXSubsystem.cpp` | Same — uses FindSocketIOComponent |
| `SabriMMOCharacter.cpp` | Same — finds socket for position/combat events |
| `ZoneTransitionSubsystem.cpp` | Remove disconnect expectation from zone change flow |
| `server/src/index.js` | Add `zone:transition` event, remove reconnect from zone path |

**Must Remove:**

| File | Change |
|------|--------|
| Level Blueprints (all 4 levels) | Remove BP_SocketManager actor spawn |
| `BP_SocketManager` Blueprint | Delete or repurpose (event bindings move to C++) |

**Must Audit:**

| File | Reason |
|------|--------|
| `BP_SocketManager` Blueprint | All event bindings must be cataloged and migrated |
| `AC_HUDManager` Blueprint | May reference BP_SocketManager |
| All Level Blueprints | May reference BP_SocketManager for initial setup |

**Deliverable**: Socket connection survives zone changes. No disconnect/reconnect cycle. No `reconnectBuffCache`. No `player:left`/`player:joined` spam. All future subsystems (party, guild, quest) get persistence for free.

---

## Phase 5: Passive Skills & First Class Completion (2 weeks)

**Why now**: 7 passive skills do literally nothing (Sword Mastery, HP Recovery, Owl's Eye, etc.). Players can learn them, spend skill points, and get zero benefit. This is the most visible broken promise in the game. Additionally, completing all 6 first classes gives players real variety.

**Benefit of doing this AFTER Phase 4**: Every new subsystem built for these 4 classes uses the clean GameInstance socket pattern. No actor-search `FindSocketIOComponent()`, no stability delay timers for reconnect handling, no migration needed later.

**Server-side: Passive skill engine**:
- Passive skills modify derived stats on learn/level (recalculate on equip/level/skill change)
- Implementation: `applyPassiveSkills(player)` called during `calculateDerivedStats()`
- Swordsman passives: Sword Mastery (+ATK with swords), Two-Hand Mastery (+ATK with 2H swords), HP Recovery (+HP regen rate), Moving HP Recovery (regen while moving)
- Mage passives: Increase SP Recovery (+SP regen rate)
- Archer passives: Owl's Eye (+DEX), Vulture's Eye (+range), Improve Concentration (+AGI/DEX)
- Acolyte passives: Divine Protection (+DEF vs undead/demon), Demon Bane (+ATK vs undead/demon)
- Thief passives: Increase Dodge (+FLEE), Double Attack (auto-attack chance for extra hit)
- Merchant passives: Pushcart (enable cart), Loud Exclamation (+STR)

**Server-side: Combat formula prerequisites** (from Phase 3 audit findings):
- **Weapon element for physical skills**: Bash, Double Strafe, Arrow Shower, Mammonite, and other physical skills currently use forced `element: 'neutral'` in `ro_skill_data.js`. They should use weapon element (`element: 'weapon'`). Add `element: 'weapon'` support to `calculatePhysicalDamage` — if skill element is `'weapon'`, use `attacker.weaponElement` instead. Only skills with forced elements (bolts, Soul Strike, etc.) should override weapon element.
- **Heal-damages-Undead mechanic**: Heal (403) must deal Holy damage to Undead element targets. Formula: `HealDamage = floor(HealAmount / 2)`, then apply element modifier (Holy vs Undead = 150-200%). Required for Acolyte class to function correctly vs undead monsters.
- **Non-elemental vs Neutral distinction**: Monster auto-attacks are non-elemental in RO (bypass element table entirely, always 100%). Player neutral attacks ARE Neutral element and use the table. Currently both are treated as Neutral. Low priority — only matters for Ghost-armor players, which don't exist yet. Defer to Phase 12.

**Server-side: Complete all 6 first classes**:

| Class | Active Skills to Implement | Key Mechanics |
|-------|---------------------------|---------------|
| Swordsman | Fatal Blow (stun chance), Auto Berserk (low HP ATK boost) | Stun via status effect system |
| Mage | Energy Coat (SP shield) | SP-to-DEF conversion |
| **Archer** | Double Strafe (2-hit ranged), Arrow Shower (AoE ranged), Arrow Repel (knockback) | Ranged physical damage, arrow/weapon element |
| **Acolyte** | Heal, Blessing, Increase AGI, Decrease AGI, Angelus, Cure, Pneuma, Teleport, Warp Portal, Ruwach, Signum Crucis, Aqua Benedicta | Healing (Holy damage vs Undead), party buffs, status cleanse, utility |
| **Thief** | Steal, Hiding, Envenom (poison), Detoxify, Sand Attack (blind), Backslide, Throw Stone, Pick Stone | Stealth, poison via status system |
| **Merchant** | Mammonite (zeny-cost attack), Cart Revolution (AoE), Vending (placeholder — full vending in Phase 14) | Zeny consumption, weapon element, cart attacks |

**Client-side**:
- Heal VFX (green light burst on target)
- Buff VFX for Blessing, Increase AGI, Angelus (reuse SelfBuff pattern)
- Arrow/ranged projectile VFX
- Hiding visual (character becomes translucent)
- Update skill tree tabs for all 6 classes

**Deliverable**: All 6 first classes fully playable. ~40 new skills executable. Passive skills actually affect stats. Players can heal, buff, cure, teleport, steal, hide.

**Refactor justification**: HIGH ROI. This quadruples the playable content (2 classes -> 6 classes) and enables party play roles (tank, healer, DPS, support). The Acolyte class alone is worth the entire phase — every group needs a healer. The status effect system from Phase 2 makes Thief skills (poison, blind) trivial to implement.

---

## Phase 6: Party System (1-1.5 weeks)

**Why now**: With 6 playable classes including a healer, players need parties. Without parties, healing is useless (you can only heal yourself), tanking is pointless (no aggro management for a group), and the MMO is still a single-player game.

**Benefit of doing this AFTER Phase 4**: Party state (member list, HP bars, EXP share mode) persists naturally across zone changes via the persistent socket. No `reconnectPartyCache` band-aid needed.

**Server-side**:
- DB tables: `parties`, `party_members`
- In-memory party state: leader, members (max 12), EXP share mode (Even Share / Each Take)
- Socket events: `party:create`, `party:invite`, `party:join`, `party:leave`, `party:kick`, `party:disband`, `party:update`, `party:chat`
- EXP sharing: Even Share (split by level ratio within 15-level range), Each Take (kill credit)
- Party member HP/SP broadcasting (party members see each other's HP)
- Death/respawn notifications to party

**Client-side**:
- `PartySubsystem` (UWorldSubsystem) + `SPartyWidget` (Slate)
- Party member list with HP bars (small RO-style vertical bars)
- Party chat tab in chat window
- Invite/kick/leave/disband buttons

**Deliverable**: Full party system. Groups of up to 12 can adventure together with shared EXP, visible HP bars, and party chat.

**Refactor justification**: HIGH ROI. This is the #1 feature that converts a tech demo into an MMO. Party play is the core loop of Ragnarok Online. Without it, there's no reason for 6 different classes to exist.

---

## Phase 7: Chat System Expansion (3-5 days)

**Why now**: Currently only zone-wide chat exists. With parties, you need party chat. Whisper is essential for social interaction. This is a relatively small effort that dramatically improves the social experience.

**Server-side**:
- Whisper: `chat:whisper` event (recipient name lookup, offline check)
- Party chat: route through party member list
- Chat message types: public, whisper, party, guild (guild placeholder)
- Message length limit (prevent spam)
- Basic profanity filter (optional, configurable)
- `/commands`: `/w <name> <msg>` (whisper), `/p <msg>` (party)

**Client-side**:
- Expand existing chat handling with message type routing
- Chat tabs or color coding by channel (RO Classic: white=public, yellow=whisper, green=party, blue=guild)
- Input prefix parsing for `/w`, `/p` commands

**Deliverable**: Whisper and party chat. Players can have private conversations and coordinate in parties.

---

## Phase 8: Priority Second Class Skills (3-4 weeks)

**Why now**: First-class characters hit a progression wall at job level 50 (job change to 2nd class). Without second-class skills, there's nothing new after job change. The 5 most impactful second classes cover all core MMO roles.

**Priority order** (by player impact and party role diversity):

### 8a. Knight (1 week)
- Tank role. Most popular melee class.
- Key skills: Bowling Bash (AoE knockback), Pierce (size bonus), Brandish Spear (AoE cone), Spear Boomerang (ranged), Counter Attack, Auto Counter, Two-Hand Quicken (ASPD buff), Peco Peco Ride (mount)
- Enables: True tank gameplay with AoE aggro and high survivability

### 8b. Priest (1 week)
- Healer/support role. Required for all group content.
- Key skills: Heal (upgrade), Sanctuary (AoE ground heal), Resurrection, Magnus Exorcismus (AoE holy), Turn Undead (instant kill undead), Kyrie Eleison (damage barrier), Magnificat (party SP regen), Gloria (party LUK boost), Lex Aeterna (double damage debuff), Aspersio (holy weapon buff)
- Enables: Full party healing, resurrection, endgame undead dungeon viability

### 8c. Wizard (1 week)
- Primary AoE damage. Most efficient mob clearing.
- Key skills: Jupitel Thunder (knockback wind), Lord of Vermillion (AoE wind), Meteor Storm (AoE fire), Storm Gust (AoE freeze), Ice Wall (ground obstacle), Water Ball, Earth Spike, Heaven's Drive, Quagmire (AoE slow debuff), Sight Rasher
- Enables: AoE farming, high-level magic damage, crowd control

### 8d. Assassin (3-5 days)
- Melee DPS role. High single-target damage.
- Key skills: Sonic Blow (single target burst), Grimtooth (hidden AoE), Cloaking (improved hiding), Enchant Poison (weapon poison buff), Venom Dust (poison ground), Katar Mastery, Sonic Acceleration
- Enables: Stealth gameplay, high burst damage, poison builds

### 8e. Hunter (3-5 days)
- Ranged DPS role. Trap specialist.
- Key skills: Blitz Beat (falcon attack), Steel Crow (falcon ATK up), Beast Bane (bonus vs brute/insect), Flasher/Freezing/Sandman/Ankle Snare (traps), Detect (reveal hidden), Spring Trap, Shockwave Trap
- Enables: Ranged DPS, trap-based area control, falcon auto-blitz

**Deliverable**: 5 playable second classes covering Tank/Healer/AoE DPS/Melee DPS/Ranged DPS. ~50 new skills. True party composition and role diversity.

---

## Phase 9: Monster Skills & MVP System (1.5-2 weeks)

**Why now**: With strong player classes, combat becomes too easy. Monsters only auto-attack, making even "dangerous" enemies trivial. Monster skills and MVP bosses create endgame PvE content that tests party coordination.

**Server-side**:
- Monster skill AI: skill selection based on HP threshold, range, and cooldowns
- Magic-using monsters: cast time + damage spells (Osiris casts Dark Soul Strike, etc.)
- Ranged attackers: Archer-type monsters with ranged physical attacks
- Healer monsters: monsters that heal allies
- MVP bosses: announcement on spawn, special skill rotations, MVP drop table, tombstone on death
- MVP timer: track last killer, MVP exp reward

**Client-side**:
- Monster cast bars (reuse CastBarSubsystem — already tracks all casters)
- MVP spawn announcement (zone-wide notification)
- MVP tombstone visual

**Deliverable**: Monsters use skills, making combat tactically interesting. MVPs provide endgame boss encounters for parties.

---

## Phase 10: World Expansion (2-3 weeks)

**Why now**: With 5+ viable classes, parties, and challenging monsters, players need places to go. 4 zones is a tutorial, not a world. This phase is mostly content creation (level design, spawn placement) using the existing zone system.

**Zones to add** (priority order):
1. **Prontera Indoor** — Tool/weapon/armor shop interiors, church (job change NPC), inn
2. **Geffen** — Mage guild town. Geffen Tower dungeon (3 floors)
3. **Payon** — Archer guild town. Payon Cave dungeon (3 floors)
4. **Morroc** — Thief guild town. Pyramid dungeon (3 floors)
5. **Alberta** — Merchant guild town. Port city
6. **2-3 field maps** per town connecting area (level-appropriate monster spawns)
7. **Byalan Dungeon** (underwater, 3 floors) — water element monsters
8. **Ant Hell** (2 floors) — insect monsters

Target: **15-20 zones** (from current 4). This creates a meaningful world with distinct towns, themed dungeons, and level progression paths from 1-99.

**Server-side**: Add zone entries to `ro_zone_data.js` with warps, spawns, Kafra NPCs, flags.
**Client-side**: Create UE5 levels, place actors, NavMesh, lighting.

**Deliverable**: A world worth exploring. Multiple towns with guild NPCs, 3-4 dungeon series, clear progression path.

---

## Phase 11: Job Change & Quest System (1.5-2 weeks)

**Why now**: Players can now reach job level 10/40 but need actual job change quests, not just a socket event. This phase adds the quest infrastructure that all future content (Platinum skill quests, storyline quests) will use.

**Server-side**:
- `AMMONPC` generic NPC actor class (replace ad-hoc ShopNPC/KafraNPC pattern)
- `NPC_REGISTRY` — NPC definitions per zone
- Dialogue system: JSON dialogue trees with condition checks and actions
- Quest state machine: `quest_progress` DB table, accept/progress/complete/fail states
- Job change quests: 1 quest per first-class change (at job level 10), 1 per second-class (at job level 40)
- Quest rewards: items, EXP, zeny, skill unlocks

**Client-side**:
- `DialogueSubsystem` + `SDialogueWidget` — NPC conversation UI with choices
- Quest log UI (optional — can defer to later if scope is tight)

**Deliverable**: Proper job change quests. Generic NPC/dialogue system reusable for all future quest content.

---

## Phase 12: Item & Equipment Deep Dive (2 weeks)

**Why now**: With 5+ second classes, players need class-appropriate equipment. The current 148 items don't cover weapon types for all classes. Refining adds equipment progression. Cards add build diversity.

**Server-side**:
- Expand item DB to 500+ items (weapons for all weapon types, armor sets per level tier)
- Refining system: +0 to +10, success rates per weapon level, Refine NPC, Elunium/Oridecon consumables
- Card system: compound cards into slotted equipment, card effects applied to stats
- **Boss/Normal monster class card category**: Add `class_boss` and `class_normal` card modifier types to `calculatePhysicalDamage`. Abysmal Knight Card = +25% vs Boss class. Currently only race/element/size card categories exist (Phase 3 audit finding).
- **Non-elemental vs Neutral distinction**: Monster auto-attacks should be non-elemental (bypass element table, always 100% damage to all armor elements). Player neutral attacks use Neutral element and ARE affected by the table. Ghost-armor players would be immune to player neutral attacks but NOT to monster auto-attacks. Implement when armor element cards (Ghostring, etc.) are added.
- Weight limit enforcement: 50% (natural regen stops), 70% (skills blocked), 90% (movement blocked)
- Class-specific equipment restrictions (Knight can't use bows, Mage can't use swords, etc.)
- Death penalty: 1% base EXP loss on death (RO Classic)

**Client-side**:
- Refine NPC interaction UI
- Card compound UI
- Weight indicator on BasicInfoWidget (color changes at thresholds)
- Equipment tooltip showing card slots, refine level

**Deliverable**: Meaningful equipment progression. Players can refine weapons, socket cards, and build unique gear combinations.

---

## Phases 13-17: Future Scope (Not Detailed Yet)

These phases are deferred because they don't block core gameplay. Plan them when Phases 3-12 are complete.

| Phase | System | Reason to Defer |
|-------|--------|-----------------|
| 13 | Guild System | Requires party system first. WoE requires guilds + PvP + castles. |
| 14 | Economy & Trading | Vending/buying stores need more items and players to be meaningful. |
| 15 | PvP & War of Emperium | Requires balanced classes, guilds, and dedicated PvP zones. |
| 16 | Companions (Pets, Homunculus) | Nice-to-have. Doesn't block core progression. |
| 17 | Art, Audio, Polish | Asset work can happen anytime. Placeholder meshes are fine during development. |

---

## Time Estimates & Dependency Graph

```
Phase 0: Critical Fixes .................. 1-2 days      ✓ DONE
  |
Phase 2: Status Effects + Buff System .... 1.5-2 weeks   ✓ DONE
  |
Phase 3: Element Table Audit ............. 0.5 days      ✓ DONE
  |
Phase 4: Persistent Socket Connection .... 1-1.5 weeks   ✓ DONE
  |
Phase 5: Passive Skills + 6 First Classes  2 weeks       ← NEXT
  |
  +---> Phase 6: Party System ............ 1-1.5 weeks
  |       |
  |       +---> Phase 7: Chat Expansion .. 3-5 days
  |
  +---> Phase 8: 5 Second Classes ........ 3-4 weeks
          |
          +---> Phase 9: Monster Skills .. 1.5-2 weeks
          |
          +---> Phase 10: World Expansion  2-3 weeks
                  |
                  +---> Phase 11: Quests . 1.5-2 weeks
                  |
                  +---> Phase 12: Items .. 2 weeks
```

**Critical path**: 3 -> 4 -> 5 -> 8 -> 9 (combat depth)
**Social path**: 3 -> 4 -> 5 -> 6 -> 7 (party play)
**Content path**: 3 -> 4 -> 5 -> 10 -> 11 -> 12 (world + items)

**Phase 1 (Server Extraction) is DEFERRED** — revisit if index.js growth becomes painful during Phase 8.
**Phases 6-7 can run in parallel with Phase 8.** They share no code dependencies.
**Phase 10 can start as soon as Phase 5 is done** (zone creation doesn't need second-class skills).

**Total estimated time for Phases 3-12**: ~17-24 weeks (4-6 months)

---

## Cost vs Benefit Analysis

### Highest ROI (Do These First)

| Phase | Cost | Benefit | ROI |
|-------|------|---------|-----|
| 3: Element Table Audit | 0.5 days | ✓ DONE — Fixed 100+ wrong element values, verified size penalties, fixed card stacking, 537 tests | **Extreme** — every hit in the game is affected |
| 4: Persistent Socket | 1-1.5 weeks | Eliminates disconnect/reconnect cycle, enables party/guild state persistence, removes band-aid caches | **Very High** — foundational infrastructure for all social features |
| 5: First Classes | 2 weeks | 6 playable classes (from 2), 40+ new skills, healing exists | **Very High** — 3x content, enables party roles |
| 6: Party System | 1-1.5 weeks | Group play, EXP sharing, party HP bars | **Very High** — converts tech demo to MMO |

### Good ROI (Do After Core)

| Phase | Cost | Benefit | ROI |
|-------|------|---------|-----|
| 7: Chat Expansion | 3-5 days | Whisper + party chat, social interaction | **High** — small cost, big social impact |
| 8: Second Classes | 3-4 weeks | 50+ new skills, endgame class diversity | **High** — progression depth |

### Moderate ROI (Important but Can Wait)

| Phase | Cost | Benefit | ROI |
|-------|------|---------|-----|
| 9: Monster Skills | 1.5-2 weeks | PvE challenge, MVP bosses | **Moderate** — combat depth |
| 10: World Expansion | 2-3 weeks | 15-20 zones, exploration content | **Moderate** — mostly level design labor |
| 11: Quest System | 1.5-2 weeks | Job change quests, NPC dialogue | **Moderate** — progression structure |
| 12: Item Deep Dive | 2 weeks | Refining, cards, 500+ items | **Moderate** — equipment depth |

### Low ROI Right Now (Defer)

| System | Cost | Why Defer |
|--------|------|-----------|
| Server Extraction | 3-5 days | AI navigates monolith fine. Extract when growth hurts. |
| Guild System | 2+ weeks | No parties yet, no WoE content to use guilds for |
| PvP/WoE | 3+ weeks | Requires balanced classes, guilds, dedicated zones |
| Companions | 2+ weeks | Nice-to-have, doesn't affect core progression |
| Vending/Trading | 1-2 weeks | Need more items and players first |
| Art/Audio | Ongoing | Placeholder meshes work fine during development |
| Full test suite | 3+ weeks | 129 unimplemented tests. Write tests alongside features instead |

---

## What MUST Be Refactored Now vs Later

### Already Refactored (Phases 0, 2, 3, 4):

1. **Element table correction** ✓ — Replaced entire 400-value table with rAthena canonical, fixed card modifier stacking, 537 tests passing.

3. **Status effect architecture** ✓ — Generic system built. Any future skill can apply/remove effects with 2-3 lines.

4. **Buff system architecture** ✓ — Generic system built. Blessing, Increase AGI, Angelus, Kyrie Eleison all use it.

5. **SP deduction timing** ✓ — Fixed while only 17 skills exist.

6. **Security fixes** ✓ — JWT auth, DB transactions, rate limiting all fixed.

7. **Persistent socket connection** ✓ — Socket on GameInstance survives zone changes. SocketEventRouter for multi-handler dispatch. All 14+ subsystems migrated. reconnectBuffCache removed. BP event bridge via MultiplayerEventSubsystem.

### Refactor LATER (Fine to defer):

1. **Full server modularization** — The REST API, inventory handlers, enemy AI, zone management work fine in index.js. Extract them when they need changes, not proactively. AI navigates the monolith without issue.

2. **AC_HUDManager Blueprint cleanup** — The old Blueprint HUD manager is legacy code alongside the new C++ subsystems. It works. Clean it up when you need to modify those Blueprints.

3. **Test infrastructure** — The 196 test function stubs are aspirational. Don't try to fill them all in. Write tests alongside the features they test.

4. **Dead code removal** — `ASabriMMOPlayerController`, `BP_GameFlow`, Blueprint damage numbers, variant code. These cause no harm and removing them risks breakage. Defer.

5. **Data-driven skill engine** — External review suggested replacing per-skill if/else handlers with a generic execution engine. With AI-assisted development, each handler takes 2-5 minutes to write vs 30-60 minutes for a human. The engine would save ~1-2 days total over 450 skills — not worth the 5-7 day investment to build. Add skills incrementally.

---

## Success Criteria

After completing Phases 3-8, the game should have:
- ✓ Correct element table matching rAthena pre-renewal (DONE — Phase 3)
- ✓ Persistent socket connection, no disconnect on zone change (DONE — Phase 4)
- 6 fully playable first classes with all skills working
- 5 playable second classes (Knight, Priest, Wizard, Assassin, Hunter)
- ~90+ executable skills (from current 17)
- Generic status effect system with 10 core effects
- Generic buff system with buff bar UI
- Party system with EXP sharing
- Whisper and party chat
- Still 4 zones (world expansion can start anytime)

This represents a **true MMO prototype** where players can:
- Choose from 11 classes (6 first + 5 second)
- Form parties with diverse roles (tank/healer/DPS)
- Experience meaningful combat with buffs, debuffs, and status effects
- Communicate via public/whisper/party chat
- Progress from Novice to second class
- Change zones without losing connection or state

**Estimated time to reach this milestone: 11-15 weeks.**

---

## External Review Assessment (2026-03-10)

An external architecture review identified 5 problems. Here's how they map to this plan:

| Problem | Verdict | Action Taken |
|---------|---------|-------------|
| 8,800-line monolith server | **Overstated** — AI navigates it fine, solo dev has no merge conflicts | Phase 1 DEFERRED. No change. |
| Hardcoded skill handlers | **Overstated** — AI writes each handler in minutes; generic engine saves ~1-2 days total over 450 skills | No data-driven engine. Add skills incrementally. |
| Element table WRONG | **CONFIRMED** — turned out to be 100+ wrong values, not just 8 | **Phase 3 COMPLETE.** Replaced entire table with rAthena canonical, fixed card stacking, 537 tests. Also identified 4 supplemental items for Phases 5 and 12. |
| Client socket polling "time bomb" | **WRONG** — pattern is well-engineered with guards, health checks, stability delays | No change. Persistent socket (Phase 4) is the right evolution. |
| Monsters can't cast skills | **Correct, already planned** | Phase 9. Timing is right — after player classes. |

---

## Appendix: Skills Implementation Priority Queue

For reference during Phases 5 and 8, here are the skills in priority order:

### Phase 5 — First Class Skills (~40 skills)
```
PREREQUISITE — WEAPON ELEMENT SUPPORT (Phase 3 audit finding):
  Fix ro_skill_data.js: Bash (103), Double Strafe (304), Arrow Shower (305),
  Mammonite (603), and other physical skills → element: 'weapon' (not 'neutral')
  Add 'weapon' element support to calculatePhysicalDamage

PREREQUISITE — HEAL-DAMAGES-UNDEAD (Phase 3 audit finding):
  Heal (403) must deal Holy damage to Undead element targets
  Formula: floor(HealAmount / 2), then apply element modifier (Holy vs Undead)

PASSIVE ENGINE (applies to all classes):
  Sword Mastery (101), Two-Hand Mastery (102), HP Recovery (107)
  Owl's Eye (301), Vulture's Eye (302), Improve Concentration (303)
  Divine Protection (401), Demon Bane (402)
  Increase Dodge (501), Double Attack (502)
  Pushcart (601), Loud Exclamation (602)
  Increase SP Recovery (204)

SWORDSMAN remaining:
  Fatal Blow (110) — stun chance
  Auto Berserk (109) — low HP ATK boost
  Moving HP Recovery (108) — regen while moving

ARCHER (new class):
  Double Strafe (304) — 2-hit ranged, uses WEAPON element
  Arrow Shower (305) — AoE ranged, uses WEAPON element
  Arrow Repel (306) — knockback ranged, uses WEAPON element

ACOLYTE (new class — HIGHEST PRIORITY):
  Heal (403) — Holy damage vs Undead, Blessing (404), Increase AGI (405)
  Decrease AGI (406), Angelus (407), Cure (408)
  Pneuma (409), Teleport (410), Warp Portal (411)
  Ruwach (412), Signum Crucis (413), Aqua Benedicta (414)

THIEF (new class):
  Steal (503), Hiding (504), Envenom (505)
  Detoxify (506), Sand Attack (507), Backslide (508)
  Throw Stone (509), Pick Stone (510)

MERCHANT (new class):
  Mammonite (603) — zeny attack, uses WEAPON element
  Cart Revolution (604) — cart AoE
```

### Phase 8 — Second Class Skills (~50 skills)
```
KNIGHT:
  Bowling Bash, Pierce, Brandish Spear, Spear Boomerang
  Counter Attack, Auto Counter, Two-Hand Quicken, Peco Ride
  Cavalier Mastery, Spear Mastery

PRIEST:
  Sanctuary, Resurrection, Magnus Exorcismus, Turn Undead
  Kyrie Eleison, Magnificat, Gloria, Lex Aeterna
  Aspersio, B.S. Sacramenti, Impositio Manus, Suffragium
  Safety Wall (upgrade), Status Recovery

WIZARD:
  Jupitel Thunder, Lord of Vermillion, Meteor Storm
  Storm Gust, Ice Wall, Water Ball, Earth Spike
  Heaven's Drive, Quagmire, Sight Rasher
  Sense, Monster Property

ASSASSIN:
  Sonic Blow, Grimtooth, Cloaking, Enchant Poison
  Venom Dust, Katar Mastery, Sonic Acceleration
  Advanced Katar Mastery, Venom Splasher

HUNTER:
  Blitz Beat, Steel Crow, Beast Bane, Detect
  Flasher, Freezing Trap, Sandman, Ankle Snare
  Shockwave Trap, Spring Trap, Land Mine
```
