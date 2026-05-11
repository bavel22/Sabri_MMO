"""Sprite quality migration — TEST PART 2 of 7: weapons batch A (female).

Covers: axe_1h, axe_2h, book, bow (4 weapon types × 17 atlases each = ~68 atlases).

Run:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_test_02_weapons_a.py"

After this finishes, wait for "Compiling Textures: 0" before running part 3.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"

WEAPON_TYPES = ["axe_1h", "axe_2h", "book", "bow"]


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
    counts = {"configured": 0, "skipped": 0, "failed": 0}
    start = time.time()
    grand_total = 0

    for wtype in WEAPON_TYPES:
        disk = os.path.join(ATLAS_ROOT, "Weapon", wtype, "female")
        game = f"{GAME_ROOT}/Weapon/{wtype}/female"
        pngs = sorted(glob.glob(os.path.join(disk, "*.png")))
        unreal.log(f"--- {wtype}/female ({len(pngs)} atlases) ---")
        for i, png in enumerate(pngs, 1):
            name = os.path.splitext(os.path.basename(png))[0]
            result = apply_quality_settings(f"{game}/{name}")
            counts[result] += 1
            grand_total += 1
            if grand_total % 10 == 0:
                unreal.log(f"  [{grand_total}] {counts}")

    elapsed = time.time() - start
    unreal.log(f"=== Part 2 done in {elapsed:.1f}s. {counts} ===")
    unreal.log(f"Wait for 'Compiling Textures: 0' before running part 3.")


main()
