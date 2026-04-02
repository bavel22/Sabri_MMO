"""
Aggressive seamless fix using Laplacian pyramid multi-band blending.
Much better than simple cross-blend — preserves detail while eliminating seams.
Processes the 2K best textures.
"""
import os
import glob
import numpy as np
from PIL import Image

INPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/upscaled_2k"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/seamless_fixed"
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)


def gaussian_pyramid(img, levels):
    """Build a Gaussian pyramid."""
    pyramid = [img.astype(np.float64)]
    for _ in range(levels):
        img = pyramid[-1]
        # Simple 2x downsample with averaging
        h, w = img.shape[:2]
        down = (img[0::2, 0::2] + img[1::2, 0::2] + img[0::2, 1::2] + img[1::2, 1::2]) / 4.0
        pyramid.append(down)
    return pyramid


def upsample(img, target_shape):
    """2x upsample to target shape."""
    h, w = target_shape[:2]
    result = np.zeros((h, w) + img.shape[2:], dtype=np.float64)
    sh, sw = img.shape[:2]
    for y in range(h):
        for x in range(w):
            sy = min(y // 2, sh - 1)
            sx = min(x // 2, sw - 1)
            result[y, x] = img[sy, sx]
    return result


def laplacian_pyramid(img, levels):
    """Build a Laplacian pyramid."""
    gauss = gaussian_pyramid(img, levels)
    lap = []
    for i in range(levels):
        up = upsample(gauss[i + 1], gauss[i].shape)
        lap.append(gauss[i] - up)
    lap.append(gauss[-1])
    return lap


def reconstruct_from_laplacian(pyramid):
    """Reconstruct image from Laplacian pyramid."""
    img = pyramid[-1]
    for i in range(len(pyramid) - 2, -1, -1):
        up = upsample(img, pyramid[i].shape)
        img = up + pyramid[i]
    return img


def make_seamless_laplacian(img_path, output_path, levels=4):
    """
    Multi-band Laplacian pyramid seamless blending.

    Blends the image with its half-offset copy at multiple frequency bands:
    - Low frequencies: wide blend zone (eliminates color banding)
    - High frequencies: narrow blend zone (preserves detail sharpness)
    """
    img = np.array(Image.open(img_path), dtype=np.float64)
    H, W = img.shape[:2]

    # Create shifted copy (offset by half in both axes)
    shifted = np.roll(np.roll(img, W // 2, axis=1), H // 2, axis=0)

    # Create blend mask — diamond shape, smooth
    y = np.linspace(-1, 1, H)
    x = np.linspace(-1, 1, W)
    xv, yv = np.meshgrid(x, y)
    # Smooth diamond with wide transition
    mask = 1.0 - np.sqrt(xv**2 + yv**2) / np.sqrt(2)
    mask = np.clip(mask, 0, 1)
    # Smoothstep for even smoother transition
    mask = mask * mask * (3 - 2 * mask)

    if len(img.shape) == 3:
        mask_3d = mask[:, :, np.newaxis]
    else:
        mask_3d = mask

    # Build Laplacian pyramids for both images and the mask
    lap_img = laplacian_pyramid(img, levels)
    lap_shifted = laplacian_pyramid(shifted, levels)
    gauss_mask = gaussian_pyramid(mask_3d, levels)

    # Blend at each level
    lap_blended = []
    for i in range(len(lap_img)):
        if i < len(gauss_mask):
            m = gauss_mask[i]
        else:
            m = gauss_mask[-1]
        # Resize mask if needed
        if m.shape[:2] != lap_img[i].shape[:2]:
            from PIL import Image as PILImage
            m_pil = PILImage.fromarray((m[:,:,0] * 255).astype(np.uint8) if len(m.shape) == 3 else (m * 255).astype(np.uint8))
            m_pil = m_pil.resize((lap_img[i].shape[1], lap_img[i].shape[0]), PILImage.LANCZOS)
            m = np.array(m_pil, dtype=np.float64) / 255.0
            if len(lap_img[i].shape) == 3:
                m = m[:, :, np.newaxis]

        blended = lap_img[i] * m + lap_shifted[i] * (1 - m)
        lap_blended.append(blended)

    # Reconstruct
    result = reconstruct_from_laplacian(lap_blended)
    result = np.clip(result, 0, 255).astype(np.uint8)
    Image.fromarray(result).save(output_path)


def make_seamless_simple_strong(img_path, output_path):
    """
    Fallback: very aggressive cosine cross-blend with 40% border.
    Simpler but effective.
    """
    img = np.array(Image.open(img_path), dtype=np.float64)
    H, W = img.shape[:2]
    shifted = np.roll(np.roll(img, W // 2, axis=1), H // 2, axis=0)

    # 40% border — very aggressive blending
    border_frac = 0.40
    bw = int(W * border_frac)
    bh = int(H * border_frac)
    ramp = lambda n: 0.5 * (1 + np.cos(np.linspace(0, np.pi, n)))

    wx = np.ones(W)
    wx[:bw] = ramp(bw)
    wx[-bw:] = ramp(bw)[::-1]
    wy = np.ones(H)
    wy[:bh] = ramp(bh)
    wy[-bh:] = ramp(bh)[::-1]

    mask = (wy[:, None] * wx[None, :])[:, :, np.newaxis]
    result = (img * mask + shifted * (1 - mask)).clip(0, 255).astype(np.uint8)
    Image.fromarray(result).save(output_path)


def tile_preview(img_path, preview_path):
    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w * 3, h * 3))
    for r in range(3):
        for c in range(3):
            preview.paste(img, (c * w, r * h))
    if preview.width > 2048:
        ratio = 2048 / preview.width
        preview = preview.resize((2048, int(preview.height * ratio)), Image.LANCZOS)
    preview.save(preview_path, quality=90)


def check_seamless(img_path):
    arr = np.array(Image.open(img_path), dtype=np.float64)
    strip = 16
    h_err = np.abs(arr[:, :strip].mean(axis=1) - arr[:, -strip:].mean(axis=1)).mean()
    v_err = np.abs(arr[:strip, :].mean(axis=0) - arr[-strip:, :].mean(axis=0)).mean()
    return h_err, v_err


def main():
    pngs = sorted(glob.glob(os.path.join(INPUT_DIR, "*.png")))
    print(f"Processing {len(pngs)} textures with aggressive seamless fix...\n")

    for i, src in enumerate(pngs):
        name = os.path.splitext(os.path.basename(src))[0]
        out = os.path.join(OUTPUT_DIR, f"{name}.png")

        print(f"[{i+1}/{len(pngs)}] {name}")

        # Check current seamless quality
        h_err, v_err = check_seamless(src)
        print(f"  Before: h={h_err:.1f}, v={v_err:.1f}")

        # Try Laplacian pyramid blend first (best quality)
        try:
            make_seamless_laplacian(src, out, levels=4)
        except Exception as e:
            print(f"  Laplacian failed ({e}), using strong cross-blend")
            make_seamless_simple_strong(src, out)

        # Check result
        h_err2, v_err2 = check_seamless(out)
        print(f"  After:  h={h_err2:.1f}, v={v_err2:.1f}")

        # If still bad, apply strong cross-blend on top
        if h_err2 > 3.0 or v_err2 > 3.0:
            print(f"  Still not great, applying additional cross-blend...")
            make_seamless_simple_strong(out, out)
            h_err3, v_err3 = check_seamless(out)
            print(f"  Final:  h={h_err3:.1f}, v={v_err3:.1f}")

        # Preview
        tile_preview(out, os.path.join(PREVIEW_DIR, f"{name}_3x3.jpg"))

    print(f"\nDone! Output: {OUTPUT_DIR}")
    print(f"Previews: {PREVIEW_DIR}")


if __name__ == "__main__":
    main()
