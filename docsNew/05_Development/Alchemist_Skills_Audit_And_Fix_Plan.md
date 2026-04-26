# Alchemist Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Alchemist_Class_Research](Alchemist_Class_Research.md)
> **Status**: RESOLVED 2026-04-26 — All 13 original audit fixes confirmed + 2 newly-discovered bugs fixed.

**Date:** 2026-03-16
**Status:** ALL 13 FIXES APPLIED (7 initial + 6 from deep research)
**Scope:** All 16 Alchemist skills (IDs 1800-1815) audited against RO Classic pre-renewal behavior
**Sources:** rAthena pre-renewal `skill_db.yml` + `skill_tree.yml` (authoritative), iRO Wiki Classic, RateMyServer, Divine Pride
**Verification:** All catalyst item IDs verified against rAthena canonical DB (`pservero.com/item/`)

---

## Audit Summary

| Category | Count |
|----------|-------|
| Skills Audited | 16 (IDs 1800-1815) |
| Bugs Found (Data Definition) | 3 |
| Bugs Found (Catalyst Item IDs) | 3 (CRITICAL — wrong items consumed) |
| Bugs Found (Handler Logic) | 5 |
| Bugs Found (Cross-System) | 2 |
| Known Deferred Items | 8 |

## Deep Research Sources (rAthena Authoritative)

All values below verified against `github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml` and `skill_tree.yml`:

### Canonical Catalyst Item IDs (rAthena)

| Item | rAthena ID | rAthena Internal Name | Old (Wrong) ID | Used By |
|------|-----------|----------------------|----------------|---------|
| Bottle Grenade | **7135** | Fire_Bottle | Was 7136 | Demonstration |
| Acid Bottle | **7136** | Acid_Bottle | Was 7135 | Acid Terror |
| Plant Bottle | **7137** | Plant_Bottle | N/A | Summon Flora (deferred) |
| Marine Sphere Bottle | **7138** | Marine_Sphere_Bottle | N/A | Summon Marine Sphere (deferred) |
| Glistening Coat | **7139** | Coating_Bottle | Was 7137 | Chemical Protection x4 |
| Embryo | **7142** | Germination_Breed | 7142 (correct) | Call Homunculus |

### rAthena Acid Terror Flags (AM_ACIDTERROR)
- `IgnoreDefense: true` — confirms ignoreHardDef
- `IgnoreFlee: true` — confirms forceHit
- `IgnoreAtkCard: true` — %ATK cards don't apply
- `IgnoreAutoGuard: true` — bypasses Auto Guard
- `Duration2: 120000` — bleeding lasts 120s (was 60s in our code)

### rAthena Demonstration Flags (AM_DEMONSTRATION)
- `Unit/Interval: 1000` — 1s tick rate (was 500ms in our code)
- `NoReiteration: true` — cannot stack
- `NoFootSet: true` — cannot place under caster/target
- `Element: Fire`
- `ItemCost: Fire_Bottle` (7135)

### rAthena Chemical Protection (AM_CP_HELM/SHIELD/ARMOR/WEAPON)
- All use `Coating_Bottle` (7139)
- CP Helm SP: 25 (pre-renewal), 20 (Renewal — our code had Renewal value)
- iRO Wiki Classic Dispell page: CP skills listed as **UNDISPELLABLE**

---

## 1. BUGS — Data Definitions (`ro_skill_data_2nd.js`)

### BUG 1: Demonstration (1802) — Wrong Prerequisite

**File:** `server/src/ro_skill_data_2nd.js` line 260
**Current:** `prerequisites: [{ skillId: 1800, level: 2 }]` (Pharmacy Lv2)
**Correct:** `prerequisites: [{ skillId: 1800, level: 4 }]` (Pharmacy Lv4)
**Source:** rAthena pre-renewal `skill_tree.txt`: `AM_DEMONSTRATION: AM_PHARMACY 4`, iRO Wiki Classic, RateMyServer
**Impact:** Players can learn Demonstration 2 Pharmacy levels earlier than intended. Affects skill progression balance.

