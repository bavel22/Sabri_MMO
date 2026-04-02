"""
Generate RO Classic-style ground textures via ComfyUI API — v2.

Improvements over v1:
  - SeamlessTile node (circular Conv2d padding) for inherently seamless output
  - CircularVAEDecode for seamless VAE decoding
  - CFG 6.5 (critical for texture quality), 60 steps, euler sampler
  - LoRA at 0.15 (not 0.3 — too strong for textures)
  - Natural language prompts (not booru tags)
  - Post-processing: seamless verification, desaturation, warm shift
  - 3x3 tile preview generation for visual QA
  - Batch variant generation (multiple seeds per texture type)

Prerequisites:
  - ComfyUI running at http://127.0.0.1:8188
  - Checkpoint: Illustrious-XL-v0.1.safetensors
  - LoRA: ROSprites-v2.1C.safetensors
  - Custom nodes: ComfyUI-seamless-tiling (git clone into custom_nodes/)

Usage:
  python generate_ground_textures_v2.py              # Generate all textures
  python generate_ground_textures_v2.py --dry-run    # Validate workflow only
  python generate_ground_textures_v2.py --type grass  # Generate only grass variants
"""

import json
import urllib.request
import urllib.parse
import time
import os
import sys
import random
import numpy as np
from PIL import Image, ImageEnhance

# ============================================================
# Configuration
# ============================================================

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output"
FINAL_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment"
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"
LORA_STRENGTH = 0.15  # Low for textures — 0.3+ causes sprite artifacts

# Generation settings (research-backed)
WIDTH = 1024
HEIGHT = 1024
STEPS = 60           # Higher = sharper detail for textures
CFG = 6.5            # Critical — NOT 7.0+
SAMPLER = "euler"    # Best for SDXL textures at 60 steps
SCHEDULER = "normal"

# Post-processing
DESATURATION = 0.85    # 15% desaturation (1.0 = no change)
WARM_R_MULT = 1.03     # Red channel +3%
WARM_B_MULT = 0.97     # Blue channel -3%
SEAMLESS_BORDER = 0.20  # 20% border for cross-blend fallback
EDGE_THRESHOLD = 2.0    # Max acceptable edge MAD (0-255 scale)

# ============================================================
# Negative prompt (same for ALL textures)
# ============================================================

NEGATIVE = (
    "photorealistic, photograph, 3d render, smooth gradient, plastic, glossy, "
    "shiny, specular, metallic, blurry, low quality, worst quality, watermark, "
    "signature, text, shadows, dramatic lighting, high contrast, saturated, neon, "
    "vivid colors, cold blue-green, person, character, face, items, objects, "
    "perspective, sky, horizon, side view, repeating pattern, grid, symmetric, "
    "border, frame, vignette, uneven lighting"
)

# ============================================================
# Texture definitions
# ============================================================

