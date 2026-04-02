# create_landscape_material.py
# Run in UE5 Editor via: Edit > Execute Python Script
#
# Creates M_Landscape_RO — a 4-layer auto-blending landscape material:
#   Layer 1: Grass Warm (olive/sage — flat surfaces)
#   Layer 2: Grass Cool (yellow-green — noise-blended patches)
#   Layer 3: Dirt (ochre — noise-driven worn patches)
#   Layer 4: Rock (purple-gray — steep slopes, automatic)
#
# Features:
#   - Slope-based grass→rock blending (DotProduct of normal vs world up)
#   - Noise-driven grass variant splotches (multi-green RO look)
#   - Noise-driven dirt patches (worn areas)
#   - Macro variation noise (breaks tile repetition)
#   - World-space UVs (consistent tiling across landscape)
#   - Roughness 0.95, Metallic 0.0, no normal maps (RO Classic style)
#   - All textures are parameters — assignable in Material Instance
#
# Reference: docsNew/05_Development/Ground_Texture_System_Research.md

import unreal

# ============================================================
# Configuration
# ============================================================

MAT_PATH = "/Game/SabriMMO/Materials/Environment"
# INCREMENT THIS NUMBER for each new variant (keeps all previous versions)
MAT_VERSION = 17
MAT_NAME = f"M_Landscape_RO_{MAT_VERSION:02d}"
FULL_PATH = f"{MAT_PATH}/{MAT_NAME}"
# DESCRIPTION — shows in UE5 tooltip when hovering and in tracking doc
MAT_DESCRIPTION = "v17 — Paintable grass layers. LandscapeLayerWeight for manual brush control of grass density zones. All v14 features plus paint layers."

# UV tiling: texture repeats every N unreal units
# Each layer uses a DIFFERENT irrational-ratio size so seams never align
GRASS_WARM_TILE_SIZE = 4000.0
GRASS_COOL_TILE_SIZE = 5173.0   # irrational ratio to warm — seams never align
DIRT_TILE_SIZE = 3347.0
ROCK_TILE_SIZE = 2891.0

# UV noise distortion strength (warps seam lines so they're wavy, not straight)
UV_DISTORT_SCALE = 0.003     # noise frequency for UV warping
UV_DISTORT_STRENGTH = 60.0   # how many UU to shift UVs (higher = more warp)

# Noise scales — must create visible variation across the floor
# Higher = faster variation = more distinct patches
GRASS_VARIANT_NOISE_SCALE = 0.004    # distinct green patches every ~250 UU
MACRO_VARIATION_NOISE_SCALE = 0.001  # brightness swaths every ~1000 UU
DIRT_PATCH_NOISE_SCALE = 0.006       # dirt patches every ~170 UU

# Slope blending
SLOPE_POWER = 2.0  # Original-ish balance — rock visible on moderate-steep slopes

# ============================================================
# Helper
# ============================================================

mel = unreal.MaterialEditingLibrary


def try_load_texture(path):
    """Try to load a texture asset, return None if not found."""
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    return None


# ============================================================
# Create material
# ============================================================

unreal.EditorAssetLibrary.make_directory(MAT_PATH)

if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
    # Re-run safe: load existing material and clear its graph
    unreal.log_warning(f"{MAT_NAME} already exists — clearing expressions and rebuilding.")
    mat = unreal.load_asset(FULL_PATH)
    if mat:
        mel.delete_all_material_expressions(mat)
else:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = asset_tools.create_asset(
        MAT_NAME, MAT_PATH,
        unreal.Material, unreal.MaterialFactoryNew()
    )

if not mat:
    unreal.log_error(f"FAILED to create/load {MAT_NAME}!")
    raise RuntimeError("Material creation failed")

unreal.log(f"Creating {MAT_NAME}...")

# Material properties — DefaultLit, Opaque, two-sided OFF
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
mat.set_editor_property("two_sided", False)

# ============================================================
# SECTION 1: World-Space UV Generation
# ============================================================
# WorldPosition.XY / TileSize → UVs for texture sampling
# This ensures textures tile consistently regardless of mesh UVs

unreal.log("  Creating world-space UV nodes with noise distortion...")

# World Position
world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -2000, 0)

# Component Mask — extract XY only
mask_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1800, 0)
mask_xy.set_editor_property("r", True)
mask_xy.set_editor_property("g", True)
mask_xy.set_editor_property("b", False)
mask_xy.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", mask_xy, "")

# === UV NOISE DISTORTION ===
# Adds noise offset to UVs so tile seam lines become wavy instead of straight
# This is the key anti-seam technique
uv_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -1800, -300)
uv_noise.set_editor_property("scale", UV_DISTORT_SCALE)
uv_noise.set_editor_property("levels", 2)
uv_noise.set_editor_property("output_min", -1.0)
uv_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", uv_noise, "Position")