### BUG 2: Demonstration (1802) — Wrong effectVal (Damage 20% Too Low)

**File:** `server/src/ro_skill_data_2nd.js` line 260
**Current:** `effectValue: 100+i*20` → Lv1-5: 100/120/140/160/180
**Correct:** `effectValue: 120+i*20` → Lv1-5: 120/140/160/180/200
**Source:** iRO Wiki Classic formula: `ATK% = 100 + 20 * SkillLevel`, research doc Section 4.5
**Impact:** Demonstration deals 20% less damage per tick at every level. At Lv5, the tick damage is 180% instead of the correct 200% — a significant 10% relative DPS loss.

---

## 2. BUGS — Handler Logic (`server/src/index.js`)

### BUG 3: Acid Terror (1801) — Missing Pneuma Check

**File:** `server/src/index.js` lines 14706-14823
**Issue:** Acid Terror is a ranged physical skill (range 900 UE) but does not check if the target enemy is standing inside a Pneuma ground effect zone. In RO Classic, Pneuma blocks ALL ranged physical damage — including skill damage.
**RO Classic Behavior:**
- Pneuma blocks Acid Terror's DAMAGE (set to 0)
- Armor break and bleeding status effects STILL APPLY even when Pneuma blocks damage
- Source: iRO Wiki Classic — "Pneuma blocks damage, armor break still applies"

**Fix:** Add Pneuma check after damage calculation, before applying damage. If Pneuma blocks, set damage to 0 but continue to armor break + bleeding logic.
```js
// After atDamage calculation (after boss halve, before Lex Aeterna)
const pneumaEffects = getGroundEffectsAtPosition(enemy.x, enemy.y, enemy.z || 0, 100);
const inPneuma = pneumaEffects.find(e => e.type === 'pneuma');
if (inPneuma) {
    atDamage = 0; // Pneuma blocks ranged physical damage, armor break/bleed still apply
}
```

**Note:** This is actually a systemic issue — other ranged skills (Double Strafe, Arrow Shower, etc.) may also lack Pneuma checks in their handlers. The auto-attack combat tick has Pneuma checks, but individual skill handlers do not. However, for this audit we fix Acid Terror specifically.

### BUG 4: Demonstration Tick — Missing Lex Aeterna Consumption

**File:** `server/src/index.js` lines 19792-19858
**Issue:** The Demonstration ground effect tick handler does not check for Lex Aeterna on the target enemy. In RO Classic, Lex Aeterna doubles the first instance of damage that hits the target, regardless of source. The first Demonstration tick should consume Lex Aeterna and deal double damage.
**RO Classic Behavior:** Lex Aeterna doubles any damage source. Ground effect ticks are damage instances.

**Fix:** Add Lex Aeterna check inside the per-enemy damage loop in the Demonstration tick handler:
```js
// After demoDmg calculation, before applying to enemy.health
let finalDemoDmg = demoDmg;
if (finalDemoDmg > 0 && enemy.activeBuffs) {
    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
    if (lexBuff) {
        finalDemoDmg *= 2;
        removeBuff(enemy, 'lex_aeterna');
        broadcastToZone(effect.zone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
    }
}
```

---

## 3. MINOR DISCREPANCIES (Low Priority)

### DISCREPANCY 1: Acid Terror Armor Break Duration (30s vs Permanent)

**File:** `server/src/index.js` line 14778
**Current:** `duration: 30000` (30 seconds)
**RO Classic:** Against monsters, armor break is effectively permanent until death. Against players, it persists until equipment is repaired.
**Assessment:** 30s is a reasonable PvE simplification. Monsters typically die within 30s anyway. Changing to 300s (5 min) or `999999` (until death) would be more accurate but has low gameplay impact.
**Recommendation:** Change to 120000 (120s / 2 minutes) as a more generous but still bounded duration.

### DISCREPANCY 2: Potion Pitcher — No Target VIT Scaling

