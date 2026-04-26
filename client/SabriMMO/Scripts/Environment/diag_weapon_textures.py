"""Diagnostic: compare imported texture properties for sword_1h (works) vs knuckle (broken).

Run in UE5 editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/diag_weapon_textures.py"

Prints sRGB, compression, lod_group, filter, and CompressionSettings for the first idle
texture of each weapon. The mismatch will tell us which property differs.
"""
import unreal

eal = unreal.EditorAssetLibrary

CASES = [
    ("sword_1h (works)", "/Game/SabriMMO/Sprites/Atlases/Weapon/sword_1h/female/weapon_2_idle"),
    ("knuckle (broken)", "/Game/SabriMMO/Sprites/Atlases/Weapon/knuckle/female/weapon_12_idle"),
    ("mage_f body (works)", "/Game/SabriMMO/Sprites/Atlases/Body/mage_f/mage_f_idle"),
]

unreal.log("\n=== Texture property diagnostic ===")
for label, path in CASES:
    if not eal.does_asset_exist(path):
        unreal.log(f"\n[{label}] {path}\n  NOT FOUND")
        continue
    tex = unreal.load_asset(path)
    if not tex or not isinstance(tex, unreal.Texture2D):
        unreal.log(f"\n[{label}] {path}\n  NOT a Texture2D")
        continue

    srgb = tex.get_editor_property("srgb")
    comp = tex.get_editor_property("compression_settings")
    lod = tex.get_editor_property("lod_group")
    filt = tex.get_editor_property("filter")
    mip = tex.get_editor_property("mip_gen_settings")
    nstr = tex.get_editor_property("never_stream")
    sx = tex.blueprint_get_size_x() if hasattr(tex, "blueprint_get_size_x") else "?"
    sy = tex.blueprint_get_size_y() if hasattr(tex, "blueprint_get_size_y") else "?"

    unreal.log(f"\n[{label}] {path}")
    unreal.log(f"  size:        {sx}x{sy}")
    unreal.log(f"  sRGB:        {srgb}")
    unreal.log(f"  compression: {comp}")
    unreal.log(f"  lod_group:   {lod}")
    unreal.log(f"  filter:      {filt}")
    unreal.log(f"  mip_gen:     {mip}")
    unreal.log(f"  never_stream:{nstr}")
