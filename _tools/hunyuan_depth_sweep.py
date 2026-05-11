"""
Sweep Hunyuan3D mesh-gen parameters to find a deeper/rounder mesh.
Same tree input, different seeds + guidance + steps.

Outputs all variations to C:/Sabri_MMO/3d art/hunyuan_test_output/depth_sweep/
"""
import json
import shutil
import time
import urllib.request
import urllib.parse
import uuid
from pathlib import Path

COMFY_URL = "http://127.0.0.1:8188"
INPUT_FILENAME = "hunyuan_tree_input.png"
OUT_DIR = Path("C:/Sabri_MMO/3d art/hunyuan_test_output/depth_sweep")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def post_prompt(workflow):
    client_id = str(uuid.uuid4())
    data = json.dumps({"prompt": workflow, "client_id": client_id}).encode()
    req = urllib.request.Request(f"{COMFY_URL}/prompt", data=data,
                                  headers={"Content-Type": "application/json"}, method="POST")
    return json.loads(urllib.request.urlopen(req).read())["prompt_id"]


def wait_for(prompt_id, label, timeout=300):
    start = time.time()
    while time.time() - start < timeout:
        h = json.loads(urllib.request.urlopen(f"{COMFY_URL}/history/{prompt_id}").read())
        if prompt_id in h:
            elapsed = time.time() - start
            if h[prompt_id].get("status", {}).get("status_str") == "error":
                msgs = h[prompt_id]["status"].get("messages", [])
                for m in msgs:
                    if m[0] == "execution_error":
                        print(f"  [{label}] ERROR: {m[1].get('exception_message','').strip()}")
                return None
            return h[prompt_id], elapsed
        time.sleep(1)
    return None


def analyze_glb(path):
    """Return (verts, faces, extents, watertight)."""
    import trimesh
    m = trimesh.load(path)
    if isinstance(m, trimesh.Scene):
        geom = list(m.geometry.values())[0]
    else:
        geom = m
    return len(geom.vertices), len(geom.faces), geom.extents.tolist(), geom.is_watertight


configs = [
    # (label, seed, steps, guidance_scale, octree_resolution)
    ("seed42_g5_s25",    42, 25, 5.0, 384),  # the original (baseline)
    ("seed100_g5_s25",  100, 25, 5.0, 384),
    ("seed200_g5_s25",  200, 25, 5.0, 384),
    ("seed42_g7.5_s50",  42, 50, 7.5, 384),  # higher guidance + more steps
    ("seed42_g5_s25_oct512", 42, 25, 5.0, 512),  # finer octree
    ("seed42_g3_s25",    42, 25, 3.0, 384),  # lower guidance (less constrained, more model "creativity")
]

results = []

for label, seed, steps, guidance, octree in configs:
    print(f"\n=== {label}: seed={seed}, steps={steps}, guidance={guidance}, octree={octree} ===")
    workflow = {
        "load_image": {
            "class_type": "Hy3D21LoadImageWithTransparency",
            "inputs": {"image": INPUT_FILENAME},
        },
        "vae_load": {
            "class_type": "Hy3D21VAELoader",
            "inputs": {"model_name": "hunyuan3d-vae-v2-1.ckpt"},
        },
        "mesh_gen": {
            "class_type": "Hy3DMeshGenerator",
            "inputs": {
                "model": "hunyuan3d-dit-v2-1-fp16.ckpt",
                "image": ["load_image", 2],
                "steps": steps,
                "guidance_scale": guidance,
                "seed": seed,
                "attention_mode": "sdpa",
            },
        },
        "vae_decode": {
            "class_type": "Hy3D21VAEDecode",
            "inputs": {
                "vae": ["vae_load", 0],
                "latents": ["mesh_gen", 0],
                "box_v": 1.01,
                "octree_resolution": octree,
                "num_chunks": 128000,
                "mc_level": 0.0,
                "mc_algo": "mc",  # NEVER dmc — broken on our compile
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
                "filename_prefix": f"depth_{label}",
                "file_format": "glb",
                "save_file": True,
            },
        },
    }

    pid = post_prompt(workflow)
    res = wait_for(pid, label, timeout=300)
    if not res:
        print(f"  FAILED")
        continue
    _, elapsed = res
    glbs = sorted(Path("C:/ComfyUI/output").glob(f"depth_{label}*.glb"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    if glbs:
        target = OUT_DIR / f"{label}.glb"
        shutil.copy(glbs[0], target)
        verts, faces, extents, watertight = analyze_glb(target)
        # Compute "roundness" — how close is depth (Z) to width (X)? 1.0 = perfect cube/sphere
        roundness = round(extents[2] / max(extents[0], extents[1]), 3)
        results.append((label, elapsed, verts, faces, extents, watertight, roundness))
        print(f"  done in {elapsed:.1f}s | {verts} verts | extents {[round(e,2) for e in extents]} | roundness Z/max(X,Y)={roundness}")

# Summary
print("\n" + "=" * 80)
print(f"{'config':<32} | {'time':>6} | {'verts':>8} | {'extents (X,Y,Z)':>30} | {'depth':>6}")
print("-" * 80)
for label, elapsed, verts, faces, extents, watertight, roundness in results:
    extent_str = f"({extents[0]:.2f},{extents[1]:.2f},{extents[2]:.2f})"
    print(f"{label:<32} | {elapsed:>5.1f}s | {verts:>8} | {extent_str:>30} | {roundness:>5.2f}")

# Pick the best by depth ratio
if results:
    best = max(results, key=lambda r: r[6])  # max roundness
    print(f"\nBEST: {best[0]} (roundness {best[6]})")
    print(f"  GLB: {OUT_DIR / (best[0] + '.glb')}")
