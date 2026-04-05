# Detailed Implementation Guide — 2D Sprite Art Pipeline for Sabri_MMO

This is the companion document to `2D_Sprite_Art_Pipeline_Plan.md`. It contains detailed step-by-step walkthroughs, expanded recommendations, pitfall warnings, and implementation-level specifics for every phase of the pipeline.

---

## Table of Contents

1. [Pipeline Decision Matrix — Which Approach Is Best For You](#1-pipeline-decision-matrix)
2. [Detailed Phase 1: ComfyUI Setup Walkthrough](#2-detailed-phase-1-comfyui-setup)
3. [Detailed Phase 2: LoRA Training Walkthrough](#3-detailed-phase-2-lora-training)
4. [Detailed Phase 3: Sprite Generation Walkthrough](#4-detailed-phase-3-sprite-generation)
5. [Detailed Phase 4: Multi-Direction Generation](#5-detailed-phase-4-multi-direction)
6. [Detailed Phase 5: Animation Production](#6-detailed-phase-5-animation)
7. [Detailed Phase 6: Monster Sprite Production](#7-detailed-phase-6-monsters)
8. [Detailed Phase 7: Equipment Layering System](#8-detailed-phase-7-equipment-layering)
9. [Detailed Phase 8: Post-Processing & Cleanup](#9-detailed-phase-8-post-processing)
10. [Detailed Phase 9: UE5 Integration Code](#10-detailed-phase-9-ue5-integration)
11. [Common Pitfalls & How to Avoid Them](#11-common-pitfalls)
12. [Alternative Pipeline: 3D-to-2D Pre-Rendered Sprites](#12-alternative-3d-to-2d-pipeline)
13. [Legal & Copyright Considerations](#13-legal-copyright)
14. [Downloadable Resources & Links](#14-downloadable-resources)
15. [My Recommendations — Final Verdict](#15-final-recommendations)

---

## 1. Pipeline Decision Matrix

Based on 35+ research iterations, here are the three viable pipelines ranked by quality, effort, and cost:

### Pipeline A: AI 2D Generation (Recommended)

```
ComfyUI (local) → PixelLab (rotation/anim) → Aseprite (polish) → UE5 Paper2D
```

| Factor | Rating | Notes |
|--------|--------|-------|
| **Quality** | 8/10 | Authentic pixel art feel, good for RO style |
| **Consistency** | 7/10 | Requires LoRA + IP-Adapter discipline |
| **Speed** | 9/10 | Fastest path to playable sprites |
| **Cost** | 9/10 | ~$60 setup + $30/mo |
| **Control** | 7/10 | Prompt-based, some randomness |
| **Learning Curve** | Medium | ComfyUI has a steep start, but workflows are reusable |
| **Best For** | Classic pixel art RO feel, fast iteration |

### Pipeline B: 3D-to-2D Pre-Render

```
AI 3D Gen (Meshy/Tripo) → Blender (rig/animate) → Render to Sprites → Aseprite → UE5
```

| Factor | Rating | Notes |
|--------|--------|-------|
| **Quality** | 7/10 | Clean but can look "3D-ish" unless heavily post-processed |
| **Consistency** | 10/10 | Perfect — same 3D model = same sprite every time |
| **Speed** | 6/10 | 3D rigging/animation is slower |
| **Cost** | 8/10 | Free tools (Blender, Mixamo) + AI 3D gen credits |
| **Control** | 9/10 | Full control over poses, angles, lighting |
| **Learning Curve** | High | Requires Blender knowledge |
| **Best For** | Maximum consistency, large sprite counts, equipment variants |

### Pipeline C: Hybrid (Best of Both)

```
AI 2D Gen (concept) → AI 3D Gen (model from concept) → Blender (animate) → Render → Pixel Snap → UE5
```

| Factor | Rating | Notes |
|--------|--------|-------|
| **Quality** | 9/10 | AI concept art quality + 3D consistency |
| **Consistency** | 9/10 | 3D guarantees alignment, AI adds charm |
| **Speed** | 7/10 | More steps but each is faster than manual |
| **Cost** | 8/10 | Combines both tool sets |
| **Control** | 9/10 | Best control with best visual quality |
| **Learning Curve** | High | Needs both ComfyUI and Blender skills |
| **Best For** | Production quality, long-term maintainability |

### My Recommendation

**Start with Pipeline A** for rapid prototyping and first playable. It gets you sprites in days, not weeks. Then evaluate if you need to move to Pipeline B or C for production polish.

If consistency becomes a major pain point (it likely will around the 5th character class), switch to **Pipeline C** — use ComfyUI to generate concept art, then Tripo3D to convert it to a 3D model, then Blender to render sprite sheets from it. This gives you AI creativity + 3D consistency.

---

## 2. Detailed Phase 1: ComfyUI Setup

### 2.1 Prerequisites

```
Required software:
├── Python 3.11.x (NOT 3.12+ which has compatibility issues)
├── Git for Windows
├── CUDA Toolkit 12.8+ (for RTX 5090 Blackwell sm_120)
├── Visual C++ Build Tools (for some custom nodes)
└── 7-Zip (for model extraction)
```

### 2.2 Step-by-Step Installation (RTX 5090)

```bash
# Step 1: Clone ComfyUI
git clone https://github.com/comfyanonymous/ComfyUI.git
cd ComfyUI

# Step 2: Create virtual environment
python -m venv venv
venv\Scripts\activate

# Step 3: Install PyTorch nightly (CRITICAL for RTX 5090)
# DO NOT use stable PyTorch — it lacks sm_120 kernels for Blackwell
pip install --pre torch torchvision torchaudio --index-url https://download.pytorch.org/whl/nightly/cu130

# Step 4: Install ComfyUI requirements
pip install -r requirements.txt

# Step 5: Optional — Install SageAttention for 15-30% speedup
pip install sageattention

# Step 6: Start ComfyUI
python main.py
# Opens at http://127.0.0.1:8188
```

**CRITICAL RTX 5090 RULES**:
1. **NEVER install xformers** — force-downgrades PyTorch to stable
2. **Strip `torch` from custom node requirements.txt** — prevents pip replacing nightly
3. If ComfyUI crashes on first launch, verify `torch.cuda.is_available()` returns True

### 2.3 Alternative: One-Click Installer

If the manual method fails:
```
1. Download ComfyUI-Win-Blackwell from GitHub
2. Run setup.bat
3. Wait ~20 minutes
4. It auto-configures PyTorch nightly cu130 + SageAttention
```

### 2.4 Install Custom Nodes

Navigate to `ComfyUI/custom_nodes/` and clone each:

```bash
cd custom_nodes

# Essential for sprite pipeline
git clone https://github.com/OSAnimate/ComfyUI-SpriteSheetMaker.git
git clone https://github.com/Fannovel16/comfyui_controlnet_aux.git
git clone https://github.com/cubiq/ComfyUI_IPAdapter_plus.git
git clone https://github.com/huchenlei/ComfyUI-layerdiffuse.git
git clone https://github.com/dimtoneff/ComfyUI-PixelArt-Detector.git
git clone https://github.com/x0x0b/ComfyUI-spritefusion-pixel-snapper.git
git clone https://github.com/ltdrdata/ComfyUI-Impact-Pack.git

# Useful utilities
git clone https://github.com/WASasquatch/was-node-suite-comfyui.git
git clone https://github.com/ninepoin4-ops/comfyui-gametools-2d.git
git clone https://github.com/Kosinkadink/ComfyUI-AnimateDiff-Evolved.git

# Optional — full character consistency pipeline
git clone https://github.com/AHEKOT/ComfyUI_VNCCS.git

# Restart ComfyUI after installing nodes
```

### 2.5 Download Models

Create the following folder structure and download models:

```
ComfyUI/models/
├── checkpoints/
│   ├── illustriousXL_v01.safetensors     # ~6.5 GB from Civitai
│   └── flux1-dev.safetensors             # ~12 GB from HuggingFace
├── loras/
│   ├── ROSPRITE_v2.1C.safetensors        # ~200 MB from Civitai (model 1043663)
│   ├── pixel_art_xl.safetensors          # ~200 MB from HuggingFace (nerijs)
│   └── (your custom trained LoRAs later)
├── controlnet/
│   ├── control_v11p_sd15_openpose.pth    # OpenPose ControlNet
│   └── diffusion_pytorch_model_promax.safetensors  # ControlNet Union SDXL
├── ipadapter/
│   ├── ip-adapter-plus_sdxl_vit-h.safetensors
│   └── CLIP-ViT-H-14-laion2B-s32B-b79K.safetensors
├── clip_vision/
│   └── CLIP-ViT-bigG-14-laion2B-39B-b160k.safetensors
└── vae/
    └── sdxl_vae.safetensors
```

### 2.6 Verify Installation

Run this test workflow:
1. Load Illustrious XL checkpoint
2. Add a KSampler with prompt: `"pixel art sprite, chibi knight, full body, white background"`
3. Set resolution to 768x768, 20 steps, Euler a sampler, CFG 7
4. Generate — should produce a pixel-art-style character in ~6 seconds on RTX 5090

---

## 3. Detailed Phase 2: LoRA Training

### 3.1 Before You Train — Try Existing LoRAs First

**Do not train your own LoRA until you've tested existing ones.** Download from Civitai:

| LoRA | Download | Trigger Word | Weight |
|------|----------|-------------|--------|
| ROSPRITE v2.1C | civitai.com/models/1043663 | `ROSPRITE` | 0.7-1.0 |
| RO Sprite NoobAI | civitai.com/models/1242746 | `pixel, full body` | 0.8-1.0 |

Test these with your base model first. If they produce good results, you may not need to train at all. Only train if:
- You want a unique art direction different from classic RO
- The existing LoRAs don't capture the specific style you need
- You need per-class character LoRAs for extreme consistency

### 3.2 Dataset Extraction from RO Client

```bash
# Step 1: Get the tools
# Download GRF Editor from rAthena board
# Download zrenderer from https://github.com/zhad3/zrenderer

# Step 2: Extract sprites from RO client data.grf
# Open data.grf in GRF Editor
# Navigate to data/sprite/인간족/ (human characters)
# Export .spr and .act files for target classes

# Step 3: Render sprites to PNG using zrenderer
# zrenderer can render individual frames as PNG:
# --frame=-1 renders ALL frames
# --canvas enforces consistent sizing
# --enableUniqueFilenames for organized output

# Example (renders all Knight body sprites):
zrenderer --spr knight_m.spr --act knight_m.act --frame=-1 --canvas 128x128
```

**Alternative: Pre-ripped sprites**
- The Spriters Resource (spriters-resource.com/pc_computer/ragnarokonline/) has pre-extracted sprite sheets
- Download, split into individual frames using Aseprite or ImageMagick

### 3.3 Dataset Preparation Steps

```
Step 1: Collect 50-200 images
  ├── Include multiple classes (Knight, Mage, Archer, etc.)
  ├── Include both genders
  ├── Include various poses (idle, walk, attack)
  ├── Include 2-3 monster sprites for variety
  └── Aim for clean, representative frames

Step 2: Upscale to training resolution
  ├── Use NEAREST-NEIGHBOR ONLY (never bilinear/bicubic)
  ├── Target: 512x512 or 768x768
  ├── Center the character in the frame
  └── Use white or solid color background

Step 3: Create caption files
  ├── Each image needs a matching .txt file
  ├── Same filename: knight_idle_01.png → knight_idle_01.txt
  ├── Format: "ROSPRITE, pixel art, knight class, male, idle pose, sword, armor, white background"
  └── Use WD14 Tagger for auto-captioning, then manually refine

Step 4: Organize into folders
  dataset/
  ├── 03_style/          # 3 repeats, mixed classes for style learning
  │   ├── knight_01.png
  │   ├── knight_01.txt
  │   ├── mage_01.png
  │   ├── mage_01.txt
  │   └── ...
  └── regularization/    # Optional: 200+ images of generic anime for balance
      └── ...
```

### 3.4 Kohya_ss Training — Exact Settings

```bash
# Install kohya_ss
git clone https://github.com/bmaltais/kohya_ss.git
cd kohya_ss
setup.bat

# Launch GUI
gui.bat
```

**Style LoRA Training Settings (Illustrious XL base)**:

```yaml
# Model tab
pretrained_model: illustriousXL_v01.safetensors
model_type: sdxl
mixed_precision: bf16        # Best for RTX 5090 Blackwell

# Folders tab
training_data: path/to/dataset/
output_dir: path/to/output/
logging_dir: path/to/logs/

# Training tab
learning_rate: 0.0005        # UNET LR
text_encoder_lr: 0           # Don't train text encoder for style
lr_scheduler: cosine
lr_warmup_steps: 100
num_cycles: 4                # Cosine restarts
train_batch_size: 2          # RTX 5090 handles 4, but 2 is safer
max_train_epochs: 15         # Monitor for overfitting
save_every_n_epochs: 3       # Save checkpoints to compare

# Network tab
network_type: LoRA
network_dim: 32              # Rank — 32 for style, 16 for character
network_alpha: 16            # Half of dim
conv_dim: 16                 # Optional convolutional LoRA
conv_alpha: 8

# Advanced tab
noise_offset: 0.03           # Matches Illustrious training
min_snr_gamma: 5
clip_skip: 1                 # NOT 2 for Illustrious (differs from Pony)
optimizer: Adafactor          # Memory efficient, good results
resolution: 768              # Training resolution
enable_bucket: true          # Handle non-square images
min_bucket_resolution: 256
max_bucket_resolution: 1024
cache_latents: true
cache_latents_to_disk: true
```

**Expected output**: ~200 MB safetensors file after ~2-4 hours on RTX 5090

### 3.5 Testing Your LoRA

After training, test with progressively complex prompts:

```
# Test 1: Basic (should produce clear RO-style sprite)
"ROSPRITE, pixel art sprite, chibi knight, full body, white background
<lora:your_style_lora:0.8>"

# Test 2: Different class (should maintain style)
"ROSPRITE, pixel art sprite, chibi mage, female, full body, staff, robe
<lora:your_style_lora:0.8>"

# Test 3: With direction (should handle angles)
"ROSPRITE, pixel art sprite, chibi archer, side view, bow, white background
<lora:your_style_lora:0.9>"

# Test 4: Stacked with existing RO LoRA (should combine styles)
"ROSPRITE, pixel art sprite, chibi priest, full body
<lora:your_style_lora:0.6> <lora:ROSPRITE_v2.1C:0.5>"
```

**Quality checklist for LoRA output**:
```
□ Characters are clearly chibi/super-deformed (2-3 heads tall)
□ Pixel edges are crisp (no anti-aliasing blur)
□ Color palette is saturated and bold
□ Outline is consistent (1-2px)
□ Eyes are large, oval, RO-style
□ Proportions are consistent across different prompts
□ Background is clean (not bleeding into character)
```

---

## 4. Detailed Phase 3: Sprite Generation

### 4.1 The Generation-Then-Refine Workflow

This is the core workflow you'll repeat hundreds of times. Master it:

```
GENERATE (ComfyUI, 2 min)
    ↓
REVIEW (Visual check, 30 sec)
    ↓ (good enough?)
REMOVE BG (ComfyUI LayerDiffuse or post-process, 10 sec)
    ↓
PIXEL SNAP (ComfyUI PixelArt-Detector, 10 sec)
    ↓
DOWNSCALE (Nearest-neighbor to target size, 5 sec)
    ↓
IMPORT TO ASEPRITE (Cleanup, palette fix, 5-10 min)
    ↓
EXPORT (PNG sprite sheet, 1 min)
```

### 4.2 ComfyUI Workflow — Node by Node

Here's the exact node graph for generating a single character sprite:

```
Node 1: Load Checkpoint
  └── Model: illustriousXL_v01.safetensors

Node 2: Load LoRA (Style)
  ├── Input: Model from Node 1
  ├── LoRA: ROSPRITE_v2.1C.safetensors
  └── Strength: 0.8

Node 3: CLIP Text Encode (Positive)
  ├── Input: CLIP from Node 2
  └── Text: "ROSPRITE, pixel art sprite, chibi [CLASS], [GENDER],
            full body, [DIRECTION] view, [EQUIPMENT],
            white background, detailed pixel shading,
            crisp edges, no anti-aliasing"

Node 4: CLIP Text Encode (Negative)
  ├── Input: CLIP from Node 2
  └── Text: "blurry, smooth, realistic, 3D render, photograph,
            watermark, text, bad anatomy, extra limbs, deformed,
            low quality, jpeg artifacts, anti-aliasing, gradient"

Node 5: Empty Latent Image
  └── Width: 768, Height: 768, Batch: 1

Node 6: KSampler
  ├── Model: from Node 2
  ├── Positive: from Node 3
  ├── Negative: from Node 4
  ├── Latent: from Node 5
  ├── Seed: (fixed for reproducibility, or random for variety)
  ├── Steps: 20-25
  ├── CFG: 6-8
  ├── Sampler: euler_a
  └── Scheduler: normal

Node 7: VAE Decode
  └── Samples: from Node 6

Node 8: PixelArt Detector (Post-process)
  ├── Image: from Node 7
  ├── Method: Grid.pixelate
  ├── Num Colors: 32 (or 16 for more retro)
  └── Downscale factor: 8 (768→96px character)

Node 9: Save Image
  └── Filename prefix: "[class]_[gender]_[direction]"
```

### 4.3 Adding IP-Adapter for Consistency

When generating multiple directions of the same character:

```
Node 2.5: Load IP-Adapter
  ├── IP-Adapter model: ip-adapter-plus_sdxl_vit-h.safetensors
  ├── CLIP Vision: CLIP-ViT-bigG-14-laion2B
  └── Weight: 0.8

Node 2.6: IP-Adapter Apply
  ├── Model: from Node 2 (after LoRA)
  ├── IP-Adapter: from Node 2.5
  ├── Image: YOUR FRONT-FACING REFERENCE
  └── Weight: 0.7-0.9

(Feed Model output from Node 2.6 into KSampler)
```

**Key insight**: Generate the **front-facing** sprite first. Get it looking exactly right (regenerate until perfect). Then use THAT image as the IP-Adapter reference for all other directions. This locks the character's visual identity.

### 4.4 Adding ControlNet for Pose Control

```
Node 10: Load ControlNet Model
  └── Model: diffusion_pytorch_model_promax.safetensors (Union)

Node 11: DWPose Preprocessor
  └── Input image: YOUR POSE REFERENCE (skeleton image for direction)

Node 12: Apply ControlNet
  ├── Model: from Node 2.6 (after IP-Adapter)
  ├── ControlNet: from Node 10
  ├── Image: from Node 11 (skeleton)
  └── Strength: 0.6-0.8

(Feed into KSampler)
```

**Pre-made pose templates**: Download "Character walking and running animation poses (8 directions)" from Civitai (model 56307). These are OpenPose skeletons for all 8 directions, generated from 3D model rotation.

### 4.5 Using LayerDiffuse for Native Transparency

Instead of generating on white background then removing it:

```
Node 13: LayerDiffuse Apply (before KSampler)
  ├── Model: from LoRA/IP-Adapter chain
  ├── Method: "Generate Transparent Image (SDXL)"
  └── (Dimensions must be multiples of 64)

Node 14: LayerDiffuse Decode (after KSampler)
  └── Outputs: RGBA image with native transparency
```

**Advantage**: No background removal artifacts, no edge halos, clean alpha channel. Only works with SDXL-based models (not Flux).

---

## 5. Detailed Phase 4: Multi-Direction Generation

### 5.1 The 8-Direction Strategy

RO uses 8 facing directions. But you only need to generate **5 unique directions** and mirror the other 3:

```
Generate:        Mirror from:
0 (South)        — (unique)
1 (Southwest)    — (unique)
2 (West)         — (unique)
3 (Northwest)    — (unique)
4 (North)        — (unique)
5 (Northeast)    ← mirror of 3 (NW)
6 (East)         ← mirror of 2 (W)
7 (Southeast)    ← mirror of 1 (SW)
```

**Exception**: If the character holds a weapon in their right hand, mirroring puts the weapon in the left hand. You have two options:
- Accept the mirrored weapon hand (simpler, most players won't notice)
- Generate all 8 directions uniquely (more work, more authentic)

### 5.2 Direction Prompt Templates

```
Direction 0 (South):    "facing camera directly, front view"
Direction 1 (SW):       "three-quarter view, facing bottom-left, slight turn"
Direction 2 (West):     "profile view, facing left, side view"
Direction 3 (NW):       "three-quarter back view, facing upper-left, rear angle"
Direction 4 (North):    "facing away from camera, back view, rear"
```

### 5.3 PixelLab Multi-Direction (Recommended Method)

```
Step 1: Generate your BEST front-facing sprite in ComfyUI (768x768)
Step 2: Open PixelLab (pixellab.ai)
Step 3: Upload the front-facing sprite as reference
Step 4: Select "Directional Rotation" → 8 directions
Step 5: Generate — PixelLab creates all 8 rotations
Step 6: Review each direction, regenerate any that are off
Step 7: Download as individual frames or sprite sheet
Step 8: Import into Aseprite for palette normalization
```

### 5.4 Alternative: TripoSR 8-Direction (Free, Local)

The `comfyui-triposr-simple` node gives you 8 directions from a single 2D image by:
1. Converting your 2D sprite to a 3D mesh (TripoSR)
2. Rendering the mesh from 8 camera angles
3. Optionally pixelating the renders

**Pros**: Free, local, guaranteed geometric consistency
**Cons**: The 3D mesh may lose fine pixel detail, results look more "3D-rendered" than hand-drawn

---

## 6. Detailed Phase 5: Animation Production

### 6.1 Animation Priority Order

Generate animations in this order (most visible first):

```
Priority 1 (MUST HAVE for first playable):
  1. Idle (3-4 frames) — player sees this most
  2. Walk cycle (6-8 frames) — second most visible
  3. Attack melee (6 frames) — core gameplay

Priority 2 (NEEDED for alpha):
  4. Attack ranged (4-6 frames) — for Archer/Mage classes
  5. Cast spell (4-6 frames) — for magic classes
  6. Take damage (2-3 frames) — combat feedback
  7. Death (4-6 frames) — necessary for gameplay

Priority 3 (NICE TO HAVE for beta):
  8. Combat idle/ready (3-6 frames) — weapon drawn pose
  9. Sit (1-2 frames) — social feature
  10. Pick up item (2-3 frames) — inventory interaction
```

### 6.2 Walk Cycle Generation — Step by Step

```
WALK CYCLE (8 frames, per direction):

Frame layout (contact → passing → contact → passing):
  F1: Right foot forward, left foot back (RIGHT CONTACT)
  F2: Right foot passing center (RIGHT PASSING)
  F3: Right foot back, left foot forward, weight shifting
  F4: Left foot fully forward (LEFT CONTACT)
  F5: Left foot passing center (LEFT PASSING)
  F6: Left foot back, right foot forward, weight shifting
  F7: Transition back to start
  F8: Almost at Frame 1 position (LOOP POINT)

For symmetric characters:
  Frames 5-8 can be horizontal mirrors of Frames 1-4
  This halves the generation work

Generation method:
  1. Create 4 pose skeleton references for F1, F2, F4, F5
  2. Use ControlNet DWPose + IP-Adapter for each
  3. Generate frames in order with seed variation (+1 each)
  4. Check loop: F8 → F1 must be seamless
  5. Adjust timing in Aseprite if needed
```

### 6.3 Attack Animation — Step by Step

```
ATTACK MELEE (6 frames):

  F1: Ready stance (weapon pulled back)
  F2: Wind-up (weapon at maximum draw)
  F3: Swing mid-point (weapon crossing center)
  F4: Impact point (weapon fully extended)
  F5: Follow-through (weapon past target)
  F6: Recovery (returning to ready stance)

Key: Frames 2-4 should be fast (short duration)
     Frames 1 and 5-6 can be slower (longer duration)

Timing in Aseprite:
  F1: 150ms (anticipation, slower)
  F2: 80ms  (wind-up, medium)
  F3: 50ms  (swing, FAST)
  F4: 50ms  (impact, FAST)
  F5: 80ms  (follow-through, medium)
  F6: 120ms (recovery, slower)
  Total: 530ms per attack cycle
```

### 6.4 Using PixelLab for Animation

```
Step 1: Upload your idle frame as reference
Step 2: Select "AI Animation"
Step 3: Choose method:
   a) Text prompt: "walking forward, 8 frame loop"
   b) Skeleton: Place joints, keyframe walking motion
   c) Animation transfer: Use another animation's skeleton
Step 4: Set frame count (depends on resolution):
   - 64x64 sprite → 16 frames per generation
   - 128x128 sprite → 4 frames per generation
Step 5: Use "Fixed Head" feature to keep head consistent
Step 6: Generate and review
Step 7: Export frames
```

---

## 7. Detailed Phase 6: Monster Sprite Production

### 7.1 Monster Archetype Strategy

Group the 509 RO monsters into base archetypes to minimize generation work:

```
ARCHETYPE MAP:

Slime/Blob (Poring family):
  Base: Poring → palette swap → Drops, Poporing, Marin, Ghostring, Angeling, Deviling
  Animations: Bounce idle (3f), Hop walk (4f), Slap attack (3f), Squish death (3f)
  Variants: 7+ via palette swap
  Unique frames needed: ~52 (4 dir × 13 frames)

Insect (Rocker, Creamy, Hornet):
  Base: 6-legged body plan
  Animations: Antenna wiggle idle, Scuttle walk, Bite attack, Flip death
  Variants: 5+ via palette/wing swap

Canine/Feline (Lunatic, Savage, Petit):
  Base: 4-legged quadruped
  Animations: Ear twitch idle, Trot walk, Pounce attack, Collapse death
  Variants: 8+ via palette/tail/ear swap

Humanoid (Goblin, Orc, Kobold):
  Base: Humanoid body plan (shorter than player)
  Animations: Fidget idle, March walk, Weapon swing attack, Fall death
  Variants: 10+ via palette/weapon/armor swap

Undead Humanoid (Zombie, Skeleton, Mummy):
  Base: Tattered humanoid
  Animations: Sway idle, Shamble walk, Claw attack, Crumble death
  Variants: 5+ via decay/color swap

Plant (Mandragora, Flora, Geographer):
  Base: Rooted plant body
  Animations: Sway idle, Hop walk, Whip attack, Wilt death
  Variants: 4+ via color/flower swap

Flying (Familiar, Whisper, Alice):
  Base: Hovering body
  Animations: Float idle, Glide walk, Swoop attack, Drop death
  Variants: 6+ via wing/body swap

Boss (Unique per monster):
  Each boss needs fully custom sprites — no shortcuts
  Baphomet, Dark Lord, Eddga, GTB, Drake, etc.
  2-4× player character size
```

### 7.2 Batch Generation Strategy for Monsters

```
For each archetype:
1. Generate high-quality concept (768x768, ComfyUI)
   Prompt: "ROSPRITE, pixel art monster, [MONSTER NAME],
            full body, front view, cute but dangerous,
            fantasy creature, white background"

2. Generate 4 directions (most monsters only need 4, not 8)
   - Front, Side-Left, Back, Side-Right
   - Mirror Side-Left → Side-Right

3. Generate 4 animation sets:
   - Idle: 3 frames
   - Walk: 4 frames
   - Attack: 4 frames
   - Death: 3 frames
   Total: 14 frames × 4 directions = 56 frames per monster

4. For variants, take the base monster's sprite sheet:
   - Open in Aseprite
   - Switch to Indexed Color mode
   - Replace palette → new variant colors
   - Export as new monster
   - Time: 5-10 minutes per variant vs. hours for full generation
```

### 7.3 Monster Size Chart

```
TINY (0.5× player): Poring, Drops, Lunatic, Fabre
  → 48x48 sprite frames

SMALL (0.75× player): Rocker, Thief Bug, Poison Spore
  → 64x72 sprite frames

NORMAL (1× player): Goblin, Orc Warrior, Skeleton
  → 80x96 sprite frames

LARGE (1.5× player): Orc Hero, Drake, Moonlight Flower
  → 120x144 sprite frames

BOSS (2-4× player): Baphomet, Dark Lord, Eddga
  → 192x240+ sprite frames
  → May need to generate at higher resolution and downscale
```

---

## 8. Detailed Phase 7: Equipment Layering System

### 8.1 How RO's Layer System Actually Works

From roBrowser source code and Korangar client analysis:

```
RENDERING ORDER (per frame):

1. Calculate character position in world space
2. Determine facing direction (0-7) from movement + camera
3. Look up current action (idle=0, walk=1, sit=2, etc.)
4. Calculate ACT frame index: action_base × 8 + direction

5. For each layer in order:
   a. Load layer's SPR frame by index
   b. Apply layer's ACT position offset (X, Y)
   c. Apply layer's ACT scale, rotation, color tint
   d. Apply anchor offset (body anchor - head anchor)
   e. Apply mirror flag if needed
   f. Draw to screen buffer

6. Layer order changes based on facing direction:
   - Facing S/SW/W: Shield BEHIND body, weapon IN FRONT
   - Facing N/NE/E: Shield IN FRONT, weapon BEHIND
   - Garment toggles above/below body per-frame via config
```

### 8.2 Implementing Layers in UE5

For Sabri_MMO, implement a multi-FlipbookComponent character:

```cpp
// In your character class header:
UPROPERTY(VisibleAnywhere, Category = "Sprite")
UPaperFlipbookComponent* BodyFlipbook;

UPROPERTY(VisibleAnywhere, Category = "Sprite")
UPaperFlipbookComponent* HeadFlipbook;

UPROPERTY(VisibleAnywhere, Category = "Sprite")
UPaperFlipbookComponent* WeaponFlipbook;

UPROPERTY(VisibleAnywhere, Category = "Sprite")
UPaperFlipbookComponent* ShieldFlipbook;

UPROPERTY(VisibleAnywhere, Category = "Sprite")
UPaperFlipbookComponent* HeadgearFlipbook;

// Each component is a child of the root, positioned via offsets
// That simulate anchor points
```

```cpp
// Direction-dependent layer ordering:
void UpdateLayerOrder(int32 Direction)
{
    // Directions 0-3 (South to Northwest): weapon in front
    // Directions 4-7 (North to Southeast): weapon behind
    if (Direction < 4)
    {
        WeaponFlipbook->SetRelativeLocation(FVector(1.0f, 0, 0)); // slightly forward
        ShieldFlipbook->SetRelativeLocation(FVector(-1.0f, 0, 0)); // slightly behind
    }
    else
    {
        WeaponFlipbook->SetRelativeLocation(FVector(-1.0f, 0, 0));
        ShieldFlipbook->SetRelativeLocation(FVector(1.0f, 0, 0));
    }
}
```

### 8.3 Anchor Point Data Format

Store anchor data in JSON alongside sprite sheets:

```json
{
  "class": "knight",
  "gender": "male",
  "anchors": {
    "idle": {
      "south": [
        { "frame": 0, "head": {"x": 0, "y": -32}, "rhand": {"x": 12, "y": -8}, "lhand": {"x": -12, "y": -8} },
        { "frame": 1, "head": {"x": 0, "y": -31}, "rhand": {"x": 12, "y": -7}, "lhand": {"x": -12, "y": -7} }
      ],
      "southwest": [
        { "frame": 0, "head": {"x": -2, "y": -32}, "rhand": {"x": 14, "y": -6}, "lhand": {"x": -10, "y": -10} }
      ]
    },
    "walk": {
      "south": [...]
    }
  }
}
```

Load these at runtime and apply as offsets to child FlipbookComponents.

### 8.4 Palette Swap Material in UE5

```
UE5 Material Graph for Palette Swap:

[TextureSample: SpriteTexture] → R channel → [Multiply: ×255] →
  → [Floor] → [Divide: ÷255] → UV.x
  → 0.5 → UV.y
  → [TextureSample: PaletteTexture (1×256 strip)]
  → Base Color

[TextureSample: SpriteTexture] → A channel → Opacity Mask

Material Properties:
  - Blend Mode: Masked
  - Shading Model: Unlit
  - Two Sided: True
  - Opacity Mask Clip Value: 0.5

Usage:
  - Create MaterialInstanceDynamic at runtime
  - Set "PaletteTexture" parameter to swap colors
  - One palette strip per equipment tier/color variant
```

---

## 9. Detailed Phase 8: Post-Processing & Cleanup

### 9.1 Aseprite Batch Processing (CLI)

Aseprite has a powerful CLI for batch operations:

```bash
# Batch export all frames as individual PNGs
aseprite -b input.ase --save-as frame_{frame}.png

# Batch export as sprite sheet
aseprite -b input.ase --sheet output_sheet.png --data output_data.json

# Batch resize (nearest-neighbor) — critical for pixel art
aseprite -b input.ase --scale 2 --save-as output_2x.png

# Batch palette swap
aseprite -b input.ase --palette new_palette.pal --save-as recolored.png

# Split layers into separate images
aseprite -b input.ase --split-layers --save-as {layer}.png

# Process entire folder
for f in sprites/*.png; do
  aseprite -b "$f" --scale 2 --save-as "output/$(basename $f)"
done
```

### 9.2 Pixel Art Cleanup Checklist

After AI generation, every sprite needs these checks:

```
STRUCTURAL CHECKS:
□ Character is centered in frame bounds
□ No stray pixels floating outside the character silhouette
□ Outline is consistent thickness (1px everywhere)
□ No anti-aliased edges (should be hard pixel edges only)
□ No "mixels" (pixels of different sizes in one sprite)
□ Proportions match the style guide (2.5 heads tall)

COLOR CHECKS:
□ Colors match the master palette (remap if needed)
□ No gradient banding (use dithering instead)
□ Shadows are a darker shade of the base color (not grey/black)
□ Highlights are a lighter shade (not white)
□ Total unique colors ≤ 32 per sprite (ideally ≤ 16)

ANIMATION CHECKS:
□ All frames in this animation have the same canvas size
□ Character's feet hit the same ground line in all frames
□ Last frame transitions smoothly back to first frame (for loops)
□ Movement feels weighted and natural (not floaty)
□ Key poses are clear even as thumbnails

TRANSPARENCY CHECKS:
□ Background is fully transparent (no halo artifacts)
□ No white/grey fringe around character edges
□ Alpha channel is binary (fully opaque or fully transparent)
```

### 9.3 Palette Normalization Script (Aseprite Lua)

```lua
-- normalize_palette.lua
-- Run in Aseprite: File > Scripts > normalize_palette
-- Forces all sprites in a folder to use the same palette

local master_palette_path = app.fs.joinPath(app.fs.userDocsPath, "master_palette.pal")

-- Load master palette
local master = Palette{ fromFile=master_palette_path }
if not master then
  app.alert("Master palette not found!")
  return
end

-- Apply to current sprite
local sprite = app.activeSprite
if not sprite then
  app.alert("No sprite open!")
  return
end

-- Convert to indexed color using master palette
app.command.ChangePixelFormat{
  format="indexed",
  rgbmap="octree",
  palette=master
}

-- Remap to master palette
sprite:setPalette(master)

app.alert("Palette normalized!")
```

---

## 10. Detailed Phase 9: UE5 Integration

### 10.1 Sprite Import Settings (Critical for Pixel Art)

```
After importing a sprite sheet PNG into UE5:

1. Double-click texture in Content Browser
2. Set these properties:

Texture Group: UI
Compression: UserInterface2D (NO compression)
Filter: Nearest                    ← CRITICAL: prevents bilinear blur
Mip Gen Settings: NoMipmaps       ← CRITICAL: prevents mip blending
sRGB: True
LOD Group: UI
Max Texture Size: 0 (no limit)

3. Apply and save
4. Right-click → Sprite Actions → Extract Sprites
5. Adjust extracted sprite boundaries if needed
6. Select frames → Right-click → Create Flipbook
7. Set Flipbook Frames Per Second: 8-12
```

### 10.2 PaperZD Setup — Step by Step

```
1. Install PaperZD from Fab (free)
   → Enable in Project Settings → Plugins → PaperZD

2. Create PaperZDCharacter Blueprint
   → New Blueprint → Parent: PaperZDCharacter
   → Add FlipbookComponent as child of Capsule

3. Create Animation Source
   → New Asset → PaperZD → Animation Source (Flipbook)
   → Register all Flipbook assets (idle, walk, attack per direction)

4. Create Multi-Directional Sequences
   → In Animation Source, create "Walk" sequence
   → Set Directional Type: 8-Way
   → Assign Flipbooks per direction:
     Direction 0 (Down): FB_Walk_South
     Direction 1 (Down-Left): FB_Walk_SW
     Direction 2 (Left): FB_Walk_West
     etc.

5. Create Animation Blueprint
   → New Asset → PaperZD → Animation Blueprint
   → Source: your Animation Source
   → Build state machine:
     [Idle] ←→ [Walk] ←→ [Attack]
              ↘ [Hurt] → [Death]
   → Transition rules based on velocity, combat state

6. Assign AnimBP to your character Blueprint
   → PaperZDCharacter → Animation Component → Anim Instance: your AnimBP
```

### 10.3 Direction Calculation for Camera-Relative Sprites

```cpp
// Add to your character class .cpp:

int32 ASabriMMOSpriteCharacter::CalculateSpriteDirection() const
{
    if (!GEngine || !GetWorld()) return 0;

    // Get camera forward (projected to XY plane)
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return 0;

    FVector CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
    CamForward.Z = 0;
    CamForward.Normalize();

    // Get character movement direction (or facing direction)
    FVector CharForward = GetActorForwardVector();
    CharForward.Z = 0;
    CharForward.Normalize();

    // Calculate relative angle
    float CamAngle = FMath::Atan2(CamForward.Y, CamForward.X);
    float CharAngle = FMath::Atan2(CharForward.Y, CharForward.X);
    float RelAngle = FMath::RadiansToDegrees(CamAngle - CharAngle);

    // Normalize to 0-360
    RelAngle = FMath::Fmod(RelAngle + 360.0f, 360.0f);

    // Map to 8 directions
    // 0=S, 1=SW, 2=W, 3=NW, 4=N, 5=NE, 6=E, 7=SE
    int32 Dir = FMath::RoundToInt(RelAngle / 45.0f) % 8;
    return Dir;
}

// In Tick:
void ASabriMMOSpriteCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update sprite direction
    int32 NewDir = CalculateSpriteDirection();
    if (NewDir != CurrentSpriteDirection)
    {
        CurrentSpriteDirection = NewDir;
        // PaperZD handles direction changes automatically
        // if using Multi-Directional Sequences
        AnimComponent->SetDirectionalAngle(CurrentSpriteDirection * 45.0f);
    }

    // Billboard: rotate sprite to face camera
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - GetActorLocation();
        ToCamera.Z = 0;
        FRotator LookRot = ToCamera.Rotation();
        BodyFlipbook->SetWorldRotation(FRotator(0, LookRot.Yaw + 90, 0));
    }
}
```

---

## 11. Common Pitfalls & How to Avoid Them

### 11.1 AI Generation Pitfalls

| Pitfall | Why It Happens | Solution |
|---------|----------------|----------|
| **Anti-aliased edges** | Diffusion models naturally produce smooth gradients | Use PixelArt-Detector node to snap to grid + quantize palette |
| **Inconsistent proportions** | Each generation is independent | Use IP-Adapter with reference image at 0.7-0.9 weight |
| **Wrong number of limbs** | Common SDXL artifact on small figures | Add "one head, two arms, two legs" to prompt. Regenerate bad results |
| **Style drift across classes** | Different prompts produce different styles | Use same Style LoRA + same CFG + same sampler for ALL classes |
| **Broken animation loops** | AI doesn't understand temporal continuity | Generate keyframes only, interpolate in Aseprite manually |
| **Background bleeding** | Residual background colors in edges | Use LayerDiffuse for native transparency. Never generate on colored BG |
| **"Mixels"** | Pixels of different sizes mixed together | Post-process with Pixel Snapper node (grid snap is mandatory) |
| **Face/detail loss at small size** | Downscaling from 768px to 96px | Generate at 2-4× target size, use nearest-neighbor downscale |

### 11.2 UE5 Integration Pitfalls

| Pitfall | Why It Happens | Solution |
|---------|----------------|----------|
| **Blurry sprites** | Bilinear filtering is default | Set Filter: Nearest on ALL sprite textures |
| **Mip shimmer** | Mipmaps blend pixels at distance | Set Mip Gen Settings: NoMipmaps |
| **Compression artifacts** | Default compression is lossy | Set Compression: UserInterface2D |
| **Z-fighting** | Multiple sprite layers at same depth | Offset each layer by 0.1-1.0 units in depth |
| **Sorting errors** | Translucent sprites don't sort correctly | Use Masked blend mode, not Translucent |
| **Flash/pop on direction change** | Abrupt sprite swap | Add 1-frame blend or just accept it (RO does this too) |

### 11.3 Workflow Pitfalls

| Pitfall | Why It Happens | Solution |
|---------|----------------|----------|
| **Spending too long on one sprite** | Perfectionism | Set a time limit: 30 min per character direction. Move on. |
| **Not saving seeds** | Can't reproduce good results | ALWAYS log seed + prompt + settings for keepers |
| **Training LoRA too early** | Existing LoRAs might be good enough | Test ROSPRITE v2.1C first before training |
| **Generating at target resolution** | Low-res = low quality from AI | Generate at 4-8× target, downscale with nearest-neighbor |
| **Not normalizing palettes** | Visual inconsistency across sprites | Define master palette first, remap ALL sprites to it |

---

## 12. Alternative Pipeline: 3D-to-2D Pre-Rendered Sprites

### 12.1 When to Use This Instead

Choose the 3D pipeline if:
- Character consistency is your #1 priority
- You need hundreds of equipment variants
- You want easy animation (animate once, render from all angles)
- You're comfortable with Blender
- You want to add new animations later without regenerating everything

### 12.2 Step-by-Step 3D-to-2D Workflow

```
Step 1: Generate 3D character model
  ├── Option A: Tripo3D (text/image → 3D mesh, ~30 sec)
  ├── Option B: Meshy AI (similar, better textures)
  ├── Option C: Hand-model in Blender (maximum control)
  └── Option D: Use MakeHuman/CharacterCreator for base

Step 2: Rig the model
  ├── Option A: Mixamo auto-rig (free, supports humanoids)
  ├── Option B: Blender auto-rig (Rigify addon)
  └── Option C: Manual rigging (maximum control)

Step 3: Animate
  ├── Option A: Mixamo pre-made animations (1000+ free animations)
  │   → Download as FBX, import to Blender
  ├── Option B: Blender keyframe animation
  └── Option C: Motion capture (if available)

Step 4: Set up render camera
  ├── Orthographic camera
  ├── 45-degree angle (matching RO perspective)
  ├── 8 camera positions around the model (45° apart)
  └── Background: transparent (RGBA film setting)

Step 5: Render all directions × all animations
  ├── Use Blender's Python API to automate:
  │   for direction in range(8):
  │     rotate_camera(direction * 45)
  │     for frame in animation_frames:
  │       render_frame(frame, direction)
  └── Output: individual PNG frames per direction per frame

Step 6: Post-process for pixel art look
  ├── Downscale with nearest-neighbor to target sprite size
  ├── Apply palette quantization (16-32 colors)
  ├── Optional: apply Blender toon/cel shader BEFORE rendering
  ├── Add pixel-art outlines in Aseprite
  └── Normalize to master palette

Step 7: Assemble sprite sheets
  └── Use Aseprite or TexturePacker to create final sheets
```

### 12.3 Blender Sprite Sheet Addons

| Addon | URL | Features |
|-------|-----|----------|
| **BlenderSpriteGenerator** | github.com/RubielGames/BlenderSpriteGenerator | Multi-angle renders, 2D/2.5D sprites |
| **Game-Sprite-Creator** | github.com/johnferley/Game-Sprite-Creator | Top-down + isometric presets, auto sprite sheets |
| **Pre Render Creator** | viniguerrero.itch.io/pre-render-creator | One-click pre-render pipeline |
| **Spritely** | github.com/peardox/Spritely | Import 3D model, select poses, auto-generate |

### 12.4 Cel-Shade Setup in Blender for Pixel Art Look

```
Blender Material Setup:

1. Shader to RGB node (converts smooth shading to stepped)
2. ColorRamp node (3-4 color stops for cel-shading)
   → Constant interpolation (hard steps, no blending)
   → Dark: shadow color
   → Mid: base color
   → Light: highlight color
3. Freestyle outline (1-2px black outline)
4. Render at 4× target resolution
5. Downscale with nearest-neighbor in Aseprite

Result: 3D model rendered with flat colors + outline
        that looks like hand-drawn pixel art when downscaled
```

---

## 13. Legal & Copyright Considerations

### 13.1 Current Legal Landscape (as of 2026)

```
KEY FACTS:

1. AI-generated images are NOT copyrightable in the US
   → The US Copyright Office requires "human authorship"
   → Pure AI output (simple prompt → image) has no copyright

2. HOWEVER: Human-modified AI art CAN be copyrighted
   → If you significantly modify AI output (edit in Aseprite,
     add details, adjust colors, composite layers), the
     modifications are copyrightable
   → The more human effort involved, the stronger the claim

3. Training data concerns:
   → Training LoRAs on copyrighted RO sprites is legally grey
   → Gravity Co. owns the RO art — using their sprites as
     training data could be challenged
   → Safer: train on your OWN art or permissively licensed art

4. Steam disclosure:
   → ~8,000 games on Steam disclose AI art usage (as of July 2025)
   → Steam requires disclosure but does not prohibit AI art
```

### 13.2 Recommended Approach for Sabri_MMO

```
SAFE PATH:

1. Use the existing ROSPRITE LoRA from Civitai (trained by others)
   → You didn't train it, so training data isn't your concern
   → The LoRA captures a "style," not specific copyrighted images

2. Generate sprites that are INSPIRED BY RO, not copies
   → Create your own character designs
   → Use RO as style reference, not content reference

3. Post-process heavily in Aseprite
   → This adds human authorship
   → Edit colors, fix proportions, add details manually
   → Your modifications ARE copyrightable

4. Disclose AI usage
   → On Steam, in your game's description
   → "Art created with AI assistance and manual pixel art editing"

5. Consider training on YOUR OWN art eventually
   → Draw 50-100 sprites by hand (or heavily edit AI output)
   → Train a LoRA on YOUR art
   → All future generations are based on your copyrightable work
```

---

## 14. Downloadable Resources & Links

### 14.1 Pre-Made ControlNet Pose Templates

| Resource | URL | What It Is |
|----------|-----|-----------|
| 8-Dir Walking/Running Poses | civitai.com/models/56307 | OpenPose skeletons for 8 directions, 4 animation types |
| OpenPose Character Sheet | gist.github.com/chewtoys/a1b26844975531c620bf47a7342d8bf8 | Character concept sheet ControlNet template |
| OpenPoses.com | openposes.com | Library of reusable OpenPose skeletons |

### 14.2 ComfyUI Workflow Files

| Workflow | URL | What It Does |
|----------|-----|-------------|
| Sprite Sheet Maker | civitai.com/models/448101 | 40-sprite sheet (8×5 grid), 128×128 each |
| Pixel Art Workflow | openart.ai/workflows/megaaziib/pixel-art-workflow | Clean pixel art generation |
| SDXL Pixel Art | gist.github.com/zaro/9243d32d56f81655fdf9e3edd48f4ed1 | SDXL-specific pixel art JSON |

### 14.3 LoRA Models to Download

| Model | URL | Base | Quality |
|-------|-----|------|---------|
| ROSPRITE v2.1C | civitai.com/models/1043663 | Illustrious | Best RO style (5/5, 48K gens) |
| RO Sprite NoobAI | civitai.com/models/1242746 | NoobAI 0.5 | Alternative RO style |
| Pixel Art XL | civitai.com/models/120096 | SDXL | General pixel art |
| Pixel Art SDXL (nerijs) | huggingface.co/nerijs/pixel-art-xl | SDXL | Clean pixel art |

### 14.4 Tool Downloads

| Tool | URL | Price |
|------|-----|-------|
| ComfyUI | github.com/comfyanonymous/ComfyUI | Free |
| Kohya_ss | github.com/bmaltais/kohya_ss | Free |
| Aseprite | store.steampowered.com/app/431730/Aseprite | $20 |
| PaperZD (UE5) | fab.com/listings/6664e3b5... | Free |
| Retro Diffusion (Aseprite plugin) | astropulse.itch.io/retrodiffusion | $65 (or Lite $20) |
| TexturePacker | codeandweb.com/texturepacker | $40 |
| zrenderer (RO sprite tool) | github.com/zhad3/zrenderer | Free |

---

## 15. Final Recommendations — Verdict

### 15.1 What I Recommend You Do (Priority Order)

```
WEEK 1: Setup & Testing
  Day 1-2: Install ComfyUI with RTX 5090 support
  Day 2-3: Download Illustrious XL + ROSPRITE LoRA
  Day 3-4: Generate 20 test sprites, evaluate quality
  Day 4-5: Install Aseprite, practice post-processing
  Day 5-7: Generate first complete character (Novice, front-facing)

WEEK 2: First Character Complete
  Day 1-2: Sign up for PixelLab, generate 8 directions
  Day 2-3: Generate walk cycle (8 frames × 5 directions)
  Day 3-4: Generate attack + idle animations
  Day 4-5: Post-process everything in Aseprite
  Day 5-7: Import into UE5, set up Paper2D + PaperZD

WEEK 3-4: Production Pipeline
  Day 1-3: Train custom Style LoRA if needed (or keep using ROSPRITE)
  Day 3-7: Generate 2-3 more character classes per week
  Ongoing: Develop batch automation scripts

MONTH 2-3: Scale Up
  Week 1-2: Complete all 7 first-job classes
  Week 2-3: Generate monster archetypes (top 30)
  Week 3-4: Create palette swap variants (100+ monsters)

MONTH 3-5: Polish & Equipment
  Week 1-2: Generate equipment overlay sprites
  Week 2-3: Implement palette swap material in UE5
  Week 3-4: Second-job classes (12 classes)
  Ongoing: Iterate and polish

MONTH 5+: Production
  → Complete all 19 classes with all animations
  → Complete all 509 monsters (using archetypes + palette swaps)
  → Equipment variants via palette swap system
  → Polish pass on all sprites
```

### 15.2 What to Spend Money On

```
ESSENTIAL ($60 one-time):
  ├── Aseprite: $20 (or build from source free)
  └── TexturePacker: $40 (optional, Aseprite can do basic sheets)

RECOMMENDED ($9-22/month):
  └── PixelLab Tier 1: $9/mo (multi-direction + animation)

OPTIONAL ($20-65):
  └── Retro Diffusion: $20 Lite or $65 Full (Aseprite AI plugin)

DO NOT SPEND ON:
  ├── Cloud GPU services (you have an RTX 5090!)
  ├── AI art subscription services for general generation
  ├── Spine ($379) unless you switch to skeletal animation approach
  └── Multiple AI tools — pick one and master it
```

### 15.3 Key Insights from Research

```
1. THE ROSPRITE LORA EXISTS AND IS EXCELLENT
   → 5/5 rating, 48K+ generations, specifically made for RO sprites
   → You can start generating RO-style sprites TODAY without any training
   → This is the single most important finding from this research

2. PIXELLAB IS THE BEST TOOL FOR MULTI-DIRECTION SPRITES
   → Purpose-built for pixel art game sprites
   → 8-direction rotation from a single reference
   → Skeleton-based animation
   → $9/month is trivial compared to time saved

3. ILLUSTRIOUS XL IS THE RIGHT BASE MODEL
   → Best anime tag understanding (100-image tag threshold)
   → Danbooru 2023 training data (20M images)
   → Biggest anime LoRA ecosystem
   → SDXL-based so all consistency tools work (IP-Adapter, ControlNet, LayerDiffuse)

4. PALETTE SWAPS ARE YOUR BEST FRIEND
   → One monster sprite + 5 palettes = 6 monsters
   → One armor sprite + 4 palettes = 4 armor tiers
   → This is exactly how RO did it — it's authentic AND efficient

5. CONSISTENCY IS THE #1 CHALLENGE
   → IP-Adapter + same seed + same LoRA = 80% there
   → Manual cleanup in Aseprite handles the remaining 20%
   → If consistency becomes unbearable, switch to 3D-to-2D pipeline

6. YOUR RTX 5090 IS MASSIVELY OVERKILL FOR THIS
   → 32GB VRAM handles everything with room to spare
   → ~6 seconds per SDXL image, ~5.5 sec per Flux image
   → Can batch 4 images simultaneously
   → LoRA training completes in 2-4 hours
   → You will NOT be bottlenecked by hardware
```

---

## Appendix: Quick Command Reference

### ComfyUI Commands
```bash
# Start ComfyUI
cd ComfyUI && venv\Scripts\activate && python main.py

# Install a custom node
cd custom_nodes && git clone [URL] && cd .. && python main.py
```

### Aseprite CLI
```bash
# Export sprite sheet
aseprite -b sprite.ase --sheet output.png --data output.json

# Batch resize (2×, nearest-neighbor)
aseprite -b input.png --scale 2 --save-as output.png

# Export individual frames
aseprite -b sprite.ase --save-as frame_{frame}.png

# Apply palette
aseprite -b input.png --palette colors.pal --save-as output.png
```

### Kohya_ss Commands
```bash
# Start GUI
cd kohya_ss && gui.bat

# Train from command line (advanced)
accelerate launch train_network.py \
  --pretrained_model_name_or_path="models/illustriousXL_v01.safetensors" \
  --train_data_dir="dataset/" \
  --output_dir="output/" \
  --network_module=networks.lora \
  --network_dim=32 \
  --network_alpha=16 \
  --learning_rate=0.0005 \
  --max_train_epochs=15 \
  --resolution=768 \
  --mixed_precision=bf16
```

### ImageMagick Batch Processing
```bash
# Remove white background from all PNGs
magick mogrify -transparent white sprites/*.png

# Resize all sprites to 96x96 (nearest-neighbor)
magick mogrify -filter point -resize 96x96 sprites/*.png

# Create sprite sheet from individual frames (8 columns)
magick montage frame_*.png -tile 8x -geometry +0+0 -background none sheet.png
```

---

## Appendix A: Shipped Games Using AI Sprites (Case Studies)

These are real games that shipped with AI-generated sprite art:

| Game | Developer | AI Tool | Workflow |
|------|-----------|---------|----------|
| **King's Bet** | FireBrick Games | Ludo.ai | Entire enemy cast — 2-3 min per animation |
| **Alumnia Knights** | Solo dev (Bouchier Tristan) | Ludo.ai | Every character spritesheet in the RPG |
| **PixelStar** | Indie (2025) | Stable Diffusion | SD assets compressed for retro, post-processed in Aseprite |
| **CastleBit** | Indie (2025) | Midjourney | "16-bit castle, earthy tones" prompts + manual refinement |

**Common pattern across all**: Generate 5-20 variations → pick best → refine in Aseprite → export. No one ships raw AI output — everyone does a manual polish pass.

---

## Appendix B: Blender 3D-to-2D Sprite Addons (Complete List)

| Addon | URL | Best For | Price |
|-------|-----|----------|-------|
| **Blender Isometric Renderer** | github.com/GraesonB/Blender-Isometric-Renderer | 4/8/16-direction renders, ideal for RO style | Free |
| **Sprite Sheet Maker** | extensions.blender.org/add-ons/sprite-sheet-maker | Built-in pixelation, auto camera (v5.1.1, Blender 5.0+) | Free |
| **Blender Spritesheet Renderer** | github.com/chrishayesmu/Blender-Spritesheet-Renderer | JSON metadata output, material iteration | Free |
| **BlenderSpriteGenerator** | github.com/RubielGames/BlenderSpriteGenerator | Multi-angle 2D/2.5D sprites | Free |
| **Game Sprite Creator** | github.com/johnferley/Game-Sprite-Creator | Top-down + isometric presets, auto sheets | Free |
| **Blender To Pixels** | astropulse.itch.io/blender-to-pixels | Compositor pixel art post-process (any model) | Free |
| **Sprite 2D** | kameloov.itch.io/sprite-2d | Multi-angle with presets | $12 |
| **PIXELARTOR** | github.com/Chleba/PIXELARTOR | Standalone glTF/FBX → sprite frames | Free |
| **Spritec** | github.com/ProtoArt/spritec | Rust CLI, inspired by Dead Cells workflow | Free |

**Recommended combo**: Blender Isometric Renderer (8-direction rendering) + Blender To Pixels (pixel art post-process) + Lucas Roedel Pixel Art Shader (lucasroedel.gumroad.com/l/pixel_art, free, Blender 5.0).

---

## Appendix C: Dead Cells — The Gold Standard 3D-to-2D Workflow

Dead Cells is the most cited example of shipping a game with 3D-rendered 2D sprites:

```
Dead Cells Pipeline:
1. Artist drew basic 2D pixel art model sheets (concept reference)
2. Created SIMPLE 3D models in 3DS Max ("would make any 3D artist's eyes bleed")
3. Rigged with basic skeleton
4. Animated with keyframes
5. Rendered each frame to PNG with toon shader + normal maps
6. Post-processed for final pixel art look

Key insight: "The 3D modeling is very basic."
Quality of 3D model matters FAR LESS than rendering + post-processing.

Massive advantage: Reusing old 3D assets for new characters saved
"hundreds of hours of work." Modifying animations for gameplay tuning
takes MINUTES instead of HOURS of redrawing.
```

**The `spritec` tool** (github.com/ProtoArt/spritec) was directly inspired by this workflow and can be used to replicate it.

---

## Appendix D: RO Sprite Layer Rendering (roBrowser Source Code)

From roBrowserLegacy `EntityRender.js`, the actual draw order code:

```javascript
// Direction determines layer ordering
var direction = (Camera.direction + entity.direction + 8) % 8;
var behind = direction > 1 && direction < 6;

// Draw order changes based on facing:
// Front-facing (dir 0-2, 6-7):
//   Shield → behind body (z=-1)
//   Weapon → in front (z=bodyOffset+250)
//   Garment → in front of body

// Back-facing (dir 3-5):
//   Shield → in front (z=bodyOffset+300)
//   Weapon → behind body (z=50)
//   Garment → behind body

// Anchor point calculation:
// headPosition = bodyPosition + bodyAnchor - headAnchor
// Only the FIRST anchor point of each sprite is used for stitching
```

This direction-dependent draw ordering is critical for authentic RO character rendering.

---

## Appendix E: UE5 Equipment Compositing (Implementation Code)

### Option A: Multi-FlipbookComponent with Direction-Based Sort

```cpp
void AROCharacter::UpdateLayerOrder()
{
    bool bFacingAway = (Direction >= 3 && Direction <= 5);

    if (bFacingAway)
    {
        WeaponComponent->SetTranslucentSortPriority(50);   // Behind body
        ShieldComponent->SetTranslucentSortPriority(350);   // In front
        GarmentComponent->SetTranslucentSortPriority(250);  // Behind head
    }
    else
    {
        WeaponComponent->SetTranslucentSortPriority(250);  // In front of body
        ShieldComponent->SetTranslucentSortPriority(-1);    // Behind body
        GarmentComponent->SetTranslucentSortPriority(50);   // Behind body
    }
}
```

### Option B: Render Target Pre-Compositing (Better Performance)

Composite all layers into one texture per frame, reducing draw calls:

```cpp
void AROCharacter::CompositeCurrentFrame()
{
    // UCanvasRenderTarget2D draws all layers onto single texture
    CompositeRT->OnCanvasRenderTargetUpdate.AddDynamic(this,
        &AROCharacter::DrawAllLayers);
    CompositeRT->UpdateResource();

    // Single sprite component uses the composited texture
    SingleSpriteComponent->GetDynamicMaterial()->SetTextureParameterValue(
        "MainTex", CompositeRT);
}

void AROCharacter::DrawAllLayers(UCanvas* Canvas, int32 W, int32 H)
{
    Canvas->DrawTile(BodyTexture, 0, 0, W, H, 0, 0, W, H);
    Canvas->DrawTile(HeadTexture, HeadOffset.X, HeadOffset.Y, W, H, 0, 0, W, H);
    Canvas->DrawTile(WeaponTexture, WeaponOffset.X, WeaponOffset.Y, W, H, 0, 0, W, H);
}
```

### Hair Color via Palette Atlas (Most Efficient)

Stack all 9 hair palettes into one 256x9 texture:

```
Material: Sample palette at UV = (paletteIndex, colorVariant / 9.0)
Swap colors by changing a single scalar parameter (Y offset)
One palette texture total for ALL hair colors
```

---

## Appendix F: Steam AI Disclosure Rules (Updated Jan 2026)

```
CURRENT STEAM POLICY:
- AI tools used to BUILD games: NO disclosure needed
- AI content players SEE/HEAR/INTERACT with: MUST be labeled
- Games using AI art from non-developer-owned sources: BANNED
- ~8,000 games on Steam disclose AI art usage as of July 2025

RECOMMENDED ACTION:
- Train LoRAs on your own art (or use permissively licensed ones)
- Heavily post-process in Aseprite (adds human authorship)
- Disclose AI usage in your Steam page description
- Keep records of your prompt + editing process
```

---

## Appendix G: AI 3D Model → Sprite Pipeline (New Finding)

The fastest zero-to-sprite pipeline discovered:

```
PIPELINE: Concept Art → 3D Model → Rig → Animate → Render → Pixel Art

Step 1: Generate concept art in ComfyUI (2 min)
Step 2: Upload to Tripo3D image-to-3D (30 sec)
Step 3: Upload to Mixamo for auto-rigging (5 min)
         → Select animations: Walk, Idle, Attack, Death, Cast
         → Enable "In Place" checkbox (prevents drift)
         → Download as FBX
Step 4: Import into Blender (2 min)
Step 5: Apply cel-shading material:
         → Shader to RGB → ColorRamp (3 steps, Constant) → Output
         → Add Solidify modifier for 1-2px outline
Step 6: Install Blender Isometric Renderer addon
         → Set 8 directions, isometric angle
         → Render all animations × all directions
Step 7: Post-process with Blender To Pixels (compositor)
Step 8: Nearest-neighbor downscale to target sprite size
Step 9: Final cleanup in Aseprite

Total time per character: ~2-4 hours (mostly rendering)
Consistency: PERFECT (same 3D model = identical sprites)
Cost: FREE (Tripo free tier + Mixamo free + Blender free)
```

This pipeline solves the #1 problem with AI 2D generation: **consistency**. The 3D model IS the character — proportions, equipment, and silhouette are locked.
