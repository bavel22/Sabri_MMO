# revert_v3_grass_clump_b.py
# Wipes M_V3_S_Grass_Clump_B back to its original state — texture sample +
# roughness constant only, no billboard, no leftover marker. Same structure
# setup_grass_v3_fixed.py would create on a fresh run.
#
# Run from UE5 Editor Python console:
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\revert_v3_grass_clump_b.py").read())

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_PATH = "/Game/SabriMMO/Environment/GrassV3/Materials/M_V3_S_Grass_Clump_B"
TEX_PATH = "/Game/SabriMMO/Environment/GrassV3/Textures/S_Grass_Clump_B"

unreal.log("\n=== Revert M_V3_S_Grass_Clump_B to original ===")

mat = unreal.load_asset(MAT_PATH)
if not mat or not isinstance(mat, unreal.Material):
    unreal.log_error(f"Material not found: {MAT_PATH}")
    raise RuntimeError("Material load failed")

tex = unreal.load_asset(TEX_PATH)
if not tex or not isinstance(tex, unreal.Texture2D):
    unreal.log_error(f"Texture not found: {TEX_PATH}")
    raise RuntimeError("Texture load failed")

# Wipe everything
mel.delete_all_material_expressions(mat)
unreal.log("  Cleared all material expressions")

# Restore original material settings
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("opacity_mask_clip_value", 0.33)

# Texture sample → BaseColor + OpacityMask
tex_sample = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
tex_sample.set_editor_property("parameter_name", "SpriteTexture")
tex_sample.set_editor_property("texture", tex)

mel.connect_material_property(
    tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(
    tex_sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

# Roughness 0.95
rough = mel.create_material_expression(
    mat, unreal.MaterialExpressionConstant, -100, 100)
rough.set_editor_property("r", 0.95)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
eal.save_asset(mat.get_path_name())

unreal.log("  Rebuilt original texture + roughness")
unreal.log("  Recompiled and saved")
unreal.log("\nDone. Material is back to its original state.")
