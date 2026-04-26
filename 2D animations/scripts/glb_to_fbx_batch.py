"""
Batch GLB -> FBX converter for Mixamo rigging.

Usage:
    blender --background --python glb_to_fbx_batch.py
"""

import bpy
import os
import sys

ENEMIES_DIR = r"C:\Sabri_MMO\2D animations\3d_models\enemies"

# (glb_filename, fbx_filename)
CONVERSIONS = [
    ("vocal.glb", "vocal.fbx"),
    ("orc baby.glb", "orc_baby.fbx"),
    ("metaller.glb", "metaller.fbx"),
    ("goblin.glb", "goblin.fbx"),
    ("orc_warrior.glb", "orc_warrior.fbx"),
    ("zerom.glb", "zerom.fbx"),
    ("raggler.glb", "raggler.fbx"),
    ("orc_zombie.glb", "orc_zombie.fbx"),
    ("smoking_orc.glb", "smoking_orc.fbx"),
    ("orc_xmas.glb", "orc_xmas.fbx"),
    ("golem.glb", "golem.fbx"),
    ("bigfoot.glb", "bigfoot.fbx"),
    ("pirate_skel.glb", "pirate_skel.fbx"),
    ("goblin_xmas.glb", "goblin_xmas.fbx"),
    ("rotar_zairo.glb", "rotar_zairo.fbx"),
    ("magnolia.glb", "magnolia.fbx"),
    ("orc_skeleton.glb", "orc_skeleton.fbx"),
    ("goblin_archer.glb", "goblin_archer.fbx"),
    ("soldier_skeleton.glb", "soldier_skeleton.fbx"),
    ("munak.glb", "munak.fbx"),
    ("sasquatch.glb", "sasquatch.fbx"),
    ("kobold.glb", "kobold.fbx"),
    ("zenorc.glb", "zenorc.fbx"),
    ("orc_lady.glb", "orc_lady.fbx"),
    ("bogun.glb", "bon_gun.fbx"),
    ("dokebi.glb", "dokebi.fbx"),
    ("sohee.glb", "sohee.fbx"),
    ("kobold_archer.glb", "kobold_archer.fbx"),
    ("miyabi_doll.glb", "miyabi_doll.fbx"),
    ("sandman.glb", "sand_man.fbx"),
    ("cruiser.glb", "cruiser.fbx"),
    ("requiem.glb", "requiem.fbx"),
    ("steam_goblin.glb", "steam_goblin.fbx"),
    ("lude.glb", "lude.fbx"),
    ("zipper_bear.glb", "zipper_bear.fbx"),
    ("mummy.glb", "mummy.fbx"),
    ("verit.glb", "verit.fbx"),
    ("jakk.glb", "jakk.fbx"),
    ("jakk_xmas.glb", "jakk_xmas.fbx"),
    ("ghoul.glb", "ghoul.fbx"),
    ("wootan_shooter.glb", "wootan_shooter.fbx"),
    ("marduk.glb", "marduk.fbx"),
    ("mime_monkey.glb", "mime_monkey.fbx"),
    ("marionette.glb", "marionette.fbx"),
    ("wootan_fighter.glb", "wootan_fighter.fbx"),
    ("chepet.glb", "chepet.fbx"),
    ("siroma.glb", "siroma.fbx"),
]


def convert_one(glb_name, fbx_name):
    glb_path = os.path.join(ENEMIES_DIR, glb_name)
    fbx_path = os.path.join(ENEMIES_DIR, fbx_name)

    if not os.path.isfile(glb_path):
        print(f"[SKIP] Source missing: {glb_path}", flush=True)
        return False

    # Reset scene
    bpy.ops.wm.read_factory_settings(use_empty=True)

    # Import GLB
    try:
        bpy.ops.import_scene.gltf(filepath=glb_path)
    except Exception as e:
        print(f"[FAIL] Import {glb_name}: {e}", flush=True)
        return False

    # Export FBX (Mixamo-compatible)
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
    ok = 0
    fail = 0
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
