"""
Blender script: Generate RO-style environment assets.
Scale calibrated to match SM_RO_Tree_01 (Blender Z=144.8 = good in-game size).
"""
import bpy
import bmesh
import math
import random
import os

OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment"

def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for m in list(bpy.data.meshes): bpy.data.meshes.remove(m)
    for m in list(bpy.data.materials): bpy.data.materials.remove(m)

def make_material(name, color, roughness=0.95):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (*color, 1.0)
    bsdf.inputs['Roughness'].default_value = roughness
    out = nodes.new('ShaderNodeOutputMaterial')
    mat.node_tree.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return mat

def finalize_and_export(obj, name, subfolder, target_height):
    """Scale asset so its Z dimension matches target_height, then export."""
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    # Calculate scale to hit target height
    current_z = obj.dimensions.z
    if current_z > 0.001:
        s = target_height / current_z
    else:
        s = target_height
    obj.scale = (s, s, s)
    bpy.ops.object.transform_apply(scale=True)

    dims = obj.dimensions
    print(f"  {name}: dims=({dims.x:.1f}, {dims.y:.1f}, {dims.z:.1f}), target_z={target_height}")

    # UV unwrap
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')

    # Export
    out_dir = os.path.join(OUTPUT_DIR, subfolder)
    os.makedirs(out_dir, exist_ok=True)
    fbx_path = os.path.join(out_dir, f"{name}.fbx")
    bpy.ops.export_scene.fbx(
        filepath=fbx_path, use_selection=True, global_scale=1.0,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
        mesh_smooth_type='FACE', use_mesh_modifiers=True,
        bake_space_transform=True, object_types={'MESH'},
        path_mode='COPY', embed_textures=True)
    print(f"  Exported -> {fbx_path}")

# ============================================================
# Target heights (calibrated to Tree_01 which has Z=144.8)
# ============================================================
# Tree_01 = 145 -> looks good in game
# Other assets proportioned relative to that:

HEIGHT_TREE_TALL   = 180   # taller birch
HEIGHT_TREE_WIDE   = 110   # shorter wide oak
HEIGHT_ROCK_LARGE  = 35    # big boulder
HEIGHT_ROCK_SMALL  = 12    # pebble cluster
HEIGHT_BUSH        = 25    # low shrub
HEIGHT_BUILDING    = 130   # medieval house (similar to tree)

# ============================================================
# ASSET 1: Tree Tall (birch-like)
# ============================================================
def make_tree_tall():
    clear_scene()
    random.seed(101)
    trunk_mat = make_material("M_Trunk_Birch", (0.6, 0.55, 0.45))
    canopy_mat = make_material("M_Canopy_Light", (0.35, 0.58, 0.22))
    bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=0.08, radius2=0.03, depth=3.0, location=(0,0,1.5))
    trunk = bpy.context.active_object
    trunk.name = "Trunk"
    trunk.data.materials.append(trunk_mat)
    bpy.ops.object.shade_smooth()
    objs = []
    for x,y,z,r in [(0,0,3.2,0.45),(0.15,0.1,3.6,0.4),(-0.1,0.1,3.9,0.35),(0.05,-0.08,4.2,0.3)]:
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=r, location=(x,y,z))
        s = bpy.context.active_object
        s.data.materials.append(canopy_mat)
        s.scale.z *= random.uniform(0.6,0.8)
        bpy.ops.object.transform_apply(scale=True)
        bpy.ops.object.shade_smooth()
        objs.append(s)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    canopy = bpy.context.active_object
    bpy.ops.object.select_all(action='DESELECT')
    trunk.select_set(True); canopy.select_set(True)
    bpy.context.view_layer.objects.active = trunk
    bpy.ops.object.join()
    tree = bpy.context.active_object
    tree.name = "SM_RO_Tree_02"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(tree, "SM_RO_Tree_02", "Trees", HEIGHT_TREE_TALL)

