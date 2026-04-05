# Novice Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14
**Implemented:** 2026-03-15
**Status:** IMPLEMENTATION COMPLETE
**Scope:** All Novice class skills (IDs 1-3 in our system; rAthena IDs 1, 142, 143)

---

## Executive Summary

Deep research against iRO Wiki (Classic + Renewal), rAthena pre-renewal YAML database (`db/pre-re/skill_db.yml`), Divine Pride, and RateMyServer reveals **significant gaps** in all 3 Novice skills. Basic Skill (ID 1) is defined as a passive with no server-side logic -- it should gate 9 game features at each level but none are enforced. First Aid (ID 2) is functionally correct but has wrong SP cost in our definition (uses 3 SP correctly in handler, but definition says `spCost: 3` which is actually correct -- however it is missing quest-skill classification and after-cast delay). Play Dead (ID 3) has **no handler at all** -- it is defined in skill data but never implemented server-side, making it a completely non-functional skill. Additionally, the job change system does not enforce Basic Skill Lv9 as a prerequisite, contrary to canonical RO Classic behavior.

**New systems needed:**
1. Play Dead toggle handler (SC_TRICKDEAD status: blocks movement, attacks, skills, items, HP/SP regen)
2. Basic Skill level-gating for game features (trading, emotes, sitting, chat rooms, parties, Kafra, party creation, class change)
3. Quest skill infrastructure (First Aid and Play Dead should be quest-learned, not skill-point-allocated)

---

## Skill-by-Skill Analysis

---

### 1. BASIC SKILL (ID 1) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 1 (NV_BASIC) | rAthena `db/pre-re/skill_db.yml` |
| Type | Passive | All sources |
| Max Level | 9 | All sources |
| SP Cost | 0 (passive) | All sources |
| Cast Time | N/A (passive) | All sources |
| After-Cast Delay | N/A (passive) | All sources |
| Cooldown | N/A (passive) | All sources |
| Prerequisites | None | rAthena `db/pre-re/skill_tree.yml` |
| Skill Points | Learned via skill point allocation (NOT quest) | All sources |
| Lv 1 | Enables Trading with other players | iRO Wiki, RateMyServer, pservero |
| Lv 2 | Enables Emotes (Alt+number keys) | iRO Wiki, RateMyServer, pservero |
| Lv 3 | Enables Sitting (doubles HP/SP regen speed) | iRO Wiki, RateMyServer, pservero |
| Lv 4 | Enables Chat Room creation (Alt+C) | iRO Wiki, RateMyServer, pservero |
| Lv 5 | Enables joining Parties | iRO Wiki, RateMyServer, pservero |
| Lv 6 | Enables Kafra Storage access | iRO Wiki, RateMyServer, pservero |
| Lv 7 | Enables Party creation (/organize command) | iRO Wiki, RateMyServer, pservero |
| Lv 8 | (Removed -- was alignment system; level still required for XP progression) | iRO Wiki |
| Lv 9 | Enables Job Change to First Class | All sources |
| Job Change Gate | Novice must have Basic Skill Lv9 AND Job Level 10 to class change | iRO Wiki, RateMyServer |

#### Current Implementation Status: PARTIALLY CORRECT (definition only)

- Skill definition: **CORRECT** -- `maxLevel: 9, type: 'passive'`
- SP cost: **CORRECT** -- `spCost: 0` at all levels
- Passive type: **CORRECT** -- `type: 'passive'`
- Level effects: **NOT IMPLEMENTED** -- No server-side logic gates any feature by Basic Skill level
- Job change gate: **NOT IMPLEMENTED** -- `job:change` handler checks Job Level 10 only, does NOT check Basic Skill Lv9
- Description: **PARTIALLY CORRECT** -- mentions class change at level 9 but not the per-level unlocks

#### Gaps Found

