"""
post_alt_mesh_pipeline.py — Queue all post-alt-mesh consolidation steps.
Waits for "ALT MESH REGEN COMPLETE" in _alt_meshes.log, then runs in sequence:

  1. flatten_to_final_v3.py    — gathers v3 + alt-mesh into Final_v3/
  2. 03_meshes cleanup         — frees remaining duplicate disk space
  3. find_multi_object_v2.py   — analyzes outputs for multi-object failures

After this, the only remaining step is to open UE5 and run
import_hunyuan_v3_assets.py inside the editor (can't be done from outside
UE5 because it uses the unreal Python module).

Logs to: C:/Sabri_MMO/3d art/generated_assets/_post_alt_mesh.log

Run:
  C:/Users/pladr/AppData/Local/Programs/Python/Python312/python.exe \\
      C:/Sabri_MMO/_tools/post_alt_mesh_pipeline.py
"""
import subprocess
import sys
import time
import json
from pathlib import Path

ROOT = Path("C:/Sabri_MMO/3d art/generated_assets")
LOG = ROOT / "_post_alt_mesh.log"
ALT_LOG = ROOT / "_alt_meshes.log"

PYTHON = "C:/Users/pladr/AppData/Local/Programs/Python/Python312/python.exe"
COMFYUI_PY = "C:/ComfyUI/venv/Scripts/python.exe"

WAIT_FOR_ALT_MESH = True


def log(msg):
    ts = time.strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    try:
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def wait_for_alt_mesh_complete():
    log("Waiting for alt-mesh to complete (polling _alt_meshes.log)...")
    while True:
        try:
            if ALT_LOG.exists():
                text = ALT_LOG.read_text(encoding="utf-8", errors="ignore")
                if "ALT MESH REGEN COMPLETE" in text:
                    log("Alt-mesh detected as complete. Starting post-pipeline.")
                    return
        except Exception as e:
            log(f"  log read err: {e}")
        time.sleep(120)


def run_step(label, cmd):
    log(f"\n{'=' * 70}")
    log(f"STEP: {label}")
    log(f"  cmd: {' '.join(cmd)}")
    log("=" * 70)
    start = time.time()
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=3600,
        )
        elapsed = time.time() - start
        if result.stdout:
            log(f"  stdout (last 30 lines):")
            for ln in result.stdout.splitlines()[-30:]:
                log(f"    {ln}")
        if result.stderr:
            log(f"  stderr (last 20 lines):")
            for ln in result.stderr.splitlines()[-20:]:
                log(f"    {ln}")
        if result.returncode == 0:
            log(f"  STEP OK ({elapsed:.0f}s)")
            return True
        else:
            log(f"  STEP FAILED (exit {result.returncode}, {elapsed:.0f}s)")
            return False
    except subprocess.TimeoutExpired:
        log(f"  STEP TIMEOUT (>3600s)")
        return False
    except Exception as e:
        log(f"  STEP EXCEPTION: {e}")
        return False


def cleanup_03_meshes():
    """Inline 03_meshes duplicate cleanup (size-match)."""
    log("\n" + "=" * 70)
    log("STEP: 03_meshes cleanup (size-based)")
    log("=" * 70)

    deleted = 0
    freed_bytes = 0
    no_meshes_dir = 0

    for cat_dir in sorted(ROOT.iterdir()):
        if not cat_dir.is_dir() or cat_dir.name.startswith("_"):
            continue
        for asset_dir in sorted(cat_dir.iterdir()):
            if not asset_dir.is_dir():
                continue
            best = asset_dir / "04_best_mesh.glb"
            meshes_dir = asset_dir / "03_meshes"
            if not best.exists() or not meshes_dir.is_dir():
                if not meshes_dir.is_dir():
                    no_meshes_dir += 1
                continue
            try:
                best_size = best.stat().st_size
                for f in sorted(meshes_dir.glob("*.glb")):
                    if f.stat().st_size == best_size:
                        freed_bytes += f.stat().st_size
                        f.unlink()
                        deleted += 1
                        break
            except Exception as e:
                log(f"  err on {asset_dir.name}: {e}")

    log(f"  Deleted: {deleted}")
    log(f"  Freed:   {freed_bytes / 1024 / 1024:.1f} MB")
    log(f"  No 03_meshes/: {no_meshes_dir}")
    log(f"  STEP OK")
    return True


def main():
    log("=" * 78)
    log("POST-ALT-MESH PIPELINE")
    log("=" * 78)

    if WAIT_FOR_ALT_MESH:
        wait_for_alt_mesh_complete()

    overall_start = time.time()

    # Step 1: flatten v3 + alt-mesh into Final_v3/
    Path("C:/Sabri_MMO/3d art/Final_v3").mkdir(parents=True, exist_ok=True)
    step1_ok = run_step(
        "flatten_to_final_v3.py",
        [PYTHON, "C:/Sabri_MMO/_tools/flatten_to_final_v3.py"],
    )

    # Step 2: 03_meshes cleanup (inline)
    step2_ok = cleanup_03_meshes()

    # Step 3: multi-object detector (re-run on all assets including alt-mesh)
    step3_ok = run_step(
        "find_multi_object_v2.py",
        [COMFYUI_PY, "C:/Sabri_MMO/_tools/find_multi_object_v2.py"],
    )

    log("")
    log("=" * 78)
    log(f"POST-ALT-MESH PIPELINE COMPLETE")
    log(f"  flatten_to_final_v3:    {'ok' if step1_ok else 'FAILED'}")
    log(f"  03_meshes cleanup:      {'ok' if step2_ok else 'FAILED'}")
    log(f"  find_multi_object_v2:   {'ok' if step3_ok else 'FAILED'}")
    log(f"  Total time: {(time.time() - overall_start) / 60:.1f} min")
    log("=" * 78)
    log("")
    log("REMAINING MANUAL STEP:")
    log("  Open UE5 -> Output Log -> Python mode -> run:")
    log("    py \"C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_hunyuan_v3_assets.py\"")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
