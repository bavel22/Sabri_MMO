# Skill Fix Execution Plan — Prompt Sequence

**Date:** 2026-03-14
**Total Skills:** 69 across 7 classes
**Fully Correct:** 16 (no work needed)
**Fixable Gaps:** 47
**Deferred:** 6 (need whole new game systems)

---

## Shared Systems (Must Be Built First)

These cross-cutting systems are referenced by 2+ class audits. Building them first prevents duplication and rework.

| System | Used By | Docs |
|--------|---------|------|
| afterCastDelay vs cooldown sweep | ALL classes | 7 |
| Boss/Undead immunity on debuffs | Swordsman, Mage, Acolyte, Thief | 4 |
| Force-hit flag in damage calc | Merchant, Thief, (Archer) | 3 |
| Movement/action blocking for buffs | Novice, Thief, (Mage) | 3 |
| Toggle skill handler pattern | Novice, Thief, Swordsman | 3 |
| Knockback shared helper | Swordsman, Archer, Merchant | 3 |
| Status resistance with level/LUK | Swordsman, Mage, Acolyte, Thief | 4 |
| Regen blocking while in status | Novice, Thief, (Mage) | 3 |

---

## Prompt Execution Order

### PROMPT 0 — Shared Infrastructure Sweep
**Run this FIRST before any class-specific prompts.**

```
Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-buff /sabrimmo-debuff

Read the audit docs:
- docsNew/05_Development/Skill_Fix_Execution_Plan.md (this file)

Then implement these cross-cutting fixes in server/src/:

1. SKILL DEFINITION SWEEP (ro_skill_data.js):
   Fix ALL cooldown vs afterCastDelay confusion across all 7 classes.
   Many skills have values in `cooldown` that should be in `afterCastDelay`,
   or have spurious non-zero cooldowns that should be 0.
   Use the "Skill Definition Corrections" table from EACH class audit doc:
   - docsNew/05_Development/Novice_Skills_Audit_And_Fix_Plan.md
   - docsNew/05_Development/Swordsman_Skills_Audit_And_Fix_Plan.md
   - docsNew/05_Development/Mage_Skills_Audit_And_Fix_Plan.md
   - docsNew/05_Development/Archer_Skills_Audit_And_Fix_Plan.md
   - docsNew/05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md
   - docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md
   - docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md

   For each skill, apply the corrections listed in that class's
   "Skill Definition Corrections" table: fix cooldowns, afterCastDelays,
   ranges, effectValues, SP costs, and any other field-level fixes.
   This is a bulk data fix — no handler logic changes.

2. FORCE-HIT FLAG (ro_damage_formulas.js + index.js):
   Add `forceHit: true` support to calculatePhysicalDamage().
   When forceHit is true, skip the HIT/FLEE accuracy check (always hit).
   Used by: Cart Revolution, Throw Stone, left-hand dual wield (already exists).
   Verify the existing dual wield forceHit path and make it generic.

3. BOSS/UNDEAD IMMUNITY STANDARDIZATION (index.js):
   Audit all debuff-applying skill handlers. Ensure every one checks:
   - `enemy.modeFlags?.boss` → immune to Provoke, Decrease AGI, status effects
   - `enemy.race === 'undead'` → immune to Provoke (some versions), poison
   Add missing checks per each class audit doc.

Do NOT change any handler logic — only data definitions and the forceHit
infrastructure. Commit message: "fix: skill definition sweep — cooldowns,
ranges, forceHit flag, immunity checks"
```

---

### PROMPT 1 — Swordsman Fixes
**Dependencies: Prompt 0 (skill defs already fixed)**

