# Sprite UE5 Integration Plan — Approach C (Texture Atlas + Layered Billboard)

## Overview

Integrate 2D sprite art into Sabri_MMO's UE5 client using the **same layered sprite system as Ragnarok Online Classic**. Each character is a billboard actor with stacked transparent planes, one per equipment layer. A material reads from texture atlases and selects the correct frame based on direction, animation state, and time.

---

## Architecture: How RO Classic Does It

```
RO Layer Stack (back to front):
  1. Shadow         — ground shadow circle
  2. Body           — class-specific sprite (Knight body, Mage body, etc.)
  3. Head/Hair      — hair style + hair color (palette-swapped)
  4. Lower Headgear — e.g., Iron Cain
  5. Middle Headgear — e.g., Sunglasses
  6. Upper Headgear — e.g., Helm of Orc Hero
  7. Garment/Robe   — optional cape/robe overlay
  8. Weapon         — drawn at weapon attachment point
  9. Shield         — drawn at shield attachment point
```

Each layer is an independent sprite with its own animation frames and direction data. Layers are composited at render time by drawing them overlapping in screen space. Equipping a new weapon just swaps which sprite file is used for layer 8.

---

## Our Implementation in UE5

### Component Hierarchy

```cpp
ASpriteCharacterActor : AActor
  ├── USceneComponent (Root)
  ├── USpriteLayerComponent "Body"        — class body atlas
  ├── USpriteLayerComponent "Hair"        — hair style atlas + color tint
  ├── USpriteLayerComponent "HeadgearTop" — upper headgear atlas
  ├── USpriteLayerComponent "HeadgearMid" — mid headgear atlas
  ├── USpriteLayerComponent "HeadgearLow" — lower headgear atlas
  ├── USpriteLayerComponent "Garment"     — robe/cape atlas
  ├── USpriteLayerComponent "Weapon"      — weapon sprite atlas
  ├── USpriteLayerComponent "Shield"      — shield sprite atlas
  ├── UDecalComponent "Shadow"            — ground shadow circle
  └── USpriteAnimController                — direction + animation state machine
```

### USpriteLayerComponent (custom UPrimitiveComponent or UStaticMeshComponent)

Each layer is a **flat quad (plane mesh)** with:
- A **Dynamic Material Instance** that reads from a texture atlas
- **UV offset parameters** to select the correct frame: `(DirectionIndex, FrameIndex)`
- **Billboard behavior**: always faces the camera (via `FRotator` update in Tick)
- **Z-offset stacking**: each layer has a tiny Z offset (0.01 units) so they don't Z-fight
- **Visibility toggle**: `SetVisibility(false)` when no equipment in that slot

### Material (M_SpriteAtlas)

```
Parameters:
  - Atlas (Texture2DParameter)         — the sprite atlas texture
  - AtlasGridSize (Vector2)            — e.g., (8, 21) for 8 cols × 21 rows
  - FrameIndex (ScalarParameter)       — which frame to show (0-167)
  - TintColor (LinearColorParameter)   — for hair color / palette tint
  - Opacity (ScalarParameter)          — for fade in/out

Node graph:
  1. Divide TexCoord by AtlasGridSize → gives one cell
  2. Add offset based on FrameIndex:
     - Column = FrameIndex % GridCols
     - Row = floor(FrameIndex / GridCols)
     - UV offset = (Column, Row) / AtlasGridSize
  3. Sample Atlas at computed UV
  4. Multiply by TintColor
  5. Output: BaseColor = RGB, Opacity = Alpha

Material settings:
  - Blend Mode: Masked (Alpha < 0.5 = transparent)
  - Shading Model: Unlit (sprites are pre-lit from Blender)
  - Two-Sided: No (billboard faces camera only)
```

---

## Phase 1: Atlas Packing (Offline Python Script)

