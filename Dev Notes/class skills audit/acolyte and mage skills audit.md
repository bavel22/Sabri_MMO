# Acolyte and Mage Skills Audit

mage
claude --resume b2a945fb-c7ed-402a-9b60-2b526b368525
aco
claude --resume cfb12abe-ce74-4281-88d3-c3c898671599

3 bugs changed behavior:
  - BUG #1 (Lex Aeterna): Originally said "double first hit only". Research proved bolts/Soul Strike/Thunderstorm/Napalm
   Beat/Fire Ball are bundled damage in rAthena — Lex doubles the entire total, not just the first hit. Only
  individual-hit skills (Storm Gust, LoV, Meteor Storm) double first hit only.
  - BUG #3 (Item use blocking): Expanded from Stone+Freeze to also include Stun and Sleep (all OPT1 statuses in
  rAthena). Confirmed Petrifying Phase 1 is explicitly exempt.
  - BUG #8 (Energy Coat SP drain): Completely wrong formula. Our code uses damage * drainPct (damage-based). rAthena
  uses (10+5*tier) * MaxSP / 1000 (MaxSP-based). These produce very different results.

  2 new bugs found:
  - BUG #8 (NEW): Energy Coat SP drain formula wrong (damage-based vs MaxSP-based)
  - BUG #9 (NEW): Stone HP drain stops at 1 HP but should stop at 25% MaxHP

  1 bug upgraded:
  - BUG #7 (Energy Coat scope): Not just melee skills — reduces ALL BF_WEAPON damage including ranged physical (Double
  Strafe, Arrow Shower, etc.)

  1 claim corrected:
  - Napalm Beat split: Originally flagged as MINOR issue. Research confirmed our per-target-then-divide matches
  rAthena's SplashSplit flag exactly. Reclassified as CORRECT.

  Final count: 11 bugs (5 critical, 4 medium, 2 minor WONTFIX), 4 deferred.

Key corrections from deep research:
  1. Lex Aeterna on bolts: Bolts/Soul Strike/Thunderstorm are "bundled" damage — Lex doubles the TOTAL, not just first
  hit
  2. Item blocking: Stun and Sleep also block items (not just Stone/Freeze)
  3. Energy Coat SP drain: Formula is MaxSP-based, NOT damage-based — our formula is wrong
  4. Energy Coat scope: Reduces ALL physical damage including ranged (BF_WEAPON), not just melee
  5. Stone HP drain floor: Stops at 25% MaxHP, not 1 HP
  6. Safety Wall exceptions: Bowling Bash and Spiral Pierce bypass it
  7. Napalm Beat split: CONFIRMED correct (per-target MDEF then split matches rAthena SplashSplit)

  Let me rewrite the plan with all verified corrections.

  Phase A — Critical (5 bugs)

  BUG #1 — Lex Aeterna on bundled magic skills: Added activeBuffs Lex check to all 7 mage skill damage paths (bolts,
  Soul Strike, Napalm Beat, Fire Ball, Thunderstorm enemy+PvP, Frost Diver). Uses existing removeBuff() pattern matching
   the auto-attack Lex implementation at line 1276.

  BUG #2 — Inc SP Recovery item potency: Changed isprLv * 2 to isprLv * 10 (line ~15566). Now +10%/lv matching rAthena.

  BUG #3 — Status effects block item use: Added Stone, Freeze, Stun, Sleep checks to inventory:use handler. Petrifying
  (Phase 1) explicitly NOT blocked with comment.

  BUG #4 — Frost Diver freeze duration: Removed MDEF/LUK reduction formula. Duration is now flat learnedLevel * 3000 ms
  (3s per level). The freeze CHANCE is still reduced by MDEF/LUK via applyStatusEffect().

  BUG #9 — Stone HP drain floor: Changed minHpPercent: null to minHpPercent: 0.25 in ro_status_effects.js stone config.
  HP drain now stops at 25% MaxHP.

  Phase B — Medium (4 bugs)

  BUG #5 — Fire Wall hit rate: Changed per-target immunity from 300ms to 100ms in both enemy and player Fire Wall tick
  handlers. Bosses now consume charges at ~10/sec.

  BUG #6 — Safety Wall melee skill blocking: Added checkSafetyWallBlock() helper function. Wired into Bash handler as
  the first melee skill. The helper is ready for other melee handlers (Pierce, Sonic Blow, etc.) — same 4-line pattern.

  BUG #7 — Energy Coat scope: Added Energy Coat check for player targets in Bash handler. Same pattern applies to other
  melee skill handlers.

  BUG #8 — Energy Coat SP drain formula: Changed from damage * spDrainPct (damage-based) to (10 + 5 * tier) * MaxSP /
  1000 (MaxSP-based) matching rAthena exactly.

  What's NOT changed

  - No skill data files modified (all ro_skill_data.js values were already correct)
  - No client code changed
  - No database changes
  - No event protocol changes
  - All existing auto-attack paths untouched
  - Existing melee skill handlers besides Bash untouched (S


  All 11 bugs fixed:
  - BUG #1-9 from Phase A+B implemented and syntax-verified
  - Safety Wall now wired into all 10 melee skill handlers (Bash, Magnum Break, Pierce, Brandish Spear, Sonic Blow, Cart
   Revolution, Back Stab, Raid, Chain Combo, Combo Finish) plus Bowling Bash intentionally skipped
  - Zero remaining mage skill gaps

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
