# Check all 4 original grass types — report mesh, density, scale, cull

import unreal

eal = unreal.EditorAssetLibrary
GRASS_PATH = "/Game/SabriMMO/Environment/Grass"

for gt_name in ["GT_GrassShort", "GT_GrassTall", "GT_Flowers", "GT_Pebbles"]:
    gt_path = f"{GRASS_PATH}/{gt_name}"
    gt = unreal.load_asset(gt_path)
    if not gt:
        unreal.log(f"{gt_name}: NOT FOUND")
        continue

    varieties = gt.get_editor_property("grass_varieties")
    unreal.log(f"\n{gt_name}: {len(varieties)} varieties")

    for i, v in enumerate(varieties):
        mesh = v.get_editor_property("grass_mesh")
        mesh_name = mesh.get_name() if mesh else "NONE"
        mesh_type = type(mesh).__name__ if mesh else "N/A"

        density = v.get_editor_property("grass_density")
        sx = v.get_editor_property("scale_x")
        sy = v.get_editor_property("scale_y")
        sz = v.get_editor_property("scale_z")
        cull_start = v.get_editor_property("start_cull_distance")
        cull_end = v.get_editor_property("end_cull_distance")

        unreal.log(f"  [{i}] mesh={mesh_name} ({mesh_type})")
        unreal.log(f"      density={density}")
        unreal.log(f"      scale=({sx}, {sy}, {sz})")
        unreal.log(f"      cull=({cull_start} - {cull_end})")

# Also check what the landscape material is
unreal.log(f"\n=== Checking M_Landscape_RO_15 ===")
mat_path = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_15"
if eal.does_asset_exist(mat_path):
    unreal.log(f"  M_Landscape_RO_15: EXISTS")
else:
    unreal.log(f"  M_Landscape_RO_15: NOT FOUND")
