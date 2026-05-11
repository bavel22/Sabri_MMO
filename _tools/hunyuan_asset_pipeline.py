"""
Master autonomous 3D asset generation pipeline for Sabri_MMO.

For each asset in ASSETS:
  1. Generate input image with rotated/perspective SDXL prompt
  2. Pre-mask with rembg BiRefNet
  3. Sweep N seeds on Hunyuan3D mesh-gen, pick best (depth_ratio × volume)
  4. Texture the best mesh (multi-view bake + inpaint)
  5. Decimate to ~5000 faces (Hy3D21MeshlibDecimate)
  6. Save all outputs to generated_assets/<category>/<id>/

Designed to run unattended for hours. Each asset is wrapped in try/except;
one failure does NOT kill the run. Progress logged to stdout AND
generated_assets/_status.json for live monitoring.

Run: C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/hunyuan_asset_pipeline.py
"""
import io
import json
import os
import shutil
import sys
import time
import traceback
import urllib.request
import urllib.parse
import uuid
from pathlib import Path

# Force unbuffered output for live monitoring of background runs
sys.stdout.reconfigure(line_buffering=True)

# ============================================================================
# CONFIG
# ============================================================================
COMFY_URL = "http://127.0.0.1:8188"
ROOT = Path("C:/Sabri_MMO/3d art/generated_assets")
STATUS_FILE = ROOT / "_status.json"
LOG_FILE = ROOT / "_pipeline.log"

# Mesh sweep settings
MESH_SEEDS = [42, 100, 777, 1234]   # 4 seeds per asset (~35s each = ~2.5 min sweep)
MESH_STEPS = 25
MESH_GUIDANCE = 5.0
MESH_OCTREE = 384

# Decimation target
DECIMATE_TARGET_FACES = 5000  # game-ready

# Texture settings
TEXTURE_VIEW_SIZE = 768
TEXTURE_STEPS = 10
TEXTURE_GUIDANCE = 3.0
TEXTURE_SIZE = 1024

# Per-asset retry on failure
MAX_RETRIES = 1

# ============================================================================
# ASSET DEFINITIONS
# ============================================================================
# Each asset has a "subject" — what the SDXL prompt describes
# The full prompt template adds the rotated/perspective wrapper around it
ASSETS = [
    # ----- VEGETATION (12) -----
    {"id": "tree_oak",         "category": "vegetation",   "subject": "fantasy oak tree with thick brown trunk and round green leafy canopy"},
    {"id": "tree_maple",       "category": "vegetation",   "subject": "fantasy maple tree with red and orange autumn leaves, twisted trunk"},
    {"id": "tree_pine",        "category": "vegetation",   "subject": "fantasy pine tree, tall coniferous evergreen with dark green needles, conical shape"},
    {"id": "tree_palm",        "category": "vegetation",   "subject": "fantasy palm tree with curved trunk and large fan-shaped fronds, tropical, oasis style"},
    {"id": "tree_bamboo",      "category": "vegetation",   "subject": "cluster of tall bamboo stalks, segmented green stems with sparse green leaves"},
    {"id": "tree_dead",        "category": "vegetation",   "subject": "dead twisted tree, gnarled gray bark, no leaves, spooky cursed forest, jagged branches"},
    {"id": "tree_sakura",      "category": "vegetation",   "subject": "fantasy cherry blossom tree, dark trunk with delicate pink flowering canopy"},
    {"id": "bush_round",       "category": "vegetation",   "subject": "round leafy fantasy bush, dense green foliage, low ground cover"},
    {"id": "bush_wide",        "category": "vegetation",   "subject": "wide spreading shrub, varied green leaves, organic bushy shape"},
    {"id": "mushroom_giant",   "category": "vegetation",   "subject": "giant fantasy mushroom with red cap and white spots, thick white stem, forest floor"},
    {"id": "flower_cluster",   "category": "vegetation",   "subject": "small cluster of yellow daisy flowers on green grass tuft, ground patch"},
    {"id": "cactus_desert",    "category": "vegetation",   "subject": "tall green saguaro cactus with two side arms, desert plant with small spines"},

    # ----- ARCHITECTURE TOWN (5) -----
    {"id": "house_prontera_small",  "category": "architecture", "subject": "small medieval European stone house with red clay tile roof, wooden door, two small windows, stone walls"},
    {"id": "house_prontera_medium", "category": "architecture", "subject": "medieval European two story stone house with red clay tile roof, chimney, wooden door, multiple windows"},
    {"id": "hut_payon",             "category": "architecture", "subject": "Korean traditional thatched roof hut, wooden frame, hay roof, paper sliding doors, raised platform"},
    {"id": "house_morroc",          "category": "architecture", "subject": "Middle Eastern adobe clay building, flat roof, small square windows, sandstone color, desert architecture"},
    {"id": "tent_morroc",           "category": "architecture", "subject": "desert nomad tent, striped fabric, tied rope at sides, sandy color, Bedouin style"},

    # ----- TOWN PROPS (8) -----
    {"id": "lamppost_medieval", "category": "props", "subject": "medieval iron lamppost, ornate metal frame, glass enclosed candle lantern at top, on stone base"},
    {"id": "sign_wooden",       "category": "props", "subject": "wooden sign on a tall post, blank rectangular plank, hand-carved frame, hanging or planted style"},
    {"id": "barrel_wooden",     "category": "props", "subject": "wooden barrel with iron bands, medieval cooper-made, light brown wood staves, small and round"},
    {"id": "crate_wooden",      "category": "props", "subject": "wooden shipping crate, square box with diagonal slats, light brown wood, medieval merchant style"},
    {"id": "sack_burlap",       "category": "props", "subject": "burlap sack tied at top with rope, cream colored, full of grain, medieval merchant good"},
    {"id": "cart_wooden",       "category": "props", "subject": "small wooden two wheeled cart with iron rim, flat platform, pull handle, medieval merchant cart"},
    {"id": "market_stall",      "category": "props", "subject": "small medieval market stall with striped fabric awning, wooden counter, four corner posts"},
    {"id": "fence_wooden",      "category": "props", "subject": "short section of wooden fence, three horizontal rails between two upright posts, weathered wood"},

    # ----- TERRAIN FEATURES (5) -----
    {"id": "rock_small",     "category": "terrain", "subject": "small fantasy boulder, rounded gray stone with mossy patches, single rock"},
    {"id": "rock_medium",    "category": "terrain", "subject": "medium sized fantasy rock, irregular gray stone shape, low-poly stylized facets"},
    {"id": "rock_large",     "category": "terrain", "subject": "large boulder, massive gray stone with crack lines, prominent rocky outcrop"},
    {"id": "rock_mossy",     "category": "terrain", "subject": "mossy stone covered in green moss patches, gray-green rock, forest ground feature"},
    {"id": "bridge_wooden",  "category": "terrain", "subject": "small wooden plank footbridge, two parallel beams with crossing planks, simple railings"},

    # ----- TOWN CENTERPIECES (3) -----
    {"id": "fountain_stone",  "category": "centerpiece", "subject": "ornate medieval stone fountain, circular basin, central pillar with water spout, classical design"},
    {"id": "statue_warrior",  "category": "centerpiece", "subject": "stone statue of armored warrior holding sword, on rectangular pedestal, weathered gray stone"},
    {"id": "well_wooden",     "category": "centerpiece", "subject": "medieval village well, round stone wall, wooden A-frame roof, hanging bucket on rope"},

    # ----- DUNGEON ASSETS (2) -----
    {"id": "pillar_broken",   "category": "dungeon", "subject": "broken classical stone pillar, fluted column with damaged top half, gothic ruined architecture"},
    {"id": "sarcophagus",     "category": "dungeon", "subject": "ancient stone sarcophagus, rectangular tomb with carved lid, gothic dungeon decoration"},
]