# Scale noise by distortion strength
uv_distort_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1600, -300)
uv_distort_const = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1800, -250)
uv_distort_const.set_editor_property("parameter_name", "UVDistortStrength")
uv_distort_const.set_editor_property("default_value", UV_DISTORT_STRENGTH)
mel.connect_material_expressions(uv_noise, "", uv_distort_mult, "A")
mel.connect_material_expressions(uv_distort_const, "", uv_distort_mult, "B")

# Add noise offset to base XY position (distorted position)
uv_distorted = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -1400, -100)
mel.connect_material_expressions(mask_xy, "", uv_distorted, "A")
mel.connect_material_expressions(uv_distort_mult, "", uv_distorted, "B")

# Helper: create a UV divide node with parameterized tile size
def make_uv_div_param(param_name, default_size, pos_y):
    div_node = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -1200, pos_y)
    div_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1400, pos_y + 50)
    div_param.set_editor_property("parameter_name", param_name)
    div_param.set_editor_property("default_value", default_size)
    mel.connect_material_expressions(uv_distorted, "", div_node, "A")
    mel.connect_material_expressions(div_param, "", div_node, "B")
    return div_node

# Each layer gets DIFFERENT tile size (irrational ratios — seams never align)
grass_warm_uv = make_uv_div_param("GrassWarmTileSize", GRASS_WARM_TILE_SIZE, -400)
grass_cool_uv = make_uv_div_param("GrassCoolTileSize", GRASS_COOL_TILE_SIZE, -200)
dirt_uv_div = make_uv_div_param("DirtTileSize", DIRT_TILE_SIZE, 100)

# Rock uses XZ projection (not XY) — maps correctly on vertical cliff faces
# Extract X and Z from world position instead of X and Y
rock_mask_xz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1800, 500)
rock_mask_xz.set_editor_property("r", True)   # X
rock_mask_xz.set_editor_property("g", False)   # skip Y
rock_mask_xz.set_editor_property("b", True)    # Z (height)
rock_mask_xz.set_editor_property("a", False)
mel.connect_material_expressions(world_pos, "", rock_mask_xz, "")

# Add UV distortion to rock XZ coords too
rock_uv_distorted = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -1400, 500)
mel.connect_material_expressions(rock_mask_xz, "", rock_uv_distorted, "A")
mel.connect_material_expressions(uv_distort_mult, "", rock_uv_distorted, "B")

rock_uv_div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -1200, 500)
rock_uv_const = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1400, 550)
rock_uv_const.set_editor_property("parameter_name", "RockTileSize")
rock_uv_const.set_editor_property("default_value", ROCK_TILE_SIZE)
mel.connect_material_expressions(rock_uv_distorted, "", rock_uv_div, "A")
mel.connect_material_expressions(rock_uv_const, "", rock_uv_div, "B")

# ============================================================
# SECTION 2: Texture Parameters (user-assignable)
# ============================================================

unreal.log("  Creating texture parameter nodes...")

# Grass Warm — primary ground cover
tex_grass_warm = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -400)
tex_grass_warm.set_editor_property("parameter_name", "GrassWarmTexture")
mel.connect_material_expressions(grass_warm_uv, "", tex_grass_warm, "UVs")

# Grass Cool — variant patches (DIFFERENT tile size so seams don't align with warm)
tex_grass_cool = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -200)
tex_grass_cool.set_editor_property("parameter_name", "GrassCoolTexture")
mel.connect_material_expressions(grass_cool_uv, "", tex_grass_cool, "UVs")

# Dirt
tex_dirt = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 100)
tex_dirt.set_editor_property("parameter_name", "DirtTexture")
mel.connect_material_expressions(dirt_uv_div, "", tex_dirt, "UVs")

# Rock
tex_rock = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 400)
tex_rock.set_editor_property("parameter_name", "RockTexture")
mel.connect_material_expressions(rock_uv_div, "", tex_rock, "UVs")

# Try to assign existing textures — search multiple paths per slot
tex_nodes = {
    "GrassWarmTexture": tex_grass_warm,
    "GrassCoolTexture": tex_grass_cool,
    "DirtTexture": tex_dirt,
    "RockTexture": tex_rock,
}

tex_candidates = {
    "GrassWarmTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_LushGreen_03_2k",
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_LushGreen_03_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_LushGreen_03",
        "/Game/SabriMMO/Textures/Environment/T_Ground_GrassDirt_RO",
    ],
    "GrassCoolTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_LushGreen_02_2k",
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_LushGreen_02_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_LushGreen_02",
        "/Game/SabriMMO/Textures/Environment/T_Grass_RO",
    ],
    "DirtTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_EarthPath_01_2k",
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_EarthPath_01_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_EarthPath_01",
        "/Game/SabriMMO/Textures/Environment/T_Ground_Dirt_RO",
    ],
    "RockTexture": [
        "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_StoneCliff_02_2k",
        "/Game/SabriMMO/Textures/Environment/Ground_2K/T_StoneCliff_02_2k",
        "/Game/SabriMMO/Textures/Environment/Ground/T_StoneCliff_02",
        "/Game/SabriMMO/Textures/Environment/T_Ground_Stone_RO",
    ],
}

