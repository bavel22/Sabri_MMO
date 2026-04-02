# import_biome_textures.py
# Imports all biome textures + sand textures from Content folder into UE5.
# Run after generate_all_biomes.py has completed and files are copied.

import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
BIOME_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/Biomes"
SAND_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/Sand"


def import_folder(source_dir, dest_path):
    """Import all PNGs from source_dir into dest_path. Skip existing."""
    pngs = glob.glob(os.path.join(source_dir, "*.png"))
    if not pngs:
        return 0

    tasks = []
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{dest_path}/{name}"
        if eal.does_asset_exist(asset_path):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", dest_path)
        task.set_editor_property("filename", png)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    if not tasks:
        return 0

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    return len(tasks)


total = 0

# Import sand textures
unreal.log("--- Sand Textures ---")
n = import_folder(SAND_ROOT, "/Game/SabriMMO/Textures/Environment/Sand")
unreal.log(f"  Imported: {n}")
total += n

# Import each biome subfolder
biome_dirs = sorted(glob.glob(os.path.join(BIOME_ROOT, "*")))
for biome_dir in biome_dirs:
    if not os.path.isdir(biome_dir):
        continue
    biome_name = os.path.basename(biome_dir)
    dest = f"/Game/SabriMMO/Textures/Environment/Biomes/{biome_name}"
    unreal.log(f"--- {biome_name} ---")
    n = import_folder(biome_dir, dest)
    unreal.log(f"  Imported: {n}")
    total += n

unreal.log(f"\nDONE: {total} textures imported")
unreal.log(f"Browse: /Game/SabriMMO/Textures/Environment/Biomes/")
