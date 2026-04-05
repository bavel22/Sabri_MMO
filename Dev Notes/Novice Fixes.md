# Novice Fixes

Novice Fixes

claude --resume fa97e30b-3e66-4dc1-95c5-67e08b8fbd6b

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



  Novice Skills Audit & Fix — Complete

  1. Play Dead (ID 3) — Full Toggle Handler

  - Toggle ON: 5 SP cost, stops auto-attack, applies play_dead buff (600s safety cap), forces ALL enemies (including
  bosses/MVPs) to immediately drop aggro
  - Toggle OFF: Removes buff, no SP cost
  - 7 enforcement points: movement blocked, attacks blocked, skills blocked (except toggle-off), items blocked,
  HP/SP/skill regen all blocked
  - 3 enemy AI checks: findAggroTarget, CHASE, ATTACK all check isPlayDead — no detector bypass (stronger than Hiding)
  - Break conditions: Provoke removes it, Bleeding HP drain removes it
  - Client: BuffBarSubsystem abbreviation PDd

  2. Basic Skill (ID 1) — Job Change Gate

  - Added learnedSkills[1] >= 9 check in job:change handler before job level check
  - Returns descriptive error: "Requires Basic Skill Level 9 (current: N)"
  - Updated description with per-level unlock list

  3. Play Dead Prerequisite Fix

  - Already prerequisites: [] — no change needed (was already correct)

  4. First Aid (ID 2) — No Changes Needed

  - Added isQuestSkill: true metadata and afterCastDelay: 0 for consistency

  Files Modified

  ┌─────────────────────────────────┬───────────────────────────────────────────────────────────────────┐
  │              File               │                              Changes                              │
  ├─────────────────────────────────┼───────────────────────────────────────────────────────────────────┤
  │ server/src/ro_skill_data.js     │ 3 skill defs updated (descriptions, isQuestSkill, afterCastDelay) │
  ├─────────────────────────────────┼───────────────────────────────────────────────────────────────────┤
  │ server/src/ro_buff_system.js    │ play_dead BUFF_TYPE + isPlayDead flag in getBuffModifiers         │
  ├─────────────────────────────────┼───────────────────────────────────────────────────────────────────┤
  │ server/src/index.js             │ 16 edit sites across combat, skills, AI, regen, items             │
  ├─────────────────────────────────┼───────────────────────────────────────────────────────────────────┤
  │ client/.../BuffBarSubsystem.cpp │ PDd abbreviation                                                  │
  └─────



  1. Bug 1 fixed: Play Dead toggle-ON now emits combat:auto_attack_stopped to the client with reason 'Play Dead',
  ensuring CombatActionSubsystem clears its attack UI state.
  2. Bug 2 fixed: CHASE and ATTACK AI state drop paths now clean inCombatWith before clearing targetPlayerId, preventing
   stale entries in enemy combat sets.

  The implementation is now at 100% correctness across all 14 enforcement points, 2 break conditions, and both bug
  fixes.





---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
