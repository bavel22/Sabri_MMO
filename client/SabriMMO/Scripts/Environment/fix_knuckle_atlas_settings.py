"""Applies the canonical sprite atlas settings (2026-04-27) to all 17 female
knuckle atlases.

Run in UE5 editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/fix_knuckle_atlas_settings.py"

Sets, on each weapon_12_*.uasset texture in /Game/SabriMMO/Sprites/Atlases/Weapon/knuckle/female/:
    - filter                            = TF_NEAREST
    - compression_settings              = TC_BC7
    - mip_gen_settings                  = TMGS_SIMPLE_AVERAGE
    - use_new_mip_filter                = True
    - do_scale_mips_for_alpha_coverage  = True
    - alpha_coverage_thresholds         = (0, 0, 0, 0.5)
    - max_texture_size                  = 0      (no cap)
    - never_stream                      = False  (lets LODBias free VRAM)
    - lod_group                         = TEXTUREGROUP_UI
    - srgb                              = True  (CRITICAL — sprite material
                                                  samples Linear Color)

See memory/feedback-sprite-texture-group-ui.md for the full rule set.
Saves each modified asset.
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary

SRC_DIR = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/knuckle/female"
GAME_DEST = "/Game/SabriMMO/Sprites/Atlases/Weapon/knuckle/female"


def apply_sprite_settings(asset_path):
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return False, "not a Texture2D"

    # Canonical sprite settings (UPDATED 2026-04-27).
    # See memory `feedback-sprite-texture-group-ui.md` for full reference.
    changes = []
    if tex.get_editor_property("filter") != unreal.TextureFilter.TF_NEAREST:
        tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        changes.append("filter")
    if tex.get_editor_property("compression_settings") != unreal.TextureCompressionSettings.TC_BC7:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
        changes.append("compression")
    if tex.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE:
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
        changes.append("mip_gen")
    if not tex.get_editor_property("use_new_mip_filter"):
        tex.set_editor_property("use_new_mip_filter", True)
        changes.append("use_new_mip_filter")
    if not tex.get_editor_property("do_scale_mips_for_alpha_coverage"):
        tex.set_editor_property("do_scale_mips_for_alpha_coverage", True)
        changes.append("alpha_coverage_on")
    cur_thresholds = tex.get_editor_property("alpha_coverage_thresholds")
    if not cur_thresholds or abs(cur_thresholds.w - 0.5) > 0.001:
        tex.set_editor_property("alpha_coverage_thresholds",
            unreal.Vector4(0.0, 0.0, 0.0, 0.5))
        changes.append("alpha_coverage_thresholds")
    if tex.get_editor_property("max_texture_size") != 0:
        tex.set_editor_property("max_texture_size", 0)
        changes.append("max_texture_size")
    if tex.get_editor_property("never_stream"):
        tex.set_editor_property("never_stream", False)
        changes.append("never_stream")
    if tex.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        changes.append("lod_group")
    if not tex.get_editor_property("srgb"):
        tex.set_editor_property("srgb", True)
        changes.append("srgb")

    if changes:
        eal.save_asset(clean)
        return True, ", ".join(changes)
    return True, "already correct"


pngs = sorted(glob.glob(os.path.join(SRC_DIR, "weapon_12_*.png")))
unreal.log(f"[knuckle/female] Patching {len(pngs)} textures...")

patched = 0
already_ok = 0
missing = 0

for png in pngs:
    name = os.path.splitext(os.path.basename(png))[0]
    asset_path = f"{GAME_DEST}/{name}"
    if not eal.does_asset_exist(asset_path):
        unreal.log_warning(f"  [missing] {name}")
        missing += 1
        continue
    ok, info = apply_sprite_settings(asset_path)
    if not ok:
        unreal.log_warning(f"  [error] {name}: {info}")
        continue
    if info == "already correct":
        already_ok += 1
    else:
        patched += 1
        unreal.log(f"  [fixed] {name}: {info}")

unreal.log(f"\n=== Done — patched={patched}, already_correct={already_ok}, missing={missing} ===")
