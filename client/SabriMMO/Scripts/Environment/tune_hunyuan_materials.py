# tune_hunyuan_materials.py
# Forces Roughness and Metallic on Hunyuan-imported materials so you can A/B
# compare to the original auto-imported PBR look.
#
# How it works: adds Constant nodes for Roughness (0.95) and Metallic (0.0)
# and connects them to the material's property pins. This REPLACES whatever
# was previously driving those pins (e.g. an ORM texture sample).
#
# Default mode: only tunes TUNE_ASSET's materials (test mode).
# Switch ALL_MODE = True to tune every M_* under Hunyuan/.
#
# Run from UE5 Output Log (Python mode):
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/tune_hunyuan_materials.py"

import unreal

# ============================================================================
# CONFIG
# ============================================================================
ROUGHNESS = 0.95   # 0 = mirror, 1 = chalk. RO Classic = matte ~0.95
METALLIC = 0.0     # 0 = no metal. RO Classic has no specular sheen.

# What to tune
ALL_MODE = False
TUNE_CATEGORY = "architecture"
TUNE_ASSET = "arch_ruined"   # tunes M_arch_ruined + M_arch_ruined_*

DEST_ROOT = "/Game/SabriMMO/Environment/Hunyuan"
CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

# ============================================================================
# UE5 API
# ============================================================================
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def tune_material(mat_path):
    """Force Roughness + Metallic constants on a UMaterial. Returns True on success."""
    if not eal.does_asset_exist(mat_path):
        unreal.log_warning(f"  not found: {mat_path}")
        return False

    mat = unreal.load_asset(mat_path)
    if not isinstance(mat, unreal.Material):
        unreal.log_warning(f"  {mat_path} is not a UMaterial; skipping")
        return False

    # Roughness constant — connect_material_property replaces whatever was on the pin
    rough_node = mel.create_material_expression(
        mat, unreal.MaterialExpressionConstant, 600, 200
    )
    rough_node.set_editor_property("r", ROUGHNESS)
    mel.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Metallic constant
    metal_node = mel.create_material_expression(
        mat, unreal.MaterialExpressionConstant, 600, 350
    )
    metal_node.set_editor_property("r", METALLIC)
    mel.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

    mel.recompile_material(mat)
    eal.save_asset(mat_path)
    unreal.log(f"  TUNED {mat_path}")
    return True


def collect_materials():
    """Returns list of material asset paths to tune."""
    paths = []
    if ALL_MODE:
        cats = CATEGORIES
    else:
        cats = [TUNE_CATEGORY]

    for cat in cats:
        mat_dir = f"{DEST_ROOT}/{cat}/Materials"
        if not eal.does_directory_exist(mat_dir):
            continue
        for mp in eal.list_assets(mat_dir, recursive=False, include_folder=False):
            name = mp.split("/")[-1]
            if not name.startswith("M_"):
                continue
            if not ALL_MODE:
                # In test mode, match only TUNE_ASSET (base + variants)
                if name == f"M_{TUNE_ASSET}" or name.startswith(f"M_{TUNE_ASSET}_"):
                    paths.append(mp)
            else:
                paths.append(mp)
    return paths


def main():
    unreal.log("=" * 70)
    unreal.log("HUNYUAN MATERIAL TUNE")
    unreal.log(f"  Mode:      {'ALL' if ALL_MODE else f'TEST ({TUNE_ASSET})'}")
    unreal.log(f"  Roughness: {ROUGHNESS}")
    unreal.log(f"  Metallic:  {METALLIC}")
    unreal.log("=" * 70)

    paths = collect_materials()
    if not paths:
        unreal.log_warning("No materials found to tune.")
        return

    unreal.log(f"\nFound {len(paths)} materials to tune:")
    for p in paths:
        unreal.log(f"  - {p}")

    unreal.log("")
    ok = 0
    fail = 0
    for p in paths:
        try:
            if tune_material(p):
                ok += 1
            else:
                fail += 1
        except Exception as e:
            fail += 1
            unreal.log_error(f"  EXCEPTION tuning {p}: {e}")
            import traceback
            unreal.log_error(traceback.format_exc())

    unreal.log("\n" + "=" * 70)
    unreal.log(f"DONE: {ok} tuned, {fail} failed")
    unreal.log("=" * 70)


main()
