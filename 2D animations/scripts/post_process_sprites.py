"""
Post-Process Rendered Sprites for Sabri_MMO
=============================================
Takes 512x512 Blender renders and produces final 256x256 game-ready sprites.

Steps per image:
  1. Auto-trim transparent border
  2. Nearest-neighbor downscale (no bilinear blur — preserves pixel art)
  3. Re-center character with feet near bottom edge
  4. Optional palette quantization

Usage:
  python post_process_sprites.py <input_dir> <output_dir> [--size 256] [--no-trim] [--quantize 32]

  Uses Pillow (install: pip install Pillow)
  Run with the ComfyUI venv: C:/ComfyUI/venv/Scripts/python.exe

Example:
  C:/ComfyUI/venv/Scripts/python.exe post_process_sprites.py ^
      "sprites/render_output/novice_m" ^
      "sprites/final/novice_m" ^
      --size 256
"""

import os
import sys
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow not installed.")
    print("  pip install Pillow")
    print("  or: C:/ComfyUI/venv/Scripts/pip.exe install Pillow")
    sys.exit(1)


def process_sprite(input_path, output_path, target_size=256,
                   trim=True, quantize_colors=0, foot_margin=0.05):
    """
    Process one sprite image.

    Args:
        input_path:       Source PNG (512x512 RGBA from Blender)
        output_path:      Destination PNG
        target_size:      Final square size in pixels
        trim:             Auto-trim transparent border and re-center
        quantize_colors:  If >0, reduce palette to this many colors
        foot_margin:      Bottom margin as fraction of target_size (feet padding)
    """
    img = Image.open(input_path).convert('RGBA')

    if trim:
        bbox = img.getbbox()
        if bbox is None:
            # Fully transparent — save empty
            result = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            result.save(output_path, 'PNG')
            return

        # Crop to content
        cropped = img.crop(bbox)
        cw, ch = cropped.size

        # Scale to fit inside target with margins
        margin_px = int(target_size * foot_margin)
        usable = target_size - margin_px * 2
        scale = min(usable / cw, usable / ch)

        if scale < 1.0:
            new_w = max(1, int(cw * scale))
            new_h = max(1, int(ch * scale))
            cropped = cropped.resize((new_w, new_h), Image.NEAREST)
        else:
            new_w, new_h = cw, ch

        # Place on target canvas — horizontally centered, bottom-aligned
        result = Image.new('RGBA', (target_size, target_size), (0, 0, 0, 0))
        paste_x = (target_size - new_w) // 2
        paste_y = target_size - new_h - margin_px  # feet near bottom
        result.paste(cropped, (paste_x, paste_y))
    else:
        # Simple nearest-neighbor downscale
        result = img.resize((target_size, target_size), Image.NEAREST)

    # Optional palette quantization
    if quantize_colors > 0:
        # Quantize while preserving transparency
        alpha = result.split()[3]
        rgb = result.convert('RGB')
        rgb = rgb.quantize(colors=quantize_colors, method=Image.Quantize.MEDIANCUT)
        rgb = rgb.convert('RGBA')
        # Restore original alpha
        rgb.putalpha(alpha)
        result = rgb

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    result.save(output_path, 'PNG', optimize=True)


def process_directory(input_dir, output_dir, target_size=256,
                      trim=True, quantize_colors=0):
    """Process all PNGs in a directory tree, preserving structure."""
    in_path = Path(input_dir)
    out_path = Path(output_dir)

    pngs = sorted(in_path.rglob("*.png"))
    total = len(pngs)

    if total == 0:
        print(f"No PNG files found in {input_dir}")
        return

    print(f"Processing {total} sprites: {input_dir} -> {output_dir}")
    print(f"  Target: {target_size}x{target_size}, trim={trim}, "
          f"quantize={quantize_colors or 'off'}")

    processed = 0
    for png in pngs:
        rel = png.relative_to(in_path)
        out = out_path / rel

        process_sprite(str(png), str(out), target_size, trim, quantize_colors)
        processed += 1

        if processed % 50 == 0 or processed == total:
            print(f"  [{processed}/{total}] {rel}")

    print(f"\nDone! {processed} sprites -> {output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Post-process Blender sprite renders for Sabri_MMO")
    parser.add_argument("input", help="Input directory (Blender render output)")
    parser.add_argument("output", help="Output directory (final sprites)")
    parser.add_argument("--size", type=int, default=256,
                        help="Target sprite size (default 256)")
    parser.add_argument("--no-trim", action="store_true",
                        help="Skip auto-trim (simple downscale only)")
    parser.add_argument("--quantize", type=int, default=0,
                        help="Reduce to N colors (0=off, try 32 for pixel art)")
    args = parser.parse_args()

    process_directory(args.input, args.output, args.size,
                      not args.no_trim, args.quantize)


if __name__ == "__main__":
    main()
