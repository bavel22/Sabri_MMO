# Deferred Systems Comprehensive Remediation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Status_Effect_Buff_System](../03_Server_Side/Status_Effect_Buff_System.md)
> **Status**: COMPLETED — All 37 systems implemented + 38 audit issues fixed + v3 formula fixes (10/10 phases)

**Date**: 2026-03-17
**Implementation Date**: 2026-03-18
**Scope**: All 37 confirmed deferred/stubbed/dead-code systems
**Reference**: rAthena pre-renewal source, iRO Wiki Classic, RateMyServer pre-re, divine-pride
**Goal**: 100% RO Classic pre-renewal accuracy — zero gaps
**Status**: **ALL 10 PHASES COMPLETE + ALL 38 AUDIT ISSUES FIXED** — Phases A through J implemented. Post-implementation audit found 38 issues (5 critical, 6 high, 12 medium, 15 low), all resolved on 2026-03-18.

---

## Second-Pass Errata (verified 2026-03-17)

All values in this plan have been **double-checked** against rAthena pre-renewal source code (`skill.cpp`, `status.cpp`, `skill_db.yml`, `abra_db.yml`, `homunculus_db.yml`), iRO Wiki Classic, RateMyServer pre-re, and divine-pride. The following corrections were applied from the second verification pass:

| Section | Original Value | Corrected Value | Why |
|---------|---------------|----------------|-----|
| Magic Rod ACD | Implied 1.0s | **0ms** (pre-renewal) | rAthena `skill_db.yml` shows 0 ACD/cooldown for pre-re |
| Magic Rod movement | "Moving cancels" | **Does NOT cancel** | No RemoveOnWalk flag in `status.yml` |
| Drum SP cost | `35+3*Lv` (38-50) | **`35+5*Lv` (40-60)** | rAthena pre-re skill_db.yml authoritative |
| Nibelungen ATK type | "DEF-ignoring" | **Flat WATK bonus** | rAthena `status_calc_watk` — added to WATK, not armor-piercing |
| MWM AoE | 7×7 | **9×9** (Layout=4) | rAthena skill_db; 7×7 was the SplashArea for knockback |
| Siegfried elements | "non-neutral" (vague) | **All 9 non-Neutral** | rAthena `#ifndef RENEWAL` block covers all 9 |
| Abracadabra pool | 131 + 13 specials | **145 + 11 specials** | Full count from `abra_db.yml`; SA_FORTUNE/LEVELUP NOT in pool |
| SA_COMA target | "Caster HP/SP → 1" | **CASTER** (TargetType: Self) | Confirmed affects SELF, not target |
| SA_DEATH EXP | Not mentioned | **NO EXP/drops** (status_kill) | Bypasses normal death rewards |
| SA_INSTANTDEATH | "Kill caster" | **With EXP loss** (normal death penalty) | iRO Wiki Classic confirms |
| Elemental Change rate | 100% | **Variable** (5-45% base + stats) | iRO Wiki Classic formula; rAthena 100% is technical, not official |
| Elemental Change duration | 30 minutes | **Permanent** | iRO Wiki Classic says "Permanent" |
| Elemental Change level | Random 1-4 | **Level 1 always** | Consistent with other element-change effects |
| Redemptio SP after cast | SP=0 | **SP=1** | iRO Wiki: "health and mana reduced to 1 each" |
| Homunculus FLEE | `Lv+AGI+floor(Lv/10)` | **`Lv+AGI`** | rAthena status.cpp — no extra term |
| Homunculus Hard DEF | Complex wrong formula | **`VIT+floor(Lv/2)`** | rAthena status.cpp |
| Homunculus ATK | `floor((STR+DEX+LUK)/3)` | **`2*Lv+STR`** (base) | rAthena status.cpp |
| Homunculus ASPD | Fixed 130 | **Dynamic** from AGI/DEX | rAthena: `(1000-4*AGI-DEX)*700/1000` |
| Lif baseSTR | 12 | **17** | rAthena pre-re homunculus_db.yml |
| Moonlight SP | [10,14,18,22,26] | **[4,8,12,16,20]** | `4*skillLv` |
| Moonlight damage | 120-560% | **220-660%** | iRO Wiki Classic + divine-pride |
| Caprice hits | 1 hit | **1/2/3/4/5 hits** | rAthena skill_db Hit count field |
| Bulwark VIT | "+10-30" | **"+2 to +10"** (pre-re: 2*lv) | rAthena commit ffead57 |
| Urgent Escape SP | [20,20,20,20,20] | **[20,25,30,35,40]** | `15+5*level` |
| Weapon Repair cast | 5000ms (in skill_data) | **7500ms** | rAthena pre-re skill_db |
| NPC_SUMMONSLAVE EXP | "Always give EXP" | **Only when killed by players** (not on master death) | iRO Wiki + rAthena |
| Ensemble undispellable | Not flagged | **Add to UNDISPELLABLE**: Eternal Chaos, Loki's Veil, Into the Abyss | RMS + iRO Wiki |
| Intimacy reset on evo | ~400 (Hate) | **10** (display scale) | rAthena `homunculus_evo_intimacy_reset = 1000` internal |
| 4th skill unlock | "re-reaching Loyal" | **At 910 intimacy** post-evolution | rAthena homunculus_db.yml |

---

## Executive Summary

After exhaustive code audit + deep research against 6 authoritative sources, **37 items** are confirmed deferred across 10 phases. This plan provides exact RO Classic specifications for every system, organized by implementation dependency.

**Verified FALSE from initial report (no fix needed):**
- Marine Sphere HP — current `2000 + 400 * lv` matches rAthena `status.cpp`
- CP UNDISPELLABLE — all 8 entries already in set
- Potion Pitcher VIT formula — correct pre-renewal `(100+VIT)/100`
- Plagiarism call sites + DB — working in PvE via `executeMonsterPlayerSkill()`
- Ensemble prereqs — fixed in Bard audit
- Devotion redirect — implemented in enemy auto-attack pipeline
- Sight Blaster proximity — fully implemented with trigger + knockback
- Remove Trap Rogue (1708) — removes any player's trap
- Summon Flora (1803) — fully implemented with auto-attack tick
- Summon Marine Sphere (1807) — fully implemented with detonation
- Scribble (1717) — fully implemented with ground text

---

## Phase A: Dead Code Wiring (~3 hours)

Wire two existing functions that are defined but never called.

### A1. Magic Rod Absorption — Wire into Magic Damage Paths

**Current state**: `checkMagicRodAbsorption()` at line ~1417 is complete but has zero call sites.

