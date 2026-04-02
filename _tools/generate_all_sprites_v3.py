"""
V3 MEGA SPRITE GENERATOR
Generates high-quality sprites for ALL zone scatter types.
Auto-checks quality, retries bad results, bottom-aligns all sprites.
Uses SDXL Base with "product render" prompts that proved to work.
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/sprites_v3"
os.makedirs(OUTPUT_DIR, exist_ok=True)

CHECKPOINT = "sd_xl_base_1.0.safetensors"
MAX_RETRIES = 3  # retry bad sprites up to 3 times with different seeds

NEGATIVE = (
    "photorealistic, photograph, blurry, low quality, worst quality, "
    "watermark, text, multiple objects, many, several, collection, "
    "tiling, repeating pattern, grid, collage, panel, split, "
    "frame, border, picture frame, ornamental, matted, "
    "background scenery, landscape, ground, soil, sky, shadow, "
    "black silhouette, technical drawing, diagram"
)

# All sprite definitions organized by zone
# (name, prompt, base_seed)
SPRITES = {
    "grassland": [
        ("S_Grass_Clump_A", "ONE small clump of green grass blades growing from dirt mound, product render, game asset, isolated on white background, centered, nothing else", 93001),
        ("S_Grass_Clump_B", "ONE small tuft of wild meadow grass, product render, game asset, olive green blades, isolated on white, centered", 93002),
        ("S_Grass_Clump_C", "ONE small patch of thick green grass, product render, game asset, lush green, isolated on white, centered", 93003),
        ("S_Grass_Tall", "ONE single tall green grass stalk with seed head, product render, game asset, isolated on white, centered", 93004),
        ("S_Flower_Daisy", "ONE single small white daisy with yellow center and green stem, product render, game asset, isolated on white, centered, no frame", 93010),
        ("S_Flower_Pink", "ONE single small pink wildflower with thin green stem, product render, game asset, isolated on white, centered", 93011),
        ("S_Flower_Purple", "ONE single small purple wildflower with green stem, product render, game asset, isolated on white, centered", 93012),
        ("S_Flower_Yellow", "ONE single small yellow buttercup flower with stem, product render, game asset, isolated on white, centered", 93013),
        ("S_Dandelion", "ONE single dandelion puffball on stem, product render, game asset, white fluffy seed head, isolated on white, centered", 93014),
        ("S_Clover", "ONE single small clover plant with three round leaves, product render, game asset, green, isolated on white, centered", 93015),
    ],
    "forest": [
        ("S_Fern_A", "ONE single small green fern frond, product render, game asset, bright green with leaflets, isolated on white, centered", 93101),
        ("S_Fern_B", "ONE single curled fiddlehead fern, product render, game asset, young green spiral fern, isolated on white, centered", 93102),
        ("S_Mushroom_Brown", "ONE single small brown mushroom, product render, game asset, tan cap on thin stem, isolated on white, centered", 93110),
        ("S_Mushroom_Red", "ONE single small red mushroom with white spots, product render, game asset, fairy tale toadstool, isolated on white, centered", 93111),
        ("S_Mushroom_White", "ONE single small white mushroom, product render, game asset, pale cap, isolated on white, centered", 93112),
        ("S_Leaf_Autumn_A", "ONE single fallen orange-brown autumn maple leaf, product render, game asset, isolated on white, centered", 93120),
        ("S_Leaf_Autumn_B", "ONE single fallen red autumn leaf, product render, game asset, curled edges, isolated on white, centered", 93121),
        ("S_Leaf_Green", "ONE single fresh green broad leaf, product render, game asset, oval shape with veins, isolated on white, centered", 93122),
        ("S_MossClump", "ONE small clump of green moss, product render, game asset, soft round moss patch, isolated on white, centered", 93130),
        ("S_PineCone", "ONE single brown pine cone, product render, game asset, small conifer cone, isolated on white, centered", 93131),
        ("S_Twig", "ONE single small brown twig with few leaves, product render, game asset, thin branch, isolated on white, centered", 93132),
    ],
    "desert": [
        ("S_DryGrass_A", "ONE single dried golden grass stalk, product render, game asset, straw colored, isolated on white, centered", 93201),
        ("S_DryGrass_B", "ONE small clump of dried yellow-brown desert grass, product render, game asset, isolated on white, centered", 93202),
        ("S_Cactus_Small", "ONE single small green cactus, product render, game asset, tiny desert cactus, isolated on white, centered", 93210),
        ("S_DesertRock", "ONE single small tan desert rock, product render, game asset, sandy brown stone, isolated on white, centered", 93211),
        ("S_DeadBranch", "ONE single dry dead branch, product render, game asset, gray-brown brittle twig, isolated on white, centered", 93212),
    ],
    "snow": [
        ("S_SnowClump", "ONE single small pile of white snow, product render, game asset, fluffy snow mound, isolated on white, centered", 93301),
        ("S_IceCrystal", "ONE single blue ice crystal shard, product render, game asset, translucent pale blue, isolated on white, centered", 93302),
        ("S_FrozenPlant", "ONE single frozen plant covered in frost, product render, game asset, ice-covered small shrub, isolated on white, centered", 93303),
    ],
    "beach": [
        ("S_Shell_A", "ONE single seashell, product render, game asset, cream colored spiral shell, isolated on white, centered", 93401),
        ("S_Shell_B", "ONE single scallop shell, product render, game asset, fan-shaped shell, isolated on white, centered", 93402),
        ("S_Starfish", "ONE single orange starfish, product render, game asset, five-pointed sea star, isolated on white, centered", 93403),
        ("S_CoralPiece", "ONE single small piece of pink coral, product render, game asset, branching coral fragment, isolated on white, centered", 93404),
        ("S_Seaweed", "ONE single strand of green seaweed, product render, game asset, dark kelp strand, isolated on white, centered", 93405),
    ],
    "volcanic": [
        ("S_LavaRock", "ONE single dark volcanic rock, product render, game asset, black basalt with rough surface, isolated on white, centered", 93501),
        ("S_ObsidianShard", "ONE single black obsidian shard, product render, game asset, glossy dark volcanic glass, isolated on white, centered", 93502),
        ("S_SulfurCrystal", "ONE single yellow sulfur crystal cluster, product render, game asset, bright yellow mineral, isolated on white, centered", 93503),
    ],
    "cursed": [
        ("S_Bone_A", "ONE single old bone, product render, game asset, pale yellowed bone, isolated on white, centered", 93601),
        ("S_Bone_B", "ONE single small animal skull, product render, game asset, pale white skull, isolated on white, centered", 93602),
        ("S_DeadGrass", "ONE single clump of dead gray-brown withered grass, product render, game asset, lifeless, isolated on white, centered", 93603),
        ("S_CandleStub", "ONE single short melted candle stub with wax drips, product render, game asset, ivory colored, isolated on white, centered", 93604),
    ],
    "amatsu": [
        ("S_CherryPetal_A", "ONE single pink cherry blossom petal, product render, game asset, soft pink, delicate curved shape, isolated on white, centered", 93701),
        ("S_CherryPetal_B", "ONE single light pink sakura petal, product render, game asset, pale pink, gentle curve, isolated on white, centered", 93702),
        ("S_BambooLeaf", "ONE single long narrow green bamboo leaf, product render, game asset, pointed ends, isolated on white, centered", 93703),
    ],
    "moscovia": [
        ("S_BirchLeaf", "ONE single small yellow-green birch leaf, product render, game asset, autumn birch, isolated on white, centered", 93801),
        ("S_Pinecone_B", "ONE single small spruce cone, product render, game asset, brown conifer cone, isolated on white, centered", 93802),
        ("S_Berry", "ONE single small red berry cluster with green leaf, product render, game asset, isolated on white, centered", 93803),
    ],
    "ruins": [
        ("S_StoneFrag", "ONE single broken stone fragment, product render, game asset, beige sandstone piece, isolated on white, centered", 93901),
        ("S_PotteryShard", "ONE single broken pottery shard, product render, game asset, terracotta piece, isolated on white, centered", 93902),
    ],
    "industrial": [
        ("S_MetalScrap", "ONE single small piece of rusty metal scrap, product render, game asset, orange-brown corroded iron, isolated on white, centered", 94001),
        ("S_GearPiece", "ONE single small broken gear cog, product render, game asset, dark metal gear fragment, isolated on white, centered", 94002),
    ],
    "cave": [
        ("S_Crystal_Blue", "ONE single blue-purple crystal cluster, product render, game asset, glowing amethyst crystal, isolated on white, centered", 94101),
        ("S_Crystal_Green", "ONE single green emerald crystal shard, product render, game asset, translucent green, isolated on white, centered", 94102),
        ("S_Stalagmite", "ONE single small brown stalagmite, product render, game asset, cave rock formation, isolated on white, centered", 94103),
    ],
    "dungeon": [
        ("S_ChainPiece", "ONE single rusty chain link piece, product render, game asset, dark iron chain, isolated on white, centered", 94201),
        ("S_Rubble", "ONE single pile of small stone rubble, product render, game asset, gray broken rock pieces, isolated on white, centered", 94202),
    ],
}


def queue_prompt(workflow):
    data = json.dumps({"prompt": workflow}).encode("utf-8")
    req = urllib.request.Request(f"{COMFYUI_URL}/prompt", data=data, headers={"Content-Type": "application/json"})
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read()).get("prompt_id")
    except urllib.error.HTTPError as e:
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

def build_workflow(prompt, seed):
    return {
        "1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CHECKPOINT}},
        "2": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": prompt}},
        "3": {"class_type": "CLIPTextEncode", "inputs": {"clip": ["1", 1], "text": NEGATIVE}},
        "4": {"class_type": "EmptyLatentImage", "inputs": {"width": 512, "height": 512, "batch_size": 1}},
        "5": {"class_type": "KSampler", "inputs": {"model": ["1", 0], "positive": ["2", 0], "negative": ["3", 0], "latent_image": ["4", 0], "seed": seed, "steps": 30, "cfg": 7.0, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}},
        "6": {"class_type": "VAEDecode", "inputs": {"samples": ["5", 0], "vae": ["1", 2]}},
        "7": {"class_type": "SaveImage", "inputs": {"images": ["6", 0], "filename_prefix": "sprite_v3"}},
    }

def process_alpha_and_align(img_path):
    """Convert white bg to alpha + shift content to bottom of image."""
    img = Image.open(img_path).convert("RGBA")
    arr = np.array(img, dtype=np.float32)

    # Alpha from whiteness
    whiteness = (arr[:,:,0] + arr[:,:,1] + arr[:,:,2]) / 3.0
    alpha = np.clip((200 - whiteness) / 50.0 * 255, 0, 255)
    arr[:,:,3] = alpha

    # Bottom-align: shift content down
    alpha_arr = arr[:,:,3]
    row_has_content = np.any(alpha_arr > 30, axis=1)

    if np.any(row_has_content):
        last_row = np.where(row_has_content)[0][-1]
        h = arr.shape[0]
        bottom_empty = h - 1 - last_row

        if bottom_empty > 5:
            shift = bottom_empty - 2
            new_arr = np.zeros_like(arr)
            new_arr[shift:, :, :] = arr[:h-shift, :, :]
            arr = new_arr

    Image.fromarray(arr.astype(np.uint8)).save(img_path)

def check_quality(img_path):
    """Check if sprite is usable. Returns (is_good, reason)."""
    img = Image.open(img_path).convert("RGBA")
    arr = np.array(img)
    alpha = arr[:,:,3]

    total_pixels = alpha.size
    transparent = (alpha == 0).sum()
    opaque = (alpha > 200).sum()

    # Too much transparency = nothing was generated
    if transparent / total_pixels > 0.95:
        return False, "too transparent (nothing generated)"

    # Too little transparency = background not removed (solid image)
    if transparent / total_pixels < 0.15:
        return False, "not enough transparency (bg not removed)"

    # Object too small (less than 5% of image is opaque)
    if opaque / total_pixels < 0.05:
        return False, "object too small"

    return True, "good"

def generate_sprite(name, prompt, seed, zone_dir):
    """Generate one sprite with quality checking and retries."""
    path = os.path.join(zone_dir, f"{name}.png")

    if os.path.exists(path):
        is_good, reason = check_quality(path)
        if is_good:
            return True

    for attempt in range(MAX_RETRIES):
        current_seed = seed + attempt * 100

        wf = build_workflow(prompt, current_seed)
        pid = queue_prompt(wf)
        if not pid:
            continue

        hist = wait_for_completion(pid)
        if not hist:
            continue

        for nid, nout in hist.get("outputs", {}).items():
            for img in nout.get("images", []):
                download_image(img["filename"], img.get("subfolder", ""), path)
                process_alpha_and_align(path)

                is_good, reason = check_quality(path)
                if is_good:
                    return True
                else:
                    if attempt < MAX_RETRIES - 1:
                        print(f"      retry {attempt+1}: {reason}")
                break
            break

    return False


def main():
    total = sum(len(sprites) for sprites in SPRITES.values())
    print(f"{'='*60}")
    print(f"  V3 MEGA SPRITE GENERATOR")
    print(f"  {len(SPRITES)} zones, {total} sprites")
    print(f"  Max {MAX_RETRIES} retries per sprite")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'='*60}\n")

    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ComfyUI not running!"); sys.exit(1)

    good = 0
    bad = 0
    idx = 0

    for zone_name, sprites in SPRITES.items():
        zone_dir = os.path.join(OUTPUT_DIR, zone_name)
        os.makedirs(zone_dir, exist_ok=True)

        print(f"\n=== {zone_name.upper()} ({len(sprites)} sprites) ===")

        for name, prompt, seed in sprites:
            idx += 1
            print(f"  [{idx}/{total}] {name}", end="")
            sys.stdout.flush()

            success = generate_sprite(name, prompt, seed, zone_dir)

            if success:
                print(f" OK")
                good += 1
            else:
                print(f" FAILED (after {MAX_RETRIES} retries)")
                bad += 1

    print(f"\n{'='*60}")
    print(f"  DONE: {good} good, {bad} failed out of {total}")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