# ============================================================================
# COMFYUI API HELPERS
# ============================================================================
def post_prompt(workflow):
    data = json.dumps({"prompt": workflow, "client_id": str(uuid.uuid4())}).encode()
    req = urllib.request.Request(f"{COMFY_URL}/prompt", data=data,
                                  headers={"Content-Type": "application/json"}, method="POST")
    try:
        return json.loads(urllib.request.urlopen(req, timeout=30).read())["prompt_id"]
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", "ignore")
        raise RuntimeError(f"POST /prompt failed: {e.code} {e.reason}\n{body[:1500]}")


def wait_for(prompt_id, label="job", timeout=900):
    start = time.time()
    while time.time() - start < timeout:
        try:
            r = urllib.request.urlopen(f"{COMFY_URL}/history/{prompt_id}", timeout=10)
            h = json.loads(r.read())
        except Exception as e:
            time.sleep(3)
            continue
        if prompt_id in h:
            elapsed = time.time() - start
            status = h[prompt_id].get("status", {})
            if status.get("status_str") == "error":
                err_msg = ""
                for m in status.get("messages", []):
                    if m[0] == "execution_error":
                        err_msg = f"{m[1].get('node_type')}: {m[1].get('exception_message','').strip()[:300]}"
                        break
                raise RuntimeError(f"[{label}] error after {elapsed:.1f}s: {err_msg}")
            return h[prompt_id], elapsed
        time.sleep(2)
    raise RuntimeError(f"[{label}] timeout after {timeout}s")


def fetch_image(filename, subfolder, type_):
    url = f"{COMFY_URL}/view?{urllib.parse.urlencode({'filename': filename, 'subfolder': subfolder, 'type': type_})}"
    return urllib.request.urlopen(url, timeout=30).read()


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
    urllib.request.urlopen(req, timeout=60)


