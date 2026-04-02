"""
Fix flat meshes — recreate all the zero-height scatter objects with:
1. A slight upward tilt (10-20 degrees) so they're visible from above
2. Billboard cross style for leaves/petals (2 planes at 90 degrees)
3. Slight curvature for shells

Run: "C:/Blender 5.1/blender.exe" --background --python blender_fix_flat_meshes.py
"""

import bpy
import bmesh
import math
import os
import random

OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/Grass/Meshes"

def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)
    for mat in bpy.data.materials:
        bpy.data.materials.remove(mat)

def make_mat(name, color):
    mat = bpy.data.materials.new(name=name)
    mat.diffuse_color = (*color, 1.0)
    return mat

def export(obj, filename):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(OUTPUT_DIR, filename),
        use_selection=True, global_scale=100.0,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
        mesh_smooth_type='OFF', object_types={'MESH'}, path_mode='COPY')
    print(f"  {filename}")

def make_tilted_leaf(name, width, length, color, tilt=0.25):
    """Leaf/petal as a tilted plane — visible from top-down camera."""
    clear_scene()
    # Two crossed planes, both tilted upward
    verts = [
        (-width/2, 0, 0),
        (width/2, 0, 0),
        (width/3, 0, length * 0.7 + tilt * 0.3),
        (-width/3, 0, length * 0.7 + tilt * 0.3),
        (0, 0, length + tilt),
        # Cross plane
        (0, -width/2, 0),
        (0, width/2, 0),
        (0, width/3, length * 0.7 + tilt * 0.3),
        (0, -width/3, length * 0.7 + tilt * 0.3),
        (0, 0, length + tilt),
    ]
    faces = [(0,1,2,3), (3,2,4), (5,6,7,8), (8,7,9)]
    mesh = bpy.data.meshes.new(f"{name}_m")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bm = bmesh.new()
    bv = [bm.verts.new(v) for v in verts]
    for f in faces:
        bm.faces.new([bv[i] for i in f])
    bm.to_mesh(mesh)
    bm.free()
    obj.data.materials.append(make_mat(f"M_{name}", color))
    return obj

def make_curved_shell(name, width, color, seed=42):
    """Shell as a slightly curved disc — not flat."""
    clear_scene()
    segments = 8
    verts = []
    random.seed(seed)
    # Center vertex raised
    verts.append((0, 0, width * 0.3))
    # Outer ring at ground level with random variation
    for i in range(segments):
        a = i * 2 * math.pi / segments
        r = width * random.uniform(0.7, 1.0)
        verts.append((math.cos(a) * r, math.sin(a) * r, random.uniform(0, width * 0.1)))
    # Fan faces from center
    faces = [(0, i+1, (i % segments) + 2) for i in range(segments - 1)]
    faces.append((0, segments, 1))

    mesh = bpy.data.meshes.new(f"{name}_m")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bm = bmesh.new()
    bv = [bm.verts.new(v) for v in verts]
    for f in faces:
        bm.faces.new([bv[i] for i in f])
    bm.to_mesh(mesh)
    bm.free()
    obj.data.materials.append(make_mat(f"M_{name}", color))
    return obj

def make_small_sphere(name, radius, color, seed=42):
    """Tiny sphere for berries etc — always visible."""
    clear_scene()
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=radius)
    obj = bpy.context.active_object
    obj.name = name
    random.seed(seed)
    for v in obj.data.vertices:
        v.co.x += random.uniform(-radius*0.2, radius*0.2)
        v.co.y += random.uniform(-radius*0.2, radius*0.2)
    obj.location.z = radius * 0.5
    obj.data.materials.append(make_mat(f"M_{name}", color))
    return obj


print("=" * 50)
print("  Fixing flat meshes — adding height/tilt")
print("=" * 50)

# Shells — curved disc instead of flat
obj = make_curved_shell("SM_Shell_01", 0.03, (0.80, 0.75, 0.65), seed=500)
export(obj, "SM_Shell_01.fbx")

obj = make_curved_shell("SM_Shell_02", 0.025, (0.75, 0.72, 0.68), seed=501)
export(obj, "SM_Shell_02.fbx")

# Clover — tilted leaf cross
obj = make_tilted_leaf("SM_Clover_01", 0.04, 0.03, (0.20, 0.38, 0.12), tilt=0.02)
export(obj, "SM_Clover_01.fbx")

