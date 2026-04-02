# create_30_variants.py
# Creates 30 Material Instances from M_Landscape_RO_09 with different
# texture combinations and settings. Each variant is a lightweight MI
# (no node graph rebuild needed — just parameter overrides).
#
# Run in UE5: exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_30_variants.py").read())

import unreal
import os
import datetime

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_09"
MI_BASE_PATH = "/Game/SabriMMO/Materials/Environment/Variants"
TRACKING_PATH = "C:/Sabri_MMO/docsNew/05_Development/Material_Variant_Tracker.md"

# Ensure output folder
eal.make_directory(MI_BASE_PATH)

# ============================================================
# Texture path helpers
# ============================================================

# All available textures by category
TEX = {
    # Green grass
    "lush3":    "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_LushGreen_03_2k",
    "lush2":    "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_LushGreen_02_2k",
    "lush1":    "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_LushGreen_01_2k",
    "moss":     "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_MossFloor_01_2k",
    "mixed_b":  "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_Ground_RO_Mixed_B_2k",
    "dense":    "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_Ground_RO_DenseGrass_A_2k",
    "dirtgrass": "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_Ground_RO_DirtGrass_A_2k",
    "grassv1":  "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_Grass_Warm_RO_v1_2k",
    "grassv3":  "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_Grass_Warm_RO_v3_2k",
    # Dirt
    "earth":    "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_EarthPath_01_2k",
    "dirt1":    "/Game/SabriMMO/Textures/Environment/Ground/T_Dirt_RO_v1",
    "rocky":    "/Game/SabriMMO/Textures/Environment/Ground/T_RockyDirt_A",
    "gb1":      "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_GreenBrown_01_2k",
    # Rock
    "cliff2":   "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_StoneCliff_02_2k",
    "cliff1":   "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_StoneCliff_01_2k",
    "rock1":    "/Game/SabriMMO/Textures/Environment/Ground_Seamless/T_Rock_RO_v1_2k",
    # Sand
    "sw1":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sand_Warm_01",
    "sw2":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sand_Warm_02",
    "sw3":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sand_Warm_03",
    "sc1":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sand_Cool_01",
    "sc2":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sand_Cool_02",
    "de1":      "/Game/SabriMMO/Textures/Environment/Sand/T_DryEarth_01",
    "de2":      "/Game/SabriMMO/Textures/Environment/Sand/T_DryEarth_02",
    "ss1":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sandstone_Cliff_01",
    "ss2":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sandstone_Cliff_02",
    "ss3":      "/Game/SabriMMO/Textures/Environment/Sand/T_Sandstone_Cliff_03",
}

# ============================================================
# 30 variant definitions
# [name, description, warm_tex, cool_tex, dirt_tex, rock_tex, dirt_amt]
# ============================================================

