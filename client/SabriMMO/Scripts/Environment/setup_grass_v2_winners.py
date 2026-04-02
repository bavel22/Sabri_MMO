# setup_grass_v2_winners.py
# Imports the best AI-generated sprite textures, creates alpha-masked materials,
# applies to billboard cross meshes, and creates GrassType for landscape.
# Everything in GrassV2/ — separate from V1.

import unreal
import os
import glob

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MESH_SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/GrassV2/Meshes"
TEX_SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/GrassV2/Textures"
MESH_DEST = "/Game/SabriMMO/Environment/GrassV2/Meshes"
TEX_DEST = "/Game/SabriMMO/Environment/GrassV2/Textures"
MAT_DEST = "/Game/SabriMMO/Environment/GrassV2/Materials"
GT_DEST = "/Game/SabriMMO/Environment/GrassV2/Types"

for d in [MESH_DEST, TEX_DEST, MAT_DEST, GT_DEST]:
    eal.make_directory(d)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# ============================================================
# STEP 1: Import meshes
# ============================================================
unreal.log("=== Step 1: Import billboard meshes ===")
tasks = []
for fbx in sorted(glob.glob(os.path.join(MESH_SRC, "*.fbx"))):
    name = os.path.splitext(os.path.basename(fbx))[0]
    if not eal.does_asset_exist(f"{MESH_DEST}/{name}"):
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", MESH_DEST)
        t.set_editor_property("filename", fbx)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
if tasks:
    asset_tools.import_asset_tasks(tasks)
unreal.log(f"  {len(tasks)} meshes imported")

# ============================================================
# STEP 2: Import sprite textures
# ============================================================
unreal.log("\n=== Step 2: Import sprite textures ===")
tasks = []
for png in sorted(glob.glob(os.path.join(TEX_SRC, "*.png"))):
    name = os.path.splitext(os.path.basename(png))[0]
    if not eal.does_asset_exist(f"{TEX_DEST}/{name}"):
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("filename", png)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
if tasks:
    asset_tools.import_asset_tasks(tasks)
unreal.log(f"  {len(tasks)} textures imported")

# ============================================================
# STEP 3: Create alpha-masked materials + apply to meshes
# ============================================================
unreal.log("\n=== Step 3: Create materials ===")

# Map: (mesh_name, sprite_texture_name)
ASSIGNMENTS = [
    ("SM_Sprite_Grass_01", "S_GrassClump_01"),
    ("SM_Sprite_Grass_02", "S_GrassClump_02"),
    ("SM_Sprite_Grass_03", "S_GrassClump_01"),  # reuse
    ("SM_Sprite_Grass_04", "S_GrassClump_02"),  # reuse
    ("SM_Sprite_Grass_05", "S_GrassClump_01"),  # reuse
    ("SM_Sprite_Flower_01", "S_Flower_01"),
    ("SM_Sprite_Flower_02", "S_CherryPetal_01"),
    ("SM_Sprite_Leaf_01", "S_Leaf_Autumn"),
    ("SM_Sprite_Leaf_02", "S_Fern_01"),
    ("SM_Sprite_Fern_01", "S_Fern_01"),
    ("SM_Sprite_Fern_02", "S_Mushroom_01"),
]

mat_count = 0
for mesh_name, tex_name in ASSIGNMENTS:
    mat_name = f"M_V2_{mesh_name}"
    mat_full = f"{MAT_DEST}/{mat_name}"

    # Create or rebuild material
    if eal.does_asset_exist(mat_full):
        mat = unreal.load_asset(mat_full)
        mel.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(mat_name, MAT_DEST,
            unreal.Material, unreal.MaterialFactoryNew())

    if not mat:
        continue

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.33)

    # Texture sample using mesh UVs (billboard cross has proper 0-1 UVs)
    tex_sample = mel.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
    tex_sample.set_editor_property("parameter_name", "SpriteTexture")

    # Assign texture
    tex_path = f"{TEX_DEST}/{tex_name}"
    if eal.does_asset_exist(tex_path):
        tex = unreal.load_asset(tex_path)
        if tex:
            tex_sample.set_editor_property("texture", tex)

    # RGB -> BaseColor
    mel.connect_material_property(tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    # Alpha -> OpacityMask
    mel.connect_material_property(tex_sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    # Roughness
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 100)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(mat_full)

    # Apply to mesh
    mesh_path = f"{MESH_DEST}/{mesh_name}"
    if eal.does_asset_exist(mesh_path):
        mesh = unreal.load_asset(mesh_path)
        if mesh and isinstance(mesh, unreal.StaticMesh):
            mesh.set_material(0, mat)
            eal.save_asset(mesh_path)

    mat_count += 1
    unreal.log(f"  {mesh_name} -> {tex_name}")

# ============================================================
# STEP 4: Create GrassType
# ============================================================
unreal.log("\n=== Step 4: Create GrassType ===")

gt_name = "GT_V2_Test"
gt_path = f"{GT_DEST}/{gt_name}"

if eal.does_asset_exist(gt_path):
    gt = unreal.load_asset(gt_path)
else:
    gt = asset_tools.create_asset(gt_name, GT_DEST,
        unreal.LandscapeGrassType, unreal.LandscapeGrassTypeFactory())

if gt:
    configs = [
        ("SM_Sprite_Grass_01", 8.0, 3000, 5000),
        ("SM_Sprite_Grass_02", 6.0, 2500, 4500),
        ("SM_Sprite_Flower_01", 1.5, 2000, 3500),
        ("SM_Sprite_Flower_02", 1.0, 2000, 3500),
        ("SM_Sprite_Leaf_01", 3.0, 1500, 3000),
        ("SM_Sprite_Fern_01", 2.0, 2000, 3500),
    ]

    new_varieties = unreal.Array(unreal.GrassVariety)
    for mesh_name, density, cull_start, cull_end in configs:
        mesh_path = f"{MESH_DEST}/{mesh_name}"
        if not eal.does_asset_exist(mesh_path):
            continue
        mesh = unreal.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            continue

        v = unreal.GrassVariety()
        v.set_editor_property("grass_mesh", mesh)
        v.set_editor_property("grass_density", unreal.PerPlatformFloat(default=density))
        v.set_editor_property("start_cull_distance", unreal.PerPlatformInt(default=cull_start))
        v.set_editor_property("end_cull_distance", unreal.PerPlatformInt(default=cull_end))
        v.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
        v.set_editor_property("scale_x", unreal.FloatInterval(min=50.0, max=150.0))
        v.set_editor_property("scale_y", unreal.FloatInterval(min=50.0, max=150.0))
        v.set_editor_property("scale_z", unreal.FloatInterval(min=50.0, max=150.0))
        v.set_editor_property("random_rotation", True)
        v.set_editor_property("align_to_surface", True)
        v.set_editor_property("use_landscape_lightmap", True)
        new_varieties.append(v)

    gt.set_editor_property("grass_varieties", new_varieties)
    gt.modify()
    eal.save_asset(gt_path)
    unreal.log(f"  {gt_name}: {len(new_varieties)} varieties")

unreal.log(f"\n{'='*60}")
unreal.log(f"  GRASS V2 READY")
unreal.log(f"  Materials: {mat_count} alpha-masked")
unreal.log(f"  GrassType: {gt_path}")
unreal.log(f"")
unreal.log(f"  To test: add GT_V2_Test to Grass Output in M_Landscape_RO_15")
unreal.log(f"  The meshes now show actual painted sprites with alpha cutout")
unreal.log(f"{'='*60}")
