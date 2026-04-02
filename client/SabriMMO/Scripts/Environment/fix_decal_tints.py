# fix_decal_tints.py
# Fixes all decal Material Instances to use neutral white tint
# so the textures show their natural colors instead of being washed out.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MI_PATH = "/Game/SabriMMO/Materials/Environment/Decals/Instances"

assets = eal.list_assets(MI_PATH, recursive=False)
fixed = 0

# Near-white tint — lets texture color show through naturally
NEUTRAL_TINT = unreal.LinearColor(0.9, 0.88, 0.85, 1.0)

# Original 15 names to skip
ORIGINAL_15 = {
    "MI_Decal_Dirt_01", "MI_Decal_Dirt_02", "MI_Decal_Dirt_03", "MI_Decal_Dirt_04", "MI_Decal_Dirt_05",
    "MI_Decal_Crack_01", "MI_Decal_Crack_02", "MI_Decal_Crack_03",
    "MI_Decal_Moss_01", "MI_Decal_Moss_02", "MI_Decal_Moss_03",
    "MI_Decal_Path_01", "MI_Decal_Path_02",
    "MI_Decal_Stain_01", "MI_Decal_Stain_02",
}

for asset in sorted(assets):
    clean = asset.split(".")[0]
    mi = unreal.load_asset(clean)
    if not mi or not isinstance(mi, unreal.MaterialInstanceConstant):
        continue

    name = clean.split("/")[-1]

    # Skip the original 15 — they already look good
    if name in ORIGINAL_15:
        continue

    # Set tint to near-neutral (very slight warm cast like original 15)
    mel.set_material_instance_vector_parameter_value(
        mi, "DecalTint", NEUTRAL_TINT,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Lower opacity slightly for subtlety
    mel.set_material_instance_scalar_parameter_value(
        mi, "OpacityStrength", 0.4,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Softer edges
    mel.set_material_instance_scalar_parameter_value(
        mi, "EdgeSoftness", 1.5,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(clean)
    fixed += 1

unreal.log(f"Fixed {fixed} decal instances — neutral tint, 0.4 opacity, soft edges")