**File:** `server/src/index.js` lines 14888-14969
**Issue:** In RO Classic, the target's VIT stat increases potion healing effectiveness. The current Potion Pitcher formula: `floor(baseHeal * effectiveness / 100) * (100 + potResBonus) / 100` does not include a VIT multiplier.
**RO Classic Formula (full):** `HealAmount = BasePotionValue * Effectiveness * (1 + TargetVIT/100) * (1 + PotionResearch * 0.05)`
**Assessment:** Currently self-only (ally targeting deferred). The self-VIT bonus would increase heal amounts by ~50-100% for high-VIT builds. This affects balance.
**Recommendation:** Add VIT factor: `* (100 + targetVIT) / 100` where targetVIT is the target's effective VIT.

### DISCREPANCY 3: Potion Research — Not Applied to Self-Use Potions (inventory:use)

**Issue:** When an Alchemist uses a potion from their inventory via `inventory:use`, the Potion Research `potionHealBonus` is not applied. Only Potion Pitcher healing gets the bonus.
**RO Classic:** Potion Research bonus applies to ALL potion consumption by the Alchemist, not just Potion Pitcher.
**Assessment:** This is a known deferred item. Requires modifying the `inventory:use` potion handler to check for Potion Research passive.
**Recommendation:** Wire `potionHealBonus` into the `inventory:use` potion handler.

---

## 4. SKILL-BY-SKILL AUDIT RESULTS

### 4.1 Pharmacy (1800) — DEFERRED (Stub)
- **Status:** Returns "not yet implemented" error
- **Data Definition:** CORRECT (SP 5, castTime 0, effectVal 3*lv, prereq: Potion Research Lv5)
- **Deferred Because:** Requires crafting UI, recipe database, creation guide system, Medicine Bowl consumption
- **No action needed** for this audit

### 4.2 Acid Terror (1801) — 2 ISSUES FOUND
- **Data Definition:** CORRECT
  - effectVal: 140+i*40 → 140/180/220/260/300 (matches `100 + 40*SkillLevel`)
  - SP 15, castTime 1000ms, ACD 500ms, CD 0, range 900
  - `forceHit: true`, `ignoreHardDef: true` — CORRECT
  - Prereq: Pharmacy Lv5 — CORRECT
- **Handler:** 2 issues
  - **BUG 3**: Missing Pneuma check (see Section 2)
  - **DISCREPANCY 1**: Armor break 30s duration (see Section 3)
- **Verified CORRECT:**
  - forceHit + ignoreHardDef (hardDef=0 in calculateSkillDamage) ✓
  - Boss 50% damage ✓
  - Lex Aeterna consumption ✓
  - Non-linear armor break chances [3,7,10,12,13%] ✓
  - Bleeding 3%/lv with 60s duration ✓
  - statusImmune check on armor break and bleeding ✓
  - Chemical Protection Armor blocks armor break ✓
  - Acid Bottle (7135) catalyst via SKILL_CATALYSTS ✓
  - cardNoGemStone bypass ✓
  - Inventory update after catalyst consumption ✓
  - Zone broadcast, death processing ✓
  - checkDamageBreakStatuses ✓
  - Element: neutral ✓

### 4.3 Demonstration (1802) — 2 BUGS FOUND
- **Data Definition:** 2 BUGS
  - **BUG 1**: Prereq Pharmacy Lv2 → should be Lv4
  - **BUG 2**: effectVal 100+i*20 → should be 120+i*20
  - Duration: 40000+i*5000 → 40/45/50/55/60s — CORRECT
  - SP 10, castTime 1000ms, ACD 500ms, range 900, element fire — all CORRECT
- **Handler (creation):** CORRECT
  - Bottle Grenade (7136) catalyst ✓
  - Removes existing Demonstration from same caster ✓
  - Ground effect with radius 150 (3x3), tickInterval 500ms ✓
  - Zone broadcast ✓
  - Inventory update after catalyst consumption ✓
- **Handler (tick loop, lines 19792-19858):** 1 BUG
  - **BUG 4**: Missing Lex Aeterna consumption (see Section 2)
  - Zone filtering ✓
  - Fire element in calculateSkillDamage ✓
  - Weapon break chance per tick (1-5%) ✓
  - Chemical Protection Weapon blocks weapon break ✓
  - statusImmune check ✓
  - checkDamageBreakStatuses ✓
  - Death processing ✓
  - Zone broadcast per tick ✓

