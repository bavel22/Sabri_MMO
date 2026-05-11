# debug_pryth_paint_layers.py
# Diagnostic: dumps M_Landscape_RO_18's parameter info + paint layer info
# to show exactly what UE5 thinks is paintable.
#
# Run in UE5 Python console:
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\debug_pryth_paint_layers.py").read())

import unreal

MAT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_18"

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

print("=" * 70)
print(f"DEBUG: {MAT_PATH}")
print("=" * 70)

mat = eal.load_asset(MAT_PATH)
if not mat:
    print(f"ERROR: Material not found at {MAT_PATH}")
    print("Run create_landscape_ro_18.py first.")
else:
    print(f"Material loaded: {mat.get_name()}")
    print(f"Parent class: {mat.__class__.__name__}")
    print()

    # === Texture parameters ===
    # Note: unreal.Name doesn't support f-string format specs, so str() it first.
    print("--- TEXTURE PARAMETERS ---")
    try:
        tex_params = mel.get_texture_parameter_names(mat)
        for name in tex_params:
            sname = str(name)
            tex = mel.get_material_default_texture_parameter_value(mat, name)
            tex_path = tex.get_path_name() if tex else "(none)"
            print(f"  {sname:24s} = {tex_path}")
    except Exception as e:
        print(f"  ERROR querying texture params: {e}")
    print()

    # === Scalar parameters ===
    print("--- SCALAR PARAMETERS (values shown for v18 sanity check) ---")
    try:
        scalar_params = mel.get_scalar_parameter_names(mat)
        for name in sorted([str(n) for n in scalar_params]):
            val = mel.get_material_default_scalar_parameter_value(mat, name)
            print(f"  {name:24s} = {val}")
    except Exception as e:
        print(f"  ERROR querying scalar params: {e}")
    print()

    # === Vector parameters ===
    print("--- VECTOR PARAMETERS ---")
    try:
        vector_params = mel.get_vector_parameter_names(mat)
        for name in vector_params:
            sname = str(name)
            val = mel.get_material_default_vector_parameter_value(mat, name)
            print(f"  {sname:24s} = ({val.r:.3f}, {val.g:.3f}, {val.b:.3f})")
    except Exception as e:
        print(f"  ERROR querying vector params: {e}")
    print()

    # === Compile errors (most important) ===
    print("--- COMPILATION STATUS ---")
    try:
        # Force a recompile and check
        mel.recompile_material(mat)
        print("  Recompile triggered (no exception)")
    except Exception as e:
        print(f"  RECOMPILE ERROR: {e}")
    print()

    # === Iterate expressions (count LandscapeLayerWeight) ===
    print("--- EXPRESSION COUNT (LandscapeLayerWeight nodes) ---")
    try:
        # The 'expressions' property on UMaterial might be protected, but
        # we can iterate the material function library.
        # Different UE5 versions expose this differently:
        all_exprs = mat.expressions if hasattr(mat, "expressions") else []
        if not all_exprs:
            try:
                all_exprs = list(mat.get_editor_property("expressions"))
            except Exception:
                all_exprs = []

        layer_weight_nodes = [
            e for e in all_exprs
            if e.__class__.__name__ == "MaterialExpressionLandscapeLayerWeight"
        ]
        layer_sample_nodes = [
            e for e in all_exprs
            if e.__class__.__name__ == "MaterialExpressionLandscapeLayerSample"
        ]
        print(f"  Total expressions: {len(all_exprs)}")
        print(f"  LandscapeLayerWeight nodes: {len(layer_weight_nodes)}")
        for e in layer_weight_nodes:
            try:
                pname = e.get_editor_property("parameter_name")
                print(f"    -> parameter_name = '{pname}'")
            except Exception:
                pass
        print(f"  LandscapeLayerSample nodes: {len(layer_sample_nodes)}")
        for e in layer_sample_nodes:
            try:
                pname = e.get_editor_property("parameter_name")
                print(f"    -> parameter_name = '{pname}'")
            except Exception:
                pass
    except Exception as e:
        print(f"  ERROR iterating expressions: {e}")
    print()

print("=" * 70)
print("DEBUG END")
print("=" * 70)
