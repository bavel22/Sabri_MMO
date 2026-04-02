# create_500_variants_v2.py
# Creates ~500 Material Instances — guaranteed equal coverage across ALL 15 biomes.
# ~30 per biome + 30 cross-biome = ~480 variants.

import unreal
import os
import datetime
import itertools

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_11"
MI_BASE = "/Game/SabriMMO/Materials/Environment/BiomeVariants"
TRACKING_PATH = "C:/Sabri_MMO/docsNew/05_Development/Biome_Material_Variants.md"

eal.make_directory(MI_BASE)

B = "/Game/SabriMMO/Textures/Environment/Biomes"
S = "/Game/SabriMMO/Textures/Environment/Ground_Seamless"
G = "/Game/SabriMMO/Textures/Environment/Ground"
SD = "/Game/SabriMMO/Textures/Environment/Sand"

# Each biome: ground_a options, ground_b options, dirt options, cliff options
BIOMES = {
    "grassland": {
        "a": [f"{B}/grassland/T_Grass_Lush_A", f"{B}/grassland/T_Grass_Lush_B", f"{B}/grassland/T_Grass_Lush_C",
              f"{S}/T_LushGreen_03_2k", f"{S}/T_LushGreen_02_2k"],
        "b": [f"{B}/grassland/T_Grass_Lush_B", f"{B}/grassland/T_Grass_Lush_C",
              f"{S}/T_LushGreen_02_2k", f"{S}/T_MossFloor_01_2k"],
        "d": [f"{B}/grassland/T_Grass_Dirt_A", f"{B}/grassland/T_Grass_Dirt_B", f"{S}/T_EarthPath_01_2k"],
        "c": [f"{B}/grassland/T_Grass_Cliff_A", f"{B}/grassland/T_Grass_Cliff_B", f"{S}/T_StoneCliff_02_2k"],
    },
    "forest": {
        "a": [f"{B}/forest/T_Forest_Floor_A", f"{B}/forest/T_Forest_Floor_B", f"{B}/forest/T_Forest_Floor_C"],
        "b": [f"{B}/forest/T_Forest_Floor_B", f"{B}/forest/T_Forest_Floor_C", f"{S}/T_MossFloor_01_2k"],
        "d": [f"{B}/forest/T_Forest_Dirt_A", f"{B}/forest/T_Forest_Dirt_B"],
        "c": [f"{B}/forest/T_Forest_Cliff_A", f"{B}/forest/T_Forest_Cliff_B"],
    },
    "desert": {
        "a": [f"{B}/desert/T_Desert_Sand_A", f"{B}/desert/T_Desert_Sand_B", f"{B}/desert/T_Desert_Sand_C",
              f"{SD}/T_Sand_Warm_01", f"{SD}/T_Sand_Warm_02"],
        "b": [f"{B}/desert/T_Desert_Sand_B", f"{B}/desert/T_Desert_Sand_C",
              f"{SD}/T_Sand_Cool_02", f"{SD}/T_Sand_Warm_03"],
        "d": [f"{B}/desert/T_Desert_Dirt_A", f"{B}/desert/T_Desert_Dirt_B", f"{SD}/T_DryEarth_01"],
        "c": [f"{B}/desert/T_Desert_Cliff_A", f"{B}/desert/T_Desert_Cliff_B",
              f"{SD}/T_Sandstone_Cliff_01", f"{SD}/T_Sandstone_Cliff_02"],
    },
    "snow": {
        "a": [f"{B}/snow/T_Snow_Ground_A", f"{B}/snow/T_Snow_Ground_B", f"{B}/snow/T_Snow_Ground_C"],
        "b": [f"{B}/snow/T_Snow_Ground_B", f"{B}/snow/T_Snow_Ground_C", f"{B}/snow/T_Snow_Ground_A"],
        "d": [f"{B}/snow/T_Snow_Dirt_A", f"{B}/snow/T_Snow_Dirt_B"],
        "c": [f"{B}/snow/T_Snow_Cliff_A", f"{B}/snow/T_Snow_Cliff_B"],
    },
    "volcanic": {
        "a": [f"{B}/volcanic/T_Volcanic_Ground_A", f"{B}/volcanic/T_Volcanic_Ground_B", f"{B}/volcanic/T_Volcanic_Ground_C"],
        "b": [f"{B}/volcanic/T_Volcanic_Ground_B", f"{B}/volcanic/T_Volcanic_Ground_C", f"{B}/volcanic/T_Volcanic_Ground_A"],
        "d": [f"{B}/volcanic/T_Volcanic_Dirt_A"],
        "c": [f"{B}/volcanic/T_Volcanic_Cliff_A", f"{B}/volcanic/T_Volcanic_Cliff_B"],
    },
    "swamp": {
        "a": [f"{B}/swamp/T_Swamp_Ground_A", f"{B}/swamp/T_Swamp_Ground_B", f"{B}/swamp/T_Swamp_Ground_C"],
        "b": [f"{B}/swamp/T_Swamp_Ground_B", f"{B}/swamp/T_Swamp_Ground_C", f"{B}/swamp/T_Swamp_Ground_A"],
        "d": [f"{B}/swamp/T_Swamp_Dirt_A"],
        "c": [f"{B}/swamp/T_Swamp_Cliff_A", f"{B}/swamp/T_Swamp_Cliff_B"],
    },
    "beach": {
        "a": [f"{B}/beach/T_Beach_Sand_A", f"{B}/beach/T_Beach_Sand_B", f"{B}/beach/T_Beach_Sand_C"],
        "b": [f"{B}/beach/T_Beach_Sand_B", f"{B}/beach/T_Beach_Sand_C", f"{B}/beach/T_Beach_Sand_A"],
        "d": [f"{B}/beach/T_Beach_Dirt_A"],
        "c": [f"{B}/beach/T_Beach_Cliff_A", f"{B}/beach/T_Beach_Cliff_B"],
    },
    "mountain": {
        "a": [f"{B}/mountain/T_Mountain_Ground_A", f"{B}/mountain/T_Mountain_Ground_B", f"{B}/mountain/T_Mountain_Ground_C"],
        "b": [f"{B}/mountain/T_Mountain_Ground_B", f"{B}/mountain/T_Mountain_Ground_C", f"{B}/mountain/T_Mountain_Ground_A"],
        "d": [f"{B}/mountain/T_Mountain_Dirt_A"],
        "c": [f"{B}/mountain/T_Mountain_Cliff_A", f"{B}/mountain/T_Mountain_Cliff_B"],
    },
    "dungeon": {
        "a": [f"{B}/dungeon/T_Dungeon_Floor_A", f"{B}/dungeon/T_Dungeon_Floor_B", f"{B}/dungeon/T_Dungeon_Floor_C"],
        "b": [f"{B}/dungeon/T_Dungeon_Floor_B", f"{B}/dungeon/T_Dungeon_Floor_C", f"{B}/dungeon/T_Dungeon_Floor_A"],
        "d": [f"{B}/dungeon/T_Dungeon_Dirt_A"],
        "c": [f"{B}/dungeon/T_Dungeon_Wall_A", f"{B}/dungeon/T_Dungeon_Wall_B"],
    },
    "cave": {
        "a": [f"{B}/cave/T_Cave_Floor_A", f"{B}/cave/T_Cave_Floor_B", f"{B}/cave/T_Cave_Floor_C"],
        "b": [f"{B}/cave/T_Cave_Floor_B", f"{B}/cave/T_Cave_Floor_C", f"{B}/cave/T_Cave_Floor_A"],
        "d": [f"{B}/cave/T_Cave_Dirt_A"],
        "c": [f"{B}/cave/T_Cave_Wall_A", f"{B}/cave/T_Cave_Wall_B"],
    },
    "jungle": {
        "a": [f"{B}/jungle/T_Jungle_Floor_A", f"{B}/jungle/T_Jungle_Floor_B", f"{B}/jungle/T_Jungle_Floor_C"],
        "b": [f"{B}/jungle/T_Jungle_Floor_B", f"{B}/jungle/T_Jungle_Floor_C", f"{B}/jungle/T_Jungle_Floor_A"],
        "d": [f"{B}/jungle/T_Jungle_Dirt_A"],
        "c": [f"{B}/jungle/T_Jungle_Cliff_A", f"{B}/jungle/T_Jungle_Cliff_B"],
    },
    "urban": {
        "a": [f"{B}/urban/T_Urban_Cobble_A", f"{B}/urban/T_Urban_Cobble_B", f"{B}/urban/T_Urban_Cobble_C"],
        "b": [f"{B}/urban/T_Urban_Cobble_B", f"{B}/urban/T_Urban_Cobble_C", f"{B}/urban/T_Urban_Cobble_A"],
        "d": [f"{B}/urban/T_Urban_Dirt_A"],
        "c": [f"{B}/urban/T_Urban_Wall_A", f"{B}/urban/T_Urban_Wall_B"],
    },
    "cursed": {
        "a": [f"{B}/cursed/T_Cursed_Ground_A", f"{B}/cursed/T_Cursed_Ground_B", f"{B}/cursed/T_Cursed_Ground_C"],
        "b": [f"{B}/cursed/T_Cursed_Ground_B", f"{B}/cursed/T_Cursed_Ground_C", f"{B}/cursed/T_Cursed_Ground_A"],
        "d": [f"{B}/cursed/T_Cursed_Dirt_A"],
        "c": [f"{B}/cursed/T_Cursed_Cliff_A", f"{B}/cursed/T_Cursed_Cliff_B"],
    },
    "ruins": {
        "a": [f"{B}/ruins/T_Ruins_Floor_A", f"{B}/ruins/T_Ruins_Floor_B", f"{B}/ruins/T_Ruins_Floor_C"],
        "b": [f"{B}/ruins/T_Ruins_Floor_B", f"{B}/ruins/T_Ruins_Floor_C", f"{B}/ruins/T_Ruins_Floor_A"],
        "d": [f"{B}/ruins/T_Ruins_Dirt_A"],
        "c": [f"{B}/ruins/T_Ruins_Wall_A", f"{B}/ruins/T_Ruins_Wall_B"],
    },
    "industrial": {
        "a": [f"{B}/industrial/T_Industrial_Floor_A", f"{B}/industrial/T_Industrial_Floor_B", f"{B}/industrial/T_Industrial_Floor_C"],
        "b": [f"{B}/industrial/T_Industrial_Floor_B", f"{B}/industrial/T_Industrial_Floor_C", f"{B}/industrial/T_Industrial_Floor_A"],
        "d": [f"{B}/industrial/T_Industrial_Dirt_A"],
        "c": [f"{B}/industrial/T_Industrial_Wall_A", f"{B}/industrial/T_Industrial_Wall_B"],
    },
}

