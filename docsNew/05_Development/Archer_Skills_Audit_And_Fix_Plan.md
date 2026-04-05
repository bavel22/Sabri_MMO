# Archer Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14
**Updated:** 2026-03-15
**Status:** PHASES 1-2 IMPLEMENTED + Reveal Hidden (Phase 3 item 10). Remaining: items 11-14 deferred.
**Scope:** All 7 Archer class skills (IDs 300-306)

---

## Executive Summary

Deep research against iRO Wiki, iRO Wiki Classic, rAthena pre-renewal database (pre.pservero.com), and RateMyServer reveals **moderate gaps** in 5 of 7 Archer skills. Two passives (Owl's Eye and Vulture's Eye) are fully correct. The remaining 5 skills have issues ranging from critical formula errors (Improve Concentration applies percentage to total effective stats instead of base+job+equip only) to missing mechanics (Arrow Shower has no knockback, Arrow Crafting is completely unimplemented, Double Strafe incorrectly fires as 2 separate hits instead of 1 bundled damage instance).

**Critical finding -- Double Strafe hit structure:** iRO Wiki explicitly states "Despite the animation, all damage is connected in one single bundle." Our implementation fires 2 separate `skill:effect_damage` events with 200ms stagger. The displayed damage formula `ATK * X% x 2` means the total damage is `ATK * (2 * X%)` delivered as a single damage packet, NOT two independent hits. The "x2" in the formula describes total multiplier, not hit count. The rAthena pre-renewal DB confirms: total ATK at Lv10 = 380% (= 190% x 2), delivered in one packet.

**Critical finding -- Arrow Shower damage:** Our implementation uses the correct pre-renewal formula (80%-125%), NOT the Renewal formula (160%-250%). However, knockback is completely missing.

**New systems needed:**
1. Arrow Crafting item conversion system (70+ material-to-arrow recipes)
2. Arrow Shower knockback (2 cells away from ground target center)
3. Improve Concentration stat source filtering (base+job+equip only, exclude buffs/cards)
4. Quest skill acquisition system for Arrow Crafting and Arrow Repel

---

## Skill-by-Skill Analysis

---

### 1. OWL'S EYE (ID 300) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | iRO Wiki, rAthena, RateMyServer |
| Max Level | 10 | All sources |
| DEX Bonus | +1 per level (+1 to +10) | iRO Wiki, rAthena pre-re |
| Prerequisites | None | All sources |
| HIT bonus | None (implicit in DEX increase) | iRO Wiki |
| Weapon restriction | None (always active) | All sources |

#### Current Implementation Status: CORRECT

- DEX bonus: **CORRECT** -- `bonusDEX += oeLv` in `getPassiveSkillBonuses()` (line 434)
- effectValue in skill def: **CORRECT** -- `effectValue: i+1` (1-10)
- No weapon restriction: **CORRECT** -- applies regardless of weapon type
- Prerequisites: **CORRECT** -- empty array

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| None | -- | -- |

**No changes needed.**

---

### 2. VULTURE'S EYE (ID 301) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| Range Bonus | +1 cell per level (bow only) | iRO Wiki, rAthena pre-re |
| HIT Bonus | +1 flat HIT per level (bow only) | iRO Wiki, rAthena pre-re |
| Prerequisites | Owl's Eye Lv 3 | All sources |
| Weapon restriction | Bow class weapons only (both range and HIT) | iRO Wiki |
| HIT visibility | "Does not appear in the Status Window" (implicit) | iRO Wiki |

**Range conversion:** 1 RO cell = ~50 UE units. Our implementation uses `bonusRange += veLv * 10` which is 10 UE units per level. This should be 50 UE units per level to match 1 cell/level.

#### Current Implementation Status: RANGE VALUE WRONG

- HIT bonus: **CORRECT** -- `bonusHIT += veLv` (line 439, flat +1/level)
- Range bonus: **WRONG VALUE** -- `bonusRange += veLv * 10` gives 10 UE units/level; should be 50 UE units/level (1 cell = ~50 UE units)
- Bow-only restriction: **CORRECT** -- both bonuses gated by `if (wType === 'bow')`
- Prerequisites: **CORRECT** -- `[{ skillId: 300, level: 3 }]`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Range bonus too small: 10 UE/level instead of 50 UE/level | MEDIUM | Trivial -- change `veLv * 10` to `veLv * 50` |

#### Implementation Notes

Change in `getPassiveSkillBonuses()` at line 440:
```js
if (wType === 'bow') bonuses.bonusRange += veLv * 50;  // 1 cell = 50 UE units
```

