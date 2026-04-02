"""
MEGA RO ZONE TEXTURE GENERATOR
Generates textures for 23 specific RO Classic zones using exact color palettes
from research. Each zone gets 8 textures (3 ground + 2 variant + 1 dirt + 2 cliff).

Total: ~184 textures. Runtime: ~3 hours on RTX 5090.
"""
import json, urllib.request, urllib.parse, time, os, sys
import numpy as np
from PIL import Image, ImageEnhance

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_ROOT = "C:/Sabri_MMO/_tools/ground_texture_output/ro_zones"
CHECKPOINT = "Illustrious-XL-v0.1.safetensors"
LORA = "ROSprites-v2.1C.safetensors"

BASE_NEG = (
    "photorealistic, photograph, 3d render, plastic, glossy, shiny, specular, "
    "metallic, blurry, low quality, worst quality, watermark, signature, text, "
    "dramatic lighting, high contrast, neon, "
    "person, character, face, items, objects, perspective, sky, horizon, "
    "side view, grid, symmetric, border, frame, vignette, "
    "individual leaves, grass blades, clover, shamrock, stems, flowers, "
    "pixel art, mosaic, repeating pattern, stripes, abstract shapes"
)

# Zone texture definitions: (name, prompt, lora_strength, seed)
# Prompts use exact colors from research
ZONES = {
    "prontera_fields": {
        "neg_extra": ", cold blue, purple, dark, desert",
        "textures": [
            ("T_Prt_Meadow_A", "overhead view of painted meadow ground, oil painting style, muted sage green and olive green color fields, warm golden undertone, pastoral sunlit meadow, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80101),
            ("T_Prt_Meadow_B", "overhead view of painted meadow ground, oil painting style, warm yellow-green and soft olive patches, bright cheerful meadow, golden afternoon light feeling, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80102),
            ("T_Prt_Meadow_C", "overhead view of painted meadow ground, oil painting style, light sage green blending with pale golden-green, open field pastoral palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80103),
            ("T_Prt_Path_A", "overhead view of painted golden dirt path, oil painting style, dusty golden ochre packed earth, warm tan with subtle grain, country road surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80111),
            ("T_Prt_Path_B", "overhead view of painted country path, oil painting style, warm sandy-tan earth with lighter cream edges, well-trodden meadow path, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80112),
            ("T_Prt_Cliff_A", "painted warm brown earth cliff, oil painting style, exposed tan and brown rock layers, warm meadow hillside cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80121),
            ("T_Prt_Cliff_B", "painted light gray-brown stone cliff, oil painting style, warm cream and tan layered rock, pastoral hill cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80122),
        ]
    },
    "geffen": {
        "neg_extra": ", warm, orange, desert, sand, tropical",
        "textures": [
            ("T_Gef_Ground_A", "overhead view of painted cool blue-gray stone pavement, oil painting style, dark slate stone with subtle purple tint, magical city ground, cool mysterious palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80201),
            ("T_Gef_Ground_B", "overhead view of painted cool forest floor, oil painting style, deep blue-green and dark sage color fields, cool shaded forest ground, mysterious palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80202),
            ("T_Gef_Ground_C", "overhead view of painted cool green ground, oil painting style, forest green with blue undertone, deep shade palette, cooler than warm meadow, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80203),
            ("T_Gef_Dirt_A", "overhead view of painted dark earth, oil painting style, cool gray-brown soil, shaded forest path, neutral cool palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80211),
            ("T_Gef_Cliff_A", "painted dark gray stone with purple moss, oil painting style, cool blue-gray rock face, magical forest cliff, mysterious palette, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80221),
            ("T_Gef_Cliff_B", "painted dark slate rock surface, oil painting style, cool gray stone with faint purple veins, arcane cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80222),
        ]
    },
    "payon_forest": {
        "neg_extra": ", bright, desert, sand, snow, urban",
        "textures": [
            ("T_Pay_Forest_A", "overhead view of painted dark forest floor, oil painting style, deep emerald green with warm brown leaf litter patches, bamboo forest ground, dappled shade palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80301),
            ("T_Pay_Forest_B", "overhead view of painted autumn forest floor, oil painting style, dark green moss with scattered amber-brown fallen leaves, Korean forest ground, warm and shaded, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80302),
            ("T_Pay_Forest_C", "overhead view of painted shaded woodland floor, oil painting style, rich dark olive green and reddish-brown earth, dense canopy forest ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80303),
            ("T_Pay_Dirt_A", "overhead view of painted dark reddish-brown forest path, oil painting style, narrow packed earth trail through dark forest, warm brown with red undertone, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80311),
            ("T_Pay_Cliff_A", "painted mossy dark brown rock cliff, oil painting style, warm brown stone heavily covered in green moss, forest boulder cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80321),
            ("T_Pay_Cliff_B", "painted dark stone with moss staining, oil painting style, gray-brown rock with green-brown moss patches, forest cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80322),
        ]
    },
    "morroc_desert": {
        "neg_extra": ", green, grass, forest, cold, snow, blue",
        "textures": [
            ("T_Moc_Sand_A", "overhead view of painted desert sand, oil painting style, warm golden amber sand, sun-baked Saharan palette, bright hot desert ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80401),
            ("T_Moc_Sand_B", "overhead view of painted pale desert ground, oil painting style, bleached bone-white sand with warm amber undertone, scorching desert surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80402),
            ("T_Moc_Sand_C", "overhead view of painted warm desert ground, oil painting style, deep amber and golden-brown sand dunes, hot desert palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80403),
            ("T_Moc_Cracked_A", "overhead view of painted dry cracked desert earth, oil painting style, lighter tan cracked ground with dark crack lines, parched desert hardpan, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80411),
            ("T_Moc_Sandstone_A", "painted layered sandstone cliff, oil painting style, warm amber and terracotta banded sedimentary rock, desert canyon wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80421),
            ("T_Moc_Sandstone_B", "painted wind-eroded sandstone, oil painting style, golden-brown weathered desert rock, mesa cliff wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80422),
        ]
    },
    "lutie_snow": {
        "neg_extra": ", green, grass, warm, orange, desert, tropical",
        "textures": [
            ("T_Lut_Snow_A", "overhead view of painted snow surface, oil painting style, white snow with subtle pale blue shadows, soft crystalline winter ground, cold festive palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80501),
            ("T_Lut_Snow_B", "overhead view of painted packed snow path, oil painting style, slightly gray-white packed snow with blue shadow hints, winter walkway surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80502),
            ("T_Lut_Snow_C", "overhead view of painted fresh snow, oil painting style, bright white snow with faint warm pink-white tint, Christmas village ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80503),
            ("T_Lut_Frost_A", "overhead view of painted frozen earth, oil painting style, dark gray-brown frozen dirt with white frost crystal patterns, cold winter ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80511),
            ("T_Lut_IceCliff_A", "painted frozen cliff face, oil painting style, dark gray-blue ice-covered rock with icicle formations, winter mountain cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 80521),
            ("T_Lut_IceCliff_B", "painted frost-covered stone, oil painting style, pale gray rock with blue ice patches and white frost, snow cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 80522),
        ]
    },
    "glast_heim": {
        "neg_extra": ", bright, cheerful, green grass, tropical, warm, sunny",
        "textures": [
            ("T_GH_Cursed_A", "overhead view of painted cursed ground, oil painting style, dark gray-green corrupted earth, sickly desaturated colors, dead vegetation tones, haunted castle courtyard, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80601),
            ("T_GH_Cursed_B", "overhead view of painted haunted ground, oil painting style, deep purple-gray dead earth with dark olive decay, gothic horror ground surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80602),
            ("T_GH_Cursed_C", "overhead view of painted decayed stone ground, oil painting style, crumbling gray-green stone with dark staining, once-grand now-corrupted palace floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80603),
            ("T_GH_Blood_A", "overhead view of painted dark bloodstained stone, oil painting style, dark slate gray with deep crimson-brown staining, horror dungeon floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80611),
            ("T_GH_Wall_A", "painted crumbling dark castle wall, oil painting style, dark gray-green mold-stained stone blocks, corrupted fortress wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80621),
            ("T_GH_Wall_B", "painted haunted stone surface, oil painting style, dark purple-black ancient stone with sickly green cracks, cursed castle wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80622),
        ]
    },
    "niflheim": {
        "neg_extra": ", bright, cheerful, green, warm, sunny, tropical, colorful",
        "textures": [
            ("T_Nif_Dead_A", "overhead view of painted dead earth ground, oil painting style, ash gray lifeless soil with no green, completely dead desaturated ground, underworld palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80701),
            ("T_Nif_Dead_B", "overhead view of painted gray wasteland ground, oil painting style, muted purple-gray dead earth, sickly and devoid of life, ghostly pale undertone, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80702),
            ("T_Nif_Dead_C", "overhead view of painted corrupted soil, oil painting style, gray-brown dead ground with faint sickly purple tint, Norse underworld surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80703),
            ("T_Nif_Ash_A", "overhead view of painted ash-covered ground, oil painting style, pale gray ash and dark gray dead soil, lifeless wasteland floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80711),
            ("T_Nif_Wall_A", "painted dead gray rock surface, oil painting style, dark gray stone with purple-tinted shadows, underworld cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 80721),
            ("T_Nif_Wall_B", "painted lifeless dark stone, oil painting style, cold gray-purple rock face, desolate underworld cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 80722),
        ]
    },
    "veins_volcanic": {
        "neg_extra": ", green, grass, snow, cold, blue, pastoral",
        "textures": [
            ("T_Vns_RedRock_A", "overhead view of painted red volcanic rock ground, oil painting style, deep crimson-red and burnt orange rock surface, hot volcanic canyon floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80801),
            ("T_Vns_RedRock_B", "overhead view of painted scorched volcanic ground, oil painting style, dark red-brown and black charred rock, heat-cracked volcanic surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80802),
            ("T_Vns_RedRock_C", "overhead view of painted volcanic hardpan, oil painting style, dark reddish-brown packed volcanic earth, scorching heat palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80803),
            ("T_Vns_Lava_A", "overhead view of painted black basalt with orange lava cracks, oil painting style, dark volcanic rock with glowing orange-red fissures, hellish ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 80811),
            ("T_Vns_Canyon_A", "painted red canyon cliff wall, oil painting style, dramatic red-orange layered sedimentary rock, volcanic canyon cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80821),
            ("T_Vns_Canyon_B", "painted dark volcanic cliff, oil painting style, black basalt and dark red rock with deep cracks, volcanic cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 80822),
        ]
    },
    "amatsu": {
        "neg_extra": ", desert, sand, snow, industrial, western",
        "textures": [
            ("T_Amt_Earth_A", "overhead view of painted Japanese garden ground, oil painting style, dark warm brown packed earth with subtle reddish tint, zen garden path, serene Japanese palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80901),
            ("T_Amt_Earth_B", "overhead view of painted temple path ground, oil painting style, dark brown earth with scattered cherry blossom pink petal hints, Japanese garden ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80902),
            ("T_Amt_Stone_A", "overhead view of painted gray stepping stone path, oil painting style, cool gray stone pavers in dark earth, Japanese garden walkway, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80903),
            ("T_Amt_ZenSand_A", "overhead view of painted zen garden sand, oil painting style, pale cream-beige raked sand with subtle ripple patterns, Japanese rock garden surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 80911),
            ("T_Amt_Cliff_A", "painted gray-brown rocky terrain with bamboo, oil painting style, Japanese hillside rock with green bamboo accents, temple mountain cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 80921),
            ("T_Amt_Cliff_B", "painted dark lacquered wood and stone, oil painting style, dark brown traditional Japanese architectural surface, temple wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 80922),
        ]
    },
    "jawaii_beach": {
        "neg_extra": ", dark, gloomy, cold, snow, industrial, dead, cursed",
        "textures": [
            ("T_Jaw_Sand_A", "overhead view of painted bright tropical beach sand, oil painting style, pale golden-cream warm sand, paradise beach ground, bright tropical palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81001),
            ("T_Jaw_Sand_B", "overhead view of painted tropical white sand, oil painting style, bright warm white-gold dry beach sand, island paradise ground, cheerful palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81002),
            ("T_Jaw_Sand_C", "overhead view of painted peach-tinted beach, oil painting style, warm peach and cream tropical sand with faint aqua water tint at edges, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81003),
            ("T_Jaw_WetSand_A", "overhead view of painted wet tropical sand, oil painting style, darker tan wet sand with wave-line debris, tropical shoreline ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81011),
            ("T_Jaw_CoralRock_A", "painted white limestone coastal cliff, oil painting style, bright cream and pale gray weathered coastal rock, tropical cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81021),
            ("T_Jaw_CoralRock_B", "painted sandy coral rock surface, oil painting style, warm tan and pink-tinted porous tropical rock, beach cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81022),
        ]
    },
    "einbroch": {
        "neg_extra": ", green, grass, tropical, bright, cheerful, pastoral",
        "textures": [
            ("T_Ein_Metal_A", "overhead view of painted industrial metal floor, oil painting style, dark gray riveted steel plate surface, factory ground, steampunk palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81101),
            ("T_Ein_Metal_B", "overhead view of painted dirty concrete floor, oil painting style, gray concrete with dark oil stain patches, industrial yard ground, grimy palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81102),
            ("T_Ein_Rust_A", "overhead view of painted rusty metal surface, oil painting style, orange-brown rust and dark gray corroded iron, industrial decay surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81103),
            ("T_Ein_Soot_A", "overhead view of painted ash and soot ground, oil painting style, dark gray industrial ash with brown pollution staining, polluted factory ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81111),
            ("T_Ein_Wall_A", "painted soot-stained industrial brick, oil painting style, dark red-brown brick with black soot marks, factory wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81121),
            ("T_Ein_Wall_B", "painted corrugated rusty metal panels, oil painting style, dark gray and rust-orange industrial wall, steampunk factory surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81122),
        ]
    },
    "yuno_marble": {
        "neg_extra": ", dark, gloomy, dirty, rust, industrial, dead",
        "textures": [
            ("T_Yun_Marble_A", "overhead view of painted white marble floor, oil painting style, cool white-gray marble with subtle veining, academic sky city ground, elegant clean palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81201),
            ("T_Yun_Marble_B", "overhead view of painted light stone pavement, oil painting style, pale cream-white polished stone, floating island plaza, bright airy palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81202),
            ("T_Yun_Alpine_A", "overhead view of painted alpine meadow ground, oil painting style, pale thin green grass on rocky soil, high altitude sparse vegetation, cool mountain palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81203),
            ("T_Yun_Gravel_A", "overhead view of painted mountain gravel path, oil painting style, light gray exposed rock and thin soil, high plateau ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81211),
            ("T_Yun_Cliff_A", "painted light gray mountain rock cliff, oil painting style, clean sharp-edged gray mountain stone, sky city cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81221),
            ("T_Yun_Cliff_B", "painted white stone cliff edge, oil painting style, pale marble-like cliff face with sky visible, floating island edge, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81222),
        ]
    },
    "mjolnir_mountain": {
        "neg_extra": ", tropical, beach, desert sand, urban, snow",
        "textures": [
            ("T_Mjo_Rock_A", "overhead view of painted mountain path ground, oil painting style, gray-brown rocky trail with sparse vegetation, rugged mountain path, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81301),
            ("T_Mjo_Rock_B", "overhead view of painted rocky highland ground, oil painting style, gray gravel and brown packed dirt, mountain plateau surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81302),
            ("T_Mjo_Pine_A", "overhead view of painted pine forest mountain floor, oil painting style, dark pine green and gray-brown needle litter, mountain forest ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81303),
            ("T_Mjo_Gravel_A", "overhead view of painted loose mountain gravel, oil painting style, gray and brown crushed rock and loose stones, mountain trail surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81311),
            ("T_Mjo_Granite_A", "painted dark gray granite cliff, oil painting style, rugged angular dark gray mountain rock face, steep mountain cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81321),
            ("T_Mjo_Granite_B", "painted gray layered mountain rock, oil painting style, gray-brown sedimentary mountain cliff, wind-eroded rock face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81322),
        ]
    },
    "moscovia": {
        "neg_extra": ", tropical, desert, sand, snow, urban, industrial",
        "textures": [
            ("T_Mos_Autumn_A", "overhead view of painted autumn forest floor, oil painting style, dark rich brown earth with scattered golden-orange fallen leaves, Russian birch forest ground, autumnal palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81401),
            ("T_Mos_Autumn_B", "overhead view of painted autumn path ground, oil painting style, dark brown packed earth with amber and gold leaf scatter, rustic forest village path, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81402),
            ("T_Mos_DarkForest_A", "overhead view of painted dark northern forest floor, oil painting style, very dark green-brown pine forest ground, deep shade with pine needle carpet, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81403),
            ("T_Mos_Leaf_A", "overhead view of painted fallen leaf carpet, oil painting style, warm amber-brown and gold fallen leaves covering dark earth, autumn forest ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81411),
            ("T_Mos_Cliff_A", "painted dark forest rock partially covered by fallen leaves, oil painting style, dark brown-gray stone with autumn leaf patches, forest cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81421),
            ("T_Mos_Cliff_B", "painted dark northern rock surface, oil painting style, dark gray-brown rugged stone, northern forest cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81422),
        ]
    },
    "comodo_cave": {
        "neg_extra": ", bright daylight, snow, desert, urban, industrial",
        "textures": [
            ("T_Com_CaveSand_A", "overhead view of painted cave beach sand, oil painting style, warm slightly pink-tinted sandy tan, tropical cave floor, torchlit warm palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81501),
            ("T_Com_CaveSand_B", "overhead view of painted exotic cave ground, oil painting style, warm tan sand with darker shadow patches, underground beach cave, festive night palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81502),
            ("T_Com_WetRock_A", "overhead view of painted wet cave rock, oil painting style, dark brown-gray moisture-stained cave floor, underground lake shore, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81503),
            ("T_Com_DarkSand_A", "overhead view of painted dark wet sand, oil painting style, darker warm brown wet sand near underground water, cave shoreline ground, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81511),
            ("T_Com_CaveWall_A", "painted dark cave rock wall, oil painting style, dark brown-gray irregular cave rock with stalactite drip stains, underground cliff, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81521),
            ("T_Com_CaveWall_B", "painted very dark cave stone, oil painting style, near-black wet cave rock with moisture sheen, deep underground wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81522),
        ]
    },
    "umbala_jungle": {
        "neg_extra": ", snow, desert, sand, urban, industrial, bright",
        "textures": [
            ("T_Umb_Jungle_A", "overhead view of painted dense jungle floor, oil painting style, very dark tropical green and warm brown, primeval jungle ground, thick canopy shade, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81601),
            ("T_Umb_Jungle_B", "overhead view of painted tropical undergrowth ground, oil painting style, rich dark green fern carpet with brown earth, dense jungle floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81602),
            ("T_Umb_Wood_A", "overhead view of painted dark tropical wood platform, oil painting style, very dark brown heavy tropical timber planks, treehouse floor surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81603),
            ("T_Umb_Mud_A", "overhead view of painted tropical mud ground, oil painting style, dark red-brown wet jungle earth, tropical rain forest path, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81611),
            ("T_Umb_Bark_A", "painted massive tree bark surface, oil painting style, very dark brown vertical tree bark texture, giant tree trunk cliff face, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81621),
            ("T_Umb_VineRock_A", "painted jungle cliff covered in vines, oil painting style, dark brown stone with thick green vine overgrowth, tropical cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81622),
        ]
    },
    "rachel_frozen": {
        "neg_extra": ", warm, green, grass, tropical, desert, cheerful",
        "textures": [
            ("T_Rac_IceStone_A", "overhead view of painted frozen white stone, oil painting style, ice-white stone pavement with pale blue tint, frozen holy city ground, cold sacred palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81701),
            ("T_Rac_IceStone_B", "overhead view of painted frozen tundra ground, oil painting style, white snow over dark frozen earth patches, harsh frozen wasteland, desolate cold palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81702),
            ("T_Rac_Tundra_A", "overhead view of painted frozen earth, oil painting style, dark gray-blue frozen soil with ice crystal patches, arctic tundra ground, harsh winter palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81703),
            ("T_Rac_FrostDirt_A", "overhead view of painted frost-covered dark ground, oil painting style, dark brown earth with white frost coating, frozen field surface, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81711),
            ("T_Rac_IceCliff_A", "painted frozen mountain cliff, oil painting style, dark gray rock covered in thick blue-white ice, frozen mountain wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81721),
            ("T_Rac_IceCliff_B", "painted ice-encased stone, oil painting style, pale blue-gray frozen cliff face, thick ice formations on rock, arctic cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81722),
        ]
    },
    "louyang": {
        "neg_extra": ", western, desert, snow, industrial, forest",
        "textures": [
            ("T_Lou_Stone_A", "overhead view of painted Chinese imperial stone pavement, oil painting style, warm neutral gray stone with formal layout, grand avenue ground, imperial palace palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81801),
            ("T_Lou_Stone_B", "overhead view of painted gray stone plaza, oil painting style, smooth warm gray pavement, Chinese imperial city ground, dignified palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81802),
            ("T_Lou_Garden_A", "overhead view of painted Chinese rock garden, oil painting style, dark jade-green and cool gray stone, decorative garden ground, imperial garden palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81803),
            ("T_Lou_Path_A", "overhead view of painted formal stone path, oil painting style, warm gray stone pavers with darker mortar, Chinese city walkway, hand-painted game texture, matte, flat lighting, seamless tileable", 0.10, 81811),
            ("T_Lou_MtnRock_A", "painted warm gray mountain rock, oil painting style, neutral gray mountain stone cliff face, Chinese mountain backdrop, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81821),
            ("T_Lou_MtnRock_B", "painted mountain stone surface, oil painting style, gray-brown layered mountain rock, dragon mountain cliff wall, hand-painted game art, matte, flat lighting, seamless tileable", 0.10, 81822),
        ]
    },
    "pyramid": {
        "neg_extra": ", green, grass, snow, cold, modern, industrial",
        "textures": [
            ("T_Pyr_Sandstone_A", "overhead view of painted ancient sandstone floor, oil painting style, warm golden-brown aged sandstone blocks, ancient Egyptian temple floor, archaeological palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81901),
            ("T_Pyr_Sandstone_B", "overhead view of painted worn temple floor, oil painting style, faded warm tan sandstone tiles, ancient pyramid interior floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81902),
            ("T_Pyr_DarkStone_A", "overhead view of painted ancient dark chamber floor, oil painting style, dark golden-brown stone, deep pyramid interior ground, torch-lit ancient palette, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81903),
            ("T_Pyr_Sand_A", "overhead view of painted sandy rubble, oil painting style, warm tan sand and broken stone debris, collapsed temple floor, hand-painted game texture, matte, flat lighting, seamless tileable", 0.12, 81911),
            ("T_Pyr_HieroWall_A", "painted ancient sandstone wall with carved patterns, oil painting style, warm golden sandstone with hieroglyph-like carved texture, pyramid wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81921),
            ("T_Pyr_HieroWall_B", "painted ancient crumbling stone wall, oil painting style, aged dark golden-brown stone blocks, ancient temple wall surface, hand-painted game art, matte, flat lighting, seamless tileable", 0.12, 81922),
        ]
    },
}


def queue_prompt(workflow):
    data = json.dumps({"prompt": workflow}).encode("utf-8")
    req = urllib.request.Request(f"{COMFYUI_URL}/prompt", data=data, headers={"Content-Type": "application/json"})
    try:
        resp = urllib.request.urlopen(req)
        return json.loads(resp.read()).get("prompt_id")
    except urllib.error.HTTPError as e:
        print(f"    Error: {e.read().decode('utf-8', errors='replace')[:200]}")
        return None

def wait_for_completion(prompt_id, timeout=300):
    start = time.time()
    while time.time() - start < timeout:
        try:
            resp = urllib.request.urlopen(f"{COMFYUI_URL}/history/{prompt_id}")
            history = json.loads(resp.read())
            if prompt_id in history: return history[prompt_id]
        except: pass
        time.sleep(2)
    return None

def download_image(filename, subfolder, save_path):
    params = urllib.parse.urlencode({"filename": filename, "subfolder": subfolder, "type": "output"})
    urllib.request.urlretrieve(f"{COMFYUI_URL}/view?{params}", save_path)

def build_workflow(prompt, negative, seed, lora):
    nodes = {"1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CHECKPOINT}}}
    m, cl = ["1", 0], ["1", 1]
    if lora > 0:
        nodes["2"] = {"class_type": "LoraLoader", "inputs": {"model": m, "clip": cl, "lora_name": LORA, "strength_model": lora, "strength_clip": lora}}
        m, cl = ["2", 0], ["2", 1]
    nodes["3"] = {"class_type": "SeamlessTile", "inputs": {"model": m, "tiling": "enable", "copy_model": "Make a copy"}}
    nodes["4"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": cl, "text": prompt}}
    nodes["5"] = {"class_type": "CLIPTextEncode", "inputs": {"clip": cl, "text": negative}}
    nodes["6"] = {"class_type": "EmptyLatentImage", "inputs": {"width": 1024, "height": 1024, "batch_size": 1}}
    nodes["7"] = {"class_type": "KSampler", "inputs": {"model": ["3", 0], "positive": ["4", 0], "negative": ["5", 0], "latent_image": ["6", 0], "seed": seed, "steps": 60, "cfg": 6.5, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0}}
    nodes["8"] = {"class_type": "CircularVAEDecode", "inputs": {"samples": ["7", 0], "vae": ["1", 2], "tiling": "enable"}}
    nodes["9"] = {"class_type": "SaveImage", "inputs": {"images": ["8", 0], "filename_prefix": "rozone"}}
    return nodes

def postprocess(img_path):
    img = Image.open(img_path)
    img = ImageEnhance.Color(img).enhance(0.90)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, 0] *= 1.03
    arr[:, :, 2] *= 0.97
    arr = arr.clip(0, 255).astype(np.uint8)
    Image.fromarray(arr).save(img_path)

def main():
    total = sum(len(z["textures"]) for z in ZONES.values())
    print(f"{'='*60}")
    print(f"  RO ZONE TEXTURE GENERATOR")
    print(f"  {len(ZONES)} zones, {total} textures")
    print(f"  Output: {OUTPUT_ROOT}")
    print(f"{'='*60}\n")
    try: urllib.request.urlopen(f"{COMFYUI_URL}/system_stats", timeout=5)
    except: print("ComfyUI not running!"); sys.exit(1)

    success, idx = 0, 0
    for zone_name, zone in ZONES.items():
        zone_dir = os.path.join(OUTPUT_ROOT, zone_name)
        os.makedirs(zone_dir, exist_ok=True)
        neg = BASE_NEG + zone.get("neg_extra", "")
        print(f"\n=== {zone_name.upper()} ({len(zone['textures'])} textures) ===")

        for tex_name, prompt, lora, seed in zone["textures"]:
            idx += 1
            path = os.path.join(zone_dir, f"{tex_name}.png")
            if os.path.exists(path):
                print(f"  [{idx}/{total}] {tex_name}: exists")
                success += 1; continue
            print(f"  [{idx}/{total}] {tex_name}")
            wf = build_workflow(prompt, neg, seed, lora)
            pid = queue_prompt(wf)
            if not pid: continue
            hist = wait_for_completion(pid)
            if not hist: print("    TIMEOUT"); continue
            for nid, nout in hist.get("outputs", {}).items():
                for img in nout.get("images", []):
                    download_image(img["filename"], img.get("subfolder", ""), path)
                    postprocess(path)
                    print(f"    OK")
                    success += 1; break
                break

    print(f"\n{'='*60}")
    print(f"  DONE: {success}/{total}")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()
