# Ground Effects & AoE Zones -- Deep Research (Pre-Renewal)

> Sources: iRO Wiki Classic, rAthena pre-re source (`skill_db.yml`, `skill_unit_db`, `skill.cpp`), RateMyServer, Ragnarok Research Lab, divine-pride.net, rAthena GitHub issues, Hercules Board Archive, GameFAQs RO guides
> All data is Pre-Renewal unless explicitly noted otherwise.

---

## Overview

Ground effects in Ragnarok Online are persistent area-of-effect (AoE) zones placed on the game map's cell grid. Unlike instant AoE skills (Magnum Break, Frost Nova) that deal damage once and vanish, ground effects persist for a duration, ticking their effect on entities that stand within them.

**Core concepts:**
- **Cell-based grid**: The RO map is divided into discrete cells. Ground effects occupy a rectangular region of cells (1x1, 3x3, 5x5, 7x7, 9x9, 11x11). Placement targets a center cell, and the effect radiates outward.
- **Persistent zones**: Ground effects remain on the field for a set duration (time-based) or until their charges are consumed (hit-based), or some combination.
- **Server-authoritative**: The server tracks all active ground effects, ticks damage/heal/buff logic, and broadcasts visual events to clients. Clients render VFX but have no authority over effect behavior.
- **Categories**: Ground effects serve different purposes -- damage, healing, crowd control, buffing, debuffing, movement blocking, and utility.
- **Overlap rules**: Most ground effects cannot freely stack on the same cells. rAthena uses `UF_` flags (`NoReiteration`, `NoOverlap`, `NoFootSet`) to control placement.
- **Land Protector**: A special "meta-ground-effect" that clears and prevents most other ground effects in its area.

In rAthena, ground effects are implemented as "skill units" (`skill_unit_db`), each with a Unit ID (`UNT_*`), visual effect, tick interval, target flags, and behavior flags.

---

## Ground Effect Types

### Damage Zones

Persistent AoE damage that ticks on entities standing within the zone.

| Skill | Class | Element | AoE | Duration | Tick Interval | Notes |
|-------|-------|---------|-----|----------|---------------|-------|
| Storm Gust | Wizard | Water | 7x7 (effective 9x9) | 4.5s | 450ms (~10 ticks) | Freeze on every 3rd hit |
| Meteor Storm | Wizard | Fire | 7x7 (potential 13x13 splash) | Varies | 300ms between meteors | Random meteor placement within AoE |
| Lord of Vermilion | Wizard | Wind | 9x9 (effective 11x11) | 4s | 1000ms (4 waves) | Blind chance |
| Fire Pillar | Wizard | Fire | 1x1 (splash scales by level) | Until triggered | Instant on trigger | Trap-type: dormant until stepped on |
| Magnus Exorcismus | Priest | Holy | 7x7 | 5-14s (level-based) | 3000ms immunity window | Only hits Demon race and Undead/Shadow element |
| Demonstration | Alchemist | Fire | 5x5 | 40-60s | 500ms | Fire ground DoT |

### Healing Zones

Persistent AoE healing that ticks on players/allies standing within the zone.

| Skill | Class | Element | AoE | Duration | Tick Interval | Notes |
|-------|-------|---------|-----|----------|---------------|-------|
| Sanctuary | Priest | Holy | 5x5 | 4-31s (level-based) | 1000ms (1s per wave) | Damages Undead element (half heal value), knockback 2 cells vs Undead/Demon |

### Trap Zones

Single-trigger ground effects placed by Hunter/Sniper. Activated when a target steps on the cell.

| Skill | ID | Element | AoE | Duration (idle) | Effect |
|-------|----|---------|-----|-----------------|--------|
| Skid Trap | 115 | Neutral | 1x1 trigger | 300s | Slides target 6-10 cells in caster's facing direction |
| Land Mine | 116 | Earth | 1x1 trigger | 300s | Earth damage: `(DEX+75)*(1+INT/100)*Lv +/- 10%` + Stun chance |
| Ankle Snare | 117 | Neutral | 1x1 trigger | 300s | Immobilize. Duration: `Base - TargetAGI/10` seconds. Boss = 1/5 duration |
| Shockwave Trap | 118 | Neutral | 1x1 trigger | 300s | Drains `(5+15*Lv)%` SP |
| Sandman | 119 | Neutral | 5x5 splash | 300s | Sleep `(40+10*Lv)%` chance |
| Flasher | 120 | Neutral | 5x5 splash | 300s | Blind status |
| Freezing Trap | 121 | Water | 3x3 | 300s | Water damage `(25+25*Lv)%` ATK + Freeze chance |
| Blast Mine | 122 | Wind | 3x3 | 300s | Wind damage: `(50+DEX/2)*(1+INT/100)*Lv +/- 10%` |
| Claymore Trap | 123 | Fire | 5x5 | 300s | Fire damage: `(75+DEX/2)*(1+INT/100)*Lv +/- 10%`. Costs 2 Trap items |
| Talkie Box | 125 | Neutral | 1x1 trigger | 600s | Displays preset message when triggered |

