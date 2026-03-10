# Sabri_MMO — Strategic Implementation Plan v2

**Status**: SUPERSEDED by `Strategic_Implementation_Plan_v3.md`
**Created**: 2026-03-09
**Last Updated**: 2026-03-10
**Based on**: Full codebase audit, skills/VFX audit, all RagnaCloneDocs, all docsNew/, complete server + client C++ analysis

## Progress Tracker

| Phase | Status | Completed | Notes |
|-------|--------|-----------|-------|
| Phase 0: Critical Fixes | **COMPLETE** | 2026-03-09 | All 6 fixes verified: JWT auth, Butterfly Wing, shop transactions, SP timing, hotbar idempotent, socket rate limiting |
| Phase 0.5: Element Table + Formula Audit | NOT STARTED | — | Fix confirmed-wrong ELEMENT_TABLE values in ro_damage_formulas.js (8+ errors vs rAthena canonical), verify Soft DEF formula |
| Phase 1: Server Extraction | DEFERRED | — | New modules created instead (ro_status_effects.js, ro_buff_system.js); full extraction deferred |
| Phase 2: Status Effects + Buffs | **COMPLETE** | 2026-03-09 | 10 status effects, buff system, BuffBarWidget, AI CC lock, reconnect cache |
| Phase 3: Passive Skills + Classes | NOT STARTED | — | |
| Phase 4: Party System | NOT STARTED | — | |
| Phase 5: Chat Expansion | NOT STARTED | — | |
| Phase 6: Second Classes | NOT STARTED | — | |
| Phase 7: Monster Skills | NOT STARTED | — | |
| Phase 8: World Expansion | NOT STARTED | — | |
| Phase 9: Quests | NOT STARTED | — | |
| Phase 10: Items Deep Dive | NOT STARTED | — | |

---

## Executive Summary

The project has a **solid foundation**: auth, 14 C++ UI subsystems, 4 working zones, auto-attack + 17 skills with VFX, RO damage formulas, 509 monster templates, inventory/equipment/hotbar, and NPC shops. The architecture (server-authoritative, UWorldSubsystem pattern, event wrapping) is sound.

**The core problem**: Only **10% of skills work**, only **2 of 13 classes are playable** (Swordsman, Mage), and **zero social systems exist** (no party, guild, chat channels, trading). The game is a tech demo, not an MMO.

**The strategy**: Fix critical bugs first, then build the **generic systems** (status effects, buffs, passive skills) that unblock ALL future class/skill work, then expand classes, social features, and content in order of player impact.

---

## Guiding Principles

1. **Build generic systems before specific content** — A proper status effect engine takes 1 week but unblocks 100+ skills. Hardcoding each status effect individually would cost 10x more time and require refactoring.

2. **Prioritize what enables group play** — An MMO without parties, healers, and chat is a single-player game with spectators. Acolyte/Priest + Party system are the highest-impact features.

3. **Fix security holes before adding features** — An auth bypass means any socket client can impersonate any character. This must be fixed before anything else.

4. **Minimal server modularization, not full rewrite** — Extract skill handlers (where 80% of new code will go) into their own file. Don't reorganize everything. The monolith works; just make the growth path sustainable.

5. **Defer what doesn't block gameplay** — Art, audio, PvP/WoE, companions, and vending can wait. They don't block core progression or party play.

---

## Phase 0: Critical Fixes (1-2 days)

**Why now**: These are security holes and crash bugs. Shipping new features on a broken foundation is waste.

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

**Refactor justification**: These are pure bug fixes with immediate safety benefit. Zero risk of rework. Not fixing them risks data corruption and exploitation.

---

## Phase 0.5: Element Table & Formula Audit (half day)

**Why now**: The `ELEMENT_TABLE` in `ro_damage_formulas.js` has **confirmed errors** against the canonical rAthena pre-renewal `attr_fix.txt`. At least 8 values are wrong at level 1 alone — the full 400-value table likely has more. Every damage calculation in the game uses this table, so every hit is potentially wrong.

**Confirmed discrepancies (level 1 only)**:

