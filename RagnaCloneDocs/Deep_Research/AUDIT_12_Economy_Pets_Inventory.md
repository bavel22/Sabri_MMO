# Audit: Economy, Pets & Inventory

> **Scope**: Comparison of deep research docs (27, 28, 33, 38) against actual server implementation in `server/src/index.js` and supporting modules.
> **Date**: 2026-03-22
> **Verdict**: Core systems are substantially implemented. Trading system is entirely missing. Several RO Classic details are absent.

---

## Summary

| System | Research Coverage | Implementation Status | Gap Severity |
|--------|------------------|----------------------|-------------|
| Trading (P2P) | Complete (Sec 3) | **NOT IMPLEMENTED** | CRITICAL |
| Vending | Complete (Sec 4) | **Fully implemented** | Minor gaps |
| Cart | Complete (Sec 4.9) | **Fully implemented** | Minor gaps |
| Kafra Storage | Complete (Sec 7) | **NOT IMPLEMENTED** | CRITICAL |
| Weight System | Complete (Sec WS) | **Mostly implemented** | Medium gap |
| NPC Shops | Complete (Sec 2) | **Fully implemented** | Minor gaps |
| Pet System | Complete (full doc) | **Fully implemented** | Minor gaps |
| Homunculus | Complete (full doc) | **Mostly implemented** | Medium gaps |
| Inventory | Complete (doc 38) | **Fully implemented** | Negligible |
| Buying Store | Complete (Sec 5) | **NOT IMPLEMENTED** | Low (late-Classic) |
| Ground Items | Complete (Sec 6.3) | **NOT IMPLEMENTED** | Medium |
| Guild Storage | Complete (Sec 7.2) | **NOT IMPLEMENTED** | Low (needs guild first) |

### Implementation Breakdown
- **Fully Working**: Vending, Cart, NPC Shops (buy/sell/batch with Discount/Overcharge), Pet taming/feeding/bonuses/hunger, Homunculus core (summon/rest/resurrect/feed/evolve/combat), Inventory (100 slots, stacking, equip), Weight (formula, thresholds, events)
- **Entirely Missing**: Player-to-player trading, Kafra storage (personal), Guild storage, Buying Store, Ground item drop/pickup system
- **Partially Missing**: Job class weight bonuses, Gym Pass weight bonus, Vending tax, Pet accessories/performance emotes, Homunculus skill casting (stubbed), Homunculus position broadcast to other players

---

## Trading System

### Deep Research Says (Sec 3)
- Two-phase confirmation trade window (10 item slots per side + zeny field)
- State machine: NONE -> INITIATED -> ACCEPTED -> LOCKED -> COMMITTED
- Anti-scam protections: atomic transactions, item locking, weight validation, zeny overflow check, re-validation at commit
- Auto-cancel on distance, death, disconnect, vending, storage open
- Max zeny per trade: 999,999,999

### Actual Implementation
**COMPLETELY MISSING.** Zero trade-related socket handlers exist. No `trade:request`, `trade:add`, `trade:lock`, `trade:commit`, or `trade:cancel` events. No trade state machine. No trade window support on the server.

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Trade request/accept | Missing | CRITICAL |
| Trade window (add items/zeny) | Missing | CRITICAL |
| Two-phase lock/commit | Missing | CRITICAL |
| Atomic transaction | Missing | CRITICAL |
| Weight/zeny validation | Missing | CRITICAL |
| Anti-scam protections | Missing | CRITICAL |

**This is the single largest missing economy feature.** Player-to-player trading is fundamental to any MMO economy and is the primary mechanism for item exchange in RO Classic.

---

## Vending System

### Deep Research Says (Sec 4)
- Merchant-only, requires Pushcart Lv3+ and Vending skill (ID 605)
- Max item stacks: 2 + skill level (3-12)
- Items vended from cart only
- Shop title displayed above head (max ~80 chars)
- Character sits and is immobile while vending
- 5% tax on items priced > 100,000,000z per unit
- Auto-close when all items sold
- 4-cell minimum distance from NPCs

