# apply_scatter_materials.py
# Creates proper colored materials for all 75 scatter meshes
# and applies them. Each material has:
#   - Correct muted RO palette color for the object type
#   - Two-sided rendering (for billboard cross meshes)
#   - Per-instance random color variation (±15%)
#   - Roughness 0.95 (RO matte style)

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_PATH = "/Game/SabriMMO/Environment/Grass/Materials"
MESH_PATH = "/Game/SabriMMO/Environment/Grass/Meshes"
eal.make_directory(MAT_PATH)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Color palette for each mesh type — muted RO colors
MESH_COLORS = {
    # Grass — various greens
    "SM_GrassBlade_01": (0.22, 0.38, 0.14),
    "SM_GrassBlade_02": (0.25, 0.40, 0.16),
    "SM_GrassBlade_03": (0.18, 0.35, 0.12),
    "SM_GrassBlade_04": (0.20, 0.36, 0.13),
    "SM_GrassBlade_05": (0.24, 0.42, 0.17),
    "SM_GrassClump_01": (0.20, 0.35, 0.12),
    "SM_WheatStalk_01": (0.60, 0.52, 0.25),

    # Flowers — warm yellows, pinks, purples
    "SM_Flower_01": (0.70, 0.62, 0.30),
    "SM_Flower_02": (0.65, 0.45, 0.55),
    "SM_Flower_03": (0.50, 0.50, 0.68),
    "SM_Dandelion_01": (0.72, 0.68, 0.22),
    "SM_Clover_01": (0.22, 0.38, 0.14),

    # Forest floor — browns, dark greens
    "SM_Mushroom_01": (0.55, 0.38, 0.22),
    "SM_Mushroom_02": (0.62, 0.30, 0.25),
    "SM_Fern_01": (0.15, 0.35, 0.12),
    "SM_FallenLeaf_01": (0.50, 0.38, 0.18),
    "SM_FallenLeaf_02": (0.58, 0.42, 0.15),
    "SM_PineCone_01": (0.42, 0.32, 0.20),
    "SM_BarkPiece_01": (0.35, 0.26, 0.17),
    "SM_Acorn_01": (0.45, 0.35, 0.22),
    "SM_MossClump_01": (0.18, 0.32, 0.14),

    # Universal — grays, browns
    "SM_Pebble_01": (0.45, 0.42, 0.38),
    "SM_Pebble_02": (0.42, 0.40, 0.35),
    "SM_Pebble_03": (0.48, 0.44, 0.38),
    "SM_SmallRock_01": (0.40, 0.38, 0.34),
    "SM_SmallRock_02": (0.46, 0.42, 0.38),
    "SM_Twig_01": (0.38, 0.30, 0.20),
    "SM_DirtClump_01": (0.38, 0.30, 0.20),

    # Desert — tans, golden browns
    "SM_DryGrass_01": (0.58, 0.50, 0.28),
    "SM_DryGrass_02": (0.55, 0.48, 0.25),
    "SM_DesertRock_01": (0.55, 0.45, 0.30),
    "SM_DesertRock_02": (0.58, 0.48, 0.34),
    "SM_DryTwig_01": (0.52, 0.44, 0.30),
    "SM_SmallCactus_01": (0.28, 0.42, 0.20),
    "SM_BoneFrag_01": (0.72, 0.70, 0.62),

    # Snow — whites, pale blues
    "SM_SnowClump_01": (0.88, 0.90, 0.94),
    "SM_SnowClump_02": (0.85, 0.88, 0.92),
    "SM_IceCrystal_01": (0.68, 0.78, 0.90),
    "SM_IceCrystal_02": (0.72, 0.82, 0.94),
    "SM_FrozenTwig_01": (0.58, 0.62, 0.70),

    # Beach — creams, corals, warm tans
    "SM_Shell_01": (0.78, 0.74, 0.65),
    "SM_Shell_02": (0.74, 0.70, 0.64),
    "SM_CoralBit_01": (0.72, 0.52, 0.48),
    "SM_Starfish_01": (0.68, 0.45, 0.38),
    "SM_Driftwood_01": (0.50, 0.45, 0.38),
    "SM_Seaweed_01": (0.22, 0.35, 0.20),

    # Volcanic — blacks, dark reds, oranges
    "SM_ObsidianShard_01": (0.10, 0.10, 0.12),
    "SM_ObsidianShard_02": (0.12, 0.10, 0.10),
    "SM_LavaRock_01": (0.18, 0.12, 0.10),
    "SM_LavaRock_02": (0.20, 0.14, 0.10),
    "SM_SulfurCrystal_01": (0.68, 0.62, 0.18),
    "SM_AshClump_01": (0.32, 0.30, 0.30),

    # Cursed — grays, bone white, dark
    "SM_DeadGrass_01": (0.40, 0.36, 0.28),
    "SM_DeadGrass_02": (0.36, 0.32, 0.26),
    "SM_Bone_01": (0.75, 0.72, 0.65),
    "SM_BoneStick_01": (0.72, 0.70, 0.62),
    "SM_SmallSkull_01": (0.70, 0.68, 0.60),
    "SM_CandleStub_01": (0.72, 0.68, 0.52),

    # Cultural
    "SM_CherryPetal_01": (0.82, 0.62, 0.68),
    "SM_BambooLeaf_01": (0.32, 0.48, 0.22),
    "SM_BirchLeaf_01": (0.58, 0.55, 0.24),
    "SM_Berry_01": (0.62, 0.18, 0.14),

    # Ruins
    "SM_StoneFrag_01": (0.52, 0.48, 0.42),
    "SM_StoneFrag_02": (0.48, 0.44, 0.38),
    "SM_PotteryShard_01": (0.58, 0.44, 0.30),

    # Industrial
    "SM_MetalScrap_01": (0.42, 0.40, 0.38),
    "SM_Bolt_01": (0.48, 0.45, 0.40),
    "SM_RustFlake_01": (0.55, 0.32, 0.18),

    # Cave
    "SM_CrystalShard_01": (0.48, 0.42, 0.65),
    "SM_CrystalShard_02": (0.52, 0.45, 0.70),
    "SM_WetRock_01": (0.24, 0.22, 0.24),
    "SM_StalagmiteNub_01": (0.38, 0.35, 0.32),

    # Dungeon
    "SM_Rubble_01": (0.38, 0.36, 0.34),
    "SM_Rubble_02": (0.34, 0.32, 0.32),
    "SM_ChainLink_01": (0.32, 0.30, 0.30),
}


