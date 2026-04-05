"""
Generate hand-painted Ragnarok Online style tiling textures via ComfyUI API.

v9 FINAL: Building on v8's excellent art style (ROSprites + booru tags)
but fixing the scene composition issue.

Key changes from v8:
  - Remove "scenery" tag (causes landscape/scene composition)
  - Add "flat, texture, material, surface, pattern, macro, extreme_close-up"
  - Replace "bird's-eye view" with "flat" (bird's eye creates perspective)
  - Stronger negative: "perspective, depth, horizon, room, scene, landscape"
  - "zoom in" to force close-up coverage

ComfyUI must be running on http://127.0.0.1:8188
"""

import json
import urllib.request
import time
import os
import shutil
import glob
import sys

COMFYUI_URL = "http://127.0.0.1:8188"
COMFYUI_OUTPUT = "C:/ComfyUI/output"
OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment"

TEXTURES = [
    (
        "T_StoneWall_RO",
        "no_humans, absurdres, still_life, "
        "stone wall, medieval wall, stone blocks, mortar, "
        "gray stone, brown stone, warm colors, weathered, "
        "watercolor \\(medium\\), flat color, simple background, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro, no perspective",
        99101
    ),
    (
        "T_Grass_RO",
        "no_humans, absurdres, still_life, nature, "
        "grass, green grass, grass field, ground, lawn, "
        "many grass blades, lush green, dense grass, "
        "watercolor \\(medium\\), flat color, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro, from above, directly above",
        99102
    ),
    (
        "T_WoodPlanks_RO",
        "no_humans, absurdres, still_life, "
        "wooden floor, wood planks, floorboard, wood grain, "
        "warm brown, chestnut, oak, horizontal boards, "
        "watercolor \\(medium\\), flat color, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro, from above",
        99103
    ),
    (
        "T_DirtPath_RO",
        "no_humans, absurdres, still_life, nature, "
        "dirt road, dirt ground, earth, soil, gravel, pebbles, "
        "brown earth, dry ground, dusty, "
        "watercolor \\(medium\\), flat color, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro, from above, directly above",
        99104
    ),
    (
        "T_Cobblestone_RO",
        "no_humans, absurdres, still_life, "
        "cobblestone, stone road, paved road, cobbles, "
        "gray stones, rounded stones, mortar gaps, medieval, "
        "watercolor \\(medium\\), flat color, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro, from above, directly above",
        99105
    ),
    (
        "T_DungeonFloor_RO",
        "no_humans, absurdres, still_life, "
        "stone floor, dungeon, stone tiles, dark floor, "
        "dark gray, blue-gray, damp, cracked, underground, "
        "watercolor \\(medium\\), dark, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro, from above, directly above",
        99106
    ),
    (
        "T_DungeonWall_RO",
        "no_humans, absurdres, still_life, "
        "brick wall, dungeon, old bricks, moss, lichen, "
        "dark brown bricks, green moss, crumbling mortar, underground, "
        "watercolor \\(medium\\), dark, "
        "game cg, rpg, fantasy, "
        "texture, material, surface, flat, extreme close-up, zoom in, "
        "full frame, macro",
        99107
    ),
]

NEGATIVE_PROMPT = (
    "1girl, 1boy, character, person, human, face, eyes, body, "
    "animal, creature, monster, "
    "text, watermark, signature, logo, border, frame, "
    "blurry, low quality, worst quality, jpeg artifacts, "
    "3d, realistic, photograph, photo, "
    "sky, cloud, horizon, sun, moon, "
    "building, tree, house, room, interior, exterior, "
    "perspective, depth, vanishing point, "
    "landscape, scenery, panorama, vista, "
    "path, road \\(going somewhere\\), trail, "
    "door, window, arch, tunnel, entrance, "
    "box, container, dish, plate, furniture, "
    "gradient background, spotlight, vignette"
)


