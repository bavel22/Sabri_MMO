# Blacksmith Skills Comprehensive Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Blacksmith_Class_Research](Blacksmith_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Status:** COMPLETE — All 9 fixes implemented (BUG 9 deferred). Verified 2026-03-17.

**Date:** 2026-03-16 (revised with deep research)
**Scope:** All Blacksmith skills (IDs 1200-1211, 1220-1230), damage pipeline integrations, passive system, forging/refining
**Reference:** rAthena pre-renewal source code (skill_db.yml, status.cpp, battle.cpp, skill.cpp), iRO Wiki Classic, divine-pride, rateMyServer
**Authoritative source:** rAthena GitHub pre-re database, cross-referenced with iRO Wiki Classic tested values

---

## Audit Summary

| Category | Count |
|----------|-------|
| Skills Audited | 24 (12 combat + 12 forging) |
| Bugs Found | 10 |
| Verified Correct | 14 skills + 4 pipeline integrations |
| Deferred (Expected) | 5 skills (Ore Discovery, Weapon Repair, Greed, Iron/Steel/Stone Tempering) |

---

## BUGS FOUND — Must Fix

### BUG 1: Maximize Power NOT Cleared on Player Death (CRITICAL)

**File:** `server/src/index.js` — respawn handler (line ~5243) and combat tick (line ~17474)
**Problem:** When a player dies, `player.isDead = true` is set but `player.maximizePowerActive` is never cleared, and the `maximize_power` buff is never removed. The SP drain tick in the combat loop also has no `isDead` check, so SP continues draining while the player is dead.

**Expected (rAthena):** `status_change_clear(bl, 0)` in `pc_dead()` removes ALL buffs except those with `NoRemoveOnDead: true`. SC_MAXIMIZEPOWER does NOT have that flag — it is removed on death.

**Fix (2 parts):**

**Part A — Add `isDead` guard to Maximize Power SP drain tick:**
```js
if (!p.maximizePowerActive || p.isDead) continue;
```

**Part B — Handled by BUG 2 (systemic death buff clearing)**

**Severity:** CRITICAL

---

### BUG 2: No Buff Clearing on Death/Respawn (SYSTEMIC)

**File:** `server/src/index.js` — all death paths + respawn handler
**Problem:** ALL buffs persist through death and respawn. This is wrong — rAthena clears most buffs on death.

**Expected (rAthena):** `pc_dead()` calls `status_change_clear(bl, 0)` which removes all SC_ statuses EXCEPT those with `NoRemoveOnDead: true` in `db/pre-re/status.yml`.

**Buffs that SHOULD survive death (from rAthena `NoRemoveOnDead: true`):**
- `auto_berserk` (SC_AUTOBERSERK) — the toggle survives; the Provoke effect re-triggers when HP drops below 25%
- `endure` (SC_ENDURE)
- `shrink` (SC_SHRINK)
- Song/Dance effects on affected players (whistle, bragi, apple_of_idun, humming, fortune_kiss, service_for_you, assassin_cross, dontforgetme)
- Weight states (weight_50, weight_90)

**Buffs that SHOULD be removed on death (everything else):**
- Adrenaline Rush, Weapon Perfection, Power Thrust, Maximize Power
- Blessing, Increase AGI, Angelus, Impositio Manus, Suffragium
- Two-Hand Quicken, Spear Quicken
- All Crusader/Knight/Priest/Sage buffs
- Provoke, Signum Crucis
- Steel Body, Critical Explosion

**Things that survive death but are NOT standard buffs:**
- Spirit Spheres (stored as player property, not a buff)
- Mount status (Peco Peco — option flag, not timed buff)
- Falcon status (option flag)
- Cart status (option flag)

**Fix:** Create `clearBuffsOnDeath(player, characterId, zone)`:
```js
const SURVIVE_DEATH = new Set([
    'auto_berserk', 'endure', 'shrink',
    'song_whistle', 'song_bragi', 'song_apple_of_idun', 'song_humming',
    'song_assassin_cross', 'dance_fortune_kiss', 'dance_service_for_you', 'dance_pdfm'
]);

function clearBuffsOnDeath(player, characterId, zone) {
    const removed = [];
    player.activeBuffs = (player.activeBuffs || []).filter(b => {
        if (SURVIVE_DEATH.has(b.name)) return true;
        removed.push(b.name);
        return false;
    });
    // Clear toggle states
    player.maximizePowerActive = false;
    player.weaponBroken = false;
    if (player.performanceState) {
        cancelPerformance(characterId, player, zone); // if performer
    }
    // Broadcast removals
    for (const name of removed) {
        broadcastToZone(zone, 'skill:buff_removed', {
            targetId: characterId, isEnemy: false, buffName: name, reason: 'death'
        });
    }
    // Note: spirit spheres, mount, falcon survive — they are player properties, not buffs
}
```