TEXTURES = [
    # --- GRASS WARM (olive/sage — primary ground cover) ---
    {
        "name": "T_Grass_Warm_RO",
        "prompt": (
            "hand painted tileable ground grass texture, stylized game art, "
            "oil painting brushstrokes, warm muted olive green with yellow-brown "
            "patches, splotchy color variation, multiple shades of green mixed "
            "together, flat lighting, top-down view, seamless pattern, highly "
            "detailed, sharp focus, game texture asset, matte surface, no shadows, "
            "full frame coverage"
        ),
        "type": "grass",
        "variants": 3,
        "base_seed": 30001,
    },
    # --- GRASS COOL (yellow-green forest variant) ---
    {
        "name": "T_Grass_Cool_RO",
        "prompt": (
            "hand painted tileable ground grass texture, stylized game art, "
            "oil painting brushstrokes, cool emerald green with moss and dark "
            "brown patches, blue-green undertones, forest floor feeling, subtle "
            "leaf litter detail, flat lighting, top-down view, seamless pattern, "
            "highly detailed, sharp focus, game texture asset, matte surface, "
            "no shadows, full frame"
        ),
        "type": "grass",
        "variants": 2,
        "base_seed": 31001,
    },
    # --- DIRT/EARTH ---
    {
        "name": "T_Dirt_RO",
        "prompt": (
            "hand painted tileable ground dirt texture, stylized game art, "
            "oil painting brushstrokes, warm ochre sienna umber earth tones, "
            "subtle pebble and grain detail, dry packed earth feeling, "
            "matte flat lighting, top-down view, seamless pattern, highly "
            "detailed, game texture asset, no shadows, full frame"
        ),
        "type": "dirt",
        "variants": 2,
        "base_seed": 32001,
    },
    # --- ROCK/CLIFF ---
    {
        "name": "T_Rock_RO",
        "prompt": (
            "hand painted rock cliff face texture, stylized game art, "
            "muted purple-gray stone, visible brushstrokes, cracks and "
            "weathering detail, warm desaturated palette, seamless tileable, "
            "flat lighting, no specular highlights, game texture asset, "
            "highly detailed, matte surface, full frame"
        ),
        "type": "rock",
        "variants": 2,
        "base_seed": 33001,
    },
    # --- COBBLESTONE (towns) ---
    {
        "name": "T_Cobble_RO",
        "prompt": (
            "hand painted tileable cobblestone road texture, stylized game art, "
            "oil painting brushstrokes, warm gray and ochre medieval stone pavers, "
            "irregular stone shapes with dark mortar lines, matte flat lighting, "
            "top-down view, seamless pattern, game texture asset, no shadows, "
            "highly detailed, full frame"
        ),
        "type": "cobble",
        "variants": 1,
        "base_seed": 34001,
    },
    # --- SAND (desert zones) ---
    {
        "name": "T_Sand_RO",
        "prompt": (
            "hand painted tileable desert sand texture, stylized game art, "
            "oil painting brushstrokes, warm golden sand with subtle ripple "
            "patterns, sienna and amber tones, organic random variation, "
            "flat lighting, top-down view, seamless pattern, game texture "
            "asset, matte surface, highly detailed, full frame"
        ),
        "type": "sand",
        "variants": 1,
        "base_seed": 35001,
    },
    # --- SNOW (Lutie zone) ---
    {
        "name": "T_Snow_RO",
        "prompt": (
            "hand painted tileable snow ground texture, stylized game art, "
            "oil painting brushstrokes, white snow with subtle blue shadows "
            "and ice crystal hints, cool pale blue tones, soft organic "
            "surface variation, flat lighting, top-down view, seamless "
            "pattern, game texture asset, matte surface, highly detailed"
        ),
        "type": "snow",
        "variants": 1,
        "base_seed": 36001,
    },
    # --- DUNGEON FLOOR ---
    {
        "name": "T_DungeonFloor_v2_RO",
        "prompt": (
            "hand painted tileable dungeon floor texture, stylized game art, "
            "dark blue-gray slate stone, cracked and worn surface, muted purple "
            "undertones, cold and damp feeling, flat lighting, top-down view, "
            "seamless pattern, game texture asset, matte surface, highly detailed"
        ),
        "type": "dungeon",
        "variants": 1,
        "base_seed": 37001,
    },
]


# ============================================================
# ComfyUI API helpers
# ============================================================

def queue_prompt(prompt_workflow):
    """Queue a prompt workflow and return the prompt_id."""
    data = json.dumps({"prompt": prompt_workflow}).encode("utf-8")
    req = urllib.request.Request(
        f"{COMFYUI_URL}/prompt", data=data,
        headers={"Content-Type": "application/json"}
    )
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read()).get("prompt_id")
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        print(f"    ComfyUI API error {e.code}: {body[:500]}")
        return None


