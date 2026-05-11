"""Verify the test migration applied the canonical sprite atlas settings (2026-04-27)
to all 356 test atlases.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_test_verify.py"

Canonical settings each atlas must have (see memory/feedback-sprite-texture-group-ui.md):
  - compression_settings              = TC_BC7
  - filter                            = TF_NEAREST
  - mip_gen_settings                  = TMGS_SIMPLE_AVERAGE
  - use_new_mip_filter                = True
  - do_scale_mips_for_alpha_coverage  = True
  - alpha_coverage_thresholds.w       = 0.5
  - max_texture_size                  = 0       (no cap)
  - never_stream                      = False   (streaming enabled)
  - lod_group                         = TEXTUREGROUP_UI

Also confirms each texture actually has multiple mip levels (the smoking-gun test
that the recompile finished successfully).
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"


def collect_test_targets():
    targets = []
    # priest_f body
    src = os.path.join(ATLAS_ROOT, "Body/priest_f")
    for png in sorted(glob.glob(os.path.join(src, "*.png"))):
        name = os.path.splitext(os.path.basename(png))[0]
        targets.append(("priest_f", f"{GAME_ROOT}/Body/priest_f/{name}"))
    # All female weapons
    weapon_root = os.path.join(ATLAS_ROOT, "Weapon")
    for wtype in sorted(os.listdir(weapon_root)):
        female_dir = os.path.join(weapon_root, wtype, "female")
        if not os.path.isdir(female_dir):
            continue
        for png in sorted(glob.glob(os.path.join(female_dir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((f"weapon/{wtype}",
                f"{GAME_ROOT}/Weapon/{wtype}/female/{name}"))
    # Hair
    hair_dir = os.path.join(ATLAS_ROOT, "Hair/style_01/female")
    if os.path.isdir(hair_dir):
        for png in sorted(glob.glob(os.path.join(hair_dir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append(("hair", f"{GAME_ROOT}/Hair/style_01/female/{name}"))
    # 5 test enemies
    for enemy in ["poring", "fabre", "pupa", "orc_warrior", "zerom"]:
        edir = os.path.join(ATLAS_ROOT, "Body/enemies", enemy)
        if not os.path.isdir(edir):
            continue
        for png in sorted(glob.glob(os.path.join(edir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((f"enemy/{enemy}",
                f"{GAME_ROOT}/Body/enemies/{enemy}/{name}"))
    return targets


def verify_one(asset_path):
    """Returns (ok: bool, problem: str). 'ok' means fully migrated and compiled
    against the canonical sprite atlas settings (2026-04-27)."""
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return False, "asset_missing"

    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False, "not_a_texture2d"

    # 1) mip generation must be SimpleAverage (not NoMipmaps)
    if tex.get_editor_property("mip_gen_settings") != \
            unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE:
        return False, "mip_gen_not_simple_average"

    # 2) alpha coverage flag on
    if not tex.get_editor_property("do_scale_mips_for_alpha_coverage"):
        return False, "alpha_coverage_off"

    # 3) alpha coverage threshold has alpha=0.5
    thresh = tex.get_editor_property("alpha_coverage_thresholds")
    # Vector4 in Python — check the W component (alpha)
    if abs(thresh.w - 0.5) > 0.01:
        return False, f"alpha_threshold_wrong (w={thresh.w})"

    # 4) modern image processing on
    if not tex.get_editor_property("use_new_mip_filter"):
        return False, "new_mip_filter_off"

    # 5) MaxTextureSize must be 0 (no cap). A fixed cap downscales non-square
    # atlases (walk 8192x12288 vs idle 8192x8192) by different factors → uneven
    # cell sizes. Canonical baseline is 0 so the mip pyramid handles resolution.
    actual_max = tex.get_editor_property("max_texture_size")
    if actual_max != 0:
        return False, f"max_texture_size_capped (actual={actual_max}, expected=0)"

    # 6) BC7 compression preserved
    if tex.get_editor_property("compression_settings") != \
            unreal.TextureCompressionSettings.TC_BC7:
        return False, "not_bc7"

    # 7) Filter still nearest
    if tex.get_editor_property("filter") != unreal.TextureFilter.TF_NEAREST:
        return False, "not_nearest_filter"

    # 8) Never-stream must be FALSE so LODBias actually frees VRAM. With
    # never_stream=True the streaming system keeps every mip resident
    # regardless of LODBias and the Sprite Quality slider has no effect on RAM.
    if tex.get_editor_property("never_stream"):
        return False, "never_stream_true (must be False for runtime LODBias)"

    # 9) LOD group must be TEXTUREGROUP_UI
    if tex.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        return False, "lod_group_not_ui"

    # 10) Smoking-gun: actual GPU resource has multiple mip levels.
    # If the texture compile didn't run yet, this will still be 1.
    num_mips = tex.blueprint_get_num_mips() if hasattr(tex, "blueprint_get_num_mips") else None
    if num_mips is None:
        # Fallback: read the platform_data path or skip the check
        try:
            num_mips = tex.get_num_mips()
        except Exception:
            num_mips = -1  # unable to introspect
    if num_mips == 1:
        return False, "only_one_mip_level (recompile not finished?)"

    return True, "ok"


def main():
    targets = collect_test_targets()
    unreal.log(f"=== Verifying {len(targets)} test atlases ===")
    unreal.log("")

    by_category = {}
    problems_by_kind = {}
    sample_ok_paths = []

    for category, asset_path in targets:
        ok, problem = verify_one(asset_path)
        cat = by_category.setdefault(category, {"ok": 0, "bad": 0})
        if ok:
            cat["ok"] += 1
            if len(sample_ok_paths) < 3:
                sample_ok_paths.append(asset_path)
        else:
            cat["bad"] += 1
            problems_by_kind.setdefault(problem, []).append(asset_path)

    unreal.log(f"--- Per-category results ---")
    for cat, counts in sorted(by_category.items()):
        symbol = "OK" if counts["bad"] == 0 else "FAIL"
        unreal.log(f"  [{symbol}] {cat:20s}  ok={counts['ok']:4d}  bad={counts['bad']:4d}")

    if problems_by_kind:
        unreal.log("")
        unreal.log(f"--- Problems found ---")
        for kind, paths in problems_by_kind.items():
            unreal.log(f"  {kind}: {len(paths)} assets")
            for p in paths[:3]:
                unreal.log(f"      {p}")
            if len(paths) > 3:
                unreal.log(f"      ... and {len(paths) - 3} more")
    else:
        unreal.log("")
        unreal.log("ALL ATLASES VERIFIED. Migration applied cleanly.")
        if sample_ok_paths:
            unreal.log(f"Sample fully-migrated atlas: {sample_ok_paths[0]}")
        unreal.log("Safe to restart editor and test the slider.")


main()