**General trap properties:**
- All traps consume 1 Trap item (purchased from NPC), except Claymore Trap (2) and Shockwave Trap (2)
- Idle traps expire after their duration and return the Trap item to the caster
- Traps occupy exactly 1 cell
- Traps cannot be placed within 2 cells of another player, monster, or trap
- Damage traps deal MISC-type damage (ignores FLEE, ignores DEF/MDEF in most implementations)
- In PvP environments, traps can be triggered by allies and the caster
- Traps can be removed by the caster (Remove Trap skill) or by a Falcon (Spring Trap skill)
- Land Protector blocks trap placement and destroys existing traps in its area

### Buff Zones

Persistent AoE buffs applied to entities standing within the zone.

| Skill | Class | Element | AoE | Duration | Tick Interval | Effect |
|-------|-------|---------|-----|----------|---------------|--------|
| Volcano | Sage | Fire | 7x7 | Lv * 60s | 1000ms (buff refresh) | Fire element ATK/damage boost |
| Deluge | Sage | Water | 7x7 | Lv * 60s | 1000ms | Water element MaxHP boost |
| Violent Gale | Sage | Wind | 7x7 | Lv * 60s | 1000ms | Wind element FLEE boost |
| Bard songs | Bard | Neutral | 7x7 | While performing | Continuous | Follows caster. Various stat buffs |
| Dancer dances | Dancer | Neutral | 7x7 | While performing | Continuous | Follows caster. Various stat buffs |
| Ensemble skills | Bard+Dancer | Neutral | 9x9 | While performing | Continuous | Stationary at midpoint between performers |

### Blocking Zones

Ground effects that prevent specific attack types or movement.

| Skill | Class | AoE | Duration | Mechanic |
|-------|-------|-----|----------|----------|
| Pneuma | Acolyte | 3x3 | 10s | Blocks ALL ranged physical attacks (range >= 4 cells) |
| Safety Wall | Mage/Priest | 1x1 | 5-50s or hit count | Blocks melee physical attacks. Hit counter: 2-11 hits |
| Ice Wall | Wizard | 1x5 cells | 5+5*Lv seconds | Creates 5 impassable cells. Each cell has HP. Decays over time |

### Status Zones

Ground effects that apply debuffs/status changes to entities within.

| Skill | Class | AoE | Duration | Effect |
|-------|-------|-----|----------|--------|
| Quagmire | Wizard | 5x5 | 5-25s (Lv * 5s) | Reduces AGI, DEX by 5-25 per level. Movement speed reduction. Three-layer reduction (move speed + AGI + DEX) |

### Misc / Utility Zones

| Skill | Class | AoE | Duration | Effect |
|-------|-------|-----|----------|--------|
| Warp Portal | Acolyte | 1x1 | ~10s or 8 players | Teleports players who step on it to a memorized location. Max 3 active per caster. Capacity: 8 players |
| Graffiti | Rogue | 1x1 | 60s? | Displays text on the ground. Cosmetic only |

---

## Ground Effect Mechanics

### Cell-Based Placement

- The RO map uses a 2D cell grid. Each cell is a discrete unit (approximately 50 UE units in our implementation, represented by `CELL_SIZE = 50`).
- Ground-target skills require the player to click a ground cell within skill range.
- The clicked cell becomes the center of the ground effect.
- The effect radiates outward in a square pattern from the center cell.
- **AoE size naming**: "5x5" means 5 cells in each direction from center, including center. So a 5x5 is actually a 2-cell radius from center (center + 2 cells each direction).
- Some skills (Fire Wall, Ice Wall) create non-square patterns (line segments).

### Duration

Ground effects use three duration models:

**Time-based (most common):**
- The effect expires after a fixed number of seconds/milliseconds.
- Duration often scales with skill level.
- Examples: Storm Gust (4.5s), Quagmire (5*Lv seconds), Sanctuary (4-31s), Volcano (Lv * 60s).

**Hit-based (charge counter):**
- The effect has a fixed number of "charges" or "hits" it can deliver.
- Each time the effect blocks/damages, a charge is consumed.
- When charges reach 0, the effect is removed regardless of time remaining.
- Examples: Safety Wall (2-11 hits), Fire Wall (2+Lv hits per cell segment).

**Combined (time + hits):**
- The effect has both a time limit and a charge counter. Whichever is exhausted first causes removal.
- Example: Safety Wall has both a time duration (5-50s) and a hit counter (2-11).

**Wave-based:**
- The effect delivers a fixed number of damage waves, then expires.
- Example: Lord of Vermilion (4 waves), Magnus Exorcismus (Lv waves with 3s immunity window).

### Tick Rate (Damage/Heal Interval)

Different ground effects tick at different rates:

| Skill | Tick Interval | Total Ticks | Notes |
|-------|--------------|-------------|-------|
| Storm Gust | ~450ms | ~10 ticks over 4.5s | Each tick can trigger freeze counter |
| Lord of Vermilion | 1000ms | 4 waves | 20 visual hits displayed as 4 actual damage waves |
| Meteor Storm | ~300ms between meteors | 2-6 meteors per cast | Each meteor does splash damage |
| Sanctuary | 1000ms | Duration/1s | Heals every second |
| Magnus Exorcismus | 3000ms immunity window | Lv waves | Target immune for 3s after each hit; max 5 damage waves achievable |
| Quagmire | Immediate on entry | N/A | Debuff applied on entry/while standing, removed on exit |
| Volcano/Deluge/Gale | 1000ms (buff refresh) | Continuous while standing | Buff lasts 5s, refreshed every tick while inside |
| Fire Wall | ~25-40ms per hit | 40-50 hits/sec | Extremely high hit rate on contact |
| Ice Wall | 1000ms (HP decay) | Duration/1s | Loses 50 HP per second passively |

### AoE Size

Standard AoE sizes used by ground effects:

| AoE Label | Cell Count | Radius (cells from center) | UE Radius (at 50 units/cell) |
|-----------|------------|---------------------------|------------------------------|
| 1x1 | 1 | 0 | 25 |
| 3x3 | 9 | 1 | 75 |
| 5x5 | 25 | 2 | 125 |
| 7x7 | 49 | 3 | 175 |
| 9x9 | 81 | 4 | 225 |
| 11x11 | 121 | 5 | 275 |

**Note on "effective" AoE**: Some skills list an AoE larger than their unit. For instance, Storm Gust is listed as "7x7" in the skill data but has "effective 9x9" because the visual effect extends 1 cell beyond the damage area, and targets on edge cells are still hit. Lord of Vermilion lists "9x9 (effective 11x11)" similarly. This refers to the actual damage/hit detection area including edge-cell rounding.

### Overlap Rules

Ground effect overlap is controlled by rAthena's `UF_` (Unit Flag) system:

**UF_NoReiteration**: The same skill cannot be cast again on cells already occupied by an instance of itself.
- Applies to: Safety Wall, Pneuma, Storm Gust, Lord of Vermilion
- Prevents "double-casting" the same skill on top of itself.

**UF_NoFootSet**: Cannot be placed on a cell where the caster is standing or where a target is standing.
- Applies to: Most traps
- Prevents placing traps directly under a target.

**UF_NoOverlap**: The skill's visual/mechanical effect does not combine with itself.
- Applies to: Storm Gust, Lord of Vermilion
- Multiple instances from different casters in the same location have no more effect than one.
- Targets hit by a NoOverlap skill gain a 500ms immunity to the same skill.

**Skills that DO stack (override NoOverlap):**
- **Meteor Storm**: Multiple casters can stack Meteor Storm on the same location for full combined damage. This is a deliberate design choice making it the best multi-wizard AoE skill.
- **Fire Wall**: Multiple Fire Walls from the same or different casters can overlap.
- **Traps**: Multiple traps can be placed in different cells near each other (not same cell).

**Safety Wall vs Pneuma mutual exclusion:**
- Safety Wall and Pneuma cannot coexist on the same cell.
- Casting one on a cell occupied by the other fails (does not replace).
- This is a critical game mechanic: you cannot be protected from both melee and ranged simultaneously.

**General overlap behavior:**
- Different skill types CAN coexist on the same cell (e.g., Sanctuary + Pneuma, Quagmire + Fire Wall).
- When conflicting effects exist, specific interaction rules apply (see Land Protector section).

### Land Protector Clearing / Blocking Other Ground Effects

Land Protector (Magnetic Earth) is the most important ground effect interaction skill:

**On placement:**
1. All existing ground effects in the LP area that are in the blocked list are immediately destroyed.
2. This includes: Safety Wall, Pneuma, Warp Portal, Sanctuary, Magnus Exorcismus, Volcano, Deluge, Violent Gale, Fire Wall, Fire Pillar, Thunderstorm, Storm Gust, Lord of Vermilion, Meteor Storm, Quagmire, Ice Wall, all Hunter traps.

**While active:**
1. No blocked ground effect can be placed on cells covered by Land Protector.
2. Skill casts that target LP-covered cells are nullified (catalysts may still be consumed depending on implementation).

**LP vs LP (Mutual Destruction):**
- One Sage's Land Protector can destroy another Sage's Land Protector.
- Casting LP on existing LP clears the old one and places the new one.
- This is a key WoE/PvP mechanic: Sages can counter-LP each other.

**LP area by level:**

| Level | AoE | SP | Duration |
|-------|-----|-----|----------|
| 1 | 7x7 | 50 | 120s |
| 2 | 7x7 | 54 | 150s |
| 3 | 9x9 | 58 | 180s |
| 4 | 9x9 | 62 | 210s |
| 5 | 11x11 | 66 | 240s |

**Edge cell note:** Cells on LP's outer rim may not fully block splash-type ground effects due to how splash radius detection works. Effects centered outside LP but splashing onto LP rim cells may still apply.

**Blocked skill list** (our implementation in `ro_ground_effects.js`):
```
safety_wall, pneuma, warp_portal, sanctuary, magnus_exorcismus,
volcano, deluge, violent_gale, fire_wall, fire_pillar,
thunderstorm, storm_gust, lord_of_vermilion, meteor_storm,
quagmire, ice_wall, frost_nova
```

