# Audit: Party, Guild, WoE & PvP

> **Auditor**: Claude Opus 4.6 | **Date**: 2026-03-22
> **Deep Research Docs**: 23_Party_System.md, 24_Guild_System.md, 25_War_of_Emperium.md, 26_PvP_System.md
> **Server Source**: `server/src/index.js` (~31,000 lines), `database/migrations/add_party_system.sql`
> **Method**: Grep + line-by-line read of all party/guild/WoE/PvP code paths against deep research specifications

---

## Summary

| System | Implemented | Total Features | Coverage |
|--------|-------------|----------------|----------|
| **Party System** | 22/33 | 33 | **67%** |
| **Guild System** | 0/26 | 26 | **0%** |
| **War of Emperium** | 0/44 | 44 | **0%** |
| **PvP System** | 2/19 | 19 | **11%** |

The party system is the only substantially implemented system, with solid coverage of core mechanics (creation, invite, kick, leave, EXP sharing, chat, DB persistence, HP/SP sync). Guild, WoE, and PvP systems have zero server-side implementation. PvP has a global toggle (`PVP_ENABLED = false`) and player-vs-player auto-attack/skill damage paths exist in the combat tick, but no arena system, point tracking, or ranking exists.

---

## Party System (22/33 features implemented)

### Implemented (22 features)

| # | Feature | Status | Implementation Notes |
|---|---------|--------|---------------------|
| 1 | Party creation | DONE | `party:create` handler, 1-24 char name validation, unique name check |
| 2 | Basic Skill Lv7 gate for creation | DONE | Checks `player.learnedSkills[1] >= 7` at line 8375 |
| 3 | Max 12 members | DONE | `MAX_PARTY = 12`, enforced in invite handler |
| 4 | Remote invite (no proximity) | DONE | Searches all `connectedPlayers` by name |
| 5 | 30-second invite timeout | DONE | `expiresAt: Date.now() + 30000` at line 8454 |
| 6 | Accept/reject invite | DONE | `party:invite_respond` handler |
| 7 | Prevent inviting players already in party | DONE | `targetPlayer.partyId` check at line 8448 |
| 8 | Leader-only invite | DONE | `party.leaderId !== characterId` check at line 8434 |
| 9 | Leader-only kick | DONE | `party.leaderId !== characterId` check at line 8570 |
| 10 | Self-leave by any member | DONE | `party:leave` handler |
| 11 | Cannot kick yourself | DONE | `targetCharId === characterId` check at line 8573 |
| 12 | Auto leader transfer on leave | DONE | First online member becomes leader at line 8544-8551 |
| 13 | Manual leader delegation | DONE | `party:change_leader` handler, requires target online |
| 14 | Party dissolution on empty | DONE | Deletes from DB when `party.members.size === 0` at line 8536 |
| 15 | DB persistence | DONE | `parties` + `party_members` tables, auto-created at startup |
| 16 | Offline member tracking | DONE | Members stay in party on disconnect, `party:member_offline` broadcast at line 7253 |
| 17 | Party data on login | DONE | Loaded from DB during `player:join`, plus `party:load` re-request handler |
| 18 | Even Share EXP (+20%/member) | DONE | `PARTY_EVEN_SHARE_BONUS = 20`, formula at line 5214 |
| 19 | 15-level range check | DONE | `PARTY_SHARE_LEVEL_GAP = 15`, `recalcEvenShareEligibility()` at line 5179 |
| 20 | Same-map EXP filter | DONE | `member.map !== enemyZone` check at line 5207 |
| 21 | Party chat (% prefix + PARTY channel) | DONE | `%` prefix routing at line 8868, `party:chat` handler at line 8667, cross-map delivery |
| 22 | EXP mode change chat broadcast | DONE | Chat notification at line 8656 when leader changes EXP mode |
| 23 | Dead member exclusion from EXP | DONE | `p.health <= 0` check at line 5209 excludes dead members |
| 24 | Tap bonus (+25%/attacker) | DONE | Line 2218, applied before party distribution |
| 25 | HP/SP sync to party members | DONE | 1-second interval at line 26549, plus immediate broadcast on damage |
| 26 | Item share mode at creation | DONE | `itemShare` and `itemDistribute` fields stored in DB and party state |
| 27 | Devotion party requirement | DONE | `player.partyId === target.partyId` check in Devotion handler |
| 28 | Lullaby party immunity | DONE | `casterPartyId` check at line 1365 |

