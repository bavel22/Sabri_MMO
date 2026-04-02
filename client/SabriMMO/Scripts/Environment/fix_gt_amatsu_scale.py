import unreal

eal = unreal.EditorAssetLibrary
gt_path = "/Game/SabriMMO/Environment/Grass/Types/GT_Amatsu"
gt = unreal.load_asset(gt_path)

if gt:
    varieties = gt.get_editor_property("grass_varieties")
    for v in varieties:
        v.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
        v.set_editor_property("scale_x", unreal.FloatInterval(min=50.0, max=1000.0))
    gt.set_editor_property("grass_varieties", varieties)
    eal.save_asset(gt_path)
    unreal.log(f"GT_Amatsu: {len(varieties)} varieties -> Uniform, Scale X min=50 max=1000")
else:
    unreal.log_error("GT_Amatsu not found")
