# 2D Sprite Pipeline — Session Context

## What This Is

A 2D sprite art generation pipeline for Sabri_MMO, an MMO inspired by Ragnarok Online. The goal is to generate all character classes, monsters, and equipment as 256x256 pixel art sprites with 8-direction rotation and animation frames (idle, walk, attack).

## What's Been Done

### Infrastructure (100% complete)
- **ComfyUI** installed at `C:/ComfyUI/` with Python 3.12 venv, PyTorch nightly cu128 (RTX 5090)
- **7 custom nodes**: IP-Adapter, ControlNet, LayerDiffuse, PixelArt-Detector, Impact Pack, WAS Suite, GameTools-2D
- **Models downloaded**: Illustrious XL v0.1 (6.5GB checkpoint), ROSPRITE v2.1C LoRA (122MB), Pixel Art XL LoRA (163MB), SDXL VAE (320MB), CLIP Vision ViT-H (2.4GB), IP-Adapter Plus SDXL (809MB)
- **Blender 5.1** installed at `C:/Blender 5.1/`
- **Blender render script** written at `2D animations/blender_sprite_render.py` — renders GLB models from 8 directions with cel-shading

### AI-Generated Sprites (Phase 1 — completed with issues)
- **~4,700 character sprites** generated across 30+ class/gender variants
- **33 hero reference images** (front-facing concepts used as IP-Adapter anchors)
- **Pipeline scripts**: `full_pipeline_v2.py` generates 1 image at a time via ComfyUI API with IP-Adapter consistency
- Organized into `sprites/characters/[class]/[m|f]/body/[anim]_[dir]_f[##].png`

### Known Issues with AI Pipeline
1. **Frame-to-frame consistency** — Even with IP-Adapter at 0.8 weight, outfit details, colors, and proportions shift between frames. Characters are recognizable but not identical frame-to-frame.
2. **Color palette drift** — Wizard goes purple→pink→red across frames. Knight's dark armor crushed all frames to near-black.
3. **No pose progression** — Idle frames don't show smooth breathing cycle; they show 4 unrelated poses. Walk frames don't show a coherent step cycle.
4. **Verdict** — The AI sprites are good concept art and prototyping placeholders but NOT production-ready animation sequences.

### 3D-to-2D Pipeline (PRODUCTION VALIDATED)
- Blender 5.1, `blender_sprite_render_v2.py` — full production pipeline
- **Female Mage**: 168 sprites (4 animations × 8 dirs) — first production run
- **Male Swordsman**: 768 sprites (20 animations × 8 dirs) — full RO Classic animation set
- **14 animation categories**: idle, walk, run, attack, attack_2h, standby, standby_2h, cast, hit, death, sit, pickup, block, taunt
- **Variant system**: Multiple FBX per animation type (e.g., 3 idles, 4 attacks) preserved as separate folders for random runtime selection
- **`--subfolders` flag**: Output organized by original FBX filename (e.g., `Stable Sword Inward Slash/`)
- **GLB→FBX conversion**: Blender export for Mixamo upload (axis_forward='-Z', axis_up='Y')
- **Classification preserved**: `_action_classification` dict stores filename→type at import, survives Blender's auto-renaming

## What Needs To Happen Next

### Production Workflow (proven)

```
1. Hero ref PNG → Tripo3D (tripo3d.ai, GLB, ~30s)
2. GLB → Blender FBX export (for Mixamo upload if GLB fails)
3. FBX/GLB → Mixamo (auto-rig + 20 animations, download as FBX "With Skin")
4. Save FBX files to 3d_models/animations/<class>/
5. Run blender_sprite_render_v2.py with --anim-dir --texture-from --subfolders
6. Output: per-animation folders with {Name}_{Direction}_f{Frame}.png
```

### Swordsman Animation Mapping (20 FBX files, proven)

| FBX Filename | Game Purpose |
|-------------|-------------|
| Breathing Idle / Idle / Idle (1) | Idle, no weapon (3 variants) |
| 2hand Idle | Idle, 2H weapon equipped |
| Fighting Idle | Combat idle between attacks |
| Walking / Walking (1) | Walk, no weapon (2 variants) |
| Run With Sword | Walk, 1H weapon equipped |
| Stable Sword Inward/Outward Slash | Attack, 1H weapon (2 variants) |
| Punching / Mutant Punch | Attack, no weapon (2 variants) |
| Great Sword Slash | Attack, 2H weapon |
| Standing 1H Cast Spell 01 | Buff/positive skill |
| Standing Taunt Chest Thump | Debuff skill (Provoke) |
| Reaction | Taking damage |
| Dying | Death |
| Sitting Idle | Sitting (2x regen) |
| Picking Up | Loot pickup |
| Sword And Shield Block | Shield block |

## File Locations

| What | Where |
|------|-------|
| ComfyUI | `C:/ComfyUI/` |
| Blender | `C:/Blender 5.1/blender.exe` |
| Pipeline docs | `C:/Sabri_MMO/2D animations/2D_Sprite_Art_Pipeline_Plan.md` |
| Implementation guide | `C:/Sabri_MMO/2D animations/Detailed_Implementation_Guide.md` |
| Blender render script | `C:/Sabri_MMO/2D animations/blender_sprite_render.py` |
| ComfyUI pipeline v2 | `C:/ComfyUI/full_pipeline_v2.py` |
| Hero reference images | `C:/Sabri_MMO/2D animations/sprites/references/hero_refs/` |
| Generated sprites | `C:/Sabri_MMO/2D animations/sprites/characters/` |
| Prompt CSVs | `C:/Sabri_MMO/2D animations/prompts/` |
| Master palette | `C:/Sabri_MMO/2D animations/sprites/references/master_palette.txt` |

## Hardware

- **GPU**: NVIDIA RTX 5090 (32GB VRAM)
- **ComfyUI generation speed**: ~5 sec/image (SDXL + LoRA + IP-Adapter)
- **Blender render speed**: ~0.5 sec/frame (Eevee, 512x512)

## Key Technical Details

### ComfyUI Startup
```bash
cd C:/ComfyUI
source venv/Scripts/activate  # or: venv\Scripts\activate on Windows
python main.py --preview-method auto --disable-auto-launch
# Opens at http://127.0.0.1:8188
```

### RTX 5090 Critical Rules
1. PyTorch MUST be nightly cu128+ (sm_120 kernels for Blackwell)
2. NEVER install xformers (force-downgrades PyTorch)
3. tqdm patched at `venv/Lib/site-packages/tqdm/std.py` line 447 (OSError catch for headless mode)
4. ComfyUI logger patched at `app/logger.py` line 34 (same OSError fix)

### Blender Command Line Rendering
```bash
"C:/Blender 5.1/blender.exe" --background --python "C:/Sabri_MMO/2D animations/blender_sprite_render.py" -- <model.glb> <output_dir> <num_directions>
```

### ROSPRITE LoRA Details
- Trigger word: `ROSPRITE`
- Weight: 0.8-0.85
- Trained on 190+ RO sprite images, 10 epochs
- Works with Illustrious XL base model
- Civitai model ID: 1043663 (version 1334272)
