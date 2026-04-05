# FINAL Consolidated Report -- Sabri_MMO vs RO Classic Pre-Renewal

**Date**: 2026-03-23
**Methodology**: Cross-referenced `00_Master_Coverage_Audit.md` (505 discrete features), `MASTER_Implementation_Gap_Analysis.md`, and all 14 individual audit reports (AUDIT_01 through AUDIT_14) against actual Sabri_MMO implementation.
**Codebase Analyzed**: `server/src/index.js` (32,566 lines) + 11 data modules (5,833 lines) + 33 C++ UWorldSubsystems + 20+ Slate widgets
**Deep Research Documents**: 41 files in `RagnaCloneDocs/Deep_Research/`

---

## 1. Project Health Dashboard

| Metric | Count | Percentage |
|--------|-------|------------|
| **Total RO Classic Features** | 505 | 100% |
| **Fully Implemented** | 216 | 42.8% |
| **Partially Implemented** | 95 | 18.8% |
| **Not Started** | 194 | 38.4% |

| Metric | Value |
|--------|-------|
| **Skill handlers (unique)** | 177 |
| **Skill handlers correct** | ~162 (92%) |
| **Skill handlers with issues** | ~15 (8%) |
| **Skill definitions in SKILL_MAP** | 293 (69 first-class + 224 second-class) |
| **Buff types implemented** | 71 of 73 (97.3%) |
| **Status effects implemented** | 12 of 14 core (85.7%) |
| **Cards implemented** | 538 at 100% coverage |
| **Monster templates** | 509 loaded, 46 spawn points active |
| **Items in database** | 6,169 |
| **Playable classes** | 20 (Novice + 6 first + 13 second) |
| **Zones implemented** | 7 of ~420 |
| **Client subsystems** | 33 UWorldSubsystems, 20+ Slate widgets |

### Formula Discrepancy Summary

| Severity | Count | Description |
|----------|-------|-------------|
| **Critical** | 9 | Wrong formulas actively producing incorrect gameplay |
| **Medium** | 18 | Partial implementations or minor formula mismatches |
| **Low** | 12 | Cosmetic, edge-case, or negligible impact |

### Strengths by Category

| Category | Coverage | Assessment |
|----------|----------|------------|
| Core Mechanics | 82% | Excellent |
| Combat Systems | 81% | Excellent |
| Class/Skill Systems | 50% | Strong (all 1st/2nd complete; gap is trans/extended) |
| Item Systems | 77% | Strong |
| Monster/Enemy Systems | 78% | Strong |
| Economy | 63% | Good (missing trade/storage) |
| Status Effects | 58% | Good |
| UI/UX Systems | 68% | Good |
| Miscellaneous | 38% | Moderate |
| Social Systems | 31% | Weak (no guild/trade/friends) |
| Audio/Visual | 13% | Weak |
| World/Navigation | 13% | Weak (content volume problem) |

---

## 2. CRITICAL FIXES (must fix -- wrong behavior in existing code)

These are bugs in code that IS implemented but produces incorrect results. They affect every player in every session.

### C1. StatusATK Missing `floor(BaseLv/4)` Term
- **What's wrong**: Physical StatusATK calculation omits the base level contribution entirely
- **Where**: `server/src/ro_damage_formulas.js` line ~288-290
- **What it should be**: `floor(BaseLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)` (melee), equivalent for ranged with DEX primary
- **Impact**: At level 99, every physical attack is ~24 ATK too low. Affects ALL physical classes.
- **Effort**: 5 minutes. Add `Math.floor(level / 4)` to both melee and ranged StatusATK lines.
- **Source**: AUDIT_02 Step 4, AUDIT_01 C1-1

### C2. StatusATK Uses `floor(LUK/5)` Instead of `floor(LUK/3)`
- **What's wrong**: LUK contribution to StatusATK uses divisor 5 instead of 3
- **Where**: `server/src/ro_damage_formulas.js` line ~289-290
- **What it should be**: `Math.floor(luk / 3)` per rAthena pre-re and iRO Wiki Classic
- **Impact**: At 99 LUK, physical ATK is 14 points too low (gives 19 instead of 33). LUK builds are ~40% weaker than intended.
- **Effort**: 5 minutes. Change `/5` to `/3` in both melee and ranged StatusATK.
- **Source**: AUDIT_02 Step 4, AUDIT_01 A6-3

### C3. Critical Hits Do NOT Bypass DEF
- **What's wrong**: The `isCritical` flag never sets `skipDEF = true`. Crits go through full Hard DEF percentage reduction AND Soft DEF flat subtraction.
- **Where**: `server/src/ro_damage_formulas.js` line ~748-749 (only `ignoreDefense` option or card effects set `skipDEF`)
- **What it should be**: Critical hits bypass both Hard DEF and Soft DEF per rAthena pre-renewal
- **Impact**: Crit builds (Assassins, LUK Knights, crit Blacksmiths) deal dramatically less damage than intended. This is the single biggest damage formula error.
- **Effort**: 10 minutes. Add `if (isCritical) skipDEF = true` before DEF application.
- **Source**: AUDIT_02 Step 2

### C4. Hard DEF Uses Simplified Linear Formula
- **What's wrong**: Server uses a simplified linear reduction instead of the official rAthena formula `damage * (4000+DEF) / (4000+DEF*10)`
- **Where**: `server/src/ro_damage_formulas.js` Hard DEF section
- **What it should be**: `finalDamage = Math.floor(damage * (4000 + hardDEF) / (4000 + hardDEF * 10))`
- **Impact**: High DEF is too powerful. At 100 Hard DEF: linear gives ~50% reduction, correct formula gives ~29% reduction. Tanks take less damage than intended; high-DEF monsters are harder than intended.
- **Effort**: 15 minutes. Replace the linear formula with the rAthena formula.
- **Source**: AUDIT_02

### C5. ASPD Attack Delay Uses `*50` Instead of `*20`
- **What's wrong**: Attack delay formula multiplies by 50 instead of 20, making attacks 2.5x slower than RO Classic
- **Where**: `server/src/index.js` line ~460: `(200 - aspd) * 50`
- **What it should be**: `(200 - aspd) * 20` or verify against rAthena `amotion` values (delay in ms)
- **Impact**: All auto-attacks are 2.5x slower than they should be. Every physical class is affected. AGI builds feel unresponsive.
- **Effort**: 5 minutes. Change `* 50` to `* 20` (or calibrate against known RO ASPD values).
- **Source**: AUDIT_02, AUDIT_01 C5-1

