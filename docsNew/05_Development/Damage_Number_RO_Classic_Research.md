# Ragnarok Online Classic — Damage Number & Hit Impact Research

**Date:** 2026-04-05
**Sources:** roBrowser source (Damage.js), roBrowserLegacy fork, rAthena server (clif.hpp), Ragnarok Research Lab (SPR/ACT formats), community forums, sprite archives

---

## Table of Contents

1. [Animation Trajectory](#1-animation-trajectory)
2. [Size Scaling](#2-size-scaling)
3. [Alpha Fade](#3-alpha-fade)
4. [Duration & Timing](#4-duration--timing)
5. [Color Coding](#5-color-coding)
6. [Critical Hit Treatment](#6-critical-hit-treatment)
7. [Miss / Dodge / Block](#7-miss--dodge--block)
8. [Heal Numbers](#8-heal-numbers)
9. [Combo / Multi-Hit](#9-combo--multi-hit)
10. [Font & Digit Rendering](#10-font--digit-rendering)
11. [Hit Impact Effects](#11-hit-impact-effects)
12. [Sound Design](#12-sound-design)
13. [Server Damage Types (rAthena)](#13-server-damage-types-rathena)
14. [Our Current Implementation vs RO Classic](#14-our-current-implementation-vs-ro-classic)
15. [Source References](#15-source-references)

---

## 1. Animation Trajectory

The core RO damage number feel comes from a **parabolic sine arc with diagonal drift**. Numbers don't just rise — they arc upward and to the side like a projectile, then fall back down as they shrink and fade.

### Exact Math (from roBrowser `Damage.js`)

```javascript
// t = normalized time [0.0 → 1.0] over the damage number's lifetime
// entity.position = target's world position at spawn time

// Horizontal drift (diagonal, upper-right in isometric view)
X = entity.position.X + (t * 4)     // drifts RIGHT over time
Y = entity.position.Y - (t * 4)     // drifts UP-RIGHT (isometric Y axis)

// Vertical bounce arc (the signature RO arc)
Z = entity.position.Z + 2 + sin(-PI/2 + PI * (0.5 + t * 1.5)) * 5
```

### Arc Breakdown

The Z formula `sin(-PI/2 + PI * (0.5 + t * 1.5)) * 5` creates a specific curve:

| Time (t) | Sine Input | Sine Value | Z Offset (above entity) |
|----------|------------|------------|------------------------|
| 0.00 | sin(0) | 0.0 | +2.0 (base offset) |
| 0.10 | sin(0.47) | +0.45 | +4.3 |
| 0.20 | sin(0.94) | +0.81 | +6.0 |
| 0.33 | sin(PI/2) | +1.0 | +7.0 (peak) |
| 0.50 | sin(2.36) | +0.71 | +5.5 |
| 0.67 | sin(PI) | 0.0 | +2.0 (back to base) |
| 0.80 | sin(3.77) | -0.59 | -1.0 |
| 1.00 | sin(3PI/2) | -1.0 | -3.0 (below entity) |

**Key characteristics:**
- Starts at +2 units above entity (base offset)
- Peaks at +7 units above entity (~33% through animation)
- Returns to entity height (~67% through)
- Dips -3 units below entity by end (but nearly invisible by then due to shrink+fade)
- The "falling away" feel comes from the arc descent combined with simultaneous shrinking

### roBrowserLegacy Directional Variants

The expanded fork supports 4 motion directions:

| Direction ID | X Motion | Y Motion | Use Case |
|-------------|----------|----------|----------|
| 0 (Default) | `+t*4` | `-t*4` | Standard diagonal upper-right |
| 1 (Left) | `-t*4` | 0 | Drift left |
| 2 (Top) | 0 | 0 | Pure vertical (only Z arc) |
| 3 (Right) | `+t*4` | 0 | Drift right |

Our UE5 implementation should use **screen-space equivalents** since we're rendering 2D overlay, not 3D billboards. The diagonal drift becomes a horizontal+vertical screen offset.

---

## 2. Size Scaling

### Normal/Crit Damage — Linear Shrink

```javascript
size = (1 - t) * 4
```

| Time (t) | Scale Factor |
|----------|-------------|
| 0.00 | 4.0x (start large) |
| 0.25 | 3.0x |
| 0.50 | 2.0x |
| 0.75 | 1.0x |
| 1.00 | 0.0x (shrunk to nothing) |

This is THE signature RO feel — numbers **pop in large** then **shrink away to nothing**. The linear curve means constant shrink rate, no easing. Combined with the arc and fade, this creates the "number is falling into the distance" illusion.

### Heal — Quick Shrink Then Hold

```javascript
size = Math.max((1 - t * 2) * 3, 0.8)
```

| Time (t) | Scale Factor |
|----------|-------------|
| 0.00 | 3.0x |
| 0.17 | 2.0x |
| 0.33 | 1.0x |
| 0.37 | 0.8x (floor reached) |
| 0.37+ | 0.8x (holds steady) |

Heals shrink quickly in the first 370ms, then hold at 0.8x for the remaining ~1130ms while rising.

### Miss/Lucky Dodge — Fixed Scale

```javascript
size = 0.5  // constant, no change
```

Text labels don't scale — they stay at 0.5x the sprite dimensions throughout their 800ms lifetime.

### Combo Total — Rapid Scale-Up

```javascript
size = Math.min(t, 0.05) * 70
```

| Time (t) | Scale Factor |
|----------|-------------|
| 0.00 | 0.0x (invisible) |
| 0.01 | 0.7x |
| 0.03 | 2.1x |
| 0.05 | 3.5x (max reached in 50ms!) |
| 0.05+ | 3.5x (holds for remaining 2950ms) |

The combo total **pops in** from nothing to 3.5x in just 50ms, then holds. Very punchy.

---

## 3. Alpha Fade

### Normal/Crit/Heal — Immediate Linear Fade

```javascript
alpha = 1.0 - t
```

Fade begins **immediately** from frame 1 and progresses linearly to full transparency. By the time the number reaches its arc peak (33%), alpha is already at 0.67. By the time it returns to entity height (67%), alpha is 0.33. This couples with the size shrink to create the "falling away into nothing" effect.

### Miss/Lucky Dodge

Same formula: `alpha = 1.0 - t`, but over 800ms instead of 1500ms (faster fade-out).

### Combo Total

`alpha = 1.0 - t` over 3000ms — very slow fade, stays visible much longer.

---

## 4. Duration & Timing

| Type | Duration | Notes |
|------|----------|-------|
| **Normal damage** | 1500ms | Standard auto-attack, skill hits |
| **Critical damage** | 1500ms | Same as normal (visual difference is color + starburst) |
| **Heal** | 1500ms | 40% stationary + 60% rising |
| **Miss** | 800ms | Shorter — gets out of the way fast |
| **Lucky Dodge** | 800ms | Same as miss |
| **Combo total** | 3000ms | 2x normal — stays on screen for impact |
| **Non-final combo hits** | ~225ms effective | Auto-removed at 15% of animation time |

---

## 5. Color Coding

### Exact RGB Values (from roBrowserLegacy source)

| Type | Color Name | RGB | Notes |
|------|-----------|-----|-------|
| **Normal damage (to monsters)** | White | `(1.0, 1.0, 1.0)` | Clean white digits |
| **Player-received damage** | Red | `(1.0, 0.0, 0.0)` | Pure red when PC takes damage |
| **Critical hit** | Yellow | `(0.9, 0.9, 0.15)` | Yellow-gold digits |
| **Combo total** | Yellow | `(0.9, 0.9, 0.15)` | Same yellow as crit |
| **Heal** | Green | `(0.0, 1.0, 0.0)` | Pure green |
| **SP recovery** | Blue | `(0.13, 0.19, 0.75)` | Deep blue |
| **Crit background** | Grey | `(0.66, 0.66, 0.66)` | Starburst sprite tint |
| **Blue combo/crit bg** | Blue | Uses `bluemsg.spr` | Alternative skin variant |

### Color Application Method

RO uses **white base sprites** that are **tinted by color multiplication** at render time. The digit sprites in the SPR file are white with dark outlines baked in. The tint color is applied as a shader multiply:

```
finalPixelColor = spritePixelColor * tintColor
```

This means the dark outline pixels (near black) stay dark regardless of tint, while the white digit body takes on the tint color. This is why RO damage numbers have such crisp, readable outlines — the outline is baked into the sprite, not rendered as a separate pass.

---

## 6. Critical Hit Treatment

### Multi-Layer Display

Critical hits in RO are visually distinct through **three layers**:

1. **Background starburst sprite** — a spiky "explosion" shape rendered BEHIND the number
   - Loaded from `msg.spr` frame 3 ("CritBg")
   - Tinted grey: `(0.66, 0.66, 0.66)`
   - Sized at **60%** of the original sprite dimensions: `width * 0.6`, `height * 0.6`
   - Offset: `(0, -6)` pixels vertically (slightly above center)
   - Same arc animation as the damage number (moves with it)

2. **Yellow damage digits** — `(0.9, 0.9, 0.15)` tint on white digit sprites
   - Same 4x→0x shrink curve as normal damage
   - Same 1500ms duration

3. **Impact effect** — `EffectManager.spam(effectId=1)` at target position
   - This is a small visual burst separate from the damage number

### Visual Comparison

```
Normal hit:   [white number]     arcs away
Critical hit: [grey starburst] + [yellow number]  arcs away together
```

### No Screen Shake

Critical hits produce **NO screen shake** in RO Classic. The visual distinction is purely in the color change and starburst background.

---

## 7. Miss / Dodge / Block

### Rendering Style

Miss, Lucky Dodge, and Guard use **pre-rendered text sprites** from `msg.spr`, NOT composed digit sprites. They are fixed images of the words "Miss", "Lucky", "Guard".

### Animation

```javascript
// Fixed scale (no shrinking)
size = 0.5

// No horizontal drift
X = entity.position.X

// Linear vertical rise (no sine arc)
Z = entity.position.Z + 3.5 + (t * 7)
```

| Parameter | Value |
|-----------|-------|
| Scale | 0.5x fixed (no change) |
| Duration | 800ms |
| Start Z | +3.5 units above entity |
| End Z | +10.5 units above entity |
| X Motion | None |
| Alpha | Linear fade: `1.0 - t` |

The key difference from damage numbers: **no arc, no shrink, no diagonal drift**. Text just rises straight up and fades out quickly.

### Sprite Frame Mapping (msg.spr)

| Frame | Content |
|-------|---------|
| 0 | "Miss" text |
| 1 | "Guard" text |
| 2 | "Crit" text (newer skins only) |
| 3 | "CritBg" — critical starburst background |
| 4 | "LuckyBg" — lucky dodge background (not commonly used) |
| 5 | "Lucky" text |

---

## 8. Heal Numbers

### Unique Animation

Heals have a distinct two-phase animation that feels different from damage:

**Phase 1 — Stationary (0-40% of lifetime, ~600ms):**
```javascript
// No movement at all during first 40%
X = entity.position.X
Y = entity.position.Y
Z = entity.position.Z + 2   // sits at base offset
```

**Phase 2 — Rising (40-100% of lifetime, ~900ms):**
```javascript
// Begins rising after the 40% delay
Z = entity.position.Z + 2 + (adjusted_t * 5)
```

### Size Curve

```javascript
size = Math.max((1 - t * 2) * 3, 0.8)
```

Quick shrink from 3x to 0.8x in the first ~370ms, then holds at 0.8x for the remaining ~1130ms.

### Color

Pure green `(0.0, 1.0, 0.0)` — no element tinting applied to heals.

### Behavioral Notes

- Heal numbers display the **heal amount**, not the damage value
- They sit still for 600ms so the player can read the number, then float up
- The initial shrink draws attention, then the steady 0.8x size maintains readability during the rise
- No diagonal drift — pure vertical motion

---

## 9. Combo / Multi-Hit

### Individual Hit Numbers

For multi-hit skills (Double Attack, Triple Attack, bolt skills):

- Each hit spawns a separate damage number with the **standard arc animation**
- Individual hits appear as semi-transparent, smaller numbers ("faint")
- Non-final combo numbers are **auto-removed at 15% of their animation time** (~225ms)

```javascript
if (!(flags & COMBO_FINAL) && t > 0.15) {
    // Remove this non-final combo number early
    remove();
}
```

### Combo Total Number

After all individual hits, the **yellow combo total** appears:

- Color: Yellow `(0.9, 0.9, 0.15)` — same as critical
- Duration: **3000ms** (double normal)
- Scale: Rapid pop-in from 0→3.5x in 50ms via `min(t, 0.05) * 70`
- Position: Higher above entity (+7 base Z offset)
- Motion: Slow rise, no arc: `Z = 7 + t` (barely moves upward)
- Only the **latest** combo total per entity is shown (older ones hidden)

### Display Order

```
Hit 1: [small white number] → removed at 225ms
Hit 2: [small white number] → removed at 225ms
Hit 3: [small white number] → removed at 225ms
...
Total: [BIG yellow total]   → stays for 3000ms
```

---

## 10. Font & Digit Rendering

### Sprite-Based Digits

RO uses **bitmap sprite fonts**, NOT vector/TrueType fonts. Digits 0-9 are stored as indexed-color sprites in SPR files:

- **File:** `data/sprite/이팩트/숫자.spr` (Effect/Numbers.spr)
- **Format:** 256-color indexed bitmaps, palette entry 0 = transparent
- **Style:** Bold, chunky, low-resolution digits with **baked dark outlines**
- **4 font skins available:** Default, NewNumber, Han, Invi/NewNumberH

### Digit Composition

When damage occurs, individual digit sprites are composed into a single texture:

1. Convert damage number to string (e.g., 1234 → "1", "2", "3", "4")
2. Calculate total width: sum of digit widths + 2px padding between each
3. Create power-of-two canvas (e.g., 64x32)
4. Draw digits left-to-right, centered on canvas
5. Upload to GPU as a texture
6. Render as billboard sprite in 3D space with color tint

### Outline

The outlines are **baked into the bitmap sprites** — dark pixels surround each digit shape in the SPR file itself. There is NO separate outline rendering pass. When color tinting is applied (multiply), the dark outline pixels stay dark because `dark * color ≈ dark`, while white digit pixels take on the tint color.

This is why RO damage numbers are so readable — the outline is pixel-perfect and always contrasts with the digit fill.

### Our UE5 Adaptation

Since we use Slate text rendering (not bitmap sprites), we achieve a similar look with:
- `FSlateFontInfo` with outline settings
- Per-digit `MakeText()` calls for digit spread effect
- Outline color near-black for contrast

To get closer to the RO bitmap look, we could:
- Use a bitmap font texture atlas (most faithful)
- Or use a bold, chunky font with thick outline (pragmatic)

---

## 11. Hit Impact Effects

### Target Flinch (HURT Animation)

When a target is hit in RO Classic, it plays a **HURT animation frame** — a brief recoil/flinch pose:

- **Player HURT:** Action index 6 in the player's sprite ACT file
- **Monster HURT:** Action index 3 in the monster's sprite ACT file
- **Duration:** Controlled by `dmotion` (damage motion) stat — varies per monster template
- **Movement interrupt:** Walking is interrupted when the HURT animation plays
- **Endure bypass:** If the target has Endure buff active, the HURT animation is suppressed — the target takes damage but does NOT flinch and is NOT interrupted

### Impact Flash Sprite

A small burst/flash sprite appears at the target's position on hit:

- **Effect IDs 0-5:** Basic "Attack Display" impact sprites — small white flash/star
- **Effect ID 81:** Generic "Skill hit" impact
- **Effect ID 131:** "Auto-Counter Hit" impact
- Different weapon types can trigger different hit effect IDs

### What RO Does NOT Do

- **NO screen shake** on normal hits or crits (reserved for AoE skills like Meteor Storm, LoV, Earthquake)
- **NO sprite color flash** (no white blink, no red tint on hit)
- **NO hit-stop / freeze frames** (no brief pause on impact)
- Visual feedback is entirely: flinch animation + impact sprite + damage number + sound

### Our Adaptation Options

Since our sprites are 2D atlas-based (not traditional RO SPR sprites), we can implement flinch as:
1. A brief animation state change to a "hurt" frame (if we have one in the atlas)
2. A quick position jitter/recoil on the sprite actor
3. A brief white flash material overlay on the sprite
4. A small Niagara particle burst at the impact point

---

## 12. Sound Design

### Weapon-Type Hit Sounds

Each weapon type produces a distinct hit sound when striking a target:

| Weapon Type | Sound Character |
|-------------|----------------|
| Sword | Sharp metallic slash |
| Dagger | Quick light stab |
| Axe | Heavy thunk |
| Mace | Blunt impact |
| Staff | Light tap |
| Bow | Arrow impact thud |
| Katar | Double slash |
| Knuckle | Punch impact |

### Sound Trigger

Hit sounds play at the **moment the damage number first appears**, triggered within the `Damage.add()` function. Sounds are positional — played at the entity's world position.

### Job-Specific Sounds

- When a **PC is hit**, a job-specific hurt vocalization plays (retrieved via job class)
- When a **monster is hit**, the weapon-specific hit sound plays (retrieved via attacker's weapon type)
- **Endure hits** play `player_metal.wav` (metallic clang) in addition to normal hit sound

### Our Adaptation

We currently have no hit sounds. Adding weapon-type hit sounds would be one of the highest-impact improvements for combat feel, alongside the damage number animation changes.

---

## 13. Server Damage Types (rAthena)

The server tells the client HOW to display damage via an enum in the damage packet:

```cpp
enum e_damage_type : uint8_t {
    DMG_NORMAL      = 0,   // Standard damage (white numbers)
    DMG_PICKUP_ITEM = 1,   // Pick up item (not damage)
    DMG_SIT_DOWN    = 2,   // Sit down (not damage)
    DMG_STAND_UP    = 3,   // Stand up (not damage)
    DMG_ENDURE      = 4,   // Damage with endure (no flinch animation)
    DMG_SPLASH      = 5,   // Splash/AoE damage
    DMG_SINGLE      = 6,   // Single-target skill damage
    DMG_REPEAT      = 7,   // Repeated damage
    DMG_MULTI_HIT   = 8,   // Multi-hit damage (combo individual)
    DMG_MULTI_HIT_ENDURE = 9,  // Multi-hit with endure
    DMG_CRITICAL    = 10,  // Critical hit (yellow + starburst)
    DMG_LUCY_DODGE  = 11,  // Lucky dodge / perfect dodge (LUK)
    DMG_TOUCH       = 12,  // Touch skill
    DMG_MULTI_HIT_CRITICAL = 13, // Multi-hit critical (2016+ client)
    DMG_SPLASH_ENDURE = 14 // Splash against endure target
};
```

### Damage Packet Fields

```cpp
void clif_damage(
    block_list& src,      // Attacker
    block_list& dst,      // Target
    t_tick tick,           // Server tick
    int32 sdelay,         // Source delay (amotion — attack animation ms)
    int32 ddelay,         // Destination delay (dmotion — flinch duration ms)
    int64 sdamage,        // Primary damage
    int16 div,            // Number of hits
    e_damage_type type,   // Display type (controls client rendering)
    int64 sdamage2,       // Secondary damage (dual wield left hand)
    bool spdamage         // SP damage flag
);
```

---

## 14. Our Current Implementation vs RO Classic

### Complete Gap Analysis

| Feature | RO Classic | Our Current | Priority |
|---------|-----------|-------------|----------|
| **Arc trajectory** | Sine parabolic arc + diagonal drift | Simple ease-out vertical rise | Critical |
| **Size scaling** | 4x→0x linear shrink | Fixed font size, no scaling | Critical |
| **Alpha fade** | Immediate linear 1.0→0.0 | Hold until 65%, then fade | High |
| **Normal damage color** | **White** (1.0, 1.0, 1.0) | **Yellow** (inverted!) | High |
| **Critical color** | **Yellow** (0.9, 0.9, 0.15) + starburst bg | **White**, larger, thicker outline | High |
| **Starburst background** | Grey spiky sprite behind crit number | None | High |
| **Duration** | 1500ms normal, 800ms miss, 3000ms combo | 1300ms for all | Medium |
| **Miss/Dodge anim** | Fixed scale, linear rise, no arc | Same anim as damage | Medium |
| **Heal animation** | Stationary 600ms then rise, shrink→hold | Same anim as damage | Medium |
| **Combo total** | Big yellow, rapid 50ms pop-in, 3000ms | Not implemented | Low (future) |
| **Diagonal drift** | Drifts upper-right during arc | Pure vertical, no drift | High |
| **Hit flinch** | HURT animation on target sprite | None | Medium |
| **Hit flash** | Small impact sprite at contact point | None | Medium |
| **Hit sounds** | Weapon-specific, positional | None | Medium |
| **Digit spread** | Composed from sprite sheet, 2px gap | Per-digit with fan-out effect | Keep (ours is fine) |
| **Outline** | Baked into bitmap sprites | Slate outline (2-3px) | Keep (ours is fine) |
| **Pool/perf** | GPU textures, deleted on expire | 64-entry circular buffer | Keep (ours is fine) |
| **Element tinting** | Not in original RO (added by us) | 40% blend with base color | Keep (our enhancement) |
| **Status text** | Not in original RO (added by us) | "Poisoned!" etc floating text | Keep (our enhancement) |

### What to Change (Ordered by Impact)

1. **Animation curve** — Replace ease-out rise with sine arc + diagonal drift + scale-down
2. **Color swap** — Normal=White, Crit=Yellow (currently inverted)
3. **Crit starburst** — Add grey starburst texture behind crit numbers
4. **Per-type timing** — 1500ms damage, 800ms miss, 3000ms combo
5. **Miss/Dodge** — Fixed scale, straight rise, shorter duration
6. **Heal** — Two-phase (stationary then rise), quick shrink to hold
7. **Hit impact** — Sprite flash/particle + positional hit sound
8. **Alpha** — Start fading immediately instead of holding

### What to Keep (Our Improvements Over RO)

- Element tinting on damage numbers (RO doesn't have this — good enhancement)
- Status effect floating text ("Poisoned!", "Stunned!" etc.)
- Per-digit fan-out spread effect
- DPI-aware scaling
- Options menu integration (toggle, scale multiplier)
- Dual-wield damage number separation

---

## 15. Source References

### Primary Sources (Code)

| Source | URL | What It Contains |
|--------|-----|-----------------|
| roBrowser Damage.js | `github.com/vthibault/roBrowser/blob/master/src/Renderer/Effects/Damage.js` | Original RO damage number rendering (animation curves, colors, timing) |
| roBrowserLegacy Damage.js | `github.com/MrAntares/roBrowserLegacy/blob/master/src/Renderer/Effects/Damage.js` | Expanded version with 4 directions, crit/lucky/endure/combo support |
| rAthena clif.hpp | `github.com/rathena/rathena/blob/master/src/map/clif.hpp` | `e_damage_type` enum, `clif_damage()` signature |
| rAthena multi-hit crit PR | `github.com/rathena/rathena/pull/2982` | DMG_MULTI_HIT_CRITICAL addition |

### Secondary Sources (Documentation & Assets)

| Source | URL | What It Contains |
|--------|-----|-----------------|
| Ragnarok Research Lab — SPR format | `ragnarokresearchlab.github.io/file-formats/spr/` | SPR sprite file format (how digit bitmaps are stored) |
| RO File Formats — ACT spec | `github.com/rdw-archive/RagnarokFileFormats/blob/master/ACT.MD` | ACT animation file format |
| RO File Formats — SPR spec | `github.com/Duckwhale/RagnarokFileFormats/blob/master/SPR.MD` | Alternate SPR documentation |
| Spriters Resource — RO damage sprites | `spriters-resource.com/pc_computer/ragnarokonline/asset/42205/` | Extracted damage digit sprite sheets |
| High-res scrolling damage sprites | `github.com/eleriaqueen/rag-highres-scrolling-dmg-sprites` | HD replacements for RO damage font |
| RO Effects list (browedit) | `github.com/CairoLee/Ragnarok.Tools/blob/master/ClientTools/browedit/data/effects.txt` | Complete effect ID → name mapping |

### Community Sources

| Source | URL | What It Contains |
|--------|-----|-----------------|
| rAthena forum — HD damage font | `rathena.org/board/files/file/3978-high-definition-damage-font/` | HD damage digit replacement mod |
| rAthena forum — original damage font | `rathena.org/board/topic/148688-ragnarok-onlines-original-damage-font/` | Discussion of original RO font style |
| rAthena forum — screen shake | `rathena.org/board/topic/107058-how-to-disable-screen-shaking-skills/` | Screen shake skill list, /quake toggle |
| Medium — Damage Numbers in RPGs | `shweep.medium.com/damage-numbers-in-rpgs-1f0e3b1bc23a` | General damage number design analysis |
| Critical Hit wiki | `ragnarok-online-encyclopedia.fandom.com/wiki/Critical_Hit` | RO crit mechanics reference |
| rAthena wiki — Acts | `github.com/rathena/rathena/wiki/Acts` | ACT animation system documentation |
