# Ragnarok Online Classic — Audio System Research

**Date**: 2026-04-06
**Purpose**: Comprehensive reference on how RO Classic audio works, where the assets live, what they're called, how to obtain them, and the legal/practical paths for using RO-style audio in Sabri_MMO.

---

## TL;DR — The Direct Answer

**Q: Can I download RO audio and modify it slightly for use in our game?**

**A: No.** Three independent reasons:

1. **Gravity Co. Ltd. owns the copyright** to all RO music and SFX (SoundTeMP composed under work-for-hire). Modifying audio does NOT transfer ownership — derivative works still infringe.
2. **Gravity is actively litigious as of 2022.** They won a **$4 million default judgment against NovaRO** in 2022-2023 and ran 7+ concurrent lawsuits in North America. A fan game shipping RO audio in its binary is in a *worse* legal position than a private server using player-supplied clients.
3. **"EQ tweak" or "5% pitch shift" is not transformative under post-Warhol fair use.** Same purpose, same audience, same market substitution = textbook derivation, not transformation.

**The path forward** (covered in Section 7): Sonniss GDC bundles + Kenney + OpenGameArt + Kevin MacLeod for free placeholder/production, then commission 5-10 hero tracks from a Bandcamp composer for the most-played zones. Total cost can be $0 to ~$1,000 depending on tier.

You CAN legally:
- Study the audio for research/reference (listening to the official 2023 Apple Music release is the cleanest path)
- Extract assets locally for analysis (measure SFX length/loudness/shape as a target)
- Stream the official OST on Spotify/Apple Music while developing
- Use these findings to brief composers and AI generation prompts

You CANNOT legally:
- Ship Gravity-owned audio in any build of Sabri_MMO
- Distribute extracted RO audio publicly
- Modify RO audio "slightly" and call it yours

---

## 1. Audio File Structure

### 1.1 Two storage locations

The Ragnarok client splits audio across two completely different storage systems inside one install:

| Asset class | Location | Format | Reason |
|---|---|---|---|
| **BGM** (background music) | `BGM/` folder, **loose files on disk**, sibling to `Ragnarok.exe` | MP3 | Streamed at runtime via Miles Sound System (`Mp3dec.asi`, `Mssfast.m3d`). Loose files mean users can swap BGM quality without repackaging. |
| **SFX** (sound effects) | Inside `data.grf`, at internal path `data\wav\` and `data\wav\effect\` | WAV (PCM) | Tightly coupled to map/skill/monster lookups, packed for fast indexed read. |

The Ragnarok Offline pre-renewal pack ships `LQ_BGM.7z` and `HQ_BGM.7z` *separately* from the GRFs — confirming the loose-MP3 design.

### 1.2 GRF file format

A **GRF** (Gravity Resource File) is Gravity's proprietary archive format, similar to ZIP/PAK but with DES-variant encryption.

**GRF v2.0 header** (the standard since 2004):
```
Signature           15 bytes  "Master of Magic" (no null terminator)
Encryption Key      15 bytes  Zero-filled when unused
FileTableOffset      4 bytes  Relative offset to file table
ScramblingSeed       4 bytes  For content obfuscation
ScrambledFileCount   4 bytes  Subtract seed and 7 for actual count
Version              4 bytes  Major.minor encoded
```

**File table** is a zlib-compressed buffer of records:
```
CompressedSize    4 bytes
ByteAlignedSize   4 bytes  // padded to 8-byte boundary
DecompressedSize  4 bytes
Type              1 byte   // raw / compressed / encrypted
Offset            4 bytes
```

**Format versions**:
- **GRF 1.2** — fRO (French) early client
- **GRF 1.3** — 2003 iRO Beta
- **GRF 2.0** — kRO and all regions since 2004 — **the version pre-renewal uses**
- **GRF 0x300** — 2024+ format supporting >4 GB archives (Event Horizon / Renewal Lite)

Hard cap on v2.0 is ~2 GB. A modern full kRO install is ~4-6 GB total.

### 1.3 BGM ↔ map mapping (`mp3nametable.txt`)

**RSW map files do NOT contain BGM references.** They only contain Spatialized 3D Audio Sources (object type 3) for ambient positional SFX — frogs, water, fires. Background music is mapped externally via `data\mp3nametable.txt`.

**Format** (one entry per line):
```
mapname.rsw#bgm\\NN.mp3#
```

- `#` characters are field delimiters at both ends
- `\\` is a literal pair of backslashes (escape-encoded path separator)
- Lines starting with `//` are comments — this is how Christmas/event music is toggled
- Track numbers are 1-3 digits, conventionally zero-padded (`08.mp3`, `100.mp3`, `997.mp3`)

**Sample lines** (from `zackdreaver/ROenglishRE`):
```
prontera.rsw#bgm\\08.mp3#
prt_in.rsw#bgm\\08.mp3#
geffen.rsw#bgm\\13.mp3#
geffen_in.rsw#bgm\\13.mp3#
prt_fild01.rsw#bgm\\12.mp3#
prt_fild05.rsw#bgm\\12.mp3#
1@xm_d.rsw#bgm\\997.mp3#
```

The full file in modern clients has ~1,096-1,222 active entries.

**Sub-zone inheritance patterns**:
- **Town + interiors share one track** (`prontera` + `prt_in` = `08.mp3`)
- **Field 1-N variants split into 2-3 tracks for variety** (`prt_fild01/05/06/08/09/12` use `12.mp3`; `prt_fild02/03/04/07` use `05.mp3`)
- **Dungeon level variants share a track per depth tier** (`pay_dun01/02` use `20.mp3`; `pay_dun03/04` use `47.mp3`)
- **All WoE castles use one global theme** — every `*g_cas01-05`, `gld_dun*`, `te_*cas*` maps to `66.mp3` (Wanna Be Free!!)
- **PvP arena maps cycle by tier** — `pvp_n_*-1` through `*-5` use 5 different tracks

### 1.4 SFX file structure