Note: The base bow range in the skill definitions is 800 UE units (~16 cells). With Vulture's Eye 10, this becomes 800 + 500 = 1300 UE units (~26 cells). RO Classic bow base range is 9 cells + 10 from Vulture's Eye = 19 cells max. The base range value (800) may also need review, but that's a separate concern from the per-level scaling.

---

### 3. IMPROVE CONCENTRATION (ID 302) -- Active/Self Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self buff | All sources |
| Max Level | 10 (selectable) | All sources |
| SP Cost | 20 + 5*SkillLv (25-70) | iRO Wiki, rAthena pre-re, RateMyServer |
| Cast Time | 0 (instant) | iRO Wiki Classic, rAthena pre-re |
| After-Cast Delay | 0 | iRO Wiki Classic, rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Prerequisites | Vulture's Eye Lv 1 | All sources |

**AGI/DEX Bonus Table:**

| Level | Bonus % | Duration (sec) | SP Cost |
|-------|---------|----------------|---------|
| 1 | 3% | 60 | 25 |
| 2 | 4% | 80 | 30 |
| 3 | 5% | 100 | 35 |
| 4 | 6% | 120 | 40 |
| 5 | 7% | 140 | 45 |
| 6 | 8% | 160 | 50 |
| 7 | 9% | 180 | 55 |
| 8 | 10% | 200 | 60 |
| 9 | 11% | 220 | 65 |
| 10 | 12% | 240 | 70 |

Formula: Bonus% = (2 + SkillLv)%, Duration = (40 + 20*SkillLv) seconds

**Stat source filtering (CRITICAL):** The percentage applies ONLY to:
- Base AGI/DEX (stat points allocated by player)
- Job bonus AGI/DEX (from job class)
- Equipment base AGI/DEX (e.g., Tights, Diabolus Wing)
- Owl's Eye passive DEX bonus

The percentage does NOT apply to AGI/DEX from:
- Cards
- Enchants
- Other buffs (Blessing, Increase AGI, etc.)
- Item refine bonuses
- Item set bonuses

**Reveal hidden:** Detects hidden/cloaked characters within 3-cell radius (7x7 area) around caster on cast. One-time detection on activation, NOT persistent.

#### Current Implementation Status: FORMULA PARTIALLY WRONG

- SP cost formula: **CORRECT** -- `25+i*5` matches 20+5*(i+1) = 25+5*i
- Duration: **CORRECT** -- `60000+i*20000` matches (40+20*(i+1))*1000 = (60+20*i)*1000
- effectValue: **CORRECT** -- `3+i` matches (2+SkillLv) = (2+(i+1)) = 3+i
- Cast time: **CORRECT** -- 0
- Prerequisite: **CORRECT** -- Vulture's Eye Lv 1
- Buff application: **PARTIALLY WRONG** -- applies percentage to `getEffectiveStats(player).agi` and `.dex`, which includes ALL stat bonuses (cards, buffs, enchants, etc.)
- Reveal hidden: **NOT IMPLEMENTED** -- the handler broadcasts `revealHidden: true` in the buff_applied event effects but does NOT actually reveal any hidden enemies/players server-side

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Percentage applied to total stats instead of base+job+equip only** | HIGH | Medium -- need to calculate stat subset excluding buffs/cards |
| **Reveal hidden not implemented server-side** | MEDIUM | Medium -- need to find hidden players/enemies in 3-cell radius and remove their hidden status |
| Buff does not distinguish stat sources for the percentage | MEDIUM | Part of the stat subset fix above |

#### Implementation Notes

**Stat source filtering fix:**
The current code does:
```js
const stats = getEffectiveStats(player);
const agiBonus = Math.floor(effectVal * stats.agi / 100);
const dexBonus = Math.floor(effectVal * stats.dex / 100);
```

This incorrectly includes card bonuses, other buff bonuses, etc. in the base for the percentage calculation. The correct approach:
```js
// Base + job bonus + equipment base + Owl's Eye passive
const baseAgi = (player.stats.agi || 1) + (player.equipmentBonuses?.agi || 0);
const baseDex = (player.stats.dex || 1) + (player.equipmentBonuses?.dex || 0) + (passive.bonusDEX || 0);
const agiBonus = Math.floor(effectVal * baseAgi / 100);
const dexBonus = Math.floor(effectVal * baseDex / 100);
```

Note: `player.stats.agi` already includes job bonus from the DB. The key exclusion is card bonuses (`cardB.agi`), other buff bonuses (`buffMods.agiBonus`), and enchant bonuses. Equipment base stat bonuses may or may not be in `player.stats` vs `equipmentBonuses` -- needs verification.

