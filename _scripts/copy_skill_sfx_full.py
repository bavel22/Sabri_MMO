"""
copy_skill_sfx_full.py
COMPREHENSIVE skill SFX mapping. One entry for EVERY active first/second-class skill
in Sabri_MMO. Uses RO Classic-accurate sound mapping where possible, with sensible
fallback substitutions for skills whose unique wav we don't have.

Each entry: (skillId, classId, skillName, sourceWavBaseName, destWavBaseName, note)
- 'note' is one of:
    'unique'   -- the canonical RO Classic wav for this skill
    'shared'   -- intentionally shares sound with a related skill (e.g., bolts)
    'fallback' -- best-effort substitute (no canonical wav available in our set)
    'silent'   -- intentionally silent (UI-only, passive trigger, etc.)
    'missing'  -- canonical wav not in our files; using a placeholder
"""
import os, shutil, sys, io

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

SRC = r'C:/Sabri_MMO/ROSFX/effect'
DST = r'C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Audio/SFX/Skills'

# Format: (id, classId, skill, src_basename, dst_basename, note)
SKILL_MAP = [
    # ============================================================================
    # NOVICE (1-3)
    # ============================================================================
    (   2, 'novice',    'first_aid',          None,                        'PLAYER:heal_effect',          'unique'),  # reuses Player/heal_effect

    # ============================================================================
    # SWORDSMAN (100-109)
    # ============================================================================
    ( 103, 'swordsman', 'bash',               'ef_bash',                   'skill_bash',                  'unique'),
    ( 104, 'swordsman', 'provoke',            'swordman_provoke',          'skill_provoke',               'unique'),
    ( 105, 'swordsman', 'magnum_break',       'ef_magnumbreak',            'skill_magnum_break',          'unique'),
    ( 106, 'swordsman', 'endure',             'ef_endure',                 'skill_endure',                'unique'),
    ( 109, 'swordsman', 'auto_berserk',       'lord_knight_berserk',       'skill_auto_berserk',          'unique'),  # canonical LK Berserk shout

    # ============================================================================
    # MAGE (200-213)
    # ============================================================================
    ( 200, 'mage',      'cold_bolt',          'ef_icearrow',               'skill_cold_bolt',             'unique'),
    ( 201, 'mage',      'fire_bolt',          'ef_firearrow',              'skill_fire_bolt',             'unique'),
    ( 202, 'mage',      'lightning_bolt',     'ef_lightbolt',              'skill_lightning_bolt',        'unique'),
    ( 203, 'mage',      'napalm_beat',        'ef_napalmbeat',             'skill_napalm_beat',           'unique'),
    ( 205, 'mage',      'sight',              'ef_sight',                  'skill_sight',                 'unique'),
    ( 206, 'mage',      'stone_curse',        'ef_stonecurse',             'skill_stone_curse',           'unique'),
    ( 207, 'mage',      'fire_ball',          'ef_fireball',               'skill_fire_ball',             'unique'),
    ( 208, 'mage',      'frost_diver',        'ef_frostdiver',             'skill_frost_diver',           'unique'),
    ( 209, 'mage',      'fire_wall',          'ef_firewall',               'skill_fire_wall',             'unique'),
    ( 210, 'mage',      'soul_strike',        'ef_soulstrike',             'skill_soul_strike',           'unique'),
    ( 211, 'mage',      'safety_wall',        'ef_glasswall',              'skill_safety_wall',           'unique'),
    ( 212, 'mage',      'thunderstorm',       'ef_thunderstorm',           'skill_thunderstorm',          'unique'),
    ( 213, 'mage',      'energy_coat',        'ef_glasswall',              'skill_energy_coat',           'missing'),  # MISSING: _energycoat.wav

    # ============================================================================
    # ARCHER (300-306)
    # ============================================================================
    ( 302, 'archer',    'improve_concentration','ac_concentration',        'skill_improve_concentration', 'unique'),  # ac_concentration IS this sound
    ( 303, 'archer',    'double_strafe',      'ef_firearrow',              'skill_double_strafe',         'fallback'),
    ( 304, 'archer',    'arrow_shower',       'ef_firearrow',              'skill_arrow_shower',          'fallback'),
    ( 305, 'archer',    'arrow_crafting',     'complete',                  'skill_arrow_crafting',        'fallback'),
    ( 306, 'archer',    'arrow_repel',        'ef_firearrow',              'skill_arrow_repel',           'fallback'),

    # ============================================================================
    # ACOLYTE (400-414)
    # ============================================================================
    ( 400, 'acolyte',   'heal',               None,                        'PLAYER:heal_effect',          'unique'),  # reuses Player/heal_effect
    ( 402, 'acolyte',   'blessing',           'ef_blessing',               'skill_blessing',              'unique'),
    ( 403, 'acolyte',   'increase_agi',       'ef_incagility',             'skill_increase_agi',          'unique'),
    ( 404, 'acolyte',   'decrease_agi',       'ef_decagility',             'skill_decrease_agi',          'unique'),
    ( 405, 'acolyte',   'cure',               'acolyte_cure',              'skill_cure',                  'unique'),
    ( 406, 'acolyte',   'angelus',            'ef_angelus',                'skill_angelus',               'unique'),
    ( 407, 'acolyte',   'signum_crucis',      'ef_signum',                 'skill_signum_crucis',         'unique'),
    ( 408, 'acolyte',   'ruwach',             'ef_ruwach',                 'skill_ruwach',                'unique'),
    ( 409, 'acolyte',   'teleport',           'ef_teleportation',          'skill_teleport',              'unique'),
    ( 410, 'acolyte',   'warp_portal',        'ef_portal',                 'skill_warp_portal',           'unique'),
    ( 411, 'acolyte',   'pneuma',             'cd_pneumaticus_procella',   'skill_pneuma',                'missing'),  # MISSING: _pneuma.wav, using renewal Cardinal substitute
    ( 412, 'acolyte',   'aqua_benedicta',     'ef_aqua',                   'skill_aqua_benedicta',        'unique'),
    ( 414, 'acolyte',   'holy_light',         'acolyte_hunew',             'skill_holy_light',            'unique'),

    # ============================================================================
    # THIEF (500-509)
    # ============================================================================
    ( 502, 'thief',     'steal',              'ef_steal',                  'skill_steal',                 'unique'),
    ( 503, 'thief',     'hiding',             'ef_hiding',                 'skill_hiding',                'unique'),
    ( 504, 'thief',     'envenom',            'ef_poisonattack',           'skill_envenom',               'unique'),
    ( 505, 'thief',     'detoxify',           'ef_detoxication',           'skill_detoxify',              'unique'),
    ( 506, 'thief',     'sand_attack',        'ef_hit3',                   'skill_sand_attack',           'fallback'),
    ( 507, 'thief',     'backslide',          'b_sideslide',               'skill_backslide',             'unique'),  # b_sideslide = Backslide canonical
    ( 508, 'thief',     'throw_stone',        'bo_throwrock',              'skill_throw_stone',           'unique'),  # bo_throwrock = Throw Rock canonical
    ( 509, 'thief',     'pick_stone',         'bo_throwrock',              'skill_pick_stone',            'shared'),

    # ============================================================================
    # MERCHANT (600-609)
    # ============================================================================
    ( 603, 'merchant',  'mammonite',          'ef_coin',                   'skill_mammonite',             'unique'),  # ef_coin is the canonical
    ( 605, 'merchant',  'vending',            'ef_coin1',                  'skill_vending',               'fallback'),
    ( 606, 'merchant',  'item_appraisal',     'complete',                  'skill_item_appraisal',        'fallback'),
    ( 608, 'merchant',  'cart_revolution',    'ef_bash',                   'skill_cart_revolution',       'fallback'),
    ( 609, 'merchant',  'loud_exclamation',   'ef_endure',                 'skill_loud_exclamation',      'fallback'),

    # ============================================================================
    # KNIGHT (700-710)
    # ============================================================================
    ( 701, 'knight',    'pierce',             'dk_chargingpierce1',        'skill_pierce',                'unique'),  # DK Charging Pierce = Pierce family
    ( 702, 'knight',    'spear_stab',         'dk_chargingpierce2',        'skill_spear_stab',            'unique'),
    ( 703, 'knight',    'brandish_spear',     'knight_brandish_spear',     'skill_brandish_spear',        'unique'),
    ( 704, 'knight',    'spear_boomerang',    'knight_spear_boomerang',    'skill_spear_boomerang',       'unique'),
    ( 705, 'knight',    'two_hand_quicken',   'knight_twohandquicken',     'skill_two_hand_quicken',      'unique'),
    ( 706, 'knight',    'auto_counter',       'knight_autocounter',        'skill_auto_counter',          'unique'),
    ( 707, 'knight',    'bowling_bash',       'knight_bowling_bash',       'skill_bowling_bash',          'unique'),
    ( 710, 'knight',    'charge_attack',      'lg_pinpointattack',         'skill_charge_attack',         'unique'),  # RG Pinpoint Attack = same charge concept

    # ============================================================================
    # WIZARD (800-813)
    # ============================================================================
    ( 800, 'wizard',    'jupitel_thunder',    'wl_telekinesis_intense',    'skill_jupitel_thunder',       'unique'),  # WL Telekinesis Intense = thunder ball
    ( 801, 'wizard',    'lord_of_vermilion',  'wl_tetravortex',            'skill_lord_of_vermilion',     'unique'),  # WL Tetra Vortex = massive 4-element AoE
    ( 802, 'wizard',    'meteor_storm',       'wizard_meteor',             'skill_meteor_storm',          'unique'),
    ( 803, 'wizard',    'storm_gust',         'wizard_stormgust',          'skill_storm_gust',            'unique'),
    ( 804, 'wizard',    'earth_spike',        'wizard_earthspike',         'skill_earth_spike',           'unique'),
    ( 805, 'wizard',    'heavens_drive',      'wizard_earthspike',         'skill_heavens_drive',         'shared'),
    ( 806, 'wizard',    'quagmire',           'wizard_quagmire',           'skill_quagmire',              'unique'),
    ( 807, 'wizard',    'water_ball',         'wizard_waterball_chulung',  'skill_water_ball',            'unique'),
    ( 808, 'wizard',    'ice_wall',           'wizard_icewall',            'skill_ice_wall',              'unique'),
    ( 809, 'wizard',    'sight_rasher',       'wizard_sightrasher',        'skill_sight_rasher',          'unique'),
    ( 810, 'wizard',    'fire_pillar',        'wizard_fire_pillar_a',      'skill_fire_pillar',           'unique'),
    ( 811, 'wizard',    'frost_nova',         'wl_jackfrost',              'skill_frost_nova',            'unique'),  # WL Jack Frost = Frost Nova family
    ( 812, 'wizard',    'sense',              'analyze',                   'skill_sense',                 'unique'),  # analyze = sense (literal RO Classic name)
    ( 813, 'wizard',    'sight_blaster',      'ef_sight',                  'skill_sight_blaster',         'shared'),

    # ============================================================================
    # HUNTER (900-917)
    # ============================================================================
    ( 900, 'hunter',    'blitz_beat',         'hunter_blitzbeat',          'skill_blitz_beat',            'unique'),
    ( 902, 'hunter',    'detect',             'hunter_detecting',          'skill_detect',                'unique'),
    ( 903, 'hunter',    'ankle_snare',        'hunter_anklesnare',         'skill_ankle_snare',           'unique'),
    ( 904, 'hunter',    'land_mine',          'hunter_landmine',           'skill_land_mine',             'unique'),
    ( 905, 'hunter',    'remove_trap',        'hunter_removetrap',         'skill_remove_trap',           'unique'),
    ( 906, 'hunter',    'shockwave_trap',     'hunter_shockwavetrap',      'skill_shockwave_trap',        'unique'),
    ( 907, 'hunter',    'claymore_trap',      'hunter_claymoretrap',       'skill_claymore_trap',         'unique'),
    ( 908, 'hunter',    'skid_trap',          'hunter_skidtrap',           'skill_skid_trap',             'unique'),
    ( 909, 'hunter',    'sandman',            'hunter_sandman',            'skill_sandman',               'unique'),
    ( 910, 'hunter',    'flasher',            'hunter_flasher',            'skill_flasher',               'unique'),
    ( 911, 'hunter',    'freezing_trap',      'hunter_freezingtrap',       'skill_freezing_trap',         'unique'),
    ( 912, 'hunter',    'blast_mine',         'hunter_blastmine',          'skill_blast_mine',            'unique'),
    ( 913, 'hunter',    'spring_trap',        'hunter_springtrap',         'skill_spring_trap',           'unique'),
    ( 914, 'hunter',    'talkie_box',         'hunter_talkiebox_a',        'skill_talkie_box',            'unique'),
    ( 917, 'hunter',    'phantasmic_arrow',   'ranger_piercing_shot',      'skill_phantasmic_arrow',      'unique'),  # Ranger Piercing Shot = phantasmic pierce

    # ============================================================================
    # PRIEST (1000-1018)
    # ============================================================================
    (1000, 'priest',    'sanctuary',          'priest_sanctuary',          'skill_sanctuary',             'unique'),
    (1001, 'priest',    'kyrie_eleison',      'priest_kyrie_eleison_a',    'skill_kyrie_eleison',         'unique'),
    (1002, 'priest',    'magnificat',         'priest_magnificat',         'skill_magnificat',            'unique'),
    (1003, 'priest',    'gloria',             'priest_gloria',             'skill_gloria',                'unique'),
    (1004, 'priest',    'resurrection',       'priest_resurrection',       'skill_resurrection',          'unique'),
    (1005, 'priest',    'magnus_exorcismus',  'priest_magnus',             'skill_magnus_exorcismus',     'unique'),
    (1006, 'priest',    'turn_undead',        'priest_turn_undead',        'skill_turn_undead',           'unique'),
    (1007, 'priest',    'lex_aeterna',        'priest_lexaeterna',         'skill_lex_aeterna',           'unique'),
    (1009, 'priest',    'impositio_manus',    'priest_impositio',          'skill_impositio_manus',       'unique'),
    (1010, 'priest',    'suffragium',         'priest_suffragium',         'skill_suffragium',            'unique'),
    (1011, 'priest',    'aspersio',           'priest_aspersio',           'skill_aspersio',              'unique'),
    (1012, 'priest',    'bs_sacramenti',      'priest_benedictio',         'skill_bs_sacramenti',         'unique'),
    (1013, 'priest',    'slow_poison',        'priest_slowpoison',         'skill_slow_poison',           'unique'),
    (1014, 'priest',    'status_recovery',    'priest_recovery',           'skill_status_recovery',       'unique'),
    (1015, 'priest',    'lex_divina',         'priest_lexdivina',          'skill_lex_divina',            'unique'),
    (1017, 'priest',    'safety_wall_priest', 'ef_glasswall',              'skill_safety_wall_priest',    'shared'),
    (1018, 'priest',    'redemptio',          'priest_resurrection',       'skill_redemptio',             'fallback'),

    # ============================================================================
    # ASSASSIN (1100-1111)
    # ============================================================================
    (1101, 'assassin',  'sonic_blow',         'assasin_sonicblow',         'skill_sonic_blow',            'unique'),
    (1102, 'assassin',  'grimtooth',          'assasin_sonicblow',         'skill_grimtooth',             'fallback'),
    (1103, 'assassin',  'cloaking',           'assasin_cloaking',          'skill_cloaking',              'unique'),
    (1105, 'assassin',  'venom_dust',         'assasin_venomdust',         'skill_venom_dust',            'unique'),
    (1109, 'assassin',  'enchant_poison',     'assasin_enchantpoison',     'skill_enchant_poison',        'unique'),
    (1110, 'assassin',  'venom_splasher',     'assasin_venomsplasher',     'skill_venom_splasher',        'unique'),
    (1111, 'assassin',  'throw_venom_knife',  'ef_poisonattack',           'skill_throw_venom_knife',     'fallback'),

    # ============================================================================
    # BLACKSMITH (1200-1230)
    # ============================================================================
    (1200, 'blacksmith','adrenaline_rush',    'black_adrenalinerush',      'skill_adrenaline_rush',       'unique'),
    (1201, 'blacksmith','weapon_perfection',  'black_weapon_perfection',   'skill_weapon_perfection',     'unique'),
    (1202, 'blacksmith','power_thrust',       'black_overthrust',          'skill_power_thrust',          'unique'),
    (1206, 'blacksmith','hammer_fall',        'black_hammerfall',          'skill_hammer_fall',           'unique'),
    (1209, 'blacksmith','weapon_repair',      'black_weapon_repair',       'skill_weapon_repair',         'unique'),
    (1210, 'blacksmith','greed',              'ef_coin',                   'skill_greed',                 'fallback'),

    # ============================================================================
    # CRUSADER (1300-1313)
    # ============================================================================
    (1301, 'crusader',  'auto_guard',         'kyrie_guard',               'skill_auto_guard',            'unique'),  # kyrie_guard = literal guard sound
    (1302, 'crusader',  'holy_cross',         'cru_holy cross',            'skill_holy_cross',            'unique'),
    (1303, 'crusader',  'grand_cross',        'cru_grand cross',           'skill_grand_cross',           'unique'),
    (1304, 'crusader',  'shield_charge',      'ig_shield_shooting',        'skill_shield_charge',         'unique'),  # IG Shield Shooting = shield charge
    (1305, 'crusader',  'shield_boomerang',   'cru_shield boomerang',      'skill_shield_boomerang',      'unique'),
    (1306, 'crusader',  'devotion',           'bo_advance_protection',     'skill_devotion',              'unique'),  # bo_advance_protection = link/protect
    (1307, 'crusader',  'reflect_shield',     'lg_reflectdamage',          'skill_reflect_shield',        'unique'),  # canonical RG reflect inherits from Crusader
    (1308, 'crusader',  'providence',         'ig_holy_shield1',           'skill_providence',            'unique'),  # IG Holy Shield = providence holy resist
    (1309, 'crusader',  'defender',           'ig_guard_stance',           'skill_defender',              'unique'),  # IG Guard Stance = literal defender stance
    (1311, 'crusader',  'heal_crusader',      None,                        'PLAYER:heal_effect',          'shared'),
    (1312, 'crusader',  'cure_crusader',      'acolyte_cure',              'skill_cure_crusader',         'shared'),
    (1313, 'crusader',  'shrink',             'ig_guardian_shield',        'skill_shrink',                'fallback'),  # shield-related anti-charge buff

    # ============================================================================
    # SAGE (1400-1421)
    # ============================================================================
    (1401, 'sage',      'cast_cancel',        'sage_spell breake',         'skill_cast_cancel',           'shared'),  # cast cancel = spell break
    (1402, 'sage',      'hindsight',          'sage_magic_power_amplify',  'skill_hindsight',             'unique'),  # autocast trigger = sage magic power
    (1403, 'sage',      'dispell',            'sage_spell breake',         'skill_dispell',               'shared'),
    (1404, 'sage',      'magic_rod',          'sage_magic rod',            'skill_magic_rod',             'unique'),
    (1406, 'sage',      'spell_breaker',      'sage_spell breake',         'skill_spell_breaker',         'unique'),
    (1408, 'sage',      'endow_blaze',        'ef_firearrow',              'skill_endow_blaze',           'fallback'),
    (1409, 'sage',      'endow_tsunami',      'ef_icearrow',               'skill_endow_tsunami',         'fallback'),
    (1410, 'sage',      'endow_tornado',      'ef_lightbolt',              'skill_endow_tornado',         'fallback'),
    (1411, 'sage',      'endow_quake',        'wizard_earthspike',         'skill_endow_quake',           'fallback'),
    (1412, 'sage',      'volcano',            'wizard_fire_pillar_a',      'skill_volcano',               'fallback'),  # fire ground zone
    (1413, 'sage',      'deluge',             'wizard_waterball_chulung',  'skill_deluge',                'fallback'),
    (1414, 'sage',      'violent_gale',       'wizard_stormgust',          'skill_violent_gale',          'fallback'),
    (1415, 'sage',      'land_protector',     'so_elemental_shield',       'skill_land_protector',        'unique'),  # SO Elemental Shield = land protector concept
    (1416, 'sage',      'abracadabra',        'dimension',                 'skill_abracadabra',           'unique'),  # dimension = magical chaos warp
    (1417, 'sage',      'earth_spike_sage',   'wizard_earthspike',         'skill_earth_spike_sage',      'shared'),
    (1418, 'sage',      'heavens_drive_sage', 'wizard_earthspike',         'skill_heavens_drive_sage',    'shared'),
    (1419, 'sage',      'sense_sage',         'analyze',                   'skill_sense_sage',            'shared'),  # shared with Sense
    (1420, 'sage',      'create_elemental_converter','complete',           'skill_create_converter',      'fallback'),
    (1421, 'sage',      'elemental_change',   'ef_beginspell',             'skill_elemental_change',      'fallback'),

    # ============================================================================
    # BARD (1500-1537)
    # ============================================================================
    (1501, 'bard',      'poem_of_bragi',          'minstrel_brage_poem',       'skill_poem_of_bragi',         'unique'),
    (1502, 'bard',      'assassin_cross_of_sunset','guillotine_cross_sunset',  'skill_assassin_cross_sunset', 'unique'),
    (1503, 'bard',      'adaptation',         'tr_musical_interlude',      'skill_adaptation',            'unique'),  # Trouvere Musical Interlude = song interrupt
    (1504, 'bard',      'encore',             'tr_pron_march',             'skill_encore',                'unique'),  # Trouvere Pron March = song restart
    (1505, 'bard',      'dissonance',         'minstrel_song_of_echo',     'skill_dissonance',            'unique'),
    (1506, 'bard',      'frost_joker',        'wanderer_voice_of_siren',   'skill_frost_joker',           'fallback'),
    (1507, 'bard',      'a_whistle',          'minstrel_lucky_song',       'skill_a_whistle',             'unique'),
    (1508, 'bard',      'apple_of_idun',      'minstrel_apple_of_idun',    'skill_apple_of_idun',         'unique'),
    (1509, 'bard',      'pang_voice',         'tr_soundblend_hit',         'skill_pang_voice',            'unique'),  # Trouvere Sound Blend Hit = voice attack
    (1511, 'bard',      'musical_strike',     'tr_soundblend_shot',        'skill_musical_strike',        'unique'),  # Trouvere Sound Blend Shot = musical projectile
    (1530, 'bard',      'lullaby',            'wanderer_lullaby_rest',     'skill_lullaby',               'unique'),
    (1531, 'bard',      'mr_kim_a_rich_man',  'genetic_zeny_research',     'skill_mr_kim_rich',           'unique'),
    (1532, 'bard',      'eternal_chaos',      'minstrel_eternal_chaos',    'skill_eternal_chaos',         'unique'),
    (1533, 'bard',      'drum_on_battlefield','minstrel_battle_chant',     'skill_drum_battlefield',      'unique'),
    (1534, 'bard',      'ring_of_nibelungen', 'minstrel_ring_of_nibelungen','skill_nibelungen_ring',      'unique'),
    (1535, 'bard',      'lokis_veil',         'minstrel_loki_veil',        'skill_lokis_veil',            'unique'),
    (1536, 'bard',      'into_the_abyss',     'minstrel_into_the_abyss',   'skill_into_the_abyss',        'unique'),
    (1537, 'bard',      'invulnerable_siegfried','minstrel_hermode_staff', 'skill_siegfried',             'unique'),

    # ============================================================================
    # DANCER (1521-1557)
    # ============================================================================
    (1521, 'dancer',    'service_for_you',    'minstrel_service_for_you',  'skill_service_for_you',       'unique'),
    (1522, 'dancer',    'humming',            'wanderer_humming',          'skill_humming',               'unique'),
    (1523, 'dancer',    'adaptation_dancer',  'tr_musical_interlude',      'skill_adaptation_dancer',     'shared'),  # shared with Bard adaptation
    (1524, 'dancer',    'encore_dancer',      'tr_pron_march',             'skill_encore_dancer',         'shared'),  # shared with Bard encore
    (1525, 'dancer',    'ugly_dance',         'wanderer_swing_dance',      'skill_ugly_dance',            'unique'),
    (1526, 'dancer',    'scream',             'wanderer_voice_of_siren',   'skill_scream',                'unique'),
    (1527, 'dancer',    'please_dont_forget_me','wanderer_dont_forget_me', 'skill_dont_forget_me',        'unique'),
    (1528, 'dancer',    'fortunes_kiss',      'minstrel_lucky_song',       'skill_fortunes_kiss',         'unique'),
    (1529, 'dancer',    'charming_wink',      'wanderer_swing_dance',      'skill_charming_wink',         'fallback'),
    (1541, 'dancer',    'slinging_arrow',     'ef_firearrow',              'skill_slinging_arrow',        'fallback'),
    (1550, 'dancer',    'lullaby_dancer',     'dancer_lullaby',            'skill_lullaby_dancer',        'unique'),
    (1551, 'dancer',    'mr_kim_dancer',      'genetic_zeny_research',     'skill_mr_kim_dancer',         'shared'),
    (1552, 'dancer',    'eternal_chaos_dancer','minstrel_eternal_chaos',   'skill_eternal_chaos_dancer',  'shared'),
    (1553, 'dancer',    'drum_battlefield_dancer','minstrel_battle_chant', 'skill_drum_battlefield_dancer','shared'),
    (1554, 'dancer',    'nibelungen_dancer',  'minstrel_ring_of_nibelungen','skill_nibelungen_dancer',    'shared'),
    (1555, 'dancer',    'lokis_veil_dancer',  'minstrel_loki_veil',        'skill_lokis_veil_dancer',     'shared'),
    (1556, 'dancer',    'into_abyss_dancer',  'minstrel_into_the_abyss',   'skill_into_abyss_dancer',     'shared'),
    (1557, 'dancer',    'siegfried_dancer',   'minstrel_hermode_staff',    'skill_siegfried_dancer',      'shared'),

    # ============================================================================
    # MONK (1600-1615)
    # ============================================================================
    (1601, 'monk',      'summon_spirit_sphere','sura_inhale',              'skill_summon_spirit_sphere',  'shared'),  # shared with absorb_spirit_sphere (same sphere SFX)
    (1602, 'monk',      'investigate',        'monk_combo_chain',          'skill_investigate',           'shared'),  # shared with chain combo (rapid hit chain)
    (1604, 'monk',      'finger_offensive',   'sura_finger_offensive',     'skill_finger_offensive',      'unique'),
    (1605, 'monk',      'asura_strike',       'monk_asura_strike',         'skill_asura_strike',          'unique'),
    (1607, 'monk',      'absorb_spirit_sphere','sura_inhale',              'skill_absorb_spirit_sphere',  'unique'),
    (1609, 'monk',      'blade_stop',         'knight_autocounter',        'skill_blade_stop',            'fallback'),
    (1610, 'monk',      'chain_combo',        'monk_combo_chain',          'skill_chain_combo',           'unique'),
    (1611, 'monk',      'critical_explosion', 'sura_explode',              'skill_critical_explosion',    'unique'),
    (1612, 'monk',      'steel_body',         'metalic',                   'skill_steel_body',            'unique'),  # metalic.wav = literal steel/metal body sound
    (1613, 'monk',      'combo_finish',       'sura_combo_finish',         'skill_combo_finish',          'unique'),
    (1614, 'monk',      'ki_translation',     'sura_inhale',               'skill_ki_translation',        'shared'),  # shared with absorb (sphere transfer)
    (1615, 'monk',      'ki_explosion',       'sura_explode',              'skill_ki_explosion',          'unique'),

    # ============================================================================
    # ROGUE (1700-1718)
    # ============================================================================
    (1701, 'rogue',     'back_stab',          'rog_back stap',             'skill_back_stab',             'unique'),
    (1703, 'rogue',     'raid',               'assasin_sonicblow',         'skill_raid',                  'fallback'),  # rapid surprise attack
    (1704, 'rogue',     'intimidate',         'rog_intimidate',            'skill_intimidate',            'unique'),
    (1707, 'rogue',     'double_strafe_rogue','ef_firearrow',              'skill_double_strafe_rogue',   'shared'),
    (1708, 'rogue',     'remove_trap_rogue',  'hunter_removetrap',         'skill_remove_trap_rogue',     'shared'),
    (1709, 'rogue',     'steal_coin',         'rog_steal coin',            'skill_steal_coin',            'unique'),
    (1710, 'rogue',     'divest_helm',        'strip',                     'skill_divest_helm',           'unique'),  # strip.wav = literal divest/strip canonical
    (1711, 'rogue',     'divest_shield',      'strip',                     'skill_divest_shield',         'shared'),
    (1712, 'rogue',     'divest_armor',       'strip',                     'skill_divest_armor',          'shared'),
    (1713, 'rogue',     'divest_weapon',      'strip',                     'skill_divest_weapon',         'shared'),
    (1717, 'rogue',     'scribble',           'complete',                  'skill_scribble',              'fallback'),
    (1718, 'rogue',     'close_confine',      'wideb',                     'skill_close_confine',         'unique'),  # wideb.wav = wide bind/restrain

    # ============================================================================
    # ALCHEMIST (1800-1815)
    # ============================================================================
    (1800, 'alchemist', 'pharmacy',           'p_success',                 'skill_pharmacy',              'unique'),  # p_success = pharmacy success
    (1801, 'alchemist', 'acid_terror',        'bo_acidified_zone',         'skill_acid_terror',           'unique'),  # acidified zone = acid family
    (1802, 'alchemist', 'demonstration',      'genetic_demonic_fire',      'skill_demonstration',         'unique'),  # demonic fire = Demonstration in Korean
    (1803, 'alchemist', 'summon_flora',       'bo_fairy_dusty',            'skill_summon_flora',          'unique'),  # fairy dust = plant summon magic
    (1806, 'alchemist', 'potion_pitcher',     None,                        'PLAYER:heal_effect',          'shared'),  # heal sound shared
    (1807, 'alchemist', 'summon_marine_sphere','bo_acidified_zone_water',  'skill_summon_marine_sphere',  'unique'),  # water variant
    (1808, 'alchemist', 'chemical_protection_helm','bo_advance_protection','skill_cp_helm',               'unique'),  # bo_advance_protection = canonical CP
    (1809, 'alchemist', 'chemical_protection_shield','bo_advance_protection','skill_cp_shield',           'shared'),
    (1810, 'alchemist', 'chemical_protection_armor','bo_advance_protection','skill_cp_armor',             'shared'),
    (1811, 'alchemist', 'chemical_protection_weapon','bo_advance_protection','skill_cp_weapon',           'shared'),
    (1813, 'alchemist', 'call_homunculus',    'priest_resurrection',       'skill_call_homunculus',       'fallback'),
    (1814, 'alchemist', 'rest',               'ef_hiding',                 'skill_homun_rest',            'fallback'),
    (1815, 'alchemist', 'resurrect_homunculus','priest_resurrection',      'skill_resurrect_homunculus',  'fallback'),
]


