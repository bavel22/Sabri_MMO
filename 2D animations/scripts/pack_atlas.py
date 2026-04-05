"""
Sprite Atlas Packer for Sabri_MMO
==================================
Packs individual sprite PNGs into a single texture atlas + JSON metadata.

Layout: 8 columns (one per direction: S, SW, W, NW, N, NE, E, SE)
        Rows grouped by animation (idle, walk, attack, death)

Usage:
  python pack_atlas.py <input_dir> <output_dir> [--cell-size 1024] [--name mage_f_body]

Example:
  C:/ComfyUI/venv/Scripts/python.exe pack_atlas.py ^
    "sprites/final/mage_f" ^
    "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body" ^
    --name mage_f_body

Output:
  mage_f_body.png   — texture atlas (8 cols × N rows)
  mage_f_body.json  — metadata for UE5 material
"""

import os
import sys
import json
import argparse
import math
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow required. pip install Pillow")
    sys.exit(1)

# Direction order (columns left to right)
DIRECTIONS = ["S", "SW", "W", "NW", "N", "NE", "E", "SE"]

# Animation order (rows top to bottom) with expected frame counts
ANIM_ORDER = [
    ("idle", 4),
    ("walk", 8),
    ("attack", 6),
    ("death", 3),
    ("cast", 4),
    ("hit", 2),
]


def discover_animations(input_dir):
    """Scan directory and discover available animations + frame counts."""
    files = {}
    for f in sorted(os.listdir(input_dir)):
        if not f.endswith('.png'):
            continue
        # Parse: {anim}_{dir}_f{frame}.png
        parts = f.rsplit('_', 2)
        if len(parts) < 3:
            continue
        frame_part = parts[-1].replace('.png', '')  # f00, f01, ...
        dir_part = parts[-2]                         # S, SW, W, ...
        anim_part = '_'.join(parts[:-2])             # idle, walk, ...

        if dir_part not in DIRECTIONS:
            continue
        if not frame_part.startswith('f'):
            continue

        frame_idx = int(frame_part[1:])
        key = (anim_part, dir_part, frame_idx)
        files[key] = os.path.join(input_dir, f)

    # Determine animations and their frame counts
    anims = {}
    for (anim, dir_name, frame_idx) in files:
        if anim not in anims:
            anims[anim] = 0
        anims[anim] = max(anims[anim], frame_idx + 1)

    return files, anims


def pack_atlas(input_dir, output_dir, name, cell_size):
    """Pack sprites into atlas."""
    os.makedirs(output_dir, exist_ok=True)

    files, discovered_anims = discover_animations(input_dir)
    if not files:
        print(f"ERROR: No sprite PNGs found in {input_dir}")
        return

    # Build ordered animation list (only animations that exist)
    anim_list = []
    for anim_name, expected_frames in ANIM_ORDER:
        if anim_name in discovered_anims:
            anim_list.append((anim_name, discovered_anims[anim_name]))

    # Add any extra animations not in ANIM_ORDER
    for anim_name, frame_count in sorted(discovered_anims.items()):
        if anim_name not in [a[0] for a in anim_list]:
            anim_list.append((anim_name, frame_count))

    num_cols = len(DIRECTIONS)  # 8
    total_rows = sum(fc for _, fc in anim_list)

    atlas_w = num_cols * cell_size
    atlas_h = total_rows * cell_size

    print(f"Atlas layout: {num_cols} cols × {total_rows} rows = "
          f"{atlas_w}×{atlas_h} pixels")
    print(f"Animations: {[(a, f) for a, f in anim_list]}")

    # Create atlas image
    atlas = Image.new('RGBA', (atlas_w, atlas_h), (0, 0, 0, 0))

    # Place sprites
    current_row = 0
    metadata = {
        "name": name,
        "atlas_size": [atlas_w, atlas_h],
        "cell_size": [cell_size, cell_size],
        "grid": [num_cols, total_rows],
        "directions": DIRECTIONS,
        "animations": {},
        "total_frames": 0,
    }

    placed = 0
    for anim_name, frame_count in anim_list:
        metadata["animations"][anim_name] = {
            "start_row": current_row,
            "frame_count": frame_count,
        }

        for frame_idx in range(frame_count):
            for col_idx, dir_name in enumerate(DIRECTIONS):
                key = (anim_name, dir_name, frame_idx)
                if key not in files:
                    continue

                img = Image.open(files[key]).convert('RGBA')

                # Resize to cell_size if needed
                if img.size != (cell_size, cell_size):
                    img = img.resize((cell_size, cell_size), Image.NEAREST)

                x = col_idx * cell_size
                y = (current_row + frame_idx) * cell_size
                atlas.paste(img, (x, y))
                placed += 1

            current_row_label = current_row + frame_idx

        current_row += frame_count

    metadata["total_frames"] = placed

    # Save atlas
    atlas_path = os.path.join(output_dir, f"{name}.png")
    atlas.save(atlas_path, 'PNG', optimize=True)
    atlas_mb = os.path.getsize(atlas_path) / (1024 * 1024)

    # Save metadata
    meta_path = os.path.join(output_dir, f"{name}.json")
    with open(meta_path, 'w') as f:
        json.dump(metadata, f, indent=2)

    print(f"\nAtlas: {atlas_path} ({atlas_mb:.1f} MB)")
    print(f"Meta:  {meta_path}")
    print(f"Placed {placed} sprites into {num_cols}×{total_rows} grid")
    print(f"\nFrame index formula:")
    print(f"  row = animations[anim].start_row + frame_index")
    print(f"  col = direction_index (0=S, 1=SW, ... 7=SE)")
    print(f"  atlas_index = row * {num_cols} + col")

    return metadata