### Actual Implementation
**Fully implemented** at `server/src/index.js`:
- `vending:start` (line ~6964): Validates cart, Vending skill level, creates DB shop, inserts vending_items, broadcasts to zone
- `vending:close` (line ~7029): Cleans up shop, removes DB entries, broadcasts dismissal
- `vending:browse` (line ~7049): Returns shop items to buyer
- `vending:buy` (line ~7106): Full purchase flow with zeny/weight validation, DB updates, inventory refresh
- `vending:sold` notification to vendor with live stock updates
- Movement lock for vending players (position rejected with reason 'vending')
- Max stacks enforced: `2 + learnedLevel`
- Skill activation via skill handler (ID 605) emits `vending:setup` with cart items
- DB tables: `vending_shops` and `vending_items` (migration: `add_cart_vending_identify.sql`)
- Active vendors broadcast to joining players per zone
- Vendor self-view with live sale log and zeny updates

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| 5% vending tax on items > 100M zeny | **Missing** | Low (rare edge case) |
| 4-cell NPC distance check | **Missing** | Low (cosmetic) |
| Auto-close when all items sold | **Unknown** (may be handled client-side) | Low |
| Shop closing summary report | **Missing** | Low (QoL) |
| Whisper while vending | Not explicitly blocked | Negligible |

**Verdict**: Vending is well-implemented. The missing vending tax only matters for extremely high-value items (>100M per unit), which are rare in pre-renewal Classic.

---

## Cart System

### Deep Research Says (Sec 4.9, Doc 33)
- Merchant-class only, requires Pushcart skill (ID 604)
- 100 slots, 8,000 weight capacity
- Rental cost varies by town (600-1,200z)
- Pushcart speed penalty: -45% at Lv1 to 0% at Lv10
- Cart persists across zone changes, logouts, death
- Items persist even when cart is removed (re-rent restores items)
- Cart Revolution damage scales with cart weight
- Change Cart skill (cosmetic, Platinum Skill)
- Cart weight is separate from character weight

### Actual Implementation
**Fully implemented** at `server/src/index.js`:
- `cart:rent` (line ~6719): 800z flat fee, Merchant-class check, broadcasts `cart:equipped`
- `cart:remove` (line ~6760): Removes cart state but items persist in DB
- `cart:move_to_cart` (line ~6780): Weight/slot validation, DB insert, weight recalculation
- `cart:move_to_inventory` (line ~6868): Reverse transfer with weight check
- `cart:load` (line ~6710): Client re-request pattern for reconnection
- DB table: `character_cart` with full attribute preservation (refine, cards, forge)
- Cart state columns on characters table: `has_cart`, `cart_type`
- `cartWeight`, `cartMaxWeight` (8000), `cartItems` tracked per player
- Cart items loaded on `player:join`
- Cart Revolution uses `cartWeight` for damage scaling
- Vending blocks cart removal (`isVending` check)

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Variable rental cost per town | **Missing** (flat 800z) | Low |
| Pushcart speed penalty by level | **Partial** (referenced but not verified per-level) | Low |
| Change Cart visual selection UI | **Missing** (`cart_type` stored but no selection) | Low (cosmetic) |
| Cart Boost (Whitesmith transcendent) | **Missing** | N/A (transcendent) |
| Cart Termination (Whitesmith) | **Missing** | N/A (transcendent) |

**Verdict**: Cart system is fully functional for pre-renewal Classic. Missing features are minor (cosmetic or transcendent-class).

---

## Storage/Kafra

### Deep Research Says (Sec 7, Doc 33)
- Kafra Storage: 300 slots, no weight limit, 40z per access
- Account-shared (keyed by `user_id`, not `character_id`)
- All equipment attributes preserved (refine, cards, forge)
- Free Ticket for Kafra Storage (item 7059) bypasses fee
- Cannot access while trading, dead, or vending
- Guild Storage: 100-500 slots based on guild skill level
- Kafra save point (free), teleport service (600-3000z)

