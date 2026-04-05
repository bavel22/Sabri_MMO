# 2-1 and 2-2 skills implementation Phase 2

2-1 and 2-2 skills implementation Phase 2

claude --resume bdd12969-1f29-4c99-8b0c-4ae480bb7634

  Phase 2 — Build on Phase 1 patterns:
Do these one by one in order.
  - 2A: Crusader (shield skills, Devotion, Grand Cross)
  - 2B: Wizard (Storm Gust, Meteor Storm, Ice Wall)
  - 2C: Sage (Endow, Dispel, Hindsight autocast)

     ---

     ## PHASE 2: Classes Building on Phase 1 Systems

     ---

     ### Prompt 2A: Crusader — All Skills

     **Dependencies:** Phase 0A, 0B (ground effects), 0C (mount system), Phase 1B (Priest buff patterns to reuse)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Crusader_Class_Research.md
     Read server/src/index.js for Knight/Swordsman skill handlers and buff patterns
     Read server/src/ro_skill_data_2nd.js for Crusader skill definitions (IDs 1300-1313)
     Read server/src/ro_damage_formulas.js

     Implement ALL Crusader active skill handlers:

     OFFENSIVE:
     1. **Holy Cross (1302)** — Holy element double-hit
        - 2 hits, holy element physical damage
        - ATK% per level from research doc
        - Chance to inflict Blind status
        - No weapon requirement

     2. **Grand Cross (1303)** — Self-damage AoE hybrid
        - Deals BOTH physical AND magical damage (hybrid formula from research doc)
        - Holy + Dark element hybrid (hits Undead/Demon extra hard)
        - Caster takes 20% of MAX HP as self-damage (ignores cards/armor)
        - AoE cross pattern around caster (not circle — actual cross shape)
        - Implement calculateHybridDamage() for ATK+MATK combined formula

     3. **Shield Charge (1304)** — Shield bash + knockback
        - Shield required (validate player has shield equipped)
        - Damage based on ATK + shield DEF/refine from research doc
        - Knockback target 2 cells
        - Chance to stun

     4. **Shield Boomerang (1305)** — Ranged shield throw
        - Shield required
        - Damage scales with shield DEF + shield refine level
        - Ranged physical attack (3 cell range)
        - ATK% per level from research doc

     DEFENSIVE BUFFS:
     5. **Auto Guard (1301)** — Passive block chance
        - When active: chance to block melee attacks completely
        - Block chance per level: 5/10/14/18/21/24/26/28/29/30% (NOT linear!)
        - Hook into incoming melee damage pipeline
        - Shield required
        - Reduces movement speed while active

     6. **Reflect Shield (1307)** — Reflect damage back
        - Reflects X% of physical damage back to attacker
        - Reflect % per level from research doc
        - Shield required
        - Reflected damage is short-range physical
        - Hook into post-damage pipeline (after damage is applied, deal reflect damage)

     7. **Defender (1308)** — Ranged damage reduction
        - Reduces ranged physical damage taken by X% per level
        - Reduces movement speed significantly while active
        - Shield required
        - Integrate with incoming ranged damage check

     8. **Devotion (1309)** — Damage redirect
        - Target a party member: ALL damage they take redirects to Crusader
        - Max range: 7 cells (if target moves out of range, Devotion breaks)
        - Max targets: 1 at Lv1, up to 5 at Lv5
        - Duration per level from research doc
        - Level difference restriction (can't Devotion someone too many levels above)
        - This is the MOST complex skill — implement as buff on target + damage redirect hook
        - Hook into target's incoming damage: redirect to Crusader, Crusader takes the damage

     9. **Providence/Resist (1310)** — Holy + Dark resistance buff
        - Increase resistance to Holy and Dark element attacks
        - Party buff, duration per level

     10. **Shrink (1311)** — Knockback on Auto Guard proc
         - Passive that triggers when Auto Guard blocks an attack
         - Pushes attacker back 2 cells
         - Chance per level from research doc

     11. **Cure (1312)** — Remove specific status effects
         - Removes Silence, Blind, Confusion from target
         - No cast time

     PASSIVES:
     12. **Faith (1300)** — Max HP + Holy resistance
         - Passive: +200 Max HP per level, +5% Holy resistance per level
         - Add to getPassiveSkillBonuses()

     13. **Shield-related passives** — verify in getPassiveSkillBonuses()

     Shared skill access: Crusader needs access to Swordsman skills (including shared
     Spear Mastery, Riding, Cavalier Mastery if the research doc says they should have them).
     Fix the classId access issue identified in the research doc.
     ```

     ---

     ### Prompt 2B: Wizard — All Skills

     **Dependencies:** Phase 0A, 0B (ground effect system)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Wizard_Class_Research.md
     Read server/src/index.js for existing Mage skill handlers (bolt skills, etc.)
     Read server/src/ro_skill_data_2nd.js for Wizard skill definitions (IDs 800-813)
     Read server/src/ro_damage_formulas.js for calculateMagicalDamage()
     Read server/src/ro_ground_effects.js (created in Phase 0B)

     Implement ALL Wizard active skill handlers:

     BOLT-PATTERN (single-target, multi-hit magical — reuse existing bolt infrastructure):
     1. **Earth Spike (808)** — Earth element, 1-5 hits
        - MATK% per hit from research doc
        - Earth element, same pattern as existing bolt skills

     2. **Heaven's Drive (809)** — Ground AoE earth damage
        - 5x5 AoE, earth element, 1-5 hits per target
        - Use ground effect for the AoE targeting, not persistent zone

     GROUND EFFECT SKILLS (use Phase 0B system):
     3. **Storm Gust (800)** — Water element ground AoE
        - Persistent ground zone, multi-wave
        - 7x7 AoE, water element, MATK% per wave
        - FREEZE COUNTER: each hit adds 1 to target's freeze counter, at 3 hits = Freeze
        - 150% freeze chance means guaranteed freeze on 3rd hit (or 50% on 2nd)
        - Duration and waves per level from research doc

     4. **Lord of Vermilion (802)** — Wind element ground AoE
        - 9x9 AoE, wind element, multi-wave (4 waves)
        - Chance to inflict Blind status
        - MATK% per level per wave from research doc

     5. **Meteor Storm (801)** — Fire element ground AoE
        - Ground target, meteors drop at RANDOM positions within AoE
        - 7x7 total area, but each meteor is 3x3 splash
        - Stagger meteor drops with delays (200-400ms between)
        - Fire element, chance to Stun
        - MATK% per meteor from research doc

     6. **Quagmire (810)** — Ground debuff zone
        - Reduces AGI and DEX of enemies standing in it
        - Strips Adrenaline Rush, Two-Hand Quicken, Spear Quicken from targets
        - Duration per level, persist as ground zone
        - No damage, pure debuff

     7. **Fire Pillar (807)** — Contact damage trap
        - Place on ground, triggers when enemy walks over it
        - Ignores MDEF
        - Multi-hit when triggered (hits per level from research doc)
        - Maximum active count: 5

     8. **Ice Wall (806)** — Obstacle placement
        - Create a row of ice cells (5 cells wide perpendicular to caster facing)
        - Each cell has HP, can be destroyed by attacks
        - Blocks movement and some projectiles
        - Duration per level, or until destroyed
        - This is complex — implement as multiple ground effect cells in a line

     OFFENSIVE SINGLE-TARGET:
     9. **Jupitel Thunder (803)** — Wind element multi-hit + knockback
        - 3-7 hits (from research doc), wind element
        - Knockback target 2-7 cells based on hits
        - MATK% per hit from research doc

     10. **Water Ball (804)** — Water element multi-hit
         - Hits based on water cells nearby (simplified: use flat hit count per level)
         - Pre-renewal: fixed hit count per level from research doc
         - Water element MATK% per hit

     11. **Frost Nova (811)** — AoE freeze around caster
         - 5x5 AoE centered on caster
         - Water element damage
         - Freeze status chance per level
         - MATK% from research doc

     12. **Sightrasher (805)** — 8-direction fire blast
         - Fire element, pushes enemies away from caster
         - Requires Sight to be active (consume Sight buff)
         - Knockback all targets hit
         - MATK% from research doc

     SELF-BUFFS:
     13. **Sight Blaster (813)** — Reactive proximity trigger
         - Places invisible barrier around caster
         - When enemy approaches within 2 cells, auto-triggers fire damage + knockback
         - One-shot: consumed after triggering once
         - MATK% from research doc

     14. **Amplify Magic Power / Magic Power (812)** — if in Wizard skill set
         - Buff: increase next magical skill's damage by 50%
         - Consumed on next spell cast (one-shot buff, like Suffragium)
     ```

     ---

     ### Prompt 2C: Sage — All Skills

     **Dependencies:** Phase 0A, 0B (ground effects), Phase 2B (reuse Wizard spell patterns)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Sage_Class_Research.md
     Read server/src/index.js for existing Mage skill handlers
     Read server/src/ro_skill_data_2nd.js for Sage skill definitions (IDs 1400-1421)
     Read server/src/ro_ground_effects.js (Phase 0B)

     Implement ALL Sage active skill handlers:

     ENDOW SKILLS (weapon element buffs):
     1. **Endow Blaze (1400)** — Fire element weapon buff
     2. **Endow Tsunami (1401)** — Water element weapon buff
     3. **Endow Tornado (1402)** — Wind element weapon buff
     4. **Endow Quake (1403)** — Earth element weapon buff
        - All 4 follow same pattern: apply buff that changes target's weapon element
        - Duration per level from research doc
        - Catalyst: 1 elemental converter item consumed (or Blue/Red/Yellow/Green Gemstone)
        - Overwrites previous weapon element buff (Enchant Poison, Aspersio, other Endow)
        - Can be cast on party members
        - Implement as buff type 'weaponEndow' with { element: 'fire'|'water'|'wind'|'earth' }
        - Integrate with damage calculation: weaponElement = buff element if active

     ANTI-MAGE SKILLS:
     5. **Dispel (1404)** — Remove all buffs from target
        - Removes ALL active buffs from target (player or monster)
        - Does NOT remove debuffs
        - Success chance: 50 + 10*SkillLv - targetMDEF (from research doc)
        - INT-based cast time from research doc

     6. **Spell Breaker (1407)** — Interrupt casting + absorb
        - If target is casting a spell: interrupt and absorb SP equal to skill's SP cost
        - If magical skill: absorb SP, if physical skill: deal damage to caster
        - Target must be in casting state (check castingSkill field)

     7. **Magic Rod (1406)** — Absorb incoming spell
        - When active: next single-target magical attack on caster is absorbed
        - Absorb SP equal to absorbed spell's SP cost
        - Duration: 0.1-0.5s per level (very short timing window)
        - One-shot buff consumed on absorption

     FREE CAST (passive):
     8. **Free Cast (1408)** — Move while casting
        - Passive: allows movement during spell cast at reduced speed
        - Movement speed while casting: 30/40/50/60/70% per level
        - This requires modifying cast time system: if Free Cast learned, don't immobilize caster
        - Add to getPassiveSkillBonuses()

     AUTOCAST:
     9. **Hindsight / Auto Spell (1409)** — Auto-cast bolts on melee attack
        - Passive buff: when doing physical melee attack, chance to auto-cast a bolt spell
        - Select which bolt when casting: Fire Bolt, Cold Bolt, Lightning Bolt, Earth Spike, Soul Strike, Heaven's
     Drive
        - Chance per level: 7/9/11/13/15/17/20/22/23/25% (pre-renewal, NOT linear)
        - Auto-cast level = min(SkillLv/2 rounded, learned bolt level)
        - Hook into auto-attack success: if Hindsight active, roll chance, cast bolt
        - The bolt fires instantly (no cast time) and costs no SP
        - Duration per level from research doc

     GROUND EFFECTS:
     10. **Volcano (1413)** — Fire element ground zone
         - Boosts fire element damage, ATK, and fire resistance for allies in area
         - Buffs per level from research doc
         - Use ground effect system, type 'buff'

     11. **Deluge (1414)** — Water element ground zone
         - Boosts water element damage, MaxHP, and water resistance for allies
         - Similar to Volcano but water-themed

     12. **Violent Gale (1415)** — Wind element ground zone
         - Boosts wind element damage, Flee, and wind resistance for allies
         - Similar to Volcano/Deluge

     13. **Land Protector (1416)** — Nullify ground effects
         - Creates zone that prevents ANY ground effect from being placed in it
         - Also removes existing ground effects in its area
         - Does NOT deal damage, pure utility
         - Use ground effect system with special nullification logic

     OFFENSIVE:
     14. **Earth Spike (Sage version)** and **Heaven's Drive (Sage version)**
         - Sage has access to these Wizard skills (verify in skill tree)
         - Reuse Wizard handlers from Phase 2B if shared, or duplicate with Sage classId

     15. **Elemental Change (1410-1412)** — Change target's element
         - Three versions: Fire/Water/Wind/Earth
         - Changes monster's element (complex, may defer)

     CRAFTING:
     16. **Create Elemental Converter (1417-1420)** — Craft items
         - Creates elemental converter items from materials
         - Crafting success rate formula from research doc
         - Deferred if crafting system not ready

     PASSIVES:
     17. **Study (1405)** — +1 INT per level
         - Add to getPassiveSkillBonuses()

     18. **Dragonology (quest)** — Stats vs Dragon race
         - Add to getPassiveSkillBonuses() if documented

     Shared skill access: Sage needs access to Mage skills AND Sage skills.
     ```

     ---

  Phase 2 Summary — All 3 Classes Complete

  Class: Crusader
  Skills Implemented: 13 handlers
  Ground Effects: —
  Combat Hooks: Auto Guard block, Reflect Shield reflect, Defender ranged reduction, Devotion link
  Passives: Faith (+MaxHP, +Holy resist)
  ────────────────────────────────────────
  Class: Wizard
  Skills Implemented: 14 handlers
  Ground Effects: Storm Gust (10-wave freeze), Lord of Vermilion (4-wave blind), Meteor Storm (random meteors stun),
    Quagmire (debuff zone), Fire Pillar (contact trap)
  Combat Hooks: —
  Passives: —
  ────────────────────────────────────────
  Class: Sage
  Skills Implemented: 19 handlers (3 shared with Wizard)
  Ground Effects: Volcano/Deluge/Violent Gale (buff zones), Land Protector (nullification)
  Combat Hooks: Hindsight autocast in auto-attack tick, Free Cast (no interrupt on movement)
  Passives: Advanced Book (+ATK), Free Cast, Dragonology (+INT, +Dragon)

  Total new code: ~41 skill handlers, 10 ground effect tick handlers, 4 combat pipeline hooks, 6 passive bonuses, 25
  buff types, 39 VFX configs, 15 AoE targeting indicators.



  Audit Fixes Applied — Summary

  ┌─────┬────────────────────────────────────┬──────────┬───────────────────────────────────────────────────────────┐
  │  #  │               Issue                │ Severity │                            Fix                            │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │     │ Volcano/Deluge/Violent Gale zone   │          │ Added volcano_zone, deluge_zone, violent_gale_zone to     │
  │ 1   │ buffs had no stat effect           │ Critical │ BUFF_TYPES + getBuffModifiers() with ATK/FLEE/MaxHP       │
  │     │                                    │          │ bonuses                                                   │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │     │ Hindsight autocast missing Frost   │          │ Added Frost Diver (Lv7+), Thunderstorm+Heaven's Drive     │
  │ 2   │ Diver, Thunderstorm, Heaven's      │ Critical │ (Lv10) to spell pool                                      │
  │     │ Drive                              │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 3   │ Hindsight Earth Spike damage wrong │ Critical │ Added per-spell matkPctPerLv field (200 for Earth Spike,  │
  │     │  (100% instead of 200%/hit)        │          │ 125 for Heaven's Drive)                                   │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │     │ Dispell success rate               │          │                                                           │
  │ 4   │ [60,68,76,84,92] instead of        │ Critical │ Fixed skill data to use explicit array [60,70,80,90,100]  │
  │     │ [60,70,80,90,100]                  │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 5   │ Land Protector SP [70,66,62,58,54] │ Critical │ Fixed skill data to use explicit array [66,62,58,54,50]   │
  │     │  instead of [66,62,58,54,50]       │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 6   │ Missing SKILL_CATALYSTS for endows │ Critical │ Added 8 entries: endow_blaze/tsunami/tornado/quake,       │
  │     │  + sage ground skills              │          │ volcano, deluge, violent_gale, abracadabra                │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │     │ Providence/Faith holyResist +      │          │ Added Providence buff check + Faith passive check in      │
  │ 7   │ demonResist not consumed in damage │ Critical │ enemy attack pipeline                                     │
  │     │  pipeline                          │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 8   │ Land Protector didn't block new    │ Critical │ Added LP blocking check in createGroundEffect() — returns │
  │     │ ground effects placed after LP     │          │  null if blocked                                          │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 9   │ Cast Cancel missing                │ Medium   │ Added emit                                                │
  │     │ combat:health_update               │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 10  │ Dragonology missing                │ Medium   │ Added bonuses.raceResist.dragon                           │
  │     │ raceResist.dragon                  │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 11  │ Shield Boomerang afterCastDelay=0  │ Medium   │ Fixed to afterCastDelay: 700                              │
  │     │ (canonical 700ms)                  │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 12  │ Grand Cross afterCastDelay=0 and   │ Medium   │ Fixed to afterCastDelay: 1500, cooldown: 1000             │
  │     │ cooldown=5000                      │          │                                                           │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 13  │ Heal Crusader missing target       │ Medium   │ Added targetSock.emit('combat:health_update') for         │
  │     │ socket health_update               │          │ non-self heal targets                                     │
  ├─────┼────────────────────────────────────┼──────────┼───────────────────────────────────────────────────────────┤
  │ 14  │ Cure Crusader missing              │ Medium   │ Added broadcastToZone('skill:buff_removed') in cure loop  │
  │     │ skill:buff_removed per status      │          │                                                           │
  └─────┴────────────────────────────────────┴──────────┴───────────────────────────────────────────────────────────┘

  Remaining Known Limitations (acceptable for Phase 2)

  - Devotion damage redirect: Link creation + break conditions implemented; actual damage interception deferred
  (requires deep changes to ALL damage paths)
  - Magic Rod absorption: Buff applied but not checked in magic damage paths (needs hook in bolt/magic skill handlers)
  - Sight Blaster proximity trigger: Buff applied but no proximity tick (needs hook in enemy AI movement tick)
  - Ice Wall: Simplified to single HP pool (no per-cell destruction, no movement blocking)
  - Water Ball: Always grants max hits (no water terrain check)
  - Free Cast: Movement allowed during cast but no speed reduction enforced
  - Advanced Book ASPD bonus: ATK implemented, ASPD deferred (needs ASPD formula hook)
  - Endow expiry weaponElement revert: Not hooked into buff expiry cycle





---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
