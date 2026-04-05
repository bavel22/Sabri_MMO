"""
Dual-Pass Sprite Renderer — Body + Weapon Overlay
===================================================
Renders body and weapon sprites from a SINGLE .blend scene, guaranteeing
pixel-perfect alignment between layers.

The .blend file must have:
  - Armature with Mixamo skeleton
  - Body mesh(es): parented to Armature (no parent_bone)
  - Equipment mesh(es): parented to Armature via a bone (has parent_bone)

Pipeline:
  1. Opens the .blend (weapon already positioned on hand bone by user)
  2. Imports Mixamo FBX animations (Blender 5.x action_slot fix)
  3. Resets armature to rest pose for consistent bounds measurement
  4. Centers + normalizes using BODY mesh bounds only (equipment hidden)
  5. Applies cel-shading to all meshes, outline to body only
  6. Auto-frames camera from body bounds (frozen — never recomputed)
  7. For each (direction, animation, frame), renders TWO passes:
     Pass 1: Body visible, equipment hidden -> body output
     Pass 2: Body hidden, equipment visible -> weapon output

Both passes use IDENTICAL camera/centering -> guaranteed pixel alignment.

Usage:
  "C:/Blender 5.1/blender.exe" dagger_on_swordsman.blend --background \
    --python "C:/Sabri_MMO/2D animations/render_blend_all_visible.py" -- \
    --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/swordsman_m/" \
    --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/swordsman_m" \
    --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_dagger" \
    --render-size 1024 --camera-angle 10 --camera-target-z 0.7

Body-only (re-render body without weapons):
  ... --body-only --weapon-output ignored

Weapon-only (re-render weapon without body):
  ... --weapon-only --body-output ignored

Test single frame (verify alignment before full render):
  ... --test-frame
"""

import bpy
import os
import sys
import argparse
import mathutils
import math
import json


# ============================================================
# Constants
# ============================================================

DIRECTIONS = ["S", "SW", "W", "NW", "N", "NE", "E", "SE"]

ANIM_NAME_MAP = {
    "idle": "idle", "breathing idle": "idle", "happy idle": "idle",
    "standing idle": "idle", "warrior idle": "idle",
    "walking": "walk", "walk": "walk", "slow walk": "walk",
    "walking in place": "walk", "run with sword": "run",
    "running": "run", "run": "run", "jogging": "run",
    "run with sword (1)": "run",
    "attack": "attack", "slash": "attack",
    "sword and shield slash": "attack", "great sword slash": "attack_2h",
    "punching": "attack", "standing melee attack horizontal": "attack",
    "sword and shield attack": "attack", "mutant punch": "attack",
    "stable sword inward slash": "attack", "stable sword outward slash": "attack",
    "stable sword inward slash (1)": "attack", "stable sword outward slash (1)": "attack",
    "casting spell": "cast", "standing 1h magic attack 01": "cast",
    "standing 1h magic attack 02": "cast", "standing 1h magic attack 03": "cast",
    "standing 1h cast spell 01": "cast",
    "standing 2h magic attack 01": "cast", "standing 2h magic attack 02": "cast",
    "standing 2h magic attack 03": "cast", "standing 2h magic attack 04": "cast",
    "standing 2h magic attack 05": "cast",
    "standing 2h magic area attack 01": "cast", "standing 2h magic area attack 02": "cast",
    "standing 2h cast spell 01": "cast",
    "great sword casting": "cast", "two hand spell casting": "cast",
    "wide arm spell casting": "cast", "spell cast": "cast", "spell casting": "cast",
    "getting hit": "hit", "hit": "hit", "damage": "hit",
    "standing react small from front": "hit", "reaction": "hit",
    "death": "death", "dying": "death", "dying backwards": "death",
    "falling back death": "death", "death backwards": "death",
    "sitting idle": "sit", "sitting": "sit",
    "fighting idle": "standby", "2hand idle": "standby_2h",
    "picking up": "pickup",
    "sword and shield block": "block",
    "standing taunt chest thump": "taunt",
    "unarmed idle 01": "idle",
}

