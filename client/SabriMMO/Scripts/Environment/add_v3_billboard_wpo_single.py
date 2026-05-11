# add_v3_billboard_wpo_single.py
# Single-material test of billboard WPO. Run this first on M_V3_S_Grass_Clump_B
# before applying to all 60 materials.
#
# v2: uses CameraPositionWS material node instead of ResolvedView.WorldCameraOrigin
# inside HLSL — avoids Large World Coordinates breakage in UE5.7.
#
# Run from UE5 Editor Python console:
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\add_v3_billboard_wpo_single.py").read())
#
# To revert: open the material, delete BillboardWPO Custom node + its 4 input
# nodes + the _BillboardWPO scalar parameter, disconnect WorldPositionOffset,
# Apply + Save.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

TARGET = "/Game/SabriMMO/Environment/GrassV3/Materials/M_V3_S_Grass_Clump_B"
MARKER_PARAM = "_BillboardWPO"

# All-LWC-safe HLSL: every world-space value comes in via input pins.
# Mesh local convention: +X depth (0 for flat plane), +Y lateral, +Z vertical.
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

unreal.log(f"\n=== Billboard WPO single-material test (v2) ===")
unreal.log(f"Target: {TARGET}")

mat = unreal.load_asset(TARGET)
if not mat or not isinstance(mat, unreal.Material):
    unreal.log_error(f"  Material not found or wrong type")
    raise RuntimeError("Material load failed")

name = mat.get_name()

# Idempotency check
scalar_params = mel.get_scalar_parameter_names(mat)
if MARKER_PARAM in scalar_params:
    unreal.log(f"  Already has billboard WPO — nothing to do")
else:
    # Marker scalar parameter (idempotency tag)
    marker = mel.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, -1400, 750)
    marker.set_editor_property("parameter_name", MARKER_PARAM)
    marker.set_editor_property("default_value", 1.0)

    # Input nodes — all return absolute world space (LWC-aware)
    cam_pos = mel.create_material_expression(
        mat, unreal.MaterialExpressionCameraPositionWS, -800, 280)
    obj_pos = mel.create_material_expression(
        mat, unreal.MaterialExpressionObjectPositionWS, -800, 400)
    local_pos = mel.create_material_expression(
        mat, unreal.MaterialExpressionLocalPosition, -800, 520)
    world_pos = mel.create_material_expression(
        mat, unreal.MaterialExpressionWorldPosition, -800, 640)

    # Custom HLSL WPO
    wpo = mel.create_material_expression(
        mat, unreal.MaterialExpressionCustom, -450, 460)
    wpo.set_editor_property("description", "BillboardWPO")
    wpo.set_editor_property(
        "output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    wpo.set_editor_property("code", BILLBOARD_HLSL)

    # Custom inputs (names must match HLSL parameter names)
    ci_cam = unreal.CustomInput()
    ci_cam.set_editor_property("input_name", "InCamPos")
    ci_obj = unreal.CustomInput()
    ci_obj.set_editor_property("input_name", "InObjPos")
    ci_local = unreal.CustomInput()
    ci_local.set_editor_property("input_name", "InLocalPos")
    ci_world = unreal.CustomInput()
    ci_world.set_editor_property("input_name", "InVertWorldPos")
    wpo.set_editor_property(
        "inputs", [ci_cam, ci_obj, ci_local, ci_world])

    # Wire input pins
    mel.connect_material_expressions(cam_pos, "", wpo, "InCamPos")
    mel.connect_material_expressions(obj_pos, "", wpo, "InObjPos")
    mel.connect_material_expressions(local_pos, "", wpo, "InLocalPos")
    mel.connect_material_expressions(world_pos, "", wpo, "InVertWorldPos")

    # Output to WorldPositionOffset
    mel.connect_material_property(
        wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    # Landscape Grass uses HISM
    mat.set_editor_property("used_with_instanced_static_meshes", True)

    mel.recompile_material(mat)
    eal.save_asset(mat.get_path_name())
    unreal.log(f"  OK: {name} — billboard WPO added and saved")

unreal.log("\nDone. Walk into a Grass_Clump_B patch and orbit the camera.")
unreal.log("If grey boxes again — check Output Log for material compile errors and report.")
