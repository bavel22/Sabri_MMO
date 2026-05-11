# Mage Essentials — UE5.7 Integration Guide

A drop-in spell pack for a Ragnarok Online style game where characters are 2D
sprites and the world is 3D. Includes Fire Bolt, Cold Bolt, Lightning Bolt,
and Fire Wall — each with cast-glow, projectile/pillar, and impact/burst
sprite sheets, plus level-1-to-10 scaling tables and a complete C++ skill
runtime.

---

## 1. What's in the pack

```
mage_essentials/
├── SpriteSheets/                 11 transparent PNG sheets, 5x5 grid, 256x256 per frame
│   ├── vfx_fire_cast_glow_spritesheet.png
│   ├── vfx_fire_bolt_proj_spritesheet.png
│   ├── vfx_fire_impact_spritesheet.png
│   ├── vfx_cold_cast_glow_spritesheet.png
│   ├── vfx_cold_bolt_proj_spritesheet.png
│   ├── vfx_cold_impact_spritesheet.png
│   ├── vfx_lightning_cast_glow_spritesheet.png
│   ├── vfx_lightning_bolt_proj_spritesheet.png
│   ├── vfx_lightning_impact_spritesheet.png
│   ├── vfx_fire_wall_burst_spritesheet.png
│   └── vfx_fire_wall_pillar_spritesheet.png
├── PreviewFrames/                Single-frame source PNGs (debug / reference only)
├── Data/
│   ├── MageSkills.csv            Imports as DataTable<FMageSkillRow>
│   └── MageSkills.json           Same data + scaling rules + element table
├── Source/MageSkills/            Drop-in C++ runtime (UE5.7)
│   ├── MageSkillTypes.h          Row struct, enums
│   ├── MageSkillComponent.h/.cpp Cast pipeline + bolt sequencer + GCD
│   ├── MageBoltProjectile.h/.cpp Generic homing bolt actor
│   ├── MageFireWallActor.h/.cpp  Per-cell wall actor with hit counter + lifetime
│   ├── MageSkills.Build.cs       Module dependencies (Paper2D, Niagara, GameplayTags)
│   └── MageSkillsModule.cpp      Optional: only if you ship as its own module
└── Niagara_Recipes/
    └── Niagara_FireBolt_Recipe.md  Material + system recipe (applies to all 11 sheets)
```

---

## 2. Ragnarok Online skill model implemented

Classic pre-Renewal RO behavior, as encoded in `Data/MageSkills.json`:

| Skill | Levels | Per-level effect |
|---|---|---|
| Fire Bolt    | 1-10 | **N bolts at level N**. Cast time +0.2s per level. Element Fire. |
| Cold Bolt    | 1-10 | **N bolts at level N**. Same scaling. Element Water. |
| Lightning Bolt | 1-10 | **N bolts at level N**. Same scaling. Element Wind. |
| Fire Wall    | 1-10 | 3-cell wall. Hits-per-cell = 2 + Level (3..12). Damage% scales 50%->100%. |

Each bolt rolls its own damage and applies elemental modifier vs the target's
element (table is in `_meta.element_modifiers`). The system fires bolts at
`bolt_interval = 0.5s` so high-level Fire Bolt 10 visibly hammers the target
with 10 staggered impacts — the classic RO feel.

The cast pipeline is:

```
BeginCastBolt -> [CastTime ticking, cast glow visible]
              -> FinishCast() -> sequence N bolts at BoltInterval
                              -> each: spawn projectile -> impact -> apply damage
              -> after bolts done: OnSkillCastFinish + GCD
```

Fire Wall is ground-targeted and spawns one `AMageFireWallActor` per cell
(width 3 by default), perpendicular to the caster's facing direction.

---

## 3. Importing into UE5.7

### 3a. Copy assets in
1. In your project's `Content/`, create folder `MageEssentials/`.
2. Copy `SpriteSheets/` and `Data/` into `Content/MageEssentials/`.
3. In the Content Browser, drag the 11 PNGs from `Content/MageEssentials/SpriteSheets`
   to import as Texture2D. Apply these settings on each:
   - **Compression Settings:** `UserInterface2D (RGBA)`
   - **Mip Gen Settings:** `NoMipmaps`
   - **sRGB:** `true`
   - **Filter:** `Bilinear`
   - **Texture Group:** `Effects`

