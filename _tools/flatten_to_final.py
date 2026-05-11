"""
Flatten generated_assets/<category>/<id>/{06_final.glb,variants/*/06_final.glb}
into Final/<category>/<id>.glb + Final/<category>/<id>_<variant>.glb

Skips bad-quality assets (per canonical pipeline thresholds) and their variants.

Trial mode (--trial): processes 3 assets per category for review.
Full mode: processes everything that passes quality.
"""
import argparse
import json
import shutil
from pathlib import Path

ROOT = Path(r"C:/Sabri_MMO/3d art/generated_assets")
FINAL = Path(r"C:/Sabri_MMO/3d art/Final")

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']


def is_good(depth_ratio, volume):
    """Canonical pipeline thresholds (Section 0)."""
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
    """Returns (is_good_bool, reason, depth, volume, score)."""
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


def pick_trial(cat, asset_ids):
    """Alphabetical first 3, but force barn_wooden first in architecture."""
    picks = []
    if cat == 'architecture' and 'barn_wooden' in asset_ids:
        picks.append('barn_wooden')
    for aid in sorted(asset_ids):
        if aid not in picks:
            picks.append(aid)
        if len(picks) >= 3:
            break
    return picks


def cleanup_bad_files(bad_assets_by_cat):
    """Remove leftover Final/ files for bad assets.

    Only deletes EXACT base + ACTUAL variant filenames (enumerated from source),
    never prefix-matches — otherwise good asset 'tree_maple_yellow' would be
    falsely matched as a variant of bad asset 'tree_maple'.
    """
    removed = []
    for cat, bad_ids in bad_assets_by_cat.items():
        catdir = FINAL / cat
        if not catdir.exists():
            continue
        for bad_id in bad_ids:
            # Base file
            targets = [f"{bad_id}.glb"]
            # Actual variants (only what exists in source)
            src_variants = ROOT / cat / bad_id / "variants"
            if src_variants.exists():
                for v in src_variants.iterdir():
                    if v.is_dir():
                        targets.append(f"{bad_id}_{v.name}.glb")
            for filename in targets:
                f = catdir / filename
                if f.exists():
                    f.unlink()
                    removed.append(str(f.relative_to(FINAL)))
    return removed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--trial", action="store_true",
                    help="Only process 3 assets per category")
    args = ap.parse_args()

    # Create top-level dirs
    for cat in CATEGORIES:
        (FINAL / cat).mkdir(parents=True, exist_ok=True)

    # First pass: determine bad assets across all categories
    bad_assets_by_cat = {}
    for cat in CATEGORIES:
        catdir = ROOT / cat
        if not catdir.exists():
            continue
        bad_ids = []
        for asset_dir in sorted(catdir.iterdir()):
            if not asset_dir.is_dir():
                continue
            ok, reason, d, v, s = asset_quality(asset_dir)
            if not ok:
                bad_ids.append(asset_dir.name)
        bad_assets_by_cat[cat] = bad_ids

    # Cleanup leftover bad files in Final/ (e.g. from trial run)
    print("=== Cleanup pass ===")
    removed = cleanup_bad_files(bad_assets_by_cat)
    if removed:
        print(f"Removed {len(removed)} bad-quality leftovers:")
        for r in removed:
            print(f"  - {r}")
    else:
        print("No leftover bad files to remove.")

    # Process
    total_base = 0
    total_variants = 0
    skipped_bad = 0
    skipped_no_base = []

    for cat in CATEGORIES:
        catdir = ROOT / cat
        if not catdir.exists():
            print(f"[skip] {cat}: directory not found")
            continue

        all_asset_ids = sorted([d.name for d in catdir.iterdir() if d.is_dir()])
        picks = pick_trial(cat, all_asset_ids) if args.trial else all_asset_ids

        print(f"\n=== {cat} ({len(picks)} candidates) ===")
        for aid in picks:
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
                skipped_no_base.append(f"{cat}/{aid}")
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
    if skipped_no_base:
        print(f"Skipped (no base 06_final.glb): {len(skipped_no_base)}")
        for s in skipped_no_base:
            print(f"  - {s}")
    print(f"Output: {FINAL}")


if __name__ == "__main__":
    main()
