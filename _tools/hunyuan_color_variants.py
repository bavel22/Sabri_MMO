"""
Color/texture variant generator — creates 3-4 thematically-appropriate color
variants for every asset in generated_assets/.

Each variant re-textures the existing decimated mesh (04b_decimated_untextured.glb)
with a new SDXL-generated input image describing the variant color/style. No
new geometry — just texture replacement. ~90s per variant.

Variants are saved to:
  generated_assets/<category>/<asset_id>/variants/<color_suffix>/06_final.glb

Idempotent — skips variants where 06_final.glb already exists.

Color palettes are hand-tuned per asset to ensure colors make sense for the
subject (e.g., trees get seasonal palettes, stone gets material variants,
metals get patina/finish variants).

Waits for cleanup orchestrator to finish before starting (so we don't conflict
on ComfyUI queue).
"""
import io
import json
import shutil
import sys
import time
import traceback
import urllib.request
import uuid
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)
sys.path.insert(0, "C:/Sabri_MMO/_tools")

import hunyuan_asset_pipeline as p
import hunyuan_overnight_continue as oc

ROOT = p.ROOT
COMFY_URL = p.COMFY_URL
LOG_FILE = ROOT / "_color_variants.log"


# ============================================================================
# COLOR VARIANT PALETTES
# Hand-tuned per asset for thematic appropriateness.
# Each entry: list of {"suffix": "name", "prompt": "color/style description"}
# The prompt is appended/replaces the original subject during input generation.
# ============================================================================

# --- Reusable palettes (assigned to multiple assets) ---
WOOD_PALETTE = [
    {"suffix": "light_oak",  "prompt": "natural golden tan light oak wood with visible grain"},
    {"suffix": "dark_walnut","prompt": "rich dark brown walnut wood, deep grain, polished"},
    {"suffix": "weathered",  "prompt": "weathered gray driftwood, sun-bleached, aged"},
    {"suffix": "red_painted","prompt": "painted red wood with worn chipped paint, rustic"},
]

STONE_PALETTE = [
    {"suffix": "gray_granite",  "prompt": "weathered gray granite stone, natural rough texture"},
    {"suffix": "white_marble",  "prompt": "polished white marble stone with subtle gray veining"},
    {"suffix": "sandstone_tan", "prompt": "warm tan sandstone, earthy yellow-brown"},
    {"suffix": "mossy_green",   "prompt": "gray stone covered in patches of green moss and lichen"},
]

IRON_METAL_PALETTE = [
    {"suffix": "black_iron",   "prompt": "black wrought iron metal, matte dark finish"},
    {"suffix": "bronze",       "prompt": "polished warm bronze metal, golden brown patina"},
    {"suffix": "copper_patina","prompt": "copper metal with green-blue verdigris patina"},
    {"suffix": "gold_trimmed", "prompt": "gold-trimmed iron, bright metallic gold accents"},
]

FABRIC_PALETTE = [
    {"suffix": "red_white",  "prompt": "red and white striped fabric"},
    {"suffix": "blue_white", "prompt": "blue and white striped fabric"},
    {"suffix": "green_gold", "prompt": "green and gold patterned fabric"},
    {"suffix": "purple",     "prompt": "rich purple velvet fabric with gold trim"},
]


