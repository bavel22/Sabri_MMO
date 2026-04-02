# create_decal_instances_v2.py
# Creates decal Material Instances using ORIGINAL RO textures (not AI-generated).
# These already have the right muted hand-painted colors.
# The decal material uses inverted luminance for opacity,
# so darker areas of the RO texture show up as the decal detail.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

PARENT_PATH = "/Game/SabriMMO/Materials/Environment/Decals"
MI_DEST = "/Game/SabriMMO/Materials/Environment/Decals/RO_Decals"
RO_TEX = "/Game/SabriMMO/Textures/Environment/RO_Original"

eal.make_directory(MI_DEST)

# Parent materials (the 5 we already created with soft radial edges)
PARENTS = {
    "dirt":  f"{PARENT_PATH}/M_Decal_Dirt",
    "crack": f"{PARENT_PATH}/M_Decal_Cracks",
    "moss":  f"{PARENT_PATH}/M_Decal_Moss",
    "path":  f"{PARENT_PATH}/M_Decal_Path",
    "stain": f"{PARENT_PATH}/M_Decal_DarkStain",
}

# Use a selection of RO original textures that work well as decals
# These are darker/more detailed textures from the 608 set
# Paired with appropriate parent material and VERY subtle settings
CONFIGS = []

# Sample every 8th texture from the 608 set — gives ~76 varied decals
# All use dirt parent with neutral tint and low opacity
for i in range(1, 609, 8):
    num = f"{i:03d}"
    tex_path = f"{RO_TEX}/{num}"

    # Cycle through parent materials
    parent_key = ["dirt", "moss", "crack", "path", "stain"][i % 5]

    CONFIGS.append({
        "name": f"MI_RODecal_{num}",
        "parent": parent_key,
        "texture": tex_path,
        # Neutral warm tint — matches the original 15 that look good
        "tint": (0.85, 0.80, 0.72),
        "opacity": 0.35,
    })

unreal.log(f"Creating {len(CONFIGS)} RO Original decal instances...\n")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()
created = 0

for cfg in CONFIGS:
    mi_full = f"{MI_DEST}/{cfg['name']}"

    if eal.does_asset_exist(mi_full):
        continue

    # Check texture exists
    if not eal.does_asset_exist(cfg["texture"]):
        continue

    parent = unreal.load_asset(PARENTS[cfg["parent"]])
    if not parent:
        continue

    mi = asset_tools.create_asset(cfg["name"], MI_DEST,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        continue

    mi.set_editor_property("parent", parent)

    # Assign RO original texture
    tex = unreal.load_asset(cfg["texture"])
    if tex:
        mel.set_material_instance_texture_parameter_value(
            mi, "DecalTexture", tex,
            unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Muted warm tint — same feel as original 15
    mel.set_material_instance_vector_parameter_value(
        mi, "DecalTint",
        unreal.LinearColor(cfg["tint"][0], cfg["tint"][1], cfg["tint"][2], 1.0),
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Low opacity — subtle, blends into terrain
    mel.set_material_instance_scalar_parameter_value(
        mi, "OpacityStrength", cfg["opacity"],
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1

    if created % 20 == 0:
        unreal.log(f"  {created} created...")

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {created} RO Original decal instances")
unreal.log(f"  Browse: {MI_DEST}")
unreal.log(f"  These use the same muted palette as the original 15")
unreal.log(f"{'='*60}")
