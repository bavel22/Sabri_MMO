"""
Generate sandy/desert ground textures for Morroc-style zones.
Sand warm, sand cool, dry earth, sandstone cliff.
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
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/sand"
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

NEGATIVE = (
    "photorealistic, photograph, 3d render, plastic, glossy, shiny, specular, "
    "metallic, blurry, low quality, worst quality, watermark, signature, text, "
    "dramatic lighting, high contrast, neon, vivid colors, cold blue, "
    "person, character, face, items, objects, perspective, sky, horizon, "
    "side view, grid, symmetric, border, frame, vignette, "
    "individual leaves, grass blades, clover, plant shapes, stems, "
    "pixel art, mosaic, repeating pattern, stripes, green"
)

TEXTURES = [
    # Sand Warm — golden amber, primary desert ground
    {
        "name": "T_Sand_Warm_01",
        "prompt": (
            "overhead view of painted desert sand ground surface, oil painting style, "
            "warm golden amber sand with subtle orange and sienna tones, gentle ripple "
            "patterns in the sand, soft organic color variation, sun-baked desert palette, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 60001,
    },
    {
        "name": "T_Sand_Warm_02",
        "prompt": (
            "overhead view of painted desert sand ground surface, oil painting style, "
            "warm golden amber sand with subtle orange and sienna tones, gentle ripple "
            "patterns in the sand, soft organic color variation, sun-baked desert palette, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 60002,
    },
    {
        "name": "T_Sand_Warm_03",
        "prompt": (
            "overhead view of painted desert sand ground surface, oil painting style, "
            "warm golden amber sand with subtle orange and sienna tones, gentle ripple "
            "patterns in the sand, soft organic color variation, sun-baked desert palette, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 60003,
    },
    # Sand Cool — pale tan with hints of gray, shaded areas
    {
        "name": "T_Sand_Cool_01",
        "prompt": (
            "overhead view of painted desert ground surface, oil painting style, "
            "pale tan and cream colored sand with cooler gray-brown shadow patches, "
            "dry dusty surface, subtle wind-swept texture, muted warm palette, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 61001,
    },
    {
        "name": "T_Sand_Cool_02",
        "prompt": (
            "overhead view of painted desert ground surface, oil painting style, "
            "pale tan and cream colored sand with cooler gray-brown shadow patches, "
            "dry dusty surface, subtle wind-swept texture, muted warm palette, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 61002,
    },
    # Dry cracked earth — darker patches in sand
    {
        "name": "T_DryEarth_01",
        "prompt": (
            "overhead view of painted dry cracked earth surface, oil painting style, "
            "dark brown and burnt sienna dried mud with crack lines, parched desert "
            "ground, warm earth tones, subtle orange undertone, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 62001,
    },
    {
        "name": "T_DryEarth_02",
        "prompt": (
            "overhead view of painted dry cracked earth surface, oil painting style, "
            "dark brown and burnt sienna dried mud with crack lines, parched desert "
            "ground, warm earth tones, subtle orange undertone, "
            "hand-painted game texture, matte surface, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 62002,
    },
    # Sandstone cliff — for slopes
    {
        "name": "T_Sandstone_Cliff_01",
        "prompt": (
            "painted sandstone cliff face texture, oil painting style, layered "
            "sedimentary rock in warm orange and tan and cream bands, horizontal "
            "stratification lines, weathered and eroded surface, desert canyon wall, "
            "hand-painted game art, matte finish, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 63001,
    },
    {
        "name": "T_Sandstone_Cliff_02",
        "prompt": (
            "painted sandstone cliff face texture, oil painting style, layered "
            "sedimentary rock in warm orange and tan and cream bands, horizontal "
            "stratification lines, weathered and eroded surface, desert canyon wall, "
            "hand-painted game art, matte finish, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 63002,
    },
    {
        "name": "T_Sandstone_Cliff_03",
        "prompt": (
            "painted rough sandstone rock surface texture, oil painting style, "
            "warm brown and orange stone with deep cracks and rough hewn surfaces, "
            "desert rock formation, sun-baked warm tones, "
            "hand-painted game art, matte finish, flat lighting, seamless tileable"
        ),
        "lora": 0.15, "cfg": 6.5, "steps": 60, "seed": 63003,
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
        print(f"    Error {e.code}: {body[:200]}")
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


def build_workflow(prompt, negative, seed, cfg, steps, lora):
    nodes = {
        "1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CHECKPOINT}},
    }
    model_src, clip_src = ["1", 0], ["1", 1]
    if lora > 0:
        nodes["2"] = {
            "class_type": "LoraLoader",
            "inputs": {"model": ["1", 0], "clip": ["1", 1],
                       "lora_name": LORA, "strength_model": lora, "strength_clip": lora}
        }
        model_src, clip_src = ["2", 0], ["2", 1]
    nodes["3"] = {"class_type": "SeamlessTile",
                  "inputs": {"model": model_src, "tiling": "enable", "copy_model": "Make a copy"}}
    nodes["4"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": clip_src, "text": prompt}}
    nodes["5"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": clip_src, "text": negative}}
    nodes["6"] = {"class_type": "EmptyLatentImage", "inputs": {"width": 1024, "height": 1024, "batch_size": 1}}
    nodes["7"] = {"class_type": "KSampler",
                  "inputs": {"model": ["3", 0], "positive": ["4", 0], "negative": ["5", 0],
                             "latent_image": ["6", 0], "seed": seed, "steps": steps, "cfg": cfg,
                             "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}}
    nodes["8"] = {"class_type": "CircularVAEDecode",
                  "inputs": {"samples": ["7", 0], "vae": ["1", 2], "tiling": "enable"}}
    nodes["9"] = {"class_type": "SaveImage",
                  "inputs": {"images": ["8", 0], "filename_prefix": "sand"}}
    return nodes


def postprocess(img_path, name):
    img = Image.open(img_path)
    img = ImageEnhance.Color(img).enhance(0.85)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= 1.03
    arr[:, :, 2] *= 0.97
    arr = arr.clip(0, 255).astype(np.uint8)
    Image.fromarray(arr).save(img_path)
    # Tile preview
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
    print(f"  Sandy/Desert Texture Generator - {total} textures")
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
        print(f"[{i+1}/{total}] {name} (seed={tex['seed']})")
        workflow = build_workflow(tex["prompt"], NEGATIVE, tex["seed"], tex["cfg"], tex["steps"], tex["lora"])
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
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
