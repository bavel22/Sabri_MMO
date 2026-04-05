# Audit: Monster AI, MVP & Monster Skills

**Date**: 2026-03-22
**Deep Research Docs Audited**:
- `RagnaCloneDocs/Deep_Research/16_Monster_AI_Behavior.md`
- `RagnaCloneDocs/Deep_Research/17_MVP_Boss_Monster_Skills.md`

**Server Implementation Files Audited**:
- `server/src/index.js` (AI constants, spawnEnemy, state machine, aggro functions, monster skill system, MVP rewards, respawn, enemy tick loop)
- `server/src/ro_monster_ai_codes.js` (1004 monster-to-AI-code mappings)
- `server/src/ro_monster_skills.js` (27 monsters with skill entries, NPC_SKILLS definitions)
- `server/src/ro_monster_templates.js` (509 monster templates)

---

## Summary

The server implementation is **strong and well-aligned** with the deep research. The core AI state machine (IDLE/CHASE/ATTACK/DEAD), mode flag parsing, aggro system, target switching, assist mechanics, monster skill system, MVP reward pipeline, and slave spawning are all implemented correctly and match rAthena pre-renewal behavior closely. The main gaps are: missing Follow/Angry sub-states (AI code 04), several advanced NPC_ skill types not yet implemented, no Looter mechanic, limited monster skill DB coverage (27 of 509 monsters), and some respawn timer inaccuracies for per-template MVP timers. No critical bugs found; the architecture is sound and extensible.

---

## AI Code Coverage (which codes are implemented)

### MD Constants (Mode Flag Bitmask)

| Flag | Hex | Research | Implemented | Notes |
|------|-----|----------|-------------|-------|
| MD_CANMOVE | 0x0001 | YES | YES | Checked in movement, wander, chase |
| MD_LOOTER | 0x0002 | YES | PARSED ONLY | Parsed in `parseModeFlags()` but **no runtime behavior** -- monsters never pick up ground items |
| MD_AGGRESSIVE | 0x0004 | YES | YES | Drives `findAggroTarget()` in IDLE state |
| MD_ASSIST | 0x0008 | YES | YES | `triggerAssist()` fully implemented |
| MD_CASTSENSORIDLE | 0x0010 | YES | YES | Set via cast sensor hook, consumed in IDLE state |
| MD_NORANDOMWALK | 0x0020 | YES | YES | Checked in `processWander()` |
| MD_NOCAST | 0x0040 | YES | PARSED ONLY | Parsed but **never checked** -- monsters with this flag could still use skills |
| MD_CANATTACK | 0x0080 | YES | YES | Checked in `setEnemyAggro()`, `triggerAssist()` |
| MD_CASTSENSORCHASE | 0x0200 | YES | PARSED ONLY | Parsed but **not used in CHASE state** -- only idle cast sensor is checked |
| MD_CHANGECHASE | 0x0400 | YES | YES | Implemented in CHASE state -- switches to closer in-range player |
| MD_ANGRY | 0x0800 | YES | PARSED ONLY | Parsed but **Follow/Angry sub-states not implemented** |
| MD_CHANGETARGETMELEE | 0x1000 | YES | YES | `shouldSwitchTarget()` checks `hitType === 'melee'` |
| MD_CHANGETARGETCHASE | 0x2000 | YES | YES | `shouldSwitchTarget()` checks CHASE state |
| MD_TARGETWEAK | 0x4000 | YES | YES | `findAggroTarget()` filters `playerLevel >= enemy.level - 5` |
| MD_RANDOMTARGET | 0x8000 | YES | YES | Implemented in ATTACK state random pick |
| MD_IGNOREMELEE | 0x10000 | YES | NOT IMPLEMENTED | Not in MD constants |
| MD_IGNOREMAGIC | 0x20000 | YES | NOT IMPLEMENTED | Not in MD constants |
| MD_IGNORERANGED | 0x40000 | YES | NOT IMPLEMENTED | Not in MD constants |
| MD_MVP | 0x80000 | YES | YES (partial) | Not in bitmask constants but set manually for `monsterClass === 'mvp'` |
| MD_IGNOREMISC | 0x100000 | YES | NOT IMPLEMENTED | Not in MD constants |
| MD_KNOCKBACKIMMUNE | 0x200000 | YES | YES | Checked in knockback handler |
| MD_TELEPORTBLOCK | 0x400000 | YES | NOT IMPLEMENTED | Not in MD constants |
| MD_FIXEDITEMDROP | 0x1000000 | YES | NOT IMPLEMENTED | Not in MD constants |
| MD_DETECTOR | 0x2000000 | YES | YES | Checked in `findAggroTarget()` for hidden players |
| MD_STATUSIMMUNE | 0x4000000 | YES | YES | Checked broadly across status application |
| MD_SKILLIMMUNE | 0x8000000 | YES | NOT IMPLEMENTED | Not in MD constants |

