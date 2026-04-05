# Quick Start Guide — 2D Sprite Pipeline

## What's Set Up

### ComfyUI (C:/ComfyUI/)
- ComfyUI installed with Python 3.12 venv
- PyTorch nightly cu128 (RTX 5090 compatible)
- 7 custom nodes installed:
  - `comfyui_controlnet_aux` — DWPose/OpenPose for pose control
  - `ComfyUI_IPAdapter_plus` — Character consistency via reference images
  - `ComfyUI-layerdiffuse` — Native transparent background generation
  - `ComfyUI-PixelArt-Detector` — Pixel grid snap + palette quantization
  - `ComfyUI-Impact-Pack` — SAM segmentation + batch tools
  - `was-node-suite-comfyui` — Image pixelate + utilities
  - `comfyui-gametools-2d` — Chroma key + sprite grid tools

### Scripts & Folders
- `C:/ComfyUI/start_comfyui.bat` — Launch ComfyUI
- `C:/ComfyUI/download_models.bat` — Download AI models
- `2D animations/generate_prompts.py` — Generate prompt CSVs for batch processing
- `2D animations/prompts/` — 200 character + 60 monster prompt files
- `2D animations/sprites/` — Full folder structure for all 20 classes + monsters
- `2D animations/sprites/references/master_palette.txt` — 256-color master palette

## Step 1: Download Models

Run `C:/ComfyUI/download_models.bat` to download the Pixel Art XL LoRA and SDXL VAE.

Then manually download (login required):
1. **ROSPRITE LoRA** from https://civitai.com/models/1043663
   → Save to `C:/ComfyUI/models/loras/ROSPRITE_v2.1C.safetensors`

2. **Illustrious XL v0.1** from https://huggingface.co/OnomaAIResearch/Illustrious-xl-early-release-v0
   → Save to `C:/ComfyUI/models/checkpoints/`

## Step 2: Launch ComfyUI

Double-click `C:/ComfyUI/start_comfyui.bat`
Open browser to http://127.0.0.1:8188

## Step 3: First Test Generation

1. In ComfyUI, right-click canvas → Add Node → loaders → Load Checkpoint
2. Select your Illustrious XL checkpoint
3. Add KSampler, CLIP Text Encode (positive + negative), Empty Latent Image, VAE Decode, Save Image
4. Positive prompt: `ROSPRITE, pixel art sprite, chibi knight, male, full body, front view, sword, armor, white background, crisp edges`
5. Negative: `blurry, smooth, realistic, 3D, photograph, watermark, bad anatomy, extra limbs`
6. Set: 768x768, 20 steps, euler_a, CFG 7
7. Queue Prompt — should generate in ~6 seconds on RTX 5090

## Step 4: Use Pre-Generated Prompts

The `prompts/` folder has CSV files for every class. Open them to copy-paste prompts into ComfyUI, or use the ComfyUI batch processing nodes to load them automatically.

## Full Documentation

- `2D_Sprite_Art_Pipeline_Plan.md` — Master plan (1,457 lines)
- `Detailed_Implementation_Guide.md` — Step-by-step walkthroughs (1,686 lines)