COLOR_VARIANTS = {
    # ============================================================================
    # VEGETATION (37) — seasonal / species variants
    # ============================================================================
    "tree_oak": [
        {"suffix": "lush_green",  "prompt": "lush vibrant green oak tree, full leafy summer canopy, thick brown trunk"},
        {"suffix": "autumn_red",  "prompt": "oak tree with bright red autumn leaves, brown trunk"},
        {"suffix": "autumn_gold", "prompt": "oak tree with golden yellow autumn leaves, brown trunk"},
        {"suffix": "winter_bare", "prompt": "bare oak tree in winter, no leaves, dark gnarled trunk"},
    ],
    "tree_oak_tall": [
        {"suffix": "lush_green",  "prompt": "tall lush green oak tree, full canopy, dark thick trunk"},
        {"suffix": "autumn_amber","prompt": "tall oak tree with amber and orange autumn leaves"},
        {"suffix": "winter_snow", "prompt": "tall oak with snow on bare branches, winter scene"},
    ],
    "tree_oak_old": [
        {"suffix": "ancient_green",  "prompt": "ancient gnarled oak with deep green leaves, twisted trunk"},
        {"suffix": "autumn_brown",   "prompt": "old oak with russet brown autumn leaves, twisted trunk"},
        {"suffix": "spring_yellow",  "prompt": "old oak with bright spring yellow-green new leaves, twisted trunk"},
    ],
    "tree_oak_dense": [
        {"suffix": "spring_green",   "prompt": "young oak with vibrant spring green leaves"},
        {"suffix": "autumn_red",     "prompt": "young oak with bright red autumn leaves"},
        {"suffix": "autumn_yellow",  "prompt": "young oak with bushy yellow autumn leaves"},
    ],
    "tree_maple": [
        {"suffix": "spring_green", "prompt": "maple tree with bright green spring leaves, slender trunk"},
        {"suffix": "autumn_red",   "prompt": "maple tree with vibrant red autumn leaves"},
        {"suffix": "autumn_gold",  "prompt": "maple tree with golden yellow autumn leaves"},
        {"suffix": "winter_bare",  "prompt": "bare maple in winter, no leaves, slim trunk"},
    ],
    "tree_maple_red": [
        {"suffix": "deep_red",     "prompt": "maple tree with deep crimson red leaves, full canopy"},
        {"suffix": "orange_red",   "prompt": "maple tree with orange-red autumn leaves"},
        {"suffix": "burgundy",     "prompt": "maple tree with dark burgundy purple leaves"},
    ],
    "tree_maple_yellow": [
        {"suffix": "bright_gold",  "prompt": "maple tree with bright golden yellow leaves"},
        {"suffix": "amber",        "prompt": "maple tree with rich amber-orange leaves"},
        {"suffix": "pale_yellow",  "prompt": "maple tree with pale yellow autumn leaves"},
    ],
    "tree_pine": [
        {"suffix": "dark_green",   "prompt": "dark forest green pine tree, dense conical needles"},
        {"suffix": "snow_covered", "prompt": "snow-covered pine, white snow on dark green needles, winter"},
        {"suffix": "frosted",      "prompt": "frosted pine with frost on green needles, winter morning"},
        {"suffix": "blue_pine",    "prompt": "blue spruce pine with blue-gray needles"},
    ],
    "tree_pine_tall": [
        {"suffix": "dark_green",   "prompt": "tall dark green pine tree, slender conical shape"},
        {"suffix": "snow_covered", "prompt": "tall snow-covered pine, dark green needles under white snow"},
        {"suffix": "spring_bright","prompt": "tall pine with bright fresh green new growth needles"},
    ],
    "tree_pine_wide": [
        {"suffix": "dark_green",  "prompt": "wide dark green pine, broad triangular shape, dense needles"},
        {"suffix": "snow_heavy",  "prompt": "wide pine heavy with snow, winter forest tree"},
        {"suffix": "autumn_rust", "prompt": "wide pine with rust-tinted autumn needles"},
    ],
    "tree_palm": [
        {"suffix": "tropical_green","prompt": "vibrant tropical green palm tree, bright fresh fronds"},
        {"suffix": "dry_brown",     "prompt": "drought-stressed palm with brown drying fronds"},
        {"suffix": "coconut_full",  "prompt": "coconut palm with full green fronds and coconuts"},
    ],
    "tree_bamboo": [
        {"suffix": "fresh_green",  "prompt": "fresh bright green bamboo stalks with new leaves"},
        {"suffix": "golden_yellow","prompt": "golden yellow bamboo with mature stalks"},
        {"suffix": "dark_jade",    "prompt": "dark jade green mature bamboo, dense cluster"},
    ],
    "tree_willow": [
        {"suffix": "spring_green", "prompt": "willow with vibrant green drooping spring leaves"},
        {"suffix": "autumn_yellow","prompt": "willow with golden yellow drooping autumn leaves"},
        {"suffix": "winter_bare",  "prompt": "bare willow in winter, no leaves, drooping branches"},
    ],
    "tree_birch": [
        {"suffix": "spring_green", "prompt": "white birch with bright spring green leaves, papery white bark"},
        {"suffix": "autumn_yellow","prompt": "white birch with golden yellow autumn leaves, white bark"},
        {"suffix": "winter_bare",  "prompt": "bare birch with white papery bark, no leaves"},
    ],
    "tree_jungle": [
        {"suffix": "lush_green",   "prompt": "lush vibrant green jungle tree with broad leaves and vines"},
        {"suffix": "exotic_purple","prompt": "exotic jungle tree with deep purple leaves, dark trunk, fantasy"},
        {"suffix": "monsoon_rain", "prompt": "rain-soaked jungle tree, dark wet bark, dripping leaves"},
    ],
    "tree_dead": [
        {"suffix": "charred_black","prompt": "charred black dead tree, blackened bark, no leaves"},
        {"suffix": "bleached_white","prompt": "bleached white dead tree, sun-faded pale wood"},
        {"suffix": "weathered_gray","prompt": "weathered gray dead tree, silver-gray bark, gnarled"},
    ],
    "tree_dead_old": [
        {"suffix": "charred_black","prompt": "ancient charred dead tree, black scorched bark, broken branches"},
        {"suffix": "moss_covered", "prompt": "ancient dead tree covered in pale moss, silver-gray wood"},
        {"suffix": "petrified",    "prompt": "petrified ancient dead tree, stone-like brown gray, fossilized"},
    ],
    "tree_dead_white": [
        {"suffix": "bone_white",   "prompt": "bone-white dead tree, smooth pale wood, ghostly"},
        {"suffix": "frost_covered","prompt": "frost-covered dead tree, white ice on pale wood"},
        {"suffix": "moonlit_silver","prompt": "silvery dead tree under moonlight, pale shimmering wood"},
    ],
    "tree_sakura": [
        {"suffix": "pink_full",    "prompt": "cherry blossom tree with full pink flower canopy"},
        {"suffix": "white_blooms", "prompt": "cherry blossom tree with white delicate flowers"},
        {"suffix": "deep_pink",    "prompt": "cherry tree with deep magenta pink blossoms"},
        {"suffix": "petals_falling","prompt": "cherry tree with falling pink petals, soft pink canopy"},
    ],
    "tree_sakura_pink": [
        {"suffix": "soft_pink",    "prompt": "cherry tree with soft pastel pink blossoms, dark trunk"},
        {"suffix": "vibrant_pink", "prompt": "cherry tree with vibrant hot pink blossoms"},
        {"suffix": "white_pink",   "prompt": "cherry tree with white-pink mixed blossoms"},
    ],
    "tree_sakura_full": [
        {"suffix": "blush_pink",   "prompt": "full cherry tree with blush pink dense blossoms"},
        {"suffix": "snow_white",   "prompt": "full cherry tree with pure white snow-like blossoms"},
        {"suffix": "rose_red",     "prompt": "full cherry tree with deep rose-red blossoms"},
    ],
    "bush_round": [
        {"suffix": "lush_green",  "prompt": "round bush with lush dense green leaves"},
        {"suffix": "red_berries", "prompt": "round bush with green leaves and bright red berries"},
        {"suffix": "white_flowers","prompt": "round bush with white flowers among green leaves"},
        {"suffix": "autumn_brown","prompt": "round bush with brown autumn leaves"},
    ],
    "bush_wide": [
        {"suffix": "lush_green",  "prompt": "wide spreading bush with dense green foliage"},
        {"suffix": "yellow_blooms","prompt": "wide bush with yellow flowers among green leaves"},
        {"suffix": "autumn_red",  "prompt": "wide bush with red autumn leaves"},
    ],
    "bush_small": [
        {"suffix": "lush_green",  "prompt": "small lush green round bush, dense leaves"},
        {"suffix": "purple_berries","prompt": "small bush with purple berries and green leaves"},
        {"suffix": "spring_yellow","prompt": "small bush with spring yellow-green new leaves"},
    ],
    "bush_dense": [
        {"suffix": "lush_green",  "prompt": "dense bush with thick vibrant green foliage"},
        {"suffix": "red_berries", "prompt": "dense bush with red berries scattered in green leaves"},
        {"suffix": "white_flowers","prompt": "dense bush with white spring flowers"},
    ],
    "bush_sparse": [
        {"suffix": "scrub_green", "prompt": "sparse open bush with scattered green leaves, scrubby"},
        {"suffix": "dry_brown",   "prompt": "sparse dry bush with brown dying leaves"},
        {"suffix": "yellow_flowers","prompt": "sparse bush with sparse yellow wildflowers"},
    ],
    "bush_flowering": [
        {"suffix": "white",       "prompt": "bush with white flowers among small green leaves"},
        {"suffix": "pink",        "prompt": "bush with pink flowers among small leaves"},
        {"suffix": "yellow",      "prompt": "bush with bright yellow flowers among green leaves"},
        {"suffix": "purple",      "prompt": "bush with vibrant purple flowers among green leaves"},
    ],
    "bush_thorny": [
        {"suffix": "red_berries", "prompt": "thorny bramble bush with dark green leaves and red berries"},
        {"suffix": "blackberries","prompt": "thorny bramble bush with green leaves and dark blackberries"},
        {"suffix": "winter_dry",  "prompt": "winter thorny bush, brown thorns, no leaves"},
    ],
    "mushroom_giant": [
        {"suffix": "red_white",   "prompt": "giant mushroom with classic red cap and white spots, white stem"},
        {"suffix": "brown_forest","prompt": "giant brown forest mushroom with rounded cap, woodland"},
        {"suffix": "blue_glow",   "prompt": "giant magical blue glowing mushroom, fantasy fungus"},
        {"suffix": "purple_dark", "prompt": "dark purple poisonous mushroom with sinister cap"},
    ],
    "mushroom_brown": [
        {"suffix": "tan_brown",   "prompt": "brown forest mushroom, tan brown cap, classic woodland"},
        {"suffix": "dark_brown",  "prompt": "dark chocolate brown mushroom, rich color"},
        {"suffix": "red_cap",     "prompt": "brown mushroom with red cap, woodland fungi"},
    ],
    "mushroom_blue": [
        {"suffix": "magical_blue","prompt": "magical blue glowing mushroom, bioluminescent fantasy"},
        {"suffix": "cyan_teal",   "prompt": "cyan-teal mushroom with glowing tall stem"},
        {"suffix": "purple_blue", "prompt": "purple-blue magical mushroom, fantasy fungus"},
    ],
    "flower_cluster": [
        {"suffix": "yellow_daisies","prompt": "cluster of yellow daisy flowers on green grass tuft"},
        {"suffix": "white_daisies", "prompt": "cluster of white daisies on green grass tuft"},
        {"suffix": "pink_blossoms", "prompt": "cluster of small pink flowers on grass tuft"},
        {"suffix": "purple_clovers","prompt": "cluster of purple clover flowers on grass tuft"},
    ],
    "flower_red_tulips": [
        {"suffix": "red",         "prompt": "small cluster of red tulip flowers on green grass"},
        {"suffix": "yellow",      "prompt": "small cluster of yellow tulip flowers on green grass"},
        {"suffix": "pink",        "prompt": "small cluster of pink tulip flowers on green grass"},
        {"suffix": "white",       "prompt": "small cluster of white tulip flowers on green grass"},
    ],
    "flower_purple": [
        {"suffix": "purple",      "prompt": "cluster of purple wildflowers on grass tuft"},
        {"suffix": "blue",        "prompt": "cluster of blue wildflowers on grass tuft"},
        {"suffix": "pink",        "prompt": "cluster of pink wildflowers on grass tuft"},
        {"suffix": "mixed",       "prompt": "cluster of mixed wildflowers, multiple colors"},
    ],
    "cactus_desert": [
        {"suffix": "green",        "prompt": "tall green saguaro cactus, healthy desert plant"},
        {"suffix": "blooming_pink","prompt": "tall cactus with pink blooming flowers, desert"},
        {"suffix": "dried_brown",  "prompt": "drought-stressed brown cactus, dying desert plant"},
    ],
    "cactus_round": [
        {"suffix": "green",        "prompt": "round green barrel cactus, healthy ribbed desert plant"},
        {"suffix": "blooming_red", "prompt": "round cactus with red flower on top, blooming desert plant"},
        {"suffix": "blue_green",   "prompt": "blue-green barrel cactus, southwestern desert"},
    ],
    "reeds_cattails": [
        {"suffix": "fresh_green", "prompt": "fresh green cattail reeds with brown heads, marsh plant"},
        {"suffix": "autumn_gold", "prompt": "golden autumn cattail reeds, dried tan stalks"},
        {"suffix": "winter_brown","prompt": "winter brown dead cattail reeds"},
    ],

    # ============================================================================
    # ARCHITECTURE (20) — material/style variants
    # ============================================================================
    "house_prontera_small": [
        {"suffix": "red_roof_stone",  "prompt": "small medieval stone house with classic red clay tile roof, gray stone walls, wooden door"},
        {"suffix": "blue_roof_stone", "prompt": "small medieval stone house with blue slate roof tiles, gray stone walls"},
        {"suffix": "green_moss",      "prompt": "small medieval stone house with green moss-covered roof, weathered stone walls"},
        {"suffix": "snow_covered",    "prompt": "small medieval house covered in white snow, winter scene"},
    ],
    "house_prontera_medium": [
        {"suffix": "red_roof",        "prompt": "medieval two story stone house with red clay tile roof, stone walls, chimney"},
        {"suffix": "blue_roof",       "prompt": "medieval two story stone house with blue slate roof, gray stone walls"},
        {"suffix": "weathered_brown", "prompt": "medieval house with brown weathered roof tiles, aged stone walls"},
        {"suffix": "white_plaster",   "prompt": "medieval house with white-plastered walls and red roof, charming"},
    ],
    "house_prontera_tall": [
        {"suffix": "red_roof",     "prompt": "three story stone house with red tile roof, balcony, multiple windows"},
        {"suffix": "blue_roof",    "prompt": "three story stone house with blue slate roof, balcony"},
        {"suffix": "green_moss",   "prompt": "three story house with mossy green roof, weathered stone walls"},
    ],
    "house_prontera_wide": [
        {"suffix": "red_roof",     "prompt": "wide medieval stone house with red clay tile roof, large door, multiple windows"},
        {"suffix": "thatched",     "prompt": "wide medieval house with thatched straw roof, stone walls"},
        {"suffix": "white_walls",  "prompt": "wide medieval house with whitewashed walls and red roof"},
    ],
    "house_prontera_corner": [
        {"suffix": "red_roof",      "prompt": "corner L-shape medieval house with red clay tile roof, two facades, gray stone"},
        {"suffix": "blue_roof",     "prompt": "corner L-shape medieval house with blue slate roof, gray stone walls"},
        {"suffix": "weathered",     "prompt": "weathered corner medieval house, faded brown roof, aged stone"},
    ],
    "hut_payon": [
        {"suffix": "natural_wood", "prompt": "Korean traditional thatched roof hut with natural light wood frame, tan thatch"},
        {"suffix": "dark_stained", "prompt": "Korean thatched hut with dark stained wooden frame, brown thatched roof"},
        {"suffix": "weathered_gray","prompt": "weathered Korean hut with gray aged wood frame and tan thatch"},
        {"suffix": "red_painted",  "prompt": "Korean hut with red lacquered wood frame and tan thatched roof"},
    ],
    "hut_payon_long": [
        {"suffix": "natural_wood", "prompt": "long Korean thatched roof building with natural wood frame, multiple paper doors"},
        {"suffix": "dark_stained", "prompt": "long Korean building with dark stained wooden frame and tan thatch"},
        {"suffix": "weathered",    "prompt": "weathered long Korean building with gray aged wood and brown thatch"},
    ],
    "hut_payon_small": [
        {"suffix": "natural_wood", "prompt": "small Korean thatched hut, natural wood frame, single paper door"},
        {"suffix": "dark_stained", "prompt": "small Korean hut, dark stained wood, brown thatched roof"},
        {"suffix": "red_lacquer",  "prompt": "small Korean hut with red lacquered wood frame, tan thatched roof"},
    ],
    "house_morroc": [
        {"suffix": "sandstone_tan","prompt": "Middle Eastern adobe building, warm tan sandstone walls, flat roof"},
        {"suffix": "terracotta",   "prompt": "Middle Eastern adobe with terracotta red walls, flat roof"},
        {"suffix": "whitewashed",  "prompt": "whitewashed adobe building with white walls, flat roof, desert"},
        {"suffix": "yellow_ochre", "prompt": "yellow ochre adobe walls, desert architecture, flat roof"},
    ],
    "house_morroc_arch": [
        {"suffix": "sandstone",   "prompt": "sandstone adobe with arched doorway, tan walls, decorative arch"},
        {"suffix": "white_blue",  "prompt": "whitewashed adobe with blue painted arch and door, Mediterranean"},
        {"suffix": "terracotta",  "prompt": "terracotta red adobe with arched doorway"},
    ],
    "house_morroc_dome": [
        {"suffix": "sandstone",   "prompt": "sandstone adobe with small dome, tan walls, arched windows"},
        {"suffix": "blue_dome",   "prompt": "white adobe with blue painted dome, Middle Eastern"},
        {"suffix": "gold_dome",   "prompt": "white adobe with golden dome, ornate Middle Eastern architecture"},
    ],
    "tent_morroc": [
        {"suffix": "tan_canvas",  "prompt": "desert tan canvas tent, beige fabric, simple nomad shelter"},
        {"suffix": "red_striped", "prompt": "red and white striped fabric desert tent, festive"},
        {"suffix": "blue_pattern","prompt": "blue patterned desert tent, ornate Bedouin fabric"},
        {"suffix": "dark_purple", "prompt": "dark purple silk desert tent, nobility"},
    ],
    "house_alberta_wood": [
        {"suffix": "natural_wood", "prompt": "wooden port building, natural pine wood siding, gabled roof"},
        {"suffix": "dark_stained", "prompt": "dark stained wooden port building, gabled roof, large windows"},
        {"suffix": "weathered",    "prompt": "weathered gray wooden port building, aged sea-spray wood"},
        {"suffix": "red_paint",    "prompt": "red painted wooden port building, white trim, charming"},
    ],
    "dock_wooden": [
        {"suffix": "natural",     "prompt": "wooden dock pier section, natural light tan wood planks"},
        {"suffix": "weathered",   "prompt": "weathered gray dock pier, sun and salt aged wood"},
        {"suffix": "dark_tar",    "prompt": "dark tar-stained wooden dock, black coating on planks"},
    ],
    "tower_geffen": [
        {"suffix": "blue_roof",    "prompt": "tall medieval stone tower with conical blue roof, gray stone walls, wizard tower"},
        {"suffix": "red_roof",     "prompt": "tall medieval stone tower with red conical roof, gray stone walls"},
        {"suffix": "purple_magic", "prompt": "tall stone tower with purple magical glow, dark stone walls, wizard"},
        {"suffix": "weathered",    "prompt": "ancient weathered stone tower with mossy green roof, aged"},
    ],
    "tower_gate": [
        {"suffix": "gray_stone",  "prompt": "medieval gray stone gate tower with crenellations, arched gateway"},
        {"suffix": "tan_sandstone","prompt": "tan sandstone gate tower with battlements, warm desert color"},
        {"suffix": "weathered",   "prompt": "weathered moss-covered gate tower, aged stone"},
        {"suffix": "white_marble","prompt": "white marble gate tower, polished stone, regal"},
    ],
    "wall_stone_arrow": [
        {"suffix": "gray_stone",  "prompt": "medieval gray stone wall with arrow slits, weathered"},
        {"suffix": "tan_sandstone","prompt": "tan sandstone wall with arrow slits, warm color"},
        {"suffix": "mossy",       "prompt": "mossy gray stone wall with arrow slits, ivy and moss"},
        {"suffix": "snow_covered","prompt": "snow-covered stone wall, white snow on gray stone"},
    ],
    "wall_stone_battlement": [
        {"suffix": "gray_stone",  "prompt": "gray stone castle battlement with crenellations on top"},
        {"suffix": "tan_sandstone","prompt": "tan sandstone battlement, warm desert castle"},
        {"suffix": "weathered",   "prompt": "weathered moss-covered battlement, aged stone"},
        {"suffix": "white_marble","prompt": "white marble battlement, polished stone, regal castle"},
    ],
    "monument_obelisk": [
        {"suffix": "gray_stone",  "prompt": "tall gray stone obelisk monument, square tapered shape, ancient"},
        {"suffix": "white_marble","prompt": "white marble obelisk with carved details, polished"},
        {"suffix": "black_obsidian","prompt": "black obsidian obelisk, glossy dark stone"},
        {"suffix": "gold_capped", "prompt": "stone obelisk with gold-capped pyramidal top, ancient Egyptian"},
    ],
    "shrine_small": [
        {"suffix": "gray_stone",  "prompt": "small gray stone shrine with sloped roof, weathered"},
        {"suffix": "white_marble","prompt": "small white marble shrine, polished, sacred"},
        {"suffix": "wooden_red",  "prompt": "small red lacquered wooden shrine, Asian style, ornate"},
        {"suffix": "mossy",       "prompt": "small mossy stone shrine, overgrown, ancient and forgotten"},
    ],

    # ============================================================================
    # PROPS (33) — material/finish variants
    # ============================================================================
    "lamppost_medieval": [
        {"suffix": "black_iron",   "prompt": "tall black iron lamppost with black metal frame, glass lantern"},
        {"suffix": "bronze",       "prompt": "bronze metal lamppost with warm patina, glass lantern at top"},
        {"suffix": "copper_green", "prompt": "copper lamppost with green verdigris patina, glass lantern"},
        {"suffix": "wrought_dark", "prompt": "ornate wrought iron lamppost, dark finish, ornate design"},
    ],
    "lamppost_amatsu": [
        {"suffix": "red_paper",    "prompt": "Asian paper lantern lamppost, red paper lantern, dark wood post"},
        {"suffix": "white_paper",  "prompt": "Asian paper lantern lamppost, white paper lantern, dark wood post"},
        {"suffix": "blue_paper",   "prompt": "Asian paper lantern lamppost, blue paper lantern, dark wood post"},
        {"suffix": "gold_lantern", "prompt": "Asian paper lantern with gold-yellow paper, ornate wooden post"},
    ],
    "lamppost_iron": [
        {"suffix": "black_iron",   "prompt": "tall black iron lamppost with four-sided glass lantern"},
        {"suffix": "rusted",       "prompt": "rusted iron lamppost, brown-orange rust patina"},
        {"suffix": "bronze",       "prompt": "bronze metal lamppost with warm gold patina"},
        {"suffix": "white_painted","prompt": "white-painted iron lamppost, ornate decorative design"},
    ],
    "sign_wooden": [
        {"suffix": "natural_wood", "prompt": "natural light brown wooden sign on post, blank plank"},
        {"suffix": "dark_stained", "prompt": "dark stained wooden sign on post, deep brown wood"},
        {"suffix": "red_painted",  "prompt": "red painted wooden sign with worn paint, rustic"},
        {"suffix": "weathered_gray","prompt": "weathered gray wooden sign, sun-bleached aged wood"},
    ],
    "sign_hanging": [
        {"suffix": "natural_wood", "prompt": "natural wood hanging sign on iron bracket, light brown"},
        {"suffix": "dark_stained", "prompt": "dark stained hanging sign with iron chains, deep brown"},
        {"suffix": "tavern_red",   "prompt": "red painted tavern sign with iron bracket, charming worn paint"},
    ],
    "sign_planted": [
        {"suffix": "natural_wood", "prompt": "natural wood planted sign on post, trapezoidal shape"},
        {"suffix": "dark_stained", "prompt": "dark stained wooden planted sign, deep brown"},
        {"suffix": "weathered",    "prompt": "weathered gray planted sign, aged sun-bleached"},
    ],
    "banner_long": [
        {"suffix": "red_gold",    "prompt": "long vertical banner, red fabric with gold crest, on wooden pole"},
        {"suffix": "blue_silver", "prompt": "long vertical banner, blue fabric with silver design, on wooden pole"},
        {"suffix": "green_yellow","prompt": "long vertical banner, green fabric with yellow heraldic design"},
        {"suffix": "purple_white","prompt": "long vertical banner, purple fabric with white design, royal"},
    ],
    "banner_horizontal": [
        {"suffix": "red_white",   "prompt": "horizontal banner stretched between poles, red and white fabric"},
        {"suffix": "blue_white",  "prompt": "horizontal banner, blue and white striped fabric"},
        {"suffix": "green_gold",  "prompt": "horizontal banner, green fabric with gold trim and design"},
        {"suffix": "black_silver","prompt": "horizontal banner, black fabric with silver heraldic design"},
    ],
    "awning_striped": [
        {"suffix": "red_cream",   "prompt": "striped fabric awning, red and cream stripes, market style"},
        {"suffix": "blue_white",  "prompt": "striped fabric awning, blue and white stripes"},
        {"suffix": "green_white", "prompt": "striped fabric awning, green and white stripes"},
        {"suffix": "yellow_red",  "prompt": "striped fabric awning, yellow and red stripes, festive"},
    ],
    "awning_market": [
        {"suffix": "red",         "prompt": "small market stall awning, red fabric on wooden corner posts"},
        {"suffix": "blue",        "prompt": "small market stall awning, blue fabric on wooden posts"},
        {"suffix": "green",       "prompt": "small market stall awning, green fabric on wooden posts"},
        {"suffix": "yellow",      "prompt": "small market stall awning, yellow fabric on wooden posts"},
    ],
    "barrel_wooden": [
        {"suffix": "light_oak",   "prompt": "small wooden barrel, light oak staves with iron bands"},
        {"suffix": "dark_walnut", "prompt": "small wooden barrel, dark walnut wood with iron bands"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden barrel, aged sea-spray wood"},
        {"suffix": "red_painted", "prompt": "red painted wooden barrel with worn chipped paint"},
    ],
    "barrel_large": [
        {"suffix": "light_oak",   "prompt": "large wooden barrel, light oak staves with iron bands, tall"},
        {"suffix": "dark_walnut", "prompt": "large wooden barrel, dark walnut wood with iron bands, tall"},
        {"suffix": "weathered",   "prompt": "large weathered gray wooden barrel, aged"},
    ],
    "barrel_open": [
        {"suffix": "light_oak",   "prompt": "open wooden barrel without lid, light oak with iron bands, grain inside"},
        {"suffix": "dark_walnut", "prompt": "open wooden barrel, dark walnut wood, no lid, contents visible"},
        {"suffix": "weathered",   "prompt": "open weathered wooden barrel, gray aged wood"},
    ],
    "crate_wooden": [
        {"suffix": "light_oak",   "prompt": "wooden shipping crate, light tan wood with diagonal slats"},
        {"suffix": "dark_walnut", "prompt": "wooden shipping crate, dark stained wood with diagonal slats"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden crate, aged warehouse"},
        {"suffix": "branded",     "prompt": "wooden crate with branded merchant logo, light wood"},
    ],
    "crate_small": [
        {"suffix": "light_oak",   "prompt": "small wooden crate, light tan wood with diagonal slats"},
        {"suffix": "dark_walnut", "prompt": "small wooden crate, dark stained wood"},
        {"suffix": "weathered",   "prompt": "small weathered gray wooden crate"},
    ],
    "crate_large": [
        {"suffix": "light_oak",   "prompt": "large wooden shipping crate, light wood, slatted sides"},
        {"suffix": "dark_walnut", "prompt": "large wooden shipping crate, dark stained wood, heavy"},
        {"suffix": "weathered",   "prompt": "large weathered gray wooden crate, aged"},
    ],
    "sack_burlap": [
        {"suffix": "cream_burlap","prompt": "burlap sack, cream-colored coarse fabric, tied with rope"},
        {"suffix": "brown_burlap","prompt": "burlap sack, light brown coarse fabric, tied at top"},
        {"suffix": "white_canvas","prompt": "white canvas sack, smoother fabric, tied with rope"},
        {"suffix": "gray_aged",   "prompt": "weathered gray aged burlap sack, dirty fabric"},
    ],
    "sack_open": [
        {"suffix": "cream_burlap","prompt": "open burlap sack standing up, cream-colored coarse fabric, grain visible"},
        {"suffix": "brown_burlap","prompt": "open brown burlap sack, top folded down, contents visible"},
        {"suffix": "white_canvas","prompt": "open white canvas sack, top folded down"},
    ],
    "basket_woven": [
        {"suffix": "tan_wicker",  "prompt": "round woven wicker basket with handle, light tan reed"},
        {"suffix": "dark_wicker", "prompt": "dark brown wicker basket with handle, aged reed"},
        {"suffix": "white_painted","prompt": "white painted wicker basket with handle"},
        {"suffix": "red_pattern", "prompt": "wicker basket with red painted pattern, decorative"},
    ],
    "basket_apple": [
        {"suffix": "red_apples",  "prompt": "wicker basket filled with bright red apples, light brown weave"},
        {"suffix": "green_apples","prompt": "wicker basket filled with green apples, light brown weave"},
        {"suffix": "mixed_fruit", "prompt": "wicker basket filled with mixed fruit, apples and oranges"},
    ],
    "cart_wooden": [
        {"suffix": "light_oak",   "prompt": "small two wheel wooden cart, light oak wood with iron rim, pull handle"},
        {"suffix": "dark_walnut", "prompt": "small wooden cart, dark stained wood with iron rim"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden cart, aged country wagon"},
        {"suffix": "red_painted", "prompt": "red painted wooden cart with worn paint, charming"},
    ],
    "cart_kafra": [
        {"suffix": "red_courier", "prompt": "small two wheel cart with red Kafra courier awning, wooden body"},
        {"suffix": "blue_courier","prompt": "small cart with blue courier awning, wooden body, delivery cart"},
        {"suffix": "green_courier","prompt": "small cart with green courier awning, wooden body"},
    ],
    "cart_merchant": [
        {"suffix": "wooden_natural","prompt": "wooden merchant pull cart with goods piled, natural wood, two wheels"},
        {"suffix": "dark_stained",  "prompt": "wooden merchant cart, dark stained wood, two large wheels"},
        {"suffix": "weathered",     "prompt": "weathered wooden merchant cart, gray aged wood"},
    ],
    "market_stall": [
        {"suffix": "red_awning",  "prompt": "medieval market stall with red striped awning, wooden counter"},
        {"suffix": "blue_awning", "prompt": "medieval market stall with blue striped awning, wooden counter"},
        {"suffix": "green_awning","prompt": "medieval market stall with green awning, wooden counter"},
        {"suffix": "yellow_awning","prompt": "medieval market stall with yellow awning, wooden counter"},
    ],
    "stall_market_red": [
        {"suffix": "red_white",   "prompt": "medieval market stall with red and white striped awning, wooden display"},
        {"suffix": "deep_red",    "prompt": "medieval market stall with deep red fabric awning, wooden counter"},
        {"suffix": "burgundy",    "prompt": "medieval market stall with burgundy fabric awning"},
    ],
    "stall_market_blue": [
        {"suffix": "blue_white",  "prompt": "medieval market stall with blue and white awning, wooden display"},
        {"suffix": "navy_blue",   "prompt": "medieval market stall with navy blue awning, wooden counter"},
        {"suffix": "teal",        "prompt": "medieval market stall with teal awning, wooden display table"},
    ],
    "weapon_rack": [
        {"suffix": "dark_wood",   "prompt": "wooden weapon rack with swords and axes, dark walnut wood"},
        {"suffix": "light_oak",   "prompt": "wooden weapon rack with weapons displayed, light oak wood"},
        {"suffix": "iron_metal",  "prompt": "iron metal weapon rack, black metal frame with weapons"},
    ],
    "table_wooden": [
        {"suffix": "light_oak",   "prompt": "rustic medieval wooden table, light oak, four legs, plank top"},
        {"suffix": "dark_walnut", "prompt": "medieval wooden table, dark walnut wood, plank top"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden table, aged country furniture"},
        {"suffix": "red_painted", "prompt": "red painted wooden table with worn chipped paint"},
    ],
    "bench_wooden": [
        {"suffix": "light_oak",   "prompt": "long wooden bench, light oak with four legs, plank seat, medieval"},
        {"suffix": "dark_walnut", "prompt": "long wooden bench, dark walnut wood, plank seat"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden bench, aged country seat"},
    ],
    "haystack": [
        {"suffix": "golden",      "prompt": "round haystack pile of golden yellow hay, farm bale"},
        {"suffix": "dried_brown", "prompt": "round haystack pile of dried tan-brown hay, weathered"},
        {"suffix": "fresh_green", "prompt": "round haystack pile of fresh green-yellow hay"},
    ],
    "fence_wooden": [
        {"suffix": "natural",     "prompt": "wooden fence section with horizontal rails, natural light brown wood"},
        {"suffix": "dark_stained","prompt": "wooden fence section, dark stained wood"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden fence, sun-bleached"},
        {"suffix": "white_paint", "prompt": "white painted wooden fence, picket-style"},
    ],
    "fence_long": [
        {"suffix": "natural",     "prompt": "long wooden fence section, natural light brown wood, four rails"},
        {"suffix": "dark_stained","prompt": "long wooden fence, dark stained wood"},
        {"suffix": "weathered",   "prompt": "long weathered gray wooden fence, aged"},
    ],
    "fence_stone": [
        {"suffix": "gray_stone",  "prompt": "low stone fence section, gray stacked stones, rough rural wall"},
        {"suffix": "tan_stone",   "prompt": "low stone fence, warm tan-brown stacked stones"},
        {"suffix": "mossy_stone", "prompt": "low stone fence covered in green moss patches, weathered"},
    ],

    # ============================================================================
    # TERRAIN (20)
    # ============================================================================
    "rock_small": [
        {"suffix": "gray_granite","prompt": "small rounded gray granite boulder, single rock"},
        {"suffix": "tan_sandstone","prompt": "small tan sandstone boulder, warm earthy color"},
        {"suffix": "mossy_green", "prompt": "small mossy boulder covered in green moss patches"},
        {"suffix": "snow_covered","prompt": "small rock partially covered in snow, gray stone in winter"},
    ],
    "rock_medium": [
        {"suffix": "gray_granite","prompt": "medium gray granite rock, irregular faceted shape"},
        {"suffix": "brown_sandstone","prompt": "medium tan-brown sandstone rock, warm color"},
        {"suffix": "dark_basalt", "prompt": "medium dark basalt rock, volcanic black-gray stone"},
        {"suffix": "white_marble","prompt": "medium white marble stone with subtle veining"},
    ],
    "rock_large": [
        {"suffix": "gray_granite","prompt": "large gray granite boulder with crack lines, prominent"},
        {"suffix": "red_sandstone","prompt": "large red sandstone boulder, southwestern desert"},
        {"suffix": "dark_basalt", "prompt": "large dark basalt boulder, volcanic stone, dark gray"},
        {"suffix": "snow_covered","prompt": "large boulder covered in snow, winter scene"},
    ],
    "rock_mossy": [
        {"suffix": "light_moss",  "prompt": "stone covered in light fresh green moss, forest floor"},
        {"suffix": "dark_moss",   "prompt": "stone covered in thick dark green moss, deep forest"},
        {"suffix": "lichen",      "prompt": "stone covered in pale lichen patches, weathered"},
        {"suffix": "snow_moss",   "prompt": "mossy stone with patches of snow, winter forest"},
    ],
    "boulder_huge": [
        {"suffix": "gray_granite","prompt": "massive gray granite boulder, rounded shape, prominent"},
        {"suffix": "brown_red",   "prompt": "massive red-brown sandstone boulder, southwestern"},
        {"suffix": "moss_covered","prompt": "massive boulder covered in moss and ferns, ancient forest"},
        {"suffix": "snow_covered","prompt": "massive boulder covered in snow, winter scene"},
    ],
    "rocks_cluster": [
        {"suffix": "gray_granite","prompt": "cluster of three gray granite rocks of different sizes"},
        {"suffix": "tan_sandstone","prompt": "cluster of tan sandstone rocks, warm earthy"},
        {"suffix": "mixed_mossy", "prompt": "cluster of mossy gray rocks with green patches"},
    ],
    "rocks_pile": [
        {"suffix": "gray_stones", "prompt": "pile of gray stone rocks and pebbles, mixed sizes"},
        {"suffix": "tan_stones",  "prompt": "pile of tan-brown rocks and pebbles"},
        {"suffix": "river_rocks", "prompt": "pile of smooth river rocks, gray and brown polished stones"},
    ],
    "rock_mossy_large": [
        {"suffix": "light_moss",  "prompt": "large mossy boulder, fresh green moss covering rounded shape"},
        {"suffix": "thick_moss",  "prompt": "large boulder thick with deep green moss, ancient forest"},
        {"suffix": "lichen_covered","prompt": "large rock covered in pale gray lichen, weathered"},
    ],
    "rock_split": [
        {"suffix": "gray_granite","prompt": "split gray rock with crack down middle, granite"},
        {"suffix": "iron_veins",  "prompt": "split rock with iron-rust veins running through, copper"},
        {"suffix": "mossy",       "prompt": "split rock with moss filling crack, weathered gray"},
    ],
    "cliff_face_low": [
        {"suffix": "gray_stone",  "prompt": "low cliff face, gray rocky wall with cracks"},
        {"suffix": "red_stone",   "prompt": "low cliff face, red sandstone rocky wall"},
        {"suffix": "tan_yellow",  "prompt": "low cliff face, yellow-tan sandstone wall"},
        {"suffix": "snow_covered","prompt": "low cliff face covered in snow, winter mountain"},
    ],
    "cliff_face_high": [
        {"suffix": "gray_stone",  "prompt": "tall gray rocky cliff face, layered stone strata"},
        {"suffix": "red_canyon",  "prompt": "tall red canyon cliff face, layered red sandstone"},
        {"suffix": "white_chalk", "prompt": "tall white chalk cliff face, pale layered stone"},
        {"suffix": "mossy",       "prompt": "tall cliff face covered in moss patches, green and gray"},
    ],
    "bridge_wooden": [
        {"suffix": "natural",     "prompt": "small wooden plank bridge, natural light brown wood"},
        {"suffix": "dark_stained","prompt": "small wooden bridge, dark stained wood"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden bridge, sun-bleached planks"},
        {"suffix": "red_painted", "prompt": "red painted wooden bridge, charming country"},
    ],
    "bridge_stone_arch": [
        {"suffix": "gray_stone",  "prompt": "stone arch bridge, gray weathered stone, single arch"},
        {"suffix": "tan_stone",   "prompt": "stone arch bridge, warm tan stone, single arch"},
        {"suffix": "mossy",       "prompt": "moss-covered stone arch bridge, ancient and overgrown"},
        {"suffix": "white_marble","prompt": "white marble arch bridge, polished stone, regal"},
    ],
    "bridge_rope": [
        {"suffix": "tan_rope",    "prompt": "rope and plank suspension bridge, tan ropes and natural planks"},
        {"suffix": "dark_weathered","prompt": "weathered dark rope bridge, aged hemp ropes and dark planks"},
        {"suffix": "snowy",       "prompt": "rope bridge with snow on planks, winter scene"},
    ],
    "stairs_stone": [
        {"suffix": "gray_stone",  "prompt": "stone steps section, gray weathered stone steps"},
        {"suffix": "tan_sandstone","prompt": "tan sandstone steps, warm earthy color"},
        {"suffix": "white_marble","prompt": "white marble steps, polished, regal"},
        {"suffix": "mossy",       "prompt": "moss-covered stone steps, overgrown ancient"},
    ],
    "stairs_wooden": [
        {"suffix": "natural",     "prompt": "wooden staircase, natural light brown plank steps"},
        {"suffix": "dark_stained","prompt": "wooden staircase, dark stained wood steps"},
        {"suffix": "weathered",   "prompt": "weathered gray wooden staircase, aged"},
    ],
    "path_stone": [
        {"suffix": "gray_stones", "prompt": "stone paved path section, irregular flat gray stones"},
        {"suffix": "tan_stones",  "prompt": "stone paved path, irregular tan-brown flat stones"},
        {"suffix": "dark_slate",  "prompt": "stone paved path, dark slate gray flat stones"},
        {"suffix": "mossy",       "prompt": "stone path with moss between stones, ancient overgrown"},
    ],
    "well_wooden": [
        {"suffix": "natural_wood","prompt": "medieval village well, stone base with natural wood roof"},
        {"suffix": "dark_wood",   "prompt": "medieval well with dark stained wood roof, gray stone wall"},
        {"suffix": "weathered",   "prompt": "weathered medieval well, aged gray wood roof and mossy stone"},
    ],
    "well_stone": [
        {"suffix": "gray_stone",  "prompt": "round stone well with gray stone wall, wooden roof"},
        {"suffix": "tan_stone",   "prompt": "round stone well with tan sandstone wall, wooden roof"},
        {"suffix": "mossy",       "prompt": "moss-covered stone well, ancient overgrown"},
        {"suffix": "red_brick",   "prompt": "round well with red brick wall, charming"},
    ],
    "fountain_basin": [
        {"suffix": "gray_stone",  "prompt": "stone fountain with circular basin, gray weathered stone, town center"},
        {"suffix": "white_marble","prompt": "white marble fountain, polished stone, ornate"},
        {"suffix": "tan_sandstone","prompt": "tan sandstone fountain, warm earthy color"},
        {"suffix": "bronze_top",  "prompt": "stone fountain with bronze top sculpture, gray basin"},
    ],
    "statue_pedestal": [
        {"suffix": "gray_stone",  "prompt": "ornate gray stone pedestal for statue, square base, carved details"},
        {"suffix": "white_marble","prompt": "white marble pedestal with carved details, polished"},
        {"suffix": "black_obsidian","prompt": "black obsidian pedestal, glossy dark stone, sinister"},
        {"suffix": "tan_sandstone","prompt": "tan sandstone pedestal, warm earthy color"},
    ],

    # ============================================================================
    # CENTERPIECE (3)
    # ============================================================================
    "fountain_stone": [
        {"suffix": "gray_stone",  "prompt": "ornate medieval gray stone fountain, circular basin with central pillar"},
        {"suffix": "white_marble","prompt": "ornate white marble fountain, polished, classical design"},
        {"suffix": "bronze_central","prompt": "stone fountain with bronze central sculpture, ornate"},
        {"suffix": "tan_sandstone","prompt": "warm tan sandstone fountain, classical design"},
    ],
    "statue_warrior": [
        {"suffix": "gray_stone",  "prompt": "stone statue of armored warrior with sword, gray weathered stone"},
        {"suffix": "white_marble","prompt": "white marble statue of armored warrior with sword, polished"},
        {"suffix": "bronze",      "prompt": "bronze statue of armored warrior with sword, golden patina"},
        {"suffix": "moss_aged",   "prompt": "moss-covered ancient statue of warrior, weathered and aged"},
    ],
    # ============================================================================
    # DUNGEON (17)
    # ============================================================================
    "pillar_broken": [
        {"suffix": "gray_stone",  "prompt": "broken classical fluted stone pillar, gray weathered stone, gothic"},
        {"suffix": "white_marble","prompt": "broken classical pillar, polished white marble with veining"},
        {"suffix": "tan_sandstone","prompt": "broken classical pillar, warm tan sandstone, ancient"},
        {"suffix": "bronze_inlaid","prompt": "broken stone pillar with bronze inlaid details, ornate"},
    ],
    "pillar_whole": [
        {"suffix": "gray_stone",  "prompt": "whole classical fluted stone pillar, gray stone, intact"},
        {"suffix": "white_marble","prompt": "whole classical pillar, polished white marble"},
        {"suffix": "tan_sandstone","prompt": "whole classical pillar, warm tan sandstone"},
        {"suffix": "black_obsidian","prompt": "whole classical pillar, black obsidian volcanic stone"},
    ],
    "pillar_fluted": [
        {"suffix": "gray_stone",  "prompt": "fluted classical column with capital, gray weathered stone"},
        {"suffix": "white_marble","prompt": "fluted classical column with capital, polished white marble"},
        {"suffix": "tan_sandstone","prompt": "fluted column with capital, warm tan sandstone"},
        {"suffix": "gold_capital","prompt": "stone column with gold-leaf capital, ornate"},
    ],
    "pillar_carved": [
        {"suffix": "gray_stone",  "prompt": "stone pillar with carved spiral patterns, gray stone, gothic"},
        {"suffix": "white_marble","prompt": "white marble pillar with carved spiral patterns, polished"},
        {"suffix": "black_obsidian","prompt": "black obsidian pillar with carved patterns, dark gothic"},
    ],
    "tomb_stone": [
        {"suffix": "gray_stone",  "prompt": "tall gray stone tombstone with rounded top, weathered, graveyard"},
        {"suffix": "white_marble","prompt": "white marble tombstone with rounded top, polished"},
        {"suffix": "moss_covered","prompt": "moss-covered tombstone, ancient and weathered"},
        {"suffix": "black_marble","prompt": "black marble tombstone, polished dark stone"},
    ],
    "tomb_cross": [
        {"suffix": "gray_stone",  "prompt": "stone cross grave marker, gray weathered, simple cross shape"},
        {"suffix": "white_marble","prompt": "white marble stone cross grave marker, polished"},
        {"suffix": "moss_covered","prompt": "moss-covered stone cross grave marker, ancient"},
    ],
    "coffin_wooden": [
        {"suffix": "dark_walnut", "prompt": "simple wooden coffin, dark walnut wood, rectangular"},
        {"suffix": "natural_pine","prompt": "simple wooden coffin, natural pine wood"},
        {"suffix": "black_lacquer","prompt": "wooden coffin with black lacquered finish, ornate"},
        {"suffix": "weathered",   "prompt": "weathered wooden coffin, gray aged wood"},
    ],
    "coffin_stone": [
        {"suffix": "gray_stone",  "prompt": "stone coffin with carved lid, gray weathered stone, ancient"},
        {"suffix": "white_marble","prompt": "white marble stone coffin with carved lid, polished"},
        {"suffix": "moss_covered","prompt": "moss-covered stone coffin, ancient sarcophagus"},
        {"suffix": "gold_inlaid", "prompt": "stone coffin with gold inlaid details, ornate"},
    ],
    "sarcophagus": [
        {"suffix": "gray_stone",  "prompt": "ancient stone sarcophagus, gray weathered stone with carved lid"},
        {"suffix": "white_marble","prompt": "ancient white marble sarcophagus with carved lid, polished"},
        {"suffix": "gold_egyptian","prompt": "Egyptian-style gold-inlaid stone sarcophagus, ornate"},
        {"suffix": "moss_aged",   "prompt": "moss-covered ancient sarcophagus, weathered and overgrown"},
    ],
    "torch_wall": [
        {"suffix": "iron_lit",    "prompt": "medieval wall torch on iron bracket, lit with orange flame"},
        {"suffix": "bronze_lit",  "prompt": "medieval wall torch with bronze bracket, lit flame"},
        {"suffix": "blue_magic",  "prompt": "magical wall torch with blue flame, iron bracket"},
        {"suffix": "extinguished","prompt": "unlit wall torch on iron bracket, cold dark torch"},
    ],
    "brazier_iron": [
        {"suffix": "black_iron",  "prompt": "black iron brazier on tripod legs, glowing orange coals"},
        {"suffix": "bronze",      "prompt": "bronze brazier on tripod legs, glowing coals"},
        {"suffix": "blue_magic",  "prompt": "iron brazier with blue magical fire, dark metal"},
        {"suffix": "extinguished","prompt": "unlit iron brazier, cold dark coals"},
    ],
    "chandelier_iron": [
        {"suffix": "black_iron",  "prompt": "medieval black iron chandelier with multiple lit candles"},
        {"suffix": "bronze",      "prompt": "bronze chandelier with multiple lit candles, ornate"},
        {"suffix": "gold_ornate", "prompt": "gold-trimmed chandelier with multiple lit candles, ornate"},
        {"suffix": "rusted",      "prompt": "rusted iron chandelier, brown rust, candles"},
    ],
    "crystal_blue": [
        {"suffix": "deep_blue",   "prompt": "tall deep blue crystal cluster, sharp pointed crystals, glowing"},
        {"suffix": "ice_white",   "prompt": "tall icy white-blue crystal cluster, sharp pointed"},
        {"suffix": "sapphire",    "prompt": "tall deep sapphire blue crystal cluster, gem-quality"},
    ],
    "crystal_purple": [
        {"suffix": "amethyst",    "prompt": "purple amethyst crystal cluster, geometric crystals"},
        {"suffix": "deep_violet", "prompt": "deep violet purple crystal cluster, dark sharp crystals"},
        {"suffix": "pink_crystal","prompt": "pink crystal cluster, rose quartz-like geometric"},
    ],
    "crystal_green": [
        {"suffix": "emerald",     "prompt": "green emerald crystal formation, sharp geode crystals"},
        {"suffix": "jade",        "prompt": "jade green crystal cluster, milky green stone"},
        {"suffix": "yellow_green","prompt": "yellow-green peridot crystal cluster, geometric"},
    ],
    "cobwebs_corner": [
        {"suffix": "white_dusty", "prompt": "thick dusty white cobwebs, abandoned spider webs"},
        {"suffix": "gray_old",    "prompt": "thick gray old cobwebs, very dusty abandoned"},
        {"suffix": "yellow_aged", "prompt": "yellowed aged cobwebs, very old abandoned spider webs"},
    ],
    "bone_pile": [
        {"suffix": "white_bleached","prompt": "pile of bleached white bones and skulls, ribs and femurs"},
        {"suffix": "yellowed_old",  "prompt": "pile of yellowed old bones, aged ossuary"},
        {"suffix": "charred_black", "prompt": "pile of charred black burnt bones, dark"},
    ],

    # ============================================================================
    # SPECIAL (5)
    # ============================================================================
    "anvil_blacksmith": [
        {"suffix": "black_iron",  "prompt": "iron blacksmith anvil on wooden block, classic horn shape, dark metal"},
        {"suffix": "rusted",      "prompt": "rusted blacksmith anvil, brown-orange rust, on wooden block"},
        {"suffix": "polished_steel","prompt": "polished steel blacksmith anvil on wooden block, bright metal"},
    ],
    "table_alchemy": [
        {"suffix": "dark_wood_glass","prompt": "alchemy table with bottles and tubes, dark walnut wood, glass laboratory"},
        {"suffix": "light_wood_brass","prompt": "alchemy table with bottles and tubes, light oak wood, brass instruments"},
        {"suffix": "white_marble", "prompt": "alchemy table with bottles, white marble top, ornate brass"},
    ],
    "bookshelf": [
        {"suffix": "dark_walnut", "prompt": "tall wooden bookshelf full of books, dark walnut wood, library"},
        {"suffix": "light_oak",   "prompt": "tall wooden bookshelf full of books, light oak wood"},
        {"suffix": "mahogany_red","prompt": "tall wooden bookshelf with books, deep mahogany red wood"},
        {"suffix": "white_painted","prompt": "tall white painted bookshelf with books, modern"},
    ],
    "kafra_pad": [
        {"suffix": "blue_glow",   "prompt": "magical teleport pad with blue glowing runes on stone base, fantasy"},
        {"suffix": "gold_glow",   "prompt": "magical teleport pad with gold glowing runes on stone base"},
        {"suffix": "purple_glow", "prompt": "magical teleport pad with purple glowing runes on stone base"},
        {"suffix": "green_glow",  "prompt": "magical teleport pad with green glowing runes on stone base"},
    ],
    "emperium_pedestal": [
        {"suffix": "gold_orb",    "prompt": "ornate stone pedestal with floating gold orb, war crystal stand, glowing"},
        {"suffix": "silver_orb",  "prompt": "ornate stone pedestal with floating silver orb, glowing"},
        {"suffix": "blue_orb",    "prompt": "ornate stone pedestal with floating blue magical orb, glowing"},
        {"suffix": "red_orb",     "prompt": "ornate stone pedestal with floating red magical orb, glowing"},
    ],
}


# ============================================================================
# UTILS
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


def find_asset_category(asset_id):
    """Walk generated_assets/ to find which category an asset_id is in."""
    for cat_dir in ROOT.iterdir():
        if cat_dir.is_dir() and not cat_dir.name.startswith("_"):
            if (cat_dir / asset_id).is_dir():
                return cat_dir.name
    return None


def asset_is_good(asset_id, category):
    """Read asset manifest, return True if asset passes quality thresholds.
    Skip variant generation for bad assets — wastes time on subpar geometry."""
    manifest_path = ROOT / category / asset_id / "manifest.json"
    if not manifest_path.exists():
        return False, "no_manifest"
    try:
        m = json.loads(manifest_path.read_text())
    except Exception:
        return False, "manifest_read_error"
    if m.get("status") != "ok":
        return False, "manifest_not_ok"
    sweep = m.get("stages", {}).get("mesh_sweep", {})
    d = sweep.get("depth_ratio", 0)
    v = sweep.get("volume", 0)
    score = d * v
    if score < 0.15:
        return False, f"low_score({score:.2f})"
    if v > 5.0:
        return False, f"cube_artifact(vol={v:.1f})"
    if v < 0.05:
        return False, f"thin_shell(vol={v:.2f})"
    if d < 0.4:
        return False, f"flat(depth={d:.2f})"
    return True, "good"


def wait_for_cleanup_done():
    """Block until cleanup orchestrator finishes (logs CLEANUP DONE)."""
    log("Waiting for cleanup orchestrator to finish...")
    cleanup_log = ROOT / "_cleanup.log"
    deadline = time.time() + 24 * 3600  # 24h safety
    while time.time() < deadline:
        try:
            if cleanup_log.exists():
                text = cleanup_log.read_text(encoding="utf-8", errors="replace")
                if "CLEANUP DONE" in text:
                    log("Cleanup finished — starting color variants")
                    return True
                # Show progress
                lines = [l for l in text.split("\n") if l.strip()]
                if lines:
                    last = lines[-1]
                    log(f"  ...cleanup running: {last[-100:]}")
        except Exception as e:
            log(f"  log read error: {e}")
        time.sleep(180)  # check every 3 min
    log("WARNING: 24h timeout waiting for cleanup")
    return False


def make_color_variant(asset_id, category, variant_def, rembg_session):
    """Generate one color variant. Returns path or None."""
    base_dir = ROOT / category / asset_id
    decim_path = base_dir / "04b_decimated_untextured.glb"
    if not decim_path.exists():
        return None, "no_untextured_mesh"

    suffix = variant_def["suffix"]
    variant_dir = base_dir / "variants" / suffix
    variant_dir.mkdir(parents=True, exist_ok=True)

    final_path = variant_dir / "06_final.glb"
    if final_path.exists():
        return final_path, "already_done"

    # Build SDXL prompt using the variant's color description
    positive = p.PERSPECTIVE_STRATEGIES[0]["build"](variant_def["prompt"])  # use rotated for variants
    negative = p.build_negative_prompt()

    # 1. Generate input
    pid = p.post_prompt(p.sdxl_workflow(positive, negative,
                                          f"colorvar_input_{asset_id}_{suffix}",
                                          seed=hash(suffix) % 100000))
    result, _ = p.wait_for(pid, "sdxl", timeout=180)
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
    cf_filename = f"colorvar_input_{asset_id}_{suffix}.png"
    p.upload_image(masked_path, cf_filename)

    # 3. Texture the existing decimated mesh
    prefix = f"colorvar_textured_{asset_id}_{suffix}"
    pid = p.post_prompt(p.hunyuan_texture_workflow(decim_path, cf_filename, prefix))
    _, _ = p.wait_for(pid, "texture", timeout=600)

    # Find output (in temp/ or output/)
    candidates = []
    for root_search in [Path("C:/ComfyUI/temp"), Path("C:/ComfyUI/output")]:
        candidates.extend(root_search.glob(f"{prefix}*.glb"))
    if not candidates:
        return None, "no_glb_produced"
    src = max(candidates, key=lambda x: x.stat().st_mtime)
    shutil.copy(src, final_path)
    return final_path, "ok"


def main():
    log("=" * 78)
    log("COLOR VARIANT GENERATOR")
    total_planned = sum(len(v) for v in COLOR_VARIANTS.values())
    log(f"  Assets defined:  {len(COLOR_VARIANTS)}")
    log(f"  Total variants planned: {total_planned}")
    log(f"  Estimated time: {total_planned * 90 / 60:.0f} min ({total_planned * 90 / 3600:.1f} hours)")
    log("=" * 78)

    # Wait for cleanup
    wait_for_cleanup_done()

    # Verify ComfyUI
    try:
        urllib.request.urlopen(f"{COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log(f"FATAL: ComfyUI unreachable: {e}")
        return 1

    log("Loading rembg...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    overall_start = time.time()
    done = 0
    skipped = 0
    failed = 0
    no_mesh = 0
    skipped_bad = 0

    # Iterate all asset variants
    asset_idx = 0
    for asset_id, variants in COLOR_VARIANTS.items():
        asset_idx += 1
        category = find_asset_category(asset_id)
        if category is None:
            log(f"  asset {asset_id} not found in any category, skip")
            no_mesh += len(variants)
            continue

        # Skip bad assets — don't waste time on subpar geometry
        is_good, why = asset_is_good(asset_id, category)
        if not is_good:
            log(f"[{asset_idx}/{len(COLOR_VARIANTS)}] {category}/{asset_id} — SKIP "
                f"({len(variants)} variants) — base asset is BAD ({why})")
            skipped_bad += len(variants)
            continue

        log("")
        log("=" * 78)
        log(f"[{asset_idx}/{len(COLOR_VARIANTS)}] {category}/{asset_id} — {len(variants)} variants")
        log("=" * 78)

        for vi, variant in enumerate(variants, 1):
            log(f"  variant {vi}/{len(variants)}: {variant['suffix']}")
            try:
                result, status = make_color_variant(asset_id, category, variant, rembg_session)
                if status == "ok":
                    done += 1
                    log(f"    OK -> {result.relative_to(ROOT)}")
                elif status == "already_done":
                    skipped += 1
                    log(f"    SKIP (already exists)")
                elif status == "no_untextured_mesh":
                    no_mesh += 1
                    log(f"    SKIP (no untextured mesh — asset never finished)")
                else:
                    failed += 1
                    log(f"    FAILED: {status}")
            except Exception as e:
                failed += 1
                log(f"    EXCEPTION: {e}")

        # Periodic ETA
        elapsed = time.time() - overall_start
        progress_total = done + skipped + failed + no_mesh
        if progress_total > 0:
            avg = elapsed / progress_total
            remaining = total_planned - progress_total
            eta_min = avg * remaining / 60
            log(f"  [progress: {progress_total}/{total_planned} | ok={done} skip={skipped} "
                f"fail={failed} no_mesh={no_mesh} | ETA: {eta_min:.0f} min]")

    total_min = (time.time() - overall_start) / 60
    log("")
    log("=" * 78)
    log(f"COLOR VARIANTS DONE — {total_min:.1f} min")
    log(f"  ok={done}, skipped_existing={skipped}, failed={failed}, no_mesh={no_mesh}, skipped_bad_asset={skipped_bad}")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
