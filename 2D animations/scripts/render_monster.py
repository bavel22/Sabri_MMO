"""
render_monster.py - Non-humanoid Monster Sprite Renderer for Sabri_MMO
======================================================================
Renders blob/amorphous monsters (Poring, Drops, etc.) using shape key
animations instead of skeletal (Mixamo) animations. Uses the same
cel-shading, camera, and lighting as blender_sprite_render_v2.py.

Usage:
  "C:/Blender 5.1/blender.exe" --background --python \
    "C:/Sabri_MMO/2D animations/scripts/render_monster.py" -- \
    "C:/Sabri_MMO/2D animations/3d_models/characters/poring/poring.glb" \
    "C:/Sabri_MMO/2D animations/sprites/render_output/poring" \
    --monster-type blob \
    --render-size 1024 --camera-angle 10 --camera-target-z 0.7

Output matches the folder structure expected by pack_atlas.py (v2):
  output_dir/
    Idle Bounce/
      Idle Bounce_S_f00.png ... Idle Bounce_SE_f07.png
    Hop Forward/
      ...
    Lunge Attack/
      ...
    Hit Squish/
      ...
    Flatten Death/
      ...
"""

import bpy
import math
import os
import sys
import argparse
import mathutils


# =============================================================
# Constants
# =============================================================

DIRECTION_NAMES_8 = ["S", "SW", "W", "NW", "N", "NE", "E", "SE"]
DIRECTION_NAMES_4 = ["S", "W", "N", "E"]

DEFAULT_FRAME_TARGETS = {
    "idle": 8, "walk": 12, "attack": 10, "hit": 6, "death": 8,
}


# =============================================================
# Shape Key Presets
# =============================================================

BLOB_SHAPE_KEYS = {
    # Deformation keys: scale from ground plane (z_min anchor)
    'Squash':  {'z_scale': 0.6,  'xy_scale': 1.25},
    'Stretch': {'z_scale': 1.35, 'xy_scale': 0.85},
    'Flatten': {'z_scale': 0.15, 'xy_scale': 2.0},
    # Translation key: uniform vertical offset (hop)
    'HopUp':   {'z_offset': 0.25},  # max hop = 25% of body height
}

BLOB_ANIMATIONS = {
    'Idle Bounce': {
        'total_frames': 56,
        'classified_type': 'idle',
        'keyframes': [
            # (frame, {shape_key: value, ...})
            # Unspecified keys default to 0.0
            (0,  {}),
            (14, {'Squash': 0.12}),
            (28, {'Stretch': 0.1, 'HopUp': 0.15}),
            (42, {'Squash': 0.05}),
            (56, {}),  # loops back to start
        ],
    },
    'Hop Forward': {
        'total_frames': 36,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.3}),                       # crouch
            (6,  {'Stretch': 0.3, 'HopUp': 0.5}),       # launch
            (14, {'Stretch': 0.15, 'HopUp': 1.0}),      # peak of hop
            (22, {'Stretch': 0.1, 'HopUp': 0.5}),       # descending
            (28, {'Squash': 0.3}),                       # landing
            (33, {'Squash': 0.1}),                       # recover
            (36, {}),                                    # rest (loop)
        ],
    },
    'Lunge Attack': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                    # rest
            (5,  {'Squash': 0.35}),                      # wind up
            (10, {'Stretch': 0.4, 'HopUp': 0.4}),       # lunge upward
            (15, {'Stretch': 0.3, 'HopUp': 0.2}),       # extending
            (20, {'Squash': 0.15}),                      # recoil
            (25, {'Squash': 0.05}),                      # settle
            (30, {}),                                    # rest
        ],
    },
    'Hit Squish': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.4}),                       # impact
            (5,  {'Squash': 0.5}),                       # max compression
            (10, {'Stretch': 0.2, 'HopUp': 0.2}),       # rebound
            (14, {'Stretch': 0.05}),                     # settling
            (18, {}),                                    # rest
        ],
    },
    'Flatten Death': {
        'total_frames': 24,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                    # normal
            (4,  {'Squash': 0.3}),                       # initial squash
            (8,  {'Stretch': 0.2, 'HopUp': 0.25}),      # bounce up
            (12, {'Flatten': 0.3}),                      # start flattening
            (18, {'Flatten': 0.7}),                      # mostly flat
            (24, {'Flatten': 1.0}),                      # fully flat
        ],
    },
}

