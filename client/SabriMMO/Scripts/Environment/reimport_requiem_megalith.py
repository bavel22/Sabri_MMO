"""Force re-import of requiem and megalith atlases (after re-render).

Deletes existing .uasset files in UE5 BEFORE importing — necessary because
UE5 caches texture data aggressively and replace_existing alone often doesn't
pick up changes when the source PNG was overwritten.

Usage (in UE5 editor Python console):
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/reimport_requiem_megalith.py"
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

FOLDERS = [
    "requiem",
    "megalith",
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


def delete_existing_assets(dest_path):
    """Delete all assets currently in the destination folder."""
    if not eal.does_directory_exist(dest_path):
        return 0
    asset_paths = eal.list_assets(dest_path, recursive=False, include_folder=False)
    deleted = 0
    for ap in asset_paths:
        clean = ap.split(".")[0]
        if eal.delete_asset(clean):
            deleted += 1
    return deleted


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
    imported = len(tasks)
    configured = 0
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{dest_path}/{name}"
        if eal.does_asset_exist(asset_path) and apply_sprite_settings(asset_path):
            configured += 1
    return imported, configured


total_deleted = 0
total_imported = 0
total_configured = 0
unreal.log(f"[FORCE REIMPORT] {len(FOLDERS)} enemy sprite folders...")
for name in FOLDERS:
    src = os.path.join(ROOT, name)
    if not os.path.isdir(src):
        unreal.log_warning(f"  {name}: source folder not found - skipping")
        continue
    dest = f"{GAME_ROOT}/{name}"
    deleted = delete_existing_assets(dest)
    unreal.log(f"  {name}: deleted {deleted} existing assets")
    imported, configured = import_folder(src, dest)
    unreal.log(f"  {name}: imported {imported}, configured {configured}")
    total_deleted += deleted
    total_imported += imported
    total_configured += configured

unreal.log(f"\n{'=' * 50}")
unreal.log(f"  Total deleted:    {total_deleted}")
unreal.log(f"  Total imported:   {total_imported}")
unreal.log(f"  Total configured: {total_configured}")
unreal.log(f"{'=' * 50}")
