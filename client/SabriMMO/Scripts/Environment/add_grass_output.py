# add_grass_output.py
# Adds a LandscapeGrassOutput node to M_Landscape_RO_14
# and connects it to the grass types we created.
#
# Run AFTER create_landscape_material.py (v14) and setup_landscape_grass.py

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

MAT_PATH = "/Game/SabriMMO/Materials/Environment/M_Landscape_RO_14"
GRASS_PATH = "/Game/SabriMMO/Environment/Grass"

# Load the material
mat = unreal.load_asset(MAT_PATH)
if not mat:
    unreal.log_error(f"Material not found: {MAT_PATH}")
    raise RuntimeError("Missing material")

unreal.log(f"Adding Grass Output to {MAT_PATH}...")

# Create the LandscapeGrassOutput node
try:
    grass_output = mel.create_material_expression(
        mat, unreal.MaterialExpressionLandscapeGrassOutput, 1200, 600)
    unreal.log("  LandscapeGrassOutput node created")
except Exception as e:
    unreal.log_error(f"  Failed to create LandscapeGrassOutput: {e}")
    raise

# Load grass type assets
grass_types = []
gt_names = ["GT_GrassShort", "GT_GrassTall", "GT_Flowers", "GT_Pebbles"]
for gt_name in gt_names:
    gt_path = f"{GRASS_PATH}/{gt_name}"
    if eal.does_asset_exist(gt_path):
        gt = unreal.load_asset(gt_path)
        grass_types.append((gt_name, gt))
        unreal.log(f"  Loaded: {gt_name}")
    else:
        unreal.log_warning(f"  Not found: {gt_path}")

# Configure the grass output node's GrassTypes array
# Each entry needs a GrassType reference and gets an input pin
try:
    # Get the current grass types array
    current_types = grass_output.get_editor_property("grass_types")

    for gt_name, gt_asset in grass_types:
        entry = unreal.GrassInput()
        entry.set_editor_property("grass_type", gt_asset)
        entry.set_editor_property("name", gt_name)
        current_types.append(entry)

    grass_output.set_editor_property("grass_types", current_types)
    unreal.log(f"  Assigned {len(grass_types)} grass types to output node")
except Exception as e:
    unreal.log_warning(f"  Could not set grass_types array: {e}")
    unreal.log("  Trying alternative approach...")

    # Alternative: set via the node's properties directly
    try:
        for i, (gt_name, gt_asset) in enumerate(grass_types):
            grass_output.set_editor_property(f"grass_type_{i}", gt_asset)
    except Exception as e2:
        unreal.log_warning(f"  Alternative also failed: {e2}")

# Create a constant 1.0 to feed all grass inputs (grass everywhere on flat)
grass_mask = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 1100, 600)
grass_mask.set_editor_property("r", 1.0)

# Try to connect the constant to each grass input pin
for i, (gt_name, _) in enumerate(grass_types):
    try:
        mel.connect_material_expressions(grass_mask, "", grass_output, gt_name)
        unreal.log(f"  Connected mask to {gt_name}")
    except Exception:
        try:
            # Try numeric pin name
            mel.connect_material_expressions(grass_mask, "", grass_output, str(i))
        except Exception:
            try:
                # Try empty for first pin
                if i == 0:
                    mel.connect_material_expressions(grass_mask, "", grass_output, "")
            except Exception:
                pass

# Recompile and save
mel.recompile_material(mat)
eal.save_asset(MAT_PATH)

unreal.log(f"\n{'='*60}")
unreal.log(f"  Grass Output node added to {MAT_PATH}")
unreal.log(f"  If pin connections failed, open Material Editor and:")
unreal.log(f"  - Find the Grass Output node (bottom-right area)")
unreal.log(f"  - Connect the Constant 1.0 node to each grass input")
unreal.log(f"  - Or connect the slope_clamp node for slope-based grass")
unreal.log(f"{'='*60}")
