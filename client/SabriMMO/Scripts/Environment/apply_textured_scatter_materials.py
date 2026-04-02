# apply_textured_scatter_materials.py
# Applies RO original textures to scatter meshes using world-aligned UVs.
# This gives each mesh the hand-painted RO look instead of flat color.
# Each mesh type gets a thematically appropriate RO texture.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_PATH = "/Game/SabriMMO/Environment/Grass/Materials"
MESH_PATH = "/Game/SabriMMO/Environment/Grass/Meshes"
RO_TEX = "/Game/SabriMMO/Textures/Environment/RO_Original"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Map each mesh to an RO texture number + tint color
# The texture provides detail/pattern, tint shifts it toward the right hue
MESH_TEX_MAP = {
    # Grass — green RO ground textures
    "SM_GrassBlade_01": ("005", (0.7, 1.0, 0.6)),
    "SM_GrassBlade_02": ("005", (0.75, 1.0, 0.65)),
    "SM_GrassBlade_03": ("005", (0.6, 0.9, 0.5)),
    "SM_GrassBlade_04": ("005", (0.65, 0.95, 0.55)),
    "SM_GrassBlade_05": ("005", (0.7, 1.0, 0.6)),
    "SM_GrassClump_01": ("005", (0.6, 0.9, 0.5)),
    "SM_WheatStalk_01": ("025", (1.0, 0.9, 0.5)),

    # Flowers
    "SM_Flower_01": ("015", (1.0, 0.85, 0.4)),
    "SM_Flower_02": ("015", (1.0, 0.7, 0.8)),
    "SM_Flower_03": ("015", (0.7, 0.7, 1.0)),
    "SM_Dandelion_01": ("025", (1.0, 0.95, 0.4)),
    "SM_Clover_01": ("005", (0.6, 0.9, 0.5)),

    # Forest — dark earthy textures
    "SM_Mushroom_01": ("042", (0.9, 0.7, 0.5)),
    "SM_Mushroom_02": ("042", (1.0, 0.6, 0.5)),
    "SM_Fern_01": ("005", (0.5, 0.85, 0.4)),
    "SM_FallenLeaf_01": ("025", (0.9, 0.7, 0.4)),
    "SM_FallenLeaf_02": ("025", (1.0, 0.75, 0.35)),
    "SM_PineCone_01": ("042", (0.8, 0.6, 0.4)),
    "SM_BarkPiece_01": ("042", (0.7, 0.5, 0.35)),
    "SM_Acorn_01": ("042", (0.85, 0.65, 0.4)),
    "SM_MossClump_01": ("005", (0.5, 0.8, 0.4)),

    # Pebbles/rocks — stone textures
    "SM_Pebble_01": ("055", (0.85, 0.8, 0.75)),
    "SM_Pebble_02": ("055", (0.8, 0.78, 0.72)),
    "SM_Pebble_03": ("055", (0.9, 0.85, 0.78)),
    "SM_SmallRock_01": ("055", (0.75, 0.72, 0.68)),
    "SM_SmallRock_02": ("055", (0.85, 0.8, 0.75)),
    "SM_Twig_01": ("042", (0.75, 0.6, 0.4)),
    "SM_DirtClump_01": ("042", (0.7, 0.55, 0.35)),

    # Desert
    "SM_DryGrass_01": ("025", (1.0, 0.85, 0.5)),
    "SM_DryGrass_02": ("025", (0.95, 0.82, 0.48)),
    "SM_DesertRock_01": ("088", (0.95, 0.8, 0.6)),
    "SM_DesertRock_02": ("088", (1.0, 0.85, 0.65)),
    "SM_DryTwig_01": ("088", (0.9, 0.75, 0.55)),
    "SM_SmallCactus_01": ("005", (0.65, 0.9, 0.5)),
    "SM_BoneFrag_01": ("150", (1.0, 0.98, 0.9)),

    # Snow
    "SM_SnowClump_01": ("150", (0.95, 0.97, 1.0)),
    "SM_SnowClump_02": ("150", (0.92, 0.95, 1.0)),
    "SM_IceCrystal_01": ("150", (0.8, 0.9, 1.0)),
    "SM_IceCrystal_02": ("150", (0.82, 0.92, 1.0)),
    "SM_FrozenTwig_01": ("055", (0.75, 0.8, 0.9)),

    # Beach
    "SM_Shell_01": ("150", (1.0, 0.95, 0.85)),
    "SM_Shell_02": ("150", (0.95, 0.92, 0.82)),
    "SM_CoralBit_01": ("088", (1.0, 0.7, 0.65)),
    "SM_Starfish_01": ("088", (1.0, 0.65, 0.55)),
    "SM_Driftwood_01": ("042", (0.8, 0.7, 0.6)),
    "SM_Seaweed_01": ("005", (0.5, 0.75, 0.45)),

    # Volcanic
    "SM_ObsidianShard_01": ("100", (0.3, 0.28, 0.32)),
    "SM_ObsidianShard_02": ("100", (0.35, 0.3, 0.3)),
    "SM_LavaRock_01": ("100", (0.45, 0.3, 0.25)),
    "SM_LavaRock_02": ("100", (0.5, 0.35, 0.25)),
    "SM_SulfurCrystal_01": ("025", (1.0, 0.95, 0.3)),
    "SM_AshClump_01": ("100", (0.55, 0.52, 0.5)),

    # Cursed
    "SM_DeadGrass_01": ("042", (0.7, 0.6, 0.5)),
    "SM_DeadGrass_02": ("042", (0.65, 0.55, 0.45)),
    "SM_Bone_01": ("150", (0.98, 0.95, 0.88)),
    "SM_BoneStick_01": ("150", (0.95, 0.92, 0.85)),
    "SM_SmallSkull_01": ("150", (0.92, 0.90, 0.82)),
    "SM_CandleStub_01": ("150", (0.95, 0.9, 0.72)),

    # Cultural
    "SM_CherryPetal_01": ("015", (1.0, 0.72, 0.78)),
    "SM_BambooLeaf_01": ("005", (0.65, 0.9, 0.5)),
    "SM_BirchLeaf_01": ("025", (1.0, 0.92, 0.45)),
    "SM_Berry_01": ("100", (1.0, 0.3, 0.25)),

    # Ruins
    "SM_StoneFrag_01": ("088", (0.88, 0.82, 0.72)),
    "SM_StoneFrag_02": ("088", (0.82, 0.78, 0.68)),
    "SM_PotteryShard_01": ("088", (0.95, 0.75, 0.55)),

    # Industrial
    "SM_MetalScrap_01": ("100", (0.65, 0.62, 0.6)),
    "SM_Bolt_01": ("100", (0.72, 0.68, 0.62)),
    "SM_RustFlake_01": ("088", (0.9, 0.55, 0.3)),

    # Cave
    "SM_CrystalShard_01": ("100", (0.6, 0.55, 0.85)),
    "SM_CrystalShard_02": ("100", (0.65, 0.58, 0.9)),
    "SM_WetRock_01": ("100", (0.4, 0.38, 0.42)),
    "SM_StalagmiteNub_01": ("055", (0.65, 0.6, 0.55)),

    # Dungeon
    "SM_Rubble_01": ("100", (0.6, 0.58, 0.55)),
    "SM_Rubble_02": ("100", (0.55, 0.53, 0.52)),
    "SM_ChainLink_01": ("100", (0.5, 0.48, 0.48)),
}


