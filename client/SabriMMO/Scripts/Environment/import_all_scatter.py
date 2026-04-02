# import_all_scatter.py
# Imports all 75 scatter FBX meshes, creates materials for each,
# and creates per-zone LandscapeGrassType assets.

import unreal
import os
import glob

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MESH_SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Environment/Grass/Meshes"
MESH_DEST = "/Game/SabriMMO/Environment/Grass/Meshes"
GT_DEST = "/Game/SabriMMO/Environment/Grass/Types"
MAT_DEST = "/Game/SabriMMO/Environment/Grass/Materials"

eal.make_directory(MESH_DEST)
eal.make_directory(GT_DEST)
eal.make_directory(MAT_DEST)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# ============================================================
# STEP 1: Import all FBX meshes
# ============================================================
unreal.log("=== Importing scatter meshes ===")
fbx_files = sorted(glob.glob(os.path.join(MESH_SRC, "*.fbx")))
tasks = []
for fbx in fbx_files:
    name = os.path.splitext(os.path.basename(fbx))[0]
    if not eal.does_asset_exist(f"{MESH_DEST}/{name}"):
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", MESH_DEST)
        t.set_editor_property("filename", fbx)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
if tasks:
    asset_tools.import_asset_tasks(tasks)
unreal.log(f"  Imported {len(tasks)} new meshes")

# ============================================================
# STEP 2: Create per-zone GrassType assets
# ============================================================
unreal.log("\n=== Creating per-zone GrassType assets ===")

