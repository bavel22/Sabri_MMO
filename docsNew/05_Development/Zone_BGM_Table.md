# Zone -> BGM Table

Extracted from `client/SabriMMO/Source/SabriMMO/Audio/AudioSubsystem.cpp:744-889` (`ZoneToBgmMap`).
This is the client-side mp3nametable equivalent — when a zone loads, `PlayZoneBgm(zoneName)` does a case-insensitive lookup in this table and forwards the resolved asset path to `PlayBgm`.

All asset paths are relative to `/Game/SabriMMO/Audio/BGM/`.

---

## Special (not in ZoneToBgmMap)

| Context        | Track   | Title  | Caller                                         |
|----------------|---------|--------|------------------------------------------------|
| Login screen   | bgm_01  | Title  | `LoginFlowSubsystem::OnWorldBeginPlay` (line 83) |

---

## Towns

| Zone             | Track    | Title                       |
|------------------|----------|-----------------------------|
| prontera         | bgm_08   | Theme of Prontera           |
| prontera_south   | bgm_08   | Theme of Prontera           |
| prontera_north   | bgm_08   | Theme of Prontera           |
| prontera_east    | bgm_08   | Theme of Prontera           |
| prontera_west    | bgm_08   | Theme of Prontera           |
| morroc           | bgm_11   | Theme of Morroc             |
| morroc_south     | bgm_11   | Theme of Morroc             |
| geffen           | bgm_13   | Theme of Geffen             |
| geffen_south     | bgm_13   | Theme of Geffen             |
| payon            | bgm_14   | Theme of Payon              |
| payon_south      | bgm_14   | Theme of Payon              |
| payon_north      | bgm_14   | Theme of Payon              |
| alberta          | bgm_15   | Theme of Alberta            |
| izlude           | bgm_26   | Everlasting Wanderers       |
| aldebaran        | bgm_39   | Theme of Al de Baran        |
| comodo           | bgm_62   | High Roller Coaster         |
| yuno             | bgm_70   | Theme of Juno               |
| umbala           | bgm_68   | Jazzy Funky Sweety          |
| amatsu           | bgm_76   | Purity of Your Smile        |
| gonryun          | bgm_74   | Not So Far Away             |
| louyang          | bgm_79   | The Great                   |
| ayothaya         | bgm_81   | Thai Orchid                 |
| xmas             | bgm_59   | Theme of Lutie              |
| niflheim         | bgm_84   | Christmas in 13th Month     |
| einbroch         | bgm_86   | Steel Me                    |
| einbech          | bgm_86   | Steel Me                    |
| lighthalzen      | bgm_90   | Noblesse Oblige             |
| hugel            | bgm_93   | Latinnova                   |
| rachel           | bgm_94   | Theme of Rachel             |
| veins            | bgm_104  | On Your Way Back            |
| moscovia         | bgm_114  | Theme of Moscovia           |

---

## Prontera Fields

| Zone         | Track    | Title                |
|--------------|----------|----------------------|
| prt_fild01   | bgm_12   | Streamside           |
| prt_fild02   | bgm_05   | Tread on the Ground  |
| prt_fild03   | bgm_05   | Tread on the Ground  |
| prt_fild04   | bgm_05   | Tread on the Ground  |
| prt_fild05   | bgm_12   | Streamside           |
| prt_fild06   | bgm_12   | Streamside           |
| prt_fild07   | bgm_05   | Tread on the Ground  |
| prt_fild08   | bgm_12   | Streamside           |
| prt_fild09   | bgm_04   | I Miss You           |
| prt_fild10   | bgm_04   | I Miss You           |
| prt_fild11   | bgm_04   | I Miss You           |
| prt_fild12   | bgm_12   | Streamside           |

---

## Geffen Fields

| Zone         | Track    | Title       |
|--------------|----------|-------------|
| gef_fild01   | bgm_23   | Travel      |
| gef_fild02   | bgm_35   | Nano East   |
| gef_fild03   | bgm_35   | Nano East   |
| gef_fild04   | bgm_25   | Plateau     |
| gef_fild05   | bgm_23   | Travel      |
| gef_fild06   | bgm_23   | Travel      |
| gef_fild07   | bgm_25   | Plateau     |
| gef_fild08   | bgm_23   | Travel      |
| gef_fild09   | bgm_23   | Travel      |

---

## Morroc Fields

| Zone         | Track    | Title             |
|--------------|----------|-------------------|
| moc_fild01   | bgm_24   | Desert            |
| moc_fild02   | bgm_03   | Peaceful Forest   |
| moc_fild03   | bgm_03   | Peaceful Forest   |
| moc_fild04   | bgm_24   | Desert            |
| moc_fild05   | bgm_24   | Desert            |
| moc_fild06   | bgm_24   | Desert            |
| moc_fild07   | bgm_24   | Desert            |

---

## Payon Fields

| Zone         | Track    | Title             |
|--------------|----------|-------------------|
| pay_fild01   | bgm_03   | Peaceful Forest   |
| pay_fild02   | bgm_03   | Peaceful Forest   |
| pay_fild03   | bgm_03   | Peaceful Forest   |
| pay_fild04   | bgm_03   | Peaceful Forest   |

---

## Dungeons

### Ant Hell

| Zone         | Track    | Title              |
|--------------|----------|--------------------|
| ant_hell     | bgm_46   | An Ant-Lion's Pit  |
| ant_hell_01  | bgm_46   | An Ant-Lion's Pit  |
| ant_hell_02  | bgm_46   | An Ant-Lion's Pit  |
| anthell01    | bgm_46   | An Ant-Lion's Pit  |
| anthell02    | bgm_46   | An Ant-Lion's Pit  |

