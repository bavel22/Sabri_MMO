# create_ro_v7_variants.py
# Creates v7/ Material Instances using M_RO_Triplanar_v2 parent.
# Uses cliff_pairings.json for auto cliff texture pairing.
# Batch-saves at the end instead of per-asset (10x faster, no source control spam).
#
# Run create_ro_triplanar_v2_material.py FIRST, then analyze_cliff_pairings.py.

import unreal
import json
import os

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/v7/M_RO_Triplanar_v2"
MI_BASE = "/Game/SabriMMO/Materials/Environment/v7"
RO_TEX = "/Game/SabriMMO/Textures/Environment/RO_Original"
PAIRINGS_FILE = r"C:\Sabri_MMO\client\SabriMMO\Content\SabriMMO\Textures\Environment\RO_Original\cliff_pairings.json"

eal.make_directory(MI_BASE)

# ============================================================
# Load cliff pairings
# ============================================================

pairings = {}
if os.path.exists(PAIRINGS_FILE):
    with open(PAIRINGS_FILE, "r") as f:
        pairings = json.load(f)
    unreal.log(f"Loaded {len(pairings)} cliff pairings from JSON")
else:
    unreal.log_warning(f"No pairings file found at {PAIRINGS_FILE}")
    unreal.log_warning("Run analyze_cliff_pairings.py with system Python first!")
    unreal.log_warning("Falling back to self-pairing (same texture for ground+cliff)")

# ============================================================
# Get all textures (filter to Texture2D only)
# ============================================================

all_assets = eal.list_assets(RO_TEX, recursive=False)
all_textures = []
tex_by_name = {}
for asset in sorted(all_assets):
    clean = asset.split(".")[0]
    if eal.does_asset_exist(clean):
        loaded = unreal.load_asset(clean)
        if loaded and isinstance(loaded, unreal.Texture2D):
            name = clean.split("/")[-1]
            all_textures.append(clean)
            tex_by_name[name] = loaded

unreal.log(f"Found {len(all_textures)} textures")

if not all_textures:
    unreal.log_error("No textures found!")
    raise RuntimeError("No textures")

# Load parent
parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent not found: {PARENT_PATH}")
    unreal.log_error("Run create_ro_triplanar_v2_material.py first!")
    raise RuntimeError("Missing parent")

# ============================================================
# Helper: set all 6 color texture params for a given texture
# ============================================================

GND_PARAMS = ["GroundTexture", "GroundTextureXZ", "GroundTextureYZ"]
CLF_PARAMS = ["CliffTexture", "CliffTextureXZ", "CliffTextureYZ"]

