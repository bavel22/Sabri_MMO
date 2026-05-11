"""Clear MaxTextureSize: player classes A-K (~170 atlases).

Identical logic to the test script. Sets MaxTextureSize=0 on each atlas.
Idempotent. Safe to interrupt.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/clear_maxsize_01_players_a.py"

Wait for "Compiling Textures: 0" before running the next batch.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body"

CLASSES = [
    "acolyte_f", "acolyte_m", "alchemist_f", "alchemist_m",
    "archer_f", "archer_m", "assassin_f", "assassin_m",
    "bard_m", "blacksmith_f", "blacksmith_m",
    "crusader_f", "crusader_m", "dancer_f",
    "hunter_f", "hunter_m", "knight_f", "knight_m",
]


def clear_max_texture_size(asset_path):
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"
    if tex.get_editor_property("max_texture_size") == 0:
        return "skipped"
    tex.set_editor_property("max_texture_size", 0)
    eal.save_asset(clean)
    return "cleared"


def main():
    counts = {"cleared": 0, "skipped": 0, "failed": 0}
    start = time.time()
    grand = 0
    for cls in CLASSES:
        disk = os.path.join(ATLAS_ROOT, cls)
        game = f"{GAME_ROOT}/{cls}"
        pngs = sorted(glob.glob(os.path.join(disk, "*.png")))
        unreal.log(f"--- {cls} ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            counts[clear_max_texture_size(f"{game}/{name}")] += 1
            grand += 1
            if grand % 25 == 0:
                unreal.log(f"  [{grand}] {counts}")
    unreal.log(f"=== Players A-K done in {time.time()-start:.1f}s. {counts} ===")
    unreal.log("Wait for 'Compiling Textures: 0' before running script 02.")


main()
