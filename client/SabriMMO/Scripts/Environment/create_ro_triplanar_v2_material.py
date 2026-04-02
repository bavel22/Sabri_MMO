# create_ro_triplanar_v2_material.py
# Creates M_RO_Triplanar_v2 — triplanar with normal maps + distance macro tinting.
#
# Improvements over M_RO_Triplanar:
#   1. Normal map support (triplanar-projected, per ground + cliff)
#   2. Distance-based macro tinting (large-scale brightness/hue shift to break tiling)
#   3. NormalStrength parameter
#   4. DistanceMacroScale + DistanceMacroStrength parameters
#
# Parameters (16):
#   GroundTexture, GroundTextureXZ, GroundTextureYZ
#   CliffTexture, CliffTextureXZ, CliffTextureYZ
#   GroundNormal, GroundNormalXZ, GroundNormalYZ
#   CliffNormal, CliffNormalXZ, CliffNormalYZ
#   TileSize (4000), CliffTileSize (3000)
#   MacroStrength (0.08), UVDistortStrength (40)
#   SlopeThreshold (0.60), SlopeTransitionWidth (0.15)
#   TriplanarSharpness (4.0)
#   NormalStrength (0.5)
#   DistanceMacroScale (0.0002), DistanceMacroStrength (0.15)

import unreal

MAT_PATH = "/Game/SabriMMO/Materials/Environment/v7"
MAT_NAME = "M_RO_Triplanar_v2"
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

# Layout columns
X_WORLD = -2200
X_UV    = -1800
X_DIV   = -1400
X_TEX   = -900
X_BLEND = -400
X_MIX   = 200
X_DIST  = 600
X_OUT   = 900

# ============================================================
# SECTION 1: World Position + Component Masks
# ============================================================

unreal.log("  World position + axis masks...")

world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, X_WORLD, 0)

mask_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_WORLD + 200, -100)
mask_xy.set_editor_property("r", True)
mask_xy.set_editor_property("g", True)
mask_xy.set_editor_property("b", False)
mask_xy.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xy, "")

mask_xz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_WORLD + 200, 100)
mask_xz.set_editor_property("r", True)
mask_xz.set_editor_property("g", False)
mask_xz.set_editor_property("b", True)
mask_xz.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xz, "")

mask_yz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_WORLD + 200, 300)
mask_yz.set_editor_property("r", False)
mask_yz.set_editor_property("g", True)
mask_yz.set_editor_property("b", True)
mask_yz.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_yz, "")

# ============================================================
# SECTION 2: UV Distortion
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

gnd_uv_xy = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, -100)
mel.connect_material_expressions(uv_xy, "", gnd_uv_xy, "A")
mel.connect_material_expressions(tile_size, "", gnd_uv_xy, "B")

gnd_uv_xz = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 100)
mel.connect_material_expressions(uv_xz, "", gnd_uv_xz, "A")
mel.connect_material_expressions(tile_size, "", gnd_uv_xz, "B")

gnd_uv_yz = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_DIV + 200, 300)
mel.connect_material_expressions(uv_yz, "", gnd_uv_yz, "A")
mel.connect_material_expressions(tile_size, "", gnd_uv_yz, "B")

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
# SECTION 4: Color texture sampling (6 TextureSampleParameter2D)
# ============================================================

# Load a default texture so the parent compiles
default_tex = unreal.load_asset("/Game/SabriMMO/Textures/Environment/RO_Original/001")
if not default_tex:
    unreal.log_warning("Default texture 001 not found, trying engine default...")
    default_tex = unreal.load_asset("/Engine/EngineMaterials/DefaultDiffuse")
default_nrm = unreal.load_asset("/Engine/EngineMaterials/DefaultNormal")

unreal.log("  Color texture samples (6)...")

tex_gnd_xy = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, -300)
tex_gnd_xy.set_editor_property("parameter_name", "GroundTexture")
if default_tex: tex_gnd_xy.set_editor_property("texture", default_tex)
mel.connect_material_expressions(gnd_uv_xy, "", tex_gnd_xy, "UVs")

