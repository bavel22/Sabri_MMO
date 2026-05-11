# create_mi_paved_town.py
# Creates MI_Landscape_PavedTown_v1 from M_Landscape_RO_18.
#
# Generic, reusable paved-town material instance:
#   - Pryth (pastoral capital, paint plaza as Brick / streets as Cobble / side paths as Pebble)
#   - Future Prontera-clone, paved Geffen, any pastoral capital town
#   - Per-zone tweaks (e.g. cooler WarmthTint for Geffen, warmer for desert town)
#     via duplicating this MI in UE5 and overriding the 1-2 changed params,
#     OR creating a child MI parented to this one
#
# v18 supports paint-driven pavement layers (Brick/Cobble/Pebble) on top of
# the v17 4-slot grass+dirt+rock auto-blend:
#   - GrassWarm/Cool slots = cobblestone (default for unpainted areas)
#   - BrickTexture  = brickstone (paint plaza floor as "Brick")
#   - CobbleTexture = cobblestone (paint streets as "Cobble" — same as default)
#   - PebbleTexture = pebblestone (paint side paths as "Pebble")
#   - DirtTexture   = packed dirt (residential side strips, slope transitions)
#   - RockTexture   = warm cliff (moat bank slope-rock kick-in)
#
# Run in UE5 editor (Tools > Execute Python Script, or paste into Python Console):
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_mi_paved_town.py").read())

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ATL = unreal.AssetToolsHelpers.get_asset_tools()

# ============================================================
# CONFIG
# ============================================================
PARENT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_18"
MI_FOLDER   = "/Game/SabriMMO/Materials/Environment/v3"
MI_NAME     = "MI_Landscape_PavedTown_v1"
MI_FULL     = f"{MI_FOLDER}/{MI_NAME}"

# Texture roots
RZ  = "/Game/SabriMMO/Textures/Environment/ROZones"
GS  = "/Game/SabriMMO/Textures/Environment/Ground_Seamless"
GR  = "/Game/SabriMMO/Textures/Environment/Ground"
G2K = "/Game/SabriMMO/Textures/Environment/Ground_2K"
URB = "/Game/SabriMMO/Textures/Environment/Biomes/urban"

# ── DEFAULT (unpainted) cobblestone for GrassWarm/Cool slots ──
TEX_GRASS_WARM = f"{GS}/T_Cobble_RO_2k"        # Primary cobblestone (despite "Grass" slot name)
TEX_GRASS_COOL = f"{URB}/T_Urban_Cobble_A"     # Variation cobblestone for blending
# ── ACCENT: packed dirt for unpaved residential side strips ──
TEX_DIRT       = f"{RZ}/prontera_fields/T_Prt_Path_A"
# ── SLOPE: warm cliff for the moat bank ──
TEX_ROCK       = f"{RZ}/prontera_fields/T_Prt_Cliff_A"

# ── v18 NEW: 3 paint-driven pavement textures ──
# Multiple candidate paths so the MI works whether the user drags PNGs into
# Content/.../Ground/ or somewhere else. First match wins.
TEX_BRICK_CANDIDATES = [
    f"{GR}/T_Brick_RO",
    f"{G2K}/T_Brick_RO_2k",
    f"{GS}/T_Brick_RO_2k",
    "/Game/SabriMMO/Textures/Environment/T_Brick_RO",
]
TEX_COBBLE_CANDIDATES = [
    f"{GS}/T_Cobble_RO_2k",
    f"{G2K}/T_Cobble_RO_2k",
    f"{GR}/T_Cobble_RO",
    "/Game/SabriMMO/Textures/Environment/T_Cobble_RO",
]
TEX_PEBBLE_CANDIDATES = [
    f"{GR}/T_Pebble_RO",
    f"{G2K}/T_Pebble_RO_2k",
    f"{GS}/T_Pebble_RO_2k",
    "/Game/SabriMMO/Textures/Environment/T_Pebble_RO",
]

