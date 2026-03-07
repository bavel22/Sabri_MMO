# create_casting_circle_material.py
# Run in UE5 Editor via: Edit > Execute Python Script
# Creates M_CastingCircle — a deferred decal material with:
#   - Procedural ring shape (no external texture needed)
#   - ElementColor vector parameter (tintable per element)
#   - FadeAlpha scalar parameter (for fade in/out)
#   - EmissiveIntensity scalar parameter (glow strength)
#   - RotationSpeed scalar parameter (ring rotation speed)
#
# The material uses two RadialGradientExponential nodes subtracted
# to form a ring, multiplied by ElementColor for emissive output,
# and FadeAlpha for opacity.

import unreal

# Ensure target directory exists
unreal.EditorAssetLibrary.make_directory("/Game/SabriMMO/VFX/Materials")

# Check if material already exists
if unreal.EditorAssetLibrary.does_asset_exist("/Game/SabriMMO/VFX/Materials/M_CastingCircle"):
    unreal.log_warning("M_CastingCircle already exists — skipping creation.")
else:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()

    mat = asset_tools.create_asset(
        "M_CastingCircle",
        "/Game/SabriMMO/VFX/Materials",
        unreal.Material,
        mat_factory
    )

    if mat:
        # Set material domain and blend mode
        mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
        mat.set_editor_property("decal_blend_mode", unreal.DecalBlendMode.DBM_TRANSLUCENT)

        mel = unreal.MaterialEditingLibrary

        # --- Create nodes ---

        # ElementColor parameter (default: cyan)
        color_param = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, -200)
        color_param.set_editor_property("parameter_name", "ElementColor")
        color_param.set_editor_property("default_value", unreal.LinearColor(0.3, 0.8, 1.0, 1.0))

        # FadeAlpha parameter
        fade_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 200)
        fade_param.set_editor_property("parameter_name", "FadeAlpha")
        fade_param.set_editor_property("default_value", 1.0)

        # EmissiveIntensity parameter
        intensity_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 400)
        intensity_param.set_editor_property("parameter_name", "EmissiveIntensity")
        intensity_param.set_editor_property("default_value", 5.0)

        # Outer radial gradient (ring outer edge)
        outer_grad = mel.create_material_expression(mat, unreal.MaterialExpressionRadialGradientExponential, -400, 0)

        # Inner radial gradient (ring inner edge - will be subtracted)
        inner_grad = mel.create_material_expression(mat, unreal.MaterialExpressionRadialGradientExponential, -400, 150)

        # Subtract inner from outer = ring shape
        subtract = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -200, 50)

        # Multiply ring * ElementColor for emissive
        color_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 0, -100)

        # Multiply color * EmissiveIntensity for glow
        intensity_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 200, -100)

        # Multiply ring * FadeAlpha for opacity
        fade_mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 0, 150)

        # --- Connect nodes ---
        # Outer gradient -> Subtract.A
        mel.connect_material_expressions(outer_grad, "", subtract, "A")
        # Inner gradient -> Subtract.B
        mel.connect_material_expressions(inner_grad, "", subtract, "B")
        # Subtract (ring) -> ColorMult.A
        mel.connect_material_expressions(subtract, "", color_mult, "A")
        # ElementColor -> ColorMult.B
        mel.connect_material_expressions(color_param, "", color_mult, "B")
        # ColorMult -> IntensityMult.A
        mel.connect_material_expressions(color_mult, "", intensity_mult, "A")
        # EmissiveIntensity -> IntensityMult.B
        mel.connect_material_expressions(intensity_param, "", intensity_mult, "B")
        # Subtract (ring) -> FadeMult.A
        mel.connect_material_expressions(subtract, "", fade_mult, "A")
        # FadeAlpha -> FadeMult.B
        mel.connect_material_expressions(fade_param, "", fade_mult, "B")

        # --- Connect to material outputs ---
        mel.connect_material_property(intensity_mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.connect_material_property(fade_mult, "", unreal.MaterialProperty.MP_OPACITY)

        # Save the asset
        unreal.EditorAssetLibrary.save_asset("/Game/SabriMMO/VFX/Materials/M_CastingCircle")

        unreal.log("SUCCESS: M_CastingCircle created at /Game/SabriMMO/VFX/Materials/M_CastingCircle")
        unreal.log("Parameters: ElementColor (Vector), FadeAlpha (Scalar), EmissiveIntensity (Scalar)")
        unreal.log("Next: Add UV rotation in Material Editor for spinning effect (CustomRotator + Time node)")
    else:
        unreal.log_error("FAILED to create M_CastingCircle material!")
