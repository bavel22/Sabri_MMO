# import_ground_textures.py
# Run in UE5 Editor via Output Log (Python mode) or Edit > Execute Python Script
#
# Imports ALL ground textures (diffuse + normal + depth + AO) from the
# Content folder PNGs into UE5 assets. Sets correct texture settings
# (normals as Normal Compression, depth/AO as grayscale, etc.)

import unreal
import os
import glob

# Source folders (PNGs sitting in Content folder)
BASE = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/Ground"
BASE_2K = "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Textures/Environment/Ground_2K"
FOLDERS = {
    # 1K originals
    "diffuse": (BASE, "/Game/SabriMMO/Textures/Environment/Ground"),
    "normals": (os.path.join(BASE, "Normals"), "/Game/SabriMMO/Textures/Environment/Ground/Normals"),
    "depth":   (os.path.join(BASE, "Depth"), "/Game/SabriMMO/Textures/Environment/Ground/Depth"),
    "ao":      (os.path.join(BASE, "AO"), "/Game/SabriMMO/Textures/Environment/Ground/AO"),
    # Seamless-fixed 2K
    "seamless": (os.path.join(os.path.dirname(BASE), "Ground_Seamless"),
                 "/Game/SabriMMO/Textures/Environment/Ground_Seamless"),
    # 2K upscaled
    "diffuse_2k": (BASE_2K, "/Game/SabriMMO/Textures/Environment/Ground_2K"),
    "normals_2k": (os.path.join(BASE_2K, "Normals"), "/Game/SabriMMO/Textures/Environment/Ground_2K/Normals"),
    "depth_2k":   (os.path.join(BASE_2K, "Depth"), "/Game/SabriMMO/Textures/Environment/Ground_2K/Depth"),
    "ao_2k":      (os.path.join(BASE_2K, "AO"), "/Game/SabriMMO/Textures/Environment/Ground_2K/AO"),
}


def import_textures(source_dir, dest_path, is_normal=False, is_grayscale=False):
    """Import all PNGs from source_dir into dest_path."""
    pngs = glob.glob(os.path.join(source_dir, "*.png"))
    if not pngs:
        unreal.log_warning(f"No PNGs found in {source_dir}")
        return 0

    tasks = []
    for png in pngs:
        name = os.path.splitext(os.path.basename(png))[0]
        asset_path = f"{dest_path}/{name}"

        # Skip if already imported
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            continue

        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", dest_path)
        task.set_editor_property("filename", png)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append((task, name, is_normal, is_grayscale))

    if not tasks:
        unreal.log(f"  All textures in {dest_path} already imported, skipping")
        return 0

    # Import all at once
    import_tasks = [t[0] for t in tasks]
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(import_tasks)

    # Post-import: set texture properties
    imported = 0
    for task, name, is_norm, is_gray in tasks:
        asset_path = f"{dest_path}/{name}"
        tex = unreal.load_asset(asset_path)
        if tex:
            if is_norm:
                # Normal map compression
                tex.set_editor_property("compression_settings",
                    unreal.TextureCompressionSettings.TC_NORMALMAP)
                tex.set_editor_property("srgb", False)
            elif is_gray:
                # Grayscale for depth/AO
                tex.set_editor_property("compression_settings",
                    unreal.TextureCompressionSettings.TC_GRAYSCALE)
                tex.set_editor_property("srgb", False)
            else:
                # Diffuse — sRGB on, default compression
                tex.set_editor_property("srgb", True)

            # All textures: enable seamless tiling
            tex.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
            tex.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)

            imported += 1

    return imported


# === Run imports ===
unreal.log("=" * 60)
unreal.log("  Importing Ground Textures")
unreal.log("=" * 60)

total = 0

unreal.log("\n--- Diffuse Textures ---")
n = import_textures(FOLDERS["diffuse"][0], FOLDERS["diffuse"][1])
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- Normal Maps ---")
n = import_textures(FOLDERS["normals"][0], FOLDERS["normals"][1], is_normal=True)
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- Depth/Height Maps ---")
n = import_textures(FOLDERS["depth"][0], FOLDERS["depth"][1], is_grayscale=True)
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- Ambient Occlusion Maps ---")
n = import_textures(FOLDERS["ao"][0], FOLDERS["ao"][1], is_grayscale=True)
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- Seamless-Fixed 2K Textures ---")
n = import_textures(FOLDERS["seamless"][0], FOLDERS["seamless"][1])
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- 2K Diffuse Textures ---")
n = import_textures(FOLDERS["diffuse_2k"][0], FOLDERS["diffuse_2k"][1])
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- 2K Normal Maps ---")
n = import_textures(FOLDERS["normals_2k"][0], FOLDERS["normals_2k"][1], is_normal=True)
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- 2K Depth/Height Maps ---")
n = import_textures(FOLDERS["depth_2k"][0], FOLDERS["depth_2k"][1], is_grayscale=True)
unreal.log(f"  Imported: {n}")
total += n

unreal.log("\n--- 2K Ambient Occlusion Maps ---")
n = import_textures(FOLDERS["ao_2k"][0], FOLDERS["ao_2k"][1], is_grayscale=True)
unreal.log(f"  Imported: {n}")
total += n

unreal.log(f"\n{'=' * 60}")
unreal.log(f"  DONE: {total} textures imported")
unreal.log(f"  Browse: /Game/SabriMMO/Textures/Environment/Ground/")
unreal.log(f"  Browse: /Game/SabriMMO/Textures/Environment/Ground_2K/")
unreal.log(f"{'=' * 60}")