### Input
168 individual PNGs per character:
```
idle_S_f00.png ... idle_SE_f03.png     (32 frames: 8 dirs × 4 frames)
walk_S_f00.png ... walk_SE_f07.png     (64 frames: 8 dirs × 8 frames)
attack_S_f00.png ... attack_SE_f05.png (48 frames: 8 dirs × 6 frames)
death_S_f00.png ... death_SE_f02.png   (24 frames: 8 dirs × 3 frames)
                                        Total: 168 frames
```

### Output
1. **One texture atlas** (e.g., 8192×8192 or multiple 4096×4096)
   - Grid layout: 8 columns (one per direction: S, SW, W, NW, N, NE, E, SE)
   - Rows grouped by animation: idle rows, walk rows, attack rows, death rows
2. **JSON metadata file** mapping `(animation, direction, frame)` → atlas index
   ```json
   {
     "atlas_size": [8192, 8192],
     "cell_size": [1024, 1024],
     "grid": [8, 21],
     "animations": {
       "idle":   { "start_row": 0,  "frame_count": 4 },
       "walk":   { "start_row": 4,  "frame_count": 8 },
       "attack": { "start_row": 12, "frame_count": 6 },
       "death":  { "start_row": 18, "frame_count": 3 }
     },
     "directions": ["S","SW","W","NW","N","NE","E","SE"]
   }
   ```

### Script: `pack_atlas.py`
- Uses Pillow to composite PNGs into atlas
- Generates `.png` atlas + `.json` metadata
- Outputs to `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/`
- UE5 imports the atlas as a single Texture2D (enable "No Mipmaps" + "Nearest" filtering for pixel art)

---

## Phase 2: Direction Calculation

RO uses 8 directions based on the angle between the character's **facing direction** and the **camera's forward vector**.

```cpp
// In USpriteAnimController::Tick()
int32 CalculateDirection(const FVector& CharFacing, const FVector& CameraForward)
{
    // Project to 2D (XY plane)
    FVector2D Facing2D(CharFacing.X, CharFacing.Y);
    FVector2D CamFwd2D(CameraForward.X, CameraForward.Y);
    Facing2D.Normalize();
    CamFwd2D.Normalize();

    // Angle between character facing and camera
    float Dot = FVector2D::DotProduct(Facing2D, CamFwd2D);
    float Cross = Facing2D.X * CamFwd2D.Y - Facing2D.Y * CamFwd2D.X;
    float Angle = FMath::Atan2(Cross, Dot); // -PI to PI

    // Quantize to 8 directions (each 45° sector)
    // 0=S (facing camera), 1=SW, 2=W, 3=NW, 4=N (facing away), 5=NE, 6=E, 7=SE
    int32 Dir = FMath::RoundToInt(Angle / (PI / 4.0f)) % 8;
    if (Dir < 0) Dir += 8;
    return Dir;
}
```

In Sabri_MMO's isometric camera (fixed angle), the camera forward is constant. Direction changes only when the character rotates/faces a new direction. The server sends `facing` direction with movement updates.

---

## Phase 3: Animation State Machine

```cpp
UENUM()
enum class ESpriteAnimState : uint8
{
    Idle,     // Standing idle — weapon atlas provides correct visual
    Walk,     // Walking — weapon atlas provides correct visual
    Attack,   // Melee attack — weapon atlas provides correct visual
    Cast,     // Buff/positive skill cast (shared atlas)
    Hit,      // Taking damage (shared atlas)
    Death,    // Dying (shared atlas)
    Sit,      // Sitting on ground (shared atlas)
    Taunt,    // Debuff skill like Provoke (shared atlas)
    Pickup,   // Picking up item (shared atlas)
    Block,    // Shield block (1H weapon atlas only)
    MAX
};

// NOTE: No weapon-specific states (IdleWeapon, WalkWeapon).
// Weapon atlas swap handles visual differences via dual atlas system.
// ESpriteWeaponMode (None/OneHand/TwoHand) selects which weapon atlas is active.

// State transitions (driven by game events):
// Movement input → Walk or WalkWeapon (revert to Idle/IdleWeapon when stopped)
// combat:attack_result → Attack (play once, revert to IdleWeapon if in combat)
// skills:cast_start → Cast or Taunt (play once, revert to Idle/IdleWeapon)
// combat:health_update (HP≤0) → Death (play once, stay)
// combat:hit → Hit (play once, revert to Idle/IdleWeapon)
// player:sit → Sit (until player:stand)
// item:pickup → Pickup (play once, revert to Idle)
// combat:blocked → Block (play once, revert to Idle/IdleWeapon)
//
// Weapon-dependent states select from variant animation folders:
//   No weapon: Breathing Idle / Idle / Punching / Walking
//   1H weapon: Fighting Idle / Stable Sword Slash / Run With Sword
//   2H weapon: 2hand Idle / Great Sword Slash
// Multiple variants per state → random selection each loop cycle
```

