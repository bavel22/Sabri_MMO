# Merchant Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14 (research), 2026-03-15 (implementation)
**Status:** IMPLEMENTATION COMPLETE
**Scope:** All 10 Merchant class skills (IDs 600-609)

---

## Executive Summary

Deep research against iRO Wiki, iRO Wiki Classic, rAthena pre-renewal database (pre.pservero.com), and RateMyServer reveals that **4 of 10 Merchant skills are fully correct** (Enlarge Weight Limit, Discount, Overcharge, Loud Exclamation), **3 have minor issues** (Mammonite has a spurious cooldown, Cart Revolution has wrong targetType and missing cart weight scaling + force-hit, Pushcart prerequisite is correct but effectValue meaning unclear), and **3 are stubbed/deferred** (Vending, Item Appraisal, Change Cart).

**Key findings:**
1. **Mammonite** has a `cooldown: 800` that does not exist in RO Classic -- pre-renewal Mammonite has NO cooldown and NO after-cast delay (entirely ASPD-based)
2. **Cart Revolution** is implemented as `targetType: 'ground'` but should be `targetType: 'single'` with 3x3 splash around the target. The damage formula is missing cart weight scaling (`150% + 100 * CartWeight / 8000`). It should also force-hit (ignore FLEE). Has a `cooldown: 500` that should be 0. Tree position collides with Item Appraisal.
3. **Loud Exclamation** is functionally correct but uses the name "Loud Exclamation" instead of the canonical "Crazy Uproar" -- both names appear in official sources, but "Crazy Uproar" is the iRO Wiki standard name.
4. **Vending, Item Appraisal, Change Cart** are all stubbed with error messages -- these require entire new systems (player shops, unidentified items, cart visuals) and should remain deferred.

**New systems needed:**
1. Cart weight tracking for Cart Revolution damage scaling (requires Pushcart/cart system implementation)
2. Force-hit flag support in `calculateSkillDamage` (skip HIT/FLEE check)

---

## Skill-by-Skill Analysis

---

### 1. ENLARGE WEIGHT LIMIT (ID 600) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| Weight Bonus | +200 per level (+200 to +2000) | iRO Wiki, rAthena pre-re, RateMyServer |
| Formula | `MaxWeight += 200 * SkillLevel` | All sources |
| Prerequisites | None | All sources |
| Unlocks | Discount (Lv3), Pushcart (Lv5) | All sources |

#### Current Implementation Status: CORRECT

- Weight bonus: **CORRECT** -- `effectValue: 200*(i+1)` gives +200/+400/.../+2000
- Applied in `getPlayerMaxWeight()`: **CORRECT** -- reads skill level, adds `lvlData.effectValue` to base weight
- Prerequisites: **CORRECT** -- none required
- Type: **CORRECT** -- `type: 'passive'`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| None | -- | -- |

**No changes needed.**

---

### 2. DISCOUNT (ID 601) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| Discount % | Lv1: 7%, Lv2: 9%, Lv3: 11%, Lv4: 13%, Lv5: 15%, Lv6: 17%, Lv7: 19%, Lv8: 21%, Lv9: 23%, Lv10: 24% | iRO Wiki, rAthena pre-re, RateMyServer |
| Pattern | +2% per level (Lv1-9), +1% at Lv10 (diminishing return) | All sources |
| Prerequisites | Enlarge Weight Limit Lv3 | All sources |
| Rounding | Floor (minimum 1z) | RateMyServer |
| Scope | NPC shop buy prices only (not player vending, not deals) | RateMyServer, iRO Wiki |

#### Current Implementation Status: CORRECT

- Discount percentages: **CORRECT** -- `effectValue: [7,9,11,13,15,17,19,21,23,24][i]`
- Applied via `getDiscountPercent()` and `applyDiscount()`: **CORRECT** -- `Math.floor(basePrice * (100 - discountPct) / 100)`
- Used in `shop:data` and `shop:buy_batch`: **CORRECT**
- Prerequisites: **CORRECT** -- `{ skillId: 600, level: 3 }` (Enlarge Weight Limit Lv3)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing minimum price of 1z floor | VERY LOW | Trivial -- `Math.max(1, ...)` in `applyDiscount()` |

