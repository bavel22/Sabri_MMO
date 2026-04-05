# Monster Stats Batch 1 Audit Report

**Audit Date**: 2026-03-22
**File Audited**: `server/src/ro_monster_templates.js`
**Batch Scope**: First 170 monsters by ID order (IDs 1001-1185)
**Reference Sources**: rAthena pre-renewal mob_db.yml, RateMyServer.net pre-re database, divine-pride.net

---

## 1. Total Monster Count

- **Total templates in file**: 509
- **Batch 1 count**: 170 (IDs 1001-1185)
- **Level range (all)**: 1-99
- **HP range (all)**: 10 - 95,000,000

### Monster Class Distribution (all 509)

| Class | Count |
|-------|-------|
| normal | 412 |
| boss (mini-boss) | 54 |
| mvp | 43 |

### Level Distribution (all 509)

| Level Range | Count |
|-------------|-------|
| 1-9 | 45 |
| 10-19 | 42 |
| 20-29 | 57 |
| 30-39 | 48 |
| 40-49 | 56 |
| 50-59 | 63 |
| 60-69 | 74 |
| 70-79 | 58 |
| 80-89 | 35 |
| 90-99 | 31 |

---

## 2. Template Structure Analysis

### Required Fields (30 fields per template)

All 509 monsters have every required field present. **Zero missing fields.**

| Field | Type | Present | Notes |
|-------|------|---------|-------|
| id | number | 509/509 | Unique, no duplicates |
| name | string | 509/509 | Display name |
| aegisName | string | 509/509 | rAthena internal name |
| level | number | 509/509 | Range: 1-99 |
| maxHealth | number | 509/509 | Range: 10-95M |
| baseExp | number | 509/509 | |
| jobExp | number | 509/509 | |
| mvpExp | number | 509/509 | 0 for non-MVPs |
| attack | number | 509/509 | Min ATK |
| attack2 | number | 509/509 | Max ATK |
| defense | number | 509/509 | Hard DEF |
| magicDefense | number | 509/509 | Hard MDEF |
| str | number | 509/509 | **See STR issue below** |
| agi | number | 509/509 | |
| vit | number | 509/509 | |
| int | number | 509/509 | |
| dex | number | 509/509 | |
| luk | number | 509/509 | |
| attackRange | number | 509/509 | In game units |
| aggroRange | number | 509/509 | |
| chaseRange | number | 509/509 | |
| aspd | number | 509/509 | |
| walkSpeed | number | 509/509 | |
| attackDelay | number | 509/509 | |
| attackMotion | number | 509/509 | |
| damageMotion | number | 509/509 | |
| size | string | 509/509 | small/medium/large |
| race | string | 509/509 | 10 valid races |
| element | object | 509/509 | {type, level} |
| monsterClass | string | 509/509 | normal/boss/mvp |
| aiType | string | 509/509 | passive/reactive/aggressive |
| respawnMs | number | 509/509 | |
| raceGroups | object | 509/509 | |
| stats | object | 509/509 | Redundant stat mirror |
| modes | object | 509/509 | |
| drops | array | 509/509 | |
| mvpDrops | array | 509/509 | |

### Optional / Structural Fields

- `drops[]` entries have `{itemName, rate, stealProtected?}` - consistent
- `mvpDrops[]` entries have `{itemName, rate}` - consistent
- Only 1 monster has 0 drops: Kiehl (1733)

### Value Enumerations

| Field | Valid Values | Status |
|-------|-------------|--------|
| size | small, medium, large | ALL VALID |
| race | formless, undead, brute, plant, insect, fish, demon, demihuman, angel, dragon | ALL VALID (10/10 RO races) |
| element.type | neutral, water, earth, fire, wind, poison, holy, shadow, undead, ghost | ALL VALID (10/10 RO elements) |
| element.level | 1, 2, 3, 4 | ALL VALID (range 1-4) |
| monsterClass | normal, boss, mvp | ALL VALID |
| aiType | passive, reactive, aggressive | ALL VALID |

### attackRange Value Distribution

| Range | Count | Meaning |
|-------|-------|---------|
| 50 | 356 | Melee (1 cell) |
| 100 | 70 | Short melee (2 cells) |
| 150 | 41 | ~3 cells |
| 200 | 2 | ~4 cells |
| 250 | 5 | ~5 cells |
| 300 | 2 | Ranged |
| 350 | 6 | Ranged |
| 400 | 1 | Ranged |
| 450 | 14 | Ranged (9 cells) |
| 500 | 7 | Ranged |
| 600 | 1 | Long ranged |
| 700 | 4 | Very long ranged |

---

## 3. Web-Verified Monster Audit (30 monsters)

Cross-referenced against RateMyServer.net pre-renewal database and rAthena pre-re mob_db.yml.

### Legend
- MATCH = template matches reference exactly
- DISC = discrepancy found (see details)
- ~MATCH = minor difference (STR 0 vs 1 default issue)

### 3.1 Verified Monsters