### 4.4 Summon Flora (1803) — DEFERRED (Stub)
- **Status:** Returns "not yet implemented" error
- **Data Definition:** CORRECT (SP 20, castTime 2000ms, range 400, prereq: Pharmacy Lv6)
- **Deferred Because:** Requires summon entity system
- **No action needed** for this audit

### 4.5 Axe Mastery (1804) — CORRECT
- **Data Definition:** CORRECT (effectVal 3*lv, passive, no prereqs)
- **Passive Handler (line 687-694):** CORRECT
  - +3 ATK/lv with axes ✓
  - Weapon type check: `['axe', 'one_hand_axe', 'two_hand_axe']` ✓
  - Stored as `bonuses.bonusATK` (same pattern as all other mastery skills in codebase) ✓
- **Note:** In RO Classic, mastery ATK bypasses both hard and soft DEF. Our codebase uses `bonusATK` for ALL weapon masteries (Sword Mastery, Axe Mastery, etc.), which is applied before DEF. This is a systemic design decision consistent across all classes — NOT an Alchemist-specific bug.

### 4.6 Potion Research (1805) — CORRECT
- **Data Definition:** CORRECT (effectVal 5*lv, passive, no prereqs)
- **Passive Handler (lines 696-701):** CORRECT
  - `potionHealBonus`: +5%/lv (0-50%) ✓
  - `brewRateBonus`: +1%/lv (0-10%) ✓
- **Usage:** `potionHealBonus` applied in Potion Pitcher handler (line 14930) ✓
- **Gap:** NOT applied in `inventory:use` potion handler (DISCREPANCY 3)

### 4.7 Potion Pitcher (1806) — 1 DISCREPANCY
- **Data Definition:** CORRECT
  - SP 1, castTime 500ms, ACD 500ms, range 900
  - effectVal: 110+i*10 → 110/120/130/140/150 ✓
  - Prereq: Pharmacy Lv3 ✓
- **Handler (lines 14888-14969):** 1 DISCREPANCY
  - **DISCREPANCY 2**: Missing target VIT scaling (see Section 3)
  - Potion types per level: Red/Orange/Yellow/White/Blue ✓
  - Potion item IDs: 501/502/503/504/505 ✓
  - Heal ranges per potion type ✓
  - Effectiveness multiplier from effectVal ✓
  - Potion Research bonus applied ✓
  - Potion consumed from inventory ✓
  - Weight update after consumption ✓
  - Lv5 SP recovery (Blue Potion) ✓
  - Self-only targeting (ally targeting deferred until party system) — expected limitation

### 4.8 Summon Marine Sphere (1807) — DEFERRED (Stub)
- **Status:** Returns "not yet implemented" error
- **Data Definition:** CORRECT (SP 10, castTime 2000ms, range 100, prereq: Pharmacy Lv2)
- **Deferred Because:** Requires summon entity system
- **No action needed** for this audit

### 4.9-4.12 Chemical Protection Helm/Shield/Armor/Weapon (1808-1811) — ALL CORRECT
- **Data Definitions:** ALL CORRECT
  - CP Helm (1808): SP 20, castTime 2000ms, range 150, prereq Pharmacy Lv2 ✓
  - CP Shield (1809): SP 25, castTime 2000ms, range 150, prereq CP Helm Lv3 ✓
  - CP Armor (1810): SP 25, castTime 2000ms, range 150, prereq CP Shield Lv3 ✓
  - CP Weapon (1811): SP 30, castTime 2000ms, range 150, prereq CP Armor Lv3 ✓
  - Duration: 120s * lv for all four ✓
- **Handler (lines 14972-15020):** ALL CORRECT
  - Glistening Coat (7137) catalyst ✓
  - cardNoGemStone bypass ✓
  - Slot protection mapping: headgear/shield/armor/weapon ✓
  - preventBreak + preventStrip flags on buff ✓
  - Duration formula: `learnedLevel * 120000` ✓
  - Zone broadcast with buff effects ✓
  - Inventory update after catalyst consumption ✓
  - Weight update ✓