### 3b. Create Paper2D Flipbooks (one per sheet)
For each sprite sheet:
1. Right-click the texture -> **Apply Paper2D Texture Settings** (sets the right
   compression + filter for sprites in one click).
2. Right-click the texture -> **Sprite Actions -> Extract Sprites**. The grid
   is 5x5; extracted sprites will be named `_Sprite_0` ... `_Sprite_24`.
3. Select all 25 sprites -> right-click -> **Create Flipbook**.
4. Open the flipbook and set:
   - **Frames per Second:** 24 (loops) / 30 (impacts/burst).
   - For loops: leave Looping = true.
   - For impacts/burst: set Looping = false in your spawning logic, or use
     `UPaperFlipbookComponent::SetLooping(false)`.

Naming convention recommended:
```
FB_FireCastGlow, FB_FireBoltProj, FB_FireImpact,
FB_ColdCastGlow, FB_ColdBoltProj, FB_ColdImpact,
FB_LightningCastGlow, FB_LightningBoltProj, FB_LightningImpact,
FB_FireWallBurst, FB_FireWallPillar
```

### 3c. Import the DataTable
1. Drag `MageSkills.csv` into the Content Browser.
2. **Row Type:** select `MageSkillRow` (this requires the C++ from step 4 to
   be compiled first — bring CSV in *after* the C++ build).
3. The CSV uses RowName = `<SkillId>_<Level>` (e.g. `FIRE_BOLT_03`). The
   component looks up rows via that exact key.

### 3d. Bring in the C++ runtime
**Easiest path (drop into your existing game module):**
1. Copy `Source/MageSkills/*.h` -> `Source/<YourGame>/Public/Skills/`
2. Copy `Source/MageSkills/*.cpp` -> `Source/<YourGame>/Private/Skills/`
3. In `<YourGame>.Build.cs`, add `"Paper2D"`, `"Niagara"`, `"GameplayTags"` to
   `PublicDependencyModuleNames`.
4. Delete `MageSkillsModule.cpp` and `MageSkills.Build.cs` (only used for
   standalone-module setup).
5. Regenerate project files and rebuild.

**Standalone module path:**
1. Copy the whole `Source/MageSkills/` folder into your project's `Source/`.
2. Add `"MageSkills"` to your project `.uproject` `Modules` array and to your
   primary game module's `PrivateDependencyModuleNames`.
3. Regenerate + rebuild.

### 3e. Wire up the DataTable
After the build, re-import `MageSkills.csv`:
- Choose Row Type: `MageSkillRow`.
- Open the resulting DataTable and assign each row's flipbook fields:
  - `CastGlowFlipbook`     -> `FB_*CastGlow`
  - `ProjectileFlipbook`   -> `FB_*BoltProj` (bolts) or leave empty for Fire Wall
  - `ImpactFlipbook`       -> `FB_*Impact`   (bolts) or `FB_FireWallPillar` for wall pillar
- Save.

---

## 4. Hooking it to a player

### 4a. Add the component
1. Open your player Pawn/Character Blueprint.
2. **Add Component -> Mage Skill Component.**
3. In Details: assign `Skill Table` to the imported DataTable.
4. Implement the BlueprintImplementableEvents on the BP:

| Event | What to do |
|---|---|
| `BP Show Cast Glow` | Spawn `FB_*CastGlow` (or Niagara) attached to the caster's hand socket. Save the spawned ref so `BP Hide Cast Glow` can destroy it. |
| `BP Hide Cast Glow` | Destroy the saved ref. |
| `BP Spawn Bolt Projectile` | `Spawn Actor From Class` -> `BP_MageBoltProjectile` (a Blueprint child of `AMageBoltProjectile`). Call `Init Toward Target` with the row + target. The projectile spawns at MuzzleLocation. |
| `BP Spawn Impact` | `Spawn Emitter at Location` (Niagara) OR `Spawn Actor` with `BP_FlipbookImpact` whose flipbook = `Row.ImpactFlipbook`. |
| `BP Spawn Fire Wall Cell` | `Spawn Actor From Class` -> `BP_MageFireWallActor` at CellLocation. Call `Init Cell` with the row. |
| `BP Apply Bolt Damage` | Plug into your damage pipeline (Gameplay Ability System, custom DamageEvent, etc). Compute final damage as `MATK * Row.DamageMultiplier * Element_Modifier`. |
| `BP Apply Fire Wall Tick` | Same idea — invoked from the wall actor's `RegisterHit` -> `BP_OnHitVictim`. |