for param_name, node in tex_nodes.items():
    tex = None
    used_path = None
    for candidate in tex_candidates[param_name]:
        tex = try_load_texture(candidate)
        if tex:
            used_path = candidate
            break
    if tex:
        node.set_editor_property("texture", tex)
        unreal.log(f"    Assigned {param_name} = {used_path}")
    else:
        unreal.log_warning(f"    {param_name}: no texture found — assign manually in Material Editor")

# ============================================================
# SECTION 3: Grass Variant Blending (splotchy multi-green)
# ============================================================
# Noise at large scale → Lerp between GrassWarm and GrassCool
# Creates the characteristic RO splotchy grass look

unreal.log("  Creating grass variant blending...")

# GrassNoiseScale parameter — controls splotchy patch size
grass_noise_scale_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, -350)
grass_noise_scale_param.set_editor_property("parameter_name", "GrassNoiseScale")
grass_noise_scale_param.set_editor_property("default_value", GRASS_VARIANT_NOISE_SCALE)

# Multiply WorldPos by noise scale (workaround: Noise node scale is property not pin)
grass_noise_pos = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -750, -320)
mel.connect_material_expressions(world_pos, "", grass_noise_pos, "A")
mel.connect_material_expressions(grass_noise_scale_param, "", grass_noise_pos, "B")

# Noise for grass variant mask
grass_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -600, -300)
grass_noise.set_editor_property("scale", 1.0)  # scale=1 since we pre-multiply position
grass_noise.set_editor_property("levels", 2)
grass_noise.set_editor_property("output_min", 0.0)
grass_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(grass_noise_pos, "", grass_noise, "Position")

# GrassVariantBalance — bias the A/B blend (0.2 = mostly A, 0.8 = mostly B)
grass_balance = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, -250)
grass_balance.set_editor_property("parameter_name", "GrassVariantBalance")
grass_balance.set_editor_property("default_value", 0.5)

# Bias noise: clamp(noise + (balance - 0.5) * 2)
balance_offset_sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -650, -250)
balance_half = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -800, -200)
balance_half.set_editor_property("r", 0.5)
mel.connect_material_expressions(grass_balance, "", balance_offset_sub, "A")
mel.connect_material_expressions(balance_half, "", balance_offset_sub, "B")

balance_scale = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -550, -250)
balance_two = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -650, -200)
balance_two.set_editor_property("r", 2.0)
mel.connect_material_expressions(balance_offset_sub, "", balance_scale, "A")
mel.connect_material_expressions(balance_two, "", balance_scale, "B")

biased_noise = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -450, -280)
mel.connect_material_expressions(grass_noise, "", biased_noise, "A")
mel.connect_material_expressions(balance_scale, "", biased_noise, "B")

biased_clamped = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, -350, -280)
mel.connect_material_expressions(biased_noise, "", biased_clamped, "")

# Lerp: GrassWarm <-> GrassCool based on biased noise
grass_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -250, -300)
mel.connect_material_expressions(tex_grass_warm, "RGB", grass_lerp, "A")
mel.connect_material_expressions(tex_grass_cool, "RGB", grass_lerp, "B")
mel.connect_material_expressions(biased_clamped, "", grass_lerp, "Alpha")

# ============================================================
# SECTION 4: Macro Variation (anti-tile-repetition)
# ============================================================
# Large-scale noise that subtly darkens/brightens patches
# Hides the repeating tile pattern over large areas

unreal.log("  Creating macro variation...")

# MacroNoiseScale parameter
macro_noise_scale_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, -100)
macro_noise_scale_param.set_editor_property("parameter_name", "MacroNoiseScale")
macro_noise_scale_param.set_editor_property("default_value", MACRO_VARIATION_NOISE_SCALE)

macro_noise_pos = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -750, -100)
mel.connect_material_expressions(world_pos, "", macro_noise_pos, "A")
mel.connect_material_expressions(macro_noise_scale_param, "", macro_noise_pos, "B")

# Macro noise — very large scale
macro_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -600, -100)
macro_noise.set_editor_property("scale", 1.0)
macro_noise.set_editor_property("levels", 3)
macro_noise.set_editor_property("output_min", 0.0)
macro_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(macro_noise_pos, "", macro_noise, "Position")

# Remap noise to 0.88-1.12 range (±12% brightness variation)
macro_range_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, -100)
macro_min = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, -50)
macro_min.set_editor_property("parameter_name", "MacroDarken")
macro_min.set_editor_property("default_value", 0.88)
macro_max = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 0)
macro_max.set_editor_property("parameter_name", "MacroBrighten")
macro_max.set_editor_property("default_value", 1.12)
mel.connect_material_expressions(macro_min, "", macro_range_lerp, "A")
mel.connect_material_expressions(macro_max, "", macro_range_lerp, "B")
mel.connect_material_expressions(macro_noise, "", macro_range_lerp, "Alpha")

# Multiply grass by macro variation factor
grass_macro = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -200)
mel.connect_material_expressions(grass_lerp, "", grass_macro, "A")
mel.connect_material_expressions(macro_range_lerp, "", grass_macro, "B")