- **Integration Points Verified:**
  - CP Armor blocks Acid Terror armor break (line 14777) ✓
  - CP Weapon blocks Demonstration weapon break (line 19844) ✓
  - CP skills block Rogue Divest (verified in Phase 5B memory) ✓
  - Self-only targeting (ally targeting deferred) — expected limitation

### 4.13 Bioethics (1812) — CORRECT
- **Data Definition:** CORRECT (passive, questSkill: true, no prereqs)
- **No handler needed** (gate check for homunculus skills)

### 4.14 Call Homunculus (1813) — CORRECT
- **Data Definition:** CORRECT (SP 10, castTime 0, prereq Bioethics+Rest)
- **Handler (lines 15030-15158):** CORRECT
  - Job class restriction (HOMUNCULUS_CLASSES) ✓
  - Max one homunculus check ✓
  - First summon: Embryo (7142) consumed ✓
  - Random type selection (25% each: lif/amistr/filir/vanilmirth) ✓
  - Re-summon: checks is_alive, loads from DB ✓
  - Dead homunculus: returns error (must Resurrect first) ✓
  - DB insertion with full stats ✓
  - `activeHomunculi` runtime map ✓
  - Zone broadcasts (homunculus:summoned, homunculus:other_summoned) ✓
  - Inventory update for Embryo consumption ✓

### 4.15 Rest / Vaporize (1814) — CORRECT
- **Data Definition:** CORRECT (SP 50, castTime 0, prereq Bioethics Lv1)
- **Handler (lines 15160-15194):** CORRECT
  - Homunculus summoned check ✓
  - HP >= 80% requirement ✓
  - Full state persistence to DB (20 fields) ✓
  - `activeHomunculi` cleanup ✓
  - Zone broadcasts (homunculus:vaporized, homunculus:other_dismissed) ✓
  - Intimacy NOT reduced ✓

### 4.16 Resurrect Homunculus (1815) — CORRECT
- **Data Definition:** CORRECT
  - SP: 80-6*(i+1) → 74/68/62/56/50 (decreasing) ✓
  - Cooldown: (170-30*(i+1))*1000 → 140s/110s/80s/50s/20s (decreasing) ✓
  - CastTime: 3000ms ✓
  - effectVal: 20+i*20 → 20/40/60/80/100% HP restore ✓
  - Prereq: Call Homunculus Lv1 ✓
- **Handler (lines 15197-15231):** CORRECT
  - Job class restriction ✓
  - Already-summoned check ✓
  - DB query for dead homunculus (is_alive = false) ✓
  - HP restoration: `floor(hp_max * learnedLevel * 20 / 100)` ✓
  - Sets is_alive=true, is_summoned=false (must Call separately) ✓
  - System chat message with restoration info ✓

---

## 5. PREREQUISITES AUDIT — ALL CORRECT

| Skill | Expected Prereq | Current | Status |
|-------|-----------------|---------|--------|
| Pharmacy (1800) | Potion Research Lv5 | `[{ skillId: 1805, level: 5 }]` | CORRECT |
| Acid Terror (1801) | Pharmacy Lv5 | `[{ skillId: 1800, level: 5 }]` | CORRECT |
| Demonstration (1802) | Pharmacy Lv4 | `[{ skillId: 1800, level: 2 }]` | **BUG 1 — FIX TO Lv4** |
| Summon Flora (1803) | Pharmacy Lv6 | `[{ skillId: 1800, level: 6 }]` | CORRECT |
| Axe Mastery (1804) | None | `[]` | CORRECT |
| Potion Research (1805) | None | `[]` | CORRECT |
| Potion Pitcher (1806) | Pharmacy Lv3 | `[{ skillId: 1800, level: 3 }]` | CORRECT |
| Summon Marine Sphere (1807) | Pharmacy Lv2 | `[{ skillId: 1800, level: 2 }]` | CORRECT |
| CP Helm (1808) | Pharmacy Lv2 | `[{ skillId: 1800, level: 2 }]` | CORRECT |
| CP Shield (1809) | CP Helm Lv3 | `[{ skillId: 1808, level: 3 }]` | CORRECT |
| CP Armor (1810) | CP Shield Lv3 | `[{ skillId: 1809, level: 3 }]` | CORRECT |
| CP Weapon (1811) | CP Armor Lv3 | `[{ skillId: 1810, level: 3 }]` | CORRECT |
| Bioethics (1812) | None (quest) | `[]` | CORRECT |
| Call Homunculus (1813) | Bioethics + Rest | `[{ 1812: 1 }, { 1814: 1 }]` | CORRECT |
| Rest (1814) | Bioethics Lv1 | `[{ skillId: 1812, level: 1 }]` | CORRECT |
| Resurrect Homunculus (1815) | Call Homunculus Lv1 | `[{ skillId: 1813, level: 1 }]` | CORRECT |

