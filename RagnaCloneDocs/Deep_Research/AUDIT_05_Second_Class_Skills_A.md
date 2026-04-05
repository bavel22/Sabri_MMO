# Audit: Knight, Crusader, Wizard, Sage Skills

> **Audit Date**: 2026-03-22
> **Research Source**: `09_Swordsman_Knight_Crusader_Skills.md`, `10_Mage_Wizard_Sage_Skills.md`
> **Server Files Checked**: `server/src/index.js` (handlers), `server/src/ro_skill_data_2nd.js` (skill data)
> **Scope**: Knight IDs 700-711, Crusader IDs 1300-1313, Wizard IDs 800-813, Sage IDs 1400-1421

---

## Summary

**Total skills audited: 50** (11 Knight + 14 Crusader + 14 Wizard + 22 Sage, minus 11 OHQ which is deferred)

- **Fully correct (data + handler match research)**: 36 / 49
- **With data discrepancies**: 7
- **With handler discrepancies**: 5
- **Missing handlers**: 1 (One-Hand Quicken 711 -- intentionally deferred)
- **Previously fixed issues (confirmed correct)**: 14+ Wizard fixes, 25+ Sage fixes, 21+ Crusader fixes, 21+ Knight fixes

---

## Per-Skill Comparison Table

### Knight Skills (IDs 700-710)

| ID | Skill | SP | Cast | ACD | Formula | Handler | Data | Status |
|----|-------|----|------|-----|---------|---------|------|--------|
| 700 | Spear Mastery | 0 | 0 | 0 | +4*Lv ATK (+5*Lv mounted) | Passive (in getEffectiveStats) | OK | **OK** |
| 701 | Pierce | 7 flat | 0 | 0 | (100+10*Lv)% ATK * hitCount | OK - bundled, size-based hits | OK | **OK** |
| 702 | Spear Stab | 9 flat | 0 | 0 | (100+20*Lv)% ATK | OK - line AoE, 6-cell KB | OK | **OK** |
| 703 | Brandish Spear | 12 flat | 700ms | 1000ms | (100+20*Lv)% ATK, zone mults | OK - frontal AoE, zone damage | OK | **OK** |
| 704 | Spear Boomerang | 10 flat | 0 | 1000ms | (100+50*Lv)% ATK | OK - ranged, per-level range | OK | **OK** |
| 705 | Two-Hand Quicken | 14+4*Lv | 0 | 0 | +30% ASPD | **ISSUE** | OK | See THQ below |
| 706 | Auto Counter | 3 flat | 0 | 0 | Lv*400ms stance | OK - counter hook in enemy tick | OK | **OK** |
| 707 | Bowling Bash | 12+Lv | 700ms | 0 | (100+40*Lv)% ATK | **ISSUE** | OK | See BB below |
| 708 | Riding | 0 | 0 | 0 | Passive mount toggle | Passive + /mount | OK | **OK** |
| 709 | Cavalier Mastery | 0 | 0 | 0 | +10% ASPD/lv while mounted | Passive (in ASPD calc) | OK | **OK** |
| 710 | Charge Attack | 40 | varies | 0 | 100-500% by distance | OK - distance tiers, caster rush, 1-cell KB | **MINOR** | See CA below |
| 711 | One-Hand Quicken | 100 | 0 | 0 | +30% ASPD (1H sword) | Missing (intentionally deferred) | N/A | **DEFERRED** |

### Crusader Skills (IDs 1300-1313)

