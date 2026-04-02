# Force-import ALL 608 RO Original textures regardless of what exists.
# The previous import skipped most because it thought they existed.

import unreal
import os
import glob

src = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/RO_Original"
dest = "/Game/SabriMMO/Textures/Environment/RO_Original"

pngs = sorted(glob.glob(os.path.join(src, "*.png")))
unreal.log(f"Found {len(pngs)} PNG files to import")

# Import in batches of 50 to avoid overwhelming the editor
batch_size = 50
total_imported = 0

for batch_start in range(0, len(pngs), batch_size):
    batch = pngs[batch_start:batch_start + batch_size]
    tasks = []
    for png in batch:
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("filename", png)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    total_imported += len(tasks)
    unreal.log(f"  Batch {batch_start//batch_size + 1}: imported {len(tasks)} ({total_imported}/{len(pngs)} total)")

unreal.log(f"\nDONE: {total_imported} textures force-imported to {dest}")
