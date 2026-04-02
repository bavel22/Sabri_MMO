# Delete all existing BiomeVariant material instances so they can be recreated
# with the new parameter variations.

import unreal

eal = unreal.EditorAssetLibrary
MI_BASE = "/Game/SabriMMO/Materials/Environment/BiomeVariants"

assets = eal.list_assets(MI_BASE, recursive=True)
count = 0
for asset in assets:
    clean = asset.split(".")[0]
    if eal.does_asset_exist(clean):
        eal.delete_asset(clean)
        count += 1

unreal.log(f"Deleted {count} old material instances from {MI_BASE}")
unreal.log(f"Now re-run create_500_variants_v2.py to recreate with full parameter variations")