### Not Implemented (11 features)

| # | Feature | Priority | Deep Research Reference | Notes |
|---|---------|----------|------------------------|-------|
| 1 | **Basic Skill Lv5 gate for joining** | Low | 23_Party_System: "join requirement: Novice Basic Skill Level 5" | Creation gate exists (Lv7) but join gate is missing |
| 2 | **Same-account block** | Low | 23_Party_System: "Characters from the same account cannot join the same party" | No `user_id` check in invite_respond handler |
| 3 | **Leader delegation same-map check** | Low | 23_Party_System: "Current leader must be on the same map as the target" | `change_leader` only checks online status, not same map |
| 4 | **Tap bonus 580% cap** | Low | 23_Party_System: "Cap 580% maximum" | Current cap: `Math.min(11, attackerCount - 1)` = max 375%. Should be `Math.min(19, attackerCount - 1)` for 580% |
| 5 | **+1 EXP floor per member** | Low | 23_Party_System: "floor(Total EXP / eligible_member_count) + 1" | Current formula: `Math.floor(...)` without +1. Very low EXP monsters could give 0 per member |
| 6 | **Item distribution logic** | Medium | 23_Party_System: "Each Take vs Party Share vs Shared (random)" | Fields are stored in DB but no runtime logic uses them for item drops |
| 7 | **Item share mode immutability** | Medium | 23_Party_System: "cannot be changed without disbanding" | Item share mode is set at creation but nothing prevents changing it |
| 8 | **Party window lock icon (skill targeting)** | Medium | 23_Party_System: "Clicking a party member name casts the selected skill on them" | Client-side feature not yet implemented |
| 9 | **Pink dots on minimap** | Medium | 23_Party_System: "Party members appear as pink/magenta dots on the minimap" | Client-side feature not yet implemented |
| 10 | **Party member map name display** | Low | 23_Party_System: "Shows which map each member is currently on" | Map data is broadcast via `party:member_update` but not displayed in widget |
| 11 | **Dead member visual indicator** | Low | 23_Party_System: "Dead members show a skull icon" | Client-side visual feature |

### Formula Discrepancies (Party)

| # | Formula | Deep Research (Correct) | Server Implementation | Severity |
|---|---------|------------------------|----------------------|----------|
| F1 | **Tap bonus cap** | 580% max (19 additional attackers + base) | 375% max (`Math.min(11, ...)` at line 2218) | Low |
| F2 | **+1 EXP floor** | `floor(EXP * bonus / n) + 1` per member | `Math.floor(EXP * bonus / 100 / n)` -- no +1 | Low |
| F3 | **Base + Job EXP same formula** | Both use identical Even Share formula | Both use same formula -- CORRECT | None |

---

## Guild System (0/26 features implemented)

The guild system has **zero server-side implementation**. The word "guild" appears only in Potion Pitcher's targeting comment ("party/guild member") -- there are no `guild:*` socket handlers, no guild data structures, no guild tables, and no guild skill logic.

### All Missing Features (26)

