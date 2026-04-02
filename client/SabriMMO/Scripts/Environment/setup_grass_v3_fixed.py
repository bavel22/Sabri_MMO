# setup_grass_v3_fixed.py
# Creates the full V3 grass system. Uses an existing StaticMesh from the
# engine or from V1 grass system as the billboard carrier, applies sprite
# textures via alpha-masked materials.

import unreal
import os
import glob

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

TEX_DEST = "/Game/SabriMMO/Environment/GrassV3/Textures"
MAT_DEST = "/Game/SabriMMO/Environment/GrassV3/Materials"
GT_DEST = "/Game/SabriMMO/Environment/GrassV3/Types"

for d in [MAT_DEST, GT_DEST]:
    eal.make_directory(d)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Find a working billboard mesh — check multiple locations
MESH_CANDIDATES = [
    "/Game/SabriMMO/Environment/GrassV2/Meshes/SM_Sprite_Grass_01",
    "/Game/SabriMMO/Environment/GrassV2/Meshes/SM_Sprite_Flower_01",
    "/Game/SabriMMO/Environment/GrassV2/Meshes/SM_Sprite_Leaf_01",
    "/Game/SabriMMO/Environment/Grass/Meshes/SM_GrassBlade_01",
    "/Game/SabriMMO/Environment/Grass/Meshes/SM_GrassBlade_04",
    "/Game/SabriMMO/Environment/Grass/Meshes/SM_Flower_01",
    "/Engine/BasicShapes/Plane",
]

billboard_mesh_path = None
for candidate in MESH_CANDIDATES:
    if eal.does_asset_exist(candidate):
        obj = unreal.load_asset(candidate)
        if obj and isinstance(obj, unreal.StaticMesh):
            billboard_mesh_path = candidate
            unreal.log(f"Using mesh: {candidate}")
            break

if not billboard_mesh_path:
    unreal.log_error("No suitable mesh found!")
    raise RuntimeError("No mesh")

billboard_mesh = unreal.load_asset(billboard_mesh_path)

# ============================================================
# STEP 1: Get all sprite textures
# ============================================================
unreal.log("\n=== Step 1: Finding sprite textures ===")
all_sprites = {}
tex_assets = eal.list_assets(TEX_DEST, recursive=False)
for a in sorted(tex_assets):
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    obj = unreal.load_asset(clean)
    if obj and isinstance(obj, unreal.Texture2D) and name.startswith("S_"):
        all_sprites[name] = clean
unreal.log(f"  Found {len(all_sprites)} sprite textures")

# ============================================================
# STEP 2: Create one material per sprite
# ============================================================
unreal.log("\n=== Step 2: Creating alpha-masked materials ===")

mat_map = {}  # sprite_name -> material_path

