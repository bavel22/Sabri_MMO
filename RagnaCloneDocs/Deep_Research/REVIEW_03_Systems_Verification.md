# Second-Pass Review: Systems & Master Gap

**Date**: 2026-03-23
**Reviewer**: Claude Opus 4.6
**Method**: Cross-verified AUDIT_08 through AUDIT_14 (7 system-level audits) against MASTER_Implementation_Gap_Analysis.md and 00_Master_Coverage_Audit.md. Checked for omissions, contradictions, dependency chains, and coverage gaps.

---

## Issues Missing From Master Analysis

The master analysis captures most critical items but omits or understates the following issues found in the individual audits:

### From AUDIT_08 (Monster AI / MVP)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 1 | Chase state skill evaluation missing -- monsters cannot use skills while in CHASE state (e.g., Baphomet NPC_DARKBREATH) | MEDIUM | NO | Master lists "Monster skill database DONE" without noting chase-state gap |
| 2 | Cast sensor in CHASE state not active (castSensorChase flag parsed but unused) | LOW | NO | Omitted entirely |
| 3 | MD_NOCAST flag parsed but never checked -- monsters with this flag can still use skills | LOW | NO | Omitted entirely |
| 4 | Slave AI code not overridden to 24 -- slaves use their own template AI instead of passive slave AI | LOW | NO | Master lists "Slave spawning DONE" without noting behavioral discrepancy |
| 5 | Slaves do not follow master after spawn | MEDIUM | NO | Master lists slave lifecycle as DONE |
| 6 | Master teleport does not teleport slaves | LOW | NO | Omitted |
| 7 | NPC_CALLSLAVE (352) missing -- cannot recall slaves to master | HIGH | NO | Omitted entirely |
| 8 | NPC_POWERUP (195) missing -- ATK x3 self-buff used by Orc Hero, Eddga, Baphomet | HIGH | NO | Master lists "NPC_ skill types DONE" at 40+ skills but omits this critical missing skill |
| 9 | NPC_REBIRTH (207) missing -- monster respawn on death | HIGH | NO | Omitted |
| 10 | MVP winner determination uses damage_dealt only, not damage_dealt + damage_tanked | LOW | PARTIAL | Master notes "MVP EXP distribution PARTIAL" but does not mention the missing damage-tanked component |
| 11 | MVP tombstone has no cleanup timer (should remove 5s after respawn) | LOW | NO | Omitted |
| 12 | Dead/disconnected MVP winner does not fall back to next highest scorer | LOW | NO | Omitted |
| 13 | Per-template MVP respawn timers not used (generic 120-130 min override) | LOW | NO | Master says "MVP spawn timers DONE" which is inaccurate |
| 14 | 8 monster skill conditions not implemented (friendstatuson/off, slavele, masterhpltmaxrate, masterattacked, alchemist, groundattacked, damagedgt) | MEDIUM | NO | Master says "Skill conditions DONE" |
| 15 | MD_IGNOREMELEE/MAGIC/RANGED/MISC damage type immunity flags missing (6 flags not in constants) | MEDIUM | NO | Omitted |
| 16 | Only 27 of 509 monsters have skill entries (13% coverage) | HIGH | NO | Master says "Monster skill database DONE" -- misleading since the system works but data coverage is 13% |

### From AUDIT_09 (Items / Cards / Crafting)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 17 | Card combo set bonuses not implemented (Mage set, Monk set, etc.) | HIGH | YES | Listed as "Equipment sets/combos MISSING" |
| 18 | Skill-granting cards non-functional (Smokie/Creamy/Horong/Poporing) | MEDIUM | NO | Master lists card system as 92% complete without mentioning this |
| 19 | Forge weapon level penalty formula bug: `wLv * 1000` instead of `(wLv-1) * 1000` | HIGH | NO | Not mentioned in master. WLv2 forging is 1000 points harder than intended |
| 20 | Only 7 forge recipes of 38+ exist | HIGH | NO | Master says "Forging/crafting (Blacksmith) DONE" |
| 21 | No anvil type detection or bonus in forge formula | MEDIUM | NO | Omitted |
| 22 | No hammer requirement/consumption in forging | MEDIUM | NO | Omitted |
| 23 | Condensed potions (545/546/547) missing from Pharmacy | HIGH | NO | Master says "Pharmacy/brewing (Alchemist) DONE" |
| 24 | All pharmacy recipes use same guide book ID 7144 | MEDIUM | NO | Omitted |
| 25 | Overupgrade bonus placed post-DEF instead of pre-DEF | MEDIUM | NO | Master says "Over-upgrade random bonus DONE" but placement is wrong |
| 26 | Socket Enchant NPC (add slots to equipment) missing | MEDIUM | NO | Omitted |
| 27 | Blacksmith Weapon Refine skill (1218) not implemented | MEDIUM | NO | Master says "Blacksmith (20 skills) DONE" but this specific skill is missing |