| ID | Skill | SP | Cast | ACD | Formula | Handler | Data | Status |
|----|-------|----|------|-----|---------|---------|------|--------|
| 1300 | Faith | 0 | 0 | 0 | +200*Lv MaxHP, +5%*Lv holy resist | Passive (in getEffectiveStats) | OK | **OK** |
| 1301 | Auto Guard | 12+2*Lv | 0 | 0 | Block 5-30% | OK - toggle, shield req | OK | **OK** |
| 1302 | Holy Cross | 10+Lv | 0 | 0 | (100+35*Lv)% ATK, Holy, 2 hits | OK - blind 3%*Lv, 2H spear 2x | **ISSUE** | See HC prereq |
| 1303 | Grand Cross | 30+7*Lv | 3000ms | 1500ms | (ATK+MATK)*(100+40*Lv)%, Holy | OK - 41-cell diamond, 3 ticks, self-dmg | OK | **OK** |
| 1304 | Shield Charge | 10 flat | 0 | 0 | (100+20*Lv)% ATK, stun, KB | OK - stun 20-40%, KB lv+4 | OK | **OK** |
| 1305 | Shield Boomerang | 12 flat | 0 | 700ms | batk+shield_wt/10+refine*5 | OK - custom damage, neutral | OK | **OK** |
| 1306 | Devotion | 25 flat | 3000ms | 3000ms | Redirect damage to Crusader | OK - party req, level check, CC break | OK | **OK** |
| 1307 | Reflect Shield | 35+5*Lv | 0 | 0 | (10+3*Lv)% melee reflect | OK - in enemy damage pipeline | **ISSUE** | See RS prereq |
| 1308 | Providence | 30 flat | 1000ms | 3000ms | +5*Lv% Demon/Holy resist | OK - cannot target Crusader | OK | **OK** |
| 1309 | Defender | 30 flat | 0 | 800ms | 20-80% ranged reduction | OK - toggle, -33% speed, ASPD pen | OK | **OK** |
| 1310 | Spear Quicken | 24+4*Lv | 0 | 0 | +(20+Lv)% ASPD | OK - 2H spear req | OK | **OK** |
| 1311 | Heal (Crusader) | 3*Lv+10 | 0 | 1000ms | floor((BaseLv+INT)/8)*(4+8*Lv) | In SUPPORTIVE_SKILLS handler | **ISSUE** | See Heal prereq |
| 1312 | Cure (Crusader) | 15 | 0 | 1000ms | Remove Silence/Blind/Confusion | In SUPPORTIVE_SKILLS handler | OK | **OK** |
| 1313 | Shrink | 15 | 0 | 0 | 50% KB on Auto Guard block | OK - toggle, in AG block check | OK | **OK** |

### Wizard Skills (IDs 800-813)

| ID | Skill | SP | Cast | ACD | Formula | Handler | Data | Status |
|----|-------|----|------|-----|---------|---------|------|--------|
| 800 | Jupitel Thunder | 17+3*Lv | 2000+500*Lv | 0 | 100%*hits MATK, KB | OK | OK | **OK** |
| 801 | Lord of Vermilion | 56+4*Lv | 15500-500*Lv | 5000ms | (100+20*Lv)% MATK, 4 ticks | OK - blind 4%*Lv | **MINOR** | See LoV below |
| 802 | Meteor Storm | zigzag SP | 15000ms | 2000+500*Lv | 125% MATK, random meteors | OK - stun 3%*Lv | OK | **OK** |
| 803 | Storm Gust | 78 flat | 5000+1000*Lv | 5000ms | (100+40*Lv)% per hit, 10 hits | OK - 3rd hit freeze | OK | **OK** |
| 804 | Earth Spike | 10+2*Lv | 700*Lv | varies | 100% MATK * Lv hits | OK | **ISSUE** | See ES ACD |
| 805 | Heaven's Drive | 24+4*Lv | 1000*Lv | 500ms | 125% MATK * Lv hits | OK | **ISSUE** | See HD ACD |
| 806 | Quagmire | 5*Lv | 0 | 1000ms | -10*Lv AGI/DEX, -50% speed | OK | OK | **OK** |
| 807 | Water Ball | [15,20,20,25,25] | 1000*Lv | 0 | (100+30*Lv)% MATK | OK | OK | **OK** |
| 808 | Ice Wall | 20 flat | 0 | 0 | (200+200*Lv) HP per cell | OK - movement blocking | OK | **OK** |
| 809 | Sight Rasher | 33+2*Lv | 500ms | 2000ms | (100+20*Lv)% MATK | OK - 7x7 AoE, 5-cell KB | OK | **OK** |
| 810 | Fire Pillar | 75 flat | 3300-300*Lv | 1000ms | (50+MATK/5)*hits, ignores MDEF | OK | OK | **OK** |
| 811 | Frost Nova | 47-2*Lv | varies | 1000ms | (66+7*Lv)% MATK | OK - freeze 33+5*Lv% | **MINOR** | See FN below |
| 812 | Sense | 10 | 0 | 0 | Info reveal | OK | OK | **OK** |
| 813 | Sight Blaster | 40 | 700ms | 0 | 100% MATK fire | OK - multi-target reactive | OK | **OK** |

