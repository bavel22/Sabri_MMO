# create_variants_v3.py
# COMPREHENSIVE variant generator using M_Landscape_RO_12 (23 parameters).
# Uses BOTH old generic biome textures AND new RO zone-specific textures.
# Every parameter varied per biome for truly unique zone identities.
#
# Run: exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_variants_v3.py").read())

import unreal
import os
import datetime
import hashlib

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_12"
MI_BASE = "/Game/SabriMMO/Materials/Environment/v3"
TRACKING_PATH = "C:/Sabri_MMO/docsNew/05_Development/Biome_Variants_v3.md"

eal.make_directory(MI_BASE)

# ============================================================
# TEXTURE PATHS — all sources
# ============================================================
B = "/Game/SabriMMO/Textures/Environment/Biomes"
S = "/Game/SabriMMO/Textures/Environment/Ground_Seamless"
G = "/Game/SabriMMO/Textures/Environment/Ground"
SD = "/Game/SabriMMO/Textures/Environment/Sand"
RC = "/Game/SabriMMO/Textures/Environment/ROClassic"
RZ = "/Game/SabriMMO/Textures/Environment/ROZones"

# ============================================================
# PER-ZONE DEFINITIONS — textures + full parameter presets
# Each zone has multiple parameter presets for variety
# ============================================================

def lerp(a, b, t):
    return a + (b - a) * t

def lerp_color(a, b, t):
    return tuple(lerp(a[i], b[i], t) for i in range(3))