### Frame Timing

```cpp
void USpriteAnimController::Tick(float DeltaTime)
{
    FrameTimer += DeltaTime;

    float FrameDuration;
    switch (CurrentState)
    {
        case Idle:        FrameDuration = 0.25f; break;  // 4 FPS (slow breathe)
        case IdleWeapon:  FrameDuration = 0.25f; break;  // 4 FPS (combat ready)
        case Walk:        FrameDuration = 0.10f; break;  // 10 FPS (quick step)
        case WalkWeapon:  FrameDuration = 0.10f; break;  // 10 FPS
        case Attack:      FrameDuration = 0.08f; break;  // 12 FPS (fast swing)
        case Cast:        FrameDuration = 0.15f; break;  // ~7 FPS
        case Taunt:       FrameDuration = 0.15f; break;  // ~7 FPS
        case Death:       FrameDuration = 0.20f; break;  // 5 FPS (slow fall)
        case Hit:         FrameDuration = 0.10f; break;  // 10 FPS (quick react)
        case Pickup:      FrameDuration = 0.12f; break;  // ~8 FPS
        case Block:       FrameDuration = 0.10f; break;  // 10 FPS
        case Sit:         FrameDuration = 0.50f; break;  // 2 FPS (barely moving)
        default:          FrameDuration = 0.20f;
    }

    if (FrameTimer >= FrameDuration)
    {
        FrameTimer -= FrameDuration;
        CurrentFrame++;

        int32 MaxFrames = GetFrameCount(CurrentState);
        if (CurrentFrame >= MaxFrames)
        {
            if (CurrentState == Walk || CurrentState == WalkWeapon ||
                CurrentState == Idle || CurrentState == IdleWeapon || CurrentState == Sit)
            {
                CurrentFrame = 0; // Loop
                // For states with variants, randomly pick a new variant each cycle
                if (HasVariants(CurrentState))
                    SelectRandomVariant(CurrentState);
            }
            else
            {
                CurrentFrame = MaxFrames - 1; // Hold last frame
                if (CurrentState == Attack || CurrentState == Cast ||
                    CurrentState == Hit || CurrentState == Taunt ||
                    CurrentState == Pickup || CurrentState == Block)
                    SetState(bInCombat ? ESpriteAnimState::IdleWeapon : ESpriteAnimState::Idle);
            }
        }

        UpdateAllLayers();
    }
}
```

---

## Phase 4: Equipment Layer System

### How equipment changes work

When the server sends `player:stats` or `inventory:equip` with equipment data:

```cpp
void ASpriteCharacterActor::OnEquipmentChanged(const FEquipmentData& Equip)
{
    // Body: always visible, determined by class
    BodyLayer->SetAtlas(GetClassBodyAtlas(Equip.ClassId));

    // Hair: style atlas + color tint
    HairLayer->SetAtlas(GetHairAtlas(Equip.HairStyle));
    HairLayer->SetTintColor(GetHairColor(Equip.HairColor));

    // Weapon: atlas per weapon visual type
    if (Equip.WeaponId > 0)
    {
        WeaponLayer->SetAtlas(GetWeaponAtlas(Equip.WeaponVisualType));
        WeaponLayer->SetVisibility(true);
    }
    else
        WeaponLayer->SetVisibility(false);

    // Shield
    if (Equip.ShieldId > 0)
    {
        ShieldLayer->SetAtlas(GetShieldAtlas(Equip.ShieldVisualType));
        ShieldLayer->SetVisibility(true);
    }
    else
        ShieldLayer->SetVisibility(false);

    // Headgear (3 slots)
    UpdateHeadgearLayer(HeadgearTopLayer, Equip.HeadgearTopId);
    UpdateHeadgearLayer(HeadgearMidLayer, Equip.HeadgearMidId);
    UpdateHeadgearLayer(HeadgearLowLayer, Equip.HeadgearLowId);

    // Garment
    if (Equip.GarmentId > 0)
    {
        GarmentLayer->SetAtlas(GetGarmentAtlas(Equip.GarmentVisualType));
        GarmentLayer->SetVisibility(true);
    }
    else
        GarmentLayer->SetVisibility(false);
}
```

### Atlas registry

A data table or C++ map that maps `(item_visual_type)` → `UTexture2D* Atlas`:

```cpp
// Loaded at startup from a data table or config
TMap<int32, UTexture2D*> WeaponAtlases;    // visualType → atlas
TMap<int32, UTexture2D*> ShieldAtlases;
TMap<int32, UTexture2D*> HeadgearAtlases;
TMap<int32, UTexture2D*> GarmentAtlases;
TMap<int32, UTexture2D*> HairAtlases;      // hairStyle → atlas
TMap<int32, UTexture2D*> BodyAtlases;      // classId → atlas
```

---

## Phase 5: Hair Color System

RO uses **palette swapping** — the hair sprite is grayscale, and a 256-color palette LUT determines the final color.

**Our approach**: Render hair as a separate layer with a **grayscale** or **neutral tone** atlas. Apply color via material parameter:

```
Material M_SpriteAtlas_Tinted:
  BaseColor = AtlasSample.RGB * TintColor.RGB
  Opacity = AtlasSample.A
```

Hair colors can be defined as FLinearColor presets:
```cpp
const FLinearColor HairColors[] = {
    FLinearColor(0.15f, 0.08f, 0.05f), // 0: Black
    FLinearColor(0.55f, 0.25f, 0.10f), // 1: Brown
    FLinearColor(0.85f, 0.65f, 0.30f), // 2: Blonde
    FLinearColor(0.75f, 0.15f, 0.10f), // 3: Red
    FLinearColor(0.40f, 0.40f, 0.45f), // 4: Gray
    FLinearColor(0.90f, 0.90f, 0.90f), // 5: White
    FLinearColor(0.15f, 0.20f, 0.60f), // 6: Blue
    FLinearColor(0.50f, 0.15f, 0.50f), // 7: Purple
    // ... RO has 8+ hair colors
};
```

For more authentic palette swapping, use a **1D LUT texture** per hair color and sample it in the material using the grayscale value as the U coordinate.

---

## Phase 6: Rendering Pipeline Integration

### Billboard Behavior (VALIDATED 2026-03-23 — took 8 attempts)

```cpp
void ASpriteCharacterActor::UpdateBillboard()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager) return;

    // Screen-aligned billboard — ALL sprites face the same direction
    // Uses reversed camera forward so sprite is perpendicular to view
    // CRITICAL: use (-CamFwd).Rotation(), NOT CamRot directly
    //   - CamRot.Pitch is NEGATIVE (looking down) → sprite lies flat (WRONG)
    //   - (-CamFwd).Rotation().Pitch is POSITIVE (facing up) → sprite faces camera (CORRECT)
    //   - ToCamera.Rotation() varies per screen position → edge-of-screen tilt (WRONG)
    FVector CamFwd = PC->PlayerCameraManager->GetCameraRotation().Vector();
    FRotator FaceCamera = (-CamFwd).Rotation();
    SetActorRotation(FRotator(FaceCamera.Pitch, FaceCamera.Yaw, 0.f));
}
```