```
Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-buff

Read: docsNew/05_Development/Swordsman_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Swordsman audit doc:

1. Increase HP Recovery (102): Wire `hpRegenBonus` into the HP regen tick
   so the passive actually does something. Add the +0.2% MaxHP/level
   component. Add +10%/level item heal bonus in inventory:use handler.

2. Bash (103): Add multiplicative HIT bonus (+5% to +50%) via
   skillOptions.hitBonus. Fix stun formula to use
   `(bashLv-5)*baseLv/10` instead of flat 5-25%.

3. Provoke (104): Add Undead element immunity check. Add Boss-type
   immunity check. Don't deduct SP when target is immune.

4. Magnum Break (105): Change handler from ground AoE to self-centered
   AoE (use caster position, not groundX/Y/Z). Add HP cost
   `21 - ceil(level/2)`. Add 10-second fire endow buff that gives
   +20% fire element bonus to normal attacks.

5. Endure (106): Add 7-hit count tracking. Decrement counter when
   player takes monster hits. Remove buff when count reaches 0.

6. Moving HP Recovery (108): Change regen rate while moving from
   100% to 50% of standing rate.

7. Auto Berserk (109): Add -55% VIT DEF penalty (currently only
   applies +32% ATK).

8. Fatal Blow (107): Fix stun formula to
   `(bashLv - 5) * baseLv / 10` instead of flat `(lv-5)*5`.

Test by verifying Bash stun, Provoke immunity, Magnum Break HP cost,
Endure hit count, and Auto Berserk DEF penalty.
```

---

### PROMPT 2 — Mage Fixes
**Dependencies: Prompt 0**

```
Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-debuff

Read: docsNew/05_Development/Mage_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Mage audit doc:

1. Thunderstorm (212): Fix handler to use effectVal (80%) instead
   of hardcoded 100% MATK per hit.

2. Soul Strike (210): Fix the afterCastDelay array to use the
   canonical zigzag pattern:
   [1200, 1000, 1400, 1200, 1600, 1400, 1800, 1600, 2000, 1800]

3. Napalm Beat (203): Fix AoE radius from 300 to 150 UE units
   (3x3 cells, not 6x6).

4. Fire Ball (207): Fix AoE radius from 500 to 250 UE units.
   Remove the non-canonical 75% outer ring damage reduction
   (all targets in AoE take full damage).

5. Stone Curse (206): Fix range from 300 to 100 UE units (2 cells).
   Implement two-phase petrification:
   - Phase 1 "Petrifying": 5 seconds, can still move but no
     attack/skill. Add as a new status type in ro_status_effects.js.
   - Phase 2 "Stone": transitions automatically after Phase 1.
     HP drain 1%/5s, DEF -50%, MDEF +25%, element becomes Earth Lv1.
   Fix petrify chance to (20+4*lv)% with MDEF resistance.

6. Safety Wall (211): Fix Lv3 cast time from 3500 to 3000ms.

7. Frost Diver (208): Verify freeze chance uses (35+3*lv)% with
   MDEF resistance. Verify freeze duration is 3*level seconds.

8. Increase SP Recovery (204): Add MaxSP% regen component and
   +2%/level SP item potency bonus if documented in audit.

Test by verifying Thunderstorm damage reduction, Stone Curse
two-phase behavior, Napalm Beat splash radius, Fire Ball radius.
```

---

### PROMPT 3 — Thief Fixes
**Dependencies: Prompt 0**

```
Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-debuff /sabrimmo-buff

Read: docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Thief audit doc:

PHASE A — Critical formula fixes:

1. Envenom (504): REWRITE the handler completely. Do NOT use
   executePhysicalSkillOnEnemy. Instead:
   - Run calculatePhysicalDamage with skillMultiplier=100 (normal ATK)
     and forceElement='poison'
   - Add flat bonus: 15 * learnedLevel (bypasses ALL DEF)
   - If main attack misses: damage = flat bonus only (bonus always hits)
   - If main attack hits: damage = result.damage + flat bonus
   - Apply poison with chance (10 + 4*learnedLevel)% and 60s duration
   - Check undead/boss immunity before applying poison

2. Throw Stone (508): Fix damage to flat 50 (remove STR bonus).
   Add 3% stun chance. Fix range to 350 UE units.
   NOTE: Stone item consumption deferred to Phase B.

3. Sand Attack (506): Fix blind chance to flat 20% base.

PHASE B — Missing features:

4. Hiding (503): Add SP drain tick — 1 SP every (4+skillLevel) seconds.
   Piggyback on existing 1-second buff tick. Store `lastDrainTime` and
   `hidingLevel` on the buff object. When SP hits 0, remove hiding.
   Add movement prevention (reject player:position while hidden).
   Add auto-attack prevention (block combat:attack while hidden).
   Add HP/SP regen blocking while hidden.
   Add item use prevention (block inventory:use while hidden).

5. Pick Stone (509): Add Stone item (ID 7049, weight 3) to items
   table via migration. Handler should: check weight < 50%, insert
   Stone into character_inventory, update currentWeight, emit
   inventory:data.

6. Throw Stone (508): Add Stone consumption — check player has
   Stone (7049) in inventory, consume 1, update weight.

PHASE C — Steal:

7. Steal (502): Fix success formula to canonical
   `(4 + 6*SkillLv) + (DEX - MonsterDEX)/2`.
   Add boss immunity check.
   Add actual item acquisition: roll against monster drop table
   from ro_monster_templates.js, insert into character_inventory,
   update weight, emit inventory:data.
   If monster has no drop data, give a generic loot item.

Test: Envenom flat bonus on miss, Hiding SP drain to 0, Pick Stone
weight check, Throw Stone stone consumption, Steal item acquisition.
```

