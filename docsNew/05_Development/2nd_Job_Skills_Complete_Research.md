# Complete 2nd Job Skills Research — RO Classic Pre-Renewal

## Research Sources (6+ per class)
1. iRO Wiki Classic (irowiki.org/classic/) — Primary pre-renewal reference
2. RateMyServer Skill Database (ratemyserver.net) — Detailed numerical data
3. rAthena Pre-Renewal DB (github.com/rathena/rathena) — Canonical server emulator
4. Divine Pride (divine-pride.net) — Cross-reference
5. Ragnarok Fandom Wiki (ragnarok.fandom.com) — Additional verification
6. RateMyServer Skill Simulator — Prerequisite tree verification
7. GameFAQs guides, community blogs — Bard/Dancer performance details

---

## GRAND TOTALS

| Class | Total Skills | In Codebase | Missing | Quest Skills |
|-------|-------------|-------------|---------|--------------|
| Knight | 11 | 10 | 1 | 1 (Charge Attack) |
| Crusader | 14 | 7 | 7 | 1 (Shrink) |
| Wizard | 14 | 10 | 4 | 1 (Sight Blaster) |
| Sage | 23 | 5 | 18 | 2 (Create Converter, Elemental Change) |
| Hunter | 18 | 8 | 10 | 1 (Phantasmic Arrow) |
| Bard | 19 | 3 | 16 | 1 (Pang Voice) |
| Dancer | 19 | 3 | 16 | 1 (Charming Wink) |
| Priest | 19 | 8 | 11 | 1 (Redemptio) |
| Monk | 17 | 6 | 11 | 2 (Ki Translation, Ki Explosion) |
| Assassin | 12 | 7 | 5 | 2 (Sonic Acceleration, Throw Venom Knife) |
| Rogue | 19 | 5 | 14 | 1 (Close Confine) |
| Blacksmith | 24 | 6 | 18 | 2 (Greed, Dubious Salesmanship) |
| Alchemist | 17 | 5 | 12 | 0 (Bioethics is quest but counted as regular) |
| **TOTAL** | **226** | **83** | **143** | **16** |

Note: Shared skills (Spear Mastery for Knight+Crusader, Sense for Wizard+Sage, etc.) counted once per class that can learn them. Soul Link skills and Transcendent class skills are EXCLUDED.

---

## KNIGHT (IDs 700-719) — 11 skills (10 in codebase + 1 missing)

### Currently in codebase (10):
| Our ID | Name | Max Lv | Type | Status |
|--------|------|--------|------|--------|
| 700 | Spear Mastery | 10 | Passive | NEEDS REVIEW — mounted bonus missing (+5/lv mounted vs +4/lv unmounted) |
| 701 | Pierce | 10 | Active (single) | NEEDS REVIEW — hit count by size missing (Small=1, Med=2, Large=3) |
| 702 | Spear Stab | 10 | Active (single) | NEEDS REVIEW — knockback 6 cells, line splash missing |
| 703 | Brandish Spear | 10 | Active (AoE) | NEEDS REVIEW — requires mounted, directional cone AoE, knockback 2 |
| 704 | Spear Boomerang | 5 | Active (single) | NEEDS REVIEW — range scales (3/5/7/9/11 cells), damage 150/200/250/300/350% |
| 705 | Two-Hand Quicken | 10 | Active (self) | NEEDS REVIEW — ASPD +30%, duration 30-300s |
| 706 | Auto Counter | 5 | Active (self) | NEEDS REVIEW — guaranteed crit counter, duration 0.4-2.0s stance |
| 707 | Bowling Bash | 10 | Active (single) | NEEDS REVIEW — hits TWICE, prereqs: Bash 10 + MB 3 + 2HSM 5 + THQ 10 + AC 5 |
| 708 | Riding | 1 | Passive | OK — enables Peco mount |
| 709 | Cavalry Mastery | 5 | Passive | OK — reduces mount ASPD penalty |

### Missing (1):
| Suggested ID | Name | Max Lv | Type | Target | Element | Description |
|-------------|------|--------|------|--------|---------|-------------|
| 710 | Charge Attack | 1 | Active | Single | Weapon | Quest skill. Rush distant target (14 cell range). Damage scales with distance: 100-500% ATK. Cast time: 0.5-1.5s based on distance. SP: 40. Knockback 1 cell. |

### Key Data Corrections for Existing Knight Skills:
- **Pierce (701)**: SP should be 7 flat (all levels). Hit count = 1/2/3 for Small/Medium/Large. Damage: 110-200% per hit.
- **Spear Stab (702)**: Hits all enemies in LINE between caster and target. Knockback 6 cells. Damage: 120-300%.
- **Brandish Spear (703)**: REQUIRES mounted on Peco. Cast time 700ms uninterruptible. AoE expands at Lv4/7/10. Damage: 120-300%. Knockback 2.
- **Spear Boomerang (704)**: Ranged physical. Range: 3/5/7/9/11 cells. Damage: 150/200/250/300/350%. SP: 10 flat.
- **Two-Hand Quicken (705)**: Duration: 30/60/90/120/150/180/210/240/270/300s. SP: 14/18/22/26/30/34/38/42/46/50.
- **Auto Counter (706)**: Counter = guaranteed crit ignoring DEF. Duration: 0.4/0.8/1.2/1.6/2.0s. SP: 3 flat.
- **Bowling Bash (707)**: Hits TWICE (2 damage calculations). Knockback 1 cell. SP: 13-22. Prereqs need updating.

