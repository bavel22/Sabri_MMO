"""
Blender Sprite Renderer v2 for Sabri_MMO
=========================================
Renders 3D models (GLB/FBX) into 8-direction sprite frames with animations.
Color-preserving cel-shading, Mixamo FBX animation support, auto-camera framing,
and batch processing.

Single model:
  "C:/Blender 5.1/blender.exe" --background --python blender_sprite_render_v2.py -- model.glb output_dir

With Mixamo animations:
  ... -- model.fbx output_dir --anims walk.fbx attack.fbx death.fbx

Batch mode (all models in a directory):
  ... -- --batch models_dir/ output_dir/

Animation directory (apply same anims to all models):
  ... -- --batch models_dir/ output_dir/ --anim-dir anims_dir/

Options:
  --directions 8        Number of directions (4 or 8, default 8)
  --render-size 512     Internal render resolution (default 512)
  --no-cel-shade        Disable cel-shading
  --no-outline          Disable black outline
  --outline-width 0.002 Outline thickness (default 0.002)
  --cel-shadow 0.45     Shadow brightness 0-1 (default 0.45)
  --cel-mid 0.78        Midtone brightness 0-1 (default 0.78)
  --camera-angle 30     Camera elevation degrees (default 30, RO-style)
  --idle-frames 8       Frames to sample for idle (default 8)
  --walk-frames 12      Frames to sample for walk (default 12)
  --attack-frames 10    Frames to sample for attack (default 10)
  --death-frames 8      Frames to sample for death (default 8)
"""

import bpy
import math
import os
import sys
import argparse
import mathutils


# ============================================================
# Constants
# ============================================================

DIRECTION_NAMES_8 = ["S", "SW", "W", "NW", "N", "NE", "E", "SE"]
DIRECTION_NAMES_4 = ["S", "W", "N", "E"]

# Mixamo animation name → standard type
ANIM_NAME_MAP = {
    "idle": "idle", "breathing idle": "idle", "happy idle": "idle",
    "standing idle": "idle", "warrior idle": "idle",
    "walking": "walk", "walk": "walk", "slow walk": "walk",
    "walking in place": "walk",
    "running": "run", "run": "run", "jogging": "run",
    "run with sword": "run",
    "attack": "attack", "slash": "attack",
    "sword and shield slash": "attack", "great sword slash": "attack_2h",
    "punching": "attack", "standing melee attack horizontal": "attack",
    "sword and shield attack": "attack", "mutant punch": "attack",
    "stable sword inward slash": "attack", "stable sword outward slash": "attack",
    "casting spell": "cast", "standing 1h magic attack 01": "cast",
    "getting hit": "hit", "hit": "hit", "damage": "hit",
    "standing react small from front": "hit", "reaction": "hit",
    "death": "death", "dying": "death", "dying backwards": "death",
    "falling back death": "death", "death backwards": "death",
    # New types for RO Classic
    "sitting idle": "sit", "sitting": "sit",
    "fighting idle": "standby", "2hand idle": "standby_2h",
    "picking up": "pickup",
    "sword and shield block": "block",
    "standing taunt chest thump": "taunt",
    # Caster-specific: "magic attack" FBX files are cast, not attack
    "standing 1h magic attack 02": "cast",
    "standing 1h magic attack 03": "cast",
    "standing 1h cast spell 01": "cast",
    "standing 2h magic attack 01": "cast",
    "standing 2h magic attack 02": "cast",
    "standing 2h magic attack 03": "cast",
    "standing 2h magic attack 04": "cast",
    "standing 2h magic attack 05": "cast",
    "standing 2h magic area attack 01": "cast",
    "standing 2h magic area attack 02": "cast",
    "standing 2h cast spell 01": "cast",
    "great sword casting": "cast",
    "two hand spell casting": "cast",
    "wide arm spell casting": "cast",
    "spell cast": "cast", "spell casting": "cast",
    "unarmed idle 01": "idle",
    "run with sword (1)": "run",
    "stable sword inward slash (1)": "attack",
    "stable sword outward slash (1)": "attack",
}

DEFAULT_FRAME_TARGETS = {
    "idle": 8,
    "walk": 12,
    "run": 12,
    "attack": 10,
    "attack_2h": 10,
    "cast": 8,
    "hit": 6,
    "death": 8,
    "sit": 4,
    "standby": 8,
    "standby_2h": 8,
    "pickup": 8,
    "block": 6,
    "taunt": 8,
}


# ============================================================
# Argument Parsing
# ============================================================

