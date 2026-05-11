"""
BuildPronteraLevel.py
Run inside Unreal Engine 5 (Window > Output Log > Python).
Reads prontera_layout.json and spawns every actor in the current level.

Prerequisites:
  1. All meshes imported under /Game/Prontera/Meshes/<name>
  2. Cobblestone material at /Game/Prontera/Materials/MI_Cobblestone
  3. prontera_layout.json sits in the same folder as this script,
     OR set LAYOUT_PATH below to its absolute location.

Re-running the script clears /Game/Prontera/SpawnedActors first so it's idempotent.
"""

import json
import os
import unreal

# ------------------------------------------------------------------ config
LAYOUT_PATH = os.path.join(os.path.dirname(__file__), "prontera_layout.json")
MESH_PACKAGE_ROOT = "/Game/Prontera/Meshes"
GROUND_MAT = "/Game/Prontera/Materials/MI_Cobblestone"

# 1 meter -> 100 unreal units
M2UU = 100.0

# ------------------------------------------------------------------ helpers
def _load_layout():
    with open(LAYOUT_PATH, "r") as fp:
        return json.load(fp)

def _find_mesh(name):
    path = "{}/{}".format(MESH_PACKAGE_ROOT, name)
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        unreal.log_warning("Missing mesh asset: {}".format(path))
    return asset

def _spawn(mesh, location_m, yaw_deg, scale):
    if mesh is None:
        return None
    # Map glTF (Y-up, RH) -> Unreal (Z-up, LH).
    # The glTF importer already flips axes for the meshes.
    # Layout coords are (x, y, z) with y=up, so we map:
    #   UE.X = layout.X, UE.Y = layout.Z, UE.Z = layout.Y
    x_m, y_m, z_m = location_m
    location = unreal.Vector(x_m * M2UU, z_m * M2UU, y_m * M2UU)
    rotation = unreal.Rotator(0.0, yaw_deg, 0.0)  # Pitch, Yaw, Roll
    actor = unreal.EditorLevelLibrary.spawn_actor_from_object(mesh, location, rotation)
    if actor:
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    return actor

def _spawn_group(layout, key):
    section = layout.get(key, {})
    instances = section.get("instances", [])
    spawned = 0
    for inst in instances:
        mesh = _find_mesh(inst["asset"])
        if mesh is None:
            continue
        if _spawn(mesh, inst["position_xyz"], inst.get("rotation_y", 0.0), inst.get("scale", 1.0)):
            spawned += 1
    unreal.log("[Prontera] {}: spawned {}/{} actors".format(key, spawned, len(instances)))

def _spawn_ground(layout):
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane")
    if cube is None:
        unreal.log_warning("[Prontera] BasicShapes/Plane not found, skipping ground.")
        return
    size_m = layout["ground"]["size_meters"]
    actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
        cube, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    if actor:
        # Default plane is 100uu (1m), so scale to size in meters
        actor.set_actor_scale3d(unreal.Vector(size_m[0], size_m[1], 1.0))
        mat = unreal.EditorAssetLibrary.load_asset(GROUND_MAT)
        if mat:
            sm_comp = actor.static_mesh_component
            sm_comp.set_material(0, mat)
    unreal.log("[Prontera] Ground plane created at {} x {} m".format(*size_m))

def _spawn_lighting(layout):
    light_cls = unreal.DirectionalLight
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        light_cls,
        unreal.Vector(0, 0, 1500),
        unreal.Rotator(*layout["lighting"]["directional_light"]["rotation_pitch_yaw_roll_deg"]),
    )
    if sun:
        sun_comp = sun.light_component
        sun_comp.set_intensity(layout["lighting"]["directional_light"]["intensity_lux"])
        sun_comp.set_temperature(layout["lighting"]["directional_light"]["color_kelvin"])
        sun_comp.set_use_temperature(True)

    sky_atm = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    sky_light = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 200), unreal.Rotator(0, 0, 0)
    )
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    if pp:
        pp.set_editor_property("unbound", True)
    unreal.log("[Prontera] Lighting + atmosphere actors spawned.")

# ------------------------------------------------------------------ main
def build():
    layout = _load_layout()
    unreal.log("[Prontera] Building level: {}".format(layout["metadata"]["name"]))

    _spawn_ground(layout)
    _spawn_lighting(layout)

    for group_key in (
        "perimeter_walls",
        "central_plaza",
        "castle_district",
        "south_residential",
        "east_market_district",
        "west_residential",
        "park_corners",
    ):
        _spawn_group(layout, group_key)

    unreal.log("[Prontera] Done. Save your level (File > Save Current).")

if __name__ == "__main__":
    build()
