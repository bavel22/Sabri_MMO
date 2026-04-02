# setup_grass_v3.py
# Full V3 grass system — imports sprite textures, creates billboard meshes
# (reuses V2 meshes), creates alpha-masked materials, and creates per-zone GrassTypes.
# Everything in GrassV3/ — separate from V1 and V2.

import unreal
import os
import glob

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# V3 uses V2 billboard meshes (they have proper UVs)
V2_MESH = "/Game/SabriMMO/Environment/GrassV2/Meshes"
TEX_SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/GrassV3/Textures"
TEX_DEST = "/Game/SabriMMO/Environment/GrassV3/Textures"
MAT_DEST = "/Game/SabriMMO/Environment/GrassV3/Materials"
GT_DEST = "/Game/SabriMMO/Environment/GrassV3/Types"

for d in [TEX_DEST, MAT_DEST, GT_DEST]:
    eal.make_directory(d)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# ============================================================
# STEP 1: Import sprite textures
# ============================================================
unreal.log("=== Step 1: Import V3 sprite textures ===")
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
# STEP 2: Create alpha-masked materials
# ============================================================
unreal.log("\n=== Step 2: Create materials ===")

# Get all sprite textures
all_textures = []
tex_assets = eal.list_assets(TEX_DEST, recursive=False)
for a in sorted(tex_assets):
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    if name.startswith("S_"):
        all_textures.append((name, clean))

# Billboard mesh to use (grass-sized from V2)
GRASS_MESH = f"{V2_MESH}/SM_Sprite_Grass_01"
FLOWER_MESH = f"{V2_MESH}/SM_Sprite_Flower_01"
LEAF_MESH = f"{V2_MESH}/SM_Sprite_Leaf_01"

mat_count = 0
mat_to_mesh = {}

