# Audit: World, NPCs, Chat & Misc Systems

> **Audit Date**: 2026-03-22
> **Deep Research Sources**: `29_Maps_Zones_Navigation.md`, `30_NPCs_Shops_Quests.md`, `31_Chat_Social_UI.md`, `32_Death_Teleport_Misc.md`, `35_Novice_SuperNovice.md`, `39_Marriage_Adoption_Misc.md`, `41_Transcendent_Rebirth.md`
> **Server Source**: `server/src/index.js`, `server/src/ro_zone_data.js`
> **Client Source**: `client/SabriMMO/Source/SabriMMO/UI/` (33 subsystem files)

---

## Summary

The project has strong foundational implementations for zones, shops, chat, death/teleportation, and job change. However, large systemic gaps remain in NPC variety, quest infrastructure, social features (guild, friends, marriage, adoption), the transcendent/rebirth system, and world scale. Of the ~420 maps in RO Classic, only 7 zones are defined. Of the deep-researched systems, approximately 35-40% are implemented, 15-20% are partially implemented, and 40-50% are entirely absent.

**Implemented (strong)**: Zone framework with map flags, warp portals, Kafra (save/teleport), NPC shops (5 shops with Discount/Overcharge), chat (global/party/whisper/block), death penalty (1% EXP, Novice exempt, buff clearing), Fly Wing/Butterfly Wing, Warp Portal skill with memo, sitting system, job change (Novice->1st->2nd), player vending, cart system, item identification.

**Partially implemented**: Emote system (monster skill emotes only, no player emotes), PvP (framework exists but globally disabled), zone map flags (defined in data but only `nosave` is checked server-side).

**Not implemented**: Kafra storage, guild system, friend list, marriage/adoption, transcendent/rebirth, Super Novice class, quest system, chat rooms, emote window, NPC dialogue trees, headgear quests, job change quests (narrative), bounty boards, stat/skill reset NPCs, stylist NPCs, refine NPCs (server refine exists but no NPC interaction), day/night cycle, weather effects, minimap, world map, airship/ship travel, PvP arenas, WoE, instance dungeons, Token of Siegfried, Kaizel, Resurrection skill (on dead players), Yggdrasil Leaf (on dead players), Guardian Angel (Super Novice), baby classes, GM broadcast commands.

---

## Zone/Map System

### Deep Research Spec (Doc 29)
- ~420 total maps: ~25 towns, ~180 fields, ~150+ dungeon floors, ~30 PvP/GvG, ~20 instances, ~15 special
- Map flags system: 30+ flags controlling teleportation, combat, saving, drops, weather, PvP/GvG
- Cell-based navigation with A* pathfinding (server-side)
- Inter-continental travel (airships, ships from Alberta)
- Day/night cycle with lighting changes
- Weather effects per zone (rain, snow, fog, sakura, fireworks)

### Current Implementation
- **7 zones defined** in `ro_zone_data.js`: `prontera`, `prontera_south`, `prontera_north`, `prt_dungeon_01`, `prt_dungeon_02`, `geffen`, `geffen_dungeon_01` (approximate from displayName count)
- Zone registry structure is well-designed: each zone has `name`, `displayName`, `type` (town/field/dungeon), `flags` object, `defaultSpawn`, `levelName`, `warps[]`, `kafraNpcs[]`, `enemySpawns[]`
- Map flags are defined per zone (`noteleport`, `noreturn`, `nosave`, `pvp`, `town`, `indoor`) but **only `nosave` is actively checked** server-side (line 6562). Other flags like `noteleport` and `noreturn` are NOT enforced in the Fly Wing/Butterfly Wing handlers
- Warp portals work: `zone:warp` socket handler performs full zone transitions with broadcast `player:left` to old zone and position sync in new zone
- Client `ZoneTransitionSubsystem` handles seamless zone changes
- 50 UE units = 1 RO cell conversion is established
- No pathfinding (click-to-move is client-side only)

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Only 7 of ~420 zones exist | MISSING | High (content) |
| Map flag enforcement (noteleport, noreturn, nomemo, nobranch) | MISSING | High |
| Indoor flag effects (weather suppression, mount rules) | MISSING | Medium |
| PvP/GvG map flags | DEFINED but not enforced | Medium |
| Day/night cycle | MISSING | Low |
| Weather effects (rain, snow, fog, sakura) | MISSING | Low |
| Airship/ship travel routes | MISSING | Low |
| Instance dungeon system (private map copies) | MISSING | Low |
| Cell-based walkability / server pathfinding | MISSING | Medium |
| Minimap with NPC icons | MISSING | Medium |
| World map overlay | MISSING | Low |

