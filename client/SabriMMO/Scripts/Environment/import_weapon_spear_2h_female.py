"""Imports the female 2H spear weapon overlay atlases into UE5.

Usage (in UE5 editor Python console):
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_weapon_spear_2h_female.py"

Imports 17 weapon_5_*.png atlases from
    Content/SabriMMO/Sprites/Atlases/Weapon/spear_2h/female/
and applies the standard sprite texture settings.

Set DELETE_FIRST=True to drop old .uasset files first if UE5 caches stale data.
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary

SRC_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/spear_2h/female"
GAME_DEST = "/Game/SabriMMO/Sprites/Atlases/Weapon/spear_2h/female"
DELETE_FIRST = False


def apply_sprite_settings(asset_path):
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("never_stream", True)
    eal.save_asset(clean)
    return True


pngs = sorted(glob.glob(os.path.join(SRC_DIR, "weapon_5_*.png")))
unreal.log(f"[spear_2h/female] Found {len(pngs)} PNG atlases to import")

if DELETE_FIRST:
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{GAME_DEST}/{name}"
        if eal.does_asset_exist(asset_path):
            eal.delete_asset(asset_path)
            unreal.log(f"  deleted stale {name}")

tasks = []
for png in pngs:
    name = os.path.splitext(os.path.basename(png))[0]
    asset_path = f"{GAME_DEST}/{name}"
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", GAME_DEST)
    task.set_editor_property("filename", png)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

configured = 0
for png in pngs:
    name = os.path.splitext(os.path.basename(png))[0]
    asset_path = f"{GAME_DEST}/{name}"
    if eal.does_asset_exist(asset_path) and apply_sprite_settings(asset_path):
        configured += 1

unreal.log(f"[spear_2h/female] Imported {len(tasks)}, configured {configured}")
