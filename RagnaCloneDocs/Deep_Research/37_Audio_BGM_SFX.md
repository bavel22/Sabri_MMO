# Audio, BGM & SFX -- Deep Research

> Comprehensive reference for replicating the Ragnarok Online Classic (pre-Renewal) audio experience.
> Sources: iRO Wiki, VGMdb, rAthena, The Cutting Room Floor, RateMyServer, Ragnarok Wiki (Fandom), Wikipedia, community forums.

---

## BGM (Background Music)

### Music Style and Composer (SoundTeMP)

**SoundTeMP** (Sound Team Production) is a South Korean video game music composer group formed in **1992**. They became famous through their work on Ragnarok Online beginning in 2002. SoundTeMP is responsible for the entire original RO soundtrack and is acclaimed for their genre diversity within a single game.

**Key Members:**

| Member | Role | Notes |
|--------|------|-------|
| Lee Seock-Jin ("BlueBlue") | Primary composer, leader | Passed away in 2015. Core creative force behind the RO soundtrack. |
| Sevin | Composer, sound director | Left SoundTeMP in 2004 due to creative differences. Founded Sound Fashion Advisor (S.F.A.). Currently sound director of Tree of Savior. |
| Park Jin-Bae ("ESTi") | Composer, arranger | Known for electronic/trance tracks. Active on YouTube with independent music. |
| Neocyon | Composer | Contributed field and dungeon tracks. |
| Warren | Composer | Contributed ambient and atmospheric pieces. |
| Workspace | Composer | Contributed town and event themes. |

**Musical Style:**

SoundTeMP's RO soundtrack spans an extraordinary range of genres, which is considered one of their defining strengths:

| Genre | Where Used | Examples |
|-------|-----------|----------|
| Orchestral / Medieval | Main towns (Prontera, Al De Baran) | Theme of Prontera (regal fanfare), Theme of Al De Baran (clockwork waltz) |
| Eastern Folk / Traditional | Cultural towns (Payon, Amatsu, Louyang) | Theme of Payon (Korean folk), Purity of Your Smile (Japanese koto/shamisen), The Great (Chinese erhu/guzheng) |
| Trance / Techno / Electronic | Dungeons, combat areas | Naive Rave (Bio Lab), TeMP it Up (Geffenia), TeMPorsche |
| Ambient / New Age | Fields, peaceful areas | Peaceful Forest, Streamside, I Miss You |
| Jazz / Latin / Swing | Beach towns, fun areas | High Roller Coaster (Comodo), Jazzy Funky Sweety (Umbala), Mucho Gusto |
| Dark Atmospheric / Industrial | Horror dungeons, dangerous zones | Fear... (Glast Heim), Curse'n Pain, Steel Me (Einbroch) |
| Christmas / Seasonal | Holiday event maps | White Christmas, Jingle Bell on Ragnarok, Silver Bell |
| Arabian / Middle Eastern | Morroc and desert areas | Theme of Morroc, Desert, Hamatan (Sphinx) |
| Celtic / Folk | Western-themed areas | March with Irish Whistle, Antique Cowboy |

**Official Releases:**

| Release | Year | Label | Catalog | Tracks | Notes |
|---------|------|-------|---------|--------|-------|
| Ragnarok Online Original Soundtrack | 2003 | Gravity | -- | 30 | First commercial release, core tracks |
| Ragnarok Online Complete Soundtrack | 2009 | Lantis | LACA-9163~7 | 141 | Definitive 5-disc box set, 345:29 total runtime |
| Ragnarok Online BGM Arrange Collection | 2012 | -- | -- | ~20 | Remixed/arranged versions of fan-favorite tracks |

The Complete Soundtrack (5 CDs) is the authoritative source, containing every BGM track used in the game up to the Renewal era.

---

### BGM Per Town (Complete Mapping)

The `mp3nametable.txt` file in the RO client data maps every map's RSW (Resource World) file to its BGM track. Format: `mapname.rsw#bgm\\XX.mp3#`

#### Major Towns

| Map Code | Town Name | BGM # | Track Name | Duration | Style |
|----------|-----------|-------|------------|----------|-------|
| `prontera` | Prontera (Capital) | 08 | Theme of Prontera | 4:35 | Regal orchestral, medieval fanfare, brass + strings |
| `prt_in` | Prontera Interiors | 08 | Theme of Prontera | -- | Same track continues |
| `prt_church` | Prontera Church | 10 | Divine Grace | 3:20 | Solemn organ, choir, holy atmosphere |
| `izlude` | Izlude | 26 | Everlasting Wanderers | 3:15 | Light orchestral, adventurous |
| `izlude_in` | Izlude Interiors | 26 | Everlasting Wanderers | -- | Same track continues |
| `geffen` | Geffen (Magic City) | 13 | Theme of Geffen | 3:01 | Mystical, ethereal synths, harp arpeggios |
| `geffen_in` | Geffen Interiors | 13 | Theme of Geffen | -- | Same track continues |
| `payon` | Payon (Archer Town) | 14 | Theme of Payon | 2:23 | Korean folk, gayageum, gentle flute |
| `payon_in01-03` | Payon Interiors | 14 | Theme of Payon | -- | Same track continues |
| `morocc` | Morroc (post-destruction) | 119 | Stained Memories | 3:40 | Somber, damaged, haunting echo |
| `morocc_in` | Morroc Interiors (original) | 11 | Theme of Morroc | 2:09 | Arabian, Middle-Eastern percussion, exotic |
| `alberta` | Alberta (Port Town) | 15 | Theme of Alberta | 3:22 | Coastal, sailing, light woodwinds |
| `alberta_in` | Alberta Interiors | 15 | Theme of Alberta | -- | Same track continues |
| `aldebaran` | Al De Baran | 39 | Theme of Al De Baran | 3:45 | Clockwork waltz, mechanical elegance |
| `aldeba_in` | Al De Baran Interiors | 39 | Theme of Al De Baran | -- | Same track continues |
| `comodo` | Comodo (Beach Town) | 62 | High Roller Coaster | 3:30 | Upbeat jazz/Latin, party atmosphere |
| `umbala` | Umbala (Jungle Town) | 68 | Jazzy Funky Sweety | 3:15 | Jazz funk, tribal rhythms |
| `yuno` | Juno / Yuno | 70 | Theme of Juno | 4:10 | Grand, elevated, celestial strings |
| `niflheim` | Niflheim (Land of Dead) | 84 | Christmas in the 13th Month | 3:25 | Eerie, dark carnival, music box distortion |
| `lutie` | Lutie (Christmas Town) | 59 | Theme of Lutie / Snow in My Heart | 3:55 | Christmas bells, gentle snow atmosphere |
| `amatsu` | Amatsu (Japanese) | 76 | Purity of Your Smile | 3:40 | Japanese koto, shamisen, bamboo flute |
| `louyang` | Louyang (Chinese) | 79 | The Great | 3:50 | Chinese erhu, guzheng, majestic |
| `hugel` | Hugel | 93 | Latinnova | 3:20 | Latin-influenced, pastoral |
| `rachel` | Rachel | 94 | Theme of Rachel | 4:00 | Sacred, Middle-Eastern blend, minor key |
| `veins` | Veins | 104 | On Your Way Back | 3:30 | Desert frontier, contemplative |
| `moscovia` | Moscovia | 114 | Theme of Moscovia | 3:45 | Russian folk, balalaika, accordion |

#### Industrial Cities (Schwarzwald Republic)

| Map Code | Town Name | BGM # | Track Name | Style |
|----------|-----------|-------|------------|-------|
| `einbroch` | Einbroch | 86 | Steel Me | Industrial, heavy machinery, dark brass |
| `einbech` | Einbech | 86 | Steel Me | Same track (sister city) |
| `lighthalzen` | Lighthalzen | 90 | Noblesse Oblige | Aristocratic, refined strings, corporate |
| `hugel` | Hugel | 93 | Latinnova | Pastoral Latin, country |

#### Satellite / Minor Towns

| Map Code | Town Name | BGM # | Track Name |
|----------|-----------|-------|------------|
| `prt_monk` | St. Capitolina Abbey | 10 | Divine Grace |
| `ayo_in01-02` | Ayothaya | 81 | Thai Orchid |
| `moc_castle` | Morroc Castle | 11 | Theme of Morroc |
| `jawaii` | Jawaii (Honeymoon Island) | 29 | Be Nice 'n Easy |

---

### BGM Per Dungeon