---

## NPC & Shop System

### Deep Research Spec (Doc 30)
- Dozens of NPC archetypes: Tool/Weapon/Armor Dealers, Kafra, Healer, Stylist, Refiner, Repairman, Inn, Guide, Arena, Guild
- Each town has 3-5+ shop NPCs with town-specific inventories
- Buy/Sell pricing: sell = floor(buy/2), Discount/Overcharge passive skills
- Kafra services: Save (free), Storage (40z), Teleport (600-2400z), Cart Rental (600-1200z), Kafra Points
- NPC dialogue trees with multi-step interactions
- Vending system (player shops from Pushcart)

### Current Implementation
- **5 NPC shops defined** (`NPC_SHOPS` at line 4549): Tool Dealer (1), Weapon Dealer (2), Armor Dealer (3), General Store (4), Arrow Dealer (5)
- Shop buy/sell with `shop:buy`, `shop:sell`, `shop:buy_batch`, `shop:sell_batch` socket handlers
- **Discount and Overcharge** are fully implemented: `applyDiscount()` (line 4726) and `applyOvercharge()` (line 4731) with correct RO formulas
- **Kafra system** implemented: `kafra:open` (sends destinations and services), `kafra:save` (save point with DB persistence), `kafra:teleport` (pay zeny, zone transition)
- Kafra NPCs defined per zone in `ro_zone_data.js` with destinations and costs
- Client `KafraSubsystem` + `ShopSubsystem` handle UI
- **Player vending** fully implemented: setup, browse, buy, close shop, vendor self-view, live sales
- **Cart system** fully implemented: `CartSubsystem` + `SCartWidget`
- **Item identification** implemented: `SIdentifyPopup`, `identify:item_list/result` handlers

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Kafra Storage (account-wide item storage) | MISSING | High |
| Cart Rental via Kafra (800z) | MISSING (cart exists but no rental fee) | Low |
| Kafra Points system (loyalty rewards) | MISSING | Low |
| Town-specific shop inventories (per-town variation) | MISSING (5 generic shops) | Medium |
| Healer NPCs (free HP/SP recovery) | MISSING | Low |
| Stylist NPCs (hair/color change) | MISSING | Low |
| Refine NPC interaction (NPC dialogue to trigger refine) | PARTIAL (refine:request exists server-side, no NPC trigger) | Medium |
| Repairman NPC (broken equipment repair) | MISSING | Low |
| Inn NPCs (paid HP/SP recovery) | MISSING | Low |
| Guide NPCs (town info/directions) | MISSING | Low |
- Ore Merchant NPC | MISSING | Low |
| Card Removal NPC | MISSING | Medium |
| Stat/Skill Reset NPC (Hypnotist) | MISSING | Medium |
| NPC dialogue tree framework | MISSING | High |
| NPC visual sprites / clickable entities | PARTIAL (Kafra click exists, generic NPC framework missing) | High |
| Buying Store (reverse vending) | MISSING | Low |

---

## Quest System

