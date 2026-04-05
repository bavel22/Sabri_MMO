#  Swordsman Fixes

 Swordsman Fixes

claude --resume 099f839c-3136-4c28-adf6-f90b9765edc6

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

     4. Magnum Break (105): DONT CHANGE HANDLER, keep ground targetted. Add HP cost
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


All formulas match the audit document exactly. Here's a summary of all 8 fixes implemented:

  Changes Made

  server/src/ro_buff_system.js

  1. Auto Berserk DEF penalty: Changed defMultiplier *= 0.75 (-25%) to *= 0.45 (-55%) — matches RO Classic self-Provoke
  Lv10
  2. Magnum Break Fire buff type: Added magnum_break_fire to BUFF_TYPES and getBuffModifiers() switch

  server/src/index.js

  ┌─────┬───────────────────┬─────────────────────────────────────────────────────┬────────────────────────────────┐
  │  #  │       Skill       │                         Fix                         │             Lines              │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 1   │ Inc HP Recovery   │ Wired hpRegenBonus + +0.2% MaxHP/lv into HP regen   │ ~10473-10476                   │
  │     │ (102)             │ tick                                                │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 2   │ Inc HP Recovery   │ Added +10%/lv item heal bonus in inventory:use      │ ~8096-8099                     │
  │     │ (102)             │ handler                                             │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 3   │ Bash (103)        │ Added multiplicative HIT bonus +5%/lv via           │ ~5557-5563                     │
  │     │                   │ skillHitBonus                                       │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 4   │ Bash/Fatal Blow   │ Fixed stun formula from flat (lv-5)*5 to            │ ~5617                          │
  │     │                   │ (lv-5)*baseLv/10                                    │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 5   │ Provoke (104)     │ Already implemented — Undead element + Boss type    │ 5666-5675                      │
  │     │                   │ immunity before SP deduction                        │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 6   │ Magnum Break      │ Added HP cost 21-ceil(lv/2) (20→16), cannot kill    │ ~5770-5773                     │
  │     │ (105)             │ caster                                              │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 7   │ Magnum Break      │ Added 10s fire endow buff (+20% fire bonus on       │ ~5921-5930 (apply) +           │
  │     │ (105)             │ auto-attacks)                                       │ ~9909-9920 (tick hook)         │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 8   │ Endure (106)      │ Added hitCount: 7 tracking — decrements on monster  │ ~5940 (apply) + ~11480-11490   │
  │     │                   │ hits, removes buff at 0                             │ (decrement)                    │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 9   │ Moving HP         │ Changed moving regen from 100% to 50% of standing   │ ~10463-10485                   │
  │     │ Recovery (108)    │ rate                                                │                                │
  ├─────┼───────────────────┼─────────────────────────────────────────────────────┼────────────────────────────────┤
  │ 10  │ Auto Berserk      │ -55% VIT DEF penalty (was -25%) — handled in        │ buff_system:380                │
  │     │ (109)             │ ro_buff_system.js                                   │                                │
  └─────┴───────────────────┴─────────────────────────────────────────────────────┴────────────────────────────────┘

  client/SabriMMO/Source/SabriMMO/UI/BuffBarSubsystem.cpp

  - Added magnum_break_fire → "MBF" abbreviation for buff bar icon



  Final Audit — All 8 Fixes Verified

  ┌─────┬─────────────────────────────────┬──────────┬─────────────────────────────────────────────────────────────┐
  │  #  │               Fix               │  Status  │                        Verification                         │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │     │ Inc HP Recovery (102):          │          │ +5 HP/lv flat + +0.2% MaxHP/lv added after VIT bonus,       │
  │ 1   │ hpRegenBonus wired into regen   │ CORRECT  │ before equipment bonus                                      │
  │     │ tick                            │          │                                                             │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 2   │ Inc HP Recovery (102): +10%/lv  │ CORRECT  │ Applied after both individual and group card heal bonuses,  │
  │     │ item heal bonus                 │          │ formula verified for Lv1-10                                 │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 3   │ Bash (103): Multiplicative HIT  │ CORRECT  │ Full chain verified: Bash handler → calculateSkillDamage →  │
  │     │ bonus +5%/lv                    │          │ roPhysicalDamage → skillHitBonus used at line 504           │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 4   │ Bash/Fatal Blow (107/109): Stun │ CORRECT  │ Verified: Lv10@BL99 = 49.5%, Lv6@BL99 = 9.9%, matches       │
  │     │  formula (lv-5)*baseLv/10       │          │ rAthena                                                     │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 5   │ Provoke (104): Undead/Boss      │ ALREADY  │ Lines 5666-5675 — checks before SP deduction, returns error │
  │     │ immunity                        │ DONE     │                                                             │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 6   │ Magnum Break (105): HP cost     │ CORRECT  │ 20,20,19,19,18,18,17,17,16,16 verified, cannot kill,        │
  │     │ 21-ceil(lv/2)                   │          │ checkAutoBerserk added                                      │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 7   │ Magnum Break (105): 10s fire    │ CORRECT  │ Buff applied after AoE, auto-attack hook uses               │
  │     │ endow buff                      │          │ getElementModifier, death check covers fire kills           │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 8   │ Endure (106): 7-hit counter     │ CORRECT  │ hitCount: 7 on buff, decrement inside !isMiss block,        │
  │     │                                 │          │ removeBuff + broadcast at 0                                 │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 9   │ Moving HP Recovery (108): 50%   │ CORRECT  │ Penalty applied after all bonuses (VIT, passive,            │
  │     │ regen rate                      │          │ equipment), Math.max(1, ...) ensures min 1                  │
  ├─────┼─────────────────────────────────┼──────────┼─────────────────────────────────────────────────────────────┤
  │ 10  │ Auto Berserk (109): -55% VIT    │ CORRECT  │ defMultiplier *= 0.45, gated on atkIncrease > 0, toggles    │
  │     │ DEF                             │          │ correctly via checkAutoBerserk                              │
  └─────┴─────────────────────────────────┴──────────┴─────────────────────────────────────────────────────────────┘

  Edge cases verified:
  - Magnum Break HP cost triggers checkAutoBerserk if it drops HP below 25%
  - Fire endow bonus respects element table (50% vs Water, 150% vs Earth, 0% vs Fire L2+)
  - Endure counter only decrements on non-miss enemy hits (inside if (!isMiss))
  - Fire endow only applies to auto-attacks (hook is in combat tick, not skill handlers)
  - Enemy death from fire endow bonus is caught by the existing death check at line 10028


---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
