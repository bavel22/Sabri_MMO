"""Enable streaming (never_stream=false) on all atlases needed for prt_south testing.

Scope:
  - Player class: priest_f body (your character)
  - All 7 enemies remaining in prt_south after despawn:
      poring, fabre, pupa, lunatic, drops, chonchon, picky
  - All hair atlases (style_01, both genders)
  - All weapon atlases (every weapon type, both genders)
  - Egg shell hat (HeadgearTop, both genders)

Only changes never_stream. Nothing else is touched. Idempotent.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/enable_streaming_prt_south.py"

After it finishes:
  1. Wait for "Compiling Textures: 0" (could be ~5 min for ~400 atlases)
  2. Restart the editor
  3. Restart the server (so the despawn change takes effect)
  4. Walk into prt_south, check VRAM in Task Manager — should be much lower
  5. Test all Sprite Quality tiers; memory should now actually scale with the slider
  6. Watch for pop-in on porings, weapon swaps, etc.
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
        return "skipped"  # already streaming-enabled
    tex.set_editor_property("never_stream", False)
    eal.save_asset(clean)
    return "enabled"


def process_folder(disk_dir, game_dir, label, counts):
    """Walk a flat folder of PNGs and apply enable_streaming to each."""
    if not os.path.isdir(disk_dir):
        unreal.log_warning(f"  {label}: folder missing — skipping")
        return
    pngs = sorted(glob.glob(os.path.join(disk_dir, "*.png")))
    unreal.log(f"--- {label} ({len(pngs)} atlases) ---")
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        counts[enable_streaming(f"{game_dir}/{name}")] += 1


def process_recursive(disk_root, game_root, label, counts):
    """Walk a directory tree and apply enable_streaming to every PNG-derived asset."""
    if not os.path.isdir(disk_root):
        unreal.log_warning(f"  {label}: folder missing — skipping")
        return
    pngs_total = 0
    for sub_dir, _, files in os.walk(disk_root):
        pngs = sorted(f for f in files if f.lower().endswith(".png"))
        if not pngs:
            continue
        rel = os.path.relpath(sub_dir, disk_root).replace("\\", "/")
        game_sub = game_root if rel == "." else f"{game_root}/{rel}"
        for png in pngs:
            name = os.path.splitext(png)[0]
            counts[enable_streaming(f"{game_sub}/{name}")] += 1
            pngs_total += 1
    unreal.log(f"--- {label} ({pngs_total} atlases) ---")


def main():
    counts = {"enabled": 0, "skipped": 0, "failed": 0}
    start = time.time()

    unreal.log("=" * 60)
    unreal.log("ENABLE STREAMING — prt_south scope")
    unreal.log("=" * 60)

    # 1. Player body: priest_f
    process_folder(
        os.path.join(ATLAS_ROOT, "Body/priest_f"),
        f"{GAME_ROOT}/Body/priest_f",
        "priest_f body",
        counts,
    )

    # 2. prt_south enemies (post-despawn)
    enemies = ["poring", "fabre", "pupa", "lunatic", "drops", "chonchon", "picky"]
    for enemy in enemies:
        process_folder(
            os.path.join(ATLAS_ROOT, "Body/enemies", enemy),
            f"{GAME_ROOT}/Body/enemies/{enemy}",
            f"enemy: {enemy}",
            counts,
        )

    # 3. Hair (style_01 male + female + any others)
    process_recursive(
        os.path.join(ATLAS_ROOT, "Hair"),
        f"{GAME_ROOT}/Hair",
        "Hair (recursive)",
        counts,
    )

    # 4. All weapons (every type, every gender)
    process_recursive(
        os.path.join(ATLAS_ROOT, "Weapon"),
        f"{GAME_ROOT}/Weapon",
        "Weapon (recursive)",
        counts,
    )

    # 5. Egg shell hat (HeadgearTop)
    process_recursive(
        os.path.join(ATLAS_ROOT, "HeadgearTop/egg_shell_hat"),
        f"{GAME_ROOT}/HeadgearTop/egg_shell_hat",
        "HeadgearTop/egg_shell_hat (recursive)",
        counts,
    )

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Done in {elapsed:.1f}s. {counts}")
    unreal.log("=" * 60)
    unreal.log("Wait for 'Compiling Textures: 0' (bottom-right) before testing.")
    unreal.log("Then restart editor + server, walk into prt_south, watch VRAM in Task Manager.")


main()
