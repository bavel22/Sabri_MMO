import unreal

eal = unreal.EditorAssetLibrary

GT_DIRS = [
    "/Game/SabriMMO/Environment/Grass",
    "/Game/SabriMMO/Environment/Grass/Types",
]

SCALE_MIN = 50.0
SCALE_MAX = 1000.0

fixed = 0
for gt_dir in GT_DIRS:
    assets = eal.list_assets(gt_dir, recursive=False)
    for asset in sorted(assets):
        clean = asset.split(".")[0]
        gt = unreal.load_asset(clean)
        if not gt or not isinstance(gt, unreal.LandscapeGrassType):
            continue

        name = clean.split("/")[-1]
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
            nv.set_editor_property("scale_x", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
            nv.set_editor_property("scale_y", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
            nv.set_editor_property("scale_z", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
            nv.set_editor_property("random_rotation", True)
            nv.set_editor_property("align_to_surface", True)
            nv.set_editor_property("use_landscape_lightmap", True)
            new_varieties.append(nv)

        gt.set_editor_property("grass_varieties", new_varieties)
        gt.modify()
        eal.save_asset(clean)
        fixed += 1
        unreal.log(f"  {name}: {len(new_varieties)} varieties rebuilt, scale {SCALE_MIN}-{SCALE_MAX}")

unreal.log(f"\nDone: {fixed} GrassTypes rebuilt. Close and reopen any open GT editors.")
