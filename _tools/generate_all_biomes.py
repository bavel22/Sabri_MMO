"""
MEGA TEXTURE GENERATOR — All RO Classic Biomes
Generates ~120 textures across 15 biome types.
Each biome gets: 3 ground variants + 2 dirt/transition + 2 cliff/slope textures.

Run time: ~2 hours on RTX 5090 (120 textures x 60 steps each)
"""

import json
import urllib.request
import urllib.parse
import time
import os
import sys
import numpy as np
from PIL import Image, ImageEnhance

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_DIR = "C:/Sabri_MMO/_tools/ground_texture_output/biomes"
PREVIEW_DIR = os.path.join(OUTPUT_DIR, "previews")

CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

BASE_NEGATIVE = (
    "photorealistic, photograph, 3d render, plastic, glossy, shiny, specular, "
    "metallic, blurry, low quality, worst quality, watermark, signature, text, "
    "dramatic lighting, high contrast, neon, "
    "person, character, face, items, objects, perspective, sky, horizon, "
    "side view, grid, symmetric, border, frame, vignette, "
    "individual leaves, individual grass blades, clover, shamrock, leaf shapes, stems, "
    "pixel art, mosaic, repeating pattern, stripes"
)

# ============================================================
# BIOME DEFINITIONS — 15 biomes, ~8 textures each
# ============================================================