### Z-Ordering for Layers

Layers are stacked with tiny offsets along the camera's forward vector so they render in the correct order:

```cpp
// In UpdateLayerPositions() — called when camera changes
FVector CamForward = Camera->GetForwardVector();
float LayerSpacing = 0.5f; // 0.5 units between layers

BodyLayer->SetRelativeLocation(CamForward * 0.0f);
HairLayer->SetRelativeLocation(CamForward * -LayerSpacing * 1);
HeadgearTopLayer->SetRelativeLocation(CamForward * -LayerSpacing * 2);
WeaponLayer->SetRelativeLocation(CamForward * -LayerSpacing * 3);
// ... etc (negative = toward camera = drawn on top)
```

### Attack Animation Sync

When the server sends `combat:attack_result`:
1. AnimController switches to `Attack` state
2. Attack animation plays through (6 frames at ~12 FPS = 0.5s)
3. Damage number pops from DamageNumberSubsystem (already implemented)
4. Reverts to Idle

For skills with cast bars:
1. `skills:cast_start` → switch to `Cast` state + start cast bar
2. Cast animation loops until `skills:cast_complete`
3. Skill VFX plays from SkillVFXSubsystem (already implemented)
4. Reverts to Idle

---

## Phase 7: Integration with Existing Sabri_MMO Systems

### Replacing 3D Characters

Currently `SabriMMOCharacter` (local player) and `BP_OtherPlayerCharacter` (remote players) are 3D skeletal mesh actors. The sprite system would either:

**Option A: Replace entirely** — Remove 3D models, use sprite actors for all characters
- Pros: Authentic RO feel, consistent art style
- Cons: Lose 3D camera orbit, need to re-render for every new equipment combo

**Option B: Hybrid** — Use 3D models for the local player (close camera), sprites for other players and enemies
- Pros: Best of both worlds, 3D detail up close, sprites for performance at distance
- Cons: Two rendering systems to maintain

**Option C: Sprite-only (recommended for RO Classic feel)**
- This is how RO does it — ALL entities are sprites
- The game is isometric with fixed camera angle = sprites are the natural choice
- Equipment layering is trivial (just swap atlas textures)
- Performance is excellent (hundreds of players = hundreds of textured quads)

### Systems that need hooks

| Existing System | Integration Point |
|----------------|-------------------|
| `SabriMMOCharacter` | Replace with `ASpriteCharacterActor` or add sprite rendering mode |
| `OtherPlayerSubsystem` | Spawn `ASpriteCharacterActor` instead of `BP_OtherPlayerCharacter` |
| `EnemySubsystem` | Spawn sprite actors for enemies (need enemy atlases) |
| `EquipmentSubsystem` | Call `OnEquipmentChanged()` on sprite actor |
| `CombatActionSubsystem` | Trigger attack/hit/death animations |
| `BuffBarSubsystem` | Visual buff effects (tint, particle overlay) |
| `SkillVFXSubsystem` | Attach VFX to sprite actor location |
| `DamageNumberSubsystem` | Pop numbers above sprite actor |
| `NameTagSubsystem` | Position name tag above sprite |
| `PlayerInputSubsystem` | Click-to-move still works (just targeting the actor) |
| `CameraSubsystem` | Fixed isometric angle (already correct for sprites) |

---

## Phase 8: Sprite Production Pipeline

For each character class (20 classes × 2 genders ≈ 35 variants):

### Body sprites
1. Generate T-pose reference image (ComfyUI or manually)
2. Upload to Tripo3D → download GLB
3. Convert to FBX → upload to Mixamo → download 4 animation FBXs (idle/walk/attack/death)
4. Render via `blender_sprite_render_v2.py` with `--texture-from`
5. Post-process → pack atlas