| Attack → Defend | Current | rAthena Canonical | Impact |
|-----------------|---------|-------------------|--------|
| Neutral → Ghost | 25% | **70%** | Ghost monsters take too little neutral damage |
| Ghost → Neutral | 100% | **10%** | Ghost attacks deal 10x too much to neutral targets |
| Undead → Shadow | 100% | **0%** (immune) | Shadow targets should be immune to undead attacks |
| Undead → Poison | -50% (heals) | **50%** (damages) | Completely inverted — poison targets get healed instead of damaged |
| Undead → Ghost | 50% | **100%** | Ghost targets take half damage instead of full |
| Poison → Earth | 100% | **125%** | Poison attacks underperform vs earth |
| Poison → Fire | 100% | **125%** | Poison attacks underperform vs fire |
| Poison → Wind | 100% | **125%** | Poison attacks underperform vs wind |

**Tasks**:
1. Download rAthena canonical `attr_fix.txt` (all 4 levels × 10×10 = 400 values)
2. Replace entire `ELEMENT_TABLE` in `ro_damage_formulas.js` with verified values
3. Spot-check Soft DEF formula against rAthena `status.cpp` — current formula `floor(vit/2) + max(1, floor((vit*2-1)/3))` may differ from canonical
4. Quick smoke test: verify a few damage calculations with known RO values

**Deliverable**: All element interactions match rAthena pre-renewal. Combat balance is correct.

**Effort**: Half day. Source file is `server/src/ro_damage_formulas.js` lines 16-137.

---

## Phase 1: Targeted Server Extraction (3-5 days)

**Why now**: The server is 8,500 lines in one file. Every skill we add (80+ remaining) goes into this file. Extracting the skill handler section now means every subsequent phase is faster to develop, debug, and review.

**What to extract (minimal, high-ROI)**:
1. `skill_handlers.js` — All `skill:use` case handlers (~800 lines). This is where 80% of new code goes.
2. `combat_handlers.js` — Auto-attack processing, damage application, death handling (~400 lines)
3. `status_effects.js` — New file for Phase 2 (created empty now with the interface)
4. `buff_system.js` — New file for Phase 2

**What NOT to touch**: REST API routes, socket event binding, enemy AI tick, zone management, inventory handlers. These work fine and modularizing them adds no value yet.

**Deliverable**: `index.js` drops from ~8,500 to ~6,500 lines. New skill code goes into clean, focused modules. Existing functionality unchanged.

**Refactor justification**: HIGH ROI. We're about to add 100+ skill handlers in Phases 3-6. Every one of those will be written, debugged, and reviewed in the extracted file. The cost of extraction (3-5 days) is repaid within Phase 3 alone.

---

## Phase 2: Generic Status Effect & Buff System (1.5-2 weeks) — COMPLETE ✓

> **Completed 2026-03-09.** All deliverables met. See `docsNew/03_Server_Side/Status_Effect_Buff_System.md` and `docsNew/02_Client_Side/C++_Code/14_BuffBarSubsystem.md` for implementation details.
>
> **What was built**: `ro_status_effects.js` (10 effects with resistance formulas, periodic drains), `ro_buff_system.js` (generic buff system with stacking rules), `getCombinedModifiers()` in index.js, `BuffBarSubsystem` + `SBuffBarWidget` (client UI), AI CC lock, movement lock, regen blocking, damage-break-all-statuses, debug test commands, `reconnectBuffCache` for zone change persistence, `buff:list`/`buff:request` socket events.
>
> **Skills created**: `/sabrimmo-buff`, `/sabrimmo-debuff`

**Why now**: This is the single biggest blocker in the entire project. Without a generic status effect system:
- You can't implement Stun, Poison, Silence, Blind, Sleep, Curse, Bleeding, Coma properly
- You can't implement Acolyte skills (Cure needs to cleanse status effects)
- You can't implement 60+ skills that apply or interact with status effects
- You can't implement status effect resistance (STR/VIT/INT-based)
- Every skill that touches status effects will need refactoring later

