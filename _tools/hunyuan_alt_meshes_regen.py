"""
Alt-mesh regeneration — for every remaining GLB in generated_assets/<cat>/<asset>/03_meshes/,
decimate it, texture it (using the original input image matching its strategy), and
generate 2 color variants. Names them with sequential _vN suffixes.

Naming:
  Source asset_id: house_prontera_corner
    Existing in Final/Final_v2/Final_v3: house_prontera_corner.glb (v1 implied), house_prontera_corner_v2.glb
    3 candidates in 03_meshes/ -> house_prontera_corner_v3, _v4, _v5
    Variants per: <base>_<variant_suffix>_v<N>

  Source asset_id with existing _vN suffix: house_prontera_corner_v2
    Sub-versions starting at v1: house_prontera_corner_v2_v1, _v2_v2, ...
    Variants per: <base>_v2_<variant_suffix>_v<N>

Waits for V3 to complete (polls _v3_regen.log for "V3 REGEN COMPLETE") before starting.

Run:
  C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/hunyuan_alt_meshes_regen.py
"""
import sys
import time
import json
import re
import shutil
import traceback
import urllib.request
import importlib.util
from collections import defaultdict
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)
sys.path.insert(0, "C:/Sabri_MMO/_tools")

import hunyuan_asset_pipeline as p
import hunyuan_color_variants as cv


def _load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


_b3 = _load("C:/Sabri_MMO/_tools/hunyuan_batch3_100.py", "hunyuan_batch3_100")
_v3m = _load("C:/Sabri_MMO/_tools/hunyuan_v3_regional.py", "hunyuan_v3_regional")
BATCH3_VARIANTS = _b3.BATCH3_VARIANTS
V3_VARIANTS = _v3m.V3_VARIANTS

ROOT = p.ROOT
LOG_FILE = ROOT / "_alt_meshes.log"
V3_LOG = ROOT / "_v3_regen.log"

WAIT_FOR_V3 = True

FINAL_DIRS = [
    Path("C:/Sabri_MMO/3d art/Imported_to_UE5/Final"),
    Path("C:/Sabri_MMO/3d art/Imported_to_UE5/Final_v2"),
    Path("C:/Sabri_MMO/3d art/Final_v3"),
]

CATEGORIES = ['architecture', 'centerpiece', 'dungeon', 'props',
              'special', 'terrain', 'vegetation']


def log(msg):
    ts = time.strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def wait_for_v3_complete():
    log("Waiting for V3 to complete (polling _v3_regen.log)...")
    while True:
        try:
            if V3_LOG.exists():
                text = V3_LOG.read_text(encoding="utf-8", errors="ignore")
                if "V3 REGEN COMPLETE" in text:
                    log("V3 detected as complete. Starting alt-mesh regen.")
                    return
        except Exception as e:
            log(f"  log read err: {e}")
        time.sleep(120)


def find_variants_for_asset(asset_id):
    """Look up variants from cv/batch3/v3 dicts."""
    if asset_id in cv.COLOR_VARIANTS:
        return cv.COLOR_VARIANTS[asset_id]
    if asset_id in BATCH3_VARIANTS:
        return BATCH3_VARIANTS[asset_id]
    if asset_id in V3_VARIANTS:
        return V3_VARIANTS[asset_id]
    return None


def find_next_version(source_asset_id, category):
    """Find next available v# for source_asset_id by checking Final*/<cat>/ files
    and any existing alt-mesh folders in generated_assets/<cat>/."""
    pattern_file = re.compile(rf'^{re.escape(source_asset_id)}_v(\d+)\.glb$')
    pattern_dir = re.compile(rf'^{re.escape(source_asset_id)}_v(\d+)$')
    versions = []

    for final_dir in FINAL_DIRS:
        cat_dir = final_dir / category
        if cat_dir.exists():
            for f in cat_dir.iterdir():
                m = pattern_file.match(f.name)
                if m:
                    versions.append(int(m.group(1)))

    cat_root = ROOT / category
    if cat_root.exists():
        for d in cat_root.iterdir():
            if d.is_dir():
                m = pattern_dir.match(d.name)
                if m:
                    versions.append(int(m.group(1)))

    # If asset_id has no _vN suffix, the original (no suffix) is implicitly v1.
    # If asset_id already has _vN, sub-versions start at v1.
    has_suffix = bool(re.search(r'_v\d+$', source_asset_id))
    if not versions:
        return 2 if not has_suffix else 1
    return max(versions) + 1


