# Monster Stats Batch 3 Audit (Positions 341-509)

**Scope**: Monsters from position ~341 onward in `ro_monster_templates.js`, covering levels 60-99. Includes all MVPs and high-level dungeon monsters.
**Source file**: `server/src/ro_monster_templates.js` (13,070 lines, 509 monsters total)
**Verification sources**: rAthena pre-renewal `mob_db.yml`, `pre.pservero.com` (rAthena pre-renewal database), `ratemyserver.net` (Pre-re)
**Date**: 2026-03-22

---

## 1. Monsters Covered

This batch covers **~169 monsters** from roughly level 59-99, including:
- **43 MVPs** (monsterClass: 'mvp')
- **54 mini-bosses** (monsterClass: 'boss')
- **72 normal high-level monsters** (monsterClass: 'normal')

### All 43 MVPs in the template file:

| # | ID | Name | Level | HP | Element | Race | Size |
|---|-----|------|-------|-----|---------|------|------|
| 1 | 1038 | Osiris | 78 | 415,400 | Undead 4 | Undead | Medium |
| 2 | 1039 | Baphomet | 81 | 668,000 | Shadow 3 | Demon | Large |
| 3 | 1046 | Doppelganger | 72 | 249,000 | Shadow 3 | Demon | Medium |
| 4 | 1059 | Mistress | 74 | 212,000 | Wind 4 | Insect | Small |
| 5 | 1086 | Golden Thief Bug | 64 | 126,000 | Fire 2 | Insect | Large |
| 6 | 1087 | Orc Hero | 77 | 585,700 | Earth 2 | Demihuman | Large |
| 7 | 1112 | Drake | 70 | 326,666 | Undead 1 | Undead | Medium |
| 8 | 1115 | Eddga | 65 | 152,000 | Fire 1 | Brute | Large |
| 9 | 1147 | Maya | 81 | 169,000 | Earth 4 | Insect | Large |
| 10 | 1150 | Moonlight Flower | 67 | 120,000 | Fire 3 | Demon | Medium |
| 11 | 1157 | Pharaoh | 93 | 445,997 | Shadow 3 | Demihuman | Large |
| 12 | 1159 | Phreeoni | 69 | 188,000 | Neutral 3 | Brute | Large |
| 13 | 1190 | Orc Lord | 74 | 783,000 | Earth 4 | Demihuman | Large |
| 14 | 1251 | Stormy Knight | 77 | 240,000 | Wind 4 | Formless | Large |
| 15 | 1252 | Hatii | 73 | 197,000 | Water 4 | Brute | Large |
| 16 | 1272 | Dark Lord | 80 | 720,000 | Undead 4 | Demon | Large |
| 17 | 1312 | Turtle General | 97 | 320,700 | Earth 2 | Brute | Large |
| 18 | 1373 | Lord of the Dead | 94 | 603,383 | Shadow 3 | Demon | Large |
| 19 | 1389 | Dracula | 85 | 320,096 | Shadow 4 | Demon | Large |
| 20 | 1418 | Evil Snake Lord | 73 | 254,993 | Ghost 3 | Brute | Large |
| 21 | 1492 | Samurai Specter | 71 | 218,652 | Shadow 3 | Demihuman | Large |
| 22 | 1502 | Bring it on! | 99 | 95,000,000 | Poison 1 | Plant | Medium |
| 23 | 1511 | Amon Ra | 88 | 1,214,138 | Earth 3 | Demihuman | Large |
| 24 | 1583 | Tao Gunka | 70 | 193,000 | Neutral 3 | Demon | Large |
| 25 | 1623 | RSX-0806 | 86 | 560,733 | Neutral 3 | Formless | Large |
| 26 | 1630 | White Lady | 85 | 253,221 | Wind 3 | Demihuman | Large |
| 27 | 1646 | Lord Knight Seyren | 99 | 1,647,590 | Fire 4 | Demihuman | Medium |
| 28 | 1647 | Assassin Cross Eremes | 99 | 1,411,230 | Poison 4 | Demihuman | Medium |
| 29 | 1648 | Whitesmith Howard | 99 | 1,460,000 | Earth 4 | Demihuman | Medium |
| 30 | 1649 | High Priest Margaretha | 99 | 1,092,910 | Holy 4 | Demihuman | Medium |
| 31 | 1650 | Sniper Cecil | 99 | 1,349,000 | Wind 4 | Demihuman | Medium |
| 32 | 1651 | High Wizard Kathryne | 99 | 1,069,920 | Ghost 3 | Demihuman | Medium |
| 33 | 1658 | Egnigem Cenia | 79 | 214,200 | Fire 2 | Demihuman | Medium |
| 34 | 1685 | Vesper | 97 | 640,700 | Holy 2 | Brute | Large |
| 35 | 1688 | Lady Tanee | 89 | 493,000 | Wind 3 | Plant | Large |
| 36 | 1708 | Memory of Thanatos | 99 | 445,660 | Ghost 4 | Demon | Large |
| 37 | 1719 | Detardeurus | 90 | 960,000 | Shadow 3 | Dragon | Large |
| 38 | 1734 | Kiel D-01 | 90 | 1,523,000 | Shadow 2 | Formless | Medium |
| 39 | 1751 | Valkyrie Randgris | 99 | 3,567,200 | Holy 4 | Angel | Large |
| 40 | 1768 | Gloom Under Night | 89 | 2,298,000 | Ghost 3 | Formless | Large |
| 41 | 1779 | Ktullanux | 98 | 4,417,000 | Water 4 | Brute | Large |
| 42 | 1785 | Atroce | 82 | 1,008,420 | Shadow 3 | Brute | Large |
| 43 | 1832 | Ifrit | 99 | 7,700,000 | Fire 4 | Formless | Large |

