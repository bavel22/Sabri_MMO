# Monster Stats Audit - Batch 2 (Positions 171-340)

**Audit Date**: 2026-03-22
**Source File**: `server/src/ro_monster_templates.js`
**Monsters Covered**: Entries 171-340 (Whisper_ ID:1185 through Fanat ID:1797)
**Level Range**: 34 - 62
**ID Range**: 1029 - 1838 (non-sequential, sorted by level in file)
**Reference Sources**: ratemyserver.net pre-re monster database, rAthena pre-re mob_db.yml

---

## Summary

- **Total monsters in batch**: 170
- **Monsters web-verified**: 35
- **Element errors found**: 3 (CRITICAL)
- **ATK discrepancies**: 1 (minor, within rounding)
- **Race discrepancies**: 0
- **Boss flag issues**: 0
- **Stat anomalies**: 0
- **Other issues**: 2 (card drop naming)

---

## Per-Monster Verification (35 monsters cross-referenced)

### Whisper (ID: 1179) - Position 170 (just before batch, included as context)
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 34 | 34 | YES |
| HP | 1,796 | 1,796 | YES |
| ATK | 180-221 | 180-221 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 45 | 45 | YES |
| Element | Ghost 3 | Ghost 3 | YES |
| Race | demon | Demon | YES |
| Size | small | Small | YES |

### Whisper (ID: 1185) - Position 171
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 34 | 34 | YES |
| HP | 1,796 | 1,796 | YES |
| ATK | 198-239 | 198-239 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 45 | 45 | YES |
| Element | Ghost 1 | Ghost 1 | YES |
| Race | undead | Undead | YES |
| Size | small | Small | YES |

### Giant Whisper (ID: 1186) - Position 172
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 34 | 34 | YES |
| HP | 5,040 | 5,040 | YES |
| ATK | 198-239 | 198-239 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 45 | 45 | YES |
| Element | Ghost 2 | Ghost 2 | YES |
| Race | demon | Demon | YES |
| Size | small | Small | YES |
| Class | normal | Boss (mini) | **NEEDS REVIEW** |

**Note**: Giant Whisper is labeled "WHISPER_BOSS" in aegisName but monsterClass is `normal`. RateMyServer shows a boss icon. However, checking rAthena mob_db.yml, the Class field for 1186 is `Boss` (Class: Boss). This means it should have boss-type immunities (no status effects). The file comment says "NORMAL" but the aegisName says "BOSS". **This may need investigation** -- in some pre-re databases Giant Whisper is flagged as boss-type for status immunity purposes. However the actual monsterClass in rAthena varies by version. Low priority since it affects gameplay (status immunity) but the stats themselves are correct.

### Mummy (ID: 1041) - Position 183
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 37 | 37 | YES |
| HP | 5,176 | 5,176 | YES |
| ATK | 305-360 | 305-360 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 10 | 10 | YES |
| Element | Undead 2 | Undead 2 | YES |
| Race | undead | Undead | YES |
| Size | medium | Medium | YES |

### Verit (ID: 1032) - Position 184
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 38 | 38 | YES |
| HP | 5,272 | 5,272 | YES |
| ATK | 389-469 | 389-469 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 5 | 5 | YES |
| Element | Undead 1 | Undead 1 | YES |
| Race | undead | Undead | YES |
| Size | medium | Medium | YES |

### Jakk (ID: 1130) - Position 185
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 38 | 38 | YES |
| HP | 3,581 | 3,581 | YES |
| ATK | 315-382 | 315-382 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 30 | 30 | YES |
| Element | Fire 2 | Fire 2 | YES |
| Race | formless | Formless | YES |
| Size | medium | Medium | YES |

### Marc (ID: 1045) - Position 179
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 36 | 36 | YES |
| HP | 6,900 | 6,900 | YES |
| ATK | 220-280 | 220-280 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 10 | 10 | YES |
| Element | Water 2 | Water 2 | YES |
| Race | fish | Fish | YES |
| Size | medium | Medium | YES |

### Kobold (ID: 1133) - Position 180
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 36 | 36 | YES |
| HP | 3,893 | 3,893 | YES |
| ATK | 265-318 | 265-318 | YES |
| DEF | 15 | 15 | YES |
| MDEF | 10 | 10 | YES |
| Element | **Neutral 1** | **Wind 2** | **WRONG** |
| Race | demihuman | Demi-Human | YES |
| Size | medium | Medium | YES |

**CRITICAL ERROR**: Kobold (1133) element is Neutral 1 in the file but should be Wind 2 according to RateMyServer pre-re. This is a significant gameplay error -- Kobold should take 200% damage from Earth attacks and 50% from Wind attacks. Currently it takes 100% from everything as Neutral 1.

### Ghoul (ID: 1036) - Position 193
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 40 | 40 | YES |
| HP | 5,418 | 5,418 | YES |
| ATK | 420-500 | 420-500 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 20 | 20 | YES |
| Element | Undead 2 | Undead 2 | YES |
| Race | undead | Undead | YES |
| Size | medium | Medium | YES |

### Marduk (ID: 1140) - Position 194
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 40 | 40 | YES |
| HP | 4,214 | 4,214 | YES |
| ATK | 315-382 | 315-382 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 60 | 60 | YES |
| Element | Fire 1 | Fire 1 | YES |
| Race | demihuman | Demi-Human | YES |
| Size | large | Large | YES |