def get_strategy_from_filename(stem):
    if stem.startswith("rotated_"):
        return "rotated"
    if stem.startswith("3quarter_"):
        return "3quarter"
    return None


def find_input_image(source_asset_dir, strategy):
    if strategy:
        path = source_asset_dir / f"02_input_{strategy}_masked.png"
        if path.exists():
            return path
    candidates = list(source_asset_dir.glob("02_input_*masked*.png"))
    return candidates[0] if candidates else None


def process_alt_mesh(candidate_glb, source_asset_dir, new_asset_dir, new_asset_id):
    new_asset_dir.mkdir(parents=True, exist_ok=True)
    final = new_asset_dir / "06_final.glb"
    if final.exists():
        return True, "skip_existing"

    strategy = get_strategy_from_filename(candidate_glb.stem)
    input_img = find_input_image(source_asset_dir, strategy)
    if not input_img:
        return False, "no_input_image"

    cf_input_name = f"alt_{new_asset_id}.png"
    try:
        p.upload_image(input_img, cf_input_name)
    except Exception as e:
        return False, f"upload_failed: {e}"

    shutil.copy(input_img, new_asset_dir / "02_input_masked.png")

    # Decimate
    decim_prefix = f"alt_decim_{new_asset_id}"
    try:
        pid = p.post_prompt(p.hunyuan_decimate_workflow(
            candidate_glb, p.DECIMATE_TARGET_FACES, decim_prefix))
        _, t_dec = p.wait_for(pid, "decimate", timeout=180)
    except Exception as e:
        return False, f"decimate_failed: {e}"

    decim_glbs = sorted(Path("C:/ComfyUI/output").glob(f"{decim_prefix}*.glb"),
                        key=lambda f: f.stat().st_mtime, reverse=True)
    if not decim_glbs:
        return False, "decimate_no_output"

    decim_path = new_asset_dir / "04b_decimated_untextured.glb"
    shutil.copy(decim_glbs[0], decim_path)

    # Texture
    tex_prefix = f"alt_tex_{new_asset_id}"
    try:
        pid = p.post_prompt(p.hunyuan_texture_workflow(decim_path, cf_input_name, tex_prefix))
        _, t_tex = p.wait_for(pid, "texture", timeout=600)
    except Exception as e:
        return False, f"texture_failed: {e}"

    tex_glbs = []
    for r in [Path("C:/ComfyUI/temp"), Path("C:/ComfyUI/output")]:
        tex_glbs.extend(r.glob(f"{tex_prefix}*.glb"))
    if not tex_glbs:
        return False, "texture_no_output"

    src = max(tex_glbs, key=lambda f: f.stat().st_mtime)
    shutil.copy(src, final)

    manifest = {
        "id": new_asset_id,
        "alt_mesh": True,
        "source_asset_dir": str(source_asset_dir),
        "source_candidate": str(candidate_glb),
        "decimate_time_s": round(t_dec, 1),
        "texture_time_s": round(t_tex, 1),
    }
    (new_asset_dir / "manifest.json").write_text(json.dumps(manifest, indent=2))
    return True, "ok"


