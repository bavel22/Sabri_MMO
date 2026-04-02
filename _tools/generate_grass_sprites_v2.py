"""
Grass sprite generation V2 — two approaches:
  Option A: SDXL Base model (general purpose, better at isolated objects)
  Option B: LayerDiffuse (native transparency, no background removal needed)

Tries Option B first (LayerDiffuse). Falls back to Option A if SDXL base is available.
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/grass_sprites_v2"
os.makedirs(OUTPUT_DIR, exist_ok=True)

SDXL_BASE = "sd_xl_base_1.0.safetensors"
ILLUSTRIOUS = "Illustrious-XL-v0.1.safetensors"

NEGATIVE = (
    "photorealistic, photograph, blurry, low quality, worst quality, "
    "watermark, text, multiple items, crowded, busy scene, background scenery, "
    "landscape, ground, soil, sky, shadow, frame, border"
)

SPRITES = [
    # (name, prompt, seed)
    ("S2_GrassBlade_01", "a single green grass blade, game asset, hand-painted style, simple shape, isolated on transparent background, centered", 90001),
    ("S2_GrassBlade_02", "a single tall dark green grass stalk, game asset, hand-painted, thin blade with pointed tip, isolated, centered", 90002),
    ("S2_GrassBlade_03", "a single short bright green grass blade, game asset, hand-painted, wide leaf shape, isolated, centered", 90003),
    ("S2_GrassClump_01", "a small clump of 3 green grass blades, game asset, hand-painted style, isolated on transparent background, centered", 90004),
    ("S2_GrassClump_02", "a small tuft of wild green grass, game asset, hand-painted, messy natural clump, isolated, centered", 90005),
    ("S2_Flower_Yellow", "a single small yellow wildflower with green stem, game asset, hand-painted style, simple daisy shape, isolated, centered", 90101),
    ("S2_Flower_Pink", "a single small pink flower with green stem, game asset, hand-painted, delicate petals, isolated, centered", 90102),
    ("S2_Flower_Purple", "a single small purple flower with green stem, game asset, hand-painted, simple shape, isolated, centered", 90103),
    ("S2_Flower_White", "a single small white flower with green stem, game asset, hand-painted, simple daisy, isolated, centered", 90104),
    ("S2_Dandelion", "a single yellow dandelion flower with stem, game asset, hand-painted, bright and cheerful, isolated, centered", 90105),
    ("S2_Leaf_Autumn", "a single fallen autumn leaf, game asset, hand-painted, orange-brown color, isolated on transparent background, centered", 90201),
    ("S2_Leaf_Green", "a single green leaf, game asset, hand-painted, simple broad shape, isolated, centered", 90202),
    ("S2_CherryPetal", "a single pink cherry blossom petal, game asset, hand-painted, soft pink, delicate curved shape, isolated, centered", 90203),
    ("S2_BambooLeaf", "a single long narrow green bamboo leaf, game asset, hand-painted, pointed ends, isolated, centered", 90204),
    ("S2_Fern", "a single small green fern frond, game asset, hand-painted, repeating leaflets pattern, isolated, centered", 90301),
    ("S2_DryGrass", "a single dried golden-brown grass stalk, game asset, hand-painted, straw colored and wilted, isolated, centered", 90401),
    ("S2_DeadGrass", "a single dead gray-brown grass blade, game asset, hand-painted, withered and lifeless, isolated, centered", 90402),
    ("S2_Mushroom", "a single small brown mushroom, game asset, hand-painted, round cap on thin stem, isolated, centered", 90501),
    ("S2_Clover", "a single small green clover plant, game asset, hand-painted, three round leaves, isolated, centered", 90502),
]


def queue_prompt(workflow):
    data = json.dumps({"prompt": workflow}).encode("utf-8")
    req = urllib.request.Request(f"{COMFYUI_URL}/prompt", data=data, headers={"Content-Type": "application/json"})
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read()).get("prompt_id")
    except urllib.error.HTTPError as e:
        print(f"    Error: {e.read().decode('utf-8', errors='replace')[:200]}")
        return None

def wait_for_completion(prompt_id, timeout=300):
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history: return history[prompt_id]
        except: pass
        time.sleep(2)
    return None

def download_image(filename, subfolder, save_path):
    params = urllib.parse.urlencode({"filename": filename, "subfolder": subfolder, "type": "output"})
    urllib.request.urlretrieve(f"{COMFYUI_URL}/view?{params}", save_path)

def check_checkpoint(name):
    try:
        resp = urllib.request.urlopen(f"{COMFYUI_URL}/object_info/CheckpointLoaderSimple")
        data = json.loads(resp.read())
        ckpts = data.get("CheckpointLoaderSimple", {}).get("input", {}).get("required", {}).get("ckpt_name", [[]])[0]
        return name in ckpts
    except:
        return False

def build_layerdiffuse_workflow(prompt, negative, seed, checkpoint):
    """Option B: LayerDiffuse — generates with native alpha transparency."""
    return {
        "1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": checkpoint}},
        "2": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": prompt}},
        "3": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": negative}},
        "4": {"class_type": "EmptyLatentImage", "inputs": {"width": 512, "height": 512, "batch_size": 1}},
        # Apply LayerDiffuse to model — generates with transparency
        "5": {"class_type": "LayeredDiffusionApply", "inputs": {
            "model": ["1", 0],
            "config": "SDXL, Attention Injection",
            "weight": 1.0,
        }},
        "6": {"class_type": "KSampler", "inputs": {
            "model": ["5", 0],
            "positive": ["2", 0], "negative": ["3", 0],
            "latent_image": ["4", 0],
            "seed": seed, "steps": 30, "cfg": 7.0,
            "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0,
        }},
        "7": {"class_type": "VAEDecode", "inputs": {"samples": ["6", 0], "vae": ["1", 2]}},
        # Decode RGBA — extracts the alpha channel from LayerDiffuse output
        "8": {"class_type": "LayeredDiffusionDecodeRGBA", "inputs": {
            "samples": ["6", 0],
            "images": ["7", 0],
            "sd_version": "SDXL",
            "sub_batch_size": 16,
        }},
        "9": {"class_type": "SaveImage", "inputs": {"images": ["8", 0], "filename_prefix": "sprite_ld"}},
    }

def build_sdxl_workflow(prompt, negative, seed):
    """Option A: SDXL base — white background, then remove."""
    return {
        "1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": SDXL_BASE}},
        "2": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": prompt + ", white background, product photo style"}},
        "3": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": negative + ", colored background, complex background"}},
        "4": {"class_type": "EmptyLatentImage", "inputs": {"width": 512, "height": 512, "batch_size": 1}},
        "5": {"class_type": "KSampler", "inputs": {
            "model": ["1", 0],
            "positive": ["2", 0], "negative": ["3", 0],
            "latent_image": ["4", 0],
            "seed": seed, "steps": 30, "cfg": 7.0,
            "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0,
        }},
        "6": {"class_type": "VAEDecode", "inputs": {"samples": ["5", 0], "vae": ["1", 2]}},
        "7": {"class_type": "SaveImage", "inputs": {"images": ["6", 0], "filename_prefix": "sprite_sdxl"}},
    }

def make_alpha_from_white(img_path):
    """Remove white background by converting to alpha."""
    img = Image.open(img_path).convert("RGBA")
    arr = np.array(img, dtype=np.float32)
    whiteness = (arr[:,:,0] + arr[:,:,1] + arr[:,:,2]) / 3.0
    alpha = np.clip((255 - whiteness) / 50.0 * 255, 0, 255)
    arr[:,:,3] = alpha
    Image.fromarray(arr.astype(np.uint8)).save(img_path)

def main():
    print(f"{'='*60}")
    print(f"  Grass Sprite Generator V2")
    print(f"  {len(SPRITES)} sprites, two approaches")
    print(f"{'='*60}\n")

    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ComfyUI not running!"); sys.exit(1)

    has_sdxl = check_checkpoint(SDXL_BASE)
    has_illustrious = check_checkpoint(ILLUSTRIOUS)

    print(f"  SDXL Base: {'YES' if has_sdxl else 'NO (downloading...)'}")
    print(f"  Illustrious: {'YES' if has_illustrious else 'NO'}")

    # Determine which approach to use
    use_layerdiffuse = True
    checkpoint = SDXL_BASE if has_sdxl else ILLUSTRIOUS

    print(f"  Strategy: LayerDiffuse with {checkpoint}\n")

    success = 0
    for i, (name, prompt, seed) in enumerate(SPRITES):
        path_ld = os.path.join(OUTPUT_DIR, f"{name}_LD.png")
        path_sdxl = os.path.join(OUTPUT_DIR, f"{name}_SDXL.png")

        if os.path.exists(path_ld) and os.path.exists(path_sdxl):
            print(f"[{i+1}/{len(SPRITES)}] {name}: both exist")
            success += 1
            continue

        print(f"[{i+1}/{len(SPRITES)}] {name}")

        # Try LayerDiffuse first
        if not os.path.exists(path_ld):
            print(f"  LayerDiffuse...")
            wf = build_layerdiffuse_workflow(prompt, NEGATIVE, seed, checkpoint)
            pid = queue_prompt(wf)
            if pid:
                hist = wait_for_completion(pid, timeout=120)
                if hist:
                    for nid, nout in hist.get("outputs", {}).items():
                        for img in nout.get("images", []):
                            download_image(img["filename"], img.get("subfolder", ""), path_ld)
                            print(f"    LD: OK")
                            break
                        break
                else:
                    print(f"    LD: timeout")
            else:
                print(f"    LD: failed to queue")

        # Also try SDXL white-bg approach if checkpoint available
        if has_sdxl and not os.path.exists(path_sdxl):
            print(f"  SDXL white-bg...")
            wf = build_sdxl_workflow(prompt, NEGATIVE, seed)
            pid = queue_prompt(wf)
            if pid:
                hist = wait_for_completion(pid, timeout=120)
                if hist:
                    for nid, nout in hist.get("outputs", {}).items():
                        for img in nout.get("images", []):
                            download_image(img["filename"], img.get("subfolder", ""), path_sdxl)
                            make_alpha_from_white(path_sdxl)
                            print(f"    SDXL: OK (alpha generated)")
                            break
                        break
                else:
                    print(f"    SDXL: timeout")

        success += 1

    print(f"\n{'='*60}")
    print(f"  DONE: {success}/{len(SPRITES)}")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"  Each sprite has _LD (LayerDiffuse) and/or _SDXL (white bg removed)")
    print(f"  Check both, pick the better ones")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()