Call from ALL player death paths AND as safety net in respawn handler.

**Severity:** CRITICAL — affects all classes

---

### BUG 3: ASPD Buffs Stack Multiplicatively Instead of Max-Wins (MEDIUM)

**File:** `server/src/ro_buff_system.js` — all ASPD buff cases
**Problem:** Currently, ASPD buffs multiply together: `mods.aspdMultiplier *= (buff.aspdMultiplier || 1.0)`. If a player somehow has both Adrenaline Rush (1.3) and Two-Hand Quicken (1.3), the result is `1.3 * 1.3 = 1.69` (69% ASPD boost).

**Expected (rAthena):** ASPD buffs **coexist but do NOT stack**. Only the **strongest** value is applied. From `status_calc_aspd_rate()`:
```cpp
int max = 0;
if(sc->data[SC_TWOHANDQUICKEN] && max < sc->data[SC_TWOHANDQUICKEN]->val2)
    max = sc->data[SC_TWOHANDQUICKEN]->val2;
if(sc->data[SC_ADRENALINE] && max < sc->data[SC_ADRENALINE]->val3)
    max = sc->data[SC_ADRENALINE]->val3;
if(sc->data[SC_SPEARQUICKEN] && max < sc->data[SC_SPEARQUICKEN]->val2)
    max = sc->data[SC_SPEARQUICKEN]->val2;
aspd_rate -= max;
```

**ASPD buff values (rAthena):**
| Buff | Self-cast | Party-received |
|------|-----------|---------------|
| Adrenaline Rush | 300 (30%) | 200 (20%) |
| Two-Hand Quicken | 300 (30%) | N/A |
| Spear Quicken | 200+10*lv (21-30%) | N/A |
| Assassin Cross of Sunset (song) | variable | variable |

Casting one does NOT cancel the other. They coexist, strongest wins.

**Fix:** Change `getBuffStatModifiers()` to use max-wins for ASPD:
```js
// Instead of multiplying all aspdMultiplier values, find the max
const ASPD_BUFF_NAMES = ['adrenaline_rush', 'two_hand_quicken', 'spear_quicken', 'one_hand_quicken'];
let maxAspdBoost = 0; // track as additive boost (0.3 = 30%)

// In the buff loop, for ASPD buffs:
case 'adrenaline_rush':
    maxAspdBoost = Math.max(maxAspdBoost, (buff.aspdMultiplier || 1.0) - 1.0);
    break;
case 'two_hand_quicken':
case 'spear_quicken':
    maxAspdBoost = Math.max(maxAspdBoost, (buff.aspdIncrease || 0) / 100);
    break;

// After buff loop:
mods.aspdMultiplier = 1.0 + maxAspdBoost; // Only strongest applies
```

**Also:** Quagmire and Decrease AGI should END all ASPD buffs when applied (this is a separate enhancement).

**Severity:** MEDIUM — currently unlikely to trigger due to weapon type restrictions, but will be wrong with party buffs

---

### BUG 4: Power Thrust Weapon Break — Axes/Maces Should Be Immune

**File:** `server/src/index.js` — Power Thrust weapon break (line ~18081)
**Problem:** The 0.1% weapon break applies to ALL weapon types. In rAthena, **Axes, Maces, and Unbreakable weapons are immune** to Power Thrust self-break.

**Expected (rAthena):** From `status_change.txt`: "Add a 0.1% of breaking the equipped weapon [except Axes, Maces & Unbreakable weapons]"

This is significant because Blacksmiths primarily use axes and maces — they should almost never trigger their own weapon break.

**Fix:** Add weapon type immunity check:
```js
if (ptBuff && Math.random() < 0.001) {
    const breakImmuneWeapons = ['axe', 'one_hand_axe', 'two_hand_axe', 'mace'];
    const isUnbreakable = attacker.equippedWeaponRight?.unbreakable;
    if (!breakImmuneWeapons.includes(attacker.weaponType) && !isUnbreakable) {
        attacker.weaponBroken = true;
        attacker.stats.weaponATK = 0;
        // ... rest of break logic
    }
}
```