### C6. Perfect Dodge Check Order Wrong + Applies to Skills
- **What's wrong**: PD is checked BEFORE crit (should be crit first, PD second per rAthena). Also, PD is not gated on `!isSkill` -- skills should bypass PD entirely.
- **Where**: `server/src/ro_damage_formulas.js` line ~481-492
- **What it should be**: Check order: Crit -> PD (auto-attack only) -> HIT/FLEE
- **Impact**: PD can dodge crits (should not), and skills can be Perfect Dodged (should not). Makes PD too strong and crits/skills too weak.
- **Effort**: 15 minutes. Reorder the check blocks and add `!isSkill` guard to PD check.
- **Source**: AUDIT_02 Step 1

### C7. Soft DEF Formula Is Wrong
- **What's wrong**: Uses `VIT/2 + AGI/5 + BaseLv/2` but rAthena pre-re uses `VIT*0.5 + max(VIT*0.3, VIT^2/150 - 1)` for the VIT component with a random range per hit
- **Where**: `server/src/ro_damage_formulas.js` line ~319
- **What it should be**: `softDEF = floor(VIT*0.5 + rnd(0, max(floor(VIT*0.3), floor(VIT^2/150) - 1)))` per rAthena
- **Impact**: VIT builds get far less DEF than intended at high VIT. A 99 VIT character gets ~49 softDEF instead of ~49 + rnd(0,64). The random component alone can be worth up to 64 DEF per hit.
- **Effort**: 20 minutes. Replace softDEF formula with rAthena version including random component.
- **Source**: AUDIT_02, AUDIT_01 C3

### C8. FLEE Multi-Attacker Penalty Starts Too Late
- **What's wrong**: Penalty starts at 3+ attackers (`numAttackers - 2`) instead of 2+ attackers (`numAttackers - 1`) per rAthena
- **Where**: `server/src/ro_damage_formulas.js` line ~398
- **What it should be**: `fleePenalty = Math.max(0, numAttackers - 1) * 10` (penalty at 2+ attackers)
- **Impact**: Players get a free pass against 2 monsters. AGI builds are slightly stronger than intended in multi-mob scenarios.
- **Effort**: 5 minutes. Change `- 2` to `- 1`.
- **Source**: AUDIT_02, AUDIT_01 C4-3

### C9. Back Stab Dagger Gives 2 Hits (Renewal-Only)
- **What's wrong**: Daggers deal 2 hits on Back Stab, but 2-hit dagger is Renewal-only. Pre-renewal: 1 hit for all weapons.
- **Where**: `server/src/index.js` line ~19525 (Back Stab handler)
- **What it should be**: 1 hit for all weapons in pre-renewal
- **Impact**: Rogue dagger Back Stab does double damage (340-700% x2 instead of x1). Rogue DPS is nearly 2x what it should be.
- **Effort**: 5 minutes. Remove the `dagger = 2 hits` special case.
- **Source**: AUDIT_07

---

## 3. MISSING SYSTEMS (not started at all)

Ranked by player impact and dependency blocking.

### Tier A: Core Infrastructure (must have for playable MMO)

| # | System | Est. Days | Dependencies | Player Impact |
|---|--------|-----------|-------------|---------------|
| 1 | **Player-to-Player Trading** | 2-3 | None | Critical -- primary item exchange mechanism in RO |
| 2 | **Kafra Storage** | 2-3 | None | Critical -- 600-slot item bank per account, essential for item management |
| 3 | **Guild System** | 8-12 | Party (done) | Critical -- guilds are the core social structure |
| 4 | **Friend List** | 2-3 | None | High -- online status, quick whisper |
| 5 | **Emote System** | 1-2 | None | Medium -- 71+ emotes, social expression |

### Tier B: Competitive Endgame

| # | System | Est. Days | Dependencies | Player Impact |
|---|--------|-----------|-------------|---------------|
| 6 | **War of Emperium** | 10-15 | Guild system | Critical for endgame -- castle sieges, treasure boxes, guardians |
| 7 | **PvP Arenas** | 2-3 | PvP framework (partial) | High -- structured PvP zones and rankings |
| 8 | **Duel System** | 1-2 | PvP damage (done) | Medium -- 1v1 challenges outside PvP zones |
| 9 | **Battlegrounds** | 5-7 | PvP system | Medium -- team PvP instances |

### Tier C: Character Progression

| # | System | Est. Days | Dependencies | Player Impact |
|---|--------|-----------|-------------|---------------|
| 10 | **Transcendent/Rebirth** | 10-15 | All 2nd classes (done) | High -- +25% HP/SP, Job Lv 70, ~65 new skills |
| 11 | **Super Novice** | 3-5 | None | Medium -- unique class with all 1st-class skills |
| 12 | **Taekwon / Star Gladiator / Soul Linker** | 8-12 | None | Medium -- ~60 skills, Feeling/Hatred systems |
| 13 | **Gunslinger** | 4-5 | None | Low -- Episode 11.3 class |
| 14 | **Ninja** | 4-5 | None | Low -- Episode 11.3 class |
| 15 | **Baby Classes** | 3-5 | Marriage system | Low -- halved stats, adoption |

### Tier D: World Content

| # | System | Est. Days | Dependencies | Player Impact |
|---|--------|-----------|-------------|---------------|
| 16 | **Zone/Map Expansion** (towns) | 20-40 | None | Critical -- only 4 of ~270 maps exist |
| 17 | **Dungeon Expansion** | 15-25 | Zones | High -- only 1 dungeon floor exists |
| 18 | **Field Map Expansion** | 10-15 | Zones | High -- only 2 fields exist |

### Tier E: Social & Polish

| # | System | Est. Days | Dependencies | Player Impact |
|---|--------|-----------|-------------|---------------|
| 19 | **Chat Rooms** | 2-3 | Chat (done) | Low -- player-created chat bubbles |
| 20 | **Marriage & Adoption** | 3-5 | None | Low -- ceremony, couple skills, baby classes |
| 21 | **Mail/RODEX System** | 3-5 | None | Low -- mail with attachments |
| 22 | **Mercenary System** | 3-5 | None | Low -- NPC-hired combat companions |
| 23 | **Equipment Set Bonuses** | 2-3 | None | Medium -- multi-piece equipment combos |
| 24 | **Dead Branch / Bloody Branch** | 1 | Monster spawn (done) | Low -- item-based monster summoning |
| 25 | **Additional Status Effects** | 2-3 | None | Low -- Confusion, Chaos, Hallucination, Fear, Burning |
| 26 | **Comprehensive /commands** | 1-2 | None | Low -- /where, /who, /noctrl, /noshift, /snap, etc. |
| 27 | **Settings/Options Window** | 1-2 | None | Low -- graphics, sound, controls |