DIRT_PRESETS = [0.0, 0.15, 0.30]
MAX_PER_BIOME = 30

# Per-biome parameter presets — each biome has characteristic settings
BIOME_PARAMS = {
    "grassland":  {"Roughness": [0.90, 0.95], "MacroDarken": [0.85, 0.90], "MacroBrighten": [1.10, 1.15],
                   "SlopeThreshold": [0.55, 0.65], "SlopeTransitionWidth": [0.12, 0.18],
                   "UVDistortStrength": [40, 80], "NormalStrength": [0.3, 0.6], "AOStrength": [0.2, 0.5],
                   "GrassWarmTileSize": [3000, 5000], "GrassCoolTileSize": [4000, 6000],
                   "DirtTileSize": [2500, 4000], "RockTileSize": [2000, 3500]},
    "forest":     {"Roughness": [0.92, 0.98], "MacroDarken": [0.80, 0.88], "MacroBrighten": [1.05, 1.12],
                   "SlopeThreshold": [0.55, 0.65], "SlopeTransitionWidth": [0.10, 0.20],
                   "UVDistortStrength": [50, 90], "NormalStrength": [0.4, 0.7], "AOStrength": [0.3, 0.6],
                   "GrassWarmTileSize": [2500, 4500], "GrassCoolTileSize": [3500, 5500],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1800, 3000]},
    "desert":     {"Roughness": [0.88, 0.95], "MacroDarken": [0.82, 0.92], "MacroBrighten": [1.08, 1.18],
                   "SlopeThreshold": [0.50, 0.65], "SlopeTransitionWidth": [0.10, 0.20],
                   "UVDistortStrength": [30, 70], "NormalStrength": [0.2, 0.5], "AOStrength": [0.1, 0.4],
                   "GrassWarmTileSize": [3500, 6000], "GrassCoolTileSize": [4500, 7000],
                   "DirtTileSize": [3000, 5000], "RockTileSize": [2000, 4000]},
    "snow":       {"Roughness": [0.85, 0.95], "MacroDarken": [0.88, 0.95], "MacroBrighten": [1.05, 1.12],
                   "SlopeThreshold": [0.55, 0.70], "SlopeTransitionWidth": [0.10, 0.20],
                   "UVDistortStrength": [30, 60], "NormalStrength": [0.2, 0.4], "AOStrength": [0.1, 0.3],
                   "GrassWarmTileSize": [3500, 5500], "GrassCoolTileSize": [4500, 6500],
                   "DirtTileSize": [3000, 4500], "RockTileSize": [2000, 3500]},
    "volcanic":   {"Roughness": [0.80, 0.92], "MacroDarken": [0.75, 0.85], "MacroBrighten": [1.10, 1.25],
                   "SlopeThreshold": [0.45, 0.60], "SlopeTransitionWidth": [0.08, 0.15],
                   "UVDistortStrength": [50, 100], "NormalStrength": [0.5, 0.9], "AOStrength": [0.4, 0.7],
                   "GrassWarmTileSize": [2500, 4000], "GrassCoolTileSize": [3500, 5000],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1500, 3000]},
    "swamp":      {"Roughness": [0.90, 0.98], "MacroDarken": [0.78, 0.88], "MacroBrighten": [1.05, 1.12],
                   "SlopeThreshold": [0.55, 0.70], "SlopeTransitionWidth": [0.12, 0.22],
                   "UVDistortStrength": [60, 100], "NormalStrength": [0.3, 0.6], "AOStrength": [0.3, 0.6],
                   "GrassWarmTileSize": [2500, 4000], "GrassCoolTileSize": [3500, 5500],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1800, 3000]},
    "beach":      {"Roughness": [0.85, 0.95], "MacroDarken": [0.88, 0.95], "MacroBrighten": [1.05, 1.15],
                   "SlopeThreshold": [0.55, 0.70], "SlopeTransitionWidth": [0.12, 0.22],
                   "UVDistortStrength": [25, 55], "NormalStrength": [0.2, 0.4], "AOStrength": [0.1, 0.3],
                   "GrassWarmTileSize": [4000, 6000], "GrassCoolTileSize": [5000, 7000],
                   "DirtTileSize": [3500, 5500], "RockTileSize": [2500, 4000]},
    "mountain":   {"Roughness": [0.88, 0.95], "MacroDarken": [0.82, 0.90], "MacroBrighten": [1.08, 1.18],
                   "SlopeThreshold": [0.50, 0.62], "SlopeTransitionWidth": [0.10, 0.18],
                   "UVDistortStrength": [40, 80], "NormalStrength": [0.5, 0.8], "AOStrength": [0.3, 0.6],
                   "GrassWarmTileSize": [2500, 4500], "GrassCoolTileSize": [3500, 5500],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1500, 3000]},
    "dungeon":    {"Roughness": [0.75, 0.90], "MacroDarken": [0.80, 0.90], "MacroBrighten": [1.05, 1.15],
                   "SlopeThreshold": [0.50, 0.65], "SlopeTransitionWidth": [0.08, 0.15],
                   "UVDistortStrength": [30, 60], "NormalStrength": [0.4, 0.7], "AOStrength": [0.4, 0.7],
                   "GrassWarmTileSize": [2000, 3500], "GrassCoolTileSize": [3000, 4500],
                   "DirtTileSize": [1800, 3000], "RockTileSize": [1500, 2500]},
    "cave":       {"Roughness": [0.78, 0.92], "MacroDarken": [0.75, 0.88], "MacroBrighten": [1.05, 1.15],
                   "SlopeThreshold": [0.48, 0.62], "SlopeTransitionWidth": [0.08, 0.16],
                   "UVDistortStrength": [40, 80], "NormalStrength": [0.5, 0.8], "AOStrength": [0.4, 0.7],
                   "GrassWarmTileSize": [2000, 3500], "GrassCoolTileSize": [3000, 4500],
                   "DirtTileSize": [1800, 3000], "RockTileSize": [1500, 2800]},
    "jungle":     {"Roughness": [0.88, 0.96], "MacroDarken": [0.78, 0.88], "MacroBrighten": [1.08, 1.18],
                   "SlopeThreshold": [0.55, 0.68], "SlopeTransitionWidth": [0.12, 0.22],
                   "UVDistortStrength": [50, 90], "NormalStrength": [0.4, 0.7], "AOStrength": [0.3, 0.6],
                   "GrassWarmTileSize": [2500, 4500], "GrassCoolTileSize": [3500, 5500],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1800, 3200]},
    "urban":      {"Roughness": [0.70, 0.88], "MacroDarken": [0.85, 0.95], "MacroBrighten": [1.05, 1.12],
                   "SlopeThreshold": [0.55, 0.70], "SlopeTransitionWidth": [0.08, 0.15],
                   "UVDistortStrength": [20, 50], "NormalStrength": [0.3, 0.6], "AOStrength": [0.3, 0.5],
                   "GrassWarmTileSize": [2000, 3500], "GrassCoolTileSize": [3000, 4500],
                   "DirtTileSize": [1800, 3000], "RockTileSize": [1500, 2800]},
    "cursed":     {"Roughness": [0.82, 0.95], "MacroDarken": [0.70, 0.82], "MacroBrighten": [1.05, 1.18],
                   "SlopeThreshold": [0.48, 0.62], "SlopeTransitionWidth": [0.08, 0.18],
                   "UVDistortStrength": [60, 100], "NormalStrength": [0.5, 0.8], "AOStrength": [0.4, 0.7],
                   "GrassWarmTileSize": [2500, 4000], "GrassCoolTileSize": [3500, 5000],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1500, 3000]},
    "ruins":      {"Roughness": [0.80, 0.92], "MacroDarken": [0.82, 0.92], "MacroBrighten": [1.05, 1.15],
                   "SlopeThreshold": [0.52, 0.65], "SlopeTransitionWidth": [0.10, 0.18],
                   "UVDistortStrength": [30, 65], "NormalStrength": [0.4, 0.7], "AOStrength": [0.3, 0.6],
                   "GrassWarmTileSize": [2500, 4000], "GrassCoolTileSize": [3500, 5000],
                   "DirtTileSize": [2000, 3500], "RockTileSize": [1800, 3200]},
    "industrial": {"Roughness": [0.65, 0.85], "MacroDarken": [0.78, 0.88], "MacroBrighten": [1.08, 1.20],
                   "SlopeThreshold": [0.50, 0.65], "SlopeTransitionWidth": [0.08, 0.15],
                   "UVDistortStrength": [25, 55], "NormalStrength": [0.4, 0.7], "AOStrength": [0.3, 0.6],
                   "GrassWarmTileSize": [2000, 3500], "GrassCoolTileSize": [3000, 4500],
                   "DirtTileSize": [1800, 3000], "RockTileSize": [1500, 2800]},
}