**RO Classic spec (rAthena `skill.cpp` + `status.cpp`, verified 2nd pass):**
- Absorbs **single-target magic ONLY** (`src == dsrc` check — direct targeting only)
- SP recovered = `floor(attacker_skill_SP_cost × (skillLv × 20) / 100)`
- **Duration window buff** (SC_MAGICROD): `0.2 + (skillLv × 0.2)` seconds (0.4s at Lv1, 1.2s at Lv5)
- Can absorb **MULTIPLE spells** during window — NO `status_change_end` after absorption (confirmed in rAthena source)
- Negates 100% damage, displays as "miss"
- Water Ball: SP per ball divided by `(skillLv|1)²` — at Lv2-3: ÷9, Lv4-5: ÷25
- **Movement does NOT cancel it** — no RemoveOnWalk flag in `status.yml`
- **Pre-renewal has NO after-cast delay (0ms) and NO cooldown (0ms)** — plan originally said 1.0s ACD which was wrong
- SP Cost: 2 (all levels)
- **Spell Breaker counter**: absorbs 20% of enemy Max SP (special case, already implemented)
- **Note**: Existing skill_data has `cooldown: 500` — must be changed to **0** for pre-renewal

**What to absorb (single-target magic):**
- Fire/Cold/Lightning/Earth Bolt, Soul Strike, Holy Light, Jupitel Thunder
- Napalm Beat (treated as targeted despite minor splash)
- Frost Diver, Stone Curse (targeted magic)
- Water Ball (each hit separately)

**What NOT to absorb (AoE/ground):**
- Storm Gust, Lord of Vermilion, Meteor Storm, Heaven's Drive, Thunderstorm
- Fire Pillar, Fire Wall, Magnus Exorcismus, Sanctuary, Quagmire

**Implementation**: Add `checkMagicRodAbsorption(target, targetId, spCost, zone)` call at the top of every single-target magic skill handler BEFORE damage calculation. If it returns `true`, skip damage entirely and return. Must pass the skill's SP cost as `spellSpCost`.

**Call sites to add** (in `skill:use` / `targetCast` handlers):
1. Fire Bolt (200) — single-target bolt
2. Cold Bolt (201) — single-target bolt
3. Lightning Bolt (202) — single-target bolt
4. Earth Spike (209 + sage 1405) — single-target bolt
5. Napalm Beat (203) — targeted
6. Soul Strike (205) — targeted
7. Frost Diver (206) — targeted
8. Stone Curse (207) — targeted
9. Jupitel Thunder (807) — targeted
10. Holy Light (412) — targeted
11. Water Ball (808) — each hit check

**Verification**: Cast Magic Rod Lv5 → have enemy cast Fire Bolt → damage should be 0, SP gained = 100% of Fire Bolt's SP cost.

---

### A2. Magic Reflection (Maya Card) — Wire into Magic Damage Paths

**Current state**: `processCardMagicReflection()` at line ~2584 is complete but has zero call sites.

**RO Classic spec (rAthena `battle.cpp`):**
- Maya Card: `bMagicDamageReturn 50` — 50% chance per targeted spell
- **Bypasses boss check** (unlike Kaite)
- Reflected damage is recalculated with swapped src/target
- Only reflects **single-target magic** (same list as Magic Rod above)
- Other sources: Orleans's Server 5%, Cat O' Nine Tails Card 5%, Frus Card 2%/refine

**Implementation**: After the Magic Rod check (A1), if Magic Rod did NOT absorb, check `processCardMagicReflection()`. If reflection triggers:
1. Roll `rnd() < target.cardMagicDamageReturn` (percentage check)
2. Calculate reflected damage using the original spell's MATK% but applying the **original caster's own MDEF** as defense
3. Apply reflected damage to the original caster
4. Negate damage to the target
5. Emit `combat:magic_reflected` event

**Call sites**: Same 11 single-target magic handlers as A1, placed after the Magic Rod check.

**Priority order**: Magic Rod absorb → Magic Reflection → Normal damage

---

## Phase B: Ensemble System (~15 hours)

### B0. Data Corrections in `ro_skill_data_2nd.js`

Fix these values BEFORE implementing ensemble logic:

| Skill | Field | Current | Correct | Source |
|-------|-------|---------|---------|--------|
| Mr. Kim (1531/1551) | spCost | `20+i*4` (20-36) | **20 flat** all levels | RateMyServer pre-re |
| Mr. Kim (1531/1551) | effectValue | `i+1` (1-5) | **`125+11*(i+1)`** (136/147/158/169/180) — EXP bonus % | iRO Wiki Classic |
| Drum (1533/1553) | spCost | `38+i*2` (38-46) | **`35+5*(i+1)`** (40/45/50/55/60) | rAthena pre-re skill_db + iRO Wiki Classic |
| Drum (1533/1553) | effectValue | `5+i*5` (10-30) | **atkBonus: `25+25*(i+1)`** (50-150), **defBonus: `2+2*(i+1)`** (4-12) | rAthena `skill_unitsetting` |
| Nibelungen (1534/1554) | spCost | `38+i*2` (38-46) | **`35+3*(i+1)`** (38/41/44/47/50) | rAthena pre-re skill_db + iRO Wiki Classic |
| Nibelungen (1534/1554) | effectValue | `15+i*15` (30-90) | **`50+25*(i+1)`** (75/100/125/150/175) — flat WATK bonus (NOT DEF-ignoring per rAthena `status_calc_watk`) | rAthena source code |
| Siegfried (1537/1557) | spCost | `20+i*5` (20-40) | **20 flat** all levels | RateMyServer pre-re |
| MWM (1540) | isPerformance | `true` | **`isEnsemble: true`** (remove isPerformance) | All sources |
| MWM (1540) | classId | `'dancer'` | Needs **Bard counterpart** (new ID, e.g. 1538) | All sources |
| MWM (1540) | prerequisites | `1523 Lv1` | **Bard**: IC Lv5 + Music Lessons Lv7; **Dancer**: IC Lv5 + Dance Lessons Lv7 | iRO Wiki |

### B1. Ensemble System Core

**General ensemble mechanics (all 8+1 ensembles):**

1. **Activation**: Bard and Dancer must be in same party AND adjacent (≤3 cells)
2. **Skill level used**: `min(bardSkillLv, dancerSkillLv)`
3. **AoE**: 9×9 cells (all ensembles including MWM per rAthena Layout=4) — centered on midpoint between performers
4. **Stationary**: Zone does NOT move once cast (unlike solo songs/dances)
5. **SP drain**: Both performers drain continuously (see ENSEMBLE_SP_DRAIN map below)
6. **Movement**: Both performers are immobilized during ensemble
7. **Aftermath**: 10s debuff period after ensemble ends (reduced ASPD + speed, no skills)
8. **Cannot overlap**: New ensemble cancels previous
9. **Weapon req**: Bard needs Instrument, Dancer needs Whip
10. **Dispel immunity**: Ensembles cannot be canceled by Dispel
11. **UNDISPELLABLE entries needed**: Add `eternal_chaos`, `lokis_veil`, `into_the_abyss` to the UNDISPELLABLE set in index.js

