# Second Class Implementation — Master Prompt Sequence

## Overview

This document contains **20 implementation prompts** to be run **in order**. Each prompt is self-contained and references the research documents created during the deep research phase.

**Total skills to implement:** ~227 across 13 classes
**Total new systems:** ~15 shared + class-specific handlers
**Estimated prompts:** 20 (3 foundation + 13 class + 4 cross-class systems)

---

## Dependency Graph

```
PHASE 0: Foundation Systems (run these FIRST — many classes depend on them)
  ├── Prompt 0A: Skill Data Fix Pass (all 13 classes)
  ├── Prompt 0B: Ground Effect / Zone System (Wizard, Sage, Priest, Hunter, Bard, Dancer, Alchemist)
  └── Prompt 0C: Mount System (Knight, Crusader)

PHASE 1: Independent Classes (no cross-class dependencies beyond Phase 0)
  ├── Prompt 1A: Assassin (dual wield already done, standalone poison/cloaking)
  ├── Prompt 1B: Priest (ground effects from 0B, buffs/barriers/heals)
  └── Prompt 1C: Knight (mount from 0C, spear/sword skills)

PHASE 2: Classes building on Phase 1 systems
  ├── Prompt 2A: Crusader (mount from 0C, shield system, Devotion)
  ├── Prompt 2B: Wizard (ground effects from 0B, multi-wave AoE)
  └── Prompt 2C: Sage (ground effects from 0B, endow system, Hindsight)

PHASE 3: Complex unique systems
  ├── Prompt 3A: Monk — Spirit Sphere System
  ├── Prompt 3B: Monk — Combo Chain + Remaining Skills
  └── Prompt 3C: Hunter — Trap System + Falcon

PHASE 4: Performance classes (shared system, implement together)
  ├── Prompt 4A: Performance System Core (shared Bard/Dancer)
  └── Prompt 4B: Bard + Dancer — All Songs/Dances

PHASE 5: Economy-adjacent classes
  ├── Prompt 5A: Blacksmith — Combat Skills
  ├── Prompt 5B: Rogue — Plagiarism + Strip + Combat
  └── Prompt 5C: Blacksmith — Forging & Refining System (deferred, economy)

PHASE 6: Highest complexity (defer until other classes stable)
  ├── Prompt 6A: Alchemist — Combat Skills + Potion Pitcher
  └── Prompt 6B: Alchemist — Homunculus System (deferred, AI companion)
```

---

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
**Used by:** Wizard (Storm Gust, Meteor Storm, LoV, Ice Wall, Fire Pillar, Quagmire), Sage (Volcano, Deluge, Violent Gale, Land Protector), Priest (Sanctuary, Magnus Exorcismus, Safety Wall), Hunter (all traps), Bard/Dancer (all songs/dances), Alchemist (Demonstration, Bio Cannibalize)

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
   - Select which bolt when casting: Fire Bolt, Cold Bolt, Lightning Bolt, Earth Spike, Soul Strike, Heaven's Drive
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

## PHASE 4: Performance Classes

---

### Prompt 4A: Performance System Core (Shared Bard/Dancer)

**Dependencies:** Phase 0A, 0B (ground effects)

