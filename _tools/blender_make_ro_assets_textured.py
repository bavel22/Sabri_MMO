"""
Blender script: Generate RO-style environment assets WITH textures.
Uses AI-generated RO textures from ComfyUI.
Scale calibrated to match SM_RO_Tree_01 (Z=145 = good in-game size).
"""
import bpy
import bmesh
import math
import random
import os

OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment"
TEX_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment"

# Texture paths
TEX_GRASS = os.path.join(TEX_DIR, "T_Grass_RO.png")
TEX_STONE = os.path.join(TEX_DIR, "T_StoneWall_RO.png")
TEX_WOOD = os.path.join(TEX_DIR, "T_WoodPlanks_RO.png")
TEX_DIRT = os.path.join(TEX_DIR, "T_DirtPath_RO.png")
TEX_COBBLE = os.path.join(TEX_DIR, "T_Cobblestone_RO.png")

def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for m in list(bpy.data.meshes): bpy.data.meshes.remove(m)
    for m in list(bpy.data.materials): bpy.data.materials.remove(m)
    for i in list(bpy.data.images): bpy.data.images.remove(i)

def make_textured_material(name, texture_path, tint=(1,1,1), roughness=0.95, uv_scale=1.0):
    """Create a material with an image texture, optional tint, and high roughness."""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    # Output
    output = nodes.new('ShaderNodeOutputMaterial')
    output.location = (600, 0)

    # BSDF
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (300, 0)
    bsdf.inputs['Roughness'].default_value = roughness
    bsdf.inputs['Metallic'].default_value = 0.0
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    if os.path.exists(texture_path):
        # Image texture
        tex_node = nodes.new('ShaderNodeTexImage')
        tex_node.location = (-300, 0)
        img = bpy.data.images.load(texture_path)
        tex_node.image = img

        # UV Scale (Mapping node)
        if uv_scale != 1.0:
            mapping = nodes.new('ShaderNodeMapping')
            mapping.location = (-500, 0)
            mapping.inputs['Scale'].default_value = (uv_scale, uv_scale, 1)
            tex_coord = nodes.new('ShaderNodeTexCoord')
            tex_coord.location = (-700, 0)
            links.new(tex_coord.outputs['UV'], mapping.inputs['Vector'])
            links.new(mapping.outputs['Vector'], tex_node.inputs['Vector'])

        # Tint multiply
        if tint != (1, 1, 1):
            mix = nodes.new('ShaderNodeMix')
            mix.data_type = 'RGBA'
            mix.location = (0, 0)
            mix.inputs['Factor'].default_value = 1.0
            mix.blend_type = 'MULTIPLY'
            mix.inputs['B'].default_value = (*tint, 1.0)
            links.new(tex_node.outputs['Color'], mix.inputs['A'])
            links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
        else:
            links.new(tex_node.outputs['Color'], bsdf.inputs['Base Color'])
    else:
        # Fallback solid color if texture missing
        bsdf.inputs['Base Color'].default_value = (*tint, 1.0)
        print(f"  WARNING: Texture not found: {texture_path}, using solid color")

    return mat

def make_solid_material(name, color, roughness=0.95):
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
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    current_z = obj.dimensions.z
    if current_z > 0.001:
        s = target_height / current_z
    else:
        s = target_height
    obj.scale = (s, s, s)
    bpy.ops.object.transform_apply(scale=True)
    dims = obj.dimensions
    print(f"  {name}: dims=({dims.x:.1f}, {dims.y:.1f}, {dims.z:.1f})")

    # UV unwrap
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')

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

# Heights calibrated to Tree_01 (Z=145)
H_TREE_TALL = 180
H_TREE_WIDE = 110
H_ROCK_LG = 35
H_ROCK_SM = 12
H_BUSH = 25
H_BUILDING = 130

# ============================================================
# Tree Tall — birch with grass texture canopy + wood trunk
# ============================================================
def make_tree_tall():
    clear_scene()
    random.seed(101)
    trunk_mat = make_textured_material("M_Trunk", TEX_WOOD, tint=(0.7, 0.6, 0.5), uv_scale=2.0)
    canopy_mat = make_textured_material("M_Canopy", TEX_GRASS, tint=(0.9, 1.0, 0.8), uv_scale=1.5)

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
    finalize_and_export(tree, "SM_RO_Tree_02", "Trees", H_TREE_TALL)

# ============================================================
# Tree Wide — oak with dark green canopy
# ============================================================
def make_tree_wide():
    clear_scene()
    random.seed(202)
    trunk_mat = make_textured_material("M_Trunk_Oak", TEX_WOOD, tint=(0.5, 0.4, 0.3), uv_scale=2.0)
    canopy_mat = make_textured_material("M_Canopy_Dark", TEX_GRASS, tint=(0.6, 0.85, 0.5), uv_scale=1.0)

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
    finalize_and_export(tree, "SM_RO_Tree_03", "Trees", H_TREE_WIDE)

# ============================================================
# Rock Large — stone texture
# ============================================================
def make_rock_large():
    clear_scene()
    random.seed(303)
    rock_mat = make_textured_material("M_Rock", TEX_STONE, tint=(0.8, 0.75, 0.7), uv_scale=2.0)

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
    finalize_and_export(rock, "SM_RO_Rock_Large", "Rocks", H_ROCK_LG)