**Coverage**: 14 of 24 flags have runtime behavior. 4 are parsed but unused. 6 are not in the constants at all.

### AI Type Code Hex Mappings

| Code | Research Hex | Implemented Hex | Match | Notes |
|------|-------------|-----------------|-------|-------|
| 01 | 0x0081 | 0x0081 | MATCH | Passive |
| 02 | 0x0083 | 0x0083 | MATCH | Passive + Looter |
| 03 | 0x1089 | 0x1089 | MATCH | Passive + Assist |
| 04 | 0x3885 | 0x3885 | MATCH | Angry/Hyper-Active (parsed, sub-states not implemented) |
| 05 | 0x2085 | 0x2085 | MATCH | Aggressive + Chase Switch |
| 06 | 0x0000 | 0x0000 | MATCH | Immobile/Plant |
| 07 | 0x108B | 0x108B | MATCH | Passive + Looter + Assist |
| 08 | 0x7085 | 0x7085 | MATCH | Aggressive + Target Weak |
| 09 | 0x3095 | 0x3095 | MATCH | Aggressive + Cast Sensor |
| 10 | 0x0084 | 0x0084 | MATCH | Immobile Turret |
| 12 | 0x2085 | 0x2085 | MATCH | WoE Guardian (Mobile) |
| 13 | 0x308D | 0x308D | MATCH | Aggressive + Assist |
| 17 | 0x0091 | 0x0091 | MATCH | Passive + Cast Sensor |
| 20 | 0x3295 | 0x3295 | MATCH | Enhanced Cast Sensor |
| 21 | 0x3695 | 0x3695 | MATCH | Boss/MVP AI |
| 24 | 0x00A1 | 0x00A1 | MATCH | Slave (Passive) |
| 25 | 0x0001 | 0x0001 | MATCH | Pet |
| 26 | 0xB695 | 0xB695 | MATCH | Chaotic Aggressive |
| 27 | 0x8084 | 0x8084 | MATCH | Immobile + Random |

**All 19 AI type hex modes match perfectly.** Code 11 (0x0084) is absent from the implementation but is identical to code 10, which is present. Codes 14-16, 18-19, 22-23 are unused per research and correctly absent.

### AI Code Database Coverage

- **ro_monster_ai_codes.js**: 1004 entries mapping monster ID to AI code
- **ro_monster_templates.js**: 509 monster templates with full stats
- All 509 templates have a corresponding AI code lookup via `MONSTER_AI_CODES[roId]`
- Fallback: `getDefaultAiCode(aiType)` maps simplified string types ('passive'=1, 'aggressive'=5, 'reactive'=3)

---

## Mode Flag Coverage

### Flags with Full Runtime Behavior (10)
- `canMove` -- movement, wander, chase, spawn state
- `aggressive` -- IDLE aggro scan with configurable interval
- `assist` -- `triggerAssist()` with 550 UE range, same templateId, IDLE state only
- `castSensorIdle` -- cast sensor aggro hook in IDLE state
- `noRandomWalk` -- blocks wander in `processWander()`
- `canAttack` -- gates `setEnemyAggro()` and `triggerAssist()`
- `changeChase` -- target-of-opportunity in CHASE state
- `changeTargetMelee` -- `shouldSwitchTarget()` melee-only check
- `changeTargetChase` -- `shouldSwitchTarget()` CHASE state check
- `targetWeak` -- level-5 filter in `findAggroTarget()`
- `randomTarget` -- random pick in ATTACK state each swing
- `knockbackImmune` -- checked in `knockbackTarget()` and boss protocol
- `detector` -- hidden player detection in `findAggroTarget()`
- `statusImmune` -- checked in status application functions