# Optional normal/AO maps for v17 4-slot blend. Skipped silently if absent.
N_ROOT = "/Game/SabriMMO/Textures/Environment/Ground/Normals"
N2K    = "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals"
A_ROOT = "/Game/SabriMMO/Textures/Environment/Ground/AO"
TEX_GRASS_WARM_N = f"{N2K}/T_Cobble_RO_2k_Normal"
TEX_GRASS_COOL_N = f"{N_ROOT}/T_Urban_Cobble_A_N"
TEX_DIRT_N       = f"{N_ROOT}/T_Prt_Path_A_N"
TEX_ROCK_N       = f"{N_ROOT}/T_Prt_Cliff_A_N"
TEX_GRASS_AO     = f"{A_ROOT}/T_Cobble_RO_AO"

# ============================================================
# PARAMETERS — pastoral capital town, paint-driven pavements
# Per Pryth_Build_Plan.md Section 5.1
# ============================================================
SCALAR_PARAMS = {
    # Color processing — stone is muted vs lush grass
    "SaturationMult":        0.92,
    "BrightnessOffset":      0.02,
    "ContrastBoost":         1.00,

    # Blending — keep some variation between cobble A and cobble B (default unpainted)
    "GrassVariantBalance":   0.50,
    "GrassNoiseScale":       0.004,
    "MacroNoiseScale":       0.0008,
    "DirtOnSlopes":          0.10,    # paved → cliff is sharp
    "DirtAmount":            0.20,    # paved areas wear with foot traffic

    # Slope — moat bank kick-in
    "SlopeThreshold":        0.60,
    "SlopeTransitionWidth":  0.15,
    "SlopeNoiseAmount":      0.06,
    "SlopeNoiseFreq":        0.012,

    # Surface — cobblestone is rougher than grass and bumpier
    "Roughness":             0.92,
    "NormalStrength":        0.85,
    "AOStrength":            0.55,
    "UVDistortStrength":     45.0,

    # Tile sizes (existing 4-slot)
    "GrassWarmTileSize":     2500.0,  # cobble (default unpainted) — primary
    "GrassCoolTileSize":     3247.0,  # cobble variation — irrational ratio
    "DirtTileSize":          3347.0,  # dirt accent
    "RockTileSize":          2891.0,  # cliff

    # v18 NEW: pavement tile sizes (paint-driven)
    "BrickTileSize":         3000.0,  # rectangular bricks
    "CobbleTileSize":        2500.0,  # rounded cobble
    "PebbleTileSize":        2000.0,  # small pebbles
}

VECTOR_PARAMS = {
    # Slightly warm neutral — stone reads less yellow than grass
    "WarmthTint": unreal.LinearColor(1.00, 0.98, 0.93, 1.0),
}


def first_existing(candidate_paths):
    """Return the first asset path that exists, or None."""
    for p in candidate_paths:
        if eal.does_asset_exist(p):
            return p
    return None


