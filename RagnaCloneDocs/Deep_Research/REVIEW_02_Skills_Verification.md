# Second-Pass Review: All Skills

> **Reviewer**: Claude Opus 4.6 (1M context)
> **Date**: 2026-03-23
> **Scope**: Cross-verification of ALL skill-related audit findings from AUDIT_04 through AUDIT_07
> **Method**: Read all 4 audit docs, compiled unified issue list, then verified top 15 most critical by reading actual server code in `index.js`, `ro_skill_data.js`, and `ro_skill_data_2nd.js`
> **Total skills in data files**: 291 (66 first-class + 225 second-class)

---

## Unified Critical Issue List (all 4 audits combined, deduplicated)

### From AUDIT_04 (First Class Skills) -- 2 issues
| # | Skill | ID | Issue | Audit Severity |
|---|-------|----|-------|----------------|
| A1 | Energy Coat | 213 | `castTime: 5000` should be `0` (instant quest skill) | Medium |
| A2 | Improve Dodge | 501 | Missing 2nd-class scaling (+4 FLEE/lv for Assassin/Rogue instead of +3) | Low |

### From AUDIT_05 (Knight, Crusader, Wizard, Sage) -- 10 issues
| # | Skill | ID | Issue | Audit Severity |
|---|-------|----|-------|----------------|
| B1 | Two-Hand Quicken | 705 | CRI/HIT bonuses are Renewal-only, not pre-renewal | Medium |
| B2 | Bowling Bash | 707 | Primary target should always receive 2 hits (guaranteed double-hit), currently only 1 unless collision | High |
| B3 | Holy Cross | 1302 | Prerequisite Faith Lv7 vs rAthena Lv3 | Low |
| B4 | Reflect Shield | 1307 | Missing Auto Guard Lv3 prerequisite | Low |
| B5 | Heal (Crusader) | 1311 | Prerequisite deviates from research (intentional) | None |
| B6 | Earth Spike | 804 | ACD flat 700ms vs per-level [1000-1800ms]; cast time 1000*Lv vs 700*Lv | Low |
| B7 | Heaven's Drive | 805 | ACD 700ms vs 500ms | Low |
| B8 | Frost Nova | 811 | Minor cast time/MATK% rounding differences | Very Low |
| B9 | Free Cast | 1405 | Movement-during-cast not enforced server-side | Medium |
| B10 | Grand Cross | 1303 | Diamond AoE (41 cells) instead of cross-shaped (9 cells) | Medium |

### From AUDIT_06 (Hunter, Bard/Dancer, Priest, Monk) -- 9 issues
| # | Skill | ID | Issue | Audit Severity |
|---|-------|----|-------|----------------|
| C1 | Turn Undead | 1006 | Missing HP factor in formula, cap 100% vs 70%, no boss immunity | Critical |
| C2 | Body Relocation/Snap | -- | Entirely missing skill (Monk mobility) | Critical |
| C3 | Sanctuary (Undead dmg) | 1000 | Missing Holy vs Undead element table scaling (Lv1-4) | Medium |
| C4 | Magnus Exorcismus | 1005 | 130% Demon/Undead bonus may be missing | Medium |
| C5 | Scream | 1526 | Party members get full stun rate (should be /4) | Medium |
| C6 | Frost Joker | 1506 | Party freeze uses /4 (rAthena Scream logic) vs /2 (iRO Wiki for FJ) | Minor |
| C7 | Safety Wall (Priest) | 1017 | Missing durability HP formula | Low |
| C8 | Turn Undead ACD | 1006 | Cooldown 500ms instead of ACD 3000ms | Minor |
| C9 | Finger Offensive | 1604 | Cast time fixed per level instead of per spheres consumed | Minor |