# Monster type registry
MONSTER_PRESETS = {
    'blob': {
        'shape_keys': BLOB_SHAPE_KEYS,
        'animations': BLOB_ANIMATIONS,
    },
    # Future: 'quadruped', 'insect', 'plant', etc.
}


# =============================================================
# Argument Parsing
# =============================================================

def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser(
        description="Render non-humanoid monsters to sprite sheets")
    parser.add_argument("input", help="Model file (GLB/FBX)")
    parser.add_argument("output", nargs="?",
                        default="C:/Sabri_MMO/2D animations/sprites/render_output",
                        help="Output directory")
    parser.add_argument("--monster-type", default="blob",
                        choices=list(MONSTER_PRESETS.keys()),
                        help="Monster type preset (default: blob)")
    parser.add_argument("--directions", type=int, default=8, choices=[4, 8])
    parser.add_argument("--render-size", type=int, default=1024)
    parser.add_argument("--no-cel-shade", action="store_true")
    parser.add_argument("--no-outline", action="store_true")
    parser.add_argument("--outline-width", type=float, default=0.002)
    parser.add_argument("--cel-shadow", type=float, default=0.45)
    parser.add_argument("--cel-mid", type=float, default=0.78)
    parser.add_argument("--camera-angle", type=float, default=10.0,
                        help="Camera elevation degrees (default 10)")
    parser.add_argument("--camera-target-z", type=float, default=0.7,
                        help="Camera look-at Z (default 0.7)")
    parser.add_argument("--model-rotation", type=float, default=0.0,
                        help="Rotate model around Z axis (degrees) before rendering. "
                             "Use to align face with south camera (dir_idx=0).")
    parser.add_argument("--save-blend", action="store_true",
                        help="Save .blend file for manual tweaking")

    return parser.parse_args(argv)


# =============================================================
# Scene Setup
# =============================================================

def clear_scene():
    """Remove all default objects."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for block in bpy.data.meshes:
        if block.users == 0:
            bpy.data.meshes.remove(block)
    print("[OK] Scene cleared")


def import_model(filepath):
    """Import GLB/FBX model."""
    ext = os.path.splitext(filepath)[1].lower()
    if ext in ('.glb', '.gltf'):
        bpy.ops.import_scene.gltf(filepath=filepath)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=filepath)
    else:
        raise ValueError(f"Unsupported format: {ext}")
    print(f"[OK] Imported {os.path.basename(filepath)}")


def join_meshes():
    """Join all mesh objects into a single mesh."""
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if not meshes:
        raise RuntimeError("No mesh objects found after import")
    if len(meshes) == 1:
        print(f"[OK] Single mesh: {meshes[0].name}")
        return meshes[0]

    # Select all meshes, make first one active
    bpy.ops.object.select_all(action='DESELECT')
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()

    result = bpy.context.active_object
    print(f"[OK] Joined {len(meshes)} meshes -> {result.name}")
    return result


def get_scene_bounds():
    """World-space bounding box of all mesh objects."""
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

    offset = mathutils.Vector((-center.x, -center.y, -min_co.z))
    roots = [o for o in bpy.context.scene.objects if o.parent is None]
    for obj in roots:
        obj.location += offset
    bpy.context.view_layer.update()

    if height > 0.001:
        scale_factor = 2.0 / height
        for obj in roots:
            obj.scale *= scale_factor
        bpy.context.view_layer.update()

    # Apply transforms so shape key vertex coordinates are in world space
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            obj.select_set(False)

    print(f"[OK] Centered (height {height:.3f} -> 2.0)")


# =============================================================
# Shape Keys
# =============================================================

def compute_mesh_bounds(mesh_obj):
    """Get bounding box info from the mesh's vertex data."""
    verts = mesh_obj.data.vertices
    zs = [v.co.z for v in verts]
    xs = [v.co.x for v in verts]
    ys = [v.co.y for v in verts]
    center = mathutils.Vector((
        (min(xs) + max(xs)) / 2,
        (min(ys) + max(ys)) / 2,
        (min(zs) + max(zs)) / 2,
    ))
    return center, min(zs), max(zs)