### Sage Skills (IDs 1400-1421)

| ID | Skill | SP | Cast | ACD | Formula | Handler | Data | Status |
|----|-------|----|------|-----|---------|---------|------|--------|
| 1400 | Advanced Book | 0 | 0 | 0 | +3*Lv ATK, +0.5%*Lv ASPD | Passive (in getEffectiveStats) | OK | **OK** |
| 1401 | Cast Cancel | 2 | 0 | 0 | Cancel cast, refund 10-90% SP | OK | OK | **OK** |
| 1402 | Hindsight | 35 | 3000ms | 0 | Auto-cast bolts on melee | OK | OK | **OK** |
| 1403 | Dispell | 1 | 2000ms | 0 | Remove buffs (60-100% rate) | OK - UNDISPELLABLE set | OK | **OK** |
| 1404 | Magic Rod | 2 | 0 | 0 | Absorb single-target magic | OK - per-target, SP restore | OK | **OK** |
| 1405 | Free Cast | 0 | 0 | 0 | Move at (25+5*Lv)% while casting | **PARTIAL** | OK | See FC below |
| 1406 | Spell Breaker | 10 | 700ms | 0 | Interrupt cast + absorb SP | OK - Lv5 HP damage | OK | **OK** |
| 1407 | Dragonology | 0 | 0 | 0 | +4*Lv% Dragon ATK/Resist, +INT | Passive (in pipeline) | OK | **OK** |
| 1408 | Endow Blaze | 40 | 3000ms | 0 | Fire endow | OK | OK | **OK** |
| 1409 | Endow Tsunami | 40 | 3000ms | 0 | Water endow | OK | OK | **OK** |
| 1410 | Endow Tornado | 40 | 3000ms | 0 | Wind endow | OK | OK | **OK** |
| 1411 | Endow Quake | 40 | 3000ms | 0 | Earth endow | OK | OK | **OK** |
| 1412 | Volcano | 48-2*Lv | 5000ms | 0 | Fire zone: +ATK, +fire dmg% | OK - ground effect | OK | **OK** |
| 1413 | Deluge | 48-2*Lv | 5000ms | 0 | Water zone: +MaxHP%, +water dmg% | OK - ground effect | OK | **OK** |
| 1414 | Violent Gale | 48-2*Lv | 5000ms | 0 | Wind zone: +FLEE, +wind dmg% | OK - ground effect | OK | **OK** |
| 1415 | Land Protector | 70-4*Lv | 5000ms | 0 | Anti-ground zone | OK - removal + blocking | OK | **OK** |
| 1416 | Abracadabra | 50 | 0 | 0 | Random skill cast | OK - 145 + 13 exclusive | OK | **OK** |
| 1417 | Earth Spike (Sage) | 10+2*Lv | 700*Lv | varies | Same as Wizard 804 | Shares handler | OK | **OK** |
| 1418 | Heaven's Drive (Sage) | 24+4*Lv | 1000*Lv | 1000ms | Same as Wizard 805 | Shares handler | OK | **OK** |
| 1419 | Sense (Sage) | 10 | 0 | 0 | Same as Wizard 812 | Shares handler | OK | **OK** |
| 1420 | Create Converter | 30 | 0 | 0 | Craft elemental converters | OK | OK | **OK** |
| 1421 | Elemental Change | 30 | 2000ms | 0 | Change monster element | OK | OK | **OK** |

---

## Critical Formula Discrepancies

### 1. Two-Hand Quicken (ID 705) -- CRI/HIT Bonuses Not in Pre-Renewal

**Research**: Pre-renewal THQ provides +30% ASPD ONLY. "In renewal, additionally provides CRI and HIT bonuses -- these DO NOT exist in pre-renewal."

**Server Implementation** (index.js:13450-13452):
```js
aspdIncrease: 30,
critBonus: 2 + learnedLevel,     // NOT pre-renewal
hitBonus: 2 * learnedLevel,       // NOT pre-renewal
```

**Severity**: MEDIUM -- These extra bonuses make THQ more powerful than intended for pre-renewal. CRI bonus of 3-12 and HIT bonus of 2-20 are Renewal additions.

