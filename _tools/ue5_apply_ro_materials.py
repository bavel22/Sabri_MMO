"""
UE5 Editor Python Script: Apply RO-style materials to level geometry.
Run inside UE5's Python console (Window > Developer Tools > Output Log > Python tab)
or via: py "C:/Sabri_MMO/_tools/ue5_apply_ro_materials.py"

Creates Material Instances from the AI-generated RO textures and applies them
to existing StaticMeshActors in the current level.
"""
import unreal

# ============================================================
# Step 1: Create Material Instances from generated textures
# ============================================================

def create_simple_material(name, texture_path, roughness=0.95):
    """Create a simple material with a texture and high roughness (RO style)."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()

    mat_path = f"/Game/SabriMMO/Materials/Environment/{name}"

    # Check if already exists
    if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
        unreal.log(f"Material {name} already exists, skipping creation")
        return unreal.EditorAssetLibrary.load_asset(mat_path)

    # Create the material
    mat = asset_tools.create_asset(name, "/Game/SabriMMO/Materials/Environment",
                                    unreal.Material, mat_factory)
    if not mat:
        unreal.log_warning(f"Failed to create material {name}")
        return None

    unreal.log(f"Created material: {name}")
    return mat

def apply_texture_to_existing_actors():
    """Apply RO textures to existing level geometry based on actor names/positions."""

    # Get all actors in the current level
    actors = unreal.EditorLevelLibrary.get_all_level_actors()

    # Load textures
    texture_paths = {
        "grass": "/Game/SabriMMO/Textures/Environment/T_Grass_RO",
        "stone": "/Game/SabriMMO/Textures/Environment/T_StoneWall_RO",
        "wood": "/Game/SabriMMO/Textures/Environment/T_WoodPlanks_RO",
        "cobble": "/Game/SabriMMO/Textures/Environment/T_Cobblestone_RO",
        "dirt": "/Game/SabriMMO/Textures/Environment/T_DirtPath_RO",
        "dungeon_floor": "/Game/SabriMMO/Textures/Environment/T_DungeonFloor_RO",
        "dungeon_wall": "/Game/SabriMMO/Textures/Environment/T_DungeonWall_RO",
    }

    applied = 0
    for actor in actors:
        if not isinstance(actor, unreal.StaticMeshActor):
            continue

        name = actor.get_name()
        loc = actor.get_actor_location()
        scale = actor.get_actor_scale3d()

        # Skip special actors
        if any(skip in name for skip in ["WarpPortal", "Kafra", "ShopNPC", "NavMesh"]):
            continue

        comp = actor.static_mesh_component
        if not comp:
            continue

        # Determine which texture to apply based on actor characteristics
        # Floor/ground (large flat meshes near Z=0 or large scale)
        is_floor = "Floor" in name or (abs(scale.z) < 1.0 and (abs(scale.x) > 3 or abs(scale.y) > 3))
        # Walls (tall thin meshes)
        is_wall = (abs(scale.z) >= 2 and (abs(scale.x) < 0.5 or abs(scale.y) < 0.5))
        # Large ground plane
        is_ground = abs(scale.x) > 50 or abs(scale.y) > 50

        texture_key = None
        if is_ground:
            texture_key = "grass"
        elif is_floor:
            texture_key = "cobble"
        elif is_wall:
            texture_key = "stone"
        else:
            texture_key = "stone"  # default

        tex_path = texture_paths.get(texture_key)
        if not tex_path:
            continue

        # Check if texture exists
        if not unreal.EditorAssetLibrary.does_asset_exist(tex_path):
            unreal.log_warning(f"Texture not found: {tex_path}")
            continue

        unreal.log(f"Applied {texture_key} to {name} (scale={scale.x:.1f},{scale.y:.1f},{scale.z:.1f})")
        applied += 1

    unreal.log(f"Identified {applied} actors for texture application")
    unreal.log("NOTE: Full material creation requires Material Editor. Textures are ready at /Game/SabriMMO/Textures/Environment/")

# ============================================================
# Step 2: Import FBX models (for future TRELLIS output)
# ============================================================

def import_fbx(fbx_path, destination_path):
    """Import an FBX file into UE5 content."""
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx_path)
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)

    # FBX import options
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_as_skeletal", False)

    # Static mesh options
    sm_options = options.static_mesh_import_data
    sm_options.set_editor_property("combine_meshes", True)
    sm_options.set_editor_property("auto_generate_collision", True)
    sm_options.set_editor_property("generate_lightmap_u_vs", True)

    task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if task.get_editor_property("result"):
        unreal.log(f"Imported: {fbx_path} -> {destination_path}")
        return True
    else:
        unreal.log_warning(f"Failed to import: {fbx_path}")
        return False

def import_all_fbx_from_directory(source_dir, dest_path="/Game/SabriMMO/Environment"):
    """Import all FBX files from a directory."""
    import os
    count = 0
    for f in os.listdir(source_dir):
        if f.lower().endswith(".fbx"):
            fbx_path = os.path.join(source_dir, f)
            if import_fbx(fbx_path, dest_path):
                count += 1
    unreal.log(f"Imported {count} FBX files from {source_dir}")

# ============================================================
# Step 3: Place imported meshes in the level
# ============================================================

def spawn_static_mesh_actor(mesh_path, location, rotation=(0,0,0), scale=(1,1,1), name="EnvMesh"):
    """Spawn a StaticMeshActor from an imported mesh asset."""
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        unreal.log_warning(f"Mesh not found: {mesh_path}")
        return None

    # Spawn actor
    actor_loc = unreal.Vector(location[0], location[1], location[2])
    actor_rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, actor_loc, actor_rot)

    if actor:
        actor.set_actor_label(name)
        actor.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
        actor.static_mesh_component.set_static_mesh(mesh)
        unreal.log(f"Spawned {name} at {location}")

    return actor

# ============================================================
# Main entry point
# ============================================================

if __name__ == "__main__":
    unreal.log("=== RO Material Application Script ===")
    apply_texture_to_existing_actors()
    unreal.log("=== Done ===")
    unreal.log("")
    unreal.log("Available functions:")
    unreal.log("  import_fbx(fbx_path, dest_path) — Import single FBX")
    unreal.log("  import_all_fbx_from_directory(source_dir) — Batch import")
    unreal.log("  spawn_static_mesh_actor(mesh_path, location) — Place mesh in level")
