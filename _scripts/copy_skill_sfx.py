"""
copy_skill_sfx.py
Bulk-copies wav files from ROSFX/effect/ into Content/SabriMMO/Audio/SFX/Skills/
with consistent skill_<name>.wav naming, and prints out the C++ map entries
to paste into AudioSubsystem.cpp.

Each entry is (skillId, source_wav_basename, dest_basename).
- source_wav_basename: file in C:/Sabri_MMO/ROSFX/effect/  (no .wav extension)
- dest_basename: file in Content/SabriMMO/Audio/SFX/Skills/ (no .wav extension)

If source != dest, the file is copied (and renamed).
If both are the same and the file already exists in dest, no copy is needed.

Run-once. Idempotent — already-copied files are skipped.
"""
import os, shutil, sys, io

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

SRC = r'C:/Sabri_MMO/ROSFX/effect'
DST = r'C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Audio/SFX/Skills'

# (skill_id, class_name, skill_name, source_wav_no_ext, dest_wav_no_ext)
# dest_wav_no_ext == None means use the same as source
# Order: alphabetical by skill_id within each class group, classes ordered by tier
SKILL_MAP = [
    # ==== NOVICE (1-3) ====
    # 2 first_aid — no obvious wav, skip (already silent)

    # ==== SWORDSMAN (100-109) — already done, listed for completeness ====
    (103, 'swordsman', 'bash',          'ef_bash',          'skill_bash'),
    (104, 'swordsman', 'provoke',       'swordman_provoke', 'skill_provoke'),
    (105, 'swordsman', 'magnum_break',  'ef_magnumbreak',   'skill_magnum_break'),
    (106, 'swordsman', 'endure',        'ef_endure',        'skill_endure'),

    # ==== MAGE (200-213) — already done ====
    (200, 'mage', 'cold_bolt',      'ef_icearrow',     'skill_cold_bolt'),
    (201, 'mage', 'fire_bolt',      'ef_firearrow',    'skill_fire_bolt'),
    (202, 'mage', 'lightning_bolt', 'ef_lightbolt',    'skill_lightning_bolt'),
    (203, 'mage', 'napalm_beat',    'ef_napalmbeat',   'skill_napalm_beat'),
    (205, 'mage', 'sight',          'ef_sight',        'skill_sight'),
    (206, 'mage', 'stone_curse',    'ef_stonecurse',   'skill_stone_curse'),
    (207, 'mage', 'fire_ball',      'ef_fireball',     'skill_fire_ball'),
    (208, 'mage', 'frost_diver',    'ef_frostdiver',   'skill_frost_diver'),
    (209, 'mage', 'fire_wall',      'ef_firewall',     'skill_fire_wall'),
    (210, 'mage', 'soul_strike',    'ef_soulstrike',   'skill_soul_strike'),
    (211, 'mage', 'safety_wall',    'ef_glasswall',    'skill_safety_wall'),
    (212, 'mage', 'thunderstorm',   'ef_thunderstorm', 'skill_thunderstorm'),
    # 213 energy_coat — no clear wav, skip

    # ==== ARCHER (300-306) ====
    (303, 'archer', 'double_strafe', 'ef_firearrow',  'skill_double_strafe'),  # arrow swoosh — using firearrow as bow
    # Actually use a non-element neutral arrow sound: hit_arrow exists in Weapons folder
    # But for skill, the cast sound is what matters; impact uses ef_firearrow
    (304, 'archer', 'arrow_shower', 'ef_firearrow',  'skill_arrow_shower'),
    (306, 'archer', 'arrow_repel',  'ef_firearrow',  'skill_arrow_repel'),

    # ==== ACOLYTE (400-414) ====
    # 400 heal — already mapped to Player/heal_effect.wav (no copy needed; handled in C++ directly)
    (402, 'acolyte', 'blessing',      'ef_blessing',     'skill_blessing'),
    (403, 'acolyte', 'increase_agi',  'ef_incagility',   'skill_increase_agi'),
    (404, 'acolyte', 'decrease_agi',  'ef_decagility',   'skill_decrease_agi'),
    (405, 'acolyte', 'cure',          'acolyte_cure',    'skill_cure'),
    (406, 'acolyte', 'angelus',       'ef_angelus',      'skill_angelus'),
    (407, 'acolyte', 'signum_crucis', 'ef_signum',       'skill_signum_crucis'),
    (408, 'acolyte', 'ruwach',        'ef_ruwach',       'skill_ruwach'),
    (409, 'acolyte', 'teleport',      'ef_teleportation','skill_teleport'),
    (410, 'acolyte', 'warp_portal',   'ef_portal',       'skill_warp_portal'),
    (412, 'acolyte', 'aqua_benedicta','ef_aqua',         'skill_aqua_benedicta'),
    (414, 'acolyte', 'holy_light',    'acolyte_hunew',   'skill_holy_light'),

    # ==== THIEF (500-509) ====
    (502, 'thief', 'steal',         'ef_steal',         'skill_steal'),
    (503, 'thief', 'hiding',        'ef_hiding',        'skill_hiding'),
    (504, 'thief', 'envenom',       'ef_poisonattack',  'skill_envenom'),
    (505, 'thief', 'detoxify',      'ef_detoxication',  'skill_detoxify'),

    # ==== MERCHANT (600-609) ====
    (603, 'merchant', 'mammonite',       'merch_maemor',  'skill_mammonite'),
    (608, 'merchant', 'cart_revolution', 'ef_bash',       'skill_cart_revolution'),  # heavy thud — reuse bash
    (609, 'merchant', 'loud_exclamation','ef_endure',     'skill_loud_exclamation'),  # battle shout — reuse endure

    # ==== KNIGHT (700-710) ====
    (703, 'knight', 'brandish_spear',  'knight_brandish_spear', 'skill_brandish_spear'),
    (704, 'knight', 'spear_boomerang', 'knight_spear_boomerang','skill_spear_boomerang'),
    (705, 'knight', 'two_hand_quicken','knight_twohandquicken', 'skill_two_hand_quicken'),
    (706, 'knight', 'auto_counter',    'knight_autocounter',    'skill_auto_counter'),
    (707, 'knight', 'bowling_bash',    'knight_bowling_bash',   'skill_bowling_bash'),

    # ==== WIZARD (800-813) ====
    (801, 'wizard', 'lord_of_vermilion','ef_thunderstorm',          'skill_lord_of_vermilion'),  # thunder cascade
    (802, 'wizard', 'meteor_storm',     'wizard_meteor',            'skill_meteor_storm'),
    (803, 'wizard', 'storm_gust',       'wizard_stormgust',         'skill_storm_gust'),
    (804, 'wizard', 'earth_spike',      'wizard_earthspike',        'skill_earth_spike'),
    (805, 'wizard', 'heavens_drive',    'wizard_earthspike',        'skill_heavens_drive'),  # earth-based AoE, reuse spike
    (806, 'wizard', 'quagmire',         'wizard_quagmire',          'skill_quagmire'),
    (807, 'wizard', 'water_ball',       'wizard_waterball_chulung', 'skill_water_ball'),
    (808, 'wizard', 'ice_wall',         'wizard_icewall',           'skill_ice_wall'),
    (809, 'wizard', 'sight_rasher',     'wizard_sightrasher',       'skill_sight_rasher'),
    (810, 'wizard', 'fire_pillar',      'wizard_fire_pillar_a',     'skill_fire_pillar'),
    (812, 'wizard', 'sense',            'ef_beginspell',            'skill_sense'),  # info-gather chime

    # ==== HUNTER (900-917) ====
    (900, 'hunter', 'blitz_beat',     'hunter_blitzbeat',     'skill_blitz_beat'),
    (902, 'hunter', 'detect',         'hunter_detecting',     'skill_detect'),
    (903, 'hunter', 'ankle_snare',    'hunter_anklesnare',    'skill_ankle_snare'),
    (904, 'hunter', 'land_mine',      'hunter_landmine',      'skill_land_mine'),
    (905, 'hunter', 'remove_trap',    'hunter_removetrap',    'skill_remove_trap'),
    (906, 'hunter', 'shockwave_trap', 'hunter_shockwavetrap', 'skill_shockwave_trap'),
    (907, 'hunter', 'claymore_trap',  'hunter_claymoretrap',  'skill_claymore_trap'),
    (908, 'hunter', 'skid_trap',      'hunter_skidtrap',      'skill_skid_trap'),
    (909, 'hunter', 'sandman',        'hunter_sandman',       'skill_sandman'),
    (910, 'hunter', 'flasher',        'hunter_flasher',       'skill_flasher'),
    (911, 'hunter', 'freezing_trap',  'hunter_freezingtrap',  'skill_freezing_trap'),
    (912, 'hunter', 'blast_mine',     'hunter_blastmine',     'skill_blast_mine'),
    (913, 'hunter', 'spring_trap',    'hunter_springtrap',    'skill_spring_trap'),
    (914, 'hunter', 'talkie_box',     'hunter_talkiebox_a',   'skill_talkie_box'),
    (917, 'hunter', 'phantasmic_arrow','ef_firearrow',        'skill_phantasmic_arrow'),

    # ==== PRIEST (1000-1018) ====
    (1000, 'priest', 'sanctuary',       'priest_sanctuary',       'skill_sanctuary'),
    (1001, 'priest', 'kyrie_eleison',   'priest_kyrie_eleison_a', 'skill_kyrie_eleison'),
    (1002, 'priest', 'magnificat',      'priest_magnificat',      'skill_magnificat'),
    (1003, 'priest', 'gloria',          'priest_gloria',          'skill_gloria'),
    (1004, 'priest', 'resurrection',    'priest_resurrection',    'skill_resurrection'),
    (1005, 'priest', 'magnus_exorcismus','priest_magnus',         'skill_magnus_exorcismus'),
    (1006, 'priest', 'turn_undead',     'priest_turn_undead',     'skill_turn_undead'),
    (1007, 'priest', 'lex_aeterna',     'priest_lexaeterna',      'skill_lex_aeterna'),
    (1009, 'priest', 'impositio_manus', 'priest_impositio',       'skill_impositio_manus'),
    (1010, 'priest', 'suffragium',      'priest_suffragium',      'skill_suffragium'),
    (1011, 'priest', 'aspersio',        'priest_aspersio',        'skill_aspersio'),
    (1012, 'priest', 'bs_sacramenti',   'priest_benedictio',      'skill_bs_sacramenti'),
    (1013, 'priest', 'slow_poison',     'priest_slowpoison',      'skill_slow_poison'),
    (1014, 'priest', 'status_recovery', 'priest_recovery',        'skill_status_recovery'),
    (1015, 'priest', 'lex_divina',      'priest_lexdivina',       'skill_lex_divina'),
    (1017, 'priest', 'safety_wall_priest','ef_glasswall',         'skill_safety_wall_priest'),

    # ==== ASSASSIN (1100-1111) ====
    (1101, 'assassin', 'sonic_blow',     'assasin_sonicblow',     'skill_sonic_blow'),
    (1102, 'assassin', 'grimtooth',      'assasin_sonicblow',     'skill_grimtooth'),  # rapid stab — reuse
    (1103, 'assassin', 'cloaking',       'assasin_cloaking',      'skill_cloaking'),
    (1105, 'assassin', 'venom_dust',     'assasin_venomdust',     'skill_venom_dust'),
    (1109, 'assassin', 'enchant_poison', 'assasin_enchantpoison', 'skill_enchant_poison'),
    (1110, 'assassin', 'venom_splasher', 'assasin_venomsplasher', 'skill_venom_splasher'),

    # ==== BLACKSMITH (1200-1230) ====
    (1200, 'blacksmith', 'adrenaline_rush',  'black_adrenalinerush',   'skill_adrenaline_rush'),
    (1201, 'blacksmith', 'weapon_perfection','black_weapon_perfection','skill_weapon_perfection'),
    (1202, 'blacksmith', 'power_thrust',     'black_overthrust',       'skill_power_thrust'),
    (1206, 'blacksmith', 'hammer_fall',      'black_hammerfall',       'skill_hammer_fall'),
    (1209, 'blacksmith', 'weapon_repair',    'black_weapon_repair',    'skill_weapon_repair'),

    # ==== CRUSADER (1300-1313) ====
    (1301, 'crusader', 'auto_guard',      'ef_glasswall',         'skill_auto_guard'),
    (1302, 'crusader', 'holy_cross',      'cru_holy cross',       'skill_holy_cross'),  # SPACE in source
    (1303, 'crusader', 'grand_cross',     'cru_grand cross',      'skill_grand_cross'),  # SPACE in source
    (1305, 'crusader', 'shield_boomerang','cru_shield boomerang', 'skill_shield_boomerang'),  # SPACE in source

    # ==== SAGE (1400-1421) ====
    (1404, 'sage', 'magic_rod',     'sage_magic rod',     'skill_magic_rod'),  # SPACE
    (1406, 'sage', 'spell_breaker', 'sage_spell breake',  'skill_spell_breaker'),  # SPACE + typo
    (1408, 'sage', 'endow_blaze',   'ef_firearrow',       'skill_endow_blaze'),
    (1409, 'sage', 'endow_tsunami', 'ef_icearrow',        'skill_endow_tsunami'),
    (1410, 'sage', 'endow_tornado', 'ef_lightbolt',       'skill_endow_tornado'),
    (1411, 'sage', 'endow_quake',   'wizard_earthspike',  'skill_endow_quake'),
    (1417, 'sage', 'earth_spike_sage',  'wizard_earthspike', 'skill_earth_spike_sage'),
    (1418, 'sage', 'heavens_drive_sage','wizard_earthspike', 'skill_heavens_drive_sage'),

    # ==== BARD (1500-1537) ====
    (1501, 'bard', 'poem_of_bragi',          'minstrel_brage_poem',         'skill_poem_of_bragi'),
    (1502, 'bard', 'assassin_cross_of_sunset','guillotine_cross_sunset',    'skill_assassin_cross_sunset'),
    (1505, 'bard', 'dissonance',             'minstrel_song_of_echo',       'skill_dissonance'),
    (1508, 'bard', 'apple_of_idun',          'minstrel_apple_of_idun',      'skill_apple_of_idun'),
    (1511, 'bard', 'musical_strike',         'minstrel_brage_poem',         'skill_musical_strike'),  # reuse
    (1530, 'bard', 'lullaby',                'wanderer_lullaby_rest',       'skill_lullaby'),
    (1531, 'bard', 'mr_kim_a_rich_man',      'genetic_zeny_research',       'skill_mr_kim_rich'),
    (1532, 'bard', 'eternal_chaos',          'minstrel_eternal_chaos',      'skill_eternal_chaos'),
    (1533, 'bard', 'drum_on_battlefield',    'minstrel_battle_chant',       'skill_drum_battlefield'),
    (1534, 'bard', 'ring_of_nibelungen',     'minstrel_ring_of_nibelungen', 'skill_nibelungen_ring'),
    (1535, 'bard', 'lokis_veil',             'minstrel_loki_veil',          'skill_lokis_veil'),
    (1536, 'bard', 'into_the_abyss',         'minstrel_into_the_abyss',     'skill_into_the_abyss'),
    (1537, 'bard', 'invulnerable_siegfried', 'minstrel_hermode_staff',      'skill_siegfried'),

    # ==== DANCER (1521-1557) ====
    (1521, 'dancer', 'service_for_you',    'minstrel_service_for_you',  'skill_service_for_you'),
    (1522, 'dancer', 'humming',            'wanderer_humming',          'skill_humming'),
    (1525, 'dancer', 'ugly_dance',         'wanderer_swing_dance',      'skill_ugly_dance'),
    (1526, 'dancer', 'scream',             'wanderer_voice_of_siren',   'skill_scream'),
    (1527, 'dancer', 'please_dont_forget_me','wanderer_dont_forget_me', 'skill_dont_forget_me'),
    (1528, 'dancer', 'fortunes_kiss',      'minstrel_lucky_song',       'skill_fortunes_kiss'),
    (1529, 'dancer', 'charming_wink',      'wanderer_swing_dance',      'skill_charming_wink'),  # reuse
    (1541, 'dancer', 'slinging_arrow',     'ef_firearrow',              'skill_slinging_arrow'),
    (1550, 'dancer', 'lullaby_dancer',     'dancer_lullaby',            'skill_lullaby_dancer'),
    (1552, 'dancer', 'eternal_chaos_dancer','minstrel_eternal_chaos',   'skill_eternal_chaos_dancer'),
    (1556, 'dancer', 'into_abyss_dancer',  'minstrel_into_the_abyss',   'skill_into_abyss_dancer'),

    # ==== MONK (1600-1615) ====
    (1602, 'monk', 'investigate',         'monk_combo_chain',       'skill_investigate'),  # rapid hit chain
    (1604, 'monk', 'finger_offensive',    'sura_finger_offensive',  'skill_finger_offensive'),
    (1605, 'monk', 'asura_strike',        'monk_asura_strike',      'skill_asura_strike'),
    (1607, 'monk', 'absorb_spirit_sphere','sura_inhale',            'skill_absorb_spirit_sphere'),
    (1610, 'monk', 'chain_combo',         'monk_combo_chain',       'skill_chain_combo'),
    (1611, 'monk', 'critical_explosion',  'sura_explode',           'skill_critical_explosion'),
    (1613, 'monk', 'combo_finish',        'sura_combo_finish',      'skill_combo_finish'),
    (1615, 'monk', 'ki_explosion',        'sura_explode',           'skill_ki_explosion'),

    # ==== ROGUE (1700-1718) ====
    (1701, 'rogue', 'back_stab',         'rog_back stap',  'skill_back_stab'),  # SPACE + typo
    (1704, 'rogue', 'intimidate',        'rog_intimidate', 'skill_intimidate'),
    (1709, 'rogue', 'steal_coin',        'rog_steal coin', 'skill_steal_coin'),  # SPACE
    (1710, 'rogue', 'divest_helm',       'ef_steal',       'skill_divest_helm'),
    (1711, 'rogue', 'divest_shield',     'ef_steal',       'skill_divest_shield'),
    (1712, 'rogue', 'divest_armor',      'ef_steal',       'skill_divest_armor'),
    (1713, 'rogue', 'divest_weapon',     'ef_steal',       'skill_divest_weapon'),

    # ==== ALCHEMIST (1800-1815) ====
    # Alchemist wavs are limited — most are renewal Genetic. Use generic substitutes.
    (1801, 'alchemist', 'acid_terror',     'ef_poisonattack', 'skill_acid_terror'),
    (1802, 'alchemist', 'demonstration',   'ef_fireball',     'skill_demonstration'),
]