### Deep Research Spec (Doc 30)
- Job change quests: 6 unique 1st class quests (Swordsman obstacle course, Mage item puzzle, Archer trunk collection, etc.), 12+ 2nd class quests
- Platinum/quest skill quests: item collection to learn bonus skills
- Headgear quests: hundreds of material-to-headgear crafting quests
- Bounty Board: repeatable monster hunting for EXP/Zeny
- Access quests: dungeon/area unlock (Bio Lab, Thanatos Tower, Turtle Island)
- Quest window (Alt+U) with Active/Inactive tabs, kill counters, progress tracking

### Current Implementation
- **Job change is implemented** via `job:change` socket handler (line 7996), but it is a **direct class swap** with validation (Basic Skill Lv9, Job Level 10/40 checks). There are **no narrative quests** -- just an API call
- Quest skill NPC gates were implemented in Phase J of deferred systems remediation (per MEMORY.md)
- No quest tracking system, quest log, or quest window exists

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Quest framework (quest DB, progress tracking, objectives) | MISSING | High |
| Job change quest narratives (per-class stories) | MISSING | Medium |
| Headgear crafting quests | MISSING | Low |
| Bounty Board (repeatable hunting quests) | MISSING | Medium |
| Access/unlock quests (dungeon entry requirements) | MISSING | Medium |
| Quest window UI (Alt+U equivalent) | MISSING | Medium |
| Quest NPCs with "!" and "?" markers | MISSING | Medium |
| Kill counter popups for quest objectives | MISSING | Low |
| Daily/repeatable quest system | MISSING | Low |

---

## Chat System

### Deep Research Spec (Doc 31)
- Channels: Global (white, overhead bubble), Whisper (yellow, PM), Party (green, % prefix), Guild (cyan, $ prefix), GM Broadcast
- Chat commands: 40+ slash commands (/w, /ex, /exall, /in, /inall, /em, /memo, /where, /who, /sit, /stand, /effect, /bgm, etc.)
- Overhead chat bubbles (5s fade, black outline)
- Chat room system (Alt+C, private password-protected rooms)
- Emote system: 50+ emotes (Alt+L window, /lv /swt /an etc., Alt+number shortcuts)
- Macro system (Alt+M, 10 slots)
- Friend list (Alt+H, 40 friends max, online/offline indicators, /hi greeting)
- Block/ignore list (per-character, whisper + public chat filtering)

### Current Implementation
- **Global chat**: `chat:message` handler (line 8682) with broadcast to all via `io.emit('chat:receive')`
- **Party chat**: `%` prefix routing to `broadcastToParty()` (line 8876) -- green channel
- **Whisper**: `/w "Name" message` parsing (line 8704-8760), target lookup, delivery via socket emit, whisper sent confirmation
- **Block/ignore**: `/ex name` (line 8786), `/exall` (line 8791), `/in name` (line 8799), `/inall` (line 8803) -- `whisperBlockList` Set per player
- **Auto-reply**: `/em message` (line 8813) sets `autoReplyMessage`, auto-responds to whispers
- **Reply**: `/reply message` (line 8774) fills last whisperer target
- **Memo commands**: `/memo1`, `/memo2`, `/memo3` for Warp Portal destinations (line 8825-8857)
- **System messages**: SYSTEM channel for combat feedback, party events, MVP announcements
- Client `ChatSubsystem` handles all chat UI
- **Sitting system**: `player:sit` / `player:stand` handlers (line 7443-7467) with buff application and zone broadcast

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Guild chat ($ prefix, /gc) | MISSING (commented out at line 8918) | High |
| GM broadcast (/b, /nb, /lb) | MISSING | Low |
| Overhead chat bubbles (client-side) | UNKNOWN (not in subsystem audit) | Medium |
| Chat room system (Alt+C, private rooms) | MISSING | Low |
| Emote system (50+ emotes, Alt+L window) | MISSING (only monster emotes at line 29517) | Medium |
| Macro system (Alt+M, 10 slots) | MISSING | Low |
| Friend list (Alt+H, add/remove/online status) | MISSING | Medium |
| /where command (current map + coordinates) | MISSING | Low |
| /who command (online player count) | MISSING | Low |
| /effect, /mineffect, /bgm, /sound commands | MISSING | Low |
| /noctrl, /noshift, /battlemode commands | MISSING | Medium |
| Chat window resize (F10 cycle) | MISSING | Low |
| Chat history scroll (PageUp/PageDown) | MISSING | Low |
| Message length cap (255 chars) | NOT ENFORCED | Low |

