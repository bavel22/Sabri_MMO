# Monk Skill Audit

claude --resume 56d65fef-4213-4b81-bd32-004e104da5a6

  Each fix verified against authoritative sources:

  1. combat:attack handler (line 6120-6124): When a player attempts to start attacking while cloaked → cloaking
  breaks, attack proceeds. This is the initiation path.
  2. Cloaking toggle ON (line 13907-13909): When a player activates Cloaking while auto-attacking → auto-attack is
  stopped immediately (autoAttackState.delete).

  There is no way to be both cloaked and auto-attacking simultaneously — Cloaking stops attacks on activation, and
  attacking breaks Cloaking on initiation. The auto-attack tick doesn't need a redundant check.

  All 12 Assassin skills and related systems are fully implemented. Here's the complete status:

  ┌───────────────────┬──────┬─────────┬────────────────────────────────────────────────────────────────────────┐
  │       Skill       │  ID  │  Type   │                                 Status                                 │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Katar Mastery     │ 1100 │ Passive │ +3 ATK/lv, katar CRI *2                                                │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Sonic Blow        │ 1101 │ Active  │ 350-800%, stun, SA bonus, 8 hits                                       │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Grimtooth         │ 1102 │ Active  │ Hidden AoE, per-level range, doesn't break hide                        │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Cloaking          │ 1103 │ Toggle  │ SP drain, deaggro, break on attack/skill/SP=0                          │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Poison React      │ 1104 │ Active  │ Mode A (poison counter) + Mode B (full Envenom Lv5)                    │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Venom Dust        │ 1105 │ Ground  │ 2x2, 60s cloud, per-level poison duration                              │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Sonic             │ 1106 │ Passive │ +50% SB damage, +50% SB HIT                                            │
  │ Acceleration      │      │         │                                                                        │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Righthand Mastery │ 1107 │ Passive │ 50+10*Lv% right-hand recovery                                          │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Lefthand Mastery  │ 1108 │ Passive │ 30+10*Lv% left-hand recovery (max 80%)                                 │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Enchant Poison    │ 1109 │ Active  │ Weapon→Poison, 3-7.5% proc, endow mutual exclusion, weapon switch      │
  │                   │      │         │ cancel                                                                 │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Venom Splasher    │ 1110 │ Active  │ Timed bomb, weapon element, damage split, PR bonus                     │
  ├───────────────────┼──────┼─────────┼────────────────────────────────────────────────────────────────────────┤
  │ Throw Venom Knife │ 1111 │ Active  │ 100% ATK, SP 15, Envenom-gated poison, weight update                   │
  └───────────────────┴──────┴─────────┴────────────────────────────────────────────────────────────────────────┘

  Cross-systems verified: Katar CRI doubling, dual wield mastery penalties, DA excluded from katars, SKILL_CATALYSTS
   (Red Gemstone), endow mutual exclusion (correct buff names), weapon switch endow cancel (equip + unequip paths),
  Cloaking SP drain tick, EP auto-attack proc, Hiding/Cloaking break on skill use (Grimtooth exempted).

  Yes — all Assassin skills and related systems are fully implemented.

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