### From AUDIT_10 (Buffs / Status Effects)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 28 | OPT1 mutual exclusion not implemented (Stun/Freeze/Stone/Sleep can stack) | MEDIUM | NO | Critical correctness issue omitted from master |
| 29 | Freeze duration missing +srcLUK extension and uses /200 instead of /100 for MDEF | MEDIUM | NO | Master says "Freeze DONE" |
| 30 | Poison HP drain floor should be 25% MaxHP, not 1 HP | LOW | NO | Omitted |
| 31 | Blind minimum duration should be 15s, not 1s | LOW | NO | Omitted |
| 32 | Confusion level difference is inverted vs rAthena | LOW | NO | Master lists "Confusion MISSING" which is correct for the status but the chance formula inversion is a separate issue |
| 33 | Curse duration uses LUK instead of VIT (split-stat behavior) | LOW | NO | Omitted |
| 34 | Poison duration LUK scaling is 10x weaker than rAthena | LOW | NO | Omitted |
| 35 | Chemical Protection buffs lack BUFF_TYPES entries (no displayName/abbrev for buff bar) | LOW | NO | Omitted |
| 36 | BUFFS_SURVIVE_DEATH incorrectly includes song/dance buffs and is missing `riding` | MEDIUM | NO | Omitted |
| 37 | UNDISPELLABLE sets are split into two copies (main Dispell + Abracadabra) that can drift | LOW | NO | Omitted |
| 38 | `magic_rod` missing from UNDISPELLABLE set | LOW | NO | Omitted |
| 39 | Deadly Poison status (SC_DPOISON) completely missing -- blocks Assassin Cross implementation | MEDIUM | YES | Master lists "EDP MISSING" under status effects |

### From AUDIT_11 (Party / Guild / WoE / PvP)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 40 | Party tap bonus cap 375% instead of 580% | LOW | NO | Omitted from master |
| 41 | +1 EXP floor per member missing | LOW | NO | Omitted |
| 42 | Item distribution logic not implemented (fields stored but unused) | MEDIUM | PARTIAL | Master says "Item distribution modes PARTIAL" |
| 43 | Player race/size/element classification missing for PvP | HIGH | NO | Master mentions PvP damage paths exist but does not flag the missing player classification |
| 44 | Guild chat placeholder exists (`// case 'GUILD':` at line 8918) but is commented out | LOW | PARTIAL | Master says "Guild chat PARTIAL" |

### From AUDIT_12 (Economy / Pets / Inventory)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 45 | Job class weight bonuses entirely missing (+0 to +1000 per class) | MEDIUM | NO | Master says "Weight formula DONE" but the class component is missing |
| 46 | Homunculus intimacy gain rates are 10x faster than RO Classic (integer vs decimal) | MEDIUM | NO | Master says "Homunculus feeding DONE" |
| 47 | Ground item drop/pickup system entirely missing | MEDIUM | NO | Loot goes directly to inventory, skipping ground items |
| 48 | Pet client-side visual actor missing | MEDIUM | NO | Master says "Pet following AI DONE" but this is server-only |
| 49 | Homunculus client actor and position broadcast missing | MEDIUM | NO | Master says homunculus system is implemented |
| 50 | Most homunculus skills are stubbed ("not yet implemented") | MEDIUM | PARTIAL | Master says "Homunculus skills (8 total) DONE" which contradicts audit finding |

### From AUDIT_13 (World / NPCs / Chat / Misc)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 51 | `player:respawn` socket handler missing -- death loop is broken | CRITICAL | NO | This is the single most critical omission. Death penalty applies but player cannot respawn server-side |
| 52 | Map flag enforcement missing (noteleport, noreturn, nomemo) | HIGH | PARTIAL | Master says "Map flags DONE" but enforcement is not implemented |
| 53 | Resurrection skill cannot target dead players | HIGH | NO | Master says "Priest (19 skills) DONE" but rez on dead players is missing |
| 54 | Yggdrasil Leaf cannot resurrect dead players | HIGH | NO | Omitted |
| 55 | NPC dialogue tree framework missing | HIGH | NO | All NPC interactions are single-action |
| 56 | Quest framework entirely missing | HIGH | YES | Master lists this correctly |
| 57 | Only 7 zones of ~420 exist (master says 4 -- discrepancy) | -- | DISCREPANCY | Audit 13 says 7, master says 4 |
| 58 | Emote system missing (50+ emotes) | MEDIUM | YES | Listed correctly |

### From AUDIT_14 (Ground Effects / Special)

| # | Issue | Audit Severity | In Master? | Notes |
|---|-------|---------------|------------|-------|
| 59 | Freezing Trap and Blast Mine should cost 2 trap items, not 1 | HIGH | NO | Omitted |
| 60 | Trap damage formulas differ significantly from rAthena (INT/100 vs INT/35, no BaseLv, no TrapResearch) | HIGH | NO | Master says "Trap system DONE" |
| 61 | Flasher AoE may be wrong (3x3 instead of 5x5 per iRO Wiki) | MEDIUM | NO | Omitted |
| 62 | Detect skill AoE too large (7x7 instead of 3x3) | MEDIUM | NO | Omitted |
| 63 | Performance SP drain rates differ for 3 skills (Bragi 5s vs 3s, Apple 6s vs 3s, PDFM 10s vs 3s) | MEDIUM | NO | Master says "Performance system DONE" |
| 64 | Dual ground effect systems (ro_ground_effects.js module unused, index.js inline system in production) | LOW | NO | Architectural issue omitted |
| 65 | Trap placement has no 2-cell minimum distance restriction | MEDIUM | NO | Omitted |
| 66 | Trap durability not implemented (cannot attack traps) | MEDIUM | NO | Omitted |
| 67 | Trap item not returned on expiry | LOW | NO | Omitted |
| 68 | Arrow Shower does not push traps | MEDIUM | NO | Core Hunter tactic missing |
| 69 | Mount state not persisted to DB | LOW | NO | Omitted |

