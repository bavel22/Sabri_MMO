"""
Generate pebblestone + brickstone textures for Pryth town.

Submits 2 jobs to ComfyUI's queue at localhost:8188 — they'll queue behind
whatever's currently running. Total wall time ~10-15 min if queue is empty,
longer if a Hunyuan3D batch is in progress.

Reuses the workflow + helpers from generate_ground_textures_v2.py (same
pipeline as all other ground textures — SeamlessTile + CircularVAEDecode
+ post-process).

Output:
  _tools/ground_texture_output/T_Pebble_RO.png
  _tools/ground_texture_output/T_Brick_RO.png
  _tools/ground_texture_output/previews/T_Pebble_RO_3x3.jpg
  _tools/ground_texture_output/previews/T_Brick_RO_3x3.jpg

Next steps after generation:
  1. Drag the PNGs into UE5 Content/SabriMMO/Textures/Environment/Ground/
     (or wait for auto-import if Content folder is being watched)
  2. Run create_mi_pryth.py in UE5 — it will pick up the new textures

Run:
  python _tools/generate_pryth_pavements.py
"""

import os
import sys
import time

# Reuse helpers from the main ground-texture generator
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

from generate_ground_textures_v2 import (
    queue_prompt,
    wait_for_completion,
    download_image,
    check_comfyui,
    build_workflow,
    postprocess,
    NEGATIVE,
    OUTPUT_DIR,
    PREVIEW_DIR,
)

# ============================================================
# Pavement texture definitions for Pryth
# ============================================================

TEXTURES = [
    # ─────────────────────────────────────────────────────────
    # PEBBLESTONE — small rounded pebbles, residential side-paths
    # ─────────────────────────────────────────────────────────
    {
        "name": "T_Pebble_RO",
        "prompt": (
            "hand painted tileable pebblestone path texture, stylized game art, "
            "oil painting brushstrokes, small rounded river pebbles set into "
            "earth-packed mortar, warm gray-brown stones with sandy ochre "
            "highlights, organic random arrangement of irregular pebble sizes, "
            "subtle dark mortar between stones, weathered medieval village "
            "footpath aesthetic, top-down view, matte flat lighting, "
            "seamless pattern, game texture asset, no shadows, full frame coverage"
        ),
        "type": "pebble",
        "variants": 1,
        "base_seed": 38001,
    },
    # ─────────────────────────────────────────────────────────
    # BRICKSTONE — rectangular clay bricks, formal civic plaza
    # ─────────────────────────────────────────────────────────
    {
        "name": "T_Brick_RO",
        "prompt": (
            "hand painted tileable brick pavement texture, stylized game art, "
            "oil painting brushstrokes, rectangular clay bricks in running-bond "
            "pattern, warm terracotta and ochre brick colors with subtle "
            "variation per brick, narrow dark mortar lines between bricks, "
            "slightly weathered worn surface with faint chipping at edges, "
            "medieval town plaza floor aesthetic, top-down view, matte flat "
            "lighting, seamless pattern, game texture asset, no shadows, "
            "full frame coverage"
        ),
        "type": "brick",
        "variants": 1,
        "base_seed": 39001,
    },
]


def generate_one(tex_def):
    """Generate one texture, submit + wait + download + post-process."""
    name = tex_def["name"]
    seed = tex_def["base_seed"]
    print(f"\n--- {name} (seed={seed}) ---")
    print(f"  Prompt: {tex_def['prompt'][:80]}...")

    workflow = build_workflow(tex_def["prompt"], NEGATIVE, seed)
    prompt_id = queue_prompt(workflow)
    if not prompt_id:
        print(f"  [FAIL] Failed to queue {name}")
        return False

    print(f"  [OK] Queued prompt_id={prompt_id}")
    print(f"  Waiting for completion (timeout 10 min)...")

    t0 = time.time()
    history = wait_for_completion(prompt_id, timeout=600)
    elapsed = time.time() - t0
    if not history:
        print(f"  [FAIL] Timeout after {elapsed:.0f}s")
        return False
    print(f"  [OK] Completed in {elapsed:.0f}s")

    # Download generated image (node 9 = SaveImage)
    outputs = history.get("outputs", {})
    images = outputs.get("9", {}).get("images", [])
    if not images:
        print(f"  [FAIL] No image in output -- {outputs}")
        return False

    info = images[0]
    save_path = os.path.join(OUTPUT_DIR, f"{name}.png")
    try:
        download_image(info["filename"], info.get("subfolder", ""), save_path)
        print(f"  [OK] Downloaded -> {save_path}")
    except Exception as e:
        print(f"  [FAIL] Download failed: {e}")
        return False

    # Post-process: seamless check + color correction + 3x3 preview
    postprocess(save_path, name)
    print(f"  [OK] {name}.png ready")
    return True


def main():
    if not check_comfyui():
        print("ERROR: ComfyUI not reachable at http://127.0.0.1:8188")
        print("Make sure ComfyUI is running before invoking this script.")
        sys.exit(1)

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(PREVIEW_DIR, exist_ok=True)

    print("=" * 64)
    print("PRYTH PAVEMENT GENERATOR")
    print("=" * 64)
    print(f"Output: {OUTPUT_DIR}")
    print(f"Submitting {len(TEXTURES)} jobs to ComfyUI queue...")
    print(f"Note: jobs queue behind any in-progress workflow.")

    successes = 0
    for tex_def in TEXTURES:
        if generate_one(tex_def):
            successes += 1

    print("\n" + "=" * 64)
    print(f"COMPLETE: {successes}/{len(TEXTURES)} textures generated.")
    print("=" * 64)
    print()
    print("Next steps:")
    print("  1. Drag the PNGs from _tools/ground_texture_output/ into UE5:")
    print("     Content/SabriMMO/Textures/Environment/Ground/")
    print("     (or run Scripts/Environment/import_ground_textures.py)")
    print("  2. After import, run create_mi_pryth.py in UE5 to apply the textures")


if __name__ == "__main__":
    main()
