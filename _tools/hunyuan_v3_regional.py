"""
Hunyuan v3 regeneration — ~300 new region-themed and modular-decorative assets.
Waits for v2 to complete (polls _v2_regen.log for "V2 REGEN COMPLETE") before
starting, so it queues cleanly behind the v2 run.

Output:
  generated_assets/<category>/<asset_id>/06_final.glb
  generated_assets/<category>/<asset_id>/variants/<color>/06_final.glb

After completion, run flatten_to_final_v3.py to gather into Final_v3/.

Total runtime estimate (RTX 5090):
  ~300 base × ~5 min = ~25 hours
  ~300 × ~4 variants × ~90s = ~30 hours
  Total: ~55 hours

Run:
  C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/hunyuan_v3_regional.py
"""
import sys
import time
import traceback
import urllib.request
from pathlib import Path

sys.stdout.reconfigure(line_buffering=True)
sys.path.insert(0, "C:/Sabri_MMO/_tools")

import hunyuan_asset_pipeline as p
import hunyuan_color_variants as cv

ROOT = p.ROOT
LOG_FILE = ROOT / "_v3_regen.log"
V2_LOG = ROOT / "_v2_regen.log"
WAIT_FOR_V2 = True  # set False to start immediately without queueing

# Reuse palettes from existing scripts
STONE_PALETTE = [
    {"suffix": "gray_granite",  "prompt": "weathered gray granite stone"},
    {"suffix": "white_marble",  "prompt": "polished white marble with subtle veining"},
    {"suffix": "sandstone_tan", "prompt": "warm tan sandstone"},
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
FABRIC_PALETTE = [
    {"suffix": "red_white",  "prompt": "red and white fabric"},
    {"suffix": "blue_white", "prompt": "blue and white fabric"},
    {"suffix": "green_gold", "prompt": "green and gold patterned fabric"},
    {"suffix": "purple",     "prompt": "rich purple velvet fabric with gold trim"},
]
COLOR4 = [
    {"suffix": "red",   "prompt": "red colored variant"},
    {"suffix": "blue",  "prompt": "blue colored variant"},
    {"suffix": "green", "prompt": "green colored variant"},
    {"suffix": "gold",  "prompt": "gold colored variant"},
]


# ============================================================================
# V3 ASSETS — ~300 items across regions and modular decoratives
# ============================================================================
V3_ASSETS = [
    # ============= PRONTERA / IZLUDE (Medieval European) =============
    {"id": "church_pew",            "category": "props",        "subject": "wooden church pew bench, long polished oak seat with high carved back, medieval cathedral furniture"},
    {"id": "altar_christian",       "category": "centerpiece",  "subject": "ornate stone Christian altar with cloth covering and gold cross on top, cathedral interior, gothic"},
    {"id": "pulpit_carved",         "category": "props",        "subject": "tall carved wooden pulpit with spiral staircase, raised preaching platform, gothic cathedral"},
    {"id": "pipe_organ",            "category": "special",      "subject": "tall pipe organ with vertical brass pipes and wooden console, cathedral instrument, ornate"},
    {"id": "stained_glass_window",  "category": "props",        "subject": "tall gothic stained glass window with colorful religious scene, lead lattice frame, cathedral"},
    {"id": "candelabra_floor",      "category": "props",        "subject": "tall ornate floor candelabra with multiple lit candles, brass branching design, cathedral"},
    {"id": "royal_throne",          "category": "centerpiece",  "subject": "ornate royal throne with red velvet cushion, high carved back with gold trim, on raised platform"},
    {"id": "carpet_runner_red",     "category": "props",        "subject": "long red carpet runner with gold trim border, royal palace floor decoration"},
    {"id": "knight_statue",         "category": "centerpiece",  "subject": "stone statue of armored knight in full plate armor with sword raised, weathered gray stone"},
    {"id": "topiary_cone",          "category": "vegetation",   "subject": "ornamental topiary shrub trimmed into tall cone shape, dense green foliage, garden plant"},
    {"id": "topiary_sphere",        "category": "vegetation",   "subject": "ornamental topiary trimmed into perfect green sphere, manicured garden plant on stone base"},
    {"id": "hedge_maze_section",    "category": "vegetation",   "subject": "section of tall green hedge maze wall, dense leafy bush wall, formal garden"},
    {"id": "ornamental_urn",        "category": "props",        "subject": "tall ornate stone garden urn with classical handles, decorative planter"},
    {"id": "sundial",               "category": "props",        "subject": "stone sundial with brass triangular gnomon on circular pedestal, garden timepiece"},
    {"id": "gargoyle_statue",       "category": "props",        "subject": "stone gargoyle statue with bat wings and snarling face, gothic cathedral water spout"},
    {"id": "balustrade_section",    "category": "props",        "subject": "ornate stone balustrade section with decorative balusters, classical architecture railing"},
    {"id": "aqueduct_arch",         "category": "architecture", "subject": "single Roman aqueduct arch section, gray weathered stone, classical infrastructure"},

    # ============= GEFFEN (Mage / Gothic) =============
    {"id": "magic_circle_floor",    "category": "dungeon",      "subject": "round magic circle on stone floor with glowing blue runes in concentric rings, mystical seal"},
    {"id": "floating_crystal_cluster","category": "dungeon",    "subject": "cluster of glowing magical crystals floating above stone base, blue and purple gems with light"},
    {"id": "alchemy_glassware",     "category": "props",        "subject": "alchemy glassware setup with bottles retorts and distillation tubes on wooden stand"},
    {"id": "magic_tome_open",       "category": "props",        "subject": "open ancient magic tome on wooden lectern, glowing text, leather-bound book with metal clasps"},
    {"id": "magic_tome_closed",     "category": "props",        "subject": "closed ornate spellbook on stone pedestal, leather-bound with metal corners and gem inlay"},
    {"id": "scroll_pile",           "category": "props",        "subject": "pile of rolled parchment scrolls with red wax seals, library accessory"},
    {"id": "cauldron_bubbling",     "category": "special",      "subject": "iron witch cauldron on tripod legs with bubbling green liquid, magical glow, three legs"},
    {"id": "runestone_engraved",    "category": "dungeon",      "subject": "tall standing stone with carved runic symbols, weathered gray stone monolith"},
    {"id": "runestone_glowing",     "category": "dungeon",      "subject": "standing stone with glowing blue magical runes carved into surface, mystical monolith"},
    {"id": "wizard_tower_top",      "category": "architecture", "subject": "decorative pointed wizard tower top section with conical roof and magical orb finial"},

    # ============= PAYON / ASIAN =============
    {"id": "pagoda_3tier",          "category": "architecture", "subject": "three tier pagoda tower with curved upturned eave roofs, red lacquered wood, Asian temple"},
    {"id": "pagoda_5tier",          "category": "architecture", "subject": "five tier tall pagoda tower with stacked upturned roofs, ornate Asian temple architecture"},
    {"id": "torii_gate",            "category": "architecture", "subject": "traditional Japanese torii gate, two red pillars with horizontal beam top, simple shrine entrance"},
    {"id": "stone_lantern_toro",    "category": "props",        "subject": "Japanese stone garden lantern with pagoda-like top, weathered gray stone, traditional toro"},
    {"id": "paper_lantern_round",   "category": "props",        "subject": "round red paper lantern hanging from wooden frame, traditional Asian decoration"},
    {"id": "paper_lantern_cylinder","category": "props",        "subject": "tall cylindrical white paper lantern with black calligraphy, hanging Asian decoration"},
    {"id": "koi_pond",              "category": "terrain",      "subject": "round koi pond with water and orange fish visible, low stone rim, garden water feature"},
    {"id": "shishi_odoshi",         "category": "props",        "subject": "Japanese bamboo water clacker shishi-odoshi with pivoting bamboo arm, garden device"},
    {"id": "dol_hareubang",         "category": "centerpiece",  "subject": "Korean stone grandfather statue with hat and folded hands, weathered black volcanic stone"},
    {"id": "fu_dog_statue",         "category": "centerpiece",  "subject": "Chinese fu dog stone statue, mythical lion-dog guardian, ornate carved temple guardian"},
    {"id": "drying_rack_fish",      "category": "props",        "subject": "wooden drying rack with hanging fish, traditional Asian outdoor preservation rack"},
    {"id": "tea_set",               "category": "props",        "subject": "Japanese tea set with cast iron teapot and small cups on tray, traditional ceremony"},
    {"id": "bamboo_water_pipe",     "category": "props",        "subject": "bamboo segmented water pipe leaning, traditional Asian plumbing fixture"},

    # ============= MORROC / SOGRAT (Desert / Egyptian) =============
    {"id": "sphinx_statue",         "category": "centerpiece",  "subject": "Egyptian sphinx statue with lion body and pharaoh head, sandstone, ancient monument"},
    {"id": "obelisk_egyptian",      "category": "architecture", "subject": "tall Egyptian obelisk with carved hieroglyphs and pyramidal gold cap, ancient stone monolith"},
    {"id": "pyramid_block",         "category": "architecture", "subject": "single large weathered limestone pyramid construction block, ancient Egyptian, sandy color"},
    {"id": "hieroglyphic_panel",    "category": "props",        "subject": "stone wall panel covered in carved Egyptian hieroglyphs, sandstone with painted accents"},
    {"id": "sand_dune_decorative",  "category": "terrain",      "subject": "small decorative sand dune mound, golden yellow sand with wind-rippled surface"},
    {"id": "hitching_rail_wooden",  "category": "props",        "subject": "wooden hitching rail with horizontal beam between two posts, animal tying point, weathered"},
    {"id": "carpet_persian",        "category": "props",        "subject": "ornate Persian carpet rug rolled out flat, intricate red and blue patterns with fringe"},
    {"id": "floor_cushion",         "category": "props",        "subject": "large square embroidered floor cushion, jewel-toned silk with tassels, Middle Eastern lounge"},
    {"id": "low_table_arabic",      "category": "props",        "subject": "small low Arabic table with brass tray top and carved wooden legs, Middle Eastern furniture"},
    {"id": "hookah",                "category": "props",        "subject": "traditional Arabic hookah water pipe with brass body and curved hose, Middle Eastern smoking"},
    {"id": "cracked_sandstone_column","category": "architecture","subject": "ancient cracked Egyptian sandstone column, partially broken, weathered sand-buried"},

    # ============= COMODO / COAST (Carnival + Beach) =============
    {"id": "carnival_tent_striped", "category": "architecture", "subject": "circus carnival tent with red and white stripes, peaked top with flag, festival"},
    {"id": "stage_platform_wood",   "category": "architecture", "subject": "raised wooden performance stage platform with steps, simple festival stage"},
    {"id": "performer_pole",        "category": "props",        "subject": "tall striped circus performer pole with platform on top, acrobat equipment"},
    {"id": "beach_umbrella",        "category": "props",        "subject": "open beach umbrella with colorful stripes on wooden pole, sun shade"},
    {"id": "deck_chair",            "category": "props",        "subject": "wooden folding deck chair with striped fabric seat, beach lounger"},
    {"id": "driftwood_log",         "category": "terrain",      "subject": "weathered driftwood log with twisted gray bark, sun-bleached, beach"},
    {"id": "seashell_conch",        "category": "props",        "subject": "large pink and white conch seashell with pointed spiral, beach decoration"},
    {"id": "starfish",              "category": "props",        "subject": "orange five-pointed starfish, textured surface, beach creature"},
    {"id": "sand_pile_beach",       "category": "terrain",      "subject": "small mound of golden beach sand, fine grain pile"},
    {"id": "lighthouse_small",      "category": "architecture", "subject": "small white stone lighthouse with red top and lantern room, coastal beacon"},
    {"id": "ship_anchor",           "category": "props",        "subject": "rusted iron ship anchor with curved arms and ring, weathered maritime equipment"},
    {"id": "rope_coil",             "category": "props",        "subject": "thick rope coiled into round pile, brown maritime rope, dock equipment"},
    {"id": "buoy_floating",         "category": "props",        "subject": "round red and white nautical buoy with flag pole, floating marker"},
    {"id": "fishing_net_drying",    "category": "props",        "subject": "fishing net hanging on wooden rack to dry, knotted brown rope mesh, dock"},
    {"id": "crab_trap",             "category": "props",        "subject": "wooden crab trap cage with rope, small wire mesh box, fishing equipment"},
    {"id": "fish_market_crate",     "category": "props",        "subject": "wooden fish market crate with ice and fish on top, weathered wood, market"},

    # ============= GLAST HEIM (Ruined Gothic) =============
    {"id": "broken_throne",         "category": "centerpiece",  "subject": "shattered stone throne with cracked back and missing arm, ruined royal seat, gothic"},
    {"id": "smashed_glass_pile",    "category": "props",        "subject": "pile of broken stained glass shards on floor, colorful fragments, ruined cathedral"},
    {"id": "tattered_banner",      "category": "props",        "subject": "torn ragged hanging banner, faded heraldic fabric with holes, abandoned ruin decoration"},
    {"id": "iron_maiden",           "category": "props",        "subject": "medieval iron maiden torture device, upright spiked sarcophagus, dungeon equipment"},
    {"id": "torture_rack",          "category": "props",        "subject": "wooden torture rack with rope pulleys, medieval interrogation device, dungeon"},
    {"id": "fallen_knight_armor",   "category": "props",        "subject": "collapsed pile of medieval knight armor with helmet and breastplate, scattered metal pieces"},
    {"id": "crumbling_buttress",    "category": "architecture", "subject": "crumbling gothic flying buttress, cracked gray stone arch support, ruined cathedral"},

    # ============= NIFLHEIM (Spooky / Halloween) =============
    {"id": "jackolantern_small",    "category": "props",        "subject": "small carved orange Halloween pumpkin jack-o-lantern with glowing face, single specimen"},
    {"id": "jackolantern_large",    "category": "props",        "subject": "large carved orange Halloween pumpkin with menacing glowing face, single specimen"},
    {"id": "will_o_wisp_post",      "category": "props",        "subject": "iron post with glowing ghostly blue flame on top, spooky lantern, no candle"},
    {"id": "wicker_scarecrow_creepy","category": "props",       "subject": "creepy wicker scarecrow on wooden cross, twisted straw body, sinister, horror"},
    {"id": "iron_fence_skull",      "category": "props",        "subject": "twisted black iron fence section with skull-shaped finials on top, gothic cemetery"},
    {"id": "soul_candle",           "category": "props",        "subject": "single tall white candle with blue flame on dripping pillar, ethereal soul light"},

    # ============= LUTIE (Snow / Christmas) =============
    {"id": "snowman",               "category": "centerpiece",  "subject": "classic snowman with three white snow balls stacked, carrot nose and coal eyes, scarf"},
    {"id": "snowdrift_modular",     "category": "terrain",      "subject": "small drifting white snow pile mound with subtle blue shadows, winter terrain"},
    {"id": "snow_capped_pine",      "category": "vegetation",   "subject": "tall pine tree with thick white snow caps on dark green branches, winter conifer"},
    {"id": "ice_crystal_formation", "category": "terrain",      "subject": "tall sharp angular ice crystal cluster formation, transparent pale blue, frozen"},
    {"id": "frozen_pond_disc",      "category": "terrain",      "subject": "round frozen pond ice surface disc, smooth pale blue with cracks, winter"},
    {"id": "christmas_wreath",      "category": "props",        "subject": "round Christmas wreath with green pine needles and red bow with berries, festive"},
    {"id": "christmas_garland",     "category": "props",        "subject": "long green Christmas garland strand with red ribbons and pine cones, decorative"},
    {"id": "gift_box",              "category": "props",        "subject": "wrapped Christmas gift box with red bow on top, single present"},
    {"id": "candy_cane_giant",      "category": "props",        "subject": "giant red and white striped candy cane standing upright, oversized Christmas decoration"},
    {"id": "ice_slide",             "category": "props",        "subject": "small ice slide with curved smooth surface, blue translucent winter playground"},

    # ============= HUGEL / RURAL =============
    {"id": "windmill_dutch",        "category": "architecture", "subject": "tall stone windmill with four wooden sail blades, classical Dutch design, single building"},
    {"id": "watermill",             "category": "architecture", "subject": "stone watermill with large wooden water wheel on side, mill house with thatched roof"},
    {"id": "round_hay_bale",        "category": "props",        "subject": "large round cylindrical hay bale, golden dried hay rolled tight, single bale"},
    {"id": "wheat_crop_section",    "category": "vegetation",   "subject": "tall golden wheat crop bunch with grain heads and stalks, harvest ready"},
    {"id": "corn_crop_section",     "category": "vegetation",   "subject": "tall green corn stalks with yellow corn ears, single plant cluster"},
    {"id": "chicken_coop",          "category": "architecture", "subject": "small wooden chicken coop with sloped roof and hatch entrance, animal pen, weathered wood"},
    {"id": "pig_sty",               "category": "architecture", "subject": "wooden pig pen enclosure with low fence and trough, farm structure"},
    {"id": "feed_trough",           "category": "props",        "subject": "long wooden trough on legs for animal feed, weathered planks, farm equipment"},
    {"id": "beehive_box",           "category": "props",        "subject": "wooden Langstroth beehive box, white painted with stacked sections, beekeeping"},
    {"id": "birdcage_ornate",       "category": "props",        "subject": "ornate brass birdcage with curved bars and dome top, Victorian, on stand"},

    # ============= JUNO / SCHWARZWALD (Sky City / Steampunk) =============
    {"id": "airship_dock_platform", "category": "architecture", "subject": "wooden airship docking platform with railings, raised wooden deck for floating ships"},
    {"id": "mooring_post",          "category": "props",        "subject": "wooden mooring post with rope rings, dock anchor, single tall post"},
    {"id": "floating_islet",        "category": "terrain",      "subject": "small floating rocky islet with grass on top and dangling magical roots, levitating"},
    {"id": "brass_cogwheel",        "category": "props",        "subject": "large brass cogwheel gear with teeth, polished steampunk machinery component"},
    {"id": "telescope_brass",       "category": "props",        "subject": "tall brass telescope on tripod stand, astronomical instrument, Victorian"},
    {"id": "astrolabe",             "category": "props",        "subject": "ornate brass astrolabe astronomical instrument, intricate engraved circles, antique"},
    {"id": "globe_stand",           "category": "props",        "subject": "polished wooden globe of the world on brass stand, library decoration"},

    # ============= EINBROCH / EINBECH (Industrial / Dwarven) =============
    {"id": "mine_cart",             "category": "props",        "subject": "iron mine cart with wooden sides on small wheels, ore hauler, industrial mining"},
    {"id": "rail_track_segment",    "category": "props",        "subject": "short section of iron rail track with wooden ties, mining or train rail"},
    {"id": "coal_pile",             "category": "terrain",      "subject": "pile of black shiny coal lumps, fuel heap, industrial fuel"},
    {"id": "slag_heap",             "category": "terrain",      "subject": "pile of dark gray industrial slag waste, rough chunks, mining byproduct"},
    {"id": "ore_vein",              "category": "terrain",      "subject": "rocky outcrop with sparkling silver ore vein, mineral deposit, mining"},
    {"id": "smokestack_chimney",    "category": "architecture", "subject": "tall industrial brick smokestack chimney with cap, single tall pillar, factory"},
    {"id": "factory_pipe",          "category": "props",        "subject": "thick rusted industrial metal pipe with valve wheel, factory equipment"},
    {"id": "steel_girder",          "category": "props",        "subject": "long horizontal steel I-beam girder, riveted construction, industrial structural"},
    {"id": "riveted_panel",         "category": "props",        "subject": "rusty riveted metal panel sheet with bolts, industrial dwarven construction"},
    {"id": "bellows_industrial",    "category": "props",        "subject": "large blacksmith bellows with leather pleated sides and wooden handles, industrial forge"},
    {"id": "smithing_hammer_display","category": "props",       "subject": "blacksmith hammer mounted on wooden display stand, decorative tool"},

    # ============= THOR / MAGMA (Volcanic) =============
    {"id": "lava_pool_surface",     "category": "terrain",      "subject": "circular pool of glowing red orange lava with dark crust edge, magma pool"},
    {"id": "magma_rock_emissive",   "category": "terrain",      "subject": "dark volcanic rock with glowing red orange cracks, hot lava stone"},
    {"id": "volcanic_vent",         "category": "terrain",      "subject": "rocky volcanic steam vent fissure, gray rock with crack opening"},
    {"id": "obsidian_shard",        "category": "props",        "subject": "tall sharp black glassy obsidian crystal shard, volcanic glass"},
    {"id": "sulfur_deposit",        "category": "terrain",      "subject": "yellow crystalline sulfur mineral deposit on rock, volcanic mineral"},

    # ============= BYALAN / UNDERWATER =============
    {"id": "coral_brain",           "category": "vegetation",   "subject": "round brain coral cluster with maze-like pattern, pink and orange marine life"},
    {"id": "coral_fan",             "category": "vegetation",   "subject": "tall purple sea fan coral with branching fronds, marine plant"},
    {"id": "coral_tube",            "category": "vegetation",   "subject": "cluster of cylindrical orange tube corals, marine creature"},
    {"id": "sea_anemone",           "category": "vegetation",   "subject": "pink sea anemone with waving tentacles, underwater creature on rock"},
    {"id": "kelp_strand",           "category": "vegetation",   "subject": "tall green kelp seaweed strand with flat broad fronds, underwater plant"},
    {"id": "sunken_ship_prow",      "category": "props",        "subject": "weathered sunken wooden ship prow with broken planks and anchor, shipwreck"},

    # ============= CAVES =============
    {"id": "stalactite_small",      "category": "terrain",      "subject": "small pointed stalactite hanging from ceiling, gray cave mineral formation"},
    {"id": "stalactite_medium",     "category": "terrain",      "subject": "medium icicle-like stalactite hanging from cave ceiling, gray dripping mineral"},
    {"id": "stalactite_large",      "category": "terrain",      "subject": "large pointed stalactite hanging downward, gray cave formation, single specimen"},
    {"id": "glow_mushroom",         "category": "vegetation",   "subject": "small glowing magical mushroom with cyan luminous cap and stem, cave fungus"},
    {"id": "mineral_vein_gold",     "category": "terrain",      "subject": "rocky outcrop with bright gold ore vein streaks, mineral deposit"},

    # ============= INTERIOR FURNISHINGS =============
    {"id": "bed_peasant_straw",     "category": "props",        "subject": "simple wooden peasant bed with straw mattress and rough blanket, rustic furniture"},
    {"id": "bed_noble_fourposter",  "category": "props",        "subject": "ornate four-poster bed with red velvet canopy and pillars, royal furniture"},
    {"id": "bed_inn_bunk",          "category": "props",        "subject": "wooden double bunk bed with two stacked mattresses, inn travel furniture"},
    {"id": "chair_armchair",        "category": "props",        "subject": "upholstered wooden armchair with red velvet cushion, formal sitting furniture"},
    {"id": "table_dining",          "category": "props",        "subject": "long wooden dining table with carved legs, medieval banquet furniture"},
    {"id": "wardrobe_wooden",       "category": "props",        "subject": "tall wooden wardrobe with two paneled doors and hinges, dark stained oak"},
    {"id": "dresser_drawers",       "category": "props",        "subject": "wooden dresser with three rows of drawers and brass handles, bedroom furniture"},
    {"id": "chest_clothing",        "category": "props",        "subject": "wooden hope chest with curved lid and iron banding, blanket storage box"},
    {"id": "fireplace_stone",       "category": "props",        "subject": "stone fireplace with mantle and arched opening, brick interior, room hearth"},
    {"id": "cooking_pot_fire",      "category": "props",        "subject": "iron cooking pot hanging on tripod over campfire flames, outdoor cooking"},
    {"id": "rug_oriental",          "category": "props",        "subject": "ornate oriental rug carpet with intricate red and blue patterns, fringed edges"},
    {"id": "rug_fur",               "category": "props",        "subject": "white shaggy fur animal pelt rug, soft polar bear hide on floor"},
    {"id": "picture_frame_oil",     "category": "props",        "subject": "ornate gold-framed oil painting hanging on wall, classical portrait"},
    {"id": "mirror_wall_oval",      "category": "props",        "subject": "tall oval mirror in ornate gold frame with carved details, hanging wall mirror"},
    {"id": "wall_sconce_iron",      "category": "props",        "subject": "iron wall sconce with single lit candle, bracket-mounted lighting"},
    {"id": "bar_counter",           "category": "props",        "subject": "wooden tavern bar counter with brass railing on top, polished oak surface"},
    {"id": "beer_keg_wooden",       "category": "props",        "subject": "small wooden beer keg with iron bands and tap spout, tavern barrel"},
    {"id": "stage_platform_bard",   "category": "architecture", "subject": "small raised wooden bard performance stage with steps, tavern stage"},

    # ============= NPC WORKSTATIONS =============
    {"id": "forge_blacksmith",      "category": "special",      "subject": "stone blacksmith forge with glowing red coals and bellows attached, smithing furnace"},
    {"id": "tailor_mannequin",      "category": "props",        "subject": "wooden tailor dress mannequin with cloth dress on stand, sewing display"},
    {"id": "sewing_table",          "category": "props",        "subject": "wooden sewing table with fabric scraps and wooden spool of thread on top, tailor furniture"},
    {"id": "fabric_bolt",           "category": "props",        "subject": "single rolled bolt of colored fabric on tailor floor, textile material"},
    {"id": "apothecary_shelf",      "category": "props",        "subject": "tall wooden apothecary shelf with rows of small labeled glass bottles, herbalist storage"},
    {"id": "coin_scale",            "category": "props",        "subject": "brass merchant coin scale with hanging plates on stand, weighing balance"},
    {"id": "ledger_book",           "category": "props",        "subject": "open large ledger accounting book on wooden lectern, business record"},
    {"id": "abacus",                "category": "props",        "subject": "wooden abacus calculator with colored beads on wires, merchant tool"},
    {"id": "iron_storage_vault",    "category": "props",        "subject": "tall iron storage vault chest with thick lock and reinforcing bands, secure storage"},

    # ============= RELIGIOUS / SPIRITUAL =============
    {"id": "altar_norse",           "category": "centerpiece",  "subject": "stone Norse pagan altar with carved runes and offering bowl, ancient pre-Christian"},
    {"id": "incense_burner_brass",  "category": "props",        "subject": "brass incense burner hanging on chains with smoke holes, religious censer"},
    {"id": "odin_cross",            "category": "props",        "subject": "stone Norse Odin cross with carved runes, ancient pagan religious symbol"},
    {"id": "asian_shrine_offering", "category": "props",        "subject": "small wooden Asian shrine altar with rice bowl offerings and incense, household shrine"},
    {"id": "reliquary_ornate",      "category": "props",        "subject": "ornate gold reliquary box with cross top and gem inlay, religious relic holder"},
    {"id": "offering_bowl_stone",   "category": "props",        "subject": "carved stone offering bowl with lotus design, religious vessel"},
    {"id": "prayer_cairn",          "category": "centerpiece",  "subject": "tall stack of round flat prayer stones balanced on top of each other, cairn pile"},

    # ============= PATH & ROAD =============
    {"id": "cobblestone_road_segment","category": "terrain",    "subject": "section of cobblestone paved road, irregular gray stones with grass between, medieval"},
    {"id": "dirt_path_tile",        "category": "terrain",      "subject": "section of dirt path with worn earth and small pebbles, rural walkway"},
    {"id": "stepping_stone",        "category": "terrain",      "subject": "single flat round gray stepping stone with moss edges, garden path stone"},
    {"id": "milepost",              "category": "props",        "subject": "wooden milepost sign with carved distance number on tall post, road marker"},
    {"id": "direction_sign_wood",   "category": "props",        "subject": "wooden direction sign post with multiple painted arrows pointing different ways, road signpost"},
    {"id": "boardwalk_section",     "category": "terrain",      "subject": "section of wooden boardwalk planks with gaps between, raised walking path"},
    {"id": "stone_arch_road_gate",  "category": "architecture", "subject": "stone arched road gate with single span, weathered gray stone, town entrance"},

    # ============= COMBAT / TRAINING =============
    {"id": "training_dummy_straw",  "category": "props",        "subject": "wooden training dummy on cross post stuffed with straw, practice target"},
    {"id": "training_dummy_armor",  "category": "props",        "subject": "training dummy in plate armor on wooden frame, knight practice target"},
    {"id": "archery_target_round",  "category": "props",        "subject": "round red and white archery target on wooden stand, bullseye, practice"},
    {"id": "archery_target_haybale","category": "props",        "subject": "round hay bale with target circles painted on, archery practice"},
    {"id": "sparring_ring_rope",    "category": "props",        "subject": "circular sparring ring with rope around posts, training arena floor"},

    # ============= WATER FEATURES =============
    {"id": "waterfall_small_full",  "category": "terrain",      "subject": "small rocky waterfall with cascading water stream, gray stones with white foam"},
    {"id": "waterfall_medium_full", "category": "terrain",      "subject": "medium tall waterfall on rocky cliff with white falling water and mist, single fall"},
    {"id": "stream_with_rocks",     "category": "terrain",      "subject": "small flowing stream with smooth rocks and clear water, gentle creek section"},
    {"id": "irrigation_channel",    "category": "terrain",      "subject": "rectangular stone irrigation channel with flowing water, ancient aqueduct trough"},
    {"id": "hot_spring_pool",       "category": "terrain",      "subject": "natural hot spring pool with steaming blue-green water and rocky rim, geothermal"},

    # ============= HOLIDAY / SEASONAL =============
    {"id": "festival_lantern_string","category": "props",       "subject": "string of glowing colorful festival paper lanterns hanging on wire, party decoration"},
    {"id": "maypole_decorative",    "category": "props",        "subject": "tall maypole with colored ribbons hanging from top, traditional spring festival"},
    {"id": "fair_stage_small",      "category": "architecture", "subject": "small striped fair stage platform with curtain backdrop, traveling festival stage"},
    {"id": "prize_booth",           "category": "architecture", "subject": "small wooden carnival prize booth with striped awning and counter, festival game stall"},
    {"id": "harvest_cornucopia",    "category": "props",        "subject": "wicker cornucopia horn overflowing with autumn fruits and vegetables, harvest"},

    # ============= MISC HIGH-VALUE RO-SPECIFIC =============
    {"id": "save_point_obelisk",    "category": "centerpiece",  "subject": "tall glowing crystal obelisk with sparkling magical aura, save point marker"},
    {"id": "casino_roulette",       "category": "props",        "subject": "round casino roulette table with red and black numbered wheel, gambling table"},
    {"id": "casino_dice_table",     "category": "props",        "subject": "rectangular casino dice gambling table with green felt and white markings"},
    {"id": "poker_chip_stack",      "category": "props",        "subject": "tall stack of colorful poker chips, casino tokens stacked"},
    {"id": "guardian_pedestal",     "category": "centerpiece",  "subject": "ornate stone guardian pedestal with carved details and crystal on top, castle defense"},
    {"id": "woe_breakable_barricade","category": "props",       "subject": "wooden defensive barricade with sharpened logs, military fortification, single section"},

    # ============= UMBALA (Treetop Jungle Village) =============
    {"id": "treehouse_platform_small","category": "architecture","subject": "small wooden treehouse platform on tree trunk with railing, jungle dwelling, single small platform"},
    {"id": "treehouse_platform_large","category": "architecture","subject": "large wooden treehouse platform built around tree trunk with thatched roof, jungle hut"},
    {"id": "rope_bridge_jungle",    "category": "terrain",      "subject": "wooden plank rope suspension bridge between trunks, jungle bridge with rope handrails"},
    {"id": "wooden_ladder_climbing","category": "props",        "subject": "tall wooden climbing ladder with rope rungs leaning against tree, primitive ladder"},
    {"id": "tribal_totem_feathered","category": "centerpiece",  "subject": "tall carved wooden tribal totem pole with feathered top and painted face, primitive art"},
    {"id": "war_drum_tribal",       "category": "props",        "subject": "large round wooden tribal war drum with stretched hide top and rope binding, tribal instrument"},
    {"id": "bone_fetish",           "category": "props",        "subject": "tribal bone fetish charm hanging from wooden post, decorative skull with feathers"},
    {"id": "vine_curtain",          "category": "vegetation",   "subject": "hanging cluster of green jungle vines forming curtain, draping leafy strands"},
    {"id": "oversized_leaf_awning", "category": "vegetation",   "subject": "huge tropical green leaf used as awning roof on wooden frame, jungle shelter"},
    {"id": "macaw_perch",           "category": "props",        "subject": "tall wooden bird perch stand with horizontal bar, decorative parrot post"},

    # ============= MOSCOVIA (Russian) =============
    {"id": "onion_dome_chapel",     "category": "architecture", "subject": "small Russian Orthodox chapel with single colorful striped onion dome, painted wooden church"},
    {"id": "painted_wooden_church", "category": "architecture", "subject": "ornate Russian wooden church with carved trim and painted blue accents, folk architecture"},
    {"id": "matryoshka_doll",       "category": "props",        "subject": "single ornate matryoshka nesting doll, painted Russian wooden doll"},
    {"id": "samovar",               "category": "props",        "subject": "tall ornate brass samovar tea urn with handles and spigot, Russian tea kettle"},
    {"id": "carved_wooden_gateway", "category": "architecture", "subject": "ornate Russian carved wooden gateway with folk patterns and arched top, decorative entrance"},

    # ============= RACHEL / VEINS (Arunafeltz desert temple) =============
    {"id": "white_marble_temple",   "category": "architecture", "subject": "tall white marble temple section with classical columns and pediment, sacred building"},
    {"id": "goddess_pedestal_statue","category": "centerpiece", "subject": "white marble goddess statue on tall ornate pedestal, classical female figure with robes"},
    {"id": "sacred_brazier",        "category": "props",        "subject": "ornate ceremonial bronze brazier on tripod legs with flames, sacred fire bowl"},
    {"id": "sand_buried_ruin",      "category": "architecture", "subject": "stone temple section partially buried in sand, weathered marble emerging from dune"},
    {"id": "eroded_statue_head",    "category": "centerpiece",  "subject": "weathered eroded ancient stone statue head on ground, broken sandstone face"},
    {"id": "religious_altar_sun",   "category": "centerpiece",  "subject": "stone altar with carved sun symbol on top, golden inlay, sacred religious"},

    # ============= ECLAGE (Fairy / Elven) =============
    {"id": "giant_walkable_flower", "category": "vegetation",   "subject": "giant oversized purple flower with mushroom-cap-sized petals, fairy world flora"},
    {"id": "mushroom_house",        "category": "architecture", "subject": "fairy mushroom house with red cap roof and tiny door at base, woodland dwelling"},
    {"id": "fairy_door_tree_base", "category": "architecture",  "subject": "tiny wooden door at base of tree trunk with arched top, fairy entrance"},
    {"id": "glowing_root_system",   "category": "vegetation",   "subject": "exposed magical tree roots with glowing blue veins, ethereal forest"},
    {"id": "fairy_ring_stones",     "category": "terrain",      "subject": "circular ring of small standing stones, fairy ring marker, magical clearing"},
    {"id": "silver_thread_web",     "category": "terrain",      "subject": "intricate silver-thread spider web bridge with shimmering strands, fairy crossing"},
    {"id": "hovering_moss_platform","category": "terrain",      "subject": "round floating moss-covered platform with magical glow, levitating fairy platform"},

    # ============= BRASILIS / DEWATA / MALAYA (Tropical) =============
    {"id": "stilt_house_bahay_kubo","category": "architecture", "subject": "tropical bamboo stilt house on wooden pillars with thatched palm roof, single stilt building"},
    {"id": "bamboo_ladder_tropical","category": "props",        "subject": "tropical bamboo ladder with bound rungs, leaning against wall, jungle access"},
    {"id": "banana_cluster",        "category": "vegetation",   "subject": "hanging cluster of yellow ripe bananas on stem, tropical fruit bunch"},
    {"id": "mango_cluster",         "category": "vegetation",   "subject": "small cluster of red and yellow ripe mangoes hanging from branch, tropical fruit"},
    {"id": "coconut_cluster",       "category": "vegetation",   "subject": "cluster of brown coconuts hanging from palm branch, tropical fruit"},
    {"id": "carnival_drum_tribal",  "category": "props",        "subject": "tall colorful tribal carnival drum with painted designs and stretched hide, festival instrument"},
    {"id": "festival_mask",         "category": "props",        "subject": "ornate carved wooden festival mask with colorful paint and feathers, tribal celebration"},
    {"id": "tribal_shield",         "category": "props",        "subject": "round tribal warrior shield with painted geometric patterns and bone trim, primitive weapon"},
    {"id": "outrigger_canoe",       "category": "props",        "subject": "tropical outrigger canoe with side float and paddle, Pacific Islander boat"},
    {"id": "bamboo_torch_palm",     "category": "props",        "subject": "tall bamboo torch with palm-frond shade on top, tropical lighting fixture"},

    # ============= OLD GLAST HEIM / GEFFENIA (Demonic) =============
    {"id": "demon_ritual_pentagram","category": "dungeon",      "subject": "stone floor pentagram circle with glowing red runes, dark demonic ritual seal"},
    {"id": "sacrificial_altar_chains","category": "dungeon",    "subject": "stone sacrificial altar slab with iron chains and shackles, gothic dark ritual"},
    {"id": "hellfire_brazier",      "category": "dungeon",      "subject": "iron tripod brazier with glowing dark red hellfire flames, sinister fire bowl"},
    {"id": "skull_pile_large",      "category": "dungeon",      "subject": "large heap of bleached white human skulls, ossuary mound, gothic"},
    {"id": "possessed_statue",      "category": "dungeon",      "subject": "weathered stone statue with glowing red demonic eyes, cursed gothic figure"},

    # ============= JAWAII (Honeymoon Resort) =============
    {"id": "heart_wedding_arch",    "category": "architecture", "subject": "white wedding arch arch decorated with pink roses and ribbons, romantic ceremony"},
    {"id": "rose_petal_pile",       "category": "props",        "subject": "small pile of red rose petals scattered on ground, romantic decoration"},
    {"id": "luxury_beach_hut",      "category": "architecture", "subject": "elegant white tropical beach hut with thatched roof and curtains, honeymoon villa"},
    {"id": "romantic_dinner_table", "category": "props",        "subject": "round dinner table for two with white tablecloth, candles, and wine glasses, romantic"},
    {"id": "pink_seashell_decor",   "category": "props",        "subject": "ornate large pink scallop seashell decorative ornament, beach decoration"},
    {"id": "ribbon_banner",         "category": "props",        "subject": "long curved ribbon banner with curly ends, decorative streamer"},

    # ============= YUNO SAGE ACADEMY =============
    {"id": "orrery_planet_model",   "category": "special",      "subject": "ornate brass mechanical orrery with planet spheres on rings around central sun, antique"},
    {"id": "lecturing_platform",    "category": "architecture", "subject": "raised wooden lecturing platform with magical chalkboard, academic stage"},
    {"id": "geometric_cube_floating","category": "special",     "subject": "floating glowing emissive geometric cube shape, magical academic study object"},
    {"id": "geometric_dodecahedron","category": "special",      "subject": "floating glowing emissive 12-sided dodecahedron shape, magical mathematical object"},
    {"id": "crystal_study_desk",    "category": "props",        "subject": "ornate wooden study desk with glowing crystal inlay top, mage scholar furniture"},
    {"id": "ancient_text_lectern",  "category": "props",        "subject": "tall wooden lectern with ancient open book on top, scholarly stand"},

    # ============= PYRAMID INTERIOR (Morroc Dungeon) =============
    {"id": "canopic_jar_jackal",    "category": "props",        "subject": "Egyptian canopic jar with jackal-headed lid, painted hieroglyphs, ancient burial"},
    {"id": "canopic_jar_bird",      "category": "props",        "subject": "Egyptian canopic jar with bird-headed lid, painted hieroglyphs, ancient burial"},
    {"id": "canopic_jar_human",     "category": "props",        "subject": "Egyptian canopic jar with human-headed lid, painted hieroglyphs, ancient burial"},
    {"id": "ankh_wall_symbol",      "category": "props",        "subject": "stone wall plaque with carved Egyptian ankh cross symbol, gold inlay, ancient"},
    {"id": "scarab_inlay",          "category": "props",        "subject": "decorative golden scarab beetle inlay on stone tile, Egyptian symbol"},
    {"id": "treasure_heap_gold",    "category": "props",        "subject": "pile of gold coins jewelry and crown, glittering treasure heap, Egyptian tomb"},
    {"id": "mummy_sarcophagus_open","category": "dungeon",      "subject": "upright Egyptian sarcophagus open with mummy inside, painted gold lid leaning against side"},
    {"id": "mummy_sarcophagus_closed","category": "dungeon",    "subject": "upright Egyptian sarcophagus closed with painted pharaoh face, gold and blue, ancient burial"},

    # ============= MODULAR BUILDING (decorative one-offs) =============
    # Walls
    {"id": "wall_stone_smooth",     "category": "architecture", "subject": "section of smooth gray stone wall with mortar lines, medieval castle wall, single segment"},
    {"id": "wall_stone_rough",      "category": "architecture", "subject": "section of rough gray fieldstone wall with irregular stones, rural construction, single segment"},
    {"id": "wall_half_timber",      "category": "architecture", "subject": "section of half-timber Tudor wall with white plaster and dark wood beams, medieval European"},
    {"id": "wall_brick_red",        "category": "architecture", "subject": "section of red brick wall with mortar lines, classic brick construction, single panel"},
    {"id": "wall_wooden_plank",     "category": "architecture", "subject": "section of vertical wooden plank wall with weathered planks, rustic construction"},
    {"id": "wall_wattle_daub",      "category": "architecture", "subject": "section of wattle-and-daub wall with woven sticks and tan plaster, primitive rural"},
    {"id": "wall_marble_panel",     "category": "architecture", "subject": "section of polished white marble wall panel with subtle veining, temple architecture"},
    {"id": "wall_adobe",            "category": "architecture", "subject": "section of tan adobe earthen wall with smooth surface, desert architecture"},

    # Windows
    {"id": "window_round",          "category": "props",        "subject": "circular round wooden window with criss-cross panes, single round porthole window"},
    {"id": "window_arched",         "category": "props",        "subject": "tall arched window with rounded top and wooden frame, simple medieval window"},
    {"id": "window_gothic_pointed", "category": "props",        "subject": "tall pointed gothic window with arched top and stone frame, cathedral lancet window"},
    {"id": "window_casement",       "category": "props",        "subject": "rectangular hinged casement window with iron latch, medieval European wooden window"},
    {"id": "window_lattice",        "category": "props",        "subject": "diamond-pattern lattice window with criss-cross wooden frame, decorative window"},
    {"id": "window_leaded_glass",   "category": "props",        "subject": "leaded glass window with diamond panes in lead frame, medieval craftsmanship"},
    {"id": "window_bay",            "category": "props",        "subject": "protruding bay window with three angled glass panes, decorative wall feature"},
    {"id": "window_dormer",         "category": "props",        "subject": "dormer window protruding from sloped roof with peaked top, single small dormer"},

    # Doors
    {"id": "door_iron_bound",       "category": "props",        "subject": "heavy wooden door reinforced with iron bands and rivets, medieval fortress door"},
    {"id": "door_paper_screen",     "category": "props",        "subject": "Japanese paper screen sliding door with wooden grid frame, traditional shoji"},
    {"id": "door_arched_cathedral", "category": "props",        "subject": "tall arched gothic cathedral door with iron hinges and pointed top, ornate religious"},
    {"id": "door_secret_stone",     "category": "props",        "subject": "stone slab door with hidden seam and small lever, secret passage entrance"},
    {"id": "door_double_wooden",    "category": "props",        "subject": "pair of wooden double doors with ornate brass handles and panel detail, formal entrance"},

    # Roofs
    {"id": "roof_red_clay_tile",    "category": "architecture", "subject": "section of red clay tile roof with curved tiles in rows, Mediterranean roofing"},
    {"id": "roof_slate_section",    "category": "architecture", "subject": "section of dark gray slate tile roof with overlapping flat tiles, medieval roofing"},
    {"id": "roof_thatch_section",   "category": "architecture", "subject": "section of golden brown thatched straw roof with rough texture, peasant roofing"},
    {"id": "roof_wooden_shingle",   "category": "architecture", "subject": "section of wooden shingle roof with overlapping cedar shakes, rustic roofing"},
    {"id": "roof_asian_upturned",   "category": "architecture", "subject": "section of Asian curved upturned eave roof with green tiles and dragon ornaments"},
    {"id": "roof_dome_cupola",      "category": "architecture", "subject": "decorative round dome cupola with gold finial top, ornate building dome"},

    # Columns
    {"id": "column_doric",          "category": "architecture", "subject": "classical Doric stone column, simple smooth shaft with plain capital top, single specimen"},
    {"id": "column_ionic",          "category": "architecture", "subject": "classical Ionic stone column with scroll spirals on capital top, fluted shaft"},
    {"id": "column_corinthian",     "category": "architecture", "subject": "ornate Corinthian column with acanthus leaf carved capital top, classical architecture"},
    {"id": "column_solomonic",      "category": "architecture", "subject": "twisted spiral Solomonic column with helical fluting, baroque architecture"},
    {"id": "column_ivy_wrapped",    "category": "architecture", "subject": "weathered stone column wrapped in green climbing ivy vines, ancient overgrown"},

    # Roof Accents
    {"id": "weather_vane_rooster",  "category": "props",        "subject": "iron weather vane with rooster shape on tall pole, rotating wind direction indicator"},
    {"id": "weather_vane_ship",     "category": "props",        "subject": "iron weather vane with sailing ship shape on pole, rotating wind indicator"},
    {"id": "finial_decorative",     "category": "props",        "subject": "ornate decorative metal finial with pointed orb top, roof peak ornament"},
    {"id": "eave_bracket",          "category": "props",        "subject": "carved wooden decorative eave bracket support, ornate Victorian roof corbel"},

    # ============= TOWN INFRASTRUCTURE =============
    {"id": "clock_tower",           "category": "architecture", "subject": "tall stone clock tower with large round clock face on top, medieval town clock building"},
    {"id": "bell_tower_belfry",     "category": "architecture", "subject": "tall stone bell tower with arched belfry openings showing brass bell, medieval"},
    {"id": "watchtower_modular",    "category": "architecture", "subject": "tall wooden watchtower with platform on top and ladder, frontier sentry post"},
    {"id": "public_bath_house",     "category": "architecture", "subject": "small bathhouse building with steam vents and stone walls, Roman style"},
    {"id": "stable_horse",          "category": "architecture", "subject": "wooden horse stable building with stalls and loft, farm structure"},
    {"id": "kennel_dog",            "category": "architecture", "subject": "small wooden dog kennel doghouse with peaked roof and entrance, single specimen"},
    {"id": "dovecote_round",        "category": "architecture", "subject": "round stone dovecote tower with conical roof and small entry holes, pigeon house"},
    {"id": "sewer_grate",           "category": "props",        "subject": "iron sewer grate with bars and frame, urban drainage cover"},
    {"id": "manhole_cover",         "category": "props",        "subject": "round iron manhole cover with raised pattern, street access cover"},
    {"id": "cistern_water_tank",    "category": "props",        "subject": "tall round wooden water cistern tank with iron bands, water storage"},
    {"id": "rooftop_water_barrel",  "category": "props",        "subject": "wooden water barrel mounted on rooftop platform, rainwater collection"},
    {"id": "stocks_pillory",        "category": "props",        "subject": "wooden stocks pillory with holes for head and arms, medieval punishment device"},
    {"id": "bandstand_gazebo",      "category": "architecture", "subject": "round wooden gazebo bandstand with pointed roof and ornate railings, town square"},
    {"id": "pergola_trellis",       "category": "architecture", "subject": "wooden pergola garden trellis with climbing pink roses, decorative arbor"},

    # ============= MAGICAL / ELEMENTAL =============
    {"id": "elemental_shrine_water","category": "centerpiece",  "subject": "blue water elemental shrine with flowing water and crystal centerpiece, magical altar"},
    {"id": "elemental_shrine_fire", "category": "centerpiece",  "subject": "red fire elemental shrine with eternal flame and ember stones, magical altar"},
    {"id": "elemental_shrine_earth","category": "centerpiece",  "subject": "brown earth elemental shrine with crystals and rock formation, magical altar"},
    {"id": "elemental_shrine_wind", "category": "centerpiece",  "subject": "white wind elemental shrine with floating ribbons and crystal, magical altar"},
    {"id": "mana_crystal_node",     "category": "props",        "subject": "tall blue glowing mana crystal cluster on stone base, magical resource node"},
    {"id": "magical_sigil_wall",    "category": "props",        "subject": "wall with glowing animated magical sigil runes carved into stone, mystical inscription"},
    {"id": "floating_book",         "category": "props",        "subject": "open glowing magical book floating in air with sparkles, levitating spellbook"},
    {"id": "animated_scroll",       "category": "props",        "subject": "unrolled magical scroll floating with glowing text, animated parchment"},
    {"id": "magic_mirror",          "category": "props",        "subject": "ornate gold-framed magic mirror with swirling reflective surface, mystical mirror"},
    {"id": "crystal_ball_stand",    "category": "props",        "subject": "clear crystal ball on small wooden tripod stand, fortune telling orb"},
    {"id": "wizard_staff_display",  "category": "props",        "subject": "ornate wizard staff with crystal top mounted on display stand, magical weapon"},
    {"id": "phoenix_urn",           "category": "props",        "subject": "ornate red and gold ceramic urn with painted phoenix design, magical vessel"},
    {"id": "dragon_urn",            "category": "props",        "subject": "ornate green ceramic urn with painted dragon coiled around, magical vessel"},
    {"id": "soul_lantern_blue",     "category": "props",        "subject": "iron lantern with gentle glowing blue flame inside glass, ethereal soul light"},

    # ============= INDUSTRIAL =============
    {"id": "train_platform",        "category": "architecture", "subject": "raised stone train station platform with bench, simple infrastructure"},
    {"id": "ticket_booth",          "category": "architecture", "subject": "small wooden ticket booth with window and counter, transit station"},
    {"id": "steam_locomotive",      "category": "props",        "subject": "decorative steam locomotive train engine with smokestack and wheels, single train car"},
    {"id": "conveyor_belt_segment", "category": "props",        "subject": "section of industrial conveyor belt with rollers and frame, factory equipment"},
    {"id": "pressure_gauge",        "category": "props",        "subject": "round brass pressure gauge with needle and dial, industrial meter"},
    {"id": "valve_wheel",           "category": "props",        "subject": "round red iron valve wheel with spokes, industrial pipe valve"},
    {"id": "rusted_machinery",      "category": "props",        "subject": "rusted abandoned industrial machinery hulk with gears and pipes, broken equipment"},
    {"id": "industrial_crane",      "category": "props",        "subject": "tall yellow industrial crane with hook on cable, construction equipment"},
    {"id": "loading_bay_platform",  "category": "architecture", "subject": "concrete loading bay platform with metal roller door, industrial dock"},
    {"id": "hotel_facade_4story",   "category": "architecture", "subject": "four story hotel building facade with rows of windows and entrance, urban architecture"},
    {"id": "iron_lamppost_gas",     "category": "props",        "subject": "tall ornate iron gas-lit lamppost with glass lantern top, Victorian streetlight"},

    # ============= MARITIME (Beyond Boats) =============
    {"id": "ship_hull_galleon_prow","category": "props",        "subject": "wooden galleon ship prow front section with figurehead, sailing ship bow"},
    {"id": "ship_hull_galleon_mid", "category": "props",        "subject": "wooden galleon ship midsection with cannons and gunports, sailing ship middle"},
    {"id": "ship_hull_galleon_stern","category": "props",       "subject": "wooden galleon ship stern back section with carved windows, sailing ship rear"},
    {"id": "fishing_dinghy",        "category": "props",        "subject": "small wooden fishing dinghy boat with oars, simple rowboat"},
    {"id": "sail_raised",           "category": "props",        "subject": "white triangular ship sail raised on mast, sailing canvas, single sail"},
    {"id": "sail_tattered",         "category": "props",        "subject": "tattered torn ragged ship sail with holes, weathered ghost ship sail"},
    {"id": "crows_nest",            "category": "props",        "subject": "wooden circular crows nest lookout platform on tall mast, ship lookout"},
    {"id": "cannon_loaded",         "category": "props",        "subject": "iron ship cannon on wooden carriage with wheels, naval artillery"},
    {"id": "captains_wheel",        "category": "props",        "subject": "wooden ship captains wheel with spokes and handles, helm steering wheel"},
    {"id": "spyglass",              "category": "props",        "subject": "brass extended spyglass telescope on small stand, sailor optic instrument"},
    {"id": "sextant",               "category": "props",        "subject": "brass nautical sextant navigation instrument, antique sailing tool"},
    {"id": "rolled_map",            "category": "props",        "subject": "rolled parchment treasure map tied with ribbon, antique"},
    {"id": "pirate_flag",           "category": "props",        "subject": "black pirate Jolly Roger flag with white skull and crossbones, on pole"},

    # ============= CAVE / DUNGEON SUB-DETAIL =============
    {"id": "stalactite_stalagmite_column","category":"terrain", "subject": "joined cave column with stalactite from ceiling meeting stalagmite from floor, tall mineral pillar"},
    {"id": "mineral_pool_acid",     "category": "terrain",      "subject": "small round pool of glowing acid green liquid in rocky basin, toxic chemical pool"},
    {"id": "mineral_pool_oil",      "category": "terrain",      "subject": "small round pool of black oil in rocky basin, dark liquid"},
    {"id": "mineral_pool_magma",    "category": "terrain",      "subject": "small round pool of glowing red magma lava in rocky basin, hot liquid"},
    {"id": "rope_bridge_chasm",     "category": "terrain",      "subject": "long wooden plank rope bridge spanning chasm with rope handrails, suspension bridge"},
    {"id": "mine_support_beam",     "category": "props",        "subject": "wooden mine support beam structure with cross-bracing, tunnel reinforcement"},
    {"id": "ore_bucket_chain",      "category": "props",        "subject": "iron ore bucket on heavy chain with hook, mining hauling equipment"},
    {"id": "underground_giant_mushroom","category":"vegetation","subject": "huge glowing magical mushroom with bioluminescent cap, underground cave fungus"},

    # ============= RESOURCE NODES =============
    {"id": "mining_ore_iron",       "category": "terrain",      "subject": "rocky outcrop with sparkling silver iron ore vein, mineable resource"},
    {"id": "mining_ore_copper",     "category": "terrain",      "subject": "rocky outcrop with orange copper ore vein streaks, mineable resource"},
    {"id": "mining_ore_gold",       "category": "terrain",      "subject": "rocky outcrop with bright gold ore vein streaks, mineable treasure"},
    {"id": "herb_gather_bush",      "category": "vegetation",   "subject": "small green medicinal herb bush with healing leaves, gathering plant"},
    {"id": "fishing_spot_marker",   "category": "terrain",      "subject": "small water surface with circular ripples and floating leaves, fishing spot marker"},
    {"id": "treasure_dig_mound",    "category": "terrain",      "subject": "small mound of loose dirt with shovel sticking out, buried treasure marker"},

    # ============= TINY DETAIL PROPS =============
    {"id": "coin_pile_gold",        "category": "props",        "subject": "small pile of shiny gold coins on ground, scattered treasure"},
    {"id": "scattered_gem_pile",    "category": "props",        "subject": "small pile of colorful precious gems scattered, ruby emerald sapphire treasure"},
    {"id": "letter_sealed",         "category": "props",        "subject": "single sealed parchment letter with red wax seal stamp, old correspondence"},
    {"id": "scroll_open_paper",     "category": "props",        "subject": "open unrolled parchment scroll with handwritten text, ancient document"},
    {"id": "quill_inkwell",         "category": "props",        "subject": "feather quill pen in glass inkwell on wooden writing surface, scribe tools"},
    {"id": "easel_painting",        "category": "props",        "subject": "wooden artist easel with canvas painting on it, art studio"},
    {"id": "lute_wooden",           "category": "props",        "subject": "round-backed wooden lute musical instrument with strings, medieval bard"},
    {"id": "harp_small",            "category": "props",        "subject": "small ornate golden harp with strings, classical musical instrument"},
    {"id": "drum_hand",             "category": "props",        "subject": "small wooden hand drum with stretched leather top, percussion instrument"},
    {"id": "flute_wooden",          "category": "props",        "subject": "long wooden flute musical instrument with finger holes, woodwind"},
    {"id": "ocarina",               "category": "props",        "subject": "small clay ocarina musical instrument with finger holes, ancient flute"},
    {"id": "wooden_sword_toy",      "category": "props",        "subject": "small carved wooden toy sword for child, simple play weapon"},
    {"id": "rag_doll",              "category": "props",        "subject": "old fabric rag doll toy with stitched face, child plaything"},
    {"id": "rocking_horse",         "category": "props",        "subject": "wooden rocking horse toy on curved rockers, painted child toy"},
    {"id": "kite",                  "category": "props",        "subject": "diamond-shaped colorful paper kite with tail and string, child toy"},
    {"id": "jewelry_box_ornate",    "category": "props",        "subject": "small ornate wooden jewelry box with brass hinges, treasure container"},
    {"id": "tea_set_porcelain",     "category": "props",        "subject": "white porcelain tea set with teapot and cups on tray, fine china"},
    {"id": "soup_pot_iron",         "category": "props",        "subject": "iron soup cooking pot with wooden ladle inside, kitchen utensil"},
    {"id": "bottle_corked",         "category": "props",        "subject": "single tall green glass bottle with cork, wine or potion bottle"},
    {"id": "broken_bottle",         "category": "props",        "subject": "shattered broken glass bottle with sharp shards, debris"},
    {"id": "rolling_pin",           "category": "props",        "subject": "wooden kitchen rolling pin with handles, baking utensil"},
    {"id": "mortar_pestle_kitchen", "category": "props",        "subject": "stone mortar bowl with pestle stick inside, herb grinding kitchen tool"},

    # ============= TERRAIN ACCENT =============
    {"id": "fallen_log_snapped",    "category": "vegetation",   "subject": "fallen broken tree log with snapped end and exposed wood, weathered, single specimen"},
    {"id": "ground_vine_dense",     "category": "vegetation",   "subject": "dense patch of green ground vines spreading on dirt, climbing plant"},
    {"id": "pine_cone_scatter",     "category": "vegetation",   "subject": "small pile of brown pine cones scattered, woodland ground litter"},
    {"id": "acorn_cluster",         "category": "vegetation",   "subject": "small pile of brown acorns with caps, woodland ground litter"},
    {"id": "leaf_pile_autumn",      "category": "vegetation",   "subject": "pile of orange and red autumn leaves on ground, fall ground litter"},
    {"id": "mushroom_ring_fairy",   "category": "vegetation",   "subject": "circular ring of small white mushrooms in grass, fairy ring patch"},
    {"id": "flower_bed_cluster",    "category": "vegetation",   "subject": "rectangular flower bed with mixed colorful blooms, garden patch"},

    # ============= QUEST / LORE =============
    {"id": "hero_memorial_statue",  "category": "centerpiece",  "subject": "stone memorial statue of heroic warrior on pedestal with name plaque, weathered"},
    {"id": "memorial_plaque",       "category": "props",        "subject": "stone memorial plaque on small pedestal with carved text, dedication marker"},
    {"id": "tomb_marker_text",      "category": "props",        "subject": "single tall tombstone with carved name and dates, weathered gray stone grave marker"},
    {"id": "wishing_tree",          "category": "vegetation",   "subject": "tree with hanging paper prayer ribbons and tags on branches, sacred wishing tree"},
    {"id": "sacred_grove_circle",   "category": "centerpiece",  "subject": "circle of standing stone monoliths in clearing, ancient sacred grove markers"},
    {"id": "boss_arena_pedestal",   "category": "centerpiece",  "subject": "raised circular stone arena pedestal with chains, boss battle platform"},
    {"id": "cursed_obelisk_glowing","category": "centerpiece",  "subject": "tall cracked stone obelisk with glowing red runes, cursed monument"},
    {"id": "ancient_ruin_marker",   "category": "centerpiece",  "subject": "broken ancient stone ruin with statue fragments and column piece, archaeological"},

    # ============= UNDERWATER SUB-DETAIL =============
    {"id": "pearl_oyster_open",     "category": "props",        "subject": "open giant clam shell oyster with white pearl visible inside, underwater treasure"},
    {"id": "giant_clam",            "category": "props",        "subject": "large blue and white giant clam with serrated lips, underwater creature"},
    {"id": "sunken_statue_head",    "category": "props",        "subject": "ancient broken stone statue head lying underwater with kelp, ruined sculpture"},
    {"id": "anchor_rusted_kelp",    "category": "props",        "subject": "rusted iron ship anchor wrapped in green kelp seaweed, underwater debris"},
    {"id": "bioluminescent_jelly",  "category": "props",        "subject": "glowing pink bioluminescent jellyfish floating above stand, decorative marine"},
    {"id": "whale_skeleton_jaw",    "category": "props",        "subject": "huge bleached white whale jaw skeleton with teeth, marine bones"},
    {"id": "kelp_forest_tall",      "category": "vegetation",   "subject": "tall green kelp forest strand with broad fronds, underwater plant column"},

    # ============= ANIMAL HUSBANDRY =============
    {"id": "stable_open_front",     "category": "architecture", "subject": "open-front wooden horse stable building with three stalls and trough, farm structure"},
    {"id": "rabbit_hutch",          "category": "architecture", "subject": "small wooden rabbit hutch with mesh wire door and pitched roof, pet pen"},
    {"id": "fish_pond",             "category": "terrain",      "subject": "small round fish pond with stone rim and lily pads, garden water feature"},

    # ============= PVP / WoE =============
    {"id": "ctf_flagpole",          "category": "props",        "subject": "tall flagpole with hanging team-colored capture flag, PVP marker"},
    {"id": "capture_point_base",    "category": "centerpiece",  "subject": "raised circular capture point platform with magical rune circle, PVP objective"},
    {"id": "siege_ladder",          "category": "props",        "subject": "tall wooden siege ladder leaning against wall, castle assault equipment"},
    {"id": "defensive_spike_wall",  "category": "props",        "subject": "wooden defensive spike wall with pointed wooden stakes, military fortification"},
    {"id": "caltrops_pile",         "category": "props",        "subject": "small pile of iron caltrop spikes scattered on ground, anti-cavalry"},
    {"id": "destructible_barricade","category": "props",        "subject": "wooden defensive barricade wall section with planks and spikes, breakable, intact"},
    {"id": "destructible_barricade_broken","category":"props",  "subject": "smashed broken wooden barricade with splintered planks scattered, destroyed defense"},
    {"id": "resurrection_altar",    "category": "centerpiece",  "subject": "small stone altar with glowing blue magical orb, respawn point, mystical"},

    # ============= EDUCATIONAL =============
    {"id": "constellation_map",     "category": "props",        "subject": "ornate brass constellation star map on wooden frame, astronomical chart"},
    {"id": "dimensional_rift_anchor","category": "centerpiece", "subject": "stone pillar with swirling purple magical rift portal floating above, dimensional gate"},
    {"id": "class_master_pedestal", "category": "centerpiece",  "subject": "ornate stone pedestal with class symbol carved on top, instructor podium"},

    # ============= HOLIDAY =============
    {"id": "lunar_lantern_red",     "category": "props",        "subject": "round red Chinese lunar new year lantern with golden tassels and calligraphy, festival"},
    {"id": "dragon_parade_segment", "category": "props",        "subject": "ornate Chinese dragon parade puppet section with red and gold scales, festival"},
    {"id": "firecracker_string",    "category": "props",        "subject": "string of red Chinese firecrackers tied together hanging, festival"},
    {"id": "flower_crown",          "category": "props",        "subject": "ornate flower crown wreath with mixed colorful blooms, festival headpiece"},
    {"id": "pumpkin_patch_section", "category": "vegetation",   "subject": "small section of pumpkin patch with several round orange pumpkins on vines, harvest"},
    {"id": "hay_bale_maze",         "category": "props",        "subject": "section of hay bale wall with stacked rectangular bales, harvest maze"},
    {"id": "witches_broom",         "category": "props",        "subject": "wooden witches broom with twig bristles leaning, Halloween prop"},
    {"id": "valentine_heart_arch",  "category": "architecture", "subject": "decorative red heart-shaped arch covered in pink roses, romantic celebration"},
]


# ============================================================================
# V3 VARIANTS — 3-4 per asset, ~1100 total
# ============================================================================
V3_VARIANTS = {
    # PRONTERA / IZLUDE
    "church_pew":            WOOD_PALETTE,
    "altar_christian":       STONE_PALETTE,
    "pulpit_carved":         WOOD_PALETTE,
    "pipe_organ":            [{"suffix":"polished_brass","prompt":"polished brass pipes"},
                              {"suffix":"silver_pipes","prompt":"silver toned pipes"},
                              {"suffix":"weathered","prompt":"weathered tarnished bronze pipes"}],
    "stained_glass_window":  [{"suffix":"red_jesus","prompt":"red and blue Christ stained glass scene"},
                              {"suffix":"green_tree","prompt":"green tree of life pattern"},
                              {"suffix":"blue_mary","prompt":"blue Virgin Mary scene"},
                              {"suffix":"gold_geometric","prompt":"gold geometric pattern"}],
    "candelabra_floor":      IRON_PALETTE,
    "royal_throne":          [{"suffix":"red_velvet","prompt":"red velvet cushion with gold trim"},
                              {"suffix":"blue_silk","prompt":"royal blue silk cushion with silver trim"},
                              {"suffix":"purple_imperial","prompt":"imperial purple cushion with gold"},
                              {"suffix":"green_emerald","prompt":"emerald green cushion with bronze"}],
    "carpet_runner_red":     FABRIC_PALETTE,
    "knight_statue":         STONE_PALETTE,
    "topiary_cone":          [{"suffix":"green","prompt":"vibrant green foliage"},
                              {"suffix":"autumn","prompt":"autumn red foliage"},
                              {"suffix":"frosted","prompt":"frosted white winter foliage"}],
    "topiary_sphere":        [{"suffix":"green","prompt":"vibrant green foliage"},
                              {"suffix":"flowering","prompt":"with white flowers"},
                              {"suffix":"autumn","prompt":"autumn red foliage"}],
    "hedge_maze_section":    [{"suffix":"green","prompt":"vibrant green hedge"},
                              {"suffix":"flowering","prompt":"hedge with white flowers"},
                              {"suffix":"autumn","prompt":"autumn red hedge"}],
    "ornamental_urn":        STONE_PALETTE,
    "sundial":               STONE_PALETTE,
    "gargoyle_statue":       STONE_PALETTE,
    "balustrade_section":    STONE_PALETTE,
    "aqueduct_arch":         STONE_PALETTE,

    # GEFFEN
    "magic_circle_floor":    [{"suffix":"blue","prompt":"glowing blue runes"},
                              {"suffix":"red","prompt":"glowing red runes"},
                              {"suffix":"green","prompt":"glowing green runes"},
                              {"suffix":"purple","prompt":"glowing purple runes"}],
    "floating_crystal_cluster": [{"suffix":"blue","prompt":"glowing blue crystals"},
                                  {"suffix":"purple","prompt":"glowing purple crystals"},
                                  {"suffix":"green","prompt":"glowing green crystals"},
                                  {"suffix":"red","prompt":"glowing red crystals"}],
    "alchemy_glassware":     [{"suffix":"blue_liquid","prompt":"blue glowing liquid in glass"},
                              {"suffix":"green_liquid","prompt":"green liquid in glass"},
                              {"suffix":"red_liquid","prompt":"red glowing liquid in glass"}],
    "magic_tome_open":       [{"suffix":"blue_glow","prompt":"glowing blue magical text"},
                              {"suffix":"red_glow","prompt":"glowing red magical text"},
                              {"suffix":"gold_glow","prompt":"glowing gold magical text"}],
    "magic_tome_closed":     [{"suffix":"red_leather","prompt":"red leather binding"},
                              {"suffix":"blue_leather","prompt":"blue leather binding"},
                              {"suffix":"black_leather","prompt":"black leather binding"},
                              {"suffix":"green_leather","prompt":"green leather binding"}],
    "scroll_pile":           WOOD_PALETTE,
    "cauldron_bubbling":     [{"suffix":"green","prompt":"bubbling green liquid"},
                              {"suffix":"purple","prompt":"bubbling purple liquid"},
                              {"suffix":"red","prompt":"bubbling red liquid"},
                              {"suffix":"blue","prompt":"bubbling blue liquid"}],
    "runestone_engraved":    STONE_PALETTE,
    "runestone_glowing":     [{"suffix":"blue","prompt":"glowing blue runes"},
                              {"suffix":"red","prompt":"glowing red runes"},
                              {"suffix":"green","prompt":"glowing green runes"},
                              {"suffix":"purple","prompt":"glowing purple runes"}],
    "wizard_tower_top":      STONE_PALETTE,

    # PAYON / ASIAN
    "pagoda_3tier":          [{"suffix":"red_lacquer","prompt":"red lacquered wood with gold trim"},
                              {"suffix":"green_jade","prompt":"green jade colored with gold"},
                              {"suffix":"natural_wood","prompt":"natural unfinished wood"}],
    "pagoda_5tier":          [{"suffix":"red_lacquer","prompt":"red lacquered with gold accents"},
                              {"suffix":"green_jade","prompt":"green jade colored"},
                              {"suffix":"weathered","prompt":"weathered aged temple"}],
    "torii_gate":            [{"suffix":"red","prompt":"vermilion red lacquer"},
                              {"suffix":"black","prompt":"black lacquer"},
                              {"suffix":"natural","prompt":"natural unfinished cedar wood"}],
    "stone_lantern_toro":    STONE_PALETTE,
    "paper_lantern_round":   [{"suffix":"red","prompt":"red paper"},
                              {"suffix":"white","prompt":"white paper with calligraphy"},
                              {"suffix":"yellow","prompt":"yellow paper"},
                              {"suffix":"blue","prompt":"blue paper"}],
    "paper_lantern_cylinder":[{"suffix":"white","prompt":"white paper"},
                              {"suffix":"red","prompt":"red paper"},
                              {"suffix":"yellow","prompt":"yellow paper with kanji"}],
    "koi_pond":              [{"suffix":"orange_koi","prompt":"orange and white koi"},
                              {"suffix":"red_koi","prompt":"red and white koi"},
                              {"suffix":"black_koi","prompt":"black and white koi"}],
    "shishi_odoshi":         WOOD_PALETTE,
    "dol_hareubang":         STONE_PALETTE,
    "fu_dog_statue":         STONE_PALETTE,
    "drying_rack_fish":      WOOD_PALETTE,
    "tea_set":               [{"suffix":"black_iron","prompt":"black cast iron"},
                              {"suffix":"porcelain","prompt":"white porcelain with blue pattern"},
                              {"suffix":"red_clay","prompt":"red clay terracotta"}],
    "bamboo_water_pipe":     WOOD_PALETTE,

    # MORROC / SOGRAT
    "sphinx_statue":         STONE_PALETTE,
    "obelisk_egyptian":      STONE_PALETTE,
    "pyramid_block":         STONE_PALETTE,
    "hieroglyphic_panel":    STONE_PALETTE,
    "sand_dune_decorative":  [{"suffix":"golden","prompt":"golden yellow sand"},
                              {"suffix":"red_desert","prompt":"red desert sand"},
                              {"suffix":"white_dune","prompt":"white sand"}],
    "hitching_rail_wooden":  WOOD_PALETTE,
    "carpet_persian":        [{"suffix":"red_blue","prompt":"red and blue patterns"},
                              {"suffix":"gold_burgundy","prompt":"gold and burgundy patterns"},
                              {"suffix":"green_cream","prompt":"green and cream patterns"},
                              {"suffix":"purple_silver","prompt":"purple and silver patterns"}],
    "floor_cushion":         FABRIC_PALETTE,
    "low_table_arabic":      WOOD_PALETTE,
    "hookah":                [{"suffix":"brass","prompt":"polished brass body"},
                              {"suffix":"copper","prompt":"copper body with patina"},
                              {"suffix":"silver","prompt":"silver toned body"}],
    "cracked_sandstone_column":STONE_PALETTE,

    # COMODO
    "carnival_tent_striped": [{"suffix":"red_white","prompt":"red and white stripes"},
                              {"suffix":"blue_yellow","prompt":"blue and yellow stripes"},
                              {"suffix":"purple_gold","prompt":"purple and gold stripes"}],
    "stage_platform_wood":   WOOD_PALETTE,
    "performer_pole":        [{"suffix":"red_white","prompt":"red and white striped"},
                              {"suffix":"blue_yellow","prompt":"blue and yellow striped"}],
    "beach_umbrella":        [{"suffix":"red_white","prompt":"red and white stripes"},
                              {"suffix":"yellow_blue","prompt":"yellow and blue stripes"},
                              {"suffix":"rainbow","prompt":"rainbow stripes"}],
    "deck_chair":            [{"suffix":"red_white","prompt":"red and white striped fabric"},
                              {"suffix":"blue_white","prompt":"blue and white striped fabric"},
                              {"suffix":"natural","prompt":"natural canvas"}],
    "driftwood_log":         [{"suffix":"weathered_gray","prompt":"weathered gray driftwood"},
                              {"suffix":"sun_bleached","prompt":"sun bleached white driftwood"},
                              {"suffix":"dark_aged","prompt":"dark aged driftwood"}],
    "seashell_conch":        [{"suffix":"pink_white","prompt":"pink and white"},
                              {"suffix":"orange_brown","prompt":"orange and brown"},
                              {"suffix":"pearl_white","prompt":"pearlescent white"}],
    "starfish":              [{"suffix":"orange","prompt":"bright orange"},
                              {"suffix":"red","prompt":"deep red"},
                              {"suffix":"purple","prompt":"purple"}],
    "sand_pile_beach":       [{"suffix":"golden","prompt":"golden tan sand"},
                              {"suffix":"white","prompt":"white sand"},
                              {"suffix":"black","prompt":"black volcanic sand"}],
    "lighthouse_small":      [{"suffix":"red_top","prompt":"white with red top"},
                              {"suffix":"blue_top","prompt":"white with blue top"},
                              {"suffix":"weathered","prompt":"weathered gray with rust accents"}],
    "ship_anchor":           IRON_PALETTE,
    "rope_coil":             WOOD_PALETTE,
    "buoy_floating":         [{"suffix":"red_white","prompt":"red and white"},
                              {"suffix":"yellow","prompt":"bright yellow"},
                              {"suffix":"orange","prompt":"orange"}],
    "fishing_net_drying":    WOOD_PALETTE,
    "crab_trap":             WOOD_PALETTE,
    "fish_market_crate":     WOOD_PALETTE,

    # GLAST HEIM
    "broken_throne":         STONE_PALETTE,
    "smashed_glass_pile":    [{"suffix":"red_blue","prompt":"red and blue glass shards"},
                              {"suffix":"green_gold","prompt":"green and gold glass shards"},
                              {"suffix":"clear","prompt":"clear glass shards"}],
    "tattered_banner":       FABRIC_PALETTE,
    "iron_maiden":           IRON_PALETTE,
    "torture_rack":          WOOD_PALETTE,
    "fallen_knight_armor":   IRON_PALETTE,
    "crumbling_buttress":    STONE_PALETTE,

    # NIFLHEIM
    "jackolantern_small":    [{"suffix":"orange","prompt":"orange pumpkin"},
                              {"suffix":"white","prompt":"white pumpkin"},
                              {"suffix":"black","prompt":"black painted pumpkin"}],
    "jackolantern_large":    [{"suffix":"orange","prompt":"orange pumpkin with bright glow"},
                              {"suffix":"green","prompt":"green pumpkin with green glow"},
                              {"suffix":"black_red","prompt":"black pumpkin with red glow"}],
    "will_o_wisp_post":      [{"suffix":"blue","prompt":"glowing ghostly blue flame"},
                              {"suffix":"green","prompt":"glowing green flame"},
                              {"suffix":"purple","prompt":"glowing purple flame"}],
    "wicker_scarecrow_creepy":WOOD_PALETTE,
    "iron_fence_skull":      IRON_PALETTE,
    "soul_candle":           [{"suffix":"blue","prompt":"blue ethereal flame"},
                              {"suffix":"white","prompt":"pure white flame"},
                              {"suffix":"green","prompt":"sickly green flame"}],

    # LUTIE
    "snowman":               [{"suffix":"classic","prompt":"with red scarf and orange carrot nose"},
                              {"suffix":"top_hat","prompt":"with black top hat and corn cob pipe"},
                              {"suffix":"cute","prompt":"with pink scarf and button eyes"}],
    "snowdrift_modular":     [{"suffix":"fresh","prompt":"fresh white snow"},
                              {"suffix":"frosted","prompt":"icy frosted snow"},
                              {"suffix":"crusted","prompt":"crusted aged snow"}],
    "snow_capped_pine":      [{"suffix":"heavy_snow","prompt":"heavy snow caps"},
                              {"suffix":"light_snow","prompt":"light snow dusting"},
                              {"suffix":"frosted","prompt":"frosted icy needles"}],
    "ice_crystal_formation": [{"suffix":"blue","prompt":"pale blue ice"},
                              {"suffix":"white","prompt":"white frosted ice"},
                              {"suffix":"clear","prompt":"clear transparent ice"}],
    "frozen_pond_disc":      [{"suffix":"blue","prompt":"pale blue ice surface"},
                              {"suffix":"clear","prompt":"clear transparent ice"},
                              {"suffix":"frosted","prompt":"frosted white ice"}],
    "christmas_wreath":      [{"suffix":"red_bow","prompt":"green pine with red bow"},
                              {"suffix":"gold_bow","prompt":"green pine with gold bow"},
                              {"suffix":"silver_bow","prompt":"green pine with silver bow"}],
    "christmas_garland":     [{"suffix":"red_ribbon","prompt":"green pine with red ribbons"},
                              {"suffix":"gold_ribbon","prompt":"green pine with gold ribbons"},
                              {"suffix":"silver_ribbon","prompt":"green pine with silver ribbons"}],
    "gift_box":              [{"suffix":"red","prompt":"red wrapping with gold bow"},
                              {"suffix":"blue","prompt":"blue wrapping with silver bow"},
                              {"suffix":"green","prompt":"green wrapping with red bow"},
                              {"suffix":"gold","prompt":"gold wrapping with white bow"}],
    "candy_cane_giant":      [{"suffix":"red_white","prompt":"red and white stripes"},
                              {"suffix":"green_white","prompt":"green and white stripes"},
                              {"suffix":"blue_white","prompt":"blue and white stripes"}],
    "ice_slide":             [{"suffix":"blue","prompt":"pale blue ice"},
                              {"suffix":"clear","prompt":"clear transparent ice"}],

    # HUGEL / RURAL
    "windmill_dutch":        [{"suffix":"white_walls","prompt":"whitewashed walls"},
                              {"suffix":"red_brick","prompt":"red brick walls"},
                              {"suffix":"weathered","prompt":"weathered gray walls"}],
    "watermill":             WOOD_PALETTE,
    "round_hay_bale":        [{"suffix":"golden","prompt":"golden dried hay"},
                              {"suffix":"weathered","prompt":"weathered gray hay"},
                              {"suffix":"fresh_green","prompt":"fresh green hay"}],
    "wheat_crop_section":    [{"suffix":"golden_ripe","prompt":"golden ripe wheat"},
                              {"suffix":"green_unripe","prompt":"green unripe wheat"}],
    "corn_crop_section":     [{"suffix":"yellow_ripe","prompt":"yellow ripe corn"},
                              {"suffix":"green_unripe","prompt":"green unripe corn"}],
    "chicken_coop":          WOOD_PALETTE,
    "pig_sty":               WOOD_PALETTE,
    "feed_trough":           WOOD_PALETTE,
    "beehive_box":           [{"suffix":"white","prompt":"white painted"},
                              {"suffix":"natural","prompt":"natural wood"},
                              {"suffix":"red","prompt":"red painted"}],
    "birdcage_ornate":       [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"black_iron","prompt":"black iron"},
                              {"suffix":"white_painted","prompt":"white painted ornate"}],

    # JUNO
    "airship_dock_platform": WOOD_PALETTE,
    "mooring_post":          WOOD_PALETTE,
    "floating_islet":        [{"suffix":"green_grass","prompt":"green grass top"},
                              {"suffix":"flowery","prompt":"with flowers"},
                              {"suffix":"snowy","prompt":"with snow cap"}],
    "brass_cogwheel":        [{"suffix":"polished_brass","prompt":"polished brass"},
                              {"suffix":"copper","prompt":"copper"},
                              {"suffix":"rusted","prompt":"rusted iron"}],
    "telescope_brass":       [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"silver","prompt":"silver toned"},
                              {"suffix":"black_lacquer","prompt":"black lacquered with brass trim"}],
    "astrolabe":             [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"silver","prompt":"silver"},
                              {"suffix":"gold","prompt":"gold"}],
    "globe_stand":           WOOD_PALETTE,

    # EINBROCH
    "mine_cart":             IRON_PALETTE,
    "rail_track_segment":    IRON_PALETTE,
    "coal_pile":             [{"suffix":"black","prompt":"shiny black coal"},
                              {"suffix":"dusty","prompt":"dusty coal pile"}],
    "slag_heap":             [{"suffix":"dark_gray","prompt":"dark gray slag"},
                              {"suffix":"red_brown","prompt":"red brown rusty slag"}],
    "ore_vein":              [{"suffix":"silver","prompt":"sparkling silver ore"},
                              {"suffix":"gold","prompt":"sparkling gold ore"},
                              {"suffix":"copper","prompt":"orange copper ore"},
                              {"suffix":"iron","prompt":"gray iron ore"}],
    "smokestack_chimney":    [{"suffix":"red_brick","prompt":"red brick"},
                              {"suffix":"sooty","prompt":"black sooty brick"},
                              {"suffix":"weathered","prompt":"weathered aged brick"}],
    "factory_pipe":          IRON_PALETTE,
    "steel_girder":          [{"suffix":"new_steel","prompt":"new gray steel"},
                              {"suffix":"rusted","prompt":"rusted brown"},
                              {"suffix":"painted_red","prompt":"painted red"}],
    "riveted_panel":         IRON_PALETTE,
    "bellows_industrial":    WOOD_PALETTE,
    "smithing_hammer_display":WOOD_PALETTE,

    # THOR / MAGMA
    "lava_pool_surface":     [{"suffix":"red_orange","prompt":"glowing red orange lava"},
                              {"suffix":"yellow_hot","prompt":"yellow hot bright lava"},
                              {"suffix":"cooling","prompt":"cooling crusty dark red lava"}],
    "magma_rock_emissive":   [{"suffix":"red_glow","prompt":"red glowing cracks"},
                              {"suffix":"orange_glow","prompt":"orange glowing cracks"},
                              {"suffix":"yellow_glow","prompt":"yellow hot glowing cracks"}],
    "volcanic_vent":         STONE_PALETTE,
    "obsidian_shard":        [{"suffix":"black","prompt":"glossy black obsidian"},
                              {"suffix":"dark_purple","prompt":"dark purple obsidian"}],
    "sulfur_deposit":        [{"suffix":"yellow","prompt":"bright yellow sulfur"},
                              {"suffix":"orange","prompt":"orange sulfur"}],

    # BYALAN / UNDERWATER
    "coral_brain":           [{"suffix":"pink","prompt":"pink coral"},
                              {"suffix":"orange","prompt":"orange coral"},
                              {"suffix":"green","prompt":"green coral"}],
    "coral_fan":             [{"suffix":"purple","prompt":"purple sea fan"},
                              {"suffix":"red","prompt":"red sea fan"},
                              {"suffix":"yellow","prompt":"yellow sea fan"}],
    "coral_tube":            [{"suffix":"orange","prompt":"orange tubes"},
                              {"suffix":"red","prompt":"red tubes"},
                              {"suffix":"yellow","prompt":"yellow tubes"}],
    "sea_anemone":           [{"suffix":"pink","prompt":"pink anemone"},
                              {"suffix":"green","prompt":"green anemone"},
                              {"suffix":"purple","prompt":"purple anemone"}],
    "kelp_strand":           [{"suffix":"green","prompt":"vibrant green kelp"},
                              {"suffix":"dark_green","prompt":"dark green kelp"},
                              {"suffix":"brown","prompt":"brown kelp"}],
    "sunken_ship_prow":      WOOD_PALETTE,

    # CAVES
    "stalactite_small":      STONE_PALETTE,
    "stalactite_medium":     STONE_PALETTE,
    "stalactite_large":      STONE_PALETTE,
    "glow_mushroom":         [{"suffix":"cyan","prompt":"glowing cyan luminous"},
                              {"suffix":"purple","prompt":"glowing purple luminous"},
                              {"suffix":"green","prompt":"glowing green luminous"},
                              {"suffix":"pink","prompt":"glowing pink luminous"}],
    "mineral_vein_gold":     [{"suffix":"gold","prompt":"bright gold ore"},
                              {"suffix":"silver","prompt":"sparkling silver ore"},
                              {"suffix":"copper","prompt":"orange copper ore"}],

    # INTERIOR
    "bed_peasant_straw":     WOOD_PALETTE,
    "bed_noble_fourposter":  [{"suffix":"red_velvet","prompt":"red velvet canopy and pillars"},
                              {"suffix":"blue_silk","prompt":"royal blue silk canopy"},
                              {"suffix":"purple_imperial","prompt":"imperial purple canopy"},
                              {"suffix":"green","prompt":"emerald green canopy"}],
    "bed_inn_bunk":          WOOD_PALETTE,
    "chair_armchair":        FABRIC_PALETTE,
    "table_dining":          WOOD_PALETTE,
    "wardrobe_wooden":       WOOD_PALETTE,
    "dresser_drawers":       WOOD_PALETTE,
    "chest_clothing":        WOOD_PALETTE,
    "fireplace_stone":       STONE_PALETTE,
    "cooking_pot_fire":      [{"suffix":"black_iron","prompt":"black iron pot"},
                              {"suffix":"copper","prompt":"copper pot"}],
    "rug_oriental":          [{"suffix":"red_blue","prompt":"red and blue patterns"},
                              {"suffix":"gold_burgundy","prompt":"gold and burgundy"},
                              {"suffix":"green_cream","prompt":"green and cream"}],
    "rug_fur":               [{"suffix":"white_polar","prompt":"white polar bear fur"},
                              {"suffix":"brown_bear","prompt":"brown bear fur"},
                              {"suffix":"black_wolf","prompt":"black wolf fur"}],
    "picture_frame_oil":     [{"suffix":"portrait","prompt":"classical portrait painting"},
                              {"suffix":"landscape","prompt":"landscape painting"},
                              {"suffix":"still_life","prompt":"still life painting"}],
    "mirror_wall_oval":      [{"suffix":"gold","prompt":"ornate gold frame"},
                              {"suffix":"silver","prompt":"silver frame"},
                              {"suffix":"black","prompt":"black lacquer frame"}],
    "wall_sconce_iron":      IRON_PALETTE,
    "bar_counter":           WOOD_PALETTE,
    "beer_keg_wooden":       WOOD_PALETTE,
    "stage_platform_bard":   WOOD_PALETTE,

    # NPC WORKSTATIONS
    "forge_blacksmith":      STONE_PALETTE,
    "tailor_mannequin":      [{"suffix":"red_dress","prompt":"with red gown"},
                              {"suffix":"blue_dress","prompt":"with blue gown"},
                              {"suffix":"white_dress","prompt":"with white wedding gown"},
                              {"suffix":"green_dress","prompt":"with green gown"}],
    "sewing_table":          WOOD_PALETTE,
    "fabric_bolt":           [{"suffix":"red","prompt":"red fabric"},
                              {"suffix":"blue","prompt":"blue fabric"},
                              {"suffix":"green","prompt":"green fabric"},
                              {"suffix":"gold","prompt":"gold patterned fabric"}],
    "apothecary_shelf":      WOOD_PALETTE,
    "coin_scale":            [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"silver","prompt":"silver toned"},
                              {"suffix":"black_iron","prompt":"black iron"}],
    "ledger_book":           [{"suffix":"red_leather","prompt":"red leather"},
                              {"suffix":"black_leather","prompt":"black leather"},
                              {"suffix":"brown_leather","prompt":"brown leather"}],
    "abacus":                WOOD_PALETTE,
    "iron_storage_vault":    IRON_PALETTE,

    # RELIGIOUS
    "altar_norse":           STONE_PALETTE,
    "incense_burner_brass":  [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"copper","prompt":"copper"},
                              {"suffix":"silver","prompt":"silver"}],
    "odin_cross":            STONE_PALETTE,
    "asian_shrine_offering": WOOD_PALETTE,
    "reliquary_ornate":      [{"suffix":"gold","prompt":"polished gold"},
                              {"suffix":"silver","prompt":"silver"},
                              {"suffix":"jeweled","prompt":"gold with jewel inlay"}],
    "offering_bowl_stone":   STONE_PALETTE,
    "prayer_cairn":          STONE_PALETTE,

    # PATH & ROAD
    "cobblestone_road_segment":[{"suffix":"gray","prompt":"gray cobblestones"},
                                 {"suffix":"red_brick","prompt":"red brick cobbles"},
                                 {"suffix":"sandstone","prompt":"tan sandstone cobbles"}],
    "dirt_path_tile":        [{"suffix":"brown","prompt":"brown earth"},
                              {"suffix":"red_clay","prompt":"red clay"},
                              {"suffix":"sandy","prompt":"sandy yellow"}],
    "stepping_stone":        STONE_PALETTE,
    "milepost":              WOOD_PALETTE,
    "direction_sign_wood":   WOOD_PALETTE,
    "boardwalk_section":     WOOD_PALETTE,
    "stone_arch_road_gate":  STONE_PALETTE,

    # COMBAT / TRAINING
    "training_dummy_straw":  WOOD_PALETTE,
    "training_dummy_armor":  IRON_PALETTE,
    "archery_target_round":  [{"suffix":"red_white","prompt":"red and white target rings"},
                              {"suffix":"black_yellow","prompt":"black and yellow target"},
                              {"suffix":"blue_red","prompt":"blue and red target"}],
    "archery_target_haybale":[{"suffix":"red_target","prompt":"red painted target"},
                              {"suffix":"black_target","prompt":"black painted target"}],
    "sparring_ring_rope":    WOOD_PALETTE,

    # WATER FEATURES
    "waterfall_small_full":  [{"suffix":"blue_water","prompt":"clear blue water"},
                              {"suffix":"misty","prompt":"misty white water"}],
    "waterfall_medium_full": [{"suffix":"blue_water","prompt":"clear blue water"},
                              {"suffix":"misty","prompt":"misty white water"},
                              {"suffix":"twilight_blue","prompt":"twilight blue water"}],
    "stream_with_rocks":     [{"suffix":"clear","prompt":"clear blue water"},
                              {"suffix":"murky","prompt":"murky brown water"}],
    "irrigation_channel":    STONE_PALETTE,
    "hot_spring_pool":       [{"suffix":"steamy_blue","prompt":"steamy blue water"},
                              {"suffix":"green","prompt":"emerald green water"},
                              {"suffix":"clear","prompt":"clear water with steam"}],

    # HOLIDAY / SEASONAL
    "festival_lantern_string":[{"suffix":"red_yellow","prompt":"red and yellow"},
                                {"suffix":"rainbow","prompt":"rainbow mixed colors"},
                                {"suffix":"warm_white","prompt":"warm white"}],
    "maypole_decorative":    [{"suffix":"rainbow","prompt":"rainbow ribbons"},
                              {"suffix":"red_white","prompt":"red and white ribbons"},
                              {"suffix":"blue_yellow","prompt":"blue and yellow ribbons"}],
    "fair_stage_small":      WOOD_PALETTE,
    "prize_booth":           WOOD_PALETTE,
    "harvest_cornucopia":    [{"suffix":"autumn","prompt":"autumn fruits and pumpkins"},
                              {"suffix":"summer","prompt":"summer fruits and flowers"}],

    # MISC RO
    "save_point_obelisk":    [{"suffix":"blue","prompt":"glowing blue crystal"},
                              {"suffix":"green","prompt":"glowing green crystal"},
                              {"suffix":"purple","prompt":"glowing purple crystal"},
                              {"suffix":"gold","prompt":"glowing gold crystal"}],
    "casino_roulette":       [{"suffix":"red_black","prompt":"red and black"},
                              {"suffix":"luxury","prompt":"luxury gold trim"}],
    "casino_dice_table":     [{"suffix":"green_felt","prompt":"green felt"},
                              {"suffix":"red_felt","prompt":"red felt"},
                              {"suffix":"blue_felt","prompt":"blue felt"}],
    "poker_chip_stack":      COLOR4,
    "guardian_pedestal":     STONE_PALETTE,
    "woe_breakable_barricade":WOOD_PALETTE,

    # UMBALA
    "treehouse_platform_small":WOOD_PALETTE,
    "treehouse_platform_large":WOOD_PALETTE,
    "rope_bridge_jungle":    WOOD_PALETTE,
    "wooden_ladder_climbing":WOOD_PALETTE,
    "tribal_totem_feathered":[{"suffix":"red_feathers","prompt":"red feathered top"},
                              {"suffix":"blue_feathers","prompt":"blue feathered top"},
                              {"suffix":"black_feathers","prompt":"black feathered top"}],
    "war_drum_tribal":       [{"suffix":"red","prompt":"red painted"},
                              {"suffix":"black","prompt":"black painted"},
                              {"suffix":"natural","prompt":"natural wood"}],
    "bone_fetish":           [{"suffix":"white_bone","prompt":"bleached white bones"},
                              {"suffix":"painted_red","prompt":"red painted bones"},
                              {"suffix":"weathered","prompt":"weathered yellowed bones"}],
    "vine_curtain":          [{"suffix":"green","prompt":"vibrant green vines"},
                              {"suffix":"flowering","prompt":"vines with flowers"},
                              {"suffix":"dry_brown","prompt":"dry brown vines"}],
    "oversized_leaf_awning": [{"suffix":"green","prompt":"vibrant green leaf"},
                              {"suffix":"yellow","prompt":"yellow leaf"},
                              {"suffix":"red","prompt":"red leaf"}],
    "macaw_perch":           WOOD_PALETTE,

    # MOSCOVIA
    "onion_dome_chapel":     [{"suffix":"colorful","prompt":"red green and gold striped dome"},
                              {"suffix":"blue_gold","prompt":"blue and gold dome"},
                              {"suffix":"green_white","prompt":"green and white dome"}],
    "painted_wooden_church": [{"suffix":"blue_white","prompt":"blue and white painted"},
                              {"suffix":"red_gold","prompt":"red and gold painted"},
                              {"suffix":"green_red","prompt":"green and red painted"}],
    "matryoshka_doll":       [{"suffix":"red_blue","prompt":"red and blue painted"},
                              {"suffix":"green_red","prompt":"green and red painted"},
                              {"suffix":"yellow_blue","prompt":"yellow and blue painted"}],
    "samovar":               [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"copper","prompt":"copper"},
                              {"suffix":"silver","prompt":"silver"}],
    "carved_wooden_gateway": WOOD_PALETTE,

    # RACHEL / VEINS
    "white_marble_temple":   STONE_PALETTE,
    "goddess_pedestal_statue":STONE_PALETTE,
    "sacred_brazier":        [{"suffix":"bronze","prompt":"polished bronze"},
                              {"suffix":"gold","prompt":"gold"},
                              {"suffix":"silver","prompt":"silver"}],
    "sand_buried_ruin":      STONE_PALETTE,
    "eroded_statue_head":    STONE_PALETTE,
    "religious_altar_sun":   STONE_PALETTE,

    # ECLAGE
    "giant_walkable_flower": [{"suffix":"purple","prompt":"giant purple flower"},
                              {"suffix":"pink","prompt":"giant pink flower"},
                              {"suffix":"blue","prompt":"giant blue flower"},
                              {"suffix":"yellow","prompt":"giant yellow flower"}],
    "mushroom_house":        [{"suffix":"red_cap","prompt":"red cap with white spots"},
                              {"suffix":"blue_cap","prompt":"blue magical cap"},
                              {"suffix":"purple_cap","prompt":"purple cap"}],
    "fairy_door_tree_base":  WOOD_PALETTE,
    "glowing_root_system":   [{"suffix":"blue","prompt":"glowing blue veins"},
                              {"suffix":"green","prompt":"glowing green veins"},
                              {"suffix":"purple","prompt":"glowing purple veins"}],
    "fairy_ring_stones":     STONE_PALETTE,
    "silver_thread_web":     [{"suffix":"silver","prompt":"shimmering silver"},
                              {"suffix":"gold","prompt":"shimmering gold"}],
    "hovering_moss_platform":[{"suffix":"green","prompt":"vibrant green moss"},
                              {"suffix":"blue_glow","prompt":"with blue magical glow"}],

    # BRASILIS / DEWATA / MALAYA
    "stilt_house_bahay_kubo":WOOD_PALETTE,
    "bamboo_ladder_tropical":WOOD_PALETTE,
    "banana_cluster":        [{"suffix":"yellow_ripe","prompt":"yellow ripe bananas"},
                              {"suffix":"green_unripe","prompt":"green unripe bananas"},
                              {"suffix":"red","prompt":"red bananas"}],
    "mango_cluster":         [{"suffix":"red_yellow","prompt":"red and yellow ripe mangoes"},
                              {"suffix":"green","prompt":"green unripe mangoes"}],
    "coconut_cluster":       [{"suffix":"brown","prompt":"brown ripe coconuts"},
                              {"suffix":"green","prompt":"green young coconuts"}],
    "carnival_drum_tribal":  [{"suffix":"red","prompt":"red painted designs"},
                              {"suffix":"blue","prompt":"blue painted designs"},
                              {"suffix":"yellow","prompt":"yellow painted designs"}],
    "festival_mask":         [{"suffix":"colorful","prompt":"red yellow blue painted"},
                              {"suffix":"gold","prompt":"gold mask"},
                              {"suffix":"black_red","prompt":"black and red mask"}],
    "tribal_shield":         WOOD_PALETTE,
    "outrigger_canoe":       WOOD_PALETTE,
    "bamboo_torch_palm":     WOOD_PALETTE,

    # OLD GLAST HEIM / GEFFENIA
    "demon_ritual_pentagram":[{"suffix":"red","prompt":"glowing red runes"},
                              {"suffix":"black","prompt":"dark black ritual marks"},
                              {"suffix":"purple","prompt":"glowing purple runes"}],
    "sacrificial_altar_chains":STONE_PALETTE,
    "hellfire_brazier":      [{"suffix":"dark_red","prompt":"dark red hellfire"},
                              {"suffix":"black_purple","prompt":"black and purple flames"},
                              {"suffix":"green_sickly","prompt":"sickly green flames"}],
    "skull_pile_large":      [{"suffix":"white","prompt":"bleached white skulls"},
                              {"suffix":"yellowed","prompt":"yellowed aged skulls"},
                              {"suffix":"charred","prompt":"charred black skulls"}],
    "possessed_statue":      [{"suffix":"red_eyes","prompt":"glowing red demonic eyes"},
                              {"suffix":"green_eyes","prompt":"glowing green eyes"},
                              {"suffix":"purple_eyes","prompt":"glowing purple eyes"}],

    # JAWAII
    "heart_wedding_arch":    [{"suffix":"pink_roses","prompt":"with pink roses"},
                              {"suffix":"red_roses","prompt":"with red roses"},
                              {"suffix":"white_lilies","prompt":"with white lilies"}],
    "rose_petal_pile":       [{"suffix":"red","prompt":"red rose petals"},
                              {"suffix":"pink","prompt":"pink rose petals"},
                              {"suffix":"white","prompt":"white rose petals"}],
    "luxury_beach_hut":      WOOD_PALETTE,
    "romantic_dinner_table": [{"suffix":"white_cloth","prompt":"white tablecloth"},
                              {"suffix":"red_cloth","prompt":"red tablecloth"},
                              {"suffix":"pink_cloth","prompt":"pink tablecloth"}],
    "pink_seashell_decor":   [{"suffix":"pink","prompt":"pink scallop"},
                              {"suffix":"white","prompt":"white shell"},
                              {"suffix":"pearlescent","prompt":"pearlescent shell"}],
    "ribbon_banner":         [{"suffix":"red","prompt":"red ribbon"},
                              {"suffix":"pink","prompt":"pink ribbon"},
                              {"suffix":"gold","prompt":"gold ribbon"},
                              {"suffix":"silver","prompt":"silver ribbon"}],

    # YUNO SAGE
    "orrery_planet_model":   [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"gold","prompt":"gold trimmed"},
                              {"suffix":"silver","prompt":"silver"}],
    "lecturing_platform":    WOOD_PALETTE,
    "geometric_cube_floating":[{"suffix":"blue_glow","prompt":"glowing blue"},
                                {"suffix":"green_glow","prompt":"glowing green"},
                                {"suffix":"purple_glow","prompt":"glowing purple"}],
    "geometric_dodecahedron":[{"suffix":"blue_glow","prompt":"glowing blue"},
                              {"suffix":"green_glow","prompt":"glowing green"},
                              {"suffix":"purple_glow","prompt":"glowing purple"}],
    "crystal_study_desk":    [{"suffix":"blue_crystal","prompt":"with blue crystal inlay"},
                              {"suffix":"purple_crystal","prompt":"with purple crystal inlay"},
                              {"suffix":"green_crystal","prompt":"with green crystal inlay"}],
    "ancient_text_lectern":  WOOD_PALETTE,

    # PYRAMID INTERIOR
    "canopic_jar_jackal":    [{"suffix":"painted","prompt":"with hieroglyphs painted"},
                              {"suffix":"gold","prompt":"with gold accents"},
                              {"suffix":"weathered","prompt":"weathered ancient"}],
    "canopic_jar_bird":      [{"suffix":"painted","prompt":"with hieroglyphs painted"},
                              {"suffix":"gold","prompt":"with gold accents"},
                              {"suffix":"weathered","prompt":"weathered ancient"}],
    "canopic_jar_human":     [{"suffix":"painted","prompt":"with hieroglyphs painted"},
                              {"suffix":"gold","prompt":"with gold accents"},
                              {"suffix":"weathered","prompt":"weathered ancient"}],
    "ankh_wall_symbol":      STONE_PALETTE,
    "scarab_inlay":          [{"suffix":"gold","prompt":"gold scarab"},
                              {"suffix":"lapis","prompt":"blue lapis lazuli scarab"},
                              {"suffix":"emerald","prompt":"emerald green scarab"}],
    "treasure_heap_gold":    [{"suffix":"gold","prompt":"glittering gold treasure"},
                              {"suffix":"silver","prompt":"silver treasure"},
                              {"suffix":"mixed","prompt":"mixed gold silver and gems"}],
    "mummy_sarcophagus_open":[{"suffix":"gold","prompt":"gold and blue painted"},
                              {"suffix":"plain","prompt":"plain weathered stone"}],
    "mummy_sarcophagus_closed":[{"suffix":"gold","prompt":"gold and blue painted"},
                                {"suffix":"plain","prompt":"plain weathered stone"}],

    # MODULAR BUILDING
    "wall_stone_smooth":     STONE_PALETTE,
    "wall_stone_rough":      STONE_PALETTE,
    "wall_half_timber":      [{"suffix":"white_dark","prompt":"white plaster dark beams"},
                              {"suffix":"cream_brown","prompt":"cream plaster brown beams"},
                              {"suffix":"weathered","prompt":"weathered aged"}],
    "wall_brick_red":        [{"suffix":"red","prompt":"red brick"},
                              {"suffix":"tan","prompt":"tan brick"},
                              {"suffix":"weathered","prompt":"weathered aged brick"}],
    "wall_wooden_plank":     WOOD_PALETTE,
    "wall_wattle_daub":      [{"suffix":"natural","prompt":"natural tan plaster"},
                              {"suffix":"whitewash","prompt":"whitewashed"},
                              {"suffix":"weathered","prompt":"weathered cracked"}],
    "wall_marble_panel":     STONE_PALETTE,
    "wall_adobe":            [{"suffix":"tan","prompt":"warm tan adobe"},
                              {"suffix":"white","prompt":"whitewashed adobe"},
                              {"suffix":"red_clay","prompt":"red clay adobe"}],
    "window_round":          WOOD_PALETTE,
    "window_arched":         WOOD_PALETTE,
    "window_gothic_pointed": STONE_PALETTE,
    "window_casement":       WOOD_PALETTE,
    "window_lattice":        WOOD_PALETTE,
    "window_leaded_glass":   IRON_PALETTE,
    "window_bay":            WOOD_PALETTE,
    "window_dormer":         WOOD_PALETTE,
    "door_iron_bound":       WOOD_PALETTE,
    "door_paper_screen":     WOOD_PALETTE,
    "door_arched_cathedral": WOOD_PALETTE,
    "door_secret_stone":     STONE_PALETTE,
    "door_double_wooden":    WOOD_PALETTE,
    "roof_red_clay_tile":    [{"suffix":"red","prompt":"red clay tiles"},
                              {"suffix":"orange","prompt":"orange clay tiles"},
                              {"suffix":"weathered","prompt":"weathered aged tiles"}],
    "roof_slate_section":    [{"suffix":"dark_gray","prompt":"dark gray slate"},
                              {"suffix":"blue_slate","prompt":"blue slate"},
                              {"suffix":"weathered","prompt":"weathered slate"}],
    "roof_thatch_section":   [{"suffix":"golden","prompt":"golden brown thatch"},
                              {"suffix":"weathered","prompt":"weathered gray thatch"},
                              {"suffix":"fresh","prompt":"fresh yellow thatch"}],
    "roof_wooden_shingle":   WOOD_PALETTE,
    "roof_asian_upturned":   [{"suffix":"green_tile","prompt":"green tiles"},
                              {"suffix":"red_tile","prompt":"red tiles"},
                              {"suffix":"black_tile","prompt":"black tiles"}],
    "roof_dome_cupola":      [{"suffix":"gold","prompt":"gold dome"},
                              {"suffix":"copper","prompt":"copper patina dome"},
                              {"suffix":"white","prompt":"white painted dome"}],
    "column_doric":          STONE_PALETTE,
    "column_ionic":          STONE_PALETTE,
    "column_corinthian":     STONE_PALETTE,
    "column_solomonic":      STONE_PALETTE,
    "column_ivy_wrapped":    STONE_PALETTE,
    "weather_vane_rooster":  IRON_PALETTE,
    "weather_vane_ship":     IRON_PALETTE,
    "finial_decorative":     IRON_PALETTE,
    "eave_bracket":          WOOD_PALETTE,

    # TOWN INFRASTRUCTURE
    "clock_tower":           STONE_PALETTE,
    "bell_tower_belfry":     STONE_PALETTE,
    "watchtower_modular":    WOOD_PALETTE,
    "public_bath_house":     STONE_PALETTE,
    "stable_horse":          WOOD_PALETTE,
    "kennel_dog":            WOOD_PALETTE,
    "dovecote_round":        STONE_PALETTE,
    "sewer_grate":           IRON_PALETTE,
    "manhole_cover":         IRON_PALETTE,
    "cistern_water_tank":    WOOD_PALETTE,
    "rooftop_water_barrel":  WOOD_PALETTE,
    "stocks_pillory":        WOOD_PALETTE,
    "bandstand_gazebo":      WOOD_PALETTE,
    "pergola_trellis":       [{"suffix":"pink_roses","prompt":"with pink roses"},
                              {"suffix":"white_roses","prompt":"with white roses"},
                              {"suffix":"red_roses","prompt":"with red roses"},
                              {"suffix":"wisteria","prompt":"with purple wisteria"}],

    # MAGICAL / ELEMENTAL
    "elemental_shrine_water":[{"suffix":"blue","prompt":"deep blue water"},
                              {"suffix":"cyan","prompt":"cyan glowing water"},
                              {"suffix":"green","prompt":"emerald green water"}],
    "elemental_shrine_fire": [{"suffix":"red","prompt":"red flames"},
                              {"suffix":"orange","prompt":"orange flames"},
                              {"suffix":"blue_flame","prompt":"blue magical flames"}],
    "elemental_shrine_earth":[{"suffix":"brown","prompt":"earthy brown stone"},
                              {"suffix":"green_moss","prompt":"with green moss"},
                              {"suffix":"crystal","prompt":"with brown crystals"}],
    "elemental_shrine_wind": [{"suffix":"white","prompt":"pale white"},
                              {"suffix":"silver","prompt":"silver"},
                              {"suffix":"blue_sky","prompt":"sky blue"}],
    "mana_crystal_node":     [{"suffix":"blue","prompt":"glowing blue"},
                              {"suffix":"purple","prompt":"glowing purple"},
                              {"suffix":"green","prompt":"glowing green"},
                              {"suffix":"red","prompt":"glowing red"}],
    "magical_sigil_wall":    [{"suffix":"blue","prompt":"glowing blue runes"},
                              {"suffix":"red","prompt":"glowing red runes"},
                              {"suffix":"gold","prompt":"glowing gold runes"}],
    "floating_book":         [{"suffix":"blue_glow","prompt":"glowing blue text"},
                              {"suffix":"red_glow","prompt":"glowing red text"},
                              {"suffix":"gold_glow","prompt":"glowing gold text"}],
    "animated_scroll":       [{"suffix":"blue_glow","prompt":"glowing blue text"},
                              {"suffix":"red_glow","prompt":"glowing red text"},
                              {"suffix":"gold_glow","prompt":"glowing gold text"}],
    "magic_mirror":          [{"suffix":"gold_frame","prompt":"ornate gold frame"},
                              {"suffix":"silver_frame","prompt":"silver frame"},
                              {"suffix":"black_frame","prompt":"black lacquer frame"}],
    "crystal_ball_stand":    [{"suffix":"clear","prompt":"clear crystal"},
                              {"suffix":"blue","prompt":"blue glowing crystal"},
                              {"suffix":"purple","prompt":"purple glowing crystal"}],
    "wizard_staff_display":  [{"suffix":"blue_crystal","prompt":"blue crystal top"},
                              {"suffix":"red_crystal","prompt":"red crystal top"},
                              {"suffix":"gold_orb","prompt":"gold orb top"}],
    "phoenix_urn":           [{"suffix":"red_gold","prompt":"red and gold"},
                              {"suffix":"orange","prompt":"orange flames pattern"},
                              {"suffix":"black_red","prompt":"black with red"}],
    "dragon_urn":            [{"suffix":"green","prompt":"green dragon"},
                              {"suffix":"red","prompt":"red dragon"},
                              {"suffix":"gold","prompt":"gold dragon"},
                              {"suffix":"blue","prompt":"blue dragon"}],
    "soul_lantern_blue":     [{"suffix":"blue","prompt":"glowing blue flame"},
                              {"suffix":"green","prompt":"glowing green flame"},
                              {"suffix":"purple","prompt":"glowing purple flame"}],

    # INDUSTRIAL
    "train_platform":        STONE_PALETTE,
    "ticket_booth":          WOOD_PALETTE,
    "steam_locomotive":      [{"suffix":"black","prompt":"black painted"},
                              {"suffix":"red","prompt":"red painted"},
                              {"suffix":"green","prompt":"green painted"}],
    "conveyor_belt_segment": IRON_PALETTE,
    "pressure_gauge":        IRON_PALETTE,
    "valve_wheel":           [{"suffix":"red","prompt":"red painted"},
                              {"suffix":"yellow","prompt":"yellow painted"},
                              {"suffix":"black","prompt":"black iron"},
                              {"suffix":"rusted","prompt":"rusted"}],
    "rusted_machinery":      IRON_PALETTE,
    "industrial_crane":      [{"suffix":"yellow","prompt":"yellow painted"},
                              {"suffix":"red","prompt":"red painted"},
                              {"suffix":"green","prompt":"green painted"}],
    "loading_bay_platform":  STONE_PALETTE,
    "hotel_facade_4story":   STONE_PALETTE,
    "iron_lamppost_gas":     IRON_PALETTE,

    # MARITIME
    "ship_hull_galleon_prow":WOOD_PALETTE,
    "ship_hull_galleon_mid": WOOD_PALETTE,
    "ship_hull_galleon_stern":WOOD_PALETTE,
    "fishing_dinghy":        WOOD_PALETTE,
    "sail_raised":           [{"suffix":"white","prompt":"white canvas"},
                              {"suffix":"red","prompt":"red canvas"},
                              {"suffix":"black","prompt":"black canvas"}],
    "sail_tattered":         FABRIC_PALETTE,
    "crows_nest":            WOOD_PALETTE,
    "cannon_loaded":         IRON_PALETTE,
    "captains_wheel":        WOOD_PALETTE,
    "spyglass":              [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"silver","prompt":"silver"},
                              {"suffix":"black_lacquer","prompt":"black lacquered"}],
    "sextant":               [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"copper","prompt":"copper"},
                              {"suffix":"silver","prompt":"silver"}],
    "rolled_map":            [{"suffix":"weathered","prompt":"weathered parchment"},
                              {"suffix":"cream","prompt":"cream parchment"}],
    "pirate_flag":           [{"suffix":"black","prompt":"black flag"},
                              {"suffix":"red","prompt":"red flag"}],

    # CAVE / DUNGEON SUB-DETAIL
    "stalactite_stalagmite_column":STONE_PALETTE,
    "mineral_pool_acid":     [{"suffix":"green","prompt":"glowing acid green"},
                              {"suffix":"yellow","prompt":"yellow toxic"}],
    "mineral_pool_oil":      [{"suffix":"black","prompt":"black oil"},
                              {"suffix":"dark_brown","prompt":"dark brown sludge"}],
    "mineral_pool_magma":    [{"suffix":"red_orange","prompt":"red orange lava"},
                              {"suffix":"yellow_hot","prompt":"yellow hot lava"}],
    "rope_bridge_chasm":     WOOD_PALETTE,
    "mine_support_beam":     WOOD_PALETTE,
    "ore_bucket_chain":      IRON_PALETTE,
    "underground_giant_mushroom":[{"suffix":"cyan","prompt":"glowing cyan"},
                                   {"suffix":"purple","prompt":"glowing purple"},
                                   {"suffix":"red","prompt":"glowing red"}],

    # RESOURCE NODES
    "mining_ore_iron":       [{"suffix":"silver","prompt":"sparkling silver"},
                              {"suffix":"dark","prompt":"dark gray"}],
    "mining_ore_copper":     [{"suffix":"orange","prompt":"orange copper"},
                              {"suffix":"green_patina","prompt":"with green patina"}],
    "mining_ore_gold":       [{"suffix":"bright","prompt":"bright gold"},
                              {"suffix":"dark","prompt":"dark gold"}],
    "herb_gather_bush":      [{"suffix":"green","prompt":"vibrant green"},
                              {"suffix":"flowering","prompt":"with white flowers"},
                              {"suffix":"berry","prompt":"with red berries"}],
    "fishing_spot_marker":   [{"suffix":"blue_water","prompt":"clear blue water"},
                              {"suffix":"murky","prompt":"murky water"}],
    "treasure_dig_mound":    [{"suffix":"brown","prompt":"brown earth"},
                              {"suffix":"sandy","prompt":"sandy"}],

    # TINY DETAIL PROPS
    "coin_pile_gold":        [{"suffix":"gold","prompt":"gold coins"},
                              {"suffix":"silver","prompt":"silver coins"},
                              {"suffix":"copper","prompt":"copper coins"}],
    "scattered_gem_pile":    [{"suffix":"mixed","prompt":"mixed colored gems"},
                              {"suffix":"red_rubies","prompt":"red rubies"},
                              {"suffix":"blue_sapphires","prompt":"blue sapphires"},
                              {"suffix":"green_emeralds","prompt":"green emeralds"}],
    "letter_sealed":         [{"suffix":"red_seal","prompt":"red wax seal"},
                              {"suffix":"black_seal","prompt":"black wax seal"},
                              {"suffix":"blue_seal","prompt":"blue wax seal"}],
    "scroll_open_paper":     [{"suffix":"weathered","prompt":"weathered parchment"},
                              {"suffix":"cream","prompt":"cream parchment"}],
    "quill_inkwell":         [{"suffix":"black_ink","prompt":"black ink"},
                              {"suffix":"red_ink","prompt":"red ink"},
                              {"suffix":"blue_ink","prompt":"blue ink"}],
    "easel_painting":        WOOD_PALETTE,
    "lute_wooden":           WOOD_PALETTE,
    "harp_small":            [{"suffix":"gold","prompt":"polished gold"},
                              {"suffix":"silver","prompt":"silver"},
                              {"suffix":"wood","prompt":"natural wood"}],
    "drum_hand":             WOOD_PALETTE,
    "flute_wooden":          WOOD_PALETTE,
    "ocarina":               [{"suffix":"clay","prompt":"natural clay"},
                              {"suffix":"blue","prompt":"blue ceramic"},
                              {"suffix":"green","prompt":"green ceramic"}],
    "wooden_sword_toy":      WOOD_PALETTE,
    "rag_doll":              [{"suffix":"red","prompt":"red dress"},
                              {"suffix":"blue","prompt":"blue dress"},
                              {"suffix":"yellow","prompt":"yellow dress"}],
    "rocking_horse":         [{"suffix":"painted","prompt":"colorful painted"},
                              {"suffix":"natural","prompt":"natural wood"},
                              {"suffix":"weathered","prompt":"weathered aged"}],
    "kite":                  [{"suffix":"red","prompt":"red kite"},
                              {"suffix":"blue","prompt":"blue kite"},
                              {"suffix":"yellow","prompt":"yellow kite"},
                              {"suffix":"rainbow","prompt":"rainbow striped"}],
    "jewelry_box_ornate":    WOOD_PALETTE,
    "tea_set_porcelain":     [{"suffix":"white_blue","prompt":"white with blue pattern"},
                              {"suffix":"white_pink","prompt":"white with pink flowers"},
                              {"suffix":"gold","prompt":"gold trimmed"}],
    "soup_pot_iron":         [{"suffix":"black","prompt":"black iron"},
                              {"suffix":"copper","prompt":"copper"}],
    "bottle_corked":         [{"suffix":"green_glass","prompt":"green glass"},
                              {"suffix":"clear_glass","prompt":"clear glass"},
                              {"suffix":"blue_glass","prompt":"blue glass"}],
    "broken_bottle":         [{"suffix":"green","prompt":"green glass shards"},
                              {"suffix":"clear","prompt":"clear glass shards"}],
    "rolling_pin":           WOOD_PALETTE,
    "mortar_pestle_kitchen": STONE_PALETTE,

    # TERRAIN ACCENT
    "fallen_log_snapped":    [{"suffix":"weathered","prompt":"weathered gray"},
                              {"suffix":"mossy","prompt":"with green moss"},
                              {"suffix":"dark","prompt":"dark wet"}],
    "ground_vine_dense":     [{"suffix":"green","prompt":"vibrant green"},
                              {"suffix":"flowering","prompt":"with white flowers"},
                              {"suffix":"autumn","prompt":"autumn red"}],
    "pine_cone_scatter":     [{"suffix":"brown","prompt":"brown weathered"},
                              {"suffix":"green","prompt":"fresh green"}],
    "acorn_cluster":         [{"suffix":"brown","prompt":"brown ripe"},
                              {"suffix":"green","prompt":"green unripe"}],
    "leaf_pile_autumn":      [{"suffix":"orange_red","prompt":"orange and red leaves"},
                              {"suffix":"yellow","prompt":"yellow leaves"},
                              {"suffix":"brown","prompt":"brown dry leaves"}],
    "mushroom_ring_fairy":   [{"suffix":"white","prompt":"white mushrooms"},
                              {"suffix":"red","prompt":"red mushrooms with white spots"},
                              {"suffix":"brown","prompt":"brown mushrooms"}],
    "flower_bed_cluster":    [{"suffix":"mixed","prompt":"mixed colorful flowers"},
                              {"suffix":"red","prompt":"red flowers"},
                              {"suffix":"yellow","prompt":"yellow flowers"},
                              {"suffix":"purple","prompt":"purple flowers"}],

    # QUEST / LORE
    "hero_memorial_statue":  STONE_PALETTE,
    "memorial_plaque":       STONE_PALETTE,
    "tomb_marker_text":      STONE_PALETTE,
    "wishing_tree":          [{"suffix":"red_ribbons","prompt":"with red prayer ribbons"},
                              {"suffix":"white_paper","prompt":"with white paper prayers"},
                              {"suffix":"colorful","prompt":"with colorful ribbons"}],
    "sacred_grove_circle":   STONE_PALETTE,
    "boss_arena_pedestal":   STONE_PALETTE,
    "cursed_obelisk_glowing":[{"suffix":"red","prompt":"glowing red runes"},
                              {"suffix":"purple","prompt":"glowing purple runes"},
                              {"suffix":"black","prompt":"glowing black runes"}],
    "ancient_ruin_marker":   STONE_PALETTE,

    # UNDERWATER SUB-DETAIL
    "pearl_oyster_open":     [{"suffix":"white_pearl","prompt":"with white pearl"},
                              {"suffix":"black_pearl","prompt":"with black pearl"},
                              {"suffix":"pink_pearl","prompt":"with pink pearl"}],
    "giant_clam":            [{"suffix":"blue","prompt":"blue and white"},
                              {"suffix":"purple","prompt":"purple"},
                              {"suffix":"green","prompt":"green"}],
    "sunken_statue_head":    STONE_PALETTE,
    "anchor_rusted_kelp":    IRON_PALETTE,
    "bioluminescent_jelly":  [{"suffix":"pink","prompt":"glowing pink"},
                              {"suffix":"blue","prompt":"glowing blue"},
                              {"suffix":"purple","prompt":"glowing purple"}],
    "whale_skeleton_jaw":    [{"suffix":"white","prompt":"bleached white"},
                              {"suffix":"yellowed","prompt":"yellowed aged"}],
    "kelp_forest_tall":      [{"suffix":"green","prompt":"vibrant green"},
                              {"suffix":"dark_green","prompt":"dark green"},
                              {"suffix":"brown","prompt":"brown"}],

    # ANIMAL HUSBANDRY
    "stable_open_front":     WOOD_PALETTE,
    "rabbit_hutch":          WOOD_PALETTE,
    "fish_pond":             [{"suffix":"clear","prompt":"clear blue water"},
                              {"suffix":"green","prompt":"green algae water"}],

    # PVP / WoE
    "ctf_flagpole":          [{"suffix":"red","prompt":"red flag"},
                              {"suffix":"blue","prompt":"blue flag"},
                              {"suffix":"green","prompt":"green flag"},
                              {"suffix":"gold","prompt":"gold flag"}],
    "capture_point_base":    STONE_PALETTE,
    "siege_ladder":          WOOD_PALETTE,
    "defensive_spike_wall":  WOOD_PALETTE,
    "caltrops_pile":         IRON_PALETTE,
    "destructible_barricade":WOOD_PALETTE,
    "destructible_barricade_broken":WOOD_PALETTE,
    "resurrection_altar":    STONE_PALETTE,

    # EDUCATIONAL
    "constellation_map":     [{"suffix":"brass","prompt":"polished brass"},
                              {"suffix":"gold","prompt":"gold trim"},
                              {"suffix":"silver","prompt":"silver"}],
    "dimensional_rift_anchor":[{"suffix":"purple","prompt":"swirling purple"},
                                {"suffix":"blue","prompt":"swirling blue"},
                                {"suffix":"green","prompt":"swirling green"}],
    "class_master_pedestal": STONE_PALETTE,

    # HOLIDAY
    "lunar_lantern_red":     [{"suffix":"red","prompt":"red and gold"},
                              {"suffix":"yellow","prompt":"yellow and red"},
                              {"suffix":"orange","prompt":"orange and red"}],
    "dragon_parade_segment": [{"suffix":"red_gold","prompt":"red and gold"},
                              {"suffix":"green_gold","prompt":"green and gold"},
                              {"suffix":"blue_gold","prompt":"blue and gold"}],
    "firecracker_string":    [{"suffix":"red","prompt":"red firecrackers"},
                              {"suffix":"yellow","prompt":"yellow firecrackers"}],
    "flower_crown":          [{"suffix":"colorful","prompt":"mixed colorful flowers"},
                              {"suffix":"white","prompt":"white flowers"},
                              {"suffix":"pink","prompt":"pink flowers"}],
    "pumpkin_patch_section": [{"suffix":"orange","prompt":"orange pumpkins"},
                              {"suffix":"green","prompt":"green pumpkins"},
                              {"suffix":"white","prompt":"white pumpkins"}],
    "hay_bale_maze":         [{"suffix":"golden","prompt":"golden hay"},
                              {"suffix":"weathered","prompt":"weathered gray hay"}],
    "witches_broom":         WOOD_PALETTE,
    "valentine_heart_arch":  [{"suffix":"pink_roses","prompt":"with pink roses"},
                              {"suffix":"red_roses","prompt":"with red roses"},
                              {"suffix":"white_lilies","prompt":"with white lilies"}],
}