**15/16 correct. 1 bug (Demonstration Lv2→Lv4).**

---

## 6. CATALYST SYSTEM AUDIT — ALL CORRECT

| Skill | Catalyst | Item ID | In SKILL_CATALYSTS | Generic Pre-Check |
|-------|----------|---------|-------------------|-------------------|
| Acid Terror | Acid Bottle | 7135 | YES | YES (line 6285) |
| Demonstration | Bottle Grenade | 7136 | YES | YES |
| CP Helm | Glistening Coat | 7137 | YES | YES |
| CP Shield | Glistening Coat | 7137 | YES | YES |
| CP Armor | Glistening Coat | 7137 | YES | YES |
| CP Weapon | Glistening Coat | 7137 | YES | YES |
| Call Homunculus (first) | Embryo | 7142 | NO (inline check) | YES (inline) |

**All catalysts verified. Generic pre-check at line 6285 verifies catalyst exists BEFORE SP deduction.**

---

## 7. PASSIVE INTEGRATION AUDIT — CORRECT

### Axe Mastery (1804) in `getPassiveSkillBonuses()` (lines 687-694)
- Weapon type check includes all axe variants ✓
- Fallback to `player.weaponType` if no `equippedWeaponRight` ✓
- +3 ATK per level ✓
- Uses `bonusATK` (consistent with ALL other mastery skills) ✓

### Potion Research (1805) in `getPassiveSkillBonuses()` (lines 696-701)
- `potionHealBonus`: +5%/lv (correctly used in Potion Pitcher) ✓
- `brewRateBonus`: +1%/lv (ready for Pharmacy system when implemented) ✓

---

## 8. CROSS-SYSTEM INTEGRATION AUDIT

| Integration Point | Status | Details |
|-------------------|--------|---------|
| Lex Aeterna on Acid Terror | CORRECT | Checked + consumed (line 14754) |
| Lex Aeterna on Demonstration | **BUG 4** | Not checked in tick handler |
| CP Armor blocks Acid Terror armor break | CORRECT | `hasBuff(enemy, 'chemical_protection_armor')` (line 14777) |
| CP Weapon blocks Demo weapon break | CORRECT | `hasBuff(enemy, 'chemical_protection_weapon')` (line 19844) |
| CP blocks Rogue Divest | CORRECT | Verified in Phase 5B implementation |
| Acid Terror aggro | CORRECT | `setEnemyAggro(enemy, characterId, 'skill')` (line 14765) |
| Demo aggro | CORRECT | `setEnemyAggro(enemy, effect.casterId, 'skill')` (line 19825) |
| Acid Terror death processing | CORRECT | `processEnemyDeathFromSkill()` (line 14812) |
| Demo death processing | CORRECT | `processEnemyDeathFromSkill()` (line 19855) |
| Demo zone filtering | CORRECT | `enemy.zone !== effect.zone` check (line 19803) |
| Homunculus EXP sharing | CORRECT | 10% in combat tick (verified in memory) |
| Homunculus persist on disconnect | CORRECT | Full state save (verified in memory) |
| Homunculus hunger tick | CORRECT | 60s decay, starvation death (verified in memory) |
| Weight update after catalyst consumption | CORRECT | All handlers update currentWeight |
| Inventory refresh after catalyst consumption | CORRECT | All handlers emit inventory:data |

