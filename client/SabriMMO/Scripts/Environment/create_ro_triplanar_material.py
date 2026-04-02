# create_ro_triplanar_material.py
# Creates M_RO_Triplanar — proper triplanar projection for slopes.
#
# Fixes the XY-stretch problem on hillsides by sampling the texture
# from all 3 axes (XY, XZ, YZ) and blending based on the surface
# normal. Also keeps slope-based cliff texture, macro variation,
# and UV distortion from M_RO_Original.
#
# Parameters (10):
#   GroundTexture, CliffTexture
#   TileSize (4000), CliffTileSize (3000)
#   MacroStrength (0.08), UVDistortStrength (40)
#   SlopeThreshold (0.60), SlopeTransitionWidth (0.15)
#   TriplanarSharpness (4.0) — controls blend falloff between axes
#   TriplanarCliffSharpness (4.0) — same for cliff texture

import unreal

MAT_PATH = "/Game/SabriMMO/Materials/Environment"
MAT_NAME = "M_RO_Triplanar"
FULL_PATH = f"{MAT_PATH}/{MAT_NAME}"

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# ============================================================
# Create or rebuild material
# ============================================================

eal.make_directory(MAT_PATH)

if eal.does_asset_exist(FULL_PATH):
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
# Helper: Y offset tracker for node layout
# ============================================================
# We place nodes in columns (X) and rows (Y) for readability.

X_WORLD = -2000     # World position / masks
X_UV    = -1600     # UV distortion
X_DIV   = -1200     # UV divide by tile size
X_TEX   = -800      # Texture samples
X_BLEND = -400      # Triplanar blend weights
X_SLOPE = -100      # Slope detection
X_OUT   = 300       # Final outputs

# ============================================================
# SECTION 1: World Position + Component Masks
# ============================================================

unreal.log("  World position + axis masks...")

world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, X_WORLD, 0)

# XY mask (top-down projection — for flat ground)
mask_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_WORLD + 200, -100)
mask_xy.set_editor_property("r", True)
mask_xy.set_editor_property("g", True)
mask_xy.set_editor_property("b", False)
mask_xy.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xy, "")

# XZ mask (side projection — for north/south-facing slopes)
mask_xz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_WORLD + 200, 100)
mask_xz.set_editor_property("r", True)
mask_xz.set_editor_property("g", False)
mask_xz.set_editor_property("b", True)
mask_xz.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xz, "")

# YZ mask (front projection — for east/west-facing slopes)
mask_yz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_WORLD + 200, 300)
mask_yz.set_editor_property("r", False)
mask_yz.set_editor_property("g", True)
mask_yz.set_editor_property("b", True)
mask_yz.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_yz, "")

# ============================================================
# SECTION 2: UV Distortion (subtle seam hiding)
# ============================================================

unreal.log("  UV distortion...")

uv_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, X_UV, -300)
uv_noise.set_editor_property("scale", 0.003)
uv_noise.set_editor_property("levels", 2)
uv_noise.set_editor_property("output_min", -1.0)
uv_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", uv_noise, "Position")

uv_distort_str = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_UV, -400)
uv_distort_str.set_editor_property("parameter_name", "UVDistortStrength")
uv_distort_str.set_editor_property("default_value", 40.0)

uv_distort_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_UV + 200, -350)
mel.connect_material_expressions(uv_noise, "", uv_distort_mult, "A")
mel.connect_material_expressions(uv_distort_str, "", uv_distort_mult, "B")

# Add distortion to each axis projection
uv_xy = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_DIV, -100)
mel.connect_material_expressions(mask_xy, "", uv_xy, "A")
mel.connect_material_expressions(uv_distort_mult, "", uv_xy, "B")

uv_xz = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_DIV, 100)
mel.connect_material_expressions(mask_xz, "", uv_xz, "A")
mel.connect_material_expressions(uv_distort_mult, "", uv_xz, "B")

uv_yz = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_DIV, 300)
mel.connect_material_expressions(mask_yz, "", uv_yz, "A")
mel.connect_material_expressions(uv_distort_mult, "", uv_yz, "B")

# ============================================================
# SECTION 3: Divide UVs by tile size
# ============================================================

unreal.log("  Tile sizes...")

tile_size = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_DIV, -300)
tile_size.set_editor_property("parameter_name", "TileSize")
tile_size.set_editor_property("default_value", 4000.0)

cliff_tile_size = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_DIV, 500)
cliff_tile_size.set_editor_property("parameter_name", "CliffTileSize")
cliff_tile_size.set_editor_property("default_value", 3000.0)