def wait_for_completion(prompt_id, timeout=300):
    """Poll ComfyUI history until the prompt completes or times out."""
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history:
                return history[prompt_id]
        except Exception:
            pass
        time.sleep(2)
    return None


def download_image(filename, subfolder, save_path):
    """Download a generated image from ComfyUI output."""
    params = urllib.parse.urlencode({
        "filename": filename, "subfolder": subfolder, "type": "output"
    })
    url = f"{COMFYUI_URL}/view?{params}"
    urllib.request.urlretrieve(url, save_path)


def check_comfyui():
    """Verify ComfyUI server is running."""
    try:
        urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
        return True
    except Exception:
        return False


# ============================================================
# ComfyUI workflow builder
# ============================================================

def build_workflow(positive_prompt, negative_prompt, seed):
    """
    Build the ComfyUI API workflow with SeamlessTile + CircularVAEDecode.

    Node chain:
      1. CheckpointLoaderSimple
      2. LoraLoader (0.15)
      3. SeamlessTile (enable)       <-- circular Conv2d padding
      4. CLIPTextEncode (positive)
      5. CLIPTextEncode (negative)
      6. EmptyLatentImage (1024x1024)
      7. KSampler (euler, 60 steps, cfg 6.5)
      8. CircularVAEDecode (enable)  <-- circular VAE decoding
      9. SaveImage
    """
    return {
        # Node 1: Load checkpoint
        "1": {
            "class_type": "CheckpointLoaderSimple",
            "inputs": {"ckpt_name": CHECKPOINT}
        },
        # Node 2: Apply LoRA (low strength for textures)
        "2": {
            "class_type": "LoraLoader",
            "inputs": {
                "model": ["1", 0],
                "clip": ["1", 1],
                "lora_name": LORA,
                "strength_model": LORA_STRENGTH,
                "strength_clip": LORA_STRENGTH,
            }
        },
        # Node 3: SeamlessTile — patches Conv2d for circular padding
        "3": {
            "class_type": "SeamlessTile",
            "inputs": {
                "model": ["2", 0],
                "tiling": "enable",
                "copy_model": "Make a copy",
            }
        },
        # Node 4: Positive prompt
        "4": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "clip": ["2", 1],
                "text": positive_prompt,
            }
        },
        # Node 5: Negative prompt
        "5": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "clip": ["2", 1],
                "text": negative_prompt,
            }
        },
        # Node 6: Empty latent
        "6": {
            "class_type": "EmptyLatentImage",
            "inputs": {"width": WIDTH, "height": HEIGHT, "batch_size": 1}
        },
        # Node 7: KSampler
        "7": {
            "class_type": "KSampler",
            "inputs": {
                "model": ["3", 0],         # SeamlessTile output
                "positive": ["4", 0],
                "negative": ["5", 0],
                "latent_image": ["6", 0],
                "seed": seed,
                "steps": STEPS,
                "cfg": CFG,
                "sampler_name": SAMPLER,
                "scheduler": SCHEDULER,
                "denoise": 1.0,
            }
        },
        # Node 8: CircularVAEDecode — seamless VAE decoding
        "8": {
            "class_type": "CircularVAEDecode",
            "inputs": {
                "samples": ["7", 0],
                "vae": ["1", 2],
                "tiling": "enable",
            }
        },
        # Node 9: Save
        "9": {
            "class_type": "SaveImage",
            "inputs": {
                "images": ["8", 0],
                "filename_prefix": "ground_tex",
            }
        },
    }


# ============================================================
# Post-processing
# ============================================================

def check_seamless(img_path):
    """
    Check if a texture tiles seamlessly by comparing edge strips.
    Returns (is_seamless, h_error, v_error).
    """
    arr = np.array(Image.open(img_path), dtype=np.float32)
    strip_w = 8  # pixels

    # Horizontal edges (left vs right)
    left = arr[:, :strip_w].mean(axis=1)
    right = arr[:, -strip_w:].mean(axis=1)
    h_error = np.abs(left - right).mean()

    # Vertical edges (top vs bottom)
    top = arr[:strip_w, :].mean(axis=0)
    bottom = arr[-strip_w:, :].mean(axis=0)
    v_error = np.abs(top - bottom).mean()

    is_seamless = h_error < EDGE_THRESHOLD and v_error < EDGE_THRESHOLD
    return is_seamless, h_error, v_error


