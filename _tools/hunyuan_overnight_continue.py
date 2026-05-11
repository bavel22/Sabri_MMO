"""
Overnight continuation orchestrator.

Runs after the batch1 (35-asset) pipeline finishes:
  1. Wait for batch1 _status.json to show "finished"
  2. Re-roll bad assets from batch1 (score < 0.15 OR vol > 5 cube artifact)
  3. Run batch2 — 100 NEW assets across all categories
  4. Run texture variants (color variations) on the best assets

Idempotent: skips any asset where manifest.json already exists with status=ok.
Per-asset error handling — one failure does not kill the run.
"""
import io
import json
import shutil
import sys
import time
import traceback
import urllib.request
import urllib.parse
import uuid
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)

# Import the existing pipeline functions
sys.path.insert(0, "C:/Sabri_MMO/_tools")
import hunyuan_asset_pipeline as p

# Re-use everything from the main pipeline
ROOT = p.ROOT
COMFY_URL = p.COMFY_URL


# ============================================================================
# BATCH 2 — 100 NEW ASSETS
# ============================================================================
BATCH2_ASSETS = [
    # ---- More vegetation (25) ----
    {"id": "tree_oak_tall",       "category": "vegetation", "subject": "tall fantasy oak tree with thick brown trunk and large green canopy spreading wide"},
    {"id": "tree_oak_old",        "category": "vegetation", "subject": "ancient gnarled oak tree, twisted thick trunk, sparse old leaves"},
    {"id": "tree_oak_dense",      "category": "vegetation", "subject": "young oak tree with dense bushy green canopy, smooth brown trunk"},
    {"id": "tree_maple_red",      "category": "vegetation", "subject": "fantasy maple tree with bright red fall leaves, slender brown trunk"},
    {"id": "tree_maple_yellow",   "category": "vegetation", "subject": "fantasy maple tree with golden yellow autumn leaves, brown trunk"},
    {"id": "tree_pine_tall",      "category": "vegetation", "subject": "tall slender pine tree with dark green needles, conical shape, narrow trunk"},
    {"id": "tree_pine_wide",      "category": "vegetation", "subject": "wide spreading pine tree, dark green needles, broad triangular silhouette"},
    {"id": "tree_willow",         "category": "vegetation", "subject": "fantasy willow tree with long drooping pale green leaves, twisted trunk"},
    {"id": "tree_birch",          "category": "vegetation", "subject": "white birch tree with thin papery white bark and small green leaves"},
    {"id": "tree_jungle",         "category": "vegetation", "subject": "tropical jungle tree with broad dark green leaves, thick brown trunk, vines"},
    {"id": "tree_dead_old",       "category": "vegetation", "subject": "ancient dead tree, charred black bark, no leaves, broken branches, ominous"},
    {"id": "tree_dead_white",     "category": "vegetation", "subject": "bleached white dead tree, smooth pale trunk, gnarled bare branches"},
    {"id": "tree_sakura_pink",    "category": "vegetation", "subject": "blooming cherry blossom tree, dark trunk, pale pink flowers, full canopy"},
    {"id": "tree_sakura_full",    "category": "vegetation", "subject": "full cherry blossom tree, thick pink flowers covering all branches, dark thick trunk"},
    {"id": "bush_small",          "category": "vegetation", "subject": "small round bush, dense dark green foliage, low ground plant"},
    {"id": "bush_dense",          "category": "vegetation", "subject": "dense thick bush with abundant small green leaves, full bushy shape"},
    {"id": "bush_sparse",         "category": "vegetation", "subject": "sparse open bush with thin branches and scattered leaves, sparse plant"},
    {"id": "bush_flowering",      "category": "vegetation", "subject": "bush with white flowers among green leaves, decorative shrub"},
    {"id": "bush_thorny",         "category": "vegetation", "subject": "thorny bramble bush, dark green leaves with red berries, prickly"},
    {"id": "mushroom_brown",      "category": "vegetation", "subject": "brown forest mushroom with rounded cap and thin stem, woodland fungus"},
    {"id": "mushroom_blue",       "category": "vegetation", "subject": "magical blue glowing mushroom with tall stem and umbrella cap, fantasy fungi"},
    {"id": "flower_red_tulips",   "category": "vegetation", "subject": "small cluster of red tulip flowers on green grass tuft, three flowers"},
    {"id": "flower_purple",       "category": "vegetation", "subject": "small cluster of purple wildflowers on grass tuft, ground plant"},
    {"id": "cactus_round",        "category": "vegetation", "subject": "small round green barrel cactus, ribbed surface with spines, desert plant"},
    {"id": "reeds_cattails",      "category": "vegetation", "subject": "cluster of cattail reeds with brown cylindrical heads, tall grass-like stalks"},

    # ---- More architecture (15) ----
    {"id": "house_prontera_tall",   "category": "architecture", "subject": "three story medieval European stone house with red tile roof, balcony, multiple windows, chimney"},
    {"id": "house_prontera_wide",   "category": "architecture", "subject": "wide medieval stone house with red clay tile roof, large wooden door, two windows on each side"},
    {"id": "house_prontera_corner", "category": "architecture", "subject": "corner medieval stone house, L-shape, red tile roof, two visible facades with windows"},
    {"id": "hut_payon_long",        "category": "architecture", "subject": "long Korean traditional thatched roof building, multiple paper sliding doors, wooden frame, raised platform"},
    {"id": "hut_payon_small",       "category": "architecture", "subject": "small Korean traditional thatched hut, single room, paper door, small wooden frame"},
    {"id": "house_morroc_arch",     "category": "architecture", "subject": "Middle Eastern adobe building with arched doorway, flat roof, sandstone color, decorative arch"},
    {"id": "house_morroc_dome",     "category": "architecture", "subject": "adobe desert building with small dome roof, sandstone walls, arched windows"},
    {"id": "house_alberta_wood",    "category": "architecture", "subject": "wooden port building, dark stained wood siding, gabled roof, large windows"},
    {"id": "dock_wooden",           "category": "architecture", "subject": "wooden dock pier section, weathered planks on supports, mooring posts"},
    {"id": "tower_geffen",          "category": "architecture", "subject": "tall medieval stone tower, conical blue roof, narrow windows, gothic stone wizard tower"},
    {"id": "tower_gate",            "category": "architecture", "subject": "medieval stone gate tower with crenellations, arched gateway, gray stone walls"},
    {"id": "wall_stone_arrow",      "category": "architecture", "subject": "medieval stone wall section with arrow slit windows, weathered gray stone, square shape"},
    {"id": "wall_stone_battlement", "category": "architecture", "subject": "stone castle battlement section with crenellations on top, gray stone wall"},
    {"id": "monument_obelisk",      "category": "architecture", "subject": "tall stone obelisk monument, square tapered shape, ancient ruins"},
    {"id": "shrine_small",          "category": "architecture", "subject": "small stone shrine with sloped roof and offering bowl, weathered gray stone"},

    # ---- More town props (25) ----
    {"id": "lamppost_amatsu",   "category": "props", "subject": "Asian paper lantern lamppost, red paper lantern at top of wooden post, traditional"},
    {"id": "lamppost_iron",     "category": "props", "subject": "tall medieval iron lamppost with black metal frame, four-sided glass lantern"},
    {"id": "sign_hanging",      "category": "props", "subject": "wooden sign hanging from iron bracket, rectangular plank, suspended by chains"},
    {"id": "sign_planted",      "category": "props", "subject": "wooden sign on a single post planted in ground, trapezoidal shape, simple frame"},
    {"id": "banner_long",       "category": "props", "subject": "long vertical banner on wooden pole, fabric flag with crest design, blue and gold"},
    {"id": "banner_horizontal", "category": "props", "subject": "horizontal banner stretched between two poles, fabric flag, red and white"},
    {"id": "awning_striped",    "category": "props", "subject": "striped fabric awning extending from wall, red and cream stripes, frame"},
    {"id": "awning_market",     "category": "props", "subject": "small market stall awning, blue fabric, supported by four wooden corner posts"},
    {"id": "barrel_large",      "category": "props", "subject": "large wooden barrel with iron bands, tall medieval cooper barrel, dark wood"},
    {"id": "barrel_open",       "category": "props", "subject": "open wooden barrel without lid, iron bands, contents visible, light brown wood"},
    {"id": "crate_small",       "category": "props", "subject": "small wooden crate, square box with diagonal slats, light brown medieval shipping crate"},
    {"id": "crate_large",       "category": "props", "subject": "large wooden shipping crate, rectangular, slatted sides, heavy duty"},
    {"id": "sack_open",         "category": "props", "subject": "open burlap sack standing up, top folded down to show grain inside, cream color"},
    {"id": "basket_woven",      "category": "props", "subject": "round woven wicker basket with handle, light brown reed weaving, market style"},
    {"id": "basket_apple",      "category": "props", "subject": "wicker basket filled with red apples, round shape, light brown weaving"},
    {"id": "cart_kafra",        "category": "props", "subject": "small two wheel cart with red Kafra service awning, wooden body, courier delivery cart"},
    {"id": "cart_merchant",     "category": "props", "subject": "wooden merchant pull cart with goods piled, two large wheels, simple platform"},
    {"id": "stall_market_red",  "category": "props", "subject": "medieval market stall with red striped awning, wooden counter displays, four corner posts"},
    {"id": "stall_market_blue", "category": "props", "subject": "medieval market stall with blue fabric canopy, wooden display table with goods"},
    {"id": "weapon_rack",       "category": "props", "subject": "wooden weapon rack with swords and axes propped, blacksmith display, vertical rack"},
    {"id": "table_wooden",      "category": "props", "subject": "rustic medieval wooden table, four legs, plain plank top, weathered wood"},
    {"id": "bench_wooden",      "category": "props", "subject": "long wooden bench, four legs, plank seat, medieval style, weathered"},
    {"id": "haystack",          "category": "props", "subject": "round haystack pile of dry yellow hay, farm bale, rustic"},
    {"id": "fence_long",        "category": "props", "subject": "long section of wooden fence, four horizontal rails, three upright posts, weathered"},
    {"id": "fence_stone",       "category": "props", "subject": "low stone fence section, stacked gray stones, rough rural wall"},

    # ---- More terrain (15) ----
    {"id": "boulder_huge",      "category": "terrain", "subject": "massive gray boulder, rounded irregular shape, prominent rock formation"},
    {"id": "rocks_cluster",     "category": "terrain", "subject": "cluster of three rocks of different sizes piled together, gray stones"},
    {"id": "rocks_pile",        "category": "terrain", "subject": "pile of small rocks and pebbles, mixed sizes, dirt and stone heap"},
    {"id": "rock_mossy_large",  "category": "terrain", "subject": "large mossy boulder covered in green moss, rounded shape, forest stone"},
    {"id": "rock_split",        "category": "terrain", "subject": "split rock with crack down the middle, gray stone broken in half"},
    {"id": "cliff_face_low",    "category": "terrain", "subject": "low cliff face module, gray rock wall section with cracks, modular terrain piece"},
    {"id": "cliff_face_high",   "category": "terrain", "subject": "tall cliff face section, vertical gray rock wall, layered stone strata"},
    {"id": "bridge_stone_arch", "category": "terrain", "subject": "stone arch bridge spanning gap, gray stone, single arch design, medieval"},
    {"id": "bridge_rope",       "category": "terrain", "subject": "rope and plank suspension bridge, wooden planks held by ropes, jungle bridge"},
    {"id": "stairs_stone",      "category": "terrain", "subject": "section of stone steps leading up, six steps, gray weathered stone"},
    {"id": "stairs_wooden",     "category": "terrain", "subject": "wooden staircase section, planks with risers, rustic outdoor steps"},
    {"id": "path_stone",        "category": "terrain", "subject": "section of stone paved path, irregular flat stones, rural path"},
    {"id": "well_stone",        "category": "terrain", "subject": "round stone well with low circular wall, wooden roof on posts, bucket"},
    {"id": "fountain_basin",    "category": "terrain", "subject": "stone fountain with circular basin and central pillar, water spout, town center"},
    {"id": "statue_pedestal",   "category": "terrain", "subject": "ornate stone pedestal for statue, square base with carved details, gothic"},

    # ---- More dungeon (15) ----
    {"id": "pillar_whole",      "category": "dungeon", "subject": "whole classical stone pillar, fluted column, intact gothic dungeon column"},
    {"id": "pillar_fluted",     "category": "dungeon", "subject": "fluted classical column with decorative top capital, gray stone"},
    {"id": "pillar_carved",     "category": "dungeon", "subject": "stone pillar with carved spiral patterns, intricate decoration, gothic"},
    {"id": "tomb_stone",        "category": "dungeon", "subject": "tall stone tombstone with rounded top, weathered gray stone, graveyard marker"},
    {"id": "tomb_cross",        "category": "dungeon", "subject": "stone cross grave marker, simple cross shape on rectangular base, weathered"},
    {"id": "coffin_wooden",     "category": "dungeon", "subject": "simple wooden coffin, rectangular box with lid, dark wood, lying flat"},
    {"id": "coffin_stone",      "category": "dungeon", "subject": "stone coffin with carved lid, ancient sarcophagus, weathered gray"},
    {"id": "torch_wall",        "category": "dungeon", "subject": "medieval wall torch, flaming brand on iron bracket, lit dungeon torch"},
    {"id": "brazier_iron",      "category": "dungeon", "subject": "iron brazier on tripod legs, glowing coals, medieval lighting"},
    {"id": "chandelier_iron",   "category": "dungeon", "subject": "medieval iron chandelier with multiple candles, hanging fixture"},
    {"id": "crystal_blue",      "category": "dungeon", "subject": "tall blue crystal cluster, multiple sharp pointed crystals, glowing magical gem"},
    {"id": "crystal_purple",    "category": "dungeon", "subject": "purple amethyst crystal cluster, geometric crystals, mineral formation"},
    {"id": "crystal_green",     "category": "dungeon", "subject": "green emerald crystal formation, sharp geode crystals, magical mineral"},
    {"id": "cobwebs_corner",    "category": "dungeon", "subject": "thick dusty cobwebs, gray spiderwebs cluster, abandoned dungeon decoration"},
    {"id": "bone_pile",         "category": "dungeon", "subject": "pile of bleached white bones and skulls, ribs and femurs, ossuary heap"},

    # ---- Special / NPC interaction (5) ----
    {"id": "anvil_blacksmith",  "category": "special", "subject": "iron blacksmith anvil on wooden block, classic horn shape, dark metal"},
    {"id": "table_alchemy",     "category": "special", "subject": "alchemy work table with bottles vials and tubes, wooden table laboratory setup"},
    {"id": "bookshelf",         "category": "special", "subject": "tall wooden bookshelf full of books, library shelf, dark wood"},
    {"id": "kafra_pad",         "category": "special", "subject": "magical glowing teleport pad, circular runic disk on stone base, blue magical glow"},
    {"id": "emperium_pedestal", "category": "special", "subject": "ornate stone pedestal with floating gold orb, war crystal stand, golden glowing"},
]