| Gap | Priority | Effort | Notes |
|-----|----------|--------|-------|
| Job change does not require Basic Skill Lv9 | HIGH | Trivial | Add `learnedSkills[1] >= 9` check in `job:change` handler |
| No per-level feature gating | LOW | Medium | Most features not yet implemented (trading, chat rooms, parties, sitting) |
| Description incomplete | LOW | Trivial | Cosmetic only |
| Missing `afterCastDelay: 0` field in level data | LOW | Trivial | Consistency with other skill defs |

#### Implementation Notes

**Job Change Gate (HIGH priority):** Add a check in the `job:change` socket handler (around line 4688 in index.js) after the job level check:

```javascript
// Validate: novice -> first class requires Basic Skill Lv9
if (currentTier === 0) {
    const basicSkillLv = (player.learnedSkills || {})[1] || 0;
    if (basicSkillLv < 9) {
        socket.emit('job:error', { message: `Requires Basic Skill Level 9 (current: ${basicSkillLv})` });
        return;
    }
    // ... existing job level 10 check
}
```

**Per-Level Feature Gating (LOW priority):** Most gated features (trading, chat rooms, party creation, Kafra storage) are not yet implemented as game systems. When they are implemented, each should check `(player.learnedSkills || {})[1] >= requiredLevel`. For now, this is a deferred concern. The sitting regen bonus (Lv3) is worth noting: if sitting is ever implemented, sitting should double HP/SP regen rates, but only if Basic Skill >= 3.

---

### 2. FIRST AID (ID 2) -- Active (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 142 (NV_FIRSTAID) | rAthena `db/pre-re/skill_db.yml` |
| Type | Active / Supportive | All sources |
| Max Level | 1 | All sources |
| Target | Self | All sources |
| SP Cost | 3 | rAthena YAML, Divine Pride, iRO Wiki, RateMyServer |
| HP Restored | 5 | All sources |
| Cast Time | 0 ms | rAthena YAML (no CastTime field) |
| After-Cast Delay | 0 ms (ASPD-based) | rAthena YAML (no AfterCastActDelay field); iRO Wiki Classic says "ASPD-based" |
| Cooldown | 0 ms | rAthena YAML (no Cooldown field) |
| Element | Neutral | rAthena YAML (no Element field = Neutral) |
| Range | 0 (self-only) | rAthena YAML, pservero |
| Prerequisites (skill tree) | None | rAthena `db/pre-re/skill_tree.yml` |
| Quest Skill | Yes (`IsQuest: true` flag) | rAthena YAML |
| Quest: Base Level Req | 4 (outside Training Grounds) | iRO Wiki Classic |
| Quest: Job Level Req | 3 | RateMyServer |
| Quest: NPC | Nami (Prontera Inn, prt_in 234,133) | iRO Wiki Classic, RateMyServer |
| Quest: Items | 3 Red Herbs + 3 Clovers + 1 Sterilized Bandage | iRO Wiki Classic, RateMyServer |
| DamageFlags | NoDamage: true | rAthena YAML |
| Hit | Single, HitCount: 1 | rAthena YAML |
| Interruptible | Yes | pservero |
| Class Availability | Novice (all classes inherit via class progression) | rAthena skill tree |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: **CORRECT** -- Definition says `spCost: 3`, handler deducts correctly
- HP restored: **CORRECT** -- `effectValue: 5`, handler heals correctly
- Cast time: **CORRECT** -- `castTime: 0`
- Cooldown: **CORRECT** -- `cooldown: 0`
- Self-target: **CORRECT** -- `targetType: 'self'`
- Handler logic: **CORRECT** -- Deducts SP, heals HP, caps at maxHealth, broadcasts VFX, emits events
- Auto Berserk check: **CORRECT** -- Calls `checkAutoBerserk()` after healing

#### Gaps Found

