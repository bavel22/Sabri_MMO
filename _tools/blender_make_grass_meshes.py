"""
Create simple grass blade and flower static meshes in Blender for UE5 Landscape Grass.
Exports as FBX for UE5 import.

Run: blender --background --python blender_make_grass_meshes.py

Creates:
  - SM_GrassBlade_01.fbx — single grass blade (2 crossed planes)
  - SM_GrassBlade_02.fbx — slightly wider/shorter variant
  - SM_GrassBlade_03.fbx — thin tall variant
  - SM_GrassClump_01.fbx — 3 blades clustered
  - SM_Flower_01.fbx — small flower (cross planes with color)
  - SM_Pebble_01.fbx — small irregular rock
"""

import bpy
import bmesh
import math
import os
import random

OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/Grass/Meshes"
os.makedirs(OUTPUT_DIR, exist_ok=True)


def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()


def create_grass_blade(name, width=0.15, height=0.5, bend=0.05):
    """Create a single grass blade as two crossed planes (billboard cross)."""
    clear_scene()

    # Blade 1 — front-facing plane
    verts = [
        (-width/2, 0, 0),
        (width/2, 0, 0),
        (width/3 + bend, 0, height * 0.6),
        (-width/3 + bend, 0, height * 0.6),
        (bend * 2, 0, height),
    ]
    faces = [
        (0, 1, 2, 3),
        (3, 2, 4),
    ]

    mesh = bpy.data.meshes.new(f"{name}_mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    bm_verts = [bm.verts.new(v) for v in verts]
    for face in faces:
        bm.faces.new([bm_verts[i] for i in face])
    bm.to_mesh(mesh)
    bm.free()

    # Blade 2 — perpendicular cross (rotated 90 degrees)
    verts2 = [
        (0, -width/2, 0),
        (0, width/2, 0),
        (0, width/3 - bend, height * 0.6),
        (0, -width/3 - bend, height * 0.6),
        (0, -bend * 2, height),
    ]

    mesh2 = bpy.data.meshes.new(f"{name}_cross_mesh")
    obj2 = bpy.data.objects.new(f"{name}_cross", mesh2)
    bpy.context.collection.objects.link(obj2)

    bm2 = bmesh.new()
    bm_verts2 = [bm2.verts.new(v) for v in verts2]
    for face in faces:
        bm2.faces.new([bm_verts2[i] for i in face])
    bm2.to_mesh(mesh2)
    bm2.free()

    # Join both into one object
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    obj2.select_set(True)
    bpy.ops.object.join()

    # Add green material
    mat = bpy.data.materials.new(name=f"M_{name}")
    mat.diffuse_color = (0.25, 0.45, 0.15, 1.0)  # muted green
    obj.data.materials.append(mat)

    return obj


def create_grass_clump(name, blade_count=3):
    """Create a clump of multiple grass blades."""
    clear_scene()

    objects = []
    random.seed(42)

    for i in range(blade_count):
        # Each blade slightly different
        width = random.uniform(0.1, 0.2)
        height = random.uniform(0.35, 0.6)
        bend = random.uniform(-0.05, 0.05)

        verts = [
            (-width/2, 0, 0),
            (width/2, 0, 0),
            (width/3 + bend, 0, height * 0.6),
            (-width/3 + bend, 0, height * 0.6),
            (bend * 2, 0, height),
        ]
        faces = [(0, 1, 2, 3), (3, 2, 4)]

        mesh = bpy.data.meshes.new(f"{name}_blade_{i}")
        obj = bpy.data.objects.new(f"{name}_blade_{i}", mesh)
        bpy.context.collection.objects.link(obj)

        bm = bmesh.new()
        bm_verts = [bm.verts.new(v) for v in verts]
        for face in faces:
            bm.faces.new([bm_verts[j] for j in face])
        bm.to_mesh(mesh)
        bm.free()

        # Offset and rotate each blade randomly
        angle = (i / blade_count) * math.pi + random.uniform(-0.3, 0.3)
        offset_x = math.cos(angle) * random.uniform(0.05, 0.15)
        offset_y = math.sin(angle) * random.uniform(0.05, 0.15)
        obj.location = (offset_x, offset_y, 0)
        obj.rotation_euler = (0, 0, angle + random.uniform(-0.2, 0.2))
        objects.append(obj)

    # Join all blades
    bpy.context.view_layer.objects.active = objects[0]
    for obj in objects:
        obj.select_set(True)
    bpy.ops.object.join()

    # Green material
    mat = bpy.data.materials.new(name=f"M_{name}")
    mat.diffuse_color = (0.22, 0.40, 0.12, 1.0)
    objects[0].data.materials.append(mat)

    return objects[0]


def create_flower(name):
    """Create a small flower — cross planes with colored petals."""
    clear_scene()

    # Stem (thin vertical plane)
    verts_stem = [
        (-0.02, 0, 0), (0.02, 0, 0),
        (0.02, 0, 0.3), (-0.02, 0, 0.3),
    ]
    # Petals (small cross at top)
    petal_size = 0.08
    petal_z = 0.3
    verts_petal1 = [
        (-petal_size, 0, petal_z - petal_size),
        (petal_size, 0, petal_z - petal_size),
        (petal_size, 0, petal_z + petal_size),
        (-petal_size, 0, petal_z + petal_size),
    ]
    verts_petal2 = [
        (0, -petal_size, petal_z - petal_size),
        (0, petal_size, petal_z - petal_size),
        (0, petal_size, petal_z + petal_size),
        (0, -petal_size, petal_z + petal_size),
    ]

    all_verts = verts_stem + verts_petal1 + verts_petal2
    faces = [
        (0, 1, 2, 3),   # stem
        (4, 5, 6, 7),   # petal 1
        (8, 9, 10, 11), # petal 2
    ]

    mesh = bpy.data.meshes.new(f"{name}_mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    bm_verts = [bm.verts.new(v) for v in all_verts]
    for face in faces:
        bm.faces.new([bm_verts[i] for i in face])
    bm.to_mesh(mesh)
    bm.free()

    # Yellow-white flower material
    mat = bpy.data.materials.new(name=f"M_{name}")
    mat.diffuse_color = (0.85, 0.80, 0.45, 1.0)
    obj.data.materials.append(mat)

    return obj


def create_pebble(name):
    """Create a small irregular pebble."""
    clear_scene()

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=0.08)
    obj = bpy.context.active_object
    obj.name = name

    # Randomize vertices for irregular shape
    random.seed(123)
    mesh = obj.data
    for vert in mesh.vertices:
        vert.co.x += random.uniform(-0.02, 0.02)
        vert.co.y += random.uniform(-0.02, 0.02)
        vert.co.z *= random.uniform(0.4, 0.7)  # flatten

    # Gray-brown material
    mat = bpy.data.materials.new(name=f"M_{name}")
    mat.diffuse_color = (0.45, 0.40, 0.35, 1.0)
    obj.data.materials.append(mat)

    return obj


def export_fbx(obj, filename):
    """Export a single object as FBX."""
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    filepath = os.path.join(OUTPUT_DIR, filename)
    bpy.ops.export_scene.fbx(
        filepath=filepath,
        use_selection=True,
        global_scale=100.0,  # Blender meters to UE5 centimeters
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL',
        mesh_smooth_type='OFF',
        object_types={'MESH'},
        path_mode='COPY',
    )
    print(f"  Exported: {filepath}")


# ============================================================
# Generate all meshes
# ============================================================

print("=" * 50)
print("  Generating grass blade meshes for UE5")
print("=" * 50)

# Grass blades — 3 variants
obj = create_grass_blade("SM_GrassBlade_01", width=0.15, height=0.5, bend=0.03)
export_fbx(obj, "SM_GrassBlade_01.fbx")

obj = create_grass_blade("SM_GrassBlade_02", width=0.2, height=0.35, bend=-0.02)
export_fbx(obj, "SM_GrassBlade_02.fbx")

obj = create_grass_blade("SM_GrassBlade_03", width=0.08, height=0.6, bend=0.05)
export_fbx(obj, "SM_GrassBlade_03.fbx")

# Grass clump
obj = create_grass_clump("SM_GrassClump_01", blade_count=4)
export_fbx(obj, "SM_GrassClump_01.fbx")

# Flower
obj = create_flower("SM_Flower_01")
export_fbx(obj, "SM_Flower_01.fbx")

# Pebble
obj = create_pebble("SM_Pebble_01")
export_fbx(obj, "SM_Pebble_01.fbx")

print(f"\nDone! 6 meshes exported to {OUTPUT_DIR}")
print("Import into UE5 and assign to LandscapeGrassType assets")