---

## 4. PARTIAL SYSTEMS (started but incomplete)

### 4.1 Party System (67% complete -- 22/33 features)

**What works**: Creation, invite/accept/reject, kick/leave, leader transfer, dissolution, DB persistence, offline tracking, Even Share EXP (+20%/member, +25%/attacker tap), 15-level range, same-map filter, party chat, HP/SP sync, dead member exclusion, Devotion party check, Lullaby party immunity.

**What's missing**:
- Basic Skill Lv5 gate for joining (only creation gate exists at Lv7)
- Same-account block on party join
- Item distribution runtime logic (fields stored but unused)
- Item share mode immutability
- Party window skill targeting (click member to cast)
- Pink minimap dots for party members
- Member map name display in widget
- Dead member skull icon visual
- Tap bonus cap should be 580% not 375%
- +1 EXP floor per member in share formula

### 4.2 PvP System (11% complete -- 2/19 features)

**What works**: PvP zone flag defined, PvP damage reduction (60-70%), player-vs-player auto-attack/skill damage paths exist in combat tick, no EXP loss on PvP death.

**What's missing**:
- PvP globally disabled (`PVP_ENABLED = false`)
- No PvP arena zones or NPCs
- No rankings/leaderboard/kill-death tracking
- No PvP room types (Yoyo, Nightmare)
- No Yggdrasil restrictions in PvP
- No structured arena system

### 4.3 Transcendent Classes (partial framework only)

**What works**: Trans class names in `JOB_EQUIP_NAMES` and class lookup maps, mount type differentiation (Grand Peco for Paladin).

**What's missing**:
- Rebirth process (Valkyrie NPC, reset to High Novice)
- +25% HP/SP modifier
- Job Level 70 cap (instead of 50)
- ~65 transcendent-only skills (Lord Knight, High Wizard, Sniper, High Priest, Assassin Cross, Whitesmith, Paladin, Scholar, Clown, Gypsy, Stalker, Creator, Champion)
- Trans-class equipment restrictions

### 4.4 Quest System (framework only)

**What works**: Quest database schema (objectives, rewards), state machine, objective types (kill/collect/talk/reach/item), NPC dialogue trees, conditional branching.

**What's missing**:
- Quest log UI
- Quest markers above NPCs (! and ?)
- Job change quests (actual dialogue trees)
- Dungeon access quests
- Daily quests, turn-in quests, bounty boards
- Stat/skill reset NPC quests

### 4.5 Monster Skill Database (27/509 monsters configured)

**What works**: Full monster skill infrastructure (`ro_monster_skills.js`), 7 execution functions, AI IDLE+ATTACK hooks, slave spawning, metamorphosis, 40+ NPC_ skill types.

**What's missing**:
- Only 27 of 509 monster templates have skill entries
- Missing Follow/Angry AI sub-states (code 04)
- `MD_CASTSENSORCHASE` parsed but not used in CHASE state
- `MD_NOCAST` parsed but never checked
- `MD_IGNOREMELEE/MAGIC/RANGED/MISC` not implemented
- Looter behavior parsed but no runtime item pickup

### 4.6 Audio/Visual (13% complete)

**What works**: 97+ skill VFX configs in SkillVFXSubsystem, skill icon art.

**What's missing**:
- Character models per class/gender (single model for all)
- Monster models/sprites (placeholder visuals)
- Equipment visual changes on character
- Headgear visuals
- Refine glow effects (+7/+8/+9/+10)
- BGM per zone (system capable but only partial coverage)
- SFX per skill/action (partial)
- Day/night cycle
- Weather effects

### 4.7 Card System (92% complete -- missing combos and skill grants)

**What works**: 538 cards, compound UI, 65+ bonus types, full damage pipeline integration, naming prefix/suffix data.

**What's missing**:
- Card combo set bonuses (Mage set, Monk set, Thief set, etc.)
- Skill-granting cards (Smokie->Hiding Lv1, Creamy->Teleport Lv1, Horong->Sight Lv1, Poporing->Detoxify Lv1)
- Socket Enchant NPC (add slots to 0-slot equipment)

### 4.8 Map Flag Enforcement

**What works**: Flags defined per zone in `ro_zone_data.js` (`noteleport`, `noreturn`, `nosave`, `pvp`, `town`, `indoor`).

**What's missing**:
- Only `nosave` is actively checked server-side
- `noteleport` not enforced in Fly Wing/Butterfly Wing handlers
- `noreturn` not enforced
- `nomemo`, `nowarp`, `nowarpto` not enforced
- `noicewall` not defined
- Indoor flag effects (mount rules, weather suppression) not enforced

### 4.9 Forging System (70% complete)

**What works**: `forge:request` socket handler, recipes, element stones, star crumbs, DB columns for forged attributes.

**What's missing**:
- Very few forge recipes defined (most RO weapons not in recipe list)
- Formula may have DEX/LUK contribution bug per AUDIT_09

### 4.10 NPC System (limited variety)

**What works**: 5 NPC shops (Tool, Weapon, Armor, General, Arrow), Kafra (save/teleport), click-to-interact, dialogue trees, conditional branching.

**What's missing**:
- Kafra storage tab
- Refine NPC click flow (socket handler works but no NPC actor)
- Stylist NPC
- Stat/skill reset NPCs
- Pet food NPCs (per-town)
- Per-town shop variants
- Guild-specific NPCs
- Quest givers with narrative dialogue

---

## 5. IMPLEMENTATION ROADMAP

### Phase 1: Formula Fixes (1-2 days, highest impact)

Fix all 9 critical formula discrepancies. These affect every player in every session and require minimal code changes.

| Fix | File | Change | Time |
|-----|------|--------|------|
| C1: Add `floor(BaseLv/4)` to StatusATK | `ro_damage_formulas.js:288-290` | Add `Math.floor(level/4)` | 5 min |
| C2: LUK/5 -> LUK/3 | `ro_damage_formulas.js:289-290` | Change `/5` to `/3` | 5 min |
| C3: Crit bypass DEF | `ro_damage_formulas.js:~750` | Add `if (isCritical) skipDEF = true` | 10 min |
| C4: Hard DEF formula | `ro_damage_formulas.js` | Replace with `(4000+DEF)/(4000+DEF*10)` | 15 min |
| C5: ASPD delay *50 -> *20 | `index.js:~460` | Change `*50` to `*20` | 5 min |
| C6: PD check order + skill gate | `ro_damage_formulas.js:~481-492` | Reorder checks, add `!isSkill` | 15 min |
| C7: Soft DEF formula | `ro_damage_formulas.js:~319` | Replace with rAthena random VIT formula | 20 min |
| C8: Flee penalty threshold | `ro_damage_formulas.js:~398` | Change `-2` to `-1` | 5 min |
| C9: Back Stab dagger 2-hit | `index.js:~19525` | Remove dagger 2-hit special case | 5 min |