---

## CRUSADER (IDs 1300-1319) — 14 skills (7 in codebase + 7 missing)

### Currently in codebase (7):
| Our ID | Name | Max Lv | Type | Status |
|--------|------|--------|------|--------|
| 1300 | Faith | 10 | Passive | NEEDS REVIEW — +200 HP/lv + Holy resist 5%/lv |
| 1301 | Guard (Auto Guard) | 10 | Active (self) | NEEDS REVIEW — block chance 5-30%, requires shield |
| 1302 | Holy Cross | 10 | Active (single) | NEEDS REVIEW — hits TWICE, blind chance 3%/lv |
| 1303 | Grand Cross | 10 | Active (AoE) | NEEDS REVIEW — hybrid ATK+MATK, 20% HP self-damage, uninterruptible 3s cast |
| 1304 | Shield Charge | 5 | Active (single) | NEEDS REVIEW — stun chance 20-40%, knockback 5-9 cells |
| 1305 | Shield Boomerang | 5 | Active (single) | NEEDS REVIEW — always Neutral, range 3-11 cells, damage scales with shield weight |
| 1306 | Devotion | 5 | Active (single) | NEEDS REVIEW — prereqs: Reflect Shield 5 + Grand Cross 4 |

### Missing (7):
| Suggested ID | Name | Max Lv | Type | Target | Element | SP | Cast | Description |
|-------------|------|--------|------|--------|---------|-----|------|-------------|
| 1307 | Reflect Shield | 10 | Active | Self | N/A | 35-80 | 0 | Reflect 13-40% of melee physical damage. Requires shield. Duration 300s. |
| 1308 | Providence | 5 | Active | Single (ally) | N/A | 30 | 1000ms | +5-25% Demon race + Holy element resistance. 180s duration. Cannot self-cast. |
| 1309 | Defender | 5 | Active (toggle) | Self | N/A | 30 | 0 | Reduce ranged damage 20-80%. -33% move speed. ASPD penalty decreases per level. Requires shield. 180s. |
| 1310 | Spear Quicken | 10 | Active | Self | N/A | 24-60 | 0 | +30% ASPD + CRIT +3/lv + FLEE +2/lv. Requires 2H Spear. Duration 30-300s. |
| 1311 | Heal (Crusader) | 10 | Active | Single | Holy | 13-40 | 0 | Same as Acolyte Heal. Prereqs: Demon Bane 5 + Faith 10. |
| 1312 | Cure (Crusader) | 1 | Active | Single | N/A | 15 | 0 | Same as Acolyte Cure. Prereqs: Faith 5. |
| 1313 | Shrink | 1 | Toggle | Self | N/A | 15 | 0 | Quest skill. When Guard blocks, chance to knockback attacker 1 cell (5-50% based on Guard level). |

---

## WIZARD (IDs 800-819) — 14 skills (10 in codebase + 4 missing)

### Currently in codebase (10) — KEY CORRECTIONS:
| Our ID | Name | Key Corrections Needed |
|--------|------|----------------------|
| 800 | Jupitel Thunder | Cast time should be 2500+500*Lv (2500-7000ms). Hits: 3-12. Knockback: 2-7 cells. SP: 20+3*Lv. |
| 801 | Lord of Vermilion | Cast time MUCH longer: 10500-15000ms (not 5000-10000). SP: 60-96. Blind chance 4-40%. 4 damage ticks. |
| 802 | Meteor Storm | Cast time: 15000ms FLAT (not 5000+1000*Lv). Prereqs WRONG: should be Thunderstorm 1 + Sightrasher 2 (not Fire Ball + Fire Wall). Stun chance 3-30%. |
| 803 | Storm Gust | SP: 78 flat (not 60+6*Lv). Cast time: 6000-15000ms. 10 hit ticks. Freeze on 3rd hit. |
| 804 | Earth Spike | SP: 12-20 (10+2*Lv). Cast time: 700-3500ms (700*Lv). Hits: 1-5 (one per level). |
| 805 | Heaven's Drive | SP: 26-34 (24+2*Lv). Cast time: 1000-5000ms (1000*Lv). Hits: 1-5. AoE 5x5. |
| 806 | Quagmire | Cast time should be 0 (not 1000). AGI/DEX reduction: 10-50 (not 5-25). |
| 807 | Water Ball | Requires water terrain. Max balls: 1/9/25/25/25. MATK: 130-250% per ball. |
| 808 | Ice Wall | Cast time should be 0 (not 2000). HP: 400-2200 (200+200*Lv). Duration = HP/50 seconds. |
| 809 | Sight Rasher | SP: 35-53 (33+2*Lv). Cast time: 500ms. Knockback. Requires Sight active. |

