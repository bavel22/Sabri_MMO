# Fix GT_GrassShort — add the missing variety with proper mesh

import unreal

eal = unreal.EditorAssetLibrary

gt_path = "/Game/SabriMMO/Environment/Grass/GT_GrassShort"
mesh_path = "/Game/SabriMMO/Environment/Grass/Meshes/SM_GrassBlade_01"

gt = unreal.load_asset(gt_path)
mesh = unreal.load_asset(mesh_path)

if not gt:
    unreal.log_error("GT_GrassShort not found!")
elif not mesh:
    unreal.log_error("SM_GrassBlade_01 not found!")
else:
    variety = unreal.GrassVariety()
    variety.set_editor_property("grass_mesh", mesh)
    variety.set_editor_property("grass_density", unreal.PerPlatformFloat(default=15.0))
    variety.set_editor_property("start_cull_distance", unreal.PerPlatformInt(default=3000))
    variety.set_editor_property("end_cull_distance", unreal.PerPlatformInt(default=5000))
    variety.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
    variety.set_editor_property("scale_x", unreal.FloatInterval(min=0.3, max=0.6))
    variety.set_editor_property("scale_y", unreal.FloatInterval(min=0.3, max=0.6))
    variety.set_editor_property("scale_z", unreal.FloatInterval(min=0.3, max=0.6))
    variety.set_editor_property("random_rotation", True)
    variety.set_editor_property("align_to_surface", True)
    variety.set_editor_property("use_landscape_lightmap", True)

    varieties = gt.get_editor_property("grass_varieties")
    varieties.append(variety)
    gt.set_editor_property("grass_varieties", varieties)
    eal.save_asset(gt_path)
    unreal.log("GT_GrassShort: fixed — SM_GrassBlade_01 assigned, density=15, cull 30-50m")
