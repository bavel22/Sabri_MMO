"""Enable streaming part 1 of 2: priest_f + prt_south enemies + hair + egg shell hat.

Scope (~120 atlases):
  - Player class: priest_f body
  - 7 prt_south enemies: poring, fabre, pupa, lunatic, drops, chonchon, picky
  - All hair atlases (recursive, every gender)
  - HeadgearTop/egg_shell_hat (recursive, every gender)

Only changes never_stream. Nothing else is touched. Idempotent.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/enable_streaming_prt_south_part_1.py"

Wait for "Compiling Textures: 0" before running part 2 (weapons).
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"


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


def process_folder(disk_dir, game_dir, label, counts):
    if not os.path.isdir(disk_dir):
        unreal.log_warning(f"  {label}: folder missing — skipping")
        return
    pngs = sorted(glob.glob(os.path.join(disk_dir, "*.png")))
    unreal.log(f"--- {label} ({len(pngs)} atlases) ---")
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        counts[enable_streaming(f"{game_dir}/{name}")] += 1


def process_recursive(disk_root, game_root, label, counts):
    if not os.path.isdir(disk_root):
        unreal.log_warning(f"  {label}: folder missing — skipping")
        return
    total = 0
    for sub_dir, _, files in os.walk(disk_root):
        pngs = sorted(f for f in files if f.lower().endswith(".png"))
        if not pngs:
            continue
        rel = os.path.relpath(sub_dir, disk_root).replace("\\", "/")
        game_sub = game_root if rel == "." else f"{game_root}/{rel}"
        for png in pngs:
            name = os.path.splitext(png)[0]
            counts[enable_streaming(f"{game_sub}/{name}")] += 1
            total += 1
    unreal.log(f"--- {label} ({total} atlases) ---")


def main():
    counts = {"enabled": 0, "skipped": 0, "failed": 0}
    start = time.time()

    unreal.log("=" * 60)
    unreal.log("ENABLE STREAMING — Part 1/2 (player + enemies + hair + headgear)")
    unreal.log("=" * 60)

    # 1. priest_f body
    process_folder(
        os.path.join(ATLAS_ROOT, "Body/priest_f"),
        f"{GAME_ROOT}/Body/priest_f",
        "priest_f body",
        counts,
    )

    # 2. prt_south enemies
    for enemy in ["poring", "fabre", "pupa", "lunatic", "drops", "chonchon", "picky"]:
        process_folder(
            os.path.join(ATLAS_ROOT, "Body/enemies", enemy),
            f"{GAME_ROOT}/Body/enemies/{enemy}",
            f"enemy: {enemy}",
            counts,
        )

    # 3. Hair (recursive)
    process_recursive(
        os.path.join(ATLAS_ROOT, "Hair"),
        f"{GAME_ROOT}/Hair",
        "Hair (recursive)",
        counts,
    )

    # 4. Egg shell hat (recursive)
    process_recursive(
        os.path.join(ATLAS_ROOT, "HeadgearTop/egg_shell_hat"),
        f"{GAME_ROOT}/HeadgearTop/egg_shell_hat",
        "HeadgearTop/egg_shell_hat (recursive)",
        counts,
    )

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Part 1 done in {elapsed:.1f}s. {counts}")
    unreal.log("=" * 60)
    unreal.log("Wait for 'Compiling Textures: 0' before running part 2 (weapons).")


main()
