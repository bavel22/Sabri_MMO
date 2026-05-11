"""RECOVERY part 2 of 7: re-import weapons batch A (female): axe_1h, axe_2h, book, bow.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_recover_02_weapons_a.py"

After it finishes, wait for "Compiling Textures: 0" before running part 3.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"

WEAPON_TYPES = ["axe_1h", "axe_2h", "book", "bow"]


def apply_canonical_settings(asset_path):
    """Apply the canonical Sabri_MMO sprite atlas settings (2026-04-27).

    Required for: runtime Sprite Quality slider (LODBias 0-4),
    ZonePreloadSubsystem async-load, Path C deferred equipment swap.
    See memory/feedback-sprite-texture-group-ui.md for the full rule set.
    """
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
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


def delete_dv_conflicts(png_path):
    """Remove orphan .dv-conflict*.uasset files left by Diversion (raw filesystem)."""
    disk_dir = os.path.dirname(png_path)
    base = os.path.splitext(os.path.basename(png_path))[0]
    deleted = 0
    for f in glob.glob(os.path.join(disk_dir, f"{base}.dv-conflict*.uasset")):
        try:
            os.remove(f)
            deleted += 1
        except OSError as e:
            unreal.log_warning(f"  Could not delete {f}: {e}")
    return deleted


def recover_one(png_path, dest_dir, asset_path):
    n_conflicts = delete_dv_conflicts(png_path)
    clean = asset_path.split(".")[0]
    if eal.does_asset_exist(clean):
        eal.delete_asset(clean)
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("filename", png_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not eal.does_asset_exist(clean):
        return ("import_failed", n_conflicts)
    apply_canonical_settings(asset_path)
    return ("recovered", n_conflicts)


def main():
    counts = {"recovered": 0, "import_failed": 0, "conflicts_deleted": 0}
    start = time.time()
    grand = 0
    for wtype in WEAPON_TYPES:
        disk = os.path.join(ATLAS_ROOT, "Weapon", wtype, "female")
        game = f"{GAME_ROOT}/Weapon/{wtype}/female"
        pngs = sorted(glob.glob(os.path.join(disk, "*.png")))
        unreal.log(f"--- {wtype}/female ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            result, n_conflicts = recover_one(png, game, f"{game}/{name}")
            counts[result] = counts.get(result, 0) + 1
            counts["conflicts_deleted"] += n_conflicts
            grand += 1
            if grand % 10 == 0:
                unreal.log(f"  [{grand}] {counts}")
    elapsed = time.time() - start
    unreal.log(f"=== Part 2 done in {elapsed:.1f}s. {counts} ===")
    unreal.log(f"Wait for 'Compiling Textures: 0' before running part 3.")


main()