---

## Death & Resurrection

### Deep Research Spec (Doc 32)
- Death sequence: fall animation, lie on ground, death UI with "Return to Save Point" / "Wait here"
- EXP penalty: 1% of next level's required EXP (base + job), applied at moment of death
- Exemptions: Novice (0%), Baby classes (0.5%), max level (0%), PvP/WoE/town maps (0%), Token of Siegfried, Kaizel, Life Insurance
- Buff clearing on death (BUFFS_SURVIVE_DEATH whitelist)
- Super Novice Guardian Angel (death-save at 99-99.9% EXP)
- Resurrection: Priest skill (Lv1-4, Blue Gem, 10-80% HP), Yggdrasil Leaf (any class, 10% HP), Redemptio (party AoE rez)
- Token of Siegfried: self-rez, 100% HP/SP, no penalty
- Kaizel: auto-rez buff, invulnerability
- Respawn at save point with HP/SP percentage

### Current Implementation
- **Death penalty**: `applyDeathPenalty()` (line 2076) correctly implements 1% base + 1% job EXP loss, Novice exempt, cannot delevel, DB persistence
- **Buff clearing**: `clearBuffsOnDeath()` (line 2038) with `BUFFS_SURVIVE_DEATH` whitelist (auto_berserk, endure, shrink, songs/dances)
- Death penalty called from ALL death paths: PvE combat, monster skills, DoTs, Grand Cross self-damage, Abracadabra SA_DEATH (18 call sites found)
- Pet intimacy loss on death (-20, line 2117)
- Homunculus handling on owner death (line 1995)
- Client handles death UI (respawn button exists in Blueprint)

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| `player:respawn` socket handler | MISSING (no server-side respawn handler found) | Critical |
| Respawn at save point (warp to saved location) | MISSING (death penalty applies but no respawn flow) | Critical |
| "Wait here" option (wait for resurrection) | MISSING | High |
| Resurrection skill on dead players | MISSING (Resurrection exists but targets Undead monsters) | High |
| Yggdrasil Leaf (item-based rez on dead players) | MISSING | High |
| Token of Siegfried (self-rez, no penalty) | MISSING | Medium |
| Kaizel buff (auto-rez on death) | MISSING | Medium |
| Baby class half-penalty (0.5%) | MISSING (baby classes not implemented) | Low |
| PvP/WoE/town map penalty exemption | PARTIAL (PvP disabled, penalty only called from PvE paths) | Low |
| Post-respawn invulnerability (~5 seconds) | MISSING | Medium |
| Death fall animation (client) | UNKNOWN | Low |
| Corpse lying on ground state (client) | UNKNOWN | Low |

---

## Teleportation System

### Deep Research Spec (Docs 29, 32)
- Fly Wing (item 601): random teleport on same map, blocked by `noteleport`
- Butterfly Wing (item 602): return to save point, blocked by `noreturn`
- Teleport skill (Acolyte): Lv1 = random, Lv2 = save point + menu
- Warp Portal (Acolyte): ground-targeted portal, Blue Gem catalyst, 3 memo slots, 8 player capacity, 3 active max
- Kafra teleport: paid town-to-town
- Giant Fly Wing: party-wide random teleport
- Dead Branch / Bloody Branch: monster summoning (blocked by `nobranch`)