# ============================================================================
# PROMPT BUILDING
# ============================================================================
def validate_masked_input(image_or_pil, multi_obj_thresh=0.20, edge_thresh=0.40,
                            min_opaque=0.03, max_opaque=0.92):
    """Validate that a masked input image is suitable for 3D generation.

    Returns (is_valid, reason, stats_dict).

    Three checks (any failure rejects):
    1. Coverage: 3% < opaque pixels < 92% (not empty, not 100% filled)
    2. Cut-off: 2+ edges have >40% of edge pixels opaque (object exceeds frame)
    3. Multi-object: connected-component analysis — 2nd blob > 20% of largest =
       likely multiple separate objects (which produces cube artifacts)

    Verified on 17 real masked inputs (2026-05-03):
    - Catches all known cube artifacts (chandelier_iron, isometric forest)
    - Catches all known cut-offs (bone_pile spilling, awning filling frame)
    - Catches multi-object inputs (forests, multi-flower clusters)
    - Allows legitimate single-object inputs even with minor masking artifacts
    """
    import numpy as np
    from PIL import Image
    from scipy.ndimage import label

    if isinstance(image_or_pil, (str, Path)):
        img = Image.open(image_or_pil)
    else:
        img = image_or_pil

    arr = np.array(img)
    if arr.ndim < 3 or arr.shape[2] < 4:
        return False, "no_alpha_channel", {}

    alpha = arr[..., 3]
    H, W = alpha.shape
    opaque = alpha > 128
    opaque_pct = float(opaque.mean())

    stats = {"opaque_pct": opaque_pct}

    # 1. Coverage
    if opaque_pct < min_opaque:
        return False, f"too_small ({opaque_pct*100:.1f}%)", stats
    if opaque_pct > max_opaque:
        return False, f"fills_frame ({opaque_pct*100:.1f}%)", stats

    # 2. Cut-off — % of each edge that is opaque
    edges = {
        "top":    float(opaque[0:3, :].any(axis=0).mean()),
        "bottom": float(opaque[-3:, :].any(axis=0).mean()),
        "left":   float(opaque[:, 0:3].any(axis=1).mean()),
        "right":  float(opaque[:, -3:].any(axis=1).mean()),
    }
    stats["edge_pct"] = edges
    cut_off = [name for name, pct in edges.items() if pct > edge_thresh]
    if len(cut_off) >= 2:
        return False, f"cut_off_{','.join(cut_off)}", stats

    # 3. Multi-object — connected components
    labeled, n_blobs = label(opaque)
    if n_blobs > 1:
        sizes = sorted([int((labeled == i+1).sum()) for i in range(n_blobs)], reverse=True)
        stats["blob_count"] = n_blobs
        stats["blob_sizes"] = sizes[:5]
        if len(sizes) >= 2 and sizes[1] / sizes[0] > multi_obj_thresh:
            return False, (f"multi_object ({n_blobs} blobs, "
                            f"2nd={100*sizes[1]/sizes[0]:.0f}% of largest)"), stats

    return True, "ok", stats


def build_positive_prompt(subject):
    """The verified rotated/perspective prompt pattern (depth ratio 0.82-0.97).
    Default single-strategy prompt — used by process_asset.
    For multi-input, use PERSPECTIVE_STRATEGIES below."""
    return (
        f"ONE single {subject}, slightly rotated to show depth, "
        "looking down from 30 degrees above, "
        "you can see the top curving away from the viewer, "
        "voxel game asset 3D rendered, octane render with ambient occlusion, "
        "isolated on plain white background, centered, "
        "low-poly stylized fantasy game asset, "
        "Ragnarok Online style, hand-painted warm earth tones, no shadow"
    )


def build_negative_prompt():
    return (
        # Wrong perspective
        "front view, side view, profile, orthographic, flat, "
        "2D illustration, 2D drawing, sketch, painting, "
        # Multiple objects (cube artifact prevention)
        "multiple objects, two objects, three objects, several, many, "
        "pair, group, cluster of objects, duplicate, copies, "
        "tiling, repeating, grid, collage, panel, "
        # Cropping / containment (incomplete reconstruction prevention)
        "cut off, clipped, cropped, partial view, zoomed in, "
        "extends beyond frame, touches edges, fills entire frame, "
        "object exceeds canvas, no margin, no padding, "
        # Style
        "photorealistic, photograph, "
        "perspective lines, horizon, scene, foreground objects, "
        "background scenery, ground, floor, surface beneath, "
        "frame, border, watermark"
    )


# Perspective strategies for multi-input generation.
# Verified on tree (2026-05-03): "rotated" + "3quarter" complement each other —
# different prompts produce different volumetric results, and picking the best
# across both strategies adds depth diversity. AVOID "isometric_3d" — it generates
# forests of multiple objects (verified failure mode).
PERSPECTIVE_STRATEGIES = [
    {
        "name": "rotated",
        "build": lambda subject: (
            f"ONE SINGLE {subject}, ONE OBJECT ONLY, "
            "the entire object is fully contained inside the frame "
            "with clear empty white space around all edges, "
            "small enough to leave at least 10 percent margin around all sides, "
            "slightly rotated to show depth, "
            "looking down from 30 degrees above, "
            "you can see the top curving away from the viewer, "
            "voxel game asset 3D rendered, octane render with ambient occlusion, "
            "isolated on plain white background, centered with padding, "
            "low-poly stylized fantasy game asset, "
            "Ragnarok Online style, hand-painted warm earth tones, no shadow"
        ),
    },
    {
        "name": "3quarter",
        "build": lambda subject: (
            f"ONE SINGLE {subject}, ONE OBJECT ONLY, "
            "the entire object is fully contained inside the frame "
            "with clear empty white space around all edges, "
            "small enough to leave at least 10 percent margin around all sides, "
            "viewed from a 3/4 perspective angle, "
            "dramatic side lighting from upper left, "
            "strong shadow on opposite side showing volume, "
            "you can see both the front and the side of the object, "
            "full 3D shape with clear depth, sculpted form, "
            "isolated on plain white background, centered with padding, "
            "low-poly stylized fantasy game asset, photogrammetry render, "
            "Ragnarok Online style, hand-painted warm earth tones, no shadow on ground"
        ),
    },
]


# ============================================================================
# WORKFLOW BUILDERS
# ============================================================================
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


