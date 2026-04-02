# fix_grass_meshes.py
# Force-updates all GrassType assets to use the proper meshes.
# Reads each GrassType, updates the mesh reference, saves.

import unreal

eal = unreal.EditorAssetLibrary

GRASS_PATH = "/Game/SabriMMO/Environment/Grass"
MESH_PATH = "/Game/SabriMMO/Environment/Grass/Meshes"

# Check what meshes are actually imported as UE5 assets (not just FBX files)
unreal.log("=== Checking imported meshes ===")
mesh_assets = eal.list_assets(MESH_PATH, recursive=False)
for m in sorted(mesh_assets):
    clean = m.split(".")[0]
    asset = unreal.load_asset(clean)
    asset_type = type(asset).__name__ if asset else "NOT FOUND"
    unreal.log(f"  {clean.split('/')[-1]}: {asset_type}")

# Check what grass types exist
unreal.log("\n=== Checking GrassType assets ===")
gt_assets = eal.list_assets(GRASS_PATH, recursive=False)
for g in sorted(gt_assets):
    clean = g.split(".")[0]
    gt = unreal.load_asset(clean)
    if gt and isinstance(gt, unreal.LandscapeGrassType):
        varieties = gt.get_editor_property("grass_varieties")
        for i, v in enumerate(varieties):
            mesh = v.get_editor_property("grass_mesh")
            mesh_name = mesh.get_name() if mesh else "NONE"
            unreal.log(f"  {clean.split('/')[-1]}: variety[{i}] mesh = {mesh_name}")

# Force update each
unreal.log("\n=== Force-updating mesh references ===")

MAP = {
    "GT_GrassShort": "SM_GrassBlade_01",
    "GT_GrassTall": "SM_GrassClump_01",
    "GT_Flowers": "SM_Flower_01",
    "GT_Pebbles": "SM_Pebble_01",
}

for gt_name, mesh_name in MAP.items():
    gt_path = f"{GRASS_PATH}/{gt_name}"

    # Try multiple possible mesh paths (UE5 may add suffixes during import)
    possible_mesh_paths = [
        f"{MESH_PATH}/{mesh_name}",
        f"{MESH_PATH}/{mesh_name}_00",
        f"{MESH_PATH}/{mesh_name}_{mesh_name}",
    ]

    # Also search for anything containing the mesh name
    for m in mesh_assets:
        clean_m = m.split(".")[0]
        if mesh_name.lower() in clean_m.lower():
            possible_mesh_paths.append(clean_m)

    gt = unreal.load_asset(gt_path)
    if not gt:
        unreal.log(f"  {gt_name}: GrassType not found!")
        continue

    mesh = None
    used_path = None
    for mp in possible_mesh_paths:
        if eal.does_asset_exist(mp):
            loaded = unreal.load_asset(mp)
            if loaded and isinstance(loaded, unreal.StaticMesh):
                mesh = loaded
                used_path = mp
                break

    if not mesh:
        unreal.log(f"  {gt_name}: could not find StaticMesh for {mesh_name}")
        unreal.log(f"    Tried: {possible_mesh_paths}")
        continue

    varieties = gt.get_editor_property("grass_varieties")
    if len(varieties) > 0:
        varieties[0].set_editor_property("grass_mesh", mesh)
        gt.set_editor_property("grass_varieties", varieties)
        eal.save_asset(gt_path)
        unreal.log(f"  {gt_name}: updated to {used_path}")
    else:
        unreal.log(f"  {gt_name}: no varieties array!")

unreal.log(f"\n  Done. If meshes still show as placeholders,")
unreal.log(f"  try: Build > Build Grass Maps in the editor menu")
