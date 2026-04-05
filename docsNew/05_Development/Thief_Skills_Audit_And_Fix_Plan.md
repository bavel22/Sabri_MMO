# Thief Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14
**Status:** RESEARCH COMPLETE — Ready for implementation
**Scope:** All 10 Thief class skills (IDs 500-509)

---

## Executive Summary

Deep research against iRO Wiki, rAthena pre-renewal database, and RateMyServer reveals **significant gaps** in 8 of 10 Thief skills. Only Improve Dodge (501) is fully correct. Double Attack (500) is mostly correct but missing a HIT bonus. The remaining 8 skills have issues ranging from missing mechanics (Steal doesn't give items, Pick Stone doesn't create items, Hiding has no SP drain) to incorrect formulas (Envenom damage is completely wrong, Throw Stone damage formula is wrong).

**New systems needed:**
1. Hiding SP drain tick (periodic SP deduction while hidden)
2. Steal item selection from monster drop tables
3. Stone item (ID 7049) inventory integration for Pick Stone / Throw Stone
4. Envenom flat ATK bonus (bypasses DEF, added AFTER normal damage)

---

## Skill-by-Skill Analysis

---

### 1. DOUBLE ATTACK (ID 500) — Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | iRO Wiki, rAthena |
| Max Level | 10 | All sources |
| Proc Chance | 5% per level (5%-50%) | iRO Wiki, RateMyServer |
| Weapon | Daggers ONLY (not katars in pre-renewal 1st class) | iRO Wiki |
| Damage | Each hit deals full normal auto-attack damage (2 hits bundled visually) | iRO Wiki |
| HIT Bonus | +1 HIT per level (only during double attack procs) | RateMyServer |
| Critical | Double Attack does NOT crit in pre-renewal | rAthena issue #4460 |
| Scope | Auto-attack ONLY — does not proc on skills | rAthena |
| ASPD | No ASPD modification | All sources |
| Katar Bonus | For Assassin with Katar: off-hand damage bonus (+1+2*SkillLv)% | RateMyServer |

#### Current Implementation Status: MOSTLY CORRECT

- Proc chance: **CORRECT** — `doubleAttackChance = daLv * 5` in `getPassiveSkillBonuses()`
- Dagger-only: **CORRECT** — checks `wType === 'dagger'`
- Auto-attack only: **CORRECT** — proc checked in combat tick, not skill handlers
- Critical bypass: **CORRECT** — Double Attack second hit doesn't independently crit
- Full damage per hit: **CORRECT** — each hit runs full damage pipeline

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing +1 HIT bonus per level during DA procs | LOW | Trivial — add `bonusHIT` to passive bonuses, but only during DA check |
| Missing Katar off-hand damage bonus for Assassin | DEFERRED | Phase 6+ (Assassin 2nd class) |

#### Implementation Notes

The HIT bonus is minor and only applies during the double attack itself (not all attacks). In rAthena, this is a passive bonus that's always active when you have the skill learned and are using daggers. For simplicity, we can add it as a flat `bonusHIT += daLv` in `getPassiveSkillBonuses()` when `wType === 'dagger'`. This slightly overbuffs (should only apply during DA procs) but matches common private server behavior.

---

### 2. IMPROVE DODGE (ID 501) — Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| FLEE Bonus (1st class) | +3 per level (+3 to +30) | iRO Wiki, RateMyServer |
| FLEE Bonus (2nd class) | +4 per level (+4 to +40) | iRO Wiki, RateMyServer |
| Perfect Dodge | NO bonus at any level | iRO Wiki (confirmed absent) |
| MSPD Bonus | Assassin class only, slight movement speed boost | iRO Wiki |

#### Current Implementation Status: CORRECT

- FLEE bonus: **CORRECT** — `bonusFLEE += idLv * 3`
- 2nd class scaling: Not yet needed (no 2nd class implementation)
- No Perfect Dodge: **CORRECT** — not implemented, correctly absent

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing 2nd class FLEE scaling (+4/level instead of +3) | DEFERRED | Phase 6+ (2nd class) |
| Missing Assassin movement speed bonus | DEFERRED | Phase 6+ (Assassin 2nd class) |