**Also fix medium-priority issues from audits**:
- Energy Coat cast time 5000 -> 0 (AUDIT_04)
- Poison React envenom counter `floor(Lv/2)` -> `floor((Lv+1)/2)` (AUDIT_07)
- Throw Venom Knife SP cost 15 -> 35 (AUDIT_07)
- Grimtooth melee/ranged classification by level (AUDIT_07)
- Scream (Dancer) missing party stun reduction /4 (AUDIT_06)
- Frost Joker party freeze chance /4 vs /2 reconciliation (AUDIT_06)
- Map flag enforcement for `noteleport` and `noreturn` (AUDIT_13)
- Novice no-death-penalty (AUDIT_01)

### Phase 2: Core Missing Systems (8-10 days)

| System | Days | Details |
|--------|------|---------|
| **Player-to-Player Trading** | 2-3 | `trade:request/offer/lock/confirm/cancel` handlers, `STradeWidget`, atomic transactions, anti-scam |
| **Kafra Storage** | 2-3 | `character_storage` DB table, `kafra:store/retrieve` handlers, `SStorageWidget`, 600 slots, 40z fee |
| **Friend List** | 2 | `friends` DB table, add/remove, online status, quick whisper |
| **Emote System** | 1-2 | 71+ emotes, `/emotion` commands, Alt+L menu, emote broadcast |

### Phase 3: Guild System + Basic PvP (10-15 days)

| System | Days | Details |
|--------|------|---------|
| **Guild Core** | 8-10 | Creation (Emperium + /guild), levels 1-50, EXP donation, 20 ranks, tax, 12 skills, storage (100-500 slots), emblem, alliances, enemies, dissolution, guild chat |
| **PvP Arenas** | 2-3 | Prontera PvP room NPC, arena zone, kill/death tracking, rankings |
| **Duel System** | 1-2 | `/duel` command, acceptance, area restriction |

### Phase 4: Content Expansion (15-25 days)

| System | Days | Details |
|--------|------|---------|
| **Geffen Zone** | 3-5 | Town + magic tower dungeon + surrounding fields, NPCs, spawns |
| **Payon Zone** | 3-5 | Town + cave dungeon + surrounding fields |
| **Alberta Zone** | 2-3 | Port town + merchant hub |
| **Morroc Zone** | 3-5 | Town + pyramids + desert fields |
| **Activate Zones 4-9** | 2-3 | Enable disabled spawn points with level-appropriate monsters |
| **Monster Skill Expansion** | 2-3 | Add skill entries for 50+ more monsters from the 509 templates |

### Phase 5: Competitive Content (15-20 days)

| System | Days | Details |
|--------|------|---------|
| **War of Emperium** | 10-15 | 5 castle zones, Emperium crystal, schedule system, guardians, treasure boxes, WoE rules |
| **Battlegrounds** | 5-7 | Tierra Valley / Flavius modes, badge rewards, queue system |

### Phase 6: Transcendence + Extended Classes (25-40 days)

| System | Days | Details |
|--------|------|---------|
| **Rebirth System** | 3-5 | Valkyrie NPC, High Novice, level/stat reset |
| **Transcendent Skills** | 8-12 | ~65 new skills across 13 trans classes |
| **Trans-Class Balance** | 1 | +25% HP/SP, Job Level 70, equipment restrictions |
| **Super Novice** | 3-5 | All 1st-class skills, unique death penalty, guardian angel |
| **Taekwon/SG/SL** | 8-12 | ~60 skills, Feeling/Hatred, Spirit Links |
| **Gunslinger** | 4-5 | 5 gun types, Coin system, ~25 skills |
| **Ninja** | 4-5 | Ninjutsu, Kunai, shadow skills, ~20 skills |

### Phase 7: Polish & Completeness (15-25 days)

- Dead Branch / Bloody Branch (1 day)
- Equipment set bonuses (2-3 days)
- Additional status effects (2-3 days)
- Comprehensive slash commands (1-2 days)
- Mercenary system (3-5 days)
- Mail system (3-5 days)
- Settings/options window (1-2 days)
- Minimap + world map (2-3 days)
- Quest log UI + quest markers (2-3 days)
- Audio expansion (BGM per zone, SFX per skill) (3-5 days)
- Marriage & adoption (3-5 days)

**Total estimated remaining work**: ~130-225 days across all phases

---

## 6. COMPLETE FEATURE CHECKLIST

### 6.1 Core Mechanics (28 features)

- [x] 6 base stats + allocation formula (cost = floor((x-1)/10)+2)
- [x] Stat points per level (floor(level/5)+3)
- [x] Stat bonuses every 10 (floor(STR/10)^2 etc.)
- [x] Base level 1-99 with EXP tables
- [x] Job levels (Novice 1-10, 1st 1-50, 2nd 1-50)
- [x] Skill points per job level (1 per level, 49 total)
- [x] Death EXP penalty (1% base, 0% job)
- [x] Novice -> 1st class (Job Lv 10)
- [x] 1st -> 2nd class (Job Lv 40+)
- [x] Weapon class restrictions (equip_jobs bitmask)
- [x] Grid movement (1 RO cell = 50 UE units, 150ms/cell)
- [x] Speed modifiers (IncAGI, Peco +36%, potions, Pushcart)
- [x] 8-direction movement (Chebyshev distance)
- [x] Sitting mechanic (Insert, 2x regen, blocks actions)
- [x] Pushcart speed penalty (Lv1 -50% through Lv5 -5%)
- [x] Walking cancels casting
- [x] Integer-only math throughout
- [ ] ~Class-specific HP/SP formulas (per-class modifiers present but incomplete tables)
- [ ] ~Class-specific ASPD tables (representative BTBA values, not all combos)
- [ ] ~Transcendent bonus stat points (names in lookup maps, no rebirth system)
- [ ] Novice no-death-penalty (0 EXP loss not enforced)
- [ ] Super Novice mechanics

### 6.2 Combat Systems (62 features)

