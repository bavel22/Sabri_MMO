"""TEST script: clear MaxTextureSize on poring atlases ONLY.

Use this to verify the change works correctly on a small set before running
clear_max_texture_size.py on all 2,206 atlases.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/clear_max_texture_size_test_poring.py"

Idempotent — already-zero atlases are skipped. Logic is identical to the full
script; if this works, the full script will work the same way.

After it finishes:
  1. Wait for "Compiling Textures: 0" (bottom-right indicator)
  2. Restart the editor
  3. Walk to a zone with porings (Prontera South)
  4. Verify: poring sprite renders correctly at all Sprite Quality tiers
  5. Verify: walking poring looks the same crispness as idle poring
  6. Open priest_f_walking.uasset for comparison — non-poring atlases should
     still have the old MaxTextureSize=4096 (only poring was changed)
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
DISK_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/poring"
GAME_DIR = "/Game/SabriMMO/Sprites/Atlases/Body/enemies/poring"


def clear_max_texture_size(asset_path):
    """Set MaxTextureSize to 0 on a single texture. Returns 'cleared', 'skipped', or 'failed'."""
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"

    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"

    current = tex.get_editor_property("max_texture_size")
    if current == 0:
        return "skipped"

    unreal.log(f"  '{os.path.basename(asset_path)}': MaxTextureSize {current} -> 0")
    tex.set_editor_property("max_texture_size", 0)
    eal.save_asset(clean)
    return "cleared"


def main():
    pngs = sorted(glob.glob(os.path.join(DISK_DIR, "*.png")))
    unreal.log("=" * 60)
    unreal.log(f"TEST: clear MaxTextureSize on poring ({len(pngs)} atlases)")
    unreal.log("=" * 60)

    if not pngs:
        unreal.log_error(f"No PNGs found in {DISK_DIR}")
        return

    counts = {"cleared": 0, "skipped": 0, "failed": 0}
    start = time.time()

    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        result = clear_max_texture_size(f"{GAME_DIR}/{name}")
        counts[result] += 1

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Test done in {elapsed:.1f}s. {counts}")
    unreal.log("=" * 60)
    unreal.log("Next steps:")
    unreal.log("  1. Wait for 'Compiling Textures: 0' (bottom-right of editor)")
    unreal.log("  2. Restart the editor")
    unreal.log("  3. Walk to Prontera South, look at porings")
    unreal.log("  4. Cycle Options > Video > Sprite Quality through all 5 tiers")
    unreal.log("  5. Verify walking poring looks as crisp as idle poring")
    unreal.log("If everything looks good, run clear_max_texture_size.py for the full project.")


main()