**Total: 69 specific issues found in audits. Of these, only ~12 are reflected in the master analysis. 57 issues are missing or misrepresented.**

---

## Contradictions Between Audits

### C1: Zone Count Discrepancy
- **AUDIT_13** says "7 zones defined" (prontera, prontera_south, prontera_north, prt_dungeon_01, prt_dungeon_02, geffen, geffen_dungeon_01)
- **Master Analysis** says "4 zones" (prontera, prt_south, prt_north, prt_dungeon_01)
- **Resolution**: AUDIT_13 is likely more accurate since it directly inspected `ro_zone_data.js`. The master may have used an older count. Need to verify by reading the zone data file.

### C2: Novice Death Penalty
- **Master Analysis** (Core Mechanics table) says "Novice no-death-penalty MISSING -- Not enforced"
- **AUDIT_13** says "Novice death penalty exemption correctly implemented in `applyDeathPenalty()` (line 2079)"
- **Resolution**: AUDIT_13 is correct -- the code exists and works. Master is wrong. This should be moved to DONE.

### C3: Monster Skill Coverage Characterization
- **Master Analysis** says "Monster skill database DONE" and "NPC_ skill types DONE"
- **AUDIT_08** says "Only 27 of 509 monsters have skill entries (13% coverage)" and "approximately 50% of NPC_ skill types are implemented (30 of 60+)"
- **Resolution**: AUDIT_08 is more nuanced. The system is architecturally complete but data coverage is very low. "DONE" is misleading for 13% monster coverage. Should be PARTIAL.

### C4: Forge/Pharmacy "DONE" vs Audit Findings
- **Master Analysis** says "Forging/crafting (Blacksmith) DONE" and "Pharmacy/brewing (Alchemist) DONE"
- **AUDIT_09** says Forging is ~70% (formula bug + only 7 of 38+ recipes) and Pharmacy is ~75% (missing condensed potions)
- **Resolution**: Both should be PARTIAL, not DONE. The formula bug alone (forge penalty doubled) is HIGH severity.

### C5: Trap System "DONE" vs Audit Findings
- **Master Analysis** says "Trap system (Hunter) DONE"
- **AUDIT_14** finds 6 critical discrepancies (item costs, damage formulas, AoE sizes) and 8 missing features
- **Resolution**: Should be PARTIAL. Traps work but have formula and cost errors.

### C6: Homunculus Skills "DONE" vs Audit Findings
- **Master Analysis** says "Homunculus skills (8 total, 4 types) DONE"
- **AUDIT_12** says "Most homunculus skills are stubbed (return 'not yet implemented')"
- **Resolution**: Should be PARTIAL. Only Healing Hands, Moonlight, Caprice, and Urgent Escape have partial implementations.

