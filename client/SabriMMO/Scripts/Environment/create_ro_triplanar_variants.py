# create_ro_triplanar_variants.py
# Creates v5/ Material Instances for all 608 RO Original textures,
# parented to M_RO_Triplanar (proper triplanar slope projection).
#
# Same structure as v4/individual/ but with triplanar parent.
# Run create_ro_triplanar_material.py FIRST.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_RO_Triplanar"
MI_BASE = "/Game/SabriMMO/Materials/Environment/v6"
RO_TEX = "/Game/SabriMMO/Textures/Environment/RO_Original"

eal.make_directory(MI_BASE)

# ============================================================
# Get all available RO Original textures
# ============================================================

all_assets = eal.list_assets(RO_TEX, recursive=False)
all_textures = []
for asset in sorted(all_assets):
    clean = asset.split(".")[0]
    if eal.does_asset_exist(clean):
        loaded = unreal.load_asset(clean)
        if loaded and isinstance(loaded, unreal.Texture2D):
            all_textures.append(clean)

unreal.log(f"Found {len(all_textures)} RO Original textures")

if not all_textures:
    unreal.log_error("No textures found! Run import_ro_originals.py first.")
    raise RuntimeError("No textures")

# Load parent
parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent not found: {PARENT_PATH}")
    unreal.log_error("Run create_ro_triplanar_material.py first!")
    raise RuntimeError("Missing parent")

# ============================================================
# Create variants
# ============================================================

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()
created = 0
skipped = 0

# --- PART 1: Each texture as its own ground (same texture for cliff) ---
unreal.log(f"\nPart 1: Individual triplanar textures...")

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

    tex = unreal.load_asset(tex_path)
    if tex:
        for pname in ["GroundTexture", "GroundTextureXZ", "GroundTextureYZ",
                       "CliffTexture", "CliffTextureXZ", "CliffTextureYZ"]:
            mel.set_material_instance_texture_parameter_value(
                mi, pname, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1

    if (i + 1) % 100 == 0:
        unreal.log(f"  [{i+1}/{len(all_textures)}] {created} created...")

unreal.log(f"  Part 1 done: {created} individual variants")

# --- PART 2: Smart pairings — ground + different cliff ---
unreal.log(f"\nPart 2: Smart pairings (ground + different cliff)...")

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

        g_tex = unreal.load_asset(ground)
        c_tex = unreal.load_asset(cliff)
        if g_tex:
            for pname in ["GroundTexture", "GroundTextureXZ", "GroundTextureYZ"]:
                mel.set_material_instance_texture_parameter_value(
                    mi, pname, g_tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
        if c_tex:
            for pname in ["CliffTexture", "CliffTextureXZ", "CliffTextureYZ"]:
                mel.set_material_instance_texture_parameter_value(
                    mi, pname, c_tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

        eal.save_asset(mi_full)
        pair_created += 1

    if (i + 1) % 60 == 0:
        unreal.log(f"  [{i}/{len(all_textures)}] {pair_created} pairs created...")

created += pair_created
unreal.log(f"  Part 2 done: {pair_created} paired variants")

# --- PART 3: Tile size variations for first 50 textures ---
unreal.log(f"\nPart 3: Tile size variations (first 50 textures)...")

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

        tex = unreal.load_asset(tex_path)
        if tex:
            for pname in ["GroundTexture", "GroundTextureXZ", "GroundTextureYZ",
                           "CliffTexture", "CliffTextureXZ", "CliffTextureYZ"]:
                mel.set_material_instance_texture_parameter_value(
                    mi, pname, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

        mel.set_material_instance_scalar_parameter_value(
            mi, "TileSize", ts, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
        mel.set_material_instance_scalar_parameter_value(
            mi, "CliffTileSize", ts * 0.75, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

        eal.save_asset(mi_full)
        size_created += 1

created += size_created
unreal.log(f"  Part 3 done: {size_created} tile-size variants")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {created} total created, {skipped} skipped")
unreal.log(f"  Browse: {MI_BASE}/")
unreal.log(f"    /individual/ — 608 textures with triplanar projection")
unreal.log(f"    /paired/     — ground+cliff combos (triplanar)")
unreal.log(f"    /tile_sizes/ — first 50 textures at 4 scales (triplanar)")
unreal.log(f"{'='*60}")