def discover_subfolder_animations(subfolder_path, folder_name):
    """Scan a subfolder for sprites matching {FolderName}_{Dir}_f{Frame}.png."""
    files = {}
    if not os.path.isdir(subfolder_path):
        return files, 0

    max_frame = -1
    for f in sorted(os.listdir(subfolder_path)):
        if not f.endswith('.png'):
            continue
        # Parse: {FolderName}_{Dir}_f{Frame}.png
        # Split from right on last 2 underscores
        parts = f.rsplit('_', 2)
        if len(parts) < 3:
            continue
        frame_part = parts[-1].replace('.png', '')
        dir_part = parts[-2]

        if dir_part not in DIRECTIONS:
            continue
        if not frame_part.startswith('f'):
            continue

        frame_idx = int(frame_part[1:])
        max_frame = max(max_frame, frame_idx)
        files[(dir_part, frame_idx)] = os.path.join(subfolder_path, f)

    frame_count = max_frame + 1 if max_frame >= 0 else 0
    return files, frame_count


def pack_multi_atlas(config_path, input_root, output_dir):
    """Pack sprites into multiple weapon-group atlases from a config file.

    Args:
        config_path: Path to atlas config JSON (e.g., swordsman_m.json)
        input_root: Root directory containing animation subfolders
        output_dir: Output directory for atlas PNGs + JSONs
    """
    os.makedirs(output_dir, exist_ok=True)

    with open(config_path, 'r') as f:
        config = json.load(f)

    character = config["character"]
    cell_size = config.get("cell_size", 256)
    num_cols = len(DIRECTIONS)

    print(f"\n{'=' * 60}")
    print(f"  Multi-Atlas Packer: {character}")
    print(f"  Cell size: {cell_size}px")
    print(f"  Input: {input_root}")
    print(f"  Output: {output_dir}")
    print(f"{'=' * 60}")

    all_results = {}

    for group_name, group_config in config["groups"].items():
        print(f"\n--- Group: {group_name} ---")

        # Discover all sprites for this group's animations
        anim_entries = []  # [(folder_name, state, files_dict, frame_count)]
        for anim_def in group_config["animations"]:
            folder_name = anim_def["folder"]
            state = anim_def["state"]
            subfolder = os.path.join(input_root, folder_name)
            files, frame_count = discover_subfolder_animations(subfolder, folder_name)

            if not files:
                print(f"  [WARN] No sprites in '{folder_name}/' — skipping")
                continue

            anim_entries.append((folder_name, state, files, frame_count))
            print(f"  {folder_name} -> {state} ({frame_count} frames, "
                  f"{len(files)} sprites)")

        if not anim_entries:
            print(f"  [SKIP] No sprites found for group '{group_name}'")
            continue

        # Calculate total rows
        total_rows = sum(fc for _, _, _, fc in anim_entries)
        atlas_w = num_cols * cell_size
        atlas_h = total_rows * cell_size

        print(f"\n  Atlas: {num_cols} cols × {total_rows} rows = "
              f"{atlas_w}×{atlas_h} pixels")

        # Check UE5 texture limit
        if atlas_h > 16384:
            print(f"  [WARN] Atlas height {atlas_h} exceeds UE5 max 16384!")

        # Create atlas image
        atlas = Image.new('RGBA', (atlas_w, atlas_h), (0, 0, 0, 0))

        # Build metadata with variant support
        # Group animations by state, tracking variants
        animations_meta = {}  # state -> { variants: [...] }
        current_row = 0
        placed = 0

        for folder_name, state, files, frame_count in anim_entries:
            # Add variant entry
            if state not in animations_meta:
                animations_meta[state] = {"variants": []}

            variant_info = {
                "start_row": current_row,
                "frame_count": frame_count,
                "source": folder_name,
            }
            animations_meta[state]["variants"].append(variant_info)

            # Place sprites into atlas
            for frame_idx in range(frame_count):
                for col_idx, dir_name in enumerate(DIRECTIONS):
                    key = (dir_name, frame_idx)
                    if key not in files:
                        continue

                    img = Image.open(files[key]).convert('RGBA')
                    if img.size != (cell_size, cell_size):
                        img = img.resize((cell_size, cell_size), Image.NEAREST)

                    x = col_idx * cell_size
                    y = (current_row + frame_idx) * cell_size
                    atlas.paste(img, (x, y))
                    placed += 1

            current_row += frame_count

        # Save atlas PNG
        atlas_name = f"{character}_{group_name}"
        atlas_path = os.path.join(output_dir, f"{atlas_name}.png")
        atlas.save(atlas_path, 'PNG', optimize=True)
        atlas_mb = os.path.getsize(atlas_path) / (1024 * 1024)

        # Save metadata JSON
        metadata = {
            "name": atlas_name,
            "character": character,
            "weapon_group": group_name,
            "atlas_size": [atlas_w, atlas_h],
            "cell_size": [cell_size, cell_size],
            "grid": [num_cols, total_rows],
            "directions": DIRECTIONS,
            "animations": animations_meta,
            "total_frames": placed,
        }

        meta_path = os.path.join(output_dir, f"{atlas_name}.json")
        with open(meta_path, 'w') as f:
            json.dump(metadata, f, indent=2)

        print(f"  -> {atlas_path} ({atlas_mb:.1f} MB, {placed} sprites)")
        all_results[group_name] = metadata

    # Summary
    print(f"\n{'=' * 60}")
    print(f"  COMPLETE: {len(all_results)} atlases for {character}")
    for group_name, meta in all_results.items():
        anims = meta["animations"]
        total_variants = sum(len(a["variants"]) for a in anims.values())
        print(f"    {group_name}: {meta['grid'][1]} rows, "
              f"{len(anims)} states, {total_variants} variants, "
              f"{meta['total_frames']} sprites")
    print(f"{'=' * 60}")

    return all_results


