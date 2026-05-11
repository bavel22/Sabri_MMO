"""
flatten_combined.py — Combine Final/, Final_v2/, Final_v3/, and alt-mesh
outputs from generated_assets/<cat>/<asset>_v<N>/ into Final_combined/.

Result: a single folder per category where everything for each asset_id is
listed alphabetically together (originals + v2 + v3 + alt meshes).

Naming for alt mesh outputs:
  generated_assets/<cat>/<base>_v<N>/06_final.glb
    -> Final_combined/<cat>/<base>_v<N>.glb
  generated_assets/<cat>/<base>_v<N>/variants/<suffix>/06_final.glb
    -> Final_combined/<cat>/<base>_<suffix>_v<N>.glb

Run:
  C:/Users/pladr/AppData/Local/Programs/Python/Python312/python.exe C:/Sabri_MMO/_tools/flatten_combined.py
"""
import shutil
import re
import json
from pathlib import Path

ROOT = Path("C:/Sabri_MMO/3d art")
GENERATED = ROOT / "generated_assets"
DEST = ROOT / "Final_combined"

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']

SOURCE_FINALS = [
    ROOT / "Imported_to_UE5" / "Final",
    ROOT / "Imported_to_UE5" / "Final_v2",
    ROOT / "Final_v3",
]

# Pattern to detect alt-mesh asset folders: anything ending in _v<N>
ALT_MESH_PATTERN = re.compile(r'^(.+?)_v(\d+)$')


def flatten_existing_final(source_root, dest_root):
    if not source_root.exists():
        return 0
    n = 0
    for cat in CATEGORIES:
        scat = source_root / cat
        if not scat.is_dir():
            continue
        dcat = dest_root / cat
        dcat.mkdir(parents=True, exist_ok=True)
        for f in scat.iterdir():
            if f.is_file() and f.suffix == ".glb":
                shutil.copy2(f, dcat / f.name)
                n += 1
    return n


def flatten_alt_meshes():
    n_base = 0
    n_var = 0
    n_skipped = 0

    for cat in CATEGORIES:
        cat_root = GENERATED / cat
        if not cat_root.exists():
            continue
        dcat = DEST / cat
        dcat.mkdir(parents=True, exist_ok=True)

        for asset_dir in sorted(cat_root.iterdir()):
            if not asset_dir.is_dir():
                continue

            mp = asset_dir / "manifest.json"
            if not mp.exists():
                continue
            try:
                m = json.loads(mp.read_text())
            except Exception:
                continue
            if not m.get("alt_mesh"):
                continue

            asset_id = asset_dir.name
            match = ALT_MESH_PATTERN.match(asset_id)
            if not match:
                continue
            base = match.group(1)
            v = match.group(2)

            base_src = asset_dir / "06_final.glb"
            if not base_src.exists():
                n_skipped += 1
                continue

            base_dst = dcat / f"{asset_id}.glb"
            shutil.copy2(base_src, base_dst)
            n_base += 1

            vdir = asset_dir / "variants"
            if vdir.exists():
                for v_dir in sorted(vdir.iterdir()):
                    if not v_dir.is_dir():
                        continue
                    v_src = v_dir / "06_final.glb"
                    if v_src.exists():
                        v_dst = dcat / f"{base}_{v_dir.name}_v{v}.glb"
                        shutil.copy2(v_src, v_dst)
                        n_var += 1

    return n_base, n_var, n_skipped


def main():
    DEST.mkdir(parents=True, exist_ok=True)

    print("=" * 70)
    print("FLATTEN COMBINED")
    print(f"  Output: {DEST}")
    print("=" * 70)
    print()

    total_existing = 0
    for src in SOURCE_FINALS:
        if src.exists():
            n = flatten_existing_final(src, DEST)
            print(f"  {src.name}: {n} files copied")
            total_existing += n
        else:
            print(f"  {src.name}: not found, skip")

    print(f"  Total from existing Finals: {total_existing}")
    print()

    print("Flattening alt-mesh outputs...")
    n_base, n_var, n_skip = flatten_alt_meshes()
    print(f"  Alt mesh base meshes: {n_base}")
    print(f"  Alt mesh variants:    {n_var}")
    print(f"  Skipped (incomplete): {n_skip}")
    print()

    grand_total = total_existing + n_base + n_var
    print(f"DONE. {grand_total} files in {DEST}")

    print("\nPer-category summary:")
    for cat in CATEGORIES:
        cdir = DEST / cat
        if cdir.exists():
            n = sum(1 for f in cdir.iterdir() if f.suffix == ".glb")
            print(f"  {cat:14s}: {n} files")


if __name__ == "__main__":
    main()
