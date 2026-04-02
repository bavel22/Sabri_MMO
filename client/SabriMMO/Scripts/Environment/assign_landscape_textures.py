# assign_landscape_textures.py
# Run in UE5 after importing textures and creating M_Landscape_RO.
#
# Creates a MaterialInstance from M_Landscape_RO and sets the texture
# parameters on it. This is the correct UE5 workflow — Material Instances
# override parent parameters without modifying the parent material.

import unreal

MAT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO"
MI_PATH = "/Game/SabriMMO/Materials/Environment"
MI_NAME = "MI_Landscape_PrtSouth"
MI_FULL = f"{MI_PATH}/{MI_NAME}"

mel = unreal.MaterialEditingLibrary

# Best textures — tries each path in order until one exists
TEXTURE_ASSIGNMENTS = {
    "GrassWarmTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_LushGreen_03_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_LushGreen_03",
        "/Game/SabriMMO/Textures/Environment/Ground/T_Ground_RO_DenseGrass_A",
    ],
    "GrassCoolTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_LushGreen_02_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_LushGreen_02",
        "/Game/SabriMMO/Textures/Environment/Ground/T_Ground_RO_Mixed_B",
    ],
    "DirtTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_EarthPath_01_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_EarthPath_01",
        "/Game/SabriMMO/Textures/Environment/Ground/T_Dirt_RO_v1",
    ],
    "RockTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_StoneCliff_02_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_StoneCliff_02",
        "/Game/SabriMMO/Textures/Environment/Ground/T_Rock_RO_v1",
    ],
}


def find_texture(candidates):
    """Try each candidate path, return first that exists."""
    for path in candidates:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.load_asset(path), path
    return None, None


# Load parent material
parent_mat = unreal.load_asset(MAT_PATH)
if not parent_mat:
    unreal.log_error(f"Parent material not found: {MAT_PATH}")
    unreal.log_error("Run create_landscape_material.py first!")
    raise RuntimeError("Material not found")

unreal.log(f"Parent material: {MAT_PATH}")

# Create or load Material Instance
if unreal.EditorAssetLibrary.does_asset_exist(MI_FULL):
    unreal.log(f"Loading existing instance: {MI_NAME}")
    mi = unreal.load_asset(MI_FULL)
else:
    unreal.log(f"Creating new instance: {MI_NAME}")
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mi_factory = unreal.MaterialInstanceConstantFactoryNew()
    mi = asset_tools.create_asset(MI_NAME, MI_PATH,
        unreal.MaterialInstanceConstant, mi_factory)

if not mi:
    unreal.log_error("Failed to create Material Instance!")
    raise RuntimeError("MI creation failed")

# Set parent
mi.set_editor_property("parent", parent_mat)

# Assign textures via MaterialEditingLibrary
assigned = 0
for param_name, candidates in TEXTURE_ASSIGNMENTS.items():
    tex, tex_path = find_texture(candidates)
    if tex:
        success = mel.set_material_instance_texture_parameter_value(
            mi, param_name, tex,
            unreal.MaterialParameterAssociation.LAYER_PARAMETER
        )
        if not success:
            # Try global association
            success = mel.set_material_instance_texture_parameter_value(
                mi, param_name, tex,
                unreal.MaterialParameterAssociation.GLOBAL_PARAMETER
            )
        if success:
            unreal.log(f"  {param_name} = {tex_path}")
            assigned += 1
        else:
            unreal.log_warning(f"  {param_name}: set_parameter returned False (param may not exist in parent)")
    else:
        unreal.log_warning(f"  {param_name}: no texture found in any candidate path")

# Also set scalar parameters with good defaults
mel.set_material_instance_scalar_parameter_value(
    mi, "DirtAmount", 0.2,
    unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(
    mi, "SlopeSharpness", 2.5,
    unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

# Save
unreal.EditorAssetLibrary.save_asset(MI_FULL)

unreal.log(f"\n{'=' * 60}")
unreal.log(f"  Material Instance created: {MI_FULL}")
unreal.log(f"  Textures assigned: {assigned}/4")
unreal.log(f"  ")
unreal.log(f"  To use: drag MI_Landscape_PrtSouth onto any mesh,")
unreal.log(f"  or apply via unrealMCP/Python")
unreal.log(f"{'=' * 60}")
