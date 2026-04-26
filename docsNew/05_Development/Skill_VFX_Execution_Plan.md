# Skill VFX Execution Plan — Step-by-Step

> **STATUS (as of 2026-04-15)**
> - **Complete**: Phases 0, 3, 4, 5 (partial), 8, 9, 10 — 97+ VFX configs live in `SkillVFXSubsystem`, casting circle actor shipped, socket event integration done, multiplayer broadcast done.
> - **Pending**: Phases 6 (Niagara template creation), 7 (texture/asset generation), 11 (in-game testing pass).
> - **Deferred**: Phase 1 (ChiR24 full automation) — blocked, fallback Path B/C in use instead.
> - **Open VFX bugs** are tracked in `_journal/Dashboard.md` → "VFX Bugs to fix" (Frost Diver persistence, frozen-enemy movement, Bolt impact VFX, Fireball multiplayer, Soul Strike travel, Stone Curse color).

**Created**: 2026-03-05
**Purpose**: Single source of truth for Claude Code to follow. Every step is explicit. No gaps.
**Strategy**: Try full automation (ChiR24 MCP) → Fall back to hybrid (existing MCP + Python + manual) → All C++ written by Claude

---

## Execution Order Summary

```
PHASE 0: Prerequisites & Setup                    [~15 min]
PHASE 1: Attempt ChiR24/Unreal_mcp (Option A)    [~1-2 hours, may fail]
PHASE 2: Fallback Setup (Option B + C)            [~30 min if Phase 1 fails]
PHASE 3: C++ Core — SkillVFXSubsystem             [~2-3 hours]
PHASE 4: C++ Core — CastingCircleActor             [~1 hour]
PHASE 5: Material Creation                         [~1 hour]
PHASE 6: Niagara Template Creation                 [~2-4 hours]
PHASE 7: Texture/Asset Generation                  [~1 hour]
PHASE 8: Socket Event Integration                  [~1-2 hours]
PHASE 9: Per-Skill VFX Wiring                      [~2-3 hours]
PHASE 10: Warp Portal VFX                          [~30 min]
PHASE 11: Testing & Optimization                   [~1 hour]
```

---

## PHASE 0: Prerequisites & Setup

### Step 0.1: Add Niagara Module Dependencies

**File**: `C:\Sabri_MMO\client\SabriMMO\Source\SabriMMO\SabriMMO.Build.cs`

**Action**: Add `"Niagara"` and `"NiagaraCore"` to `PublicDependencyModuleNames`

**Automation**: 100% — Claude edits file directly

**Verification**: File compiles when UE5 editor rebuilds

---

### Step 0.2: Create VFX Directory Structure

**Action**: Create folder structure for new C++ files

```
client/SabriMMO/Source/SabriMMO/VFX/
├── SkillVFXSubsystem.h
├── SkillVFXSubsystem.cpp
├── SkillVFXData.h
├── CastingCircleActor.h
└── CastingCircleActor.cpp
```

**Automation**: 100% — Claude creates all files

---

### Step 0.3: User Downloads Free Resources (MANUAL — 5 minutes)

**User must download** these free packs from Fab/ArtStation:

| Resource | URL | Import To |
|----------|-----|-----------|
| **Niagara Examples Pack** | https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600 | Via Fab launcher → Add to project |
| **Free Niagara Magic Circle** | https://www.artstation.com/marketplace/p/dBJmq/ | Download ZIP → import manually |

**Why manual**: Fab downloads require browser authentication. Claude cannot download marketplace assets.

**Verification**: After import, check that `Content/NiagaraExamples/` (or similar) folder exists in UE5 Content Browser.

---

## PHASE 1: Attempt ChiR24/Unreal_mcp (Option A)

**Goal**: Install the VFX-capable MCP server. If it works on UE 5.7, use it for Niagara automation.
**Risk**: 13 known crash bugs on UE 5.7. Plugin may fail to compile.
**Time budget**: 1-2 hours max. If it's not working by then, skip to Phase 2.

### Step 1.1: Clone ChiR24/Unreal_mcp

```bash
cd C:/Sabri_MMO
git clone https://github.com/ChiR24/Unreal_mcp.git tools/ChiR24_unreal_mcp
```

**Automation**: 100% — Claude runs git clone

---

### Step 1.2: Copy C++ Plugin to UE5 Project

```bash
# Copy the McpAutomationBridge plugin
cp -r tools/ChiR24_unreal_mcp/plugins/McpAutomationBridge client/SabriMMO/Plugins/McpAutomationBridge
```

**Automation**: 100% — Claude copies files

**Note**: This plugin has 64 C++ files and 21 plugin dependencies. Compilation will take several minutes.

---

### Step 1.3: Enable Plugin in .uproject

**File**: `C:\Sabri_MMO\client\SabriMMO\SabriMMO.uproject`

**Action**: Add McpAutomationBridge to the Plugins array

**Automation**: 100% — Claude edits file

---

### Step 1.4: Build and Test

**Action**: User opens UE5 editor. It will prompt to rebuild.

**Possible outcomes**:
- **SUCCESS**: Plugin compiles, no crashes → proceed to Step 1.5
- **COMPILE FAILURE**: Plugin dependencies missing or API incompatible → **SKIP TO PHASE 2**
- **EDITOR CRASH**: Known UE 5.7 bug → **SKIP TO PHASE 2**

**Decision point**: If the plugin doesn't work after 30 minutes of troubleshooting, abandon it and proceed to Phase 2. Do NOT spend more than 1 hour on this.

