"""
Iteration 2 — Focus on what worked in round 1:
  - "Mixed B" style (soft green-brown surface blend) was best
  - Describe COLORS and SURFACE, not plants
  - Avoid "grass" as noun (generates blades)
  - More seeds of winning prompts + new color variations
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
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/iter2"
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

NEGATIVE = (
    "photorealistic, photograph, 3d render, plastic, glossy, shiny, specular, "
    "metallic, blurry, low quality, worst quality, watermark, signature, text, "
    "dramatic lighting, high contrast, neon, vivid colors, "
    "person, character, face, items, objects, perspective, sky, horizon, "
    "side view, grid, symmetric, border, frame, vignette, "
    "individual leaves, individual grass blades, scattered objects, flowers, "
    "clover, shamrock, leaf shapes, plant silhouettes, stems, "
    "pixel art, mosaic, repeating pattern, stripes, streaks, lines, "
    "abstract art, geometric shapes"
)

TEXTURES = [
    # =============================================
    # GROUP A: "Mixed B" winners — more seeds of the best prompt
    # =============================================
    {
        "name": "T_GreenBrown_01",
        "prompt": (
            "overhead view of fantasy game ground surface, hand-painted oil painting style, "
            "mixed green and brown earth tones, warm olive greens blending into ochre brown "
            "patches, splotchy organic color variation, soft brushstroke texture throughout, "
            "muted warm palette, matte surface, flat even lighting, full frame ground "
            "texture, seamless tileable, game asset"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 50001,
    },
    {
        "name": "T_GreenBrown_02",
        "prompt": (
            "overhead view of fantasy game ground surface, hand-painted oil painting style, "
            "mixed green and brown earth tones, warm olive greens blending into ochre brown "
            "patches, splotchy organic color variation, soft brushstroke texture throughout, "
            "muted warm palette, matte surface, flat even lighting, full frame ground "
            "texture, seamless tileable, game asset"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 50002,
    },
    {
        "name": "T_GreenBrown_03",
        "prompt": (
            "overhead view of fantasy game ground surface, hand-painted oil painting style, "
            "mixed green and brown earth tones, warm olive greens blending into ochre brown "
            "patches, splotchy organic color variation, soft brushstroke texture throughout, "
            "muted warm palette, matte surface, flat even lighting, full frame ground "
            "texture, seamless tileable, game asset"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 50003,
    },
    {
        "name": "T_GreenBrown_04",
        "prompt": (
            "overhead view of fantasy game ground surface, hand-painted oil painting style, "
            "mixed green and brown earth tones, warm olive greens blending into ochre brown "
            "patches, splotchy organic color variation, soft brushstroke texture throughout, "
            "muted warm palette, matte surface, flat even lighting, full frame ground "
            "texture, seamless tileable, game asset"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 50004,
    },

    # =============================================
    # GROUP B: More green dominant (for lush areas)
    # =============================================
    {
        "name": "T_LushGreen_01",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, mostly sage green "
            "and moss green with subtle darker green patches, hints of warm yellow-green, "
            "very slight brown undertone in shadows, soft blended color fields, "
            "hand-painted game texture, matte, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 51001,
    },
    {
        "name": "T_LushGreen_02",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, mostly sage green "
            "and moss green with subtle darker green patches, hints of warm yellow-green, "
            "very slight brown undertone in shadows, soft blended color fields, "
            "hand-painted game texture, matte, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 51002,
    },
    {
        "name": "T_LushGreen_03",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, rich emerald green "
            "and forest green color fields with occasional yellow-green highlights, "
            "dark moss patches, organic soft-edged color blending like impressionist painting, "
            "hand-painted game texture, warm muted tones, flat lighting, seamless tileable"
        ),
        "lora": 0.12, "cfg": 6.5, "steps": 60, "seed": 51003,
    },

    # =============================================
    # GROUP C: Yellow-green (autumn/dry fields)
    # =============================================
    {
        "name": "T_DryField_01",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, warm yellow-green "
            "and golden-brown dry field colors, faded green patches mixing with straw-colored "
            "areas, late summer meadow palette, soft organic blending, hand-painted game "
            "texture, matte, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 52001,
    },
    {
        "name": "T_DryField_02",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, warm yellow-green "
            "and golden-brown dry field colors, faded green patches mixing with straw-colored "
            "areas, late summer meadow palette, soft organic blending, hand-painted game "
            "texture, matte, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 52002,
    },

    # =============================================
    # GROUP D: Brown dominant (paths, clearings)
    # =============================================
    {
        "name": "T_EarthPath_01",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, warm packed earth "
            "in ochre and sienna tones, subtle variation between lighter sandy patches and "
            "darker brown areas, very faint green tinge in places, worn footpath feeling, "
            "hand-painted game texture, matte, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 53001,
    },
    {
        "name": "T_EarthPath_02",
        "prompt": (
            "overhead view of painted ground surface, oil painting style, warm packed earth "
            "in ochre and sienna tones, subtle variation between lighter sandy patches and "
            "darker brown areas, very faint green tinge in places, worn footpath feeling, "
            "hand-painted game texture, matte, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 53002,
    },

    # =============================================
    # GROUP E: Rocky/cliff surfaces
    # =============================================
    {
        "name": "T_StoneCliff_01",
        "prompt": (
            "painted stone surface texture, oil painting style, layered rock face with "
            "cracks and rough weathered surface, muted gray-purple and warm brown tones, "
            "slight mossy green in crevices, hand-painted game art, thick impasto paint "
            "texture, matte finish, flat lighting, seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 54001,
    },
    {
        "name": "T_StoneCliff_02",
        "prompt": (
            "painted stone surface texture, oil painting style, rough hewn rock with deep "
            "cracks, gray and dark brown stone, purple undertones in shadows, warm highlights "
            "on edges, hand-painted game texture, matte surface, flat lighting, "
            "seamless tileable, full frame"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 54002,
    },

    # =============================================
    # GROUP F: No LoRA — pure SDXL (comparison)
    # =============================================
    {
        "name": "T_NoLora_Green_01",
        "prompt": (
            "seamless tileable ground surface texture top view, stylized hand-painted, "
            "oil painting of earth and vegetation from above, olive green and sage green "
            "color fields blending with ochre brown patches, organic splotchy variation, "
            "matte diffuse surface, flat uniform lighting, game art asset"
        ),
        "lora": 0.0, "cfg": 6.5, "steps": 60, "seed": 55001,
    },
    {
        "name": "T_NoLora_Green_02",
        "prompt": (
            "seamless tileable ground surface texture top view, stylized hand-painted, "
            "oil painting of earth and vegetation from above, olive green and sage green "
            "color fields blending with ochre brown patches, organic splotchy variation, "
            "matte diffuse surface, flat uniform lighting, game art asset"
        ),
        "lora": 0.0, "cfg": 6.5, "steps": 60, "seed": 55002,
    },

    # =============================================
    # GROUP G: Impasto / thick paint emphasis
    # =============================================
    {
        "name": "T_Impasto_Green_01",
        "prompt": (
            "thick impasto oil painting ground texture seen from above, heavy visible "
            "brushstrokes of olive green and yellow-ochre and brown, textured paint surface "
            "with ridges, warm muted earth palette, organic irregular color placement, "
            "no distinct objects just color and texture, matte, flat lighting, "
            "seamless tileable, game texture"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 56001,
    },
    {
        "name": "T_Impasto_Green_02",
        "prompt": (
            "thick impasto oil painting ground texture seen from above, heavy visible "
            "brushstrokes of olive green and yellow-ochre and brown, textured paint surface "
            "with ridges, warm muted earth palette, organic irregular color placement, "
            "no distinct objects just color and texture, matte, flat lighting, "
            "seamless tileable, game texture"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 56002,
    },

    # =============================================
    # GROUP H: Mossy/forest floor (dark cool green)
    # =============================================
    {
        "name": "T_MossFloor_01",
        "prompt": (
            "overhead view of painted forest floor surface, oil painting style, dark "
            "moss-covered ground in deep green and blue-green tones, patches of brown "
            "earth and decaying matter showing through, damp shaded feeling, cool muted "
            "palette, hand-painted game texture, matte, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 57001,
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


def build_workflow(prompt, negative, seed, cfg, steps, lora, sampler="euler"):
    nodes = {
        "1": {
            "class_type": "CheckpointLoaderSimple",
            "inputs": {"ckpt_name": CHECKPOINT}
        },
    }
    model_src = ["1", 0]
    clip_src = ["1", 1]

    if lora > 0:
        nodes["2"] = {
            "class_type": "LoraLoader",
            "inputs": {
                "model": ["1", 0], "clip": ["1", 1],
                "lora_name": LORA,
                "strength_model": lora, "strength_clip": lora,
            }
        }
        model_src = ["2", 0]
        clip_src = ["2", 1]

    nodes["3"] = {
        "class_type": "SeamlessTile",
        "inputs": {"model": model_src, "tiling": "enable", "copy_model": "Make a copy"}
    }
    nodes["4"] = {
        "class_type": "CLIPTextEncode",
        "inputs": {"clip": clip_src, "text": prompt}
    }
    nodes["5"] = {
        "class_type": "CLIPTextEncode",
        "inputs": {"clip": clip_src, "text": negative}
    }
    nodes["6"] = {
        "class_type": "EmptyLatentImage",
        "inputs": {"width": 1024, "height": 1024, "batch_size": 1}
    }
    nodes["7"] = {
        "class_type": "KSampler",
        "inputs": {
            "model": ["3", 0],
            "positive": ["4", 0], "negative": ["5", 0],
            "latent_image": ["6", 0],
            "seed": seed, "steps": steps, "cfg": cfg,
            "sampler_name": sampler, "scheduler": "normal", "denoise": 1.0,
        }
    }
    nodes["8"] = {
        "class_type": "CircularVAEDecode",
        "inputs": {"samples": ["7", 0], "vae": ["1", 2], "tiling": "enable"}
    }
    nodes["9"] = {
        "class_type": "SaveImage",
        "inputs": {"images": ["8", 0], "filename_prefix": "iter2"}
    }
    return nodes


def postprocess(img_path, name):
    img = Image.open(img_path)
    img = ImageEnhance.Color(img).enhance(0.85)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= 1.03
    arr[:, :, 2] *= 0.97
    arr = arr.clip(0, 255).astype(np.uint8)
    Image.fromarray(arr).save(img_path)

    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w * 3, h * 3))
    for r in range(3):
        for c in range(3):
            preview.paste(img, (c * w, r * h))
    if preview.width > 2048:
        ratio = 2048 / preview.width
        preview = preview.resize((2048, int(preview.height * ratio)), Image.LANCZOS)
    preview.save(os.path.join(PREVIEW_DIR, f"{name}_3x3.jpg"), quality=90)


def main():
    total = len(TEXTURES)
    print(f"{'=' * 60}")
    print(f"  Ground Texture Iteration 2 — {total} textures")
    print(f"  Groups: GreenBrown(4) LushGreen(3) DryField(2)")
    print(f"          EarthPath(2) StoneCliff(2) NoLora(2)")
    print(f"          Impasto(2) MossFloor(1)")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'=' * 60}\n")

    try:
        urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except Exception:
        print("ERROR: ComfyUI not running!")
        sys.exit(1)

    success = 0
    for i, tex in enumerate(TEXTURES):
        name = tex["name"]
        print(f"[{i+1}/{total}] {name} (seed={tex['seed']}, lora={tex['lora']})")

        workflow = build_workflow(
            tex["prompt"], NEGATIVE, tex["seed"],
            tex["cfg"], tex["steps"], tex["lora"]
        )
        prompt_id = queue_prompt(workflow)
        if not prompt_id:
            continue

        history = wait_for_completion(prompt_id, timeout=300)
        if not history:
            print("  TIMEOUT")
            continue

        for nid, nout in history.get("outputs", {}).items():
            for img_info in nout.get("images", []):
                img_path = os.path.join(OUTPUT_DIR, f"{name}.png")
                download_image(img_info["filename"], img_info.get("subfolder", ""), img_path)
                postprocess(img_path, name)
                print(f"  OK -> {name}.png")
                success += 1
                break
            break

    print(f"\n{'=' * 60}")
    print(f"  DONE: {success}/{total}")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"  Previews: {PREVIEW_DIR}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
