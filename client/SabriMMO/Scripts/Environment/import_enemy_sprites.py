# import_enemy_sprites.py
# Bulk import enemy sprite atlases and apply sprite texture settings.
# Scans Content/SabriMMO/Sprites/Atlases/Body/enemies/{name}/*.png
# and imports any missing atlases with settings:
#   Canonical 2026-04-27 — see memory/feedback-sprite-texture-group-ui.md:
#   - Filter:                            Nearest
#   - Compression:                       BC7
#   - Mip Gen Settings:                  SimpleAverage
#   - Use New Mip Filter:                True
#   - Do Scale Mips For Alpha Coverage:  True
#   - Alpha Coverage Thresholds:         (0, 0, 0, 0.5)
#   - Maximum Texture Size:              0   (no cap)
#   - Never Stream:                      False
#   - Texture Group:                     UI
#   - sRGB:                              True
#
# Run from UE5 Editor Python console:
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_enemy_sprites.py"

import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

# Set to None to process all subfolders; set to a folder name string for test mode.
SUBFOLDER_FILTER = None


def apply_sprite_settings(asset_path):
    """Canonical Sabri_MMO sprite atlas settings (UPDATED 2026-04-27).

    Required for the runtime Sprite Quality slider, ZonePreloadSubsystem
    async-load, and Path C deferred equipment swap to work correctly.
    See memory `feedback-sprite-texture-group-ui.md` for full reference.
    """
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False

    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    tex.set_editor_property("use_new_mip_filter", True)
    tex.set_editor_property("do_scale_mips_for_alpha_coverage", True)
    tex.set_editor_property("alpha_coverage_thresholds",
        unreal.Vector4(0.0, 0.0, 0.0, 0.5))
    tex.set_editor_property("max_texture_size", 0)
    tex.set_editor_property("never_stream", False)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("srgb", True)

    eal.save_asset(clean)
    return True


def import_folder(source_dir, dest_path):
    """Import all PNGs from source_dir into dest_path. Skip existing for import,
    but apply settings to everything."""
    pngs = sorted(glob.glob(os.path.join(source_dir, "*.png")))
    if not pngs:
        return 0, 0

    tasks = []
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{dest_path}/{name}"
        if eal.does_asset_exist(asset_path):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", dest_path)
        task.set_editor_property("filename", png)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    imported = 0
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        imported = len(tasks)

    # Apply settings to every PNG in this folder (both new and existing)
    configured = 0
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{dest_path}/{name}"
        if eal.does_asset_exist(asset_path) and apply_sprite_settings(asset_path):
            configured += 1

    return imported, configured


# ──────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────
if not os.path.isdir(ROOT):
    unreal.log_error(f"Root not found: {ROOT}")
else:
    all_subdirs = sorted([d for d in os.listdir(ROOT) if os.path.isdir(os.path.join(ROOT, d))])

    # Test mode: limit to a single folder when SUBFOLDER_FILTER is set
    if SUBFOLDER_FILTER:
        subdirs = [d for d in all_subdirs if d == SUBFOLDER_FILTER]
        unreal.log(f"TEST MODE: processing only '{SUBFOLDER_FILTER}' ({len(subdirs)} folder)")
    else:
        subdirs = all_subdirs
        unreal.log(f"Scanning {len(subdirs)} enemy folders...")

    total_imported = 0
    total_configured = 0
    for name in subdirs:
        src = os.path.join(ROOT, name)
        dest = f"{GAME_ROOT}/{name}"
        imported, configured = import_folder(src, dest)
        unreal.log(f"  {name}: imported {imported}, configured {configured}")
        total_imported += imported
        total_configured += configured

    unreal.log(f"\n{'=' * 50}")
    unreal.log(f"  Total imported:   {total_imported}")
    unreal.log(f"  Total configured: {total_configured}")
    unreal.log(f"{'=' * 50}")
