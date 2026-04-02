# setup_grass_v2.py
# Imports sprite billboard meshes + sprite textures, creates alpha-masked
# materials, applies them to meshes, and creates GrassType assets.
# Everything goes in GrassV2/ — separate from the original V1 system.

import unreal
import os
import glob

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MESH_SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/GrassV2/Meshes"
TEX_SRC = "C:/Sabri_MMO/_tools/ground_texture_output/grass_sprites"
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
unreal.log("=== Step 1: Importing billboard cross meshes ===")
fbx_files = sorted(glob.glob(os.path.join(MESH_SRC, "*.fbx")))
tasks = []
for fbx in fbx_files:
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
# STEP 2: Copy sprite textures to Content + import
# ============================================================
unreal.log("\n=== Step 2: Importing sprite textures ===")

# Copy PNGs to Content folder first
tex_content_dir = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/GrassV2/Textures"
os.makedirs(tex_content_dir, exist_ok=True)
import shutil
sprite_pngs = sorted(glob.glob(os.path.join(TEX_SRC, "S_*.png")))
for png in sprite_pngs:
    dest = os.path.join(tex_content_dir, os.path.basename(png))
    if not os.path.exists(dest):
        shutil.copy2(png, dest)

# Import into UE5
tasks = []
for png in glob.glob(os.path.join(tex_content_dir, "*.png")):
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
# STEP 3: Create alpha-masked materials
# ============================================================
unreal.log("\n=== Step 3: Creating alpha-masked materials ===")

# Map sprite textures to meshes
SPRITE_MAP = {
    "SM_Sprite_Grass_01": "S_GrassBlade_01",
    "SM_Sprite_Grass_02": "S_GrassBlade_02",
    "SM_Sprite_Grass_03": "S_GrassBlade_03",
    "SM_Sprite_Grass_04": "S_GrassClump_01",
    "SM_Sprite_Grass_05": "S_GrassClump_02",
    "SM_Sprite_Flower_01": "S_Flower_01",
    "SM_Sprite_Flower_02": "S_Flower_02",
    "SM_Sprite_Flower_03": "S_Flower_03",
    "SM_Sprite_Flower_04": "S_Dandelion_01",
    "SM_Sprite_Leaf_01": "S_FallenLeaf_01",
    "SM_Sprite_Leaf_02": "S_FallenLeaf_02",
    "SM_Sprite_Leaf_03": "S_CherryPetal_01",
    "SM_Sprite_Leaf_04": "S_BambooLeaf_01",
    "SM_Sprite_Leaf_05": "S_BirchLeaf_01",
    "SM_Sprite_Fern_01": "S_Fern_01",
    "SM_Sprite_Fern_02": "S_Weed_01",
    "SM_Sprite_DryGrass_01": "S_DryGrass_01",
    "SM_Sprite_DryGrass_02": "S_DeadGrass_01",
    "SM_Sprite_Seaweed_01": "S_Seaweed_01",
}