ZONES = {
    # =====================
    # PRONTERA FIELDS
    # =====================
    "prontera": {
        "ground_a": [
            f"{RZ}/prontera_fields/T_Prt_Meadow_A", f"{RZ}/prontera_fields/T_Prt_Meadow_B",
            f"{RZ}/prontera_fields/T_Prt_Meadow_C", f"{RC}/T_RO_YellowGreen_A",
            f"{S}/T_LushGreen_03_2k", f"{S}/T_LushGreen_02_2k",
        ],
        "ground_b": [
            f"{RZ}/prontera_fields/T_Prt_Meadow_B", f"{RZ}/prontera_fields/T_Prt_Meadow_C",
            f"{RC}/T_RO_DarkGreen_A", f"{S}/T_LushGreen_02_2k", f"{S}/T_MossFloor_01_2k",
        ],
        "dirt": [f"{RZ}/prontera_fields/T_Prt_Path_A", f"{RZ}/prontera_fields/T_Prt_Path_B", f"{S}/T_EarthPath_01_2k"],
        "cliff": [f"{RZ}/prontera_fields/T_Prt_Cliff_A", f"{RZ}/prontera_fields/T_Prt_Cliff_B", f"{S}/T_StoneCliff_02_2k", f"{RC}/T_RO_DarkCliff_A"],
        "params_lo": {
            "WarmthTint": (1.01, 0.99, 0.93), "SaturationMult": 0.88, "BrightnessOffset": 0.0,
            "ContrastBoost": 0.95, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0005, "DirtAmount": 0.10, "DirtOnSlopes": 0.0,
            "SlopeNoiseAmount": 0.03, "SlopeNoiseFreq": 0.008, "Roughness": 0.90,
            "NormalStrength": 0.2, "AOStrength": 0.2, "UVDistortStrength": 40.0,
            "GrassWarmTileSize": 3500.0, "GrassCoolTileSize": 4500.0, "DirtTileSize": 3000.0, "RockTileSize": 2500.0,
        },
        "params_hi": {
            "WarmthTint": (1.04, 1.0, 0.95), "SaturationMult": 1.02, "BrightnessOffset": 0.08,
            "ContrastBoost": 1.05, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.006,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.30, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.10, "SlopeNoiseFreq": 0.015, "Roughness": 0.96,
            "NormalStrength": 0.6, "AOStrength": 0.5, "UVDistortStrength": 80.0,
            "GrassWarmTileSize": 5500.0, "GrassCoolTileSize": 6500.0, "DirtTileSize": 4500.0, "RockTileSize": 3500.0,
        },
    },
    # =====================
    # GEFFEN
    # =====================
    "geffen": {
        "ground_a": [f"{RZ}/geffen/T_Gef_Ground_A", f"{RZ}/geffen/T_Gef_Ground_B", f"{RZ}/geffen/T_Gef_Ground_C", f"{B}/forest/T_Forest_Floor_A"],
        "ground_b": [f"{RZ}/geffen/T_Gef_Ground_B", f"{RZ}/geffen/T_Gef_Ground_C", f"{B}/forest/T_Forest_Floor_C", f"{S}/T_MossFloor_01_2k"],
        "dirt": [f"{RZ}/geffen/T_Gef_Dirt_A", f"{B}/forest/T_Forest_Dirt_A"],
        "cliff": [f"{RZ}/geffen/T_Gef_Cliff_A", f"{RZ}/geffen/T_Gef_Cliff_B", f"{S}/T_StoneCliff_02_2k"],
        "params_lo": {
            "WarmthTint": (0.92, 0.94, 1.0), "SaturationMult": 0.82, "BrightnessOffset": -0.06,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0006, "DirtAmount": 0.10, "DirtOnSlopes": 0.0,
            "SlopeNoiseAmount": 0.04, "SlopeNoiseFreq": 0.008, "Roughness": 0.90,
            "NormalStrength": 0.3, "AOStrength": 0.3, "UVDistortStrength": 45.0,
            "GrassWarmTileSize": 3000.0, "GrassCoolTileSize": 4000.0, "DirtTileSize": 2800.0, "RockTileSize": 2200.0,
        },
        "params_hi": {
            "WarmthTint": (0.96, 0.97, 1.04), "SaturationMult": 0.95, "BrightnessOffset": 0.0,
            "ContrastBoost": 1.15, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.007,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.25, "DirtOnSlopes": 0.4,
            "SlopeNoiseAmount": 0.12, "SlopeNoiseFreq": 0.018, "Roughness": 0.96,
            "NormalStrength": 0.7, "AOStrength": 0.6, "UVDistortStrength": 85.0,
            "GrassWarmTileSize": 5000.0, "GrassCoolTileSize": 6000.0, "DirtTileSize": 4200.0, "RockTileSize": 3200.0,
        },
    },
    # =====================
    # PAYON FOREST
    # =====================
    "payon": {
        "ground_a": [f"{RZ}/payon_forest/T_Pay_Forest_A", f"{RZ}/payon_forest/T_Pay_Forest_B", f"{RZ}/payon_forest/T_Pay_Forest_C", f"{B}/forest/T_Forest_Floor_A", f"{B}/forest/T_Forest_Floor_B"],
        "ground_b": [f"{RZ}/payon_forest/T_Pay_Forest_B", f"{RZ}/payon_forest/T_Pay_Forest_C", f"{B}/forest/T_Forest_Floor_C", f"{S}/T_MossFloor_01_2k"],
        "dirt": [f"{RZ}/payon_forest/T_Pay_Dirt_A", f"{B}/forest/T_Forest_Dirt_A", f"{B}/forest/T_Forest_Dirt_B"],
        "cliff": [f"{RZ}/payon_forest/T_Pay_Cliff_A", f"{RZ}/payon_forest/T_Pay_Cliff_B", f"{B}/forest/T_Forest_Cliff_A"],
        "params_lo": {
            "WarmthTint": (1.0, 0.97, 0.92), "SaturationMult": 0.85, "BrightnessOffset": -0.08,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0008, "DirtAmount": 0.15, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.05, "SlopeNoiseFreq": 0.008, "Roughness": 0.92,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 50.0,
            "GrassWarmTileSize": 2800.0, "GrassCoolTileSize": 3800.0, "DirtTileSize": 2500.0, "RockTileSize": 2000.0,
        },
        "params_hi": {
            "WarmthTint": (1.04, 0.99, 0.94), "SaturationMult": 0.98, "BrightnessOffset": -0.02,
            "ContrastBoost": 1.15, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.008,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.35, "DirtOnSlopes": 0.6,
            "SlopeNoiseAmount": 0.12, "SlopeNoiseFreq": 0.018, "Roughness": 0.97,
            "NormalStrength": 0.7, "AOStrength": 0.6, "UVDistortStrength": 90.0,
            "GrassWarmTileSize": 4500.0, "GrassCoolTileSize": 5500.0, "DirtTileSize": 3800.0, "RockTileSize": 3000.0,
        },
    },
    # =====================
    # MORROC DESERT
    # =====================
    "morroc": {
        "ground_a": [f"{RZ}/morroc_desert/T_Moc_Sand_A", f"{RZ}/morroc_desert/T_Moc_Sand_B", f"{RZ}/morroc_desert/T_Moc_Sand_C", f"{SD}/T_Sand_Warm_01", f"{SD}/T_Sand_Warm_02", f"{B}/desert/T_Desert_Sand_A"],
        "ground_b": [f"{RZ}/morroc_desert/T_Moc_Sand_B", f"{RZ}/morroc_desert/T_Moc_Sand_C", f"{SD}/T_Sand_Cool_02", f"{B}/desert/T_Desert_Sand_B", f"{B}/desert/T_Desert_Sand_C"],
        "dirt": [f"{RZ}/morroc_desert/T_Moc_Cracked_A", f"{SD}/T_DryEarth_01", f"{B}/desert/T_Desert_Dirt_A"],
        "cliff": [f"{RZ}/morroc_desert/T_Moc_Sandstone_A", f"{RZ}/morroc_desert/T_Moc_Sandstone_B", f"{SD}/T_Sandstone_Cliff_01", f"{SD}/T_Sandstone_Cliff_02"],
        "params_lo": {
            "WarmthTint": (1.04, 0.97, 0.86), "SaturationMult": 0.82, "BrightnessOffset": 0.03,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0004, "DirtAmount": 0.05, "DirtOnSlopes": 0.0,
            "SlopeNoiseAmount": 0.02, "SlopeNoiseFreq": 0.006, "Roughness": 0.86,
            "NormalStrength": 0.2, "AOStrength": 0.1, "UVDistortStrength": 30.0,
            "GrassWarmTileSize": 3800.0, "GrassCoolTileSize": 5000.0, "DirtTileSize": 3200.0, "RockTileSize": 2500.0,
        },
        "params_hi": {
            "WarmthTint": (1.08, 1.0, 0.90), "SaturationMult": 0.98, "BrightnessOffset": 0.15,
            "ContrastBoost": 1.15, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.006,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.25, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.08, "SlopeNoiseFreq": 0.015, "Roughness": 0.95,
            "NormalStrength": 0.5, "AOStrength": 0.4, "UVDistortStrength": 70.0,
            "GrassWarmTileSize": 6000.0, "GrassCoolTileSize": 7500.0, "DirtTileSize": 5000.0, "RockTileSize": 4000.0,
        },
    },
    # =====================
    # LUTIE SNOW
    # =====================
    "lutie": {
        "ground_a": [f"{RZ}/lutie_snow/T_Lut_Snow_A", f"{RZ}/lutie_snow/T_Lut_Snow_B", f"{RZ}/lutie_snow/T_Lut_Snow_C", f"{B}/snow/T_Snow_Ground_A", f"{B}/snow/T_Snow_Ground_B"],
        "ground_b": [f"{RZ}/lutie_snow/T_Lut_Snow_B", f"{RZ}/lutie_snow/T_Lut_Snow_C", f"{B}/snow/T_Snow_Ground_C"],
        "dirt": [f"{RZ}/lutie_snow/T_Lut_Frost_A", f"{B}/snow/T_Snow_Dirt_A", f"{B}/snow/T_Snow_Dirt_B"],
        "cliff": [f"{RZ}/lutie_snow/T_Lut_IceCliff_A", f"{RZ}/lutie_snow/T_Lut_IceCliff_B", f"{B}/snow/T_Snow_Cliff_A"],
        "params_lo": {
            "WarmthTint": (0.94, 0.97, 1.03), "SaturationMult": 0.75, "BrightnessOffset": 0.08,
            "ContrastBoost": 0.85, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0005, "DirtAmount": 0.0, "DirtOnSlopes": 0.0,
            "SlopeNoiseAmount": 0.01, "SlopeNoiseFreq": 0.005, "Roughness": 0.82,
            "NormalStrength": 0.15, "AOStrength": 0.1, "UVDistortStrength": 25.0,
            "GrassWarmTileSize": 4000.0, "GrassCoolTileSize": 5500.0, "DirtTileSize": 3500.0, "RockTileSize": 2800.0,
        },
        "params_hi": {
            "WarmthTint": (0.98, 0.99, 1.06), "SaturationMult": 0.95, "BrightnessOffset": 0.25,
            "ContrastBoost": 0.98, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.005,
            "MacroNoiseScale": 0.0015, "DirtAmount": 0.15, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.012, "Roughness": 0.95,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 60.0,
            "GrassWarmTileSize": 6000.0, "GrassCoolTileSize": 7000.0, "DirtTileSize": 5000.0, "RockTileSize": 3800.0,
        },
    },
    # =====================
    # GLAST HEIM
    # =====================
    "glast_heim": {
        "ground_a": [f"{RZ}/glast_heim/T_GH_Cursed_A", f"{RZ}/glast_heim/T_GH_Cursed_B", f"{RZ}/glast_heim/T_GH_Cursed_C", f"{B}/cursed/T_Cursed_Ground_A", f"{B}/cursed/T_Cursed_Ground_B"],
        "ground_b": [f"{RZ}/glast_heim/T_GH_Cursed_B", f"{RZ}/glast_heim/T_GH_Cursed_C", f"{B}/cursed/T_Cursed_Ground_C"],
        "dirt": [f"{RZ}/glast_heim/T_GH_Blood_A", f"{B}/cursed/T_Cursed_Dirt_A"],
        "cliff": [f"{RZ}/glast_heim/T_GH_Wall_A", f"{RZ}/glast_heim/T_GH_Wall_B", f"{B}/cursed/T_Cursed_Cliff_A"],
        "params_lo": {
            "WarmthTint": (0.93, 0.94, 0.96), "SaturationMult": 0.35, "BrightnessOffset": -0.18,
            "ContrastBoost": 1.1, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0008, "DirtAmount": 0.15, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.008, "Roughness": 0.82,
            "NormalStrength": 0.5, "AOStrength": 0.4, "UVDistortStrength": 60.0,
            "GrassWarmTileSize": 2800.0, "GrassCoolTileSize": 3800.0, "DirtTileSize": 2500.0, "RockTileSize": 2000.0,
        },
        "params_hi": {
            "WarmthTint": (0.97, 0.97, 1.0), "SaturationMult": 0.65, "BrightnessOffset": -0.05,
            "ContrastBoost": 1.35, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.008,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.40, "DirtOnSlopes": 0.7,
            "SlopeNoiseAmount": 0.15, "SlopeNoiseFreq": 0.020, "Roughness": 0.95,
            "NormalStrength": 0.8, "AOStrength": 0.7, "UVDistortStrength": 100.0,
            "GrassWarmTileSize": 4200.0, "GrassCoolTileSize": 5200.0, "DirtTileSize": 3800.0, "RockTileSize": 3000.0,
        },
    },
    # =====================
    # NIFLHEIM
    # =====================
    "niflheim": {
        "ground_a": [f"{RZ}/niflheim/T_Nif_Dead_A", f"{RZ}/niflheim/T_Nif_Dead_B", f"{RZ}/niflheim/T_Nif_Dead_C", f"{B}/cursed/T_Cursed_Ground_A"],
        "ground_b": [f"{RZ}/niflheim/T_Nif_Dead_B", f"{RZ}/niflheim/T_Nif_Dead_C", f"{RZ}/niflheim/T_Nif_Ash_A"],
        "dirt": [f"{RZ}/niflheim/T_Nif_Ash_A", f"{B}/cursed/T_Cursed_Dirt_A"],
        "cliff": [f"{RZ}/niflheim/T_Nif_Wall_A", f"{RZ}/niflheim/T_Nif_Wall_B", f"{B}/cursed/T_Cursed_Cliff_B"],
        "params_lo": {
            "WarmthTint": (0.92, 0.92, 0.95), "SaturationMult": 0.25, "BrightnessOffset": -0.12,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0006, "DirtAmount": 0.10, "DirtOnSlopes": 0.1,
            "SlopeNoiseAmount": 0.04, "SlopeNoiseFreq": 0.008, "Roughness": 0.88,
            "NormalStrength": 0.3, "AOStrength": 0.3, "UVDistortStrength": 50.0,
            "GrassWarmTileSize": 3000.0, "GrassCoolTileSize": 4000.0, "DirtTileSize": 2800.0, "RockTileSize": 2200.0,
        },
        "params_hi": {
            "WarmthTint": (0.96, 0.95, 0.98), "SaturationMult": 0.50, "BrightnessOffset": 0.0,
            "ContrastBoost": 1.25, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.006,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.30, "DirtOnSlopes": 0.5,
            "SlopeNoiseAmount": 0.10, "SlopeNoiseFreq": 0.016, "Roughness": 0.96,
            "NormalStrength": 0.6, "AOStrength": 0.6, "UVDistortStrength": 85.0,
            "GrassWarmTileSize": 4500.0, "GrassCoolTileSize": 5500.0, "DirtTileSize": 4000.0, "RockTileSize": 3000.0,
        },
    },
    # =====================
    # VEINS VOLCANIC
    # =====================
    "veins": {
        "ground_a": [f"{RZ}/veins_volcanic/T_Vns_RedRock_A", f"{RZ}/veins_volcanic/T_Vns_RedRock_B", f"{RZ}/veins_volcanic/T_Vns_RedRock_C", f"{B}/volcanic/T_Volcanic_Ground_A", f"{B}/volcanic/T_Volcanic_Ground_B"],
        "ground_b": [f"{RZ}/veins_volcanic/T_Vns_RedRock_B", f"{RZ}/veins_volcanic/T_Vns_RedRock_C", f"{RZ}/veins_volcanic/T_Vns_Lava_A", f"{B}/volcanic/T_Volcanic_Ground_C"],
        "dirt": [f"{RZ}/veins_volcanic/T_Vns_Lava_A", f"{B}/volcanic/T_Volcanic_Dirt_A"],
        "cliff": [f"{RZ}/veins_volcanic/T_Vns_Canyon_A", f"{RZ}/veins_volcanic/T_Vns_Canyon_B", f"{B}/volcanic/T_Volcanic_Cliff_A"],
        "params_lo": {
            "WarmthTint": (1.06, 0.93, 0.82), "SaturationMult": 0.88, "BrightnessOffset": -0.08,
            "ContrastBoost": 1.15, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0008, "DirtAmount": 0.20, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.010, "Roughness": 0.78,
            "NormalStrength": 0.5, "AOStrength": 0.4, "UVDistortStrength": 55.0,
            "GrassWarmTileSize": 2800.0, "GrassCoolTileSize": 3800.0, "DirtTileSize": 2500.0, "RockTileSize": 1800.0,
        },
        "params_hi": {
            "WarmthTint": (1.10, 0.97, 0.88), "SaturationMult": 1.1, "BrightnessOffset": 0.05,
            "ContrastBoost": 1.5, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.008,
            "MacroNoiseScale": 0.0025, "DirtAmount": 0.45, "DirtOnSlopes": 0.8,
            "SlopeNoiseAmount": 0.15, "SlopeNoiseFreq": 0.022, "Roughness": 0.92,
            "NormalStrength": 0.9, "AOStrength": 0.7, "UVDistortStrength": 100.0,
            "GrassWarmTileSize": 4200.0, "GrassCoolTileSize": 5200.0, "DirtTileSize": 3800.0, "RockTileSize": 3000.0,
        },
    },
    # =====================
    # JAWAII BEACH
    # =====================
    "jawaii": {
        "ground_a": [f"{RZ}/jawaii_beach/T_Jaw_Sand_A", f"{RZ}/jawaii_beach/T_Jaw_Sand_B", f"{RZ}/jawaii_beach/T_Jaw_Sand_C", f"{B}/beach/T_Beach_Sand_A", f"{B}/beach/T_Beach_Sand_B"],
        "ground_b": [f"{RZ}/jawaii_beach/T_Jaw_Sand_B", f"{RZ}/jawaii_beach/T_Jaw_Sand_C", f"{B}/beach/T_Beach_Sand_C"],
        "dirt": [f"{RZ}/jawaii_beach/T_Jaw_WetSand_A", f"{B}/beach/T_Beach_Dirt_A"],
        "cliff": [f"{RZ}/jawaii_beach/T_Jaw_CoralRock_A", f"{RZ}/jawaii_beach/T_Jaw_CoralRock_B", f"{B}/beach/T_Beach_Cliff_A"],
        "params_lo": {
            "WarmthTint": (1.01, 0.99, 0.95), "SaturationMult": 0.98, "BrightnessOffset": 0.05,
            "ContrastBoost": 0.88, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0004, "DirtAmount": 0.0, "DirtOnSlopes": 0.0,
            "SlopeNoiseAmount": 0.01, "SlopeNoiseFreq": 0.005, "Roughness": 0.82,
            "NormalStrength": 0.15, "AOStrength": 0.1, "UVDistortStrength": 20.0,
            "GrassWarmTileSize": 4500.0, "GrassCoolTileSize": 6000.0, "DirtTileSize": 4000.0, "RockTileSize": 3000.0,
        },
        "params_hi": {
            "WarmthTint": (1.04, 1.01, 0.97), "SaturationMult": 1.15, "BrightnessOffset": 0.18,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.005,
            "MacroNoiseScale": 0.0012, "DirtAmount": 0.15, "DirtOnSlopes": 0.1,
            "SlopeNoiseAmount": 0.05, "SlopeNoiseFreq": 0.010, "Roughness": 0.94,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 55.0,
            "GrassWarmTileSize": 6500.0, "GrassCoolTileSize": 7500.0, "DirtTileSize": 5500.0, "RockTileSize": 4000.0,
        },
    },
    # =====================
    # EINBROCH INDUSTRIAL
    # =====================
    "einbroch": {
        "ground_a": [f"{RZ}/einbroch/T_Ein_Metal_A", f"{RZ}/einbroch/T_Ein_Metal_B", f"{RZ}/einbroch/T_Ein_Rust_A", f"{B}/industrial/T_Industrial_Floor_A", f"{B}/industrial/T_Industrial_Floor_B"],
        "ground_b": [f"{RZ}/einbroch/T_Ein_Metal_B", f"{RZ}/einbroch/T_Ein_Rust_A", f"{B}/industrial/T_Industrial_Floor_C"],
        "dirt": [f"{RZ}/einbroch/T_Ein_Soot_A", f"{B}/industrial/T_Industrial_Dirt_A"],
        "cliff": [f"{RZ}/einbroch/T_Ein_Wall_A", f"{RZ}/einbroch/T_Ein_Wall_B", f"{B}/industrial/T_Industrial_Wall_A"],
        "params_lo": {
            "WarmthTint": (0.95, 0.93, 0.92), "SaturationMult": 0.45, "BrightnessOffset": -0.12,
            "ContrastBoost": 1.1, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0008, "DirtAmount": 0.20, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.03, "SlopeNoiseFreq": 0.008, "Roughness": 0.62,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 25.0,
            "GrassWarmTileSize": 2200.0, "GrassCoolTileSize": 3200.0, "DirtTileSize": 2000.0, "RockTileSize": 1600.0,
        },
        "params_hi": {
            "WarmthTint": (0.98, 0.96, 0.95), "SaturationMult": 0.72, "BrightnessOffset": -0.02,
            "ContrastBoost": 1.35, "GrassVariantBalance": 0.6, "GrassNoiseScale": 0.007,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.45, "DirtOnSlopes": 0.6,
            "SlopeNoiseAmount": 0.10, "SlopeNoiseFreq": 0.016, "Roughness": 0.86,
            "NormalStrength": 0.7, "AOStrength": 0.6, "UVDistortStrength": 60.0,
            "GrassWarmTileSize": 3800.0, "GrassCoolTileSize": 4800.0, "DirtTileSize": 3200.0, "RockTileSize": 2800.0,
        },
    },
    # =====================
    # AMATSU
    # =====================
    "amatsu": {
        "ground_a": [f"{RZ}/amatsu/T_Amt_Earth_A", f"{RZ}/amatsu/T_Amt_Earth_B", f"{RZ}/amatsu/T_Amt_Stone_A"],
        "ground_b": [f"{RZ}/amatsu/T_Amt_Earth_B", f"{RZ}/amatsu/T_Amt_ZenSand_A", f"{RZ}/amatsu/T_Amt_Stone_A"],
        "dirt": [f"{RZ}/amatsu/T_Amt_ZenSand_A", f"{S}/T_EarthPath_01_2k"],
        "cliff": [f"{RZ}/amatsu/T_Amt_Cliff_A", f"{RZ}/amatsu/T_Amt_Cliff_B"],
        "params_lo": {
            "WarmthTint": (1.02, 0.97, 0.90), "SaturationMult": 0.80, "BrightnessOffset": -0.04,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0005, "DirtAmount": 0.10, "DirtOnSlopes": 0.1,
            "SlopeNoiseAmount": 0.03, "SlopeNoiseFreq": 0.006, "Roughness": 0.85,
            "NormalStrength": 0.3, "AOStrength": 0.2, "UVDistortStrength": 30.0,
            "GrassWarmTileSize": 2500.0, "GrassCoolTileSize": 3500.0, "DirtTileSize": 2200.0, "RockTileSize": 1800.0,
        },
        "params_hi": {
            "WarmthTint": (1.05, 1.0, 0.94), "SaturationMult": 0.98, "BrightnessOffset": 0.03,
            "ContrastBoost": 1.12, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.006,
            "MacroNoiseScale": 0.0015, "DirtAmount": 0.25, "DirtOnSlopes": 0.4,
            "SlopeNoiseAmount": 0.08, "SlopeNoiseFreq": 0.014, "Roughness": 0.94,
            "NormalStrength": 0.6, "AOStrength": 0.5, "UVDistortStrength": 65.0,
            "GrassWarmTileSize": 4000.0, "GrassCoolTileSize": 5000.0, "DirtTileSize": 3500.0, "RockTileSize": 2800.0,
        },
    },
    # =====================
    # YUNO / SKY CITY
    # =====================
    "yuno": {
        "ground_a": [f"{RZ}/yuno_marble/T_Yun_Marble_A", f"{RZ}/yuno_marble/T_Yun_Marble_B", f"{RZ}/yuno_marble/T_Yun_Alpine_A", f"{B}/urban/T_Urban_Cobble_C"],
        "ground_b": [f"{RZ}/yuno_marble/T_Yun_Marble_B", f"{RZ}/yuno_marble/T_Yun_Alpine_A", f"{B}/mountain/T_Mountain_Ground_A"],
        "dirt": [f"{RZ}/yuno_marble/T_Yun_Gravel_A", f"{B}/mountain/T_Mountain_Dirt_A"],
        "cliff": [f"{RZ}/yuno_marble/T_Yun_Cliff_A", f"{RZ}/yuno_marble/T_Yun_Cliff_B", f"{B}/mountain/T_Mountain_Cliff_A"],
        "params_lo": {
            "WarmthTint": (0.98, 0.99, 1.02), "SaturationMult": 0.80, "BrightnessOffset": 0.08,
            "ContrastBoost": 0.90, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0004, "DirtAmount": 0.05, "DirtOnSlopes": 0.0,
            "SlopeNoiseAmount": 0.02, "SlopeNoiseFreq": 0.005, "Roughness": 0.78,
            "NormalStrength": 0.2, "AOStrength": 0.2, "UVDistortStrength": 20.0,
            "GrassWarmTileSize": 3000.0, "GrassCoolTileSize": 4200.0, "DirtTileSize": 2800.0, "RockTileSize": 2200.0,
        },
        "params_hi": {
            "WarmthTint": (1.01, 1.01, 1.05), "SaturationMult": 0.98, "BrightnessOffset": 0.20,
            "ContrastBoost": 1.05, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.005,
            "MacroNoiseScale": 0.0012, "DirtAmount": 0.20, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.012, "Roughness": 0.92,
            "NormalStrength": 0.5, "AOStrength": 0.4, "UVDistortStrength": 55.0,
            "GrassWarmTileSize": 5000.0, "GrassCoolTileSize": 6200.0, "DirtTileSize": 4200.0, "RockTileSize": 3200.0,
        },
    },
    # =====================
    # MT. MJOLNIR
    # =====================
    "mjolnir": {
        "ground_a": [f"{RZ}/mjolnir_mountain/T_Mjo_Rock_A", f"{RZ}/mjolnir_mountain/T_Mjo_Rock_B", f"{RZ}/mjolnir_mountain/T_Mjo_Pine_A", f"{B}/mountain/T_Mountain_Ground_A"],
        "ground_b": [f"{RZ}/mjolnir_mountain/T_Mjo_Rock_B", f"{RZ}/mjolnir_mountain/T_Mjo_Pine_A", f"{B}/mountain/T_Mountain_Ground_C"],
        "dirt": [f"{RZ}/mjolnir_mountain/T_Mjo_Gravel_A", f"{B}/mountain/T_Mountain_Dirt_A"],
        "cliff": [f"{RZ}/mjolnir_mountain/T_Mjo_Granite_A", f"{RZ}/mjolnir_mountain/T_Mjo_Granite_B", f"{B}/mountain/T_Mountain_Cliff_A"],
        "params_lo": {
            "WarmthTint": (0.98, 0.97, 0.96), "SaturationMult": 0.80, "BrightnessOffset": -0.04,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0006, "DirtAmount": 0.15, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.05, "SlopeNoiseFreq": 0.008, "Roughness": 0.86,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 40.0,
            "GrassWarmTileSize": 2800.0, "GrassCoolTileSize": 3800.0, "DirtTileSize": 2500.0, "RockTileSize": 1800.0,
        },
        "params_hi": {
            "WarmthTint": (1.01, 0.99, 0.98), "SaturationMult": 0.95, "BrightnessOffset": 0.03,
            "ContrastBoost": 1.18, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.007,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.35, "DirtOnSlopes": 0.6,
            "SlopeNoiseAmount": 0.12, "SlopeNoiseFreq": 0.018, "Roughness": 0.95,
            "NormalStrength": 0.8, "AOStrength": 0.6, "UVDistortStrength": 80.0,
            "GrassWarmTileSize": 4500.0, "GrassCoolTileSize": 5500.0, "DirtTileSize": 3800.0, "RockTileSize": 3000.0,
        },
    },
    # =====================
    # MOSCOVIA
    # =====================
    "moscovia": {
        "ground_a": [f"{RZ}/moscovia/T_Mos_Autumn_A", f"{RZ}/moscovia/T_Mos_Autumn_B", f"{RZ}/moscovia/T_Mos_DarkForest_A"],
        "ground_b": [f"{RZ}/moscovia/T_Mos_Autumn_B", f"{RZ}/moscovia/T_Mos_DarkForest_A", f"{RZ}/moscovia/T_Mos_Leaf_A"],
        "dirt": [f"{RZ}/moscovia/T_Mos_Leaf_A", f"{B}/forest/T_Forest_Dirt_A"],
        "cliff": [f"{RZ}/moscovia/T_Mos_Cliff_A", f"{RZ}/moscovia/T_Mos_Cliff_B", f"{B}/forest/T_Forest_Cliff_A"],
        "params_lo": {
            "WarmthTint": (1.02, 0.96, 0.88), "SaturationMult": 0.82, "BrightnessOffset": -0.06,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0006, "DirtAmount": 0.20, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.05, "SlopeNoiseFreq": 0.008, "Roughness": 0.90,
            "NormalStrength": 0.3, "AOStrength": 0.3, "UVDistortStrength": 45.0,
            "GrassWarmTileSize": 2800.0, "GrassCoolTileSize": 3800.0, "DirtTileSize": 2500.0, "RockTileSize": 2000.0,
        },
        "params_hi": {
            "WarmthTint": (1.06, 0.99, 0.92), "SaturationMult": 0.98, "BrightnessOffset": 0.02,
            "ContrastBoost": 1.15, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.007,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.40, "DirtOnSlopes": 0.5,
            "SlopeNoiseAmount": 0.10, "SlopeNoiseFreq": 0.016, "Roughness": 0.96,
            "NormalStrength": 0.6, "AOStrength": 0.5, "UVDistortStrength": 80.0,
            "GrassWarmTileSize": 4500.0, "GrassCoolTileSize": 5500.0, "DirtTileSize": 3800.0, "RockTileSize": 3000.0,
        },
    },
    # =====================
    # COMODO CAVE
    # =====================
    "comodo": {
        "ground_a": [f"{RZ}/comodo_cave/T_Com_CaveSand_A", f"{RZ}/comodo_cave/T_Com_CaveSand_B", f"{RZ}/comodo_cave/T_Com_WetRock_A", f"{B}/cave/T_Cave_Floor_A"],
        "ground_b": [f"{RZ}/comodo_cave/T_Com_CaveSand_B", f"{RZ}/comodo_cave/T_Com_WetRock_A", f"{B}/cave/T_Cave_Floor_B"],
        "dirt": [f"{RZ}/comodo_cave/T_Com_DarkSand_A", f"{B}/cave/T_Cave_Dirt_A"],
        "cliff": [f"{RZ}/comodo_cave/T_Com_CaveWall_A", f"{RZ}/comodo_cave/T_Com_CaveWall_B", f"{B}/cave/T_Cave_Wall_A"],
        "params_lo": {
            "WarmthTint": (1.02, 0.97, 0.92), "SaturationMult": 0.78, "BrightnessOffset": -0.10,
            "ContrastBoost": 1.05, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0006, "DirtAmount": 0.15, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.05, "SlopeNoiseFreq": 0.008, "Roughness": 0.82,
            "NormalStrength": 0.4, "AOStrength": 0.4, "UVDistortStrength": 45.0,
            "GrassWarmTileSize": 2500.0, "GrassCoolTileSize": 3500.0, "DirtTileSize": 2200.0, "RockTileSize": 1800.0,
        },
        "params_hi": {
            "WarmthTint": (1.05, 1.0, 0.96), "SaturationMult": 0.95, "BrightnessOffset": -0.02,
            "ContrastBoost": 1.20, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.007,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.35, "DirtOnSlopes": 0.5,
            "SlopeNoiseAmount": 0.10, "SlopeNoiseFreq": 0.016, "Roughness": 0.94,
            "NormalStrength": 0.7, "AOStrength": 0.6, "UVDistortStrength": 80.0,
            "GrassWarmTileSize": 4000.0, "GrassCoolTileSize": 5000.0, "DirtTileSize": 3500.0, "RockTileSize": 2800.0,
        },
    },
    # =====================
    # UMBALA JUNGLE
    # =====================
    "umbala": {
        "ground_a": [f"{RZ}/umbala_jungle/T_Umb_Jungle_A", f"{RZ}/umbala_jungle/T_Umb_Jungle_B", f"{RZ}/umbala_jungle/T_Umb_Wood_A", f"{B}/jungle/T_Jungle_Floor_A"],
        "ground_b": [f"{RZ}/umbala_jungle/T_Umb_Jungle_B", f"{RZ}/umbala_jungle/T_Umb_Wood_A", f"{B}/jungle/T_Jungle_Floor_C"],
        "dirt": [f"{RZ}/umbala_jungle/T_Umb_Mud_A", f"{B}/jungle/T_Jungle_Dirt_A"],
        "cliff": [f"{RZ}/umbala_jungle/T_Umb_Bark_A", f"{RZ}/umbala_jungle/T_Umb_VineRock_A", f"{B}/jungle/T_Jungle_Cliff_A"],
        "params_lo": {
            "WarmthTint": (1.0, 0.97, 0.90), "SaturationMult": 0.82, "BrightnessOffset": -0.10,
            "ContrastBoost": 1.05, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.003,
            "MacroNoiseScale": 0.0008, "DirtAmount": 0.20, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.010, "Roughness": 0.88,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 50.0,
            "GrassWarmTileSize": 2500.0, "GrassCoolTileSize": 3500.0, "DirtTileSize": 2200.0, "RockTileSize": 1800.0,
        },
        "params_hi": {
            "WarmthTint": (1.04, 1.0, 0.94), "SaturationMult": 0.98, "BrightnessOffset": -0.02,
            "ContrastBoost": 1.20, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.008,
            "MacroNoiseScale": 0.002, "DirtAmount": 0.40, "DirtOnSlopes": 0.7,
            "SlopeNoiseAmount": 0.12, "SlopeNoiseFreq": 0.018, "Roughness": 0.96,
            "NormalStrength": 0.7, "AOStrength": 0.6, "UVDistortStrength": 90.0,
            "GrassWarmTileSize": 4000.0, "GrassCoolTileSize": 5000.0, "DirtTileSize": 3500.0, "RockTileSize": 2800.0,
        },
    },
    # =====================
    # RACHEL FROZEN
    # =====================
    "rachel": {
        "ground_a": [f"{RZ}/rachel_frozen/T_Rac_IceStone_A", f"{RZ}/rachel_frozen/T_Rac_IceStone_B", f"{RZ}/rachel_frozen/T_Rac_Tundra_A", f"{B}/snow/T_Snow_Ground_A"],
        "ground_b": [f"{RZ}/rachel_frozen/T_Rac_IceStone_B", f"{RZ}/rachel_frozen/T_Rac_Tundra_A", f"{B}/snow/T_Snow_Ground_C"],
        "dirt": [f"{RZ}/rachel_frozen/T_Rac_FrostDirt_A", f"{B}/snow/T_Snow_Dirt_A"],
        "cliff": [f"{RZ}/rachel_frozen/T_Rac_IceCliff_A", f"{RZ}/rachel_frozen/T_Rac_IceCliff_B", f"{B}/snow/T_Snow_Cliff_B"],
        "params_lo": {
            "WarmthTint": (0.92, 0.96, 1.04), "SaturationMult": 0.70, "BrightnessOffset": 0.05,
            "ContrastBoost": 0.88, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0004, "DirtAmount": 0.05, "DirtOnSlopes": 0.1,
            "SlopeNoiseAmount": 0.02, "SlopeNoiseFreq": 0.005, "Roughness": 0.80,
            "NormalStrength": 0.2, "AOStrength": 0.1, "UVDistortStrength": 25.0,
            "GrassWarmTileSize": 3800.0, "GrassCoolTileSize": 5200.0, "DirtTileSize": 3500.0, "RockTileSize": 2800.0,
        },
        "params_hi": {
            "WarmthTint": (0.96, 0.98, 1.08), "SaturationMult": 0.90, "BrightnessOffset": 0.20,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.005,
            "MacroNoiseScale": 0.0012, "DirtAmount": 0.20, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.012, "Roughness": 0.94,
            "NormalStrength": 0.4, "AOStrength": 0.3, "UVDistortStrength": 55.0,
            "GrassWarmTileSize": 5800.0, "GrassCoolTileSize": 7000.0, "DirtTileSize": 5000.0, "RockTileSize": 3800.0,
        },
    },
    # =====================
    # PYRAMID / RUINS
    # =====================
    "pyramid": {
        "ground_a": [f"{RZ}/pyramid/T_Pyr_Sandstone_A", f"{RZ}/pyramid/T_Pyr_Sandstone_B", f"{RZ}/pyramid/T_Pyr_DarkStone_A", f"{B}/ruins/T_Ruins_Floor_A"],
        "ground_b": [f"{RZ}/pyramid/T_Pyr_Sandstone_B", f"{RZ}/pyramid/T_Pyr_DarkStone_A", f"{B}/ruins/T_Ruins_Floor_C"],
        "dirt": [f"{RZ}/pyramid/T_Pyr_Sand_A", f"{B}/ruins/T_Ruins_Dirt_A"],
        "cliff": [f"{RZ}/pyramid/T_Pyr_HieroWall_A", f"{RZ}/pyramid/T_Pyr_HieroWall_B", f"{B}/ruins/T_Ruins_Wall_A"],
        "params_lo": {
            "WarmthTint": (1.03, 0.97, 0.88), "SaturationMult": 0.78, "BrightnessOffset": -0.05,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0005, "DirtAmount": 0.15, "DirtOnSlopes": 0.2,
            "SlopeNoiseAmount": 0.03, "SlopeNoiseFreq": 0.006, "Roughness": 0.80,
            "NormalStrength": 0.3, "AOStrength": 0.3, "UVDistortStrength": 30.0,
            "GrassWarmTileSize": 2500.0, "GrassCoolTileSize": 3500.0, "DirtTileSize": 2200.0, "RockTileSize": 1800.0,
        },
        "params_hi": {
            "WarmthTint": (1.06, 1.0, 0.92), "SaturationMult": 0.95, "BrightnessOffset": 0.05,
            "ContrastBoost": 1.18, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.006,
            "MacroNoiseScale": 0.0015, "DirtAmount": 0.35, "DirtOnSlopes": 0.5,
            "SlopeNoiseAmount": 0.08, "SlopeNoiseFreq": 0.014, "Roughness": 0.93,
            "NormalStrength": 0.6, "AOStrength": 0.5, "UVDistortStrength": 65.0,
            "GrassWarmTileSize": 4000.0, "GrassCoolTileSize": 5000.0, "DirtTileSize": 3500.0, "RockTileSize": 2800.0,
        },
    },
    # =====================
    # LOUYANG
    # =====================
    "louyang": {
        "ground_a": [f"{RZ}/louyang/T_Lou_Stone_A", f"{RZ}/louyang/T_Lou_Stone_B", f"{RZ}/louyang/T_Lou_Garden_A", f"{B}/urban/T_Urban_Cobble_A"],
        "ground_b": [f"{RZ}/louyang/T_Lou_Stone_B", f"{RZ}/louyang/T_Lou_Garden_A", f"{B}/urban/T_Urban_Cobble_C"],
        "dirt": [f"{RZ}/louyang/T_Lou_Path_A", f"{B}/urban/T_Urban_Dirt_A"],
        "cliff": [f"{RZ}/louyang/T_Lou_MtnRock_A", f"{RZ}/louyang/T_Lou_MtnRock_B", f"{B}/mountain/T_Mountain_Cliff_A"],
        "params_lo": {
            "WarmthTint": (1.0, 0.98, 0.95), "SaturationMult": 0.82, "BrightnessOffset": -0.02,
            "ContrastBoost": 1.0, "GrassVariantBalance": 0.3, "GrassNoiseScale": 0.002,
            "MacroNoiseScale": 0.0005, "DirtAmount": 0.10, "DirtOnSlopes": 0.1,
            "SlopeNoiseAmount": 0.03, "SlopeNoiseFreq": 0.006, "Roughness": 0.78,
            "NormalStrength": 0.3, "AOStrength": 0.2, "UVDistortStrength": 25.0,
            "GrassWarmTileSize": 2200.0, "GrassCoolTileSize": 3200.0, "DirtTileSize": 2000.0, "RockTileSize": 1600.0,
        },
        "params_hi": {
            "WarmthTint": (1.03, 1.0, 0.97), "SaturationMult": 0.96, "BrightnessOffset": 0.05,
            "ContrastBoost": 1.12, "GrassVariantBalance": 0.7, "GrassNoiseScale": 0.005,
            "MacroNoiseScale": 0.0012, "DirtAmount": 0.25, "DirtOnSlopes": 0.3,
            "SlopeNoiseAmount": 0.06, "SlopeNoiseFreq": 0.012, "Roughness": 0.92,
            "NormalStrength": 0.5, "AOStrength": 0.4, "UVDistortStrength": 55.0,
            "GrassWarmTileSize": 3800.0, "GrassCoolTileSize": 4800.0, "DirtTileSize": 3200.0, "RockTileSize": 2600.0,
        },
    },
}