```
Load skills: /sabrimmo-skills, /full-stack, /realtime, /sabrimmo-buff

Read BOTH research documents:
- docsNew/05_Development/Bard_Class_Research.md (Performance System Architecture section)
- docsNew/05_Development/Dancer_Class_Research.md (Performance System Architecture section)
Read server/src/index.js

Implement the PERFORMANCE SYSTEM — shared infrastructure for both Bard and Dancer:

PERFORMANCE STATE:
1. Add to player object:
   - player.performanceState = null (or { skillId, skillLevel, startedAt, spDrainInterval })
   - player.lastPerformanceSkillId = null (for Encore)
   - player.performanceType = null ('song' for Bard, 'dance' for Dancer)

2. Performance activation flow:
   a. Player uses song/dance skill
   b. Validate: instrument equipped (Bard) or whip equipped (Dancer)
   c. Validate: not already performing another song/dance
   d. Validate: SP sufficient for initial cost + drain
   e. Set performanceState
   f. Create ground effect (7x7 AoE centered on caster) with type: 'song'
   g. Broadcast `performance:start` event to zone

3. Performance tick (integrate into server tick loop or ground effect tick):
   a. Every SP drain interval: drain SP from performer
   b. If performer runs out of SP: cancel performance
   c. Every effect tick: apply buff/debuff to entities in 7x7 AoE
   d. Ground effect follows caster (moves with them, unlike static ground effects)
   e. Movement speed reduction while performing: base 50%, improved by Music/Dancing Lessons

4. Performance cancellation:
   a. Use Adaptation to Circumstances skill
   b. Swap to dagger (weapon change)
   c. Run out of SP
   d. Get hit by certain CC effects (Stun, Freeze, etc.)
   e. Die
   f. On cancel: remove ground effect, clear performanceState
   g. Broadcast `performance:stop` event

5. Buff linger:
   - When ally leaves song/dance AoE, buff persists for 20 seconds
   - After 20s, buff fades
   - Implementation: when entity exits AoE, set a linger timer on their buff

6. Stacking rules:
   - Only ONE song/dance can affect a player at a time
   - If player enters area of two songs, most recently entered takes priority
   - Same performer cannot stack multiple songs (only one active at a time)

7. Socket.io events:
   - `performance:start` { characterId, skillId, position, radius }
   - `performance:stop` { characterId }
   - `performance:effect` { characterId, skillId, affectedIds, buffType } (per tick)

COMMON SKILLS (both Bard and Dancer have these):
8. **Adaptation to Circumstances** — Cancel current performance
   - Simple: clear performanceState, remove ground effect
   - 1 SP cost

9. **Encore** — Replay last song/dance at half SP
   - Uses lastPerformanceSkillId
   - Casts same skill but with 50% SP cost
   - 10s cooldown from research doc

Do NOT implement individual songs/dances yet — just the performance infrastructure.
```

---

### Prompt 4B: Bard + Dancer — All Songs and Dances

**Dependencies:** Prompt 4A (performance system core)

```
Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

Read BOTH research documents:
- docsNew/05_Development/Bard_Class_Research.md
- docsNew/05_Development/Dancer_Class_Research.md
Read server/src/index.js for performance system from Prompt 4A

Implement ALL individual Bard and Dancer skills:

BARD SONGS (use performance system, apply buffs to allies in AoE):
1. **A Whistle** — FLEE + Perfect Dodge buff
   - FLEE = SkillLv + AGI/10 + MusicLessons*0.5
   - PD = floor((SkillLv+1)/2) + LUK/30 + MusicLessons*0.2

2. **Assassin Cross of Sunset** — ASPD buff
   - ASPD = MusicLessons/2 + AGI/20 + SkillLv + 5
   - Integrate with ASPD calculation

3. **A Poem of Bragi** — Cast time + after-cast delay reduction
   - VCT reduction = SkillLv*3 + DEX/10 + MusicLessons
   - ACD reduction = (Lv<10 ? Lv*3 : 50) + INT/5 + MusicLessons*2
   - Hook into cast time and afterCastDelay calculations

4. **Apple of Idun** — MaxHP buff + HP regen
   - MaxHP% = 5 + SkillLv*2 + VIT/10 + MusicLessons/2
   - HP per tick = 30 + SkillLv*5 + VIT/2 + MusicLessons*5

5. **Humming** — HIT buff (shared with Dancer)
   - HIT = SkillLv*2 + DEX/10 + MusicLessons

6. **Fortune's Kiss** — CRIT buff (shared with Dancer)
   - CRIT = SkillLv + LUK/10 + MusicLessons

7. **Service for You** — SP reduction (shared with Dancer)
   - MaxSP% = 15 + SkillLv + INT/10
   - SP cost reduction = 20 + SkillLv*3 + INT/10 + ceil(MusicLessons/2)

8. **Dissonance** — Damage to enemies in AoE
   - Damage per 3s tick: 30 + 10*SkillLv + MusicLessons*3

BARD OFFENSIVE:
9. **Musical Strike** — Instrument attack
   - 150-250% ATK per level, requires instrument
   - Usable during performance (does not cancel song)

10. **Frost Joker** — AoE freeze chance
    - (15 + 5*SkillLv)% freeze chance
    - Screen-wide (or large AoE), affects everyone INCLUDING allies
    - Cancels current performance when used

DANCER DANCES (use performance system, apply debuffs to enemies in AoE):
11. **Please Don't Forget Me** — ASPD + move speed debuff
    - ASPD reduction = (SkillLv*3) + DEX/10 + DancingLessons
    - Move speed reduction = (SkillLv*2) + AGI/10 + ceil(DancingLessons/2)
    - Debuff applied to enemies in AoE

12. **Ugly Dance** — SP drain on enemies
    - SP drain per tick from research doc formula

13. **Scream / Dazzler** — AoE stun chance on enemies
    - Stun chance per level from research doc

14. **Moonlit Water Mill** — Block self-buff songs (if in research doc)

DANCER OFFENSIVE:
15. **Slinging Arrow / Throw Arrow** — Whip attack
    - ATK% per level, requires whip
    - Usable during performance

16. **Charming Wink** — Provoke/taunt (quest skill)
    - Forces target to attack Dancer

PASSIVES:
17. **Music Lessons (Bard)** — +3 ATK per level with instrument, improves song effects
    - Add to getPassiveSkillBonuses()

18. **Dancing Lessons (Dancer)** — +3 ATK per level with whip, improves dance effects
    - Add to getPassiveSkillBonuses()

ENSEMBLE SKILLS (defer until party system exists):
19. Mark these as TODO in code comments:
    - Lullaby, Mr. Kim A Rich Man, Eternal Chaos, Drum on Battlefield
    - Ring of Nibelungen, Loki's Veil, Into the Abyss, Invulnerable Siegfried
    - Require: 1 Bard + 1 Dancer in same party, adjacent cells
    - 9x9 AoE (larger than solo), average both performers' skill levels
    - Leave stub handlers with TODO comments referencing research docs
```