DEFAULT_FRAME_TARGETS = {
    "idle": 8, "walk": 12, "run": 12, "attack": 10, "attack_2h": 10,
    "cast": 8, "hit": 6, "death": 8, "sit": 4, "standby": 8,
    "standby_2h": 8, "pickup": 8, "block": 6, "taunt": 8,
}


# ============================================================
# Argument Parsing
# ============================================================

def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser(description="Dual-pass body + weapon sprite renderer")
    parser.add_argument("--anim-dir", required=True,
                        help="Directory of Mixamo FBX animation files")
    parser.add_argument("--body-output", required=True,
                        help="Output directory for body sprites")
    parser.add_argument("--weapon-output", required=True,
                        help="Output directory for weapon overlay sprites")
    parser.add_argument("--render-size", type=int, default=1024)
    parser.add_argument("--camera-angle", type=float, default=10.0)
    parser.add_argument("--camera-target-z", type=float, default=0.7)
    parser.add_argument("--cel-shadow", type=float, default=0.45,
                        help="Shadow brightness 0-1 (default 0.45)")
    parser.add_argument("--cel-mid", type=float, default=0.78,
                        help="Midtone brightness 0-1 (default 0.78)")
    parser.add_argument("--no-cel-shade", action="store_true")
    parser.add_argument("--no-outline", action="store_true")
    parser.add_argument("--outline-width", type=float, default=0.002)
    parser.add_argument("--directions", type=int, default=8, choices=[4, 8])
    parser.add_argument("--texture-from", default=None,
                        help="GLB to recover body textures from (if body mesh has no textures)")
    parser.add_argument("--body-only", action="store_true",
                        help="Render body pass only (skip weapon)")
    parser.add_argument("--weapon-only", action="store_true",
                        help="Render weapon pass only (skip body)")
    parser.add_argument("--test-frame", action="store_true",
                        help="Render 1 frame only (S dir, first anim, frame 0) for alignment test")
    return parser.parse_args(argv)


# ============================================================
# Classification + Sampling
# ============================================================

def classify_animation(name):
    """Map animation name to standard type for frame sampling."""
    low = name.lower().strip()
    if low in ANIM_NAME_MAP:
        return ANIM_NAME_MAP[low]
    keywords = [
        ("attack_2h", "attack_2h"), ("great sword", "attack_2h"),
        ("standby_2h", "standby_2h"), ("2hand idle", "standby_2h"),
        ("standby", "standby"), ("fighting idle", "standby"),
        ("attack", "attack"), ("slash", "attack"), ("punch", "attack"),
        ("melee", "attack"), ("swing", "attack"), ("strike", "attack"),
        ("cast", "cast"), ("spell", "cast"), ("magic", "cast"),
        ("block", "block"), ("parry", "block"),
        ("taunt", "taunt"),
        ("death", "death"), ("die", "death"), ("dying", "death"),
        ("hit", "hit"), ("damage", "hit"), ("react", "hit"),
        ("pick", "pickup"),
        ("walk", "walk"), ("run", "run"), ("jog", "run"),
        ("sit", "sit"), ("fight", "standby"),
        ("idle", "idle"), ("stand", "idle"), ("breath", "idle"),
    ]
    for kw, atype in keywords:
        if kw in low:
            return atype
    return "unknown"


def sample_frames(start, end, count):
    """Sample evenly-spaced frames from a range."""
    total = end - start + 1
    if total <= count:
        return list(range(start, end + 1))
    step = (total - 1) / (count - 1)
    return [start + int(round(i * step)) for i in range(count)]


# ============================================================
# Scene Analysis
# ============================================================

def find_scene_objects():
    """Classify scene objects: armature, body meshes, equipment meshes.

    Equipment detection: any mesh with parent_bone set (bone-parented).
    Body: any mesh without parent_bone (armature-parented or root-level).
    """
    armature = None
    body_meshes = []
    equip_meshes = []

    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
        elif obj.type == 'MESH':
            if obj.parent_bone:
                equip_meshes.append(obj)
            else:
                body_meshes.append(obj)

    return armature, body_meshes, equip_meshes


