# Monster Element Verification Report

**Date**: 2026-03-23
**Source file**: `server/src/ro_monster_templates.js`
**Cross-reference sources**: rAthena pre-renewal mob_db.yml, RateMyServer pre-re, divine-pride.net, iRO Wiki Classic DB, pservero pre-renewal DB

---

## Round 1 Claimed Issues (6 monsters)

### 1. Golem (ID 1040) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `neutral` |
| element.level | `1` | `3` |

- **Line**: 3052
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Neutral Lv3 (confirmed divine-pride, iRO Wiki, rAthena pre-re mob_db.yml)
- **Verdict**: **CONFIRMED BUG** -- element level wrong (1 -> 3)

---

### 2. Orc Skeleton (ID 1152) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `undead` |
| element.level | `1` | `1` |

- **Line**: 3567
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Undead Lv1 (confirmed RateMyServer, divine-pride, pservero pre-re)
- **Note**: race is correctly `undead`, but element type is wrong
- **Verdict**: **CONFIRMED BUG** -- element type wrong (neutral -> undead)

---

### 3. Orc Zombie (ID 1153) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `undead` |
| element.level | `1` | `1` |

- **Line**: 2952
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Undead Lv1 (confirmed RateMyServer, pservero pre-re)
- **Note**: race is correctly `undead`, but element type is wrong
- **Verdict**: **CONFIRMED BUG** -- element type wrong (neutral -> undead)

---

### 4. Kobold (ID 1133) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `wind` |
| element.level | `1` | `2` |

- **Line**: 4605
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Wind Lv2 (confirmed RateMyServer, pservero pre-re)
- **Verdict**: **CONFIRMED BUG** -- element type AND level wrong (neutral/1 -> wind/2)

---

### 5. Orc Archer (ID 1189) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `earth` |
| element.level | `1` | `1` |

- **Line**: 6231
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Earth Lv1 (confirmed RateMyServer, pservero pre-re)
- **Verdict**: **CONFIRMED BUG** -- element type wrong (neutral -> earth)

---

### 6. High Orc (ID 1213) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `fire` |
| element.level | `1` | `2` |

- **Line**: 6742
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Fire Lv2 (confirmed RateMyServer, pservero pre-re)
- **Verdict**: **CONFIRMED BUG** -- element type AND level wrong (neutral/1 -> fire/2)

---

## Additional Monsters Checked (6 monsters)

### 7. Orc Lady (ID 1273, NOT 1272) -- CONFIRMED

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `neutral` | `earth` |
| element.level | `1` | `2` |

- **Line**: 4055
- **Code**: `element: { type: 'neutral', level: 1 }`
- **rAthena**: Earth Lv2 (confirmed RateMyServer)
- **Note**: Audit request had wrong ID (1272 is Dark Lord). Correct ID is 1273.
- **Verdict**: **CONFIRMED BUG** -- element type AND level wrong (neutral/1 -> earth/2)

---

### 8. Orc Baby (ID 1686, NOT 1368) -- NO ISSUE

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `earth` | `earth` |
| element.level | `1` | `1` |

- **Line**: 2467
- **Code**: `element: { type: 'earth', level: 1 }`
- **rAthena**: Earth Lv1 (confirmed RateMyServer)
- **Note**: Audit request had wrong ID (1368 is Geographer). Correct ID is 1686. Element is already correct.
- **Verdict**: **NO ISSUE** -- already correct

---

### 9. Verit (ID 1032) -- DENIED (already correct)

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `undead` | `undead` |
| element.level | `1` | `1` |

- **Line**: 4704
- **Code**: `element: { type: 'undead', level: 1 }`
- **rAthena**: Undead Lv1 (confirmed RateMyServer, divine-pride, iRO Wiki)
- **Note**: Audit claimed it should be "Dark 1" -- this is WRONG. All authoritative sources confirm Undead 1. The "Dark" element is called "Shadow" in RO, and Verit is Undead, not Shadow.
- **Verdict**: **DENIED** -- code is already correct. Audit claim was wrong.

---

### 10. Mummy (ID 1041) -- NO ISSUE

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `undead` | `undead` |
| element.level | `2` | `2` |

- **Line**: 4678
- **Code**: `element: { type: 'undead', level: 2 }`
- **rAthena**: Undead Lv2 (confirmed rAthena pre-re mob_db.yml)
- **Verdict**: **NO ISSUE** -- already correct

---

### 11. Drainliar (ID 1111) -- DENIED (already correct)

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `shadow` | `shadow` |
| element.level | `2` | `2` |

- **Line**: 2848
- **Code**: `element: { type: 'shadow', level: 2 }`
- **rAthena**: Shadow Lv2 (confirmed RateMyServer "Shadow 2")
- **Note**: Audit claimed "Dark 2" -- in RO, "Dark" and "Shadow" are the same element. Code uses `shadow` which is the rAthena convention. Already correct.
- **Verdict**: **DENIED** -- code is already correct. Naming convention difference only.

---

### 12. Penomena (ID 1216) -- PARTIAL (level wrong)

| Field | Current (code) | Expected (rAthena pre-re) |
|-------|----------------|---------------------------|
| element.type | `poison` | `poison` |
| element.level | `1` | `1` |

