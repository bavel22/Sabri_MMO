# create_ro_original_material.py
# Creates M_RO_Original — a simpler material designed for the original
# 608 RO Classic textures. Preserves their hand-painted character.
#
# Much simpler than M_Landscape_RO_12 (23 params). Only 8 params:
#   - GroundTexture (the RO original texture)
#   - CliffTexture (different RO texture for slopes, or same)
#   - TileSize (world-space UV scale)
#   - CliffTileSize
#   - MacroStrength (subtle anti-repetition)
#   - SlopeThreshold, SlopeTransitionWidth (45-deg cutoff)
#   - UVDistortStrength (subtle seam hiding)

import unreal

MAT_PATH = "/Game/SabriMMO/Materials/Environment"
MAT_NAME = "M_RO_Original"
FULL_PATH = f"{MAT_PATH}/{MAT_NAME}"

mel = unreal.MaterialEditingLibrary

# ============================================================
# Create or rebuild material
# ============================================================

unreal.EditorAssetLibrary.make_directory(MAT_PATH)

if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
    unreal.log(f"Rebuilding {MAT_NAME}...")
    mat = unreal.load_asset(FULL_PATH)
    mel.delete_all_material_expressions(mat)
else:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = asset_tools.create_asset(MAT_NAME, MAT_PATH, unreal.Material, unreal.MaterialFactoryNew())

if not mat:
    raise RuntimeError("Failed to create material")

mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
mat.set_editor_property("two_sided", False)

# ============================================================
# SECTION 1: World-Space UVs + subtle distortion
# ============================================================

unreal.log("  World-space UVs...")

world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1600, 0)

mask_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1400, 0)
mask_xy.set_editor_property("r", True)
mask_xy.set_editor_property("g", True)
mask_xy.set_editor_property("b", False)
mask_xy.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xy, "")

# Subtle UV distortion
uv_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -1400, -200)
uv_noise.set_editor_property("scale", 0.003)
uv_noise.set_editor_property("levels", 2)
uv_noise.set_editor_property("output_min", -1.0)
uv_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", uv_noise, "Position")

uv_distort_str = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1600, -200)
uv_distort_str.set_editor_property("parameter_name", "UVDistortStrength")
uv_distort_str.set_editor_property("default_value", 40.0)

uv_distort_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1200, -200)
mel.connect_material_expressions(uv_noise, "", uv_distort_mult, "A")
mel.connect_material_expressions(uv_distort_str, "", uv_distort_mult, "B")

uv_distorted = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -1000, 0)
mel.connect_material_expressions(mask_xy, "", uv_distorted, "A")
mel.connect_material_expressions(uv_distort_mult, "", uv_distorted, "B")

# Ground UV
tile_size = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1200, 100)
tile_size.set_editor_property("parameter_name", "TileSize")
tile_size.set_editor_property("default_value", 4000.0)

ground_uv = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -800, 0)
mel.connect_material_expressions(uv_distorted, "", ground_uv, "A")
mel.connect_material_expressions(tile_size, "", ground_uv, "B")

# Cliff UV (XZ projection for vertical faces)
mask_xz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1400, 300)
mask_xz.set_editor_property("r", True)
mask_xz.set_editor_property("g", False)
mask_xz.set_editor_property("b", True)
mask_xz.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xz, "")

cliff_uv_distorted = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -1000, 300)
mel.connect_material_expressions(mask_xz, "", cliff_uv_distorted, "A")
mel.connect_material_expressions(uv_distort_mult, "", cliff_uv_distorted, "B")

cliff_tile_size = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1200, 400)
cliff_tile_size.set_editor_property("parameter_name", "CliffTileSize")
cliff_tile_size.set_editor_property("default_value", 3000.0)

cliff_uv = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -800, 300)
mel.connect_material_expressions(cliff_uv_distorted, "", cliff_uv, "A")
mel.connect_material_expressions(cliff_tile_size, "", cliff_uv, "B")

# ============================================================
# SECTION 2: Texture sampling
# ============================================================

unreal.log("  Texture parameters...")

tex_ground = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, -50)
tex_ground.set_editor_property("parameter_name", "GroundTexture")
mel.connect_material_expressions(ground_uv, "", tex_ground, "UVs")