for sprite_name, tex_path in all_sprites.items():
    mat_name = f"M_V3_{sprite_name}"
    mat_full = f"{MAT_DEST}/{mat_name}"

    if eal.does_asset_exist(mat_full):
        mat = unreal.load_asset(mat_full)
        if mat and isinstance(mat, unreal.Material):
            mel.delete_all_material_expressions(mat)
        else:
            continue
    else:
        mat = asset_tools.create_asset(mat_name, MAT_DEST,
            unreal.Material, unreal.MaterialFactoryNew())

    if not mat:
        continue

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.33)

    tex_sample = mel.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
    tex_sample.set_editor_property("parameter_name", "SpriteTexture")

    tex = unreal.load_asset(tex_path)
    if tex:
        tex_sample.set_editor_property("texture", tex)

    mel.connect_material_property(tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(tex_sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 100)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(mat_full)
    mat_map[sprite_name] = mat_full

unreal.log(f"  Created {len(mat_map)} materials")

# ============================================================
# STEP 3: Create per-zone GrassTypes
# Each variety uses the SAME mesh but a DIFFERENT material
# The mesh gets the material applied when the GrassType is used
# ============================================================
unreal.log("\n=== Step 3: Creating per-zone GrassTypes ===")

# Since all varieties share the same mesh but need different materials,
# we create Material Instances and apply them.
# But GrassType uses the mesh's material slot 0 — so all varieties
# would show the same texture.
#
# WORKAROUND: Create a separate StaticMesh copy per sprite by
# duplicating the billboard mesh and applying the material to each copy.

mi_factory = unreal.MaterialInstanceConstantFactoryNew()
MESH_COPY_DEST = "/Game/SabriMMO/Environment/GrassV3/Meshes"
eal.make_directory(MESH_COPY_DEST)

# Create mesh copies with unique materials
sprite_meshes = {}  # sprite_name -> mesh_path

for sprite_name, mat_path in mat_map.items():
    copy_name = f"SM_V3_{sprite_name}"
    copy_full = f"{MESH_COPY_DEST}/{copy_name}"

    if not eal.does_asset_exist(copy_full):
        # Duplicate the billboard mesh
        new_asset = eal.duplicate_asset(billboard_mesh_path, copy_full)
        if not new_asset:
            continue

    # Load and apply material
    mesh_copy = unreal.load_asset(copy_full)
    mat = unreal.load_asset(mat_path)
    if mesh_copy and mat and isinstance(mesh_copy, unreal.StaticMesh):
        mesh_copy.set_material(0, mat)
        eal.save_asset(copy_full)
        sprite_meshes[sprite_name] = copy_full

unreal.log(f"  Created {len(sprite_meshes)} mesh copies with materials")

# Now create GrassTypes using the unique mesh copies
ZONE_SPRITES = {
    "GT_V3_Grassland": [
        ("S_Grass_Clump_A", 8.0), ("S_Grass_Clump_B", 6.0), ("S_Grass_Clump_C", 5.0),
        ("S_GrassClump_01", 5.0), ("S_GrassClump_02", 4.0),
        ("S_Flower_Daisy", 1.5), ("S_Flower_Pink", 1.0), ("S_Flower_Yellow", 1.0),
        ("S_Dandelion", 0.5), ("S_Clover", 2.0), ("S_Grass_Tall", 3.0),
    ],
    "GT_V3_Forest": [
        ("S_Fern_A", 3.0), ("S_Fern_B", 2.0), ("S_Fern_01", 2.0),
        ("S_Mushroom_Brown", 1.0), ("S_Mushroom_Red", 0.5), ("S_Mushroom_White", 0.5), ("S_Mushroom_02", 0.5),
        ("S_Leaf_Autumn_A", 4.0), ("S_Leaf_Autumn_B", 3.0), ("S_Leaf_Green", 3.0),
        ("S_MossClump", 2.0), ("S_PineCone", 1.0), ("S_Twig", 1.5),
    ],
    "GT_V3_Desert": [
        ("S_DryGrass_A", 4.0), ("S_DryGrass_B", 3.0),
        ("S_Cactus_Small", 0.3), ("S_DesertRock", 2.0), ("S_DeadBranch", 1.0),
    ],
    "GT_V3_Snow": [
        ("S_SnowClump", 5.0), ("S_IceCrystal", 1.5), ("S_FrozenPlant", 2.0),
    ],
    "GT_V3_Beach": [
        ("S_Shell_A", 2.0), ("S_Shell_B", 2.0), ("S_Starfish", 0.5),
        ("S_CoralPiece", 1.0), ("S_Seaweed", 1.5),
    ],
    "GT_V3_Volcanic": [
        ("S_LavaRock", 3.0), ("S_ObsidianShard", 2.0), ("S_SulfurCrystal", 1.0),
    ],
    "GT_V3_Cursed": [
        ("S_Bone_A", 1.5), ("S_Bone_B", 0.5), ("S_DeadGrass", 4.0), ("S_CandleStub", 0.3),
    ],
    "GT_V3_Amatsu": [
        ("S_CherryPetal_A", 5.0), ("S_CherryPetal_B", 4.0), ("S_BambooLeaf", 3.0),
        ("S_Grass_Clump_A", 6.0), ("S_Flower_Pink", 1.0),
    ],
    "GT_V3_Moscovia": [
        ("S_BirchLeaf", 4.0), ("S_Pinecone_B", 1.0), ("S_Berry", 0.5),
        ("S_Leaf_Autumn_A", 3.0), ("S_Fern_A", 2.0), ("S_MossClump", 2.0),
    ],
    "GT_V3_Ruins": [
        ("S_StoneFrag", 3.0), ("S_PotteryShard", 1.5), ("S_DryGrass_A", 2.0),
    ],
    "GT_V3_Industrial": [
        ("S_MetalScrap", 2.0), ("S_GearPiece", 1.5),
    ],
    "GT_V3_Cave": [
        ("S_Crystal_Blue", 1.5), ("S_Crystal_Green", 1.0), ("S_Stalagmite", 2.0),
    ],
    "GT_V3_Dungeon": [
        ("S_ChainPiece", 0.5), ("S_Rubble", 3.0), ("S_Bone_A", 1.0),
    ],
}

gt_count = 0
for gt_name, varieties_list in ZONE_SPRITES.items():
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

    for sprite_name, density in varieties_list:
        if sprite_name not in sprite_meshes:
            continue

        mesh_path = sprite_meshes[sprite_name]
        mesh = unreal.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            continue

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
unreal.log(f"  Sprites: {len(all_sprites)}")
unreal.log(f"  Materials: {len(mat_map)}")
unreal.log(f"  Mesh copies: {len(sprite_meshes)}")
unreal.log(f"  GrassTypes: {gt_count} zones")
unreal.log(f"  Location: /Game/SabriMMO/Environment/GrassV3/")
unreal.log(f"{'='*60}")
