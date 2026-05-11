"""Retry just the Hunyuan3D mesh generation with mc_algo='mc' (standard marching cubes)."""
import json
import time
import urllib.request
import urllib.parse
import uuid
from pathlib import Path

COMFY_URL = "http://127.0.0.1:8188"
INPUT_FILENAME = "hunyuan_tree_input.png"  # already uploaded to ComfyUI/input/


def post_prompt(workflow):
    client_id = str(uuid.uuid4())
    data = json.dumps({"prompt": workflow, "client_id": client_id}).encode()
    req = urllib.request.Request(
        f"{COMFY_URL}/prompt",
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    return json.loads(urllib.request.urlopen(req).read())["prompt_id"]


def wait_for(prompt_id, label="job", timeout=600):
    start = time.time()
    while time.time() - start < timeout:
        r = urllib.request.urlopen(f"{COMFY_URL}/history/{prompt_id}")
        h = json.loads(r.read())
        if prompt_id in h:
            elapsed = time.time() - start
            status = h[prompt_id].get("status", {})
            if status.get("status_str") == "error":
                print(f"  [{label}] ERROR after {elapsed:.1f}s")
                for m in status.get("messages", []):
                    if m[0] == "execution_error":
                        e = m[1]
                        print(f"    node: {e.get('node_id')} ({e.get('node_type')})")
                        print(f"    msg: {e.get('exception_message','').strip()}")
                        tb = e.get("traceback", [])
                        if tb:
                            print(f"    last frame: {tb[-1].strip()}")
                return None
            print(f"  [{label}] DONE in {elapsed:.1f}s")
            return h[prompt_id]
        time.sleep(2)
    return None


# Build workflow with mc_algo='mc' instead of 'dmc' + lower octree_resolution as fallback
configs = [
    {"mc_algo": "mc", "octree_resolution": 384, "label": "mc/384"},
    {"mc_algo": "mc", "octree_resolution": 256, "label": "mc/256"},
    {"mc_algo": "dmc", "octree_resolution": 256, "label": "dmc/256"},
]

for cfg in configs:
    print(f"\n=== Trying {cfg['label']} ===")
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
                "octree_resolution": cfg["octree_resolution"],
                "num_chunks": 128000,
                "mc_level": 0.0,
                "mc_algo": cfg["mc_algo"],
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
                "filename_prefix": f"hunyuan_tree_{cfg['mc_algo']}_{cfg['octree_resolution']}",
                "file_format": "glb",
                "save_file": True,
            },
        },
    }

    prompt_id = post_prompt(workflow)
    print(f"  prompt_id: {prompt_id}")
    result = wait_for(prompt_id, cfg["label"], timeout=600)
    if result:
        print(f"  SUCCESS with {cfg['label']}")
        # Find the GLB
        out_dir = Path("C:/ComfyUI/output")
        glbs = sorted(
            out_dir.glob(f"hunyuan_tree_{cfg['mc_algo']}_{cfg['octree_resolution']}*.glb"),
            key=lambda p: p.stat().st_mtime,
            reverse=True,
        )
        if glbs:
            print(f"  GLB: {glbs[0]} ({glbs[0].stat().st_size/1024:.1f} KB)")
            target = Path("C:/Sabri_MMO/3d art/hunyuan_test_output") / f"03_tree_mesh_{cfg['mc_algo']}_{cfg['octree_resolution']}.glb"
            import shutil
            shutil.copy(glbs[0], target)
            print(f"  copied to: {target}")
        break
else:
    print("\nAll configs failed.")
