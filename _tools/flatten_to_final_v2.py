"""
Flatten v2 regenerated assets into Final_v2/ — mirror of flatten_to_final.py
but filtered to assets with IDs ending in `_v2`.

Output:
  Final_v2/<category>/<asset>_v2.glb
  Final_v2/<category>/<asset>_v2_<variant>.glb

Skips bad-quality assets (per canonical pipeline thresholds) and their variants.
"""
import argparse
import json
import shutil
from pathlib import Path

ROOT = Path(r"C:/Sabri_MMO/3d art/generated_assets")
FINAL = Path(r"C:/Sabri_MMO/3d art/Final_v2")

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']


def is_good(depth_ratio, volume):
    score = depth_ratio * volume
    if score < 0.15:
        return False, "low_score"
    if volume > 5.0:
        return False, "cube_artifact"
    if volume < 0.05:
        return False, "thin_shell"
    if depth_ratio < 0.4:
        return False, "flat"
    return True, "good"


def asset_quality(asset_dir):
    mp = asset_dir / "manifest.json"
    if not mp.exists():
        return False, "no_manifest", 0, 0, 0
    try:
        m = json.loads(mp.read_text())
        sweep = m.get("stages", {}).get("mesh_sweep", {})
        d = sweep.get("depth_ratio", 0)
        v = sweep.get("volume", 0)
        ok, reason = is_good(d, v)
        return ok, reason, d, v, d * v
    except Exception as e:
        return False, f"manifest_error:{e}", 0, 0, 0


def main():
    for cat in CATEGORIES:
        (FINAL / cat).mkdir(parents=True, exist_ok=True)

    total_base = 0
    total_variants = 0
    skipped_bad = 0

    for cat in CATEGORIES:
        catdir = ROOT / cat
        if not catdir.exists():
            continue

        # Only v2 assets
        v2_ids = sorted([d.name for d in catdir.iterdir()
                         if d.is_dir() and d.name.endswith("_v2")])

        if not v2_ids:
            continue

        print(f"\n=== {cat} ({len(v2_ids)} v2 candidates) ===")
        for aid in v2_ids:
            asset_dir = catdir / aid
            ok, reason, d, v, s = asset_quality(asset_dir)
            if not ok:
                print(f"  [SKIP bad: {reason}, score={s:.3f}] {aid}")
                skipped_bad += 1
                continue

            # Base
            base_src = asset_dir / "06_final.glb"
            if base_src.exists():
                base_dst = FINAL / cat / f"{aid}.glb"
                shutil.copy2(base_src, base_dst)
                size_kb = base_src.stat().st_size // 1024
                print(f"  {aid}.glb  ({size_kb} KB, score={s:.2f})")
                total_base += 1
            else:
                print(f"  [no 06_final.glb] {aid}")
                continue

            # Variants
            vdir = asset_dir / "variants"
            if vdir.exists():
                for v_dir in sorted(vdir.iterdir()):
                    if not v_dir.is_dir():
                        continue
                    v_src = v_dir / "06_final.glb"
                    if v_src.exists():
                        v_dst = FINAL / cat / f"{aid}_{v_dir.name}.glb"
                        shutil.copy2(v_src, v_dst)
                        size_kb = v_src.stat().st_size // 1024
                        print(f"  {aid}_{v_dir.name}.glb  ({size_kb} KB)")
                        total_variants += 1

    print(f"\n{'=' * 60}")
    print(f"DONE: {total_base} base + {total_variants} variants = "
          f"{total_base + total_variants} files copied")
    print(f"Skipped {skipped_bad} bad-quality assets")
    print(f"Output: {FINAL}")


if __name__ == "__main__":
    main()
