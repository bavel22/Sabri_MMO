"""Enable streaming part 2b of 3: weapons L-Z (last 8 types). FINAL batch.

Scope (~150 atlases):
  - mace, rod, spear_1h, spear_2h, staff, sword_1h, sword_2h, whip
  - Recursive (both male/female subfolders if present)

Only changes never_stream. Nothing else is touched. Idempotent.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/enable_streaming_prt_south_part_2b.py"

After it finishes:
  1. Wait for "Compiling Textures: 0" (~5 min)
  2. Restart the editor
  3. Restart the server (so prt_south despawn takes effect)
  4. Walk into prt_south, equip weapons, check VRAM in Task Manager
  5. Watch for pop-in on weapon swaps + sprite animations
"""
import unreal
import os
import time

eal = unreal.EditorAssetLibrary
DISK_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Weapon"

WEAPON_TYPES = [
    "mace", "rod", "spear_1h", "spear_2h",
    "staff", "sword_1h", "sword_2h", "whip",
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

    unreal.log("=" * 60)
    unreal.log("ENABLE STREAMING — Part 2b (weapons L-Z) FINAL")
    unreal.log("=" * 60)

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

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Part 2b done in {elapsed:.1f}s. {counts}")
    unreal.log("=" * 60)
    unreal.log("ALL STREAMING ENABLE SCRIPTS COMPLETE.")
    unreal.log("Wait for 'Compiling Textures: 0', restart editor + server, test in prt_south.")


main()