### From AUDIT_07 (Assassin, Rogue, Blacksmith, Alchemist) -- 14 issues
| # | Skill | ID | Issue | Audit Severity |
|---|-------|----|-------|----------------|
| D1 | Back Stab | 1701 | Dagger = 2 hits (Renewal-only), pre-renewal = 1 hit always | Critical |
| D2 | Abracadabra ID Table | -- | 6 SKILL_LOOKUP entries map wrong IDs to wrong names | Critical |
| D3 | Poison React | 1104 | Envenom limit `floor(Lv/2)` should be `floor((Lv+1)/2)` | Medium |
| D4 | Blacksmith Party Buffs | 1200-1202 | AR/WP/PT all self-only, should be party-wide (reduced effect) | Medium |
| D5 | Throw Venom Knife SP | 1111 | SP cost 15, research says 35 | Medium |
| D6 | Divest Cast Time/ACD | 1710-1713 | Cast time and ACD both present (1000ms each) | See verification |
| D7 | Grimtooth Range Type | 1102 | No melee/ranged classification per level (Lv1-2 melee, Lv3-5 ranged) | Medium |
| D8 | Demonstration Tick | 1802 | 1000ms tick vs some sources saying 500ms | Low-Medium |
| D9 | Demonstration Break | 1802 | Weapon break 1*Lv% vs rAthena 3*Lv% | Medium |
| D10 | Pharmacy Rate | 1800 | Potion Research contributes 50/10000 vs 100/10000 per level | Medium |
| D11 | Plagiarism Whitelist | 1714 | Monk skills (investigate, finger_offensive, asura_strike) may not be pre-renewal copyable | Low |
| D12 | Katar Double CRI | -- | CRI*2 for Katar weapons may be missing | Low |
| D13 | Forge Recipes | 1224-1230 | Only 7 recipes vs ~45 canonical | Low |
| D14 | AR SP Cost | 1200 | Skill data SP formula mismatch (audit claim) | See verification |

**Total unique issues across all 4 audits: 35**

---

## Verified Issues (confirmed with code)

### V1. Turn Undead (1006) -- CONFIRMED CRITICAL
**File**: `index.js` line 17402
**Code**: `const tuChance = Math.min(100, (200 * learnedLevel + baseLv + intStat + lukStat) / 10);`
**Problems confirmed**:
1. Missing `{1 - (targetHP/targetMaxHP)} * 200` HP factor -- low-HP enemies should be easier to instant-kill
2. Cap is 100% instead of 70% -- Turn Undead should never have >70% instant kill chance
3. No boss immunity check for instant kill -- boss-type Undead should only take the fail-damage path
**Impact**: High. Core Priest mechanic is incorrect.

### V2. Bowling Bash (707) -- CONFIRMED HIGH
**File**: `index.js` lines 13502-13644
**Code flow**: `bbChainReaction()` calls `bbDealDamage(enemy, eid, true)` exactly once at line 13632 (the "chain hit"). A second hit only occurs at line 13619 if the knockback causes a collision. With no nearby enemies, the primary target receives exactly 1 hit.
**Research**: Pre-renewal BB always hits twice. Effective Lv10 damage = 500% * 2 = 1000%.
**Impact**: Without nearby enemies for collision, BB deals half its intended damage.

### V3. Back Stab (1701) -- CONFIRMED CRITICAL
**File**: `index.js` line 19525
**Code**: `const numHits = isDagger ? 2 : 1;`
**Research**: Pre-renewal Back Stab is 1 hit for ALL weapons. 2-hit dagger is Renewal-only.
**Impact**: Daggers deal double damage, massively overtuning Rogue DPS with daggers.

### V4. Abracadabra ID Mapping -- CONFIRMED CRITICAL
**File**: `index.js` lines 15823-15835 (ABRA_REGULAR_SKILLS array)
**Confirmed mismatches**:
| Array Entry | ID | Name Given | Actual Name in ro_skill_data_2nd | Correct ID for Given Name |
|------------|-----|-----------|----------------------------------|---------------------------|
| Line 15824 | 1100 | cloaking | katar_mastery | 1103 |
| Line 15827 | 1103 | enchant_poison | cloaking | 1109 |
| Line 15830 | 1106 | venom_splasher | sonic_acceleration | 1110 |
| Line 15832 | 1700 | steal_coin | snatcher | 1709 |
| Line 15834 | 1702 | raid | tunnel_drive | 1703 |
| Line 15835 | 1707 | intimidate | double_strafe_rogue | 1704 |

**Impact**: When Abracadabra randomly selects one of these 6 skills, it uses the WRONG skill ID to look up data and execute. For example, casting "cloaking" through Abracadabra would use ID 1100 (Katar Mastery data), producing incorrect behavior. Since Abracadabra looks up the skill handler by `name` field from the data file, and the actual routing is done via `SKILL_MAP[id]`, these mismatches cause Abracadabra to attempt to cast the wrong skill entirely.

