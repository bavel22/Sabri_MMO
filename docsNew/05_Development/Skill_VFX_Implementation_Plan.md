# Skill VFX Implementation Plan — Sabri_MMO

**Created**: 2026-03-05
**Research Sources**: 60+ (iRO Wiki, rAthena, Ragnarok Research Lab, Epic Documentation, GitHub, Fab, CGHOW, ArtStation, EmberGen, ZibraVDB, MCP repos, community forums, YouTube, Reddit, The Spriters Resource)

---

## Table of Contents

1. [Current State](#current-state)
2. [RO Classic Skill Visual Reference](#ro-classic-skill-visual-reference)
3. [Implementation Plans (Ranked)](#implementation-plans-ranked)
4. [Plan 1: Niagara + AI Textures + C++ Subsystem (RECOMMENDED)](#plan-1-niagara--ai-textures--c-subsystem-recommended)
5. [Plan 2: MCP Server Automation (ChiR24/Unreal_mcp)](#plan-2-mcp-server-automation-chir24unreal_mcp)
6. [Plan 3: EmberGen Flipbook Pipeline (Most RO-Authentic)](#plan-3-embergen-flipbook-pipeline-most-ro-authentic)
7. [Plan 4: Marketplace Pack + Customization (Fastest)](#plan-4-marketplace-pack--customization-fastest)
8. [Plan 5: Hybrid AI Generation Pipeline](#plan-5-hybrid-ai-generation-pipeline)
9. [Comparison Matrix](#comparison-matrix)
10. [The Casting Circle — Detailed Implementation](#the-casting-circle--detailed-implementation)
11. [Per-Skill VFX Specifications](#per-skill-vfx-specifications)
12. [C++ Architecture — SkillVFXSubsystem](#c-architecture--skillvfxsubsystem)
13. [Free Resources Inventory](#free-resources-inventory)
14. [Sources](#sources)

---

## Current State

### Skills Needing VFX (15 implemented server-side)

| Skill | ID | Class | Target Type | VFX Priority |
|-------|-----|---------|------------|-------------|
| First Aid | 2 | Novice | Self | Low |
| Bash | 103 | Swordsman | Single | High |
| Provoke | 104 | Swordsman | Single | Medium |
| Magnum Break | 105 | Swordsman | AoE (self) | High |
| Endure | 106 | Swordsman | Self | Low |
| Cold Bolt | 200 | Mage | Single | High |
| Fire Bolt | 201 | Mage | Single | High |
| Lightning Bolt | 202 | Mage | Single | High |
| Napalm Beat | 203 | Mage | Single (splash) | Medium |
| Sight | 205 | Mage | Self | Medium |
| Stone Curse | 206 | Mage | Single | Medium |
| Fire Ball | 207 | Mage | Single (AoE splash) | High |
| Frost Diver | 208 | Mage | Single | High |
| Fire Wall | 209 | Mage | Ground | High |
| Safety Wall | 211 | Mage | Ground | High |
| Thunderstorm | 212 | Mage | Ground | High |
| **Warp Portal** | N/A | System | Ground | High |
| **Casting Circle** | N/A | System | Ground | **Critical** |

### Existing VFX Infrastructure
- `NS_Damage.uasset` — Niagara damage number system (working)
- `DamageNumberSubsystem` — spawns floating damage text
- `CastBarSubsystem` — cast time progress bar (Slate, no particles)
- `SSkillTargetingOverlay` — crosshair overlay during targeting (Slate, no ground indicator)
- Blueprint hooks: `BP_PlayDamageDealEffect()`, `BP_PlayDamageReceivedEffect()` (not yet connected)
- All socket events include `targetX/Y/Z` coordinates for world-space placement
- `Niagara` and `NiagaraCore` modules **NOT yet added** to `SabriMMO.Build.cs`

---

## RO Classic Skill Visual Reference

### The Iconic Casting Circle (magic_target.tga)

The single most important visual to replicate. In RO Classic:

- **Texture**: `data\texture\effect\magic_target.tga` — a flat circular glyph with concentric rings and runic symbols
- **Color**: Blue-white/cyan, semi-transparent, with additive blending
- **Animation**: Rotates slowly clockwise at the caster's feet (self-cast) or target location (ground-target)
- **Size**: Scales to match the spell's AoE footprint (5x5 cells for Thunderstorm, 1x1 for Safety Wall)
- **Duration**: Persists for the entire cast time, disappears instantly on cast complete/interrupt
- **Element-colored variants**: Fire=red, Water=blue, Wind=yellow, Earth=brown, Holy=white, Poison=green
  - Effect IDs 54-59 (EF_BEGINSPELL2-7) for element-specific cast auras

### Per-Skill Visual Summary

| Skill | VFX Type | Description | Colors | Behavior |
|-------|----------|-------------|--------|----------|
| **First Aid** | Self flash | Green cross + sparkle particles floating up | Green/white | One-shot 0.3s |
| **Bash** | Impact flash | Large white-yellow starburst slash at target | White-yellow | One-shot 0.2s |
| **Magnum Break** | AoE burst | Expanding fire ring from caster + flame particles | Orange/red/yellow | One-shot 0.5s |
| **Provoke** | Target flash | Red "!" icon above target + red pulse | Red | One-shot 0.3s |
| **Endure** | Self aura | Golden aura flash around caster | Yellow-gold | One-shot 0.5s |
| **Cold Bolt** | Bolts from sky | Ice shards descend onto target (1 per level) | Ice blue/cyan/white | Sequence, 0.15s/bolt |
| **Fire Bolt** | Bolts from sky | Fire pillars descend onto target (1 per level) | Orange/red/yellow | Sequence, 0.15s/bolt |
| **Lightning Bolt** | Bolts from sky | Jagged lightning strikes from sky (1 per level) | White-yellow/electric blue | Sequence, 0.15s/bolt |
| **Soul Strike** | Projectiles | Ghost orbs travel from caster to target (1-5 hits) | White/purple/blue | Sequence projectiles |
| **Napalm Beat** | Impact cluster | Purple puff explosions at target (3x3 area) | Purple/violet/magenta | One-shot burst 0.4s |
| **Fire Ball** | Projectile + AoE | Fireball arcs to target, explodes (5x5 area) | Orange/red/yellow | One-shot arc + explosion |
| **Frost Diver** | Ground projectile | Ice spike slides along ground to target, freeze encasement | Ice blue/white | Projectile + persistent freeze |
| **Fire Wall** | Ground persistent | 1x3 line of fire columns on ground | Orange/red/yellow | Looping until depleted |
| **Safety Wall** | Ground persistent | Pink-white light pillar on 1 cell | Pink/magenta/white | Looping until broken |
| **Thunderstorm** | Ground AoE | Lightning bolts rain randomly in 5x5 area | White-yellow/blue | Sequence over AoE |
| **Sight** | Orbiting particle | Small fireball orbits caster at waist height | Orange/red | Looping 10s |
| **Stone Curse** | Status overlay | Gray stone texture creeps over target | Dark gray/brown | Progressive + persistent |
| **Warp Portal** | Ground persistent | Golden ring on ground + spiraling particles rising upward | Yellow-gold + white-blue | Looping |
| **Casting Circle** | Ground indicator | Rotating runic circle at target location during cast | Blue-white (or element-colored) | Looping during cast |

---

## Implementation Plans (Ranked)

### Quick Comparison

| Plan | Time to First VFX | Total Time | Cost | RO Accuracy | Automation | Difficulty |
|------|-------------------|-----------|------|-------------|-----------|------------|
| **1. Niagara + AI Textures** | 2-3 days | 2-3 weeks | $0-20 | 85% | Medium | Medium |
| **2. MCP Server Automation** | 3-5 days setup | 2-3 weeks | $0 | 75% | **High** | High |
| **3. EmberGen Flipbook** | 1 week setup | 3-4 weeks | $20-40/mo | **95%** | Medium | Medium-High |
| **4. Marketplace Pack** | **1 day** | **3-5 days** | $30-60 | 60% | Low | **Easy** |
| **5. Hybrid AI Pipeline** | 3-4 days | 2-3 weeks | $10-30 | 80% | High | Medium |

---

## Plan 1: Niagara + AI Textures + C++ Subsystem (RECOMMENDED)

**Why #1**: Best balance of quality, speed, cost, and maintainability. Builds skills you'll reuse for all future classes.

### Overview
1. Download Epic's free Niagara Examples Pack (50+ systems)
2. Generate custom spell textures with AI tools (Midjourney, SEELE AI, or Stable Diffusion)
3. Build a C++ `USkillVFXSubsystem` that maps skill IDs → Niagara systems
4. Create the casting circle as an animated decal material
5. Build 6-8 parameterized Niagara templates that cover ALL skills through color/scale variation

### Step-by-Step

#### Step 1: Add Niagara to Build.cs (5 minutes)
```cpp
// In SabriMMO.Build.cs, add to PublicDependencyModuleNames:
"Niagara", "NiagaraCore"
```

#### Step 2: Download Free Resources (30 minutes)
- **Niagara Examples Pack** from Fab (free) — 50+ systems including explosions, lightning, buffs, impacts
  - URL: https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600
- **Free Niagara Magic Circle** from ArtStation (free)
  - URL: https://www.artstation.com/marketplace/p/dBJmq/unreal-engine-niagara-magical-circle-download-free-file-ue4-ue5

#### Step 3: Generate Custom Textures with AI (1-2 hours)
Use any of these tools to generate magic circle textures, spell effect sprites, rune patterns:

| Tool | Type | Best For | Cost |
|------|------|----------|------|
| **SEELE AI** (seeles.ai) | Text-to-sprite sheet | Fire/ice/lightning flipbook animations | Free |
| **Midjourney** | Text-to-image | Magic circle textures, rune patterns | $10/mo |
| **DALL-E 3** (via ChatGPT) | Text-to-image | Quick concept generation | Included in ChatGPT+ |
| **OpenArt Sprite Generator** (openart.ai) | Text-to-sprite | Spell effect sprite sequences | Free |
| **Stable Diffusion** (local or via UE plugin) | Text-to-image | Bulk texture generation | Free (local) |

Example prompts for generating textures:
```
"Magic circle with runic symbols, glowing cyan blue, transparent background,
top-down view, game asset, clean vector style" (for casting circle)

"Fire bolt impact explosion, orange red flames, sprite sheet 4x4 grid,
transparent background, game VFX asset" (for Fire Bolt impact)

"Ice shard crystal, angular jagged, cyan blue translucent, side view,
transparent background, game asset" (for Cold Bolt projectile)
```

#### Step 4: Create the Casting Circle Decal (2-3 hours)
This is the most critical VFX. Detailed implementation in [The Casting Circle section](#the-casting-circle--detailed-implementation).

#### Step 5: Build the SkillVFXSubsystem (1-2 days)
Create a new C++ `UWorldSubsystem` that:
- Listens to existing socket events (`skill:cast_start`, `skill:effect_damage`, `skill:buff_applied`)
- Maps each skill ID to a Niagara system + parameters
- Spawns effects at the correct world locations
- Manages persistent effects (Fire Wall, Safety Wall, Sight aura, status effects)

Full architecture in [C++ Architecture section](#c-architecture--skillvfxsubsystem).

#### Step 6: Create Parameterized Niagara Templates (3-5 days)
Instead of creating 20+ unique Niagara systems, create **8 parameterized templates**:

| Template | Used By | Exposed Parameters |
|----------|---------|-------------------|
| `NS_BoltFromSky` | Cold/Fire/Lightning Bolt | Color, BoltCount, BoltMesh/Sprite, ImpactColor |
| `NS_Projectile` | Soul Strike, Fire Ball, Frost Diver | Color, Speed, Scale, TrailColor, ProjectileMesh |
| `NS_AoEImpact` | Magnum Break, Fire Ball explosion, Napalm Beat | Color, Radius, ParticleCount, ExplosionForce |
| `NS_GroundPersistent` | Fire Wall, Safety Wall | Color, CellCount, ColumnHeight, PulseSpeed |
| `NS_GroundAoERain` | Thunderstorm | Color, Radius, StrikeCount, StrikeInterval |
| `NS_SelfBuff` | Endure, Energy Coat, Sight | Color, OrbCount, OrbitRadius, OrbitSpeed |
| `NS_TargetDebuff` | Provoke, Stone Curse, Frost Diver freeze | Color, OverlayIntensity, Duration |
| `NS_HealFlash` | First Aid | Color, ParticleCount, RiseSpeed |

Each template is ONE Niagara system with User Parameters that C++ sets per skill. This means:
- Fire Bolt = `NS_BoltFromSky` with Color=(1, 0.3, 0, 1), BoltMesh=FirePillar
- Cold Bolt = `NS_BoltFromSky` with Color=(0.3, 0.8, 1, 1), BoltMesh=IceShard
- Lightning Bolt = `NS_BoltFromSky` with Color=(1, 1, 0.5, 1), BoltMesh=LightningJagged

#### Step 7: Integrate with Warp Portal Actor (1-2 hours)
Add a Niagara component to your existing `WarpPortal` actor:
- Golden ground ring (decal or mesh)
- Spiraling particles rising upward
- Looping while portal is active

### Pros
- Best long-term maintainability (parameterized templates scale to all future classes)
- Free or very cheap ($0-20 for AI textures)
- Full C++ control matches your existing architecture
- Niagara is the engine's native VFX system (best performance, best tooling)
- AI-generated textures are unique to your game

### Cons
- Requires learning Niagara editor basics (2-4 hours)
- Some Niagara template creation is manual
- AI-generated textures may need cleanup in an image editor

---

## Plan 2: MCP Server Automation (ChiR24/Unreal_mcp)

**Why this exists**: There IS an MCP server specifically built to create Niagara systems programmatically via Claude.

### Overview
Install the [ChiR24/Unreal_mcp](https://github.com/ChiR24/Unreal_mcp) MCP server, which provides a `manage_effect` tool that can create and manipulate Niagara systems, emitters, modules, and particles. This would let Claude Code directly build spell effects in your UE5 editor.

### What ChiR24/Unreal_mcp Offers
- **36 MCP tools** covering: Blueprints, Materials, Textures, Static Meshes, Skeletal Meshes, Levels, Sounds, **Particles**, **Niagara Systems**, Behavior Trees
- Key tools for VFX:
  - `manage_effect` — Create/manipulate Niagara systems, emitters, modules, GPU simulations
  - `manage_material_authoring` — Create materials, expression graphs (for decals)
  - `manage_texture` — Create/modify textures, compression settings

### Step-by-Step

#### Step 1: Install the C++ Automation Bridge Plugin
The MCP server requires a native UE5 plugin installed in your project:
```
1. Clone https://github.com/ChiR24/Unreal_mcp
2. Copy the AutomationBridge plugin to client/SabriMMO/Plugins/
3. Enable plugin in .uproject
4. Build the project
```

#### Step 2: Configure MCP Server
```json
// In your Claude Code MCP settings:
{
  "mcpServers": {
    "unreal_mcp": {
      "command": "node",
      "args": ["path/to/ChiR24/Unreal_mcp/server/index.js"],
      "env": {}
    }
  }
}
```

#### Step 3: Use Claude to Create Niagara Effects
Once connected, Claude could issue commands like:
```
manage_effect({
  action: "create_niagara_system",
  name: "NS_FireBolt",
  emitters: [{
    name: "FirePillar",
    spawn_rate: 10,
    lifetime: 0.5,
    renderer: "sprite",
    material: "M_FireBolt",
    color: [1.0, 0.3, 0.0, 1.0]
  }]
})
```

### Pros
- Highest automation — Claude creates effects directly in UE5 editor
- No manual Niagara editor work
- Can iterate quickly through conversation

### Cons
- **Untested with UE 5.7** — may have compatibility issues
- Requires building a C++ plugin (compile step)
- Quality of generated effects depends on how well MCP tools map to Niagara internals
- The `manage_effect` tool's actual capabilities may be limited (documentation is sparse)
- You already have a working MCP server (flopperam/unreal-engine-mcp) — potential conflicts
- **Risk**: If the plugin doesn't compile or the Niagara control is too limited, you've wasted setup time

### Verdict
Worth exploring as an **add-on** to Plan 1 for generating base Niagara systems, but don't depend on it as the primary approach. The plugin is relatively new and may not handle complex Niagara configurations.

---

## Plan 3: EmberGen Flipbook Pipeline (Most RO-Authentic)

**Why this exists**: RO's original effects are 2D sprite-based. EmberGen generates GPU-simulated fire/smoke/explosions and exports them as flipbook sprite sheets — the exact format RO used, but at modern quality.

### Overview
Use [EmberGen by JangaFX](https://jangafx.com/software/embergen) to simulate fire, ice, lightning, and explosion effects in real-time, then export as flipbook sprite sheets. Import these into UE5 as SubUV materials for Niagara or on billboard meshes.

### Step-by-Step

#### Step 1: Get EmberGen ($20-40/month indie license)
- Download from https://jangafx.com/software/embergen
- Free trial available

#### Step 2: Simulate Each Spell Effect
EmberGen has presets for fire, smoke, explosions. For each spell:
- Fire Bolt: Simulate a vertical fire column descending
- Cold Bolt: Simulate ice particle burst (use velocity + blue color)
- Lightning Bolt: Use the built-in lightning preset
- Magnum Break: Simulate radial explosion
- Fire Wall: Simulate looping fire column
- Frost Diver: Simulate ice wave traveling horizontally

#### Step 3: Export as Flipbook Sprite Sheets
EmberGen exports directly as assembled sprite sheet PNGs:
- Set resolution (e.g., 128x128 per frame)
- Set grid (e.g., 8x8 = 64 frames)
- Export with transparent background
- Output: One PNG per effect, ready for UE5

#### Step 4: Import into UE5 and Create Materials
```
For each sprite sheet:
1. Import as Texture (disable sRGB for additive effects)
2. Create Material:
   - Blend Mode = Additive (fire, lightning) or Translucent (ice, healing)
   - Use SubUV / Flipbook node with rows=8, cols=8
   - Connect Time * PlaybackSpeed to Flipbook input
3. Create Niagara Emitter using this material as Sprite Renderer
```

#### Step 5: Build same SkillVFXSubsystem as Plan 1

### Pros
- **Most RO-authentic look** — flipbook sprites in 3D world matches RO's aesthetic perfectly
- EmberGen simulations look professional (used in AAA games)
- Each effect takes ~5-10 minutes to simulate and export
- Consistent art style across all effects

### Cons
- Monthly cost ($20-40)
- Requires learning EmberGen (1-2 hours, intuitive UI)
- Generated effects are 2D (no volumetric 3D) — but this IS the RO style
- Still need to build the Niagara integration + C++ subsystem

---

## Plan 4: Marketplace Pack + Customization (Fastest)

**Why this exists**: If you want VFX working TODAY with minimal effort.

### Overview
Purchase 1-2 fantasy spell VFX packs from Fab/Marketplace, customize colors to match RO, and wire them into your skill system.

### Recommended Packs

| Pack | Contents | Estimated Cost |
|------|----------|---------------|
| **Fantasy RPG VFX Pack** | 87 systems: Fire, Ice, Arcane, Blood, Nature, Lightning. AoE, Aura, Beam effects. Niagara + Cascade. | ~$30 |
| **Niagara Magic Projectiles VFX** | 59 projectiles with cast/impact: Healing, Ice, Lightning | ~$20 |
| **Stylised Niagara Spell FX** | 25 VFX: ice, magic, fire, lightning, shadow, holy, healing | ~$15 |

### Step-by-Step

#### Step 1: Purchase and Import Pack (30 minutes)
#### Step 2: Map Pack Effects to Your Skills (2-3 hours)
- Browse the pack's gallery level
- Identify which effect maps to each skill
- Note the asset paths

#### Step 3: Build SkillVFXSubsystem (Same as Plan 1 Step 5)
#### Step 4: Create Casting Circle Decal (Same as Plan 1 Step 4)

### Pros
- **Fastest time to working VFX** (1 day)
- Professional quality
- No art skills needed
- Pre-optimized for games

### Cons
- **Doesn't look like RO** — these packs are designed for modern 3D RPGs
- Every UE5 game using the same pack looks the same
- Limited customization (you get what you get)
- Cost adds up if you need multiple packs
- You'll likely replace these later with custom effects

---

## Plan 5: Hybrid AI Generation Pipeline

**Why this exists**: Combines multiple AI tools for maximum automation without depending on any single tool.

### Overview
Use a pipeline of AI tools to generate textures and sprite sheets, then assemble them in Niagara.

### Pipeline

```
Step 1: SEELE AI / OpenArt → Generate spell effect sprite sheets from text prompts (FREE)
Step 2: Midjourney / DALL-E → Generate magic circle textures, rune patterns ($10-20/mo)
Step 3: MCP game-asset-gen → Generate textures directly via Claude (FREE, GitHub MCP)
Step 4: Import all generated assets into UE5
Step 5: Create Niagara templates using generated sprites as SubUV materials
Step 6: Build SkillVFXSubsystem (same C++ system)
```

### MCP Asset Generation Servers

| MCP Server | What It Does | GitHub |
|------------|-------------|--------|
| **Flux159/mcp-game-asset-gen** | Generate images, textures, 3D models via DALL-E/Gemini/Fal.ai | github.com/Flux159/mcp-game-asset-gen |
| **MubarakHAlketbi/game-asset-mcp** | Generate 2D sprites and 3D models via HuggingFace AI | github.com/MubarakHAlketbi/game-asset-mcp |

These MCP servers can be installed alongside your existing unrealMCP and used to generate spell effect textures from text descriptions during our conversations.

### Step-by-Step

#### Step 1: Install mcp-game-asset-gen
```bash
git clone https://github.com/Flux159/mcp-game-asset-gen
cd mcp-game-asset-gen
npm install
```
Add to Claude Code MCP config.

#### Step 2: Generate Textures via Conversation
```
"Generate a 512x512 image: magic circle with runic symbols, glowing cyan,
transparent background, top-down view, game asset"
→ Outputs PNG texture file
```

#### Step 3: Generate Sprite Sheets via SEELE AI (Manual Step)
Go to seeles.ai, enter prompts for each spell effect, download sprite sheets.

#### Step 4: Import into UE5 + Build Niagara Templates + Subsystem

### Pros
- Most textures generated for free
- Can generate unlimited variations
- MCP integration means Claude can generate assets during conversation
- Unique art style

### Cons
- AI-generated art quality varies — may need manual cleanup
- Sprite sheet generation tools may produce inconsistent frame sizes
- Still requires Niagara knowledge to assemble into working effects
- Multiple tools = multiple workflows to manage

---

## Comparison Matrix

| Criteria | Plan 1 (Niagara+AI) | Plan 2 (MCP Auto) | Plan 3 (EmberGen) | Plan 4 (Marketplace) | Plan 5 (Hybrid AI) |
|----------|:---:|:---:|:---:|:---:|:---:|
| **Overall Rank** | **#1** | #4 | #2 | #3 | #5 |
| Time to first effect | 2-3 days | 3-5 days | ~1 week | **1 day** | 3-4 days |
| Total implementation | 2-3 weeks | 2-3 weeks | 3-4 weeks | 3-5 days | 2-3 weeks |
| Cost | $0-20 | $0 | $20-40/mo | $30-60 | $10-30 |
| RO Accuracy | 85% | 75% | **95%** | 60% | 80% |
| Art Uniqueness | High | Medium | High | Low | High |
| Automation Level | Medium | **High** | Medium | Low | High |
| Learning Curve | Medium | High | Medium | **Low** | Medium |
| Scalability (future classes) | **Excellent** | Good | Excellent | Poor | Good |
| Maintainability | **Excellent** | Unknown | Good | Good | Fair |
| Risk Level | Low | **High** | Low | **None** | Medium |

### My Recommendation

**Start with Plan 1**, enhanced by elements of Plan 3 and Plan 5:

1. **Week 1**: Set up Niagara in Build.cs, download free packs, create the casting circle decal, build SkillVFXSubsystem skeleton
2. **Week 2**: Create parameterized Niagara templates, generate custom textures with AI, wire up the 5 highest-priority skills (Casting Circle, Bolt trio, Magnum Break)
3. **Week 3**: Remaining skills, warp portal, polish and optimization

If you want the most RO-authentic look and have budget, add EmberGen (Plan 3) for the flipbook sprite sheets.

---

## The Casting Circle — Detailed Implementation

This is the #1 priority VFX. It's what makes RO casting "feel" like RO.

### What to Build
A rotating arcane circle that appears:
- At the **target location** during ground-target skill casting (Thunderstorm, Fire Wall, Safety Wall)
- Under the **caster's feet** during single-target skill casting (bolt spells)
- Color-coded by element (Fire=red-orange, Water=blue, Wind=yellow, Earth=brown, Holy=white, Poison=green, Ghost=purple, Neutral=cyan)
- Scales to match AoE radius
- Fades in on cast start, fades out on cast complete/interrupt

### Implementation: Decal Material Approach

#### Step 1: Create the Magic Circle Texture
Option A: Generate with AI (Midjourney prompt: "top-down view magic circle glowing runic symbols, concentric rings, arcane glyphs, transparent background, game asset, blue cyan glow")
Option B: Download free from ArtStation: https://www.artstation.com/marketplace/p/dBJmq/
Option C: Download from rAthena collection: https://rathena.org/board/topic/75230-magic-target-collectionfree-download/
Option D: Create procedurally in Material Editor (no texture needed — see below)

#### Step 2: Create the Decal Material (M_CastingCircle)

```
Material Settings:
  Material Domain = Deferred Decal
  Decal Blend Mode = Translucent

Node Graph:
  1. TextureCoordinate [0] → CustomRotator
     - Rotation Center = (0.5, 0.5)
     - Rotation Angle = Time * RotationSpeed (ScalarParameter, default 0.5)
  2. CustomRotator Output → TextureSample (your magic circle texture)
  3. TextureSample.RGB * ElementColor (VectorParameter, default cyan) → Emissive Color
  4. TextureSample.A * FadeAlpha (ScalarParameter, 0-1) → Opacity
  5. EmissiveIntensity (ScalarParameter, default 3.0) multiplied into Emissive

Exposed Parameters:
  - RotationSpeed (float): Controls rotation speed
  - ElementColor (FLinearColor): Skill element color
  - FadeAlpha (float): For fade in/out
  - EmissiveIntensity (float): Glow brightness
```

#### Step 2b: Procedural Ring (No Texture Needed)

If you want to skip texture generation entirely:
```
Material Graph (Procedural):
  1. Two RadialGradientExponential nodes:
     - Outer: Radius=0.5, Density=100
     - Inner: Radius=0.4, Density=100
  2. Subtract Inner from Outer → Ring shape
  3. Add a second ring pair (Radius=0.35/0.30) for inner ring
  4. Add rune pattern: Use a simple texture or noise for rune marks around the ring
  5. Apply UV rotation via CustomRotator + Time
  6. Multiply by ElementColor → Emissive Color
  7. Ring result → Opacity
```

#### Step 3: Create BP_CastingCircle Actor

```cpp
// CastingCircleActor.h
UCLASS()
class ACastingCircleActor : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere)
    UDecalComponent* CircleDecal;

    UPROPERTY(EditAnywhere, Category="VFX")
    UMaterialInterface* CastingCircleMaterial;

    void Initialize(FLinearColor ElementColor, float Radius, float Duration);
    void FadeOut(float FadeTime = 0.3f);

private:
    UMaterialInstanceDynamic* DynMaterial;
    float TotalDuration;
    float CurrentAlpha;
};
```

#### Step 4: Spawn from SkillVFXSubsystem

```cpp
// On skill:cast_start event:
void USkillVFXSubsystem::OnCastStart(int32 CasterId, int32 SkillId, float CastTime, FVector TargetLocation)
{
    FLinearColor ElementColor = GetElementColor(SkillId); // Fire=red, Water=blue, etc.
    float AoERadius = GetSkillAoERadius(SkillId);         // 500 for Thunderstorm, 100 for single-target

    ACastingCircleActor* Circle = GetWorld()->SpawnActor<ACastingCircleActor>(
        CastingCircleBP, TargetLocation, FRotator::ZeroRotator);
    Circle->Initialize(ElementColor, AoERadius, CastTime);

    ActiveCastingCircles.Add(CasterId, Circle);
}

// On skill:effect_damage or skill:cast_interrupted:
void USkillVFXSubsystem::OnCastEnd(int32 CasterId)
{
    if (ACastingCircleActor* Circle = ActiveCastingCircles.FindRef(CasterId))
    {
        Circle->FadeOut(0.3f); // 300ms fade out
        ActiveCastingCircles.Remove(CasterId);
    }
}
```

#### Element Color Map

```cpp
FLinearColor GetElementColor(int32 SkillId)
{
    // Get element from skill data
    FString Element = GetSkillElement(SkillId);

    if (Element == "fire")    return FLinearColor(1.0f, 0.3f, 0.05f, 1.0f);  // Orange-red
    if (Element == "water")   return FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);   // Blue
    if (Element == "wind")    return FLinearColor(0.9f, 1.0f, 0.3f, 1.0f);   // Yellow-green
    if (Element == "earth")   return FLinearColor(0.6f, 0.4f, 0.2f, 1.0f);   // Brown
    if (Element == "holy")    return FLinearColor(1.0f, 1.0f, 0.8f, 1.0f);   // White-gold
    if (Element == "dark")    return FLinearColor(0.3f, 0.0f, 0.5f, 1.0f);   // Dark purple
    if (Element == "ghost")   return FLinearColor(0.6f, 0.3f, 0.9f, 1.0f);   // Purple
    if (Element == "undead")  return FLinearColor(0.2f, 0.0f, 0.3f, 1.0f);   // Deep purple
    if (Element == "poison")  return FLinearColor(0.3f, 0.8f, 0.1f, 1.0f);   // Green
    return FLinearColor(0.3f, 0.8f, 1.0f, 1.0f);                             // Default: Cyan
}
```

---

## Per-Skill VFX Specifications

### Warp Portal

| Property | Value |
|----------|-------|
| **Niagara Template** | Custom `NS_WarpPortal` (unique, not parameterized) |
| **Components** | 1) Golden ground ring decal, 2) Spiraling particles column |
| **Ground Ring** | Decal material with golden concentric rings, slow rotation |
| **Particles** | 20-30 small bright sprites, spawning in ring pattern, rising upward in helix |
| **Height** | Particles rise 200-300 UE units above ground |
| **Color** | Ground: gold-yellow. Particles: white-yellow with slight blue tint |
| **Lifetime** | Permanent (looping while portal exists) |
| **Trigger** | Spawn when WarpPortal actor is in view, destroy when out of range |

### Bolt Skills (Cold/Fire/Lightning Bolt)

| Property | Cold Bolt | Fire Bolt | Lightning Bolt |
|----------|-----------|-----------|---------------|
| **Template** | `NS_BoltFromSky` | `NS_BoltFromSky` | `NS_BoltFromSky` |
| **Color** | (0.3, 0.8, 1.0) cyan | (1.0, 0.3, 0.0) orange | (1.0, 1.0, 0.5) yellow |
| **Bolt Shape** | Angular ice shard mesh | Fire pillar sprite | Jagged zigzag ribbon |
| **Bolt Count** | = Skill Level | = Skill Level | = Skill Level |
| **Bolt Interval** | 150-200ms | 150-200ms | 150-200ms |
| **Spawn Height** | 500 UE units above target | 500 UE units above target | 800 UE units above target |
| **Descent Speed** | Fast (0.1s travel) | Fast (0.1s travel) | Very fast (0.05s) |
| **Impact Effect** | Blue-white flash + ice shatter particles | Orange flash + flame particles | White-yellow flash + electric sparks |
| **Impact Sound** | Ice crack | Fire whoosh | Electric crack |

### Soul Strike

| Property | Value |
|----------|-------|
| **Template** | `NS_Projectile` |
| **Projectile Shape** | Translucent glowing orb (sprite, FaceCamera) |
| **Color** | White-purple glow (0.6, 0.3, 0.9) |
| **Trail** | Soft ribbon trail, pale purple |
| **Hit Count** | 1-5 based on level (fired in pairs at higher levels) |
| **Travel Speed** | 1500 UE units/sec |
| **Impact** | White-purple burst flash |
| **Spawn Offset** | From caster's chest height |

### Fire Ball

| Property | Value |
|----------|-------|
| **Template** | `NS_Projectile` (flight) + `NS_AoEImpact` (explosion) |
| **Projectile** | Large glowing orange sphere with fire trail |
| **Arc** | Parabolic (add upward initial velocity + gravity) |
| **Travel Time** | 0.3-0.5s depending on distance |
| **Explosion** | Expanding fire ring + scattered flame particles |
| **Explosion Radius** | 500 UE units (5x5 cells) |
| **Color** | Orange-red-yellow |

### Magnum Break

| Property | Value |
|----------|-------|
| **Template** | `NS_AoEImpact` centered on caster |
| **Effect** | Expanding fire ring burst from caster + flame particles radiating outward |
| **Radius** | 500 UE units |
| **Color** | Orange-red-yellow |
| **Duration** | 0.5s burst |
| **Additional** | Ground decal flash (ring_yellow) at caster's feet |

### Thunderstorm

| Property | Value |
|----------|-------|
| **Template** | `NS_GroundAoERain` |
| **Area** | 500 UE unit radius circle on ground |
| **Strikes** | Random positions within area, count = skill level |
| **Strike Interval** | 200ms (matching server multi-hit delay) |
| **Per-Strike** | Lightning bolt descends + ground flash + spark particles |
| **Color** | White-yellow with blue-electric tinge |
| **Casting Circle** | Yellow-green (wind element) rotating at target during cast |

### Fire Wall

| Property | Value |
|----------|-------|
| **Template** | `NS_GroundPersistent` |
| **Shape** | 1x3 line of fire columns (3 cells, 300 UE units long) |
| **Orientation** | Perpendicular to caster→target vector |
| **Column Height** | 200 UE units |
| **Flame Type** | Looping fire sprite (flipbook SubUV) or mesh flame particles |
| **Color** | Orange-red-yellow |
| **Duration** | Persistent until server sends removal event |
| **Spawning** | 3 separate flame emitters or 1 long emitter with spawn along line |

### Safety Wall

| Property | Value |
|----------|-------|
| **Template** | `NS_GroundPersistent` |
| **Shape** | Vertical light pillar on single cell |
| **Column Height** | 300 UE units |
| **Color** | Pink-magenta-white (1.0, 0.4, 0.6) |
| **Effect** | Soft glowing translucent column + gentle pulse |
| **Base** | Small circular ground ring decal |
| **Duration** | Persistent until server sends removal event |

### Frost Diver

| Property | Value |
|----------|-------|
| **Template** | `NS_Projectile` (spike travel) + `NS_TargetDebuff` (freeze overlay) |
| **Projectile** | Ice spike/wave traveling along ground from caster to target |
| **Speed** | Very fast (3000 UE units/sec) |
| **Impact** | Ice shatter particles + cold mist puff |
| **Freeze Effect** | Translucent blue-white ice overlay on frozen target |
| **Freeze Duration** | Matches server freeze duration (3-10s) |
| **Color** | Ice blue, cyan, white |

### Status Effect Overlays

For status effects (freeze from Frost Diver, petrify from Stone Curse):

**Approach**: Post-process material on the affected character's mesh:
- **Frozen**: Add a translucent blue-white ice material overlay with angular facets
- **Petrified**: Desaturate the character mesh to gray using a Material Parameter Collection that the subsystem updates

Alternatively, use Niagara particles attached to the character mesh:
- **Frozen**: Ring of ice crystal mesh particles around the character + frost mist
- **Petrified**: Slowly rising stone-colored particles + gray tint (simpler to implement)

---

## C++ Architecture — SkillVFXSubsystem

### New Files to Create

```
client/SabriMMO/Source/SabriMMO/
├── VFX/
│   ├── SkillVFXSubsystem.h
│   ├── SkillVFXSubsystem.cpp
│   ├── SkillVFXData.h          // Data table struct mapping skill ID → VFX config
│   └── CastingCircleActor.h/.cpp  // The ground casting circle
```

### SkillVFXSubsystem Design

```cpp
// SkillVFXSubsystem.h
UCLASS()
class USkillVFXSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // === Socket Event Handlers ===
    void OnCastStart(int32 CasterId, int32 SkillId, float CastTime,
                     int32 TargetId, bool bIsEnemy, FVector TargetLocation);
    void OnCastComplete(int32 CasterId, int32 SkillId);
    void OnCastInterrupted(int32 CasterId, int32 SkillId);
    void OnSkillEffectDamage(int32 AttackerId, int32 TargetId, int32 SkillId,
                              float Damage, bool bIsCritical, FString Element,
                              FVector TargetLocation, int32 HitNumber);
    void OnBuffApplied(int32 TargetId, int32 SkillId, FString BuffName,
                        float Duration);
    void OnBuffRemoved(int32 TargetId, int32 SkillId, FString BuffName);

    // === VFX Spawning ===
    void SpawnCastingCircle(FVector Location, FLinearColor Color, float Radius, float Duration);
    void SpawnSkillImpact(int32 SkillId, FVector Location, FLinearColor Color, float Scale);
    void SpawnProjectile(int32 SkillId, FVector Start, FVector End, float Speed, FLinearColor Color);
    void SpawnPersistentGroundEffect(int32 SkillId, FVector Location, FLinearColor Color);
    void SpawnBuffAura(int32 TargetId, int32 SkillId, FLinearColor Color);
    void RemovePersistentEffect(int32 EffectId);

private:
    // === Niagara System References (loaded from Data Asset or soft references) ===
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_BoltFromSky;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_Projectile;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_AoEImpact;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_GroundPersistent;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_GroundAoERain;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_SelfBuff;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_TargetDebuff;
    UPROPERTY()
    TObjectPtr<UNiagaraSystem> NS_HealFlash;

    // === Casting Circle Material ===
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MI_CastingCircle;

    // === Active Effects Tracking ===
    TMap<int32, TObjectPtr<ACastingCircleActor>> ActiveCastingCircles;  // CasterId → Circle
    TMap<int32, TObjectPtr<UNiagaraComponent>> ActivePersistentEffects; // EffectId → Component
    TMap<int32, TObjectPtr<UNiagaraComponent>> ActiveBuffAuras;         // TargetId_SkillId → Component

    // === Helpers ===
    FLinearColor GetElementColor(const FString& Element) const;
    float GetSkillAoERadius(int32 SkillId) const;
    FString GetSkillElement(int32 SkillId) const;
    AActor* FindActorById(int32 Id, bool bIsEnemy) const;
};
```

### Integration Points

The subsystem hooks into your existing socket event system. In `SabriMMOCharacter.cpp` or wherever you handle incoming socket events, add calls to `SkillVFXSubsystem`:

```cpp
// In your existing skill:cast_start handler:
if (USkillVFXSubsystem* VFX = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
{
    VFX->OnCastStart(CasterId, SkillId, CastTime, TargetId, bIsEnemy, TargetLocation);
}

// In your existing skill:effect_damage handler:
if (USkillVFXSubsystem* VFX = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
{
    VFX->OnSkillEffectDamage(AttackerId, TargetId, SkillId, Damage, bIsCritical,
                              Element, FVector(TargetX, TargetY, TargetZ), HitNumber);
}
```

### Loading Niagara Assets

Use soft object references to avoid hard dependencies:

```cpp
void USkillVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Load Niagara systems via soft references
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BoltVFX(
        TEXT("/Game/SabriMMO/VFX/NS_BoltFromSky.NS_BoltFromSky"));
    if (BoltVFX.Succeeded()) NS_BoltFromSky = BoltVFX.Object;

    // ... or load async via FSoftObjectPath for better load times
}
```

---

## Free Resources Inventory

### Must-Download (Free, Essential)

| Resource | What | URL |
|----------|------|-----|
| **Niagara Examples Pack** | 50+ production Niagara systems | https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600 |
| **Free Niagara Magic Circle** | Ready-made magic circle Niagara effect | https://www.artstation.com/marketplace/p/dBJmq/ |
| **Free Niagara Pack (Fab)** | Community Niagara effects | https://www.fab.com/listings/cc942f1c-1766-446a-bee2-3baabf95a71e |
| **Free Realistic Explosions** | Explosion Niagara systems | https://www.fab.com/listings/a48b3fa2-2ebf-42c2-8892-fa20a1eff289 |

### Free AI Texture Generators

| Tool | URL | Best For |
|------|-----|----------|
| **SEELE AI** | seeles.ai | Animated sprite sheets from text |
| **OpenArt Sprite** | openart.ai/generator/sprite | Single spell effect sprites |
| **CGDream** | cgdream.ai | Sprite sheet generation |
| **DALL-E 3** | ChatGPT Plus | Quick concept/texture generation |

### Free Learning Resources

| Resource | URL | What |
|----------|-----|------|
| **CGHOW Niagara Guide** | cghow.com | Comprehensive beginner Niagara tutorials |
| **CGHOW Spell FX** | cghow.com/spell-fx-in-ue5-niagara-tutorial/ | Spell effect specific tutorials |
| **CGHOW Heal Spell** | cghow.com/heal-spell-in-ue5-niagara-tutorial/ | Healing circle tutorial |
| **CGHOW Magic Ring Material** | cghow.com/magical-ring-material-in-ue5-3-tutorial/ | Procedural magic circle material |
| **Epic Niagara Tutorials** | dev.epicgames.com (Niagara section) | Official documentation |
| **bp-shirai/UE-Learn-Niagara** | github.com | Step-by-step Niagara learning project |

### MCP Servers for Asset Generation

| Server | URL | Capability |
|--------|-----|-----------|
| **ChiR24/Unreal_mcp** | github.com/ChiR24/Unreal_mcp | Niagara system creation via MCP (experimental) |
| **Flux159/mcp-game-asset-gen** | github.com/Flux159/mcp-game-asset-gen | Image/texture generation via DALL-E/Gemini |
| **MubarakHAlketbi/game-asset-mcp** | github.com/MubarakHAlketbi/game-asset-mcp | 2D sprite generation via HuggingFace |

### Optional Paid Tools

| Tool | URL | Cost | Best For |
|------|-----|------|----------|
| **EmberGen** | jangafx.com | $20-40/mo | Flipbook sprite sheets (most RO-authentic) |
| **ZibraVDB** | zibra.ai | Free for indie | Volumetric smoke/fire (advanced) |
| **Houdini Indie** | sidefx.com | $269/yr | Procedural VFX (overkill for this project) |

---

## Sources

### RO Visual Effects Research
- iRO Wiki (all skill pages: Cold Bolt, Fire Bolt, Lightning Bolt, Soul Strike, etc.)
- RateMyServer skill database
- rAthena effect_list.txt (github.com/idathena/trunk/blob/master/doc/effect_list.txt)
- rAthena Magic Target Collection (rathena.org/board/topic/75230)
- Hercules Board — STR/TGA file identification
- The Spriters Resource — RO effect sprites
- Ragnarok Research Lab — file formats, mechanics documentation
- roBrowser (github.com/vthibault/roBrowser) — open-source RO client
- UnityRO (github.com/guilhermelhr/unityro) — Unity RO client with STR rendering
- RagnarokRebuildTcp (github.com/Doddler/RagnarokRebuildTcp) — Unity RO rebuild
- GRF Editor (github.com/Tokeiburu/GRFEditor) — RO asset extraction

### UE5 VFX Technical
- Epic Official Niagara Documentation (dev.epicgames.com)
- Epic Niagara Quick Start Guide
- Epic Niagara Scalability & Best Practices
- Epic Niagara Lightweight Emitters Overview
- Epic Decal Materials Documentation
- Epic Gradient Material Functions
- UNiagaraFunctionLibrary API Reference
- Community Tutorial: Using Niagara in C++ (dev.epicgames.com)
- Community Tutorial: Optimizing Niagara Scalability
- CGHOW complete Niagara beginner guide
- CGHOW Magical Ring Material tutorial
- ArtStation animated materials tutorial
- Epic Forums: AoE target/range indicators
- Epic Forums: AoE spell pre-cast indicator

### AI/Automated VFX
- ChiR24/Unreal_mcp — GitHub (36 MCP tools including Niagara)
- Flux159/mcp-game-asset-gen — GitHub (image/texture generation)
- MubarakHAlketbi/game-asset-mcp — GitHub (sprite generation)
- Mystfit/Unreal-StableDiffusionTools — GitHub (SD in UE5)
- Epic: Creating VFX from Midjourney Textures (dev.epicgames.com tutorial)
- EmberGen by JangaFX (jangafx.com)
- ZibraVDB (zibra.ai) — free for indie
- Houdini Niagara plugin (github.com/sideeffects/HoudiniNiagara)

### Free Resources
- Niagara Examples Pack (fab.com — 50+ free systems)
- ArtStation Free Niagara Magic Circle
- Fab Free Niagara Pack
- Fab Free Realistic Explosions Pack
- NiagaraUIRenderer plugin (github.com/SourySK/NiagaraUIRenderer)
- bp-shirai/UE-Learn-Niagara (GitHub)

### Community/Recreation Projects
- Tree of Savior effect discussion (forum.treeofsavior.com)
- Ragnarok X: Next Generation (Steam)
- Epic Forums: 2D characters in 3D world
- Niagara sprite facing tutorial (cyanhall.com)
- Flipbook animation in UE5 tutorial (foro3d.com)
- Circular ring material tutorial (unrealpossibilities.blogspot.com)

---

**Total Research Sources**: 60+
**Last Updated**: 2026-03-05