- **Line**: 7490
- **Code**: `element: { type: 'poison', level: 1 }`
- **rAthena**: Poison Lv1 (confirmed by RateMyServer, iRO Wiki, divine-pride, pservero pre-re -- ALL say Poison 1)
- **Note**: Audit claimed "Poison 3" -- this is WRONG. Every authoritative source confirms Poison 1.
- **Verdict**: **DENIED** -- code is already correct. Audit claim of Poison 3 was wrong.

---

## Special Checks

### 13. Baphomet Jr. (ID 1101) Drop Table -- CONFIRMED BUG

- **Line**: 6345
- **Code**: `{ itemName: 'Baphomet Card', rate: 0.01, stealProtected: true }`
- **rAthena**: Drops "Baphomet Jr. Card" (item ID 4129), NOT "Baphomet Card" (item ID 4147)
- **Confirmed by**: RateMyServer ("Baphomet Jr. Card"), iRO Wiki ("Bapho Jr. Card"), divine-pride ("Baphomet Jr. Card"), pservero pre-re ("Baphomet Jr. Card")
- **Impact**: Players could be getting the wrong card entirely. Baphomet Card (from Baphomet MVP, ID 1039) gives +10 HIT, ATK+10% to all enemies in 9x9 splash. Baphomet Jr. Card gives Agi+3, Critical+1. Completely different items.
- **Verdict**: **CONFIRMED BUG** -- wrong card name in drop table

---

### 14. Giant Whisper (ID 1186) monsterClass -- AMBIGUOUS, LIKELY CORRECT

| Field | Current (code) | rAthena pre-re |
|-------|----------------|----------------|
| monsterClass | `normal` | `Normal` (Class field) |

- **Line**: 4407
- **Code**: `monsterClass: 'normal'`
- **rAthena mob_db.yml**: Class: Normal (confirmed by pservero pre-re, which reads directly from rAthena DB)
- **Note**: Despite aegisName `WHISPER_BOSS`, rAthena pre-renewal classifies Giant Whisper as Class: Normal. It does NOT have boss protocol (not immune to certain status effects the way true bosses are). The "BOSS" in the aegisName is a naming convention, not a class designation. RateMyServer's "Boss" label appears to be from the aegisName, not the actual Class field.
- **Verdict**: **NO ISSUE** -- `monsterClass: 'normal'` matches rAthena pre-renewal Class: Normal

---

## Summary

| # | Monster | ID | Issue | Status |
|---|---------|-----|-------|--------|
| 1 | Golem | 1040 | element level 1 -> 3 | **CONFIRMED** |
| 2 | Orc Skeleton | 1152 | element neutral -> undead | **CONFIRMED** |
| 3 | Orc Zombie | 1153 | element neutral -> undead | **CONFIRMED** |
| 4 | Kobold | 1133 | element neutral/1 -> wind/2 | **CONFIRMED** |
| 5 | Orc Archer | 1189 | element neutral -> earth | **CONFIRMED** |
| 6 | High Orc | 1213 | element neutral/1 -> fire/2 | **CONFIRMED** |
| 7 | Orc Lady | 1273 | element neutral/1 -> earth/2 | **CONFIRMED** |
| 8 | Orc Baby | 1686 | (already correct earth/1) | **NO ISSUE** |
| 9 | Verit | 1032 | (already correct undead/1) | **DENIED** |
| 10 | Mummy | 1041 | (already correct undead/2) | **NO ISSUE** |
| 11 | Drainliar | 1111 | (already correct shadow/2) | **DENIED** |
| 12 | Penomena | 1216 | (already correct poison/1) | **DENIED** |
| 13 | Baphomet Jr. | 1101 | drop "Baphomet Card" -> "Baphomet Jr. Card" | **CONFIRMED** |
| 14 | Giant Whisper | 1186 | monsterClass normal | **NO ISSUE** |

**Confirmed bugs: 8** (6 original element issues + 1 additional Orc Lady element + 1 Baphomet Jr. card drop)
**Denied/No issue: 6** (Orc Baby, Verit, Mummy, Drainliar, Penomena, Giant Whisper)

---

## Fix Summary (for implementation)

```js
// 1. Golem (line 3052) -- level 1 -> 3
element: { type: 'neutral', level: 3 },

// 2. Orc Skeleton (line 3567) -- neutral -> undead
element: { type: 'undead', level: 1 },

// 3. Orc Zombie (line 2952) -- neutral -> undead
element: { type: 'undead', level: 1 },

// 4. Kobold (line 4605) -- neutral/1 -> wind/2
element: { type: 'wind', level: 2 },

// 5. Orc Archer (line 6231) -- neutral -> earth
element: { type: 'earth', level: 1 },

// 6. High Orc (line 6742) -- neutral/1 -> fire/2
element: { type: 'fire', level: 2 },

// 7. Orc Lady (line 4055) -- neutral/1 -> earth/2
element: { type: 'earth', level: 2 },

// 8. Baphomet Jr. (line 6345) -- wrong card name
{ itemName: 'Baphomet Jr. Card', rate: 0.01, stealProtected: true },
```