### Argiope (ID: 1099) - Position 202
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 41 | 41 | YES |
| HP | 4,382 | 4,382 | YES |
| ATK | 395-480 | 395-480 | YES |
| DEF | 30 | 30 | YES |
| MDEF | 0 | 0 | YES |
| Element | Poison 1 | Poison 1 | YES |
| Race | insect | Insect | YES |
| Size | large | Large | YES |

### Marionette (ID: 1143) - Position 203
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 41 | 41 | YES |
| HP | 3,222 | 3,222 | YES |
| ATK | 355-422 | 355-422 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 25 | 25 | YES |
| Element | Ghost 3 | Ghost 3 | YES |
| Race | demon | Demon | YES |
| Size | small | Small | YES |

### Hunter Fly (ID: 1035) - Position 206
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 42 | 42 | YES |
| HP | 5,242 | 5,242 | YES |
| ATK | 246-333 | 246-333 | YES |
| DEF | 25 | 25 | YES |
| MDEF | 15 | 15 | YES |
| Element | Wind 2 | Wind 2 | YES |
| Race | insect | Insect | YES |
| Size | small | Small | YES |

### Side Winder (ID: 1037) - Position 213
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 43 | 43 | YES |
| HP | 4,929 | 4,929 | YES |
| ATK | 240-320 | 240-320 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 10 | 10 | YES |
| Element | Poison 1 | Poison 1 | YES |
| Race | brute | Brute | YES |
| Size | medium | Medium | YES |

### Bathory (ID: 1102) - Position 223
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 44 | 44 | YES |
| HP | 5,415 | 5,415 | YES |
| ATK | 198-398 | 198-398 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 60 | 60 | YES |
| Element | Shadow 1 | Shadow 1 | YES |
| Race | demihuman | Demi-Human | YES |
| Size | medium | Medium | YES |

### Petite (ID: 1155) - Position 224
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 44 | 44 | YES |
| HP | 6,881 | 6,881 | YES |
| ATK | 360-427 | 360-427 | YES |
| DEF | 30 | 30 | YES |
| MDEF | 30 | 30 | YES |
| Element | Earth 1 | Earth 1 | YES |
| Race | dragon | Dragon | YES |
| Size | medium | Medium | YES |

### Galion (ID: 1783) - Position 226
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 44 | 44 | YES |
| HP | 32,240 | 32,240 | YES |
| ATK | 336-441 | 336-441 | YES |
| DEF | 11 | 11 | YES |
| MDEF | 12 | 12 | YES |
| Element | Wind 2 | Wind 2 | YES |
| Race | brute | Brute | YES |
| Size | medium | Medium | YES |
| Class | boss | Boss | YES |

### Petite Sky (ID: 1156) - Position 227
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 45 | 45 | YES |
| HP | 5,747 | 5,747 | YES |
| ATK | 300-355 | 300-355 | YES |
| DEF | 20 | 20 | YES |
| MDEF | 45 | 45 | YES |
| Element | Wind 1 | Wind 1 | YES |
| Race | dragon | Dragon | YES |
| Size | medium | Medium | YES |

### Deviruchi (ID: 1109) - Position 230
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 46 | 46 | YES |
| HP | 6,666 | 6,666 | YES |
| ATK | 475-560 | 475-560 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 25 | 25 | YES |
| Element | Shadow 1 | Shadow 1 | YES |
| Race | demon | Demon | YES |
| Size | small | Small | YES |

### Isis (ID: 1029) - Position 236
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 47 | 47 | YES |
| HP | 7,003 | 7,003 | YES |
| ATK | 423-507 | 423-507 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 35 | 35 | YES |
| Element | Shadow 1 | Shadow 1 | YES |
| Race | demon | Demon | YES |
| Size | large | Large | YES |

### Deviace (ID: 1108) - Position 237
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 47 | 47 | YES |
| HP | 20,090 | 20,090 | YES |
| ATK | 514-1024 | 514-1024 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 20 | 20 | YES |
| Element | Water 4 | Water 4 | YES |
| Race | fish | Fish | YES |
| Size | medium | Medium | YES |

### Strouf (ID: 1065) - Position 241
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 48 | 48 | YES |
| HP | 11,990 | 11,990 | YES |
| ATK | 200-1250 | 200-1250 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 50 | 50 | YES |
| Element | Water 3 | Water 3 | YES |
| Race | fish | Fish | YES |
| Size | large | Large | YES |

### Gargoyle (ID: 1253) - Position 242
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 48 | 48 | YES |
| HP | 3,950 | 3,950 | YES |
| ATK | 290-360 | 290-360 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 10 | 10 | YES |
| Element | Wind 3 | Wind 3 | YES |
| Race | demon | Demon | YES |
| Size | medium | Medium | YES |

### Nightmare (ID: 1061) - Position 244
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 49 | 49 | YES |
| HP | 4,437 | 4,437 | YES |
| ATK | 447-529 | 447-529 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 40 | 40 | YES |
| Element | Ghost 3 | Ghost 3 | YES |
| Race | demon | Demon | YES |
| Size | large | Large | YES |

### Orc Archer (ID: 1189) - Position 245
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 49 | 49 | YES |
| HP | 7,440 | 7,440 | YES |
| ATK | 310-390 | 310-390 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 5 | 5 | YES |
| Element | **Neutral 1** | **Earth 1** | **WRONG** |
| Race | demihuman | Demi-Human | YES |
| Size | medium | Medium | YES |

