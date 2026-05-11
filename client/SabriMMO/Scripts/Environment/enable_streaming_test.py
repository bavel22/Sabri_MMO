"""TEST script: enable streaming (never_stream=false) on priest_f and poring atlases ONLY.

Use this to verify streaming behavior on a small set before applying to all atlases.

What this changes:
  - never_stream: True -> False (the ONLY change)

Nothing else is touched. mip_gen_settings, alpha_coverage, BC7, filter, lod_group,
LODBias, max_texture_size all stay as-is.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/enable_streaming_test.py"

After it finishes:
  1. Wait for "Compiling Textures: 0" (should be quick — ~22 atlases)
  2. Restart the editor
  3. Set Options > Video > Sprite Quality to Low or Very Low
  4. Walk to Prontera South — focus on priest_f character + porings
  5. Watch Task Manager > Performance > GPU memory: should drop massively
     because only the lowest mips of priest_f + poring atlases are now resident
  6. Watch for pop-in: when you first see a poring or your priest_f sprite
     animates rapidly, does the texture briefly look low-res then snap to crisp?
     Or does it stay crisp throughout?

If pop-in is invisible or acceptable, run the full version on every atlas.
If pop-in is bad, we'll switch to baking LODBias into the import settings instead.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"

# Test scope: priest_f body + poring enemy
TARGETS = [
    (os.path.join(ATLAS_ROOT, "Body/priest_f"),       f"{GAME_ROOT}/Body/priest_f"),
    (os.path.join(ATLAS_ROOT, "Body/enemies/poring"), f"{GAME_ROOT}/Body/enemies/poring"),
]


def enable_streaming(asset_path):
    """Set never_stream=False. Returns 'enabled', 'skipped', or 'failed'."""
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"

    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"

    if not tex.get_editor_property("never_stream"):
        return "skipped"  # already streaming-enabled

    unreal.log(f"  '{os.path.basename(asset_path)}': never_stream True -> False")
    tex.set_editor_property("never_stream", False)
    eal.save_asset(clean)
    return "enabled"


def main():
    counts = {"enabled": 0, "skipped": 0, "failed": 0}
    start = time.time()
    grand = 0

    unreal.log("=" * 60)
    unreal.log("ENABLE STREAMING TEST — priest_f + poring")
    unreal.log("=" * 60)

    for disk_dir, game_dir in TARGETS:
        pngs = sorted(glob.glob(os.path.join(disk_dir, "*.png")))
        unreal.log(f"--- {os.path.basename(disk_dir)} ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            counts[enable_streaming(f"{game_dir}/{name}")] += 1
            grand += 1

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Done in {elapsed:.1f}s. {counts}")
    unreal.log("=" * 60)
    unreal.log("Next steps:")
    unreal.log("  1. Wait for 'Compiling Textures: 0'")
    unreal.log("  2. Restart the editor")
    unreal.log("  3. Test in-game with Sprite Quality at Low or Very Low")
    unreal.log("  4. Check VRAM in Task Manager — should drop dramatically")
    unreal.log("  5. Watch for pop-in on priest_f sprite + porings")


main()