# Cross-biome pairs
CROSS_PAIRS = [
    ("prontera", "geffen"), ("prontera", "payon"), ("prontera", "morroc"),
    ("prontera", "jawaii"), ("prontera", "mjolnir"), ("prontera", "lutie"),
    ("geffen", "payon"), ("geffen", "glast_heim"), ("geffen", "mjolnir"),
    ("morroc", "veins"), ("morroc", "pyramid"), ("morroc", "jawaii"),
    ("lutie", "rachel"), ("lutie", "mjolnir"), ("lutie", "yuno"),
    ("payon", "moscovia"), ("payon", "umbala"), ("payon", "amatsu"),
    ("glast_heim", "niflheim"), ("glast_heim", "geffen"),
    ("veins", "einbroch"), ("veins", "morroc"),
    ("jawaii", "comodo"), ("jawaii", "umbala"),
    ("einbroch", "yuno"), ("einbroch", "louyang"),
    ("amatsu", "louyang"), ("amatsu", "payon"),
    ("comodo", "umbala"), ("comodo", "payon"),
    ("niflheim", "glast_heim"), ("niflheim", "rachel"),
    ("pyramid", "morroc"), ("pyramid", "louyang"),
    ("yuno", "rachel"), ("yuno", "mjolnir"),
    ("umbala", "moscovia"), ("moscovia", "lutie"),
    ("rachel", "niflheim"), ("louyang", "amatsu"),
]