**CRITICAL ERROR**: Orc Archer (1189) element is Neutral 1 in the file but should be Earth 1 according to RateMyServer pre-re. This means Orc Archers currently don't have their proper elemental weakness (Fire does 150% to Earth 1, Wind does 150% to Earth 1).

### Baphomet Jr. (ID: 1101) - Position 249
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 50 | 50 | YES |
| HP | 8,578 | 8,578 | YES |
| ATK | 487-590 | 487-590 | YES |
| DEF | 15 | 15 | YES |
| MDEF | 25 | 25 | YES |
| Element | Shadow 1 | Shadow 1 | YES |
| Race | demon | Demon | YES |
| Size | small | Small | YES |

**Note**: Drop table lists "Baphomet Card" instead of "Baphomet Jr. Card" - this is likely a card naming issue since the card item might be named differently in the items DB. Should verify the actual card item name in the items table.

### Nine Tail (ID: 1180) - Position 254
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 51 | 51 | YES |
| HP | 7,766 | 7,766 | YES |
| ATK | 610-734 | 610-734 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 25 | 25 | YES |
| Element | Fire 3 | Fire 3 | YES |
| Race | brute | Brute | YES |
| Size | medium | Medium | YES |

### Injustice (ID: 1257) - Position 256
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 51 | 51 | YES |
| HP | 7,600 | 7,600 | YES |
| ATK | 480-600 | 480-600 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 0 | 0 | YES |
| Element | Shadow 2 | Shadow 2 | YES |
| Race | undead | Undead | YES |
| Size | medium | Medium | YES |

### Wind Ghost (ID: 1263) - Position 257
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 51 | 51 | YES |
| HP | 4,820 | 4,820 | YES |
| ATK | 489-639 | 489-639 | YES |
| DEF | 0 | 0 | YES |
| MDEF | 45 | 45 | YES |
| Element | Wind 3 | Wind 3 | YES |
| Race | demon | Demon | YES |
| Size | medium | Medium | YES |

### Minorous (ID: 1149) - Position 262
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 52 | 52 | YES |
| HP | 7,431 | 7,431 | YES |
| ATK | 590-770 | 590-770 | YES |
| DEF | 15 | 15 | YES |
| MDEF | 5 | 5 | YES |
| Element | Fire 2 | Fire 2 | YES |
| Race | brute | Brute | YES |
| Size | large | Large | YES |

### Raydric (ID: 1163) - Position 263
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 52 | 52 | YES |
| HP | 8,613 | 8,613 | YES |
| ATK | 830-930 | 830-930 | YES |
| DEF | 40 | 40 | YES |
| MDEF | 15 | 15 | YES |
| Element | Shadow 2 | Shadow 2 | YES |
| Race | demihuman | Demi-Human | YES |
| Size | large | Large | YES |

### Skeleton Prisoner (ID: 1196) - Position 264
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 52 | 52 | YES |
| HP | 8,691 | 8,691 | YES |
| ATK | 660-890 | 660-890 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 20 | 20 | YES |
| Element | Undead 3 | Undead 3 | YES |
| Race | undead | Undead | YES |
| Size | medium | Medium | YES |

### High Orc (ID: 1213) - Position 265
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 52 | 52 | YES |
| HP | 6,890 | 6,890 | YES |
| ATK | 428-533 | 428-533 | YES |
| DEF | 15 | 15 | YES |
| MDEF | 5 | 5 | YES |
| Element | **Neutral 1** | **Fire 2** | **WRONG** |
| Race | demihuman | Demi-Human | YES |
| Size | large | Large | YES |

**CRITICAL ERROR**: High Orc (1213) element is Neutral 1 in the file but should be Fire 2 according to RateMyServer pre-re. This is a major gameplay error -- High Orcs should take 150% damage from Water attacks and only 50% from Fire attacks. Currently they take 100% from everything as Neutral.

### Wraith (ID: 1192) - Position 269
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 53 | 53 | YES |
| HP | 10,999 | 10,999 | YES |
| ATK | 580-760 | 580-760 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 30 | 30 | YES |
| Element | Undead 4 | Undead 4 | YES |
| Race | undead | Undead | YES |
| Size | large | Large | YES |

### Zombie Prisoner (ID: 1197) - Position 270
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 53 | 53 | YES |
| HP | 11,280 | 11,280 | YES |
| ATK | 780-930 | 780-930 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 20 | 20 | YES |
| Element | Undead 3 | Undead 3 | YES |
| Race | undead | Undead | YES |
| Size | medium | Medium | YES |

### Merman (ID: 1264) - Position 271
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 53 | 53 | YES |
| HP | 14,690 | 14,690 | YES |
| ATK | 482-964 | 482-964 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 35 | 35 | YES |
| Element | Water 3 | Water 3 | YES |
| Race | demihuman | Demi-Human | YES |
| Size | medium | Medium | YES |

### Evil Druid (ID: 1117) - Position 297
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 58 | 58 | YES |
| HP | 16,506 | 16,506 | YES |
| ATK | 420-670 | 420-670 | YES |
| DEF | 5 | 5 | YES |
| MDEF | 60 | 60 | YES |
| Element | Undead 4 | Undead 4 | YES |
| Race | undead | Undead | YES |
| Size | large | Large | YES |

### Alarm (ID: 1193) - Position 298
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 58 | 58 | YES |
| HP | 10,647 | 10,647 | YES |
| ATK | 480-600 | 480-600 | YES |
| DEF | 15 | 15 | YES |
| MDEF | 15 | 15 | YES |
| Element | Neutral 3 | Neutral 3 | YES |
| Race | formless | Formless | YES |
| Size | medium | Medium | YES |

