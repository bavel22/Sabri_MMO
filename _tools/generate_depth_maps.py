"""
Generate Normal Maps, Depth Maps, and AO from existing diffuse textures.

Uses ComfyUI's AI preprocessors:
  - BAE-NormalMapPreprocessor (best quality normals)
  - MiDaS-NormalMapPreprocessor (alternative normals)
  - MiDaS-DepthMapPreprocessor (depth/height map)
  - DepthAnythingV2Preprocessor (alternative depth)

Also generates AO from depth via simple processing.

Processes ALL textures in a source directory.
"""

import json
import urllib.request
import urllib.parse
import time
import os
import sys
import glob
import numpy as np
from PIL import Image, ImageFilter

COMFYUI_URL = "http://127.0.0.1:8188"

# Source directories to process
SOURCE_DIRS = [
    "C:/Sabri_MMO/_tools/ground_texture_output",
    "C:/Sabri_MMO/_tools/ground_texture_output/allinone",
    "C:/Sabri_MMO/_tools/ground_texture_output/iter2",
]

OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/maps"
os.makedirs(OUTPUT_DIR, exist_ok=True)


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


def wait_for_completion(prompt_id, timeout=120):
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


def generate_map(src_path, map_type, preprocessor_class, output_path):
    """
    Load an image, run a preprocessor, save the result.

    Workflow:
      LoadImage → Preprocessor → SaveImage
    """
    workflow = {
        "1": {
            "class_type": "LoadImage",
            "inputs": {"image": os.path.basename(src_path)}
        },
        "2": {
            "class_type": preprocessor_class,
            "inputs": {"image": ["1", 0]}
        },
        "3": {
            "class_type": "SaveImage",
            "inputs": {
                "images": ["2", 0],
                "filename_prefix": f"map_{map_type}",
            }
        }
    }

    prompt_id = queue_prompt(workflow)
    if not prompt_id:
        return False

    history = wait_for_completion(prompt_id, timeout=120)
    if not history:
        return False

    for nid, nout in history.get("outputs", {}).items():
        for img_info in nout.get("images", []):
            download_image(
                img_info["filename"],
                img_info.get("subfolder", ""),
                output_path
            )
            return True
    return False


def generate_ao_from_depth(depth_path, ao_path):
    """
    Generate ambient occlusion from a depth map.
    AO = inverted, blurred, contrast-enhanced depth.
    Dark areas in depth (low/crevices) become dark in AO.
    """
    depth = Image.open(depth_path).convert("L")

    # Invert — low depth (crevices) should be dark in AO
    depth_arr = 255 - np.array(depth, dtype=np.float32)

    # Normalize to full range
    dmin, dmax = depth_arr.min(), depth_arr.max()
    if dmax > dmin:
        depth_arr = (depth_arr - dmin) / (dmax - dmin) * 255

    # Blur for soft AO
    ao_img = Image.fromarray(depth_arr.astype(np.uint8))
    ao_img = ao_img.filter(ImageFilter.GaussianBlur(radius=3))

    # Increase contrast — push midtones toward white, keep darks
    ao_arr = np.array(ao_img, dtype=np.float32)
    ao_arr = np.power(ao_arr / 255.0, 0.6) * 255  # gamma < 1 = brighten mids
    ao_arr = ao_arr.clip(0, 255).astype(np.uint8)

    Image.fromarray(ao_arr).save(ao_path)


def copy_to_comfyui_input(src_path):
    """Copy source image to ComfyUI input folder so LoadImage can find it."""
    input_dir = "C:/ComfyUI/input"
    os.makedirs(input_dir, exist_ok=True)
    dest = os.path.join(input_dir, os.path.basename(src_path))
    if not os.path.exists(dest):
        import shutil
        shutil.copy2(src_path, dest)
    return dest


def main():
    # Collect all PNG textures from source dirs
    all_textures = []
    for src_dir in SOURCE_DIRS:
        pngs = glob.glob(os.path.join(src_dir, "*.png"))
        all_textures.extend(pngs)

    # Filter out previews and maps
    all_textures = [t for t in all_textures if "preview" not in t.lower() and "maps" not in t.lower()]

    print(f"{'=' * 60}")
    print(f"  Depth/Normal/AO Map Generator")
    print(f"  Found {len(all_textures)} textures to process")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'=' * 60}\n")

    # Check ComfyUI
    try:
        urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except Exception:
        print("ERROR: ComfyUI not running!")
        sys.exit(1)

    success = 0
    total_maps = 0

    for i, tex_path in enumerate(all_textures):
        name = os.path.splitext(os.path.basename(tex_path))[0]
        print(f"[{i+1}/{len(all_textures)}] {name}")

        # Copy to ComfyUI input
        copy_to_comfyui_input(tex_path)

        # Generate Normal Map (BAE — best quality)
        normal_path = os.path.join(OUTPUT_DIR, f"{name}_Normal.png")
        if not os.path.exists(normal_path):
            total_maps += 1
            if generate_map(tex_path, "normal", "BAE-NormalMapPreprocessor", normal_path):
                print(f"  Normal: OK")
                success += 1
            else:
                print(f"  Normal: FAILED")
        else:
            print(f"  Normal: exists, skipping")

        # Generate Depth Map (MiDaS)
        depth_path = os.path.join(OUTPUT_DIR, f"{name}_Depth.png")
        if not os.path.exists(depth_path):
            total_maps += 1
            if generate_map(tex_path, "depth", "MiDaS-DepthMapPreprocessor", depth_path):
                print(f"  Depth: OK")
                success += 1

                # Generate AO from depth
                ao_path = os.path.join(OUTPUT_DIR, f"{name}_AO.png")
                generate_ao_from_depth(depth_path, ao_path)
                print(f"  AO: OK (derived from depth)")
            else:
                print(f"  Depth: FAILED")
        else:
            print(f"  Depth: exists, skipping")
            # Still generate AO if missing
            ao_path = os.path.join(OUTPUT_DIR, f"{name}_AO.png")
            if not os.path.exists(ao_path):
                generate_ao_from_depth(depth_path, ao_path)
                print(f"  AO: OK (derived)")

    print(f"\n{'=' * 60}")
    print(f"  DONE: {success} maps generated")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"")
    print(f"  Per texture you now have:")
    print(f"    {name}_Normal.png  — plug into Normal input")
    print(f"    {name}_Depth.png   — plug into Height (LB_HeightBlend)")
    print(f"    {name}_AO.png      — multiply with BaseColor for depth")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