| Gap | Priority | Effort | Notes |
|-----|----------|--------|-------|
| Not classified as quest skill | LOW | Trivial | Add `isQuestSkill: true` field to definition |
| Prerequisites say `[]` but should indicate quest-learned | LOW | Trivial | Cosmetic -- quest infrastructure not implemented |
| Missing `afterCastDelay: 0` field in level data | LOW | Trivial | Consistency |
| Heal VFX broadcasts as 'holy' element but skill is neutral | LOW | Trivial | Change element in broadcast to 'neutral' (or keep 'holy' for visual consistency with heal effects) |
| Prerequisite in our def is `[]` (no prereqs) which matches rAthena skill tree | NONE | N/A | Correct as-is |

#### Implementation Notes

First Aid is functionally correct. The only changes needed are cosmetic/metadata:

1. Add `isQuestSkill: true` to the skill definition for future quest system integration
2. Optionally add `afterCastDelay: 0` to level data for consistency with other skill definitions

The heal VFX currently broadcasts with `element: 'holy'` which is technically wrong (First Aid is neutral, not holy), but heal VFX are typically shown as green/holy regardless of element. This is an acceptable cosmetic choice and matches player expectations.

**No handler changes needed.**

---

### 3. PLAY DEAD (ID 3) -- Toggle (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 143 (NV_TRICKDEAD) | rAthena `db/pre-re/skill_db.yml` |
| Type | Active / Supportive / **Toggleable** | rAthena YAML (`Toggleable: true`) |
| Max Level | 1 | All sources |
| Target | Self | All sources |
| SP Cost | **5** | rAthena YAML, Divine Pride (143), pservero |
| Cast Time | 0 ms | rAthena YAML, pservero |
| After-Cast Delay | 0 ms | rAthena YAML (no AfterCastActDelay), pservero |
| Cooldown | 0 ms | rAthena YAML (no Cooldown field), pservero |
| Duration | **INFINITE** (600000ms / 10 min in rAthena as safety, but `tick = INFINITE_TICK` in source) | rAthena YAML `Duration1: 600000`, status.cpp sets `tick = INFINITE_TICK` |
| Element | Neutral | rAthena YAML (no Element field) |
| Range | 0 (self) | pservero |
| Prerequisites (skill tree) | None | rAthena `db/pre-re/skill_tree.yml` |
| Quest Skill | Yes (`IsQuest: true` flag) | rAthena YAML |
| Quest: Job Level Req | 7 | RateMyServer, iRO Wiki Classic |
| Quest: Skill Req | First Aid (must have First Aid before learning Play Dead) | iRO Wiki Classic, RateMyServer |
| Quest: NPC | Prontera Chivalry Member (prt_in 73,87) / Instructor Argos (Training Grounds) | iRO Wiki, RateMyServer |
| Status Applied | SC_TRICKDEAD (status ID 47) | rAthena `status.hpp` |
| Excluded from Inheritance | Yes (`Exclude: true` in skill tree) | rAthena `skill_tree.yml` |
| DamageFlags | NoDamage: true | rAthena YAML |

**Status Effect Behaviors (SC_TRICKDEAD from rAthena source):**

| Behavior | Detail | Source |
|----------|--------|--------|
| Visual | Sets `dead_sit = 1` (corpse lying-down animation) | `status.cpp` line 11572 |
| Movement | **BLOCKED** -- `sc->cant.move` prevents walking | `status.cpp` SCF flags, `unit.cpp` `unit_can_move()` |
| Auto-Attack | **BLOCKED** -- normal attacks cannot target player in TrickDead | `status.cpp` line 2219: `if(!skill_id && tsc->getSCE(SC_TRICKDEAD)) return false` |
| Skill Use | **BLOCKED** -- all skills blocked except NV_TRICKDEAD itself | `status.cpp` line 2115: `sc->getSCE(SC_TRICKDEAD) && skill_id != NV_TRICKDEAD` |
| Item Use | **BLOCKED** (implied by dead_sit state) | rAthena client behavior |
| HP Regen | **BLOCKED** | `status.cpp` line 5407: `sc->getSCE(SC_TRICKDEAD)` sets `regen->flag = RGN_NONE` |
| SP Regen | **BLOCKED** | Same line -- `RGN_NONE` blocks both HP and SP regen |
| Monster Aggro | **IMMUNE** -- all monsters ignore player, including bosses/MVPs | iRO Wiki: "no monsters are able to detect a player in this state" |
| PvP Damage | **IMMUNE** -- players in PvP cannot damage | iRO Wiki |
| Toggle Off | Recasting NV_TRICKDEAD cancels the status | All sources |
| Dispel | Can be removed by Sage's Dispell | iRO Wiki, rAthena |
| Provoke | Can be removed by Swordsman's Provoke | iRO Wiki, rAthena |
| Bleeding | Deactivated upon each HP drain tick from Bleeding | iRO Wiki |
| AoE Status | Hammer Fall, Venom Dust, Bloody Party can affect player | iRO Wiki |
| Tarot | Tarot Card of Fate can damage/kill player | iRO Wiki |
| Class Restriction | Becomes unusable upon relog/mapchange after job change from Novice | iRO Wiki Classic |
| Dance Break | Removes SC_DANCING if active | `status_change.txt`: "Remove SC_DANCING" |