---

## PHASE 5: Economy-Adjacent Classes

---

### Prompt 5A: Blacksmith — Combat Skills

**Dependencies:** Phase 0A

```
Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff

Read the full research document: docsNew/05_Development/Blacksmith_Class_Research.md
Read server/src/index.js for existing Merchant skill handlers
Read server/src/ro_skill_data_2nd.js for Blacksmith skill definitions (IDs 1200-1230)
Read docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md

Implement Blacksmith COMBAT skills only (forging/refining deferred to Prompt 5C):

BUFFS:
1. **Adrenaline Rush (1200)** — Party ASPD buff
   - Increases ASPD for caster and party members
   - ONLY works with axes and maces (validate weapon type)
   - Duration per level from research doc
   - Apply as buff, integrate with ASPD calculation
   - Party-wide effect (if party system exists, otherwise self only)

2. **Power Thrust / Over Thrust (1202)** — ATK% buff
   - Increases ATK by 5% per level (5/10/15/20/25%)
   - Party-wide effect
   - Duration per level from research doc
   - RISK: 0.1% chance per attack to break weapon (weapon break system)
   - Implement weapon break as rare event: on each attack, roll 0.1%, if break,
     unequip weapon and set weapon durability to 0 (or flag as broken)

3. **Maximize Power (1203)** — Always max ATK
   - Toggle skill: while active, always use max ATK value (no random range)
   - Drains SP per second while active (from research doc)
   - In damage calc: if maximizePowerActive, use maxATK instead of random(minATK, maxATK)

4. **Weapon Perfection (1201)** — Ignore size penalty
   - While active: weapon size penalty is removed (all size modifiers = 100%)
   - Duration per level from research doc
   - Integrate with size modifier in damage calculation

OFFENSIVE:
5. **Hammerfall (1206)** — AoE stun
   - 5x5 AoE around target
   - Stun chance per level from research doc
   - Low damage, primarily CC skill

PASSIVES (add to getPassiveSkillBonuses()):
6. **Weaponry Research (1207)** — Flat ATK + HIT bonus
   - +2 ATK per level, +1 HIT per level

7. **Hilt Binding (1208)** — ATK + status resist
   - +1 ATK per level, +1% status resistance per level

8. **Skin Tempering (1209)** — Fire + Neutral resistance
   - +4% fire resistance per level (add to element resistance in damage pipeline)
   - +1% neutral resistance per level
   - Implement element resistance check in damage-received pipeline

Shared skill access: Blacksmith needs Merchant skill tree access (Discount, Overcharge,
Pushcart, Cart Revolution, Vending, Mammonite, etc.)
```