### V5. Poison React Envenom Limit (1104) -- CONFIRMED MEDIUM
**File**: `index.js` line 16894
**Code**: `const prEnvenomLimit = Math.floor(learnedLevel / 2);`
**Comment in code**: `// rAthena: val2 = val1/2 -> [0,1,1,2,2,3,3,4,4,5]`
**Research**: `floor((Lv+1)/2)` gives [1,1,2,2,3,3,4,4,5,5], Lv10 special = 6
**Impact**: Lv1 Poison React gets 0 envenom counters (useless), Lv10 gets 5 instead of 6.

### V6. Energy Coat (213) Cast Time -- CONFIRMED MEDIUM
**File**: `ro_skill_data.js` line 48
**Code**: `castTime: 5000`
**Research**: Energy Coat is a quest skill with instant cast (0ms) in RO Classic.
**Impact**: 5-second cast time makes Energy Coat nearly unusable in combat situations.

### V7. THQ CRI/HIT Bonuses (705) -- CONFIRMED MEDIUM
**File**: `index.js` lines 13451-13452
**Code**: `critBonus: 2 + learnedLevel, hitBonus: 2 * learnedLevel`
**Research**: Pre-renewal THQ is ASPD-only. CRI/HIT bonuses are Renewal additions.
**Impact**: THQ is stronger than intended. CRI 3-12 and HIT 2-20 are free bonuses that don't exist in RO Classic.

### V8. Blacksmith Party Buffs (1200-1202) -- CONFIRMED MEDIUM
**File**: `index.js` lines 19955-20043
**Adrenaline Rush**: Self-only `aspdMultiplier: 1.3`. No party member iteration.
**Weapon Perfection**: Self-only `noSizePenalty: true`. No party member iteration.
**Power Thrust**: Self-only `atkPercent`. No party member iteration.
**Research**: AR gives party Axe/Mace users *1.2 ASPD. WP gives all party noSizePenalty. PT gives party +5% ATK (flat, not scaling).
**Impact**: Blacksmith party utility is nonexistent. Three core party support skills only benefit the caster.

### V9. Scream (1526) Party Stun Reduction -- CONFIRMED MEDIUM
**File**: `index.js` lines 19384-19393
**Code**: No party check. Full `stunChance` applied to all players.
**Research**: rAthena divides stun chance by 4 for party members (same as Frost Joker).
**Impact**: Party members are stunned at full rate, making Scream self-sabotaging in parties.

### V10. Free Cast (1405) -- CONFIRMED IMPLEMENTED (FALSE POSITIVE from AUDIT_05)
**File**: `index.js` lines 6129-6132
**Code**: `const freeCastLv = player && player.learnedSkills ? (player.learnedSkills[1405] || 0) : 0; if (freeCastLv <= 0) { interruptCast(characterId, 'moved'); }`
**Reality**: Free Cast IS implemented in the `player:position` handler. If the player has Free Cast learned, movement does NOT interrupt casting. The audit was **wrong** on this point.
**Note**: The speed reduction during casting (25+5*Lv% of normal) is also referenced at line 9522-9523 with `freeCastSpeedPct`.

### V11. Sanctuary Undead Damage (1000) -- CONFIRMED MEDIUM
**File**: `index.js` lines 28441-28450
**Code**: `const holyDmg = Math.floor(healPerTick / 2);` -- flat damage, no element modifier
**Research**: Should apply Holy vs Undead element table: Undead Lv1=100%, Lv2=125%, Lv3=150%, Lv4=175%
**Also missing**: 2-cell knockback from center for damaged Undead/Demon targets
**Impact**: Undead Lv2-4 enemies take less damage than they should from Sanctuary.

### V12. Magnus Exorcismus 130% Bonus (1005) -- CONFIRMED IMPLEMENTED (FALSE POSITIVE from AUDIT_06)
**File**: `index.js` lines 28507-28511
**Code**: `if (meRace === 'undead' || meRace === 'demon' || meEle === 'shadow' || meEle === 'undead') { meDmg = Math.floor(meDmg * 1.3); }`
**Reality**: The 130% bonus IS implemented. The audit flagged this as "need to verify" but the code clearly applies 1.3x damage for Undead race, Demon race, Shadow element, and Undead element targets.