def add_shape_keys(mesh_obj, preset):
    """Add shape keys to the mesh based on the preset definition.

    Deformation keys (Squash/Stretch/Flatten) anchor to z_min so the
    model stays on the ground plane. The HopUp key translates all
    vertices uniformly upward for hop/bounce animations.
    """
    # Create Basis if needed
    if not mesh_obj.data.shape_keys:
        mesh_obj.shape_key_add(name='Basis', from_mix=False)

    basis = mesh_obj.data.shape_keys.key_blocks['Basis']
    center, z_min, z_max = compute_mesh_bounds(mesh_obj)
    height = z_max - z_min

    for sk_name, params in preset.items():
        key = mesh_obj.shape_key_add(name=sk_name, from_mix=False)
        key.value = 0.0

        if 'z_offset' in params:
            # Uniform vertical translation (HopUp)
            offset = params['z_offset'] * height
            for i in range(len(key.data)):
                bco = basis.data[i].co
                key.data[i].co = mathutils.Vector((bco.x, bco.y, bco.z + offset))
        else:
            # Scale deformation anchored to z_min (ground)
            z_scale = params['z_scale']
            xy_scale = params['xy_scale']
            for i in range(len(key.data)):
                bco = basis.data[i].co
                key.data[i].co = mathutils.Vector((
                    center.x + (bco.x - center.x) * xy_scale,
                    center.y + (bco.y - center.y) * xy_scale,
                    z_min + (bco.z - z_min) * z_scale,
                ))

    print(f"[OK] Added {len(preset)} shape keys: {list(preset.keys())}")


# =============================================================
# Animation Creation
# =============================================================

def create_animations(mesh_obj, anim_defs, all_sk_names):
    """Create one Blender action per animation with shape key keyframes.

    Returns dict: { anim_name: { 'action', 'total_frames', 'classified_type' } }
    """
    sk = mesh_obj.data.shape_keys
    if not sk.animation_data:
        sk.animation_data_create()

    animations = {}

    for anim_name, anim_def in anim_defs.items():
        action = bpy.data.actions.new(name=anim_name)
        action.use_fake_user = True
        sk.animation_data.action = action

        total = anim_def['total_frames']
        keyframes = anim_def['keyframes']

        # For each keyframe, set all shape key values and insert
        for frame, sk_values in keyframes:
            bpy.context.scene.frame_set(frame)
            for sk_name in all_sk_names:
                val = sk_values.get(sk_name, 0.0)
                sk.key_blocks[sk_name].value = val
                sk.key_blocks[sk_name].keyframe_insert('value', frame=frame)

        # Set interpolation to BEZIER for smooth transitions
        if hasattr(action, 'fcurves'):
            for fc in action.fcurves:
                for kp in fc.keyframe_points:
                    kp.interpolation = 'BEZIER'
        elif hasattr(action, 'layers'):
            # Blender 5.x layered action API
            for layer in action.layers:
                for strip in layer.strips:
                    for bag in strip.channelbags:
                        for fc in bag.fcurves:
                            for kp in fc.keyframe_points:
                                kp.interpolation = 'BEZIER'

        animations[anim_name] = {
            'action': action,
            'total_frames': total,
            'classified_type': anim_def['classified_type'],
        }
        print(f"  {anim_name}: {total} frames, "
              f"{len(keyframes)} keyframes, type={anim_def['classified_type']}")

    # Reset all shape keys to 0
    for sk_name in all_sk_names:
        sk.key_blocks[sk_name].value = 0.0

    print(f"[OK] Created {len(animations)} animations")
    return animations


# =============================================================
# Cel-Shading (identical to blender_sprite_render_v2.py)
# =============================================================

def setup_cel_shade(shadow_brightness=0.45, mid_brightness=0.78):
    """Color-preserving cel-shading with 4 brightness steps."""
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

            principled = None
            for node in nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    principled = node
                    break
            if not principled:
                continue

            output_node = None
            for node in nodes:
                if node.type == 'OUTPUT_MATERIAL':
                    output_node = node
                    break
            if not output_node:
                continue

            base_color_source = None
            for link in links:
                if link.to_socket == principled.inputs['Base Color']:
                    base_color_source = link.from_socket
                    break
            base_color_value = principled.inputs['Base Color'].default_value[:]

            px = principled.location.x
            py = principled.location.y

            # Shader to RGB
            s2rgb = nodes.new('ShaderNodeShaderToRGB')
            s2rgb.location = (px + 300, py)
            links.new(principled.outputs['BSDF'], s2rgb.inputs['Shader'])

            # RGB to BW (luminance)
            rgb2bw = nodes.new('ShaderNodeRGBToBW')
            rgb2bw.location = (px + 500, py - 150)
            links.new(s2rgb.outputs['Color'], rgb2bw.inputs['Color'])

            # ColorRamp (4 cel-shade steps)
            ramp = nodes.new('ShaderNodeValToRGB')
            ramp.location = (px + 700, py - 150)
            ramp.color_ramp.interpolation = 'CONSTANT'

            cr = ramp.color_ramp
            ds = shadow_brightness * 0.65
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

            # Multiply: ramp x base color
            mix = nodes.new('ShaderNodeMix')
            mix.data_type = 'RGBA'
            mix.blend_type = 'MULTIPLY'
            mix.location = (px + 950, py)
            mix.inputs[0].default_value = 1.0

            links.new(ramp.outputs['Color'], mix.inputs[6])
            if base_color_source:
                links.new(base_color_source, mix.inputs[7])
            else:
                mix.inputs[7].default_value = base_color_value

            # Emission (avoid Eevee double-lighting)
            emission = nodes.new('ShaderNodeEmission')
            emission.location = (px + 1200, py)
            links.new(mix.outputs[2], emission.inputs['Color'])

            # Handle alpha transparency
            alpha_source = None
            for link in list(links):
                if link.to_socket == principled.inputs['Alpha']:
                    alpha_source = link.from_socket
                    break
            has_alpha = (alpha_source is not None or
                         principled.inputs['Alpha'].default_value < 0.999)

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

    print(f"[OK] Cel-shade applied to {processed} materials")


