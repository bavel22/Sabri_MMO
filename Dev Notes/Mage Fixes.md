# Mage Fixes

Mage Fixes

claude --resume 4e258de1-992a-440f-bbbe-257ad8674198

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


 Mage Fixes — Complete

  Skill Data (already correct, no changes needed)

  - Soul Strike (210): ACD zigzag pattern already correct
  - Safety Wall (211): Lv3 cast time already 3000ms
  - Stone Curse (206): Range already 100 UE units

  Handler Fixes (index.js)

  ┌─────┬─────────────────────────┬───────────────────────────────────┬─────────────────────────────────────────────┐
  │  #  │           Fix           │              Before               │                    After                    │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 1   │ Thunderstorm (212)      │ Hardcoded 100 MATK%               │ Uses effectVal (80%)                        │
  │     │ damage                  │                                   │                                             │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 2   │ Thunderstorm AoE radius │ STORM_AOE = 500                   │ 250 (5x5 cells)                             │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 3   │ Napalm Beat (203) AoE   │ NAPALM_AOE = 300                  │ 150 (3x3 cells)                             │
  │     │ radius                  │                                   │                                             │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 4   │ Fire Ball (207) AoE     │ FIREBALL_AOE = 500                │ 250 (5x5 cells)                             │
  │     │ radius                  │                                   │                                             │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 5   │ Fire Ball edge damage   │ 75% outer ring reduction          │ Removed — full damage to all targets        │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 6   │ Stone Curse (206)       │ Directly applied stone            │ Phase 1 petrifying (5s) → Phase 2 stone     │
  │     │ two-phase               │                                   │ (20s) auto-transition                       │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 7   │ Stone Curse resistance  │ effectVal - targetHardMdef        │ Raw effectVal passed to applyStatusEffect() │
  │     │                         │ (double-reduction)                │  engine                                     │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 8   │ Stone Curse             │ Not implemented                   │ Casting on petrifying target cures it       │
  │     │ counter-cast            │                                   │                                             │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 9   │ Stone Curse gem Lv6-10  │ Always consumed                   │ Lv1-5 always, Lv6-10 on success only        │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 10  │ Frost Diver (208)       │ (35+lv*3) - MDEF then engine      │ Raw (35+lv*3) — engine handles MDEF         │
  │     │ freeze chance           │ subtracts again                   │ resistance                                  │
  ├─────┼─────────────────────────┼───────────────────────────────────┼─────────────────────────────────────────────┤
  │ 11  │ Frost Diver freeze      │ No MDEF/LUK reduction             │ duration - duration*MDEF/200 - 10*LUK       │
  │     │ duration                │                                   │                                             │
  └─────┴─────────────────────────┴───────────────────────────────────┴─────────────────────────────────────────────┘

  New Systems (ro_status_effects.js)

  - petrifying status type: 5s Phase 1 — can move, cannot attack/cast, NOT broken by damage
  - transitionsTo: 'stone': auto-transition in tickStatusEffects() when Phase 1 expires
  - Player + enemy tick loops broadcast status:applied / skill:buff_applied on transition

  Increase SP Recovery (204) Fixes

  - MaxSP% regen: Added floor(MaxSP * 0.002 * SkillLv) per skill regen tick
  - SP item potency: Added +2%/level to SP recovery items in inventory:use handler


 537/537 element table tests pass. Zero regressions.

  Final Audit Summary — 100% Verified

  All 76 programmatic assertions pass. 537 existing tests pass. 0 regressions.

  Changes verified against requirements:

  ┌─────┬──────────────────────────────────────────┬──────────────┬─────────────────────────────────────────────────┐
  │  #  │               Requirement                │    Status    │                  Verification                   │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 1   │ Thunderstorm: effectVal (80%) not        │ DONE         │ Both PvE + PvP loops use effectVal              │
  │     │ hardcoded 100                            │              │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 2   │ Soul Strike: ACD zigzag pattern          │ Already      │ All 10 levels match canonical                   │
  │     │                                          │ correct      │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 3   │ Napalm Beat: AoE 300→150                 │ DONE         │ NAPALM_AOE = 150                                │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 4   │ Fire Ball: AoE 500→250, no edge damage   │ DONE         │ FIREBALL_AOE = 250, FIREBALL_INNER/edgeMult     │
  │     │                                          │              │ removed                                         │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 5   │ Stone Curse: range 300→100               │ Already      │ range: 100                                      │
  │     │                                          │ correct      │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 6   │ Stone Curse: two-phase petrification     │ DONE         │ petrifying (5s) → auto-transition → stone (20s) │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 7   │ Stone Curse: petrify chance with MDEF    │ DONE         │ Raw effectVal to applyStatusEffect(), no        │
  │     │ resistance                               │              │ double-reduction                                │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 8   │ Stone Curse: counter-cast cures          │ DONE         │ isPetrifying check → removeStatusEffect         │
  │     │ petrifying                               │              │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 9   │ Stone Curse: gem Lv6-10 success-only     │ DONE         │ Lv1-5 always consumed, Lv6-10 on success        │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 10  │ Safety Wall: Lv3 cast time 3500→3000     │ Already      │ All 10 levels match canonical                   │
  │     │                                          │ correct      │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 11  │ Frost Diver: freeze chance with MDEF     │ DONE         │ Raw (35+3*lv) to engine, no pre-subtraction     │
  │     │ resistance                               │              │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 12  │ Frost Diver: freeze duration 3*level     │ DONE         │ learnedLevel * 3000 with MDEF/LUK reduction     │
  │     │ with reduction                           │              │                                                 │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 13  │ Inc SP Recovery: MaxSP% regen component  │ DONE         │ floor(MaxSP * 0.002 * SkillLv) per tick         │
  ├─────┼──────────────────────────────────────────┼──────────────┼─────────────────────────────────────────────────┤
  │ 14  │ Inc SP Recovery: SP item potency         │ DONE         │ Applied in inventory:use heal handler           │
  │     │ +2%/level                                │              │                                                 │
  └─────┴──────────────────────────────────────────┴──────────────┴─────────────────────────────────────────────────┘

  Additional fixes found during audit:

  ┌─────┬────────────────────────────────────────────────────────────┬──────────────────────────────────────────────┐
  │  #  │                           Issue                            │                     Fix                      │
  ├─────┼────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ 15  │ getCombinedModifiers missing isPetrifying                  │ Added isPetrifying: statusMods.isPetrifying  │
  │     │                                                            │ || false                                     │
  ├─────┼────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ 16  │ Thunderstorm AoE was 500 (not in original prompt)          │ Fixed to 250 (5x5 cells)                     │
  ├─────┼────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ 17  │ Thunderstorm/Napalm Beat/Frost Diver missing               │ Added to all 3 (PvE + PvP paths)             │
  │     │ checkDamageBreakStatuses                                   │                                              │
  └─────┴────────────────────────────────────────────────────────────┴──────────────────────────────────────────────┘


---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
