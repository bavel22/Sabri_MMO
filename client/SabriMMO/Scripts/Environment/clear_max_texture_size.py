"""Clear MaxTextureSize on every sprite atlas (set to 0 = no cap).

Why: a fixed MaxTextureSize=4096 makes non-square atlases (walk/attack — taller
than 8192) downscale non-uniformly, producing smaller cells than idle. The result
is sprites that look crisp when idle but blurry when walking/attacking.

Removing MaxTextureSize lets the mip pyramid handle resolution control. Every
atlas has the same source cell size (1024x1024), so every mip level produces
uniform cells across all animations. The Sprite Quality slider's LODBias picks
which mip is loaded.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/clear_max_texture_size.py"

Idempotent — atlases already at MaxTextureSize=0 are skipped.
Safe to interrupt with Ctrl+C and resume.
After it finishes, wait for "Compiling Textures: 0" before testing in-game.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"

# How often to log progress
LOG_EVERY_N = 50


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
        return "skipped"  # already cleared

    tex.set_editor_property("max_texture_size", 0)
    eal.save_asset(clean)
    return "cleared"


def collect_all_atlas_paths():
    """Walk every PNG under Sprites/Atlases/ and return matching asset paths."""
    targets = []
    for root_dir, _, files in os.walk(ATLAS_ROOT):
        for f in files:
            if not f.lower().endswith(".png"):
                continue
            png_full = os.path.join(root_dir, f)
            # Convert disk path to UE5 asset path
            rel = os.path.relpath(png_full, ATLAS_ROOT).replace("\\", "/")
            asset_subpath = os.path.splitext(rel)[0]
            asset_path = f"{GAME_ROOT}/{asset_subpath}"
            targets.append(asset_path)
    return targets


def main():
    targets = collect_all_atlas_paths()
    unreal.log("=" * 60)
    unreal.log(f"CLEAR MAX TEXTURE SIZE — {len(targets)} atlases")
    unreal.log("=" * 60)
    unreal.log("Setting MaxTextureSize=0 on every sprite atlas.")
    unreal.log("UE5 will recompile each texture in the background after save.")
    unreal.log("")

    counts = {"cleared": 0, "skipped": 0, "failed": 0}
    start = time.time()

    for i, asset_path in enumerate(targets, start=1):
        result = clear_max_texture_size(asset_path)
        counts[result] += 1
        if i % LOG_EVERY_N == 0 or i == len(targets):
            elapsed = time.time() - start
            unreal.log(f"  [{i}/{len(targets)}] {counts}  ({elapsed:.1f}s)")

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Done in {elapsed:.1f}s.")
    unreal.log(f"  Cleared: {counts['cleared']}")
    unreal.log(f"  Skipped: {counts['skipped']} (already 0 or asset missing)")
    unreal.log(f"  Failed:  {counts['failed']}")
    unreal.log("=" * 60)
    unreal.log("Wait for 'Compiling Textures: 0' (bottom-right) before testing.")
    unreal.log("Then restart the editor.")


main()
