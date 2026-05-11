"""Enable streaming: Hair + HeadgearTop/Mid/Low + Garment + Shield (~50 atlases). Idempotent."""
import unreal
import os
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"

LAYER_DIRS = ["Hair", "HeadgearTop", "HeadgearMid", "HeadgearLow", "Garment", "Shield"]


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
    for layer in LAYER_DIRS:
        layer_disk = os.path.join(ATLAS_ROOT, layer)
        layer_game = f"{GAME_ROOT}/{layer}"
        if not os.path.isdir(layer_disk):
            unreal.log(f"--- {layer}/ (not present, skipping) ---")
            continue
        for sub_dir, _, files in os.walk(layer_disk):
            pngs = sorted(f for f in files if f.lower().endswith(".png"))
            if not pngs:
                continue
            rel = os.path.relpath(sub_dir, layer_disk).replace("\\", "/")
            game_dir = layer_game if rel == "." else f"{layer_game}/{rel}"
            unreal.log(f"--- {layer}/{rel if rel != '.' else ''} ({len(pngs)} atlases) ---")
            for png in pngs:
                name = os.path.splitext(png)[0]
                counts[enable_streaming(f"{game_dir}/{name}")] += 1
                grand += 1
                if grand % 25 == 0:
                    unreal.log(f"  [{grand}] {counts}")
    unreal.log(f"=== Hair/Headgear done in {time.time()-start:.1f}s. {counts} ===")


main()