def hunyuan_meshgen_workflow(input_filename, seed, octree, prefix):
    return {
        "load_image": {"class_type": "Hy3D21LoadImageWithTransparency",
                       "inputs": {"image": input_filename}},
        "vae_load": {"class_type": "Hy3D21VAELoader",
                     "inputs": {"model_name": "hunyuan3d-vae-v2-1.ckpt"}},
        "mesh_gen": {"class_type": "Hy3DMeshGenerator",
                     "inputs": {"model": "hunyuan3d-dit-v2-1-fp16.ckpt",
                                "image": ["load_image", 2], "steps": MESH_STEPS,
                                "guidance_scale": MESH_GUIDANCE, "seed": seed,
                                "attention_mode": "sdpa"}},
        "vae_decode": {"class_type": "Hy3D21VAEDecode",
                       "inputs": {"vae": ["vae_load", 0], "latents": ["mesh_gen", 0],
                                  "box_v": 1.01, "octree_resolution": octree,
                                  "num_chunks": 128000, "mc_level": 0.0, "mc_algo": "mc",
                                  "enable_flash_vdm": True, "force_offload": False}},
        "postprocess": {"class_type": "Hy3D21PostprocessMesh",
                        "inputs": {"trimesh": ["vae_decode", 0], "remove_floaters": True,
                                   "remove_degenerate_faces": True, "reduce_faces": False,
                                   "max_facenum": 200000, "smooth_normals": False}},
        "export": {"class_type": "Hy3D21ExportMesh",
                   "inputs": {"trimesh": ["postprocess", 0],
                              "filename_prefix": prefix,
                              "file_format": "glb", "save_file": True}},
    }


def hunyuan_texture_workflow(mesh_path, input_filename, prefix):
    """Texture an existing GLB using multi-view + bake + inpaint."""
    return {
        "load_mesh": {"class_type": "Hy3D21LoadMesh",
                      "inputs": {"glb_path": str(mesh_path)}},
        "uv_wrap": {"class_type": "Hy3D21MeshUVWrap",
                    "inputs": {"trimesh": ["load_mesh", 0]}},
        "load_image": {"class_type": "Hy3D21LoadImageWithTransparency",
                       "inputs": {"image": input_filename}},
        "camera_config": {"class_type": "Hy3D21CameraConfig",
                          "inputs": {"camera_azimuths": "0, 90, 180, 270, 0, 180",
                                     "camera_elevations": "0, 0, 0, 0, 90, -90",
                                     "view_weights": "1, 0.5, 1, 0.5, 1, 1",
                                     "ortho_scale": 1.1}},
        "multi_views": {"class_type": "Hy3DMultiViewsGenerator",
                        "inputs": {"trimesh": ["uv_wrap", 0],
                                   "camera_config": ["camera_config", 0],
                                   "image": ["load_image", 2],
                                   "view_size": TEXTURE_VIEW_SIZE,
                                   "steps": TEXTURE_STEPS,
                                   "guidance_scale": TEXTURE_GUIDANCE,
                                   "texture_size": TEXTURE_SIZE,
                                   "unwrap_mesh": False,
                                   "seed": 42}},
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
                               "output_mesh_name": prefix}},
    }


def hunyuan_decimate_workflow(mesh_path, target_faces, prefix):
    """Decimate an existing GLB to target_faces."""
    return {
        "load_mesh": {"class_type": "Hy3D21LoadMesh",
                      "inputs": {"glb_path": str(mesh_path)}},
        "decimate": {"class_type": "Hy3D21MeshlibDecimate",
                     "inputs": {"trimesh": ["load_mesh", 0],
                                "subdivideParts": 1,
                                "target_face_num": target_faces,
                                "strategy": "MinimizeError",
                                "optimizeVertexPos": True,
                                "collapseNearNotFlippable": False,
                                "decimateBetweenParts": False,
                                "minFacesInPart": 100}},
        "export": {"class_type": "Hy3D21ExportMesh",
                   "inputs": {"trimesh": ["decimate", 0],
                              "filename_prefix": prefix,
                              "file_format": "glb", "save_file": True}},
    }


# ============================================================================
# MESH ANALYSIS
# ============================================================================
def analyze_mesh(path):
    import trimesh
    m = trimesh.load(str(path))
    g = list(m.geometry.values())[0] if isinstance(m, trimesh.Scene) else m
    e = g.extents.tolist()
    roundness = float(e[2] / max(e[0], e[1])) if max(e[0], e[1]) > 0 else 0
    return {
        "verts": int(len(g.vertices)),
        "faces": int(len(g.faces)),
        "extents": e,
        "depth_ratio": roundness,
        "volume": float(g.volume),
        "watertight": bool(g.is_watertight),
        "score": float(roundness * g.volume),  # selection metric
    }


# ============================================================================
# STATUS / LOGGING
# ============================================================================
def log_msg(msg):
    ts = time.strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def update_status(state):
    try:
        STATUS_FILE.write_text(json.dumps(state, indent=2, default=str))
    except Exception as e:
        log_msg(f"  WARN: status write failed: {e}")


# ============================================================================
# PIPELINE STAGES
# ============================================================================
def stage_generate_input(asset, asset_dir):
    """Generate input image with SDXL Base."""
    positive = build_positive_prompt(asset["subject"])
    negative = build_negative_prompt()
    prefix = f"asset_input_{asset['id']}"
    pid = post_prompt(sdxl_workflow(positive, negative, prefix))
    result, elapsed = wait_for(pid, "sdxl", timeout=180)
    img_info = result["outputs"]["7"]["images"][0]
    img_bytes = fetch_image(img_info["filename"], img_info["subfolder"], "output")

    raw_path = asset_dir / "01_input_raw.png"
    raw_path.write_bytes(img_bytes)
    return raw_path, img_bytes, elapsed