# Cross-biome pairs for transition zones
CROSS_PAIRS = [
    ("grassland", "desert"), ("grassland", "forest"), ("grassland", "mountain"),
    ("grassland", "snow"), ("grassland", "beach"),
    ("desert", "volcanic"), ("desert", "ruins"), ("desert", "beach"),
    ("forest", "swamp"), ("forest", "jungle"), ("forest", "mountain"),
    ("snow", "mountain"), ("snow", "cave"),
    ("beach", "jungle"), ("beach", "urban"),
    ("cursed", "forest"), ("cursed", "dungeon"), ("cursed", "cave"),
    ("ruins", "desert"), ("ruins", "jungle"), ("ruins", "urban"),
    ("urban", "grassland"), ("urban", "industrial"),
    ("cave", "dungeon"), ("cave", "volcanic"),
    ("industrial", "urban"), ("industrial", "volcanic"),
    ("volcanic", "cursed"), ("volcanic", "cave"),
    ("jungle", "swamp"), ("swamp", "cursed"),
]


def lerp_val(lo, hi, t):
    """Linearly interpolate between lo and hi by t (0-1)."""
    return lo + (hi - lo) * t

def generate_biome_variants(biome_name, textures):
    """Generate up to MAX_PER_BIOME variants for a single biome, each with unique params."""
    variants = []
    all_combos = []
    bp = BIOME_PARAMS.get(biome_name, BIOME_PARAMS["grassland"])

    for a in textures["a"]:
        for b in textures["b"]:
            if a == b:
                continue
            for c in textures["c"]:
                for dirt in DIRT_PRESETS:
                    d = textures["d"][len(all_combos) % len(textures["d"])]
                    all_combos.append((a, b, d, c, dirt))

    # Evenly sample if too many combos
    if len(all_combos) > MAX_PER_BIOME:
        step = len(all_combos) / MAX_PER_BIOME
        selected = [all_combos[int(i * step)] for i in range(MAX_PER_BIOME)]
    else:
        selected = all_combos

    for idx, (a, b, d, c, dirt) in enumerate(selected):
        # Spread parameter values evenly across variants (0 to 1 fraction)
        t = idx / max(len(selected) - 1, 1)

        params = {"DirtAmount": dirt}
        for param_name, (lo, hi) in bp.items():
            params[param_name] = round(lerp_val(lo, hi, t), 3)

        a_short = a.split("/")[-1][:15]
        c_short = c.split("/")[-1][:15]
        name = f"{biome_name}_{idx+1:02d}_{a_short}_{c_short}_d{int(dirt*100):02d}"
        desc = f"{a.split('/')[-1]} + {b.split('/')[-1]} | cliff: {c.split('/')[-1]} | dirt: {dirt} | rough: {params.get('Roughness', 0.95)}"
        variants.append({"name": name, "biome": biome_name, "a": a, "b": b, "d": d, "c": c,
                         "dirt": dirt, "desc": desc, "params": params})

    return variants