### Missing (4):
| Suggested ID | Name | Max Lv | Type | Target | Element | Description |
|-------------|------|--------|------|--------|---------|-------------|
| 810 | Fire Pillar | 10 | Active | Ground | Fire | Trap-like: placed on ground, triggers on enemy. Hits: 3-12 (Lv+2). Ignores MDEF. Cast: 300-3000ms. SP: 75. AoE: 3x3 (Lv1-5), 5x5 (Lv6-10). Blue Gem at Lv6+. Prereqs: Fire Wall 1. |
| 811 | Frost Nova | 10 | Active | Self AoE | Water | 5x5 AoE centered on caster. MATK: 73-133%. Freeze chance: 38-83%. Cast: 4000-6000ms. SP: 27-45 (DECREASES with level). Prereqs: Ice Wall 1, Frost Diver 1. |
| 812 | Sense | 1 | Active | Single | Neutral | Reveals monster info (HP, element, size, race, DEF, MDEF). SP: 10. No prereqs. Shared with Sage. |
| 813 | Sight Blaster | 1 | Active | Self | Fire | Quest skill. Protective fireball, 2 min duration. When enemy enters 3x3, fires 100% MATK + 3 cell knockback. Single use. SP: 40. Cast: 700ms. |

---

## SAGE (IDs 1400-1439) — 23 skills (5 in codebase + 18 missing)

### Currently in codebase (5) — CORRECTIONS NEEDED:
| Our ID | Name | Corrections |
|--------|------|-------------|
| 1400 | Study | Description WRONG: should be "Book weapon mastery +3 ATK/lv + 0.5% ASPD/lv" (NOT "Increases Max SP and INT") |
| 1401 | Cast Cancel | Missing prereq: Study Lv 2. SP retained: 10/30/50/70/90% (not 15/30/45/60/75%) |
| 1402 | Hindsight | Prereq WRONG: should be Free Cast Lv 4 (not Cast Cancel 1). SP: 35 flat (not 20+5*Lv). |
| 1403 | Dispell | Prereq WRONG: should be Spell Breaker Lv 3 (not Cast Cancel 3). Cast time: 2000ms. Consumes Yellow Gem. |
| 1404 | Magic Rod | Missing prereq: Study Lv 4. Duration: 400-1200ms (200+200*Lv). Absorbs 20-100% SP. |

### Missing (18):
| Suggested ID | Name | Max Lv | Type | Target | Element | Key Details |
|-------------|------|--------|------|--------|---------|-------------|
| 1405 | Free Cast | 10 | Passive | None | Neutral | Move/attack while casting at 30-75% speed. Prereq: Cast Cancel 1. |
| 1406 | Spell Breaker | 5 | Active | Single | Neutral | Interrupt enemy cast + absorb SP. Lv2+: 2% MaxHP damage + 1% HP heal. SP: 10. Cast: 700ms. Prereq: Magic Rod 1. |
| 1407 | Dragonology | 5 | Passive | None | Neutral | +4-20% Dragon resist, +4-20% ATK vs Dragon, +2-10% MATK vs Dragon, +1-3 INT. Prereq: Study 9. |
| 1408 | Endow Blaze | 5 | Active | Single | Fire | Endow weapon with Fire. Success: 70-100%. Duration: 20-30 min. Cast: 3000ms. SP: 40. Catalyst: Red Blood. Prereq: Fire Bolt 1, Study 5. |
| 1409 | Endow Tsunami | 5 | Active | Single | Water | Same as Blaze but Water. Catalyst: Crystal Blue. Prereq: Cold Bolt 1, Study 5. |
| 1410 | Endow Tornado | 5 | Active | Single | Wind | Same as Blaze but Wind. Catalyst: Wind of Verdure. Prereq: Lightning Bolt 1, Study 5. |
| 1411 | Endow Quake | 5 | Active | Single | Earth | Same as Blaze but Earth. Catalyst: Green Live. Prereq: Stone Curse 1, Study 5. |
| 1412 | Volcano | 5 | Active | Ground | Fire | 7x7 zone: +10-20% fire damage, +10-50 ATK. Duration: 60-300s. Cast: 5000ms. SP: 40-48. Catalyst: Yellow Gem. Prereq: Endow Blaze 2. |
| 1413 | Deluge | 5 | Active | Ground | Water | 7x7 zone: +10-20% water damage, +5-15% MaxHP for water chars. Creates water terrain. Cast: 5000ms. SP: 40-48. Catalyst: Yellow Gem. Prereq: Endow Tsunami 2. |
| 1414 | Violent Gale | 5 | Active | Ground | Wind | 7x7 zone: +10-20% wind damage, +3-15 FLEE for wind chars. Cast: 5000ms. SP: 40-48. Catalyst: Yellow Gem. Prereq: Endow Tornado 2. |
| 1415 | Land Protector | 5 | Active | Ground | Neutral | 7x7 to 11x11 zone that negates ALL ground-targeting magic. Removes existing ground effects. Duration: 120-300s. SP: 50-66. Catalyst: Blue + Yellow Gem. Prereq: Volcano 3, Deluge 3, V.Gale 3. |
| 1416 | Abracadabra | 10 | Active | Self | Neutral | Random skill cast. Extremely unpredictable. SP: 50. Catalyst: 2 Yellow Gems. Prereq: Hindsight 5, Land Protector 1, Dispell 1. |
| 1417 | Earth Spike (Sage) | 5 | Active | Single | Earth | Same as Wizard Earth Spike. Prereq: Stone Curse 1, Endow Quake 1. |
| 1418 | Heaven's Drive (Sage) | 5 | Active | Ground | Earth | Same as Wizard Heaven's Drive. Prereq: Earth Spike (Sage) 1. |
| 1419 | Sense (Sage) | 1 | Active | Single | Neutral | Same as Wizard Sense. No prereqs. |
| 1420 | Create Elemental Converter | 1 | Active | Self | Neutral | Quest skill. Craft element converters from materials. SP: 30. |
| 1421 | Elemental Change | 1 | Active | Single | Varies | Quest skill. Change monster's element permanently. SP: 30. Cast: 2000ms. Catalyst: Elemental Converter. |

