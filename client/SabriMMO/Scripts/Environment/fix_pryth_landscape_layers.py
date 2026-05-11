# fix_pryth_landscape_layers.py
# Forces UE5 to re-detect paint layers on the Landscape Actor.
# UE5 caches the layer list at material assignment time. After rebuilding
# the master material with new paint layers, the Landscape doesn't auto-refresh.
# This script: clears the material, then re-applies it, forcing re-detection.
#
# Run in UE5 editor (Tools > Execute Python Script, or paste into Python Console):
#   exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\fix_pryth_landscape_layers.py").read())

import unreal

MI_PATH = "/Game/SabriMMO/Materials/Environment/v3/MI_Landscape_PavedTown_v1"

eal = unreal.EditorAssetLibrary
els = unreal.EditorLevelLibrary

print("=" * 70)
print("PRYTH LANDSCAPE LAYER REFRESH")
print("=" * 70)

# Load the MI
mi = eal.load_asset(MI_PATH)
if not mi:
    print(f"ERROR: Could not load {MI_PATH}")
else:
    print(f"Loaded MI: {mi.get_name()}")

# Find all Landscape Actors in the current level
all_actors = els.get_all_level_actors()
landscapes = []
for a in all_actors:
    cls_name = a.__class__.__name__
    if cls_name in ("Landscape", "LandscapeProxy", "LandscapeStreamingProxy"):
        landscapes.append(a)

if not landscapes:
    print("ERROR: No Landscape Actor found in current level.")
    print("Make sure your level with the Landscape is open in the editor.")
else:
    print(f"\nFound {len(landscapes)} Landscape actor(s):")
    for ls in landscapes:
        print(f"  - {ls.get_actor_label()} ({ls.__class__.__name__})")

    print("\n--- Refresh sequence ---")
    for ls in landscapes:
        label = ls.get_actor_label()
        print(f"\n[{label}] Starting refresh...")

        # Step 1: Clear the material (sets to None)
        try:
            ls.set_editor_property("landscape_material", None)
            print(f"[{label}]   Cleared landscape_material -> None")
        except Exception as e:
            print(f"[{label}]   Failed to clear material: {e}")
            continue

        # Step 2: Force a tick / refresh by re-querying
        try:
            ls.modify()
            print(f"[{label}]   modify() called")
        except Exception as e:
            # Non-critical
            pass

        # Step 3: Re-assign the MI
        try:
            ls.set_editor_property("landscape_material", mi)
            print(f"[{label}]   Re-assigned landscape_material -> {MI_PATH}")
        except Exception as e:
            print(f"[{label}]   Failed to re-assign material: {e}")
            continue

        # Step 4: Try to update layer info (LandscapeProxy specific)
        try:
            # Try the various refresh methods that may exist
            for method_name in ("update_components_visibility", "update_target_layers",
                                "register_all_actor_tick_functions",
                                "post_edit_change"):
                if hasattr(ls, method_name):
                    try:
                        getattr(ls, method_name)()
                        print(f"[{label}]   Called {method_name}()")
                    except Exception as e:
                        pass
        except Exception:
            pass

        print(f"[{label}] Refresh complete.")

print("\n" + "=" * 70)
print("DONE.")
print("=" * 70)
print()
print("Now check the Paint UI:")
print("  1. Select the Landscape Actor in the level")
print("  2. Shift+2 -> Landscape Mode -> Paint tab")
print("  3. Target Layers panel should show 6 layers:")
print("       Brick, Cobble, Pebble, GrassDense, FlowerPatch, Debris")
print()
print("If still only 3, save + restart the editor.")