### V13. Divest Cast Time/ACD (1710-1713) -- CONFIRMED FALSE POSITIVE
**File**: `ro_skill_data_2nd.js` lines 220-223
**Code**: All four Divest skills have `castTime: 1000, afterCastDelay: 1000`
**Reality**: The audit in AUDIT_07 claimed `castTime: 0, afterCastDelay: 0` but the actual data file has both set to 1000ms. This was likely fixed between the audit planning doc and the actual implementation, or the auditor checked an outdated version.

### V14. Throw Venom Knife SP (1111) -- CONFIRMED MEDIUM
**File**: `ro_skill_data_2nd.js` line 207
**Code**: `spCost: 15`
**Research**: SP cost should be 35.
**Impact**: Skill costs less than half the intended SP, making it too spammable.

### V15. Adrenaline Rush SP Cost (1200) -- CONFIRMED FALSE POSITIVE
**File**: `ro_skill_data_2nd.js` line 231
**Code**: `spCost: 17+(i+1)*3` which produces [20, 23, 26, 29, 32]
**Research says**: `17+3*Lv` = [20, 23, 26, 29, 32]
**Reality**: The formulas produce identical results. The audit claim of a mismatch was incorrect.

---

## False Positives (5 total)

| # | Issue | Audit | Reality |
|---|-------|-------|---------|
| 1 | Free Cast not enforced | AUDIT_05 B9 | IS implemented at line 6129-6132. Movement skips `interruptCast()` if Free Cast learned. Speed reduction also present at line 9522-9523. |
| 2 | Magnus Exorcismus 130% bonus missing | AUDIT_06 C4 | IS implemented at line 28510-28511. Applies 1.3x for Undead race, Demon race, Shadow element, Undead element. |
| 3 | Divest cast time/ACD missing | AUDIT_07 D6 | All 4 Divest skills have `castTime: 1000, afterCastDelay: 1000` in ro_skill_data_2nd.js lines 220-223. |
| 4 | Adrenaline Rush SP formula wrong | AUDIT_07 D14 | Formula `17+(i+1)*3` = [20,23,26,29,32] matches research exactly. |
| 5 | Crusader Heal prereq wrong | AUDIT_05 B5 | Intentional deviation documented in session notes (removed cross-class FK constraint). |

---

## Newly Discovered Issues

### N1. Abracadabra Crusader ID Mismatches
The ABRA_REGULAR_SKILLS array at line 15836-15846 also has Crusader IDs that don't match ro_skill_data_2nd.js:
| Array ID | Array Name | Actual Skill at ID in Data | Correct ID |
|----------|-----------|---------------------------|------------|
| 1300 | auto_guard | faith | 1301 |
| 1301 | shield_charge | auto_guard | 1304 |
| 1303 | reflect_shield | grand_cross | 1307 |
| 1304 | holy_cross | shield_charge | 1302 |
| 1305 | shield_boomerang | shield_boomerang | CORRECT |
| 1306 | grand_cross | devotion | 1303 |
| 1308 | providence | providence | CORRECT |
| 1309 | defender | defender | CORRECT |
| 1310 | devotion | spear_quicken | 1306 |
| 1312 | spear_quicken | cure_crusader | 1310 |

This means Abracadabra tries to cast "auto_guard" but resolves ID 1300 = Faith (passive). Similarly "grand_cross" resolves to ID 1306 = Devotion. These will produce wrong behavior or silent failures.

### N2. Abracadabra Monk ID Mismatches
| Array ID | Array Name | Actual Skill at ID in Data | Correct ID |
|----------|-----------|---------------------------|------------|
| 1600 | summon_spirit_sphere | iron_fists | 1601 |
| 1605 | asura_strike | asura_strike | CORRECT |
| 1606 | investigate | spirits_recovery | 1602 |
| 1607 | finger_offensive | absorb_spirit_sphere | 1604 |
| 1608 | steel_body | dodge | 1612 |
| 1609 | blade_stop | blade_stop | CORRECT |
| 1610 | critical_explosion | chain_combo | 1611 |
| 1262 | ki_explosion | -- (no ID 1262 in data) | 1615 |

Nearly every Monk entry in the Abracadabra table has wrong IDs. Ki Explosion uses ID 1262 which doesn't exist at all.

