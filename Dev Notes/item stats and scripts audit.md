# item stats and scripts audit

claude --resume 5312d21c-2d16-4fec-bd54-9b0ab4410dd4

can you do a full review of item stats/scripts to ensure that there are not other issues/gaps like this. i want you to do an indepth full analysis and review of
everything and make sure that all the script and stat functionalities of items are properly applied. make sure nothing like this perfect dodge issue exists for anything
else or any other stats or effects on an item. also do a generic audit to loook for other issues that could exist related to items and stats. record all your findings in a
 .md file that could be referenced and implemented. it should contain all the details ncessary to properly implement.


claude --resume 5312d21c-2d16-4fec-bd54-9b0ab4410dd4

Audit complete. Written to docsNew/05_Development/Item_Stat_Script_Audit.md.

  Key findings by priority:

  HIGH (gameplay bugs right now):
  1. Equipment MaxHP/SP rate bonuses ignored — 51 items (Assassin Dagger +20% HP, Vidar's Boots +9% HP/SP, etc.) have zero effect. Server only tracks card-based rates, not
  equipment-based.
  2. 8 innate-element armors stuck at neutral — Flame/Water/Wind/Earth Sprits Armor variants have bDefEle in scripts but element column is 'neutral'. Migration only fixed
  bAtkEle (weapons), not bDefEle (armor).

  MEDIUM (noticeable gameplay gaps):
  3. 83 items with ASPD rate bonuses ignored — bAspdRate not parsed into any column. Muramasa (+8%), Doom Slayer (-40%), etc. have no ASPD effect.
  4. Generator INSERT still missing element, two_handed — relies entirely on post-insert migrations that can get out of sync.

  LOW (minor/deferred):
  5. Masamune flat ASPD bonus (+2) not populated
  6. 14 items with movement speed bonuses (system not yet built)
  7. 6 items with long-range ATK% bonuses
  8. bMatkRate parsed then deleted (by design, but confusing)

  The document includes exact SQL migration templates, server code changes needed, and verification queries for each issue.





Secondary Audit — Issues Found & Fixed

  1. Case sensitivity bug — bMaxHPRate (13 items) and bMaxSPRate (6 items) were missed because the generator only handled bMaxHPrate/bMaxSPrate. Fixed by adding
  fall-through cases.
  - max_hp_rate count went from 33 → 44 items
  - max_sp_rate count went from 27 → 31 items

  2. Six new equipment bonus types implemented end-to-end:

  ┌──────────────┬───────────────┬───────┬─────────────────────────────────────────────────────────────────────────┐
  │    Bonus     │   DB Column   │ Items │                            Integration Point                            │
  ├──────────────┼───────────────┼───────┼─────────────────────────────────────────────────────────────────────────┤
  │ bHPrecovRate │ hp_regen_rate │ 27    │ HP regen tick (6s interval) — hpRegen *= (100 + rate) / 100             │
  ├──────────────┼───────────────┼───────┼─────────────────────────────────────────────────────────────────────────┤
  │ bSPrecovRate │ sp_regen_rate │ 27    │ SP regen tick (8s interval) — spRegen *= (100 + rate) / 100             │
  ├──────────────┼───────────────┼───────┼─────────────────────────────────────────────────────────────────────────┤
  │ bCritAtkRate │ crit_atk_rate │ 15    │ calculatePhysicalDamage() — added base +40% crit bonus + equipment rate │
  ├──────────────┼───────────────┼───────┼─────────────────────────────────────────────────────────────────────────┤
  │ bCastrate    │ cast_rate     │ 31    │ calculateActualCastTime() — applied after DEX reduction                 │
  ├──────────────┼───────────────┼───────┼─────────────────────────────────────────────────────────────────────────┤
  │ bUseSPrate   │ use_sp_rate   │ 18    │ skill:use handler — modifies effective SP cost                          │
  ├──────────────┼───────────────┼───────┼─────────────────────────────────────────────────────────────────────────┤
  │ bHealPower   │ heal_power    │ 13    │ calculateHealAmount() — scales heal output                              │
  └──────────────┴───────────────┴───────┴─────────────────────────────────────────────────────────────────────────┘

  3. Critical damage bonus (+40%) was missing from the damage formula entirely — now properly implemented with equipment bCritAtkRate stacking on top.

  Each bonus has full lifecycle tracking: initialized on join, added on equip, subtracted on unequip (including all 5 unequip query variants), and applied in the correct
  game formula. Restart your server to pick up all changes.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
