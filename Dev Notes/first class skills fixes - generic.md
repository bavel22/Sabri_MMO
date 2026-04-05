# first class skills fixes - generic

first class skills fixes - generic

claude --resume 8fbc7cc4-2469-480f-a6e0-0c9439ece817

Phase 0
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



  Task 1 — Skill Definition Sweep (22 field changes across 20 skills):

  ┌───────────────────────┬────────────────────────────────────────────────────────────────────────────────────────┐
  │         Skill         │                                        Changes                                         │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Play Dead (3)         │ prerequisites [] (was Basic Skill Lv1)                                                 │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Bash (103)            │ cooldown 0 (was 700)                                                                   │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Magnum Break (105)    │ targetType aoe (was ground), range 0 (was 50), ACD 2000 (was 0), cooldown 0 (was 2000) │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Stone Curse (206)     │ range 100 (was 300)                                                                    │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Soul Strike (210)     │ ACD zigzag [1200,1000,1400,1200,1600,1400,1800,1600,2000,1800] (was linear)            │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Safety Wall (211)     │ Lv3 castTime 3000 (was 3500)                                                           │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Double Strafe (303)   │ cooldown 0 (was 300)                                                                   │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Arrow Shower (304)    │ ACD 1000 (was 0), cooldown 0 (was 1000)                                                │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Heal (400)            │ ACD 1000 + cooldown 0 (was cooldown 1000)                                              │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Increase AGI (403)    │ ACD 1000 (was missing)                                                                 │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Decrease AGI (404)    │ duration (3+i)*10000 = 30-120s (was 40-130s)                                           │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Cure (405)            │ cooldown 0 (was 1000)                                                                  │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Signum Crucis (407)   │ duration 86400000 (was 0, now permanent)                                               │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Steal (502)           │ effectValue 10+i*6 = 10-64 (was 10+i*5 = 10-55), cooldown→ACD                          │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Hiding (503)          │ cooldown 0 (was 500)                                                                   │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Envenom (504)         │ cooldown 0 (was 500), range 100 (was 150)                                              │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Detoxify (505)        │ cooldown 0 (was 500)                                                                   │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Sand Attack (506)     │ cooldown 0 (was 1000)                                                                  │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Back Slide (507)      │ ACD 0 (was 500)                                                                        │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Throw Stone (508)     │ range 350 (was 700)                                                                    │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Mammonite (603)       │ cooldown 0 (was 800)                                                                   │
  ├───────────────────────┼────────────────────────────────────────────────────────────────────────────────────────┤
  │ Cart Revolution (608) │ targetType single (was ground), cooldown 0 (was 500), treeRow 3 (was 1, fix collision) │
  └───────────────────────┴────────────────────────────────────────────────────────────────────────────────────────┘

  Task 2 — ForceHit Infrastructure: executePhysicalSkillOnEnemy now passes forceHit from options through to
  calculateSkillDamage. The flag was already generic in ro_damage_formulas.js (skips PD + HIT/FLEE checks).

  Task 3 — Boss/Undead Immunity:
  - Provoke: Boss flag + Undead element immunity before SP deduction
  - Decrease AGI: Boss flag immunity before SP deduction



 Everything is correct. Here's the final verification summary:

  34/34 skill definition checks PASS — all cooldowns, afterCastDelays, ranges, effectValues, SP costs, targetTypes,
  treePositions, and prerequisites match audit docs exactly.

  3/3 index.js changes verified:
  - executePhysicalSkillOnEnemy line 772: passes forceHit: options.forceHit || false through to calculateSkillDamage
  - Provoke lines 5661-5672: Boss + Undead element immunity before SP deduction, gated by isEnemy
  - Decrease AGI lines 7153-7157: Boss immunity right after target resolution, before SP deduction, gated by isEnemy

  Teleport (409) Lv2 SP cost fix added (was the one gap caught during review): Lv1=10, Lv2=9.

  Syntax: Both files pass node -c cleanly. 291 skills load without errors.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