tex_gnd_xz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, -150)
tex_gnd_xz.set_editor_property("parameter_name", "GroundTextureXZ")
if default_tex: tex_gnd_xz.set_editor_property("texture", default_tex)
mel.connect_material_expressions(gnd_uv_xz, "", tex_gnd_xz, "UVs")

tex_gnd_yz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 0)
tex_gnd_yz.set_editor_property("parameter_name", "GroundTextureYZ")
if default_tex: tex_gnd_yz.set_editor_property("texture", default_tex)
mel.connect_material_expressions(gnd_uv_yz, "", tex_gnd_yz, "UVs")

tex_clf_xy = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 500)
tex_clf_xy.set_editor_property("parameter_name", "CliffTexture")
if default_tex: tex_clf_xy.set_editor_property("texture", default_tex)
mel.connect_material_expressions(clf_uv_xy, "", tex_clf_xy, "UVs")

tex_clf_xz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 650)
tex_clf_xz.set_editor_property("parameter_name", "CliffTextureXZ")
if default_tex: tex_clf_xz.set_editor_property("texture", default_tex)
mel.connect_material_expressions(clf_uv_xz, "", tex_clf_xz, "UVs")

tex_clf_yz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, 800)
tex_clf_yz.set_editor_property("parameter_name", "CliffTextureYZ")
if default_tex: tex_clf_yz.set_editor_property("texture", default_tex)
mel.connect_material_expressions(clf_uv_yz, "", tex_clf_yz, "UVs")

# ============================================================
# SECTION 4b: Normal map sampling (6 TextureSampleParameter2D)
# ============================================================

unreal.log("  Normal map samples (6)...")

Y_NORM = 1100  # offset normals below color textures

nrm_gnd_xy = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, Y_NORM)
nrm_gnd_xy.set_editor_property("parameter_name", "GroundNormal")
nrm_gnd_xy.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if default_nrm: nrm_gnd_xy.set_editor_property("texture", default_nrm)
mel.connect_material_expressions(gnd_uv_xy, "", nrm_gnd_xy, "UVs")

nrm_gnd_xz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, Y_NORM + 150)
nrm_gnd_xz.set_editor_property("parameter_name", "GroundNormalXZ")
nrm_gnd_xz.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if default_nrm: nrm_gnd_xz.set_editor_property("texture", default_nrm)
mel.connect_material_expressions(gnd_uv_xz, "", nrm_gnd_xz, "UVs")

nrm_gnd_yz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, Y_NORM + 300)
nrm_gnd_yz.set_editor_property("parameter_name", "GroundNormalYZ")
nrm_gnd_yz.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if default_nrm: nrm_gnd_yz.set_editor_property("texture", default_nrm)
mel.connect_material_expressions(gnd_uv_yz, "", nrm_gnd_yz, "UVs")

nrm_clf_xy = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, Y_NORM + 500)
nrm_clf_xy.set_editor_property("parameter_name", "CliffNormal")
nrm_clf_xy.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if default_nrm: nrm_clf_xy.set_editor_property("texture", default_nrm)
mel.connect_material_expressions(clf_uv_xy, "", nrm_clf_xy, "UVs")

nrm_clf_xz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, Y_NORM + 650)
nrm_clf_xz.set_editor_property("parameter_name", "CliffNormalXZ")
nrm_clf_xz.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if default_nrm: nrm_clf_xz.set_editor_property("texture", default_nrm)
mel.connect_material_expressions(clf_uv_xz, "", nrm_clf_xz, "UVs")

nrm_clf_yz = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, X_TEX, Y_NORM + 800)
nrm_clf_yz.set_editor_property("parameter_name", "CliffNormalYZ")
nrm_clf_yz.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if default_nrm: nrm_clf_yz.set_editor_property("texture", default_nrm)
mel.connect_material_expressions(clf_uv_yz, "", nrm_clf_yz, "UVs")

# ============================================================
# SECTION 5: Triplanar blend weights
# ============================================================

unreal.log("  Triplanar blend weights...")

vert_normal = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, X_BLEND - 200, -200)

abs_normal = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, X_BLEND - 100, -200)
mel.connect_material_expressions(vert_normal, "", abs_normal, "")

