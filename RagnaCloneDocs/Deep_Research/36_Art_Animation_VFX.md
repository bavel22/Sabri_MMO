# Art, Animation & VFX -- Deep Research

**Document Version**: 1.0
**Date**: 2026-03-22
**Scope**: Comprehensive analysis of Ragnarok Online Classic (pre-Renewal) visual style, animation systems, VFX patterns, UI art direction, and environment art. Includes UE5 implementation guidance for replicating the RO aesthetic in 3D.

---

## Table of Contents

1. [Visual Style Overview](#1-visual-style-overview)
2. [Character Visuals](#2-character-visuals)
3. [Monster Visuals](#3-monster-visuals)
4. [Skill VFX](#4-skill-vfx)
5. [UI Art Style](#5-ui-art-style)
6. [Environment Art](#6-environment-art)
7. [Implementation for UE5](#7-implementation-for-ue5)
8. [Gap Analysis](#8-gap-analysis)

---

## 1. Visual Style Overview

### 1.1 2D Sprite Characters on 3D Environments

Ragnarok Online's defining visual innovation is its hybrid rendering approach: **hand-painted 2D sprite characters** rendered over **textured 3D polygonal environments**. This "2.5D" technique was pioneered by Gravity Corp. in 2002 and remains one of the most recognizable visual identities in MMO history.

**Technical composition:**
- **Characters, monsters, and NPCs** are 2D sprites composed of multiple layered sprite sheets (body, head, hair, headgear, weapon, shield, garment). Each layer is independently rendered and composited in real-time.
- **The world geometry** (terrain, buildings, props) is fully 3D, built with polygonal meshes and textured with hand-painted diffuse maps. Environments use a fixed directional light source with pre-baked lightmaps and minimal real-time lighting.
- **Sprite billboarding**: Character sprites always face the camera, rotating only in the Y axis (vertical). They do not tilt or skew with camera angle changes.
- **Sprite compositing order**: Body sprite (base) -> outfit layer -> hair layer -> headgear layers (upper, mid, lower) -> weapon/shield -> garment/cape -> status effect overlays. Each layer uses alpha transparency for clean compositing.

**Why it works:**
The 2D-on-3D approach creates a distinctive "storybook diorama" feel. Characters appear as animated illustrations placed into a miniature world. The contrast between the flat, hand-crafted sprite art and the dimensional environment generates a visual warmth that photorealistic 3D MMOs struggle to replicate. The sprites' limited animation frames (typically 5-8 per action per direction) give movement a charming, stop-motion quality rather than the uncanny valley of interpolated 3D animation.

**Sprite creation method:**
RO sprites are NOT pixel art in the traditional sense (no dithering, no strict limited palette). They are **painted sprites** -- created in tools like Photoshop at relatively large resolution, using smooth gradients, anti-aliased edges, and painterly shading. The sprites were then scaled down and palettized for the game engine. The art has been described as "painted sprites" rather than "pixel art" because the artists used painting techniques rather than pixel-by-pixel placement.

### 1.2 Isometric Perspective

RO Classic uses a **fixed isometric camera** (technically dimetric/trimetric) with the following characteristics:

| Property | Value |
|----------|-------|
| Camera type | Orthographic projection (classic) / Perspective with limited FOV (later clients) |
| Vertical angle | Approximately 45-55 degrees from horizontal |
| Horizontal rotation | Player-rotatable in 45-degree increments (8 positions) |
| Zoom | 5 discrete zoom levels |
| Camera follow | Centered on player character with smooth follow |
| Tilt | Fixed -- cannot be adjusted by the player |

**Isometric grid alignment:**
- The world is built on a cell-based grid system where each cell is approximately 5x5 game units.
- Buildings and terrain conform to this grid, giving maps a clean, readable layout.
- Sprite positions snap to grid cells for movement, though rendering allows sub-cell interpolation for smooth animation.

**Visual impact of isometric perspective:**
- All environment art is designed to read correctly from the fixed camera angle. Building facades face the camera, interiors are "dollhouse" cutaway style.
- Depth perception relies on sprite scaling (smaller = farther) and ground plane positioning rather than atmospheric perspective.
- The fixed angle means artists can optimize every surface for that single viewpoint, resulting in higher visual quality per polygon than a free-camera system.

### 1.3 Anime / Chibi Art Direction

RO's visual style derives directly from **Lee Myung-jin's manhwa "Ragnarok"**, originally serialized in 1998. The manhwa featured a Korean comic art style with Western fantasy influences, and Gravity Corp. adapted this into the game's sprite-based aesthetic.

**Key art direction pillars:**

| Aspect | Description |
|--------|-------------|
| **Proportions** | Chibi-adjacent: approximately 1:4 to 1:5 head-to-body ratio. Heads are large and round, bodies are compact. This is shorter than standard anime (1:6-1:7) but taller than true chibi/super-deformed (1:2-1:3). |
| **Eyes** | Large, expressive, occupying approximately 25-30% of face height. Simple but readable at sprite scale (no iris detail, just color blocks with highlight). |
| **Facial features** | Minimal -- small dot or line nose, simple mouth (usually just a line or small curve). Expressions conveyed through emote bubbles rather than facial animation. |
| **Body type** | Uniformly lean/athletic regardless of class. Even "heavy" classes like Blacksmith or Knight are not bulky. Gender dimorphism exists but is moderate. |
| **Hands** | Slightly oversized relative to body -- critical for weapon readability at sprite scale. |
| **Feet** | Slightly oversized for visual grounding, preventing the "floating" appearance common in chibi styles. |

**Color philosophy:**
- **Saturated but not neon**: Colors are rich and warm, with saturation levels around 55-75% in most cases. Pure/neon saturated colors are avoided.
- **Warm base palette**: The overall color temperature leans warm, with ochre, cream, soft greens, and sky blues dominating town and field environments.
- **Class-identifying colors**: Each job class has a distinctive color identity visible even at small sprite sizes:

| Class | Primary Color Identity | Secondary |
|-------|----------------------|-----------|
| Novice | Tan/beige tunic | Brown boots |
| Swordsman | Blue armor, red accents | Silver trim |
| Mage | Purple/blue robes | Gold trim |
| Archer | Green/brown leather | Tan accents |
| Acolyte | White robes | Blue sash |
| Merchant | Brown apron | Gold pouches |
| Thief | Dark purple/black | Red scarf |
| Knight | Dark blue plate | Red cape |
| Wizard | Dark purple robes | Gold stars |
| Hunter | Brown/green ranger | Falcon companion |
| Priest | White/gold vestments | Holy blue accents |
| Assassin | Dark gray/black suit | Red accents |
| Crusader | White/silver plate | Blue cross motifs |
| Monk | Orange/brown gi | Bare arms |
| Sage | Blue academic robes | Brown book |
| Rogue | Purple/dark outfit | Daggers |
| Blacksmith | Brown leather apron | Hammer, goggles |
| Alchemist | White lab coat | Potion belt |
| Bard | Red/gold minstrel | Instrument |
| Dancer | Purple/pink | Performance outfit |

**Original artist contributions:**
- **Lee Myung-jin**: Created the original manhwa character designs and base art direction. The basic drawings of the early RO classes were done by Lee Myung-jin.
- **SR (illustrator)**: Handled the design and coloring of specific classes for the game adaptation, translating manhwa art into sprite-compatible designs.
- **Studio DTDS**: Lee's studio, which assisted with later art production. Their influence on style is noticeable in inking and shading differences across content eras.

---

## 2. Character Visuals

### 2.1 Sprite Animation Frames (Idle, Walk, Attack, Cast, Sit, Dead)

RO Classic characters use a sprite-based animation system stored in paired **SPR** (sprite sheet) and **ACT** (animation metadata) binary files. The SPR file contains all individual sprite images, while the ACT file defines which sprites to display, for how long, and in what order for each action.

**File format details:**
- **SPR files**: Spritesheet content with indexed-color (palette-based) sprites. Header encodes version in reverse (e.g., `53 50 01 02` = version 2.1). Contains all unique sprite images for the entity.
- **ACT files**: Animation metadata defining frame sequences, timings, anchor points, and sound events. Header `41 43 05 02` = version 2.5. Defines what images to display when and for how long for any given action.

**Direction system:**
- Player characters are rendered in **8 directions** (N, NE, E, SE, S, SW, W, NW), with each animation action having a complete frame set for all 8 directions.
- Monsters typically have **4-5 directions** (with horizontal mirroring producing the remaining directions from fewer unique sprites).
- NPCs may have as few as **2 directions** (front-facing and back-facing, with horizontal mirroring).

**Frame counts per action (typical player character):**

| Action | Unique Sprites | Frames per Direction | Directions | Total Frames | Loop? |
|--------|---------------|---------------------|-----------|-------------|-------|
| **Idle/Standing** | 5 | 3-5 | 8 | ~24-40 | Yes |
| **Walking** | 40 | 8 | 8 | ~64 | Yes |
| **Sitting** | 5 | 1-3 | 8 | ~8-24 | Yes (hold) |
| **Picking Up Item** | 4 | 2-4 | 8 | ~16-32 | No |
| **Standby/Ready** | 12 | 3-6 | 8 | ~24-48 | Yes |
| **Attack (1H)** | 8 | 4-6 | 8 | ~32-48 | No |
| **Attack (Bow)** | 16 | 4-8 | 8 | ~32-64 | No |
| **Attack (Knife/Dagger)** | 12 | 4-6 | 8 | ~32-48 | No |
| **Receiving Damage** | 2 | 2 | 8 | ~16 | No |
| **Dead** | 2 | 2 | 1 (down) | ~2 | No (hold final) |
| **Casting Spell** | 10 | 3-5 | 8 | ~24-40 | Yes (during cast) |

**Animation framerate:**
- Maximum 8 frames per one movement cycle of a character in one direction.
- The game renders at variable framerate (typically 30-60 fps) but sprite frames advance at a fixed interval determined by the ACT file's per-frame delay values.
- ASPD (Attack Speed) directly modifies the playback speed of attack animations. A character with 190 ASPD plays attack frames approximately twice as fast as one with 150 ASPD.

**Key animation behaviors:**
- **Idle**: Subtle breathing/weight shift cycle. The character gently sways or shifts weight between feet.
- **Walking**: Smooth 8-frame cycle per direction. Arms swing naturally, slight body bob.
- **Sitting**: Character sits on the ground cross-legged (male) or side-sitting (female). Only standing and sitting poses support user-initiated direction changes.
- **Attack**: Weapon-type dependent. 1H weapons have a quick slash, 2H weapons have a wider swing arc, bows have a draw-and-release. Attack animation speed is directly tied to ASPD from the server.
- **Casting**: Arms raised, body slightly tilted back, sparkle effects around hands. A magic circle appears beneath the caster's feet (separate ground effect sprite).
- **Hit reaction**: Brief flinch/knockback frame. Very short duration (~0.3s).
- **Death**: Character collapses to the ground. The final frame is held until respawn. Death sprites are direction-independent (always the same fallen pose).

### 2.2 Per-Class Sprite Variations

Each job class in RO has a completely unique sprite set. Job changes result in a total visual transformation -- the character's entire outfit changes.

**Sprite progression philosophy:**
From first job to second job to transcendent class, there is a deliberate visual progression:
- **First job**: Simple, functional clothing appropriate to the class archetype. Clean silhouettes, moderate detail.
- **Second job**: More elaborate, specialized outfits with class-specific accessories. Higher detail, more armor/ornamentation.
- **Transcendent (rebirth)**: Enhanced versions of second job sprites with upgraded detail, metallic accents, glowing trim elements, and occasional aura effects. The silhouette remains similar to the second job but with clear visual "upgrade" signals.

**Specific class progression examples:**
- **Swordsman -> Knight -> Lord Knight**: The armor becomes progressively more ornate. The skirt/tabard gets progressively shorter, the body type appears taller and more adult-proportioned, and the armor turns more elaborate with each tier.
- **Mage -> Wizard -> High Wizard**: Robes become longer and more decorated. Hat becomes more prominent. Color shifts from light purple/blue to deeper, darker purple with arcane rune patterns.
- **Acolyte -> Priest -> High Priest**: Religious vestments become more elaborate. White robes gain gold embroidery. High Priest sprites include subtle halo/glow overlay effects.

**Transcendent class visual differentiation:**
High First Classes (e.g., High Novice, High Swordsman) are distinguished from their non-transcendent counterparts primarily by sprite **discoloration** -- altered color palettes rather than completely new sprites. For example:
- High Novice females wear blue clothing (vs. standard tan)
- High Novice males wear brown clothing (vs. standard tan)

Second transcendent classes (Lord Knight, High Wizard, etc.) have fully unique sprites with visibly upgraded detail.

**Total sprite sets in RO Classic:**

| Category | Classes | Sets per Gender | Total Unique Sets |
|----------|---------|----------------|------------------|
| Novice | 1 | 2 (M/F) | 2 |
| 1st Job | 6 | 12 | 12 |
| 2nd Job | 13 (incl. Bard/Dancer split) | 26 | 26 |
| Transcendent | 13 | 26 | 26 |
| **Total** | **33** | **66** | **66** |

### 2.3 Hair Styles (Number per Gender, Visual Reference)

**Classic RO hair system:**
- **Styles 1-19**: Available in character creation and at the Stylist NPC. These are the official kRO (Korean Ragnarok Online) classic hairstyles.
- **Extended styles 20-28+**: Added in later patches and available on some servers. The default kRO package includes up to 31 hairstyles.
- **Private server extensions**: Many servers extend to 90-100+ custom hairstyles per gender.

**For classic pre-Renewal scope, the canonical count is 19 styles per gender (styles 1-19).**

| Style ID | Male Description | Female Description |
|----------|-----------------|-------------------|
| 1 | Short spiky | Short bob |
| 2 | Medium parted | Shoulder-length straight |
| 3 | Slicked back | Twin tails / pigtails |
| 4 | Long tied back | Long flowing |
| 5 | Mohawk-adjacent | Side ponytail |
| 6 | Bowl cut | Braided crown |
| 7 | Short messy / tousled | Short pixie |
| 8 | Long wild | Long wavy |
| 9 | Bandana-covered | Bun with loose strands |
| 10 | Crew cut | Bob with bangs |
| 11 | Samurai topknot | Hime cut (straight, long, even bangs) |
| 12 | Spiked up | Short twintails |
| 13 | Shaggy medium | Side braid |
| 14 | Pompadour | Curly shoulder-length |
| 15 | Ponytail (low) | Long straight with ribbon |
| 16 | Buzz cut | Messy short |
| 17 | Long straight | Odango (buns + tails) |
| 18 | Wild spikes | French braid |
| 19 | Afro / volume | Drill curls |

**Hair sprite structure:**
- Hair is a separate sprite layer composited on top of the head sprite.
- Each hair style has sprites for all 8 directions and all animation actions.
- Hair sprites include built-in shading (highlight and shadow areas painted into the sprite) that is recolored via palette swapping.
- Some hairstyles have minimal physics simulation (long hair, ponytails) expressed through alternate sprites for movement frames -- not real-time physics, but pre-drawn "wind-blown" variants.

### 2.4 Hair Colors (Palette)

**Classic RO has 9 hair color palettes (indices 0-8).**

Each color is applied via **palette swapping** -- the hair sprite uses indexed color, and the game engine swaps the palette to change the color. This is extremely efficient (no texture re-rendering needed).

| Color ID | Name | Approximate sRGB Hex | Visual Description |
|----------|------|---------------------|-------------------|
| 0 | Black | #1A1A2E | Near-black with very subtle blue undertone |
| 1 | Red-Brown | #8B4513 | Warm chestnut/saddle brown |
| 2 | Sandy Blonde | #D4A574 | Warm golden blonde |
| 3 | Chestnut | #6B3A2A | Dark reddish-brown |
| 4 | Gray | #9E9E9E | Medium silver-gray |
| 5 | Purple | #6A0DAD | Vivid medium purple |
| 6 | Orange | #E06020 | Warm burnt orange |
| 7 | Green | #2E8B57 | Forest/sea green |
| 8 | Blue | #2060C0 | Medium royal blue |

**Extended palette notes:**
- Some servers offer 297+ hair color palettes via custom palette files.
- The palette system allows three color values per palette entry: **Base Color**, **Highlight**, and **Shadow**, giving each hair color a painted quality with proper light/dark variation.
- Hair color is selected at character creation (numerical input 0-8) and can be changed at Stylist NPCs in-game.

### 2.5 Headgear Display (Upper / Mid / Lower Visual Slots)

RO's headgear system is the primary means of visual character customization. Three independent display slots allow players to wear up to three headgear items simultaneously:

| Slot | Position | Socket Location | Examples | Typical Poly/Sprite Count |
|------|----------|----------------|----------|--------------------------|
| **Upper (Top)** | Top of head | Above hair sprite | Helmets, hats, tiaras, horns, angel wings, animal ears, crowns | Largest -- full head coverage items |
| **Middle (Mid)** | Eye level | Over face/eyes | Glasses, monocles, sunglasses, masks (eye area), goggles | Medium -- partial face items |
| **Lower (Low)** | Mouth level | Lower face | Scarves, pipes, lollipops, masks (mouth area), beards, cigarettes | Small -- mouth/chin items |

**Headgear sprite behavior:**
- Each headgear is a separate sprite layer composited over the character's head area.
- Headgear sprites are anchored to the head's anchor point in the ACT file, so they move with the head during animation.
- Some upper headgear items partially or fully obscure the hair sprite (helmets), while others sit on top of it (hats, ribbons).
- Headgear sprites exist for all 8 directions, matching the character's facing.
- A single headgear can occupy multiple slots (e.g., a full-face gas mask occupies both Upper and Middle slots).

**Iconic headgear items (most recognized):**

| Item | Slot | Visual Description |
|------|------|--------------------|
| Angel Wing | Upper | Small white feathered wings on sides of head |
| Deviruchi Hat | Upper | Small devil creature sitting on head |
| Poring Hat | Upper | Pink Poring blob sitting on head |
| Cat Ear Band | Upper | Simple cat ears on a headband |
| Bunny Band | Upper | Tall rabbit ears on headband |
| Sunglasses | Middle | Classic dark aviator sunglasses |
| Opera Mask | Middle | Half-face decorative mask |
| Gangster Mask | Lower | Bandana/scarf covering mouth |
| Pipe | Lower | Smoking pipe in mouth |
| Lollipop | Lower | Candy on stick in mouth |
| Majestic Goat | Upper | Large curved ram horns |
| Crown | Upper | Royal golden crown |
| Beret | Upper | Military-style flat cap |
| Sakkat | Upper | Wide conical Asian hat |
| Wizard Hat | Upper | Tall pointed magic hat |

**Production scale:**
RO Classic has over 1,000 unique headgear items with visual sprites. This represents the single largest individual art asset category in the game.

### 2.6 Equipment Visual Changes

Beyond headgear, equipment visibility in RO Classic is limited:

| Equipment Slot | Visible on Character? | How It Displays |
|---------------|----------------------|-----------------|
| **Weapon (Right Hand)** | Yes | Separate weapon sprite layer, weapon-type specific. Changes based on equipped weapon. |
| **Shield (Left Hand)** | Yes | Separate shield sprite layer. Several distinct shield appearances. |
| **Garment/Cape** | Yes (limited) | Some garments (capes, mantles, wings) display as back-mounted sprites. Most garments are invisible. |
| **Armor** | No (outfit change only at class change) | Armor does NOT change the character's appearance in classic RO. Only job class determines outfit sprite. |
| **Footgear** | No | Part of the class outfit sprite, not individually visible. |
| **Accessories** | No | Completely invisible on character. |

**Weapon sprite variations:**
- Each weapon type (sword, dagger, staff, bow, mace, axe, spear, katar, knuckle, book, instrument, whip) has a distinct sprite shape.
- Within a weapon type, there are typically 3-5 visual variations representing different tiers of weapon (e.g., a simple iron sword vs. an ornate flamberge).
- Weapon sprites appear in the character's hand and animate with the attack action. Two-handed weapons appear larger.

### 2.7 Mount Visuals (Peco Peco)

**Peco Peco mounting** is a class-specific visual feature for Knights and Crusaders:

| Class | Mount Name | Visual Description |
|-------|------------|-------------------|
| **Knight / Lord Knight** | Peco Peco | Large yellow/orange riding bird (chocobo-like). Armored with plate barding matching the Knight's armor color scheme. |
| **Crusader / Paladin** | Grand Peco (Swift Peco) | Similar bird but with distinct armor design -- white/silver barding with holy cross motifs. Crusader and Paladin mounts were updated to have a unique sprite distinct from the Knight's version. |

**Mount sprite behavior:**
- When mounted, the character sprite changes entirely to a "mounted" version where the rider sits atop the bird.
- The Peco Peco has its own idle, walk, attack, and damage animation sets.
- Walking animation shows the bird's legs moving with a distinctive waddle/gallop.
- Attack animations show the rider swinging weapons from the mounted position.
- The mounted sprite set is larger than the unmounted character sprite, representing the combined size of rider + bird.
- Movement speed is visually faster (36% speed increase in-game).
- Peco Peco sprites include the character's outfit, so there are separate mounted sprite sets per class.

**Other mounts (post-classic):**
In Renewal and later content, additional mounts were added (Dragon for Rune Knight, Gryphon for Royal Guard, wolf mounts, etc.), but these are outside the classic pre-Renewal scope.

---

## 3. Monster Visuals

### 3.1 Sprite Art Style

RO monster sprites follow the same painted sprite technique as player characters but with important stylistic distinctions:

**Design philosophy:**
1. **Preserve silhouette readability**: Every monster must be instantly recognizable by silhouette alone, even at small sprite sizes in a crowded field.
2. **Maintain color identity**: Each monster has a distinctive, recognizable color palette. Poring is THAT specific pink (#FF88AA). Drops is THAT specific orange. Color variants of the same base monster (e.g., Poring/Drops/Poporing/Marin) share the same shape but have clearly different colors.
3. **"Cute but dangerous" duality**: Even aggressive monsters maintain a cartoon charm. Orc Warriors are brutish but not grotesque. Skeletons are spooky but stylized, not realistic. The art direction avoids grimdark or horror-realistic aesthetics.
4. **Eyes are character**: Most RO monsters have distinctive, expressive eye designs that convey personality. Porings have simple dot eyes with a cheerful expression. Baphomet has menacing glowing red eyes. The eyes are typically the first thing a player notices.
5. **Scale relationships tell gameplay stories**: Small monsters (Poring, Lunatic) are clearly non-threatening by scale. Large monsters (Golem, Minorous) read as dangerous. MVPs (Baphomet, Orc Lord) are dramatically oversized, filling a significant portion of the screen.

**Sprite rendering characteristics:**
- Monster sprites use the same painted technique as player sprites: smooth gradients, anti-aliased edges, painted shading.
- Sprites tend to have bold outlines (1-2 pixel dark outline around the form) for readability against varied backgrounds.
- Shading follows a consistent top-left light source across all sprites.
- Color palettes per monster typically use 3-5 main colors with 2-3 shade levels each.

### 3.2 Animation Types (Idle, Move, Attack, Damage, Death)

Monsters have a simpler animation set than player characters:

| Action | Typical Frame Count | Directions | Behavior |
|--------|-------------------|-----------|----------|
| **Idle** | 3-6 frames | 4-5 (mirrored to 8) | The signature animation -- defines the monster's personality. Poring bounces. Lunatic twitches ears. Skeleton shifts weight. |
| **Move/Walk** | 4-8 frames | 4-5 (mirrored) | Movement cycle. Speed varies by monster type. Flying monsters (Chonchon, Condor) have wing-flap cycles. |
| **Attack** | 3-8 frames | 4-5 (mirrored) | Primary attack animation. Melee monsters lunge/swing. Ranged monsters (Archer Skeleton) have draw-and-shoot sequences. |
| **Receive Damage (Hit)** | 1-2 frames | 1-4 | Brief flinch/flash. Very fast (~0.2s). Some monsters flash white or red on hit. |
| **Death** | 2-4 frames | 1 (direction-independent) | Collapse/dissolve animation. Final frame fades out with transparency. Some monsters have unique death effects (slimes "pop," undead crumble). |
| **Special Attack** | 3-8 frames | 4-5 (mirrored) | Only for bosses/elites with multiple attack types. |
| **Spawn/Appear** | 2-4 frames | 1 | Optional: materialization effect. Some monsters simply appear; others have fade-in or emerge-from-ground effects. |

**Monster animation personality examples:**

| Monster | Idle Signature | Attack Signature | Death Signature |
|---------|---------------|-----------------|-----------------|
| Poring | Gentle bounce, eyes blink | Quick lunge forward | Squish/pop, deflate |
| Skeleton | Jaw clatter, weapon shift | Sword slash overhead | Collapse into bone pile |
| Baphomet (MVP) | Dark aura pulse, wings spread | Giant scythe sweep | Extended death sequence, explosions |
| Whisper | Float/bob with transparency flicker | Phase-through lunge | Dissolve into mist |
| Peco Peco | Head bob, scratch ground | Beak peck/charge | Fall sideways |

### 3.3 Boss / MVP Visual Differences

Boss and MVP monsters are visually distinguished from normal monsters through several techniques:

| Visual Feature | Normal Monster | Mini-Boss | MVP Boss |
|---------------|----------------|-----------|----------|
| **Size** | Standard (0.5-1.5x player) | 1.2-2x player (20% larger than normal variant) | 3-8x player (dramatically oversized) |
| **Sprite detail** | Standard | Same as normal + size increase | Higher detail sprites, more animation frames |
| **Color intensity** | Normal palette | Slightly enhanced saturation | More vivid, often darker or more intense palette |
| **Visual effects** | None | Champion aura (glow around sprite) | Persistent aura effects, particle overlays, screen presence effects |
| **Animation complexity** | 5 actions | 5 actions | 6-8 actions (multiple attack types, enrage state, special moves) |
| **Death sequence** | Simple fade | Simple fade + minor effect | Extended death animation, explosion/energy release VFX, screen flash |

**Champion monsters** (elite versions of regular monsters) are distinguished by:
- 20% larger sprite scale
- Significantly higher HP
- A visible Champion aura effect (glowing outline or particle effect around the sprite)
- Same base sprite as the regular monster, just scaled and with an aura overlay

**MVP-specific visual elements:**
- MVPs often have unique idle animations not shared with their normal counterparts (Baphomet spreads wings, Orc Lord brandishes weapon).
- Some MVPs have multi-phase visual changes (different stance/aura when below 50% HP).
- MVP death is typically the most elaborate visual event in classic RO: extended death animation, energy release effects, particle shower, and sometimes a screen flash.
- MVP summoning slaves (NPC_SUMMONSLAVE) creates a visible burst of smaller monsters appearing around the MVP -- a rush of spawn effects radiating outward.

---

## 4. Skill VFX

### 4.1 Spell Casting Circles

The casting circle is one of RO's most iconic visual elements. When a character begins casting a spell, a **magic circle** appears beneath their feet on the ground.

**Casting circle characteristics:**

| Property | Description |
|----------|-------------|
| **Shape** | Circular, with concentric rings. Outer ring contains runic/arcane symbols. Inner area has geometric patterns. |
| **Size** | Approximately 1.5-2x the character's width, appearing on the ground plane. |
| **Animation** | The circle spins slowly (approximately 15-30 degrees/second) during the cast time. Glow intensity increases as the cast progresses. |
| **Appearance** | Fades in rapidly at cast start (~0.2s). Full opacity during cast. Bright flash on cast completion, then rapid fade-out (~0.3s). |
| **Interruption** | If the cast is interrupted, the circle flickers/shatters and fades out quickly (~0.5s). |
| **Source file** | Stored as `data\texture\effect\magic_target.tga` in the classic client. Listed as effect ID 60 in the effects database. |

**Element-specific casting circle colors:**

| Element | Circle Color (Inner) | Circle Color (Outer) | Glow Color |
|---------|---------------------|---------------------|-----------|
| Fire | Deep red-orange (#FF4400) | Warm orange (#FFAA00) | Orange (#FF6600) |
| Water/Ice | Deep blue (#0044FF) | Cyan (#00CCFF) | Blue (#0088FF) |
| Wind/Lightning | Green (#00CC44) | Light green (#88FF88) | Lime (#44FF44) |
| Earth | Dark brown (#886600) | Golden brown (#CCAA44) | Amber (#AA8800) |
| Holy | White (#FFFFFF) | Pale gold (#FFFFAA) | Warm white (#FFFFCC) |
| Shadow/Dark | Deep purple (#440066) | Violet (#8800CC) | Purple (#6600AA) |
| Poison | Dark green (#006600) | Green (#00CC00) | Bright green (#00AA00) |
| Neutral | Gray (#AAAAAA) | White (#FFFFFF) | Light gray (#CCCCCC) |

**Cast bar UI element:**
Simultaneously with the ground casting circle, a **cast bar** appears above the character's head -- a horizontal progress bar that fills from left to right during the cast time. The cast bar has a simple colored fill (matching the skill's element) with a dark border.

### 4.2 Skill Impact Effects (Per Element)

Each element family has a distinctive visual language for its skill impacts:

**Fire:**
- **Visual language**: Bright orange-red flames, upward-rising fire particles, heat shimmer, ember trails.
- **Key skills**: Fire Bolt (red bolts from sky), Fire Ball (flaming projectile + explosion), Fire Wall (wall of flame segments on ground), Meteor Storm (giant flaming rocks falling from sky), Magnum Break (radial fire burst around caster), Fire Pillar (flame erupts from ground).
- **Color palette**: Deep red (#CC2200) through bright orange (#FF8800) to yellow-white (#FFEE88) at hot spots.

**Ice/Water:**
- **Visual language**: Blue-white crystals, frost particles, snowflake-shaped impact markers, ice shard debris, frozen encasement.
- **Key skills**: Cold Bolt (ice-blue bolts from sky), Frost Diver (ice projectile + freeze encasement), Storm Gust (blizzard with ice crystals across AoE), Frost Nova (radial ice explosion), Water Ball (water spheres crashing on target).
- **Color palette**: Deep blue (#003399) through cyan (#00CCFF) to white (#FFFFFF) at brightest points.

**Lightning/Wind:**
- **Visual language**: Jagged electric arcs, bright white-yellow bolts, crackling energy, spark bursts, wind spiral effects.
- **Key skills**: Lightning Bolt (electric bolts from sky), Thunderstorm (lightning strikes across AoE), Jupitel Thunder (chain lightning balls), Lord of Vermilion (massive AoE lightning rain with screen flash).
- **Color palette**: Deep yellow (#CCAA00) through bright yellow (#FFFF00) to white (#FFFFFF). Electric arcs are thin bright white lines with yellow glow.

**Earth:**
- **Visual language**: Brown/amber energy, rising rock debris, ground cracks, dust/sand particles, stone pillars.
- **Key skills**: Earth Spike (stone pillars erupt from ground), Heaven's Drive (ground wave), Stone Curse (target turns gray/stone).
- **Color palette**: Dark brown (#663300) through amber (#CC8800) to sandy tan (#DDBB88).

**Holy:**
- **Visual language**: Golden-white light, upward-rising sparkles, halo/ring effects, divine rays, cross/star patterns.
- **Key skills**: Heal (rising green-gold sparkles), Blessing (golden aura), Magnus Exorcismus (grand cross pattern with holy light columns), Turn Undead (holy light burst), Sanctuary (ground healing field with golden glow).
- **Color palette**: Golden (#FFD700) through warm white (#FFFFF0) to pure white (#FFFFFF). Green-gold (#88CC44) for healing specifically.

**Shadow/Dark/Ghost:**
- **Visual language**: Purple-black energy, ghostly wisps, dark smoke trails, skull/spirit imagery, void effects.
- **Key skills**: Soul Strike (purple ghost projectiles), Napalm Beat (dark energy burst), Dark element attacks.
- **Color palette**: Deep purple (#330066) through violet (#8800CC) to lavender (#CC88FF) at edges.

**Poison:**
- **Visual language**: Green bubbling liquid, toxic cloud/mist, dripping droplets, sickly green glow.
- **Key skills**: Envenom (green poison splash), Venom Dust (poison cloud on ground), Poison element attacks.
- **Color palette**: Dark green (#004400) through medium green (#00AA00) to yellow-green (#88CC00) at bright spots.

### 4.3 Ground Effect Visuals

Ground effects are persistent AoE visual zones that remain on the map for a duration:

| Skill | Visual Description | Duration | Size | Color Theme |
|-------|-------------------|----------|------|-------------|
| **Sanctuary** (1015) | Glowing golden circle on ground with gentle upward sparkle particles. Warm healing light. Cross or star pattern visible in the center. | Up to 30s | 5x5 cells | Gold/white |
| **Storm Gust** (806) | Blizzard zone with swirling ice crystals, snowfall particles, frost accumulation on ground, visibility-reducing white haze | 4.6s total (10 hits) | 9x9 cells | Blue-white |
| **Meteor Storm** (802) | Fiery impact craters on ground, burning debris, smoke rising from impact points, ember particles | ~8-10s (varies) | 7x7 cells | Red-orange |
| **Lord of Vermilion** (807) | Crackling lightning field, electric arcs jumping between points, ground glow with spark bursts | ~5s | 9x9 cells | Yellow-white |
| **Fire Wall** (209) | Line of flame segments on ground, each segment burns with upward fire particles, enemies passing through are ignited | 5-13s | Linear (14 cells) | Red-orange |
| **Fire Pillar** (808) | Trap that erupts as a column of flame when triggered. Pre-trigger: subtle ground glow. Post-trigger: towering fire column | 30s (trap) + burst | 1 cell | Red-orange |
| **Safety Wall** (812) | Translucent white barrier/dome on ground. Subtle shimmer effect. Absorbs physical hits. | Until hits absorbed | 1 cell | White/translucent |
| **Pneuma** (211) | Near-invisible wind barrier. Very subtle visual -- slight air distortion/shimmer. Blocks ranged attacks. | 10s | 3x3 cells | Nearly invisible |
| **Land Protector** (1421) | Green protective zone on ground. Blocks all ground-targeted magic. Visible but not overwhelming. | 3-5 min | Variable | Green |
| **Quagmire** (813) | Dark muddy/swampy zone on ground. Slows movement and reduces stats. Brownish-green coloration with bubble effects. | 5-10s | 5x5 cells | Brown-green |
| **Volcano** (1414) | Red-orange glowing zone. Increases fire damage and ATK. Lava-like ground effect with heat shimmer. | 1-5 min | 7x7 cells | Red-orange |
| **Deluge** (1415) | Blue water zone on ground. Increases water damage and MaxHP. Gentle water ripple effect. | 1-5 min | 7x7 cells | Blue |
| **Violent Gale** (1416) | Green wind zone. Increases wind damage and Flee. Swirling wind particles near ground. | 1-5 min | 7x7 cells | Green |

**Performance system ground effects (Bard/Dancer):**
Songs and dances create a visible AoE zone around the performer. Unlike other ground effects, solo performances **follow the caster** as they move. Ensemble effects (Bard+Dancer duet) create **stationary** zones at the midpoint between the two performers.

### 4.4 Buff Visual Indicators

Active buffs display as persistent visual effects attached to the character:

| Buff | Visual Effect | Attachment Point | Color |
|------|--------------|-----------------|-------|
| **Blessing** | Rising golden sparkle particles around character | Full body | Gold (#FFD700) |
| **Increase AGI** | Blue-white speed lines trailing from feet/body during movement, subtle aura at rest | Feet/lower body | Blue-white (#88CCFF) |
| **Angelus** | Translucent golden shield dome/sphere around character | Full body (enclosing) | Gold, semi-transparent |
| **Kyrie Eleison** | Visible white barrier/sphere effect around character. Shows remaining hit count. | Full body | White/holy (#FFFFFF) |
| **Provoke** | Red angry icon above head + faint red aura tint | Above head | Red (#FF4444) |
| **Endure** | Subtle yellow/golden outline glow | Body outline | Yellow-gold (#FFCC00) |
| **Two Hand Quicken / Adrenaline Rush** | Faint red/orange speed aura around hands/weapon area | Upper body/hands | Red-orange (#FF6644) |
| **Maximize Power** | Weapon glows with enhanced white-yellow light | Weapon | White-yellow (#FFFF88) |
| **Steel Body** | Character tints slightly metallic/gray, reduced animation speed | Full body tint | Silver-gray |
| **Freeze (debuff)** | Character encased in ice crystal block. Cannot move. | Full body encasement | Blue-white (#88DDFF) |
| **Stone Curse (debuff)** | Character turns gray stone texture. Cracks appear. | Full body texture override | Gray (#888888) |
| **Stun (debuff)** | Stars/sparkles spinning in circle above head | Above head | Yellow stars (#FFFF00) |
| **Silence (debuff)** | Speech bubble with X mark above head | Above head | White with red X |
| **Blind (debuff)** | Dark fog/shadow around character's head area | Head area | Dark (#333333) |
| **Poison (debuff)** | Green dripping particles from body + periodic green flash | Body | Green (#00CC00) |
| **Hallucination (debuff)** | Screen distortion/wave effect (player-side only) | Camera/screen | Psychedelic colors |

---

## 5. UI Art Style

### 5.1 RO Classic Brown / Gold / Parchment Theme

RO's UI is one of the most immediately recognizable elements of its visual identity. The interface uses a warm, medieval-inspired **brown wood panel with gold trim** aesthetic that evokes parchment, wooden furniture, and medieval manuscripts.

**Design principles:**
- **Warmth over sterility**: Every UI element feels handcrafted, not clinical. Wood grain, parchment texture, and ornate metal corners create a tactile quality.
- **Minimal chrome/glass**: No glossy plastic or glass-effect elements. The UI feels made of natural materials -- wood, leather, parchment, metal.
- **Consistent theming**: Every panel, button, scroll bar, and icon frame uses the same visual language. There is no element that breaks the medieval parchment theme.
- **Readability first**: Despite the decorative style, text is always highly readable. Dark text on light parchment backgrounds, or light text on dark panel headers.

**Core color values:**

| Element | Color (Hex) | Role |
|---------|-------------|------|
| Panel Background (Wood) | #5C3A1E | Warm dark wood, primary surface |
| Panel Background (Lighter center) | #6B4A2E | Subtle radial gradient center |
| Panel Background (Dark edge) | #4A2A10 | Edge vignetting |
| Title Bar | #4A2A10 | Slightly darker than panel body |
| Title Text | #FFE8B0 | Warm cream-gold |
| Border Highlight | #8B6A3E | Raised edge bevel |
| Border Shadow | #2A1A08 | Inset edge bevel |
| Corner Ornaments | #C0A060 | Gold-tinted metal brackets |
| Content Background | #F0E8D0 at 25% opacity | Parchment overlay on panel |
| Body Text | #2A1A08 | Dark brown for readability |
| Label Text | #8B4513 | Medium brown for secondary text |
| Separator Lines | #8B6A3E | Gold-brown horizontal rules |
| Button (Normal) | #6B4A2E -> #4A2A10 gradient | Top-to-bottom linear gradient |
| Button (Hover) | #7B5A3E -> #5A3A20 gradient | 15% brightened |
| Button (Hover Border) | #C0A060 | Gold highlight glow |
| Button (Pressed) | #5B3A1E -> #3A1A00 gradient | 10% darkened, text offset 1px down |
| Scrollbar Track | #3A1A08 | Dark groove |
| Scrollbar Thumb | #6B4A2E | Wood texture bar with ridge detail |

### 5.2 Window Design (Title Bar, Borders, Buttons)

**Window anatomy:**

```
+==============================+  <- Gold corner ornaments (4 corners)
| [X]  Window Title            |  <- Title bar: dark brown bg, cream-gold text, close button (X)
+------------------------------+
|                              |  <- 3px inset bevel border
|   Content Area               |  <- Parchment-tinted background
|                              |
|   [Button] [Button]         |  <- Brown gradient buttons with gold border
|                              |
|   +---------+               |  <- Inner sub-panels use same wood but slightly lighter
|   | Sub-panel|               |
|   +---------+               |
|                          [^]|  <- Scrollbar on right edge
|                          [ ]|
|                          [v]|
+==============================+  <- Bottom border with corner ornaments
```

**Window behaviors:**
- **Draggable**: All windows can be dragged by their title bar. This is a core interaction pattern.
- **Close button**: Small [X] in the title bar, rendered as a decorative cross icon.
- **Minimize**: Some windows support minimizing to a compact form showing only the title bar.
- **Transparency**: Windows are mostly opaque but some (like the minimap) have slight transparency to see the game world beneath.
- **Z-ordering**: Windows layer on top of each other with the most recently interacted window on top.
- **Fixed anchor**: Most windows remember their last position and re-open there.

**Tab design:**
Some windows use tabs (e.g., Equipment window has tabs for different equipment views). Tabs appear as slightly raised rectangular segments along the top or bottom of the content area, with the active tab matching the content background color and inactive tabs being slightly darker.

### 5.3 Icon Design (Skill Icons, Item Icons)

**Skill icons:**

| Property | Specification |
|----------|--------------|
| Size | 24x24 pixels in-game display (drawn at higher resolution, typically 48x48 or 96x96) |
| Background | Dark gradient (#222222 to #111111) |
| Border | 1px gold frame (#C0A060) |
| Art style | Simplified symbolic representation of the skill. Bold shapes, limited palette (3-5 colors per icon). |
| Readability | Must be recognizable at 24x24 display size. Distinct silhouette is critical. |
| Color coding | Icons subtly reflect element -- fire skills have warm red/orange tones, ice skills have blue tones, etc. |

**Item icons:**

| Property | Specification |
|----------|--------------|
| Size | 24x24 pixels in-game display |
| Background | Transparent (icon sits on inventory slot background) |
| Perspective | Slight 3/4 view (not flat orthographic, not full perspective) |
| Outline | 1px dark outline on all items for readability against any background |
| Shading | Simple 2-tone shading with highlight. Painterly but not realistic, not flat. |
| Color | Saturated and distinctive -- each item must be recognizable by color alone at inventory scale |
| Scale consistency | All items drawn at same relative scale (a dagger is smaller than a sword in the icon) |

**Item icon categories and visual conventions:**

| Category | Visual Convention |
|----------|------------------|
| Potions | Small flask/bottle shape. Color indicates type: red (HP), blue (SP), white (cure), green (stat) |
| Weapons | Weapon shape in profile. Color/material indicates tier. |
| Armor | Chest plate or armor piece. Usually metallic colors. |
| Cards | Small rectangular card with monster illustration. Distinct from items by shape. |
| Crafting materials | Raw material shape (ore chunk, herb leaf, monster drop). |
| Headgear | Miniature version of the headgear appearance. |
| Arrows/Ammo | Small arrow/bolt shapes, color-coded by element. |
| Gems/Crystals | Faceted gem shapes, color-coded by element. |

### 5.4 Damage Number Display Style

RO's floating damage numbers have a distinctive and satisfying visual presentation:

**Display characteristics:**

| Property | Description |
|----------|-------------|
| **Font** | Bold sans-serif, slightly condensed. Custom bitmap font (not a system font). |
| **Size** | Variable -- starts large and shrinks during the float animation. Critical hits start even larger. |
| **Animation path** | Inverted U-shape trajectory: numbers rise upward, arc to one side, then fall while fading out. This creates a "bouncing" visual effect. |
| **Duration** | Approximately 1-1.5 seconds from appearance to full fade. |
| **Scaling** | Numbers scale down from initial size during the animation, becoming less visually prominent as they become less relevant. |
| **Stacking** | Multiple damage numbers from rapid hits (e.g., Double Attack, multi-hit skills) stack vertically or fan out to remain readable. |

**Color coding by damage type:**

| Damage Type | Color | Additional Visual |
|-------------|-------|-------------------|
| **Normal hit (player deals)** | White | Standard size |
| **Normal hit (player receives)** | White or light red | Standard size |
| **Critical hit** | Yellow (#FFFF00) | Larger initial size, bold "CRITICAL" text above number |
| **Multi-hit total** | Yellow (#FFFF00) | Displayed after individual hit numbers; shows combined total |
| **Individual multi-hit** | Faint white / light gray | Smaller, less prominent than the total |
| **Miss / Dodge** | White | "Miss" text instead of number, smaller |
| **Heal** | Green (#00CC00) | Upward-rising, sometimes preceded by "+" symbol |
| **Skill damage** | White (standard) or colored by element | May be larger for high-damage skills |
| **Blocked (shield)** | -- | No number; shield block visual + sound effect |

**High-resolution damage sprite details:**
Custom damage number sprite projects (such as high-res scrolling damage sprites) use **true-to-RO color gradients** for the damage display -- each digit has a subtle gradient from a lighter color at the top to a slightly darker shade at the bottom, giving the numbers a sense of depth.

### 5.5 Emotion Bubbles

Emotes in RO display as **speech bubble icons** that appear above the character's head:

**Emote system:**
- Activated by chat commands (e.g., `/gg`, `/ho`, `/no`, `/!`, `/?`) or via the Emotion List window (Alt+L).
- The emote appears in a small white speech bubble with a thin dark outline.
- Duration: approximately 2-3 seconds before fading out.
- The bubble follows the character's head position.

**Core emote list (classic pre-Renewal):**

| Command | Emote Icon | Visual Description |
|---------|-----------|-------------------|
| `/!` or `/e1` | Exclamation | Red exclamation mark (!) |
| `/?` or `/e2` | Question | Blue question mark (?) |
| `/gg` or `/e3` | Music Notes | Purple music notes |
| `/ho` or `/e4` | Heart | Red heart |
| `/no` or `/e5` | Sweat Drop | Blue sweat drop |
| `/e6` | Ellipsis | Three dots (...) |
| `/e7` | Anger | Red anger vein cross |
| `/e8` | Cash/Dollar | Dollar sign ($) |
| `/e9` | Thumbs Down | Red thumbs down |
| `/e10` | Star | Yellow star |
| `/e11` | Skull | White skull |
| `/e12` | Eyes | Large staring eyes |
| `/thx` | Thank You | Bowing figure |
| `/lv` or `/love` | Heart Eyes | Pink hearts |
| `/an` or `/anger` | Red Face | Red angry face |

**Visual characteristics:**
- The emote icons are pixel art / small painted sprites, typically 24x24 to 32x32 pixels.
- They use bold, simple shapes with high contrast for readability.
- The speech bubble is a standard rounded rectangle with a triangular pointer toward the character.
- Emote sprites were originally borrowed from another Gravity game, "Arcturus: The Curse and Loss of Divinity" (the anger, exclamation, and heart bubbles specifically).
- Approximately 33-40 standard emotes exist in classic RO, with later patches adding more (up to 71+ in extended clients).

---

## 6. Environment Art

### 6.1 Town Themes Per City

Each RO town has a strong, distinct cultural identity reflected in its architecture, color palette, vegetation, and atmospheric effects:

| Town | Cultural Theme | Architecture | Key Visual Elements | Palette |
|------|---------------|-------------|--------------------|---------|
| **Prontera** | Central European Medieval | Gothic cathedral, stone walls, cobblestone streets, marketplace stalls | Central fountain square, cathedral with spires, castle gates to the north, green meadows surrounding | Warm stone (#C4A882), forest green, sky blue |
| **Izlude** | Mediterranean Coastal | White-washed buildings, terracotta roofs, wooden docks | Arena building, lighthouse, harbor with ships | White (#F5F5F0), terracotta (#CC6633), sky blue |
| **Geffen** | Arcane European | Dark stone towers, magical crystals embedded in architecture, arcane circles on ground | Central magic tower (enormous, city built around it), library, wizard guild, underground dungeon | Deep blue (#1A2A5E), purple (#6633AA), gold (#FFD700) |
| **Payon** | Korean Traditional | Hanok-style wooden buildings, curved tile roofs, bamboo groves, wooden bridges | Palace compound, archery range, cave entrance nearby, thick surrounding forest | Rose (#CC6677), wood brown (#8B6914), bamboo green (#556B2F) |
| **Morroc** | Arabian/Egyptian | Sandstone buildings, market tents and awnings, pyramid visible in distance | Pyramid entrance, destroyed quarter (post-Satan Morroc event), oasis, bustling market | Sand (#DAA520), terracotta (#CC6633), teal accent (#008080) |
| **Alberta** | Nordic/Merchant Port | Timber-frame buildings, wooden docks, merchant ships, warehouses | Harbor with moored ships, merchant guild, shipwright | Sea blue (#4682B4), timber brown (#8B6914), rope tan |
| **Aldebaran** | Clockwork European | Clock tower center, canal system with bridges, mechanical elements | The iconic Clock Tower, canal waterways, Al De Baran Kafra | Brass (#B5A642), stone gray (#808080), water blue (#4169E1) |
| **Lutie** | Christmas/Nordic Winter | Snow-covered cottages, candy cane decorations, pine trees, toy-themed buildings | Toy Factory, giant Christmas tree, perpetual snow | Snow white (#FFFAFA), red (#CC0000), green (#006400) |
| **Amatsu** | Japanese Traditional | Torii gates, shoji screen walls, zen gardens, pagodas, cherry blossoms | Shinto shrine, tatami rooms, bridge over koi pond | Red (#CC3333), black lacquer, cherry pink (#FFB7C5) |
| **Kunlun/Gonryun** | Chinese Traditional | Dragon pillars, curved upswept roofs, floating islands, misty mountain backdrop | Dragon temple, cloud platforms, mystical atmosphere | Red (#CC0000), gold (#FFD700), jade green (#00A86B) |
| **Comodo** | Tropical Cave/Beach | Palm trees, tiki torches, cave entrances, waterfall, sandy beach | Casino entrance, beach area, cave network entrances, nightlife atmosphere | Turquoise (#40E0D0), sand (#F4A460), torch orange (#FF8C00) |
| **Umbala** | Tribal/Treehouse | Massive ancient trees, rope bridges between platforms, wooden platforms | World Tree base, treehouse village, tribal decorations | Deep green (#006400), wood (#8B7355), earth (#8B4513) |
| **Niflheim** | Norse Underworld | Dead/twisted trees, perpetual fog, dark stone, ghostly lights | Dead Man's mansion, grey market, oppressive atmosphere | Gray (#696969), purple (#4B0082), sickly green (#8FBC8F) |
| **Juno/Yuno** | Greek/Academic | Marble columns, floating islands connected by bridges, libraries | Sage Academy, Valkyrja temple, floating island architecture | White marble (#F5F5F5), sky blue (#87CEEB), gold (#FFD700) |
| **Einbroch** | Industrial German | Smokestacks, metal/riveted buildings, railway tracks, gear motifs | Train station, factory district, smog atmosphere | Iron gray (#5A5A5A), rust (#B7410E), industrial brown |
| **Lighthalzen** | Modern Corporate | Glass/steel buildings (upper), slum contrast (lower), laboratories | Rekenber Corporation HQ, stark rich/poor divide, Biolabs entrance | Steel blue (#4682B4), clean white, slum brown (#8B6914) |
| **Rachel** | Religious/Arctic | White stone temple complex, cold architecture, religious iconography | Grand temple, Ice Cave entrance, devout atmosphere | Ice white (#F0F8FF), pale blue (#B0E0E6), gold (#FFD700) |
| **Veins** | Volcanic/Desert | Red rock formations, heat haze, volcanic vents, cracked earth | Thor's Volcano entrance, geothermal landscape | Red (#B22222), orange (#FF6347), volcanic black (#1A1A1A) |
| **Hugel** | Alpine Bavarian | Timber-frame houses, mountain backdrop, green meadows | Odin Temple approach, brewery, pastoral landscape | Green (#228B22), wood brown (#DEB887), mountain blue (#6495ED) |

### 6.2 Dungeon Visual Themes

RO dungeons each have a strong atmospheric identity:

| Dungeon | Type | Visual Theme | Lighting | Atmosphere | Monster Types |
|---------|------|-------------|----------|-----------|--------------|
| **Payon Cave** | Natural Cave | Rock walls, stalactites, underground streams, wooden support beams | Dim torchlight, occasional bioluminescence | Eerie, traditional horror (undead theme) | Skeletons, Zombies, Munak, Bongun, Sohee |
| **Prontera Culverts** | Sewer/Culvert | Brick tunnels, water channels, iron grates, dripping pipes | Dim overhead lighting, water reflections | Claustrophobic, damp | Thief Bugs, Thief Bug Eggs, Golden Thief Bug (MVP) |
| **Ant Hell** | Natural Cave/Colony | Organic tunnels, ant colony architecture, soil walls | Dark with scattered light sources | Industrial (ant colony busy-ness) | Andre, Deniro, Piere, Maya (MVP) |
| **Pyramid (Morroc)** | Ancient Temple/Ruins | Sandstone walls, hieroglyphics, carved pillars, treasure rooms | Shaft light through cracks, torch-lit chambers | Ancient, mysterious, treasure-laden | Mummies, Verit, Isis, Pasana, Osiris (MVP) |
| **Sphinx** | Ancient Temple | Elaborate stone chambers, sphinx statues, trap corridors | Warm torchlight, golden hue | Grand, imposing, puzzle-like | Pasana, Minorous, Marduk, Phreeoni (MVP) |
| **Glast Heim** | Gothic Castle Ruins | Dark stone halls, stained glass windows, iron fixtures, cobwebs, crumbling masonry | Moonlight through windows, candelabras, ghostly light | Dark, haunted, oppressive, once-grand | Raydric, Khalitzburg, Abysmal Knight, Dark Lord (MVP) |
| **Clock Tower (Aldebaran)** | Mechanical | Gears, pipes, moving clock mechanisms, bronze/brass surfaces | Industrial sparks, steam vents, warm metal glow | Mechanical, ticking, vertically-oriented | Alarm, Clock, Owl Baron, Owl Duke |
| **Geffen Tower/Dungeon** | Magical Underground | Dark stone with magical rune inscriptions, arcane crystals | Purple-blue magical glow, crystal luminescence | Mystical, dangerous | Jakk, Whisper, Doppelganger (MVP) |
| **Byalan (Undersea)** | Underwater | Coral formations, seaweed, bubbles, sunken structures | Caustic light from surface, bioluminescence | Serene-to-dangerous (deeper = darker) | Hydra, Marina, Vadon, Marc, Cornutus |
| **Thor's Volcano** | Volcanic | Obsidian walls, lava flows, heat distortion, crumbling platforms | Red-orange lava glow, fire particle effects | Hostile, hot, dangerous | Kasa, Salamander, Ifrit (MVP) |
| **Ice Cave (Rachel)** | Frozen | Ice walls, frost crystals, frozen creatures embedded in ice | Blue-white ambient, crystal refraction | Cold, pristine, deadly | Ice Titan, Snowier, Ktullanux (MVP) |
| **Niflheim Dungeon** | Haunted/Underworld | Twisted architecture, impossible geometry, ghostly elements | Flickering, ghostly green glow | Unsettling, horror | Loli Ruri, Dullahan |
| **Abbey (Cursed Monastery)** | Corrupted Holy | Once-sacred architecture now defiled, dark blood stains, broken religious icons | Dim red-tinted, occasional holy light | Tragic corruption, horror | Banshee, Necromancer, Fallen Bishop (MVP) |
| **Thanatos Tower** | Tower/Vertical | Stacked floors with increasing difficulty, varying themes per floor | Changes per floor (lit -> dark -> otherworldly) | Escalating dread | Various per floor, Thanatos (MVP) |

### 6.3 Field / Outdoor Visual Design

Fields are the expansive outdoor areas between towns where players travel and fight monsters:

| Biome | Terrain | Vegetation | Props | Lighting | Example Zones |
|-------|---------|-----------|-------|----------|--------------|
| **Grassland** | Rolling green hills, dirt paths, gentle elevation | Grass, wildflowers, scattered deciduous trees | Fences, signposts, wells, road markers | Bright warm daylight | Prontera fields (prt_fild01-08) |
| **Forest** | Dense tree cover, dappled ground, leaf litter | Dense mixed trees, undergrowth, mushrooms, moss | Fallen logs, moss-covered rocks, streams | Dappled sunlight through canopy | Payon fields (pay_fild01-11) |
| **Desert** | Flat sand dunes, cracked earth, scattered oasis | Cacti, dead shrubs, tumbleweeds | Bleached bones, ancient ruins, oasis pools | Harsh bright overhead sun | Morroc fields (moc_fild01-22) |
| **Snow/Tundra** | Snow-covered flat terrain, frozen lakes | Pine trees, frozen bushes, icicle formations | Ice formations, snow drifts, frozen ponds | Cool diffused light, high reflection | Lutie fields (xmas_fild01) |
| **Mountain** | Steep rocky slopes, cliff faces, narrow passes | Alpine flowers, pine trees at lower elevations | Boulders, mine entrances, cliff edges | Clear bright, dramatic shadows | Mjolnir Mountains (mjolnir_01-12) |
| **Coast/Beach** | Sandy shores meeting ocean water | Palm trees, beach grass, tropical flowers | Driftwood, shells, beached boats | Bright warm, water reflections | Byalan coast, Alberta coast |
| **Swamp** | Flat, muddy, shallow standing water | Mangrove trees, lily pads, reeds, hanging moss | Bubbling mud pools, fog banks | Green-tinted diffused, atmospheric | Comodo swamp areas |
| **Volcanic** | Dark rock, lava fissures, scorched earth | Minimal/dead vegetation, fire-resistant plants | Lava pools, geysers, volcanic vents | Red-orange ambient, heat haze | Thor's Volcano exterior |
| **Corrupted/Dark** | Discolored terrain, cracked ground, dead soil | Dead/twisted trees, toxic-looking plants | Ruined structures, dark crystals | Dim, purple-green tint | Glast Heim exterior, Niflheim surroundings |

**Field design principles:**
- **Readability over density**: RO fields are designed to be easily navigable. Clear sightlines to town entrances, warp portals, and dungeon entrances. Monster spawn areas are visible open spaces.
- **Walking paths**: Visible dirt paths guide players between points of interest. These are textured differently from surrounding terrain.
- **Landmark navigation**: Each field has at least one distinctive landmark (unique tree, rock formation, ruin) that helps with spatial orientation.
- **Spawn zone clarity**: Areas where monsters spawn are typically flat, open ground with clear visibility, making combat encounters predictable and fair.

---

## 7. Implementation for UE5

### 7.1 Translating 2D Sprite Style to 3D

The fundamental challenge in replicating RO's visual identity in UE5 is preserving the warmth, charm, and readability of 2D sprite art while using 3D models and modern rendering.

**Approach: PBR + NPR Hybrid**

Rather than attempting to replicate 2D sprites in 3D (which produces uncanny results), the recommended approach is a "PBR base with anime/cel-shading post-processing" pipeline. This is the approach used by modern games that successfully capture a similar aesthetic:

| Reference Game | Relevance to RO Style |
|---------------|----------------------|
| **Tree of Savior** | Same spiritual lineage (same original creator). 2.5D-to-3D transition. Hand-painted textures, warm color grading. Closest direct reference. |
| **Genshin Impact** | Anime cel-shading on PBR base. Strong silhouettes, painterly environments. Demonstrates that anime aesthetic scales to AAA quality. |
| **Blue Protocol** | Anime rendering pipeline with bright saturated colors in an MMO context. |
| **Ni no Kuni** | Studio Ghibli-inspired cel-shading. Soft shadows, watercolor-adjacent rendering. Similar warmth to RO. |

**Key technical strategies:**

1. **Cel-shading post-process**: Quantize scene lighting to 2-3 discrete bands (shadow, mid, highlight) via post-process material. This replicates the flat, painted look of RO sprites without per-material work.

2. **Sobel outline pass**: Edge detection on depth + normal buffers produces the dark outlines that are characteristic of RO's sprite style. Configurable thickness (1.5-2.5 pixels at 1080p) and color (near-black brown for towns, dark blue for night zones).

3. **Hand-painted textures**: All textures should be painted or AI-upscaled-then-painted-over, NOT photoscanned. The texture style should use flat color fields with painted shadows and highlights. Normal maps should be subtle -- broad forms only, no pore-level detail.

4. **Chibi-adjacent proportions**: 3D character models should maintain RO's proportional language: ~1:5.5 head-to-body ratio, large expressive eyes, slightly oversized hands and feet. NOT realistic proportions.

5. **Saturated warm palette**: Color grading LUTs per zone should enforce RO's warm, saturated color philosophy. Avoid desaturated, "realistic" palettes.

6. **High roughness materials**: Anime-style materials are NOT glossy. Roughness values should be 0.6-0.9 across the board. Metallic should be binary (0 or 1), never "somewhat metallic."

### 7.2 Maintaining the RO Feel in 3D

Beyond technical rendering, preserving the "RO feel" requires attention to design-level decisions:

**Character design:**
- Maintain the compact, lean body type for all classes. Even Knights and Blacksmiths should not be bulky/muscular.
- Hair should be a prominent visual feature with exaggerated volume and movement (hair physics via bone chains).
- Class silhouettes must be instantly recognizable. Each class outfit should have a unique profile at any zoom level.
- Headgear should be visually prominent and slightly oversized relative to the head (matching RO's emphasis on headgear as the primary customization vector).

**Monster design:**
- 3D monsters should feel like "the 2D sprite inflated into 3D" -- maintain the silhouette, color palette, and proportional relationships.
- Do NOT reimagine monsters as photorealistic or grimdark. A Poring is a cute, bouncy pink blob. An Orc Warrior is a cartoonish green brute, not a realistic monster.
- Eyes must be the expressive focus. Monster personality comes through their eye design.
- Scale relationships between monsters must match RO: a Poring is tiny next to a Baphomet.

**Animation:**
- Use UE5's Animation Blueprint system with state machines (Idle/Walk/Run/Attack/Cast/Death states).
- ASPD-based attack playback: The server sends `attackMotion` in ms. The client adjusts montage play rate: `PlayRate = BaseAnimLength / (attackMotion / 1000.0)`.
- Maintain RO's animation "snappiness" -- attacks should be quick and punchy, not fluid and realistic. The slight stop-motion quality of low-frame sprite animation can be approximated by using fewer keyframes and sharper interpolation curves.
- Emote animations should play as full-body montages with matching emote bubble UI overlay.

**UI:**
- The brown/gold/parchment theme is already implemented in the project's Slate UI system using the `ROColors` namespace.
- All UI panels should maintain the wood panel aesthetic with gold corner ornaments.
- Damage numbers should follow the inverted U-shape trajectory with color coding (white normal, yellow crit, green heal).

**Environment:**
- Each town and zone should have a distinct color palette enforced through zone-specific Post Process Volumes.
- Environments should feel like RO's "storybook diorama" quality -- slightly stylized, warm, inviting. NOT photorealistic.
- Dungeons should have strong atmospheric identity through lighting, fog, and ambient particles.
- Water should use stylized shader (not physically accurate ocean sim). Simple Gerstner waves, hand-painted foam textures.

**VFX:**
- All VFX should be built in Niagara (not Cascade, except for legacy assets).
- Element-specific color language must be consistent (fire = red-orange, ice = blue-white, lightning = yellow-white, etc.).
- Casting circles are essential -- the ground magic circle during spell casting is one of RO's most iconic visual elements.
- Buff/debuff visual indicators should be visible but not overwhelming. Persistent Niagara systems attached to character actors.
- Ground effects must be clearly visible but not obscure gameplay. Use semi-transparent ground-projected effects.

---

## 8. Gap Analysis

### 8.1 Coverage Summary

| Area | Existing Documentation Coverage | This Document's Addition | Remaining Gaps |
|------|-------------------------------|------------------------|----------------|
| **Visual style philosophy** | `10_Art_Animation_VFX_Pipeline.md` Sec 1: Rendering approach, color palettes, day/night | Art direction origins (Lee Myung-jin), sprite creation technique, chibi proportions rationale | Art direction mood boards, per-zone color target screenshots |
| **Character sprites/animation** | `10_Art_` Sec 2, 4: Body architecture, hair system, animation system | SPR/ACT file format, frame counts per action, direction system, animation personality | Actual frame-by-frame reference sheets per class |
| **Hair system** | `10_Art_` Sec 2.2-2.3: 19 styles + 9 colors + Material Instance workflow | Hair palette hex values from web research, extended style counts (31 kRO) | Visual reference images for each style |
| **Headgear** | `10_Art_` Sec 2.4: 3-slot system, production phases | Iconic headgear list, visual behavior (obscuring, anchoring) | Per-headgear sprite reference, priority list for 3D modeling |
| **Equipment visuals** | `10_Art_` Sec 2.5: Visibility per slot, weapon mesh counts | Weapon sprite variation details, armor non-visibility in classic | Weapon mesh priority list with specific item IDs |
| **Mounts** | Minimal (mentioned in skill docs) | Peco Peco visual description, Grand Peco differentiation, mount sprite behavior | Mount model reference, armor design specs |
| **Monster art** | `10_Art_` Sec 3: Size categories, production list, design guidelines | Animation personality per monster, boss/MVP visual differences, Champion aura | Monster art priority beyond Tier 1, visual reference sheets |
| **Skill VFX** | `10_Art_` Sec 6: Current VFX system, element tables, casting circles | Detailed per-element visual language, ground effect inventory, complete color specs | VFX reference videos/screenshots for each skill |
| **Buff visuals** | `10_Art_` Sec 6.4: Buff/debuff indicator table | Expanded buff list with specific color values, attachment points | Visual reference for each buff effect |
| **UI art** | `10_Art_` Sec 7: Panel specs, icon pipeline, fonts | Window anatomy diagram, tab design, emotion bubble system, damage number animation path | UI asset textures (wood grain, corner ornaments) |
| **Environment art** | `10_Art_` Sec 5: Town themes, field biomes, dungeon types | Expanded town cultural details, dungeon atmosphere descriptions, field design principles | Per-zone concept art, modular kit specs |
| **UE5 implementation** | `14_Art_Audio_Pipeline.md`: Full C++ code patterns | Summary of rendering approach decisions | Performance profiling of cel-shade + outline pipeline |

### 8.2 Existing Project Assets and Implementation Status

Based on the project documentation:

| System | Status | Key Files |
|--------|--------|-----------|
| **VFX Subsystem** | Implemented: 5 patterns (Bolt, Projectile, Multi-Hit, Persistent Buff, Ground AoE Rain) + 97+ skill configs | `SkillVFXSubsystem.h/.cpp`, `SkillVFXData.h/.cpp`, `CastingCircleActor.h/.cpp` |
| **Damage Numbers** | Implemented: inverted-U trajectory, color-coded (white/yellow/green/silver/red) | `SDamageNumberOverlay.h/.cpp`, `DamageNumberSubsystem.h/.cpp` |
| **UI Theme** | Implemented: ROColors namespace, brown/gold Slate panels, all 30+ subsystem widgets | All `S*.h/.cpp` widget files in `UI/` |
| **Character Model Pipeline** | Documented but not implemented in code | Specifications in `10_Art_Animation_VFX_Pipeline.md` |
| **Animation System** | Documented with C++ code patterns | `UMMOAnimInstance` pattern in `14_Art_Audio_Pipeline.md` |
| **Post-Processing** | Documented: cel-shade + outline + color grading | Specs in both reference docs, not yet built as UE5 materials |
| **Casting Circles** | Implemented: `CastingCircleActor` with element colors | `VFX/CastingCircleActor.h/.cpp` |

### 8.3 Prioritized Work Remaining

| Priority | Task | Effort | Dependency |
|----------|------|--------|-----------|
| **P0** | Character base model (male + female) with correct proportions | High (3D modeling) | None |
| **P0** | Novice outfit mesh (M + F) | High | Base model |
| **P0** | 5 core hair meshes per gender (styles 1-5) | Medium | Base model |
| **P1** | Post-process materials (cel-shade + outline) | Medium | None (UE5 materials) |
| **P1** | 6 first-job outfit meshes (M + F = 12) | High | Base model |
| **P1** | First 10 monster models (Poring through Peco Peco) | High | None |
| **P1** | Locomotion animations (idle, walk, run) | Medium | Base model |
| **P2** | Combat animations per weapon type (14 types) | High | Base model + weapon meshes |
| **P2** | Phase 1 headgear models (50 iconic items) | High | Base model |
| **P2** | Second-job outfit meshes (14 classes x 2 genders = 28) | Very High | Base model |
| **P2** | Zone-specific Post Process Volumes and color grading | Medium | Post-process materials |
| **P3** | Mount models (Peco Peco + Grand Peco) with riding animations | High | Base model + Knight/Crusader outfits |
| **P3** | Transcendent class outfit meshes (14 x 2 = 28) | Very High | Base model |
| **P3** | Remaining 60 monster models (Tier 2 + 3) | Very High | None |
| **P4** | Remaining hair styles (styles 6-19) | Medium | Base model |
| **P4** | Phase 2-4 headgear models (600+ items) | Very High | Base model |
| **P4** | Emotion bubble UI overlay system | Low | UI subsystem |
| **P5** | Environmental VFX per zone (weather, ambient particles) | Medium | Zone setup |

### 8.4 Key Questions for Future Research

1. **Hair physics approach**: Should hair use Alembic-cached simulation, bone chain dynamics (5-8 bones per strand), or UE5 Chaos Cloth? Bone chains are recommended for performance with 50+ characters on screen.
2. **Headgear LOD strategy**: With 1,000+ headgear items, should models share materials/atlases to reduce draw calls? Headgear items are small screen presence -- no individual LODs needed, but texture atlasing is critical.
3. **Monster animation sharing**: Can smaller monsters (Poring variants: Drops, Poporing, Marin) share animation data with different meshes/materials? Yes -- if skeletons match, the AnimBP can be reused with different mesh assignments.
4. **Sprite-to-3D reference workflow**: Should artists work from original RO sprite sheets as direct reference, or from Lee Myung-jin's concept art? Both -- sprites for gameplay accuracy (silhouette, color, proportions), concept art for detail and mood.
5. **Performance target for cel-shade post-process**: What is the GPU cost of the full post-process chain (cel-shade + outline + color grade + bloom + vignette) at 1080p and 1440p? Needs profiling on target hardware.

---

## Sources

### Web Research Sources
- [The Spriters Resource - Ragnarok Online](https://www.spriters-resource.com/pc_computer/ragnarokonline/)
- [Creating Ragnarok Online Style Sprites - Civitai](https://civitai.com/articles/8302/creating-ragnarok-online-style-sprites-a-work-in-progress-study)
- [RO Sprite Art Discussion - GameDev.net](https://www.gamedev.net/forums/topic/633661-is-ragnarok39s-art-pixel-art/)
- [Ragnarok File Formats - ACT Specification](https://github.com/rdw-archive/RagnarokFileFormats/blob/master/ACT.MD)
- [SPR File Format - Ragnarok Research Lab](https://ragnarokresearchlab.github.io/file-formats/spr/)
- [rAthena Wiki - Acts](https://github.com/rathena/rathena/wiki/Acts)
- [rAthena Wiki - Spriting](https://github.com/rathena/rathena/wiki/Spriting)
- [zrenderer - Ragnarok Online Sprite Renderer](https://github.com/zhad3/zrenderer)
- [RO Hairstyle Sprite List](https://nn.ai4rei.net/dev/hairlist/)
- [iRO Wiki - Hair Styling](https://irowiki.org/wiki/Hair_Styling)
- [Hairstyles (RO) - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Hairstyles_(RO))
- [rAthena - Hairstyle Package](https://rathena.org/board/files/file/4114-hairstyle-package/)
- [RO Headgear View Sprite List](https://nn.ai4rei.net/dev/viewlist/)
- [iRO Wiki - Equipment](https://irowiki.org/wiki/Equipment)
- [Peco Peco Ride - iRO Wiki Classic](https://irowiki.org/classic/Peco_Peco_Ride)
- [Pecopeco Mounts - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Pecopeco_Mounts)
- [Ragnarok Online Mount System](https://luffykudo.wordpress.com/2024/06/01/ragnarok-online-mount-system/)
- [Monster Category - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Category:Monsters)
- [MVP Boss Monsters - RateMyServer](https://ratemyserver.net/index.php?page=mob_db&mvp=1&mob_search=Search&sort_r=0)
- [Ragnarok Online Graphical Bytes - Cutting Room Floor](https://tcrf.net/Ragnarok_Online/Graphical_Bytes)
- [Ragnarok Online UI Redux - DeviantArt](https://www.deviantart.com/eidrian/art/Ragnarok-Online-UI-Redux-142666449)
- [Ragnarok Online UI Elements - Dribbble](https://dribbble.com/shots/20537412-Ragnarok-Online-UI-Elements)
- [RO UI Revamp - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/User_blog:ZeroTigress/RO_UI_Revamp)
- [Ragnarok Online Concept Art - Creative Uncut](https://www.creativeuncut.com/art_ragnarok-online_a.html)
- [Damage Numbers - Spriters Resource](https://www.spriters-resource.com/pc_computer/ragnarokonline/asset/42205/)
- [High-Res Scrolling Damage Sprites](https://github.com/eleriaqueen/rag-highres-scrolling-dmg-sprites)
- [Damage Number Font Discussion - rAthena](https://rathena.org/board/topic/105296-how-do-i-edit-the-damage-number-font/)
- [Damage Numbers in RPGs - Medium](https://shweep.medium.com/damage-numbers-in-rpgs-1f0e3b1bc23a)
- [Emote - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Emote)
- [71 Essential Emotes - Online Game Commands](https://www.onlinegamecommands.com/ragnarok-emoticons/)
- [RO Emotion Sprite List](https://nn.ai4rei.net/dev/emolist/)
- [Emotes - iRO Wiki Classic](https://irowiki.org/classic/Emotes)
- [Emotes - Spriters Resource](https://www.spriters-resource.com/pc_computer/ragnarokonline/asset/127425/)
- [Prontera - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Prontera)
- [Glastheim - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Glastheim)
- [Cities, Towns and Places - Philippine RO Wiki](https://pro.fandom.com/wiki/Cities,_Towns_and_Places)
- [Sprite Progression in RO - Continuing World](https://tsuzukusekai.wordpress.com/2010/12/13/sprite-progression-in-ragnarok-online-and-how-3rd-jobs-ruined-it/)
- [Lee Myung-jin - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Lee_Myung-jin)
- [Ragnarok (manhwa) - Wikipedia](https://en.wikipedia.org/wiki/Ragnarok_(manhwa))
- [Lee Myung-jin - Lambiek Comiclopedia](https://www.lambiek.net/artists/l/lee-myung-jin.htm)
- [Cast Circle Discussion - Forsaken RO Forums](https://forum.forsaken-ro.net/topic/30444-cast-circle-wizard/)
- [Classic Cast Circle - RetroRefugees](https://retrorefugees.forumotion.com/t27-classic-cast-circle)
- [Effects Database - CairoLee Ragnarok Tools](https://github.com/CairoLee/Ragnarok.Tools/blob/master/ClientTools/browedit/data/effects.txt)
- [iRO Wiki - Status Effects](https://irowiki.org/wiki/Status_Effects)

### Project Documentation Sources
- `C:/Sabri_MMO/RagnaCloneDocs/10_Art_Animation_VFX_Pipeline.md` -- Primary art/animation/VFX reference (1,320 lines)
- `C:/Sabri_MMO/RagnaCloneDocs/Implementation/14_Art_Audio_Pipeline.md` -- UE5 C++ implementation patterns (~800 lines)
