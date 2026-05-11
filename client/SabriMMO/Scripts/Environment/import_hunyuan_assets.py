# import_hunyuan_assets.py
# Bulk import Hunyuan-generated GLBs from C:/Sabri_MMO/3d art/Final/<category>/.
# Pure reorganize — does NOT modify materials in any way. The auto-imported
# PBR_Material from UE5's Interchange GLB importer is moved + renamed only.
#
# Final layout:
#   /Game/SabriMMO/Environment/Hunyuan/
#     <category>/
#       <asset>                         <- static mesh (no prefix; matches GLB name)
#       <asset>_<variant>
#       Materials/
#         M_<asset>                     <- auto-imported material, just renamed
#         M_<asset>_<variant>
#       Textures/
#         T_<asset>_texture_0           <- diffuse
#         T_<asset>_texture_1           <- secondary (likely ORM or normal)
#         ...
#
# UE5.7 Interchange GLB import creates per-asset subfolders with
# StaticMeshes/, Materials/, Textures/ inside. This script flattens them.
# Asset references (mesh -> material, material -> textures) auto-update on
# rename, so the visual look is preserved exactly as UE5 imported it.
#
# Run from UE5 Output Log (Python mode):
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_hunyuan_assets.py"

import unreal
import os
import glob

# ============================================================================
# CONFIG
# ============================================================================
SOURCE_ROOT = "C:/Sabri_MMO/3d art/Final"
DEST_ROOT = "/Game/SabriMMO/Environment/Hunyuan"

TEST_MODE = False
TEST_CATEGORY = "architecture"
TEST_ASSET = "arch_ruined"
FORCE_REIMPORT = False

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

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
    # Final-layout files
    for path in [
        f"{dest_dir}/{asset_name}",
        f"{dest_dir}/Materials/M_{asset_name}",
    ]:
        if eal.does_asset_exist(path):
            eal.delete_asset(path)

    # Final-layout textures (T_<asset>_*)
    tex_dir = f"{dest_dir}/Textures"
    if eal.does_directory_exist(tex_dir):
        for tp in eal.list_assets(tex_dir, recursive=False, include_folder=False):
            tname = tp.split("/")[-1]
            if tname.startswith(f"T_{asset_name}_") or tname == f"T_{asset_name}":
                eal.delete_asset(tp)

    # Interchange per-asset subfolder
    interchange_dir = f"{dest_dir}/{asset_name}"
    if eal.does_directory_exist(interchange_dir):
        for sub in eal.list_assets(interchange_dir, recursive=True, include_folder=False):
            eal.delete_asset(sub)
        eal.delete_directory(interchange_dir)


def import_one_glb(glb_path, dest_dir):
    """Import GLB and reorganize into flat layout. Returns SM path or None."""
    asset_name = os.path.splitext(os.path.basename(glb_path))[0]
    sm_target = f"{dest_dir}/{asset_name}"
    mat_target = f"{dest_dir}/Materials/M_{asset_name}"

    if eal.does_asset_exist(sm_target) and not FORCE_REIMPORT:
        unreal.log(f"  SKIP {asset_name} (exists)")
        return sm_target

    if FORCE_REIMPORT:
        cleanup_asset(dest_dir, asset_name)

    # Import — Interchange creates <dest>/<asset>/{StaticMeshes,Materials,Textures}/
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

    # ---- Move textures ----
    # Renaming via API auto-updates references in the auto-material that points to them
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
        else:
            unreal.log_warning(f"    failed to move {tp} -> {new_path}")

    # ---- Move material — rename PBR_Material -> M_<asset> ----
    # Auto-material's texture refs already updated above; SM's material slot
    # reference auto-updates on this rename
    mat_dir = f"{dest_dir}/Materials"
    eal.make_directory(mat_dir)
    moved_mat = False
    if mat_paths:
        # Use the first material (most common) — rename to M_<asset>
        mat_orig = mat_paths[0]
        if eal.does_asset_exist(mat_target):
            eal.delete_asset(mat_target)
        if eal.rename_asset(mat_orig, mat_target):
            moved_mat = True
        else:
            unreal.log_warning(f"    failed to move {mat_orig} -> {mat_target}")
        # Delete any extra materials (rare — e.g., if GLB had multiple materials)
        for extra in mat_paths[1:]:
            if eal.does_asset_exist(extra):
                eal.delete_asset(extra)

    # ---- Move SM up to <dest>/<asset_name> ----
    sm_orig = sm_paths[0]
    if sm_orig != sm_target:
        if eal.does_asset_exist(sm_target):
            eal.delete_asset(sm_target)
        if not eal.rename_asset(sm_orig, sm_target):
            unreal.log_warning(f"  failed to move SM {sm_orig} -> {sm_target}")
            return None

    # ---- Cleanup empty Interchange subfolders ----
    for sub in [sm_subdir, mat_subdir, tex_subdir, interchange_dir]:
        if eal.does_directory_exist(sub):
            eal.delete_directory(sub)

    unreal.log(f"  OK   {asset_name}: SM + {'M_' if moved_mat else 'no '}mat + {moved_tex} tex")
    return sm_target


# ============================================================================
# MAIN
# ============================================================================
def main():
    unreal.log("=" * 70)
    unreal.log("HUNYUAN ASSET IMPORT (pure reorganize)")
    unreal.log(f"  Mode:           {'TEST' if TEST_MODE else 'FULL'}")
    unreal.log(f"  Force reimport: {FORCE_REIMPORT}")
    unreal.log(f"  Source:         {SOURCE_ROOT}")
    unreal.log(f"  Destination:    {DEST_ROOT}")
    unreal.log("=" * 70)

    eal.make_directory(DEST_ROOT)

    files = []
    if TEST_MODE:
        cat_dir = f"{SOURCE_ROOT}/{TEST_CATEGORY}"
        for g in sorted(glob.glob(f"{cat_dir}/{TEST_ASSET}*.glb")):
            files.append((TEST_CATEGORY, g))
        unreal.log(f"\nTEST MODE: {len(files)} files matching '{TEST_ASSET}*'")
    else:
        for cat in CATEGORIES:
            for g in sorted(glob.glob(f"{SOURCE_ROOT}/{cat}/*.glb")):
                files.append((cat, g))
        unreal.log(f"\nFULL MODE: {len(files)} files")

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
                if import_one_glb(glb, dest_dir):
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