tri_sharp = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_BLEND - 200, -350)
tri_sharp.set_editor_property("parameter_name", "TriplanarSharpness")
tri_sharp.set_editor_property("default_value", 4.0)

sharp_pow = mel.create_material_expression(mat, unreal.MaterialExpressionPower, X_BLEND, -250)
mel.connect_material_expressions(abs_normal, "", sharp_pow, "Base")
mel.connect_material_expressions(tri_sharp, "", sharp_pow, "Exponent")

break_r = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_BLEND + 150, -350)
break_r.set_editor_property("r", True); break_r.set_editor_property("g", False)
break_r.set_editor_property("b", False); break_r.set_editor_property("a", False)
mel.connect_material_expressions(sharp_pow, "", break_r, "")

break_g = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_BLEND + 150, -250)
break_g.set_editor_property("r", False); break_g.set_editor_property("g", True)
break_g.set_editor_property("b", False); break_g.set_editor_property("a", False)
mel.connect_material_expressions(sharp_pow, "", break_g, "")

break_b = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_BLEND + 150, -150)
break_b.set_editor_property("r", False); break_b.set_editor_property("g", False)
break_b.set_editor_property("b", True); break_b.set_editor_property("a", False)
mel.connect_material_expressions(sharp_pow, "", break_b, "")

sum_rg = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 300, -300)
mel.connect_material_expressions(break_r, "", sum_rg, "A")
mel.connect_material_expressions(break_g, "", sum_rg, "B")

sum_rgb = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 450, -250)
mel.connect_material_expressions(sum_rg, "", sum_rgb, "A")
mel.connect_material_expressions(break_b, "", sum_rgb, "B")

w_x = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_BLEND + 600, -350)
mel.connect_material_expressions(break_r, "", w_x, "A")
mel.connect_material_expressions(sum_rgb, "", w_x, "B")

w_y = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_BLEND + 600, -250)
mel.connect_material_expressions(break_g, "", w_y, "A")
mel.connect_material_expressions(sum_rgb, "", w_y, "B")

w_z = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_BLEND + 600, -150)
mel.connect_material_expressions(break_b, "", w_z, "A")
mel.connect_material_expressions(sum_rgb, "", w_z, "B")

# ============================================================
# SECTION 6: Triplanar color blend (ground + cliff)
# ============================================================

unreal.log("  Triplanar color blend...")

# Ground color = xy*wz + xz*wy + yz*wx
gc_xy = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, -300)
mel.connect_material_expressions(tex_gnd_xy, "RGB", gc_xy, "A")
mel.connect_material_expressions(w_z, "", gc_xy, "B")

gc_xz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, -150)
mel.connect_material_expressions(tex_gnd_xz, "RGB", gc_xz, "A")
mel.connect_material_expressions(w_y, "", gc_xz, "B")

gc_yz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 0)
mel.connect_material_expressions(tex_gnd_yz, "RGB", gc_yz, "A")
mel.connect_material_expressions(w_x, "", gc_yz, "B")

gc_s1 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1000, -200)
mel.connect_material_expressions(gc_xy, "", gc_s1, "A")
mel.connect_material_expressions(gc_xz, "", gc_s1, "B")

gnd_color = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1150, -100)
mel.connect_material_expressions(gc_s1, "", gnd_color, "A")
mel.connect_material_expressions(gc_yz, "", gnd_color, "B")

# Cliff color
cc_xy = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 500)
mel.connect_material_expressions(tex_clf_xy, "RGB", cc_xy, "A")
mel.connect_material_expressions(w_z, "", cc_xy, "B")

cc_xz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 650)
mel.connect_material_expressions(tex_clf_xz, "RGB", cc_xz, "A")
mel.connect_material_expressions(w_y, "", cc_xz, "B")

cc_yz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, 800)
mel.connect_material_expressions(tex_clf_yz, "RGB", cc_yz, "A")
mel.connect_material_expressions(w_x, "", cc_yz, "B")

cc_s1 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1000, 600)
mel.connect_material_expressions(cc_xy, "", cc_s1, "A")
mel.connect_material_expressions(cc_xz, "", cc_s1, "B")