def make_seamless_crossblend(img_path):
    """
    Fallback: cross-blend with cosine ramp to fix seams.
    Only called if edge check fails.
    """
    img = np.array(Image.open(img_path), dtype=np.float32)
    H, W = img.shape[:2]
    shifted = np.roll(np.roll(img, W // 2, axis=1), H // 2, axis=0)

    bw = int(W * SEAMLESS_BORDER)
    bh = int(H * SEAMLESS_BORDER)
    ramp = lambda n: 0.5 * (1 + np.cos(np.linspace(0, np.pi, n)))

    wx = np.ones(W)
    wx[:bw] = ramp(bw)
    wx[-bw:] = ramp(bw)[::-1]

    wy = np.ones(H)
    wy[:bh] = ramp(bh)
    wy[-bh:] = ramp(bh)[::-1]

    mask = (wy[:, None] * wx[None, :])[:, :, np.newaxis]
    result = (img * mask + shifted * (1 - mask)).clip(0, 255).astype(np.uint8)
    Image.fromarray(result).save(img_path)


def apply_ro_color_correction(img_path):
    """
    Desaturate + warm shift to match RO's muted warm palette.
    """
    img = Image.open(img_path)

    # Step 1: Desaturation (15%)
    enhancer = ImageEnhance.Color(img)
    img = enhancer.enhance(DESATURATION)

    # Step 2: Warm shift (boost R, reduce B)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= WARM_R_MULT   # Red +3%
    arr[:, :, 2] *= WARM_B_MULT   # Blue -3%
    arr = arr.clip(0, 255).astype(np.uint8)

    Image.fromarray(arr).save(img_path)


def generate_tile_preview(img_path, preview_path):
    """
    Create a 3x3 tiled preview image for visual QA.
    Saved as a separate file in the previews directory.
    """
    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w * 3, h * 3))
    for row in range(3):
        for col in range(3):
            preview.paste(img, (col * w, row * h))
    # Downsample to reasonable preview size (max 2048px wide)
    max_w = 2048
    if preview.width > max_w:
        ratio = max_w / preview.width
        preview = preview.resize(
            (max_w, int(preview.height * ratio)), Image.LANCZOS
        )
    preview.save(preview_path, quality=90)


def postprocess(img_path, name):
    """
    Full post-processing pipeline for a single texture.
    """
    # 1. Check seamless
    is_ok, h_err, v_err = check_seamless(img_path)
    if is_ok:
        print(f"    Seamless: OK (h={h_err:.1f}, v={v_err:.1f})")
    else:
        print(f"    Seamless: FIXING (h={h_err:.1f}, v={v_err:.1f} > {EDGE_THRESHOLD})")
        make_seamless_crossblend(img_path)
        is_ok2, h2, v2 = check_seamless(img_path)
        print(f"    After fix: h={h2:.1f}, v={v2:.1f}")

    # 2. RO color correction
    apply_ro_color_correction(img_path)
    print(f"    Color: desaturated {int((1 - DESATURATION) * 100)}%, warm shifted")

    # 3. 3x3 tile preview
    preview_path = os.path.join(PREVIEW_DIR, f"{name}_3x3.jpg")
    generate_tile_preview(img_path, preview_path)
    print(f"    Preview: {preview_path}")


# ============================================================
# Main generation loop
# ============================================================

