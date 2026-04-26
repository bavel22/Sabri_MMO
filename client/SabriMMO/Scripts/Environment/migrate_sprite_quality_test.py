"""TEST migration: enables runtime sprite quality LOD on a small subset.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_sprite_quality_test.py"

What it does to each test atlas:
  - mip_gen_settings = TMGS_ALPHA_COVERAGE (preserves binary alpha when generating mips)
  - alpha_coverage_thresholds = (0, 0, 0, 0.5)
  - never_stream = True (was already on, kept for clarity)
  - lod_group = TEXTUREGROUP_UI (kept — switch to TEXTUREGROUP_Project01 for production)
  - Compression / filter / no-mipmaps NOT changed (BC7 + Nearest are correct)

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
  4. Switch between Ultra/High/Medium/Low to compare on the test entities.
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
    """Add mip generation settings for runtime LOD scaling.
    Returns 'configured', 'skipped', or 'failed'."""
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"

    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"

    # Already migrated? Skip the modify+save (saves recompile time).
    current_mip = tex.get_editor_property("mip_gen_settings")
    if current_mip == unreal.TextureMipGenSettings.TMGS_ALPHA_COVERAGE:
        return "skipped"

    # New: alpha-preserving mip generation for runtime LOD bias
    tex.set_editor_property("mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_ALPHA_COVERAGE)
    tex.set_editor_property("alpha_coverage_thresholds",
        unreal.LinearColor(0.0, 0.0, 0.0, 0.5))

    # Re-affirm settings already in place (idempotent)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("never_stream", True)

    eal.save_asset(clean)
    return "configured"


def main():
    targets = collect_test_targets()
    unreal.log(f"=== Sprite Quality Migration: TEST SUBSET ===")
    unreal.log(f"Targeting {len(targets)} atlases.")
    unreal.log(f"Each will be reconfigured for runtime LOD bias.")
    unreal.log(f"UE5 will recompile textures in the background after this finishes.")
    unreal.log(f"Watch the 'Compiling Textures' indicator — full recompile may take hours.")
    unreal.log(f"")

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