# Cherry petal — tilted
obj = make_tilted_leaf("SM_CherryPetal_01", 0.02, 0.015, (0.85, 0.65, 0.70), tilt=0.01)
export(obj, "SM_CherryPetal_01.fbx")

# Bamboo leaf — tilted
obj = make_tilted_leaf("SM_BambooLeaf_01", 0.015, 0.05, (0.30, 0.45, 0.20), tilt=0.02)
export(obj, "SM_BambooLeaf_01.fbx")

# Fallen leaves — tilted cross
obj = make_tilted_leaf("SM_FallenLeaf_01", 0.03, 0.025, (0.50, 0.35, 0.15), tilt=0.015)
export(obj, "SM_FallenLeaf_01.fbx")

obj = make_tilted_leaf("SM_FallenLeaf_02", 0.025, 0.03, (0.60, 0.40, 0.12), tilt=0.02)
export(obj, "SM_FallenLeaf_02.fbx")

# Bark piece — tilted
obj = make_tilted_leaf("SM_BarkPiece_01", 0.05, 0.04, (0.32, 0.24, 0.15), tilt=0.015)
export(obj, "SM_BarkPiece_01.fbx")

# Pebble_03 — make it a proper small rock, not flat
obj = make_small_sphere("SM_Pebble_03", 0.025, (0.50, 0.45, 0.38), seed=102)
export(obj, "SM_Pebble_03.fbx")

# Metal scrap — tilted
obj = make_tilted_leaf("SM_MetalScrap_01", 0.035, 0.03, (0.40, 0.38, 0.36), tilt=0.01)
export(obj, "SM_MetalScrap_01.fbx")

# Rust flake — tilted
obj = make_tilted_leaf("SM_RustFlake_01", 0.025, 0.02, (0.55, 0.30, 0.15), tilt=0.008)
export(obj, "SM_RustFlake_01.fbx")

# Birch leaf — tilted
obj = make_tilted_leaf("SM_BirchLeaf_01", 0.02, 0.025, (0.60, 0.55, 0.22), tilt=0.015)
export(obj, "SM_BirchLeaf_01.fbx")

# Berry — proper sphere
obj = make_small_sphere("SM_Berry_01", 0.012, (0.65, 0.15, 0.12), seed=800)
export(obj, "SM_Berry_01.fbx")

# Pottery shard — tilted
obj = make_tilted_leaf("SM_PotteryShard_01", 0.03, 0.025, (0.60, 0.45, 0.30), tilt=0.01)
export(obj, "SM_PotteryShard_01.fbx")

# Starfish — curved disc
obj = make_curved_shell("SM_Starfish_01", 0.04, (0.70, 0.45, 0.35), seed=510)
export(obj, "SM_Starfish_01.fbx")

# GrassClump — this was huge from first script, make a proper small version
clear_scene()
objects = []
random.seed(42)
for i in range(4):
    width = random.uniform(0.01, 0.018)
    height = random.uniform(0.04, 0.07)
    bend = random.uniform(-0.005, 0.005)
    verts = [
        (-width/2, 0, 0), (width/2, 0, 0),
        (width/3+bend, 0, height*0.6), (-width/3+bend, 0, height*0.6),
        (bend*2, 0, height),
    ]
    faces = [(0,1,2,3), (3,2,4)]
    mesh = bpy.data.meshes.new(f"clump_blade_{i}")
    obj = bpy.data.objects.new(f"clump_blade_{i}", mesh)
    bpy.context.collection.objects.link(obj)
    bm = bmesh.new()
    bv = [bm.verts.new(v) for v in verts]
    for f in faces:
        bm.faces.new([bv[j] for j in f])
    bm.to_mesh(mesh)
    bm.free()
    angle = (i / 4) * math.pi + random.uniform(-0.3, 0.3)
    obj.location = (math.cos(angle)*0.01, math.sin(angle)*0.01, 0)
    obj.rotation_euler = (0, 0, angle)
    objects.append(obj)
bpy.context.view_layer.objects.active = objects[0]
for o in objects:
    o.select_set(True)
bpy.ops.object.join()
objects[0].name = "SM_GrassClump_01"
objects[0].data.materials.append(make_mat("M_GrassClump", (0.20, 0.35, 0.12)))
export(objects[0], "SM_GrassClump_01.fbx")

print(f"\nDone! 16 meshes re-exported with height/tilt")
