# Fix ALL scatter GrassType varieties to use 0.005-0.01 scale range
# matching the working GT_GrassShort size.

import unreal

eal = unreal.EditorAssetLibrary

SCALE_MIN = 0.005
SCALE_MAX = 0.01

# All GrassType locations
GT_PATHS = [
    "/Game/SabriMMO/Environment/Grass",       # original 4 (GT_GrassShort etc)
    "/Game/SabriMMO/Environment/Grass/Types",  # zone-specific 13
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
            v.set_editor_property("scale_x", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
            v.set_editor_property("scale_y", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))
            v.set_editor_property("scale_z", unreal.FloatInterval(min=SCALE_MIN, max=SCALE_MAX))

        gt.set_editor_property("grass_varieties", varieties)
        eal.save_asset(clean)
        fixed += 1
        unreal.log(f"  {name}: {len(varieties)} varieties -> scale {SCALE_MIN}-{SCALE_MAX}")

unreal.log(f"\nDone: {fixed} GrassTypes updated. Re-apply landscape material to refresh.")