**Severity:** MEDIUM — without this fix, Blacksmiths using axes/maces can break their own weapon, which shouldn't happen

---

### BUG 5: Hammer Fall Weapon Restriction Too Narrow

**File:** `server/src/index.js` — Hammer Fall handler (line ~14589)
**Problem:** Current code allows only Axe and Mace. rAthena allows a broader set of weapons.

**Current code:**
```js
const validWeapons = ['axe', 'one_hand_axe', 'two_hand_axe', 'mace'];
```

**Expected (rAthena `skill_db.yml` for BS_HAMMERFALL):**
```yaml
Weapon:
  Dagger: true
  1hSword: true
  1hAxe: true
  2hAxe: true
  Mace: true
```

**Missing:** `dagger` and `one_hand_sword`. Bare hands are NOT allowed.

**Fix:** Expand the valid weapons list:
```js
const validWeapons = ['dagger', 'one_hand_sword', 'axe', 'one_hand_axe', 'two_hand_axe', 'mace'];
```

**Severity:** MEDIUM — Blacksmiths with daggers or 1H swords cannot use Hammer Fall

---

### BUG 6: Maximize Power Does Not Block SP Regeneration

**File:** `server/src/index.js` — SP regen tick
**Problem:** While Maximize Power is active, SP continues to regenerate normally. In rAthena, SP regeneration is explicitly disabled during Maximize Power.

