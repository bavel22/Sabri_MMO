# Skill VFX Research Findings — Complete Reference

**Created**: 2026-03-05
**Scope**: Exhaustive research on implementing RO-style skill visual effects in UE5
**Research Agents**: 4 parallel agents, 60+ sources, covering RO visuals, UE5 Niagara, AI tools, recreation methods
**Purpose**: Raw reference document — all findings, no filtering

---

## Table of Contents

- [Part 1: Ragnarok Online Classic — Complete Skill Visual Effects Reference](#part-1-ragnarok-online-classic--complete-skill-visual-effects-reference)
- [Part 2: Unreal Engine 5 VFX Technical Reference](#part-2-unreal-engine-5-vfx-technical-reference)
- [Part 3: AI Tools, MCP Servers, and Automated VFX Pipelines](#part-3-ai-tools-mcp-servers-and-automated-vfx-pipelines)
- [Part 4: RO VFX Recreation in Modern Engines](#part-4-ro-vfx-recreation-in-modern-engines)
- [Part 5: Current Sabri_MMO Skill System State](#part-5-current-sabri_mmo-skill-system-state)
- [Part 6: Complete Source List](#part-6-complete-source-list)

---

# Part 1: Ragnarok Online Classic — Complete Skill Visual Effects Reference

## 1.1 The RO Effect System — Technical Background

RO uses **four distinct rendering approaches** for visual effects:

| Format | Type | Usage | Files |
|--------|------|-------|-------|
| **STR** | Compiled binary keyframe | Most skill effects | `.str` + `.tga` textures |
| **EZV** | Raw text (dev format) | Source format for STR | `.ezv` |
| **ACT/SPR** | Sprite animation | Simpler effects, character sprites | `.act` + `.spr` |
| **Hardcoded** | Client executable | 3D particles, geometric primitives | N/A |
| **Lua** | Script-based | Newer particle effects (recent clients) | `.lub`/`.lua` |

### STR File Structure
- **Layers**: Multiple independent visual layers stacked together, each with own textures, transforms, blending
- **Keyframes**: Timeline of animation states per layer, with interpolation between them
- **Properties per keyframe**: Position (XY), rotation, scale, UV coordinates, blend mode, bias/easing, color/alpha
- **Blend modes**: Control how layers composite over each other and the game world
- **Interpolation**: Easing functions and bezier curves for smooth transitions

### ACT/SPR System
- **SPR files**: Indexed-color bitmaps with palettes + truecolor TGA-segment images with per-pixel alpha
- **ACT files**: Define animations with actions, frames, intervals, and layer stacking
- Casting effects "assemble many smaller effects to form advanced visual effects"

### Effect ID System
RO maps skills to numeric effect IDs. Master list: `doc/effect_list.txt` in rAthena/Hercules

---

## 1.2 The Iconic Casting Circle (magic_target.tga) — EXTREME DETAIL

This is the single most important visual element to replicate. Every RO player recognizes it instantly.

### Physical Description
- **Texture file**: `data\texture\effect\magic_target.tga`
- **Effect ID**: 60 (EF_LOCKON — "Cast target circle")
- **Shape**: A flat, circular magical glyph projected onto the ground plane
- **Structure**: Semi-transparent disc with:
  - Outer ring: concentric bands with small geometric rune-like markings evenly spaced around the circumference
  - Inner area: fainter secondary ring pattern with additional arcane symbols
  - The texture itself is roughly 256x256 pixels in the original client
- **Rendering**: Single TGA texture rendered as a flat quad projected onto the ground plane with **additive blending**, rotated each frame

### Animation
- **Rotation**: Slow clockwise rotation, continuous for the entire cast duration
- **Rotation speed**: Approximately 30-45 degrees per second (one full revolution every 8-12 seconds)
- **Position**: At the caster's feet for single-target spells, at the target ground location for ground-target spells
- **For ground-target skills**: During the targeting phase (before cast starts), the circle follows the cursor. When the player clicks, it locks to that location and stays there during the entire cast.

### Color
- **Default**: Blue-white/cyan, the primary ring glows a soft cyan/blue-white
- **Element variants** (Effect IDs 54-59, EF_BEGINSPELL2-7):
  - Water (EF_BEGINSPELL2, ID 54): Blue
  - Fire (EF_BEGINSPELL3, ID 55): Red-orange
  - Wind (EF_BEGINSPELL4, ID 56): Yellow-green
  - Earth (EF_BEGINSPELL5, ID 57): Brown
  - Holy (EF_BEGINSPELL6, ID 58): White-gold
  - Poison (EF_BEGINSPELL7, ID 59): Green-purple

### Size
- Scales to approximately match the skill's AoE footprint
- For a 5x5 AoE skill (like Thunderstorm): spans ~5 cells
- For a 1x1 skill (like Safety Wall): smaller circle, roughly 1 cell diameter
- Each RO cell is about 50 pixels/units in the original 2D client

### Timing
- **Appears**: Instantly on cast start (or when targeting for ground-target skills)
- **Disappears**: Instantly on cast complete or cast interrupted (no fade in the original)
- **Duration**: Matches cast time exactly

### Customization
- Private servers can replace `magic_target.tga` in the GRF to customize the look
- rAthena community has created [19+ alternative magic target textures](https://rathena.org/board/topic/75230-magic-target-collectionfree-download/) — all freely downloadable
- Players also discuss [changing spell circles](https://rathena.org/board/topic/86998-how-to-change-spell-circle/) by replacing the texture file

---

## 1.3 Novice Skills

### First Aid (Skill ID 142)
**Effect ID**: 309 (EF_FIRSTAID)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | No cast time. Instant activation. |
| **Visual on caster** | A small green cross/plus symbol briefly appears above or on the character. Tiny green sparkle particles float upward. |
| **Projectile** | None. Self-targeted only. |
| **Impact** | The green cross flashes once. A few small green/white particles drift upward and fade. |
| **Ground indicator** | None. |
| **Color palette** | Green (cross), white/green (sparkle particles). Similar to Heal but much smaller/simpler. |
| **Duration** | ~0.3-0.5 seconds. Very quick flash. |
| **Behavior** | One-shot. Single flash and done. |

### Basic Attack (Normal Hit)
**Effect ID**: 0 (EF_HIT1 — "Regular Hit")

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Character plays attack animation (weapon swing). Swordsmen: forward slash. Mages: staff jab/swing. |
| **Projectile** | None for melee. Ranged classes produce arrow/projectile sprite traveling to target. |
| **Impact** | White starburst/cross-shaped flash at point of impact. The classic RO "hit mark" — a brief white angular slash/cross that expands outward and fades. Composed of 2-3 white streak lines radiating from center, like a small white "X" or asterisk flash. |
| **Ground indicator** | None. |
| **Color palette** | Pure white with slight transparency falloff at edges. |
| **Duration** | ~0.15-0.2 seconds. Extremely brief. |
| **Behavior** | One-shot per hit. |

---

## 1.4 Swordsman Skills

### Bash (Skill ID 5)
**Effect ID**: 1 (EF_HIT2 — "Bash")

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Swordsman performs an exaggerated, forceful melee swing — wider, more dramatic slash than basic attack. |
| **Projectile** | None. Melee range. |
| **Impact** | Larger, more pronounced hit flash than basic attack. The Bash hit mark is a bigger white-yellow starburst/slash effect — a brighter, wider cross/slash flash. EF_HIT2 is a "heavier" version of EF_HIT1 with more prominent white-yellow slash lines radiating outward. At Level 6+ with Fatal Blow, if stun procs, a brief stun star icon appears above target. |
| **Ground indicator** | None. |
| **Color palette** | White with slight yellow tint. Brighter and larger than basic attack. |
| **Duration** | ~0.2-0.3 seconds for the hit flash. |
| **Behavior** | One-shot per use. |

### Magnum Break (Skill ID 7)
**Effect ID**: 17 (EF_MAGNUMBREAK)
**Texture files**: `ring_yellow.tga` (ground ring component)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Swordsman performs a spinning slash animation — character visually whirls around twice. HP decreases slightly (costs HP). |
| **Projectile** | None — AoE burst centered on caster. |
| **Impact** | Fiery explosion erupts outward from caster in all directions: (1) Bright orange-red-yellow fire ring/blast wave expands outward covering 5x5 cell area, (2) Flame-colored particle effects (orange/red/yellow) burst outward, (3) Enemies knocked back 2 cells with visible push/slide animation, (4) Ground beneath briefly flashes with warm orange-yellow ring (`ring_yellow.tga`). |
| **Ground indicator** | Orange-yellow ring on ground expands momentarily. |
| **Color palette** | Orange, red, yellow (fire tones). Bright warm explosion colors. |
| **Duration** | Explosion animation ~0.5-0.7 seconds. Fire property buff (20% bonus fire damage) lasts 10 seconds but has no persistent visual. |
| **Behavior** | One-shot burst animation. |

### Provoke (Skill ID 6)
**Effect ID**: 67 (EF_PROVOKE)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Swordsman points or gestures aggressively toward target. Brief casting gesture animation. |
| **Projectile** | None — instant effect on target. |
| **Impact** | A red exclamation mark ("!") or anger symbol briefly appears above the provoked target. Target flashes with a brief reddish tint/pulse. Small burst of red-tinted particles may appear around target. |
| **Ground indicator** | None. |
| **Color palette** | Red/dark red. The exclamation/anger indicator is red. |
| **Duration** | Visual effect ~0.3-0.5 seconds (flash). Debuff lasts 30 seconds but visual is momentary. |
| **Behavior** | One-shot flash on application. Status icon appears in target's buff bar. |

### Endure (Skill ID 8)
**Effect ID**: 11 (EF_ENDURE)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Swordsman briefly flexes or braces. Yellow-golden aura briefly flashes around character. |
| **Projectile** | None — self-buff. |
| **Impact** | N/A (self only). Yellow-golden translucent glow/aura briefly envelops caster upon activation. |
| **Ground indicator** | None. |
| **Color palette** | Yellow/golden. Warm metallic gold tones. |
| **Duration** | Activation flash ~0.3-0.5 seconds. Buff persists for duration (up to 37 seconds) but no persistent visible aura — only status icon in buff bar. |
| **Behavior** | One-shot activation flash. Status icon display for duration. |

### Sword Mastery / Two-Hand Sword Mastery / Increase HP Recovery
**Passive skills — NO visual effects.** Stat bonuses only. Appear only in skill tree UI.

---

## 1.5 Mage Skills

### General Mage Casting Animation
All mage skills with cast time share a common casting animation:
- **Casting pose**: Sprite switches to specific frame — character raises both hands/arms upward (or holds staff upward), body facing target direction
- **Cast glow**: Blue-white glow/shimmer effect appears on/around character. Small blue-white sparkle particles may orbit caster
- **Ground circle**: For ground-targeted spells, casting circle (`magic_target.tga`) appears at target location, rotating slowly
- **Duration**: Casting pose persists for entire cast time. On completion, character briefly transitions to attack/release frame before returning to idle

### Cold Bolt (Skill ID 14)
**Effect ID**: Ice bolt effect (ice chunks falling from sky)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose with blue-white glow. Cast time increases with level. |
| **Projectile** | No traveling projectile. Large angular **ice chunks/shards** materialize above target and descend straight down, striking from above. Each chunk is a distinct sprite — jagged, angular, semi-transparent blue-white crystalline ice shard. |
| **Impact** | Each ice chunk strikes target sequentially, creating brief white-blue flash/burst on impact. Small spray of ice particle fragments explodes outward on each hit. Number of visual strikes = skill level (Level 5 = 5 ice chunks in rapid succession). |
| **Ground indicator** | Casting circle at target during cast time. |
| **Color palette** | Ice blue, cyan, white, pale translucent blue. Ice shards rendered with semi-transparency giving crystalline/glass-like appearance. |
| **Duration** | Each bolt hits ~0.15-0.2 seconds apart. Level 10 = ~1.5-2 seconds total of ice chunks raining. |
| **Behavior** | One-shot sequence. Each level adds one more ice chunk. |

### Fire Bolt (Skill ID 19)
**Effect ID**: Fire bolt effect (fire pillars/bolts from sky)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose with blue-white glow. |
| **Projectile** | No traveling projectile. **Pillars/bolts of flame** drop from above onto target. Each bolt: narrow, elongated column of fire — bright orange-red at core with yellow-white edges, descending rapidly from top of screen to target. |
| **Impact** | Each fire bolt strikes sequentially, producing orange-red flash/burst and small flame particle spray on impact. Fire-colored particles briefly scatter outward. Bolt count = skill level. |
| **Ground indicator** | Casting circle at target during cast time. |
| **Color palette** | Orange, red, yellow-white (fire tones). Bolts have bright orange-red core with lighter yellow-white outer glow. |
| **Duration** | Similar timing to Cold Bolt — each bolt spaced ~0.15-0.2 seconds apart. |
| **Behavior** | One-shot sequence. Bolt count = skill level. |

### Lightning Bolt (Skill ID 20)
**Effect ID**: 29 (EF_LIGHTBOLT)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose with blue-white glow. |
| **Projectile** | No traveling projectile. **Bolts of lightning** strike downward from above onto target. Each bolt: bright jagged/zigzag lightning line descending rapidly from sky, flashing brilliantly white-yellow. |
| **Impact** | Each strike creates bright white-yellow flash at target position with electrical spark/crackle particles radiating outward. Impact area briefly illuminates. Bolt count = skill level. |
| **Ground indicator** | Casting circle at target during cast time. |
| **Color palette** | Bright white, electric yellow, pale blue. Bolts are predominantly white-yellow with slight blue-electric tinge. |
| **Duration** | Same rapid-fire timing as other bolts. Each strike spaced ~0.15-0.2 seconds. |
| **Behavior** | One-shot sequence. Bolt count = skill level. |

### Soul Strike (Skill ID 13)
**Effect ID**: 15 (EF_SOULSTRIKE)
**Texture files**: `pok1.tga`, `pok3.tga`, `ring_blue.tga`, `lens1.tga`, `lens2.tga`

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. Brief blue-purple glow around caster. |
| **Projectile** | **Light ghosts** — translucent white-purple orb-like projectiles launched from caster toward target. Travel HORIZONTALLY through air (unlike bolt skills descending from above). Each ghost/orb: soft, glowing, semi-transparent sphere with slight trailing tail. Rendered using `ring_blue.tga` (blue ring/glow) and lens flare textures (`lens1.tga`, `lens2.tga`) for bright core. |
| **Impact** | Each ghost strikes target with soft white-purple burst (`pok1.tga`, `pok3.tga` impact sprites). Brief flash of blue-white energy at impact point. Multiple hits at higher levels (Level 1 = 1 ghost, up to 5 at Level 10, hitting in pairs). |
| **Ground indicator** | None (single-target, not ground-targeted). |
| **Color palette** | White, pale purple/violet, soft blue. Ghosts have luminous, ethereal quality — pale and ghostly rather than vivid. |
| **Duration** | Travel time depends on distance. Each ghost arrives ~0.15-0.2 seconds after previous. |
| **Behavior** | One-shot sequence per cast. Projectiles travel caster → target. |

### Napalm Beat (Skill ID 11)
**Effect ID**: 32 (EF_NAPALMBEAT — "Small clustered explosions")

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. No cast time in classic RO. |
| **Projectile** | Invisible or very faint psychokinetic projectile travels caster to target. Small purple/violet energy wisps may travel from caster to target area. |
| **Impact** | Cluster of **small purple-violet explosions** erupts at target location and surrounding area (3x3 cell splash). Multiple small burst effects — puffs of purple-violet energy detonating in rapid succession. Distinctly "clustered" — several small pops rather than one big explosion. |
| **Ground indicator** | None. |
| **Color palette** | Purple, violet, dark magenta. Ghost/psychic purple color family. |
| **Duration** | ~0.3-0.5 seconds for full cluster. |
| **Behavior** | One-shot burst. All small explosions nearly simultaneous. |

### Fire Ball (Skill ID 17)
**Effect ID**: 24 (EF_FIREBALL)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. |
| **Projectile** | Caster conjures a **large spherical fireball** lobbed in a **parabolic arc** toward target. Sprite: bright, glowing orange-red-yellow sphere with fiery trailing tail/wake behind it. Visually much larger than individual fire bolts — single chunky ball of flame arcing through air. |
| **Impact** | Fireball **explodes** with fiery burst — expanding ring/sphere of orange-red flame particles. Affects 5x5 cell area. Inner 3x3 = full damage; outer ring (5x5 minus 3x3) = 75% damage. Bright orange-red-yellow burst with flame particles scattering outward. |
| **Ground indicator** | Casting circle during cast time. |
| **Color palette** | Bright orange, red, yellow-white. Fireball: vivid orange-red with yellow-white hot core. |
| **Duration** | Projectile flight ~0.3-0.5 seconds (distance dependent), explosion ~0.3-0.4 seconds. |
| **Behavior** | One-shot. Single projectile, single explosion. |

### Fire Wall (Skill ID 18)
**Effect ID**: 25 (EF_FIREWALL)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. Short cast time. |
| **Projectile** | None. |
| **Impact per contact** | Enemies walking into fire wall take fire damage per hit and are **knocked back**. Each contact produces fire damage flash. |
| **Persistent visual** | **Primary visual**: 1x3 cell line of fire pillars on ground, oriented perpendicular to caster→target cell line. Three cells display vertical columns of animated flame. Each cell: looping fire animation with flickering, dancing flames rising upward from ground. Flames = animated sprite sequences of fire columns. Orientation depends on caster-to-target angle: horizontal, vertical, or diagonal. |
| **Color palette** | Orange, red, yellow (classic fire). Bright orange-red flames with yellow tips. |
| **Duration** | Persists until hit count depleted (4 + SkillLevel hits per cell) or duration expires (up to ~12s at Level 10). Fire animation **loops continuously** while wall exists. |
| **Behavior** | **Looping** flame animation per cell. Fades/disappears when depleted or expired. |

### Frost Diver (Skill ID 15)
**Effect ID**: 27 (EF_FROSTDIVER — "Traveling to Target")
**Texture files**: `ice.tga`, `pok1.tga`, `pok3.tga`, `smoke.tga`
**Sprite sheet**: 485x146 pixels (from The Spriters Resource)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose with brief blue glow. |
| **Projectile** | An **ice spike/wave** travels along the ground from caster to target at very high speed. Sharp, pointed ice crystal/spike slides rapidly along ground surface. Jagged, angular blue-white ice shard/wave rushing horizontally toward target using `ice.tga` crystalline texture. |
| **Impact** | Ice spike shatters with impact burst (`pok1.tga`, `pok3.tga`) and puff of cold mist/smoke (`smoke.tga`). |
| **Freeze effect** | If freeze procs: target becomes **encased in ice block** — character sprite overlaid with translucent blue-white ice crystal encasement. Frozen target appears trapped inside semi-transparent block of ice, unable to move. Ice block has angular, crystalline facets and blue-white coloring. |
| **Ground indicator** | None (single-target). |
| **Color palette** | Ice blue, cyan, white, pale translucent blue. Traveling spike: sharp blue-white; frozen encasement: translucent pale blue. |
| **Duration** | Projectile travel very fast (near-instantaneous). Frozen status lasts several seconds (varies by level). Ice block overlay **persists** on frozen target until status expires or broken by damage. |
| **Behavior** | One-shot projectile. Frozen status overlay persistent/looping until broken. |

### Thunderstorm (Skill ID 21)
**Effect ID**: 30 (EF_THUNDERSTORM)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. Cast time varies by level. |
| **Projectile** | None — area effect. |
| **Impact** | Multiple **lightning bolts** rain down from sky randomly across targeted 5x5 cell area. Each bolt: bright white-yellow jagged lightning strike descending from above, similar to Lightning Bolt but scattered. Bolts strike sequentially (1 per 0.2 seconds), creating repeated flashes of white-yellow electrical energy throughout AoE zone. |
| **Ground indicator** | **Casting circle** (`magic_target.tga`) at target location during cast time, rotating. AoE zone indicated by this circle. During lightning rain, ground area flickers with reflected white-yellow light from each strike. |
| **Color palette** | White, electric yellow, pale blue. Same palette as Lightning Bolt distributed over area. |
| **Duration** | Total animation = (skill level) x 0.2 seconds approximately. Level 10 = ~2 seconds of lightning rain. Each individual bolt flash ~0.15 seconds. |
| **Behavior** | One-shot sequence of multiple random strikes across AoE. Not looping — plays through once. |

### Safety Wall (Skill ID 12)
**Effect ID**: 315 (EF_GLASSWALL2)
**Texture files**: `safetywall.str`, `safeline.bmp`, `freeze_ice_part.bmp`, `alpha_down.tga`

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. Consumes 1 Blue Gemstone. |
| **Projectile** | None. |
| **Impact** | When melee attack hits a character protected by Safety Wall, attack is nullified — no damage number. Brief white/blue flash may appear at wall indicating attack absorbed. |
| **Persistent visual** | A **pink-white translucent light pillar** on targeted cell. Glowing vertical column of soft pink/magenta-white light rising from ground. Semi-transparent with gentle pulsing glow. Protects single cell (1x1). Base uses `safeline.bmp` for ground ring; pillar rendered via `safetywall.str`. |
| **Color palette** | Pink, magenta, white. Soft rosy pink-white glow. Some describe slight reddish tint ("red circle effect" / "red barrier"), but classic is more pink-white/magenta. |
| **Duration** | Persists until wall's HP depleted (set number of hits) or time expires. Pillar animation **loops continuously** while active. |
| **Behavior** | **Looping** gentle pulsing glow for duration. Disappears when broken or expired. |

### Stone Curse (Skill ID 16)
**Effect ID**: 23 (EF_STONECURSE)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Standard mage casting pose. Consumes 1 Red Gemstone. Short cast time. |
| **Projectile** | None — targets single enemy in range. |
| **Impact — Stage 1 (Stone forming)** | Dark gray/brown rocky texture begins to creep over target sprite, starting from feet and progressing upward. Character can still move during this brief phase but cannot attack or use skills. Grayish-brown overlay gradually engulfs sprite. |
| **Impact — Stage 2 (Fully petrified)** | Character sprite turns completely gray/dark stone-colored. Target appears as solid stone — entire sprite rendered in dark gray monochrome palette, losing all original colors. Completely immobilized. Being hit breaks the stone status. |
| **Ground indicator** | None. |
| **Color palette** | Dark gray, stone gray, brownish-gray. Petrification removes all color, replacing with uniform dark gray stone tint. |
| **Duration** | "Forming" stage is brief (few seconds). Petrified state lasts until hit or duration expires. Gray stone overlay **persistent** while active. |
| **Behavior** | Progressive overlay (stage 1), then persistent gray tint (stage 2). Removed on hit. |

### Sight (Skill ID 10)
**Effect ID**: 22 (EF_SIGHT)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Instant activation. Small **fireball** materializes near caster. |
| **Projectile** | None — fireball stays near caster. |
| **Impact** | If hidden enemy detected, they are revealed (no damage, no additional visual). |
| **Persistent effect** | Small, bright **orange-red fireball** continuously **orbits/spins around** caster at roughly waist height. Compact sphere of fire (much smaller than Fire Ball) circles character in smooth orbital path, completing full revolutions. Illuminates immediate area with warm orange glow. Detects hidden enemies within 7x7 cell area. |
| **Color palette** | Orange, red, yellow (fire tones). Small but bright. |
| **Duration** | 10 seconds or until hidden enemy detected. Orbiting fireball **loops continuously**. |
| **Behavior** | **Looping** orbit animation. Circles caster repeatedly until expires or triggers. |

### Increase SP Recovery
**Passive skill — NO visual effect.** Increases natural SP regeneration rate. Appears only in skill tree UI.

### Energy Coat (Skill ID 157)
**Effect ID**: 169 (EF_ENERGYCOAT)

| Aspect | Description |
|--------|-------------|
| **Casting effect** | Upon activation, blue-white aura/glow briefly pulses outward from caster. |
| **Projectile** | None — self-buff. |
| **Persistent effect** | Caster gains **translucent blue aura/glow** enveloping character sprite. "Spiritual energy coat" — slightly shimmering, semi-transparent blue-white layer surrounding character. Subtle — soft blue tint that gently pulses or shimmers. |
| **Ground indicator** | None. |
| **Color palette** | Blue, pale blue-white, cyan. Cool-toned spiritual energy color. |
| **Duration** | Up to 5 minutes or until SP reaches 0. Blue aura **loops continuously** (subtle shimmer/pulse). Each hit drains SP and aura may flicker. |
| **Behavior** | **Looping** subtle aura while buff active. Quest-obtained skill. |

---

## 1.6 Special Effects

### Warp Portal
**Effect IDs**: 316 (EF_READYPORTAL2 — Warp Portal animation)
**Texture files**: `ring_yellow.tga` (ground rings), `data\sprite\effect\particle1.act` / `particle1.spr` (swirling particles)
**Implementation**: Hardcoded in client (CRagEffect::Warp / CRagEffect::WarpZone / CRagEffect::WarpZone2)

| Aspect | Description |
|--------|-------------|
| **Ground ring** | **Yellow-tinted concentric ring** pattern projected flat onto ground. Using `ring_yellow.tga`, renders as luminous golden-yellow circular ring with multiple concentric bands radiating outward. |
| **Particle effect** | Above ground ring, **small bright dots/particles** swirl upward in spiraling pattern. Using `particle1.spr`, orbit in vertical helix/column above ring, creating distinctive swirling funnel/vortex. White-yellow with slight blue-purple tinting. |
| **Overall appearance** | Glowing yellow ring on ground + column of spiraling particles rising above. ~1 cell diameter. Walk into it to teleport. |
| **Color palette** | Yellow-gold (ground ring), white with blue-purple tints (swirling particles). Warm golden portal with mystical particle swirl. |
| **Duration** | Persists for set duration (skill level) or permanently for NPC portals. **Loops continuously**. |
| **Behavior** | **Looping** — ground ring pulses/glows gently, particles continuously spiral upward. |

---

## 1.7 Effect ID Quick Reference Table

| Skill | Effect ID | Key Texture Files | Primary Colors | Behavior |
|-------|-----------|-------------------|----------------|----------|
| Basic Attack | 0 (EF_HIT1) | — | White | One-shot |
| Bash | 1 (EF_HIT2) | — | White-yellow | One-shot |
| Endure | 11 (EF_ENDURE) | — | Yellow-gold | One-shot |
| Soul Strike | 15 (EF_SOULSTRIKE) | ring_blue.tga, lens1/2.tga, pok1/3.tga | White-purple-blue | Sequence |
| Magnum Break | 17 (EF_MAGNUMBREAK) | ring_yellow.tga | Orange-red-yellow | One-shot |
| Sight | 22 (EF_SIGHT) | — | Orange-red | Looping |
| Stone Curse | 23 (EF_STONECURSE) | — | Dark gray | Persistent |
| Fire Ball | 24 (EF_FIREBALL) | — | Orange-red-yellow | One-shot |
| Fire Wall | 25 (EF_FIREWALL) | — | Orange-red-yellow | Looping |
| Frost Diver | 27 (EF_FROSTDIVER) | ice.tga, pok1/3.tga, smoke.tga | Ice blue-white | One-shot + persistent |
| Lightning Bolt | 29 (EF_LIGHTBOLT) | — | White-yellow | Sequence |
| Thunderstorm | 30 (EF_THUNDERSTORM) | — | White-yellow | Sequence |
| Napalm Beat | 32 (EF_NAPALMBEAT) | — | Purple-violet | One-shot |
| Casting Circle | 60 (EF_LOCKON) | magic_target.tga | Blue-white-cyan | Looping |
| Provoke | 67 (EF_PROVOKE) | — | Red | One-shot |
| Energy Coat | 169 (EF_ENERGYCOAT) | — | Blue-white | Looping |
| First Aid | 309 (EF_FIRSTAID) | — | Green-white | One-shot |
| Safety Wall | 315 (EF_GLASSWALL2) | safetywall.str, safeline.bmp | Pink-magenta-white | Looping |
| Warp Portal | 316 (EF_READYPORTAL2) | ring_yellow.tga, particle1.spr/act | Yellow-gold | Looping |

---

# Part 2: Unreal Engine 5 VFX Technical Reference

## 2.1 Niagara Particle System Architecture

### Hierarchy
- **Niagara System** — Top-level container placed in world. Holds 1+ emitters. = complete spell effect
- **Niagara Emitter** — Defines how particles spawn, simulate, and render. Fire spell might have emitters for flames, sparks, smoke, glow
- **Modules** — Stackable behavior blocks within emitter (Spawn, Update, Event, Render)
- **Renderers** — Visual output: Sprite (camera-facing quads), Ribbon (connected trails), Mesh (3D geometry), Light (dynamic lights)

### C++ Integration

#### Build.cs Dependencies Required
```csharp
// Add to SabriMMO.Build.cs PublicDependencyModuleNames:
"Niagara", "NiagaraCore"
```

#### Required Headers
```cpp
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
```

#### Spawning — Fire-and-Forget (Spell Impacts)
```cpp
UNiagaraComponent* VFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    GetWorld(),              // WorldContextObject
    FireballVFX,             // UNiagaraSystem* SystemTemplate
    ImpactLocation,          // FVector Location
    FRotator::ZeroRotator,   // FRotator Rotation
    FVector::OneVector,      // FVector Scale
    true,                    // bool bAutoDestroy
    true,                    // bool bAutoActivate
    ENCPoolMethod::AutoRelease, // Pooling method
    true                     // bool bPreCullCheck
);
```

#### Spawning — Attached to Actor (Buffs/Auras)
```cpp
UNiagaraComponent* AuraVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
    HealingVFX,              // UNiagaraSystem*
    CharacterMesh,           // USceneComponent* AttachTo
    NAME_None,               // FName AttachPointName
    FVector::ZeroVector,     // FVector Location offset
    FRotator::ZeroRotator,   // FRotator Rotation
    FVector::OneVector,      // FVector Scale
    EAttachLocation::KeepRelativeOffset,
    true,                    // bool bAutoActivate
    ENCPoolMethod::AutoRelease
);
```

#### Runtime Parameter Control
```cpp
NiagaraComp->SetVariableFloat(FName("SpellIntensity"), 2.5f);
NiagaraComp->SetVariableVec3(FName("TargetPosition"), TargetLoc);
NiagaraComp->SetVariableLinearColor(FName("SpellColor"), FLinearColor::Red);
NiagaraComp->SetVariableInt(FName("ParticleCount"), 50);
```

This is extremely powerful: create ONE parameterized Niagara system for "generic projectile" and drive color, speed, size, particle count from C++ per skill.

### Can Niagara Systems Be Created Programmatically?
**Partially.** Cannot fully construct `UNiagaraSystem` from scratch at runtime. But you CAN:
1. Reference pre-made systems and control via User Parameters (RECOMMENDED)
2. Use `UNiagaraComponent` methods to activate/deactivate/reset at runtime
3. Use Niagara Data Channels (production-ready UE 5.5+) to feed external data
4. Use Custom Data Interfaces to pipe C++ data into particle simulation

**Practical approach**: Create 10-15 template Niagara systems, drive all visual variation through exposed parameters.

## 2.2 MMO Performance Best Practices

### Scalability/LOD
- **Scalability LOD**: Reduce spawn rates, disable expensive renderers/emitters by distance from camera
- **Effect Types**: Reusable asset defining global performance budgets — max active system counts, distance-based culling curves
- Key MMO settings:
  - Aggressive distance culling (spell effects beyond 5000 units get culled)
  - Scalability groups so Low-quality uses fewer particles
  - Spawn rate multipliers per quality level

### Component Pooling
Niagara built-in pooling via `ENCPoolMethod`:
- **`None`** — No pooling
- **`AutoRelease`** — RECOMMENDED for most spell effects. Returns to pool when done
- **`ManualRelease`** — You control return to pool
- **`ManualRelease_OnComplete`** — Returns when you release AND effect finishes

### Network Replication
**Critical: Niagara simulations are NOT replicated over network and do NOT run on dedicated servers.** VFX are cosmetic only.

For Socket.io architecture (your case):
1. Server sends skill event (e.g., `skill:cast_start { skillId, casterId, targetId, position }`)
2. Each client receives event and locally spawns appropriate Niagara system
3. No VFX data ever sent over network — only gameplay events that trigger them
4. **Already how your project works** — just add VFX spawning to client-side handlers

### GPU vs CPU Simulation
- **GPU emitters**: Best for 1000+ particles. Simulation runs entirely on GPU
- **CPU emitters**: Better for 1-100 particles. GPU overhead not worth it for small counts
- For MMO spell effects: mostly CPU simulation (50-200 particles per effect). GPU for environmental effects

### Lightweight Emitters (UE 5.4+)
- **Do not tick** — zero Game Thread cost when fully stateless
- **No compilation needed** — faster iteration
- **No per-particle memory** — reduced footprint
- **Fixed-function modules only**
- Best for: ambient dust, simple sparks, light flares, smoke. NOT for complex spells

## 2.3 Ground Decal Effects (Targeting Circles)

### Creating the Decal Material
```
Material Settings:
  Material Domain = Deferred Decal
  Decal Blend Mode = Translucent (or Emissive for glowing)

Procedural Ring (No Texture):
  1. RadialGradientExponential A: Radius=0.5, Density=100 (outer circle)
  2. RadialGradientExponential B: Radius=0.4, Density=100 (inner circle)
  3. Subtract B from A = ring shape
  4. Multiply by color parameter
  5. Connect to Emissive Color and Opacity
```

### Spawning from C++
```cpp
UDecalComponent* TargetCircle = UGameplayStatics::SpawnDecalAtLocation(
    GetWorld(),
    TargetCircleMaterial,          // UMaterialInterface*
    FVector(200.f, 200.f, 200.f),  // Decal size
    TargetLocation,                 // FVector
    FRotator(-90.f, 0.f, 0.f),     // Face downward
    5.0f                            // Lifespan seconds
);
```

### Animated Materials
- **Rotating ring**: `CustomRotator` node, `Time * RotationSpeed` as angle input
- **Pulsing glow**: `sine(Time * PulseSpeed)` oscillates emissive intensity
- **Filling circle (cast progress)**: `ScalarParameter` "FillProgress" (0-1), compare radial distance to FillProgress
- **Rune overlay**: Texture with rune pattern, UV rotation for spinning, additive blending layer

## 2.4 Free Resources Available

### Must-Download (Free)

| Resource | Contents | URL |
|----------|----------|-----|
| **Niagara Examples Pack** | 50+ production systems: explosions, fire, lightning, buffs, impacts, hit dissolves, markers, footsteps | https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600 |
| **Free Niagara Pack** | Community effects | https://www.fab.com/listings/cc942f1c-1766-446a-bee2-3baabf95a71e |
| **Free Realistic Explosions** | Explosion systems | https://www.fab.com/listings/a48b3fa2-2ebf-42c2-8892-fa20a1eff289 |
| **Free Niagara Magic Circle** | Ready-made magic circle | https://www.artstation.com/marketplace/p/dBJmq/ |
| **NiagaraUIRenderer** | Plugin: render Niagara in UMG | https://github.com/SourySK/NiagaraUIRenderer |

The Niagara Examples Pack alone could provide 60-70% of the VFX needed for basic spell effects. It includes player/weapon buffs, debuffs, lightning, explosions, and impact systems.

### Built-in UE5 Templates
When creating a new Niagara System in editor:
- Simple Sprite Burst
- Fountain (continuous spray)
- Directional Burst
- Mesh Burst
- GPU Sprite System
- Empty system (build from scratch)

### Paid Packs (Reference)

| Pack | Contents | Notes |
|------|----------|-------|
| **Fantasy RPG VFX Pack** | 87 systems: Fire, Ice, Arcane, Blood, Nature, Lightning. 7 AoE, 6 Aura, 9 Beam | Niagara + Cascade |
| **Niagara Magic Projectiles VFX** | 59 projectiles with cast/impact: Healing, Ice, Lightning | — |
| **Stylised Niagara Spell FX** | 25 VFX: ice, magic, fire, lightning, shadow, holy, healing | — |
| **AoE Magic Abilities Vol.2** | 58 AoE abilities: Fire, Ice, Water, Electricity, Heal | — |

### Learning Resources

| Resource | URL | What |
|----------|-----|------|
| CGHOW Niagara Guide | cghow.com | Complete beginner tutorial |
| CGHOW Spell FX | cghow.com/spell-fx-in-ue5-niagara-tutorial/ | Spell effect tutorials |
| CGHOW Heal Spell | cghow.com/heal-spell-in-ue5-niagara-tutorial/ | Healing circle |
| CGHOW Magic Ring Material | cghow.com/magical-ring-material-in-ue5-3-tutorial/ | Procedural magic circle |
| bp-shirai/UE-Learn-Niagara | GitHub | Step-by-step project |
| Limbicnation/NiagaraVFXGallery | GitHub | Examples and experiments |
| PacktPublishing/Build-Stunning-VFX-UE5 | GitHub | Book companion files |
| DarknessFX/Niagara_Study | GitHub | Sample collection |
| UE5 Magic VFX Gameplay | cgcircuit.com | Blueprints, Niagara & Houdini course |

---

# Part 3: AI Tools, MCP Servers, and Automated VFX Pipelines

## 3.1 MCP Servers for VFX / Niagara / Game Assets

### ChiR24/Unreal_mcp — The Most VFX-Capable MCP Server
- **GitHub**: https://github.com/ChiR24/Unreal_mcp
- **Tech stack**: TypeScript + C++ + Rust
- **36 MCP tools** covering: Blueprints, Materials, Textures, Static Meshes, Skeletal Meshes, Levels, Sounds, Particles, Niagara Systems, Behavior Trees
- **VFX-specific tools**:
  - `manage_effect` — Create/manipulate Niagara systems, emitters, modules, particles, GPU simulations, debug shapes
  - `manage_material_authoring` — Material creation, expression graphs, landscape layers
  - `manage_texture` — Texture creation, modification, compression settings
  - `manage_blueprint` — Graph editing for material and VFX blueprints
- **Requires**: C++ Automation Bridge plugin installed in UE5 project
- **Risk**: Untested with UE 5.7, documentation sparse, actual Niagara control depth unknown

### Flux159/mcp-game-asset-gen — General Asset Generation
- **GitHub**: https://github.com/Flux159/mcp-game-asset-gen
- **What**: Generates images, textures, 3D models using OpenAI DALL-E, Google Gemini, Fal.ai
- **Useful tools**: Image generation with transparent backgrounds (spell sprites), seamless/tileable textures (VFX materials), character sheets, pixel art
- **Does NOT** generate particle systems or Niagara assets directly — generates raw image/texture inputs

### MubarakHAlketbi/game-asset-mcp — 2D/3D Asset Generation
- **GitHub**: https://github.com/MubarakHAlketbi/game-asset-mcp
- **AI models**: `gokaygokay/Flux-2D-Game-Assets-LoRA` (50 inference steps), HuggingFace-based
- **Tools**: `generate_2d_asset`, `generate_3d_asset`
- **Useful for**: Generating spell effect sprite sheets from text like "fire explosion sprite sheet pixel art top-down"

### Other UE5 MCP Servers (Less VFX-Focused)
| Server | GitHub | Focus |
|--------|--------|-------|
| flopperam/unreal-engine-mcp | github.com/flopperam/unreal-engine-mcp | Scene construction (YOUR CURRENT ONE) |
| GenOrca/unreal-mcp | github.com/GenOrca/unreal-mcp | Asset management, scene manipulation |
| chongdashu/unreal-mcp | github.com/chongdashu/unreal-mcp | Natural language UE5 control |

## 3.2 AI-Generated VFX Tools

### Stable Diffusion Plugins for UE5 (In-Editor Texture Generation)

| Plugin | GitHub | Capability |
|--------|--------|-----------|
| **Unreal-StableDiffusionTools** | github.com/Mystfit/Unreal-StableDiffusionTools | Create animations, renders, textures using SD directly in UE5. Sequencer animated prompts, inpainting, seamless textures, upscaling |
| **ComfyTextures** | github.com/AlexanderDzhoganov/ComfyTextures | ComfyUI + UE5 integration for SDXL textures. "Create" mode for fast prototyping. Requires 16GB+ VRAM |
| **SdiffusionUE** | github.com/Ow1onp/SdiffusionUE | Sends requests to local SD API from UE5 |
| **Infinite Texture Generator** | fab.com | Generates batches of images and tileable textures from prompts within UE5 |

### Midjourney for VFX Textures
- Epic's official tutorial: [Creating VFX in UE5 from Midjourney-Generated Textures](https://dev.epicgames.com/community/learning/tutorials/z4R7/)
- Workflow: Generate stylized spell textures in Midjourney → clean up in Photoshop → import to UE5 → use as Niagara sprite/material textures
- Midjourney excels at "artistic, cinematic, concept art" images — excellent for fantasy spell effects

### Runway ML for VFX Video
- Gen-4.5 generates video with realistic physics (liquids, fire, particles)
- Use for: generating reference animations or short clips to convert to flipbook sprite sheets

### ZibraVDB — AI-Compressed Volumetric VFX (FREE for Indies)
- **URL**: https://www.zibra.ai/
- Compresses OpenVDB volumetric effects (smoke, fire, explosions) by 20-100x, renders real-time in UE5
- **Free for indie developers** (under $100K/year revenue) including commercial use
- Compatible with UE5.3+, custom AI compression, renders up to 3x faster than standard Heterogeneous Volumes
- Workflow: Houdini → export VDB → compress with ZibraVDB → drag & drop into UE5
- Great for dramatic fire columns, smoke explosions, magical fog

### AI Sprite Sheet Generators

| Tool | URL | Capability | Cost |
|------|-----|-----------|------|
| **AutoSprite** | autosprite.io | Upload sprite, pick moveset, export engine-ready sheets. UE5 support | Paid |
| **Ludo.ai Sprite** | ludo.ai/features/sprite-generator | Text-to-sprite animation | Free tier |
| **SEELE AI** | seeles.ai | Free AI sprite generator, pixel art + animated sheets from text. UE5 Paper2D Flipbook format | Free |
| **CGDream** | cgdream.ai | Free text-to-sprite-sheet | Free |
| **God Mode AI** | godmodeai.co | Fire spells, explosive impacts, weapon trails, slash effects from reference images | Paid |
| **OpenArt Sprite** | openart.ai/generator/sprite | Free text-to-sprite | Free |
| **Dzine AI** | dzine.ai/tools/ai-sprite-generator/ | AI-powered animated sprite creation | Paid |

## 3.3 Procedural VFX Generation Tools

### EmberGen by JangaFX — Gold Standard for Flipbook/Sprite VFX
- **URL**: https://jangafx.com/software/embergen
- Real-time GPU-based volumetric fluid simulation
- **Key capability**: Instantly generates game-ready, fully assembled flipbook sprite sheets
- Simulates fire, smoke, explosions in real-time → exports directly as flipbook textures for UE5
- **THE single most relevant tool** for RO-style effects (2D sprite sheets at modern quality)
- Udemy course: [Real-time VFX in EmberGen and UE5](https://www.udemy.com/course/learn-embergen-real-time-volumetric-fluid-simulation/)
- Pricing: Indie license available (~$20-40/month)

### Houdini — Procedural VFX Powerhouse
- Industry-standard procedural VFX tool, node-based, non-destructive
- [Houdini Niagara plugin](https://github.com/sideeffects/HoudiniNiagara) — open source, adds Houdini Data Interface to Niagara
- Course: [UE5 Magic VFX Gameplay](https://www.cgcircuit.com/tutorial/ue5-magic-vfx-gameplay-blueprints-niagara-houdini) — specifically covers spell effects
- Houdini Indie: ~$269/year (under $100K revenue)
- High learning curve but maximum flexibility

### PopcornFX — Alternative Particle System for UE5
- **URL**: https://www.popcornfx.com/
- Standalone real-time particle effects engine with UE5 plugin
- [Open source UE5 plugin](https://github.com/PopcornFX/UnrealEnginePopcornFXPlugin) on GitHub
- [Example effects](https://github.com/PopcornFX/UnrealEnginePopcornFXExamples) also on GitHub
- Free "Personal Licensing Edition" for non-commercial use
- Requires C++ project (you have one)
- **Note**: Alternative to Niagara, not a Niagara generator

### Video/GIF to Flipbook Converters

| Tool | What |
|------|------|
| ezgif.com | Online GIF-to-sprite-sheet converter. Upload animated GIF → PNG sprite sheet grid |
| GlueIT | Industry-standard flipbook assembly tool |
| EmberGen | Simulates AND directly exports flipbook textures (best option) |

### UE5 Flipbook Workflow
```
1. Import sprite sheet texture (disable sRGB for additive effects)
2. Create Material:
   - Blend Mode = Additive/Translucent
   - Add FlipBook node (set rows/columns to match grid)
   - Connect Time * PlaybackSpeed to FlipBook input
3. Use as Niagara Sprite Renderer material or billboard mesh material
```

---

# Part 4: RO VFX Recreation in Modern Engines

## 4.1 Existing RO Recreation Projects

### UnityRO (Most Feature-Complete)
- **GitHub**: https://github.com/guilhermelhr/unityro (186 stars)
- Unity-based RO client implementing both **STR-based effects rendering** and **Primitive-based effects rendering**
- Loads original RO game data at runtime (GRF files)
- Renders 3D maps with real-time lighting, dynamic shadows, baked lightmaps
- Displays sprite-based characters with full effect support
- Last release: v0.6.1 (October 2022)

### RagnarokRebuildTcp (Doddler's Rebuild)
- **GitHub**: https://github.com/Doddler/RagnarokRebuildTcp
- Full server + client in Unity (2022.3.62f2+)
- Uses Unity's Addressables for asset management
- Imports original RO sprites and models, custom ShaderLab shaders
- .NET 8 server, C# client

### roBrowser (Browser-Based)
- **GitHub**: https://github.com/vthibault/roBrowser
- Complete RO client in JavaScript/WebGL running in browser
- Implements **STR file loader and renderer** for `.str` effect files
- Developer noted: "Not all effects rendered, only ones using STR files listed in DB. Many effects hardcoded and don't rely on files."
- STR renderer has known limitations — parts of format not fully understood

### Other Implementations
| Project | Language/Engine | Year | Notes |
|---------|----------------|------|-------|
| Korangar | Rust/Vulkan | 2021 | Most technically modern |
| Dolori | C++/OpenGL | 2017 | — |
| RagnarokJS | JavaScript | — | Browser-based |

All listed on [Ragnarok Research Lab Community Projects](https://ragnarokresearchlab.github.io/community-projects/)

### UE5 Attempts
- TikTok-documented "Ragnarok Remastered" project showing weapon/skill/battle tests in UE5
- No substantial open-source UE5 RO recreation exists
- Epic Forums discussed 2D-in-3D approaches using **PaperZD plugin** for sprite management

## 4.2 RO Asset Extraction Tools

| Tool | GitHub | Capability |
|------|--------|-----------|
| **GRF Editor** | github.com/Tokeiburu/GRFEditor | Open-source, clients 2012-2024+, GRF v0x300 for >4GB. Extract, preview, validate, convert |
| **Act Editor** | github.com/Tokeiburu/ActEditor | Edit ACT/SPR sprite animation files |
| **zextractor** | github.com/zhad3/zextractor | CLI utility for GRF/GPF/THOR extraction |
| **RagLite Toolkit** | ragnarokresearchlab.github.io/tools/ | Analyzes RO binary formats, converts, visualizes sprites and 3D geometry |
| **ro-str-viewer** | github.com/skardach/ro-str-viewer | Java library for reading and rendering STR effect files |
| **rAthena STR Editor** | rathena.org board thread | Community STR editing discussion |

## 4.3 Recreating 2D Sprite Effects in UE5's 3D World

### Approach A: Niagara Sprite Particles with SubUV/Flipbook (RECOMMENDED)

1. **Create sprite sheet atlas** — all animation frames in a grid (e.g., 8x8 = 64 frames)
2. **Create Material**:
   - Material Domain: Surface
   - Blend Mode: Translucent or Masked
   - Texture Sample → Flipbook node
   - Configure rows/columns to match grid
   - Time node controls playback
3. **Create Niagara Emitter** — Simple Sprite Burst template
4. **Add Sub UV Animation module** in Particle Update:
   - Mode: Linear
   - Frames: match grid (64 for 8x8)
   - Start: 0, End: 63
5. **Sprite Renderer facing**:
   - `FaceCamera` — default billboard (best for bolt effects)
   - `Custom Facing Vector` — face specific direction via `Particles.SpriteFacing`
   - `FaceCameraDistanceBlend` — blends between billboard and velocity-aligned

### Approach B: Paper 2D Flipbooks
- UE5's Paper 2D Flipbook system for hand-drawn animation
- Import sprite frames → create Flipbook asset → render as PaperFlipbookComponent
- Add camera-facing material for billboard behavior
- Simpler but less performant than Niagara for many simultaneous effects

### Approach C: Flipbook Material on a Plane Mesh
- For ground effects (like Storm Gust ice area)
- Simple plane mesh or decal + material with Flipbook animation nodes
- No particle system needed — efficient for ground-based area effects

## 4.4 The Casting Circle in UE5 — Decal Approach

### Procedural Ring Material (No Texture Needed)
```
1. Two RadialGradientExponential nodes:
   - Outer: Radius=0.5, Density=100
   - Inner: Radius=0.4, Density=100
2. Subtract inner from outer = ring shape
3. Scalar parameters: OuterRadius (0.5), InnerRadius (0.4)
4. Multiply by BaseColor vector + Emissivity scalar
5. Output: Subtraction to Opacity, color product to Emissive
```

### Textured Approach
```
1. Import magic circle TGA (from AI generation or rAthena collection)
2. Material Domain = Deferred Decal, Decal Blend Mode = Emissive
3. Rotator node: Time * RotationSpeed → Texture UV rotation
4. Color tinting: ElementColor parameter multiplied into RGB
5. FadeAlpha parameter (0-1) for fade in/out control
```

### Dynamic Parameters from C++
Use Material Parameter Collections or Dynamic Material Instances:
- Color (element-specific)
- Opacity (fade in/out)
- Scale (grow/shrink)
- Rotation speed

## 4.5 Modern RO-Inspired Games and Their VFX

### Tree of Savior
- By Kim Hakkyu (creator of RO)
- 2D textured planes for character heads that change by camera angle
- 3D environments with 2D sprite-like characters
- Heavy particle effects — players requested `/effect` toggle (just like RO's `/effect`)
- VFX density was known performance issue

### Ragnarok X: Next Generation
- Released globally 2025, full 3D reimagining
- Fully 3D characters and environments (no sprites)
- Anime-style cel-shaded rendering
- Modern particle effects, maintains "feel" of RO but with volumetric 3D
- Available on Steam

### Ragnarok Online 2
- Entirely 3D with standard MMO particle effects
- Used Unreal Engine 3
- Lost much of RO1's distinctive visual character
- Many fans felt it lacked the charm of sprite-based effects

### Key Insight
The most successful RO-inspired games preserve the **contrast between 2D sprite characters and 3D environments**. VFX approaches:

| Approach | Used By | Best For |
|----------|---------|----------|
| Billboard sprite particles (flipbook) | UnityRO, original RO | Bolt effects, hit impacts, status auras |
| Ground decal with rotation | Original RO, UE5 projects | Casting circles, AoE indicators |
| Niagara particle systems | Modern UE5 games | Fire/ice/lightning environmental effects |
| Layered STR-style compositing | Original RO, roBrowser | Complex multi-layer effects like Meteor Storm |

---

# Part 5: Current Sabri_MMO Skill System State

## 5.1 All Defined Skills (86 First Class + 60+ Second Class)

### First Class Skills (ro_skill_data.js)

| Class | ID Range | Skills |
|-------|----------|--------|
| **Novice** | 1-3 | Basic Skill, First Aid, Play Dead |
| **Swordsman** | 100-106 | Sword Mastery, Two-Hand Sword Mastery, Increase HP Recovery, Bash, Provoke, Magnum Break, Endure |
| **Mage** | 200-212 | Cold Bolt, Fire Bolt, Lightning Bolt, Napalm Beat, Increase SP Recovery, Sight, Stone Curse, Fire Ball, Frost Diver, Fire Wall, Safety Wall, Thunderstorm |
| **Archer** | 300-305 | Owl's Eye, Vulture's Eye, Improve Concentration, Double Strafe, Arrow Shower, Arrow Crafting |
| **Acolyte** | 400-412 | Heal, Divine Protection, Blessing, Increase AGI, Decrease AGI, Cure, Angelus, Signum Crucis, Ruwach, Teleport, Warp Portal, Pneuma, Aqua Benedicta |
| **Thief** | 500-505 | Double Attack, Improve Dodge, Steal, Hiding, Envenom, Detoxify |
| **Merchant** | 600-607 | Enlarge Weight Limit, Discount, Overcharge, Mammonite, Pushcart, Vending, Item Appraisal, Change Cart |

### Second Class Skills (ro_skill_data_2nd.js)

| Class | ID Range | Count |
|-------|----------|-------|
| Knight | 700-709 | 10 |
| Wizard | 800-809 | 10 |
| Hunter | 900-907 | 8 |
| Priest | 1000-1007 | 8 |
| Assassin | 1100-1106 | 7 |
| Blacksmith | 1200-1205 | 6 |
| Crusader | 1300-1306 | 7 |
| Sage | 1400-1404 | 5 |
| Bard | 1500-1502 | 3 |
| Dancer | 1520-1522 | 3 |
| Monk | 1600-1605 | 6 |
| Rogue | 1700-1704 | 5 |
| Alchemist | 1800-1804 | 5 |

### Skill Data Structure (Server)
```javascript
{
  id: number,
  name: string,           // internal_name
  displayName: string,
  classId: string,        // novice|swordsman|mage|etc.
  maxLevel: number,
  type: string,           // active|passive|toggle
  targetType: string,     // none|self|single|ground|aoe
  element: string,        // neutral|fire|water|wind|earth|holy|dark|ghost|undead|poison
  range: number,
  description: string,
  icon: string,
  treeRow: number,
  treeCol: number,
  prerequisites: [{ skillId, level }],
  levels: [{
    level, spCost, castTime, afterCastDelay, cooldown, effectValue, duration
  }]
}
```

## 5.2 Implemented Server-Side Skills (15 total)

| Skill | ID | Target | Cast Time | Key Mechanic |
|-------|----|--------|-----------|-------------|
| First Aid | 2 | Self | 0 | +5 HP heal |
| Bash | 103 | Single | 0 | Physical damage + aggro |
| Provoke | 104 | Single | 0 | Debuff: -DEF%, +ATK% (4s) |
| Magnum Break | 105 | AoE | 0 | Fire AoE + knockback |
| Endure | 106 | Self | 0 | Buff: +MDEF, status immunity (10s) |
| Cold Bolt | 200 | Single | Level-based | Multi-hit ice (1-10 bolts) |
| Fire Bolt | 201 | Single | Level-based | Multi-hit fire (1-10 bolts) |
| Lightning Bolt | 202 | Single | Level-based | Multi-hit lightning (1-10 bolts) |
| Soul Strike | 210 | Single | 0 | Ghost multi-hit (5 hits), undead bonus |
| Napalm Beat | 203 | Single | 0 | Ghost AoE split damage |
| Fire Ball | 207 | Single | Level-based | Fire AoE (5x5 splash): center full, outer 75% |
| Thunderstorm | 212 | Ground | Level-based | Wind AoE multi-hit, 200ms delays |
| Frost Diver | 208 | Single | Level-based | Water damage + freeze (3-10s) |
| Stone Curse | 206 | Single | Level-based | Earth damage + petrify (20s) |
| Sight | 205 | Self | 0 | Reveals hidden (10s) |
| Fire Wall | 209 | Ground | Level-based | Multi-hit fire barrier + knockback |
| Safety Wall | 211 | Ground | Level-based | Blocks melee hits |

## 5.3 Client-Side Skill Execution Flow

### Key C++ Files
| File | Role |
|------|------|
| `UI/SkillTreeSubsystem.cpp/h` | Skill usage, targeting, cooldowns, hotbar |
| `UI/SSkillTargetingOverlay.cpp/h` | Crosshair cursor during targeting mode |
| `UI/CastBarSubsystem.cpp/h` | Cast time progress bar |
| `UI/DamageNumberSubsystem.cpp/h` | Floating damage numbers |

### Socket Events (Incoming)
- `skill:data` — Initial skill tree
- `skill:learned` — Skill leveled
- `skill:used` — Executed successfully (targetsHit, totalDamage for AoE)
- `skill:effect_damage` — Detailed damage (skillId, name, level, element, damage, isCritical, targetX/Y/Z)
- `skill:buff_applied` — Buff on (skillId, buffName, duration, effects)
- `skill:buff_removed` — Buff off (skillId, buffName, reason)
- `skill:cast_start` — Cast begins (casterId, skillId, actualCastTime, targetId)
- `skill:cast_interrupted_broadcast` — Cast interrupted
- `skill:cooldown_started` — Cooldown begins (skillId, cooldownMs)

### Targeting Modes (ESkillTargetingMode)
- `None` — Passive, self-cast
- `SingleTarget` — Click enemy to cast (Bash, Cold Bolt)
- `GroundTarget` — Click ground to place AoE (Fire Wall, Thunderstorm)

### Existing VFX Assets
| Asset | Location | Purpose |
|-------|----------|---------|
| `NS_Damage.uasset` | `Content/Variant_Combat/VFX/` | Floating damage numbers |
| `NS_Jump_Trail.uasset` | `Content/Variant_Platforming/VFX/` | Platforming only, not skill-related |
| Skill icons (20+) | `Content/SabriMMO/Assets/Skill_Icons/` | UI icons for hotbar/skill tree |

### Missing VFX (To Implement)
- No skill-specific impact particles
- No cast-time visual effects (spell glow, charging)
- No status effect visual indicators (frozen ice, stone petrify)
- No projectile trails or beams
- No knockback visual feedback
- No AoE radius preview during targeting
- No ground casting circle

---

# Part 6: Complete Source List

## RO Visual Effects Sources
1. [iRO Wiki — Cold Bolt](https://irowiki.org/wiki/Cold_Bolt)
2. [iRO Wiki — Fire Bolt](https://irowiki.org/wiki/Fire_Bolt)
3. [iRO Wiki — Lightning Bolt](https://irowiki.org/wiki/Lightning_Bolt)
4. [iRO Wiki — Soul Strike](https://irowiki.org/wiki/Soul_Strike)
5. [iRO Wiki — Napalm Beat](https://irowiki.org/wiki/Napalm_Beat)
6. [iRO Wiki — Fire Ball](https://irowiki.org/wiki/Fire_Ball)
7. [iRO Wiki — Fire Wall](https://irowiki.org/wiki/Fire_Wall)
8. [iRO Wiki — Frost Diver](https://irowiki.org/wiki/Frost_Diver)
9. [iRO Wiki — Thunderstorm](https://irowiki.org/wiki/Thunderstorm)
10. [iRO Wiki — Safety Wall](https://irowiki.org/wiki/Safety_Wall)
11. [iRO Wiki — Stone Curse](https://irowiki.org/wiki/Stone_Curse)
12. [iRO Wiki — Magnum Break](https://irowiki.org/wiki/Magnum_Break)
13. [iRO Wiki — Provoke](https://irowiki.org/wiki/Provoke)
14. [iRO Wiki — Endure](https://irowiki.org/wiki/Endure)
15. [iRO Wiki — Energy Coat](https://irowiki.org/wiki/Energy_Coat)
16. [iRO Wiki — Warp Portal](https://irowiki.org/wiki/Warp_Portal)
17. [iRO Wiki — Status Effects](https://irowiki.org/wiki/Status_Effects)
18. [iRO Wiki — Swordman](https://irowiki.org/wiki/Swordman)
19. [RateMyServer — Sight Skill](https://ratemyserver.net/index.php?page=skill_db&skid=10)
20. [RateMyServer — First Aid](https://ratemyserver.net/index.php?page=skill_db&skid=142)
21. [RateMyServer — Magician Skills](https://ratemyserver.net/index.php?page=skill_db&jid=2)
22. [RateMyServer — Swordman Skills](https://ratemyserver.net/index.php?page=skill_db&jid=1)
23. [RateMyServer Forum — Safety Wall Red Barrier](https://forum.ratemyserver.net/modification-sprite-and-tool-discussion/what-is-the-file-name-of-safety-wall's-red-barrier-in-grf/)
24. [rAthena — effect_list.txt](https://github.com/idathena/trunk/blob/master/doc/effect_list.txt)
25. [rAthena — Magic Target Collection](https://rathena.org/board/topic/75230-magic-target-collectionfree-download/)
26. [rAthena — Editing Warp Portal Sprite](https://rathena.org/board/topic/112195-editing-the-warp-portal-sprite/)
27. [rAthena — Spell Circle Discussion](https://rathena.org/board/topic/86998-how-to-change-spell-circle/)
28. [rAthena — STR Editor Discussion](https://rathena.org/board/topic/130296-a-more-friendly-str-editor/)
29. [Hercules Board — STR/TGA File Identification](https://board.herc.ws/topic/8427-identify-str-or-tga-filenames-of-some-effectskill/)
30. [The Spriters Resource — RO Frost Diver](https://www.spriters-resource.com/fullview/42206/)
31. [The Spriters Resource — RO Status Effects](https://www.spriters-resource.com/pc_computer/ragnarokonline/asset/42209/)
32. [The Spriters Resource — RO Main Page](https://www.spriters-resource.com/pc_computer/ragnarokonline/)
33. [Sprite Database — Ragnarok Online](https://spritedatabase.net/game/790)
34. [Ragnarok Research Lab — Fire Wall Mechanics](https://ragnarokresearchlab.github.io/game-mechanics/effects/fire-wall/)
35. [Ragnarok Research Lab — SPR Format](https://ragnarokresearchlab.github.io/file-formats/spr/)
36. [Ragnarok Research Lab — Community Projects](https://ragnarokresearchlab.github.io/community-projects/)
37. [RagnarokFileFormats Repository](https://github.com/rdw-archive/RagnarokFileFormats)
38. [Retro Refugees Forum — Classic Cast Circle](https://retrorefugees.forumotion.com/t27-classic-cast-circle)
39. [Ragnarok Battle Offline Wiki — Fire Ball](https://rbo.fandom.com/wiki/Fire_Ball)
40. [Ragnarok Battle Offline Wiki — Magnum Break](https://rbo.fandom.com/wiki/Magnum_Break)
41. [Project Alfheim Wiki — Status Effects](https://projectalfheim.net/wiki/index.php/Status_Effects)

## UE5 VFX Technical Sources
42. [Epic — Niagara Tutorials](https://dev.epicgames.com/documentation/en-us/unreal-engine/tutorials-for-niagara-effects-in-unreal-engine)
43. [Epic — Creating Visual Effects in Niagara](https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-visual-effects-in-niagara-for-unreal-engine)
44. [Epic — Niagara Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/quick-start-for-niagara-effects-in-unreal-engine)
45. [Epic — Niagara Scalability & Best Practices](https://dev.epicgames.com/documentation/en-us/unreal-engine/scalability-and-best-practices-for-niagara)
46. [Epic — Effect Types Performance Budgeting](https://dev.epicgames.com/documentation/en-us/unreal-engine/performance-budgeting-using-effect-types-in-niagara-for-unreal-engine)
47. [Epic — Niagara Lightweight Emitters](https://dev.epicgames.com/documentation/en-us/unreal-engine/niagara-lightweight-emitters-overview)
48. [Epic — UNiagaraFunctionLibrary API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraFunctionLibrary)
49. [Epic — SpawnSystemAtLocation API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraFunctionLibrary/SpawnSystemAtLocation)
50. [Epic Community — Using Niagara in C++](https://dev.epicgames.com/community/learning/tutorials/Gx5j/using-niagara-in-c)
51. [Epic Community — Optimizing Niagara](https://dev.epicgames.com/community/learning/tutorials/15PL/unreal-engine-optimizing-niagara-scalability-and-best-practices)
52. [Epic News — 50 Free Niagara Systems for UE 5.7](https://www.unrealengine.com/en-US/news/discover-over-50-free-niagara-systems-ready-to-use-in-unreal-engine-5-7)
53. [Niagara Examples Pack on Fab](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600)
54. [80.lv — Epic Shares 50+ Free Niagara Systems](https://80.lv/articles/epic-releases-over-50-free-niagara-systems-for-unreal-engine-5-7)
55. [Epic — Decal Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine)
56. [Epic — Gradient Material Functions](https://dev.epicgames.com/documentation/en-us/unreal-engine/gradient-material-functions-in-unreal-engine)
57. [Epic — Animating UV Coordinates](https://dev.epicgames.com/documentation/en-us/unreal-engine/animating-uv-coordinates-in-unreal-engine)
58. [Epic — GPU Sprite Niagara](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-create-a-gpu-sprite-effect-in-niagara-for-unreal-engine)
59. [Epic — Paper 2D Flipbooks](https://dev.epicgames.com/documentation/en-us/unreal-engine/paper-2d-flipbooks-in-unreal-engine)
60. [Epic Forums — AoE Target/Range Indicators](https://forums.unrealengine.com/t/aoe-target-range-indicators/425740)
61. [Epic Forums — AoE Spell Pre-Cast Indicator](https://forums.unrealengine.com/t/how-to-create-an-aoe-spell-pre-cast-indicator/136860)
62. [Epic Forums — 2D Characters in 3D World](https://forums.unrealengine.com/t/how-to-create-2d-characters-in-3d-world-like-ragnarok-online-boomer-shooter/1308742)
63. [Epic Forums — Multiplayer VFX](https://forums.unrealengine.com/t/do-niagara-vfx-work-in-multiplayer-games/1952254)
64. [Epic Forums — Multiplayer VFX Spawning](https://forums.unrealengine.com/t/right-way-to-spawn-niagara-system-in-multiplayer-im-looking-for-a-condition-to-filter-in-replication/2102046)
65. [CGHOW — Complete Niagara Beginner Guide](https://cghow.com/unreal-engine-niagara-a-complete-guide-for-beginners-ultimate-real-time-vfx-tutorial-for-game-devs-technical-artists/)
66. [CGHOW — Spell FX in UE5 Niagara](https://cghow.com/spell-fx-in-ue5-niagara-tutorial/)
67. [CGHOW — Heal Spell Tutorial](https://cghow.com/heal-spell-in-ue5-niagara-tutorial/)
68. [CGHOW — Magical Ring Material](https://cghow.com/magical-ring-material-in-ue5-3-tutorial/)
69. [CGHOW — AoE Impact FX](https://cghow.com/aoe-impact-fx-in-ue5-3-niagara-tutorial/)
70. [CGHOW — Spell Breaker](https://cghow.com/spell-breaker-in-ue5-niagara-tutorial-3/)
71. [CGHOW — LOD/Scalability Niagara](https://cghow.com/lod-scalability-in-ue5-niagara-tutorial/)
72. [ArtStation — Animated Materials UE5](https://www.artstation.com/blogs/jsabbott/e1BvL/animated-materials-ue5-tutorial)
73. [ArtStation — Free Niagara Magic Circle](https://www.artstation.com/marketplace/p/dBJmq/unreal-engine-niagara-magical-circle-download-free-file-ue4-ue5)
74. [Niagara Sprite Facing Tutorial](https://www.cyanhall.com/tutorial/4.niagara-sprite-facing/)
75. [Flipbook Animation in UE5](https://foro3d.com/en/2026/january/flipbook-animation-in-unreal-engine-5-for-dynamic-effects.html)
76. [Circular Ring Material Tutorial](https://unrealpossibilities.blogspot.com/2016/01/unreal-engine-tutorial-create-circular.html)
77. [Game Dev Tactics — Niagara Variables in C++](https://gdtactics.com/setting-a-variable-in-a-niagara-component-with-cpp-in-unreal-engine-5)
78. [Niagara Component Pooling API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraComponentPool)
79. [More VFX Academy — Niagara Optimization Guide](https://morevfxacademy.com/complete-guide-to-niagara-vfx-optimization-in-unreal-engine/)
80. [Epic Community — Effect Types](https://dev.epicgames.com/community/learning/knowledge-base/LJnb/unreal-engine-niagara-scalability-effect-types)
81. [Wayline — UE5 Niagara Advanced Techniques](https://www.wayline.io/blog/ue5-niagara-particle-systems-tutorial-advanced-techniques)
82. [Tom Looman — UE5 5.5 Performance Highlights](https://tomlooman.com/unreal-engine-5-5-performance-highlights/)
83. [Medium — Solo UE5 MMO Development Guide](https://medium.com/@Jamesroha/solo-ue5-mmo-development-guide-f70b9c46d8ac)
84. [GitHub — Niagara and C++ Reference](https://github.com/ylyking/LearningUnrealEngine-cp/blob/master/Niagara%20and%20C++.md)

## AI/Automation Sources
85. [ChiR24/Unreal_mcp — GitHub](https://github.com/ChiR24/Unreal_mcp)
86. [Flux159/mcp-game-asset-gen — GitHub](https://github.com/Flux159/mcp-game-asset-gen)
87. [MubarakHAlketbi/game-asset-mcp — GitHub](https://github.com/MubarakHAlketbi/game-asset-mcp)
88. [flopperam/unreal-engine-mcp — GitHub](https://github.com/flopperam/unreal-engine-mcp)
89. [GenOrca/unreal-mcp — GitHub](https://github.com/GenOrca/unreal-mcp)
90. [chongdashu/unreal-mcp — GitHub](https://github.com/chongdashu/unreal-mcp)
91. [Docker UE MCP Server](https://hub.docker.com/mcp/server/unreal-engine-mcp-server/overview)
92. [Mystfit/Unreal-StableDiffusionTools — GitHub](https://github.com/Mystfit/Unreal-StableDiffusionTools)
93. [AlexanderDzhoganov/ComfyTextures — GitHub](https://github.com/AlexanderDzhoganov/ComfyTextures)
94. [Ow1onp/SdiffusionUE — GitHub](https://github.com/Ow1onp/SdiffusionUE)
95. [Epic Tutorial — Creating VFX from Midjourney Textures](https://dev.epicgames.com/community/learning/tutorials/z4R7/creating-vfx-in-unreal-engine-5-from-midjourney-generated-textures)
96. [EmberGen by JangaFX](https://jangafx.com/software/embergen)
97. [ZibraVDB](https://www.zibra.ai/)
98. [ZibraVDB Free for Indies](https://www.cgchannel.com/2025/08/zibravdb-for-houdini-and-ue5-is-now-free-to-indie-artists/)
99. [ZibraVDB on Fab](https://www.fab.com/listings/23aef313-3c6a-40ea-810d-35de2ea5bca2)
100. [PopcornFX UE5 Plugin — GitHub](https://github.com/PopcornFX/UnrealEnginePopcornFXPlugin)
101. [PopcornFX Examples — GitHub](https://github.com/PopcornFX/UnrealEnginePopcornFXExamples)
102. [Houdini Niagara Plugin — GitHub](https://github.com/sideeffects/HoudiniNiagara)
103. [UE5 Magic VFX Gameplay Course](https://www.cgcircuit.com/tutorial/ue5-magic-vfx-gameplay-blueprints-niagara-houdini)
104. [Ludus AI](https://ludusengine.com/)
105. [Tripo AI Solo Dev Sprint](https://www.tripo3d.ai/blog/two-week-solo-game-dev)
106. [Runway AI for VFX](https://runwayml.com/ai-for-vfx)
107. [AutoSprite](https://www.autosprite.io/)
108. [Ludo.ai Sprite Generator](https://ludo.ai/features/sprite-generator)
109. [SEELE AI Sprite Generator](https://www.seeles.ai/features/tools/sprite.html)
110. [CGDream Sprite Sheet Generator](https://cgdream.ai/features/ai-sprite-sheet-generator)
111. [God Mode AI](https://www.godmodeai.co)
112. [OpenArt Sprite Generator](https://openart.ai/generator/sprite)
113. [Dzine AI Sprite Generator](https://www.dzine.ai/tools/ai-sprite-generator/)
114. [ezgif.com Sprite Sheet Converter](https://ezgif.com/gif-to-sprite)
115. [Infinite Texture Generator on Fab](https://www.fab.com/listings/5a1ceba7-23eb-4301-af24-8dc4782e95f0)
116. [Ubisoft CHORD Model ComfyUI PBR](https://blog.comfy.org/p/ubisoft-open-sources-the-chord-model)

## Community/Recreation Sources
117. [roBrowser — GitHub](https://github.com/vthibault/roBrowser)
118. [roBrowser Effects Blog](https://www.robrowser.com/blog/welcome-effects)
119. [UnityRO — GitHub](https://github.com/guilhermelhr/unityro)
120. [RagnarokRebuildTcp — GitHub](https://github.com/Doddler/RagnarokRebuildTcp)
121. [GRF Editor — GitHub](https://github.com/Tokeiburu/GRFEditor)
122. [Act Editor — GitHub](https://github.com/Tokeiburu/ActEditor)
123. [zextractor — GitHub](https://github.com/zhad3/zextractor)
124. [ro-str-viewer — GitHub](https://github.com/skardach/ro-str-viewer)
125. [Ragnarok Research Lab — Main](https://ragnarokresearchlab.github.io/)
126. [Tree of Savior Forum — Effect Discussion](https://forum.treeofsavior.com/t/effect-like-ragnarok-online/291572)
127. [Ragnarok X: Next Generation — Steam](https://store.steampowered.com/app/4007140/Ragnarok_X_Next_Generation/)

## Free Resource Downloads
128. [Niagara Examples Pack — Fab](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600)
129. [Free Niagara Pack — Fab](https://www.fab.com/listings/cc942f1c-1766-446a-bee2-3baabf95a71e)
130. [Free Realistic Explosions — Fab](https://www.fab.com/listings/a48b3fa2-2ebf-42c2-8892-fa20a1eff289)
131. [NiagaraUIRenderer — GitHub](https://github.com/SourySK/NiagaraUIRenderer)
132. [bp-shirai/UE-Learn-Niagara — GitHub](https://github.com/bp-shirai/UE-Learn-Niagara)
133. [Limbicnation/NiagaraVFXGallery — GitHub](https://github.com/Limbicnation/NiagaraVFXGallery)
134. [PacktPublishing/Build-Stunning-VFX-UE5 — GitHub](https://github.com/PacktPublishing/Build-Stunning-Real-time-VFX-with-Unreal-Engine-5)
135. [DarknessFX/Niagara_Study — GitHub](https://github.com/DarknessFX/Niagara_Study)
136. [Fab Free Content Page](https://www.unrealengine.com/en-US/fabfreecontent)
137. [Fab Limited-Time Free](https://www.fab.com/limited-time-free)
138. [Niagara VFX Fundamentals Tutorial (CGChannel)](https://www.cgchannel.com/2024/07/tutorial-real-time-vfx-fundamentals-for-unreal-engine-5/)

---

**Total Documented Sources**: 138
**Last Updated**: 2026-03-05
