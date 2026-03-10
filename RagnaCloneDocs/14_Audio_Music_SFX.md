# 14 -- Audio, Music, and Sound Effects System

> Complete reference for Ragnarok Online Classic (pre-Renewal) audio design and UE5 implementation plan for Sabri_MMO.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Background Music (BGM)](#2-background-music-bgm)
3. [Sound Effects (SFX)](#3-sound-effects-sfx)
4. [Voice and Character Sounds](#4-voice-and-character-sounds)
5. [3D Audio in UE5](#5-3d-audio-in-ue5)
6. [UE5 Implementation Architecture](#6-ue5-implementation-architecture)
7. [Asset Creation and Sourcing](#7-asset-creation-and-sourcing)
8. [Audio Data Structures](#8-audio-data-structures)
9. [Phase Plan](#9-phase-plan)

---

## 1. Overview

### RO Classic Audio Architecture

Ragnarok Online Classic uses a straightforward audio system with three distinct layers:

| Layer | Technology | Format | Control |
|-------|-----------|--------|---------|
| Background Music (BGM) | MP3 playback | `.mp3` files in `/BGM/` folder | `/bgm` toggle, volume slider |
| Sound Effects (SFX) | Miles Sound System (RAD Game Tools) | `.wav` files in `data/wav/` | `/sound` toggle, volume slider |
| Voice/Character Audio | Miles Sound System | `.wav` files in `data/wav/` | Part of SFX volume |

### Composers

All original RO music was composed by **SoundTeMP** (Sound Team Production), a South Korean composer group formed in 1992. Key members include multiple arrangers who contributed to the 5-disc, 141-track "Ragnarok Online Complete Soundtrack" (Lantis, 2009, catalog LACA-9163~7). Additional composers credited: Neocyon, Warren, and Workspace.

Musical style ranges across orchestral, MIDI, trance/techno, electronic, ambient, new age, jazz, Latin, and folk -- SoundTeMP is acclaimed for genre diversity within a single game soundtrack.

### RO Client Audio Configuration

RO Classic provides these player-facing audio controls:

- **`/bgm`** command -- toggles background music on/off
- **`/sound`** command -- toggles sound effects on/off
- **Setup > Sound Configuration** menu -- separate volume sliders for:
  - BGM volume
  - Sound effect volume
- **`Alt+Y`** -- opens command list with `/bgm` and `/sound` shortcuts
- BGM and SFX are independently controllable

### File Organization in Original RO Client

```
RagnarokOnline/
  BGM/                          # Background music
    01.mp3 through 160.mp3      # Numbered BGM tracks
    997.mp3                     # Silence (special)
  data.grf (archive containing):
    data/wav/                   # Sound effects root
      effect/                   # Skill and combat effect sounds
      monster/                  # Monster-specific sounds (idle, attack, damage, death)
      _hit*.wav                 # Hit impact sounds
      _attack*.wav              # Attack swing sounds
    data/mp3nametable.txt       # Map-to-BGM assignment table
```

The `mp3nametable.txt` file maps every map RSW file to its BGM track number using the format:
```
prontera.rsw#bgm\\08.mp3#
geffen.rsw#bgm\\13.mp3#
payon.rsw#bgm\\14.mp3#
```

---

## 2. Background Music (BGM)

### 2.1 Complete BGM Track List (01-160 + 997)

All tracks are stored as numbered MP3 files in the `/BGM/` folder. The complete listing:

| # | Track Name | Primary Use |
|---|-----------|-------------|
| 01 | Title | Login screen |
| 02 | Gambler of Highway | Fields (Prontera south) |
| 03 | Peaceful Forest | Fields (light forest areas) |
| 04 | I Miss You | Fields (Prontera/Geffen fields) |
| 05 | Tread on the Ground | Fields |
| 06 | Risk Your Life | Combat / danger areas |
| 07 | Wind of Tragedy | Tragic/somber locations |
| 08 | **Theme of Prontera** | Prontera city + interiors |
| 09 | Great Honor | Special / ceremonial |
| 10 | Divine Grace | Church / holy areas |
| 11 | **Theme of Morroc** | Morroc city interiors (pre-destruction) |
| 12 | Streamside | Fields (prt_fild08, waterside) |
| 13 | **Theme of Geffen** | Geffen city + interiors |
| 14 | **Theme of Payon** | Payon city + interiors |
| 15 | **Theme of Alberta** | Alberta city + interiors |
| 16 | Labyrinth | Maze / labyrinth dungeons |
| 17 | Treasure Hunter | Treasure-themed dungeons |
| 18 | Time Up!! | Timed events / urgency |
| 19 | Under the Ground | Prontera Culvert dungeon |
| 20 | Ancient Groover | Payon Caves / dungeon |
| 21 | Through the Tower | Geffen Tower dungeon |
| 22 | Backattack!! | Pyramid dungeon (Morroc) |
| 23 | Travel | Fields (gef_fild01, travel routes) |
| 24 | Desert | Morroc fields / desert areas |
| 25 | Plateau | Fields (gef_fild07, mountain plateau) |
| 26 | Everlasting Wanderers | Izlude city |
| 27 | Dreamer's Dream | Mjolnir dungeon |
| 28 | You're in Ruins | Ruin areas |
| 29 | Be Nice 'n Easy | Relaxed / casual areas |
| 30 | One Step Closer | Training Grounds |
| 31 | Brassy Road | Mjolnir fields |
| 32 | *(not assigned in standard client)* | -- |
| 33 | Yuna Song | Mjolnir fields (mjolnir_01) |
| 34 | Pampas Upas | Mt. Mjolnir areas |
| 35 | TeMPoison | Poison-themed dungeons |
| 36 | Nano East | Payon fields (pay_fild07) |
| 37 | TeMPorsche | Morroc fields (moc_fild18) |
| 38 | Hamatan | Sphinx dungeon |
| 39 | **Theme of Al De Baran** | Al De Baran city + interiors |
| 40 | Fear... | Glast Heim areas |
| 41 | Rag All Night Long | Party / upbeat areas |
| 42 | Curse'n Pain | Dark dungeons |
| 43 | Morning Gloomy | Geffen Tower upper level |
| 44 | TeMP it Up | Geffenia |
| 45 | Don't Piss Me Off | Aggressive dungeon areas |
| 46 | An Ant-lion's Pit | Ant Hell dungeon |
| 47 | Welcome Mr. Hwang | Eastern-themed areas |
| 48 | Help Yourself | Mid-level dungeons |
| 49 | Watery Grave | Byalan / underwater dungeon |
| 50 | Out of Curiosity | Geffen dungeon variants |
| 51 | Believe in Myself | Uplifting / quest areas |
| 52 | Ready~ | PvP / arena preparation |
| 53 | White Christmas | Christmas event |
| 54 | Come on My Deer!! | Christmas event |
| 55 | Welcome, My Lord | Christmas event (Castle) |
| 56 | Silver Bell | Christmas event |
| 57 | Don't Cry, Baby | Christmas event |
| 58 | Jingle Bell on Ragnarok | Christmas event |
| 59 | Theme of Lutie / Snow in My Heart | **Lutie (Xmas town)** |
| 60 | Aeon | Clock Tower dungeon |
| 61 | Zingaro | Clock Tower Basement |
| 62 | High Roller Coaster | **Comodo city** |
| 63 | Mucho Gusto | Comodo beach / dungeon |
| 64 | One Fine Day | Relaxed areas |
| 65 | Into the Abyss | Deep dungeon areas |
| 66 | Wanna Be Free!! | Mid-level areas |
| 67 | TeMPotato | Casual / town areas |
| 68 | Jazzy Funky Sweety | **Umbala city** |
| 69 | Retro Metro | Retro-themed areas |
| 70 | **Theme of Juno** | Juno (Yuno) city |
| 71 | Antique Cowboy | Western-themed areas |
| 72 | Big Guys Love This | Heavy / grand areas |
| 73 | Higher Than the Sun | Elevated / sky areas |
| 74 | Not So Far Away | Distant areas |
| 75 | Come, and Get It! | Combat-oriented areas |
| 76 | Purity of Your Smile | **Amatsu city** |
| 77 | Can't Go Home Again, Baby | Wandering / field areas |
| 78 | Adios | Farewell-themed areas |
| 79 | The Great | **Louyang city** |
| 80 | Jumping Dragon | Louyang dungeon |
| 81 | Thai Orchid | Ayothaya areas |
| 82 | Muay Thai King | Ayothaya dungeon |
| 83 | Sleepless | Night / dark areas |
| 84 | Christmas in the 13th Month | **Niflheim city** |
| 85 | Dancing Christmas in the 13th Month | Niflheim areas |
| 86 | Steel Me | Industrial / forge areas |
| 87 | Ethnica | Cultural / ethnic areas |
| 88 | Come in Peace | Peaceful areas |
| 89 | We Have Lee But You Don't Have | Korean-themed areas |
| 90 | Noblesse Oblige | Noble / aristocratic areas |
| 91 | CheongChoon | Korean-themed areas |
| 92 | Naive Rave | Lighthalzen dungeon (lhz_dun04) |
| 93 | Latinnova | **Hugel city** |
| 94 | **Theme of Rachel** | Rachel city |
| 95 | Underneath the Temple | Rachel temple dungeon |
| 96 | Yetit Petit | Ice cave / cold areas |
| 97 | Lastman Dancing | Einbroch areas |
| 98 | Top Hoppy | Upbeat areas |
| 99 | Uncanny Lake | Lake / water areas |
| 100 | Abyss | Abyss Lake dungeon |
| 101 | AbSolitude | Solitary / desolate areas |
| 102 | Erebos' Prelude | Dark / underworld areas |
| 103 | Invisible Invasion | Stealth / hidden areas |
| 104 | On Your Way Back | **Veins city** |
| 105 | Rose of Sharon | Korean cultural areas |
| 106 | Sleeping Volcano | Volcanic areas |
| 107 | Seven Days Seven Nights | Extended dungeon areas |
| 108 | Angelica | Angelic / holy areas |
| 109 | Alpen Rose | Alpine / mountain areas |
| 110 | Kingdom Memories | Royal / castle areas |
| 111 | Good Morning | Morning / dawn areas |
| 112 | Good Night | Night / dusk areas |
| 113 | Monastery in Disguise | Monastery areas |
| 114 | **Theme of Moscovia** | Moscovia city |
| 115 | Tale of the East | Eastern folklore areas |
| 116 | Away from Home | Travel / distant areas |
| 117 | Dream of a Whale | Ocean / coastal areas |
| 118 | Taiko's Fury | Japanese drumming / combat |
| 119 | Stained Memories | **Morroc post-destruction** |
| 120 | Fissure Eruption | Volcanic / eruption areas |
| 121 | Outer Breath | Atmospheric / ambient |
| 122 | Ethical Aspiration | Contemplative areas |
| 123 | Into the Arena! | PvP / arena battles |
| 124 | Stranger Aeons | Otherworldly areas |
| 125 | Forbidden Anguish | Forbidden / dark areas |
| 126 | New World Order | New world continent |
| 127 | Mystic Haze | Mystical / ethereal areas |
| 128 | Splendid Dreams | Dream-like areas |
| 129 | Fireflies Heaven | Forest / nature areas |
| 130 | Dread and Bold | Dangerous areas |
| 131 | Daytime in Manuk | Manuk city |
| 132 | March with Irish Whistle | Celtic / folk areas |
| 133 | Sunny Side of Life | Bright / cheerful areas |
| 134 | Borborema | Tropical / jungle areas |
| 135 | At Dusk | Twilight areas |
| 136 | Emotion Creep | Emotional / dramatic areas |
| 137 | Dazzling Snow | Snow / ice areas |
| 138 | TeMPlatonic | Mixed-theme areas |
| 139 | Sugar Cane Carnival | Festival / carnival areas |
| 140 | Twilight Heaven | Evening sky areas |
| 141 | Tricky Cheeky | Playful / mischief areas |
| 142 | Arrival | Arrival / landing areas |
| 143 | Mother Earth | Nature / earth areas |
| 144 | Silent Voyage | Ocean voyage areas |
| 145 | Marshmallow Waltz | Whimsical areas |
| 146 | Diamond Dust | Crystal / ice areas |
| 147 | Octopus Scramble | Underwater areas |
| 148 | Melt Down! | Forge / industrial areas |
| 149 | **Theme of Port Malaya** | Port Malaya city |
| 150 | Uncanny Dreams | Surreal areas |
| 151 | Maximum | High-intensity areas |
| 152 | Voices | Ambient / atmospheric |
| 153 | Horizon | Distant / open areas |
| 154 | Eclage | Eclage (fairy city) |
| 155 | Eclage | Eclage variant |
| 156 | Beyond the Grave | Undead / graveyard areas |
| 157 | Disillusion | Dark / despair areas |
| 158 | Indelible Scars | Battle-scarred areas |
| 159 | Judgement | Boss / final areas |
| 160 | Jittering Nightmare | Horror / nightmare areas |
| 997 | Silence | Silent maps (no BGM) |

### 2.2 Zone-to-BGM Assignment Map (Pre-Renewal)

The `mp3nametable.txt` file defines which BGM plays on each map. Here are all documented assignments from the RO Classic client:

#### Towns (Cities)

| Map Code | Location | BGM # | Track Name |
|----------|----------|-------|------------|
| `prontera` | Prontera Capital | 08 | Theme of Prontera |
| `prt_in` | Prontera Interiors | 08 | Theme of Prontera |
| `izlude` | Izlude Satellite Town | 26 | Everlasting Wanderers |
| `izlude_in` | Izlude Interiors | 26 | Everlasting Wanderers |
| `geffen` | Geffen Magic City | 13 | Theme of Geffen |
| `geffen_in` | Geffen Interiors | 13 | Theme of Geffen |
| `payon` | Payon Archer Town | 14 | Theme of Payon |
| `payon_in01-03` | Payon Interiors | 14 | Theme of Payon |
| `morocc` | Morroc Desert Town | 119 | Stained Memories (post-destruction) |
| `morocc_in` | Morroc Interiors | 11 | Theme of Morroc (original) |
| `alberta` | Alberta Port Town | 15 | Theme of Alberta |
| `alberta_in` | Alberta Interiors | 15 | Theme of Alberta |
| `aldebaran` | Al De Baran | 39 | Theme of Al De Baran |
| `aldeba_in` | Al De Baran Interiors | 39 | Theme of Al De Baran |
| `comodo` | Comodo Beach Town | 62 | High Roller Coaster |
| `umbala` | Umbala Jungle Town | 68 | Jazzy Funky Sweety |
| `yuno` | Juno (Yuno) | 70 | Theme of Juno |
| `niflheim` | Niflheim (Land of Dead) | 84 | Christmas in the 13th Month |
| `amatsu` | Amatsu (Japanese) | 76 | Purity of Your Smile |
| `louyang` | Louyang (Chinese) | 79 | The Great |
| `lutie` | Lutie (Christmas) | 59 | Theme of Lutie |
| `hugel` | Hugel | 93 | Latinnova |
| `rachel` | Rachel | 94 | Theme of Rachel |
| `veins` | Veins | 104 | On Your Way Back |
| `moscovia` | Moscovia | 114 | Theme of Moscovia |

#### Fields

| Map Code Pattern | Area | BGM # | Track Name |
|-----------------|------|-------|------------|
| `prt_fild01-08` | Prontera Fields | 04, 12, 23 | I Miss You, Streamside, Travel |
| `moc_fild01-18` | Morroc Fields | 24, 37 | Desert, TeMPorsche |
| `gef_fild01-14` | Geffen Fields | 04, 23, 25 | I Miss You, Travel, Plateau |
| `pay_fild01-11` | Payon Fields | 36, 14 | Nano East, Theme of Payon |
| `mjolnir_01-12` | Mt. Mjolnir | 31, 33, 34 | Brassy Road, Yuna Song, Pampas Upas |
| `yuno_fild01-12` | Juno Fields | 70, 73 | Theme of Juno, Higher Than Sun |

#### Dungeons

| Map Code | Dungeon | BGM # | Track Name |
|----------|---------|-------|------------|
| `prt_sewb1-4` | Prontera Culvert | 19 | Under the Ground |
| `pay_dun00-04` | Payon Caves | 20 | Ancient Groover |
| `gef_dun00-03` | Geffen Tower | 21 | Through the Tower |
| `moc_pryd01-06` | Pyramid | 22 | Backattack!! |
| `moc_ruins` | Morroc Ruins | 28 | You're in Ruins |
| `iz_dun01-05` | Byalan Island | 49 | Watery Grave |
| `treasure01-02` | Sunken Ship | 17 | Treasure Hunter |
| `alde_dun01-04` | Clock Tower | 60 | Aeon |
| `c_tower1-4` | Clock Tower Base. | 61 | Zingaro |
| `gl_cas01-02` | Glast Heim Castle | 40 | Fear... |
| `gl_church` | Glast Heim Church | 40 | Fear... |
| `gl_prison` | Glast Heim Prison | 42 | Curse'n Pain |
| `anthell01-02` | Ant Hell | 46 | An Ant-lion's Pit |
| `beach_dun` | Comodo Beach Dng. | 63 | Mucho Gusto |
| `ama_dun01-03` | Amatsu Dungeon | 76 | Purity of Your Smile |
| `lou_dun01-03` | Louyang Dungeon | 80 | Jumping Dragon |
| `ayo_dun01-02` | Ayothaya Dungeon | 82 | Muay Thai King |
| `ra_san01-05` | Rachel Sanctuary | 95 | Underneath the Temple |
| `abbey01-03` | Nameless Island | 113 | Monastery in Disguise |
| `tha_t01-12` | Thanatos Tower | 100, 102 | Abyss, Erebos' Prelude |
| `lhz_dun01-04` | Lighthalzen Bio Lab | 92 | Naive Rave |
| `nif_fild01-02` | Niflheim Fields | 85 | Dancing Christmas in the 13th Month |
| `sphinx01-05` | Sphinx | 38 | Hamatan |
| `in_sphinx` | Inside Sphinx | 38 | Hamatan |

#### Special / Event

| Context | BGM # | Track Name |
|---------|-------|------------|
| Login Screen | 01 | Title |
| Character Select | 01 | Title (same track) |
| PvP Arena | 52, 123 | Ready~, Into the Arena! |
| WoE/GvG | 06 | Risk Your Life |
| Christmas Event Maps | 53-58 | White Christmas through Jingle Bell |
| Wedding | 09 | Great Honor |

### 2.3 BGM Behavior and Transitions

| Behavior | RO Classic Implementation |
|----------|--------------------------|
| **Looping** | All BGM tracks loop seamlessly. When a track reaches its end, it restarts from the beginning with no gap. |
| **Map Change** | BGM stops abruptly when entering a new map and the new map's BGM starts from the beginning. No crossfade in the original client. |
| **Same BGM** | If the new map uses the same BGM number, the music continues uninterrupted (e.g., moving between Prontera interior and exterior both using 08.mp3). |
| **Volume** | Player-adjustable via the Sound Configuration menu. Persists across sessions in the client config. |
| **Toggle** | `/bgm` command toggles all background music on/off instantly. |
| **Priority** | BGM plays at a fixed volume layer beneath SFX. No dynamic ducking in the original client. |

### 2.4 Musical Style Analysis by Zone Type

| Zone Type | Musical Characteristics | Examples |
|-----------|------------------------|----------|
| **Towns** | Warm, melodic, often orchestral with distinct cultural flavor. Each town has a unique identity. | Prontera (regal/medieval), Geffen (mystical), Payon (Eastern/folk), Morroc (Arabian) |
| **Fields** | Lighter, ambient, often featuring acoustic instruments. Less intense than town themes. Walking/exploration feel. | Streamside (gentle flute), Desert (Middle-Eastern percussion), Plateau (breezy) |
| **Dungeons** | Dark, tense, rhythmic. Often electronic or industrial elements. Builds urgency. | Fear (Glast Heim, ominous), Ancient Groover (groovy but dark), Backattack (percussive) |
| **Boss Areas** | Intense, driving rhythms. Heavy percussion, dramatic strings. | Risk Your Life (WoE combat), Erebos' Prelude (Thanatos) |
| **Events** | Themed to match event mood -- cheerful for Christmas, dramatic for PvP. | Christmas tracks (jolly), Into the Arena (competitive) |
| **Cultural Towns** | Incorporate traditional instruments from the represented culture. | Amatsu (koto/shamisen), Louyang (erhu/guzheng), Ayothaya (gamelan) |

---

## 3. Sound Effects (SFX)

### 3.1 File Organization

RO Classic stores all sound effects as WAV files within the `data.grf` archive under `data/wav/`. The directory structure:

```
data/wav/
  effect/               # Skill effects, casting sounds, impact effects
    ef_*.wav            # Effect sounds (hit, magic, buff)
    skill_*.wav         # Skill-specific sounds
  monster/              # Per-monster sounds organized by monster sprite name
    {monster_name}/
      atk.wav           # Attack sound
      damage.wav        # Taking damage sound
      die.wav           # Death sound
      idle.wav           # Ambient/idle sound (not all monsters)
  _hit*.wav             # Physical hit impact variants
  _attack*.wav          # Weapon swing variants
```

Monster sounds are tied to the ACT (Animation Collection) file format. Each monster's `.act` file contains keyframe data that references which sound effect to play at specific animation frames. This means sound timing is synchronized to sprite animation.

### 3.2 Combat Sound Effects

#### 3.2.1 Weapon Swing Sounds (by weapon type)

Each weapon type in RO has a distinct attack sound that plays when the player's auto-attack animation triggers:

| Weapon Type | Sound Description | Characteristics |
|-------------|-------------------|-----------------|
| **Dagger** | Quick slash/stab | Short, sharp metallic sound. Fast attack speed matches rapid sound. |
| **1H Sword** | Medium sword swing | Clean metallic slash, moderate duration. |
| **2H Sword** | Heavy sword swing | Deeper, weightier slash with more follow-through. |
| **Axe** | Chopping impact | Heavy, dull thud combined with wind. |
| **Mace** | Blunt impact | Solid thud/crack, no slicing sound. |
| **Spear** | Thrust/stab | Piercing sound, moderate weight. |
| **Staff/Rod** | Light bonk/tap | Lighter, wooden impact sound. |
| **Bow** | Arrow release/twang | String release + arrow flight whoosh. |
| **Knuckle/Fist** | Punch impact | Fleshy impact sound, quick. |
| **Instrument** | Strum/note | Musical note or strum sound. |
| **Whip** | Crack/snap | Sharp whip crack sound. |
| **Book** | Slam | Paper/leather impact. |
| **Katar** | Double slash | Dual rapid metallic slashes. |

#### 3.2.2 Hit Impact Sounds

| Sound | Trigger | Description |
|-------|---------|-------------|
| **Normal Hit** | Physical attack connects | Standard impact thud. Multiple variants for randomization. |
| **Critical Hit** | Critical strike lands | Higher-pitched, more dramatic impact. Distinctly different from normal hit to signal the critical. |
| **Miss** | Attack misses | Quiet "whoosh" / air swing. Sometimes no sound at all -- just the "MISS" text. |
| **Perfect Dodge** | Dodge triggers | Light evasion sound effect. |
| **Monster Hit Player** | Monster attack connects | Similar to normal hit but often has a "pain" element. |
| **Blocked** | Guard/Parry skill | Shield impact / deflection sound. |

#### 3.2.3 Death Sounds

| Entity | Sound Description |
|--------|-------------------|
| **Player Death** | Collapse / falling sound. Gendered (male vs female character model). |
| **Monster Death** | Per-monster death sounds defined in ACT files. Varies dramatically -- Poring "pop," skeleton "rattle and collapse," dragon "roar and crash." |
| **MVP Death** | Enhanced death sound, often longer and more dramatic than regular monsters. |

#### 3.2.4 Progression Sounds

| Event | Sound Description | Notes |
|-------|-------------------|-------|
| **Base Level Up** | Triumphant fanfare, ascending chime sequence | One of the most iconic RO sounds. Bright, celebratory, ~2 seconds. |
| **Job Level Up** | Similar to base level up but shorter/different variant | Distinct enough for players to tell which one occurred. |
| **Stat Point Applied** | Click/confirm sound | Quick feedback sound. |
| **Skill Point Applied** | Similar confirm sound | Quick feedback sound. |

### 3.3 Skill Sound Effects

RO Classic skills each have distinct audio cues. Sounds are triggered by the client-side effect system (effect IDs mapped in `effect_list.txt` with 968 entries, IDs 0-967).

#### 3.3.1 Mage / Wizard Skills

| Skill | Sound Description | Element |
|-------|-------------------|---------|
| **Fire Bolt** | Crackling fire whoosh per bolt, fireball launch sound | Fire |
| **Cold Bolt** | Ice crystal formation, sharp frozen impact per bolt | Water/Ice |
| **Lightning Bolt** | Electric crack/zap per bolt, thunder rumble | Wind/Lightning |
| **Fire Ball** | Large fireball launch whoosh + explosion on impact | Fire |
| **Fire Wall** | Sustained crackling fire, looping while active | Fire |
| **Frost Diver** | Ice spreading/freezing sound, crystallization effect | Water/Ice |
| **Thunderstorm** | Multiple thunder cracks, rain-like electrical strikes | Wind/Lightning |
| **Napalm Beat** | Dark energy burst, ghostly whoosh | Ghost/Shadow |
| **Soul Strike** | Multiple energy projectile launches, arcane impact | Ghost/Neutral |
| **Stone Curse** | Rumbling earth, petrification crackle | Earth |
| **Meteor Storm** | Deep rumbling, explosive meteor impacts, screen-shaking bass | Fire |
| **Storm Gust** | Howling blizzard wind, ice crystal shattering | Water/Ice |
| **Lord of Vermilion** | Massive thunder cascade, sustained electrical barrage | Wind/Lightning |
| **Jupitel Thunder** | Concentrated lightning ball, electric buzz, knockback whoosh | Wind/Lightning |
| **Water Ball** | Splashing water burst, liquid impact | Water |
| **Sight** | Fire ignition ring, burning aura hum | Fire |
| **Safety Wall** | Protective barrier hum, shield activation chime | Neutral |
| **Quagmire** | Mud/swamp bubbling, squelching | Earth |

#### 3.3.2 Acolyte / Priest / High Priest Skills

| Skill | Sound Description | Element |
|-------|-------------------|---------|
| **Heal** | Gentle ascending chime, warm bell tone | Holy |
| **Blessing** | Angelic chord, ethereal ascending notes | Holy |
| **Increase AGI** | Quick upward whoosh, speed aura sound | Holy |
| **Cure** | Cleansing chime, purification tone | Holy |
| **Kyrie Eleison** | Protective barrier activation, crystalline shield sound | Holy |
| **Magnificat** | Deep church bell, resonant holy chime | Holy |
| **Gloria** | Grand angelic chord, heavenly light sound | Holy |
| **Resurrection** | Dramatic rising tone, rebirth fanfare | Holy |
| **Sanctuary** | Sustained holy ground hum, healing aura | Holy |
| **Magnus Exorcismus** | Grand cross formation, holy light burst, dramatic choir | Holy |
| **Turn Undead** | Holy flash, purification burst | Holy |
| **Lex Aeterna** | Ethereal marking sound, faint holy echo | Holy |
| **Lex Divina** | Silencing seal, muting effect | Holy |
| **Aspersio** | Holy water blessing, crystal splash | Holy |
| **B.S. Sacramenti** | Deep holy enchantment, sustained blessing tone | Holy |
| **Assumptio** | Grand protective aura, ascending chord | Holy |

#### 3.3.3 Swordsman / Knight / Crusader Skills

| Skill | Sound Description |
|-------|-------------------|
| **Bash** | Heavy weapon impact, intensified hit sound |
| **Magnum Break** | Fire explosion burst from player, area push sound |
| **Endure** | Hardening/fortification sound, stoic tone |
| **Provoke** | Aggressive shout/taunt, challenging sound |
| **Bowling Bash** | Heavy spinning impact, multi-hit whoosh |
| **Brandish Spear** | Wide arc sweep sound, multiple slash impacts |
| **Pierce** | Rapid triple thrust, piercing metallic sounds |
| **Spear Boomerang** | Spear launch whoosh + return whoosh |
| **Two-Hand Quicken** | Rapid speed-up aura, weapon hum |
| **Auto Guard** | Shield raise, defensive stance |
| **Shield Charge** | Shield bash impact, rushing forward |
| **Grand Cross** | Massive holy cross explosion, dramatic holy burst |
| **Devotion** | Link/tether formation sound, protective chord |

#### 3.3.4 Archer / Hunter / Bard / Dancer Skills

| Skill | Sound Description |
|-------|-------------------|
| **Double Strafe** | Rapid double arrow release, twin twang |
| **Arrow Shower** | Multiple arrows rain, scattered impacts |
| **Improve Concentration** | Focus aura, heightened senses tone |
| **Ankle Snare** | Trap deployment click, spring activation |
| **Blast Mine** | Bomb placement, ticking, explosion |
| **Claymore Trap** | Heavy explosive placement, massive detonation |
| **Skid Trap** | Slippery surface deployment |
| **Freezing Trap** | Ice trap activation, freezing burst |
| **Sandman** | Sleep dust release, drowsy tone |
| **Falcon Assault** | Falcon screech, diving attack whoosh |
| **Blitz Beat** | Falcon attack sound, rapid pecking |
| **Arrow Repel** | Powerful knockback shot, heavy twang |

#### 3.3.5 Thief / Assassin / Rogue Skills

| Skill | Sound Description |
|-------|-------------------|
| **Double Attack** | Rapid twin slash, quick metallic impacts |
| **Steal** | Quick grabbing/snatching sound |
| **Hiding** | Fading/vanishing sound, stealth activation |
| **Envenom** | Poison application, dripping venom sound |
| **Sonic Blow** | Rapid multi-hit slash flurry, intense speed |
| **Venom Dust** | Poison cloud release, toxic hiss |
| **Grimtooth** | Underground attack burst, surprise strike |
| **Cloaking** | Invisibility activation, shimmer/fade sound |
| **Backstab** | Sneaky strike, amplified critical hit |
| **Raid** | Area attack burst from hiding |
| **Strip Weapon/Armor/Shield/Helm** | Equipment removal sounds (each has unique variant) |

#### 3.3.6 Merchant / Blacksmith / Alchemist Skills

| Skill | Sound Description |
|-------|-------------------|
| **Mammonite** | Gold coin toss + heavy impact |
| **Cart Revolution** | Cart swing, heavy wooden impact |
| **Adrenaline Rush** | Speed buff activation, forge hammer sound |
| **Power Thrust** | Power buff activation, strengthening sound |
| **Weapon Forge** | Hammer on anvil, metalworking sounds |
| **Acid Terror** | Acid splash, corrosive hissing |
| **Demonstration** | Bomb throw, explosive impact |
| **Bomb** | Explosive deployment, blast sound |
| **Homunculus Skills** | Various creature-specific sounds |

#### 3.3.7 Buff Application / Debuff Sounds

| Effect Type | Sound Description |
|-------------|-------------------|
| **Buff Applied** | Gentle ascending tone, positive aura activation |
| **Debuff Applied** | Descending tone, negative effect sound |
| **Buff Expired** | Quiet fading sound, buff disappearance |
| **Poison Status** | Sickly bubbling, toxic drip |
| **Stun Status** | Stars/birds circling, dizzy tone |
| **Freeze Status** | Ice encasement, crystallization crack |
| **Stone Status** | Petrification rumble, hardening crack |
| **Blind Status** | Darkness descending, vision impairment sound |
| **Silence Status** | Muting seal, quiet dampening |
| **Sleep Status** | Lullaby tone, drowsy effect |
| **Curse Status** | Dark curse application, ominous whisper |
| **Confusion Status** | Disorienting swirl, chaotic sound |

### 3.4 UI Sound Effects

| UI Action | Sound Description | Spatial |
|-----------|-------------------|---------|
| **Menu Open** | Clean UI pop/click | 2D (non-spatial) |
| **Menu Close** | Reverse pop/click | 2D |
| **Button Click** | Crisp click/tap | 2D |
| **Item Pickup** | Quick grab/collection sound, slight chime | 2D |
| **Item Drop** | Thud/drop sound | 2D |
| **Equip Item** | Equipment attachment sound, metallic clip | 2D |
| **Unequip Item** | Equipment removal sound | 2D |
| **Inventory Full** | Error/denied buzz | 2D |
| **Error / Denied** | Negative buzz/beep | 2D |
| **Chat Message Received** | Subtle notification chime | 2D |
| **Whisper Notification** | Distinct private message alert, different from public chat | 2D |
| **Trade Window Open** | Transaction initiation sound | 2D |
| **Trade Complete** | Successful exchange chime | 2D |
| **NPC Dialog Advance** | Text advancement click, page turn | 2D |
| **NPC Dialog Open** | Dialog window appearance sound | 2D |
| **Skill Drag to Hotbar** | Slot assignment click | 2D |
| **Hotbar Skill Activate** | Quick activation feedback | 2D |
| **Zeny Transaction** | Coin counting/jingling | 2D |
| **Card Insert** | Card slot activation, compound sound | 2D |
| **Refine Success** | Triumphant chime, upgrade fanfare | 2D |
| **Refine Failure** | Breaking/shattering sound, negative tone | 2D |

### 3.5 Environment / Ambient Sound Effects

| Environment | Sounds | Spatial |
|-------------|--------|---------|
| **Warp Portal** | Sustained magical hum/whoosh, pulsating energy. Plays at the portal location. | 3D spatial |
| **Kafra Station** | NPC ambient, possibly faint Kafra greeting | 3D spatial |
| **Fountain** | Water splashing, trickling | 3D spatial |
| **Forge / Anvil** | Hammering, metalworking ambient | 3D spatial |
| **Fireplace** | Crackling fire, warm ambiance | 3D spatial |
| **Water (stream/river)** | Running water, babbling brook | 3D spatial |
| **Water (ocean/lake)** | Waves, lapping water | 3D spatial |
| **Wind** | Gentle to strong wind depending on elevation/exposure | 3D spatial |
| **Town Crowd** | Background chatter, marketplace murmur, footsteps | 3D spatial |
| **Dungeon Dripping** | Water drip echoes, resonant drops | 3D spatial |
| **Dungeon Ambient** | Distant growls, creaking, stone shifting | 3D spatial |
| **Forest Birds** | Bird calls, chirping, rustling leaves | 3D spatial |
| **Desert Wind** | Dry wind, sand shifting | 3D spatial |
| **Snow Crunch** | Footsteps on snow (Lutie) | 3D spatial |
| **Grass Footstep** | Soft ground footstep | 3D spatial |
| **Stone Footstep** | Hard surface footstep (dungeons, towns) | 3D spatial |
| **Wood Footstep** | Wooden floor footstep (interiors) | 3D spatial |

> **Note on RO Classic**: The original 2D client had minimal environmental ambient audio. Most ambiance came from the BGM itself. True ambient sound layers were limited compared to modern 3D games. For our UE5 replica, we should add rich ambient layers that RO could not achieve in its 2D engine.

### 3.6 Monster Sound Effects

Each monster in RO has up to 5 animation states with associated audio, defined in the monster's ACT sprite file:

| Animation State | Sound Trigger | Description |
|-----------------|---------------|-------------|
| **Idle** | Monster standing/breathing | Ambient creature sound -- growl, chirp, buzz. Many monsters are silent in idle. |
| **Walk/Chase** | Monster moving toward target | Footstep/movement sounds. Often same as idle or absent. |
| **Attack** | Monster executes attack animation | Attack sound -- slash, bite, spell cast. Synchronized to attack animation frame. |
| **Damage** | Monster takes a hit | Pain/impact reaction -- yelp, crunch, recoil sound. |
| **Death** | Monster is killed | Death cry/collapse -- varies wildly by monster type. |

#### Notable Monster Sounds (Representative Examples)

| Monster | Idle | Attack | Death | Notes |
|---------|------|--------|-------|-------|
| **Poring** | Soft bouncing/jiggling | Light slap | Pop/burst | Iconic jelly sound |
| **Lunatic** | Quiet rustling | Quick scratch | Squeak | Small rodent sounds |
| **Fabre** | Soft crawling/chittering | Tiny bite | Squish | Insect sounds |
| **Rocker** | Cricket chirping | String strum | Crash | Musical insect |
| **Willow** | Rustling leaves | Branch whip | Timber crack | Tree sounds |
| **Skeleton** | Bone rattling | Sword slash | Bone collapse/scatter | Undead sounds |
| **Zombie** | Groaning/moaning | Claw swipe | Collapse | Classic zombie audio |
| **Orc Warrior** | Grunting | Heavy weapon swing | Death grunt | Humanoid aggressive |
| **Baphomet** | Deep demonic growl | Massive slash | Roar + collapse | MVP boss, dramatic audio |
| **Moonlight Flower** | Mystical hum | Magic burst | Ethereal fade | MVP, magical audio |
| **Golden Bug** | Buzzing | Sting attack | Crunch | Insect MVP |
| **Drake** | Ghost ship creaking | Cutlass slash | Spectral wail | Undead pirate MVP |

---

## 4. Voice and Character Sounds

### 4.1 Overview

RO Classic has **minimal voice acting**. The game relies heavily on text for dialogue and character expression. Voice/character sounds are limited to:

### 4.2 Player Character Sounds

| Sound | Trigger | Notes |
|-------|---------|-------|
| **Pain/Hurt** | Taking damage | Gendered (male/female). Short grunt/yelp. Different per job class in some versions. |
| **Death** | Player character dies | Gendered collapse sound. Each job class originally had unique death noises. |
| **Sitting** | Character sits down | Subtle "sit" sound (cloth rustling). |
| **Emote Sounds** | Using emotes | Laugh (/heh), cry (sobbing), surprise (gasp). Tied to emote bubble display. |

### 4.3 Skill Shouts / Voice Lines

Some advanced job skills have vocal components:

| Class | Skill | Voice |
|-------|-------|-------|
| **High Wizard** | Casting high-level spells | Incantation-like vocal sounds during cast |
| **Champion/Monk** | Asura Strike (Guillotine Fist) | Battle cry / ki shout on execution |
| **Crusader/Paladin** | Grand Cross / Sacrifice | Holy invocation voice |
| **Bard** | Performance skills | Singing voice for performance skills |
| **Dancer** | Performance skills | Vocal accompaniment during dances |

> **Note**: Voice lines are more prominent in later updates and Ragnarok Online 2. RO Classic (pre-Renewal) uses them sparingly. Many sounds were added in kRO (Korean Ragnarok Online) updates and not all made it to iRO (international).

### 4.4 NPC Voice Lines

| NPC | Voice Line | Context |
|-----|-----------|---------|
| **Kafra NPCs** | "Welcome!" / greeting sound | When player interacts with Kafra. Korean voice. |
| **Cool Event Corp** | Event NPC greetings | Seasonal event NPCs may have voice greetings. |
| **Job Change NPCs** | Congratulatory lines | Some job change sequences include voiced congratulations (in later patches). |

### 4.5 Implementation Note for Sabri_MMO

For our UE5 replica, voice implementation should be:
- **Phase 1**: No voice -- focus on BGM and SFX first
- **Phase 2**: Add gendered hurt/death sounds for player characters
- **Phase 3**: Add key NPC greetings (Kafra "Welcome!")
- **Phase 4**: Add skill vocal effects for advanced classes
- **Future**: Consider AI-generated voice lines or commissioned voice acting for unique NPC dialogue

---

## 5. 3D Audio in UE5

### 5.1 Spatialization

Since we are rebuilding RO as a 3D game in UE5, we gain full 3D audio capabilities that the original 2D client could not provide:

| Feature | Implementation | Purpose |
|---------|---------------|---------|
| **Spatialized Audio** | UE5 built-in HRTF spatialization | Combat effects sound like they come from the source direction |
| **Distance Attenuation** | `USoundAttenuation` with linear/logarithmic falloff | Sounds get quieter with distance, creating depth |
| **Sound Occlusion** | Raycast-based occlusion checks | Sounds behind walls are muffled |
| **Reverb Zones** | `AReverbEffect` volumes per zone type | Dungeons echo, open fields do not |
| **Audio Priority** | `USoundConcurrency` priority system | Important sounds (skill impacts) play over ambient |

### 5.2 Attenuation Profiles

Define different attenuation settings per sound category:

| Profile | Inner Radius | Outer Radius | Falloff | Use For |
|---------|-------------|-------------|---------|---------|
| **Combat_Close** | 200 UU | 2000 UU | Linear | Melee hits, weapon swings |
| **Combat_Ranged** | 400 UU | 4000 UU | Logarithmic | Ranged skill impacts, arrows |
| **Combat_AoE** | 500 UU | 5000 UU | Logarithmic | AoE spell effects, explosions |
| **Ambient_Small** | 100 UU | 1000 UU | Linear | Fountains, torches, small sources |
| **Ambient_Large** | 500 UU | 3000 UU | Linear | Waterfalls, crowds, large sources |
| **Monster_Idle** | 200 UU | 1500 UU | Linear | Monster ambient/idle sounds |
| **Monster_Attack** | 300 UU | 3000 UU | Linear | Monster attack/death sounds |
| **NPC_Voice** | 200 UU | 1000 UU | Linear | NPC greetings, voice lines |
| **UI** | N/A | N/A | None | 2D UI sounds -- no spatialization |
| **BGM** | N/A | N/A | None | 2D background music -- no spatialization |

### 5.3 Reverb Zones

| Zone Type | Reverb Preset | Settings |
|-----------|--------------|----------|
| **Open Field** | None / minimal | Very short decay, low wet level |
| **Town Square** | Light outdoor | Short decay (~0.5s), low reflection |
| **Building Interior** | Small room | Medium decay (~1.0s), moderate reflection |
| **Cathedral/Church** | Large hall | Long decay (~2.5s), high reflection |
| **Cave/Dungeon** | Cave | Long decay (~3.0s), high wet, moderate diffusion |
| **Deep Dungeon** | Large cave | Very long decay (~4.0s), heavy reverb |
| **Sewer/Culvert** | Pipe/tunnel | Medium decay with distinct echo |
| **Underwater** | Underwater | Heavy low-pass filter, muffled, long decay |

### 5.4 Sound Concurrency Limits

Prevent audio chaos when many entities are active:

| Sound Group | Max Concurrent | Resolution Rule | Notes |
|-------------|---------------|-----------------|-------|
| **BGM** | 1 | Newest replaces oldest | Only one BGM track at a time |
| **Weapon Swings** | 4 | Oldest stops | Prevent 50 simultaneous swings |
| **Skill Impacts** | 8 | Lowest volume stops | Allow more skill variety |
| **Monster Idle** | 6 | Farthest stops | Nearby monsters only |
| **Monster Attack** | 6 | Oldest stops | Prioritize recent attacks |
| **Monster Death** | 4 | Oldest stops | Death sounds are brief |
| **Footsteps** | 3 | Nearest plays | Only hear closest footsteps |
| **Ambient Loops** | 8 | Farthest stops | Environmental ambiance |
| **UI Sounds** | 3 | Newest replaces | Prevent UI sound spam |
| **Level Up** | 1 | Always plays | Never miss the level up fanfare |

---

## 6. UE5 Implementation Architecture

### 6.1 Audio Subsystem Classes

Following the project's existing subsystem pattern (like `SkillVFXSubsystem`, `BasicInfoSubsystem`), audio will use `UWorldSubsystem` classes:

```
client/SabriMMO/Source/SabriMMO/Audio/
  AudioManager.h / .cpp              # Main audio coordinator
  BGMSubsystem.h / .cpp              # Background music management (UWorldSubsystem)
  SFXSubsystem.h / .cpp              # Sound effect playback (UWorldSubsystem)
  AmbientAudioSubsystem.h / .cpp     # Environmental ambient management (UWorldSubsystem)
  AudioData.h / .cpp                 # Data tables: zone->BGM, skill->SFX mappings
```

### 6.2 BGMSubsystem (Background Music Manager)

```cpp
// BGMSubsystem.h
UCLASS()
class UBGMSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Play BGM for a zone (with crossfade)
    void PlayBGMForZone(const FString& ZoneName);

    // Direct BGM control
    void PlayBGM(USoundWave* Track, float FadeInDuration = 1.0f);
    void StopBGM(float FadeOutDuration = 1.0f);
    void PauseBGM();
    void ResumeBGM();

    // Volume control (0.0 - 1.0)
    void SetBGMVolume(float Volume);
    float GetBGMVolume() const;

    // Toggle BGM on/off (like /bgm command)
    void ToggleBGM();
    bool IsBGMEnabled() const;

private:
    // Two audio components for crossfading
    UPROPERTY()
    UAudioComponent* CurrentBGMComponent;

    UPROPERTY()
    UAudioComponent* FadingOutBGMComponent;

    // Crossfade state
    FTimerHandle CrossfadeTimerHandle;
    float CrossfadeDuration;
    float BGMVolume;
    bool bBGMEnabled;

    // Current zone tracking
    FString CurrentZoneName;
    USoundWave* CurrentTrack;

    // Zone -> BGM lookup
    USoundWave* GetBGMForZone(const FString& ZoneName) const;

    // Crossfade tick
    void UpdateCrossfade();
};
```

**Key Design Decisions:**
- **Two UAudioComponents** for crossfading: one playing current track, one fading out the previous track. This gives smooth transitions unlike RO's original abrupt switches.
- **UWorldSubsystem** so it is scoped to the world/level and automatically cleaned up on level transitions.
- **Same-BGM detection**: If the new zone uses the same BGM as the current zone, skip the crossfade (match RO behavior where music continues between maps sharing the same track).
- **Volume persistence**: Store BGM volume in `GameUserSettings.ini` so it persists across sessions.

### 6.3 SFXSubsystem (Sound Effect Manager)

```cpp
// SFXSubsystem.h
UCLASS()
class USFXSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Combat SFX (3D spatialized)
    void PlayWeaponSwing(EWeaponType WeaponType, FVector Location);
    void PlayHitImpact(EHitType HitType, FVector Location);
    void PlayMissSound(FVector Location);
    void PlayDeathSound(bool bIsPlayer, bool bIsMale, FVector Location);

    // Skill SFX (3D spatialized)
    void PlaySkillCastStart(int32 SkillId, FVector Location);
    void PlaySkillImpact(int32 SkillId, FVector Location);
    void PlaySkillLoop(int32 SkillId, FVector Location, UAudioComponent*& OutComponent);

    // Status SFX
    void PlayBuffApplied(int32 BuffId, FVector Location);
    void PlayDebuffApplied(int32 DebuffId, FVector Location);

    // Progression SFX (2D, non-spatial)
    void PlayLevelUp();
    void PlayJobLevelUp();

    // UI SFX (2D, non-spatial)
    void PlayUISound(EUISoundType SoundType);

    // Monster SFX (3D spatialized)
    void PlayMonsterSound(int32 MonsterId, EMonsterSoundType SoundType, FVector Location);

    // Volume control
    void SetSFXVolume(float Volume);
    float GetSFXVolume() const;

private:
    // Sound pools for commonly used effects
    TMap<EUISoundType, USoundBase*> UISounds;
    TMap<int32, FSkillSFXConfig> SkillSFXMap;

    float SFXVolume;
};
```

### 6.4 Sound Class / Sound Mix Hierarchy

UE5 Sound Classes group sounds for collective volume control:

```
SC_Master (root)
  |
  +-- SC_BGM                    # Background music
  |
  +-- SC_SFX                    # All sound effects
  |     |
  |     +-- SC_Combat           # Weapon swings, hits, skill impacts
  |     +-- SC_Skills           # Skill casting and effect sounds
  |     +-- SC_Monster          # Monster idle, attack, death
  |     +-- SC_Progression      # Level up, job level up
  |
  +-- SC_Ambient                # Environmental ambiance
  |     |
  |     +-- SC_AmbientLoop      # Looping ambient (water, wind, crowd)
  |     +-- SC_Footsteps        # Terrain-based footsteps
  |
  +-- SC_UI                     # UI feedback sounds
  |
  +-- SC_Voice                  # Voice / character sounds
```

**Sound Mixes** for dynamic volume adjustment:

| Mix | Purpose | Adjustments |
|-----|---------|-------------|
| **SM_Default** | Normal gameplay | All classes at user-set volumes |
| **SM_Menu** | In menu/dialog | BGM -3dB, SFX -6dB, UI normal |
| **SM_Combat** | Active combat | Combat +3dB, Ambient -6dB |
| **SM_Loading** | Zone loading | Everything faded, BGM continues |
| **SM_Muted** | All muted | Everything at -80dB |

### 6.5 MetaSounds vs Sound Cues Decision

| Technology | Strengths | Use In This Project |
|-----------|-----------|-------------------|
| **MetaSounds** (UE5 native) | Procedural audio, full DSP control, better performance, node-based programming | Skill effects that need parameter-driven variation, ambient layers, procedural footsteps |
| **Sound Cues** (Legacy) | Simple randomization, pitch variation, easy setup, well-documented | Quick SFX with random variant selection (hit sounds, UI clicks), simple one-shot effects |
| **Sound Waves** (Raw) | Direct playback, lowest overhead | BGM tracks (direct streaming), simple UI sounds |

**Recommendation**: Use **Sound Cues** for most combat/UI SFX (simpler, proven pattern). Use **MetaSounds** for ambient systems and effects needing real-time parameter control. Use **Sound Waves** directly for BGM streaming.

### 6.6 Integration with Existing Systems

| Existing System | Audio Integration Point |
|----------------|------------------------|
| **ZoneTransitionSubsystem** | Triggers `BGMSubsystem::PlayBGMForZone()` when zone changes. Start crossfade during loading. |
| **SkillVFXSubsystem** | Call `SFXSubsystem::PlaySkillCastStart()` and `PlaySkillImpact()` alongside VFX spawn. |
| **DamageNumberSubsystem** | Call `SFXSubsystem::PlayHitImpact()` on `combat:damage` and `skill:effect_damage` events. |
| **BasicInfoSubsystem** | Call `SFXSubsystem::PlayLevelUp()` on level up detection. |
| **InventorySubsystem** | Call `SFXSubsystem::PlayUISound()` on pickup, equip, drop, error events. |
| **HotbarSubsystem** | Call `SFXSubsystem::PlayUISound()` on hotbar activation. |
| **CastBarSubsystem** | Trigger casting sounds on `skill:cast_start`, stop on `skill:cast_complete`/`interrupted`. |
| **WorldHealthBarSubsystem** | No direct audio integration needed. |
| **BP_EnemyManager** | Route monster spawn, attack, death sounds through SFXSubsystem. |
| **BP_OtherPlayerManager** | Route other player combat sounds through SFXSubsystem (spatialized). |

### 6.7 C++ Code Patterns

#### Playing a Spatialized Sound at Location

```cpp
// SFXSubsystem.cpp
void USFXSubsystem::PlayHitImpact(EHitType HitType, FVector Location)
{
    USoundBase* Sound = GetHitSound(HitType);
    if (!Sound) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UGameplayStatics::PlaySoundAtLocation(
        World,
        Sound,
        Location,
        FRotator::ZeroRotator,
        SFXVolume,           // Volume multiplier
        FMath::RandRange(0.95f, 1.05f),  // Pitch variation
        0.0f,                // Start time
        GetAttenuationSettings(EAttenuationProfile::Combat_Close),
        GetConcurrencySettings(ESoundGroup::WeaponSwings)
    );
}
```

#### Playing a Non-Spatial UI Sound

```cpp
void USFXSubsystem::PlayUISound(EUISoundType SoundType)
{
    USoundBase* Sound = UISounds.FindRef(SoundType);
    if (!Sound) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // UI sounds: no attenuation, no spatialization
    UGameplayStatics::PlaySound2D(World, Sound, SFXVolume);
}
```

#### BGM Crossfade Implementation

```cpp
void UBGMSubsystem::PlayBGMForZone(const FString& ZoneName)
{
    if (!bBGMEnabled) return;

    USoundWave* NewTrack = GetBGMForZone(ZoneName);
    if (!NewTrack) return;

    // If same track, keep playing (match RO behavior)
    if (NewTrack == CurrentTrack) return;

    // Start crossfade
    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        // Move current to fading-out slot
        if (FadingOutBGMComponent)
        {
            FadingOutBGMComponent->Stop();
        }
        FadingOutBGMComponent = CurrentBGMComponent;
        FadingOutBGMComponent->FadeOut(CrossfadeDuration, 0.0f);
    }

    // Start new track
    CurrentBGMComponent = NewObject<UAudioComponent>(this);
    CurrentBGMComponent->SetSound(NewTrack);
    CurrentBGMComponent->bAutoDestroy = false;
    CurrentBGMComponent->bIsUISound = true;  // Not spatialized
    CurrentBGMComponent->FadeIn(CrossfadeDuration, BGMVolume);
    CurrentBGMComponent->Play();

    CurrentTrack = NewTrack;
    CurrentZoneName = ZoneName;
}
```

---

## 7. Asset Creation and Sourcing

### 7.1 Copyright Considerations

All original RO music and sound effects are **copyrighted by Gravity Corp and SoundTeMP**. We cannot use them directly. Options:

### 7.2 Background Music Sourcing

| Option | Pros | Cons | Cost | Recommendation |
|--------|------|------|------|---------------|
| **Commission original compositions** | Unique, legally clean, can match RO style exactly | Expensive, time-consuming | $200-2000+ per track | Best for final release |
| **AI music generation (Suno, Udio)** | Fast, cheap, can iterate on style | Quality varies, copyright uncertain for commercial use, may not capture RO magic | $10-50/month subscription | Good for prototyping |
| **Royalty-free fantasy music** | Legal, affordable, available now | Generic, won't match RO's specific vibe | $5-50 per track | Good for early development |
| **UE5 Marketplace music packs** | Pre-formatted for UE5, legal | Limited selection matching RO style | $10-100 per pack | Supplement for ambient |
| **Open-source game music** | Free, legal | Very limited RO-style options | Free | Background/filler only |

**Recommended approach**: Use **AI-generated music** for rapid prototyping (match RO styles per zone type), then **commission original tracks** for key zones (Prontera, Geffen, Payon, Morroc) as development matures.

### 7.3 Sound Effects Sourcing

| Source | Use For | Notes |
|--------|---------|-------|
| **Freesound.org** | Base combat sounds, ambient, environmental | CC0/CC-BY licensed. Needs post-processing. |
| **OpenGameArt.org** | RPG combat SFX packs, UI sounds | Free, game-oriented. Quality varies. |
| **Audacity / Reaper** | Custom sound design, mixing, processing | Free (Audacity) or affordable (Reaper). Layer multiple sources. |
| **UE5 Marketplace SFX** | Combat, magic, ambient packs | Pre-formatted for UE5. "Fantasy RPG SFX" packs available. |
| **SONNISS GDC Bundle** | Professional game audio | Annual free bundle with thousands of sounds. |
| **Custom Foley** | Unique sounds, weapon impacts | Record real sounds, process digitally. |

### 7.4 File Format Specifications

| Audio Type | Format | Sample Rate | Bit Depth | Channels | Notes |
|-----------|--------|-------------|-----------|----------|-------|
| **BGM** | OGG Vorbis | 44100 Hz | 16-bit | Stereo | Streaming playback, smaller file size than WAV |
| **SFX (short)** | WAV | 44100 Hz | 16-bit | Mono | Low-latency playback, under 5 seconds |
| **SFX (loops)** | WAV | 44100 Hz | 16-bit | Mono | Seamless loop points, mono for 3D spatialization |
| **Ambient** | OGG Vorbis | 44100 Hz | 16-bit | Stereo | Longer loops, streaming preferred |
| **Voice** | WAV or OGG | 44100 Hz | 16-bit | Mono | Short clips WAV, longer OGG |
| **UI** | WAV | 44100 Hz | 16-bit | Mono | Very short, instant playback needed |

> **UE5 Import Note**: UE5 internally converts all audio to its own format on import. Import source files at highest reasonable quality. OGG/WAV both work. UE5 supports streaming for large files (BGM).

### 7.5 Asset Naming Convention (UE5)

Following the project's naming patterns:

```
Content/SabriMMO/Audio/
  BGM/
    BGM_Prontera.uasset           # Town BGMs
    BGM_Geffen.uasset
    BGM_Payon.uasset
    BGM_Morroc.uasset
    BGM_Title.uasset              # Login screen
    BGM_PrtCulvert.uasset         # Dungeon BGMs
    BGM_PayonCave.uasset
    ...
  SFX/
    Combat/
      SFX_Hit_Normal_01.uasset    # Numbered variants for randomization
      SFX_Hit_Normal_02.uasset
      SFX_Hit_Normal_03.uasset
      SFX_Hit_Critical_01.uasset
      SFX_Miss_01.uasset
      SC_Hit_Normal.uasset        # Sound Cue wrapping variants
    Weapons/
      SFX_Swing_Sword1H_01.uasset
      SFX_Swing_Sword2H_01.uasset
      SFX_Swing_Dagger_01.uasset
      SFX_Swing_Bow_01.uasset
      SFX_Swing_Mace_01.uasset
      SFX_Swing_Axe_01.uasset
      SFX_Swing_Staff_01.uasset
      SFX_Swing_Spear_01.uasset
      SFX_Swing_Knuckle_01.uasset
    Skills/
      SFX_Skill_FireBolt_Cast.uasset
      SFX_Skill_FireBolt_Impact.uasset
      SFX_Skill_ColdBolt_Cast.uasset
      SFX_Skill_ColdBolt_Impact.uasset
      SFX_Skill_Heal_01.uasset
      SFX_Skill_Blessing_01.uasset
      ...
    Monsters/
      SFX_Monster_Poring_Attack.uasset
      SFX_Monster_Poring_Death.uasset
      SFX_Monster_Skeleton_Attack.uasset
      ...
    UI/
      SFX_UI_Click.uasset
      SFX_UI_MenuOpen.uasset
      SFX_UI_ItemPickup.uasset
      SFX_UI_Error.uasset
      SFX_UI_LevelUp.uasset
      SFX_UI_JobLevelUp.uasset
      ...
    Ambient/
      SFX_Amb_Water_Stream.uasset
      SFX_Amb_Wind_Gentle.uasset
      SFX_Amb_Dungeon_Drip.uasset
      SFX_Amb_Town_Crowd.uasset
      SFX_Amb_Forest_Birds.uasset
      ...
    Footsteps/
      SFX_Foot_Grass_01.uasset
      SFX_Foot_Stone_01.uasset
      SFX_Foot_Wood_01.uasset
      SFX_Foot_Sand_01.uasset
      SFX_Foot_Snow_01.uasset
      SFX_Foot_Water_01.uasset
  Voice/
    SFX_Voice_Male_Hurt_01.uasset
    SFX_Voice_Female_Hurt_01.uasset
    SFX_Voice_Male_Death_01.uasset
    SFX_Voice_Female_Death_01.uasset
    SFX_Voice_Kafra_Welcome.uasset
  SoundCues/
    SC_Hit_Normal.uasset           # Sound Cue with random variant selection
    SC_Swing_Sword.uasset
    SC_Footstep_Grass.uasset
    ...
  Attenuation/
    SA_Combat_Close.uasset
    SA_Combat_Ranged.uasset
    SA_Ambient_Small.uasset
    SA_Ambient_Large.uasset
    SA_Monster_Idle.uasset
    ...
  Concurrency/
    SCon_WeaponSwings.uasset
    SCon_SkillImpacts.uasset
    SCon_MonsterIdle.uasset
    SCon_UI.uasset
    ...
  SoundClasses/
    SC_Master.uasset
    SC_BGM.uasset
    SC_SFX.uasset
    SC_Combat.uasset
    SC_UI.uasset
    SC_Ambient.uasset
    SC_Voice.uasset
  SoundMixes/
    SM_Default.uasset
    SM_Menu.uasset
    SM_Combat.uasset
    SM_Loading.uasset
```

---

## 8. Audio Data Structures

### 8.1 Zone-to-BGM Mapping (C++)

```cpp
// AudioData.h
struct FZoneBGMConfig
{
    FString ZoneName;           // e.g., "prt_south"
    FString TrackName;          // e.g., "Theme of Prontera"
    TSoftObjectPtr<USoundWave> BGMAsset;  // Soft reference for async loading
    float DefaultVolume;        // Zone-specific volume (0.0-1.0)
};

// AudioData.cpp
TMap<FString, FZoneBGMConfig> BuildZoneBGMMap()
{
    TMap<FString, FZoneBGMConfig> Map;

    // Towns
    Map.Add("prt_south", {"prt_south", "Theme of Prontera",
        TSoftObjectPtr<USoundWave>(FSoftObjectPath("/Game/SabriMMO/Audio/BGM/BGM_Prontera")), 1.0f});
    Map.Add("geffen", {"geffen", "Theme of Geffen",
        TSoftObjectPtr<USoundWave>(FSoftObjectPath("/Game/SabriMMO/Audio/BGM/BGM_Geffen")), 1.0f});
    // ... etc for all zones
    return Map;
}
```

### 8.2 Skill-to-SFX Mapping

```cpp
// AudioData.h
struct FSkillSFXConfig
{
    int32 SkillId;
    TSoftObjectPtr<USoundBase> CastSound;       // Sound on cast start
    TSoftObjectPtr<USoundBase> ImpactSound;      // Sound on skill hit
    TSoftObjectPtr<USoundBase> LoopSound;        // Looping sound (fire wall, etc.)
    float CastVolume;
    float ImpactVolume;
    EAttenuationProfile AttenuationProfile;
};

// AudioData.cpp
TMap<int32, FSkillSFXConfig> BuildSkillSFXMap()
{
    TMap<int32, FSkillSFXConfig> Map;

    // Mage skills
    Map.Add(19, {19, /* Fire Bolt cast */, /* Fire Bolt impact */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Ranged});
    Map.Add(14, {14, /* Cold Bolt cast */, /* Cold Bolt impact */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Ranged});
    Map.Add(20, {20, /* Lightning Bolt cast */, /* Lightning Bolt impact */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Ranged});
    Map.Add(207, {207, /* Fire Ball cast */, /* Fire Ball impact */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_AoE});
    Map.Add(208, {208, /* Frost Diver cast */, /* Frost Diver impact */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Ranged});
    Map.Add(209, {209, /* Fire Wall cast */, nullptr, /* Fire Wall loop */, 1.0f, 1.0f, EAttenuationProfile::Combat_AoE});
    Map.Add(210, {210, /* Soul Strike cast */, /* Soul Strike impact */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Ranged});
    Map.Add(211, {211, /* Safety Wall cast */, nullptr, /* Safety Wall loop */, 1.0f, 1.0f, EAttenuationProfile::Combat_Close});
    Map.Add(212, {212, /* Thunderstorm cast */, /* Thunderstorm strike */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_AoE});

    // Priest skills
    Map.Add(28, {28, /* Heal */ nullptr, /* Heal chime */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Close});
    Map.Add(34, {34, /* Blessing */ nullptr, /* Blessing chord */, nullptr, 1.0f, 1.0f, EAttenuationProfile::Combat_Close});

    // ... etc for all skills
    return Map;
}
```

### 8.3 Monster-to-SFX Mapping

```cpp
// AudioData.h
struct FMonsterSFXConfig
{
    int32 MonsterId;
    TSoftObjectPtr<USoundBase> IdleSound;
    TSoftObjectPtr<USoundBase> AttackSound;
    TSoftObjectPtr<USoundBase> DamageSound;
    TSoftObjectPtr<USoundBase> DeathSound;
    float IdleInterval;  // Seconds between idle sound plays
};

// AudioData.cpp
TMap<int32, FMonsterSFXConfig> BuildMonsterSFXMap()
{
    TMap<int32, FMonsterSFXConfig> Map;

    // Example entries (509 monsters from ro_monster_templates)
    Map.Add(1002, {1002, /* Poring idle */, /* Poring attack */, /* Poring damage */, /* Poring death */, 5.0f});
    Map.Add(1063, {1063, /* Lunatic idle */, /* Lunatic attack */, /* Lunatic damage */, /* Lunatic death */, 8.0f});
    Map.Add(1007, {1007, /* Fabre idle */, /* Fabre attack */, /* Fabre damage */, /* Fabre death */, 7.0f});
    // ... etc
    return Map;
}
```

### 8.4 Weapon-to-SFX Mapping

```cpp
// AudioData.h
enum class EWeaponSFXType : uint8
{
    Dagger,
    Sword1H,
    Sword2H,
    Axe1H,
    Axe2H,
    Mace,
    Spear1H,
    Spear2H,
    Staff,
    Bow,
    Knuckle,
    Instrument,
    Whip,
    Book,
    Katar,
    Unarmed
};

struct FWeaponSFXConfig
{
    EWeaponSFXType WeaponType;
    TSoftObjectPtr<USoundBase> SwingSound;     // Sound Cue with variants
    TSoftObjectPtr<USoundBase> HitSound;       // Sound Cue with variants
    float PitchVariation;  // Random pitch range (+/- this value)
};
```

### 8.5 Terrain-to-Footstep Mapping

```cpp
// AudioData.h
enum class ETerrainType : uint8
{
    Grass,
    Stone,
    Wood,
    Sand,
    Snow,
    Water,
    Dirt,
    Metal,
    Carpet
};

struct FFootstepSFXConfig
{
    ETerrainType TerrainType;
    TSoftObjectPtr<USoundBase> FootstepCue;  // Sound Cue with 3-5 variants
    float VolumeMultiplier;
    float PitchRange;
};
```

### 8.6 UI Action-to-SFX Mapping

```cpp
// AudioData.h
enum class EUISoundType : uint8
{
    ButtonClick,
    MenuOpen,
    MenuClose,
    ItemPickup,
    ItemDrop,
    ItemEquip,
    ItemUnequip,
    InventoryFull,
    Error,
    ChatMessage,
    WhisperReceived,
    TradeComplete,
    NPCDialogAdvance,
    NPCDialogOpen,
    HotbarActivate,
    SkillAssign,
    LevelUp,
    JobLevelUp,
    RefineSuccess,
    RefineFail,
    CardInsert,
    ZenyTransaction
};
```

---

## 9. Phase Plan

### Phase 1: BGM System (Zone-Based Music Switching)
**Priority: HIGH | Effort: Medium | Impact: HIGH**

Tasks:
- [ ] Create `BGMSubsystem` (UWorldSubsystem) with play/stop/crossfade
- [ ] Create `AudioData.cpp` with zone-to-BGM mapping table
- [ ] Set up Sound Class hierarchy (SC_Master, SC_BGM, SC_SFX, SC_Ambient, SC_UI)
- [ ] Set up Sound Mix presets (SM_Default, SM_Menu, SM_Loading)
- [ ] Integrate with `ZoneTransitionSubsystem` to trigger BGM on zone change
- [ ] Source/create placeholder BGM for each implemented zone (prt_south at minimum)
- [ ] Add BGM volume control to game settings
- [ ] Add `/bgm` toggle support
- [ ] Test crossfade between zones
- [ ] Test same-BGM zone transitions (music continues)
- [ ] Store volume preferences in GameUserSettings.ini

### Phase 2: Core Combat SFX (Hit, Miss, Critical, Death)
**Priority: HIGH | Effort: Medium | Impact: HIGH**

Tasks:
- [ ] Create `SFXSubsystem` (UWorldSubsystem)
- [ ] Source/create base combat sounds:
  - Normal hit (3 variants)
  - Critical hit (2 variants)
  - Miss/dodge (2 variants)
  - Player death (male + female)
- [ ] Create Sound Cues with random variant selection + pitch variation
- [ ] Set up attenuation profiles (SA_Combat_Close, SA_Combat_Ranged)
- [ ] Set up concurrency settings (SCon_WeaponSwings, SCon_SkillImpacts)
- [ ] Integrate with `DamageNumberSubsystem` (play hit sound on `combat:damage`)
- [ ] Integrate with `SkillVFXSubsystem` (play impact on `skill:effect_damage`)
- [ ] Test with multiple concurrent combatants

### Phase 3: Skill SFX (Per Skill Category)
**Priority: HIGH | Effort: High | Impact: HIGH**

Tasks:
- [ ] Build skill-to-SFX mapping in `AudioData.cpp`
- [ ] Source/create sounds for each implemented skill:
  - Fire skills (Fire Bolt, Fire Ball, Fire Wall)
  - Ice skills (Cold Bolt, Frost Diver)
  - Lightning skills (Lightning Bolt, Thunderstorm)
  - Holy skills (Heal, Blessing, Increase AGI)
  - Ghost skills (Napalm Beat, Soul Strike)
  - Earth skills (Stone Curse)
  - Buff application (generic positive aura)
  - Debuff application (generic negative aura)
- [ ] Integrate cast-start sounds with `CastBarSubsystem`
- [ ] Integrate impact sounds with `SkillVFXSubsystem` spawn events
- [ ] Handle looping sounds (Fire Wall crackling) with UAudioComponent lifecycle
- [ ] Handle multi-hit sounds (bolt skills, Thunderstorm) with staggered playback
- [ ] Test concurrency with multiple skills firing simultaneously

### Phase 4: UI SFX (Menus, Inventory, Chat)
**Priority: MEDIUM | Effort: Low | Impact: MEDIUM**

Tasks:
- [ ] Source/create UI sound set:
  - Button click
  - Menu open/close
  - Item pickup/drop
  - Equip/unequip
  - Inventory full / error
  - Chat notification
  - Whisper notification
  - Level up fanfare
  - Job level up fanfare
- [ ] Create EUISoundType enum and mapping
- [ ] Integrate with `InventorySubsystem` (pickup, equip, drop, error)
- [ ] Integrate with `HotbarSubsystem` (skill activation)
- [ ] Integrate with chat system (message received, whisper)
- [ ] Integrate with `BasicInfoSubsystem` (level up detection)
- [ ] All UI sounds play as 2D (non-spatialized, `PlaySound2D`)

### Phase 5: Environment Ambient (Per Zone Type)
**Priority: MEDIUM | Effort: Medium | Impact: MEDIUM**

Tasks:
- [ ] Create `AmbientAudioSubsystem` (UWorldSubsystem)
- [ ] Define ambient presets per zone type:
  - Town: crowd murmur, distant commerce
  - Field: wind, birds, rustling
  - Forest: dense bird calls, leaves, insects
  - Desert: dry wind, sand
  - Dungeon: dripping, distant groans, echo
  - Snow: wind, crunch ambient
  - Beach: waves, seagulls
- [ ] Place ambient sound sources in levels using Blueprint actors
- [ ] Set up reverb volumes per zone type
- [ ] Source/create ambient sound loops
- [ ] Integrate with zone transitions (ambient starts with zone load)

### Phase 6: 3D Spatialization and Polish
**Priority: MEDIUM | Effort: Medium | Impact: MEDIUM**

Tasks:
- [ ] Fine-tune attenuation curves for all sound categories
- [ ] Add reverb zones to all dungeon levels
- [ ] Implement sound occlusion for indoor/outdoor transitions
- [ ] Add distance-based culling (don't process sounds beyond max range)
- [ ] Test audio performance with many concurrent entities
- [ ] Optimize concurrency limits based on real gameplay scenarios
- [ ] Add audio LOD (simplified sounds at distance)
- [ ] Polish crossfade timing and volume curves

### Phase 7: Monster-Specific Sounds
**Priority: LOW | Effort: High | Impact: MEDIUM**

Tasks:
- [ ] Build monster-to-SFX mapping in `AudioData.cpp`
- [ ] Source/create per-monster sounds for implemented monsters:
  - Idle (ambient creature sounds)
  - Attack (per-monster attack sound)
  - Damage (per-monster hurt sound)
  - Death (per-monster death sound)
- [ ] Integrate with enemy AI system (idle sound timer, attack/damage/death events)
- [ ] Start with zone 1-3 monsters, expand as new zones are added
- [ ] Test idle sound interval and concurrency (don't hear 50 porings at once)

### Phase 8: Voice / Emote Sounds
**Priority: LOW | Effort: Low-Medium | Impact: LOW**

Tasks:
- [ ] Source/create gendered hurt/death sounds (male + female)
- [ ] Source/create Kafra NPC greeting
- [ ] Add emote sounds (laugh, cry, etc.)
- [ ] Add skill vocal effects for advanced classes (if implemented)
- [ ] Consider AI voice generation for NPC greetings
- [ ] Integrate with player character damage events
- [ ] Integrate with KafraNPC interaction

### Phase 9: Weapon-Type Audio Differentiation
**Priority: LOW | Effort: Medium | Impact: LOW**

Tasks:
- [ ] Create weapon type enum and mapping
- [ ] Source/create distinct swing sounds per weapon type (13+ types)
- [ ] Create Sound Cues per weapon with variant selection
- [ ] Integrate with auto-attack system (read equipped weapon type)
- [ ] Add weapon-specific hit sounds (sword clang vs mace thud)

### Phase 10: Footsteps and Terrain Audio
**Priority: LOW | Effort: Medium | Impact: LOW**

Tasks:
- [ ] Define terrain types per zone/material
- [ ] Source/create footstep sounds per terrain (grass, stone, wood, sand, snow, water)
- [ ] Create Sound Cues with variant selection per terrain type
- [ ] Implement terrain detection (physical material on ground surfaces)
- [ ] Integrate with character movement (play footstep on movement tick)
- [ ] Adjust footstep rate based on movement speed

---

## Appendix A: RO Effect ID Reference (Selected Audio-Related)

From the rAthena `effect_list.txt` (968 entries, IDs 0-967). These are client-side effect IDs that include audio components:

| Effect ID Range | Category | Description |
|----------------|----------|-------------|
| 0-10 | Basic Combat | Hit effects, bash, melee impacts |
| 11-25 | Elemental Casting | Casting auras by element (water, fire, wind, earth, holy, poison) |
| 26-50 | Buffs/Status | AGI/DEX boost, blessing, endure, status effects |
| 51-68 | Thief/Rogue | Stealing, hiding, envenom, grimtooth |
| 69-85 | Traps | Hunter traps (skid, blast mine, claymore, ankle snare) |
| 86-130 | Misc Skills | Cart revolution, loud exclamation, various class skills |
| 131-155 | Environment | Waterfalls, portals, map effects |
| 156-200 | Job/Level | Job change effects, level up, warp portal visual+audio |
| 200-300 | Advanced Skills | Meteor storm, storm gust, LoV, safety wall, etc. |
| 300-400 | Status Ailments | Poison, blind, freeze, burn visual+audio indicators |
| 400-600 | Second Classes | Knight, Priest, Wizard, Hunter advanced skill effects |
| 600-967 | Third Classes / Extended | Third class auras, special skill effects |

## Appendix B: Sound Design Notes for RO-Style Audio

### Characteristics of RO's Sound Design

1. **MIDI-Influenced BGM**: Early RO tracks had a MIDI/synthesizer quality even in their orchestral arrangements. This gives them a nostalgic, slightly "digital" warmth.

2. **Short, Punchy SFX**: Combat sounds are brief (under 0.5 seconds typically). They need to be impactful but not overstay their welcome, since attacks happen rapidly.

3. **Distinct Skill Identity**: Every skill sounds different enough that experienced players can identify skills by sound alone. This is critical for PvP awareness.

4. **Non-Intrusive Ambient**: The original client had very little environmental ambiance. Music carried the atmosphere. Our UE5 version can add ambiance but should keep it subtle to preserve the RO "feel."

5. **Celebratory Progression**: Level up and job level up sounds are among the most emotionally important sounds in the game. They should feel triumphant and rewarding.

6. **Cultural Music Diversity**: Each town's BGM reflects its cultural inspiration. This is a core part of RO's identity and should be preserved in new compositions.

---

## Research Sources

- [Ragnarok Online Complete Soundtrack (Internet Archive)](https://archive.org/details/ragnarok-online-complete-soundtrack)
- [BGM Files - iRO Wiki](https://irowiki.org/wiki/BGM_Files)
- [Sound Effects - iRO Wiki](https://irowiki.org/wiki/Sound_Effects)
- [Ragnarok Online Complete Soundtrack - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Ragnarok_Online_Complete_Soundtrack)
- [SoundTeMP - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/SoundTeMP)
- [Ragnarok Online Original Soundtrack (KHInsider)](https://downloads.khinsider.com/game-soundtracks/album/ragnarok-online-original-soundtrack)
- [ROenglishPRE mp3nametable.txt (GitHub)](https://github.com/zackdreaver/ROenglishPRE/blob/master/data/mp3nametable.txt)
- [rAthena Effect List (GitHub)](https://github.com/idathena/trunk/blob/master/doc/effect_list.txt)
- [Ragnarok Online Unused Sound Effects (The Cutting Room Floor)](https://tcrf.net/Ragnarok_Online/Unused_Sound_Effects)
- [Ragnarok Online Sound Files (The Phrozen Keep)](https://d2mods.info/forum/viewtopic.php?t=66802)
- [RagnarokFileFormats (GitHub)](https://github.com/rdw-archive/RagnarokFileFormats)
- [UE5 MetaSounds Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-in-unreal-engine)
- [UE5 Sound Concurrency Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-concurrency-reference-guide)
- [UE5 Audio Engine Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-engine-overview-in-unreal-engine)
- [Ragnarok Online BGM Collection (Internet Archive)](https://archive.org/details/121_20191019)
- [RAGNAROK Online Complete Soundtrack (VGMdb)](https://vgmdb.net/album/14011)
- [rAthena Mapflag Documentation (GitHub Wiki)](https://github.com/rathena/rathena/wiki/Mapflag)
