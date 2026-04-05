# Dancer Skills Audit

claude --resume a79a214d-65fc-493c-960a-02b875888cb8

Fix: Fortune's Kiss CRIT
  Before: skillLv + LUK/10 + DL (29 max)
  After: 10 + skillLv + LUK/10 + DL (39 max)
  Source: eathena: val1 = 10+skilllv+luk/10+DL
  ────────────────────────────────────────
  Fix: PDFM ASPD
  Before: 3*lv + DEX/10 + DL (49% max)
  After: 5 + 3*lv + DEX/10 + DL (54% max)
  Source: eathena: val1 = dex/10+3*lv+5+DL
  ────────────────────────────────────────
  Fix: PDFM Move
  Before: 2*lv + AGI/10 + ceil(DL/2) (34% max)
  After: 5 + 3*lv + AGI/10 + DL (54% max)
  Source: eathena: val2 = agi/10+3*lv+5+DL
  ────────────────────────────────────────
  Fix: Service MaxSP%
  Before: 15 + lv + INT/10 (34% max)
  After: 15 + lv + INT/10 + DL (44% max)
  Source: eathena: val1 = 15+lv+int/10+DL
  ────────────────────────────────────────
  Fix: Service SP red
  Before: 20 + 3*lv + INT/10 + ceil(DL/2) (64%)
  After: 20 + 3*lv + INT/10 + DL (69%)
  Source: eathena: val2 = 20+3*lv+int/10+DL
  ────────────────────────────────────────
  Fix: Ugly Dance drain
  Before: 5 + 5*lv (30 max)
  After: (5+5*lv) + (DL>0 ? 5+DL : 0) (45 max)
  Source: eathena: rate += 5+DL
  ────────────────────────────────────────
  Fix: bonusMaxSpRate/HpRate
  Before: Cards+equip only
  After: + buffMods.bonusMaxSpPercent/HpPercent
  Source: Fixes Service For You + Apple of Idun
  ────────────────────────────────────────
  Fix: Charming Wink
  Before: Flat 70% confusion 10s
  After: Level-based charm: (casterLv-targetLv)+40%, deaggro 10s
  Source: rAthena winkofcharm.cpp
  ────────────────────────────────────────
  Fix: Scream
  Before: Instant stun
  After: 3-second delayed execution via setTimeout
  Source: rAthena: skill_addtimerskill(tick+3000)
  ────────────────────────────────────────
  Fix: Frost Joker
  Before: Instant freeze
  After: 3-second delayed execution + party rate/4
  Source: Same as Scream
  ────────────────────────────────────────
  Fix: Enemy AI charm
  Before: N/A
  After: charmedUntil check skips AI, blocks setEnemyAggro
  Source: New mechanic

  server/src/ro_skill_data_2nd.js — 2 fixes:
  - Charming Wink: afterCastDelay: 1000 → 2000, cooldown: 3000 → 0
  - Moonlit Water Mill: (24+i*4)*1000 → (15+(i+1)*5)*1000 (20/25/30/35/40s)

  Documentation Updated

  - docsNew/05_Development/Dancer_Skills_Audit_And_Fix_Plan.md — status set to IMPLEMENTED
  - sabrimmo-skill-dancer/skill.md — all formulas, Charming Wink, Scream, PDFM, Fortune's Kiss, Service For You
  sections corrected with eathena source citations

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