**No changes needed for current phase.**

---

### 3. STEAL (ID 502) — Active

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target (enemy only) | All sources |
| Max Level | 10 | All sources |
| SP Cost | 10 (all levels) | iRO Wiki |
| Cast Time | 0 (instant) | iRO Wiki |
| After-Cast Delay | 1 second | iRO Wiki |
| Range | Melee (1 cell = ~50 UE units) | iRO Wiki |
| Success Formula | Base rate: `4 + 6*SkillLv`% → Lv1=10%, Lv10=64% | iRO Wiki |
| Adjusted Formula | `DropRatio * (DEX + 3*SkillLv - MonsterDEX + 10) / 100` | iRO Wiki |
| Boss Immunity | Boss monsters CANNOT be stolen from | iRO Wiki |
| One-time | Can only steal once per monster (success or fail doesn't matter — success locks it) | iRO Wiki |
| Item Source | Steals from monster's NORMAL drop table (same items it drops on death) | iRO Wiki |
| Kill Drops | Successful steal does NOT affect what drops when monster dies | iRO Wiki |
| Result | Stolen item goes directly to player's inventory | All sources |

**How the adjusted formula works (rAthena):**
1. Roll overall success: `Base_Success_Rate + (UserDEX - MonsterDEX) / 2`
2. If overall roll succeeds, select which item: iterate through monster's drop list, each item has `AdjustedDropRatio = DropRatio * (DEX + 3*SkillLv - MonsterDEX + 10) / 100`
3. Roll against each item's adjusted rate (top to bottom through drop table)
4. First item that passes its roll is stolen
5. If no item passes, steal "succeeds" but gives nothing (rare edge case)

#### Current Implementation Status: MAJOR GAPS

- Success formula: **PARTIALLY WRONG** — uses `effectVal + (DEX/2) - (enemyLevel/3)` instead of canonical formula
- Item acquisition: **NOT IMPLEMENTED** — just shows "Stole an item!" message, no actual item given
- Boss immunity: **NOT CHECKED** — missing `modeFlags.boss` check
- One-time lock: **PARTIALLY CORRECT** — tracks `enemy.stolenBy` Set, but only adds on success (should probably still allow retry on failure per some servers, but locking on success is correct)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No actual item given to player** | HIGH | Medium — need to roll against monster drop table, insert into `character_inventory` |
| Wrong success formula | MEDIUM | Easy — replace with canonical formula |
| No boss immunity check | MEDIUM | Trivial — check `enemy.modeFlags?.boss` |
| effectValue in skill definition wrong | LOW | Fix to match base success rate per level |

#### Implementation Notes

**Item steal requires:**
1. Monster templates need drop tables accessible at runtime (currently in `ro_monster_templates.js` — verify drop data exists)
2. Roll against adjusted drop ratios per item
3. Insert stolen item into `character_inventory` via DB query
4. Emit `inventory:data` or `inventory:item_added` to client with new item
5. Weight check: add item weight to `player.currentWeight`
6. If overweight (100%+), steal succeeds but item dropped on ground (or blocked)

**Simplified approach (recommended for now):**
Since we may not have full drop tables per monster yet, implement the success formula correctly, and on success give a random common loot item appropriate to the monster's level. Full drop table integration can come later.

---

### 4. HIDING (ID 503) — Toggle

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Toggle | All sources |
| Max Level | 10 | All sources |
| SP Cost (activation) | 10 SP | iRO Wiki, rAthena |
| SP Drain Rate | 1 SP every (4+SkillLv) seconds | iRO Wiki, RateMyServer |
| Duration | 30s × SkillLv (Lv1=30s, Lv10=300s) | iRO Wiki |
| Movement | **CANNOT MOVE** (unless Rogue with Stalk skill) | iRO Wiki, RateMyServer |
| Items | **CANNOT USE ITEMS** while hidden | iRO Wiki |
| Attacking | **CANNOT ATTACK** while hidden | RateMyServer |
| Skills | **CANNOT USE ANY SKILL** (except Hiding toggle to cancel) | RateMyServer |
| HP/SP Regen | **NO REGENERATION** while hidden | RateMyServer |

**SP Drain Table:**

| Level | Duration | SP Drain Interval |
|-------|----------|-------------------|
| 1 | 30s | 1 SP / 5s |
| 2 | 60s | 1 SP / 6s |
| 3 | 90s | 1 SP / 7s |
| 4 | 120s | 1 SP / 8s |
| 5 | 150s | 1 SP / 9s |
| 6 | 180s | 1 SP / 10s |
| 7 | 210s | 1 SP / 11s |
| 8 | 240s | 1 SP / 12s |
| 9 | 270s | 1 SP / 13s |
| 10 | 300s | 1 SP / 14s |

**Break Conditions:**
1. Using the skill again (manual cancel) — no SP refund
2. Duration expires
3. SP reaches 0 (from drain)
4. Detected by Sight, Ruwach, or Detecting skills (within AoE)
5. Detected by Insect, Demon, or Boss-type monsters (they can see hidden players)
6. Taking damage from detection attacks (Ruwach deals Holy damage to hidden targets)
7. Bleeding status effect drains break hiding on each HP drain tick

**Does NOT break on:**
- Normal damage (enemies that can't detect hidden won't target you)
- Idle state

#### Current Implementation Status: SIGNIFICANT GAPS

- Toggle behavior: **CORRECT** — casts again to cancel
- Activation SP cost: **CORRECT** — 10 SP
- Duration: **CORRECT** — `(level+1)*30000` ms
- Enemy AI detection: **CORRECT** — `isHidden` flag checked in AI, detector enemies bypass
- Breaks on offensive skill use: **CORRECT** — checked at top of `skill:use` handler

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No SP drain while hidden** | HIGH | Medium — need periodic tick that drains 1 SP per interval |
| **No break when SP reaches 0** | HIGH | Part of SP drain implementation |
| **No movement prevention** | HIGH | Medium — need to reject `player:position` updates while hidden |
| **No item use prevention** | MEDIUM | Easy — check `isHidden` in `inventory:use` handler |
| **No HP/SP regen block while hidden** | MEDIUM | Easy — check `isHidden` in regen tick |
| **No Bleeding break on HP drain** | LOW | Easy — check in bleeding tick |
| Auto-attack while hidden not blocked | MEDIUM | Easy — check `isHidden` in `combat:attack` handler |
| No Ruwach/Sight forced reveal (server-side) | MEDIUM | Medium — AoE reveal check when Sight/Ruwach active |

#### Implementation Notes

**SP Drain System (new):**
Need a periodic tick (can piggyback on existing 1s buff expiry tick) that:
1. Checks all connected players for active `hiding` buff
2. Tracks `lastDrainTime` on the buff object
3. Every `(4 + skillLevel)` seconds, drains 1 SP
4. If `player.mana <= 0` after drain: remove hiding buff, broadcast `skill:buff_removed`

**Movement Prevention:**
In the `player:position` handler, check `hasBuff(player, 'hiding')`. If hidden:
- Reject position update
- Emit `player:position_rejected` with reason `'hidden'`
(Similar to existing CC movement lock pattern)

**Auto-attack Prevention:**
In `combat:attack` handler, check `isHidden` modifier from `getCombinedModifiers(player)`. If hidden:
- Emit `combat:error` with message "Cannot attack while hidden"
- Do NOT auto-cancel hiding (player must manually cancel or it expires)

---

### 5. ENVENOM (ID 504) — Active

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, melee single target | All sources |
| Max Level | 10 | All sources |
| SP Cost | 12 (all levels) | iRO Wiki, RateMyServer, rAthena |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 0 | rAthena |
| Range | 2 cells (~100 UE units) | rAthena |
| Element | Poison | All sources |
| Damage Formula | **Normal melee attack + flat (15 × SkillLv) ATK bonus** | iRO Wiki, RateMyServer |
| ATK Bonus Behavior | The +15/lv bonus is **unmodified by Armor and VIT Def** (bypasses DEF) | RateMyServer (pre-renewal) |
| ATK Bonus Always Hits | The bonus damage is "always inflicted, whether your character lands a normal hit or not" | RateMyServer |
| Poison Chance | `(10 + 4 × SkillLv)%` | iRO Wiki, RateMyServer |
| Poison Duration | 60 seconds (pre-renewal) | RateMyServer |
| Poison DEF Reduction | -25% physical DEF on poisoned target | All sources |
| Poison HP Drain | 3% MaxHP per 3 seconds (floor 25% HP) | All sources |
| Boss Immunity | Bosses cannot be poisoned | iRO Wiki |
| Undead Immunity | Undead cannot be poisoned | RateMyServer |

**Envenom Level Data:**

| Level | ATK Bonus | Poison Chance | SP Cost |
|-------|-----------|---------------|---------|
| 1 | +15 | 14% | 12 |
| 2 | +30 | 18% | 12 |
| 3 | +45 | 22% | 12 |
| 4 | +60 | 26% | 12 |
| 5 | +75 | 30% | 12 |
| 6 | +90 | 34% | 12 |
| 7 | +105 | 38% | 12 |
| 8 | +120 | 42% | 12 |
| 9 | +135 | 46% | 12 |
| 10 | +150 | 50% | 12 |

#### Current Implementation Status: FUNDAMENTALLY WRONG

**Critical bug:** The current implementation passes `effectVal = 15*(level)` as the `skillMultiplier` to `calculateSkillDamage()`. This means at Lv1, Envenom deals 15% ATK damage instead of 100% ATK + 15 flat bonus. At Lv10, it deals 150% ATK instead of 100% ATK + 150 flat bonus. The damage model is completely wrong.

**What it should be:**
1. Execute a normal melee attack (100% ATK multiplier, uses weapon element BUT forced to Poison by skill)
2. ADD a flat `15 * SkillLv` damage that bypasses all DEF
3. The flat bonus hits even if the main attack misses
4. Apply poison status with `(10 + 4*SkillLv)%` chance

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Damage formula completely wrong** (using effectVal as skill multiplier) | CRITICAL | Medium — need custom handler instead of `executePhysicalSkillOnEnemy` |
| **Flat ATK bonus not bypassing DEF** | CRITICAL | Part of damage rewrite |
| **Flat bonus should hit even on miss** | HIGH | Part of damage rewrite |
| Poison chance formula wrong (`15 + learnedLevel * 3` vs canonical `10 + 4*SkillLv`) | MEDIUM | Easy fix |
| Poison duration not set (using status effect default vs 60s) | LOW | Easy — pass explicit 60000ms |
| Cooldown should be 0 (currently 500ms in skill def) | LOW | Easy fix in `ro_skill_data.js` |
| Missing Undead immunity for poison | LOW | Add undead race check |

#### Implementation Notes

**Envenom needs a custom handler** (cannot use `executePhysicalSkillOnEnemy` as-is):

```
1. Run calculatePhysicalDamage() with skillMultiplier=100 (normal attack), forceElement='poison'
2. Add flat bonus: flatBonus = 15 * learnedLevel
3. If main attack hits: totalDamage = result.damage + flatBonus
4. If main attack misses: totalDamage = flatBonus (the bonus always hits)
5. Apply totalDamage to target
6. Roll poison: chance = 10 + 4 * learnedLevel
7. Check boss/undead immunity before applying poison
```

---

### 6. DETOXIFY (ID 505) — Active (Supportive)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, supportive | All sources |
| Max Level | 1 | All sources |
| SP Cost | 10 | iRO Wiki, rAthena |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Range | 9 cells (~450 UE units) | iRO Wiki, rAthena |
| Target | "Any entity" / "Friend" — can target **self or any friendly player** | iRO Wiki, rAthena |
| Effect | Removes **Poison** status effect ONLY | All sources |
| Prerequisites | Envenom Lv3 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- Self-targeting: **CORRECT** — defaults to `player` if no targetId
- Other player targeting: **CORRECT** — `connectedPlayers.get(targetId)`
- Removes poison: **CORRECT** — `cleanseStatusEffects(detoxTarget, ['poison'])`
- Range check: **CORRECT** — checks distance for other players

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Cooldown should be 0 (currently 500ms in skill def) | LOW | Easy fix |
| Range in skill def is 450, canonical is ~450 (9 cells × 50) | — | Already correct |
| Can currently target ANY player, not just party — this is actually correct per RO Classic | — | No change needed |

**Detoxify is essentially working correctly.** Only minor skill definition tweaks needed.

---

### 7. SAND ATTACK (ID 506) — Active (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, melee single target | All sources |
| Max Level | 1 | All sources |
| SP Cost | 9 | All sources |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Range | Melee (1 cell) | rAthena |
| Element | Earth | All sources |
| Damage | **130% ATK** (rAthena, RateMyServer) — some sources say 125% (iRO Wiki Classic) | Conflict |
| Blind Chance | **20%** (RateMyServer pre-renewal) — some sources say 15% (iRO Wiki Classic) | Conflict |
| Blind Duration | 30 seconds (pre-renewal) | RateMyServer |

**Source conflict resolution:**
- rAthena pre-renewal DB says 130% damage — this is the authoritative source code
- RateMyServer pre-renewal says 130% damage, 20% blind — aligns with rAthena
- iRO Wiki Classic says 125% damage, 15% blind — likely outdated or different server version
- **Use rAthena values: 130% ATK, 20% blind chance** (RateMyServer confirms)

#### Current Implementation Status: MOSTLY CORRECT

- Damage multiplier: **CORRECT** — `effectValue: 130` in skill def, passed as skill multiplier
- Element: **CORRECT** — Earth via `forceElement: 'earth'`
- Uses `executePhysicalSkillOnEnemy`: **CORRECT** — proper physical skill pipeline

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Blind chance formula wrong: uses `20 + learnedLevel * 5` (=25%) vs canonical 20% | MEDIUM | Easy — change to flat 20 base chance |
| Cooldown 1000ms in skill def, canonical is 0 | LOW | Easy fix |
| Blind duration not explicitly set (relies on status effect default) | LOW | Verify default matches 30s |

---

### 8. BACK SLIDE (ID 507) — Active (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self-targeted | All sources |
| Max Level | 1 | All sources |
| SP Cost | 7 | All sources |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 0 | rAthena |
| Distance | **5 cells backward** (relative to facing direction) | iRO Wiki, RateMyServer |
| Direction Tricks | Can use `/bingbing` and `/bangbang` to face different direction first | iRO Wiki |
| WoE | Disabled in WoE (varies by server — enabled on iRO) | iRO Wiki |
| Obstacles | Blocked by unwalkable cells (stops at obstacle) | Common knowledge |

**Distance in UE units:** 5 cells × 50 UE units/cell = **250 UE units** — this matches current implementation.

#### Current Implementation Status: MOSTLY CORRECT

- Distance: **CORRECT** — 250 UE units
- Direction: **CORRECT** — uses `player.lastDirX/lastDirY`
- SP cost: **CORRECT** — 7 SP
- Uses `player:teleport`: **ACCEPTABLE** — instant movement

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| After-cast delay 500ms in skill def, canonical is 0 | MEDIUM | Easy fix — should be 0 for instant use |
| No obstacle/wall collision check | LOW | Medium — would need server-side walkability check |
| `player:teleport` doesn't update server position cache | MEDIUM | Need to update position in DB/cache after teleport |
| Should break auto-attack state on use | LOW | Check if `autoAttackState` is cleared |

#### Implementation Notes

The back slide position update is important — if the server doesn't know the player's new position, subsequent range checks will fail. Need to update `player.x/y/z` in the position cache after the teleport.

---

### 9. PICK STONE (ID 508 in our system, canonical ID 509) — Active (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self-targeted | All sources |
| Max Level | 1 | All sources |
| SP Cost | 3 | rAthena (our def says 3 — correct) |
| Cast Time | 0.5 seconds | rAthena |
| After-Cast Delay | 0 | rAthena |
| Weight Restriction | Must be **under 50% weight** | RateMyServer pre-renewal, rAthena |
| Result | Adds **1 Stone** (Item ID 7049, Weight: 3) to inventory | rAthena, divine-pride |
| Multiple Stones | Yes, can hold multiple stones (limited by weight) | Common knowledge |

#### Current Implementation Status: NOT IMPLEMENTED (STUB)

The handler just shows a chat message "Picked up 1 Stone." but does NOT:
- Check weight (50% threshold)
- Actually add a Stone item (ID 7049) to inventory
- Update player weight

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Does not add Stone item to inventory** | HIGH | Medium — DB insert into `character_inventory` |
| **No weight check (must be under 50%)** | HIGH | Easy — check `player.currentWeight / player.maxWeight < 0.5` |
| **No weight update after pickup** | HIGH | Part of item add |
| No stone item defined in items table | HIGH | Need to add Stone (ID 7049) to `items` table if not already present |

#### Implementation Notes

**Stone item (ID 7049):**
- Name: Stone
- Type: Misc / Ammunition
- Weight: 3
- NPC Sell Price: 0 (or 1)
- Max stack: 99 (or unlimited)
- Used by: Throw Stone skill

Need to:
1. Verify/add Stone to the `items` database table
2. In handler: check weight < 50%
3. Insert into `character_inventory` (or increment quantity if already held)
4. Update `player.currentWeight += 3`
5. Emit `inventory:data` with updated inventory to client

---

### 10. THROW STONE (ID 508) — Active (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, ranged single target | All sources |
| Max Level | 1 | All sources |
| SP Cost | 2 | rAthena, RateMyServer |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Range | 7 cells (~350 UE units) | rAthena |
| Element | Neutral | rAthena |
| Damage | **Fixed 50** — ignores FLEE (always hits) | rAthena, RateMyServer |
| DEF Interaction | **Does NOT ignore DEF in pre-renewal** (rAthena pre-re says no defense pierce) | rAthena pre-renewal DB |
| Stun Chance | 3% | RateMyServer, rAthena |
| Blind Chance | Mentioned in some sources but not confirmed for pre-renewal | Uncertain |
| Item Consumed | **1 Stone (ID 7049)** per cast | rAthena |
| Stun Duration | Standard stun duration (5 seconds base, reduced by target VIT) | Status effect system |

#### Current Implementation Status: SIGNIFICANT GAPS

- Damage: **WRONG** — currently `50 + STR` (should be flat 50, no STR bonus)
- FLEE bypass: **NOT IMPLEMENTED** — should always hit (ignore FLEE)
- DEF application: **NOT IMPLEMENTED** — currently bypasses all DEF (wrong for pre-renewal)
- Stun chance: **NOT IMPLEMENTED** — no status effect application
- Stone consumption: **NOT IMPLEMENTED** — doesn't check or consume Stone item
- Range: **WRONG** — definition says 700 UE units but canonical is 350 (7 cells × 50)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Damage formula wrong** — includes STR bonus, should be flat 50 | HIGH | Easy fix |
| **No Stone item consumption** | HIGH | Medium — check inventory for Stone, consume 1 |
| **No stun chance (3%)** | MEDIUM | Easy — `applyStatusEffect(source, target, 'stun', 3)` |
| **No FLEE bypass** (should always hit) | MEDIUM | Easy — set `forceHit: true` in damage calc |
| **Range wrong** (700 vs 350 UE units) | MEDIUM | Easy fix in skill def |
| DEF interaction unclear — pre-renewal doesn't pierce DEF | LOW | Apply DEF reduction to the 50 damage |
| SP cost in skill def is 2 (correct) | — | Already correct |

#### Implementation Notes

**Throw Stone needs a custom handler:**

```
1. Check player has Stone item (ID 7049) in inventory
2. If no stone: emit skill:error "You need a Stone to throw!"
3. Consume 1 Stone from inventory (DB update)
4. Update player.currentWeight -= 3
5. Fixed damage = 50 (no STR bonus)
6. Apply DEF reduction: damage = max(1, 50 * (100 - hardDEF) / 100 - softDEF)
7. Always hits (bypass FLEE check)
8. 3% chance to apply Stun
9. Apply damage, broadcast, check death
```

---

## New Systems Required

### System 1: Hiding SP Drain Tick

**Where:** Piggyback on existing 1-second buff expiry tick in `index.js`

**Logic:**
```
For each connected player with 'hiding' buff:
    Get hiding buff object
    Get skill level from buff.skillId -> SKILL_MAP -> player.learnedSkills
    Calculate drain interval: (4 + skillLevel) seconds
    If (now - lastDrainTime >= drainInterval * 1000):
        player.mana = max(0, player.mana - 1)
        Update lastDrainTime
        Emit combat:health_update to player socket
        If player.mana <= 0:
            Remove hiding buff
            Broadcast skill:buff_removed
```

**Buff object needs:** `lastDrainTime`, `hidingLevel` fields added at application time.

### System 2: Steal Item Selection

**Where:** `steal` handler in `index.js`

**Logic:**
```
1. Look up monster template from ro_monster_templates.js
2. Get drops array (if available)
3. For each drop in order:
    adjustedRate = dropRate * (playerDEX + 3*skillLevel - monsterDEX + 10) / 100
    If random(0,10000) < adjustedRate:
        Stolen item = this drop's itemId
        Break
4. If item selected:
    Insert into character_inventory (or increment quantity)
    Update player weight
    Emit inventory:data
    Chat message: "Stole [item name] from [monster]!"
5. If no item selected despite success roll:
    Chat message: "Steal succeeded but found nothing of value."
```

**Dependency:** Monster templates need drop table data. Verify `ro_monster_templates.js` has `drops` arrays.

### System 3: Stone Item Integration (Pick Stone / Throw Stone)

**Where:** Both handlers in `index.js`

**Requirements:**
1. Stone item (ID 7049) in `items` database table
2. Pick Stone: DB insert + weight update + inventory emit
3. Throw Stone: DB check + DB consume + weight update + inventory emit

**Database migration:**
```sql
INSERT INTO items (item_id, name, type, weight, price_buy, price_sell, description)
VALUES (7049, 'Stone', 'misc', 3, 0, 0, 'A simple stone. Can be thrown at enemies.')
ON CONFLICT (item_id) DO NOTHING;
```

### System 4: Envenom Flat ATK Bonus

**Where:** Custom envenom handler in `index.js`

**This is NOT a new "system" — it's a rewrite of the envenom handler to:**
1. Call `calculatePhysicalDamage()` with `skillMultiplier: 100` (normal attack)
2. Add flat `15 * level` bonus that bypasses DEF
3. Apply bonus even on miss
4. Use correct poison chance formula

---

## Skill Definition Corrections (`ro_skill_data.js`)

| Skill | Field | Current | Correct | Notes |
|-------|-------|---------|---------|-------|
| Steal (502) | effectValue formula | `10+i*5` (10-55) | `4+6*(i+1)` (10-64) | Base success rate per iRO Wiki |
| Hiding (503) | cooldown | 500 | 0 | No cooldown in RO Classic |
| Envenom (504) | cooldown | 500 | 0 | No cooldown in rAthena |
| Envenom (504) | range | 150 | 100 | 2 cells = ~100 UE units |
| Envenom (504) | effectValue | `15*(i+1)` | `15*(i+1)` | Keep same but handler must treat as flat bonus, not multiplier |
| Detoxify (505) | cooldown | 500 | 0 | No cooldown in rAthena |
| Sand Attack (506) | cooldown | 1000 | 0 | No cooldown in rAthena |
| Back Slide (507) | afterCastDelay | 500 | 0 | No after-cast delay in rAthena |
| Throw Stone (508) | range | 700 | 350 | 7 cells = ~350 UE units |
| Pick Stone (509) | spCost | 3 | 3 | Already correct |

---

## Implementation Priority

### Phase A: Critical Fixes (Envenom, Throw Stone, Skill Defs)

1. **Fix Envenom handler** — rewrite with correct damage formula (100% ATK + flat bonus bypassing DEF)
2. **Fix Envenom poison chance** — `(10 + 4*level)%` instead of `15 + level*3`
3. **Fix Throw Stone handler** — flat 50 damage (remove STR), add 3% stun, fix range
4. **Fix Sand Attack blind chance** — flat 20% instead of `20 + level*5`
5. **Update skill definitions** — cooldowns, ranges, effectValues per table above

### Phase B: Missing Features (Hiding, Pick Stone, Steal)

6. **Add Hiding SP drain** — 1 SP per (4+level) seconds periodic tick
7. **Add Hiding movement block** — reject position updates while hidden
8. **Add Hiding auto-attack block** — prevent `combat:attack` while hidden
9. **Add Hiding regen block** — no HP/SP regen while hidden
10. **Add Pick Stone item creation** — add Stone (7049) to inventory, check 50% weight
11. **Add Throw Stone item consumption** — check and consume 1 Stone from inventory

### Phase C: Steal Completion

12. **Add Steal item selection** — roll against monster drop table
13. **Add Steal boss immunity** — check `modeFlags.boss`
14. **Add item to inventory on successful steal** — DB insert + weight + emit

### Phase D: Polish

15. **Back Slide position update** — update server-side position cache after teleport
16. **Back Slide auto-attack clear** — clear `autoAttackState` when back sliding
17. **Double Attack HIT bonus** — +1 HIT per level (minor buff)
18. **Hiding item use prevention** — block `inventory:use` while hidden
19. **Hiding bleeding break** — check in bleeding tick

---

## Sources

- [iRO Wiki — Steal](https://irowiki.org/wiki/Steal) — Success formula, base rates, restrictions
- [iRO Wiki Classic — Steal](https://irowiki.org/classic/Steal) — Pre-renewal specific data
- [iRO Wiki — Hiding](https://irowiki.org/wiki/Hiding) — SP drain table, break conditions, detection
- [iRO Wiki — Envenom](https://irowiki.org/wiki/Envenom) — ATK bonus, poison chance table
- [iRO Wiki — Sand Attack](https://irowiki.org/wiki/Sand_Attack) — Damage, blind mechanics
- [iRO Wiki Classic — Sand Attack](https://irowiki.org/classic/Sand_Attack) — Pre-renewal damage (125%)
- [iRO Wiki — Back Slide](https://irowiki.org/wiki/Back_Slide) — Distance, WoE restriction
- [iRO Wiki — Double Attack](https://irowiki.org/wiki/Double_Attack) — Proc rates, crit interaction
- [iRO Wiki — Improve Dodge](https://irowiki.org/wiki/Improve_Dodge) — FLEE table, 2nd class scaling
- [iRO Wiki — Detoxify](https://irowiki.org/wiki/Detoxify) — Range, targeting, SP cost
- [rAthena Pre-RE DB — Stone Fling](https://pre.pservero.com/skill/TF_THROWSTONE) — Fixed 50 damage, stone consumption, stun chance
- [rAthena Pre-RE DB — Envenom](https://pre.pservero.com/skill/TF_POISON) — SP cost, range, timing
- [rAthena Pre-RE DB — Hiding](https://pre.pservero.com/skill/TF_HIDING) — SP cost, timing
- [rAthena Pre-RE DB — Detoxify](https://pre.pservero.com/skill/TF_DETOXIFY) — Range 9 cells, target: friend
- [rAthena Pre-RE DB — Back Slide](https://pre.pservero.com/skill/TF_BACKSLIDING) — 5 cells, 7 SP
- [rAthena Pre-RE DB — Pick Stone](https://pre.pservero.com/skill/TF_PICKSTONE) — 3 SP, 50% weight
- [rAthena Pre-RE DB — Sand Attack](https://pre.pservero.com/skill/TF_SPRINKLESAND) — 130% damage, earth element
- [rAthena GitHub Issue #4460](https://github.com/rathena/rathena/issues/4460) — Double Attack should NOT crit in pre-renewal
- [RateMyServer — Envenom](https://ratemyserver.net/index.php?page=skill_db&skid=52) — ATK bonus bypasses DEF
- [RateMyServer — Hiding](https://ratemyserver.net/index.php?page=skill_db&skid=51) — SP drain table
- [RateMyServer — Sand Attack](https://ratemyserver.net/index.php?page=skill_db&skid=149) — 130% ATK, 20% blind
- [RateMyServer — Double Attack](https://ratemyserver.net/index.php?page=skill_db&skid=48) — HIT bonus per level
- [RateMyServer — Back Slide](https://ratemyserver.net/index.php?page=skill_db&skid=150) — 5 cells, 7 SP
- [RateMyServer — Pick Stone](https://ratemyserver.net/index.php?page=skill_db&skid=151) — Weight restriction
- [Divine Pride — Stone Item](https://www.divine-pride.net/database/item/7049/stone) — Item ID 7049, Weight 3
