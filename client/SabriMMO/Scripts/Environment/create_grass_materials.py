# create_grass_materials.py
# Creates simple materials for the grass blade/flower/pebble meshes.
# Uses Masked blend mode with opacity mask for transparent grass blades.
# Colors matched to RO Classic muted palette.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_PATH = "/Game/SabriMMO/Environment/Grass/Materials"
eal.make_directory(MAT_PATH)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def create_simple_material(name, color, opacity_mask=True, two_sided=True):
    """Create a simple colored material for grass meshes."""
    full_path = f"{MAT_PATH}/{name}"

    if eal.does_asset_exist(full_path):
        mat = unreal.load_asset(full_path)
        mel.delete_all_material_expressions(mat)
        unreal.log(f"  {name}: rebuilding")
    else:
        mat = asset_tools.create_asset(name, MAT_PATH,
            unreal.Material, unreal.MaterialFactoryNew())
        unreal.log(f"  {name}: creating")

    if not mat:
        return None

    mat.set_editor_property("two_sided", two_sided)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    if opacity_mask:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    else:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    # Base color — muted RO palette color
    base_color = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 0)
    base_color.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))

    # Add slight random variation per instance using PerInstanceRandom
    try:
        instance_random = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceRandom, -400, -150)
        # Remap random 0-1 to 0.85-1.15 for subtle color variation
        rand_remap = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -200, -100)
        rand_min = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, -100)
        rand_min.set_editor_property("r", 0.85)
        rand_max = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, -50)
        rand_max.set_editor_property("r", 1.15)
        mel.connect_material_expressions(rand_min, "", rand_remap, "A")
        mel.connect_material_expressions(rand_max, "", rand_remap, "B")
        mel.connect_material_expressions(instance_random, "", rand_remap, "Alpha")

        color_varied = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, -50)
        mel.connect_material_expressions(base_color, "", color_varied, "A")
        mel.connect_material_expressions(rand_remap, "", color_varied, "B")
        mel.connect_material_property(color_varied, "", unreal.MaterialProperty.MP_BASE_COLOR)
    except Exception:
        # If PerInstanceRandom not available, use flat color
        mel.connect_material_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Roughness — high for RO style
    roughness = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 100)
    roughness.set_editor_property("r", 0.95)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if opacity_mask:
        # Create opacity mask from vertex position — fade out at top of blade
        # This gives grass blades transparent tips
        try:
            vert_color = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -300, 200)
            mel.connect_material_property(vert_color, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
        except Exception:
            # Fallback: full opacity
            opacity_const = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 200)
            opacity_const.set_editor_property("r", 1.0)
            mel.connect_material_property(opacity_const, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    mel.recompile_material(mat)
    eal.save_asset(full_path)
    return full_path


unreal.log("=== Creating Grass Mesh Materials ===\n")

# Grass materials — muted greens matching RO palette
m_grass_short = create_simple_material("M_GrassBlade_Short",
    color=(0.22, 0.38, 0.14), two_sided=True)

m_grass_tall = create_simple_material("M_GrassBlade_Tall",
    color=(0.18, 0.32, 0.10), two_sided=True)

m_grass_clump = create_simple_material("M_GrassClump",
    color=(0.20, 0.35, 0.12), two_sided=True)

# Flower material — muted warm yellow
m_flower = create_simple_material("M_Flower",
    color=(0.70, 0.62, 0.30), two_sided=True)

# Pebble material — warm gray-brown
m_pebble = create_simple_material("M_Pebble",
    color=(0.40, 0.36, 0.30), opacity_mask=False, two_sided=False)


# ============================================================
# Apply materials to the imported meshes
# ============================================================

unreal.log("\n=== Applying Materials to Meshes ===\n")

MESH_PATH = "/Game/SabriMMO/Environment/Grass/Meshes"

MESH_MAT_MAP = {
    "SM_GrassBlade_01": m_grass_short,
    "SM_GrassBlade_02": m_grass_short,
    "SM_GrassBlade_03": m_grass_tall,
    "SM_GrassClump_01": m_grass_clump,
    "SM_Flower_01": m_flower,
    "SM_Pebble_01": m_pebble,
}

for mesh_name, mat_path in MESH_MAT_MAP.items():
    mesh_full = f"{MESH_PATH}/{mesh_name}"
    if not eal.does_asset_exist(mesh_full):
        unreal.log(f"  {mesh_name}: mesh not found")
        continue
    if not mat_path:
        continue

    mesh_asset = unreal.load_asset(mesh_full)
    mat_asset = unreal.load_asset(mat_path)

    if mesh_asset and mat_asset:
        # Set material on static mesh
        if isinstance(mesh_asset, unreal.StaticMesh):
            mesh_asset.set_material(0, mat_asset)
            eal.save_asset(mesh_full)
            unreal.log(f"  {mesh_name}: material applied ({mat_path.split('/')[-1]})")
        else:
            unreal.log(f"  {mesh_name}: not a StaticMesh, skipping")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: Grass materials created and applied")
unreal.log(f"  Materials at: {MAT_PATH}")
unreal.log(f"  Colors: muted RO greens with per-instance random variation")
unreal.log(f"  Each grass instance is slightly different shade")
unreal.log(f"{'='*60}")