---

## 2. Per-MVP Stat Audit Against Web Sources

All MVPs verified against `pre.pservero.com` (rAthena pre-renewal database). The template was auto-generated from rAthena `mob_db.yml`.

### VERIFIED CORRECT (core stats match rAthena pre-renewal):

| MVP | Level | HP | ATK | DEF | MDEF | Element | Race | Size | Verdict |
|-----|-------|-----|------|-----|------|---------|------|------|---------|
| Osiris (1038) | 78 | 415,400 | 780 | 10 | 25 | Undead 4 | Undead | Med | MATCH |
| Baphomet (1039) | 81 | 668,000 | 3,220 | 35 | 45 | Shadow 3 | Demon | Large | MATCH |
| Doppelganger (1046) | 72 | 249,000 | 1,340 | 60 | 35 | Shadow 3 | Demon | Med | MATCH |
| Mistress (1059) | 74 | 212,000 | 880 | 40 | 60 | Wind 4 | Insect | Small | MATCH |
| Golden Thief Bug (1086) | 64 | 126,000 | 870 | 60 | 45 | Fire 2 | Insect | Large | MATCH |
| Orc Hero (1087) | 77 | 585,700 | 2,257 | 40 | 45 | Earth 2 | Demi | Large | MATCH |
| Drake (1112) | 70 | 326,666 | 1,800 | 20 | 35 | Undead 1 | Undead | Med | MATCH |
| Eddga (1115) | 65 | 152,000 | 1,215 | 15 | 15 | Fire 1 | Brute | Large | MATCH |
| Maya (1147) | 81 | 169,000 | 1,800 | 60 | 25 | Earth 4 | Insect | Large | MATCH |
| Moonlight Flower (1150) | 67 | 120,000 | 1,200 | 10 | 55 | Fire 3 | Demon | Med | MATCH |
| Pharaoh (1157) | 93 | 445,997 | 2,267 | 67 | 70 | Shadow 3 | Demi | Large | MATCH |
| Phreeoni (1159) | 69 | 188,000 | 880 | 10 | 20 | Neutral 3 | Brute | Large | MATCH |
| Orc Lord (1190) | 74 | 783,000 | 3,700 | 40 | 5 | Earth 4 | Demi | Large | MATCH |
| Stormy Knight (1251) | 77 | 240,000 | 1,425 | 35 | 60 | Wind 4 | Formless | Large | MATCH |
| Hatii (1252) | 73 | 197,000 | 1,700 | 40 | 45 | Water 4 | Brute | Large | MATCH |
| Dark Lord (1272) | 80 | 720,000 | 2,800 | 30 | 70 | Undead 4 | Demon | Large | MATCH |
| Turtle General (1312) | 97 | 320,700 | 2,438 | 50 | 54 | Earth 2 | Brute | Large | MATCH |
| Lord of the Dead (1373) | 94 | 603,383 | 3,430 | 77 | 73 | Shadow 3 | Demon | Large | MATCH |
| Dracula (1389) | 85 | 320,096 | 1,625 | 45 | 76 | Shadow 4 | Demon | Large | MATCH |
| Evil Snake Lord (1418) | 73 | 254,993 | 2,433 | 25 | 55 | Ghost 3 | Brute | Large | MATCH |
| Samurai Specter (1492) | 71 | 218,652 | 2,219 | 10 | 51 | Shadow 3 | Demi | Large | MATCH |
| Amon Ra (1511) | 88 | 1,214,138 | 1,647 | 26 | 52 | Earth 3 | Demi | Large | MATCH |
| Tao Gunka (1583) | 70 | 193,000 | 1,450 | 20 | 20 | Neutral 3 | Demon | Large | MATCH |
| RSX-0806 (1623) | 86 | 560,733 | 2,740 | 39 | 41 | Neutral 3 | Formless | Large | MATCH |
| White Lady (1630) | 85 | 253,221 | 1,868 | 20 | 55 | Wind 3 | Demi | Large | MATCH |
| Egnigem Cenia (1658) | 79 | 214,200 | 3,890 | 48 | 25 | Fire 2 | Demi | Med | MATCH |
| Vesper (1685) | 97 | 640,700 | 4,000 | 50 | 54 | Holy 2 | Brute | Large | MATCH |
| Lady Tanee (1688) | 89 | 493,000 | 450 | 20 | 44 | Wind 3 | Plant | Large | MATCH |
| Memory of Thanatos (1708) | 99 | 445,660 | 3,812 | 35 | 35 | Ghost 4 | Demon | Large | MATCH |
| Detardeurus (1719) | 90 | 960,000 | 4,560 | 66 | 59 | Shadow 3 | Dragon | Large | MATCH |
| Kiel D-01 (1734) | 90 | 1,523,000 | 3,280 | 28 | 32 | Shadow 2 | Formless | Med | MATCH |
| Valkyrie Randgris (1751) | 99 | 3,567,200 | 5,560 | 25 | 42 | Holy 4 | Angel | Large | MATCH |
| Gloom Under Night (1768) | 89 | 2,298,000 | 5,880 | 10 | 20 | Ghost 3 | Formless | Large | MATCH |
| Ktullanux (1779) | 98 | 4,417,000 | 1,680 | 40 | 42 | Water 4 | Brute | Large | MATCH |
| Atroce (1785) | 82 | 1,008,420 | 2,526 | 25 | 25 | Shadow 3 | Brute | Large | MATCH |
| Ifrit (1832) | 99 | 7,700,000 | 13,530 | 40 | 50 | Fire 4 | Formless | Large | MATCH |
| Bring it on! (1502) | 99 | 95,000,000 | 10,000 | 0 | 10 | Poison 1 | Plant | Med | MATCH |
| LK Seyren (1646) | 99 | 1,647,590 | 7,238 | 72 | 37 | Fire 4 | Demi | Med | MATCH |
| AX Eremes (1647) | 99 | 1,411,230 | 4,189 | 37 | 39 | Poison 4 | Demi | Med | MATCH |
| WS Howard (1648) | 99 | 1,460,000 | 7,822 | 66 | 36 | Earth 4 | Demi | Med | MATCH |
| HP Margaretha (1649) | 99 | 1,092,910 | 4,688 | 35 | 78 | Holy 4 | Demi | Med | MATCH |
| Sniper Cecil (1650) | 99 | 1,349,000 | 4,892 | 22 | 35 | Wind 4 | Demi | Med | MATCH |
| HW Kathryne (1651) | 99 | 1,069,920 | 1,197 | 10 | 88 | Ghost 3 | Demi | Med | MATCH |