# ============================================================
# ASSET 2: Tree Wide (oak-like)
# ============================================================
def make_tree_wide():
    clear_scene()
    random.seed(202)
    trunk_mat = make_material("M_Trunk_Oak", (0.30, 0.18, 0.08))
    canopy_mat = make_material("M_Canopy_Dark", (0.2, 0.42, 0.12))
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=0.22, radius2=0.12, depth=1.5, location=(0,0,0.75))
    trunk = bpy.context.active_object
    trunk.name = "Trunk"
    trunk.data.materials.append(trunk_mat)
    bpy.ops.object.shade_smooth()
    objs = []
    for x,y,z,r in [(0,0,1.8,0.9),(0.5,0.3,1.9,0.65),(-0.5,0.2,1.7,0.7),(0.3,-0.4,2.0,0.6),(-0.3,-0.3,2.1,0.55),(0,0,2.3,0.5)]:
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=r, location=(x,y,z))
        s = bpy.context.active_object
        s.data.materials.append(canopy_mat)
        s.scale.z *= random.uniform(0.6,0.85)
        s.scale.x *= random.uniform(0.9,1.15)
        bpy.ops.object.transform_apply(scale=True)
        bpy.ops.object.shade_smooth()
        objs.append(s)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    canopy = bpy.context.active_object
    bpy.ops.object.select_all(action='DESELECT')
    trunk.select_set(True); canopy.select_set(True)
    bpy.context.view_layer.objects.active = trunk
    bpy.ops.object.join()
    tree = bpy.context.active_object
    tree.name = "SM_RO_Tree_03"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(tree, "SM_RO_Tree_03", "Trees", HEIGHT_TREE_WIDE)

# ============================================================
# ASSET 3: Large Rock
# ============================================================
def make_rock_large():
    clear_scene()
    random.seed(303)
    rock_mat = make_material("M_Rock", (0.45, 0.4, 0.35))
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.8, location=(0,0,0.4))
    rock = bpy.context.active_object
    rock.name = "SM_RO_Rock_Large"
    rock.data.materials.append(rock_mat)
    rock.scale = (1.3, 1.0, 0.6)
    bpy.ops.object.transform_apply(scale=True)
    bpy.ops.object.mode_set(mode='EDIT')
    bm = bmesh.from_edit_mesh(rock.data)
    for v in bm.verts:
        v.co.x += random.uniform(-0.08, 0.08)
        v.co.y += random.uniform(-0.08, 0.08)
        v.co.z += random.uniform(-0.06, 0.06)
        if v.co.z < 0.1: v.co.z = max(v.co.z, -0.02)
    bmesh.update_edit_mesh(rock.data)
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.shade_smooth()
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(rock, "SM_RO_Rock_Large", "Rocks", HEIGHT_ROCK_LARGE)

# ============================================================
# ASSET 4: Small Rock Cluster
# ============================================================
def make_rock_small():
    clear_scene()
    random.seed(404)
    rock_mat = make_material("M_Rock_Small", (0.5, 0.45, 0.38))
    rocks = []
    for (x,y,z), sz in zip([(0,0,0.15),(0.3,0.2,0.12),(-0.2,0.25,0.1),(0.15,-0.2,0.08)],[0.2,0.15,0.18,0.12]):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=sz, location=(x,y,z))
        r = bpy.context.active_object
        r.data.materials.append(rock_mat)
        r.scale = (random.uniform(0.8,1.3), random.uniform(0.8,1.2), random.uniform(0.5,0.8))
        bpy.ops.object.transform_apply(scale=True)
        bpy.ops.object.mode_set(mode='EDIT')
        bm = bmesh.from_edit_mesh(r.data)
        for v in bm.verts:
            v.co.x += random.uniform(-0.03, 0.03)
            v.co.y += random.uniform(-0.03, 0.03)
            v.co.z += random.uniform(-0.02, 0.02)
        bmesh.update_edit_mesh(r.data)
        bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.shade_smooth()
        rocks.append(r)
    bpy.ops.object.select_all(action='DESELECT')
    for r in rocks: r.select_set(True)
    bpy.context.view_layer.objects.active = rocks[0]
    bpy.ops.object.join()
    cluster = bpy.context.active_object
    cluster.name = "SM_RO_Rock_Small"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(cluster, "SM_RO_Rock_Small", "Rocks", HEIGHT_ROCK_SMALL)

