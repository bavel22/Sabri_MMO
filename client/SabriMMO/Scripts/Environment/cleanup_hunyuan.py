# cleanup_hunyuan.py
# Deletes EVERYTHING under /Game/SabriMMO/Environment/Hunyuan/.
# Use this to reset before re-running import_hunyuan_assets.py with new layout.
#
# Run from UE5 Output Log (Python mode):
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/cleanup_hunyuan.py"

import unreal

PATH = "/Game/SabriMMO/Environment/Hunyuan"

eal = unreal.EditorAssetLibrary

unreal.log("=" * 70)
unreal.log(f"CLEANUP {PATH}")
unreal.log("=" * 70)

if not eal.does_directory_exist(PATH):
    unreal.log(f"  {PATH} does not exist. Nothing to clean up.")
else:
    # List everything before deleting (for the log)
    assets = eal.list_assets(PATH, recursive=True, include_folder=False)
    unreal.log(f"  Found {len(assets)} assets to delete:")
    for a in sorted(assets):
        unreal.log(f"    - {a}")

    # Force-delete all assets first (handles cases where references would block)
    deleted = 0
    for a in assets:
        if eal.delete_asset(a):
            deleted += 1
        else:
            unreal.log_warning(f"    failed to delete {a}")
    unreal.log(f"  Deleted {deleted}/{len(assets)} assets")

    # Now remove the directory tree
    if eal.delete_directory(PATH):
        unreal.log(f"  Removed directory: {PATH}")
    else:
        unreal.log_warning(f"  Could not remove directory: {PATH}")

unreal.log("=" * 70)
unreal.log("DONE — re-run import_hunyuan_assets.py to reimport.")
unreal.log("=" * 70)