def main():
    log("=" * 78)
    log("HUNYUAN ALT MESH REGEN")
    log("=" * 78)

    if WAIT_FOR_V3:
        wait_for_v3_complete()

    try:
        urllib.request.urlopen(f"{p.COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log(f"FATAL: ComfyUI not reachable: {e}")
        return 1

    log("Loading rembg session...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    # Build (cat, asset_id) -> [candidate_glbs]
    by_asset = defaultdict(list)
    for cat in CATEGORIES:
        cat_root = ROOT / cat
        if not cat_root.exists():
            continue
        for asset_dir in sorted(cat_root.iterdir()):
            if not asset_dir.is_dir():
                continue
            meshes_dir = asset_dir / "03_meshes"
            if not meshes_dir.is_dir():
                continue
            cands = sorted(meshes_dir.glob("*.glb"))
            if cands:
                by_asset[(cat, asset_dir.name)].extend(cands)

    total_candidates = sum(len(v) for v in by_asset.values())
    log(f"Found {total_candidates} alt-mesh candidates across {len(by_asset)} source assets")

    overall_start = time.time()
    completed = 0
    failed = 0
    skipped = 0
    var_ok = 0
    var_fail = 0

    for (cat, source_asset_id), candidates in by_asset.items():
        next_v = find_next_version(source_asset_id, cat)

        # Look up variants. Strip _vN if present so we use the base asset's variants.
        base_for_variants = re.sub(r'_v\d+$', '', source_asset_id)
        all_variants = find_variants_for_asset(base_for_variants)
        if all_variants is None:
            all_variants = find_variants_for_asset(source_asset_id)
        if all_variants and len(all_variants) >= 2:
            chosen_variants = all_variants[:2]
        elif all_variants and len(all_variants) == 1:
            chosen_variants = all_variants
        else:
            chosen_variants = [
                {"suffix": "warm", "prompt": "warm earthy tone variant"},
                {"suffix": "cool", "prompt": "cool gray tone variant"},
            ]

        log(f"\n=== {cat}/{source_asset_id} - {len(candidates)} candidates @ v{next_v}+ ===")
        log(f"    variants: {[v['suffix'] for v in chosen_variants]}")

        source_dir = ROOT / cat / source_asset_id
        for i, candidate in enumerate(candidates):
            v = next_v + i
            new_id = f"{source_asset_id}_v{v}"
            new_dir = ROOT / cat / new_id

            log(f"  -> {new_id}  (from {candidate.name})")

            try:
                success, msg = process_alt_mesh(candidate, source_dir, new_dir, new_id)
                if msg == "skip_existing":
                    skipped += 1
                elif not success:
                    log(f"     FAIL: {msg}")
                    failed += 1
                    continue
                else:
                    completed += 1
            except Exception as e:
                log(f"     EXCEPTION: {e}")
                log(traceback.format_exc())
                failed += 1
                continue

            for variant in chosen_variants:
                try:
                    result, status = cv.make_color_variant(new_id, cat, variant, rembg_session)
                    if status == "ok":
                        var_ok += 1
                    elif status == "skip":
                        var_ok += 1  # already done counts as ok
                    else:
                        var_fail += 1
                        log(f"     variant {variant['suffix']}: {status}")
                except Exception as e:
                    var_fail += 1
                    log(f"     variant {variant['suffix']}: EXCEPTION {e}")

            elapsed = time.time() - overall_start
            done = completed + failed + skipped
            avg = elapsed / max(done, 1)
            eta_min = avg * (total_candidates - done) / 60
            log(f"     [progress: {done}/{total_candidates} | ok={completed} fail={failed} skip={skipped} | "
                f"variants ok={var_ok} fail={var_fail} | ETA: {eta_min:.0f} min]")

    log("")
    log("=" * 78)
    log(f"ALT MESH REGEN COMPLETE")
    log(f"  Bases:    ok={completed}, failed={failed}, skipped={skipped}")
    log(f"  Variants: ok={var_ok}, failed={var_fail}")
    log(f"  Total time: {(time.time() - overall_start) / 60:.1f} min")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