### Flags Parsed but Not Behaviorally Used (4)
1. **`looter`** -- No ground item system exists. Parsed into modeFlags but never read at runtime.
2. **`nocast` (MD_NOCAST)** -- Parsed but monster skill system does not check this flag before executing skills.
3. **`castSensorChase`** -- Parsed but CHASE state does not implement cast sensor checking. Only IDLE state has the cast sensor hook.
4. **`angry`** -- Parsed but Follow/Angry sub-states from AI code 04 are not implemented. Angry mobs use standard IDLE/CHASE/ATTACK like all other AI types.

### Flags Missing from Constants (6)
1. **MD_IGNOREMELEE** (0x10000) -- Melee damage immunity
2. **MD_IGNOREMAGIC** (0x20000) -- Magic damage immunity
3. **MD_IGNORERANGED** (0x40000) -- Ranged damage immunity
4. **MD_IGNOREMISC** (0x100000) -- Misc/trap damage immunity
5. **MD_TELEPORTBLOCK** (0x400000) -- Teleport immunity
6. **MD_FIXEDITEMDROP** (0x1000000) -- Fixed drop rates
7. **MD_SKILLIMMUNE** (0x8000000) -- Skill immunity

---

## State Machine Comparison

### Research: 4 primary states + 2 sub-states for Angry mobs
### Implementation: 4 states (`AI_STATE.IDLE`, `AI_STATE.CHASE`, `AI_STATE.ATTACK`, `AI_STATE.DEAD`)

| State | Research | Implementation | Match |
|-------|----------|----------------|-------|
| IDLE | Wander, aggro scan, assist, cast sensor, idle skills | Wander, aggro scan, cast sensor, idle skills | MATCH |
| CHASE | Target validation, pending switch, range check, movement, changeChase, cast sensor chase, chase skills | Target validation, pending switch, range check, movement, changeChase | PARTIAL -- no chase skill evaluation, no cast sensor chase |
| ATTACK | Target validation, pending switch, random target, range check, hit stun, attack timing, execute attack, attack skills, target change melee | Target validation, pending switch, random target, range check, hit stun, attack timing, execute attack, attack skills | MATCH |
| DEAD | Mark dead, stop combat, award EXP, loot drops, broadcast, respawn timer | Mark dead, stop combat, award EXP, loot drops, broadcast, respawn timer | MATCH |
| FOLLOW | Pre-attack follow (Angry AI) | NOT IMPLEMENTED | MISSING |
| ANGRY | Pre-attack angry state | NOT IMPLEMENTED | MISSING |

### Tick Processing Details

| Aspect | Research | Implementation | Match |
|--------|----------|----------------|-------|
| Tick interval | 100ms (rAthena) | 200ms | CLOSE -- 200ms is reasonable for performance |
| Aggro scan interval | Every tick in rAthena (100ms) | 500ms (`AGGRO_SCAN_MS`) | REASONABLE -- reduces CPU cost |
| Idle skill interval | 1000ms | Every tick (200ms) with skill cooldowns | DIFFERENT but acceptable -- cooldowns control rate |
| Wander pause | 3-8 seconds | 3-8 seconds | MATCH |
| Wander distance | 100-300 UE units | 100-300 UE units | MATCH |
| Wander speed | 60% of chase speed | 60% of chase speed | MATCH |
| Chase give-up | chaseRange + extra | chaseRange + 200 UE | MATCH |
| Hit stun | damageMotion ms | damageMotion ms (default 300ms) | MATCH |
| Move broadcast | 200ms | 200ms | MATCH |
| Lazy AI (active zones) | Only process near players | `getActiveZones()` skips empty zones | MATCH |

### CHASE State Gaps
1. **No chase skill evaluation**: The CHASE state does not check `MONSTER_SKILL_DB` for `state: 'chase'` skills. Only IDLE and ATTACK states evaluate skills. Research says monsters should also use skills while chasing.
2. **No cast sensor chase**: `castSensorChase` flag is parsed but the CHASE state does not check if nearby players are casting. The research specifies this should trigger target switching during pursuit.

---

## Aggro System

