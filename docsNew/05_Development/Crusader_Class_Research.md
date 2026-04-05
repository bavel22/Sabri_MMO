# Crusader Class — Complete Pre-Renewal Research

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Crusader_Skills_Audit](Crusader_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Sources:** iRO Wiki Classic, RateMyServer Skill DB, rAthena pre-renewal database, `RagnaCloneDocs/03_Skills_Complete.md`

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [Skill Tree (Prerequisites)](#skill-tree-prerequisites)
3. [Complete Skill Reference](#complete-skill-reference)
   - [Passives](#passives)
   - [Offensive Skills](#offensive-skills)
   - [Supportive/Buff Skills](#supportivebuff-skills)
   - [Quest Skill](#quest-skill)
4. [Current Implementation Status](#current-implementation-status)
5. [New Systems Required](#new-systems-required)
6. [Implementation Priority](#implementation-priority)
7. [Integration Points](#integration-points)
8. [Constants and Data Tables](#constants-and-data-tables)

---

## Class Overview

| Property | Value |
|----------|-------|
| Full Name | Crusader |
| rAthena Job ID | 14 (Crusader), 4015 (Paladin transcendent) |
| Base Class | Swordsman (inherits all Swordsman skills) |
| Job Levels | 50 |
| Skill Points Available | 49 |
| Total Crusader Skills | 14 class skills + 1 quest skill (Shrink) |
| Transcendent | Paladin (adds 5 more skills) |

### Stat Bonuses (per job level, cumulative at Job 50)

| Stat | Total Bonus |
|------|-------------|
| STR | +7 |
| AGI | +2 |
| VIT | +7 |
| INT | +6 |
| DEX | +3 |
| LUK | +5 |

### Weapons

Crusaders can equip:
- One-handed swords
- Two-handed swords
- One-handed spears
- Two-handed spears
- Maces (shared with Swordsman)
- Shields (in left hand, required for many Crusader skills)

### Shield Dependency

Many Crusader skills REQUIRE a shield to be equipped:
- Auto Guard
- Shield Charge
- Shield Boomerang
- Reflect Shield
- Defender
- Shrink

If the shield is removed (e.g., by Rogue's Divest Shield), these skills immediately deactivate.

### Peco Peco Mount

- Unlocked via Riding (Peco Peco Ride) passive skill
- Requires Endure Lv1 (Swordsman skill, ID 106)
- Benefits: +25% movement speed, +1000 weight capacity, spears deal 100% to Medium size (normally 75%)
- Penalty: ASPD set to 50% (mitigated by Cavalry Mastery)
- Spear Mastery bonus increases from +4/lv to +5/lv while mounted
- Crusader mounts a "Grand Peco" (visual variant)

---

## Skill Tree (Prerequisites)

```
Skill Tree Layout — Crusader (IDs 1300-1313)

Row 0: Faith(1300)   Auto Guard(1301)   Devotion(1306)   Heal(1311)
Row 1: Holy Cross(1302)  Shield Charge(1304)  Reflect Shield(1307)  Cure(1312)
Row 2: Grand Cross(1303)  Shield Boomerang(1305)  Providence(1308)
Row 3: Spear Quicken(1310)  Defender(1309)  Shrink(1313)

Prerequisite Chains:

Faith(1300) ----Lv3----> Holy Cross(1302) ----Lv6----> Grand Cross(1303)
Faith(1300) ----Lv10---> Grand Cross(1303)
Faith(1300) ----Lv5----> Cure(1312)
Faith(1300) ----Lv10---> Heal(1311)

Auto Guard(1301) ----Lv5----> Shield Charge(1304) ----Lv3----> Shield Boomerang(1305)
Auto Guard(1301) ----Lv3----> Reflect Shield(1307)

Shield Boomerang(1305) ----Lv3----> Reflect Shield(1307) [ALSO needed]
Shield Boomerang(1305) ----Lv1----> Defender(1309)

Reflect Shield(1307) ----Lv5----> Devotion(1306)
Grand Cross(1303) ----Lv4----> Devotion(1306)

Demon Bane(413, Acolyte) ----Lv5----> Heal(1311) [cross-class prereq]
Auto Guard(1301) ----Lv5----> Providence(1308) [ALSO uses Divine Protection path]
Heal(1311) ----Lv5----> Providence(1308)

Spear Mastery(700, Knight) ----Lv10----> Spear Quicken(1310) [shares Knight's Spear Mastery]
Endure(106, Swordsman) ----Lv1----> Riding(708, Knight)
Riding(708) ----Lv1----> Cavalry Mastery(709, Knight)
```

### Shared Skills with Knight

Crusaders share these skills with Knight class (same IDs):
- **Spear Mastery** (ID 700): +4 ATK/lv with spears (+5/lv mounted)
- **Riding** (ID 708): Enables Peco Peco mount
- **Cavalry Mastery** (ID 709): Reduces mounted ASPD penalty

These are already defined in `ro_skill_data_2nd.js` under the Knight section. The `CLASS_PROGRESSION` in `ro_skill_data.js` already includes `'crusader': ['novice', 'swordsman', 'crusader']` but does NOT grant access to Knight-specific skills. This is a gap that needs addressing: Crusaders need Spear Mastery, Riding, and Cavalry Mastery but these are currently classId 'knight'.

**Solution:** Either:
1. Add duplicate definitions with classId 'crusader' (messy)
2. Create a `SHARED_SKILLS` mechanism that allows certain skill IDs to be available to multiple classes
3. Add 'knight' to the crusader progression chain for just those 3 skill IDs (not correct, would give all Knight skills)
4. **Best approach:** Change Spear Mastery (700), Riding (708), and Cavalry Mastery (709) to have a `sharedClasses` field, and modify `getAvailableSkills()` to check it

---

## Complete Skill Reference

### Passives

---

#### 1. Faith (ID 1300)

| Property | Value |
|----------|-------|
| rAthena ID | 248 (CR_TRUST) |
| Type | Passive |
| Max Level | 10 |
| Target | None (always active) |
| Prerequisites | None |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Max HP Bonus | +200 | +400 | +600 | +800 | +1000 | +1200 | +1400 | +1600 | +1800 | +2000 |
| Holy Resist | 5% | 10% | 15% | 20% | 25% | 30% | 35% | 40% | 45% | 50% |

**Mechanics:**
- MaxHP bonus is flat, added AFTER VIT calculation: `MaxHP = floor(BaseHP * (1 + VIT*0.01)) + faithBonus`
- Holy Resistance reduces damage from Holy element attacks (Holy Light, Aspersio-enchanted weapons, Silver Arrows, etc.)
- The Holy Resistance formula: `holyDamage = holyDamage * (100 - faithLevel*5) / 100`

**Implementation Notes:**
- Must be integrated into `getPassiveSkillBonuses()` to add `bonusMaxHp` and `holyResist`
- Holy resist needs to be applied during damage calculation when attack element is 'holy'

**Current Code Status:** Definition exists in `ro_skill_data_2nd.js` (ID 1300). No passive handler in `getPassiveSkillBonuses()`.

---

#### 2. Spear Mastery (ID 700, shared with Knight)

| Property | Value |
|----------|-------|
| rAthena ID | 60 (KN_SPEARMASTERY) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK (dismounted) | +4 | +8 | +12 | +16 | +20 | +24 | +28 | +32 | +36 | +40 |
| ATK (mounted) | +5 | +10 | +15 | +20 | +25 | +30 | +35 | +40 | +45 | +50 |

**Mechanics:**
- Mastery ATK is flat bonus added to weapon damage, bypasses armor DEF
- Only applies when wielding one_hand_spear or two_hand_spear
- Mounted bonus requires Riding skill active + actually mounted

**Current Code Status:** Definition exists as Knight skill (ID 700). Handler exists in `getPassiveSkillBonuses()` for Knights. Crusaders cannot currently access it due to class progression chain.

---

#### 3. Cavalry Mastery (ID 709, shared with Knight)

| Property | Value |
|----------|-------|
| rAthena ID | 62 (KN_CAVALIERMASTERY) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Riding Lv1 |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ASPD Recovery | 60% | 70% | 80% | 90% | 100% |

**Mechanics:**
- Without this skill, mounted ASPD is 50% of normal
- At Lv5, ASPD penalty is fully negated
- Formula: `mountedASPD = normalASPD * (0.5 + cavalierMasteryLv * 0.1)`
- At Lv5: `mountedASPD = normalASPD * 1.0` (full recovery)

**Current Code Status:** Definition exists as Knight skill (ID 709). No ASPD handler implementation.

---

### Offensive Skills

---

#### 4. Holy Cross (ID 1302)

| Property | Value |
|----------|-------|
| rAthena ID | 253 (CR_HOLYCROSS) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 2 cells (melee) |
| Element | **Holy** (forced, ignores weapon element) |
| Hits | 2 (damage displayed as one bundle of 2 hits) |
| Prerequisites | Faith Lv3 (existing code says Lv3, iRO Wiki says Lv7 -- **DISCREPANCY, use Lv7**) |

**Damage and SP per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 135 | 170 | 205 | 240 | 275 | 310 | 345 | 380 | 415 | 450 |
| SP Cost | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
| Blind% | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

**Mechanics:**
- Damage is dealt as **2 hits** bundled into one visual attack (total damage = ATK% * 2 hits / 2 per hit)
- In practice, the total ATK% is applied once but split into 2 visual hits (each hit = ATK%/2)
- **Two-handed weapon bonus**: Damage is **doubled** when using a two-handed spear (Pre-Renewal: total ATK% becomes 270-900%)
- Element is always **Holy**, regardless of weapon element or endow
- Blind status chance: `3 * skillLevel`%
- Ignores size modifications (no weapon size penalty)
- No cast time, no after-cast delay in pre-renewal (instant cast)

**Implementation Notes:**
- `skillElement: 'holy'` must be forced
- Two-handed spear check: `if (weaponType === 'two_hand_spear') multiplier *= 2`
- Blind status application after damage
- Size penalty bypass via `cardNoSizeFix: true` or a dedicated flag

**Current Code Status:** Definition exists (ID 1302). No active handler in `index.js`.

---

#### 5. Grand Cross (ID 1303)

| Property | Value |
|----------|-------|
| rAthena ID | 254 (CR_GRANDCROSS) |
| Type | Offensive (Hybrid Physical+Magical) |
| Max Level | 10 |
| Target | Self-centered AoE |
| AoE Shape | **Cross-shaped** (5x5 cross pattern around caster) |
| Element | **Holy** |
| Cast Time | 3 seconds (uninterruptible -- cannot be broken by damage) |
| After-Cast Delay | 1.5 seconds |
| Cooldown | 1 second |
| HP Cost | **20% of current HP** per cast |
| Prerequisites | Holy Cross Lv6, Faith Lv10 |

**SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP Cost | 37 | 44 | 51 | 58 | 65 | 72 | 79 | 86 | 93 | 100 |

**Damage Formula (Pre-Renewal):**

```
Per-Hit Damage = floor((ATK + MATK) * (100 + 40*SkillLv) / 100) * HolyElementMod
```

Where:
- ATK = full physical ATK calculation (StatusATK + WeaponATK + PassiveATK with all card/buff modifiers)
- MATK = random between MATKmin and MATKmax
- The skill hits **1-3 times** depending on enemy position relative to caster (enemies on same cell as caster can receive fewer hits)
- HolyElementMod = element effectiveness of Holy vs target's element

**Damage % table (combined ATK+MATK percentage):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| (ATK+MATK)% | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |

**Self-Damage Formula (Pre-Renewal):**

```
SelfDamage = floor(GrandCrossDamage / 2) * HolyElementModOnCaster
```

- Caster takes approximately **half** the damage they deal
- Faith's Holy Resistance reduces self-damage: with Faith Lv10 (50% holy resist), self-damage is halved again to ~25% of dealt damage
- The HP cost (20% of current HP) is applied BEFORE the skill activates
- Self-damage is applied AFTER the skill effect resolves

**AoE Cross Pattern (Pre-Renewal):**

```
    X
    X
  X X X
    X
    X
```

Where the center is the caster's position. The cross extends 2 cells in each cardinal direction (up, down, left, right) but NOT diagonals.

Cell pattern (caster at 0,0):
```
Affected cells: (0,-2), (0,-1), (-2,0), (-1,0), (0,0), (1,0), (2,0), (0,1), (0,2)
Total: 9 cells in cross pattern (not 5x5 square)
```

**Special Mechanics:**
- Cast is **uninterruptible** (immune to flinch/knockback during cast)
- Caster is **immobilized** during the 0.9-second effect animation
- Defense applies as **hard defense only** (flat subtraction, not percentage)
- Affected by **long-range modifiers** (Archer Skeleton Card, etc.)
- Blind chance on Undead/Demon monsters: `3 * skillLevel`%
- Golden Thief Bug Card (GTB) on shield disables Grand Cross entirely
- INT scaling is very strong because MATK scales with INT^2

**Implementation Notes:**
- This is a **hybrid damage** skill -- needs a new `calculateHybridDamage()` function or combined ATK+MATK calculation
- Self-damage must be applied after AoE damage
- Cross-shaped AoE needs custom cell selection (not standard circular/square)
- HP cost (20% current HP) deducted before cast starts
- Must check `energyCoatActive` interaction (Grand Cross self-damage should not trigger Energy Coat)
- Cast interruption immunity flag needed

**Current Code Status:** Definition exists (ID 1303). No active handler in `index.js`. No hybrid damage function exists.

---

#### 6. Shield Charge / Smite (ID 1304)

| Property | Value |
|----------|-------|
| rAthena ID | 250 (CR_SHIELDCHARGE) |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 4 cells |
| Element | Neutral (weapon element) |
| SP Cost | 10 (all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 |
| Cooldown | 500ms (existing code) |
| Requirements | Shield equipped |
| Prerequisites | Auto Guard Lv5 |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 |
| Stun Chance | 20% | 25% | 30% | 35% | 40% |
| Knockback | 5 cells | 6 cells | 7 cells | 8 cells | 9 cells |

**Mechanics:**
- Physical melee attack using ATK
- Stun duration: 5 seconds (pre-renewal)
- Knockback formula: `skillLevel + 4` cells
- Stun chances from cards/items STACK with skill stun chance
- Knockback is disabled during War of Emperium
- ASPD affects skill delay when mounted without full Cavalry Mastery

**Discrepancy in existing code:** The existing `ro_skill_data_2nd.js` uses `effectValue: 120+i*30` which gives 120/150/180/210/240. The canonical values are 120/140/160/180/200 (increment of 20, not 30). **This needs fixing.**

The existing `RagnaCloneDocs/03_Skills_Complete.md` shows stun chances as 15/20/25/30/40 but iRO Wiki and RateMyServer both confirm 20/25/30/35/40. **Use iRO Wiki/RateMyServer values.**

**Current Code Status:** Definition exists (ID 1304) but ATK% values are WRONG. No active handler.

---

#### 7. Shield Boomerang (ID 1305)

| Property | Value |
|----------|-------|
| rAthena ID | 251 (CR_SHIELDBOOMERANG) |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Element | Neutral |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 700ms |
| Requirements | Shield equipped |
| Prerequisites | Shield Charge Lv3 |

**Pre-Renewal Damage Formula:**

```
Damage = ATK * (100 + 30*SkillLv) / 100
```

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 130 | 160 | 190 | 220 | 250 |
| Range | 3 cells | 5 cells | 7 cells | 9 cells | 11 cells |

Range formula: `1 + skillLevel * 2` cells

**Mechanics:**
- **Ignores size modifications** (no weapon-type size penalty)
- Single hit per cast
- In pre-renewal, the formula is simply ATK * skillMod% with NO shield weight/refine bonus
- Renewal (post-May 2020) added shield weight/refine: `(SkillLv*80 + ShieldRefine*5 + ShieldWeight)%` -- NOT used in our pre-renewal build
- Carries status effects from equipped cards or weapons
- The shield is visually thrown and returns

**Discrepancy in existing code:** The existing `ro_skill_data_2nd.js` uses `effectValue: 130+i*20` which gives 130/150/170/190/210. The canonical pre-renewal values are 130/160/190/220/250 (increment of 30). **This needs fixing.**

**Current Code Status:** Definition exists (ID 1305) but ATK% values are WRONG. No active handler.

---

### Supportive/Buff Skills

---

#### 8. Auto Guard (ID 1301)

| Property | Value |
|----------|-------|
| rAthena ID | 249 (CR_AUTOGUARD) |
| Type | Supportive (Toggle) |
| Max Level | 10 |
| Target | Self |
| Duration | 300 seconds (5 minutes) |
| Requirements | Shield equipped |
| Prerequisites | None |

**Block Chance and SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Block% | 5 | 10 | 14 | 18 | 21 | 24 | 26 | 28 | 29 | 30 |
| SP Cost | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Move Delay | 0.3s | 0.3s | 0.3s | 0.3s | 0.3s | 0.2s | 0.2s | 0.2s | 0.2s | 0.1s |

**Mechanics:**
- **Blocks both melee AND ranged physical attacks** (does NOT block magic)
- When a block occurs, the player is briefly immobilized (movement delay above)
- Toggle skill: can be turned on/off at will during the 5-minute window
- Shield must remain equipped; removing shield cancels the effect
- Incompatible with Magnum Break (if both trigger, no damage or knockback from Magnum Break)
- When blocked, the attack deals 0 damage but still triggers card on-hit effects

**Discrepancy in existing code:** The existing `ro_skill_data_2nd.js` uses `effectValue: 5+Math.floor((i+1)*2.5)` which generates: 5, 7, 10, 12, 15, 17, 20, 22, 25, 27. This is WRONG. The canonical iRO Wiki values are 5, 10, 14, 18, 21, 24, 26, 28, 29, 30. **This needs fixing.**

**Implementation Notes:**
- Requires a new buff type: `autoGuard` with `blockChance` and `moveDelay`
- On each incoming physical attack, check: `Math.random() * 100 < blockChance`
- If blocked: damage = 0, apply movement delay to defender, emit `combat:blocked` event
- Must be checked BEFORE damage calculation in the combat tick
- Status stored on player: `player.buffs.autoGuard = { level, blockChance, moveDelay, expiresAt }`

**Current Code Status:** Definition exists (ID 1301) with WRONG block% values. No handler.

---

#### 9. Reflect Shield (ID 1307)

| Property | Value |
|----------|-------|
| rAthena ID | 252 (CR_REFLECTSHIELD) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Duration | 300 seconds (5 minutes) |
| Requirements | Shield equipped |
| Prerequisites | Shield Boomerang Lv3 (existing code says Auto Guard Lv3 -- **DISCREPANCY: both are needed**) |

**Reflect % and SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Reflect% | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 | 40 |
| SP Cost | 35 | 40 | 45 | 50 | 55 | 60 | 65 | 70 | 75 | 80 |

**Mechanics:**
- Reflects a percentage of **melee physical damage** back to the attacker
- Does NOT reflect ranged attacks or magic
- Reflected damage is classified as "Reflect Type Damage" (separate from physical attack)
- Reflected damage does NOT trigger auto-cast cards on the Crusader
- Reflected damage does NOT trigger enemy Kaahi (HP recovery on hit)
- Ignores weapon size modifications
- Shield must remain equipped
- Formula: `reflectedDamage = floor(incomingMeleeDamage * reflectPercent / 100)`

**Implementation Notes:**
- After damage calculation on melee attacks vs a player with Reflect Shield active, calculate reflected damage
- Reflected damage bypasses attacker's DEF (it's based on the original incoming damage)
- Must emit a separate `combat:reflected` event to show the attacker taking reflect damage
- Stacks with card-based reflect effects (Maya Card)

**Current Code Status:** Definition exists (ID 1307). No handler. Prerequisite in code says `Auto Guard Lv3` but canonical prereq is `Shield Boomerang Lv3`. **Prereq needs fixing.**

---

#### 10. Devotion (ID 1306)

| Property | Value |
|----------|-------|
| rAthena ID | 255 (CR_DEVOTION) |
| Type | Supportive |
| Max Level | 5 |
| Target | Single Party Member |
| SP Cost | 25 (all levels) |
| Cast Time | 3 seconds (1.5s variable + 1.5s fixed) |
| After-Cast Delay | 3 seconds |
| Prerequisites | Reflect Shield Lv5, Grand Cross Lv4 |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Max Targets | 1 | 2 | 3 | 4 | 5 |
| Range | 7 cells | 8 cells | 9 cells | 10 cells | 11 cells |
| Duration | 30s | 45s | 60s | 75s | 90s |

**Mechanics:**
- Creates a damage-redirect link from a party member to the Crusader
- ALL physical AND magical damage intended for the protected ally is transferred to the Crusader
- Damage uses the **protected target's DEF/MDEF** (NOT the Crusader's) for reduction calculation
- If the redirected damage would kill the Crusader, the Crusader dies; excess damage is NOT passed back to the protected target
- Level difference restriction: target must be within **10 base levels** of the Crusader
- **Crusaders and Paladins CANNOT be Devotion targets** (both as casters and receivers)
- Cannot target self

**Breaking Conditions:**
- Duration expires
- Protected target moves out of skill range (7-11 cells)
- Crusader's HP drops below **25%** of MaxHP
- Crusader or protected target dies
- Crusader is stunned, frozen, or otherwise incapacitated (debatable, server-specific)

**Implementation Notes:**
- This is a complex new system requiring:
  1. A `devotionLinks` array on the Crusader player object: `[{ targetCharId, expiresAt, range }]`
  2. On EVERY damage event to a protected player, intercept and redirect to Crusader
  3. Distance checking on each game tick (or on damage event)
  4. HP threshold check on each damage event
  5. Link visualization on client (line connecting Crusader to protected player)
  6. New socket events: `devotion:link`, `devotion:break`, `devotion:damage_redirect`
- Must interact correctly with Auto Guard (Crusader can block redirected damage)
- Must interact correctly with Reflect Shield (reflected damage from Crusader to attacker)

**Current Code Status:** Definition exists (ID 1306). No handler. This is the most complex new system.

---

#### 11. Providence / Resistant Souls (ID 1308)

| Property | Value |
|----------|-------|
| rAthena ID | 256 (CR_PROVIDENCE) |
| Type | Supportive |
| Max Level | 5 |
| Target | Single Player (party member) |
| Range | 9 cells |
| SP Cost | 30 (all levels) |
| Cast Time | 1 second |
| After-Cast Delay | 3 seconds |
| Duration | 180 seconds (3 minutes) |
| Prerequisites | Divine Protection Lv5 (Acolyte), Heal Lv5 (Crusader) |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Demon Race Resist | 5% | 10% | 15% | 20% | 25% |
| Holy Element Resist | 5% | 10% | 15% | 20% | 25% |

**Mechanics:**
- Provides resistance to both Demon race attacks AND Holy element attacks
- **Crusaders CANNOT be targeted by Providence** (including self-cast)
- Only works on non-Crusader party members
- Stacks with Faith's holy resistance (multiplicative)

**Implementation Notes:**
- New buff type: `providence` with `demonResist` and `holyResist` fields
- Applied during damage calculation:
  - If attacker race is 'demon': `damage = floor(damage * (100 - providenceLevel*5) / 100)`
  - If attack element is 'holy': `damage = floor(damage * (100 - providenceLevel*5) / 100)`
- Must validate target is NOT a Crusader/Paladin class

**Current Code Status:** Definition exists (ID 1308). No handler. Prerequisite in code says `[{skillId: 1301, level: 5}, {skillId: 400, level: 5}]` which is Auto Guard Lv5 + Heal(Acolyte) Lv5. Canonical prereqs are Divine Protection(Acolyte) Lv5 + Heal(Crusader) Lv5. **Prereqs need fixing.**

---

#### 12. Defender / Defending Aura (ID 1309)

| Property | Value |
|----------|-------|
| rAthena ID | 257 (CR_DEFENDER) |
| Type | Supportive (Toggle) |
| Max Level | 5 |
| Target | Self |
| SP Cost | 30 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 800ms |
| Duration | 180 seconds (3 minutes) |
| Requirements | Shield equipped |
| Prerequisites | Shield Boomerang Lv1 (existing code says Shield Charge Lv5 -- **DISCREPANCY**) |

**Effects per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Ranged Damage Reduction | 20% | 35% | 50% | 65% | 80% |
| ASPD Penalty | -20% | -15% | -10% | -5% | 0% |
| Movement Speed | -33% | -33% | -33% | -33% | -33% |

**Mechanics:**
- Toggle skill: can be turned on/off during the duration
- Reduces ALL ranged physical damage by the listed percentage
- Movement speed reduced by ~33% at all levels
- ASPD penalty decreases with level; at Lv5 there is no ASPD penalty
- Shield must remain equipped
- When cast on self, nearby party members under Devotion also receive the ranged reduction (but also the movement penalty)
- Can stack with Auto Guard (guard blocks the attack entirely, Defender reduces if not blocked)

**Implementation Notes:**
- New buff type: `defender` with `rangedReduction`, `aspdPenalty`, `moveSpeedPenalty`
- Applied before ranged physical damage: `if (isRanged && target.buffs.defender) damage = floor(damage * (100 - reduction) / 100)`
- ASPD penalty applied via `buffAspdMultiplier` reduction
- Movement speed penalty applied via movement speed multiplier

**Current Code Status:** Definition exists (ID 1309) as toggle type. Prerequisite says Shield Charge Lv5, should be Shield Boomerang Lv1. **Prereq needs fixing.**

---

#### 13. Spear Quicken (ID 1310)

| Property | Value |
|----------|-------|
| rAthena ID | 258 (CR_SPEARQUICKEN) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Requirements | Two-handed spear equipped |
| Prerequisites | Spear Mastery Lv10 (Knight ID 700) |

**Effects per Level (Pre-Renewal):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ASPD Bonus | +21% | +22% | +23% | +24% | +25% | +26% | +27% | +28% | +29% | +30% |
| SP Cost | 24 | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 |
| Duration | 30s | 60s | 90s | 120s | 150s | 180s | 210s | 240s | 270s | 300s |

Pre-Renewal ASPD formula: `+(20 + SkillLv)%`

**Mechanics:**
- Identical to Knight's Two-Hand Quicken but for two-handed spears
- Cancelled by Decrease AGI and Quagmire
- Only works with two_hand_spear weapon type
- In pre-renewal, provides PURE ASPD bonus (no crit/flee)

**Implementation Notes:**
- New buff type: `spearQuicken` modifying `buffAspdMultiplier`
- Formula: `buffAspdMultiplier += (20 + skillLevel) / 100`
- Must check weapon type on cast AND on weapon change (if weapon changes, buff drops)
- Cancelled by: Decrease AGI, Quagmire, weapon swap to non-spear

**Current Code Status:** Definition exists (ID 1310). The `effectValue: 30` is hardcoded but pre-renewal scales 21-30%. No handler.

---

#### 14. Heal (Crusader version, ID 1311)

| Property | Value |
|----------|-------|
| rAthena ID | 28 (AL_HEAL, shared) |
| Type | Supportive |
| Max Level | 10 |
| Target | Single (ally or undead enemy) |
| Range | 9 cells |
| Element | Holy |
| Cast Time | 0 (instant in pre-renewal for Crusaders) |
| After-Cast Delay | 1 second |
| Prerequisites | Demon Bane Lv5 (Acolyte, ID 413), Faith Lv10 |

**Heal Formula (Pre-Renewal):**

```
HealAmount = floor((BaseLv + INT) / 8) * (4 + 8*SkillLv)
```

**SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 | 40 |

**Mechanics:**
- Identical to Acolyte Heal in formula and behavior
- Heals allies; damages Undead property monsters at 50% effectiveness
- When targeting Undead: `damage = floor(healAmount / 2)`, element is Holy
- Crusader's Heal typically has lower max level access and higher SP costs than a dedicated Priest

**Current Code Status:** Definition exists (ID 1311). The Acolyte Heal handler (ID 400) already exists and should work identically. May need to add ID 1311 to the existing heal handler as an alias.

---

#### 15. Cure (Crusader version, ID 1312)

| Property | Value |
|----------|-------|
| rAthena ID | 29 (AL_CURE, shared) |
| Type | Supportive |
| Max Level | 1 |
| Target | Single (ally) |
| Range | 9 cells |
| SP Cost | 15 |
| Cast Time | 0 (instant) |
| After-Cast Delay | 1 second |
| Prerequisites | Faith Lv5 |

**Mechanics:**
- Removes **Silence**, **Blind**, and **Confusion** status effects from target
- Identical to Acolyte Cure (ID 405)

**Current Code Status:** Definition exists (ID 1312). The Acolyte Cure handler (ID 405) already exists. Needs alias.

---

### Quest Skill

---

#### 16. Shrink (ID 1313)

| Property | Value |
|----------|-------|
| rAthena ID | 1002 (SN_SHRINK / CR_SHRINK) |
| Type | Toggle |
| Max Level | 1 |
| Target | Self |
| SP Cost | 15 |
| Duration | 300 seconds (5 minutes) |
| Requirements | Shield equipped, Auto Guard active |
| Prerequisites | Quest completion |

**Mechanics (Pre-Renewal):**
- When Auto Guard successfully blocks an attack, there is a **50% chance** to knockback the attacker **2 cells**
- The knockback chance is flat 50% (NOT scaled by Auto Guard level -- the per-AG-level scaling was a Renewal change)
- Requires Auto Guard to be active; Shrink does nothing without Auto Guard
- Toggle skill: can be turned on/off

**Implementation Notes:**
- Check during Auto Guard block: if Shrink is active AND block succeeds, roll 50% for knockback
- Knockback 2 cells away from the Crusader

**Current Code Status:** Definition exists (ID 1313). No handler.

---

## Current Implementation Status

### Skill Data Definitions (`ro_skill_data_2nd.js`)

| Skill | ID | Definition Exists | Data Correct | Handler Exists | Issues |
|-------|----|-------------------|--------------|----------------|--------|
| Faith | 1300 | YES | YES | NO | Missing passive handler in `getPassiveSkillBonuses()` |
| Auto Guard | 1301 | YES | **NO** | NO | Block% values WRONG (5,7,10,12,15,17,20,22,25,27 vs canonical 5,10,14,18,21,24,26,28,29,30) |
| Holy Cross | 1302 | YES | PARTIAL | NO | Prereq says Faith Lv3, should be Faith Lv7 |
| Grand Cross | 1303 | YES | **NO** | NO | SP costs WRONG (37+i*3 gives 37/40/43/... vs canonical 37/44/51/58/65/72/79/86/93/100, should be 30+7*level) |
| Shield Charge | 1304 | YES | **NO** | NO | ATK% WRONG (120/150/180/210/240 vs canonical 120/140/160/180/200) |
| Shield Boomerang | 1305 | YES | **NO** | NO | ATK% WRONG (130/150/170/190/210 vs canonical 130/160/190/220/250) |
| Devotion | 1306 | YES | YES | NO | — |
| Reflect Shield | 1307 | YES | PARTIAL | NO | Prereq says Auto Guard Lv3, should also require Shield Boomerang Lv3 |
| Providence | 1308 | YES | PARTIAL | NO | Prereqs WRONG (should be Divine Protection Lv5 + Heal(Crusader) Lv5) |
| Defender | 1309 | YES | PARTIAL | NO | Prereq says Shield Charge Lv5, should be Shield Boomerang Lv1 |
| Spear Quicken | 1310 | YES | PARTIAL | NO | effectValue hardcoded 30 but pre-renewal scales 21-30% |
| Heal (Crusader) | 1311 | YES | YES | PARTIAL | Needs alias to Acolyte Heal handler |
| Cure (Crusader) | 1312 | YES | YES | PARTIAL | Needs alias to Acolyte Cure handler |
| Shrink | 1313 | YES | YES | NO | — |

### Shared Skills (Knight skills Crusader needs)

| Skill | ID | Available to Crusader | Handler Exists |
|-------|----|----------------------|----------------|
| Spear Mastery | 700 | **NO** (classId='knight') | YES (for knights) |
| Riding | 708 | **NO** (classId='knight') | NO |
| Cavalry Mastery | 709 | **NO** (classId='knight') | NO |

### Passive Handlers (`getPassiveSkillBonuses()`)

None of the Crusader passives are handled. Missing:
1. Faith (1300): `bonusMaxHp += level * 200`, `holyResist += level * 5`
2. Spear Mastery (700): Already handled for Knights, need access for Crusaders

---

## New Systems Required

### Priority 1: Fix Data Definitions (Low Effort, High Impact)

1. **Fix Auto Guard block% values** in `ro_skill_data_2nd.js`
   - Change effectValue formula to match canonical: `[5,10,14,18,21,24,26,28,29,30]`

2. **Fix Shield Charge ATK% values**
   - Change effectValue from `120+i*30` to `120+i*20`

3. **Fix Shield Boomerang ATK% values**
   - Change effectValue from `130+i*20` to `130+i*30` (it was `100+30*SkillLv`, so Lv1=130, Lv2=160...)

4. **Fix prerequisites**
   - Holy Cross: Faith Lv3 -> Faith Lv7
   - Reflect Shield: Add Shield Boomerang Lv3 requirement
   - Providence: Change to Divine Protection Lv5 + Heal(Crusader) Lv5
   - Defender: Change from Shield Charge Lv5 to Shield Boomerang Lv1

5. **Fix Spear Quicken ASPD scaling**
   - Pre-renewal: `+(20+SkillLv)%`, not flat 30%

### Priority 2: Shared Skill Access System (Medium Effort)

Create a mechanism for Crusaders to access Spear Mastery (700), Riding (708), and Cavalry Mastery (709) without granting full Knight skill access.

**Recommended approach:** Add a `sharedWith` array to skill definitions:
```javascript
{ id: 700, name: 'spear_mastery', classId: 'knight', sharedWith: ['crusader'], ... }
```

Then modify `getAvailableSkills()`:
```javascript
function getAvailableSkills(jobClass) {
    const chain = CLASS_PROGRESSION[jobClass] || ['novice'];
    const skills = [];
    for (const cls of chain) {
        if (CLASS_SKILLS[cls]) skills.push(...CLASS_SKILLS[cls]);
    }
    // Add shared skills from other classes
    for (const s of ALL_SKILLS) {
        if (s.sharedWith && s.sharedWith.includes(jobClass)) {
            if (!skills.find(sk => sk.id === s.id)) skills.push(s);
        }
    }
    return skills;
}
```

### Priority 3: Passive Skill Handlers (Low Effort)

Add to `getPassiveSkillBonuses()`:

```javascript
// Faith (1300): +200 MaxHP/lv, +5% Holy resist/lv
const faithLv = learned[1300] || 0;
if (faithLv > 0) {
    bonuses.bonusMaxHp = (bonuses.bonusMaxHp || 0) + faithLv * 200;
    bonuses.holyResist = faithLv * 5; // percentage
}

// Spear Mastery (700) — already handled for Knights, extend for Crusaders
// (will be auto-handled once shared skill access works)
```

The `holyResist` bonus needs to be applied in `calculatePhysicalDamage()` and `calculateMagicalDamage()` when the attack element is 'holy'.

### Priority 4: Auto Guard System (Medium Effort)

**New buff type:** `autoGuard`

**Server-side:**
1. On `skill:use` for Auto Guard (1301):
   - Toggle on: apply buff `{ type: 'autoGuard', level, blockChance: [5,10,14,18,21,24,26,28,29,30][level-1], moveDelay: level<=5?300:level<=9?200:100, expiresAt: now+300000 }`
   - Toggle off: remove buff
   - Validate shield equipped

2. In combat tick, BEFORE damage calculation:
   ```javascript
   if (target.buffs && target.buffs.autoGuard && target.buffs.autoGuard.active) {
       if (isPhysicalAttack && Math.random() * 100 < target.buffs.autoGuard.blockChance) {
           // Block! No damage dealt
           broadcastToZone(zone, 'combat:blocked', { targetId, attackerId, skillName: 'Auto Guard' });
           // Apply movement delay to target
           target.moveBlockedUntil = Date.now() + target.buffs.autoGuard.moveDelay;
           // Check Shrink
           if (target.buffs.shrink && target.buffs.shrink.active) {
               if (Math.random() < 0.5) knockbackTarget(attacker, target, 2);
           }
           return; // Skip damage
       }
   }
   ```

**Client-side:**
- New `combat:blocked` event handler in CombatActionSubsystem
- Display "Guard" text above defender (similar to miss text)

### Priority 5: Reflect Shield System (Medium Effort)

**New buff type:** `reflectShield`

**Server-side:**
1. On `skill:use` for Reflect Shield (1307):
   - Apply buff: `{ type: 'reflectShield', reflectPercent: 13+3*(level-1), expiresAt: now+300000 }`
   - Validate shield equipped

2. AFTER melee physical damage is dealt to a player with Reflect Shield:
   ```javascript
   if (target.buffs && target.buffs.reflectShield && isMelee) {
       const reflectedDmg = Math.floor(finalDamage * target.buffs.reflectShield.reflectPercent / 100);
       attacker.hp -= reflectedDmg;
       broadcastToZone(zone, 'combat:reflect', { reflectorId: targetId, reflectTargetId: attackerId, damage: reflectedDmg });
   }
   ```

### Priority 6: Offensive Skill Handlers (Medium Effort)

#### Holy Cross Handler
```javascript
case 1302: { // Holy Cross
    const atkPct = 135 + (learnedLevel - 1) * 35;
    const is2H = player.weaponType === 'two_hand_spear';
    const finalMultiplier = is2H ? atkPct * 2 : atkPct;
    // Calculate physical damage with forced holy element
    const dmgResult = calculatePhysicalDamage(attackerObj, targetObj, {
        isSkill: true, skillMultiplier: finalMultiplier, skillElement: 'holy',
        forceHit: false, skillName: 'holy_cross'
    });
    // 2 hits bundled as one
    // Blind chance: 3*level %
    if (Math.random() * 100 < learnedLevel * 3) {
        applyStatusEffect(target, 'blind', 30000);
    }
    break;
}
```

#### Shield Charge Handler
```javascript
case 1304: { // Shield Charge
    if (!player.equippedShield) { socket.emit('skill:error', { msg: 'Requires shield' }); return; }
    const atkPct = 120 + (learnedLevel - 1) * 20;
    const dmgResult = calculatePhysicalDamage(attackerObj, targetObj, {
        isSkill: true, skillMultiplier: atkPct, skillName: 'shield_charge'
    });
    // Stun chance: 20 + 5*(level-1) %, stun duration 5000ms
    if (Math.random() * 100 < 20 + (learnedLevel - 1) * 5) {
        applyStatusEffect(target, 'stun', 5000);
    }
    // Knockback: level + 4 cells
    knockbackTarget(target, player, learnedLevel + 4);
    break;
}
```

#### Shield Boomerang Handler
```javascript
case 1305: { // Shield Boomerang
    if (!player.equippedShield) { socket.emit('skill:error', { msg: 'Requires shield' }); return; }
    const atkPct = 100 + learnedLevel * 30; // 130/160/190/220/250
    const dmgResult = calculatePhysicalDamage(attackerObj, targetObj, {
        isSkill: true, skillMultiplier: atkPct, skillName: 'shield_boomerang',
        // No size penalty for Shield Boomerang
    });
    break;
}
```

### Priority 7: Grand Cross — Hybrid Damage System (High Effort)

This requires a new damage calculation function that combines ATK and MATK:

```javascript
function calculateHybridDamage(attacker, target, options = {}) {
    const { skillMultiplier = 100, skillElement = 'neutral' } = options;

    // Calculate ATK component (physical)
    const atkDerived = calculateDerivedStats(attacker.stats);
    const statusATK = atkDerived.statusATK;
    const weaponATK = attacker.weaponATK || 0;
    const passiveATK = attacker.passiveATK || 0;
    const totalATK = statusATK + weaponATK + passiveATK;

    // Calculate MATK component (magical)
    const matkMin = atkDerived.matkMin;
    const matkMax = atkDerived.matkMax;
    const matk = matkMin + Math.floor(Math.random() * (matkMax - matkMin + 1));

    // Combine: (ATK + MATK) * skillMultiplier%
    let hybridDamage = Math.floor((totalATK + matk) * skillMultiplier / 100);

    // Apply element modifier
    const targetElement = (target.element && target.element.type) || 'neutral';
    const targetElementLevel = (target.element && target.element.level) || 1;
    const eleMod = getElementModifier(skillElement, targetElement, targetElementLevel);
    hybridDamage = Math.floor(hybridDamage * eleMod / 100);

    // Apply DEF as hard defense only (flat subtraction, not percentage)
    const hardDef = Math.min(99, target.hardDef || 0);
    hybridDamage = hybridDamage - hardDef;

    return Math.max(1, hybridDamage);
}
```

#### Grand Cross Handler
```javascript
case 1303: { // Grand Cross
    // HP cost: 20% of current HP
    const hpCost = Math.floor(player.hp * 0.2);
    player.hp -= hpCost;

    const atkMatkPct = 100 + learnedLevel * 40; // 140/180/220/.../500

    // Find all enemies in cross-shaped AoE around caster
    const crossCells = getCrossCells(player.x, player.y, 2); // 2-cell radius cross
    const enemiesInCross = getEnemiesInCells(zone, crossCells);

    for (const enemy of enemiesInCross) {
        const dmg = calculateHybridDamage(attackerObj, enemyObj, {
            skillMultiplier: atkMatkPct, skillElement: 'holy'
        });
        enemy.hp -= dmg;
        broadcastToZone(zone, 'skill:effect_damage', { ... });

        // Blind on Undead/Demon: 3*level %
        if ((enemy.race === 'undead' || enemy.race === 'demon') && Math.random()*100 < learnedLevel*3) {
            applyStatusEffect(enemy, 'blind', 30000);
        }
    }

    // Self-damage: half of dealt damage * holy element mod on caster
    const casterHolyMod = getElementModifier('holy', player.armorElement || 'neutral', 1);
    const faithResist = (learned[1300] || 0) * 5; // Faith holy resist
    const selfDmg = Math.floor(avgDamageDealt / 2 * casterHolyMod / 100 * (100 - faithResist) / 100);
    player.hp -= selfDmg;

    break;
}
```

### Priority 8: Devotion System (High Effort, Complex)

This is the most complex new system for Crusader. It requires:

1. **Data structure on player:**
   ```javascript
   player.devotionLinks = []; // Array of { targetCharId, expiresAt, maxRange }
   player.devotedTo = null;   // charId of Crusader protecting this player
   ```

2. **Devotion cast handler:**
   ```javascript
   case 1306: { // Devotion
       const maxTargets = learnedLevel;
       if (player.devotionLinks.length >= maxTargets) { /* error */ }
       const targetPlayer = getPlayerByCharId(data.targetId);
       if (!targetPlayer) { /* error */ }
       if (targetPlayer.jobClass === 'crusader' || targetPlayer.jobClass === 'paladin') { /* error: cannot target Crusaders */ }
       if (Math.abs(player.baseLevel - targetPlayer.baseLevel) > 10) { /* error: level diff */ }
       const maxRange = 7 + (learnedLevel - 1);
       const duration = 30000 + (learnedLevel - 1) * 15000;
       player.devotionLinks.push({ targetCharId: targetPlayer.characterId, expiresAt: Date.now() + duration, maxRange });
       targetPlayer.devotedTo = player.characterId;
       broadcastToZone(zone, 'devotion:link', { casterId: player.characterId, targetId: targetPlayer.characterId });
       break;
   }
   ```

3. **Damage interception (in combat tick and skill damage):**
   ```javascript
   // Before applying damage to target, check if they have Devotion protection
   if (target.devotedTo) {
       const crusader = getPlayerByCharId(target.devotedTo);
       if (crusader && crusader.hp > 0) {
           const dist = getDistance(crusader, target);
           const link = crusader.devotionLinks.find(l => l.targetCharId === target.characterId);
           if (link && dist <= link.maxRange && Date.now() < link.expiresAt) {
               // Redirect damage to Crusader
               // Damage uses TARGET's DEF, not Crusader's DEF (already calculated above)
               crusader.hp -= finalDamage;
               if (crusader.hp <= 0) { /* Crusader dies, break all links */ }
               if (crusader.hp < crusader.maxHp * 0.25) { /* Break link: HP below 25% */ }
               broadcastToZone(zone, 'devotion:damage_redirect', { ... });
               return; // Target takes no damage
           }
       }
   }
   ```

4. **Link expiry/break checking (in game tick):**
   ```javascript
   // Every tick, check all Devotion links for expiry and range
   for (const player of allPlayers) {
       if (player.devotionLinks && player.devotionLinks.length > 0) {
           player.devotionLinks = player.devotionLinks.filter(link => {
               if (Date.now() >= link.expiresAt) { breakDevotionLink(player, link); return false; }
               const target = getPlayerByCharId(link.targetCharId);
               if (!target) { breakDevotionLink(player, link); return false; }
               const dist = getDistance(player, target);
               if (dist > link.maxRange) { breakDevotionLink(player, link); return false; }
               return true;
           });
       }
   }
   ```

### Priority 9: Defender System (Medium Effort)

**New buff type:** `defender`

Similar to Auto Guard but specifically reduces ranged damage. Toggle on/off with movement speed penalty.

```javascript
case 1309: { // Defender
    if (!player.equippedShield) { /* error */ }
    if (player.buffs.defender && player.buffs.defender.active) {
        // Toggle off
        player.buffs.defender.active = false;
        player.moveSpeedMultiplier = (player.moveSpeedMultiplier || 1) / 0.67;
        player.buffAspdMultiplier = (player.buffAspdMultiplier || 1) / (1 - aspdPenalty/100);
    } else {
        // Toggle on
        const rangedReduction = [20, 35, 50, 65, 80][learnedLevel - 1];
        const aspdPenalty = [20, 15, 10, 5, 0][learnedLevel - 1];
        player.buffs.defender = {
            active: true, rangedReduction, aspdPenalty,
            expiresAt: Date.now() + 180000
        };
        player.moveSpeedMultiplier = (player.moveSpeedMultiplier || 1) * 0.67;
        if (aspdPenalty > 0) player.buffAspdMultiplier *= (1 - aspdPenalty/100);
    }
}
```

### Priority 10: Riding / Mount System (High Effort, Shared with Knight)

This is a visual + mechanical system:

**Server-side:**
1. Track `player.isMounted` boolean
2. When mounted:
   - `player.moveSpeed *= 1.25` (25% movement speed increase)
   - `player.maxWeight += 1000`
   - Spear size penalty to Medium becomes 100% (from 75%)
   - ASPD penalty: `player.mountedAspdPenalty = 0.5` (mitigated by Cavalry Mastery)
   - Spear Mastery bonus increases: `+5/lv` instead of `+4/lv`
3. Mount/dismount via skill toggle (Riding enables the ability, not auto-mounts)

**Client-side:**
- Swap character model/mesh to mounted version (BP_MMOCharacter with Peco Peco)
- This is primarily a visual/animation task
- New socket event: `player:mount`, `player:dismount`

---

## Implementation Priority

| Priority | System | Effort | Dependencies | Impact |
|----------|--------|--------|-------------|--------|
| **P0** | Fix data definitions (5 skills) | Low | None | Fixes incorrect skill data |
| **P1** | Shared skill access system | Low | None | Enables Crusader skill tree |
| **P2** | Faith passive handler | Low | P0 | MaxHP + Holy resist |
| **P3** | Heal/Cure alias handlers | Low | None | Reuse existing Acolyte code |
| **P4** | Holy Cross handler | Medium | P2 | Core offensive skill |
| **P5** | Shield Charge handler | Medium | None | Knockback + stun |
| **P6** | Shield Boomerang handler | Medium | P5 | Ranged shield attack |
| **P7** | Auto Guard system | Medium | None | Core defensive mechanic |
| **P8** | Reflect Shield system | Medium | P7 | Damage reflection |
| **P9** | Spear Quicken handler | Low | P1 | ASPD buff |
| **P10** | Grand Cross hybrid damage | High | P2, P4 | Signature AoE skill |
| **P11** | Providence handler | Low | None | Party resist buff |
| **P12** | Defender system | Medium | P6 | Ranged damage reduction |
| **P13** | Shrink handler | Low | P7 | Knockback on block |
| **P14** | Devotion system | High | P7, P8 | Most complex: damage redirect |
| **P15** | Mount system (Peco Peco) | High | P1 | Visual + mechanical, shared with Knight |

---

## Integration Points

### Combat Tick (`index.js`)

The combat tick must be modified to check:
1. **Auto Guard** block chance BEFORE damage calculation
2. **Reflect Shield** reflect damage AFTER damage dealt
3. **Devotion** damage redirect BEFORE applying damage to target
4. **Defender** ranged damage reduction during damage calculation
5. **Faith** holy resistance during element modifier application

Processing order for incoming physical damage:
```
1. Check Devotion redirect (transfer to Crusader)
2. Check Auto Guard block (if blocked, skip damage)
3. Calculate damage normally
4. Apply Defender ranged reduction (if ranged attack)
5. Apply Providence resistance (if attacker is Demon or Holy element)
6. Apply Faith holy resistance (if attack is Holy element)
7. Apply final damage
8. Apply Reflect Shield reflection (if melee attack)
9. Check Shrink knockback (if Auto Guard blocked earlier -- N/A if we reach here)
```

### Buff System (`ro_buff_system.js`)

New buff types needed:
- `autoGuard`: { blockChance, moveDelay, active }
- `reflectShield`: { reflectPercent, active }
- `defender`: { rangedReduction, aspdPenalty, moveSpeedPenalty, active }
- `spearQuicken`: { aspdBonus }
- `providence`: { demonResist, holyResist }
- `shrink`: { active }

### Damage Formulas (`ro_damage_formulas.js`)

New function needed:
- `calculateHybridDamage()` for Grand Cross (combines ATK + MATK)

Modifications:
- `calculatePhysicalDamage()`: add Holy resistance check (Faith, Providence)
- `calculateMagicalDamage()`: add Holy resistance check (Faith, Providence)

### Client Subsystems

New socket events needed:
- `combat:blocked` — Auto Guard block notification
- `combat:reflect` — Reflect Shield damage notification
- `devotion:link` — Devotion link established
- `devotion:break` — Devotion link broken
- `devotion:damage_redirect` — Damage redirected via Devotion
- `player:mount` / `player:dismount` — Peco Peco mount state

CombatActionSubsystem modifications:
- Handle `combat:blocked` (display "Guard" text)
- Handle `combat:reflect` (display reflected damage number)

### Skill Tree UI

The SkillTreeSubsystem already supports skill trees. Crusader skills need:
- Correct prerequisites linking
- Skill icons (need to be created/generated)
- Tree layout per the `treeRow`/`treeCol` values in skill definitions

---

## Constants and Data Tables

### Auto Guard Block Chances
```javascript
const AUTO_GUARD_BLOCK_CHANCE = [5, 10, 14, 18, 21, 24, 26, 28, 29, 30];
```

### Auto Guard Move Delay (ms)
```javascript
const AUTO_GUARD_MOVE_DELAY = [300, 300, 300, 300, 300, 200, 200, 200, 200, 100];
```

### Shield Charge Stun Chances
```javascript
const SHIELD_CHARGE_STUN = [20, 25, 30, 35, 40];
```

### Shield Charge Knockback
```javascript
const SHIELD_CHARGE_KNOCKBACK = [5, 6, 7, 8, 9]; // level + 4
```

### Defender Ranged Reduction
```javascript
const DEFENDER_RANGED_REDUCTION = [20, 35, 50, 65, 80];
```

### Defender ASPD Penalty
```javascript
const DEFENDER_ASPD_PENALTY = [20, 15, 10, 5, 0];
```

### Reflect Shield Percentage
```javascript
const REFLECT_SHIELD_PCT = [13, 16, 19, 22, 25, 28, 31, 34, 37, 40]; // 13 + 3*(level-1)
```

### Spear Quicken ASPD Bonus (Pre-Renewal)
```javascript
const SPEAR_QUICKEN_ASPD = [21, 22, 23, 24, 25, 26, 27, 28, 29, 30]; // 20 + level
```

### Grand Cross SP Costs
```javascript
const GRAND_CROSS_SP = [37, 44, 51, 58, 65, 72, 79, 86, 93, 100]; // 30 + 7*level
```

### Cross-Shaped AoE Cell Offsets (for Grand Cross)
```javascript
const CROSS_OFFSETS = [
    { dx: 0, dy: -2 }, { dx: 0, dy: -1 },
    { dx: -2, dy: 0 }, { dx: -1, dy: 0 }, { dx: 0, dy: 0 }, { dx: 1, dy: 0 }, { dx: 2, dy: 0 },
    { dx: 0, dy: 1 }, { dx: 0, dy: 2 }
]; // 9 cells total in cross pattern
```

### Devotion Duration (ms)
```javascript
const DEVOTION_DURATION = [30000, 45000, 60000, 75000, 90000];
```

### Devotion Range (cells)
```javascript
const DEVOTION_RANGE = [7, 8, 9, 10, 11];
```

### Cavalry Mastery ASPD Recovery
```javascript
const CAVALRY_ASPD_RECOVERY = [0.6, 0.7, 0.8, 0.9, 1.0]; // multiplier
```

### Crusader Job Stat Bonuses (HP/SP coefficients)
The `HP_SP_COEFFICIENTS` table in `ro_exp_tables.js` should already have entries for 'crusader'. If not, add:
```javascript
'crusader': { HP_JOB_A: 5.60, HP_JOB_B: 5.0, SP_JOB: 2.0 }
```

---

## Paladin (Transcendent) — Future Reference

Paladin inherits all Swordsman + Crusader skills and adds:

| Skill | rAthena ID | Max Lv | Description |
|-------|-----------|--------|-------------|
| Gloria Domini (Pressure) | 367 | 5 | Holy ranged attack, ignores DEF/MDEF, drains SP |
| Martyr's Reckoning (Sacrifice) | 368 | 5 | Self-HP-cost attacks, 9% MaxHP per hit, massive damage |
| Battle Chant (Gospel) | 369 | 10 | Random buffs to allies, random debuffs to enemies |
| Shield Chain (Rapid Smiting) | 480 | 5 | 5-hit shield attack, 500-1300% ATK |
| Defending Aura (party) | — | — | Extends Defender to affect party in range |

These are NOT in scope for the initial Crusader implementation but are documented here for future planning.

---

## Summary of Bugs to Fix

| File | Line/Area | Bug | Fix |
|------|-----------|-----|-----|
| `ro_skill_data_2nd.js` | ID 1301 Auto Guard | effectValue formula gives wrong block% | Use lookup array `[5,10,14,18,21,24,26,28,29,30]` |
| `ro_skill_data_2nd.js` | ID 1302 Holy Cross | Prerequisite Faith Lv3 | Change to Faith Lv7 |
| `ro_skill_data_2nd.js` | ID 1304 Shield Charge | effectValue `120+i*30` (120/150/180/210/240) | Change to `120+i*20` (120/140/160/180/200) |
| `ro_skill_data_2nd.js` | ID 1305 Shield Boomerang | effectValue `130+i*20` (130/150/170/190/210) | Change to `100+(i+1)*30` (130/160/190/220/250) |
| `ro_skill_data_2nd.js` | ID 1307 Reflect Shield | Prerequisite only Auto Guard Lv3 | Should also require Shield Boomerang Lv3 |
| `ro_skill_data_2nd.js` | ID 1308 Providence | Prereqs: Auto Guard(1301) Lv5 + Heal(Acolyte,400) Lv5 | Should be: Divine Protection(401) Lv5 + Heal(Crusader,1311) Lv5. Auto Guard is NOT a prereq. |
| `ro_skill_data_2nd.js` | ID 1309 Defender | Prerequisite Shield Charge Lv5 | Should be Shield Boomerang Lv1 |
| `ro_skill_data_2nd.js` | ID 1303 Grand Cross | SP cost formula `37+i*3` gives 37/40/43/... | Should be `30+(i+1)*7` giving 37/44/51/58/65/72/79/86/93/100 |
| `ro_skill_data_2nd.js` | ID 1310 Spear Quicken | effectValue hardcoded 30 | Should scale `20+level` for pre-renewal |
| `index.js` | getPassiveSkillBonuses | No Crusader passives handled | Add Faith (1300) handler |
| `ro_skill_data.js` | getAvailableSkills | Crusaders can't access Spear Mastery/Riding/Cavalry | Add sharedWith mechanism |
