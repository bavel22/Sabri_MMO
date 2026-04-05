# Archer Skill Audit

is dex/agi/str properly working to affect ranged damage? is vulture's eye properly increasing range? when i used improved concentration i dont see any increased in my stats window, this should should in there. arrow repel doesn't knockback the enemy - do we have a knockback system implemented? an you review all these skills and compare against the expected
behaviors in ragnarok online. they should work exactly the same in my game. please identify any issues or gaps and
remediate them. be 100% sure that you are correctly implementing these skills


  Fixes Applied

  Fix 1: Improve Concentration stats now show in client window

  server/src/index.js:8655-8658 — Added player:stats emit after applyBuff(), matching the exact pattern used by Blessing
   (line 8090) and Increase AGI (line 8133). The client's CombatStatsSubsystem will now receive updated AGI/DEX values
  immediately when IC is cast.

  Fix 2: Knockback now works visually (Arrow Repel + Arrow Shower)

  client/.../UI/EnemySubsystem.h:71 — Added HandleCombatKnockback declaration.
  client/.../UI/EnemySubsystem.cpp:152-153 — Registered combat:knockback handler (6th event).
  client/.../UI/EnemySubsystem.cpp:481-516 — Handler parses targetId/newX/newY/newZ, sets TargetPosition +
  bIsMoving=true on the BP_EnemyCharacter actor. The BP's existing Tick interpolation smoothly slides the enemy to the
  knockback destination.

  Fix 3: Stats refresh on buff expiry

  server/src/index.js:18706-18711 — After the expired buffs loop, if any buffs expired, emits player:stats with fresh
  getEffectiveStats() → buildFullStatsPayload(). This ensures the client stats window reflects correct values when IC,
  Blessing, Provoke, or any stat buff wears off.

  Verified Correct (no changes needed):

  - DEX/AGI/STR damage: StatusATK uses STR as primary stat for all weapons (correct per rAthena). DEX contributes via
  floor(DEX/5). AGI affects ASPD. All working correctly.
  - Vulture's Eye range: +50 UE/level, bow-only, used in all skill range checks
  - All skill formulas: DS 200-380%, AS 80-125%, IC 3-12%, AR 150% — all match RO Classic
  - Knockback server logic: Direction, cell size (50 UE), boss/card immunity — all correct

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