| Feature | Research | Implementation | Match |
|---------|----------|----------------|-------|
| No threat table | Correct -- rule-based, not cumulative threat | Correct -- `shouldSwitchTarget()` uses flag checks only | MATCH |
| `setEnemyAggro()` | Add to inCombatWith, record damage time, check target switch, trigger assist | Identical logic with all steps | MATCH |
| `shouldSwitchTarget()` | No target = switch. Same target = no. RandomTarget = always. CTM+melee in IDLE/ATK. CTC in CHASE. | Exactly implemented | MATCH |
| `triggerAssist()` | Same templateId, IDLE, canMove+canAttack, within 550 UE (11 cells) | Correct with all checks | MATCH |
| Assist chaining | Does NOT chain | Does NOT chain (only iterates once from attacked enemy) | MATCH |
| `findAggroTarget()` | Closest player within aggroRange, zone filter, TargetWeak filter, detector filter | All filters present, plus Play Dead, Hiding, Gangster's Paradise, homunculus targets | MATCH+ |
| Rude attack behavior | Normal mobs teleport, bosses immune | `_rudeAttacked` flag set on chase give-up, Creamy has AL_TELEPORT on `rudeattacked` condition | PARTIAL -- teleport is skill-driven, not automatic for all normal mobs |
| Charming Wink | Not in research | Implemented -- charmed mobs follow caster, ignore aggro | EXTRA |
| Play Dead | Not in research | Implemented -- all monsters ignore player, no detector bypass | EXTRA |
| Gangster's Paradise | Not in research | Implemented -- 2+ sitting Rogues within 50 UE, boss immune | EXTRA |

### Aggro System Quality
The implementation exceeds the research in several areas: homunculus targeting, Gangster's Paradise, Play Dead immunity, and Charming Wink charm behavior. The core aggro logic matches rAthena precisely.

---

## MVP Reward System

| Feature | Research | Implementation | Match |
|---------|----------|----------------|-------|
| MVP determination | Highest `damage_dealt + damage_tanked` | Highest `totalDamage` (damage dealt only) | PARTIAL -- no damage-tanked component |
| MVP EXP bonus | `mvpExp` to winner as base EXP only | Correct -- `processExpGain(mvpPlayer, mvpExpReward, 0)` | MATCH |
| MVP drops | 3 slots, each rolled independently, rates /10000 | Iterated from `mvpDrops` array, independent rolls, rate/10000 | MATCH |
| MVP drops to inventory | Goes directly to winner's inventory | `addItemToInventory(mvpCharId, ...)` | MATCH |
| MVP announcement | Server-wide yellow system message | `io.emit('chat:receive', ...)` with `[MVP] {name} has defeated {monster}!` | MATCH |
| MVP tombstone | Spawns at death location, shows name/time/winner | `broadcastToZone('mvp:tombstone', ...)` with enemyId, bossName, killerName, x/y/z | MATCH |
| Tombstone duration | Until 5 seconds after MVP respawns | NOT IMPLEMENTED -- tombstone event emitted but no cleanup timer | MISSING |
| Tombstone exceptions | No tombstone for summoned/branch MVPs | NOT CHECKED -- always emits tombstone for any MVP death | MISSING |
| Regular EXP distribution | Proportional to damage dealt | Tap bonus (+25%/attacker), then all EXP to killer via party distribution | PARTIAL -- tap bonus present but proportional per-attacker distribution not implemented |
| Damage log size | 20 entries max (rAthena) | Unlimited `inCombatWith` Map | DIFFERENT -- more generous |
| Dead/disconnected winner | Falls to next highest scorer | `connectedPlayers.get(mvpCharId)` -- if null, skips rewards | PARTIAL -- does not fall back to next player |

### MVP Winner Determination Discrepancy
Research says `MVP Score = totalDamageDealt + totalDamageTanked`. Implementation only tracks `totalDamageDealt`. In practice this mostly favors DPS players, which is reasonable for a pre-renewal server. Adding damage-tanked tracking would require recording how much damage the MVP dealt to each player, which is moderately complex.

---

## Monster Skill System (which NPC_ skills exist)

### NPC_SKILLS Definitions in `ro_monster_skills.js`