# Ground UVs (3 axes)
gnd_uv_xy = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, -100)
mel.connect_material_expressions(uv_xy, "", gnd_uv_xy, "A")
mel.connect_material_expressions(tile_size, "", gnd_uv_xy, "B")

gnd_uv_xz = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 100)
mel.connect_material_expressions(uv_xz, "", gnd_uv_xz, "A")
mel.connect_material_expressions(tile_size, "", gnd_uv_xz, "B")

gnd_uv_yz = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 300)
mel.connect_material_expressions(uv_yz, "", gnd_uv_yz, "A")
mel.connect_material_expressions(tile_size, "", gnd_uv_yz, "B")

# Cliff UVs (3 axes)
clf_uv_xy = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 500)
mel.connect_material_expressions(uv_xy, "", clf_uv_xy, "A")
mel.connect_material_expressions(cliff_tile_size, "", clf_uv_xy, "B")

clf_uv_xz = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 700)
mel.connect_material_expressions(uv_xz, "", clf_uv_xz, "A")
mel.connect_material_expressions(cliff_tile_size, "", clf_uv_xz, "B")

clf_uv_yz = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 900)
mel.connect_material_expressions(uv_yz, "", clf_uv_yz, "A")
mel.connect_material_expressions(cliff_tile_size, "", clf_uv_yz, "B")

# ============================================================
# SECTION 4: Texture sampling (3 per texture = 6 total)
# ============================================================

unreal.log("  Texture samples (6 — triplanar x 2 textures)...")

# Use TextureSampleParameter2D for all 6 — UE5 Python TextureObjectParameter
# connections don't work reliably. Each axis gets its own parameter name;
# the MI script sets all 3 ground params to the same texture (same for cliff).

tex_gnd_xy = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, -200)
tex_gnd_xy.set_editor_property("parameter_name", "GroundTexture")
mel.connect_material_expressions(gnd_uv_xy, "", tex_gnd_xy, "UVs")

tex_gnd_xz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 0)
tex_gnd_xz.set_editor_property("parameter_name", "GroundTextureXZ")
mel.connect_material_expressions(gnd_uv_xz, "", tex_gnd_xz, "UVs")

tex_gnd_yz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 200)
tex_gnd_yz.set_editor_property("parameter_name", "GroundTextureYZ")
mel.connect_material_expressions(gnd_uv_yz, "", tex_gnd_yz, "UVs")

tex_clf_xy = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 450)
tex_clf_xy.set_editor_property("parameter_name", "CliffTexture")
mel.connect_material_expressions(clf_uv_xy, "", tex_clf_xy, "UVs")

tex_clf_xz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 650)
tex_clf_xz.set_editor_property("parameter_name", "CliffTextureXZ")
mel.connect_material_expressions(clf_uv_xz, "", tex_clf_xz, "UVs")

tex_clf_yz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 850)
tex_clf_yz.set_editor_property("parameter_name", "CliffTextureYZ")
mel.connect_material_expressions(clf_uv_yz, "", tex_clf_yz, "UVs")

# ============================================================
# SECTION 5: Triplanar blend weights from world normal
# ============================================================

unreal.log("  Triplanar blend weights...")

vert_normal = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, X_BLEND - 200, -200)

# Abs(normal) — we need unsigned weights per axis
abs_normal = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, X_BLEND - 100, -200)
mel.connect_material_expressions(vert_normal, "", abs_normal, "")

# Sharpness parameter — higher = sharper transitions between axes
tri_sharp = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_BLEND - 200, -350)
tri_sharp.set_editor_property("parameter_name", "TriplanarSharpness")
tri_sharp.set_editor_property("default_value", 4.0)

# Power(abs_normal, sharpness) — concentrates weight on dominant axis
sharp_pow = mel.create_material_expression(mat, unreal.MaterialExpressionPower, X_BLEND, -250)
mel.connect_material_expressions(abs_normal, "", sharp_pow, "Base")
mel.connect_material_expressions(tri_sharp, "", sharp_pow, "Exponent")

# Break into components for normalization
break_sharp = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_BLEND + 150, -350)
break_sharp.set_editor_property("r", True)
break_sharp.set_editor_property("g", False)
break_sharp.set_editor_property("b", False)
break_sharp.set_editor_property("a", False)
mel.connect_material_expressions(sharp_pow, "", break_sharp, "")

break_sharp_g = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_BLEND + 150, -250)
break_sharp_g.set_editor_property("r", False)
break_sharp_g.set_editor_property("g", True)
break_sharp_g.set_editor_property("b", False)
break_sharp_g.set_editor_property("a", False)
mel.connect_material_expressions(sharp_pow, "", break_sharp_g, "")

