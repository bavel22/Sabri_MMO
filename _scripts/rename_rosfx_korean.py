"""
rename_rosfx_korean.py
Renames mojibake-encoded Korean .wav files in C:/Sabri_MMO/ROSFX/effect/
to readable English RO skill names.

The original RO/jRO data ships these effect wavs with Korean filenames; when
the GRF was extracted on a Western system they got mis-decoded latin1<->cp949,
so file names look like 's¹Ù·¹Æ¼¸£½ºÇÇ¾î.wav'. This script:
  1. Re-decodes each name back to its original Korean form
  2. Looks the Korean form up in a hand-built mapping table
  3. Renames the file to a readable English skill name

Run this ONCE from the repo root. It is idempotent — renamed files are skipped.
"""
import os
import sys
import io

# Force UTF-8 stdout so Korean names can be printed on Windows console
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ROSFX_DIR = r'C:/Sabri_MMO/ROSFX/effect'

# Korean -> English RO skill name mapping. Class prefix gives a hint about which
# job uses the effect. Renewal-era skills are kept (Sorcerer/Genetic/Sura/Taekwon)
# even though Sabri_MMO is pre-renewal — they may be useful for late-game expansion.
KOREAN_TO_ENGLISH = {
    # ---- "g" = Genetic (renewal merchant 3rd class) ----
    'g가시나무덫.wav':           'genetic_thorns_trap.wav',
    'g데모닉화이어.wav':         'genetic_demonic_fire.wav',
    'g스포어익스플로젼.wav':     'genetic_spore_explosion.wav',
    'g카트부스트.wav':           'genetic_cart_boost.wav',
    'g카트캐논.wav':             'genetic_cart_cannon.wav',
    'g카트토네이도.wav':         'genetic_cart_tornado.wav',
    'g크레이지위드.wav':         'genetic_crazy_weed.wav',
    'g하울링오브만드라고라.wav': 'genetic_howling_mandragora.wav',

    # ---- "mon_" = Sura / Monk monk-line skills ----
    'mon_금강불괴.wav':       'sura_gentle_touch_quiet.wav',   # Diamond Body / Iron Body
    'mon_맹룡과강.wav':       'sura_dragon_combo.wav',          # Fierce Dragon
    'mon_아수라 패황권.wav':  'monk_asura_strike.wav',          # Asura Strike
    'mon_연환.wav':           'monk_combo_chain.wav',           # Chain Combo
    'mon_침투경.wav':         'sura_skynet_blow.wav',           # Penetration
    'mon_탄지신통.wav':       'sura_finger_offensive.wav',      # Touch of God / Finger
    'mon_폭기.wav':           'sura_explode.wav',               # Explosive Bomb

    # ---- "s" = Sorcerer (renewal mage 3rd class) ----
    's바레티르스피어.wav':     'sorcerer_varetyr_spear.wav',
    's바큠익스트림.wav':       'sorcerer_vacuum_extreme.wav',
    's사이킥웨이브.wav':       'sorcerer_psychic_wave.wav',
    's스트라이킹.wav':         'sorcerer_striking.wav',
    's스펠피스트.wav':         'sorcerer_spell_fist.wav',
    's워머.wav':               'sorcerer_warmer.wav',
    's일렉트릭워크.wav':       'sorcerer_electric_walk.wav',
    's클라우드킬.wav':         'sorcerer_cloud_kill.wav',
    's파이어워크.wav':         'sorcerer_fire_walk.wav',
    's포이즌버스터.wav':       'sorcerer_poison_buster.wav',

    # ---- "t_" = Taekwon / Star Gladiator / Soul Linker ----
    't_공격력.wav':       'taekwon_attack_stance.wav',     # ATK power up
    't_낙법.wav':         'taekwon_tumbling.wav',           # Tumbling
    't_날라차기.wav':     'taekwon_flying_kick.wav',        # Flying side kick
    't_내려찍기.wav':     'taekwon_axe_kick.wav',           # Heel drop
    't_달리기.wav':       'taekwon_running.wav',            # Sprint
    't_돌려차기.wav':     'taekwon_roundhouse_kick.wav',
    't_등록.wav':         'taekwon_register.wav',           # Feeling-of-the-sun-moon-stars register
    't_따듯한마법.wav':   'soullinker_warm_magic.wav',      # Warm wind buff
    't_마법.wav':         'soullinker_magic.wav',
    't_마법반사.wav':     'soullinker_magic_reflection.wav',
    't_바람마법.wav':     'soullinker_wind_magic.wav',
    't_바람방출.wav':     'soullinker_wind_release.wav',
    't_방어형.wav':       'taekwon_defensive_stance.wav',
    't_벽튕김.wav':       'taekwon_wall_bounce.wav',
    't_변신.wav':         'stargladiator_transform.wav',    # Union of Body and Soul
    't_보조마법.wav':     'soullinker_support_magic.wav',
    't_삐용.wav':         'taekwon_beep_sfx.wav',            # generic UI beep
    't_슈웃.wav':         'taekwon_shoot.wav',
    't_안락한마법.wav':   'soullinker_comfort_magic.wav',
    't_에너지방출.wav':   'taekwon_energy_release.wav',
    't_영혼.wav':         'soullinker_soul.wav',
    't_전기.wav':         'taekwon_electric.wav',
    't_점프.wav':         'taekwon_jump.wav',
    't_쳐내기.wav':       'taekwon_strike_out.wav',
    't_치잉.wav':         'taekwon_cheeing.wav',
    't_캐스팅.wav':       'taekwon_casting.wav',
    't_피링.wav':         'taekwon_feeling.wav',             # Feel skill
    't_회오리차기.wav':   'taekwon_tornado_kick.wav',
    't_회피.wav':         'taekwon_dodge.wav',
    't_회피2.wav':        'taekwon_dodge2.wav',
    't_효과음1.wav':      'taekwon_effect1.wav',

    # ---- No-prefix renewal/extended class skills ----
    '가스펠.wav':                 'arch_bishop_gospel.wav',
    '고성방가.wav':               'minstrel_loud_song.wav',
    '그라운드.wav':               'warlock_earth_strain.wav',          # Ground / Earth Strain
    '그레이트에코.wav':           'minstrel_great_echo.wav',
    '그림자베기.wav':             'shadow_chaser_shadow_slash.wav',
    '기공포.wav':                 'sura_gates_of_hell.wav',
    '김서방돈.wav':               'genetic_zeny_research.wav',         # 'Zeny throw' Mr Kim's money
    '나락의노래.wav':             'minstrel_song_of_hell.wav',         # Lerad's Dew?
    '나를잊지말아요.wav':         'wanderer_dont_forget_me.wav',
    '뇌격쇄.wav':                 'sura_lightning_walk.wav',           # Brain Storm / Thunder
    '니벨룽겐의 반지.wav':        'minstrel_ring_of_nibelungen.wav',
    '닌자_던지기.wav':            'ninja_throw.wav',
    '다다미뒤집기.wav':           'ninja_tatami_gaeshi.wav',
    '달빛.wav':                   'taekwon_moonlight.wav',             # Moonlight kick
    '달빛세레나데.wav':           'minstrel_moonlit_serenade.wav',
    '당신을 위한 서비스.wav':     'minstrel_service_for_you.wav',
    '더스트샷.wav':               'rebellion_dust_shot.wav',           # Gunslinger Dust
    '데스페라도.wav':             'gunslinger_desperado.wav',
    '드래곤테일.wav':             'rune_knight_dragon_breath.wav',     # 'Dragontail'
    '디스암.wav':                 'rebellion_disarm.wav',              # Disarm shot
    '래피드샤워.wav':             'gunslinger_rapid_shower.wav',
    '레라드의이슬.wav':           'minstrel_lerads_dew.wav',
    '로키.wav':                   'minstrel_loki_veil.wav',
    '마나의노래.wav':             'minstrel_mana_song.wav',
    '마법력 증폭.wav':            'sage_magic_power_amplify.wav',      # Sage Mind Breaker?
    '매지컬블릿.wav':             'gunslinger_magical_bullet.wav',
    '매직 크래쉬.wav':            'royal_guard_magic_crasher.wav',
    '맹호경파산.wav':             'sura_tiger_cannon.wav',
    '메아리의노래.wav':           'minstrel_song_of_echo.wav',
    '메테오 어썰트.wav':          'guillotine_cross_meteor_assault.wav',
    '멜로디오브싱크.wav':         'wanderer_melody_of_sink.wav',
    '바실리카.wav':               'priest_basilica.wav',
    '바인드트랩.wav':             'ranger_bind_trap.wav',
    '배니싱버스터.wav':           'rebellion_banishing_buster.wav',
    '버서크.wav':                 'lord_knight_berserk.wav',
    '복호격.wav':                 'sura_combo_finish.wav',
    '불사신.wav':                 'lord_knight_immortality.wav',
    '브라기의 시.wav':            'minstrel_brage_poem.wav',
    '비욘드오브워크라이.wav':     'minstrel_beyond_warcry.wav',
    '비트 조인트.wav':            'wanderer_metallic_sound.wav',       # Beat Joint
    '빙정락.wav':                 'sura_diamond_storm.wav',
    '사망의골짜기에서.wav':       'minstrel_valley_of_death.wav',
    '사운드오브디스트럭션.wav':   'wanderer_sound_of_destruction.wav',
    '새터데이나이트피버.wav':     'wanderer_saturday_night_fever.wav',
    '샤프슈팅.wav':               'ranger_sharp_shooting.wav',
    '섀터스톰.wav':               'genetic_shatter_storm.wav',
    '석양의 어쌔신.wav':          'guillotine_cross_sunset.wav',
    '세이렌의목소리.wav':         'wanderer_voice_of_siren.wav',
    '세크리파이스.wav':           'royal_guard_sacrifice.wav',
    '소울 체인지.wav':            'soullinker_soul_change.wav',
    '수둔.wav':                   'ninja_water_escape.wav',            # Sudon - water hide
    '수줍은하루의우울.wav':       'wanderer_melancholy_song.wav',
    '순환하는자연의소리.wav':     'wanderer_circling_nature.wav',
    '스윙댄스.wav':               'wanderer_swing_dance.wav',
    '슬러그탄.wav':               'gunslinger_slug_shot.wav',
    '심연속으로.wav':             'minstrel_into_the_abyss.wav',
    '아숨프티오.wav':             'arch_bishop_assumptio.wav',
    '안개베기.wav':               'guillotine_cross_dark_illusion.wav',
    '안식의자장가.wav':           'wanderer_lullaby_rest.wav',
    '안티매터리얼블래스트.wav':   'rebellion_anti_material_blast.wav',
    '애로우 발칸.wav':            'ranger_arrow_vulcan.wav',
    '언리미티드허밍보이스.wav':   'wanderer_humming_voice.wav',
    '연인들을위한심포니.wav':     'wanderer_lovers_symphony.wav',
    '연주붕격.wav':               'shura_collapse_strike.wav',
    '영원의 혼돈.wav':            'minstrel_eternal_chaos.wav',
    '오라 블레이드.wav':          'lord_knight_aura_blade.wav',
    '워그와함께춤을.wav':         'ranger_warg_dance.wav',
    '윈드워크.wav':               'minstrel_wind_walker.wav',
    '이둔의 사과.wav':            'minstrel_apple_of_idun.wav',
    '일섬.wav':                   'kagerou_one_strike.wav',            # Single flash
    '자장가.wav':                 'dancer_lullaby.wav',
    '전장의.wav':                 'minstrel_battle_chant.wav',
    '크래커.wav':                 'taekwon_cracker.wav',
    '크림즌마커.wav':             'rebellion_crimson_marker.wav',
    '트리플액션.wav':             'rebellion_triple_action.wav',
    '파이어레인.wav':             'rebellion_fire_rain.wav',
    '폭염룡.wav':                 'rune_knight_phantom_thrust.wav',    # Blazing Dragon
    '폭죽.wav':                   'taekwon_firework.wav',
    '풀버스터.wav':               'rebellion_full_buster.wav',
    '풍인.wav':                   'kagerou_wind_blade.wav',
    '풍차를향해돌격.wav':         'royal_guard_pinpoint_attack.wav',   # Charge windmill
    '프레셔.wav':                 'royal_guard_pressure.wav',
    '플리커.wav':                 'rebellion_flicker.wav',
    '플립.wav':                   'rebellion_flip.wav',
    '피어싱샷.wav':               'ranger_piercing_shot.wav',
    '하모나이즈.wav':             'wanderer_harmonize.wav',
    '하울링마인.wav':             'ranger_howling_mine.wav',
    '해머오브갓.wav':             'rebellion_hammer_of_god.wav',
    '행운의.wav':                 'minstrel_lucky_song.wav',
    '헤드 크러쉬.wav':            'royal_guard_overbrand.wav',
    '헤르모드의 지팡이.wav':      'minstrel_hermode_staff.wav',
    '화염진.wav':                 'sura_blaze_array.wav',              # Flame Array
    '흡기.wav':                   'sura_inhale.wav',
    '흥얼거림.wav':               'wanderer_humming.wav',
}