def stage_mask(asset, asset_dir, img_bytes, rembg_session):
    """Pre-mask with rembg BiRefNet."""
    from PIL import Image
    from rembg import remove
    import numpy as np

    img = Image.open(io.BytesIO(img_bytes))
    masked = remove(img, session=rembg_session, alpha_matting=True,
                    alpha_matting_foreground_threshold=240,
                    alpha_matting_background_threshold=10)
    arr = np.array(masked)
    trans_pct = float((arr[..., 3] == 0).mean() * 100)

    masked_path = asset_dir / "02_input_masked.png"
    masked.save(masked_path)

    cf_filename = f"asset_input_{asset['id']}.png"
    upload_image(masked_path, cf_filename)
    return masked_path, cf_filename, trans_pct


def stage_meshgen_sweep(asset, asset_dir, cf_filename):
    """Sweep N seeds, return best mesh by score = depth_ratio × volume."""
    sweep_dir = asset_dir / "03_meshes"
    sweep_dir.mkdir(exist_ok=True)
    candidates = []

    for seed in MESH_SEEDS:
        prefix = f"asset_mesh_{asset['id']}_s{seed}"
        try:
            pid = post_prompt(hunyuan_meshgen_workflow(cf_filename, seed, MESH_OCTREE, prefix))
            _, elapsed = wait_for(pid, f"meshgen_s{seed}", timeout=300)
        except Exception as e:
            log_msg(f"    seed {seed} mesh-gen FAILED: {e}")
            continue

        glbs = sorted(Path("C:/ComfyUI/output").glob(f"{prefix}*.glb"),
                      key=lambda p: p.stat().st_mtime, reverse=True)
        if not glbs:
            continue
        target = sweep_dir / f"s{seed}.glb"
        shutil.copy(glbs[0], target)
        try:
            stats = analyze_mesh(target)
            stats["seed"] = seed
            stats["path"] = str(target)
            candidates.append(stats)
            log_msg(f"    s{seed}: depth={stats['depth_ratio']:.2f} vol={stats['volume']:.2f} score={stats['score']:.3f}")
        except Exception as e:
            log_msg(f"    s{seed} analysis FAILED: {e}")

    if not candidates:
        raise RuntimeError("No valid meshes generated from seed sweep")

    # Filter by minimum volume to avoid thin shells, then pick highest score
    valid = [c for c in candidates if c["volume"] > 0.1]
    if not valid:
        valid = candidates  # if all are thin, just use them
    best = max(valid, key=lambda c: c["score"])
    log_msg(f"    BEST seed {best['seed']}: depth={best['depth_ratio']:.2f} vol={best['volume']:.2f}")

    best_path = asset_dir / "04_best_mesh.glb"
    shutil.copy(best["path"], best_path)
    return best_path, best


def stage_texture(asset, asset_dir, mesh_path, cf_filename):
    """Texture the mesh via multi-view bake + inpaint.
    InPaint node saves output to ComfyUI/temp/ (not output/) — search both.
    On a decimated 5K-face mesh this takes ~40s.
    """
    prefix = f"asset_textured_{asset['id']}"
    pid = post_prompt(hunyuan_texture_workflow(mesh_path, cf_filename, prefix))
    _, elapsed = wait_for(pid, "texture", timeout=600)
    # Search in temp/ first (where InPaint actually saves), then output/
    candidates = []
    for root in [Path("C:/ComfyUI/temp"), Path("C:/ComfyUI/output")]:
        candidates.extend(root.glob(f"{prefix}*.glb"))
    if not candidates:
        raise RuntimeError(f"No textured GLB found with prefix {prefix}")
    src = max(candidates, key=lambda p: p.stat().st_mtime)
    target = asset_dir / "05_textured.glb"
    shutil.copy(src, target)
    return target, elapsed


