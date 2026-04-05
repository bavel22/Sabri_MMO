"""
Generate RO Classic-style ground textures via ComfyUI API.
Warm, hand-painted, flat, top-down tiling textures.
"""
import json
import urllib.request
import time
import os
import random

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def queue_prompt(prompt):
    data = json.dumps({"prompt": prompt}).encode('utf-8')
    req = urllib.request.Request(f"{COMFYUI_URL}/prompt", data=data,
                                 headers={"Content-Type": "application/json"})
    resp = urllib.request.urlopen(req)
    return json.loads(resp.read())

def wait_for_completion(prompt_id, timeout=120):
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history:
                return history[prompt_id]
        except:
            pass
        time.sleep(1)
    return None

def download_image(filename, save_path):
    url = f"{COMFYUI_URL}/view?filename={filename}&type=output"
    urllib.request.urlretrieve(url, save_path)

def generate_texture(name, positive_prompt, negative_prompt, seed=None):
    if seed is None:
        seed = random.randint(1, 999999999)

    prompt = {
        "1": {
            "class_type": "CheckpointLoaderSimple",
            "inputs": {"ckpt_name": "Illustrious-XL-v0.1.safetensors"}
        },
        "2": {
            "class_type": "LoraLoader",
            "inputs": {
                "model": ["1", 0],
                "clip": ["1", 1],
                "lora_name": "ROSprites-v2.1C.safetensors",
                "strength_model": 0.3,
                "strength_clip": 0.3
            }
        },
        "3": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "clip": ["2", 1],
                "text": positive_prompt
            }
        },
        "4": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "clip": ["2", 1],
                "text": negative_prompt
            }
        },
        "5": {
            "class_type": "EmptyLatentImage",
            "inputs": {"width": 1024, "height": 1024, "batch_size": 1}
        },
        "6": {
            "class_type": "KSampler",
            "inputs": {
                "model": ["2", 0],
                "positive": ["3", 0],
                "negative": ["4", 0],
                "latent_image": ["5", 0],
                "seed": seed,
                "steps": 25,
                "cfg": 7.0,
                "sampler_name": "euler_ancestral",
                "scheduler": "normal",
                "denoise": 1.0
            }
        },
        "7": {
            "class_type": "VAEDecode",
            "inputs": {"samples": ["6", 0], "vae": ["1", 2]}
        },
        "8": {
            "class_type": "SaveImage",
            "inputs": {"images": ["7", 0], "filename_prefix": name}
        }
    }

    print(f"  Generating {name}...")
    result = queue_prompt(prompt)
    prompt_id = result.get("prompt_id")
    if not prompt_id:
        print(f"  ERROR: No prompt_id returned")
        return False

    history = wait_for_completion(prompt_id, timeout=180)
    if not history:
        print(f"  ERROR: Timed out waiting for {name}")
        return False

    # Find output image
    outputs = history.get("outputs", {})
    for node_id, node_output in outputs.items():
        images = node_output.get("images", [])
        for img in images:
            filename = img["filename"]
            save_path = os.path.join(OUTPUT_DIR, f"{name}.png")
            download_image(filename, save_path)
            print(f"  Saved: {save_path}")
            return True

    print(f"  ERROR: No output image found for {name}")
    return False


# Common negative for all ground textures
NEG = "photorealistic, 3d_render, realistic, person, character, face, text, watermark, signature, blurry, noisy, perspective, vanishing_point, horizon, sky, scenery, landscape, depth, shadow, 3d, render"

# ============================================================
# RO-specific ground textures — warm, flat, hand-painted, top-down
# ============================================================

textures = [
    {
        "name": "T_Ground_Grass_RO",
        "prompt": "no_humans, flat_color, hand_painted, game_texture, seamless_texture, top_down_view, grass, green_grass, warm_green, soft_brush_strokes, rpg, fantasy, ragnarok_online, muted_colors, watercolor_(medium), simple, flat, uniform, game_asset, tileable, overhead_view, macro, extreme_close-up, full_frame",
        "seed": 7001,
    },
    {
        "name": "T_Ground_Dirt_RO",
        "prompt": "no_humans, flat_color, hand_painted, game_texture, seamless_texture, top_down_view, dirt, brown_earth, warm_ochre, sandy_brown, soft_brush_strokes, rpg, fantasy, ragnarok_online, muted_colors, watercolor_(medium), simple, flat, uniform, game_asset, tileable, overhead_view, macro, extreme_close-up, full_frame",
        "seed": 7002,
    },
    {
        "name": "T_Ground_Cobble_RO",
        "prompt": "no_humans, flat_color, hand_painted, game_texture, seamless_texture, top_down_view, cobblestone, medieval_road, warm_gray_stones, mortar_lines, rpg, fantasy, ragnarok_online, muted_colors, watercolor_(medium), simple, flat, uniform, game_asset, tileable, overhead_view, macro, extreme_close-up, full_frame",
        "seed": 7003,
    },
    {
        "name": "T_Ground_Stone_RO",
        "prompt": "no_humans, flat_color, hand_painted, game_texture, seamless_texture, top_down_view, stone_floor, medieval_plaza, warm_beige_stone, flat_tiles, rpg, fantasy, ragnarok_online, muted_colors, watercolor_(medium), simple, flat, uniform, game_asset, tileable, overhead_view, macro, extreme_close-up, full_frame",
        "seed": 7004,
    },
    {
        "name": "T_Ground_GrassDirt_RO",
        "prompt": "no_humans, flat_color, hand_painted, game_texture, seamless_texture, top_down_view, grass_and_dirt, worn_path, green_grass_with_brown_dirt_patches, rpg, fantasy, ragnarok_online, muted_colors, watercolor_(medium), simple, flat, uniform, game_asset, tileable, overhead_view, macro, extreme_close-up, full_frame",
        "seed": 7005,
    },
]

print("\n=== Generating RO Ground Textures ===\n")

# Check if ComfyUI is running
try:
    urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=3)
    print("ComfyUI server is running.\n")
except:
    print("ERROR: ComfyUI server not running! Start it first:")
    print("  cd C:/ComfyUI && python main.py")
    exit(1)

success = 0
for tex in textures:
    if generate_texture(tex["name"], tex["prompt"], NEG, tex["seed"]):
        success += 1

print(f"\n=== Done! Generated {success}/{len(textures)} ground textures ===")
print(f"Output: {OUTPUT_DIR}")