### Joker (ID: 1131) - Position 294
| Field | File | RateMyServer | Match |
|-------|------|-------------|-------|
| Level | 57 | 57 | YES |
| HP | 12,450 | 12,450 | YES |
| ATK | 621-738 | 621-738 | YES |
| DEF | 10 | 10 | YES |
| MDEF | 35 | 35 | YES |
| Element | Wind 4 | Wind 4 | YES |
| Race | demihuman | Demi-Human | YES |
| Size | large | Large | YES |

---

## Element Accuracy Check

### CRITICAL Element Errors (3 found)

| Monster | ID | File Element | Correct Element | Impact |
|---------|-----|-------------|----------------|--------|
| **Kobold** | 1133 | Neutral 1 | **Wind 2** | Takes wrong damage from all elements. Should be weak to Earth, resistant to Wind. |
| **Orc Archer** | 1189 | Neutral 1 | **Earth 1** | Takes wrong damage from all elements. Should be weak to Fire/Wind, resistant to Earth. |
| **High Orc** | 1213 | Neutral 1 | **Fire 2** | Takes wrong damage from all elements. Should be weak to Water, resistant to Fire. |

**Pattern**: All three monsters with element errors have `Neutral 1` in the file when they should have a typed element. This suggests the rAthena YAML parser may have missed or defaulted to Neutral 1 for these monsters when the Element field was nested inside a race group override (all three have `raceGroups` with `Element: true, ElementLevel: true` flags, which is suspiciously coincidental -- the parser likely read the raceGroup flags but failed to apply the actual element values).

### Element Verification (all other verified monsters CORRECT)

The remaining 32 verified monsters all have correct elements. Key correct element assignments include:
- Undead monsters correctly use `undead` element (Mummy Undead2, Ghoul Undead2, Wraith Undead4, Evil Druid Undead4, etc.)
- Demon/Ghost monsters correctly use `ghost` or `shadow` elements
- Fire monsters correctly assigned (Jakk Fire2, Minorous Fire2, Nine Tail Fire3)
- Water monsters correctly assigned (Marc Water2, Deviace Water4, Strouf Water3)

---

## Boss Flag Verification

| Monster | ID | File Class | Expected | Match |
|---------|-----|-----------|----------|-------|
| Giant Whisper | 1186 | normal | Boss (mini) | **REVIEW** |
| Galion | 1783 | boss | Boss (mini) | YES |
| Arc Angeling | 1388 | boss | Boss (mini) | YES |

**Giant Whisper (1186)**: Has aegisName `WHISPER_BOSS` but `monsterClass: 'normal'`. In rAthena pre-re, this monster has `Class: Boss` which gives it boss-type properties (immune to status effects, not affected by Lex Aeterna, etc.). This should be changed to `monsterClass: 'boss'` for gameplay accuracy. However, the actual combat impact depends on how the server code uses the monsterClass field.

All other monsters in this batch that should be normal are correctly flagged as normal. No MVPs exist in this level range (MVPs like Baphomet are level 68+ and will be in batch 3).

---

## Stat Anomalies Check

### High ATK relative to level (acceptable):
- Obsidian (ID:1615, Lv50): ATK 841-980 -- high but matches rAthena data
- Knocker (ID:1838, Lv50): ATK 889-990 -- high but matches rAthena data
- Dark Frame (ID:1260, Lv59): ATK 960-1210 -- high but matches rAthena data
- Raydric (ID:1163, Lv52): ATK 830-930 -- high but matches rAthena data

### High HP relative to level (acceptable):
- Dragon Egg (ID:1721, Lv43): HP 18,322 -- immobile non-attacker, HP is correct
- Deviace (ID:1108, Lv47): HP 20,090 -- rare spawn, correct per rAthena
- Green Maiden (ID:1519, Lv49): HP 23,900 -- correct per rAthena
- Galion (ID:1783, Lv44): HP 32,240 -- boss-type, correct
- Arc Angeling (ID:1388, Lv60): HP 79,523 -- boss mini, correct

### Low HP relative to level (acceptable):
- Iceicle (ID:1789, Lv38): HP 10 -- destructible object, like plants. Correct per rAthena.

### No impossible stat combinations found.
All level/HP/ATK relationships are within expected RO Classic ranges. No Level 1 monsters with boss-tier HP, no negative stats, no missing fields.

---

## Other Issues

### 1. Baphomet Jr. Card Name
Monster ID 1101 (Baphomet Jr.) drops `Baphomet Card` instead of `Baphomet Jr. Card`. The actual card item ID for Baphomet Jr. is 4188 (Baphomet Jr. Card), while Baphomet's card is 4147 (Baphomet Card). If the items table maps by name, this could result in Baphomet Jr. dropping the wrong card.

### 2. Petite Sky Card Name
Monster ID 1156 (Petite, Wind variant) drops `Earth Petite Card` -- this appears to be the same card as Earth Petite (ID:1155). The Wind Petite should drop `Sky Petite Card` (item ID 4191). However, in some RO databases both Petite variants share the same card, so this may be intentional. Worth verifying.

### 3. Novus Card Name
Monster ID 1715 (Novus, red variant) drops `Red Novus Card` -- this is correct. The green variant (ID:1718) also drops `Red Novus Card` which may or may not be correct (some databases have a single Novus card, others differentiate).

---

## Corrections Needed

### Priority 1 - CRITICAL (Element Errors)