| Map Code | Dungeon Name | BGM # | Track Name | Style / Atmosphere |
|----------|-------------|-------|------------|-------------------|
| `prt_sewb1-4` | Prontera Culvert (Sewers) | 19 | Under the Ground | Dark, dripping, echoing corridors |
| `pay_dun00-04` | Payon Cave (5 floors) | 20 | Ancient Groover | Groovy but tense, undead rhythm |
| `gef_dun00-03` | Geffen Tower (4 floors) | 21 | Through the Tower | Ascending urgency, magical tension |
| `moc_pryd01-06` | Pyramid (6 floors) | 22 | Backattack!! | Percussive, trap awareness, ancient |
| `moc_ruins` | Morroc Ruins | 28 | You're in Ruins | Desolate, crumbling architecture |
| `iz_dun01-05` | Byalan Island (5 floors) | 49 | Watery Grave | Underwater, bubbling, aquatic pressure |
| `treasure01-02` | Sunken Ship | 17 | Treasure Hunter | Adventurous, pirate, treasure hunting |
| `alde_dun01-04` | Clock Tower (4 floors) | 60 | Aeon | Mechanical ticking, clockwork, time distortion |
| `c_tower1-4` | Clock Tower Basement | 61 | Zingaro | Darker clockwork, deeper mechanical |
| `gl_cas01-02` | Glast Heim Castle | 40 | Fear... | Ominous, dread, haunted silence broken by tension |
| `gl_church` | Glast Heim Church | 40 | Fear... | Same oppressive atmosphere |
| `gl_prison` | Glast Heim Prison | 42 | Curse'n Pain | Torturous, chains, suffering echoes |
| `gl_knt01-02` | Glast Heim Chivalry | 40 | Fear... | Corrupted knights' hall |
| `gl_sew01` | Glast Heim Sewers | 42 | Curse'n Pain | Underground corruption |
| `gl_step` | Glast Heim Staircase | 40 | Fear... | Transitional dread |
| `anthell01-02` | Ant Hell | 46 | An Ant-lion's Pit | Insectoid buzzing, underground colony |
| `beach_dun` | Comodo Beach Dungeon | 63 | Mucho Gusto | Beach-adjacent, lighter dungeon |
| `ama_dun01-03` | Amatsu Dungeon | 76 | Purity of Your Smile | Japanese theme continues underground |
| `lou_dun01-03` | Louyang Dungeon | 80 | Jumping Dragon | Chinese combat, martial arts energy |
| `ayo_dun01-02` | Ayothaya Dungeon | 82 | Muay Thai King | Thai martial, gamelan percussion |
| `ra_san01-05` | Rachel Sanctuary | 95 | Underneath the Temple | Sacred corruption, holy decay |
| `abbey01-03` | Nameless Island / Abbey | 113 | Monastery in Disguise | Monastic chant gone wrong, hidden evil |
| `tha_t01-06` | Thanatos Tower (lower) | 100 | Abyss | Deepening darkness, descent |
| `tha_t07-12` | Thanatos Tower (upper) | 102 | Erebos' Prelude | Climactic, boss approach, dread crescendo |
| `lhz_dun01-04` | Lighthalzen Bio Lab | 92 | Naive Rave | Trance/electronic, frenetic energy, experiment horror |
| `sphinx01-05` | Sphinx | 38 | Hamatan | Egyptian mystery, ancient tomb |
| `in_sphinx` | Inside Sphinx | 38 | Hamatan | Same ancient atmosphere |
| `gef_tower` | Geffen Tower (upper) | 43 | Morning Gloomy | Higher-level magical corruption |
| `gef_fild04` | Geffenia | 44 | TeMP it Up | Hidden paradise, trance/electronic |
| `nif_fild01-02` | Niflheim Fields | 85 | Dancing Christmas in the 13th Month | Dark carnival extended |
| `mjolnir_04-12` | Mjolnir Dead Pit | 27 | Dreamer's Dream | Deep underground, dreamlike |
| `ice_dun01-03` | Ice Cave (Rachel) | 96 | Yetit Petit | Frozen, crystalline, cold isolation |
| `abyss_01-03` | Abyss Lake | 100 | Abyss | Dragon territory, deep water |
| `odin_tem01-03` | Odin Temple | 102 | Erebos' Prelude | Norse divine ruins |
| `tur_dun01-06` | Turtle Island | 48 | Help Yourself | Mid-level challenge, turtle shells |

---

### BGM Per Field Area

| Map Code Pattern | Area Description | BGM # | Track Name |
|-----------------|-----------------|-------|------------|
| `prt_fild01` | Prontera South Field | 02 | Gambler of Highway |
| `prt_fild02-03` | Prontera West Fields | 04 | I Miss You |
| `prt_fild04-05` | Prontera Fields (misc) | 03 | Peaceful Forest |
| `prt_fild06-07` | Prontera East Fields | 05 | Tread on the Ground |
| `prt_fild08` | Prontera Waterside | 12 | Streamside |
| `moc_fild01-10` | Morroc Desert Fields | 24 | Desert |
| `moc_fild11-18` | Morroc Far Fields | 37 | TeMPorsche |
| `gef_fild01-03` | Geffen Fields (near) | 23 | Travel |
| `gef_fild04-06` | Geffen Fields (mid) | 04 | I Miss You |
| `gef_fild07-10` | Geffen Mountain Fields | 25 | Plateau |
| `gef_fild11-14` | Geffen Far Fields | 23 | Travel |
| `pay_fild01-06` | Payon Fields (near) | 14 | Theme of Payon |
| `pay_fild07-11` | Payon Far Fields | 36 | Nano East |
| `mjolnir_01-03` | Mt. Mjolnir (low) | 33 | Yuna Song |
| `mjolnir_04-06` | Mt. Mjolnir (mid) | 31 | Brassy Road |
| `mjolnir_07-12` | Mt. Mjolnir (high) | 34 | Pampas Upas |
| `yuno_fild01-06` | Juno Fields (near) | 70 | Theme of Juno |
| `yuno_fild07-12` | Juno Fields (far) | 73 | Higher Than the Sun |
| `ein_fild01-10` | Einbroch Fields | 87 | Ethnica |
| `lhz_fild01-03` | Lighthalzen Fields | 91 | CheongChoon |
| `hu_fild01-07` | Hugel Fields | 93 | Latinnova |
| `ra_fild01-13` | Rachel Fields | 94 | Theme of Rachel |
| `ve_fild01-07` | Veins Fields | 104 | On Your Way Back |
| `cmd_fild01-09` | Comodo Fields | 63 | Mucho Gusto |
| `um_fild01-04` | Umbala Fields | 68 | Jazzy Funky Sweety |
| `alde_fild01` | Al De Baran Fields | 39 | Theme of Al De Baran |
| `moc_desert` | Sograt Desert (deep) | 24 | Desert |
| `nif_fild01-02` | Niflheim Fields | 85 | Dancing Christmas in the 13th Month |
| `mos_fild01-02` | Moscovia Fields | 114 | Theme of Moscovia |

---

### Special BGM (Boss Fights, Events, Login Screen)

| Context | BGM # | Track Name | Notes |
|---------|-------|------------|-------|
| **Login Screen** | 01 | Title | Iconic opening, sets the tone for the entire game. Orchestral, grand. Plays during character select too. |
| **Character Select** | 01 | Title | Same as login -- continuous, no restart. |
| **PvP Arena (prep)** | 52 | Ready~ | Pre-battle anticipation, competitive energy. |
| **PvP Arena (active)** | 123 | Into the Arena! | Active PvP combat, intense. |
| **War of Emperium** | 06 | Risk Your Life | High-stakes guild warfare, driving rhythm. Also used for dangerous combat areas. |
| **WoE Castles** | 66 | Wanna Be Free!! | Castle interior during siege. |
| **Wedding Ceremony** | 09 | Great Honor | Ceremonial, grand, celebratory. |
| **Training Grounds** | 30 | One Step Closer | Tutorial/beginner area, encouraging. |
| **Novice Grounds** | 30 | One Step Closer | Same beginner encouragement. |
| **Christmas Events** | 53-58 | White Christmas through Jingle Bell on Ragnarok | 6 tracks for holiday maps. |
| **Silent Maps** | 997 | (Silence) | Special track = no BGM. Used for cutscenes or specific event maps. |
| **Airship** | 87 | Ethnica | International airship travel between continents. |

