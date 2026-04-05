# Lex Aeterna Cross-System Integration Audit

**Date**: 2026-03-22
**Auditor**: Claude Opus 4.6 (automated, 7-pass)
**Scope**: All damage application points in `server/src/index.js`
**Rule**: Lex Aeterna doubles the FIRST damage instance on a target, then is consumed. Should apply to direct attacks and skills from PLAYERS. Should NOT apply to: DoT/tick damage, reflect damage, self-damage, enemy->player damage, summon/pet damage, trap damage, ground effect periodic ticks.

---

## Lex Aeterna Skill Handler Analysis (Pass 6)

**Location**: Line ~17453 (`skill.name === 'lex_aeterna'`)
**Skill ID**: 1013 (listed in skill registry at line 15788)

### Application Logic
- **Enemy target path** (line 17457-17491): Checks frozen/stoned immunity, applies buff with `name: 'lex_aeterna'`, `doubleNextDamage: true`, duration 600000ms (10 minutes). Correct -- lasts until consumed or expires.
- **Player target path** (line 17492-17515): Same logic for PvP targets.
- **Frozen/Stone block** (line 17472): Correctly blocks application on frozen/stoned targets (rAthena behavior).
- **Boss immunity**: Correctly NOT immune -- LA works on bosses in pre-renewal.
- **Abracadabra LA** (line 16378): Also applies LA via Abracadabra random cast. Same buff structure.

### Verdict: CORRECT. No issues with the skill handler itself.

---

## All Damage Application Points Catalog (Pass 1-2)

### Legend
- **Should Check LA**: YES = direct player skill/attack on enemy; NO = DoT/reflect/self/enemy-source/summon/trap
- **Does Check LA**: Whether the code currently checks for `lex_aeterna` buff
- **Consumes Correctly**: Calls `removeBuff()` + broadcasts `skill:buff_removed`

---

### CATEGORY A: Helper Functions (Shared Damage Paths)

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| A1 | 1945 | `executePhysicalSkillOnEnemy()` | YES | YES | YES | Used by: Mammonite, Soul Destroyer, Back Stab (via options), Ki Explosion primary, etc. |

**Skills using `executePhysicalSkillOnEnemy`**: Any skill calling this helper gets LA for free. This includes Mammonite (600), Sonic Blow, Cart Revolution, Soul Destroyer physical, Ki Explosion primary hit, and others that delegate to this function.

---

### CATEGORY B: Player Auto-Attack (PvE Combat Tick)

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| B1 | 25070 | Right-hand auto-attack (PvE) | YES | YES | YES | Main auto-attack damage |
| B2 | 25211 | Left-hand (dual wield) auto-attack | NO | NO | N/A | LA already consumed on right-hand hit (B1). Correct behavior -- LA doubles the FIRST hit only |
| B3 | 25244 | Left-hand (katar) auto-attack | NO | NO | N/A | Same as B2 -- katar left-hand follows right-hand |
| B4 | 25270 | Splash attack (Baphomet card etc.) | NO | NO | N/A | Splash from auto-attack, not a separate damage instance against the primary target. LA consumed on main hit |
| B5 | 25302 | Magnum Break fire bonus (auto-attack) | NO | NO | N/A | Bonus fire damage added to same attack, not a separate LA-eligible hit |
| B6 | 25428 | Hindsight autocast (Sage) | NO | NO | N/A | Auto-spell proc, not a direct player action. rAthena does not consume LA on auto-cast procs |
| B7 | 25567 | Double Attack passive | NO | NO | N/A | Second hit of same attack -- LA consumed on first hit. Correct |
| B8 | 25618 | Triple Attack passive | NO | NO | N/A | Multi-hit passive proc, not a separate LA-eligible instance |
| B9 | 25142 | Autoberserk splash (auto-attack) | NO | NO | N/A | Card auto-spell splash |
| B10 | 25691 | Autoberserk splash (2nd instance) | NO | NO | N/A | Same as B9 |