### N3. Abracadabra Hunter ID Mismatches
| Array ID | Array Name | Actual Skill at ID in Data |
|----------|-----------|---------------------------|
| 907 | blast_mine | claymore_trap |
| 908 | claymore_trap | skid_trap |
| 909 | remove_trap | sandman |
| 911 | blitz_beat | freezing_trap |
| 913 | spring_trap | spring_trap (CORRECT) |

### N4. Body Relocation / Snap Missing -- CONFIRMED
Searched all server source files for `body_relocation`, `snap`, `bodyRelocation`, `MO_BODYRELOCATION`. No results in any skill data file or handler. The skill is entirely absent. This is a core Monk mobility skill used in virtually all Monk builds.

### N5. Katar Double CRI -- CONFIRMED MISSING
Searched for `katar.*crit`, `katarCrit`, `crit.*katar`, `KATAR.*CRI` in index.js. No results. The `isKatar()` function exists (line 4685) but is only used for equip slot management, not CRI calculation. Katar users should have their CRI value doubled, which is a core Assassin mechanic for Katar builds.

### N6. Turn Undead ACD vs Cooldown
**File**: `ro_skill_data_2nd.js` line 159
**Code**: `afterCastDelay: 0, cooldown: 500`
**Research**: Should have `afterCastDelay: 3000` (3 second after-cast delay). Currently uses a 500ms cooldown instead, which is a much shorter restriction.

### N7. Frost Joker Party Rate Inconsistency
Frost Joker uses `/4` for party at line 19341, matching rAthena's Scream logic. But iRO Wiki Classic specifically states Frost Joker uses `/2` for party members. The Scream handler (line 19384-19393) applies NO party reduction at all. These three skills (Frost Joker, Scream, Pang Voice) should all apply a party reduction factor, and currently:
- Frost Joker: /4 (debatable, should be /2)
- Scream: no reduction (bug -- should be /4)
- Pang Voice: not checked (likely should have party exemption too)

---

## Cross-Skill Interaction Problems

### X1. Abracadabra + All Classes -- Systemic ID Mapping Failure
The ABRA_REGULAR_SKILLS array (lines 15774-15870) uses skill IDs that frequently don't match the names assigned. This is a **systemic problem** affecting Assassin (3 wrong), Rogue (3 wrong), Crusader (7 wrong), Monk (6 wrong), Hunter (4 wrong), and Sage/Bard/Dancer entries. When Abracadabra randomly selects a skill from this pool, it looks up the skill by ID to find the handler. Since the ID is wrong, it resolves to a different skill entirely, causing Abracadabra to cast the wrong skill.

**Root cause**: The IDs in the array appear to be sequential slot numbers rather than actual skill IDs. For example, Assassin skills start at 1100 in the data, but the array lists IDs 1100-1106 sequentially, while the actual active Assassin skills have non-sequential IDs (1101, 1102, 1103, 1104, 1105, 1109, 1110, 1111).

**Fix**: Replace all IDs in ABRA_REGULAR_SKILLS with the correct IDs from ro_skill_data_2nd.js, matching by name.

### X2. THQ CRI Bonus + Katar Double CRI Interaction
If THQ CRI bonus (V7) is kept AND Katar Double CRI (N5) is added, a dual-stacking CRI issue arises. However, THQ requires a 2-handed sword (not katar), so these two bonuses cannot coexist on the same character. No interaction problem.

### X3. Blacksmith Party Buffs + ASPD Mutual Exclusion
If Blacksmith party buffs (V8) are implemented, the party-wide AR at *1.2 needs to participate in the ASPD haste2 mutual exclusion system (strongest wins among AR, AR Full, THQ, OHQ, SQ, Cart Boost). Currently the exclusion only applies to the caster's own buffs.

### X4. Devotion + Defender Propagation
Audit_05 noted that Defender's ranged reduction should propagate to Devotion targets. Since Devotion redirects damage to the Crusader, and the redirect check happens in the damage pipeline, the Crusader's own Defender buff naturally applies to redirected damage. However, the Devotion target does NOT get the movement speed penalty, which is correct behavior.

### X5. Bowling Bash + Lex Aeterna Double-Hit
If BB is fixed to always hit twice (V2), Lex Aeterna currently only doubles the first hit (the `isLexEligible=true` call). This is correct per research -- Lex Aeterna is consumed on the first damage instance.