### Hair sprites (per hair style)
1. Render the SAME model but with hair isolated (separate mesh or masking)
2. Render as grayscale or neutral tone
3. Hair color applied via material tint at runtime

### Weapon sprites (per weapon visual type)
1. 3D model of weapon → render from 8 directions with attack animation
2. OR: render weapon as part of the attack animation and composite
3. RO approach: weapon is a separate sprite layer with its own animation sync

### Headgear sprites (per headgear item)
1. 3D model of headgear → render from 8 directions
2. Static (no animation) — just 8 direction frames
3. Positioned relative to head anchor point

### Equipment sprite count estimate
- 20 body atlases (one per class) × 2 genders = ~35 atlases
- 10 hair styles × grayscale = 10 atlases (color via tint)
- 30 weapon types = 30 atlases
- 50 headgears = 50 atlases (8 frames each, small)
- 10 shields = 10 atlases
- 10 garments = 10 atlases
- **Total: ~145 atlases** (each 8192×8192 or smaller)

---

## Implementation Order

### Phase 1 — Atlas packing + Material (1 session)
- [ ] Write `pack_atlas.py` script
- [ ] Pack mage_f sprites into test atlas
- [ ] Create `M_SpriteAtlas` material in UE5
- [ ] Create test actor with single plane showing atlas frames
- [ ] Verify direction switching works

### Phase 2 — Sprite Actor + Animation (1 session)
- [ ] Create `ASpriteCharacterActor` C++ class
- [ ] Create `USpriteLayerComponent` for individual layers
- [ ] Implement `USpriteAnimController` state machine
- [ ] Billboard behavior
- [ ] Test with mage_f body only

### Phase 3 — Equipment Layers (1 session)
- [ ] Add all 9 layer slots
- [ ] Equipment change handler
- [ ] Z-ordering
- [ ] Test equipping/unequipping

### Phase 4 — System Integration (1-2 sessions)
- [ ] Hook into OtherPlayerSubsystem (spawn sprites for remote players)
- [ ] Hook into EnemySubsystem (spawn sprites for enemies)
- [ ] Hook into CombatActionSubsystem (attack/death animations)
- [ ] Hook into EquipmentSubsystem (equipment visual changes)
- [ ] Hook into NameTagSubsystem + DamageNumberSubsystem

### Phase 5 — Hair Color + Polish (1 session)
- [ ] Hair layer with tint material
- [ ] Hair color selection in character creation
- [ ] Garment/cape rendering
- [ ] Shadow decal

### Phase 6 — Sprite Production Scaling (ongoing)
- [ ] Render all 20 class bodies
- [ ] Render weapon sprites
- [ ] Render headgear sprites
- [ ] Render monster sprites

---

## File Structure

```
client/SabriMMO/Source/SabriMMO/
  Sprite/
    SpriteCharacterActor.h/.cpp         — Main sprite character
    SpriteLayerComponent.h/.cpp         — Single atlas layer (plane + material)
    SpriteAnimController.h/.cpp         — Animation state machine
    SpriteAtlasData.h                   — Atlas metadata structs
    SpriteEquipmentManager.h/.cpp       — Maps equipment IDs to atlas textures

client/SabriMMO/Content/SabriMMO/
  Sprites/
    Atlases/
      Body/
        mage_f_body.png                 — 8192×8192 atlas
        mage_f_body.json                — metadata
        knight_m_body.png
        ...
      Hair/
        hair_style_01.png               — grayscale hair atlas
        ...
      Weapons/
        sword_01.png
        staff_01.png
        ...
      Headgear/
        helm_01.png
        ...
    Materials/
      M_SpriteAtlas.uasset             — base sprite material
      M_SpriteAtlas_Tinted.uasset      — tintable variant (hair)
      MI_MageF_Body.uasset             — material instance (mage body)
      ...

2D animations/
  pack_atlas.py                         — Atlas packing script
```