VARIANTS_PER_ZONE = 30
VARIANTS_PER_CROSS = 8

# ============================================================
# VARIANT GENERATOR
# ============================================================

def generate_zone_variants(zone_name, zone_def):
    """Generate VARIANTS_PER_ZONE variants for a single zone."""
    variants = []
    ga, gb = zone_def["ground_a"], zone_def["ground_b"]
    d_list, c_list = zone_def["dirt"], zone_def["cliff"]
    p_lo, p_hi = zone_def["params_lo"], zone_def["params_hi"]

    for i in range(VARIANTS_PER_ZONE):
        t = i / max(VARIANTS_PER_ZONE - 1, 1)
        # Cycle through texture combos
        a = ga[i % len(ga)]
        b = gb[(i + 1) % len(gb)]
        if a == b:
            b = gb[(i + 2) % len(gb)]
        d = d_list[i % len(d_list)]
        c = c_list[i % len(c_list)]

        # Interpolate ALL params
        params = {}
        for key in p_lo:
            if key == "WarmthTint":
                color = lerp_color(p_lo[key], p_hi[key], t)
                params[key] = color
            else:
                params[key] = round(lerp(p_lo[key], p_hi[key], t), 4)

        a_short = a.split("/")[-1][:10]
        name = f"{zone_name}_{i+1:02d}_{a_short}"
        variants.append({"name": name, "zone": zone_name, "a": a, "b": b, "d": d, "c": c, "params": params})

    return variants