**Note on MVP/Boss fights:** RO Classic does NOT have a separate boss fight BGM. When fighting MVPs (boss monsters), the map's normal BGM continues playing. There is no dynamic music change on boss engage. The tension comes entirely from the existing dungeon atmosphere. This is a deliberate design choice -- the player is always "in the dungeon" and the boss is just another (much harder) enemy in that space.

---

### BGM Transition Mechanics

| Behavior | RO Classic Implementation | Sabri_MMO Enhancement |
|----------|--------------------------|----------------------|
| **Map Change (different BGM)** | Current BGM stops **abruptly**. New BGM starts from the **beginning**. No crossfade, no fade-out. Instant cut. | Add 1.0s crossfade (fade out old, fade in new) for smoother transitions. |
| **Map Change (same BGM)** | Music **continues uninterrupted**. Example: `prontera` -> `prt_in` both use BGM 08. No restart. | Match this behavior exactly -- detect same-track and skip transition. |
| **Looping** | All BGM tracks loop seamlessly from end back to beginning with zero gap. Infinite loop until map change or `/bgm` toggle. | Use UE5 `bLooping = true` on the UAudioComponent. |
| **Volume** | Player-adjustable via Sound Configuration menu. Separate BGM slider. Value persists in client config across sessions. | Store in `GameUserSettings.ini`, expose via settings UI. |
| **Toggle** | `/bgm` chat command toggles all background music on/off instantly. No fade. | Match behavior: instant on/off toggle via `/bgm` command. |
| **Priority** | BGM plays at a fixed volume layer beneath SFX. No dynamic ducking or sidechain compression. | Match: BGM is always behind SFX. Consider optional ducking for future polish. |
| **Death** | BGM continues playing during death screen. No special death music. | Match: no interruption on death. |
| **Loading Screen** | BGM stops during zone load. Silence while loading. New BGM starts when new map fully loads. | Match, or optionally continue old BGM during load for smoother feel. |

---

## SOUND EFFECTS

### Combat SFX (Weapon Hit by Type, Spell Casting, Skill Impacts)

#### Weapon Swing Sounds

Each weapon type produces a distinct swing/attack sound synchronized to the attack animation frame. The original RO client used the Miles Sound System (by RAD Game Tools) for WAV playback, stored in `data/wav/` within the `data.grf` archive.

| Weapon Type | Sound Characteristics | File Pattern | Attack Speed Feel |
|-------------|----------------------|-------------|-------------------|
| **Dagger** | Short, sharp metallic slash. Quick and light. | `_attack_dagger*.wav` | Very fast -- rapid repetition feels natural |
| **1H Sword** | Clean metallic sword swing, moderate weight | `_attack_sword*.wav` | Medium-fast, satisfying slash |
| **2H Sword** | Heavy, deep metallic swing with follow-through whoosh | `_attack_2hsword*.wav` | Slower, more impact per swing |
| **Axe** | Chopping impact, dull thud + wind | `_attack_axe*.wav` | Heavy, chunky |
| **Mace** | Solid blunt thud/crack, no slicing | `_attack_mace*.wav` | Weighty, bone-crushing |
| **Spear** | Piercing thrust, moderate weight | `_attack_spear*.wav` | Medium, stabbing rhythm |
| **Staff / Rod** | Light wooden bonk/tap | `_attack_staff*.wav` | Light, quick, unimposing |
| **Bow** | String release twang + arrow flight whoosh | `_attack_bow*.wav` | Distinctive launch sound, no impact at source |
| **Knuckle / Fist** | Fleshy punch impact, quick | `_attack_knuckle*.wav` | Rapid, martial arts feel |
| **Instrument (Lute)** | Musical strum or plucked note | `_attack_instrument*.wav` | Musical, unique |
| **Whip** | Sharp crack/snap | `_attack_whip*.wav` | Quick, snapping |
| **Book** | Paper/leather slam | `_attack_book*.wav` | Thuddy, unusual |
| **Katar** | Dual rapid metallic slashes (two hits in one) | `_attack_katar*.wav` | Fast double-slash |
| **Unarmed** | Punch/slap | `_attack_unarmed*.wav` | Light, basic |

#### Hit Impact Sounds

| Hit Type | Sound | File | Visual Companion |
|----------|-------|------|-----------------|
| **Normal Hit** | Standard impact thud. Multiple variants (3-5) for randomization to avoid repetitive feel. | `_hit1.wav` through `_hit5.wav` | White damage number |
| **Critical Hit** | Higher-pitched, more dramatic impact with a distinctive "crunch" element. Clearly different from normal hit. | `_hit_critical*.wav` or `ef_hit_critical.wav` | Yellow damage number + "Critical!" text |
| **Miss** | Quiet "whoosh" of air, sometimes silence. The visual "MISS" text is the primary feedback. | `_miss.wav` (very short) | "MISS" text popup |
| **Perfect Dodge** | Light evasion whoosh, slightly different from miss | Similar to miss variant | "MISS" text |
| **Block (Auto Guard / Shield)** | Metallic shield deflection clang | `ef_guard.wav` | "Block" text / guard animation |

**Variant randomization:** The original RO client cycles through multiple hit sound variants (e.g., `_hit1.wav`, `_hit2.wav`, `_hit3.wav`) to prevent the same sound playing repeatedly. This is critical for combat feel -- repetitive identical sounds become annoying quickly.

**Pitch variation:** A small random pitch shift (+/- 5%) is applied to each playback, further preventing repetition even when the same variant is selected.

---

### Skill-Specific SFX (Per Skill Sound Design)

RO Classic skills each have distinct audio cues triggered by the client-side effect system. The `effect_list.txt` file (documented in rAthena/Hercules) maps 387+ client-side visual and sound effects. Each effect ID can have an associated WAV file in `data/wav/effect/`.

#### First Class Skills

**Swordsman (IDs 100-109):**

| Skill | Cast Sound | Impact Sound | Element |
|-------|-----------|-------------|---------|
| Bash (101) | -- | Heavy weapon impact, amplified hit | Physical |
| Magnum Break (102) | Fire charge buildup | Fire explosion burst, area push whoosh | Fire |
| Endure (103) | -- | Hardening/fortification tone, stoic aura activation | -- |
| Provoke (104) | -- | Aggressive shout/taunt | -- |

**Mage (IDs 200-213):**

| Skill | Cast Sound | Impact Sound | Looping Sound | Element |
|-------|-----------|-------------|--------------|---------|
| Fire Bolt (19) | Crackling fire whoosh per bolt | Fireball launch + explosion | -- | Fire |
| Cold Bolt (14) | Ice crystal formation per bolt | Sharp frozen crack | -- | Water/Ice |
| Lightning Bolt (20) | Electric crackle per bolt | Thunder rumble | -- | Wind |
| Fire Ball (207) | Large fireball launch whoosh | Explosion on impact | -- | Fire |
| Fire Wall (209) | Fire ignition sequence | -- | Sustained crackling fire loop | Fire |
| Frost Diver (208) | Ice spreading sound | Crystallization freeze crack | -- | Water/Ice |
| Thunderstorm (212) | -- | Multiple thunder cracks, electrical rain | -- | Wind |
| Napalm Beat (206) | Dark energy charging | Ghostly burst/whoosh | -- | Ghost |
| Soul Strike (210) | Energy projectile launches | Arcane impact per hit | -- | Ghost |
| Stone Curse (209) | Rumbling earth | Petrification crackle | -- | Earth |
| Safety Wall (211) | Barrier activation chime | -- | Protective hum loop | Neutral |
| Sight (205) | Fire ignition ring | -- | Burning aura hum loop | Fire |

**Archer (IDs 300-306):**

| Skill | Sound Description |
|-------|-------------------|
| Double Strafe (301) | Rapid double arrow release -- twin twang with brief overlap |
| Arrow Shower (303) | Multiple arrow launches in spread + scattered ground impacts |
| Improve Concentration (304) | Focus aura hum, heightened senses tone |
| Owl's Eye (300) | Passive -- no sound |
| Vulture's Eye (301) | Passive -- no sound |
| Arrow Crafting (305) | Crafting/assembly sound |

**Acolyte (IDs 400-414):**

| Skill | Sound Description | Character |
|-------|-------------------|-----------|
| Heal (28) | Gentle ascending chime, warm bell tone. One of the most-heard sounds in the game. | Warm, reassuring |
| Blessing (34) | Angelic chord, ethereal ascending notes | Grand, holy |
| Increase AGI (29) | Quick upward whoosh, speed aura | Energizing |
| Decrease AGI (30) | Downward swoosh, slowing effect | Debilitating |
| Cure (32) | Cleansing chime, purification tone | Cleansing |
| Angelus (35) | Protective aura, shield formation | Defensive |
| Pneuma (36) | Wind barrier, deflection field | Airy |
| Ruwach (37) | Revealing light burst, detection pulse | Holy flash |
| Teleport (38) | Spatial distortion, warp sound | Displacement |
| Warp Portal (39) | Portal opening whoosh, sustained hum | Spatial rift |