- [x] StatusATK formula (melee vs ranged) -- **HAS BUGS C1+C2**
- [x] WeaponATK and variance per weapon level
- [x] Size penalty table (17 weapon types x 3 sizes)
- [x] Element modifier table (10x10x4, 400 values verified)
- [x] Refinement ATK bonus (+2/+3/+5/+7 per weapon level)
- [x] Over-upgrade random bonus
- [x] Card/equipment bonuses (race/ele/size, multiplicative stacking)
- [x] Mastery ATK (post-modifier flat addition)
- [x] Buff ATK (Impositio, Provoke, etc.)
- [x] Complete 16-step physical damage pipeline -- **HAS BUGS C3+C4+C7**
- [x] StatusMATK formula (INT + floor(INT/7)^2)
- [x] MATK min/max range
- [x] Weapon MATK (staff bonus + refinement)
- [x] Skill multipliers per spell
- [x] Complete 9-step magical damage pipeline
- [x] Bolt multi-hit (N = skill level, independent rolls)
- [x] Hard DEF (percentage) -- **HAS BUG C4**
- [x] Soft DEF (VIT flat) -- **HAS BUG C7**
- [x] Hard MDEF (equipment percentage)
- [x] Soft MDEF (INT-based)
- [x] Armor refinement DEF bonus (floor((3+refine)/4))
- [x] DEF bypass mechanics (crit should bypass -- **BUG C3**)
- [x] Multi-monster DEF penalty (-5% per attacker >2)
- [x] HIT formula (175 + BaseLv + DEX + bonus)
- [x] FLEE formula (100 + BaseLv + AGI + bonus) -- **HAS BUG C8**
- [x] Hit rate (80 + HIT - FLEE, clamped 5-95%)
- [x] Multi-monster FLEE penalty -- **HAS BUG C8**
- [x] Perfect Dodge (1 + floor(LUK/10) + bonus) -- **HAS BUG C6**
- [x] Critical rate (floor(LUK*0.3)+1+bonus)
- [x] Crit shield (floor(targetLUK/5))
- [x] Critical damage (1.4x, max ATK) -- **MISSING DEF bypass C3**
- [x] Pre-renewal ASPD formula -- **HAS BUG C5**
- [x] Speed modifiers (potions, skills)
- [x] Auto-attack targeting (click-to-attack)
- [x] Attack delay from ASPD -- **HAS BUG C5**
- [x] Auto-attack element priority (Endow > Arrow > Weapon)
- [x] Cast time formula (1 - DEX/150)
- [x] After-cast delay (ACD) per skill
- [x] Skill interruption on damage (Endure/Phen prevent)
- [x] Target types (self/single/ground/aoe/passive)
- [x] Skill tree prerequisites
- [x] Lex Aeterna (double damage, consumed in 8 paths)
- [x] Dual wield system (Assassin, 8-phase complete)
- [x] Dual wield ASPD (0.7 * (BTBA_Main + BTBA_Off))
- [x] Ranged auto-attack with arrow consumption
- [x] Skill level selection (per-hotbar-slot, DB-persisted)
- [x] Ground-targeted skills
- [x] Self-centered AoE skills
- [ ] ~Weapon element endow (works; not all converter paths tested)
- [ ] ~Arrow ATK contribution (works; some edge cases)
- [ ] ~Katar double crit rate (display vs actual discrepancy)
- [ ] ~Skills cannot crit (rule enforced; exception list incomplete)
- [ ] ~Shield ASPD penalty (mentioned; not all combos verified)
- [ ] ~ASPD display (0-190 scale works; cap details sparse)
- [ ] ~BTBA per class/weapon (representative values; not every combo)
- [ ] ~Armor element (cards can change; not all paths verified)
- [ ] ~Gem/catalyst consumption (some skills; not all validated)
- [ ] ~Skill cooldowns vs global ACD (distinction exists; per-skill varies)
- [ ] ~Attack animation (basic; not per-weapon-type visual)
- [ ] Walk delay after skills (per-skill data not compiled)

### 6.3 Class/Skill Systems (32 major systems)

- [x] Novice (3/3 skills)
- [x] Swordsman (10/10 skills)
- [x] Mage (14/14 skills)
- [x] Archer (7/7 skills)
- [x] Thief (10/10 skills)
- [x] Merchant (10/10 skills)
- [x] Acolyte (15/15 skills)
- [x] Knight (10/11 skills -- OHQ deferred)
- [x] Wizard (14/14 skills)
- [x] Hunter (18/18 skills)
- [x] Priest (19/19 skills)
- [x] Assassin (12/12 skills)
- [x] Blacksmith (20/20 skills)
- [x] Crusader (14/14 skills)
- [x] Sage (22/22 skills)
- [x] Bard (20/20 skills + ensembles)
- [x] Dancer (20/20 skills + ensembles)
- [x] Rogue (19/19 skills)
- [x] Alchemist (16/16 skills)
- [x] Monk (16/16 skills)
- [x] Ensemble skills (9/9 duets)
- [x] Performance system (movement, overlap, SP drain, cancel)
- [x] Trap system (10 types, placement, trigger, damage)
- [x] Spirit sphere system (counter resource + consumption)
- [x] Combo system (chain timing + skill whitelist)
- [x] Plagiarism (copy, whitelist, DB persist)
- [x] Forging/crafting (recipes, success formula, element stones)
- [x] Pharmacy/brewing (recipes, success formula)
- [ ] ~Quest skills (some quests listed; NPC gates partial)
- [ ] ~Transcendent classes (names only; no skills, no rebirth)
- [ ] Super Novice
- [ ] Taekwon Kid
- [ ] Star Gladiator
- [ ] Soul Linker
- [ ] Gunslinger
- [ ] Ninja
- [ ] Baby classes

### 6.4 Item Systems (48 features)