---

### Prompt 5B: Rogue — Plagiarism + Strip + Combat

**Dependencies:** Phase 0A

```
Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

Read the full research document: docsNew/05_Development/Rogue_Class_Research.md
Read server/src/index.js for Thief skill handlers and hiding system
Read server/src/ro_skill_data_2nd.js for Rogue skill definitions (IDs 1700-1718)
Read docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md

Implement ALL Rogue skills:

PLAGIARISM SYSTEM:
1. Add to player object:
   - player.plagiarismSkill = null (or { skillId, skillLevel, skillName })

2. **Plagiarism (passive)** — Copy last offensive skill used on you
   - When player is hit by a 1st or 2nd class offensive skill:
   - Store that skill in plagiarismSkill (overwrite previous)
   - Copied skill level is capped at Plagiarism level
   - Only offensive skills can be copied (from whitelist in research doc)
   - Cannot copy: 3rd class skills, boss skills, transcendent skills
   - Persist plagiarismSkill in database (survives logout)
   - Hook into damage-received pipeline: after taking skill damage, check if copyable

3. **Using copied skill** — Rogue can use the plagiarized skill
   - When Rogue uses skill:use with the plagiarized skillId, execute it
   - Use the stored level (capped at Plagiarism level)
   - Normal SP cost and cooldown apply
   - Copied skill uses Rogue's own stats for damage calculation

4. **Preserve (quest skill)** — Prevent Plagiarism overwrite
   - While active: new skills won't overwrite current plagiarismSkill
   - Toggle buff

STRIP EQUIPMENT SYSTEM:
5. **Divest Weapon / Strip Weapon (1708)** — Remove target's weapon
6. **Divest Shield / Strip Shield (1709)** — Remove target's shield
7. **Divest Armor / Strip Armor (1710)** — Remove target's armor
8. **Divest Helm / Strip Helm (1711)** — Remove target's helm
   - All 4 follow same pattern:
   - Success rate: 50*(SkillLv+1) + 2*(casterDEX - targetDEX), per 1000
   - Duration: 60000 + 15000*SkillLv + 500*(DEX diff) ms
   - On success: temporarily remove equipment (set equipped_position to null for duration)
   - On expiry: re-equip automatically
   - If target has Chemical Protection for that slot: strip fails
   - Apply as debuff type 'stripped_weapon' etc.
   - Monster strip effects: ATK -25% (weapon), DEF -15% (shield), VIT -40% (armor), INT -40% (helm)

9. **Full Strip (if Rogue or Stalker)** — Strip all 4 at once
   - Combined check, each slot rolls independently

OFFENSIVE:
10. **Back Stab (1701)** — Behind-target attack
    - Must be behind target (check facing direction vs caster position)
    - Always hits (ignore Flee)
    - Dagger: 2 hits, Bow: 50% damage
    - Can be used from Hiding
    - 340-700% ATK per level from research doc

11. **Raid (1703)** — AoE from Hiding
    - Must be in Hiding
    - 3x3 AoE around caster
    - 140-300% ATK (pre-renewal values!)
    - Chance to Stun + Blind
    - Breaks Hiding after use

12. **Mug / Steal Coin (1706)** — Steal zeny from target
    - Success formula from research doc
    - Gain zeny, deduct from target (monster drops zeny directly)

13. **Sword Mastery (1700)** — Passive ATK with swords
    - Add to getPassiveSkillBonuses()

14. **Tunnel Drive (1704)** — Move while hidden
    - Allows movement during Hiding
    - Reduced movement speed per level
    - SP drain while moving in Hiding
    - Hook into movement: if hiding and tunnelDrive learned, allow movement

15. **Intimidate / Snatch (1705)** — Attack + random teleport both
    - Physical attack, then both caster and target teleport to random location
    - Can be used from Hiding

PASSIVES:
16. **Compulsion Discount (1712)** — NPC shop discount
    - Reduce NPC buy prices by 1% per level
    - Hook into shop buy handler

17. **Gangster's Paradise (1713)** — No aggro while sitting
    - When sitting near 2+ Rogues/Thieves: immune to monster aggro
    - Simple check in enemy AI aggro loop

18. **Snatcher (1707)** — Auto-steal on attack
    - Chance to auto-steal items on melee attack
    - Uses existing Steal formula from Thief
    - Hook into auto-attack success
```

