# Prompt B — Tripo3D → Mixamo → Blender Full Animation Pipeline

Copy this entire prompt into a new Claude Code session:

---

## Task

I need you to build a complete **3D-to-2D sprite rendering pipeline** for Sabri_MMO (a Ragnarok Online-inspired MMO). This pipeline takes AI-generated concept art → converts to 3D models → rigs and animates → renders to 2D pixel art sprite sheets from 8 directions.

The previous session proved that AI image-by-image sprite generation has fatal consistency issues. The 3D pipeline is the solution because the same 3D model renders identically in every frame.

**Your job: Build the complete Tripo3D → Mixamo → Blender → sprite sheet pipeline and produce production-ready sprites for all 20 RO character classes.**

## Context

Read these files first:
- `C:/Sabri_MMO/2D animations/SESSION_CONTEXT.md` — Full project status
- `C:/Sabri_MMO/2D animations/blender_sprite_render.py` — Existing (basic) Blender render script
- `C:/Sabri_MMO/2D animations/2D_Sprite_Art_Pipeline_Plan.md` — Research document
- `C:/Sabri_MMO/2D animations/Detailed_Implementation_Guide.md` — Implementation details

## What's Already Installed
- **Blender 5.1** at `C:/Blender 5.1/blender.exe`
- **ComfyUI** at `C:/ComfyUI/` (Illustrious XL + ROSPRITE LoRA for concept art)
- **Python 3.12** in `C:/ComfyUI/venv/` with Pillow, rembg
- **RTX 5090** (32GB VRAM)
- **33 hero reference images** at `C:/Sabri_MMO/2D animations/sprites/references/hero_refs/`

## The Full Pipeline I Need Built

### Phase 1: 3D Model Acquisition

For each of the 20 character classes (both genders):

1. **Hero ref** already exists at `sprites/references/hero_refs/[class]_[m|f].png`
2. User uploads hero ref to **Tripo3D** (tripo3d.ai, free 300 credits/mo) → downloads GLB
3. GLB saved to `C:/Sabri_MMO/2D animations/3d_models/characters/[class]_[m|f].glb`

Help me automate or streamline this. The Tripo3D API endpoint is at `https://api.tripo3d.ai/v2/openapi/task` — if I provide an API key, can you script the upload/download? Otherwise, document the manual steps clearly.

### Phase 2: Rigging + Animation via Mixamo

For each GLB model:

1. Upload to **Mixamo** (mixamo.com, free Adobe account)
2. Mixamo auto-rigs in ~2 minutes
3. Download these animations (FBX format, "In Place" checked):
   - **Idle**: "Breathing Idle" or "Happy Idle" (looping)
   - **Walk**: "Walking" (looping)
   - **Run**: "Running" (looping)
   - **Attack**: "Sword And Shield Slash" or "1H Melee Attack" or class-appropriate
   - **Cast**: "Standing React Large From Front" or "Standing 2H Cast Spell 01" (for magic classes)
   - **Death**: "Dying" (not looping)
   - **Sit**: "Sitting Idle" (looping)
4. FBX saved to `C:/Sabri_MMO/2D animations/3d_models/animations/[class]_[m|f]/[animation_name].fbx`

If Mixamo has an API or CLI tool, help me automate this. Otherwise, document the manual workflow clearly.

### Phase 3: Blender Rendering

Build a comprehensive Blender Python script that:

1. **Imports** the rigged FBX model
2. **Applies cel-shading** material:
   - Shader to RGB → ColorRamp (3 steps, CONSTANT interpolation)
   - Color ramp should MULTIPLY with base texture colors (preserve original palette)
   - Solidify modifier with flipped normals for 1-2px black outline
3. **Sets up isometric camera**:
   - Orthographic projection
   - 30° from horizontal (RO-style angle)
   - Auto-frames character to fill 80% of render area
4. **Renders 8 directions**:
   - Camera orbits around model at 45° increments
   - Direction order: S(0°), SW(45°), W(90°), NW(135°), N(180°), NE(225°), E(270°), SE(315°)
5. **For each animation**:
   - Plays the FBX animation
   - Samples N evenly-spaced frames (idle=4, walk=8, attack=6, death=3)
   - Renders each frame at 512x512 with transparent background
6. **Post-processes**:
   - Runs rembg for any remaining background artifacts
   - Downscales to 256x256 with nearest-neighbor interpolation
   - Saves as `sprites/characters/[class]/[m|f]/body/[anim]_[dir]_f[##].png`

### Phase 4: Monsters

Same pipeline for 30 monster types:
- Tripo3D from monster concept images (some already in `sprites/references/hero_refs/mon_*.png`)
- Simpler animations: idle(3f), walk(4f), attack(4f), death(3f)
- 4 directions only (S, W, N, mirror W→E)
- Output: `sprites/monsters/[name]/sprites/[anim]_[dir]_f[##].png`