def set_ground_tex(mi, tex):
    for p in GND_PARAMS:
        mel.set_material_instance_texture_parameter_value(
            mi, p, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

def set_cliff_tex(mi, tex):
    for p in CLF_PARAMS:
        mel.set_material_instance_texture_parameter_value(
            mi, p, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

# ============================================================
# Create variants — collect dirty packages, batch save at end
# ============================================================

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()
created = 0
skipped = 0
dirty_packages = []

# --- PART 1: Individual textures with auto cliff pairing ---
unreal.log(f"\nPart 1: Individual textures with auto cliff pairing...")

for i, tex_path in enumerate(all_textures):
    tex_name = tex_path.split("/")[-1]
    batch = f"{(i // 50) * 50 + 1:03d}-{min((i // 50 + 1) * 50, len(all_textures)):03d}"
    mi_folder = f"{MI_BASE}/individual/{batch}"
    mi_name = f"MI_RO_{tex_name}"
    mi_full = f"{mi_folder}/{mi_name}"

    if eal.does_asset_exist(mi_full):
        skipped += 1
        continue

    eal.make_directory(mi_folder)
    mi = asset_tools.create_asset(mi_name, mi_folder,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        continue

    mi.set_editor_property("parent", parent)

    tex = tex_by_name.get(tex_name)
    if tex:
        set_ground_tex(mi, tex)

        # Use cliff pairing if available
        cliff_name = pairings.get(tex_name, tex_name)
        cliff_tex = tex_by_name.get(cliff_name, tex)
        set_cliff_tex(mi, cliff_tex)

    dirty_packages.append(mi_full)
    created += 1

    if (i + 1) % 100 == 0:
        unreal.log(f"  [{i+1}/{len(all_textures)}] {created} created...")

unreal.log(f"  Part 1 done: {created} individual variants")

# --- PART 2: Smart pairings (ground + different cliff from pairing) ---
unreal.log(f"\nPart 2: Smart pairings...")

pair_created = 0
for i in range(0, len(all_textures) - 5, 3):
    ground = all_textures[i]
    for offset in [2, 4, 10, 20]:
        if i + offset >= len(all_textures):
            continue
        cliff = all_textures[i + offset]

        g_name = ground.split("/")[-1]
        c_name = cliff.split("/")[-1]
        mi_name = f"MI_RO_{g_name}_x_{c_name}"
        mi_folder = f"{MI_BASE}/paired"
        mi_full = f"{mi_folder}/{mi_name}"

        if eal.does_asset_exist(mi_full):
            skipped += 1
            continue

        eal.make_directory(mi_folder)
        mi = asset_tools.create_asset(mi_name, mi_folder,
            unreal.MaterialInstanceConstant, mi_factory)
        if not mi:
            continue

        mi.set_editor_property("parent", parent)

        g_tex = tex_by_name.get(g_name)
        c_tex = tex_by_name.get(c_name)
        if g_tex:
            set_ground_tex(mi, g_tex)
        if c_tex:
            set_cliff_tex(mi, c_tex)

        dirty_packages.append(mi_full)
        pair_created += 1

    if (i + 1) % 60 == 0:
        unreal.log(f"  [{i}/{len(all_textures)}] {pair_created} pairs created...")

created += pair_created
unreal.log(f"  Part 2 done: {pair_created} paired variants")

# --- PART 3: Tile size variations for first 50 ---
unreal.log(f"\nPart 3: Tile size variations...")

size_created = 0
tile_sizes = [2000.0, 3000.0, 5000.0, 7000.0]
for i, tex_path in enumerate(all_textures[:50]):
    tex_name = tex_path.split("/")[-1]
    for ts in tile_sizes:
        mi_name = f"MI_RO_{tex_name}_ts{int(ts)}"
        mi_folder = f"{MI_BASE}/tile_sizes"
        mi_full = f"{mi_folder}/{mi_name}"

        if eal.does_asset_exist(mi_full):
            skipped += 1
            continue

        eal.make_directory(mi_folder)
        mi = asset_tools.create_asset(mi_name, mi_folder,
            unreal.MaterialInstanceConstant, mi_factory)
        if not mi:
            continue

        mi.set_editor_property("parent", parent)

        tex = tex_by_name.get(tex_name)
        if tex:
            set_ground_tex(mi, tex)
            cliff_name = pairings.get(tex_name, tex_name)
            cliff_tex = tex_by_name.get(cliff_name, tex)
            set_cliff_tex(mi, cliff_tex)

        mel.set_material_instance_scalar_parameter_value(
            mi, "TileSize", ts, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
        mel.set_material_instance_scalar_parameter_value(
            mi, "CliffTileSize", ts * 0.75, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

        dirty_packages.append(mi_full)
        size_created += 1

created += size_created
unreal.log(f"  Part 3 done: {size_created} tile-size variants")

# ============================================================
# Batch save all dirty packages at once
# ============================================================

unreal.log(f"\nBatch saving {len(dirty_packages)} assets...")

# Save in batches of 50 to avoid memory pressure
BATCH = 50
for start in range(0, len(dirty_packages), BATCH):
    chunk = dirty_packages[start:start+BATCH]
    for pkg in chunk:
        eal.save_asset(pkg, only_if_is_dirty=True)
    if start + BATCH < len(dirty_packages):
        unreal.log(f"  Saved [{start+BATCH}/{len(dirty_packages)}]...")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {created} total created, {skipped} skipped")
unreal.log(f"  Browse: {MI_BASE}/")
unreal.log(f"    /individual/ — 608 textures with auto cliff pairing + triplanar + normals")
unreal.log(f"    /paired/     — ground+cliff combos")
unreal.log(f"    /tile_sizes/ — first 50 textures at 4 scales")
unreal.log(f"  Parent: {PARENT_PATH}")
unreal.log(f"  Features: triplanar projection, normal maps, distance macro tinting")
unreal.log(f"{'='*60}")
