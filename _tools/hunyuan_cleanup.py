"""
Cleanup orchestrator — runs after batch1 + rerolls + batch2 + variants.

Iteratively scans all asset folders, identifies "bad" assets, and re-rolls them
with prompt variations + new seeds. Repeats until either:
  - No more bad assets (clean!)
  - All bad assets have hit max attempts per asset (give up)
  - Hit max global iterations (safety limit)

"Bad" definition:
  - score = depth_ratio × volume < 0.15  → low overall quality
  - volume > 5.0                          → cube artifact (model baked multiple objects)
  - depth_ratio < 0.4                     → flat 2D cutout
  - volume < 0.05                         → thin shell (no real mass)
  - manifest.status != "ok"               → previous failure

Each asset gets up to 8 reroll attempts, each with a different prompt variation
designed to push the model into producing more volumetric output.
"""
import io
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

ROOT = p.ROOT
COMFY_URL = p.COMFY_URL

CLEANUP_LOG = ROOT / "_cleanup.log"
MAX_ATTEMPTS_PER_ASSET = 8
MAX_GLOBAL_ITERATIONS = 30

# Bad-asset thresholds
THRESH_SCORE = 0.15
THRESH_VOL_HIGH = 5.0
THRESH_DEPTH_LOW = 0.4
THRESH_VOL_LOW = 0.05


# Prompt variations applied at increasing attempt numbers — each pushes harder
# for volumetric output. Cycle through these as attempts increment.
VARIATION_SUFFIXES = [
    "",  # attempt 1 = original prompt
    ", very volumetric form, equally wide and deep, full 3D round shape, no flat sides",
    ", thick chunky volumetric body, dense solid mass, full plump volume",
    ", viewed from a high 3/4 angle clearly showing depth and back surfaces",
    ", chubby rounded silhouette, sphere-like proportions, depth equals width",
    ", solid 3D form, definitely not flat, hand-sculpted clay maquette style, viewed three-quarters",
    ", isometric voxel game asset, blocky cubic volume, fully 3D dimensional",
    ", full body 3D model rotated to show its depth, perspective render with strong shading",
]