| Path | Contents |
|---|---|
| `data\wav\` | Monster sounds, weapon hit sounds, status effect sounds, body-material reaction sounds, level up, item drops |
| `data\wav\effect\` | Skill SFX (one wav per skill effect), traps, ground effects, weather, item-script sounds |

**File format conventions**:
- Format: WAV (PCM)
- Sample rate: Mostly 22.05 kHz, some 11.025 kHz, some 44.1 kHz
- Channels: Mostly mono (allowing 3D spatialization)
- Bit depth: 16-bit PCM (some 8-bit legacy)
- BGM equivalent: 96 kbps CBR MP3, 44.1 kHz stereo (deliberately low quality for early-2000s install sizes)

### 1.5 Encoding warning — Korean filenames

Many original wav files use **EUC-KR Korean characters**. When extracted with the wrong locale, they appear as garbled bytes like `\xB9\xF6\xC6\xB0\xBC\xD2\xB8\xAE.wav` or `¹öÆ°¼Ò¸®.wav`.

The single most important Korean filename is `버튼소리.wav` ("button sound") — the universal UI click used for *every* button in the classic client (login, character create, character select, ESC menu, NPC dialog Next, item buttons, whisper notification).

Other notable Korean-named files:
- `폭죽.wav` — firework / pokjuk
- `자장가.wav` — Lullaby (Bard skill)
- `김서방돈.wav` — Mr. Kim's ballad
- `영원의 혹돈.wav` — Eternal Chaos
- `휘파람.wav` — Whistle
- `브라기의 시.wav` — A Poem of Bragi
- `이듣의 사과.wav` — Apple of Idun
- `힘기.wav` — power up

### 1.6 Sound triggering — three sources

Sounds are NOT in any single skill database. They're referenced from three places:

1. **ACT files** (sprite animations) embed sound names per-frame — that's how monster sounds and base weapon swing sounds get triggered. The sound plays when the animation reaches that frame.
2. **EffectTable / .str effect files** — each skill visual effect carries an optional `wav:` field that the client plays when the effect spawns. This is the canonical skill SFX system.
3. **RSW map files** — environmental ambient sounds are stored as 3D positioned sound sources per-map (`Name`, `SoundFile`, `Range`, `CycleInterval`, `Position X/Y/Z`, `VolumeGain`).

This means **for Sabri_MMO**, you should:
- Add a `wav` field to your `FSkillVFXConfig` struct alongside the Niagara system (skills)
- Add `attackSound`, `damageSound`, `dieSound`, `moveSound` fields to monster templates
- Use weapon-type-keyed lookup tables for hit/swing sounds
- Use map-level ambient placement for environmental audio

---

## 2. Complete BGM Tracklist

The pre-renewal kRO inventory has tracks **01-122** in widely-shipped clients (Episode 11.x cutoff). Tracks 123-176 are renewal-era additions. Track **997** is a special silence track used only by Christmas event horror dungeons.

Track names verified against the Internet Archive `121_20191019` collection and cross-referenced with `mp3nametable.txt`.

### 2.1 Tracks 01-50 — Original 2003 era

| # | Title | Notable Map(s) |
|---|---|---|
| 01 | **Title** | Login screen / character select (played by client, not assigned to any RSW) |
| 02 | Gambler of Highway | PvP T1 maps, job change rooms (knt/cru/sage/wiz/prist/hunte) |
| 03 | Peaceful Forest | `moc_fild02/03/13`, `pay_fild01-04/08/11` |
| 04 | I Miss You | `prt_fild09/10/11`, `gef_fild11` |
| 05 | Tread on the Ground | `prt_fild00/02/03/04/07` |
| 06 | Risk Your Life | `mag_dun01/02/03`, ordeal maps |
| 07 | Wind of Tragedy | `06guild_01-08`, `guild_vs1-5`, `quiz_00` |
| 08 | **Theme of Prontera** | `prontera`, `prt_in`, `itemmall`, `auction_01` — the most-recognized RO track |
| 09 | Great Honor | `prt_castle`, `prt_cas`, `himinn`, `valkyrie` |
| 10 | Divine Grace | `prt_church`, `odin_past`, `prt_lib` |
| 11 | **Theme of Morroc** | `morocc`, `morocc_in` |
| 12 | Streamside | `prt_fild01/05/06/08/12`, `prt_fild08a-d` |
| 13 | **Theme of Geffen** | `geffen`, `geffen_in`, `gef_tower` |
| 14 | **Theme of Payon** | `payon`, `payon_in01-03`, `pay_arche` |
| 15 | **Theme of Alberta** | `alberta`, `alberta_in` |
| 16 | Labyrinth | `prt_maze01/02/03` |
| 17 | Treasure Hunter | `treasure01/02`, PvP T3 |
| 18 | Time Up!! | `quiz_01/02`, `force_*`, PvP T2 |
| 19 | Under the Ground | `prt_sewb1/2/3/4` (Prontera Culvert), PvP T4 |
| 20 | Ancient Groover | `pay_dun00/01/02` (Payon Cave upper) |
| 21 | Through the Tower | `gef_dun00/01` (Geffen Tower upper), PvP T5 |
| 22 | Backattack!! | `moc_pryd01-06` (Pyramid all floors) |
| 23 | Travel | `gef_fild01/05/06/08/09` |
| 24 | Desert | `moc_fild01/04/05/06/07` |
| 25 | Plateau | `gef_fild00/04/07` |
| 26 | Everlasting Wanderers | `izlude`, `izlude_in`, `iz_ac01/02` |
| 27 | Dreamer's Dream | `mjo_dun01/02/03` |
| 28 | You're in Ruins | `monk_test`, `job_monk`, `prt_monk`, `monk_in`, `jawaii` |
| 29 | Be Nice 'n Easy | `iz_dun00/01/02` (Byalan upper) |
| 30 | One Step Closer | `new_1-1` through `new_5-4` (Novice training grounds) |
| 31 | Brassy Road | `mjolnir_01/06/07/09` |
| 32 | Yuna Song | (no current assignment) |
| 33 | Pampas Upas | `mjolnir_02/03/04/08` |
| 34 | TeMPoison | `mjolnir_05/10/11/12` |
| 35 | Nano East | `gef_fild02/03/10`, `in_orcs01` |
| 36 | TeMPorsche | `pay_fild05/06/07/09/10` |
| 37 | Hamatan | `cmd_fild01-09`, `moc_fild08-19` |
| 38 | Sphinx theme | `in_sphinx1-5` |
| 39 | **Theme of Al de Baran** | `aldebaran`, `aldeba_in`, `alde_alche` |
| 40 | Monk Zonk | `gl_chyard`, `gl_church`, `gl_prison/1` |
| 41 | Rag All Night Long | `beach_dun/2/3` (Comodo Beach Dungeon) |
| 42 | Curse'n Pain | `glast_01`, `gl_dun01/02`, `gl_in01`, `gl_step`, `sec_pri` |
| 43 | Morning Gloomy | `gl_cas01/02` (Glast Heim Castle) |
| 44 | TeMP it Up | `gl_knt01/02`, `gefenia01-04`, `gld_dun01-04` |
| 45 | Don't Piss Me Off | `moc_fild16` |
| 46 | An Ant-Lion's Pit | `anthell01/02` |
| 47 | Welcome Mr. Hwang | `pay_dun03/04` (Payon Cave deep) |
| 48 | Help Yourself | `orcsdun01/02`, `gl_sew01-04` |
| 49 | Watery Grave | `iz_dun03/04/05` (Byalan deep) |
| 50 | Out of Curiosity | `gef_dun02/03` (Geffen Tower deep), `bossnia_01-04` |

### 2.2 Tracks 51-100 — Episode 1-9 era

| # | Title | Notable Map(s) |
|---|---|---|
| 51 | Believe in Myself | All `*_1-1`/`2-1`/`3-1` job training maps |
| 52 | Ready~ | Arena lobbies, ship interiors, PvP rooms |
| 53 | White Christmas | (event, commented out) |
| 54 | Come on My Deer!! | (Christmas event, commented) |
| 55 | Welcome, My Lord | `xmas_fild01` |
| 56-57 | Silver Bell / Don't Cry, Baby | (Christmas, commented) |
| 58 | Jingle Bell on Ragnarok | `xmas_dun01/02` |
| 59 | Theme of Lutie / Snow In My Heart | `xmas`, `xmas_in` |
| 60 | Aeon | `c_tower1/2/3/4` (Clock Tower) |
| 61 | Zingaro | `alde_dun01/02/03/04` (Clock Tower deep) |
| 62 | High Roller Coaster | `comodo`, `cmd_in01/02`, `job_duncer` |
| 63 | Mucho Gusto | `cmd_fild03-09` |
| 64 | One Fine Day | `gef_fild12/14`, `int_land` |
| 65 | Into the Abyss | `tur_dun01-06` (Turtle Island) |
| 66 | **Wanna Be Free!!** | **ALL WoE castles** + agit areas + guild dungeons |
| 67 | TeMPotato | (no current assignment) |
| 68 | Jazzy Funky Sweety | `umbala`, `um_in` |
| 69 | Retro Metro | `um_dun01/02` (Umbala Dungeon) |
| 70 | **Theme of Juno** | `yuno`, `yuno_in01-05`, `y_airport` |
| 71 | Antique Cowboy | `yuno_fild01/02/08/09/11/12` |
| 72 | Big Guys Love This | `yuno_fild03-07/10`, `hu_fild01/04/07` |
| 73 | Higher Than the Sun | `ama_fild01`, `gon_fild01` |
| 74 | Not So Far Away | `gonryun`, `gon_in` (Gonryun Town) |
| 75 | Come, and Get It! | `gon_dun01/02/03` (Gonryun Dungeon) |
| 76 | Purity of Your Smile | `amatsu`, `ama_in01/02` |
| 77 | Can't Go Home Again, Baby | `ama_dun01/02/03` (Amatsu Dungeon) |
| 78 | adios | `um_fild02/04` |
| 79 | The Great | `louyang`, `lou_in01/02`, `lou_fild01` |
| 80 | Jumping Dragon | `lou_dun01/02/03` (Louyang Dungeon) |
| 81 | Thai Orchid | `ayothaya`, `ayo_in01/02`, `ayo_fild01` |
| 82 | Muay Thai King | `ayo_dun01/02` (Ayothaya Dungeon) |
| 83 | Sleepless | `yggdrasil01` |
| 84 | Christmas in the 13th Month | `niflheim`, `nif_in` |
| 85 | Dancing Christmas in the 13th Month | `nif_fild01/02` |
| 86 | Steel Me | `airport`, `ein_in01`, `einbroch`, `einbech` |
| 87 | Ethnica | `ein_fild01-10`, `airplane` |
| 88 | Come in Peace | `ein_dun01/02/03` |
| 89 | We Have Lee but You Don't Have | `juperos_01/02`, `jupe_*` |
| 90 | Noblesse Oblige | `lighthalzen`, `lhz_in01/02/03` |
| 91 | CheongChoon | `lhz_fild01/02/03`, `hu_fild02/03` |
| 92 | Naive Rave | `lhz_dun01-04` (Bio Lab) |
| 93 | Latinnova | `hugel`, `hu_in01` |
| 94 | Theme of Rachel | `rachel`, `ra_in01` |
| 95 | Underneath the Temple | `ra_temple`, `ra_san01-05` (Sanctuary) |
| 96 | Yetit Petit | `ice_dun01-04` (Ice Dungeon) |
| 97 | Lastman Dancing | `ra_fild02/03/07/08/10-13` |
| 98 | Top Hoppy | `ra_fild01/04/05/06/09` |
| 99 | Uncanny Lake | `hu_fild05` |
| 100 | Abyss | `abyss_01-04` (Abyss Lakes) |

### 2.3 Tracks 101-160 — Late pre-renewal + early renewal

| # | Title | Notable Map(s) |
|---|---|---|
| 101 | AbSolitude | `kh_mansion`, `kh_school`, `kh_vila` |
| 102 | Erebos' Prelude | `tha_t01/02` (Thanatos Tower) |
| 103 | Invisible Invasion | `thana_boss` |
| 104 | On Your Way Back | `veins`, `ve_in/in02` |
| 105 | Rose of Sharon | `ve_fild01-07` |
| 106 | Sleeping Volcano | `thor_camp`, `thor_v01/02/03` |
| 107 | Seven Days Seven Nights | `tha_t03/04/05/06` |
| 108 | Angelica | `tha_t07-12`, `thana_step` |
| 109 | Alpen Rose | `hu_fild06` |
| 110 | Kingdom Memories | `odin_tem01/02/03` |
| 111 | Good Morning | `nameless_i`, `nameless_in` |
| 112 | Good Night | `nameless_n` |
| 113 | Monastery in Disguise | `abbey01/02/03` |
| 114 | Theme of Moscovia | `moscovia`, `mosk_in` |
| 115 | Tale of the East | `mosk_ship`, `mosk_fild02` |
| 116 | Away from Home | `mosk_dun01/02/03` |
| 117 | Dream of a Whale | `mosk_fild01` |
| 118 | Taiko's Fury | `que_ba` |
| 119 | Stained Memories | PvP T5 maps |
| 120 | Fissure Eruption | `moc_fild20/21` |
| 121 | Outer Breath | `moc_fild22`, `moc_fild22b` |
| 122 | Ethical Aspiration | `cave`, `1@uns`, `un_bk_q` (**Pre-renewal cutoff**) |
| 123 | Into the Arena! | Battlegrounds maps |
| 124 | Stranger Aeons | `1@gol1/gol2`, dungeon instances |
| 125 | Forbidden Anguish | `prt_prison`, `1@tower` through `6@tower` |
| 126 | New World Order | `mid_camp`, `mid_campin` |
| 127 | Mystic Haze | `man_fild01/02/03` |
| 128 | Splendid Dreams | `splendide`, `spl_fild01-03` |
| 129 | Fireflies Heaven | `1@nyd`, `nyd_dun01/02` |
| 130 | Dread and Bold | `2@nyd` |
| 131 | Daytime in Manuk | `manuk`, `man_in01` |
| 132 | March with Irish Whistle | `splendide`, `spl_in01/02` |
| 133 | Sunny Side of Life | `brasilis`, `bra_in01` |
| 134 | Borborema | `bra_fild01` |
| 135 | At Dusk | `bra_dun01/02` |
| 136 | Emotion Creep | `dic_dun01/02/03` |
| 137 | Dazzling Snow | `dicastes01/02` |
| 138 | TeMPlatonic | `dic_fild01/02` |
| 139 | Sugar Cane Carnival | `mora` |
| 140 | Twilight Heaven | `bif_fild01/02`, `ecl_fild01` |
| 141 | Tricky Cheeky | `1@mist` |
| 142 | Arrival | `dewata`, `paramk` |
| 143 | Mother Earth | `dew_in01`, `dew_fild01` |
| 144 | Silent Voyage | `dew_dun01/02` |
| 145 | Marshmallow Waltz | `malangdo`, `mal_in01/02` |
| 146 | Diamond Dust | `mal_dun01`, `silk_lair` |
| 147 | Octopus Scramble | `1@cash` |
| 148 | Melt Down! | `1@pump`, `2@pump` |
| 149 | Theme of Port Malaya | `malaya`, `ma_in01` |
| 150 | Uncanny Dreams | `1@ma_h`, `ma_dun01` |
| 151 | Maximum | `1@ma_b` |
| 152 | Voices | `1@ma_c`, `1@def03` |
| 153 | Horizon | `ma_scene01`, `ma_fild01/02` |
| 154 | Eclage (1) | `ecl_tdun01-04` |
| 155 | Eclage (2) | `eclage`, `ecl_in01-04`, `ecl_hub01` |
| 156 | Beyond the Grave | `1@eom` |
| 157 | Disillusion | `1@dth1/2/3` |
| 158 | Indelible Scars | `1@rev` |
| 159 | Judgement | `moro_vol`, `moro_cav` |
| 160 | Jittering Nightmare | `1@mcd`, `1@jtb` |

### 2.4 Tracks 161-176 — Renewal-era additions (post-cutoff)

| # | Notable Map(s) |
|---|---|
| 161 | `verus01`, `verus02` |
| 162 | `1@lab`, `1@rgsr` |
| 163 | `1@uns` |
| 164 | `verus03`, `verus04` |
| 165 | `1@sthb/c/d` |
| 166 | `1@gl_he`, `1@mir` |
| 167 | `prt_q` |
| 168 | `lasa_dun01-03`, `lasa_dun_q` |
| 169 | `lasa_fild01/02` |
| 170 | `lasagna`, `lasa_in01`, `conch_in` |
| 171 | `harboro1/2`, `har_in01` |
| 172 | `rockmi1/2`, `rockrdg1/2` |
| 173-176 | Late renewal additions (2018+) |

### 2.5 Special tracks

| # | Title | Use |
|---|---|---|
| **01** | **Title** | Login screen / character select. The most iconic track. Played by client, not in mp3nametable. |
| **08** | **Theme of Prontera** | The capital city. Heard by every player constantly. The cultural anchor. |
| **66** | **Wanna Be Free!!** | The WoE / castle siege theme. ALL guild castles use this. The "war music" of RO. |
| **997** | **(Silence)** | Used ONLY by Christmas event horror dungeons (`1@xm_d`, `1@xm_d2`) for atmosphere. ~11 KB OGG / 705 KB MP3 of effective silence. |

### 2.6 Composers

**SoundTeMP** is the Korean studio that composed RO. Founded 1992, became internationally famous via RO in 2002.

| Member | Alias | Notes |
|---|---|---|
| **Lee Seock-Jin** | **BlueBlue** | Primary composer of the most iconic RO tracks. **Deceased 2015.** Track 191 of the extended catalog ("Remembering BlueBlue") is a memorial. |
| Kwon Goo-Hee | Goopal | Active |
| Jang Seong-Woon | Nikacha | Active |
| Park Jin-Bae | Silhouetti / ESTi | Active |
| Park Soo-Il | — | Active |
| Nam Goo-Min | Nauts | Active |
| Kwak Dong-Il | Sevin | 1993-2004 (left) |

**Other contributors** (Renewal-era only, on the 2009 Lantis Complete Soundtrack discs 4-5):
- **Neocyon** — Disc 4 tracks 24-27
- **Warren** — Disc 5 tracks 7, 9
- **Workspace** — Disc 5 tracks 10, 12, 15

**All pre-renewal tracks (01-122) are pure SoundTeMP work.**

> ⚠️ **Correction to the existing `/sabrimmo-audio` skill**: The skill mentions Tobias Marberger as a contributor. **There is no documented connection between Tobias Marberger and Ragnarok Online or SoundTeMP** in any verified source (Wikipedia, Wikia, MusicBrainz, VGMdb, RPGFan, Discogs). This appears to be a misattribution and should be removed. All RO composer credits go to SoundTeMP as a collective with BlueBlue as primary composer.

### 2.7 Album releases

| Album | Year | Catalog | Label | Discs | Tracks |
|---|---|---|---|---|---|
| Ragnarok Online soundTeMP Special Remix!! | 2002 | — | — | 1 | 24 |
| **Ragnarok Online Original Soundtrack** | **2003-05-14** | **SSCX-10090~1** | **DigiCube** | 2 + bonus | **50** |
| Love Forever... | 2002 | — | — | 1 | — |
| The Memory of RAGNAROK | 2006-06-02 | — | — | 2 | 27 |
| Eien no Story / Ragna Girls | 2007-04-25 | — | — | EP | 6 |
| **RAGNAROK Online Complete Soundtrack** | **2009-08-26** | **LACA-9163~7** | **Lantis** | **5** | **141** |
| BGM Arrange Collection | 2012-12-03 | — | — | 1 | 14 |
| **Ragnarok Online (Original Game Soundtrack)** (digital) | **2023-07-21** | — | 5505082 Records DK | streaming | **22** |
| **Ragnarok Online BGM, Pt. 1 (Original Soundtrack)** | **2026** | — | — | streaming | **68** |

The 2023 and 2026 streaming releases on Apple Music / Spotify / iTunes are the **cleanest legal way to listen** to canonical RO music today (~$9.99 or free with Apple Music subscription).

---

## 3. SFX Inventory

All paths are relative to the GRF root. The conventions:
- `data\wav\<name>.wav` — monsters, weapons, status, body materials, level up
- `data\wav\effect\<name>.wav` — skills, traps, ground effects, items

### 3.1 Combat — Weapon Hit Sounds

Source: roBrowser/roBrowserLegacy `WeaponHitSoundTable.js`. Played when a weapon connects with a target.

| Weapon Type | Hit Sound File |
|---|---|
| Unarmed (NONE/KNUKLE) | `_hit_fist1.wav`, `_hit_fist2.wav`, `_hit_fist3.wav`, `_hit_fist4.wav` (random) |
| Shortsword / Sword / 2H Sword | `_hit_sword.wav` |
| Spear / 2H Spear | `_hit_spear.wav` |
| Axe / 2H Axe | `_hit_axe.wav` |
| Mace / 2H Mace | `_hit_mace.wav` |
| Rod / 2H Rod (Staff) | `_hit_rod.wav` |
| Bow | `_hit_arrow.wav` |
| Instrument (Bard) | `_hit_mace.wav` (re-used) |
| Whip (Dancer) | `_hit_mace.wav` (re-used) |
| Book (Sage) | `_hit_mace.wav` (re-used) |
| Katar | `_hit_mace.wav` (re-used) |
| Shuriken (Ninja) | `_hit_mace.wav` (re-used) |
| Handgun | `_hit_권총.wav` |
| Rifle | `_hit_라이플.wav` |
| Gatling | `_hit_개틀링한발.wav` |
| Shotgun | `_hit_샷건.wav` |
| Grenade Launcher | `_hit_그레네이드런쳐.wav` |

### 3.2 Combat — Player Body Reaction (when hit)

Source: `JobHitSoundTable.js`. Played on the target when damaged. Three "body material" buckets:

| Material | Wav File | Used By |
|---|---|---|
| **Cloth** | `player_clothes.wav` | Novice, Swordman, Magician, Acolyte, Merchant, Priest, Wizard, Blacksmith, Sage, Alchemist, Linker, SuperNovice, Rebellion |
| **Wood/Leather** | `player_wooden_male.wav` | Archer, Thief, Hunter, Assassin, Gunslinger, Ninja, Taekwon, Rogue, Bard, Dancer |
| **Metal/Plate** | `player_metal.wav` | Knight, Crusader, Monk, Star Gladiator |

### 3.3 Combat — Generic Element Hits

| Effect | Wav File |
|---|---|
| Generic neutral hit | `_enemy_hit_normal1.wav` |
| Wind / electric hit | `_enemy_hit_wind1.wav` |
| Fire hit | `_enemy_hit_fire2.wav` |
| Hit effect tier 2-6 | `effect/ef_hit2.wav` through `ef_hit6.wav` |

### 3.4 Combat — Heal / Recovery / Progression

| Effect | Wav File |
|---|---|
| Heal (skill, item, regen) | `_heal_effect.wav` |
| Level up (the iconic ascending chime) | `levelup.wav` (root, no extension/effect prefix in source) |

### 3.5 Status Effects

Source: `EntityState.js`. Played when status flag is added to an entity. All in `data\wav\` root with leading underscore.

| Status | Wav File |
|---|---|
| Frozen (impact) | `_frozen_explosion.wav` |
| Stone Curse petrified (impact) | `_stone_explosion.wav` |
| Stone Curse forming | `_stonecurse.wav` |
| Stun | `_stun.wav` |
| Curse | `_curse.wav` |
| Poison | `_poison.wav` |
| Blind | `_blind.wav` |
| Silence | `_silence.wav` |
| Sleep (snore) | `_snore.wav` |
| Cloaking on/off | `effect/assasin_cloaking.wav` |

> No "buff applied" / "buff expired" sound exists. Buffs only play their cast SFX.

### 3.6 Skill SFX (per class)

#### Mage / Wizard

| Skill | Wav File |
|---|---|
| Stone Curse | `effect/ef_stonecurse.wav` |
| Fire Ball | `effect/ef_fireball.wav` |
| Fire Wall | `effect/ef_firewall.wav` |
| Ice Arrow / Frost Diver projectile | `effect/ef_icearrow1.wav` / `ef_icearrow2.wav` / `ef_icearrow3.wav` (random) |
| Frost Diver freeze | `effect/ef_frostdiver2.wav` |
| Lightning Bolt / Thunderstorm | `effect/magician_thunderstorm.wav` |
| Fire Bolt / Fire Arrow | `effect/ef_firearrow1.wav` (random `ef_firearrow%d`) |
| Napalm Beat | `effect/ef_napalmbeat.wav` |
| Soul Strike | `effect/ef_soulstrike.wav` |
| Sight | `effect/ef_sight.wav` |
| Safety Wall | `effect/ef_glasswall.wav` |
| Quagmire | `effect/wizard_quagmire.wav` |
| Storm Gust | `effect/wizard_stormgust.wav` |
| Lord of Vermilion / Fire Ivy | `effect/wizard_fire_ivy.wav` |
| Meteor Storm | `effect/wizard_meteor.wav` |
| Fire Pillar (cast) | `effect/wizard_fire_pillar_a.wav` |
| Fire Pillar (trigger) | `effect/wizard_fire_pillar_b.wav` |
| Earth Spike | `effect/wizard_earthspike.wav` |
| Sightrasher | `effect/wizard_sightrasher.wav` |
| Ice Wall | `effect/wizard_icewall.wav` |
| Water Ball | `effect/wizard_waterball_chulung.wav` |
| Jupitel Thunder | `effect/chainlight.wav` |

#### Acolyte / Priest

| Skill | Wav File |
|---|---|
| Heal | `_heal_effect.wav` |
| Cure | `effect/acolyte_cure.wav` |
| Aspersio | `effect/priest_aspersio.wav` |
| Angelus | `effect/ef_angelus.wav` |
| Signum Crucis | `effect/ef_signum.wav` |
| Aqua Benedicta | `effect/ef_aqua.wav` |
| Blessing | `effect/ef_blessing.wav` |
| Increase Agility | `effect/ef_incagility.wav` |
| Decrease Agility | `effect/ef_decagility.wav` |
| Ruwach | `effect/ef_ruwach.wav` |
| Resurrection | `effect/priest_resurrection.wav` |
| Sanctuary | `effect/priest_sanctuary.wav` |
| Magnus Exorcismus | `effect/priest_magnus.wav` |
| Lex Aeterna | `effect/priest_lexaeterna.wav` |
| Lex Divina | `effect/priest_lexdivina.wav` |
| Status Recovery | `effect/priest_recovery.wav` |
| Suffragium | `effect/priest_suffragium.wav` |
| Impositio Manus | `effect/priest_impositio.wav` |
| Magnificat | `effect/priest_magnificat.wav` |
| Gloria | `effect/priest_gloria.wav` |
| Kyrie Eleison (caster) | `effect/priest_kyrie_eleison_a.wav` |
| Kyrie Eleison (target) | `effect/priest_kyrie_eleison_b.wav` |
| Benedictio | `effect/priest_benedictio.wav` |
| Slow Poison | `effect/priest_slowpoison.wav` |

#### Swordsman / Knight / Crusader

| Skill | Wav File |
|---|---|
| Bash | `effect/ef_bash.wav` |
| Magnum Break | `effect/ef_magnumbreak.wav` |
| Endure | `effect/ef_endure.wav` |
| Provoke | `effect/swordman_provoke.wav` |
| MVP fanfare | `effect/st_mvp.wav` |
| Two-Hand Quicken | `effect/knight_twohandquicken.wav` |
| Auto-Counter | `effect/knight_autocounter.wav` |
| Brandish Spear | `effect/knight_brandish_spear.wav` |
| Spear Boomerang | `effect/knight_spear_boomerang.wav` |
| Bowling Bash | `effect/knight_bowling_bash.wav` |
| Holy Cross | `effect/cru_holycross.wav` (also `cru_holy cross.wav` with space — both exist) |
| Grand Cross | `effect/cru_grand cross.wav` |
| Shield Boomerang | `effect/cru_shield boomerang.wav` |

#### Archer / Hunter

| Skill | Wav File |
|---|---|
| Skidtrap (general traps cast) | `effect/hunter_skidtrap.wav` |
| Shockwave Trap | `effect/hunter_shockwavetrap.wav` |
| Flasher | `effect/hunter_flasher.wav` |
| Remove Trap | `effect/hunter_removetrap.wav` |
| Blast Mine | `effect/hunter_blastmine.wav` |
| Claymore Trap | `effect/hunter_claymoretrap.wav` |
| Freezing Trap | `effect/hunter_freezingtrap.wav` |
| Spring Trap | `effect/hunter_springtrap.wav` |
| Sandman Trap | `effect/hunter_sandman.wav` |
| Ankle Snare | `effect/hun_anklesnare.wav` |
| Detecting (Ruwach trap) | `effect/hunter_detecting.wav` |
| Blitz Beat (Falcon attack) | `effect/hunter_blitzbeat.wav` |
| Land Mine | `effect/hunter_landmine.wav` |

#### Thief / Assassin / Rogue

| Skill | Wav File |
|---|---|
| Steal | `effect/ef_steal.wav` |
| Envenom | `effect/ef_poisonattack.wav` |
| Detoxify | `effect/ef_detoxication.wav` |
| Enchant Poison | `effect/assasin_enchantpoison.wav` (sic, single-S) |
| Cloaking | `effect/assasin_cloaking.wav` |
| Venom Dust | `effect/assasin_venomdust.wav` |
| Poison React | `effect/assasin_poisonreact.wav` |
| Venom Splasher | `effect/assasin_venomsplasher.wav` |
| Steal Coin | `effect/rog_steal coin.wav` |
| Back Stab | `effect/rog_back stap.wav` (sic — "stap" not "stab") |
| Intimidate | `effect/rog_intimidate.wav` |
| Strip Weapon/Shield/Armor/Helm | `effect/t_벗팀.wav` (Korean, "stripping") |

#### Merchant / Blacksmith / Alchemist

| Skill | Wav File |
|---|---|
| Mammonite | `effect/ef_coin2.wav` |
| Adrenaline Rush | `effect/black_adrenalinerush_a.wav` + `_b.wav` |
| Weapon Repair | `effect/black_weapon_repair_a.wav` |
| Hammer Fall | `effect/black_hammerfall.wav` |
| Weapon Perfection | `effect/black_weapon_perfection.wav` |
| Maximize Power | `effect/black_maximize_power_sword.wav`, `_circle.wav`, `_sword_bic.wav` |
| Over Thrust | `effect/black_overthrust.wav` |
| Refine Success (anvil) | `effect/bs_refinesuccess.wav` |
| Refine Failure (anvil) | `effect/bs_refinefailed.wav` |
| Pharmacy success | `effect/p_success.wav` |
| Pharmacy failed | `effect/p_failed.wav` |
| Cooking success | `cook_suc.wav` (root) |
| Cooking failed | `cook_fail.wav` (root) |

#### Sage

| Skill | Wav File |
|---|---|
| Magic Rod | `effect/sage_magic rod.wav` |
| Spell Breaker | `effect/sage_spell breake.wav` (sic — "breake") |
| Volcano | `effect/sage_volcano.wav` |
| Deluge | `effect/sage_deluge.wav` |
| Whirlwind | `effect/sage_whirlwind.wav` |

#### Bard / Dancer (Korean filenames)

| Skill | Wav File |
|---|---|
| Lullaby | `effect/자장가.wav` |
| Mr. Kim's Bowman ballad | `effect/김서방돈.wav` |
| Eternal Chaos | `effect/영원의 혹돈.wav` |
| Whistle | `effect/휘파람.wav` |
| Assassin Cross of Sunset | `effect/석양의 어슨싱.wav` |
| A Poem of Bragi | `effect/브라기의 시.wav` |
| Apple of Idun | `effect/이듣의 사과.wav` |
| Frost Joke (Unbarring Octave) | `effect/험얼거림.wav` |
| Don't Forget Me | `effect/나를잊지말아요.wav` |
| Service for You | `effect/당신을 위한 서비스.wav` |
| Hermode's Rod | `effect/허르모드의 막대.wav` |
| Battle Theme of Niflheim | `effect/니벨룸겟의 반지.wav` |

#### Monk / Sura

| Skill | Wav File |
|---|---|
| Crescent Elbow | `effect/sr_crescentelbow.wav` |
| Tiger Cannon | `effect/sr_tigercannon.wav` |
| Knuckle Arrow | `effect/sr_knucklearrow.wav` |
| Falling Empire | `effect/sr_fallenempire.wav` |
| Earth Shaker | `effect/sr_earthshaker.wav` |
| Howling of Lion | `effect/sr_howlingoflion.wav` |
| Cursed Circle | `effect/sr_cursedcircle.wav` |
| Dragon Combo | `effect/sr_dragoncombo.wav` |
| Lightning Walk | `effect/sr_lightningwalk.wav` |
| Power Velocity | `effect/sr_powervelocity.wav` |
| Raising Dragon | `effect/sr_raisingdragon.wav` |
| Rampage Blaster | `effect/sr_rampageblaster.wav` |
| Ride in Lightning | `effect/sr_rideinlightning.wav` |
| Sky Net Blow | `effect/sr_skynetblow.wav` |
| Windmill | `effect/sr_windmill.wav` |
| Flash Combo | `effect/sr_flashcombo.wav` |
| Asura Strike (later) | `effect/sr_gateofhell.wav` |

#### Skill Misc

| Effect | Wav File |
|---|---|
| Begin Spell (cast circle) | `effect/ef_beginspell.wav` |
| Teleportation (Fly Wing) | `effect/ef_teleportation.wav` |
| Ready Portal (Open Warp) | `effect/ef_readyportal.wav` |
| Warp Portal active | `effect/ef_portal.wav` |
| Wedding | `effect/wedding.wav` |
| Earthquake | `effect/earth_quake.wav` |
| Coin (Mammonite, Mug) | `effect/ef_coin2.wav` |

### 3.7 Monster Sounds

**Naming convention** (root `data\wav\`):
```
<monster>_attack.wav    // attack swing (sometimes _attack1, _attack2)
<monster>_damage.wav    // taking a hit
<monster>_die.wav       // death (sometimes _die1, _die2)
<monster>_move.wav      // walking (sometimes _move1, _move2)
<monster>_stand.wav     // idle ambient
<monster>_step1.wav     // footstep (large monsters: 2-3 steps)
```

**Body-material shared sounds** — many monsters reuse these instead of unique reaction sounds, saving memory:

| Material | Wav File | Used By |
|---|---|---|
| Insect | `monster_insect.wav` | Fabre, Hornet, Argiope, Mistress, Andre, Thief Bug, Golden Thief Bug |
| Plant | `monster_plant.wav` | Red Mushroom, Red Plant, Mandragora-tier |
| Flesh (soft) | `monster_flesh.wav` | Roda Frog, Spore, Cookie, Maya, Mandragora, Phreeoni, Moonlight, Pharaoh, Orc Lord |
| Clothes | `monster_clothes.wav` | Mummy, Wraith, Evil Druid, Drake, Doppelganger, Jakk |
| Metal | `monster_metal.wav` | Khalitzburg, Bloody Knight |
| Feather | `monster_feather.wav` | Desert Wolf, Savage, Eddga, Matyr, Martin, Kobold |
| Reptile | `monster_reptiles.wav` | Frilldora, Worm Tail |
| Wooden mail | `monster_woodenmail.wav` | Willow |
| Shell | `monster_shell.wav` | Thief Bug Egg, Eggyra, Andre |
| Skeleton | `monster_skelton.wav` (sic, missing E) | Archer Skeleton |
| Zombie | `monster_zombie.wav` | Zombie, Ghoul |
| Poring | `monster_poring.wav` | Mastering, all Poring family |
| Book | `monster_book.wav` | Joker |

**Iconic monster sound sets** (verified via Divine Pride):

**Poring family** (Poring, Poporing, Drops, Marin, Mastering all share):
- `poring_attack.wav` / `poring_damage.wav` / `poring_die.wav` (the iconic "pop") / `poring_move.wav` / `monster_poring.wav`

**Fabre**: `fabre_attack.wav`, `fabre_damage.wav`, `fabre_die.wav`, `monster_insect.wav`

**Pupa**: `pupa_die.wav`, `monster_shell.wav`

**Lunatic**: `lunatic_attack.wav`, `lunatic_die.wav`, `bigfoot_step1/2.wav`, `kocot_foot1/2.wav`

**Roda Frog**: `roda_frog_attack1/2.wav`, `roda_frog_damage.wav`, `roda_frog_die.wav`, `roda_frog_stand.wav`, `monster_flesh.wav`

**Spore**: `spore_attack.wav`, `spore_damage.wav`, `spore_die.wav`, `spore_move/move1/move2.wav`

**Hornet**: `hornet_attack1/2.wav`, `hornet_damage.wav`, `hornet_die.wav`, `hornet_move.wav`, `hornet_stand.wav`

**Wolf / Desert Wolf**: `wolf_attack.wav`, `wolf_damage.wav`, `wolf_die.wav`, `wolf_stand.wav`, `were_wolf_foot1/2.wav`, `monster_feather.wav`

**Rocker (Metaller)**: `metaller_attack.wav`, `metaller_damage.wav`, `metaller_die1/2.wav`, `metaller_step1/2/3.wav`

**Mandragora**: `mandragora_attack.wav`, `mandragora_damage.wav`, `mandragora_die.wav`, `monster_flesh.wav`

**Frilldora**: `frilldora_attack.wav`, `frilldora_damage.wav`, `frilldora_die.wav`, `frilldora_move.wav`, `monster_reptiles.wav`

**Chonchon**: `chocho_attack.wav` (sic), `chocho_damage.wav`, `chocho_die.wav`, `chocho_stand.wav`, `_enemy_hit_fire2.wav`

**PecoPeco**: `kocot_attack.wav`, `kocot_damage.wav`, `kocot_die.wav`, `kocot_foot1/2.wav`, `kocot_stand/stand2.wav`

**Yoyo**: `yoyo_attack.wav`, `yoyo_damage.wav`, `yoyo_die.wav`, `yoyo_fuck.wav` (sic — actual filename, early Korean dev profanity), `yoyo_hop1/2.wav`

**Ghoul/Zombie**: `ghoul_attack1-4.wav`, `ghoul_damage.wav`, `ghoul_die1/2/3.wav`, `ghoul_step1/2.wav`, `monster_zombie.wav`

**Skeleton family**: `monster_skelton.wav` (sic), `skel_archer_attack.wav`, `skel_archer_die.wav`, `skel_damage.wav`

**Mummy**: `mummy_attack.wav`, `mummy_damage.wav`, `mummy_die.wav`, `mummy_move1/2.wav`, `monster_clothes.wav`

**Munak/Eggyra**: `eggyra_die.wav`, `eggyra_move.wav`, `eggyra_stand.wav`, `monster_shell.wav`

**Sohee**: `sohee_attack.wav`, `sohee_stand.wav`

**Anubis**: `anubis_attask1.wav` (sic — "attask"), `anubis_attack2.wav`, `anubis_damage.wav`, `anubis_move.wav`

**Khalitzburg**: `khalitzburg_attack.wav`, `khalitzburg_die.wav`, `khalitzburg_move.wav`, `_attack_dagger.wav`, `monster_metal.wav`

**Doppelganger**: `_swordman_attack.wav`, `doppleganger_damage.wav` (sic), `doppleganger_die.wav`, `monster_clothes.wav`

**Eddga**: `eddga_attack/attack2.wav`, `eddga_die.wav`, `eddga_move.wav`, `monster_feather.wav`

**Maya**: `maya_attack1/2.wav`, `maya_die1/2.wav`, `maya_move.wav`, `monster_flesh.wav`

**Phreeoni**: `phreeoni_attack.wav`, `phreeoni_die.wav`, `phreeoni_move.wav`, `monster_flesh.wav`

**Moonlight Flower**: `moonlight_attack.wav`, `moonlight_die.wav`, `moonlight_move.wav`, `effect/h_moonlight_1/2/3.wav`

**Pharaoh**: `pharaoh_attack.wav`, `pharaoh_die.wav`

**Orc Warrior/Hero/Lord**: `ork_warrior_attack1/2.wav`, `ork_warrior_breath.wav`, `ork_warrior_damage.wav`, `ork_warrior_die1/2.wav`, `ork_warrior_step1/2.wav` + `orc_lord_attack.wav`, `orc_lord_die1/2.wav`, `orc_lord_stand.wav`

**Stormy Knight**: `knight_of_windstorm_attack.wav`, `knight_of_windstorm_damage.wav`, `knight_of_windstorm_die.wav`, `knight_of_windstorm_move1/2.wav`

**Garm**: `garm_attack.wav`, `garm_move1/2.wav`, `garm_stand.wav`

**Lord of Death**: `lord_of_death_attack.wav`, `lord_of_death_die.wav`, `knight_of_abyss_move.wav`

**Dark Lord**: `dark_lord_attack.wav`, `dark_lord_damage.wav`, `dark_lord_die.wav`, `dark_lord_stand.wav`

**Baphomet**: `baphomet_attack.wav`, `baphomet_breath.wav`, `baphomet_damage.wav`, `baphomet_die.wav`

**Baphomet Jr.** (note double underscore): `baphomet__attack.wav`, `baphomet__breath.wav`, `baphomet__damage.wav`, `baphomet__die.wav`

**Drake**: `drake_attack.wav`, `drake_move.wav`, `drake_stand.wav`, `monster_clothes.wav`

**Bloody Knight**: `bloody_knight_die.wav`, `bloody_knight_move1/2.wav`, `bloody_knight_stand.wav`, `monster_metal.wav`

**Evil Druid**: `evil_druid_attack.wav`, `evil_druid_damage.wav`, `evil_druid_die/die2.wav`, `monster_clothes.wav`

**Raydric**: `raydric_attack.wav`, `raydric_damage.wav`, `raydric_die.wav`, `raydric_move.wav`

**Mistress**: `mistress_attack.wav`, `monster_insect.wav` (re-uses Hornet for damage/die/stand)

**Deviruchi**: `deviruchi_attack.wav`, `deviruchi_move.wav`, `deviruchi_stand.wav`

**Dokebi**: `dokebi_attack.wav`, `dokebi_move.wav`

**Wraith / Wanderer**: `wander_man_attack.wav`, `wander_man_die.wav`, `wander_man_move.wav`

### 3.8 Misspellings preserved from the original Korean dev team

When implementing or referencing files, **do not "fix" these** — they're the actual filenames in the GRF:

- `anubis_attask1.wav` (not "attack")
- `monster_skelton.wav` (not "skeleton")
- `elder_wilow_*.wav` (single L)
- `chocho_*.wav` for Chonchon
- `doppleganger_*.wav` (not "doppelganger")
- `assasin_*.wav` in `effect/` (single S)
- `rog_back stap.wav` (not "stab")
- `vallentine.wav` (not "valentine")
- `cru_holy cross.wav` (with literal space)
- `yoyo_fuck.wav` (yes, really)

### 3.9 UI Sounds

The original RO Classic client has remarkably few UI sounds:

| UI Action | Wav File |
|---|---|
| **Universal button click** (login, char create, char select, ESC menu, NPC dialog Next, item buttons) | `버튼소리.wav` ("button sound") |
| Whisper received | `버튼소리.wav` (re-uses click) |
| PvP score change | `effect/number_change.wav` |
| NPC custom sound | Server-emitted via `Sound.play(pkt.fileName)` script command |

**Item drop sounds** (per loot rarity):
- `effect/drop_pink.wav`
- `effect/drop_yellow.wav`
- `effect/drop_purple.wav`
- `effect/drop_blue.wav`
- `effect/drop_green.wav`
- `effect/drop_red.wav`

**The original classic client did NOT have**:
- Distinct hover sounds
- Inventory open/close sound (silent)
- Item pickup/drop/equip sound (silent — only ground-bounce drop_*.wav exists)
- NPC dialog beeps (silent — `버튼소리.wav` plays only on Next click)
- Trade complete chime (silent)
- Storage open/close (silent)
- Refine success/failure outside the anvil-specific sounds

Renewal added a few of these. **For "RO Classic feel" stay sparse — use `버튼소리.wav` for clicks and stay silent for everything else.**

### 3.10 Environmental / Ambient SFX

Stored as 3D positioned sound sources **inside each `.rsw` map file**, NOT in a global table. Each emitter has:
- `Name` (80 bytes)
- `SoundFile` (80 bytes — points to `data\wav\<name>.wav`)
- Position X/Y/Z, Volume, Range, Width/Height
- `CycleInterval` (default 4 sec loop period)

| Environment | Common wav names |
|---|---|
| Warp portal hum | `effect/ef_portal.wav` (active), `effect/ef_readyportal.wav` (opening) |
| Fountain | Per-map `_water.wav` / `_fountain.wav` |
| Forge / anvil | `bs_refinesuccess.wav`, `bs_refinefailed.wav` |
| Forest birds, desert wind, dungeon drips, ocean waves, crowd murmur, fireplace | Per-map embedded |
| Geographer poison gas | `effect/se_gas_pushhh.wav` |

### 3.11 NPC / Voice / Special

Classic RO has **no voiced NPCs**. Dialog is silent text. Some special NPCs use script `soundeffect` commands:

- `wedding.wav` — Prontera wedding
- `vallentine.wav` (sic) — Valentine event
- `wewish.wav` — winter event
- `bellding.wav` — quiz "correct"
- `buzzer.wav` — quiz "wrong"

Kafra NPCs are entirely silent. The famous "NPC dialog beep" is just `버튼소리.wav` on the Next click.

---

## 4. Where the Audio Files Live

### 4.1 Official sources

| Source | URL | Notes |
|---|---|---|
| **kRO official** | `ro.gnjoy.asia` (Gravity Game Hub) | Korean RO, freely downloadable, still operational in 2026 |
| **iRO official** | `renewal.playragnarok.com/downloads/clientdownload.aspx` | International RO via WarpPortal |
| **kRO Full Client mirror (rAthena)** | `rathena.org/board/topic/106413-kro-full-client-2023-04-04-includes-bgm-rsu/` | The community-canonical full client. **Includes BGM folder + RSU patcher.** Latest as of research date. |
| **HostPoring direct downloads** | `hostporing.com/clients/index.php/knowledgebase/22/` | Pre-renewal and renewal direct links |
| **RateMyServer kRO links** | `ratemyserver.net/index.php?page=download_kROLinks` | Mirror index |
| **RO Patcher Lite** | `nn.ai4rei.net/dev/rsu/` | Ai4rei's patcher to keep an existing install current. Latest 4.10.1.1384, 64-bit Windows. |

### 4.2 Streaming releases (the cleanest legal route)

| Album | Where | Cost |
|---|---|---|
| **Ragnarok Online (Original Game Soundtrack)** (2023, 22 tracks) | Apple Music, Spotify, iTunes | $9.99 or free with Apple Music sub |
| **Ragnarok Online BGM, Pt. 1 (Original Soundtrack)** (2026, 68 tracks) | Apple Music | Same pricing — newest official release, broader catalog |

These are released by soundTeMP / Gravity through 5505082 Records DK on streaming platforms. **Listening is fully legal and supports the rights holder.**

### 4.3 Internet Archive holdings (research-grade lossless masters)

| Item | Size | Format | Contents |
|---|---|---|---|
| **Ragnarok Online Music Collection [FLAC]** | **2.4 GB** | FLAC + JPG scans | **The richest single archive.** 5 official CD releases: 2002 Special Remix (24), 2003 OST 2-disc (50), 2006 Memory of Ragnarok (27), 2007 Eien no Story (6 vocals), 2012 BGM Arrange Collection (14). 121 audio files + 60 cover/booklet scans. |
| **Ragnarok Online Complete Soundtrack** | 2.2 GB | Lossless FLAC EAC rip | The 2009 Lantis 5-disc set, 141 tracks, 5:48:58 runtime |
| **Ragnarok BGM** (`121_20191019`) | 567 MB | OGG Vorbis + VBR MP3 | **160 game-rip BGM tracks** (01-160, missing #32). Closest to "the actual BGM folder contents." Has torrent. |
| **High Quality BGM V0** (`121_20220224`) | — | V0 VBR MP3 | High-bitrate remasters of the game-rip set |

### 4.4 KHInsider (parallel BGM mirror)

| Item | Tracks | Size |
|---|---|---|
| Ragnarok Online (Windows) (gamerip) (2002) | **174** | 303 MB MP3 (uploaded 2024-08-02) — **the most direct mirror of an actual `BGM/` folder** |
| Ragnarok Online Original Soundtrack (2003) | 50 | 261 MB MP3 / 754 MB FLAC |
| RAGNAROK Online Complete Soundtrack (2009) | 141 | 748 MB MP3 / 2342 MB FLAC |
| BGM Arrange Collection (2012) | 14 | — |

### 4.5 SFX-only archives

| Archive | Size | Notes |
|---|---|---|
| **Ragnarok Online (KOR) Untouched Sound Files** (Phrozen Keep) | **~110 MB** | Mega.nz hosted, April 2020. Clean drop of `data\wav\` content. |

### 4.6 Ragnarok Offline distribution (the most plug-and-play)

`ragnarokoffline.github.io` ships a **fully documented offline-playable distribution** that splits audio cleanly:
- `LQ_BGM.7z` and `HQ_BGM.7z` (separate, easily swappable)
- `data.7z`, `rdata.7z`, `prerenewal.7z` for the GRFs
- Mirrors on Google Drive, MediaFire, Mega
- Fixes the known mono-version Prontera HQ BGM playback issue

**This is the easiest "everything in one place, including BGM" distribution.**

### 4.7 Private server communities (for context)

| Site | Purpose |
|---|---|
| **rAthena forums** | `rathena.org/board/` — biggest emulator community, hosts kRO full client mirrors and audio threads |
| **Hercules** | `board.herc.ws/` — second emulator, hosts Ridley's *(data) GRF Project* (cleaned visual GRF, audio untouched) |
| **OriginsRO BBS** | `bbs.originsro.org/viewtopic.php?t=2601` — community-uploaded high-quality (320 kbps / V0) BGM packs |
| **Midgard Community** | `midgard-community.com/forums/files/category/17-bgm/` — dedicated BGM downloads category |
| **Divine Pride** | `divine-pride.net` — monster database with HTML5 `<audio>` players exposing each monster's WAV files (right-click save to extract individual sounds) |

---

## 5. GRF Extraction Tools

| Tool | Author | Type | Best For |
|---|---|---|---|
| **GRF Editor** | Tokei (Tokeiburu) | C# / WPF GUI | **De facto standard.** Supports GRF, GPF, Thor. Add/remove/merge/extract/preview/save. Previews wav, txt, lua, png, bmp, tga, gnd, rsw, gat, rsm, str, spr, act. CRC32/MD5 hashing, format validation, version 0x300 support (>4 GB). Open-source. [GitHub](https://github.com/Tokeiburu/GRFEditor) |
| **zextractor** | zhad3 | D / CLI | **Best for scripted extraction.** v1.2.0 March 2024, MIT license. Example: `zextractor --grf=data.grf --filters=data\\wav\\*.wav --out=./sfx`. Supports `--patchMode` for patch-priority overlays. [GitHub](https://github.com/zhad3/zextractor) |
| **grf-loader** | vthibault | TypeScript / JS library | Used by web-based RO viewers (roBrowser ecosystem). Browser + Node. MIT. |
| **grf-extractor** | herenow | Node.js CLI | Simpler than zextractor. MIT. |
| **SecureGRF** | realanan | C++ | Includes encryptor + decrypt library. On Gitee. |

### 5.1 Practical workflow

**Extract all SFX from a kRO client:**

1. **Get a kRO full client** with BGM included from the rAthena 2023-04-04 thread
2. **Install GRF Editor** from rAthena Files
3. **Open `data.grf`** → in the search box type `*.wav` → select all → Extract → flat folder
4. **Total size**: ~100-150 MB

**Or scripted with zextractor:**
```bash
zextractor --grf=data.grf --filters=data\\wav\\*.wav --out=./ro_sfx_extracted
```

**Programmatic via GRF Editor C# library:**
```csharp
var entry = grf.FileTable.TryGet(@"data\wav\poring_die.wav");
var bytes = entry.GetDecompressedData();
File.WriteAllBytes(@"./poring_die.wav", bytes);
```

### 5.2 Decoding Korean filenames

When extracted with the wrong locale, EUC-KR filenames appear as garbled bytes. To fix:
- Use `convmv` on Linux: `convmv -f cp949 -t utf8 -r --notest .`
- Use Python with codec: `bytes.decode('cp949')` or `decode('euc-kr')`
- GRF Editor handles this internally if you set the right locale

### 5.3 Useful post-extraction tools

| Tool | Use |
|---|---|
| **Audacity** | WAV editing, trimming, normalizing |
| **ffmpeg** | Mass conversion (`ffmpeg -i in.wav -ar 44100 -ac 2 out.ogg`), loudness normalization |
| **Foobar2000** | MP3/FLAC management, replay-gain, batch tagging |
| **Sonic Visualiser / Spek / SoX** | Spectrograms — verify lossless provenance (transcoded MP3 reveals brick-wall cutoffs) |
| **mp3val / flac --test** | Integrity verification |

---

## 6. Legal Status — The Hard Truth

### 6.1 Ownership

- **Composer**: SoundTeMP (Korean studio, BlueBlue as primary composer until his 2015 death)
- **Copyright holder**: **GRAVITY CO., LTD.** SoundTeMP composed RO under work-for-hire under Korean copyright law (Article 9, 저작권법). Even SoundTeMP cannot grant you a license — only Gravity can.
- **Active commercial product**: The 2023 streaming release on Apple Music / Spotify is monetized in 2026. The 2026 *Pt. 1* release shows Gravity is actively expanding their official catalog.

### 6.2 The "free client" misconception

Extracting audio from a free-to-download RO client does NOT confer any license to redistribute or use it in derivative works. The client EULA is a one-way grant: you may run the client, you may not extract and reuse its contents.

This is identical to: WoW is free-to-download (trial), but extracting Jeremy Soule's soundtrack and putting it in your indie game is straightforward copyright infringement.

### 6.3 Gravity's enforcement history

The myth of "Gravity doesn't care" is **out of date as of 2022**.

**Gravity Co., Ltd. v. Novaro, LLC** (2:22-cv-02763, C.D. Cal.):
- Filed April 26, 2022 in Central District of California
- **$4,000,000 default judgment entered against NovaRO**
- Settled July 5, 2023 with the judgment on the books and an injunction
- Reportedly structured to trigger automatically if injunction breached

Per reporting from MMOs.com, Massively Overpowered, TheGamer, ResetEra in 2022:
- Gravity confirmed **at least 7 concurrent lawsuits** against private servers in North America alone
- A wave of private servers shut down voluntarily in 2022 after the lawsuits became public
- The complaints reference "copyrighted works" broadly, not just code

**Gravity tolerated rAthena/Hercules for ~16 years before suddenly enforcing.** The pattern is "tolerate, then enforce." Don't bet your work on continued forbearance.

### 6.4 Private server vs. fan game — different risk profiles

| Risk Factor | Private Server | Fan Game / Clone |
|---|---|---|
| Distributes original assets in binary? | No (player supplies own client) | **Yes** |
| Easy DMCA target? | Hard (decentralized) | **Easy** (you own the distribution) |
| Brand confusion risk | Moderate | **High** if RO-styled |
| Clean-room defense? | Yes (eAthena GPL clean-room reimpl) | **No** |

**A fan game shipping RO audio is in a strictly worse legal position than a private server.** Anyone telling you "private servers are fine, so my fan game using RO music is fine" is conflating two very different legal situations.

### 6.5 The "modify slightly" doctrine — direct answer

**Q: Can I download RO audio and modify it (EQ, pitch, edit)?**

**A: No. Here's exactly why:**

Under **17 U.S.C. § 101**, a "derivative work" is "based upon one or more preexisting works" including any work "in which a work may be recast, transformed, or adapted." Under **§ 106(2)**, the **exclusive right to prepare derivative works belongs to the copyright owner**. Modifying a copyrighted track without permission is creating an unauthorized derivative work — infringement under § 501.

**Transformative use test** (Campbell v. Acuff-Rose 1994; **Andy Warhol Foundation v. Goldsmith 2023**, the controlling current Supreme Court precedent): the Warhol case tightened the standard significantly. The use must serve a **fundamentally different purpose** from the original.

Applied to "I changed the EQ on the Theme of Prontera":
- ✗ Same purpose (atmospheric music in a fantasy MMO)
- ✗ Same audience (fantasy MMO players)
- ✗ Same market substitution (your version replaces licensing the original)
- ✗ No commentary, parody, critique
- ✗ No new expression, meaning, or message

This fails on every prong. From the Wolters Kluwer Copyright Blog: *"Merely imprinting one's own style on another's work without comment or critique is derivation, not transformation."*

**Even if you have copyright in your *additional contributions* (EQ choices, layered sounds), you do NOT get copyright in the underlying work.** Your derivative work embeds Gravity's original. This is identical to cover song mechanical royalties — you have a performance copyright in your recording, but you must still pay the original composer. There is no equivalent statutory mechanical license for game music remixes.

**The only legitimate paths to music you can use**:
1. Create from scratch (you, or commissioned)
2. License directly from rights holder (Gravity is not in the business of doing this for fan games)
3. Use openly licensed sources (CC0, CC-BY, public domain, or paid royalty-free libraries with explicit commercial-use rights)
4. AI generation (with caveats — Section 7.4)

**There is no fifth path called "modify it a little." That path leads to a takedown notice or a lawsuit.**

### 6.6 Fair use reality check

The four-factor § 107 test:
1. **Purpose** — Transformative? No. Same purpose as original.
2. **Nature** — Music is highly creative. ✗
3. **Amount** — A fan game uses entire tracks. ✗
4. **Market effect** — RO OST is actively sold; free fan game with same music substitutes. ✗

**Fan games almost universally lose fair use defenses on these factors.** From Odin Law: *"Fair use is a post-litigation defense, not preemptive permission, and fan creations based on a previously existing work are considered derivative. Unless explicitly licensed, they are technically unauthorized — even if they are created out of love and shared freely."*

---

## 7. Royalty-Free Alternatives (the actual path forward)

### 7.1 Free, CC0/permissive — the indie default

**Music (BGM):**

| Source | License | Suitability for RO-style | URL |
|---|---|---|---|
| **OpenGameArt.org** | CC0, CC-BY, GPL | Excellent — fantasy/RPG/MMO category with hundreds of orchestral and chiptune tracks | opengameart.org |
| **Kevin MacLeod (Incompetech)** | CC-BY 4.0 (free with credit) or paid no-attribution license ($30-50+) | Very good — 2,000+ tracks. Look at: `Fantastic Dim Bar`, `Ossuary 6 - Air`, `Folk Round`, `Wallpaper`, anything in the World/Medieval set | incompetech.com |
| **FreePD** | CC0 / public domain | Mixed quality, hidden gems for atmospheric/orchestral | freepd.com |
| **Pixabay Music** | Pixabay Content License (free, commercial OK, no attribution) | Decent. Watch out: many tracks in YouTube Content ID — fine in-game, can flag YouTube demos | pixabay.com/music |
| **Free Music Archive** | Mixed CC | Variable; check each track | freemusicarchive.org |
| **Musopen** | Public domain classical | For regal/dramatic feel from real classical pieces | musopen.org |

**Sound Effects:**

| Source | License | Notes | URL |
|---|---|---|---|
| **Sonniss GDC Bundle 2026** | Royalty-free, no attribution, lifetime, unlimited | **Top recommendation.** 7.47 GB / 347 WAV in 2026 alone, plus ~200 GB accumulated past years. AAA-quality. **Critical**: AI/ML training is prohibited under license. | gdc.sonniss.com |
| **Kenney Audio Packs** | CC0 (public domain) | Indie-dev legend. UI, RPG SFX, sci-fi. **No credit required** even commercially. | kenney.nl/assets/category:Audio |
| **Freesound.org** | Mixed: CC0, CC-BY, CC-BY-NC | **Use the "Approved for Free Cultural Works" filter** (CC0 + Attribution only). Avoid CC-BY-NC. | freesound.org |
| **Zapsplat** | Free with attribution (free tier) | Solid library | zapsplat.com |
| **GameSounds.xyz** | Mixed curated | Curated index of game-friendly free audio | gamesounds.xyz |
| **99Sounds** | Free, mixed | Producer-curated SFX packs | 99sounds.org |

**⚠️ AVOID for commercial/distributed projects:**
- **BBC Sound Effects** free archive — RemArc License is **personal/educational only**, NOT commercial
- **Anything CC-BY-NC** — non-commercial only
- **Anything CC-BY-ND** — no derivatives allowed (you can't loop or process)

### 7.2 Paid, professional (Tier 2)

| Service | Cost (2026) | Best For |
|---|---|---|
| **Epidemic Sound Pro** | $204/year (annual) or $39.99/month | One subscription covers BGM + SFX, commercial games. Pro covers commercial game distribution; Enterprise needed for cinema/TV/major-publisher use. |
| **Artlist** | $15-25/month | Music, traditional production-recorded library, no AI tools |
| **Soundstripe** | $15-20/month | Music + SFX, broad coverage |
| **Soundly** | $10-30/month | Massive SFX cloud library — closest to "Boom but cheaper" |
| **Boom Library** | $99-199/pack, ~$500/year subscription, ~$3k full buyout | AAA SFX cinematic |
| **Pro Sound Effects (BBC)** | $5 single / $199 per pack / $1,999 full BBC library | One-time purchases of legendary BBC archive with commercial license |
| **Ninichi Music** | $30-100/pack | RPG/fantasy/MMO music packs that fit RO Classic vibe well |

### 7.3 Commission original (Tier 3)

| Tier | Source | Price/track | What you get |
|---|---|---|---|
| **Hobbyist** | Fiverr lowest | $10-50 | Often unusable. Avoid the $10 tier. |
| **Indie (recommended for fan project)** | Fiverr mid; itch.io composers; **Bandcamp DMs** | **$50-200** | 1-2 minute looped track from a competent hobbyist. Many indie composers happily work in this range. |
| **Professional indie** | SoundBetter, dedicated game audio freelancers | $200-1,000 | High-quality, multiple revisions, broadcast-ready masters. Mid-level €100/hr typical. |
| **AAA tier** | Established game composers | $1,000-10,000+ | Original orchestrated score |

**Where to find "RO-style fantasy MMO" composers:**
- **Bandcamp** — search tags: `chiptune`, `JRPG`, `fantasy`, `MMO`, `orchestral`. Many indie composers post original work and accept commissions through Bandcamp contact forms.
- **itch.io** — filter assets by `royalty-free music`. Many composers sell prepackaged fantasy packs ($5-50) and accept commissions.
- **Ninichi Music** — sells curated game music packs in fantasy/RPG style with commercial licensing
- **r/gameDevClassifieds, r/INAT** — active composer/dev matchmaking
- **VGM Twitter / Bluesky** — many video game composers self-promote and take small commissions

**Critical contract language for any commission:**
- Specify "work for hire" OR explicit assignment of all rights to you
- Specify commercial use, derivative use, sync rights
- Get the agreement in writing even for $50 commissions

### 7.4 AI generation (Tier ?) — legally unsettled in 2026

**Active lawsuits in progress:**
- **RIAA v. Suno** (D. Mass.) and **RIAA v. Udio** (S.D.N.Y.) — filed June 2024 by Universal, Warner, Sony Music. Allege training on copyrighted recordings. Statutory damages up to $150,000 per work, potential billions in exposure.
- **Warner Music Group** settled with both Suno and Udio in November 2025.
- **Universal Music Group** settled with Udio in October 2025.
- **Sony has not settled.**

**Copyright ownership of AI output:**
- The U.S. Copyright Office (Jan 2025; reaffirmed throughout 2025) holds that **prompt-only AI output is not copyrightable by the user**.
- Suno's December 2025 terms update went further — **"Suno itself is ultimately responsible for the output"** and users "generally are not considered the owner of the songs."

**Quality (2026 review):**
| Tool | Strength |
|---|---|
| **Suno v5** | Best for vocal songs, overall fidelity |
| **Udio** | Best for high-fi instrumental, 48 kHz output |
| **Stable Audio** | **Best for game integration**, loops, ambient texture, sound design |
| **AIVA** | **Best for orchestral / cinematic** — exports MIDI, closest to RO-style |
| Meta MusicGen / Riffusion | Open-source, lower quality |

**ElevenLabs Sound Effects** — paid plans starting $5/mo include commercial license. Cleaner legal situation than music AI because the SFX market is less litigated.

**Honest practical recommendation for AI in 2026:**
- ✓ OK for: rapid prototyping, scratch/temp audio, inspiration, ambient texture, one-off SFX where ownership doesn't matter
- ✗ **Do NOT use as final shipped score** for any project you care about long-term. Legal foundation could collapse, and you have no copyright in the output.
- **AIVA exports MIDI** — take MIDI into a DAW, replace instruments with your own samples, edit substantially → much stronger ownership claim than raw Suno output.

### 7.5 Specific recommendations for an RO-style clone

**Combat hit sounds**: Sonniss GDC bundles (multiple `CB Sounddesign`, `344 Audio` packs include weapon impacts) + Kenney's RPG Audio Pack (CC0)

**Weapon swings**: Sonniss `344 Audio Whoosh & Swish` packs (free, AAA), or Boom Library Bladestorm ($199 paid)

**Monster sounds**: Sonniss `CB Sounddesign Creatures` (free) + AI generation (ElevenLabs SFX or Stable Audio — legal/ownership concerns lower for one-off creature vocals than for music)

**UI / Menu**: Kenney UI Audio Pack (CC0) — perfect indie default + Material Design SFX (Apache)

**BGM "RO Classic feel"**:
- Kevin MacLeod fantasy tag (CC-BY) — 70% of the way for free
- AIVA for AI-generated orchestral starts you then edit in a DAW
- PlayOnLoop subscription ($10-20/mo) — fantasy game music
- IndieGameMusic.com — closest community to "MMO music for indies"
- **Ninichi Music packs** ($30-100) — coherent fantasy MMO sets

**The best result**: buy a baseline Ninichi/itch.io pack to fill the world ($30-100), then commission 5-10 hero tracks for your most-played zones (starting town, capital, signature dungeon, boss music) at $50-200 each. Total ~$300-1,000 for music that's clearly yours.

---

## 8. Recommended Path for Sabri_MMO

### 8.1 Tiered roadmap

**Tier 1 — Free, immediate (Phase 1: BGM + Combat SFX)**

Total cost: **$0**

- Sonniss GDC 2026 bundle + accumulated GDC archive (~200 GB) → primary SFX library
- Kenney audio packs (CC0) → UI, RPG SFX, additional combat
- OpenGameArt (CC0/CC-BY filter) → fill gaps in monster sounds, ambient
- Freesound.org (with "Approved for Free Cultural Works" filter) → specific one-off needs
- Kevin MacLeod (CC-BY 4.0, with credits) → 5-15 tracks for initial zones
- OpenGameArt fantasy music → additional BGM coverage
- Pixabay Music → fill remaining BGM

**Required**: Maintain a `LICENSES.md` in the repo listing every audio file, source, license, attribution required. **Non-negotiable for legal hygiene.**

**Tier 2 — Paid, professional (Phase 2-3)**

Total cost: **~$250-600/year**

- Epidemic Sound Pro ($204/year) — covers BGM + SFX with one subscription, eliminates attribution requirements
- OR Soundly ($120/year) for SFX + Pixabay/OpenGameArt/Kevin MacLeod for music
- + Sonniss free bundles as supplementary

**Tier 3 — Premium hero content (Phase 4+)**

Total cost: **~$500-3,000 one-time**

- Commission 5-10 original tracks from a Bandcamp/itch.io composer at $50-200/track for the most important zones
- Buy 1 cohesive fantasy music pack from Ninichi or PlayOnLoop ($50-150) for filler
- Use Tier 1 free sources for all SFX

**Tier 4 — AVOID**
- Direct use of Ragnarok Online audio (modified or unmodified)
- Heavy reliance on Suno/Udio raw output for shipped final audio
- BBC Sound Effects free archive in commercial/distributed game
- Any source under CC-BY-NC

### 8.2 What to extract for *research only* (legitimate)

You can legitimately extract RO audio **as a target reference** for your own original work:

1. Get a kRO full client from rAthena's 2023 mirror
2. Extract `data\wav\` with GRF Editor or zextractor
3. Extract `BGM/` (loose folder, no GRF needed)
4. Use the files as reference targets — measure their:
   - Length
   - Loudness (LUFS)
   - Frequency content
   - Emotional shape
5. Brief composers / AI prompts to hit those targets without using the source material directly

**This is legitimate research use.** Storing extracted files locally for analysis is fine. **Do not commit them to the repo. Do not ship them in any build.**

### 8.3 Implementation lookup tables

Build three lookup tables in `server/src/`:

1. **`ro_weapon_sounds.js`** — port `WeaponHitSoundTable` (weapon type → hit sound) and `JobHitSoundTable` (job → body material)
2. **`ro_skill_sounds.js`** — extract wav field from roBrowserLegacy `EffectTable.js` and combine with `SkillEffect.js` to map skill IDs to wav names. **For Sabri_MMO**: store the wav reference as a string, then load your own original asset under that name.
3. **`ro_monster_sounds.js`** — for each entry in `ro_monster_templates`, add `attackSound`, `damageSound`, `dieSound`, `moveSound` fields. Sound names sourced from Divine Pride monster pages.

Then on the client side:
- `EnemySubsystem` plays `<monster>_attack.wav` on `enemy:attack`, `<monster>_damage.wav` on damage, `<monster>_die.wav` on death
- `CombatActionSubsystem` plays `_hit_<weapontype>.wav` on hit and `player_<material>.wav` for target body reaction
- Status effects play `_<status>.wav` set when added
- Skills play their VFX `wav` field via `SkillVFXSubsystem` (add `wav` to `FSkillVFXConfig` alongside Niagara system)

**Critical**: the file *names* (e.g. `_hit_sword.wav`, `poring_die.wav`) are facts about how RO worked — they aren't copyrighted. You can use the same naming scheme in your code and load your own original audio under those names. This gives you a familiar developer experience that mirrors RO's architecture without using Gravity's actual files.

### 8.4 Skill update needed

The existing `/sabrimmo-audio` skill at `C:\Users\pladr\.claude\skills\sabrimmo-audio\` has one factual error to correct:
- **Remove all references to Tobias Marberger** as a RO composer — no documented connection exists in any verified source
- All RO composer credits go to **SoundTeMP collectively**, with **BlueBlue (Lee Seock-Jin)** as primary composer

---

## 9. Key Sources

### Technical / file format
- [GRF format spec — Ragnarok Research Lab](https://ragnarokresearchlab.github.io/file-formats/grf/)
- [RSW format spec — Ragnarok Research Lab](https://ragnarokresearchlab.github.io/file-formats/rsw/)
- [GRF Editor — Tokeiburu / GitHub](https://github.com/Tokeiburu/GRFEditor)
- [zextractor — zhad3 / GitHub](https://github.com/zhad3/zextractor)
- [BGM Files — iRO Wiki](https://irowiki.org/wiki/BGM_Files)
- [Sound Effects — iRO Wiki](https://irowiki.org/wiki/Sound_Effects)
- [mp3nametable.txt — zackdreaver/ROenglishRE](https://github.com/zackdreaver/ROenglishRE/blob/master/Ragnarok/data/mp3nametable.txt)
- [Hercules Wiki — Data Folder Structure](https://github.com/HerculesWS/Hercules/wiki/Data-Folder-Structure)

### roBrowser source code (the canonical sound mapping)
- [roBrowserLegacy — EffectTable.js (436 wav references)](https://raw.githubusercontent.com/MrAntares/roBrowserLegacy/master/src/DB/Effects/EffectTable.js)
- [roBrowserLegacy — WeaponHitSoundTable.js](https://raw.githubusercontent.com/MrAntares/roBrowserLegacy/master/src/DB/Items/WeaponHitSoundTable.js)
- [roBrowserLegacy — JobHitSoundTable.js](https://raw.githubusercontent.com/MrAntares/roBrowserLegacy/master/src/DB/Jobs/JobHitSoundTable.js)
- [roBrowserLegacy — EntityState.js (status sounds)](https://raw.githubusercontent.com/MrAntares/roBrowserLegacy/master/src/Renderer/Entity/EntityState.js)

### Asset sources
- [kRO Full Client 2023-04-04 (with BGM) — rAthena](https://rathena.org/board/topic/106413-kro-full-client-2023-04-04-includes-bgm-rsu/)
- [Ragnarok Offline Pre-Renewal Pack](https://ragnarokoffline.github.io/)
- [Ragnarok Online Music Collection [FLAC] — archive.org (2.4 GB)](https://archive.org/details/ragnarok-online-music-collection)
- [Ragnarok BGM 160-track gamerip — archive.org](https://archive.org/details/121_20191019)
- [Ragnarok Online (Windows) gamerip 2002, 174 tracks — KHInsider](https://downloads.khinsider.com/game-soundtracks/album/ragnarok-online-windows-gamerip-2002)
- [Untouched Sound Files — Phrozen Keep (110 MB SFX-only)](https://d2mods.info/forum/viewtopic.php?t=66802)
- [Divine Pride monster database](https://www.divine-pride.net/)

### Official streaming
- [Ragnarok Online (Original Game Soundtrack) 2023, 22 tracks — Apple Music](https://music.apple.com/us/album/ragnarok-online-original-game-soundtrack/1698774555)
- [soundTeMP artist page (lists 2023 OST and 2026 BGM Pt. 1) — Apple Music](https://music.apple.com/us/artist/soundtemp/1698706065)
- [VGMdb — Ragnarok Online OST 2003 (DigiCube SSCX-10090~1)](https://vgmdb.net/album/44)

### Composer
- [SoundTeMP — Wikipedia](https://en.wikipedia.org/wiki/SoundTeMP)
- [SoundTeMP — Ragnarok Wiki](https://ragnarok.fandom.com/wiki/SoundTeMP)

### Legal — Gravity / NovaRO
- [Gravity Co., Ltd. v. Novaro, LLC — Justia docket](https://dockets.justia.com/docket/california/cacdce/2:2022cv02763/850478)
- [Ragnarok Online Devs Crackdown — TheGamer](https://www.thegamer.com/ragnarok-online-private-server-lawsuit-gravity/)
- [Gravity Crackdown — Massively Overpowered](https://massivelyop.com/2022/05/06/gravity-is-apparently-cracking-down-on-ragnaroks-rogue-server-community/)

### Legal — fan games and derivative works
- [Fan Games and Legal Risks — Odin Law](https://odinlaw.com/blog-fan-games-legal-risks/)
- [Derivative work — Wikipedia](https://en.wikipedia.org/wiki/Derivative_work)
- [Transformative Fair Uses vs Infringing Derivative Works — Wolters Kluwer](https://legalblogs.wolterskluwer.com/copyright-blog/how-to-distinguish-transformative-fair-uses-from-infringing-derivative-works/)

### Free SFX / music libraries
- [Sonniss GDC 2026 Bundle](https://gdc.sonniss.com/)
- [Kenney Audio Packs (CC0)](https://kenney.nl/assets/category:Audio)
- [OpenGameArt — CC0 Fantasy Music & Sounds](https://opengameart.org/content/cc0-fantasy-music-sounds)
- [Incompetech (Kevin MacLeod) — Music FAQ](https://incompetech.com/music/royalty-free/faq.html)
- [Freesound — FAQ](https://freesound.org/help/faq/)
- [Pixabay Content License](https://pixabay.com/service/license-summary/)

### Paid licensing
- [Epidemic Sound — Game Development pricing](https://www.epidemicsound.com/game-development/)
- [Boom Library](https://www.boomlibrary.com/)
- [Soundly](https://getsoundly.com/)

### AI music legal status
- [RIAA v. Suno / Udio lawsuits — RIAA press release](https://www.riaa.com/record-companies-bring-landmark-cases-for-responsible-ai-againstsuno-and-udio-in-boston-and-new-york-federal-courts-respectively/)
- [U.S. Copyright Office AI Policy Guidance](https://www.copyright.gov/ai/ai_policy_guidance.pdf)

---

## 10. Final Recommendation

**For Sabri_MMO specifically:**

1. **Immediately download a kRO client + extract BGM and `data\wav\` for research** (local, never committed). Use it to measure target loudness, length, frequency shape, emotional tone.
2. **Stream the official 2023 + 2026 RO OST releases on Apple Music / Spotify** while developing — legal, supports rights holder, gives you the canonical compositional language.
3. **Build the implementation lookup tables now** (`ro_weapon_sounds.js`, `ro_skill_sounds.js`, `ro_monster_sounds.js`) using the names and structures documented here. The architecture is fact, not copyright.
4. **Start with Tier 1 free sources** (Sonniss + Kenney + OpenGameArt + Kevin MacLeod). Drop them into the lookup table file paths. The game will sound 70% there.
5. **Maintain `LICENSES.md` from day one** listing every audio file, source URL, license, attribution required.
6. **As the project matures**, commission 5-10 hero tracks from a Bandcamp composer ($50-200 each, ~$500-2,000 total) for the most-played zones.
7. **Never ship Gravity-owned audio** in any build of Sabri_MMO. The 8 hours you'd "save" by extracting RO audio is not worth the existential risk to the project.
8. **Update the `/sabrimmo-audio` skill** to remove the Tobias Marberger misattribution.

The path from research → free placeholder → original commissioned hero content is well-trodden by indie MMO clones. It works. The legal landscape in 2026 makes the alternative untenable.