---

## 9. FIX EXECUTION PLAN

### Fix 1: Demonstration Prerequisite (ro_skill_data_2nd.js)
**Change:** `prerequisites: [{ skillId: 1800, level: 2 }]` → `prerequisites: [{ skillId: 1800, level: 4 }]`
**Location:** Line 260, `id: 1802`
**Risk:** Low — only affects skill learning. Existing characters who already learned Demonstration with Pharmacy <Lv4 keep it.

### Fix 2: Demonstration effectVal (ro_skill_data_2nd.js)
**Change:** `effectValue: 100+i*20` → `effectValue: 120+i*20`
**Location:** Line 260, `id: 1802`
**Risk:** Low — increases Demonstration tick damage by ~11-20% depending on level (100→120 is +20%, 180→200 is +11%).

### Fix 3: Acid Terror Pneuma Check (index.js)
**Location:** After line 14750 (boss damage halve), before Lex Aeterna check
**Change:** Add `getGroundEffectsAtPosition()` Pneuma check. If enemy in Pneuma, set damage to 0 but continue to armor break + bleeding.
**Risk:** Low — only affects Acid Terror against enemies standing in Pneuma zones (rare scenario, important for RO accuracy).

### Fix 4: Demonstration Lex Aeterna (index.js)
**Location:** Line 19821, after `demoDmg` calculation, before `enemy.health` update
**Change:** Add Lex Aeterna check + consume + double damage.
**Risk:** Low — only affects enemies with Lex Aeterna standing in Demonstration zones.

### Fix 5 (Optional): Acid Terror Armor Break Duration
**Location:** Line 14778
**Change:** `duration: 30000` → `duration: 120000` (120s)
**Risk:** Very low — makes armor break last 4x longer on monsters.

### Fix 6 (Optional): Potion Pitcher VIT Scaling
**Location:** Line 14931
**Change:** Add `* (100 + targetVIT) / 100` to heal formula
**Risk:** Low — increases self-heal amounts based on VIT stat.

### Fix 7 (Optional): Potion Research in inventory:use
**Location:** `inventory:use` potion handler (separate section in index.js)
**Change:** Look up `getPassiveSkillBonuses(player).potionHealBonus` and multiply potion heal amounts
**Risk:** Low — increases Alchemist self-potion healing.

---

## 10. KNOWN DEFERRED ITEMS (NOT Bugs)

| Item | Skills | Why Deferred | When to Implement |
|------|--------|-------------|-------------------|
| Pharmacy crafting system | 1800 | Needs crafting UI, recipe DB, creation guides | Phase 8+ |
| Summon Flora | 1803 | Needs summon entity system | Phase 9+ |
| Summon Marine Sphere | 1807 | Needs summon entity system | Phase 9+ |
| Ally targeting | 1806, 1808-1811 | Needs party system | Phase 7+ |
| PvP interactions | All combat skills | Needs PvP system | Phase 10+ |
| Homunculus evolution | 1812+ | Needs Stone of Sage, evolved forms | Phase 10+ |
| Homunculus skill casting | All 4 types | Needs homunculus skill execution engine | Phase 10+ |
| Homunculus client actor | 1813 | Needs UE5 companion actor, HP bar, follow AI | Phase 10+ |

---

## RESOLUTION LOG (2026-04-26)

End-to-end re-audit confirmed all 13 audit fixes are present in code. Re-audit also surfaced 2 new bugs that the original audit missed (both broken since initial implementation).

### Confirmed fixed (audit items, with verification line numbers)

