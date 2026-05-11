"""Quick check: srgb setting on poring (working) vs ambernite (broken).

Run from UE5 Editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/check_srgb.py"
"""
import unreal

eal = unreal.EditorAssetLibrary

ASSETS = [
    "/Game/SabriMMO/Sprites/Atlases/Body/enemies/poring/poring_idle.poring_idle",
    "/Game/SabriMMO/Sprites/Atlases/Body/enemies/fabre/fabre_idle.fabre_idle",
    "/Game/SabriMMO/Sprites/Atlases/Body/enemies/ambernite/ambernite_idle_wobble.ambernite_idle_wobble",
    "/Game/SabriMMO/Sprites/Atlases/Body/enemies/parasite/parasite_idle_sway.parasite_idle_sway",
    "/Game/SabriMMO/Sprites/Atlases/Body/enemies/plasma/plasma_idle_bounce.plasma_idle_bounce",
    "/Game/SabriMMO/Sprites/Atlases/Body/enemies/rafflesia/rafflesia_idle_sway.rafflesia_idle_sway",
]

unreal.log("=" * 60)
unreal.log("Comparing canonical settings between working and new enemies")
unreal.log("=" * 60)

for asset_path in ASSETS:
    clean = asset_path.split(".")[0]
    if not eal.does_asset_exist(clean):
        unreal.log(f"  [MISSING] {clean}")
        continue
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        unreal.log(f"  [NOT TEX] {clean}")
        continue

    name = clean.split("/")[-1]
    srgb = tex.get_editor_property("srgb")
    comp = tex.get_editor_property("compression_settings")
    mip = tex.get_editor_property("mip_gen_settings")
    never = tex.get_editor_property("never_stream")
    maxsize = tex.get_editor_property("max_texture_size")
    alpha = tex.get_editor_property("do_scale_mips_for_alpha_coverage")
    new_mip = tex.get_editor_property("use_new_mip_filter")
    lod = tex.get_editor_property("lod_group")

    unreal.log(f"")
    unreal.log(f"  {name}:")
    unreal.log(f"    srgb={srgb}  compression={comp}  mip_gen={mip}")
    unreal.log(f"    never_stream={never}  max_size={maxsize}  alpha_cov={alpha}  new_mip={new_mip}")
    unreal.log(f"    lod_group={lod}")