for tex_name, tex_path in all_textures:
    mat_name = f"M_V3_{tex_name}"
    mat_full = f"{MAT_DEST}/{mat_name}"

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

    # Texture sample
    tex_sample = mel.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
    tex_sample.set_editor_property("parameter_name", "SpriteTexture")

    tex = unreal.load_asset(tex_path)
    if tex and isinstance(tex, unreal.Texture2D):
        tex_sample.set_editor_property("texture", tex)

    mel.connect_material_property(tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(tex_sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 100)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(mat_full)

    # Pick appropriate mesh based on sprite name
    if "Grass" in tex_name or "Fern" in tex_name or "DryGrass" in tex_name or "DeadGrass" in tex_name or "Seaweed" in tex_name or "Weed" in tex_name:
        mesh_path = GRASS_MESH
    elif "Flower" in tex_name or "Dandelion" in tex_name or "Cherry" in tex_name:
        mesh_path = FLOWER_MESH
    else:
        mesh_path = LEAF_MESH

    mat_to_mesh[tex_name] = (mat_full, mesh_path)
    mat_count += 1

unreal.log(f"  {mat_count} materials created")

# ============================================================
# STEP 3: Create per-zone GrassTypes
# ============================================================
unreal.log("\n=== Step 3: Create per-zone GrassTypes ===")

# Map sprites to zones
ZONE_SPRITES = {
    "GT_V3_Grassland": {
        "sprites": ["S_Grass_Clump_A", "S_Grass_Clump_B", "S_Grass_Clump_C", "S_GrassClump_01", "S_GrassClump_02",
                     "S_Flower_Daisy", "S_Flower_Pink", "S_Flower_Yellow", "S_Dandelion", "S_Clover", "S_Grass_Tall"],
        "densities": [8, 6, 5, 5, 4, 1.5, 1, 1, 0.5, 2, 3],
    },
    "GT_V3_Forest": {
        "sprites": ["S_Fern_A", "S_Fern_B", "S_Fern_01", "S_Mushroom_Brown", "S_Mushroom_Red", "S_Mushroom_White", "S_Mushroom_02",
                     "S_Leaf_Autumn_A", "S_Leaf_Autumn_B", "S_Leaf_Green", "S_MossClump", "S_PineCone", "S_Twig"],
        "densities": [3, 2, 2, 1, 0.5, 0.5, 0.5, 4, 3, 3, 2, 1, 1.5],
    },
    "GT_V3_Desert": {
        "sprites": ["S_DryGrass_A", "S_DryGrass_B", "S_Cactus_Small", "S_DesertRock", "S_DeadBranch"],
        "densities": [4, 3, 0.3, 2, 1],
    },
    "GT_V3_Snow": {
        "sprites": ["S_SnowClump", "S_IceCrystal", "S_FrozenPlant"],
        "densities": [5, 1.5, 2],
    },
    "GT_V3_Beach": {
        "sprites": ["S_Shell_A", "S_Shell_B", "S_Starfish", "S_CoralPiece", "S_Seaweed"],
        "densities": [2, 2, 0.5, 1, 1.5],
    },
    "GT_V3_Volcanic": {
        "sprites": ["S_LavaRock", "S_ObsidianShard", "S_SulfurCrystal"],
        "densities": [3, 2, 1],
    },
    "GT_V3_Cursed": {
        "sprites": ["S_Bone_A", "S_Bone_B", "S_DeadGrass", "S_CandleStub"],
        "densities": [1.5, 0.5, 4, 0.3],
    },
    "GT_V3_Amatsu": {
        "sprites": ["S_CherryPetal_A", "S_CherryPetal_B", "S_BambooLeaf",
                     "S_Grass_Clump_A", "S_Flower_Pink"],
        "densities": [5, 4, 3, 6, 1],
    },
    "GT_V3_Moscovia": {
        "sprites": ["S_BirchLeaf", "S_Pinecone_B", "S_Berry",
                     "S_Leaf_Autumn_A", "S_Fern_A", "S_MossClump"],
        "densities": [4, 1, 0.5, 3, 2, 2],
    },
    "GT_V3_Ruins": {
        "sprites": ["S_StoneFrag", "S_PotteryShard", "S_DryGrass_A"],
        "densities": [3, 1.5, 2],
    },
    "GT_V3_Industrial": {
        "sprites": ["S_MetalScrap", "S_GearPiece"],
        "densities": [2, 1.5],
    },
    "GT_V3_Cave": {
        "sprites": ["S_Crystal_Blue", "S_Crystal_Green", "S_Stalagmite"],
        "densities": [1.5, 1, 2],
    },
    "GT_V3_Dungeon": {
        "sprites": ["S_ChainPiece", "S_Rubble", "S_Bone_A"],
        "densities": [0.5, 3, 1],
    },
}

gt_count = 0
mi_factory = unreal.MaterialInstanceConstantFactoryNew()

for gt_name, config in ZONE_SPRITES.items():
    gt_path = f"{GT_DEST}/{gt_name}"

    if eal.does_asset_exist(gt_path):
        gt = unreal.load_asset(gt_path)
    else:
        gt = asset_tools.create_asset(gt_name, GT_DEST,
            unreal.LandscapeGrassType, unreal.LandscapeGrassTypeFactory())

    if not gt:
        continue

    new_varieties = unreal.Array(unreal.GrassVariety)
    added = 0

    for sprite_name, density in zip(config["sprites"], config["densities"]):
        if sprite_name not in mat_to_mesh:
            continue

        mat_path, mesh_path = mat_to_mesh[sprite_name]

        if not eal.does_asset_exist(mesh_path):
            continue

        mesh = unreal.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            continue

        # Apply the material to this mesh variant
        mat = unreal.load_asset(mat_path)
        if mat:
            mesh.set_material(0, mat)

        v = unreal.GrassVariety()
        v.set_editor_property("grass_mesh", mesh)
        v.set_editor_property("grass_density", unreal.PerPlatformFloat(default=density))
        v.set_editor_property("start_cull_distance", unreal.PerPlatformInt(default=3000))
        v.set_editor_property("end_cull_distance", unreal.PerPlatformInt(default=5000))
        v.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
        v.set_editor_property("scale_x", unreal.FloatInterval(min=0.5, max=2.0))
        v.set_editor_property("scale_y", unreal.FloatInterval(min=0.5, max=2.0))
        v.set_editor_property("scale_z", unreal.FloatInterval(min=0.5, max=2.0))
        v.set_editor_property("random_rotation", True)
        v.set_editor_property("align_to_surface", True)
        v.set_editor_property("use_landscape_lightmap", True)
        new_varieties.append(v)
        added += 1

    gt.set_editor_property("grass_varieties", new_varieties)
    gt.modify()
    eal.save_asset(gt_path)
    gt_count += 1
    unreal.log(f"  {gt_name}: {added} varieties")

unreal.log(f"\n{'='*60}")
unreal.log(f"  GRASS V3 COMPLETE")
unreal.log(f"  Textures: {len(all_textures)}")
unreal.log(f"  Materials: {mat_count}")
unreal.log(f"  GrassTypes: {gt_count} zones")
unreal.log(f"  Location: /Game/SabriMMO/Environment/GrassV3/")
unreal.log(f"")
unreal.log(f"  To use: add any GT_V3_* to Grass Output in landscape material")
unreal.log(f"  Each sprite shows a painted object with alpha cutout")
unreal.log(f"{'='*60}")
