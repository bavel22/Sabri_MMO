# setup_landscape_grass.py
# Sets up the Landscape Grass Type system for Sabri_MMO.
#
# Creates:
#   1. LandscapeGrassType assets (grass, flowers, pebbles)
#   2. Adds Grass Output node to M_Landscape_RO_14
#
# Requires:
#   - A Landscape Actor in the level (not static mesh)
#   - Grass blade static meshes (uses engine plane as placeholder)
#
# Run: exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\setup_landscape_grass.py").read())

import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

GRASS_PATH = "/Game/SabriMMO/Environment/Grass"
eal.make_directory(GRASS_PATH)

# ============================================================
# STEP 1: Create LandscapeGrassType assets
# ============================================================

unreal.log("=== Creating Landscape Grass Types ===\n")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

def create_grass_type(name, mesh_path, density, start_cull, end_cull,
                       min_scale=0.8, max_scale=1.2, random_rotation=True,
                       align_to_surface=True):
    """Create a LandscapeGrassType asset with one grass variety."""
    full_path = f"{GRASS_PATH}/{name}"

    if eal.does_asset_exist(full_path):
        unreal.log(f"  {name}: already exists")
        return full_path

    # Create the grass type asset
    grass_type = asset_tools.create_asset(
        name, GRASS_PATH,
        unreal.LandscapeGrassType,
        unreal.LandscapeGrassTypeFactory()
    )

    if not grass_type:
        unreal.log_warning(f"  {name}: failed to create")
        return None

    # Load the mesh
    mesh = None
    if eal.does_asset_exist(mesh_path):
        mesh = unreal.load_asset(mesh_path)

    # Configure grass varieties
    # LandscapeGrassType has a GrassVarieties array
    varieties = grass_type.get_editor_property("grass_varieties")

    # Create a new variety
    variety = unreal.GrassVariety()

    if mesh:
        variety.set_editor_property("grass_mesh", mesh)

    variety.set_editor_property("grass_density", unreal.PerPlatformFloat(default=density))
    variety.set_editor_property("start_cull_distance", unreal.PerPlatformInt(default=int(start_cull)))
    variety.set_editor_property("end_cull_distance", unreal.PerPlatformInt(default=int(end_cull)))
    variety.set_editor_property("min_lod", -1)
    variety.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
    variety.set_editor_property("scale_x", unreal.FloatInterval(min=min_scale, max=max_scale))
    variety.set_editor_property("scale_y", unreal.FloatInterval(min=min_scale, max=max_scale))
    variety.set_editor_property("scale_z", unreal.FloatInterval(min=min_scale, max=max_scale))
    variety.set_editor_property("random_rotation", random_rotation)
    variety.set_editor_property("align_to_surface", align_to_surface)
    variety.set_editor_property("use_landscape_lightmap", True)

    varieties.append(variety)
    grass_type.set_editor_property("grass_varieties", varieties)

    eal.save_asset(full_path)
    unreal.log(f"  {name}: created (density={density}, cull={start_cull}-{end_cull})")
    return full_path


# Engine placeholder meshes (replace with proper grass models later)
PLANE_MESH = "/Engine/BasicShapes/Plane.Plane"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

# Create grass types with aggressive cull distances for MMO performance
# Density is per 10m^2 area

grass_short = create_grass_type(
    "GT_GrassShort",
    PLANE_MESH,
    density=15.0,         # 15 instances per 10m^2
    start_cull=3000,      # start fading at 30m
    end_cull=5000,        # gone at 50m
    min_scale=0.3,
    max_scale=0.6,
)

grass_tall = create_grass_type(
    "GT_GrassTall",
    PLANE_MESH,
    density=8.0,
    start_cull=2500,
    end_cull=4000,
    min_scale=0.5,
    max_scale=1.0,
)

flowers = create_grass_type(
    "GT_Flowers",
    PLANE_MESH,
    density=3.0,          # sparse
    start_cull=2000,
    end_cull=3500,
    min_scale=0.2,
    max_scale=0.4,
)

pebbles = create_grass_type(
    "GT_Pebbles",
    CUBE_MESH,
    density=5.0,
    start_cull=1500,
    end_cull=3000,
    min_scale=0.05,       # tiny
    max_scale=0.15,
    random_rotation=True,
    align_to_surface=True,
)

# ============================================================
# STEP 2: Info about connecting to landscape material
# ============================================================

unreal.log(f"\n{'='*60}")
unreal.log(f"  Grass Types created at {GRASS_PATH}")
unreal.log(f"")
unreal.log(f"  GT_GrassShort  — 15/10m^2, cull 30-50m, small")
unreal.log(f"  GT_GrassTall   — 8/10m^2, cull 25-40m, medium")
unreal.log(f"  GT_Flowers     — 3/10m^2, cull 20-35m, sparse")
unreal.log(f"  GT_Pebbles     — 5/10m^2, cull 15-30m, tiny cubes")
unreal.log(f"")
unreal.log(f"  NOTE: These use engine placeholder meshes (Plane/Cube).")
unreal.log(f"  Replace with proper grass blade models for final look.")
unreal.log(f"")
unreal.log(f"  TO CONNECT TO LANDSCAPE:")
unreal.log(f"  1. Open M_Landscape_RO_14 in Material Editor")
unreal.log(f"  2. Add a 'Landscape Grass Output' node")
unreal.log(f"  3. In node details, add 4 entries to 'Grass Types' array")
unreal.log(f"  4. Assign GT_GrassShort, GT_GrassTall, GT_Flowers, GT_Pebbles")
unreal.log(f"  5. Connect the grass mask (e.g., inverted slope mask) to each input")
unreal.log(f"  6. Apply material to Landscape — grass auto-spawns")
unreal.log(f"")
unreal.log(f"  The Grass Output node cannot be fully created via Python")
unreal.log(f"  because LandscapeGrassOutput requires array property setup")
unreal.log(f"  that is editor-only. The manual step takes ~2 minutes.")
unreal.log(f"{'='*60}")
