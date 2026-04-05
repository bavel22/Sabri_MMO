# Thief Skills Audit

claude --resume 3b0c9acb-dc3a-4ab6-b851-cecbbf49ffc4

 All 11 Changes Implemented:

  ┌─────┬───────────────────────────────────────────────────────────────────────────────────────────────────┬────────┐
  │  #  │                                                Fix                                                │ Status │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 1   │ Throw Stone DEF bypass — Removed hard+soft DEF calc, now flat 50 MISC damage                      │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 2   │ Throw Stone blind fallback — 3% stun, if fails → 3% blind                                         │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 3   │ Envenom element on flat bonus — getElementModifier('poison', ...) applied to 15*lv                │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 4   │ Steal ACD — afterCastDelay: 1000 → 0 in ro_skill_data.js                                          │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 5   │ Steal global lock — stolenBy Set → stealLocked boolean (one steal per monster for ALL players)    │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 6   │ Steal effectValue — Already correct (10+i*6 = 10,16,...,64), no change needed                     │ N/A    │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 7   │ Hiding auto-attack cancel — autoAttackState.delete() + emit on toggle ON                          │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 8   │ Back Slide auto-attack cancel — autoAttackState.delete() + emit before teleport                   │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 9   │ DA crit suppression (extra hit) — daResult2.isCritical = false                                    │ Done   │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 10  │ DA pre-roll refactor — willDA rolled before main damage calc, main hit crit suppressed if DA      │ Done   │
  │     │ procs                                                                                             │        │
  ├─────┼───────────────────────────────────────────────────────────────────────────────────────────────────┼────────┤
  │ 11  │ DA HIT bonus — +daLv flat HIT added to DA hit's effective stats                                   │ Done   │
  └─────┴───────────────────────────────────────────────────────────────────────────────────────────────────┴────────┘

  Also fixed Snatcher (Rogue auto-steal) to use the same stealLocked global flag instead of the old stolenBy Set.

  Both files pass syntax validation. The Steal effectValue was already correct (the earlier agent report was wrong —
  10+i*6 with i=0..9 gives 10,16,...,64 which matches canonical 4+6*SkillLv).

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