| # | Feature | Priority | Deep Research Reference |
|---|---------|----------|------------------------|
| 1 | **Guild creation** (consume Emperium) | Critical | 24_Guild_System: `/guild "Name"`, consume item ID 714 |
| 2 | **Guild invite/join** | Critical | 24_Guild_System: 20 position slots, capacity check |
| 3 | **Guild leave/kick/disband** | Critical | 24_Guild_System: GM cannot leave, must disband |
| 4 | **Guild master transfer** | Medium | 24_Guild_System: 24h cooldown, blocked during WoE |
| 5 | **20 position slots** (name, invite, kick, storage, tax) | High | 24_Guild_System: Position 0 = GM (immutable), 1-19 configurable |
| 6 | **Guild EXP tax** | High | 24_Guild_System: 0-50% Base EXP deducted per kill, Job EXP never taxed |
| 7 | **Guild leveling** (50 levels, 1 skill point each) | High | 24_Guild_System: EXP table, level up broadcast |
| 8 | **Guild Extension** (+4 members/level, max 56) | High | 24_Guild_System: `maxMembers = 16 + extensionLevel * 4` |
| 9 | **Official Guild Approval** | Critical | 24_Guild_System: Gate for WoE participation |
| 10 | **Kafra Contract** | Medium | 24_Guild_System: Free Kafra inside owned castles |
| 11 | **Guardian Research** | Medium | 24_Guild_System: Enables hiring guardians |
| 12 | **Strengthen Guardians** (3 levels) | Medium | 24_Guild_System: +HP/ATK/ASPD to guardians |
| 13 | **Battle Orders** (WoE only, +5 STR/DEX/INT) | High | 24_Guild_System: 31x31 area, 60s duration, 5-min shared cooldown |
| 14 | **Regeneration** (3 levels, WoE only) | Medium | 24_Guild_System: HP/SP recovery rate multiplier |
| 15 | **Restore** (WoE only, 10s cast, 90% HP/SP) | Medium | 24_Guild_System: Pre-renewal 10s cast time |
| 16 | **Emergency Call** (WoE only, teleport all) | High | 24_Guild_System: 5s uninterruptible cast, 5-min shared cooldown |
| 17 | **Passive auras** (Leadership/Wounds/Cold/HawkEyes) | Medium | 24_Guild_System: 5 levels each, +1 stat/lv, WoE only |
| 18 | **Permanent Development** | Low | 24_Guild_System: 50% bonus economy point chance |
| 19 | **Guild's Glory** (emblem) | Low | 24_Guild_System: Emblem permission gate |
| 20 | **Guild emblem** (24x24 BMP/GIF) | Medium | 24_Guild_System: base64 storage, broadcast |
| 21 | **Alliance system** (max 3 allies, mutual) | Medium | 24_Guild_System: Green names in WoE, no friendly fire |
| 22 | **Enemy system** (unilateral, red names) | Medium | 24_Guild_System: Open-world PvP between rival guilds |
| 23 | **Guild chat** ($prefix, GUILD channel) | High | 24_Guild_System: Cross-map, distinct color |
| 24 | **Guild notice** (title 60 chars + body 240 chars) | Low | 24_Guild_System: Display on login/map change |
| 25 | **Login/logout broadcasts** | Medium | 24_Guild_System: "[Name] has connected/disconnected" |
| 26 | **Guild storage** (100 slots/level, 5 levels) | Medium | 24_Guild_System: Renewal feature but recommended for QoL |

### Required Database Tables (0/6 exist)

| Table | Status | Columns |
|-------|--------|---------|
| `guilds` | MISSING | guild_id, name, master_id, level, exp, max_members, skill_points, notice_title, notice_body, emblem_data |
| `guild_positions` | MISSING | guild_id, position_id (0-19), name, can_invite, can_kick, can_storage, tax_rate |
| `guild_members` | MISSING | guild_id, character_id, position_id, contributed_exp |
| `guild_skills` | MISSING | guild_id, skill_id, skill_level |
| `guild_storage` | MISSING | guild_id, item_id, quantity, slot_index, deposited_by |
| `guild_alliances` | MISSING | guild_id_1, guild_id_2, alliance_type |

---

## War of Emperium (0/44 features implemented)

WoE has **zero implementation**. No WoE scheduler, no castle system, no Emperium entity, no guardians, no combat modifications, no treasure system. The server has no concept of WoE state, WoE zones, or castle ownership.

### All Missing Features (44)

#### Phase 1: Core Infrastructure (11 features)

| # | Feature | Priority | Deep Research Reference |
|---|---------|----------|------------------------|
| 1 | **WoE scheduler** (state machine: INACTIVE/PRE/ACTIVE/POST) | Critical | 25_WoE: Configurable schedule, 2h sessions |
| 2 | **Server-wide announcements** | Critical | 25_WoE: Start/end/capture broadcasts |
| 3 | **Castle DB table** (20 castles, 4 realms) | Critical | 25_WoE: woe_castles with owner_guild_id, economy levels |
| 4 | **Castle state loading/saving** | Critical | 25_WoE: Load at PRE_WOE, save at POST_WOE |
| 5 | **Zone flags** (woe, gvg, noKnockback) | Critical | 25_WoE: Zone registry additions |
| 6 | **Warp non-owners** on WoE start/end | High | 25_WoE: Non-allied warped to save points |
| 7 | **Emperium entity** (HP 68430 + defense investment) | Critical | 25_WoE: Holy Lv1, Angel race, Small size, Boss class |
| 8 | **Emperium damage rules** (auto-attack only) | Critical | 25_WoE: All skills miss, Holy = 0% damage |
| 9 | **Castle conquest** (Emperium break -> ownership change) | Critical | 25_WoE: Warp non-allies, respawn Emperium, announcement |
| 10 | **Multiple breaks per session** | High | 25_WoE: Castle can change hands repeatedly |
| 11 | **Official Guild Approval check** | Critical | 25_WoE: Only guilds with this skill can attack Emperium |

