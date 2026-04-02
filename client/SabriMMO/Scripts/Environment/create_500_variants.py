# create_500_variants.py
# Creates 500 Material Instances from M_Landscape_RO_09.
# Each variant uses different biome texture combinations and parameter settings.
#
# Run in UE5: exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_500_variants.py").read())

import unreal
import os
import datetime
import random

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_09"
MI_BASE = "/Game/SabriMMO/Materials/Environment/BiomeVariants"
TRACKING_PATH = "C:/Sabri_MMO/docsNew/05_Development/Biome_Material_Variants.md"

eal.make_directory(MI_BASE)

# ============================================================
# ALL TEXTURE PATHS BY BIOME AND ROLE
# ============================================================

# Format: biome -> role -> [list of texture paths]
# Roles: ground_a (warm/primary), ground_b (cool/variant), dirt, cliff

B = "/Game/SabriMMO/Textures/Environment/Biomes"
S = "/Game/SabriMMO/Textures/Environment/Ground_Seamless"
G = "/Game/SabriMMO/Textures/Environment/Ground"
SD = "/Game/SabriMMO/Textures/Environment/Sand"

BIOME_TEXTURES = {
    "grassland": {
        "ground_a": [f"{B}/grassland/T_Grass_Lush_A", f"{B}/grassland/T_Grass_Lush_B", f"{B}/grassland/T_Grass_Lush_C",
                     f"{S}/T_LushGreen_03_2k", f"{S}/T_LushGreen_02_2k", f"{S}/T_Ground_RO_DenseGrass_A_2k"],
        "ground_b": [f"{B}/grassland/T_Grass_Lush_B", f"{B}/grassland/T_Grass_Lush_C", f"{B}/grassland/T_Grass_Lush_A",
                     f"{S}/T_LushGreen_02_2k", f"{S}/T_LushGreen_01_2k", f"{S}/T_MossFloor_01_2k"],
        "dirt":     [f"{B}/grassland/T_Grass_Dirt_A", f"{B}/grassland/T_Grass_Dirt_B", f"{S}/T_EarthPath_01_2k"],
        "cliff":    [f"{B}/grassland/T_Grass_Cliff_A", f"{B}/grassland/T_Grass_Cliff_B", f"{S}/T_StoneCliff_02_2k"],
    },
    "forest": {
        "ground_a": [f"{B}/forest/T_Forest_Floor_A", f"{B}/forest/T_Forest_Floor_B", f"{B}/forest/T_Forest_Floor_C"],
        "ground_b": [f"{B}/forest/T_Forest_Floor_B", f"{B}/forest/T_Forest_Floor_C", f"{B}/forest/T_Forest_Floor_A",
                     f"{S}/T_MossFloor_01_2k"],
        "dirt":     [f"{B}/forest/T_Forest_Dirt_A", f"{B}/forest/T_Forest_Dirt_B"],
        "cliff":    [f"{B}/forest/T_Forest_Cliff_A", f"{B}/forest/T_Forest_Cliff_B"],
    },
    "desert": {
        "ground_a": [f"{B}/desert/T_Desert_Sand_A", f"{B}/desert/T_Desert_Sand_B", f"{B}/desert/T_Desert_Sand_C",
                     f"{SD}/T_Sand_Warm_01", f"{SD}/T_Sand_Warm_02", f"{SD}/T_Sand_Warm_03"],
        "ground_b": [f"{B}/desert/T_Desert_Sand_B", f"{B}/desert/T_Desert_Sand_C", f"{B}/desert/T_Desert_Sand_A",
                     f"{SD}/T_Sand_Cool_01", f"{SD}/T_Sand_Cool_02", f"{SD}/T_Sand_Warm_01"],
        "dirt":     [f"{B}/desert/T_Desert_Dirt_A", f"{B}/desert/T_Desert_Dirt_B", f"{SD}/T_DryEarth_01", f"{SD}/T_DryEarth_02"],
        "cliff":    [f"{B}/desert/T_Desert_Cliff_A", f"{B}/desert/T_Desert_Cliff_B",
                     f"{SD}/T_Sandstone_Cliff_01", f"{SD}/T_Sandstone_Cliff_02", f"{SD}/T_Sandstone_Cliff_03"],
    },
    "snow": {
        "ground_a": [f"{B}/snow/T_Snow_Ground_A", f"{B}/snow/T_Snow_Ground_B", f"{B}/snow/T_Snow_Ground_C"],
        "ground_b": [f"{B}/snow/T_Snow_Ground_B", f"{B}/snow/T_Snow_Ground_C", f"{B}/snow/T_Snow_Ground_A"],
        "dirt":     [f"{B}/snow/T_Snow_Dirt_A", f"{B}/snow/T_Snow_Dirt_B"],
        "cliff":    [f"{B}/snow/T_Snow_Cliff_A", f"{B}/snow/T_Snow_Cliff_B"],
    },
    "volcanic": {
        "ground_a": [f"{B}/volcanic/T_Volcanic_Ground_A", f"{B}/volcanic/T_Volcanic_Ground_B", f"{B}/volcanic/T_Volcanic_Ground_C"],
        "ground_b": [f"{B}/volcanic/T_Volcanic_Ground_B", f"{B}/volcanic/T_Volcanic_Ground_C", f"{B}/volcanic/T_Volcanic_Ground_A"],
        "dirt":     [f"{B}/volcanic/T_Volcanic_Dirt_A"],
        "cliff":    [f"{B}/volcanic/T_Volcanic_Cliff_A", f"{B}/volcanic/T_Volcanic_Cliff_B"],
    },
    "swamp": {
        "ground_a": [f"{B}/swamp/T_Swamp_Ground_A", f"{B}/swamp/T_Swamp_Ground_B", f"{B}/swamp/T_Swamp_Ground_C"],
        "ground_b": [f"{B}/swamp/T_Swamp_Ground_B", f"{B}/swamp/T_Swamp_Ground_C", f"{B}/swamp/T_Swamp_Ground_A"],
        "dirt":     [f"{B}/swamp/T_Swamp_Dirt_A"],
        "cliff":    [f"{B}/swamp/T_Swamp_Cliff_A", f"{B}/swamp/T_Swamp_Cliff_B"],
    },
    "beach": {
        "ground_a": [f"{B}/beach/T_Beach_Sand_A", f"{B}/beach/T_Beach_Sand_B", f"{B}/beach/T_Beach_Sand_C"],
        "ground_b": [f"{B}/beach/T_Beach_Sand_B", f"{B}/beach/T_Beach_Sand_C", f"{B}/beach/T_Beach_Sand_A"],
        "dirt":     [f"{B}/beach/T_Beach_Dirt_A"],
        "cliff":    [f"{B}/beach/T_Beach_Cliff_A", f"{B}/beach/T_Beach_Cliff_B"],
    },
    "mountain": {
        "ground_a": [f"{B}/mountain/T_Mountain_Ground_A", f"{B}/mountain/T_Mountain_Ground_B", f"{B}/mountain/T_Mountain_Ground_C"],
        "ground_b": [f"{B}/mountain/T_Mountain_Ground_B", f"{B}/mountain/T_Mountain_Ground_C", f"{B}/mountain/T_Mountain_Ground_A"],
        "dirt":     [f"{B}/mountain/T_Mountain_Dirt_A"],
        "cliff":    [f"{B}/mountain/T_Mountain_Cliff_A", f"{B}/mountain/T_Mountain_Cliff_B"],
    },
    "dungeon": {
        "ground_a": [f"{B}/dungeon/T_Dungeon_Floor_A", f"{B}/dungeon/T_Dungeon_Floor_B", f"{B}/dungeon/T_Dungeon_Floor_C"],
        "ground_b": [f"{B}/dungeon/T_Dungeon_Floor_B", f"{B}/dungeon/T_Dungeon_Floor_C", f"{B}/dungeon/T_Dungeon_Floor_A"],
        "dirt":     [f"{B}/dungeon/T_Dungeon_Dirt_A"],
        "cliff":    [f"{B}/dungeon/T_Dungeon_Wall_A", f"{B}/dungeon/T_Dungeon_Wall_B"],
    },
    "cave": {
        "ground_a": [f"{B}/cave/T_Cave_Floor_A", f"{B}/cave/T_Cave_Floor_B", f"{B}/cave/T_Cave_Floor_C"],
        "ground_b": [f"{B}/cave/T_Cave_Floor_B", f"{B}/cave/T_Cave_Floor_C", f"{B}/cave/T_Cave_Floor_A"],
        "dirt":     [f"{B}/cave/T_Cave_Dirt_A"],
        "cliff":    [f"{B}/cave/T_Cave_Wall_A", f"{B}/cave/T_Cave_Wall_B"],
    },
    "jungle": {
        "ground_a": [f"{B}/jungle/T_Jungle_Floor_A", f"{B}/jungle/T_Jungle_Floor_B", f"{B}/jungle/T_Jungle_Floor_C"],
        "ground_b": [f"{B}/jungle/T_Jungle_Floor_B", f"{B}/jungle/T_Jungle_Floor_C", f"{B}/jungle/T_Jungle_Floor_A"],
        "dirt":     [f"{B}/jungle/T_Jungle_Dirt_A"],
        "cliff":    [f"{B}/jungle/T_Jungle_Cliff_A", f"{B}/jungle/T_Jungle_Cliff_B"],
    },
    "urban": {
        "ground_a": [f"{B}/urban/T_Urban_Cobble_A", f"{B}/urban/T_Urban_Cobble_B", f"{B}/urban/T_Urban_Cobble_C"],
        "ground_b": [f"{B}/urban/T_Urban_Cobble_B", f"{B}/urban/T_Urban_Cobble_C", f"{B}/urban/T_Urban_Cobble_A"],
        "dirt":     [f"{B}/urban/T_Urban_Dirt_A"],
        "cliff":    [f"{B}/urban/T_Urban_Wall_A", f"{B}/urban/T_Urban_Wall_B"],
    },
    "cursed": {
        "ground_a": [f"{B}/cursed/T_Cursed_Ground_A", f"{B}/cursed/T_Cursed_Ground_B", f"{B}/cursed/T_Cursed_Ground_C"],
        "ground_b": [f"{B}/cursed/T_Cursed_Ground_B", f"{B}/cursed/T_Cursed_Ground_C", f"{B}/cursed/T_Cursed_Ground_A"],
        "dirt":     [f"{B}/cursed/T_Cursed_Dirt_A"],
        "cliff":    [f"{B}/cursed/T_Cursed_Cliff_A", f"{B}/cursed/T_Cursed_Cliff_B"],
    },
    "ruins": {
        "ground_a": [f"{B}/ruins/T_Ruins_Floor_A", f"{B}/ruins/T_Ruins_Floor_B", f"{B}/ruins/T_Ruins_Floor_C"],
        "ground_b": [f"{B}/ruins/T_Ruins_Floor_B", f"{B}/ruins/T_Ruins_Floor_C", f"{B}/ruins/T_Ruins_Floor_A"],
        "dirt":     [f"{B}/ruins/T_Ruins_Dirt_A"],
        "cliff":    [f"{B}/ruins/T_Ruins_Wall_A", f"{B}/ruins/T_Ruins_Wall_B"],
    },
    "industrial": {
        "ground_a": [f"{B}/industrial/T_Industrial_Floor_A", f"{B}/industrial/T_Industrial_Floor_B", f"{B}/industrial/T_Industrial_Floor_C"],
        "ground_b": [f"{B}/industrial/T_Industrial_Floor_B", f"{B}/industrial/T_Industrial_Floor_C", f"{B}/industrial/T_Industrial_Floor_A"],
        "dirt":     [f"{B}/industrial/T_Industrial_Dirt_A"],
        "cliff":    [f"{B}/industrial/T_Industrial_Wall_A", f"{B}/industrial/T_Industrial_Wall_B"],
    },
}