def stage_decimate(asset, asset_dir, mesh_path):
    """Decimate to game-ready face count."""
    prefix = f"asset_decimated_{asset['id']}"
    pid = post_prompt(hunyuan_decimate_workflow(mesh_path, DECIMATE_TARGET_FACES, prefix))
    _, elapsed = wait_for(pid, "decimate", timeout=180)
    glbs = sorted(Path("C:/ComfyUI/output").glob(f"{prefix}*.glb"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    if not glbs:
        raise RuntimeError(f"No decimated GLB found with prefix {prefix}")
    target = asset_dir / "06_decimated.glb"
    shutil.copy(glbs[0], target)
    stats = analyze_mesh(target)
    return target, stats, elapsed


# ============================================================================
# PIPELINE PER-ASSET — MULTI-INPUT (preferred for hard assets / cleanup)
# ============================================================================
def process_asset_multi_input(asset, rembg_session, strategies=None,
                                seeds_per_input=2, extra_subject_suffix=""):
    """Process an asset using MULTIPLE perspective strategies for input image.
    For each strategy, generate input -> mask -> run mesh-gen with K seeds.
    Pick the best mesh across all (input × seed) combinations by depth × volume.

    Same total time as single-input × 4 seeds (current process_asset), but with
    input diversity that catches assets where one perspective fails.

    Args:
        asset: dict with id, category, subject
        rembg_session: rembg session
        strategies: list of perspective strategies (default: PERSPECTIVE_STRATEGIES)
        seeds_per_input: how many seeds per input strategy (default 2 -> 2x2=4 total mesh-gens)
        extra_subject_suffix: optional string appended to subject (used by cleanup retries)
    """
    import io
    if strategies is None:
        strategies = PERSPECTIVE_STRATEGIES

    asset_dir = ROOT / asset["category"] / asset["id"]
    asset_dir.mkdir(parents=True, exist_ok=True)

    full_subject = asset["subject"] + extra_subject_suffix

    manifest = {
        "id": asset["id"],
        "category": asset["category"],
        "subject": full_subject,
        "started": time.time(),
        "stages": {},
        "strategies": [s["name"] for s in strategies],
        "seeds_per_input": seeds_per_input,
        "multi_input": True,
    }

    candidates = []  # list of dicts with strategy, seed, score, input_filename, path

    # Step 1+2: For each strategy, generate input image, mask it, VALIDATE.
    # If validation fails, retry with a different SDXL seed (up to 3 attempts).
    inputs_by_strategy = {}
    sweep_seeds = MESH_SEEDS[:seeds_per_input]

    MAX_INPUT_RETRIES = 3
    SDXL_SEED_BASE = abs(hash(asset["id"])) % 100000  # deterministic per-asset

    for strat in strategies:
        log_msg(f"  [{strat['name']}] generate + mask input (with validation)...")
        positive = strat["build"](full_subject)
        negative = build_negative_prompt()

        # Try up to MAX_INPUT_RETRIES times to get a validation-passing input
        from PIL import Image
        from rembg import remove

        masked = None
        last_validation = None
        for attempt in range(MAX_INPUT_RETRIES):
            sdxl_seed = SDXL_SEED_BASE + attempt * 1000 + hash(strat["name"]) % 1000
            prefix = f"asset_in_{asset['id']}_{strat['name']}_t{attempt}"
            try:
                wf = sdxl_workflow(positive, negative, prefix, seed=sdxl_seed)
                pid = post_prompt(wf)
                result, _ = wait_for(pid, f"sdxl_{strat['name']}_t{attempt}", timeout=180)
                img_info = result["outputs"]["7"]["images"][0]
                img_bytes = fetch_image(img_info["filename"], img_info["subfolder"], "output")
            except Exception as e:
                log_msg(f"    SDXL attempt {attempt+1} FAILED: {e}")
                continue

            try:
                img = Image.open(io.BytesIO(img_bytes))
                masked_try = remove(img, session=rembg_session, alpha_matting=True,
                                     alpha_matting_foreground_threshold=240,
                                     alpha_matting_background_threshold=10)
            except Exception as e:
                log_msg(f"    mask attempt {attempt+1} FAILED: {e}")
                continue

            valid, reason, _stats = validate_masked_input(masked_try)
            last_validation = (valid, reason)
            if valid:
                # Save raw (pre-mask) for debugging
                raw_path = asset_dir / f"01_input_{strat['name']}_raw.png"
                raw_path.write_bytes(img_bytes)
                masked = masked_try
                log_msg(f"    attempt {attempt+1} validated: {reason}")
                break
            else:
                log_msg(f"    attempt {attempt+1} REJECTED: {reason}, retrying with new seed...")

        if masked is None:
            # Accept the last attempt anyway — better than nothing
            log_msg(f"  [{strat['name']}] all {MAX_INPUT_RETRIES} attempts failed validation, using last "
                    f"({last_validation[1] if last_validation else 'unknown'})")
            try:
                # Re-do once more without retry to get something usable
                wf = sdxl_workflow(positive, negative, f"asset_in_{asset['id']}_{strat['name']}_final",
                                    seed=SDXL_SEED_BASE + hash(strat["name"]) % 100)
                pid = post_prompt(wf)
                result, _ = wait_for(pid, f"sdxl_{strat['name']}_final", timeout=180)
                img_info = result["outputs"]["7"]["images"][0]
                img_bytes = fetch_image(img_info["filename"], img_info["subfolder"], "output")
                raw_path = asset_dir / f"01_input_{strat['name']}_raw.png"
                raw_path.write_bytes(img_bytes)
                img = Image.open(io.BytesIO(img_bytes))
                masked = remove(img, session=rembg_session, alpha_matting=True,
                                 alpha_matting_foreground_threshold=240,
                                 alpha_matting_background_threshold=10)
            except Exception as e:
                log_msg(f"    final fallback gen failed: {e}")
                continue

        masked_path = asset_dir / f"02_input_{strat['name']}_masked.png"
        masked.save(masked_path)
        cf_filename = f"asset_in_{asset['id']}_{strat['name']}.png"
        upload_image(masked_path, cf_filename)
        inputs_by_strategy[strat["name"]] = cf_filename

    if not inputs_by_strategy:
        raise RuntimeError("No input images generated for any strategy")

    # Step 3: For each (strategy, seed) combo, run mesh-gen
    log_msg(f"  [mesh sweep: {len(inputs_by_strategy)} strategies × {seeds_per_input} seeds = "
            f"{len(inputs_by_strategy) * seeds_per_input} candidates]")
    sweep_dir = asset_dir / "03_meshes"
    sweep_dir.mkdir(exist_ok=True)

    for strat_name, cf_filename in inputs_by_strategy.items():
        for seed in sweep_seeds:
            mesh_prefix = f"asset_mesh_{asset['id']}_{strat_name}_s{seed}"
            try:
                pid = post_prompt(hunyuan_meshgen_workflow(cf_filename, seed, MESH_OCTREE, mesh_prefix))
                _, _ = wait_for(pid, f"meshgen_{strat_name}_s{seed}", timeout=300)
            except Exception as e:
                log_msg(f"    {strat_name}_s{seed}: mesh-gen FAILED ({str(e)[:80]})")
                continue
            glbs = sorted(Path("C:/ComfyUI/output").glob(f"{mesh_prefix}*.glb"),
                          key=lambda p: p.stat().st_mtime, reverse=True)
            if not glbs:
                continue
            target = sweep_dir / f"{strat_name}_s{seed}.glb"
            shutil.copy(glbs[0], target)
            try:
                stats = analyze_mesh(target)
                stats["strategy"] = strat_name
                stats["seed"] = seed
                stats["path"] = str(target)
                stats["input_filename"] = cf_filename
                candidates.append(stats)
                log_msg(f"    {strat_name}_s{seed}: depth={stats['depth_ratio']:.2f} "
                        f"vol={stats['volume']:.2f} score={stats['score']:.3f}")
            except Exception as e:
                log_msg(f"    {strat_name}_s{seed}: analysis failed: {e}")

    if not candidates:
        raise RuntimeError("All mesh-gen attempts failed across all strategies")

    # Step 4: Pick best - filter cube artifacts (vol > 5) and thin shells (vol < 0.05),
    # then pick highest score. If none pass filter, fall back to all.
    valid = [c for c in candidates if 0.05 < c["volume"] < 5.0]
    if not valid:
        valid = [c for c in candidates if c["volume"] > 0.01]
    if not valid:
        valid = candidates  # last resort
    best = max(valid, key=lambda c: c["score"])
    log_msg(f"    BEST: {best['strategy']}_s{best['seed']} (score={best['score']:.3f}, "
            f"depth={best['depth_ratio']:.2f}, vol={best['volume']:.2f})")

    best_path = asset_dir / "04_best_mesh.glb"
    shutil.copy(best["path"], best_path)
    manifest["stages"]["mesh_sweep"] = {
        "best_strategy": best["strategy"],
        "best_seed": best["seed"],
        "depth_ratio": round(best["depth_ratio"], 3),
        "volume": round(best["volume"], 3),
        "score": round(best["score"], 3),
        "verts": best["verts"],
        "faces": best["faces"],
        "candidates_total": len(candidates),
        "candidates_valid": len(valid),
        "path": str(best_path),
    }

    # Step 5: Decimate
    log_msg(f"  [decimate to {DECIMATE_TARGET_FACES} faces]")
    decim_path = asset_dir / "04b_decimated_untextured.glb"
    pid = post_prompt(hunyuan_decimate_workflow(best_path, DECIMATE_TARGET_FACES,
                                                  f"asset_decim_{asset['id']}"))
    _, t_dec = wait_for(pid, "decimate", timeout=180)
    glbs = sorted(Path("C:/ComfyUI/output").glob(f"asset_decim_{asset['id']}*.glb"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    if not glbs:
        raise RuntimeError("decimate produced no GLB")
    shutil.copy(glbs[0], decim_path)
    pre_stats = analyze_mesh(decim_path)
    manifest["stages"]["decimate_pre"] = {"time_s": round(t_dec, 1),
                                            "faces": pre_stats["faces"],
                                            "path": str(decim_path)}

    # Step 6: Texture using best's input image
    log_msg(f"  [texture decimated mesh with {best['strategy']} input]")
    textured_path, t_tex = stage_texture(asset, asset_dir, decim_path,
                                           best["input_filename"])
    manifest["stages"]["texture"] = {"time_s": round(t_tex, 1),
                                       "path": str(textured_path)}

    final_path = asset_dir / "06_final.glb"
    shutil.copy(textured_path, final_path)
    final_stats = analyze_mesh(final_path)
    manifest["stages"]["final"] = {"faces": final_stats["faces"],
                                     "verts": final_stats["verts"],
                                     "path": str(final_path)}

    manifest["finished"] = time.time()
    manifest["total_time_s"] = round(manifest["finished"] - manifest["started"], 1)
    manifest["status"] = "ok"
    (asset_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, default=str))
    return manifest


# ============================================================================
# PIPELINE PER-ASSET — SINGLE-INPUT (legacy, used by batch1)
# ============================================================================
def process_asset(asset, rembg_session):
    asset_dir = ROOT / asset["category"] / asset["id"]
    asset_dir.mkdir(parents=True, exist_ok=True)

    manifest = {
        "id": asset["id"],
        "category": asset["category"],
        "subject": asset["subject"],
        "started": time.time(),
        "stages": {},
    }

    log_msg(f"  [1/5] Generate input image...")
    raw_path, img_bytes, t = stage_generate_input(asset, asset_dir)
    manifest["stages"]["sdxl"] = {"time_s": round(t, 1), "path": str(raw_path)}

    log_msg(f"  [2/5] Pre-mask with rembg BiRefNet...")
    t0 = time.time()
    masked_path, cf_filename, trans_pct = stage_mask(asset, asset_dir, img_bytes, rembg_session)
    manifest["stages"]["mask"] = {"time_s": round(time.time() - t0, 1),
                                   "path": str(masked_path), "trans_pct": round(trans_pct, 1)}

    log_msg(f"  [3/5] Mesh-gen seed sweep ({len(MESH_SEEDS)} seeds)...")
    t0 = time.time()
    best_path, best_stats = stage_meshgen_sweep(asset, asset_dir, cf_filename)
    manifest["stages"]["mesh_sweep"] = {"time_s": round(time.time() - t0, 1),
                                         "best_seed": best_stats["seed"],
                                         "depth_ratio": round(best_stats["depth_ratio"], 3),
                                         "volume": round(best_stats["volume"], 3),
                                         "verts": best_stats["verts"],
                                         "faces": best_stats["faces"],
                                         "path": str(best_path)}

    # NOTE: Decimate BEFORE texture for 30-60x speedup. Texturing a 5K-face
    # mesh takes ~60s; texturing a 700K-face mesh takes 15+ min. Quality
    # difference is minor for stylized RO-style assets.
    log_msg(f"  [4/5] Decimate to {DECIMATE_TARGET_FACES} faces (before texturing for speed)...")
    decimated_untextured = asset_dir / "04b_decimated_untextured.glb"
    pid = post_prompt(hunyuan_decimate_workflow(best_path, DECIMATE_TARGET_FACES,
                                                  f"asset_decim_pre_{asset['id']}"))
    _, t_dec1 = wait_for(pid, "decimate-pre", timeout=180)
    glbs = sorted(Path("C:/ComfyUI/output").glob(f"asset_decim_pre_{asset['id']}*.glb"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    if not glbs:
        raise RuntimeError("decimate-pre produced no GLB")
    shutil.copy(glbs[0], decimated_untextured)
    pre_stats = analyze_mesh(decimated_untextured)
    manifest["stages"]["decimate_pre"] = {"time_s": round(t_dec1, 1),
                                            "faces": pre_stats["faces"],
                                            "path": str(decimated_untextured)}
    log_msg(f"    pre-decimate done: {pre_stats['faces']} faces")

    log_msg(f"  [5/5] Texture decimated mesh (multi-view bake + inpaint)...")
    textured_path, t = stage_texture(asset, asset_dir, decimated_untextured, cf_filename)
    manifest["stages"]["texture"] = {"time_s": round(t, 1), "path": str(textured_path)}

    # Final output is the textured GLB (which is already decimated)
    final_path = asset_dir / "06_final.glb"
    shutil.copy(textured_path, final_path)
    final_stats = analyze_mesh(final_path)
    manifest["stages"]["final"] = {"faces": final_stats["faces"],
                                     "verts": final_stats["verts"],
                                     "path": str(final_path)}

    manifest["finished"] = time.time()
    manifest["total_time_s"] = round(manifest["finished"] - manifest["started"], 1)
    manifest["status"] = "ok"
    (asset_dir / "manifest.json").write_text(json.dumps(manifest, indent=2))
    return manifest


# ============================================================================
# MAIN
# ============================================================================
def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    log_msg("=" * 78)
    log_msg(f"AUTONOMOUS 3D ASSET PIPELINE — {len(ASSETS)} assets")
    log_msg(f"Output root: {ROOT}")
    log_msg(f"Status file: {STATUS_FILE}")
    log_msg("=" * 78)

    # Verify ComfyUI is up
    try:
        urllib.request.urlopen(f"{COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log_msg(f"FATAL: ComfyUI not reachable at {COMFY_URL}: {e}")
        return 1

    # Load rembg session once
    log_msg("Loading rembg BiRefNet session...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    # State tracking
    state = {
        "started": time.strftime("%Y-%m-%d %H:%M:%S"),
        "total_assets": len(ASSETS),
        "completed": 0,
        "failed": 0,
        "current": None,
        "results": [],
    }
    update_status(state)

    overall_start = time.time()
    for i, asset in enumerate(ASSETS, 1):
        log_msg("")
        log_msg("-" * 78)
        log_msg(f"[{i}/{len(ASSETS)}] {asset['category']}/{asset['id']}")
        log_msg(f"  subject: {asset['subject'][:80]}...")
        log_msg("-" * 78)
        state["current"] = f"{asset['category']}/{asset['id']}"
        update_status(state)

        last_err = None
        for attempt in range(MAX_RETRIES + 1):
            try:
                manifest = process_asset(asset, rembg_session)
                log_msg(f"  DONE in {manifest['total_time_s']:.0f}s "
                        f"(depth={manifest['stages']['mesh_sweep']['depth_ratio']:.2f} "
                        f"vol={manifest['stages']['mesh_sweep']['volume']:.2f} "
                        f"faces={manifest['stages']['final']['faces']})")
                state["completed"] += 1
                state["results"].append({
                    "id": asset["id"], "category": asset["category"], "status": "ok",
                    "time_s": manifest["total_time_s"],
                    "depth": manifest["stages"]["mesh_sweep"]["depth_ratio"],
                    "volume": manifest["stages"]["mesh_sweep"]["volume"],
                    "faces": manifest["stages"]["final"]["faces"],
                })
                last_err = None
                break
            except Exception as e:
                last_err = str(e)
                tb = traceback.format_exc()
                log_msg(f"  FAILED (attempt {attempt+1}/{MAX_RETRIES+1}): {e}")
                if attempt < MAX_RETRIES:
                    log_msg(f"  retrying in 5s...")
                    time.sleep(5)

        if last_err:
            state["failed"] += 1
            state["results"].append({
                "id": asset["id"], "category": asset["category"], "status": "failed",
                "error": last_err[:500],
            })
            # Save error to asset folder
            asset_dir = ROOT / asset["category"] / asset["id"]
            asset_dir.mkdir(parents=True, exist_ok=True)
            (asset_dir / "ERROR.txt").write_text(f"{last_err}\n\n{tb}")

        update_status(state)

        # ETA
        elapsed = time.time() - overall_start
        avg_per_asset = elapsed / max(i, 1)
        eta_s = avg_per_asset * (len(ASSETS) - i)
        log_msg(f"  [progress: {i}/{len(ASSETS)} done, "
                f"{state['completed']} ok, {state['failed']} failed | "
                f"ETA: {eta_s/60:.1f} min remaining]")

    state["finished"] = time.strftime("%Y-%m-%d %H:%M:%S")
    state["total_time_min"] = round((time.time() - overall_start) / 60, 1)
    state["current"] = None
    update_status(state)

    log_msg("")
    log_msg("=" * 78)
    log_msg(f"DONE — {state['completed']}/{len(ASSETS)} succeeded, "
            f"{state['failed']} failed, total {state['total_time_min']:.1f} min")
    log_msg("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