**New constants needed:**
```js
const ENSEMBLE_SP_DRAIN = {
    'lullaby': 4000,           // 1 SP per 4 sec
    'mr_kim_a_rich_man': 3000, // 1 SP per 3 sec
    'eternal_chaos': 4000,
    'drum_on_battlefield': 5000,
    'ring_of_nibelungen': 3000,
    'lokis_veil': 4000,
    'into_the_abyss': 5000,
    'invulnerable_siegfried': 3000,
    'moonlit_water_mill': 10000, // from SONG_SP_DRAIN (move here)
};
```

### B2. Individual Ensemble Effects

#### Lullaby (1530/1550) — Sleep AoE
- **Effect**: Attempts to inflict Sleep on all enemies in 9×9 every 6 seconds
- **Sleep duration**: 30 seconds per application
- **Sleep chance**: Based on performers' combined INT, vs target status resistance
- **Boss immunity**: Yes — bosses immune
- **Duration**: 60 seconds

#### Mr. Kim A Rich Man (1531/1551) — EXP Bonus
- **Effect**: Monsters killed by party members in 9×9 give bonus EXP
- **EXP bonus**: `25 + 11 × skillLv`% bonus (+36%/+47%/+58%/+69%/+80% — total multiplier 136%/147%/158%/169%/180%)
- **rAthena formula**: `val1 = 25 + 11 * skill_lv` — store as BONUS percentage, apply as `expGained * (100 + bonus) / 100`
- **Restriction**: Does NOT increase EXP from Boss monsters
- **Duration**: 60 seconds

#### Eternal Chaos (1532/1552) — VIT DEF Reduction
- **Effect**: Sets VIT-based soft DEF to 0 for ALL enemies in 9×9
- **Does NOT affect**: Equipment/armor hard DEF
- **Pre-renewal**: Works in ALL maps (PvP restriction is renewal-only)
- **Duration**: 60 seconds

#### Drum of Battlefield (1533/1553) — ATK + DEF Buff
- **Effect**: Party members in 9×9 get ATK and DEF bonus
- **ATK bonus**: `25 + 25 × skillLv` (50/75/100/125/150)
- **DEF bonus**: `2 + 2 × skillLv` (4/6/8/10/12)
- **Duration**: 60 seconds

#### Ring of Nibelungen (1534/1554) — Lv4 Weapon ATK
- **Effect**: Party members wielding **Level 4 weapons** get flat WATK bonus
- **ATK bonus**: `50 + 25 × skillLv` (75/100/125/150/175) — flat WATK (added via `status_calc_watk` in rAthena, NOT DEF-ignoring despite iRO Wiki "armor-piercing" description)
- **Lv4 weapon requirement**: Players with Lv1-3 weapons get NO benefit
- **Duration**: 60 seconds

#### Loki's Veil (1535/1555) — Skill Block Zone
- **Effect**: Blocks ALL skill usage for everything in 9×9
- **Includes**: Offensive, supportive, healing, item skills — even caster's own party
- **Exceptions**: Boss monsters immune, Longing for Freedom (transcendent) bypasses
- **Duration**: 60 seconds

#### Into the Abyss (1536/1556) — Catalyst Removal
- **Effect**: Party members in 9×9 do not consume gemstones/catalysts/trap items when casting
- **Exceptions**: Abracadabra still requires 1 Yellow Gemstone minimum, Ganbantein still requires both
- **Duration**: 60 seconds

#### Invulnerable Siegfried (1537/1557) — Element + Status Resist
- **Element resist**: `55 + 5 × skillLv` (60%/65%/70%/75%/80%) — applies to **ALL 9 non-Neutral elements** in pre-renewal (Water, Earth, Fire, Wind, Poison, Holy, Dark, Ghost, Undead) per rAthena `#ifndef RENEWAL` block
- **Status resist**: `10 × skillLv` (10%/20%/30%/40%/50%) — Blind, Frozen, Petrified, Stun, Curse, Sleep, Silence
- **Duration**: 60 seconds

#### Moonlit Water Mill (1538+1540 or new IDs) — Movement Barrier
- **Effect**: Creates barrier zone — monsters/players **cannot enter**
- **Characters inside when cast**: Pushed out (2-cell knockback per rAthena `Knockback: 2`)
- **AoE**: 9×9 cells (rAthena Layout=4 — the 7×7 from RateMyServer was the knockback SplashArea, not the ground unit size)
- **Does NOT block**: Magic attacks, ranged physical attacks, skills — only physical entry
- **Duration**: `15 + 5 × skillLv` seconds (20/25/30/35/40)
- **SP cost**: `20 + 10 × skillLv` (30/40/50/60/70)
- **SP drain**: `4 + skillLv` SP per 10 seconds (5/6/7/8/9) per iRO Wiki
- **Blocked in**: GvG, Battlegrounds
- **Cannot be cast adjacent to walls** (rAthena checks obstacles in range+1)

---

## Phase C: Consumable Item Effects (~6 hours)

### C1. `sc_start` Buff Potion System

Replace the stub `case 'sc_start'` in `inventory:use` with a full handler.

**ASPD Potions (attack delay reduction):**

| Potion | Item ID | Status | Delay Reduction | Duration | Level Req | Class Restrictions |
|--------|---------|--------|----------------|----------|-----------|-------------------|
| Concentration | 645 | SC_ASPDPOTION0 | 10% | 30 min | None | All classes |
| Awakening | 656 | SC_ASPDPOTION1 | 15% | 30 min | Lv 40 | NOT Acolyte, Priest, Bard, Dancer |
| Berserk | 657 | SC_ASPDPOTION2 | 20% | 30 min | Lv 85 | Swordman/Mage/Merchant/Knight/Wizard/BS/Crusader/Rogue/Alchemist only |

**ASPD formula integration**: These are Speed Modifiers (SM) in `calculateASPD()`:
```
ASPD = 200 - (WD - ([WD*AGI/25] + [WD*DEX/100]) / 10) × (1 - SM)
SM = potion_reduction + skill_reductions + equipment_reductions   (additive)
```

**Stacking rules (verified in rAthena `status_calc_aspd_rate()`):**
- **Within potions**: Only the strongest applies (priority: Berserk > Awakening > Concentration)
- **Between potions and skills**: They stack **additively** — both subtract from `aspd_rate` independently
- Example: THQ (30% reduction) + Berserk Potion (20% reduction) = 50% total delay reduction

**rAthena integration verified**: `aspd_rate` starts at 1000 (100%), potions subtract `val2` (100/150/200), skills subtract their own `val2` independently. Both apply via `amotion = baseAmotion * aspd_rate / 1000`.

**In your existing formula**: `SM = potionSM + skillSM` where `(1 - SM)` = `aspd_rate / 1000`. This is correct.

**Death behavior**: ASPD potions are **cleared on death** (no `NoRemoveOnDead` flag). Persist through zone changes.
**Dispel**: All 3 potions are **dispellable** (no `NoDispell` flag).