### Byalan / Izlude Underwater

| Zone         | Track    | Title                  |
|--------------|----------|------------------------|
| byalan       | bgm_29   | Be Nice 'n Easy        |
| byalan_01    | bgm_29   | Be Nice 'n Easy        |
| byalan_02    | bgm_29   | Be Nice 'n Easy        |
| iz_dun00     | bgm_29   | Be Nice 'n Easy        |
| iz_dun01     | bgm_29   | Be Nice 'n Easy        |
| iz_dun02     | bgm_29   | Be Nice 'n Easy        |
| iz_dun03     | bgm_49   | Watery Grave (deep)    |
| iz_dun04     | bgm_49   | Watery Grave (deep)    |
| iz_dun05     | bgm_49   | Watery Grave (deep)    |

### Payon Cave

| Zone         | Track    | Title                       |
|--------------|----------|-----------------------------|
| payon_dun    | bgm_20   | Ancient Groover             |
| pay_dun00    | bgm_20   | Ancient Groover             |
| pay_dun01    | bgm_20   | Ancient Groover             |
| pay_dun02    | bgm_20   | Ancient Groover             |
| pay_dun03    | bgm_47   | Welcome Mr. Hwang (deep)    |
| pay_dun04    | bgm_47   | Welcome Mr. Hwang (deep)    |

### Geffen Tower

| Zone         | Track    | Title                       |
|--------------|----------|-----------------------------|
| gef_dun00    | bgm_21   | Through the Tower           |
| gef_dun01    | bgm_21   | Through the Tower           |
| gef_dun02    | bgm_50   | Out of Curiosity (deep)     |
| gef_dun03    | bgm_50   | Out of Curiosity (deep)     |

### Orc Dungeon

| Zone         | Track    | Title           |
|--------------|----------|-----------------|
| orcsdun01    | bgm_48   | Help Yourself   |
| orcsdun02    | bgm_48   | Help Yourself   |

### Prontera Culvert

| Zone         | Track    | Title              |
|--------------|----------|--------------------|
| prt_sewb1    | bgm_19   | Under the Ground   |
| prt_sewb2    | bgm_19   | Under the Ground   |
| prt_sewb3    | bgm_19   | Under the Ground   |
| prt_sewb4    | bgm_19   | Under the Ground   |

### Prontera Maze

| Zone         | Track    | Title       |
|--------------|----------|-------------|
| prt_maze01   | bgm_16   | Labyrinth   |
| prt_maze02   | bgm_16   | Labyrinth   |
| prt_maze03   | bgm_16   | Labyrinth   |

---

## Glast Heim

| Zone         | Track    | Title           |
|--------------|----------|-----------------|
| glast_01     | bgm_42   | Curse'n Pain    |
| gl_dun01     | bgm_42   | Curse'n Pain    |
| gl_dun02     | bgm_42   | Curse'n Pain    |
| gl_in01      | bgm_42   | Curse'n Pain    |
| gl_step      | bgm_42   | Curse'n Pain    |
| gl_chyard    | bgm_40   | Monk Zonk       |
| gl_church    | bgm_40   | Monk Zonk       |
| gl_prison    | bgm_40   | Monk Zonk       |
| gl_cas01     | bgm_43   | Morning Gloomy  |
| gl_cas02     | bgm_43   | Morning Gloomy  |
| gl_knt01     | bgm_44   | TeMP it Up      |
| gl_knt02     | bgm_44   | TeMP it Up      |

---

## Pyramids / Sphinx

| Zone         | Track    | Title          |
|--------------|----------|----------------|
| moc_pryd01   | bgm_22   | Backattack!!   |
| moc_pryd02   | bgm_22   | Backattack!!   |
| moc_pryd03   | bgm_22   | Backattack!!   |
| moc_pryd04   | bgm_22   | Backattack!!   |
| moc_pryd05   | bgm_22   | Backattack!!   |
| moc_pryd06   | bgm_22   | Backattack!!   |
| in_sphinx1   | bgm_38   | Sphinx Theme   |
| in_sphinx2   | bgm_38   | Sphinx Theme   |
| in_sphinx3   | bgm_38   | Sphinx Theme   |
| in_sphinx4   | bgm_38   | Sphinx Theme   |
| in_sphinx5   | bgm_38   | Sphinx Theme   |

---

## Special / Event

| Zone         | Track    | Title                       |
|--------------|----------|-----------------------------|
| xmas_dun01   | bgm_58   | Jingle Bell on Ragnarok     |
| xmas_dun02   | bgm_58   | Jingle Bell on Ragnarok     |

---

## Notes

- **Lookup is case-insensitive**: `PlayZoneBgm` first tries the exact key, then `ZoneName.ToLower()`.
- **Unmapped zones**: if a zone isn't in this table, `PlayZoneBgm` returns silently — the previous track keeps playing rather than going silent.
- **Idempotency**: `PlayBgm` no-ops when the requested track equals `CurrentBgmPath`, so adjacent prontera quadrants (all `bgm_08`) cause no audible cut.
- **Crossfade**: 1.5s fade-out on the old track, 1.5s fade-in on the new track.
- **Looping**: forced at runtime via `USoundWave::bLooping = true` in `PlayBgm` regardless of import setting.
- **Volume bus**: assigned to `BgmSoundClass` so the BGM volume slider in Options controls it.

**Total entries**: 121 zones across 31 town entries, 32 field entries, 35 dungeon entries, 12 Glast Heim, 11 Pyramids/Sphinx, 2 special.
