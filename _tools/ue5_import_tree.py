"""
UE5 Python Script: Import the RO tree and place it in the current level.
Run in UE5 Output Log Python console:
  py "C:/Sabri_MMO/_tools/ue5_import_tree.py"
"""
import unreal

# Step 1: Import the FBX
fbx_path = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/Trees/SM_Tree_01.fbx"
dest_path = "/Game/SabriMMO/Environment/Trees"

task = unreal.AssetImportTask()
task.set_editor_property("filename", fbx_path)
task.set_editor_property("destination_path", dest_path)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("replace_existing", True)

options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_textures", True)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_as_skeletal", False)
options.static_mesh_import_data.set_editor_property("combine_meshes", True)
options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
options.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)

task.set_editor_property("options", options)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.log("FBX import complete")

# Step 2: Load the imported mesh and place it in the level
mesh_path = dest_path + "/SM_Tree_01"
mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)

if mesh:
    # Place 3 trees at different locations in PrtSouth
    locations = [
        (500, 300, 0),
        (-400, 600, 0),
        (900, -500, 0),
    ]
    for i, loc in enumerate(locations):
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(loc[0], loc[1], loc[2]),
            unreal.Rotator(0, i * 45, 0)  # slight rotation variety
        )
        if actor:
            actor.static_mesh_component.set_static_mesh(mesh)
            actor.set_actor_label(f"RO_Tree_{i+1:02d}")
            actor.set_actor_scale3d(unreal.Vector(100, 100, 100))  # TRELLIS models are tiny, scale up
            unreal.log(f"Placed RO_Tree_{i+1:02d} at {loc}")

    unreal.log("=== Done! 3 RO trees placed. Save the level (Ctrl+S) ===")
else:
    unreal.log_warning(f"Could not load mesh at {mesh_path}. Check Content Browser.")