# ============================================================
# SECTION 5: Dirt Patches (noise-driven worn areas)
# ============================================================

unreal.log("  Creating dirt patch blending...")

# Noise for dirt patch placement
dirt_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -700, 100)
dirt_noise.set_editor_property("scale", DIRT_PATCH_NOISE_SCALE)
dirt_noise.set_editor_property("levels", 3)
dirt_noise.set_editor_property("output_min", 0.0)
dirt_noise.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(world_pos, "", dirt_noise, "Position")

# Square the noise for sharper dirt patches (Power of 2 via self-multiply)
dirt_mask_sq = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 100)
mel.connect_material_expressions(dirt_noise, "", dirt_mask_sq, "A")
mel.connect_material_expressions(dirt_noise, "", dirt_mask_sq, "B")

# Scale dirt amount (parameter — user adjustable)
dirt_amount = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 200)
dirt_amount.set_editor_property("parameter_name", "DirtAmount")
dirt_amount.set_editor_property("default_value", 0.25)

dirt_mask_scaled = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 100)
mel.connect_material_expressions(dirt_mask_sq, "", dirt_mask_scaled, "A")
mel.connect_material_expressions(dirt_amount, "", dirt_mask_scaled, "B")

# Lerp: Grass+Macro ↔ Dirt based on dirt mask
grass_dirt_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -100, -50)
mel.connect_material_expressions(grass_macro, "", grass_dirt_lerp, "A")
mel.connect_material_expressions(tex_dirt, "RGB", grass_dirt_lerp, "B")
mel.connect_material_expressions(dirt_mask_scaled, "", grass_dirt_lerp, "Alpha")

# ============================================================
# SECTION 6: Slope-Based Rock Blending (HARD 45-degree cutoff)
# ============================================================
# DotProduct gives slope: 1.0=flat, 0.707=45deg, 0.0=vertical
# We use a tight SmoothStep around 45 degrees for a sharp transition
# Below 40deg = 100% grass, above 50deg = 100% rock, narrow blend between

unreal.log("  Creating slope-based rock blending (hard 45-deg cutoff)...")

# Vertex Normal in World Space
vert_normal = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -500, 400)

# World Up vector (0, 0, 1)
world_up = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -500, 500)
world_up.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))

# DotProduct — slope value (1=flat, 0=vertical)
slope_dot = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -300, 450)
mel.connect_material_expressions(vert_normal, "", slope_dot, "A")
mel.connect_material_expressions(world_up, "", slope_dot, "B")

# === SLOPE NOISE — makes the cutoff line organic/wavy instead of perfectly straight ===
slope_noise_amount = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 650)
slope_noise_amount.set_editor_property("parameter_name", "SlopeNoiseAmount")
slope_noise_amount.set_editor_property("default_value", 0.0)

slope_noise_freq = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 700)
slope_noise_freq.set_editor_property("parameter_name", "SlopeNoiseFreq")
slope_noise_freq.set_editor_property("default_value", 0.01)

slope_noise_pos = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 670)
mel.connect_material_expressions(world_pos, "", slope_noise_pos, "A")
mel.connect_material_expressions(slope_noise_freq, "", slope_noise_pos, "B")

slope_noise_node = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -200, 670)
slope_noise_node.set_editor_property("scale", 1.0)
slope_noise_node.set_editor_property("levels", 2)
slope_noise_node.set_editor_property("output_min", -1.0)
slope_noise_node.set_editor_property("output_max", 1.0)
mel.connect_material_expressions(slope_noise_pos, "", slope_noise_node, "Position")

slope_noise_scaled = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -50, 670)
mel.connect_material_expressions(slope_noise_node, "", slope_noise_scaled, "A")
mel.connect_material_expressions(slope_noise_amount, "", slope_noise_scaled, "B")

# Add noise to slope dot product (makes cutoff wavy)
slope_dot_noisy = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -150, 480)
mel.connect_material_expressions(slope_dot, "", slope_dot_noisy, "A")
mel.connect_material_expressions(slope_noise_scaled, "", slope_dot_noisy, "B")

# === DIRT ON SLOPES — blend dirt into the grass->rock transition band ===
dirt_on_slopes_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 750)
dirt_on_slopes_param.set_editor_property("parameter_name", "DirtOnSlopes")
dirt_on_slopes_param.set_editor_property("default_value", 0.0)

# Step 1: Subtract lower threshold (0.60)
slope_sub_const = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 550)
slope_sub_const.set_editor_property("parameter_name", "SlopeThreshold")
slope_sub_const.set_editor_property("default_value", 0.60)
slope_sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -50, 500)
mel.connect_material_expressions(slope_dot_noisy, "", slope_sub, "A")
mel.connect_material_expressions(slope_sub_const, "", slope_sub, "B")

# Step 2: Divide by range width (0.75 - 0.60 = 0.15)
slope_range_const = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 600)
slope_range_const.set_editor_property("parameter_name", "SlopeTransitionWidth")
slope_range_const.set_editor_property("default_value", 0.15)
slope_div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -50, 550)
mel.connect_material_expressions(slope_sub, "", slope_div, "A")
mel.connect_material_expressions(slope_range_const, "", slope_div, "B")