# ============================================================
# Animation Import
# ============================================================

_action_classification = {}


def import_animations(anim_dir):
    """Import all FBX animations from directory. Returns dict keyed by filename."""
    anim_actions = {}

    if not os.path.isdir(anim_dir):
        print(f"[ERROR] Animation directory not found: {anim_dir}")
        return anim_actions

    for f in sorted(os.listdir(anim_dir)):
        if not f.lower().endswith('.fbx'):
            continue

        filepath = os.path.join(anim_dir, f)
        before_actions = set(bpy.data.actions)
        before_objects = set(bpy.context.scene.objects)

        bpy.ops.import_scene.fbx(filepath=filepath)

        new_actions = set(bpy.data.actions) - before_actions
        new_objects = set(bpy.context.scene.objects) - before_objects

        if new_actions:
            action = list(new_actions)[0]
            action.use_fake_user = True
            basename = os.path.splitext(f)[0]
            action.name = basename

            atype = classify_animation(basename.lower())
            _action_classification[action.name] = atype

            start = int(action.frame_range[0])
            end = int(action.frame_range[1])
            tf = DEFAULT_FRAME_TARGETS.get(atype, 6)

            anim_actions[basename] = {
                'action': action, 'atype': atype,
                'start': start, 'end': end, 'target_frames': tf,
            }
            print(f"  {f} -> '{atype}' ({start}-{end}, {tf}f)")

        # Delete imported objects (keep only the action data)
        for obj in new_objects:
            bpy.data.objects.remove(obj, do_unlink=True)

    return anim_actions


def strip_root_motion():
    """Zero out XY root bone translation in all actions.
    Handles both legacy (Blender <5) and layered (Blender 5+) action APIs."""
    for action in bpy.data.actions:
        if 'mixamo.com' in action.name.lower():
            continue
        try:
            if hasattr(action, 'layers'):
                for layer in action.layers:
                    for strip in layer.strips:
                        for cb in strip.channelbags:
                            for fc in cb.fcurves:
                                if ('Hips' in fc.data_path or 'Root' in fc.data_path) and \
                                   'location' in fc.data_path and fc.array_index in (0, 1):
                                    for kp in fc.keyframe_points:
                                        kp.co[1] = 0
                                    fc.update()
            elif hasattr(action, 'fcurves'):
                for fc in action.fcurves:
                    if ('Hips' in fc.data_path or 'Root' in fc.data_path) and \
                       'location' in fc.data_path and fc.array_index in (0, 1):
                        for kp in fc.keyframe_points:
                            kp.co[1] = 0
                        fc.update()
        except Exception:
            pass
    print("[OK] Root motion stripped")


# ============================================================
# Centering + Normalization
# ============================================================

