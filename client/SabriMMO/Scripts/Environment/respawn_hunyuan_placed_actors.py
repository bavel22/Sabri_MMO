# respawn_hunyuan_placed_actors.py
# Nuclear refresh: for every placed StaticMeshActor in the current level
# that uses a Hunyuan static mesh, destroy and respawn the actor with the
# same transform, label, folder path, and tags. The new actor reads fresh
# asset state, including the re-cooked physics for Complex As Simple.
#
# Use this when set_hunyuan_collision_complex.py + refresh_hunyuan_placed_actors.py
# didn't fully take effect (UE5 caches cooked physics meshes on BodySetup
# and re-using them via recreate_physics_state can keep stale data).
#
# Run from UE5 Output Log (Python mode), with the affected level OPEN:
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/respawn_hunyuan_placed_actors.py"
#
# Save the level afterwards (Ctrl+S), then Build > Build Paths Only.

import unreal

ROOT_PATH = "/Game/SabriMMO/Environment/Hunyuan"

eal = unreal.EditorAssetLibrary
editor_actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def collect_hunyuan_sm_paths():
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


def respawn_actor(old_actor, sm):
    """Destroy old_actor and spawn a fresh StaticMeshActor with same identity."""
    label = old_actor.get_actor_label()
    folder_path = old_actor.get_folder_path()
    transform = old_actor.get_actor_transform()
    tags = list(old_actor.tags) if old_actor.tags else []
    mobility = old_actor.static_mesh_component.get_editor_property("mobility")

    # Spawn new at the right location
    new_actor = editor_actor.spawn_actor_from_class(
        unreal.StaticMeshActor,
        transform.translation,
        transform.rotation.rotator(),
    )
    if not new_actor:
        unreal.log_warning(f"  failed to spawn replacement for {label}")
        return None

    # Mobility must be set BEFORE assigning mesh on a Static actor
    new_actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    new_actor.static_mesh_component.set_static_mesh(sm)
    new_actor.set_actor_transform(transform, False, True)
    new_actor.static_mesh_component.set_mobility(mobility)

    # Preserve identity
    new_actor.set_actor_label(label)
    if folder_path:
        new_actor.set_folder_path(folder_path)
    for t in tags:
        new_actor.tags.append(t)

    # Destroy old
    editor_actor.destroy_actor(old_actor)
    return new_actor


def main():
    unreal.log("=" * 70)
    unreal.log("RESPAWN PLACED HUNYUAN ACTORS (destroy + respawn)")
    unreal.log("=" * 70)

    hunyuan_paths = collect_hunyuan_sm_paths()
    unreal.log(f"  Found {len(hunyuan_paths)} Hunyuan StaticMesh assets")

    all_actors = editor_actor.get_all_level_actors()
    unreal.log(f"  Scanning {len(all_actors)} actors in current level")

    # Collect targets first (don't iterate while destroying)
    targets = []
    for actor in all_actors:
        if not isinstance(actor, unreal.StaticMeshActor):
            continue
        comp = actor.static_mesh_component
        if not comp:
            continue
        sm = comp.get_editor_property("static_mesh")
        if not sm:
            continue
        if sm.get_path_name() in hunyuan_paths:
            targets.append((actor, sm))

    unreal.log(f"  {len(targets)} placed actors target for respawn")
    if not targets:
        unreal.log("  Nothing to do.")
        return

    respawned = 0
    failed = 0
    for actor, sm in targets:
        try:
            label = actor.get_actor_label()
            new_actor = respawn_actor(actor, sm)
            if new_actor:
                respawned += 1
                if respawned % 25 == 0:
                    unreal.log(f"  ...{respawned}/{len(targets)} done")
            else:
                failed += 1
        except Exception as e:
            unreal.log_warning(f"  failed for {label}: {e}")
            failed += 1

    unreal.log("")
    unreal.log("=" * 70)
    unreal.log(f"DONE")
    unreal.log(f"  Respawned: {respawned}")
    unreal.log(f"  Failed:    {failed}")
    unreal.log("=" * 70)
    unreal.log("Save the level (Ctrl+S), then Build > Build Paths Only.")


main()
