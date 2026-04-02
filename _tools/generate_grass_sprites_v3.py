"""
V3 — Re-generate failed sprites with better single-object prompts.
Key fixes:
  - "ONE single" emphasis
  - "centered in frame, nothing else"
  - "product render, isolated object"
  - "simple background, no pattern, no tiling"
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/grass_sprites_v3"
os.makedirs(OUTPUT_DIR, exist_ok=True)

CHECKPOINT = "sd_xl_base_1.0.safetensors"

NEGATIVE = (
    "photorealistic, photograph, blurry, low quality, worst quality, "
    "watermark, text, multiple objects, many, several, collection, "
    "tiling, repeating pattern, grid, collage, panel, split, "
    "background scenery, landscape, ground, soil, sky, shadow, "
    "frame, border, technical drawing, diagram, blueprint, "
    "black silhouette, black and white"
)

# Fixed prompts — much more specific about ONE single object
SPRITES = [
    # Grass blades — the ones that failed
    ("S3_GrassBlade_01",
     "ONE single green grass blade, product render style, hand-painted game asset, "
     "olive green blade with lighter tip, isolated object on plain white background, "
     "centered in frame, nothing else in image, simple clean shape",
     91001),
    ("S3_GrassBlade_02",
     "ONE single tall dark green grass stalk, product render, game asset, "
     "tall thin blade curving slightly, isolated on white background, "
     "centered, only one object, nothing else",
     91002),
    ("S3_GrassBlade_03",
     "ONE single wide short grass leaf, product render, game asset, "
     "bright yellow-green color, round tip, isolated on white, centered, alone",
     91003),

    # More grass clump variants (the winner)
    ("S3_GrassClump_02",
     "ONE small clump of green grass, product render, game asset, "
     "3-4 grass blades growing from small mound of dirt, "
     "isolated on white background, centered, single object only",
     91004),
    ("S3_GrassClump_03",
     "ONE small tuft of dark green wild grass, product render, game asset, "
     "messy natural grass bunch, isolated on white, centered, alone",
     91005),
    ("S3_GrassClump_04",
     "ONE small patch of meadow grass, product render, game asset, "
     "light green grass blades in small cluster, isolated on white, centered",
     91006),

    # Flowers — fix the tiling issue
    ("S3_Flower_Yellow",
     "ONE single small yellow daisy flower with green stem, product render, "
     "game asset, simple flower shape, isolated on white background, "
     "centered in frame, only one flower, nothing else",
     91101),
    ("S3_Flower_Pink",
     "ONE single small pink wildflower with thin stem, product render, "
     "game asset, delicate petals, isolated on white, centered, alone",
     91102),
    ("S3_Flower_Purple",
     "ONE single small purple flower with green stem, product render, "
     "game asset, simple 5-petal shape, isolated on white, centered, only one",
     91103),
    ("S3_Flower_White",
     "ONE single small white daisy with yellow center and green stem, "
     "product render, game asset, isolated on white, centered, alone",
     91104),

    # Leaves — fix multiple objects
    ("S3_Leaf_Autumn",
     "ONE single fallen autumn leaf, product render, game asset, "
     "orange-brown maple leaf shape, curled edges, isolated on white, "
     "centered, only one leaf, nothing else",
     91201),
    ("S3_Leaf_Green",
     "ONE single green broad leaf, product render, game asset, "
     "simple oval leaf shape with visible veins, isolated on white, centered, alone",
     91202),

    # Fern — fix the black silhouette
    ("S3_Fern",
     "ONE single small green fern frond, product render, game asset, "
     "bright green fern leaf with repeating leaflets, hand-painted style, "
     "isolated on white background, centered, only one",
     91301),

    # More cherry petal variants (the winner)
    ("S3_CherryPetal_02",
     "ONE single pink cherry blossom petal, product render, game asset, "
     "soft pastel pink, delicate curved petal shape, isolated on white, centered",
     91401),
    ("S3_CherryPetal_03",
     "ONE single light pink sakura petal, product render, game asset, "
     "translucent soft pink, gentle curve, isolated on white, centered, alone",
     91402),

    # More mushroom variants (the winner)
    ("S3_Mushroom_02",
     "ONE single small red mushroom with white spots, product render, "
     "game asset, fairy tale toadstool, cute round cap, isolated on white, centered",
     91501),
    ("S3_Mushroom_03",
     "ONE single small brown mushroom, product render, game asset, "
     "tan cap on thin white stem, simple shape, isolated on white, centered",
     91502),

    # Dry grass — fix panels/collage
    ("S3_DryGrass",
     "ONE single dried golden wheat stalk, product render, game asset, "
     "straw colored wilted grass blade, isolated on white, centered, alone",
     91601),

    # Bamboo leaf
    ("S3_BambooLeaf",
     "ONE single long narrow green bamboo leaf, product render, game asset, "
     "pointed ends, slightly curved, isolated on white, centered, only one",
     91701),

    # Seaweed
    ("S3_Seaweed",
     "ONE single green seaweed strand, product render, game asset, "
     "dark green wavy kelp shape, isolated on white, centered, alone",
     91801),
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

def wait_for_completion(prompt_id, timeout=180):
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

def build_workflow(prompt, negative, seed):
    return {
        "1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CHECKPOINT}},
        "2": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": prompt}},
        "3": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": negative}},
        "4": {"class_type": "EmptyLatentImage", "inputs": {"width": 512, "height": 512, "batch_size": 1}},
        "5": {"class_type": "KSampler", "inputs": {
            "model": ["1", 0], "positive": ["2", 0], "negative": ["3", 0],
            "latent_image": ["4", 0], "seed": seed, "steps": 30, "cfg": 7.0,
            "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}},
        "6": {"class_type": "VAEDecode", "inputs": {"samples": ["5", 0], "vae": ["1", 2]}},
        "7": {"class_type": "SaveImage", "inputs": {"images": ["6", 0], "filename_prefix": "sprite_v3"}},
    }

def make_alpha(img_path):
    img = Image.open(img_path).convert("RGBA")
    arr = np.array(img, dtype=np.float32)
    whiteness = (arr[:,:,0] + arr[:,:,1] + arr[:,:,2]) / 3.0
    alpha = np.clip((255 - whiteness) / 50.0 * 255, 0, 255)
    arr[:,:,3] = alpha
    Image.fromarray(arr.astype(np.uint8)).save(img_path)

def main():
    print(f"V3 Sprite Generation — {len(SPRITES)} sprites with SDXL Base\n")
    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ComfyUI not running!"); sys.exit(1)

    success = 0
    for i, (name, prompt, seed) in enumerate(SPRITES):
        path = os.path.join(OUTPUT_DIR, f"{name}.png")
        if os.path.exists(path):
            print(f"[{i+1}/{len(SPRITES)}] {name}: exists")
            success += 1; continue
        print(f"[{i+1}/{len(SPRITES)}] {name}")
        wf = build_workflow(prompt, NEGATIVE, seed)
        pid = queue_prompt(wf)
        if not pid: continue
        hist = wait_for_completion(pid)
        if not hist: print("  timeout"); continue
        for nid, nout in hist.get("outputs", {}).items():
            for img in nout.get("images", []):
                download_image(img["filename"], img.get("subfolder", ""), path)
                make_alpha(path)
                print(f"  OK")
                success += 1; break
            break

    print(f"\nDONE: {success}/{len(SPRITES)}")

if __name__ == "__main__":
    main()