break_sharp_b = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_BLEND + 150, -150)
break_sharp_b.set_editor_property("r", False)
break_sharp_b.set_editor_property("g", False)
break_sharp_b.set_editor_property("b", True)
break_sharp_b.set_editor_property("a", False)
mel.connect_material_expressions(sharp_pow, "", break_sharp_b, "")

# Sum = R + G + B for normalization
sum_rg = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 300, -300)
mel.connect_material_expressions(break_sharp, "", sum_rg, "A")
mel.connect_material_expressions(break_sharp_g, "", sum_rg, "B")

sum_rgb = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 450, -250)
mel.connect_material_expressions(sum_rg, "", sum_rgb, "A")
mel.connect_material_expressions(break_sharp_b, "", sum_rgb, "B")

# Normalized weights: w_axis = pow_axis / sum
# w_x = weight for YZ projection (surface faces X axis)
w_x = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_BLEND + 600, -350)
mel.connect_material_expressions(break_sharp, "", w_x, "A")
mel.connect_material_expressions(sum_rgb, "", w_x, "B")

# w_y = weight for XZ projection (surface faces Y axis)
w_y = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_BLEND + 600, -250)
mel.connect_material_expressions(break_sharp_g, "", w_y, "A")
mel.connect_material_expressions(sum_rgb, "", w_y, "B")

# w_z = weight for XY projection (surface faces Z axis = flat ground)
w_z = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_BLEND + 600, -150)
mel.connect_material_expressions(break_sharp_b, "", w_z, "A")
mel.connect_material_expressions(sum_rgb, "", w_z, "B")

# ============================================================
# SECTION 6: Blend ground texture triplanarly
# ============================================================

unreal.log("  Triplanar ground blend...")

# ground = tex_xy * w_z + tex_xz * w_y + tex_yz * w_x
gnd_xy_w = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, -200)
mel.connect_material_expressions(tex_gnd_xy, "RGB", gnd_xy_w, "A")
mel.connect_material_expressions(w_z, "", gnd_xy_w, "B")

gnd_xz_w = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 0)
mel.connect_material_expressions(tex_gnd_xz, "RGB", gnd_xz_w, "A")
mel.connect_material_expressions(w_y, "", gnd_xz_w, "B")

gnd_yz_w = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 200)
mel.connect_material_expressions(tex_gnd_yz, "RGB", gnd_yz_w, "A")
mel.connect_material_expressions(w_x, "", gnd_yz_w, "B")

gnd_sum1 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1000, -100)
mel.connect_material_expressions(gnd_xy_w, "", gnd_sum1, "A")
mel.connect_material_expressions(gnd_xz_w, "", gnd_sum1, "B")

gnd_triplanar = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1150, 0)
mel.connect_material_expressions(gnd_sum1, "", gnd_triplanar, "A")
mel.connect_material_expressions(gnd_yz_w, "", gnd_triplanar, "B")

# ============================================================
# SECTION 7: Blend cliff texture triplanarly
# ============================================================

unreal.log("  Triplanar cliff blend...")

# Same blend for cliff texture
clf_xy_w = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 450)
mel.connect_material_expressions(tex_clf_xy, "RGB", clf_xy_w, "A")
mel.connect_material_expressions(w_z, "", clf_xy_w, "B")

clf_xz_w = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 650)
mel.connect_material_expressions(tex_clf_xz, "RGB", clf_xz_w, "A")
mel.connect_material_expressions(w_y, "", clf_xz_w, "B")

clf_yz_w = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 850)
mel.connect_material_expressions(tex_clf_yz, "RGB", clf_yz_w, "A")
mel.connect_material_expressions(w_x, "", clf_yz_w, "B")

clf_sum1 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1000, 550)
mel.connect_material_expressions(clf_xy_w, "", clf_sum1, "A")
mel.connect_material_expressions(clf_xz_w, "", clf_sum1, "B")

clf_triplanar = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1150, 650)
mel.connect_material_expressions(clf_sum1, "", clf_triplanar, "A")
mel.connect_material_expressions(clf_yz_w, "", clf_triplanar, "B")

# ============================================================
# SECTION 8: Macro variation (subtle anti-repetition)
# ============================================================

unreal.log("  Macro variation...")

macro_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, X_SLOPE - 200, -400)
macro_noise.set_editor_property("scale", 0.0008)
macro_noise.set_editor_property("levels", 3)
macro_noise.set_editor_property("output_min", 0.0)
macro_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", macro_noise, "Position")

