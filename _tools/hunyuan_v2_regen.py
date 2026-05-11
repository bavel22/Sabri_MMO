"""
Hunyuan v2 regeneration — re-generate a curated list of assets that came out
multi-object or otherwise bad in the original batch runs. Uses the same
canonical pipeline (multi-input + validation) but writes to <id>_v2 IDs so
the v2 outputs sit alongside the originals without overwriting.

Output layout:
  generated_assets/<category>/<asset>_v2/06_final.glb
  generated_assets/<category>/<asset>_v2/variants/<color>/06_final.glb

After this completes, run flatten_to_final_v2.py (separate script) to gather
into Final_v2/.

Total runtime estimate (RTX 5090):
  ~64 assets × ~5 min base = ~5.5 hours
  ~64 assets × ~4 variants × ~90s = ~6.5 hours
  Total: ~12 hours

Run:
  C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/hunyuan_v2_regen.py
"""
import json
import shutil
import sys
import time
import traceback
import urllib.request
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)
sys.path.insert(0, "C:/Sabri_MMO/_tools")

import hunyuan_asset_pipeline as p
import hunyuan_overnight_continue as oc
import hunyuan_color_variants as cv

# Import batch3 (file has hyphens in name handled by direct import)
import importlib.util
_spec = importlib.util.spec_from_file_location(
    "hunyuan_batch3_100",
    "C:/Sabri_MMO/_tools/hunyuan_batch3_100.py",
)
_b3 = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_b3)
BATCH3_ASSETS = _b3.BATCH3_ASSETS
BATCH3_VARIANTS = _b3.BATCH3_VARIANTS

ROOT = p.ROOT
LOG_FILE = ROOT / "_v2_regen.log"


# ============================================================================
# LIST OF ORIGINAL ASSETS TO REGENERATE
# ============================================================================
# (deduped, typos fixed). Each will be regenerated with _v2 suffix.
# mushroom_giant_red and mushroom_giant_blue from user's list are variants
# of mushroom_giant — regenerating mushroom_giant produces them automatically.
TO_REGEN = [
    "arch_doorway",
    "awning_striped",
    "banner_horizontal",
    "banner_long",
    "basket_apple",
    "basket_woven",
    "birdhouse",
    "bookshelf",
    "bush_dense",
    "bush_flowering",
    "bush_round",
    "bush_small",
    "bush_sparse",
    "bush_thorny",
    "bush_wide",
    "cactus_desert",
    "chandelier_iron",
    "cliff_face_high",
    "cobwebs_corner",
    "crate_large",
    "crate_small",
    "desert_yucca",
    "door_wooden",
    "fence_long",
    "fence_stone",
    "fence_wooden",
    "flower_bluebells",
    "flower_cluster",
    "flower_lavender",
    "flower_red_tulips",
    "fountain_stone",
    "house_prontera_corner",
    "lamppost_amatsu",
    "lamppost_medieval",
    "lantern_floor",
    "moss_patch",
    "mushroom_blue",
    "mushroom_brown",
    "mushroom_giant",
    "pillar_carved",
    "planter_box",
    "pottery_jar",
    "reeds_cattails",
    "rock_large",
    "rock_small",
    "rocks_cluster",
    "rocks_pile",
    "sign_wooden",
    "torch_wall",
    "tree_bamboo",
    "tree_birch",
    "tree_dead_old",
    "tree_jungle",
    "tree_maple_yellow",
    "tree_oak_dense",
    "tree_oak_old",
    "tree_oak_tall",
    "tree_pine",
    "tree_pine_tall",
    "tree_pine_wide",
    "tree_stump",
    "vine_hanging",
    "waterfall_rock",
    "window_shutters",
]


# ============================================================================
# BUILD V2 ASSETS + VARIANTS BY LOOKUP
# ============================================================================
def build_master_lookups():
    """Combine all asset definitions and variant definitions across batches."""
    asset_lookup = {}  # id -> {category, subject}
    for a in p.ASSETS + oc.BATCH2_ASSETS + BATCH3_ASSETS:
        asset_lookup[a["id"]] = {"category": a["category"], "subject": a["subject"]}

    variant_lookup = {}
    variant_lookup.update(cv.COLOR_VARIANTS)
    variant_lookup.update(BATCH3_VARIANTS)
    return asset_lookup, variant_lookup


def build_v2_lists(asset_lookup, variant_lookup):
    """Build V2_ASSETS list and V2_VARIANTS dict from TO_REGEN names."""
    v2_assets = []
    v2_variants = {}
    missing = []
    for orig_id in TO_REGEN:
        if orig_id not in asset_lookup:
            missing.append(orig_id)
            continue
        info = asset_lookup[orig_id]
        new_id = f"{orig_id}_v2"
        v2_assets.append({
            "id": new_id,
            "category": info["category"],
            "subject": info["subject"],
        })
        if orig_id in variant_lookup:
            v2_variants[new_id] = variant_lookup[orig_id]
    return v2_assets, v2_variants, missing


