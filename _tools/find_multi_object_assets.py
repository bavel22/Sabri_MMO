"""
Detect multi-object Hunyuan assets — models that contain 2+ separate objects
when they should be single (e.g., a "cactus" model with 3 cacti).

The key insight: AI-generated meshes are heavily fragmented by marching-cubes
artifacts (a "single" tree may have 100+ disconnected leaf-pieces). To find
TRUE multi-object failures, we need to filter aggressively:

  - Only count components that are at least SIG_PCT (default 30%) of the
    largest component's volume — fragments are excluded
  - Check spatial separation between these large components via bbox no-overlap

Score each asset:
  - HIGH:   2+ comparable-size components, all spatially separate
  - MEDIUM: 2+ comparable-size components, partially overlapping
  - LOW:    fragmented but appears single-object

Skips variants (they share geometry with their base).

Run:
  C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/find_multi_object_assets.py
"""
import csv
import json
import sys
from pathlib import Path

import trimesh

ROOT = Path("C:/Sabri_MMO/3d art/generated_assets")
REPORT_CSV = Path("C:/Sabri_MMO/3d art/multi_object_report.csv")
REPORT_JSON = Path("C:/Sabri_MMO/3d art/multi_object_report.json")

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

# A "comparable" component must be at least this fraction of largest's volume.
# 0.30 means each comparable piece is at least 30% of the largest piece's volume.
# Lower = more sensitive to multi-object, more false positives from mesh noise.
COMPARABLE_PCT = 0.30
# Strict threshold for HIGH-confidence multi (e.g. 4-cactus AI output)
HIGH_CONF_PCT = 0.50


def analyze_glb(glb_path):
    """Returns dict with multi-object analysis."""
    try:
        m = trimesh.load(str(glb_path))
    except Exception as e:
        return {"error": f"load_failed: {e}"}

    if isinstance(m, trimesh.Scene):
        if not m.geometry:
            return {"error": "empty_scene"}
        # Combine all geometries into one mesh for analysis
        g = trimesh.util.concatenate(list(m.geometry.values()))
    else:
        g = m

    if not hasattr(g, "vertices") or len(g.vertices) == 0:
        return {"error": "no_vertices"}

    try:
        components = g.split(only_watertight=False)
    except Exception as e:
        return {"error": f"split_failed: {e}"}

    if len(components) == 0:
        return {"error": "no_components"}

    if len(components) == 1:
        return {
            "n_components": 1,
            "n_comparable": 1,
            "n_high_conf": 1,
            "n_separated_pairs_comparable": 0,
            "n_separated_pairs_high_conf": 0,
            "comparable_sizes": [],
            "confidence": "single",
        }

    # Per-component size: prefer volume (proper 3D mass), fall back to face count
    sizes, bboxes = [], []
    for c in components:
        try:
            v = abs(c.volume)
        except Exception:
            v = 0.0
        if v <= 0:
            v = len(c.faces) * 0.0001  # surrogate
        sizes.append(v)
        bboxes.append(c.bounds.tolist())

    max_size = max(sizes)
    if max_size <= 0:
        return {"error": "zero_size"}

    # Two thresholds: HIGH_CONF_PCT for confident multi-object detection,
    # COMPARABLE_PCT for medium-confidence
    high_conf_idx = [i for i, s in enumerate(sizes) if s >= HIGH_CONF_PCT * max_size]
    comparable_idx = [i for i, s in enumerate(sizes) if s >= COMPARABLE_PCT * max_size]

    def count_separated_pairs(indices):
        n = 0
        for ii in range(len(indices)):
            for jj in range(ii + 1, len(indices)):
                i, j = indices[ii], indices[jj]
                bi, bj = bboxes[i], bboxes[j]
                separate = any(
                    bi[1][axis] < bj[0][axis] or bj[1][axis] < bi[0][axis]
                    for axis in range(3)
                )
                if separate:
                    n += 1
        return n

    n_sep_high = count_separated_pairs(high_conf_idx)
    n_sep_comp = count_separated_pairs(comparable_idx)

    # Confidence:
    #   HIGH   = 2+ near-equal-size components, separated (e.g. 4-cactus output)
    #   MEDIUM = 2+ moderately-sized components, separated (might be multi)
    #   LOW    = fragmented but appears single-object (mesh noise dominated)
    if len(high_conf_idx) >= 2 and n_sep_high >= 1:
        confidence = "HIGH"
    elif len(comparable_idx) >= 2 and n_sep_comp >= 1:
        confidence = "MEDIUM"
    else:
        confidence = "LOW"

    comp_sizes = sorted([sizes[i] for i in comparable_idx], reverse=True)[:6]

    return {
        "n_components": len(components),
        "n_comparable": len(comparable_idx),
        "n_high_conf": len(high_conf_idx),
        "n_separated_pairs_comparable": n_sep_comp,
        "n_separated_pairs_high_conf": n_sep_high,
        "comparable_sizes": [round(s, 3) for s in comp_sizes],
        "confidence": confidence,
    }