**Simplified approach (recommended):** Since our stat system doesn't track per-source stat breakdowns in full detail, a practical simplification is to use `player.stats.X + equipmentBonuses.X + Owl's Eye` (excluding buff mods and card bonuses). This covers the most impactful exclusions (no double-dipping with Blessing/Inc AGI buffs).

**Reveal hidden implementation:**
On cast, iterate through all connected players and enemies in the same zone within 150 UE units (3 cells) of the caster. For any that have `isHidden === true`, remove their Hiding buff and broadcast `skill:buff_removed`.

---

### 4. DOUBLE STRAFE (ID 303) -- Active/Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, offensive, physical | All sources |
| Max Level | 10 (fixed at 10 on learn) | All sources |
| SP Cost | 12 (all levels) | iRO Wiki, rAthena pre-re, RateMyServer |
| Cast Time | 0 (instant) | rAthena pre-re: 0.00s, iRO Classic: "None" |
| After-Cast Delay | 0 | rAthena pre-re: 0.00s global delay |
| Cooldown | 0 | rAthena pre-re: 0.00s |
| Range | 9 cells base (+ Vulture's Eye bonus) | rAthena pre-re, iRO Classic: "+3 cells" |
| Element | Weapon (inherits bow/arrow element) | rAthena pre-re |
| Weapon | Bow required | All sources |
| Ammunition | 1 Arrow consumed per cast | All sources |
| Target | Single enemy | All sources |
| Prerequisites (Archer) | None | iRO Wiki, rAthena |
| Prerequisites (Rogue) | Vulture's Eye Lv 10 | rAthena pre-re |
| Critical | Cannot crit (standard for skills in pre-renewal) | rAthena general rule |

**Damage Table:**

| Level | Per-hit ATK% | Total ATK% | SP |
|-------|-------------|------------|-----|
| 1 | 100% | 200% | 12 |
| 2 | 110% | 220% | 12 |
| 3 | 120% | 240% | 12 |
| 4 | 130% | 260% | 12 |
| 5 | 140% | 280% | 12 |
| 6 | 150% | 300% | 12 |
| 7 | 160% | 320% | 12 |
| 8 | 170% | 340% | 12 |
| 9 | 180% | 360% | 12 |
| 10 | 190% | 380% | 12 |

Formula: Per-hit = (90 + 10*SkillLv)%, Total = 2 * Per-hit

**CRITICAL -- Hit structure:** iRO Wiki states: "Despite the animation, all damage is connected in **one single bundle**." This means the total damage (e.g., 380% at Lv10) is calculated as a single damage value and delivered as one damage packet. The visual shows two arrows but mechanically it is ONE hit. This is the standard behavior in rAthena's pre-renewal source code -- the "x2" in the formula means the total multiplier is doubled, not that there are two independent damage calculations.

However, the iRO Classic wiki says "Regular attack with two hits and increased damage" and the rAthena pre-re DB shows "200% to 380%" as total damage. The practical implementation in rAthena's `battle.cpp` treats it as one skill damage calculation with the total multiplier applied. The display shows as one damage number.

**Practical resolution:** Implement as 1 single damage calculation at the total multiplier (200-380%), displayed as 1 damage number. This matches iRO Wiki's "one single bundle" statement and is the most accurate pre-renewal behavior.

#### Current Implementation Status: HIT STRUCTURE WRONG

- SP cost: **CORRECT** -- 12 all levels
- Cast time: **CORRECT** -- 0
- Bow requirement: **CORRECT** -- checks `player.weaponType !== 'bow'`
- Range: **CORRECT** -- 800 UE units + bonusRange from Vulture's Eye
- effectValue: **CORRECT** -- `100+i*10` (100-190 per hit)
- **Hit structure: WRONG** -- fires 2 separate `skill:effect_damage` events with 200ms stagger (two independent damage calculations, each can miss/crit independently). Should be 1 bundled hit at total multiplier (200-380%)
- **Cooldown: WRONG** -- set to 300ms, should be 0 (no per-skill cooldown in rAthena)
- afterCastDelay: **MISSING** -- not set in skill definition (should be 0 per rAthena)
- Arrow consumption: **NOT IMPLEMENTED** -- no arrow deduction from inventory
- Element: **PARTIALLY CORRECT** -- hardcoded `skillElement: null` in handler; should inherit weapon/arrow element

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **2 separate hits instead of 1 bundled hit** | HIGH | Medium -- restructure handler to single damage calc at total multiplier |
| **Cooldown 300ms should be 0** | MEDIUM | Trivial -- remove `cooldown: 300` from skill definition |
| Arrow consumption not implemented | LOW | Deferred -- needs arrow/ammunition system |
| Element should inherit weapon/arrow | LOW | Easy -- pass weapon element to damage calc |

#### Implementation Notes

**Restructure handler to single hit:**
```js
if (skill.name === 'double_strafe') {
    // ... validation ...
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Single bundled hit at total multiplier (effectVal * 2)
    const totalMultiplier = effectVal * 2;  // 200% at Lv1, 380% at Lv10
    const result = calculateSkillDamage(
        getEffectiveStats(player), enemy.stats, enemy.hardDef || 0,
        totalMultiplier, atkBuffMods, defBuffMods,
        getEnemyTargetInfo(enemy), getAttackerInfo(player),
        { skillElement: null }  // TODO: inherit weapon/arrow element
    );

    if (!result.isMiss) {
        enemy.health = Math.max(0, enemy.health - result.damage);
        // ... aggro ...
    }

    // Single damage event (hitNumber: 1, totalHits: 1)
    broadcastToZone(zone, 'skill:effect_damage', {
        // ... standard fields ...
        hits: 1, hitNumber: 1, totalHits: 1
    });
    // ... death check, skill:used ...
}
```

Alternative approach: Keep `effectValue` as per-hit (100-190) and multiply by 2 in the handler. This preserves the original skill definition semantics.

**Skill definition fix:**
```js
{ id: 303, ..., levels: genLevels(10, i => ({
    level: i+1, spCost: 12, castTime: 0, afterCastDelay: 0,
    cooldown: 0,  // Was 300, should be 0
    effectValue: 100+i*10,  // Per-hit, multiply by 2 in handler for total
    duration: 0
})) }
```

---

### 5. ARROW SHOWER (ID 304) -- Active/Ground AoE

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, offensive, physical, ground AoE | All sources |
| Max Level | 10 | All sources |
| SP Cost | 15 (all levels) | iRO Wiki, rAthena pre-re, RateMyServer |
| Cast Time | 0 (instant) | iRO Classic: "None", rAthena pre-re: 0.00s |
| After-Cast Delay | 1 second | iRO Classic: "1 second" |
| Cooldown | 0 | rAthena pre-re: 0.00s |
| Range | 9 cells base + Vulture's Eye bonus (14 cells in iRO Classic) | iRO Classic, rAthena pre-re |
| AoE Area | **5x5 cells** (pre-renewal, constant all levels) | RateMyServer pre-re column, iRO Classic |
| Knockback | 2 cells, direction: away from ground target center | iRO Wiki, rAthena pre-re |
| Element | Weapon (inherits bow/arrow element) | rAthena pre-re |
| Weapon | Bow required | All sources |
| Ammunition | 1 Arrow consumed per cast | All sources |
| Target | Ground | All sources |
| Prerequisites | Double Strafe Lv 5 | All sources |

**IMPORTANT -- Pre-renewal vs Renewal AoE:**
- Pre-renewal: 5x5 at ALL levels
- Renewal: 3x3 at Lv1-5, 5x5 at Lv6-10
Our implementation should use pre-renewal (5x5 constant).

**Damage Table (Pre-Renewal):**

| Level | ATK% | SP |
|-------|------|-----|
| 1 | 80% | 15 |
| 2 | 85% | 15 |
| 3 | 90% | 15 |
| 4 | 95% | 15 |
| 5 | 100% | 15 |
| 6 | 105% | 15 |
| 7 | 110% | 15 |
| 8 | 115% | 15 |
| 9 | 120% | 15 |
| 10 | 125% | 15 |

Formula: (75 + 5*SkillLv)% -- confirmed by rAthena source code (`100 - 25 + 5 * skill_level`), RateMyServer pre-re column, and iRO Classic wiki.

**Knockback direction:** Enemies are pushed 2 cells away from the ground target position (NOT from the caster). If an enemy is standing on the exact target cell, they are pushed westward (default direction for same-cell knockback, matching `knockbackTarget()` behavior).

**Special properties:** Can hit cloaked/hidden targets. Can displace traps.

#### Current Implementation Status: MISSING KNOCKBACK

- Damage formula: **CORRECT** -- `effectValue: 80+i*5` matches (75+5*(i+1)) = (80+5*i)
- SP cost: **CORRECT** -- 15 all levels
- Cast time: **CORRECT** -- 0
- AoE radius: **PARTIALLY WRONG** -- uses `aoeRadius = 400` (8-cell radius). A 5x5 cell area has a radius of 2 cells from center = 100-125 UE units. 400 UE units is far too large (covers a ~16x16 cell area).
- Prerequisite: **CORRECT** -- Double Strafe Lv 5
- Bow requirement: **CORRECT** -- checks `player.weaponType !== 'bow'`
- **Knockback: NOT IMPLEMENTED** -- no knockback code in handler at all
- **Cooldown: WRONG** -- set to 1000ms per-skill cooldown; should be 0 cooldown, 1000ms after-cast delay instead
- afterCastDelay: **MISSING** -- not set in skill definition (should be ~1000ms)
- Arrow consumption: **NOT IMPLEMENTED** -- no arrow deduction
- Element: **PARTIALLY CORRECT** -- hardcoded `skillElement: null`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **No knockback (2 cells from ground target center)** | HIGH | Easy -- use `knockbackTarget(enemy, groundX, groundY, 2, zone, io)` |
| **AoE radius 400 is way too large (should be ~125)** | HIGH | Trivial -- change `aoeRadius` to 125 |
| **Cooldown 1000ms should be afterCastDelay 1000ms** | MEDIUM | Easy -- move from cooldown to afterCastDelay in skill def |
| Arrow consumption not implemented | LOW | Deferred -- needs arrow/ammunition system |
| Element should inherit weapon/arrow | LOW | Easy |

#### Implementation Notes

**Knockback fix:**
After applying damage to each enemy in the AoE, call the shared knockback function:
```js
// After damage application and death check
if (!result.isMiss && enemy.health > 0) {
    knockbackTarget(enemy, groundX, groundY, 2, asZone, io);
}
```

Note: `knockbackTarget()` already handles boss immunity and same-cell default-west direction, which perfectly matches RO Classic behavior.

**AoE radius fix:**
5x5 cell area = 2 cells from center = ~125 UE units radius (2.5 cells * 50 UE/cell):
```js
const aoeRadius = 125;  // 5x5 cells = 2-cell radius from center
```

**Skill definition fix:**
```js
{ id: 304, ..., levels: genLevels(10, i => ({
    level: i+1, spCost: 15, castTime: 0,
    afterCastDelay: 1000,  // 1 second global lockout
    cooldown: 0,            // Was 1000, should be afterCastDelay instead
    effectValue: 80+i*5, duration: 0
})) }
```

---

### 6. ARROW CRAFTING (ID 305) -- Active/Self (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self | All sources |
| Max Level | 1 | All sources |
| SP Cost | 10 | iRO Wiki, rAthena pre-re |
| Cast Time | 0 (instant) | rAthena pre-re: 0.00s |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Weight restriction | Cannot use if >= 50% weight | rAthena pre-re, RateMyServer |
| Acquisition | Quest skill (Archer Platinum Skill Quest) | All sources |
| Quest NPC | Roberto (moc_ruins 118/99) | iRO Wiki |
| Quest items | 7 Mushroom Spore, 1 Red Potion, 20 Resin, 13 Trunk, 41 Pointed Scale | iRO Wiki |
| Quest job level req | Job Level 30 | iRO Wiki |

**Arrow Conversion System:**
70+ items can be converted into 21 arrow types. Each item produces a specific arrow type in a specific quantity. The conversion opens a selection UI where the player chooses which inventory item to convert.

**Arrow Types Produced (21 types):**
1. Arrow (basic) -- from Tree Root, Jellopy, Cactus Needle, Trunk, etc.
2. Iron Arrow -- from Iron Ore, Garlet, Wolf Claw, Horn, etc.
3. Silver Arrow -- from Zargon, Emveretarcon, Fang, etc.
4. Steel Arrow -- from Scell, Orc Claw, Manacles, Steel, etc.
5. Fire Arrow -- from Red Blood, Flame Heart, Burning Heart, etc.
6. Crystal Arrow (Water) -- from Crystal Blue, Fang of Hatii, etc.
7. Stone Arrow (Earth) -- from Yellow Gem, Green Live, Mole Claw, etc.
8. Arrow of Wind -- from Wind of Verdure, Rough Wind, Cat's Eye, etc.
9. Frozen Arrow -- from Mystic Frozen, Ice Scale, etc.
10. Poison Arrow -- from Venom Canine, Stinky Scale, etc.
11. Cursed Arrow -- from Amulet, Dragon Skin, Mother's Nightmare, etc.
12. Sleep Arrow -- from Yellow Gem, Cursed Ruby, etc.
13. Mute Arrow (Silence) -- from Dead Branch, Dragon Skin, etc.
14. Stun Arrow -- from Elunium, Heroic Emblem, etc.
15. Flash Arrow (Blind) -- from Star Dust, Star Crumb, Gold, etc.
16. Shadow Arrow -- from Coal, Tooth of Bat, Dokebi Horn, etc.
17. Rusty Arrow -- from Scorpion Tail, Bee Sting, etc.
18. Sharp Arrow (critical bonus) -- from Mantis Scythe, Fang, Leopard Claw, etc.
19. Holy Arrow -- from Golden Ornament, Valhalla's Flower
20. Immaterial Arrow (Ghost) -- from Emperium, Piece of Shield, etc.
21. Oridecon Arrow -- from Rough Oridecon, Oridecon, Gold, etc.

Quantities range from 1 arrow (Bee Sting -> 1 Rusty Arrow) to 3,000 arrows (Matchstick -> 3,000 Fire Arrows).

#### Current Implementation Status: NOT IMPLEMENTED (STUB)

- Skill definition exists with correct basic properties (SP 10, max level 1, self target)
- Handler is a deferred stub: `socket.emit('skill:error', { message: 'Arrow Crafting is not yet implemented.' })`
- No item conversion logic exists
- No conversion table data file exists
- No client UI for material selection exists

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| **Entire skill not implemented** | LOW (requires ammunition system first) | Large -- needs conversion table, material selection UI, inventory integration |
| No weight threshold check (50%) | Part of implementation | Easy once skill exists |
| No quest acquisition system | LOW | Medium -- quest system not yet built |

#### Implementation Notes

**Arrow Crafting requires the following systems that don't yet exist:**
1. **Ammunition/Arrow system** -- arrows as equippable items in an ammo slot, consumed on ranged attacks and skills
2. **Item conversion recipe table** -- data file mapping `sourceItemId -> { arrowItemId, quantity }`
3. **Client selection UI** -- when skill is used, show list of convertible items in inventory, player picks one
4. **Weight check** -- reject if player is >= 50% weight capacity

**Recommended deferral:** This skill should be implemented as part of a broader "Ammunition System" feature that also handles:
- Arrow equipping (ammo slot)
- Arrow consumption on attacks
- Arrow element affecting skill/attack element
- Property arrows (elemental, status effect)

Without the ammunition system, Arrow Crafting arrows would have nowhere to go. Defer until ammunition system is built.

**Data file structure (for future):**
Create `server/src/ro_arrow_crafting.js`:
```js
const ARROW_CRAFTING_RECIPES = [
    { sourceItemId: 902, arrowItemId: 1750, quantity: 7 },    // Tree Root -> 7 Arrows
    { sourceItemId: 909, arrowItemId: 1750, quantity: 4 },    // Jellopy -> 4 Arrows
    // ... 70+ more entries
];
```

---

### 7. ARROW REPEL (ID 306) -- Active/Single Target (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, offensive, physical, single target | All sources |
| Max Level | 1 | All sources |
| SP Cost | 15 | iRO Wiki, rAthena pre-re, RateMyServer |
| Cast Time | 1.5 seconds | iRO Classic, rAthena pre-re, RateMyServer |
| Cast Interruptible | **Not interruptible** (iRO Classic) / Yes (rAthena pre-re) | Conflicting sources |
| After-Cast Delay | 0 | iRO Classic: "None", rAthena pre-re: 0.00s |
| Cooldown | 0 | rAthena pre-re: 0.00s |
| Range | 9 cells base + Vulture's Eye bonus | rAthena pre-re |
| Damage | 150% ATK | All sources (pre-renewal) |
| Knockback | **6 cells** (pre-renewal) | iRO Classic, RateMyServer pre-re |
| Element | Weapon (inherits bow/arrow element) | rAthena pre-re |
| Weapon | Bow required | All sources |
| Ammunition | 1 Arrow consumed per cast | All sources |
| Target | Single enemy | All sources |
| Acquisition | Quest skill (Archer Platinum Skill Quest) | All sources |
| Quest NPC | Jason (payon 103/63) | iRO Wiki |
| Quest items | 36 Banana Juice, 10 Bill of Bird, 2 Emerald, 10 Tentacle, 3 Yoyo Tail | iRO Wiki |
| Quest job level req | Job Level 35 | iRO Wiki |
| Boss immunity | Bosses immune to knockback | Standard RO mechanic |

**Cast interruptibility note:** iRO Classic says "Not interruptible" but rAthena pre-re marks it as interruptible. This is a known discrepancy between iRO behavior and private server emulators. For our implementation, we follow iRO Classic (not interruptible by damage).

#### Current Implementation Status: MOSTLY CORRECT, KNOCKBACK INLINE

- SP cost: **CORRECT** -- 15
- Cast time: **CORRECT** -- 1500ms (1.5 seconds)
- Damage: **CORRECT** -- effectValue 150 (150% ATK)
- Bow requirement: **CORRECT** -- checks `player.weaponType !== 'bow'`
- Range: **CORRECT** -- 800 + bonusRange
- Knockback distance: **WRONG** -- uses `knockDist = 250` UE units (2.5 cells). Should be 6 cells = 300 UE units (6 * 50). But more importantly, this uses inline knockback code instead of the shared `knockbackTarget()` function.
- Knockback direction: **CORRECT** -- pushes away from caster
- Boss immunity: **PARTIALLY CORRECT** -- checks `enemy.modeFlags?.knockbackImmune` but `knockbackTarget()` checks `modeFlags.isBoss`. The inline code may miss some boss flag names.
- **Cast interruptibility: WRONG** -- currently interruptible (standard behavior). iRO Classic says not interruptible. Need to add a `notInterruptible` flag.
- afterCastDelay in def: **CORRECT** -- set to 0
- Arrow consumption: **NOT IMPLEMENTED**
- Element: **PARTIALLY CORRECT** -- hardcoded `skillElement: null`
- Uses inline knockback instead of shared `knockbackTarget()` function

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Knockback distance 250 should be 300 (6 cells * 50 UE/cell) | MEDIUM | Trivial |
| Uses inline knockback instead of shared `knockbackTarget()` | MEDIUM | Easy -- refactor to use `knockbackTarget()` |
| Cast should not be interruptible by damage | LOW | Medium -- need `notInterruptible` skill flag + check in `interruptCast()` |
| Arrow consumption not implemented | LOW | Deferred |
| Element should inherit weapon/arrow | LOW | Easy |

#### Implementation Notes

**Knockback refactor:**
Replace the inline knockback code (lines 7540-7548) with:
```js
if (!result.isMiss && enemy.health > 0) {
    knockbackTarget(enemy, attackerPos.x, attackerPos.y, 6, arZone, io);
}
```

This automatically handles:
- Boss immunity (`modeFlags.isBoss`)
- Card immunity (`cardNoKnockback`)
- Same-position default direction (push west)
- `combat:knockback` broadcast event
- 6 cells * 100 UE units = 600 UE push distance

Note: `knockbackTarget()` uses `cellSize = 100` UE units per cell, while our range calculations use 50 UE units per cell. This is an existing discrepancy in the codebase. If we standardize on 50 UE/cell, the knockback function needs updating too. For now, using the existing function with `cells = 6` will push 600 UE units, which may or may not be the intended 6 RO cells depending on the project's cell-size convention.

**Cast interruption protection:**
Add a `notInterruptible: true` flag to the skill definition. Then in `interruptCast()`, check this flag before interrupting:
```js
function interruptCast(characterId, reason) {
    const cast = activeCasts.get(characterId);
    if (!cast) return;
    if (cast.skill?.notInterruptible && reason === 'damage') return;  // Arrow Repel, etc.
    // ... existing interrupt logic ...
}
```

---

## New Systems Required

### 1. Ammunition/Arrow System (DEFERRED)
**Needed by:** Arrow Crafting, Double Strafe (element), Arrow Shower (element), Arrow Repel (element)
**Scope:** Arrow equip slot, arrow consumption on ranged attacks/skills, arrow element inheritance
**Priority:** LOW -- all skills function without this (just no arrow consumption or element inheritance)

### 2. Arrow Crafting Conversion Table (DEFERRED)
**Needed by:** Arrow Crafting
**Scope:** Data file with 70+ material-to-arrow recipes, client selection UI, weight check
**Priority:** LOW -- depends on ammunition system

### 3. Quest Skill Acquisition System (DEFERRED)
**Needed by:** Arrow Crafting, Arrow Repel
**Scope:** NPC quest dialogs, item collection verification, skill granting
**Priority:** LOW -- both skills are already learnable (just not quest-gated)

### 4. Cast Interruption Protection Flag
**Needed by:** Arrow Repel
**Scope:** `notInterruptible` skill property, check in `interruptCast()`
**Priority:** LOW -- minor behavioral difference

---

## Skill Definition Corrections

### Double Strafe (ID 303)
```js
// Remove cooldown (was 300, should be 0)
levels: genLevels(10, i => ({
    level: i+1, spCost: 12, castTime: 0,
    afterCastDelay: 0, cooldown: 0,
    effectValue: 100+i*10, duration: 0
}))
```

### Arrow Shower (ID 304)
```js
// Move cooldown to afterCastDelay
levels: genLevels(10, i => ({
    level: i+1, spCost: 15, castTime: 0,
    afterCastDelay: 1000,  // Was cooldown, now ACD
    cooldown: 0,            // Was 1000, now 0
    effectValue: 80+i*5, duration: 0
}))
```

### Arrow Repel (ID 306)
```js
// No definition changes needed (already correct: castTime 1500, SP 15, effectValue 150)
// Optional: add notInterruptible flag if system supports it
```

---

## Implementation Priority

### Phase 1 -- Critical fixes (immediate) — DONE (2026-03-15)
1. ~~**Double Strafe: Convert to single bundled hit**~~ — DONE. `totalMultiplier = effectVal * 2`, single `calculateSkillDamage` + single `broadcastToZone`
2. ~~**Double Strafe: Remove cooldown**~~ — DONE. Skill def already had `cooldown: 0`
3. ~~**Arrow Shower: Fix AoE radius**~~ — DONE. Server 400→125, VFX 400→125, targeting circle 400→125
4. ~~**Arrow Shower: Add knockback**~~ — DONE. `knockbackTarget(enemy, groundX, groundY, 2, asZone, io)` — pushes away from ground center
5. ~~**Arrow Shower: Move cooldown to afterCastDelay**~~ — DONE. Skill def already had `afterCastDelay: 1000, cooldown: 0`

### Phase 2 -- Medium fixes — DONE (2026-03-15)
6. ~~**Vulture's Eye: Fix range scaling**~~ — DONE. `veLv * 50` (1 cell = 50 UE units)
7. ~~**Improve Concentration: Fix stat base for percentage**~~ — DONE. Uses `player.stats.X + equipB.X + passive.bonusDEX` (excludes cards/buffs)
8. ~~**Arrow Repel: Fix knockback distance**~~ — DONE. Uses `knockbackTarget(enemy, ..., 6, ...)` = 6 × 50 = 300 UE
9. ~~**Arrow Repel: Refactor to use shared `knockbackTarget()`**~~ — DONE. Replaced 8-line inline code

### Phase 3 -- Low priority / deferred
10. ~~**Improve Concentration: Implement reveal hidden**~~ — DONE (2026-03-15). Iterates `connectedPlayers` within 150 UE (3 cells), `removeBuff(p, 'hiding')` + broadcast
11. **Arrow Repel: Not-interruptible cast** -- DEFERRED, needs new skill flag system
12. **All bow skills: Weapon element inheritance** -- DEFERRED, needs arrow/ammunition system
13. **Arrow Crafting: Full implementation** -- DEFERRED, needs ammunition system
14. **Quest skill gating** -- DEFERRED, needs quest system

### Additional fixes (2026-03-15)
15. ~~**knockbackTarget() cellSize 100→50**~~ — DONE. Fixed to match codebase standard (50 UE/cell). Affects Magnum Break, Arrow Shower, Arrow Repel.
16. ~~**cardSplashRange cellSize 100→50**~~ — DONE. Baphomet Card splash now uses correct cell size.

---

## Sources

- [Owl's Eye - iRO Wiki](https://irowiki.org/wiki/Owl's_Eye)
- [Owl's Eye - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=43)
- [Vulture's Eye - iRO Wiki](https://irowiki.org/wiki/Vulture's_Eye)
- [Vulture's Eye - iRO Wiki Classic](https://irowiki.org/classic/Vulture's_Eye)
- [Vulture's Eye - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=44)
- [Vulture's Eye - rAthena Pre-Re](https://pre.pservero.com/skill/AC_VULTURE)
- [Improve Concentration - iRO Wiki](https://irowiki.org/wiki/Improve_Concentration)
- [Improve Concentration - iRO Wiki Classic](https://irowiki.org/classic/Improve_Concentration)
- [Improve Concentration - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=45)
- [Improve Concentration - rAthena Pre-Re](https://pre.pservero.com/skill/AC_CONCENTRATION)
- [Double Strafe - iRO Wiki](https://irowiki.org/wiki/Double_Strafe) -- "all damage is connected in one single bundle"
- [Double Strafe - iRO Wiki Classic](https://irowiki.org/classic/Double_Strafe)
- [Double Strafe - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=46)
- [Double Strafe - rAthena Pre-Re](https://pre.pservero.com/skill/AC_DOUBLE)
- [Double Strafe Formula Discussion - rAthena Forum](https://rathena.org/board/topic/124782-double-strafe-skill-formula/)
- [Arrow Shower - iRO Wiki](https://irowiki.org/wiki/Arrow_Shower)
- [Arrow Shower - iRO Wiki Classic](https://irowiki.org/classic/Arrow_Shower) -- 80-125% pre-renewal damage
- [Arrow Shower - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=47) -- pre-re vs renewal columns
- [Arrow Shower - rAthena Pre-Re](https://pre.pservero.com/skill/AC_SHOWER)
- [Arrow Crafting - iRO Wiki](https://irowiki.org/wiki/Arrow_Crafting)
- [Arrow Crafting - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=147)
- [Arrow Crafting - rAthena Pre-Re](https://pre.pservero.com/skill/AC_MAKINGARROW)
- [Arrow Crafting Conversion Table - RateMyServer](https://ratemyserver.net/index.php?op=1&page=creation_db)
- [Arrow Repel - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=148)
- [Arrow Repel - iRO Wiki Classic](https://irowiki.org/classic/Arrow_Repel) -- "Not interruptible", 6 cells
- [Arrow Repel - rAthena Pre-Re](https://pre.pservero.com/skill/AC_CHARGEARROW)
- [Archer Skill Quest - iRO Wiki](https://irowiki.org/wiki/Archer_Skill_Quest) -- quest requirements