# Step 3: Clamp 0-1
slope_clamp = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, 50, 550)
mel.connect_material_expressions(slope_div, "", slope_clamp, "")

# Step 4: OneMinus — so 1=rock, 0=grass (currently grass is high)
slope_power = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 150, 550)
mel.connect_material_expressions(slope_clamp, "", slope_power, "")

# Compute transition band mask (where slope is between grass and rock)
# transition_mask = slope_power * (1 - slope_power) * 4  — peaks at 0.5 blend
transition_oneminus = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 200, 600)
mel.connect_material_expressions(slope_power, "", transition_oneminus, "")
transition_mask = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 250, 600)
mel.connect_material_expressions(slope_power, "", transition_mask, "A")
mel.connect_material_expressions(transition_oneminus, "", transition_mask, "B")
transition_mask_scaled = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 300, 600)
transition_four = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 250, 650)
transition_four.set_editor_property("r", 4.0)
mel.connect_material_expressions(transition_mask, "", transition_mask_scaled, "A")
mel.connect_material_expressions(transition_four, "", transition_mask_scaled, "B")

# DirtOnSlopes: blend dirt into transition band
dirt_slope_mask = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 350, 620)
mel.connect_material_expressions(transition_mask_scaled, "", dirt_slope_mask, "A")
mel.connect_material_expressions(dirt_on_slopes_param, "", dirt_slope_mask, "B")

# Blend dirt into grass before rock lerp
grass_dirt_slope = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 200, 50)
mel.connect_material_expressions(grass_dirt_lerp, "", grass_dirt_slope, "A")
mel.connect_material_expressions(tex_dirt, "RGB", grass_dirt_slope, "B")
mel.connect_material_expressions(dirt_slope_mask, "", grass_dirt_slope, "Alpha")

# Final Lerp: GrassDirt ↔ Rock based on slope mask
final_color_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 400, 100)
mel.connect_material_expressions(grass_dirt_slope, "", final_color_lerp, "A")
mel.connect_material_expressions(tex_rock, "RGB", final_color_lerp, "B")
mel.connect_material_expressions(slope_power, "", final_color_lerp, "Alpha")

# ============================================================
# SECTION 7: Normal Maps (subtle depth)
# ============================================================

unreal.log("  Creating normal map blending...")

# Normal map texture parameters
norm_grass_warm = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, 200, -600)
norm_grass_warm.set_editor_property("parameter_name", "GrassWarmNormal")
norm_grass_warm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mel.connect_material_expressions(grass_warm_uv, "", norm_grass_warm, "UVs")

norm_grass_cool = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, 200, -450)
norm_grass_cool.set_editor_property("parameter_name", "GrassCoolNormal")
norm_grass_cool.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mel.connect_material_expressions(grass_cool_uv, "", norm_grass_cool, "UVs")

norm_dirt = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, 200, -300)
norm_dirt.set_editor_property("parameter_name", "DirtNormal")
norm_dirt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mel.connect_material_expressions(dirt_uv_div, "", norm_dirt, "UVs")

norm_rock = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, 200, -150)
norm_rock.set_editor_property("parameter_name", "RockNormal")
norm_rock.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mel.connect_material_expressions(rock_uv_div, "", norm_rock, "UVs")

# Try to assign normal map textures
norm_candidates = {
    "GrassWarmNormal": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals/T_LushGreen_03_2k_Normal",
        "/Game/SabriMMO/Textures/Environment/Ground/Normals/T_LushGreen_03_Normal",
    ],
    "GrassCoolNormal": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals/T_LushGreen_02_2k_Normal",
        "/Game/SabriMMO/Textures/Environment/Ground/Normals/T_LushGreen_02_Normal",
    ],
    "DirtNormal": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals/T_EarthPath_01_2k_Normal",
        "/Game/SabriMMO/Textures/Environment/Ground/Normals/T_EarthPath_01_Normal",
    ],
    "RockNormal": [
        "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals/T_StoneCliff_02_2k_Normal",
        "/Game/SabriMMO/Textures/Environment/Ground/Normals/T_StoneCliff_02_Normal",
    ],
}
norm_nodes = {
    "GrassWarmNormal": norm_grass_warm,
    "GrassCoolNormal": norm_grass_cool,
    "DirtNormal": norm_dirt,
    "RockNormal": norm_rock,
}
for param_name, node in norm_nodes.items():
    for candidate in norm_candidates[param_name]:
        tex = try_load_texture(candidate)
        if tex:
            node.set_editor_property("texture", tex)
            unreal.log(f"    Assigned {param_name} = {candidate}")
            break

# Blend normals with same masks as color (grass noise -> dirt mask -> slope mask)
norm_grass_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 400, -500)
mel.connect_material_expressions(norm_grass_warm, "RGB", norm_grass_lerp, "A")
mel.connect_material_expressions(norm_grass_cool, "RGB", norm_grass_lerp, "B")
mel.connect_material_expressions(grass_noise, "", norm_grass_lerp, "Alpha")