clf_color = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1150, 700)
mel.connect_material_expressions(cc_s1, "", clf_color, "A")
mel.connect_material_expressions(cc_yz, "", clf_color, "B")

# ============================================================
# SECTION 6b: Triplanar normal blend (ground + cliff)
# ============================================================

unreal.log("  Triplanar normal blend...")

Y_NB = 1100  # normal blend Y offset

# Ground normal
gn_xy = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, Y_NB)
mel.connect_material_expressions(nrm_gnd_xy, "RGB", gn_xy, "A")
mel.connect_material_expressions(w_z, "", gn_xy, "B")

gn_xz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, Y_NB + 150)
mel.connect_material_expressions(nrm_gnd_xz, "RGB", gn_xz, "A")
mel.connect_material_expressions(w_y, "", gn_xz, "B")

gn_yz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, Y_NB + 300)
mel.connect_material_expressions(nrm_gnd_yz, "RGB", gn_yz, "A")
mel.connect_material_expressions(w_x, "", gn_yz, "B")

gn_s1 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1000, Y_NB + 50)
mel.connect_material_expressions(gn_xy, "", gn_s1, "A")
mel.connect_material_expressions(gn_xz, "", gn_s1, "B")

gnd_nrm = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1150, Y_NB + 150)
mel.connect_material_expressions(gn_s1, "", gnd_nrm, "A")
mel.connect_material_expressions(gn_yz, "", gnd_nrm, "B")

# Cliff normal
cn_xy = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, Y_NB + 500)
mel.connect_material_expressions(nrm_clf_xy, "RGB", cn_xy, "A")
mel.connect_material_expressions(w_z, "", cn_xy, "B")

cn_xz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, Y_NB + 650)
mel.connect_material_expressions(nrm_clf_xz, "RGB", cn_xz, "A")
mel.connect_material_expressions(w_y, "", cn_xz, "B")

cn_yz = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_BLEND + 800, Y_NB + 800)
mel.connect_material_expressions(nrm_clf_yz, "RGB", cn_yz, "A")
mel.connect_material_expressions(w_x, "", cn_yz, "B")

cn_s1 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1000, Y_NB + 550)
mel.connect_material_expressions(cn_xy, "", cn_s1, "A")
mel.connect_material_expressions(cn_xz, "", cn_s1, "B")

clf_nrm = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_BLEND + 1150, Y_NB + 650)
mel.connect_material_expressions(cn_s1, "", clf_nrm, "A")
mel.connect_material_expressions(cn_yz, "", clf_nrm, "B")

# ============================================================
# SECTION 7: Macro variation (small-scale anti-tiling)
# ============================================================

unreal.log("  Macro variation...")

macro_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, X_MIX - 200, -400)
macro_noise.set_editor_property("scale", 0.0008)
macro_noise.set_editor_property("levels", 3)
macro_noise.set_editor_property("output_min", 0.0)
macro_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", macro_noise, "Position")

macro_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_MIX - 400, -450)
macro_strength.set_editor_property("parameter_name", "MacroStrength")
macro_strength.set_editor_property("default_value", 0.08)

macro_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_MIX - 300, -500)
macro_one.set_editor_property("r", 1.0)

macro_lo = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, X_MIX - 100, -500)
mel.connect_material_expressions(macro_one, "", macro_lo, "A")
mel.connect_material_expressions(macro_strength, "", macro_lo, "B")

macro_hi = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_MIX - 100, -400)
mel.connect_material_expressions(macro_one, "", macro_hi, "A")
mel.connect_material_expressions(macro_strength, "", macro_hi, "B")

macro_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, X_MIX + 50, -450)
mel.connect_material_expressions(macro_lo, "", macro_lerp, "A")
mel.connect_material_expressions(macro_hi, "", macro_lerp, "B")
mel.connect_material_expressions(macro_noise, "", macro_lerp, "Alpha")

gnd_macro = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_MIX + 200, -100)
mel.connect_material_expressions(gnd_color, "", gnd_macro, "A")
mel.connect_material_expressions(macro_lerp, "", gnd_macro, "B")

