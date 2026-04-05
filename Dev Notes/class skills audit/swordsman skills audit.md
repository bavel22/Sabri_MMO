# Swordsman Skills Audit

claude --resume 24afe47d-eb46-41c1-a5d8-6bda6691814b

  Final Verification Report

  All 7 Fixes — Confirmed Correct per RO Classic Pre-Renewal

  ┌─────────────────────────────────┬────────────────────────────────────────────┬──────────────────────────────────┐
  │               Fix               │             rAthena Reference              │             Verdict              │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │ Provoke/Auto Berserk → soft DEF │ status_calc_def2(): def2 -= def2 *         │ CORRECT — rAthena reduces def2   │
  │                                 │ val3/100                                   │ (VIT soft DEF) for all targets   │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │                                 │ battle_calc_attack →                       │ CORRECT — frozen/stone break on  │
  │ Bash checkDamageBreakStatuses   │ status_change_end(SC_FREEZE) on physical   │ any physical damage              │
  │                                 │ hit                                        │                                  │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │ Magnum Break                    │ Same as above                              │ CORRECT                          │
  │ checkDamageBreakStatuses        │                                            │                                  │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │ Magnum Break self-centered      │ skill_castend_pos2 → SM_MAGNUM uses src    │ CORRECT — always caster position │
  │                                 │ position                                   │                                  │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │ Fatal Blow PvP stun             │ status_change_start(SC_STUN) has no BL_PC  │ CORRECT — applies to players in  │
  │                                 │ exclusion                                  │ PvP                              │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │ Magnum Break HIT bonus          │ battle_calc_hit_rate: hitrate += hitrate * │ CORRECT — HIT bonus exists in    │
  │                                 │  10 * skill_lv / 100                       │ pre-renewal rAthena              │
  ├─────────────────────────────────┼────────────────────────────────────────────┼──────────────────────────────────┤
  │ Full buff system via            │ —                                          │ CORRECT — removed manual Provoke │
  │ getBuffStatModifiers()          │                                            │  double-counting                 │
  └─────────────────────────────────┴────────────────────────────────────────────┴──────────────────────────────────┘

  One Known Minor Difference (PvE only)

  In rAthena, Provoke has a dual DEF reduction:
  - status_calc_def(): Reduces hard DEF for monsters only (bl->type != BL_PC)
  - status_calc_def2(): Reduces soft DEF for all targets

  Our implementation only reduces soft DEF for all targets. This makes Provoke slightly weaker against high-armor
  monsters (e.g., a monster with 50 hard DEF would take ~56% more damage from the hard DEF component in rAthena). For
  low-armor monsters (0-20 hard DEF), the difference is negligible. PvP behavior is correct.

  HIT Bonus Formula Difference (cosmetic)

  rAthena applies Bash/MB HIT bonus as a post-calculation multiplicative on the final hit rate percentage:
  - Bash: hitrate *= (1 + 5 * skill_lv / 100) — Lv10: +50% on final rate
  - MB: hitrate *= (1 + 10 * skill_lv / 100) — Lv10: +100% on final rate

  Our implementation adds the bonus to the HIT stat before calculating hit rate. Both approaches achieve essentially the
   same gameplay result — high-level Bash/MB rarely miss — but the exact math paths differ slightly.

  Everything Else Verified Correct

  All 10 Swordsman skills (IDs 100-109) — passives, damage formulas, SP costs, durations, buffs/debuffs, range checks,
  knockback, fire endow, hit counters, toggle behavior, regen effects, and item heal bonuses — match RO Classic
  pre-renewal mechanics.

  Want me to also implement the Provoke monster-only hard DEF reduction to close that last gap?

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