def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser(description="Render 3D models to sprite sheets")
    parser.add_argument("input", help="Model file (GLB/FBX) or directory for --batch")
    parser.add_argument("output", nargs="?",
                        default="C:/Sabri_MMO/2D animations/sprites/render_output",
                        help="Output directory")
    parser.add_argument("--batch", action="store_true",
                        help="Process all models in input directory")
    parser.add_argument("--directions", type=int, default=8, choices=[4, 8])
    parser.add_argument("--render-size", type=int, default=512)
    parser.add_argument("--no-cel-shade", action="store_true")
    parser.add_argument("--no-outline", action="store_true")
    parser.add_argument("--outline-width", type=float, default=0.002)
    parser.add_argument("--cel-shadow", type=float, default=0.45,
                        help="Shadow brightness 0-1 (higher = brighter shadows)")
    parser.add_argument("--cel-mid", type=float, default=0.78,
                        help="Midtone brightness 0-1")
    parser.add_argument("--anims", nargs="*",
                        help="Additional FBX animation files")
    parser.add_argument("--anim-dir",
                        help="Directory of FBX animation files")
    parser.add_argument("--idle-frames", type=int,
                        default=DEFAULT_FRAME_TARGETS["idle"])
    parser.add_argument("--walk-frames", type=int,
                        default=DEFAULT_FRAME_TARGETS["walk"])
    parser.add_argument("--attack-frames", type=int,
                        default=DEFAULT_FRAME_TARGETS["attack"])
    parser.add_argument("--death-frames", type=int,
                        default=DEFAULT_FRAME_TARGETS["death"])
    parser.add_argument("--camera-angle", type=float, default=30.0,
                        help="Camera elevation in degrees (default 30)")
    parser.add_argument("--camera-target-z", type=float, default=1.0,
                        help="Camera look-at Z height (default 1.0 = model center)")
    parser.add_argument("--texture-from",
                        help="GLB/FBX file to copy textures from (for Mixamo FBX "
                             "that lost textures)")
    parser.add_argument("--subfolders", action="store_true",
                        help="Organize output into subfolders per animation type "
                             "(e.g., idle/, walk/, attack/)")
    parser.add_argument("--equipment",
                        help="GLB/FBX file of equipment to attach to skeleton. "
                             "Body mesh is hidden, only equipment renders.")
    parser.add_argument("--attach-bone", default="mixamorig:RightHand",
                        help="Bone name to parent equipment to (default: mixamorig:RightHand)")
    parser.add_argument("--equip-offset", nargs=3, type=float, default=[0, 0, 0],
                        help="Equipment position offset (X Y Z) relative to bone")
    parser.add_argument("--equip-rotation", nargs=3, type=float, default=[0, 0, 0],
                        help="Equipment rotation offset (X Y Z degrees) relative to bone")
    parser.add_argument("--equip-scale", nargs='+', type=float, default=[1.0],
                        help="Equipment scale — 1 value for uniform, 3 values for X Y Z")
    parser.add_argument("--equip-pose-fbx",
                        help="FBX file used when positioning equipment in Blender. "
                             "The script sets the armature to this animation's pose "
                             "before computing bone transforms for correct alignment.")

    return parser.parse_args(argv)


# ============================================================
# Scene Setup
# ============================================================