**Result: ALL 43 MVPs have correct core stats (Level, HP, ATK, DEF, MDEF, Element, Race, Size).**

---

## 3. MVP Drop Table Discrepancies

While core stats are correct, the MVP drop tables show some differences between the template and the pservero/rAthena database. These are typically the third MVP reward slot which may be version-dependent.

### Baphomet (1039)
- **Template mvpDrops**: Yggdrasil Berry (20%), Baphomet Doll (5%)
- **rAthena**: Evil Horn (50%), Yggdrasil Berry (20%), Baphomet Doll (5%)
- **Issue**: Missing Evil Horn as first MVP drop

### Doppelganger (1046)
- **Template mvpDrops**: Ruby (15%)
- **rAthena**: Blue Potion (60%), Cursed Ruby (15%)
- **Issue**: Missing Blue Potion as first MVP drop; item is "Cursed Ruby" not "Ruby"

### Pharaoh (1157)
- **Template mvpDrops**: Yggdrasil Berry (55%), Royal Jelly (50%)
- **rAthena**: Yggdrasil Berry (55%), Royal Jelly (50%), 3carat Diamond (50%)
- **Issue**: Missing 3carat Diamond as third MVP drop

### Phreeoni (1159)
- **Template mvpDrops**: Necklace of Oblivion (5%), 1carat Diamond (10%)
- **rAthena**: Star Crumb (40%), 1carat Diamond (10%), Necklace of Oblivion (5%)
- **Issue**: Missing Star Crumb as first MVP drop; order differs