# ============================================================================
# UTILITIES
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


def wait_for_v2_complete():
    """Block until v2 log shows 'V2 REGEN COMPLETE'."""
    log("Waiting for v2 to complete (polling _v2_regen.log)...")
    while True:
        try:
            if V2_LOG.exists():
                text = V2_LOG.read_text(encoding="utf-8", errors="ignore")
                if "V2 REGEN COMPLETE" in text:
                    log("v2 detected as complete. Starting v3.")
                    return
        except Exception as e:
            log(f"  log read err: {e}")
        time.sleep(120)  # check every 2 min


# ============================================================================
# MAIN
# ============================================================================
def main():
    log("=" * 78)
    log(f"HUNYUAN V3 REGIONAL REGEN")
    log(f"  Total assets:   {len(V3_ASSETS)}")
    log(f"  Total variants: {sum(len(v) for v in V3_VARIANTS.values())}")
    log("=" * 78)

    if WAIT_FOR_V2:
        wait_for_v2_complete()

    # Verify ComfyUI is up
    try:
        urllib.request.urlopen(f"{p.COMFY_URL}/system_stats", timeout=5)
    except Exception as e:
        log(f"FATAL: ComfyUI not reachable at {p.COMFY_URL}: {e}")
        return 1

    log("Loading rembg BiRefNet session...")
    from rembg import new_session
    rembg_session = new_session("birefnet-general")

    # ---- PHASE A ----
    log("")
    log("=" * 78)
    log(f"PHASE A: Generate {len(V3_ASSETS)} v3 base assets")
    log("=" * 78)

    overall_start = time.time()
    a_ok = a_skip = a_fail = 0

    for i, asset in enumerate(V3_ASSETS, 1):
        asset_dir = ROOT / asset["category"] / asset["id"]
        if (asset_dir / "06_final.glb").exists():
            log(f"[{i}/{len(V3_ASSETS)}] {asset['category']}/{asset['id']} - SKIP (done)")
            a_skip += 1
            continue

        log("")
        log("-" * 78)
        log(f"[{i}/{len(V3_ASSETS)}] {asset['category']}/{asset['id']}")
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
        eta = avg * (len(V3_ASSETS) - i) / 60
        log(f"  [phase A: {i}/{len(V3_ASSETS)} | ok={a_ok} skip={a_skip} fail={a_fail} | ETA: {eta:.0f} min]")

    log(f"\nPHASE A done: ok={a_ok}, skipped={a_skip}, failed={a_fail} in "
        f"{(time.time()-overall_start)/60:.1f} min")

    # ---- PHASE B ----
    log("")
    log("=" * 78)
    log(f"PHASE B: Generate variants for v3 assets")
    log("=" * 78)

    total_planned = sum(len(v) for v in V3_VARIANTS.values())
    log(f"  Total variants planned: {total_planned}")

    b_start = time.time()
    b_ok = b_skip = b_fail = b_skip_bad = 0

    for asset_id, variants in V3_VARIANTS.items():
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
        log(f"=== {category}/{asset_id} - {len(variants)} variants ===")
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

    log(f"\nPHASE B done: ok={b_ok}, skipped_existing={b_skip}, "
        f"failed={b_fail}, skipped_bad_asset={b_skip_bad} in "
        f"{(time.time()-b_start)/60:.1f} min")

    log("")
    log("=" * 78)
    log(f"V3 REGEN COMPLETE")
    log(f"  Phase A:  ok={a_ok} skip={a_skip} fail={a_fail}")
    log(f"  Phase B:  ok={b_ok} skip_exist={b_skip} fail={b_fail} skip_bad={b_skip_bad}")
    log(f"  Total time: {(time.time()-overall_start)/60:.1f} min")
    log("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