- [x] HP/SP potions (all tiers)
- [x] Status cure items (Green Potion, Panacea, Royal Jelly)
- [x] ASPD potions (3 tiers, class restrictions, mutual exclusion)
- [x] Stat food (+1 to +10, 60 items)
- [x] Teleport items (Fly Wing, Butterfly Wing)
- [x] Gem/catalyst items (consumed by skills)
- [x] Elemental converters (4 endows)
- [x] Scroll items (itemskill type)
- [x] Equipment (10 slots, drag-drop)
- [x] Headgear combo positions (all 6)
- [x] Two-handed weapon shield lock
- [x] Class restrictions (bitmask)
- [x] Level restrictions
- [x] 17 weapon types
- [x] Weapon levels (1-4) with variance
- [x] Size modifier table (17x3)
- [x] Refinement levels (+0 to +10)
- [x] Safety limits per weapon level
- [x] Success rates beyond safety
- [x] Failure = destruction (cards lost)
- [x] Refinement materials (ores)
- [x] Card compounding (server-validated)
- [x] Card slot types (7 categories)
- [x] Card effect types (8 types)
- [x] Card removal (NPC only)
- [x] Weight formula (2000 + STR*30)
- [x] 50% overweight regen block
- [x] 90% overweight attack/skill block
- [x] Unidentified items + Magnifier
- [x] Ammunition system (arrows, element override, consumption)
- [ ] ~Card naming display (data exists; client display partial)
- [ ] ~Equipment visual on character (art pipeline dependency)
- [ ] ~Slotted vs unslotted variants (slots work; variant list not compiled)
- [ ] ~Weapon element per weapon (system works; not all weapons have data)
- [ ] ~Card stacking rules (basic rule works; exceptions not listed)
- [ ] ~MVP cards (drop system works; card list incomplete)
- [ ] ~Yggdrasil Berry/Seed (items exist; WoE restrictions not enforced)
- [ ] ~Box items (items exist; random loot tables not implemented)
- [ ] Equipment sets/combos (set bonuses)
- [ ] Dead Branch / Bloody Branch (item-based monster summoning)
- [ ] Named/unique weapons (special quest/MVP weapons)

### 6.5 Social Systems (45 features)

- [x] Party creation (/organize)
- [x] Party invite/accept/reject
- [x] EXP distribution (Each Take / Even Share)
- [x] Level restriction for Even Share (15 levels)
- [x] Same-map requirement for shared EXP
- [x] Party EXP bonus (+20%/member)
- [x] Party leader mechanics (transfer, kick, disband)
- [x] Party HP/SP display
- [x] Party chat (/p, % prefix)
- [x] Global/shout chat
- [x] Zone chat
- [x] Whisper/PM (/w charname)
- [x] Normal/local chat
- [x] Basic Skill Lv6 party requirement
- [ ] ~Item distribution modes (stored but not all verified)
- [ ] ~Guild chat (channel exists but commented out)
- [ ] ~Block/ignore system (server-side; UI not complete)
- [ ] ~Chat commands (some: /w, /p, /memo)
- [ ] ~Chat log filtering (3 tabs)
- [ ] Party minimap dots
- [ ] Guild system (entire -- creation, ranks, skills, storage, EXP, emblems)
- [ ] Guild levels/capacity/Extension
- [ ] Guild tax system
- [ ] Guild skills (12 total)
- [ ] Guild alliances/enemies
- [ ] Guild storage
- [ ] Guild emblem
- [ ] Guild dissolution
- [ ] Guild notice/announcement
- [ ] Friend list system
- [ ] Marriage ceremony
- [ ] Wedding Ring (summon)
- [ ] Marriage skills
- [ ] Divorce mechanic
- [ ] Adoption system (baby classes)
- [ ] Emote/emotion system (71+ emotes)
- [ ] Chat rooms (Alt+C)
- [ ] Duel system
- [ ] Player-to-player trading
- [ ] /ex and /exall commands
- [ ] Jawaii honeymoon island

### 6.6 World/Navigation (55 features)

- [x] Prontera (town zone with NPCs, shops, Kafra)
- [x] Prontera South (field with spawns)
- [x] Prontera North (field with spawns)
- [x] Prontera Dungeon Floor 1
- [x] Warp portals (WarpPortal overlap trigger actors)
- [x] Kafra teleport service (town-to-town with zeny cost)
- [x] Map flags system (noteleport, noreturn, nosave, pvp, etc.)
- [ ] ~Zone-scoped broadcasting (works for 4 zones; not tested at scale)
- [ ] ~Lazy enemy spawning per zone (works; zones 4-9 disabled)
- [ ] ~Navigation (NavMesh exists; no NPC patrol routes)
- [ ] ~Minimap (described; not implemented)
- [ ] ~World map (described; not implemented)
- [ ] All other towns (21 remaining)
- [ ] All field maps (~120 remaining)
- [ ] All dungeons (~80 dungeon floors remaining)
- [ ] PvP/WoE maps (~30)
- [ ] Inter-continental airships
- [ ] Ship travel to islands
- [ ] Cell walkability (using NavMesh instead of RO-style cell grid)
- [ ] noicewall map flag
- [ ] nomemo / nowarp / nowarpto enforcement

### 6.7 Economy (24 features)

- [x] Zeny (sole currency, max 2^31-1, overflow protection)
- [x] Zeny sources (NPC sales, drops, vending)
- [x] Zeny sinks (shop, Kafra, skills, refine)
- [x] Buy/sell price formula (sell = floor(buy/2))
- [x] Discount skill (Merchant, up to 24%)
- [x] Overcharge skill (Merchant, up to 24%)
- [x] Tool Dealers (13 items)
- [x] Weapon Dealers (15 items)
- [x] Armor Dealers (14 items)
- [x] Arrow Dealers (35 items)
- [x] Merchant Vending skill (max items = Vending Lv + 2)
- [x] Vendor sign above character
- [x] Browse/buy from vendor (dual-mode UI)
- [x] Vending tax (5% over 10M)
- [x] Cart system (100 slots, weight, speed penalty)
- [ ] Player-to-player trading
- [ ] Kafra storage (600 slots)
- [ ] Guild storage
- [ ] Mail/RODEX system
- [ ] Auction system
- [ ] ~Buying Store (described; not implemented)
- [ ] ~Pet NPCs (some; not complete)
- [ ] ~Refine NPC click flow (handler exists; NPC click partial)
- [ ] ~Ore processing NPC (mentioned; no NPC)

### 6.8 Monster/Enemy Systems (36 features)

