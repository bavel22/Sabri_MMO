"""Patches sRGB=False on all recently-imported female weapon overlay textures.

Run in UE5 editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/fix_weapon_atlas_srgb.py"

Why: The sprite material samples textures as Linear Color, but UE5 imports PNGs
with sRGB=true by default. That mismatch makes the material compile fail and
the engine falls back to the default gray material — the weapon overlay vanishes.
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary

# (subdir, weapon_id) for every female weapon atlas this session
WEAPON_DIRS = [
    ("sword_1h",   2),
    ("sword_2h",   3),
    ("spear_1h",   4),
    ("spear_2h",   5),
    ("axe_1h",     6),
    ("axe_2h",     7),
    ("mace",       8),
    ("rod",        10),
    ("knuckle",    12),
    ("instrument", 13),
    ("whip",       14),
    ("book",       15),
    ("katar",      16),
    ("staff",      23),
]

CONTENT_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Weapon"

total_fixed = 0
total_already_ok = 0
total_missing = 0

for subdir, wid in WEAPON_DIRS:
    src_dir = os.path.join(CONTENT_ROOT, subdir, "female")
    if not os.path.isdir(src_dir):
        unreal.log(f"[skip] {subdir}/female: folder missing on disk")
        continue
    pngs = sorted(glob.glob(os.path.join(src_dir, f"weapon_{wid}_*.png")))
    if not pngs:
        unreal.log(f"[skip] {subdir}/female: no weapon_{wid}_*.png files")
        continue

    fixed_in_dir = 0
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{GAME_ROOT}/{subdir}/female/{name}"
        if not eal.does_asset_exist(asset_path):
            total_missing += 1
            continue
        tex = unreal.load_asset(asset_path)
        if not tex or not isinstance(tex, unreal.Texture2D):
            total_missing += 1
            continue
        if tex.get_editor_property("srgb"):
            tex.set_editor_property("srgb", False)
            eal.save_asset(asset_path)
            fixed_in_dir += 1
            total_fixed += 1
        else:
            total_already_ok += 1

    unreal.log(f"[{subdir}/female] {fixed_in_dir} textures patched (sRGB → False)")

unreal.log(f"\n=== Total: fixed={total_fixed}, already_correct={total_already_ok}, missing={total_missing} ===")
