# 10 -- Art, Animation, and VFX Pipeline

## Ragnarok Online Classic 3D Replica -- Sabri_MMO

**Document Version**: 1.0
**Date**: 2026-03-08
**Engine**: Unreal Engine 5.7 (C++ + Blueprints)
**Target Aesthetic**: Anime-inspired 3D (Genshin Impact / Tree of Savior register)
**Rendering**: PBR base + cel-shading post-process + custom outline pass

---

## Table of Contents

1. [Visual Style](#1-visual-style)
2. [Character Models](#2-character-models)
3. [Monster Models](#3-monster-models)
4. [Animation System](#4-animation-system)
5. [Environment Art](#5-environment-art)
6. [VFX -- Visual Effects](#6-vfx----visual-effects)
7. [UI Art](#7-ui-art)
8. [UE5 Asset Pipeline](#8-ue5-asset-pipeline)
9. [Tools and Software](#9-tools-and-software)
10. [Phased Production Plan](#10-phased-production-plan)

---

## 1. Visual Style

### 1.1 Design Philosophy

Ragnarok Online's original art style is defined by Myung-Jin Lee's manhwa illustrations -- soft watercolor palettes, whimsical character proportions (large heads, expressive eyes, compact bodies), and a world that feels like a storybook brought to life. Translating this into 3D requires deliberate choices to preserve that warmth while leveraging modern rendering.

**Target reference titles** (in order of relevance):
- **Tree of Savior** -- Same spiritual lineage (same original creator), 2.5D-to-3D transition, hand-painted textures, warm color grading
- **Genshin Impact** -- Anime cel-shading on PBR base, strong silhouettes, painterly environments, global illumination with stylized lighting
- **Blue Protocol** -- Anime rendering pipeline, bright saturated colors, action MMORPG context
- **Ni no Kuni** -- Studio Ghibli-inspired cel-shading, soft shadows, watercolor-adjacent rendering

### 1.2 Rendering Approach

The rendering pipeline combines PBR (Physically Based Rendering) as a foundation with stylized post-processing layered on top. This "PBR + NPR hybrid" approach gives artists physically correct lighting behavior while maintaining the anime aesthetic.

#### Material Layer Stack

```
Layer 4:  Post-Process Outline (Sobel edge detection on depth + normal buffers)
Layer 3:  Color Grading LUT (warm shift, saturation boost, soft contrast curve)
Layer 2:  Cel-Shading Pass (2-3 band toon ramp on diffuse, soft specular clamp)
Layer 1:  PBR Base (albedo, normal, roughness/metallic -- NO emissive abuse)
```

#### Cel-Shading Implementation Options (UE5)

| Approach | Pros | Cons | Recommendation |
|----------|------|------|----------------|
| **Post-Process Material** | Easy to iterate, project-wide, no per-material work | Less per-material control, performance cost on full screen | Use for global toon ramp + outlines |
| **Custom Shading Model (engine fork)** | Best quality, per-material control, Nanite compatible | Requires engine source build, harder to maintain across UE versions | Consider for production if team grows |
| **Material Function injection** | Per-material control, no engine fork | Must apply to every material, can break Nanite | Use for hero characters + key props |
| **Marketplace SRS (Stylized Rendering System)** | Production-ready, UE5.6+ compatible, Nanite support | Cost, dependency on third party | Good prototyping option |

**Recommended starting approach**: Post-Process Material for global cel-shading ramp + Sobel outline, combined with Material Function overrides on hero characters for finer control. Migrate to custom shading model only if the project scales beyond solo development.

#### Outline Shader Specifications

```
Outline Source:       Depth buffer + Normal buffer (Sobel filter)
Outline Color:        Per-zone configurable (black default, dark brown for towns, dark blue for night)
Outline Width:        1.5-2.5 pixels at 1080p (scale with resolution)
Depth Threshold:      Tunable per biome (tighter in towns, looser in open fields)
Normal Threshold:     Moderate (catch creases, ignore smooth surfaces)
Inner Lines:          Optional per-material (fabric folds, armor plates)
```

### 1.3 Color Palette Guidelines

RO's palette is distinctive -- saturated but not neon, warm but not orange-heavy. Each zone type has a palette anchor.

| Zone Type | Palette Anchor | Key Hues | Saturation | Value Range | Example |
|-----------|---------------|----------|------------|-------------|---------|
| Medieval Town | Warm stone | Ochre, cream, forest green, sky blue | Medium (55-70%) | Bright (mid-high) | Prontera, Izlude |
| Asian Town | Cherry blossom | Rose, ivory, deep green, slate blue | Medium-High (60-75%) | Medium | Payon, Amatsu |
| Desert Town | Sand and heat | Amber, burnt sienna, terracotta, teal accent | Medium (50-65%) | High (sun-washed) | Morroc, Comodo |
| Snow/Nordic | Frost and pine | Ice blue, evergreen, charcoal, silver | Low-Medium (35-55%) | High (snow reflection) | Lutie, Rachel |
| Dark/Gothic | Shadow and stone | Slate, crimson accent, deep purple, bone white | Low (30-50%) | Low-Medium | Glast Heim, Nifflheim |
| Magic/Academic | Arcane blue | Royal blue, gold, purple, white marble | Medium-High (60-80%) | Medium | Geffen, Juno |
| Tropical | Ocean and green | Turquoise, palm green, coral, sand | High (70-85%) | Medium-High | Alberta, Comodo |

#### Day/Night Lighting Per Zone Type

| Time | Global Light Color | Intensity Multiplier | Shadow Softness | Sky Color | Post-Process Adjustment |
|------|-------------------|---------------------|-----------------|-----------|------------------------|
| Dawn (5:00-7:00) | Warm orange-pink (#FFB088) | 0.6 | Very soft | Gradient: purple to peach | Slight warm tint, low contrast |
| Morning (7:00-10:00) | Warm white (#FFF5E1) | 0.85 | Medium-soft | Clear blue with warm horizon | Neutral, slight saturation boost |
| Midday (10:00-14:00) | Pure white (#FFFFFF) | 1.0 | Medium | Bright sky blue | Neutral |
| Afternoon (14:00-17:00) | Warm yellow (#FFF0C0) | 0.9 | Medium-soft | Deep blue with golden horizon | Warm shift |
| Dusk (17:00-19:00) | Deep orange-red (#FF8855) | 0.5 | Very soft | Gradient: orange to deep blue | Strong warm tint, contrast increase |
| Night (19:00-5:00) | Cool blue (#6688CC) | 0.25 | Soft | Dark blue-black with stars | Blue tint, desaturate 30%, contrast increase |

**Indoor zones** (dungeons, buildings): Fixed lighting, no day/night cycle. Each dungeon has its own mood lighting preset.

---

## 2. Character Models

### 2.1 Base Body Architecture

All player characters share one of two base meshes: **Male** and **Female**. These are the "mannequins" onto which all class outfits, hair, and headgear are layered.

#### Proportions

RO characters are NOT realistic proportions. They follow a chibi-adjacent "anime game" ratio:

| Attribute | Specification | Reference |
|-----------|--------------|-----------|
| Head-to-body ratio | ~1:5.5 (slightly taller than chibi, shorter than realistic) | Between Genshin "short" and "medium" body types |
| Eye size | Large, expressive, ~30% of face height | Standard anime proportions |
| Hand size | Slightly oversized for weapon readability | Critical for weapon silhouettes |
| Foot size | Slightly oversized for grounding | Prevents "floating" feel |
| Body type | Lean athletic, no extreme musculature | Even Swordsman class is lean, not bulky |
| Overall height | ~160cm for male base, ~152cm for female base (in-world scale) | Scaled to UE5 units: 160/152 UU |

#### Skeletal Hierarchy

A single master skeleton is used for ALL player characters, enabling animation sharing across classes.

```
Root
 +-- Pelvis
      +-- Spine_01
           +-- Spine_02
                +-- Spine_03
                     +-- Neck
                          +-- Head
                               +-- Hair_Root (dynamic chain: 3-5 bones per strand)
                               +-- HeadTop_Socket (headgear top)
                               +-- HeadMid_Socket (headgear mid -- glasses, masks)
                               +-- HeadLow_Socket (headgear lower -- mouth items)
                     +-- Clavicle_L / Clavicle_R
                          +-- UpperArm_L / UpperArm_R
                               +-- LowerArm_L / LowerArm_R
                                    +-- Hand_L / Hand_R
                                         +-- Weapon_L_Socket / Weapon_R_Socket
                                         +-- Finger bones (3 per finger, optional for LOD)
           +-- Thigh_L / Thigh_R
                +-- Calf_L / Calf_R
                     +-- Foot_L / Foot_R
                          +-- Toe_L / Toe_R
      +-- Shield_Socket (left back)
      +-- Back_Socket (quiver, backpack, wings)
      +-- Cape_Root (dynamic chain: 5-8 bones, cloth sim)
```

**Total bone count target**: 65-75 bones (base skeleton without hair/cape dynamics). This keeps GPU skinning costs reasonable for 50+ visible characters.

#### Modular Equipment System

UE5 supports modular characters through three approaches. For this project, we use **Master Pose Component** as the primary method:

```
BP_MMOCharacter (Actor)
 +-- SK_Body (SkeletalMeshComponent -- Master)     [Base body mesh, always visible]
 +-- SK_Outfit (SkeletalMeshComponent -- Child)     [Class outfit, Master Pose from SK_Body]
 +-- SK_Hair (SkeletalMeshComponent -- Child)        [Hair mesh, Master Pose from SK_Body]
 +-- SM_HeadTop (StaticMeshComponent)                [Headgear top slot, attached to HeadTop_Socket]
 +-- SM_HeadMid (StaticMeshComponent)                [Headgear mid slot, attached to HeadMid_Socket]
 +-- SM_HeadLow (StaticMeshComponent)                [Headgear lower slot, attached to HeadLow_Socket]
 +-- SM_WeaponR (StaticMeshComponent)                [Right hand weapon, attached to Weapon_R_Socket]
 +-- SM_WeaponL (StaticMeshComponent)                [Left hand weapon/shield, attached to Weapon_L_Socket]
 +-- SM_Back (StaticMeshComponent)                   [Back item, attached to Back_Socket]
```

**Master Pose Component** means child skeletal meshes copy the master's bone transforms each frame without running their own animation -- reducing game thread cost significantly when 50+ players are on screen.

### 2.2 Hair System

#### Hair Styles

RO Classic supports 19 hair styles for male and 19 for female (numbered 1-19). Some private servers extend to 30+. For this project, target **24 styles per gender** (19 classic + 5 bonus).

| Style ID | Male Description | Female Description | Bone Chain Count |
|----------|-----------------|-------------------|-----------------|
| 1 | Short spiky | Short bob | 0 (rigid) |
| 2 | Medium parted | Shoulder-length straight | 2 |
| 3 | Slicked back | Twin tails | 3 (per tail) |
| 4 | Long tied back | Long flowing | 4 |
| 5 | Mohawk-adjacent | Side ponytail | 2 |
| 6 | Bowl cut | Braided crown | 0 (rigid) |
| 7 | Short messy | Short pixie | 0 (rigid) |
| 8 | Long wild | Long wavy | 5 |
| 9 | Bandana-covered | Bun with strands | 2 |
| 10 | Crew cut | Bob with bangs | 0 (rigid) |
| 11 | Samurai topknot | Hime cut | 3 |
| 12 | Spiked up | Twintails short | 2 (per tail) |
| 13 | Shaggy medium | Side braid | 3 |
| 14 | Pompadour | Curly shoulder | 2 |
| 15 | Ponytail | Long straight with ribbon | 4 |
| 16 | Buzz cut | Messy short | 0 (rigid) |
| 17 | Long straight | Odango (buns) | 0 (rigid) |
| 18 | Wild spikes | French braid | 3 |
| 19 | Afro/Volume | Drill curls | 2 (per drill) |
| 20-24 | Bonus: varies | Bonus: varies | Varies |

#### Hair Colors

RO Classic has **9 hair color palettes** (IDs 0-8). Each color is applied via a **Material Instance Dynamic** that swaps the hair color parameter.

| Color ID | Name | Base Color (sRGB) | Highlight | Shadow |
|----------|------|-------------------|-----------|--------|
| 0 | Black | #1A1A2E | #3A3A4E | #0A0A1E |
| 1 | Red-Brown | #8B4513 | #A0522D | #5C2E0E |
| 2 | Sandy Blonde | #D4A574 | #E8C49A | #A07848 |
| 3 | Chestnut | #6B3A2A | #8B5A3A | #4B1A0A |
| 4 | Gray | #9E9E9E | #BEBEBE | #6E6E6E |
| 5 | Purple | #6A0DAD | #8B2FC0 | #4A0080 |
| 6 | Orange | #E06020 | #FF8040 | #B04010 |
| 7 | Green | #2E8B57 | #3CB371 | #1E6B37 |
| 8 | Blue | #2060C0 | #4080E0 | #104090 |

**Implementation**: One master hair material `M_Hair_Master` with parameters for BaseColor, HighlightColor, ShadowColor, SpecularShift. Create 9 Material Instances, one per color ID. At runtime, `SetMaterial()` swaps the instance on the hair mesh.

### 2.3 Class-Specific Outfits

Every class change in RO gives the character a new visual appearance. Each outfit is a separate Skeletal Mesh that replaces SK_Outfit on the character.

#### First Job Classes (6 outfits x 2 genders = 12 meshes)

| Class | Male Visual Key Features | Female Visual Key Features | Priority |
|-------|-------------------------|---------------------------|----------|
| **Novice** | Simple tunic, shorts, basic boots | Simple dress, short boots | P0 (ship with base) |
| **Swordsman** | Light armor chest plate, arm guards, greaves, cape | Light armor corset, arm guards, short skirt, cape | P1 |
| **Mage** | Long robe, pointed hat (optional), staff holster | Long robe with sash, circlet, staff holster | P1 |
| **Archer** | Leather vest, arm guard (left), quiver (back socket) | Leather halter top, arm guard, quiver | P1 |
| **Acolyte** | Religious robes, rope belt, sandals | Religious robes with hood option, sandals | P1 |
| **Merchant** | Heavy apron, pouches, thick boots, cart handle (back) | Apron dress, pouches, thick boots, cart handle | P1 |
| **Thief** | Tight dark clothing, scarf/mask, light shoes | Tight dark clothing, short cape, light shoes | P1 |

#### Second Job Classes (14 outfits x 2 genders = 28 meshes)

| Class | Source | Visual Notes | Priority |
|-------|--------|-------------|----------|
| **Knight** | Swordsman | Full plate segments, helmet visor, long cape | P2 |
| **Crusader** | Swordsman | Holy plate, cross motifs, shield prominence, shorter cape | P2 |
| **Wizard** | Mage | Elaborate robes, larger hat, floating elements (VFX overlay) | P2 |
| **Sage** | Mage | Academic robes, book holster, spectacles option | P2 |
| **Hunter** | Archer | Ranger leather, larger quiver, falcon perch (shoulder socket) | P2 |
| **Bard** | Archer | Minstrel outfit, instrument holster (back socket) | P3 |
| **Dancer** | Archer | Performance outfit, whip holster | P3 |
| **Priest** | Acolyte | Ornate vestments, holy symbol, longer robes | P2 |
| **Monk** | Acolyte | Fighting gi, knuckle wraps, shaved/short hair override | P2 |
| **Blacksmith** | Merchant | Heavy leather apron, hammer holster, goggles (HeadMid) | P2 |
| **Alchemist** | Merchant | Lab coat, potion belt, goggles (HeadMid) | P3 |
| **Assassin** | Thief | Full dark bodysuit, dual weapon sheaths, face scarf | P2 |
| **Rogue** | Thief | Medium armor, dagger and bow dual holster, rakish appearance | P3 |

#### Transcendent Classes (14 outfits x 2 genders = 28 meshes)

Transcendent classes are visually enhanced versions of 2nd job classes. They share the same silhouette but with upgraded detail, metallic accents, glowing trim, and aura effects.

| Transcendent Class | Base Class | Visual Upgrade Notes | Priority |
|-------------------|-----------|---------------------|----------|
| **Lord Knight** | Knight | Gold-trimmed armor, more ornate helmet, flowing cape | P4 |
| **Paladin** | Crusader | White-gold holy armor, larger shield, radiant trim | P4 |
| **High Wizard** | Wizard | Darker robes with arcane runes, floating crystals (VFX) | P4 |
| **Scholar** | Sage | Professor attire, enchanted book, more refined | P4 |
| **Sniper** | Hunter | Camouflage elements, tactical quiver, falcon upgrade | P4 |
| **Clown** | Bard | Elaborate minstrel costume, larger instrument | P5 |
| **Gypsy** | Dancer | More ornate performance outfit, exotic accessories | P5 |
| **High Priest** | Priest | Grand vestments, halo VFX overlay, golden staff | P4 |
| **Champion** | Monk | Enhanced fighting outfit, spiritual aura VFX | P4 |
| **Whitesmith** | Blacksmith | Master craftsman outfit, glowing forge tools | P4 |
| **Creator** | Alchemist | Advanced lab gear, homunculus flask (back socket) | P5 |
| **Assassin Cross** | Assassin | Elite dark outfit, poison trail VFX, dual katars default | P4 |
| **Stalker** | Rogue | Shadow outfit, disguise elements | P5 |

#### Total Outfit Count Summary

| Category | Meshes per Gender | Total (M + F) |
|----------|------------------|---------------|
| Novice | 1 | 2 |
| 1st Job | 6 | 12 |
| 2nd Job | 14 | 28 |
| Transcendent | 14 | 28 |
| **Grand Total** | **35** | **70** |

### 2.4 Headgear System

RO has 1000+ headgear items displayed on characters. These occupy three slots:

| Slot | Socket | Examples | Mesh Type |
|------|--------|----------|-----------|
| **Upper** (Top) | HeadTop_Socket | Helmets, hats, tiaras, horns, angel wings (head-mounted) | StaticMesh |
| **Middle** (Mid) | HeadMid_Socket | Glasses, monocles, masks (eye-level), sunglasses | StaticMesh |
| **Lower** (Low) | HeadLow_Socket | Scarves, pipe, lollipop, masks (mouth-level), beard | StaticMesh |

#### Production Strategy for 1000+ Headgears

Creating 1000+ headgear models is the single largest individual art task. Production must be phased and systematized.

**Phase 1 (50 headgears)**: Iconic, universally recognized items
- Angel Wing, Helm, Cap, Crown, Wizard Hat, Nurse Cap, Bunny Band, Kitty Band, Apple of Archer, Magician Hat, Sakkat, Munak Hat, Deviruchi Hat, Poring Hat, Beret, Ribbon, Flower, Sunglasses, Monocle, Opera Mask, Gangster Mask, Pipe, Lollipop, Cigarette, Romantic Flower, Romantic Leaf, Ghost Bandana, Hockey Mask, Mr. Smile, Smiley Mask, Welding Mask, Gas Mask, Goblin Mask, Masquerade, Phantom of the Opera, Magestic Goat, Grand Circlet, Tiara, Corsair, Halo, Evil Wing, Fallen Angel Wing, Dark Blinker, Elven Ears, Demon Horns, Feather Beret, Poo Poo Hat, Mini Propeller, Yellow Bandana, Ninja Mask

**Phase 2 (100 headgears)**: Class-defining and popular PvP/WoE items

**Phase 3 (200 headgears)**: Quest reward and mid-game items

**Phase 4 (300+ headgears)**: Long-tail completionist items, seasonal, event-exclusive

**Per-headgear specs**:
- Triangle count: 200-2000 tris depending on complexity
- Single material instance (from M_Equipment_Master)
- No LODs needed (small screen presence)
- Naming: `SM_Headgear_{SlotPrefix}_{ItemID}_{Name}` (e.g., `SM_Headgear_Top_5027_AngelWing`)

### 2.5 Equipment Visibility

Beyond headgear, the following equipment types have visual representation:

| Equipment Slot | Visibility | Implementation |
|---------------|-----------|----------------|
| Weapon (R hand) | Always visible when equipped | StaticMesh on Weapon_R_Socket, per-weapon-type mesh |
| Shield (L hand) | Visible for classes that use shields | StaticMesh on Weapon_L_Socket |
| Garment/Cape | Visible (some capes, mantles) | SkeletalMesh child with cloth physics, or StaticMesh |
| Armor | Outfit color tint only (RO style) | Material parameter swap on outfit mesh |
| Footgear | NOT individually visible (part of outfit) | -- |
| Accessories | NOT visible on character (RO classic behavior) | -- |

#### Weapon Mesh Count Estimates

| Weapon Type | Classes | Mesh Varieties (Phase 1) | Total Target |
|-------------|---------|------------------------|--------------|
| Sword (1H) | Swordsman, Knight | 10 | 30 |
| Sword (2H) | Knight, Crusader | 8 | 25 |
| Spear | Knight, Crusader | 6 | 20 |
| Dagger | Thief, Assassin, Mage, Rogue | 8 | 25 |
| Katar | Assassin | 6 | 20 |
| Rod/Staff | Mage, Wizard, Sage, Priest | 8 | 25 |
| Mace | Acolyte, Priest, Monk | 6 | 20 |
| Bow | Archer, Hunter, Bard/Dancer | 8 | 25 |
| Axe | Merchant, Blacksmith | 6 | 20 |
| Knuckle | Monk | 5 | 15 |
| Book | Sage | 4 | 12 |
| Instrument | Bard | 3 | 8 |
| Whip | Dancer | 3 | 8 |
| Shield | Knight, Crusader, Swordsman | 8 | 25 |
| **Total** | -- | **89** | **278** |

---

## 3. Monster Models

### 3.1 Size Categories

RO monsters come in three gameplay-relevant sizes that affect weapon damage modifiers. In 3D, size determines base mesh scale, animation speed, and visual weight.

| Size | RO Scale | UE5 World Scale | Typical Bone Count | Triangle Budget | Example Monsters |
|------|----------|-----------------|-------------------|-----------------|-----------------|
| **Small** | 0.5-0.8x player | 50-130 cm tall | 15-25 | 1,000-3,000 tris | Poring, Drops, Lunatic, Fabre, Rocker, Picky |
| **Medium** | 0.8-1.5x player | 130-250 cm tall | 25-45 | 3,000-8,000 tris | Golem, Orc Warrior, Kobold, Skeleton, Zombie |
| **Large** | 1.5-4.0x player | 250-700 cm tall | 35-60 | 8,000-20,000 tris | Drake, Baphomet Jr, Eddga, Stormy Knight |
| **Boss/MVP** | 3.0-8.0x player | 500-1500 cm tall | 45-80 | 15,000-40,000 tris | Baphomet, Orc Lord, Mistress, Moonlight Flower |

### 3.2 Prioritized Monster Production List

Monsters are prioritized by: (A) spawn frequency in zones 1-3, (B) iconic recognition, (C) mechanical necessity. The server already has 509 monster templates loaded from rAthena.

#### Priority Tier 1: First 30 Monsters (Zones 1-2, Novice/1st Job)

| # | Monster | Size | Element | Zone(s) | Visual Key |
|---|---------|------|---------|---------|-----------|
| 1 | Poring | S | Water | Prontera fields | Pink slime blob, cute face, bouncing idle |
| 2 | Lunatic | S | Neutral | Prontera fields | White rabbit, red eyes |
| 3 | Fabre | S | Earth | Prontera fields | Green caterpillar |
| 4 | Pupa | S | Earth | Prontera fields | Brown cocoon, static |
| 5 | Drops | S | Fire | Prontera fields | Orange slime blob (Poring variant) |
| 6 | Poporing | S | Poison | Payon/Geffen fields | Green slime blob (Poring variant) |
| 7 | Picky | S | Fire | Prontera fields | Small yellow bird |
| 8 | Chonchon | S | Wind | Prontera fields | Flying insect, buzzing wings |
| 9 | Roda Frog | S | Water | Prontera fields | Cartoon frog |
| 10 | Willow | S | Earth | Payon fields | Animated tree stump |
| 11 | Condor | S | Wind | Morroc fields | Desert vulture |
| 12 | Peco Peco | M | Fire | Morroc fields | Large riding bird (chocobo-esque) |
| 13 | Skeleton | M | Undead | Payon Cave | Classic skeleton warrior |
| 14 | Zombie | M | Undead | Payon Cave | Shambling undead |
| 15 | Archer Skeleton | M | Undead | Payon Cave | Skeleton with bow |
| 16 | Rocker | S | Earth | Prontera fields | Musical insect with violin |
| 17 | Thief Bug | S | Dark | Prontera Culverts | Dark beetle |
| 18 | Thief Bug Egg | S | Dark | Prontera Culverts | Pulsing egg sac |
| 19 | Spore | S | Water | Payon fields | Walking mushroom |
| 20 | Poison Spore | S | Poison | Payon fields | Purple mushroom variant |
| 21 | Wolf | M | Earth | Payon fields | Gray wolf |
| 22 | Snake (Boa) | M | Earth | Morroc fields | Desert serpent |
| 23 | Scorpion | M | Fire | Morroc fields | Desert scorpion |
| 24 | Steel Chonchon | S | Wind | Geffen fields | Metal insect variant |
| 25 | Goblin | M | Poison | Geffen fields | Small green humanoid |
| 26 | Orc Warrior | M | Earth | Orc Dungeon | Brutish green humanoid |
| 27 | Andre | S | Earth | Ant Hell | Large ant |
| 28 | Deniro | S | Earth | Ant Hell | Brown ant variant |
| 29 | Piere | S | Earth | Ant Hell | Red ant variant |
| 30 | Creamy | S | Wind | Prontera/Geffen fields | Butterfly fairy |

#### Priority Tier 2: Next 30 Monsters (Zones 2-3, Mid 1st Job)

| # | Monster | Size | Key Visual Feature |
|---|---------|------|--------------------|
| 31 | Muka | M | Cactus creature |
| 32 | Hornet | S | Flying wasp |
| 33 | Mandragora | S | Plant with face |
| 34 | Magnolia | M | Zombie woman |
| 35 | Sohee | M | Korean ghost girl |
| 36 | Munak | M | Chinese zombie |
| 37 | Bongun | M | Chinese zombie warrior |
| 38 | Hydra | S | Stationary tentacle plant |
| 39 | Marina | S | Jellyfish |
| 40 | Vadon | S | Crab |
| 41 | Cornutus | S | Hermit crab |
| 42 | Marc | M | Fish/seal |
| 43 | Marse | S | Seahorse |
| 44 | Golem | L | Stone construct |
| 45 | Whisper | S | Floating ghost |
| 46 | Nightmare | M | Demonic horse |
| 47 | Mummy | M | Bandaged undead |
| 48 | Verit | S | Mummified dog |
| 49 | Ghoul | M | Large zombie |
| 50 | Isis | M | Snake-woman (Naga) |
| 51 | Pasana | M | Desert warrior |
| 52 | Minorous | L | Minotaur |
| 53 | Marduk | M | Egyptian priest |
| 54 | Requiem | M | Zombie soldier |
| 55 | Matyr | M | Dark wolf |
| 56 | Savage | M | Boar |
| 57 | Bigfoot | L | Bear |
| 58 | Angeling | M | Angel Poring (mini-boss) |
| 59 | Deviling | M | Devil Poring (mini-boss) |
| 60 | Golden Thief Bug | L | Giant golden beetle (mini-boss) |

#### Priority Tier 3: Next 40 Monsters (Dungeons, 2nd Job, MVPs)

Includes: Bathory, Joker, Injustice, Raydric, Khalitzburg, Abysmal Knight, Penomena, Alarm, Clock, High Orc, Orc Hero (MVP), Orc Lord (MVP), Mistress (MVP), Baphomet (MVP), Baphomet Jr, Moonlight Flower (MVP), Drake (MVP), Stormy Knight (MVP), Eddga (MVP), Phreeoni (MVP), Maya (MVP), Osiris (MVP), Doppelganger (MVP), and key dungeon residents for Glast Heim, Clock Tower, Sphinx, and Geffen Dungeon.

### 3.3 Monster Design Translation Guidelines

When converting RO's 2D sprites to 3D models, follow these rules:

1. **Preserve silhouette** -- The RO sprite silhouette at 45 degrees IS the character. Model to match that silhouette exactly.
2. **Maintain color accuracy** -- RO monsters have distinctive, recognizable color palettes. Poring is THAT specific pink. Do not "reimagine" colors.
3. **Add depth, not detail** -- 3D models should feel like the 2D sprite inflated into 3D, not like a photorealistic reinterpretation. Keep surfaces relatively smooth.
4. **Eyes are character** -- Most RO monsters have distinctive eye designs. These must be prominent and expressive.
5. **Scale relationships matter** -- A Poring next to a Baphomet must have the same relative scale ratio as in the original game.
6. **Retain the "cute factor"** -- Even aggressive monsters like Orc Warriors have a certain cartoon charm. Do not make them grimdark.

---

## 4. Animation System

### 4.1 UE5 Animation Architecture

The animation system uses UE5's standard pipeline: Skeletal Meshes, Animation Blueprints (AnimBP), State Machines, Blend Spaces, Animation Montages, and Anim Notify events.

```
                          ┌───────────────────────────────────┐
                          │       ABP_PlayerCharacter          │
                          │       (Animation Blueprint)        │
                          ├───────────────────────────────────┤
                          │  EventGraph:                       │
                          │    • Read speed, direction, state  │
                          │    • Update bools (IsMoving, etc.) │
                          ├───────────────────────────────────┤
                          │  AnimGraph:                        │
                          │    ┌──────────────┐               │
                          │    │ Locomotion   │◄── BlendSpace  │
                          │    │ StateMachine │    (Speed,Dir) │
                          │    └──────┬───────┘               │
                          │           │                        │
                          │    ┌──────▼───────┐               │
                          │    │  Upper Body  │◄── Layered     │
                          │    │  Override    │    Blend per   │
                          │    │  Slot       │    Bone         │
                          │    └──────┬───────┘               │
                          │           │                        │
                          │    ┌──────▼───────┐               │
                          │    │  Montage     │◄── Attack,     │
                          │    │  Slot       │    Cast, Emote  │
                          │    └─────────────┘               │
                          └───────────────────────────────────┘
```

### 4.2 Character Animations

#### Locomotion Animations (Shared by all classes)

| Animation | Frames (30fps) | Loop | Blend Space Axis | Notes |
|-----------|---------------|------|-------------------|-------|
| Idle | 60-90 | Yes | Speed = 0 | Subtle breathing, weight shift |
| Walk | 30 | Yes | Speed = 0-300 | Casual pace, RO walk speed |
| Run | 24 | Yes | Speed = 300-600 | Faster gait, more forward lean |
| Sprint | 20 | Yes | Speed = 600+ | For movement speed buffs (AGI UP, etc.) |
| Idle_Combat | 60-90 | Yes | In combat stance | Weapon raised, ready position |
| Strafe_L/R | 30 | Yes | Direction blend | Side movement while facing target |
| Walk_Back | 30 | Yes | Direction blend | Backward movement while facing target |

**Blend Space**: `BS_Locomotion` -- 2D blend space with Speed (X) and Direction (Y, -180 to 180).

#### Combat Animations (Per weapon type)

Each weapon type has its own attack animation set. These play as **Animation Montages** triggered by server combat events.

| Weapon Type | Attack Variants | Combo Chain? | ASPD Scale Range | Classes |
|-------------|---------------|-------------|-----------------|---------|
| Unarmed | 2 (punch L/R) | No | 0.5-2.0x | Novice, Monk |
| Sword_1H | 3 (slash, thrust, overhead) | Yes (3-hit) | 0.5-2.0x | Swordsman, Knight |
| Sword_2H | 2 (wide slash, overhead) | Yes (2-hit) | 0.6-2.0x | Knight |
| Dagger | 3 (stab, slash, backhand) | Yes (3-hit) | 0.4-1.5x | Thief, Assassin |
| Katar | 2 (cross slash, thrust) | Yes (2-hit) | 0.3-1.2x | Assassin |
| Spear | 2 (thrust, sweep) | No | 0.6-2.0x | Knight, Crusader |
| Bow | 1 (draw and release) | No | 0.5-2.0x | Archer, Hunter |
| Staff | 2 (swing, thrust) | No | 0.7-2.0x | Mage, Wizard |
| Mace | 2 (overhead, side swing) | No | 0.6-2.0x | Acolyte, Priest |
| Axe | 2 (chop, wide swing) | No | 0.6-2.0x | Merchant, Blacksmith |
| Knuckle | 3 (jab, hook, uppercut) | Yes (3-hit) | 0.3-1.2x | Monk |
| Book | 1 (slam) | No | 0.6-2.0x | Sage |
| Instrument | 1 (strum strike) | No | 0.6-2.0x | Bard |
| Whip | 1 (crack) | No | 0.6-2.0x | Dancer |

**ASPD Integration**: The server sends `attackMotion` (in ms) with each attack event. The client plays the attack montage with `PlayRate = BaseAnimDuration / (attackMotion / 1000.0)`. This means faster ASPD = faster animation playback.

#### Status Animations

| Animation | Type | Loop | Trigger |
|-----------|------|------|---------|
| Hit_React_Light | Montage | No | `combat:damage` with small damage |
| Hit_React_Heavy | Montage | No | `combat:damage` with large damage (>20% HP) |
| Death | Montage | No | HP reaches 0 |
| Death_Idle | Pose | Yes (hold) | After death montage completes, hold until respawn |
| Sit | Full body | Yes | Player /sit command |
| Stand_From_Sit | Transition | No | Player stands after sitting |
| Cast_Start | Upper body | No | `skill:cast_start` event |
| Cast_Loop | Upper body | Yes | During cast bar duration |
| Cast_End | Upper body | No | `skill:cast_complete` event |
| Stun | Full body | Yes | Stun status effect |
| Frozen | Pose | Yes (hold) | Frost Diver freeze |

#### Emote Animations

| Emote | Animation Length | Loop | Chat Command |
|-------|----------------|------|-------------|
| Greeting | 2s | No | /hi |
| Nod | 1s | No | /nod |
| Shake Head | 1s | No | /no |
| Cheer | 2.5s | No | /cheer |
| Cry | 3s | Yes (until cancelled) | /cry |
| Laugh | 2s | No | /laugh |
| Anger | 2s | No | /anger |
| Confusion | 1.5s | No | /hmm |
| Heart | 1.5s | No | /love |
| Rock Paper Scissors | 2s | No | /rps |

### 4.3 Monster Animations

Each monster needs a minimum animation set. The bone count and complexity varies by size category.

#### Required Animations Per Monster

| Animation | Required? | Notes |
|-----------|----------|-------|
| Idle | YES | Primary state, must be charming/characteristic |
| Walk | YES | Wandering state, speed must match server `walkSpeed` |
| Attack | YES | At least 1 attack animation |
| Hit_React | YES | Damage received flinch |
| Die | YES | Death animation, holds final frame until despawn |
| Spawn | Optional | Appear/materialize effect (can be VFX only) |
| Special_Attack | Optional | For boss/elite monsters with multiple attacks |
| Enrage | Optional | For bosses when HP drops below threshold |

#### Monster Animation Blueprint Structure

```
ABP_Monster_Base (Parent AnimBP)
 ├── StateMachine: MonsterLocomotion
 │    ├── State: Idle        ← plays idle loop
 │    ├── State: Walking     ← plays walk loop, speed-scaled
 │    ├── State: Attacking   ← plays attack montage
 │    ├── State: Hit         ← plays hit react, returns to previous
 │    └── State: Dead        ← plays death, holds final pose
 │
 └── Variables (set by BP_EnemyManager):
      ├── bIsMoving (bool)
      ├── MoveSpeed (float)
      ├── bIsAttacking (bool)
      ├── bIsHit (bool)
      └── bIsDead (bool)
```

**Child AnimBPs** (e.g., `ABP_Monster_Poring`, `ABP_Monster_Skeleton`) inherit from `ABP_Monster_Base` and override state animations with monster-specific clips. The state machine logic remains identical.

### 4.4 IK (Inverse Kinematics)

| IK Type | Use Case | UE5 System | Priority |
|---------|----------|-----------|----------|
| Foot IK | Player characters on uneven terrain | Control Rig + IK Retargeter | P2 (after basic locomotion) |
| Hand IK | Two-handed weapon grips | Two Bone IK node in AnimBP | P2 |
| Look At IK | Head tracking toward target/camera | Look At node in AnimBP | P3 (polish) |
| Ground Adaptation | Monster feet on terrain | Foot placement via Control Rig | P3 |

### 4.5 Animation Montage Slots

| Slot | Purpose | Blend Mode |
|------|---------|-----------|
| DefaultSlot | Full-body overrides (death, emotes, sit) | Blend per Bone (full) |
| UpperBody | Upper-body overrides (attack, cast) while legs locomote | Blend per Bone (above spine_02) |
| Additive | Layered effects (breathing heavy, flinch while moving) | Additive |

### 4.6 Anim Notify Events

Animation Notifies trigger gameplay events at specific frames:

| Notify | Fires At | Purpose |
|--------|----------|---------|
| AN_AttackImpact | Frame where weapon connects | Trigger hit VFX, play hit SFX |
| AN_FootstepL / AN_FootstepR | Foot contact frames | Play footstep SFX, spawn dust VFX |
| AN_CastRelease | End of cast animation | Sync with skill VFX launch |
| AN_WeaponTrailStart / End | Swing arc | Enable/disable weapon trail VFX |
| AN_EnableCombo | During combo window | Allow input for next attack in chain |
| AN_ProjectileLaunch | Arrow/spell release frame | Spawn projectile actor |

---

## 5. Environment Art

### 5.1 Town Environments

RO has ~20 towns, each with a distinctive cultural theme. In 3D, these become full explorable environments.

#### Town Style Reference

| Town | Cultural Theme | Architectural Style | Key Landmarks | Palette |
|------|---------------|--------------------|----|---------|
| **Prontera** | Central European Medieval | Gothic cathedral, stone walls, cobblestone streets, marketplace stalls | Cathedral, fountain square, castle gates | Warm stone, green accents |
| **Izlude** | Mediterranean Coast | White-washed buildings, terracotta roofs, harbor docks | Arena, lighthouse, docks | Sky blue, white, terracotta |
| **Geffen** | Arcane European | Dark stone towers, magical crystals, arcane circles on ground | Magic tower (center), library | Deep blue, purple, gold |
| **Payon** | Korean Traditional | Hanok-style buildings, wooden bridges, bamboo, cherry blossoms | Palace, archery range, cave entrance | Rose, wood brown, green |
| **Morroc** | Arabian/Egyptian | Sandstone buildings, market tents, pyramid visible in distance | Pyramid entrance, destroyed quarter | Sand, terracotta, teal |
| **Alberta** | Nordic/Merchant Port | Wooden docks, merchant ships, warehouses, timber buildings | Harbor, merchant guild | Sea blue, timber brown |
| **Aldebaran** | Clockwork European | Clock tower, canals, mechanical elements, bridges | Clock Tower, canal system | Brass, stone gray, water blue |
| **Lutie** | Christmas/Nordic | Snow-covered cottages, candy cane decor, pine trees, toy shop | Toy Factory, giant Christmas tree | Snow white, red, green |
| **Amatsu** | Japanese Traditional | Torii gates, shoji screens, zen gardens, pagodas | Shrine, tatami rooms | Red, black, cherry pink |
| **Kunlun/Gonryun** | Chinese Traditional | Dragon pillars, curved roofs, floating islands, misty mountains | Dragon temple, cloud platforms | Red, gold, jade green |
| **Comodo** | Tropical Cave/Beach | Palm trees, tiki torches, cave entrances, waterfall, beach | Casino, beach, cave network | Turquoise, sand, torch orange |
| **Umbala** | Tribal/Treehouse | Massive trees, rope bridges, wooden platforms, jungle | World Tree base, treehouse village | Deep green, wood, earth |
| **Niflheim** | Norse Underworld | Dead trees, foggy, dark stone, ghostly lights | Dead Man's mansion, grey market | Gray, purple, sickly green |
| **Juno/Yuno** | Greek/Academic | Marble buildings, floating islands, libraries, columns | Sage Academy, Valkyrja temple | White marble, sky blue, gold |
| **Einbroch** | Industrial German | Smokestacks, metal buildings, railways, gear motifs | Train station, factory district | Iron gray, rust, smog |
| **Lighthalzen** | Modern Corporate | Glass buildings, clean streets, slum contrast, labs | Rekenber HQ, Slums | Steel blue, white, slum brown |
| **Rachel** | Religious/Arctic | White stone temple, cold architecture, religious iconography | Temple, Ice Cave entrance | Ice white, pale blue, gold |
| **Veins** | Volcanic/Desert | Red rock formations, heat haze, volcanic vents | Thor's Volcano entrance | Red, orange, black |
| **Hugel** | Alpine Bavarian | Timber-frame houses, mountain backdrop, meadows | Odin Temple entrance, brewery | Green, wood, mountain blue |
| **Brasilis** | Brazilian Tropical | Colorful buildings, carnival vibes, tropical plants | Beach, carnival area | Bright multicolor, green |

### 5.2 Field/Outdoor Environments

Fields are the large outdoor zones between towns where monsters roam. They need to be traversable and visually distinct.

#### Field Biome Types

| Biome | Terrain | Foliage | Props | Lighting | Example Maps |
|-------|---------|---------|-------|----------|-------------|
| **Grassland** | Rolling hills, dirt paths | Grass, wildflowers, scattered trees | Fences, signposts, wells | Bright daylight | Prontera fields |
| **Forest** | Gentle hills, dense canopy | Dense trees, undergrowth, mushrooms | Fallen logs, moss rocks | Dappled sun | Payon fields |
| **Desert** | Flat with dunes, cracked earth | Cacti, dead shrubs, tumbleweeds | Bones, ruins, oasis | Harsh bright | Morroc fields |
| **Snow/Tundra** | Snow-covered flat/hills | Pine trees, frozen bushes | Ice formations, snow mounds | Cool diffused | Lutie fields |
| **Volcanic** | Dark rock, lava cracks | Minimal/dead vegetation, fire plants | Lava pools, geysers | Red-orange ambient | Thor's Volcano exterior |
| **Swamp** | Flat, muddy, shallow water | Mangrove trees, lily pads, reeds | Bubbling mud, fog | Green-tinted diffused | Comodo swamp |
| **Mountain** | Steep slopes, cliff faces | Alpine flowers, pine trees | Boulders, mine entrances | Clear bright | Mjolnir Mountains |
| **Coast/Beach** | Sand meeting water | Palm trees, beach grass | Driftwood, shells, boats | Bright warm | Byalan coast |
| **Corrupted/Dark** | Cracked, discolored terrain | Dead trees, toxic plants | Ruined structures, dark crystals | Dim, tinted | Glast Heim exterior |

#### Terrain Workflow

```
1. Heightmap Generation
   Tool: World Machine or Gaea → export 16-bit heightmap
   Resolution: 4033x4033 per landscape (8x8 components, 63 quads each)
   Scale: ~500m x 500m per RO map (1 RO cell = ~50 UE units)

2. Landscape Material
   Approach: Landscape Material with 4-8 layers, auto-painted by slope/height
   Layers: Grass, Dirt, Rock, Sand, Snow, Mud, Stone Path, Lava Rock
   Technique: Runtime Virtual Texturing (RVT) for blending
   Stylization: Hand-painted texture tiles (NOT photoscanned)

3. Foliage Placement
   Tool: UE5 Procedural Foliage Tool + manual painting for hero areas
   PCG Framework: For scatter props (rocks, debris, flowers)
   Density: Moderate (RO maps are readable, not dense jungle)
   Nanite: OFF for foliage (experimental in 5.7, use standard LODs)

4. Hand-Painting Pass
   Artists manually adjust foliage around paths, spawn points, landmarks
   Clear sightlines to key navigation points (town entrances, dungeon entrances)
   RO maps have "walkable vs decoration" zones -- maintain this readability
```

### 5.3 Dungeon Environments

Dungeons are instanced or shared indoor/underground environments with heavier atmosphere and denser monster populations.

#### Dungeon Type Reference

| Dungeon Type | Visual Style | Lighting | Key Features | Example Dungeons |
|-------------|-------------|----------|--------------|-----------------|
| **Cave/Natural** | Rock walls, stalactites, underground rivers | Bioluminescence, torch light | Narrow passages, open chambers, water pools | Payon Cave, Ant Hell, Byalan |
| **Ruins/Temple** | Ancient stone, carved pillars, hieroglyphics | Shaft light, glowing runes | Puzzle rooms, trap corridors, altar chambers | Pyramid, Sphinx, Odin Temple |
| **Gothic Castle** | Dark stone, stained glass, iron fixtures | Moonlight through windows, candelabras | Grand halls, throne rooms, dungeons, crypts | Glast Heim |
| **Mechanical/Clock** | Gears, pipes, moving platforms | Industrial sparks, steam vents | Rotating elements, vertical traversal, ticking | Clock Tower |
| **Sewer/Culvert** | Brick tunnels, water channels, grates | Dim overhead, water reflections | Linear progression, branching paths | Prontera Culverts |
| **Underwater** | Coral, seaweed, bubbles, sunken structures | Caustic light from above, bioluminescence | Swimming/floating movement, visibility variation | Byalan (deep levels) |
| **Volcanic** | Obsidian rock, lava flows, heat distortion | Red/orange lava glow, fire particles | Lava hazards, crumbling platforms | Thor's Volcano |
| **Tower** | Stacked floors, stairs, increasing difficulty | Changes per floor (lit to dark) | Vertical progression, floor-based encounters | Thanatos Tower |
| **Frozen** | Ice walls, frost crystals, frozen creatures | Blue-white ambient, crystal refraction | Slippery surfaces, breakable ice | Ice Cave (Rachel) |
| **Haunted** | Twisted architecture, impossible geometry | Flickering, ghostly glow | Horror elements, jump scares (mild), fog | Niflheim dungeon, Abbey |

#### Dungeon Modular Kit Approach

Each dungeon type is built from a modular kit of reusable pieces:

```
Kit Piece Categories:
 ├── Floor tiles (4-6 variants per type)
 ├── Wall segments (straight, corner, T-junction, doorway)
 ├── Ceiling tiles (with and without light fixtures)
 ├── Pillars/Columns (2-3 variants)
 ├── Stairs/Ramps (straight, spiral)
 ├── Doorways/Arches (open, gated, sealed)
 ├── Props (torches, crates, barrels, rubble, cobwebs)
 └── Unique landmarks (boss room throne, altar, mechanism)

Grid Size: 400x400 UU per tile (aligns with RO cell-based movement)
Wall Height: 400-600 UU (varies by dungeon type)
Snap Grid: 100 UU increments
```

### 5.4 Skybox and Atmosphere

| Environment | Sky Type | Cloud Style | Atmospheric Fog | Special |
|------------|----------|-------------|-----------------|---------|
| Town (day) | Clear/scattered clouds | Soft cumulus | Light distance fog | Sun position matches time |
| Town (night) | Star field + moon | Minimal wispy | Heavier distance fog | Moonlight + artificial light |
| Fields (day) | Dynamic sky blueprint | Volumetric clouds (stylized) | Light | Wind-animated clouds |
| Fields (night) | Star dome | Minimal | Medium | Campfire glow from monster camps |
| Dungeon | None (ceiling) | N/A | Heavy interior fog | Per-type ambient particles |
| Volcanic | Overcast/red | Ash clouds | Thick smoke | Ember particles |
| Underwater | Water surface above | Light rays from surface | Heavy blue fog | Bubble particles, caustics |

**UE5 Implementation**: Use `BP_Sky_Sphere` modified with stylized cloud textures (hand-painted, NOT photorealistic). Exponential Height Fog for distance, Volumetric Fog for dungeon interiors. Sky Atmosphere component for the base gradient, overridden per zone.

### 5.5 Water

| Water Type | Shader Features | Movement | Transparency | Use |
|-----------|----------------|----------|-------------|-----|
| Ocean | Stylized waves (Gerstner), foam, depth coloring | Large slow waves | Semi-transparent with depth fade | Coastlines, Alberta |
| River | Flow direction, ripple on obstacles, foam at edges | Directional flow | Semi-transparent | Town canals, field rivers |
| Lake/Pond | Gentle ripple, reflections, lily pad float | Subtle | Transparent with depth color | Town decorative, field ponds |
| Swamp | Murky, slow bubble, floating debris | Very slow | Opaque/murky | Swamp biome |
| Lava | Emissive flow, heat distortion above, crust plates | Slow viscous flow | Opaque, self-lit | Volcanic dungeons |
| Waterfall | Cascade particles, mist at base, splash foam | Vertical flow | Transparent spray | Comodo, dungeon transitions |

**UE5 Implementation**: Single Water Material `M_Water_Master` with parameters for color, opacity, wave scale, flow direction, foam threshold. Use Material Instances per water type. Water body actors with spline-based shapes.

---

## 6. VFX -- Visual Effects

### 6.1 Current VFX System Status

The project already has a functional VFX subsystem (`SkillVFXSubsystem`) with 1,574 VFX assets cataloged across Niagara (NS_) and Cascade (P_) systems. 127 assets are already migrated into the project. Five VFX behavior patterns are implemented:

| Pattern | Name | Description | Implemented Skills |
|---------|------|-------------|-------------------|
| A | Bolt | N bolts from sky, staggered 0.15s | Cold Bolt, Fire Bolt, Lightning Bolt |
| B | AoE Projectile | One projectile to primary target + impact explosion | Fire Ball |
| C | Multi-Hit Projectile | N projectiles player to enemy, staggered 200ms | Soul Strike |
| D | Persistent Buff | Spawned on buff_applied, destroyed on buff_removed | Frost Diver, Provoke, Stone Curse |
| E | Ground AoE Rain | N strikes at random positions in AoE radius | Thunderstorm |

### 6.2 Skill VFX by Element

#### Fire Element VFX

| Skill | VFX Pattern | Visual Description | Current Asset | Needed? |
|-------|------------|--------------------|----|---------|
| Fire Bolt (19) | A (Bolt) | Red-orange bolts descend from sky | Mixed_Magic NS_ assets | DONE |
| Fire Ball (207) | B (AoE Projectile) | NS_Magma_Shot projectile, NS_Magma_Shot_Impact explosion | NS_Magma_Shot + Impact | DONE |
| Fire Wall (209) | Custom | Wall of flame segments on ground, burns in line | Needs custom Niagara | TODO |
| Meteor Storm (802) | E variant | Giant flaming rocks fall from sky across AoE | NS_Explosion + custom | TODO |
| Magnum Break (105) | Custom | Radial fire burst around caster | M5VFXVOL2 fire assets | TODO |
| Fire Pillar (808) | Custom | Flame erupts from ground trap | M5VFXVOL2 fire column | TODO |

#### Ice/Water Element VFX

| Skill | VFX Pattern | Visual Description | Current Asset | Needed? |
|-------|------------|--------------------|----|---------|
| Cold Bolt (14) | A (Bolt) | Ice-blue bolts descend from sky | Mixed_Magic NS_ assets | DONE |
| Frost Diver (208) | D (Persistent Buff) | P_Ice_Proj_charge_01 Cascade, scale 1.5, looping | InfinityBlade P_ asset | DONE |
| Storm Gust (806) | E variant | Blizzard across AoE, ice crystals, freeze chance | Needs custom Niagara | TODO |
| Water Ball (805) | A variant | Water spheres crash on target | Free_Magic NS_ assets | TODO |
| Frost Nova | Custom | Radial ice explosion from caster | Needs custom Niagara | TODO |

#### Lightning Element VFX

| Skill | VFX Pattern | Visual Description | Current Asset | Needed? |
|-------|------------|--------------------|----|---------|
| Lightning Bolt (20) | A (Bolt) | Yellow-white electric bolts from sky | Zap_VFX NS_ assets | DONE |
| Thunderstorm (212) | E (Ground AoE Rain) | NS_Lightning_Strike at random AoE positions | NS_Lightning_Strike | DONE |
| Jupitel Thunder (803) | C variant | Multiple lightning balls player to enemy | Needs variant of Soul Strike | TODO |
| Lord of Vermilion (807) | E variant | Massive lightning AoE rain, screen flash | Needs custom + screen PP | TODO |

#### Holy Element VFX

| Skill | VFX Pattern | Visual Description | Current Asset | Needed? |
|-------|------------|--------------------|----|---------|
| Heal (401) | Custom | Rising green-gold sparkles on target | Needs custom Niagara | TODO |
| Blessing (402) | D (Persistent Buff) | Golden aura around target | NS_Player_Buff_Looping | TODO |
| Increase AGI (403) | D (Persistent Buff) | Blue-white speed lines aura | NS_Player_Buff_Looping variant | TODO |
| Magnus Exorcismus (809) | E variant | Grand cross pattern on ground, holy light columns | Needs custom Niagara | TODO |
| Turn Undead (407) | Custom | Holy light burst on undead target | Needs custom | TODO |

#### Shadow/Dark Element VFX

| Skill | VFX Pattern | Visual Description | Current Asset | Needed? |
|-------|------------|--------------------|----|---------|
| Soul Strike (210) | C (Multi-Hit) | NS_Magic_Bubbles, purple-ghost projectiles | NS_Magic_Bubbles | DONE |
| Napalm Beat (21) | Custom | Dark spirit energy burst (AoE) | Needs custom | TODO |
| Envenom (502) | Custom | Green poison splash on target | Needs custom | TODO |
| Venom Dust (503) | Custom | Poison cloud on ground | Needs custom | TODO |
| Grimtooth (504) | Custom | Shadow slash wave from hiding | Needs custom | TODO |

### 6.3 Casting Circles

RO's iconic casting animation shows a magic circle beneath the caster's feet during cast time. This is a critical visual element.

#### Implementation

```
System: Niagara or Decal Projector (decal preferred for ground conformity)
Trigger: skill:cast_start socket event
Duration: Cast time from server (skill-dependent)
Behavior:
  1. Spawn decal at caster feet
  2. Scale up from 0 to full radius over 0.2s
  3. Rotate slowly during cast (15 deg/sec)
  4. Pulse glow intensity with cast progress (brighter near completion)
  5. Burst flash on cast_complete, then fade out 0.3s
  6. If cast_interrupted, shatter effect + fade 0.5s

Element Colors:
  Fire:      Inner #FF4400, Outer #FFaa00, Glow #FF6600
  Water/Ice: Inner #0044FF, Outer #00CCFF, Glow #0088FF
  Wind:      Inner #00CC44, Outer #88FF88, Glow #44FF44
  Earth:     Inner #886600, Outer #CCAA44, Glow #AA8800
  Holy:      Inner #FFFFFF, Outer #FFFFaa, Glow #FFFFCC
  Shadow:    Inner #440066, Outer #8800CC, Glow #6600AA
  Poison:    Inner #006600, Outer #00CC00, Glow #00AA00
  Neutral:   Inner #AAAAAA, Outer #FFFFFF, Glow #CCCCCC

Circle Design:
  - Outer ring with runic text (scrolling UV animation)
  - Inner geometric pattern (varies by element -- pentagram for fire, snowflake for ice, etc.)
  - Particle sprites rising from the circle edge
  - Ground glow (emissive decal beneath the geometric pattern)
```

### 6.4 Buff/Debuff Indicators

| Buff Type | Visual | Duration | System |
|-----------|--------|----------|--------|
| Blessing | Golden particles rising around character | Until buff_removed | Pattern D (Persistent) |
| Increase AGI | Blue-white speed lines trailing from feet | Until buff_removed | Pattern D |
| Angelus | Translucent golden shield dome | Until buff_removed | Pattern D |
| Provoke | Red angry icon above head + red aura | Until buff_removed | Pattern D (DONE) |
| Poison | Green drip particles + periodic green flash | Until cured/expired | Pattern D |
| Freeze (Frost Diver) | Ice crystal encasement on character | Until buff_removed | Pattern D (DONE) |
| Stone Curse | Gray stone texture override + crack particles | Until buff_removed | Pattern D (DONE) |
| Stun | Star circle spinning above head | Until recovered | Pattern D |
| Silence | Speech bubble with X above head | Until recovered | Pattern D |
| Blind | Dark fog around character head | Until recovered | Pattern D |

### 6.5 Environmental VFX

| Effect | Location | System | Asset Source |
|--------|----------|--------|-------------|
| Torch flame | Dungeons, towns | Niagara (NS_) or Cascade (P_) | M5VFXVOL2 fire assets |
| Waterfall mist | Comodo, outdoor waterfalls | Niagara particle + mesh | Custom |
| Firefly/Sparkle | Night fields, forest | Niagara ambient particle | NiagaraExamples |
| Rain | Weather system | Niagara + post-process wet | Custom |
| Snow | Lutie, Rachel | Niagara falling particles | Custom |
| Sandstorm | Morroc desert | Niagara + post-process | Custom |
| Lava bubble | Volcanic zones | Niagara | M5VFXVOL2 |
| Magic crystal glow | Geffen, magic dungeons | Niagara ambient | Free_Magic |
| Fog/Mist | Dungeons, swamps | Exponential Height Fog + Volumetric | Built-in UE5 |
| Dust motes | Indoor dungeons, sunbeams | Niagara ambient | NiagaraExamples |
| Portal swirl | Warp portals | Niagara looping | Already implemented |

### 6.6 Hit Effects and Combat VFX

| Effect | Trigger | Visual | Duration |
|--------|---------|--------|----------|
| Melee hit spark | AN_AttackImpact notify | White-yellow spark burst at contact point | 0.3s |
| Critical hit | Critical flag on damage event | Larger spark + screen shake + "CRITICAL" text | 0.5s |
| Arrow impact | Projectile collision | Arrow stick + small dust puff | Arrow persists 2s |
| Magic impact | Skill effect damage | Element-colored burst at target | 0.3s |
| Miss/Dodge | Miss flag on combat event | "MISS" text float, no VFX | Text only |
| Block (shield) | Block flag | Metallic clang spark at shield position | 0.3s |
| Level up | Level up event | Golden light column + expanding ring + particle shower | 3s |
| Job change | Job change event | Grand golden burst + class-colored aura settle | 5s |
| Death dissolve | Death + respawn | Character mesh dissolves with particles, reforms at respawn | 2s each |

### 6.7 UE5 Niagara System Architecture

All new VFX should be built in Niagara (not Cascade). Cascade assets (P_ prefix from InfinityBlade) are used as-is but new work uses Niagara exclusively.

#### Niagara Module Organization

```
Content/SabriMMO/VFX/
 ├── Niagara/
 │    ├── Systems/                    # Complete effect systems (NS_ prefix)
 │    │    ├── NS_Fire_Bolt.uasset
 │    │    ├── NS_CastingCircle_Fire.uasset
 │    │    └── ...
 │    ├── Emitters/                   # Reusable emitters (NE_ prefix)
 │    │    ├── NE_Spark_Burst.uasset
 │    │    ├── NE_Trail_Fire.uasset
 │    │    └── ...
 │    ├── Modules/                    # Custom Niagara modules (NM_ prefix)
 │    │    ├── NM_CurlNoise_Stylized.uasset
 │    │    └── ...
 │    └── Parameters/                 # Shared parameter collections
 │         └── NPC_GlobalVFX.uasset
 ├── Materials/
 │    ├── M_VFX_Fire_Master.uasset
 │    ├── M_VFX_Ice_Master.uasset
 │    └── ...
 ├── Textures/
 │    ├── T_VFX_FireNoise.uasset
 │    ├── T_VFX_RuneCircle_Fire.uasset
 │    └── ...
 └── Meshes/
      ├── SM_VFX_Bolt.uasset
      └── ...
```

#### Niagara Best Practices for This Project

1. **Use GPU Sim where possible** -- Particle counts for AoE spells (Meteor Storm, Storm Gust) can be high. GPU simulation handles thousands of particles efficiently.
2. **Scalability settings** -- Create Low/Medium/High VFX quality settings. Low = fewer particles + simpler materials. High = full particle count + complex shaders.
3. **Warm-up frames** -- Looping systems (buff auras, environmental) must use WarmupTime to avoid the "burst on spawn" artifact.
4. **Pool particle components** -- Reuse NiagaraComponents via object pooling for frequently spawned effects (bolt impacts, hit sparks).
5. **Distance culling** -- VFX beyond 5000 UU are culled. VFX beyond 3000 UU use reduced particle count (LOD).

---

## 7. UI Art

### 7.1 RO Brown Wooden Panel Theme (HD)

RO's UI is iconic: warm brown wooden panels with ornate borders, parchment-colored text backgrounds, and small decorative corner elements. The HD version preserves this warmth while increasing resolution and adding subtle depth.

#### Panel Style Specifications

```
Panel Background:
  Base Color:    #5C3A1E (warm dark wood)
  Gradient:      Subtle radial, lighter center (#6B4A2E) to darker edges (#4A2A10)
  Texture:       Tileable wood grain overlay at 15% opacity
  Border:        3px inset bevel, highlight (#8B6A3E), shadow (#2A1A08)
  Corner Pieces: Ornate metal bracket sprites (gold-tinted #C0A060)
  Inner Margin:  12px padding from border to content

Title Bar:
  Background:    Slightly darker than panel (#4A2A10)
  Text Color:    #FFE8B0 (warm cream-gold)
  Font:          Bold, small-caps
  Height:        32px at 1080p

Content Area:
  Background:    Parchment overlay (#F0E8D0 at 25% opacity over panel)
  Text Color:    #2A1A08 (dark brown) for body, #8B4513 for labels
  Separator:     1px line #8B6A3E

Button (Normal):
  Background:    Linear gradient top #6B4A2E to bottom #4A2A10
  Border:        2px solid #8B6A3E
  Text:          #FFE8B0
  Corner Radius: 4px

Button (Hover):
  Background:    Brightened 15% (#7B5A3E to #5A3A20)
  Border:        2px solid #C0A060 (gold highlight)
  Glow:          Subtle outer glow 4px #FFE8B0 at 30%

Button (Pressed):
  Background:    Darkened 10% (#5B3A1E to #3A1A00)
  Border:        2px solid #4A2A10 (inset feel)
  Text:          Offset down 1px

Scrollbar:
  Track:         Dark groove #3A1A08
  Thumb:         Wood texture bar #6B4A2E with ridge detail
  Arrows:        Small triangular metal ornaments
```

### 7.2 Icon Creation Pipeline

RO has 1000+ item icons, skill icons, status icons, and emote icons. Each needs an HD version.

#### Icon Specifications

| Icon Type | Dimensions | Background | Border | Count Needed |
|-----------|-----------|-----------|--------|-------------|
| Item (Inventory) | 48x48 (render at 96x96) | Transparent or slot background | None (slot provides frame) | 1000+ |
| Skill (Skill Tree) | 48x48 (render at 96x96) | Dark gradient (#222 to #111) | 1px gold (#C0A060) | 200+ |
| Status Effect | 24x24 (render at 48x48) | Transparent | None | 80+ |
| Emote Bubble | 64x64 (render at 128x128) | White speech bubble | Thin dark outline | 30+ |
| Minimap Icon | 16x16 (render at 32x32) | Transparent | None | 20+ |
| Class Emblem | 64x64 (render at 128x128) | Transparent | Optional ornate frame | 35+ |

#### Item Icon Art Style

- **Perspective**: Slight 3/4 view (not flat orthographic, not full perspective)
- **Outline**: 1px dark outline on all items (readable on any background)
- **Shading**: Simple 2-tone shading with highlight (not realistic, not flat)
- **Color**: Saturated and distinctive (each item recognizable by color alone at inventory scale)
- **Consistency**: All items drawn at same relative scale (a dagger is smaller than a sword)

#### Production Pipeline for Icons

```
1. Reference: Capture original RO icon from sprite data
2. Concept: AI-assisted upscale + artist redraw at 4x resolution
3. Render: Paint in Photoshop/Clip Studio at 96x96 or 128x128
4. Export: PNG with transparency, sRGB color space
5. Import: UE5 Texture2D, Compression: UserInterface2D (BC7 on PC, ASTC on mobile)
6. Atlas: Group into texture atlases (16x16 icons per 768x768 atlas) for draw call reduction
7. Name: T_Icon_{Type}_{ItemID}_{Name} (e.g., T_Icon_Item_501_RedPotion)
```

### 7.3 Fonts

| Use | Font Style | Size Range (1080p) | Color | Weight |
|-----|-----------|--------------------|----|--------|
| UI Panel Title | Serif/Medieval (e.g., Cinzel, IM Fell) | 18-22pt | #FFE8B0 | Bold |
| UI Body Text | Clean Sans (e.g., Noto Sans, Source Sans Pro) | 12-14pt | #2A1A08 | Regular |
| Damage Numbers | Bold Sans/Impact | 18-28pt | White (player), Red (enemy), Yellow (crit) | Bold |
| Chat Text | Monospace or clean Sans | 12pt | Varies by channel | Regular |
| System Messages | Clean Sans | 13pt | #FFE8B0 | Italic |
| NPC Dialog | Serif (matching title) | 14-16pt | #2A1A08 | Regular |
| Floating Names | Clean Sans | 11-13pt | White (player), Red (enemy), Yellow (NPC) | Bold |

**UE5 Implementation**: Import as `.ttf`/`.otf` into `Content/SabriMMO/UI/Fonts/`. Create `FSlateFontInfo` entries in a central `UDataAsset` (or hard-code in Slate widget constructors as currently done). Use Font Face assets with sub-pixel hinting enabled.

---

## 8. UE5 Asset Pipeline

### 8.1 Folder Structure

```
Content/SabriMMO/
 ├── Characters/
 │    ├── Player/
 │    │    ├── Base/
 │    │    │    ├── SK_Male_Base.uasset          (Skeletal Mesh)
 │    │    │    ├── SK_Female_Base.uasset
 │    │    │    ├── SKEL_Player.uasset            (Skeleton)
 │    │    │    ├── PHYS_Player.uasset             (Physics Asset)
 │    │    │    └── ABP_PlayerCharacter.uasset      (Animation Blueprint)
 │    │    ├── Outfits/
 │    │    │    ├── Novice/
 │    │    │    │    ├── SK_Outfit_Novice_M.uasset
 │    │    │    │    ├── SK_Outfit_Novice_F.uasset
 │    │    │    │    └── MI_Outfit_Novice.uasset    (Material Instance)
 │    │    │    ├── Swordsman/
 │    │    │    ├── Mage/
 │    │    │    └── ... (per class)
 │    │    ├── Hair/
 │    │    │    ├── SK_Hair_M_01.uasset ... SK_Hair_M_24.uasset
 │    │    │    ├── SK_Hair_F_01.uasset ... SK_Hair_F_24.uasset
 │    │    │    └── MI_Hair_Color_00.uasset ... MI_Hair_Color_08.uasset
 │    │    ├── Weapons/
 │    │    │    ├── Swords/
 │    │    │    │    ├── SM_Weapon_Sword_1301.uasset
 │    │    │    │    └── ...
 │    │    │    ├── Daggers/
 │    │    │    ├── Staves/
 │    │    │    └── ... (per weapon type)
 │    │    ├── Headgear/
 │    │    │    ├── Top/
 │    │    │    ├── Mid/
 │    │    │    └── Low/
 │    │    └── Animations/
 │    │         ├── Locomotion/
 │    │         │    ├── A_Idle.uasset
 │    │         │    ├── A_Walk.uasset
 │    │         │    ├── A_Run.uasset
 │    │         │    └── BS_Locomotion.uasset      (Blend Space)
 │    │         ├── Combat/
 │    │         │    ├── Sword_1H/
 │    │         │    │    ├── AM_Attack_Sword1H_01.uasset  (Montage)
 │    │         │    │    └── ...
 │    │         │    ├── Dagger/
 │    │         │    └── ... (per weapon type)
 │    │         ├── Status/
 │    │         │    ├── AM_Death.uasset
 │    │         │    ├── AM_HitReact_Light.uasset
 │    │         │    └── ...
 │    │         └── Emotes/
 │    │              ├── AM_Emote_Greeting.uasset
 │    │              └── ...
 │    └── Monsters/
 │         ├── Poring/
 │         │    ├── SK_Monster_Poring.uasset
 │         │    ├── SKEL_Poring.uasset
 │         │    ├── ABP_Monster_Poring.uasset
 │         │    ├── A_Poring_Idle.uasset
 │         │    ├── A_Poring_Walk.uasset
 │         │    ├── A_Poring_Attack.uasset
 │         │    ├── A_Poring_Hit.uasset
 │         │    ├── A_Poring_Die.uasset
 │         │    └── MI_Monster_Poring.uasset
 │         ├── Lunatic/
 │         └── ... (per monster)
 ├── Environments/
 │    ├── Towns/
 │    │    ├── Prontera/
 │    │    │    ├── Meshes/
 │    │    │    ├── Materials/
 │    │    │    └── Textures/
 │    │    ├── Geffen/
 │    │    └── ...
 │    ├── Fields/
 │    │    ├── Grassland/
 │    │    ├── Desert/
 │    │    ├── Forest/
 │    │    └── ...
 │    ├── Dungeons/
 │    │    ├── Kits/
 │    │    │    ├── Cave/
 │    │    │    ├── Gothic/
 │    │    │    ├── Mechanical/
 │    │    │    └── ...
 │    │    └── Unique/
 │    │         ├── GlastHeim/
 │    │         └── ...
 │    ├── Skybox/
 │    ├── Water/
 │    └── Props/
 │         ├── Vegetation/
 │         ├── Structures/
 │         └── Decorative/
 ├── VFX/
 │    ├── Niagara/
 │    ├── Materials/
 │    ├── Textures/
 │    └── Meshes/
 ├── UI/
 │    ├── Icons/
 │    │    ├── Items/
 │    │    ├── Skills/
 │    │    ├── Status/
 │    │    └── Atlases/
 │    ├── Panels/
 │    ├── Fonts/
 │    └── Textures/
 └── Audio/ (separate doc)
```

### 8.2 Naming Conventions

| Asset Type | Prefix | Example |
|-----------|--------|---------|
| Skeletal Mesh | SK_ | SK_Male_Base, SK_Monster_Poring |
| Static Mesh | SM_ | SM_Weapon_Sword_1301, SM_Headgear_Top_5027 |
| Skeleton | SKEL_ | SKEL_Player, SKEL_Poring |
| Physics Asset | PHYS_ | PHYS_Player |
| Animation Sequence | A_ | A_Idle, A_Poring_Walk |
| Animation Montage | AM_ | AM_Attack_Sword1H_01 |
| Animation Blueprint | ABP_ | ABP_PlayerCharacter, ABP_Monster_Base |
| Blend Space | BS_ | BS_Locomotion |
| Material | M_ | M_Hair_Master, M_VFX_Fire_Master |
| Material Instance | MI_ | MI_Hair_Color_00, MI_Outfit_Novice |
| Texture | T_ | T_Icon_Item_501, T_VFX_RuneCircle_Fire |
| Niagara System | NS_ | NS_Fire_Bolt, NS_CastingCircle_Fire |
| Niagara Emitter | NE_ | NE_Spark_Burst |
| Blueprint | BP_ | BP_MMOCharacter, BP_EnemyManager |
| Widget Blueprint | WBP_ | WBP_Inventory |
| Interface | BPI_ | BPI_Damageable |

### 8.3 FBX Import Settings

#### Skeletal Mesh (Characters, Monsters)

```
Import Settings:
  Skeletal Mesh:         YES
  Import Animations:     Separate import pass
  Import Morph Targets:  YES (for facial if applicable)
  Normal Import Method:  Import Normals and Tangents
  Import Materials:      YES (then replace with project materials)
  Vertex Color Import:   Replace (not Ignore -- used for AO painting)

Skeleton:
  Use Existing:          YES (SKEL_Player for all player meshes)
  Scale:                 1.0 (model in cm, matching UE5 default)

Geometry:
  Convert Scene Unit:    YES
  Force Front X Axis:    NO
  Import Rotation:       [0, 0, 0] (model facing +X in DCC tool)

Source Format:           FBX 2020 (or glTF 2.0 via Interchange)
Coordinate System:      Right-handed Z-up (Blender default) auto-converted
```

#### Static Mesh (Weapons, Headgear, Props)

```
Import Settings:
  Import as Skeletal:    NO
  Auto Generate Collision: YES (for gameplay props), NO (for cosmetic)
  Generate Lightmap UVs:  YES (UV channel 1)
  Normal Import Method:   Import Normals
  LOD:                    Import LODs from file if present, else auto-generate

LOD Settings (auto-generate):
  LOD 1: 50% triangles at 1000 UU distance
  LOD 2: 25% triangles at 2500 UU distance
  LOD 3: 12% triangles at 5000 UU distance (large props/buildings only)
```

### 8.4 Texture Specifications

| Texture Type | Resolution | Format (PC) | sRGB | Compression | Mips |
|-------------|-----------|-------------|------|------------|------|
| Character Albedo | 2048x2048 | BC7 | YES | Default | YES |
| Character Normal | 2048x2048 | BC5 | NO | Normalmap | YES |
| Character ORM | 2048x2048 | BC7 | NO | Masks | YES |
| Monster Albedo | 1024-2048 | BC7 | YES | Default | YES |
| Monster Normal | 1024-2048 | BC5 | NO | Normalmap | YES |
| Environment Albedo | 2048x2048 | BC7 | YES | Default | YES |
| Environment Normal | 2048x2048 | BC5 | NO | Normalmap | YES |
| VFX Texture | 256-1024 | BC7 | YES | Default | YES |
| UI Icon | 48-128 | BC7 | YES | UserInterface2D | NO |
| UI Panel | 512-1024 | BC7 | YES | UserInterface2D | NO |
| Landscape Layer | 2048x2048 (tiling) | BC7 | YES | Default | YES |

**ORM Format**: A single texture packing Occlusion (R), Roughness (G), Metallic (B) into one BC7 texture. Reduces texture samples per material from 3 to 1.

#### Stylized Texture Guidelines

- Textures should be **hand-painted or AI-upscaled-then-painted-over**, NOT photoscanned
- Use **flat color with painted shadows** (not ambient occlusion baked into albedo)
- Normal maps should be **subtle** -- broad forms only, not pore-level detail
- Roughness maps should be **high overall** (0.6-0.9) -- anime materials are not glossy
- Metallic should be **binary** (0 or 1) -- nothing is "somewhat metallic"

### 8.5 PBR with Anime Post-Processing Material Setup

#### Master Material Architecture

```
M_Character_Master (Material)
 ├── Parameters:
 │    ├── BaseColor (Texture2D)          -- Hand-painted albedo
 │    ├── Normal (Texture2D)             -- Subtle broad-form normals
 │    ├── ORM (Texture2D)               -- Packed Occlusion/Roughness/Metallic
 │    ├── TintColor (Vector3)           -- Per-instance color shift (armor tinting)
 │    ├── EmissiveColor (Vector3)       -- For glowing elements (enchanted gear)
 │    ├── EmissiveIntensity (Scalar)    -- Glow strength
 │    ├── RimLightColor (Vector3)       -- Anime rim light color
 │    └── RimLightIntensity (Scalar)    -- Rim strength (0-2)
 ├── Custom Nodes:
 │    ├── MF_CelShadeRamp              -- Material Function: 2-3 band toon ramp
 │    └── MF_RimLight                  -- Material Function: Fresnel-based rim
 └── Output:
      ├── Base Color:  Albedo * TintColor
      ├── Normal:      Normal texture
      ├── Roughness:   ORM.G (clamped 0.5-1.0 for stylized)
      ├── Metallic:    ORM.B (binary 0 or 1)
      ├── Emissive:    EmissiveColor * EmissiveIntensity
      └── Custom Data: RimLight data for post-process
```

#### Post-Process Chain (ordered)

```
PP_01_CelShade       Priority: 1    -- Toon ramp on scene color (2-3 band quantization)
PP_02_Outline        Priority: 2    -- Sobel edge detection on depth + normals
PP_03_ColorGrade     Priority: 3    -- Per-zone LUT (warm/cool/desaturated)
PP_04_Bloom          Priority: 4    -- Soft stylized bloom (wider kernel, lower threshold)
PP_05_Vignette       Priority: 5    -- Subtle edge darkening
```

### 8.6 LOD Strategy

| Asset Type | LOD 0 (Close) | LOD 1 | LOD 2 | LOD 3 | Nanite? |
|-----------|---------------|-------|-------|-------|---------|
| Player Character | 8K-12K tris | 4K-6K | 2K-3K | 1K | NO (animated) |
| Monster (Small) | 1K-3K tris | 500-1.5K | -- | -- | NO |
| Monster (Medium) | 3K-8K tris | 1.5K-4K | 800-2K | -- | NO |
| Monster (Large/Boss) | 8K-20K tris | 4K-10K | 2K-5K | 1K-2.5K | NO |
| Building | 5K-15K tris | 2.5K-7.5K | 1K-3K | -- | YES (if static) |
| Prop (Large) | 2K-5K tris | 1K-2.5K | 500-1K | -- | YES |
| Prop (Small) | 500-2K tris | 250-1K | -- | -- | YES |
| Foliage (Tree) | 3K-8K tris | 1.5K-4K | Billboard | -- | NO |
| Foliage (Grass) | 100-300 tris | 50-100 | -- | -- | NO |
| Headgear | 200-2K tris | -- | -- | -- | NO |
| Weapon | 500-3K tris | 250-1.5K | -- | -- | NO |

**Nanite Note**: Nanite is used ONLY for static environment meshes (buildings, rocks, terrain props). Skeletal meshes (characters, monsters) and foliage do NOT support Nanite in UE5.7.

### 8.7 Performance Budgets

| Metric | Target (1080p Medium) | Target (1440p High) |
|--------|----------------------|---------------------|
| Draw calls per frame | < 2000 | < 3000 |
| Triangle count (scene) | < 2M | < 5M |
| Texture memory (VRAM) | < 2GB | < 4GB |
| Visible characters | 50+ at 30fps | 50+ at 60fps |
| Visible monsters | 30+ at 30fps | 30+ at 60fps |
| Active VFX systems | 20 simultaneous | 40 simultaneous |
| Skeletal mesh updates | < 100 per frame | < 150 per frame |
| Target framerate | 30fps (minimum) | 60fps (target) |

---

## 9. Tools and Software

### 9.1 3D Modeling

| Tool | Use | License | Notes |
|------|-----|---------|-------|
| **Blender 4.x** | Primary modeling, sculpting, UV unwrap, rigging, animation | Free/Open Source | Industry standard for indie. Excellent FBX export. |
| Maya (optional) | Animation polish, character rigging, mocap cleanup | Commercial (Indie: $325/yr) | Superior animation tools, but Blender sufficient for solo dev |
| ZBrush (optional) | High-poly sculpting for normal map baking | Commercial ($39.95/mo) | Only needed for complex organic models |

#### Blender-to-UE5 Workflow

```
1. Model in Blender at 1 BU = 1 cm (matching UE5 default)
2. Apply all transforms (Ctrl+A → All Transforms)
3. Forward axis: -Y Forward, Z Up (Blender default, auto-converted by UE5)
4. Export as FBX 2020 or glTF 2.0
   FBX Settings:
     - Scale: 1.0
     - Apply Scalings: FBX All
     - Forward: -Y Forward
     - Up: Z Up
     - Apply Unit: ON
     - Apply Transform: ON
     - Mesh: OFF (Triangulate Faces) -- let UE5 triangulate
     - Armature: OFF (Add Leaf Bones) -- reduces skeleton bloat
     - Bake Animation: ON (for animation exports)
5. Import into UE5 via Content Browser or Interchange framework
```

### 9.2 Texturing

| Tool | Use | License | Notes |
|------|-----|---------|-------|
| **Substance Painter** | PBR texturing, hand-painted style with smart materials | Commercial ($19.90/mo or perpetual) | Best for PBR texturing workflow |
| **Photoshop / Clip Studio Paint** | 2D icon painting, texture touch-up, UI art | Commercial | Essential for UI work |
| **Krita** | Free alternative for 2D painting, icon creation | Free/Open Source | Good Photoshop alternative |
| **ArmorPaint** | Free alternative to Substance Painter | Free/Open Source | Less polished but functional |

#### Substance Painter Stylized Workflow

```
1. Bake mesh maps (normal from high-poly, AO, curvature, position)
2. Base fill layer with hand-painted albedo color
3. Paint shadows manually (NOT just rely on AO bake)
4. Add painted highlights on raised surfaces
5. Keep roughness high (0.6-0.9) and uniform per surface type
6. Metallic is binary: full metal or no metal
7. Export:
   - BaseColor (sRGB)
   - Normal (OpenGL or DirectX as needed)
   - ORM packed (Linear: AO=R, Roughness=G, Metallic=B)
```

### 9.3 VFX

| Tool | Use | Notes |
|------|-----|-------|
| **UE5 Niagara** | All new particle effects | Primary VFX authoring tool, built into UE5 |
| **Houdini (optional)** | Complex procedural VFX, SideFX Labs for UE5 | Apprentice version free, export VATs for UE5 |
| **After Effects (optional)** | VFX sprite sheet generation, flipbook textures | For 2D animated textures used in Niagara |
| **EmberGen (optional)** | Realtime fluid sim for fire/smoke flipbooks | Indie: $19.99/mo |

### 9.4 AI-Assisted Tools

| Tool | Use | Workflow Stage | Caveats |
|------|-----|---------------|---------|
| **Stable Diffusion / Midjourney** | Concept art generation, reference images | Pre-production | NOT for final assets -- legal/style inconsistency |
| **ControlNet (SD)** | Pose-guided concept art, maintaining character consistency | Character concept | Use with RO sprite references as input |
| **AI Upscaling (ESRGAN)** | Upscale original RO sprites to high-res references | Reference creation | 4x upscale of original icons/sprites for clarity |
| **Meshy / Tripo3D** | Quick 3D blockouts from concept art | Prototype | Output needs heavy artist cleanup |
| **ChatGPT / Claude** | Writing item descriptions, lore, NPC dialog | Content writing | Always review and edit output |

**AI Ethics Policy**: AI tools are used for **concept exploration and reference only**. All final shipped assets are created or substantially modified by human artists. AI-generated textures are NOT used directly due to style inconsistency and potential legal issues.

### 9.5 Project Management

| Tool | Use |
|------|-----|
| **Git (this repo)** | Source code version control (C++, JS, SQL, docs) |
| **Git LFS** | Large binary files (`.uasset` if tracked, PSD source files) |
| **Perforce (optional)** | Better binary asset management if team scales |

**Note**: UE5 `.uasset` files are NOT tracked in this Git repo (they are in `.gitignore`). Binary assets live on the local machine and would need a separate version control strategy (Git LFS or Perforce) if the team grows beyond one person.

---

## 10. Phased Production Plan

### Phase 0: Technical Foundation (Weeks 1-2)

**Goal**: Establish the rendering pipeline and prove the visual style before creating content.

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| Set up cel-shading post-process material | PP_01_CelShade working in test level | 2 days |
| Set up outline post-process | PP_02_Outline with depth + normal edges | 2 days |
| Create M_Character_Master material | Base material with all parameters | 1 day |
| Create M_Environment_Master material | Landscape + prop base material | 1 day |
| Create M_Monster_Master material | Monster base material with tint | 0.5 day |
| Create M_VFX_Master material | Transparent/additive VFX base | 0.5 day |
| Configure post-process volume per-zone system | Zone-specific LUT + fog settings | 1 day |
| Performance baseline | Test scene with 50 placeholder characters | 1 day |
| Document style guide with screenshots | Visual reference for all future art | 1 day |

**Exit Criteria**: A test level with placeholder meshes that LOOKS like the target style. If a screenshot could be mistaken for a stylized anime game, the foundation is correct.

### Phase 1: Base Character + Novice (Weeks 3-6)

**Goal**: One fully playable character with animations, replacing the current placeholder.

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| Model SK_Male_Base | Male base body mesh (6K tris) | 3 days |
| Model SK_Female_Base | Female base body mesh (6K tris) | 3 days |
| Rig both to SKEL_Player | Skeleton with proper weighting | 2 days |
| Model SK_Outfit_Novice_M / _F | Novice class outfit meshes | 2 days |
| Texture all base + novice meshes | Albedo, Normal, ORM | 3 days |
| Model 6 hair styles per gender (12 total) | SK_Hair_M_01 through _06, same for F | 4 days |
| Create 9 hair color material instances | MI_Hair_Color_00 through _08 | 0.5 day |
| Locomotion animations | Idle, Walk, Run, Strafe, BackWalk | 4 days |
| Combat animations (unarmed) | 2 attack anims, hit react, death | 2 days |
| Status animations | Sit, Cast_Start/Loop/End | 2 days |
| Set up ABP_PlayerCharacter | Locomotion state machine + montage slots | 2 days |
| Integrate with BP_MMOCharacter | Replace placeholder with new character | 2 days |
| Test with multiplayer (5+ characters visible) | Performance + visual verification | 1 day |

**Exit Criteria**: Log in, see your customized character (gender, hair style, hair color), walk around, attack, sit, cast. Other players visible with their customizations.

### Phase 2: First Job Outfits + Weapons (Weeks 7-12)

**Goal**: All 6 first-job class outfits and their primary weapon types.

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| Model 6 outfit sets x 2 genders | 12 outfit skeletal meshes | 12 days |
| Texture all outfits | 12 texture sets | 6 days |
| Model weapon sets (Sword, Dagger, Staff, Bow, Mace, Axe) | 3 weapons per type = 18 meshes | 6 days |
| Model 3 shields | SM_Weapon_Shield variants | 1 day |
| Per-weapon attack animations | 2-3 anims per weapon type x 6 = ~15 anims | 8 days |
| Remaining 18 hair styles per gender | Complete hair library to 24 | 6 days |
| Equipment swap system implementation | C++ integration with modular character | 3 days |

**Exit Criteria**: Change job to any first class, see outfit change. Equip weapons, see them on character. All weapon attack animations playing correctly with ASPD scaling.

### Phase 3: Core Monsters -- Tier 1 (Weeks 13-20)

**Goal**: First 30 monsters modeled, rigged, animated, and integrated with server monster data.

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| Model 30 monster meshes | Grouped by size (18 Small, 10 Medium, 2 Large) | 20 days |
| Rig all monsters | Per-monster skeletons | 6 days |
| Texture all monsters | Hand-painted albedo + normal | 10 days |
| Animate all monsters (5 anims each) | 150 animations total | 15 days |
| Set up ABP_Monster per type | State machines, animation integration | 4 days |
| Integrate with BP_EnemyManager | Replace placeholders, match server data | 3 days |
| Monster size/scale calibration | Match RO proportions in 3D space | 1 day |

**Exit Criteria**: All 30 Tier 1 monsters visible in game, correctly sized, animating (idle, walk, attack, hit, die), matched to server spawn data.

### Phase 4: Prontera Town (Weeks 21-26)

**Goal**: First fully-realized 3D town environment.

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| Block out Prontera layout | Whitebox matching original map walkability | 3 days |
| Create medieval building kit | 15-20 modular pieces | 8 days |
| Texture building kit | Hand-painted tileable materials | 4 days |
| Model unique landmarks | Cathedral, fountain, castle gates | 5 days |
| Terrain + landscape material | Ground, paths, grass areas | 3 days |
| Foliage (trees, flowers, grass) | 8-10 foliage assets | 4 days |
| Props (market stalls, barrels, signs) | 20-30 small props | 5 days |
| Lighting setup (day + night) | Directional light + sky + point lights | 2 days |
| NPC placement + Kafra visual | NPC mesh stands, Kafra model | 2 days |
| Water (fountain, canals) | Water material + mesh | 1 day |
| Skybox | Stylized sky dome | 1 day |
| Optimization pass | LODs, culling, draw call reduction | 2 days |

**Exit Criteria**: Walk through Prontera in 3D. It should feel unmistakably like Prontera -- same general layout, same landmarks, same mood. Maintain 60fps with 20+ other players visible.

### Phase 5: Prontera Fields + First Dungeon (Weeks 27-32)

**Goal**: Outdoor gameplay area + first dungeon environment.

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| Prontera field terrain (2-3 field maps) | Landscape with grass, paths, trees | 6 days |
| Field props and foliage | Fences, rocks, rivers, bridges | 4 days |
| Payon Cave dungeon kit | Cave modular pieces (8-10) | 5 days |
| Payon Cave levels 1-3 layout | 3 dungeon levels | 6 days |
| Dungeon lighting | Torch lights, ambient glow | 2 days |
| Dungeon props | Bones, cobwebs, cracked walls, treasure chests | 3 days |
| Environmental VFX | Torch fire, cave dust, field wind | 3 days |
| Performance testing across all zones | Multi-zone stress test | 1 day |

**Exit Criteria**: Complete gameplay loop: Log in at Prontera, walk to fields, fight monsters, enter Payon Cave dungeon, fight dungeon monsters. All in finished 3D environments.

### Phase 6: Second Job Content (Weeks 33-44)

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| 14 second-job outfits x 2 genders | 28 outfit meshes | 28 days |
| Additional weapon types (Katar, Knuckle, Book, Instrument, Whip) | 15+ weapon meshes | 5 days |
| Tier 2 monsters (30 more) | Models, rigs, textures, animations | 30 days |
| Headgear Phase 1 (50 items) | 50 headgear static meshes | 10 days |
| 3 more town environments | Geffen, Payon, Morroc blockouts + art | 24 days |

### Phase 7: Polish + Transcendent Content (Weeks 45-56)

| Task | Deliverable | Time Estimate |
|------|-----------|---------------|
| 14 transcendent outfits x 2 genders | 28 outfit meshes (enhanced 2nd job) | 21 days |
| Tier 3 monsters (40 more) | Including MVPs and bosses | 30 days |
| Headgear Phase 2 (100 more) | 100 additional headgear meshes | 15 days |
| Skill VFX completion | All implemented skills have VFX | 10 days |
| UI icon set Phase 1 | 200 item icons + 50 skill icons | 15 days |
| Additional dungeon environments | Pyramid, Geffen Dungeon, Clock Tower | 20 days |
| Animation polish | IK, blending improvements, emotes | 10 days |

### Total Asset Count Summary

| Category | Phase 1 | Phase 2 | Phase 3 | Phase 4-5 | Phase 6 | Phase 7 | Grand Total |
|----------|---------|---------|---------|-----------|---------|---------|-------------|
| Character Outfits | 2 | 12 | -- | -- | 28 | 28 | **70** |
| Hair Meshes | 12 | 36 | -- | -- | -- | -- | **48** |
| Weapons | -- | 21 | -- | -- | 15 | -- | **36** |
| Headgear | -- | -- | -- | -- | 50 | 100 | **150+** |
| Monsters | -- | -- | 30 | -- | 30 | 40 | **100** |
| Monster Anims | -- | -- | 150 | -- | 150 | 200 | **500** |
| Character Anims | 10 | 15 | -- | -- | 5 | 10 | **40** |
| Environment Sets | -- | -- | -- | 5 zones | 3 towns | 3 dungeons | **11+** |
| Icons | -- | -- | -- | -- | -- | 250 | **250** |
| VFX Systems | -- | -- | -- | 10 | 15 | 20 | **45+** |

---

## Appendix A: RO Class Progression Reference

```
Novice ──┬── Swordsman ──┬── Knight ───────── Lord Knight
         │               └── Crusader ──────── Paladin
         ├── Mage ────────┬── Wizard ──────── High Wizard
         │                └── Sage ─────────── Scholar (Professor)
         ├── Archer ──────┬── Hunter ──────── Sniper
         │                ├── Bard (M) ──────── Clown (Minstrel)
         │                └── Dancer (F) ────── Gypsy
         ├── Acolyte ─────┬── Priest ──────── High Priest
         │                └── Monk ─────────── Champion
         ├── Merchant ────┬── Blacksmith ───── Whitesmith
         │                └── Alchemist ────── Creator (Biochemist)
         └── Thief ───────┬── Assassin ─────── Assassin Cross
                          └── Rogue ─────────── Stalker

Expanded Classes (separate progression):
  Super Novice
  TaeKwon Kid → TaeKwon Master / Soul Linker
  Gunslinger → Rebellion
  Ninja → Kagerou / Oboro
```

## Appendix B: Monster Size Reference Table

| Size | Weapon Modifier (Dagger) | Weapon Modifier (Sword) | Weapon Modifier (Spear) | Weapon Modifier (Bow) |
|------|-------------------------|------------------------|------------------------|----------------------|
| Small | 100% | 75% | 75% | 100% |
| Medium | 75% | 100% | 75% | 100% |
| Large | 50% | 75% | 100% | 75% |

These modifiers affect physical damage calculations and are implemented server-side. They do NOT affect the visual size of monsters, but should be understood when prioritizing which monsters to model (since players will target different sizes based on their weapon type).

## Appendix C: Existing VFX Asset Inventory

The project already has **1,574 VFX assets** across the following packs (see `docsNew/05_Development/VFX_Asset_Reference.md` for full inventory):

| Pack | Count | Type | Status |
|------|-------|------|--------|
| NiagaraExamples | 76 | Niagara | Partially migrated |
| Free_Magic | 17 | Niagara | Fully migrated |
| Mixed_Magic_VFX_Pack | 29 | Niagara + Cascade | Fully migrated |
| Vefects/Zap_VFX | 7 | Niagara | Fully migrated |
| _SplineVFX | 10 | Niagara | Fully migrated |
| Knife_light | 5 | Niagara | Fully migrated |
| M5VFXVOL2 | 113 | Niagara + Cascade | NOT migrated |
| InfinityBlade Effects | 838 | Cascade | NOT migrated (cherry-pick as needed) |
| Starter Content | 375 | Cascade | NOT migrated |
| Others | ~104 | Mixed | Various states |

---

*This document is a living reference. Update asset counts, phase timelines, and technical specifications as the project evolves.*