# ============================================================
# SECTION 8: Slope detection (45-degree cutoff)
# ============================================================

unreal.log("  Slope detection...")

world_up = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, X_MIX - 200, 200)
world_up.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))

slope_dot = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, X_MIX, 250)
mel.connect_material_expressions(vert_normal, "", slope_dot, "A")
mel.connect_material_expressions(world_up, "", slope_dot, "B")

slope_thresh = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_MIX - 200, 350)
slope_thresh.set_editor_property("parameter_name", "SlopeThreshold")
slope_thresh.set_editor_property("default_value", 0.60)

slope_sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, X_MIX + 100, 300)
mel.connect_material_expressions(slope_dot, "", slope_sub, "A")
mel.connect_material_expressions(slope_thresh, "", slope_sub, "B")

slope_width = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_MIX - 200, 450)
slope_width.set_editor_property("parameter_name", "SlopeTransitionWidth")
slope_width.set_editor_property("default_value", 0.15)

slope_div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, X_MIX + 200, 350)
mel.connect_material_expressions(slope_sub, "", slope_div, "A")
mel.connect_material_expressions(slope_width, "", slope_div, "B")

slope_clamp = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, X_MIX + 300, 350)
mel.connect_material_expressions(slope_div, "", slope_clamp, "")

# Slope blend: color
color_slope = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, X_MIX + 450, 100)
mel.connect_material_expressions(clf_color, "", color_slope, "A")   # steep = cliff
mel.connect_material_expressions(gnd_macro, "", color_slope, "B")   # flat = ground+macro
mel.connect_material_expressions(slope_clamp, "", color_slope, "Alpha")

# Slope blend: normal
normal_slope = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, X_MIX + 450, Y_NB + 300)
mel.connect_material_expressions(clf_nrm, "", normal_slope, "A")
mel.connect_material_expressions(gnd_nrm, "", normal_slope, "B")
mel.connect_material_expressions(slope_clamp, "", normal_slope, "Alpha")

# ============================================================
# SECTION 9: Distance-based macro tinting
# ============================================================

unreal.log("  Distance macro tinting...")

# Large-scale noise (10x larger than regular macro) that shifts
# brightness based on world position. Breaks tiling at distance.

dist_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, X_DIST - 200, -200)
dist_noise.set_editor_property("scale", 0.00015)  # very large scale
dist_noise.set_editor_property("levels", 4)
dist_noise.set_editor_property("output_min", 0.0)
dist_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", dist_noise, "Position")

# Second noise at different frequency for hue variation
dist_noise2 = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, X_DIST - 200, 0)
dist_noise2.set_editor_property("scale", 0.00025)
dist_noise2.set_editor_property("levels", 3)
dist_noise2.set_editor_property("output_min", -1.0)
dist_noise2.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", dist_noise2, "Position")

dist_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_DIST - 400, -100)
dist_strength.set_editor_property("parameter_name", "DistanceMacroStrength")
dist_strength.set_editor_property("default_value", 0.15)

# Brightness shift: lerp(1-str, 1+str, noise1)
d_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_DIST - 300, -300)
d_one.set_editor_property("r", 1.0)

d_lo = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, X_DIST - 100, -300)
mel.connect_material_expressions(d_one, "", d_lo, "A")
mel.connect_material_expressions(dist_strength, "", d_lo, "B")

d_hi = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_DIST - 100, -200)
mel.connect_material_expressions(d_one, "", d_hi, "A")
mel.connect_material_expressions(dist_strength, "", d_hi, "B")

d_brightness = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, X_DIST + 50, -250)
mel.connect_material_expressions(d_lo, "", d_brightness, "A")
mel.connect_material_expressions(d_hi, "", d_brightness, "B")
mel.connect_material_expressions(dist_noise, "", d_brightness, "Alpha")

# Warm/cool hue shift: subtle (noise2 * strength * 0.3) added to R, subtracted from B
hue_scale = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_DIST, 50)
mel.connect_material_expressions(dist_noise2, "", hue_scale, "A")
mel.connect_material_expressions(dist_strength, "", hue_scale, "B")