def generate_cross_variants(zone_a_name, zone_b_name):
    """Generate cross-biome variants by blending two zones."""
    za, zb = ZONES.get(zone_a_name), ZONES.get(zone_b_name)
    if not za or not zb:
        return []

    variants = []
    for i in range(VARIANTS_PER_CROSS):
        t = i / max(VARIANTS_PER_CROSS - 1, 1)
        a = za["ground_a"][i % len(za["ground_a"])]
        b = zb["ground_a"][i % len(zb["ground_a"])]
        c = za["cliff"][i % len(za["cliff"])] if t < 0.5 else zb["cliff"][i % len(zb["cliff"])]
        d = zb["dirt"][i % len(zb["dirt"])] if t < 0.5 else za["dirt"][i % len(za["dirt"])]

        # Blend params from both zones
        params = {}
        for key in za["params_lo"]:
            if key == "WarmthTint":
                c_a = lerp_color(za["params_lo"][key], za["params_hi"][key], t)
                c_b = lerp_color(zb["params_lo"][key], zb["params_hi"][key], t)
                params[key] = lerp_color(c_a, c_b, 0.5)
            else:
                v_a = lerp(za["params_lo"][key], za["params_hi"][key], t)
                v_b = lerp(zb["params_lo"][key], zb["params_hi"][key], t)
                params[key] = round((v_a + v_b) / 2.0, 4)

        name = f"x_{zone_a_name[:4]}_{zone_b_name[:4]}_{i+1:02d}"
        variants.append({"name": name, "zone": f"{zone_a_name}_{zone_b_name}", "a": a, "b": b, "d": d, "c": c, "params": params})

    return variants