# ============================================================================
# LOGGING
# ============================================================================
def log(msg):
    ts = time.strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


# ============================================================================
# MAIN
# ============================================================================
def main():
    asset_lookup, variant_lookup = build_master_lookups()
    v2_assets, v2_variants, missing = build_v2_lists(asset_lookup, variant_lookup)

    log("=" * 78)
    log(f"HUNYUAN V2 REGEN")
    log(f"  Requested: {len(TO_REGEN)} assets")
    log(f"  Resolved:  {len(v2_assets)} assets, {sum(len(v) for v in v2_variants.values())} variants")
    if missing:
        log(f"  MISSING (no asset definition found): {missing}")
    log("=" * 78)

    # Verify ComfyUI is up
    try:
        urllib.request.urlopen(f"{p.COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log(f"FATAL: ComfyUI not reachable at {p.COMFY_URL}: {e}")
        return 1

    # Load rembg session once (used by both base + variant gen)
    log("Loading rembg BiRefNet session...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    # ---- PHASE A: generate base assets ----
    log("")
    log("=" * 78)
    log(f"PHASE A: Generate {len(v2_assets)} v2 base assets (multi-input + validation)")
    log("=" * 78)

    overall_start = time.time()
    a_ok = a_skip = a_fail = 0

    for i, asset in enumerate(v2_assets, 1):
        asset_dir = ROOT / asset["category"] / asset["id"]
        # Skip if already done (idempotent)
        if (asset_dir / "06_final.glb").exists():
            log(f"[{i}/{len(v2_assets)}] {asset['category']}/{asset['id']} — SKIP (already done)")
            a_skip += 1
            continue

        log("")
        log("-" * 78)
        log(f"[{i}/{len(v2_assets)}] {asset['category']}/{asset['id']}")
        log(f"  subject: {asset['subject'][:80]}...")

        try:
            t0 = time.time()
            manifest = p.process_asset_multi_input(asset, rembg_session)
            dt = time.time() - t0
            log(f"  OK in {dt:.0f}s "
                f"(d={manifest['stages']['mesh_sweep']['depth_ratio']:.2f} "
                f"v={manifest['stages']['mesh_sweep']['volume']:.2f} "
                f"faces={manifest['stages']['final']['faces']})")
            a_ok += 1
        except Exception as e:
            log(f"  FAILED: {e}")
            log(traceback.format_exc())
            a_fail += 1

        elapsed = time.time() - overall_start
        avg = elapsed / max(i, 1)
        eta = avg * (len(v2_assets) - i) / 60
        log(f"  [phase A: {i}/{len(v2_assets)} | ok={a_ok} skip={a_skip} fail={a_fail} | ETA: {eta:.0f} min]")

    log("")
    log(f"PHASE A done: ok={a_ok}, skipped={a_skip}, failed={a_fail} in "
        f"{(time.time()-overall_start)/60:.1f} min")

    # ---- PHASE B: generate variants ----
    log("")
    log("=" * 78)
    log(f"PHASE B: Generate variants for {len(v2_variants)} v2 assets")
    log("=" * 78)

    total_planned = sum(len(v) for v in v2_variants.values())
    log(f"  Total variants planned: {total_planned}")

    b_start = time.time()
    b_ok = b_skip = b_fail = b_skip_bad = 0

    for asset_id, variants in v2_variants.items():
        category = cv.find_asset_category(asset_id)
        if not category:
            log(f"  {asset_id}: not found, skip")
            b_skip += len(variants)
            continue

        is_good, why = cv.asset_is_good(asset_id, category)
        if not is_good:
            log(f"  {asset_id}: SKIP (bad: {why}), {len(variants)} variants saved")
            b_skip_bad += len(variants)
            continue

        log("")
        log(f"=== {category}/{asset_id} — {len(variants)} variants ===")
        for v in variants:
            try:
                result, status = cv.make_color_variant(asset_id, category, v, rembg_session)
                if status == "ok":
                    b_ok += 1
                elif status == "skip":
                    b_skip += 1
                else:
                    b_fail += 1
            except Exception as e:
                log(f"  variant {v.get('suffix','?')}: FAILED {e}")
                b_fail += 1

    log("")
    log(f"PHASE B done: ok={b_ok}, skipped_existing={b_skip}, "
        f"failed={b_fail}, skipped_bad_asset={b_skip_bad} in "
        f"{(time.time()-b_start)/60:.1f} min")

    # ---- Summary ----
    log("")
    log("=" * 78)
    log(f"V2 REGEN COMPLETE")
    log(f"  Phase A (base):    ok={a_ok} skip={a_skip} fail={a_fail}")
    log(f"  Phase B (variants):ok={b_ok} skip_exist={b_skip} fail={b_fail} skip_bad={b_skip_bad}")
    log(f"  Total time:        {(time.time()-overall_start)/60:.1f} min")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