# Zone configs: zone_name -> list of (mesh_name, density, cull_start, cull_end, scale_min, scale_max)
ZONE_SCATTER = {
    "grassland": [
        ("SM_GrassBlade_01", 12.0, 3000, 5000, 0.03, 0.06),
        ("SM_GrassBlade_04", 8.0, 2500, 4500, 0.03, 0.05),
        ("SM_GrassClump_01", 5.0, 2500, 4000, 0.04, 0.08),
        ("SM_Flower_01", 2.0, 2000, 3500, 0.02, 0.04),
        ("SM_Dandelion_01", 1.5, 2000, 3500, 0.02, 0.03),
        ("SM_Clover_01", 3.0, 1500, 3000, 0.03, 0.06),
        ("SM_Pebble_01", 2.0, 1500, 3000, 0.005, 0.012),
        ("SM_Twig_01", 1.0, 1500, 2500, 0.02, 0.04),
    ],
    "forest": [
        ("SM_GrassBlade_05", 6.0, 2500, 4000, 0.03, 0.05),
        ("SM_Fern_01", 4.0, 2000, 3500, 0.04, 0.08),
        ("SM_Mushroom_01", 1.5, 1500, 3000, 0.03, 0.06),
        ("SM_Mushroom_02", 1.0, 1500, 3000, 0.02, 0.04),
        ("SM_FallenLeaf_01", 5.0, 1500, 3000, 0.04, 0.08),
        ("SM_FallenLeaf_02", 4.0, 1500, 3000, 0.04, 0.07),
        ("SM_PineCone_01", 1.0, 1500, 2500, 0.03, 0.05),
        ("SM_MossClump_01", 2.0, 1500, 3000, 0.04, 0.08),
        ("SM_BarkPiece_01", 1.5, 1500, 2500, 0.03, 0.06),
        ("SM_Acorn_01", 2.0, 1000, 2000, 0.02, 0.04),
    ],
    "desert": [
        ("SM_DryGrass_01", 4.0, 2500, 4000, 0.03, 0.06),
        ("SM_DryGrass_02", 3.0, 2500, 4000, 0.03, 0.05),
        ("SM_DesertRock_01", 2.0, 2000, 3500, 0.02, 0.05),
        ("SM_DesertRock_02", 2.5, 1500, 3000, 0.015, 0.035),
        ("SM_DryTwig_01", 1.5, 1500, 2500, 0.02, 0.04),
        ("SM_SmallCactus_01", 0.5, 2000, 3500, 0.03, 0.06),
        ("SM_BoneFrag_01", 0.5, 1000, 2000, 0.02, 0.04),
    ],
    "snow": [
        ("SM_SnowClump_01", 5.0, 2500, 4000, 0.03, 0.07),
        ("SM_SnowClump_02", 4.0, 2000, 3500, 0.02, 0.05),
        ("SM_IceCrystal_01", 1.5, 1500, 3000, 0.02, 0.04),
        ("SM_IceCrystal_02", 1.0, 1500, 3000, 0.015, 0.03),
        ("SM_FrozenTwig_01", 1.5, 1500, 2500, 0.02, 0.04),
        ("SM_Pebble_02", 2.0, 1500, 2500, 0.005, 0.012),
    ],
    "beach": [
        ("SM_Shell_01", 3.0, 1500, 3000, 0.03, 0.06),
        ("SM_Shell_02", 2.5, 1500, 3000, 0.025, 0.05),
        ("SM_CoralBit_01", 1.5, 1500, 2500, 0.02, 0.04),
        ("SM_Starfish_01", 0.5, 1500, 3000, 0.03, 0.06),
        ("SM_Driftwood_01", 0.5, 2000, 3500, 0.03, 0.06),
        ("SM_Seaweed_01", 2.0, 1500, 2500, 0.02, 0.04),
        ("SM_Pebble_03", 3.0, 1000, 2000, 0.005, 0.01),
    ],
    "volcanic": [
        ("SM_ObsidianShard_01", 3.0, 2000, 3500, 0.02, 0.04),
        ("SM_ObsidianShard_02", 2.0, 1500, 3000, 0.015, 0.03),
        ("SM_LavaRock_01", 2.5, 2000, 3500, 0.02, 0.05),
        ("SM_LavaRock_02", 2.0, 1500, 3000, 0.015, 0.035),
        ("SM_SulfurCrystal_01", 1.0, 1500, 2500, 0.015, 0.03),
        ("SM_AshClump_01", 3.0, 1500, 2500, 0.02, 0.04),
    ],
    "cursed": [
        ("SM_DeadGrass_01", 5.0, 2500, 4000, 0.03, 0.06),
        ("SM_DeadGrass_02", 4.0, 2000, 3500, 0.025, 0.05),
        ("SM_Bone_01", 1.5, 1500, 3000, 0.02, 0.04),
        ("SM_BoneStick_01", 1.0, 1500, 2500, 0.02, 0.04),
        ("SM_SmallSkull_01", 0.3, 1500, 3000, 0.02, 0.04),
        ("SM_CandleStub_01", 0.5, 1000, 2000, 0.02, 0.04),
    ],
    "amatsu": [
        ("SM_GrassBlade_01", 8.0, 2500, 4000, 0.03, 0.05),
        ("SM_CherryPetal_01", 6.0, 1500, 3000, 0.03, 0.06),
        ("SM_BambooLeaf_01", 4.0, 1500, 3000, 0.03, 0.06),
        ("SM_Pebble_01", 2.0, 1000, 2000, 0.005, 0.01),
    ],
    "moscovia": [
        ("SM_GrassBlade_04", 6.0, 2500, 4000, 0.03, 0.05),
        ("SM_BirchLeaf_01", 5.0, 1500, 3000, 0.04, 0.07),
        ("SM_FallenLeaf_01", 4.0, 1500, 3000, 0.04, 0.07),
        ("SM_PineCone_01", 1.5, 1500, 2500, 0.03, 0.05),
        ("SM_Berry_01", 1.0, 1000, 2000, 0.01, 0.02),
        ("SM_MossClump_01", 2.0, 1500, 2500, 0.03, 0.06),
    ],
    "ruins": [
        ("SM_StoneFrag_01", 3.0, 2000, 3500, 0.02, 0.05),
        ("SM_StoneFrag_02", 2.5, 1500, 3000, 0.015, 0.035),
        ("SM_PotteryShard_01", 1.5, 1500, 2500, 0.03, 0.05),
        ("SM_DryGrass_01", 3.0, 2000, 3500, 0.02, 0.04),
        ("SM_DirtClump_01", 2.0, 1500, 2500, 0.02, 0.04),
    ],
    "industrial": [
        ("SM_MetalScrap_01", 2.5, 1500, 3000, 0.03, 0.06),
        ("SM_Bolt_01", 2.0, 1000, 2000, 0.02, 0.04),
        ("SM_RustFlake_01", 3.0, 1000, 2000, 0.03, 0.05),
        ("SM_Pebble_02", 2.0, 1500, 2500, 0.005, 0.01),
        ("SM_SmallRock_02", 1.0, 1500, 2500, 0.01, 0.02),
    ],
    "cave": [
        ("SM_CrystalShard_01", 1.5, 1500, 3000, 0.02, 0.04),
        ("SM_CrystalShard_02", 1.0, 1500, 2500, 0.015, 0.03),
        ("SM_WetRock_01", 3.0, 2000, 3500, 0.02, 0.04),
        ("SM_StalagmiteNub_01", 2.0, 1500, 3000, 0.02, 0.04),
        ("SM_Pebble_03", 3.0, 1000, 2000, 0.005, 0.01),
    ],
    "dungeon": [
        ("SM_Rubble_01", 3.0, 2000, 3500, 0.02, 0.04),
        ("SM_Rubble_02", 2.5, 1500, 3000, 0.015, 0.03),
        ("SM_ChainLink_01", 0.5, 1000, 2000, 0.02, 0.04),
        ("SM_Bone_01", 1.0, 1500, 2500, 0.02, 0.03),
        ("SM_DirtClump_01", 2.0, 1500, 2500, 0.02, 0.04),
    ],
}