**Thief (IDs 500-509):**

| Skill | Sound Description |
|-------|-------------------|
| Double Attack (500) | Rapid twin slash, quick metallic impacts |
| Steal (502) | Quick snatching/grabbing sound |
| Hiding (503) | Fading/vanishing sound, stealth activation |
| Envenom (504) | Poison application, dripping venom |
| Detoxify (505) | Antidote application, cleansing |
| Sand Attack (506) | Sand throw whoosh, gritting |

**Merchant (IDs 600-609):**

| Skill | Sound Description |
|-------|-------------------|
| Mammonite (601) | Gold coin toss jingling + heavy weapon impact |
| Discount / Overcharge | Passive -- no combat sound |
| Cart Revolution (606) | Cart swing, heavy wooden impact + knockback |
| Vending (609) | Shop opening chime |

#### Second Class Skills

**Knight (IDs 700-710):**

| Skill | Sound Description |
|-------|-------------------|
| Two-Hand Quicken (705) | Rapid speed-up aura, weapon hum |
| Bowling Bash (706) | Heavy spinning impact, multi-hit cascade whoosh |
| Brandish Spear (707) | Wide arc sweep, multiple slash impacts in sequence |
| Pierce (703) | Rapid triple thrust, piercing metallic |
| Spear Boomerang (704) | Spear launch whoosh + arc + return whoosh |
| Charge Attack (710) | Rush forward whoosh + heavy impact |
| Counter Attack (709) | Counter-stance activation + retaliatory strike |

**Wizard (IDs 800-813):**

| Skill | Sound Description |
|-------|-------------------|
| Meteor Storm (806) | Deep rumbling, explosive meteor impacts, screen-shaking bass |
| Storm Gust (807) | Howling blizzard wind, ice crystal shattering, sustained |
| Lord of Vermilion (808) | Massive thunder cascade, sustained electrical barrage |
| Jupitel Thunder (809) | Concentrated lightning ball, electric buzz, knockback whoosh |
| Water Ball (810) | Splashing water burst, liquid impact |
| Ice Wall (811) | Ice barrier formation, crystalline wall |
| Frost Nova (812) | Freezing explosion outward, ice spreading |
| Quagmire (813) | Mud/swamp bubbling, squelching |
| Sight Rasher/Blaster (803/804) | Fire burst outward, aggressive ignition |
| Earth Spike (800) | Ground eruption, stone piercing upward |
| Heaven's Drive (801) | Ground tremor, earth pillars rising |

**Priest (IDs 1000-1018):**

| Skill | Sound Description |
|-------|-------------------|
| Kyrie Eleison (1007) | Protective barrier activation, crystalline shield |
| Magnificat (1008) | Deep church bell, resonant holy chime |
| Gloria (1010) | Grand angelic chord, heavenly light |
| Resurrection (1009) | Dramatic rising tone, rebirth fanfare |
| Sanctuary (1012) | Sustained holy ground hum, healing aura |
| Magnus Exorcismus (1013) | Grand cross formation, holy light burst, dramatic |
| Turn Undead (1011) | Holy flash, purification burst |
| Lex Aeterna (1014) | Ethereal marking sound, faint holy echo |
| Lex Divina (1015) | Silencing seal, muting effect |
| Aspersio (1016) | Holy water blessing, crystal splash |
| Suffragium (1017) | Cast time reduction buff sound |
| Impositio Manus (1018) | Blessing hands, power infusion |

**Hunter (IDs 900-917):**

| Skill | Sound Description |
|-------|-------------------|
| Ankle Snare (907) | Trap deployment click, spring activation |
| Blast Mine (908) | Bomb placement ticking, explosion |
| Claymore Trap (909) | Heavy explosive placement, massive detonation |
| Skid Trap (910) | Slippery surface deployment |
| Freezing Trap (911) | Ice trap activation, freezing burst |
| Sandman (912) | Sleep dust release, drowsy tone |
| Land Mine (913) | Ground mine deployment, earth explosion |
| Flasher (914) | Blinding light burst |
| Falcon Assault/Blitz Beat (914/916) | Falcon screech, diving attack whoosh, rapid pecking |

**Assassin (IDs 1100-1111):**

| Skill | Sound Description |
|-------|-------------------|
| Sonic Blow (1107) | Rapid multi-hit slash flurry, intense speed, 8 rapid cuts |
| Venom Dust (1104) | Poison cloud release, toxic hiss |
| Grimtooth (1105) | Underground attack burst, surprise claw strike |
| Cloaking (1106) | Invisibility activation, shimmer/fade |
| Enchant Poison (1103) | Weapon poison coating, dripping application |
| Katar Mastery | Passive -- no sound |
| Sonic Acceleration | Passive -- no sound |

**Crusader (IDs 1300-1313):**

| Skill | Sound Description |
|-------|-------------------|
| Grand Cross (1301) | Massive holy cross explosion, dramatic burst, one of the game's most dramatic SFX |
| Shield Charge (1305) | Shield bash impact, rushing forward |
| Shield Boomerang (1306) | Shield launch + spinning arc + return |
| Auto Guard (1307) | Shield raise, defensive stance |
| Devotion (1310) | Link/tether formation, protective chord between characters |
| Holy Cross (1300) | Holy-infused double strike |

**Bard (IDs 1500-1537) / Dancer (IDs 1520-1557):**

| Skill | Sound Description |
|-------|-------------------|
| Musical Strike (1500) | Instrument attack, musical note impact |
| Slinging Arrow (1520) | Whip attack, arrow-like release |
| All Performance Skills | Background music-like loop while active, distinct per song |
| Frost Joker/Scream (1504/1524) | Sudden burst, area effect |
| Pang Voice/Charming Wink (1505/1525) | Voice/charm effect, brief distinctive sound |

**Blacksmith (IDs 1200-1230):**

| Skill | Sound Description |
|-------|-------------------|
| Adrenaline Rush (1203) | Speed buff, forge hammer rhythm |
| Power Thrust (1204) | Power buildup, strengthening |
| Weapon Forge (1210+) | Hammer on anvil, metalworking sequence |
| Maximize Power (1207) | Maximum force activation |
| Hammer Fall (1209) | Heavy hammer ground slam, stun impact |

**Sage (IDs 1400-1421):**

| Skill | Sound Description |
|-------|-------------------|
| Volcano / Deluge / Whirlwind (1407-1409) | Elemental zone creation, sustained elemental ambient |
| Dispell (1406) | Magic stripping, dispersion burst |
| Magic Rod (1405) | Absorption sound, magic intake |
| Land Protector (1410) | Protective zone creation, barrier hum |
| Hindsight (1404) | Auto-cast activation, magical readiness |

**Monk (IDs 1600-1615):**

| Skill | Sound Description |
|-------|-------------------|
| Asura Strike (1605) | Massive ki explosion, the most dramatic single-hit sound. Battle cry + devastating impact. Screen-shaking. |
| Occult Impaction (1606) | DEF-piercing strike, spiritual force |
| Steel Body (1614) | Body hardening, iron transformation |
| Triple Attack (1600) | Three rapid combo hits |
| Chain Combo (1607) | Follow-up combo strikes |
| Combo Finish (1608) | Final combo blow, finishing impact |
| Finger Offensive (1613) | Spirit sphere launch, energy projectile |

**Rogue (IDs 1700-1718):**

| Skill | Sound Description |
|-------|-------------------|
| Backstab (1706) | Sneaky approach + amplified critical backstrike |
| Raid (1707) | Area burst from hiding, surprise attack |
| Intimidate (1708) | Forceful strike + teleport displacement |
| Divest (strip skills) | Equipment removal, stripping sounds (distinct per slot) |
| Close Confine (1717) | Binding/trapping sound, restriction lock |

**Alchemist (IDs 1800-1815):**

| Skill | Sound Description |
|-------|-------------------|
| Acid Terror (1801) | Acid splash, corrosive hissing, burning |
| Demonstration (1802) | Bomb throw, fire ground explosion |
| Potion Pitcher (1803) | Potion throw arc, bottle splash on target |
| Chemical Protection (1804-1807) | Protective coating application, chemical buff |
| Pharmacy (1808) | Mixing/brewing sounds, potion creation |
| Homunculus Call/Rest | Summoning/dismissal magical sound |