# ============================================================================
# RE-ROLL CRITERIA AND SUBJECTS
# Higher-quality prompts for assets that produced bad meshes in batch1
# ============================================================================
RE_ROLL_PROMPTS = {
    "tree_maple": "fantasy maple tree with vibrant red leaves, full round canopy, thick brown trunk, autumn forest tree",
    "tree_pine": "wide christmas tree style pine, full triangular shape with thick green needles, sturdy trunk, conifer",
    "tree_palm": "thick tropical palm tree, sturdy trunk with fan-shaped frond cluster on top, oasis plant",
    "tree_sakura": "full blooming cherry blossom tree, thick clouds of pink flowers, dark trunk, spring tree",
    "tree_bamboo": "single thick bamboo plant, segmented green stalks bundled together, tropical bamboo",
}


# ============================================================================
# TEXTURE VARIANTS — Re-texture existing decimated meshes with prompt variations
# ============================================================================
# For each base asset id, define 2 prompt variants (different colors/styles)
# Variants re-use the existing decimated mesh — only the input image + texture pass change
TEXTURE_VARIANTS = {
    # Tree color variants
    "tree_oak":       [{"suffix": "yellow",  "prompt": "fantasy oak tree with golden yellow autumn leaves, brown trunk"},
                       {"suffix": "lush",    "prompt": "fantasy oak tree with vibrant lush green leaves, dark trunk"}],
    "tree_oak_tall":  [{"suffix": "amber",   "prompt": "tall fantasy oak with amber and orange leaves, twisted brown trunk"},
                       {"suffix": "dark",    "prompt": "tall dark fantasy oak with deep emerald leaves, black trunk, mysterious"}],
    "tree_oak_old":   [{"suffix": "fall",    "prompt": "ancient gnarled oak with red orange autumn leaves, twisted dark trunk"}],
    "tree_oak_dense": [{"suffix": "yellow",  "prompt": "young oak with bushy yellow leaves, smooth pale trunk"}],
    "tree_maple":     [{"suffix": "green",   "prompt": "fantasy maple tree with bright green spring leaves, slim trunk"}],
    "tree_pine":      [{"suffix": "snow",    "prompt": "snow-covered pine tree, white snow on dark green branches, winter"}],
    "tree_dead":      [{"suffix": "burnt",   "prompt": "burnt charred dead tree, black bark, ominous, no leaves"}],
    "tree_birch":     [{"suffix": "yellow",  "prompt": "white birch tree with bright yellow autumn leaves"}],
    "tree_jungle":    [{"suffix": "purple",  "prompt": "exotic jungle tree with deep purple leaves, dark trunk, fantasy"}],
    "bush_round":     [{"suffix": "red_berry","prompt": "bush with red berries among dark green leaves"}],
    "bush_flowering": [{"suffix": "pink",    "prompt": "bush with pink flowers among small leaves"}],
    "mushroom_giant": [{"suffix": "blue",    "prompt": "giant blue mushroom with white spots, glowing stem, magical fungus"},
                       {"suffix": "green",   "prompt": "giant green mushroom with yellow stripes, tall stem, fantasy"}],
    # Building variants
    "house_prontera_small":  [{"suffix": "wood", "prompt": "small wooden house with thatched roof and stone foundation, rustic"}],
    "house_prontera_medium": [{"suffix": "stone", "prompt": "medieval stone house with slate roof, wooden door, two windows, gray walls"}],
    # Rock variants
    "rock_small":   [{"suffix": "snow",  "prompt": "small rock partially covered in snow, gray stone in winter"}],
    "rock_medium":  [{"suffix": "moss",  "prompt": "medium gray rock with thick green moss covering, forest stone"}],
    "boulder_huge": [{"suffix": "moss",  "prompt": "massive boulder covered in moss and ferns, ancient forest"}],
    # Misc
    "barrel_wooden":  [{"suffix": "dark", "prompt": "old dark wood barrel with rusted iron bands, weathered medieval"}],
    "lamppost_amatsu":[{"suffix": "blue", "prompt": "blue paper lantern lamppost, traditional Asian style"}],
}