| Item | Where | Verified at |
|------|-------|-------------|
| BUG 1 Demonstration prereq Lv4 | `ro_skill_data_2nd.js:259` | `prerequisites: [{ skillId: 1800, level: 4 }]` ✓ |
| BUG 2 Demonstration ATK 120-200 | `ro_skill_data_2nd.js:259` | `effectValue: 120+i*20` ✓ |
| BUG 3 Acid Terror Pneuma check | `index.js:22619-22624` | `getGroundEffectsAtPosition` Pneuma check, sets damage=0, status applies anyway ✓ |
| BUG 4 Demonstration Lex Aeterna in tick | `index.js:30750-30758` | Per-target Lex Aeterna check + consume in ground tick ✓ |
| Catalyst IDs corrected | `index.js:394-399` SKILL_CATALYSTS | Acid Bottle=7136, Bottle Grenade=7135, Glistening Coat=7139 ✓ |
| Acid Terror bleeding 120s | `index.js:22663` | `duration: 120000` (was 60000) ✓ |
| Armor break 30s→120s | `index.js:22651` | `duration: 120000` ✓ |
| Demonstration tickInterval 1000ms | `index.js:22734` | Per rAthena Unit/Interval ✓ |
| CP Helm SP 25 (pre-renewal) | `ro_skill_data_2nd.js:265` | `spCost: 25` ✓ |
| Potion Pitcher VIT*2 + INT*2 | `index.js:22820-22822` | Hercules formula with stat multiplier ✓ |
| Inc HP Recovery in Potion Pitcher | `index.js:22823-22826` | `(100 + smRecoveryLv*10) / 100` ✓ |
| Potion Research in inventory:use | `index.js:24604-24607, 24618-24620` | Applies to both HP and SP potions ✓ |
| Pharmacy crafting | `index.js:22933+` | 14 recipes, success rate scaled to 0-100% ✓ |
| Summon Flora | `index.js:22988+` | 5 plant types, runtime registry, healer flag ✓ |
| Summon Marine Sphere | `index.js:23058+` | Max 3, 30s timer, fire 11x11 detonation ✓ |
| Homunculus system | `index.js:23108+`, `ro_homunculus_data.js`, `add_homunculus_table.sql` | 4 types, hunger/intimacy, EXP sharing, evolution, 8 skills ✓ |

### Newly-discovered bugs (FIXED 2026-04-26)

#### N1 — `armor_break` buff applied but never read

**File:** `server/src/index.js:22651` + `server/src/ro_buff_system.js`

Acid Terror's armor break chance roll succeeds and applies a buff named `armor_break` with `hardDefReduction: 100`, but `getBuffModifiers` had no `case 'armor_break'` — the buff data was silently ignored. Net effect: armor break did nothing despite the proc roll firing. Audit verified the chance roll but not the effect.

**Fix:**
1. Added `case 'armor_break'` to `ro_buff_system.js` — `mods.hardDefReduction += buff.hardDefReduction`
2. Changed Acid Terror handler to use the fraction format `hardDefReduction: 1.0` (1.0 = 100% reduction → enemy hard DEF effectively 0, matching RO Classic where armor break removes all armor DEF)

#### N2 — `weapon_break` buff applied but never read + wrong value format

**File:** `server/src/index.js:30782` + `server/src/ro_buff_system.js`

Same root cause as N1 for Demonstration tick. Additionally, the buff value was `atkReduction: 50`, which (had any handler existed) would have used the wrong scale — `strip_weapon` (the existing precedent) uses fraction format `0.25 = 25%`. A naive case addition would have done `1 - 50 = -49`, inverting damage.

**Fix:**
1. Added `case 'weapon_break'` to `ro_buff_system.js` — `mods.atkMultiplier *= (1 - buff.atkReduction)`
2. Changed Demonstration tick to use `atkReduction: 0.5` (0-1 fraction format, 50% ATK reduction on the affected entity)

### Files modified (this pass)

- `server/src/ro_buff_system.js` — Added `armor_break` + `weapon_break` cases
- `server/src/index.js` — Acid Terror buff value (1.0 fraction), Demonstration tick buff value (0.5 fraction)
- `docsNew/05_Development/Alchemist_Skills_Audit_And_Fix_Plan.md` — header + this resolution log

### Verification

- ✅ `node --check server/src/index.js` passes
- ✅ `node --check server/src/ro_buff_system.js` passes
- ✅ Both buffs now have handlers using the same fraction format pattern as `strip_weapon` / `strip_shield`