### 4b. Input bindings (Enhanced Input)
Suggested mapping:
- `IA_FireBolt_LV5`    -> calls `BeginCastBolt(FName("FIRE_BOLT"), 5, GetTarget())`
- `IA_ColdBolt_LV3`    -> `BeginCastBolt("COLD_BOLT", 3, GetTarget())`
- `IA_LightningBolt_LV1` -> `BeginCastBolt("LIGHTNING_BOLT", 1, GetTarget())`
- `IA_FireWall_LV5`    -> `BeginCastFireWall(5, AimedGroundLocation, GetActorForwardVector())`

Where `AimedGroundLocation` is a line trace from the camera through the cursor
hitting the navmesh / ground (standard top-down aim).

### 4c. Sprite-character compatibility
Your characters are Paper2D / billboard sprites. The flipbooks in this pack
also use Paper2D (`UPaperFlipbookComponent`) so they share the same renderer
and sort cleanly with characters. If you've moved characters to Niagara
billboards, follow `Niagara_Recipes/Niagara_FireBolt_Recipe.md` and use the
Niagara variants in the row instead — the C++ doesn't care which is set.

---

## 5. Scaling beyond the basics

The C++ system already supports:

- **More projectiles per skill level** (the BoltCount field) — just author more
  rows in the DataTable. The scheduler will fire them at `BoltInterval`.
- **Cast time scaling** — `CastTime` per row.
- **SP cost scaling** — `SP` per row.
- **Range gating** — read `Range` in your input layer to disallow casts on
  out-of-range targets.
- **Element multipliers** — pass `Element` from the row + target's element
  into `BP_ApplyBoltDamage` and consult `_meta.element_modifiers`.
- **Fire Wall scaling** — `WidthCells`, `HitsPerCell`, `LifetimeSeconds`, and
  `DamageMultiplier` per row.

Want **Soul Strike** (single soul that hits 1 + ceil(level/2) times)? Add a
new SkillId and treat `BoltCount` as hit count, BoltInterval as time-between-hits.
Same code path.

Want **Meteor Storm** (rain of meteors over an area)? Add a Ground category
skill and override the cell-spawn logic in `FinishCast`'s ground branch. The
runtime is intentionally small so this stays trivial.

---

## 6. Damage formula reference (RO classic)

```
BoltDamage = (MATK_min..MATK_max)        // re-rolled per bolt
           * Row.DamageMultiplier
           * ElementalModifier(Row.Element, Target.Element)
           * (1 - Target.MDef_pct/100)
           - Target.MDef_flat
```

`MATK_min..MATK_max` is your stat system's magic attack roll (INT-based in
classic RO). All four bolts skills use 1.0 multiplier; Fire Wall uses
0.5..1.0 by level.

---

## 7. Performance notes

- **Paper2D Flipbooks** — perfect for ~50 simultaneous bolts on screen. Each
  bolt = 1 actor + 1 flipbook component. Cheap.
- **Niagara SubUV** — switch to this for Fire Bolt 10 + multiple casters
  (i.e. 30+ bolts in flight). One emitter draws all bolts in one batch.
- The Fire Wall actor uses `OverlapAllDynamic` with a 0.4s tick interval, not
  per-frame — safe for dozens of cells in one fight.

---

## 8. Quick smoke test

1. Drop a `BP_MageCharacter` (your Pawn with `MageSkillComponent`) into the level.
2. Drop any actor as a target dummy.
3. Bind a key to call `BeginCastBolt("FIRE_BOLT", 10, DummyActor)`.
4. Press it: 3.3s cast -> 10 fire bolts hammer the dummy at 0.5s intervals
   with the cast-glow attached to your hand socket the entire time.
5. Bind another key to `BeginCastFireWall(10, AimLocation, GetActorForwardVector())`.
   3 fiery pillars erupt; each kills enemies after 12 hits or 5s.

If those work, you can author the rest of your spellbook by appending rows to
the DataTable.