**Server-side scope**:
- Generic `StatusEffectEngine` — apply, tick, resist, cleanse, expire, stack rules
- 10 core status effects: Stun, Freeze, Petrify, Poison, Silence, Blind, Sleep, Curse, Bleeding, Coma
- Resistance formulas: STR-based Stun resist, VIT-based Poison resist, INT-based Silence resist, etc.
- Buff system: apply, duration, stack rules (overwrite vs stack vs refresh), stat modifications, cleanse
- Integration: refactor existing hardcoded effects (Frost Diver freeze, Stone Curse petrify, Provoke buff) into the generic system
- Socket events: `status:applied`, `status:removed`, `status:tick`, `buff:applied`, `buff:removed`, `buff:list`
- Player movement locked during Stun/Freeze/Petrify/Sleep
- Flee penalty for multiple attackers (RO Classic: -10% per additional attacker after first)

**Client-side scope**:
- **Buff Bar UI** — New `BuffBarSubsystem` (UWorldSubsystem + Slate widget). Shows active buffs/debuffs with icons, duration timers, and tooltip on hover. RO Classic style: row of small icons below the basic info panel.
- Status effect visual indicators (frozen character tint, stun stars, poison bubbles — can be simple color overlays initially)
- Socket event wrapping for `status:*` and `buff:*` events

**Deliverable**: Any future skill can apply/remove status effects and buffs with 2-3 lines of code instead of building custom per-skill logic. Buff bar shows all active effects.

**Refactor justification**: CRITICAL. This is the architectural cornerstone that the next 100+ skills depend on. Building it now costs 1.5 weeks. Building it later costs the same 1.5 weeks PLUS refactoring every skill that was built without it. The RagnaCloneDocs explicitly call out status effects as a Phase 1 requirement.

---

## Phase 3: Passive Skills & First Class Completion (2 weeks)