#### Current Implementation Status: NOT IMPLEMENTED

**Skill Definition:**
- Name: **CORRECT** -- `play_dead`
- Type: **CORRECT** -- `type: 'toggle'`
- Max Level: **CORRECT** -- `maxLevel: 1`
- Target: **CORRECT** -- `targetType: 'self'`
- SP Cost: **CORRECT** -- `spCost: 5` matches rAthena YAML
- Prerequisite: **WRONG** -- Our def says `prerequisites: [{ skillId: 1, level: 1 }]` (Basic Skill Lv1), but rAthena skill tree has NO prerequisite skills. The prerequisite is a *quest* requirement (Job Level 7 + First Aid known), NOT a skill tree dependency.

**Handler:** **COMPLETELY MISSING** -- No `play_dead` handler exists in `index.js`. The skill is defined in `ro_skill_data.js` but searching for `play_dead` in `index.js` returns zero results. Attempting to use Play Dead would fall through all handler checks and do nothing (or hit a generic "skill not handled" path).

#### Gaps Found

| Gap | Priority | Effort | Notes |
|-----|----------|--------|-------|
| No server handler exists | **CRITICAL** | Medium | Must implement full toggle handler with TrickDead status |
| SP cost in definition is correct (5) but prerequisite is wrong | HIGH | Trivial | Remove `prerequisites: [{ skillId: 1, level: 1 }]`, replace with `[]` |
| No TrickDead status effect | HIGH | Medium | Must block movement, attacks, skills, items, HP/SP regen |
| No monster deaggro mechanic | HIGH | Medium | All enemies must drop aggro on player when Play Dead activates |
| No protection from auto-attacks while in TrickDead | HIGH | Small | Combat tick must skip targets with TrickDead status |
| No break conditions (Dispel, Provoke, Bleeding) | MEDIUM | Small | Add removal hooks in relevant handlers |
| Not classified as quest skill | LOW | Trivial | Add `isQuestSkill: true` |
| Missing `afterCastDelay: 0` in level data | LOW | Trivial | Consistency |
| Class restriction after job change not enforced | DEFERRED | N/A | Very niche -- Super Novice edge case |
| Duration should be infinite (currently 0 in def) | HIGH | Trivial | Set to very large value or handle as permanent until toggled |

#### Implementation Notes

Play Dead requires a complete handler implementation. Model it after the existing Hiding handler (ID 503) but with these key differences:

**1. Toggle Handler (in `skill:use` event):**
```javascript
if (skill.name === 'play_dead') {
    if (hasBuff(player, 'play_dead')) {
        // Toggle OFF
        removeBuff(player, 'play_dead');
        broadcastToZone(zone, 'skill:buff_removed', {
            targetId: characterId, isEnemy: false,
            buffName: 'play_dead', reason: 'cancel'
        });
        socket.emit('skill:used', { skillId, skillName: skill.displayName,
            level: learnedLevel, spCost: 0,
            remainingMana: player.mana, maxMana: player.maxMana });
        return;
    }
    // Toggle ON
    player.mana = Math.max(0, player.mana - spCost);
    // Stop any auto-attack in progress
    autoAttackState.delete(characterId);
    // Apply TrickDead buff
    applyBuff(player, {
        skillId, name: 'play_dead',
        casterId: characterId, casterName: player.characterName,
        preventsMovement: true,
        preventsAttack: true,
        preventsCasting: true,
        preventsItems: true,
        blocksHPRegen: true,
        blocksSPRegen: true,
        isPlayDead: true,
        duration: 0  // Infinite -- only removed by toggle or break conditions
    });
    // Force all enemies to drop aggro on this player
    for (const [enemyId, enemy] of activeEnemies) {
        if (enemy.aggroTarget === characterId) {
            enemy.aggroTarget = null;
            enemy.state = 'idle';
        }
    }
    broadcastToZone(zone, 'skill:buff_applied', {
        targetId: characterId, targetName: player.characterName,
        isEnemy: false, casterId: characterId,
        casterName: player.characterName,
        skillId, buffName: 'Play Dead', duration: 0,
        effects: { isPlayDead: true }
    });
    socket.emit('skill:used', { ... });
    socket.emit('combat:health_update', { ... });
    return;
}
```

**2. Combat Tick Protection:**
In the auto-attack combat tick loop, add a check:
```javascript
// Skip targets that are playing dead
if (hasBuff(targetPlayer, 'play_dead')) continue;
```

**3. Enemy AI Deaggro:**
In the enemy AI tick, add:
```javascript
// Monsters cannot see players in Play Dead
if (hasBuff(targetPlayer, 'play_dead')) {
    // Drop aggro, return to idle
}
```

**4. Break Conditions:**
- Dispel handler: Add `play_dead` to list of buffs removed by Dispel
- Provoke handler: Add `play_dead` removal when Provoked
- Bleeding tick: If player has `play_dead` buff and takes Bleeding damage, remove `play_dead`

**5. Skill/Action Blocking:**
The existing `getCombinedModifiers()` system already supports `preventsAttack`, `preventsCasting`, `preventsItems`, `blocksHPRegen`, `blocksSPRegen` flags. Setting these on the buff will automatically block most actions through existing checks.

**6. Movement Blocking:**
Need to add `preventsMovement` check to the `player:position` handler to reject position updates while Play Dead is active. This prevents client-side movement exploits.

---

## New Systems Required

### 1. Play Dead Status (TrickDead)

**Scope:** Toggle buff that blocks all player actions, deaggros all monsters, and prevents HP/SP regen.

**Implementation approach:** Use the existing buff system (`applyBuff`/`hasBuff`/`removeBuff`) with appropriate flags. The buff should have `duration: 0` (infinite) and only be removed by:
- Recasting Play Dead (toggle off)
- Sage's Dispel
- Swordsman's Provoke
- Bleeding HP drain tick
- Disconnect / zone change (safety net)

**New flag needed:** `isPlayDead: true` -- distinguished from `isHidden` because Play Dead is stronger than Hiding (bosses cannot detect, no SP drain, but player cannot use items or recover HP/SP).

### 2. Quest Skill Infrastructure (DEFERRED)

**Scope:** First Aid and Play Dead should be learned through quests, not skill point allocation.

**Current state:** Our skill system learns skills via `skill:learn` which deducts skill points. Quest skills should be granted by quest completion NPCs without costing skill points.

**Recommendation:** Defer this until the quest/NPC system is implemented. For now, allow players to learn First Aid and Play Dead via skill points as a temporary simplification. Mark both with `isQuestSkill: true` in the definition so the UI can eventually distinguish them.

### 3. Movement Blocking for Buffs

**Scope:** The position broadcast system and `player:position` handler need to respect `preventsMovement` buff flag.

