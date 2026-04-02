import unreal, glob, os
for zone_dir in glob.glob("C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/ROZones/*"):
    if not os.path.isdir(zone_dir): continue
    zone = os.path.basename(zone_dir)
    dest = f"/Game/SabriMMO/Textures/Environment/ROZones/{zone}"
    tasks = []
    for png in glob.glob(os.path.join(zone_dir, "*.png")):
        name = os.path.splitext(os.path.basename(png))[0]
        if not unreal.EditorAssetLibrary.does_asset_exist(f"{dest}/{name}"):
            t = unreal.AssetImportTask()
            t.set_editor_property("automated", True)
            t.set_editor_property("destination_path", dest)
            t.set_editor_property("filename", png)
            t.set_editor_property("replace_existing", True)
            t.set_editor_property("save", True)
            tasks.append(t)
    if tasks: unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.log(f"{zone}: {len(tasks)} imported")