**Why now**: 7 passive skills do literally nothing (Sword Mastery, HP Recovery, Owl's Eye, etc.). Players can learn them, spend skill points, and get zero benefit. This is the most visible broken promise in the game. Additionally, completing all 6 first classes gives players real variety.

**Server-side: Passive skill engine**:
- Passive skills modify derived stats on learn/level (recalculate on equip/level/skill change)
- Implementation: `applyPassiveSkills(player)` called during `calculateDerivedStats()`
- Swordsman passives: Sword Mastery (+ATK with swords), Two-Hand Mastery (+ATK with 2H swords), HP Recovery (+HP regen rate), Moving HP Recovery (regen while moving)
- Mage passives: Increase SP Recovery (+SP regen rate)
- Archer passives: Owl's Eye (+DEX), Vulture's Eye (+range), Improve Concentration (+AGI/DEX)
- Acolyte passives: Divine Protection (+DEF vs undead/demon), Demon Bane (+ATK vs undead/demon)
- Thief passives: Increase Dodge (+FLEE), Double Attack (auto-attack chance for extra hit)
- Merchant passives: Pushcart (enable cart), Loud Exclamation (+STR)

**Server-side: Complete all 6 first classes**:

| Class | Active Skills to Implement | Key Mechanics |
|-------|---------------------------|---------------|
| Swordsman | Fatal Blow (stun chance), Auto Berserk (low HP ATK boost) | Stun via status effect system |
| Mage | Energy Coat (SP shield) | SP-to-DEF conversion |
| **Archer** | Double Strafe (2-hit ranged), Arrow Shower (AoE ranged), Arrow Repel (knockback) | Ranged physical damage, arrow element |
| **Acolyte** | Heal, Blessing, Increase AGI, Decrease AGI, Angelus, Cure, Pneuma, Teleport, Warp Portal, Ruwach, Signum Crucis, Aqua Benedicta | Healing, party buffs, status cleanse, utility |
| **Thief** | Steal, Hiding, Envenom (poison), Detoxify, Sand Attack (blind), Backslide, Throw Stone, Pick Stone | Stealth, poison via status system |
| **Merchant** | Mammonite (zeny-cost attack), Cart Revolution (AoE), Vending (placeholder — full vending in Phase 12) | Zeny consumption, cart attacks |

**Client-side**:
- Heal VFX (green light burst on target)
- Buff VFX for Blessing, Increase AGI, Angelus (reuse SelfBuff pattern)
- Arrow/ranged projectile VFX
- Hiding visual (character becomes translucent)
- Update skill tree tabs for all 6 classes

**Deliverable**: All 6 first classes fully playable. ~40 new skills executable. Passive skills actually affect stats. Players can heal, buff, cure, teleport, steal, hide.

**Refactor justification**: HIGH ROI. This quadruples the playable content (2 classes -> 6 classes) and enables party play roles (tank, healer, DPS, support). The Acolyte class alone is worth the entire phase — every group needs a healer. The status effect system from Phase 2 makes Thief skills (poison, blind) trivial to implement.

---

## Phase 4: Party System (1-1.5 weeks)

**Why now**: With 6 playable classes including a healer, players need parties. Without parties, healing is useless (you can only heal yourself), tanking is pointless (no aggro management for a group), and the MMO is still a single-player game.

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

## Phase 5: Chat System Expansion (3-5 days)

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

## Phase 6: Priority Second Class Skills (3-4 weeks)

**Why now**: First-class characters hit a progression wall at job level 50 (job change to 2nd class). Without second-class skills, there's nothing new after job change. The 5 most impactful second classes cover all core MMO roles.

**Priority order** (by player impact and party role diversity):

### 6a. Knight (1 week)
- Tank role. Most popular melee class.
- Key skills: Bowling Bash (AoE knockback), Pierce (size bonus), Brandish Spear (AoE cone), Spear Boomerang (ranged), Counter Attack, Auto Counter, Two-Hand Quicken (ASPD buff), Peco Peco Ride (mount)
- Enables: True tank gameplay with AoE aggro and high survivability

### 6b. Priest (1 week)
- Healer/support role. Required for all group content.
- Key skills: Heal (upgrade), Sanctuary (AoE ground heal), Resurrection, Magnus Exorcismus (AoE holy), Turn Undead (instant kill undead), Kyrie Eleison (damage barrier), Magnificat (party SP regen), Gloria (party LUK boost), Lex Aeterna (double damage debuff), Aspersio (holy weapon buff)
- Enables: Full party healing, resurrection, endgame undead dungeon viability

### 6c. Wizard (1 week)
- Primary AoE damage. Most efficient mob clearing.
- Key skills: Jupitel Thunder (knockback wind), Lord of Vermillion (AoE wind), Meteor Storm (AoE fire), Storm Gust (AoE freeze), Ice Wall (ground obstacle), Water Ball, Earth Spike, Heaven's Drive, Quagmire (AoE slow debuff), Sight Rasher
- Enables: AoE farming, high-level magic damage, crowd control

### 6d. Assassin (3-5 days)
- Melee DPS role. High single-target damage.
- Key skills: Sonic Blow (single target burst), Grimtooth (hidden AoE), Cloaking (improved hiding), Enchant Poison (weapon poison buff), Venom Dust (poison ground), Katar Mastery, Sonic Acceleration
- Enables: Stealth gameplay, high burst damage, poison builds

### 6e. Hunter (3-5 days)
- Ranged DPS role. Trap specialist.
- Key skills: Blitz Beat (falcon attack), Steel Crow (falcon ATK up), Beast Bane (bonus vs brute/insect), Flasher/Freezing/Sandman/Ankle Snare (traps), Detect (reveal hidden), Spring Trap, Shockwave Trap
- Enables: Ranged DPS, trap-based area control, falcon auto-blitz

**Deliverable**: 5 playable second classes covering Tank/Healer/AoE DPS/Melee DPS/Ranged DPS. ~50 new skills. True party composition and role diversity.

---

## Phase 7: Monster Skills & MVP System (1.5-2 weeks)

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

## Phase 8: World Expansion (2-3 weeks)

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

## Phase 9: Job Change & Quest System (1.5-2 weeks)

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

## Phase 10: Item & Equipment Deep Dive (2 weeks)

**Why now**: With 5+ second classes, players need class-appropriate equipment. The current 148 items don't cover weapon types for all classes. Refining adds equipment progression. Cards add build diversity.

**Server-side**:
- Expand item DB to 500+ items (weapons for all weapon types, armor sets per level tier)
- Refining system: +0 to +10, success rates per weapon level, Refine NPC, Elunium/Oridecon consumables
- Card system: compound cards into slotted equipment, card effects applied to stats
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

## Phases 11-15: Future Scope (Not Detailed Yet)

These phases are deferred because they don't block core gameplay. Plan them when Phases 0-10 are complete.

| Phase | System | Reason to Defer |
|-------|--------|-----------------|
| 11 | Guild System | Requires party system first. WoE requires guilds + PvP + castles. |
| 12 | Economy & Trading | Vending/buying stores need more items and players to be meaningful. |
| 13 | PvP & War of Emperium | Requires balanced classes, guilds, and dedicated PvP zones. |
| 14 | Companions (Pets, Homunculus) | Nice-to-have. Doesn't block core progression. |
| 15 | Art, Audio, Polish | Asset work can happen anytime. Placeholder meshes are fine during development. |

---

## Time Estimates & Dependency Graph

```
Phase 0: Critical Fixes .................. 1-2 days      ✓ DONE
  |
Phase 0.5: Element Table Audit ........... 0.5 days
  |
Phase 1: Server Extraction ............... 3-5 days      (DEFERRED)
  |
Phase 2: Status Effects + Buff System .... 1.5-2 weeks   ✓ DONE
  |
Phase 3: Passive Skills + 6 First Classes  2 weeks
  |
  +---> Phase 4: Party System ............ 1-1.5 weeks
  |       |
  |       +---> Phase 5: Chat Expansion .. 3-5 days
  |
  +---> Phase 6: 5 Second Classes ........ 3-4 weeks
          |
          +---> Phase 7: Monster Skills .. 1.5-2 weeks
          |
          +---> Phase 8: World Expansion . 2-3 weeks
                  |
                  +---> Phase 9: Quests .. 1.5-2 weeks
                  |
                  +---> Phase 10: Items .. 2 weeks
```

**Critical path**: 0 -> 0.5 -> 2 -> 3 -> 6 -> 7 (combat depth)
**Social path**: 0 -> 0.5 -> 2 -> 3 -> 4 -> 5 (party play)
**Content path**: 0 -> 0.5 -> 2 -> 3 -> 8 -> 9 -> 10 (world + items)

**Phases 4-5 can run in parallel with Phase 6.** They share no code dependencies.
**Phase 8 can start as soon as Phase 3 is done** (zone creation doesn't need second-class skills).

**Total estimated time for Phases 0-10**: ~16-22 weeks (4-5.5 months)

---

## Cost vs Benefit Analysis

### Highest ROI (Do These First)

| Phase | Cost | Benefit | ROI |
|-------|------|---------|-----|
| 0: Critical Fixes | 1-2 days | Eliminates security holes, crash bugs, SP timing deviation | **Extreme** — prevents exploitation |
| 0.5: Element Table Audit | 0.5 days | Fixes 8+ confirmed wrong element values, all combat math corrected | **Extreme** — every hit in the game is affected |
| 2: Status Effects | 1.5-2 weeks | Unblocks 100+ skills, proper buff/debuff system | **Extreme** — 1 week investment saves months of per-skill hacking |
| 3: First Classes | 2 weeks | 6 playable classes (from 2), 40+ new skills, healing exists | **Very High** — 3x content, enables party roles |
| 4: Party System | 1-1.5 weeks | Group play, EXP sharing, party HP bars | **Very High** — converts tech demo to MMO |

### Good ROI (Do After Core)

| Phase | Cost | Benefit | ROI |
|-------|------|---------|-----|
| 1: Server Extract | 3-5 days | Faster development for all subsequent skill work | **High** — compound benefit |
| 5: Chat Expansion | 3-5 days | Whisper + party chat, social interaction | **High** — small cost, big social impact |
| 6: Second Classes | 3-4 weeks | 50+ new skills, endgame class diversity | **High** — progression depth |

### Moderate ROI (Important but Can Wait)

| Phase | Cost | Benefit | ROI |
|-------|------|---------|-----|
| 7: Monster Skills | 1.5-2 weeks | PvE challenge, MVP bosses | **Moderate** — combat depth |
| 8: World Expansion | 2-3 weeks | 15-20 zones, exploration content | **Moderate** — mostly level design labor |
| 9: Quest System | 1.5-2 weeks | Job change quests, NPC dialogue | **Moderate** — progression structure |
| 10: Item Deep Dive | 2 weeks | Refining, cards, 500+ items | **Moderate** — equipment depth |

### Low ROI Right Now (Defer)

| System | Cost | Why Defer |
|--------|------|-----------|
| Guild System | 2+ weeks | No parties yet, no WoE content to use guilds for |
| PvP/WoE | 3+ weeks | Requires balanced classes, guilds, dedicated zones |
| Companions | 2+ weeks | Nice-to-have, doesn't affect core progression |
| Vending/Trading | 1-2 weeks | Need more items and players first |
| Art/Audio | Ongoing | Placeholder meshes work fine during development |
| Full test suite | 3+ weeks | 129 unimplemented tests. Write tests alongside features instead |

---

## What MUST Be Refactored Now vs Later

### Refactor NOW (Phases 0-2):

1. **Status effect architecture** — Building this generic system now prevents per-skill hardcoding. Every skill added without it creates debt. Current hardcoded effects (Frost Diver freeze, Stone Curse petrify, Provoke) must be migrated.

2. **Buff system architecture** — Same reasoning. Blessing, Increase AGI, Angelus, Kyrie Eleison, Two-Hand Quicken — all need a generic buff system. Building each one ad-hoc creates unmaintainable spaghetti.

3. **Skill handler extraction** — Moving skill handlers out of index.js now means every new skill (100+) goes into a clean module. The cost is 1-2 days. The compound benefit is enormous.

4. **SP deduction timing** — Fix now while only 17 skills exist. Fixing later means auditing 100+ skills.

### Refactor LATER (Fine to defer):

1. **Full server modularization** — The REST API, inventory handlers, enemy AI, zone management work fine in index.js. Extract them when they need changes, not proactively.

2. **AC_HUDManager Blueprint cleanup** — The old Blueprint HUD manager is legacy code alongside the new C++ subsystems. It works. Clean it up when you need to modify those Blueprints.

3. **Test infrastructure** — The 196 test function stubs are aspirational. Don't try to fill them all in. Write tests alongside the features they test.

4. **Dead code removal** — `ASabriMMOPlayerController`, `BP_GameFlow`, Blueprint damage numbers, variant code. These cause no harm and removing them risks breakage. Defer.

---

## Success Criteria

After completing Phases 0-6, the game should have:
- Zero known critical security issues
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

**Estimated time to reach this milestone: 10-14 weeks.**

---

## Appendix: Skills Implementation Priority Queue

For reference during Phases 3 and 6, here are the skills in priority order:

### Phase 3 — First Class Skills (~40 skills)
```
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
  Double Strafe (304) — 2-hit ranged
  Arrow Shower (305) — AoE ranged
  Arrow Repel (306) — knockback ranged

ACOLYTE (new class — HIGHEST PRIORITY):
  Heal (403), Blessing (404), Increase AGI (405)
  Decrease AGI (406), Angelus (407), Cure (408)
  Pneuma (409), Teleport (410), Warp Portal (411)
  Ruwach (412), Signum Crucis (413), Aqua Benedicta (414)

THIEF (new class):
  Steal (503), Hiding (504), Envenom (505)
  Detoxify (506), Sand Attack (507), Backslide (508)
  Throw Stone (509), Pick Stone (510)

MERCHANT (new class):
  Mammonite (603) — zeny attack
  Cart Revolution (604) — cart AoE
```

### Phase 6 — Second Class Skills (~50 skills)
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
