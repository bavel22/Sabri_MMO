# 2-1 and 2-2 skills implementation Phase 1

2-1 and 2-2 skills implementation Phase 1

claude --resume bb6e7ab6-56c5-41a0-91d0-503d6bc87814


  Phase 1 — Independent classes (any order after Phase 0):
Do these one by one in order.
  - 1A: Assassin (fastest — dual wield already done)
  - 1B: Priest (19 skills, buffs/heals/barriers)
  - 1C: Knight (spear/sword skills, Bowling Bash)


  ---

     ## PHASE 1: Independent Classes

     These classes have minimal cross-dependencies. Can be done in any order after Phase 0.

     ---

     ### Prompt 1A: Assassin — All Skills

     **Dependencies:** Phase 0A (data fixes). Dual wield already implemented.

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Assassin_Class_Research.md
     Read server/src/index.js for existing skill handler patterns (look at how Mage/Swordsman skills work)
     Read server/src/ro_skill_data_2nd.js for Assassin skill definitions (IDs 1100-1111)
     Read server/src/ro_damage_formulas.js for damage pipeline
     Read docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md for parent class context

     Implement ALL Assassin active skill handlers in server/src/index.js:

     1. **Sonic Blow (1105)** — 8-hit attack, katar required
        - Damage: ATK% per research doc per level, divided into 8 equal hits
        - Validation: must have katar equipped (check weaponType === 'katar')
        - Each hit emits skill:effect_damage with 100ms delay between hits
        - Use existing multi-hit pattern from bolt skills

     2. **Grimtooth (1106)** — Ranged AoE from Hiding
        - Validation: caster must be in Hiding state
        - 3x3 AoE around target, does NOT reveal caster
        - Damage per research doc formula
        - Katar required

     3. **Enchant Poison (1110)** — Self-buff, add poison element to weapon
        - Duration per level from research doc
        - Add as buff type, modify player's effective weapon element to 'poison'
        - Should be overwritten by Sage endow skills (and vice versa)
        - Integrate with getBuffStatModifiers() or weapon element check in damage calc

     4. **Venom Dust (1108)** — Ground poison trap
        - Place ground effect (use createGroundEffect from Phase 0B if available,
          otherwise implement inline with TODO for refactor)
        - Enemies entering the zone get Poison status
        - Duration per level, AoE size per research doc

     5. **Venom Splasher (1111)** — Timed explosion
        - Target must be below 75% HP (33% in some versions — use research doc value)
        - Mark target with a timer (duration from research doc)
        - When timer expires, target takes splash damage + surrounding enemies
        - Red Gemstone catalyst required (check and consume from inventory)

     6. **Cloaking (1107)** — Invisibility with wall requirement
        - Different from Hiding: allows movement, requires adjacent wall (in pre-renewal)
        - SP drain over time while active
        - Revealed by: taking damage, using skills (except Grimtooth), Ruwach, Sight, detection skills
        - Movement speed bonus per level while cloaked
        - Add cloaking state tracking to player object

     7. **Poison React (1109)** — Counter poison attacks
        - When active: if hit by poison element attack, counter with damage
        - Also: chance to auto-cast Envenom on attacker
        - Duration and level-based counter damage from research doc
        - Integrate with damage-received pipeline

     8. **Throw Venom Knife (quest skill)** — If defined, ranged poison attack
        - Simple single-target ranged physical + poison chance

     9. **Passives** — Verify these are already in getPassiveSkillBonuses():
        - Katar Mastery (1100): ATK bonus with katars
        - Righthand Mastery (1103): dual wield right-hand damage %
        - Lefthand Mastery (1104): dual wield left-hand damage %
        - Sonic Acceleration (1102): Sonic Blow damage bonus (if passive)
        - Advanced Katar Mastery (if Assassin Cross, defer)

     For each skill handler, follow the existing pattern in index.js:
     - Validate skill learned, SP cost, cast time, cooldown, weight threshold
     - Consume SP, apply damage, emit events
     - Use executeCastComplete() pattern for cast-time skills

     Update ro_skill_data_2nd.js class access: ensure Assassin class can access all
     skills (classId should include 'assassin' in the progression chain).
     ```

     ---

     ### Prompt 1B: Priest — All Skills

     **Dependencies:** Phase 0A (data fixes), Phase 0B (ground effects for Sanctuary/Magnus/Safety Wall)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Priest_Class_Research.md
     Read server/src/index.js for existing skill handler patterns and Acolyte skill handlers
     Read server/src/ro_skill_data_2nd.js for Priest skill definitions (IDs 1000-1018)
     Read server/src/ro_buff_system.js for buff application patterns
     Read docsNew/05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md for parent class context

     Implement ALL Priest active skill handlers in server/src/index.js:

     BUFFS (integrate with existing buff system):
     1. **Kyrie Eleison (1009)** — Damage absorb barrier
        - Creates a shield that absorbs N hits or X% of MaxHP damage (whichever first)
        - Hit count and HP% per level from research doc
        - New buff type 'kyrieEleison' with { hitsRemaining, damageRemaining }
        - Hook into physical damage pipeline: before applying damage, check for Kyrie
        - If Kyrie absorbs all damage, target takes 0 and barrier decrements
        - Visual: emit buff:applied with barrier info

     2. **Impositio Manus (1011)** — ATK buff on target
        - Flat ATK bonus per level from research doc
        - Duration 60s all levels
        - Standard buff application, integrate with getBuffStatModifiers()

     3. **Suffragium (1012)** — Cast time reduction buff
        - Reduces next spell's variable cast time by 15/30/45%
        - Consumed after ONE spell cast (one-shot buff)
        - New buff type 'suffragium' with { castReduction }
        - Hook into cast time calculation

     4. **Aspersio (1013)** — Holy weapon element buff
        - Changes target's weapon element to Holy for duration
        - Duration per level from research doc
        - Similar to Enchant Poison but Holy element
        - Overwrites other weapon element buffs

     5. **B.S. Sacramenti (1014)** — Holy armor element buff
        - Changes target's armor element to Holy
        - Requires 2 Acolyte-class characters nearby (complex prereq — can simplify for now)
        - Duration per level from research doc

     6. **Gloria (1015)** — Party LUK buff
        - +30 LUK to all party members in range
        - Duration per level from research doc

     7. **Magnificat (1016)** — SP regen buff
        - Doubles SP natural regen rate for duration
        - Affects all party members in range
        - Hook into SP regen tick

     8. **Lex Divina (1004)** — Silence debuff
        - Apply Silence status to target for duration
        - Duration per level (use manual array from research doc, NOT linear)
        - Can also be used on silenced target to REMOVE silence

     9. **Lex Aeterna (1005)** — Double next damage
        - Apply debuff: next damage instance is doubled
        - Consumed on first damage taken (one-shot debuff)
        - New debuff type 'lexAeterna' with consumption logic
        - Hook into damage pipeline: if target has lexAeterna, double damage and remove

     OFFENSIVE:
     10. **Turn Undead (1006)** — Instant kill OR holy damage on Undead
         - Success chance: (20*SkillLv + BaseLv + INT + LUK) / 1000 (from research doc formula)
         - On success: instant kill (if monster is Undead race)
         - On failure: deal Holy element magical damage
         - Only works on Undead race targets

     11. **Magnus Exorcismus (1007)** — Ground AoE Holy damage
         - Use ground effect system from Phase 0B
         - Multi-wave hits over duration
         - Only damages Undead and Demon race enemies
         - Per-enemy immunity timer between waves (from research doc)
         - Holy element, MATK-based damage

     GROUND EFFECTS:
     12. **Sanctuary (1008)** — Ground heal zone
         - Use ground effect system
         - Heals allies standing in area every tick
         - Damages Undead enemies in area (Holy damage)
         - Heal cap: 777 HP per tick at Lv7+ (from research doc)
         - Duration and tick interval per level

     13. **Safety Wall (1010)** — Ground melee block
         - Use ground effect system with type 'obstacle'
         - Blocks N melee attacks (hit count per level from research doc)
         - Does NOT block ranged or magical attacks
         - Duration OR hit count, whichever expires first
         - Blue Gemstone catalyst required

     UTILITY:
     14. **Status Recovery (1017)** — Cure all status effects
         - Remove all negative status effects from target
         - List of removable statuses from research doc

     15. **Recovery (1003)** — Remove specific statuses
         - Removes specific status effects per the research doc

     16. **Resurrection (1018)** — Revive dead player
         - Target must be dead (implement death state check)
         - HP restored: 10/30/50/80% based on level
         - Blue Gemstone required at Lv1-3, no gem at Lv4

     PASSIVES:
     17. **Mace Mastery (1001)** — ATK bonus with maces
         - Add to getPassiveSkillBonuses()

     18. **Meditatio (1000)** — MATK + SP regen + heal bonus
         - Passive: +1% MATK per level, +SP regen, +heal effectiveness
         - Add to getPassiveSkillBonuses() and integrate with heal formula
     ```

     ---

     ### Prompt 1C: Knight — All Skills

     **Dependencies:** Phase 0A (data fixes), Phase 0C (mount system)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff

     Read the full research document: docsNew/05_Development/Knight_Class_Research.md
     Read server/src/index.js for existing Swordsman skill handlers
     Read server/src/ro_skill_data_2nd.js for Knight skill definitions (IDs 700-711)
     Read server/src/ro_damage_formulas.js for physical damage pipeline
     Read docsNew/05_Development/Swordsman_Skills_Audit_And_Fix_Plan.md for parent class context

     Implement ALL Knight active skill handlers in server/src/index.js:

     OFFENSIVE SKILLS:
     1. **Pierce (701)** — Multi-hit by target size
        - Small target: 1 hit, Medium: 2 hits, Large: 3 hits
        - Damage per hit: ATK% per level from research doc
        - Spear required
        - Emit multiple skill:effect_damage events with delays

     2. **Spear Stab (702)** — Line AoE + knockback
        - Hits all enemies in a LINE between caster and target (not just splash)
        - 6-cell knockback on all targets hit
        - Spear required
        - Need to implement line AoE detection: find all enemies on the line path

     3. **Brandish Spear (703)** — Directional frontal AoE
        - Mount required (check player.isMounted)
        - Spear required
        - AoE pattern expands at Lv4 and Lv7 and Lv10 (see research doc for exact cell pattern)
        - Implement directional AoE: based on caster facing direction, hit cells in front
        - Damage varies by distance from caster (inner = full, outer = reduced)

     4. **Spear Boomerang (704)** — Ranged spear throw
        - Range increases per level: 3/5/7/9/11 cells
        - Spear required, damage per level from research doc
        - Simple single-target ranged physical skill

     5. **Bowling Bash (707)** — AoE double-hit
        - ALWAYS hits TWICE per target
        - 3x3 AoE splash around target
        - 700ms uninterruptible animation (use afterCastDelay)
        - Knockback targets hit
        - Prerequisites: Magnum Break Lv3 + Two-Hand Sword Mastery Lv5 (verify in data)

     6. **Charge Attack (710)** — Distance-based ranged physical
        - Quest skill
        - Damage scales with distance: closer = less, farther = more
        - Formula from research doc (100-500% ATK based on cell distance)
        - Calculate distance between caster and target, map to damage tier

     BUFFS:
     7. **Two-Hand Quicken (705)** — ASPD buff
        - Requires two-handed sword equipped
        - Pre-renewal: +30% ASPD only (NO crit/hit bonus — that's renewal)
        - Duration per level from research doc
        - Apply as buff type, integrate with ASPD calculation
        - Cancel if weapon is changed to non-2H sword
        - New buff type 'twoHandQuicken' with { aspdBonus: 30 }

     STANCES:
     8. **Auto Counter (706)** — Guard + counter stance
        - Activates a time-limited guard state (duration per level: 0.4-2.0s)
        - During guard: if hit by melee attack, block it and counter with forced critical
        - Counter always crits, ignores flee
        - Cannot move during stance
        - Implementation: set player.autoCounterActive with expiry timer
        - Hook into incoming melee damage: if autoCounterActive, block + counter

     PASSIVES:
     9. **Spear Mastery (700)** — already may exist, verify:
        - ATK bonus with spears: +4/lv dismounted, +5/lv mounted
        - Ensure mounted bonus is correctly applied (needs mount state check)
        - Add to getPassiveSkillBonuses()

     10. **Cavalier Mastery (709)** — ASPD restoration when mounted
         - Pure passive, integrated into mount system from Phase 0C
         - Verify it's in getPassiveSkillBonuses()

     Weapon validation helper: create isSpear(weaponType) and isTwoHandSword(weaponType)
     helper functions that skill handlers can use for weapon requirement checks.

     Shared skill access: ensure Knight class has access to Swordsman skills AND Knight
     skills in the skill progression chain.
     ```


  Phase 1A: Assassin — COMPLETE (8 active + 3 passives)

  Active skill handlers (8):
  - Sonic Blow (1101): 8-hit katar strike, (440-800)% ATK, (10+2*Lv)% stun, Sonic Acceleration passive +50% dmg/hit
  - Grimtooth (1102): Ranged from Hiding, 3x3 AoE splash, does NOT break Hiding
  - Cloaking (1103): Toggle stealth with level-based SP drain (0.5s-9s intervals), deaggros enemies
  - Enchant Poison (1109): Poison weapon endow, (2.5-7.5)% poison proc on auto-attacks
  - Poison React (1104): Reactive counter buff (Mode A: poison counter + Mode B: auto-Envenom)
  - Venom Dust (1105): Ground poison cloud (ground effect tick system), Red Gemstone catalyst
  - Venom Splasher (1110): Timed bomb (<75% HP target), 5x5 AoE detonation, Red Gemstone
  - Throw Venom Knife (1111): Ranged physical, poison chance based on Envenom level

  Systems integrated: Cloaking SP drain tick, Cloaking break on skill/attack, Venom Splasher detonation in combat tick,
  Enchant Poison auto-attack proc, Poison React counter hook in enemy damage path, Venom Dust ground effect tick

  Phase 1B: Priest — COMPLETE (17 active + 2 passives)

  Active skill handlers (17):
  - Sanctuary (1000): Ground heal zone (ticks every 1s, heals players + damages Undead)
  - Kyrie Eleison (1001): Damage barrier (MaxHP% + hit count), integrated in damage pipeline
  - Magnificat (1002): Doubles HP/SP regen, integrated in regen tick
  - Gloria (1003): +30 LUK self buff
  - Resurrection (1004): Revive dead players at 10/30/50/80% HP
  - Magnus Exorcismus (1005): Multi-wave ground AoE vs Undead/Demon (3s immunity)
  - Turn Undead (1006): Instant-kill chance on Undead, Holy MATK on fail
  - Lex Aeterna (1007): Double next damage debuff
  - Impositio Manus (1009): +5*Lv ATK buff
  - Suffragium (1010): -15/30/45% next cast time (ally only, one-shot), consumed in cast system
  - Aspersio (1011): Holy weapon endow
  - B.S. Sacramenti (1012): Holy armor endow (simplified)
  - Slow Poison (1013): Pauses poison HP drain
  - Status Recovery (1014): Cure Frozen/Stone/Stun
  - Lex Divina (1015): Silence debuff (toggle: cure if already silenced)
  - Safety Wall Priest (1017): Aliased to Mage Safety Wall
  - Redemptio (1018): Stub (deferred until party system)

  Phase 1C: Knight — COMPLETE (8 active + 3 passives)

  Active skill handlers (8):
  - Pierce (701): Multi-hit by size (S=1, M=2, L=3), spear required
  - Spear Stab (702): Line AoE with knockback 6, spear required
  - Brandish Spear (703): Frontal AoE, mount+spear required, level-scaled depth
  - Spear Boomerang (704): Ranged physical, spear required, per-level range
  - Two-Hand Quicken (705): +30% ASPD buff, 2H sword required
  - Auto Counter (706): Counter stance with forced-crit counter-attack hook
  - Bowling Bash (707): Double-hit + 3x3 splash + knockback
  - Charge Attack (710): Distance-based damage scaling (100-500%)

  Total new: 36 active handlers + 5 passive additions = 41 skill implementations across 3 classes.


 Phase 1 Audit — All Issues Fixed

  Critical (2 found, 2 fixed)

  - Grimtooth broke Hiding — Added skill.name !== 'grimtooth' exemption to hiding-break code
  - Lex Aeterna never consumed — Added consumption in 8 damage paths (auto-attack, executePhysicalSkillOnEnemy, Sonic
  Blow, Grimtooth, Pierce, Spear Stab, Brandish Spear, Bowling Bash)

  High (5 found, 5 fixed)

  - Aspersio/Enchant Poison had no effect — getAttackerInfo() now reads getCombinedModifiers(player).weaponElement first
  - Turn Undead checked race instead of element — Fixed to check enemy.element?.type
  - Turn Undead kill formula — Changed 20*Lv to 200*Lv (was 10x too low)
  - Turn Undead fail damage used MATK — Changed to piercing formula (BaseLv+INT+Lv*10)*2
  - Priest Inc SP Recovery not wired — Added skill 1016 to 10s regen tick

  Medium (9 found, 9 fixed)

  - Venom Splasher missing range check — Added
  - Bowling Bash Lex Aeterna on both hits — Now consumed after first hit only
  - Pierce missing +5%/lv accuracy bonus — Added skillHitBonus
  - Pierce single damage roll shared — Now independent rolls per hit
  - Spear Mastery missing generic 'spear' type — Added
  - Lex Aeterna boss immunity removed (non-canonical)
  - Lex Aeterna frozen/stone rejection added
  - Sanctuary target limit added (3+Lv)
  - Magnus Exorcismus +30% bonus damage added

  Minor (6 found, 3 fixed)

  - Poison React buff missing skillLevel — Fixed
  - Venom Dust cooldown: 500 → 0 — Fixed
  - Enchant Poison afterCastDelay: 0 → 1000 — Fixed
  - Cloaking toggle OFF missing combat:health_update — Cosmetic, deferred
  - Auto Counter combat:damage falls through with 0 — Cosmetic, deferred
  - Grimtooth aggro source 'melee' vs 'skill' — Cosmetic, deferred


---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