def main():
    rows = []
    total = 0
    errors = 0

    for cat in CATEGORIES:
        cat_dir = ROOT / cat
        if not cat_dir.exists():
            continue
        for asset_dir in sorted(cat_dir.iterdir()):
            if not asset_dir.is_dir():
                continue
            glb = asset_dir / "06_final.glb"
            if not glb.exists():
                continue
            total += 1
            result = analyze_glb(glb)
            row = {
                "category": cat,
                "asset_id": asset_dir.name,
                "glb_path": str(glb),
                **result,
            }
            rows.append(row)
            if "error" in result:
                errors += 1
            if total % 25 == 0:
                print(f"  ...{total} analyzed", flush=True)

    # Counts by confidence
    high = [r for r in rows if r.get("confidence") == "HIGH"]
    medium = [r for r in rows if r.get("confidence") == "MEDIUM"]
    low = [r for r in rows if r.get("confidence") == "LOW"]
    single = [r for r in rows if r.get("confidence") == "single"]

    # Sort each tier
    def sort_key(r):
        return (-r.get("n_high_conf", 0),
                -r.get("n_separated_pairs_high_conf", 0),
                -r.get("n_comparable", 0))
    high.sort(key=sort_key)
    medium.sort(key=sort_key)

    # ---- Console report ----
    print("\n" + "=" * 78)
    print(f"MULTI-OBJECT ASSET REPORT")
    print(f"  Analyzed:        {total} base assets")
    print(f"  HIGH confidence: {len(high)} (likely multi-object)")
    print(f"  MED confidence:  {len(medium)} (review individually)")
    print(f"  LOW / single:    {len(low) + len(single)}")
    print(f"  Errors:          {errors}")
    print(f"  Thresholds: high={int(HIGH_CONF_PCT*100)}%, comp={int(COMPARABLE_PCT*100)}%")
    print("=" * 78)

    print(f"\n--- HIGH CONFIDENCE MULTI-OBJECT ({len(high)}) ---")
    print(f"  These likely contain 2+ comparable-size objects (50%+ of largest each).")
    print(f"  Examples: cactus with 3 cacti, fence with 4 fences, banner with 4 banners.")
    print()
    print(f"  {'Category':14s} {'Asset':40s} {'#hi':>4s} {'#cmp':>5s} {'sep':>4s}  comparable_sizes")
    print(f"  {'-'*14} {'-'*40} {'-'*4} {'-'*5} {'-'*4}  {'-'*30}")
    for r in high:
        sizes_str = ", ".join(f"{s:.2f}" for s in r.get("comparable_sizes", [])[:4])
        print(f"  {r['category']:14s} {r['asset_id']:40s} "
              f"{r['n_high_conf']:>4d} {r['n_comparable']:>5d} {r['n_separated_pairs_high_conf']:>4d}  [{sizes_str}]")

    print(f"\n--- MEDIUM CONFIDENCE ({len(medium)}) ---")
    print(f"  2+ moderate-size components (30-50%) — could be multi-object or noisy mesh.")
    print()
    print(f"  {'Category':14s} {'Asset':40s} {'#cmp':>5s} {'sep':>4s}  comparable_sizes")
    print(f"  {'-'*14} {'-'*40} {'-'*5} {'-'*4}  {'-'*30}")
    for r in medium[:50]:  # cap to top 50 for readability
        sizes_str = ", ".join(f"{s:.2f}" for s in r.get("comparable_sizes", [])[:4])
        print(f"  {r['category']:14s} {r['asset_id']:40s} "
              f"{r['n_comparable']:>5d} {r['n_separated_pairs_comparable']:>4d}  [{sizes_str}]")
    if len(medium) > 50:
        print(f"  ... and {len(medium) - 50} more (see CSV for full list)")

    if errors:
        print(f"\n--- ERRORS ({errors}) ---")
        for r in rows:
            if "error" in r:
                print(f"  {r['category']}/{r['asset_id']}: {r['error']}")

    # ---- CSV ----
    rows.sort(key=lambda r: (
        {"HIGH": 0, "MEDIUM": 1, "LOW": 2, "single": 3}.get(r.get("confidence", "single"), 4),
        sort_key(r),
    ))
    with open(REPORT_CSV, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow([
            "confidence", "category", "asset_id",
            "n_components", "n_comparable", "n_high_conf",
            "n_separated_pairs_comparable", "n_separated_pairs_high_conf",
            "comparable_sizes", "error", "glb_path",
        ])
        for r in rows:
            w.writerow([
                r.get("confidence", ""),
                r.get("category", ""),
                r.get("asset_id", ""),
                r.get("n_components", ""),
                r.get("n_comparable", ""),
                r.get("n_high_conf", ""),
                r.get("n_separated_pairs_comparable", ""),
                r.get("n_separated_pairs_high_conf", ""),
                ";".join(map(str, r.get("comparable_sizes", []))),
                r.get("error", ""),
                r.get("glb_path", ""),
            ])

    # ---- JSON (full data) ----
    with open(REPORT_JSON, "w", encoding="utf-8") as f:
        json.dump(rows, f, indent=2)

    print(f"\nReport written to:")
    print(f"  {REPORT_CSV}")
    print(f"  {REPORT_JSON}")


if __name__ == "__main__":
    main()