---

## HUNTER (IDs 900-929) — 18 skills (8 in codebase + 10 missing)

### Currently in codebase (8):
| Our ID | Name | Status |
|--------|------|--------|
| 900 | Blitz Beat | NEEDS REVIEW — MISC damage, auto-blitz LUK/3% on normal attacks |
| 901 | Steel Crow | OK |
| 902 | Detect | NEEDS REVIEW — 7x7 AoE reveal |
| 903 | Ankle Snare | NEEDS REVIEW — immobilize 4-20s |
| 904 | Land Mine | NEEDS REVIEW — MISC damage (DEX+75*Lv), stun 35-55% |
| 905 | Remove Trap | OK |
| 906 | Shockwave Trap | NEEDS REVIEW — SP drain 20-80% |
| 907 | Claymore Trap | NEEDS REVIEW — Fire element, 5x5 AoE |

### Missing (10):
| Suggested ID | Name | Max Lv | Type | Target | Element | Key Details |
|-------------|------|--------|------|--------|---------|-------------|
| 908 | Skid Trap | 5 | Active | Ground | Neutral | Knockback 6-10 cells, no damage. SP: 10. |
| 909 | Sandman | 5 | Active | Ground | Neutral | Sleep 50-90% chance, 5x5 AoE. SP: 12. |
| 910 | Flasher | 5 | Active | Ground | Neutral | Blind 30s, 3x3 AoE. SP: 12. |
| 911 | Freezing Trap | 5 | Active | Ground | Water | Freeze 3-15s, 3x3 AoE + Water damage. SP: 10. |
| 912 | Blast Mine | 5 | Active | Ground | Wind | Wind MISC damage, auto-detonates, 3x3. SP: 10. |
| 913 | Spring Trap | 5 | Active | Ground | Neutral | Remote trap destroy, range 4-8 cells. SP: 10. |
| 914 | Talkie Box | 1 | Active | Ground | Neutral | Custom message trap, cosmetic. SP: 1. |
| 915 | Beast Bane | 10 | Passive | None | Neutral | +4 ATK/lv vs Brute and Insect race. |
| 916 | Falconry Mastery | 1 | Passive | None | Neutral | Enables falcon companion. |
| 917 | Phantasmic Arrow | 1 | Active | Single | Neutral | Quest skill. 150% ATK, no arrow consumed, 3-cell knockback. SP: 10. |

---

## BARD (IDs 1500-1549) — 19 skills (3 in codebase + 16 missing)

### Currently in codebase (3):
| Our ID | Name | Status |
|--------|------|--------|
| 1500 | Music Lessons | OK |
| 1501 | A Poem of Bragi | NEEDS REVIEW — reduces cast time + delay for allies in AoE |
| 1502 | Assassin Cross of Sunset | NEEDS REVIEW — ASPD boost for allies in AoE |

### Missing Solo Skills (8):
| Suggested ID | Name | Max Lv | Type | Key Details |
|-------------|------|--------|------|-------------|
| 1503 | Adaptation to Circumstances | 1 | Active | Cancel current performance. SP: 1. |
| 1504 | Encore | 1 | Active | Replay last performance at half SP. SP: 1. |
| 1505 | Dissonance | 5 | Active | Offensive song: Neutral damage in AoE. SP: 18-30. Prereq: Music Lessons 1. |
| 1506 | Frost Joker | 5 | Active | Chance to freeze all on-screen. SP: 12-20. Prereq: Encore 1. |
| 1507 | A Whistle | 10 | Performance | +FLEE and Perfect Dodge for allies. Prereq: Music Lessons 3. |
| 1508 | Apple of Idun | 10 | Performance | +MaxHP and HP regen for allies. Prereq: Music Lessons 10. |
| 1509 | Pang Voice | 1 | Active | Quest skill. Chance to confuse target. SP: 20. |
| 1510 | Unbarring Octave | 5 | Performance | Prevent skill use in AoE. Prereq: Adaptation 1. |

### Missing Ensemble Skills (8, shared with Dancer):
| Suggested ID | Name | Max Lv | Key Details |
|-------------|------|--------|-------------|
| 1530 | Lullaby | 5 | Sleep 15-35% in AoE. Prereq: Music Lessons 10. |
| 1531 | Mr. Kim A Rich Man | 5 | +EXP bonus in AoE. |
| 1532 | Eternal Chaos | 5 | Reduce DEF in AoE. |
| 1533 | A Drum on the Battlefield | 5 | +ATK/DEF in AoE. |
| 1534 | The Ring of Nibelungen | 5 | +ATK for Lv4 weapons in AoE. |
| 1535 | Loki's Veil | 5 | Prevent skills in AoE. |
| 1536 | Into the Abyss | 5 | No gemstone cost in AoE. |
| 1537 | Invulnerable Siegfried | 5 | +element resist in AoE. |

---

## DANCER (IDs 1520-1569) — 19 skills (3 in codebase + 16 missing)

### Currently in codebase (3):
| Our ID | Name | Status |
|--------|------|--------|
| 1520 | Dance Lessons | OK |
| 1521 | Service For You | NEEDS REVIEW — +MaxSP + reduce SP cost in AoE |
| 1522 | Humming | NEEDS REVIEW — +HIT for allies in AoE |

