# Fix grass scale — reduce all varieties by 10x

import unreal

eal = unreal.EditorAssetLibrary

GRASS_PATH = "/Game/SabriMMO/Environment/Grass"

SCALE_MAP = {
    "GT_GrassShort": (0.03, 0.06),
    "GT_GrassTall": (0.05, 0.10),
    "GT_Flowers": (0.02, 0.04),
    "GT_Pebbles": (0.005, 0.015),
}

for gt_name, (min_s, max_s) in SCALE_MAP.items():
    gt_path = f"{GRASS_PATH}/{gt_name}"
    gt = unreal.load_asset(gt_path)
    if not gt:
        continue

    varieties = gt.get_editor_property("grass_varieties")
    for v in varieties:
        v.set_editor_property("scale_x", unreal.FloatInterval(min=min_s, max=max_s))
        v.set_editor_property("scale_y", unreal.FloatInterval(min=min_s, max=max_s))
        v.set_editor_property("scale_z", unreal.FloatInterval(min=min_s, max=max_s))

    gt.set_editor_property("grass_varieties", varieties)
    eal.save_asset(gt_path)
    unreal.log(f"  {gt_name}: scale {min_s}-{max_s}")

unreal.log("Done — Build > Build Grass Maps to update")
