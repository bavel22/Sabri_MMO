-- Migration: Add headgear_view_id to items table
-- Maps headgear items to their visual sprite (111 unique RO Classic view IDs)
-- Multiple items can share a view_id (e.g., Ribbon and Ribbon [1] both use view 17)
-- Source: rAthena pre-re item_db_equip.yml + ai4rei.net viewlist

ALTER TABLE items ADD COLUMN IF NOT EXISTS headgear_view_id INTEGER DEFAULT NULL;

-- View 1: GOGGLES
UPDATE items SET headgear_view_id = 1 WHERE item_id IN (2224, 2225);
-- View 2: CATTYHAIRBAND
UPDATE items SET headgear_view_id = 2 WHERE item_id IN (2213);
-- View 3: GLASS
UPDATE items SET headgear_view_id = 3 WHERE item_id IN (2203, 2204);
-- View 4: FLOWER
UPDATE items SET headgear_view_id = 4 WHERE item_id IN (2207);
-- View 5: FLORALHAIRBAND
UPDATE items SET headgear_view_id = 5 WHERE item_id IN (2215, 5803);
-- View 6: HOOD (Bandana)
UPDATE items SET headgear_view_id = 6 WHERE item_id IN (2211, 5408);
-- View 7: ROUNDCAP (Turban)
UPDATE items SET headgear_view_id = 7 WHERE item_id IN (2222, 2223);
-- View 8: FLUMASK
UPDATE items SET headgear_view_id = 8 WHERE item_id IN (2218, 2219, 5156);
-- View 9: HAIRBAND
UPDATE items SET headgear_view_id = 9 WHERE item_id IN (2210);
-- View 10: DIVERSGOGGLES
UPDATE items SET headgear_view_id = 10 WHERE item_id IN (2205);
-- View 11: BIRETTA
UPDATE items SET headgear_view_id = 11 WHERE item_id IN (2216, 2217);
-- View 12: SUNGLASS
UPDATE items SET headgear_view_id = 12 WHERE item_id IN (2201, 2202, 5154, 19504);
-- View 13: EYEBANDAGE
UPDATE items SET headgear_view_id = 13 WHERE item_id IN (2212, 5804);
-- View 14: CAP
UPDATE items SET headgear_view_id = 14 WHERE item_id IN (2226, 2227);
-- View 15: BUNNYBAND
UPDATE items SET headgear_view_id = 15 WHERE item_id IN (2214, 5218, 5266, 5553);
-- View 16: HAT
UPDATE items SET headgear_view_id = 16 WHERE item_id IN (2220, 2221, 5144, 5812);
-- View 17: RIBBON
UPDATE items SET headgear_view_id = 17 WHERE item_id IN (2208, 2209);
-- View 18: CIRCLET
UPDATE items SET headgear_view_id = 18 WHERE item_id IN (2232, 2233, 5540, 5541, 5542, 5543);
-- View 19: TIARA
UPDATE items SET headgear_view_id = 19 WHERE item_id IN (2234, 5164);
-- View 20: SANTAHAT
UPDATE items SET headgear_view_id = 20 WHERE item_id IN (2236, 5136);
-- View 21: BEARD (Bandit Beard)
UPDATE items SET headgear_view_id = 21 WHERE item_id IN (2237);
-- View 22: MUSTACHE
UPDATE items SET headgear_view_id = 22 WHERE item_id IN (2238);
-- View 23: SPECTACLE (Monocle)
UPDATE items SET headgear_view_id = 23 WHERE item_id IN (2239, 5516, 5517);
-- View 24: BLACK_BEARD
UPDATE items SET headgear_view_id = 24 WHERE item_id IN (2240);
-- View 25: WHITE_BEARD (Grampa Beard)
UPDATE items SET headgear_view_id = 25 WHERE item_id IN (2241, 5155, 5811);
-- View 26: QUALITY_SUNGLASS (Purple Glasses)
UPDATE items SET headgear_view_id = 26 WHERE item_id IN (2242);
-- View 27: SPIN_GLASS (Geek Glasses)
UPDATE items SET headgear_view_id = 27 WHERE item_id IN (2243);
-- View 28: LARGE_RIBBON (Big Ribbon)
UPDATE items SET headgear_view_id = 28 WHERE item_id IN (2244, 5348);
-- View 29: SWEET_GENTLE
UPDATE items SET headgear_view_id = 29 WHERE item_id IN (2245);
-- View 30: GOLDEN_HEADGEAR
UPDATE items SET headgear_view_id = 30 WHERE item_id IN (2246, 5159);
-- View 31: OLDSTER_ROMANCE (Romantic Gent)
UPDATE items SET headgear_view_id = 31 WHERE item_id IN (2247);
-- View 32: WESTERN_GRACE
UPDATE items SET headgear_view_id = 32 WHERE item_id IN (2248);
-- View 33: CORONET
UPDATE items SET headgear_view_id = 33 WHERE item_id IN (2249);
-- View 34: HAIR_STRING (Cute Ribbon)
UPDATE items SET headgear_view_id = 34 WHERE item_id IN (2250);
-- View 35: PRIEST_CAP (Monk Hat)
UPDATE items SET headgear_view_id = 35 WHERE item_id IN (2251, 5158);
-- View 36: WIZARD_HAT
UPDATE items SET headgear_view_id = 36 WHERE item_id IN (2252);
-- View 37: SUNFLOWER
UPDATE items SET headgear_view_id = 37 WHERE item_id IN (2253, 5351);
-- View 38: ANGEL_HAIRBAND (Angel Wing)
UPDATE items SET headgear_view_id = 38 WHERE item_id IN (2254, 5215, 5368);
-- View 39: DEVIL_HAIRBAND (Evil Wing)
UPDATE items SET headgear_view_id = 39 WHERE item_id IN (2255, 5216, 5369);
-- View 40: HELM
UPDATE items SET headgear_view_id = 40 WHERE item_id IN (2228, 2229);
-- View 41: MAJESTIC_GOUT
UPDATE items SET headgear_view_id = 41 WHERE item_id IN (2256, 5160, 5217, 5280, 5683);
-- View 42: WHITE_HORN (Unicorn Horn)
UPDATE items SET headgear_view_id = 42 WHERE item_id IN (2257);
-- View 43: SHARP_HEADGEAR (Spiky Band)
UPDATE items SET headgear_view_id = 43 WHERE item_id IN (2258, 5161, 5805);
-- View 44: WEDDING_VEIL
UPDATE items SET headgear_view_id = 44 WHERE item_id IN (2206);
-- View 45: CROWN
UPDATE items SET headgear_view_id = 45 WHERE item_id IN (2235, 5165);
-- View 46: PROPELLER (Mini Propeller)
UPDATE items SET headgear_view_id = 46 WHERE item_id IN (2259);
-- View 47: TINY_EYE_GLASSES (Mini Glasses)
UPDATE items SET headgear_view_id = 47 WHERE item_id IN (2260);
-- View 48: PRONTERA_ARMY_CAP
UPDATE items SET headgear_view_id = 48 WHERE item_id IN (2261, 5685);
-- View 49: PIERROTS_NOSE (Clown Nose)
UPDATE items SET headgear_view_id = 49 WHERE item_id IN (2262, 5204);
-- View 50: PIRATES_EYEBANDAGE (Zorro Masque)
UPDATE items SET headgear_view_id = 50 WHERE item_id IN (2263);
-- View 51: MUNAK_TURBAN
UPDATE items SET headgear_view_id = 51 WHERE item_id IN (2264, 5167);
-- View 52: HIP_HOP_MASK (Gangster Mask)
UPDATE items SET headgear_view_id = 52 WHERE item_id IN (2265);
-- View 53: IRONCANE (Iron Cain)
UPDATE items SET headgear_view_id = 53 WHERE item_id IN (2266);
-- View 54: CIGAR (Cigarette)
UPDATE items SET headgear_view_id = 54 WHERE item_id IN (2267, 19505);
-- View 55: CIGAR_PIPE (Pipe)
UPDATE items SET headgear_view_id = 55 WHERE item_id IN (2268, 5220);
-- View 56: SENTIMENTAL_FLOWER (Romantic Flower)
UPDATE items SET headgear_view_id = 56 WHERE item_id IN (2269, 2687);
-- View 57: SENTIMENTAL_BLADE (Romantic Leaf)
UPDATE items SET headgear_view_id = 57 WHERE item_id IN (2270, 5419);
-- View 58: HEY_DUDE (Jack be Dandy)
UPDATE items SET headgear_view_id = 58 WHERE item_id IN (2271);
-- View 59: STOP_SIGNPOST
UPDATE items SET headgear_view_id = 59 WHERE item_id IN (2272);
-- View 60: DOCTOR_BAND
UPDATE items SET headgear_view_id = 60 WHERE item_id IN (2273);
-- View 61: JAPANESE_GHOST (Ghost Bandana)
UPDATE items SET headgear_view_id = 61 WHERE item_id IN (2274);
-- View 62: SCARLET_BANDANA (Red Bandana)
UPDATE items SET headgear_view_id = 62 WHERE item_id IN (2275);
-- View 63: EAGLE_EYES (Angled Glasses)
UPDATE items SET headgear_view_id = 63 WHERE item_id IN (2276);
-- View 64: NURSE_CAP
UPDATE items SET headgear_view_id = 64 WHERE item_id IN (2277);
-- View 65: SMILE (Mr. Smile)
UPDATE items SET headgear_view_id = 65 WHERE item_id IN (2278, 19500);
-- View 66: BOMB_WICK
UPDATE items SET headgear_view_id = 66 WHERE item_id IN (2279);
-- View 67: SAHT_GAHT (Sakkat)
UPDATE items SET headgear_view_id = 67 WHERE item_id IN (2280, 5267, 5806);
-- View 68: MASQUERADE (Opera Masque)
UPDATE items SET headgear_view_id = 68 WHERE item_id IN (2281);
-- View 69: SERAPHIC_RING (Halo)
UPDATE items SET headgear_view_id = 69 WHERE item_id IN (2282);
-- View 70: EARMUFFS
UPDATE items SET headgear_view_id = 70 WHERE item_id IN (2283);
-- View 71: MOOSE_HORN (Antlers)
UPDATE items SET headgear_view_id = 71 WHERE item_id IN (2284);
-- View 72: THE_APPLE_OF_WILHELM_TELL
UPDATE items SET headgear_view_id = 72 WHERE item_id IN (2285, 5265);
-- View 73: TINKER_BELL (Elven Ears)
UPDATE items SET headgear_view_id = 73 WHERE item_id IN (2286, 2686, 18507);
-- View 74: PIRATE_BANDANA
UPDATE items SET headgear_view_id = 74 WHERE item_id IN (2287, 5350);
-- View 75: MUNCHS_SCREAM (Mr. Scream)
UPDATE items SET headgear_view_id = 75 WHERE item_id IN (2288);
-- View 76: POO_POO_HAT
UPDATE items SET headgear_view_id = 76 WHERE item_id IN (2289);
-- View 77: KOEAN_FUNERAL_COSTUME (Funeral Hat)
UPDATE items SET headgear_view_id = 77 WHERE item_id IN (2290);
-- View 78: BUTTERFLY_MASQUE (Masquerade)
UPDATE items SET headgear_view_id = 78 WHERE item_id IN (2291, 5326);
-- View 79: WELDER_MASK
UPDATE items SET headgear_view_id = 79 WHERE item_id IN (2292);
-- View 80: PRETEND_TO_BE_MURDERED
UPDATE items SET headgear_view_id = 80 WHERE item_id IN (2293);
-- View 81: STAR_DUST (Stellar)
UPDATE items SET headgear_view_id = 81 WHERE item_id IN (2294);
-- View 82: EYE_MASK (Blinker)
UPDATE items SET headgear_view_id = 82 WHERE item_id IN (2295);
-- View 83: BINOCULARS
UPDATE items SET headgear_view_id = 83 WHERE item_id IN (2296);
-- View 84: GOBLIN_MASQUE
UPDATE items SET headgear_view_id = 84 WHERE item_id IN (2297);
-- View 85: GREEN_FEELER
UPDATE items SET headgear_view_id = 85 WHERE item_id IN (2298);
-- View 86: VIKING_HELMET (Orc Helm)
UPDATE items SET headgear_view_id = 86 WHERE item_id IN (2299, 5157, 5687);
-- View 87: HEAD_SET
UPDATE items SET headgear_view_id = 87 WHERE item_id IN (5001);
-- View 88: GEMMED_CROWN (Jewel Crown)
UPDATE items SET headgear_view_id = 88 WHERE item_id IN (5002, 5684);
-- View 89: PIERROT_CROWN (Joker Jester)
UPDATE items SET headgear_view_id = 89 WHERE item_id IN (5003, 5145);
-- View 90: OXYGEN_MASK
UPDATE items SET headgear_view_id = 90 WHERE item_id IN (5004);
-- View 91: GAS_MASK
UPDATE items SET headgear_view_id = 91 WHERE item_id IN (5005);
-- View 92: MACHO_MANS_GLASSES
UPDATE items SET headgear_view_id = 92 WHERE item_id IN (5006);
-- View 93: LORD_CIRCLET (Grand Circlet)
UPDATE items SET headgear_view_id = 93 WHERE item_id IN (5007, 5268, 5552);
-- View 94: PUPPY_LOVE
UPDATE items SET headgear_view_id = 94 WHERE item_id IN (5008);
-- View 95: CRASH_HELMET (Safety Helmet)
UPDATE items SET headgear_view_id = 95 WHERE item_id IN (5009);
-- View 96: INDIAN_BAND (Indian Fillet)
UPDATE items SET headgear_view_id = 96 WHERE item_id IN (5010);
-- View 97: AERIAL
UPDATE items SET headgear_view_id = 97 WHERE item_id IN (5011);
-- View 98: BA_HAT (Ph.D Hat)
UPDATE items SET headgear_view_id = 98 WHERE item_id IN (5012, 5347, 5810);
-- View 99: HORN_OF_LORD_KAHO
UPDATE items SET headgear_view_id = 99 WHERE item_id IN (5013);
-- View 100: FIN_HELM
UPDATE items SET headgear_view_id = 100 WHERE item_id IN (5014);
-- View 101: EGG_SHELL
UPDATE items SET headgear_view_id = 101 WHERE item_id IN (5015, 5055, 5741);
-- View 102: KOREAN_SCHOOL_HAT (Boy's Cap)
UPDATE items SET headgear_view_id = 102 WHERE item_id IN (5016, 5349, 5492);
-- View 103: BONE_HELM
UPDATE items SET headgear_view_id = 103 WHERE item_id IN (5017, 5162);
-- View 104: WILHELM_TELLS_HAT (Feather Bonnet)
UPDATE items SET headgear_view_id = 104 WHERE item_id IN (5018, 5686, 5807);
-- View 105: CORSAIR
UPDATE items SET headgear_view_id = 105 WHERE item_id IN (5019, 5163, 5331);
-- View 106: BAND_OF_KAFRA
UPDATE items SET headgear_view_id = 106 WHERE item_id IN (5020);
-- View 107: LOST_MONEY_HEART (Grief for Greed)
UPDATE items SET headgear_view_id = 107 WHERE item_id IN (5021);
-- View 108: BUNDLE (Parcel Hat)
UPDATE items SET headgear_view_id = 108 WHERE item_id IN (5023);
-- View 109: CAKE_HAT
UPDATE items SET headgear_view_id = 109 WHERE item_id IN (5024, 5105, 5771);
-- View 110: ANGEL_HELM (Helm of Angel)
UPDATE items SET headgear_view_id = 110 WHERE item_id IN (5025);
-- View 111: COOKER_HAT (Chef Hat)
UPDATE items SET headgear_view_id = 111 WHERE item_id IN (5026);

-- Create index for view_id lookups
CREATE INDEX IF NOT EXISTS idx_items_headgear_view_id ON items(headgear_view_id) WHERE headgear_view_id IS NOT NULL;