---

### Prompt 5C: Blacksmith — Forging & Refining System

**Dependencies:** Prompt 5A (Blacksmith combat skills)

```
Load skills: /sabrimmo-skills, /sabrimmo-items, /sabrimmo-economy, /full-stack

Read the full research document: docsNew/05_Development/Blacksmith_Class_Research.md
(Focus on: Forging System Architecture, Refining System sections)
Read server/src/index.js for inventory system

Implement the FORGING and REFINING systems:

FORGING SYSTEM:
1. Forging handler for skill:use with forge skills:
   - Smith Dagger, Smith Sword, Smith Two-Handed Sword, Smith Axe, Smith Mace,
     Smith Knucklebrace, Smith Spear
   - Each creates a specific weapon type

2. Forging recipe validation:
   - Base material: Iron (for Lv1 weapons), Steel (Lv2), etc.
   - Optional: Element stones (Flame Heart, Mystic Frozen, Rough Wind, Great Nature)
   - Optional: Star Crumbs (1-3, for stat bonuses)
   - Optional: Anvil type affects success rate
   - Consume all materials on attempt (even if fail)

3. Success rate formula (from rAthena source in research doc):
   - Base rate depends on weapon type and DEX/LUK/JobLv
   - Element stones increase difficulty
   - Star Crumbs increase difficulty
   - Anvil bonus: Iron +0, Golden +10, Oridecon +20, Emperium +50

4. On success:
   - Create weapon item with forger's name embedded
   - If element stone used: weapon has that element
   - If Star Crumbs used: weapon gets bonus ATK (+5/+10/+15 per crumb)
   - Add to forger's inventory

5. On failure:
   - All materials consumed, no item created
   - Notify player of failure

REFINING SYSTEM (can be NPC-triggered):
6. Refine handler (NPC service or skill):
   - Target: equipment in inventory
   - Material: Phracon (Lv1), Emveretarcon (Lv2), Oridecon (Lv3-4 weapon),
     Elunium (armor)

7. Refine success rates (per level, from research doc):
   - Safe limit: +4 for armor, +4 for Lv1 weapons, +5 for Lv2, +6 for Lv3, +7 for Lv4
   - Below safe limit: 100% success
   - Above safe limit: decreasing chance per level (from data tables)
   - On failure: equipment is DESTROYED

8. Refine bonuses:
   - Weapons: +ATK per refine level (varies by weapon level)
   - Armor: +DEF per refine level
   - Over-refine bonus (above safe limit): extra random ATK bonus

9. Store refineLevel on items in database (character_inventory.refine_level column)

10. **Repair Weapon** — Fix broken weapons
    - Broken weapons (from Power Thrust break): restore to usable state
    - Material cost from research doc

DEFERRED (mark as TODO):
- Greed skill (AoE loot pickup — needs loot drop system)
- Cart system integration (Cart Revolution damage scaling with cart weight)
```

---

## PHASE 6: Highest Complexity (Defer)

---

### Prompt 6A: Alchemist — Combat Skills + Potion Pitcher

**Dependencies:** Phase 0A, 0B (ground effects)