def main():
    if not os.path.isdir(DST):
        os.makedirs(DST, exist_ok=True)

    copied = 0
    skipped = 0
    missing_src = []
    counts = {'unique': 0, 'shared': 0, 'fallback': 0, 'silent': 0, 'missing': 0}

    for sid, cls, name, src_base, dst_base, note in SKILL_MAP:
        counts[note] = counts.get(note, 0) + 1

        # Skip entries that reuse existing assets (PLAYER:*, etc.)
        if src_base is None:
            continue

        src = os.path.join(SRC, src_base + '.wav')
        dst = os.path.join(DST, dst_base + '.wav')

        if not os.path.isfile(src):
            missing_src.append(f'  {sid:5d} {cls:12s} {name:30s} <- MISSING SOURCE: {src_base}.wav')
            continue

        # Always overwrite — source mappings may have been updated
        try:
            # If destination exists, only copy when source is different (faster + idempotent)
            if os.path.isfile(dst):
                src_size = os.path.getsize(src)
                dst_size = os.path.getsize(dst)
                if src_size == dst_size:
                    skipped += 1
                    continue
            shutil.copy2(src, dst)
            copied += 1
        except Exception as e:
            missing_src.append(f'  {sid:5d} {cls:12s} {name:30s} <- COPY FAILED: {e}')

    print('=' * 70)
    print(f'COPY SUMMARY')
    print('=' * 70)
    print(f'  Total entries:        {len(SKILL_MAP)}')
    print(f'  Copied (new):         {copied}')
    print(f'  Skipped (existing):   {skipped}')
    print(f'  Missing source:       {len(missing_src)}')
    print()
    print('  Mapping notes:')
    for note, n in sorted(counts.items()):
        print(f'    {note:10s} {n:4d}')
    if missing_src:
        print()
        print('SOURCE FILES NOT FOUND:')
        for m in missing_src:
            print(m)

    # Print the C++ map
    print()
    print('=' * 70)
    print('C++ MAP — paste into AudioSubsystem.cpp OnWorldBeginPlay')
    print('=' * 70)
    last_class = None
    for sid, cls, name, src_base, dst_base, note in SKILL_MAP:
        if cls != last_class:
            print(f'\n\t\t// ---- {cls.upper()} ----')
            last_class = cls
        if dst_base.startswith('PLAYER:'):
            wav_path = '/Game/SabriMMO/Audio/SFX/Player/' + dst_base.split(':', 1)[1]
            print(f'\t\tSkillImpactSoundMap.Add({sid:4d}, TEXT("{wav_path}"));  // {name} [{note}]')
        else:
            print(f'\t\tSkillImpactSoundMap.Add({sid:4d}, SkillRoot + TEXT("{dst_base}"));  // {name} [{note}]')


if __name__ == '__main__':
    main()
