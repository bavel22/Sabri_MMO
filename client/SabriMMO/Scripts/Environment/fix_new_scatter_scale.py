# The new scatter meshes (from blender_make_all_scatter.py) are ~10,000x smaller
# than the original 6 (from blender_make_grass_meshes.py).
# Original meshes: bounds ~800 UU -> scale 0.005-0.01 works
# New meshes: bounds ~0.07 UU -> need scale 50-100 to match

import unreal

eal = unreal.EditorAssetLibrary
GT_PATH = "/Game/SabriMMO/Environment/Grass/Types"

# These are the HUGE original meshes that work at 0.005-0.01 scale
BIG_MESHES = {"SM_GrassBlade_01", "SM_GrassBlade_02", "SM_GrassBlade_03",
              "SM_GrassClump_01", "SM_Flower_01", "SM_Pebble_01"}

# Scale for new tiny meshes (to match visual size of originals at 0.005-0.01)
NEW_SCALE_MIN = 50.0
NEW_SCALE_MAX = 100.0

assets = eal.list_assets(GT_PATH, recursive=False)
fixed = 0

for asset in sorted(assets):
    clean = asset.split(".")[0]
    gt = unreal.load_asset(clean)
    if not gt or not isinstance(gt, unreal.LandscapeGrassType):
        continue

    gt_name = clean.split("/")[-1]
    varieties = gt.get_editor_property("grass_varieties")
    changed = False

    for v in varieties:
        mesh = v.get_editor_property("grass_mesh")
        if not mesh:
            continue

        mesh_name = mesh.get_name()

        if mesh_name in BIG_MESHES:
            # Original big meshes — keep small scale
            v.set_editor_property("scale_x", unreal.FloatInterval(min=0.005, max=0.01))
            v.set_editor_property("scale_y", unreal.FloatInterval(min=0.005, max=0.01))
            v.set_editor_property("scale_z", unreal.FloatInterval(min=0.005, max=0.01))
        else:
            # New tiny meshes — need much larger scale
            v.set_editor_property("scale_x", unreal.FloatInterval(min=NEW_SCALE_MIN, max=NEW_SCALE_MAX))
            v.set_editor_property("scale_y", unreal.FloatInterval(min=NEW_SCALE_MIN, max=NEW_SCALE_MAX))
            v.set_editor_property("scale_z", unreal.FloatInterval(min=NEW_SCALE_MIN, max=NEW_SCALE_MAX))
        changed = True

    if changed:
        gt.set_editor_property("grass_varieties", varieties)
        eal.save_asset(clean)
        fixed += 1
        unreal.log(f"  {gt_name}: scales fixed ({len(varieties)} varieties)")

unreal.log(f"\nFixed {fixed} GrassTypes. Re-apply landscape material to refresh.")