### Current Implementation
- **Fly Wing**: Implemented (line 22284) -- broadcasts `player:teleport` with type `fly_wing`, generates random coordinates
- **Butterfly Wing**: Implemented (line 22337-22369) -- queries save point from DB, broadcasts `player:left` to old zone, emits `player:teleport` with type `butterfly_wing`, handles cross-zone transition
- **Warp Portal skill**: Fully implemented (lines 11923-12044, 23517-23564) -- `_pendingWarpPortal` state, `warp_portal:select` destination popup, `warp_portal:confirm` handler, ground effect creation, 3 portal limit with oldest removal, Blue Gem consumption
- **Memo system**: `/memo1-3` chat commands implemented (line 8825-8857), DB-persisted `character_memo` table, Warp Portal level validation
- **Kafra teleport**: `kafra:teleport` handler (line 6584) with zeny cost, destination validation, zone transition
- **Zone warp portals**: `zone:warp` handler (line 6288) for NPC-style auto-warps between zones

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| `noteleport` flag enforcement on Fly Wing | MISSING (flag defined but not checked) | High |
| `noreturn` flag enforcement on Butterfly Wing | MISSING | High |
| `nomemo` flag enforcement on /memo | MISSING | Medium |
| Teleport skill (Acolyte Lv1/Lv2 via skill system) | PARTIAL (itemskill scrolls exist, skill handler unclear) | Medium |
| Giant Fly Wing (party-wide teleport) | MISSING | Low |
| Dead/Bloody Branch summoning | MISSING | Low |
| Portal player capacity (8 max per portal) | UNKNOWN (may not be enforced) | Low |
| Portal duration by skill level (10-25 seconds) | UNKNOWN | Low |
| Post-teleport invulnerability | MISSING | Low |

---

## Novice & Super Novice

### Deep Research Spec (Doc 35)
- Novice: 3 skills (Basic Skill Lv1-9, First Aid, Play Dead), restricted equipment, 0 death penalty, 48 starting stat points
- Basic Skill gates: Lv1 Trading, Lv3 Sitting, Lv5 Party Join, Lv6 Storage, Lv7 Party Create, Lv9 Job Change
- Super Novice: access to all 6 first-class skill trees, Guardian Angel death-save at 99-99.9% EXP, unique items
- Super Novice job change: Base Level 45+, Job Level 10, from Novice only, via Tzerero NPC in Al De Baran

### Current Implementation
- **Novice death penalty exemption**: Correctly implemented in `applyDeathPenalty()` (line 2079: `if (jobClass === 'novice') return;`)
- **Basic Skill Lv9 gate**: Checked in `job:change` handler (line 8022: `basicSkillLv < 9` validation)
- **Super Novice in class system**: Referenced in `JOB_EQUIP_NAMES` (line 4604-4624) for equipment access, and `CART_CLASSES` (line 6707)
- **Play Dead**: Implemented as `SC_TRICKDEAD` status (per MEMORY.md Monk audit -- sitting system prerequisite)
- **First Aid**: Implemented as skill ID 2/142

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Basic Skill level gates (Trading, Sitting, Storage, Party) | MISSING (only Lv9 job change gate exists) | Low |
| Super Novice as a playable class | PARTIAL (equip names defined, but no skill tree access to all 6 class trees) | Medium |
| Super Novice job change path (Base Level 45+ requirement) | MISSING | Medium |
| Guardian Angel death-save (99-99.9% EXP) | MISSING | Low |
| Super Novice-exclusive items (Angel set, Novice Potion class restriction) | MISSING | Low |
| Super Novice Steel Body at 99% EXP (Guardian Angel effect) | MISSING | Low |
| Starting equipment (Knife + Cotton Shirt for new characters) | UNKNOWN | Low |
| Training Grounds (tutorial area with free items) | MISSING | Low |

---

## Marriage & Adoption

