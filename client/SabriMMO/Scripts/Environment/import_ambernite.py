"""Import the ambernite enemy sprite atlases into UE5.

The ambernite folder has 3 PNGs on disk but no .uasset files yet:
  ambernite_crack_death.png
  ambernite_hit_jiggle.png
  ambernite_idle_wobble.png
(Non-standard animation set — ambernite is a 'blob' preset enemy.)

Run from UE5 Editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_ambernite.py"

Applies the canonical sprite atlas settings (2026-04-27 — see
memory/feedback-sprite-texture-group-ui.md):
    Filter=Nearest, Compression=BC7, MipGen=SimpleAverage, UseNewMipFilter=True,
    DoScaleMipsForAlphaCoverage=True, AlphaCoverageThresholds=(0,0,0,0.5),
    MaxTextureSize=0, NeverStream=False, LODGroup=UI, sRGB=True (body sprite material samples as Color/sRGB; srgb=False makes the sprite invisible).
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary

DISK_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/ambernite"
GAME_DIR = "/Game/SabriMMO/Sprites/Atlases/Body/enemies/ambernite"


def apply_canonical_settings(asset_path):
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
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


def import_one(png_path):
    name = os.path.splitext(os.path.basename(png_path))[0]
    asset_path = f"{GAME_DIR}/{name}"
    clean = asset_path.split(".")[0]

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", GAME_DIR)
    task.set_editor_property("filename", png_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if not eal.does_asset_exist(clean):
        return False
    apply_canonical_settings(asset_path)
    return True


def main():
    pngs = sorted(glob.glob(os.path.join(DISK_DIR, "*.png")))
    unreal.log(f"=== Importing ambernite ({len(pngs)} atlases) ===")
    if not pngs:
        unreal.log_warning(f"No PNGs in {DISK_DIR}")
        return

    ok = 0
    fail = 0
    for png in pngs:
        if import_one(png):
            ok += 1
            unreal.log(f"  [imported] {os.path.basename(png)}")
        else:
            fail += 1
            unreal.log_warning(f"  [failed]   {os.path.basename(png)}")

    unreal.log(f"--- Done: imported={ok} failed={fail} ---")
    unreal.log(f"Wait for 'Compiling Textures: 0' before launching the game.")


main()
