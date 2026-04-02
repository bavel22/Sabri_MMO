# import_grass_meshes.py
# Imports the 6 grass/flower/pebble FBX meshes into UE5
# and updates the LandscapeGrassType assets to use them.

import unreal
import os
import glob

eal = unreal.EditorAssetLibrary

SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/Grass/Meshes"
DEST = "/Game/SabriMMO/Environment/Grass/Meshes"
GRASS_PATH = "/Game/SabriMMO/Environment/Grass"

eal.make_directory(DEST)

# Step 1: Import FBX meshes
unreal.log("=== Importing grass meshes ===")
fbx_files = sorted(glob.glob(os.path.join(SRC, "*.fbx")))
tasks = []
for fbx in fbx_files:
    name = os.path.splitext(os.path.basename(fbx))[0]
    if not eal.does_asset_exist(f"{DEST}/{name}"):
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", DEST)
        t.set_editor_property("filename", fbx)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)

if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.log(f"  Imported {len(tasks)} meshes")
else:
    unreal.log("  All meshes already imported")

# Step 2: Update GrassType assets to use proper meshes
unreal.log("\n=== Updating Grass Types ===")

GRASS_MESH_MAP = {
    "GT_GrassShort": f"{DEST}/SM_GrassBlade_01",
    "GT_GrassTall": f"{DEST}/SM_GrassClump_01",
    "GT_Flowers": f"{DEST}/SM_Flower_01",
    "GT_Pebbles": f"{DEST}/SM_Pebble_01",
}

for gt_name, mesh_path in GRASS_MESH_MAP.items():
    gt_path = f"{GRASS_PATH}/{gt_name}"
    if not eal.does_asset_exist(gt_path):
        unreal.log(f"  {gt_name}: not found, skipping")
        continue

    if not eal.does_asset_exist(mesh_path):
        unreal.log(f"  {gt_name}: mesh {mesh_path} not found, skipping")
        continue

    gt = unreal.load_asset(gt_path)
    mesh = unreal.load_asset(mesh_path)

    if gt and mesh:
        # Update the first variety's mesh
        varieties = gt.get_editor_property("grass_varieties")
        if len(varieties) > 0:
            varieties[0].set_editor_property("grass_mesh", mesh)
            gt.set_editor_property("grass_varieties", varieties)
            eal.save_asset(gt_path)
            unreal.log(f"  {gt_name}: mesh updated to {mesh_path.split('/')[-1]}")
        else:
            unreal.log(f"  {gt_name}: no varieties to update")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: Grass meshes imported and assigned")
unreal.log(f"  Meshes at: {DEST}")
unreal.log(f"  The landscape should now show grass blades instead of white planes")
unreal.log(f"{'='*60}")
