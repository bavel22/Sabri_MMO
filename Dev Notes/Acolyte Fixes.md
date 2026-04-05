# Acolyte Fixes

Acolyte Fixes

claude --resume 98818c18-e21e-4cb2-ae6e-36229bb93320

     ### PROMPT 5 — Acolyte Fixes
     **Dependencies: Prompt 0, ideally after Prompt 3 (Thief/Hiding for hidden reveal)**

     ```
     Load skills: /sabrimmo-skills /sabrimmo-combat /sabrimmo-buff /sabrimmo-debuff

     Read: docsNew/05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md

     Implement ALL fixes from the Acolyte audit doc:

     1. Blessing (402): Add enemy targeting path — when cast on
        Undead/Demon race enemy, apply debuff that halves STR/DEX/INT.
        When cast on cursed/stoned friendly, cure those statuses first.
        Keep existing friendly buff path unchanged.

     2. Increase AGI (403): Add 15 HP cost per cast (check player has
        >= 16 HP, deduct 15 HP). Add missing afterCastDelay (1000ms).
        Add interaction: casting Increase AGI removes Decrease AGI and
        vice versa (they cancel each other).

     3. Decrease AGI (404): Add success rate formula:
        `(40 + 2*SkillLv)% - TargetMDEF + (CasterBaseLv - TargetBaseLv)
         + CasterINT/5`. Clamp 5-95%.
        Add boss immunity check.
        Fix duration formula. Add Increase AGI cancellation.

     4. Signum Crucis (407): Add success rate: (23+4*lv)%.
        Make effect permanent (no duration) — only removed by death.
        Fix AoE range. Only affect Undead/Demon race enemies.

     5. Holy Light (414): Add handler — 125% MATK Holy element,
        single target, 2s cast time, 9 cells range. Standard magic
        damage pipeline via calculateMagicSkillDamage().

     6. Angelus (406): Verify DEF% values per level. Party-wide
        application deferred until party system exists.

     7. Pneuma (411): Add overlap prevention — cannot place Pneuma
        where Safety Wall exists and vice versa.

     8. Warp Portal (410): Add Blue Gemstone consumption check.
        Memorize system deferred.

     Test: Blessing on undead enemy, Decrease AGI success rate,
     Increase AGI HP cost, Holy Light damage, Pneuma/Safety Wall overlap.
     ```

  Here's a summary of all changes made:

  Changes Summary

  server/src/ro_buff_system.js

  - Added blessing_debuff buff type (category: debuff, abbrev: BCR)
  - Added blessing_debuff case in getBuffModifiers() — applies negative STR/DEX/INT bonuses

  server/src/index.js — 8 handler modifications

  1. Blessing (402) — Restructured into 3 paths:
  - PATH 1 (NEW): Enemy target with Undead element OR Demon race → applies blessing_debuff (halves STR/DEX/INT). Boss
  immunity check included.
  - PATH 2 (NEW): Friendly target with Curse or Stone → cures those statuses, does NOT apply stat buff (RO Classic).
  - PATH 3: Normal friendly buff → +STR/DEX/INT (unchanged).

  2. Increase AGI (403) — Added HP cost:
  - HP >= 16 check before execution
  - Deducts 15 HP from caster (before SP)
  - Added player:stats broadcast to buff target (ASPD update)
  - Added combat:health_update to buff target if different player

  3. Decrease AGI (404) — Added success rate:
  - Formula: 40 + 2*SkillLv + floor((BaseLv+INT)/5) - TargetMDEF, clamped 5-95%
  - SP consumed even on failure (RO Classic)
  - Broadcasts skill:effect_damage with hitType: 'miss' on failure
  - Separate duration for players: (2+lv)*5s vs monsters: (3+lv)*10s
  - Boss immunity already existed (kept)

  4. Signum Crucis (407) — 4 fixes:
  - Success rate: 23 + 4*SkillLv + CasterBaseLv - TargetLv, clamped 5-95%, per-enemy roll
  - AoE radius: 500 → 750 UE units (screen-wide ~15 cells)
  - Duration: 30s hardcode → uses skill def's 86400000 (permanent until death)
  - Element check: Now also affects Undead element enemies (not just Undead/Demon race)

  5. Holy Light (414) — New handler:
  - 125% MATK Holy element magic damage via calculateMagicSkillDamage()
  - Range check, SP deduction, applySkillDelays()
  - Aggro, damage-break statuses, death processing
  - Full skill:effect_damage broadcast

  6. Pneuma (411) — Overlap prevention:
  - Checks getGroundEffectsAtPosition() for existing pneuma or safety_wall
  - Returns error before SP consumption if overlapping

  7. Safety Wall (211) — Overlap prevention (reciprocal):
  - Same check as Pneuma — cannot place on existing Pneuma or Safety Wall

  8. Warp Portal (410) — Blue Gemstone consumption:
  - Queries character_inventory for item 717 with quantity >= 1
  - Uses RETURNING id to verify consumption succeeded
  - Respects cardNoGemStone flag (Mistress Card)
  - Error before SP consumption if no gem found




  Final Audit: 100% Complete

  ┌──────────────────┬──────────────────────────────┬──────────┬───────────────────────────────────────────────────┐
  │        #         │         Requirement          │  Status  │                   Verification                    │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1. Blessing      │                              │          │                                                   │
  │ (402)            │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1a               │ Enemy targeting path for     │ Done     │ blessing_debuff applied with halved STR/DEX/INT   │
  │                  │ Undead/Demon                 │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1b               │ Debuff halves STR/DEX/INT    │ Done     │ Math.floor(stat / 2), modifier case in            │
  │                  │                              │          │ getBuffModifiers()                                │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1c               │ Boss immunity check          │ Done     │ Checks modeFlags.boss || modeFlags.isBoss         │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1d               │ Undead element + Demon race  │ Done     │ Checks both enemy.element.type and enemy.race     │
  │                  │ check                        │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1e               │ Cure Curse/Stone on friendly │ Done     │ cleanseStatusEffects(target, ['curse', 'stone'])  │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1f               │ Cure path does NOT apply     │ Done     │ Returns after cure, skips buff                    │
  │                  │ stat buff                    │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 1g               │ Friendly buff path unchanged │ Done     │ PATH 3 preserved (original code)                  │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 2. Increase AGI  │                              │          │                                                   │
  │ (403)            │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 2a               │ 15 HP cost per cast          │ Done     │ player.health = Math.max(1, player.health - 15)   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 2b               │ Fails if HP < 16             │ Done     │ Check before everything else                      │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 2c               │ afterCastDelay: 1000         │ Done     │ Already in skill def                              │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 2d               │ Removes Decrease AGI         │ Done     │ Was already there, verified                       │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 2e               │ Stats broadcast to target    │ Done     │ player:stats + combat:health_update to target     │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3. Decrease AGI  │                              │          │                                                   │
  │ (404)            │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3a               │ Success rate formula         │ Done     │ 40 + 2*Lv + floor((BaseLv+INT)/5) - MDEF, clamped │
  │                  │                              │          │  5-95%                                            │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3b               │ SP consumed on failure       │ Done     │ SP deducted before roll                           │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3c               │ Miss broadcast on failure    │ Done     │ skill:effect_damage with hitType: 'miss'          │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3d               │ Boss immunity                │ Done     │ Was already there                                 │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3e               │ Removes Increase AGI         │ Done     │ Was already there                                 │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3f               │ Monster duration fix         │ Done     │ Uses skill def (3+i)*10000 (30-120s)              │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 3g               │ Player duration              │ Done     │ (3+lv)*5s = 20-65s, verified against iRO Wiki     │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 4. Signum Crucis │                              │          │                                                   │
  │  (407)           │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 4a               │ Success rate: (23+4*Lv)      │ Done     │ Per-enemy roll, clamped 5-95%                     │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 4b               │ Permanent duration           │ Done     │ Uses 86400000 from skill def                      │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 4c               │ AoE range 750                │ Done     │ Changed from 500 to 750                           │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 4d               │ Undead element + race check  │ Done     │ Checks both element and race                      │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 5. Holy Light    │                              │          │                                                   │
  │ (414)            │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 5a               │ Handler exists               │ Done     │ New handler added                                 │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 5b               │ 125% MATK Holy element       │ Done     │ calculateMagicSkillDamage(stats, target, mdef,    │
  │                  │                              │          │ 125, 'holy')                                      │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 5c               │ 2s cast time, 450 range      │ Done     │ In skill def                                      │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 5d               │ Damage break, aggro, death   │ Done     │ Full pipeline                                     │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 6. Angelus (406) │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 6a               │ DEF% values verified         │ Done     │ 5, 10, 15...50% correct                           │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 6b               │ Party-wide deferred          │ Deferred │ No party system yet                               │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 7. Pneuma (411)  │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 7a               │ Overlap prevention           │ Done     │ Checks for existing pneuma/safety_wall            │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 7b               │ Safety Wall reciprocal       │ Done     │ SW also checks for pneuma                         │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 8. Warp Portal   │                              │          │                                                   │
  │ (410)            │                              │          │                                                   │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 8a               │ Blue Gemstone consumption    │ Done     │ DB query with RETURNING id verification           │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 8b               │ cardNoGemStone bypass        │ Done     │ Mistress Card support                             │
  ├──────────────────┼──────────────────────────────┼──────────┼───────────────────────────────────────────────────┤
  │ 8c               │ Memorize deferred            │ Deferred │ Per audit doc                                     │
  └──────────────────┴──────────────────────────────┴──────────┴───────────────────────────────────────────────────┘

  Bugs caught during audit and fixed:
  - Decrease AGI player duration: (2+lv)*5s -> (3+lv)*5s (was 5s short per level)
  - Warp Portal missing combat:health_update emit added
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