#### Casting Sounds (Universal)

| Cast Event | Sound | Notes |
|-----------|-------|-------|
| **Cast Start** | Magical charging hum, ascending tone that builds during cast bar | Pitch/intensity increases as cast progresses |
| **Cast Complete** | Release burst, energy discharge | Marks the moment the skill fires |
| **Cast Interrupted** | Fizzle/disruption sound, descending failed tone | Indicates cast was broken by damage |
| **Cast Bar Progress** | Subtle ticking/charging at intervals during cast | Optional ambient feedback |

---

### UI SFX (Button Click, Window Open/Close, Item Equip/Use)

All UI sounds are **2D (non-spatial)** -- they play at consistent volume regardless of camera/character position. They are part of the SFX volume control, not BGM.

| UI Action | Sound Description | Duration | Priority |
|-----------|-------------------|----------|----------|
| **Button Click** | Crisp, short click/tap. Clean. | ~50ms | Low |
| **Menu/Window Open** | Clean UI pop, light whoosh upward | ~100ms | Low |
| **Menu/Window Close** | Reverse pop, light whoosh downward | ~100ms | Low |
| **Tab Switch** | Subtle click, lighter than button | ~30ms | Low |
| **Item Pickup** | Quick grab + slight chime, rewarding | ~150ms | Medium |
| **Item Drop** | Thud/drop sound, loss feedback | ~100ms | Low |
| **Item Equip** | Metallic clip/attachment, gear-specific | ~200ms | Medium |
| **Item Unequip** | Reverse attachment, removal | ~150ms | Low |
| **Item Use (Consumable)** | Drinking/eating sound, potion gulp | ~300ms | Medium |
| **Inventory Full** | Error buzz, denied | ~200ms | High |
| **Error / Denied Action** | Negative beep/buzz | ~200ms | High |
| **Chat Message Received** | Subtle notification chime | ~100ms | Low |
| **Whisper Received** | Distinct private message alert, higher pitch than public chat | ~200ms | High |
| **Trade Window Open** | Transaction initiation, formal sound | ~200ms | Medium |
| **Trade Complete** | Successful exchange chime, coins | ~300ms | Medium |
| **Trade Cancel** | Cancel/close sound | ~150ms | Low |
| **NPC Dialog Open** | Dialog window appearance | ~150ms | Medium |
| **NPC Dialog Advance** | Text advancement click, page turn | ~50ms | Low |
| **NPC Dialog Close** | Window close | ~100ms | Low |
| **Skill Drag to Hotbar** | Slot assignment click, placement | ~100ms | Low |
| **Hotbar Skill Activate** | Quick activation feedback, key press confirmation | ~50ms | Low |
| **Zeny Transaction** | Coin counting/jingling | ~300ms | Medium |
| **Card Insert (Compound)** | Card slot activation, magical compound sound | ~400ms | High |
| **Refine Success** | Triumphant chime, upgrade fanfare, ascending | ~500ms | High |
| **Refine Failure** | Breaking/shattering sound, negative descending tone | ~500ms | High |
| **Skill Point Applied** | Confirm/assignment click | ~100ms | Low |
| **Stat Point Applied** | Similar confirm click | ~100ms | Low |
| **Screenshot** | Camera shutter click | ~100ms | Low |

---

### Notification SFX (Level Up, Quest Complete, Death, Chat Message)

| Event | Sound Description | Duration | Spatial | Priority |
|-------|-------------------|----------|---------|----------|
| **Base Level Up** | Triumphant ascending fanfare with chimes. One of the most iconic and recognizable sounds in RO. Bright, celebratory, satisfying. Immediately communicates achievement. | ~2.0s | 2D (self) | **Highest** -- always plays, never culled |
| **Job Level Up** | Similar fanfare but shorter and slightly different tone. Distinct enough that experienced players can tell which type of level up occurred by sound alone. | ~1.5s | 2D (self) | **Highest** |
| **Other Player Level Up** | Same level up sound but spatialized (3D) from their position, at reduced volume. Players near someone leveling up hear it. | ~2.0s | 3D (other) | High |
| **Player Death** | Collapse/falling body sound. Gendered (male grunt vs female cry). Combined with a brief dramatic sting. | ~1.0s | 3D | High |
| **Respawn** | Revival/awakening sound | ~0.5s | 2D | Medium |
| **Chat Message** | Subtle notification chime for public/guild/party chat | ~0.1s | 2D | Low |
| **Whisper** | Distinct alert, different from normal chat. More prominent to ensure private messages are noticed. | ~0.2s | 2D | High |
| **Party Invite** | Invitation notification, attention-getting | ~0.3s | 2D | High |
| **Trade Request** | Request notification | ~0.3s | 2D | High |
| **Disconnect Warning** | Warning tone | ~0.5s | 2D | Highest |
| **MVP Announcement** | Server-wide MVP kill announcement (chat-based, no special SFX in classic) | -- | -- | -- |

**Note on RO Classic:** There are no quest completion sounds in the pre-Renewal client. Quest tracking was minimal. The main progression sounds are Base Level Up and Job Level Up.

---

### Ambient SFX (Environmental: Wind, Water, Birds, Dungeon Echoes)

**Important context:** The original RO Classic 2D client had **minimal environmental ambient audio**. Most atmosphere came from the BGM tracks themselves. The 2D sprite engine did not have sophisticated spatial audio. For our UE5 implementation, we add rich ambient layers that RO's 2D engine could never achieve, while staying true to the game's atmospheric intent.

#### Town Ambience

| Town Type | Ambient Sounds | Implementation |
|-----------|---------------|----------------|
| **Large Towns** (Prontera, Geffen) | Crowd murmur, footsteps, distant marketplace chatter, birds | AmbientSound actors + Audio Volumes |
| **Port Towns** (Alberta, Izlude) | Seagulls, lapping waves, dock creaking, rope sounds | 3D point sources at docks |
| **Cultural Towns** (Payon, Amatsu, Louyang) | Cultural ambient -- wind chimes (Payon), temple bells (Amatsu), market bustle (Louyang) | Zone-specific ambient layers |
| **Industrial** (Einbroch) | Machinery hum, steam vents, hammering, factory noise | Heavy ambient + point sources |
| **Beach** (Comodo) | Ocean waves, tropical birds, palm rustling | Directional ocean source |
| **Christmas** (Lutie) | Wind, snow crunch, distant bells, fireplace crackle | Seasonal ambient |
| **Dead** (Niflheim) | Eerie silence broken by whispers, ghostly wind, chains | Sparse, unsettling |

#### Field Ambience

| Terrain Type | Ambient Sounds |
|-------------|---------------|
| **Grassland** | Wind through grass, bird calls, insects, distant animals |
| **Forest** | Rustling leaves, bird songs, wood creaking, stream sounds |
| **Mountain** | Strong wind, echoing, rock shifting, altitude |
| **Desert** | Dry wind, sand shifting, heat shimmer (subtle low hum), distant hawk |
| **Snow** | Wind, snow settling, muffled silence, ice creaking |
| **Waterside** | Running water, babbling brook, frogs, dragonflies |
| **Coastal** | Waves, seabirds, salt wind |

#### Dungeon Ambience

| Dungeon Type | Ambient Sounds | Reverb Profile |
|-------------|---------------|----------------|
| **Sewer/Culvert** | Water dripping (echoed), running water, rats, pipe resonance | Tunnel echo (medium decay, distinct reflection) |
| **Cave** | Deep dripping, stone shifting, distant growls, wind through passages | Cave reverb (long decay, high wet) |
| **Tower** | Wind through windows, stone creaking, magical hum | Stone room (medium decay) |
| **Pyramid/Tomb** | Sand falling, ancient mechanisms, muffled air, tomb silence | Tomb (long decay, muffled) |
| **Underwater** (Byalan) | Bubbles, water pressure, muffled everything, current | Heavy low-pass filter, underwater reverb |
| **Haunted** (Glast Heim) | Ghostly whispers, chains dragging, distant screams, door creaks | Large haunted hall (very long decay) |
| **Industrial** (Clock Tower) | Clockwork ticking, gear grinding, steam, mechanical rhythm | Mechanical room (medium, rhythmic) |
| **Ice** (Ice Cave) | Ice cracking, frozen wind, crystal resonance | Ice cave (bright reflections) |
| **Lava/Volcanic** | Rumbling, lava bubbling, heat, distant explosions | Cavern (long decay, low rumble) |