def clear_scene():
    """Remove all objects and orphan data from scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

    for collection in [bpy.data.meshes, bpy.data.materials,
                       bpy.data.armatures, bpy.data.cameras,
                       bpy.data.lights, bpy.data.actions]:
        for block in list(collection):
            if block.users == 0:
                collection.remove(block)


def import_model(filepath):
    """Import GLB/FBX/OBJ model. Returns (armature, meshes)."""
    ext = os.path.splitext(filepath)[1].lower()
    before = set(bpy.context.scene.objects)

    if ext in ('.glb', '.gltf'):
        bpy.ops.import_scene.gltf(filepath=filepath)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=filepath)
    elif ext == '.obj':
        bpy.ops.wm.obj_import(filepath=filepath)
    else:
        print(f"[ERROR] Unsupported format: {ext}")
        return None, []

    new_objects = set(bpy.context.scene.objects) - before
    armature = None
    meshes = []
    for obj in new_objects:
        if obj.type == 'ARMATURE':
            armature = obj
        elif obj.type == 'MESH':
            meshes.append(obj)

    print(f"[OK] Imported {os.path.basename(filepath)}: "
          f"armature={'yes' if armature else 'no'}, meshes={len(meshes)}")
    return armature, meshes


def recover_textures(texture_source_path, target_meshes):
    """
    Import a GLB/FBX that has textures, copy its materials to the target meshes.
    Works perfectly when source (GLB) and target (FBX) have the same topology
    (same vertex count = same UVs), which is the case when the GLB is the exact
    model that was uploaded to Mixamo.
    """
    before = set(bpy.context.scene.objects)
    ext = os.path.splitext(texture_source_path)[1].lower()

    if ext in ('.glb', '.gltf'):
        bpy.ops.import_scene.gltf(filepath=texture_source_path)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=texture_source_path)
    else:
        print(f"[WARN] Unsupported texture source: {ext}")
        return

    new_objects = set(bpy.context.scene.objects) - before
    source_meshes = [o for o in new_objects if o.type == 'MESH']

    if not source_meshes:
        print("[WARN] No meshes in texture source file")
        return

    # Collect all materials from the source
    source_materials = []
    for src in source_meshes:
        for slot in src.material_slots:
            if slot.material:
                source_materials.append(slot.material)

    if not source_materials:
        print("[WARN] No materials found in texture source")
        for obj in new_objects:
            bpy.data.objects.remove(obj, do_unlink=True)
        return

    # Apply source materials to target meshes
    for tgt in target_meshes:
        tgt.data.materials.clear()
        for mat in source_materials:
            tgt.data.materials.append(mat)

    # Delete the imported source objects
    for obj in new_objects:
        bpy.data.objects.remove(obj, do_unlink=True)

    print(f"[OK] Recovered {len(source_materials)} material(s) from "
          f"{os.path.basename(texture_source_path)}")


# Store filename-based classification so Blender's auto-renaming doesn't lose it
_action_classification = {}

def import_animation_fbx(filepath, base_armature):
    """Import FBX, steal its animation action, delete imported objects."""
    before_actions = set(bpy.data.actions)
    before_objects = set(bpy.context.scene.objects)

    bpy.ops.import_scene.fbx(filepath=filepath)

    new_actions = set(bpy.data.actions) - before_actions
    new_objects = set(bpy.context.scene.objects) - before_objects

    action = None
    if new_actions:
        action = list(new_actions)[0]
        action.use_fake_user = True  # prevent garbage collection

        # Classify from the ORIGINAL filename (not Blender's internal name)
        basename = os.path.splitext(os.path.basename(filepath))[0].lower()
        for suffix in [" (1)", "_mixamo", " mixamo"]:
            basename = basename.replace(suffix, "")
        mapped = classify_animation(basename)

        # Use a unique action name that includes the original filename
        # to prevent Blender from colliding names (idle → idle.001)
        safe_basename = os.path.splitext(os.path.basename(filepath))[0]
        action.name = safe_basename

        # Store the classification keyed by action name
        _action_classification[action.name] = mapped

        print(f"[OK] Animation: {os.path.basename(filepath)} -> '{mapped}' "
              f"(frames {int(action.frame_range[0])}-{int(action.frame_range[1])})")

    # Delete imported objects, keep only the action
    for obj in new_objects:
        bpy.data.objects.remove(obj, do_unlink=True)

    return action


# ============================================================
# Model Preparation
# ============================================================

def get_scene_bounds():
    """World-space bounding box of all mesh objects (evaluated/deformed)."""
    min_co = mathutils.Vector((float('inf'),) * 3)
    max_co = mathutils.Vector((float('-inf'),) * 3)

    depsgraph = bpy.context.evaluated_depsgraph_get()

    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        eval_obj = obj.evaluated_get(depsgraph)
        try:
            mesh = eval_obj.to_mesh()
        except RuntimeError:
            continue

        for vert in mesh.vertices:
            world_co = obj.matrix_world @ vert.co
            min_co.x = min(min_co.x, world_co.x)
            min_co.y = min(min_co.y, world_co.y)
            min_co.z = min(min_co.z, world_co.z)
            max_co.x = max(max_co.x, world_co.x)
            max_co.y = max(max_co.y, world_co.y)
            max_co.z = max(max_co.z, world_co.z)

        eval_obj.to_mesh_clear()

    return min_co, max_co


def center_and_normalize():
    """Center model at origin, feet at Z=0, normalize height to 2.0 units."""
    min_co, max_co = get_scene_bounds()
    if min_co.x == float('inf'):
        print("[WARN] No mesh objects found for centering")
        return

    center = (min_co + max_co) / 2
    height = max_co.z - min_co.z

    # Offset: center XY, feet at Z=0
    offset = mathutils.Vector((-center.x, -center.y, -min_co.z))
    roots = [o for o in bpy.context.scene.objects if o.parent is None]
    for obj in roots:
        obj.location += offset

    bpy.context.view_layer.update()

    # Normalize height to 2.0
    if height > 0.001:
        scale_factor = 2.0 / height
        for obj in roots:
            obj.scale *= scale_factor
        bpy.context.view_layer.update()

    print(f"[OK] Centered (height {height:.3f} -> 2.0)")


# ============================================================
# Cel-Shading (Color-Preserving)
# ============================================================

def setup_cel_shade(shadow_brightness=0.45, mid_brightness=0.78):
    """
    Color-preserving cel-shading.

    Instead of replacing colors with fixed grays (which crushes dark models),
    this extracts the lighting factor from the fully-lit result, quantizes it
    into 4 brightness steps, then multiplies back against the original base
    color. Result: original colors preserved, only lighting is stepped.

    Brightness stops:
      deep shadow  = shadow * 0.65
      shadow       = shadow_brightness   (default 0.45)
      midtone      = mid_brightness      (default 0.78)
      highlight    = 1.0
    """
    processed = 0
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue

        for slot in obj.material_slots:
            mat = slot.material
            if not mat or not mat.use_nodes:
                continue

            tree = mat.node_tree
            nodes = tree.nodes
            links = tree.links

            # Find Principled BSDF
            principled = None
            for node in nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    principled = node
                    break
            if not principled:
                continue

            # Find Material Output
            output_node = None
            for node in nodes:
                if node.type == 'OUTPUT_MATERIAL':
                    output_node = node
                    break
            if not output_node:
                continue

            # Capture base color source before we rewire
            base_color_source = None
            for link in links:
                if link.to_socket == principled.inputs['Base Color']:
                    base_color_source = link.from_socket
                    break
            base_color_value = principled.inputs['Base Color'].default_value[:]

            # --- Node chain ---
            px = principled.location.x
            py = principled.location.y

            # 1. Principled BSDF -> Shader to RGB (captures full lighting)
            s2rgb = nodes.new('ShaderNodeShaderToRGB')
            s2rgb.location = (px + 300, py)
            links.new(principled.outputs['BSDF'], s2rgb.inputs['Shader'])

            # 2. RGB to BW (extract luminance from lit result)
            rgb2bw = nodes.new('ShaderNodeRGBToBW')
            rgb2bw.location = (px + 500, py - 150)
            links.new(s2rgb.outputs['Color'], rgb2bw.inputs['Color'])

            # 3. ColorRamp (quantize luminance into 4 cel-shade steps)
            ramp = nodes.new('ShaderNodeValToRGB')
            ramp.location = (px + 700, py - 150)
            ramp.color_ramp.interpolation = 'CONSTANT'

            cr = ramp.color_ramp
            ds = shadow_brightness * 0.65  # deep shadow
            cr.elements[0].position = 0.0
            cr.elements[0].color = (ds, ds, ds, 1.0)
            cr.elements[1].position = 0.20
            cr.elements[1].color = (shadow_brightness, shadow_brightness,
                                    shadow_brightness, 1.0)
            e_mid = cr.elements.new(0.50)
            e_mid.color = (mid_brightness, mid_brightness, mid_brightness, 1.0)
            e_hi = cr.elements.new(0.75)
            e_hi.color = (1.0, 1.0, 1.0, 1.0)

            links.new(rgb2bw.outputs['Val'], ramp.inputs['Fac'])

            # 4. Mix (MULTIPLY): quantized lighting x base color
            mix = nodes.new('ShaderNodeMix')
            mix.data_type = 'RGBA'
            mix.blend_type = 'MULTIPLY'
            mix.location = (px + 950, py)
            mix.inputs[0].default_value = 1.0  # factor = full

            links.new(ramp.outputs['Color'], mix.inputs[6])   # A
            if base_color_source:
                links.new(base_color_source, mix.inputs[7])    # B
            else:
                mix.inputs[7].default_value = base_color_value

            # 5. Emission (avoid double-lighting from Eevee)
            emission = nodes.new('ShaderNodeEmission')
            emission.location = (px + 1200, py)
            links.new(mix.outputs[2], emission.inputs['Color'])

            # 6. Handle alpha transparency
            alpha_source = None
            for link in list(links):
                if link.to_socket == principled.inputs['Alpha']:
                    alpha_source = link.from_socket
                    break
            has_alpha = (alpha_source is not None or
                         principled.inputs['Alpha'].default_value < 0.999)

            # Remove old connection to output
            for link in list(links):
                if (link.to_node == output_node and
                        link.to_socket == output_node.inputs['Surface']):
                    links.remove(link)

            if has_alpha:
                transparent = nodes.new('ShaderNodeBsdfTransparent')
                transparent.location = (px + 1200, py - 200)
                mix_shader = nodes.new('ShaderNodeMixShader')
                mix_shader.location = (px + 1450, py)

                if alpha_source:
                    links.new(alpha_source, mix_shader.inputs['Fac'])
                else:
                    mix_shader.inputs['Fac'].default_value = \
                        principled.inputs['Alpha'].default_value

                links.new(transparent.outputs['BSDF'], mix_shader.inputs[1])
                links.new(emission.outputs['Emission'], mix_shader.inputs[2])
                links.new(mix_shader.outputs['Shader'],
                          output_node.inputs['Surface'])
            else:
                links.new(emission.outputs['Emission'],
                          output_node.inputs['Surface'])

            processed += 1

    print(f"[OK] Cel-shade applied to {processed} materials "
          f"(shadow={shadow_brightness}, mid={mid_brightness})")


# ============================================================
# Outline
# ============================================================

def add_outline(thickness=0.002):
    """Add solidify modifier for 1px-style black outline."""
    # Shared outline material
    outline_mat = bpy.data.materials.new(name="Outline_Black")
    outline_mat.use_nodes = True
    outline_mat.use_backface_culling = True
    nodes = outline_mat.node_tree.nodes
    lnk = outline_mat.node_tree.links
    nodes.clear()
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = (0.02, 0.02, 0.02, 1.0)
    out = nodes.new('ShaderNodeOutputMaterial')
    lnk.new(em.outputs['Emission'], out.inputs['Surface'])

    count = 0
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        obj.data.materials.append(outline_mat)
        idx = len(obj.data.materials) - 1

        mod = obj.modifiers.new(name="Outline", type='SOLIDIFY')
        mod.thickness = thickness
        mod.offset = -1
        mod.use_flip_normals = True
        mod.material_offset = idx
        count += 1

    print(f"[OK] Outline added to {count} meshes (thickness={thickness})")


# ============================================================
# Camera, Lighting, Render Settings
# ============================================================

def setup_camera(elevation_deg=30.0):
    """Create orthographic camera."""
    cam_data = bpy.data.cameras.new(name="SpriteCamera")
    cam_data.type = 'ORTHO'
    cam_data.ortho_scale = 2.8

    cam_obj = bpy.data.objects.new("SpriteCamera", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    print(f"[OK] Camera created (ortho, {elevation_deg} deg)")
    return cam_obj


def auto_frame_camera(cam_obj, elevation_deg=30.0, padding=1.15):
    """Set ortho_scale to fit the model with padding."""
    min_co, max_co = get_scene_bounds()
    if min_co.x == float('inf'):
        return

    size = max_co - min_co
    ev = math.radians(elevation_deg)

    # Projected visible extents at the given camera angle
    vis_h = size.z * math.cos(ev) + max(size.x, size.y) * math.sin(ev)
    vis_w = max(size.x, size.y)
    scale = max(vis_h, vis_w) * padding
    cam_obj.data.ortho_scale = max(scale, 2.5)

    print(f"[OK] Auto-frame ortho_scale={cam_obj.data.ortho_scale:.2f}")


def setup_lighting():
    """Three-point lighting for clear, consistent sprites."""
    # Key light — bright, slightly warm, from upper-front-right
    key = bpy.data.lights.new("KeyLight", 'SUN')
    key.energy = 3.5
    key.color = (1.0, 0.98, 0.95)
    key_obj = bpy.data.objects.new("KeyLight", key)
    bpy.context.scene.collection.objects.link(key_obj)
    key_obj.rotation_euler = (math.radians(50), math.radians(10),
                              math.radians(-25))

    # Fill light — softer, slightly cool, from opposite side
    fill = bpy.data.lights.new("FillLight", 'SUN')
    fill.energy = 1.5
    fill.color = (0.95, 0.95, 1.0)
    fill_obj = bpy.data.objects.new("FillLight", fill)
    bpy.context.scene.collection.objects.link(fill_obj)
    fill_obj.rotation_euler = (math.radians(55), math.radians(-10),
                               math.radians(155))

    # Rim light — from behind for edge definition
    rim = bpy.data.lights.new("RimLight", 'SUN')
    rim.energy = 1.0
    rim_obj = bpy.data.objects.new("RimLight", rim)
    bpy.context.scene.collection.objects.link(rim_obj)
    rim_obj.rotation_euler = (math.radians(30), 0, math.radians(180))

    print("[OK] 3-point lighting")


def setup_render(render_size):
    """Configure EEVEE for transparent sprite rendering."""
    scene = bpy.context.scene

    for engine in ['BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE']:
        try:
            scene.render.engine = engine
            print(f"[OK] Engine: {engine}")
            break
        except Exception:
            continue

    scene.render.resolution_x = render_size
    scene.render.resolution_y = render_size
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.image_settings.compression = 15
    scene.render.use_stamp = False

    if hasattr(scene.render, 'use_motion_blur'):
        scene.render.use_motion_blur = False

    print(f"[OK] Render: {render_size}x{render_size} RGBA PNG, transparent bg")


def strip_root_motion():
    """Remove XY translation from Hips/Root bone in ALL actions.
    Handles both legacy (Blender <5) and layered (Blender 5+) action APIs."""
    stripped = 0
    for action in bpy.data.actions:
        # Collect all F-curves from the action (API varies by Blender version)
        fcurves = []
        if hasattr(action, 'fcurves'):
            fcurves = list(action.fcurves)
        elif hasattr(action, 'layers'):
            # Blender 5.x layered actions
            for layer in action.layers:
                for strip in layer.strips:
                    for bag in strip.channelbags:
                        fcurves.extend(list(bag.fcurves))

        for fcurve in fcurves:
            path = fcurve.data_path
            if ('location' in path and fcurve.array_index in (0, 1) and
                    any(name in path for name in
                        ['Hips', 'mixamorig:Hips', 'Root'])):
                for kp in fcurve.keyframe_points:
                    kp.co.y = 0.0
                stripped += 1

    if stripped:
        print(f"[OK] Stripped root motion ({stripped} F-curves zeroed)")
    else:
        print("[WARN] No root motion F-curves found — may need wider camera")


# ============================================================
# Animation Helpers
# ============================================================

def classify_animation(name):
    """Map an action/filename to a standard animation type."""
    low = name.lower().strip()

    # Direct match
    if low in ANIM_NAME_MAP:
        return ANIM_NAME_MAP[low]

    # Substring match (order matters — more specific first)
    keywords = [
        # Order matters — more specific keywords FIRST to avoid
        # "Standing 2H Magic Attack" matching "stand" before "attack"
        # Specific compound types FIRST (before generic attack/idle)
        ("attack_2h", "attack_2h"), ("2h attack", "attack_2h"),
        ("great sword", "attack_2h"), ("2hand attack", "attack_2h"),
        ("standby_2h", "standby_2h"), ("2hand idle", "standby_2h"),
        ("standby", "standby"), ("fighting idle", "standby"),
        # Combat/action keywords
        ("attack", "attack"), ("slash", "attack"), ("punch", "attack"),
        ("melee", "attack"), ("swing", "attack"), ("strike", "attack"),
        ("cast", "cast"), ("spell", "cast"), ("magic", "cast"),
        ("block", "block"), ("parry", "block"), ("shield block", "block"),
        ("taunt", "taunt"),
        ("death", "death"), ("die", "death"), ("dying", "death"),
        ("fall", "death"),
        ("hit", "hit"), ("damage", "hit"), ("react", "hit"), ("hurt", "hit"),
        ("pick", "pickup"),
        # Movement
        ("walk", "walk"),
        ("run", "run"), ("jog", "run"),
        # Posture keywords BEFORE idle/stand (sit, fight must win)
        ("sit", "sit"),
        ("fight", "standby"), ("combat idle", "standby"), ("battle idle", "standby"),
        # Generic idle last
        ("idle", "idle"), ("stand", "idle"), ("breath", "idle"),
    ]
    for kw, atype in keywords:
        if kw in low:
            return atype

    # Sanitize for use as filename (strip |, :, etc.)
    import re
    safe = re.sub(r'[|:*?"<>/\\]', '_', low).strip('_')
    return safe or "unknown"


def get_target_frames(anim_type, args):
    """Target sample count for an animation type."""
    overrides = {
        "idle": args.idle_frames,
        "walk": args.walk_frames,
        "attack": args.attack_frames,
        "death": args.death_frames,
    }
    return overrides.get(anim_type, DEFAULT_FRAME_TARGETS.get(anim_type, 6))


def sample_frames(start, end, target_count):
    """Evenly sample target_count frames from [start, end]."""
    total = end - start + 1
    if total <= target_count:
        return list(range(start, end + 1))

    step = (total - 1) / max(target_count - 1, 1)
    return [int(start + i * step) for i in range(target_count)]


# ============================================================
# Rendering
# ============================================================

def position_camera(cam_obj, dir_idx, num_dirs, elevation_deg, target_z=1.0):
    """Place camera at the given direction around the model."""
    dist = 10
    ev = math.radians(elevation_deg)
    angle_h = dir_idx * (2 * math.pi / num_dirs)

    cam_obj.location = (
        dist * math.sin(angle_h) * math.cos(ev),
        -dist * math.cos(angle_h) * math.cos(ev),
        dist * math.sin(ev) + target_z,
    )

    target = mathutils.Vector((0, 0, target_z))
    direction = target - cam_obj.location
    rot = direction.to_track_quat('-Z', 'Y')
    cam_obj.rotation_euler = rot.to_euler()


def render_sprites(cam_obj, output_dir, args):
    """Render every direction x every animation x sampled frames."""
    os.makedirs(output_dir, exist_ok=True)
    dir_names = DIRECTION_NAMES_8 if args.directions == 8 else DIRECTION_NAMES_4

    # Find armature
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break

    # Classify all actions — use original FBX filename as key for folder/file naming,
    # classification only determines frame sampling count
    animations = {}
    for action in bpy.data.actions:
        # Skip Mixamo internal action names (duplicates of the real ones)
        if 'mixamo.com' in action.name.lower():
            continue

        # Use pre-stored filename classification if available,
        # otherwise fall back to classifying the Blender action name
        if action.name in _action_classification:
            base_type = _action_classification[action.name]
        else:
            base_type = classify_animation(action.name)

        tf = get_target_frames(base_type, args)

        # Use original filename as the animation key (for folder & file naming)
        atype = action.name

        start = int(action.frame_range[0])
        end = int(action.frame_range[1])

        animations[atype] = {
            'action': action,
            'start': start,
            'end': end,
            'target_frames': tf,
            'original_name': action.name,
            'classified_type': base_type,
        }

    if animations:
        print(f"\n  Animations ({len(animations)}):")
        for atype, info in animations.items():
            print(f"    {atype}: '{info['original_name']}' "
                  f"frames {info['start']}-{info['end']}, "
                  f"sampling {info['target_frames']}")
    else:
        print("[WARN] No animations found — rendering static idle")
        animations["idle"] = {
            'action': None, 'start': 1, 'end': 4,
            'target_frames': 4, 'original_name': 'static',
        }

    total = 0
    num_dirs = args.directions

    for dir_idx in range(num_dirs):
        dir_name = dir_names[dir_idx]
        position_camera(cam_obj, dir_idx, num_dirs, args.camera_angle,
                        args.camera_target_z)

        for atype, info in animations.items():
            # Assign animation
            if armature and info['action']:
                if not armature.animation_data:
                    armature.animation_data_create()
                armature.animation_data.action = info['action']

            frames = sample_frames(info['start'], info['end'],
                                   info['target_frames'])

            for fi, frame in enumerate(frames):
                bpy.context.scene.frame_set(frame)

                # Cancel Mixamo root motion: reset armature XY to origin
                # and lock root bone XY (keeps vertical motion for bounce)
                if armature:
                    armature.location.x = 0
                    armature.location.y = 0
                    if armature.pose:
                        for bone_name in ['Hips', 'mixamorig:Hips', 'Root']:
                            bone = armature.pose.bones.get(bone_name)
                            if bone:
                                bone.location.x = 0
                                bone.location.y = 0
                                break
                    bpy.context.view_layer.update()

                filename = f"{atype}_{dir_name}_f{fi:02d}.png"
                if args.subfolders:
                    atype_dir = os.path.join(output_dir, atype)
                    os.makedirs(atype_dir, exist_ok=True)
                    filepath = os.path.join(atype_dir, filename)
                else:
                    filepath = os.path.join(output_dir, filename)

                bpy.context.scene.render.filepath = filepath
                bpy.ops.render.render(write_still=True)
                total += 1

        print(f"    {dir_name}: done")

    print(f"\n[OK] Rendered {total} sprites -> {output_dir}")
    return total


# ============================================================
# Main Pipeline
# ============================================================

def attach_equipment(equip_path, armature, bone_name, offset, rotation, scale, body_meshes, pose_fbx=None):
    """Import equipment GLB/FBX and parent it to a bone on the armature.

    Args:
        pose_fbx: If set, temporarily switch armature to this FBX's animation
                  before computing matrix_parent_inverse. This must match the
                  FBX used when positioning the equipment in Blender.

    Returns list of equipment mesh objects, or empty list on failure.
    """
    import math

    before_objects = set(bpy.context.scene.objects)

    ext = os.path.splitext(equip_path)[1].lower()
    if ext == '.glb' or ext == '.gltf':
        bpy.ops.import_scene.gltf(filepath=equip_path)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=equip_path)
    else:
        print(f"[ERROR] Unsupported equipment format: {ext}")
        return []

    new_objects = set(bpy.context.scene.objects) - before_objects
    equip_meshes = [obj for obj in new_objects if obj.type == 'MESH']

    if not equip_meshes:
        print(f"[ERROR] No meshes found in equipment file: {equip_path}")
        return []

    if not armature:
        print("[ERROR] No armature found — cannot attach equipment")
        return []

    # Find the target bone
    bone = armature.pose.bones.get(bone_name)
    if not bone:
        # Try common alternatives
        alternatives = [
            bone_name,
            bone_name.replace('mixamorig:', ''),
            'RightHand', 'mixamorig:RightHand',
            'LeftHand', 'mixamorig:LeftHand',
            'Head', 'mixamorig:Head',
        ]
        for alt in alternatives:
            bone = armature.pose.bones.get(alt)
            if bone:
                print(f"[OK] Using bone '{alt}' ('{bone_name}' not found)")
                bone_name = alt
                break

    if not bone:
        print(f"[ERROR] Bone '{bone_name}' not found on armature")
        available = [b.name for b in armature.pose.bones]
        print(f"  Available bones: {available[:20]}...")
        return []

    # Parent each equipment mesh to the bone
    # Compute matrix_parent_inverse to match Blender's Ctrl+P -> Bone behavior
    # Must use the SAME animation pose that was active when the user positioned
    # the equipment in Blender, otherwise the bone world transform is different
    # and the offset values produce wrong results.
    saved_action = None
    saved_frame = bpy.context.scene.frame_current
    if pose_fbx and os.path.exists(pose_fbx):
        # Import the pose FBX to get its animation action
        pose_action = import_animation_fbx(pose_fbx, armature)
        if pose_action and armature.animation_data:
            saved_action = armature.animation_data.action
            armature.animation_data.action = pose_action
            bpy.context.scene.frame_set(1)  # Frame 1 = default pose
            bpy.context.view_layer.update()
            print(f"[OK] Equipment pose set to: {os.path.basename(pose_fbx)}")

    pose_bone = armature.pose.bones[bone_name]
    bone_matrix_world = armature.matrix_world @ pose_bone.matrix
    parent_inverse = bone_matrix_world.inverted()

    # Restore original animation if we swapped
    if saved_action is not None and armature.animation_data:
        armature.animation_data.action = saved_action
        bpy.context.scene.frame_set(saved_frame)
        bpy.context.view_layer.update()

    for mesh in equip_meshes:
        mesh.parent = armature
        mesh.parent_type = 'BONE'
        mesh.parent_bone = bone_name
        mesh.matrix_parent_inverse = parent_inverse

        # Apply offset, rotation, scale — these now match the Blender UI values exactly
        mesh.location = mathutils.Vector(offset)
        mesh.rotation_euler = mathutils.Euler(
            (math.radians(rotation[0]), math.radians(rotation[1]), math.radians(rotation[2])))
        if isinstance(scale, list) and len(scale) == 3:
            mesh.scale = mathutils.Vector((scale[0], scale[1], scale[2]))
        elif isinstance(scale, list) and len(scale) == 1:
            mesh.scale = mathutils.Vector((scale[0], scale[0], scale[0]))
        else:
            s = float(scale) if not isinstance(scale, list) else scale[0]
            mesh.scale = mathutils.Vector((s, s, s))

    # Delete any non-mesh objects from the equipment import (armatures, empties)
    for obj in new_objects:
        if obj.type != 'MESH':
            bpy.data.objects.remove(obj, do_unlink=True)

    print(f"[OK] Attached {len(equip_meshes)} equipment meshes to bone '{bone_name}'")
    print(f"     Offset: {offset}, Rotation: {rotation}, Scale: {scale}")

    return equip_meshes


def process_model(model_path, output_dir, args):
    """Full render pipeline for one model file."""
    name = os.path.splitext(os.path.basename(model_path))[0]
    model_output = os.path.join(output_dir, name)

    print(f"\n{'=' * 60}")
    print(f"  Model: {name}")
    print(f"  Output: {model_output}")
    print(f"{'=' * 60}")

    clear_scene()

    # 1. Import base model
    armature, meshes = import_model(model_path)
    if not meshes:
        print(f"[ERROR] No meshes in {model_path}")
        return 0

    # 1b. Recover textures from original GLB if Mixamo stripped them
    if args.texture_from and os.path.exists(args.texture_from):
        recover_textures(args.texture_from, meshes)

    # 2. Import additional animation FBX files
    anim_files = []
    if args.anims:
        anim_files.extend(args.anims)
    if args.anim_dir and os.path.isdir(args.anim_dir):
        for f in sorted(os.listdir(args.anim_dir)):
            if f.lower().endswith('.fbx'):
                anim_files.append(os.path.join(args.anim_dir, f))

    for af in anim_files:
        if os.path.exists(af):
            import_animation_fbx(af, armature)
        else:
            print(f"[WARN] Animation file not found: {af}")

    # 2b. Strip root motion from all animations (keeps character in place)
    strip_root_motion()

    # 2c. Equipment mode: attach equipment to bone, hide body meshes
    is_equipment_mode = False
    if args.equipment and os.path.exists(args.equipment):
        equip_meshes = attach_equipment(
            args.equipment, armature, args.attach_bone,
            args.equip_offset, args.equip_rotation, args.equip_scale,
            meshes, pose_fbx=args.equip_pose_fbx)
        if equip_meshes:
            is_equipment_mode = True

            # Hide equipment during centering so bounding box matches body-only render
            for em in equip_meshes:
                em.hide_viewport = True
            center_and_normalize()
            for em in equip_meshes:
                em.hide_viewport = False

            # Now hide body meshes — camera framing uses body bounds
            for m in meshes:
                m.hide_render = True
                m.hide_viewport = True
            meshes = equip_meshes  # Use equipment for cel-shade + outline
            print(f"[OK] Equipment mode: body hidden, rendering {len(equip_meshes)} equipment meshes")

    # 3. Center and normalize (skip if equipment mode already did it)
    if not is_equipment_mode:
        center_and_normalize()

    # 4. Cel-shading
    if not args.no_cel_shade:
        setup_cel_shade(args.cel_shadow, args.cel_mid)

    # 5. Outline
    if not args.no_outline:
        add_outline(args.outline_width)

    # 6. Camera + auto-frame
    cam = setup_camera(args.camera_angle)
    if is_equipment_mode:
        # Temporarily show body meshes + hide equipment for correct camera framing
        # (must match body-only render's bounding box exactly)
        body_meshes_hidden = [obj for obj in bpy.context.scene.objects
                              if obj.type == 'MESH' and obj.hide_viewport and obj not in meshes]
        for m in body_meshes_hidden:
            m.hide_viewport = False
        for em in meshes:  # meshes is now equip_meshes
            em.hide_viewport = True
        auto_frame_camera(cam, args.camera_angle)
        for m in body_meshes_hidden:
            m.hide_viewport = True
        for em in meshes:
            em.hide_viewport = False
    else:
        auto_frame_camera(cam, args.camera_angle)

    # 7. Lighting + render settings
    setup_lighting()
    setup_render(args.render_size)

    # 8. Render
    return render_sprites(cam, model_output, args)


def main():
    args = parse_args()

    print(f"\n{'=' * 60}")
    print(f"  Sabri_MMO Sprite Renderer v2")
    print(f"  Input:      {args.input}")
    print(f"  Output:     {args.output}")
    print(f"  Directions: {args.directions}")
    print(f"  Render:     {args.render_size}x{args.render_size}")
    print(f"  Cel-shade:  {not args.no_cel_shade}")
    print(f"  Outline:    {not args.no_outline}")
    print(f"{'=' * 60}")

    grand_total = 0

    if args.batch:
        if not os.path.isdir(args.input):
            print(f"[ERROR] --batch requires a directory: {args.input}")
            return

        exts = {'.glb', '.gltf', '.fbx', '.obj'}
        models = sorted([
            f for f in os.listdir(args.input)
            if os.path.splitext(f)[1].lower() in exts
        ])

        print(f"\n[BATCH] {len(models)} models in {args.input}")
        for mf in models:
            path = os.path.join(args.input, mf)
            grand_total += process_model(path, args.output, args)
    else:
        if not os.path.exists(args.input):
            print(f"[ERROR] File not found: {args.input}")
            return
        grand_total = process_model(args.input, args.output, args)

    print(f"\n{'=' * 60}")
    print(f"  COMPLETE: {grand_total} total sprites")
    print(f"  Output:   {args.output}")
    print(f"  Next:     python post_process_sprites.py {args.output} "
          f"<final_dir> --size 256")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