VARIANTS = [
    # === GREEN ZONE (1-10) ===
    ["V01_Green_Classic",
     "Classic green — LushGreen 03+02, earth dirt, gray stone cliff",
     "lush3", "lush2", "earth", "cliff2", 0.25],

    ["V02_Green_MossForest",
     "Dark forest floor — LushGreen 03 + dark moss, earth, gray cliff",
     "lush3", "moss", "earth", "cliff2", 0.20],

    ["V03_Green_BrightField",
     "Bright meadow — LushGreen 02+01, earth, lighter cliff",
     "lush2", "lush1", "earth", "cliff1", 0.25],

    ["V04_Green_DenseRich",
     "Dense rich green — DenseGrass + LushGreen 03, rocky dirt, gray cliff",
     "dense", "lush3", "rocky", "cliff2", 0.15],

    ["V05_Green_WildMeadow",
     "Wild meadow — DirtGrass + LushGreen 02, earth dirt, original rock",
     "dirtgrass", "lush2", "earth", "rock1", 0.30],

    ["V06_Green_SoftBlend",
     "Soft subtle — Mixed_B + LushGreen 03, basic dirt, gray cliff",
     "mixed_b", "lush3", "dirt1", "cliff2", 0.20],

    ["V07_Green_WarmGrass",
     "Warm grass — GrassWarmV1 + LushGreen 03, earth, gray cliff",
     "grassv1", "lush3", "earth", "cliff2", 0.25],

    ["V08_Green_NoDirt",
     "Pure green, no dirt — LushGreen 03+02, zero dirt, gray cliff",
     "lush3", "lush2", "earth", "cliff2", 0.0],

    ["V09_Green_HeavyDirt",
     "Heavy worn paths — LushGreen 03+02, lots of earth showing, gray cliff",
     "lush3", "lush2", "earth", "cliff2", 0.50],

    ["V10_Green_DarkForest",
     "Dark moody forest — MossFloor + DirtGrass, earth, gray cliff",
     "moss", "dirtgrass", "earth", "cliff2", 0.30],

    # === SAND/DESERT ZONE (11-20) ===
    ["V11_Sand_Classic",
     "Classic desert — warm sand + dune waves, dry earth, layered sandstone cliff",
     "sw2", "sw1", "de1", "ss1", 0.25],

    ["V12_Sand_DuneWaves",
     "Flowing dunes — warm sand variants, dry earth, canyon wall cliff",
     "sw1", "sw3", "de1", "ss2", 0.20],

    ["V13_Sand_CoolDesert",
     "Cooler desert — warm + cool sand mix, dry earth, sandstone layers",
     "sw2", "sc2", "de1", "ss1", 0.25],

    ["V14_Sand_RoughDesert",
     "Rough terrain — warm sand, crater earth, cobble-stone cliff",
     "sw1", "sw2", "de2", "ss3", 0.30],

    ["V15_Sand_CleanDunes",
     "Clean sand dunes — no dirt patches, pure sand, layered cliff",
     "sw2", "sw1", "de1", "ss1", 0.0],

    ["V16_Sand_PaleDust",
     "Pale dusty — cool sand + warm sand, dry earth, canyon walls",
     "sc2", "sw2", "de1", "ss2", 0.20],

    ["V17_Sand_GoldenHot",
     "Hot golden desert — all warm sand tones, dry earth, sandstone",
     "sw3", "sw1", "de1", "ss1", 0.15],

    ["V18_Sand_PurpleRock",
     "Sand with purple-gray cliff — sand flats, gray stone slopes",
     "sw2", "sw1", "de1", "cliff2", 0.25],

    ["V19_Sand_HeavyErosion",
     "Eroded desert — warm sand, heavy crater earth, canyon cliff",
     "sw1", "sw2", "de2", "ss2", 0.45],

    ["V20_Sand_WarmCanyon",
     "Warm canyon floor — dry earth blending into sand, layered cliff",
     "sw2", "de1", "sw1", "ss1", 0.35],

    # === MIXED/TRANSITION ZONE (21-30) ===
    ["V21_Mixed_GrassToSand",
     "Grass with sandy patches — green base, sand variant, earth, gray cliff",
     "lush3", "sw2", "earth", "cliff2", 0.25],

    ["V22_Mixed_SandToGrass",
     "Sand with grass patches — sand base, green variant, dry earth, sandstone",
     "sw2", "lush3", "de1", "ss1", 0.25],

    ["V23_Mixed_GreenSandstone",
     "Green fields with sandstone cliffs — classic green, sandstone slopes",
     "lush3", "lush2", "earth", "ss1", 0.25],

    ["V24_Mixed_SandGrayRock",
     "Sand flats with gray rock slopes — desert meets mountain",
     "sw1", "sw2", "earth", "cliff2", 0.20],

    ["V25_Mixed_DarkMoss",
     "Dark mossy forest — moss + lush green, dry earth, dark cliff",
     "moss", "lush2", "de1", "cliff1", 0.35],

    ["V26_Mixed_GreenCanyon",
     "Green plateau with canyon walls — lush green, canyon cliff slopes",
     "lush3", "lush2", "de1", "ss2", 0.20],

    ["V27_Mixed_AutumnField",
     "Autumn field — warm grass + sand tones, rocky dirt, sandstone cliff",
     "grassv3", "sw2", "rocky", "ss1", 0.30],

    ["V28_Mixed_LushForest",
     "Lush dense forest — DenseGrass + Moss, earth, gray cliff",
     "dense", "moss", "earth", "cliff2", 0.15],

    ["V29_Mixed_AridHighDesert",
     "High desert — sand + dry earth dominant, lots of dirt showing, sandstone",
     "sw2", "sw1", "de1", "ss1", 0.50],

    ["V30_Mixed_Coastal",
     "Coastal zone — sand + green grass, earth transition, gray cliff",
     "sw2", "lush3", "earth", "cliff2", 0.30],
]