### Max Simultaneous Ground Effects Per Caster

Each caster has limits on how many instances of the same ground effect they can maintain:

| Skill | Max Per Caster | Behavior When Exceeded |
|-------|---------------|----------------------|
| Fire Wall | 3 | Oldest removed to make room |
| Safety Wall | 1 | Oldest removed |
| Pneuma | 1 | Oldest removed |
| Sanctuary | 1 | Oldest removed |
| Ice Wall | 5 | Oldest removed |
| Volcano | 1 | Oldest removed (also removes Deluge/Gale) |
| Deluge | 1 | Oldest removed (also removes Volcano/Gale) |
| Violent Gale | 1 | Oldest removed (also removes Volcano/Deluge) |
| Land Protector | 1 | Oldest removed |
| Traps (general) | No hard per-skill limit | Limited by Trap item inventory + SP |
| Songs/Dances | 1 active performance per performer | New performance cancels old |
| Warp Portal | 3 | Oldest removed when placing 4th |

**Sage elemental zone mutual exclusion:**
- Only ONE of Volcano, Deluge, or Violent Gale can be active per caster at a time.
- Casting any one automatically removes the other two from that caster.
- Different casters CAN have different elemental zones active simultaneously.

---

## Specific Ground Effects (Detailed)

### Fire Wall

**rAthena ID:** `WZ_FIREWALL` (18) / `UNT_FIREWALL`

**Overview:** Creates a line of fire segments perpendicular to the line from caster to target cell. When monsters walk through, each segment deals damage and knocks them back.

**Segment placement:**
- The wall is 3 cells long (1x3 line), centered on the target cell.
- Orientation depends on the direction from caster to target:
  - If cast to the left/right: horizontal wall
  - If cast above/below: vertical wall
  - If cast diagonally: diagonal wall (two segments adjacent to root + two additional to prevent holes)
- The center cell is always on the target cell.

**Per-segment mechanics:**
- Each segment has a charge count: `2 + SkillLevel` hits (Lv1 = 3, Lv10 = 12).
- Each charge hit deals **50% MATK** fire-element magic damage.
- When a monster contacts a segment, it is hit at an extremely high rate (~40-50 hits per second) and knocked back.
- Each hit consumes one charge from that segment.
- When all charges are consumed, that segment disappears.

**Duration by level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration(s) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
| Hits/segment | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| SP | 40 | 40 | 40 | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Cast(s) | 2.0 | 1.85 | 1.7 | 1.55 | 1.4 | 1.25 | 1.1 | 0.95 | 0.8 | 0.65 |

**Special interactions:**
- Boss-flagged monsters are NOT knocked back but must exhaust all hits in a cell before proceeding.
- Undead-property characters are neither blocked nor pushed back; they pass through freely (but still take damage from each hit).
- Stationary/immobile enemies are generally pushed southward by Fire Wall.
- Max 3 Fire Walls per caster. Placing a 4th removes the oldest.
- Destroyed by Land Protector.
- Does not block player movement.

### Sanctuary

**rAthena ID:** `AL_SANCTUARY` (70) / `UNT_SANCTUARY`

**Overview:** Creates a 5x5 holy ground that heals allies every second and damages Undead element/Demon race monsters.

**Healing mechanics:**
- Heals every 1 second (1000ms tick interval).
- Each tick heals a flat amount (not MATK-based):

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Heal/tick | 100 | 200 | 300 | 400 | 500 | 600 | 777 | 777 | 777 | 777 |
| Duration(s) | 4 | 7 | 10 | 13 | 16 | 19 | 22 | 25 | 28 | 31 |
| SP | 15 | 18 | 21 | 24 | 27 | 30 | 33 | 36 | 39 | 42 |

**Undead/Demon damage:**
- Undead element and Demon race monsters that enter Sanctuary take Holy-property damage equal to **half** the healing value (50 damage at Lv1, 388 at Lv10).
- Undead/Demon monsters are knocked back **2 cells** on each tick.
- This damage bypasses DEF (holy-property magic damage).

**Other properties:**
- Cast time: 5s (all levels).
- Catalyst: 1 Blue Gemstone per cast.
- Max 1 Sanctuary per caster.
- Cannot be placed on cells with Land Protector.
- The caster IS healed by their own Sanctuary (standing inside it).

### Pneuma

**rAthena ID:** `AL_PNEUMA` (25) / `UNT_PNEUMA`

**Overview:** Creates a 3x3 cloud that blocks ALL ranged physical attacks targeting entities within.

**Mechanics:**
- Duration: 10 seconds.
- SP Cost: 10.
- Prerequisite: Warp Portal Lv4.
- Blocks ranged physical attacks (attacks with range >= 4 cells).
- Does NOT block magic attacks.
- Does NOT block melee attacks.
- Monster skills cast within 3 cells are always considered melee, so Pneuma does not block them.

**Overlap rules:**
- Cannot be placed on cells occupied by Safety Wall (and vice versa).
- Max 1 Pneuma per caster.
- Multiple casters CAN have Pneuma on different cells simultaneously.