### Actual Implementation
**Kafra NPC framework exists** but storage is missing:
- `kafra:open` (line ~6515): Opens Kafra menu, sends available services
- `kafra:save` (line ~6553): Save point to DB (zone, coordinates) -- **working**
- `kafra:teleport` (line ~6584): Zone teleport with zeny cost -- **working**

**Storage is NOT implemented:**
- No `storage:open/close/deposit/withdraw` socket handlers
- No `character_storage` database table
- No `StorageSubsystem` or `SStorageWidget` on the client
- No account-level storage sharing logic

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Kafra Storage (300 slots) | **Missing** | CRITICAL |
| Account-shared storage | **Missing** | CRITICAL |
| Storage DB table | **Missing** | CRITICAL |
| 40z access fee | **Missing** | Low (depends on storage) |
| Free Ticket bypass | **Missing** | Low |
| Guild Storage | **Missing** (needs guild system) | Medium |
| Kafra Points | **Missing** | Low |
| Storage client UI | **Missing** | CRITICAL |

**Verdict**: Kafra save/teleport work. Storage is entirely unbuilt. This is the second-largest missing economy feature after trading.

---

## Weight System

### Deep Research Says (Doc 38, Doc 33)
- Formula: `2000 + (Base_STR * 30) + Job_Bonus + EWL + Mount`
- Job class bonuses: +0 (Novice) to +1000 (Blacksmith/Alchemist)
- Enlarge Weight Limit: +200/level (max +2000 at Lv10)
- Mount bonus: +1000 (Knight/Crusader on Peco)
- Gym Pass: +200/level (max +2000), available to ALL classes, stacks with Merchant EWL
- 50% threshold: natural HP/SP regen stops, creation skills disabled
- 90% threshold: cannot attack, use skills, or pick up items
- Cart weight separate from character weight

### Actual Implementation
**Mostly implemented** at `server/src/index.js`:
- `getPlayerMaxWeight()` (line ~4737): `2000 + str * 30 + EWL bonus + mount bonus(1000)`
- `INVENTORY.OVERWEIGHT_50` (0.5) and `OVERWEIGHT_90` (0.9) thresholds defined
- `getWeightRatio()` and `getWeightDetails()` helper functions
- `updatePlayerWeightCache()` emits `weight:status` when threshold boundaries crossed
- `calculatePlayerCurrentWeight()` recalculates from DB
- Weight check on item pickup (loot skip if overweight)
- Weight check on `vending:buy` (reject if too heavy)
- Weight check on `shop:buy_batch` (atomic batch validation)

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Job class weight bonuses (+0 to +1000) | **Missing** | MEDIUM |
| Gym Pass (+2000 all classes) | **Missing** | Low (cash shop) |
| 50% regen block enforcement | **Partially verified** (threshold defined, regen check unclear) | Medium |
| 90% attack/skill block enforcement | **Partially verified** (threshold defined, combat check unclear) | Medium |
| Overweight UI color indicators | **Partial** (weight bar exists, threshold colors TBD) | Low |

**Critical note on job class weight bonuses**: All characters currently have lower max weight than RO Classic intended. A Blacksmith with 90 STR gets 4700 instead of 5700 (missing +1000 class bonus). This affects gameplay balance -- Merchant-class characters are most impacted since they rely on high weight capacity for farming/vending.

---

## Pet System