### Missing Solo Skills (8):
| Suggested ID | Name | Max Lv | Type | Key Details |
|-------------|------|--------|------|-------------|
| 1523 | Adaptation to Circumstances | 1 | Active | Cancel current dance. SP: 1. |
| 1524 | Encore | 1 | Active | Replay last dance at half SP. SP: 1. |
| 1525 | Ugly Dance | 5 | Active | Offensive dance: drains SP in AoE. SP: 23-31. Prereq: Dance Lessons 1. |
| 1526 | Scream | 5 | Active | Chance to stun all on-screen. SP: 12-20. Prereq: Encore 1. |
| 1527 | Please Don't Forget Me | 10 | Performance | Reduce ASPD + move speed for enemies. Prereq: Dance Lessons 3. |
| 1528 | Fortune's Kiss | 10 | Performance | +Critical rate for allies. Prereq: Dance Lessons 7. |
| 1529 | Charming Wink | 1 | Active | Quest skill. Chance to confuse target. SP: 20. |
| 1540 | Moonlit Water Mill | 5 | Performance | Prevent normal attacks in AoE. Prereq: Adaptation 1. |

### Missing Ensemble Skills (8, same as Bard):
Same IDs as Bard ensembles (1530-1537) — ensembles require both Bard+Dancer adjacent.

---

## PRIEST (IDs 1000-1029) — 19 skills (8 in codebase + 11 missing)

### Currently in codebase (8):
| Our ID | Name | Status |
|--------|------|--------|
| 1000 | Sanctuary | NEEDS REVIEW — SP: 15-42 (12+3*Lv). Cast: 5000ms. Heals per wave: 100-777. Catalyst: Blue Gem. |
| 1001 | Kyrie Eleison | NEEDS REVIEW — barrier HP: 12-30% MaxHP. Max hits: 5-10. SP: 20-35. Cast: 2000ms. |
| 1002 | Magnificat | NEEDS REVIEW — doubles SP regen. Duration: 30-90s. SP: 40. Cast: 4000ms. |
| 1003 | Gloria | NEEDS REVIEW — +30 LUK flat. Duration: 10-30s. Prereqs: Kyrie 4 + Magnificat 3. |
| 1004 | Resurrection | OK — cast: 6000/4000/2000/0ms. HP restored: 10/30/50/80%. Catalyst: Blue Gem. |
| 1005 | Magnus Exorcismus | NEEDS REVIEW — Cast: 15000ms (!). SP: 40-58. Only damages Undead element + Demon race. 7x7 AoE. Catalyst: Blue Gem. |
| 1006 | Turn Undead | NEEDS REVIEW — instant kill chance formula complex. SP: 20 flat. Only Undead element. |
| 1007 | Lex Aeterna | OK — doubles next damage on target. SP: 10. |

### Missing (11):
| Suggested ID | Name | Max Lv | Type | Target | Element | Key Details |
|-------------|------|--------|------|--------|---------|-------------|
| 1008 | Mace Mastery | 10 | Passive | None | Neutral | +3 ATK/lv with Maces. |
| 1009 | Impositio Manus | 5 | Active | Single | Neutral | +5 ATK/lv. Duration: 60s. SP: 13-25. |
| 1010 | Suffragium | 3 | Active | Single (ally, NOT self) | Neutral | -15/30/45% cast time on next spell. Duration: 30/20/10s. SP: 8. |
| 1011 | Aspersio | 5 | Active | Single | Holy | Endow weapon with Holy. Duration: 60-180s. SP: 14-30. Cast: 2000ms. Catalyst: Holy Water. |
| 1012 | B.S. Sacramenti | 5 | Active | Ground 3x3 | Holy | Endow armor with Holy. Duration: 40-200s. SP: 20. Requires 2 adjacent Acolyte-class chars. |
| 1013 | Slow Poison | 4 | Active | Single (ally) | Neutral | Pause poison HP drain. Duration: 10-40s. SP: 6-12. |
| 1014 | Status Recovery | 1 | Active | Single | Neutral | Cure Frozen/Stone/Stun. SP: 5. Delay: 2000ms. |
| 1015 | Lex Divina | 10 | Active | Single | Neutral | Silence 30-60s. Toggle: if already silenced, REMOVES it. Range: 5. SP: 10-20 (decreases at high lv). |
| 1016 | Increase SP Recovery | 10 | Passive | None | Neutral | Shared with Mage. +SP regen while idle + potion effectiveness. |
| 1017 | Safety Wall | 10 | Active | Ground 1x1 | Neutral | Shared with Mage. Blocks 2-11 melee hits. Duration: 5-50s. Catalyst: Blue Gem. |
| 1018 | Redemptio | 1 | Active | Self AoE | Holy | Quest skill. Mass resurrect all dead party in 15x15 area at 50% HP. Caster → 1 HP/1 SP. Costs 1% base EXP. SP: 400. Cast: 4000ms uninterruptible. |

---

## MONK (IDs 1600-1629) — 17 skills (6 in codebase + 11 missing)

