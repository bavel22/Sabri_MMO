"""Enable streaming: weapons A-K (~150 atlases). Idempotent.
Covers: axe_1h, axe_2h, book, bow, dagger, instrument, katar, knuckle.
Recursive across male/female subfolders."""
import unreal
import os
import time

eal = unreal.EditorAssetLibrary
DISK_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Weapon"

WEAPON_TYPES = [
    "axe_1h", "axe_2h", "book", "bow",
    "dagger", "instrument", "katar", "knuckle",
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
    for wtype in WEAPON_TYPES:
        type_dir = os.path.join(DISK_ROOT, wtype)
        if not os.path.isdir(type_dir):
            unreal.log_warning(f"  {wtype}: folder missing — skipping")
            continue
        for sub_dir, _, files in os.walk(type_dir):
            pngs = sorted(f for f in files if f.lower().endswith(".png"))
            if not pngs:
                continue
            rel = os.path.relpath(sub_dir, DISK_ROOT).replace("\\", "/")
            game_sub = f"{GAME_ROOT}/{rel}"
            unreal.log(f"--- {rel} ({len(pngs)} atlases) ---")
            for png in pngs:
                name = os.path.splitext(png)[0]
                counts[enable_streaming(f"{game_sub}/{name}")] += 1
                grand += 1
                if grand % 25 == 0:
                    unreal.log(f"  [{grand}] {counts}")
    unreal.log(f"=== Weapons A-K done in {time.time()-start:.1f}s. {counts} ===")


main()
