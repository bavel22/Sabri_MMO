"""
Generate ALL scatter meshes for Landscape Grass system.
60+ small static meshes for every zone type in the game.
Each mesh is 3-20 triangles — ultra lightweight for MMO.

Run: "C:/Blender 5.1/blender.exe" --background --python blender_make_all_scatter.py
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
    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)
    for mat in bpy.data.materials:
        bpy.data.materials.remove(mat)

def make_material(name, color):
    mat = bpy.data.materials.new(name=name)
    mat.diffuse_color = (*color, 1.0)
    return mat

def export(obj, filename):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUTPUT_DIR, filename)
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, global_scale=100.0,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
        mesh_smooth_type='OFF', object_types={'MESH'}, path_mode='COPY')
    print(f"  {filename}")

def make_crossed_planes(name, width, height, bend, color):
    """Billboard cross — 2 planes at 90 degrees. Base shape for grass/flowers."""
    clear_scene()
    verts = [
        (-width/2, 0, 0), (width/2, 0, 0),
        (width/3+bend, 0, height*0.6), (-width/3+bend, 0, height*0.6),
        (bend*2, 0, height),
        (0, -width/2, 0), (0, width/2, 0),
        (0, width/3-bend, height*0.6), (0, -width/3-bend, height*0.6),
        (0, -bend*2, height),
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
    obj.data.materials.append(make_material(f"M_{name}", color))
    return obj

def make_flat_shape(name, points, height, color):
    """Flat irregular shape on ground — for leaves, petals, patches."""
    clear_scene()
    verts = [(p[0], p[1], height) for p in points]
    # Fan triangulation from center
    cx = sum(p[0] for p in points) / len(points)
    cy = sum(p[1] for p in points) / len(points)
    verts.append((cx, cy, height))  # center vertex
    center_idx = len(verts) - 1
    faces = [(i, (i+1) % (len(points)), center_idx) for i in range(len(points))]

    mesh = bpy.data.meshes.new(f"{name}_m")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bm = bmesh.new()
    bv = [bm.verts.new(v) for v in verts]
    for f in faces:
        bm.faces.new([bv[i] for i in f])
    bm.to_mesh(mesh)
    bm.free()
    obj.data.materials.append(make_material(f"M_{name}", color))
    return obj

def make_rock(name, radius, flatten, color, seed=42):
    """Irregular rock/pebble from deformed icosphere."""
    clear_scene()
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=radius)
    obj = bpy.context.active_object
    obj.name = name
    random.seed(seed)
    for v in obj.data.vertices:
        v.co.x += random.uniform(-radius*0.3, radius*0.3)
        v.co.y += random.uniform(-radius*0.3, radius*0.3)
        v.co.z *= flatten
        v.co.z = max(v.co.z, 0)  # keep above ground
    obj.data.materials.append(make_material(f"M_{name}", color))
    return obj

def make_cylinder(name, radius, height, segments, color):
    """Simple cylinder for stumps, candles, etc."""
    clear_scene()
    bpy.ops.mesh.primitive_cylinder_add(vertices=segments, radius=radius, depth=height)
    obj = bpy.context.active_object
    obj.name = name
    obj.location.z = height / 2
    obj.data.materials.append(make_material(f"M_{name}", color))
    return obj

def make_cone(name, radius, height, segments, color):
    """Simple cone for small pointed objects."""
    clear_scene()
    bpy.ops.mesh.primitive_cone_add(vertices=segments, radius1=radius, radius2=0, depth=height)
    obj = bpy.context.active_object
    obj.name = name
    obj.location.z = height / 2
    obj.data.materials.append(make_material(f"M_{name}", color))
    return obj


print("=" * 60)
print("  SCATTER MESH GENERATOR — 60+ meshes")
print("=" * 60)

# ============================================================
# UNIVERSAL (all zones)
# ============================================================
print("\n--- Universal ---")

obj = make_rock("SM_Pebble_02", 0.06, 0.5, (0.42, 0.38, 0.32), seed=101)
export(obj, "SM_Pebble_02.fbx")

obj = make_rock("SM_Pebble_03", 0.04, 0.6, (0.50, 0.45, 0.38), seed=102)
export(obj, "SM_Pebble_03.fbx")

obj = make_rock("SM_SmallRock_01", 0.12, 0.4, (0.38, 0.35, 0.30), seed=103)
export(obj, "SM_SmallRock_01.fbx")

obj = make_rock("SM_SmallRock_02", 0.10, 0.5, (0.45, 0.40, 0.35), seed=104)
export(obj, "SM_SmallRock_02.fbx")

# Stick/twig
clear_scene()
bpy.ops.mesh.primitive_cylinder_add(vertices=4, radius=0.01, depth=0.4)
obj = bpy.context.active_object
obj.name = "SM_Twig_01"
obj.rotation_euler = (0.3, 0, 0.5)
obj.location.z = 0.02
obj.data.materials.append(make_material("M_Twig", (0.35, 0.28, 0.18)))
export(obj, "SM_Twig_01.fbx")

# Dirt clump
obj = make_rock("SM_DirtClump_01", 0.05, 0.3, (0.35, 0.28, 0.18), seed=110)
export(obj, "SM_DirtClump_01.fbx")

# ============================================================
# GRASSLAND / FIELD
# ============================================================
print("\n--- Grassland ---")

# Additional grass variants
obj = make_crossed_planes("SM_GrassBlade_04", 0.12, 0.4, 0.04, (0.25, 0.42, 0.16))
export(obj, "SM_GrassBlade_04.fbx")

obj = make_crossed_planes("SM_GrassBlade_05", 0.18, 0.3, -0.03, (0.28, 0.45, 0.18))
export(obj, "SM_GrassBlade_05.fbx")

# Wheat/grain stalk
obj = make_crossed_planes("SM_WheatStalk_01", 0.06, 0.7, 0.08, (0.65, 0.55, 0.25))
export(obj, "SM_WheatStalk_01.fbx")

# Clover — flat 3-leaf shape
pts = [(0, 0.04), (0.035, -0.02), (-0.035, -0.02)]
obj = make_flat_shape("SM_Clover_01", pts, 0.01, (0.20, 0.38, 0.12))
export(obj, "SM_Clover_01.fbx")

# Dandelion
obj = make_crossed_planes("SM_Dandelion_01", 0.08, 0.25, 0.01, (0.80, 0.75, 0.20))
export(obj, "SM_Dandelion_01.fbx")

# Wildflower variants
obj = make_crossed_planes("SM_Flower_02", 0.10, 0.30, 0.02, (0.75, 0.55, 0.65))
export(obj, "SM_Flower_02.fbx")

obj = make_crossed_planes("SM_Flower_03", 0.08, 0.20, -0.01, (0.60, 0.60, 0.80))
export(obj, "SM_Flower_03.fbx")

# ============================================================
# FOREST
# ============================================================
print("\n--- Forest ---")

# Mushroom (cone cap + cylinder stem)
clear_scene()
bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=0.06, radius2=0.02, depth=0.04)
cap = bpy.context.active_object
cap.name = "SM_Mushroom_01"
cap.location.z = 0.10
bpy.ops.mesh.primitive_cylinder_add(vertices=4, radius=0.015, depth=0.08)
stem = bpy.context.active_object
stem.location.z = 0.04
stem.select_set(True)
cap.select_set(True)
bpy.context.view_layer.objects.active = cap
bpy.ops.object.join()
cap.data.materials.append(make_material("M_Mushroom", (0.55, 0.35, 0.20)))
export(cap, "SM_Mushroom_01.fbx")

# Mushroom variant
clear_scene()
bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=0.04, radius2=0.01, depth=0.03)
cap = bpy.context.active_object
cap.name = "SM_Mushroom_02"
cap.location.z = 0.08
bpy.ops.mesh.primitive_cylinder_add(vertices=4, radius=0.01, depth=0.06)
stem = bpy.context.active_object
stem.location.z = 0.03
stem.select_set(True)
cap.select_set(True)
bpy.context.view_layer.objects.active = cap
bpy.ops.object.join()
cap.data.materials.append(make_material("M_Mushroom2", (0.70, 0.30, 0.25)))
export(cap, "SM_Mushroom_02.fbx")

# Fern (small crossed planes, green)
obj = make_crossed_planes("SM_Fern_01", 0.20, 0.15, 0.02, (0.15, 0.35, 0.10))
export(obj, "SM_Fern_01.fbx")

# Fallen leaf (single)
pts = [(0, 0.03), (0.025, 0.01), (0.02, -0.02), (0, -0.03), (-0.02, -0.02), (-0.025, 0.01)]
obj = make_flat_shape("SM_FallenLeaf_01", pts, 0.005, (0.50, 0.35, 0.15))
export(obj, "SM_FallenLeaf_01.fbx")

obj = make_flat_shape("SM_FallenLeaf_02", pts, 0.005, (0.60, 0.40, 0.12))
export(obj, "SM_FallenLeaf_02.fbx")

# Pine cone
obj = make_rock("SM_PineCone_01", 0.03, 0.7, (0.40, 0.30, 0.18), seed=200)
export(obj, "SM_PineCone_01.fbx")

# Bark piece
pts = [(0, 0.04), (0.05, 0.02), (0.04, -0.03), (-0.02, -0.04), (-0.05, 0.01)]
obj = make_flat_shape("SM_BarkPiece_01", pts, 0.008, (0.32, 0.24, 0.15))
export(obj, "SM_BarkPiece_01.fbx")

# Acorn
obj = make_rock("SM_Acorn_01", 0.02, 0.8, (0.45, 0.35, 0.20), seed=210)
export(obj, "SM_Acorn_01.fbx")

# Moss clump (flat green blob)
obj = make_rock("SM_MossClump_01", 0.08, 0.2, (0.18, 0.32, 0.12), seed=220)
export(obj, "SM_MossClump_01.fbx")

# ============================================================
# DESERT
# ============================================================
print("\n--- Desert ---")

# Dry grass tuft (yellow-brown)
obj = make_crossed_planes("SM_DryGrass_01", 0.15, 0.35, 0.06, (0.60, 0.50, 0.25))
export(obj, "SM_DryGrass_01.fbx")

obj = make_crossed_planes("SM_DryGrass_02", 0.12, 0.25, -0.04, (0.55, 0.45, 0.22))
export(obj, "SM_DryGrass_02.fbx")

# Desert rock (warm brown)
obj = make_rock("SM_DesertRock_01", 0.10, 0.45, (0.55, 0.42, 0.28), seed=300)
export(obj, "SM_DesertRock_01.fbx")

obj = make_rock("SM_DesertRock_02", 0.07, 0.5, (0.60, 0.48, 0.32), seed=301)
export(obj, "SM_DesertRock_02.fbx")

# Dry twig (tan)
clear_scene()
bpy.ops.mesh.primitive_cylinder_add(vertices=4, radius=0.008, depth=0.3)
obj = bpy.context.active_object
obj.name = "SM_DryTwig_01"
obj.rotation_euler = (0.4, 0, 0.8)
obj.location.z = 0.01
obj.data.materials.append(make_material("M_DryTwig", (0.55, 0.45, 0.30)))
export(obj, "SM_DryTwig_01.fbx")

# Small cactus
obj = make_cone("SM_SmallCactus_01", 0.03, 0.15, 5, (0.25, 0.40, 0.18))
export(obj, "SM_SmallCactus_01.fbx")

# Bone fragment (pale)
obj = make_rock("SM_BoneFrag_01", 0.03, 0.6, (0.75, 0.72, 0.65), seed=310)
export(obj, "SM_BoneFrag_01.fbx")

# ============================================================
# SNOW
# ============================================================
print("\n--- Snow ---")

# Snow clump
obj = make_rock("SM_SnowClump_01", 0.10, 0.3, (0.90, 0.92, 0.95), seed=400)
export(obj, "SM_SnowClump_01.fbx")

obj = make_rock("SM_SnowClump_02", 0.06, 0.4, (0.85, 0.88, 0.92), seed=401)
export(obj, "SM_SnowClump_02.fbx")

# Ice crystal (pointy cone)
obj = make_cone("SM_IceCrystal_01", 0.02, 0.12, 4, (0.70, 0.80, 0.92))
export(obj, "SM_IceCrystal_01.fbx")

obj = make_cone("SM_IceCrystal_02", 0.015, 0.08, 4, (0.75, 0.85, 0.95))
export(obj, "SM_IceCrystal_02.fbx")

# Frozen twig
clear_scene()
bpy.ops.mesh.primitive_cylinder_add(vertices=4, radius=0.008, depth=0.25)
obj = bpy.context.active_object
obj.name = "SM_FrozenTwig_01"
obj.rotation_euler = (0.3, 0, 0.6)
obj.location.z = 0.01
obj.data.materials.append(make_material("M_FrozenTwig", (0.60, 0.65, 0.72)))
export(obj, "SM_FrozenTwig_01.fbx")

# ============================================================
# BEACH
# ============================================================
print("\n--- Beach ---")

# Seashell (flat spiral-ish shape)
pts = [(0, 0.03), (0.025, 0.015), (0.03, -0.01), (0.01, -0.025), (-0.015, -0.02), (-0.025, 0.005)]
obj = make_flat_shape("SM_Shell_01", pts, 0.008, (0.80, 0.75, 0.65))
export(obj, "SM_Shell_01.fbx")

obj = make_flat_shape("SM_Shell_02", pts, 0.006, (0.75, 0.72, 0.68))
export(obj, "SM_Shell_02.fbx")

# Coral bit
obj = make_rock("SM_CoralBit_01", 0.04, 0.6, (0.75, 0.55, 0.50), seed=500)
export(obj, "SM_CoralBit_01.fbx")

# Starfish (flat 5-point)
pts = []
for i in range(5):
    a = i * 2 * math.pi / 5 - math.pi/2
    pts.append((math.cos(a)*0.04, math.sin(a)*0.04))
    a2 = a + math.pi/5
    pts.append((math.cos(a2)*0.02, math.sin(a2)*0.02))
obj = make_flat_shape("SM_Starfish_01", pts, 0.005, (0.70, 0.45, 0.35))
export(obj, "SM_Starfish_01.fbx")

# Driftwood
clear_scene()
bpy.ops.mesh.primitive_cylinder_add(vertices=5, radius=0.02, depth=0.25)
obj = bpy.context.active_object
obj.name = "SM_Driftwood_01"
obj.rotation_euler = (1.5, 0, 0.7)
obj.location.z = 0.02
obj.data.materials.append(make_material("M_Driftwood", (0.50, 0.45, 0.38)))
export(obj, "SM_Driftwood_01.fbx")

# Seaweed strand
obj = make_crossed_planes("SM_Seaweed_01", 0.06, 0.12, 0.03, (0.20, 0.35, 0.18))
export(obj, "SM_Seaweed_01.fbx")

# ============================================================
# VOLCANIC
# ============================================================
print("\n--- Volcanic ---")

# Obsidian shard (dark pointy)
obj = make_cone("SM_ObsidianShard_01", 0.025, 0.10, 4, (0.08, 0.08, 0.10))
export(obj, "SM_ObsidianShard_01.fbx")

obj = make_cone("SM_ObsidianShard_02", 0.02, 0.07, 3, (0.10, 0.08, 0.08))
export(obj, "SM_ObsidianShard_02.fbx")

# Lava rock (dark with slight red)
obj = make_rock("SM_LavaRock_01", 0.08, 0.5, (0.15, 0.10, 0.08), seed=600)
export(obj, "SM_LavaRock_01.fbx")

obj = make_rock("SM_LavaRock_02", 0.06, 0.6, (0.18, 0.12, 0.08), seed=601)
export(obj, "SM_LavaRock_02.fbx")

# Sulfur crystal (yellow)
obj = make_cone("SM_SulfurCrystal_01", 0.02, 0.06, 4, (0.70, 0.65, 0.15))
export(obj, "SM_SulfurCrystal_01.fbx")

# Ash clump
obj = make_rock("SM_AshClump_01", 0.05, 0.25, (0.30, 0.28, 0.28), seed=610)
export(obj, "SM_AshClump_01.fbx")

# ============================================================
# CURSED / UNDEAD
# ============================================================
print("\n--- Cursed/Undead ---")

# Bone fragment
obj = make_rock("SM_Bone_01", 0.04, 0.7, (0.78, 0.75, 0.68), seed=700)
export(obj, "SM_Bone_01.fbx")

clear_scene()
bpy.ops.mesh.primitive_cylinder_add(vertices=4, radius=0.01, depth=0.15)
obj = bpy.context.active_object
obj.name = "SM_BoneStick_01"
obj.rotation_euler = (0.5, 0, 0.3)
obj.location.z = 0.01
obj.data.materials.append(make_material("M_BoneStick", (0.75, 0.72, 0.65)))
export(obj, "SM_BoneStick_01.fbx")

# Small skull
obj = make_rock("SM_SmallSkull_01", 0.03, 0.7, (0.72, 0.68, 0.62), seed=710)
export(obj, "SM_SmallSkull_01.fbx")

# Candle stub
obj = make_cylinder("SM_CandleStub_01", 0.015, 0.05, 5, (0.75, 0.70, 0.55))
export(obj, "SM_CandleStub_01.fbx")

# Dead grass (gray-brown)
obj = make_crossed_planes("SM_DeadGrass_01", 0.12, 0.25, 0.05, (0.40, 0.35, 0.28))
export(obj, "SM_DeadGrass_01.fbx")

obj = make_crossed_planes("SM_DeadGrass_02", 0.10, 0.20, -0.03, (0.35, 0.30, 0.25))
export(obj, "SM_DeadGrass_02.fbx")

# ============================================================
# JAPANESE (AMATSU)
# ============================================================
print("\n--- Amatsu ---")

# Cherry blossom petal (tiny flat pink)
pts = [(0, 0.015), (0.012, 0), (0, -0.015), (-0.012, 0)]
obj = make_flat_shape("SM_CherryPetal_01", pts, 0.003, (0.85, 0.65, 0.70))
export(obj, "SM_CherryPetal_01.fbx")

# Bamboo leaf
pts = [(0, 0.04), (0.008, 0), (0, -0.04), (-0.008, 0)]
obj = make_flat_shape("SM_BambooLeaf_01", pts, 0.005, (0.30, 0.45, 0.20))
export(obj, "SM_BambooLeaf_01.fbx")

# ============================================================
# RUSSIAN (MOSCOVIA)
# ============================================================
print("\n--- Moscovia ---")

# Birch leaf (yellow-green)
pts = [(0, 0.02), (0.015, 0.005), (0.01, -0.015), (0, -0.02), (-0.01, -0.015), (-0.015, 0.005)]
obj = make_flat_shape("SM_BirchLeaf_01", pts, 0.005, (0.60, 0.55, 0.22))
export(obj, "SM_BirchLeaf_01.fbx")

# Berry (tiny red sphere)
obj = make_rock("SM_Berry_01", 0.01, 0.9, (0.65, 0.15, 0.12), seed=800)
export(obj, "SM_Berry_01.fbx")

# ============================================================
# RUINS / ANCIENT
# ============================================================
print("\n--- Ruins ---")

# Stone fragment (warm)
obj = make_rock("SM_StoneFrag_01", 0.06, 0.4, (0.55, 0.50, 0.42), seed=900)
export(obj, "SM_StoneFrag_01.fbx")

obj = make_rock("SM_StoneFrag_02", 0.04, 0.5, (0.50, 0.45, 0.38), seed=901)
export(obj, "SM_StoneFrag_02.fbx")

# Pottery shard
pts = [(0, 0.03), (0.025, 0.01), (0.02, -0.02), (-0.01, -0.025), (-0.025, 0.01)]
obj = make_flat_shape("SM_PotteryShard_01", pts, 0.006, (0.60, 0.45, 0.30))
export(obj, "SM_PotteryShard_01.fbx")

# ============================================================
# INDUSTRIAL
# ============================================================
print("\n--- Industrial ---")

# Metal scrap (flat irregular)
pts = [(0, 0.03), (0.03, 0.015), (0.025, -0.02), (-0.01, -0.03), (-0.03, 0.005)]
obj = make_flat_shape("SM_MetalScrap_01", pts, 0.004, (0.40, 0.38, 0.36))
export(obj, "SM_MetalScrap_01.fbx")

# Bolt (tiny cylinder)
obj = make_cylinder("SM_Bolt_01", 0.008, 0.02, 6, (0.45, 0.42, 0.38))
export(obj, "SM_Bolt_01.fbx")

# Rust flake
pts = [(0, 0.02), (0.018, 0.005), (0.015, -0.015), (-0.005, -0.018), (-0.018, 0.005)]
obj = make_flat_shape("SM_RustFlake_01", pts, 0.003, (0.55, 0.30, 0.15))
export(obj, "SM_RustFlake_01.fbx")

# ============================================================
# CAVE
# ============================================================
print("\n--- Cave ---")

# Crystal shard (blue-purple)
obj = make_cone("SM_CrystalShard_01", 0.015, 0.08, 4, (0.45, 0.40, 0.65))
export(obj, "SM_CrystalShard_01.fbx")

obj = make_cone("SM_CrystalShard_02", 0.012, 0.06, 4, (0.50, 0.42, 0.70))
export(obj, "SM_CrystalShard_02.fbx")

# Wet rock (dark)
obj = make_rock("SM_WetRock_01", 0.07, 0.5, (0.22, 0.20, 0.22), seed=1000)
export(obj, "SM_WetRock_01.fbx")

# Stalagmite nub (small cone)
obj = make_cone("SM_StalagmiteNub_01", 0.03, 0.08, 5, (0.35, 0.32, 0.30))
export(obj, "SM_StalagmiteNub_01.fbx")

# ============================================================
# DUNGEON
# ============================================================
print("\n--- Dungeon ---")

# Rubble
obj = make_rock("SM_Rubble_01", 0.05, 0.35, (0.35, 0.33, 0.32), seed=1100)
export(obj, "SM_Rubble_01.fbx")

obj = make_rock("SM_Rubble_02", 0.03, 0.4, (0.32, 0.30, 0.30), seed=1101)
export(obj, "SM_Rubble_02.fbx")

# Chain link piece
clear_scene()
bpy.ops.mesh.primitive_torus_add(major_radius=0.02, minor_radius=0.005, major_segments=6, minor_segments=4)
obj = bpy.context.active_object
obj.name = "SM_ChainLink_01"
obj.location.z = 0.005
obj.data.materials.append(make_material("M_ChainLink", (0.30, 0.28, 0.28)))
export(obj, "SM_ChainLink_01.fbx")

print(f"\n{'='*60}")
total = len([f for f in os.listdir(OUTPUT_DIR) if f.endswith('.fbx')])
print(f"  DONE: {total} total FBX meshes in {OUTPUT_DIR}")
print(f"{'='*60}")
