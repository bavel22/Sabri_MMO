"""
Generate RO-authentic decal textures via ComfyUI.
These are projected onto terrain as DBuffer decals for ground variety.
All textures need TRANSPARENT BACKGROUNDS (alpha channel) or dark-on-light
with luminance-based opacity in the material.
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image, ImageEnhance

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/decals"
os.makedirs(OUTPUT_DIR, exist_ok=True)

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

# Decals work best as dark details on light/white background
# The material uses inverted luminance for opacity
NEGATIVE = (
    "photorealistic, photograph, 3d render, plastic, glossy, "
    "person, character, face, text, watermark, "
    "perspective, sky, horizon, border, frame"
)

TEXTURES = [
    # === UNIVERSAL SCATTER ===
    ("T_Decal_LeafScatter_01", "scattered fallen leaves on white background, top-down view, hand-painted game art, brown and orange autumn leaves randomly scattered, sparse distribution, oil painting style, flat lighting", 0.10, 95001),
    ("T_Decal_LeafScatter_02", "scattered green and yellow leaves on white background, top-down view, hand-painted, spring leaves randomly scattered, sparse, oil painting style, flat lighting", 0.10, 95002),
    ("T_Decal_FlowerPatch_01", "small cluster of tiny wildflowers on white background, top-down view, hand-painted game art, muted purple and yellow wildflowers, sparse, oil painting style", 0.10, 95003),
    ("T_Decal_FlowerPatch_02", "small cluster of tiny white and pink flowers on white background, top-down view, hand-painted, dainty wildflowers, sparse, oil painting style", 0.10, 95004),
    ("T_Decal_GrassTuft_01", "dark green grass tufts on white background, top-down view, hand-painted game art, small clumps of thick grass, oil painting style", 0.10, 95005),
    ("T_Decal_MossPatch_01", "green moss patch on white background, top-down view, hand-painted, organic irregular moss shape, dark green, oil painting style", 0.10, 95006),
    ("T_Decal_MossPatch_02", "wet mossy stain on white background, top-down view, hand-painted, blue-green damp moss patch, organic edges, oil painting style", 0.10, 95007),
    ("T_Decal_MushroomCluster", "small mushroom cluster on white background, top-down view, hand-painted game art, 2-3 tiny brown mushrooms, oil painting style", 0.10, 95008),
    ("T_Decal_RootTendril", "surface tree root tendrils on white background, top-down view, hand-painted, dark brown thin roots spreading outward, oil painting style", 0.10, 95009),

    # === CRACKS AND WEAR ===
    ("T_Decal_StoneCracks_01", "stone surface cracks on white background, top-down view, hand-painted, thin dark crack lines spreading organically, oil painting style", 0.10, 95010),
    ("T_Decal_StoneCracks_02", "deep stone fracture on white background, top-down view, hand-painted, single large crack with smaller branches, dark lines, oil painting style", 0.10, 95011),
    ("T_Decal_DirtSmudge_01", "dirt smudge stain on white background, top-down view, hand-painted, brown organic dirt mark, soft edges, oil painting style", 0.10, 95012),
    ("T_Decal_ScuffMark_01", "worn scuff marks on white background, top-down view, hand-painted, gray-brown wear marks on stone, subtle, oil painting style", 0.10, 95013),

    # === PUDDLES AND MOISTURE ===
    ("T_Decal_Puddle_01", "small puddle on white background, top-down view, hand-painted game art, dark irregular water puddle shape, organic edges, oil painting style", 0.10, 95014),
    ("T_Decal_Puddle_02", "tiny rain puddles on white background, top-down view, hand-painted, several small dark water spots, organic shapes, oil painting style", 0.10, 95015),
    ("T_Decal_DampPatch", "damp wet ground patch on white background, top-down view, hand-painted, darker colored irregular wet area, subtle, oil painting style", 0.10, 95016),

    # === ZONE-SPECIFIC: DESERT ===
    ("T_Decal_SandRipple", "sand ripple wave pattern on white background, top-down view, hand-painted, subtle tan wave lines in sand, wind-formed, oil painting style", 0.08, 95020),
    ("T_Decal_SandDrift", "sand drift accumulation on white background, top-down view, hand-painted, golden sand pile shape with soft edges, desert wind deposit, oil painting style", 0.08, 95021),
    ("T_Decal_CrackedEarth", "dry cracked earth on white background, top-down view, hand-painted, polygon crack pattern in dried mud, dark lines, oil painting style", 0.08, 95022),
    ("T_Decal_BoneScatter", "small bone fragments on white background, top-down view, hand-painted game art, scattered small white bone pieces, desert remains, oil painting style", 0.10, 95023),

    # === ZONE-SPECIFIC: SNOW ===
    ("T_Decal_IcePatch", "ice patch on white background, top-down view, hand-painted, pale blue-white translucent ice, smooth with subtle cracks, oil painting style", 0.08, 95030),
    ("T_Decal_FrostPattern", "frost crystal pattern on white background, top-down view, hand-painted, delicate fern-like ice crystal pattern, pale blue-white, oil painting style", 0.08, 95031),
    ("T_Decal_SnowDrift", "snow drift patch on white background, top-down view, hand-painted, white snow accumulation shape with soft edges, oil painting style", 0.08, 95032),

    # === ZONE-SPECIFIC: VOLCANIC ===
    ("T_Decal_LavaCrack", "glowing lava crack on black background, top-down view, hand-painted game art, bright orange-red lava line with dark rock edges, oil painting style", 0.12, 95040),
    ("T_Decal_ScorchMark", "radial scorch burn mark on white background, top-down view, hand-painted, dark charred circular burn pattern, oil painting style", 0.10, 95041),
    ("T_Decal_SulfurDeposit", "yellow sulfur crystal deposit on white background, top-down view, hand-painted, bright yellow crystalline mineral patch, oil painting style", 0.10, 95042),

    # === ZONE-SPECIFIC: CURSED/UNDEAD ===
    ("T_Decal_BloodStain_01", "dried blood stain on white background, top-down view, hand-painted game art, dark red-brown blood splatter, organic shape, oil painting style", 0.10, 95050),
    ("T_Decal_BloodStain_02", "old blood pool on white background, top-down view, hand-painted, dark dried blood puddle shape, irregular edges, oil painting style", 0.10, 95051),
    ("T_Decal_BoneScatter_GH", "bone scatter on white background, top-down view, hand-painted, scattered pale bones and skull fragments, dungeon floor debris, oil painting style", 0.10, 95052),
    ("T_Decal_ClawMarks", "claw scratch marks on white background, top-down view, hand-painted, three parallel deep scratch lines, dark gouges, oil painting style", 0.10, 95053),
    ("T_Decal_SlimeTrail", "green slime trail on white background, top-down view, hand-painted, sickly green mucus trail, organic path, oil painting style", 0.10, 95054),

    # === ZONE-SPECIFIC: JAPANESE (AMATSU) ===
    ("T_Decal_CherryPetals", "cherry blossom petals scattered on white background, top-down view, hand-painted, pink and white flower petals randomly scattered, sparse, oil painting style", 0.08, 95060),
    ("T_Decal_BambooLeaves", "fallen bamboo leaves on white background, top-down view, hand-painted, long thin green bamboo leaves scattered, oil painting style", 0.08, 95061),

    # === ZONE-SPECIFIC: FOREST (MOSCOVIA) ===
    ("T_Decal_PineCones", "pine cones scattered on white background, top-down view, hand-painted game art, small brown pine cones randomly placed, sparse, oil painting style", 0.10, 95070),
    ("T_Decal_BirchLeaves", "fallen birch leaves on white background, top-down view, hand-painted, small yellow-green birch leaves scattered, autumn, oil painting style", 0.10, 95071),

    # === TRANSITION DETAILS ===
    ("T_Decal_RubbleScatter", "fallen rock rubble on white background, top-down view, hand-painted game art, small gray stone fragments scattered, cliff debris, oil painting style", 0.10, 95080),
    ("T_Decal_GravelStrip", "gravel scatter strip on white background, top-down view, hand-painted, small pebbles in a loose strip shape, path edge, oil painting style", 0.10, 95081),
    ("T_Decal_ErosionLines", "water erosion lines on white background, top-down view, hand-painted, thin dark lines where water has carved paths, organic branching, oil painting style", 0.10, 95082),

    # === LIGHT AND SHADOW ===
    ("T_Decal_DappledLight", "dappled sunlight patches on white background, top-down view, hand-painted, soft bright circular light spots, tree canopy light, oil painting style", 0.08, 95090),
    ("T_Decal_ShadowPool", "soft shadow pool on white background, top-down view, hand-painted, dark soft-edged shadow patch, organic shape, ambient occlusion, oil painting style", 0.10, 95091),
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
    nodes["5"] = {"class_type": "EmptyLatentImage", "inputs": {"width": 512, "height": 512, "batch_size": 1}}
    nodes["6"] = {"class_type": "KSampler", "inputs": {"model": m, "positive": ["3", 0], "negative": ["4", 0], "latent_image": ["5", 0], "seed": seed, "steps": 40, "cfg": 6.5, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}}
    nodes["7"] = {"class_type": "VAEDecode", "inputs": {"samples": ["6", 0], "vae": ["1", 2]}}
    nodes["8"] = {"class_type": "SaveImage", "inputs": {"images": ["7", 0], "filename_prefix": "decal"}}
    return nodes

def main():
    print(f"Generating {len(TEXTURES)} decal textures at 512x512...\n")
    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ComfyUI not running!"); sys.exit(1)

    success = 0
    for i, (name, prompt, lora, seed) in enumerate(TEXTURES):
        path = os.path.join(OUTPUT_DIR, f"{name}.png")
        if os.path.exists(path):
            print(f"[{i+1}/{len(TEXTURES)}] {name}: exists")
            success += 1; continue
        print(f"[{i+1}/{len(TEXTURES)}] {name}")
        wf = build_workflow(prompt, NEGATIVE, seed, lora)
        pid = queue_prompt(wf)
        if not pid: continue
        hist = wait_for_completion(pid)
        if not hist: continue
        for nid, nout in hist.get("outputs", {}).items():
            for img in nout.get("images", []):
                download_image(img["filename"], img.get("subfolder", ""), path)
                print(f"  OK")
                success += 1; break
            break

    print(f"\nDONE: {success}/{len(TEXTURES)}")
    print(f"Output: {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