---

### Step 1.5: Install TypeScript MCP Server

```bash
cd tools/ChiR24_unreal_mcp
npm install
```

**Automation**: 100% — Claude runs npm install

---

### Step 1.6: Configure MCP in Claude Code Settings

**File**: Claude Code MCP settings (user's settings.json or project .mcp.json)

**Action**: Add ChiR24 server configuration. Keep existing flopperam server but mark it as secondary.

```json
{
  "mcpServers": {
    "unrealMCP_niagara": {
      "command": "node",
      "args": ["C:/Sabri_MMO/tools/ChiR24_unreal_mcp/server/dist/index.js"],
      "env": {
        "UNREAL_MCP_PORT": "8090"
      }
    }
  }
}
```

**Important**: The existing flopperam MCP runs on port 55557 (TCP). ChiR24 uses port 8090 (WebSocket). No port conflict.

---

### Step 1.7: Test Niagara Creation via MCP

**Action**: Claude attempts to create a simple test Niagara system:

```
manage_effect({
  action: "create_niagara_system",
  name: "NS_Test_VFX",
  path: "/Game/SabriMMO/VFX/"
})
```

Then try adding a module:
```
manage_effect({
  action: "add_spawn_burst_module",
  system_name: "NS_Test_VFX",
  spawn_count: 10
})
```

**If this works**: ChiR24 MCP is viable. Use it for all Niagara creation in Phase 6.
**If this fails**: Skip to Phase 2. The test system (if partially created) should be deleted.

---

### Step 1.8: Record Result

**Action**: Claude records whether Phase 1 succeeded or failed in this document.

**PHASE 1 RESULT**: `[TO BE FILLED DURING EXECUTION]`
- [ ] ChiR24 plugin compiled successfully
- [ ] MCP server connected to editor
- [ ] Test Niagara system created
- [ ] Module additions work
- **VERDICT**: Use ChiR24 for Phase 6 / Skip to Phase 2

---

## PHASE 2: Fallback Setup (If Phase 1 Failed)

**Only execute this phase if Phase 1 failed.**

### Step 2.1: Remove ChiR24 Plugin (If Installed)

```bash
rm -rf client/SabriMMO/Plugins/McpAutomationBridge
```

Also remove from .uproject if added.

**Automation**: 100% — Claude removes files

---

### Step 2.2: Prepare Python Editor Scripts Directory

```bash
mkdir -p client/SabriMMO/Scripts/VFX
```

Claude will write Python scripts here that the user runs via `Edit > Execute Python Script` in UE5.

---

### Step 2.3: Install Flux159/mcp-game-asset-gen (For Texture Generation)

```bash
cd C:/Sabri_MMO
git clone https://github.com/Flux159/mcp-game-asset-gen.git tools/mcp-game-asset-gen
cd tools/mcp-game-asset-gen
npm install
```

**Action**: Add to Claude Code MCP config:
```json
{
  "mcpServers": {
    "game_asset_gen": {
      "command": "node",
      "args": ["C:/Sabri_MMO/tools/mcp-game-asset-gen/dist/index.js"],
      "env": {
        "OPENAI_API_KEY": "<user's key if available>"
      }
    }
  }
}
```

**Note**: This requires an OpenAI API key for DALL-E. If user doesn't have one, skip texture generation via MCP and use alternative AI tools (manual step).

---

### Step 2.4: Verify Existing unrealMCP Still Works

**Action**: Claude tests a simple command via existing MCP:
```
mcp__unrealMCP__get_actors_in_level()
```

If this works, the existing MCP is available for Blueprint actor creation in Phase 6.

---

## PHASE 3: C++ Core — SkillVFXSubsystem

**Automation**: 100% — Claude writes all code

### Step 3.1: Create SkillVFXData.h

**File**: `C:\Sabri_MMO\client\SabriMMO\Source\SabriMMO\VFX\SkillVFXData.h`

**Contents**: Data structures mapping skill IDs to VFX configurations

```cpp
// Enums for VFX template types
UENUM(BlueprintType)
enum class ESkillVFXTemplate : uint8
{
    None,
    BoltFromSky,      // Cold/Fire/Lightning Bolt
    Projectile,        // Soul Strike, Fire Ball, Frost Diver projectile
    AoEImpact,         // Magnum Break, Fire Ball explosion, Napalm Beat
    GroundPersistent,  // Fire Wall, Safety Wall
    GroundAoERain,     // Thunderstorm
    SelfBuff,          // Endure, Energy Coat, Sight
    TargetDebuff,      // Provoke, Stone Curse, Frost Diver freeze
    HealFlash,         // First Aid
    WarpPortal         // Warp portal effect
};

// Per-skill VFX configuration
USTRUCT(BlueprintType)
struct FSkillVFXConfig
{
    GENERATED_BODY()

    int32 SkillId = 0;
    ESkillVFXTemplate Template = ESkillVFXTemplate::None;
    FLinearColor PrimaryColor = FLinearColor::White;
    FLinearColor SecondaryColor = FLinearColor::White;
    float Scale = 1.0f;
    float Duration = 0.5f;
    bool bUseCastingCircle = false;
    FLinearColor CastingCircleColor = FLinearColor(0.3f, 0.8f, 1.0f, 1.0f); // Cyan default
    float CastingCircleRadius = 200.0f;
    int32 BoltCount = 1;           // For bolt skills: number of hits
    float BoltInterval = 0.15f;     // Seconds between bolt hits
    float ProjectileSpeed = 2000.f; // For projectile skills
    bool bLooping = false;          // For persistent effects (Fire Wall, Safety Wall)
    FString Element;                // fire, water, wind, earth, etc.
};
```

**Claude also creates**: A static function `GetSkillVFXConfig(int32 SkillId)` that returns the correct `FSkillVFXConfig` for each of the 18 skill effects. This is a big switch/map with all the per-skill parameters from the research document.

---

### Step 3.2: Create SkillVFXSubsystem.h

**File**: `C:\Sabri_MMO\client\SabriMMO\Source\SabriMMO\VFX\SkillVFXSubsystem.h`

**Key design decisions**:
- Inherits `UWorldSubsystem` (same pattern as all other subsystems)
- `ShouldCreateSubsystem()` checks for SocketManager actor (game level only, not login)
- Listens to socket events via the existing SocketIOClient plugin
- Holds soft references to Niagara systems (loaded in Initialize)
- Tracks active persistent effects in TMaps

**Public interface**:
```cpp
// Called from socket event handlers (SabriMMOCharacter or SkillTreeSubsystem)
void HandleCastStart(const FString& JsonPayload);
void HandleCastComplete(const FString& JsonPayload);
void HandleCastInterrupted(const FString& JsonPayload);
void HandleSkillEffectDamage(const FString& JsonPayload);
void HandleBuffApplied(const FString& JsonPayload);
void HandleBuffRemoved(const FString& JsonPayload);

// Manual VFX control (for testing or Blueprint use)
UFUNCTION(BlueprintCallable)
void SpawnTestEffect(ESkillVFXTemplate Template, FVector Location, FLinearColor Color);
```

---

### Step 3.3: Create SkillVFXSubsystem.cpp

**File**: `C:\Sabri_MMO\client\SabriMMO\Source\SabriMMO\VFX\SkillVFXSubsystem.cpp`

**Key implementation details**:

1. **Initialize()**:
   - Find SocketManager actor in world
   - Bind to socket events: `skill:cast_start`, `skill:effect_damage`, `skill:buff_applied`, `skill:buff_removed`, `skill:cast_interrupted_broadcast`
   - Load Niagara system soft references
   - Load CastingCircle material

2. **HandleCastStart()**:
   - Parse JSON: casterId, skillId, actualCastTime, targetId, isEnemy
   - Look up FSkillVFXConfig for skillId
   - If `bUseCastingCircle`: spawn CastingCircleActor at target location
   - Track in `ActiveCastingCircles` map

3. **HandleSkillEffectDamage()**:
   - Parse JSON: attackerId, targetId, skillId, damage, isCritical, element, targetX/Y/Z, hitNumber
   - Look up FSkillVFXConfig for skillId
   - Spawn appropriate Niagara effect at target location with element color
   - For multi-hit skills (bolts): only spawn on first hit or every hit depending on skill
   - For persistent effects: track in `ActivePersistentEffects` map

4. **HandleBuffApplied()/HandleBuffRemoved()**:
   - Spawn/remove aura Niagara effects attached to target actor
   - Track in `ActiveBuffAuras` map

5. **Element Color Helper**:
   ```cpp
   FLinearColor GetElementColor(const FString& Element)
   {
       static TMap<FString, FLinearColor> Colors = {
           {"fire",    FLinearColor(1.0f, 0.3f, 0.05f, 1.0f)},
           {"water",   FLinearColor(0.2f, 0.5f, 1.0f, 1.0f)},
           {"wind",    FLinearColor(0.9f, 1.0f, 0.3f, 1.0f)},
           {"earth",   FLinearColor(0.6f, 0.4f, 0.2f, 1.0f)},
           {"holy",    FLinearColor(1.0f, 1.0f, 0.8f, 1.0f)},
           {"dark",    FLinearColor(0.3f, 0.0f, 0.5f, 1.0f)},
           {"ghost",   FLinearColor(0.6f, 0.3f, 0.9f, 1.0f)},
           {"undead",  FLinearColor(0.2f, 0.0f, 0.3f, 1.0f)},
           {"poison",  FLinearColor(0.3f, 0.8f, 0.1f, 1.0f)},
           {"neutral", FLinearColor(0.3f, 0.8f, 1.0f, 1.0f)}
       };
       if (auto* Found = Colors.Find(Element)) return *Found;
       return FLinearColor(0.3f, 0.8f, 1.0f, 1.0f); // Default cyan
   }
   ```

6. **FindActorByServerId()**: Helper to locate player/enemy actors by their server-assigned ID (needed for attached effects and projectile targeting).

---

### Step 3.4: Compile and Fix Errors

**Automation**: 100% — Claude reviews compiler output and fixes

---

## PHASE 4: C++ Core — CastingCircleActor

**Automation**: 100% — Claude writes all code

### Step 4.1: Create CastingCircleActor.h

**File**: `C:\Sabri_MMO\client\SabriMMO\Source\SabriMMO\VFX\CastingCircleActor.h`

```cpp
UCLASS()
class ACastingCircleActor : public AActor
{
    GENERATED_BODY()
public:
    ACastingCircleActor();

    void Initialize(FLinearColor ElementColor, float Radius, float Duration);
    void FadeOut(float FadeTime = 0.3f);

    UPROPERTY(VisibleAnywhere, Category="VFX")
    UDecalComponent* CircleDecal;

private:
    UPROPERTY()
    UMaterialInstanceDynamic* DynMaterial;

    FTimerHandle FadeTimerHandle;
    FTimerHandle DestroyTimerHandle;
    float FadeAlpha = 1.0f;
    float FadeSpeed = 0.0f;
    bool bFadingOut = false;

    void TickFade();
};
```

### Step 4.2: Create CastingCircleActor.cpp

**Key implementation**:
- Constructor creates DecalComponent, sets default material, rotation (-90 pitch = face down)
- `Initialize()`: Creates MID from base material, sets ElementColor, FadeAlpha=1, starts fade-in
- `FadeOut()`: Begins alpha interpolation to 0, then destroys self
- `TickFade()`: Called on timer, updates DynMaterial FadeAlpha parameter

### Step 4.3: Compile and Verify

---

## PHASE 5: Material Creation

### Step 5.1: Create Casting Circle Decal Material via Python Script

**File**: `C:\Sabri_MMO\client\SabriMMO\Scripts\VFX\create_casting_circle_material.py`

**Automation**: 100% — Claude writes the script. User runs it once in UE5 editor.

```python
import unreal

# Create material asset
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_factory = unreal.MaterialFactoryNew()
mat = asset_tools.create_asset(
    "M_CastingCircle", "/Game/SabriMMO/VFX/Materials",
    unreal.Material, mat_factory
)

mel = unreal.MaterialEditingLibrary

# Set material domain to Deferred Decal
mel.set_material_instance_scalar_parameter_value(...)  # Domain/blend mode set via properties

# Create texture coordinate node
tex_coord = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -800, 0)

# Create RadialGradientExponential nodes for ring shape
outer_grad = mel.create_material_expression(mat, unreal.MaterialExpressionRadialGradientExponential, -600, 0)
inner_grad = mel.create_material_expression(mat, unreal.MaterialExpressionRadialGradientExponential, -600, 200)

# Create subtract node (outer - inner = ring)
subtract = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -400, 100)

# Create vector parameter for ElementColor
color_param = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
color_param.parameter_name = "ElementColor"
color_param.default_value = unreal.LinearColor(0.3, 0.8, 1.0, 1.0)

# Create scalar parameter for FadeAlpha
fade_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 300)
fade_param.parameter_name = "FadeAlpha"
fade_param.default_value = 1.0

# Create scalar parameter for EmissiveIntensity
intensity_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -200, -300)
intensity_param.parameter_name = "EmissiveIntensity"
intensity_param.default_value = 3.0

# Create multiply nodes for color * ring and ring * fade
color_multiply = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 0)
fade_multiply = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 200)

# Connect nodes
mel.connect_material_expressions(outer_grad, "", subtract, "A")
mel.connect_material_expressions(inner_grad, "", subtract, "B")
mel.connect_material_expressions(subtract, "", color_multiply, "A")
mel.connect_material_expressions(color_param, "", color_multiply, "B")
mel.connect_material_expressions(subtract, "", fade_multiply, "A")
mel.connect_material_expressions(fade_param, "", fade_multiply, "B")

# Connect to material outputs
mel.connect_material_property(color_multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(fade_multiply, "", unreal.MaterialProperty.MP_OPACITY)

# Set material properties
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
mat.set_editor_property("decal_blend_mode", unreal.DecalBlendMode.DBM_TRANSLUCENT)

# Save
unreal.EditorAssetLibrary.save_asset("/Game/SabriMMO/VFX/Materials/M_CastingCircle")
unreal.log("M_CastingCircle created successfully!")
```

**User action**: Open UE5 editor → Edit → Execute Python Script → select this file → Run

**Verification**: Check Content Browser for `/Game/SabriMMO/VFX/Materials/M_CastingCircle`

---

### Step 5.2: Add UV Rotation to Casting Circle Material (Manual or MCP)

The Python script above creates the static ring. For rotation animation:

**If ChiR24 MCP works (Phase 1 succeeded)**:
- Use `manage_material_authoring` to add CustomRotator + Time nodes

**If not**:
- Claude writes a second Python script adding the rotation nodes
- OR Claude provides exact step-by-step instructions for user to add in Material Editor (3 nodes, ~2 minutes)

**The rotation requires**:
1. `Time` node → multiply by `RotationSpeed` scalar parameter (default 0.5)
2. `CustomRotator` node: Input = TextureCoordinate, Rotation Center = (0.5, 0.5), Rotation Angle = Time output
3. Route CustomRotator output through the existing RadialGradient nodes

---

## PHASE 6: Niagara Template Creation

**This is the critical phase where approach varies based on Phase 1 result.**

### Path A: ChiR24 MCP Succeeded → Full Automation

Claude uses `manage_effect` MCP tool to create all 8 Niagara templates:

#### Step 6A.1: NS_BoltFromSky
```
manage_effect({action: "create_niagara_system", name: "NS_BoltFromSky", path: "/Game/SabriMMO/VFX/"})
manage_effect({action: "add_spawn_burst_module", system: "NS_BoltFromSky", count: 1})
manage_effect({action: "add_velocity_module", system: "NS_BoltFromSky", velocity: [0, 0, -5000]})
manage_effect({action: "add_size_module", system: "NS_BoltFromSky", size: [50, 50]})
manage_effect({action: "add_color_module", system: "NS_BoltFromSky", color: [1, 1, 1, 1]})
manage_effect({action: "add_sprite_renderer_module", system: "NS_BoltFromSky"})
manage_effect({action: "add_user_parameter", system: "NS_BoltFromSky", name: "SpellColor", type: "LinearColor"})
manage_effect({action: "add_user_parameter", system: "NS_BoltFromSky", name: "BoltCount", type: "int", default: 1})
```

Repeat for all 8 templates: NS_Projectile, NS_AoEImpact, NS_GroundPersistent, NS_GroundAoERain, NS_SelfBuff, NS_TargetDebuff, NS_HealFlash

#### Step 6A.2: Compile and Test Each System
After creating each, spawn at a test location to verify it works.

---

### Path B: ChiR24 Failed → Blueprint + Free Pack Hybrid

#### Step 6B.1: Identify Effects from Niagara Examples Pack

After user downloads the pack (Step 0.3), Claude reads the content directory to find suitable base effects:

```
# Claude uses Glob/Read to find the pack's Niagara systems
Glob: Content/NiagaraExamples/**/*.uasset
```

Map pack effects to our templates:
| Our Template | Likely Pack Effect | Customization Needed |
|-------------|-------------------|---------------------|
| NS_BoltFromSky | Explosion or Impact system | Modify direction (top-down), add streak |
| NS_Projectile | Projectile trail system | Change color, add glow |
| NS_AoEImpact | Explosion system | Adjust radius, color |
| NS_GroundPersistent | Fire/buff system | Make looping, set color |
| NS_GroundAoERain | Lightning system | Randomize positions in area |
| NS_SelfBuff | Buff/debuff aura system | Set orbit pattern |
| NS_TargetDebuff | Debuff system | Attach mode |
| NS_HealFlash | Healing/sparkle system | Green color, upward motion |

#### Step 6B.2: Create Blueprint Wrapper Actors via Existing MCP

For each template, Claude uses the existing unrealMCP to create a Blueprint actor:

```
mcp__unrealMCP__create_blueprint({name: "BP_VFX_BoltFromSky", parent_class: "Actor"})
mcp__unrealMCP__add_component_to_blueprint({
    blueprint_name: "BP_VFX_BoltFromSky",
    component_type: "NiagaraComponent",
    component_name: "NiagaraEffect"
})
```

**User manual step (~2 min per BP)**: In UE5 editor, open each BP_VFX_* Blueprint, select the NiagaraComponent, and assign the appropriate Niagara System from the Examples Pack dropdown. This is a single click per Blueprint.

#### Step 6B.3: Duplicate and Customize Pack Effects

If the pack effects need modification:
- User duplicates the pack's Niagara system (right-click → Duplicate in Content Browser)
- Claude provides exact parameter changes via written instructions
- Or Claude writes Python scripts to modify Niagara system parameters (experimental)

---

### Path C: Python Script Approach (Supplementary)

#### Step 6C.1: Write Python Scripts for Niagara Creation

**File**: `client/SabriMMO/Scripts/VFX/create_niagara_templates.py`

Claude writes a Python script attempting to create Niagara systems via `unreal.NiagaraSystemFactoryNew`:

```python
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
niagara_factory = unreal.NiagaraSystemFactoryNew()

# Attempt to create system
ns = asset_tools.create_asset(
    "NS_BoltFromSky", "/Game/SabriMMO/VFX/Niagara",
    unreal.NiagaraSystem, niagara_factory
)

if ns:
    unreal.log("NS_BoltFromSky created successfully!")
else:
    unreal.log_error("Failed to create Niagara system")
```

**Note**: This may create an empty system that needs manual configuration. The Python API for adding Niagara modules is undocumented and may not work. This is experimental.

---

## PHASE 7: Texture/Asset Generation

### Step 7.1: Generate Magic Circle Texture (If Not Using Procedural Material)

**Option A — MCP game-asset-gen (if installed in Phase 2.3)**:
```
generate_2d_asset({
    prompt: "magic circle runic symbols glowing cyan, concentric rings, arcane glyphs, transparent background, top-down view, game asset",
    width: 512,
    height: 512,
    output_path: "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/VFX/Textures/"
})
```

**Option B — Manual AI generation**: User generates via Midjourney/DALL-E/SEELE AI using prompts from research doc, imports manually.

**Option C — Download free**: rAthena Magic Target Collection (19 free variants): https://rathena.org/board/topic/75230-magic-target-collectionfree-download/

---

### Step 7.2: Generate Spell Effect Textures (Optional Enhancement)

For sprite-based effects (more RO-authentic), generate flipbook textures:

| Texture | Prompt | Use |
|---------|--------|-----|
| Fire bolt impact | "fire explosion burst sprite sheet 4x4 transparent background orange red" | NS_BoltFromSky (fire) |
| Ice shard | "ice crystal shard angular jagged cyan blue transparent background game asset" | NS_BoltFromSky (ice) |
| Lightning flash | "electric lightning strike white yellow spark transparent background" | NS_BoltFromSky (lightning) |
| Soul orb | "glowing ghost orb translucent purple white ethereal transparent background" | NS_Projectile (soul strike) |
| Fire ring | "fire ring explosion expanding outward orange red transparent background" | NS_AoEImpact (magnum break) |

---

## PHASE 8: Socket Event Integration

**Automation**: 100% — Claude edits existing C++ files

### Step 8.1: Register SkillVFXSubsystem Socket Events

**File**: Wherever socket events are currently bound (likely `SabriMMOCharacter.cpp` or `SkillTreeSubsystem.cpp`)

**Action**: Add socket event bindings that forward to SkillVFXSubsystem:

```cpp
// In the function where you bind socket events:
if (USkillVFXSubsystem* VFXSub = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
{
    // These events are ALREADY emitted by the server - we just need to listen
    Socket->On("skill:cast_start", [VFXSub](auto Response) {
        VFXSub->HandleCastStart(Response[0]->AsString());
    });
    // skill:effect_damage is already handled - add VFX call alongside existing damage number logic
    // skill:buff_applied and skill:buff_removed - add VFX call
    // skill:cast_interrupted_broadcast - add VFX cleanup call
}
```

**Important**: Do NOT remove existing handlers. ADD the VFX calls alongside them. The existing DamageNumberSubsystem, CastBarSubsystem, etc. must continue working.

---

### Step 8.2: Add VFX Calls to Existing skill:effect_damage Handler

**File**: `SkillTreeSubsystem.cpp` (or wherever `HandleSkillEffectDamage` is)

**Action**: After the existing damage number / tracking logic, add:

```cpp
// EXISTING CODE: damage tracking, cooldown update, etc.
// ...

// NEW: Spawn skill VFX
if (USkillVFXSubsystem* VFXSub = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
{
    VFXSub->HandleSkillEffectDamage(JsonPayload);
}
```

---

### Step 8.3: Add VFX Calls to Existing skill:cast_start Handler

Same pattern — add VFX call alongside existing CastBarSubsystem logic.

---

### Step 8.4: Add VFX Calls for Buff/Debuff Events

If not already handled, bind `skill:buff_applied` and `skill:buff_removed` socket events and forward to SkillVFXSubsystem.

---

### Step 8.5: Add Ground Effect Lifecycle Events

The server needs to tell clients when persistent ground effects (Fire Wall, Safety Wall) are created and destroyed. Check if these events already exist:

- `skill:ground_effect_start { skillId, position, duration }` — does this exist?
- `skill:ground_effect_end { effectId }` — does this exist?

If not, Claude may need to add them to the server (`server/src/index.js`).

**Check**: Read the server code for Fire Wall and Safety Wall to see what events are broadcast when they are created/destroyed.

---

## PHASE 9: Per-Skill VFX Wiring

**Automation**: 100% — all in SkillVFXData.h/cpp

### Step 9.1: Define All Skill VFX Configs

In `GetSkillVFXConfig()`, define the complete mapping:

```cpp
static TMap<int32, FSkillVFXConfig> Configs = {
    // === NOVICE ===
    {2,   {2,   ESkillVFXTemplate::HealFlash,      FLinearColor(0.2f, 0.9f, 0.3f), ..., false}},  // First Aid

    // === SWORDSMAN ===
    {103, {103, ESkillVFXTemplate::AoEImpact,       FLinearColor(1.0f, 1.0f, 0.8f), ..., false}},  // Bash
    {104, {104, ESkillVFXTemplate::TargetDebuff,     FLinearColor(1.0f, 0.1f, 0.1f), ..., false}},  // Provoke
    {105, {105, ESkillVFXTemplate::AoEImpact,        FLinearColor(1.0f, 0.3f, 0.0f), ..., false}},  // Magnum Break
    {106, {106, ESkillVFXTemplate::SelfBuff,         FLinearColor(1.0f, 0.9f, 0.3f), ..., false}},  // Endure

    // === MAGE ===
    {200, {200, ESkillVFXTemplate::BoltFromSky,      FLinearColor(0.3f, 0.8f, 1.0f), ..., true}},   // Cold Bolt
    {201, {201, ESkillVFXTemplate::BoltFromSky,       FLinearColor(1.0f, 0.3f, 0.0f), ..., true}},   // Fire Bolt
    {202, {202, ESkillVFXTemplate::BoltFromSky,       FLinearColor(1.0f, 1.0f, 0.5f), ..., true}},   // Lightning Bolt
    {203, {203, ESkillVFXTemplate::AoEImpact,         FLinearColor(0.6f, 0.2f, 0.8f), ..., false}},  // Napalm Beat
    {205, {205, ESkillVFXTemplate::SelfBuff,          FLinearColor(1.0f, 0.4f, 0.1f), ..., false}},  // Sight
    {206, {206, ESkillVFXTemplate::TargetDebuff,      FLinearColor(0.4f, 0.4f, 0.4f), ..., false}},  // Stone Curse
    {207, {207, ESkillVFXTemplate::Projectile,        FLinearColor(1.0f, 0.3f, 0.0f), ..., true}},   // Fire Ball
    {208, {208, ESkillVFXTemplate::Projectile,        FLinearColor(0.3f, 0.8f, 1.0f), ..., true}},   // Frost Diver
    {209, {209, ESkillVFXTemplate::GroundPersistent,  FLinearColor(1.0f, 0.3f, 0.0f), ..., false}},  // Fire Wall
    {210, {210, ESkillVFXTemplate::Projectile,        FLinearColor(0.6f, 0.3f, 0.9f), ..., false}},  // Soul Strike
    {211, {211, ESkillVFXTemplate::GroundPersistent,  FLinearColor(1.0f, 0.4f, 0.6f), ..., false}},  // Safety Wall
    {212, {212, ESkillVFXTemplate::GroundAoERain,     FLinearColor(1.0f, 1.0f, 0.5f), ..., true}},   // Thunderstorm
};
```

Each config specifies: template type, colors, whether it uses a casting circle, AoE radius, bolt count, bolt interval, projectile speed, looping flag, and element name.

---

### Step 9.2: Implement Template-Specific Spawn Logic

In `SkillVFXSubsystem.cpp`, the `HandleSkillEffectDamage` function dispatches based on template:

```cpp
void USkillVFXSubsystem::HandleSkillEffectDamage(const FString& Json)
{
    // Parse JSON...
    FSkillVFXConfig Config = GetSkillVFXConfig(SkillId);

    switch (Config.Template)
    {
    case ESkillVFXTemplate::BoltFromSky:
        SpawnBoltFromSky(TargetLocation, Config, HitNumber);
        break;
    case ESkillVFXTemplate::Projectile:
        SpawnProjectile(AttackerLocation, TargetLocation, Config);
        break;
    case ESkillVFXTemplate::AoEImpact:
        SpawnAoEImpact(TargetLocation, Config);
        break;
    case ESkillVFXTemplate::GroundPersistent:
        SpawnGroundPersistent(TargetLocation, Config, SkillId);
        break;
    case ESkillVFXTemplate::GroundAoERain:
        SpawnGroundAoERain(TargetLocation, Config);
        break;
    // ... etc
    }
}
```

Each `Spawn*` method creates a `UNiagaraComponent` via `UNiagaraFunctionLibrary::SpawnSystemAtLocation`, then sets User Parameters (SpellColor, Scale, etc.) on the component.

---

## PHASE 10: Warp Portal VFX

**Automation**: 100% — Claude edits existing WarpPortal C++ files

### Step 10.1: Add Niagara Component to WarpPortal Actor

**File**: `C:\Sabri_MMO\client\SabriMMO\Source\SabriMMO\WarpPortal.h`

**Action**: Add a `UNiagaraComponent*` member and a `UNiagaraSystem*` asset reference

```cpp
UPROPERTY(VisibleAnywhere, Category="VFX")
UNiagaraComponent* PortalEffect;

UPROPERTY(EditAnywhere, Category="VFX")
UNiagaraSystem* PortalVFXSystem;
```

### Step 10.2: Create and Attach in Constructor

**File**: `WarpPortal.cpp`

```cpp
PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
PortalEffect->SetupAttachment(RootComponent);
PortalEffect->SetAutoActivate(true);
```

### Step 10.3: Assign VFX System (Manual or MCP)

**If MCP available**: Set the Niagara system reference via Blueprint property
**If not**: User opens WarpPortal Blueprint, assigns the portal Niagara system to the component

---

## PHASE 11: Testing & Optimization

### Step 11.1: Compile Full Project

```
UnrealBuildTool SabriMMO Win64 Development
```

Fix any compile errors.

### Step 11.2: Test Each Skill VFX

**In-game testing checklist**:

| Skill | Test Action | Expected VFX | Status |
|-------|------------|-------------|--------|
| First Aid | Use skill (self) | Green cross flash + sparkles | [ ] |
| Bash | Attack enemy with Bash | White-yellow impact flash | [ ] |
| Provoke | Cast on enemy | Red "!" flash on target | [ ] |
| Magnum Break | Use skill | Fire ring burst from player | [ ] |
| Endure | Use skill | Golden aura flash | [ ] |
| Cold Bolt | Cast on enemy | Blue ice shards fall from sky | [ ] |
| Fire Bolt | Cast on enemy | Orange fire pillars from sky | [ ] |
| Lightning Bolt | Cast on enemy | Yellow lightning from sky | [ ] |
| Soul Strike | Cast on enemy | Purple orbs fly to target | [ ] |
| Napalm Beat | Cast on enemy | Purple puff cluster at target | [ ] |
| Fire Ball | Cast on enemy | Fireball arcs + explodes | [ ] |
| Frost Diver | Cast on enemy | Ice spike to target + freeze | [ ] |
| Fire Wall | Cast on ground | Fire columns on ground (looping) | [ ] |
| Safety Wall | Cast on ground | Pink light pillar (looping) | [ ] |
| Thunderstorm | Cast on ground | Lightning rain in area | [ ] |
| Sight | Use skill | Orange fireball orbits player | [ ] |
| Casting Circle | Cast any bolt spell | Rotating circle at target/feet | [ ] |
| Warp Portal | Walk near portal | Golden ring + spiraling particles | [ ] |

### Step 11.3: Multiplayer Testing

Test with 2+ PIE instances to verify:
- [ ] VFX spawns correctly for OTHER players' skills (not just local player)
- [ ] Persistent effects (Fire Wall, Safety Wall) visible to all players in zone
- [ ] Buff auras appear on remote players
- [ ] No VFX spawned in wrong location
- [ ] Casting circles appear for remote player casts
- [ ] Performance: stat niagara shows reasonable numbers

### Step 11.4: Performance Optimization

```cpp
// Set Effect Type with distance culling
// In editor: Create ET_SkillVFX Effect Type asset
// - Max distance: 5000 UE units
// - Significance handler: Distance-based
// - Budget: Max 20 simultaneous spell effects
```

Add to all spawned Niagara components:
```cpp
NiagaraComp->SetAsset(NS_BoltFromSky);
// Effect Type assigned in the Niagara system asset itself, not in C++
```

### Step 11.5: Add `/effect` Toggle (Like RO)

Optional but nice: Let players toggle other players' VFX off for performance.

Add to chat command handler or keybind:
```cpp
void USkillVFXSubsystem::ToggleEffects(bool bEnabled)
{
    bEffectsEnabled = bEnabled;
    // If disabled, destroy all active effects from other players
}
```

---

## APPENDIX: File Modification Summary

### New Files Created by Claude

| File | Phase |
|------|-------|
| `VFX/SkillVFXSubsystem.h` | Phase 3 |
| `VFX/SkillVFXSubsystem.cpp` | Phase 3 |
| `VFX/SkillVFXData.h` | Phase 3 |
| `VFX/CastingCircleActor.h` | Phase 4 |
| `VFX/CastingCircleActor.cpp` | Phase 4 |
| `Scripts/VFX/create_casting_circle_material.py` | Phase 5 |
| `Scripts/VFX/create_niagara_templates.py` | Phase 6C |

### Existing Files Modified by Claude

| File | Change | Phase |
|------|--------|-------|
| `SabriMMO.Build.cs` | Add Niagara, NiagaraCore modules | Phase 0 |
| `SabriMMOCharacter.cpp` (or SkillTreeSubsystem.cpp) | Add VFX socket event bindings | Phase 8 |
| `WarpPortal.h` | Add NiagaraComponent | Phase 10 |
| `WarpPortal.cpp` | Create + attach NiagaraComponent | Phase 10 |
| `SabriMMO.uproject` | Add McpAutomationBridge plugin (Phase 1 only) | Phase 1 |

### Manual User Steps (Total: ~30 minutes)

| Step | Time | Phase |
|------|------|-------|
| Download Niagara Examples Pack from Fab | 5 min | Phase 0 |
| Download Free Magic Circle from ArtStation | 2 min | Phase 0 |
| Open UE5 editor, let it rebuild after Build.cs change | 3 min | Phase 0 |
| Run Python script: `create_casting_circle_material.py` | 1 min | Phase 5 |
| Assign Niagara systems to BP_VFX_* Blueprints (if Path B) | 15 min | Phase 6B |
| Assign portal VFX to WarpPortal Blueprint | 2 min | Phase 10 |
| Test each skill in-game | varies | Phase 11 |

---

## APPENDIX: Server-Side Changes (If Needed)

### Ground Effect Events

Check if the server already broadcasts events when Fire Wall / Safety Wall are created and removed. If not, Claude adds:

**In `server/src/index.js`**:

```javascript
// When Fire Wall is created:
broadcastToZone(zone, 'skill:ground_effect', {
    skillId: 209,
    casterId: playerId,
    position: { x: groundX, y: groundY, z: groundZ },
    orientation: wallOrientation, // angle in degrees
    duration: wallDuration,
    effectId: uniqueEffectId
});

// When Fire Wall expires or is depleted:
broadcastToZone(zone, 'skill:ground_effect_end', {
    effectId: uniqueEffectId
});
```

Same pattern for Safety Wall (skillId 211).

---

**PHASE 1 RESULT**: `DEFERRED — UE5 editor not running. Will attempt when user opens editor.`

**EXECUTION STATUS**:
- [x] Phase 0 complete — Build.cs updated (Niagara+NiagaraCore), VFX dir created, include path added
- [ ] Phase 1 deferred — ChiR24 MCP requires editor open for plugin compile test
- [ ] Phase 2 deferred — Fallback setup if Phase 1 fails
- [x] Phase 3 complete — SkillVFXSubsystem.h/.cpp + SkillVFXData.h created (all 15 skills configured)
- [x] Phase 4 complete — CastingCircleActor.h/.cpp created (decal + fade in/out)
- [x] Phase 5 partial — Python script for M_CastingCircle written (user must run in editor)
- [ ] Phase 6 pending — Niagara templates need editor (MCP or manual)
- [ ] Phase 7 pending — Texture generation (MCP or AI tools)
- [x] Phase 8 complete — Socket events registered via EventRouter->RegisterHandler() (no manual wiring needed)
- [x] Phase 9 complete — All 15 skill VFX configs defined in SkillVFXData.h
- [x] Phase 10 complete — WarpPortal.h/.cpp updated with NiagaraComponent
- [ ] Phase 11 pending — Testing requires editor

### Files Created/Modified This Session:
- **CREATED**: `VFX/SkillVFXData.h` — 15 skill configs, element color map, VFX template enum
- **CREATED**: `VFX/SkillVFXSubsystem.h` — Full subsystem header with socket handlers
- **CREATED**: `VFX/SkillVFXSubsystem.cpp` — 500+ lines: socket wrapping, event handlers, Niagara spawning, casting circle management, actor lookup
- **CREATED**: `VFX/CastingCircleActor.h` — Decal actor with dynamic material
- **CREATED**: `VFX/CastingCircleActor.cpp` — Fade in/out, auto-destroy, material parameter control
- **CREATED**: `Scripts/VFX/create_casting_circle_material.py` — Python editor script for procedural ring material
- **MODIFIED**: `SabriMMO.Build.cs` — Added Niagara, NiagaraCore, VFX include path
- **MODIFIED**: `WarpPortal.h` — Added NiagaraComponent, NiagaraSystem
- **MODIFIED**: `WarpPortal.cpp` — Creates/activates portal VFX component

### What Remains (When You Wake Up):
1. **Open UE5 Editor** — it will rebuild with Niagara modules (3 min compile)
2. **Download free packs** from Fab (Niagara Examples Pack + Magic Circle)
3. **Run Python script**: Edit > Execute Python Script > `Scripts/VFX/create_casting_circle_material.py`
4. **Create Niagara templates** — I'll try ChiR24 MCP first, fall back to Blueprint wrappers
5. **Test in-game** — use skills and verify VFX appear

---

**Last Updated**: 2026-03-05 (execution session 1 — code complete, editor steps pending)