---

### CATEGORY C: Player Auto-Attack (PvP Combat Tick)

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| C1 | 26188 | PvP auto-attack damage | YES | NO | N/A | **VIOLATION**: PvP auto-attacks do not check LA on target player |

---

### CATEGORY D: First Job Skills (Swordsman/Mage/Archer/Acolyte/Thief/Merchant)

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| D1 | 9688 | Bash (101) - enemy | YES | NO | N/A | **VIOLATION**: Bash does not check LA |
| D2 | 9688 | Bash (101) - player PvP | YES | NO | N/A | **VIOLATION**: Bash PvP path also missing |
| D3 | 9928 | Magnum Break (102) - enemy AoE | YES | NO | N/A | **VIOLATION**: MB enemy hits missing LA |
| D4 | 9998 | Magnum Break (102) - player PvP AoE | YES | NO | N/A | **VIOLATION**: MB PvP hits missing LA |
| D5 | 10216 | Napalm Beat (209) - bundled | YES | YES | YES | Multi-hit bundled, LA on total |
| D6 | 10364 | Soul Strike (210) - bundled | YES | YES | YES | Multi-hit bundled, LA on total |
| D7 | 10527 | Napalm Beat AoE splash targets | YES | YES | YES | Per-target LA check |
| D8 | 10633 | Fire Ball (207) AoE splash | YES | YES | YES | Per-target LA check |
| D9 | 10781 | Thunderstorm (212) - multi-hit enemy | YES | YES | YES | Bundled, LA on total |
| D10 | 10855 | Thunderstorm (212) - PvP | YES | YES | YES | Bundled, LA on total |
| D11 | 10973 | Fire Ball direct (207) | YES | YES | YES | |
| D12 | 11347 | Heal vs Undead (400) | YES | NO | N/A | **VIOLATION**: Heal offensive path missing LA |
| D13 | 12148 | Holy Light (413) | YES | YES | YES | |
| D14 | 12218 | Double Strafe (303) | YES | NO | N/A | **VIOLATION**: DS missing LA |
| D15 | 12271 | Arrow Shower (304) AoE per-target | YES | NO | N/A | **VIOLATION**: Arrow Shower missing LA |
| D16 | 12390 | Arrow Repel (306) | YES | NO | N/A | **VIOLATION**: Arrow Repel missing LA |
| D17 | 12655 | Envenom (504) | YES | NO | N/A | **VIOLATION**: Envenom missing LA |
| D18 | 12840 | Throw Stone (1) | YES | NO | N/A | **VIOLATION**: Throw Stone missing LA. Though it deals fixed 50 MISC damage, LA still applies per rAthena |

---

### CATEGORY E: Knight Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| E1 | 13010 | Pierce (701) per-hit | YES | YES | YES | Per-target per-first-hit |
| E2 | 13157 | Spear Stab (702) bundled | YES | YES | YES | |
| E3 | 13258 | Spear Boomerang (703) per-target | YES | YES | YES | |
| E4 | 13388 | Brandish Spear (704) per-target | YES | YES | YES | |
| E5 | 13556 | Bowling Bash (705) - first hit eligible | YES | YES | YES | Uses `isLexEligible` flag per-target |
| E6 | 13754 | Holy Cross (1301) | YES | YES | YES | |
| E7 | 13920 | Grand Cross (1302) - enemy ticks | YES | YES | YES | Only first tick (tickNum===0). Correct. |
| E8 | 14046 | Shield Boomerang (1305) | YES | YES | YES | |

---

### CATEGORY F: Wizard/Sage Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| F1 | 14533 | Jupitel Thunder (806) | YES | YES | YES | First hit only, uses `jtLexApplied` flag |
| F2 | 14619 | Earth Spike (815) | YES | YES | YES | First hit doubled |
| F3 | 14691 | Heaven's Drive (814) per-target | YES | YES | YES | First hit per target |
| F4 | 14770 | Water Ball (813) | YES | YES | YES | First hit doubled |
| F5 | 15068 | Sight Rasher (811) per-target | YES | YES | YES | |
| F6 | 15128 | Frost Nova (812) per-target | YES | YES | YES | |