def generate_texture(tex_def, variant_idx, seed, dry_run=False):
    """Generate a single texture variant."""
    if tex_def["variants"] > 1:
        name = f"{tex_def['name']}_v{variant_idx + 1}"
    else:
        name = tex_def["name"]

    img_path = os.path.join(OUTPUT_DIR, f"{name}.png")

    print(f"\n  [{name}] seed={seed}")

    if dry_run:
        print(f"    DRY RUN — would generate {WIDTH}x{HEIGHT}, {STEPS} steps, cfg={CFG}")
        print(f"    Prompt: {tex_def['prompt'][:80]}...")
        return True

    # Build and queue workflow
    workflow = build_workflow(tex_def["prompt"], NEGATIVE, seed)
    prompt_id = queue_prompt(workflow)
    if not prompt_id:
        print(f"    ERROR: No prompt_id returned")
        return False

    print(f"    Queued: {prompt_id}")
    print(f"    Waiting ({STEPS} steps @ {WIDTH}x{HEIGHT})...")

    # Wait for completion
    history = wait_for_completion(prompt_id, timeout=600)
    if not history:
        print(f"    ERROR: Timed out")
        return False

    # Download output image
    outputs = history.get("outputs", {})
    for node_id, node_output in outputs.items():
        images = node_output.get("images", [])
        for img_info in images:
            filename = img_info["filename"]
            subfolder = img_info.get("subfolder", "")
            download_image(filename, subfolder, img_path)
            print(f"    Downloaded: {img_path}")

            # Post-process
            postprocess(img_path, name)
            return True

    print(f"    ERROR: No output image found")
    return False


def main():
    # Parse args
    dry_run = "--dry-run" in sys.argv
    type_filter = None
    if "--type" in sys.argv:
        idx = sys.argv.index("--type")
        if idx + 1 < len(sys.argv):
            type_filter = sys.argv[idx + 1]

    # Setup output dirs
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(PREVIEW_DIR, exist_ok=True)

    print("=" * 60)
    print("  RO Ground Texture Generator v2")
    print("=" * 60)
    print(f"  Checkpoint: {CHECKPOINT}")
    print(f"  LoRA: {LORA} @ {LORA_STRENGTH}")
    print(f"  Settings: {WIDTH}x{HEIGHT}, {STEPS} steps, cfg={CFG}, {SAMPLER}")
    print(f"  Tiling: SeamlessTile + CircularVAEDecode")
    print(f"  Output: {OUTPUT_DIR}")
    if dry_run:
        print(f"  MODE: DRY RUN (no generation)")
    if type_filter:
        print(f"  FILTER: type={type_filter}")
    print()

    # Check ComfyUI
    if not dry_run:
        if not check_comfyui():
            print("ERROR: ComfyUI not running! Start it first:")
            print("  cd C:/ComfyUI && python main.py")
            sys.exit(1)
        print("ComfyUI server: OK\n")

    # Filter textures if requested
    tex_list = TEXTURES
    if type_filter:
        tex_list = [t for t in TEXTURES if t["type"] == type_filter]
        if not tex_list:
            print(f"ERROR: No textures of type '{type_filter}'")
            print(f"Available types: {sorted(set(t['type'] for t in TEXTURES))}")
            sys.exit(1)

    # Count total
    total = sum(t["variants"] for t in tex_list)
    success = 0

    print(f"Generating {total} textures ({len(tex_list)} types)...\n")

    for tex in tex_list:
        print(f"--- {tex['name']} ({tex['variants']} variant(s)) ---")
        for v in range(tex["variants"]):
            seed = tex["base_seed"] + v
            if generate_texture(tex, v, seed, dry_run):
                success += 1

    print(f"\n{'=' * 60}")
    print(f"  DONE: {success}/{total} textures generated")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"  Previews: {PREVIEW_DIR}")
    if not dry_run:
        print(f"\n  Next steps:")
        print(f"  1. Review 3x3 previews in {PREVIEW_DIR}")
        print(f"  2. Pick best variant per type")
        print(f"  3. Copy winners to {FINAL_DIR}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
