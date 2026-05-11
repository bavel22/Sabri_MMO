"""Fix the 4 newly-imported enemy folders by setting srgb=True (canonical).

Background (2026-04-28):
  The body sprite material has a `TextureSampleParameter2D` with Sampler type =
  `Color` (which expects sRGB). When ambernite/parasite/plasma/rafflesia were
  first imported with srgb=False (linear), UE5 logged:
    "Sampler type is Color, should be Linear Color for ..."
  and the texture rendered black/transparent → invisible sprite.

  Canonical rule (memory/feedback-sprite-texture-group-ui.md): all sprite atlas
  textures MUST have `srgb = True`.

Run from UE5 Editor Python console:
    py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/fix_missing_4_srgb.py"
"""
import unreal
import os
import glob

eal = unreal.EditorAssetLibrary
ATLAS_ROOT = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies"
GAME_ROOT = "/Game/SabriMMO/Sprites/Atlases/Body/enemies"

NEW_ENEMIES = ["ambernite", "parasite", "plasma", "rafflesia"]


def fix_one(asset_path):
    clean = asset_path.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        return "not_a_texture"
    if tex.get_editor_property("srgb"):
        return "already_ok"
    tex.set_editor_property("srgb", True)
    eal.save_asset(clean)
    return "fixed"


def main():
    unreal.log("=" * 60)
    unreal.log("Forcing srgb=True on ambernite/parasite/plasma/rafflesia")
    unreal.log("(canonical — body sprite material samples as Color/sRGB)")
    unreal.log("=" * 60)

    counts = {"fixed": 0, "already_ok": 0, "not_a_texture": 0}
    for enemy in NEW_ENEMIES:
        disk_dir = os.path.join(ATLAS_ROOT, enemy)
        game_dir = f"{GAME_ROOT}/{enemy}"
        pngs = sorted(glob.glob(os.path.join(disk_dir, "*.png")))
        unreal.log(f"\n--- {enemy} ({len(pngs)} atlases) ---")
        for png in pngs:
            name = os.path.splitext(os.path.basename(png))[0]
            asset_path = f"{game_dir}/{name}"
            result = fix_one(asset_path)
            counts[result] = counts.get(result, 0) + 1
            tag = {"fixed": "[FIXED]", "already_ok": "[OK]", "not_a_texture": "[ERROR]"}[result]
            unreal.log(f"  {tag} {name}")

    unreal.log("")
    unreal.log("=" * 60)
    unreal.log(f"Done. fixed={counts['fixed']}  already_ok={counts['already_ok']}  errors={counts['not_a_texture']}")
    unreal.log("Wait for 'Compiling Textures: 0' before launching the game.")
    unreal.log("=" * 60)


main()
