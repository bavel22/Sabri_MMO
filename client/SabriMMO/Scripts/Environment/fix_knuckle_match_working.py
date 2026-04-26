"""Copies all texture settings from a known-working weapon (sword_1h) onto the
knuckle atlas textures, then re-saves the knuckle assets.

Run in UE5 editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/fix_knuckle_match_working.py"

Why: the sprite material uses SAMPLERTYPE_Color and the body/dagger/sword
atlases compile cleanly. Whatever combination of sRGB / Compression / LOD
group the working textures have, we want the knuckle to match.

The script:
  1) Reads the settings from sword_1h's idle atlas (the reference).
  2) Logs them so you can see what's expected.
  3) Iterates all 17 weapon_12_*.uasset textures, copies the same settings
     onto each, and saves.
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary

REFERENCE = "/Game/SabriMMO/Sprites/Atlases/Weapon/sword_1h/female/weapon_2_idle"

KNUCKLE_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/knuckle/female"
KNUCKLE_GAME = "/Game/SabriMMO/Sprites/Atlases/Weapon/knuckle/female"

ref = unreal.load_asset(REFERENCE)
if not ref or not isinstance(ref, unreal.Texture2D):
    raise RuntimeError(f"Reference texture not found: {REFERENCE}")

REF_SRGB = ref.get_editor_property("srgb")
REF_COMPRESSION = ref.get_editor_property("compression_settings")
REF_LOD = ref.get_editor_property("lod_group")
REF_FILTER = ref.get_editor_property("filter")
REF_MIPGEN = ref.get_editor_property("mip_gen_settings")
REF_NEVERSTREAM = ref.get_editor_property("never_stream")

unreal.log(f"\n=== Reference (working sword_1h weapon_2_idle) ===")
unreal.log(f"  sRGB:         {REF_SRGB}")
unreal.log(f"  compression:  {REF_COMPRESSION}")
unreal.log(f"  lod_group:    {REF_LOD}")
unreal.log(f"  filter:       {REF_FILTER}")
unreal.log(f"  mip_gen:      {REF_MIPGEN}")
unreal.log(f"  never_stream: {REF_NEVERSTREAM}")

# Apply to every knuckle texture
pngs = sorted(glob.glob(os.path.join(KNUCKLE_DIR, "weapon_12_*.png")))
unreal.log(f"\n=== Applying to {len(pngs)} knuckle textures ===")

patched = 0
missing = 0
for png in pngs:
    name = os.path.splitext(os.path.basename(png))[0]
    asset_path = f"{KNUCKLE_GAME}/{name}"
    if not eal.does_asset_exist(asset_path):
        unreal.log_warning(f"  [missing] {name}")
        missing += 1
        continue
    tex = unreal.load_asset(asset_path)
    if not tex or not isinstance(tex, unreal.Texture2D):
        unreal.log_warning(f"  [skip] {name} not a Texture2D")
        continue

    tex.set_editor_property("srgb", REF_SRGB)
    tex.set_editor_property("compression_settings", REF_COMPRESSION)
    tex.set_editor_property("lod_group", REF_LOD)
    tex.set_editor_property("filter", REF_FILTER)
    tex.set_editor_property("mip_gen_settings", REF_MIPGEN)
    tex.set_editor_property("never_stream", REF_NEVERSTREAM)
    eal.save_asset(asset_path)
    patched += 1

unreal.log(f"\n=== Done — patched={patched}, missing={missing} ===")
unreal.log("If any were missing, run import_weapon_knuckle_female.py first.")
