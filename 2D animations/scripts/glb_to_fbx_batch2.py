"""Batch GLB -> FBX converter (batch 2) - Mixamo-tuned settings."""

import bpy
import os

ENEMIES_DIR = r"C:\Sabri_MMO\2D animations\3d_models\enemies"

# (glb_filename, fbx_filename)
CONVERSIONS = [
    ("pitman.glb", "pitman.fbx"),
    ("hill_wind.glb", "hill_wind.fbx"),
    ("bathory.glb", "bathory.fbx"),
    ("megalith.glb", "megalith.fbx"),
    ("deviruchi.glb", "deviruchi.fbx"),
    ("iron_fist.glb", "iron_fist.fbx"),
    ("strouf.glb", "strouf.fbx"),
    ("firelock_soldier.glb", "antique_firelock.fbx"),
    ("Gargoyle.glb", "gargoyle.fbx"),
    ("jing_guai.glb", "li_me_mang_ryang.fbx"),
    ("orc_archer.glb", "orc_archer.fbx"),
    ("green_maiden.glb", "chung_e.fbx"),
    ("obsidian.glb", "obsidian.fbx"),
    ("knocker.glb", "knocker.fbx"),
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
    ok = fail = 0
    print(f"Converting {len(CONVERSIONS)} files in {ENEMIES_DIR}", flush=True)
    print("=" * 70, flush=True)
    for glb, fbx in CONVERSIONS:
        if convert_one(glb, fbx):
            ok += 1
        else:
            fail += 1
    print("=" * 70, flush=True)
    print(f"DONE: {ok} succeeded, {fail} failed", flush=True)


if __name__ == "__main__":
    main()