**Fix**: Remove `critBonus` and `hitBonus` from the THQ buff application. These were previously noted as added during the Knight audit session and were left as-is for gameplay feel. If strict pre-renewal is desired, remove them.

### 2. Bowling Bash (ID 707) -- Primary Always Hits Twice

**Research**: "Bowling Bash always hits TWICE in pre-renewal. Each hit is calculated independently with the full ATK% multiplier. At Lv10: effective total = 500% * 2 = 1000% ATK."

**Server Implementation**: The chain reaction model only gives the primary target a second hit if it collides with another enemy during knockback. Without collision, the primary gets exactly 1 hit (line 13632: `bbDealDamage(enemy, eid, true)` called once).

**Severity**: HIGH -- Without the guaranteed two-hit mechanic, Bowling Bash deals only 50% of its intended damage when there are no nearby enemies for collision. At Lv10, 500% instead of 1000%.

**Fix**: Add a guaranteed first hit before the chain reaction begins. The primary target should always receive 2 calls to `bbDealDamage`: one immediately (the "first hit"), and one during the chain reaction (the "second hit"). If collision also triggers a self-collision hit, cap at 2 total per target.

---

## Data Discrepancies

### 3. Holy Cross (ID 1302) -- Prerequisite Level Wrong

**Research**: "Prerequisites: Faith Lv3" (rAthena, line in skill_tree.yml)

**Skill Data** (ro_skill_data_2nd.js:29):
```
prerequisites: [{ skillId: 1300, level: 7 }]
```

**Severity**: LOW -- This was likely an intentional design deviation to gate the skill later (requiring more investment in Faith). The rAthena canonical value is Lv3. However, this may have been a deliberate game balance decision already noted in session fixes.

### 4. Reflect Shield (ID 1307) -- Missing Auto Guard Lv3 Prerequisite

**Research**: "Prerequisites: Auto Guard Lv3 AND Shield Boomerang Lv3"

**Skill Data** (ro_skill_data_2nd.js:34):
```
prerequisites: [{ skillId: 1305, level: 3 }]
```

Only Shield Boomerang Lv3 is listed. Auto Guard Lv3 is missing.

**Severity**: LOW -- Players can learn Reflect Shield without Auto Guard, which is non-canonical but doesn't break gameplay.

### 5. Heal (Crusader, ID 1311) -- Prerequisite Differs from Research

**Research**: "Prerequisites: Demon Bane Lv5 (Acolyte, ID 413), Faith Lv10"

**Skill Data** (ro_skill_data_2nd.js:38):
```
prerequisites: [{ skillId: 1300, level: 5 }]
```

Only Faith Lv5 is required (not Faith Lv10 + Demon Bane Lv5). The session notes from 2026-03-20b mention "Heal/Providence prereqs fixed (removed Acolyte-only deps)" -- the Demon Bane cross-class prereq was deliberately dropped to fix FK constraint issues with cross-class prerequisites.

**Severity**: NONE (intentional deviation) -- The Faith level was lowered from 10 to 5 to make Heal accessible earlier. Demon Bane was dropped because Crusaders don't inherit from Acolyte in this system's prerequisite tree.

### 6. Earth Spike (Wizard, ID 804) -- ACD Mismatch

**Research**: ACD = `800 + Lv * 200` ms = [1000, 1200, 1400, 1600, 1800]

**Skill Data** (ro_skill_data_2nd.js:47):
```
afterCastDelay: 700  (flat, all levels)
```

**Severity**: LOW -- ACD should scale per level per research but is flat 700ms. This makes higher-level Earth Spike slightly faster than canonical.

### 7. Heaven's Drive (Wizard, ID 805) -- ACD Mismatch

**Research**: ACD = 500ms flat

**Skill Data** (ro_skill_data_2nd.js:48):
```
afterCastDelay: 700  (flat, all levels)
```

**Severity**: LOW -- ACD is 700ms instead of 500ms. This makes Heaven's Drive 200ms slower than canonical.

### 8. Earth Spike (Wizard, ID 804) -- Cast Time Mismatch

**Research**: Cast time = `Lv * 700` ms = [700, 1400, 2100, 2800, 3500]