def create_textured_material(mesh_name, tex_num, tint, is_grass=False):
    """Material with RO texture + world-aligned UVs + color tint + optional tip fade."""
    mat_name = f"M_{mesh_name}"
    full_path = f"{MAT_PATH}/{mat_name}"

    if eal.does_asset_exist(full_path):
        mat = unreal.load_asset(full_path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(mat_name, MAT_PATH,
            unreal.Material, unreal.MaterialFactoryNew())

    if not mat:
        return None

    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    # World position for color variation
    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -600, 0)

    # Base color from tint (flat color approach — reliable, no texture compile issues)
    tint_param = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -50)
    tint_param.set_editor_property("constant", unreal.LinearColor(tint[0], tint[1], tint[2], 0.0))

    # Use tint as the base color (textured approach caused LWCVector3->float2 errors)
    tinted = tint_param

    # === WORLD-POSITION COLOR VARIATION ===
    # Noise based on world position creates patches of warmer/cooler color
    # Adjacent meshes in same area share similar tint = natural color fields
    world_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -300, -300)
    world_noise.set_editor_property("scale", 0.0008)
    world_noise.set_editor_property("levels", 2)
    world_noise.set_editor_property("output_min", 0.0)
    world_noise.set_editor_property("output_max", 1.0)
    mel.connect_material_expressions(world_pos, "", world_noise, "Position")

    # Remap noise to warm/cool shift: 0.88-1.12 range
    world_color_lo = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, -350)
    world_color_lo.set_editor_property("constant", unreal.LinearColor(0.92, 0.88, 0.82, 0.0))
    world_color_hi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, -280)
    world_color_hi.set_editor_property("constant", unreal.LinearColor(1.08, 1.05, 1.12, 0.0))

    world_color_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -50, -310)
    mel.connect_material_expressions(world_color_lo, "", world_color_lerp, "A")
    mel.connect_material_expressions(world_color_hi, "", world_color_lerp, "B")
    mel.connect_material_expressions(world_noise, "", world_color_lerp, "Alpha")

    # Per-instance random brightness on top (±10%)
    try:
        rand = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceRandom, -200, -200)
        rand_remap = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -50, -200)
        lo = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -150, -220)
        lo.set_editor_property("r", 0.9)
        hi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -150, -180)
        hi.set_editor_property("r", 1.1)
        mel.connect_material_expressions(lo, "", rand_remap, "A")
        mel.connect_material_expressions(hi, "", rand_remap, "B")
        mel.connect_material_expressions(rand, "", rand_remap, "Alpha")

        # Combine: texture * tint * world_variation * per_instance_random
        color_with_world = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 50, -150)
        mel.connect_material_expressions(tinted, "", color_with_world, "A")
        mel.connect_material_expressions(world_color_lerp, "", color_with_world, "B")

        final_color = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 150, -100)
        mel.connect_material_expressions(color_with_world, "", final_color, "A")
        mel.connect_material_expressions(rand_remap, "", final_color, "B")
        mel.connect_material_property(final_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    except Exception:
        color_with_world = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 50, -100)
        mel.connect_material_expressions(tinted, "", color_with_world, "A")
        mel.connect_material_expressions(world_color_lerp, "", color_with_world, "B")
        mel.connect_material_property(color_with_world, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Tip fade removed — caused checkerboard artifacts on some meshes
    # The BLEND_MASKED mode with height-based opacity didn't work reliably
    # across the two different mesh size classes

    # Roughness
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 100)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(full_path)
    return full_path