| Skill ID | Name | Research | Implemented | Notes |
|----------|------|----------|-------------|-------|
| 171 | NPC_COMBOATTACK | YES | YES | type: 'multi_hit', hits: 2 |
| 175 | NPC_SELFDESTRUCTION | YES | YES | type: 'self_destruct', 400% multiplier |
| 176 | NPC_POISON | YES | YES | type: 'status_melee', 20% chance |
| 177 | NPC_BLINDATTACK | YES | YES | type: 'status_melee' |
| 178 | NPC_SILENCEATTACK | YES | YES | type: 'status_melee' |
| 179 | NPC_STUNATTACK | YES | YES | type: 'status_melee' |
| 180 | NPC_PETRIFYATTACK | YES | YES | type: 'status_melee', 5% chance |
| 181 | NPC_CURSEATTACK | YES | YES | type: 'status_melee' |
| 182 | NPC_SLEEPATTACK | YES | YES | type: 'status_melee' |
| 183 | NPC_RANDOMATTACK | YES | YES | type: 'random_element' |
| 184-192 | NPC_WATERATTACK thru NPC_UNDEADATTACK | YES | YES | 9 elemental melee types |
| 196 | NPC_SPEEDUP | YES | YES | type: 'self_buff', 1.5x speed |
| 197 | NPC_CRITICALSLASH | YES | YES | type: 'forced_crit' |
| 198 | NPC_EMOTION | YES | YES | type: 'emote' |
| 199 | NPC_AGIUP | YES | YES | type: 'self_buff', +20 AGI |
| 304 | NPC_METAMORPHOSIS | YES | YES | type: 'transform' -- full implementation |
| 485 | NPC_SUMMONSLAVE | YES | YES | type: 'summon' -- full implementation |
| 653 | NPC_EARTHQUAKE | YES | YES | type: 'aoe_physical', 750 radius |
| 656 | NPC_DARKBREATH | YES | YES | type: 'ranged_magic', dark element |
| 657 | NPC_DARKBLESSING | YES | YES | type: 'status_ranged', Coma (HP=1, SP=1) |
| 660-669 | NPC_WIDEBLEEDING thru NPC_WIDESLEEP | YES | YES | 6 AoE status skills |
| 687 | NPC_BLOODDRAIN | YES | YES | type: 'drain_hp', 5% drain |
| 688 | NPC_ENERGYDRAIN | YES | YES | type: 'drain_sp', 5% drain |

### NPC_ Skills in Research but NOT Implemented

| Skill ID | Name | Category | Priority |
|----------|------|----------|----------|
| 195 | NPC_POWERUP | Self-buff (ATK x3) | HIGH -- used by Baphomet, Eddga, Orc Hero |
| 198 | NPC_DEFENSE | Self-buff (DEF increase) | MEDIUM |
| 200 | NPC_HALLUCINATION | Status (screen distortion) | LOW |
| 201 | NPC_KEEPING | Self-buff (DEF=90, immobile) | LOW |
| 203 | NPC_BARRIER | Invincible barrier | MEDIUM |
| 204 | NPC_DEFENDER | Ranged damage reduction | LOW |
| 205/206 | NPC_INVINCIBLE/OFF | Full invincibility toggle | MEDIUM |
| 207 | NPC_REBIRTH | Respawn on death | HIGH -- used by some MVPs |
| 303 | NPC_SUMMONMONSTER | Random monster summon | LOW |
| 331 | NPC_RANDOMMOVE | Teleport to random position | MEDIUM -- needed for rude attack response |
| 339 | NPC_GRANDDARKNESS | AoE Shadow Grand Cross | HIGH -- Dark Lord signature |
| 342 | NPC_TRANSFORM | Cosmetic appearance change | LOW |
| 342 | NPC_CHANGEUNDEAD | Change target element | MEDIUM |
| 352 | NPC_CALLSLAVE | Recall slaves to master | HIGH -- used by Baphomet, Eddga |
| 654-658 | NPC_FIREBREATH thru NPC_ACIDBREATH | Breath attacks | MEDIUM |
| 659 | NPC_DRAGONFEAR | AoE fear + random statuses | MEDIUM |
| 662 | NPC_PULSESTRIKE | AoE knockback damage | MEDIUM |
| 663 | NPC_HELLJUDGEMENT | Massive AoE Shadow damage | HIGH -- Dark Lord signature |
| 664/666 | NPC_WIDECONFUSE/WIDESIGHT | AoE confusion/detection | LOW |
| 667-674 | Various WIDE_ skills | Additional AoE statuses | LOW |
| 670/671 | NPC_STONESKIN/ANTIMAGIC | DEF/MDEF tradeoff buffs | MEDIUM |

### Implemented NPC Skill Count
- **Defined in NPC_SKILLS**: 30 skill types
- **Research total NPC_ skills**: ~60+ skill types
- **Coverage**: approximately 50%

### Monster Skill DB Coverage