### Currently in codebase (6):
| Our ID | Name | Status |
|--------|------|--------|
| 1600 | Iron Fists | NEEDS REVIEW — prereqs: Demon Bane 10 + Divine Protection 10 |
| 1601 | Summon Spirit Sphere | NEEDS REVIEW — +3 ATK per sphere (Holy, never misses). Duration: 600s. |
| 1602 | Occult Impaction | NEEDS REVIEW — damage scales with ENEMY DEF. Always Neutral. Consumes 1 sphere. |
| 1603 | Investigate | NOTE: This IS Occult Impaction (same skill, different name). Remove duplicate? |
| 1604 | Finger Offensive | NEEDS REVIEW — uses 1-5 spheres. Damage: 150-350% per sphere. Ranged. |
| 1605 | Asura Strike | NEEDS REVIEW — consumes ALL SP. Formula: (ATK)*(8+SP/10)+250+(150*Lv). Blocks regen 5 min. |

### Missing (11):
| Suggested ID | Name | Max Lv | Type | Target | Key Details |
|-------------|------|--------|------|--------|-------------|
| 1606 | Spirits Recovery | 5 | Passive | None | HP/SP regen while sitting, even over 50% weight. |
| 1607 | Absorb Spirit Sphere | 1 | Active | Single | Absorb spheres for SP. Self: 10 SP/sphere. Monster: 20% chance, 2 SP/monsLv. Player: steal spheres. |
| 1608 | Triple Attack | 10 | Passive | None | COMBO STARTER. 20-29% chance on normal attack to hit 3x. Damage: 120-300%. Proc chance DECREASES with level. |
| 1609 | Dodge | 10 | Passive | None | +1-15 FLEE. Prereqs: Iron Fists 5, Summon Spirit Sphere 5. |
| 1610 | Blade Stop | 5 | Active | Self | Catch melee attack, lock both in place. Duration: 20-60s. Unlocks skills during lock at higher levels. Costs 1 sphere. |
| 1611 | Critical Explosion (Fury) | 5 | Active | Self | +10-20 CRIT. Enables Asura Strike. Consumes 5 spheres. Blocks SP regen. Duration: 180s. |
| 1612 | Steel Body | 5 | Active | Self | Sets DEF/MDEF to 90. -25% ASPD + move speed. Cannot use active skills. Costs 5 spheres. SP: 200. Cast: 5000ms. Duration: 30-150s. |
| 1613 | Chain Combo | 5 | Active (combo) | Single | 4 hits, 200-400%. ONLY usable during Triple Attack delay window. SP: 11-15. |
| 1614 | Combo Finish | 5 | Active (combo) | Single | 300-540%. ONLY usable during Chain Combo delay window. 5x5 AoE splash. Costs 1 sphere. SP: 11-15. |
| 1615 | Ki Translation | 1 | Active | Single (party) | Quest skill. Transfer 1 sphere to party member. SP: 40. Cast: 2000ms. |
| 1616 | Ki Explosion | 1 | Active | Single | Quest skill. 300% ATK, 3x3 AoE, knockback 5, stun 2s. SP: 20. HP cost: 10. |

### Combo System:
```
Triple Attack (auto-proc) → Chain Combo (during delay) → Combo Finish (during delay) → Asura Strike (if in Fury + 4 spheres)
```

---

## ASSASSIN (IDs 1100-1119) — 12 skills (7 in codebase + 5 missing)

### Currently in codebase (7):
| Our ID | Name | Status |
|--------|------|--------|
| 1100 | Katar Mastery | OK |
| 1101 | Sonic Blow | NEEDS REVIEW — damage: 440-800% (not 50+50*Lv). 8 visual hits. Stun 12-30%. |
| 1102 | Grimtooth | NEEDS REVIEW — does NOT break Hiding. Lv1-2 melee type, Lv3-5 ranged type. Range: 3-7 cells. |
| 1103 | Cloaking | NEEDS REVIEW — Lv1-2 requires wall. Lv3+ can move off wall. Speed varies. SP drain varies. |
| 1104 | Poison React | NEEDS REVIEW — two-part: autocast Envenom on non-poison hits (50% chance) + counter poison attacks. |
| 1105 | Venom Dust | NEEDS REVIEW — 2x2 AoE, Catalyst: Red Gemstone. Duration: 5-50s. No damage, only applies Poison. |
| 1106 | Sonic Acceleration | OK — quest skill, +50% Sonic Blow damage |

### Missing (5):
| Suggested ID | Name | Max Lv | Type | Target | Key Details |
|-------------|------|--------|------|--------|-------------|
| 1107 | Righthand Mastery | 5 | Passive | None | Dual-wield right-hand damage: 60-100%. |
| 1108 | Lefthand Mastery | 5 | Passive | None | Dual-wield left-hand damage: 40-80% (never 100%). Prereq: Righthand 2. |
| 1109 | Enchant Poison | 10 | Active | Single | Endow weapon with Poison element. Poison proc: 3-7.5%. Duration: 30-165s. SP: 20. |
| 1110 | Venom Splasher | 10 | Active | Single | Timed bomb on target (2-11s). 500-1400% AoE on detonation. Target must be <75% HP. Catalyst: Red Gem. SP: 12-30. |
| 1111 | Throw Venom Knife | 1 | Active | Single | Quest skill. Ranged (10 cells). 100% ATK + poison chance. Consumes Venom Knife item. SP: 15. |

---

## ROGUE (IDs 1700-1729) — 19 skills (5 in codebase + 14 missing)