BIOMES = {
    # ==========================================
    # 1. GRASSLAND / MEADOW (Prontera Fields)
    # ==========================================
    "grassland": {
        "neg_extra": ", cold blue, purple, dark",
        "textures": [
            ("T_Grass_Lush_A", "overhead view of painted ground surface, oil painting style, rich sage green and olive green color fields with warm yellow-green patches, soft organic blending, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 70001),
            ("T_Grass_Lush_B", "overhead view of painted ground surface, oil painting style, warm olive green with scattered golden yellow patches, soft watercolor blending, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 70002),
            ("T_Grass_Lush_C", "overhead view of painted ground surface, oil painting style, multiple shades of muted green from dark forest to light lime, splotchy organic variation, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 70003),
            ("T_Grass_Dirt_A", "overhead view of painted ground surface, oil painting style, warm ochre brown packed earth with subtle grain texture, matte, flat lighting, seamless tileable, hand-painted game texture", 0.15, 70011),
            ("T_Grass_Dirt_B", "overhead view of painted ground surface, oil painting style, sandy brown earth with small pebble hints, warm sienna tones, matte, flat lighting, seamless tileable, hand-painted game texture", 0.15, 70012),
            ("T_Grass_Cliff_A", "painted stone cliff face, oil painting style, gray and purple-brown layered rock with cracks and weathering, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 70021),
            ("T_Grass_Cliff_B", "painted rough stone surface, oil painting style, muted gray boulders with dark crevices, mossy green hints, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 70022),
        ]
    },

    # ==========================================
    # 2. FOREST (Payon, Mt. Mjolnir)
    # ==========================================
    "forest": {
        "neg_extra": ", bright yellow, sand, desert",
        "textures": [
            ("T_Forest_Floor_A", "overhead view of painted dark forest floor, oil painting style, deep emerald green and dark brown, decaying leaves and moss patches, damp shaded feeling, cool muted palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 71001),
            ("T_Forest_Floor_B", "overhead view of painted forest floor, oil painting style, rich dark green moss with brown earth patches, blue-green undertones, mushroom-dotted ground feeling, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 71002),
            ("T_Forest_Floor_C", "overhead view of painted shaded ground, oil painting style, dark olive green and cool brown, leaf litter tones, deep shadow patches, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 71003),
            ("T_Forest_Dirt_A", "overhead view of painted dark earth surface, oil painting style, rich dark brown soil with decomposing matter, cool undertone, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 71011),
            ("T_Forest_Dirt_B", "overhead view of painted muddy ground, oil painting style, wet dark brown and gray-green mud, organic patches, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 71012),
            ("T_Forest_Cliff_A", "painted mossy rock cliff face, oil painting style, dark gray stone covered in green moss patches, damp crevices, forest boulder, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 71021),
            ("T_Forest_Cliff_B", "painted dark stone surface with roots, oil painting style, gray-brown rock with thin root tendrils and moss, forest cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 71022),
        ]
    },

    # ==========================================
    # 3. DESERT (Morroc, Sograt)
    # ==========================================
    "desert": {
        "neg_extra": ", green, grass, forest, cold blue, snow",
        "textures": [
            ("T_Desert_Sand_A", "overhead view of painted desert sand surface, oil painting style, warm golden amber sand with subtle orange ripple patterns, sun-baked palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 72001),
            ("T_Desert_Sand_B", "overhead view of painted desert ground, oil painting style, pale cream and warm tan sand, soft wind-swept texture, dry arid palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 72002),
            ("T_Desert_Sand_C", "overhead view of painted hot sand surface, oil painting style, deep sienna and burnt orange sand tones, scorching desert palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 72003),
            ("T_Desert_Dirt_A", "overhead view of painted dry cracked earth, oil painting style, dark brown and burnt sienna dried mud with crack lines, parched desert ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 72011),
            ("T_Desert_Dirt_B", "overhead view of painted dusty ground, oil painting style, pale tan dust with scattered dark pebble patches, arid wasteland surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 72012),
            ("T_Desert_Cliff_A", "painted sandstone cliff face, oil painting style, layered sedimentary rock in warm orange and tan bands, horizontal stratification, weathered canyon wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 72021),
            ("T_Desert_Cliff_B", "painted rough sandstone surface, oil painting style, warm brown and orange eroded rock, deep cracks and wind-carved shapes, desert mesa wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 72022),
        ]
    },

    # ==========================================
    # 4. SNOW / TUNDRA (Lutie, Rachel)
    # ==========================================
    "snow": {
        "neg_extra": ", green, grass, warm, orange, desert, sand",
        "textures": [
            ("T_Snow_Ground_A", "overhead view of painted snow covered ground, oil painting style, white snow with subtle pale blue shadows, soft crystalline sparkle hints, cool winter palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 73001),
            ("T_Snow_Ground_B", "overhead view of painted snowy surface, oil painting style, bright white snow with gray-blue shadow drifts, wind-packed surface texture, arctic palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 73002),
            ("T_Snow_Ground_C", "overhead view of painted ice and snow surface, oil painting style, pale blue-white ice with white snow patches, frozen ground, crystal clear winter palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 73003),
            ("T_Snow_Dirt_A", "overhead view of painted frozen earth, oil painting style, dark gray-brown frozen dirt with frost crystals, patches of thin ice, cold muted palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 73011),
            ("T_Snow_Dirt_B", "overhead view of painted muddy snow, oil painting style, gray-brown slush mixed with dirty snow, melting winter ground, cold palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 73012),
            ("T_Snow_Cliff_A", "painted frozen cliff face, oil painting style, dark gray-blue ice-covered rock, icicle formations, frost patterns on stone, arctic cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 73021),
            ("T_Snow_Cliff_B", "painted frost-covered stone surface, oil painting style, pale gray rock with blue ice patches and white frost, winter mountain cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 73022),
        ]
    },

    # ==========================================
    # 5. VOLCANIC (Magma Dungeon, Thor, Veins)
    # ==========================================
    "volcanic": {
        "neg_extra": ", green, grass, snow, cold blue, pastoral",
        "textures": [
            ("T_Volcanic_Ground_A", "overhead view of painted volcanic ground, oil painting style, black and dark gray basalt rock with glowing orange-red lava cracks between, hellish palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 74001),
            ("T_Volcanic_Ground_B", "overhead view of painted scorched earth, oil painting style, charcoal black and dark brown burnt ground with ember-orange hot spots, volcanic ash surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 74002),
            ("T_Volcanic_Ground_C", "overhead view of painted dark volcanic rock surface, oil painting style, obsidian black with dark red veins, cooled lava texture, jagged dark palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 74003),
            ("T_Volcanic_Dirt_A", "overhead view of painted volcanic ash ground, oil painting style, dark gray ash with orange-brown scorched patches, still-warm ground feeling, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 74011),
            ("T_Volcanic_Cliff_A", "painted dark volcanic cliff face, oil painting style, black basalt columns with red-orange lava glow in deep cracks, jagged volcanic wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 74021),
            ("T_Volcanic_Cliff_B", "painted obsidian rock surface, oil painting style, glassy black and dark gray volcanic rock with sharp angular fractures, dark volcanic cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 74022),
        ]
    },

    # ==========================================
    # 6. SWAMP / MARSH (Comodo outskirts)
    # ==========================================
    "swamp": {
        "neg_extra": ", bright, cheerful, desert, sand, snow",
        "textures": [
            ("T_Swamp_Ground_A", "overhead view of painted swamp ground, oil painting style, murky dark green and brown waterlogged earth, algae patches, damp rotting vegetation tones, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 75001),
            ("T_Swamp_Ground_B", "overhead view of painted marshy surface, oil painting style, dark olive green mud with standing water patches, decaying plant matter, sickly green-brown palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 75002),
            ("T_Swamp_Ground_C", "overhead view of painted boggy ground, oil painting style, wet dark brown and yellow-green moss, stagnant water sheen, murky swamp palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 75003),
            ("T_Swamp_Dirt_A", "overhead view of painted wet mud surface, oil painting style, dark brown saturated mud with puddle reflections, decomposing matter, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 75011),
            ("T_Swamp_Cliff_A", "painted damp mossy rock, oil painting style, dark gray-green slimy rock surface, thick moss and algae coating, swamp cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 75021),
            ("T_Swamp_Cliff_B", "painted waterlogged stone surface, oil painting style, dark brown and green wet rock with water stains running down, marsh cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 75022),
        ]
    },

    # ==========================================
    # 7. BEACH / COASTAL (Comodo, Alberta, Jawaii)
    # ==========================================
    "beach": {
        "neg_extra": ", dark, gloomy, volcanic, lava, snow",
        "textures": [
            ("T_Beach_Sand_A", "overhead view of painted beach sand, oil painting style, light warm cream and pale gold wet sand, gentle wave-wash texture, tropical beach palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 76001),
            ("T_Beach_Sand_B", "overhead view of painted dry beach sand, oil painting style, bright warm white-gold dry sand with subtle shell fragment hints, paradise beach palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 76002),
            ("T_Beach_Sand_C", "overhead view of painted tropical shore, oil painting style, warm peach and cream sand with faint aqua water edge tint, island paradise palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 76003),
            ("T_Beach_Dirt_A", "overhead view of painted wet sand surface, oil painting style, dark tan wet sand with small wave-deposited debris lines, shoreline ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 76011),
            ("T_Beach_Cliff_A", "painted white limestone cliff face, oil painting style, bright cream and pale gray weathered coastal rock, sea-eroded surface, beach cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 76021),
            ("T_Beach_Cliff_B", "painted sandy coral rock surface, oil painting style, warm tan and pink-tinted porous coastal rock, beach cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 76022),
        ]
    },

    # ==========================================
    # 8. MOUNTAIN / ALPINE (Mt. Mjolnir peaks)
    # ==========================================
    "mountain": {
        "neg_extra": ", tropical, beach, desert sand, lava",
        "textures": [
            ("T_Mountain_Ground_A", "overhead view of painted alpine ground, oil painting style, sparse gray-green grass and exposed gray rock patches, high altitude barren ground, cool muted palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 77001),
            ("T_Mountain_Ground_B", "overhead view of painted rocky highland, oil painting style, gray-brown gravel and sparse dry grass tufts, windswept mountain plateau, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 77002),
            ("T_Mountain_Ground_C", "overhead view of painted mountain meadow, oil painting style, cool green and gray mixed alpine grass with exposed stone patches, high altitude palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 77003),
            ("T_Mountain_Dirt_A", "overhead view of painted rocky gravel surface, oil painting style, gray and brown loose stones and crushed rock, mountain path ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 77011),
            ("T_Mountain_Cliff_A", "painted mountain cliff face, oil painting style, rugged dark gray granite with sharp angular fractures, exposed mountain rock wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 77021),
            ("T_Mountain_Cliff_B", "painted weathered mountain stone, oil painting style, gray and brown layered sedimentary rock, wind-eroded mountain cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 77022),
        ]
    },

    # ==========================================
    # 9. DUNGEON / SEWER (Prontera Culvert, GH Sewers)
    # ==========================================
    "dungeon": {
        "neg_extra": ", bright, sunny, green grass, cheerful, outdoor",
        "textures": [
            ("T_Dungeon_Floor_A", "overhead view of painted dungeon floor, oil painting style, dark blue-gray stone tiles with dark mortar lines, cold underground palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 78001),
            ("T_Dungeon_Floor_B", "overhead view of painted sewer floor, oil painting style, dark green-gray wet stone, slime trails, dank underground surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 78002),
            ("T_Dungeon_Floor_C", "overhead view of painted crypt floor, oil painting style, dark purple-gray ancient stone, crumbling and worn, dusty dungeon surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 78003),
            ("T_Dungeon_Dirt_A", "overhead view of painted underground rubble, oil painting style, dark brown and gray crushed stone debris, dungeon floor detritus, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 78011),
            ("T_Dungeon_Wall_A", "painted dungeon wall surface, oil painting style, dark gray-blue brick wall with mortar lines, damp stains, underground wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 78021),
            ("T_Dungeon_Wall_B", "painted ancient stone wall, oil painting style, dark purple-gray hewn stone blocks, cobweb corner hints, crypt wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 78022),
        ]
    },

    # ==========================================
    # 10. CAVE / CAVERN (Payon Cave, Byalan)
    # ==========================================
    "cave": {
        "neg_extra": ", bright, sunny, green grass, outdoor, sky",
        "textures": [
            ("T_Cave_Floor_A", "overhead view of painted cave floor, oil painting style, dark brown and gray natural stone, uneven cave ground with stalactite drip stains, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 79001),
            ("T_Cave_Floor_B", "overhead view of painted cavern floor, oil painting style, dark reddish-brown stone with mineral deposits, natural cave ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 79002),
            ("T_Cave_Floor_C", "overhead view of painted underground lake shore, oil painting style, wet dark stone with blue-tinted mineral patches, crystal cave ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 79003),
            ("T_Cave_Dirt_A", "overhead view of painted cave sediment, oil painting style, dark brown fine clay and silt, damp cave floor deposit, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 79011),
            ("T_Cave_Wall_A", "painted natural cave wall, oil painting style, dark brown and gray irregular rock face, stalactite formations, dripping wet cave wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 79021),
            ("T_Cave_Wall_B", "painted crystal cave wall, oil painting style, dark stone with embedded glowing blue-purple crystal clusters, magical cavern wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 79022),
        ]
    },

    # ==========================================
    # 11. JUNGLE / TROPICAL (Umbala, Ayothaya)
    # ==========================================
    "jungle": {
        "neg_extra": ", snow, ice, cold, desert sand, urban",
        "textures": [
            ("T_Jungle_Floor_A", "overhead view of painted jungle floor, oil painting style, rich dark green and warm brown tropical undergrowth, dense vegetation carpet, humid jungle palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 80001),
            ("T_Jungle_Floor_B", "overhead view of painted tropical ground, oil painting style, vivid green ferns and warm brown earth, exotic jungle floor with fallen tropical leaf patterns, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 80002),
            ("T_Jungle_Floor_C", "overhead view of painted rainforest ground, oil painting style, deep saturated green moss and dark rich soil, tropical humidity palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 80003),
            ("T_Jungle_Dirt_A", "overhead view of painted tropical mud, oil painting style, rich dark brown wet earth with orange-red clay undertone, jungle path ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 80011),
            ("T_Jungle_Cliff_A", "painted jungle cliff face, oil painting style, dark brown stone covered in thick green vines and moss, tropical rock wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 80021),
            ("T_Jungle_Cliff_B", "painted ancient temple stone covered in jungle growth, oil painting style, gray-brown carved stone with green overgrowth, ruins cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 80022),
        ]
    },

    # ==========================================
    # 12. COBBLESTONE / URBAN (Prontera, Geffen, Aldebaran)
    # ==========================================
    "urban": {
        "neg_extra": ", natural, wild, jungle, forest, desert",
        "textures": [
            ("T_Urban_Cobble_A", "overhead view of painted cobblestone road, oil painting style, warm gray and ochre medieval stone pavers with dark mortar lines, town plaza surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 81001),
            ("T_Urban_Cobble_B", "overhead view of painted brick road surface, oil painting style, warm red-brown and gray rectangular bricks, old town road, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 81002),
            ("T_Urban_Cobble_C", "overhead view of painted stone plaza floor, oil painting style, light beige and cream stone tiles, grand medieval plaza, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 81003),
            ("T_Urban_Dirt_A", "overhead view of painted worn stone path, oil painting style, faded gray-brown stone with dirt in gaps, weathered town road, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 81011),
            ("T_Urban_Wall_A", "painted medieval stone wall, oil painting style, warm gray stone blocks with mortar, castle wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 81021),
            ("T_Urban_Wall_B", "painted old brick wall surface, oil painting style, warm red-brown bricks with aged mortar, medieval building wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 81022),
        ]
    },

    # ==========================================
    # 13. CURSED / UNDEAD (Glast Heim, Nifflheim)
    # ==========================================
    "cursed": {
        "neg_extra": ", bright, cheerful, sunny, tropical, green grass",
        "textures": [
            ("T_Cursed_Ground_A", "overhead view of painted cursed ground, oil painting style, dark purple-gray dead earth with sickly green patches, decaying and corrupted soil, undead wasteland palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 82001),
            ("T_Cursed_Ground_B", "overhead view of painted haunted ground, oil painting style, dark gray and deep purple earth, withered dead vegetation traces, eerie cursed palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 82002),
            ("T_Cursed_Ground_C", "overhead view of painted corrupted earth, oil painting style, black and dark red tainted soil, evil energy seeping through ground, dark horror palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 82003),
            ("T_Cursed_Dirt_A", "overhead view of painted bone-littered ground, oil painting style, dark gray soil with pale bone-white fragments, graveyard earth, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 82011),
            ("T_Cursed_Cliff_A", "painted cursed castle wall, oil painting style, dark purple-black stone with glowing sickly green cracks, haunted fortress wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 82021),
            ("T_Cursed_Cliff_B", "painted crumbling tomb wall, oil painting style, dark gray ancient stone with purple shadow stains, cryptic carved surface, cursed ruin wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 82022),
        ]
    },

    # ==========================================
    # 14. RUINS / ANCIENT (Sphinx, Pyramids, Temples)
    # ==========================================
    "ruins": {
        "neg_extra": ", modern, industrial, snow, jungle",
        "textures": [
            ("T_Ruins_Floor_A", "overhead view of painted ancient stone floor, oil painting style, warm beige and tan sandstone tiles with cracks and age, crumbling temple floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 83001),
            ("T_Ruins_Floor_B", "overhead view of painted ruined marble floor, oil painting style, faded white and gray cracked marble with gold-brown age stains, ancient palace floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 83002),
            ("T_Ruins_Floor_C", "overhead view of painted overgrown stone floor, oil painting style, gray stone blocks with green moss growing in cracks, abandoned temple floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 83003),
            ("T_Ruins_Dirt_A", "overhead view of painted sandy rubble, oil painting style, warm tan sand and broken stone debris, collapsed temple floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 83011),
            ("T_Ruins_Wall_A", "painted ancient temple wall, oil painting style, warm sandstone with carved hieroglyph-like patterns, aged pyramid surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 83021),
            ("T_Ruins_Wall_B", "painted crumbling ancient wall, oil painting style, gray-brown stone blocks with missing sections and age cracks, ruined fortress wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 83022),
        ]
    },

    # ==========================================
    # 15. INDUSTRIAL (Einbroch, Lighthalzen)
    # ==========================================
    "industrial": {
        "neg_extra": ", natural, forest, grass, pastoral, tropical",
        "textures": [
            ("T_Industrial_Floor_A", "overhead view of painted factory floor, oil painting style, dark gray metal plate with bolt rivets, industrial steel surface, steampunk palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 84001),
            ("T_Industrial_Floor_B", "overhead view of painted industrial ground, oil painting style, dirty gray concrete with oil stain patches, factory yard surface, grimy palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 84002),
            ("T_Industrial_Floor_C", "overhead view of painted rusty metal surface, oil painting style, orange-brown rust and dark gray corroded metal, industrial decay palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 84003),
            ("T_Industrial_Dirt_A", "overhead view of painted ash and soot ground, oil painting style, dark gray and black industrial ash with brown dirt, polluted ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.15, 84011),
            ("T_Industrial_Wall_A", "painted industrial brick wall, oil painting style, dark red-brown soot-stained brick with rusted pipe marks, factory wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 84021),
            ("T_Industrial_Wall_B", "painted corrugated metal wall, oil painting style, dark gray and rust-orange industrial metal panels, steampunk wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.15, 84022),
        ]
    },
}

# ============================================================
# ComfyUI helpers (same as other scripts)
# ============================================================

def queue_prompt(workflow):
    data = json.dumps({"prompt": workflow}).encode("utf-8")
    req = urllib.request.Request(f"{COMFYUI_URL}/prompt", data=data, headers={"Content-Type": "application/json"})
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read()).get("prompt_id")
    except urllib.error.HTTPError as e:
        print(f"    Error {e.code}: {e.read().decode('utf-8', errors='replace')[:200]}")
        return None

def wait_for_completion(prompt_id, timeout=300):
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history:
                return history[prompt_id]
        except Exception:
            pass
        time.sleep(2)
    return None

def download_image(filename, subfolder, save_path):
    params = urllib.parse.urlencode({"filename": filename, "subfolder": subfolder, "type": "output"})
    urllib.request.urlretrieve(f"{COMFYUI_URL}/view?{params}", save_path)

def build_workflow(prompt, negative, seed, lora_strength):
    nodes = {"1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CHECKPOINT}}}
    model_src, clip_src = ["1", 0], ["1", 1]
    if lora_strength > 0:
        nodes["2"] = {"class_type": "LoraLoader", "inputs": {
            "model": ["1", 0], "clip": ["1", 1], "lora_name": LORA,
            "strength_model": lora_strength, "strength_clip": lora_strength}}
        model_src, clip_src = ["2", 0], ["2", 1]
    nodes["3"] = {"class_type": "SeamlessTile", "inputs": {"model": model_src, "tiling": "enable", "copy_model": "Make a copy"}}
    nodes["4"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": clip_src, "text": prompt}}
    nodes["5"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": clip_src, "text": negative}}
    nodes["6"] = {"class_type": "EmptyLatentImage", "inputs": {"width": 1024, "height": 1024, "batch_size": 1}}
    nodes["7"] = {"class_type": "KSampler", "inputs": {
        "model": ["3", 0], "positive": ["4", 0], "negative": ["5", 0], "latent_image": ["6", 0],
        "seed": seed, "steps": 60, "cfg": 6.5, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}}
    nodes["8"] = {"class_type": "CircularVAEDecode", "inputs": {"samples": ["7", 0], "vae": ["1", 2], "tiling": "enable"}}
    nodes["9"] = {"class_type": "SaveImage", "inputs": {"images": ["8", 0], "filename_prefix": "biome"}}
    return nodes

def postprocess(img_path, name):
    img = Image.open(img_path)
    img = ImageEnhance.Color(img).enhance(0.85)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= 1.03
    arr[:, :, 2] *= 0.97
    arr = arr.clip(0, 255).astype(np.uint8)
    Image.fromarray(arr).save(img_path)
    # 3x3 preview
    img = Image.open(img_path)
    w, h = img.size
    preview = Image.new("RGB", (w * 3, h * 3))
    for r in range(3):
        for c in range(3):
            preview.paste(img, (c * w, r * h))
    if preview.width > 2048:
        ratio = 2048 / preview.width
        preview = preview.resize((2048, int(preview.height * ratio)), Image.LANCZOS)
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    preview.save(os.path.join(PREVIEW_DIR, f"{name}_3x3.jpg"), quality=90)

# ============================================================
# MAIN
# ============================================================

def main():
    total = sum(len(b["textures"]) for b in BIOMES.values())
    print(f"{'=' * 60}")
    print(f"  MEGA BIOME TEXTURE GENERATOR")
    print(f"  {len(BIOMES)} biomes, {total} textures total")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'=' * 60}\n")

    try:
        urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except Exception:
        print("ERROR: ComfyUI not running!")
        sys.exit(1)

    success = 0
    idx = 0
    for biome_name, biome in BIOMES.items():
        biome_dir = os.path.join(OUTPUT_DIR, biome_name)
        os.makedirs(biome_dir, exist_ok=True)

        neg = BASE_NEGATIVE + biome.get("neg_extra", "")
        print(f"\n=== BIOME: {biome_name.upper()} ({len(biome['textures'])} textures) ===")

        for tex_name, prompt, lora, seed in biome["textures"]:
            idx += 1
            img_path = os.path.join(biome_dir, f"{tex_name}.png")

            if os.path.exists(img_path):
                print(f"  [{idx}/{total}] {tex_name}: exists, skipping")
                success += 1
                continue

            print(f"  [{idx}/{total}] {tex_name} (seed={seed})")
            workflow = build_workflow(prompt, neg, seed, lora)
            prompt_id = queue_prompt(workflow)
            if not prompt_id:
                continue
            history = wait_for_completion(prompt_id, timeout=300)
            if not history:
                print(f"    TIMEOUT")
                continue
            for nid, nout in history.get("outputs", {}).items():
                for img_info in nout.get("images", []):
                    download_image(img_info["filename"], img_info.get("subfolder", ""), img_path)
                    postprocess(img_path, tex_name)
                    print(f"    OK")
                    success += 1
                    break
                break

    print(f"\n{'=' * 60}")
    print(f"  DONE: {success}/{total} textures generated")
    print(f"  Output: {OUTPUT_DIR}")
    print(f"{'=' * 60}")

if __name__ == "__main__":
    main()
