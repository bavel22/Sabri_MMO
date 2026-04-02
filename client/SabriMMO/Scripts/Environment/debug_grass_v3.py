import unreal

eal = unreal.EditorAssetLibrary

unreal.log("=== V3 Textures (what actually exists) ===")
assets = eal.list_assets("/Game/SabriMMO/Environment/GrassV3/Textures", recursive=False)
for a in sorted(assets):
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    obj = unreal.load_asset(clean)
    if isinstance(obj, unreal.Texture2D):
        unreal.log(f"  {name}")

unreal.log("\n=== V3 Materials (what was created) ===")
assets = eal.list_assets("/Game/SabriMMO/Environment/GrassV3/Materials", recursive=False)
count = 0
for a in sorted(assets):
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    count += 1
unreal.log(f"  {count} materials total")

unreal.log("\n=== V2 Meshes (what we reuse) ===")
assets = eal.list_assets("/Game/SabriMMO/Environment/GrassV2/Meshes", recursive=False)
for a in sorted(assets)[:5]:
    clean = a.split(".")[0]
    name = clean.split("/")[-1]
    obj = unreal.load_asset(clean)
    if isinstance(obj, unreal.StaticMesh):
        unreal.log(f"  {name}: StaticMesh")
