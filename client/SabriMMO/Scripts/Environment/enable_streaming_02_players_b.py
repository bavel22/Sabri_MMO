"""Enable streaming: player classes M-Z (~180 atlases). Idempotent."""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body"

CLASSES = [
    "mage_f", "mage_m", "merchant_f", "merchant_m",
    "monk_f", "monk_m", "novice_f", "novice_m",
    "priest_f", "priest_m", "rogue_f", "rogue_m",
    "sage_f", "sage_m", "swordsman_f", "swordsman_m",
    "thief_f", "thief_m", "wizard_f", "wizard_m",
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
    for cls in CLASSES:
        disk = os.path.join(ATLAS_ROOT, cls)
        game = f"{GAME_ROOT}/{cls}"
        pngs = sorted(glob.glob(os.path.join(disk, "*.png")))
        unreal.log(f"--- {cls} ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            counts[enable_streaming(f"{game}/{name}")] += 1
            grand += 1
            if grand % 25 == 0:
                unreal.log(f"  [{grand}] {counts}")
    unreal.log(f"=== Players M-Z done in {time.time()-start:.1f}s. {counts} ===")


main()