### Deep Research Spec (Doc 39)
- Marriage: requires Base Level 45+, one male + one female, Wedding Dress/Tuxedo/Diamond Rings/Marriage Covenant (~40M zeny total)
- Ceremony: Prontera Church, multi-step NPC dialogue, both parties confirm
- Marriage skills: Loving Touch (HP share), Undying Love (SP share), Romantic Rendezvous (summon spouse, 15s cast)
- Wedding Rings: accessory items that enable marriage skills
- Divorce: Niflheim NPC, 2.5M zeny, destroys rings
- Adoption: married couple (Base 70+) adopts a Novice/1st class character -> Baby class
- Baby classes: 75% HP/SP, stat cap 80, no rebirth, smaller sprite
- Family skills: Call Parent, Call Baby, Happy Break (parent death penalty immunity)

### Current Implementation
- **Not implemented.** No marriage, adoption, baby class, wedding ring, or family skill code exists in the server.

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Marriage system (ceremony, rings, skills) | MISSING | Low |
| Divorce system | MISSING | Low |
| Adoption system (Baby classes) | MISSING | Low |
| Family skills (Call Parent/Baby, Happy Break) | MISSING | Low |
| Wedding Ring items | MISSING | Low |
| Baby class stat penalties (75% HP/SP, stat cap 80) | MISSING | Low |

---

## Transcendent & Rebirth

### Deep Research Spec (Doc 41)
- Rebirth: Base 99 + Job 50 second class -> High Novice (Level 1/1, 100 stat points)
- Quest: Juno Sage Academy -> Book of Ymir -> Valhalla -> Valkyrie, 1.285M zeny
- Transcendent bonuses: +25% MaxHP/MaxSP, +52 stat points, Job Level 70 cap (vs 50), exclusive skills
- 12 transcendent classes (Lord Knight, High Wizard, Sniper, High Priest, Assassin Cross, Whitesmith, Paladin, Scholar, Clown, Gypsy, Champion, Creator)
- ~60 transcendent-exclusive skills across all classes
- Transcendent EXP table: ~1.9x multiplier vs normal
- Transcendent-only equipment access

### Current Implementation
- **Not implemented.** No rebirth, transcendent class, High Novice, or transcendent skill code exists in the server.
- The `job:change` handler only supports Novice->1st class and 1st class->2nd class transitions
- Some transcendent class names appear in `JOB_EQUIP_NAMES` (line 7874: "Must be Knight or Crusader (or transcendent variants)" comment for /mount) but no actual transcendent logic exists

### Gaps
| Feature | Status | Priority |
|---------|--------|----------|
| Rebirth/transcendence system | MISSING | High |
| High Novice class (100 stat points, auto quest skills) | MISSING | High |
| +25% MaxHP/MaxSP multiplier for transcendent | MISSING | High |
| Job Level 70 cap for transcendent second classes | MISSING | High |
| 12 transcendent class definitions | MISSING | High |
| ~60 transcendent-exclusive skills | MISSING | High |
| Transcendent EXP table (1.9x) | MISSING | Medium |
| Rebirth quest (Juno/Valhalla/Valkyrie NPC flow) | MISSING | Medium |
| Transcendent-only equipment restrictions | MISSING | Medium |
| High First Class sprites (different color) | MISSING | Low |

---

## UI/UX Coverage

### Deep Research Spec (Doc 31)
- Basic Info Window (HP/SP/EXP/Weight/Zeny)
- Equipment Window (10 slots with character preview)
- Inventory Window (3 tabs: Item/Equip/Etc)
- Skill Tree Window (prerequisite lines, drag to hotbar)
- Status Window (stats + derived values)
- Chat Window (multi-channel, resize, scroll)
- Hotbar (F1-F9 + extended rows)
- Quest Window (active/inactive tabs)
- Party Window (HP bars, settings, context menu)
- Guild Window (6 tabs)
- Friend List Window
- Minimap + World Map
- Emote Window (Alt+L)
- Macro Window (Alt+M)

