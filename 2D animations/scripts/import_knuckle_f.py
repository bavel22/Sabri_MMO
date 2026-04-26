"""
UE5 Import Helper — Knuckle Female (weapon_12)

Run inside the UE5 Editor's Python console:
    py "C:/Sabri_MMO/2D animations/scripts/import_knuckle_f.py"

What it does:
  1. For each weapon_12_*.png in Content/SabriMMO/Sprites/Atlases/Weapon/knuckle/female/,
     finds or creates the corresponding Texture2D asset.
  2. Sets the standard sprite atlas texture properties:
       - Compression: BC7 (TC_BC7)
       - Filter: Nearest (TF_Nearest)
       - Mip Gen Settings: NoMipmaps
       - Never Stream: True
       - sRGB: False (linear color so atlas pixels are color-precise)
  3. Saves all modified assets.

Prerequisites:
  - Pack pass already finished (PNGs + JSONs + manifest exist on disk)
  - Old .uassets deleted (or run with editor closed first; UE5 will auto-import on re-open)
  - This script runs INSIDE UE5's Python — uses unreal.* APIs

If UE5 hasn't auto-imported the PNGs yet, this script will trigger import
via AssetImportTask before applying settings.
"""

import unreal
import os
import glob

# ============================================================
# Config
# ============================================================
ATLAS_DIR_DISK = r"C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/knuckle/female"
ATLAS_DIR_GAME = "/Game/SabriMMO/Sprites/Atlases/Weapon/knuckle/female"
PREFIX = "weapon_12"

# ============================================================
# Helpers
# ============================================================
def import_png_if_needed(png_path, dest_dir):
    """Import a PNG into UE5 if no Texture2D exists at the destination."""
    name = os.path.splitext(os.path.basename(png_path))[0]
    asset_path = f"{dest_dir}/{name}.{name}"

    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing is not None:
        return existing

    # Asset doesn't exist — import it
    task = unreal.AssetImportTask()
    task.filename = png_path
    task.destination_path = dest_dir
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    return unreal.EditorAssetLibrary.load_asset(asset_path)


def configure_sprite_texture(texture):
    """Apply the standard sprite atlas settings."""
    if texture is None:
        return False

    # Compression: BC7 for high-quality color + alpha
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    # Filter: Nearest (no blur on pixel art)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    # No mips
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    # Never stream — load full resolution always
    texture.set_editor_property("never_stream", True)
    # sRGB OFF — linear color space so the cel-shade ramp colors stay accurate
    texture.set_editor_property("srgb", False)
    # Texture Group: UI (sprite atlases are UI textures, not world textures)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)

    # Save
    pkg = texture.get_outer()
    if pkg:
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return True


# ============================================================
# Main
# ============================================================
def main():
    if not os.path.isdir(ATLAS_DIR_DISK):
        unreal.log_error(f"Atlas directory not found on disk: {ATLAS_DIR_DISK}")
        return

    pngs = sorted(glob.glob(os.path.join(ATLAS_DIR_DISK, f"{PREFIX}_*.png")))
    if not pngs:
        unreal.log_error(f"No PNGs matching {PREFIX}_*.png found in {ATLAS_DIR_DISK}")
        return

    unreal.log(f"Knuckle Female import: found {len(pngs)} PNGs")

    imported = 0
    configured = 0

    for png in pngs:
        tex = import_png_if_needed(png, ATLAS_DIR_GAME)
        if tex is None:
            unreal.log_warning(f"Failed to import {os.path.basename(png)}")
            continue
        imported += 1

        if configure_sprite_texture(tex):
            configured += 1

    unreal.log(f"Knuckle Female import complete: {imported} imported, {configured} configured")
    unreal.SystemLibrary.print_string(
        None,
        f"Knuckle Female: {imported} imported, {configured} configured",
        text_color=unreal.LinearColor(0.2, 1.0, 0.2),
        duration=8.0,
    )


main()