def try_load(key):
    """Load a texture by key, return None if not found."""
    path = TEX.get(key)
    if not path:
        return None
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    # Try without _2k suffix
    alt = path.replace("_2k", "")
    if eal.does_asset_exist(alt):
        return unreal.load_asset(alt)
    # Try Ground/ instead of Ground_Seamless/
    alt2 = path.replace("Ground_Seamless", "Ground")
    if eal.does_asset_exist(alt2):
        return unreal.load_asset(alt2)
    return None


# Load parent material
parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent material not found: {PARENT_PATH}")
    unreal.log_error("Run create_landscape_material.py with MAT_VERSION=9 first!")
    raise RuntimeError("Parent not found")

unreal.log(f"Parent: {PARENT_PATH}")
unreal.log(f"Creating {len(VARIANTS)} material instance variants...\n")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()
created = 0
tracking_lines = []

for var in VARIANTS:
    name, desc, warm_key, cool_key, dirt_key, rock_key, dirt_amt = var
    mi_name = f"MI_{name}"
    mi_full = f"{MI_BASE_PATH}/{mi_name}"

    # Skip if exists
    if eal.does_asset_exist(mi_full):
        unreal.log(f"  {mi_name}: exists, skipping")
        created += 1
        continue

    # Create MI
    mi = asset_tools.create_asset(mi_name, MI_BASE_PATH,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        unreal.log_warning(f"  {mi_name}: FAILED to create")
        continue

    mi.set_editor_property("parent", parent)

    # Set textures
    tex_assignments = {
        "GrassWarmTexture": warm_key,
        "GrassCoolTexture": cool_key,
        "DirtTexture": dirt_key,
        "RockTexture": rock_key,
    }
    tex_ok = 0
    for param, key in tex_assignments.items():
        tex = try_load(key)
        if tex:
            # Try both association types
            ok = mel.set_material_instance_texture_parameter_value(
                mi, param, tex, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
            if not ok:
                ok = mel.set_material_instance_texture_parameter_value(
                    mi, param, tex, unreal.MaterialParameterAssociation.BLEND_PARAMETER)
            if ok:
                tex_ok += 1

    # Set dirt amount
    mel.set_material_instance_scalar_parameter_value(
        mi, "DirtAmount", dirt_amt, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    unreal.log(f"  {mi_name}: created ({tex_ok}/4 textures, dirt={dirt_amt})")
    created += 1

    tracking_lines.append(
        f"| {name} | {desc} | {warm_key}+{cool_key} | {dirt_key} | {rock_key} | {dirt_amt} |"
    )

# Append to tracking doc
if tracking_lines:
    with open(TRACKING_PATH, "a") as f:
        f.write(f"\n## 30 Variants (Material Instances of v09) — {datetime.date.today()}\n\n")
        f.write("| Name | Description | Ground Textures | Dirt | Cliff | DirtAmt |\n")
        f.write("|------|-------------|-----------------|------|-------|---------|\n")
        for line in tracking_lines:
            f.write(line + "\n")

unreal.log(f"\n{'=' * 60}")
unreal.log(f"  DONE: {created}/{len(VARIANTS)} variants created")
unreal.log(f"  Browse: {MI_BASE_PATH}")
unreal.log(f"  Drag any MI onto your landscape to preview")
unreal.log(f"{'=' * 60}")