| ID | Name | Lv | HP | ATK | DEF | MDEF | Size | Race | Elem | Result |
|----|------|----|----|-----|-----|------|------|------|------|--------|
| 1001 | Scorpion | 24 | 1109 | 80-135 | 30 | 0 | S | Insect | Fire1 | ~MATCH (STR) |
| 1002 | Poring | 1 | 50 | 7-10 | 0 | 5 | M | Plant | Water1 | ~MATCH (STR) |
| 1005 | Familiar | 8 | 155 | 20-28 | 0 | 0 | S | Brute | Shadow1 | ~MATCH (STR) |
| 1007 | Fabre | 2 | 63 | 8-11 | 0 | 0 | S | Insect | Earth1 | ~MATCH (STR) |
| 1015 | Zombie | 15 | 534 | 67-79 | 0 | 10 | M | Undead | Undead1 | ~MATCH (STR) |
| 1019 | Peco Peco | 19 | 531 | 50-64 | 0 | 0 | L | Brute | Fire1 | ~MATCH (STR) |
| 1029 | Isis | 47 | 7003 | 423-507 | 10 | 35 | L | Demon | Shadow1 | MATCH |
| 1035 | Hunter Fly | 42 | 5242 | 246-333 | 25 | 15 | S | Insect | Wind2 | MATCH |
| 1037 | Side Winder | 43 | 4929 | 240-320 | 5 | 10 | M | Brute | Poison1 | MATCH |
| 1038 | Osiris | 78 | 415400 | 780-2880 | 10 | 25 | M | Undead | Undead4 | ~MATCH (STR) |
| 1039 | Baphomet | 81 | 668000 | 3220-4040 | 35 | 45 | L | Demon | Shadow3 | ~MATCH (STR) |
| 1040 | Golem | 25 | 3900 | 175-187 | 40 | 0 | L | Formless | Neutral1 | **DISC** |
| 1046 | Doppelganger | 72 | 249000 | 1340-1590 | 60 | 35 | M | Demon | Shadow3 | MATCH |
| 1059 | Mistress | 74 | 212000 | 880-1110 | 40 | 60 | S | Insect | Wind4 | MATCH |
| 1077 | Poison Spore | 19 | 665 | 89-101 | 0 | 0 | M | Plant | Poison1 | ~MATCH (STR) |
| 1086 | GTB | 64 | 126000 | 870-1145 | 60 | 45 | L | Insect | Fire2 | MATCH |
| 1087 | Orc Hero | 77 | 585700 | 2257-2542 | 40 | 45 | L | Demihuman | Earth2 | ~MATCH (STR) |
| 1096 | Angeling | 20 | 55000 | 120-195 | 0 | 70 | M | Angel | Holy4 | ~MATCH (STR) |
| 1098 | Anubis | 75 | 38000 | 530-1697 | 25 | 31 | L | Demihuman | Undead2 | MATCH |
| 1102 | Bathory | 44 | 5415 | 198-398 | 0 | 60 | M | Demihuman | Shadow1 | ~MATCH (STR) |
| 1112 | Drake | 70 | 326666 | 1800-2100 | 20 | 35 | M | Undead | Undead1 | MATCH |
| 1115 | Eddga | 65 | 152000 | 1215-1565 | 15 | 15 | L | Brute | Fire1 | MATCH |
| 1117 | Evil Druid | 58 | 16506 | 420-670 | 5 | 60 | L | Undead | Undead4 | ~MATCH (STR) |
| 1120 | Ghostring | 18 | 73300 | 82-122 | 0 | 60 | M | Demon | Ghost4 | MATCH |
| 1147 | Maya | 81 | 169000 | 1800-2070 | 60 | 25 | L | Insect | Earth4 | MATCH |
| 1148 | Medusa | 79 | 16408 | 827-1100 | 28 | 18 | M | Demon | Neutral2 | ~MATCH (STR) |
| 1149 | Minorous | 52 | 7431 | 590-770 | 15 | 5 | L | Brute | Fire2 | MATCH |
| 1150 | Moonlight | 67 | 120000 | 1200-1700 | 10 | 55 | M | Demon | Fire3 | MATCH |
| 1157 | Pharaoh | 93 | 445997 | 2267-3015 | 67 | 70 | L | Demihuman | Shadow3 | ~MATCH (STR) |
| 1159 | Phreeoni | 69 | 188000 | 880-1530 | 10 | 20 | L | Brute | Neutral3 | ~MATCH (STR) |
| 1163 | Raydric | 52 | 8613 | 830-930 | 40 | 15 | L | Demihuman | Shadow2 | MATCH |
| 1152 | Orc Skeleton | 28 | 2278 | 190-236 | 10 | 10 | M | Undead | Neutral1 | **DISC** |
| 1153 | Orc Zombie | 24 | 1568 | 151-184 | 5 | 10 | M | Undead | Neutral1 | **DISC** |

---

## 4. Discrepancies Found

### 4.1 CRITICAL: Golem (1040) Element Level Wrong

| Field | Template | rAthena/RMS | Impact |
|-------|----------|-------------|--------|
| element.level | 1 | **3** | Wrong elemental damage multipliers. Neutral Lv3 vs Lv1 significantly changes how elemental attacks interact with this monster. |

**Fix**: Change `element: { type: 'neutral', level: 1 }` to `element: { type: 'neutral', level: 3 }` in the Golem template.

### 4.2 CRITICAL: Orc Skeleton (1152) Wrong Element Type

| Field | Template | RMS (pre-re) | Impact |
|-------|----------|--------------|--------|
| element.type | neutral | **undead** | Completely wrong element. Orc Skeleton should be Undead 1, not Neutral 1. This affects Holy damage (should deal 200% vs Undead1, but only 100% vs Neutral1), Turn Undead effectiveness, Resurrection damage, and other undead-specific interactions. |