tex_cliff = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 300)
tex_cliff.set_editor_property("parameter_name", "CliffTexture")
mel.connect_material_expressions(cliff_uv, "", tex_cliff, "UVs")

# ============================================================
# SECTION 3: Subtle macro variation (break repetition)
# ============================================================

unreal.log("  Macro variation...")

macro_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -500, -250)
macro_noise.set_editor_property("scale", 0.0008)
macro_noise.set_editor_property("levels", 3)
macro_noise.set_editor_property("output_min", 0.0)
macro_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", macro_noise, "Position")

macro_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, -250)
macro_strength.set_editor_property("parameter_name", "MacroStrength")
macro_strength.set_editor_property("default_value", 0.08)

# Remap: Lerp(1-strength, 1+strength, noise)
macro_lo = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -400, -300)
macro_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, -350)
macro_one.set_editor_property("r", 1.0)
mel.connect_material_expressions(macro_one, "", macro_lo, "A")
mel.connect_material_expressions(macro_strength, "", macro_lo, "B")

macro_hi = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -400, -200)
mel.connect_material_expressions(macro_one, "", macro_hi, "A")
mel.connect_material_expressions(macro_strength, "", macro_hi, "B")

macro_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, -250)
mel.connect_material_expressions(macro_lo, "", macro_lerp, "A")
mel.connect_material_expressions(macro_hi, "", macro_lerp, "B")
mel.connect_material_expressions(macro_noise, "", macro_lerp, "Alpha")

# Apply macro to ground
ground_macro = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, -50)
mel.connect_material_expressions(tex_ground, "RGB", ground_macro, "A")
mel.connect_material_expressions(macro_lerp, "", ground_macro, "B")

# ============================================================
# SECTION 4: Slope detection (45-degree cutoff)
# ============================================================

unreal.log("  Slope detection...")

vert_normal = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -300, 400)
world_up = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 500)
world_up.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))

slope_dot = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -100, 450)
mel.connect_material_expressions(vert_normal, "", slope_dot, "A")
mel.connect_material_expressions(world_up, "", slope_dot, "B")

slope_thresh = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -100, 550)
slope_thresh.set_editor_property("parameter_name", "SlopeThreshold")
slope_thresh.set_editor_property("default_value", 0.60)

slope_sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, 50, 480)
mel.connect_material_expressions(slope_dot, "", slope_sub, "A")
mel.connect_material_expressions(slope_thresh, "", slope_sub, "B")

slope_width = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -100, 620)
slope_width.set_editor_property("parameter_name", "SlopeTransitionWidth")
slope_width.set_editor_property("default_value", 0.15)

slope_div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, 150, 520)
mel.connect_material_expressions(slope_sub, "", slope_div, "A")
mel.connect_material_expressions(slope_width, "", slope_div, "B")

slope_clamp = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, 250, 520)
mel.connect_material_expressions(slope_div, "", slope_clamp, "")

slope_invert = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 350, 520)
mel.connect_material_expressions(slope_clamp, "", slope_invert, "")

# Final blend: ground (with macro) on flat, cliff on slopes
final_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 400, 100)
mel.connect_material_expressions(ground_macro, "", final_lerp, "A")
mel.connect_material_expressions(tex_cliff, "RGB", final_lerp, "B")
mel.connect_material_expressions(slope_invert, "", final_lerp, "Alpha")

# ============================================================
# SECTION 5: Outputs
# ============================================================

unreal.log("  Connecting outputs...")

mel.connect_material_property(final_lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)

roughness = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 500, 200)
roughness.set_editor_property("r", 0.95)
mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

metallic = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 500, 300)
metallic.set_editor_property("r", 0.0)
mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

# ============================================================
# Save
# ============================================================

mel.layout_material_expressions(mat)
mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(FULL_PATH)

unreal.log(f"\nSUCCESS: {MAT_NAME} created at {FULL_PATH}")
unreal.log(f"Parameters (8):")
unreal.log(f"  GroundTexture, CliffTexture")
unreal.log(f"  TileSize (4000), CliffTileSize (3000)")
unreal.log(f"  MacroStrength (0.08), UVDistortStrength (40)")
unreal.log(f"  SlopeThreshold (0.60), SlopeTransitionWidth (0.15)")
