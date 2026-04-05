# Merchant Skills Audit

claude --resume 8aa6b282-85b9-4607-9abe-996c07227b48

  Merchant Skills — Complete (4 Fixes Applied)

  ┌─────┬───────────────────────┬──────────────────────┬────────────────────────────────────────────────────────────┐
  │  #  │          Fix          │         File         │                        What Changed                        │
  ├─────┼───────────────────────┼──────────────────────┼────────────────────────────────────────────────────────────┤
  │ 1   │ Cart Revolution Lex   │ index.js:9555-9558   │ Added per-target Lex Aeterna double+consume                │
  │     │ Aeterna               │                      │                                                            │
  ├─────┼───────────────────────┼──────────────────────┼────────────────────────────────────────────────────────────┤
  │     │ Cart Revolution       │                      │ skillElement: null → skillElement: 'neutral' (forced       │
  │ 2   │ Pseudo-Elemental      │ index.js:9551        │ Neutral element table, matching rAthena                    │
  │     │                       │                      │ battle_attr_fix(ELE_NEUTRAL))                              │
  ├─────┼───────────────────────┼──────────────────────┼────────────────────────────────────────────────────────────┤
  │ 3   │ Cart Revolution VFX   │ SkillVFXData.cpp:455 │ AoERadius: 300.f → 150.f                                   │
  │     │ Radius                │                      │                                                            │
  ├─────┼───────────────────────┼──────────────────────┼────────────────────────────────────────────────────────────┤
  │ 4   │ Quagmire Strips Loud  │ index.js:20113       │ Added 'loud_exclamation', 'adrenaline_rush',               │
  │     │ Exclamation           │                      │ 'improve_concentration' to Quagmire's strip list           │
  └─────┴───────────────────────┴──────────────────────┴────────────────────────────────────────────────────────────┘

  Nothing Left — All 10 Skills Verified

  - 600-602 (EWL/Discount/Overcharge): Passives correct, formulas/percentages match rAthena
  - 603 (Mammonite): Weapon element, 150-600% ATK, 100-1000z, SP 5, ASPD-based, Lex Aeterna via shared helper, Safety
  Wall block, Dubious Salesmanship hook — all correct
  - 604-607 (Pushcart/Vending/Appraisal/Change Cart): Data definitions correct, stubs correct (deferred until cart
  system Phase 8+)
  - 608 (Cart Revolution): All 3 bugs fixed — pseudo-elemental, Lex Aeterna, VFX radius
  - 609 (Loud Exclamation): +4 STR only (confirmed #ifdef RENEWAL guards +30 ATK in rAthena), self-only, 300s, Quagmire
  strip now wired

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