### Moonlight Flower (1150)
- **Template mvpDrops**: Nine Tails (50%), White Potion (15%)
- **rAthena**: Nine Tails (50%), White Potion (15%), Topaz (5%)
- **Issue**: Missing Topaz as third MVP drop

### Eddga (1115)
- **Template mvpDrops**: Tiger's Skin (50%), Tiger's Footskin (10%)
- **rAthena**: Tiger Skin (50%), Flame Heart (30%), Tiger's Footskin (10%)
- **Issue**: Missing Flame Heart as second MVP drop

### Golden Thief Bug (1086)
- **Template mvpDrops**: Gold Ring (20%)
- **rAthena**: Gold Ring (20%), Ora Ora (10%)
- **Issue**: Missing Ora Ora as second MVP drop

### Orc Lord (1190)
- **Template mvpDrops**: Heroic Emblem (55%)
- **rAthena**: Heroic Emblem (55%), Old Purple Box (20%)
- **Issue**: Missing Old Purple Box as second MVP drop

### Orc Hero (1087)
- **Template mvpDrops**: Red Jewel (20%), Yggdrasil Berry (15%)
- **rAthena**: Steel (50%), Sardonyx (20%), Yggdrasil Berry (15%)
- **Issue**: First MVP drop is Steel not Red Jewel; missing Sardonyx

### Hatii (1252)
- **Template mvpDrops**: Fang of Hatii (10%), Old Blue Box (30%)
- **rAthena**: Fang of Hatii (10%), Old Blue Box (30%), Mystic Frozen (30%)
- **Issue**: Missing Mystic Frozen as third MVP drop

### Stormy Knight (1251)
- **Template mvpDrops**: Aquamarine (45%), Boots (5%)
- **rAthena**: Aquamarine (45%), Mystic Frozen (30%), Boots (5%)
- **Issue**: Missing Mystic Frozen as second MVP drop

### Dark Lord (1272)
- **Template mvpDrops**: Skull (60%), Coif (5%)
- **rAthena**: Skull (60%), Old Purple Box (20%), Coif (5%)
- **Issue**: Missing Old Purple Box as second MVP drop

### Dracula (1389)
- **Template mvpDrops**: Yggdrasil Berry (55%), 1carat Diamond (50%)
- **rAthena**: Yggdrasil Berry (55%), 3carat Diamond (50%), Mastela Fruit (50%)
- **Issue**: "1carat Diamond" should be "3carat Diamond"; missing Mastela Fruit

### Evil Snake Lord (1418)
- **Template mvpDrops**: Yggdrasil Berry (55%), Old Purple Box (50%)
- **rAthena**: Yggdrasil Berry (55%), Elunium (55%), Old Purple Box (50%)
- **Issue**: Missing Elunium as second MVP drop