def generate_cross_variants():
    """Generate 15 variants per cross-biome pair."""
    variants = []
    for biome_a, biome_b in CROSS_PAIRS:
        ta = BIOMES[biome_a]
        tb = BIOMES[biome_b]
        idx = 0

        # Mix ground_a from biome_a with ground_b from biome_b, and vice versa
        # Also vary cliff source (a or b) and dirt levels
        combos = []
        for a in ta["a"]:
            for b in tb["a"]:
                for use_cliff_a in [True, False]:
                    for dirt in DIRT_PRESETS:
                        c = ta["c"][len(combos) % len(ta["c"])] if use_cliff_a else tb["c"][len(combos) % len(tb["c"])]
                        d = tb["d"][len(combos) % len(tb["d"])] if use_cliff_a else ta["d"][len(combos) % len(ta["d"])]
                        combos.append((a, b, d, c, dirt, use_cliff_a))

        # Sample 15 evenly
        target = 15
        if len(combos) > target:
            step = len(combos) / target
            selected = [combos[int(i * step)] for i in range(target)]
        else:
            selected = combos[:target]

        for a, b, d, c, dirt, cliff_from_a in selected:
            idx += 1
            cliff_src = biome_a if cliff_from_a else biome_b
            name = f"cross_{biome_a}_{biome_b}_{idx:02d}_d{int(dirt*100):02d}"

            # Blend parameters from both biomes
            bp_a = BIOME_PARAMS.get(biome_a, BIOME_PARAMS["grassland"])
            bp_b = BIOME_PARAMS.get(biome_b, BIOME_PARAMS["grassland"])
            t = idx / max(target - 1, 1)
            params = {"DirtAmount": dirt}
            for param_name in bp_a:
                lo_a, hi_a = bp_a[param_name]
                lo_b, hi_b = bp_b[param_name]
                # Average the two biomes' ranges, then interpolate by t
                lo = (lo_a + lo_b) / 2
                hi = (hi_a + hi_b) / 2
                params[param_name] = round(lerp_val(lo, hi, t), 3)

            desc = f"{a.split('/')[-1]} + {b.split('/')[-1]} | cliff: {cliff_src} {c.split('/')[-1]} | dirt: {dirt}"
            variants.append({"name": name, "biome": f"{biome_a}_{biome_b}", "a": a, "b": b, "d": d, "c": c, "dirt": dirt, "desc": desc, "params": params})

    return variants