### Deep Research Says (Doc 28)
- 56 tameable pre-renewal pets with specific taming items, food, accessories
- Capture rate formula: `(BaseCaptureRate + (PlayerLv - MonsterLv) * 30 + PlayerLUK * 20) * (200 - MonsterHP%) / 100`
- Intimacy system: 0-1000, five brackets (Awkward/Shy/Neutral/Cordial/Loyal)
- Stat bonuses activate at Cordial (750+)
- Hunger system: 0-100, decay per minute, feeding +20 hunger
- Optimal feeding at hunger 11-25 for best intimacy gain
- Overfeeding penalty: -100 intimacy, third consecutive overfeed = pet runs away permanently
- Owner death: -20 intimacy
- Pet accessories (cosmetic, per-pet unique)
- Performance emotes based on intimacy level
- Pet Incubator (item 643) to hatch eggs
- Return to egg preserves state
- One pet active at a time

### Actual Implementation
**Fully implemented** at `server/src/index.js` + `server/src/ro_pet_data.js`:

**Pet Data** (`ro_pet_data.js`):
- All 56 pre-renewal pets defined with correct taming items, food, capture rates, hunger decay, intimacy values, and stat bonuses
- `PET_INTIMATE` constants: CORDIAL=750, LOYAL=910, etc.
- `PET_HUNGER` constants: FEED_AMOUNT=20, hunger zones
- `calculateCaptureRate()`, `calculateFeedIntimacyChange()`, `getIntimacyLevel()`, `getHungerLevel()`

**Server Handlers**:
- `pet:tame` (line ~21946): Validates target, consumes taming item regardless, capture rate calculation, creates pet egg in DB
- `pet:incubate` (line ~22028): Hatches egg, loads pet state into `activePets` map, applies bonuses
- `pet:return_to_egg` (line ~22086): Saves state, removes from active, clears bonuses
- `pet:feed` (line ~22110): Food item validation, hunger increase (+20), intimacy change based on hunger zone
- `pet:rename` (line ~22172): One-time rename (24 char max)
- Pet hunger tick (every 10s): Hunger decay per `hungryDelay`/`fullnessDecay`, starvation intimacy loss, pet runs away at intimacy 0
- Owner death intimacy loss (-20 via `intimacyOwnerDie`)
- `recalcPetBonuses()`: Applies bonuses to `getEffectiveStats()` when intimacy >= CORDIAL (750)
- Pet bonuses integrated into stat pipeline: STR, AGI, VIT, INT, DEX, LUK, ATK, FLEE, DEF, MDEF, CRIT, Perfect Dodge, MaxHP, MaxSP, HP/SP regen%, ATK/MATK%, ASPD%, cast time%, crit damage%, element resist, race resist, status resist, HP drain

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Pet accessories (equip/visual) | **Missing** (data exists in `equipItemId` but no equip handler) | Low (cosmetic) |
| Performance emotes by intimacy | **Missing** (no emote system) | Low (cosmetic) |
| Triple consecutive overfeed = pet runs away | **Partially** (overfeed reduces intimacy, but no consecutive count tracked; pet runs away when intimacy hits 0) | Low |
| Pet support scripts (Smokie Hide, Sohee heal, etc.) | **Missing** | Low (optional per doc) |
| Pet visual actor on client | **Missing** (no client-side pet actor/sprite) | Medium |
| One pet at a time enforcement | **Implemented** (via `activePets` keyed by `characterId`) | Done |
| Pet egg items in inventory | **Partial** (egg created in DB but no pet egg item type rendering) | Low |

**Verdict**: Pet system is functionally complete on the server. The primary gap is the lack of client-side pet actors (visual following sprite). All stat bonuses, intimacy, hunger, taming, and feeding mechanics match RO Classic.

---

## Homunculus System

### Deep Research Says (Doc 28, Sec Homunculus)
- 4 types: Lif, Amistr, Filir, Vanilmirth (25% each on creation)
- Combat companion: attacks enemies, gains 10% of owner's Base EXP
- Level cap 99, probabilistic stat growth per level
- 3 skills per type + 1 evolved skill
- Skill points: 1 per 3 levels (33 total at Lv99)
- Feeding: type-specific food, +10 hunger, 1/min decay
- Intimacy: 0-1000, five brackets, optimal feeding at hunger 11-25
- Starvation: intimacy drain, death at intimacy 0 (resurrectable)
- Evolution: requires Loyal (911+) + Stone of Sage, resets intimacy to 10, +1-10 random stat bonus, unlocks 4th skill
- AI: follow/attack/standby, auto-target owner's target
- Alchemist-class only (with Bioethics quest skill)