# ============================================================
# CREATE / UPDATE
# ============================================================
def main():
    unreal.log("-" * 60)
    unreal.log(f"create_mi_paved_town -- building {MI_NAME}")
    unreal.log("-" * 60)

    # Verify parent exists
    parent = eal.load_asset(PARENT_PATH)
    if not parent:
        unreal.log_error(f"Parent material not found: {PARENT_PATH}")
        unreal.log_error("Run create_landscape_ro_18.py first to build M_Landscape_RO_18.")
        return False

    # Resolve pavement textures (each tries multiple candidate paths)
    tex_brick_path  = first_existing(TEX_BRICK_CANDIDATES)
    tex_cobble_path = first_existing(TEX_COBBLE_CANDIDATES)
    tex_pebble_path = first_existing(TEX_PEBBLE_CANDIDATES)

    TEXTURE_PARAMS = {
        # v17 base 4-slot blend (default unpainted = cobblestone)
        "GrassWarmTexture":  TEX_GRASS_WARM,
        "GrassCoolTexture":  TEX_GRASS_COOL,
        "DirtTexture":       TEX_DIRT,
        "RockTexture":       TEX_ROCK,
        # v18 paint-driven pavement layers
        "BrickTexture":      tex_brick_path  or "",
        "CobbleTexture":     tex_cobble_path or "",
        "PebbleTexture":     tex_pebble_path or "",
        # Optional normals + AO
        "GrassWarmNormal":   TEX_GRASS_WARM_N,
        "GrassCoolNormal":   TEX_GRASS_COOL_N,
        "DirtNormal":        TEX_DIRT_N,
        "RockNormal":        TEX_ROCK_N,
        "GrassAO":           TEX_GRASS_AO,
    }

    # Ensure folder exists
    eal.make_directory(MI_FOLDER)

    # Create or load MI
    if eal.does_asset_exist(MI_FULL):
        unreal.log(f"  MI already exists -- updating parameters in place: {MI_FULL}")
        mi = eal.load_asset(MI_FULL)
    else:
        unreal.log(f"  Creating new MI: {MI_FULL}")
        factory = unreal.MaterialInstanceConstantFactoryNew()
        mi = ATL.create_asset(
            asset_name=MI_NAME,
            package_path=MI_FOLDER,
            asset_class=unreal.MaterialInstanceConstant,
            factory=factory,
        )
        if not mi:
            unreal.log_error(f"  Failed to create MI at {MI_FULL}")
            return False

    # Set parent
    mi.set_editor_property("parent", parent)
    unreal.log(f"  Parent set: {PARENT_PATH}")

    # Apply scalar parameters
    unreal.log("  Scalar parameters:")
    for name, value in SCALAR_PARAMS.items():
        ok = mel.set_material_instance_scalar_parameter_value(mi, name, float(value))
        flag = "[OK]" if ok else "[MISS]"
        unreal.log(f"    {flag} {name:22s} = {value}")

    # Apply vector parameters
    unreal.log("  Vector parameters:")
    for name, value in VECTOR_PARAMS.items():
        ok = mel.set_material_instance_vector_parameter_value(mi, name, value)
        flag = "[OK]" if ok else "[MISS]"
        unreal.log(f"    {flag} {name:22s} = ({value.r:.3f}, {value.g:.3f}, {value.b:.3f})")

    # Apply texture parameters (skip silently if optional texture missing)
    unreal.log("  Texture parameters:")
    for name, path in TEXTURE_PARAMS.items():
        is_optional = (
            name.endswith("Normal") or name == "GrassAO"
        )
        is_pavement = name in ("BrickTexture", "CobbleTexture", "PebbleTexture")

        if not path or not eal.does_asset_exist(path):
            level = "    [-]" if is_optional or is_pavement else "    [FAIL]"
            note  = "skipped (asset missing)" if is_optional else "MISSING -- texture not yet imported"
            unreal.log(f"{level} {name:22s} = {path or '(none)'}  [{note}]")
            if is_pavement:
                unreal.log(f"      Generate via _tools/generate_pryth_pavements.py + import to UE5.")
            elif not is_optional:
                unreal.log_warning(f"      Required texture missing -- MI falls back to parent default")
            continue

        tex = eal.load_asset(path)
        if not tex:
            unreal.log_warning(f"    [FAIL] {name:22s} = {path}  [load failed]")
            continue

        ok = mel.set_material_instance_texture_parameter_value(mi, name, tex)
        flag = "[OK]" if ok else "[MISS]"
        unreal.log(f"    {flag} {name:22s} = {path}")

    # Save
    eal.save_asset(MI_FULL, only_if_is_dirty=False)
    unreal.log("-" * 60)
    unreal.log(f"DONE: {MI_FULL}")
    unreal.log("-" * 60)
    unreal.log("")
    unreal.log("Apply to your Landscape:")
    unreal.log("  1. Open L_Pryth")
    unreal.log("  2. Select Landscape Actor")
    unreal.log("  3. Drag this MI from Content Browser onto the Landscape")
    unreal.log(f"     ({MI_FULL})")
    unreal.log("  4. Shift+2 -> Landscape Mode -> Paint tab")
    unreal.log("  5. Click + next to each layer -> Create Layer Info -> save:")
    unreal.log("       Brick (paint plaza)")
    unreal.log("       Cobble (paint streets and main areas)")
    unreal.log("       Pebble (paint side paths)")
    unreal.log("       GrassDense (paint residential side-yards for sprite scatter)")
    unreal.log("  6. Brush-paint each district as desired ([ and ] to resize brush)")
    unreal.log("  7. Build > Build Navigation after sculpting")
    return True


if __name__ == "__main__" or True:
    main()
