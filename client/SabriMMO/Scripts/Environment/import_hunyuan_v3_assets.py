# import_hunyuan_v3_assets.py
# Bulk import v3 Hunyuan GLBs from C:/Sabri_MMO/3d art/Final_v3/.
# Sets Collision Complexity = "Use Complex Collision As Simple" on each
# StaticMesh DURING import, so placed instances pick up the right collision
# from the start (no post-import fix required).
#
# Final layout (same as v1/v2 — coexists in the same folders):
#   /Game/SabriMMO/Environment/Hunyuan/
#     <category>/
#       <asset>                            <- v3 assets are NEW (no _vN suffix)
#       <asset>_<variant>
#       Materials/
#         M_<asset>
#       Textures/
#         T_<asset>_*
#
# Run from UE5 Output Log (Python mode):
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_hunyuan_v3_assets.py"

import unreal
import os
import glob

# ============================================================================
# CONFIG
# ============================================================================
SOURCE_ROOT = "C:/Sabri_MMO/3d art/Final_v3"
DEST_ROOT = "/Game/SabriMMO/Environment/Hunyuan"

# Skip assets that already exist in UE5 (so re-runs only fill gaps)
SKIP_EXISTING = True

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

TARGET_COLLISION_FLAG = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE

# ============================================================================
# UE5 API
# ============================================================================
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


# ============================================================================
# HELPERS
# ============================================================================
def cleanup_asset(dest_dir, asset_name):
    """Remove existing assets for asset_name (final layout + Interchange leftovers)."""
    for path in [
        f"{dest_dir}/{asset_name}",
        f"{dest_dir}/Materials/M_{asset_name}",
    ]:
        if eal.does_asset_exist(path):
            eal.delete_asset(path)

    tex_dir = f"{dest_dir}/Textures"
    if eal.does_directory_exist(tex_dir):
        for tp in eal.list_assets(tex_dir, recursive=False, include_folder=False):
            tname = tp.split("/")[-1]
            if tname.startswith(f"T_{asset_name}_") or tname == f"T_{asset_name}":
                eal.delete_asset(tp)

    interchange_dir = f"{dest_dir}/{asset_name}"
    if eal.does_directory_exist(interchange_dir):
        for sub in eal.list_assets(interchange_dir, recursive=True, include_folder=False):
            eal.delete_asset(sub)
        eal.delete_directory(interchange_dir)


def set_complex_as_simple(sm_path):
    """Set Collision Complexity = Use Complex Collision As Simple on a StaticMesh."""
    asset = unreal.load_asset(sm_path)
    if not isinstance(asset, unreal.StaticMesh):
        return False
    body_setup = asset.get_editor_property("body_setup")
    if not body_setup:
        return False
    body_setup.set_editor_property("collision_trace_flag", TARGET_COLLISION_FLAG)
    eal.save_asset(sm_path)
    return True