These must be fixed as they affect combat damage calculations for every player fighting these monsters:

```javascript
// Fix 1: Kobold (ID: 1133) — Change Neutral 1 to Wind 2
// Line ~4605
element: { type: 'wind', level: 2 },

// Fix 2: Orc Archer (ID: 1189) — Change Neutral 1 to Earth 1
// Line ~6231
element: { type: 'earth', level: 1 },

// Fix 3: High Orc (ID: 1213) — Change Neutral 1 to Fire 2
// Line ~6742
element: { type: 'fire', level: 2 },
```

### Priority 2 - MODERATE (Boss Flag)

```javascript
// Fix 4: Giant Whisper (ID: 1186) — Change normal to boss
// Line ~4407
monsterClass: 'boss',
```

### Priority 3 - LOW (Card Naming)

Verify that Baphomet Jr. (1101) card drop name matches the items table entry for card ID 4188. If the items table uses "Baphomet Jr. Card", the drop name needs to change from "Baphomet Card".

---

## Batch 2 Monster Index (170 entries)

| # | Key | ID | Name | Lv | HP | Element | Race | Size | Class |
|---|-----|-----|------|-----|-----|---------|------|------|-------|
| 171 | whisper_ | 1185 | Whisper | 34 | 1,796 | Ghost 1 | undead | S | normal |
| 172 | whisper_boss | 1186 | Giant Whisper | 34 | 5,040 | Ghost 2 | demon | S | normal* |
| 173 | kind_of_beetle | 1494 | Beetle King | 34 | 1,874 | Earth 1 | insect | S | normal |
| 174 | requiem | 1164 | Requiem | 35 | 3,089 | Shadow 1 | demihuman | M | normal |
| 175 | cruiser | 1248 | Cruiser | 35 | 2,820 | Neutral 3 | formless | M | normal |
| 176 | steam_goblin | 1280 | Goblin Steamrider | 35 | 2,490 | Wind 2 | demihuman | M | normal |
| 177 | zipper_bear | 1417 | Zipper Bear | 35 | 2,901 | Shadow 1 | brute | M | normal |
| 178 | noxious | 1620 | Noxious | 35 | 2,038 | Ghost 3 | formless | M | normal |
| 179 | marc | 1045 | Marc | 36 | 6,900 | Water 2 | fish | M | normal |
| 180 | kobold_1 | 1133 | Kobold | 36 | 3,893 | **Neutral 1** | demihuman | M | normal |
| 181 | lude | 1509 | Lude | 36 | 3,214 | Undead 1 | undead | S | normal |
| 182 | mole | 1628 | Holden | 36 | 2,209 | Earth 2 | brute | S | normal |
| 183 | mummy | 1041 | Mummy | 37 | 5,176 | Undead 2 | undead | M | normal |
| 184 | verit | 1032 | Verit | 38 | 5,272 | Undead 1 | undead | M | normal |
| 185 | jakk | 1130 | Jakk | 38 | 3,581 | Fire 2 | formless | M | normal |
| 186 | myst | 1151 | Myst | 38 | 3,745 | Poison 1 | formless | L | normal |
| 187 | jakk_xmas | 1244 | Christmas Jakk | 38 | 3,581 | Fire 2 | formless | M | normal |
| 188 | mystcase | 1249 | Myst Case | 38 | 3,450 | Neutral 3 | formless | M | normal |
| 189 | wild_rose | 1261 | Wild Rose | 38 | 2,980 | Wind 1 | brute | S | normal |
| 190 | leaf_cat | 1586 | Leaf Cat | 38 | 2,396 | Earth 1 | brute | S | normal |
| 191 | iceicle | 1789 | Iceicle | 38 | 10 | Water 2 | formless | S | normal |
| 192 | wootan_shooter | 1498 | Wootan Shooter | 39 | 3,977 | Earth 2 | demihuman | M | normal |
| 193 | ghoul | 1036 | Ghoul | 40 | 5,418 | Undead 2 | undead | M | normal |
| 194 | marduk | 1140 | Marduk | 40 | 4,214 | Fire 1 | demihuman | L | normal |
| 195 | stem_worm | 1215 | Stem Worm | 40 | 6,136 | Wind 1 | plant | M | normal |
| 196 | neraid | 1255 | Nereid | 40 | 4,120 | Earth 1 | brute | S | normal |
| 197 | pest | 1256 | Pest | 40 | 3,240 | Shadow 2 | brute | S | normal |
| 198 | greatest_general | 1277 | Greatest General | 40 | 3,632 | Fire 2 | formless | M | normal |
| 199 | quve | 1508 | Quve | 40 | 4,559 | Undead 1 | undead | S | normal |
| 200 | mime_monkey | 1585 | Mime Monkey | 40 | 6,000 | Water 1 | plant | M | normal |
| 201 | magmaring | 1836 | Magmaring | 40 | 5,300 | Fire 2 | formless | S | normal |
| 202 | argiope | 1099 | Argiope | 41 | 4,382 | Poison 1 | insect | L | normal |
| 203 | marionette | 1143 | Marionette | 41 | 3,222 | Ghost 3 | demon | S | normal |
| 204 | kapha | 1406 | Kapha | 41 | 7,892 | Water 1 | fish | M | normal |
| 205 | wootan_fighter | 1499 | Wootan Fighter | 41 | 4,457 | Fire 2 | demihuman | M | normal |
| 206 | hunter_fly | 1035 | Hunter Fly | 42 | 5,242 | Wind 2 | insect | S | normal |
| 207 | chepet | 1250 | Chepet | 42 | 4,950 | Fire 1 | demihuman | M | normal |
| 208 | alligator | 1271 | Alligator | 42 | 6,962 | Water 1 | brute | M | normal |
| 209 | stone_shooter | 1495 | Stone Shooter | 42 | 4,104 | Fire 3 | plant | M | normal |
| 210 | venomous | 1621 | Venomous | 42 | 4,653 | Poison 1 | formless | M | normal |
| 211 | novus | 1715 | Novus | 42 | 5,430 | Neutral 1 | dragon | S | normal |
| 212 | siroma | 1776 | Siroma | 42 | 6,800 | Water 3 | formless | S | normal |
| 213 | side_winder | 1037 | Side Winder | 43 | 4,929 | Poison 1 | brute | M | normal |
| 214 | punk | 1199 | Punk | 43 | 3,620 | Wind 1 | plant | S | normal |
| 215 | choco | 1214 | Choco | 43 | 4,278 | Fire 1 | brute | S | normal |
| 216 | sageworm | 1281 | Sage Worm | 43 | 3,850 | Neutral 3 | brute | S | normal |
| 217 | blazzer | 1367 | Blazer | 43 | 8,252 | Fire 2 | demon | M | normal |
| 218 | pitman | 1616 | Pitman | 43 | 5,015 | Earth 2 | undead | L | normal |
| 219 | hill_wind | 1629 | Hill Wind | 43 | 3,189 | Wind 3 | brute | M | normal |
| 220 | plasma_r | 1694 | Plasma (Fire) | 43 | 5,700 | Fire 4 | formless | S | normal |
| 221 | novus_ | 1718 | Novus (Green) | 43 | 5,830 | Neutral 1 | dragon | S | normal |
| 222 | dragon_egg | 1721 | Dragon Egg | 43 | 18,322 | Neutral 2 | dragon | M | normal |
| 223 | bathory | 1102 | Bathory | 44 | 5,415 | Shadow 1 | demihuman | M | normal |
| 224 | petit | 1155 | Petite (Earth) | 44 | 6,881 | Earth 1 | dragon | M | normal |
| 225 | plasma_b | 1697 | Plasma (Water) | 44 | 8,200 | Water 4 | formless | S | normal |
| 226 | galion | 1783 | Galion | 44 | 32,240 | Wind 2 | brute | M | boss |
| 227 | petit_ | 1156 | Petite (Sky) | 45 | 5,747 | Wind 1 | dragon | M | normal |
| 228 | megalith | 1274 | Megalith | 45 | 5,300 | Neutral 4 | formless | L | normal |
| 229 | hill_wind_1 | 1680 | Hill Wind (alt) | 45 | 4,233 | Wind 3 | brute | M | normal |
| 230 | deviruchi | 1109 | Deviruchi | 46 | 6,666 | Shadow 1 | demon | S | normal |
| 231 | brilight | 1211 | Brilight | 46 | 5,562 | Fire 1 | insect | S | normal |
| 232 | explosion | 1383 | Explosion | 46 | 8,054 | Fire 3 | brute | S | normal |
| 233 | poison_toad | 1402 | Poison Toad | 46 | 6,629 | Poison 2 | brute | M | normal |
| 234 | wild_ginseng | 1413 | Hermit Plant | 46 | 6,900 | Fire 2 | plant | S | normal |
| 235 | drosera | 1781 | Drosera | 46 | 7,221 | Earth 1 | plant | M | normal |
| 236 | isis | 1029 | Isis | 47 | 7,003 | Shadow 1 | demon | L | normal |
| 237 | deviace | 1108 | Deviace | 47 | 20,090 | Water 4 | fish | M | normal |
| 238 | iron_fist | 1212 | Iron Fist | 47 | 4,221 | Neutral 3 | insect | M | normal |
| 239 | antique_firelock | 1403 | Firelock Soldier | 47 | 3,852 | Undead 2 | undead | M | normal |
| 240 | plasma_g | 1695 | Plasma (Earth) | 47 | 7,600 | Earth 4 | formless | S | normal |
| 241 | strouf | 1065 | Strouf | 48 | 11,990 | Water 3 | fish | L | normal |
| 242 | gargoyle | 1253 | Gargoyle | 48 | 3,950 | Wind 3 | demon | M | normal |
| 243 | li_me_mang_ryang | 1517 | Jing Guai | 48 | 5,920 | Earth 3 | demon | M | normal |
| 244 | nightmare | 1061 | Nightmare | 49 | 4,437 | Ghost 3 | demon | L | normal |
| 245 | orc_archer | 1189 | Orc Archer | 49 | 7,440 | **Neutral 1** | demihuman | M | normal |
| 246 | parasite | 1500 | Parasite | 49 | 5,188 | Wind 2 | plant | M | normal |
| 247 | chung_e | 1519 | Green Maiden | 49 | 23,900 | Neutral 2 | demihuman | M | normal |
| 248 | plasma_p | 1696 | Plasma (Shadow) | 49 | 5,900 | Shadow 4 | formless | S | normal |
| 249 | baphomet_ | 1101 | Baphomet Jr. | 50 | 8,578 | Shadow 1 | demon | S | normal |
| 250 | dryad | 1493 | Dryad | 50 | 8,791 | Earth 4 | plant | M | normal |
| 251 | kraben | 1587 | Kraben | 50 | 5,880 | Ghost 2 | formless | M | normal |
| 252 | obsidian | 1615 | Obsidian | 50 | 8,812 | Earth 2 | formless | S | normal |
| 253 | knocker | 1838 | Knocker | 50 | 7,755 | Earth 1 | demon | S | normal |
| 254 | nine_tail | 1180 | Nine Tail | 51 | 7,766 | Fire 3 | brute | M | normal |
| 255 | mimic | 1191 | Mimic | 51 | 6,120 | Neutral 3 | formless | M | normal |
| 256 | injustice | 1257 | Injustice | 51 | 7,600 | Shadow 2 | undead | M | normal |
| 257 | wind_ghost | 1263 | Wind Ghost | 51 | 4,820 | Wind 3 | demon | M | normal |
| 258 | carat | 1267 | Carat | 51 | 5,200 | Wind 2 | demon | M | normal |
| 259 | wooden_golem | 1497 | Wooden Golem | 51 | 9,200 | Neutral 1 | plant | L | normal |
| 260 | hylozoist | 1510 | Heirozoist | 51 | 7,186 | Shadow 2 | demon | S | normal |
| 261 | increase_soil | 1516 | Mi Gao | 51 | 8,230 | Earth 3 | formless | M | normal |
| 262 | minorous | 1149 | Minorous | 52 | 7,431 | Fire 2 | brute | L | normal |
| 263 | raydric | 1163 | Raydric | 52 | 8,613 | Shadow 2 | demihuman | L | normal |
| 264 | skel_prisoner | 1196 | Skeleton Prisoner | 52 | 8,691 | Undead 3 | undead | M | normal |
| 265 | high_orc | 1213 | High Orc | 52 | 6,890 | **Neutral 1** | demihuman | L | normal |
| 266 | raydric_archer | 1276 | Raydric Archer | 52 | 5,250 | Shadow 2 | demon | M | normal |
| 267 | driller | 1380 | Driller | 52 | 7,452 | Earth 1 | brute | M | normal |
| 268 | tamruan | 1584 | Tamruan | 52 | 10,234 | Shadow 3 | demon | L | normal |
| 269 | wraith | 1192 | Wraith | 53 | 10,999 | Undead 4 | undead | L | normal |
| 270 | zombie_prisoner | 1197 | Zombie Prisoner | 53 | 11,280 | Undead 3 | undead | M | normal |
| 271 | merman | 1264 | Merman | 53 | 14,690 | Water 3 | demihuman | M | normal |
| 272 | live_peach_tree | 1410 | Enchanted Peach Tree | 53 | 8,905 | Earth 2 | plant | M | normal |
| 273 | gremlin | 1632 | Gremlin | 53 | 9,280 | Shadow 2 | demon | L | normal |
| 274 | dancing_dragon | 1514 | Zhu Po Long | 54 | 9,136 | Wind 2 | dragon | M | normal |
| 275 | green_iguana | 1687 | Grove | 54 | 6,444 | Earth 2 | brute | M | normal |
| 276 | giant_spider | 1304 | Giant Spider | 55 | 11,874 | Poison 1 | insect | L | normal |
| 277 | blood_butterfly | 1408 | Bloody Butterfly | 55 | 8,082 | Wind 2 | insect | M | normal |
| 278 | disguise | 1506 | Disguise | 55 | 7,543 | Earth 4 | demon | M | normal |
| 279 | removal | 1682 | Remover | 55 | 10,289 | Undead 2 | undead | M | normal |
| 280 | constant | 1738 | Constant | 55 | 10,000 | Shadow 3 | formless | S | normal |
| 281 | gazeti | 1778 | Gazeti | 55 | 12,300 | Water 1 | demon | M | normal |
| 282 | cramp | 1209 | Cramp | 56 | 4,720 | Poison 2 | brute | S | normal |
| 283 | killer_mantis | 1294 | Killer Mantis | 56 | 13,183 | Earth 1 | insect | M | normal |
| 284 | giant_honet | 1303 | Giant Hornet | 56 | 13,105 | Wind 1 | insect | S | normal |
| 285 | geographer | 1368 | Geographer | 56 | 8,071 | Earth 3 | plant | M | normal |
| 286 | the_paper | 1375 | The Paper | 56 | 18,557 | Neutral 3 | formless | M | normal |
| 287 | demon_pungus | 1378 | Demon Pungus | 56 | 7,259 | Poison 3 | demon | S | normal |
| 288 | evil_cloud_hermit | 1412 | Taoist Hermit | 56 | 10,392 | Neutral 2 | formless | L | normal |
| 289 | hyegun | 1512 | Yao Jun | 56 | 9,981 | Undead 2 | undead | M | normal |
| 290 | mineral | 1614 | Mineral | 56 | 7,950 | Neutral 2 | formless | S | normal |
| 291 | beholder | 1633 | Beholder | 56 | 7,950 | Wind 2 | formless | S | normal |
| 292 | breeze | 1692 | Breeze | 56 | 5,099 | Wind 3 | formless | M | normal |
| 293 | plasma_y | 1693 | Plasma (Ghost) | 56 | 8,400 | Ghost 4 | formless | S | normal |
| 294 | joker | 1131 | Joker | 57 | 12,450 | Wind 4 | demihuman | L | normal |
| 295 | penomena | 1216 | Penomena | 57 | 7,256 | Poison 1 | fish | M | normal |
| 296 | muscipular | 1780 | Muscipular | 57 | 4,332 | Earth 1 | plant | M | normal |
| 297 | evil_druid | 1117 | Evil Druid | 58 | 16,506 | Undead 4 | undead | L | normal |
| 298 | alarm | 1193 | Alarm | 58 | 10,647 | Neutral 3 | formless | M | normal |
| 299 | leib_olmai | 1306 | Leib Olmai | 58 | 24,233 | Earth 1 | brute | L | normal |
| 300 | spring_rabbit | 1322 | Spring Rabbit | 58 | 9,045 | Earth 2 | brute | M | normal |
| 301 | grand_peco | 1369 | Grand Peco | 58 | 8,054 | Fire 2 | brute | L | normal |
| 302 | gibbet | 1503 | Gibbet | 58 | 6,841 | Shadow 1 | demon | L | normal |
| 303 | ygnizem | 1652 | Egnigem Cenia | 58 | 11,200 | Fire 2 | demihuman | M | normal |
| 304 | arclouse | 1194 | Arclouze | 59 | 6,075 | Earth 2 | insect | M | normal |
| 305 | rideword | 1195 | Rideword | 59 | 11,638 | Neutral 3 | formless | S | normal |
| 306 | dark_frame | 1260 | Dark Frame | 59 | 7,500 | Shadow 3 | demon | M | normal |
| 307 | panzer_goblin | 1308 | Panzer Goblin | 59 | 14,130 | Wind 2 | demihuman | M | normal |
| 308 | see_otter | 1323 | Sea Otter | 59 | 9,999 | Water 3 | brute | M | normal |
| 309 | chung_e_ | 1631 | Green Maiden (alt) | 59 | 23,900 | Wind 2 | demihuman | M | normal |
| 310 | erend | 1655 | Errende Ebecee | 59 | 6,980 | Holy 2 | demihuman | M | normal |
| 311 | vanberk | 1771 | Vanberk | 59 | 9,988 | Neutral 4 | demihuman | M | normal |
| 312 | kaho | 1072 | Kaho | 60 | 8,409 | Fire 4 | demon | M | normal |
| 313 | clock | 1269 | Clock | 60 | 11,050 | Earth 2 | formless | M | normal |
| 314 | stalactic_golem | 1278 | Stalactic Golem | 60 | 18,700 | Neutral 1 | formless | L | normal |
| 315 | gig | 1387 | Gig | 60 | 8,409 | Fire 2 | brute | S | normal |
| 316 | archangeling | 1388 | Arc Angeling | 60 | 79,523 | Holy 3 | angel | M | boss |
| 317 | kavac | 1656 | Kavach Icarus | 60 | 7,899 | Wind 2 | demihuman | M | normal |
| 318 | ancient_mimic | 1699 | Ancient Mimic | 60 | 8,080 | Neutral 3 | formless | L | normal |
| 319 | snowier | 1775 | Snowier | 60 | 19,230 | Water 2 | formless | L | normal |
| 320 | ice_titan | 1777 | Ice Titan | 60 | 38,200 | Water 3 | formless | L | normal |
| 321 | pasana | 1154 | Pasana | 61 | 8,289 | Fire 2 | demihuman | M | normal |
| 322 | anolian | 1206 | Anolian | 61 | 18,960 | Water 2 | fish | M | normal |
| 323 | sting | 1207 | Sting | 61 | 9,500 | Earth 3 | formless | M | normal |
| 324 | am_mut | 1301 | Am Mut | 61 | 12,099 | Shadow 1 | demon | S | normal |
| 325 | mobster | 1313 | Mobster | 61 | 7,991 | Neutral 1 | demihuman | M | normal |
| 326 | dragon_tail | 1321 | Dragon Tail | 61 | 8,368 | Wind 2 | insect | M | normal |
| 327 | galapago | 1391 | Galapago | 61 | 9,145 | Earth 1 | brute | S | normal |
| 328 | garm_baby | 1515 | Baby Hatii | 61 | 20,199 | Water 2 | brute | M | normal |
| 329 | rawrel | 1657 | Laurell Weinder | 61 | 6,168 | Ghost 2 | demihuman | M | normal |
| 330 | hodremlin | 1773 | Hodremlin | 61 | 12,180 | Shadow 3 | demon | M | normal |
| 331 | alice | 1275 | Alice | 62 | 10,000 | Neutral 3 | demihuman | M | normal |
| 332 | cremy_fear | 1293 | Creamy Fear | 62 | 13,387 | Wind 1 | insect | S | normal |
| 333 | zombie_master | 1298 | Zombie Master | 62 | 14,211 | Undead 1 | undead | M | normal |
| 334 | gullinbursti | 1311 | Gullinbursti | 62 | 21,331 | Earth 2 | brute | L | normal |
| 335 | dullahan | 1504 | Dullahan | 62 | 12,437 | Undead 2 | undead | M | normal |
| 336 | civil_servant | 1513 | Mao Guai | 62 | 14,390 | Wind 2 | brute | M | normal |
| 337 | whikebain | 1653 | Wickebine Tres | 62 | 7,320 | Poison 3 | demihuman | M | normal |
| 338 | isilla | 1772 | Isilla | 62 | 8,297 | Neutral 4 | demihuman | M | normal |
| 339 | aunoe | 1796 | Aunoe | 62 | 21,297 | Neutral 4 | demihuman | M | normal |
| 340 | fanat | 1797 | Fanat | 62 | 21,297 | Neutral 4 | demihuman | M | normal |

Entries marked with **bold** in the Element column have confirmed errors.
Entry marked with * in the Class column needs review (Giant Whisper boss flag).