# Dirt amount presets
DIRT_LEVELS = [0.0, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.40, 0.50]

# ============================================================
# Generate all variant definitions
# ============================================================

def generate_variants():
    """Generate 500 variant definitions programmatically."""
    variants = []
    variant_id = 0

    for biome_name, textures in BIOME_TEXTURES.items():
        ga_list = textures["ground_a"]
        gb_list = textures["ground_b"]
        d_list = textures["dirt"]
        c_list = textures["cliff"]

        # For each biome: generate all combos of ground_a x ground_b x cliff x dirt_level
        for ga_idx, ga in enumerate(ga_list):
            for gb_idx, gb in enumerate(gb_list):
                if ga == gb:
                    continue  # skip same texture for both slots
                for c_idx, cliff in enumerate(c_list):
                    for dirt_amt in [0.0, 0.15, 0.30]:
                        variant_id += 1
                        dirt_tex = d_list[variant_id % len(d_list)]

                        ga_short = ga.split("/")[-1]
                        gb_short = gb.split("/")[-1]
                        c_short = cliff.split("/")[-1]
                        d_short = f"d{int(dirt_amt*100):02d}"

                        name = f"{biome_name}_{variant_id:03d}_{ga_short[:12]}_{c_short[:12]}_{d_short}"
                        desc = f"{biome_name} | {ga_short} + {gb_short} | cliff: {c_short} | dirt: {dirt_amt}"

                        variants.append({
                            "name": name,
                            "desc": desc,
                            "biome": biome_name,
                            "ground_a": ga,
                            "ground_b": gb,
                            "dirt": dirt_tex,
                            "cliff": cliff,
                            "dirt_amt": dirt_amt,
                        })

    # Also add cross-biome combos (mix biomes for transition zones)
    cross_combos = [
        ("grassland", "desert"), ("grassland", "forest"), ("grassland", "mountain"),
        ("desert", "volcanic"), ("forest", "swamp"), ("forest", "jungle"),
        ("snow", "mountain"), ("beach", "jungle"), ("cursed", "forest"),
        ("ruins", "desert"), ("urban", "grassland"), ("cave", "dungeon"),
        ("industrial", "urban"), ("volcanic", "cursed"), ("beach", "desert"),
    ]
    for biome_a, biome_b in cross_combos:
        ta = BIOME_TEXTURES[biome_a]
        tb = BIOME_TEXTURES[biome_b]
        for dirt_amt in [0.10, 0.25]:
            variant_id += 1
            ga = ta["ground_a"][0]
            gb = tb["ground_a"][0]
            cliff = ta["cliff"][0]
            dirt_tex = tb["dirt"][0]

            name = f"cross_{variant_id:03d}_{biome_a}_{biome_b}_d{int(dirt_amt*100):02d}"
            desc = f"CROSS: {biome_a} ground + {biome_b} variant | cliff: {biome_a} | dirt: {dirt_amt}"

            variants.append({
                "name": name,
                "desc": desc,
                "biome": f"{biome_a}+{biome_b}",
                "ground_a": ga,
                "ground_b": gb,
                "dirt": dirt_tex,
                "cliff": cliff,
                "dirt_amt": dirt_amt,
            })

    return variants[:500]  # Cap at 500


