# reset_v3_billboard_single.py
# Force-reset for M_V3_S_Grass_Clump_B: wipes the material's expression graph
# entirely, rebuilds the original (texture sample + roughness 0.95), then adds
# v2 billboard WPO. Use this to recover from a partial revert.
#
# Run from UE5 Editor Python console:
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\reset_v3_billboard_single.py").read())
#
# Same expression structure as setup_grass_v3_fixed.py for the texture+roughness
# part, plus the v2 billboard WPO graph on top.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_PATH = "/Game/SabriMMO/Environment/GrassV3/Materials/M_V3_S_Grass_Clump_B"
TEX_PATH = "/Game/SabriMMO/Environment/GrassV3/Textures/S_Grass_Clump_B"

BILLBOARD_HLSL = (
    "float3 ToCam = normalize(InCamPos - InObjPos);\n"
    "float3 WorldUp = float3(0, 0, 1);\n"
    "float3 RightVec = cross(WorldUp, ToCam);\n"
    "float RightLen = length(RightVec);\n"
    "RightVec = (RightLen > 0.001) ? (RightVec / RightLen) : float3(1, 0, 0);\n"
    "float3 UpVec = cross(ToCam, RightVec);\n"
    "float3 NewWorldPos = InObjPos\n"
    "    + InLocalPos.x * ToCam\n"
    "    + InLocalPos.y * RightVec\n"
    "    + InLocalPos.z * UpVec;\n"
    "return NewWorldPos - InVertWorldPos;\n"
)

unreal.log("\n=== Force-reset M_V3_S_Grass_Clump_B + v2 billboard ===")

mat = unreal.load_asset(MAT_PATH)
if not mat or not isinstance(mat, unreal.Material):
    unreal.log_error(f"Material not found: {MAT_PATH}")
    raise RuntimeError("Material load failed")

tex = unreal.load_asset(TEX_PATH)
if not tex or not isinstance(tex, unreal.Texture2D):
    unreal.log_error(f"Texture not found: {TEX_PATH}")
    raise RuntimeError("Texture load failed")

# 1. Wipe the expression graph completely
mel.delete_all_material_expressions(mat)
unreal.log("  Cleared all material expressions")

# 2. Rebuild original sprite material (same as setup_grass_v3_fixed.py)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("opacity_mask_clip_value", 0.33)
mat.set_editor_property("used_with_instanced_static_meshes", True)

tex_sample = mel.create_material_expression(
    mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
tex_sample.set_editor_property("parameter_name", "SpriteTexture")
tex_sample.set_editor_property("texture", tex)

mel.connect_material_property(
    tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(
    tex_sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

rough = mel.create_material_expression(
    mat, unreal.MaterialExpressionConstant, -100, 100)
rough.set_editor_property("r", 0.95)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
unreal.log("  Rebuilt original texture+roughness graph")

# 3. Add v2 billboard WPO
marker = mel.create_material_expression(
    mat, unreal.MaterialExpressionScalarParameter, -1400, 750)
marker.set_editor_property("parameter_name", "_BillboardWPO")
marker.set_editor_property("default_value", 1.0)

cam_pos = mel.create_material_expression(
    mat, unreal.MaterialExpressionCameraPositionWS, -800, 280)
obj_pos = mel.create_material_expression(
    mat, unreal.MaterialExpressionObjectPositionWS, -800, 400)
local_pos = mel.create_material_expression(
    mat, unreal.MaterialExpressionLocalPosition, -800, 520)
world_pos = mel.create_material_expression(
    mat, unreal.MaterialExpressionWorldPosition, -800, 640)

wpo = mel.create_material_expression(
    mat, unreal.MaterialExpressionCustom, -450, 460)
wpo.set_editor_property("description", "BillboardWPO")
wpo.set_editor_property(
    "output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
wpo.set_editor_property("code", BILLBOARD_HLSL)

ci_cam = unreal.CustomInput()
ci_cam.set_editor_property("input_name", "InCamPos")
ci_obj = unreal.CustomInput()
ci_obj.set_editor_property("input_name", "InObjPos")
ci_local = unreal.CustomInput()
ci_local.set_editor_property("input_name", "InLocalPos")
ci_world = unreal.CustomInput()
ci_world.set_editor_property("input_name", "InVertWorldPos")
wpo.set_editor_property("inputs", [ci_cam, ci_obj, ci_local, ci_world])

mel.connect_material_expressions(cam_pos, "", wpo, "InCamPos")
mel.connect_material_expressions(obj_pos, "", wpo, "InObjPos")
mel.connect_material_expressions(local_pos, "", wpo, "InLocalPos")
mel.connect_material_expressions(world_pos, "", wpo, "InVertWorldPos")

mel.connect_material_property(
    wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
unreal.log("  Added v2 billboard WPO graph")

# 4. Recompile and save
mel.recompile_material(mat)
eal.save_asset(mat.get_path_name())
unreal.log("  Recompiled and saved")

unreal.log("\nDone. Walk into a Grass_Clump_B patch and orbit the camera.")
unreal.log("If still grey, check Output Log for 'Material ... failed to compile'.")