#### Phase 2: Combat Modifications (8 features)

| # | Feature | Priority |
|---|---------|----------|
| 12 | **-40% skill damage reduction** | Critical |
| 13 | **-25% long-range normal attack reduction** | High |
| 14 | **-20% Flee reduction** | High |
| 15 | **4x trap duration** | Medium |
| 16 | **Knockback disabled** | Critical |
| 17 | **Disabled skills** (Teleport, Warp Portal, Ice Wall, Basilica, Assumptio, Intimidate, etc.) | Critical |
| 18 | **Phen Card suppression** (cast-interrupt immunity disabled) | Medium |
| 19 | **Portal invulnerability** (5 seconds on room transition) | High |

#### Phase 3: Guardian System (6 features)

| # | Feature | Priority |
|---|---------|----------|
| 20 | **3 guardian types** (Soldier/Knight/Archer) | High |
| 21 | **Guardian AI** (chase, attack, leash) | High |
| 22 | **Guardian skills** (Bash, Stun Attack, Arrow Shower, Double Strafe) | Medium |
| 23 | **Guardian stats** (base + Strengthen Guardian + Defense investment) | Medium |
| 24 | **Guardian death/respawn** (30s cooldown) | Medium |
| 25 | **Guardian ownership change** on castle capture | High |

#### Phase 4: Castle Economy (7 features)

| # | Feature | Priority |
|---|---------|----------|
| 26 | **Commerce investment** (2/day, 2x cost scaling) | Medium |
| 27 | **Defense investment** (+1000 HP to Emperium/guardians) | Medium |
| 28 | **Treasure box generation** (4 base + commerce bonus, daily midnight) | High |
| 29 | **Treasure Room NPC** (GM only) | Medium |
| 30 | **Treasure loot tables** (per-realm, 0.4% God Item material) | Medium |
| 31 | **Investment inheritance** on castle loss | Low |
| 32 | **Permanent Development** guild skill (50% bonus) | Low |

#### Phase 5: Navigation & Dungeons (6 features)

| # | Feature | Priority |
|---|---------|----------|
| 33 | **Castle flag NPCs** (defender teleport) | Medium |
| 34 | **Castle room portals** (multi-room navigation) | High |
| 35 | **Guild Dungeon access NPC** | Medium |
| 36 | **3-floor dungeons per realm** (12 zones) | Medium |
| 37 | **GvG on Floor 3** (Hall of Abyss) | Medium |
| 38 | **Guild dungeon monsters + MVPs** | Medium |

#### Phase 6: Client-Side (6 features)

| # | Feature | Priority |
|---|---------|----------|
| 39 | **WoESubsystem** (UWorldSubsystem) | High |
| 40 | **WoE timer/status widget** | High |
| 41 | **Emperium HP bar** | High |
| 42 | **Castle ownership display** | Medium |
| 43 | **Guardian actors** (reuse enemy system) | Medium |
| 44 | **WoE announcements in chat** | High |

---

## PvP System (2/19 features implemented)

### Implemented (2 features)

| # | Feature | Status | Notes |
|---|---------|--------|-------|
| 1 | **PVP_ENABLED toggle** | DONE | Global `const PVP_ENABLED = false` at line 161. Currently disabled. |
| 2 | **Player-vs-player damage paths** | DONE | Auto-attack PvP damage path exists (line 7619+), skill PvP targeting paths exist for some skills (Magnum Break, Bowling Bash, etc.). Party/guild ally checks present. |

### Not Implemented (17 features)

