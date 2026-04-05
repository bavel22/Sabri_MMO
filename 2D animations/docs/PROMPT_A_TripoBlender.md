# Prompt A — Tripo3D → Blender Sprite Pipeline

Copy this entire prompt into a new Claude Code session:

---

## Task

I need you to continue building my 2D sprite art pipeline for Sabri_MMO (a Ragnarok Online-inspired MMO). The previous session set up ComfyUI and generated ~4,700 AI sprites, but found that AI image-by-image generation has fatal consistency problems (outfit/color/proportion drift between frames). We determined the **3D-to-2D pipeline** is the correct approach.

**Your job: Take 3D character models (GLB files) and render them into 8-direction sprite sheets with animations using Blender.**

## Context

Read these files first for full background:
- `C:/Sabri_MMO/2D animations/SESSION_CONTEXT.md` — Complete status of what's been done
- `C:/Sabri_MMO/2D animations/blender_sprite_render.py` — Existing Blender render script (already tested, works)
- `C:/Sabri_MMO/2D animations/2D_Sprite_Art_Pipeline_Plan.md` — Full research document (1,457 lines)
- `C:/Sabri_MMO/2D animations/Detailed_Implementation_Guide.md` — Implementation details (1,686 lines)

## What's Already Installed
- **Blender 5.1** at `C:/Blender 5.1/blender.exe` (verified working from CLI)
- **ComfyUI** at `C:/ComfyUI/` with Illustrious XL + ROSPRITE LoRA (for concept art if needed)
- **Python 3.12** in ComfyUI venv with Pillow, rembg
- **RTX 5090** (32GB VRAM)
- **33 hero reference images** at `C:/Sabri_MMO/2D animations/sprites/references/hero_refs/` — these are the character concept art we generated via AI

## What I Need You To Do

### Step 1: Get 3D Models
I have uploaded GLB files from Tripo3D to: `C:/Sabri_MMO/2D animations/3d_models/`

(If no models exist there yet, help me set up the Tripo3D workflow — I have a free account. I need to upload the hero ref images from `sprites/references/hero_refs/` and download the resulting GLB files.)

### Step 2: Improve the Blender Render Script
The existing `blender_sprite_render.py` works but needs:
- Better cel-shading (currently too dark — needs color-preserving toon shader)
- Support for Mixamo FBX animations (walk/idle/attack/death)
- Automatic camera framing (zoom to fit character tightly)
- Post-processing integration (rembg background removal + downscale to 256x256)
- Batch mode: process ALL models in a directory

### Step 3: Render All Characters
For each character class (20 classes × 2 genders = ~38 variants):
- 8 directions (S, SW, W, NW, N, NE, E, SE)
- Animations: idle (4f), walk (8f), attack (6f), death (3f)
- Output: `sprites/characters/[class]/[m|f]/body/[anim]_[dir]_f[##].png`
- Resolution: 256x256 with transparent background

### Step 4: Render Monsters
Same approach for 30 monster types (4 directions × animations).

### Step 5: Equipment Overlays
Render weapons/shields/headgear as separate transparent overlay sprites.

## Key Technical Details

### Blender CLI
```bash
"C:/Blender 5.1/blender.exe" --background --python script.py -- args
```

### Target Specs
- 256x256 final sprite size
- Transparent PNG (RGBA)
- Cel-shaded with 1px black outline
- Isometric camera angle (~30° from horizontal, like RO)
- 8 directions, camera rotates around model
- Nearest-neighbor downscale (no bilinear blur)

### Quality Rules
1. Every frame of the same character MUST look identical (same model = guaranteed)
2. Animations must loop smoothly (Mixamo handles this)
3. Background must be 100% transparent
4. Character must be centered and fill ~80% of the frame
5. Cel-shading should preserve the model's original colors (not crush to dark)

### RO Character Classes
Novice, Swordsman, Mage, Archer, Acolyte, Thief, Merchant, Knight, Wizard, Hunter, Priest, Assassin, Blacksmith, Crusader, Sage, Bard (male), Dancer (female), Monk, Rogue, Alchemist

### RO Monsters (priority)
Poring, Drops, Lunatic, Fabre, Rocker, Goblin, Skeleton, Zombie, Wolf, Orc Warrior, Poison Spore, Mandragora, Savage, Condor, Whisper + 15 more

---