- [x] AI type codes (01-27) with hex bitmasks
- [x] 18+ mode flag bitmask
- [x] State machine (IDLE/CHASE/ATTACK/DEAD)
- [x] Aggro mechanics (5-step setEnemyAggro)
- [x] Assist trigger (same-type, distance, conditions)
- [x] Target selection rules (8 situations)
- [x] Chase behavior (speed, range, leash)
- [x] Wander behavior (IDLE, 60% speed)
- [x] Cast sensor (detect casters, idle variant)
- [x] Boss protocol (immune, detector)
- [x] Random target selection (from inCombatWith)
- [x] Hit stun (damageMotion pause)
- [x] 509 monster templates (rAthena pre-re)
- [x] Monster elements (10x4)
- [x] Monster races (20 types)
- [x] Monster sizes (S/M/L)
- [x] Monster stats (HP, ATK, DEF, MDEF, etc.)
- [x] Monster skill database (27 monsters, 40+ NPC_ skills)
- [x] Skill conditions (HP%, target count, random)
- [x] NPC_ skill types (summonslave, metamorphosis)
- [x] Monster skill execution functions (7 functions)
- [x] Slave spawning (master/slave lifecycle)
- [x] Chance-based loot rolling
- [x] Guaranteed drops (100% rate)
- [x] Card drops (0.01% base, LUK modifier)
- [x] Spawn points per zone (46 active)
- [x] Respawn timers
- [x] MVP spawn timers
- [ ] ~MVP announcement (zone broadcast; not server-wide)
- [ ] ~MVP EXP distribution (most damage = MVP credit; partial)
- [ ] ~MVP tombstone (concept; not visual)
- [ ] ~Phase mechanics (HP threshold skills; per-boss data sparse)
- [ ] ~Drop rate server multiplier (mentioned; not runtime configurable)
- [x] Looter behavior flag parsed
- [ ] Dead Branch spawning
- [ ] Bloody Branch spawning
- [ ] Looter recovery (items held by looter not tracked)

### 6.9 Status Effect Systems (26 features)

- [x] Poison (DoT, -HP regen)
- [x] Stun (cannot act, VIT resistance)
- [x] Freeze (cannot act, Water 1)
- [x] Stone Curse (2-phase, Earth 1)
- [x] Sleep (cannot act, wake on damage)
- [x] Curse (LUK=0, -25% speed)
- [x] Silence (cannot use skills)
- [x] Blind (-25 HIT, -25% FLEE)
- [x] Bleeding (DoT, no natural regen)
- [x] Coma (HP=1, SP=1 instant)
- [x] Provoke (+ATK%, -DEF%)
- [x] Blessing (+STR/DEX/INT)
- [x] All 71/73 combat buffs (~95 distinct types)
- [x] checkDamageBreakStatuses() (breaks freeze/stone/sleep)
- [x] Boss immunity to all CC (MD_STATUSIMMUNE)
- [ ] ~VIT-based stun resistance (formula exists; thresholds not fully compiled)
- [ ] ~INT-based sleep/blind resistance (formula exists)
- [ ] ~LUK-based curse/stone resistance (formula exists)
- [ ] ~Status resistance per effect (per-status thresholds incomplete)
- [ ] Confusion (reversed controls)
- [ ] Chaos (random item drop)
- [ ] Hallucination (screen distortion)
- [ ] Fear (cannot act)
- [ ] Burning (fire DoT, speed reduction)
- [ ] Crystallization/Deep Freeze
- [ ] EDP (Enchant Deadly Poison -- trans-class buff)

### 6.10 UI/UX Systems (38 features)

- [x] Basic Info window (HP/SP/EXP bars, weight, zeny, draggable)
- [x] Status window (6 stats + derived + Advanced Stats panel)
- [x] Equipment window (10 slots, drag-drop, ammo slot)
- [x] Inventory window (tabs: Item/Equip/Etc, drag-drop)
- [x] Skill tree window (prerequisites, + buttons, level selection)
- [x] Hotbar/shortcut bar (4 rows, keybinds, skill level per slot)
- [x] Chat window (3 tabs, combat log, whisper)
- [x] Damage numbers (miss, crit, heal, skill, block)
- [x] Cast bar (world-projected progress bar)
- [x] HP/SP bars above characters
- [x] Name tags (guild name, title, vendor sign)
- [x] Death overlay (respawn prompt)
- [x] Loading screen (zone transition)
- [x] Party window (member list, HP/SP bars, context menu)
- [x] Shop window (buy/sell NPC, batch operations)
- [x] Kafra window (save/teleport -- no storage tab)
- [x] Vending setup window (cart items, price entry, shop title)
- [x] Vending browse window (dual-mode, buyer + vendor self-view)
- [x] Item tooltip (name, type, weight, description, stats)
- [x] Click-to-target (enemy/NPC/player, cursor change)
- [x] Target info display (name + HP bar)
- [x] Auto-attack on target click
- [x] Cursor types (normal/attack/talk/pickup)
- [x] Cart inventory window (10-col grid, weight bar, F10)
- [x] Item Appraisal popup
- [x] Crafting popup (Pharmacy/Arrow Crafting)
- [ ] ~Minimap (described; not implemented)
- [ ] ~World map (described; not implemented)
- [ ] ~Equipment comparison tooltip (mentioned; not fully implemented)
- [ ] ~Item rarity colors (white/green/blue; not all tiers)
- [ ] ~Right-click context menus (partial)
- [ ] ~Refine window (handler exists; NPC click flow partial)
- [ ] ~Confirmation dialogs (some: delete char, drop item)
- [ ] Guild window
- [ ] Quest log
- [ ] Friends list window
- [ ] Configuration/options window
- [ ] Macro window (Alt+M)

### 6.11 Audio/Visual Systems (16 features)

- [x] Skill VFX (97+ configs in SkillVFXSubsystem)
- [x] Skill icon art
- [ ] ~BGM per zone (system capable; only 4 zones have BGM)
- [ ] ~SFX per skill/action (categories identified; partial)
- [ ] ~Attack hit sounds (per-weapon partial)
- [ ] ~Monster death sounds (some monsters)
- [ ] ~UI sounds (some events)
- [ ] ~Hair style system (creation works; in-game visual partial)
- [ ] ~Hair color system (creation works; visual partial)
- [ ] Character models (per class, gender) -- single model for all
- [ ] Monster models/sprites -- placeholder visuals
- [ ] Equipment visual changes
- [ ] Headgear visuals
- [ ] Refine glow effects (+7/+8/+9/+10)
- [ ] Day/night cycle
- [ ] Weather effects (rain, snow)

### 6.12 Miscellaneous Systems (95 features)