**Common strategy:** Place 2 Pneumas side-by-side and shift between them for continuous coverage, since re-casting on the same cell is slow.

### Safety Wall

**rAthena ID:** `MG_SAFETYWALL` (12) / `UNT_SAFETYWALL`

**Overview:** Creates a 1x1 barrier on a single cell that blocks melee physical attacks until charges are consumed or time runs out.

**Mechanics:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 30 | 30 | 30 | 35 | 35 | 35 | 40 | 40 | 40 | 40 |
| Cast(s) | 4.0 | 3.5 | 3.0 | 2.5 | 2.0 | 1.5 | 1.0 | 1.0 | 1.0 | 1.0 |
| Hits Blocked | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
| Duration(s) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

- Catalyst: 1 Blue Gemstone.
- Blocks melee physical attacks (and some ranged skills at melee range).
- Against monsters: blocks regular attacks, melee skills, and some ranged skills when used at close range.
- Does NOT block magic attacks.
- Expires when hit counter reaches 0 OR when duration ends (whichever first).
- Cannot overlap with Pneuma on the same cell.
- Max 1 Safety Wall per caster.
- Both Mage and Priest can learn this skill.

### Ice Wall

**rAthena ID:** `WZ_ICEWALL` (87) / `UNT_ICEWALL`

**Overview:** Creates a 5-cell-long ice barrier that blocks movement. Each cell is an independent destructible entity.

**Per-cell mechanics:**
- Each cell has HP: `200 + 200 * SkillLevel` (Lv1 = 400 HP, Lv10 = 2200 HP).
- Duration: `5 + 5 * SkillLevel` seconds (Lv1 = 10s, Lv10 = 55s).
- Cells decay passively at **50 HP per second**.
- Cells can be directly attacked by players (Neutral property, no race, no size, 0 DEF, 0 MDEF).
- Monsters cannot directly target or attack Ice Wall cells, but indirect AoE damage can hit them.
- When a cell's HP reaches 0, that cell disappears.

**Movement blocking:**
- Ice Wall cells function as **impassable terrain** -- blocks both monster and player movement.
- Knockback collides with Ice Wall: entities knocked into Ice Wall stop at the wall cell (the wall takes damage from the collision in some implementations).
- The wall is oriented perpendicular to the caster-to-target direction (same alignment logic as Fire Wall but 5 cells long instead of 3).

**Max 5 Ice Walls per caster.** SP Cost: 20 (all levels).

### Storm Gust

**rAthena ID:** `WZ_STORMGUST` (89) / `UNT_STORMGUST`

**Overview:** Summons a blizzard on a targeted area for 4.5 seconds, dealing Water magic damage and freezing enemies.

**Damage mechanics:**
- Duration: ~4.5 seconds total.
- Tick interval: ~450ms (approximately 10 damage ticks).
- Each tick deals MATK% Water damage:

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| MATK% per tick | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |
| Cast(s) | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |

**Freeze mechanic (3-hit counter):**
- Each entity maintains a per-Storm-Gust hit counter.
- Every **3rd hit** has a **150%** base chance to freeze the target (effectively guaranteed on non-boss/non-Undead).
- Frozen targets take no further damage from this Storm Gust instance (they are immune while frozen).
- Frozen enemies pushed 2 cells in a random direction on each tick.
- Freeze fails vs Boss-flagged and Undead-element monsters.

**Stacking rules:**
- `UF_NoReiteration + UF_NoOverlap`: Storm Gust does NOT stack with other instances.
- Two simultaneous Storm Gusts in one location have no more effect than one.
- Targets gain a **500ms immunity** after each hit to prevent double-damage from overlapping instances.
- ACD: 5 seconds.

### Meteor Storm

**rAthena ID:** `WZ_METEOR` (83) / `UNT_METEOR`

**Overview:** Drops a series of fire meteors at random positions within the target area. Each meteor deals splash damage on impact.

**Meteor mechanics:**
- Meteors fall at ~300ms intervals at random positions within the 7x7 area.
- Each meteor has a splash radius of 7x7 (centered on its random impact point, which can extend the effective area to ~13x13).
- Meteor count per cast:

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Meteors | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 5 | 6 | 6 |

- Each meteor hit deals **125% MATK** fire-element magic damage.
- Each hit has a chance to inflict **Stun** status.

**Stacking (UNIQUE among AoE ground effects):**
- Meteor Storm is the one major AoE that **DOES stack**.
- Multiple casters can all cast Meteor Storm on the same location for full combined damage.
- This makes coordinated Meteor Storm the highest-damage multi-wizard strategy.
- ACD: `2 + 0.5 * Lv` seconds.

### Quagmire

**rAthena ID:** `WZ_QUAGMIRE` (92) / `UNT_QUAGMIRE`

**Overview:** Transforms an area into marshland that severely debuffs AGI, DEX, and movement speed.

**Debuff mechanics:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| AGI Reduction | 5 | 10 | 15 | 20 | 25 |
| DEX Reduction | 5 | 10 | 15 | 20 | 25 |
| Duration(s) | 5 | 10 | 15 | 20 | 25 |
| SP | 5 | 10 | 15 | 20 | 25 |

