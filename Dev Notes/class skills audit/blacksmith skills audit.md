# Blacksmith Skills Audit

claude --resume e70b2228-98c4-48c0-a735-e7d2b228f529



 ┌─────────────────────────┬──────────┬────────────────────────────────────────────────────────────────────────┐
  │           Fix           │  Status  │                                 Notes                                  │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 1+2:                │ FIXED    │ 10 death paths + 1 respawn safety net. 2 missing paths (Fire Ball,     │
  │ clearBuffsOnDeath       │          │ Enemy AI) now patched.                                                 │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 3: ASPD max-wins    │ FIXED    │ Removal loops deleted from 3 handlers. haste2Bonus in                  │
  │                         │          │ ro_buff_system.js handles it correctly.                                │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 4: PT break         │ FIXED    │ Axe/Mace/Unbreakable weapons immune to 0.1% self-break.                │
  │ immunity                │          │                                                                        │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 5: Hammer Fall      │ FIXED    │ Expanded to dagger, 1h_sword, axe, mace per rAthena.                   │
  │ weapons                 │          │                                                                        │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 6: MP SP regen      │ FIXED    │ SP regen skipped while Maximize Power active.                          │
  │ block                   │          │                                                                        │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 7: Refine +10 rates │ FIXED    │ 20%/10% per rAthena pre-renewal.                                       │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 8: Forge penalty    │ FIXED    │ Lv2=-20%, Lv3=-30% per rAthena.                                        │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 9: AR weapon check  │ Deferred │ Low priority, needs party system.                                      │
  ├─────────────────────────┼──────────┼────────────────────────────────────────────────────────────────────────┤
  │ BUG 10: PT bare-hand    │ FIXED    │ Rejects bare-handed casting.                                           │
  └─────────────────────────┴──────────┴──────────────────────────────

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
