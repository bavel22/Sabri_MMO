"""FULL migration: applies AlphaCoverage mips to every sprite atlas in the project.

DO NOT run this until the test (migrate_sprite_quality_test.py) has been validated.
This script processes ~2,700 atlases in waves of 10 entities at a time. Each wave
logs progress so you can interrupt with Ctrl+C and resume later — already-migrated
assets are skipped on re-run.

Run from UE5 Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/migrate_sprite_quality_full.py"

Total runtime estimate: 4-8 hours of UE5 background texture compilation after the
script itself finishes (the script just sets properties; UE5 recompiles textures
asynchronously). Safe to leave running overnight.

Wave order:
  1. Player classes  (39 folders, 4 waves of 10)
  2. Weapons         (16 types × 2 genders = up to 32 folders, ~3 waves of 10)
  3. Hair / Headgear (small)
  4. Enemies         (204 folders, ~21 waves of 10)
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases"
WAVE_SIZE = 10  # entities per wave (adjust if needed)


# ────────────────────────────────────────────────────────────
# Per-asset migration
# ────────────────────────────────────────────────────────────

def apply_quality_settings(asset_path):
    """Apply mip-gen and alpha-coverage settings. Idempotent.
    Returns 'configured', 'skipped', or 'failed'."""
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        return "skipped"

    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "failed"

    if tex.get_editor_property("mip_gen_settings") == \
            unreal.TextureMipGenSettings.TMGS_ALPHA_COVERAGE:
        return "skipped"

    tex.set_editor_property("mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_ALPHA_COVERAGE)
    tex.set_editor_property("alpha_coverage_thresholds",
        unreal.LinearColor(0.0, 0.0, 0.0, 0.5))

    # Re-affirm the existing baseline
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("never_stream", True)

    eal.save_asset(clean)
    return "configured"


def migrate_folder(disk_dir, game_dir, counts):
    """Migrate every PNG-derived atlas in a folder. Recurses into gender subfolders."""
    if not os.path.isdir(disk_dir):
        return

    # Gender subfolders (male/female) — recurse first
    for sub in ("male", "female"):
        sub_disk = os.path.join(disk_dir, sub)
        if os.path.isdir(sub_disk):
            migrate_folder(sub_disk, f"{game_dir}/{sub}", counts)

    for png in sorted(glob.glob(os.path.join(disk_dir, "*.png"))):
        name = os.path.splitext(os.path.basename(png))[0]
        result = apply_quality_settings(f"{game_dir}/{name}")
        counts[result] += 1


# ────────────────────────────────────────────────────────────
# Wave processing
# ────────────────────────────────────────────────────────────

def chunked(seq, n):
    for i in range(0, len(seq), n):
        yield seq[i:i + n]


def process_wave(wave_label, entries):
    """One wave = list of (disk_dir, game_dir) pairs."""
    counts = {"configured": 0, "skipped": 0, "failed": 0}
    start = time.time()
    unreal.log(f"--- {wave_label} ({len(entries)} entities) ---")
    for disk_dir, game_dir in entries:
        migrate_folder(disk_dir, game_dir, counts)
    elapsed = time.time() - start
    unreal.log(f"    configured={counts['configured']} "
        f"skipped={counts['skipped']} failed={counts['failed']} ({elapsed:.1f}s)")
    return counts


def gather_player_classes():
    body_root = os.path.join(ATLAS_ROOT, "Body")
    classes = sorted(d for d in os.listdir(body_root)
                     if os.path.isdir(os.path.join(body_root, d))
                     and d != "enemies"
                     and d != "tests")
    return [(os.path.join(body_root, c), f"{GAME_ROOT}/Body/{c}") for c in classes]


def gather_weapons():
    weapon_root = os.path.join(ATLAS_ROOT, "Weapon")
    if not os.path.isdir(weapon_root):
        return []
    entries = []
    for wtype in sorted(os.listdir(weapon_root)):
        wdir = os.path.join(weapon_root, wtype)
        if os.path.isdir(wdir):
            entries.append((wdir, f"{GAME_ROOT}/Weapon/{wtype}"))
    return entries


def gather_hair_headgear():
    entries = []
    for layer in ("Hair", "HeadgearTop", "HeadgearMid", "HeadgearLow", "Garment", "Shield"):
        ldir = os.path.join(ATLAS_ROOT, layer)
        if not os.path.isdir(ldir):
            continue
        for sub in sorted(os.listdir(ldir)):
            sdir = os.path.join(ldir, sub)
            if os.path.isdir(sdir):
                entries.append((sdir, f"{GAME_ROOT}/{layer}/{sub}"))
    return entries


def gather_enemies():
    edir_root = os.path.join(ATLAS_ROOT, "Body/enemies")
    if not os.path.isdir(edir_root):
        return []
    enemies = sorted(d for d in os.listdir(edir_root)
                     if os.path.isdir(os.path.join(edir_root, d)))
    return [(os.path.join(edir_root, e),
             f"{GAME_ROOT}/Body/enemies/{e}") for e in enemies]


# ────────────────────────────────────────────────────────────
# Main
# ────────────────────────────────────────────────────────────

def main():
    unreal.log("=" * 60)
    unreal.log("SPRITE QUALITY MIGRATION — FULL")
    unreal.log("=" * 60)
    unreal.log("This will set TMGS_ALPHA_COVERAGE on every sprite atlas.")
    unreal.log("UE5 will recompile each texture in the background after save.")
    unreal.log("Watch the bottom-right 'Compiling Textures' indicator.")
    unreal.log("Safe to interrupt with Ctrl+C and resume — already-migrated assets are skipped.")
    unreal.log("")

    grand_total = {"configured": 0, "skipped": 0, "failed": 0}

    sections = [
        ("Player classes", gather_player_classes()),
        ("Weapons",        gather_weapons()),
        ("Hair / Headgear / Garment / Shield", gather_hair_headgear()),
        ("Enemies",        gather_enemies()),
    ]

    overall_start = time.time()
    for section_name, entries in sections:
        unreal.log("")
        unreal.log(f"### {section_name} — {len(entries)} entities total ###")
        for i, wave in enumerate(chunked(entries, WAVE_SIZE), start=1):
            label = f"{section_name} wave {i}/{(len(entries)+WAVE_SIZE-1)//WAVE_SIZE}"
            counts = process_wave(label, wave)
            for k, v in counts.items():
                grand_total[k] += v

    elapsed = time.time() - overall_start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"GRAND TOTAL after {elapsed:.1f}s of script time:")
    unreal.log(f"  Configured:  {grand_total['configured']} atlases")
    unreal.log(f"  Skipped:     {grand_total['skipped']} (already migrated)")
    unreal.log(f"  Failed:      {grand_total['failed']}")
    unreal.log("=" * 60)
    unreal.log("UE5 is now recompiling textures in the background.")
    unreal.log("This can take 4-8 hours. Watch the 'Compiling Textures: N' indicator.")
    unreal.log("DO NOT close the editor until it reads zero. After that, restart the editor.")


main()
