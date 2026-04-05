# ammunition system

ammunition system

claude --resume ef974f8f-f3e3-48e0-9b36-fec6d3708e3a

  sabrimmo-ammunition skill (3 updates):
  - Arrow ATK section: noted size penalty is applied inline
  - Auto-Attack consumption: documented AFTER-damage timing, last arrow fires, Double Attack no extra consume, consumed
  even on miss
  - Mandatory rules: added rules 11-13 (consumption timing, attack blocked without arrows, Double Attack no extra)
  - Rule 5: updated with correct applyStatusEffect call signature warning
  - Rule 6: updated with explicit size penalty formula

  sabrimmo-skill-archer skill (2 updates):
  - Arrow Crafting (305): changed from "DEFERRED" to "IMPLEMENTED" with full details
  - Deferred items: moved 3 items to new "IMPLEMENTED (formerly deferred)" table, only 2 remain deferred
  (non-interruptible cast, quest gating)

  ammunition-system.md memory (1 update):
  - Added deep research audit section documenting all 3 bug fixes and 15 verified mechanics

  Verified Correct (no changes needed)

  - Arrow element override chain (Endow > Arrow > Weapon)
  - 1 arrow per cast for DS/AS/AR/MS/SA
  - Blitz Beat + Phantasmic Arrow do NOT consume arrows
  - Arrow Crafting 50% weight restriction
  - Cannot attack without arrows (attack blocked, not 0 damage)
  - Sharp Arrow is bCriticalLong (ranged only)
  - Auto-unequip on weapon removal
  - Quivers exist in pre-renewal
  - Endow expiry correctly reverts to arrow element
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
