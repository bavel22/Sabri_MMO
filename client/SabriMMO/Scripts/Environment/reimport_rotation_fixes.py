import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

FOLDERS = [
    "drainliar",         # NEW import (was never imported)
    "greatest_general",  # Reimport after rotation fix
    "mole",              # Reimport after rotation fix (Holden)
    "sword_fish",        # Reimport after rotation fix
]


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


def import_folder(source_dir, dest_path):
    pngs = sorted(glob.glob(os.path.join(source_dir, "*.png")))
    if not pngs:
        return 0, 0

    tasks = []
    for png in pngs:
        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", dest_path)
        task.set_editor_property("filename", png)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    configured = 0
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{dest_path}/{name}"
        if eal.does_asset_exist(asset_path) and apply_sprite_settings(asset_path):
            configured += 1

    return len(tasks), configured


total_imported = 0
total_configured = 0
unreal.log(f"Importing/reimporting {len(FOLDERS)} enemy sprite folders...")

for name in FOLDERS:
    src = os.path.join(ROOT, name)
    if not os.path.isdir(src):
        unreal.log_warning(f"  {name}: source folder not found - skipping")
        continue
    dest = f"{GAME_ROOT}/{name}"
    imported, configured = import_folder(src, dest)
    unreal.log(f"  {name}: imported {imported}, configured {configured}")
    total_imported += imported
    total_configured += configured

unreal.log(f"\n{'=' * 50}")
unreal.log(f"  Total imported:   {total_imported}")
unreal.log(f"  Total configured: {total_configured}")
unreal.log(f"{'=' * 50}")