#### Specific Ambient Sources (3D Point Sources)

| Source | Sound | Attenuation Range | Looping |
|--------|-------|--------------------|---------|
| Warp Portal | Sustained magical hum/whoosh, pulsating energy | Inner 100, Outer 800 | Yes |
| Fountain | Water splashing, trickling | Inner 100, Outer 600 | Yes |
| Forge / Anvil | Hammering, metalworking | Inner 100, Outer 500 | Yes |
| Fireplace / Torch | Crackling fire | Inner 50, Outer 400 | Yes |
| Waterfall | Rushing water, spray | Inner 200, Outer 1500 | Yes |
| Stream / River | Running water, babbling | Inner 100, Outer 800 | Yes |
| NPC Kafra | "Welcome!" voice, ambient presence | Inner 100, Outer 500 | No (on interact) |

#### Footstep Sounds (Terrain-Based)

| Surface | Sound | Trigger | Notes |
|---------|-------|---------|-------|
| Grass | Soft rustling, light step | Movement on grass terrain | Default outdoor surface |
| Stone | Hard click, sharp step | Movement on stone/tile | Towns, dungeons |
| Wood | Hollow wooden thud | Movement on wooden floors | Building interiors |
| Sand | Crunching, shifting grit | Desert terrain | Morroc areas |
| Snow | Crunching, compressing | Snow terrain | Lutie, ice maps |
| Water (shallow) | Splashing, light wade | Water-covered terrain | Streams, shallow areas |
| Dirt | Soft thud, earthy | Packed earth paths | Fields, trails |
| Metal | Metallic ring, clang | Metal surfaces | Clock Tower, industrial |

**Note:** Original RO had minimal footstep audio due to its 2D isometric nature. Footsteps are an enhancement for our 3D UE5 implementation.

---

### Monster SFX (Aggro Sound, Attack, Death)

Each monster in RO has up to 5 animation states with associated audio. Sounds are stored in `data/wav/monster/{monster_sprite_name}/` within the GRF archive. The ACT (Animation Collection) sprite file contains keyframe data specifying which sound effect plays at which animation frame, ensuring sound-to-animation synchronization.

| Animation State | Sound Trigger | Coverage |
|-----------------|---------------|----------|
| **Idle** | Monster standing/breathing ambient | ~40% of monsters (many are silent in idle) |
| **Walk/Chase** | Monster moving/pursuing | ~20% (often absent or same as idle) |
| **Attack** | Monster executes attack hit | ~90% (most monsters have attack sounds) |
| **Damage/Hit** | Monster takes damage | ~60% (pain reactions) |
| **Death** | Monster is killed | ~85% (death cries/collapse) |

#### Iconic Monster Sounds

| Monster | Idle | Attack | Death | Audio Character |
|---------|------|--------|-------|-----------------|
| **Poring** | Soft bouncing/jiggling | Light slap/bump | Pop/burst (iconic!) | Cheerful jelly blob |
| **Drops** | Similar to Poring | Slap | Pop (pitch-shifted) | Poring variant |
| **Lunatic** | Quiet rustling | Quick scratch | Squeak | Small fuzzy rodent |
| **Fabre** | Soft chittering | Tiny bite | Squish | Small insect |
| **Rocker** | Cricket chirping | String strum/musical note | Musical crash | Cricket with instrument |
| **Willow** | Rustling leaves | Branch whip/swing | Timber crack/splintering | Animated tree |
| **Chonchon / Hunter Fly** | Buzzing/droning | Sting/dive | Crunch/squash | Flying insect |
| **Skeleton** | Bone rattling | Sword slash (metallic) | Bone collapse/scatter | Undead warrior |
| **Zombie** | Groaning/moaning | Claw swipe | Collapse/thud | Shambling undead |
| **Archer Skeleton** | Bone rattling | Bow twang + arrow | Bone collapse | Undead archer |
| **Orc Warrior** | Grunting | Heavy weapon swing | Death grunt | Large humanoid |
| **Goblin** | Snickering/chattering | Quick weapon strike | Shrill death cry | Small aggressive |
| **Argiope** | Skittering/hissing | Poison bite | Squish | Giant spider |
| **Raydric** | Armor clanking | Heavy sword slash | Armor collapse | Haunted armor |
| **Baphomet** | Deep demonic growl | Massive slash/ground slam | Extended roar + collapse | MVP demon lord |
| **Moonlight Flower** | Mystical hum | Magic burst/fox cry | Ethereal fade | MVP fox spirit |
| **Golden Bug** | Intense buzzing | Sting attack | Crunch/pop | MVP insect |
| **Drake** | Ghost ship creaking | Cutlass slash | Spectral wail/moan | MVP undead pirate |
| **Eddga** | Tiger growl/rumble | Claw swipe + roar | Extended roar | MVP tiger |
| **Mistress** | Queen bee hum | Magic burst + sting | Dramatic buzz-out | MVP bee queen |
| **Maya** | Chitin clicking | Mandible crush | Chitin shattering | MVP insect queen |
| **Phreeoni** | Deep ground rumble | Earth strike | Ground collapse | MVP earth worm |
| **Osiris** | Ancient whisper/chant | Dark magic burst | Dramatic ancient dispersal | MVP mummy lord |

**Monster Sound Coverage Note:** Not all monsters have sounds for every animation state. The Cutting Room Floor documents that some RO sounds were removed or merged -- sounds considered distracting with high-frequency skill spam were sometimes removed. Some monster sounds were re-used across similar monster types (e.g., all skeleton variants share base sounds).

---

## AUDIO SYSTEM DESIGN

### Per-Map BGM Assignment

The `mp3nametable.txt` file is the core data structure for BGM assignment. Format:

```
mapname.rsw#bgm\\XX.mp3#
```

**Rules:**
1. Each map RSW file maps to exactly one BGM track number
2. Interior maps of a town use the same BGM as the exterior (e.g., `prt_in` -> `08.mp3` same as `prontera`)
3. Multi-floor dungeons typically share one BGM across all floors (e.g., `pay_dun00` through `pay_dun04` all use `20.mp3`)
4. Exceptions exist: Thanatos Tower uses different BGM for lower vs upper floors
5. Track `997.mp3` is the "silence" track -- used for maps that intentionally have no music

**For Sabri_MMO:** Replicate this with a C++ `TMap<FString, FZoneBGMConfig>` lookup in `AudioData.cpp`, keyed by zone name.

---

### Volume Controls (BGM, SFX, Separate Sliders)

| Control | RO Classic | Sabri_MMO Target |
|---------|-----------|-----------------|
| **BGM Volume** | Slider in Sound Configuration menu (0-100) | Float slider 0.0-1.0 in settings UI |
| **SFX Volume** | Slider in Sound Configuration menu (0-100) | Float slider 0.0-1.0 in settings UI |
| **BGM Toggle** | `/bgm` chat command (on/off) | `/bgm` command support + settings checkbox |
| **SFX Toggle** | `/sound` chat command (on/off) | `/sound` command support + settings checkbox |
| **Persistence** | Values saved in client config file, persist across sessions | `GameUserSettings.ini` persistence |
| **Separate Control** | BGM and SFX are fully independent. Turning off BGM has no effect on SFX, and vice versa. | Match exactly. |

**Sound Class Hierarchy for Volume Control:**

```
SC_Master (root -- master volume)
  |
  +-- SC_BGM                    # Background music (player BGM slider)
  |
  +-- SC_SFX                    # All sound effects (player SFX slider)
  |     |
  |     +-- SC_Combat           # Weapon swings, hits, skill impacts
  |     +-- SC_Skills           # Skill casting and effect sounds
  |     +-- SC_Monster          # Monster idle, attack, death
  |     +-- SC_Progression      # Level up, job level up
  |
  +-- SC_Ambient                # Environmental ambiance (tied to SFX slider)
  |     |
  |     +-- SC_AmbientLoop      # Looping ambient (water, wind, crowd)
  |     +-- SC_Footsteps        # Terrain-based footsteps
  |
  +-- SC_UI                     # UI feedback sounds (tied to SFX slider)
  |
  +-- SC_Voice                  # Voice / character sounds (tied to SFX slider)
```

---

### Sound Priority System (Which Sounds Play When Many Trigger)

RO Classic uses the Miles Sound System's built-in voice management for sound priority. In a large-scale WoE or grinding scenario, dozens of sounds could trigger simultaneously. The system must decide which to play.

