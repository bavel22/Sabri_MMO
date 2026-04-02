import unreal

eal = unreal.EditorAssetLibrary

gt_path = "/Game/SabriMMO/Environment/GrassV2/Types/GT_V2_Test"
gt = unreal.load_asset(gt_path)

if not gt:
    unreal.log_error("GT_V2_Test not found")
else:
    old = gt.get_editor_property("grass_varieties")
    new_v = unreal.Array(unreal.GrassVariety)

    # These billboard meshes are ~30 UU tall from Blender export (100x scale)
    # Scale 0.5-2.0 = 15-60 UU in game = reasonable grass size
    SCALE_MIN = 0.5
    SCALE_MAX = 2.0

    for v in old:
        mesh = v.get_editor_property("grass_mesh")
        density = v.get_editor_property("grass_density")
        cull_start = v.get_editor_property("start_cull_distance")
        cull_end = v.get_editor_property("end_cull_distance")

        nv = unreal.GrassVariety()
        nv.set_editor_property("grass_mesh", mesh)
        nv.set_editor_property("grass_density", density)
        nv.set_editor_property("start_cull_distance", cull_start)
        nv.set_editor_property("end_cull_distance", cull_end)
        nv.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
        nv.set_editor_property("scale_x", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
        nv.set_editor_property("scale_y", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
        nv.set_editor_property("scale_z", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
        nv.set_editor_property("random_rotation", True)
        nv.set_editor_property("align_to_surface", True)
        nv.set_editor_property("use_landscape_lightmap", True)
        new_v.append(nv)

        mesh_name = mesh.get_name() if mesh else "NONE"
        unreal.log(f"  {mesh_name}: scale {SCALE_MIN}-{SCALE_MAX}")

    gt.set_editor_property("grass_varieties", new_v)
    gt.modify()
    eal.save_asset(gt_path)
    unreal.log(f"\nFixed {len(new_v)} varieties. Re-apply landscape material to refresh.")