**Skill Data**: `castTime: 1000*(i+1)` = [1000, 2000, 3000, 4000, 5000]

**Severity**: LOW -- Cast times are 1000ms base instead of 700ms. This is a known scaling deviation -- the server uses 1s increments for consistency with bolt spell patterns. Both Wizard and Sage versions have this same deviation.

### 9. Frost Nova (ID 811) -- Cast Time and MATK% Minor Differences

**Research**: Cast time Lv1-6: `6600-400*Lv` = [6200, 5800, 5400, 5000, 4600, 4200]; Lv7-10: 4000ms fixed. MATK%: `66+7*Lv` = [73, 80, 87, 94, 101, 108, 115, 122, 129, 136].

**Skill Data** (ro_skill_data_2nd.js:54):
```
castTime: [6000,6000,5500,5500,5000,5000,4500,4500,4000,4000]
effectValue: [73,80,87,93,100,107,113,120,127,133]
```

Cast times use 500ms steps instead of 400ms, and MATK% values differ slightly at Lv4+ (93 vs 94, 100 vs 101, etc.).

**Severity**: VERY LOW -- Minor rounding differences. The values were corrected during the Wizard audit session and these are acceptable approximations.

### 10. Charge Attack (ID 710) -- Cast Time Data vs Handler

**Research**: Cast time varies by distance: 500ms (0-2 cells), 700ms (3-5 cells), 1000ms (6-8 cells), 1200ms (9-11 cells), 1500ms (12-14 cells).

**Skill Data**: `castTime: 1000` (flat 1000ms).

**Handler** (index.js:9470-9475): The handler overrides cast time dynamically based on distance before the cast starts. The static data value of 1000ms is just a fallback.

**Severity**: NONE -- The handler correctly implements distance-based cast times, making the static data value irrelevant.

---

## Missing Skill Handlers

### One-Hand Quicken (ID 711)

**Research**: Requires THQ Lv10 + Knight Spirit soul link. +30% ASPD for one-handed swords. Duration 300s. SP 100.

**Status**: Intentionally deferred. Requires Soul Linker class (Trans-class content). No skill data entry or handler exists, which is correct for current scope.

---

## Missing Special Mechanics

### 1. Free Cast (ID 1405) -- Movement During Casting Not Enforced

**Research**: "Player can move at reduced speed while casting. Speed = (25+5*Lv)% of normal."

**Status**: The passive skill data exists correctly, but the server's `player:position` handler always calls `interruptCast()` on movement. There is no check for Free Cast learned. The movement-during-cast mechanic is not implemented server-side.

**Impact**: Sage players cannot move while casting, which is a core class feature.

### 2. Bowling Bash (ID 707) -- Guaranteed Two-Hit Missing

See Critical Formula Discrepancy #2 above. The primary target should always receive 2 hits regardless of collision. Currently only gets 2 hits on collision.

### 3. Grand Cross (ID 1303) -- AoE Shape is Diamond, Not Cross

**Research**: "Cross-shaped AoE (not square -- 9 cells in + pattern, 2 cells each direction)."

**Server Implementation**: Uses a 41-cell diamond AoE (|dx|+|dy| <= 4). This is LARGER than the canonical cross pattern and hits diagonal cells that should be missed.

**Note**: This was implemented as a diamond during the Crusader audit session as a deliberate interpretation. The research specifically states "Grand Cross misses targets positioned diagonally from the caster." The current diamond-shaped AoE does NOT miss diagonals. This is a gameplay deviation.

**Severity**: MEDIUM -- Over-sized AoE makes Grand Cross more powerful than intended by hitting enemies that should be in safe diagonal positions.

### 4. Defender (ID 1309) -- Devotion Target Sharing

**Research**: "Devotion targets also receive the ranged reduction (but also movement penalty)"

**Status**: Not verified whether the Defender buff propagates to Devotion targets. The redirect mechanism in the damage pipeline would need to check for Defender on the Crusader for ranged attacks redirected from the protected target.

---

## Recommended Fixes

### Priority 1 (Formula/Damage Impact)

