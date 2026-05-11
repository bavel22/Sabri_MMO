# reimport_hermit_plant.py
# Force re-import the hermit_plant atlases (overwriting existing .uasset files)
# and apply sprite texture settings.
#
# Use this after re-rendering hermit_plant with updated lighting — the main
# import_enemy_sprites.py skips already-imported PNGs, so changes wouldn't apply.
#
# Settings applied (canonical 2026-04-27 — see memory/feedback-sprite-texture-group-ui.md):
#   Filter:                          Nearest
#   Compression:                     BC7
#   Mip Gen:                         SimpleAverage
#   Use New Mip Filter:              True
#   Do Scale Mips For Alpha Cov.:    True
#   Alpha Coverage Thresholds:       (0, 0, 0, 0.5)
#   Maximum Texture Size:            0   (no cap)
#   Never Stream:                    False
#   Texture Group:                   UI
#   sRGB:                            True
#
# Run from UE5 Editor Python console:
#   py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/reimport_hermit_plant.py"

import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
SOURCE = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/hermit_plant"
DEST = "/Game/SabriMMO/Sprites/Atlases/Body/enemies/hermit_plant"


def apply_sprite_settings(asset_path):
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    tex.set_editor_property("use_new_mip_filter", True)
    tex.set_editor_property("do_scale_mips_for_alpha_coverage", True)
    tex.set_editor_property("alpha_coverage_thresholds",
        unreal.Vector4(0.0, 0.0, 0.0, 0.5))
    tex.set_editor_property("max_texture_size", 0)
    tex.set_editor_property("never_stream", False)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("srgb", True)
    eal.save_asset(clean)
    return True


pngs = sorted(glob.glob(os.path.join(SOURCE, "*.png")))
if not pngs:
    unreal.log_error(f"No PNGs found in {SOURCE}")
else:
    unreal.log(f"Force re-importing {len(pngs)} hermit_plant atlases...")
    tasks = []
    for png in pngs:
        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("filename", png)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    # Apply settings to each imported atlas
    configured = 0
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{DEST}/{name}"
        if eal.does_asset_exist(asset_path) and apply_sprite_settings(asset_path):
            configured += 1

    unreal.log(f"\n{'=' * 50}")
    unreal.log(f"  Re-imported: {len(pngs)}")
    unreal.log(f"  Configured:  {configured}")
    unreal.log(f"{'=' * 50}")