---

### CATEGORY G: Crusader Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| G1 | 14366 | Heal Crusader (1311) vs Undead | YES | NO | N/A | **VIOLATION**: Crusader Heal offensive path missing LA |

---

### CATEGORY H: Priest Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| H1 | 15444 | Soul Breaker Lv5 HP damage (Magic Rod counter) | NO | NO | N/A | This is a utility drain, not a direct damage skill. Borderline, but at 2% MaxHP it is a minor effect |
| H2 | 17425 | Turn Undead (1012) fail damage | YES | NO | N/A | **VIOLATION**: Turn Undead fail path deals holy piercing damage but missing LA |

---

### CATEGORY I: Bard/Dancer Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| I1 | 16635 | Musical Strike (1509) / Slinging Arrow | YES | YES | YES | |
| I2 | 16752 | Melody Strike AoE / Guitar Twang AoE | YES | YES | YES | Per-target |

---

### CATEGORY J: Monk Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| J1 | 17115 | Raging Trifecta Blow (TVK, Combo 1602) | YES | NO | N/A | **VIOLATION**: Combo attack missing LA |
| J2 | 18274 | Investigate (1606) | YES | YES | YES | |
| J3 | 18354 | Finger Offensive (1607) per-hit | YES | YES (pre-loop) | YES | Uses `foLexActive` flag, consumed once before loop. All hits doubled. Correct |
| J4 | 18473 | Asura Strike (1605) | YES | YES | YES | |
| J5 | 18523 | Ki Explosion (1615) splash | YES | NO | N/A | **VIOLATION**: KE splash targets missing LA. Primary hit goes through `executePhysicalSkillOnEnemy` (has LA), but splash targets do not |
| J6 | 18606 | Chain Combo (1608) per-hit | NO | NO | N/A | Multi-hit timed, same target -- should consume on first hit only. Actually this IS a separate skill cast, so it SHOULD check. **VIOLATION** |
| J7 | 18684 | Combo Finish (1609) | YES | YES | YES | |

---

### CATEGORY K: Assassin/Rogue Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| K1 | 19544 | Back Stab (1701) per-hit | YES | NO | N/A | **VIOLATION**: Back Stab missing LA |
| K2 | 19597 | Raid (1703) AoE per-target | YES | NO | N/A | **VIOLATION**: Raid missing LA |
| K3 | 19655 | Intimidate (1704) | YES | NO | N/A | **VIOLATION**: Intimidate missing LA |
| K4 | 18714 | Sonic Blow splash (1107) | YES | NO | N/A | **VIOLATION**: SB splash missing LA. Main target handled by helper |

---

### CATEGORY L: Alchemist Skills

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| L1 | 20447 | Acid Terror (1801) | YES | YES | YES | |

---

### CATEGORY M: Abracadabra Random Casts

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| M1 | 16238 | Abracadabra magic damage | YES | YES | YES | |
| M2 | 16294 | Abracadabra physical damage | YES | YES | YES | |

---

### CATEGORY N: Auto-Cast / Reactive Systems (Combat Tick)

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| N1 | 3666 | Card autocast magic (executeAutoSpellEffect, magic) | NO | NO | N/A | Auto-spell procs don't consume LA in rAthena |
| N2 | 3698 | Card autocast physical (executeAutoSpellEffect, phys) | NO | NO | N/A | Same as N1 |
| N3 | 30657 | Auto Counter reactive counter-attack | NO | NO | N/A | Reactive counter from enemy attack, not player-initiated. Debatable -- rAthena may consume LA here but it's a defensive reaction |
| N4 | 31008 | Poison React Mode A counter | NO | NO | N/A | Reactive counter, not player-initiated |
| N5 | 31050 | Poison React Mode B auto-Envenom | NO | NO | N/A | Reactive, not player-initiated |
| N6 | 27109 | Sight Blaster reactive trigger | YES | YES | YES | Player buff, damage to enemies. Correctly checked |

