# Set ALL GrassType varieties to Uniform scaling, 10-1000 range

import unreal

eal = unreal.EditorAssetLibrary

GT_PATHS = [
    "/Game/SabriMMO/Environment/Grass",
    "/Game/SabriMMO/Environment/Grass/Types",
]

fixed = 0
for gt_dir in GT_PATHS:
    assets = eal.list_assets(gt_dir, recursive=False)
    for asset in sorted(assets):
        clean = asset.split(".")[0]
        gt = unreal.load_asset(clean)
        if not gt or not isinstance(gt, unreal.LandscapeGrassType):
            continue

        name = clean.split("/")[-1]
        varieties = gt.get_editor_property("grass_varieties")

        for v in varieties:
            v.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
            v.set_editor_property("scale_x", unreal.FloatInterval(min=10.0, max=1000.0))
            v.set_editor_property("scale_y", unreal.FloatInterval(min=10.0, max=1000.0))
            v.set_editor_property("scale_z", unreal.FloatInterval(min=10.0, max=1000.0))

        gt.set_editor_property("grass_varieties", varieties)
        eal.save_asset(clean)
        fixed += 1
        unreal.log(f"  {name}: {len(varieties)} varieties -> Uniform 10-1000")

unreal.log(f"\nDone: {fixed} GrassTypes updated. Re-apply landscape material to refresh.")