# ============================================================
# MAIN
# ============================================================

parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent not found: {PARENT_PATH}")
    raise RuntimeError("Parent material missing")

variants = generate_variants()
unreal.log(f"Creating {len(variants)} material instance variants...")
unreal.log(f"Output: {MI_BASE}\n")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()

created = 0
skipped = 0
failed_tex = 0
tracking_lines = []

for i, v in enumerate(variants):
    mi_name = f"MI_{v['name']}"
    # Organize into subfolders by biome
    biome_folder = v['biome'].replace("+", "_")
    mi_path = f"{MI_BASE}/{biome_folder}"
    mi_full = f"{mi_path}/{mi_name}"

    if eal.does_asset_exist(mi_full):
        skipped += 1
        if i % 50 == 0:
            unreal.log(f"  [{i+1}/{len(variants)}] skipping existing...")
        continue

    eal.make_directory(mi_path)
    mi = asset_tools.create_asset(mi_name, mi_path,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        continue

    mi.set_editor_property("parent", parent)

    # Assign textures
    tex_map = {
        "GrassWarmTexture": v["ground_a"],
        "GrassCoolTexture": v["ground_b"],
        "DirtTexture": v["dirt"],
        "RockTexture": v["cliff"],
    }
    for param, path in tex_map.items():
        if eal.does_asset_exist(path):
            tex = unreal.load_asset(path)
            if tex:
                mel.set_material_instance_texture_parameter_value(
                    mi, param, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Set dirt amount
    mel.set_material_instance_scalar_parameter_value(
        mi, "DirtAmount", v["dirt_amt"], unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1

    tracking_lines.append(f"| {v['name']} | {v['biome']} | {v['desc']} | {v['dirt_amt']} |")

    if (i + 1) % 25 == 0:
        unreal.log(f"  [{i+1}/{len(variants)}] created {created} so far...")

# Write tracking doc
with open(TRACKING_PATH, "w") as f:
    f.write(f"# Biome Material Variants\n\n")
    f.write(f"Generated: {datetime.date.today()}\n")
    f.write(f"Parent: M_Landscape_RO_09\n")
    f.write(f"Total: {created} created, {skipped} skipped (already existed)\n\n")
    f.write(f"Browse in UE5: {MI_BASE}/\n\n")
    f.write(f"| Name | Biome | Description | Dirt |\n")
    f.write(f"|------|-------|-------------|------|\n")
    for line in tracking_lines:
        f.write(line + "\n")

unreal.log(f"\n{'=' * 60}")
unreal.log(f"  DONE: {created} created, {skipped} skipped")
unreal.log(f"  Browse: {MI_BASE}/")
unreal.log(f"  Organized by biome subfolders")
unreal.log(f"  Tracking: {TRACKING_PATH}")
unreal.log(f"{'=' * 60}")