**Implementation:** In the `player:position` socket handler, check:
```javascript
const mods = getCombinedModifiers(player);
if (mods.preventsMovement) {
    // Reject position update -- player cannot move
    return;
}
```

This also benefits future status effects like Stun, Freeze, etc. that block movement.

---

## Skill Definition Corrections

### Changes needed in `ro_skill_data.js`:

| Skill | Field | Current Value | Correct Value | Reason |
|-------|-------|--------------|---------------|--------|
| Basic Skill (1) | `levels[*].afterCastDelay` | (missing) | `0` | Consistency with other skill defs |
| Basic Skill (1) | `description` | `'Enables basic commands. Required for class change at level 9.'` | `'Enables basic interface commands at each level. Trading (Lv1), Emotes (Lv2), Sitting (Lv3), Chat Rooms (Lv4), Party Join (Lv5), Kafra Storage (Lv6), Party Create (Lv7), Job Change (Lv9).'` | Per-level unlocks documented |
| First Aid (2) | (add field) | (missing) | `isQuestSkill: true` | Mark as quest-learned skill |
| First Aid (2) | `levels[0].afterCastDelay` | (missing) | `0` | Consistency |
| Play Dead (3) | `prerequisites` | `[{ skillId: 1, level: 1 }]` | `[]` | rAthena skill tree has NO skill prerequisites; quest requirement is separate |
| Play Dead (3) | `spCost` | `5` | `5` | Already correct (matches rAthena YAML) |
| Play Dead (3) | (add field) | (missing) | `isQuestSkill: true` | Mark as quest-learned skill |
| Play Dead (3) | `levels[0].afterCastDelay` | (missing) | `0` | Consistency |
| Play Dead (3) | `levels[0].duration` | `0` | `0` | Keep 0 (infinite -- handled as permanent until toggled) |
| Play Dead (3) | `description` | `'Pretend to be dead. Monsters ignore you.'` | `'Feign death. All monsters (including MVPs) ignore you. Cannot move, attack, use skills, or use items. HP/SP regen blocked. Toggle on/off.'` | Complete description |

---

## Implementation Priority

### Phase 1: Critical Fixes (HIGH priority)

**Estimated effort:** ~1 hour

1. **Fix Play Dead prerequisite** in `ro_skill_data.js` -- Change from `[{ skillId: 1, level: 1 }]` to `[]`
2. **Add Basic Skill Lv9 check** to `job:change` handler in `index.js`
3. **Implement Play Dead toggle handler** in `index.js` skill handler section:
   - Toggle on: Apply buff with `preventsMovement`, `preventsAttack`, `preventsCasting`, `preventsItems`, `blocksHPRegen`, `blocksSPRegen`, `isPlayDead` flags
   - Toggle off: Remove buff, emit events
   - Stop auto-attack on activation
   - Force all enemies to drop aggro on player
4. **Add combat tick protection** -- Skip auto-attack targets that have `play_dead` buff
5. **Add enemy AI deaggro** -- Monsters cannot see/target players with Play Dead
6. **Add position blocking** -- Reject `player:position` updates while Play Dead is active

### Phase 2: Break Conditions (MEDIUM priority)

**Estimated effort:** ~30 min

1. Add `play_dead` removal to Dispel handler (if/when Sage Dispel is implemented)
2. Add `play_dead` removal to Provoke handler (already implemented for Swordsman)
3. Add `play_dead` removal on Bleeding HP drain tick

### Phase 3: Metadata & Cosmetic (LOW priority)

**Estimated effort:** ~15 min

1. Add `isQuestSkill: true` to First Aid and Play Dead definitions
2. Update descriptions for all 3 skills
3. Add `afterCastDelay: 0` to level data for consistency

### Phase 4: Deferred Systems

1. Quest skill infrastructure (learn via NPC quest, not skill points)
2. Per-level Basic Skill feature gating (trading, chat rooms, parties, etc.)
3. Sitting mechanic with HP/SP regen doubling at Basic Skill Lv3
4. Play Dead class restriction after job change (Novice-only on relog/mapchange)