| Category | Count | Examples |
|----------|-------|---------|
| Zone 1 starters with skills | 2 | Hornet (poison), Familiar (blind) |
| Zone 2-3 mid-level | 7 | Zombie, Skeleton, Poporing, Smokie, Yoyo, Poison Spore, Creamy |
| MVPs with skill entries | 12 | Osiris, Baphomet, Drake, Angeling, Deviling, Orc Hero, Mistress, Maya, Eddga, GTB, Moonlight Flower, Phreeoni |
| Other bosses/mini-bosses | 6 | Orc Lord, Stormy Knight, Doppelganger, Pharaoh, Hatii, Turtle General (estimated from remaining entries) |
| **Total monsters with skills** | **~27** | Out of 509 templates |

**Gap**: 482 monsters have no skill entries. Research documents ~200+ monsters with skills in the full rAthena `mob_skill_db.txt`. Priority should be expanding entries for monsters that are actively spawned.

### Skill Condition Coverage

| Condition | Research | Implemented | Notes |
|-----------|----------|-------------|-------|
| always | YES | YES | |
| myhpltmaxrate | YES | YES | |
| myhpinrate | YES | YES | |
| mystatuson | YES | YES | |
| mystatusoff | YES | YES | |
| friendhpltmaxrate | YES | YES | Same templateId, 500 UE range |
| closedattacked | YES | YES | `_lastHitType === 'melee'` |
| longrangeattacked | YES | YES | `_lastHitType === 'ranged'` |
| skillused | YES | YES | `_lastSkillHitId` |
| casttargeted | YES | YES | `_isCastTarget` flag |
| rudeattacked | YES | YES | `_rudeAttacked` flag |
| onspawn | YES | YES | `_justSpawned` with 1s timeout |
| slavelt | YES | YES | `_slaveCount` |
| afterskill | YES | YES | `_lastSkillUsed` |
| attackpcgt | YES | YES | `inCombatWith.size` |
| attackpcge | YES | YES | |
| friendstatuson | YES | NOT IMPLEMENTED | |
| friendstatusoff | YES | NOT IMPLEMENTED | |
| slavele | YES | NOT IMPLEMENTED | |
| masterhpltmaxrate | YES | NOT IMPLEMENTED | |
| masterattacked | YES | NOT IMPLEMENTED | |
| alchemist | YES | NOT IMPLEMENTED | |
| groundattacked | YES | NOT IMPLEMENTED | |
| damagedgt | YES | NOT IMPLEMENTED | |

**Condition coverage**: 16 of 24 condition types implemented (67%).

---

## Slave Spawning

| Feature | Research | Implementation | Match |
|---------|----------|----------------|-------|
| Spawn position | Near master's current position | +/-200 UE offset from master | MATCH |
| Master death = slaves die | YES, no EXP/drops | YES -- slaves killed, `enemies.delete()`, skip EXP/drops | MATCH |
| Slave EXP when killed individually | YES, gives normal EXP/drops | YES -- only master-death path suppresses EXP | MATCH |
| Slave AI type | AI code 24 (CanMove + NoRandomWalk + CanAttack) | Uses template's native AI code, not forced to 24 | DISCREPANCY |
| Slave follows master | YES, movement speed matches master | NOT IMPLEMENTED -- slaves act independently | MISSING |
| Master teleport = slaves teleport | YES | NOT IMPLEMENTED | MISSING |
| NPC_CALLSLAVE (352) | Recall slaves to master | NOT IMPLEMENTED | MISSING |
| slavelt condition | Controls max slave count | YES -- `_slaveCount` tracked, condition evaluated | MATCH |
| Target sharing | Slaves attack master's target | YES -- shares `targetPlayerId` on summon | PARTIAL -- initial share only, no ongoing sync |
| Slave count tracking | `_slaves` Set + `_slaveCount` | YES -- both maintained | MATCH |

### Slave Lifecycle Quality
The core summon/death lifecycle is correct. The main gaps are:
1. Slaves do not actively follow the master after spawn
2. No NPC_CALLSLAVE to recall distant slaves
3. Slaves use their own template's AI code rather than AI code 24

---

## Critical Discrepancies

### D1: Follow/Angry Sub-States Not Implemented (MEDIUM)
AI code 04 monsters (Familiar, Zombie, Orc Warrior, Mummy, ~90 monsters) should have pre-attack Follow/Angry states with separate skill sets. Currently they use standard IDLE/CHASE/ATTACK. This affects their initial approach behavior and skill usage before first damage.