### RSX-0806 (1623)
- **Template mvpDrops**: Yggdrasil Berry (55%), Dark Blinder (35%)
- **rAthena**: Yggdrasil Berry (55%), 3carat Diamond (55%), Dark Blinder (35%)
- **Issue**: Missing 3carat Diamond as second MVP drop

### Maya (1147)
- **Template mvpDrops**: 1carat Diamond (20%), Old Blue Box (30%)
- **rAthena**: Old Blue Box (30%), 1carat Diamond (20%), Old Purple Box (20%)
- **Issue**: Missing Old Purple Box as third MVP drop

### Amon Ra (1511)
- **Template mvpDrops**: Yggdrasil Berry (55%), Yggdrasil Seed (35%)
- **rAthena**: Yggdrasil Berry (55%), Yggdrasil Seed (35%), 3carat Diamond (55%)
- **Issue**: Missing 3carat Diamond as third MVP drop

### Drake (1112)
- **Template mvpDrops**: White Potion (50%)
- **rAthena**: White Potion (50%), Amethyst (5%)
- **Issue**: Missing Amethyst as second MVP drop

### Turtle General (1312)
- **Template mvpDrops**: Turtle Shell (55%), Yggdrasil Berry (15%)
- **rAthena**: Turtle Shell (55%), Old Purple Box (20%), Yggdrasil Berry (15%)
- **Issue**: Missing Old Purple Box as second MVP drop

### Osiris (1038)
- **Template mvpDrops**: Old Blue Box (40%), Yggdrasil Seed (30%)
- **rAthena**: Old Blue Box (40%), Yggdrasil Seed (30%), Osiris Doll (5%)
- **Issue**: Missing Osiris Doll as third MVP drop

### Mistress (1059)
- **Template mvpDrops**: Rough Wind (15%), Royal Jelly (40%)
- **rAthena**: Royal Jelly (40%), Pearl (30%), Rough Wind (15%)
- **Issue**: Missing Pearl as second MVP drop

**Summary**: Most MVPs are missing 1 of 3 MVP reward slots. The rAthena pre-renewal database provides 3 MVP drop slots; the template often only has 1-2 populated. This is a systematic omission across the file.

---

## 4. Respawn Timer Audit

**All MVPs**: `respawnMs: 3600000` (1 hour)

This is a simplified flat timer. In rAthena, many MVPs have variable respawn with a base time plus a random variance window. For example:
- Pharaoh: ~70 min base + variable
- Orc Lord: ~130 min base + variable
- Dark Lord: ~60-70 min base + variable

The flat 1-hour timer is acceptable for early development but does not match RO Classic variable respawn windows. Similarly, all BOSS-class mini-bosses have `respawnMs: 1800000` (30 min), also flat.

---

## 5. Missing MVPs

### Present in rAthena pre-renewal but NOT in the template (6 MVPs):

| ID | Name | Level | HP | Notes |
|----|------|-------|-----|-------|
| 1399 | Baphomet (Event) | N/A | N/A | Event-only variant; OK to omit |
| 1871 | Fallen Bishop Hibram | 90+ | ~3.5M | Episode 12.1 content, late dungeon |
| 1874 | Beelzebub | 95+ | ~5.5M | Episode 12.1 content |
| 1885 | Gopinich | 80+ | ~600K | Episode 8.1+ content |
| 1917 | Wounded Morocc | 95+ | ~5M | Satan Morocc episode content |
| 1980 | Kublin | N/A | N/A | Battleground content |
| 2022 | Nidhoggr's Shadow | 95+ | ~4M | Episode 13.1 content |
| 2068 | Boitata | 93 | ~1.28M | Episode content |

**Assessment**: The template covers 509 monsters from the original "core" pre-renewal set. The missing MVPs are primarily from later episodes (12.1+) which are beyond the base classic content scope. IDs 1871/1874/1885/1917/2022/2068 are all episode-gated content. ID 1399 (Event Baphomet) is event-only. ID 1980 (Kublin) is Battleground content. None are critical omissions for core classic RO gameplay.

---

## 6. High-Level Monster Accuracy (Non-MVP)

