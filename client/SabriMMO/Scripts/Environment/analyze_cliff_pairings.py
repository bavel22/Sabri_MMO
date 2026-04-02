# analyze_cliff_pairings.py
# Analyzes 608 RO Original texture PNGs by average color, then pairs
# each "ground-like" texture with a visually distinct "rock-like" cliff.
#
# Run OUTSIDE UE5 with system Python (needs Pillow):
#   python analyze_cliff_pairings.py
#
# Outputs: cliff_pairings.json — mapping { "001": "342", "002": "415", ... }

import os
import sys
import json
import math

# Add PIL
from PIL import Image

SRC_DIR = r"C:\Sabri_MMO\client\SabriMMO\Content\SabriMMO\Textures\Environment\RO_Original"
OUT_FILE = os.path.join(SRC_DIR, "cliff_pairings.json")

# ============================================================
# Step 1: Compute average color + saturation for each texture
# ============================================================

print("Analyzing texture colors...")

textures = {}
png_files = sorted([f for f in os.listdir(SRC_DIR) if f.endswith(".png")])
print(f"  Found {len(png_files)} PNG files")

for i, fname in enumerate(png_files):
    name = os.path.splitext(fname)[0]
    path = os.path.join(SRC_DIR, fname)
    try:
        img = Image.open(path).convert("RGB")
        # Downsample to 16x16 for speed
        img = img.resize((16, 16), Image.LANCZOS)
        pixels = list(img.getdata())
        r_avg = sum(p[0] for p in pixels) / len(pixels)
        g_avg = sum(p[1] for p in pixels) / len(pixels)
        b_avg = sum(p[2] for p in pixels) / len(pixels)

        # Brightness (0-255)
        brightness = (r_avg + g_avg + b_avg) / 3.0

        # Saturation: how colorful vs gray (0-1)
        max_c = max(r_avg, g_avg, b_avg)
        min_c = min(r_avg, g_avg, b_avg)
        saturation = (max_c - min_c) / max_c if max_c > 0 else 0

        # Green dominance: how much more green than red/blue
        green_dom = g_avg - (r_avg + b_avg) / 2.0

        textures[name] = {
            "r": r_avg, "g": g_avg, "b": b_avg,
            "brightness": brightness,
            "saturation": saturation,
            "green_dom": green_dom,
        }
    except Exception as e:
        print(f"  SKIP {fname}: {e}")

    if (i + 1) % 100 == 0:
        print(f"  [{i+1}/{len(png_files)}]")

print(f"  Analyzed {len(textures)} textures")

# ============================================================
# Step 2: Classify textures as "rock" (cliff candidates)
# ============================================================
# Rock textures: low saturation, moderate-dark brightness, gray/brown
# Grass textures: higher saturation or green dominance

ROCK_THRESHOLD_SAT = 0.15     # saturation below this = rocky
ROCK_THRESHOLD_GREEN = -10.0  # green dominance below this = not grassy

rock_textures = []
grass_textures = []

for name, data in textures.items():
    is_rock = (data["saturation"] < ROCK_THRESHOLD_SAT or
               (data["green_dom"] < ROCK_THRESHOLD_GREEN and data["saturation"] < 0.25))
    if is_rock:
        rock_textures.append(name)
    else:
        grass_textures.append(name)

print(f"  Classification: {len(rock_textures)} rock, {len(grass_textures)} grass/organic")

# If very few rocks, lower the threshold
if len(rock_textures) < 50:
    print("  Adjusting thresholds (too few rocks)...")
    ROCK_THRESHOLD_SAT = 0.20
    rock_textures = []
    grass_textures = []
    for name, data in textures.items():
        is_rock = (data["saturation"] < ROCK_THRESHOLD_SAT or
                   (data["brightness"] < 80 and data["saturation"] < 0.30))
        if is_rock:
            rock_textures.append(name)
        else:
            grass_textures.append(name)
    print(f"  Adjusted: {len(rock_textures)} rock, {len(grass_textures)} grass/organic")

# ============================================================
# Step 3: For each texture, find the best cliff match
# ============================================================
# Each ground texture gets paired with the rock texture closest in
# brightness but different enough in saturation to look distinct.

def color_distance(a, b):
    """Euclidean distance in RGB space, weighted toward brightness match."""
    dr = a["r"] - b["r"]
    dg = a["g"] - b["g"]
    db = a["b"] - b["b"]
    # Weight brightness match heavily (we want similar tone, different texture)
    dbr = a["brightness"] - b["brightness"]
    return math.sqrt(dr*dr + dg*dg + db*db + dbr*dbr * 2)

print("Computing pairings...")

pairings = {}

# If we have enough rock textures, pair grass->rock
if len(rock_textures) >= 10:
    for name in textures:
        if name in rock_textures:
            # Rock textures pair with themselves (they ARE cliff textures)
            # But still find a different rock for variety
            candidates = [r for r in rock_textures if r != name]
        else:
            candidates = rock_textures

        if not candidates:
            pairings[name] = name  # fallback: self
            continue

        src_data = textures[name]
        best = min(candidates, key=lambda c: color_distance(src_data, textures[c]))
        pairings[name] = best
else:
    # Not enough rocks identified — use brightness-offset pairing
    # Pair each texture with one that's 30% darker (cliff = darker ground)
    print("  Not enough rocks — using brightness-offset pairing")
    sorted_by_brightness = sorted(textures.keys(), key=lambda n: textures[n]["brightness"])
    total = len(sorted_by_brightness)

    # Build index lookup
    brightness_idx = {name: i for i, name in enumerate(sorted_by_brightness)}

    for name in textures:
        idx = brightness_idx[name]
        # Pair with texture ~30% of the way toward the dark end
        offset = max(1, total // 3)
        cliff_idx = (idx - offset) % total
        pairings[name] = sorted_by_brightness[cliff_idx]

# ============================================================
# Step 4: Output
# ============================================================

with open(OUT_FILE, "w") as f:
    json.dump(pairings, f, indent=2, sort_keys=True)

print(f"\nSUCCESS: {len(pairings)} pairings written to {OUT_FILE}")

# Stats
unique_cliffs = set(pairings.values())
self_paired = sum(1 for k, v in pairings.items() if k == v)
print(f"  Unique cliff textures used: {len(unique_cliffs)}")
print(f"  Self-paired (no better match): {self_paired}")

# Show sample pairings
print("\nSample pairings (ground -> cliff):")
for name in sorted(pairings.keys())[:10]:
    cliff = pairings[name]
    gd = textures[name]
    cd = textures.get(cliff, {})
    print(f"  {name} (sat={gd['saturation']:.2f} br={gd['brightness']:.0f}) -> "
          f"{cliff} (sat={cd.get('saturation',0):.2f} br={cd.get('brightness',0):.0f})")