**Implementation:**
1. On `sc_start` item use, check level requirement and class restriction
2. Apply buff to player: `aspdPotionReduction: 0.10/0.15/0.20` (decimal, maps to SM)
3. Store as active buff with duration (1800000ms = 30 min)
4. In `calculateASPD()`, add `player.aspdPotionReduction` to the speed modifier alongside existing skill SMs
5. Stronger potion replaces weaker (overwrite if new reduction > existing)
6. On death: clear the potion buff (add to `clearBuffsOnDeath`)
7. Emit `buffs:update` with potion buff icon + duration

**Other `sc_start` consumables to implement:**

| Item | ID | Effect | Duration |
|------|-----|--------|----------|
| Panacea | 525 | Cure Poison/Silence/Blind/Confusion/Curse/Hallucination | Instant |
| Holy Water | 523 | Cure Curse | Instant |
| Authoritative Badge | 662 | +25% move speed | 3 min |
| Speed Up Potion | 12016 | +50% move speed | 5 sec |
| Fireproof Potion | 12118 | +20% Fire resist, −15% Water resist | 20 min |
| Coldproof Potion | 12119 | +20% Water resist, −15% Wind resist | 20 min |
| Earthproof Potion | 12120 | +20% Earth resist, −15% Fire resist | 20 min |
| Thunderproof Potion | 12121 | +20% Wind resist, −15% Earth resist | 20 min |

**Stat Food consumables** (IDs 12041-12100):
- Apply `+N` to corresponding stat for 20 minutes
- Only one food per stat type active at a time (new replaces old)
- Bonus values: +1 to +10 depending on item tier

### C2. `itemskill` Scroll System

Replace the stub `case 'itemskill'` handler.

**How scrolls work:**
1. Item use triggers `itemskill` effect with skill name + level
2. Scroll is consumed
3. Skill is cast using **player's own stats** (MATK, INT, DEX, etc.)
4. **No SP cost** from scroll
5. **No class restriction** — any class can use any scroll
6. Normal cast time applies (affected by DEX)

**Scroll inventory (pre-renewal):**

| Item ID | Name | Skill | Level |
|---------|------|-------|-------|
| 686 | Earth Spike Scroll | WZ_EARTHSPIKE | 3 |
| 687 | Earth Spike Scroll | WZ_EARTHSPIKE | 5 |
| 688 | Cold Bolt Scroll | MG_COLDBOLT | 3 |
| 689 | Cold Bolt Scroll | MG_COLDBOLT | 5 |
| 690 | Fire Bolt Scroll | MG_FIREBOLT | 3 |
| 691 | Fire Bolt Scroll | MG_FIREBOLT | 5 |
| 692 | Lightning Bolt Scroll | MG_LIGHTNINGBOLT | 3 |
| 693 | Lightning Bolt Scroll | MG_LIGHTNINGBOLT | 5 |
| 694 | Soul Strike Scroll | MG_SOULSTRIKE | 3 |
| 695 | Soul Strike Scroll | MG_SOULSTRIKE | 5 |
| 696 | Fire Ball Scroll | MG_FIREBALL | 1 |
| 697 | Fire Ball Scroll | MG_FIREBALL | 5 |
| 698 | Fire Wall Scroll | MG_FIREWALL | 1 |
| 699 | Fire Wall Scroll | MG_FIREWALL | 5 |
| 700 | Frost Diver Scroll | MG_FROSTDIVER | 1 |

**Implementation**: When `itemskill` effect fires (excluding already-handled Magnifier/Fly Wing/Butterfly Wing):
1. Look up the skill by name from `ro_skill_data` / `ro_skill_data_2nd`
2. Set `player.pendingScrollSkill = { skillId, level }`
3. Emit `skill:scroll_ready` to client with skill info for targeting
4. On `skill:use` with scroll flag, execute the skill using player's stats at the given level
5. Skip SP cost check (scroll provides it)
6. Skip class restriction check (any class can use)

### C3. Elemental Converter `itemskill` (ITEM_ENCHANTARMS)

Converters (12114-12117) use `itemskill ITEM_ENCHANTARMS,N` to endow weapon:
- Fire Converter (12114): Fire endow, 20 min
- Water Converter (12115): Water endow, 20 min
- Earth Converter (12116): Earth endow, 20 min
- Wind Converter (12117): Wind endow, 20 min

Implementation: Handle `ITEM_ENCHANTARMS` as a special itemskill that applies `endow_fire/water/earth/wind` buff to the user with 20-minute duration.

---

## Phase D: Blacksmith Deferred Skills (~5 hours)

### D1. Ore Discovery (1208) — Passive on Kill

**RO Classic spec (rAthena `mob.cpp` + `item_group_db`):**
- **Passive**: Triggers on every monster kill where Blacksmith has loot priority
- **Single level**: No scaling
- **Algorithm**:
  1. Pick one random item from IG_Ore group (20 items, equal 1/20 chance)
  2. Roll against that item's rate: `chance = (rate × 2 + 1) / 20001`
  3. If pass, drop that item; if fail, nothing drops

**IG_Ore group (20 items):**

| Item ID | Name | Display Rate |
|---------|------|-------------|
| 1002 | Iron Ore | 11.32% |
| 998 | Iron | 10.57% |
| 993 | Green Live | 9.81% |
| 1003 | Coal | 9.06% |
| 992 | Wind of Verdure | 8.30% |
| 1010 | Phracon | 7.55% |
| 991 | Crystal Blue | 6.79% |
| 990 | Red Blood | 6.04% |
| 999 | Steel | 5.28% |
| 1011 | Emveretarcon | 4.53% |
| 757 | Rough Elunium | 3.77% |
| 756 | Rough Oridecon | 3.40% |
| 997 | Great Nature | 3.02% |
| 996 | Rough Wind | 2.64% |
| 995 | Mystic Frozen | 2.26% |
| 994 | Flame Heart | 1.89% |
| 985 | Elunium | 1.51% |
| 984 | Oridecon | 1.13% |
| 969 | Gold | 0.75% |
| 714 | Emperium | 0.38% |

**Implementation**: In the enemy death handler, after normal drops, if killer has `ore_discovery` learned:
1. Pick random item from IG_ORE array
2. Roll `Math.random() < (rate * 2 + 1) / 20001`
3. If pass, add item to killer's inventory via `addItemToInventory()`

### D2. Weapon Repair (1209)