**Impact**: Moderate. These monsters work correctly as basic aggressive mobs but lack the authentic pre-attack behavior where they use a different skill set before being hit.

### D2: Chase Skill Evaluation Missing (MEDIUM)
The CHASE state does not evaluate `MONSTER_SKILL_DB` for `state: 'chase'` skills. The research specifies that monsters should be able to use skills while pursuing. Some MVP entries (e.g., Baphomet's NPC_DARKBREATH) use `state: 'chase'`.

**Impact**: Monsters that should use ranged skills while approaching their target do not. Chase-state skills in the MONSTER_SKILL_DB are silently ignored.

### D3: Cast Sensor Chase Not Active (LOW)
`castSensorChase` flag is parsed but the CHASE state does not check for nearby casters. Only IDLE state has cast sensor checking. This primarily affects AI code 20 and 21 monsters.

**Impact**: Low. Boss/MVP AI 21 monsters should detect casters while chasing but currently only detect on initial idle scan.

### D4: MVP Score Missing Damage-Tanked Component (LOW)
Research says MVP score = damage_dealt + damage_tanked. Implementation only tracks damage_dealt. This means tank characters who absorb significant damage from the MVP but deal less damage get less credit.

**Impact**: Low. Most pre-renewal MVPs are killed by high-DPS characters. The damage-tanked component mainly benefits Crusader/Knight tanks in party play.

### D5: Respawn Timer Not Per-Template for MVPs (LOW)
Implementation uses generic 120-130 min for all MVPs and 60-70 min for all bosses, ignoring per-template `respawnMs`. The research documents varied timers (60 min to 24 hours) based on the specific MVP.

**Impact**: Low until MVPs are actively spawned. The template data has the correct `respawnMs` values; the respawn code just overrides them with generic values.

---

## Missing Features

### High Priority (affects current gameplay)

1. **Chase state skill evaluation** -- Add `MONSTER_SKILL_DB` check in the CHASE case of the AI tick loop, matching `state: 'chase'` entries. Several MVP skill entries use chase-state skills.

2. **NPC_CALLSLAVE (352)** -- Recall slaves to master position. Used by Baphomet, Eddga, and other MVPs with wandering slaves.

3. **NPC_POWERUP (195)** -- ATK x3 self-buff. Used by Orc Hero, Eddga, Baphomet in rAthena's mob_skill_db. Several MVP entries reference this but the NPC skill type is missing.

4. **NPC_REBIRTH (207)** -- Monster respawn at same location on death. Used by some mini-bosses and special monsters.

### Medium Priority (affects authenticity)

5. **Looter behavior** -- Ground item pickup during IDLE. Would require a ground item system first.

6. **MD_NOCAST check** -- Prevent skill usage for monsters with this flag.

7. **Cast sensor in CHASE state** -- Check for nearby casters during pursuit (for AI 20/21).

8. **Follow/Angry sub-states** -- Separate skill sets before/after first damage for AI code 04.

9. **NPC_HELLJUDGEMENT (663)** -- Dark Lord's signature massive AoE shadow damage.

10. **NPC_GRANDDARKNESS (339)** -- AoE shadow Grand Cross variant.

11. **NPC_STONESKIN/ANTIMAGIC (670/671)** -- DEF/MDEF tradeoff buffs used by high-level MVPs.

12. **NPC_BARRIER/INVINCIBLE (203/205)** -- Short invincibility used by some bosses.

13. **NPC_RANDOMMOVE (331)** -- Random teleport, needed for proper rude attack response on normal mobs.

14. **Per-template MVP respawn timers** -- Use `template.respawnMs` instead of hardcoded 120 min for MVPs.

15. **Slave follow master** -- Slaves should follow master's position, not act independently.

16. **Breath attack skills (654-658)** -- Elemental breath attacks used by dragon-type monsters.

### Low Priority (niche behavior)

17. **MD_IGNOREMELEE/MAGIC/RANGED/MISC** -- Damage type immunity flags.
18. **MD_TELEPORTBLOCK** -- Prevent teleportation of monster.
19. **MD_FIXEDITEMDROP** -- Fixed drop rates unaffected by server multipliers.
20. **MD_SKILLIMMUNE** -- Full skill immunity.
21. **NPC_HALLUCINATION (200)** -- Screen distortion status.
22. **NPC_KEEPING/DEFENDER (201/204)** -- Defensive self-buffs.
23. **NPC_TRANSFORM (342)** -- Cosmetic transform vs full metamorphosis.
24. **NPC_CHANGEUNDEAD (342)** -- Change target's element.
25. **Proportional EXP distribution** -- Split EXP among multiple attackers by damage share (currently all goes to killer/party).
26. **Tombstone cleanup** -- Remove tombstone 5 seconds after MVP respawns.
27. **Tombstone exceptions** -- Skip tombstone for summoned/branch MVPs.
28. **MVP winner fallback** -- If top scorer is disconnected, award to next highest.
29. **friendstatuson/off conditions** -- Monster skill conditions for ally status checks.
30. **Damage type immunity flags** -- MD_IGNOREMELEE, MD_IGNOREMAGIC, MD_IGNORERANGED, MD_IGNOREMISC.

---

## Recommended Fixes

### Fix 1: Add Chase State Skill Evaluation (EASY)
In the AI tick loop CHASE case (~line 30352), add monster skill evaluation before movement:

```javascript
// In AI_STATE.CHASE, before movement:
const chaseSkills = MONSTER_SKILL_DB[enemy.templateId];
if (chaseSkills && chaseSkills.length > 0 && !inHitStun) {
    const chaseSkill = selectMonsterSkill(enemy, chaseSkills, AI_STATE.CHASE);
    if (chaseSkill) {
        executeMonsterSkill(enemy, chaseSkill, enemy.zone, io);
        break;
    }
}
```

### Fix 2: Use Per-Template Respawn for MVPs (EASY)
In `processEnemyDeathFromSkill()` (~line 2531), change the respawn logic to use template values:

```javascript
let respawnDelay = enemy.respawnMs || 15000;
// MVP/Boss variance: add 0-10 min random on top of template base
if (enemy.monsterClass === 'mvp' || enemy.monsterClass === 'boss') {
    respawnDelay += Math.floor(Math.random() * 600000); // +0-10 min variance
}
```
This preserves the variance window while using per-template base timers (which are already set from `ro_monster_templates.js`).

### Fix 3: Add MD_NOCAST Check (EASY)
In `selectMonsterSkill()` (~line 28862), add at the top:

```javascript
if (enemy.modeFlags && enemy.modeFlags.nocast) return null;
```

### Fix 4: Add NPC_POWERUP (195) (EASY)
Add to NPC_SKILLS and the executeNPCSkill handler. ATK x3 self-buff for 30 seconds. Critical for Orc Hero and Eddga.

### Fix 5: Add NPC_CALLSLAVE (352) (EASY)
Add a 'recall_slaves' type to NPC_SKILLS. On execution, iterate `enemy._slaves`, set each slave's position to master's position, and broadcast `enemy:move` with `teleport: true`.

### Fix 6: Slave AI Code Override (EASY)
In the NPC_SUMMONSLAVE handler, after spawning the slave, override its AI code to 24 and reparse mode flags:

```javascript
slave.aiCode = 24;
slave.modeFlags = parseModeFlags(AI_TYPE_MODES[24]);
```

---

## Score Summary

| Category | Score | Notes |
|----------|-------|-------|
| AI Code Hex Mappings | 19/19 (100%) | All codes match perfectly |
| Mode Flag Constants | 18/24 (75%) | 6 flags missing from MD constants |
| Mode Flag Runtime | 14/24 (58%) | 4 parsed but unused, 6 missing |
| State Machine | 4/6 (67%) | Follow/Angry sub-states missing |
| Aggro System | 10/10 (100%) | Exceeds research with extra features |
| MVP Rewards | 7/11 (64%) | Core works, missing damage-tanked, tombstone cleanup, fallback |
| NPC Skills Defined | 30/60+ (~50%) | Core combat skills present, many utility/defensive missing |
| Monster Skill DB | 27/200+ (~13%) | Only 27 monsters have skill entries |
| Skill Conditions | 16/24 (67%) | Core conditions present, slave/master/friend variants missing |
| Slave Lifecycle | 6/10 (60%) | Core summon/death works, follow/recall missing |
| Spawn System | 9/10 (90%) | Only gap is per-template MVP respawn timer override |
| **Overall** | **~70%** | Strong foundation, extensible architecture, clear expansion path |