**Expected (rAthena):** From `status_change.txt` for SC_MAXIMIZEPOWER: "SP Regeneration is disabled". Confirmed by [rAthena commit 4771d05](https://github.com/rathena/rathena/commit/4771d055ecf0216fd984ed74cb3bd3e8d0560932).

**Fix:** In the SP regen tick, check for Maximize Power:
```js
// In SP regen section:
if (player.maximizePowerActive) {
    // Skip SP regen — Maximize Power disables SP recovery
    continue; // or skip the SP portion
}
```

**Severity:** HIGH — SP regen partially offsets the SP drain, making Maximize Power far more sustainable than intended. At Lv5 the drain is only 1 SP/5s, and SP regen can easily exceed that, making the toggle essentially free.

---

### BUG 7: Refine +10 Success Rates Slightly Wrong

**File:** `server/src/index.js` — REFINE_RATES constant (line ~16900)
**Problem:** The +10 refine rates use 19% and 9% instead of the rAthena pre-renewal values of 20% and 10%.

**Current code:**
```js
weapon_lv1: { ...8:60,9:40,10:19 },
weapon_lv2: { ...8:40,9:20,10:19 },
weapon_lv3: { ...8:20,9:20,10:19 },
weapon_lv4: { ...8:20,9:20,10:9 },
armor:      { ...8:20,9:20,10:9 }
```

**Expected (rAthena `db/pre-re/refine.yml`):**
```
weapon_lv1: 10:20
weapon_lv2: 10:20
weapon_lv3: 10:20
weapon_lv4: 10:10
armor:      10:10
```

**Fix:** Change `10:19` to `10:20` and `10:9` to `10:10`.

**Severity:** LOW — 1% difference at +10 only

---

### BUG 8: Forge Weapon Level Penalty Formula Wrong

**File:** `server/src/index.js` — forge:request success rate calculation (line ~17155)
**Problem:** The weapon level penalty uses `(recipe.weaponLevel - 1) * 1000`, giving lv1=0, lv2=1000(-10%), lv3=2000(-20%).

**Expected (rAthena `skill.cpp`):**
```cpp
if(wlv >= 3) make_per -= (wlv-2)*1000; // Lv3: -1000, Lv4: -2000
```
Actually, from the detailed research: `wlv > 1 ? wlv * 1000 : 0` gives lv1=0, lv2=2000(-20%), lv3=3000(-30%).

**DISCREPANCY:** Different rAthena versions show different formulas. The most commonly referenced pre-renewal formula from multiple sources is:
- Lv1: 0 penalty
- Lv2: -2000 (-20%)
- Lv3: -3000 (-30%)

Our code gives Lv2=-1000(-10%), Lv3=-2000(-20%) — **undercharging the penalty by 10% for both levels.**

**Fix:**
```js
// Change from:
makePer -= (recipe.weaponLevel - 1) * 1000;
// To:
makePer -= (recipe.weaponLevel > 1 ? recipe.weaponLevel * 1000 : 0);
```

**Severity:** MEDIUM — forging is 10% easier than intended for Lv2/Lv3 weapons

---

### BUG 9: Adrenaline Rush ASPD Not Weapon-Checked at Calculation Time

**File:** `server/src/ro_buff_system.js` — adrenaline_rush case (line ~873)
**Problem:** ASPD boost applies unconditionally once the buff exists. In rAthena, `pc_checkallowskill()` removes Adrenaline Rush if the player switches to a non-qualifying weapon.

**Expected (rAthena):** The ASPD boost only applies while wielding Axe or Mace. Switching weapons removes the buff entirely.

**Fix:** Either check weapon type at ASPD calculation time, or add weapon-change check that removes the buff. Lower priority since weapon swapping mid-combat is uncommon.

**Severity:** LOW

---

### BUG 10: Power Thrust Has No Weapon Requirement

**File:** `server/src/index.js` — Power Thrust handler (line ~14518)
**Problem:** Power Thrust can be cast bare-handed. In rAthena, BS_OVERTHRUST requires a weapon equipped (lists 23 weapon types — essentially everything except bare hands).

**Expected (rAthena `skill_db.yml`):** Requires: Weapon (Dagger, 1hSword, 2hSword, 1hSpear, 2hSpear, 1hAxe, 2hAxe, Mace, Staff, Bow, Knuckle, Musical, Whip, Book, Katar, etc.)

**Fix:** Add bare-hand check:
```js
if (skill.name === 'power_thrust') {
    const wType = player.weaponType || 'bare_hand';
    if (wType === 'bare_hand') {
        socket.emit('skill:error', { message: 'Requires a weapon equipped' });
        return;
    }
    // ... rest of handler
}
```

**Severity:** LOW — Blacksmiths rarely fight bare-handed

---

## VERIFIED CORRECT — No Changes Needed

### Skill Data Definitions (`ro_skill_data_2nd.js`)

| Skill | SP Cost | Duration | effectValue | Prerequisites | Verdict |
|-------|---------|----------|-------------|---------------|---------|
| Adrenaline Rush (1200) | [20,23,26,29,32] | [30-150s] | 30 | HF Lv2 | CORRECT |
| Weapon Perfection (1201) | [18,16,14,12,10] | [10-50s] | 100 | AR Lv2, WR Lv2 | CORRECT |
| Power Thrust (1202) | [18,16,14,12,10] | [20-100s] | [5,10,15,20,25] | AR Lv3 | CORRECT |
| Maximize Power (1203) | 10 | toggle | 100 | PT Lv2, WP Lv3 | CORRECT |
| Weaponry Research (1204) | 0 | passive | [2-20] | HB Lv1 | CORRECT |
| Skin Tempering (1205) | 0 | passive | [4-20] | none | CORRECT (see note) |
| Hammer Fall (1206) | 10 | 5000 | [30-70] | none | CORRECT |
| Hilt Binding (1207) | 0 | passive | 4 | none | CORRECT |

**Skin Tempering note:** rAthena source uses 5%/lv fire resist (`skill*5`), but iRO Wiki Classic says the actual tested value is 4%/lv. Our implementation uses 4%/lv which matches iRO Classic tested behavior. This is a known rAthena deviation from official servers.

### Skill Handlers (`index.js`)

| Handler | Verdict | Notes |
|---------|---------|-------|
| Adrenaline Rush | CORRECT | Weapon check, SP, buff(aspdMultiplier:1.3), hilt bonus |
| Weapon Perfection | CORRECT | SP, buff(noSizePenalty:true), hilt bonus |
| Maximize Power toggle | CORRECT | Toggle on/off, SP check, drain interval |
| Hammer Fall | CORRECT (core) | Ground AoE, stun only, effectVal chance (weapon list needs BUG 5 fix) |
| Deferred stubs | CORRECT | Weapon Repair/Greed/Ore Discovery return error |
| Dubious Salesmanship | CORRECT | 10% Mammonite zeny reduction |

### Damage Pipeline Integrations

| Integration | Verdict | Notes |
|-------------|---------|-------|
| noSizePenalty (WP) | CORRECT | Skips SIZE_PENALTY lookup in ro_damage_formulas.js:544 |
| maximizePower (MP) | CORRECT | Forces max weapon variance in ro_damage_formulas.js:568 |
| atkMultiplier (PT) | CORRECT | Multiplies totalATK before skill mod in ro_damage_formulas.js:604 |
| elementResist (ST) | CORRECT | Reduces damage by resist% after element in ro_damage_formulas.js:697 |

### Passive Skill Bonuses

| Passive | Bonuses | Verdict |
|---------|---------|---------|
| Weaponry Research (1204) | +2 ATK/lv, +2 HIT/lv (all weapons) | CORRECT |
| Skin Tempering (1205) | +4% fire/lv, +1% neutral/lv | CORRECT |
| Hilt Binding (1207) | +1 STR, +4 ATK, +10% duration to AR/WP/PT | CORRECT |

### Forging System

| Component | Verdict | Notes |
|-----------|---------|-------|
| Material consumption | CORRECT | All consumed on attempt (even failure) |
| Element stones | CORRECT | 994-997 mapped to fire/water/wind/earth |
| Star crumb ATK bonus | CORRECT | [0,5,10,40] flat mastery ATK |
| Smith skill gating | CORRECT | Each recipe requires matching Smith skill |
| Research Oridecon | CORRECT | +100/lv for Lv3 weapons (wlv>=3, but only lv3 forgeable) |
| Weaponry Research | CORRECT | +100/lv to forge rate |

### Refining System

| Component | Verdict | Notes |
|-----------|---------|-------|
| Safe limits | CORRECT | wep_lv1:+7, lv2:+6, lv3:+5, lv4:+4, armor:+4 |
| Failure = destruction | CORRECT | Item destroyed permanently with all cards |
| Ore costs | CORRECT | Phracon/Emveretarcon/Oridecon/Elunium |
| Zeny fees | CORRECT | 50/200/5000/20000/2000 |
| Max refine +10 | CORRECT | Rejects above +10 |

---

## DEFERRED ITEMS (Expected — Not Bugs)

| Item | Skills Affected | Why Deferred | Priority |
|------|----------------|--------------|----------|
| Party buff broadcast | AR, WP, PT | No party system | Phase 7+ |
| Party PT ATK: flat 5% | PT (1202) — pre-renewal is flat 5% all levels, NOT scaling | No party system | Phase 7+ |
| Party AR ASPD: 20% | AR (1200) — 20% for party (NOT 25%, that's Renewal) | No party system | Phase 7+ |
| Material crafting | Iron/Steel/Stone Tempering (1220-1222) | Need crafting UI | Phase 9+ |
| Weapon Repair handler | Weapon Repair (1209) | Need weapon break system | Phase 7+ |
| Ore Discovery | Ore Discovery (1208) | Need loot drop enhancement | Phase 10+ |
| Greed (ground pickup) | Greed (1210) | Need ground loot system | Phase 10+ |
| Anvil bonus in forging | forge:request | No anvil items yet (+3%/+5%/+10%) | Phase 9+ |
| Quagmire/Dec AGI clear ASPD buffs | AR (1200), THQ, SQ | Separate debuff enhancement | Phase 7+ |

---

## EXECUTION PLAN

### Phase A: Death Buff Clearing (BUG 1 + BUG 2)
**~50 lines in `index.js`**

1. Create `SURVIVE_DEATH` Set and `clearBuffsOnDeath()` helper
2. Call from all `isDead = true` paths (~6 locations)
3. Call from respawn handler as safety net
4. Add `isDead` guard to Maximize Power SP drain tick

### Phase B: ASPD Max-Wins System (BUG 3)
**~25 lines in `ro_buff_system.js`**

1. Change ASPD buff handling from multiplicative to max-wins
2. Track `maxAspdBoost` across all ASPD buff cases
3. Apply as `mods.aspdMultiplier = 1.0 + maxAspdBoost` after loop

### Phase C: Maximize Power SP Regen Block (BUG 6)
**~5 lines in `index.js`**

1. Add `maximizePowerActive` check to SP regen tick to skip SP recovery

### Phase D: Hammer Fall Weapon List (BUG 5)
**~3 lines changed in `index.js`**

1. Add `'dagger'` and `'one_hand_sword'` to valid weapons list

### Phase E: Power Thrust Weapon Break Immunity (BUG 4)
**~5 lines in `index.js`**

1. Add axe/mace/unbreakable immunity check before weapon break trigger

### Phase F: Refine + Forge Rate Fixes (BUG 7 + BUG 8)
**~5 lines in `index.js`**

1. Change +10 refine rates: 19->20, 9->10
2. Change forge penalty: `(wlv-1)*1000` -> `wlv > 1 ? wlv*1000 : 0`

### Phase G: Power Thrust Bare-Hand Check (BUG 10)
**~5 lines in `index.js`**

1. Add bare-hand rejection to Power Thrust handler

### Phase H: AR Runtime Weapon Check (BUG 9) — DEFER OK
**~10 lines in `ro_buff_system.js`**

1. Defer until weapon swapping or party system is implemented

---

## FIX PRIORITY ORDER

| Priority | Phase | Bug | Severity | Impact |
|----------|-------|-----|----------|--------|
| 1 | A | Death buff clearing (BUG 1+2) | CRITICAL | SP drain leak, all buffs persist |
| 2 | B | ASPD max-wins (BUG 3) | MEDIUM | Multiplicative stacking exploit |
| 3 | C | MP SP regen block (BUG 6) | HIGH | MP toggle is essentially free at high levels |
| 4 | D | Hammer Fall weapons (BUG 5) | MEDIUM | Dagger/sword users blocked |
| 5 | E | PT break immunity (BUG 4) | MEDIUM | BS can break own axe/mace (shouldn't) |
| 6 | F | Refine+forge rates (BUG 7+8) | MEDIUM | Rates off by 1-10% |
| 7 | G | PT bare-hand check (BUG 10) | LOW | Rare edge case |
| 8 (DEFER) | H | AR runtime weapon check (BUG 9) | LOW | Needs party/weapon swap system |

---

## RESEARCH SOURCES

| Source | URL/Reference | Used For |
|--------|--------------|----------|
| rAthena pre-re/skill_db.yml | github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml | Skill definitions, weapon restrictions |
| rAthena status.cpp | github.com/rathena/rathena/blob/master/src/map/status.cpp | Buff values, ASPD calc, death clearing |
| rAthena battle.cpp | github.com/rathena/rathena/blob/master/src/map/battle.cpp | Damage modifiers, ATK% application |
| rAthena skill.cpp | github.com/rathena/rathena/blob/master/src/map/skill.cpp | Skill handlers, forge formula, break rates |
| rAthena status_change.txt | github.com/rathena/rathena/blob/master/doc/status_change.txt | SC documentation, break immunity |
| rAthena pre-re/status.yml | github.com/rathena/rathena/blob/master/db/pre-re/status.yml | NoRemoveOnDead flags |
| rAthena pre-re/refine.yml | github.com/rathena/rathena/blob/master/db/pre-re/refine.yml | Refine success rates |
| iRO Wiki Classic | irowiki.org/classic/ | Cross-reference all skills, tested values |
| divine-pride.net | divine-pride.net/database/skill/ | Skill data verification |
| RateMyServer | ratemyserver.net | Forge calculator, refine tables |

---

## KEY CORRECTIONS FROM DEEP RESEARCH

1. **ASPD buffs coexist, strongest wins** — They do NOT cancel each other. Multiple can exist, only max value applies. This replaces the original "mutual exclusion" design.

2. **Hammer Fall allows Dagger + 1hSword** — rAthena `skill_db.yml` includes 5 weapon types: Dagger, 1hSword, 1hAxe, 2hAxe, Mace. Bare hands NOT allowed.

3. **Power Thrust weapon break: Axes/Maces immune** — From `status_change.txt`: "except Axes, Maces & Unbreakable weapons". Blacksmiths are mostly immune to their own break.

4. **Maximize Power blocks SP regen** — Confirmed by rAthena commit 4771d05. SP recovery is disabled while toggle is active.

5. **Party PT ATK: flat 5% all levels (pre-renewal)** — The scaling 5/5/10/10/15% table found on RateMyServer is Renewal only. Pre-renewal is flat 5%.

6. **Party AR ASPD: 20% (pre-renewal)** — iRO Wiki's "25%" figure is Renewal. Pre-renewal party bonus is 20%.

7. **Skin Tempering: 4%/lv is correct for our game** — rAthena code uses 5%/lv but iRO Wiki Classic says actual tested value is 4%/lv. Known rAthena deviation from official servers.

8. **Forge Lv2 penalty is -20%, not -10%** — Our formula undercharges by 10% for Lv2 and Lv3 weapons.

9. **Refine +10 rates: 20%/10%, not 19%/9%** — The 19%/9% values appear to be from rateMyServer's Renewal display.

10. **Adrenaline Rush has NO HIT bonus in pre-renewal** — The HIT bonus (+8/+11/+14/+17/+20) only exists under `#ifdef RENEWAL` in rAthena.
