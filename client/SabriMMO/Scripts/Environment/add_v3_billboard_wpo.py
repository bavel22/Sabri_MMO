# add_v3_billboard_wpo.py
# Adds full-billboard World Position Offset to every M_V3_* material so painted
# scatter (grass/flowers/debris) faces the camera the same way the player sprite
# does — both pitch and yaw, roll locked to 0.
#
# Run from UE5 Editor Python console:
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\add_v3_billboard_wpo.py").read())
#
# Idempotent — re-running is safe; materials with the marker scalar are skipped.
#
# To revert: open a material, delete the BillboardWPO Custom node + its inputs,
# disconnect WorldPositionOffset, recompile.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_DIR = "/Game/SabriMMO/Environment/GrassV3/Materials"
MARKER_PARAM = "_BillboardWPO"  # presence = already set up

# Full billboard: matches SpriteCharacterActor::UpdateBillboard (pitch+yaw, roll=0).
# Mesh local convention assumed: +X depth, +Y lateral, +Z vertical (V2 sprite mesh).
BILLBOARD_HLSL = (
    "float3 CamPos = ResolvedView.WorldCameraOrigin;\n"
    "float3 ToCam = normalize(CamPos - InObjPos);\n"
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

# Find all M_V3_* materials
asset_list = eal.list_assets(MAT_DIR, recursive=False)
mat_paths = []
for a in asset_list:
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    if name.startswith("M_V3_"):
        mat_paths.append(clean)

unreal.log(f"\n=== V3 Billboard WPO Setup ===")
unreal.log(f"Found {len(mat_paths)} V3 materials")

UPDATED = 0
SKIPPED = 0
FAILED = 0

for mat_path in mat_paths:
    mat = unreal.load_asset(mat_path)
    if not mat or not isinstance(mat, unreal.Material):
        SKIPPED += 1
        continue

    name = mat.get_name()

    # Idempotency check: if marker param is already a parameter, skip
    try:
        scalar_params = mel.get_scalar_parameter_names(mat)
        if MARKER_PARAM in scalar_params:
            unreal.log(f"  Skip (already done): {name}")
            SKIPPED += 1
            continue
    except Exception:
        pass

    try:
        # Marker so re-runs are idempotent
        marker = mel.create_material_expression(
            mat, unreal.MaterialExpressionScalarParameter, -1400, 700)
        marker.set_editor_property("parameter_name", MARKER_PARAM)
        marker.set_editor_property("default_value", 1.0)

        # Input nodes
        obj_pos = mel.create_material_expression(
            mat, unreal.MaterialExpressionObjectPositionWS, -800, 350)
        local_pos = mel.create_material_expression(
            mat, unreal.MaterialExpressionLocalPosition, -800, 480)
        world_pos = mel.create_material_expression(
            mat, unreal.MaterialExpressionWorldPosition, -800, 610)

        # Custom HLSL WPO
        wpo = mel.create_material_expression(
            mat, unreal.MaterialExpressionCustom, -450, 480)
        wpo.set_editor_property("description", "BillboardWPO")
        wpo.set_editor_property(
            "output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        wpo.set_editor_property("code", BILLBOARD_HLSL)

        # Custom inputs (names must match HLSL parameter names)
        ci_local = unreal.CustomInput()
        ci_local.set_editor_property("input_name", "InLocalPos")
        ci_obj = unreal.CustomInput()
        ci_obj.set_editor_property("input_name", "InObjPos")
        ci_world = unreal.CustomInput()
        ci_world.set_editor_property("input_name", "InVertWorldPos")
        wpo.set_editor_property("inputs", [ci_local, ci_obj, ci_world])

        # Wire input pins
        mel.connect_material_expressions(local_pos, "", wpo, "InLocalPos")
        mel.connect_material_expressions(obj_pos, "", wpo, "InObjPos")
        mel.connect_material_expressions(world_pos, "", wpo, "InVertWorldPos")

        # Output to WorldPositionOffset
        mel.connect_material_property(
            wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

        # Landscape Grass uses HISM — make sure WPO compiles for instanced rendering
        mat.set_editor_property("used_with_instanced_static_meshes", True)

        mel.recompile_material(mat)
        eal.save_asset(mat.get_path_name())
        UPDATED += 1
        unreal.log(f"  OK: {name}")
    except Exception as e:
        unreal.log_warning(f"  FAILED on {name}: {e}")
        FAILED += 1

unreal.log(
    f"\n=== Summary === Updated={UPDATED} Skipped={SKIPPED} Failed={FAILED}")