### Actual Implementation
**Mostly implemented** at `server/src/index.js` + `server/src/ro_homunculus_data.js`:

**Homunculus Data** (`ro_homunculus_data.js`):
- All 4 types with correct base stats, growth probability tables, food items
- `rollStatGrowth()`, `rollHPSPGrowth()`, `applyLevelUpGrowth()`
- `calculateHomunculusStats()` for derived combat stats (ATK, MATK, HIT, CRIT, DEF, MDEF, FLEE, ASPD)
- `getExpToNextLevel()`, `getIntimacyBracket()`, `getFeedIntimacyChange()`
- `createHomunculus()` with sprite variant randomization

**Server Handlers**:
- Call Homunculus (skill 1813): Create (with Embryo) or re-summon, DB persistence
- Rest Homunculus (skill 1814): Vaporize if HP >= 80%, DB update
- Resurrect Homunculus (skill 1815): Revive dead homunculus (20-100% HP by level)
- `homunculus:feed` (line ~21364): Food item validation, hunger +10, intimacy change
- `homunculus:command` (line ~21410): follow/attack/standby modes
- `homunculus:skill_up` (line ~21430): Skill point allocation
- `homunculus:use_skill`: Skill casting (Healing Hands, Urgent Escape, Moonlight, Caprice -- others stubbed)
- `homunculus:evolve` (line ~21828): Loyalty check, Stone of Sage consumption, random +1-10 stat bonuses, intimacy reset to 10, 4th skill unlock
- Homunculus combat tick: Auto-attack enemies (ASPD-based), Flitting ASPD bonus, auto-target owner's target in follow mode
- Homunculus hunger tick (10s interval): Hunger decay, starvation intimacy drain, death at intimacy 0
- EXP sharing: 10% of Base EXP to homunculus, level-up with random stat growth
- Death handling: Auto-vaporize on owner death (HP >= 80%), standby otherwise
- Full DB persistence on disconnect (HP, SP, EXP, stats, intimacy, hunger)
- DB table: `character_homunculus` with all fields

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Homunculus skill casting (most skills) | **Stubbed** (Healing Hands, Urgent Escape, Moonlight, Caprice partially done; others return "not yet implemented") | Medium |
| Homunculus position broadcast to other players | **Missing** (no `homunculus:other_*` position sync) | Medium |
| Homunculus client actor (visual sprite) | **Missing** | Medium |
| Enemy targeting of homunculus | **Partial** (enemies can target but limited AI support) | Low |
| Homunculus passive skills (Brain Surgery, Adamantium Skin, Accelerated Flight, Instruction Change) | **Partial** (Accelerated Flight FLEE integrated, Flitting ATK/ASPD integrated; others unclear) | Medium |
| Homunculus Chaotic Blessings (random target heal) | **Stubbed** | Low |
| Evolved skills (Mental Charge, Blood Lust, S.B.R.44, Self-Destruction) | **Stubbed** (evolution works but skills return "not yet implemented") | Medium |
| Homunculus rename | **Missing** (no `homunculus:rename` handler) | Low |
| Custom AI toggle (`/hoai`) | **Missing** | Low |

**Verdict**: Homunculus core lifecycle is solid (summon/rest/resurrect/evolve/feed/combat/death/persistence). The main gaps are in skill effects (most are stubbed) and client-side visualization.

---

## Inventory Management

### Deep Research Says (Doc 38)
- 100 slot limit (rAthena default for pre-renewal)
- Weight-based capacity (2000 + STR*30 + bonuses)
- Stackable items share slots; equipment never stacks
- Three tabs: Item (consumables), Equip (equipment), Etc (misc/cards/ammo)
- Double-click to use/equip, drag to trade/storage/cart/drop
- Unidentified items: generic names, cannot equip/refine/card
- Magnifier for identification
- No sorting in pre-renewal (acquisition order)

