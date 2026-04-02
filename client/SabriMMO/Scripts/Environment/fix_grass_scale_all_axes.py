# Force ALL axes to 0.005-0.01 on ALL grass types

import unreal

eal = unreal.EditorAssetLibrary
GRASS_PATH = "/Game/SabriMMO/Environment/Grass"

S_MIN = 0.005
S_MAX = 0.01
interval = unreal.FloatInterval(min=S_MIN, max=S_MAX)

for gt_name in ["GT_GrassShort", "GT_GrassTall", "GT_Flowers", "GT_Pebbles"]:
    gt = unreal.load_asset(f"{GRASS_PATH}/{gt_name}")
    if not gt:
        continue

    new_varieties = unreal.Array(unreal.GrassVariety)
    old_varieties = gt.get_editor_property("grass_varieties")

    for v in old_varieties:
        v.set_editor_property("scale_x", unreal.FloatInterval(min=S_MIN, max=S_MAX))
        v.set_editor_property("scale_y", unreal.FloatInterval(min=S_MIN, max=S_MAX))
        v.set_editor_property("scale_z", unreal.FloatInterval(min=S_MIN, max=S_MAX))
        new_varieties.append(v)

    gt.set_editor_property("grass_varieties", new_varieties)
    eal.save_asset(f"{GRASS_PATH}/{gt_name}")
    unreal.log(f"  {gt_name}: all axes set to {S_MIN}-{S_MAX}")

unreal.log("Done. Re-apply landscape material to refresh.")
