import unreal

eal = unreal.EditorAssetLibrary

# Check meshes
unreal.log("=== V2 Meshes ===")
assets = eal.list_assets("/Game/SabriMMO/Environment/GrassV2/Meshes", recursive=False)
for a in sorted(assets)[:10]:
    clean = a.split(".")[0]
    obj = unreal.load_asset(clean)
    name = clean.split("/")[-1]
    if isinstance(obj, unreal.StaticMesh):
        bounds = obj.get_bounds()
        unreal.log(f"  {name}: bounds={bounds.box_extent}")
    else:
        unreal.log(f"  {name}: {type(obj).__name__}")

# Check textures
unreal.log("\n=== V2 Textures ===")
assets = eal.list_assets("/Game/SabriMMO/Environment/GrassV2/Textures", recursive=False)
for a in sorted(assets):
    clean = a.split(".")[0]
    obj = unreal.load_asset(clean)
    name = clean.split("/")[-1]
    unreal.log(f"  {name}: {type(obj).__name__}")

# Check materials - do they compile?
unreal.log("\n=== V2 Materials ===")
assets = eal.list_assets("/Game/SabriMMO/Environment/GrassV2/Materials", recursive=False)
for a in sorted(assets)[:5]:
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    unreal.log(f"  {name}")

# Check GrassType
unreal.log("\n=== V2 GrassType ===")
gt = unreal.load_asset("/Game/SabriMMO/Environment/GrassV2/Types/GT_V2_Test")
if gt:
    varieties = gt.get_editor_property("grass_varieties")
    unreal.log(f"  GT_V2_Test: {len(varieties)} varieties")
    for i, v in enumerate(varieties):
        mesh = v.get_editor_property("grass_mesh")
        mesh_name = mesh.get_name() if mesh else "NONE"
        sx = v.get_editor_property("scale_x")
        unreal.log(f"    [{i}] {mesh_name} scale={sx}")
else:
    unreal.log("  GT_V2_Test: NOT FOUND")
