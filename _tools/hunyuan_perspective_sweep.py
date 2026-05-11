"""
Generate stronger-perspective tree inputs and sweep seeds for max depth.

Approach:
  - Generate 3 candidate input images with different prompt strategies
    (3/4 perspective, isometric 3D render, side+front composite)
  - Mask each with rembg
  - For each candidate, sweep 6 seeds on Hunyuan3D mesh-gen
  - Track depth ratio Z/max(X,Y) — target > 0.85 (close to a sphere/cylinder)
  - Save the winner

Outputs to C:/Sabri_MMO/3d art/hunyuan_test_output/perspective_sweep/
"""
import io, json, shutil, time, urllib.request, urllib.parse, uuid
from pathlib import Path
import trimesh
import numpy as np
from PIL import Image
from rembg import remove, new_session

COMFY_URL = "http://127.0.0.1:8188"
OUT_DIR = Path("C:/Sabri_MMO/3d art/hunyuan_test_output/perspective_sweep")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def post_prompt(workflow):
    data = json.dumps({"prompt": workflow, "client_id": str(uuid.uuid4())}).encode()
    req = urllib.request.Request(f"{COMFY_URL}/prompt", data=data,
                                  headers={"Content-Type": "application/json"}, method="POST")
    return json.loads(urllib.request.urlopen(req).read())["prompt_id"]


def wait_for(pid, label, timeout=300):
    start = time.time()
    while time.time() - start < timeout:
        h = json.loads(urllib.request.urlopen(f"{COMFY_URL}/history/{pid}").read())
        if pid in h:
            elapsed = time.time() - start
            if h[pid].get("status",{}).get("status_str") == "error":
                msgs = h[pid]["status"].get("messages", [])
                for m in msgs:
                    if m[0] == "execution_error":
                        print(f"  [{label}] ERROR: {m[1].get('exception_message','').strip()[:200]}")
                return None
            return elapsed
        time.sleep(2)
    return None


def fetch_image(filename, subfolder, type_):
    url = f"{COMFY_URL}/view?{urllib.parse.urlencode({'filename': filename, 'subfolder': subfolder, 'type': type_})}"
    return urllib.request.urlopen(url).read()


def upload_image(local_path, target_filename):
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
    req = urllib.request.Request(f"{COMFY_URL}/upload/image", data=payload,
                                  headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
                                  method="POST")
    urllib.request.urlopen(req)


def sdxl_workflow(positive, negative, prefix, seed=42):
    return {
        "1": {"class_type": "CheckpointLoaderSimple",
              "inputs": {"ckpt_name": "sd_xl_base_1.0.safetensors"}},
        "2": {"class_type": "CLIPTextEncode", "inputs": {"text": positive, "clip": ["1", 1]}},
        "3": {"class_type": "CLIPTextEncode", "inputs": {"text": negative, "clip": ["1", 1]}},
        "4": {"class_type": "EmptyLatentImage", "inputs": {"width": 1024, "height": 1024, "batch_size": 1}},
        "5": {"class_type": "KSampler",
              "inputs": {"seed": seed, "steps": 30, "cfg": 7.0,
                         "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0,
                         "model": ["1", 0], "positive": ["2", 0], "negative": ["3", 0],
                         "latent_image": ["4", 0]}},
        "6": {"class_type": "VAEDecode", "inputs": {"samples": ["5", 0], "vae": ["1", 2]}},
        "7": {"class_type": "SaveImage", "inputs": {"filename_prefix": prefix, "images": ["6", 0]}},
    }


def hunyuan_workflow(input_filename, seed, label):
    return {
        "load_image": {"class_type": "Hy3D21LoadImageWithTransparency",
                       "inputs": {"image": input_filename}},
        "vae_load": {"class_type": "Hy3D21VAELoader",
                     "inputs": {"model_name": "hunyuan3d-vae-v2-1.ckpt"}},
        "mesh_gen": {"class_type": "Hy3DMeshGenerator",
                     "inputs": {"model": "hunyuan3d-dit-v2-1-fp16.ckpt",
                                "image": ["load_image", 2], "steps": 25,
                                "guidance_scale": 5.0, "seed": seed,
                                "attention_mode": "sdpa"}},
        "vae_decode": {"class_type": "Hy3D21VAEDecode",
                       "inputs": {"vae": ["vae_load", 0], "latents": ["mesh_gen", 0],
                                  "box_v": 1.01, "octree_resolution": 384,
                                  "num_chunks": 128000, "mc_level": 0.0, "mc_algo": "mc",
                                  "enable_flash_vdm": True, "force_offload": False}},
        "postprocess": {"class_type": "Hy3D21PostprocessMesh",
                        "inputs": {"trimesh": ["vae_decode", 0], "remove_floaters": True,
                                   "remove_degenerate_faces": True, "reduce_faces": False,
                                   "max_facenum": 200000, "smooth_normals": False}},
        "export": {"class_type": "Hy3D21ExportMesh",
                   "inputs": {"trimesh": ["postprocess", 0],
                              "filename_prefix": f"persp_{label}",
                              "file_format": "glb", "save_file": True}},
    }


def analyze(path):
    m = trimesh.load(path)
    g = list(m.geometry.values())[0] if isinstance(m, trimesh.Scene) else m
    e = g.extents
    roundness = float(e[2] / max(e[0], e[1]))
    return len(g.vertices), e.tolist(), roundness, float(g.volume), bool(g.is_watertight)