---

### CATEGORY O: Ground Effect Ticks (Periodic Damage)

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| O1 | 27730 | Fire Wall (enemy hits) | NO | NO | N/A | Ground effect periodic tick, not a direct cast |
| O2 | 27829 | Fire Wall (PvP player hits) | NO | NO | N/A | Same as O1 |
| O3 | 27915 | Storm Gust tick | YES | YES | YES | Ground AoE first-tick LA per target. Correct (only first tick per enemy) |
| O4 | 27999 | Lord of Vermilion tick | YES | YES | YES | Same as O3 |
| O5 | 28088 | Meteor Storm tick | YES | YES | YES | Same as O3 |
| O6 | 28152 | Demonstration DoT tick | YES | YES | YES | First tick only consumes LA. Correct |
| O7 | 28317 | Fire Pillar trigger | NO | NO | N/A | Trap trigger, not a direct cast. Borderline, but Fire Pillar is a trap-like ground effect |
| O8 | 28450 | Sanctuary undead damage tick | NO | NO | N/A | Periodic ground heal/damage, not direct |
| O9 | 28514 | Magnus Exorcismus tick | NO | NO | N/A | Periodic ground AoE tick |

**Note on ground effects (O3-O6)**: These have LA checks on the FIRST tick per enemy only, which is the rAthena-correct behavior. Storm Gust, LoV, Meteor Storm, and Demonstration all correctly consume LA on the first damage tick each enemy receives from that ground effect instance.

---

### CATEGORY P: Trap Damage

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| P1 | 31197 | Sandman/Spring Trap damage | NO | NO | N/A | Trap MISC damage, not player-initiated |
| P2 | 31229 | Land Mine | NO | NO | N/A | Same |
| P3 | 31251 | Blast Mine splash | NO | NO | N/A | Same |
| P4 | 31267 | Claymore Trap splash | NO | NO | N/A | Same |
| P5 | 31298 | Freezing Trap | NO | NO | N/A | Same |

---

### CATEGORY Q: Summon/Pet/Homunculus Damage

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| Q1 | 26346 | Summon Flora plant auto-attack | NO | NO | N/A | Summon entity damage, not player action |
| Q2 | 26381 | Marine Sphere auto-detonate (timer) | NO | NO | N/A | Summon entity |
| Q3 | 21333 | Marine Sphere manual detonate | NO | NO | N/A | Summon entity |
| Q4 | 26486 | Homunculus auto-attack | NO | NO | N/A | Pet entity |
| Q5 | 21658 | Homunculus Moonlight skill | NO | NO | N/A | Pet entity skill |
| Q6 | 21733 | Homunculus Caprice skill | NO | NO | N/A | Pet entity skill |

---

### CATEGORY R: Enemy->Player Damage

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| R1 | 30906 | Enemy auto-attack (main pipeline) | NO | NO | N/A | Enemy damage, not player-initiated. LA only doubles player->target damage |
| R2 | 29184 | Monster skill damage (executeMonsterPlayerSkill) | NO | NO | N/A | Enemy-sourced |
| R3 | 29269 | NPC elemental_melee | NO | NO | N/A | Enemy-sourced |
| R4 | 29306 | NPC status_melee | NO | NO | N/A | Enemy-sourced |
| R5 | 29351 | NPC multi_hit | NO | NO | N/A | Enemy-sourced |
| R6 | 29377 | NPC force_crit | NO | NO | N/A | Enemy-sourced |
| R7 | 29404 | NPC aoe_damage | NO | NO | N/A | Enemy-sourced |
| R8 | 29463 | NPC drain_attack | NO | NO | N/A | Enemy-sourced |
| R9 | 29722 | NPC hp_percent_damage | NO | NO | N/A | Enemy-sourced |
| R10 | 29765 | NPC random_target_damage | NO | NO | N/A | Enemy-sourced |
| R11 | 29798 | NPC magic_ranged | NO | NO | N/A | Enemy-sourced |