---

### PROMPT 4 — Archer Fixes
**Dependencies: Prompt 0**

```
Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-buff

Read: docsNew/05_Development/Archer_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Archer audit doc:

1. Double Strafe (303): Restructure from 2 separate damage events
   to 1 bundled hit. Calculate total damage as
   2 * (90+10*lv)% ATK, broadcast as single skill:effect_damage
   with the combined total. Remove spurious cooldown.

2. Arrow Shower (304): Fix AoE radius from 400 to 125 UE units
   (5x5 cells = ~125 UE). Add knockback: push all hit targets
   2 cells AWAY FROM the ground target center (not caster).
   Move cooldown to afterCastDelay (1000ms ACD).

3. Vulture's Eye (301): Fix range bonus from +10 to +50 UE units
   per level (1 cell = 50 UE units).

4. Improve Concentration (302): Fix stat source filtering —
   AGI/DEX percentage bonus should apply to (base + job bonus +
   equipment) only, NOT to buffs/cards. Add server-side reveal
   hidden execution (currently only broadcasts revealHidden flag
   but doesn't actually break hiding on nearby hidden players).

5. Arrow Repel (306): Fix knockback distance from 250 to 300 UE
   units (6 cells). Use shared knockbackTarget() helper.

Arrow Crafting (305) is deferred — needs ammunition system.

Test: Double Strafe single-hit display, Arrow Shower knockback
direction, Vulture's Eye range at max level.
```

---

### PROMPT 5 — Acolyte Fixes
**Dependencies: Prompt 0, ideally after Prompt 3 (Thief/Hiding for hidden reveal)**

```
Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-buff /sabrimmo-debuff

Read: docsNew/05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Acolyte audit doc:

1. Blessing (402): Add enemy targeting path — when cast on
   Undead/Demon race enemy, apply debuff that halves STR/DEX/INT.
   When cast on cursed/stoned friendly, cure those statuses first.
   Keep existing friendly buff path unchanged.

2. Increase AGI (403): Add 15 HP cost per cast (check player has
   >= 16 HP, deduct 15 HP). Add missing afterCastDelay (1000ms).
   Add interaction: casting Increase AGI removes Decrease AGI and
   vice versa (they cancel each other).

3. Decrease AGI (404): Add success rate formula:
   `(40 + 2*SkillLv)% - TargetMDEF + (CasterBaseLv - TargetBaseLv)
    + CasterINT/5`. Clamp 5-95%.
   Add boss immunity check.
   Fix duration formula. Add Increase AGI cancellation.

4. Signum Crucis (407): Add success rate: (23+4*lv)%.
   Make effect permanent (no duration) — only removed by death.
   Fix AoE range. Only affect Undead/Demon race enemies.

5. Holy Light (414): Add handler — 125% MATK Holy element,
   single target, 2s cast time, 9 cells range. Standard magic
   damage pipeline via calculateMagicSkillDamage().

6. Angelus (406): Verify DEF% values per level. Party-wide
   application deferred until party system exists.

7. Pneuma (411): Add overlap prevention — cannot place Pneuma
   where Safety Wall exists and vice versa.

8. Warp Portal (410): Add Blue Gemstone consumption check.
   Memorize system deferred.

Test: Blessing on undead enemy, Decrease AGI success rate,
Increase AGI HP cost, Holy Light damage, Pneuma/Safety Wall overlap.
```