**RO Classic spec (verified 2nd pass):**
- **SP**: 30
- **Cast Time**: 7500ms (pre-renewal, variable, reducible by DEX) — **existing skill_data has 5000ms which is WRONG**
- **Range**: 2 cells (activates at 3, succeeds at 2)
- **Target**: Self **or** Ally (can repair own equipment AND others')
- **Interruptible**: Yes

**Materials per equipment type:**

| Equipment | Material | Item ID |
|-----------|----------|---------|
| Armor (all) | 1× Steel | 999 |
| Lv1 Weapon | 1× Iron Ore | 1002 |
| Lv2 Weapon | 1× Iron | 998 |
| Lv3 Weapon | 1× Steel | 999 |
| Lv4 Weapon | 1× Rough Oridecon | 756 |

**Implementation**:
1. On cast, emit `repair:item_list` with target's broken equipment list
2. Player selects which item to repair via `repair:select`
3. Check Blacksmith has required material in inventory
4. Consume material, clear `weaponBroken`/`armorBroken` flag on target's equipment
5. Emit equipment update

### D3. Greed (1210)

**RO Classic spec:**
- **SP**: 10
- **Cast Time**: 0 (instant)
- **After-Cast Delay**: 1 second
- **AoE**: 5×5 cells (2-cell radius)
- **Quest Skill**: No skill prerequisites
- **Cannot be used in**: Towns, PvP maps, GvG

**BLOCKED BY**: Ground loot system (items currently go directly to inventory).

**Implementation (deferred until ground loot exists)**:
1. Iterate all ground loot items within 5×5 of caster
2. Check pickup priority for each item
3. Add all eligible items to caster's inventory
4. Remove from ground

**Note**: Can implement a simplified version now that instantly picks up all items from recently killed enemies in 5×5 range (checking kill priority), even without a full ground loot system.

---

## Phase E: Sage Deferred Skills (~8 hours)

### E1. Abracadabra (1416) — Random Skill Cast

**RO Classic spec (rAthena `hocuspocus.cpp` + `abra_db.yml`):**
- **SP**: 50 flat (all levels)
- **Catalyst**: 2× Yellow Gemstone (minimum 1 with Mistress Card/Into the Abyss)
- **Cast Time**: 0
- **Max Level**: 10

**Algorithm:**
1. Pick random skill from abra_db pool
2. Roll `rnd() % 10000` against skill's probability for current Abracadabra level
3. If fail, re-pick (up to `pool_size × 3` attempts)
4. If all fail, last picked skill is used regardless
5. Cast selected skill at `min(abracadabraLv, skillMaxLv)` with NO additional SP/catalyst cost
6. Selected skill uses its own cast time and delay

**145 regular/castable skills** (all have default probability 500 = 5% weight — pool is LARGER than initially reported):
- Swordsman (4): Bash, Provoke, Magnum Break, Endure
- Mage (12): Sight, Napalm Beat, Safety Wall, Soul Strike, Cold/Fire/Lightning Bolt, Frost Diver, Stone Curse, Fireball, Fire Wall, Thunderstorm
- Acolyte (12): Ruwach, Pneuma, Teleport, Warp, Heal, Inc AGI, Dec AGI, Holy Water, Crucis, Angelus, Blessing, Cure
- Archer (3): Concentration, Double Strafe, Arrow Shower
- Thief (4): Steal, Hiding, Envenom, Detoxify
- Merchant (3): Identify, Vending, Mammonite
- Other (1): All Resurrection
- Knight (7): Pierce, Brandish Spear, Spear Stab, Spear Boomerang, THQ, Auto Counter, Bowling Bash
- Priest (14): Impositio through Magnus
- Wizard (13): Fire Pillar through Estimation
- Blacksmith (5): Hammer Fall, Adrenaline Rush, Weapon Perfect, Overthrust, Maximize
- Hunter (14): All traps + Blitz Beat + Detecting + Spring Trap
- Assassin (7): Cloaking, Sonic Blow, Grimtooth, Enchant Poison, Poison React, Venom Dust, Splasher
- Rogue (10): Steal Coin, Backstab, Raid, 4× Strip, Intimidate, Graffiti, Cleaner
- Crusader (10): Auto Guard, Shield Charge/Boomerang, Reflect Shield, Holy/Grand Cross, Devotion, Providence, Defender, Spear Quicken
- Monk (9): Call/Absorb Spirits, Body Relocation, Investigate, Finger Offensive, Steel Body, Blade Stop, Explosion Spirits, Extremity Fist
- Sage (13): Cast Cancel, Magic Rod, Spell Breaker, Autospell, 4× Endow, Volcano, Deluge, Violent Gale, Land Protector, Dispell
- Bard/Dancer (4): Musical Strike, Frost Joker, Slinging Arrow, Scream

**Selected skill uses caster's stats** — the skill is cast as if the player had learned it themselves (their own ATK/MATK/INT/DEX apply). Skill level = `min(abracadabraLv, skillMaxLv)`.

**11 Special effects with custom probabilities** (2nd pass correction — SA_FORTUNE and SA_LEVELUP are NOT in abra_db.yml):

| Effect | Internal Name | What It Does | Target | Probabilities (Lv1→Lv10) |
|--------|--------------|--------------|--------|---------------------------|
| Beastly Hypnosis | SA_TAMINGMONSTER | Tame monster without item | Enemy | 50/100/150/200/250/300/350/400/450/500 (pre-re only) |
| Mono Cell | SA_MONOCELL | Transform target to Poring (boss immune) | Enemy | 250/500/750/1000/1250/1200*/1750/2000/2250/2500 |
| Class Change | SA_CLASSCHANGE | Transform to random mob (CAN be MVP, boss immune) | Enemy | 0/0/0/0/10/10/20/20/30/30 |
| Monster Chant | SA_SUMMONMONSTER | Summon random mob near caster (like Dead Branch) | Self | 100/200/300/400/500/600/700/800/900/1000 |
| Grampus Morph | SA_REVERSEORCISH | Orc head on caster (cosmetic, 20 min) | Self | 0/0/0/0/0/0/0/10/50/100 |
| Grim Reaper | SA_DEATH | Instant kill non-boss monster (**NO EXP/drops** via status_kill) | Enemy | 50/100/150/200/250/300/350/400/450/500 |
| Questioning | SA_QUESTION | "?" emote, nothing happens | Self | 1000/800/600/400/200/0/0/0/0/0 |
| Gravity | SA_GRAVITY | Gravity logo on head (cosmetic) | Self | 0/0/0/0/0/0/0/20/50/100 |
| Suicide | SA_INSTANTDEATH | **Kill CASTER** (with EXP loss via normal death penalty) | Self | 0/0/0/0/0/0/0/10/50/100 |
| Rejuvenation | SA_FULLRECOVERY | Full HP **AND** SP restore to 100% | Self | 0/0/0/0/0/0/20/50/100/200 |
| Coma | SA_COMA | **CASTER's** HP and SP → 1 (affects SELF, not target!) | Self | 0/0/0/0/100/200/300/400/500/600 |

*Lv6=1200 appears to be a data error in rAthena (should be 1500 to follow pattern) but matches the actual file.

**NOT in pool** (contrary to initial report): SA_FORTUNE (Gold Digger) and SA_LEVELUP (Leveling) — skill IDs exist (296, 300) but are absent from `abra_db.yml`.

**Simplified implementation for now**: Implement regular skill pool (145 skills) + the 6 most impactful special effects (Grim Reaper, Mono Cell, Rejuvenation, Coma, Suicide, Questioning). Defer cosmetic/disabled effects (Gravity, Grampus Morph, Taming, Class Change, Summon).

### E2. Create Elemental Converter (1420)

**RO Classic spec:**
- **Quest Skill**: Learned via quest
- **SP**: 30
- **Success Rate**: 100% (hardcoded in rAthena — `make_per = 100000`)

**Recipes (pre-renewal `produce_db.txt`):**

| Converter | Product ID | Materials |
|-----------|-----------|-----------|
| Fire | 12114 | 1× Blank Scroll (7433) + 3× Scorpion Tail (904) |
| Water | 12115 | 1× Blank Scroll (7433) + 3× Snail's Shell (946) |
| Earth | 12116 | 1× Blank Scroll (7433) + 3× Horn (947) |
| Wind | 12117 | 1× Blank Scroll (7433) + 3× Rainbow Shell (1013) |

**Implementation**:
1. On `skill:use` with ID 1420, send recipe list to client via `crafting:open`
2. Player selects which converter to create
3. Check materials in inventory
4. Consume materials, create 1× converter, add to inventory
5. 100% success rate — no failure possible
6. Can reuse existing CraftingSubsystem/SCraftingPopup UI

### E3. Elemental Change (1421)

**RO Classic spec (rAthena + iRO Wiki Classic, verified 2nd pass):**
- **Quest Skill**: Player can only learn ONE variant (Fire/Water/Earth/Wind)
- **SP**: 30
- **Cast Time**: 2000ms (2 seconds)
- **After-Cast Delay**: 1000ms
- **Range**: 9 cells (450 UE units)
- **Target**: Enemy monster only — does NOT work on players
- **Success Rate**: Variable based on Endow skill level + stats (iRO Wiki Classic formula, **NOT 100%** — initial plan was wrong):
  - Base rate: `endowLevel × 10 - 5`% (5%/15%/25%/35%/45% for Endow Lv1-5)
  - Bonuses: +INT/DEX/JobLevel contributions — with high stats and Lv5 Endow, approaches 100%
  - **2nd pass correction**: rAthena code passes `rate=100` (100%) but iRO Wiki Classic documents variable rates. Use iRO Wiki formula for RO Classic accuracy.
- **Duration**: **Permanent** (persists until monster death/respawn — iRO Wiki Classic says "Permanent"; rAthena uses 30 min technically but official behavior is permanent)
- **Boss immunity**: YES — does NOT work on bosses (`MD_STATUSIMMUNE` check)
- **Catalyst**: 1× matching Elemental Converter
- **Element level set**: **Level 1 always** (2nd pass correction — random 1-4 was wrong; consistent with other element-changing effects in RO Classic)

**Implementation**:
1. On cast, validate: target is monster, target is not boss, caster has matching converter
2. Calculate success rate based on Endow level + caster stats
3. Roll success check — if fail, emit skill:error, still consume SP and converter
4. If success: consume 1× converter, set `enemy.overrideElement = { element: skillElement, level: 1 }`
5. Duration: permanent (until enemy death)
6. Damage calculations read `enemy.overrideElement` if set

---

## Phase F: Redemptio (~2 hours)

### Redemptio (1018) — Mass Party Resurrection

**RO Classic spec (rAthena `redemptio.cpp`):**
- **Quest Skill**: Max Level 1
- **SP**: 400
- **Cast Time**: 4 seconds (uninterruptible, unreducible by DEX)
- **AoE**: 15×15 cells (7-cell splash radius)
- **Target**: Dead party members only (uses `BCT_PARTY` flag)

**Effect**:
1. Resurrects ALL dead party members within 15×15 at **50% HP, 0 SP**
2. Caster: HP set to **1**, SP set to **1** (2nd pass correction — both HP AND SP set to 1, not SP=0)
3. Caster does NOT die
4. If 0 dead party members in range: skill still casts (HP/SP set, EXP deducted) — does NOT pre-check for targets

**EXP penalty**:
```
EXP_lost = nextBaseEXP × (5 - revived_count) × 1 / 5 / 100
```
- Base penalty: 1% of EXP to next base level
- Reduced by 0.2% per person actually revived
- With 5+ people revived: penalty = 0
- If caster doesn't have enough EXP: skill FAILS (checked before execution)

**Implementation**:
1. Replace stub with full handler
2. Find all dead party members in 15×15 range using `player.partyId`
3. Check caster EXP >= penalty amount
4. Deduct EXP from caster
5. Set caster HP=1, SP=1
6. Revive each dead party member: `status_revive(target, 50, 0)`
7. Emit health updates for caster + all revived members

---

## Phase G: Homunculus Combat + Skills (~12 hours)

### G1. Homunculus Targetable by Enemies

**RO Classic spec (rAthena `mob.cpp` + `status.cpp`):**
- Enemies CAN target homunculi
- Aggressive mobs aggro on homunculi within detection range
- Bosses CAN target homunculi

**Homunculus defensive stats (pre-renewal, verified from rAthena `status.cpp` BL_HOM section):**
- FLEE = Level + AGI (**NOT** `Level + AGI + floor(Level/10)` — existing code has bug adding extra term)
- HIT = Level + DEX + 150 (the +150 IS in rAthena source despite iRO Wiki omitting it)
- Hard DEF = VIT + floor(Level/2) (**existing code has completely wrong formula** — overly complex with AGI terms)
- Soft DEF = VIT + floor(AGI/2)
- MDEF = floor((VIT + Level) / 4) + floor(INT / 2) (**missing from existing code**)
- Soft MDEF = floor((VIT + INT) / 2) (**missing from existing code**)
- ATK: baseATK = `2 × Level + STR`, variance min = `(STR+DEX)/5`, max = `(LUK+STR+DEX)/3` (**existing code has wrong formula**)
- MATK: min = `INT + Level + (INT+DEX)/5`, max = `INT + Level + (LUK+INT+DEX)/3` (**existing code only has max with spurious terms**)
- ASPD: **NOT fixed at 130** — dynamic: `amotion = (1000 - 4×AGI - DEX) × 700 / 1000`, ASPD = `200 - amotion/10` (**existing code uses fixed 130**)
- CRIT = floor(LUK/3) + 1 (existing code correct)
- **No multi-mob FLEE reduction** (homunculi exempt from penalty when attacked by multiple enemies)

**Existing `ro_homunculus_data.js` bugs to fix:**
- Lif baseSTR: 12 → **17** (rAthena pre-re homunculus_db.yml)
- Moonlight SP costs: [10,14,18,22,26] → **[4,8,12,16,20]** (formula: `4 × skillLv`)
- Moonlight damage ratios: plan had 120-560%, iRO Wiki + divine-pride say **220/330/440/550/660%** — use iRO Wiki values
- Caprice: single bolt → **1/2/3/4/5 hits** (scales with Caprice level, matching bolt level)
- Amistr Bulwark description: "+10-30 VIT" → **"+2/4/6/8/10 VIT/DEF"** (pre-renewal: `2 × level`, NOT `5+5×level` which is renewal)
- Urgent Escape SP: [20,20,20,20,20] → **[20,25,30,35,40]** (formula: `15 + 5 × level`)

**Implementation**:
1. Fix `calculateHomunculusStats()` in `ro_homunculus_data.js` with correct formulas above
2. Add homunculi to `findAggroTarget()` — include active homunculi as valid targets
3. Give homunculi position data (x/y/z from owner + small offset)
4. Add damage reception path: calculate damage using homunculus DEF/FLEE stats
5. Implement `homunculus:health_update` event
6. Implement homunculus death (HP <= 0)
7. Owner death behavior: if homunculus HP > 80%, auto-vaporize; else continue fighting

### G2. Homunculus Skill Execution

**12 skills across 4 types:**

#### Lif Skills
| Skill | Type | SP | Effect |
|-------|------|-----|--------|
| Healing Hands | Supportive | 13/16/19/22/25 | Heal = `(Level + INT) / 8 × (4 + skillLv × 8)`, +2%/lv from Brain Surgery |
| Urgent Escape | Buff | 20/25/30/35/40 | +10/20/30/40/50% move speed, 40/35/30/25/20s, both Lif AND owner |
| Brain Surgery | Passive | — | +1-5% MaxSP, +2-10% Healing Hands, +3-15% SP regen |

#### Amistr Skills
| Skill | Type | SP | Effect |
|-------|------|-----|--------|
| Castling | Active | 10 | 20/40/60/80/100% chance to swap positions + redirect aggro |
| Amistr Bulwark | Buff | 20/25/30/35/40 | +2/4/6/8/10 VIT, 40/35/30/25/20s, both Amistr AND owner |
| Adamantium Skin | Passive | — | +2-10% HP, +5-25% HP regen, +4/8/12/16/20 flat DEF |

#### Filir Skills
| Skill | Type | SP | Effect |
|-------|------|-----|--------|
| Moonlight | Offensive | 4/8/12/16/20 | 220/330/440/550/660% physical ATK, 1/2/2/2/3 hits (damage divided among hits), 2s delay |
| Flitting | Self-buff | 30/40/50/60/70 | +3/6/9/12/15 ASPD, +10/15/20/25/30% ATK, 60/55/50/45/40s |
| Accelerated Flight | Self-buff | 30/40/50/60/70 | +20/30/40/50/60 FLEE, 60/55/50/45/40s |

#### Vanilmirth Skills
| Skill | Type | SP | Effect |
|-------|------|-----|--------|
| Caprice | Offensive | 22/24/26/28/30 | Random bolt (Fire/Cold/Lightning/Earth Spike) at skillLv — **1/2/3/4/5 hits** (= bolt level = Caprice level), 2.0-3.0s delay |
| Chaotic Blessings | Supportive | 40 | Random heal on self/owner/enemy, chance varies by level |
| Instruction Change | Passive | — | +1/1/3/4/4 STR, +1/2/2/4/5 INT |

**Implementation**:
1. Add `homunculus:use_skill` socket handler
2. Validate: homunculus exists, has skill points in slot, enough SP
3. Execute skill effect (damage/heal/buff)
4. Apply cooldown/delay
5. For AI auto-cast: homunculus AI can use skills based on command mode

### G3. Homunculus Evolution

**Requirements:**
- Intimacy ≥ 91,100 (Loyal grade on rAthena scale)
- Use Stone of Sage item

**On evolution:**
1. Class changes: Lif → Lif_H, Amistr → Amistr_H, etc.
2. Random stat bonuses applied (per-type growth tables)
3. Intimacy resets to **10** (Hate grade — rAthena `homunculus_evo_intimacy_reset = 1000` internal = 10 display)
4. 4th skill slot unlocked when intimacy re-reaches **910** post-evolution (1 point before Loyal)

**Evolution stat growth (multiply these by 10 for base stats):**

| Type | HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|------|-----|-----|------|------|------|------|------|------|
| Lif | 1-10 | 10-20 | 1-5 | 1-4 | 1-5 | 4-10 | 1-10 | 1-3 |
| Amistr | 10-20 | 1-10 | 1-10 | 1-5 | 4-10 | 1-3 | 1-4 | 1-5 |
| Filir | 5-15 | 5-15 | 4-10 | 1-10 | 1-3 | 1-4 | 1-5 | 1-5 |
| Vanilmirth | 1-30 | 1-30 | 1-10 | 1-10 | 1-10 | 1-10 | 1-10 | 1-10 |

**4th skills (unlocked after re-reaching Loyal post-evolution):**

| Type | Skill | Effect |
|------|-------|--------|
| Lif | Mental Charge | ATK uses MATK, +30/60/90 VIT, +20/40/60 INT, 1/3/5 min |
| Amistr | Blood Lust | +30/40/50% ATK, 3/6/9% HP drain, 1/3/5 min |
| Filir | S.B.R.44 | Damage = 100/200/300 × Intimacy, intimacy → 0 |
| Vanilmirth | Bio Explosion | Damage = MaxHP × 1.0/1.5/2.0, 9×9 AoE, pierces DEF, kills homunculus |

---

## Phase H: Monster Summon/Transform (~4 hours)

### H1. NPC_SUMMONSLAVE — Monster Summoning

**RO Classic spec (verified 2nd pass against rAthena `mob.cpp` + `mob_skill_db`):**
- Triggered by `slavelt` condition (slave count < threshold) or `onspawn` (once at spawn)
- Spawns slaves from the skill's value list (monster IDs)
- Slaves inherit mode/speed from master (configurable via `slaves_inherit_mode`/`slaves_inherit_speed`)
- **Slave EXP/drops depend on how they die**:
  - Killed by players: give normal EXP and drops (based on the slave's mob_id)
  - Die when master dies: **NO EXP, NO drops** (despawn without rewards)
  - **Bug in current code**: `_giveExpOnDeath`/`_giveDropsOnDeath` flags are SET but **never checked** in `processEnemyDeathFromSkill()` — needs fix
- Re-summon when count drops below threshold
- Slaves follow master, teleport with master, retaliate if attacked

**Implementation**:
Replace the `case 'summon'` stub in monster skill execution:
1. Read `skill.val1` (or val array) for monster IDs to spawn
2. Check `slavelt` condition: `(enemy._slaveCount || 0) < skill.conditionValue`
3. Spawn N slave enemies at positions near master
4. Set `slave.masterId = enemy.id` on spawned slaves
5. Increment `enemy._slaveCount`
6. On slave death, decrement master's `_slaveCount`
7. Emit standard `enemy:spawned` for each slave

**Notable summoning bosses to support:**
- Baphomet: summons Baphomet Jr (1101)
- Osiris: summons Isis + Ancient Mummy
- Golden Thief Bug: summons 10 Thief Bug Male
- Drake, Eddga, Maya, Moonlight Flower, Orc Lord, Stormy Knight, etc.

### H2. NPC_METAMORPHOSIS — Monster Transformation

**RO Classic spec:**
- Changes monster class (sprite + stats)
- Preserves HP **ratio** (not absolute HP)
- Recalculates all stats from new monster template
- One-way (cannot transform back)
- Pre-renewal usage: Only egg/larval forms

**Monsters using metamorphosis:**

| Monster | Transforms Into |
|---------|----------------|
| Fabre (1007) | Pupa (1008) |
| Pupa (1008) | Creamy (1018) |
| Peco Peco Egg (1047) | Picky (1049) or Shell Picky (1050) |
| Thief Bug Egg (1048) | Thief Bug Larva (1051) |
| Ant Egg (1097) | Ant (1095), Andre (1105), or Vitata (1160) |

**Implementation**:
1. Store HP ratio: `hpRatio = enemy.hpCurrent / enemy.hpMax`
2. Look up new monster template from `ro_monster_templates`
3. Replace enemy stats with new template stats
4. Set `enemy.hpMax` from new template, `enemy.hpCurrent = Math.floor(hpRatio * enemy.hpMax)`
5. Update visual: emit `enemy:transformed` with new monster data
6. Recalculate ATK, DEF, MDEF, FLEE, HIT from new template

**Note**: `NPC_TRANSFORMATION` (non-metamorphosis) has 0 entries in pre-renewal db — do NOT implement.

---

## Phase I: Moonlit Water Mill Effect (~2 hours)

**Current state**: Performance exists (SP drain, ground effect created) but `PERFORMANCE_BUFF_MAP` maps to `null` — no actual effect.

**RO Classic effect**: Movement blocking barrier — monsters/players CANNOT enter the zone.

**Implementation** (after Phase B reclassifies MWM as ensemble):
1. In the movement validation code (enemy AI movement + player position update):
   - Check if destination position falls within any active MWM ground effect
   - If yes, block the movement (enemy stays in place / player position rejected)
2. On cast: Push out any entities currently inside the zone
3. Does NOT block: ranged attacks, magic, skills — only physical entry

---

## Phase J: Quest Skill NPC Gates (~3 hours, LOW PRIORITY)

**Current state**: `isQuestSkill`/`questSkill` flags exist in skill data but `canLearnSkill()` never checks them.

**RO Classic**: Quest skills require NPC interaction to learn, cannot be learned from skill tree.

**17 quest skills affected:**
- First Aid (2), Play Dead (3), Charge Attack (710), Phantasmic Arrow (917)
- Pang Voice (1509), Charming Wink (1529), Sonic Acceleration (1106), Throw Venom Knife (1111)
- Dubious Salesmanship (1211), Greed (1210), Scribble (1717)
- Close Confine (1718), Redemptio (1018)
- Create Elemental Converter (1420), Elemental Change (1421)
- Ki Translation (1614), Ki Explosion (1615)

**Implementation**:
1. In `canLearnSkill()`, check `skill.isQuestSkill || skill.questSkill`
2. If true, reject with "This skill must be learned from an NPC quest"
3. Add `quest:learn_skill` event that bypasses the quest check
4. Wire NPC quest dialogs to emit `quest:learn_skill`

**Priority**: LOW — skill tree learning is acceptable for MVP. NPC gating is polish.

---

## Implementation Priority Order

| Phase | Est. Hours | Impact | Dependencies | Status |
|-------|-----------|--------|--------------|--------|
| **A** Dead Code Wiring | 3 | HIGH — 2 core mechanics non-functional | None | **DONE** (2026-03-18) |
| **C** Consumable Effects | 6 | HIGH — ASPD potions, scrolls, all buff items | None | **DONE** (2026-03-18) |
| **D** Blacksmith Skills | 5 | MEDIUM — Ore Discovery, Weapon Repair | None | **DONE** (2026-03-18) |
| **E** Sage Skills | 8 | MEDIUM — Abracadabra, Converter, Elem Change | None | **DONE** (2026-03-18) |
| **F** Redemptio | 2 | MEDIUM — mass party res | Party system (done) | **DONE** (2026-03-18) |
| **B** Ensemble System | 15 | HIGH — 16 skills + data corrections | None | **DONE** (2026-03-18) |
| **G** Homunculus Combat | 12 | HIGH — targeting + 12 skills + evolution | None | **DONE** (2026-03-18) |
| **H** Monster Summon/Transform | 4 | MEDIUM — boss encounters incomplete | None | **DONE** (2026-03-18) |
| **I** MWM Effect | 2 | LOW — single ensemble effect | Phase B | **DONE** (2026-03-18) |
| **J** Quest NPC Gates | 3 | LOW — polish, not gameplay-critical | NPCs | **DONE** (2026-03-18) |
| **TOTAL** | **~60** | | | **~60 hours implemented** |

---

## Verification Checklist

After each phase, verify:

- [ ] **Phase A**: Cast Magic Rod → enemy bolt → damage 0, SP gained. Equip Maya Card → enemy bolt → 50% reflect chance.
- [ ] **Phase B**: Bard+Dancer in party → adjacent → ensemble skill → 9×9 AoE effect active → SP drain both → aftermath debuff.
- [ ] **Phase C**: Use Awakening Potion → ASPD increases by 15% delay reduction → lasts 30 min. Use Fire Bolt Scroll → casts Fire Bolt Lv3 → uses player MATK → no SP cost.
- [ ] **Phase D**: Kill monster as Blacksmith with Ore Discovery → chance of ore drop. Use Weapon Repair on ally → material consumed → weapon unbroken.
- [ ] **Phase E**: Cast Abracadabra → random skill fires → 2 Yellow Gems consumed. Create Converter → Blank Scroll + materials → converter item created.
- [ ] **Phase F**: Cast Redemptio → all dead party members in 15×15 res at 50% HP → caster HP=1 SP=1 → EXP penalty applied.
- [ ] **Phase G**: Enemy attacks homunculus → takes damage → can die. Homunculus uses Healing Hands → heals. Feed Stone of Sage → evolution → stat bonuses.
- [ ] **Phase H**: Baphomet spawns → summons Baphomet Jr slaves → on slave death → re-summons. Fabre → metamorphosis → becomes Pupa.
- [ ] **Phase I**: MWM ensemble active → enemies cannot enter zone → pushed out if inside.
- [ ] **Phase J**: Cannot learn Charge Attack from skill tree → must interact with NPC.