def add_outline(thickness=0.002):
    """Add solidify modifier for 1px-style black outline."""
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

    print(f"[OK] Outline added to {count} meshes")


# =============================================================
# Camera, Lighting, Render Settings
# =============================================================

def setup_camera(elevation_deg=10.0):
    """Create orthographic camera."""
    cam_data = bpy.data.cameras.new(name="SpriteCamera")
    cam_data.type = 'ORTHO'
    cam_data.ortho_scale = 2.8

    cam_obj = bpy.data.objects.new("SpriteCamera", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    print(f"[OK] Camera created (ortho, {elevation_deg} deg)")
    return cam_obj


def auto_frame_camera(cam_obj, mesh_obj, sk_names, elevation_deg=10.0,
                       padding=1.2):
    """Frame camera to fit the model at its tallest pose (Stretch + HopUp).

    Temporarily activates shape keys to compute worst-case bounds, then resets.
    """
    sk = mesh_obj.data.shape_keys
    if sk:
        # Activate stretch + hop to find max bounds
        for name in sk_names:
            if name in ('Stretch', 'HopUp'):
                sk.key_blocks[name].value = 1.0
        bpy.context.view_layer.update()

    min_co, max_co = get_scene_bounds()

    # Reset
    if sk:
        for name in sk_names:
            sk.key_blocks[name].value = 0.0
        bpy.context.view_layer.update()

    if min_co.x == float('inf'):
        return

    size = max_co - min_co
    ev = math.radians(elevation_deg)
    vis_h = size.z * math.cos(ev) + max(size.x, size.y) * math.sin(ev)
    vis_w = max(size.x, size.y)
    scale = max(vis_h, vis_w) * padding
    cam_obj.data.ortho_scale = max(scale, 2.5)

    print(f"[OK] Auto-frame ortho_scale={cam_obj.data.ortho_scale:.2f}")


def setup_lighting():
    """Three-point lighting for clear, consistent sprites."""
    key = bpy.data.lights.new("KeyLight", 'SUN')
    key.energy = 3.5
    key.color = (1.0, 0.98, 0.95)
    key_obj = bpy.data.objects.new("KeyLight", key)
    bpy.context.scene.collection.objects.link(key_obj)
    key_obj.rotation_euler = (math.radians(50), math.radians(10),
                              math.radians(-25))

    fill = bpy.data.lights.new("FillLight", 'SUN')
    fill.energy = 1.5
    fill.color = (0.95, 0.95, 1.0)
    fill_obj = bpy.data.objects.new("FillLight", fill)
    bpy.context.scene.collection.objects.link(fill_obj)
    fill_obj.rotation_euler = (math.radians(55), math.radians(-10),
                               math.radians(155))

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

    print(f"[OK] Render: {render_size}x{render_size} RGBA PNG")


# =============================================================
# Rendering
# =============================================================

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


def sample_frames(start, end, target_count):
    """Evenly sample target_count frames from [start, end]."""
    total = end - start + 1
    if total <= target_count:
        return list(range(start, end + 1))
    step = (total - 1) / max(target_count - 1, 1)
    return [int(start + i * step) for i in range(target_count)]


def render_all(cam_obj, output_dir, mesh_obj, animations, args):
    """Render every animation x direction x sampled frame."""
    os.makedirs(output_dir, exist_ok=True)
    dir_names = DIRECTION_NAMES_8 if args.directions == 8 else DIRECTION_NAMES_4
    num_dirs = args.directions

    sk = mesh_obj.data.shape_keys
    total = 0

    for anim_name, anim_info in animations.items():
        atype = anim_info['classified_type']
        target_frames = DEFAULT_FRAME_TARGETS.get(atype, 8)
        total_frames = anim_info['total_frames']

        # Switch to this animation's action
        sk.animation_data.action = anim_info['action']

        frames = sample_frames(0, total_frames, target_frames)

        for dir_idx in range(num_dirs):
            dir_name = dir_names[dir_idx]
            position_camera(cam_obj, dir_idx, num_dirs,
                            args.camera_angle, args.camera_target_z)

            for fi, frame in enumerate(frames):
                bpy.context.scene.frame_set(frame)
                bpy.context.view_layer.update()

                # Output to subfolder per animation
                anim_dir = os.path.join(output_dir, anim_name)
                os.makedirs(anim_dir, exist_ok=True)

                filename = f"{anim_name}_{dir_name}_f{fi:02d}.png"
                filepath = os.path.join(anim_dir, filename)

                bpy.context.scene.render.filepath = filepath
                bpy.ops.render.render(write_still=True)
                total += 1

        print(f"  {anim_name}: {len(frames)} frames x {num_dirs} dirs")

    # Reset shape keys
    for kb in sk.key_blocks:
        if kb.name != 'Basis':
            kb.value = 0.0

    print(f"\n[OK] Rendered {total} sprites -> {output_dir}")
    return total


# =============================================================
# Main Pipeline
# =============================================================

def main():
    args = parse_args()

    preset = MONSTER_PRESETS[args.monster_type]
    sk_preset = preset['shape_keys']
    anim_defs = preset['animations']
    sk_names = list(sk_preset.keys())

    print(f"\n{'='*60}")
    print(f"Monster Sprite Renderer - {args.monster_type}")
    print(f"{'='*60}")
    print(f"  Input:  {args.input}")
    print(f"  Output: {args.output}")
    print(f"  Size:   {args.render_size}x{args.render_size}")
    print(f"  Dirs:   {args.directions}")
    print(f"  Anims:  {list(anim_defs.keys())}")
    print()

    # 1. Clear scene and import
    clear_scene()
    import_model(os.path.abspath(args.input))

    # 2. Remove any armatures from the import (not needed for shape keys)
    for obj in list(bpy.context.scene.objects):
        if obj.type == 'ARMATURE':
            bpy.data.objects.remove(obj, do_unlink=True)

    # 3. Join meshes and normalize
    mesh_obj = join_meshes()
    center_and_normalize()

    # 3b. Rotate model to align face with south camera (dir_idx=0)
    if args.model_rotation != 0.0:
        import bmesh
        rot_rad = math.radians(args.model_rotation)
        rot_matrix = mathutils.Matrix.Rotation(rot_rad, 4, 'Z')
        bm = bmesh.new()
        bm.from_mesh(mesh_obj.data)
        bmesh.ops.rotate(bm, verts=bm.verts, matrix=rot_matrix,
                         cent=mathutils.Vector((0, 0, 0)))
        bm.to_mesh(mesh_obj.data)
        bm.free()
        mesh_obj.data.update()
        bpy.context.view_layer.update()
        print(f"[OK] Rotated mesh vertices {args.model_rotation}° around Z")

    # 4. Add shape keys
    print("\n--- Shape Keys ---")
    add_shape_keys(mesh_obj, sk_preset)

    # 5. Create animations
    print("\n--- Animations ---")
    animations = create_animations(mesh_obj, anim_defs, sk_names)

    # 6. Visual setup
    print("\n--- Visual Setup ---")
    if not args.no_cel_shade:
        setup_cel_shade(args.cel_shadow, args.cel_mid)
    if not args.no_outline:
        add_outline(args.outline_width)
    setup_lighting()
    setup_render(args.render_size)

    cam = setup_camera(args.camera_angle)
    auto_frame_camera(cam, mesh_obj, sk_names, args.camera_angle)

    # 7. Save .blend for manual tweaking
    if args.save_blend:
        blend_path = os.path.join(args.output, "monster_scene.blend")
        os.makedirs(args.output, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=blend_path)
        print(f"[OK] Saved {blend_path}")

    # 8. Render
    print("\n--- Rendering ---")
    render_all(cam, args.output, mesh_obj, animations, args)

    print(f"\n{'='*60}")
    print(f"DONE. Output: {args.output}")
    print(f"Next: pack with pack_atlas.py using a v2 config")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
