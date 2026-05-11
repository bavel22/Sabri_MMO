"""Import the 4 enemy folders that have PNGs on disk but no .uasset files yet:
    ambernite   (3 PNGs)
    parasite    (4 PNGs)
    plasma      (5 PNGs)
    rafflesia   (4 PNGs)

Run from UE5 Editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_missing_4.py"

Applies the canonical sprite atlas settings (2026-04-27 — see
memory/feedback-sprite-texture-group-ui.md):
    Compression                       = TC_BC7
    Filter                            = TF_NEAREST
    Mip Gen Settings                  = TMGS_SIMPLE_AVERAGE
    Use New Mip Filter                = True
    Do Scale Mips For Alpha Coverage  = True
    Alpha Coverage Thresholds         = (0, 0, 0, 0.5)
    Maximum Texture Size              = 0     (no cap)
    Never Stream                      = False (lets the runtime LODBias slider free VRAM)
    LOD Group                         = TEXTUREGROUP_UI
    sRGB                              = True

Required for the Sprite Quality slider, ZonePreloadSubsystem, and Path C deferred
equipment swap to all behave correctly on these new atlases.
"""
import unreal
import os
import glob
import time

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

ENEMIES = ["ambernite", "parasite", "plasma", "rafflesia"]


def apply_canonical_settings(asset_path):
    """Apply the canonical Sabri_MMO sprite atlas settings (2026-04-27).

    See memory/feedback-sprite-texture-group-ui.md for the rule set.
    """
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


def verify_settings(asset_path):
    """Re-load the asset and confirm every canonical setting stuck.
    Returns (ok: bool, problem: str)."""
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False, "not_a_texture2d"

    if tex.get_editor_property("compression_settings") != unreal.TextureCompressionSettings.TC_BC7:
        return False, "compression != BC7"
    if tex.get_editor_property("filter") != unreal.TextureFilter.TF_NEAREST:
        return False, "filter != Nearest"
    if tex.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE:
        return False, "mip_gen != SimpleAverage"
    if not tex.get_editor_property("use_new_mip_filter"):
        return False, "use_new_mip_filter off"
    if not tex.get_editor_property("do_scale_mips_for_alpha_coverage"):
        return False, "alpha_coverage off"
    thresh = tex.get_editor_property("alpha_coverage_thresholds")
    if not thresh or abs(thresh.w - 0.5) > 0.01:
        return False, f"alpha_threshold.w != 0.5 (got {thresh.w if thresh else 'None'})"
    if tex.get_editor_property("max_texture_size") != 0:
        return False, f"max_texture_size != 0 (got {tex.get_editor_property('max_texture_size')})"
    if tex.get_editor_property("never_stream"):
        return False, "never_stream is True"
    if tex.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        return False, "lod_group != TEXTUREGROUP_UI"
    if not tex.get_editor_property("srgb"):
        return False, "srgb is False (must be True — body sprite material's sampler is Color/sRGB)"
    return True, "ok"


def import_one(png_path, dest_dir, asset_path):
    clean = asset_path.split(".")[0]

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("filename", png_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if not eal.does_asset_exist(clean):
        return "import_failed", "asset not created"

    if not apply_canonical_settings(asset_path):
        return "settings_failed", "could not apply settings"

    ok, problem = verify_settings(asset_path)
    if not ok:
        return "verify_failed", problem
    return "ok", "ok"


def main():
    grand_total = {"ok": 0, "import_failed": 0, "settings_failed": 0, "verify_failed": 0}
    start = time.time()

    unreal.log("=" * 60)
    unreal.log(f"Importing {len(ENEMIES)} missing enemy folders with canonical settings")
    unreal.log("=" * 60)

    for enemy in ENEMIES:
        disk_dir = os.path.join(ATLAS_ROOT, enemy)
        game_dir = f"{GAME_ROOT}/{enemy}"
        pngs = sorted(glob.glob(os.path.join(disk_dir, "*.png")))

        unreal.log(f"")
        unreal.log(f"--- {enemy} ({len(pngs)} PNGs) ---")
        if not pngs:
            unreal.log_warning(f"  no PNGs in {disk_dir}")
            continue

        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            asset_path = f"{game_dir}/{name}"
            result, detail = import_one(png, game_dir, asset_path)
            grand_total[result] += 1
            tag = "[OK]" if result == "ok" else f"[{result.upper()}]"
            line = f"  {tag} {name}"
            if result != "ok":
                line += f"  ({detail})"
                unreal.log_warning(line)
            else:
                unreal.log(line)

    elapsed = time.time() - start
    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Done in {elapsed:.1f}s")
    unreal.log(f"  ok               = {grand_total['ok']}")
    unreal.log(f"  import_failed    = {grand_total['import_failed']}")
    unreal.log(f"  settings_failed  = {grand_total['settings_failed']}")
    unreal.log(f"  verify_failed    = {grand_total['verify_failed']}")
    unreal.log("=" * 60)
    unreal.log("Wait for 'Compiling Textures: 0' before launching the game.")


main()
