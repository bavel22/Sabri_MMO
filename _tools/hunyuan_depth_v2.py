"""Push depth further by combining good seeds with higher guidance + steps."""
import json, shutil, time, urllib.request, uuid
from pathlib import Path
import trimesh

COMFY_URL = "http://127.0.0.1:8188"
INPUT_FILENAME = "hunyuan_tree_input.png"
OUT_DIR = Path("C:/Sabri_MMO/3d art/hunyuan_test_output/depth_sweep")
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
            if h[pid].get("status",{}).get("status_str") == "error":
                return None
            return time.time() - start
        time.sleep(1)


def build_workflow(seed, steps, guidance, label):
    return {
        "load_image": {"class_type": "Hy3D21LoadImageWithTransparency",
                       "inputs": {"image": INPUT_FILENAME}},
        "vae_load": {"class_type": "Hy3D21VAELoader",
                     "inputs": {"model_name": "hunyuan3d-vae-v2-1.ckpt"}},
        "mesh_gen": {"class_type": "Hy3DMeshGenerator",
                     "inputs": {"model": "hunyuan3d-dit-v2-1-fp16.ckpt",
                                "image": ["load_image", 2], "steps": steps,
                                "guidance_scale": guidance, "seed": seed,
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
                              "filename_prefix": f"depthv2_{label}",
                              "file_format": "glb", "save_file": True}},
    }


def analyze(path):
    m = trimesh.load(path)
    geom = list(m.geometry.values())[0] if isinstance(m, trimesh.Scene) else m
    e = geom.extents
    roundness = e[2] / max(e[0], e[1])
    return len(geom.vertices), e.tolist(), float(roundness), float(geom.volume)


# Combined sweeps targeting depth
configs = [
    # (label, seed, steps, guidance)
    ("s100_g7.5_s50", 100, 50, 7.5),     # best seed + max guidance + max steps
    ("s100_g10_s50",  100, 50, 10.0),    # extreme guidance
    ("s100_g6_s40",   100, 40, 6.0),     # mid guidance
    ("s200_g7.5_s50", 200, 50, 7.5),     # second-best seed + max
    ("s500_g5_s25",   500, 25, 5.0),     # try a fresh seed
    ("s777_g5_s25",   777, 25, 5.0),     # another fresh seed
    ("s1234_g5_s25",  1234, 25, 5.0),    # another fresh seed
]

results = []

for label, seed, steps, guidance in configs:
    print(f"\n=== {label} (seed={seed}, steps={steps}, g={guidance}) ===")
    pid = post_prompt(build_workflow(seed, steps, guidance, label))
    elapsed = wait_for(pid, label)
    if elapsed is None:
        print(f"  FAILED")
        continue
    glbs = sorted(Path("C:/ComfyUI/output").glob(f"depthv2_{label}*.glb"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    if glbs:
        target = OUT_DIR / f"{label}.glb"
        shutil.copy(glbs[0], target)
        verts, extents, roundness, volume = analyze(target)
        results.append((label, elapsed, verts, extents, roundness, volume))
        print(f"  done in {elapsed:.1f}s | extents {[round(e,2) for e in extents]} | roundness {roundness:.3f} | vol {volume:.3f}")

# Also include previous winner for comparison
prev = OUT_DIR / "seed100_g5_s25.glb"
if prev.exists():
    verts, extents, roundness, volume = analyze(prev)
    results.append(("s100_g5_s25 (prev)", 35.0, verts, extents, roundness, volume))

print("\n" + "=" * 90)
print(f"{'config':<24} | {'time':>6} | {'verts':>7} | {'extents':>22} | {'depth':>6} | {'volume':>6}")
print("-" * 90)
for label, elapsed, verts, extents, roundness, volume in sorted(results, key=lambda r: -r[4]):
    extent_str = f"({extents[0]:.2f},{extents[1]:.2f},{extents[2]:.2f})"
    print(f"{label:<24} | {elapsed:>5.1f}s | {verts:>7} | {extent_str:>22} | {roundness:>5.2f} | {volume:>5.2f}")

if results:
    best = max(results, key=lambda r: r[4])
    print(f"\nBEST: {best[0]} (depth ratio {best[4]:.3f}, volume {best[5]:.3f})")
    print(f"  GLB: {OUT_DIR / (best[0].split(' ')[0] + '.glb')}")
