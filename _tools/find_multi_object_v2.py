"""
Detect multi-object Hunyuan assets via the "K-pack pattern" signature.

When Hunyuan generates multiple copies (a cactus that's actually 3 cacti, a
fence that's actually 4 fences), the output mesh has K components of nearly
identical volume — much more uniform than the natural noise distribution
of a single fragmented object.

Heuristic:
  1. Get all connected mesh components, sorted by volume desc
  2. Look at top 2/3/4 components' size ratios
  3. If top-K sizes are within SIMILARITY_PCT of each other AND each is at
     least MIN_PCT of total mesh volume → flag as K-pack
  4. Verify spatial separation: top K components must be in distinct positions

This is much more selective than counting fragmented components.

Run:
  C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/find_multi_object_v2.py
"""
import csv
import json
import sys
from pathlib import Path

import trimesh

ROOT = Path("C:/Sabri_MMO/3d art/generated_assets")
REPORT_CSV = Path("C:/Sabri_MMO/3d art/multi_object_v2.csv")
REPORT_JSON = Path("C:/Sabri_MMO/3d art/multi_object_v2.json")

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

# K-pack pattern thresholds
SIMILARITY_PCT = 0.70  # smallest of top-K must be >= 70% of largest
MIN_TOTAL_PCT = 0.10   # each top-K component must be >= 10% of total volume