# ============================================================
# ASSET 5: Bush
# ============================================================
def make_bush():
    clear_scene()
    random.seed(505)
    bush_mat = make_material("M_Bush", (0.22, 0.45, 0.15))
    objs = []
    for x,y,z,r in [(0,0,0.3,0.4),(0.25,0.15,0.25,0.3),(-0.2,0.2,0.28,0.32),(0.1,-0.2,0.22,0.28)]:
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=r, location=(x,y,z))
        s = bpy.context.active_object
        s.data.materials.append(bush_mat)
        s.scale.z *= random.uniform(0.6,0.8)
        s.scale.x *= random.uniform(0.9,1.1)
        bpy.ops.object.transform_apply(scale=True)
        bpy.ops.object.shade_smooth()
        bpy.ops.object.mode_set(mode='EDIT')
        bm = bmesh.from_edit_mesh(s.data)
        for v in bm.verts:
            v.co.x += random.uniform(-0.02, 0.02)
            v.co.y += random.uniform(-0.02, 0.02)
            v.co.z += random.uniform(-0.015, 0.015)
        bmesh.update_edit_mesh(s.data)
        bpy.ops.object.mode_set(mode='OBJECT')
        objs.append(s)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    bush = bpy.context.active_object
    bush.name = "SM_RO_Bush_01"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(bush, "SM_RO_Bush_01", "Foliage", HEIGHT_BUSH)

# ============================================================
# ASSET 6: Medieval Building
# ============================================================
def make_building():
    clear_scene()
    wall_mat = make_material("M_Stone_Wall", (0.6, 0.52, 0.4))
    roof_mat = make_material("M_Wood_Roof", (0.45, 0.25, 0.12))
    door_mat = make_material("M_Wood_Door", (0.35, 0.2, 0.1))
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0,0,1.0))
    body = bpy.context.active_object
    body.name = "Body"
    body.scale = (1.5, 1.2, 1.0)
    bpy.ops.object.transform_apply(scale=True)
    body.data.materials.append(wall_mat)
    bpy.ops.mesh.primitive_cone_add(vertices=4, radius1=1.2, radius2=0.0, depth=0.8, location=(0,0,2.0))
    roof = bpy.context.active_object
    roof.rotation_euler.z = math.radians(45)
    roof.scale = (1.4, 1.1, 1.0)
    bpy.ops.object.transform_apply(rotation=True, scale=True)
    roof.data.materials.append(roof_mat)
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0,-0.61,0.45))
    door = bpy.context.active_object
    door.scale = (0.25, 0.05, 0.4)
    bpy.ops.object.transform_apply(scale=True)
    door.data.materials.append(door_mat)
    for wx in [-0.5, 0.5]:
        bpy.ops.mesh.primitive_cube_add(size=1, location=(wx,-0.61,1.2))
        win = bpy.context.active_object
        win.scale = (0.15, 0.04, 0.15)
        bpy.ops.object.transform_apply(scale=True)
        win.data.materials.append(door_mat)
    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    building = bpy.context.active_object
    building.name = "SM_RO_Building_01"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(building, "SM_RO_Building_01", "Buildings", HEIGHT_BUILDING)

print("\n=== Generating RO Environment Assets ===\n")
print("1/6: Tree Tall...")
make_tree_tall()
print("2/6: Tree Wide...")
make_tree_wide()
print("3/6: Rock Large...")
make_rock_large()
print("4/6: Rock Small...")
make_rock_small()
print("5/6: Bush...")
make_bush()
print("6/6: Building...")
make_building()
print("\n=== ALL 6 ASSETS EXPORTED ===")
