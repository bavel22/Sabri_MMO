"""Sprite quality migration — TEST PART 6 of 7: hair style_01 female.

Run:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_test_06_hair.py"

After this finishes, wait for "Compiling Textures: 0" before running part 7.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"

DISK_DIR = os.path.join(ATLAS_ROOT, "Hair/style_01/female")
GAME_DIR = f"{GAME_ROOT}/Hair/style_01/female"


def apply_quality_settings(asset_path):
    """Apply the canonical sprite atlas settings (2026-04-27). Idempotent.
    See memory/feedback-sprite-texture-group-ui.md for the rule set."""
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"
    if (tex.get_editor_property("mip_gen_settings") ==
            unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE
            and tex.get_editor_property("do_scale_mips_for_alpha_coverage")
            and tex.get_editor_property("use_new_mip_filter")
            and tex.get_editor_property("max_texture_size") == 0
            and not tex.get_editor_property("never_stream")):
        return "skipped"
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
    return "configured"


def main():
    pngs = sorted(glob.glob(os.path.join(DISK_DIR, "*.png")))
    unreal.log(f"=== TEST 6/7: hair style_01 female ({len(pngs)} atlases) ===")
    if not pngs:
        unreal.log_warning(f"No PNGs in {DISK_DIR}")
        return

    counts = {"configured": 0, "skipped": 0, "failed": 0}
    start = time.time()
    for i, png in enumerate(pngs, 1):
        name = os.path.splitext(os.path.basename(png))[0]
        result = apply_quality_settings(f"{GAME_DIR}/{name}")
        counts[result] += 1
        if i % 5 == 0 or i == len(pngs):
            unreal.log(f"  [{i}/{len(pngs)}] {counts}")

    elapsed = time.time() - start
    unreal.log(f"--- Part 6 done in {elapsed:.1f}s. {counts} ---")
    unreal.log(f"Wait for 'Compiling Textures: 0' before running part 7.")


main()
