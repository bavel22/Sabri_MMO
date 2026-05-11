"""
End-to-end test of the Hunyuan3D 2.1 ComfyUI install:
  1. Generate a stylized tree image with SDXL Base via ComfyUI API
  2. Pre-mask the canopy with rembg (avoids Hunyuan's bg-removal failure on green)
  3. Run Hunyuan3D Mesh_Generation workflow on the masked image
  4. Save GLB output

Run: C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/hunyuan_test_tree.py
"""
import io
import json
import time
import urllib.request
import urllib.parse
import uuid
from pathlib import Path

COMFY_URL = "http://127.0.0.1:8188"
OUT_DIR = Path("C:/Sabri_MMO/3d art/hunyuan_test_output")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def post_prompt(workflow):
    """POST workflow to /prompt, return prompt_id."""
    client_id = str(uuid.uuid4())
    data = json.dumps({"prompt": workflow, "client_id": client_id}).encode()
    req = urllib.request.Request(
        f"{COMFY_URL}/prompt",
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read())["prompt_id"]
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", "ignore")
        print(f"  POST /prompt FAILED: {e.code} {e.reason}")
        print(f"  body: {body[:2000]}")
        raise


def wait_for(prompt_id, label="job", timeout=600):
    """Poll /history until prompt_id appears, return outputs."""
    start = time.time()
    last_status = None
    while time.time() - start < timeout:
        r = urllib.request.urlopen(f"{COMFY_URL}/history/{prompt_id}")
        h = json.loads(r.read())
        if prompt_id in h:
            elapsed = time.time() - start
            status = h[prompt_id].get("status", {})
            if status.get("status_str") == "error":
                print(f"  [{label}] ERROR after {elapsed:.1f}s")
                for m in status.get("messages", []):
                    print(f"    {m}")
                return None
            print(f"  [{label}] DONE in {elapsed:.1f}s")
            return h[prompt_id]
        time.sleep(2)
    print(f"  [{label}] TIMEOUT after {timeout}s")
    return None


def fetch_image(filename, subfolder, type_):
    url = f"{COMFY_URL}/view?{urllib.parse.urlencode({'filename': filename, 'subfolder': subfolder, 'type': type_})}"
    return urllib.request.urlopen(url).read()


def upload_image(local_path, target_filename):
    """Upload a file to ComfyUI's input folder via the /upload/image API."""
    with open(local_path, "rb") as f:
        body = f.read()
    boundary = "----comfyui-upload-" + uuid.uuid4().hex
    payload = b""
    payload += f"--{boundary}\r\n".encode()
    payload += f'Content-Disposition: form-data; name="image"; filename="{target_filename}"\r\n'.encode()
    payload += b"Content-Type: image/png\r\n\r\n"
    payload += body + b"\r\n"
    payload += f"--{boundary}\r\n".encode()
    payload += b'Content-Disposition: form-data; name="overwrite"\r\n\r\ntrue\r\n'
    payload += f"--{boundary}--\r\n".encode()
    req = urllib.request.Request(
        f"{COMFY_URL}/upload/image",
        data=payload,
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
        method="POST",
    )
    resp = urllib.request.urlopen(req)
    return json.loads(resp.read())


# ===========================================================================
# STEP 1: Generate stylized tree with SDXL Base
# ===========================================================================
print("\n=== STEP 1: Generate stylized tree with SDXL Base ===")

sdxl_workflow = {
    "1": {
        "class_type": "CheckpointLoaderSimple",
        "inputs": {"ckpt_name": "sd_xl_base_1.0.safetensors"},
    },
    "2": {
        "class_type": "CLIPTextEncode",
        "inputs": {
            "text": (
                "ONE single low-poly stylized fantasy tree, product render, "
                "game asset, isolated on plain white background, centered, "
                "nothing else in image, hand-painted style, warm earth tones, "
                "no shadow, simplified shape, Ragnarok Online style"
            ),
            "clip": ["1", 1],
        },
    },
    "3": {
        "class_type": "CLIPTextEncode",
        "inputs": {
            "text": (
                "multiple objects, many trees, several, tiling, repeating, grid, "
                "collage, frame, border, photorealistic, photograph, leaves on "
                "ground, foreground objects, background scenery, perspective, "
                "horizon, scene"
            ),
            "clip": ["1", 1],
        },
    },
    "4": {
        "class_type": "EmptyLatentImage",
        "inputs": {"width": 1024, "height": 1024, "batch_size": 1},
    },
    "5": {
        "class_type": "KSampler",
        "inputs": {
            "seed": 42,
            "steps": 30,
            "cfg": 7.0,
            "sampler_name": "euler",
            "scheduler": "normal",
            "denoise": 1.0,
            "model": ["1", 0],
            "positive": ["2", 0],
            "negative": ["3", 0],
            "latent_image": ["4", 0],
        },
    },
    "6": {
        "class_type": "VAEDecode",
        "inputs": {"samples": ["5", 0], "vae": ["1", 2]},
    },
    "7": {
        "class_type": "SaveImage",
        "inputs": {"filename_prefix": "hunyuan_test_tree_raw", "images": ["6", 0]},
    },
}

prompt_id = post_prompt(sdxl_workflow)
print(f"  prompt_id: {prompt_id}")
result = wait_for(prompt_id, "SDXL", timeout=300)
if not result:
    raise SystemExit("FAILED at SDXL step")