### Phase 5: Equipment

Render individual equipment items as transparent overlay sprites:
- Weapons (15 types), shields (3 types), headgear (10 types)
- Front-facing only, 256x256
- Output: `sprites/equipment/[category]/[item].png`

## Expected Output

When complete, the folder structure should be:
```
sprites/
├── characters/
│   ├── novice/
│   │   ├── m/body/
│   │   │   ├── idle_S_f00.png through idle_SE_f03.png  (32 files)
│   │   │   ├── walk_S_f00.png through walk_SE_f07.png  (64 files)
│   │   │   ├── attack_S_f00.png through attack_SE_f05.png (48 files)
│   │   │   └── death_S_f00.png through death_SE_f02.png (24 files)
│   │   └── f/body/ (same structure)
│   ├── [19 more classes]
├── monsters/
│   ├── poring/sprites/ (56 files: 4 dirs × 14 frames)
│   ├── [29 more monsters]
├── equipment/
│   ├── weapons/ (15 items)
│   ├── shields/ (3 items)
│   └── headgear/ (10 items)
```

Total expected: ~7,000+ sprites with **perfect frame-to-frame consistency** (same 3D model rendered from different angles/frames).

## Quality Rules

1. **Consistency**: Every frame of the same character MUST be pixel-identical in design (guaranteed by 3D rendering)
2. **Cel-shading**: Must preserve original model colors. Shadow = 70% brightness, midtone = 100%, highlight = 120%. NOT flat grey shadows.
3. **Outline**: 1px black outline on character silhouette (Solidify modifier with flipped normals)
4. **Background**: 100% transparent (RGBA PNG)
5. **Centering**: Character centered, feet at bottom 10% of frame, head at top 10%
6. **Animation loops**: Walk/idle/run must loop seamlessly (Mixamo handles this)
7. **File size**: Each 256x256 PNG should be 10-80KB
8. **No blur**: Nearest-neighbor downscale only, never bilinear

## Key Technical Notes

- Blender 5.1 uses `BLENDER_EEVEE` (not `BLENDER_EEVEE_NEXT`)
- Blender CLI: `"C:/Blender 5.1/blender.exe" --background --python script.py -- args`
- Python environment: `C:/ComfyUI/venv/` has Pillow and rembg installed
- rembg uses u2net model, first run downloads automatically
- ComfyUI can still be used for generating NEW hero refs if needed: start with `C:/ComfyUI/start_comfyui.bat`
- ROSPRITE LoRA trigger word: `ROSPRITE` at weight 0.85

## RO Character Classes (20 total)

| Class | Genders | Priority | Visual Description |
|-------|---------|----------|-------------------|
| Novice | M/F | P0 | Simple brown tunic, small knife |
| Swordsman | M/F | P0 | Light plate armor, short sword, round shield |
| Mage | M/F | P0 | Blue robe, wooden staff, pointed hat |
| Archer | M/F | P0 | Green leather vest, bow, quiver |
| Acolyte | M/F | P0 | White robes, mace, biretta hat |
| Thief | M/F | P0 | Dark leather, red scarf, daggers |
| Merchant | M/F | P0 | Apron, axe, backpack |
| Knight | M/F | P1 | Full plate armor, longsword, helmet |
| Wizard | M/F | P1 | Ornate robe, tall staff, wizard hat |
| Hunter | M/F | P1 | Green outfit, composite bow, falcon |
| Priest | M/F | P1 | White vestments, gold trim, mitre, rod |
| Assassin | M/F | P1 | Dark tight outfit, katar, face mask |
| Blacksmith | M/F | P1 | Leather apron, large hammer |
| Crusader | M/F | P2 | Holy armor with cross, sword, large shield |
| Sage | M/F | P2 | Scholar robes, cape, book |
| Bard | M only | P2 | Performer outfit, feathered hat, guitar |
| Dancer | F only | P2 | Dancing outfit, flowing fabric, whip |
| Monk | M/F | P2 | Martial arts robes, knuckle weapon |
| Rogue | M/F | P2 | Hooded cloak, dagger |
| Alchemist | M/F | P2 | Lab coat, axe, potion bottles |

## RO Monsters (30 types)

Poring, Drops, Poporing, Lunatic, Fabre, Pupa, Rocker, Thief Bug, Condor, Wolf, Goblin, Skeleton, Zombie, Orc Warrior, Poison Spore, Horn, Mandragora, Savage, Desert Wolf, Muka, Snake, Andre, Deniro, Piere, Whisper, Argos, Steel Chonchon, Elder Willow, Marina, Magnolia

---