**Three-layer speed reduction:**
1. Movement speed reduction (entities move slower while inside)
2. AGI reduction (affects ASPD, FLEE)
3. DEX reduction (affects HIT, cast time, ranged damage)

**Stat reduction caps:**
- Monsters: Cannot reduce stats by more than 50%.
- Players: Cannot reduce stats by more than 25%.

**Additional effects:**
- Strips certain buffs on entry (Increase AGI, Wind Walk, etc.).
- Works on Boss-flagged monsters (one of few debuffs that does).
- AoE: 5x5.
- Prerequisite: Heaven's Drive Lv1.

### Volcano / Deluge / Violent Gale (Whirlwind)

**rAthena IDs:** `SA_VOLCANO` (285) / `SA_DELUGE` (286) / `SA_VIOLENTGALE` (287)

**Overview:** Three Sage elemental ground zones that boost a specific element's damage and provide a secondary stat bonus to targets with matching armor element.

**Volcano (Fire):**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Fire Damage Boost | +10% | +14% | +17% | +19% | +20% |
| ATK Bonus (fire armor) | +10 | +20 | +30 | +40 | +50 |

**Deluge (Water):**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Water Damage Boost | +10% | +14% | +17% | +19% | +20% |
| MaxHP Boost (water armor) | +5% | +9% | +12% | +14% | +15% |

**Violent Gale (Wind):**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Wind Damage Boost | +10% | +14% | +17% | +19% | +20% |
| FLEE Bonus (wind armor) | +3 | +6 | +9 | +12 | +15 |

**Common properties (all three):**
- AoE: 7x7.
- Duration: `Lv * 60` seconds (1-5 minutes).
- SP Cost: 40-48 (varies slightly per skill).
- Catalyst: 1 Yellow Gemstone.
- Buff tick interval: 1000ms (buff refreshed every second while standing inside).
- Buff lingers for 5 seconds after leaving the zone.
- The secondary stat bonus (ATK/MaxHP/FLEE) only applies if the target's armor element matches the zone's element.
- The elemental damage boost applies to all attackers regardless of armor element.
- Only one of the three can be active per caster (mutual exclusion). Casting one removes the other two.
- Different casters can have different zones active simultaneously (a Volcano and a Deluge can coexist from two different Sages).
- Destroyed by Land Protector.

### Land Protector (Magnetic Earth)

**rAthena ID:** `SA_LANDPROTECTOR` (288) / `UNT_LANDPROTECTOR`

**Overview:** Creates a zone that prevents and destroys other ground-targeted skills. The ultimate ground effect counter.