# ============================================================================
# UTILITIES
# ============================================================================
def status_log(msg):
    """Log to a separate log file for the continue script."""
    ts = time.strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    log_path = ROOT / "_continue.log"
    try:
        with open(log_path, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def wait_for_batch1_done():
    """Block until batch1 status.json shows 'finished' field."""
    status_log("Waiting for batch1 (35 assets) to finish...")
    status_path = ROOT / "_status.json"
    while True:
        try:
            if status_path.exists():
                s = json.loads(status_path.read_text())
                if "finished" in s:
                    status_log(f"Batch1 done: {s['completed']}/{s['total_assets']} succeeded, {s['failed']} failed")
                    return s
                # Show progress
                completed = s.get("completed", 0)
                current = s.get("current", "?")
                status_log(f"  ...batch1 in progress: {completed}/35, current={current}")
        except Exception as e:
            status_log(f"  status read err: {e}")
        time.sleep(60)


def asset_already_done(asset):
    """Check if a manifest exists with status=ok."""
    asset_dir = ROOT / asset["category"] / asset["id"]
    manifest_path = asset_dir / "manifest.json"
    if manifest_path.exists():
        try:
            m = json.loads(manifest_path.read_text())
            return m.get("status") == "ok"
        except Exception:
            return False
    return False


def process_one(asset, rembg_session, force=False, multi_input=True):
    """Run full pipeline on a single asset. Returns manifest or None on error.
    multi_input=True (default): use 2 perspective strategies × 2 seeds (better quality)
    multi_input=False: legacy single-input × 4 seeds path
    """
    if not force and asset_already_done(asset):
        status_log(f"  already done, skipping")
        return None

    asset_dir = ROOT / asset["category"] / asset["id"]
    if force and asset_dir.exists():
        # Clean prior attempt
        try:
            shutil.rmtree(asset_dir)
        except Exception:
            pass
    asset_dir.mkdir(parents=True, exist_ok=True)

    try:
        if multi_input:
            return p.process_asset_multi_input(asset, rembg_session)
        else:
            return p.process_asset(asset, rembg_session)
    except Exception as e:
        tb = traceback.format_exc()
        status_log(f"  FAILED: {e}")
        try:
            (asset_dir / "ERROR.txt").write_text(f"{e}\n\n{tb}")
        except Exception:
            pass
        return None


# ============================================================================
# PHASE 1: Re-roll bad batch1 assets
# ============================================================================
def phase_rerolls(batch1_status, rembg_session):
    status_log("=" * 78)
    status_log("PHASE 1: Re-roll bad batch1 assets")
    status_log("=" * 78)

    # Identify bad ones
    rerolls = []
    for r in batch1_status.get("results", []):
        if r["status"] != "ok":
            continue
        score = r["depth"] * r["volume"]
        # Bad: very low score, OR cube artifact (volume too high)
        if score < 0.15 or r["volume"] > 5.0:
            rerolls.append(r)

    if not rerolls:
        status_log("  no rerolls needed")
        return

    status_log(f"  rerolling {len(rerolls)} assets:")
    for r in rerolls:
        status_log(f"    {r['id']}: depth={r['depth']:.2f} vol={r['volume']:.2f}")

    for i, r in enumerate(rerolls, 1):
        # Get original asset definition
        orig = next((a for a in p.ASSETS if a["id"] == r["id"]), None)
        if not orig:
            continue
        # Use better prompt if defined
        better_subject = RE_ROLL_PROMPTS.get(r["id"], orig["subject"])
        asset = {**orig, "subject": better_subject, "id": r["id"]}
        status_log("")
        status_log("-" * 78)
        status_log(f"REROLL [{i}/{len(rerolls)}] {asset['category']}/{asset['id']}")
        status_log(f"  new subject: {better_subject[:80]}...")
        status_log("-" * 78)
        process_one(asset, rembg_session, force=True)


# ============================================================================
# PHASE 2: Batch2 — 100 NEW assets
# ============================================================================
def phase_batch2(rembg_session):
    status_log("=" * 78)
    status_log(f"PHASE 2: Batch2 — {len(BATCH2_ASSETS)} new assets")
    status_log("=" * 78)

    completed = 0
    failed = 0
    overall_start = time.time()

    for i, asset in enumerate(BATCH2_ASSETS, 1):
        status_log("")
        status_log("-" * 78)
        status_log(f"BATCH2 [{i}/{len(BATCH2_ASSETS)}] {asset['category']}/{asset['id']}")
        status_log(f"  subject: {asset['subject'][:80]}...")
        status_log("-" * 78)

        manifest = process_one(asset, rembg_session)
        if manifest:
            completed += 1
            status_log(f"  DONE in {manifest['total_time_s']:.0f}s "
                        f"(depth={manifest['stages']['mesh_sweep']['depth_ratio']:.2f} "
                        f"vol={manifest['stages']['mesh_sweep']['volume']:.2f})")
        else:
            failed += 1

        # ETA
        elapsed = time.time() - overall_start
        avg = elapsed / max(i, 1)
        eta_min = avg * (len(BATCH2_ASSETS) - i) / 60
        status_log(f"  [batch2 progress: {i}/{len(BATCH2_ASSETS)} | "
                    f"{completed} ok, {failed} failed | ETA: {eta_min:.1f} min]")

    status_log(f"\nbatch2 done: {completed}/{len(BATCH2_ASSETS)} ok, {failed} failed in {(time.time()-overall_start)/60:.1f} min")


# ============================================================================
# PHASE 3: Texture variants (re-texture existing meshes with new prompts)
# ============================================================================
def texture_variant_workflow(mesh_path, input_filename, prefix):
    """Same as p.hunyuan_texture_workflow but inline — independent of any seed."""
    return p.hunyuan_texture_workflow(mesh_path, input_filename, prefix)


def make_variant(base_id, base_category, variant_def, rembg_session):
    """Generate a texture variant by re-running SDXL+texture on existing decimated mesh."""
    base_dir = ROOT / base_category / base_id
    decim_path = base_dir / "04b_decimated_untextured.glb"
    if not decim_path.exists():
        status_log(f"  no untextured decimated mesh for {base_id}, skip")
        return None

    suffix = variant_def["suffix"]
    variant_id = f"{base_id}_var_{suffix}"
    variant_dir = base_dir / "variants" / suffix
    variant_dir.mkdir(parents=True, exist_ok=True)

    final_path = variant_dir / "06_final.glb"
    if final_path.exists():
        status_log(f"  variant {suffix} already done, skip")
        return final_path

    # Build new SDXL prompt for the variant
    positive = p.build_positive_prompt(variant_def["prompt"])
    negative = p.build_negative_prompt()

    # 1. Generate variant input image
    pid = p.post_prompt(p.sdxl_workflow(positive, negative, f"variant_input_{variant_id}", seed=hash(suffix) % 100000))
    result, _ = p.wait_for(pid, "variant_sdxl", timeout=180)
    img_info = result["outputs"]["7"]["images"][0]
    img_bytes = p.fetch_image(img_info["filename"], img_info["subfolder"], "output")
    raw_path = variant_dir / "01_input_raw.png"
    raw_path.write_bytes(img_bytes)

    # 2. Mask
    from PIL import Image
    from rembg import remove
    img = Image.open(io.BytesIO(img_bytes))
    masked = remove(img, session=rembg_session, alpha_matting=True,
                    alpha_matting_foreground_threshold=240,
                    alpha_matting_background_threshold=10)
    masked_path = variant_dir / "02_input_masked.png"
    masked.save(masked_path)
    cf_filename = f"variant_input_{variant_id}.png"
    p.upload_image(masked_path, cf_filename)

    # 3. Texture the existing decimated mesh with this new image
    prefix = f"variant_textured_{variant_id}"
    pid = p.post_prompt(texture_variant_workflow(decim_path, cf_filename, prefix))
    _, t = p.wait_for(pid, "variant_texture", timeout=600)

    # Find output
    candidates = []
    for root in [Path("C:/ComfyUI/temp"), Path("C:/ComfyUI/output")]:
        candidates.extend(root.glob(f"{prefix}*.glb"))
    if not candidates:
        status_log(f"  variant {suffix} produced no GLB")
        return None
    src = max(candidates, key=lambda x: x.stat().st_mtime)
    shutil.copy(src, final_path)
    return final_path


def phase_variants(rembg_session):
    status_log("=" * 78)
    status_log("PHASE 3: Texture variants")
    status_log("=" * 78)

    # Compute total work
    total = sum(len(v) for v in TEXTURE_VARIANTS.values())
    status_log(f"  {total} texture variants planned across {len(TEXTURE_VARIANTS)} base assets")

    done = 0
    failed = 0
    start = time.time()
    for base_id, variants in TEXTURE_VARIANTS.items():
        # Find category by looking up the asset
        base_asset = next((a for a in p.ASSETS + BATCH2_ASSETS if a["id"] == base_id), None)
        if not base_asset:
            status_log(f"  unknown base asset {base_id}, skip")
            continue

        for variant in variants:
            done += 1
            status_log("")
            status_log("-" * 78)
            status_log(f"VARIANT [{done}/{total}] {base_id} -> {variant['suffix']}")
            status_log(f"  prompt: {variant['prompt'][:80]}...")
            status_log("-" * 78)
            try:
                result = make_variant(base_id, base_asset["category"], variant, rembg_session)
                if result:
                    status_log(f"  saved: {result}")
                else:
                    failed += 1
            except Exception as e:
                status_log(f"  FAILED: {e}")
                failed += 1

    status_log(f"\nvariants done: {done-failed}/{total} ok, {failed} failed in {(time.time()-start)/60:.1f} min")


# ============================================================================
# MAIN
# ============================================================================
def main():
    status_log("=" * 78)
    status_log("OVERNIGHT CONTINUATION ORCHESTRATOR")
    status_log(f"  Phase 1: Re-roll bad batch1 assets")
    status_log(f"  Phase 2: Batch2 — {len(BATCH2_ASSETS)} new assets")
    status_log(f"  Phase 3: {sum(len(v) for v in TEXTURE_VARIANTS.values())} texture variants")
    status_log("=" * 78)

    # 1. Wait for batch1 to finish
    batch1_status = wait_for_batch1_done()

    # 2. Verify ComfyUI still up
    try:
        urllib.request.urlopen(f"{COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        status_log(f"FATAL: ComfyUI unreachable: {e}")
        return 1

    # 3. Load rembg
    status_log("Loading rembg session...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    overall_start = time.time()

    # Phase 1: rerolls
    try:
        phase_rerolls(batch1_status, rembg_session)
    except Exception as e:
        status_log(f"PHASE 1 ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    # Phase 2: batch2
    try:
        phase_batch2(rembg_session)
    except Exception as e:
        status_log(f"PHASE 2 ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    # Phase 3: variants
    try:
        phase_variants(rembg_session)
    except Exception as e:
        status_log(f"PHASE 3 ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    total_min = (time.time() - overall_start) / 60
    status_log("")
    status_log("=" * 78)
    status_log(f"ALL PHASES COMPLETE — {total_min:.1f} min total")
    status_log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
