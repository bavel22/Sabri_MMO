import unreal

eal = unreal.EditorAssetLibrary
RO_TEX = "/Game/SabriMMO/Textures/Environment/RO_Original"

tex_nums = ["005", "015", "025", "042", "055", "088", "100", "150"]

for num in tex_nums:
    path = f"{RO_TEX}/{num}"
    exists = eal.does_asset_exist(path)
    unreal.log(f"  {num}: {'EXISTS' if exists else 'MISSING'}")

    # Also check with common suffixes UE5 adds
    for suffix in ["_Mat", "_00"]:
        alt = f"{RO_TEX}/{num}{suffix}"
        if eal.does_asset_exist(alt):
            unreal.log(f"    Found as: {num}{suffix}")