### Current Client Subsystems (33 files)
| Subsystem | Status | Notes |
|-----------|--------|-------|
| `BasicInfoSubsystem` | Implemented | HP/SP/EXP/Weight/Zeny display |
| `EquipmentSubsystem` | Implemented | All equip slots including ammo |
| `InventorySubsystem` | Implemented | Grid, tabs, drag-drop, stack merge |
| `SkillTreeSubsystem` | Implemented | Per-class trees, prereqs, level selection |
| `CombatStatsSubsystem` | Implemented | Status window with stats display |
| `ChatSubsystem` | Implemented | Global/party/whisper chat |
| `HotbarSubsystem` | Implemented | Skill/item assignment, level selection |
| `PartySubsystem` | Implemented | HP/SP bars, invite, settings, context menu |
| `BuffBarSubsystem` | Implemented | Active buffs display |
| `CastBarSubsystem` | Implemented | Cast progress bar |
| `DamageNumberSubsystem` | Implemented | Floating damage/heal/block numbers |
| `EnemySubsystem` | Implemented | Enemy registry, 5 events |
| `OtherPlayerSubsystem` | Implemented | Player registry, movement sync |
| `NameTagSubsystem` | Implemented | Player/NPC name tags |
| `WorldHealthBarSubsystem` | Implemented | Enemy/player health bars |
| `CameraSubsystem` | Implemented | Camera control |
| `PlayerInputSubsystem` | Implemented | Click-to-move, attack, interact |
| `LoginFlowSubsystem` | Implemented | Auth state machine |
| `ZoneTransitionSubsystem` | Implemented | Seamless zone changes |
| `PositionBroadcastSubsystem` | Implemented | 30Hz position updates |
| `MultiplayerEventSubsystem` | Implemented | Combat events dispatch |
| `CombatActionSubsystem` | Implemented | 10 combat event handlers |
| `KafraSubsystem` | Implemented | Kafra NPC interaction UI |
| `ShopSubsystem` | Implemented | NPC shop buy/sell UI |
| `CartSubsystem` | Implemented | Cart inventory management |
| `VendingSubsystem` | Implemented | Player shop setup/browse |
| `ItemInspectSubsystem` | Implemented | Item detail popup |
| `CraftingSubsystem` | Implemented | Pharmacy/Arrow Crafting UI |
| `SummonSubsystem` | Implemented | Summoned entity overlay |
| `TargetingSubsystem` | Implemented | Skill targeting system |
| `PetSubsystem` | Implemented | Pet management UI |
| `HomunculusSubsystem` | Implemented | Homunculus management UI |
| `CompanionVisualSubsystem` | Implemented | Pet/homunculus actor visuals |

### Missing UI Systems
| UI Element | Priority |
|------------|----------|
| Quest Window (Alt+U) | Medium |
| Guild Window (Alt+G, 6 tabs) | Medium |
| Friend List Window (Alt+H) | Medium |
| Emote Window (Alt+L) | Low |
| Macro Window (Alt+M) | Low |
| Minimap (Ctrl+Tab) | Medium |
| World Map (Ctrl+~) | Low |
| Storage Window (Kafra storage) | High |
| Stat/Skill Reset confirmation | Low |
| Chat Room creation/join | Low |
| Marriage ceremony UI | Low |
| Adoption confirmation dialog | Low |
| Death UI ("Return to Save Point" / "Wait here") | High |

---

## Critical Missing Systems

Ranked by gameplay impact and dependency chains:

### Tier 1: Core Gameplay Blockers
1. **Player Respawn Handler** -- `player:respawn` socket handler does not exist. Death penalty applies but there is no server-side flow to warp the player back to their save point. This is a critical gap that breaks the death->respawn loop.
2. **Map Flag Enforcement** -- `noteleport`, `noreturn`, `nomemo`, `nobranch` flags are defined in zone data but never checked by the Fly Wing, Butterfly Wing, Warp Portal, or /memo handlers. Any dungeon balance relying on movement restrictions is currently unenforced.
3. **Kafra Storage** -- Account-wide shared storage is a fundamental MMO feature. No `storage:open/deposit/withdraw` handlers exist.
4. **Quest Framework** -- No quest tracking, progress, or objective system exists. Blocks all narrative content, headgear quests, bounty boards, and access quests.

