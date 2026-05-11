"""Clear MaxTextureSize: enemies batch 2/8 (cookie_xmas → fire_crystal).

Run:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/clear_maxsize_06_enemies_02.py"
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

ENEMIES = [
    "cookie_xmas", "cornutus", "crab", "creamy", "deniro", "desert_wolf",
    "desert_wolf_b", "deviace", "deviling", "deviruchi", "dokebi",
    "dragon_egg", "dragon_fly", "drainliar", "drops", "drosera", "dryad",
    "dustiness", "earth_crystal", "eclipse", "eggyra", "elder_wilow",
    "explosion", "fabre", "farmiliar", "fire_crystal",
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
    for enemy in ENEMIES:
        disk = os.path.join(ATLAS_ROOT, enemy)
        game = f"{GAME_ROOT}/{enemy}"
        pngs = sorted(glob.glob(os.path.join(disk, "*.png")))
        if not pngs:
            unreal.log_warning(f"  no PNGs in {enemy}")
            continue
        unreal.log(f"--- {enemy} ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            counts[clear_max_texture_size(f"{game}/{name}")] += 1
            grand += 1
            if grand % 25 == 0:
                unreal.log(f"  [{grand}] {counts}")
    unreal.log(f"=== Enemies 02 done in {time.time()-start:.1f}s. {counts} ===")
    unreal.log("Wait for 'Compiling Textures: 0' before running script 07.")


main()