def build_workflow(positive_prompt: str, filename_prefix: str, seed: int) -> dict:
    return {
        "1": {
            "class_type": "CheckpointLoaderSimple",
            "inputs": {
                "ckpt_name": "Illustrious-XL-v0.1.safetensors"
            }
        },
        "2": {
            "class_type": "LoraLoader",
            "inputs": {
                "model": ["1", 0],
                "clip": ["1", 1],
                "lora_name": "ROSprites-v2.1C.safetensors",
                "strength_model": 0.5,
                "strength_clip": 0.5
            }
        },
        "3": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "text": positive_prompt,
                "clip": ["2", 1]
            }
        },
        "4": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "text": NEGATIVE_PROMPT,
                "clip": ["2", 1]
            }
        },
        "5": {
            "class_type": "EmptyLatentImage",
            "inputs": {
                "width": 1024,
                "height": 1024,
                "batch_size": 1
            }
        },
        "6": {
            "class_type": "KSampler",
            "inputs": {
                "model": ["2", 0],
                "positive": ["3", 0],
                "negative": ["4", 0],
                "latent_image": ["5", 0],
                "seed": seed,
                "steps": 30,
                "cfg": 7.0,
                "sampler_name": "euler_ancestral",
                "scheduler": "normal",
                "denoise": 1.0
            }
        },
        "7": {
            "class_type": "VAEDecode",
            "inputs": {
                "samples": ["6", 0],
                "vae": ["1", 2]
            }
        },
        "8": {
            "class_type": "SaveImage",
            "inputs": {
                "images": ["7", 0],
                "filename_prefix": f"v9_{filename_prefix}"
            }
        }
    }


def queue_prompt(workflow: dict) -> str:
    payload = json.dumps({"prompt": workflow}).encode("utf-8")
    req = urllib.request.Request(
        f"{COMFYUI_URL}/prompt",
        data=payload,
        headers={"Content-Type": "application/json"}
    )
    resp = urllib.request.urlopen(req)
    return json.loads(resp.read())["prompt_id"]


def wait_for_completion(prompt_id: str, timeout: int = 300) -> bool:
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history:
                status = history[prompt_id].get("status", {})
                if status.get("completed", False) or status.get("status_str") == "success":
                    return True
                for msg in status.get("messages", []):
                    if isinstance(msg, list) and len(msg) >= 2:
                        if msg[0] == "execution_error":
                            print(f"  ERROR: {json.dumps(msg[1], indent=2)[:500]}")
                            return False
                        if msg[0] == "execution_success":
                            return True
        except:
            pass
        time.sleep(2)
    print(f"  TIMEOUT after {timeout}s")
    return False


def find_output_file(filename_prefix: str) -> str:
    pattern = os.path.join(COMFYUI_OUTPUT, f"v9_{filename_prefix}_*.png")
    files = glob.glob(pattern)
    if files:
        files.sort(key=os.path.getmtime, reverse=True)
        return files[0]
    return ""


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    try:
        resp = urllib.request.urlopen(f"{COMFYUI_URL}/system_stats")
        stats = json.loads(resp.read())
        gpu = stats["devices"][0]["name"] if stats.get("devices") else "unknown"
        print(f"ComfyUI running: {stats['system']['comfyui_version']} on {gpu}")
    except Exception as e:
        print(f"ERROR: ComfyUI not reachable at {COMFYUI_URL}: {e}")
        sys.exit(1)

    results = []

    for i, (filename, pos_prompt, seed) in enumerate(TEXTURES):
        print(f"\n[{i+1}/7] Generating: {filename} (seed={seed})")

        workflow = build_workflow(pos_prompt, filename, seed)
        prompt_id = queue_prompt(workflow)
        print(f"  Queued: {prompt_id}")

        success = wait_for_completion(prompt_id, timeout=300)
        if not success:
            print(f"  FAILED: {filename}")
            results.append((filename, False, ""))
            continue

        time.sleep(1)
        src = find_output_file(filename)
        if not src:
            print(f"  WARNING: Output file not found")
            results.append((filename, False, ""))
            continue

        dst = os.path.join(OUTPUT_DIR, f"{filename}.png")
        shutil.copy2(src, dst)
        size_kb = os.path.getsize(dst) / 1024
        print(f"  SAVED: {dst} ({size_kb:.0f} KB)")
        results.append((filename, True, dst))

    print("\n" + "=" * 60)
    print("GENERATION SUMMARY (v9)")
    print("=" * 60)
    ok = sum(1 for _, s, _ in results if s)
    print(f"Success: {ok}/7")
    for name, success, path in results:
        status = "OK" if success else "FAIL"
        print(f"  [{status}] {name}" + (f" -> {path}" if path else ""))


if __name__ == "__main__":
    main()