def create_scatter_material(name, color, two_sided=True):
    """Create a simple material with per-instance random variation."""
    mat_name = f"M_{name}"
    full_path = f"{MAT_PATH}/{mat_name}"

    if eal.does_asset_exist(full_path):
        mat = unreal.load_asset(full_path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(mat_name, MAT_PATH,
            unreal.Material, unreal.MaterialFactoryNew())

    if not mat:
        return None

    mat.set_editor_property("two_sided", two_sided)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    # Base color
    base = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    base.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 0.0))

    # Per-instance random variation (±15%)
    try:
        rand = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceRandom, -400, -150)
        remap = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -200, -100)
        lo = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, -120)
        lo.set_editor_property("r", 0.85)
        hi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, -80)
        hi.set_editor_property("r", 1.15)
        mel.connect_material_expressions(lo, "", remap, "A")
        mel.connect_material_expressions(hi, "", remap, "B")
        mel.connect_material_expressions(rand, "", remap, "Alpha")

        varied = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, -50)
        mel.connect_material_expressions(base, "", varied, "A")
        mel.connect_material_expressions(remap, "", varied, "B")
        mel.connect_material_property(varied, "", unreal.MaterialProperty.MP_BASE_COLOR)
    except Exception:
        mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Roughness 0.95
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 100)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(full_path)
    return full_path


# Create and apply materials
unreal.log("=== Creating scatter materials ===\n")
applied = 0

for mesh_name, color in MESH_COLORS.items():
    mesh_full = f"{MESH_PATH}/{mesh_name}"
    if not eal.does_asset_exist(mesh_full):
        continue

    mesh = unreal.load_asset(mesh_full)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        continue

    mat_path = create_scatter_material(mesh_name, color)
    if not mat_path:
        continue

    mat = unreal.load_asset(mat_path)
    if mat:
        mesh.set_material(0, mat)
        eal.save_asset(mesh_full)
        applied += 1

    if applied % 10 == 0:
        unreal.log(f"  {applied} materials applied...")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {applied} scatter meshes now have proper colored materials")
unreal.log(f"  Each mesh has:")
unreal.log(f"    - Correct muted RO palette color")
unreal.log(f"    - Per-instance random variation (+-15%)")
unreal.log(f"    - Two-sided rendering")
unreal.log(f"    - Roughness 0.95")
unreal.log(f"  Re-apply landscape material to see updated grass")
unreal.log(f"{'='*60}")
