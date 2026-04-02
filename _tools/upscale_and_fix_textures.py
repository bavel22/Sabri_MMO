"""
Upscale best ground textures from 1024->2048 using 4x-UltraSharp,
then downscale to 2048 (4x would give 4096, too large).
Also re-applies seamless cross-blend and generates new normal/depth/AO maps.

This doubles the world coverage per tile = half the visible repetition.
"""

import json
import urllib.request
import urllib.parse
import time
import os
import sys
import shutil
import numpy as np
from PIL import Image, ImageFilter, ImageEnhance

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/upscaled_2k"
MAPS_DIR = os.path.join(OUTPUT_DIR, "maps")
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(MAPS_DIR, exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

UPSCALE_MODEL = "4x-UltraSharp.pth"

# Best textures from all batches — upscale these
BEST_TEXTURES = [
    # Grass variants
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_LushGreen_03.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_LushGreen_02.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_LushGreen_01.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/allinone/T_Ground_RO_Mixed_B.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/allinone/T_Ground_RO_DirtGrass_A.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/allinone/T_Ground_RO_DenseGrass_A.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_MossFloor_01.png",
    # Dirt variants
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_EarthPath_01.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/T_Dirt_RO_v1.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_GreenBrown_01.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/allinone/T_Ground_RO_Mixed_A.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_RockyDirt_A.png",
    # Rock variants
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_StoneCliff_02.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2/T_StoneCliff_01.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/T_Rock_RO_v1.png",
    # Special
    "C:/Sabri_MMO/_tools/ground_texture_output/T_Sand_RO.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/T_Cobble_RO.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/T_Grass_Warm_RO_v1.png",
    "C:/Sabri_MMO/_tools/ground_texture_output/T_Grass_Warm_RO_v3.png",
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


def wait_for_completion(prompt_id, timeout=180):
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history:
                return history[prompt_id]
        except Exception:
            pass
        time.sleep(1)
    return None


def download_image(filename, subfolder, save_path):
    params = urllib.parse.urlencode({
        "filename": filename, "subfolder": subfolder, "type": "output"
    })
    urllib.request.urlretrieve(f"{COMFYUI_URL}/view?{params}", save_path)


def copy_to_comfyui_input(src_path):
    input_dir = "C:/ComfyUI/input"
    os.makedirs(input_dir, exist_ok=True)
    dest = os.path.join(input_dir, os.path.basename(src_path))
    shutil.copy2(src_path, dest)
    return dest


def upscale_texture(src_path, out_path):
    """Upscale with 4x model, then downscale to 2048x2048."""
    basename = os.path.basename(src_path)
    copy_to_comfyui_input(src_path)

    workflow = {
        # Load the image
        "1": {
            "class_type": "LoadImage",
            "inputs": {"image": basename}
        },
        # Load upscale model
        "2": {
            "class_type": "UpscaleModelLoader",
            "inputs": {"model_name": UPSCALE_MODEL}
        },
        # Upscale (1024->4096)
        "3": {
            "class_type": "ImageUpscaleWithModel",
            "inputs": {
                "upscale_model": ["2", 0],
                "image": ["1", 0],
            }
        },
        # Downscale to 2048 (half of 4096)
        "4": {
            "class_type": "ImageScaleBy",
            "inputs": {
                "image": ["3", 0],
                "upscale_method": "lanczos",
                "scale_by": 0.5,
            }
        },
        # Save
        "5": {
            "class_type": "SaveImage",
            "inputs": {
                "images": ["4", 0],
                "filename_prefix": "upscaled",
            }
        }
    }

    prompt_id = queue_prompt(workflow)
    if not prompt_id:
        return False

    history = wait_for_completion(prompt_id, timeout=180)
    if not history:
        return False

    for nid, nout in history.get("outputs", {}).items():
        for img_info in nout.get("images", []):
            download_image(img_info["filename"], img_info.get("subfolder", ""), out_path)
            return True
    return False


def make_seamless(img_path, border_frac=0.15):
    """Cross-blend with cosine ramp for seamless tiling."""
    img = np.array(Image.open(img_path), dtype=np.float32)
    H, W = img.shape[:2]
    shifted = np.roll(np.roll(img, W // 2, axis=1), H // 2, axis=0)

    bw = int(W * border_frac)
    bh = int(H * border_frac)
    ramp = lambda n: 0.5 * (1 + np.cos(np.linspace(0, np.pi, n)))

    wx = np.ones(W)
    wx[:bw] = ramp(bw)
    wx[-bw:] = ramp(bw)[::-1]
    wy = np.ones(H)
    wy[:bh] = ramp(bh)
    wy[-bh:] = ramp(bh)[::-1]

    mask = (wy[:, None] * wx[None, :])[:, :, np.newaxis]
    result = (img * mask + shifted * (1 - mask)).clip(0, 255).astype(np.uint8)
    Image.fromarray(result).save(img_path)


def generate_map(src_path, preprocessor, out_path):
    """Generate normal or depth map via ComfyUI preprocessor."""
    basename = os.path.basename(src_path)
    copy_to_comfyui_input(src_path)

    workflow = {
        "1": {"class_type": "LoadImage", "inputs": {"image": basename}},
        "2": {"class_type": preprocessor, "inputs": {"image": ["1", 0]}},
        "3": {"class_type": "SaveImage", "inputs": {"images": ["2", 0], "filename_prefix": "map"}},
    }
    prompt_id = queue_prompt(workflow)
    if not prompt_id:
        return False
    history = wait_for_completion(prompt_id, timeout=120)
    if not history:
        return False
    for nid, nout in history.get("outputs", {}).items():
        for img_info in nout.get("images", []):
            download_image(img_info["filename"], img_info.get("subfolder", ""), out_path)
            return True
    return False


def generate_ao(depth_path, ao_path):
    depth = Image.open(depth_path).convert("L")
    depth_arr = 255 - np.array(depth, dtype=np.float32)
    dmin, dmax = depth_arr.min(), depth_arr.max()
    if dmax > dmin:
        depth_arr = (depth_arr - dmin) / (dmax - dmin) * 255
    ao_img = Image.fromarray(depth_arr.astype(np.uint8))
    ao_img = ao_img.filter(ImageFilter.GaussianBlur(radius=4))
    ao_arr = np.power(np.array(ao_img, dtype=np.float32) / 255.0, 0.6) * 255
    Image.fromarray(ao_arr.clip(0, 255).astype(np.uint8)).save(ao_path)


def tile_preview(img_path, preview_path):
    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w * 3, h * 3))
    for r in range(3):
        for c in range(3):
            preview.paste(img, (c * w, r * h))
    if preview.width > 2048:
        ratio = 2048 / preview.width
        preview = preview.resize((2048, int(preview.height * ratio)), Image.LANCZOS)
    preview.save(preview_path, quality=90)


def main():
    textures = [t for t in BEST_TEXTURES if os.path.exists(t)]
    print(f"{'=' * 60}")
    print(f"  Upscale + Seamless + Maps Generator")
    print(f"  {len(textures)} textures -> 2048x2048")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'=' * 60}\n")

    try:
        urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except Exception:
        print("ERROR: ComfyUI not running!")
        sys.exit(1)

    for i, src in enumerate(textures):
        name = os.path.splitext(os.path.basename(src))[0]
        out_path = os.path.join(OUTPUT_DIR, f"{name}_2k.png")

        print(f"\n[{i+1}/{len(textures)}] {name}")

        # Step 1: Upscale
        if not os.path.exists(out_path):
            print(f"  Upscaling 1024->2048...")
            if not upscale_texture(src, out_path):
                print(f"  FAILED to upscale")
                continue
        else:
            print(f"  Already upscaled, skipping")

        # Step 2: Seamless fix
        print(f"  Applying seamless cross-blend...")
        make_seamless(out_path)

        # Step 3: Tile preview
        tile_preview(out_path, os.path.join(PREVIEW_DIR, f"{name}_2k_3x3.jpg"))
        print(f"  Preview saved")

        # Step 4: Normal map
        normal_path = os.path.join(MAPS_DIR, f"{name}_2k_Normal.png")
        if not os.path.exists(normal_path):
            print(f"  Generating normal map...")
            generate_map(out_path, "BAE-NormalMapPreprocessor", normal_path)
        else:
            print(f"  Normal map exists")

        # Step 5: Depth map
        depth_path = os.path.join(MAPS_DIR, f"{name}_2k_Depth.png")
        if not os.path.exists(depth_path):
            print(f"  Generating depth map...")
            if generate_map(out_path, "MiDaS-DepthMapPreprocessor", depth_path):
                # Step 6: AO from depth
                ao_path = os.path.join(MAPS_DIR, f"{name}_2k_AO.png")
                generate_ao(depth_path, ao_path)
                print(f"  AO generated")
        else:
            print(f"  Depth map exists")

        print(f"  DONE: {name}_2k.png (2048x2048)")

    print(f"\n{'=' * 60}")
    print(f"  ALL DONE")
    print(f"  2K textures: {OUTPUT_DIR}")
    print(f"  Maps: {MAPS_DIR}")
    print(f"  Previews: {PREVIEW_DIR}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
