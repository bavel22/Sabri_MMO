"""
Create billboard cross meshes with proper UVs for grass sprite textures.
Each mesh is two planes at 90 degrees with UV mapping that covers the full texture.
The alpha channel in the texture handles transparency — mesh is just a carrier.

Run: "C:/Blender 5.1/blender.exe" --background --python blender_make_sprite_meshes.py
"""

import bpy
import bmesh
import math
import os

OUTPUT_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/GrassV2/Meshes"
os.makedirs(OUTPUT_DIR, exist_ok=True)


def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)
    for mat in bpy.data.materials:
        bpy.data.materials.remove(mat)


def create_billboard_cross(name, width=1.0, height=1.5):
    """
    Create a billboard cross mesh — two textured planes at 90 degrees.
    UV mapped so the full texture appears on each plane.
    Width and height are in Blender units (exported at 100x to UE5).
    """
    clear_scene()

    mesh = bpy.data.meshes.new(f"{name}_mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()

    hw = width / 2  # half width

    # Plane 1: front-facing (along X axis)
    v0 = bm.verts.new((-hw, 0, 0))
    v1 = bm.verts.new((hw, 0, 0))
    v2 = bm.verts.new((hw, 0, height))
    v3 = bm.verts.new((-hw, 0, height))
    f1 = bm.faces.new([v0, v1, v2, v3])

    # Plane 2: side-facing (along Y axis, rotated 90 degrees)
    v4 = bm.verts.new((0, -hw, 0))
    v5 = bm.verts.new((0, hw, 0))
    v6 = bm.verts.new((0, hw, height))
    v7 = bm.verts.new((0, -hw, height))
    f2 = bm.faces.new([v4, v5, v6, v7])

    # UV mapping — each plane gets full 0-1 UV coverage
    uv_layer = bm.loops.layers.uv.new("UVMap")

    # Plane 1 UVs
    for loop in f1.loops:
        vert = loop.vert
        # Map: bottom-left=0,0 bottom-right=1,0 top-right=1,1 top-left=0,1
        if vert == v0:
            loop[uv_layer].uv = (0.0, 0.0)
        elif vert == v1:
            loop[uv_layer].uv = (1.0, 0.0)
        elif vert == v2:
            loop[uv_layer].uv = (1.0, 1.0)
        elif vert == v3:
            loop[uv_layer].uv = (0.0, 1.0)

    # Plane 2 UVs (same mapping)
    for loop in f2.loops:
        vert = loop.vert
        if vert == v4:
            loop[uv_layer].uv = (0.0, 0.0)
        elif vert == v5:
            loop[uv_layer].uv = (1.0, 0.0)
        elif vert == v6:
            loop[uv_layer].uv = (1.0, 1.0)
        elif vert == v7:
            loop[uv_layer].uv = (0.0, 1.0)

    bm.to_mesh(mesh)
    bm.free()

    # Simple white material as placeholder (UE5 will override)
    mat = bpy.data.materials.new(name=f"M_{name}")
    mat.diffuse_color = (1.0, 1.0, 1.0, 1.0)
    obj.data.materials.append(mat)

    return obj


def export_fbx(obj, filename):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    filepath = os.path.join(OUTPUT_DIR, filename)
    bpy.ops.export_scene.fbx(
        filepath=filepath, use_selection=True, global_scale=100.0,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
        mesh_smooth_type='OFF', object_types={'MESH'}, path_mode='COPY')
    print(f"  {filename}")


print("=" * 50)
print("  Billboard Cross Meshes for Grass Sprites")
print("=" * 50)

# Grass blades — tall and narrow
for i in range(1, 6):
    name = f"SM_Sprite_Grass_{i:02d}"
    width = 0.3 + (i - 1) * 0.05  # slightly different widths
    height = 0.5 + (i - 1) * 0.1  # slightly different heights
    obj = create_billboard_cross(name, width, height)
    export_fbx(obj, f"{name}.fbx")

# Flowers — shorter and wider
for i in range(1, 5):
    name = f"SM_Sprite_Flower_{i:02d}"
    obj = create_billboard_cross(name, 0.3, 0.4)
    export_fbx(obj, f"{name}.fbx")

# Leaves — small and low
for i in range(1, 6):
    name = f"SM_Sprite_Leaf_{i:02d}"
    obj = create_billboard_cross(name, 0.25, 0.2)
    export_fbx(obj, f"{name}.fbx")

# Ferns/weeds
for i in range(1, 3):
    name = f"SM_Sprite_Fern_{i:02d}"
    obj = create_billboard_cross(name, 0.35, 0.35)
    export_fbx(obj, f"{name}.fbx")

# Dry grass
for i in range(1, 3):
    name = f"SM_Sprite_DryGrass_{i:02d}"
    obj = create_billboard_cross(name, 0.3, 0.45)
    export_fbx(obj, f"{name}.fbx")

# Seaweed
name = "SM_Sprite_Seaweed_01"
obj = create_billboard_cross(name, 0.2, 0.4)
export_fbx(obj, f"{name}.fbx")

total = len([f for f in os.listdir(OUTPUT_DIR) if f.endswith('.fbx')])
print(f"\nDone! {total} billboard cross meshes exported")
print(f"Each mesh has proper UVs covering full 0-1 range")
print(f"Output: {OUTPUT_DIR}")