# ============================================================================
# UTILITIES
# ============================================================================
def log(msg):
    ts = time.strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    try:
        with open(CLEANUP_LOG, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def wait_for_continue_finished():
    """Block until the continuation orchestrator finishes (or times out at 12h)."""
    log("Waiting for continuation orchestrator to finish (batch1 + rerolls + batch2 + variants)...")
    cont_log = ROOT / "_continue.log"
    deadline = time.time() + 12 * 3600  # 12 hour safety limit
    while time.time() < deadline:
        try:
            if cont_log.exists():
                text = cont_log.read_text(encoding="utf-8", errors="replace")
                if "ALL PHASES COMPLETE" in text:
                    log("Continuation orchestrator finished — starting cleanup")
                    return True
                # Show progress
                lines = [l for l in text.split("\n") if l.strip()]
                if lines:
                    log(f"  ...orchestrator running, last log line: {lines[-1][-100:]}")
        except Exception as e:
            log(f"  log read error: {e}")
        time.sleep(180)  # check every 3 minutes
    log("WARNING: 12h timeout waiting for orchestrator — proceeding to cleanup anyway")
    return False


def find_asset_def(asset_id):
    """Locate the original asset definition by id (in batch1 or batch2 lists)."""
    for src in (p.ASSETS, oc.BATCH2_ASSETS):
        for a in src:
            if a["id"] == asset_id:
                return a
    return None


def evaluate_manifest(manifest):
    """Return (is_bad, reasons[]) for an asset's manifest."""
    if manifest.get("status") != "ok":
        return True, ["previous_failure"]

    sweep = manifest.get("stages", {}).get("mesh_sweep", {})
    depth = sweep.get("depth_ratio", 0)
    vol = sweep.get("volume", 0)
    score = depth * vol

    reasons = []
    if score < THRESH_SCORE:
        reasons.append(f"low_score({score:.2f})")
    if vol > THRESH_VOL_HIGH:
        reasons.append(f"cube_artifact(vol={vol:.2f})")
    if depth < THRESH_DEPTH_LOW:
        reasons.append(f"flat(depth={depth:.2f})")
    if vol < THRESH_VOL_LOW:
        reasons.append(f"thin_shell(vol={vol:.2f})")

    return (len(reasons) > 0), reasons


def scan_all_assets():
    """Walk generated_assets/, return list of (asset_dir, manifest) for all assets."""
    results = []
    for cat_dir in ROOT.iterdir():
        if not cat_dir.is_dir() or cat_dir.name.startswith("_"):
            continue
        for asset_dir in cat_dir.iterdir():
            if not asset_dir.is_dir():
                continue
            manifest_path = asset_dir / "manifest.json"
            if not manifest_path.exists():
                continue
            try:
                m = json.loads(manifest_path.read_text())
                results.append((asset_dir, m))
            except Exception:
                continue
    return results


def find_bad_assets():
    """Return list of (asset_dir, manifest, reasons) for all bad assets that
    haven't yet hit max attempts."""
    bad = []
    for asset_dir, manifest in scan_all_assets():
        is_bad, reasons = evaluate_manifest(manifest)
        if not is_bad:
            continue
        attempts = manifest.get("attempts", 1)
        if attempts >= MAX_ATTEMPTS_PER_ASSET:
            continue  # gave up on this one
        bad.append((asset_dir, manifest, reasons))
    return bad


def quality_score(depth, volume):
    """Compute quality-aware score that PENALIZES cube artifacts and thin shells.
    Plain (depth × volume) is wrong because a cube artifact (vol=7.68) gets
    a HIGHER score than a slightly-thin asset (vol=0.4), even though it's worse.

    Cube artifacts (vol > 5) are catastrophically wrong (model baked multiple
    objects together) — return a near-zero score so ANY non-cube alternative wins.
    Thin shells (vol < 0.05) are usable shapes just lacking volume — half-credit.
    Flat (depth < 0.4) is similar — half-credit.
    Normal (0.05 < vol < 5.0 and depth >= 0.4) gets full depth × volume.
    """
    if volume <= 0:
        return 0.0
    if volume > 5.0:
        # Cube artifact — catastrophically wrong. Always lose to non-cube.
        return 0.001
    if volume < 0.05:
        # Thin shell — usable shape, just thin. Half-credit.
        return depth * volume * 0.5
    if depth < 0.4:
        # Flat cutout — half-credit
        return depth * volume * 0.5
    # Normal case — full credit
    return depth * volume


def reroll_asset(asset_dir, manifest, reasons, rembg_session):
    """Re-roll a single bad asset using MULTI-INPUT pipeline + a different
    suffix variation per attempt. Best result is preserved across attempts
    using a QUALITY-AWARE score (not raw depth*vol — see quality_score above).

    Backup is stored OUTSIDE asset_dir so rmtree doesn't destroy it.
    """
    asset_id = manifest["id"]
    asset_def = find_asset_def(asset_id)
    if not asset_def:
        log(f"  unknown asset id {asset_id}, skip")
        return None

    attempts = manifest.get("attempts", 1)
    suffix = VARIATION_SUFFIXES[min(attempts, len(VARIATION_SUFFIXES) - 1)]

    # Compute prev quality score using QUALITY-AWARE metric
    sweep = manifest.get("stages", {}).get("mesh_sweep", {})
    prev_d = sweep.get("depth_ratio", 0)
    prev_v = sweep.get("volume", 0)
    prev_q = quality_score(prev_d, prev_v)

    log(f"  attempt #{attempts + 1}, prev_quality={prev_q:.3f} (d={prev_d:.2f} v={prev_v:.2f}), "
        f"suffix='{suffix[:50]}'")

    # CRITICAL: Backup OUTSIDE asset_dir so rmtree doesn't destroy it.
    backup_dir = ROOT / "_history" / asset_id / f"attempt_{attempts}"
    backup_dir.mkdir(parents=True, exist_ok=True)
    backed_up_files = []
    for fname in ["06_final.glb", "05_textured.glb", "04b_decimated_untextured.glb",
                  "04_best_mesh.glb", "manifest.json"]:
        src = asset_dir / fname
        if src.exists():
            shutil.copy(src, backup_dir / fname)
            backed_up_files.append(fname)

    def restore_from_backup(reason):
        """Copy backed-up files back into asset_dir."""
        log(f"  RESTORING backup: {reason}")
        for fname in backed_up_files:
            src = backup_dir / fname
            if src.exists():
                shutil.copy(src, asset_dir / fname)
        # Update manifest with attempt count
        try:
            old_m = json.loads((asset_dir / "manifest.json").read_text())
            old_m["attempts"] = attempts + 1
            old_m["last_reroll_attempt"] = {
                "attempt": attempts,
                "reasons": reasons,
                "result": reason,
                "old_quality": round(prev_q, 3),
            }
            (asset_dir / "manifest.json").write_text(json.dumps(old_m, indent=2, default=str))
            return old_m
        except Exception:
            return None

    # Reroll with multi-input
    asset_with_suffix = {**asset_def}
    new_manifest = None
    try:
        if asset_dir.exists():
            shutil.rmtree(asset_dir)
        asset_dir.mkdir(parents=True, exist_ok=True)
        new_manifest = p.process_asset_multi_input(
            asset_with_suffix, rembg_session,
            extra_subject_suffix=suffix,
        )
    except Exception as e:
        log(f"  FAILED: {e}")
        # Restore from backup since reroll crashed
        return restore_from_backup(f"reroll_crashed: {str(e)[:100]}")

    if not new_manifest:
        return restore_from_backup("reroll_returned_none")

    # Compute new quality score
    new_sweep = new_manifest.get("stages", {}).get("mesh_sweep", {})
    new_d = new_sweep.get("depth_ratio", 0)
    new_v = new_sweep.get("volume", 0)
    new_q = quality_score(new_d, new_v)

    log(f"  new_quality={new_q:.3f} (d={new_d:.2f} v={new_v:.2f}) vs prev_quality={prev_q:.3f}")

    # If prior attempt was BETTER (quality-aware), restore the backup
    if prev_q > new_q + 0.01:  # +0.01 hysteresis to prefer new on near-tie
        return restore_from_backup(f"new attempt was worse "
                                     f"(new_q={new_q:.3f} < prev_q={prev_q:.3f})")

    # New is better OR similar — keep new and update metadata
    new_manifest["attempts"] = attempts + 1
    new_manifest["reroll_history"] = manifest.get("reroll_history", []) + [{
        "attempt": attempts,
        "reasons": reasons,
        "old_quality": round(prev_q, 3),
        "new_quality": round(new_q, 3),
        "improved": new_q > prev_q,
    }]
    new_manifest["best_quality"] = round(new_q, 3)
    (asset_dir / "manifest.json").write_text(json.dumps(new_manifest, indent=2, default=str))
    return new_manifest


# ============================================================================
# MAIN
# ============================================================================
def main():
    log("=" * 78)
    log("CLEANUP ORCHESTRATOR")
    log(f"  Max attempts per asset: {MAX_ATTEMPTS_PER_ASSET}")
    log(f"  Max global iterations:  {MAX_GLOBAL_ITERATIONS}")
    log(f"  Bad thresholds: score<{THRESH_SCORE}, vol>{THRESH_VOL_HIGH}, "
        f"depth<{THRESH_DEPTH_LOW}, vol<{THRESH_VOL_LOW}")
    log("=" * 78)

    # Wait for upstream pipelines to finish
    wait_for_continue_finished()

    # Verify ComfyUI still up
    try:
        urllib.request.urlopen(f"{COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log(f"FATAL: ComfyUI unreachable: {e}")
        return 1

    log("Loading rembg session...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    # Initial scan
    all_assets = scan_all_assets()
    log(f"Total assets in pipeline: {len(all_assets)}")

    overall_start = time.time()
    iteration = 0

    while iteration < MAX_GLOBAL_ITERATIONS:
        iteration += 1
        bad = find_bad_assets()

        if not bad:
            log("")
            log("=" * 78)
            log(f"ALL CLEAN — no bad assets after {iteration - 1} cleanup passes")
            log("=" * 78)
            break

        log("")
        log("=" * 78)
        log(f"CLEANUP ITERATION {iteration}: {len(bad)} bad assets to reroll")
        log("=" * 78)
        for ad, m, reasons in bad[:15]:
            log(f"  {m['id']:30}: {' '.join(reasons)}")
        if len(bad) > 15:
            log(f"  ... and {len(bad) - 15} more")

        successes_this_iter = 0
        for j, (asset_dir, manifest, reasons) in enumerate(bad, 1):
            log("")
            log("-" * 60)
            log(f"REROLL [{j}/{len(bad)}] {manifest['id']}: {' '.join(reasons)}")
            log("-" * 60)
            try:
                new_m = reroll_asset(asset_dir, manifest, reasons, rembg_session)
                if new_m:
                    sweep = new_m.get("stages", {}).get("mesh_sweep", {})
                    is_bad, new_reasons = evaluate_manifest(new_m)
                    if is_bad:
                        log(f"  STILL BAD: {' '.join(new_reasons)} (attempt {new_m.get('attempts','?')}/{MAX_ATTEMPTS_PER_ASSET})")
                    else:
                        successes_this_iter += 1
                        log(f"  CLEAN: depth={sweep.get('depth_ratio',0):.2f} "
                            f"vol={sweep.get('volume',0):.2f}")
            except Exception as e:
                log(f"  FAILED: {e}\n{traceback.format_exc()[:500]}")

        log("")
        log(f"--- iteration {iteration}: {successes_this_iter}/{len(bad)} fixed | "
            f"elapsed: {(time.time()-overall_start)/60:.1f} min ---")

        if successes_this_iter == 0 and iteration > 3:
            # No progress for 3 iterations after the first 3 — likely stuck
            log("")
            log("No new fixes this iteration. Continuing one more pass to be sure...")

    # Final report
    log("")
    log("=" * 78)
    log("CLEANUP DONE")
    log(f"  Total time: {(time.time()-overall_start)/60:.1f} min")
    log(f"  Iterations: {iteration}")

    final_bad = find_bad_assets()
    given_up = []
    for asset_dir, m in scan_all_assets():
        is_bad, reasons = evaluate_manifest(m)
        if is_bad and m.get("attempts", 1) >= MAX_ATTEMPTS_PER_ASSET:
            given_up.append((m["id"], reasons))

    log(f"  Final bad (still rerollable): {len(final_bad)}")
    log(f"  Given up (max attempts hit):  {len(given_up)}")
    for asset_id, reasons in given_up:
        log(f"    {asset_id}: {' '.join(reasons)}")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