# ============================================================
# MAIN
# ============================================================

parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent not found: {PARENT_PATH}")
    raise RuntimeError("Missing parent")

# Generate all variant definitions
all_variants = []
for biome_name, textures in BIOMES.items():
    bv = generate_biome_variants(biome_name, textures)
    all_variants.extend(bv)
    unreal.log(f"  {biome_name}: {len(bv)} variants planned")

cross = generate_cross_variants()
all_variants.extend(cross)
unreal.log(f"  cross-biome: {len(cross)} variants planned")
unreal.log(f"\nTotal: {len(all_variants)} variants to create\n")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()

created = 0
skipped = 0
tracking_lines = []

for i, v in enumerate(all_variants):
    mi_name = f"MI_{v['name']}"
    biome_folder = v["biome"]
    mi_path = f"{MI_BASE}/{biome_folder}"
    mi_full = f"{mi_path}/{mi_name}"

    # Append _v2 to avoid collision with old variants
    mi_name = f"MI_{v['name']}_v2"
    mi_full = f"{mi_path}/{mi_name}"
    if eal.does_asset_exist(mi_full):
        skipped += 1
        continue

    eal.make_directory(mi_path)
    mi = asset_tools.create_asset(mi_name, mi_path,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        continue

    mi.set_editor_property("parent", parent)

    for param, path in [("GrassWarmTexture", v["a"]), ("GrassCoolTexture", v["b"]),
                         ("DirtTexture", v["d"]), ("RockTexture", v["c"])]:
        if eal.does_asset_exist(path):
            tex = unreal.load_asset(path)
            if tex:
                mel.set_material_instance_texture_parameter_value(
                    mi, param, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Set ALL scalar parameters with variation
    params = v.get("params", {})
    for param_name, value in params.items():
        mel.set_material_instance_scalar_parameter_value(
            mi, param_name, value, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1
    tracking_lines.append(f"| {v['name']} | {v['biome']} | {v['desc']} | {v['dirt']} |")

    if (i + 1) % 50 == 0:
        unreal.log(f"  [{i+1}/{len(all_variants)}] {created} created...")

# Write tracking doc
with open(TRACKING_PATH, "w") as f:
    f.write(f"# Biome Material Variants\n\n")
    f.write(f"Generated: {datetime.date.today()}\n")
    f.write(f"Parent: M_Landscape_RO_09\n")
    f.write(f"Total: {created} created, {skipped} skipped\n\n")
    f.write(f"## Per-Biome Count\n\n")
    biome_counts = {}
    for v in all_variants:
        biome_counts[v["biome"]] = biome_counts.get(v["biome"], 0) + 1
    for b, c in sorted(biome_counts.items()):
        f.write(f"- **{b}**: {c} variants\n")
    f.write(f"\n## All Variants\n\n")
    f.write(f"| Name | Biome | Description | Dirt |\n")
    f.write(f"|------|-------|-------------|------|\n")
    for line in tracking_lines:
        f.write(line + "\n")

unreal.log(f"\n{'=' * 60}")
unreal.log(f"  DONE: {created} created, {skipped} skipped")
unreal.log(f"  Browse: {MI_BASE}/")
unreal.log(f"  15 biome folders + cross-biome folders")
unreal.log(f"{'=' * 60}")