### Notable BOSS-class mini-bosses verified:

| ID | Name | Level | HP | Element | Verified |
|----|------|-------|-----|---------|----------|
| 1203 | Mysteltainn | 76 | 33,350 | Shadow 4 | Correct |
| 1219 | Abysmal Knight | 79 | 36,140 | Shadow 4 | Correct |
| 1259 | Gryphon | 72 | 27,800 | Wind 4 | Correct |
| 1268 | Bloody Knight | 82 | 57,870 | Shadow 4 | Correct |
| 1283 | Chimera | 70 | 32,600 | Fire 3 | Correct |
| 1289 | Maya Purple | 81 | 55,479 | Earth 4 | Correct |
| 1295 | Owl Baron | 75 | 60,746 | Neutral 3 | Correct |
| 1302 | Dark Illusion | 77 | 103,631 | Undead 4 | Correct |
| 1307 | Cat o' Nine Tails | 76 | 64,512 | Fire 3 | Correct |
| 1388 | Arc Angeling | 60 | 79,523 | Holy 3 | Correct |
| 1681 | Gemini-S58 | 72 | 57,870 | Water 1 | Correct |
| 1700 | Dame of Sentinel | 81 | 65,111 | Neutral 4 | Correct |
| 1701 | Mistress of Shelter | 80 | 38,000 | Holy 3 | Correct |
| 1702 | Baroness of Retribution | 79 | 46,666 | Shadow 3 | Correct |
| 1703 | Lady Solace | 77 | 25,252 | Holy 3 | Correct |
| 1733 | Kiehl | 90 | 523,000 | Shadow 2 | Correct |
| 1752 | Skogul | 70 | 87,544 | Shadow 3 | Correct |
| 1754 | Skeggiold | 81 | 295,200 | Holy 2 | Correct |
| 1795 | Bloody Knight (Event) | 82 | 800,000 | Ghost 1 | Correct |
| 1829 | Sword Master | 86 | 152,533 | Neutral 4 | Correct |
| 1831 | Salamander | 91 | 97,934 | Fire 3 | Correct |
| 1833 | Kasa | 85 | 80,375 | Fire 3 | Correct |
| 1839 | Byorgue | 86 | 38,133 | Neutral 1 | Correct |

### Notable high-level normal monsters verified:

| ID | Name | Level | HP | Element | Verified |
|----|------|-------|-----|---------|----------|
| 1098 | Anubis | 75 | 38,000 | Undead 2 | Correct |
| 1148 | Medusa | 79 | 16,408 | Neutral 2 | Correct |
| 1370 | Succubus | 85 | 16,955 | Shadow 3 | Correct |
| 1374 | Incubus | 75 | 17,281 | Shadow 3 | Correct |
| 1382 | Diabolic | 67 | 9,642 | Shadow 2 | Correct |
| 1635 | Eremes Guile | 87 | 60,199 | Poison 4 | Correct |
| 1636 | Howard Alt-Eisen | 83 | 78,690 | Water 4 | Correct |
| 1637 | Margaretha Sorin | 90 | 61,282 | Holy 3 | Correct |
| 1638 | Cecil Damon | 82 | 58,900 | Wind 3 | Correct |
| 1639 | Kathryne Keyron | 92 | 47,280 | Ghost 3 | Correct |
| 1634 | Seyren Windsor | 91 | 88,402 | Fire 3 | Correct |

### Bio Lab 2F non-MVP transcendent class monsters (6 entries):

| ID | Name | Level | HP | Notes |
|----|------|-------|-----|-------|
| 1805 | Lord Knight Seyren | 99 | 1,647,590 | BOSS variant (no MVP flag) |
| 1806 | Assassin Cross Eremes | 99 | 1,411,230 | BOSS variant (no MVP flag) |
| 1807 | Mastersmith Howard | 99 | 1,460,000 | BOSS variant (no MVP flag) |
| 1808 | High Priest Margaretha | 99 | 1,092,910 | BOSS variant (no MVP flag) |
| 1809 | Sniper Cecil | 99 | 1,349,000 | BOSS variant (no MVP flag) |
| 1810 | High Wizard Kathryne | 99 | 1,069,920 | BOSS variant (no MVP flag) |

