# create_decal_instances.py
# Imports 40 decal textures and creates Material Instances from the
# existing M_Decal_* parent materials.
#
# Run: exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_decal_instances.py").read())

import unreal
import os
import glob

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

SRC = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/Decals"
TEX_DEST = "/Game/SabriMMO/Textures/Environment/Decals"
MI_DEST = "/Game/SabriMMO/Materials/Environment/Decals/Instances"
PARENT_PATH = "/Game/SabriMMO/Materials/Environment/Decals"

eal.make_directory(MI_DEST)

# Map texture name prefixes to parent materials and default settings
DECAL_MAP = {
    "LeafScatter":   ("M_Decal_Moss",    (0.45, 0.35, 0.20), 0.50),
    "FlowerPatch":   ("M_Decal_Moss",    (0.50, 0.35, 0.45), 0.45),
    "GrassTuft":     ("M_Decal_Moss",    (0.20, 0.35, 0.15), 0.45),
    "MossPatch":     ("M_Decal_Moss",    (0.15, 0.28, 0.12), 0.50),
    "Mushroom":      ("M_Decal_Dirt",    (0.40, 0.30, 0.20), 0.45),
    "RootTendril":   ("M_Decal_Dirt",    (0.30, 0.22, 0.15), 0.40),
    "StoneCrack":    ("M_Decal_Cracks",  (0.20, 0.20, 0.22), 0.45),
    "DirtSmudge":    ("M_Decal_Dirt",    (0.38, 0.30, 0.20), 0.45),
    "ScuffMark":     ("M_Decal_Cracks",  (0.30, 0.28, 0.25), 0.35),
    "Puddle":        ("M_Decal_DarkStain", (0.15, 0.18, 0.22), 0.50),
    "DampPatch":     ("M_Decal_DarkStain", (0.20, 0.22, 0.25), 0.40),
    "SandRipple":    ("M_Decal_Path",    (0.50, 0.42, 0.30), 0.35),
    "SandDrift":     ("M_Decal_Path",    (0.55, 0.45, 0.32), 0.40),
    "CrackedEarth":  ("M_Decal_Cracks",  (0.35, 0.28, 0.18), 0.50),
    "BoneScatter":   ("M_Decal_Dirt",    (0.60, 0.55, 0.48), 0.40),
    "IcePatch":      ("M_Decal_Path",    (0.70, 0.78, 0.85), 0.40),
    "FrostPattern":  ("M_Decal_Path",    (0.75, 0.82, 0.90), 0.35),
    "SnowDrift":     ("M_Decal_Path",    (0.85, 0.88, 0.92), 0.45),
    "LavaCrack":     ("M_Decal_DarkStain", (0.80, 0.30, 0.05), 0.70),
    "ScorchMark":    ("M_Decal_DarkStain", (0.12, 0.10, 0.08), 0.55),
    "SulfurDeposit": ("M_Decal_Dirt",    (0.65, 0.60, 0.15), 0.45),
    "BloodStain":    ("M_Decal_DarkStain", (0.35, 0.08, 0.06), 0.55),
    "ClawMarks":     ("M_Decal_Cracks",  (0.15, 0.12, 0.12), 0.50),
    "SlimeTrail":    ("M_Decal_Moss",    (0.20, 0.40, 0.15), 0.50),
    "CherryPetals":  ("M_Decal_Moss",    (0.75, 0.50, 0.55), 0.40),
    "BambooLeaves":  ("M_Decal_Moss",    (0.30, 0.45, 0.20), 0.40),
    "PineCones":     ("M_Decal_Dirt",    (0.40, 0.30, 0.18), 0.40),
    "BirchLeaves":   ("M_Decal_Moss",    (0.55, 0.50, 0.25), 0.45),
    "RubbleScatter": ("M_Decal_Cracks",  (0.40, 0.38, 0.35), 0.45),
    "GravelStrip":   ("M_Decal_Path",    (0.45, 0.40, 0.35), 0.40),
    "ErosionLines":  ("M_Decal_Cracks",  (0.25, 0.22, 0.18), 0.40),
    "DappledLight":  ("M_Decal_Path",    (0.90, 0.85, 0.70), 0.30),
    "ShadowPool":    ("M_Decal_DarkStain", (0.08, 0.08, 0.10), 0.40),
}

# Step 1: Import textures
unreal.log("=== Importing decal textures ===")
pngs = sorted(glob.glob(os.path.join(SRC, "*.png")))
tasks = []
for png in pngs:
    name = os.path.splitext(os.path.basename(png))[0]
    if not eal.does_asset_exist(f"{TEX_DEST}/{name}"):
        t = unreal.AssetImportTask()
        t.set_editor_property("automated", True)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("filename", png)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
unreal.log(f"  Imported {len(tasks)} textures")

# Step 2: Create Material Instances
unreal.log("\n=== Creating decal Material Instances ===")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()
created = 0

# Get all imported decal textures
all_tex = eal.list_assets(TEX_DEST, recursive=False)

for tex_asset in sorted(all_tex):
    tex_path = tex_asset.split(".")[0]
    tex_name = tex_path.split("/")[-1]

    # Skip non-decal textures
    if not tex_name.startswith("T_Decal_"):
        continue

    # Find matching parent material
    parent_name = None
    tint = (0.30, 0.25, 0.20)
    opacity = 0.45
    for prefix, (mat_name, t, o) in DECAL_MAP.items():
        if prefix in tex_name:
            parent_name = mat_name
            tint = t
            opacity = o
            break

    if not parent_name:
        parent_name = "M_Decal_Dirt"

    parent_full = f"{PARENT_PATH}/{parent_name}"
    parent = unreal.load_asset(parent_full)
    if not parent:
        continue

    mi_name = f"MI_{tex_name}"
    mi_full = f"{MI_DEST}/{mi_name}"

    if eal.does_asset_exist(mi_full):
        continue

    mi = asset_tools.create_asset(mi_name, MI_DEST,
        unreal.MaterialInstanceConstant, mi_factory)
    if not mi:
        continue

    mi.set_editor_property("parent", parent)

    # Assign texture
    tex = unreal.load_asset(tex_path)
    if tex:
        mel.set_material_instance_texture_parameter_value(
            mi, "DecalTexture", tex,
            unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Set tint and opacity
    mel.set_material_instance_vector_parameter_value(
        mi, "DecalTint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0),
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
    mel.set_material_instance_scalar_parameter_value(
        mi, "OpacityStrength", opacity,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    eal.save_asset(mi_full)
    created += 1

unreal.log(f"\n{'='*60}")
unreal.log(f"  DONE: {created} decal Material Instances created")
unreal.log(f"  Browse: {MI_DEST}")
unreal.log(f"  Drag any MI_T_Decal_* into viewport to place")
unreal.log(f"{'='*60}")