def get_mesh_bounds(mesh_list):
    """World-space bounding box of specified mesh objects."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    min_co = mathutils.Vector((float('inf'),) * 3)
    max_co = mathutils.Vector((float('-inf'),) * 3)

    for obj in mesh_list:
        if obj.hide_viewport:
            continue
        eval_obj = obj.evaluated_get(depsgraph)
        try:
            mesh = eval_obj.to_mesh()
        except RuntimeError:
            continue
        for vert in mesh.vertices:
            wc = obj.matrix_world @ vert.co
            min_co.x = min(min_co.x, wc.x)
            min_co.y = min(min_co.y, wc.y)
            min_co.z = min(min_co.z, wc.z)
            max_co.x = max(max_co.x, wc.x)
            max_co.y = max(max_co.y, wc.y)
            max_co.z = max(max_co.z, wc.z)
        eval_obj.to_mesh_clear()

    return min_co, max_co


def center_and_normalize_from_body(body_meshes, equip_meshes):
    """Center scene at origin using BODY mesh bounds only.

    Equipment is hidden during measurement. Centering moves ALL root objects
    (armature, etc.) so equipment (bone-parented) follows automatically.
    Normalizes height to 2.0 units.
    """
    # Hide equipment during bounds measurement
    for em in equip_meshes:
        em.hide_viewport = True

    min_co, max_co = get_mesh_bounds(body_meshes)

    # Restore equipment visibility
    for em in equip_meshes:
        em.hide_viewport = False

    if min_co.x == float('inf'):
        print("[WARN] No body meshes found for centering")
        return

    center = (min_co + max_co) / 2
    height = max_co.z - min_co.z

    # Offset: center XY at origin, feet at Z=0
    offset = mathutils.Vector((-center.x, -center.y, -min_co.z))
    roots = [o for o in bpy.context.scene.objects if o.parent is None]
    for obj in roots:
        obj.location += offset
    bpy.context.view_layer.update()

    # Normalize height to 2.0
    if height > 0.001:
        sf = 2.0 / height
        for obj in roots:
            obj.scale *= sf
        bpy.context.view_layer.update()

    print(f"[OK] Centered on body bounds (height {height:.3f} -> 2.0)")


# ============================================================
# Cel-Shading (exact match to blender_sprite_render_v2.py)
# ============================================================

def apply_cel_shade(mesh_list, shadow=0.45, mid=0.78):
    """Color-preserving cel-shading: quantized lighting x original base color.

    Node chain: Principled BSDF -> Shader to RGB -> RGB to BW ->
    ColorRamp (4 constant steps) -> Mix(MULTIPLY) with base color -> Emission.
    """
    processed = 0
    for obj in mesh_list:
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

            # Capture base color source before rewiring
            base_color_source = None
            for link in links:
                if link.to_socket == principled.inputs['Base Color']:
                    base_color_source = link.from_socket
                    break
            base_color_value = principled.inputs['Base Color'].default_value[:]

            px = principled.location.x
            py = principled.location.y

            # 1. Principled BSDF -> Shader to RGB (captures full lighting)
            s2rgb = nodes.new('ShaderNodeShaderToRGB')
            s2rgb.location = (px + 300, py)
            links.new(principled.outputs['BSDF'], s2rgb.inputs['Shader'])

            # 2. RGB to BW (extract luminance)
            rgb2bw = nodes.new('ShaderNodeRGBToBW')
            rgb2bw.location = (px + 500, py - 150)
            links.new(s2rgb.outputs['Color'], rgb2bw.inputs['Color'])

            # 3. ColorRamp (4-step quantization, CONSTANT interpolation)
            ramp = nodes.new('ShaderNodeValToRGB')
            ramp.location = (px + 700, py - 150)
            ramp.color_ramp.interpolation = 'CONSTANT'
            cr = ramp.color_ramp
            ds = shadow * 0.65  # deep shadow
            cr.elements[0].position = 0.0
            cr.elements[0].color = (ds, ds, ds, 1.0)
            cr.elements[1].position = 0.20
            cr.elements[1].color = (shadow, shadow, shadow, 1.0)
            e_mid = cr.elements.new(0.50)
            e_mid.color = (mid, mid, mid, 1.0)
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

            # Remove old connection to Material Output
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

    print(f"[OK] Cel-shade: {processed} materials (shadow={shadow}, mid={mid})")


# ============================================================
# Outline (body meshes only)
# ============================================================

def add_outline(mesh_list, thickness=0.002):
    """Add solidify modifier for 1px-style black outline on specified meshes."""
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
    for obj in mesh_list:
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

    print(f"[OK] Outline: {count} meshes (thickness={thickness})")


# ============================================================
# Camera, Lighting, Render Settings
# ============================================================

def setup_camera():
    """Create orthographic camera."""
    cam_data = bpy.data.cameras.new("SpriteCamera")
    cam_data.type = 'ORTHO'
    cam_obj = bpy.data.objects.new("SpriteCamera", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj
    return cam_obj


def auto_frame_from_body(cam_obj, body_meshes, equip_meshes, elevation_deg):
    """Set ortho_scale from BODY bounds only. Matches v2 formula.

    Equipment is hidden during measurement so camera framing is
    identical regardless of weapon size/position.
    """
    for em in equip_meshes:
        em.hide_viewport = True

    min_co, max_co = get_mesh_bounds(body_meshes)

    for em in equip_meshes:
        em.hide_viewport = False

    if min_co.x == float('inf'):
        cam_obj.data.ortho_scale = 2.5
        print("[WARN] No body bounds — using default ortho=2.5")
        return

    size = max_co - min_co
    ev = math.radians(elevation_deg)

    # Projected visible extents (matches blender_sprite_render_v2.py)
    vis_h = size.z * math.cos(ev) + max(size.x, size.y) * math.sin(ev)
    vis_w = max(size.x, size.y)
    scale = max(vis_h, vis_w) * 1.15  # padding
    cam_obj.data.ortho_scale = max(scale, 2.5)

    print(f"[OK] Auto-frame ortho_scale={cam_obj.data.ortho_scale:.2f} (body bounds only)")


def position_camera(cam_obj, dir_idx, num_dirs, elevation_deg, target_z):
    """Place camera at the given direction around the model."""
    angle = (dir_idx / num_dirs) * 2 * math.pi
    elev = math.radians(elevation_deg)
    d = 10.0
    x = math.sin(angle) * math.cos(elev) * d
    y = -math.cos(angle) * math.cos(elev) * d
    z = math.sin(elev) * d + target_z
    cam_obj.location = (x, y, z)
    direction = mathutils.Vector((0, 0, target_z)) - mathutils.Vector((x, y, z))
    cam_obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()


def setup_lighting():
    """Three-point lighting matching blender_sprite_render_v2.py exactly."""
    key = bpy.data.lights.new("KeyLight", 'SUN')
    key.energy = 3.5
    key.color = (1.0, 0.98, 0.95)
    key_obj = bpy.data.objects.new("KeyLight", key)
    bpy.context.scene.collection.objects.link(key_obj)
    key_obj.rotation_euler = (math.radians(50), math.radians(10), math.radians(-25))

    fill = bpy.data.lights.new("FillLight", 'SUN')
    fill.energy = 1.5
    fill.color = (0.95, 0.95, 1.0)
    fill_obj = bpy.data.objects.new("FillLight", fill)
    bpy.context.scene.collection.objects.link(fill_obj)
    fill_obj.rotation_euler = (math.radians(55), math.radians(-10), math.radians(155))

    rim = bpy.data.lights.new("RimLight", 'SUN')
    rim.energy = 1.0
    rim_obj = bpy.data.objects.new("RimLight", rim)
    bpy.context.scene.collection.objects.link(rim_obj)
    rim_obj.rotation_euler = (math.radians(30), 0, math.radians(180))

    print("[OK] 3-point lighting (matches v2)")


def setup_render(render_size):
    """Configure EEVEE for transparent sprite rendering."""
    scene = bpy.context.scene
    for engine in ['BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE']:
        try:
            scene.render.engine = engine
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
    print(f"[OK] Render: {render_size}x{render_size} RGBA PNG")


# ============================================================
# Texture Recovery (optional, for .blend files with untextured body)
# ============================================================

def recover_textures(texture_source_path, target_meshes):
    """Import GLB with textures, copy materials to target meshes.

    Only needed if the .blend body mesh was imported from Mixamo FBX
    (which strips textures). The GLB must be the exact model uploaded to Mixamo.
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

    source_materials = []
    for src in source_meshes:
        for slot in src.material_slots:
            if slot.material:
                source_materials.append(slot.material)

    if source_materials:
        for tgt in target_meshes:
            tgt.data.materials.clear()
            for mat in source_materials:
                tgt.data.materials.append(mat)
        print(f"[OK] Recovered {len(source_materials)} material(s) from "
              f"{os.path.basename(texture_source_path)}")

    for obj in new_objects:
        bpy.data.objects.remove(obj, do_unlink=True)