### C7: Performance System "DONE" vs SP Drain Discrepancies
- **Master Analysis** says "Performance system DONE"
- **AUDIT_14** finds 4 SP drain rate discrepancies (Bragi, Apple, PDFM, Fortune's Kiss)
- **Resolution**: Functionally DONE but with formula inaccuracies. Should be noted.

### C8: Looter Behavior Contradiction
- **Master Analysis** (Monster/Enemy table, line 397) says "Looter behavior (pick up items) DONE"
- **AUDIT_08** says "Looter -- No ground item system exists. Parsed into modeFlags but never read at runtime."
- **AUDIT_12** says "Ground Items NOT IMPLEMENTED"
- **Resolution**: AUDIT_08 is correct. Looter flag is parsed but there is no ground item system for monsters to pick up. Should be NOT IMPLEMENTED (blocked on ground item system).

### C9: Vending Tax Status
- **Master Analysis** (Economy table) says "Vending tax (5% over 10M) DONE"
- **AUDIT_12** says "5% vending tax on items > 100M zeny MISSING"
- **Resolution**: Need to verify which is correct. The amounts differ too (10M in master vs 100M in audit). Both could be partially right depending on the specific implementation.

### C10: Quest System Characterization
- **Master Analysis** (Miscellaneous, line 535-537) lists "Quest database DONE", "Quest states DONE", "Objective types DONE"
- **AUDIT_13** says "No quest tracking system, quest log, or quest window exists"
- **Resolution**: The Master Analysis appears to have marked these based on `00_Master_Coverage_Audit.md` documentation coverage (research docs describe these systems) rather than actual implementation status. AUDIT_13's ground-truth check is correct -- no quest system is implemented.

### C11: NPC Dialogue Trees
- **Master Analysis** (Miscellaneous, line 530) says "NPC dialogue trees DONE"
- **AUDIT_13** says "No scripted NPC conversations, branching dialogue, or multi-step interactions"
- **Resolution**: Same issue as C10. Master confused documentation coverage with implementation status. Should be MISSING.

### C12: Sandman/Flasher AoE Internal Research Contradiction
- **Deep Research Doc 34** says Sandman is 3x3 and Flasher is 3x3
- **Deep Research Doc 40** says Sandman is 5x5 and Flasher is 5x5
- **iRO Wiki** says both are 5x5
- **Implementation**: Sandman=5x5 (correct per iRO), Flasher=3x3 (incorrect per iRO)
- **Resolution**: Both should be 5x5 per authoritative iRO Wiki source.

---

## Cross-System Dependencies (Blocking Chains)

### Chain 1: Guild -> WoE -> God Items
```
Guild System (0% implemented)
  -> War of Emperium (0% implemented) [BLOCKED]
    -> Castle Economy / Treasure Boxes [BLOCKED]
      -> God Item Materials [BLOCKED]
        -> God Item Crafting [BLOCKED]
  -> Guild Storage (0%) [BLOCKED]
  -> Guild Chat (commented out) [BLOCKED]
  -> Guild Dungeons (0%) [BLOCKED]
```
**Impact**: Entire endgame progression chain is blocked by guild system.

### Chain 2: Ground Items -> Looter Behavior -> Ground Drops
```
Ground Item Entity System (0% implemented)
  -> Monster Loot Drop to Ground [BLOCKED] (currently bypassed: items go to inventory)
  -> Looter Monster Behavior [BLOCKED] (flag parsed but no runtime behavior)
  -> Player Item Pickup from Ground [BLOCKED]
  -> Loot Protection / Ownership Priority [BLOCKED]
  -> Ground Item Visibility (see other players' drops) [BLOCKED]
```
**Impact**: The entire loot-on-ground system is bypassed. Current behavior (direct-to-inventory) works but differs from RO Classic.

### Chain 3: Player Classification -> PvP Card Bonuses
```
Player Race/Size/Element Classification (missing)
  -> Hydra Card bonus vs players (+20% Demi-Human) [INCORRECT]
  -> Thara Frog Card defense vs players (-30% Demi-Human) [INCORRECT]
  -> Weapon size penalty vs players (Medium) [INCORRECT]
  -> Ghostring Card armor element (Ghost) [INCORRECT in PvP context]
  -> All racial/elemental card effects in PvP [INCORRECT]
```
**Impact**: Enabling PvP without player classification would cause all race/size/element card bonuses to miscalculate.

### Chain 4: Respawn Handler -> Death/Rez Flow
```
player:respawn socket handler (MISSING)
  -> Return to save point after death [BROKEN]
  -> "Wait here" for resurrection [BLOCKED]
  -> Resurrection skill on dead players [BLOCKED]
  -> Yggdrasil Leaf on dead players [BLOCKED]
  -> Token of Siegfried self-rez [BLOCKED]
  -> Kaizel auto-rez buff [BLOCKED]
```
**Impact**: The entire death-to-respawn loop may be broken server-side. Death penalty applies but there is no confirmed `player:respawn` handler to warp the player back. This is the highest-priority blocking chain.

### Chain 5: Kafra Storage -> Economy Flow
```
Kafra Storage (0% implemented)
  -> Item banking (store excess items) [BLOCKED]
  -> Account-shared items between characters [BLOCKED]
  -> Overflow management for farmers/vendors [BLOCKED]
  -> Free Ticket for Kafra Storage consumable [BLOCKED]
```
**Impact**: Players have no way to store items across sessions beyond inventory (100 slots) and cart (100 slots).

### Chain 6: Quest Framework -> Content Systems
```
Quest Framework (0% implemented)
  -> Job change narrative quests [BLOCKED]
  -> Headgear crafting quests [BLOCKED]
  -> Dungeon access quests (Bio Lab, Thanatos, etc.) [BLOCKED]
  -> Bounty Board repeatable quests [BLOCKED]
  -> Quest skill NPC interactions [PARTIALLY WORKING via Phase J gates]
```
**Impact**: No narrative or progression quest content is possible.

### Chain 7: Trading -> Economy Completeness
```
Player-to-Player Trading (0% implemented)
  -> Direct item exchange between players [BLOCKED]
  -> Pre-vending price negotiation [BLOCKED]
  -> Zeny exchange for services [BLOCKED]
```
**Impact**: The only way to transfer items between players is via vending. No direct trade exists.

### Chain 8: Transcendent System -> Endgame
```
Rebirth/Transcendence (0% implemented)
  -> +25% MaxHP/MaxSP [BLOCKED]
  -> Job Level 70 cap (vs 50) [BLOCKED]
  -> 13 transcendent class definitions [BLOCKED]
    -> ~65 transcendent-exclusive skills [BLOCKED]
    -> Trans-only equipment restrictions [BLOCKED]
  -> Assassin Cross (EDP, Meteor Assault, Soul Destroyer) [BLOCKED]
  -> Lord Knight (Berserk, Spiral Pierce, Parrying) [BLOCKED]
  -> High Wizard (Ganbantein, Gravitation Field) [BLOCKED]
  -> (all 13 trans classes) [BLOCKED]
```
**Impact**: Character progression ceiling is base level 99 / job level 50. No endgame class advancement.

---

## Uncovered Features (in coverage audit but not checked by any audit)

The `00_Master_Coverage_Audit.md` covers 505 features across 12 categories. Audits 01-14 focused primarily on server implementation accuracy. The following features appear in the coverage audit but were NOT verified by any audit document:

### Completely Unaudited Systems

1. **Transcendent classes (Section 3.4)** -- 13 trans classes with ~65 skills. Coverage audit marks them PARTIAL (skill IDs listed in doc 03). No audit checked whether any trans-class infrastructure exists beyond name lookups.

2. **Extended classes (Section 3.5)** -- Super Novice, Taekwon Kid, Star Gladiator, Soul Linker, Gunslinger, Ninja. All marked MISSING in coverage audit. No audit verified this.

3. **Marriage/Adoption (Section 5.6-5.7)** -- All features marked MISSING. AUDIT_13 confirmed "not implemented" but did not deeply audit.

4. **Duel system (Section 5.9)** -- Marked MISSING. No audit checked.

5. **Mail/RODEX system (Section 7.7)** -- Marked PARTIAL in coverage. No audit verified.

6. **Auction system (Section 7.8)** -- Marked PARTIAL in coverage. No audit verified.

7. **Mercenary system (Section 12.6)** -- Marked PARTIAL in coverage (system described in docs). No audit checked implementation.

8. **Battlegrounds (not in coverage audit)** -- Mentioned in master analysis as MISSING. Not in any audit.

9. **Audio systems (Section 11.1)** -- BGM/SFX coverage marked PARTIAL. No audit checked actual audio implementation.

10. **Visual/Art systems (Section 11.2)** -- Character models, equipment visuals, hair system. All marked PARTIAL. No audit checked.

### Features in Coverage Audit Not Cross-Checked Against Implementation

11. **Tab targeting (Section 10.4)** -- Marked MISSING. Not checked by any audit.
12. **Ctrl+click force attack** -- Marked MISSING. Not checked.
13. **Chat rooms (Alt+C)** -- Marked MISSING. AUDIT_13 confirmed.
14. **Day/night cycle** -- Marked MISSING. No audit checked.
15. **Weather effects** -- Marked MISSING. No audit checked.
16. **Cloth dye/palette system** -- Marked PARTIAL. No audit checked.
17. **Dead Branch / Bloody Branch** -- Marked MISSING. Master analysis lists as MISSING. No audit verified.
18. **Cooking system** -- AUDIT_09 mentions it in passing ("Cooking system -- quest chain, cookbooks, 60 stat food recipes"). Not in coverage audit at all.

### Coverage Audit Categories That No Audit Addressed

- **Section 6 (World/Navigation)**: AUDIT_13 covered this partially (zone count, map flags, teleportation). No audit checked airship/ship routes, cell walkability, minimap, or world map.
- **Section 11 (Audio/Visual)**: No dedicated audio/visual audit was performed.
- **Section 12.7-12.8 (PvP/WoE details)**: AUDIT_11 covered the feature list but did not audit PvP damage multiplier values, PvP room type mechanics, or WoE combat modification constants.

---

## Corrected Priority Rankings

The master analysis prioritizes: Trading > Storage > PvP > Quests > Transcendent. Based on cross-system dependency analysis and audit findings, the corrected priority order is:

### Tier 0: CRITICAL FIX (blocks core gameplay loop)

1. **player:respawn handler** -- If truly missing, this breaks the death->respawn loop. Must verify and fix immediately. (From AUDIT_13, not in master)
2. **Map flag enforcement** -- noteleport/noreturn/nomemo flags are defined but not checked. Any dungeon that relies on no-teleport is circumventable. (From AUDIT_13, understated in master)

### Tier 1: Formula/Data Fixes (wrong behavior in implemented systems)

Priority is higher than new features because these are bugs in shipped code.

3. **Forge weapon level penalty bug** -- `wLv * 1000` should be `(wLv-1) * 1000`. Simple one-line fix. (AUDIT_09)
4. **OPT1 mutual exclusion** -- Stun/Freeze/Stone/Sleep can stack simultaneously. Simple fix. (AUDIT_10)
5. **Trap item costs** -- Freezing Trap and Blast Mine should cost 2, not 1. One-line fix. (AUDIT_14)
6. **Overupgrade bonus position** -- Currently post-DEF, should be pre-DEF. (AUDIT_09)
7. **Trap damage formulas** -- INT scaling 3.5x weaker than rAthena, missing BaseLv, missing TrapResearch bonus. (AUDIT_14)
8. **Detect skill AoE** -- 7x7 should be 3x3. One-line fix. (AUDIT_14)
9. **Flasher AoE** -- 3x3 should be 5x5 per iRO Wiki. One-line fix. (AUDIT_14)
10. **Freeze duration formula** -- Missing +srcLUK, MDEF scaling halved. (AUDIT_10)
11. **BUFFS_SURVIVE_DEATH corrections** -- Remove song buffs, add riding. (AUDIT_10)
12. **Poison HP floor** -- Should be 25% MaxHP, not 1 HP. (AUDIT_10)
13. **Job class weight bonuses** -- All classes get +0 instead of +0 to +1000. Simple table addition. (AUDIT_12)
14. **Novice death penalty** -- Master says MISSING but AUDIT_13 says implemented. Verify and update master. (Contradiction C2)

### Tier 2: Missing Data (system works but data is incomplete)

15. **Forge recipes** -- Only 7 of 38+ exist. System works, needs data. (AUDIT_09)
16. **Condensed potion recipes** -- 3 critical recipes missing. (AUDIT_09)
17. **Monster skill DB coverage** -- 27 of 509 monsters have skills (13%). System works, needs data. (AUDIT_08)
18. **NPC_POWERUP/CALLSLAVE/REBIRTH** -- Critical NPC_ skills missing for MVP encounters. (AUDIT_08)
19. **Chase state skill evaluation** -- Easy fix, several MVP skills use chase state. (AUDIT_08)

### Tier 3: Core Missing Systems (new features)

20. **Player-to-Player Trading** -- No dependencies, high player demand. (Master + AUDIT_12)
21. **Kafra Storage** -- No dependencies (Kafra NPC exists). (Master + AUDIT_12)
22. **Respawn/Resurrection flow** -- Resurrection skill on dead players, Yggdrasil Leaf, Token of Siegfried. (AUDIT_13)
23. **Card combo set bonuses** -- Multi-card combos are build-defining. (AUDIT_09)
24. **Player race/size/element classification** -- Required before PvP can be enabled. (AUDIT_11)
25. **Guild System** -- Blocks WoE, guild chat, guild storage. (Master + AUDIT_11)

### Tier 4: Content & Social (new features, lower urgency)

26. **Quest framework** -- Blocks all narrative content. (Master + AUDIT_13)
27. **NPC dialogue framework** -- All NPC interactions are single-action. (AUDIT_13)
28. **Friend list** -- Social feature. (Master)
29. **Emote system** -- Social expression. (Master)
30. **Zone expansion** -- Only 7 of ~420 maps. Content scaling issue. (Master + AUDIT_13)
31. **Ground item system** -- Enables authentic loot flow. (AUDIT_12)

### Tier 5: Endgame & Competitive

32. **PvP Arena system** -- Requires player classification first. (Master + AUDIT_11)
33. **Transcendent/Rebirth system** -- Blocks endgame progression. (Master + AUDIT_13)
34. **War of Emperium** -- Requires guild system first. (Master + AUDIT_11)

### Tier 6: Polish & Niche

35. **Homunculus skill completion** -- Most skills stubbed. (AUDIT_12)
36. **Client-side pet/homunculus actors** -- Visual companions. (AUDIT_12)
37. **Performance SP drain rate corrections** -- Balance tuning. (AUDIT_14)
38. **Status effect formula corrections** -- Blind min duration, Confusion inversion, Curse split-stat. (AUDIT_10)
39. **Trap feature completeness** -- Durability, arming delay, item return, Arrow Shower push. (AUDIT_14)
40. **Marriage/Adoption/Baby classes** -- Niche social system. (AUDIT_13)

---

## Final Consolidated Gap List (ALL issues from ALL audits, unified)

### Category A: Bugs in Implemented Systems (WRONG behavior)

| # | System | Issue | Source | Severity | Fix Effort |
|---|--------|-------|--------|----------|------------|
| A1 | Forging | Weapon level penalty: `wLv*1000` should be `(wLv-1)*1000` | AUDIT_09 | HIGH | 1 line |
| A2 | Refining | Overupgrade bonus placed post-DEF, should be pre-DEF | AUDIT_09 | MEDIUM | Move code block |
| A3 | Traps | Freezing Trap + Blast Mine cost 1 trap, should cost 2 | AUDIT_14 | HIGH | 1 line |
| A4 | Traps | Damage formulas use INT/100 instead of INT/35, missing BaseLv, missing TrapResearch | AUDIT_14 | HIGH | Rewrite formulas |
| A5 | Traps | Flasher AoE 3x3 should be 5x5 per iRO Wiki | AUDIT_14 | MEDIUM | 1 line |
| A6 | Traps | Detect skill AoE 7x7 should be 3x3 | AUDIT_14 | MEDIUM | 1 line |
| A7 | Status | OPT1 statuses (Stun/Freeze/Stone/Sleep) can stack simultaneously | AUDIT_10 | MEDIUM | Add mutual exclusion check |
| A8 | Status | Freeze duration missing +srcLUK and MDEF scaling halved (/200 vs /100) | AUDIT_10 | MEDIUM | Special-case in formula |
| A9 | Status | Poison HP floor is 1 HP instead of 25% MaxHP | AUDIT_10 | LOW | Set minHpPercent |
| A10 | Status | Blind min duration 1s instead of 15s | AUDIT_10 | LOW | Special-case |
| A11 | Status | Confusion chance uses standard level diff instead of inverted | AUDIT_10 | LOW | Special-case |
| A12 | Status | Curse duration uses LUK instead of VIT (split-stat) | AUDIT_10 | LOW | Refactor |
| A13 | Status | Poison duration LUK scaling 10x weaker than rAthena | AUDIT_10 | LOW | Fix coefficient |
| A14 | Buffs | BUFFS_SURVIVE_DEATH includes song buffs (should not) and excludes riding (should include) | AUDIT_10 | MEDIUM | Fix list |
| A15 | Buffs | magic_rod missing from UNDISPELLABLE set | AUDIT_10 | LOW | Add to set |
| A16 | Buffs | Two separate UNDISPELLABLE copies can drift out of sync | AUDIT_10 | LOW | Consolidate |
| A17 | Buffs | Chemical Protection buffs lack BUFF_TYPES entries (no display name) | AUDIT_10 | LOW | Add entries |
| A18 | Party | Tap bonus cap 375% instead of 580% | AUDIT_11 | LOW | Fix Math.min |
| A19 | Party | +1 EXP floor per member missing | AUDIT_11 | LOW | Add +1 |
| A20 | Performance | SP drain rates differ for Bragi (5s vs 3s), Apple (6s vs 3s), PDFM (10s vs 3s) | AUDIT_14 | MEDIUM | Review/fix |
| A21 | Monster AI | Slaves use own AI code instead of 24 (passive slave) | AUDIT_08 | LOW | Override after spawn |
| A22 | Monster AI | Per-template MVP respawn timers overridden with generic 120 min | AUDIT_08 | LOW | Use template value |
| A23 | Pharmacy | Blue Potion rate modifier 0 should be 1000 | AUDIT_09 | LOW | Fix rateMod |
| A24 | Pharmacy | All recipes use same guide book ID 7144 | AUDIT_09 | MEDIUM | Add per-group IDs |
| A25 | Homunculus | Intimacy gain rates 10x faster than RO Classic | AUDIT_12 | MEDIUM | Scale down |
| A26 | Master | Novice death penalty listed as MISSING but is actually implemented | Contradiction C2 | -- | Fix master doc |
| A27 | Master | Zone count listed as 4 but actual count appears to be 7 | Contradiction C1 | -- | Fix master doc |
| A28 | Master | Multiple DONE ratings should be PARTIAL (forge, pharmacy, traps, homunculus skills, monster skills, looter) | Multiple | -- | Fix master doc |

### Category B: Missing Features in Existing Systems (INCOMPLETE)

| # | System | Feature | Source | Severity | Fix Effort |
|---|--------|---------|--------|----------|------------|
| B1 | Death | `player:respawn` socket handler | AUDIT_13 | CRITICAL | 1-2 hours |
| B2 | Zones | Map flag enforcement (noteleport, noreturn, nomemo) | AUDIT_13 | HIGH | 2-3 hours |
| B3 | Forging | Only 7 of 38+ recipes defined | AUDIT_09 | HIGH | 4-6 hours (data entry) |
| B4 | Pharmacy | Condensed potion recipes (545/546/547) missing | AUDIT_09 | HIGH | 30 min |
| B5 | Forging | No anvil type detection or bonus | AUDIT_09 | MEDIUM | 2 hours |
| B6 | Forging | No hammer requirement/consumption | AUDIT_09 | MEDIUM | 2 hours |
| B7 | Cards | Card combo set bonuses not implemented | AUDIT_09 | HIGH | 1-2 days |
| B8 | Cards | Skill-granting cards (Smokie/Creamy/Horong/Poporing) non-functional | AUDIT_09 | MEDIUM | 4-6 hours |
| B9 | Cards | Socket Enchant NPC missing | AUDIT_09 | MEDIUM | 4-6 hours |
| B10 | Refining | Blacksmith Weapon Refine skill (1218) missing | AUDIT_09 | MEDIUM | 2-3 hours |
| B11 | Status | Deadly Poison (SC_DPOISON) completely missing | AUDIT_10 | MEDIUM | 3-4 hours |
| B12 | Status | Spider Web not implemented | AUDIT_10 | LOW | 3-4 hours |
| B13 | Status | Coma handler (HP=1, SP=1) not implemented | AUDIT_10 | LOW | 1-2 hours |
| B14 | Monster | Chase state skill evaluation missing | AUDIT_08 | MEDIUM | 1-2 hours |
| B15 | Monster | NPC_POWERUP (195) -- ATK x3 self-buff | AUDIT_08 | HIGH | 1-2 hours |
| B16 | Monster | NPC_CALLSLAVE (352) -- recall slaves | AUDIT_08 | HIGH | 1-2 hours |
| B17 | Monster | NPC_REBIRTH (207) -- respawn on death | AUDIT_08 | HIGH | 2-3 hours |
| B18 | Monster | MD_NOCAST flag not checked before skill use | AUDIT_08 | LOW | 1 line |
| B19 | Monster | 8 skill conditions not implemented | AUDIT_08 | MEDIUM | 4-6 hours |
| B20 | Monster | Only 27/509 monsters have skill entries | AUDIT_08 | HIGH | 10-20 hours (data) |
| B21 | Monster | MD_IGNOREMELEE/MAGIC/RANGED/MISC missing | AUDIT_08 | MEDIUM | 3-4 hours |
| B22 | Monster | Slaves do not follow master | AUDIT_08 | MEDIUM | 3-4 hours |
| B23 | Monster | MVP winner fallback on disconnect | AUDIT_08 | LOW | 1 hour |
| B24 | Monster | MVP tombstone cleanup timer | AUDIT_08 | LOW | 30 min |
| B25 | Death | Resurrection skill on dead players | AUDIT_13 | HIGH | 3-4 hours |
| B26 | Death | Yggdrasil Leaf on dead players | AUDIT_13 | HIGH | 2-3 hours |
| B27 | Death | Token of Siegfried self-rez | AUDIT_13 | MEDIUM | 2-3 hours |
| B28 | Weight | Job class weight bonuses (+0 to +1000) | AUDIT_12 | MEDIUM | 30 min |
| B29 | Party | Item distribution logic (fields stored, not used) | AUDIT_11 | MEDIUM | 3-4 hours |
| B30 | PvP | Player race/size/element classification | AUDIT_11 | HIGH | 2-3 hours |
| B31 | Traps | 2-cell minimum placement distance | AUDIT_14 | MEDIUM | 1-2 hours |
| B32 | Traps | Trap durability (3500 HP, destructible) | AUDIT_14 | MEDIUM | 3-4 hours |
| B33 | Traps | Trap item return on expiry | AUDIT_14 | LOW | 1-2 hours |
| B34 | Traps | Arrow Shower trap pushing | AUDIT_14 | MEDIUM | 2-3 hours |
| B35 | Mount | Mount state not persisted to DB | AUDIT_14 | LOW | 30 min |
| B36 | Homunculus | Most skills stubbed | AUDIT_12 | MEDIUM | 6-10 hours |
| B37 | Homunculus | Position broadcast to other players | AUDIT_12 | MEDIUM | 3-4 hours |
| B38 | NPC | Dialogue tree framework | AUDIT_13 | HIGH | 2-3 days |

### Category C: Entirely Missing Systems (NOT STARTED)

| # | System | Source | Severity | Fix Effort |
|---|--------|--------|----------|------------|
| C1 | Player-to-Player Trading | AUDIT_12, Master | CRITICAL | 2-3 days |
| C2 | Kafra Storage (account-wide) | AUDIT_12, Master | CRITICAL | 2-3 days |
| C3 | Guild System (entire) | AUDIT_11, Master | CRITICAL | 8-12 days |
| C4 | War of Emperium (entire) | AUDIT_11, Master | HIGH | 10-15 days (needs C3) |
| C5 | PvP Arena system | AUDIT_11, Master | MEDIUM | 2-3 days (needs B30) |
| C6 | Quest Framework | AUDIT_13, Master | HIGH | 5-7 days |
| C7 | Ground Item System | AUDIT_12 | MEDIUM | 3-5 days |
| C8 | Transcendent/Rebirth | AUDIT_13, Master | HIGH | 10-15 days |
| C9 | Friend List | AUDIT_13, Master | MEDIUM | 2-3 days |
| C10 | Emote System (50+ emotes) | AUDIT_13, Master | LOW | 1-2 days |
| C11 | Chat Rooms (Alt+C) | AUDIT_13 | LOW | 2-3 days |
| C12 | Duel System | AUDIT_11 | LOW | 1-2 days |
| C13 | Marriage/Adoption | AUDIT_13 | LOW | 3-5 days |
| C14 | Extended Classes (6 classes) | Coverage Audit | LOW | 15-25 days |
| C15 | Dead Branch / Bloody Branch | Master | LOW | 1 day |
| C16 | Mercenary System | Coverage Audit | LOW | 3-5 days |
| C17 | Battlegrounds | Master | LOW | 5-7 days |
| C18 | Mail/Auction System | Coverage Audit | LOW | 3-5 days |
| C19 | Pet/Homunculus Client Actors | AUDIT_12 | MEDIUM | 3-5 days |
| C20 | Zone Expansion (413+ maps) | AUDIT_13, Master | HIGH | 20-40 days |

### Summary Counts

| Category | Count | Description |
|----------|-------|-------------|
| A (Bugs) | 28 | Wrong formulas, wrong values, incorrect behavior |
| B (Incomplete) | 38 | Missing features in systems that otherwise work |
| C (Not Started) | 20 | Entirely absent systems |
| **Total** | **86** | All verified gaps across 7 audits + master + coverage |

### Estimated Total Remediation Effort

| Category | Effort Range |
|----------|-------------|
| A (Bug fixes) | 3-5 days |
| B (Feature completion) | 15-25 days |
| C (New systems) | 80-140 days |
| **Total** | **~100-170 days** |

---

*Review generated by cross-referencing 7 system-level audit documents (AUDIT_08 through AUDIT_14), MASTER_Implementation_Gap_Analysis.md, and 00_Master_Coverage_Audit.md. All findings trace back to specific audit documents with issue numbers.*