norm_dirt_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 500, -400)
mel.connect_material_expressions(norm_grass_lerp, "", norm_dirt_lerp, "A")
mel.connect_material_expressions(norm_dirt, "RGB", norm_dirt_lerp, "B")
mel.connect_material_expressions(dirt_mask_scaled, "", norm_dirt_lerp, "Alpha")

norm_final_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 600, -300)
mel.connect_material_expressions(norm_dirt_lerp, "", norm_final_lerp, "A")
mel.connect_material_expressions(norm_rock, "RGB", norm_final_lerp, "B")
mel.connect_material_expressions(slope_power, "", norm_final_lerp, "Alpha")

# Normal Strength parameter — keep subtle for RO style (default 0.3)
normal_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 600, -150)
normal_strength.set_editor_property("parameter_name", "NormalStrength")
normal_strength.set_editor_property("default_value", 0.5)

# FlattenNormal approach — scale the XY of the blended normal by strength
# Normal maps in UE5 are tangent-space: XY = deviation, Z = up
# Multiplying XY by strength and keeping Z=1 controls intensity
norm_xy_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 700, -350)
norm_xy_mask.set_editor_property("r", True)
norm_xy_mask.set_editor_property("g", True)
norm_xy_mask.set_editor_property("b", False)
norm_xy_mask.set_editor_property("a", False)
mel.connect_material_expressions(norm_final_lerp, "", norm_xy_mask, "")

# Scale XY by NormalStrength
norm_xy_scaled = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 800, -300)
mel.connect_material_expressions(norm_xy_mask, "", norm_xy_scaled, "A")
mel.connect_material_expressions(normal_strength, "", norm_xy_scaled, "B")

# Append Z=1 to make (scaledX, scaledY, 1)
norm_z = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 800, -200)
norm_z.set_editor_property("r", 1.0)

norm_append = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, 900, -280)
mel.connect_material_expressions(norm_xy_scaled, "", norm_append, "A")
mel.connect_material_expressions(norm_z, "", norm_append, "B")

# ============================================================
# SECTION 8: Ambient Occlusion (depth in crevices)
# ============================================================

unreal.log("  Creating AO blending...")

# AO texture parameter (just for grass — most visible layer)
ao_grass = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, 200, 600)
ao_grass.set_editor_property("parameter_name", "GrassAO")
mel.connect_material_expressions(grass_warm_uv, "", ao_grass, "UVs")

# Try to assign AO texture
ao_candidates = [
    "/Game/SabriMMO/Textures/Environment/Ground_2K/AO/T_LushGreen_03_2k_AO",
    "/Game/SabriMMO/Textures/Environment/Ground/AO/T_LushGreen_03_AO",
]
for candidate in ao_candidates:
    tex = try_load_texture(candidate)
    if tex:
        ao_grass.set_editor_property("texture", tex)
        unreal.log(f"    Assigned GrassAO = {candidate}")
        break

# AO strength parameter
ao_strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 200, 700)
ao_strength.set_editor_property("parameter_name", "AOStrength")
ao_strength.set_editor_property("default_value", 0.4)

# Lerp AO toward white (1.0) based on strength — 0 strength = no AO, 1.0 = full AO
ao_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 300, 700)
ao_one.set_editor_property("r", 1.0)

ao_lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 400, 650)
mel.connect_material_expressions(ao_one, "", ao_lerp, "A")
mel.connect_material_expressions(ao_grass, "R", ao_lerp, "B")
mel.connect_material_expressions(ao_strength, "", ao_lerp, "Alpha")

# Multiply final BaseColor by AO
final_with_ao = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 500, 100)
mel.connect_material_expressions(final_color_lerp, "", final_with_ao, "A")
mel.connect_material_expressions(ao_lerp, "", final_with_ao, "B")

# ============================================================
# SECTION 9: Color Processing (warmth, saturation, brightness, contrast)
# ============================================================

unreal.log("  Creating color processing chain...")

# WarmthTint — color multiply for zone temperature
warmth_tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, 600, 0)
warmth_tint.set_editor_property("parameter_name", "WarmthTint")
warmth_tint.set_editor_property("default_value", unreal.LinearColor(1.0, 0.98, 0.94, 1.0))

color_warmed = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 700, 50)
mel.connect_material_expressions(final_with_ao, "", color_warmed, "A")
mel.connect_material_expressions(warmth_tint, "", color_warmed, "B")

# SaturationMult — desaturate (Niflheim ~0.4, Jawaii ~1.1)
saturation_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 600, 150)
saturation_param.set_editor_property("parameter_name", "SaturationMult")
saturation_param.set_editor_property("default_value", 1.0)

# UE5 Desaturation node: 0 = fully saturated, 1 = grayscale
# We want SaturationMult=1.0 to mean normal, 0.3 to mean very desaturated
# So Desaturation.Fraction = 1.0 - SaturationMult
sat_invert = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 700, 150)
mel.connect_material_expressions(saturation_param, "", sat_invert, "")