```
Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-items

Read the full research document: docsNew/05_Development/Alchemist_Class_Research.md
Read server/src/index.js for Merchant skill handlers
Read server/src/ro_skill_data_2nd.js for Alchemist skill definitions (IDs 1800-1815)
Read docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md

Implement Alchemist COMBAT skills (Homunculus deferred to Prompt 6B):

OFFENSIVE:
1. **Acid Terror (1805)** — Armor break attack
   - ATK% per level: 140-300% from research doc
   - Ignores hard DEF (DEF from equipment, not VIT)
   - Force hit (always hits, ignore Flee)
   - Chance to break target's armor: 3-13% per level
   - Chance to inflict Bleeding status: 3-15% per level
   - Consumes 1 Acid Bottle from inventory
   - Implement armor break: if roll succeeds, unequip target's armor temporarily

2. **Demonstration / Acid Demonstration (1806)** — Ground fire zone
   - Use ground effect system
   - Fire element ground zone, ticks damage over duration
   - ATK% per tick: 120-200% from research doc
   - Chance to break target's weapon: 1-5% per level
   - Duration: 40-60s per level
   - Consumes 1 Bottle Grenade from inventory

3. **Bio Cannibalize / Summon Flora (1807)** — Summon plant monsters
   - Summon a plant ally at target location
   - Plant type varies by level (1=Mandragora, 2=Hydra, 3=Flora, 4=Parasite, 5=Geographer)
   - Each plant has HP from research doc, attacks enemies
   - Lv5 Geographer heals allies
   - Plants are AI-controlled (basic attack nearest enemy)
   - Implement as simplified enemy-like entities allied to caster
   - Max active: 1 per level

4. **Sphere Mine / Marine Sphere (1808)** — Summon bomb
   - Summon Marine Sphere at target location
   - Sphere has HP, self-destructs when attacked or after timer
   - Self-destruct: 11x11 AoE fire damage
   - Marine Sphere HP and damage per level from research doc

SUPPORT:
5. **Potion Pitcher (1803)** — Throw healing potion at ally
   - Target an ally (or self), heal them based on potion type
   - Potion type by level: Lv1=Red, Lv2=Orange, Lv3=Yellow, Lv4=White, Lv5=Blue
   - Heal amount = base potion heal * effectiveness% per level (110-150%)
   - Consumes the actual potion item from inventory
   - Can target from range (5-9 cells)

6. **Chemical Protection Helm (1809)**
7. **Chemical Protection Shield (1810)**
8. **Chemical Protection Armor (1811)**
9. **Chemical Protection Weapon (1812)**
   - All 4: prevent equipment from being stripped or broken for duration
   - Duration: 120s * SkillLv from research doc
   - Each consumes 1 Glistening Coat
   - Apply as buff type, check in strip/break handlers

CRAFTING:
10. **Pharmacy / Prepare Potion (1802)** — Craft potions
    - Open crafting interface (or use skill on materials)
    - Recipe list from research doc (20+ recipes)
    - Success rate formula: base + DEX/LUK/INT/JobLv contributions + Learning Potion bonus
    - On success: create potion item(s)
    - On failure: materials consumed, no output
    - Consumes: ingredients + Medicine Bowl (or Mortar as tool)

PASSIVES:
11. **Axe Mastery (1800)** — ATK bonus with axes
    - +3 ATK per level
    - Add to getPassiveSkillBonuses()

12. **Potion Research / Learning Potion (1801)** — Brewing success + potion effectiveness
    - +1% brew success rate per level
    - +5% potion heal effectiveness per level
    - Integrate with Pharmacy success and potion healing calc
```

---

### Prompt 6B: Alchemist — Homunculus System

**Dependencies:** Prompt 6A, requires significant infrastructure