#### Implementation Notes

The minimum 1z floor is almost never hit in practice (would require an item priced at 1z with 24% discount = 0.76z -> 0z). Low priority but trivial to add.

---

### 3. OVERCHARGE (ID 602) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| Overcharge % | Lv1: 7%, Lv2: 9%, Lv3: 11%, Lv4: 13%, Lv5: 15%, Lv6: 17%, Lv7: 19%, Lv8: 21%, Lv9: 23%, Lv10: 24% | iRO Wiki, rAthena pre-re, RateMyServer |
| Pattern | +2% per level (Lv1-9), +1% at Lv10 (diminishing return) | All sources |
| Prerequisites | Discount Lv3 | All sources |
| Rounding | Floor (minimum 0z) | RateMyServer |
| Scope | NPC shop sell prices only (not deals) | RateMyServer, iRO Wiki |

#### Current Implementation Status: CORRECT

- Overcharge percentages: **CORRECT** -- `effectValue: [7,9,11,13,15,17,19,21,23,24][i]`
- Applied via `getOverchargePercent()` and `applyOvercharge()`: **CORRECT** -- `Math.floor(basePrice * (100 + overchargePct) / 100)`
- Used in `shop:data` and `shop:sell_batch`: **CORRECT**
- Prerequisites: **CORRECT** -- `{ skillId: 601, level: 3 }` (Discount Lv3)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| None | -- | -- |

**No changes needed.**

---

