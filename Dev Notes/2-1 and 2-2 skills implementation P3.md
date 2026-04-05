# 2-1 and 2-2 skills implementation Phase 3

2-1 and 2-2 skills implementation Phase 3

claude --resume 22ce505b-f25d-43ca-b0b4-2ebac7319373

  Phase 3 — Complex unique systems:
Do these one by one in order.
  - 3A: Monk foundation (Spirit Spheres + simple skills)
  - 3B: Monk combo chain + Asura Strike
  - 3C: Hunter (Trap system + Falcon)




     ---

     ## PHASE 3: Complex Unique Systems

     ---

     ### Prompt 3A: Monk — Spirit Sphere System + Foundation

     **Dependencies:** Phase 0A

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff

     Read the full research document: docsNew/05_Development/Monk_Class_Research.md
     Read server/src/index.js for Acolyte skill handlers
     Read server/src/ro_skill_data_2nd.js for Monk skill definitions
     Read docsNew/05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md

     Implement Monk FOUNDATION systems and simple skills (combo system in next prompt):

     SPIRIT SPHERE SYSTEM:
     1. Add to player object:
        - player.spiritSpheres = 0 (current count)
        - player.maxSpheres = 5 (for Monk, may increase with skills)
        - player.sphereExpiry = null (timestamp when spheres expire)

     2. **Summon Spirit Sphere (skill)** — Create spheres
        - Each use generates 1 sphere (up to max)
        - SP cost per use from research doc
        - Spheres last 10 minutes (600000ms), reset timer on new summon
        - Each sphere grants +3 ATK (flat, mastery-type, bypasses DEF reduction)
        - Integrate sphere ATK bonus into getPassiveSkillBonuses() or damage calc
        - Emit `sphere:update` event: { characterId, count, maxCount }

     3. **Absorb Spirits** — Drain spheres from Monk target
        - Target another Monk: steal their spheres, gain SP per sphere
        - SP gained per sphere from research doc
        - Can also absorb from monster spirit spheres (if implemented)

     4. **Ki Translation** — Transfer spheres to party member
        - Give spheres to party member (if party system exists, otherwise defer)

     5. **Sphere expiry** — Add to server tick loop:
        - If player has spheres and current time > sphereExpiry, set spheres to 0
        - Emit sphere:update

     SIMPLE OFFENSIVE SKILLS:
     6. **Investigate / Occult Impaction** — Inverted DEF damage
        - Unique formula: ATK * (1 + 0.75*SkillLv) * (targetDEF + targetVIT) / 50
        - Higher target DEF = MORE damage (inverted!)
        - Ignores normal DEF reduction in damage pipeline
        - Consumes 1 spirit sphere
        - Implement as custom damage calculation (bypass calculatePhysicalDamage)

     7. **Finger Offensive / Throw Spirit Sphere** — Ranged sphere throw
        - Ranged attack, consumes spheres (1 per level, up to 5)
        - Each sphere = 1 hit of damage
        - Damage per hit from research doc
        - Multi-hit with delays

     8. **Ki Explosion** — AoE knockback
        - Consumes all spheres
        - AoE around caster, knockback all targets
        - Damage scales with sphere count

     PASSIVES:
     9. **Iron Fists** — ATK bonus bare-handed
        - Add to getPassiveSkillBonuses()
        - Only when no weapon equipped

     10. **Dodge / Root** — Passive flee bonus
         - FLEE bonus per level from research doc (use floor(1.5*level))
         - Add to getPassiveSkillBonuses()

     BUFFS:
     11. **Critical Explosion / Fury** — CRIT + combo enable
         - Self-buff: +CRIT per level from research doc
         - ENABLES combo skills (Triple Attack -> Chain Combo -> etc.)
         - Without Fury active, combo chain skills cannot be used
         - Consumes 5 spirit spheres to activate
         - When active: disables SP regen (from research doc)
         - Duration from research doc

     12. **Steel Body / Mental Strength** — Extreme defense buff
         - Massive DEF and MDEF boost (reduce all damage to fraction)
         - BUT: cannot attack, use skills, use items, or move
         - Duration from research doc
         - Implement as buff with movement/action lock
         - Consumes 5 spirit spheres

     13. **Spirits Recovery** — Enhanced sitting regen
         - When sitting: bonus HP and SP regen per tick
         - Values per level from research doc
         - Hook into regen system's sitting check
     ```

     ---

     ### Prompt 3B: Monk — Combo Chain System + Asura Strike

     **Dependencies:** Prompt 3A (spirit sphere system must exist)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime

     Read the full research document: docsNew/05_Development/Monk_Class_Research.md
     (Focus on: Combo System Architecture, Triple Attack, Chain Combo, Combo Finish, Asura Strike)
     Read server/src/index.js for the auto-attack tick loop

     Implement the Monk COMBO SYSTEM:

     COMBO STATE MACHINE:
     1. Add to player object:
        - player.comboState = null (or { lastSkill, timestamp, windowExpiry })
        - player.comboWindow = 0 (calculated from AGI/DEX)

     2. Combo timing window formula:
        - Window = 1.3 - (AGI * 0.004) - (DEX * 0.002) seconds
        - After using a combo-eligible skill, player has this window to use the next skill
        - If window expires, combo resets

     3. Combo chain validation:
        - Triple Attack -> Chain Combo -> Combo Finish -> Asura Strike
        - Each skill can ONLY be used if the previous skill in chain was used within the window
        - Fury/Critical Explosion must be active to enable combo skills
        - Combo skills bypass normal cast times and cooldowns when used in chain

     COMBO SKILLS:
     4. **Triple Attack (passive)** — Combo starter
        - Passive chance on auto-attack: 29% chance (at max level) to trigger Triple Attack
        - Triple Attack = 3 hits in one attack cycle
        - When Triple Attack procs, set comboState = { lastSkill: 'tripleAttack', timestamp: now }
        - Damage per hit from research doc
        - Only works bare-handed or with Monk weapons (knuckles)

     5. **Chain Combo** — Second in chain
        - Can ONLY be used within combo window after Triple Attack
        - 4 hits, damage from research doc
        - On success: update comboState = { lastSkill: 'chainCombo', timestamp: now }
        - Consumes SP per level

     6. **Combo Finish** — Third in chain
        - Can ONLY be used within combo window after Chain Combo
        - Single powerful hit + knockback
        - Damage from research doc
        - On success: update comboState = { lastSkill: 'comboFinish', timestamp: now }

     7. **Asura Strike / Guillotine Fist** — Ultimate finisher
        - Can be used standalone OR as combo finisher (after Combo Finish)
        - When used in combo: INSTANT cast (no cast time)
        - When used standalone: has cast time from research doc
        - Damage formula: (WeaponATK + StatusATK) * (8 + SP/10) + 250 + 150*SkillLv
        - Consumes ALL remaining SP
        - Consumes all spirit spheres
        - After use: 5-MINUTE SP regen lockout (player.asuraLockout timestamp)
        - This is the HIGHEST single-hit damage in the game
        - Always hits (ignores Flee)
        - Bypasses DEF

     8. **Body Relocation / Snap** — Instant teleport
        - Can be used within combo window (after any combo skill) for instant activation
        - Teleport to target cell instantly
        - Requires 1 spirit sphere
        - Standalone: has cast time; in combo: instant

     COMBO EVENT FLOW:
     9. Socket.io events for combo:
        - When Triple Attack procs: emit `skill:effect_damage` with comboReady flag
        - When combo skill used: emit `skill:effect_damage` with comboChain info
        - Client uses comboReady to show combo skill highlight on hotbar
        - `combo:ready` event: { characterId, nextValidSkills: ['chainCombo'], windowMs }
        - `combo:expired` event: { characterId }

     10. Integrate combo window check into skill:use handler:
         - When a combo skill is attempted, verify:
           a. Fury/Critical Explosion is active
           b. comboState.lastSkill matches the prerequisite
           c. current time < comboState.windowExpiry
         - If valid: execute skill instantly (no cast time, no SP delay)
         - If invalid: reject with error message

     BLADE STOP (complex, can defer):
     11. **Blade Stop** — Catch enemy attack
         - When active: if hit by melee attack, catch the weapon
         - Both attacker and caster are locked in place for duration
         - During lock: caster can use combo skills directly
         - This is complex — implement basic version or defer with TODO
     ```

     ---

     ### Prompt 3C: Hunter — Trap System + Falcon

     **Dependencies:** Phase 0A, 0B (ground effect system)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Hunter_Class_Research.md
     Read server/src/index.js for existing Archer skill handlers
     Read server/src/ro_skill_data_2nd.js for Hunter skill definitions
     Read server/src/ro_ground_effects.js (Phase 0B)
     Read docsNew/05_Development/Archer_Skills_Audit_And_Fix_Plan.md

     Implement Hunter TRAP SYSTEM, FALCON SYSTEM, and all skills:

     TRAP SYSTEM (extends ground effect from Phase 0B):
     1. Trap placement rules:
        - Max 3 traps per Hunter active at once
        - Minimum 2-cell spacing between traps
        - Each trap consumes 1 Trap item (itemId from research doc) from inventory
        - Check and consume item before placing

     2. For each trap skill, use createGroundEffect() with type: 'trap' and these specifics:

     3. **Land Mine** — Earth damage trap
        - Triggers on enemy step, deals earth MISC damage
        - Damage from research doc (MISC type: ignores DEF/MDEF)
        - Stun chance per level

     4. **Blast Mine** — Wind damage trap
        - 3x3 AoE when triggered
        - Wind MISC damage from research doc
        - Stun chance

     5. **Claymore Trap** — Fire AoE trap
        - 5x5 AoE when triggered (largest trap AoE)
        - Fire MISC damage from research doc
        - Heavy damage trap

     6. **Ankle Snare** — Immobilize trap
        - No damage, immobilizes target for duration
        - Duration per level from research doc
        - Target cannot move but CAN attack and use skills
        - Implement as Ankle Snare debuff/status

     7. **Freezing Trap** — Freeze status trap
        - Water damage + Freeze status
        - 3x3 AoE on trigger

     8. **Sandman** — Sleep status trap
        - Sleep status on trigger
        - 3x3 AoE

     9. **Shockwave Trap** — SP drain trap
        - Drains X% of target's SP
        - SP drain % per level from research doc

     10. **Spring Trap** — Remove enemy trap
         - Target an enemy Hunter's trap to remove it
         - Range per level from research doc

     11. **Remove Trap** — Remove own trap + recover item
         - Remove your own trap, get Trap item back

     12. **Detect** — Reveal hidden enemies
         - Reveals Hiding/Cloaking enemies in 3x3 area
         - Interacts with hidden state system

     MISC DAMAGE TYPE:
     13. Implement calculateMiscDamage() in ro_damage_formulas.js (or index.js):
         - MISC damage ignores DEF and MDEF
         - Affected by: race modifier cards, element modifier cards, elemental table
         - NOT affected by: ATK/MATK buffs, weapon element (uses trap's innate element)
         - Formula base from research doc per trap

     FALCON SYSTEM:
     14. Add to player object:
         - player.hasFalcon = false (set true when Falcon learned)
         - player.falconActive = true (can toggle on/off)

     15. **Blitz Beat (manual)** — Falcon attack skill
         - 5 hits of MISC damage (INT-based)
         - Damage formula: (DEX/10 + INT/2 + SkillLv*5 + SteelCrow*4) * 2
         - Split across 5 hits, each hit is 1/5 of total
         - 3x3 splash AoE
         - SP cost per level from research doc

     16. **Auto-Blitz Beat** — Passive auto-proc
         - On auto-attack hit: LUK/3 % chance to trigger Blitz Beat
         - Auto version uses same damage formula
         - BUT: number of hits varies by Job Level (JobLv/10 + 1, capped at 5)
         - Auto version damage is NOT split among targets (single target only)
         - Hook into auto-attack success in combat tick

     17. **Steel Crow** — Passive falcon damage bonus
         - +4 MISC damage per level to Blitz Beat
         - Already factored into Blitz Beat formula above
         - Add to getPassiveSkillBonuses()

     OFFENSIVE SKILLS:
     18. **Beast Strafing** — if Hunter skill (check research doc)
         - DEX-based ranged physical, bow required

     19. **Phantasmic Arrow** — Quest skill, ranged through obstacles
         - Ignores obstacles, ranged physical

     PASSIVES:
     20. **Beast Bane** — ATK bonus vs Brute/Insect
         - Add to getPassiveSkillBonuses() with raceATK pattern
     ```

     ---


Audit Fix Summary — All Issues Resolved

  Phase 3A Fixes

  - No changes needed (minor naming differences are intentional, Ki Explosion correctly doesn't consume spheres per
  research doc)

  Phase 3B Fixes (4 bugs fixed)

  1. comboState: null added to player:join initialization (line 3714)
  2. spiritSpheres/maxSpiritSpheres/sphereExpireTime added to player:join initialization
  3. Asura combo target forced — changed if (!targetId) targetId = asCombo.targetId to targetId = asCombo.targetId
  (always use combo target)
  4. Asura cast time bypass — added isComboAsuraPreCast check before cast time gate (line 5705) so combo Asura skips the
   cast timer entirely
  5. knockbackTarget calls fixed — Ki Explosion and Combo Finish were passing objects instead of (x, y, cells, zone, io)
   params

  Phase 3C Fixes (4 bugs fixed)

  1. hasFalcon initialized in player:join from learnedSkills[916]
  2. hasFalcon = true set in skill:learn handler when learning Falconry Mastery (916)
  3. hasFalcon = false cleared in skill:reset handler alongside spiritSpheres/comboState
  4. Auto-Blitz hit count formula fixed from floor((jobLv+9)/10) to floor(jobLv/10)+1
  5. Skid Trap knockbackTarget call signature fixed

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
