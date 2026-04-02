import unreal

eal = unreal.EditorAssetLibrary
gt_path = "/Game/SabriMMO/Environment/Grass/Types/GT_Amatsu"
gt = unreal.load_asset(gt_path)

if not gt:
    unreal.log_error("GT_Amatsu not found")
else:
    old = gt.get_editor_property("grass_varieties")
    new_varieties = unreal.Array(unreal.GrassVariety)

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
        nv.set_editor_property("scale_x", unreal.FloatInterval(min=50.0, max=1000.0))
        nv.set_editor_property("scale_y", unreal.FloatInterval(min=50.0, max=1000.0))
        nv.set_editor_property("scale_z", unreal.FloatInterval(min=50.0, max=1000.0))
        nv.set_editor_property("random_rotation", True)
        nv.set_editor_property("align_to_surface", True)
        nv.set_editor_property("use_landscape_lightmap", True)
        new_varieties.append(nv)

        mesh_name = mesh.get_name() if mesh else "NONE"
        unreal.log(f"  Rebuilt: {mesh_name} scale 50-1000")

    gt.set_editor_property("grass_varieties", new_varieties)

    # Force dirty + save
    gt.modify()
    eal.save_asset(gt_path)
    unreal.log(f"\nGT_Amatsu: {len(new_varieties)} varieties rebuilt with scale 50-1000")
    unreal.log("Close and reopen GT_Amatsu in editor to see updated values")