def main():
    if not os.path.isdir(DST):
        os.makedirs(DST, exist_ok=True)

    copied = 0
    skipped = 0
    missing = []

    for sid, cls, name, src_base, dst_base in SKILL_MAP:
        src = os.path.join(SRC, src_base + '.wav')
        dst = os.path.join(DST, dst_base + '.wav')

        if not os.path.isfile(src):
            missing.append(f'  {sid:5d} {cls:12s} {name:25s} <- MISSING SOURCE: {src_base}.wav')
            continue

        if os.path.isfile(dst):
            skipped += 1
            continue

        try:
            shutil.copy2(src, dst)
            copied += 1
        except Exception as e:
            missing.append(f'  {sid:5d} {cls:12s} {name:25s} <- COPY FAILED: {e}')

    print('=' * 70)
    print(f'COPY SUMMARY')
    print('=' * 70)
    print(f'  Total entries:  {len(SKILL_MAP)}')
    print(f'  Copied (new):   {copied}')
    print(f'  Skipped (existing): {skipped}')
    print(f'  Missing/failed: {len(missing)}')
    if missing:
        print()
        print('MISSING / FAILED:')
        for m in missing:
            print(m)
    print()
    print('=' * 70)
    print('C++ MAP ENTRIES (paste into AudioSubsystem.cpp OnWorldBeginPlay)')
    print('=' * 70)
    last_class = None
    for sid, cls, name, src_base, dst_base in SKILL_MAP:
        # Verify dst exists before printing
        if not os.path.isfile(os.path.join(DST, dst_base + '.wav')):
            continue
        if cls != last_class:
            print(f'\n\t\t// ---- {cls.upper()} ----')
            last_class = cls
        print(f'\t\tSkillImpactSoundMap.Add({sid:4d}, SkillRoot + TEXT("{dst_base}"));  // {name}')


if __name__ == '__main__':
    main()
