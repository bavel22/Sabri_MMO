# setup_decals.py
# Creates DBuffer decal materials and spawns decal actors on the terrain.
#
# DBuffer decals project textures (dirt, cracks, moss, paths) onto terrain
# without modifying the landscape material. Each decal is a separate actor.
#
# Prerequisites:
#   - DBuffer must be enabled: Project Settings > Rendering > DBuffer Decals = true
#     (If using Lumen/VSM, DBuffer is already active)
#
# Run: exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\setup_decals.py").read())

import unreal
import random
import math

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

DECAL_MAT_PATH = "/Game/SabriMMO/Materials/Environment/Decals"
eal.make_directory(DECAL_MAT_PATH)

# ============================================================
# STEP 1: Create decal materials
# ============================================================
# Each decal material is a Deferred Decal with:
#   - Base Color from texture
#   - Opacity from texture alpha (or derived from luminance)
#   - Configurable tint and opacity strength

def create_decal_material(name, default_tint=(0.3, 0.25, 0.2), default_opacity=0.6):
    """Create a reusable decal material with texture, tint, and opacity parameters."""
    full_path = f"{DECAL_MAT_PATH}/{name}"

    if eal.does_asset_exist(full_path):
        mat = unreal.load_asset(full_path)
        if mat:
            mel.delete_all_material_expressions(mat)
            unreal.log(f"  {name}: rebuilding")
        else:
            return full_path
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        mat = asset_tools.create_asset(name, DECAL_MAT_PATH,
            unreal.Material, unreal.MaterialFactoryNew())

    if not mat:
        unreal.log_warning(f"  {name}: failed to create")
        return None

    # Decal material settings
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    # Texture parameter
    tex_node = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
    tex_node.set_editor_property("parameter_name", "DecalTexture")

    # Tint color parameter
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
    tint.set_editor_property("parameter_name", "DecalTint")
    tint.set_editor_property("default_value", unreal.LinearColor(default_tint[0], default_tint[1], default_tint[2], 1.0))

    # Multiply texture by tint
    color_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, -100)
    mel.connect_material_expressions(tex_node, "RGB", color_mult, "A")
    mel.connect_material_expressions(tint, "", color_mult, "B")

    # Opacity strength parameter
    opacity_str = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
    opacity_str.set_editor_property("parameter_name", "OpacityStrength")
    opacity_str.set_editor_property("default_value", default_opacity)

    # === SOFT RADIAL FALLOFF ===
    # Creates a soft-edged circle that fades the decal edges to zero.
    # Uses RadialGradientExponential centered on the decal's UV space.
    # This eliminates the hard rectangular edges.

    # Build radial gradient from math: 1 - saturate(distance(UV, 0.5) * 2)
    # This gives 1 at center, 0 at edges, circular shape

    tex_coord_grad = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -500, 250)

    uv_center = mel.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -500, 300)
    uv_center.set_editor_property("r", 0.5)
    uv_center.set_editor_property("g", 0.5)

    # Distance from center
    uv_sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -350, 270)
    mel.connect_material_expressions(tex_coord_grad, "", uv_sub, "A")
    mel.connect_material_expressions(uv_center, "", uv_sub, "B")

    # Length of offset vector = distance from center (0 at center, 0.707 at corners)
    uv_dot = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -200, 270)
    mel.connect_material_expressions(uv_sub, "", uv_dot, "A")
    mel.connect_material_expressions(uv_sub, "", uv_dot, "B")

    # sqrt(dot) = true distance, but dot itself works fine for a radial shape
    # Scale so edges reach 1.0: multiply by ~3 (since max distance is 0.707)
    dist_scale = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, 270)
    dist_scale_const = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 310)
    dist_scale_const.set_editor_property("r", 4.0)
    mel.connect_material_expressions(uv_dot, "", dist_scale, "A")
    mel.connect_material_expressions(dist_scale_const, "", dist_scale, "B")

    # Invert: 1 - distance (1 at center, 0 at edge)
    dist_invert = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 0, 270)
    mel.connect_material_expressions(dist_scale, "", dist_invert, "")

    # Clamp to 0-1
    dist_clamp = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, 50, 270)
    mel.connect_material_expressions(dist_invert, "", dist_clamp, "")

    # Edge softness — power controls falloff curve
    edge_softness = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -100, 330)
    edge_softness.set_editor_property("parameter_name", "EdgeSoftness")
    edge_softness.set_editor_property("default_value", 1.5)

    soft_edge = mel.create_material_expression(mat, unreal.MaterialExpressionPower, 100, 290)
    mel.connect_material_expressions(dist_clamp, "", soft_edge, "Base")
    mel.connect_material_expressions(edge_softness, "", soft_edge, "Exp")

    # === NOISE EDGE DISTORTION ===
    # Add noise to the edge mask so it's organic/blobby, not a perfect circle

    # Sample noise at texture UVs scaled up (higher = more jagged edge)
    edge_noise_scale = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 400)
    edge_noise_scale.set_editor_property("parameter_name", "EdgeNoiseScale")
    edge_noise_scale.set_editor_property("default_value", 5.0)

    tex_coord = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -400, 450)

    edge_noise_uv = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 430)
    mel.connect_material_expressions(tex_coord, "", edge_noise_uv, "A")
    mel.connect_material_expressions(edge_noise_scale, "", edge_noise_uv, "B")

    edge_noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -100, 430)
    edge_noise.set_editor_property("scale", 1.0)
    edge_noise.set_editor_property("levels", 3)
    edge_noise.set_editor_property("output_min", 0.5)
    edge_noise.set_editor_property("output_max", 1.0)
    mel.connect_material_expressions(edge_noise_uv, "", edge_noise, "Position")

    # Multiply radial gradient by noise — creates organic blobby shape
    organic_mask = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 50, 300)
    mel.connect_material_expressions(soft_edge, "", organic_mask, "A")
    mel.connect_material_expressions(edge_noise, "", organic_mask, "B")

    # === FINAL OPACITY ===
    # Combine organic mask with texture luminance and opacity strength

    # Use texture luminance for variation within the decal
    desat = mel.create_material_expression(mat, unreal.MaterialExpressionDesaturation, -200, 100)
    mel.connect_material_expressions(tex_node, "RGB", desat, "")
    desat_full = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 150)
    desat_full.set_editor_property("r", 1.0)
    mel.connect_material_expressions(desat_full, "", desat, "Fraction")

    # Invert luminance (dark areas = more opaque)
    invert = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -100, 100)
    mel.connect_material_expressions(desat, "", invert, "")

    # Combine: organic mask * texture variation * opacity strength
    opacity_tex = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 100, 150)
    mel.connect_material_expressions(organic_mask, "", opacity_tex, "A")
    mel.connect_material_expressions(invert, "", opacity_tex, "B")

    opacity_final = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 200, 180)
    mel.connect_material_expressions(opacity_tex, "", opacity_final, "A")
    mel.connect_material_expressions(opacity_str, "", opacity_final, "B")

    # Connect outputs
    mel.connect_material_property(color_mult, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(opacity_final, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    eal.save_asset(full_path)
    unreal.log(f"  {name}: created")
    return full_path


unreal.log("=== Creating Decal Materials ===\n")

# Dirt decal — warm brown, for worn patches on grass
dirt_mat = create_decal_material("M_Decal_Dirt",
    default_tint=(0.35, 0.28, 0.18), default_opacity=0.5)

# Crack decal — dark gray, for rocky area detail
crack_mat = create_decal_material("M_Decal_Cracks",
    default_tint=(0.2, 0.2, 0.2), default_opacity=0.4)

# Moss decal — dark green, for forest floor variety
moss_mat = create_decal_material("M_Decal_Moss",
    default_tint=(0.15, 0.25, 0.12), default_opacity=0.45)

# Path decal — sandy brown, for worn footpaths
path_mat = create_decal_material("M_Decal_Path",
    default_tint=(0.4, 0.35, 0.25), default_opacity=0.55)

# Dark stain decal — for dungeon/cursed areas
stain_mat = create_decal_material("M_Decal_DarkStain",
    default_tint=(0.1, 0.08, 0.12), default_opacity=0.5)

# ============================================================
# STEP 2: Create decal Material Instances with RO original textures
# ============================================================
# Use some of the 608 original RO textures as decal sources

unreal.log("\n=== Creating Decal Material Instances ===\n")

MI_PATH = f"{DECAL_MAT_PATH}/Instances"
eal.make_directory(MI_PATH)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()

# Map decal types to parent materials and good RO texture candidates
DECAL_CONFIGS = [
    # (MI name, parent material, RO texture number, tint RGB, opacity)
    ("MI_Decal_Dirt_01", dirt_mat, "010", (0.35, 0.28, 0.18), 0.45),
    ("MI_Decal_Dirt_02", dirt_mat, "025", (0.40, 0.32, 0.22), 0.50),
    ("MI_Decal_Dirt_03", dirt_mat, "042", (0.30, 0.25, 0.16), 0.40),
    ("MI_Decal_Dirt_04", dirt_mat, "088", (0.38, 0.30, 0.20), 0.55),
    ("MI_Decal_Dirt_05", dirt_mat, "150", (0.32, 0.26, 0.18), 0.45),
    ("MI_Decal_Crack_01", crack_mat, "055", (0.22, 0.20, 0.20), 0.40),
    ("MI_Decal_Crack_02", crack_mat, "100", (0.18, 0.18, 0.20), 0.35),
    ("MI_Decal_Crack_03", crack_mat, "120", (0.25, 0.22, 0.22), 0.45),
    ("MI_Decal_Moss_01", moss_mat, "005", (0.15, 0.28, 0.12), 0.40),
    ("MI_Decal_Moss_02", moss_mat, "030", (0.12, 0.22, 0.10), 0.45),
    ("MI_Decal_Moss_03", moss_mat, "045", (0.18, 0.30, 0.14), 0.35),
    ("MI_Decal_Path_01", path_mat, "015", (0.42, 0.36, 0.26), 0.50),
    ("MI_Decal_Path_02", path_mat, "060", (0.38, 0.32, 0.22), 0.55),
    ("MI_Decal_Stain_01", stain_mat, "200", (0.10, 0.08, 0.14), 0.45),
    ("MI_Decal_Stain_02", stain_mat, "250", (0.12, 0.10, 0.16), 0.50),
]

decal_instances = []
for mi_name, parent_path, tex_num, tint, opacity in DECAL_CONFIGS:
    mi_full = f"{MI_PATH}/{mi_name}"
    if eal.does_asset_exist(mi_full):
        decal_instances.append(mi_full)
        continue

    if not parent_path:
        continue

    parent = unreal.load_asset(parent_path)
    if not parent:
        continue

    mi = asset_tools.create_asset(mi_name, MI_PATH,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        continue

    mi.set_editor_property("parent", parent)

    # Try to assign RO original texture
    tex_path = f"/Game/SabriMMO/Textures/Environment/RO_Original/{tex_num}"
    if eal.does_asset_exist(tex_path):
        tex = unreal.load_asset(tex_path)
        if tex:
            mel.set_material_instance_texture_parameter_value(
                mi, "DecalTexture", tex,
                unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Set tint and opacity
    mel.set_material_instance_vector_parameter_value(
        mi, "DecalTint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0),
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
    mel.set_material_instance_scalar_parameter_value(
        mi, "OpacityStrength", opacity,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    decal_instances.append(mi_full)
    unreal.log(f"  {mi_name}: created (tex={tex_num}, opacity={opacity})")

# ============================================================
# STEP 3: Spawn decal actors on the terrain
# ============================================================

unreal.log("\n=== Spawning Decal Actors ===\n")
unreal.log("  Placing decals in a scatter pattern around the landscape...")

# Get the world
world = unreal.EditorLevelLibrary.get_editor_world()

# Scatter settings
CENTER_X = 0.0
CENTER_Y = 0.0
GROUND_Z = 200.0  # approximate ground height — decals project downward
SCATTER_RADIUS = 3000.0  # scatter within this radius from center
NUM_DECALS = 40  # how many to place

spawned = 0
random.seed(42)  # deterministic for reproducibility

for i in range(NUM_DECALS):
    # Random position within scatter radius
    angle = random.uniform(0, 2 * math.pi)
    dist = random.uniform(200, SCATTER_RADIUS)
    x = CENTER_X + math.cos(angle) * dist
    y = CENTER_Y + math.sin(angle) * dist
    z = GROUND_Z + 50  # slightly above ground, projects down

    # Random rotation (yaw only — decals project downward)
    yaw = random.uniform(0, 360)

    # Random scale
    scale = random.uniform(1.5, 4.0)

    # Pick a random decal material
    if not decal_instances:
        break
    mat_path = random.choice(decal_instances)

    # Spawn DecalActor
    actor_name = f"TerrainDecal_{i:03d}"

    # Use spawn_actor to create a DecalActor
    location = unreal.Vector(x, y, z)
    rotation = unreal.Rotator(-90.0, yaw, 0.0)  # -90 pitch = project straight down

    decal_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DecalActor, location, rotation)

    if decal_actor:
        decal_actor.set_actor_label(actor_name)
        decal_actor.set_actor_scale3d(unreal.Vector(scale, scale * 1.5, scale * 1.5))

        # Get decal component and set material
        decal_comp = decal_actor.get_component_by_class(unreal.DecalComponent)
        if decal_comp and eal.does_asset_exist(mat_path):
            mat = unreal.load_asset(mat_path)
            if mat:
                decal_comp.set_decal_material(mat)

        spawned += 1

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE:")
unreal.log(f"  Decal materials: 5 parent + {len(decal_instances)} instances")
unreal.log(f"  Decal actors spawned: {spawned}")
unreal.log(f"  Location: scattered around ({CENTER_X}, {CENTER_Y})")
unreal.log(f"")
unreal.log(f"  To adjust:")
unreal.log(f"    - Select any TerrainDecal actor in the level")
unreal.log(f"    - Move/rotate/scale to position")
unreal.log(f"    - Change material in Details panel")
unreal.log(f"    - Adjust OpacityStrength in material instance")
unreal.log(f"")
unreal.log(f"  To add more: duplicate existing decals (Ctrl+D)")
unreal.log(f"  To delete: select and press Delete")
unreal.log(f"{'='*60}")
