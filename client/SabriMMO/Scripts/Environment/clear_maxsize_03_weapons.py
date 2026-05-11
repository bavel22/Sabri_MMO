"""Clear MaxTextureSize: all weapons (~289 atlases).

Run:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/clear_maxsize_03_weapons.py"

Walks every .png under Sprites/Atlases/Weapon/ recursively
(handles male/female subfolders + flat layout).
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
DISK_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Weapon"


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

    for root_dir, _, files in os.walk(DISK_ROOT):
        pngs = sorted(f for f in files if f.lower().endswith(".png"))
        if not pngs:
            continue
        rel = os.path.relpath(root_dir, DISK_ROOT).replace("\\", "/")
        game_dir = GAME_ROOT if rel == "." else f"{GAME_ROOT}/{rel}"
        unreal.log(f"--- {rel if rel != '.' else 'Weapon/'} ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(png)[0]
            counts[clear_max_texture_size(f"{game_dir}/{name}")] += 1
            grand += 1
            if grand % 25 == 0:
                unreal.log(f"  [{grand}] {counts}")

    unreal.log(f"=== Weapons done in {time.time()-start:.1f}s. {counts} ===")
    unreal.log("Wait for 'Compiling Textures: 0' before running script 04.")


main()
