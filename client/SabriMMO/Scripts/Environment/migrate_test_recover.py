"""RECOVERY: re-import every test atlas from its PNG with the CANONICAL settings.

Use this when the migration scripts produced broken (empty/transparent) atlases.
This script:
  1. Deletes the existing .uasset for each test atlas
  2. Re-imports the PNG with the canonical sprite atlas settings (2026-04-27):
       Compression                   = BC7
       Filter                        = Nearest
       Mip Gen                       = SimpleAverage
       Use New Mip Filter            = True
       Do Scale Mips For Alpha Cov.  = True
       Alpha Coverage Thresholds     = (0, 0, 0, 0.5)
       Maximum Texture Size          = 0  (no cap)
       Never Stream                  = False
       LOD Group                     = TEXTUREGROUP_UI
       sRGB                          = True

These match memory/feedback-sprite-texture-group-ui.md and are required for the
runtime Sprite Quality slider (LODBias 0-4), ZonePreloadSubsystem async-load,
and Path C deferred equipment swap.

Source PNGs on disk are untouched.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_test_recover.py"

Processes ~356 atlases. Watch the "Compiling Textures" indicator finish before
launching the game.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"


def collect_targets():
    """Same scope as the test migration: priest_f, female weapons, hair, 5 enemies."""
    targets = []  # (png_path, dest_dir, asset_path)

    # priest_f body
    src = os.path.join(ATLAS_ROOT, "Body/priest_f")
    for png in sorted(glob.glob(os.path.join(src, "*.png"))):
        name = os.path.splitext(os.path.basename(png))[0]
        targets.append((png, f"{GAME_ROOT}/Body/priest_f",
            f"{GAME_ROOT}/Body/priest_f/{name}"))

    # All female weapons
    weapon_root = os.path.join(ATLAS_ROOT, "Weapon")
    for wtype in sorted(os.listdir(weapon_root)):
        female_dir = os.path.join(weapon_root, wtype, "female")
        if not os.path.isdir(female_dir):
            continue
        for png in sorted(glob.glob(os.path.join(female_dir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((png, f"{GAME_ROOT}/Weapon/{wtype}/female",
                f"{GAME_ROOT}/Weapon/{wtype}/female/{name}"))

    # Hair style_01 female
    hair_dir = os.path.join(ATLAS_ROOT, "Hair/style_01/female")
    if os.path.isdir(hair_dir):
        for png in sorted(glob.glob(os.path.join(hair_dir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((png, f"{GAME_ROOT}/Hair/style_01/female",
                f"{GAME_ROOT}/Hair/style_01/female/{name}"))

    # 5 test enemies
    for enemy in ["poring", "fabre", "pupa", "orc_warrior", "zerom"]:
        edir = os.path.join(ATLAS_ROOT, "Body/enemies", enemy)
        if not os.path.isdir(edir):
            continue
        for png in sorted(glob.glob(os.path.join(edir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((png, f"{GAME_ROOT}/Body/enemies/{enemy}",
                f"{GAME_ROOT}/Body/enemies/{enemy}/{name}"))

    return targets


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


def recover_one(png_path, dest_dir, asset_path):
    """Delete old asset, re-import from PNG, apply canonical settings."""
    clean = asset_path.split(".")[0]

    # 1) Delete old (potentially broken) asset
    if eal.does_asset_exist(clean):
        eal.delete_asset(clean)

    # 2) Re-import the PNG
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("filename", png_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    # 3) Apply canonical sprite atlas settings
    if not eal.does_asset_exist(clean):
        return "import_failed"
    apply_canonical_settings(asset_path)
    return "recovered"


def main():
    targets = collect_targets()
    unreal.log("=" * 60)
    unreal.log(f"RECOVERY: re-importing {len(targets)} test atlases from PNG.")
    unreal.log("Canonical settings (BC7, Nearest, SimpleAverage mips, alpha coverage W=0.5,")
    unreal.log("MaxTextureSize=0, NeverStream=False, TEXTUREGROUP_UI).")
    unreal.log("=" * 60)

    counts = {"recovered": 0, "import_failed": 0, "skipped": 0}
    start = time.time()

    for i, (png_path, dest_dir, asset_path) in enumerate(targets, start=1):
        if not os.path.exists(png_path):
            counts["skipped"] += 1
            continue
        result = recover_one(png_path, dest_dir, asset_path)
        counts[result] = counts.get(result, 0) + 1
        if i % 25 == 0 or i == len(targets):
            unreal.log(f"  [{i}/{len(targets)}] {counts}")

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Recovery done in {elapsed:.1f}s. {counts}")
    unreal.log("=" * 60)
    unreal.log("Wait for 'Compiling Textures: 0' before launching the game.")
    unreal.log("After restart, all test atlases use the canonical Sprite Quality baseline.")


main()
