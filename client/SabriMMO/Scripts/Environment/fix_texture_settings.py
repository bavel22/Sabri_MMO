# fix_texture_settings.py
# Fixes compression settings on already-imported textures.
# UE5 auto-imports PNGs with wrong settings (sRGB on, Default compression).
# Normal maps need TC_NORMALMAP + sRGB OFF.
# Depth/AO maps need TC_GRAYSCALE + sRGB OFF.

import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# All texture folders to scan
SEARCH_PATHS = [
    "/Game/SabriMMO/Textures/Environment/Ground/Normals",
    "/Game/SabriMMO/Textures/Environment/Ground/Depth",
    "/Game/SabriMMO/Textures/Environment/Ground/AO",
    "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals",
    "/Game/SabriMMO/Textures/Environment/Ground_2K/Depth",
    "/Game/SabriMMO/Textures/Environment/Ground_2K/AO",
]

fixed = 0

for search_path in SEARCH_PATHS:
    is_normal = "Normals" in search_path
    is_gray = "Depth" in search_path or "AO" in search_path
    label = "NORMAL" if is_normal else "GRAYSCALE"

    assets = eal.list_assets(search_path, recursive=False)
    if not assets:
        continue

    unreal.log(f"\n--- {search_path} ({len(assets)} assets, setting {label}) ---")

    for asset_path in assets:
        # Clean the path (remove trailing class info if present)
        clean_path = asset_path.split(".")[0]
        tex = unreal.load_asset(clean_path)
        if not tex or not isinstance(tex, unreal.Texture2D):
            continue

        name = clean_path.split("/")[-1]
        changed = False

        if is_normal:
            current = tex.get_editor_property("compression_settings")
            if current != unreal.TextureCompressionSettings.TC_NORMALMAP:
                tex.set_editor_property("compression_settings",
                    unreal.TextureCompressionSettings.TC_NORMALMAP)
                changed = True
            if tex.get_editor_property("srgb"):
                tex.set_editor_property("srgb", False)
                changed = True

        elif is_gray:
            current = tex.get_editor_property("compression_settings")
            if current != unreal.TextureCompressionSettings.TC_GRAYSCALE:
                tex.set_editor_property("compression_settings",
                    unreal.TextureCompressionSettings.TC_GRAYSCALE)
                changed = True
            if tex.get_editor_property("srgb"):
                tex.set_editor_property("srgb", False)
                changed = True

        # Ensure wrap tiling
        tex.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)

        if changed:
            eal.save_asset(clean_path)
            unreal.log(f"  FIXED: {name} -> {label}, sRGB OFF")
            fixed += 1
        else:
            unreal.log(f"  OK:    {name}")

unreal.log(f"\n{'=' * 50}")
unreal.log(f"  Fixed {fixed} textures")
unreal.log(f"{'=' * 50}")