### Actual Implementation
**Fully implemented**:
- `INVENTORY.MAX_SLOTS = 100`
- Weight system with full formula
- Stacking logic (stackable items merge, equipment unique slots)
- Stack merging via drag-and-drop (implemented 2026-03-18)
- Unidentified items: `identified` column, generic name mapping (15+ weapon + 7 armor types), orange "?" overlay, tooltip hiding
- Magnifier identification: `identify:select` handler, one-item-per-cast
- NPC buy/sell with Discount/Overcharge: `shop:buy`, `shop:sell`, `shop:buy_batch`, `shop:sell_batch`
- Equip/unequip with class/level validation, two-handed checks, headgear combos
- Equipment bonus application/removal on equip/unequip

### Gaps
| Feature | Status | Severity |
|---------|--------|----------|
| Item dropping to ground | **Missing** (no ground item system) | Medium |
| Item pickup from ground | **Missing** (loot goes directly to inventory from monster death) | Medium |
| Loot protection system (ownership priority) | **Missing** (no ground items = no protection needed yet) | Medium |
| Stack limits per item (30,000 for ammo/loot, 100 for potions) | **Not enforced** (no max stack per item type) | Low |
| Item sorting | **Not implemented** (correct for pre-renewal) | N/A |

---

## Critical Missing Features

### Priority 1 -- Core Economy (Must Implement)

1. **Player-to-Player Trading System**
   - No trade handlers exist at all
   - Needed: `trade:request`, `trade:accept`, `trade:add_item`, `trade:add_zeny`, `trade:lock`, `trade:confirm`, `trade:cancel`
   - State machine with atomic DB transactions
   - Weight/zeny/slot validation on both sides
   - Anti-scam: item locking, re-validation at commit, zeny overflow protection
   - Client: `TradeSubsystem` + `STradeWidget`

2. **Kafra Storage System**
   - No storage table or handlers exist
   - Needed: `character_storage` table (keyed by `user_id` for account-sharing), `storage:open/close/deposit/withdraw` socket handlers
   - 300 slot capacity, no weight limit, 40z fee
   - Equipment attribute preservation (refine, cards, forge)
   - Client: `StorageSubsystem` + `SStorageWidget`

### Priority 2 -- Gameplay Impact

3. **Job Class Weight Bonuses**
   - Currently missing from `getPlayerMaxWeight()` -- all classes get 0 bonus
   - Simple lookup table addition: Novice +0, Mage/Wizard/Sage +200, Archer/Thief/Assassin/Acolyte +400, Hunter/Bard/Dancer/Rogue/Priest/Monk +600, Swordsman/Knight/Crusader/Merchant +800, Blacksmith/Alchemist +1000
   - Affects weight capacity for every character in the game

4. **Ground Item System**
   - Items from monster death currently go directly to inventory
   - RO Classic: items drop on ground with ownership priority timers, other players can see/pick them up
   - Needed: ground item entity system, ownership tiers, despawn timer (60s), pickup range, weight check on pickup

5. **Homunculus Skill Effects**
   - Most homunculus skills are stubbed (return "not yet implemented")
   - Healing Hands, Amistr Bulwark, Castling, Chaotic Blessings, evolved skills all need implementation
   - Some partial work exists (Moonlight damage, Caprice bolts, Urgent Escape speed, Flitting ASPD)

### Priority 3 -- Polish

6. **Vending Tax** (5% on items > 100M zeny per unit)
7. **Pet Client Actor** (visual pet sprite following player)
8. **Homunculus Client Actor** (visual sprite, position sync)
9. **Pet Accessories** (equip system, visual change)
10. **Gym Pass Weight Bonus** (+2000 for all classes)
11. **Variable Cart Rental Cost** (per-town pricing)
12. **Buying Store** (reverse vending -- late Classic feature)

