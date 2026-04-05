# party and refine systems

party and refine systems


claude --resume 8325f9cf-2a2a-4fea-a4ae-77dd494c82f5



  Audit Results — All Issues Resolved

  Fixed (8 bugs)

  ┌─────┬──────────┬────────────────────────────────────┬─────────────────────────────────────────────────────┐
  │  #  │ Severity │               Issue                │                         Fix                         │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │     │          │ Auto-attack tap bonus dead code —  │                                                     │
  │ 1   │ CRITICAL │ inCombatWith.clear() before .size  │ Captured aaCombatCount before clear()               │
  │     │          │ read                               │                                                     │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │     │          │ Party HP only broadcast on         │ Added 1-second periodic party HP sync (matches      │
  │ 2   │ CRITICAL │ monster/PvP auto-attack damage —   │ rAthena party_update_interval: 1000). Also added    │
  │     │          │ missing for heal, regen, drain,    │ syncPartyMemberHP to emitRegenUpdate. Catches ALL   │
  │     │          │ items, skills, self-damage         │ HP changes.                                         │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │ 3   │ CRITICAL │ activeParties member hp/maxHp      │ Fixed by the 1-second sync — updates                │
  │     │          │ never updated after login          │ member.hp/member.maxHp from live player.health      │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │ 4   │ MEDIUM   │ Leaving player gets no             │ Added socket.emit('party:dissolved', { reason:      │
  │     │          │ confirmation event                 │ 'left' })                                           │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │ 5   │ MEDIUM   │ % prefix without party broadcasts  │ Now shows "You are not in a party." error and       │
  │     │          │ raw %text globally                 │ returns                                             │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │ 6   │ MEDIUM   │ Case-sensitive invite name         │ Changed to .toLowerCase() comparison                │
  │     │          │ matching                           │                                                     │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │ 7   │ MEDIUM   │ Existing members don't receive     │ Changed to broadcastToParty('party:update') to ALL  │
  │     │          │ full state on member join          │ members                                             │
  ├─────┼──────────┼────────────────────────────────────┼─────────────────────────────────────────────────────┤
  │ 8   │ MEDIUM   │ Teleport Lv2 missing party map     │ Added party map update + broadcast on cross-zone    │
  │     │          │ broadcast                          │ teleport                                            │
  └─────┴──────────┴────────────────────────────────────┴─────────────────────────────────────────────────────┘

  Also Fixed (minor)

  ┌────────────────────────┬─────────────────────────────────┐
  │         Issue          │               Fix               │
  ├────────────────────────┼─────────────────────────────────┤
  │ Party name not trimmed │ Added .trim() before validation │
  └────────────────────────┴─────────────────────────────────┘

  Verified Correct (no change needed)

  ┌─────────────────────────────────────┬────────────────────────────────────────────────────────────────┐
  │                Item                 │                            Finding                             │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ Level gap check (online only)       │ Matches rAthena party_check_exp_share — online-only is correct │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ recalcEvenShareEligibility          │ Correct per rAthena source                                     │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ Dead members excluded from EXP pool │ Correct — excluded from eligible list AND from N               │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ Even Share formula                  │ floor(exp * (100 + 20*(N-1)) / 100 / N) — correct              │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ Killer dead → EXP lost              │ Correct behavior (dead players don't get EXP in RO Classic)    │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ Party chat cross-map                │ Uses broadcastToParty not broadcastToZone — correct            │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────┤
  │ Self receives own party chat        │ Correct for RO Classic                                         │
  └─────────────────────────────────────┴────────────────────────────────────────────────────────────────┘

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
