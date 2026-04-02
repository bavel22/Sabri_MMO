# Rebuild ALL 4 grass types from scratch with correct meshes and scales

import unreal

eal = unreal.EditorAssetLibrary

GRASS_PATH = "/Game/SabriMMO/Environment/Grass"
MESH_PATH = "/Game/SabriMMO/Environment/Grass/Meshes"

CONFIGS = {
    "GT_GrassShort": {
        "mesh": f"{MESH_PATH}/SM_GrassBlade_01",
        "density": 15.0,
        "start_cull": 3000,
        "end_cull": 5000,
        "scale_min": 0.03,
        "scale_max": 0.06,
    },
    "GT_GrassTall": {
        "mesh": f"{MESH_PATH}/SM_GrassClump_01",
        "density": 8.0,
        "start_cull": 2500,
        "end_cull": 4000,
        "scale_min": 0.05,
        "scale_max": 0.10,
    },
    "GT_Flowers": {
        "mesh": f"{MESH_PATH}/SM_Flower_01",
        "density": 3.0,
        "start_cull": 2000,
        "end_cull": 3500,
        "scale_min": 0.02,
        "scale_max": 0.04,
    },
    "GT_Pebbles": {
        "mesh": f"{MESH_PATH}/SM_Pebble_01",
        "density": 5.0,
        "start_cull": 1500,
        "end_cull": 3000,
        "scale_min": 0.005,
        "scale_max": 0.015,
    },
}

for gt_name, cfg in CONFIGS.items():
    gt_path = f"{GRASS_PATH}/{gt_name}"
    gt = unreal.load_asset(gt_path)
    if not gt:
        unreal.log(f"  {gt_name}: not found, skipping")
        continue

    mesh = unreal.load_asset(cfg["mesh"])
    if not mesh:
        unreal.log(f"  {gt_name}: mesh not found at {cfg['mesh']}")
        continue

    # Clear existing varieties and rebuild from scratch
    variety = unreal.GrassVariety()
    variety.set_editor_property("grass_mesh", mesh)
    variety.set_editor_property("grass_density", unreal.PerPlatformFloat(default=cfg["density"]))
    variety.set_editor_property("start_cull_distance", unreal.PerPlatformInt(default=cfg["start_cull"]))
    variety.set_editor_property("end_cull_distance", unreal.PerPlatformInt(default=cfg["end_cull"]))
    variety.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
    variety.set_editor_property("scale_x", unreal.FloatInterval(min=cfg["scale_min"], max=cfg["scale_max"]))
    variety.set_editor_property("scale_y", unreal.FloatInterval(min=cfg["scale_min"], max=cfg["scale_max"]))
    variety.set_editor_property("scale_z", unreal.FloatInterval(min=cfg["scale_min"], max=cfg["scale_max"]))
    variety.set_editor_property("random_rotation", True)
    variety.set_editor_property("align_to_surface", True)
    variety.set_editor_property("use_landscape_lightmap", True)

    # Replace the entire array with just our one correct variety
    new_varieties = unreal.Array(unreal.GrassVariety)
    new_varieties.append(variety)
    gt.set_editor_property("grass_varieties", new_varieties)

    eal.save_asset(gt_path)
    unreal.log(f"  {gt_name}: rebuilt — mesh={mesh.get_name()}, scale={cfg['scale_min']}-{cfg['scale_max']}")

unreal.log("\nDone. Re-apply the material to the landscape or nudge it to force refresh.")
