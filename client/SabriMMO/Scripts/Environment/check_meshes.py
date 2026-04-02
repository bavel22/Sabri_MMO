# Check all scatter meshes — report vertex count, bounds, material

import unreal

eal = unreal.EditorAssetLibrary
MESH_PATH = "/Game/SabriMMO/Environment/Grass/Meshes"

assets = eal.list_assets(MESH_PATH, recursive=False)
sm_count = 0
empty_count = 0

for asset in sorted(assets):
    clean = asset.split(".")[0]
    obj = unreal.load_asset(clean)
    name = clean.split("/")[-1]

    if not obj:
        continue

    if isinstance(obj, unreal.StaticMesh):
        # Get render data info
        num_lods = obj.get_num_lods()
        bounds = obj.get_bounds()
        box_extent = bounds.box_extent if bounds else "N/A"

        # Check if mesh has any sections
        num_sections = obj.get_num_sections(0) if num_lods > 0 else 0

        if num_sections == 0:
            empty_count += 1
            unreal.log(f"  {name}: EMPTY (0 sections, {num_lods} LODs)")
        else:
            sm_count += 1
            unreal.log(f"  {name}: {num_sections} sections, {num_lods} LODs, bounds={box_extent}")
    else:
        unreal.log(f"  {name}: {type(obj).__name__} (not StaticMesh)")

unreal.log(f"\nTotal: {sm_count} valid meshes, {empty_count} empty, {len(assets)} total assets")