color_desat = mel.create_material_expression(mat, unreal.MaterialExpressionDesaturation, 800, 80)
mel.connect_material_expressions(color_warmed, "", color_desat, "")
mel.connect_material_expressions(sat_invert, "", color_desat, "Fraction")

# BrightnessOffset — add/subtract from BaseColor
brightness_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 600, 250)
brightness_param.set_editor_property("parameter_name", "BrightnessOffset")
brightness_param.set_editor_property("default_value", 0.0)

color_bright = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, 900, 100)
mel.connect_material_expressions(color_desat, "", color_bright, "A")
mel.connect_material_expressions(brightness_param, "", color_bright, "B")

# ContrastBoost — power curve on BaseColor (1.0 = normal, 1.4 = punchy)
contrast_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 600, 350)
contrast_param.set_editor_property("parameter_name", "ContrastBoost")
contrast_param.set_editor_property("default_value", 1.0)

color_contrast = mel.create_material_expression(mat, unreal.MaterialExpressionPower, 1000, 120)
mel.connect_material_expressions(color_bright, "", color_contrast, "Base")
mel.connect_material_expressions(contrast_param, "", color_contrast, "Exp")

# Clamp final result to 0-1
color_final = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, 1100, 120)
mel.connect_material_expressions(color_contrast, "", color_final, "")

# ============================================================
# SECTION 10: Material Outputs
# ============================================================

unreal.log("  Connecting material outputs...")

# Base Color (with full color processing)
mel.connect_material_property(color_final, "", unreal.MaterialProperty.MP_BASE_COLOR)

# Normal (XY scaled by strength, Z=1)
mel.connect_material_property(norm_append, "", unreal.MaterialProperty.MP_NORMAL)

# Roughness = 0.95 (fully rough, RO Classic diffuse-only)
roughness_const = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 800, 300)
roughness_const.set_editor_property("parameter_name", "Roughness")
roughness_const.set_editor_property("default_value", 0.95)
mel.connect_material_property(roughness_const, "", unreal.MaterialProperty.MP_ROUGHNESS)

# Metallic = 0.0 (no metallic surfaces in RO)
metallic_const = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 800, 400)
metallic_const.set_editor_property("r", 0.0)
mel.connect_material_property(metallic_const, "", unreal.MaterialProperty.MP_METALLIC)

# ============================================================
# SECTION 11: Paintable Grass Layers (for Landscape Grass Output)
# ============================================================
# These LandscapeLayerWeight nodes output 0-1 based on where you paint
# in Landscape Mode > Paint. Multiply with slope mask for final grass density.
# Connect these outputs to the Grass Output node (added manually).

unreal.log("  Creating paintable grass layer weights...")

# Slope mask for grass (1 on flat, 0 on cliff) — reuse slope_clamp from Section 6
# slope_clamp is already defined above

# GrassDense layer — paint this for thick grass areas
grass_dense_layer = mel.create_material_expression(
    mat, unreal.MaterialExpressionLandscapeLayerWeight, 1200, 500)
grass_dense_layer.set_editor_property("parameter_name", "GrassDense")
grass_dense_layer.set_editor_property("preview_weight", 1.0)

# Connect: constant 1.0 as base, layer weight modulates
grass_dense_base = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 480)
grass_dense_base.set_editor_property("r", 0.0)
grass_dense_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 520)
grass_dense_one.set_editor_property("r", 1.0)
mel.connect_material_expressions(grass_dense_base, "", grass_dense_layer, "Base")
mel.connect_material_expressions(grass_dense_one, "", grass_dense_layer, "Layer")

# Multiply dense layer × slope mask = grass only on painted flat areas
grass_dense_final = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 1350, 500)
mel.connect_material_expressions(grass_dense_layer, "", grass_dense_final, "A")
mel.connect_material_expressions(slope_clamp, "", grass_dense_final, "B")

# FlowerPatch layer — paint for flower clusters
flower_layer = mel.create_material_expression(
    mat, unreal.MaterialExpressionLandscapeLayerWeight, 1200, 600)
flower_layer.set_editor_property("parameter_name", "FlowerPatch")
flower_layer.set_editor_property("preview_weight", 0.0)

flower_base = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 580)
flower_base.set_editor_property("r", 0.0)
flower_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 620)
flower_one.set_editor_property("r", 1.0)
mel.connect_material_expressions(flower_base, "", flower_layer, "Base")
mel.connect_material_expressions(flower_one, "", flower_layer, "Layer")

flower_final = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 1350, 600)
mel.connect_material_expressions(flower_layer, "", flower_final, "A")
mel.connect_material_expressions(slope_clamp, "", flower_final, "B")

# Debris layer — paint for rocks, twigs, leaves
debris_layer = mel.create_material_expression(
    mat, unreal.MaterialExpressionLandscapeLayerWeight, 1200, 700)
debris_layer.set_editor_property("parameter_name", "Debris")
debris_layer.set_editor_property("preview_weight", 0.0)