# ============================================================
# Dual-Pass Render
# ============================================================

def compute_equip_depth_front(armature, cam_obj, equip_meshes):
    """Determine if equipment is in front of body center from the camera's view.

    Projects the equipment bone position and body center into camera space.
    In camera space, higher Z = closer to camera = in front.
    Returns True if equipment is in front of body.
    """
    # Find the equipment bone from any equipment mesh
    equip_bone_name = None
    for em in equip_meshes:
        if em.parent_bone:
            equip_bone_name = em.parent_bone
            break

    if not equip_bone_name or not armature.pose:
        return True  # Default to front if no bone info

    pose_bone = armature.pose.bones.get(equip_bone_name)
    if not pose_bone:
        return True

    # Equipment bone world position (bone head)
    bone_world = armature.matrix_world @ pose_bone.matrix @ mathutils.Vector((0, 0, 0))

    # Body center in world space (after centering: origin, height=2.0, center at Z=1.0)
    body_center = mathutils.Vector((0, 0, 1.0))

    # Transform both to camera space
    cam_inv = cam_obj.matrix_world.inverted()
    equip_cam = cam_inv @ bone_world
    body_cam = cam_inv @ body_center

    # Camera looks along its local -Z. Higher Z = closer to camera = in front.
    return equip_cam.z > body_cam.z