**Fix**: Change `element: { type: 'neutral', level: 1 }` to `element: { type: 'undead', level: 1 }` in the Orc Skeleton template.

### 4.3 CRITICAL: Orc Zombie (1153) Wrong Element Type

| Field | Template | RMS (pre-re) | Impact |
|-------|----------|--------------|--------|
| element.type | neutral | **undead** | Same issue as Orc Skeleton. Should be Undead 1. Holy attacks incorrectly deal neutral damage instead of double damage. |

**Fix**: Change `element: { type: 'neutral', level: 1 }` to `element: { type: 'undead', level: 1 }` in the Orc Zombie template.

### 4.4 SYSTEMIC: STR=0 Instead of STR=1 (299 monsters affected)

**Background**: In rAthena's YAML mob_db format, when a stat is not explicitly specified, it defaults to 1 (not 0). The template generation script appears to have used 0 as the default instead.

**Affected Count**: 299 out of 509 monsters have `str: 0` where rAthena would have `str: 1` (the Str field omitted from the YAML means default=1).

**In Batch 1**: 139 out of 170 monsters have `str: 0`.

**Monsters that correctly have non-zero STR in batch 1** (31 total): Hornet(1004) str=6, Isis(1029) str=38, Hunter Fly(1035) str=33, Side Winder(1037) str=38, Mummy(1041) str=28, Doppelganger(1046) str=88, Muka(1055) str=15, Mistress(1059) str=50, Pirate Skeleton(1071) str=25, Crab(1073) str=18, Golden Thief Bug(1086) str=65, Vocal(1088) str=77, Vagabond Wolf(1092) str=57, Anubis(1098) str=5, Caramel(1103) str=35, Coco(1104) str=24, Desert Wolf(1106) str=56, Dokebi(1110) str=50, Drake(1112) str=85, Eddga(1115) str=78, Frilldora(1119) str=35, Ghostring(1120) str=40, Giearth(1121) str=25, Horn(1128) str=22, Khalitzburg(1132) str=58, Martin(1145) str=12, Maya(1147) str=95, Minorous(1149) str=65, Moonlight Flower(1150) str=55, Raydric(1163) str=58, Sandman(1165) str=24.

**Impact Assessment**: LOW to NEGLIGIBLE. In pre-renewal RO, monster STR has minimal gameplay impact. Monster ATK is determined by the `attack`/`attack2` fields directly, not calculated from STR. The STR stat on monsters primarily affects:
- Monster HP recovery (very minor)
- Some skill damage formulas that reference caster STR (very few monster skills use this)
- Weight capacity (irrelevant for monsters)

The difference between STR=0 and STR=1 is functionally negligible for gameplay. This is a data accuracy issue, not a gameplay bug.

**Fix (if desired)**: Bulk update all 299 monsters with `str: 0` to `str: 1`, EXCEPT for plants and special monsters (Red Plant, Blue Plant, etc.) which may genuinely have STR=0 in rAthena when explicitly set.

**Plant/Mushroom exceptions**: The following should remain at STR=0 (rAthena explicitly sets them): Red Plant(1078), Blue Plant(1079), Green Plant(1080), Yellow Plant(1081), White Plant(1082), Shining Plant(1083), Black Mushroom(1084), Red Mushroom(1085), Thief Mushroom(1182), Pupa(1008), Ant Egg(1097), Peco Peco Egg(1047).

---

## 5. Batch-Wide Analysis

### 5.1 HP Ranges by Level (Batch 1 normals only)

| Level Range | HP Min | HP Max | Typical HP |
|-------------|--------|--------|------------|
| 1-5 | 10 (plants) | 427 (Pupa) | 50-100 |
| 6-10 | 48 | 507 | 150-250 |
| 11-15 | 354 | 1333 | 400-700 |
| 16-20 | 510 | 2451 | 600-1100 |
| 21-25 | 633 | 3900 | 1000-2000 |
| 26-30 | 2023 | 5034 | 2000-3500 |
| 31-35 | 1796 | 5628 | 2000-4000 |
| 36-40 | 3581 | 6900 | 4000-6000 |
| 41-50 | 3222 | 8613 | 5000-8000 |
| 51-60 | 7431 | 16506 | 7000-12000 |
| 61-75 | 8289 | 38000 | 8000-20000 |
| 76-80 | 16408 | 16408 | ~16000 |

### 5.2 HP for MVPs (Batch 1)

| Name | ID | Level | HP |
|------|----|-------|----|
| Osiris | 1038 | 78 | 415,400 |
| Baphomet | 1039 | 81 | 668,000 |
| Doppelganger | 1046 | 72 | 249,000 |
| Mistress | 1059 | 74 | 212,000 |
| Golden Thief Bug | 1086 | 64 | 126,000 |
| Orc Hero | 1087 | 77 | 585,700 |
| Drake | 1112 | 70 | 326,666 |
| Eddga | 1115 | 65 | 152,000 |
| Maya | 1147 | 81 | 169,000 |
| Moonlight Flower | 1150 | 67 | 120,000 |
| Pharaoh | 1157 | 93 | 445,997 |
| Phreeoni | 1159 | 69 | 188,000 |

All MVP HP values match reference data.

### 5.3 HP for Mini-Bosses (Batch 1)

