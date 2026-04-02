"""
Generate grass blade/flower/leaf SPRITE images via ComfyUI.
These are 2D images with transparent backgrounds, designed to be
applied as alpha-masked textures on billboard cross meshes.

This is the standard game industry approach for foliage — NOT 3D geometry.
Each image shows a single grass blade, flower, leaf etc. painted in RO style.
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/grass_sprites"
os.makedirs(OUTPUT_DIR, exist_ok=True)

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

NEGATIVE = (
    "photorealistic, photograph, 3d render, blurry, low quality, worst quality, "
    "watermark, text, multiple objects, scene, landscape, background, ground, "
    "soil, dirt, sky, shadow on ground"
)

# Each sprite: (name, prompt, lora_weight, seed)
SPRITES = [
    # === GRASS BLADES ===
    ("S_GrassBlade_01",
     "single grass blade on pure white background, hand-painted game art style, "
     "olive green grass blade with yellow-green highlights, tapered pointed tip, "
     "simple clean shape, oil painting style, centered, isolated object",
     0.15, 80001),
    ("S_GrassBlade_02",
     "single tall grass blade on pure white background, hand-painted game art, "
     "dark sage green grass stalk, thin and tall, slight bend at top, "
     "oil painting style, centered, isolated",
     0.15, 80002),
    ("S_GrassBlade_03",
     "single short wide grass blade on pure white background, hand-painted, "
     "bright yellow-green grass leaf, wide and short, rounded tip, "
     "oil painting style, centered, isolated",
     0.15, 80003),
    ("S_GrassClump_01",
     "small clump of 3-4 grass blades on pure white background, hand-painted game art, "
     "mixed green grass tufts growing together, olive and yellow-green, "
     "oil painting style, centered, isolated",
     0.15, 80004),
    ("S_GrassClump_02",
     "small cluster of wild grass on pure white background, hand-painted, "
     "dark green and brown dry grass bunch, messy natural look, "
     "oil painting style, centered, isolated",
     0.15, 80005),

    # === FLOWERS ===
    ("S_Flower_01",
     "single small wildflower on pure white background, hand-painted game art, "
     "tiny yellow daisy with green stem, simple flower shape, "
     "oil painting style, centered, isolated",
     0.12, 80101),
    ("S_Flower_02",
     "single small pink flower on pure white background, hand-painted game art, "
     "tiny rose-pink blossom with thin green stem, delicate petals, "
     "oil painting style, centered, isolated",
     0.12, 80102),
    ("S_Flower_03",
     "single small purple wildflower on pure white background, hand-painted, "
     "tiny lavender flower with green stem, simple 5-petal shape, "
     "oil painting style, centered, isolated",
     0.12, 80103),
    ("S_Dandelion_01",
     "single dandelion on pure white background, hand-painted game art, "
     "yellow dandelion flower on green stem, bright cheerful, "
     "oil painting style, centered, isolated",
     0.12, 80104),

    # === LEAVES ===
    ("S_FallenLeaf_01",
     "single fallen autumn leaf on pure white background, hand-painted game art, "
     "orange-brown maple-like leaf, curled edges, warm autumn color, "
     "oil painting style, centered, isolated",
     0.10, 80201),
    ("S_FallenLeaf_02",
     "single fallen green leaf on pure white background, hand-painted, "
     "dark green broad leaf, slightly wilted, "
     "oil painting style, centered, isolated",
     0.10, 80202),
    ("S_CherryPetal_01",
     "single cherry blossom petal on pure white background, hand-painted game art, "
     "soft pink flower petal, delicate curved shape, "
     "oil painting style, centered, isolated",
     0.10, 80203),
    ("S_BambooLeaf_01",
     "single bamboo leaf on pure white background, hand-painted game art, "
     "long narrow green bamboo leaf, pointed ends, "
     "oil painting style, centered, isolated",
     0.10, 80204),
    ("S_BirchLeaf_01",
     "single small birch leaf on pure white background, hand-painted, "
     "yellow-green birch leaf with serrated edge, autumn, "
     "oil painting style, centered, isolated",
     0.10, 80205),

    # === FERNS / WEEDS ===
    ("S_Fern_01",
     "single small fern frond on pure white background, hand-painted game art, "
     "dark green fern leaf with repeating leaflets, forest plant, "
     "oil painting style, centered, isolated",
     0.12, 80301),
    ("S_Weed_01",
     "single wild weed plant on pure white background, hand-painted game art, "
     "scraggly brown-green weed with thin stem and small leaves, "
     "oil painting style, centered, isolated",
     0.12, 80302),

    # === DRY / DEAD ===
    ("S_DryGrass_01",
     "single dry grass stalk on pure white background, hand-painted game art, "
     "golden-brown dried grass blade, straw colored, wilted, "
     "oil painting style, centered, isolated",
     0.12, 80401),
    ("S_DeadGrass_01",
     "single dead gray-brown grass blade on pure white background, hand-painted, "
     "withered dark gray grass, lifeless, "
     "oil painting style, centered, isolated",
     0.12, 80402),

    # === SEAWEED ===
    ("S_Seaweed_01",
     "single seaweed strand on pure white background, hand-painted game art, "
     "dark green-brown kelp strand, wavy organic shape, "
     "oil painting style, centered, isolated",
     0.10, 80501),
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

def build_workflow(prompt, negative, seed, lora):
    nodes = {"1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CHECKPOINT}}}
    m, cl = ["1", 0], ["1", 1]
    if lora > 0:
        nodes["2"] = {"class_type": "LoraLoader", "inputs": {"model": m, "clip": cl, "lora_name": LORA, "strength_model": lora, "strength_clip": lora}}
        m, cl = ["2", 0], ["2", 1]
    nodes["3"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": cl, "text": prompt}}
    nodes["4"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": cl, "text": negative}}
    # 512x512 for sprites — small objects don't need high res
    nodes["5"] = {"class_type": "EmptyLatentImage", "inputs": {"width": 512, "height": 512, "batch_size": 1}}
    nodes["6"] = {"class_type": "KSampler", "inputs": {"model": m, "positive": ["3", 0], "negative": ["4", 0], "latent_image": ["5", 0], "seed": seed, "steps": 40, "cfg": 6.5, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}}
    nodes["7"] = {"class_type": "VAEDecode", "inputs": {"samples": ["6", 0], "vae": ["1", 2]}}
    nodes["8"] = {"class_type": "SaveImage", "inputs": {"images": ["7", 0], "filename_prefix": "grass_sprite"}}
    return nodes

def make_alpha_from_white(img_path):
    """Convert white background to alpha transparency."""
    img = Image.open(img_path).convert("RGBA")
    arr = np.array(img, dtype=np.float32)

    # Calculate how "white" each pixel is (average of RGB closeness to 255)
    whiteness = (arr[:,:,0] + arr[:,:,1] + arr[:,:,2]) / 3.0

    # Pixels close to white (>220) become transparent
    # Pixels far from white (<180) become fully opaque
    # Smooth transition between
    alpha = np.clip((255 - whiteness) / 40.0 * 255, 0, 255)

    arr[:,:,3] = alpha
    result = Image.fromarray(arr.astype(np.uint8))
    result.save(img_path)

def main():
    print(f"Generating {len(SPRITES)} grass/flower/leaf sprites...\n")
    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ComfyUI not running!"); sys.exit(1)

    success = 0
    for i, (name, prompt, lora, seed) in enumerate(SPRITES):
        path = os.path.join(OUTPUT_DIR, f"{name}.png")
        if os.path.exists(path):
            print(f"[{i+1}/{len(SPRITES)}] {name}: exists")
            success += 1; continue
        print(f"[{i+1}/{len(SPRITES)}] {name}")
        wf = build_workflow(prompt, NEGATIVE, seed, lora)
        pid = queue_prompt(wf)
        if not pid: continue
        hist = wait_for_completion(pid)
        if not hist: continue
        for nid, nout in hist.get("outputs", {}).items():
            for img in nout.get("images", []):
                download_image(img["filename"], img.get("subfolder", ""), path)
                # Convert white background to alpha
                make_alpha_from_white(path)
                print(f"  OK (alpha generated)")
                success += 1; break
            break

    print(f"\nDONE: {success}/{len(SPRITES)}")
    print(f"Output: {OUTPUT_DIR}")
    print(f"Next: Run blender_make_sprite_meshes.py to create billboard cross meshes")
    print(f"Then: Import into UE5 with alpha-masked materials")

if __name__ == "__main__":
    main()