# ============================================================
# MAIN
# ============================================================

parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent not found: {PARENT_PATH}")
    unreal.log_error("Run create_landscape_material.py (v12) first!")
    raise RuntimeError("Missing parent")

# Generate all variants
all_variants = []
for zone_name, zone_def in ZONES.items():
    zv = generate_zone_variants(zone_name, zone_def)
    all_variants.extend(zv)
    unreal.log(f"  {zone_name}: {len(zv)} variants")

for za, zb in CROSS_PAIRS:
    cv = generate_cross_variants(za, zb)
    all_variants.extend(cv)

unreal.log(f"  cross-biome: {len(CROSS_PAIRS) * VARIANTS_PER_CROSS} variants")
unreal.log(f"\n  TOTAL: {len(all_variants)} variants to create\n")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()

created = 0
skipped = 0
tracking = []

for i, v in enumerate(all_variants):
    mi_name = f"MI_{v['name']}"
    zone_folder = v["zone"]
    mi_path = f"{MI_BASE}/{zone_folder}"
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

    # Textures
    for param, path in [("GrassWarmTexture", v["a"]), ("GrassCoolTexture", v["b"]),
                         ("DirtTexture", v["d"]), ("RockTexture", v["c"])]:
        if eal.does_asset_exist(path):
            tex = unreal.load_asset(path)
            if tex:
                mel.set_material_instance_texture_parameter_value(
                    mi, param, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Scalar params
    for param_name, value in v["params"].items():
        if param_name == "WarmthTint":
            mel.set_material_instance_vector_parameter_value(
                mi, "WarmthTint", unreal.LinearColor(value[0], value[1], value[2], 1.0),
                unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
        else:
            mel.set_material_instance_scalar_parameter_value(
                mi, param_name, value, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1

    if (i + 1) % 50 == 0:
        unreal.log(f"  [{i+1}/{len(all_variants)}] {created} created...")

# Write tracking doc
with open(TRACKING_PATH, "w") as f:
    f.write(f"# Biome Variants v3 (23 parameters)\n\n")
    f.write(f"Generated: {datetime.date.today()}\n")
    f.write(f"Parent: M_Landscape_RO_12\n")
    f.write(f"Total: {created} created, {skipped} skipped\n\n")
    f.write(f"## Zone counts\n\n")
    zone_counts = {}
    for v in all_variants:
        zone_counts[v["zone"]] = zone_counts.get(v["zone"], 0) + 1
    for z, c in sorted(zone_counts.items()):
        f.write(f"- **{z}**: {c}\n")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {created} created, {skipped} skipped")
unreal.log(f"  Browse: {MI_BASE}/")
unreal.log(f"  {len(ZONES)} zones + {len(CROSS_PAIRS)} cross-biome pairs")
unreal.log(f"  23 parameters varied per variant")
unreal.log(f"{'='*60}")