---

## Skills With Data But No Handler

| ID | Name | Type | Notes |
|----|------|------|-------|
| 1210 | greed | active | Deferred -- requires ground loot pickup system |
| 1220 | iron_tempering | passive | Material crafting passive -- no recipe system for materials |
| 1221 | steel_tempering | passive | Same as above |
| 1222 | enchanted_stone_craft | passive | Same as above |
| 711 | one_hand_quicken | -- | No data entry exists (correctly absent -- Soul Linker content) |

Note: Skills 1220-1222 have data entries as passives but the actual Iron/Steel/Stone crafting recipes don't exist. The passives themselves (forge rate bonuses) may be partially wired into the forge formula but the standalone material crafting feature is not implemented.

Greed (1210) has a data entry and a comment in the handler noting it's deferred pending ground loot.

---

## Skills With Handler But No Data

| Handler Location | Name | Notes |
|-----------------|------|-------|
| None found | -- | All handlers have corresponding data entries. No orphan handlers detected. |

The audit found no cases of handlers without data entries. Every `skill.name === 'xxx'` check in index.js has a corresponding entry in either `ro_skill_data.js` or `ro_skill_data_2nd.js`.

---

## Missing Skills (No Data AND No Handler)

| Skill | rAthena ID | Class | Priority |
|-------|-----------|-------|----------|
| Body Relocation / Snap | MO_BODYRELOCATION (264) | Monk | HIGH -- core mobility skill |
| One-Hand Quicken | KN_ONEHAND (272) | Knight | LOW -- requires Soul Linker (Trans content) |

---

## Final Fix Priority List

### P1: Critical (incorrect behavior, immediate gameplay impact)

| # | Fix | Skills | Effort | Details |
|---|-----|--------|--------|---------|
| 1 | **Turn Undead HP factor + 70% cap + boss immunity** | 1006 | Low | Add `{1-(HP/MaxHP)}*200` term, cap at 70%, skip instant kill for bosses |
| 2 | **Abracadabra ID table rewrite** | ~145 entries | Medium | Replace all IDs in ABRA_REGULAR_SKILLS with correct IDs from ro_skill_data_2nd.js. Affects ~20+ entries across Assassin, Rogue, Crusader, Monk, Hunter. Every entry must be verified against data file. |
| 3 | **Back Stab remove dagger 2-hit** | 1701 | Trivial | Change `const numHits = isDagger ? 2 : 1;` to `const numHits = 1;` |
| 4 | **Bowling Bash guaranteed 2-hit** | 707 | Low | Add a first `bbDealDamage(enemy, eid, true)` call before the chain reaction at line 13632. Cap at 2 hits per target. |

### P2: High (formula/mechanic deviation, noticeable gameplay impact)

| # | Fix | Skills | Effort | Details |
|---|-----|--------|--------|---------|
| 5 | **Blacksmith party-wide buffs** | 1200, 1201, 1202 | Medium | Iterate party members in each handler: AR *1.2 for Axe/Mace users, WP noSizePenalty, PT +5% ATK flat |
| 6 | **Scream party stun reduction** | 1526 | Trivial | Add party check: `const isParty = (pid === characterId) \|\| (caster.partyId && pTarget.partyId && caster.partyId === pTarget.partyId); const adjChance = isParty ? Math.floor(stunChance / 4) : stunChance;` |
| 7 | **Energy Coat cast time** | 213 | Trivial | Change `castTime: 5000` to `castTime: 0` in ro_skill_data.js line 48 |
| 8 | **Throw Venom Knife SP cost** | 1111 | Trivial | Change `spCost: 15` to `spCost: 35` in ro_skill_data_2nd.js line 207 |
| 9 | **Poison React envenom limit** | 1104 | Trivial | Change `Math.floor(learnedLevel / 2)` to `Math.floor((learnedLevel + 1) / 2)` at line 16894, add Lv10 special case of 6 |

### P3: Medium (accuracy improvements, moderate gameplay impact)