---

### CATEGORY S: Reflect/Counter Damage

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| S1 | 30935 | Reflect Shield (Crusader) reflected damage | NO | NO | N/A | Reflect damage, not direct |
| S2 | 30971 | Card melee reflection | NO | NO | N/A | Reflect damage, not direct |

---

### CATEGORY T: Reveal/Passive Damage

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| T1 | 27045 | Ruwach reveal damage (PvP) | NO | NO | N/A | Passive buff reveal, targets players only (LA on players from player attacks is a PvP-specific edge case, and Ruwach is not a targeted skill) |
| T2 | 24872 | Venom Splasher detonation | YES | NO | N/A | **VIOLATION**: Venom Splasher is a player-cast skill that detonates after a delay. LA should be checked per-target |

---

### CATEGORY U: Performance/Song Damage

| # | Line | Handler/System | Should Check | Does Check | Consumes OK | Notes |
|---|------|---------------|-------------|------------|-------------|-------|
| U1 | 27244 | Dissonance tick damage | NO | NO | N/A | MISC periodic tick |
| U2 | 27305 | Dissonance overlap tick damage | NO | NO | N/A | MISC periodic tick |

---

### CATEGORY V: Grand Cross Self-Damage

Grand Cross deals self-damage to the caster (~20% MaxHP per tick). This is self-inflicted damage and should NOT trigger LA on the caster. Currently the LA check at line 13912 only checks `enemy.activeBuffs`, not the caster's buffs. **Correct behavior** -- self-damage is excluded.

---

## VIOLATIONS SUMMARY (Pass 3)

### Player Skills Missing LA Check (17 violations)

| # | Ref | Skill | Line | Type | Severity |
|---|-----|-------|------|------|----------|
| V1 | D1 | Bash (101) - enemy | 9688 | Single-target physical | HIGH |
| V2 | D2 | Bash (101) - PvP | 9688 | Single-target physical PvP | MEDIUM |
| V3 | D3 | Magnum Break (102) - enemy AoE | 9928 | AoE physical | HIGH |
| V4 | D4 | Magnum Break (102) - PvP AoE | 9998 | AoE physical PvP | MEDIUM |
| V5 | D12 | Heal vs Undead (400) | 11347 | Single-target holy | MEDIUM |
| V6 | D14 | Double Strafe (303) | 12218 | Single-target ranged | HIGH |
| V7 | D15 | Arrow Shower (304) AoE | 12271 | AoE ranged | HIGH |
| V8 | D16 | Arrow Repel (306) | 12390 | Single-target ranged | MEDIUM |
| V9 | D17 | Envenom (504) | 12655 | Single-target physical | MEDIUM |
| V10 | D18 | Throw Stone (1) | 12840 | MISC fixed damage | LOW |
| V11 | G1 | Heal Crusader (1311) vs Undead | 14366 | Single-target holy | MEDIUM |
| V12 | H2 | Turn Undead (1012) fail | 17425 | Single-target holy | MEDIUM |
| V13 | J1 | Raging Trifecta Blow (1602 combo) | 17115 | Single-target physical | HIGH |
| V14 | J5 | Ki Explosion (1615) splash | 18523 | AoE splash | MEDIUM |
| V15 | K1 | Back Stab (1701) | 19544 | Multi-hit physical | HIGH |
| V16 | K2 | Raid (1703) AoE | 19597 | AoE physical | MEDIUM |
| V17 | K3 | Intimidate (1704) | 19655 | Single-target physical | MEDIUM |

### Additional Missing Checks (3 violations)

