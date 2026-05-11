# verify_hunyuan_collision.py
# Prints the current Collision Complexity value of every Hunyuan StaticMesh.
# Use this to confirm whether set_hunyuan_collision_complex.py actually
# persisted the change to disk.
#
# Run from UE5 Output Log (Python mode):
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/verify_hunyuan_collision.py"

import unreal

ROOT_PATH = "/Game/SabriMMO/Environment/Hunyuan"
TARGET = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE

eal = unreal.EditorAssetLibrary

FLAG_NAMES = {
    unreal.CollisionTraceFlag.CTF_USE_DEFAULT: "Default",
    unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX: "Simple AND Complex",
    unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX: "Simple As Complex",
    unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE: "Complex As Simple",
}


def main():
    counts = {name: 0 for name in FLAG_NAMES.values()}
    counts["unknown"] = 0
    counts["error"] = 0

    target_count = 0
    other_examples = []  # collect first 10 with non-target value

    for path in eal.list_assets(ROOT_PATH, recursive=True, include_folder=False):
        try:
            asset = unreal.load_asset(path)
            if not isinstance(asset, unreal.StaticMesh):
                continue
            body_setup = asset.get_editor_property("body_setup")
            if not body_setup:
                counts["error"] += 1
                continue
            flag = body_setup.get_editor_property("collision_trace_flag")
            name = FLAG_NAMES.get(flag, "unknown")
            counts[name] += 1
            if flag == TARGET:
                target_count += 1
            else:
                if len(other_examples) < 10:
                    other_examples.append((path.split("/")[-1], name))
        except Exception as e:
            counts["error"] += 1

    unreal.log("=" * 70)
    unreal.log("HUNYUAN COLLISION COMPLEXITY AUDIT")
    unreal.log("=" * 70)
    for name, n in counts.items():
        if n > 0:
            mark = "<-- TARGET" if name == "Complex As Simple" else ""
            unreal.log(f"  {name:25s}: {n:4d}  {mark}")

    if other_examples:
        unreal.log("")
        unreal.log("First 10 NON-target assets (to verify):")
        for short, name in other_examples:
            unreal.log(f"  {short}: {name}")

    unreal.log("=" * 70)


main()