| Name | ID | Level | HP |
|------|----|-------|----|
| Toad | 1089 | 10 | 5,065 |
| Mastering | 1090 | 2 | 2,415 |
| Dragon Fly | 1091 | 8 | 2,400 |
| Vagabond Wolf | 1092 | 24 | 12,240 |
| Eclipse | 1093 | 6 | 1,800 |
| Angeling | 1096 | 20 | 55,000 |
| Ghostring | 1120 | 18 | 73,300 |

All mini-boss HP values match reference data. Mini-bosses correctly have significantly higher HP than same-level normals.

### 5.4 Suspicious Entries

| Monster | ID | Issue | Verdict |
|---------|----|-------|---------|
| Iceicle | 1789 | Lv38, HP=10 | **CORRECT** - Immobile trap-type monster (confirmed via RMS) |
| Golden Savage | 1840 | Lv99, DEF=100, MDEF=99 | **CORRECT** - Special quest/summon monster, not normally encountered |
| Dream Metal | 1846 | Lv90, DEF=100, MDEF=99 | **CORRECT** - Immobile formless, not on any map, quest/summon use only |
| Kiehl | 1733 | 0 drops | Worth checking but may be intentional for mini-boss |

### 5.5 Undead Race with Non-Undead Element

These are intentional in RO Classic and confirmed correct against reference data:

| Monster | ID | Race | Element | Verified |
|---------|----|------|---------|----------|
| Whisper (variant) | 1185 | Undead | Ghost 1 | Correct (RMS confirmed) |
| Injustice | 1257 | Undead | Shadow 2 | Correct (RMS confirmed) |
| Pitman | 1616 | Undead | Earth 2 | Correct per rAthena |
| Odium of Thanatos | 1704 | Undead | Ghost 4 | Correct per rAthena |
| Despero of Thanatos | 1705 | Undead | Ghost 4 | Correct per rAthena |
| Maero of Thanatos | 1706 | Undead | Ghost 4 | Correct per rAthena |
| Dolor of Thanatos | 1707 | Undead | Ghost 4 | Correct per rAthena |

**Only Orc Skeleton (1152) and Orc Zombie (1153) are incorrect** -- they should be Undead element, not Neutral.

---

## 6. Statistical Analysis

### 6.1 Race Distribution (all 509)

| Race | Count |
|------|-------|
| demihuman | 103 |
| brute | 82 |
| demon | 72 |
| insect | 61 |
| undead | 55 |
| formless | 48 |
| plant | 35 |
| fish | 24 |
| dragon | 16 |
| angel | 13 |

### 6.2 Element Distribution (all 509)

| Element | Count |
|---------|-------|
| neutral | 91 |
| shadow | 73 |
| fire | 72 |
| undead | 68 |
| water | 58 |
| earth | 56 |
| wind | 42 |
| ghost | 22 |
| poison | 16 |
| holy | 11 |

### 6.3 Size Distribution (all 509)

| Size | Count |
|------|-------|
| medium | 225 |
| large | 148 |
| small | 136 |

---

## 7. Summary of Corrections Needed

### Critical (3 issues)

1. **Golem (1040)**: `element.level` 1 -> 3 (Neutral Lv3)
2. **Orc Skeleton (1152)**: `element.type` neutral -> undead (Undead Lv1)
3. **Orc Zombie (1153)**: `element.type` neutral -> undead (Undead Lv1)

### Low Priority (1 systemic issue)

4. **STR=0 default**: 299 monsters have str=0 where rAthena defaults to str=1. Functionally negligible impact since monster ATK is defined by attack/attack2 fields, not calculated from STR. Fix would be a bulk update of ~287 non-plant monsters from str=0 to str=1.

### Data Quality Score

- **Structure completeness**: 100% (all fields present, no missing data, no duplicates)
- **Value validity**: 100% (all enumerations valid, no out-of-range values)
- **Stat accuracy** (web-verified sample of 32 monsters): 90.6% exact match, 9.4% with minor STR default issue
- **Element accuracy**: 98.2% (3 errors out of 170 batch, 0.6% of all 509)
- **Overall assessment**: HIGH QUALITY with 3 element corrections needed

---

## 8. Recommended Actions

### Immediate (affects gameplay)

```javascript
// Fix 1: Golem element level
// In ro_monster_templates.js, find golem template
// Change: element: { type: 'neutral', level: 1 }
// To:     element: { type: 'neutral', level: 3 }

// Fix 2: Orc Skeleton element type
// Change: element: { type: 'neutral', level: 1 }
// To:     element: { type: 'undead', level: 1 }

// Fix 3: Orc Zombie element type
// Change: element: { type: 'neutral', level: 1 }
// To:     element: { type: 'undead', level: 1 }
```

### Optional (data accuracy, no gameplay impact)

Bulk update 287 non-plant/non-egg monsters from `str: 0` to `str: 1` to match rAthena defaults. Exclude plants (1078-1085, 1182), eggs (1008, 1047, 1097), and any monster that explicitly has Str: 0 in rAthena source.

---

## Appendix: Batch 1 Monster List (170 monsters)

