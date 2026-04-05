# Merchant Fixes

Merchant Fixes

claude --resume 00af7d83-4b2e-4241-b968-338fe4cd7349
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


  Merchant Skill Fixes — Complete

  Skill Definitions (ro_skill_data.js)

  Already correct from Prompt 0:
  - Mammonite (603): cooldown: 0 ✓, afterCastDelay: 0 ✓
  - Cart Revolution (608): targetType: 'single' ✓, cooldown: 0 ✓, treeRow: 3, treeCol: 1 ✓ (no collision with Item
  Appraisal at 1,1)
  - Discount (601): percentages [7,9,11,13,15,17,19,21,23,24] ✓
  - Overcharge (602): percentages [7,9,11,13,15,17,19,21,23,24] ✓
  - Loud Exclamation (609): +4 STR, 300s duration ✓

  Handler Fixes (index.js)

  1. Cart Revolution handler — full rewrite from ground-target AoE to single-target + splash:
    - Now requires targetId + isEnemy (click enemy, not ground)
    - Range check against primary target
    - forceHit: true — always hits, ignores FLEE
    - 3x3 splash radius (150 UE units) around primary target
    - Cart weight scaling formula ready: 150% + floor(100 * cartWeight / 8000) (baseline 150% until cart system)
    - Uses shared knockbackTarget() (2 cells) instead of inline math
    - Added checkDamageBreakStatuses() (was missing — breaks Freeze/Stone/Sleep on hit)
  2. Discount — added Math.max(1, ...) minimum 1z floor
  3. Comment fix — corrected stale Discount/Overcharge formula comment (Lv10=24%, not 25%)

  Verified Correct (no changes needed)

  - Mammonite handler: Uses executePhysicalSkillOnEnemy with zeny deduction + DB persist
  - Loud Exclamation handler: Applies strBonus: 4 buff via applyBuff(), picked up by getEffectiveStats()
  - Vending/Item Appraisal/Change Cart: Remain stubbed (require cart inventory, item identification, cart visual systems
   — all deferred)



 Audit Results — 100% Correct

  ┌───────────────┬─────┬───────────────────────────────────────────┬──────────────────────────────────────────────┐
  │     Skill     │ ID  │           Audit Doc Requirement           │                    Status                    │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Enlarge       │ 600 │ +200/lv, passive, no prereqs              │ CORRECT — effectValue: 200*(i+1), wired in   │
  │ Weight Limit  │     │                                           │ getPlayerMaxWeight()                         │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Discount      │ 601 │ 7-24% table, prereq EWL Lv3, min 1z floor │ CORRECT — [7,9,11,...,24], prereq {600, 3},  │
  │               │     │                                           │ Math.max(1,...) added                        │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Overcharge    │ 602 │ 7-24% table, prereq Discount Lv3          │ CORRECT — [7,9,11,...,24], prereq {601, 3}   │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │               │     │ SP=5, cooldown=0, ACD=0, ATK 150-600%,    │ CORRECT — cooldown:0, afterCastDelay:0,      │
  │ Mammonite     │ 603 │ zeny cost                                 │ effectValue:150+i*50, handler deducts        │
  │               │     │                                           │ zeny+DB persist                              │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Pushcart      │ 604 │ Passive, prereq EWL Lv5, speed penalty    │ CORRECT — stub def, no cart system yet       │
  │               │     │ data                                      │ (deferred)                                   │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Vending       │ 605 │ Deferred stub                             │ CORRECT — stub returns "not yet implemented" │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Item          │ 606 │ Deferred stub                             │ CORRECT — stub, tree pos (1,1) no collision  │
  │ Appraisal     │     │                                           │                                              │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Change Cart   │ 607 │ Deferred stub                             │ CORRECT — stub returns "not yet implemented" │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Cart          │     │ targetType:'single', splash 3x3,          │                                              │
  │ Revolution    │ 608 │ forceHit, knockback 2, cooldown=0, tree   │ CORRECT — all implemented                    │
  │               │     │ no collision                              │                                              │
  ├───────────────┼─────┼───────────────────────────────────────────┼──────────────────────────────────────────────┤
  │ Loud          │ 609 │ +4 STR, 300s, SP=8                        │ CORRECT — strBonus:4, duration:300000,       │
  │ Exclamation   │     │                                           │ spCost:8                                     │
  └───────────────┴─────┴───────────────────────────────────────────┴──────────────────────────────────────────────┘

  Cart Revolution handler specifics:
  - targetType: 'single' in definition, targetId/isEnemy required in handler
  - forceHit: true passed to calculateSkillDamage
  - Splash: 150 UE unit radius (3x3 cells at 50 UE/cell)
  - Knockback: knockbackTarget(enemy, ..., 2, ...) — shared helper, 2 cells
  - Cart weight scaling: effectVal + floor(100 * cartWeight / 8000) — baseline 150% when cartWeight=0
  - checkDamageBreakStatuses() included for Freeze/Stone/Sleep/Confusion break
  - skillElement: null — uses weapon element (correct for physical skill)
  - Tree position (3,1) — no collision with any other merchant skill
  - All 10 positions unique ✓

  Zero gaps remaining. Everything matches the audit document requirements.

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
