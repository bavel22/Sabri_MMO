"""
Blender script: Generate a stylized Ragnarok Online tree.
Run via: "C:/Blender 5.1/blender.exe" --background --python "C:/Sabri_MMO/_tools/blender_make_ro_tree.py"

Creates a simple hand-painted style tree with:
- Tapered trunk (cylinder, slightly bent)
- 3-4 overlapping puffy canopy spheres (organic shape)
- Proper UVs for texture application
- Exports as FBX ready for UE5 import
"""
import bpy
import bmesh
import math
import random
import os

random.seed(42)

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# ============================================================
# Materials (vertex colors + solid base for UE5)
# ============================================================

# Trunk material - warm brown
trunk_mat = bpy.data.materials.new(name="M_Trunk")
trunk_mat.use_nodes = True
nodes = trunk_mat.node_tree.nodes
nodes.clear()
bsdf = nodes.new('ShaderNodeBsdfPrincipled')
bsdf.inputs['Base Color'].default_value = (0.35, 0.22, 0.12, 1.0)  # warm brown
bsdf.inputs['Roughness'].default_value = 0.95
output = nodes.new('ShaderNodeOutputMaterial')
trunk_mat.node_tree.links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

# Canopy material - soft green
canopy_mat = bpy.data.materials.new(name="M_Canopy")
canopy_mat.use_nodes = True
nodes = canopy_mat.node_tree.nodes
nodes.clear()
bsdf = nodes.new('ShaderNodeBsdfPrincipled')
bsdf.inputs['Base Color'].default_value = (0.28, 0.52, 0.18, 1.0)  # RO green
bsdf.inputs['Roughness'].default_value = 0.95
output = nodes.new('ShaderNodeOutputMaterial')
canopy_mat.node_tree.links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

# ============================================================
# Trunk - tapered cylinder with slight curve
# ============================================================

# Create trunk from a cone (tapered)
bpy.ops.mesh.primitive_cone_add(
    vertices=8,        # low poly (RO style)
    radius1=0.15,      # base radius
    radius2=0.06,      # top radius (tapered)
    depth=2.0,         # height
    location=(0, 0, 1.0)
)
trunk = bpy.context.active_object
trunk.name = "Trunk"
trunk.data.materials.append(trunk_mat)

# Add slight bend via proportional edit (simple lattice-like deform)
bpy.ops.object.mode_set(mode='EDIT')
bm = bmesh.from_edit_mesh(trunk.data)

# Push top vertices slightly to one side for organic feel
for v in bm.verts:
    height_factor = (v.co.z + 1.0) / 2.0  # 0 at base, 1 at top (cone centered at origin)
    v.co.x += height_factor * 0.08  # slight lean
    v.co.y += height_factor * 0.05

bmesh.update_edit_mesh(trunk.data)
bpy.ops.object.mode_set(mode='OBJECT')

# Smooth shade the trunk
bpy.ops.object.shade_smooth()

# ============================================================
# Canopy - overlapping spheres for puffy RO look
# ============================================================

canopy_spheres = [
    # (x, y, z, radius) - offset from trunk top
    (0.0,  0.0,  2.2, 0.7),    # center main
    (0.35, 0.2,  2.4, 0.55),   # right upper
    (-0.3, 0.15, 2.1, 0.6),    # left
    (0.1, -0.25, 2.5, 0.45),   # back upper
    (-0.15, 0.3, 2.6, 0.4),    # top
]

canopy_objects = []
for i, (x, y, z, r) in enumerate(canopy_spheres):
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=2,  # low poly for RO style
        radius=r,
        location=(x, y, z)
    )
    sphere = bpy.context.active_object
    sphere.name = f"Canopy_{i}"
    sphere.data.materials.append(canopy_mat)
    bpy.ops.object.shade_smooth()

    # Slightly squash vertically for puffy cloud-like shape
    sphere.scale.z *= random.uniform(0.7, 0.9)
    sphere.scale.x *= random.uniform(0.9, 1.1)
    sphere.scale.y *= random.uniform(0.9, 1.1)
    bpy.ops.object.transform_apply(scale=True)

    canopy_objects.append(sphere)

# ============================================================
# Join canopy spheres into one mesh (boolean union)
# ============================================================

# Select all canopy objects
bpy.ops.object.select_all(action='DESELECT')
for obj in canopy_objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = canopy_objects[0]
bpy.ops.object.join()
canopy = bpy.context.active_object
canopy.name = "Canopy"

# ============================================================
# Add some variation to canopy vertices for organic feel
# ============================================================

bpy.ops.object.mode_set(mode='EDIT')
bm = bmesh.from_edit_mesh(canopy.data)
for v in bm.verts:
    # Small random displacement for hand-sculpted look
    v.co.x += random.uniform(-0.03, 0.03)
    v.co.y += random.uniform(-0.03, 0.03)
    v.co.z += random.uniform(-0.02, 0.02)
bmesh.update_edit_mesh(canopy.data)
bpy.ops.object.mode_set(mode='OBJECT')

# ============================================================
# Join trunk + canopy into single mesh for export
# ============================================================

bpy.ops.object.select_all(action='DESELECT')
trunk.select_set(True)
canopy.select_set(True)
bpy.context.view_layer.objects.active = trunk
bpy.ops.object.join()

tree = bpy.context.active_object
tree.name = "SM_RO_Tree_01"

# Apply all transforms
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# ============================================================
# UV unwrap
# ============================================================

bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=66, island_margin=0.02)
bpy.ops.object.mode_set(mode='OBJECT')

# ============================================================
# Export FBX
# ============================================================

output_dir = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/Trees"
os.makedirs(output_dir, exist_ok=True)
fbx_path = os.path.join(output_dir, "SM_RO_Tree_01.fbx")

# Also save .blend for future editing
blend_dir = "C:/Sabri_MMO/_tools/3d_models"
os.makedirs(blend_dir, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(blend_dir, "ro_tree_01.blend"))

bpy.ops.export_scene.fbx(
    filepath=fbx_path,
    use_selection=False,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_ALL',
    mesh_smooth_type='FACE',
    use_mesh_modifiers=True,
    bake_space_transform=True,
    object_types={'MESH'},
    path_mode='COPY',
    embed_textures=True,
)

print(f"\n=== EXPORT COMPLETE ===")
print(f"FBX: {fbx_path}")
print(f"Blend: {os.path.join(blend_dir, 'ro_tree_01.blend')}")

# Print stats
mesh = tree.data
print(f"Vertices: {len(mesh.vertices)}")
print(f"Faces: {len(mesh.polygons)}")
print(f"Materials: {len(mesh.materials)} ({', '.join(m.name for m in mesh.materials)})")