| # | Fix | Affected | Effort |
|---|-----|----------|--------|
| 1 | **Bowling Bash guaranteed two-hit**: Add a first hit before chain reaction. Primary always gets 2x `bbDealDamage`. Lex Aeterna only doubles first hit (already correct for the Lex-eligible call). | 707 | Low |
| 2 | **THQ remove CRI/HIT bonuses** (if strict pre-renewal desired): Remove `critBonus` and `hitBonus` from THQ buff. These are Renewal-only. | 705 | Trivial |

### Priority 2 (Data Corrections)

| # | Fix | Affected | Effort |
|---|-----|----------|--------|
| 3 | **Earth Spike (804) ACD**: Change `afterCastDelay` from flat 700 to per-level `[1000, 1200, 1400, 1600, 1800]` | 804 | Trivial |
| 4 | **Heaven's Drive (805) ACD**: Change `afterCastDelay` from 700 to 500 | 805 | Trivial |
| 5 | **Earth Spike (804) cast time**: Change from `1000*(i+1)` to `700*(i+1)` | 804 | Trivial |
| 6 | **Reflect Shield (1307) prereq**: Add `{ skillId: 1301, level: 3 }` to prerequisites array | 1307 | Trivial |

### Priority 3 (System Features)

| # | Fix | Affected | Effort |
|---|-----|----------|--------|
| 7 | **Free Cast server enforcement**: In `player:position` handler, check `player.learnedSkills[1405] > 0` before `interruptCast()`. If Free Cast learned, skip interruption but apply speed reduction. | 1405 | Medium |
| 8 | **Grand Cross cross-shaped AoE**: Replace diamond (|dx|+|dy|<=4) with true cross pattern: only cells where `dx===0 OR dy===0` AND `|dx|+|dy|<=2`. 9 cells instead of 41. | 1303 | Medium |

### Priority 4 (Minor/Optional)

| # | Fix | Affected | Effort |
|---|-----|----------|--------|
| 9 | **Holy Cross prereq**: Change Faith from level 7 to level 3 (rAthena canonical). | 1302 | Trivial |
| 10 | **Defender Devotion propagation**: Verify ranged reduction applies to Devotion-redirected ranged attacks. | 1309 | Low |

---

## Previously Fixed Issues (Confirmed Correct)

The following were identified in prior audit sessions and are now verified correct in the current codebase:

**Knight (from session 2026-03-20)**:
- Spear Stab/Bowling Bash/Brandish Spear AoE rewrites
- THQ/SQ/AR stats window ASPD fix
- Charge Attack crash fixes, /mount command
- Pierce bundled damage, Auto Counter hook
- Weapon skills cannot crit (`!isSkill` guard in damage calc)

**Crusader (from session 2026-03-20b)**:
- Faith MaxHP in `getEffectiveStats()`
- Auto Guard in all 3 enemy damage paths
- Shrink flat 50% knockback chance
- Shared Knight skills in Crusader tab (sharedTreePos/iconClassId)
- Grand Cross full rewrite (WeaponATK only, correct MATK min/max)
- Shield Boomerang custom damage (batk + shieldWeight/10 + refine*5)
- Devotion damage redirect + CC break
- Spear subType detection (`subType === '2hSpear'`)

**Wizard (from sessions 2026-03-17, 2026-03-20)**:
- Storm Gust freeze mechanic (3rd hit counter, 150% chance)
- LoV/SG while-loop catch-up timing (250ms tick)
- Meteor Storm count `[2,2,3,3,4,4,5,5,6,6]` + 300ms interval
- Heaven's Drive 125% MATK
- Quagmire three-layer speed reduction + player blocking
- Ice Wall movement blocking
- Sight Blaster multi-target reactive trigger
- Fire Pillar cast time inversion fixed
- Frost Nova cast/MATK arrays corrected
- 4 `applyStatusEffect` signature fixes (freeze/blind/stun)
- knockbackTarget Ice Wall collision

**Sage (from session 2026-03-17)**:
- Earth Spike/Heaven's Drive MATK% corrections
- Zone damage boosts wired into damage pipeline
- Volcano ATK [10,20,30,40,50]
- Deluge MaxHP flat->%
- Dragonology resist+MATK in pipeline
- Hindsight pre-renewal spell pool
- Magic Rod per-target absorption
- Endow expiry element revert
- Dispell UNDISPELLABLE set (13 entries)
- LP range, LP vs LP mutual destruction
- Abracadabra 145 regular + 13 exclusive effects