```
Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-companions

Read the full research document: docsNew/05_Development/Alchemist_Class_Research.md
(Focus on: Homunculus System Architecture section)
Read RagnaCloneDocs/12_Pets_Homunculus_Companions.md

Implement the HOMUNCULUS companion system:

NOTE: This is the MOST complex system across all second classes. It involves AI
companions with their own stats, skills, leveling, and behavior. Plan for significant
development time.

CORE HOMUNCULUS SYSTEM:
1. Database: Create homunculus table
   - homunculus_id, owner_character_id, type (lif/amistr/filir/vanilmirth)
   - level, exp, hp, sp, str, agi, vit, int, dex, luk
   - intimacy (0-1000), hunger (0-100)
   - skill_1_level, skill_2_level (each type has 2 skills)
   - is_alive, is_summoned

2. Homunculus types (base stats from research doc):
   - Lif: Support healer, high VIT/INT
   - Amistr: Tank, high VIT/STR, defense skills
   - Filir: DPS, high AGI/STR, attack skills
   - Vanilmirth: Magic DPS, high INT, random AoE

3. Stat growth: each level up, roll for stat gains
   - Each stat has min/max growth range per type (from research doc tables)
   - Growth probability tables from research doc

4. Intimacy system (0-1000):
   - Starts at 100 on creation
   - Feed: +1 per feed (max 1 feed per interval)
   - Die: -20 intimacy, kill summon
   - At 910+ intimacy: can evolve (transform to stronger version)
   - At 0 intimacy: homunculus leaves permanently (DELETED)

5. Hunger system (0-100):
   - Decreases over time (1 per 60 seconds)
   - Feed to restore hunger
   - If hunger reaches 0: intimacy decreases rapidly

SKILLS:
6. **Bioethics (1813)** — Prerequisite passive, unlocks homunculus skills
7. **Call Homunculus (1814)** — Summon your homunculus
   - Summon stored homunculus into world as entity
   - Spawns near owner, follows owner
   - Requires Embryo item (consumed on first creation, not on re-summon)
8. **Rest (1815)** — Dismiss homunculus
   - Store homunculus back (HP/SP preserved)
9. **Resurrect Homunculus** — Revive dead homunculus
   - Consumes Embryo item
   - Revives with partial HP

HOMUNCULUS AI:
10. Simplified AI (not full enemy AI):
    - IDLE: follow owner, stay within 3 cells
    - ATTACK: when owner attacks, homunculus attacks same target
    - SKILL: use skills on cooldown (automatic or manual control)
    - RETURN: if too far from owner (>15 cells), teleport back

11. Homunculus-specific skills (4 types, 2 skills each):
    - Lif: Healing Hands (heal owner), Emergency Avoid (flee buff)
    - Amistr: Castling (swap position with owner), Adamantium Skin (DEF buff)
    - Filir: Moonlight (attack), Flitting (ASPD buff)
    - Vanilmirth: Caprice (random element attack), Chaotic Blessings (random heal/damage)

12. EXP sharing: Homunculus gets 10% of EXP from kills while summoned
    - Owner gets 90% (reduced from normal 100%)
    - Homunculus levels up independently

Socket.io events:
- `homunculus:summon` { characterId, homunculusData }
- `homunculus:dismiss` { characterId }
- `homunculus:died` { characterId, homunculusId }
- `homunculus:levelup` { characterId, homunculusId, newLevel, stats }
- `homunculus:skill` { characterId, homunculusId, skillId, targetId }
- `homunculus:move` { characterId, homunculusId, position }

Mark as TODO: Homunculus evolution system, Vanilmirth random skill targeting
```

---

## Prompt Execution Summary

| # | Prompt | Est. Size | Dependencies | Priority |
|---|--------|-----------|--------------|----------|
| 0A | Skill Data Fix Pass (all 13 classes) | Large | None | CRITICAL |
| 0B | Ground Effect System | Large | None | CRITICAL |
| 0C | Mount System | Medium | None | HIGH |
| 1A | Assassin Skills | Medium | 0A | HIGH |
| 1B | Priest Skills | Large | 0A, 0B | HIGH |
| 1C | Knight Skills | Large | 0A, 0C | HIGH |
| 2A | Crusader Skills | Large | 0A, 0B, 0C | HIGH |
| 2B | Wizard Skills | Large | 0A, 0B | HIGH |
| 2C | Sage Skills | Large | 0A, 0B | MEDIUM |
| 3A | Monk Foundation + Spheres | Large | 0A | MEDIUM |
| 3B | Monk Combo + Asura | Medium | 3A | MEDIUM |
| 3C | Hunter Traps + Falcon | Large | 0A, 0B | MEDIUM |
| 4A | Performance System Core | Medium | 0A, 0B | MEDIUM |
| 4B | Bard + Dancer Songs/Dances | Large | 4A | MEDIUM |
| 5A | Blacksmith Combat | Medium | 0A | MEDIUM |
| 5B | Rogue Plagiarism + Strip | Large | 0A | MEDIUM |
| 5C | Blacksmith Forging/Refining | Large | 5A | LOW |
| 6A | Alchemist Combat + Potions | Large | 0A, 0B | LOW |
| 6B | Alchemist Homunculus | XL | 6A | DEFERRED |

**Run order:** 0A → 0B → 0C → (1A, 1B, 1C in any order) → (2A, 2B, 2C) → (3A → 3B, 3C) → (4A → 4B) → (5A, 5B) → 5C → 6A → 6B

**Parallelizable groups** (can run simultaneously if you have multiple sessions):
- Group 1: 0B + 0C (independent foundation systems)
- Group 2: 1A + 1B + 1C (independent classes after Phase 0)
- Group 3: 2A + 2B (after Phase 0+1 patterns established)
- Group 4: 3A + 3C (independent complex systems)
- Group 5: 5A + 5B (independent classes)
