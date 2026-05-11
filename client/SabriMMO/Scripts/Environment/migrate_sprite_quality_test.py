"""TEST migration: applies canonical sprite atlas settings (2026-04-27)
to a small subset, enabling runtime Sprite Quality LOD.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_sprite_quality_test.py"

Canonical settings applied to each test atlas (see memory/feedback-sprite-texture-group-ui.md):
  - compression_settings              = TC_BC7
  - filter                            = TF_NEAREST
  - mip_gen_settings                  = TMGS_SIMPLE_AVERAGE
  - use_new_mip_filter                = True
  - do_scale_mips_for_alpha_coverage  = True
  - alpha_coverage_thresholds         = (0, 0, 0, 0.5)
  - max_texture_size                  = 0   (no cap → uniform mip pyramid)
  - never_stream                      = False  (lets LODBias actually free VRAM)
  - lod_group                         = TEXTUREGROUP_UI
  - srgb                              = True

After running, UE5 will recompile each affected texture in the background. This can
take ~30s-2min PER texture for the alpha-coverage mip generation. The migration
itself is fast (sets properties only); the long part is UE5's compile pass which
runs after each save_asset call. Watch the "Compiling Textures: N remaining"
indicator in the bottom-right of the editor.

Test scope:
  - priest_f body (17 atlases)
  - All 16 female weapon atlas folders (~272 atlases)
  - Hair style_01 female (17 atlases)
  - 5 enemies: poring, fabre, pupa, orc_warrior, zerom (~50 atlases)
  Total: ~356 atlases. Re-encode time roughly 2-4 hours.

After this script:
  1. Build the editor target (already up to date if you've built recently).
  2. Restart UE5 editor (so the new EnsureTextureLoaded() applies LODBias).
  3. Open Options > Video > Sprite Quality.
  4. Switch between Ultra/High/Medium/Low/Very Low to compare on the test entities.
  5. If quality looks good, run migrate_sprite_quality_full.py to do everything.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"


def collect_test_targets():
    """Build the list of (disk_path, asset_path) tuples to migrate."""
    targets = []

    # 1) priest_f body
    src = os.path.join(ATLAS_ROOT, "Body/priest_f")
    dst = f"{GAME_ROOT}/Body/priest_f"
    for png in sorted(glob.glob(os.path.join(src, "*.png"))):
        name = os.path.splitext(os.path.basename(png))[0]
        targets.append((src, f"{dst}/{name}"))

    # 2) Female weapons (all 16 types)
    weapon_root = os.path.join(ATLAS_ROOT, "Weapon")
    for wtype in sorted(os.listdir(weapon_root)):
        female_dir = os.path.join(weapon_root, wtype, "female")
        if not os.path.isdir(female_dir):
            continue
        for png in sorted(glob.glob(os.path.join(female_dir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((female_dir, f"{GAME_ROOT}/Weapon/{wtype}/female/{name}"))

    # 3) Hair style_01 female
    hair_dir = os.path.join(ATLAS_ROOT, "Hair/style_01/female")
    if os.path.isdir(hair_dir):
        for png in sorted(glob.glob(os.path.join(hair_dir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((hair_dir, f"{GAME_ROOT}/Hair/style_01/female/{name}"))

    # 4) Five test enemies
    for enemy in ["poring", "fabre", "pupa", "orc_warrior", "zerom"]:
        edir = os.path.join(ATLAS_ROOT, "Body/enemies", enemy)
        if not os.path.isdir(edir):
            unreal.log_warning(f"Missing enemy folder: {enemy}")
            continue
        for png in sorted(glob.glob(os.path.join(edir, "*.png"))):
            name = os.path.splitext(os.path.basename(png))[0]
            targets.append((edir, f"{GAME_ROOT}/Body/enemies/{enemy}/{name}"))

    return targets


def apply_quality_settings(asset_path):
    """Apply the canonical sprite atlas settings (2026-04-27). Idempotent.
    Returns 'configured', 'skipped', or 'failed'.

    See memory/feedback-sprite-texture-group-ui.md for the rule set.
    NOTE: UE5 Python strips the leading `b` from boolean UPROPERTY names
    (e.g. C++ `bDoScaleMipsForAlphaCoverage` → Python `do_scale_mips_for_alpha_coverage`).
    """
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"

    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"

    # Idempotent: skip if all canonical settings are already in place.
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


def preflight_check(targets):
    """Try the migration on a single asset first to catch property-name errors
    before processing all 356. Returns True if OK, False to abort."""
    if not targets:
        unreal.log_warning("No targets found — nothing to do.")
        return False

    _, first_asset = targets[0]
    unreal.log(f"Preflight: testing property access on {first_asset}")
    try:
        clean = first_asset.split(".")[0]
        if not eal.does_asset_exist(clean):
            unreal.log_warning(f"  First asset doesn't exist; skipping preflight: {clean}")
            return True  # not necessarily fatal — other assets may exist

        tex = unreal.load_asset(clean)
        if not tex or not isinstance(tex, unreal.Texture2D):
            unreal.log_warning(f"  First asset isn't a Texture2D; skipping preflight")
            return True

        # Probe each property we will set. Any error here aborts the run.
        _ = tex.get_editor_property("mip_gen_settings")
        _ = tex.get_editor_property("do_scale_mips_for_alpha_coverage")
        _ = tex.get_editor_property("alpha_coverage_thresholds")
        _ = tex.get_editor_property("use_new_mip_filter")
        _ = tex.get_editor_property("filter")
        _ = tex.get_editor_property("compression_settings")
        _ = tex.get_editor_property("lod_group")
        _ = tex.get_editor_property("never_stream")
        unreal.log("  All required properties exist on Texture2D — proceeding.")
        return True
    except Exception as e:
        unreal.log_error(f"PREFLIGHT FAILED: {e}")
        unreal.log_error("  Property names may have changed in this UE5 version.")
        unreal.log_error("  Run this in the Python console to list available properties:")
        unreal.log_error(f"    tex = unreal.load_asset('{clean}')")
        unreal.log_error(f"    print([p for p in dir(tex) if 'mip' in p.lower() or 'alpha' in p.lower()])")
        return False


def main():
    targets = collect_test_targets()
    unreal.log(f"=== Sprite Quality Migration: TEST SUBSET ===")
    unreal.log(f"Targeting {len(targets)} atlases.")
    unreal.log(f"Each will be reconfigured for runtime LOD bias.")
    unreal.log(f"UE5 will recompile textures in the background after this finishes.")
    unreal.log(f"Watch the 'Compiling Textures' indicator — full recompile may take hours.")
    unreal.log(f"")

    if not preflight_check(targets):
        unreal.log_error("Aborting migration. Fix property names and re-run.")
        return

    counts = {"configured": 0, "skipped": 0, "failed": 0}
    start = time.time()

    for i, (src_dir, asset_path) in enumerate(targets, start=1):
        result = apply_quality_settings(asset_path)
        counts[result] += 1
        if i % 25 == 0:
            unreal.log(f"  [{i}/{len(targets)}] configured={counts['configured']} "
                f"skipped={counts['skipped']} failed={counts['failed']}")

    elapsed = time.time() - start
    unreal.log(f"")
    unreal.log(f"=== Migration complete in {elapsed:.1f}s ===")
    unreal.log(f"  Configured:  {counts['configured']}")
    unreal.log(f"  Skipped:     {counts['skipped']} (already migrated or asset missing)")
    unreal.log(f"  Failed:      {counts['failed']}")
    unreal.log(f"")
    unreal.log(f"NEXT: wait for UE5 to finish 'Compiling Textures' (bottom-right indicator).")
    unreal.log(f"Then restart the editor and test the new Options > Video > Sprite Quality slider.")


main()