def render_dual_pass(cam_obj, armature, body_meshes, equip_meshes,
                     anim_actions, args):
    """Render each frame twice: body-only pass + weapon-only pass.

    Both passes use the IDENTICAL camera position, ortho_scale, and scene
    centering. This guarantees pixel-perfect alignment between the two
    sprite layers when composited at runtime.

    Also computes per-frame depth ordering (weapon in front vs behind body)
    and writes depth_map.json to the weapon output directory.
    """
    dir_names = DIRECTIONS[:args.directions]
    render_body = not args.weapon_only
    render_weapon = not args.body_only and len(equip_meshes) > 0

    if render_body:
        os.makedirs(args.body_output, exist_ok=True)
    if render_weapon:
        os.makedirs(args.weapon_output, exist_ok=True)

    body_total = 0
    weapon_total = 0

    # Depth map: {anim_key: {dir_name: [bool per frame]}} — legacy, kept for backward compat
    depth_map = {}

    # Create holdout material for body occlusion during equipment pass.
    # Holdout makes the body transparent but it still blocks equipment pixels
    # behind it — baking physically correct occlusion into the equipment sprite.
    # This eliminates the need for runtime depth ordering (+5/-5 offset).
    holdout_mat = bpy.data.materials.new(name="__holdout__")
    holdout_mat.use_nodes = True
    holdout_mat.node_tree.nodes.clear()
    output_node = holdout_mat.node_tree.nodes.new('ShaderNodeOutputMaterial')
    holdout_node = holdout_mat.node_tree.nodes.new('ShaderNodeHoldout')
    holdout_mat.node_tree.links.new(holdout_node.outputs[0], output_node.inputs[0])

    # Save original body materials (for restore after equipment pass)
    saved_body_materials = {}
    for bm in body_meshes:
        saved_body_materials[bm.name] = [(slot.material, slot.link) for slot in bm.material_slots]

    # Test mode: only render 1 frame from direction S
    test_dirs = 1 if args.test_frame else args.directions
    test_max_anims = 1 if args.test_frame else len(anim_actions)
    test_max_frames = 1 if args.test_frame else 9999

    anim_count = 0
    for dir_idx in range(test_dirs):
        position_camera(cam_obj, dir_idx, args.directions,
                        args.camera_angle, args.camera_target_z)

        anim_count = 0
        for anim_key, info in anim_actions.items():
            if anim_count >= test_max_anims:
                break
            anim_count += 1

            # Initialize depth map for this animation
            if anim_key not in depth_map:
                depth_map[anim_key] = {}

            # Set animation action + Blender 5.x slot
            if armature and info['action']:
                if not armature.animation_data:
                    armature.animation_data_create()
                ad = armature.animation_data
                ad.action = info['action']
                if hasattr(info['action'], 'slots') and len(info['action'].slots) > 0:
                    ad.action_slot = info['action'].slots[0]
                bpy.context.scene.frame_set(info['start'])
                bpy.context.view_layer.update()

            frames = sample_frames(info['start'], info['end'], info['target_frames'])
            dir_depth = []

            frame_count = 0
            for fi, frame in enumerate(frames):
                if frame_count >= test_max_frames:
                    break
                frame_count += 1

                bpy.context.scene.frame_set(frame)

                # Cancel root motion per-frame
                if armature:
                    armature.location.x = 0
                    armature.location.y = 0
                    if armature.pose:
                        for bn in ['Hips', 'mixamorig:Hips', 'Root']:
                            b = armature.pose.bones.get(bn)
                            if b:
                                b.location.x = 0
                                b.location.y = 0
                                break
                    bpy.context.view_layer.update()

                # Compute depth: is weapon in front of or behind body?
                is_front = compute_equip_depth_front(armature, cam_obj, equip_meshes)
                dir_depth.append(is_front)

                filename = f"{anim_key}_{dir_names[dir_idx]}_f{fi:02d}.png"

                # ---- PASS 1: Body only (equipment hidden) ----
                if render_body:
                    for bm in body_meshes:
                        bm.hide_render = False
                    for em in equip_meshes:
                        em.hide_render = True

                    out_dir = os.path.join(args.body_output, anim_key)
                    os.makedirs(out_dir, exist_ok=True)
                    bpy.context.scene.render.filepath = os.path.join(out_dir, filename)
                    bpy.ops.render.render(write_still=True)
                    body_total += 1

                # ---- PASS 2: Equipment with body as holdout occluder ----
                # Body uses holdout material (transparent but blocks equipment behind it).
                # Equipment renders normally. Result: only visible equipment pixels output.
                if render_weapon:
                    # Swap body materials to holdout
                    for bm in body_meshes:
                        bm.hide_render = False
                        for slot in bm.material_slots:
                            slot.material = holdout_mat
                    for em in equip_meshes:
                        em.hide_render = False

                    out_dir = os.path.join(args.weapon_output, anim_key)
                    os.makedirs(out_dir, exist_ok=True)
                    bpy.context.scene.render.filepath = os.path.join(out_dir, filename)
                    bpy.ops.render.render(write_still=True)
                    weapon_total += 1

                    # Restore body materials for next pass 1
                    for bm in body_meshes:
                        for i, (mat, link) in enumerate(saved_body_materials.get(bm.name, [])):
                            if i < len(bm.material_slots):
                                bm.material_slots[i].material = mat

            depth_map[anim_key][dir_names[dir_idx]] = dir_depth

        print(f"    {dir_names[dir_idx]}: done")

    # Restore visibility and materials
    for bm in body_meshes:
        bm.hide_render = False
        for i, (mat, link) in enumerate(saved_body_materials.get(bm.name, [])):
            if i < len(bm.material_slots):
                bm.material_slots[i].material = mat
    for em in equip_meshes:
        em.hide_render = False

    # Clean up holdout material
    bpy.data.materials.remove(holdout_mat)

    # Write depth map JSON
    if render_weapon and depth_map:
        depth_path = os.path.join(args.weapon_output, 'depth_map.json')
        with open(depth_path, 'w') as f:
            json.dump(depth_map, f, indent=2)
        # Count front/behind stats
        total_front = sum(1 for a in depth_map.values()
                          for d in a.values() for v in d if v)
        total_behind = sum(1 for a in depth_map.values()
                           for d in a.values() for v in d if not v)
        print(f"[OK] Depth map: {total_front} front, {total_behind} behind -> {depth_path}")

    return body_total, weapon_total