| # | Feature | Priority | Deep Research Reference |
|---|---------|----------|------------------------|
| 1 | **PvP arena zones** (zone registry entries) | Critical | 26_PvP: `pvp_y_*` maps with `pvp` mapflag |
| 2 | **PvP entry NPC** (500 Zeny fee, Level 31+) | High | 26_PvP: Inn buildings in major cities |
| 3 | **Point system** (+5 start, +1 kill, -5 death) | Critical | 26_PvP: Core PvP mechanic |
| 4 | **Auto-ejection at 0 points** | Critical | 26_PvP: Warp to save point |
| 5 | **Spawn invulnerability** (5 seconds) | High | 26_PvP: Canceled on movement/attack/skill |
| 6 | **PvP rankings** (per-map, per-session) | High | 26_PvP: Points descending, ties by kills |
| 7 | **Top 10 aura visual effects** | Medium | 26_PvP: Progressive white glow under feet |
| 8 | **TIME ATTACK!! system** | Low | 26_PvP: Timer at Rank 1 |
| 9 | **Fly Wing/Teleport block in PvP zones** | High | 26_PvP: Only these two items/skills disabled |
| 10 | **Butterfly Wing allowed** (exit method) | Medium | 26_PvP: Works as intended exit |
| 11 | **PvP kill/death feed** (chat messages) | Medium | 26_PvP: Announcements in chat |
| 12 | **Player race = Demi-Human** (card calculations) | High | 26_PvP: Hydra/Thara Frog bonuses |
| 13 | **Player size = Medium** | Medium | 26_PvP: Size modifier calculations |
| 14 | **Player default element = Neutral Lv1** | High | 26_PvP: Armor cards change element |
| 15 | **Duel system** (@duel, max 3, no penalties) | Medium | 26_PvP: Consensual 1v1/3-way anywhere |
| 16 | **Resurrection in PvP** (if points > 0) | Medium | 26_PvP: Priest can revive, player stays |
| 17 | **Persistent PvP stats** (pvp_stats table) | Low | 26_PvP: Lifetime kills, deaths, streak |
| 18 | **PvP rank HUD element** | Medium | 26_PvP: "Rank X / Y" in lower-right corner |
| 19 | **PvP zone indicator** | Low | 26_PvP: "PvP Zone" text on screen |

### PvP Player Classification (Missing)

The server currently has **no player race/size/element classification**. This affects all player-vs-player damage calculations:

- **Race**: Players are not tagged as `demihuman`. Hydra Card (+20% vs Demi-Human) and Thara Frog Card (-30% vs Demi-Human) would not apply correctly in PvP.
- **Size**: Players are not tagged as `medium`. Weapon size penalties (Spear 75% vs Medium) would not apply.
- **Element**: Players default to having no explicit element. Armor cards that change element (Ghostring -> Ghost, Evil Druid -> Undead) would not interact with elemental damage tables.

---

## Critical Missing Features

### Tier 1: Blocks Major Game Systems

| # | System | Feature | Impact |
|---|--------|---------|--------|
| 1 | **Guild** | Entire system | WoE cannot exist without guilds. Blocks all endgame GvG content. |
| 2 | **WoE** | Entire system | Flagship endgame content. No castles, no treasure, no God Items. |
| 3 | **PvP** | Arena system | No structured PvP. Only raw `PVP_ENABLED` toggle exists. |
| 4 | **PvP** | Player race/size/element | Card bonuses for player-vs-player combat would miscalculate. |

### Tier 2: Incomplete Party Features

| # | Feature | Impact |
|---|---------|--------|
| 5 | Item distribution logic | Item drops not distributed according to party settings |
| 6 | Party window skill targeting (lock icon) | Priest/Acolyte cannot efficiently heal party members |

### Tier 3: Polish & Accuracy

| # | Feature | Impact |
|---|---------|--------|
| 7 | Tap bonus cap (375% vs 580%) | Minor -- affects only 12+ attacker scenarios |
| 8 | +1 EXP floor | Very minor -- only matters for extremely low EXP monsters |
| 9 | Same-account party block | Anti-abuse -- prevents multiboxing in same party |
| 10 | Guild chat ($prefix, GUILD channel) | Chat system has `// case 'GUILD':` placeholder at line 8918 but no handler |

---

## Formula Discrepancies