# ============================================================
# Rock Small — stone texture cluster
# ============================================================
def make_rock_small():
    clear_scene()
    random.seed(404)
    rock_mat = make_textured_material("M_Rock_Sm", TEX_STONE, tint=(0.85, 0.8, 0.75), uv_scale=3.0)
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
    finalize_and_export(cluster, "SM_RO_Rock_Small", "Rocks", H_ROCK_SM)

# ============================================================
# Bush — grass textured
# ============================================================
def make_bush():
    clear_scene()
    random.seed(505)
    bush_mat = make_textured_material("M_Bush", TEX_GRASS, tint=(0.7, 0.9, 0.6), uv_scale=2.0)
    objs = []
    for x,y,z,r in [(0,0,0.3,0.4),(0.25,0.15,0.25,0.3),(-0.2,0.2,0.28,0.32),(0.1,-0.2,0.22,0.28)]:
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=r, location=(x,y,z))
        s = bpy.context.active_object
        s.data.materials.append(bush_mat)
        s.scale.z *= random.uniform(0.6,0.8)
        s.scale.x *= random.uniform(0.9,1.1)
        bpy.ops.object.transform_apply(scale=True)
        bpy.ops.object.shade_smooth()
        objs.append(s)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    bush = bpy.context.active_object
    bush.name = "SM_RO_Bush_01"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(bush, "SM_RO_Bush_01", "Foliage", H_BUSH)

# ============================================================
# Building — stone walls + wood roof + cobblestone base, more detail
# ============================================================
def make_building():
    clear_scene()

    wall_mat = make_textured_material("M_Wall", TEX_STONE, tint=(1.0, 0.95, 0.9), uv_scale=2.0)
    roof_mat = make_textured_material("M_Roof", TEX_WOOD, tint=(0.8, 0.5, 0.3), uv_scale=1.5)
    door_mat = make_textured_material("M_Door", TEX_WOOD, tint=(0.6, 0.35, 0.2), uv_scale=3.0)
    base_mat = make_textured_material("M_Base", TEX_COBBLE, tint=(0.9, 0.85, 0.8), uv_scale=2.0)

    all_objs = []

    # Foundation/base
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 0.1))
    base = bpy.context.active_object
    base.scale = (1.7, 1.4, 0.15)
    bpy.ops.object.transform_apply(scale=True)
    base.data.materials.append(base_mat)
    all_objs.append(base)

    # Main walls
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 1.0))
    body = bpy.context.active_object
    body.scale = (1.5, 1.2, 0.9)
    bpy.ops.object.transform_apply(scale=True)
    body.data.materials.append(wall_mat)
    all_objs.append(body)

    # Second floor (slightly narrower)
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 1.95))
    upper = bpy.context.active_object
    upper.scale = (1.55, 1.25, 0.7)
    bpy.ops.object.transform_apply(scale=True)
    upper.data.materials.append(wall_mat)
    all_objs.append(upper)

    # Roof (4-sided pyramid)
    bpy.ops.mesh.primitive_cone_add(vertices=4, radius1=1.3, radius2=0.0, depth=0.9, location=(0, 0, 2.8))
    roof = bpy.context.active_object
    roof.rotation_euler.z = math.radians(45)
    roof.scale = (1.35, 1.1, 1.0)
    bpy.ops.object.transform_apply(rotation=True, scale=True)
    roof.data.materials.append(roof_mat)
    all_objs.append(roof)

    # Door
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, -0.61, 0.45))
    door = bpy.context.active_object
    door.scale = (0.22, 0.06, 0.35)
    bpy.ops.object.transform_apply(scale=True)
    door.data.materials.append(door_mat)
    all_objs.append(door)

    # Windows (ground floor)
    for wx in [-0.45, 0.45]:
        bpy.ops.mesh.primitive_cube_add(size=1, location=(wx, -0.61, 0.9))
        win = bpy.context.active_object
        win.scale = (0.12, 0.05, 0.14)
        bpy.ops.object.transform_apply(scale=True)
        win.data.materials.append(door_mat)
        all_objs.append(win)

    # Windows (second floor)
    for wx in [-0.35, 0.0, 0.35]:
        bpy.ops.mesh.primitive_cube_add(size=1, location=(wx, -0.635, 1.9))
        win = bpy.context.active_object
        win.scale = (0.1, 0.04, 0.12)
        bpy.ops.object.transform_apply(scale=True)
        win.data.materials.append(door_mat)
        all_objs.append(win)

    # Chimney
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0.5, 0.3, 3.0))
    chimney = bpy.context.active_object
    chimney.scale = (0.12, 0.12, 0.35)
    bpy.ops.object.transform_apply(scale=True)
    chimney.data.materials.append(wall_mat)
    all_objs.append(chimney)

    # Join all
    bpy.ops.object.select_all(action='DESELECT')
    for o in all_objs: o.select_set(True)
    bpy.context.view_layer.objects.active = all_objs[0]
    bpy.ops.object.join()

    building = bpy.context.active_object
    building.name = "SM_RO_Building_01"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    finalize_and_export(building, "SM_RO_Building_01", "Buildings", H_BUILDING)

# ============================================================
# RUN ALL
# ============================================================
print("\n=== Generating TEXTURED RO Environment Assets ===\n")
print("1/6: Tree Tall (textured)...")
make_tree_tall()
print("2/6: Tree Wide (textured)...")
make_tree_wide()
print("3/6: Rock Large (textured)...")
make_rock_large()
print("4/6: Rock Small (textured)...")
make_rock_small()
print("5/6: Bush (textured)...")
make_bush()
print("6/6: Building (textured)...")
make_building()
print("\n=== ALL 6 TEXTURED ASSETS EXPORTED ===")