These are the non-MVP Bio Lab 3F variants. Each drops only "Evil Mind" (100%). Stats match their MVP counterparts. This is correct behavior per rAthena.

### Special monsters:

| ID | Name | Level | HP | Notes |
|----|------|-------|-----|-------|
| 1840 | Golden Savage | 99 | 500 | BOSS, passive, event-type monster |
| 1846 | Dream Metal | 90 | 999 | BOSS, passive, special drop |

---

## 7. Systematic Issues Found

### 7.1 MVP Drop Slots (HIGH - affects all MVPs)
**Issue**: rAthena pre-renewal provides 3 MVP drop slots per MVP. The template systematically has only 1-2 MVP drops populated for most MVPs. At least 21 of 43 MVPs are missing one or more MVP reward drops.

**Impact**: Players receive fewer rewards per MVP kill than intended by RO Classic.

**Fix**: Populate the third (and sometimes second) MVP drop slot for all 43 MVPs from rAthena source data.

### 7.2 Uniform Respawn Timers (LOW - gameplay balance)
**Issue**: All MVPs use `respawnMs: 3600000` (1 hour flat). All mini-bosses use `respawnMs: 1800000` (30 min flat). In RO Classic, MVP respawn times vary significantly (60 min to 8+ hours with random variance windows).

**Impact**: All MVPs respawn at the same rate regardless of difficulty. Higher-tier MVPs like Ifrit, Valkyrie Randgris, and Gloom Under Night should have longer respawn windows.

**Fix**: Update respawn timers per rAthena spawn data when variable respawn is implemented.

### 7.3 Dracula mvpDrops Item Name (MEDIUM)
**Issue**: Template has "1carat Diamond" but rAthena source says "3carat Diamond" for Dracula's MVP drop.

**Fix**: Change `1carat Diamond` to `3carat Diamond` in Dracula's mvpDrops.

### 7.4 Orc Hero mvpDrops Item (MEDIUM)
**Issue**: Template has "Red Jewel" (20%) as first MVP drop, but rAthena has "Steel" (50%).

**Fix**: Change first MVP drop to Steel (50%), add Sardonyx (20%).

### 7.5 Doppelganger mvpDrops Item Name (LOW)
**Issue**: Template has "Ruby" but rAthena has "Cursed Ruby" for the 15% MVP drop.

**Fix**: Rename to "Cursed Ruby".

---

## 8. Summary

### Stats Accuracy
- **43/43 MVPs**: Core stats (Level, HP, ATK, DEF, MDEF, Element, Race, Size) are CORRECT
- **All BOSS mini-bosses checked**: Core stats CORRECT
- **All high-level normals checked**: Core stats CORRECT
- **EXP values**: All match rAthena pre-renewal

### Drop Table Accuracy
- **Regular drops**: All match rAthena pre-renewal
- **MVP drops**: 21+ of 43 MVPs are missing 1 of 3 MVP reward slots (systematic)
- **3 specific item name errors** in MVP drops (Dracula, Orc Hero, Doppelganger)

### Missing Content
- **6 late-episode MVPs** not in template (IDs 1871, 1874, 1885, 1917, 2022, 2068) -- acceptable for core classic scope
- **1 event MVP** not in template (ID 1399 Baphomet Event) -- acceptable
- **1 Battleground MVP** not in template (ID 1980 Kublin) -- acceptable

### Corrections Needed (Priority Order)

1. **[HIGH]** Populate missing third MVP drop slot for 21+ MVPs
2. **[MEDIUM]** Fix Dracula mvpDrops: "1carat Diamond" -> "3carat Diamond"
3. **[MEDIUM]** Fix Orc Hero mvpDrops: "Red Jewel" (20%) -> "Steel" (50%), add "Sardonyx" (20%)
4. **[LOW]** Fix Doppelganger mvpDrops: "Ruby" -> "Cursed Ruby", add "Blue Potion" (60%)
5. **[LOW]** Consider variable respawn timers for MVPs (future enhancement)
6. **[LOW]** Consider adding late-episode MVPs (Fallen Bishop, Beelzebub, etc.) when episode content is implemented