# ============================================================
# Main
# ============================================================

def main():
    args = parse_args()

    print(f"\n{'='*60}")
    print(f"  Dual-Pass Sprite Renderer (Body + Weapon)")
    print(f"  Body output:   {args.body_output}")
    print(f"  Weapon output: {args.weapon_output}")
    print(f"  Anim dir:      {args.anim_dir}")
    if args.test_frame:
        print(f"  ** TEST MODE: 1 frame only **")
    print(f"{'='*60}")

    # --- 1. Find scene objects ---
    armature, body_meshes, equip_meshes = find_scene_objects()

    if not armature:
        print("[ERROR] No armature found in .blend file!")
        return

    print(f"[OK] Armature: {armature.name}")
    print(f"[OK] Body meshes ({len(body_meshes)}): "
          f"{[m.name[:40] for m in body_meshes]}")
    print(f"[OK] Equipment meshes ({len(equip_meshes)}): "
          f"{[m.name[:40] for m in equip_meshes]}")

    if not equip_meshes and not args.body_only:
        print("[WARN] No equipment meshes found (no parent_bone). "
              "Running body-only render.")
        args.body_only = True

    # --- 2. Remove existing cameras/lights from .blend ---
    for obj in list(bpy.context.scene.objects):
        if obj.type in ('LIGHT', 'CAMERA'):
            bpy.data.objects.remove(obj, do_unlink=True)
    print("[OK] Cleared .blend cameras/lights")

    # --- 3. Import animations ---
    print(f"\n--- Importing animations ---")
    anim_actions = import_animations(args.anim_dir)
    if not anim_actions:
        print("[WARN] No animations found — rendering static pose")
        anim_actions["static"] = {
            'action': None, 'atype': 'idle',
            'start': 1, 'end': 4, 'target_frames': 4,
        }

    # --- 4. Strip root motion ---
    strip_root_motion()

    # --- 5. Reset armature to rest pose for consistent centering ---
    # The .blend might be saved in any pose (attack, idle, etc.).
    # Rest pose = Mixamo T-pose = predictable bounds measurement.
    if armature.animation_data:
        armature.animation_data.action = None
    for bone in armature.pose.bones:
        bone.location = (0, 0, 0)
        bone.rotation_quaternion = (1, 0, 0, 0)
        bone.rotation_euler = (0, 0, 0)
        bone.scale = (1, 1, 1)
    bpy.context.view_layer.update()
    print("[OK] Armature reset to rest pose")

    # --- 6. Recover body textures if needed ---
    if args.texture_from and os.path.exists(args.texture_from):
        recover_textures(args.texture_from, body_meshes)

    # --- 7. Center + normalize from body bounds ---
    center_and_normalize_from_body(body_meshes, equip_meshes)

    # --- 8. Cel-shading (all meshes for consistent look) ---
    if not args.no_cel_shade:
        apply_cel_shade(body_meshes + equip_meshes, args.cel_shadow, args.cel_mid)

    # --- 9. Outline (body only — outline on small weapons looks wrong) ---
    if not args.no_outline:
        add_outline(body_meshes, args.outline_width)

    # --- 10. Camera + auto-frame from body bounds ---
    cam_obj = setup_camera()
    position_camera(cam_obj, 0, args.directions,
                    args.camera_angle, args.camera_target_z)
    auto_frame_from_body(cam_obj, body_meshes, equip_meshes, args.camera_angle)

    # --- 11. Lighting + render settings ---
    setup_lighting()
    setup_render(args.render_size)

    # --- 12. Dual-pass render ---
    passes = []
    if not args.weapon_only:
        passes.append("body")
    if not args.body_only and equip_meshes:
        passes.append("weapon")
    print(f"\n--- Rendering: {len(anim_actions)} animations x "
          f"{args.directions} directions, passes: {passes} ---")

    body_total, weapon_total = render_dual_pass(
        cam_obj, armature, body_meshes, equip_meshes, anim_actions, args)

    print(f"\n{'='*60}")
    print(f"  COMPLETE")
    if body_total:
        print(f"  Body:   {body_total} sprites -> {args.body_output}")
    if weapon_total:
        print(f"  Weapon: {weapon_total} sprites -> {args.weapon_output}")
    print(f"  Total:  {body_total + weapon_total} sprites")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
