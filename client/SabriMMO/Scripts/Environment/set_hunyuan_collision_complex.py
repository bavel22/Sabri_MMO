# set_hunyuan_collision_complex.py
# Sets Collision Complexity = "Use Complex Collision As Simple" on every
# imported Hunyuan StaticMesh under /Game/SabriMMO/Environment/Hunyuan/.
#
# Idempotent — skips assets already set to Complex As Simple.
# Logs each updated asset.
#
# Run from UE5 Output Log (Python mode):
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/set_hunyuan_collision_complex.py"
#
# After running, rebuild NavMesh: Build > Build Paths Only

import unreal

# ============================================================================
# CONFIG
# ============================================================================
ROOT_PATH = "/Game/SabriMMO/Environment/Hunyuan"

# Categories to skip. Empty = apply to ALL Hunyuan static meshes.
SKIP_CATEGORIES = []

# ============================================================================
# UE5 API
# ============================================================================
eal = unreal.EditorAssetLibrary
TARGET_FLAG = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE


def main():
    if not eal.does_directory_exist(ROOT_PATH):
        unreal.log_warning(f"Path not found: {ROOT_PATH}. Nothing to do.")
        return

    unreal.log("=" * 70)
    unreal.log("SET HUNYUAN COLLISION = COMPLEX AS SIMPLE")
    unreal.log(f"  Root:           {ROOT_PATH}")
    unreal.log(f"  Skip categories: {SKIP_CATEGORIES}")
    unreal.log("=" * 70)

    all_paths = eal.list_assets(ROOT_PATH, recursive=True, include_folder=False)

    updated = 0
    skipped_already_set = 0
    skipped_category = 0
    skipped_not_sm = 0
    failed = 0

    for path in all_paths:
        # Skip configured categories (e.g. vegetation)
        if any(f"/{c}/" in path for c in SKIP_CATEGORIES):
            skipped_category += 1
            continue

        try:
            asset = unreal.load_asset(path)
        except Exception as e:
            unreal.log_warning(f"  load failed: {path} ({e})")
            failed += 1
            continue

        if not isinstance(asset, unreal.StaticMesh):
            skipped_not_sm += 1
            continue

        body_setup = asset.get_editor_property("body_setup")
        if not body_setup:
            unreal.log_warning(f"  no body_setup: {path}")
            failed += 1
            continue

        current = body_setup.get_editor_property("collision_trace_flag")
        if current == TARGET_FLAG:
            skipped_already_set += 1
            continue

        try:
            body_setup.set_editor_property("collision_trace_flag", TARGET_FLAG)
            eal.save_asset(path)
            short = path.split("/")[-1]
            unreal.log(f"  OK {short}")
            updated += 1
        except Exception as e:
            unreal.log_warning(f"  failed to set {path}: {e}")
            failed += 1

    unreal.log("")
    unreal.log("=" * 70)
    unreal.log(f"DONE")
    unreal.log(f"  Updated:               {updated}")
    unreal.log(f"  Already set:           {skipped_already_set}")
    unreal.log(f"  Skipped (vegetation):  {skipped_category}")
    unreal.log(f"  Skipped (not SM):      {skipped_not_sm}")
    unreal.log(f"  Failed:                {failed}")
    unreal.log("=" * 70)
    unreal.log("Now: Build > Build Paths Only to regenerate NavMesh.")


main()