def sanitize_name(name):
    """Convert folder name to filesystem-safe atlas name."""
    return name.lower().replace(' ', '_').replace('(', '').replace(')', '').strip('_')


def pack_individual_atlases(config_path, input_root, output_dir):
    """V2: Pack one atlas per animation folder. Each atlas = 8 cols x N frames.

    Args:
        config_path: Path to v2 atlas config JSON
        input_root: Root directory containing animation subfolders
        output_dir: Output directory for atlas PNGs + JSONs + manifest
    """
    os.makedirs(output_dir, exist_ok=True)

    with open(config_path, 'r') as f:
        config = json.load(f)

    character = config["character"]
    cell_size = config.get("cell_size", 1024)
    num_cols = len(DIRECTIONS)

    print(f"\n{'=' * 60}")
    print(f"  Per-Animation Atlas Packer (v2): {character}")
    print(f"  Cell size: {cell_size}px")
    print(f"  Input: {input_root}")
    print(f"  Output: {output_dir}")
    print(f"{'=' * 60}")

    manifest_entries = []
    total_atlases = 0
    total_sprites = 0

    for anim_def in config["animations"]:
        folder_name = anim_def["folder"]
        state = anim_def["state"]
        group = anim_def["group"]

        subfolder = os.path.join(input_root, folder_name)
        files, frame_count = discover_subfolder_animations(subfolder, folder_name)

        if not files:
            print(f"  [WARN] No sprites in '{folder_name}/' — skipping")
            continue

        # Atlas: 8 cols x frame_count rows
        atlas_w = num_cols * cell_size
        atlas_h = frame_count * cell_size

        if atlas_h > 16384:
            print(f"  [WARN] {folder_name}: {atlas_h}px exceeds UE5 16384 limit!")

        atlas = Image.new('RGBA', (atlas_w, atlas_h), (0, 0, 0, 0))
        placed = 0

        for frame_idx in range(frame_count):
            for col_idx, dir_name in enumerate(DIRECTIONS):
                key = (dir_name, frame_idx)
                if key not in files:
                    continue

                img = Image.open(files[key]).convert('RGBA')
                if img.size != (cell_size, cell_size):
                    img = img.resize((cell_size, cell_size), Image.NEAREST)

                x = col_idx * cell_size
                y = frame_idx * cell_size
                atlas.paste(img, (x, y))
                placed += 1

        # Save atlas PNG
        safe_name = sanitize_name(folder_name)
        atlas_name = f"{character}_{safe_name}"
        atlas_path = os.path.join(output_dir, f"{atlas_name}.png")
        atlas.save(atlas_path, 'PNG', optimize=True)
        atlas_mb = os.path.getsize(atlas_path) / (1024 * 1024)

        # Load depth map if available (from dual-pass weapon render)
        # Three modes controlled by config "depth_mode":
        #   "global" (default): Global majority vote per direction across ALL animations.
        #       Prevents depth flipping when switching animations (used for weapons).
        #   "raw": Use actual per-frame per-direction depth from the render camera.
        #       Each frame can have different front/behind per direction.
        #   "always_front": No depth array emitted — C++ defaults to +5 (always in front).
        #       Used for most headgear that should always overlay on top of the body.
        depth_front_flat = None
        depth_map_path = os.path.join(input_root, 'depth_map.json')
        depth_mode = config.get("depth_mode", "global")

        if depth_mode == "always_front":
            pass  # depth_front_flat stays None — C++ defaults to front (+5)

        elif os.path.exists(depth_map_path):
            with open(depth_map_path, 'r') as df:
                depth_map = json.load(df)

            if depth_mode == "raw" and folder_name in depth_map:
                # Raw mode: use actual per-frame depth from render camera
                anim_depth = depth_map[folder_name]
                depth_front_flat = []
                for frame_idx in range(frame_count):
                    for dir_name in DIRECTIONS:
                        if dir_name in anim_depth and frame_idx < len(anim_depth[dir_name]):
                            depth_front_flat.append(1 if anim_depth[dir_name][frame_idx] else 0)
                        else:
                            depth_front_flat.append(1)  # default: front
            else:
                # Global majority vote (default): one depth per direction across all animations
                if not hasattr(pack_individual_atlases, '_global_depth'):
                    global_counts = {d: [0, 0] for d in DIRECTIONS}  # [behind, front]
                    for anim_name, anim_depth in depth_map.items():
                        for dir_name in DIRECTIONS:
                            if dir_name in anim_depth:
                                for v in anim_depth[dir_name]:
                                    global_counts[dir_name][1 if v else 0] += 1
                    pack_individual_atlases._global_depth = {
                        d: counts[1] > counts[0]  # True if front wins
                        for d, counts in global_counts.items()
                    }

                global_depth = pack_individual_atlases._global_depth

                if folder_name in depth_map:
                    # Build flat array using global direction vote
                    depth_front_flat = []
                    for frame_idx in range(frame_count):
                        for dir_name in DIRECTIONS:
                            depth_front_flat.append(1 if global_depth.get(dir_name, True) else 0)

        # Save per-atlas JSON
        atlas_meta = {
            "version": 2,
            "name": atlas_name,
            "state": state,
            "group": group,
            "cell_size": [cell_size, cell_size],
            "grid": [num_cols, frame_count],
            "frame_count": frame_count,
            "source": folder_name,
        }
        if depth_front_flat is not None:
            atlas_meta["depth_front"] = depth_front_flat

        meta_path = os.path.join(output_dir, f"{atlas_name}.json")
        with open(meta_path, 'w') as f:
            json.dump(atlas_meta, f, indent=2)

        manifest_entries.append({
            "file": atlas_name,
            "state": state,
            "group": group,
        })

        total_atlases += 1
        total_sprites += placed
        print(f"  {folder_name} -> {atlas_name}.png "
              f"({num_cols}×{frame_count} = {atlas_w}×{atlas_h}, "
              f"{atlas_mb:.1f} MB, {placed} sprites)")

    # Save manifest
    manifest = {
        "version": 2,
        "character": character,
        "cell_size": cell_size,
        "atlases": manifest_entries,
    }

    manifest_path = os.path.join(output_dir, f"{character}_manifest.json")
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)

    print(f"\n{'=' * 60}")
    print(f"  COMPLETE: {total_atlases} atlases, {total_sprites} sprites")
    print(f"  Manifest: {manifest_path}")
    print(f"{'=' * 60}")

    return manifest


def main():
    parser = argparse.ArgumentParser(
        description="Pack sprite PNGs into a texture atlas for UE5")
    parser.add_argument("input", help="Directory of sprite PNGs "
                        "(or render output root when using --config)")
    parser.add_argument("output", help="Output directory for atlas + JSON")
    parser.add_argument("--name", default="sprite_atlas",
                        help="Atlas filename (without extension)")
    parser.add_argument("--cell-size", type=int, default=1024,
                        help="Cell size in pixels (default 1024)")
    parser.add_argument("--config",
                        help="Atlas config JSON (auto-detects v1 weapon-group "
                        "or v2 per-animation format)")
    args = parser.parse_args()

    if args.config:
        # Auto-detect config version
        with open(args.config, 'r') as f:
            cfg = json.load(f)
        if cfg.get("version") == 2:
            pack_individual_atlases(args.config, args.input, args.output)
        else:
            pack_multi_atlas(args.config, args.input, args.output)
    else:
        pack_atlas(args.input, args.output, args.name, args.cell_size)


if __name__ == "__main__":
    main()
