"""Single GLB -> FBX converter (same Mixamo-tuned settings as batch script)."""

import bpy
import os
import sys

ENEMIES_DIR = r"C:\Sabri_MMO\2D animations\3d_models\enemies"

# (glb_filename, fbx_filename)
CONVERSIONS = [
    ("dumpling_child.glb", "rice_cake_boy.fbx"),
]


def convert_one(glb_name, fbx_name):
    glb_path = os.path.join(ENEMIES_DIR, glb_name)
    fbx_path = os.path.join(ENEMIES_DIR, fbx_name)

    if not os.path.isfile(glb_path):
        print(f"[SKIP] Source missing: {glb_path}", flush=True)
        return False

    bpy.ops.wm.read_factory_settings(use_empty=True)

    try:
        bpy.ops.import_scene.gltf(filepath=glb_path)
    except Exception as e:
        print(f"[FAIL] Import {glb_name}: {e}", flush=True)
        return False

    try:
        bpy.ops.export_scene.fbx(
            filepath=fbx_path,
            use_selection=False,
            object_types={'MESH', 'ARMATURE', 'EMPTY'},
            mesh_smooth_type='FACE',
            path_mode='COPY',
            embed_textures=True,
            add_leaf_bones=False,
            bake_anim=False,
            apply_scale_options='FBX_SCALE_ALL',
        )
    except Exception as e:
        print(f"[FAIL] Export {fbx_name}: {e}", flush=True)
        return False

    size = os.path.getsize(fbx_path) if os.path.isfile(fbx_path) else 0
    print(f"[OK] {glb_name} -> {fbx_name} ({size // 1024} KB)", flush=True)
    return True


def main():
    for glb, fbx in CONVERSIONS:
        convert_one(glb, fbx)


if __name__ == "__main__":
    main()
