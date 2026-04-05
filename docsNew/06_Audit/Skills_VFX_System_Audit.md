# Skills & VFX System Audit — Comprehensive Findings & Recommendations

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [VFX_Asset_Reference](../05_Development/VFX_Asset_Reference.md) | [Skill_VFX_Implementation_Plan](../05_Development/Skill_VFX_Implementation_Plan.md)

**Date**: 2026-03-09
**Scope**: Full audit of server-side skill logic, client-side SkillTreeSubsystem, SkillVFXSubsystem, SkillVFXData, targeting system, ground AoE indicators, and comparison against RO Classic pre-renewal reference.

---

## Executive Summary

The skill and VFX systems are **architecturally sound** — clean separation of concerns, event-driven design, Live Coding safety, multiplayer awareness. However, they are **severely under-populated**: only **14 of 139 defined skills** have full server logic, and only **15 skills have VFX configurations**. The infrastructure is good enough — it does NOT need a refactor. What it needs is **massive content expansion** and **targeted fixes** for the gaps identified below.

**Verdict: Do NOT refactor. Expand and fix.**

---

## Table of Contents

1. [Current State Summary](#1-current-state-summary)
2. [Critical Issues (Must Fix)](#2-critical-issues-must-fix)
3. [Architecture Assessment](#3-architecture-assessment)
4. [Missing RO Classic Features](#4-missing-ro-classic-features)
5. [Server-Side Gaps](#5-server-side-gaps)
6. [Client VFX Gaps](#6-client-vfx-gaps)
7. [Targeting & AoE Indicator Gaps](#7-targeting--aoe-indicator-gaps)
8. [Status Effect System (Missing)](#8-status-effect-system-missing)
9. [Buff/Debuff System Gaps](#9-buffdebuff-system-gaps)
10. [Code Quality Issues](#10-code-quality-issues)
11. [Stale Documentation](#11-stale-documentation)
12. [Prioritized Fix Plan](#12-prioritized-fix-plan)
13. [Per-Skill Implementation Checklist](#13-per-skill-implementation-checklist)
14. [New VFX Templates Needed](#14-new-vfx-templates-needed)
15. [RO Classic Reference Data](#15-ro-classic-reference-data)

---

## 1. Current State Summary

### What Works Well

| System | Status | Notes |
|--------|--------|-------|
| Cast time with DEX reduction | COMPLETE | `castTime * (150 / (150 + DEX))`, instant at 150 DEX |
| After-cast delay (ACD) | COMPLETE | Global skill lockout, Bragi reducible |
| Per-skill cooldowns | COMPLETE | Server-enforced, client-tracked |
| Cast interruption | COMPLETE | Movement > 5 UE units or damage cancels |
| Range validation | COMPLETE | Pre-cast + post-cast checks |
| Socket event architecture | COMPLETE | 18 event types, zone-scoped broadcasting |
| VFX infrastructure (5 patterns) | COMPLETE | BoltFromSky, Projectile, AoEImpact, GroundPersistent, GroundAoERain, SelfBuff, TargetDebuff, HealFlash |
| Casting circle actor | COMPLETE | Element-colored, auto-fade |
| Ground AoE targeting indicator | COMPLETE | DrawDebugCircle, cursor-follow, element colors |
| Click-to-cast targeting | COMPLETE | Single-target + ground-target modes |
| Skill tree UI | COMPLETE | Class-based tree, prerequisites, learn/reset |
| Hotbar integration | COMPLETE | 4 rows x 9 slots, keybinds |
| Damage number popups | COMPLETE | World-to-screen projection |
| Cooldown display | COMPLETE | Client-side tracking with expiry times |

### What's Broken or Missing

| System | Status | Impact |
|--------|--------|--------|
| 125/139 skills use generic fallback | CRITICAL | Only deducts SP, no game effect |
| All passive skills (bonuses) | NOT IMPLEMENTED | Sword Mastery, HP Recovery, etc. do nothing |
| Healing skills | NOT IMPLEMENTED | Heal, Sanctuary — only First Aid works |
| Support/buff skills | NOT IMPLEMENTED | Blessing, Increase AGI, Kyrie Eleison |
| Status effects (visual) | NOT IMPLEMENTED | No freeze overlay, no stun stars, no poison tint |
| Buff bar UI | NOT IMPLEMENTED | No buff icons at top of screen |
| Knockback physics | NOT IMPLEMENTED | Magnum Break, Fire Wall should push enemies |
| Item/gem costs for skills | NOT IMPLEMENTED | Safety Wall should consume Blue Gemstone |
| Weapon type requirements | NOT IMPLEMENTED | Pierce should require Spear equipped |
| Trap placement system | NOT IMPLEMENTED | Hunter traps need world objects |
| Song/Dance aura system | NOT IMPLEMENTED | Party-wide radius effects |
| Skill-based movement | NOT IMPLEMENTED | Teleport, Back Slide, Snap |
| InfinityBlade asset migration | UNKNOWN | 7 skills reference paths that may not exist in project |

### Coverage Numbers

| Metric | Count | Total | Percentage |
|--------|-------|-------|-----------|
| Skills with full server logic | 14 | 139 | **10%** |
| Skills with VFX configs | 15 | 139 | **11%** |
| Skills with ground AoE indicator | 6 | ~20 ground skills | **30%** |
| Buff types tracked by server | 4 | 40+ needed | **10%** |
| Status effects with visuals | 0 | 10 core effects | **0%** |
| Classes with viable combat | 2 | 13 | **15%** (Swordsman, Mage only) |

---

## 2. Critical Issues (Must Fix)

### Issue 1: InfinityBlade Asset Paths May Not Exist

**Severity**: HIGH — Skills may have no VFX at runtime

**Problem**: `SkillVFXData.cpp` hardcodes Cascade asset paths like:
```
/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/ICE/P_Elemental_Ice_Proj.P_Elemental_Ice_Proj
```

`VFX_Asset_Reference.md` says InfinityBlade Effects (838 assets) are **NOT migrated** to the SabriMMO project. If these assets don't exist in the Content folder, `LoadObject<UParticleSystem>()` silently returns nullptr and skills render with NO VFX.

**Affected skills**: Cold Bolt (200), Provoke (104), Endure (106), Sight (205), Frost Diver (208), Fire Wall (209), Safety Wall (211) — all 7 Cascade-based VFX configs.

**Fix**:
1. Run the game and check UE_LOG output for failed asset loads
2. Either migrate InfinityBlade assets into the project, OR
3. Replace Cascade references with Niagara equivalents from the packs already in-project (Free_Magic, Mixed_Magic_VFX_Pack)

### Issue 2: Duplicate AoE Radius Definitions

**Severity**: MEDIUM — Maintenance burden, potential desync

**Problem**: Ground AoE radius is defined in TWO places:
- `GroundAoE::GetAoEInfo()` in SkillTreeSubsystem.cpp (for targeting indicator)
- `FSkillVFXConfig.AoERadius` in SkillVFXData.cpp (for VFX spawning)

If one is updated and the other is not, the targeting circle won't match the actual AoE.

**Fix**: Have `GetAoEInfo()` read from `GetSkillVFXConfig()` instead of maintaining its own switch statement. OR create a shared `GetSkillAoERadius(int32 SkillId)` function in SkillVFXData that both systems call.

### Issue 3: Two Separate Targeting Circle Systems

**Severity**: MEDIUM — Confusing architecture, potential visual conflicts

**Problem**: Two independent circle systems exist:
1. `GroundAoE` namespace in SkillTreeSubsystem.cpp — `DrawDebugCircle` for targeting indicator (follows cursor)
2. `CastingCircleActor` in SkillVFXSubsystem — spawned on `skill:cast_start` (at cast position)

These are actually **different phases** (targeting vs casting), but this isn't documented. It's unclear if both render simultaneously during cast time.

**Fix**: Document the lifecycle clearly:
- Phase 1 (Targeting): GroundAoE circle follows cursor → user clicks → circle locks at position
- Phase 2 (Casting): CastingCircleActor spawns at locked position during cast time
- Phase 3 (Effect): VFX spawns, both circles dismiss
- Verify no visual overlap between locked GroundAoE circle and CastingCircleActor

### Issue 4: CascadeLoopTimer Hack

**Severity**: LOW-MEDIUM — Works but scales poorly

**Problem**: Every 2 seconds, ALL active Cascade particle system components are re-activated via `PSC->ActivateSystem(true)`. This is because Cascade non-looping emitters don't auto-restart after particle depletion, and `IsActive()` returns stale state.

With many players + ground effects (Fire Walls, Safety Walls, buffs), this could spike frame time.

**Fix**: Long-term, migrate all Cascade effects to Niagara. Short-term, the hack is fine for current scale (< 50 concurrent effects).

### Issue 5: SP Deducted Before Cast Completion

**Severity**: MEDIUM — Deviates from RO Classic

**Problem**: In the server's `skill:use` handler, SP is deducted when the skill is initiated (before cast time). In RO Classic, SP is deducted **after cast completes**. If the cast is interrupted, the player should NOT lose SP.

**Fix**: Move SP deduction from the pre-cast validation block to `executeCastComplete()`. Add SP cost to the `activeCasts` entry so it's available at completion time.

### Issue 6: Buffs/Cooldowns Lost on Server Restart

**Severity**: MEDIUM — Gameplay disruption

**Problem**: `player.skillCooldowns`, `player.activeBuffs`, and `afterCastDelayEnd` are all in-memory only. Server restart clears everything, allowing instant skill spam and losing all active buffs.

**Fix**: For buffs with duration > 30s, persist to Redis with TTL matching remaining duration. On reconnect, reload active buffs from Redis.

---

## 3. Architecture Assessment

### Verdict: Architecture is GOOD — Do Not Refactor

The current architecture follows sound patterns:

1. **Server-authoritative**: All damage, SP costs, cooldowns validated server-side
2. **Event-driven**: Socket.io events bridge server logic to client presentation
3. **Separation of concerns**: SkillTreeSubsystem (UI/targeting) vs SkillVFXSubsystem (visual effects) vs server (game logic)
4. **Multiplayer-safe**: Keyed by CasterId/TargetId, no global singletons
5. **Live Coding compatible**: Heap-allocated state maps, configs rebuilt per-call

### What Should Stay As-Is

- `FSkillVFXConfig` struct and template enum — extensible, clean
- Socket event registration via `EventRouter->RegisterHandler()` — proper multi-handler dispatch
- Casting circle lifecycle (spawn on cast_start, destroy on complete/interrupt)
- Ground AoE indicator with DrawDebugCircle — lightweight, no actor overhead
- Bolt/Projectile/AoE spawning patterns — all 5 templates work correctly
- Cooldown tracking with `FPlatformTime::Seconds()` — works fine

### What Needs Extension (Not Replacement)

| System | Current | Needs |
|--------|---------|-------|
| VFX templates | 9 types | 3-4 new types (see Section 14) |
| Buff tracking | 4 buff names | 40+ buff names |
| Status effect system | 2 (freeze, stone) | 10 core statuses |
| Ground effect system | Fire Wall, Safety Wall | Sanctuary, Pneuma, Quagmire, Ice Wall, traps |
| Damage types | Physical + Magical | Add: Healing, Trap (DEX+INT), Reflect |
| Skill validation | 11 checks | Add: weapon type, item cost, weight limit |

---

## 4. Missing RO Classic Features

### 4A. Passive Skill Bonuses (Priority: CRITICAL)

No passive skills apply any bonuses. Every passive uses the generic fallback handler.

| Skill | ID | Expected Effect | Current State |
|-------|-----|----------------|---------------|
| Sword Mastery | 100 | +4 ATK/level with 1H swords | Fallback (no effect) |
| 2H Sword Mastery | 101 | +4 ATK/level with 2H swords | Fallback |
| Increase HP Recovery | 102 | +5 HP/level per regen tick | Fallback |
| Increase SP Recovery | 204 | +3 SP/level per regen tick | Fallback |
| Owl's Eye | 300 | +1 HIT/level | Fallback |
| Vulture's Eye | 301 | +1 range/level for bows | Fallback |
| Double Attack | 500 | X% chance double hit with daggers | Fallback |
| Improve Dodge | 502 | +1 FLEE/level | Fallback |
| Katar Mastery | 1100 | +3 ATK/level with katars | Fallback |

**Fix approach**: In `getBuffStatModifiers()` or a new `getPassiveStatModifiers()` function, iterate `player.learnedSkills` and apply flat bonuses based on skill type === 'passive'.

### 4B. Healing Skills (Priority: CRITICAL)

| Skill | ID | RO Mechanics | Current State |
|-------|-----|-------------|---------------|
| Heal | 400 | `[(BaseLv + INT) / 8] * (4 + 8*SkillLv)` Holy damage to Undead | Fallback |
| Sanctuary | 1000 | 5x5 ground heal, 1s tick, heals allies / damages Undead | Fallback |
| Resurrection | 1004 | Revive dead player at X% HP | Fallback |
| Kyrie Eleison | 1001 | Absorption barrier (blocks N hits / X damage) | Fallback |

### 4C. Buff Skills (Priority: HIGH)

| Skill | ID | Effect | Duration |
|-------|-----|--------|---------|
| Blessing | 402 | +STR/DEX/INT (= SkillLv) | 60 + Lv*20s |
| Increase AGI | 403 | +AGI (= 3 + Lv) | 60 + Lv*20s |
| Decrease AGI | 404 | -AGI debuff, -25% movement | 40 + Lv*10s |
| Angelus | 406 | +VIT DEF% (= 5*Lv) | 30*Lv seconds |
| Impositio Manus | 1003 | +ATK (= 5*Lv) | 60s |
| Suffragium | 1005 | -15% cast time / level | 30*Lv seconds |
| Aspersio | 1006 | Weapon becomes Holy element | 60*Lv seconds |
| Two-Hand Quicken | 705 | +30% ASPD | 30*Lv seconds |
| Adrenaline Rush | 1200 | +30% ASPD (blacksmith) | 30*Lv seconds |
| Gloria | 1002 | +30 LUK | 10*Lv seconds |
| Magnificat | 1008 | +100% SP regen | 15*Lv seconds |

### 4D. Knockback System (Priority: MEDIUM)

RO Classic has knockback on many skills:
- Magnum Break: 2 cells away from caster
- Fire Wall: 2 cells backward per contact
- Bowling Bash: 1 cell + splash
- Shield Charge/Boomerang: 5/2 cells
- Arrow Repel: 6 cells
- Storm Gust: 2 cells random direction per tick
- Jupitel Thunder: knockback distance = skill level cells

**Implementation needed**:
- Server: Calculate knockback direction + distance, validate destination (walkable cell)
- Server: Emit `combat:knockback { targetId, newX, newY, newZ, distance }` event
- Client: Smooth interpolation to new position
- Boss immunity: Boss monsters are immune to knockback

### 4E. Item/Gem Costs (Priority: MEDIUM)

| Skill | Required Item | Item ID |
|-------|--------------|---------|
| Safety Wall | Blue Gemstone | 717 |
| Warp Portal | Blue Gemstone | 717 |
| Pneuma | (none) | — |
| Magnus Exorcismus | Blue Gemstone | 717 |
| Resurrection | Blue Gemstone (Lv1-3) | 717 |
| Sanctuary | Blue Gemstone | 717 |
| All Traps | Trap item | 1065 |
| Elemental Converters | Specific items | varies |

**Fix**: Add `catalysts: [{ itemId, count }]` field to skill definitions. Check inventory before skill execution. Consume items on successful cast.

### 4F. Weapon Type Requirements (Priority: MEDIUM)

| Skill | Required Weapon |
|-------|----------------|
| Pierce, Brandish Spear, Spear Stab | Spear |
| Two-Hand Quicken | Two-Handed Sword |
| Adrenaline Rush | Axe or Mace |
| Double Attack | Dagger |
| Katar skills | Katar |
| Melody Strike | Instrument |
| Slinging Arrow | Whip |
| Arrow Shower, Double Strafe | Bow |

**Fix**: Add `requiredWeaponTypes: string[]` to skill definitions. Check `player.equipment.weapon.type` before execution.

---

## 5. Server-Side Gaps

### 5A. Skills Without Server Implementation (by class)

**Swordsman** (4/7 implemented): Missing Sword Mastery(100), 2H Sword Mastery(101), Increase HP Recovery(102)

**Mage** (10/13 implemented): Missing Increase SP Recovery(204), Sight Rasher(N/A), Energy Coat(N/A)

**Archer** (0/5 implemented): ALL missing — Double Strafe(302), Arrow Shower(303), Owl's Eye(300), Vulture's Eye(301), Improve Concentration(304)

**Acolyte** (0/13 implemented): ALL missing — Heal(400), Blessing(402), Increase AGI(403), Cure(405), Angelus(406), etc.

**Thief** (0/5 implemented): ALL missing — Double Attack(500), Steal(501), Hiding(503), Envenom(504), Improve Dodge(502)

**Merchant** (0/7 implemented): ALL missing — Mammonite(603), Discount(600), Overcharge(601), Vending(604), etc.

**All 2nd Classes** (0/83 implemented): Knight, Crusader, Wizard, Sage, Hunter, Bard, Dancer, Priest, Monk, Assassin, Rogue, Blacksmith, Alchemist — NONE implemented.

### 5B. Missing Server Validation

| Check | Status | Fix |
|-------|--------|-----|
| Weapon type requirement | MISSING | Add `requiredWeaponTypes` to skill data |
| Item/gem cost | MISSING | Add `catalysts` array to skill data |
| Weight overload (>90%) | MISSING | Check `player.weight / player.maxWeight > 0.9` |
| Skill prerequisites at use time | MISSING | Only checked at learn time |
| Zone restrictions (noskill flag) | MISSING | Check zone flags before skill execution |
| Duplicate ground effects | PARTIAL | Only Safety Wall checked, not Pneuma overlap |

### 5C. Missing Damage Types

| Type | Formula | Used By |
|------|---------|---------|
| Healing | `[(BaseLv + INT) / 8] * (4 + 8*Lv)` | Heal, Sanctuary |
| Trap | `DEX * (3 + BaseLv/100) * (1 + INT/35) * Lv` | All Hunter traps |
| Reflect | % of received damage | Shield Reflect, Blaze Shield |
| Fixed | Exact number (not reduced by DEF) | Acid Terror, Bomb |
| HP% | Percentage of Max HP | Asura Strike (spends all SP) |

---

## 6. Client VFX Gaps

### 6A. Skills With Server Logic But No VFX Config

All 14 implemented skills have VFX configs. No gap here.

### 6B. Missing VFX Templates for Future Skills

Current templates cover bolt, projectile, AoE, ground, buff, debuff, heal. Missing:

| Template Needed | Skills That Need It | Description |
|----------------|--------------------|----|
| **Healing** (distinct from HealFlash) | Heal, Sanctuary | Green/white number rising, soft glow, ground aura for Sanctuary |
| **BarrierShield** | Kyrie Eleison, Assumptio | Translucent sphere around target, shrinks as hits absorbed |
| **GroundTick** | Sanctuary, Quagmire, Song/Dance | Persistent AoE that pulses/ticks visually |
| **KnockbackTrail** | Magnum Break, Fire Wall contact | Trail effect during knockback movement |
| **TrapPlacement** | Land Mine, Ankle Snare, etc. | Small ground object with trigger particle |
| **StatusOverlay** | Freeze, Stone, Poison, Stun | Full-character material swap or overlay |
| **ChanneledBeam** | Jupitel Thunder | Continuous beam from caster to target |
| **SongAura** | Bragi, Whistle, etc. | Radius-based ground aura around performer |

### 6C. Missing Skill VFX Configurations (First Priority)

When these skills get server logic, they'll need VFX configs added to `SkillVFXData.cpp`:

**Archer Skills**:
- Double Strafe (302): Two arrow projectiles → target. Reuse Projectile template.
- Arrow Shower (303): Fan-shaped arrows → AoE. New template or AoEImpact variant.

**Acolyte Skills**:
- Heal (400): Green upward sparkle + number. New Healing template.
- Blessing (402): Golden sparkle. Reuse SelfBuff template (gold color).
- Increase AGI (403): Green wind sparkle. Reuse SelfBuff template (green).
- Angelus (406): Blue shield aura. Reuse SelfBuff template (blue).
- Pneuma (411): Blue barrier on ground. Reuse GroundPersistent (blue).
- Ruwach (407): Brief flash around caster. Reuse AoEImpact (holy).

**Thief Skills**:
- Envenom (504): Green poison slash. Reuse AoEImpact (green).
- Hiding (503): Fade-out effect. Custom (alpha fade on character mesh).

**Merchant Skills**:
- Mammonite (603): Gold coin impact. Reuse AoEImpact (gold).

### 6D. Cascade vs Niagara Decision

**Current split**: 7 skills use Cascade (P_ assets), 8 use Niagara (NS_ assets).

**Recommendation**: Standardize on **Niagara** for all new skills. Keep existing Cascade effects working but don't add new ones. The CascadeLoopTimer hack is sustainable at current scale but won't scale to 100+ concurrent effects.

**Migration plan** (if desired, low priority):
1. Cold Bolt: Replace P_Elemental_Ice_Proj → NS_Ice_Shard (from Free_Magic pack)
2. Provoke: Replace P_Enrage_Base → NS_Fire_Burst (from Mixed_Magic pack)
3. Endure: Replace P_Ice_Proj_charge_01 → NS_Shield_Buff (custom)
4. Sight: Replace P_ElementalFire_Lg → NS_Fire_Aura (from Free_Magic)
5. Frost Diver: Replace P_Ice_Proj_charge_01 → NS_Ice_Lock (custom)
6. Fire Wall: Replace P_Env_Fire_Grate_01 → NS_Ground_Fire (from Mixed_Magic)
7. Safety Wall: Replace P_levelUp_Detail → NS_Protection_Circle (custom)

---

## 7. Targeting & AoE Indicator Gaps

### 7A. Missing Ground AoE Indicators

`GroundAoE::GetAoEInfo()` currently handles 6 skills. Many more ground-targeted skills need entries:

| Skill | ID | AoE Size (RO cells) | UE Radius | Element |
|-------|-----|---------------------|-----------|---------|
| Magnum Break | 105 | 3x3 (self-centered) | 150 | Fire |
| Napalm Beat | 203 | 3x3 | 150 | Ghost |
| Fire Ball | 207 | 5x5 (splash) | 250 | Fire |
| Fire Wall | 209 | 1x3 (line) | 75 | Fire |
| Safety Wall | 211 | 1x1 | 50 | Neutral |
| Thunderstorm | 212 | 5x5 | 250 | Wind |
| **Storm Gust** | — | **11x11** | **550** | **Water** |
| **Lord of Vermilion** | — | **11x11** | **550** | **Wind** |
| **Meteor Storm** | — | **7x7** | **350** | **Fire** |
| **Magnus Exorcismus** | — | **7x7 cross** | **350** | **Holy** |
| **Sanctuary** | — | **5x5** | **250** | **Holy** |
| **Quagmire** | — | **5x5** | **250** | **Earth** |
| **Heaven's Drive** | — | **5x5** | **250** | **Earth** |
| **Pneuma** | — | **3x3** | **150** | **Wind** |

**Fix**: Add cases to `GetAoEInfo()` switch as these skills are implemented.

### 7B. Non-Standard AoE Shapes

The current system only draws circles. RO Classic has non-circular AoEs:

| Shape | Skills | How to Render |
|-------|--------|--------------|
| **Line (1x3)** | Fire Wall | Draw a rectangle instead of circle |
| **Line (1x5)** | Ice Wall | Draw a rectangle |
| **Cross** | Grand Cross, Magnus Exorcismus | Draw two perpendicular rectangles |
| **Fan/Cone** | Arrow Shower, Brandish Spear | Draw a wedge shape |

**Fix**: Extend `GroundAoE::FState` with an `EAoEShape` enum (Circle, Rectangle, Cross, Cone). Update `DrawCircleAtCursor()` to handle each shape.

### 7C. Missing Targeting Indicator for Self-Centered AoEs

Magnum Break (105) is self-centered but still shows a ground indicator. In RO Classic, self-centered skills show the AoE circle locked on the caster (no cursor follow). Currently Magnum Break is in `GetAoEInfo()` but the targeting flow for "self" skills may not enter ground targeting mode.

**Fix**: For self-centered AoE skills, auto-lock the indicator on the player position (no cursor follow phase), then execute immediately.

---

## 8. Status Effect System (Missing)

This is the **biggest missing feature** in the skill system. RO Classic has 10+ status effects with distinct visuals, gameplay mechanics, and cure methods. Currently only Freeze and Stone Curse exist server-side with NO client-side visuals.

### 8A. Required Status Effects

| Status | Visual | Gameplay Effect | Duration | Resist |
|--------|--------|----------------|---------|--------|
| **Frozen** | Blue tint, ice block overlay | Immobile, DEF-50%, element→Water | 3-30s | MDEF, LUK |
| **Petrified** | Grey stone texture | Immobile, DEF-50%, element→Earth, -1%HP/5s | 20s | MDEF, LUK |
| **Stunned** | Stars spinning above head | Cannot move/attack/skill, FLEE=0 | 5s | VIT, LUK |
| **Poisoned** | Green tint, bubble particles | DEF-25%, -1.5%HP/s, SP regen off | 30-60s | VIT, LUK |
| **Silenced** | Speech bubble with X | Cannot use active skills | 30s | VIT, LUK |
| **Blinded** | Dark overlay/cloud | HIT-25%, FLEE-25% | 30s | INT+VIT, LUK |
| **Sleeping** | Zzz particles above head | Immobile, always-hit, 2x crit vulnerability | 30s | INT, LUK |
| **Cursed** | Dark purple aura | ATK-25%, LUK=0, slow movement | 30s | LUK |
| **Bleeding** | Red drip particles | HP loss over time (can kill), regen disabled | 120s | VIT, LUK |
| **Confused** | Spiral/dizzy effect | Movement direction randomized | 30s | STR+INT, LUK |

### 8B. Implementation Plan: Status Effect Visuals

**Option A: Particle Overlay (Recommended)**
- Spawn Niagara/Cascade effect attached to character root component
- Use `ActiveBuffAuras` map (already exists in SkillVFXSubsystem)
- Different effect per status type
- Cleanup on `skill:buff_removed`

**Option B: Material Parameter Override**
- Dynamic material instance on character mesh
- Set color tint (blue for freeze, grey for stone, green for poison)
- More visually authentic to RO Classic
- Requires character mesh to support dynamic material instances

**Recommendation**: Use BOTH — particle overlay for the effect (ice shards, stars, bubbles) + material tint for the character color change. This is what modern RO private servers do.

### 8C. Server-Side Status Effect System Needed

```javascript
// Proposed structure in server
const STATUS_EFFECTS = {
    frozen: {
        blockMovement: true,
        blockAttack: true,
        blockSkills: true,
        defMultiplier: 0.5,
        overrideElement: { type: 'water', level: 1 },
        breakOnDamage: true,
        resistStat: 'mdef',
        immuneAtStatValue: 100  // 100 MDEF = immune
    },
    stunned: {
        blockMovement: true,
        blockAttack: true,
        blockSkills: true,
        fleeOverride: 0,
        breakOnDamage: false,
        resistStat: 'vit',
        immuneAtStatValue: 97
    },
    // ... etc for all 10 statuses
};
```

---

## 9. Buff/Debuff System Gaps

### 9A. Current State

`getBuffStatModifiers()` only recognizes 4 buff names:
- `provoke`: -DEF%, +ATK%
- `endure`: +MDEF
- `frozen`: isFrozen flag, element override
- `stone_curse`: isStoned flag, element override

### 9B. Needed Buff Modifiers

| Buff | Modifier Type | Values |
|------|--------------|--------|
| Blessing | Flat stat bonus | +STR, +DEX, +INT (= SkillLv) |
| Increase AGI | Flat stat bonus | +AGI (= 3 + SkillLv) |
| Decrease AGI | Flat stat reduction + slow | -AGI, -25% move speed |
| Angelus | DEF% multiplier | +5% DEF per level |
| Impositio Manus | Flat ATK bonus | +5 ATK per level |
| Aspersio | Weapon element override | Weapon → Holy |
| Two-Hand Quicken | ASPD% multiplier | +30% ASPD |
| Adrenaline Rush | ASPD% multiplier | +30% ASPD |
| Gloria | Flat stat bonus | +30 LUK |
| Magnificat | SP regen multiplier | +100% SP regen rate |
| Suffragium | Cast time reduction | -15% per level |
| Energy Coat | Damage reduction + SP drain | -6% to -30% damage taken, SP drains on hit |

### 9C. Missing Buff Mechanics

1. **Kyrie Eleison absorption barrier**: Needs hit counter + damage threshold tracking (not just a simple stat modifier)
2. **Buff mutual exclusivity**: Blessing removes Curse, Increase AGI cancels Decrease AGI
3. **Buff removal on death/map change**: All buffs should clear on death
4. **Buff dispelling**: Sage's Dispell should remove most buffs
5. **Buff icons on client**: Need a buff bar UI showing active buffs with countdown timers
6. **Party-wide buffs**: Blessing, Increase AGI should be castable on party members

### 9D. Buff Bar UI (New Widget Needed)

RO Classic shows buff icons in a row at the top of the screen. Each shows:
- Small square icon (32x32)
- Remaining time (countdown or progress bar)
- Tooltip on hover (buff name + effect)
- Max ~30 icons displayed

**Implementation**: New `BuffBarSubsystem` + `SBuffBarWidget` (Slate), Z-order ~11. Listens to `skill:buff_applied` and `skill:buff_removed`. Tracks local player's active buffs.

---

## 10. Code Quality Issues

### 10A. Hardcoded Skill Logic (Server)

All 14 skill implementations use `if (skill.name === 'bash')` chains. This should be migrated to a **strategy pattern** where each skill type has a handler function looked up by name.

```javascript
// Current (fragile):
if (skill.name === 'bash') { ... }
else if (skill.name === 'cold_bolt') { ... }
// etc.

// Better (extensible):
const SKILL_HANDLERS = {
    bash: handleBash,
    cold_bolt: handleBoltSkill,
    fire_bolt: handleBoltSkill,
    // ...
};
const handler = SKILL_HANDLERS[skill.name] || handleGenericSkill;
handler(player, target, skill, level, ...);
```

### 10B. Missing Error Logging in VFX System

`GetOrLoadNiagaraOverride()` and `GetOrLoadCascadeOverride()` silently return nullptr when asset paths are invalid. Add `UE_LOG(LogTemp, Warning, ...)` when `LoadObject` fails so developers know which VFX assets are missing.

### 10C. Socket Event Chain Initialization

SkillVFXSubsystem waits 6 ticks (~3 seconds) before wrapping socket events, hoping all other subsystems have initialized. This is fragile.

**Better approach**: Use `UWorldSubsystem::OnWorldBeginPlay()` with a `bEventsWrapped` guard, and if wrapping fails (no socket), retry on next tick with a counter.

### 10D. VFX Config Rebuild Performance

`BuildSkillVFXConfigs()` rebuilds the entire config map on every `GetSkillVFXConfig()` call. This is intentional (Live Coding safety) but wastes cycles during gameplay.

**Optimization** (optional): Add a frame-cached version that rebuilds only once per frame:
```cpp
static uint64 LastFrameNumber = 0;
if (GFrameCounter != LastFrameNumber) {
    RebuildConfigs();
    LastFrameNumber = GFrameCounter;
}
```

---

## 11. Stale Documentation

### Documents That Need Updating

| Document | Issue | Fix |
|----------|-------|-----|
| `Skill_VFX_Implementation_Plan.md` (2026-03-05) | Says "15 skills need VFX" — 12 are now configured | Update to reflect current state |
| `Skill_VFX_Execution_Plan.md` (2026-03-05) | Lists 11 phases as TODO — Phase 3-9 are done | Mark completed phases |
| `VFX_Asset_Reference.md` | Says InfinityBlade not migrated — code references IB paths | Clarify whether assets exist or not |
| `00_Project_Overview.md` (2026-02-24) | Lists 18 skills — now 15 with full impl | Update counts |

---

## 12. Prioritized Fix Plan

### Phase 1: Critical Fixes (1-2 days)

1. **Verify InfinityBlade asset paths** — Run PIE and check UE_LOG for LoadObject failures. If assets missing, remap to available Niagara packs.
2. **Fix SP deduction timing** — Move from pre-cast to post-cast (in `executeCastComplete()`).
3. **Add VFX load failure logging** — `UE_LOG(Warning)` in `GetOrLoadCascadeOverride/GetOrLoadNiagaraOverride` when LoadObject returns nullptr.
4. **Unify AoE radius data** — Have `GetAoEInfo()` read from `GetSkillVFXConfig()` instead of duplicate switch.

### Phase 2: Passive Skills (1-2 days)

5. **Implement `getPassiveStatModifiers()`** on server — scan `player.learnedSkills` for type==='passive', apply flat bonuses.
6. **Test**: Sword Mastery gives +ATK, Increase HP Recovery boosts regen, Owl's Eye gives +HIT.

### Phase 3: Core Buff System (2-3 days)

7. **Expand `getBuffStatModifiers()`** — Add Blessing, Increase AGI, Angelus, Impositio, Two-Hand Quicken, Gloria, Magnificat, Suffragium.
8. **Implement Heal skill** (400) — New `handleHeal()` function, Healing formula, VFX config (green sparkle).
9. **Implement Blessing** (402) — Buff application, stat modifiers, VFX (gold sparkle).
10. **Implement Increase AGI** (403) — Buff application, AGI modifier, VFX (green wind).

### Phase 4: Status Effects (3-4 days)

11. **Create status effect data table** on server — `STATUS_EFFECTS` object with all 10 statuses.
12. **Add server-side status resistance checks** — VIT/MDEF/LUK resistance formula.
13. **Add Stun status** — Server logic + client particle overlay (stars).
14. **Add Poison status** — Server logic (DoT, DEF reduction) + client tint + particles.
15. **Add Silence status** — Server logic (block skills) + client icon.

### Phase 5: Buff Bar UI (2-3 days)

16. **Create `BuffBarSubsystem`** — New UWorldSubsystem tracking local player buffs.
17. **Create `SBuffBarWidget`** — Slate widget showing buff icons with countdown.
18. **Wire to `skill:buff_applied` / `skill:buff_removed`** events.

### Phase 6: 2nd Class Skills — Wizard (3-4 days)

19. **Jupitel Thunder** — Horizontal beam projectile + knockback.
20. **Storm Gust** — 11x11 AoE, 4.5s duration, 0.5s ticks, freeze chance, knockback.
21. **Lord of Vermilion** — 11x11 AoE, 20 sub-hits, blind chance.
22. **Meteor Storm** — 7x7 area, random meteor impacts, stun chance.
23. **Earth Spike** — Multi-hit earth projectile.
24. **Heaven's Drive** — 5x5 AoE earth damage.
25. **Quagmire** — 5x5 ground debuff (slow).
26. **Ice Wall** — Line of ice blocks (1x5).

### Phase 7: Archer + Thief Skills (2-3 days)

27. **Double Strafe** — Two rapid arrows (Projectile template x2).
28. **Arrow Shower** — Fan-shaped AoE (new AoE shape needed).
29. **Improve Concentration** — Self-buff (+DEX/AGI).
30. **Envenom** — Poison slash + poison status.
31. **Hiding** — Invisibility (character alpha fade).
32. **Steal** — Item theft mechanic.

### Phase 8: Priest Skills (3-4 days)

33. **Sanctuary** — 5x5 ground heal, ticking, anti-Undead.
34. **Kyrie Eleison** — Absorption barrier (hit counter tracking).
35. **Resurrection** — Revive dead player.
36. **Magnus Exorcismus** — 7x7 cross Holy AoE.
37. **Turn Undead** — Instant kill chance vs Undead.
38. **Lex Aeterna** — Double damage debuff.

### Phase 9: Knight + Assassin Skills (2-3 days)

39. **Pierce** — Spear skill, hits based on target size.
40. **Bowling Bash** — Splash knockback.
41. **Two-Hand Quicken** — ASPD buff.
42. **Sonic Blow** — 8-hit combo attack.
43. **Cloaking** — Wall-hugging invisibility.
44. **Grimtooth** — Attack from Hiding.

### Phase 10: Remaining Classes (ongoing)

- Crusader, Hunter, Bard/Dancer, Monk, Rogue, Blacksmith, Alchemist
- Each class: 2-3 days for full skill implementation

---

## 13. Per-Skill Implementation Checklist

Use this checklist when implementing any new skill:

```
[ ] 1. Add/verify skill definition in ro_skill_data.js or ro_skill_data_2nd.js
      - ID, name, displayName, type, targetType, element, range
      - Level scaling: spCost, castTime, cooldown, afterCastDelay, damage%
      - Prerequisites array

[ ] 2. Add server handler in index.js
      - Specific handler (not generic fallback)
      - Damage calculation (physical/magical/healing/trap formula)
      - Status effect application (if applicable)
      - Buff/debuff application (if applicable)
      - Ground effect creation (if applicable)
      - Multi-hit staggering (if applicable)
      - Knockback (if applicable)
      - Item cost consumption (if applicable)
      - Weapon type validation (if applicable)

[ ] 3. Add VFX config in SkillVFXData.cpp
      - Choose template (BoltFromSky/Projectile/AoEImpact/etc.)
      - Set element color
      - Set scale, duration, lifetime
      - Set Niagara/Cascade asset path (prefer Niagara)
      - Set casting circle (if has cast time)
      - Set AoE radius (if ground-targeted)

[ ] 4. Add ground AoE indicator (if ground-targeted)
      - Add case to GroundAoE::GetAoEInfo() in SkillTreeSubsystem.cpp
      - Set radius, element color, effect duration
      - OR read from GetSkillVFXConfig() if unified

[ ] 5. Add skill icon
      - Place texture at /Game/SabriMMO/Assets/Skill_Icons/RO_Skill_Icon_<PascalName>
      - OR add explicit mapping in ResolveIconContentPath()

[ ] 6. Test
      - Verify SP deduction
      - Verify damage/healing numbers
      - Verify VFX spawns and despawns
      - Verify casting circle appears during cast
      - Verify targeting indicator (if ground-targeted)
      - Verify cooldown tracking
      - Verify buff/debuff application and removal
      - Test with 2+ PIE instances (multiplayer safety)
      - Verify zone broadcasting (other players see effects)
```

---

## 14. New VFX Templates Needed

### Template: HealingAura

**For**: Heal, Sanctuary ground effect
**Behavior**: Green/white particles rising upward from target. For Sanctuary: ground circle with periodic upward pulses.
**Niagara**: NS_Heal_Sparkle (create from Mixed_Magic pack base)
**Color**: Green `(0.2, 1.0, 0.3)`

### Template: BarrierShield

**For**: Kyrie Eleison, Assumptio, Energy Coat
**Behavior**: Translucent sphere around character. Shrinks/dims as hits absorbed. Shatters on break.
**Niagara**: NS_Barrier_Sphere (create custom)
**Color**: White/blue `(0.8, 0.9, 1.0)` for Kyrie, gold for Assumptio

### Template: GroundTickAoE

**For**: Sanctuary, Quagmire, Song/Dance effects
**Behavior**: Persistent ground circle that pulses at tick interval (1s for Sanctuary). Different from GroundPersistent (which is static fire/wall).
**Niagara**: NS_Ground_Pulse (create from Free_Magic base)

### Template: StatusOverlay

**For**: All 10 status effects
**Behavior**: Full-character effect — material tint + attached particles
**Implementation**: Not a single Niagara system but a combination of:
- Dynamic material instance (color tint)
- Small particle emitter (ice shards, stars, poison bubbles)
- Attached to character mesh, not world position

---

## 15. RO Classic Reference Data

### Cast Time Formula (Pre-Renewal)
```
FinalCastTime = BaseCastTime * (1 - DEX/150) * (1 - Suffragium%) * (1 - ItemReduction%)
```
- 150 DEX = instant cast
- NO fixed cast time in pre-renewal (all variable)
- Phen Card: +25% cast time, prevents interruption

### AoE Size Conversion (RO Cells → UE Units)
```
1 RO cell ≈ 50 UE units
3x3 = 150 UE radius (from center to edge = 1.5 cells = 75 units... but typically use 150 for full coverage)
5x5 = 250 UE radius
7x7 = 350 UE radius
9x9 = 450 UE radius
11x11 = 550 UE radius
```

### Buff Duration Formulas
| Buff | Duration |
|------|---------|
| Blessing | `60 + SkillLv * 20` seconds |
| Increase AGI | `60 + SkillLv * 20` seconds |
| Angelus | `30 * SkillLv` seconds |
| Kyrie Eleison | 120 seconds (or until hits depleted) |
| Gloria | `10 * SkillLv` seconds |
| Magnificat | `15 * SkillLv` seconds |
| Impositio Manus | 60 seconds (fixed) |
| Suffragium | `30 * SkillLv` seconds |
| Two-Hand Quicken | `30 * SkillLv` seconds |
| Provoke | 30 seconds (fixed) |
| Endure | `10 + SkillLv * 10` seconds (or until hit count depleted) |

### Status Effect Resistance Formula
```
ResistChance = BaseStat * 1% (VIT for Stun, MDEF for Freeze, etc.)
At 97+ stat: immune (except via level superiority)
300 LUK: immune to ALL statuses
Level superiority: if attacker is 10+ levels above target, immunity is ignored
```

### Key Interaction Rules
- Frozen → Water element (weak to Wind: 175% damage)
- Petrified → Earth element (weak to Fire: 175% damage)
- Freeze/Stone breaks on ANY damage hit
- Boss monsters immune to: Stun, Freeze, Stone, Sleep, Poison, Blind, Silence, Confusion, Bleeding, Curse
- Sanctuary heals allies but damages Undead/Demon
- Pneuma blocks ranged physical damage but NOT magic or status effects
- Safety Wall blocks melee physical but NOT ranged or magic
- Storm Gust/Lord of Vermilion do NOT stack from multiple casters
- Meteor Storm DOES stack from multiple casters
- Fire Wall knockback: 2 cells per contact, bosses take all hits but aren't pushed

---

## Conclusion

**The systems do NOT need a refactor.** The architecture is sound, extensible, and follows good patterns. What they need is:

1. **Content expansion** — 125 more skills need server logic, VFX configs, and AoE indicators
2. **Targeted fixes** — SP timing, asset path validation, duplicate data unification
3. **New features** — Status effect visuals, buff bar UI, knockback system, item costs
4. **Documentation update** — Stale VFX plans need to reflect current state

The infrastructure handles everything RO Classic needs. It's a matter of populating it with data and implementing the remaining skill handlers one class at a time.

**Estimated total effort to reach full RO Classic parity**: 30-40 days of focused development across all 13 classes.
