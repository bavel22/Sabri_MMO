"""
Generate all-inclusive ground textures — each texture has the full variation
baked in (mixed greens, yellows, browns, dirt patches, splotchy color).

Unlike the v2 script which generates separate grass/dirt/rock layers,
these are single textures that look like the RO screenshot ground —
painterly, varied, organic, ready to use as-is.

Multiple variants generated to use with randomized/paintable material.
"""

import json
import urllib.request
import urllib.parse
import time
import os
import sys
import numpy as np
from PIL import Image, ImageEnhance

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/allinone"
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

NEGATIVE = (
    "photorealistic, photograph, 3d render, plastic, glossy, shiny, specular, "
    "metallic, blurry, low quality, worst quality, watermark, signature, text, "
    "dramatic lighting, high contrast, saturated, neon, vivid colors, "
    "person, character, face, items, objects, perspective, sky, horizon, "
    "side view, grid, symmetric, border, frame, vignette, "
    "individual leaves, scattered objects, flowers, clover shapes, "
    "pixel art, mosaic, repeating pattern"
)

# Each entry tests a different prompt approach
TEXTURES = [
    # --- APPROACH 1: Direct RO ground description ---
    {
        "name": "T_Ground_RO_Mixed_A",
        "prompt": (
            "overhead view of fantasy game ground surface, hand-painted oil painting style, "
            "mixed grass and bare earth, warm olive greens blending into yellow-green and "
            "ochre brown patches, splotchy organic color variation, worn dirt showing through "
            "sparse grass, soft brushstroke texture, muted warm palette, matte surface, "
            "flat even lighting, full frame ground texture, seamless tileable, game asset"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40001,
    },
    {
        "name": "T_Ground_RO_Mixed_B",
        "prompt": (
            "overhead view of fantasy game ground surface, hand-painted oil painting style, "
            "mixed grass and bare earth, warm olive greens blending into yellow-green and "
            "ochre brown patches, splotchy organic color variation, worn dirt showing through "
            "sparse grass, soft brushstroke texture, muted warm palette, matte surface, "
            "flat even lighting, full frame ground texture, seamless tileable, game asset"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40002,
    },
    # --- APPROACH 2: Painterly meadow with variation ---
    {
        "name": "T_Ground_RO_Meadow_A",
        "prompt": (
            "top-down view of a painterly meadow ground texture, oil painting with visible "
            "brushstrokes, patches of sage green and yellow-green grass mixed with sandy "
            "brown bare soil, organic irregular color blending, no individual grass blades, "
            "just soft color patches like a watercolor painting, warm earth tones, "
            "matte diffuse surface, flat lighting from above, seamless tileable, game texture"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40101,
    },
    {
        "name": "T_Ground_RO_Meadow_B",
        "prompt": (
            "top-down view of a painterly meadow ground texture, oil painting with visible "
            "brushstrokes, patches of sage green and yellow-green grass mixed with sandy "
            "brown bare soil, organic irregular color blending, no individual grass blades, "
            "just soft color patches like a watercolor painting, warm earth tones, "
            "matte diffuse surface, flat lighting from above, seamless tileable, game texture"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40102,
    },
    # --- APPROACH 3: Emphasize splotchy color like classic RO ---
    {
        "name": "T_Ground_RO_Splotchy_A",
        "prompt": (
            "abstract painted ground surface texture seen from directly above, large soft "
            "splotches of olive green and yellow-green and warm brown, blended like wet "
            "watercolor on paper, no distinct objects or plants, just organic color fields "
            "merging together, warm muted palette, hand-painted game texture, flat lighting, "
            "seamless tileable, full frame, matte finish"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.12,
        "sampler": "euler",
        "seed": 40201,
    },
    {
        "name": "T_Ground_RO_Splotchy_B",
        "prompt": (
            "abstract painted ground surface texture seen from directly above, large soft "
            "splotches of olive green and yellow-green and warm brown, blended like wet "
            "watercolor on paper, no distinct objects or plants, just organic color fields "
            "merging together, warm muted palette, hand-painted game texture, flat lighting, "
            "seamless tileable, full frame, matte finish"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.12,
        "sampler": "euler",
        "seed": 40202,
    },
    # --- APPROACH 4: Low LoRA, more SDXL control ---
    {
        "name": "T_Ground_RO_Natural_A",
        "prompt": (
            "seamless tileable ground texture top view, stylized hand-painted art style, "
            "natural grass field with varying shades of green from dark forest green to "
            "light yellow-green, patches of exposed brown earth and dry grass, soft edges "
            "between colors, painterly brushwork quality, warm color temperature, "
            "no shadows, no lighting gradient, flat uniform illumination, game asset"
        ),
        "cfg": 7.0,
        "steps": 50,
        "lora": 0.08,
        "sampler": "euler",
        "seed": 40301,
    },
    {
        "name": "T_Ground_RO_Natural_B",
        "prompt": (
            "seamless tileable ground texture top view, stylized hand-painted art style, "
            "natural grass field with varying shades of green from dark forest green to "
            "light yellow-green, patches of exposed brown earth and dry grass, soft edges "
            "between colors, painterly brushwork quality, warm color temperature, "
            "no shadows, no lighting gradient, flat uniform illumination, game asset"
        ),
        "cfg": 7.0,
        "steps": 50,
        "lora": 0.08,
        "sampler": "euler",
        "seed": 40302,
    },
    # --- APPROACH 5: Dirt-heavy ground with grass patches ---
    {
        "name": "T_Ground_RO_DirtGrass_A",
        "prompt": (
            "top-down painted ground texture, mostly warm brown packed earth with scattered "
            "patches of green grass growing through, oil painting style, ochre and sienna "
            "dirt with moss green grass clumps, organic natural distribution, hand-painted "
            "game art, matte surface, flat lighting, seamless tileable, full frame"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40401,
    },
    # --- APPROACH 6: Dense green with subtle variation ---
    {
        "name": "T_Ground_RO_DenseGrass_A",
        "prompt": (
            "top-down painted dense grass ground texture, oil painting style, rich carpet "
            "of green with subtle shifts between dark green and yellow-green areas, hints "
            "of brown earth peeking through, soft dappled color like impressionist painting, "
            "warm muted green palette not vivid, hand-painted game texture, matte, "
            "flat lighting, seamless tileable, full frame"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40501,
    },
    # --- APPROACH 7: Rocky dirt ground ---
    {
        "name": "T_Ground_RO_RockyDirt_A",
        "prompt": (
            "top-down painted rocky dirt ground texture, oil painting style, warm brown "
            "packed earth with embedded small stones and pebbles, occasional crack lines, "
            "ochre sienna and gray-brown tones, worn path surface feeling, hand-painted "
            "game texture, matte surface, flat lighting, seamless tileable, full frame"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40601,
    },
    # --- APPROACH 8: Cliff/slope rock face ---
    {
        "name": "T_Ground_RO_CliffRock_A",
        "prompt": (
            "painted rock cliff face texture, oil painting style, layered stone with cracks "
            "and weathering, muted gray-purple and warm brown tones, mossy green patches in "
            "crevices, rough hewn surface, hand-painted game art style, matte finish, "
            "flat lighting, seamless tileable, full frame"
        ),
        "cfg": 6.5,
        "steps": 60,
        "lora": 0.15,
        "sampler": "euler",
        "seed": 40701,
    },
]


def queue_prompt(workflow):
    data = json.dumps({"prompt": workflow}).encode("utf-8")
    req = urllib.request.Request(
        f"{COMFYUI_URL}/prompt", data=data,
        headers={"Content-Type": "application/json"}
    )
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read()).get("prompt_id")
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        print(f"    ComfyUI error {e.code}: {body[:300]}")
        return None


def wait_for_completion(prompt_id, timeout=300):
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
    params = urllib.parse.urlencode({
        "filename": filename, "subfolder": subfolder, "type": "output"
    })
    urllib.request.urlretrieve(f"{COMFYUI_URL}/view?{params}", save_path)


def build_workflow(prompt, negative, seed, cfg, steps, lora, sampler):
    return {
        "1": {
            "class_type": "CheckpointLoaderSimple",
            "inputs": {"ckpt_name": CHECKPOINT}
        },
        "2": {
            "class_type": "LoraLoader",
            "inputs": {
                "model": ["1", 0], "clip": ["1", 1],
                "lora_name": LORA,
                "strength_model": lora, "strength_clip": lora,
            }
        },
        "3": {
            "class_type": "SeamlessTile",
            "inputs": {
                "model": ["2", 0],
                "tiling": "enable",
                "copy_model": "Make a copy",
            }
        },
        "4": {
            "class_type": "CLIPTextEncode",
            "inputs": {"clip": ["2", 1], "text": prompt}
        },
        "5": {
            "class_type": "CLIPTextEncode",
            "inputs": {"clip": ["2", 1], "text": negative}
        },
        "6": {
            "class_type": "EmptyLatentImage",
            "inputs": {"width": 1024, "height": 1024, "batch_size": 1}
        },
        "7": {
            "class_type": "KSampler",
            "inputs": {
                "model": ["3", 0],
                "positive": ["4", 0], "negative": ["5", 0],
                "latent_image": ["6", 0],
                "seed": seed, "steps": steps, "cfg": cfg,
                "sampler_name": sampler, "scheduler": "normal",
                "denoise": 1.0,
            }
        },
        "8": {
            "class_type": "CircularVAEDecode",
            "inputs": {"samples": ["7", 0], "vae": ["1", 2], "tiling": "enable"}
        },
        "9": {
            "class_type": "SaveImage",
            "inputs": {"images": ["8", 0], "filename_prefix": "allinone"}
        },
    }


def postprocess(img_path, name):
    # Desaturation + warm shift
    img = Image.open(img_path)
    img = ImageEnhance.Color(img).enhance(0.85)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= 1.03
    arr[:, :, 2] *= 0.97
    arr = arr.clip(0, 255).astype(np.uint8)
    Image.fromarray(arr).save(img_path)

    # 3x3 tile preview
    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w * 3, h * 3))
    for r in range(3):
        for c in range(3):
            preview.paste(img, (c * w, r * h))
    max_w = 2048
    if preview.width > max_w:
        ratio = max_w / preview.width
        preview = preview.resize((max_w, int(preview.height * ratio)), Image.LANCZOS)
    preview.save(os.path.join(PREVIEW_DIR, f"{name}_3x3.jpg"), quality=90)


def main():
    print("=" * 60)
    print("  All-Inclusive Ground Texture Generator")
    print(f"  {len(TEXTURES)} textures, 8 prompt approaches")
    print(f"  Output: {OUTPUT_DIR}")
    print("=" * 60)

    try:
        urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except Exception:
        print("ERROR: ComfyUI not running!")
        sys.exit(1)

    success = 0
    for i, tex in enumerate(TEXTURES):
        name = tex["name"]
        print(f"\n[{i+1}/{len(TEXTURES)}] {name} (seed={tex['seed']}, cfg={tex['cfg']}, lora={tex['lora']})")

        workflow = build_workflow(
            tex["prompt"], NEGATIVE, tex["seed"],
            tex["cfg"], tex["steps"], tex["lora"], tex["sampler"]
        )
        prompt_id = queue_prompt(workflow)
        if not prompt_id:
            print("  FAILED to queue")
            continue

        print(f"  Queued: {prompt_id}")
        history = wait_for_completion(prompt_id, timeout=300)
        if not history:
            print("  TIMEOUT")
            continue

        for nid, nout in history.get("outputs", {}).items():
            for img_info in nout.get("images", []):
                img_path = os.path.join(OUTPUT_DIR, f"{name}.png")
                download_image(img_info["filename"], img_info.get("subfolder", ""), img_path)
                postprocess(img_path, name)
                print(f"  Saved: {img_path}")
                success += 1
                break
            break

    print(f"\n{'=' * 60}")
    print(f"  DONE: {success}/{len(TEXTURES)} textures")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"  Previews: {PREVIEW_DIR}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