saved = result["outputs"]["7"]["images"][0]
img_filename = saved["filename"]
img_subfolder = saved["subfolder"]
print(f"  saved: {img_filename}")

img_bytes = fetch_image(img_filename, img_subfolder, "output")
raw_path = OUT_DIR / "01_tree_raw.png"
raw_path.write_bytes(img_bytes)
print(f"  copied to: {raw_path}")


# ===========================================================================
# STEP 2: Pre-mask the canopy with rembg (BiRefNet — handles green-on-white)
# ===========================================================================
print("\n=== STEP 2: Pre-mask with rembg (avoid canopy stripping) ===")

from PIL import Image
from rembg import remove, new_session

print("  loading birefnet-general model (first run downloads ~880MB)...")
session = new_session("birefnet-general")

img = Image.open(io.BytesIO(img_bytes))
print(f"  input size: {img.size}")
masked = remove(
    img,
    session=session,
    alpha_matting=True,
    alpha_matting_foreground_threshold=240,
    alpha_matting_background_threshold=10,
)

# Check transparency stats — sanity check the mask worked
import numpy as np
arr = np.array(masked)
alpha = arr[..., 3]
trans_pct = (alpha == 0).mean() * 100
opaque_pct = (alpha == 255).mean() * 100
print(f"  alpha stats: {trans_pct:.1f}% transparent, {opaque_pct:.1f}% opaque")
if trans_pct > 95:
    print("  WARNING: image is almost fully transparent — mask removed too much")
elif trans_pct < 15:
    print("  WARNING: image has very little transparency — mask kept too much")

masked_path = OUT_DIR / "02_tree_masked.png"
masked.save(masked_path)
print(f"  saved: {masked_path}")

# Upload to ComfyUI input via API (refreshes the dropdown)
input_filename = "hunyuan_tree_input.png"
print(f"  uploading to ComfyUI input as '{input_filename}'...")
upload_resp = upload_image(masked_path, input_filename)
print(f"  upload response: {upload_resp}")


# ===========================================================================
# STEP 3: Hunyuan3D mesh generation (shape only)
# ===========================================================================
print("\n=== STEP 3: Hunyuan3D mesh generation ===")

hunyuan_workflow = {
    "load_image": {
        "class_type": "Hy3D21LoadImageWithTransparency",
        "inputs": {"image": input_filename},
    },
    "vae_load": {
        "class_type": "Hy3D21VAELoader",
        "inputs": {"model_name": "hunyuan3d-vae-v2-1.ckpt"},
    },
    "mesh_gen": {
        "class_type": "Hy3DMeshGenerator",
        "inputs": {
            "model": "hunyuan3d-dit-v2-1-fp16.ckpt",
            "image": ["load_image", 2],   # output[2] = image_with_alpha
            "steps": 25,
            "guidance_scale": 5.0,
            "seed": 42,
            "attention_mode": "sdpa",
        },
    },
    "vae_decode": {
        "class_type": "Hy3D21VAEDecode",
        "inputs": {
            "vae": ["vae_load", 0],
            "latents": ["mesh_gen", 0],
            "box_v": 1.01,
            "octree_resolution": 384,
            "num_chunks": 128000,
            "mc_level": 0.0,
            "mc_algo": "dmc",
            "enable_flash_vdm": True,
            "force_offload": False,
        },
    },
    "postprocess": {
        "class_type": "Hy3D21PostprocessMesh",
        "inputs": {
            "trimesh": ["vae_decode", 0],
            "remove_floaters": True,
            "remove_degenerate_faces": True,
            "reduce_faces": False,
            "max_facenum": 200000,
            "smooth_normals": False,
        },
    },
    "export": {
        "class_type": "Hy3D21ExportMesh",
        "inputs": {
            "trimesh": ["postprocess", 0],
            "filename_prefix": "hunyuan_tree",
            "file_format": "glb",
            "save_file": True,
        },
    },
}

prompt_id = post_prompt(hunyuan_workflow)
print(f"  prompt_id: {prompt_id}")
result = wait_for(prompt_id, "Hunyuan3D", timeout=600)
if not result:
    raise SystemExit("FAILED at Hunyuan3D step")

# Find the GLB output (saved to ComfyUI/output/)
print("\n=== SUCCESS ===")
out_dir = Path("C:/ComfyUI/output")
glbs = sorted(out_dir.glob("hunyuan_tree*.glb"), key=lambda p: p.stat().st_mtime, reverse=True)
if glbs:
    latest_glb = glbs[0]
    target_glb = OUT_DIR / "03_tree_mesh.glb"
    import shutil
    shutil.copy(latest_glb, target_glb)
    print(f"  GLB: {latest_glb}")
    print(f"  copied to: {target_glb}")
    print(f"  size: {target_glb.stat().st_size/1024:.1f} KB")
else:
    print(f"  WARNING: no .glb file found in {out_dir} matching hunyuan_tree*.glb")
    print(f"  workflow outputs: {json.dumps(result.get('outputs', {}), indent=2, default=str)[:1000]}")

print(f"\nAll outputs: {OUT_DIR}")
print("Open the .glb in Blender, Windows 3D Viewer, or any GLB viewer to inspect.")