def import_one_glb(glb_path, dest_dir):
    """Import GLB, reorganize into flat layout, set collision flag. Returns SM path or None."""
    asset_name = os.path.splitext(os.path.basename(glb_path))[0]
    sm_target = f"{dest_dir}/{asset_name}"
    mat_target = f"{dest_dir}/Materials/M_{asset_name}"

    if eal.does_asset_exist(sm_target) and SKIP_EXISTING:
        unreal.log(f"  SKIP {asset_name} (exists)")
        return sm_target

    cleanup_asset(dest_dir, asset_name)

    # Import (Interchange creates <dest>/<asset>/{StaticMeshes,Materials,Textures}/)
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("filename", glb_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])

    interchange_dir = f"{dest_dir}/{asset_name}"
    sm_subdir = f"{interchange_dir}/StaticMeshes"
    mat_subdir = f"{interchange_dir}/Materials"
    tex_subdir = f"{interchange_dir}/Textures"

    sm_paths = (eal.list_assets(sm_subdir, recursive=False, include_folder=False)
                if eal.does_directory_exist(sm_subdir) else [])
    mat_paths = (eal.list_assets(mat_subdir, recursive=False, include_folder=False)
                 if eal.does_directory_exist(mat_subdir) else [])
    tex_paths = sorted(eal.list_assets(tex_subdir, recursive=False, include_folder=False)
                       if eal.does_directory_exist(tex_subdir) else [])

    if not sm_paths:
        unreal.log_warning(f"  FAIL {asset_name}: no StaticMesh under {sm_subdir}")
        return None

    # Move textures (auto-updates references in auto-material)
    tex_dir = f"{dest_dir}/Textures"
    eal.make_directory(tex_dir)
    moved_tex = 0
    for tp in tex_paths:
        old_name = tp.split("/")[-1]
        new_name = old_name if old_name.startswith("T_") else f"T_{old_name}"
        new_path = f"{tex_dir}/{new_name}"
        if eal.does_asset_exist(new_path):
            eal.delete_asset(new_path)
        if eal.rename_asset(tp, new_path):
            moved_tex += 1

    # Move material -> M_<asset>
    mat_dir = f"{dest_dir}/Materials"
    eal.make_directory(mat_dir)
    moved_mat = False
    if mat_paths:
        mat_orig = mat_paths[0]
        if eal.does_asset_exist(mat_target):
            eal.delete_asset(mat_target)
        if eal.rename_asset(mat_orig, mat_target):
            moved_mat = True
        for extra in mat_paths[1:]:
            if eal.does_asset_exist(extra):
                eal.delete_asset(extra)

    # Move SM up to <dest>/<asset_name>
    sm_orig = sm_paths[0]
    if sm_orig != sm_target:
        if eal.does_asset_exist(sm_target):
            eal.delete_asset(sm_target)
        if not eal.rename_asset(sm_orig, sm_target):
            unreal.log_warning(f"  failed to move SM {sm_orig} -> {sm_target}")
            return None

    # Cleanup empty Interchange subfolders
    for sub in [sm_subdir, mat_subdir, tex_subdir, interchange_dir]:
        if eal.does_directory_exist(sub):
            eal.delete_directory(sub)

    # Set Collision Complexity = Use Complex Collision As Simple
    collision_set = set_complex_as_simple(sm_target)

    cflag = "+collision" if collision_set else "(NO collision set)"
    mflag = "M_" if moved_mat else "no_mat"
    unreal.log(f"  OK {asset_name}: SM + {mflag} + {moved_tex} tex {cflag}")
    return sm_target


def main():
    unreal.log("=" * 70)
    unreal.log("HUNYUAN V3 ASSET IMPORT (with collision = complex as simple)")
    unreal.log(f"  Source:        {SOURCE_ROOT}")
    unreal.log(f"  Destination:   {DEST_ROOT}")
    unreal.log(f"  Skip existing: {SKIP_EXISTING}")
    unreal.log("=" * 70)

    eal.make_directory(DEST_ROOT)

    files = []
    for cat in CATEGORIES:
        cat_dir = f"{SOURCE_ROOT}/{cat}"
        for g in sorted(glob.glob(f"{cat_dir}/*.glb")):
            files.append((cat, g))
    unreal.log(f"\nFound {len(files)} v3 GLBs to import")

    if not files:
        unreal.log_warning("No files to import.")
        return

    by_cat = {}
    for cat, g in files:
        by_cat.setdefault(cat, []).append(g)

    total_ok = 0
    total_fail = 0
    for cat, glbs in by_cat.items():
        dest_dir = f"{DEST_ROOT}/{cat}"
        eal.make_directory(dest_dir)
        unreal.log(f"\n=== {cat} ({len(glbs)} files) ===")
        for glb in glbs:
            try:
                result = import_one_glb(glb, dest_dir)
                if result:
                    total_ok += 1
                else:
                    total_fail += 1
            except Exception as e:
                total_fail += 1
                unreal.log_error(f"  EXCEPTION on {glb}: {e}")
                import traceback
                unreal.log_error(traceback.format_exc())

    unreal.log("\n" + "=" * 70)
    unreal.log(f"DONE: {total_ok} ok, {total_fail} failed")
    unreal.log("=" * 70)


main()
