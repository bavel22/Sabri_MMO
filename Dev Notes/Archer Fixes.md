#  Archer Fixes

 Archer Fixes

claude --resume fb639c48-1a89-4c02-9d6f-d06760f834f0
     ### PROMPT 4 — Archer Fixes
     **Dependencies: Prompt 0**

     ```
     Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-buff

     Read: docsNew/05_Development/Archer_Skills_Audit_And_Fix_Plan.md

     Implement ALL fixes from the Archer audit doc:

     1. Double Strafe (303): Restructure from 2 separate damage events
        to 1 bundled hit. Calculate total damage as
        2 * (90+10*lv)% ATK, broadcast as single skill:effect_damage
        with the combined total. Remove spurious cooldown.

     2. Arrow Shower (304): Fix AoE radius from 400 to 125 UE units
        (5x5 cells = ~125 UE). Add knockback: push all hit targets
        2 cells AWAY FROM the ground target center (not caster).
        Move cooldown to afterCastDelay (1000ms ACD).

     3. Vulture's Eye (301): Fix range bonus from +10 to +50 UE units
        per level (1 cell = 50 UE units).

     4. Improve Concentration (302): Fix stat source filtering —
        AGI/DEX percentage bonus should apply to (base + job bonus +
        equipment) only, NOT to buffs/cards. Add server-side reveal
        hidden execution (currently only broadcasts revealHidden flag
        but doesn't actually break hiding on nearby hidden players).

     5. Arrow Repel (306): Fix knockback distance from 250 to 300 UE
        units (6 cells). Use shared knockbackTarget() helper.

     Arrow Crafting (305) is deferred — needs ammunition system.

     Test: Double Strafe single-hit display, Arrow Shower knockback
     direction, Vulture's Eye range at max level.
     ```


  Archer Skills — All 5 Fixes Implemented

  1. Double Strafe (303) — Restructured from 2 separate damage events to 1 bundled hit
  - Total multiplier = effectVal * 2 (200% at Lv1, 380% at Lv10)
  - Single skill:effect_damage broadcast (hitNumber: 1, totalHits: 1)
  - Death check now uses await properly (was inside setTimeout)
  - Cooldown already 0 in skill definition (was previously fixed)

  2. Arrow Shower (304) — Fixed AoE radius + added knockback
  - AoE radius: 400 → 125 UE units (5x5 cells = 2.5 cell radius)
  - Added knockbackTarget(enemy, groundX, groundY, 2, asZone, io) — pushes 2 cells away from ground target center (not
  caster), matching RO Classic behavior
  - Knockback only on hit and if target survived
  - Cooldown moved to afterCastDelay (already correct in skill definition)

  3. Vulture's Eye (301) — Fixed range bonus
  - veLv * 10 → veLv * 50 (1 cell = 50 UE units)
  - At max level 10: +500 UE units range (was +100)

  4. Improve Concentration (302) — Fixed stat source filtering + reveal hidden
  - AGI/DEX percentage now applies to player.stats.X + equipmentBonuses.X + Owl's Eye only
  - Excludes card bonuses, buff bonuses, and enchants (no double-dipping with Blessing/Inc AGI)
  - Reveal hidden implemented: iterates connectedPlayers in same zone within 150 UE (3 cells), calls removeBuff(p,
  'hiding') + broadcasts skill:buff_removed

  5. Arrow Repel (306) — Fixed knockback distance + refactored to shared helper
  - Replaced 8-line inline knockback code with knockbackTarget(enemy, attackerPos.x, attackerPos.y, 6, arZone, io)
  - 6 cells × 100 UE/cell = 600 UE push distance (was 250 UE flat)
  - Now uses shared helper which handles boss immunity, card immunity, and proper combat:knockback broadcast






  Final Audit Checklist

  ┌─────────────────────┬─────────────────────────────┬────────┬────────────────────────────────────────────────────┐
  │          #          │         Requirement         │ Status │                    Verification                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 1. Double Strafe    │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 1a                  │ Single bundled hit (not 2   │ DONE   │ totalMultiplier = effectVal * 2, single            │
  │                     │ separate)                   │        │ calculateSkillDamage call, single broadcastToZone  │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │                     │ Total damage = 2 *          │        │ effectVal = 100-190 (per-hit), * 2 = 200-380%      │
  │ 1b                  │ (90+10*lv)% ATK             │ DONE   │ total. calculateSkillDamage divides by 100         │
  │                     │                             │        │ internally                                         │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 1c                  │ Remove cooldown             │ DONE   │ Skill def already has cooldown: 0                  │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 1d                  │ No setTimeout stagger loop  │ DONE   │ Loop removed, replaced with single synchronous     │
  │                     │                             │        │ damage calc                                        │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 1e                  │ Proper await on death check │ DONE   │ await processEnemyDeathFromSkill(...)              │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 2. Arrow Shower     │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 2a                  │ AoE radius 400→125 UE (5x5  │ DONE   │ Server: aoeRadius = 125, VFX: AoERadius = 125.f,   │
  │                     │ cells)                      │        │ Targeting circle: 125.f                            │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 2b                  │ Knockback 2 cells from      │ DONE   │ knockbackTarget(enemy, groundX, groundY, 2,        │
  │                     │ ground center               │        │ asZone, io) — source = ground pos, not caster      │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 2c                  │ Knockback only on hit +     │ DONE   │ Inside else if (!result.isMiss) after death check  │
  │                     │ alive                       │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 2d                  │ Cooldown→afterCastDelay     │ DONE   │ Skill def already has afterCastDelay: 1000,        │
  │                     │ (1000ms ACD)                │        │ cooldown: 0                                        │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 3. Vulture's Eye    │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 3a                  │ Range bonus +50 UE/level    │ DONE   │ veLv * 50                                          │
  │                     │ (not +10)                   │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 3b                  │ Max level 10 = +500 UE      │ DONE   │ 10 * 50 = 500                                      │
  │                     │ range                       │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4. Improve          │                             │        │                                                    │
  │ Concentration       │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4a                  │ % applies to base+job+equip │ DONE   │ baseAgi = player.stats.agi + equipB.agi, baseDex = │
  │                     │  only                       │        │  player.stats.dex + equipB.dex + passive.bonusDEX  │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4b                  │ Excludes card bonuses       │ DONE   │ cardB not included                                 │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4c                  │ Excludes other buff bonuses │ DONE   │ buffMods.agiBonus not included                     │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4d                  │ Includes Owl's Eye DEX      │ DONE   │ passive.bonusDEX included in baseDex               │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4e                  │ Reveal hidden executed      │ DONE   │ Iterates connectedPlayers, checks isHidden,        │
  │                     │ server-side                 │        │ removeBuff(p, 'hiding') + broadcast                │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4f                  │ Reveal radius 3 cells (150  │ DONE   │ revealRadius = 150                                 │
  │                     │ UE)                         │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 4g                  │ Player positions use        │ DONE   │ Fixed to p.lastX/p.lastY (not p.x/p.y)             │
  │                     │ correct field               │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 5. Arrow Repel      │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 5a                  │ Knockback 6 cells (was 250  │ DONE   │ knockbackTarget(enemy, ..., 6, ...) = 6 * 50 = 300 │
  │                     │ flat UE)                    │        │  UE                                                │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 5b                  │ Use shared                  │ DONE   │ Replaced 8 lines of inline code                    │
  │                     │ knockbackTarget()           │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 5c                  │ Boss/card immunity handled  │ DONE   │ knockbackTarget checks modeFlags.isBoss +          │
  │                     │                             │        │ cardNoKnockback                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 6.                  │                             │        │                                                    │
  │ knockbackTarget()   │                             │        │                                                    │
  │ fix                 │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 6a                  │ cellSize 100→50 (match      │ DONE   │ All 3 callers (Magnum Break, Arrow Shower, Arrow   │
  │                     │ codebase)                   │        │ Repel) now get correct distances                   │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 6b                  │ Same-position default       │ DONE   │ dx = -1 (was -100, which was pre-multiplied for    │
  │                     │ normalized                  │        │ old cellSize)                                      │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 6c                  │ cardSplashRange also fixed  │ DONE   │ attacker.cardSplashRange * 50 (was * 100)          │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 7. Client VFX sync  │                             │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 7a                  │ Arrow Shower VFX AoERadius  │ DONE   │ SkillVFXData.cpp                                   │
  │                     │ 400→125                     │        │                                                    │
  ├─────────────────────┼─────────────────────────────┼────────┼────────────────────────────────────────────────────┤
  │ 7b                  │ Arrow Shower targeting      │ DONE   │ SkillTreeSubsystem.cpp                             │
  │                     │ circle 400→125              │        │                                                    │
  └─────────────────────┴─────────────────────────────┴────────┴────────────────────────────────────────────────────┘

  Deferred items (per audit doc, not in scope):
  - Arrow Crafting (305): needs ammunition system
  - Arrow consumption on all bow skills
  - Arrow element inheritance
  - Arrow Repel non-interruptible cast flag


❯ update documentation and skills. make a new skill called sabrimmo-skill-archer to hold archer specific information
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