hue_scale2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_DIST + 100, 50)
hue_third = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_DIST, 100)
hue_third.set_editor_property("r", 0.3)
mel.connect_material_expressions(hue_scale, "", hue_scale2, "A")
mel.connect_material_expressions(hue_third, "", hue_scale2, "B")

# Build tint vector: (1+hue, 1, 1-hue) * brightness
tint_r = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, X_DIST + 200, -50)
mel.connect_material_expressions(d_one, "", tint_r, "A")
mel.connect_material_expressions(hue_scale2, "", tint_r, "B")

tint_b = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, X_DIST + 200, 100)
mel.connect_material_expressions(d_one, "", tint_b, "A")
mel.connect_material_expressions(hue_scale2, "", tint_b, "B")

# AppendVector to build RGB tint
tint_rg = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, X_DIST + 350, -20)
mel.connect_material_expressions(tint_r, "", tint_rg, "A")
mel.connect_material_expressions(d_one, "", tint_rg, "B")  # G stays at 1

tint_rgb = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, X_DIST + 500, 20)
mel.connect_material_expressions(tint_rg, "", tint_rgb, "A")
mel.connect_material_expressions(tint_b, "", tint_rgb, "B")

# Multiply brightness
tint_final = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_DIST + 650, -100)
mel.connect_material_expressions(tint_rgb, "", tint_final, "A")
mel.connect_material_expressions(d_brightness, "", tint_final, "B")

# Apply distance tint to final color
final_color = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_OUT, 0)
mel.connect_material_expressions(color_slope, "", final_color, "A")
mel.connect_material_expressions(tint_final, "", final_color, "B")

# ============================================================
# SECTION 10: Normal strength scaling
# ============================================================

unreal.log("  Normal strength...")

nrm_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, X_OUT - 200, Y_NB + 200)
nrm_strength.set_editor_property("parameter_name", "NormalStrength")
nrm_strength.set_editor_property("default_value", 0.5)

# Extract XY from blended normal, scale by strength
nrm_mask_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, X_OUT - 100, Y_NB + 300)
nrm_mask_xy.set_editor_property("r", True)
nrm_mask_xy.set_editor_property("g", True)
nrm_mask_xy.set_editor_property("b", False)
nrm_mask_xy.set_editor_property("a", False)
mel.connect_material_expressions(normal_slope, "", nrm_mask_xy, "")

# Scale XY deviation by NormalStrength
nrm_scaled = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, X_OUT, Y_NB + 300)
mel.connect_material_expressions(nrm_mask_xy, "", nrm_scaled, "A")
mel.connect_material_expressions(nrm_strength, "", nrm_scaled, "B")

# Reconstruct: append Z=1 (flat normal Z component)
nrm_z = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_OUT, Y_NB + 450)
nrm_z.set_editor_property("r", 1.0)

nrm_final = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, X_OUT + 150, Y_NB + 350)
mel.connect_material_expressions(nrm_scaled, "", nrm_final, "A")
mel.connect_material_expressions(nrm_z, "", nrm_final, "B")

# ============================================================
# SECTION 11: Outputs
# ============================================================

unreal.log("  Connecting outputs...")

mel.connect_material_property(final_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(nrm_final, "", unreal.MaterialProperty.MP_NORMAL)

roughness = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_OUT + 200, 200)
roughness.set_editor_property("r", 0.95)
mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

metallic = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, X_OUT + 200, 300)
metallic.set_editor_property("r", 0.0)
mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

# ============================================================
# Save & compile
# ============================================================

mel.layout_material_expressions(mat)
mel.recompile_material(mat)
eal.save_asset(FULL_PATH)

unreal.log(f"\nSUCCESS: {MAT_NAME} created at {FULL_PATH}")
unreal.log(f"New features over M_RO_Triplanar:")
unreal.log(f"  1. Triplanar normal maps (GroundNormal + CliffNormal)")
unreal.log(f"  2. Distance macro tinting (large-scale brightness + warm/cool hue)")
unreal.log(f"  3. NormalStrength (0.5), DistanceMacroStrength (0.15)")