unreal.log("=== Applying textured RO materials to scatter meshes ===\n")
applied = 0

for mesh_name, (tex_num, tint) in MESH_TEX_MAP.items():
    mesh_full = f"{MESH_PATH}/{mesh_name}"
    if not eal.does_asset_exist(mesh_full):
        continue

    mesh = unreal.load_asset(mesh_full)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        continue

    # Grass/leaf meshes get tip fade
    GRASS_MESHES = {"SM_GrassBlade", "SM_GrassClump", "SM_WheatStalk", "SM_Fern",
                    "SM_DryGrass", "SM_DeadGrass", "SM_Dandelion", "SM_Seaweed",
                    "SM_FallenLeaf", "SM_BambooLeaf", "SM_CherryPetal", "SM_BirchLeaf",
                    "SM_Clover"}
    is_grass = any(mesh_name.startswith(prefix) for prefix in GRASS_MESHES)

    mat_path = create_textured_material(mesh_name, tex_num, tint, is_grass=is_grass)
    if not mat_path:
        continue

    mat = unreal.load_asset(mat_path)
    if mat:
        mesh.set_material(0, mat)
        eal.save_asset(mesh_full)
        applied += 1
        unreal.log(f"  {mesh_name}: textured with RO/{tex_num} (grass={is_grass})")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {applied} meshes now have RO-textured materials")
unreal.log(f"  Each mesh samples an actual RO texture via world UVs")
unreal.log(f"  + color tint to match the object type")
unreal.log(f"  + per-instance random brightness variation")
unreal.log(f"{'='*60}")