- [x] Pet taming (taming items + success rate with HP factor)
- [x] Pet egg system (unique instances)
- [x] Hunger system (0-100, feeding, decay, starvation)
- [x] Intimacy system (0-1000, 5 tiers)
- [x] Pet stat bonuses (Cordial/Loyal)
- [x] Pet following AI (follow distance, teleport)
- [x] Pet commands (5 total)
- [x] 34 core pre-renewal pets
- [x] Overfeed penalty (3 overfeed = escape)
- [x] 4 homunculus types (stats, growth tables)
- [x] Homunculus creation (Embryo, Alchemist skill)
- [x] Homunculus feeding (hunger/intimacy)
- [x] Homunculus auto-attack (ASPD 130, combat tick)
- [x] Homunculus EXP sharing (10%)
- [x] Homunculus evolution (Stone of Sage + Loyal)
- [x] Homunculus skills (8 total, 2 per type)
- [x] Falcon rental (Hunter, from NPC)
- [x] Blitz Beat (auto/manual, correct formula)
- [x] Auto-Blitz chance (floor((jobLv+9)/10))
- [x] Detect (reveal hidden with Falcon)
- [x] Cart rental (Kafra, 600-1200z)
- [x] Cart inventory (100 slots)
- [x] Cart skills (Revolution, etc.)
- [x] Cart speed penalty (Pushcart Lv1-5)
- [x] Peco Peco mount (Knight/Crusader, +36%, /mount)
- [x] NPC dialogue trees (branching choices)
- [x] NPC sprites and facing
- [x] Conditional branching (level, class, items)
- [x] NPC actions (give/take, warp)
- [x] Refiner NPC (full mechanics via socket)
- [x] Quest database (objectives, rewards, JSON schema)
- [x] Quest states (not started/in progress/complete)
- [x] Objective types (kill/collect/talk/reach/item)
- [x] PvP maps (zone flags)
- [x] PvP damage rules (60-70% of PvE)
- [x] No EXP loss on PvP death
- [x] Homunculus persistence (full state to DB)
- [ ] ~Pet accessories (per-pet data; visuals not shown)
- [ ] ~Falcon exclusivity with Peco (not enforced on all paths)
- [ ] ~Cart weight limit (not enforced in all operations)
- [ ] ~Grand Peco (mount type differentiated; not visual)
- [ ] ~PvP rankings (tracking mentioned; no leaderboard)
- [ ] ~PvP arena (mentioned; no dedicated zones)
- [ ] ~Change Cart visual (skill exists; no visual change)
- [ ] ~Quest markers (! and ? above NPCs)
- [ ] ~Job change quests (locations listed; details sparse)
- [ ] ~Access quests (5 listed; not implemented)
- [ ] ~Stylist NPC (mentioned; no NPC)
- [ ] ~WoE castle zones (documented; no zones exist)
- [ ] ~WoE Emperium (documented; no implementation)
- [ ] ~WoE schedule (documented; no implementation)
- [ ] Mercenary system (NPC rental, 3 types x 10 grades)
- [ ] Mercenary skills
- [ ] Free-for-all PvP mode
- [ ] PvP room types (Yoyo, Nightmare)
- [ ] War of Emperium (full)
- [ ] WoE treasure boxes
- [ ] WoE Guardian NPCs
- [ ] WoE Guardian stones/barricades
- [ ] WoE SE (Second Edition)
- [ ] God items/divine equipment
- [ ] Battlegrounds
- [ ] BG rewards (badges, equipment)
- [ ] BG queue system
- [ ] Quest log UI
- [ ] Daily quests
- [ ] Turn-in quests (collection)
- [ ] /commands system (comprehensive)
- [ ] /where, /who, /noctrl, /noshift
- [ ] /effect, /mineffect, /bgm, /sound
- [ ] /snap (auto-target nearest)
- [ ] Screenshot system
- [ ] Character rename
- [ ] GM commands (admin tools)
- [ ] Anti-bot measures
- [ ] Client-side settings persistence

---

## Appendix A: Audit Summary Table

| Audit | Scope | Match Rate | Critical | Medium | Low |
|-------|-------|-----------|----------|--------|-----|
| AUDIT_01 | Stats, Leveling, Class | 52/93 (56%) | 3 (StatusATK, ASPD) | 17 partial | 24 missing |
| AUDIT_02 | Combat Damage Formulas | 38/53 (72%) | 9 (DEF, crit, ASPD, PD) | 9 partial | 6 missing |
| AUDIT_03 | Element, Size, Race | ~95% correct | 2 (MD_IGNORE flags) | 5 | 4 |
| AUDIT_04 | First Class Skills (66) | 59/66 (89%) | 0 | 2 (Energy Coat cast, Charge Attack) | 5 minor |
| AUDIT_05 | Knight/Crusader/Wizard/Sage (49) | 36/49 (73%) | 0 (all previously fixed) | 7 data, 5 handler | 1 deferred |
| AUDIT_06 | Hunter/Bard/Dancer/Priest/Monk (75) | 59/75 (79%) | 3 (Frost Joker, Asura, Ankle Snare) | 6 | 7 |
| AUDIT_07 | Assassin/Rogue/Blacksmith/Alchemist (44) | 40/44 (91%) | 6 (Back Stab, forging, poison) | 11 | 8 |
| AUDIT_08 | Monster AI, MVP | Strong alignment | 0 | 5 (missing flags, skill DB coverage) | 6 missing features |
| AUDIT_09 | Items, Cards, Crafting | ~90% overall | 1 (card combos) | 3 (skill-grant, socket enchant, forge) | 3 |
| AUDIT_10 | Buffs & Status Effects | 71/73 buffs (97%) | 0 | 2 formula | 2 missing buffs |
| AUDIT_11 | Party/Guild/WoE/PvP | Party 67%, rest 0% | 0 | 11 party gaps | Guild/WoE/PvP all missing |
| AUDIT_12 | Economy, Pets, Inventory | ~75% | 2 (trade, storage) | 4 | 6 |
| AUDIT_13 | World, NPCs, Chat, Misc | ~35-40% | 0 | Many missing systems | 40-50% absent |
| AUDIT_14 | Ground Effects, Special | Largely correct | 6 | 5 | 8 |

## Appendix B: Effort Estimate Summary

| Phase | Scope | Est. Days | Cumulative |
|-------|-------|-----------|------------|
| 1. Formula Fixes | 9 critical + medium bugs | 1-2 | 1-2 |
| 2. Core Missing | Trade, Storage, Friends, Emotes | 8-10 | 9-12 |
| 3. Guild + PvP | Guild system, PvP arenas, Duel | 10-15 | 19-27 |
| 4. Content Expansion | 4 new towns + fields + dungeons | 15-25 | 34-52 |
| 5. Competitive | WoE, Battlegrounds | 15-20 | 49-72 |
| 6. Trans + Extended | Rebirth, ~130 new skills, 6 new classes | 25-40 | 74-112 |
| 7. Polish | All remaining systems | 15-25 | 89-137 |
| **TOTAL** | **289 features remaining** | **~90-140 days** | |

---

**Document Version**: 1.0
**Generated**: 2026-03-23
**Source Documents**: `00_Master_Coverage_Audit.md`, `MASTER_Implementation_Gap_Analysis.md`, AUDIT_01 through AUDIT_14
**Total features tracked**: 505
**Total audit findings consolidated**: 39 critical, 70+ medium, 50+ low across 14 audits
