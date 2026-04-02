# create_ro_classic_variants.py
# Creates simple RO Classic-style material instances.
# These are SIMPLER than the biome variants — focused on the warm
# yellow-green ground look from the reference screenshots.
#
# Key: DirtAmount=0, strong macro variation, warm palette,
# just two similar grass textures blended by large noise.

import unreal
import os
import datetime

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_11"
MI_BASE = "/Game/SabriMMO/Materials/Environment/ROClassic"
eal.make_directory(MI_BASE)

RC = "/Game/SabriMMO/Textures/Environment/ROClassic"
S = "/Game/SabriMMO/Textures/Environment/Ground_Seamless"
G = "/Game/SabriMMO/Textures/Environment/Ground"

# Good textures for classic RO look
GROUND_A = [
    f"{RC}/T_RO_YellowGreen_A",   # warm yellow-green (best)
    f"{RC}/T_RO_YellowGreen_D",   # muted golden
    f"{RC}/T_RO_DarkGreen_A",     # darker olive
    f"{S}/T_LushGreen_03_2k",     # watercolor splotches
    f"{S}/T_LushGreen_02_2k",     # green/brown camo
]
GROUND_B = [
    f"{RC}/T_RO_DarkGreen_A",     # darker variant
    f"{RC}/T_RO_DarkGreen_B",     # deep sage
    f"{RC}/T_RO_YellowGreen_A",   # warm yellow-green
    f"{RC}/T_RO_BrownGreen_A",    # green with tan
    f"{S}/T_MossFloor_01_2k",     # dark moody
]
DIRT = [
    f"{RC}/T_RO_BrownGreen_A",    # green-brown
    f"{RC}/T_RO_BrownGreen_B",    # forest floor brown
    f"{S}/T_EarthPath_01_2k",     # warm earth
]
CLIFF = [
    f"{RC}/T_RO_DarkCliff_A",     # dark cracked rock (best match)
    f"{RC}/T_RO_DarkCliff_B",     # dark rocky
    f"{S}/T_StoneCliff_02_2k",    # purple-gray boulders
]

# Simple variants — all combinations
VARIANTS = []
idx = 0
for a in GROUND_A:
    for b in GROUND_B:
        if a == b:
            continue
        for c in CLIFF:
            for dirt in [0.0, 0.10, 0.20]:
                for macro_style in ["subtle", "strong"]:
                    idx += 1
                    d = DIRT[idx % len(DIRT)]
                    a_short = a.split("/")[-1][:12]
                    c_short = c.split("/")[-1][:12]
                    m_tag = "sm" if macro_style == "subtle" else "sg"

                    params = {
                        "DirtAmount": dirt,
                        "Roughness": 0.95,
                        "NormalStrength": 0.3,
                        "AOStrength": 0.3,
                        "UVDistortStrength": 50.0 if macro_style == "subtle" else 80.0,
                        "GrassWarmTileSize": 4500.0 if macro_style == "subtle" else 3500.0,
                        "GrassCoolTileSize": 5800.0 if macro_style == "subtle" else 4500.0,
                        "DirtTileSize": 3800.0,
                        "RockTileSize": 2800.0,
                        "SlopeThreshold": 0.60,
                        "SlopeTransitionWidth": 0.15,
                        "MacroDarken": 0.86 if macro_style == "strong" else 0.90,
                        "MacroBrighten": 1.14 if macro_style == "strong" else 1.10,
                    }

                    VARIANTS.append({
                        "name": f"ROClassic_{idx:03d}_{a_short}_{c_short}_d{int(dirt*100):02d}_{m_tag}",
                        "a": a, "b": b, "d": d, "c": c,
                        "params": params,
                    })

# Cap at reasonable number
VARIANTS = VARIANTS[:200]

# Load parent
parent = unreal.load_asset(PARENT_PATH)
if not parent:
    unreal.log_error(f"Parent not found: {PARENT_PATH}")
    raise RuntimeError("Missing parent")

unreal.log(f"Creating {len(VARIANTS)} RO Classic material variants...")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()
created = 0

for i, v in enumerate(VARIANTS):
    mi_name = f"MI_{v['name']}"
    mi_full = f"{MI_BASE}/{mi_name}"

    if eal.does_asset_exist(mi_full):
        continue

    mi = asset_tools.create_asset(mi_name, MI_BASE,
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

    # All scalar params
    for param_name, value in v["params"].items():
        mel.set_material_instance_scalar_parameter_value(
            mi, param_name, value, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1

    if (i+1) % 25 == 0:
        unreal.log(f"  [{i+1}/{len(VARIANTS)}] {created} created...")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {created} RO Classic variants created")
unreal.log(f"  Browse: {MI_BASE}/")
unreal.log(f"  These are SIMPLE: warm yellow-green, no dirt or minimal,")
unreal.log(f"  dark cliff on slopes, matching the reference screenshots")
unreal.log(f"{'='*60}")
