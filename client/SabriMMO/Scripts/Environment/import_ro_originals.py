# import_ro_originals.py
# Imports all 608 original RO Classic ground textures from bghq.com
# into /Game/SabriMMO/Textures/Environment/RO_Original/

import unreal
import os
import glob

src = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/RO_Original"
dest = "/Game/SabriMMO/Textures/Environment/RO_Original"

pngs = sorted(glob.glob(os.path.join(src, "*.png")))
tasks = []
for png in pngs:
    name = os.path.splitext(os.path.basename(png))[0]
    if not unreal.EditorAssetLibrary.does_asset_exist(f"{dest}/{name}"):
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("filename", png)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)

if tasks:
    unreal.log(f"Importing {len(tasks)} RO Original textures...")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
else:
    unreal.log("All RO Original textures already imported")

unreal.log(f"DONE: {len(tasks)} imported to {dest}")
unreal.log(f"Total: 608 textures from bghq.com/textures/ro/")
unreal.log(f"Browse: {dest}")