def analyze_glb(glb_path):
    try:
        m = trimesh.load(str(glb_path))
    except Exception as e:
        return {"error": f"load_failed: {e}"}

    if isinstance(m, trimesh.Scene):
        if not m.geometry:
            return {"error": "empty_scene"}
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

    # Per-component volume + bbox
    sizes, bboxes, centroids = [], [], []
    for c in components:
        try:
            v = abs(c.volume)
        except Exception:
            v = 0.0
        if v <= 0:
            v = len(c.faces) * 0.0001
        sizes.append(v)
        bboxes.append(c.bounds.tolist())
        centroids.append([(c.bounds[0][i] + c.bounds[1][i]) / 2 for i in range(3)])

    total_vol = sum(sizes)
    if total_vol <= 0:
        return {"error": "zero_total"}

    # Sort components by size, keep top 6 with original indices
    indexed = sorted(range(len(sizes)), key=lambda i: -sizes[i])[:6]
    top_sizes = [sizes[i] for i in indexed]
    top_bboxes = [bboxes[i] for i in indexed]

    # K-pack detection: try K=4, 3, 2 in that order
    detected_k = 0
    pack_sizes = []
    for k in [4, 3, 2]:
        if len(top_sizes) < k:
            continue
        candidates = top_sizes[:k]
        if min(candidates) < SIMILARITY_PCT * max(candidates):
            continue
        if min(candidates) < MIN_TOTAL_PCT * total_vol:
            continue
        seps = 0
        bbs = top_bboxes[:k]
        for i in range(k):
            for j in range(i + 1, k):
                bi, bj = bbs[i], bbs[j]
                separate = any(
                    bi[1][a] < bj[0][a] or bj[1][a] < bi[0][a]
                    for a in range(3)
                )
                if separate:
                    seps += 1
        max_pairs = k * (k - 1) // 2
        if seps >= max_pairs * 0.66:
            detected_k = k
            pack_sizes = candidates
            break

    # ---- Fragmented multi-object detection (cactus-like) ----
    # When 3+ identical objects are heavily fragmented, no single component
    # dominates. Detect by clustering centroids: if component centroids form
    # 2+ distinct clusters along the longest horizontal axis, it's multi-object.
    fragmented_k = 0
    cluster_info = ""
    if detected_k == 0 and len(centroids) >= 6:
        # Compute total mesh bounding box
        all_xyz = [(b[0], b[1]) for b in bboxes]
        mesh_min = [min(b[0][a] for b in bboxes) for a in range(3)]
        mesh_max = [max(b[1][a] for b in bboxes) for a in range(3)]
        mesh_extent = [mesh_max[a] - mesh_min[a] for a in range(3)]

        # Find axis with most extent (longest dimension)
        # For most assets: Z is vertical, X/Y are horizontal
        # Multi-object usually spreads along X or Y (horizontal ground placement)
        best_axis = 0
        best_score = 0
        best_clusters = []
        for axis in range(3):
            if mesh_extent[axis] <= 0:
                continue
            # Project centroids onto this axis, weighted by size
            pts = sorted([(centroids[i][axis], sizes[i]) for i in range(len(centroids))])
            # Find gaps: large gap relative to extent = cluster boundary
            # Use top-N largest components (by size) for cluster detection
            top_n_idx = sorted(range(len(sizes)), key=lambda i: -sizes[i])[:max(20, len(sizes) // 2)]
            top_pts = sorted([(centroids[i][axis], sizes[i]) for i in top_n_idx])
            if len(top_pts) < 3:
                continue
            # Look for gaps >= 25% of extent
            gap_threshold = 0.25 * mesh_extent[axis]
            cluster_boundaries = []
            for i in range(1, len(top_pts)):
                gap = top_pts[i][0] - top_pts[i-1][0]
                if gap >= gap_threshold:
                    cluster_boundaries.append(i)
            n_clusters = len(cluster_boundaries) + 1
            if n_clusters >= 2:
                # Assign each component to its cluster, sum mass per cluster
                cluster_masses = []
                start = 0
                for b in cluster_boundaries + [len(top_pts)]:
                    mass = sum(s for _, s in top_pts[start:b])
                    cluster_masses.append(mass)
                    start = b
                # Check if clusters are reasonably balanced (smallest >= 15% of largest)
                if min(cluster_masses) >= 0.15 * max(cluster_masses):
                    score = n_clusters * (min(cluster_masses) / max(cluster_masses))
                    if score > best_score:
                        best_score = score
                        best_axis = axis
                        best_clusters = cluster_masses

        if best_clusters:
            fragmented_k = len(best_clusters)
            cluster_info = f"axis={['X','Y','Z'][best_axis]} masses={[round(m,2) for m in best_clusters]}"

    return {
        "n_components": len(components),
        "total_volume": round(total_vol, 3),
        "top6_sizes": [round(s, 3) for s in top_sizes],
        "top6_pct_of_total": [round(100 * s / total_vol, 1) for s in top_sizes],
        "k_pack": detected_k,        # K-pack pattern (clean copies)
        "fragmented_k": fragmented_k,  # Heavily-fragmented multi-cluster pattern
        "cluster_info": cluster_info,
        "pack_sizes": [round(s, 3) for s in pack_sizes],
    }


def main():
    rows = []
    total = 0
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
            r = analyze_glb(glb)
            r["category"] = cat
            r["asset_id"] = asset_dir.name
            r["glb_path"] = str(glb)
            rows.append(r)
            if total % 25 == 0:
                print(f"  ...{total} analyzed", flush=True)

    # Group by detection type
    k4 = [r for r in rows if r.get("k_pack") == 4]
    k3 = [r for r in rows if r.get("k_pack") == 3]
    k2 = [r for r in rows if r.get("k_pack") == 2]
    fragmented = [r for r in rows if r.get("k_pack") == 0 and r.get("fragmented_k", 0) >= 2]
    single = [r for r in rows
              if r.get("k_pack") == 0 and r.get("fragmented_k", 0) < 2
              and "error" not in r]
    errors = [r for r in rows if "error" in r]

    print()
    print("=" * 78)
    print(f"MULTI-OBJECT REPORT (K-pack + fragmented detection)")
    print(f"  Analyzed:    {total} base assets")
    print(f"  4-pack:      {len(k4)}  (4 near-identical clean copies)")
    print(f"  3-pack:      {len(k3)}  (3 near-identical clean copies)")
    print(f"  2-pack:      {len(k2)}  (2 near-identical clean copies)")
    print(f"  Fragmented:  {len(fragmented)}  (heavily fragmented multi-object e.g. cactus)")
    print(f"  Single:      {len(single)}")
    print(f"  Errors:      {len(errors)}")
    print("=" * 78)

    def print_pack(label, lst):
        if not lst:
            return
        print(f"\n--- {label} ({len(lst)}) ---")
        print(f"  {'Category':14s} {'Asset':40s} pack_sizes (pct of total)")
        print(f"  {'-'*14} {'-'*40} {'-'*40}")
        for r in sorted(lst, key=lambda x: -sum(x["pack_sizes"])):
            sizes = r.get("pack_sizes", [])
            tot = r.get("total_volume", 0)
            pcts = [f"{100*s/tot:.0f}%" if tot else "?" for s in sizes]
            sizes_str = ", ".join(f"{s:.2f} ({p})" for s, p in zip(sizes, pcts))
            print(f"  {r['category']:14s} {r['asset_id']:40s} [{sizes_str}]")

    print_pack("4-PACK (4 copies)", k4)
    print_pack("3-PACK (3 copies)", k3)
    print_pack("2-PACK (2 copies)", k2)

    # Fragmented multi-object section (different format)
    if fragmented:
        print(f"\n--- FRAGMENTED MULTI-OBJECT ({len(fragmented)}) ---")
        print(f"  Heavily-fragmented mesh with 2+ spatial clusters along an axis.")
        print(f"  No clean K-pack pattern but components form distinct groups.")
        print()
        print(f"  {'Category':14s} {'Asset':40s} clusters  cluster_info")
        print(f"  {'-'*14} {'-'*40} {'-'*8}  {'-'*40}")
        for r in sorted(fragmented, key=lambda x: -x["fragmented_k"]):
            print(f"  {r['category']:14s} {r['asset_id']:40s} "
                  f"{r['fragmented_k']:>8d}  {r.get('cluster_info', '')}")

    if errors:
        print(f"\n--- ERRORS ({len(errors)}) ---")
        for r in errors:
            print(f"  {r['category']}/{r['asset_id']}: {r['error']}")

    # CSV
    rows.sort(key=lambda r: (
        -r.get("k_pack", 0),
        -r.get("fragmented_k", 0),
        r["category"], r["asset_id"]
    ))
    with open(REPORT_CSV, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow([
            "k_pack", "fragmented_k", "category", "asset_id",
            "n_components", "total_volume",
            "pack_sizes", "top6_sizes", "top6_pct_of_total",
            "cluster_info", "error", "glb_path",
        ])
        for r in rows:
            w.writerow([
                r.get("k_pack", 0),
                r.get("fragmented_k", 0),
                r.get("category", ""),
                r.get("asset_id", ""),
                r.get("n_components", ""),
                r.get("total_volume", ""),
                ";".join(map(str, r.get("pack_sizes", []))),
                ";".join(map(str, r.get("top6_sizes", []))),
                ";".join(map(str, r.get("top6_pct_of_total", []))),
                r.get("cluster_info", ""),
                r.get("error", ""),
                r.get("glb_path", ""),
            ])

    with open(REPORT_JSON, "w", encoding="utf-8") as f:
        json.dump(rows, f, indent=2)

    print(f"\nReport written to:")
    print(f"  {REPORT_CSV}")
    print(f"  {REPORT_JSON}")


if __name__ == "__main__":
    main()
