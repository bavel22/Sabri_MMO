# 2D Sprite Art Generation Pipeline for Sabri_MMO

## Executive Summary

This document details a complete pipeline for generating 2D sprite art and animations in the style of Ragnarok Online for Sabri_MMO, using a local RTX 5090 GPU (32GB VRAM). After 25+ research iterations across academic papers, community projects, commercial tools, and technical documentation, the recommended approach is a **hybrid pipeline** combining:

1. **Local AI generation** (ComfyUI + Flux/SDXL + custom LoRAs) for base character/monster art
2. **PixelLab** for multi-directional sprite rotation and animation
3. **Aseprite** for post-processing, palette management, and animation polish
4. **Composite sprite layering** for equipment variations (RO-style body/head/equipment layers)
5. **UE5 Paper2D + PaperZD** for in-engine integration with billboard rendering

**Confidence Level**: HIGH — Every component of this pipeline has working implementations validated by indie developers and the AI art community in 2025-2026.

---

## Table of Contents

1. [Ragnarok Online Sprite Technical Reference](#1-ragnarok-online-sprite-technical-reference)
2. [Pipeline Architecture Overview](#2-pipeline-architecture-overview)
3. [Phase 1: Local AI Generation Setup (ComfyUI)](#3-phase-1-local-ai-generation-setup)
4. [Phase 2: LoRA Training for RO Style](#4-phase-2-lora-training-for-ro-style)
5. [Phase 3: Character Sprite Generation Workflow](#5-phase-3-character-sprite-generation-workflow)
6. [Phase 4: Multi-Directional Sprite Rotation](#6-phase-4-multi-directional-sprite-rotation)
7. [Phase 5: Animation Generation](#7-phase-5-animation-generation)
8. [Phase 6: Monster/Enemy Sprite Generation](#8-phase-6-monster-enemy-sprite-generation)
9. [Phase 7: Equipment & Variation System](#9-phase-7-equipment-variation-system)
10. [Phase 8: Post-Processing Pipeline](#10-phase-8-post-processing-pipeline)
11. [Phase 9: UE5 Integration](#11-phase-9-ue5-integration)
12. [Phase 10: Production Workflow & Batch Pipeline](#12-phase-10-production-workflow-batch-pipeline)
13. [Tool Comparison Matrix](#13-tool-comparison-matrix)
14. [Cost & Time Estimates](#14-cost-time-estimates)
15. [Sources](#15-sources)

---

## 1. Ragnarok Online Sprite Technical Reference

Understanding the target format is critical. RO uses a very specific sprite system:

### 1.1 Sprite Dimensions & Format
| Property | Value |
|----------|-------|
| **Frame size** | Varies per entity — typically 50-120px wide, 60-150px tall |
| **Art style** | 2D pixel art, chibi proportions (2:1 to 2.5:1 head-to-body ratio) |
| **Color depth** | 256-color indexed palette (BMP segment) + truecolor ABGR (TGA segment) |
| **Background** | Palette index 0 = transparent (always, regardless of alpha value) |
| **Compression** | RLE on background pixels (v2.1) — consecutive zeros encoded as `00 [count]` |
| **File format** | `.spr` (sprite atlas) + `.act` (animation data) paired files |

### 1.2 Animation Structure
| Action | Directions | Frames per Direction | Total Frames |
|--------|-----------|---------------------|-------------|
| **Idle/Standing** | 8 | 3-4 | 24-32 |
| **Walking** | 8 | 8 (max framerate) | 64 |
| **Attacking** | 8 | 6-10 | 48-80 |
| **Casting** | 8 | 4-6 | 32-48 |
| **Taking Damage** | 8 | 2-3 | 16-24 |
| **Death** | 8 | 4-6 | 32-48 |
| **Sitting** | 8 | 1-2 | 8-16 |
| **Total per class** | — | — | ~224-312 frames |

A full character class (e.g., Knight) has approximately **240 frames with 150 unique sprites** for running alone. The max framerate is **8 frames per one movement in one direction**.

### 1.3 Sprite Layering System (Critical for Equipment)
RO composites multiple separate sprite files at render time:

```
Layer Order (back to front):
1. Shadow (ground shadow circle)
2. Body sprite (class-specific: Knight, Priest, etc.)
3. Head sprite (hair style + color)
4. Lower headgear (e.g., Iron Cain)
5. Middle headgear (e.g., Sunglasses)
6. Upper headgear (e.g., Helm)
7. Garment/Robe (optional cape/robe overlay)
8. Weapon (drawn in weapon hand position)
9. Shield (drawn in shield hand position)
```

Each layer has its own `.spr`/`.act` pair. **Anchors** in the ACT file define connection points between layers, allowing body, head, and equipment sprites to align correctly across all frames and directions.

### 1.4 Chibi Proportions
- **Head-to-body ratio**: ~2:1 to 2.5:1 (head is roughly 40-50% of total height)
- **Eyes**: Oversized, expressive, dominate upper face
- **Limbs**: Stubby, simplified, minimal detail
- **Overall**: Rounded shapes, exaggerated expressions
- **Color**: Bold, saturated palette with clear outlines

### 1.5 Direction Convention
```
RO uses 8 directions (0-7):
     0 (South)
  1 (SW)  7 (SE)
  2 (West)  6 (East)
  3 (NW)  5 (NE)
     4 (North)
```
The isometric camera angle means "South" faces toward the camera (down-left in screen space).

---

## 2. Pipeline Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    FULL SPRITE PIPELINE                         │
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │ PHASE 1-2    │    │ PHASE 3-4    │    │ PHASE 5      │      │
│  │ Setup &      │───▶│ Generate     │───▶│ Animate      │      │
│  │ LoRA Train   │    │ Base Art     │    │ Sprites      │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         │                   │                   │               │
│         ▼                   ▼                   ▼               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │ ComfyUI +    │    │ Flux/SDXL +  │    │ PixelLab     │      │
│  │ Kohya_ss     │    │ ControlNet   │    │ or ComfyUI   │      │
│  │ (RTX 5090)   │    │ + IP-Adapter │    │ Batch Gen    │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │ PHASE 6-7    │    │ PHASE 8      │    │ PHASE 9-10   │      │
│  │ Monsters &   │───▶│ Post-Process │───▶│ UE5 Import   │      │
│  │ Equipment    │    │ & Polish     │    │ & Production │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         │                   │                   │               │
│         ▼                   ▼                   ▼               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │ Batch Gen    │    │ Aseprite +   │    │ Paper2D +    │      │
│  │ Style LoRA   │    │ BG Removal   │    │ PaperZD +    │      │
│  │ Consistency  │    │ Palette Fix  │    │ Billboard    │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
└─────────────────────────────────────────────────────────────────┘
```

### Recommended Primary Approach: AI Generation + Post-Processing

After evaluating all options, the recommended approach is:

| Component | Primary Tool | Backup Tool |
|-----------|-------------|-------------|
| **Base art generation** | ComfyUI + Flux LoRA (local) | SDXL + Illustrious XL |
| **RO style consistency** | Custom-trained style LoRA | Existing Civitai RO LoRAs |
| **Multi-direction rotation** | PixelLab ($9-22/mo) | ComfyUI + ControlNet DWPose |
| **Animation frames** | PixelLab skeleton animation | ComfyUI batch + pose refs |
| **Sprite sheet assembly** | ComfyUI SpriteSheetMaker | TexturePacker |
| **Post-processing** | Aseprite | GIMP/Krita |
| **Background removal** | ComfyUI BG removal node | Sprite Buff |
| **Palette management** | Aseprite palette swap | Custom UE5 material shader |
| **UE5 integration** | Paper2D + PaperZD plugin | Custom billboard material |
| **LoRA training** | Kohya_ss GUI | ai-toolkit |

### Why Not Pure AI or Pure Manual?

| Approach | Pros | Cons |
|----------|------|------|
| **Pure AI** | Fastest generation | Inconsistency between frames, no fine control over animation timing |
| **Pure Manual** | Perfect control | Months of work per class, requires skilled pixel artist |
| **Hybrid (recommended)** | AI generates 80% of work, manual polish for quality | Some learning curve for tools |
| **3D-to-2D pre-render** | Perfect consistency, easy animation | Loses the hand-drawn pixel art charm of RO |

---

## 3. Phase 1: Local AI Generation Setup

### 3.1 Hardware Specifications
Your RTX 5090 is one of the best consumer GPUs for this workflow:

| Spec | RTX 5090 Value | Impact |
|------|---------------|--------|
| **VRAM** | 32 GB GDDR7 | Run Flux/SDXL with multiple LoRAs + IP-Adapter simultaneously |
| **CUDA Cores** | 21,760 | ~50% faster than 4090 for generation |
| **SDXL 1024x1024** | ~6.2s/image (batch 1), ~3.8s/image (batch 4) | Rapid iteration |
| **Flux DEV** | ~5.5s/image (optimized) | High-quality generation |
| **Batch capacity** | 4x 1024x1024 at 100% VRAM | Parallel sprite generation |
| **LoRA training** | ~10 hours for full finetune | Overnight training feasible |

### 3.2 ComfyUI Installation (RTX 5090 Specific)

**CRITICAL**: Standard install guides may fail on RTX 5090 Blackwell architecture. Use the verified approach:

```bash
# Option A: One-click installer (RECOMMENDED)
# Download ComfyUI-Win-Blackwell from GitHub
# Run setup.bat — takes ~20 minutes
# Automatically configures PyTorch nightly cu130

# Option B: Manual setup
# 1. Install Python 3.11+ and Git
# 2. Clone ComfyUI
git clone https://github.com/comfyanonymous/ComfyUI.git
cd ComfyUI

# 3. Create venv
python -m venv venv
venv\Scripts\activate

# 4. CRITICAL: Install PyTorch nightly with CUDA 13.0 (sm_120 kernels)
pip install --pre torch torchvision torchaudio --index-url https://download.pytorch.org/whl/nightly/cu130

# 5. Install requirements
pip install -r requirements.txt

# 6. Install essential custom nodes (see section 3.3)
```

**RTX 5090 Setup Rules**:
1. **Use PyTorch nightly cu130** — stable builds lack sm_120 kernels for Blackwell
2. **NEVER install xformers** — it force-downgrades PyTorch to stable
3. **Strip `torch` from every custom node's requirements.txt** — prevents pip from replacing nightly with stable
4. **Install SageAttention** for additional 15-30% speedup
5. **Optional: Triton + FlashAttention** for maximum performance

### 3.3 Essential ComfyUI Custom Nodes

```
Required nodes for sprite workflow:
├── ComfyUI-SpriteSheetMaker          # Arrange frames into sprite sheets
├── comfyui_controlnet_aux            # DWPose/OpenPose for pose control
├── ComfyUI_IPAdapter_plus            # Character consistency via reference images
├── comfyui-art-venture               # Background removal
├── ComfyUI-KJNodes                   # Batch processing utilities
├── ComfyUI-Impact-Pack               # Detailing, face fix, upscale
├── ComfyUI-Custom-Scripts            # CSV prompt loading for batch
└── ComfyUI_essentials                # General utility nodes
```

### 3.4 Model Downloads

| Model | Purpose | Size | Source |
|-------|---------|------|--------|
| **Flux.1 Dev** | Best quality base model | ~12 GB | HuggingFace (black-forest-labs) |
| **SDXL Base 1.0** | Alternative base, larger LoRA ecosystem | ~6.5 GB | HuggingFace (stabilityai) |
| **Illustrious XL / NoobAI** | Best anime/pixel art SDXL finetune | ~6.5 GB | Civitai |
| **Flux-2D-Game-Assets-LoRA** | Game asset style LoRA | ~200 MB | Replicate / HuggingFace |
| **Retro-Pixel-Flux-LoRA** | Retro pixel art LoRA | ~200 MB | HuggingFace |
| **Pixel-Art-XL LoRA** | Pixel art for SDXL | ~200 MB | HuggingFace (nerijs) |
| **RO Sprite LoRA** (Illustrious) | Ragnarok Online sprite style | ~106 MB | Civitai (model 1242746) |
| **ROSL LoRA** (SDXL) | RO style LoRA | ~200 MB | Civitai (model 762637) |
| **ControlNet DWPose** | Pose estimation/control | ~1.5 GB | HuggingFace |
| **IP-Adapter Plus (SDXL)** | Character identity preservation | ~1 GB | HuggingFace |

---

## 4. Phase 2: LoRA Training for RO Style

### 4.1 Strategy: Two-Layer LoRA System

Train TWO separate LoRAs and combine them at generation time:

```
Layer 1: STYLE LoRA — "RO Chibi Pixel Art Style"
  └─ Captures: chibi proportions, pixel art rendering, color palette, outline style
  └─ Used for ALL sprites (characters + monsters + NPCs)
  └─ Trained ONCE, used everywhere

Layer 2: CHARACTER LoRAs — One per character class/monster
  └─ Captures: specific character design, outfit, colors, silhouette
  └─ Used when generating that specific character
  └─ Trained per-class (Knight, Priest, etc.)
```

### 4.2 Dataset Preparation

**Extracting RO Sprites for Training Data**:
```bash
# Tools for extracting SPR files:
# 1. pyro-tools (Python) — https://github.com/MyGodIsHe/pyro-tools
# 2. zrenderer (Zig/Web) — https://github.com/zhad3/zrenderer
# 3. SprViewer (Windows GUI) — SourceForge

# Extract sprites from RO client data.grf:
# 1. Use GRF Editor to extract .spr/.act files
# 2. Use zrenderer or pyro-tools to render individual frames as PNGs
# 3. Organize by class/direction/action
```

**Style LoRA Dataset** (30-50 images):
- Extract clean renders of 8-10 different RO character classes
- Include front, side, 3/4 views
- Include both male and female versions
- Include 2-3 monster sprites for variety
- Resolution: 512x512 or 768x768 (upscale from original)
- **Captions**: `"pixel art sprite, chibi character, ragnarok online style, [class name], [direction], simple background, full body"`

**Character LoRA Dataset** (15-25 images per class):
- Extract all views of one specific class
- 3-5 images per angle (front, side, back, 3/4)
- Include key poses (idle, walk mid-stride, attack)
- Same resolution and captioning format

### 4.3 Training with Kohya_ss GUI

**Installation**:
```bash
git clone https://github.com/bmaltais/kohya_ss.git
cd kohya_ss
setup.bat  # Windows installer

# Or use ai-toolkit for Flux:
# git clone https://github.com/ostris/ai-toolkit.git
```

**Training Parameters for Style LoRA (SDXL/Illustrious)**:

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Base model** | Illustrious XL or NoobAI 0.5 | Best anime tag understanding |
| **Network type** | LoRA | Standard, well-supported |
| **Network rank (dim)** | 32-64 | Higher for style capture |
| **Network alpha** | 16-32 | Half of rank is standard |
| **Learning rate** | 1e-4 to 2e-4 | Conservative for style stability |
| **LR scheduler** | cosine_with_restarts | Smooth convergence |
| **Epochs** | 15-25 | Monitor for overfitting |
| **Batch size** | 2-4 | RTX 5090 handles 4 easily |
| **Resolution** | 768x768 | Good detail for sprites |
| **Optimizer** | AdamW8bit | Memory efficient |
| **Caption dropout** | 0.1 | Prevents over-reliance on captions |
| **Noise offset** | 0.05 | Better contrast |
| **Mixed precision** | bf16 | Best for Blackwell GPUs |

**Training Parameters for Flux LoRA**:

| Parameter | Value |
|-----------|-------|
| **Base model** | Flux.1 Dev |
| **Network rank** | 16-32 |
| **Learning rate** | 4e-5 |
| **Epochs** | 20-30 |
| **Resolution** | 512x512 to 1024x1024 |
| **Training time** | ~2-4 hours on RTX 5090 |

### 4.4 Testing & Iteration

After training, test with prompts like:
```
# Test prompt (Illustrious/SDXL):
"pixel art sprite, chibi character, ragnarok online style, knight class,
full body, front view, simple white background, sword, armor,
<lora:ro_style:0.9> <lora:knight_char:0.8>"

# Test prompt (Flux):
"GRPZA pixel art sprite of a chibi knight character in ragnarok online style,
full body, front facing, white background, medieval armor and sword"
```

Generate 20-30 test images. If results are:
- **Too generic**: Increase LoRA strength (0.9-1.2), add more training images
- **Overfitting**: Reduce epochs, increase regularization images, lower LoRA strength
- **Wrong proportions**: Add more explicit chibi proportion references to dataset
- **Colors off**: Curate dataset to emphasize RO's saturated palette

---

## 5. Phase 3: Character Sprite Generation Workflow

### 5.1 Single Character Concept Generation

**Step 1: Generate the base concept art**

Use ComfyUI with your trained Style LoRA + Character LoRA:

```
Prompt Template:
"pixel art sprite, chibi character, ragnarok online style,
[CLASS] class, [GENDER], full body, [DIRECTION] view,
simple white background, [EQUIPMENT DESCRIPTION],
detailed pixel shading, high contrast, crisp edges, no anti-aliasing,
32-bit color palette, isometric perspective"

Negative Prompt:
"blurry, smooth, realistic proportions, 3D render,
photograph, watermark, text, bad anatomy,
extra limbs, deformed, low quality"
```

**Step 2: Generate the reference sheet (turnaround)**

Generate the character from 4 key angles first:
1. Front (South) — primary reference
2. Side-Left (West) — profile
3. Back (North) — rear view
4. 3/4 Front (SE) — three-quarter

Use **IP-Adapter** to maintain consistency:
- Generate the front view first
- Feed it as IP-Adapter reference for all subsequent angles
- Use ControlNet DWPose with pre-drawn skeleton poses for each angle
- Lock seed + sampling parameters for maximum coherence

### 5.2 Multi-Angle Generation with ControlNet

```
ComfyUI Workflow:
1. Load base model (Illustrious XL)
2. Load Style LoRA (ro_style) at 0.9
3. Load Character LoRA (knight) at 0.8
4. Load reference image via IP-Adapter (strength 0.7-0.9)
5. Load pose skeleton via ControlNet DWPose (strength 0.6-0.8)
6. Generate at 768x768
7. Background removal node
8. Downscale to target sprite size (e.g., 96x128)
```

**Pose Skeleton Templates** (create 8 skeleton images):
- Draw or extract simple stick figure poses for each of the 8 RO directions
- Use these as ControlNet conditioning for consistent body positioning
- Same skeleton = consistent proportions across all directions

### 5.3 Character Classes to Generate

For Sabri_MMO with RO Classic classes:

| Priority | Class | Male/Female | Equipment Variants |
|----------|-------|-------------|-------------------|
| P0 | Novice | Both | Knife, Cotton Shirt |
| P0 | Swordsman | Both | Sword, Buckler |
| P0 | Mage | Both | Rod, Mage Hat |
| P0 | Archer | Both | Bow, Quiver |
| P0 | Acolyte | Both | Mace, Biretta |
| P0 | Thief | Both | Dagger x2 |
| P0 | Merchant | Both | Axe, Cart |
| P1 | Knight | Both | Sword/Spear, Full Plate |
| P1 | Wizard | Both | Staff, Wizard Hat |
| P1 | Hunter | Both | Bow, Falcon |
| P1 | Priest | Both | Mace, Mitre |
| P1 | Assassin | Both | Katar/Dagger |
| P1 | Blacksmith | Both | Hammer, Apron |
| P2 | Crusader | Both | Sword, Shield, Mount |
| P2 | Sage | Both | Book, Cape |
| P2 | Bard/Dancer | Male/Female | Instrument/Whip |
| P2 | Monk | Both | Knuckle, Robes |
| P2 | Rogue | Both | Dagger, Bow |
| P2 | Alchemist | Both | Axe, Bottles |

**Total: ~38 character variants** (19 classes x 2 genders)

---

## 6. Phase 4: Multi-Directional Sprite Rotation

### 6.1 Approach A: PixelLab (Recommended for Quality)

PixelLab is the **strongest tool** for this specific task. It's purpose-built for multi-directional sprite generation:

**Capabilities**:
- Generate 4 or 8 directional views from a single reference image
- AI-driven rotation that maintains character identity, proportions, and colors
- Works natively with pixel art at various resolutions (32x32 to 128x128)
- Aseprite plugin integration for seamless workflow
- Skeleton-based animation control

**Workflow**:
1. Generate your best front-facing character in ComfyUI (high-res, 768x768)
2. Import into PixelLab as reference image
3. Use "Directional Rotation" feature for 8-direction generation
4. Review and manually fix any inconsistencies
5. Export as sprite sheet or individual frames

**Pricing**: $9/mo (Tier 1) or $22/mo (Tier 2)
- ~2,000-3,000 credits/month for active generation
- Basic generation: 1 credit/request; Pro features: 40 credits/request

**Quality Assessment**: PixelLab is "the only tool consistently able to generate high quality, style accurate, and usable pixel art game assets" — produces clean edges, sharp corners, and stable color palettes. Better results at larger sprite sizes (64x64+); weaker at 16x16 or smaller.

### 6.2 Approach B: ComfyUI + ControlNet (Fully Local, Free)

For 100% local generation without any subscription:

1. **Create 8 pose skeleton templates** — Draw simple stick figures for each RO direction
2. **Use ControlNet DWPose** — Feed skeleton + IP-Adapter reference of front view
3. **Lock parameters** — Same seed base, same LoRA strengths, same CFG
4. **Batch generate** — Queue all 8 directions as a batch with CSV prompt loading
5. **Post-process** — Background removal, palette normalization in Aseprite

**Pros**: Free, fully local, unlimited generations
**Cons**: More manual work to achieve consistency, requires pose template creation

### 6.3 Mirroring Optimization

RO sprites mirror left-right directions to save sprite count:
- Generate 5 unique directions: S, SW, W, NW, N
- Mirror SW → SE, W → E, NW → NE
- This cuts generation work by ~37%
- Handle weapon-hand swapping in the mirrored sprites

---

## 7. Phase 5: Animation Generation

### 7.1 Animation Types Needed

For each character class, generate these animation sets:

| Animation | Frames/Direction | Priority | Technique |
|-----------|-----------------|----------|-----------|
| **Idle** | 3-4 frames | P0 | Subtle breathing/sway |
| **Walk Cycle** | 6-8 frames | P0 | Full step cycle, looping |
| **Attack (Melee)** | 6-8 frames | P0 | Weapon swing arc |
| **Attack (Ranged)** | 4-6 frames | P0 | Draw + release |
| **Cast Spell** | 4-6 frames | P1 | Arms raised, magical pose |
| **Take Damage** | 2-3 frames | P1 | Flinch/recoil |
| **Death** | 4-6 frames | P1 | Fall to ground |
| **Sit** | 1-2 frames | P2 | Seated pose |
| **Pick Up Item** | 2-3 frames | P2 | Bend down |

### 7.2 Animation Generation Approaches

**Approach A: PixelLab Skeleton Animation (Recommended)**

PixelLab offers skeleton-based animation where you:
1. Upload your character sprite
2. Define skeleton joints on the character
3. Describe the animation in text or use skeleton controls
4. AI generates frame-by-frame animation
5. Export as sprite sheet

This produces 4-16 frames per generation depending on sprite size:
- 32x32 sprites: 16 frames per generation
- 128x128 sprites: 4 frames per generation

**Approach B: ComfyUI Pose-Driven Batch Generation**

1. Create pose reference frames for each animation keyframe
2. Use ControlNet DWPose + IP-Adapter for each frame
3. Generate frames in sequence with seed variation (+1 per frame)
4. Assemble into sprite sheet with SpriteSheetMaker node

**Approach C: AI Frame Interpolation**

1. Generate only keyframes (3-4 per animation) using ComfyUI
2. Use RIFE or FILM for AI frame interpolation between keyframes
3. Clean up interpolated frames in Aseprite
4. This approach works well for smooth animations like walking

**Approach D: Sprite Sheet Diffusion (Research Paper)**

The academic "Sprite Sheet Diffusion" paper (arXiv 2412.03685) demonstrates:
- ReferenceNet for consistent appearance across frames
- Pose Guider (4 conv layers) for precise pose control
- Motion Module for temporal stability
- Subject Consistency score: 0.901 (high)
- Available on GitHub: `chenganhsieh/SpriteSheetDiffusion`
- Limitation: Struggles with fine details like hairstyles and props

### 7.3 Walk Cycle Workflow (Detailed Example)

```
WALK CYCLE — 8 frames, South-facing direction:

Frame 1: Standing, right foot slightly forward (contact)
Frame 2: Right foot passing, left foot planted (passing)
Frame 3: Right foot forward, weight shifting (contact)
Frame 4: Both feet close, weight centered (passing)
Frame 5: Left foot slightly forward (contact — mirror of F1)
Frame 6: Left foot passing, right foot planted (passing — mirror of F2)
Frame 7: Left foot forward, weight shifting (contact — mirror of F3)
Frame 8: Both feet close, returning to start (passing — mirror of F4)

Generation strategy:
1. Generate frames 1-4 with ControlNet pose references
2. Mirror frames 1-4 horizontally for frames 5-8 (if character is symmetric)
3. Or generate all 8 frames independently for asymmetric characters (weapon in one hand)
4. Verify loop: Frame 8 → Frame 1 must be seamless
```

### 7.4 Animation Frame Rate

| Game Context | Recommended FPS | Notes |
|-------------|----------------|-------|
| RO Classic authentic | 8 FPS | Matches original game feel |
| Modern "enhanced" | 12 FPS | Smoother but still pixel art |
| Your game (recommended) | 8-12 FPS | Balance of authenticity and smoothness |
| Idle animations | 4-6 FPS | Slow, subtle movement |
| Attack animations | 10-12 FPS | Fast, impactful |

---

## 8. Phase 6: Monster/Enemy Sprite Generation

### 8.1 Monster Sprite Requirements

Sabri_MMO has **509 RO monster templates**. Prioritize by spawn frequency:

| Tier | Count | Examples | Sprite Complexity |
|------|-------|---------|-------------------|
| **Common** (46 active spawns) | ~46 | Poring, Lunatic, Fabre | Simple, 4-direction |
| **Field bosses** | ~20 | Golden Thief Bug, Eddga | Complex, 8-direction |
| **MVP bosses** | ~15 | Baphomet, Dark Lord | Detailed, 8-direction, special animations |
| **Remaining** | ~428 | Various | Can reuse base sprites with palette swaps |

### 8.2 Monster Generation Strategy

**Step 1: Generate "archetype" monsters**

Many RO monsters share base forms. Generate archetypes first:
- Slime-type (Poring family) — 1 base, palette swap for Drops/Poporing/Marin
- Insect-type (Rocker, Hornet) — 1 base per body plan
- Humanoid-type (Goblin, Orc) — share body proportions
- Beast-type (Lunatic, Savage) — quadruped base
- Undead-type (Zombie, Skeleton) — humanoid variant
- Plant-type (Mandragora, Flora) — unique per species
- Boss-type — fully unique per boss

**Step 2: Palette swap for variants**

RO extensively uses palette swaps for monster families:
```
Poring (pink) → Drops (orange) → Poporing (green) → Marin (blue)
Same sprite, different palette = instant variety
```

Use Aseprite's indexed color mode to swap palettes programmatically.

**Step 3: Batch generation workflow**

```
For each archetype monster:
1. Generate concept art with Style LoRA (ComfyUI)
2. Create 4-direction versions (most monsters only need 4, not 8)
3. Generate 3 animations: idle (3 frames), walk (4 frames), attack (4 frames)
4. Death animation: 3 frames
5. Total per monster: ~56 frames (4 directions × 14 frames)
6. Apply palette swaps for variants: 5-10 minutes each in Aseprite
```

### 8.3 Monster Art Style Considerations

- Monsters in RO have **less strict chibi proportions** than characters
- Boss monsters can be 2-4x the size of player characters
- Many monsters have unique silhouettes (Poring = bouncing blob, not humanoid)
- Elemental effects (fire, ice, etc.) can be added as particle overlays in UE5 rather than baked into sprites

---

## 9. Phase 7: Equipment & Variation System

### 9.1 Composite Sprite System (RO-Style)

Rather than generating every equipment combination, use a **layered composite system**:

```
Final Rendered Character = Body Layer + Head Layer + Equipment Layers

Benefits:
- 10 body types × 5 heads × 30 headgears = 1,500 combinations
- Only need 45 sprite sets instead of 1,500
- Equipment changes don't require new full-body sprites
```

### 9.2 Layer Generation Strategy

**Body Sprites** (per class, per gender):
- Generate full character WITHOUT equipment details
- Just the base class outfit (tunic, robes, armor base)
- 8 directions × ~14 frames/animation × 7 animations = ~784 frames per body

**Head Sprites** (per hair style × hair color):
- Generate heads separately at a fixed anchor point
- RO has ~30 hair styles × 9 colors = ~270 head variants
- Each head: 8 directions × 2-3 expression frames = ~20 frames
- Use **palette swap** for hair colors (9 colors from 1 generated set)

**Equipment Overlay Sprites**:
- Headgears (helmets, hats, masks): Generate as transparent overlays
- Weapons: Generate per weapon type, positioned at hand anchor
- Shields: Generate per shield type
- Garments/Capes: Generate as back-layer overlays

### 9.3 Palette Swap System for Equipment Colors

Instead of generating every armor color, use a **palette swap shader** in UE5:

```
Technique:
1. Generate sprites using indexed colors (limited palette)
2. Create a "palette texture" (1×N pixel strip)
3. UE5 material reads sprite pixel → looks up palette texture → outputs final color
4. Swap palette texture at runtime for different equipment tiers

Example:
- Leather Armor: Brown palette → tan, brown, dark brown
- Iron Armor: Grey palette → silver, grey, dark grey
- Gold Armor: Yellow palette → gold, amber, dark gold
- Same sprite frames, different color palettes
```

Material setup in UE5:
```
Material Nodes:
1. Texture Sample (sprite) → extract R channel as palette index
2. Texture Sample (palette strip) → use index as UV.x
3. Output: palette color × sprite alpha
4. Blend Mode: Masked (for transparency)
```

### 9.4 Anchor Point System

For layering to work, every sprite frame needs consistent **anchor points**:

```
Anchor Points per Frame:
├── Head anchor (where head sprite attaches to body)
├── Right hand anchor (weapon position)
├── Left hand anchor (shield position)
├── Top head anchor (headgear position)
├── Middle head anchor (mid-headgear like glasses)
└── Back anchor (garment/cape position)

Define these once per body type animation set.
Store as JSON metadata alongside sprite sheets.
```

---

## 10. Phase 8: Post-Processing Pipeline

### 10.1 Background Removal

AI-generated sprites will have backgrounds. Remove them:

**ComfyUI Method** (in-pipeline):
- Use `comfyui-art-venture` background removal node
- Outputs RGBA PNG with transparent background
- Process: Generate → BG Remove → Save PNG

**Batch Method** (post-generation):
- **Sprite Buff** (web tool) — professional BG removal for game sprites
- **ImageMagick** CLI for batch processing:
  ```bash
  # Remove white background, make transparent
  magick mogrify -transparent white *.png

  # Remove green screen
  magick mogrify -fuzz 10% -transparent "#00FF00" *.png
  ```

### 10.2 Aseprite Post-Processing

Aseprite is the industry standard for sprite polish:

```
Post-processing checklist per sprite:
1. Import AI-generated frame
2. Convert to indexed color (256 colors max, matching RO spec)
3. Clean up stray pixels and artifacts
4. Verify consistent palette across all frames
5. Adjust animation timing (frame duration per frame)
6. Enable onion skinning to check frame-to-frame consistency
7. Export as sprite sheet (horizontal strip or grid)
```

**Aseprite Plugins**:
- **Retro Diffusion Extension** — AI generation directly inside Aseprite (for touch-ups)
- **PixelLab Aseprite Plugin** — Generate and iterate within Aseprite

### 10.3 Palette Normalization

Ensure all sprites share a consistent master palette:

```
Master Palette Strategy:
1. Define a 256-color master palette for the game
   - 16 skin tones
   - 16 hair colors (2 per base color × 8 base colors)
   - 32 armor/equipment colors (4 tones × 8 material types)
   - 32 weapon colors
   - 16 magic/elemental colors
   - 64 monster colors
   - 32 environment colors
   - 32 UI colors
   - 16 reserved

2. After AI generation, remap all sprites to master palette
3. Aseprite: Sprite → Color Mode → Indexed → Remap to master palette
4. This ensures visual cohesion across the entire game
```

### 10.4 Quality Checklist

```
□ Background fully transparent (no halo artifacts)
□ Consistent palette across all frames of this character
□ Proportions consistent between all 8 directions
□ Animation loops seamlessly (last frame → first frame)
□ No floating pixels or disconnected artifacts
□ Outline consistent thickness (1px everywhere)
□ Anchor points defined for all frames
□ Sprite centered correctly in frame bounds
□ File naming follows convention: [class]_[gender]_[action]_[dir]_[frame].png
```

---

## 11. Phase 9: UE5 Integration

### 11.1 Paper2D System (UE5.7)

Paper2D is **still supported** in UE5.7 and provides:
- Sprite importing from texture atlases
- Flipbook animations (frame-by-frame playback)
- Integration with 3D world (sprites in 3D space)

**Import Workflow**:
```
1. Import sprite sheet PNG into UE5 Content Browser
2. Right-click → Sprite Actions → "Extract Sprites"
   - Or use "Create Flipbook" for automatic animation setup
3. Set Sprite → Texture Settings:
   - Filter: Nearest (NO bilinear filtering — preserves pixel art crispness)
   - Compression: UserInterface2D (lossless)
   - sRGB: ON
   - Mip Gen Settings: NoMipmaps (critical for pixel art)
4. Create Flipbook asset for each animation
5. Set frame rate: 8-12 FPS
```

### 11.2 PaperZD Plugin (Recommended)

PaperZD provides a full animation Blueprint system for 2D:

**Installation**: Available on Fab (Unreal Marketplace), free

**Setup**:
```
1. Enable PaperZD plugin in Project Settings → Plugins
2. Create PaperZDCharacter Blueprint (replaces standard Character)
3. Add PaperZDAnimationComponent
4. Create PaperZDAnimBP (animation state machine)
5. Create Animation Sources (Flipbook-based)
6. Set up state transitions: Idle → Walk → Attack → etc.
```

**Key PaperZD Features for RO-style game**:
- **Directional support** built-in (top-down, isometric modes)
- **Animation state machine** with transitions and blend
- **Multi-source** — can drive Flipbooks, Spine, or custom renderers
- **Blueprint-friendly** — full BP API for controlling animations

### 11.3 Billboard Rendering (2D Sprites in 3D World)

To make 2D sprites look correct in the 3D world (like RO):

**Option A: PaperZD Billboard**
```
PaperZDCharacter Blueprint:
1. Set FlipbookComponent rotation to always face camera
2. Tick Event:
   - Get Camera Forward Vector
   - Calculate angle between camera and character facing direction
   - Select appropriate direction sprite set (8-direction)
   - Play correct directional flipbook

Direction Selection Logic:
  angle = atan2(cameraDir) - atan2(characterDir)
  normalized = (angle + 360) % 360
  directionIndex = round(normalized / 45) % 8
  Play flipbook for directionIndex
```

**Option B: Custom Material Billboard**
```
Material setup for camera-facing sprites:
1. World Position Offset node
2. Camera Vector → calculate rotation to face camera
3. Apply rotation only on vertical axis (Y-axis)
4. Sprite always faces camera but doesn't tilt

Nodes:
- CameraPositionWS → subtract ActorPositionWS → normalize
- atan2(X, Y) → rotation angle
- Apply to sprite mesh rotation via Material
```

**Option C: Using the existing 3D character pawn with sprite rendering**
```
Since Sabri_MMO already has BP_MMOCharacter with 3D:
1. Add a FlipbookComponent to the existing character Blueprint
2. Hide the 3D mesh, show the flipbook
3. Or: transition gradually (3D for close-up, 2D for distant)
```

### 11.4 Direction Calculation for Camera-Relative Sprites

```cpp
// In SabriMMOCharacter.cpp or a custom component:
// Calculate which sprite direction to show based on camera angle

int32 GetSpriteDirection(const FVector& CameraForward, const FVector& CharacterForward)
{
    // Get angle between camera looking direction and character facing
    float CameraAngle = FMath::Atan2(CameraForward.Y, CameraForward.X);
    float CharAngle = FMath::Atan2(CharacterForward.Y, CharacterForward.X);

    float RelativeAngle = FMath::RadiansToDegrees(CameraAngle - CharAngle);
    RelativeAngle = FMath::Fmod(RelativeAngle + 360.0f, 360.0f);

    // Map to 8 directions (0=South, 1=SW, 2=W, 3=NW, 4=N, 5=NE, 6=E, 7=SE)
    int32 Direction = FMath::RoundToInt(RelativeAngle / 45.0f) % 8;
    return Direction;
}
```

### 11.5 Sprite Sheet Data Structure

```cpp
// Proposed C++ struct for sprite management:
USTRUCT(BlueprintType)
struct FSpriteAnimationSet
{
    GENERATED_BODY()

    // 8 flipbooks, one per direction
    UPROPERTY(EditAnywhere)
    TArray<UPaperFlipbook*> DirectionalFlipbooks; // [0]=S, [1]=SW, ... [7]=SE

    // Animation type
    UPROPERTY(EditAnywhere)
    FName AnimationName; // "Idle", "Walk", "Attack", etc.

    // Playback rate
    UPROPERTY(EditAnywhere)
    float FrameRate = 8.0f;

    // Whether this animation loops
    UPROPERTY(EditAnywhere)
    bool bLooping = true;
};
```

---

## 12. Phase 10: Production Workflow & Batch Pipeline

### 12.1 Daily Production Pipeline

```
MORNING SESSION (Character Generation):
1. Open ComfyUI
2. Load workflow: character_sprite_generator.json
3. Set character class + gender in prompt CSV
4. Queue batch: 8 directions × front concept
5. Generation time: ~50 seconds per direction = ~7 minutes total
6. Review outputs, regenerate any bad ones
7. Export to post-processing folder

MIDDAY SESSION (Animation Generation):
1. Open PixelLab (or ComfyUI animation workflow)
2. Import best direction renders as reference
3. Generate animation frames for each action
4. Walk cycle: ~15 minutes including review
5. Attack: ~10 minutes
6. Idle: ~5 minutes
7. Total per direction per animation set: ~30 minutes

AFTERNOON SESSION (Post-Processing):
1. Open Aseprite
2. Import all generated frames
3. Background cleanup (5 min per sprite set)
4. Palette normalization (5 min)
5. Animation timing adjustment (5 min)
6. Export sprite sheets (2 min)
7. Total: ~20 minutes per character direction set

EVENING SESSION (UE5 Integration):
1. Import sprite sheets into UE5
2. Create Flipbooks
3. Test in-game
4. Iterate on any issues
```

### 12.2 Estimated Production Rate

| Task | Time per Unit | Units Needed | Total Time |
|------|--------------|-------------|-----------|
| **Style LoRA training** | 4-10 hours | 1 | 10 hours |
| **Character LoRA training** (per class) | 2-4 hours | 19 | 38-76 hours |
| **Character concept generation** | 1 hour | 38 (19×2 genders) | 38 hours |
| **8-direction generation** (per character) | 2 hours | 38 | 76 hours |
| **Animation generation** (per character) | 4 hours | 38 | 152 hours |
| **Post-processing** (per character) | 2 hours | 38 | 76 hours |
| **Monster archetype generation** | 2 hours | 30 archetypes | 60 hours |
| **Monster palette swaps** | 0.5 hours | 100 variants | 50 hours |
| **Equipment overlays** | 1 hour | 50 equipment types | 50 hours |
| **UE5 integration** | 0.5 hours | per character | 19 hours |
| **Total** | — | — | **~570-630 hours** |

At ~4 productive hours/day, that's roughly **140-160 working days** for complete sprite coverage. This is dramatically less than the **years** it would take for traditional hand-drawn sprites (RO's original art team spent 3+ years).

### 12.3 Batch Automation Scripts

**ComfyUI Batch Generation Script** (Python):
```python
# batch_sprite_gen.py
# Generates all 8 directions for a character class

import json
import os

CLASSES = ["novice", "swordsman", "mage", "archer", "acolyte", "thief", "merchant"]
GENDERS = ["male", "female"]
DIRECTIONS = [
    ("south", "facing camera directly, front view"),
    ("southwest", "three-quarter view facing left"),
    ("west", "profile view facing left"),
    ("northwest", "three-quarter back view facing left"),
    ("north", "facing away from camera, back view"),
    # NE, E, SE mirrored from NW, W, SW
]

PROMPT_TEMPLATE = """pixel art sprite, chibi character, ragnarok online style,
{class_name} class, {gender}, full body, {direction_desc},
simple white background, detailed pixel shading, high contrast,
crisp edges, no anti-aliasing, 32-bit color palette"""

def generate_prompts():
    prompts = []
    for cls in CLASSES:
        for gender in GENDERS:
            for dir_name, dir_desc in DIRECTIONS:
                prompt = PROMPT_TEMPLATE.format(
                    class_name=cls,
                    gender=gender,
                    direction_desc=dir_desc
                )
                prompts.append({
                    "class": cls,
                    "gender": gender,
                    "direction": dir_name,
                    "prompt": prompt,
                    "output_name": f"{cls}_{gender}_{dir_name}"
                })
    return prompts

# Export as CSV for ComfyUI Custom-Scripts batch loading
```

### 12.4 File Naming Convention

```
sprites/
├── characters/
│   ├── novice/
│   │   ├── male/
│   │   │   ├── body/
│   │   │   │   ├── novice_m_idle_s.png      (sprite sheet: idle, south)
│   │   │   │   ├── novice_m_idle_sw.png
│   │   │   │   ├── novice_m_walk_s.png
│   │   │   │   ├── novice_m_attack_s.png
│   │   │   │   └── ...
│   │   │   ├── head/
│   │   │   │   ├── hair01_s.png
│   │   │   │   └── ...
│   │   │   └── metadata/
│   │   │       ├── anchors.json
│   │   │       └── palette.pal
│   │   └── female/
│   │       └── ... (same structure)
│   ├── knight/
│   └── ...
├── monsters/
│   ├── poring/
│   │   ├── poring_idle.png
│   │   ├── poring_walk.png
│   │   ├── poring_attack.png
│   │   ├── poring_death.png
│   │   └── palettes/
│   │       ├── poring.pal         (pink)
│   │       ├── drops.pal          (orange)
│   │       ├── poporing.pal       (green)
│   │       └── marin.pal          (blue)
│   └── ...
├── equipment/
│   ├── headgear/
│   ├── weapons/
│   ├── shields/
│   └── garments/
└── effects/
    ├── cast_circles/
    ├── hit_effects/
    └── skill_effects/
```

---

## 13. Tool Comparison Matrix

### 13.1 AI Generation Tools

| Tool | Local/Cloud | Quality | Consistency | Multi-Dir | Animation | Price |
|------|-----------|---------|-------------|-----------|-----------|-------|
| **ComfyUI + Flux** | Local (RTX 5090) | 9/10 | 7/10 (with LoRA+IP-Adapter) | Manual | Manual | Free |
| **ComfyUI + SDXL** | Local | 8/10 | 7/10 | Manual | Manual | Free |
| **PixelLab** | Cloud | 9/10 | 9/10 | Built-in 8-dir | Built-in | $9-22/mo |
| **AutoSprite** | Cloud | 7/10 | 8/10 | Built-in | Built-in | Freemium |
| **SEELE** | Cloud | 7/10 | 8/10 (98% claim) | Combined gen | Auto | Freemium |
| **Ludo.ai** | Cloud | 7/10 | 7/10 | Basic | Basic | Freemium |
| **God Mode AI** | Cloud | 8/10 | 8/10 | 8-dir | Spine export | $12-100 credits |
| **Scenario** | Cloud | 8/10 | 8/10 | Via tools | Seedance | Paid |

### 13.2 Post-Processing Tools

| Tool | Purpose | Price | Platform |
|------|---------|-------|----------|
| **Aseprite** | Sprite editing, palette, animation | $20 (one-time) | All |
| **TexturePacker** | Sprite sheet packing, trimming | $40 (one-time) | All |
| **Sprite Buff** | Background removal for sprites | Free | Web |
| **ImageMagick** | Batch image processing | Free | CLI |
| **GIMP/Krita** | General image editing | Free | All |

### 13.3 Base Model Comparison for Sprites

| Model | Anime Quality | Pixel Art | LoRA Ecosystem | Speed (RTX 5090) |
|-------|-------------|-----------|---------------|-------------------|
| **Flux.1 Dev** | Excellent | Good (with LoRA) | Growing | ~5.5s/img |
| **SDXL Base** | Good | Good | Largest | ~6.2s/img |
| **Illustrious XL** | Best | Excellent | Large (anime) | ~6.2s/img |
| **NoobAI 0.5** | Excellent | Excellent | Growing | ~6.2s/img |
| **Pony V7** | Excellent | Good | Largest character LoRAs | ~6.2s/img |
| **SD 1.5** | Decent | Good | Legacy | ~2s/img |

**Recommendation**: Start with **Illustrious XL** (best anime tag comprehension, 100-image tag threshold, excellent pixel art) for SDXL-based workflow, or **Flux.1 Dev** for highest raw quality.

---

## 14. Cost & Time Estimates

### 14.1 One-Time Setup Costs

| Item | Cost | Notes |
|------|------|-------|
| **RTX 5090** | Already owned | 32GB VRAM, ideal for this pipeline |
| **Aseprite** | $20 | One-time purchase (or build from source free) |
| **PaperZD** (UE5 plugin) | Free | Available on Fab |
| **Kohya_ss** | Free | Open source LoRA training |
| **ComfyUI** | Free | Open source |
| **TexturePacker** | $40 | Optional, sprite sheet tool |
| **Total setup** | **~$60** | |

### 14.2 Monthly Operating Costs

| Service | Monthly Cost | Usage |
|---------|-------------|-------|
| **PixelLab Tier 1** | $9/month | Multi-direction + animation generation |
| **Electricity** (RTX 5090 @ 575W) | ~$20-30/month | Heavy generation sessions |
| **Total monthly** | **~$30-40/month** | |

### 14.3 Time Investment

| Phase | Duration | Parallel? |
|-------|----------|-----------|
| **Setup** (ComfyUI, models, tools) | 1-2 days | — |
| **LoRA Training** (style + first class) | 2-3 days | — |
| **First character class (learning)** | 3-5 days | — |
| **Subsequent classes** | 1-2 days each | Can parallelize |
| **Monster archetypes** (30) | 2-3 weeks | After characters |
| **Equipment overlays** | 1-2 weeks | After characters |
| **UE5 integration** | 1-2 weeks | After first sprites |
| **Polish & iteration** | Ongoing | Throughout |

**Realistic timeline**: First playable character sprites in **~2 weeks**. Full character roster in **~2-3 months**. Complete monster set in **~4-5 months**. This is with part-time effort (~4 hours/day).

---

## 15. Sources

### Academic Papers
- [Sprite Sheet Diffusion: Generate Game Character for Animation](https://arxiv.org/abs/2412.03685) — arXiv 2412.03685 (Dec 2024, updated Mar 2025)

### Technical Documentation
- [SPR File Format — Ragnarok Research Lab](https://ragnarokresearchlab.github.io/file-formats/spr/)
- [ACT File Format — rAthena Wiki](https://github.com/rathena/rathena/wiki/Acts)
- [RagnarokFileFormats — GitHub](https://github.com/rdw-archive/RagnarokFileFormats)
- [Paper2D Flipbooks — UE5.7 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/paper-2d-flipbooks-in-unreal-engine)
- [Paper2D Overview — UE5.7 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/paper-2d-overview-in-unreal-engine)
- [PaperZD Documentation — Critical Failure Studio](https://www.criticalfailure-studio.com/paperzd-documentation/)

### Tools & Models
- [ComfyUI GitHub](https://github.com/comfyanonymous/ComfyUI)
- [Kohya_ss LoRA Training](https://github.com/bmaltais/kohya_ss)
- [PixelLab — AI Pixel Art Generator](https://www.pixellab.ai/)
- [RO Sprite LoRA (Illustrious) — Civitai](https://civitai.com/models/1242746/ragnarok-online-sprite-pixel-art-style-illustriousxl-noob-lora)
- [ROSL LoRA (SDXL) — Civitai](https://civitai.com/models/762637/rosl-ragnarok-online-style-lora)
- [Creating RO Style Sprites Study — Civitai](https://civitai.com/articles/8302/creating-ragnarok-online-style-sprites-a-work-in-progress-study)
- [Flux-2D-Game-Assets-LoRA](https://www.promptlayer.com/models/flux-2d-game-assets-lora)
- [Pixel-Art-XL LoRA — HuggingFace](https://huggingface.co/nerijs/pixel-art-xl)
- [ComfyUI-SpriteSheetMaker](https://comfyai.run/documentation/SpriteSheetMaker)
- [zrenderer — RO Sprite Renderer](https://github.com/zhad3/zrenderer)
- [pyro-tools — RO Data Tools](https://github.com/MyGodIsHe/pyro-tools)
- [Sprite Sheet Diffusion — GitHub](https://github.com/chenganhsieh/SpriteSheetDiffusion)
- [God Mode AI — Spine Animation](https://www.godmodeai.co/ai-spine-animation)
- [Retro Diffusion — Aseprite Extension](https://astropulse.itch.io/retrodiffusion)
- [BlenderSpriteGenerator](https://github.com/RubielGames/BlenderSpriteGenerator)
- [ComfyUI-Win-Blackwell (RTX 5090 Setup)](https://blog.comfy.org/p/how-to-get-comfyui-running-on-your)
- [Sprite Buff — Background Remover](https://spritebuff.com/)

### Benchmarks & Performance
- [RTX 5090 SDXL Benchmark in ComfyUI](https://www.databasemart.com/blog/stable-diffusion-benchmark-in-comfyui-on-rtx5090)
- [RTX 5090 vs 4090 vs 3090 Flux Benchmark](https://github.com/comfyanonymous/ComfyUI/discussions/9002)
- [RTX 5090 Tested Against Flux/SD Models](https://github.com/FurkanGozukara/Stable-Diffusion/wiki/RTX-5090-Tested-Against-FLUX-DEV-SD-35-Large-SD-35-Medium-SDXL-SD-15-AMD-9950X-RTX-3090-TI)
- [Best GPU for Stable Diffusion 2026](https://offlinecreator.com/blog/best-gpu-for-stable-diffusion-2026)

### Community & Reviews
- [PixelLab Review — Jonathan Yu](https://www.jonathanyu.xyz/2025/12/31/pixellab-review-the-best-ai-tool-for-2d-pixel-art-games/)
- [SEELE AI Sprite Sheet Guide](https://www.seeles.ai/resources/blogs/how-to-create-sprite-sheets-with-ai)
- [2D Characters in 3D World (UE5 Forum)](https://forums.unrealengine.com/t/how-to-create-2d-characters-in-3d-world-like-ragnarok-online-boomer-shooter/1308742)
- [2D Sprites in 3D World with Paper2D — Epic Tutorial](https://dev.epicgames.com/community/learning/tutorials/wraV/2d-sprites-in-3d-world-with-paper2d)
- [The Spriters Resource — RO Sprites](https://www.spriters-resource.com/pc_computer/ragnarokonline/)
- [Best AI Pixel Art Generators 2026](https://www.sprite-ai.art/blog/best-pixel-art-generators-2026)
- [Illustrious XL vs Pony vs SDXL Comparison — Civitai](https://civitai.com/articles/11668/illustrious-xl-10-comparison-against-other-up-to-date-anime-models)
- [LoRA Training Ultimate Guide 2025](https://sanj.dev/post/lora-training-2025-ultimate-guide)

### Alternative Approaches Evaluated
- [3D Rendered Pixel Sprites — congusbongusgames](https://cxong.github.io/2017/03/3d-rendered-pixel-sprites)
- [RetroRender — 3D to Retro Sprites](https://jettelly.com/blog/turn-your-3d-models-into-retro-sprites-with-retrorender)
- [spine-animation-ai — GitHub](https://github.com/GenielabsOpenSource/spine-animation-ai)
- [Palette Swap Shader Tutorial — Envato](https://gamedevelopment.tutsplus.com/tutorials/how-to-use-a-shader-to-dynamically-swap-a-sprites-colors--cms-25129)

---

## Appendix A: Quick-Start Checklist

```
□ 1. Install ComfyUI with RTX 5090 Blackwell support (PyTorch nightly cu130)
□ 2. Download Illustrious XL or Flux.1 Dev base model
□ 3. Download existing RO style LoRAs from Civitai (test before training custom)
□ 4. Install Aseprite ($20)
□ 5. Sign up for PixelLab ($9/mo Tier 1)
□ 6. Extract RO sprites using zrenderer/pyro-tools for training reference
□ 7. Train custom Style LoRA on RO sprites (overnight, ~4-10 hours)
□ 8. Generate first character concept (front-facing, Novice class)
□ 9. Use PixelLab for 8-direction rotation from concept
□ 10. Generate walk cycle animation (8 frames)
□ 11. Post-process in Aseprite (palette, cleanup, transparency)
□ 12. Import into UE5 as Paper2D Flipbook
□ 13. Test with PaperZD billboard system
□ 14. Iterate and refine workflow
□ 15. Scale to all character classes
```

## Appendix B: Prompt Engineering Templates

### Character Sprite (ComfyUI/SDXL)
```
Positive: "pixel art sprite, chibi character, ragnarok online style,
{CLASS} class, {GENDER}, full body, {DIRECTION} view,
simple white background, {EQUIPMENT}, detailed pixel shading,
high contrast lighting, crisp edges, no anti-aliasing,
32-bit color palette, isometric perspective
<lora:ro_style:0.9> <lora:{class}_char:0.8>"

Negative: "blurry, smooth, realistic proportions, 3D render,
photograph, watermark, text, bad anatomy, extra limbs,
deformed, low quality, jpeg artifacts, signature,
out of frame, cropped, worst quality"
```

### Monster Sprite
```
Positive: "pixel art sprite, ragnarok online monster style,
{MONSTER_NAME}, full body, {DIRECTION} view,
simple white background, detailed pixel shading,
cute but dangerous, fantasy creature, crisp edges,
<lora:ro_style:0.9>"
```

### Equipment Overlay
```
Positive: "pixel art, ragnarok online style equipment,
{ITEM_NAME}, transparent background, isolated item,
game sprite, clean edges, no character, just the item,
<lora:ro_style:0.7>"
```

---

## Appendix C: Deep Research — RO Sprite Action Table (Verified)

From rAthena wiki, Hercules wiki, roBrowser source, and Korangar client source:

### Player Character Actions (13 base × 8 directions = 104 ACT entries)

| Index | Action | Frames/Dir | Looping | Notes |
|-------|--------|-----------|---------|-------|
| 0 | Idle (Standing) | 1-5 | Yes | Frame 0 displayed; "standby" is separate |
| 1 | Walk | 8 | Yes | Max framerate = 8 frames/direction |
| 2 | Sit | 1-3 | Yes | |
| 3 | Pickup (Item) | 2-4 | No | |
| 4 | ReadyFight (Combat Idle) | 3-6 | Yes | |
| 5 | Attack1 (Primary) | 4-8 | No | |
| 6 | Hurt (Damage) | 2 | No | |
| 7 | Freeze1 (Status) | 1 | Yes | |
| 8 | Die (Death) | 1-3 | No | |
| 9 | Freeze2 (Status Alt) | 1 | Yes | |
| 10 | Attack2 (Secondary) | 4-8 | No | |
| 11 | Attack3 (Ranged) | 4-8 | No | |
| 12 | Skill (Casting) | 4-7 | No | |

**Total per class body**: ~100-160 unique sprite frames in SPR file.

### Monster Actions (5 base × 8 directions = 40 ACT entries)
Idle (1-4f), Walk (4-8f), Attack (4-8f), Hurt (1-2f), Die (2-4f)

### Entity Direction Counts
- **Players**: 8 directions for all actions
- **Monsters**: Typically 4 directions (other 4 are horizontally mirrored)
- **NPCs**: 1-2 directions (static)
- **Homunculus**: 8 base actions × 8 directions = 64 ACT entries
- **Pets**: 9 base actions × 8 directions = 72 ACT entries

### Animation Timing
- Frame times are in **ticks** (1 tick = 24ms)
- AnimationSpeed=4.0 means 100ms/frame (10 FPS)
- All binary data is **little-endian**

---

## Appendix D: Deep Research — ComfyUI Workflow Repositories

Key ComfyUI repos specifically for sprite generation, verified by agent research:

### Best Complete Workflows

| Repository | Stars | Approach |
|------------|-------|----------|
| **AHEKOT/ComfyUI_VNCCS** | 833 | Full character consistency pipeline (5-stage) |
| **Tidwell32/comfyui-sprite-generator** | — | 4-stage: Qwen pose transfer → cleanup → pixel LoRA → downscale |
| **P5ina/comfyui-triposr-simple** | — | 2D→3D mesh→8-direction renders (TripoSR) |

### Critical Node: LayerDiffuse
`huchenlei/ComfyUI-layerdiffuse` — Generates images with **native transparent backgrounds** in RGBA. No post-processing BG removal needed. Avoids edge halo artifacts. SDXL/SD1.5 only (no Flux). Dimensions must be multiples of 64.

### Critical Node: Pixel Snapper
`x0x0b/ComfyUI-spritefusion-pixel-snapper` — Fixes AI pixel art by grid-snapping pixels and quantizing to `k_colors` (default 16). Eliminates anti-aliased fuzzy edges that diffusion models produce. **Essential post-processing step.**

### TripoSR 8-Direction Pipeline
`P5ina/comfyui-triposr-simple` converts a single 2D image → 3D mesh → renders from 8 directions automatically. Guarantees geometric consistency. Has optional pixelation pass. **Fastest path to 8-directional sprites from one reference image.**

### Full Recommended Node Pipeline
```
IP-Adapter (reference, weight 0.8)
  + ControlNet DWPose (direction skeleton)
  + LayerDiffuse (native transparency)
  → KSampler
  → Pixel Snapper (grid snap + palette quantize)
  → SpriteSheetMaker (grid assembly)
  → Export PNG
```

---

## Appendix E: Deep Research — Existing RO LoRAs (Verified Downloads)

| Model | Base | Dataset | Rating | Downloads | Trigger |
|-------|------|---------|--------|-----------|---------|
| **ROSPRITE v2.1C** | Illustrious | 190+ imgs, 10 epochs | 5/5 (709 reviews) | ~5K | `ROSPRITE` |
| **RO Sprite NoobAI** | NoobAI 0.5 | 80 imgs, 16 epochs | 5/5 (367 reviews) | ~2K | `pixel, full body` |
| **ROSL** | Pony Diffusion | Unknown | 5/5 (32 reviews) | 240 | `Rochar` |
| **ROSSv2** | SD 1.5 | 600+ imgs | — | 226 | — |

**Key finding**: The ROSPRITE v2.1C LoRA (Civitai model 1043663) has 48,854 generations and 5/5 rating. You can start using this immediately without training your own — it captures the RO sprite style well. Only train custom LoRAs if you need a distinct art direction.

**LoRA stacking for best results**: `RO_Style_LoRA (0.7-0.9)` + `body_fix LoRA` + `colorful_anime LoRA` + `block_effect LoRA` (for pixel crispness). Use **EasyNegative** embedding — "crucial for good results."

---

## Appendix F: Deep Research — UE5 Performance at MMO Scale

### Paper2D Draw Call Benchmark (Worst Case)

| Engine | 5,000 sprites | Draw Calls | FPS | Memory |
|--------|--------------|-----------|-----|--------|
| UE4 Paper2D | 5,000 | 5,001 | 24 | 556 MB |
| Unity (batched) | 5,000 | 137 | 234 | 45 MB |

### Why This Is Fine for Sabri_MMO
- RO typically shows **20-100 characters** on screen, not thousands
- Each character = 1-2 draw calls → 100 characters = ~200 draw calls
- Modern GPUs handle 2,000-5,000 draw calls at 60 FPS easily
- Main bottleneck shifts to network/server with sprite-based rendering

### Workarounds for Scale
1. **UE5.5+ Auto-Batching**: Static meshes sharing material auto-instance. Use billboard plane mesh approach.
2. **HISM**: For distant characters — 50,000 draw calls → under 4ms render time
3. **Niagara GPU Sprites**: For background crowds — millions of flipbook particles
4. **Hybrid**: Paper2D for nearby (~50 actors), ISM planes for mid-distance, Niagara for far crowd

### HD-2D Reference Games (UE4/5)
- Octopath Traveler I/II, Triangle Strategy, Dragon Quest III HD-2D Remake, Live A Live Remake
- All use 2D pixel sprites in full 3D environments with modern lighting
- **Spaceman Memories** — solo dev UE5 HD-2D JRPG (Paper2D + Nanite + Lumen)

---

## Appendix G: Deep Research — AI Sprite Animation Tool Comparison (Detailed)

### Ludo.ai (Best Feature Set)
- **Max**: 64 frames/sheet, 512x512/frame, 3 seconds duration
- **5 perspectives**: Side, Hero, Isometric, Tactical, Top-Down × 8 directions = 40 variants
- **Motion Transfer**: Apply reference video motion to static sprites (unique feature)
- **Pricing**: Indie $15/mo (3K credits/yr), Pro $35/mo (12K credits/yr)
- Animation costs 5 credits per generation

### PixelLab (Best for Pixel Art Specifically)
- **Max**: 256x256 for characters, auto-animation limited to 64x64
- **Frame output**: 32-64px → 16 frames/gen, 128px → 4 frames/gen
- **Fixed Head feature**: Keeps head consistent across animation frames
- **Animation-to-animation transfer**: Reuse skeleton from one character for another

### Scenario.com (Best for Spine Rigging)
- **2D Animation Rigging Sheet app**: Auto-separates character into Spine-ready body parts
- **Custom model training**: Train LoRA in ~30 minutes, combine up to 5 LoRAs
- **Pricing**: Starter $15/mo (1,500 credits), Pro $45/mo (5,000 credits)

### AutoSprite (Too Early Stage)
- Only itch.io rating: 1.0/5 (single review). Launched ~80 days ago.
- **Not recommended for production** — wait for maturity.

### God Mode AI (Cannot Verify)
- Domain `godmodeai.com` redirects to domain parking. May not exist as described.
- **Do not rely on this tool.**

### Frame Interpolation (RIFE)
- Best model: Practical-RIFE v4.25, anime-optimized variants v4.7-4.10
- PSNR 35.615, SSIM 0.9779
- **Caveat**: Can produce ghosting on pixel art. Works better on smooth 2D art.
- Use as supplementary tool, not primary animation pipeline.

### Spine ($379 Professional)
- Industry standard for skeletal 2D animation
- Works with any PNG body parts (AI-generated or hand-drawn)
- 19 game toolkits, 7 languages, 40+ runtimes including UE5
- **Not ideal for pixel art** — smooth interpolation conflicts with pixel-perfect positioning
- Best if you decide to go smooth 2D art instead of pixel art

---

## Appendix H: Key Decision — Pixel Art vs Smooth 2D Art

Based on all research, you have two viable art direction choices:

### Option A: Classic Pixel Art (RO Authentic)
- **Resolution**: 64-128px per character frame
- **Tools**: ComfyUI + Illustrious XL + ROSPRITE LoRA → PixelLab → Aseprite
- **UE5**: Paper2D Flipbooks, Nearest-neighbor filtering, no mipmaps
- **Pros**: Authentic RO feel, smaller file sizes, palette swap system works perfectly
- **Cons**: AI generation at small sizes can be inconsistent, more manual cleanup

### Option B: HD-2D Style (Modern Take)
- **Resolution**: 256-512px per character frame
- **Tools**: ComfyUI + Illustrious XL + style LoRA → Ludo.ai/Scenario → Spine
- **UE5**: Paper2D or Spine runtime, can use lit materials for dynamic lighting
- **Pros**: Higher quality per-frame, better AI consistency at larger sizes, dynamic lighting
- **Cons**: Larger file sizes, more VRAM per character, loses some RO nostalgia

**Recommendation**: Start with **Option A** (pixel art) since it matches your game's RO inspiration. The ROSPRITE LoRA and PixelLab are specifically built for this. You can always upscale later if you change direction.
