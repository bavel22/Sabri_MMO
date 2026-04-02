"""
Generate textures specifically matching the RO Classic reference screenshots.
Key features:
  - Warm yellow-green as PRIMARY color (golden undertone)
  - Soft large-scale splotchy patches (not fine detail)
  - Very flat, painterly — just color fields blending
  - Dark brown/green for darker forest-edge areas
  - Simple, unified feel — one color family
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image, ImageEnhance

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/ro_classic"
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(os.path.join(OUTPUT_DIR, "previews"), exist_ok=True)

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

NEGATIVE = (
    "photorealistic, photograph, 3d render, plastic, glossy, shiny, specular, "
    "metallic, blurry, low quality, worst quality, watermark, signature, text, "
    "dramatic lighting, high contrast, neon, vivid, saturated, cold blue, purple, "
    "person, character, face, items, objects, perspective, sky, horizon, "
    "side view, grid, symmetric, border, frame, vignette, "
    "individual leaves, grass blades, clover, shamrock, stems, flowers, "
    "pixel art, mosaic, repeating pattern, stripes, abstract"
)

TEXTURES = [
    # === WARM YELLOW-GREEN (the dominant RO grass color) ===
    ("T_RO_YellowGreen_A",
     "overhead view of painted ground surface, oil painting style, warm yellow-green and golden green color fields, soft splotchy patches like a sun-dappled meadow, muted warm palette with golden undertone, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90001),
    ("T_RO_YellowGreen_B",
     "overhead view of painted ground surface, oil painting style, pale yellow-green and warm chartreuse color fields blending softly, golden sunlit meadow palette, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90002),
    ("T_RO_YellowGreen_C",
     "overhead view of painted ground surface, oil painting style, warm lime green and golden yellow-green patches, bright cheerful meadow palette, soft organic blending, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90003),
    ("T_RO_YellowGreen_D",
     "overhead view of painted ground surface, oil painting style, muted yellow-green with warm ochre undertones, faded golden meadow, gentle color variation, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90004),

    # === DARKER GREEN (for variant blending — darker patches) ===
    ("T_RO_DarkGreen_A",
     "overhead view of painted ground surface, oil painting style, dark olive green and warm brown-green color fields, shaded forest-edge ground, darker muted green palette, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90101),
    ("T_RO_DarkGreen_B",
     "overhead view of painted ground surface, oil painting style, deep sage green and dark warm green, shadowy meadow edge, rich dark green with brown undertone, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90102),

    # === WARM BROWN-GREEN (transition/dirt areas) ===
    ("T_RO_BrownGreen_A",
     "overhead view of painted ground surface, oil painting style, warm brown-green and dark ochre earth, muddy path through meadow, brown dominant with green hints, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90201),
    ("T_RO_BrownGreen_B",
     "overhead view of painted ground surface, oil painting style, dark warm brown with olive green patches, forest floor near clearing, earthy brown-green palette, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.12, 90202),

    # === DARK CLIFF/ROCK (matching the dark rocky cliff in likethis.png) ===
    ("T_RO_DarkCliff_A",
     "painted dark rock cliff face, oil painting style, very dark brown and charcoal gray rough stone, deep cracks and rough hewn surface, dark natural cliff wall, hand-painted game art, matte, flat lighting, seamless tileable, full frame",
     0.15, 90301),
    ("T_RO_DarkCliff_B",
     "painted dark rocky surface, oil painting style, dark brown-gray rugged stone, natural cliff face with shadows in crevices, earthy dark palette, hand-painted game art, matte, flat lighting, seamless tileable, full frame",
     0.15, 90302),

    # === RICH VIVID GRASS (like likethis.png — more modern, vibrant) ===
    ("T_RO_VividGrass_A",
     "overhead view of painted lush ground surface, oil painting style, rich bright green with scattered yellow-green highlights, vibrant but not neon meadow, warm sunlit palette, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.10, 90401),
    ("T_RO_VividGrass_B",
     "overhead view of painted vibrant meadow ground, oil painting style, bright emerald green with golden yellow patches, lush spring meadow, warm vivid palette, hand-painted game texture, matte, flat lighting, seamless tileable, full frame",
     0.10, 90402),
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
    nodes["3"] = {"class_type": "SeamlessTile", "inputs": {"model": m, "tiling": "enable", "copy_model": "Make a copy"}}
    nodes["4"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": cl, "text": prompt}}
    nodes["5"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": cl, "text": negative}}
    nodes["6"] = {"class_type": "EmptyLatentImage", "inputs": {"width": 1024, "height": 1024, "batch_size": 1}}
    nodes["7"] = {"class_type": "KSampler", "inputs": {"model": ["3", 0], "positive": ["4", 0], "negative": ["5", 0], "latent_image": ["6", 0], "seed": seed, "steps": 60, "cfg": 6.5, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}}
    nodes["8"] = {"class_type": "CircularVAEDecode", "inputs": {"samples": ["7", 0], "vae": ["1", 2], "tiling": "enable"}}
    nodes["9"] = {"class_type": "SaveImage", "inputs": {"images": ["8", 0], "filename_prefix": "roclassic"}}
    return nodes

def postprocess(img_path, name):
    # Lighter desaturation for these — keep warmth
    img = Image.open(img_path)
    img = ImageEnhance.Color(img).enhance(0.90)  # only 10% desat
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= 1.04  # warmer red
    arr[:, :, 2] *= 0.96  # less blue
    arr = arr.clip(0, 255).astype(np.uint8)
    Image.fromarray(arr).save(img_path)
    # Preview
    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w*3, h*3))
    for r in range(3):
        for c in range(3):
            preview.paste(img, (c*w, r*h))
    if preview.width > 2048:
        ratio = 2048/preview.width
        preview = preview.resize((2048, int(preview.height*ratio)), Image.LANCZOS)
    preview.save(os.path.join(OUTPUT_DIR, "previews", f"{name}_3x3.jpg"), quality=90)

def main():
    print(f"RO Classic Ground Textures — {len(TEXTURES)} textures\n")
    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ERROR: ComfyUI not running!"); sys.exit(1)

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
                postprocess(path, name)
                print(f"  OK")
                success += 1; break
            break

    print(f"\nDONE: {success}/{len(TEXTURES)}")
    print(f"Output: {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