# Three input strategies — different prompts targeting volumetric output
INPUT_STRATEGIES = [
    {
        "name": "3quarter",
        "positive": (
            "low angle 3/4 perspective view of ONE single low-poly stylized fantasy tree, "
            "isometric angle, dramatic side lighting from upper left, strong shadow on trunk right side, "
            "visible canopy depth and volume, you can see the back of the tree, "
            "octane render, 3D game asset, isolated on plain white background, centered, "
            "Ragnarok Online style, hand-painted warm earth tones, no shadow on ground"
        ),
        "negative": (
            "front view, flat, orthographic, 2D illustration, 2D drawing, "
            "multiple objects, several trees, tiling, repeating, frame, border, "
            "perspective lines, horizon, scene, foreground objects"
        ),
    },
    {
        "name": "isometric_3d",
        "positive": (
            "isometric 3D rendered low-poly stylized tree, octane render, "
            "ambient occlusion, volumetric, full 3D model, you can see depth and volume, "
            "studio product render, photogrammetry style, 3D game asset, "
            "isolated on plain white background, centered, "
            "Ragnarok Online style, hand-painted warm earth tones"
        ),
        "negative": (
            "flat, 2D, illustration, drawing, sketch, painting, anime, "
            "multiple objects, tiling, frame, border, scene, perspective lines"
        ),
    },
    {
        "name": "rotated",
        "positive": (
            "ONE single stylized fantasy tree slightly rotated to show depth, "
            "looking down from 30 degrees above, you can see the top of the canopy curving away, "
            "trunk visible from front-left angle, voxel game asset 3D rendered, "
            "octane render with ambient occlusion, isolated on plain white background, centered, "
            "Ragnarok Online style, hand-painted warm earth tones, low-poly"
        ),
        "negative": (
            "front view, side view, profile, orthographic, flat, 2D illustration, "
            "multiple objects, tiling, frame, border, scene"
        ),
    },
]

print("=" * 70)
print("Generating 3 input image strategies + masking")
print("=" * 70)

# Use birefnet for masking (already cached)
print("\nLoading birefnet-general for masking...")
session = new_session("birefnet-general")

input_filenames = []  # (strategy_name, comfy_input_filename)

for strat in INPUT_STRATEGIES:
    print(f"\n--- Strategy: {strat['name']} ---")
    pid = post_prompt(sdxl_workflow(strat["positive"], strat["negative"], f"persp_input_{strat['name']}"))
    res = wait_for(pid, f"sdxl_{strat['name']}", timeout=120)
    if res is None:
        print(f"  SDXL failed for {strat['name']}")
        continue
    # Find saved image
    h = json.loads(urllib.request.urlopen(f"{COMFY_URL}/history/{pid}").read())
    img_info = h[pid]["outputs"]["7"]["images"][0]
    img_bytes = fetch_image(img_info["filename"], img_info["subfolder"], "output")
    raw_path = OUT_DIR / f"01_input_{strat['name']}_raw.png"
    raw_path.write_bytes(img_bytes)

    # Mask
    img = Image.open(io.BytesIO(img_bytes))
    masked = remove(img, session=session, alpha_matting=True,
                    alpha_matting_foreground_threshold=240,
                    alpha_matting_background_threshold=10)
    # Sanity check transparency
    arr = np.array(masked)
    trans_pct = (arr[..., 3] == 0).mean() * 100
    masked_path = OUT_DIR / f"02_input_{strat['name']}_masked.png"
    masked.save(masked_path)
    print(f"  generated + masked ({trans_pct:.0f}% transparent)")

    # Upload to ComfyUI
    cf_filename = f"persp_input_{strat['name']}.png"
    upload_image(masked_path, cf_filename)
    input_filenames.append((strat["name"], cf_filename))

print("\n" + "=" * 70)
print(f"Sweeping seeds on {len(input_filenames)} candidate inputs (6 seeds each)")
print("=" * 70)

SEEDS = [42, 100, 200, 500, 777, 1234]
results = []

for strat_name, cf_filename in input_filenames:
    for seed in SEEDS:
        label = f"{strat_name}_s{seed}"
        print(f"\n  {label}")
        pid = post_prompt(hunyuan_workflow(cf_filename, seed, label))
        elapsed = wait_for(pid, label, timeout=200)
        if elapsed is None:
            print(f"    FAILED")
            continue
        glbs = sorted(Path("C:/ComfyUI/output").glob(f"persp_{label}*.glb"),
                      key=lambda p: p.stat().st_mtime, reverse=True)
        if glbs:
            target = OUT_DIR / f"{label}.glb"
            shutil.copy(glbs[0], target)
            verts, extents, roundness, volume, watertight = analyze(target)
            results.append({
                "label": label, "strat": strat_name, "seed": seed,
                "verts": verts, "extents": extents,
                "roundness": roundness, "volume": volume, "watertight": watertight
            })
            print(f"    extents {[round(e,2) for e in extents]} | depth {roundness:.3f} | vol {volume:.3f}")

# Summary
print("\n" + "=" * 90)
print(f"{'config':<28} | {'verts':>7} | {'X':>5} | {'Y':>5} | {'Z':>5} | {'depth':>6} | {'volume':>6}")
print("-" * 90)
for r in sorted(results, key=lambda x: -x["roundness"]):
    e = r["extents"]
    print(f"{r['label']:<28} | {r['verts']:>7} | {e[0]:>5.2f} | {e[1]:>5.2f} | {e[2]:>5.2f} | {r['roundness']:>5.2f} | {r['volume']:>5.2f}")

if results:
    best = max(results, key=lambda r: r["roundness"])
    final_path = OUT_DIR.parent / "04_tree_BEST_PERSPECTIVE.glb"
    src_path = OUT_DIR / f"{best['label']}.glb"
    shutil.copy(src_path, final_path)
    print(f"\nWINNER: {best['label']} — depth {best['roundness']:.3f}, volume {best['volume']:.3f}")
    print(f"  saved to: {final_path}")
