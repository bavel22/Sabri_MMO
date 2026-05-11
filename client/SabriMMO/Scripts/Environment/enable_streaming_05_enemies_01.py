"""Enable streaming: enemies batch 1/8 (alligator → cookie). Idempotent."""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

ENEMIES = [
    "alligator", "ambernite", "anacondaq", "andre", "angeling", "anopheles",
    "ant_egg", "antonio", "argiope", "argos", "aster", "baby_leopard",
    "baphomet_jr", "bathory", "black_mushroom", "blazzer", "blue_plant",
    "bogun", "boiled_rice", "brilight", "caramel", "choco", "chonchon",
    "coco", "condor", "cookie",
]


def enable_streaming(asset_path):
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"
    if not tex.get_editor_property("never_stream"):
        return "skipped"
    tex.set_editor_property("never_stream", False)
    eal.save_asset(clean)
    return "enabled"


def main():
    counts = {"enabled": 0, "skipped": 0, "failed": 0}
    start = time.time()
    grand = 0
    for enemy in ENEMIES:
        disk = os.path.join(ATLAS_ROOT, enemy)
        game = f"{GAME_ROOT}/{enemy}"
        pngs = sorted(glob.glob(os.path.join(disk, "*.png")))
        if not pngs:
            continue
        unreal.log(f"--- {enemy} ({len(pngs)}) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            counts[enable_streaming(f"{game}/{name}")] += 1
            grand += 1
            if grand % 25 == 0:
                unreal.log(f"  [{grand}] {counts}")
    unreal.log(f"=== Enemies 01 done in {time.time()-start:.1f}s. {counts} ===")


main()