debris_base = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 680)
debris_base.set_editor_property("r", 0.0)
debris_one = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 720)
debris_one.set_editor_property("r", 1.0)
mel.connect_material_expressions(debris_base, "", debris_layer, "Base")
mel.connect_material_expressions(debris_one, "", debris_layer, "Layer")

debris_final = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 1350, 700)
mel.connect_material_expressions(debris_layer, "", debris_final, "A")
mel.connect_material_expressions(slope_clamp, "", debris_final, "B")

unreal.log("  Paint layers created: GrassDense, FlowerPatch, Debris")
unreal.log("  Connect these to Grass Output node manually:")
unreal.log("    grass_dense_final -> grass varieties")
unreal.log("    flower_final -> flower varieties")
unreal.log("    debris_final -> debris/leaf/rock varieties")

# ============================================================
# SECTION 12: Auto-Layout and Save
# ============================================================

unreal.log("  Laying out and saving...")

# Add a comment node with the description (visible in Material Editor)
try:
    comment = mel.create_material_expression(mat, unreal.MaterialExpressionComment, -2000, -700)
    comment.set_editor_property("text", f"v{MAT_VERSION:02d}: {MAT_DESCRIPTION}")
except Exception as e:
    unreal.log_warning(f"  Comment node: {e} (non-critical, skipping)")

mel.layout_material_expressions(mat)
mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(FULL_PATH)

# Append to tracking document
import os
tracking_path = "C:/Sabri_MMO/docsNew/05_Development/Material_Variant_Tracker.md"
is_new = not os.path.exists(tracking_path)
with open(tracking_path, "a") as f:
    if is_new:
        f.write("# M_Landscape_RO — Material Variant Tracker\n\n")
        f.write("| Version | Name | Description | Slope | Dirt | Normals | Roughness | Date |\n")
        f.write("|---------|------|-------------|-------|------|---------|-----------|------|\n")
    f.write(f"| {MAT_VERSION:02d} | {MAT_NAME} | {MAT_DESCRIPTION} | {SLOPE_POWER} | ")
    f.write(f"{dirt_amount.get_editor_property('default_value'):.2f} | ")
    f.write(f"{normal_strength.get_editor_property('default_value'):.1f} | ")
    f.write(f"{roughness_const.get_editor_property('r'):.2f} | ")
    import datetime
    f.write(f"{datetime.date.today()} |\n")
unreal.log(f"  Tracked in: {tracking_path}")

unreal.log("=" * 60)
unreal.log(f"SUCCESS: {MAT_NAME} created at {FULL_PATH}")
unreal.log("=" * 60)
unreal.log("")
unreal.log("=== 23 PARAMETERS (all MI-overridable) ===")
unreal.log("")
unreal.log("Textures (9):")
unreal.log("  GrassWarmTexture, GrassCoolTexture, DirtTexture, RockTexture")
unreal.log("  GrassWarmNormal, GrassCoolNormal, DirtNormal, RockNormal, GrassAO")
unreal.log("")
unreal.log("Color Processing (4):")
unreal.log("  WarmthTint (Vector, default warm gold) | SaturationMult (0.3-1.2, default 1.0)")
unreal.log("  BrightnessOffset (-0.2 to 0.3, default 0.0) | ContrastBoost (0.8-1.5, default 1.0)")
unreal.log("")
unreal.log("Blending (4):")
unreal.log("  GrassVariantBalance (0-1, default 0.5) | GrassNoiseScale (0.001-0.01, default 0.004)")
unreal.log("  MacroNoiseScale (0.0003-0.003, default 0.001) | DirtOnSlopes (0-1, default 0.0)")
unreal.log("")
unreal.log("Slope (4):")
unreal.log("  SlopeThreshold (default 0.60) | SlopeTransitionWidth (default 0.15)")
unreal.log("  SlopeNoiseAmount (0-0.15, default 0.0) | SlopeNoiseFreq (default 0.01)")
unreal.log("")
unreal.log("Surface (6):")
unreal.log("  DirtAmount (default 0.25) | Roughness (default 0.95) | NormalStrength (default 0.5)")
unreal.log("  AOStrength (default 0.4) | UVDistortStrength (default 60)")
unreal.log("  GrassWarmTileSize, GrassCoolTileSize, DirtTileSize, RockTileSize")
unreal.log("")
unreal.log("Features:")
unreal.log("  - World-space UVs (consistent tiling, no mesh UV dependency)")
unreal.log("  - Noise-driven grass variant splotches")
unreal.log("  - Macro variation (anti-tile-repetition)")
unreal.log("  - Noise-driven dirt patches")
unreal.log("  - Slope-based automatic rock on steep surfaces")
unreal.log("  - Roughness 0.95, Metallic 0.0 (RO Classic diffuse-only)")
unreal.log("")
unreal.log("Next steps:")
unreal.log("  1. Open the material in Material Editor to verify node graph")
unreal.log("  2. Assign textures if not auto-detected")
unreal.log("  3. Create Landscape in level, assign this material")
unreal.log("  4. Create Material Instance per zone for tint/scale variations")
