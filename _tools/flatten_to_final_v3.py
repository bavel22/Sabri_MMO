"""
Flatten v3 + alt-mesh assets into Final_v3/.

Sources:
  1. v3 assets (asset_id in V3_ASSETS list from hunyuan_v3_regional.py)
       generated_assets/<cat>/<asset>/06_final.glb -> Final_v3/<cat>/<asset>.glb
       generated_assets/<cat>/<asset>/variants/<sfx>/06_final.glb
                                              -> Final_v3/<cat>/<asset>_<sfx>.glb

  2. Alt-mesh assets (any folder with manifest.alt_mesh=True, named <base>_v<N>)
       generated_assets/<cat>/<base>_v<N>/06_final.glb
                                          -> Final_v3/<cat>/<base>_v<N>.glb
       generated_assets/<cat>/<base>_v<N>/variants/<sfx>/06_final.glb
                                          -> Final_v3/<cat>/<base>_<sfx>_v<N>.glb

Skips bad-quality assets (per pipeline thresholds, except alt-mesh manifests
which lack mesh_sweep stats — those are accepted as-is).
"""
import json
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, "C:/Sabri_MMO/_tools")
from hunyuan_v3_regional import V3_ASSETS

ROOT = Path(r"C:/Sabri_MMO/3d art/generated_assets")
FINAL = Path(r"C:/Sabri_MMO/3d art/Final_v3")

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

V3_IDS = {a["id"] for a in V3_ASSETS}
ALT_MESH_PATTERN = re.compile(r'^(.+?)_v(\d+)$')


def is_good(d, v):
    s = d * v
    if s < 0.15: return False, "low_score"
    if v > 5.0: return False, "cube_artifact"
    if v < 0.05: return False, "thin_shell"
    if d < 0.4: return False, "flat"
    return True, "good"


def asset_quality(asset_dir):
    """Returns (is_good, reason, d, v, score). Alt-mesh manifests are accepted as-is."""
    mp = asset_dir / "manifest.json"
    if not mp.exists():
        return False, "no_manifest", 0, 0, 0
    try:
        m = json.loads(mp.read_text())
        if m.get("alt_mesh"):
            # Alt-mesh manifests have no mesh_sweep — accept if final exists
            return True, "alt_ok", 0, 0, 0
        sweep = m.get("stages", {}).get("mesh_sweep", {})
        d = sweep.get("depth_ratio", 0)
        v = sweep.get("volume", 0)
        ok, reason = is_good(d, v)
        return ok, reason, d, v, d * v
    except Exception as e:
        return False, f"manifest_error:{e}", 0, 0, 0


def is_alt_mesh(asset_dir):
    mp = asset_dir / "manifest.json"
    if not mp.exists():
        return False
    try:
        return bool(json.loads(mp.read_text()).get("alt_mesh"))
    except Exception:
        return False


def copy_v3_asset(cat, aid, asset_dir, stats):
    """Copy v3 base + variants. Returns (base_count, variant_count)."""
    base_count = 0
    variant_count = 0
    base_src = asset_dir / "06_final.glb"
    if not base_src.exists():
        return 0, 0

    base_dst = FINAL / cat / f"{aid}.glb"
    shutil.copy2(base_src, base_dst)
    print(f"  {aid}.glb  ({base_src.stat().st_size // 1024} KB, score={stats:.2f})")
    base_count = 1

    vdir = asset_dir / "variants"
    if vdir.exists():
        for v_dir in sorted(vdir.iterdir()):
            if not v_dir.is_dir():
                continue
            v_src = v_dir / "06_final.glb"
            if v_src.exists():
                v_dst = FINAL / cat / f"{aid}_{v_dir.name}.glb"
                shutil.copy2(v_src, v_dst)
                print(f"  {aid}_{v_dir.name}.glb  ({v_src.stat().st_size // 1024} KB)")
                variant_count += 1
    return base_count, variant_count


