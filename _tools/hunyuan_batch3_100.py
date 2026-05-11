"""
Batch 3 — 100 new assets filling gaps in the asset library.

Audited against the user's checklist (2026-05-03):
  GAPS IDENTIFIED:
    - Castle/keep modules (only 1 tower existed)
    - Arches (separate from pillars) — totally missing
    - More flowers (only 3 types)
    - Desert grass / Sograt vegetation
    - Snow/ice features
    - Volcanic features
    - Debris piles (only bone_pile)
    - Water features (lily pads, boats)
    - NPC/utility props (cauldron, forge, loom)
    - Urban detail (doors, shutters, planters)
    - More statues (only warrior)
    - More dungeon variety (gates, chests, altars)
    - Architectural detail (window shutters, chimney)

This script:
  1. Waits for current color_variants run to finish
  2. Runs 100 new asset generations using process_asset_multi_input + validation
  3. Iterative cleanup pass
  4. Generates 4 color variants per good asset (~400 variants total)

Uses ALL the new techniques:
  - Multi-input perspective (rotated + 3quarter, 2 seeds each)
  - Input validation (single-object + frame-contained)
  - Decimate-first then texture
  - Quality-aware scoring (cube_artifact = 0.001)
  - Backup-restore on regression
  - Retry-on-fail for SDXL inputs
  - Skip variants for bad assets

Run: C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/hunyuan_batch3_100.py
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
import hunyuan_cleanup as cu
import hunyuan_color_variants as cv

ROOT = p.ROOT
COMFY_URL = p.COMFY_URL
LOG_FILE = ROOT / "_batch3.log"


# ============================================================================
# 100 NEW ASSETS — gap-filling, organized by category
# ============================================================================
BATCH3_ASSETS = [
    # ---- CASTLE / KEEP MODULES (10) — major gap, only had tower_geffen ----
    {"id": "castle_wall_segment",   "category": "architecture", "subject": "modular medieval castle wall section, gray stone with crenellations on top, walkway between battlements, fortified rampart"},
    {"id": "castle_battlement_corner","category": "architecture","subject": "medieval castle corner battlement with crenellations, gray stone, defensive corner tower joining two walls"},
    {"id": "castle_gatehouse",      "category": "architecture", "subject": "medieval castle gatehouse with two flanking towers, central arched gateway with portcullis, gray stone fortification"},
    {"id": "castle_keep",           "category": "architecture", "subject": "medieval castle keep tower, large square stone donjon with battlements on top, narrow windows, gothic fortress"},
    {"id": "castle_drawbridge",     "category": "architecture", "subject": "medieval wooden drawbridge with iron chains, raised position, dark wood planks, castle entrance"},
    {"id": "castle_portcullis",     "category": "architecture", "subject": "medieval iron portcullis gate, vertical metal grid lattice raised in stone arch, dark wrought iron"},
    {"id": "castle_round_tower",    "category": "architecture", "subject": "medieval round castle tower, cylindrical gray stone with conical roof, narrow arrow slits, defensive tower"},
    {"id": "castle_throne",         "category": "architecture", "subject": "ornate medieval royal throne, large stone or wooden throne with high carved back, gothic style"},
    {"id": "siege_catapult",        "category": "architecture", "subject": "medieval wooden catapult siege weapon, throwing arm with bucket, wheels and frame"},
    {"id": "siege_battering_ram",   "category": "architecture", "subject": "medieval battering ram with wooden frame and metal-tipped log, suspended on chains, siege weapon"},

    # ---- ARCHES (5) — totally missing category ----
    {"id": "arch_stone_simple",     "category": "architecture", "subject": "simple stone archway, single arch span between two columns, gray weathered stone, classical"},
    {"id": "arch_gothic_dungeon",   "category": "dungeon",      "subject": "gothic dungeon stone archway, pointed arch top, weathered gray stone with carved details"},
    {"id": "arch_ruined",           "category": "architecture", "subject": "ruined stone archway, half-collapsed with broken keystone, ancient ruins, weathered"},
    {"id": "arch_triumphal",        "category": "architecture", "subject": "ornate triumphal arch, large stone arch with carved reliefs, classical Roman style"},
    {"id": "arch_doorway",          "category": "architecture", "subject": "stone doorway arch with wooden door, medieval entrance, gray stone frame"},

    # ---- FLOWERS (8) — gap, only had 3 ----
    {"id": "flower_white_daisies",  "category": "vegetation",   "subject": "small cluster of white daisy flowers with yellow centers on green grass tuft, ground patch"},
    {"id": "flower_pink_roses",     "category": "vegetation",   "subject": "small bush of pink rose flowers with green leaves, garden plant"},
    {"id": "flower_sunflowers",     "category": "vegetation",   "subject": "tall yellow sunflower with brown center on green stem with leaves, single flower or small cluster"},
    {"id": "flower_lavender",       "category": "vegetation",   "subject": "cluster of purple lavender stalks with green leaves, herb plant, bushy"},
    {"id": "flower_bluebells",      "category": "vegetation",   "subject": "small cluster of blue bell-shaped wildflowers on green grass tuft"},
    {"id": "flower_marigolds",      "category": "vegetation",   "subject": "small cluster of orange and yellow marigold flowers on green stems, garden flowers"},
    {"id": "flower_lily_pads",      "category": "vegetation",   "subject": "round green lily pads with one white water lily flower, pond plant"},
    {"id": "flower_red_poppies",    "category": "vegetation",   "subject": "cluster of red poppy flowers with green stems, wildflowers"},

    # ---- DESERT VEGETATION (3) — gap ----
    {"id": "desert_grass_tuft",     "category": "vegetation",   "subject": "tuft of dry yellow desert grass, bunch of pale beige grass blades, arid plant"},
    {"id": "tumbleweed",            "category": "vegetation",   "subject": "round dry tumbleweed plant, brown tangled dead grass ball, desert"},
    {"id": "desert_yucca",          "category": "vegetation",   "subject": "yucca plant with spiky green sword-shaped leaves, desert plant, single specimen"},

    # ---- SNOW / ICE (4) — gap ----
    {"id": "snow_drift",            "category": "terrain",      "subject": "small mound of white snow, snow drift pile with subtle blue shadows, winter terrain"},
    {"id": "ice_crystal_large",     "category": "terrain",      "subject": "large clear ice crystal cluster, sharp angular ice formations, glowing pale blue"},
    {"id": "icicle_cluster",        "category": "terrain",      "subject": "cluster of hanging icicles, sharp pointed ice spikes, transparent pale blue"},
    {"id": "snow_stump",            "category": "vegetation",   "subject": "tree stump covered in snow, weathered brown wood with white snow cap, winter forest"},

    # ---- VOLCANIC (4) — gap ----
    {"id": "volcanic_rock",         "category": "terrain",      "subject": "dark volcanic rock with red glowing cracks, black porous lava stone, hot"},
    {"id": "lava_crystal",          "category": "terrain",      "subject": "fiery red orange crystal cluster glowing with heat, sharp angular volcanic gem"},
    {"id": "ash_pile",              "category": "terrain",      "subject": "pile of gray volcanic ash, mound of fine charcoal-gray powder"},
    {"id": "charred_log",           "category": "terrain",      "subject": "burnt wooden log, blackened charred wood with red glowing embers, fire damage"},

    # ---- DEBRIS / RUBBLE (5) — gap, only had bone_pile ----
    {"id": "rubble_stone",          "category": "dungeon",      "subject": "pile of broken stone rubble, mixed sized gray rocks and chunks, ruins debris"},
    {"id": "rubble_wood",           "category": "props",        "subject": "pile of broken wooden boards and planks, splintered debris pile"},
    {"id": "broken_weapons_pile",   "category": "props",        "subject": "pile of broken weapons, rusty swords and shield fragments, abandoned battlefield"},
    {"id": "pottery_shards",        "category": "props",        "subject": "pile of broken terracotta pottery shards, orange-brown ceramic fragments"},
    {"id": "broken_cart",           "category": "props",        "subject": "broken abandoned wooden cart with cracked wheel, weathered, half collapsed"},

    # ---- WATER FEATURES (5) — gap ----
    {"id": "pond_rocks",            "category": "terrain",      "subject": "cluster of smooth round wet pond rocks, gray stones partially submerged"},
    {"id": "small_waterfall",       "category": "terrain",      "subject": "small waterfall rock formation with cascading water, gray stones with white water flow"},
    {"id": "canoe_wooden",          "category": "props",        "subject": "small wooden canoe boat, narrow vessel with paddle, natural light wood"},
    {"id": "rowboat_wooden",        "category": "props",        "subject": "small wooden rowboat with two oars, weathered planks, fishing boat"},
    {"id": "fishing_net",           "category": "props",        "subject": "wooden frame with hanging fishing net, drying rack with rope mesh"},

    # ---- NPC / UTILITY PROPS (10) — gap ----
    {"id": "cauldron_iron",         "category": "special",      "subject": "large iron cauldron on three legs with wooden ladle, dark cast iron pot, witch cooking pot"},
    {"id": "apothecary_cabinet",    "category": "special",      "subject": "tall wooden apothecary cabinet with many small drawers and bottles on shelves"},
    {"id": "forge_furnace",         "category": "special",      "subject": "stone blacksmith forge with glowing red coals, bellows attached, anvil ready, smithing furnace"},
    {"id": "spinning_wheel",        "category": "special",      "subject": "wooden spinning wheel with treadle, large wheel and spindle, classic Rumpelstiltskin style"},
    {"id": "weaving_loom",          "category": "special",      "subject": "wooden weaving loom with vertical threads and shuttle, traditional textile craft"},
    {"id": "millstone",             "category": "special",      "subject": "large round stone millstone for grinding grain, gray stone wheel"},
    {"id": "water_trough",          "category": "props",        "subject": "long wooden water trough on stone supports, animal watering basin"},
    {"id": "well_pump",             "category": "props",        "subject": "iron hand-pump water well with wooden bucket, vintage water pump"},
    {"id": "outdoor_oven",          "category": "props",        "subject": "stone outdoor bread oven with arched opening, dome-shaped pizza oven, brick"},
    {"id": "potter_wheel",          "category": "special",      "subject": "wooden potter's wheel with clay vessel forming on top, ceramics craft"},

    # ---- URBAN DETAIL (8) ----
    {"id": "door_wooden",           "category": "props",        "subject": "wooden door panel with iron hinges, single door in medieval style, dark wood"},
    {"id": "window_shutters",       "category": "props",        "subject": "wooden window shutters with iron latches, both panels, weathered light wood"},
    {"id": "chimney_brick",         "category": "props",        "subject": "tall red brick chimney, rectangular brick stack with cap, building accessory"},
    {"id": "birdhouse",             "category": "props",        "subject": "small wooden birdhouse with pitched roof and round entrance, on tall pole"},
    {"id": "flower_pot",            "category": "props",        "subject": "terracotta clay flower pot with red flowers blooming inside, garden planter"},
    {"id": "planter_box",           "category": "props",        "subject": "wooden window planter box with green herbs growing inside, kitchen garden"},
    {"id": "lantern_floor",         "category": "props",        "subject": "tall floor-standing iron lantern with glass panels, candlestick lantern on stand"},
    {"id": "pottery_jar",           "category": "props",        "subject": "tall ceramic terracotta storage jar with handles, classic amphora shape"},

    # ---- ADDITIONAL DUNGEON (10) ----
    {"id": "iron_gate",             "category": "dungeon",      "subject": "medieval iron gate with vertical metal bars, dark wrought iron, dungeon entrance"},
    {"id": "dungeon_door",          "category": "dungeon",      "subject": "heavy wooden dungeon door with iron studs and reinforcements, gothic, dark wood"},
    {"id": "skull_pile",            "category": "dungeon",      "subject": "pile of human skulls, bleached white bone heap, ossuary"},
    {"id": "treasure_chest",        "category": "dungeon",      "subject": "ornate wooden treasure chest with iron bands and large lock, dark wood, gold trim"},
    {"id": "old_chest",             "category": "dungeon",      "subject": "weathered old wooden chest with rusty iron lock, simple wooden box, peasant"},
    {"id": "magic_circle",          "category": "dungeon",      "subject": "glowing magic circle on stone floor, blue runes in circular pattern, mystical seal"},
    {"id": "altar_stone",           "category": "dungeon",      "subject": "ornate stone altar with carved details, sacrificial slab, gothic religious"},
    {"id": "ritual_pentagram",      "category": "dungeon",      "subject": "stone pentagram circle with red glowing runes, occult ritual ground marker"},
    {"id": "bone_throne",           "category": "dungeon",      "subject": "throne made of skulls and bones, gothic horror, dark fantasy, sinister seat"},
    {"id": "spider_egg_sack",       "category": "dungeon",      "subject": "large hanging spider egg sack, white silk cocoon with dark spots, dungeon hazard"},

    # ---- MORE STATUES (4) ----
    {"id": "statue_lion",           "category": "centerpiece",  "subject": "stone lion statue on pedestal, sitting majestic lion, gray weathered stone"},
    {"id": "statue_dragon",         "category": "centerpiece",  "subject": "stone dragon statue with curled body and spread wings, weathered gray stone"},
    {"id": "statue_angel",          "category": "centerpiece",  "subject": "white marble angel statue with spread wings, classical religious sculpture"},
    {"id": "statue_obelisk_carved", "category": "centerpiece",  "subject": "stone obelisk with carved hieroglyphic patterns, ancient Egyptian style"},

    # ---- MORE BUSHES / STUMPS / ROOTS (5) ----
    {"id": "vine_hanging",          "category": "vegetation",   "subject": "hanging green vine with leaves and small flowers, draping plant"},
    {"id": "moss_patch",            "category": "vegetation",   "subject": "patch of green moss on rocks and ground, lush mossy growth"},
    {"id": "fallen_log",            "category": "vegetation",   "subject": "fallen tree log on ground with moss covering, weathered brown wood"},
    {"id": "tree_stump",            "category": "vegetation",   "subject": "tree stump on ground, weathered brown wood with visible rings, cut tree base"},
    {"id": "tree_root_exposed",     "category": "vegetation",   "subject": "exposed tree roots above ground, gnarled brown wooden roots, ancient tree base"},

    # ---- MORE PROPS / UTILITY (5) ----
    {"id": "ladder_wooden",         "category": "props",        "subject": "tall wooden ladder leaning against support, simple rungs, weathered wood"},
    {"id": "wagon_covered",         "category": "props",        "subject": "covered wagon with white canvas top and wooden wheels, traveling merchant cart"},
    {"id": "hand_cart_wooden",      "category": "props",        "subject": "small two-wheeled wooden hand cart with handles, peasant push cart"},
    {"id": "barrel_open_water",     "category": "props",        "subject": "open wooden barrel filled with water, dark wood with iron bands"},
    {"id": "pulley_system",         "category": "props",        "subject": "wooden pulley with rope and hook, hanging from beam, mechanical lifting"},

    # ---- LIGHTING (4) ----
    {"id": "lantern_oil",           "category": "props",        "subject": "small handheld oil lantern with glass panels and metal frame, lit"},
    {"id": "candelabra",            "category": "props",        "subject": "ornate brass candelabra with three lit candles, formal dining room"},
    {"id": "torch_floor_iron",      "category": "props",        "subject": "tall iron floor torch stand with flaming pitch top, medieval lighting"},
    {"id": "wagon_wheel_chandelier","category": "props",        "subject": "wooden wagon wheel chandelier with multiple candles, rustic ceiling fixture"},

    # ---- MORE TERRAIN (5) ----
    {"id": "ledge_overhang",        "category": "terrain",      "subject": "stone ledge overhang formation, jutting rock shelf with cracks, mountain feature"},
    {"id": "waterfall_rock",        "category": "terrain",      "subject": "tall vertical rock cliff with water cascading down, single waterfall"},
    {"id": "mountain_peak",         "category": "terrain",      "subject": "tall sharp mountain peak with snow cap, gray rocky pinnacle"},
    {"id": "crater",                "category": "terrain",      "subject": "circular crater with rocky rim, depression in ground, impact site"},
    {"id": "stone_mound",           "category": "terrain",      "subject": "rounded mound covered in stones and pebbles, small natural hill"},

    # ---- ADDITIONAL ARCHITECTURE (4) ----
    {"id": "watchtower_wooden",     "category": "architecture", "subject": "tall wooden watchtower with platform on top, sentry post, frontier outpost"},
    {"id": "windmill",              "category": "architecture", "subject": "tall stone windmill with wooden sails, classical Dutch style, single windmill"},
    {"id": "lighthouse",            "category": "architecture", "subject": "tall white stone lighthouse with red top and lantern room, coastal beacon"},
    {"id": "barn_wooden",           "category": "architecture", "subject": "red wooden barn with sloped roof and large doors, farmhouse, country building"},
    {"id": "stone_bench",           "category": "props",        "subject": "ornate stone garden bench with carved scrollwork, gray weathered stone, decorative seat"},
]

# Verify count
assert len(BATCH3_ASSETS) == 100, f"Expected 100 assets, got {len(BATCH3_ASSETS)}"


# ============================================================================
# COLOR VARIANTS for batch3 — 4 variants per asset for ~400 total
# ============================================================================
# Reusable palettes (assigned to multiple assets)
STONE_PALETTE = [
    {"suffix": "gray_granite",  "prompt": "weathered gray granite stone"},
    {"suffix": "white_marble",  "prompt": "polished white marble with subtle veining"},
    {"suffix": "sandstone_tan", "prompt": "warm tan sandstone, earthy color"},
    {"suffix": "mossy",         "prompt": "stone covered in green moss patches"},
]
WOOD_PALETTE = [
    {"suffix": "light_oak",   "prompt": "natural golden tan light oak wood"},
    {"suffix": "dark_walnut", "prompt": "rich dark brown walnut wood"},
    {"suffix": "weathered",   "prompt": "weathered gray driftwood, sun-bleached"},
    {"suffix": "red_painted", "prompt": "painted red wood with worn chipped paint"},
]
IRON_PALETTE = [
    {"suffix": "black_iron",   "prompt": "black wrought iron metal"},
    {"suffix": "bronze",       "prompt": "polished warm bronze metal"},
    {"suffix": "copper_patina","prompt": "copper metal with green-blue verdigris patina"},
    {"suffix": "rusted",       "prompt": "heavily rusted iron, brown-orange patina"},
]
FLOWER_PALETTE_DEFAULT = [
    {"suffix": "bright",      "prompt": "bright vibrant colors"},
    {"suffix": "pastel",      "prompt": "soft pastel colors"},
    {"suffix": "deep",        "prompt": "deep saturated colors"},
    {"suffix": "wild",        "prompt": "mixed wild colors"},
]

BATCH3_VARIANTS = {
    # CASTLE (10)
    "castle_wall_segment":   STONE_PALETTE,
    "castle_battlement_corner": STONE_PALETTE,
    "castle_gatehouse":      STONE_PALETTE,
    "castle_keep":           STONE_PALETTE,
    "castle_drawbridge":     [
        {"suffix": "natural_wood", "prompt": "natural medium brown wooden drawbridge"},
        {"suffix": "dark_oak",     "prompt": "dark stained oak drawbridge"},
        {"suffix": "weathered",    "prompt": "weathered gray drawbridge, aged wood"},
    ],
    "castle_portcullis":     IRON_PALETTE,
    "castle_round_tower":    STONE_PALETTE,
    "castle_throne":         [
        {"suffix": "stone",        "prompt": "gray stone throne"},
        {"suffix": "gold_inlaid",  "prompt": "gold-inlaid throne, ornate"},
        {"suffix": "dark_wood",    "prompt": "dark walnut wood throne"},
        {"suffix": "white_marble", "prompt": "white marble throne"},
    ],
    "siege_catapult":        WOOD_PALETTE,
    "siege_battering_ram":   WOOD_PALETTE,

    # ARCHES (5)
    "arch_stone_simple":     STONE_PALETTE,
    "arch_gothic_dungeon":   STONE_PALETTE,
    "arch_ruined":           STONE_PALETTE,
    "arch_triumphal":        STONE_PALETTE,
    "arch_doorway":          STONE_PALETTE,

    # FLOWERS (8) — color-specific
    "flower_white_daisies":  [
        {"suffix": "white",       "prompt": "white daisy flowers with yellow centers"},
        {"suffix": "pink",        "prompt": "pink daisy flowers with yellow centers"},
        {"suffix": "yellow",      "prompt": "yellow daisy flowers"},
        {"suffix": "purple",      "prompt": "purple daisy flowers"},
    ],
    "flower_pink_roses":     [
        {"suffix": "pink",        "prompt": "pink rose flowers"},
        {"suffix": "red",         "prompt": "deep red rose flowers"},
        {"suffix": "white",       "prompt": "white rose flowers"},
        {"suffix": "yellow",      "prompt": "yellow rose flowers"},
    ],
    "flower_sunflowers":     [
        {"suffix": "yellow",      "prompt": "bright yellow sunflower"},
        {"suffix": "orange",      "prompt": "orange sunflower variant"},
        {"suffix": "red",         "prompt": "red sunflower variant"},
    ],
    "flower_lavender":       [
        {"suffix": "purple",      "prompt": "purple lavender stalks"},
        {"suffix": "white",       "prompt": "white lavender stalks"},
        {"suffix": "pink",        "prompt": "pink lavender stalks"},
    ],
    "flower_bluebells":      [
        {"suffix": "blue",        "prompt": "blue bellflower cluster"},
        {"suffix": "white",       "prompt": "white bellflower cluster"},
        {"suffix": "purple",      "prompt": "purple bellflower cluster"},
    ],
    "flower_marigolds":      [
        {"suffix": "orange",      "prompt": "orange marigold flowers"},
        {"suffix": "yellow",      "prompt": "yellow marigold flowers"},
        {"suffix": "red",         "prompt": "red marigold flowers"},
    ],
    "flower_lily_pads":      [
        {"suffix": "white",       "prompt": "white water lily on green pads"},
        {"suffix": "pink",        "prompt": "pink water lily on green pads"},
        {"suffix": "yellow",      "prompt": "yellow water lily on green pads"},
    ],
    "flower_red_poppies":    [
        {"suffix": "red",         "prompt": "red poppy flowers"},
        {"suffix": "orange",      "prompt": "orange poppy flowers"},
        {"suffix": "white",       "prompt": "white poppy flowers"},
    ],

    # DESERT VEGETATION (3)
    "desert_grass_tuft":     [
        {"suffix": "dry_yellow",  "prompt": "dry yellow desert grass"},
        {"suffix": "green",       "prompt": "fresh green desert grass"},
        {"suffix": "red_dry",     "prompt": "red-tinged dry grass"},
    ],
    "tumbleweed":            [
        {"suffix": "brown_dry",   "prompt": "brown dry tumbleweed"},
        {"suffix": "gray_aged",   "prompt": "gray aged tumbleweed"},
        {"suffix": "burnt",       "prompt": "burnt black tumbleweed"},
    ],
    "desert_yucca":          [
        {"suffix": "green",       "prompt": "green spiky yucca plant"},
        {"suffix": "blue_green",  "prompt": "blue-green agave variant"},
        {"suffix": "red_tinged",  "prompt": "red-tinged yucca, desert sun"},
    ],

    # SNOW / ICE (4)
    "snow_drift":            [
        {"suffix": "white",       "prompt": "pure white snow drift"},
        {"suffix": "dirty",       "prompt": "dirty gray snow drift"},
        {"suffix": "blue_shadow", "prompt": "snow drift with deep blue shadows"},
    ],
    "ice_crystal_large":     [
        {"suffix": "pale_blue",   "prompt": "pale blue ice crystals"},
        {"suffix": "clear",       "prompt": "clear transparent ice crystals"},
        {"suffix": "magical_glow","prompt": "glowing cyan ice crystals, magical"},
    ],
    "icicle_cluster":        [
        {"suffix": "clear",       "prompt": "clear transparent icicles"},
        {"suffix": "pale_blue",   "prompt": "pale blue icicles"},
        {"suffix": "frosted",     "prompt": "frosted white-blue icicles"},
    ],
    "snow_stump":            [
        {"suffix": "fresh_snow",  "prompt": "fresh white snow on tree stump"},
        {"suffix": "dusted",      "prompt": "lightly snow-dusted tree stump"},
        {"suffix": "frozen",      "prompt": "frozen ice-covered tree stump"},
    ],

    # VOLCANIC (4)
    "volcanic_rock":         [
        {"suffix": "glowing_red", "prompt": "black volcanic rock with red glowing cracks"},
        {"suffix": "cooled_gray", "prompt": "cooled gray volcanic rock"},
        {"suffix": "obsidian",    "prompt": "glossy black obsidian rock"},
    ],
    "lava_crystal":          [
        {"suffix": "red_orange",  "prompt": "red-orange lava crystal"},
        {"suffix": "deep_red",    "prompt": "deep red lava crystal"},
        {"suffix": "yellow_white","prompt": "white-hot yellow lava crystal"},
    ],
    "ash_pile":              [
        {"suffix": "gray",        "prompt": "gray volcanic ash pile"},
        {"suffix": "black_white", "prompt": "black and white ash mixed"},
        {"suffix": "red_embers",  "prompt": "ash pile with red glowing embers"},
    ],
    "charred_log":           [
        {"suffix": "burnt_black", "prompt": "fully burnt black charred log"},
        {"suffix": "glowing",     "prompt": "charred log with red glowing embers"},
        {"suffix": "weathered",   "prompt": "weathered burnt log, gray-black"},
    ],

    # DEBRIS (5)
    "rubble_stone":          STONE_PALETTE,
    "rubble_wood":           WOOD_PALETTE[:3],
    "broken_weapons_pile":   IRON_PALETTE[:3],
    "pottery_shards":        [
        {"suffix": "terracotta",  "prompt": "terracotta orange-brown pottery shards"},
        {"suffix": "blue_glazed", "prompt": "blue glazed pottery shards"},
        {"suffix": "white_porcelain","prompt": "white porcelain shards"},
    ],
    "broken_cart":           WOOD_PALETTE[:3],

    # WATER FEATURES (5)
    "pond_rocks":            STONE_PALETTE,
    "small_waterfall":       STONE_PALETTE,
    "canoe_wooden":          WOOD_PALETTE,
    "rowboat_wooden":        WOOD_PALETTE,
    "fishing_net":           [
        {"suffix": "natural",     "prompt": "natural light tan rope fishing net"},
        {"suffix": "weathered",   "prompt": "weathered gray fishing net"},
        {"suffix": "dark_tar",    "prompt": "dark tar-stained fishing net"},
    ],

    # NPC PROPS (10)
    "cauldron_iron":         IRON_PALETTE,
    "apothecary_cabinet":    WOOD_PALETTE,
    "forge_furnace":         [
        {"suffix": "stone",       "prompt": "gray stone forge furnace with red coals"},
        {"suffix": "brick",       "prompt": "red brick forge furnace"},
        {"suffix": "iron_lined",  "prompt": "iron-lined dark forge furnace"},
    ],
    "spinning_wheel":        WOOD_PALETTE,
    "weaving_loom":          WOOD_PALETTE,
    "millstone":             [
        {"suffix": "gray_granite","prompt": "gray granite millstone"},
        {"suffix": "white_stone", "prompt": "white limestone millstone"},
        {"suffix": "weathered",   "prompt": "weathered worn millstone"},
    ],
    "water_trough":          WOOD_PALETTE,
    "well_pump":             IRON_PALETTE,
    "outdoor_oven":          [
        {"suffix": "stone",       "prompt": "gray stone outdoor oven"},
        {"suffix": "brick",       "prompt": "red brick outdoor oven"},
        {"suffix": "white_plaster","prompt": "white plastered outdoor oven"},
    ],
    "potter_wheel":          WOOD_PALETTE,

    # URBAN DETAIL (8)
    "door_wooden":           WOOD_PALETTE,
    "window_shutters":       WOOD_PALETTE,
    "chimney_brick":         [
        {"suffix": "red_brick",   "prompt": "red brick chimney"},
        {"suffix": "gray_brick",  "prompt": "gray brick chimney"},
        {"suffix": "weathered",   "prompt": "weathered brick chimney"},
    ],
    "birdhouse":             WOOD_PALETTE,
    "flower_pot":            [
        {"suffix": "terracotta",  "prompt": "terracotta clay pot with red flowers"},
        {"suffix": "blue_glaze",  "prompt": "blue glazed pot with white flowers"},
        {"suffix": "wooden",      "prompt": "wooden barrel pot with mixed flowers"},
        {"suffix": "stone",       "prompt": "gray stone pot with herbs"},
    ],
    "planter_box":           WOOD_PALETTE,
    "lantern_floor":         IRON_PALETTE,
    "pottery_jar":           [
        {"suffix": "terracotta",  "prompt": "terracotta orange pottery jar"},
        {"suffix": "blue_glaze",  "prompt": "blue glazed ceramic jar"},
        {"suffix": "white",       "prompt": "white porcelain jar with patterns"},
    ],

    # DUNGEON (10)
    "iron_gate":             IRON_PALETTE,
    "dungeon_door":          WOOD_PALETTE,
    "skull_pile":            [
        {"suffix": "white_bleached","prompt": "bleached white skulls"},
        {"suffix": "yellowed",    "prompt": "yellowed aged skulls"},
        {"suffix": "charred",     "prompt": "charred black skulls"},
    ],
    "treasure_chest":        [
        {"suffix": "wood_gold",   "prompt": "dark wood chest with gold trim and lock"},
        {"suffix": "iron_bound",  "prompt": "iron-bound dark wood treasure chest"},
        {"suffix": "ornate_red",  "prompt": "ornate red wooden chest with gold details"},
        {"suffix": "weathered",   "prompt": "weathered old treasure chest, aged"},
    ],
    "old_chest":             WOOD_PALETTE,
    "magic_circle":          [
        {"suffix": "blue",        "prompt": "blue glowing magic circle"},
        {"suffix": "purple",      "prompt": "purple glowing magic circle"},
        {"suffix": "red",         "prompt": "red glowing magic circle, sinister"},
        {"suffix": "gold",        "prompt": "golden glowing magic circle, divine"},
    ],
    "altar_stone":           STONE_PALETTE,
    "ritual_pentagram":      [
        {"suffix": "red_blood",   "prompt": "blood-red pentagram circle"},
        {"suffix": "black_dark",  "prompt": "dark black pentagram, ominous"},
        {"suffix": "gold_holy",   "prompt": "gold pentagram, holy"},
    ],
    "bone_throne":           [
        {"suffix": "white_bone",  "prompt": "white bone throne"},
        {"suffix": "yellowed",    "prompt": "yellowed aged bone throne"},
        {"suffix": "charred",     "prompt": "charred black bone throne"},
    ],
    "spider_egg_sack":       [
        {"suffix": "white_silk",  "prompt": "white silk spider egg sack"},
        {"suffix": "gray_dusty",  "prompt": "gray dusty old egg sack"},
        {"suffix": "yellow_aged", "prompt": "yellowed aged egg sack"},
    ],

    # STATUES (4)
    "statue_lion":           STONE_PALETTE,
    "statue_dragon":         STONE_PALETTE,
    "statue_angel":          [
        {"suffix": "white_marble","prompt": "white marble angel statue"},
        {"suffix": "gray_stone",  "prompt": "gray weathered stone angel"},
        {"suffix": "moss_aged",   "prompt": "moss-covered ancient angel statue"},
        {"suffix": "gold_inlaid", "prompt": "gold-trimmed marble angel statue"},
    ],
    "statue_obelisk_carved": STONE_PALETTE,

    # MORE BUSHES (5)
    "vine_hanging":          [
        {"suffix": "green",       "prompt": "lush green hanging vine"},
        {"suffix": "yellow_autumn","prompt": "yellow autumn vine"},
        {"suffix": "purple_flowers","prompt": "vine with purple flowers"},
        {"suffix": "red_berries", "prompt": "vine with red berries"},
    ],
    "moss_patch":            [
        {"suffix": "fresh_green", "prompt": "fresh bright green moss"},
        {"suffix": "dark_green",  "prompt": "dark forest green moss"},
        {"suffix": "yellow_lichen","prompt": "yellow lichen patches"},
    ],
    "fallen_log":            [
        {"suffix": "mossy",       "prompt": "fallen log covered in green moss"},
        {"suffix": "weathered",   "prompt": "weathered gray fallen log"},
        {"suffix": "burnt",       "prompt": "burnt black fallen log"},
    ],
    "tree_stump":            [
        {"suffix": "fresh_cut",   "prompt": "freshly cut tree stump, light wood"},
        {"suffix": "weathered",   "prompt": "weathered gray tree stump"},
        {"suffix": "mossy",       "prompt": "mossy tree stump with mushrooms"},
    ],
    "tree_root_exposed":     [
        {"suffix": "natural",     "prompt": "natural brown exposed tree roots"},
        {"suffix": "dark",        "prompt": "dark gnarled exposed roots"},
        {"suffix": "mossy",       "prompt": "mossy exposed tree roots"},
    ],

    # MORE PROPS (5)
    "ladder_wooden":         WOOD_PALETTE,
    "wagon_covered":         [
        {"suffix": "white_canvas","prompt": "covered wagon with white canvas top"},
        {"suffix": "blue_canvas", "prompt": "covered wagon with blue canvas top"},
        {"suffix": "red_canvas",  "prompt": "covered wagon with red canvas top"},
    ],
    "hand_cart_wooden":      WOOD_PALETTE,
    "barrel_open_water":     WOOD_PALETTE,
    "pulley_system":         WOOD_PALETTE,

    # LIGHTING (4)
    "lantern_oil":           IRON_PALETTE,
    "candelabra":            [
        {"suffix": "brass",       "prompt": "polished brass candelabra"},
        {"suffix": "silver",      "prompt": "polished silver candelabra"},
        {"suffix": "gold",        "prompt": "ornate gold candelabra"},
        {"suffix": "iron_black",  "prompt": "black iron candelabra"},
    ],
    "torch_floor_iron":      IRON_PALETTE,
    "wagon_wheel_chandelier": WOOD_PALETTE,

    # TERRAIN (5)
    "ledge_overhang":        STONE_PALETTE,
    "waterfall_rock":        STONE_PALETTE,
    "mountain_peak":         [
        {"suffix": "snow_capped", "prompt": "snow-capped gray mountain peak"},
        {"suffix": "bare_rock",   "prompt": "bare gray rock mountain peak"},
        {"suffix": "red_rocky",   "prompt": "red rocky mountain peak, desert"},
    ],
    "crater":                [
        {"suffix": "rocky",       "prompt": "rocky impact crater"},
        {"suffix": "snow",        "prompt": "snow-filled crater"},
        {"suffix": "lava",        "prompt": "lava-filled volcanic crater"},
    ],
    "stone_mound":           STONE_PALETTE,

    # ARCHITECTURE (4)
    "watchtower_wooden":     WOOD_PALETTE,
    "windmill":              [
        {"suffix": "stone",       "prompt": "gray stone windmill with wooden sails"},
        {"suffix": "white",       "prompt": "white-painted windmill"},
        {"suffix": "wooden",      "prompt": "all-wooden windmill"},
    ],
    "lighthouse":            [
        {"suffix": "white_red",   "prompt": "white lighthouse with red top"},
        {"suffix": "stone_gray",  "prompt": "gray stone lighthouse"},
        {"suffix": "white_blue",  "prompt": "white lighthouse with blue top"},
    ],
    "barn_wooden":           [
        {"suffix": "red",         "prompt": "classic red barn"},
        {"suffix": "natural_wood","prompt": "natural wood barn, unpainted"},
        {"suffix": "weathered",   "prompt": "weathered gray old barn"},
    ],
    "stone_bench":           STONE_PALETTE,
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


def wait_for_color_variants_done():
    """Block until existing color_variants run logs COLOR VARIANTS DONE."""
    log("Waiting for existing color_variants run to finish...")
    cv_log = ROOT / "_color_variants.log"
    deadline = time.time() + 24 * 3600
    while time.time() < deadline:
        try:
            if cv_log.exists():
                text = cv_log.read_text(encoding="utf-8", errors="replace")
                if "COLOR VARIANTS DONE" in text:
                    log("color_variants finished — starting batch3")
                    return True
                lines = [l for l in text.split("\n") if l.strip()]
                if lines:
                    log(f"  ...still running: {lines[-1][-100:]}")
        except Exception as e:
            log(f"  log read err: {e}")
        time.sleep(180)
    log("WARNING: 24h timeout waiting; proceeding anyway")
    return False


def asset_already_done(asset):
    asset_dir = ROOT / asset["category"] / asset["id"]
    m = asset_dir / "manifest.json"
    if not m.exists():
        return False
    try:
        d = json.loads(m.read_text())
        return d.get("status") == "ok"
    except Exception:
        return False


# ============================================================================
# PHASES
# ============================================================================
def phase_generate(rembg_session):
    log("=" * 78)
    log(f"PHASE A: Generate {len(BATCH3_ASSETS)} new assets (multi-input + validation)")
    log("=" * 78)

    completed = 0
    failed = 0
    skipped = 0
    start = time.time()

    for i, asset in enumerate(BATCH3_ASSETS, 1):
        if asset_already_done(asset):
            skipped += 1
            continue

        log("")
        log("-" * 78)
        log(f"[{i}/{len(BATCH3_ASSETS)}] {asset['category']}/{asset['id']}")
        log(f"  subject: {asset['subject'][:80]}...")
        log("-" * 78)

        asset_dir = ROOT / asset["category"] / asset["id"]
        if asset_dir.exists():
            try: shutil.rmtree(asset_dir)
            except: pass
        asset_dir.mkdir(parents=True, exist_ok=True)

        try:
            manifest = p.process_asset_multi_input(asset, rembg_session)
            sweep = manifest.get("stages", {}).get("mesh_sweep", {})
            log(f"  DONE in {manifest['total_time_s']:.0f}s "
                f"(d={sweep.get('depth_ratio',0):.2f} v={sweep.get('volume',0):.2f})")
            completed += 1
        except Exception as e:
            failed += 1
            log(f"  FAILED: {str(e)[:200]}")
            try: (asset_dir / "ERROR.txt").write_text(f"{e}\n\n{traceback.format_exc()}")
            except: pass

        elapsed = time.time() - start
        avg = elapsed / max(i, 1)
        eta_min = avg * (len(BATCH3_ASSETS) - i) / 60
        log(f"  [batch3 progress: {i}/{len(BATCH3_ASSETS)} | ok={completed} fail={failed} skip={skipped} | ETA: {eta_min:.0f} min]")

    log(f"\nphase A done: {completed} ok, {failed} failed, {skipped} skipped in {(time.time()-start)/60:.1f} min")


def phase_cleanup(rembg_session):
    """Run cleanup specifically on batch3 assets — iterate until clean or max attempts."""
    log("=" * 78)
    log("PHASE B: Cleanup batch3 (iterate bad assets until 8 attempts each)")
    log("=" * 78)

    batch3_ids = {a["id"] for a in BATCH3_ASSETS}
    iteration = 0
    while iteration < 30:
        iteration += 1
        # Find bad batch3 assets
        bad = []
        for asset_dir, m in cu.scan_all_assets():
            if m.get("id") not in batch3_ids:
                continue
            is_bad, reasons = cu.evaluate_manifest(m)
            if not is_bad:
                continue
            attempts = m.get("attempts", 1)
            if attempts >= 8:
                continue
            bad.append((asset_dir, m, reasons))

        if not bad:
            log(f"\nALL CLEAN — no bad batch3 assets after {iteration-1} cleanup passes")
            break

        log(f"\n--- iteration {iteration}: {len(bad)} bad batch3 assets ---")
        for asset_dir, m, reasons in bad:
            log(f"")
            log(f"REROLL {m['id']}: {' '.join(reasons)}")
            try:
                cu.reroll_asset(asset_dir, m, reasons, rembg_session)
            except Exception as e:
                log(f"  reroll failed: {e}")


def phase_variants(rembg_session):
    """Generate color variants for batch3 good assets."""
    log("=" * 78)
    log(f"PHASE C: Color variants for batch3 (skip bad assets)")
    total_planned = sum(len(v) for v in BATCH3_VARIANTS.values())
    log(f"  Total variants planned: {total_planned}")
    log("=" * 78)

    done = 0
    failed = 0
    skipped_bad = 0
    skipped_existing = 0
    start = time.time()

    for asset_id, variants in BATCH3_VARIANTS.items():
        category = cv.find_asset_category(asset_id)
        if category is None:
            log(f"  {asset_id}: not found, skip")
            skipped_bad += len(variants)
            continue

        is_good, why = cv.asset_is_good(asset_id, category)
        if not is_good:
            log(f"  {asset_id}: SKIP (bad: {why}), {len(variants)} variants saved")
            skipped_bad += len(variants)
            continue

        log("")
        log(f"=== {category}/{asset_id} — {len(variants)} variants ===")
        for v in variants:
            log(f"  variant: {v['suffix']}")
            try:
                result, status = cv.make_color_variant(asset_id, category, v, rembg_session)
                if status == "ok":
                    done += 1
                    log(f"    OK")
                elif status == "already_done":
                    skipped_existing += 1
                    log(f"    SKIP (exists)")
                else:
                    failed += 1
                    log(f"    FAILED: {status}")
            except Exception as e:
                failed += 1
                log(f"    EXCEPTION: {e}")

    log(f"\nphase C done: ok={done}, existing={skipped_existing}, "
        f"bad_skipped={skipped_bad}, failed={failed} in {(time.time()-start)/60:.1f} min")


# ============================================================================
# MAIN
# ============================================================================
def main():
    log("=" * 78)
    log("BATCH 3 ORCHESTRATOR — 100 new assets + cleanup + variants")
    log("=" * 78)

    wait_for_color_variants_done()

    try:
        urllib.request.urlopen(f"{COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log(f"FATAL: ComfyUI unreachable: {e}")
        return 1

    log("Loading rembg...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    overall_start = time.time()

    try:
        phase_generate(rembg_session)
    except Exception as e:
        log(f"PHASE A ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    try:
        phase_cleanup(rembg_session)
    except Exception as e:
        log(f"PHASE B ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    try:
        phase_variants(rembg_session)
    except Exception as e:
        log(f"PHASE C ABORTED: {e}\n{traceback.format_exc()[:1000]}")

    log("")
    log("=" * 78)
    log(f"BATCH 3 COMPLETE — {(time.time()-overall_start)/60:.1f} min total")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