| # | Ref | System | Line | Type | Severity |
|---|-----|--------|------|------|----------|
| V18 | C1 | PvP auto-attack | 26188 | Auto-attack PvP | MEDIUM |
| V19 | K4 | Sonic Blow (1107) splash | 18714 | AoE splash | MEDIUM |
| V20 | T2 | Venom Splasher (1110) detonation | 24872 | Delayed AoE | MEDIUM |

### Debatable / Edge Cases (4 items -- NOT counted as violations)

| # | Ref | System | Line | Reasoning |
|---|-----|--------|------|-----------|
| E1 | J6 | Chain Combo (1608) timed hits | 18606 | Multi-hit timed on same target. Could argue first hit should consume LA. Currently no check. Borderline because the hits are staggered via setTimeout |
| E2 | N3 | Auto Counter reactive | 30657 | Defensive reaction, not player-initiated attack. rAthena does apply LA on counter-attacks, but this is player-defending, not attacking |
| E3 | O7 | Fire Pillar trigger | 28317 | Trap-like ground effect. LA typically does not apply to traps, but Fire Pillar is cast by a player |
| E4 | H1 | Soul Breaker Lv5 HP drain | 15444 | Utility 2% MaxHP drain, not a combat skill |

---

## DOUBLE CONSUMPTION BUGS (Pass 4)

**None found.** Every damage path that checks LA does so exactly once:
- Helper function `executePhysicalSkillOnEnemy` checks LA internally. Skills that use this helper do NOT also check LA.
- Multi-hit skills (Pierce, Bowling Bash, Finger Offensive, etc.) use per-target flags (`isLexEligible`, `jtLexApplied`, `foLexActive`) to ensure single consumption.
- AoE skills (Napalm Beat, Thunderstorm, Grand Cross, etc.) check LA per-target independently, which is correct -- each enemy's LA is consumed separately.

---

## MISSED CONSUMPTION BUGS (Pass 5)

**None found.** Every LA check follows the pattern:
```js
const lexBuff = target.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
if (lexBuff) {
    damage *= 2;
    removeBuff(target, 'lex_aeterna');
    broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy, buffName: 'lex_aeterna', reason: 'consumed' });
}
```
All 30+ existing checks correctly: (1) double damage, (2) call `removeBuff()`, (3) broadcast removal. No path doubles without removing, and no path removes without doubling.

---

## FINAL VERIFICATION (Pass 7)

### Existing LA Check Locations (30 confirmed correct)
1. `executePhysicalSkillOnEnemy` (line 1948) -- shared helper
2. Napalm Beat bundled (line 10206)
3. Soul Strike bundled (line 10355)
4. Napalm Beat AoE splash (line 10520)
5. Fire Ball AoE splash (line 10626)
6. Thunderstorm multi-hit enemy (line 10771)
7. Thunderstorm PvP (line 10846)
8. Fire Ball direct (line 10965)
9. Holy Light (line 12139)
10. Pierce per-hit (line 13006)
11. Spear Stab bundled (line 13147)
12. Spear Boomerang per-target (line 13254)
13. Brandish Spear per-target (line 13384)
14. Bowling Bash per-target (line 13551)
15. Holy Cross (line 13745)
16. Grand Cross first-tick (line 13912)
17. Shield Boomerang (line 14038)
18. Jupitel Thunder first-hit (line 14523)
19. Earth Spike first-hit (line 14610)
20. Heaven's Drive per-target (line 14682)
21. Water Ball first-hit (line 14761)
22. Sight Rasher per-target (line 15059)
23. Frost Nova per-target (line 15120)
24. Abracadabra magic (line 16230)
25. Abracadabra physical (line 16286)
26. Musical Strike / Slinging Arrow (line 16628)
27. Melody Strike AoE (line 16749)
28. Investigate (line 18267)
29. Finger Offensive pre-loop (line 18333)
30. Asura Strike (line 18436)
31. Combo Finish (line 18681)
32. Acid Terror (line 20439)
33. PvE auto-attack right-hand (line 25073)
34. Sight Blaster reactive (line 27101)
35. Storm Gust first-tick (line 27907)
36. Lord of Vermilion first-tick (line 27991)
37. Meteor Storm first-tick (line 28080)
38. Demonstration first-tick (line 28144)

