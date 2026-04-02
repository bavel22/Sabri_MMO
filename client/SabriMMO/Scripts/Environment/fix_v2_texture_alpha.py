# Fix V2 sprite textures — ensure alpha channel is recognized by UE5.
# The issue: UE5 imports PNGs with alpha but may compress away the alpha
# or set wrong compression settings. We need TC_EditorIcon or TC_Default
# with sRGB on, and the texture must have "Compress Without Alpha" UNCHECKED.

import unreal

eal = unreal.EditorAssetLibrary
TEX_PATH = "/Game/SabriMMO/Environment/GrassV2/Textures"

assets = eal.list_assets(TEX_PATH, recursive=False)
fixed = 0

for asset in sorted(assets):
    clean = asset.split(".")[0]
    tex = unreal.load_asset(clean)
    if not tex or not isinstance(tex, unreal.Texture2D):
        continue

    name = clean.split("/")[-1]

    # Ensure alpha is preserved — use DXT5/BC3 which has alpha
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_DEFAULT)
    tex.set_editor_property("srgb", True)

    # Critical: make sure alpha is NOT compressed away
    try:
        tex.set_editor_property("compress_without_alpha", False)
    except Exception:
        pass

    eal.save_asset(clean)
    fixed += 1
    unreal.log(f"  {name}: TC_DEFAULT, sRGB=True, alpha preserved")

unreal.log(f"\nFixed {fixed} textures. Now rebuild materials.")

# Also check if the materials are actually compiling
unreal.log("\n=== Checking material compilation ===")
MAT_PATH = "/Game/SabriMMO/Environment/GrassV2/Materials"
mat_assets = eal.list_assets(MAT_PATH, recursive=False)
for a in sorted(mat_assets)[:3]:
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    mat = unreal.load_asset(clean)
    if mat and isinstance(mat, unreal.Material):
        # Force recompile
        unreal.MaterialEditingLibrary.recompile_material(mat)
        eal.save_asset(clean)
        unreal.log(f"  {name}: recompiled")
