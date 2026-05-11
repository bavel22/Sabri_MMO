# refresh_hunyuan_placed_actors.py
# Forces a physics-state rebuild on every placed StaticMeshActor in the
# current level that's using a Hunyuan static mesh. Use this after running
# set_hunyuan_collision_complex.py — placed instances cache their BodySetup
# and need a poke to pick up the new Collision Complexity setting.
#
# Run from UE5 Output Log (Python mode), with the affected level OPEN:
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/refresh_hunyuan_placed_actors.py"
#
# The level becomes dirty after this — Ctrl+S to save.

import unreal

ROOT_PATH = "/Game/SabriMMO/Environment/Hunyuan"

eal = unreal.EditorAssetLibrary
editor_actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def collect_hunyuan_sm_paths():
    """Returns a set of every Hunyuan StaticMesh's full object path."""
    paths = set()
    if not eal.does_directory_exist(ROOT_PATH):
        unreal.log_warning(f"Path not found: {ROOT_PATH}")
        return paths
    for asset_path in eal.list_assets(ROOT_PATH, recursive=True, include_folder=False):
        try:
            asset = unreal.load_asset(asset_path)
            if isinstance(asset, unreal.StaticMesh):
                paths.add(asset.get_path_name())
        except Exception:
            continue
    return paths


def main():
    unreal.log("=" * 70)
    unreal.log("REFRESH PLACED HUNYUAN ACTORS")
    unreal.log("=" * 70)

    hunyuan_paths = collect_hunyuan_sm_paths()
    unreal.log(f"  Found {len(hunyuan_paths)} Hunyuan StaticMesh assets")

    all_actors = editor_actor.get_all_level_actors()
    unreal.log(f"  Scanning {len(all_actors)} actors in current level")

    refreshed = 0
    not_sm = 0
    not_hunyuan = 0
    failed = 0

    for actor in all_actors:
        if not isinstance(actor, unreal.StaticMeshActor):
            not_sm += 1
            continue

        comp = actor.static_mesh_component
        if not comp:
            continue
        sm = comp.get_editor_property("static_mesh")
        if not sm:
            continue

        if sm.get_path_name() not in hunyuan_paths:
            not_hunyuan += 1
            continue

        try:
            # Trick: set the static mesh to itself to trigger PostEditChange,
            # which rebuilds the physics body using the new BodySetup.
            comp.set_static_mesh(sm)
            # Also explicitly recreate the physics state for safety
            comp.recreate_physics_state()
            refreshed += 1
        except Exception as e:
            unreal.log_warning(f"  failed to refresh {actor.get_actor_label()}: {e}")
            failed += 1

    unreal.log("")
    unreal.log("=" * 70)
    unreal.log(f"DONE")
    unreal.log(f"  Refreshed:        {refreshed} placed Hunyuan actors")
    unreal.log(f"  Skipped (not SM): {not_sm}")
    unreal.log(f"  Skipped (not Hunyuan): {not_hunyuan}")
    unreal.log(f"  Failed:           {failed}")
    unreal.log("=" * 70)
    unreal.log("Level is now dirty. Ctrl+S to save, then Build > Build Paths Only.")


main()
