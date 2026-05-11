"""
Resume script — continuation orchestrator died at batch2 [92/100].
This skips already-done assets (via asset_already_done check) and
finishes the remaining batch2 + runs phase 3 variants.

Logs "ALL PHASES COMPLETE" to _continue.log so the cleanup orchestrator
(which is polling for that string) fires automatically when done.
"""
import json
import sys
import time
import traceback
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)
sys.path.insert(0, "C:/Sabri_MMO/_tools")

import hunyuan_asset_pipeline as p
import hunyuan_overnight_continue as oc

ROOT = oc.ROOT


def log(msg):
    """Log to BOTH a resume log AND the _continue.log so cleanup detects completion."""
    ts = time.strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    for path in [ROOT / "_resume.log", ROOT / "_continue.log"]:
        try:
            with open(path, "a", encoding="utf-8") as f:
                f.write(line + "\n")
        except Exception:
            pass


def main():
    log("=" * 78)
    log("RESUME ORCHESTRATOR")
    log("Skipping batch1 wait (done) + phase_rerolls (done)")
    log("Continuing: batch2 (skip done) + variants")
    log("=" * 78)

    # Verify ComfyUI
    import urllib.request
    try:
        urllib.request.urlopen(f"{oc.COMFY_URL}/system_stats", timeout=5)
        log("ComfyUI reachable")
    except Exception as e:
        log(f"FATAL: ComfyUI unreachable: {e}")
        return 1

    log("Loading rembg...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    overall_start = time.time()

    # ---- Continue batch2 (skip already done) ----
    log("=" * 78)
    log(f"PHASE 2 RESUME: Batch2 — {len(oc.BATCH2_ASSETS)} assets total (most should be skipped)")
    log("=" * 78)

    completed = 0
    failed = 0
    skipped = 0
    batch_start = time.time()

    for i, asset in enumerate(oc.BATCH2_ASSETS, 1):
        if oc.asset_already_done(asset):
            skipped += 1
            continue

        log("")
        log("-" * 78)
        log(f"BATCH2 RESUME [{i}/{len(oc.BATCH2_ASSETS)}] {asset['category']}/{asset['id']}")
        log(f"  subject: {asset['subject'][:80]}...")
        log("-" * 78)

        try:
            manifest = oc.process_one(asset, rembg_session, multi_input=True)
            if manifest:
                completed += 1
                sweep = manifest.get("stages", {}).get("mesh_sweep", {})
                log(f"  DONE in {manifest['total_time_s']:.0f}s "
                    f"(depth={sweep.get('depth_ratio', 0):.2f} "
                    f"vol={sweep.get('volume', 0):.2f})")
            else:
                failed += 1
        except Exception as e:
            failed += 1
            log(f"  EXCEPTION: {e}")
            log(traceback.format_exc()[:1000])

    log(f"\nbatch2 resume done: skipped={skipped}, ok={completed}, failed={failed} "
        f"in {(time.time()-batch_start)/60:.1f} min")

    # ---- Phase 3: variants ----
    log("")
    log("=" * 78)
    log(f"PHASE 3: Variants — {sum(len(v) for v in oc.TEXTURE_VARIANTS.values())} variants")
    log("=" * 78)
    try:
        oc.phase_variants(rembg_session)
    except Exception as e:
        log(f"PHASE 3 ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    total_min = (time.time() - overall_start) / 60
    log("")
    log("=" * 78)
    log(f"ALL PHASES COMPLETE — {total_min:.1f} min")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