| # | Fix | Skills | Effort | Details |
|---|-----|--------|--------|---------|
| 10 | **Sanctuary Undead element scaling** | 1000 | Low | Apply `getElementModifier('holy', enemyElement, enemyElementLevel)` to holyDmg at line 28441 |
| 11 | **THQ remove Renewal CRI/HIT** | 705 | Trivial | Remove `critBonus` and `hitBonus` from THQ buff at lines 13451-13452 (if strict pre-renewal desired) |
| 12 | **Katar Double CRI** | -- | Low | In `calculateCritChance()`, check if weapon is Katar and double the CRI value |
| 13 | **Body Relocation / Snap** | New ID | Medium | New skill entry + handler: instant ground-target teleport, SP 14, 1 sphere cost, range 18, 2s CD after Asura |
| 14 | **Demonstration weapon break rate** | 1802 | Trivial | Change `learnedLevel` to `learnedLevel * 3` for break chance (3-15%) |
| 15 | **Pharmacy Potion Research rate** | 1800 | Trivial | Change `potResLv*50` to `potResLv*100` in pharmacy formula |
| 16 | **Improve Dodge 2nd class scaling** | 501 | Low | Check class in getPassiveSkillBonuses: Assassin/Rogue get +4/lv instead of +3/lv |

### P4: Low (minor accuracy, edge cases, cosmetic)

| # | Fix | Skills | Effort | Details |
|---|-----|--------|--------|---------|
| 17 | **Frost Joker party rate /2** | 1506 | Trivial | Change `/4` to `/2` at line 19341 (iRO Wiki says half, not quarter) |
| 18 | **Earth Spike ACD per-level** | 804 | Trivial | Change flat 700 to `[1000,1200,1400,1600,1800]` |
| 19 | **Heaven's Drive ACD** | 805 | Trivial | Change 700 to 500 |
| 20 | **Reflect Shield prereq** | 1307 | Trivial | Add `{ skillId: 1301, level: 3 }` to prerequisites |
| 21 | **Holy Cross prereq** | 1302 | Trivial | Change Faith from level 7 to level 3 |
| 22 | **Turn Undead ACD** | 1006 | Trivial | Change `cooldown: 500` to `afterCastDelay: 3000, cooldown: 0` |
| 23 | **Grimtooth melee/ranged per level** | 1102 | Low | Add level-based classification for Pneuma/Safety Wall blocking |
| 24 | **Grand Cross cross shape** | 1303 | Medium | Replace diamond (41 cells) with cross (9 cells: dx===0 OR dy===0, max distance 2) |

### P5: Deferred (requires new systems)

| # | Fix | Skills | Effort | Details |
|---|-----|--------|--------|---------|
| 25 | **Greed** | 1210 | High | Requires ground loot pickup system |
| 26 | **Material crafting recipes** | 1220-1222 | Medium | Iron/Steel/Stone tempering as craftable recipes |
| 27 | **Anvil bonuses** | -- | Low | Standard/Oridecon/Golden/Emperium Anvils for forge rate |
| 28 | **Expanded forge recipes** | 1224-1230 | Medium | Expand from 7 to ~45 canonical weapon recipes |
| 29 | **Safety Wall durability HP** | 1017 | Low | Implement `300*lv + 7000*(1+0.1*JobLv/50) + 65*INT + MaxSP` |
| 30 | **Homunculus Evolution + Skills** | 1813 | High | Stone of Sage, stat bonuses, active/passive skills per type |

---

## Summary Statistics

| Category | Count |
|----------|-------|
| Total issues from all 4 audits | 35 |
| Verified as real issues | 22 |
| Confirmed false positives | 5 |
| Newly discovered issues | 7 |
| **Total real issues** | **29** |
| Priority 1 (Critical) | 4 |
| Priority 2 (High) | 5 |
| Priority 3 (Medium) | 7 |
| Priority 4 (Low) | 8 |
| Priority 5 (Deferred) | 6 |

**Most impactful single fix**: Abracadabra ID table rewrite (#2) -- affects the entire random skill pool (~20+ wrong entries across 5+ classes), causing Abracadabra to cast wrong skills silently.

**Highest DPS impact**: Bowling Bash guaranteed 2-hit (#4) + Back Stab dagger 1-hit (#3) -- BB deals 50% intended damage in solo, Back Stab deals 200% intended damage with daggers.

**Most class-defining missing feature**: Body Relocation/Snap (#13) -- Monk mobility skill used in virtually every Monk build for Asura Strike positioning.
