"""Validate texture + decimate steps on the existing tree mesh before launching the overnight run."""
import json, shutil, sys, time, urllib.request, uuid
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)
COMFY_URL = "http://127.0.0.1:8188"

# Existing assets
MESH_PATH = "C:/Sabri_MMO/3d art/hunyuan_test_output/04_tree_BEST_PERSPECTIVE.glb"
INPUT_FILENAME = "persp_input_rotated.png"  # already uploaded
OUT_DIR = Path("C:/Sabri_MMO/3d art/hunyuan_test_output")


def post(workflow):
    data = json.dumps({"prompt": workflow, "client_id": str(uuid.uuid4())}).encode()
    req = urllib.request.Request(f"{COMFY_URL}/prompt", data=data,
                                  headers={"Content-Type": "application/json"}, method="POST")
    try:
        return json.loads(urllib.request.urlopen(req, timeout=30).read())["prompt_id"]
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", "ignore")
        print(f"POST failed: {e.code} {e.reason}\n{body[:1500]}")
        raise


def wait(pid, label, timeout=600):
    start = time.time()
    while time.time() - start < timeout:
        try:
            h = json.loads(urllib.request.urlopen(f"{COMFY_URL}/history/{pid}", timeout=10).read())
        except Exception:
            time.sleep(3); continue
        if pid in h:
            elapsed = time.time() - start
            st = h[pid].get("status", {})
            if st.get("status_str") == "error":
                msgs = st.get("messages", [])
                for m in msgs:
                    if m[0] == "execution_error":
                        e = m[1]
                        print(f"  [{label}] ERROR after {elapsed:.1f}s")
                        print(f"    node {e.get('node_id')} ({e.get('node_type')}): {e.get('exception_message','').strip()[:500]}")
                return None
            print(f"  [{label}] DONE in {elapsed:.1f}s")
            return h[pid], elapsed
        time.sleep(2)
    print(f"  [{label}] TIMEOUT")
    return None


# ============================================================================
# Step 1: Texture
# ============================================================================
print("=" * 60)
print("STEP 1: Texture the tree mesh (multi-view bake + inpaint)")
print("=" * 60)

texture_wf = {
    "load_mesh": {"class_type": "Hy3D21LoadMesh",
                  "inputs": {"glb_path": MESH_PATH}},
    "uv_wrap": {"class_type": "Hy3D21MeshUVWrap",
                "inputs": {"trimesh": ["load_mesh", 0]}},
    "load_image": {"class_type": "Hy3D21LoadImageWithTransparency",
                   "inputs": {"image": INPUT_FILENAME}},
    "camera_config": {"class_type": "Hy3D21CameraConfig",
                      "inputs": {"camera_azimuths": "0, 90, 180, 270, 0, 180",
                                 "camera_elevations": "0, 0, 0, 0, 90, -90",
                                 "view_weights": "1, 0.5, 1, 0.5, 1, 1",
                                 "ortho_scale": 1.1}},
    "multi_views": {"class_type": "Hy3DMultiViewsGenerator",
                    "inputs": {"trimesh": ["uv_wrap", 0],
                               "camera_config": ["camera_config", 0],
                               "image": ["load_image", 2],
                               "view_size": 768, "steps": 10,
                               "guidance_scale": 3.0, "texture_size": 1024,
                               "unwrap_mesh": False, "seed": 42}},
    "bake": {"class_type": "Hy3DBakeMultiViews",
             "inputs": {"pipeline": ["multi_views", 0],
                        "camera_config": ["camera_config", 0],
                        "albedo": ["multi_views", 1],
                        "mr": ["multi_views", 2]}},
    "inpaint": {"class_type": "Hy3DInPaint",
                "inputs": {"pipeline": ["bake", 0],
                           "albedo": ["bake", 1],
                           "albedo_mask": ["bake", 2],
                           "mr": ["bake", 3],
                           "mr_mask": ["bake", 4],
                           "output_mesh_name": "test_textured_tree"}},
}

pid = post(texture_wf)
print(f"  prompt_id: {pid}")
result = wait(pid, "texture", timeout=600)
if not result:
    sys.exit(1)

# Find the textured GLB output
glbs = sorted(Path("C:/ComfyUI/output").rglob("test_textured_tree*.glb"),
              key=lambda p: p.stat().st_mtime, reverse=True)
if not glbs:
    print("ERROR: No textured GLB found. Searching for any recent GLB...")
    glbs = sorted(Path("C:/ComfyUI/output").glob("*.glb"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    print(f"  Latest 5 GLBs:")
    for g in glbs[:5]:
        print(f"    {g}  ({time.strftime('%H:%M:%S', time.localtime(g.stat().st_mtime))})")
    sys.exit(1)

textured_path = OUT_DIR / "05_tree_textured.glb"
shutil.copy(glbs[0], textured_path)
print(f"  saved: {textured_path}  ({textured_path.stat().st_size/1024:.0f} KB)")


# ============================================================================
# Step 2: Decimate
# ============================================================================
print()
print("=" * 60)
print("STEP 2: Decimate to 5000 faces")
print("=" * 60)

decimate_wf = {
    "load_mesh": {"class_type": "Hy3D21LoadMesh",
                  "inputs": {"glb_path": str(textured_path)}},
    "decimate": {"class_type": "Hy3D21MeshlibDecimate",
                 "inputs": {"trimesh": ["load_mesh", 0],
                            "subdivideParts": 1,
                            "target_face_num": 5000,
                            "strategy": "MinimizeError",
                            "optimizeVertexPos": True,
                            "collapseNearNotFlippable": False,
                            "decimateBetweenParts": False,
                            "minFacesInPart": 100}},
    "export": {"class_type": "Hy3D21ExportMesh",
               "inputs": {"trimesh": ["decimate", 0],
                          "filename_prefix": "test_decimated_tree",
                          "file_format": "glb", "save_file": True}},
}

pid = post(decimate_wf)
print(f"  prompt_id: {pid}")
result = wait(pid, "decimate", timeout=180)
if not result:
    sys.exit(1)

glbs = sorted(Path("C:/ComfyUI/output").glob("test_decimated_tree*.glb"),
              key=lambda p: p.stat().st_mtime, reverse=True)
if not glbs:
    print("ERROR: No decimated GLB found")
    sys.exit(1)

decimated_path = OUT_DIR / "06_tree_decimated.glb"
shutil.copy(glbs[0], decimated_path)


# ============================================================================
# Analyze
# ============================================================================
print()
print("=" * 60)
print("RESULTS — Tree progression (untextured -> textured -> decimated)")
print("=" * 60)
import trimesh
for label, path in [("04 untextured (orig)", "04_tree_BEST_PERSPECTIVE.glb"),
                    ("05 textured",         "05_tree_textured.glb"),
                    ("06 decimated",        "06_tree_decimated.glb")]:
    p = OUT_DIR / path
    if not p.exists():
        continue
    m = trimesh.load(str(p))
    g = list(m.geometry.values())[0] if isinstance(m, trimesh.Scene) else m
    sz_kb = p.stat().st_size / 1024
    has_tex = bool(getattr(g.visual, 'material', None)) and (
        hasattr(g.visual.material, 'baseColorTexture') and g.visual.material.baseColorTexture is not None
        or hasattr(g.visual, 'uv') and g.visual.uv is not None
    )
    print(f"  {label:25} | {len(g.vertices):>7} verts | {len(g.faces):>7} faces | {sz_kb:>7.0f} KB | textured: {has_tex}")