---

## RECOMMENDATIONS

### Priority 1 (HIGH -- Core Skills): Fix these first
1. **Bash (101)** at line ~9682: Add LA check before `target.health = Math.max(0, target.health - bashDmg)`. Per-target for both enemy and PvP paths.
2. **Magnum Break (102)** at line ~9927: Add per-target LA check in enemy AoE loop. Also PvP path at ~9998.
3. **Double Strafe (303)** at line ~12216: Add LA check before `enemy.health -= result.damage`.
4. **Arrow Shower (304)** at line ~12269: Add per-target LA check in AoE loop.
5. **Back Stab (1701)** at line ~19540: Add LA check on first hit (consume once for the entire multi-hit).
6. **Raging Trifecta Blow (1602)** at line ~17114: Add LA check before damage.

### Priority 2 (MEDIUM -- Secondary Skills): Fix next
7. **Arrow Repel (306)** at line ~12388
8. **Envenom (504)** at line ~12653
9. **Heal vs Undead (400)** at line ~11345
10. **Heal Crusader vs Undead (1311)** at line ~14365
11. **Turn Undead fail (1012)** at line ~17424
12. **Ki Explosion splash (1615)** at line ~18522
13. **Raid (1703)** at line ~19595 -- per-target in AoE loop
14. **Intimidate (1704)** at line ~19654
15. **PvP auto-attack** at line ~26187
16. **Sonic Blow splash (1107)** at line ~18713
17. **Venom Splasher detonation (1110)** at line ~24871 -- per-target

### Priority 3 (LOW): Minimal impact
18. **Throw Stone (1)** at line ~12839 -- fixed 50 damage, LA would make it 100. Minor.

### Implementation Pattern
For each missing check, insert this block BEFORE the `target.health = Math.max(0, ...)` line:
```js
// Lex Aeterna: double damage then consume
if (damage > 0 && target.activeBuffs) {
    const lexBuff = target.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
    if (lexBuff) {
        damage *= 2;
        removeBuff(target, 'lex_aeterna');
        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
    }
}
```
Adjust variable names (`damage`/`bashDmg`/`result.damage`, `target`/`enemy`, `targetId`/`eid`, `isEnemy`, `zone`) to match each handler's local scope.

---

## STATISTICS

| Category | Total Points | Should Check | Does Check | Missing | Correct |
|----------|-------------|-------------|------------|---------|---------|
| Player skills (D-M) | 42 | 42 | 25 | 17 | 25 |
| Auto-attack PvE | 10 | 1 | 1 | 0 | 1 |
| Auto-attack PvP | 1 | 1 | 0 | 1 | 0 |
| Venom Splasher | 1 | 1 | 0 | 1 | 0 |
| Sonic Blow splash | 1 | 1 | 0 | 1 | 0 |
| Ground effects | 9 | 4 | 4 | 0 | 4 |
| Traps | 5 | 0 | 0 | 0 | N/A |
| Summon/Pet/Homunculus | 6 | 0 | 0 | 0 | N/A |
| Enemy->Player | 11 | 0 | 0 | 0 | N/A |
| Reflect/Counter | 2 | 0 | 0 | 0 | N/A |
| Passive/Reactive | 6 | 1 | 1 | 0 | 1 |
| Performance ticks | 2 | 0 | 0 | 0 | N/A |
| **TOTALS** | **96** | **51** | **31** | **20** | **31** |

**Coverage: 31/51 eligible paths checked = 60.8%**
**Violations: 20 missing checks**
**Double consumption bugs: 0**
**Missed consumption bugs: 0**