---

### PROMPT 6 — Merchant Fixes
**Dependencies: Prompt 0**

```
Load skills: /sabrimmo-skills /sabrimmo-combat

Read: docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Merchant audit doc:

1. Mammonite (603): Cooldown already fixed to 0 in Prompt 0.
   Verify handler works correctly with no cooldown (ASPD-based).

2. Cart Revolution (608): Fix targetType from 'ground' to 'single'
   (click enemy, splash hits 3x3 around that enemy). Add forceHit
   (Cart Revolution always hits, ignores FLEE). Fix tree position
   collision (move to non-conflicting treeRow/treeCol).
   Cart weight scaling deferred (use baseline 150% until cart
   inventory system exists).

3. Verify Discount (601) and Overcharge (602) percentage tables
   are correct per audit doc.

4. Verify Loud Exclamation (609) is correct (+4 STR, 300s).

Vending, Item Appraisal, and Change Cart are all deferred —
need entire new game systems (cart inventory, item identification).

Test: Cart Revolution targeting flow (click enemy not ground),
Mammonite with no cooldown.
```

---

### PROMPT 7 — Novice Fixes
**Dependencies: Prompt 0, Prompt 3 (Hiding pattern for Play Dead)**

```
Load skills: /sabrimmo-skills /sabrimmo-buff

Read: docsNew/05_Development/Novice_Skills_Audit_And_Fix_Plan.md

Implement ALL fixes from the Novice audit doc:

1. Play Dead (3): Implement full toggle handler modeled after
   Hiding (503) but with these differences:
   - Blocks movement, attacks, skills, items, HP/SP regen
   - ALL monsters (including MVPs/bosses) drop aggro immediately
   - Does NOT make player invisible (unlike Hiding)
   - 5 SP cost to activate
   - Duration: 600 seconds (fixed, not level-scaled)
   - Break conditions: using skill again (manual cancel),
     taking damage from Dispel/Provoke, Bleeding HP drain

2. Basic Skill (1): Add Lv9 check to job:change handler.
   Currently only checks Job Level 10.

3. Play Dead prerequisite: Fix from Basic Skill Lv1 to no
   skill prerequisite (quest-learned).

4. First Aid (2): No changes needed (already correct).

Test: Play Dead toggle on/off, monster deaggro, movement block,
job change with Basic Skill < 9.
```

---

## Execution Order Summary

```
PROMPT 0: Shared Infrastructure Sweep          ← DO FIRST (unblocks all)
    |
    +--→ PROMPT 1: Swordsman Fixes             ← Independent
    +--→ PROMPT 2: Mage Fixes                  ← Independent
    +--→ PROMPT 3: Thief Fixes                 ← Independent
    +--→ PROMPT 4: Archer Fixes                ← Independent
    +--→ PROMPT 6: Merchant Fixes              ← Independent
    |
    +--→ PROMPT 5: Acolyte Fixes               ← After Prompt 3 (hidden reveal)
    +--→ PROMPT 7: Novice Fixes                ← After Prompt 3 (Hiding pattern)
```

**Prompts 1-4 and 6 can run in any order** after Prompt 0.
**Prompt 5 (Acolyte)** benefits from Prompt 3 (Thief) being done first
because Ruwach/Sight reveal needs the Hiding SP drain system.
**Prompt 7 (Novice)** benefits from Prompt 3 (Thief) being done first
because Play Dead reuses the Hiding toggle pattern.

**Recommended sequential order:** 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7

---

## Deferred Systems (Not in Prompts Above)

These require entirely new game systems and should be separate projects:

| System | Skills Blocked | Effort |
|--------|---------------|--------|
| Ammunition/Arrow system | Arrow Crafting (305) | Large |
| Cart inventory (8000 weight, 100 slots) | Cart Rev weight scaling, Vending, Change Cart | Large |
| Item identification system | Item Appraisal (606) | Medium |
| Warp Portal memorize system | Warp Portal full functionality (410) | Medium |
| Quest skill learning infrastructure | 15+ quest skills across all classes | Medium |
| Party-wide buff broadcasting | Angelus party-wide (406) | Medium (needs party system) |