**Full mechanics:**
- See [Land Protector Clearing section](#land-protector-clearing--blocking-other-ground-effects) above.
- Catalysts: 1 Blue Gemstone + 1 Yellow Gemstone.
- Max 1 LP per caster.
- LP does NOT block targeted (non-ground) skills. Bolts, heals, and targeted debuffs still work normally on targets inside LP.
- LP vs LP: mutual destruction -- casting LP on existing LP removes the old one.

### Traps (Hunter)

See [Trap Zones section](#trap-zones) above for the complete list.

**General trap lifecycle:**
1. **Placement**: Caster uses trap skill, trap appears on the ground (visible to all).
2. **Arming**: There is a brief arming delay (~1-2 seconds) before the trap becomes active.
3. **Idle**: Trap sits on the ground waiting for a target to step on it. Duration: 300s (5 minutes) for most, 600s for Talkie Box.
4. **Trigger**: When a valid target enters the trap's cell, the trap activates.
5. **Effect**: The trap's effect fires (damage, status, displacement, etc.).
6. **Destruction**: The trap is consumed on trigger. Some traps (damage types) affect the AoE area, others (status types) affect only the triggering target.
7. **Expiry**: If not triggered within the idle duration, the trap disappears and the Trap item is returned to the caster.

**Damage trap formulas (pre-renewal):**
- **Land Mine**: `[DEX * (3 + BaseLv/100) * (1 + INT/35)] * Lv +/- 10% + (TrapResearchLv * 40)` -- Earth element
- **Blast Mine**: `[DEX * (2 + BaseLv/100) * (1 + INT/35)] * Lv +/- 10% + (TrapResearchLv * 40)` -- Wind element, 3x3 AoE
- **Claymore Trap**: `[DEX * (3 + BaseLv/85) * (1.1 + INT/35)] * Lv +/- 10% + (TrapResearchLv * 40)` -- Fire element, 5x5 AoE
- **Freezing Trap**: `(25 + 25*Lv)%` of caster's ATK -- Water element, 3x3 AoE + Freeze chance

**Status trap effects:**
- **Ankle Snare**: Immobilize for `5*Lv - TargetAGI/10` seconds (min 0). Boss duration = 1/5. Target can still attack and use skills while snared.
- **Sandman**: Sleep status with `(40 + 10*Lv)%` chance on 5x5 area.
- **Flasher**: Blind status on 5x5 area.
- **Shockwave Trap**: Drains `(5 + 15*Lv)%` of target's SP. Costs 2 Trap items.
- **Skid Trap**: Slides target 6+Lv cells in the direction the caster was facing when the trap was set.

### Performance AoE (Bard/Dancer)

**Solo Performance (follows caster):**
- AoE: 7x7, centered on the caster.
- The effect area MOVES with the caster (though at reduced movement speed: `25 + 2.5*MusicLessons%` of normal).
- The performer themselves does NOT benefit from their own song/dance.
- Buff effect lingers for 20 seconds after allies leave the area or the performance ends.
- Only 1 active performance per performer. Starting a new performance cancels the old one (causes the old buff to "flash" briefly).
- Weapon swap cancels the performance.
- Dispel cancels the performance.
- Heavy damage (>25% MaxHP in a single hit) cancels the performance.
- Silence does NOT cancel solo performances.
- The performer cannot use regular attacks during performance (except Musical Strike / Slinging Arrow which are special performance-only attacks).

**Ensemble (stationary at midpoint):**
- AoE: 9x9, centered at the **midpoint** between the Bard and Dancer.
- The effect area is STATIONARY -- it does not move.
- Both performers must remain within range of each other or the ensemble ends.
- Neither performer can move during an ensemble.
- Performers themselves MAY be affected by certain ensemble effects (varies by skill).
- When the ensemble ends, both performers suffer an "aftermath" debuff period.
- Ensemble skills require one Bard and one Dancer performing simultaneously.

**Performance ground effect types:**

| Skill | Performer | Effect | AoE |
|-------|-----------|--------|-----|
| A Poem of Bragi | Bard | Reduces cast time + ACD | 7x7 (solo) |
| Apple of Idun | Bard | MaxHP boost + HP regen | 7x7 |
| Assassin Cross of Sunset | Bard | ASPD boost | 7x7 |
| A Whistle | Bard | FLEE + Perfect Dodge boost | 7x7 |
| Humming (Perfect Tablature) | Dancer | HIT boost | 7x7 |
| Please Don't Forget Me | Dancer | ASPD + Move Speed reduction (debuff) | 7x7 |
| Service for You | Dancer | MaxSP boost + SP regen | 7x7 |
| Fortune's Kiss | Dancer | CRI boost | 7x7 |
| Lullaby | Ensemble (9x9) | Sleep chance on enemies | 9x9 |
| Mr. Kim A Rich Man | Ensemble (9x9) | EXP bonus | 9x9 |
| Eternal Chaos | Ensemble (9x9) | Reduces DEF to 0 | 9x9 |
| Into the Abyss | Ensemble (9x9) | No gemstone consumption | 9x9 |
| Invulnerable Siegfried | Ensemble (9x9) | Element resist + status resist | 9x9 |
| A Drum on the Battlefield | Ensemble (9x9) | ATK + DEF boost | 9x9 |
| Ring of Nibelungen | Ensemble (9x9) | ATK boost (Lv4 weapons only) | 9x9 |
| Loki's Veil | Ensemble (9x9) | Prevents skill use in area | 9x9 |
| Down Tempo (Moonlit Water Mill) | Ensemble (9x9) | Movement barrier (removed in some builds) | 9x9 |

---

## Implementation Checklist

### Ground Effect Infrastructure (ro_ground_effects.js)

- [x] `GroundEffect` typedef with all required fields
- [x] `createGroundEffect()` -- placement with LP check, caster limits, sage mutual exclusion
- [x] `removeGroundEffect()` -- cleanup with onExpire callback
- [x] `getGroundEffectsAt()` -- spatial query for effects at a position
- [x] `getGroundEffectsInZone()` -- all effects in a zone
- [x] `countEffects()` / `removeOldestEffect()` / `removeAllEffects()` -- per-caster management
- [x] `isBlockedByLandProtector()` -- LP placement check
- [x] `hasImmunity()` / `recordHit()` -- per-target hit cooldown (Magnus Exorcismus, Storm Gust)
- [x] `findEntitiesInEffect()` -- find all enemies + players inside a ground effect radius
- [x] `cleanupZone()` -- zone-wide cleanup on zone reset
- [x] `LAND_PROTECTOR_BLOCKED` set -- list of skills blocked by LP
- [x] `CASTER_LIMITS` map -- per-caster limits per skill type
- [x] `SAGE_ZONE_GROUP` set -- mutual exclusion group
- [x] `EFFECT_CATEGORY` enum -- damage_zone, heal_zone, buff_zone, debuff_zone, trap, obstacle, protector, contact

### Individual Ground Effect Handlers (server/src/index.js)

- [x] **Volcano** -- Fire buff zone (7x7, ATK + fire damage boost, element-restricted stat bonus)
- [x] **Deluge** -- Water buff zone (7x7, MaxHP + water damage boost, element-restricted)
- [x] **Violent Gale** -- Wind buff zone (7x7, FLEE + wind damage boost, element-restricted)
- [x] **Land Protector** -- Ground effect blocker/destroyer
- [x] **Quagmire** -- AGI/DEX debuff zone + movement slow
- [x] **Storm Gust** -- Water AoE damage + freeze counter
- [x] **Meteor Storm** -- Fire AoE random meteors + stun (STACKS)
- [x] **Lord of Vermilion** -- Wind AoE damage waves + blind
- [x] **Fire Wall** -- Contact-trigger fire damage + knockback
- [x] **Fire Pillar** -- Trap-style ground trigger + splash fire damage
- [x] **Ice Wall** -- 5-cell movement blocker with HP decay
- [x] **Sanctuary** -- Holy heal zone + Undead/Demon damage
- [x] **Magnus Exorcismus** -- Holy damage zone (Demon/Undead only, 3s immunity)
- [x] **Pneuma** -- Ranged attack blocker
- [x] **Safety Wall** -- Melee attack blocker with charge counter
- [x] **Warp Portal** -- Teleport on step
- [x] **All Hunter traps** -- Trigger-on-step with damage/status effects
- [x] **Demonstration** -- Alchemist fire ground DoT
- [x] **Bard songs** -- Following buff zone (7x7)
- [x] **Dancer dances** -- Following buff zone (7x7)
- [x] **Ensemble skills** -- Stationary buff zone (9x9, midpoint)

### Client-Side Rendering

- [ ] Ground effect VFX placement on `skill:ground_effect_placed` event
- [ ] Ground effect removal on `skill:ground_effect_removed` event
- [ ] Duration-based auto-removal (client timer)
- [ ] Per-skill Niagara VFX assets (fire circles, ice barriers, healing auras, trap models, song notes)
- [ ] Buff/debuff icon display when entering a zone
- [ ] Zone entry/exit visual feedback

### Tick System (server combat loop)

- [x] Ground effect tick loop (checks all active effects every `GROUND_EFFECT_TICK_MS`)
- [x] Buff zone refresh ticks (Volcano/Deluge/Gale every 1000ms)
- [x] Damage zone ticks (Storm Gust, LoV, Magnus every interval)
- [x] Trap proximity detection (check entity positions vs trap cells)
- [x] Ice Wall HP decay (50 HP/sec)
- [x] Expired effect cleanup
- [x] Performance AoE position update (follows caster)
- [x] Ensemble position calculation (midpoint)

---

## Gap Analysis

### Compared to rAthena Pre-Renewal Reference

| Feature | rAthena | Our Implementation | Status |
|---------|---------|-------------------|--------|
| Cell-based grid system | Discrete cell occupancy per unit | Radius-based distance check | Simplified -- uses circular radius instead of square cell grid. Functionally equivalent for most cases but edge-cell behavior differs |
| UF_NoReiteration flag | Prevents same-skill re-placement on occupied cells | Handled by CASTER_LIMITS (oldest removed) | Different approach -- we remove oldest instead of blocking placement |
| UF_NoOverlap flag | Same-skill instances don't combine damage | 500ms immunity window on Storm Gust/LoV | Implemented via immunityWindowMs |
| UF_NoFootSet flag | Cannot place on occupied cells | Not implemented for traps | Minor gap -- traps can be placed under targets currently |
| Fire Wall direction calculation | Full 8-direction perpendicular calculation | Simplified | May need directional segment placement |
| Fire Wall per-segment charges | Each of 3 segments has independent charge count | Single effect with total charges | Could be improved to per-segment tracking |
| Meteor Storm random placement | Random cell within AoE per meteor | Implemented | Working |
| Warp Portal player counter | 8 players max per portal | Partially implemented | Needs player count tracking |
| Trap arming delay | ~1-2 second delay before trap activates | Not implemented | Minor -- traps activate immediately on placement |
| Performance AoE follows caster | Continuous position update | Implemented via performance system | Working |
| Ensemble midpoint calculation | Midpoint of Bard + Dancer positions | Implemented | Working |
| Ground effect visual sync | Client receives placement + removal events | Implemented | Working -- needs more VFX assets |
| LP edge-cell behavior | Rim cells may not fully block splash effects | Full radius blocking | Minor difference |

### Key Differences from rAthena

1. **Circular vs Square AoE**: Our implementation uses circular distance checks rather than cell-grid square occupancy. A "5x5" in rAthena is a strict 5-cell square; ours is a circle with radius matching the square's inscribed circle. This can cause minor differences at corner cells.

2. **Per-segment Fire Wall**: rAthena tracks each Fire Wall cell independently with its own charge counter. Our current implementation treats Fire Wall as a single zone. For accurate behavior, each of the 3 segments should be a separate ground effect entity.

3. **Trap placement restrictions**: rAthena enforces UF_NoFootSet (cannot place within 2 cells of entities) for traps. Our system does not enforce this restriction.

4. **Ground effect ID tracking**: rAthena uses Unit Group IDs (`UNT_*`) for visual and behavioral categorization. Our system uses string type names. Functionally equivalent but different data structure.

5. **Water Ball cell consumption**: Water Ball (Wizard) consumes Deluge cells when cast on a Deluge zone, increasing its hit count. This interaction is not currently implemented.

---

*Document generated via deep research from iRO Wiki Classic, rAthena pre-renewal source, RateMyServer, Ragnarok Research Lab, divine-pride.net, rAthena GitHub, Hercules Board Archive, GameFAQs RO guides, and RO community resources.*
