# 2-1 and 2-2 skills implementation Phase 0

2-1 and 2-2 skills implementation Phase 0

claude --resume 0caa9e7c-e6b8-4922-9c23-e3ea27c10277

  Phase 0 — Foundation (run FIRST, blocks everything else):
Do these one by one in order.

  - 0A: Fix all skill data bugs across 13 classes (~100+ corrections)
  - 0B: Build Ground Effect/Zone system (reused by 7+ classes)
  - 0C: Build Mount system (Knight + Crusader)


 ## PHASE 0: Foundation Systems

     These MUST be completed before any class implementation. They create shared infrastructure used by 8+ classes.

     ---

     ### Prompt 0A: Skill Data Fix Pass (All 13 Classes)

     **Run this FIRST before anything else.**

     ```
     Load skills: /sabrimmo-skills, /full-stack

     Read the research document for EACH class and fix ALL skill definition bugs in
     server/src/ro_skill_data_2nd.js. The research documents are:

     - docsNew/05_Development/Knight_Class_Research.md
     - docsNew/05_Development/Crusader_Class_Research.md
     - docsNew/05_Development/Wizard_Class_Research.md
     - docsNew/05_Development/Sage_Class_Research.md
     - docsNew/05_Development/Hunter_Class_Research.md
     - docsNew/05_Development/Bard_Class_Research.md
     - docsNew/05_Development/Dancer_Class_Research.md
     - docsNew/05_Development/Priest_Class_Research.md
     - docsNew/05_Development/Monk_Class_Research.md
     - docsNew/05_Development/Assassin_Class_Research.md
     - docsNew/05_Development/Rogue_Class_Research.md
     - docsNew/05_Development/Blacksmith_Class_Research.md
     - docsNew/05_Development/Alchemist_Class_Research.md

     Each document has a "Skill Definition Audit" or "Existing Code Audit" section that
     lists specific bugs: wrong SP costs, wrong prerequisites, wrong effectValues, wrong
     cast times, wrong cooldowns, wrong maxLevels, missing skills, etc.

     For each class:
     1. Read the audit section of the research doc
     2. Read the current skill definitions in ro_skill_data_2nd.js
     3. Fix every bug identified (SP formulas, prerequisites, effectValues, cast times, etc.)
     4. Add any missing skill definitions (e.g., Musical Strike for Bard, Slinging Arrow for Dancer)
     5. Verify the fix matches the canonical pre-renewal values in the research doc

     This is DATA ONLY — do not add any skill handlers to index.js yet.
     Expected: ~100+ individual field corrections across all 13 classes.
     ```



  ---

     ### Prompt 0B: Ground Effect / Zone System

     **Dependency:** None (foundation system)
     **Used by:** Wizard (Storm Gust, Meteor Storm, LoV, Ice Wall, Fire Pillar, Quagmire), Sage (Volcano, Deluge,
     Violent Gale, Land Protector), Priest (Sanctuary, Magnus Exorcismus, Safety Wall), Hunter (all traps),
     Bard/Dancer (all songs/dances), Alchemist (Demonstration, Bio Cannibalize)

     ```
     Load skills: /full-stack, /realtime, /sabrimmo-skills, /sabrimmo-combat

     Read these research docs for ground effect requirements:
     - docsNew/05_Development/Wizard_Class_Research.md (Section: Ground Effect Architecture)
     - docsNew/05_Development/Hunter_Class_Research.md (Section: Trap System Architecture)
     - docsNew/05_Development/Priest_Class_Research.md (Sanctuary, Magnus Exorcismus, Safety Wall)
     - docsNew/05_Development/Bard_Class_Research.md (Section: Performance System Architecture)
     - docsNew/05_Development/Sage_Class_Research.md (Volcano, Deluge, Violent Gale, Land Protector)

     Read the existing server code: server/src/index.js

     Implement a generic Ground Effect System in the server that ALL ground-based skills
     will use. This is the FOUNDATION — individual skill handlers come later.

     Requirements:
     1. Create a Map `activeGroundEffects` on the server that tracks all active ground zones
     2. Each ground effect entry should have:
        - id (unique), skillId, skillLevel, casterId, casterName
        - position (x, y, z), radius/dimensions (cells converted to UE units)
        - element, damagePerTick, healPerTick, statusEffect, statusChance
        - duration (total ms), tickInterval (ms), createdAt, expiresAt
        - type: 'damage' | 'heal' | 'buff' | 'debuff' | 'trap' | 'obstacle' | 'song'
        - maxHits (for Safety Wall), hitCount
        - affectedTargets (Set of IDs already hit this tick, for immunity)
        - onEnter/onExit/onTick callbacks (or type-based dispatch)
        - zoneId (which map zone it belongs to)
     3. Ground Effect Tick Loop (runs every 500ms or configurable):
        - Iterate all active ground effects
        - For each effect, find all entities (players + enemies) within radius
        - Apply damage/heal/buff/debuff based on type
        - Handle per-hit tracking (some skills hit once per wave, not per tick)
        - Remove expired effects
        - Handle Land Protector (nullifies other ground effects in its area)
     4. Socket.io events:
        - `ground_effect:create` — broadcast to zone (position, skillId, radius, duration, element)
        - `ground_effect:destroy` — broadcast when effect expires or is removed
        - `ground_effect:tick` — optional, for damage numbers from ground effects
     5. Placement rules:
        - Max ground effects per caster (configurable, default 3 for traps, 1 for songs)
        - Stacking rules (same skill type cannot overlap, different types can)
        - Land Protector nullification
     6. Helper functions:
        - createGroundEffect(params) — validates and creates
        - removeGroundEffect(id) — removes and broadcasts
        - getGroundEffectsAt(position, radius) — query what effects cover a point
        - getGroundEffectsInZone(zoneId) — get all effects in a zone
        - cleanupGroundEffects(zoneId) — remove all when zone resets

     Do NOT implement individual skill handlers yet. Just the reusable infrastructure.
     Create the ground effect system as a separate module file: server/src/ro_ground_effects.js
     Export functions that index.js can import and use.
     ```

     ---

     ### Prompt 0C: Mount System (Knight, Crusader)

     **Dependency:** None (foundation system)
     **Used by:** Knight (Riding, Cavalier Mastery, Brandish Spear mount requirement), Crusader (Riding, Grand Peco)

     ```
     Load skills: /full-stack, /realtime, /sabrimmo-stats, /sabrimmo-combat

     Read these research docs:
     - docsNew/05_Development/Knight_Class_Research.md (Riding, Cavalier Mastery sections)
     - docsNew/05_Development/Crusader_Class_Research.md (Mount system section)

     Read server/src/index.js for existing movement speed, ASPD, and weight systems.
     Read server/src/ro_damage_formulas.js for ASPD calculation.

     Implement the Peco Peco Mount System:

     Server-side (index.js):
     1. Add `player.isMounted` boolean field (default false)
     2. Add `player.mountType` string field (default null, 'pecopeco' when mounted)
     3. Mount toggle handler for `skill:use` when skillId matches Riding:
        - Check class is Knight or Crusader (or transcendent versions)
        - Check Riding skill is learned
        - Toggle isMounted on/off
        - Recalculate derived stats on toggle
     4. Mount effects on derived stats:
        - Movement speed: +25% when mounted
        - Weight capacity: +1000 when mounted
        - ASPD penalty: 50% reduction when mounted (before Cavalier Mastery)
        - Cavalier Mastery passive: restores ASPD by 10% per level (Lv5 = full restore)
        - Formula: mountedASPD = baseASPD * (0.5 + 0.1 * cavalierMasteryLv)
     5. Update calculateASPD() in ro_damage_formulas.js:
        - Accept isMounted and cavalierMasteryLv parameters
        - Apply mount ASPD penalty after base calculation
     6. Update buildFullStatsPayload() to include mount state
     7. Persist mount state across zone transitions (don't dismount on zone change)
     8. Brandish Spear mount check: add validation that caster isMounted for this skill

     Socket.io events:
     - `player:mount` — broadcast to zone when player mounts/dismounts
       payload: { characterId, isMounted, mountType }
     - Include isMounted in player:moved broadcasts so other clients see mount state

     Do NOT implement Knight/Crusader skill handlers yet — just the mount infrastructure.
     ```



  Phase 0 Complete — Foundation Systems

  Phase 0A: Skill Data Fix Pass (All 13 Classes)

  File: server/src/ro_skill_data_2nd.js
  - ~170+ field corrections across all 13 second-class skill definitions
  - 2 new skills added: Musical Strike (Bard, ID 1511), Slinging Arrow (Dancer, ID 1541)
  - 8 ensemble maxLevel fixes (Lullaby, Eternal Chaos, Loki's Veil, Into the Abyss → maxLevel 1)
  - afterCastDelay field added consistently to all 224 skill level objects
  - Assassin class: 0 corrections needed (already correct)
  - All 13 spot-checks pass, 0 duplicate ID conflicts

  Phase 0B: Ground Effect / Zone System

  File: server/src/ro_ground_effects.js (NEW — 400 lines)
  - 8 effect categories: damage_zone, heal_zone, buff_zone, debuff_zone, trap, obstacle, protector, contact
  - Registry CRUD: createGroundEffect(), removeGroundEffect(), getEffect(), getActiveEffects()
  - Spatial queries: getGroundEffectsAt(), getGroundEffectsInZone(), isBlockedByLandProtector()
  - Entity detection: findEntitiesInEffect() (enemies + players in radius)
  - Tick processing: processGroundEffectTick() with wave tracking, expiration, custom callbacks
  - Land Protector: 18 blocked skill types, auto-destroys overlapping effects on creation
  - Sage zone mutual exclusion: volcano/deluge/violent_gale — only 1 active per caster
  - Trap subsystem: shouldTrapTrigger(), triggerTrap(), calculateTrapDamage() (MISC damage)
  - Per-caster limits with auto-replacement of oldest effect

  Phase 0C: Mount System (Knight + Crusader)

  Files: index.js, ro_damage_formulas.js, ro_skill_data.js, ro_skill_data_2nd.js
  - player.isMounted field added to player initialization
  - ASPD penalty: 50% base, +10% per Cavalry Mastery level (Lv5 = full restore)
  - calculateASPD() accepts isMounted and cavalierMasteryLv params
  - Movement speed: +25% when mounted
  - Weight capacity: +1000 when mounted
  - mount:toggle socket event handler with class/alive/CC validation
  - player:mount broadcast to zone with mount type (peco_peco vs grand_peco for Crusader)
  - player:moved broadcasts include isMounted state
  - sharedClasses mechanism for Crusader access to Knight skills (700, 708, 709)
  - Spear Mastery mounted bonus: +5/lv (vs +4/lv dismounted)
  - cavalierMasteryLv + spearMasteryLv added to passive bonuses
  - buildFullStatsPayload includes isMounted in stats response



Phase 0 Audit — Final Status

  Phase 0A: Skill Data Fix Pass

  ┌──────────────────┬───────────────────────┬─────────────────────────────────────────────────────────────────────┐
  │      Class       │        Status         │                                Notes                                │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Knight (700-710) │ 9/9 PASS              │ All cooldowns, prereqs, castTimes fixed                             │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Crusader         │ 9/9 PASS              │ effectValues, prereqs, sharedClasses                                │
  │ (1300-1313)      │                       │                                                                     │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Wizard (800-813) │ 14/14 PASS            │ Formula reversals, arrays, effectValues                             │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Sage (1400-1421) │ 9/9 PASS              │ SP reversed, ranges, durations                                      │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Hunter (900-917) │ 25/25 PASS            │ Massive prereq + cooldown rewrite                                   │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Bard (1500-1537) │ 15/15 PASS + Musical  │                                                                     │
  │                  │ Strike added          │                                                                     │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Dancer           │ 13/13 PASS + Slinging │                                                                     │
  │ (1520-1557)      │  Arrow added          │                                                                     │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Priest           │ 14/14 PASS            │ effectValues, durations, ACDs, prereqs                              │
  │ (1000-1018)      │                       │                                                                     │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Monk (1600-1615) │ 16/16 PASS            │ castTimes, effectValues, prereqs                                    │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Assassin         │ 15/15 PASS            │ Fixed in audit — 15 corrections applied (SP scaling, prereqs,       │
  │ (1100-1111)      │                       │ ranges, effectValues, durations, cooldowns, element)                │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Rogue            │ 20/20 PASS            │ Divest castTimes, ACDs, prereqs                                     │
  │ (1700-1718)      │                       │                                                                     │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Blacksmith       │ 12/12 PASS            │ Fixed in audit — Weapon Perfection prereq added                     │
  │ (1200-1230)      │                       │                                                                     │
  ├──────────────────┼───────────────────────┼─────────────────────────────────────────────────────────────────────┤
  │ Alchemist        │ 27/27 PASS            │ Ranges, castTimes, ACDs, prereqs                                    │
  │ (1800-1815)      │                       │                                                                     │
  └──────────────────┴───────────────────────┴─────────────────────────────────────────────────────────────────────┘

  Total: 224 skills, 0 afterCastDelay gaps, 0 duplicate IDs, ~185+ corrections applied

  Phase 0B: Ground Effect System

  - ro_ground_effects.js — 26 exports, all functional
  - Land Protector blocking: verified
  - Trap damage formula: verified
  - Sage zone mutual exclusion: verified
  - Per-caster limits with auto-replacement: verified

  Phase 0C: Mount System

  - isMounted field in player init: verified
  - mount:toggle handler with class/alive/CC validation: verified
  - ASPD penalty (50% + 10%/CavMastery): verified (CavMas 0=50%, 3=80%, 5=100%)
  - +25% movement speed when mounted: verified
  - +1000 weight capacity when mounted: verified
  - player:moved includes isMounted: verified
  - buildFullStatsPayload includes isMounted: verified
  - getEffectiveStats passes mount params to ASPD calc: verified
  - Spear Mastery +5/lv (vs +4) when mounted: verified
  - sharedClasses for Crusader access to skills 700/708/709: verified
  - canLearnSkill respects sharedClasses: verified

  Zero gaps remaining.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