### Currently in codebase (5):
| Our ID | Name | Status |
|--------|------|--------|
| 1700 | Snatcher | NEEDS REVIEW — auto-steal chance: 7-20.5% |
| 1701 | Back Stab | NEEDS REVIEW — teleport behind target. Damage: 340-700%. Always hits. |
| 1702 | Tunnel Drive | OK — move while hiding |
| 1703 | Raid | NEEDS REVIEW — 7x7 AoE from Hiding. 200-800%. Stun/Blind 13-25%. +30% damage debuff 10s. |
| 1704 | Intimidate | NEEDS REVIEW — attack + random teleport both. 130-250%. Doesn't work on Boss. |

### Missing (14):
| Suggested ID | Name | Max Lv | Type | Target | Key Details |
|-------------|------|--------|------|--------|-------------|
| 1705 | Sword Mastery (Rogue) | 10 | Passive | None | Shared from Swordsman. +4 ATK/lv with Swords/Daggers. |
| 1706 | Vulture's Eye (Rogue) | 10 | Passive | None | Shared from Archer. +1 bow range + HIT per level. |
| 1707 | Double Strafe (Rogue) | 10 | Active | Single | Shared from Archer. 100-190% x2. SP: 12. Requires Bow. Prereq: Vulture's Eye 10. |
| 1708 | Remove Trap (Rogue) | 1 | Active | Ground | Remove ground traps. SP: 5. Prereq: Double Strafe 5. |
| 1709 | Steal Coin | 10 | Active | Single (monster) | Steal Zeny. Success: 1-10%. SP: 15. Prereq: Snatcher 4. |
| 1710 | Divest Helm | 5 | Active | Single | Strip headgear. Success: 10-30%. Duration: 75-135s. Reduces INT 40% on monsters. |
| 1711 | Divest Shield | 5 | Active | Single | Strip shield. Success: 10-30%. Reduces hard DEF 15% on monsters. Prereq: Divest Helm 5. |
| 1712 | Divest Armor | 5 | Active | Single | Strip armor. Success: 10-30%. Reduces VIT 40% on monsters. Prereq: Divest Shield 5. |
| 1713 | Divest Weapon | 5 | Active | Single | Strip weapon. Success: 10-30%. Reduces ATK 25% on monsters. Prereq: Divest Armor 5. |
| 1714 | Plagiarism | 10 | Passive | None | Copy last skill that hit you. Max copied level = skill level. +1-10% ASPD. Prereq: Intimidate 5. |
| 1715 | Gangster's Paradise | 1 | Passive | None | 2+ sitting Rogues = monsters won't attack. Prereq: Divest Shield 3. |
| 1716 | Compulsion Discount | 5 | Passive | None | NPC discount 9-25%. Prereq: Gangster's Paradise 1. |
| 1717 | Scribble | 1 | Active | Ground | Write text on ground (cosmetic). SP: 15. Catalyst: Red Gem. |
| 1718 | Close Confine | 1 | Active | Single | Quest skill. Lock both in place 15s. Caster gets +10 FLEE. SP: 25. |

---

## BLACKSMITH (IDs 1200-1239) — 24 skills (6 in codebase + 18 missing)

### Currently in codebase (6):
| Our ID | Name | Status |
|--------|------|--------|
| 1200 | Adrenaline Rush | NEEDS REVIEW — Axe/Mace only. Caster -30% delay, party -20%. SP: 20-32. Duration: 30-150s. |
| 1201 | Weapon Perfection | NEEDS REVIEW — 100% damage all sizes. SP: 18→10 (decreases). Duration: 10-50s. |
| 1202 | Power Thrust (Over Thrust) | NEEDS REVIEW — +5-25% ATK caster, +5% party. 0.1% weapon break. SP: 18→10. Duration: 20-100s. |
| 1203 | Maximize Power | NEEDS REVIEW — TOGGLE. Max weapon variance. SP drain: 1/1-5s. SP: 10 activate. |
| 1204 | Weaponry Research | NEEDS REVIEW — +2 HIT/lv + 2 ATK/lv + 1% forge rate/lv. Prereq: Hilt Binding 1. |
| 1205 | Skin Tempering | OK — +4% fire resist/lv + 1% neutral resist/lv. |

### Missing Combat/Passive Skills (6):
| Suggested ID | Name | Max Lv | Type | Key Details |
|-------------|------|--------|------|-------------|
| 1206 | Hammer Fall | 5 | Active (ground AoE) | 5x5 stun only (NO damage). Stun chance: 30-70%. SP: 10. |
| 1207 | Hilt Binding | 1 | Passive | +1 STR, +4 ATK, +10% duration to AR/WP/OT. |
| 1208 | Ore Discovery | 1 | Passive | Chance for extra ore drops. Prereq: Steel Tempering 1, Hilt Binding 1. |
| 1209 | Weapon Repair | 1 | Active | Fix broken equipment. SP: 30. Cast: 5000ms. Prereq: Weaponry Research 1. |
| 1210 | Greed | 1 | Active | Quest skill. Pick up all items in 5x5. SP: 10. |
| 1211 | Dubious Salesmanship | 1 | Passive | Quest skill. -10% Mammonite Zeny cost. |