def main():
    if not os.path.isdir(ROSFX_DIR):
        print(f"ERROR: {ROSFX_DIR} not found")
        sys.exit(1)

    files = sorted(os.listdir(ROSFX_DIR))
    renamed = 0
    skipped = 0
    unknown = 0
    failed = 0

    for f in files:
        full = os.path.join(ROSFX_DIR, f)
        if not os.path.isfile(full):
            continue

        # Try ASCII first — if pure ASCII, leave alone
        try:
            f.encode('ascii')
            continue  # Already ASCII, no rename needed
        except UnicodeEncodeError:
            pass

        # Decode mojibake
        try:
            korean_name = f.encode('latin1').decode('cp949')
        except Exception as e:
            print(f"DECODE FAIL: {f!r} -> {e}")
            failed += 1
            continue

        # Look up in mapping
        new_name = KOREAN_TO_ENGLISH.get(korean_name)
        if not new_name:
            print(f"UNKNOWN:    {korean_name}")
            unknown += 1
            continue

        new_full = os.path.join(ROSFX_DIR, new_name)
        if os.path.exists(new_full):
            print(f"SKIP (exists): {new_name}")
            skipped += 1
            continue

        try:
            os.rename(full, new_full)
            print(f"RENAMED: {korean_name} -> {new_name}")
            renamed += 1
        except OSError as e:
            print(f"RENAME FAIL: {korean_name} -> {new_name} ({e})")
            failed += 1

    print()
    print(f"=== SUMMARY ===")
    print(f"Renamed: {renamed}")
    print(f"Skipped: {skipped}")
    print(f"Unknown: {unknown}")
    print(f"Failed:  {failed}")


if __name__ == '__main__':
    main()