---

## Server-Side Reference: Existing Patterns

**Toggle skill pattern (from Hiding handler, line 7606):**
- Check `hasBuff(player, 'hiding')` -- if active, remove buff and return (toggle off, no SP cost)
- If not active, deduct SP, apply buff with flags, broadcast, emit events

**Buff flag pattern (from `getCombinedModifiers()`, line ~650):**
- `preventsAttack`, `preventsCasting`, `preventsItems`, `blocksHPRegen`, `blocksSPRegen` -- all supported
- `isHidden` flag exists for Hiding -- Play Dead needs a distinct `isPlayDead` flag

**Auto-attack skip pattern (from combat tick):**
- Hiding already has a check: `if (hasBuff(player, 'hiding'))` in various places
- Add similar check for `play_dead`

**Enemy AI deaggro pattern (from Hiding):**
- Hidden players are checked via `canSeeHiddenTarget()` function (line 1699)
- Play Dead should add an equivalent check or extend the existing one

---

## Sources

### Primary (rAthena Pre-Renewal Database -- Authoritative)
- rAthena `db/pre-re/skill_db.yml`: https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml
- rAthena `db/pre-re/skill_tree.yml`: https://github.com/rathena/rathena/blob/master/db/pre-re/skill_tree.yml
- rAthena `src/map/status.cpp`: https://github.com/rathena/rathena/blob/master/src/map/status.cpp
- rAthena `src/map/status.hpp`: https://github.com/rathena/rathena/blob/master/src/map/status.hpp
- rAthena `src/map/unit.cpp`: https://github.com/rathena/rathena/blob/master/src/map/unit.cpp
- rAthena `doc/status_change.txt`: https://github.com/rathena/rathena/blob/master/doc/status_change.txt

### Secondary (Community Wikis)
- iRO Wiki - Basic Skill: https://irowiki.org/wiki/Basic_Skill
- iRO Wiki - First Aid: https://irowiki.org/wiki/First_Aid
- iRO Wiki - Play Dead: https://irowiki.org/wiki/Play_Dead
- iRO Wiki Classic - Novice: https://irowiki.org/classic/Novice
- iRO Wiki Classic - Play Dead: https://irowiki.org/classic/Play_Dead
- iRO Wiki Classic - Novice Skill Quest: https://irowiki.org/wiki/Novice_Skill_Quest(Classic)
- iRO Wiki - Novice Skill Quest: https://irowiki.org/wiki/Novice_Skill_Quest

### Tertiary (Cross-reference)
- Divine Pride - First Aid (142): https://www.divine-pride.net/database/skill/142
- Divine Pride - Play Dead (143): https://www.divine-pride.net/database/skill/143
- pservero Pre-Renewal - NV_BASIC: https://db.pservero.com/skill/NV_BASIC
- pservero Pre-Renewal - NV_FIRSTAID: https://pre.pservero.com/skill/NV_FIRSTAID
- pservero Pre-Renewal - NV_TRICKDEAD: https://pre.pservero.com/skill/NV_TRICKDEAD
- RateMyServer - Basic Skill (ID 1): https://ratemyserver.net/index.php?page=skill_db&skid=1
- RateMyServer - First Aid (ID 142): https://ratemyserver.net/index.php?page=skill_db&skid=142
- RateMyServer - Play Dead (ID 143): https://ratemyserver.net/index.php?page=skill_db&skid=143
- RateMyServer - Novice Platinum Quests: https://ratemyserver.net/quest_db.php?type=50000&qid=50001

### SP Cost Discrepancy Resolution
- iRO Wiki says SP Cost = 1 for Play Dead -- **INCORRECT** (wiki error)
- rAthena YAML says SP Cost = 5 -- **AUTHORITATIVE**
- Divine Pride says SP Cost = 5 -- **CONFIRMS rAthena**
- pservero says SP Cost = 5 -- **CONFIRMS rAthena**
- Our definition says SP Cost = 5 -- **ALREADY CORRECT**