### Missing Forging/Crafting Skills (12 — deferred):
| Suggested ID | Name | Max Lv | Key Details |
|-------------|------|--------|-------------|
| 1220 | Iron Tempering | 5 | Refine Iron Ore → Iron |
| 1221 | Steel Tempering | 5 | Refine Iron + Coal → Steel |
| 1222 | Enchanted Stone Craft | 5 | Create elemental stones |
| 1223 | Research Oridecon | 5 | +forge rate for Lv3 weapons |
| 1224 | Smith Dagger | 3 | Forge daggers |
| 1225 | Smith Sword | 3 | Forge 1H swords |
| 1226 | Smith Two-Handed Sword | 3 | Forge 2H swords |
| 1227 | Smith Axe | 3 | Forge axes |
| 1228 | Smith Mace | 3 | Forge maces |
| 1229 | Smith Knucklebrace | 3 | Forge knuckle weapons |
| 1230 | Smith Spear | 3 | Forge spears |

---

## ALCHEMIST (IDs 1800-1829) — 17 skills (5 in codebase + 12 missing)

### Currently in codebase (5):
| Our ID | Name | Status |
|--------|------|--------|
| 1800 | Pharmacy | NEEDS REVIEW — crafting, +3% success/lv. Prereq: Potion Research 5. |
| 1801 | Acid Terror | NEEDS REVIEW — ignores hard DEF, always hits. 140-300%. Armor break 3-13%. Bleeding 3-15%. Catalyst: Acid Bottle. |
| 1802 | Demonstration | NEEDS REVIEW — 3x3 fire ground zone, hits every 0.5s. Weapon break chance. Catalyst: Bottle Grenade. Duration: 40-60s. |
| 1803 | Summon Flora | NEEDS REVIEW — summon plant allies. Lv1=5 plants, Lv5=1 strong plant. Catalyst: Plant Bottle. |
| 1804 | Axe Mastery | OK — +3 ATK/lv with axes. |

### Missing (12):
| Suggested ID | Name | Max Lv | Type | Target | Key Details |
|-------------|------|--------|------|--------|-------------|
| 1805 | Potion Research | 10 | Passive | None | +5% potion effectiveness/lv + 1% creation rate/lv. |
| 1806 | Potion Pitcher | 5 | Active | Single | Throw potions at allies. Red/Orange/Yellow/White/Blue Potion. +10-50% effectiveness. SP: 1. |
| 1807 | Summon Marine Sphere | 5 | Active | Ground | Summon explosive sphere. HP: 2400-4000. Fire AoE 11x11 on detonation. Catalyst: Marine Sphere Bottle. |
| 1808 | Chemical Protection Helm | 5 | Active | Single | Protect headgear from break/strip. Duration: 120-600s. Catalyst: Glistening Coat. SP: 20. |
| 1809 | Chemical Protection Shield | 5 | Active | Single | Protect shield. Duration: 120-600s. Catalyst: Glistening Coat. SP: 25. Prereq: CP Helm 3. |
| 1810 | Chemical Protection Armor | 5 | Active | Single | Protect armor. Duration: 120-600s. Catalyst: Glistening Coat. SP: 25. Prereq: CP Shield 3. |
| 1811 | Chemical Protection Weapon | 5 | Active | Single | Protect weapon. Duration: 120-600s. Catalyst: Glistening Coat. SP: 30. Prereq: CP Armor 3. |
| 1812 | Bioethics | 1 | Passive | None | Quest unlock for Homunculus skills. |
| 1813 | Call Homunculus | 1 | Active | Self | Summon/recall Homunculus. Catalyst: Embryo (first time). SP: 10. Prereq: Bioethics 1, Rest 1. |
| 1814 | Rest (Vaporize) | 1 | Active | Self | Dismiss Homunculus. SP: 50. Prereq: Bioethics 1. |
| 1815 | Resurrect Homunculus | 5 | Active | Self | Revive dead Homunculus. HP restored: 20-100%. SP: 50-74. Prereq: Call Homunculus 1. |

---

## EXCLUDED SKILLS (Transcendent / Soul Link — NOT for implementation)

### Lord Knight: Aura Blade, Parrying, Concentration, Tension Relax, Berserk, Spiral Pierce, Head Crush, Joint Beat
### Paladin: Pressure/Gloria Domini, Sacrifice/Martyr's Reckoning, Gospel/Battle Chant, Shield Chain
### High Wizard: Mystical Amplification, Napalm Vulcan, Gravitational Field, Ganbantein
### Professor: Soul Change, Spider Web, Mind Breaker, Double Casting, Wall of Fog, Health Conversion
### Sniper: Falcon Assault, Sharp Shooting, Wind Walk, True Sight
### Clown/Gypsy: Moonlit Water Mill (Marionette Control), Tarot Card of Fate, Longing for Freedom
### High Priest: Meditatio, Assumptio, Basilica
### Champion: Zen, Glacier Fist, Chain Crush Combo, Palm Push Strike
### Assassin Cross: Soul Destroyer, Enchant Deadly Poison, Create Deadly Poison, Meteor Assault
### Stalker: Full Divestment, Preserve, Reject Sword
### Whitesmith: Cart Termination, Maximum Over Thrust, Meltdown
### Creator: Acid Bomb, Full Chemical Protection, Cultivate Plant

### Soul Link skills (granted by Soul Linker class):
- Knight: One-Hand Quicken
- Hunter: Beast Strafing
- Bard/Dancer: Cross-gender performances
- Blacksmith: Full Adrenaline Rush
- Alchemist: Berserk Pitcher, Twilight Alchemy I/II/III
