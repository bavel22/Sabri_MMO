"""
UE5 Python: Create materials from RO textures and apply to imported environment meshes.
Run: paste in Output Log Python console:
exec(open("C:/Sabri_MMO/_tools/ue5_fix_materials.py").read())
"""
import unreal

TEX_BASE = "/Game/SabriMMO/Textures/Environment"
MAT_BASE = "/Game/SabriMMO/Materials/Environment"

def create_material_from_texture(mat_name, texture_path):
    """Create a simple material asset that uses a texture."""
    mat_path = f"{MAT_BASE}/{mat_name}"

    # Skip if already exists
    if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
        return unreal.EditorAssetLibrary.load_asset(mat_path)

    # Check texture exists
    if not unreal.EditorAssetLibrary.does_asset_exist(texture_path):
        unreal.log_warning(f"Texture not found: {texture_path}")
        return None

    # Create material
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(mat_name, MAT_BASE, unreal.Material, mat_factory)
    if not mat:
        unreal.log_warning(f"Failed to create material {mat_name}")
        return None

    unreal.log(f"Created material: {mat_name}")
    return mat

# Create materials (even without node wiring, they'll be assignable)
materials = {}
tex_map = {
    "MI_RO_Grass": f"{TEX_BASE}/T_Grass_RO",
    "MI_RO_Stone": f"{TEX_BASE}/T_StoneWall_RO",
    "MI_RO_Wood": f"{TEX_BASE}/T_WoodPlanks_RO",
    "MI_RO_Cobble": f"{TEX_BASE}/T_Cobblestone_RO",
    "MI_RO_Dirt": f"{TEX_BASE}/T_DirtPath_RO",
    "MI_RO_DungeonFloor": f"{TEX_BASE}/T_DungeonFloor_RO",
    "MI_RO_DungeonWall": f"{TEX_BASE}/T_DungeonWall_RO",
}

for mat_name, tex_path in tex_map.items():
    m = create_material_from_texture(mat_name, tex_path)
    if m:
        materials[mat_name] = m

# Now apply materials to actors in the level by name pattern
actors = unreal.EditorLevelLibrary.get_all_level_actors()

applied = 0
for actor in actors:
    if not isinstance(actor, unreal.StaticMeshActor):
        continue

    label = actor.get_actor_label()
    comp = actor.static_mesh_component
    if not comp:
        continue

    # Match assets to materials based on name
    mat_to_apply = None
    if "Tree" in label:
        # Trees need canopy (grass) on material 1, wood on material 0
        # But since they're joined meshes, just apply grass (dominant visual)
        mat_to_apply = materials.get("MI_RO_Grass")
    elif "Rock" in label:
        mat_to_apply = materials.get("MI_RO_Stone")
    elif "Bush" in label:
        mat_to_apply = materials.get("MI_RO_Grass")
    elif "Building" in label:
        mat_to_apply = materials.get("MI_RO_Stone")

    if mat_to_apply and comp.get_num_materials() > 0:
        for i in range(comp.get_num_materials()):
            comp.set_material(i, mat_to_apply)
        applied += 1
        unreal.log(f"Applied material to {label}")

unreal.log(f"Done! Applied materials to {applied} actors.")
unreal.log("NOTE: You can manually drag textures from Content/SabriMMO/Textures/Environment/ onto any mesh for finer control.")