---

## Recommended Fixes

### Immediate (Low Effort, High Impact)

**Fix 1: Job Class Weight Bonuses** -- Add to `getPlayerMaxWeight()`:
```javascript
// In getPlayerMaxWeight(player):
const JOB_WEIGHT_BONUS = {
    novice: 0, super_novice: 0,
    mage: 200, wizard: 200, sage: 200,
    archer: 400, thief: 400, assassin: 400, acolyte: 400,
    hunter: 600, bard: 600, dancer: 600, rogue: 600, priest: 600, monk: 600,
    swordsman: 800, knight: 800, crusader: 800, merchant: 800,
    blacksmith: 1000, alchemist: 1000,
};
maxW += JOB_WEIGHT_BONUS[player.jobClass] || 0;
```
This is a ~5-line change that affects every character's weight capacity.

**Fix 2: Vending Tax** -- Add 5% tax check in `vending:buy` handler:
```javascript
// After calculating totalCost in vending:buy:
for (const vp of validPurchases) {
    if (vp.price > 100000000) {
        const tax = Math.floor(vp.price * vp.qty * 0.05);
        // Vendor receives totalCost - tax (tax is destroyed)
    }
}
```

### Medium Term (Trading System)

**Fix 3: Implement Trading** -- Full P2P trade system:
- New socket handlers: `trade:request/accept/deny/add_item/add_zeny/lock/confirm/cancel`
- Trade state per player: `tradeState: { partner, items: [], zeny: 0, locked: false }`
- Atomic DB transaction on confirm
- Client: `TradeSubsystem` + `STradeWidget`

### Medium Term (Storage System)

**Fix 4: Implement Kafra Storage** -- Account-shared storage:
- New DB table: `account_storage` (keyed by `user_id`, 300 slots)
- New socket handlers: `storage:open/close/deposit/withdraw`
- 40z fee on open (check/deduct zeny)
- Equipment attribute preservation
- Client: `StorageSubsystem` + `SStorageWidget`

### Data Accuracy Notes

- **Zeny field**: Server uses both `zeny` and `zuzucoin` (kept in sync via `attacker.zuzucoin = attacker.zeny`). The `zuzucoin` name appears to be a legacy field. Should be unified to `zeny` for clarity.
- **Pet Bonuses**: All 56 pet stat bonuses match the deep research doc. The `stunResist: 100` on Bongun (research says 1%) may be a scale mismatch -- research uses percentage (1%), implementation uses basis points (100 = 1%). Needs verification.
- **Homunculus EXP**: Uses approximate formula `floor(21 * level^2.8 + 100)` which matches research. Total ~203M to level 99.
- **Capture Rate Formula**: Implementation uses `calculateCaptureRate()` from `ro_pet_data.js` which matches the rAthena formula from the research doc.
- **Homunculus Intimacy**: `getFeedIntimacyChange()` in `ro_homunculus_data.js` uses hunger-based brackets (5/10/3/0/-10) which differ slightly from the research doc's values (0.5/1.0/0.75/-0.05/-0.5). The implementation uses integer values while research uses decimal. This means intimacy gains are 10x faster than RO Classic in the implementation.

---

## Source File References

| File | Contains |
|------|----------|
| `server/src/index.js` | All socket handlers, combat tick, hunger ticks, weight system |
| `server/src/ro_pet_data.js` | 56 pet definitions, intimacy/hunger constants, capture formula |
| `server/src/ro_homunculus_data.js` | 4 homunculus types, growth tables, derived stats, EXP curve |
| `database/migrations/add_cart_vending_identify.sql` | Cart, vending, identification tables |
| `database/migrations/add_homunculus_table.sql` | Homunculus persistence table |
| No migration exists for pet table | `character_pets` created via auto-creation or inline |
| No migration exists for storage | Storage system not yet built |