**Concurrency Limits and Resolution:**

| Sound Group | Max Concurrent | Resolution Rule | Rationale |
|-------------|---------------|-----------------|-----------|
| **BGM** | 1 | Newest replaces oldest | Only one BGM at a time, always |
| **Level Up** | 1 | Always plays, never culled | Players must NEVER miss their own level up sound |
| **Skill Impacts** | 8 | Lowest volume stops | Allow variety in multi-skill combat |
| **Weapon Swings** | 4 | Oldest stops | Prevent 50 simultaneous swings in WoE |
| **Monster Attack** | 6 | Oldest stops | Prioritize recent/nearby attacks |
| **Monster Death** | 4 | Oldest stops | Death sounds are brief, let them play |
| **Monster Idle** | 6 | Farthest stops | Only hear nearby monsters |
| **Ambient Loops** | 8 | Farthest stops | Environmental layers, nearby priority |
| **Footsteps** | 3 | Nearest plays | Only hear closest/own footsteps |
| **UI Sounds** | 3 | Newest replaces | Prevent UI sound spam |

**Priority Tiers (highest to lowest):**
1. **Critical Notifications:** Level Up, Death, Disconnect Warning
2. **Combat Feedback:** Own hits, own skill impacts, own damage taken
3. **Nearby Combat:** Other players' skill effects, nearby monster combat
4. **Ambient:** Environmental sounds, distant monster idle
5. **Background:** BGM (always present but lowest priority in the mix)

---

### 3D Positional Audio (If Applicable)

**Original RO Classic:** The 2D isometric client had **NO true 3D positional audio**. All sounds played at flat volume from their source. There was basic distance attenuation (sounds at the edge of the screen were quieter) but no directional/HRTF spatialization.

**Sabri_MMO (UE5 Enhancement):**

Since we are building in full 3D, we gain spatialization capabilities:

| Feature | Implementation | Purpose |
|---------|---------------|---------|
| **HRTF Spatialization** | UE5 built-in or Steam Audio plugin | Combat effects sound directional relative to camera |
| **Distance Attenuation** | `USoundAttenuation` profiles per sound category | Sounds get quieter with distance |
| **Occlusion** | Raycast-based checks | Sounds behind walls are muffled (optional) |
| **Reverb Zones** | `AReverbEffect` volumes per zone type | Dungeons echo, open fields are dry |

**Attenuation Profiles:**

| Profile | Inner Radius (UU) | Outer Radius (UU) | Falloff | Use For |
|---------|-------------------|-------------------|---------|---------|
| Combat_Close | 200 | 2000 | Linear | Melee hits, weapon swings |
| Combat_Ranged | 400 | 4000 | Logarithmic | Ranged skill impacts, arrows |
| Combat_AoE | 500 | 5000 | Logarithmic | AoE spell effects, explosions |
| Ambient_Small | 100 | 1000 | Linear | Fountains, torches, small sources |
| Ambient_Large | 500 | 3000 | Linear | Waterfalls, crowds, large sources |
| Monster_Idle | 200 | 1500 | Linear | Monster ambient/idle |
| Monster_Attack | 300 | 3000 | Linear | Monster attack/death |
| NPC_Voice | 200 | 1000 | Linear | NPC greetings |
| UI | N/A | N/A | None | 2D, no spatialization |
| BGM | N/A | N/A | None | 2D, no spatialization |

**Reverb Zones by Area Type:**

| Zone Type | Preset | Decay Time | Wet Level | Notes |
|-----------|--------|-----------|-----------|-------|
| Open Field | None/Minimal | <0.3s | Very low | Dry, open space |
| Town Square | Light outdoor | ~0.5s | Low | Slight reflection from buildings |
| Building Interior | Small room | ~1.0s | Moderate | Standard indoor reverb |
| Church/Cathedral | Large hall | ~2.5s | High | Echo, grandeur |
| Cave/Dungeon | Cave | ~3.0s | High | Deep echo, atmosphere |
| Deep Dungeon | Large cave | ~4.0s | Very high | Oppressive echo |
| Sewer/Culvert | Tunnel | ~1.5s | Medium-high | Pipe echo, dripping amplification |
| Underwater | Underwater | ~2.0s | High + low-pass filter | Muffled, pressure feel |

---

### Loop Mechanics for BGM

| Property | Value | Notes |
|----------|-------|-------|
| **Loop Method** | Seamless end-to-start loop | Track endpoint wraps to track start with zero gap |
| **Loop Count** | Infinite | BGM loops forever until map change or toggle off |
| **Loop Points** | Start-of-file to end-of-file | No custom loop point markers in original RO (full track loops) |
| **Crossfade on Loop** | None | Track simply restarts -- no overlap or crossfade on loop boundary |
| **Format** | MP3 in original (OGG Vorbis recommended for UE5) | UE5 handles loop points better with OGG/WAV than MP3 |

**UE5 Implementation:** Set `bLooping = true` on the `UAudioComponent`. For gapless looping, ensure the imported audio asset has proper loop markers, or use WAV/OGG with precise trim points. MP3 can introduce small gaps at loop boundaries due to encoder padding.

---

## Implementation for UE5

### Audio Engine Considerations

| Decision | Recommendation | Rationale |
|----------|---------------|-----------|
| **Audio Engine** | UE5 default audio engine (not Wwise/FMOD) | Sufficient for our needs, no licensing cost, built-in MetaSound support |
| **Spatialization** | UE5 built-in HRTF (or Steam Audio for advanced) | Default is good enough; Steam Audio adds reflection/occlusion if needed later |
| **MetaSounds vs Sound Cues** | Sound Cues for most SFX, MetaSounds for procedural ambient | Sound Cues are simpler, well-documented, proven. MetaSounds only where procedural control needed. |
| **Sound Waves (Raw)** | Direct playback for BGM and simple one-shot UI sounds | Lowest overhead, no processing chain needed |

**Technology Selection Per Category:**

| Category | Technology | Why |
|----------|-----------|-----|
| BGM | `USoundWave` (direct streaming) | Simple playback, no processing needed |
| Combat SFX | `USoundCue` (with random variant + pitch variation nodes) | Need randomization for hit sounds |
| Skill SFX | `USoundCue` or `USoundWave` | Most are single sounds, some need layering |
| UI SFX | `USoundWave` (direct 2D) | Simplest, lowest latency |
| Ambient | `MetaSound` (for procedural layers) or `USoundCue` (for simple loops) | Ambient benefits from parameter-driven mixing |
| Footsteps | `USoundCue` (with random variant) | Terrain-switched, needs randomization |
| Monster SFX | `USoundWave` or `USoundCue` | Per-monster lookup, simple playback |

---

### BGM Streaming vs Loaded

| Approach | Use For | Memory | Latency |
|----------|---------|--------|---------|
| **Streaming** | BGM tracks (2-5 minutes each) | ~0 MB per track (streamed from disk) | ~100ms initial, then seamless |
| **Fully Loaded** | Short SFX (<2 seconds) | ~50KB-500KB per sound | Instant playback, zero latency |
| **Async Loaded** | Monster/skill SFX (loaded on first encounter) | Pooled, loaded on demand | Small delay on first play, then cached |

**BGM should always stream.** At 44.1kHz 16-bit stereo, a 4-minute track is ~40MB uncompressed. With 20+ zones, loading all BGM into memory would waste ~800MB+. Streaming uses negligible memory.

**SFX should be fully loaded for combat-critical sounds** (hits, skills, level up) and async-loaded for less critical sounds (monster idle, distant ambient).

**UE5 Implementation:** Set `bStreaming = true` on BGM `USoundWave` assets in the editor. Short SFX assets leave streaming off (fully loaded). Use `TSoftObjectPtr<USoundBase>` for lazy loading of infrequently-used sounds, with `LoadSynchronous()` on first use.

---

### SFX Triggering System

**Integration Points with Existing Subsystems:**