created_gt = 0
for zone_name, scatter_list in ZONE_SCATTER.items():
    gt_name = f"GT_{zone_name.capitalize()}"
    gt_path = f"{GT_DEST}/{gt_name}"

    if eal.does_asset_exist(gt_path):
        gt = unreal.load_asset(gt_path)
        if gt:
            # Clear and rebuild
            gt.set_editor_property("grass_varieties", unreal.Array(unreal.GrassVariety))
    else:
        gt = asset_tools.create_asset(gt_name, GT_DEST,
            unreal.LandscapeGrassType, unreal.LandscapeGrassTypeFactory())

    if not gt:
        unreal.log_warning(f"  {gt_name}: failed to create")
        continue

    varieties = gt.get_editor_property("grass_varieties")
    added = 0

    for mesh_name, density, cull_start, cull_end, s_min, s_max in scatter_list:
        mesh_path = f"{MESH_DEST}/{mesh_name}"
        if not eal.does_asset_exist(mesh_path):
            continue

        mesh = unreal.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            continue

        v = unreal.GrassVariety()
        v.set_editor_property("grass_mesh", mesh)
        v.set_editor_property("grass_density", unreal.PerPlatformFloat(default=density))
        v.set_editor_property("start_cull_distance", unreal.PerPlatformInt(default=int(cull_start)))
        v.set_editor_property("end_cull_distance", unreal.PerPlatformInt(default=int(cull_end)))
        v.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
        v.set_editor_property("scale_x", unreal.FloatInterval(min=s_min, max=s_max))
        v.set_editor_property("scale_y", unreal.FloatInterval(min=s_min, max=s_max))
        v.set_editor_property("scale_z", unreal.FloatInterval(min=s_min, max=s_max))
        v.set_editor_property("random_rotation", True)
        v.set_editor_property("align_to_surface", True)
        v.set_editor_property("use_landscape_lightmap", True)
        varieties.append(v)
        added += 1

    gt.set_editor_property("grass_varieties", varieties)
    eal.save_asset(gt_path)
    created_gt += 1
    unreal.log(f"  {gt_name}: {added} varieties")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE:")
unreal.log(f"  Meshes imported: {len(tasks)}")
unreal.log(f"  GrassTypes created: {created_gt} zones")
unreal.log(f"")
unreal.log(f"  Per-zone GrassType assets at: {GT_DEST}")
unreal.log(f"    GT_Grassland  — grass, flowers, clover, dandelions")
unreal.log(f"    GT_Forest     — ferns, mushrooms, leaves, moss, bark")
unreal.log(f"    GT_Desert     — dry grass, rocks, cactus, bones")
unreal.log(f"    GT_Snow       — snow clumps, ice crystals, frozen twigs")
unreal.log(f"    GT_Beach      — shells, coral, starfish, driftwood")
unreal.log(f"    GT_Volcanic   — obsidian, lava rocks, sulfur, ash")
unreal.log(f"    GT_Cursed     — dead grass, bones, skulls, candles")
unreal.log(f"    GT_Amatsu     — cherry petals, bamboo leaves")
unreal.log(f"    GT_Moscovia   — birch leaves, pine cones, berries")
unreal.log(f"    GT_Ruins      — stone fragments, pottery, dry grass")
unreal.log(f"    GT_Industrial — metal scraps, bolts, rust")
unreal.log(f"    GT_Cave       — crystals, wet rocks, stalagmites")
unreal.log(f"    GT_Dungeon    — rubble, chains, bones, dirt")
unreal.log(f"")
unreal.log(f"  To use: add Grass Output node in landscape material,")
unreal.log(f"  pick the zone's GT_* from the dropdown")
unreal.log(f"{'='*60}")