### Tier 2: Major Feature Gaps
5. **Transcendent/Rebirth System** -- Entire endgame progression tier is missing. 12 classes, 60+ skills, +25% HP/SP, Job Level 70 cap. Blocks access to the highest-tier content.
6. **Guild System** -- No guild creation, management, storage, skills, or WoE. Blocks guild chat, guild-based content, and social features.
7. **Death UI / Resurrection** -- Players cannot choose to wait for resurrection or self-rez with Token of Siegfried. Resurrection skill cannot target dead players.
8. **NPC Dialogue Framework** -- No scripted NPC conversations, branching dialogue, or multi-step interactions. All NPC interactions are single-action (shop/kafra).

### Tier 3: Important but Deferrable
9. **Friend List** -- No friend add/remove, online status, /hi command
10. **Emote System** -- 50+ emotes with no implementation
11. **Super Novice** -- Partially referenced but not playable
12. **World Scale** -- 7 of ~420 zones; need more towns, fields, and dungeons for content variety

### Tier 4: Low Priority / Endgame Content
13. **Marriage/Adoption** -- Niche social system
14. **PvP Arenas / WoE** -- PvP exists but disabled; no arena maps or WoE mechanics
15. **Instance Dungeons** -- Endless Tower, Memorial Dungeons
16. **Day/Night Cycle / Weather** -- Visual polish

---

## Recommended Implementation Order

### Phase 1: Core Loop Fixes (Immediate)
1. **Player Respawn Handler** -- Add `player:respawn` event that warps player to save point, restores HP/SP, broadcasts position
2. **Map Flag Enforcement** -- Check `noteleport` in Fly Wing handler, `noreturn` in Butterfly Wing handler, `nomemo` in /memo handler
3. **Death UI** -- Client death overlay with "Return to Save Point" button that emits `player:respawn`

### Phase 2: Storage & NPC Framework
4. **Kafra Storage** -- DB table, `storage:open/deposit/withdraw` handlers, client `StorageSubsystem`
5. **NPC Dialogue Framework** -- Server-side dialogue tree system, client popup for NPC conversations
6. **Additional NPC Shops** -- Per-town weapon/armor dealers with town-specific inventories

### Phase 3: Social Systems
7. **Guild System** -- Create, invite, ranks, guild chat ($ prefix), guild storage, emblem
8. **Friend List** -- Add/remove, online/offline status, /hi command, Alt+H window
9. **Emote System** -- 33+ emotes, slash commands, Alt+L window, zone broadcast

### Phase 4: Quest & Content
10. **Quest Framework** -- Quest DB table, progress tracking, NPC quest givers, quest window UI
11. **Job Change Quests** -- Narrative quest content for each 1st/2nd class change
12. **Zone Content** -- Add Geffen, Payon, Morroc, Alberta, Izlude towns + surrounding fields + dungeon floors

### Phase 5: Transcendence & Endgame
13. **Rebirth System** -- High Novice, High 1st Classes, Transcendent 2nd Classes
14. **Transcendent Bonuses** -- +25% HP/SP, 100 stat points, Job Level 70
15. **Transcendent Skills** -- 60+ class-specific exclusive skills
16. **Super Novice** -- Full all-tree skill access, Guardian Angel

### Phase 6: Niche Systems
17. **Marriage & Adoption** -- Ceremony, rings, skills, baby classes
18. **PvP Arenas & WoE** -- Arena maps, castle siege, Emperium
19. **Instance Dungeons** -- Endless Tower, private map copies
20. **Day/Night Cycle & Weather** -- Visual polish

---

*This audit covers 7 deep research documents totaling ~130,000 tokens of RO Classic reference material compared against ~30,000 lines of server code and 33 client subsystems.*