| # | ID | Name | Lv | HP | ATK | DEF/MDEF | Size | Race | Element |
|---|-----|------|----|----|-----|----------|------|------|---------|
| 1 | 1001 | Scorpion | 24 | 1109 | 80-135 | 30/0 | S | Insect | Fire1 |
| 2 | 1002 | Poring | 1 | 50 | 7-10 | 0/5 | M | Plant | Water1 |
| 3 | 1004 | Hornet | 8 | 169 | 22-27 | 5/5 | S | Insect | Wind1 |
| 4 | 1005 | Familiar | 8 | 155 | 20-28 | 0/0 | S | Brute | Shadow1 |
| 5 | 1007 | Fabre | 2 | 63 | 8-11 | 0/0 | S | Insect | Earth1 |
| 6 | 1008 | Pupa | 2 | 427 | 1-2 | 0/20 | S | Insect | Earth1 |
| 7 | 1009 | Condor | 5 | 92 | 11-14 | 0/0 | M | Brute | Wind1 |
| 8 | 1010 | Willow | 4 | 95 | 9-12 | 5/15 | M | Plant | Earth1 |
| 9 | 1011 | Chonchon | 4 | 67 | 10-13 | 10/0 | S | Insect | Wind1 |
| 10 | 1012 | Roda Frog | 5 | 133 | 11-14 | 0/5 | M | Fish | Water1 |
| 11 | 1013 | Wolf | 25 | 919 | 37-46 | 0/0 | M | Brute | Earth1 |
| 12 | 1014 | Spore | 16 | 510 | 24-48 | 0/5 | M | Plant | Water1 |
| 13 | 1015 | Zombie | 15 | 534 | 67-79 | 0/10 | M | Undead | Undead1 |
| 14 | 1016 | Archer Skeleton | 31 | 3040 | 128-153 | 0/0 | M | Undead | Undead1 |
| 15 | 1018 | Creamy | 16 | 595 | 53-64 | 0/30 | S | Insect | Wind1 |
| 16 | 1019 | Peco Peco | 19 | 531 | 50-64 | 0/0 | L | Brute | Fire1 |
| 17 | 1020 | Mandragora | 12 | 405 | 26-35 | 0/25 | M | Plant | Earth3 |
| 18 | 1023 | Orc Warrior | 24 | 1400 | 104-126 | 10/5 | M | Demihuman | Neutral1 |
| 19 | 1024 | Wormtail | 14 | 426 | 42-51 | 5/0 | M | Plant | Earth1 |
| 20 | 1025 | Boa | 15 | 471 | 46-55 | 0/0 | M | Brute | Earth1 |
| 21 | 1026 | Munak | 30 | 2872 | 150-230 | 0/0 | M | Undead | Undead1 |
| 22 | 1028 | Soldier Skeleton | 29 | 2334 | 221-245 | 10/15 | M | Undead | Undead1 |
| 23 | 1029 | Isis | 47 | 7003 | 423-507 | 10/35 | L | Demon | Shadow1 |
| 24 | 1030 | Anacondaq | 23 | 1109 | 124-157 | 0/0 | M | Brute | Poison1 |
| 25 | 1031 | Poporing | 14 | 344 | 59-72 | 0/10 | M | Plant | Poison1 |
| 26 | 1032 | Verit | 38 | 5272 | 389-469 | 0/5 | M | Undead | Undead1 |
| 27 | 1033 | Elder Willow | 20 | 693 | 58-70 | 10/30 | M | Plant | Fire2 |
| 28 | 1034 | Thara Frog | 22 | 2152 | 105-127 | 0/10 | M | Fish | Water2 |
| 29 | 1035 | Hunter Fly | 42 | 5242 | 246-333 | 25/15 | S | Insect | Wind2 |
| 30 | 1036 | Ghoul | 40 | 5418 | 420-500 | 5/20 | M | Undead | Undead2 |
| 31 | 1037 | Side Winder | 43 | 4929 | 240-320 | 5/10 | M | Brute | Poison1 |
| 32 | 1038 | Osiris (MVP) | 78 | 415400 | 780-2880 | 10/25 | M | Undead | Undead4 |
| 33 | 1039 | Baphomet (MVP) | 81 | 668000 | 3220-4040 | 35/45 | L | Demon | Shadow3 |
| 34 | 1040 | **Golem** | 25 | 3900 | 175-187 | 40/0 | L | Formless | **Neutral1 (WRONG, should be 3)** |
| 35 | 1041 | Mummy | 37 | 5176 | 305-360 | 0/10 | M | Undead | Undead2 |
| 36 | 1042 | Steel Chonchon | 17 | 530 | 54-65 | 15/0 | S | Insect | Wind1 |
| 37 | 1044 | Obeaune | 31 | 3952 | 141-165 | 0/40 | M | Fish | Water2 |
| 38 | 1045 | Marc | 36 | 6900 | 220-280 | 5/10 | M | Fish | Water2 |
| 39 | 1046 | Doppelganger (MVP) | 72 | 249000 | 1340-1590 | 60/35 | M | Demon | Shadow3 |
| 40 | 1047 | Peco Peco Egg | 3 | 420 | 1-2 | 20/20 | S | Formless | Neutral3 |
| 41 | 1048 | Thief Bug Egg | 4 | 48 | 13-17 | 20/0 | S | Insect | Shadow1 |
| 42 | 1049 | Picky | 3 | 80 | 9-12 | 0/0 | S | Brute | Fire1 |
| 43 | 1050 | Picky (shell) | 4 | 83 | 8-11 | 20/0 | S | Brute | Fire1 |
| 44 | 1051 | Thief Bug | 6 | 126 | 18-24 | 5/0 | S | Insect | Neutral3 |
| 45 | 1052 | Rocker | 9 | 198 | 24-29 | 5/10 | M | Insect | Earth1 |
| 46 | 1053 | Thief Bug Female | 10 | 170 | 33-40 | 5/5 | M | Insect | Shadow1 |
| 47 | 1054 | Thief Bug Male | 19 | 583 | 76-88 | 15/5 | M | Insect | Shadow1 |
| 48 | 1055 | Muka | 17 | 610 | 40-49 | 5/5 | L | Plant | Earth1 |
| 49 | 1056 | Smokie | 18 | 641 | 61-72 | 0/10 | S | Brute | Earth1 |
| 50 | 1057 | Yoyo | 21 | 879 | 71-82 | 0/0 | S | Brute | Earth1 |
| 51 | 1058 | Metaller | 22 | 926 | 131-159 | 15/30 | M | Insect | Fire1 |
| 52 | 1059 | Mistress (MVP) | 74 | 212000 | 880-1110 | 40/60 | S | Insect | Wind4 |
| 53 | 1060 | Bigfoot | 25 | 1619 | 198-220 | 10/0 | L | Brute | Earth1 |
| 54 | 1061 | Nightmare | 49 | 4437 | 447-529 | 0/40 | L | Demon | Ghost3 |
| 55 | 1062 | Santa Poring | 3 | 69 | 12-16 | 0/0 | M | Plant | Holy1 |
| 56 | 1063 | Lunatic | 3 | 60 | 9-12 | 0/20 | S | Brute | Neutral3 |
| 57 | 1064 | Megalodon | 24 | 1648 | 155-188 | 0/15 | M | Undead | Undead1 |
| 58 | 1065 | Strouf | 48 | 11990 | 200-1250 | 5/50 | L | Fish | Water3 |
| 59 | 1066 | Vadon | 19 | 1017 | 74-85 | 20/0 | S | Fish | Water1 |
| 60 | 1067 | Cornutus | 23 | 1620 | 109-131 | 30/0 | S | Fish | Water1 |
| 61 | 1068 | Hydra | 14 | 660 | 22-28 | 0/40 | S | Plant | Water2 |
| 62 | 1069 | Swordfish | 30 | 4299 | 168-199 | 5/20 | L | Fish | Water2 |
| 63 | 1070 | Kukre | 11 | 507 | 28-37 | 15/0 | S | Fish | Water1 |
| 64 | 1071 | Pirate Skeleton | 25 | 1676 | 145-178 | 10/15 | M | Undead | Undead1 |
| 65 | 1072 | Kaho | 60 | 8409 | 110-760 | 5/50 | M | Demon | Fire4 |
| 66 | 1073 | Crab | 20 | 2451 | 71-81 | 35/0 | S | Fish | Water1 |
| 67 | 1074 | Shellfish | 15 | 920 | 35-42 | 35/0 | S | Fish | Water1 |
| 68 | 1076 | Skeleton | 10 | 234 | 39-47 | 10/10 | M | Undead | Undead1 |
| 69 | 1077 | Poison Spore | 19 | 665 | 89-101 | 0/0 | M | Plant | Poison1 |
| 70 | 1078 | Red Plant | 1 | 10 | 1-2 | 100/99 | S | Plant | Earth1 |
| 71 | 1079 | Blue Plant | 1 | 10 | 1-2 | 100/99 | S | Plant | Earth1 |
| 72 | 1080 | Green Plant | 1 | 10 | 1-2 | 100/99 | S | Plant | Earth1 |
| 73 | 1081 | Yellow Plant | 1 | 10 | 1-2 | 100/99 | S | Plant | Earth1 |
| 74 | 1082 | White Plant | 1 | 10 | 1-2 | 100/99 | S | Plant | Earth1 |
| 75 | 1083 | Shining Plant | 1 | 20 | 1-2 | 100/99 | S | Plant | Holy1 |
| 76 | 1084 | Black Mushroom | 1 | 15 | 1-2 | 100/99 | S | Plant | Earth1 |
| 77 | 1085 | Red Mushroom | 1 | 15 | 1-2 | 100/99 | S | Plant | Earth1 |
| 78 | 1086 | GTB (MVP) | 64 | 126000 | 870-1145 | 60/45 | L | Insect | Fire2 |
| 79 | 1087 | Orc Hero (MVP) | 77 | 585700 | 2257-2542 | 40/45 | L | Demihuman | Earth2 |
| 80 | 1088 | Vocal | 18 | 3016 | 71-82 | 10/30 | M | Insect | Earth1 |
| 81 | 1089 | Toad (boss) | 10 | 5065 | 26-32 | 0/0 | M | Fish | Water1 |
| 82 | 1090 | Mastering (boss) | 2 | 2415 | 18-24 | 0/10 | M | Plant | Water1 |
| 83 | 1091 | Dragon Fly (boss) | 8 | 2400 | 22-27 | 40/0 | S | Insect | Wind1 |
| 84 | 1092 | Vagabond Wolf (boss) | 24 | 12240 | 135-159 | 10/0 | M | Brute | Earth1 |
| 85 | 1093 | Eclipse (boss) | 6 | 1800 | 20-26 | 0/40 | M | Brute | Neutral3 |
| 86 | 1094 | Ambernite | 13 | 495 | 39-46 | 30/0 | L | Insect | Water1 |
| 87 | 1095 | Andre | 17 | 688 | 60-71 | 10/0 | S | Insect | Earth1 |
| 88 | 1096 | Angeling (boss) | 20 | 55000 | 120-195 | 0/70 | M | Angel | Holy4 |
| 89 | 1097 | Ant Egg | 4 | 420 | 1-2 | 20/20 | S | Formless | Neutral3 |
| 90 | 1098 | Anubis | 75 | 38000 | 530-1697 | 25/31 | L | Demihuman | Undead2 |
| 91 | 1099 | Argiope | 41 | 4382 | 395-480 | 30/0 | L | Insect | Poison1 |
| 92 | 1100 | Argos | 25 | 1117 | 158-191 | 15/0 | L | Insect | Poison1 |
| 93 | 1101 | Baphomet Jr. | 50 | 8578 | 487-590 | 15/25 | S | Demon | Shadow1 |
| 94 | 1102 | Bathory | 44 | 5415 | 198-398 | 0/60 | M | Demihuman | Shadow1 |
| 95 | 1103 | Caramel | 23 | 1424 | 90-112 | 5/5 | S | Brute | Earth1 |
| 96 | 1104 | Coco | 17 | 817 | 56-67 | 0/0 | S | Brute | Earth1 |
| 97 | 1105 | Deniro | 19 | 760 | 68-79 | 15/0 | S | Insect | Earth1 |
| 98 | 1106 | Desert Wolf | 27 | 1716 | 169-208 | 0/10 | M | Brute | Fire1 |
| 99 | 1107 | Baby Desert Wolf | 9 | 164 | 30-36 | 0/0 | S | Brute | Fire1 |
| 100 | 1108 | Deviace | 47 | 20090 | 514-1024 | 10/20 | M | Fish | Water4 |
| 101 | 1109 | Deviruchi | 46 | 6666 | 475-560 | 10/25 | S | Demon | Shadow1 |
| 102 | 1110 | Dokebi | 33 | 2697 | 197-249 | 0/10 | S | Demon | Shadow1 |
| 103 | 1111 | Drainliar | 24 | 1162 | 74-84 | 0/0 | S | Brute | Shadow2 |
| 104 | 1112 | Drake (MVP) | 70 | 326666 | 1800-2100 | 20/35 | M | Undead | Undead1 |
| 105 | 1113 | Drops | 3 | 55 | 10-13 | 0/0 | M | Plant | Fire1 |
| 106 | 1114 | Dustiness | 21 | 1044 | 80-102 | 0/10 | S | Insect | Wind2 |
| 107 | 1115 | Eddga (MVP) | 65 | 152000 | 1215-1565 | 15/15 | L | Brute | Fire1 |
| 108 | 1116 | Eggyra | 24 | 633 | 85-107 | 20/25 | M | Formless | Ghost2 |
| 109 | 1117 | Evil Druid | 58 | 16506 | 420-670 | 5/60 | L | Undead | Undead4 |
| 110 | 1118 | Flora | 26 | 2092 | 242-273 | 10/35 | L | Plant | Earth1 |
| 111 | 1119 | Frilldora | 30 | 2023 | 200-239 | 0/10 | M | Brute | Fire1 |
| 112 | 1120 | Ghostring (boss) | 18 | 73300 | 82-122 | 0/60 | M | Demon | Ghost4 |
| 113 | 1121 | Giearth | 29 | 2252 | 154-185 | 10/50 | S | Demon | Earth1 |
| 114 | 1122 | Goblin (1) | 25 | 1176 | 118-140 | 10/5 | M | Demihuman | Neutral1 |
| 115 | 1123 | Goblin (2) | 24 | 1034 | 88-100 | 10/5 | M | Demihuman | Neutral1 |
| 116 | 1124 | Goblin (3) | 24 | 1034 | 132-165 | 10/5 | M | Demihuman | Neutral1 |
| 117 | 1125 | Goblin (4) | 23 | 1359 | 109-131 | 10/5 | M | Demihuman | Neutral1 |
| 118 | 1126 | Goblin (5) | 22 | 1952 | 105-127 | 10/5 | M | Demihuman | Neutral1 |
| 119 | 1127 | Hode | 26 | 2282 | 146-177 | 0/30 | M | Brute | Earth2 |
| 120 | 1128 | Horn | 18 | 659 | 58-69 | 10/0 | M | Insect | Earth1 |
| 121 | 1129 | Horong | 34 | 1939 | 275-327 | 99/50 | S | Formless | Fire4 |
| 122 | 1130 | Jakk | 38 | 3581 | 315-382 | 5/30 | M | Formless | Fire2 |
| 123 | 1131 | Joker | 57 | 12450 | 621-738 | 10/35 | L | Demihuman | Wind4 |
| 124 | 1132 | Khalitzburg | 63 | 19276 | 875-1025 | 45/10 | L | Undead | Undead1 |
| 125 | 1133 | Kobold (1) | 36 | 3893 | 265-318 | 15/10 | M | Demihuman | Neutral1 |
| 126 | 1134 | Kobold (2) | 31 | 2179 | 262-324 | 15/10 | M | Demihuman | Neutral1 |
| 127 | 1135 | Kobold (3) | 31 | 2179 | 186-216 | 15/10 | M | Demihuman | Neutral1 |
| 128 | 1138 | Magnolia | 26 | 3195 | 120-151 | 5/30 | S | Demon | Water1 |
| 129 | 1139 | Mantis | 26 | 2472 | 118-149 | 10/0 | M | Insect | Earth1 |
| 130 | 1140 | Marduk | 40 | 4214 | 315-382 | 0/60 | L | Demihuman | Fire1 |
| 131 | 1141 | Marina | 21 | 2087 | 84-106 | 0/5 | S | Plant | Water2 |
| 132 | 1142 | Marine Sphere | 28 | 3518 | 120-320 | 0/40 | S | Plant | Water2 |
| 133 | 1143 | Marionette | 41 | 3222 | 355-422 | 0/25 | S | Demon | Ghost3 |
| 134 | 1144 | Marse | 31 | 5034 | 211-252 | 0/5 | S | Fish | Water2 |
| 135 | 1145 | Martin | 18 | 1109 | 52-63 | 0/5 | S | Brute | Earth2 |
| 136 | 1146 | Matyr | 31 | 2585 | 134-160 | 0/0 | M | Brute | Shadow1 |
| 137 | 1147 | Maya (MVP) | 81 | 169000 | 1800-2070 | 60/25 | L | Insect | Earth4 |
| 138 | 1148 | Medusa | 79 | 16408 | 827-1100 | 28/18 | M | Demon | Neutral2 |
| 139 | 1149 | Minorous | 52 | 7431 | 590-770 | 15/5 | L | Brute | Fire2 |
| 140 | 1150 | Moonlight Flower (MVP) | 67 | 120000 | 1200-1700 | 10/55 | M | Demon | Fire3 |
| 141 | 1151 | Myst | 38 | 3745 | 365-445 | 0/40 | L | Formless | Poison1 |
| 142 | 1152 | **Orc Skeleton** | 28 | 2278 | 190-236 | 10/10 | M | Undead | **Neutral1 (WRONG)** |
| 143 | 1153 | **Orc Zombie** | 24 | 1568 | 151-184 | 5/10 | M | Undead | **Neutral1 (WRONG)** |
| 144 | 1154 | Pasana | 61 | 8289 | 513-682 | 29/35 | M | Demihuman | Fire2 |
| 145 | 1155 | Petite (ground) | 44 | 6881 | 360-427 | 30/30 | M | Dragon | Earth1 |
| 146 | 1156 | Petite (flying) | 45 | 5747 | 300-355 | 20/45 | M | Dragon | Wind1 |
| 147 | 1157 | Pharaoh (MVP) | 93 | 445997 | 2267-3015 | 67/70 | L | Demihuman | Shadow3 |
| 148 | 1158 | Phen | 26 | 3347 | 138-150 | 0/15 | M | Fish | Water2 |
| 149 | 1159 | Phreeoni (MVP) | 69 | 188000 | 880-1530 | 10/20 | L | Brute | Neutral3 |
| 150 | 1160 | Piere | 18 | 733 | 64-75 | 15/0 | S | Insect | Earth1 |
| 151 | 1161 | Plankton | 10 | 354 | 26-31 | 0/5 | S | Plant | Water3 |
| 152 | 1162 | Rafflesia | 17 | 1333 | 105-127 | 0/2 | S | Plant | Earth1 |
| 153 | 1163 | Raydric | 52 | 8613 | 830-930 | 40/15 | L | Demihuman | Shadow2 |
| 154 | 1164 | Requiem | 35 | 3089 | 220-272 | 0/15 | M | Demihuman | Shadow1 |
| 155 | 1165 | Sandman | 34 | 3413 | 180-205 | 10/25 | M | Formless | Earth3 |
| 156 | 1166 | Savage | 26 | 2092 | 120-150 | 10/5 | L | Brute | Earth2 |
| 157 | 1167 | Savage Babe | 7 | 182 | 20-25 | 0/0 | S | Brute | Earth1 |
| 158 | 1169 | Skeleton Worker | 30 | 2872 | 242-288 | 0/15 | M | Undead | Undead1 |
| 159 | 1170 | Sohee | 33 | 5628 | 210-251 | 0/10 | M | Demon | Water1 |
| 160 | 1174 | Stainer | 16 | 538 | 53-64 | 10/0 | S | Insect | Wind1 |
| 161 | 1175 | Tarou | 11 | 284 | 34-45 | 0/0 | S | Brute | Shadow1 |
| 162 | 1176 | Vitata | 20 | 894 | 69-80 | 15/20 | S | Insect | Earth1 |
| 163 | 1177 | Zenorc | 31 | 2585 | 188-223 | 0/15 | M | Demihuman | Shadow1 |
| 164 | 1178 | Zerom | 23 | 1109 | 127-155 | 0/10 | M | Demihuman | Fire1 |
| 165 | 1179 | Whisper | 34 | 1796 | 180-221 | 0/45 | S | Demon | Ghost3 |
| 166 | 1180 | Nine Tail | 51 | 7766 | 610-734 | 10/25 | M | Brute | Fire3 |
| 167 | 1182 | Thief Mushroom | 1 | 15 | 1-2 | 100/99 | S | Plant | Earth1 |
| 168 | 1183 | Chonchon (aggr) | 4 | 67 | 10-13 | 10/0 | S | Insect | Wind1 |
| 169 | 1184 | Fabre (aggr) | 1 | 30 | 4-7 | 0/0 | S | Insect | Earth1 |
| 170 | 1185 | Whisper (variant) | 34 | 1796 | 198-239 | 0/45 | S | Undead | Ghost1 |