mat_created = 0
for mesh_name, tex_name in SPRITE_MAP.items():
    mat_name = f"M_{mesh_name}"
    mat_full = f"{MAT_DEST}/{mat_name}"

    if eal.does_asset_exist(mat_full):
        mat = unreal.load_asset(mat_full)
        mel.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(mat_name, MAT_DEST,
            unreal.Material, unreal.MaterialFactoryNew())

    if not mat:
        continue

    # Alpha masked, two-sided, no specular
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.33)

    # Texture sample using mesh UVs (NOT world position — these have proper UVs)
    tex_sample = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
    tex_sample.set_editor_property("parameter_name", "SpriteTexture")

    # Try to assign the sprite texture
    tex_path = f"{TEX_DEST}/{tex_name}"
    if eal.does_asset_exist(tex_path):
        tex = unreal.load_asset(tex_path)
        if tex:
            tex_sample.set_editor_property("texture", tex)

    # Connect RGB to BaseColor
    mel.connect_material_property(tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    # Connect Alpha to OpacityMask
    mel.connect_material_property(tex_sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    # Roughness
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 100)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(mat_full)

    # Apply material to mesh
    mesh_path = f"{MESH_DEST}/{mesh_name}"
    if eal.does_asset_exist(mesh_path):
        mesh = unreal.load_asset(mesh_path)
        if mesh and isinstance(mesh, unreal.StaticMesh):
            mesh.set_material(0, mat)
            eal.save_asset(mesh_path)

    mat_created += 1

unreal.log(f"  {mat_created} materials created and applied")

# ============================================================
# STEP 4: Create GrassType assets
# ============================================================
unreal.log("\n=== Step 4: Creating GrassType assets ===")

GT_CONFIGS = {
    "GT_V2_Grassland": [
        ("SM_Sprite_Grass_01", 10.0, 3000, 5000),
        ("SM_Sprite_Grass_02", 8.0, 2500, 4500),
        ("SM_Sprite_Grass_03", 6.0, 2500, 4000),
        ("SM_Sprite_Grass_04", 4.0, 2500, 4000),
        ("SM_Sprite_Flower_01", 1.5, 2000, 3500),
        ("SM_Sprite_Flower_02", 1.0, 2000, 3500),
        ("SM_Sprite_Fern_01", 2.0, 2000, 3500),
    ],
    "GT_V2_Forest": [
        ("SM_Sprite_Grass_05", 6.0, 2500, 4000),
        ("SM_Sprite_Fern_01", 4.0, 2000, 3500),
        ("SM_Sprite_Fern_02", 3.0, 2000, 3500),
        ("SM_Sprite_Leaf_01", 5.0, 1500, 3000),
        ("SM_Sprite_Leaf_02", 4.0, 1500, 3000),
    ],
    "GT_V2_Desert": [
        ("SM_Sprite_DryGrass_01", 5.0, 2500, 4000),
        ("SM_Sprite_DryGrass_02", 3.0, 2000, 3500),
    ],
    "GT_V2_Amatsu": [
        ("SM_Sprite_Grass_01", 8.0, 2500, 4000),
        ("SM_Sprite_Leaf_03", 6.0, 1500, 3000),  # cherry petals
        ("SM_Sprite_Leaf_04", 4.0, 1500, 3000),  # bamboo
        ("SM_Sprite_Flower_03", 1.5, 2000, 3500),
    ],
}

SCALE_MIN = 50.0
SCALE_MAX = 150.0

gt_created = 0
for gt_name, varieties_config in GT_CONFIGS.items():
    gt_path = f"{GT_DEST}/{gt_name}"

    if eal.does_asset_exist(gt_path):
        gt = unreal.load_asset(gt_path)
    else:
        gt = asset_tools.create_asset(gt_name, GT_DEST,
            unreal.LandscapeGrassType, unreal.LandscapeGrassTypeFactory())

    if not gt:
        continue

    new_varieties = unreal.Array(unreal.GrassVariety)

    for mesh_name, density, cull_start, cull_end in varieties_config:
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
        v.set_editor_property("scale_x", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
        v.set_editor_property("scale_y", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
        v.set_editor_property("scale_z", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
        v.set_editor_property("random_rotation", True)
        v.set_editor_property("align_to_surface", True)
        v.set_editor_property("use_landscape_lightmap", True)
        new_varieties.append(v)

    gt.set_editor_property("grass_varieties", new_varieties)
    gt.modify()
    eal.save_asset(gt_path)
    gt_created += 1
    unreal.log(f"  {gt_name}: {len(new_varieties)} varieties")

unreal.log(f"\n{'='*60}")
unreal.log(f"  GRASS V2 SETUP COMPLETE")
unreal.log(f"  Meshes: {MESH_DEST}")
unreal.log(f"  Textures: {TEX_DEST}")
unreal.log(f"  Materials: {MAT_DEST} ({mat_created} alpha-masked)")
unreal.log(f"  GrassTypes: {GT_DEST} ({gt_created} zone types)")
unreal.log(f"")
unreal.log(f"  To use: add Grass Output to landscape material,")
unreal.log(f"  pick GT_V2_Grassland / GT_V2_Forest / etc.")
unreal.log(f"  Each mesh shows a painted sprite with alpha cutout")
unreal.log(f"{'='*60}")