def copy_alt_mesh(cat, asset_dir):
    """Copy alt-mesh base + variants. Output naming swaps variant before _v<N>.

    Source: generated_assets/<cat>/<base>_v<N>/06_final.glb
    Dest:   Final_v3/<cat>/<base>_v<N>.glb

    Source: generated_assets/<cat>/<base>_v<N>/variants/<sfx>/06_final.glb
    Dest:   Final_v3/<cat>/<base>_<sfx>_v<N>.glb
    """
    asset_id = asset_dir.name
    match = ALT_MESH_PATTERN.match(asset_id)
    if not match:
        return 0, 0
    base = match.group(1)
    v = match.group(2)

    base_count = 0
    variant_count = 0
    base_src = asset_dir / "06_final.glb"
    if not base_src.exists():
        return 0, 0

    base_dst = FINAL / cat / f"{asset_id}.glb"
    shutil.copy2(base_src, base_dst)
    print(f"  {asset_id}.glb  (alt mesh, {base_src.stat().st_size // 1024} KB)")
    base_count = 1

    vdir = asset_dir / "variants"
    if vdir.exists():
        for v_dir in sorted(vdir.iterdir()):
            if not v_dir.is_dir():
                continue
            v_src = v_dir / "06_final.glb"
            if v_src.exists():
                v_dst = FINAL / cat / f"{base}_{v_dir.name}_v{v}.glb"
                shutil.copy2(v_src, v_dst)
                print(f"  {base}_{v_dir.name}_v{v}.glb  ({v_src.stat().st_size // 1024} KB)")
                variant_count += 1
    return base_count, variant_count


def main():
    for cat in CATEGORIES:
        (FINAL / cat).mkdir(parents=True, exist_ok=True)

    v3_base = 0
    v3_var = 0
    alt_base = 0
    alt_var = 0
    skipped_bad = 0

    # --- Pass 1: v3 assets ---
    print("=" * 70)
    print("PASS 1: v3 base assets (from V3_ASSETS list)")
    print("=" * 70)
    for cat in CATEGORIES:
        catdir = ROOT / cat
        if not catdir.exists():
            continue

        v3_in_cat = sorted([d.name for d in catdir.iterdir()
                            if d.is_dir() and d.name in V3_IDS])
        if not v3_in_cat:
            continue

        print(f"\n=== {cat} ({len(v3_in_cat)} v3 candidates) ===")
        for aid in v3_in_cat:
            asset_dir = catdir / aid
            ok, reason, d, v, s = asset_quality(asset_dir)
            if not ok:
                print(f"  [SKIP bad: {reason}, score={s:.3f}] {aid}")
                skipped_bad += 1
                continue
            b, vc = copy_v3_asset(cat, aid, asset_dir, s)
            v3_base += b
            v3_var += vc

    # --- Pass 2: alt-mesh assets ---
    print()
    print("=" * 70)
    print("PASS 2: alt-mesh assets (manifest.alt_mesh = True)")
    print("=" * 70)
    for cat in CATEGORIES:
        catdir = ROOT / cat
        if not catdir.exists():
            continue

        alt_in_cat = sorted([d.name for d in catdir.iterdir()
                             if d.is_dir() and is_alt_mesh(d)])
        if not alt_in_cat:
            continue

        print(f"\n=== {cat} ({len(alt_in_cat)} alt-mesh assets) ===")
        for aid in alt_in_cat:
            asset_dir = catdir / aid
            b, vc = copy_alt_mesh(cat, asset_dir)
            alt_base += b
            alt_var += vc

    print(f"\n{'=' * 60}")
    print(f"DONE")
    print(f"  v3 base:        {v3_base}")
    print(f"  v3 variants:    {v3_var}")
    print(f"  alt-mesh base:  {alt_base}")
    print(f"  alt-mesh var:   {alt_var}")
    total = v3_base + v3_var + alt_base + alt_var
    print(f"  TOTAL:          {total} files")
    print(f"  Skipped bad:    {skipped_bad}")
    print(f"  Output:         {FINAL}")


if __name__ == "__main__":
    main()