| # | System | Formula | Deep Research Value | Server Value | Location | Severity |
|---|--------|---------|-------------------|--------------|----------|----------|
| 1 | Party EXP | Tap bonus cap | 580% (19 extra attackers) | 375% (`Math.min(11, ...)`) | Line 2218 | Low |
| 2 | Party EXP | +1 EXP floor | `floor(EXP/n) + 1` | `Math.floor(EXP/n)` (no +1) | Line 5215-5216 | Low |
| 3 | Party EXP | Even Share bonus | `+20%` per eligible member | `+20%` per eligible member | Line 5214 | None (correct) |
| 4 | Party EXP | Level gap | 15 base levels | 15 base levels | Line 150 | None (correct) |
| 5 | Party | Max members | 12 | 12 | Line 149 | None (correct) |
| 6 | PvP | Skill damage reduction | 0% in PvP arenas, -40% in WoE | N/A (not implemented) | N/A | N/A |
| 7 | WoE | Emperium HP | 68,430 + defense*1000 | N/A (not implemented) | N/A | N/A |
| 8 | WoE | Emperium DEF | 64 (hard) + 43 (soft) | N/A (not implemented) | N/A | N/A |
| 9 | Guild | Extension per level | +4 members (iRO Classic) | N/A (not implemented) | N/A | N/A |
| 10 | Guild | Passive aura max level | 5 levels (+1 stat/level) | N/A (not implemented) | N/A | N/A |

---

## Recommended Implementation Order

### Phase 1: Guild System Foundation (HIGH PRIORITY)
WoE cannot function without guilds. The guild system should be implemented first as it is also a standalone social feature.

1. **Guild DB tables** (guilds, guild_positions, guild_members, guild_skills)
2. **Guild creation** (consume Emperium, create 20 default positions)
3. **Guild invite/join/leave/kick/disband**
4. **Guild EXP tax** (deduct % of Base EXP, add to guild pool)
5. **Guild leveling** (level 1-50, +1 skill point/level)
6. **Guild Extension** (+4 members/level)
7. **Guild chat** ($prefix -> GUILD channel, cross-map)
8. **Guild data on login** (load from DB, send to client)
9. **Client: GuildSubsystem + SGuildWidget** (Alt+G toggle)

### Phase 2: Guild Skills & Features
1. **Official Guild Approval** (WoE gate)
2. **Battle Orders / Regeneration / Restore / Emergency Call** (WoE-only active skills)
3. **Passive auras** (Leadership/Wounds/Cold/HawkEyes)
4. **Alliance/enemy system**
5. **Guild emblem** (base64 upload/broadcast)
6. **Guild notice** (title + body, display on login)
7. **Guild storage** (100 slots/level, 5 levels)

### Phase 3: PvP Arena System (MEDIUM PRIORITY)
PvP is independent of guilds/WoE and can be built in parallel.

1. **Player classification** (race=demihuman, size=medium, element=neutral1)
2. **PvP zone flags** in zone registry
3. **PvP entry NPC** (fee, level check)
4. **Point system** (+5 start, +1 kill, -5 death, eject at 0)
5. **Spawn invulnerability** (5 seconds)
6. **PvP rankings** (per-map, broadcast on kill)
7. **Fly Wing/Teleport block** in PvP zones
8. **Client: PvP rank display** + kill feed
9. **Duel system** (challenge/accept/reject)

### Phase 4: WoE Core (HIGH PRIORITY, requires Phase 1+2)
1. **WoE scheduler** (state machine with configurable times)
2. **Castle DB** (20 castles seeded, owner_guild_id)
3. **Emperium entity** (68,430 HP, auto-attack only, Holy immunity)
4. **Castle conquest** (break -> ownership change -> warp non-allies)
5. **WoE combat modifications** (-40% skill dmg, -25% ranged, no knockback, no flee, disabled skills)
6. **Server-wide announcements**
7. **Client: WoESubsystem** (timer, castle ownership, Emperium HP)

### Phase 5: WoE Economy & Guardians
1. **Guardian system** (3 types, AI, stats from investment)
2. **Commerce/Defense investment** NPCs
3. **Treasure box generation** (daily, per-realm loot tables)
4. **Castle flags** (defender teleport)
5. **Guild dungeons** (3 floors, GvG on floor 3)

### Phase 6: Party System Remaining Gaps (LOW PRIORITY)
1. Fix tap bonus cap (375% -> 580%)
2. Add +1 EXP floor
3. Item distribution logic
4. Same-account party block
5. Basic Skill Lv5 join gate
6. Leader delegation same-map check
7. Client: party window lock icon, minimap dots, dead indicator

### Phase 7: WoE 2 + God Items (DEFERRED)
1. Guardian Stones + Barricades
2. WoE 2 castles (10 additional)
3. Link Flags (12 per castle)
4. God Item Seal Quest system
5. God Item materials + crafting NPCs