### 4. MAMMONITE (ID 603) -- Active, Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, offensive, melee | All sources |
| Max Level | 10 | All sources |
| Target | Single enemy | All sources |
| Range | 1 cell (melee) | rAthena pre-re, RateMyServer |
| Element | Weapon property (uses equipped weapon's element) | RateMyServer, rAthena |
| SP Cost | 5 (all levels) | All sources |
| Cast Time | 0 (none) | iRO Wiki Classic, rAthena pre-re |
| After-Cast Delay | 0 (entirely ASPD-based) | iRO Wiki Classic, rAthena pre-re |
| Cooldown | 0 (none) | rAthena pre-re |
| Prerequisites | None | All sources |
| Interruptible | Yes (for cast-time skills; N/A since cast time = 0) | rAthena |

**ATK% and Zeny Cost per Level:**

| Level | ATK% | Zeny Cost |
|-------|------|-----------|
| 1 | 150% | 100z |
| 2 | 200% | 200z |
| 3 | 250% | 300z |
| 4 | 300% | 400z |
| 5 | 350% | 500z |
| 6 | 400% | 600z |
| 7 | 450% | 700z |
| 8 | 500% | 800z |
| 9 | 550% | 900z |
| 10 | 600% | 1000z |

Formula: `ATK% = 100 + 50 * SkillLevel`, `ZenyCost = 100 * SkillLevel`

**Notes:**
- Zeny cost can be reduced by 10% if the user learns Blacksmith skill Dubious Salesmanship (2nd class, deferred)
- Uses `executePhysicalSkillOnEnemy()` helper -- this correctly handles weapon element

#### Current Implementation Status: MOSTLY CORRECT

- ATK% scaling: **CORRECT** -- `effectValue: 150+i*50` gives 150/200/.../600
- Zeny cost: **CORRECT** -- `learnedLevel * 100` in handler
- SP cost: **CORRECT** -- `spCost: 5`
- Zeny deduction + DB persist: **CORRECT** -- handler deducts and persists to DB
- Uses `executePhysicalSkillOnEnemy()`: **CORRECT** -- handles damage calc, aggro, death
- Cast time: **CORRECT** -- `castTime: 0`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| `cooldown: 800` is wrong -- should be 0 (no cooldown in pre-renewal) | HIGH | Trivial -- remove `cooldown: 800` from levels |
| `afterCastDelay` not set (defaults to undefined/0) | OK | No change needed -- 0 is correct |
| Missing `afterCastDelay: 0` field explicitly in genLevels | VERY LOW | Cosmetic -- add for consistency |

#### Implementation Notes

The `cooldown: 800` is the only real bug. In pre-renewal RO, Mammonite has no cooldown and no after-cast delay -- the attack speed is entirely ASPD-based, meaning the player can spam Mammonite as fast as their attack speed allows (limited only by zeny). Remove `cooldown: 800` or set to 0.

**Fix:** In `ro_skill_data.js` line 92, change `cooldown: 800` to `cooldown: 0`.

---

### 5. PUSHCART (ID 604) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| Prerequisites | Enlarge Weight Limit Lv5 | All sources |
| Cart Weight | 8000 max weight, 100 item slots | iRO Wiki, RateMyServer |
| Speed Penalty | Base 45% penalty at Lv1, reduced by 5% per level | iRO Wiki, rAthena pre-re |
| Movement Speed | (50 + 5*SkillLv)% of normal speed | rAthena pre-re |
| Lv1 speed | 55% (45% penalty) | All sources |
| Lv10 speed | 100% (0% penalty) | All sources |
| Unlock | Vending (Lv3), Change Cart (quest only) | All sources |
| Cart Rental | Must rent cart from Kafra employee | iRO Wiki |

**Speed per Level:**

| Level | Move Speed % | Speed Penalty |
|-------|-------------|---------------|
| 1 | 55% | -45% |
| 2 | 60% | -40% |
| 3 | 65% | -35% |
| 4 | 70% | -30% |
| 5 | 75% | -25% |
| 6 | 80% | -20% |
| 7 | 85% | -15% |
| 8 | 90% | -10% |
| 9 | 95% | -5% |
| 10 | 100% | 0% |

#### Current Implementation Status: DEFINITION CORRECT, NO CART SYSTEM YET

- Prerequisites: **CORRECT** -- `{ skillId: 600, level: 5 }` (Enlarge Weight Limit Lv5)
- `effectValue: (i+1)*5` -- stores speed penalty reduction (5/10/.../50) -- **REASONABLE** as a data store
- Type: **CORRECT** -- `type: 'passive'`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| No cart system implemented (no cart storage, no cart weight tracking) | DEFERRED | Large -- entire cart inventory subsystem |
| No speed penalty applied when cart is equipped | DEFERRED | Medium -- requires movement speed modifiers |
| No cart rental from Kafra NPCs | DEFERRED | Medium -- requires Kafra service extension |

#### Implementation Notes

Pushcart requires an entire cart storage subsystem: separate inventory (100 slots, 8000 weight), Kafra rental service, movement speed modifier system, and cart weight tracking (needed for Cart Revolution damage scaling). This is a major feature that should be implemented as its own development phase. The skill definition is correct as-is for when the system is built.

---

### 6. VENDING (ID 605) -- Active, Self -- DEFERRED

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self | All sources |
| Max Level | 10 | All sources |
| SP Cost | 30 (all levels) | All sources |
| Cast Time | 0 | rAthena pre-re |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Prerequisites | Pushcart Lv3 | All sources |
| Required State | Must have a pushcart equipped | rAthena pre-re |
| NPC Distance | Must be 4+ cells from any NPC | iRO Wiki |

**Max Vending Slots per Level:**

| Level | Slots |
|-------|-------|
| 1 | 3 |
| 2 | 4 |
| 3 | 5 |
| 4 | 6 |
| 5 | 7 |
| 6 | 8 |
| 7 | 9 |
| 8 | 10 |
| 9 | 11 |
| 10 | 12 |

Formula: `Slots = 2 + SkillLevel`

**Restrictions:**
- Only items in pushcart can be sold
- Max item price: 1,000,000,000z (servers may cap lower)
- 5% commission on items priced above 10,000,000z (iRO-specific)
- Shop closes when all items sold or character dies

#### Current Implementation Status: STUB

- Definition: **CORRECT** -- `effectValue: i+3` gives 3/4/.../12 slots
- SP cost: **CORRECT** -- 30
- Prerequisites: **CORRECT** -- Pushcart Lv3
- Handler: **STUB** -- returns "not yet implemented" error

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Entire vending system not implemented | DEFERRED | Very Large -- player shop system, shop browsing, purchase protocol |
| Requires cart system first | DEFERRED | Depends on Pushcart system |

#### Implementation Notes

Vending requires: (1) Pushcart/cart inventory system, (2) player shop creation UI, (3) shop title/name entry, (4) shop browsing for other players, (5) purchase Socket.io events, (6) shop closing/revenue collection. This is a Phase 9+ feature per the master build plan. The skill definition is correct; keep the stub.

---

### 7. ITEM APPRAISAL (ID 606) -- Active, Self -- DEFERRED

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self | All sources |
| Max Level | 1 | All sources |
| SP Cost | 10 | All sources |
| Cast Time | 0 | rAthena pre-re |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Prerequisites | None | All sources |
| Effect | Identifies an unidentified item in inventory | All sources |
| Alternative | Magnifier item (consumable, same effect) | iRO Wiki |

**How Identification Works in RO:**
- Equipment dropped by monsters may be "unidentified" -- showing only a generic name
- Using Item Appraisal or a Magnifier reveals the item's true stats, name, and card slots
- Unidentified items cannot be equipped or used
- Item must be in character inventory (not cart)

#### Current Implementation Status: STUB

- Definition: **CORRECT** -- SP 10, maxLevel 1
- Handler: **STUB** -- returns "not yet implemented" error

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| No unidentified item system exists | DEFERRED | Large -- requires item identification flag, drop system changes, Magnifier item |
| No skill handler | DEFERRED | Small (once identification system exists) |

#### Implementation Notes

Item Appraisal requires an entire item identification system: (1) `identified` boolean column on `character_inventory`, (2) monster drops sometimes drop unidentified, (3) unidentified items show generic names and can't be equipped, (4) Magnifier consumable item, (5) skill handler opens UI to select item. This is a Phase 8+ feature. The skill definition is correct; keep the stub.

**Tree position conflict:** Item Appraisal is at `treeRow: 1, treeCol: 1` which collides with Cart Revolution (608) at the same position. One of these needs to be moved. Recommended: Move Cart Revolution to `treeRow: 3, treeCol: 1` (new row for quest skills).

---

### 8. CHANGE CART (ID 607) -- Active, Self -- DEFERRED (was ID 607, uses Cart)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self (quest skill) | All sources |
| Max Level | 1 | All sources |
| SP Cost | 40 | All sources |
| Cast Time | 0 | rAthena pre-re |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Prerequisites | Quest completion (Merchant Skill Quest) | All sources |
| Required State | Must have pushcart equipped | rAthena pre-re |

**Cart Appearance by Base Level (Pre-Renewal):**

| Base Level | Cart Design |
|------------|-------------|
| 1-40 | Normal cart (default, no Change Cart needed) |
| 41-65 | Wooden cart |
| 66-80 | Flower/fern covered cart |
| 81-90 | Panda doll cart |
| 91-99 | Big wheels + roof + banner cart |

**Quest Requirements (Merchant Skill Quest):**
- NPC: Charlron in Alberta (119, 221)
- Items: 20 Animal Skin, 10 Iron, 50 Trunk

#### Current Implementation Status: STUB

- Definition: **CORRECT** -- SP 40, maxLevel 1
- Handler: **STUB** -- returns "not yet implemented" error
- Prerequisites: **CORRECT** -- none in skill tree (quest-learned)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| No cart visual system | DEFERRED | Large -- requires cart mesh/model system, level-based selection |
| Requires cart system first | DEFERRED | Depends on Pushcart system |
| No quest system for skill learning | DEFERRED | Medium -- requires quest framework |

#### Implementation Notes

Change Cart requires: (1) multiple cart mesh/model assets, (2) cart visual attachment to player character, (3) base level-based cart selection logic, (4) quest system for skill learning. This is a cosmetic/QoL feature with low gameplay priority. The skill definition is correct; keep the stub.

---

### 9. CART REVOLUTION (ID 608) -- Active, Single Target + Splash

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, offensive, melee (quest skill) | All sources |
| Max Level | 1 | All sources |
| Target | Single enemy (with 3x3 splash around target) | iRO Wiki, rAthena pre-re |
| Range | 1 cell (melee) | rAthena pre-re |
| Element | Weapon property | rAthena pre-re, RateMyServer |
| SP Cost | 12 | All sources |
| Cast Time | 0 | rAthena pre-re |
| After-Cast Delay | 0 (ASPD-based) | iRO Wiki Classic, rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Knockback | 2 cells | All sources |
| AoE | 3x3 cells around target (splash radius 1) | All sources |
| Force Hit | Yes -- ignores FLEE/accuracy check | iRO Wiki, RateMyServer |
| Required State | Must have pushcart equipped | rAthena pre-re |
| Prerequisites | Quest completion (Merchant Skill Quest) | All sources |
| Interruptible | Yes (N/A since cast time = 0) | rAthena |

**Damage Formula:**

```
Damage (ATK) = [150 + floor(100 * CurrentCartWeight / MaxCartWeight)]%
```

- Base damage: 150% ATK
- Cart weight bonus: 0% to 100% (scales linearly with cart weight)
- MaxCartWeight: 8000 (fixed)
- Empty cart: 150% ATK
- Full cart (8000/8000): 250% ATK
- RateMyServer note: "1% per 80 weight" (equivalent: 8000/80 = 100% max bonus)

**Quest Requirements (Merchant Skill Quest):**
- NPC: Gershaun in Alberta (232, 106)
- Items: ~20 Iron, ~30 Sticky Mucus, ~20 Fly Wing, ~5 Tentacle, 2+ Grape Juice, 1+ Banana Juice
- Alternative: Free upon job change at Alberta Merchant Guild

#### Current Implementation Status: PARTIALLY CORRECT, MULTIPLE ISSUES

- SP cost: **CORRECT** -- 12
- Knockback: **CORRECT** -- 2 cells implemented in handler
- AoE damage: **CORRECT** -- hits all enemies in radius
- Death processing: **CORRECT** -- calls `processEnemyDeathFromSkill()`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| `targetType: 'ground'` is WRONG -- should be `'single'` with splash | HIGH | Medium -- change targetType, update handler to target enemy with splash |
| `cooldown: 500` is WRONG -- should be 0 (no cooldown in pre-renewal) | HIGH | Trivial -- set `cooldown: 0` |
| Damage formula uses flat 150% -- missing cart weight bonus (`+ 100 * CartWeight / 8000`) | HIGH | Medium -- requires cart weight tracking |
| Missing force-hit flag (should ignore FLEE/accuracy check) | MEDIUM | Small -- add `forceHit: true` to `calculateSkillDamage` options |
| Tree position collision with Item Appraisal (both at treeRow:1, treeCol:1) | HIGH | Trivial -- move to different grid position |
| Handler doesn't check if player has cart equipped | DEFERRED | Small -- requires cart system |
| No quest skill learning system | DEFERRED | Medium -- requires quest framework |

#### Implementation Notes

**Critical fix (targetType):** Cart Revolution should target a single enemy (like Bash), but deal 3x3 splash damage around that target. Currently it's implemented as a ground-target AoE, which means the player clicks a ground position rather than an enemy. The handler should be restructured to:
1. Receive `targetId` (single enemy target)
2. Calculate damage on the primary target
3. Find all other enemies within 3x3 splash radius of the primary target
4. Apply damage to splash targets too
5. Apply knockback to all hit targets (2 cells away from attacker)

**Force-hit:** Cart Revolution ignores the accuracy check (always hits). The `calculateSkillDamage` function needs a `forceHit` option that skips the HIT/FLEE roll. Other skills that force-hit (like some 2nd class skills) will benefit from this.

**Cart weight scaling:** Until the cart system is implemented, the damage formula should use 150% as a baseline (empty cart). When cart inventory is implemented, add the weight bonus: `effectVal = 150 + Math.floor(100 * (player.cartWeight || 0) / 8000)`.

**Recommended tree position fix:** Move Cart Revolution to `treeRow: 3, treeCol: 0` or `treeRow: 3, treeCol: 1` to avoid collision with Item Appraisal at (1,1).

---

### 10. LOUD EXCLAMATION / CRAZY UPROAR (ID 609) -- Active, Self Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self buff (quest skill) | All sources |
| Max Level | 1 | All sources |
| SP Cost | 8 | All sources |
| Cast Time | 0 (pre-renewal) | rAthena pre-re |
| After-Cast Delay | 0 (pre-renewal) | rAthena pre-re |
| Cooldown | 0 (pre-renewal) | rAthena pre-re |
| Duration | 300 seconds (5 minutes) | All sources |
| Effect | +4 STR (self only in pre-renewal) | iRO Wiki Classic, rAthena pre-re, RateMyServer |
| ATK Bonus | None in pre-renewal (+30 ATK was added in renewal) | RateMyServer (confirmed renewal-only) |
| Target | Self only (party-wide was added in renewal) | rAthena pre-re |
| Dispelled by | Quagmire | iRO Wiki |
| Prerequisites | Quest completion (Merchant Skill Quest) | All sources |

**Quest Requirements (Merchant Skill Quest):**
- NPC: Necko in Alberta (83, 96)
- Items: 7 Pearl, 50 Mushroom Spore, 1 Banana Juice

**Naming:**
- Official iRO Wiki name: "Crazy Uproar"
- Alternative name used in some sources: "Loud Exclamation"
- rAthena internal: `MC_LOUD`
- Our project uses: "Loud Exclamation" (acceptable but non-standard)

#### Current Implementation Status: CORRECT

- SP cost: **CORRECT** -- 8
- STR bonus: **CORRECT** -- `effectValue: 4`
- Duration: **CORRECT** -- `duration: 300000` (5 minutes)
- Self-buff: **CORRECT** -- applies to `player` only
- Buff system: **CORRECT** -- `applyBuff(player, { strBonus: effectVal, ... })`
- `strBonus` integrates with `getBuffStatModifiers()` and `getEffectiveStats()`: **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Display name "Loud Exclamation" instead of canonical "Crazy Uproar" | VERY LOW | Trivial -- rename `displayName` |
| No Quagmire dispel interaction | DEFERRED | Requires Quagmire implementation |
| No quest skill learning system | DEFERRED | Requires quest framework |

#### Implementation Notes

The skill is functionally correct for pre-renewal. The only actionable item is the display name, which is cosmetic. The buff correctly applies +4 STR via `strBonus` in the buff system, which is picked up by `getEffectiveStats()` and `buildFullStatsPayload()`. No ATK bonus should be added -- that's a renewal-only change.

---

## New Systems Required

### 1. Force-Hit Flag for `calculateSkillDamage()` (Priority: MEDIUM)

Cart Revolution ignores FLEE (always hits). Add a `forceHit` option to the damage calculation system:

```js
// In calculateSkillDamage options:
{ forceHit: true }  // Skip HIT/FLEE roll, always hit

// In the function:
if (!options.forceHit) {
    // Normal HIT/FLEE check
} else {
    // Force hit -- skip accuracy roll, cannot miss
}
```

Other skills that will use this: Grimtooth (Assassin), some 2nd class skills.

### 2. Cart Inventory System (Priority: DEFERRED -- Phase 8+)

Required for: Pushcart storage, Cart Revolution damage scaling, Vending, Change Cart.

Components:
- `cart_inventory` database table (separate from `character_inventory`)
- Cart weight tracking (`player.cartWeight`, `player.maxCartWeight = 8000`)
- Cart item CRUD Socket.io events
- Cart UI (Alt+W hotkey)
- Kafra cart rental service
- Movement speed modifier when cart equipped

### 3. Item Identification System (Priority: DEFERRED -- Phase 8+)

Required for: Item Appraisal.

Components:
- `identified` boolean column on `character_inventory`
- Monster drops sometimes drop unidentified items
- Unidentified items show generic names, can't be equipped
- Magnifier consumable item (ID 611)
- Item Appraisal skill handler opens item selection UI

### 4. Quest Skill Learning System (Priority: DEFERRED -- Phase 9+)

Required for: Cart Revolution, Change Cart, Crazy Uproar quest-based learning.

The 3 Merchant quest skills (Cart Revolution, Change Cart, Crazy Uproar) are currently learnable from the skill tree (no prerequisites), which is a common simplification on private servers. In official RO, they require completing specific quests. This can wait for the quest system implementation.

---

## Skill Definition Corrections

### Immediate Fixes (No new systems needed)

1. **Mammonite (603):** Remove spurious `cooldown: 800` -- set to `cooldown: 0`
2. **Cart Revolution (608):**
   - Change `targetType: 'ground'` to `targetType: 'single'`
   - Change `cooldown: 500` to `cooldown: 0`
   - Fix `treeRow: 1, treeCol: 1` collision -- move to `treeRow: 3, treeCol: 1`
3. **Loud Exclamation (609):** Optionally rename `displayName` to "Crazy Uproar"

### Corrected Skill Definitions

```js
// Mammonite -- remove cooldown
{ id: 603, ..., levels: genLevels(10, i => ({ level: i+1, spCost: 5, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: 150+i*50, duration: 0 })) },

// Cart Revolution -- fix targetType, cooldown, tree position
{ id: 608, name: 'cart_revolution', displayName: 'Cart Revolution', classId: 'merchant', maxLevel: 1, type: 'active', targetType: 'single', element: 'neutral', range: 150, description: 'AoE attack with cart. Damage scales with cart weight. Knockback 2. Quest skill.', icon: 'cart_revolution', treeRow: 3, treeCol: 1, prerequisites: [], levels: [{ level: 1, spCost: 12, castTime: 0, afterCastDelay: 0, cooldown: 0, effectValue: 150, duration: 0 }] },

// Loud Exclamation -- optional rename
{ id: 609, name: 'loud_exclamation', displayName: 'Crazy Uproar', ... },
```

### Handler Fixes

**Cart Revolution handler:** Needs rewrite from ground-target AoE to single-target + splash:

```js
if (skill.name === 'cart_revolution') {
    if (!targetId || !isEnemy) { socket.emit('skill:error', { message: 'No enemy target selected' }); return; }

    // Primary target damage (single target like Bash)
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Cart weight scaling: 150% + (100 * cartWeight / 8000)
    const cartWeight = player.cartWeight || 0;
    const cartBonus = Math.floor(100 * cartWeight / 8000);
    const totalEffectVal = effectVal + cartBonus; // 150 + cartBonus

    const crZone = player.zone || 'prontera_south';
    const attackerPos = await getPlayerPosition(characterId);

    // Find and damage primary target
    const primaryTarget = enemies.get(targetId);
    if (!primaryTarget || primaryTarget.isDead) return;

    // Force-hit: skip HIT/FLEE check
    const result = calculateSkillDamage(
        getEffectiveStats(player), primaryTarget.stats, primaryTarget.hardDef || 0,
        totalEffectVal, getBuffStatModifiers(player), getCombinedModifiers(primaryTarget),
        getEnemyTargetInfo(primaryTarget), getAttackerInfo(player),
        { skillElement: null, forceHit: true }
    );

    // Apply to primary + splash (3x3 around primary target)
    const splashRadius = 150; // 3x3 cells = ~150 UE units
    const targetsHit = [];

    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.zone !== crZone) continue;
        const dx = primaryTarget.x - enemy.x;
        const dy = primaryTarget.y - enemy.y;
        const dist = Math.sqrt(dx*dx + dy*dy);
        if (eid === targetId || dist <= splashRadius) {
            // Calculate damage for this target
            // Apply knockback 2 cells away from attacker
            // Broadcast skill:effect_damage
            // Check death
            targetsHit.push(eid);
        }
    }

    socket.emit('skill:used', { ... });
    socket.emit('combat:health_update', { ... });
    return;
}
```

---

## Implementation Priority

### Phase 1: Immediate Fixes (no new systems, ~30 min)

| Task | Skill | Type | Effort |
|------|-------|------|--------|
| Remove `cooldown: 800` from Mammonite | 603 | Definition fix | 1 min |
| Fix Cart Revolution `targetType` to `'single'` | 608 | Definition fix | 1 min |
| Fix Cart Revolution `cooldown: 500` to `0` | 608 | Definition fix | 1 min |
| Fix Cart Revolution tree position collision | 608 | Definition fix | 1 min |
| Rewrite Cart Revolution handler (single-target + splash) | 608 | Handler rewrite | 20 min |
| Add `forceHit` option to damage calc | -- | System enhancement | 10 min |
| Rename Loud Exclamation to Crazy Uproar (optional) | 609 | Definition fix | 1 min |
| Add minimum 1z floor to Discount | 601 | Trivial fix | 1 min |

### Phase 2: Cart Weight Scaling (requires cart system, deferred)

| Task | Skill | Type | Effort |
|------|-------|------|--------|
| Add `player.cartWeight` tracking | 604/608 | System | Medium |
| Cart Revolution damage scaling with cart weight | 608 | Handler update | Small |
| Cart equipped check for Cart Revolution | 608 | Validation | Small |

### Phase 3: Full Cart System (Phase 8+ of master plan)

| Task | Skill | Type | Effort |
|------|-------|------|--------|
| Cart inventory system (100 slots, 8000 weight) | 604 | Major system | Large |
| Kafra cart rental | 604 | System | Medium |
| Movement speed penalty | 604 | System | Medium |
| Vending/player shop system | 605 | Major system | Very Large |
| Change Cart visual system | 607 | System | Medium |
| Item identification system + Item Appraisal | 606 | Major system | Large |

---

## Sources

- [Mammonite - iRO Wiki](https://irowiki.org/wiki/Mammonite)
- [Mammonite - iRO Wiki Classic](https://irowiki.org/classic/Mammonite)
- [Mammonite - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=42)
- [Mammonite - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_MAMMONITE)
- [Cart Revolution - iRO Wiki](https://irowiki.org/wiki/Cart_Revolution)
- [Cart Revolution - iRO Wiki Classic](https://irowiki.org/classic/Cart_Revolution)
- [Cart Revolution - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=153)
- [Cart Revolution - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_CARTREVOLUTION)
- [Cart Revolution - rAthena Renewal DB](https://db.pservero.com/skill/MC_CARTREVOLUTION)
- [Enlarge Weight Limit - iRO Wiki](https://irowiki.org/wiki/Enlarge_Weight_Limit)
- [Enlarge Weight Limit - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=36)
- [Enlarge Weight Limit - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_INCCARRY)
- [Discount - iRO Wiki](https://irowiki.org/wiki/Discount)
- [Discount - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=37)
- [Discount - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_DISCOUNT)
- [Overcharge - iRO Wiki](https://irowiki.org/wiki/Overcharge)
- [Overcharge - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=38)
- [Overcharge - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_OVERCHARGE)
- [Pushcart - iRO Wiki](https://irowiki.org/wiki/Pushcart)
- [Pushcart - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=39)
- [Pushcart - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_PUSHCART)
- [Vending - iRO Wiki](https://irowiki.org/wiki/Vending)
- [Vending - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=41)
- [Vending - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_VENDING)
- [Item Appraisal - iRO Wiki](https://irowiki.org/wiki/Item_Appraisal)
- [Item Appraisal - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=40)
- [Item Appraisal - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_IDENTIFY)
- [Change Cart - iRO Wiki](https://irowiki.org/wiki/Change_Cart)
- [Change Cart - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=154)
- [Change Cart - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_CHANGECART)
- [Crazy Uproar - iRO Wiki](https://irowiki.org/wiki/Crazy_Uproar)
- [Crazy Uproar - iRO Wiki Classic](https://irowiki.org/classic/Crazy_Uproar)
- [Crazy Uproar - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=155)
- [Crazy Uproar - rAthena Pre-Re DB](https://pre.pservero.com/skill/MC_LOUD)
- [Merchant - iRO Wiki](https://irowiki.org/wiki/Merchant)
- [Merchant Skill Quest - iRO Wiki](https://irowiki.org/wiki/Merchant_Skill_Quest)
- [Merchant Quest Skill Guide - RateMyServer](https://ratemyserver.net/quest_db.php?type=50000&qid=50006)