macro_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_SLOPE - 400, -450)
macro_strength.set_editor_property("parameter_name", "MacroStrength")
macro_strength.set_editor_property("default_value", 0.08)

macro_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_SLOPE - 300, -500)
macro_one.set_editor_property("r", 1.0)

macro_lo = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, X_SLOPE - 100, -500)
mel.connect_material_expressions(macro_one, "", macro_lo, "A")
mel.connect_material_expressions(macro_strength, "", macro_lo, "B")

macro_hi = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_SLOPE - 100, -400)
mel.connect_material_expressions(macro_one, "", macro_hi, "A")
mel.connect_material_expressions(macro_strength, "", macro_hi, "B")

macro_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, X_SLOPE + 50, -450)
mel.connect_material_expressions(macro_lo, "", macro_lerp, "A")
mel.connect_material_expressions(macro_hi, "", macro_lerp, "B")
mel.connect_material_expressions(macro_noise, "", macro_lerp, "Alpha")

# Apply macro to ground triplanar
gnd_macro = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_SLOPE + 200, 0)
mel.connect_material_expressions(gnd_triplanar, "", gnd_macro, "A")
mel.connect_material_expressions(macro_lerp, "", gnd_macro, "B")

# ============================================================
# SECTION 9: Slope detection (45-degree cutoff)
# ============================================================

unreal.log("  Slope detection...")

world_up = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, X_SLOPE - 200, 300)
world_up.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))

slope_dot = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, X_SLOPE, 350)
mel.connect_material_expressions(vert_normal, "", slope_dot, "A")
mel.connect_material_expressions(world_up, "", slope_dot, "B")

slope_thresh = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_SLOPE - 200, 450)
slope_thresh.set_editor_property("parameter_name", "SlopeThreshold")
slope_thresh.set_editor_property("default_value", 0.60)

slope_sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, X_SLOPE + 100, 400)
mel.connect_material_expressions(slope_dot, "", slope_sub, "A")
mel.connect_material_expressions(slope_thresh, "", slope_sub, "B")

slope_width = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_SLOPE - 200, 550)
slope_width.set_editor_property("parameter_name", "SlopeTransitionWidth")
slope_width.set_editor_property("default_value", 0.15)

slope_div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_SLOPE + 200, 450)
mel.connect_material_expressions(slope_sub, "", slope_div, "A")
mel.connect_material_expressions(slope_width, "", slope_div, "B")

slope_clamp = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, X_SLOPE + 300, 450)
mel.connect_material_expressions(slope_div, "", slope_clamp, "")

# slope_clamp: 1.0 = flat (ground), 0.0 = steep (cliff)
# We want: flat → ground, steep → cliff, so use clamp directly as alpha
# lerp(cliff, ground, slope_clamp) — A=cliff when alpha=0, B=ground when alpha=1

# ============================================================
# SECTION 10: Final blend (ground vs cliff based on slope)
# ============================================================

unreal.log("  Final slope blend...")

final_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, X_OUT, 200)
mel.connect_material_expressions(clf_triplanar, "", final_lerp, "A")  # cliff when flat_alpha=0 (steep)
mel.connect_material_expressions(gnd_macro, "", final_lerp, "B")      # ground when flat_alpha=1 (flat)
mel.connect_material_expressions(slope_clamp, "", final_lerp, "Alpha")

# ============================================================
# SECTION 11: Outputs
# ============================================================

unreal.log("  Connecting outputs...")

mel.connect_material_property(final_lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)

roughness = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_OUT, 350)
roughness.set_editor_property("r", 0.95)
mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

metallic = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_OUT, 450)
metallic.set_editor_property("r", 0.0)
mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

# ============================================================
# Save & compile
# ============================================================

mel.layout_material_expressions(mat)
mel.recompile_material(mat)
eal.save_asset(FULL_PATH)

unreal.log(f"\nSUCCESS: {MAT_NAME} created at {FULL_PATH}")
unreal.log(f"Parameters (10):")
unreal.log(f"  GroundTexture, CliffTexture")
unreal.log(f"  TileSize (4000), CliffTileSize (3000)")
unreal.log(f"  MacroStrength (0.08), UVDistortStrength (40)")
unreal.log(f"  SlopeThreshold (0.60), SlopeTransitionWidth (0.15)")
unreal.log(f"  TriplanarSharpness (4.0)")
unreal.log(f"\nTriplanar projection: samples each texture from XY, XZ, and YZ")
unreal.log(f"  axes, blended by surface normal. No more stretching on slopes.")