| Existing Subsystem | Audio Trigger | SFX Function Call |
|-------------------|---------------|-------------------|
| `DamageNumberSubsystem` | `combat:damage` event | `SFXSubsystem::PlayHitImpact(hitType, location)` |
| `SkillVFXSubsystem` | `skill:effect_damage` event | `SFXSubsystem::PlaySkillImpact(skillId, location)` |
| `CastBarSubsystem` | `skill:cast_start` / `skill:cast_complete` | `SFXSubsystem::PlaySkillCastStart/Complete(skillId, location)` |
| `BasicInfoSubsystem` | Level up detection | `SFXSubsystem::PlayLevelUp()` / `PlayJobLevelUp()` |
| `InventorySubsystem` | Item pickup, equip, drop, error | `SFXSubsystem::PlayUISound(type)` |
| `EquipmentSubsystem` | Equip/unequip events | `SFXSubsystem::PlayUISound(ItemEquip/ItemUnequip)` |
| `HotbarSubsystem` | Hotbar activation | `SFXSubsystem::PlayUISound(HotbarActivate)` |
| `EnemySubsystem` | `enemy:spawn`, `enemy:attack`, `enemy:death` | `SFXSubsystem::PlayMonsterSound(monsterId, soundType, location)` |
| `OtherPlayerSubsystem` | Other player combat | Route through DamageNumberSubsystem |
| `ChatSubsystem` | `chat:receive` | `SFXSubsystem::PlayUISound(ChatMessage/WhisperReceived)` |
| `ZoneTransitionSubsystem` | Zone change | `BGMSubsystem::PlayBGMForZone(zoneName)` |
| `BuffBarSubsystem` | Buff applied/removed | `SFXSubsystem::PlayBuffApplied/Removed(buffId, location)` |
| `LoginFlowSubsystem` | Login screen load | `BGMSubsystem::PlayBGM(TitleTrack)` |
| `MultiplayerEventSubsystem` | `combat:player_death` | `SFXSubsystem::PlayDeathSound(isMale, location)` |
| `PartySubsystem` | `party:invite` | `SFXSubsystem::PlayUISound(PartyInvite)` |
| `VendingSubsystem` | `vending:sold` | `SFXSubsystem::PlayUISound(ZenyTransaction)` |

**Data-Driven Mapping:** All skill->SFX and monster->SFX mappings live in `AudioData.cpp` as `TMap` lookups, following the same pattern as `SkillVFXData.cpp`. No hardcoded sound references in subsystem code.

---

## Gap Analysis

### What We Have (from existing docs)

- Complete BGM track list (160 numbered tracks + silence track 997)
- Complete zone-to-BGM mapping for all major towns, dungeons, and fields
- Detailed SFX descriptions per weapon type, skill, UI action, and monster
- UE5 architecture plan (BGMSubsystem, SFXSubsystem, AmbientAudioSubsystem)
- C++ data structures for zone/skill/monster/weapon/terrain SFX mappings
- Sound Class hierarchy and Sound Mix presets
- Concurrency limits and attenuation profiles
- Asset naming conventions and folder structure

### What We Need to Build

| Priority | Item | Effort | Notes |
|----------|------|--------|-------|
| **P0** | `BGMSubsystem` C++ implementation | Medium | UWorldSubsystem with crossfade, volume, toggle |
| **P0** | `SFXSubsystem` C++ implementation | Medium | UWorldSubsystem with spatial/2D playback |
| **P0** | `AudioData.cpp` mapping tables | Medium | Zone-to-BGM, Skill-to-SFX, Monster-to-SFX |
| **P0** | Sound Class hierarchy in UE5 editor | Low | SC_Master tree, create assets |
| **P0** | Placeholder BGM for implemented zones | Medium | Source or generate at least 5-8 zone tracks |
| **P0** | Core combat SFX (hit/miss/crit/death) | Medium | Source/create ~15 base sounds |
| **P1** | Skill SFX for all implemented skills | High | ~100+ skills need sounds |
| **P1** | UI SFX set (~20 sounds) | Low-Medium | Button clicks, menus, notifications |
| **P1** | Level Up / Job Level Up fanfare | Low | 2 iconic sounds |
| **P1** | Integration hooks in existing subsystems | Medium | Add PlaySound calls to ~12 subsystems |
| **P2** | Monster SFX (attack/death per template) | High | 509 monster templates, prioritize common ones |
| **P2** | Ambient sound actors per zone | Medium | Place audio sources in levels |
| **P2** | Footstep system (terrain-based) | Medium | Physical Material -> sound mapping |
| **P2** | Reverb zones per dungeon/area type | Low-Medium | Audio Volume actors in levels |
| **P2** | Settings UI for volume sliders | Low | Integrate with existing settings |
| **P3** | `/bgm` and `/sound` chat commands | Low | Route through ChatSubsystem |
| **P3** | Voice sounds (hurt/death gendered) | Low | 4 base voice sounds |
| **P3** | NPC voice greetings (Kafra) | Low | 1-2 voiced lines |
| **P3** | MetaSound ambient procedural layers | Medium | Advanced ambient mixing |

### Risks and Considerations

| Risk | Mitigation |
|------|-----------|
| **Copyright** -- Cannot use original RO BGM/SFX | Commission original tracks, use AI generation for prototyping, source royalty-free alternatives |
| **Sound fatigue** -- Repetitive combat sounds in grinding | Multiple variants per sound (3-5), pitch randomization, concurrency limits |
| **Performance** -- Too many sounds in WoE/large battles | Strict concurrency limits, distance culling, priority system |
| **Memory** -- Loading all sounds at once | Stream BGM, lazy-load monster/skill SFX on first use |
| **Loop gaps** -- MP3 encoder padding causing audible gaps in BGM loops | Use OGG Vorbis or WAV for BGM assets, not MP3 |
| **Missing atmosphere** -- 3D game without ambient audio feels empty | Rich ambient layers are essential, even though original RO was sparse |

### Original RO Technical Details

| Property | Value |
|----------|-------|
| Audio middleware | Miles Sound System (RAD Game Tools) |
| BGM format | MP3 files in `/BGM/` folder (01.mp3 through 160.mp3) |
| SFX format | WAV files in `data/wav/` within `data.grf` archive |
| SFX subdirectories | `effect/` (skill/combat), `monster/{name}/` (per-monster), root `_hit*.wav`, `_attack*.wav` |
| Map-to-BGM data | `data/mp3nametable.txt` (format: `mapname.rsw#bgm\\XX.mp3#`) |
| Effect-to-SFX data | `effect_list.txt` (387+ entries mapping effect IDs to visual + audio) |
| Monster sound sync | ACT sprite file keyframes reference WAV files at specific animation frames |
| Volume controls | Two independent sliders (BGM, SFX) + `/bgm` and `/sound` toggle commands |
| Channel limit | Hardware-dependent via Miles, ~32 concurrent channels typical |

---

*Document generated from deep research across iRO Wiki, VGMdb, rAthena source, The Cutting Room Floor, Ragnarok Wiki (Fandom), Wikipedia (SoundTeMP), RateMyServer, WarpPortal Forums, and community resources.*

Sources:
- [iRO Wiki - BGM Files](https://irowiki.org/wiki/BGM_Files)
- [iRO Wiki - Sound Effects](https://irowiki.org/wiki/Sound_Effects)
- [iRO Wiki - Original Soundtrack](https://irowiki.org/wiki/Original_Soundtrack)
- [VGMdb - RAGNAROK Online Complete Soundtrack](https://vgmdb.net/album/14011)
- [Wikipedia - SoundTeMP](https://en.wikipedia.org/wiki/SoundTeMP)
- [Ragnarok Wiki - SoundTeMP](https://ragnarok.fandom.com/wiki/SoundTeMP)
- [Ragnarok Wiki - Complete Soundtrack](https://ragnarok.fandom.com/wiki/Ragnarok_Online_Complete_Soundtrack)
- [The Cutting Room Floor - RO Unused Sound Effects](https://tcrf.net/Ragnarok_Online/Unused_Sound_Effects)
- [rAthena - effect_list.txt](https://github.com/idathena/trunk/blob/master/doc/effect_list.txt)
- [ROenglishPRE - mp3nametable.txt](https://github.com/zackdreaver/ROenglishPRE/blob/master/data/mp3nametable.txt)
- [ROClientSide - mp3nametable.txt](https://github.com/ROClientSide/kRO-RAW-RE/blob/master/data/mp3nametable.txt)
- [RateMyServer - Dungeon BGM Downloads](https://ratemyserver.net/index.php?page=download_music&type=3)
- [Hercules Wiki - Data Folder Structure](https://github.com/HerculesWS/Hercules/wiki/Data-Folder-Structure)
- [rAthena Wiki - GRF](https://github.com/rathena/rathena/wiki/GRF)
- [The Phrozen Keep - RO KOR Sound Files](https://d2mods.info/forum/viewtopic.php?t=66802)
- [KHInsider - RO Original Soundtrack](https://downloads.khinsider.com/game-soundtracks/album/ragnarok-online-original-soundtrack)
