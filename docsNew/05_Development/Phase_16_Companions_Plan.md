# Phase 16: Companions System Completion Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Pet_System](../03_Server_Side/Pet_System.md) | [Alchemist_Class_Research](Alchemist_Class_Research.md)
> **Status**: COMPLETED — Pet taming/feeding/loyalty, homunculus skills/evolution, companion visual actors

**Created**: 2026-03-18
**Based on**: Full codebase audit, rAthena pre-re source (pet.cpp, pet_db.yml, pet.conf), iRO Wiki Classic, RateMyServer, ro_homunculus_data.js audit

---

## Current State Audit

| System | Server | Client | DB | Verdict |
|--------|--------|--------|----|---------|
| Homunculus | **FULL** (4 types, 12 skills, combat tick, feeding, evolution, death, persistence) | **NONE** (no subsystem, no actor) | `character_homunculus` (26 cols) | Server done, client needed |
| Falcon | **FULL** (hasFalcon, Blitz Beat manual+auto, Spring Trap, Detect) | **NONE** (no visual) | N/A (player flag) | Functional, visual only |
| Cart | **FULL** (rent/remove/transfer, vending, Change Cart, speed penalty) | **PARTIAL** (ShopSubsystem, no visual) | `character_cart`, `vending_*` | Functional, visual only |
| Mount | **FULL** (toggle, ASPD penalty, speed+weight, Cavalry Mastery) | **NONE** (data received, no visual) | N/A (runtime flag) | Functional, visual only |
| Pet | **NONE** (only a stub message in Abracadabra) | **NONE** | **NONE** | Not implemented |
| Mercenary | **NONE** | **NONE** | **NONE** | Not implemented (defer) |

---

## Implementation Priority

| # | Task | Effort | Impact | Status |
|---|------|--------|--------|--------|
| 1 | Pet System (server) | 3-4 days | HIGH | **COMPLETE** — 48 pets in ro_pet_data.js, DB table, taming/incubate/feed/hunger/intimacy/bonuses/death |
| 2 | Pet System (client) | 2 days | HIGH | **COMPLETE** — PetSubsystem + inline SPetWidget (Z=21), RO theme, hunger/intimacy bars, feed/return buttons |
| 3 | Homunculus client subsystem | 2 days | MEDIUM | **COMPLETE** — HomunculusSubsystem + inline SHomunculusWidget (Z=22), HP/SP/hunger bars, feed/vaporize buttons, 9 event handlers |
| 4 | Companion visuals (ALL placeholders) | 2-3 days | LOW | **COMPLETE** — Pet: BP_EnemyCharacter 50% scale. Homunculus: BP_EnemyCharacter 60%. Cart: brown box. Mount: golden cylinder. Falcon: grey sphere with bob. All 10Hz follow, smooth interp. CompanionVisualSubsystem listens to player:stats for cart/mount/falcon flags. |
| 5 | Mercenary system | 3-4 days | LOW | DEFERRED |

---

## Task 1: Pet System — Server Implementation

### 1A. Pet Data File (`server/src/ro_pet_data.js`)

Create data file with all 56 pre-renewal pets from rAthena `pre-re/pet_db.yml`:

```javascript
// Per pet entry:
{
    mobId: 1002,           // Monster template ID (Poring)
    tamingItemId: 619,     // Unripe Apple
    eggItemId: 9001,       // Poring Egg
    foodItemId: 531,       // Apple Juice
    equipItemId: 10013,    // Backpack (0 = no accessory)
    captureRate: 2000,     // Base rate out of 10000 (20%)
    fullnessDecay: 3,      // Hunger points lost per tick
    hungryDelay: 60000,    // ms between hunger ticks (60s default)
    intimacyFed: 50,       // Intimacy gained per feed at Hungry
    intimacyOverfed: -100, // Intimacy lost on overfeed
    intimacyOwnerDie: -20, // Intimacy lost on owner death
    startIntimacy: 250,    // Starting intimacy (Neutral)
    specialPerformance: true,
    bonusScript: null,     // Function reference for stat bonuses at Cordial/Loyal
    // Stat bonuses (applied when intimacy >= 750 Cordial)
    bonuses: { luk: 2, critical: 1 }, // Poring example
}
```

**Top 20 pets to prioritize** (most useful bonuses):
1. Poring (1002) — LUK +2, CRIT +1
2. Drops (1113) — HIT +3, ATK +3
3. Lunatic (1063) — CRIT +2, ATK +2
4. Yoyo (1057) — CRIT +3, LUK -1
5. Orc Warrior (1023) — ATK +10, DEF -3
6. Smokie (1056) — AGI +1, Perfect Dodge +1
7. Hunter Fly (1035) — FLEE -5, Perfect Dodge +2
8. Peco Peco (1019) — MaxHP +150, MaxSP -10
9. Deviruchi (1109) — MATK +1%, ATK +1%, MaxHP/SP -3%
10. Baphomet Jr. (1101) — DEF +1, MDEF +1, Stun Resist
11. Chonchon (1011) — AGI +1, FLEE +2
12. Steel Chonchon (1042) — FLEE +6, AGI -1
13. Whisper (1179) — FLEE +7, DEF -3
14. Sohee (1170) — STR +1, DEX +1
15. Isis (1029) — ATK +1%, MATK -1%
16. Poporing (1031) — LUK +2, Poison Resist +10%
17. Savage Babe (1167) — VIT +1, MaxHP +50
18. Baby Desert Wolf (1107) — INT +1, MaxSP +50
19. Zealotus (1200) — +2% ATK & MATK vs Demi-Human
20. Bongun (1188) — VIT +1, Stun Resist +100

### 1B. Database Table (`character_pets`)

```sql
CREATE TABLE IF NOT EXISTS character_pets (
    id SERIAL PRIMARY KEY,
    character_id INTEGER NOT NULL REFERENCES characters(id),
    mob_id INTEGER NOT NULL,         -- Monster template ID
    egg_item_id INTEGER NOT NULL,    -- Egg item ID (9001-9056)
    pet_name VARCHAR(24) DEFAULT '', -- Custom pet name
    intimacy INTEGER DEFAULT 250,    -- 0-1000
    hunger INTEGER DEFAULT 100,      -- 0-100
    equip_item_id INTEGER DEFAULT 0, -- Equipped accessory (0 = none)
    is_hatched BOOLEAN DEFAULT FALSE,-- true = out of egg
    is_active BOOLEAN DEFAULT FALSE, -- true = following player
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(character_id, egg_item_id)
);
```

Auto-create at server startup (ADD COLUMN IF NOT EXISTS pattern).

### 1C. Taming System

**Socket event: `pet:tame`**
- Client sends: `{ targetEnemyId, tamingItemId }`
- Server validates:
  1. Player has taming item in inventory
  2. Target enemy is alive, in range (5 cells = 250 UE units), same zone
  3. Target monster is in `PET_DB` (tameable)
  4. Monster is not boss class
  5. Player doesn't already have a pet of this type
- Calculate capture rate (pre-renewal legacy formula):
  ```
  rate = (baseRate + (playerLevel - monsterLevel) * 30 + playerLUK * 20)
         * (200 - monsterHPPercent) / 100
  ```
- Consume taming item regardless of success
- On success:
  1. Remove monster from world (enemy:death broadcast)
  2. Create pet egg item in player inventory
  3. Create `character_pets` row with `is_hatched = false`
  4. Emit `pet:tamed` to player
- On failure:
  1. Emit `pet:tame_failed` to player

### 1D. Egg/Incubator System

**Socket event: `pet:incubate`**
- Client sends: `{ petId }` (character_pets row ID)
- Validate: Player has Pet Incubator (643) in inventory, pet exists and is not hatched
- Consume Pet Incubator
- Set `is_hatched = true`, `is_active = true`
- Initialize pet state in `activePets` Map
- Emit `pet:hatched` with full pet data

**Socket event: `pet:return_to_egg`**
- Deactivate pet, save state to DB
- Set `is_hatched = false`, `is_active = false`
- No intimacy loss on return (confirmed rAthena)

### 1E. Hunger/Intimacy System

**Hunger tick** (every `hungryDelay` ms per pet, default 60s):
- Decrease hunger by `fullnessDecay` (varies per pet, 1-7)
- If hunger reaches 0-10 (Very Hungry): -5 intimacy every 20 seconds
- If intimacy reaches 0: pet runs away (DELETE from DB, pet is lost forever)
- Emit `pet:hunger_update` to owner

**Feeding** (`pet:feed` event):
- Validate: Player has pet's specific food item
- Consume food item
- Hunger += 20 (cap at 100)
- Intimacy change based on hunger bracket:
  | Hunger | Intimacy Change |
  |--------|----------------|
  | 0-10 (Very Hungry) | +IntimacyFed * 0.5 |
  | 11-25 (Hungry) | +IntimacyFed (full) |
  | 26-75 (Neutral) | +IntimacyFed * 0.75 |
  | 76-100 (Satisfied/Stuffed) | IntimacyOverfed (-100) |
- Emit `pet:fed` with emote (thumbs up / hmm / omg)

**Owner death** hook:
- In `applyDeathPenalty()` or `clearBuffsOnDeath()`, also: intimacy -= 20 for active pet

**Intimacy levels** (rAthena pet.hpp):
| Level | Range | Effects |
|-------|-------|---------|
| Awkward | 0-99 | May run away. No bonuses. |
| Shy | 100-249 | No bonuses. |
| Neutral | 250-749 | No bonuses. |
| Cordial | 750-909 | **Stat bonuses active** |
| Loyal | 910-1000 | **Full stat bonuses active** |

### 1F. Pet Stat Bonuses

When pet intimacy >= 750 (Cordial), apply bonuses from `PET_DB[mobId].bonuses` to player's derived stats.

Integration points:
- `getEffectiveStats(player)` — add pet bonus merge (similar to buff/passive bonuses)
- Recalculate when: pet hatched/returned, intimacy crosses 750 threshold, pet equipment changes
- Emit `player:stats` on bonus activation/deactivation

### 1G. Pet Commands

**Socket events:**
- `pet:rename` — set custom name (once only, like RO Classic)
- `pet:performance` — trigger pet trick animation (cosmetic)
- `pet:equip_accessory` — equip pet's unique accessory item
- `pet:unequip_accessory` — remove accessory

### 1H. Pet Following AI (server-side)

- Active pet follows owner at ~80% of player move speed
- Position updated in player tick (alongside homunculus position)
- Pet position broadcast via `pet:position` or bundled with `player:moved`
- Pet does NOT take damage, cannot be targeted by monsters
- Pet does NOT attack (no combat in pre-renewal pet system)

### 1I. Pet Items in Database

Ensure these items exist in the `items` table:
- 56 taming items (619-642, 659-661, 12225, 12357-12374, 12395, 14569-14574)
- 56 pet eggs (9001-9056)
- Pet Incubator (643)
- 25+ pet food items (various)
- 25+ pet accessories (10001-10038)

Many of these are already in the 6,169 canonical items. Verify and add any missing.

---

## Task 2: Pet System — Client Implementation

### 2A. PetSubsystem (UWorldSubsystem)

New file: `client/SabriMMO/Source/SabriMMO/UI/PetSubsystem.h/.cpp`

Responsibilities:
- Register for `pet:hatched`, `pet:hunger_update`, `pet:fed`, `pet:tame_failed`, `pet:ran_away`
- Track active pet state (name, mob ID, hunger, intimacy, equipped accessory)
- Manage pet actor spawning/despawning
- Pet UI widget (Alt+J equivalent) showing: pet sprite, name, hunger bar, intimacy bar, feed/return/performance/rename buttons

### 2B. Pet Following Actor

- Spawn a `BP_PetCharacter` actor near the player
- Follow player position with slight delay (lerp to player position at ~80% speed)
- Use monster mesh for the pet's mob ID (or placeholder)
- No HP bar, no targeting, no combat

### 2C. Pet Window Widget (SPetWidget)

- RO brown/gold theme, Z=21
- Shows: pet name, type icon, hunger bar (0-100), intimacy bar (0-1000) with level label
- Buttons: Feed, Return to Egg, Performance, Rename
- Accessory slot: drag pet accessory item to equip

---

## Task 3: Homunculus Client Subsystem

### 3A. HomunculusSubsystem (UWorldSubsystem)

New file: `client/SabriMMO/Source/SabriMMO/UI/HomunculusSubsystem.h/.cpp`

Register for existing server events:
- `homunculus:summoned`, `homunculus:vaporized`, `homunculus:update`
- `homunculus:leveled_up`, `homunculus:died`, `homunculus:attack`, `homunculus:damage`
- `homunculus:fed`, `homunculus:hunger_tick`, `homunculus:evolved`
- `homunculus:other_summoned`, `homunculus:other_dismissed`

Responsibilities:
- Spawn `BP_HomunculusCharacter` actor for local player's homunculus
- Spawn actors for OTHER players' homunculi (via `homunculus:other_summoned`)
- Position sync (follow owner)
- HP/SP bars (overhead, similar to enemy HP bars)
- Homunculus window widget: stats, skills, feed button, vaporize, evolution

### 3B. Homunculus Position Broadcasting

**Server addition**: In the player position tick (30Hz), also broadcast homunculus position:
- `homunculus:position` event with `{ ownerId, homunculusId, x, y, z }`
- Only broadcast to players in same zone
- Homunculus follows owner with slight offset

---

## Task 4: Companion Visuals (Cart/Mount/Falcon)

### 4A. Cart Visual

- Spawn `BP_CartActor` attached to merchant-class player characters
- 5 cart meshes (by level: 1-40, 41-65, 66-80, 81-90, 91+)
- Follows behind player at fixed offset
- Show/hide based on `hasCart` from `player:joined` and `player:moved`
- Change Cart skill updates `cartType` → swap mesh

### 4B. Mount Visual

- Replace player mesh/animation set when `isMounted = true`
- Knight → Peco Peco rider mesh, Crusader → Grand Peco rider mesh
- Requires separate mounted animation set (idle, walk, attack)
- Toggle on `player:mount` event

### 4C. Falcon Visual

- Small falcon mesh attached to Hunter/Sniper character shoulder
- Visible when `hasFalcon = true` (data already in stats payload)
- Blitz Beat animation: falcon swoops at target (purely cosmetic)

**Note**: All 3 visual tasks require **3D art assets** (meshes + animations) that don't exist yet. These are blocked by art production.

---

## Task 5: Mercenary System (DEFERRED)

Mercenaries are a post-Rebirth feature in many server implementations and lower priority for pre-renewal Classic. Defer until Phase 16B.

Key features if implemented:
- 30 mercenary types (10 per tier: Sword/Spear/Bow × 3 levels each)
- Hired from Mercenary Guild NPCs
- Fixed duration (30 minutes)
- Own AI, HP, attacks, skills
- Cannot be healed by player

---

## Execution Order

### Week 1: Pet System Server (Tasks 1A-1I)
- Day 1: Create `ro_pet_data.js` (56 pets), DB table, item verification
- Day 2: Taming + egg/incubator handlers
- Day 3: Hunger/intimacy tick, feeding, stat bonuses integration
- Day 4: Pet commands, owner death hook, following position, persistence

### Week 2: Pet + Homunculus Client (Tasks 2, 3)
- Day 5-6: PetSubsystem + SPetWidget + pet following actor
- Day 7-8: HomunculusSubsystem + actor spawning + position broadcasting

### Week 3: Companion Visuals (Task 4) — ART DEPENDENT
- Cart/mount/falcon visuals — only when 3D assets are ready

---

## RO Classic Compliance Checklist

### Pet System
- [ ] 56 tameable pets with correct taming items, eggs, food, accessories
- [ ] Capture formula: `(baseRate + (level_diff)*30 + LUK*20) * (200 - HP%) / 100`
- [ ] Hunger: 0-100 scale, per-pet decay rate, feed +20
- [ ] Intimacy: 0-1000, 5 levels (Awkward/Shy/Neutral/Cordial/Loyal)
- [ ] Stat bonuses at Cordial (750+), per-pet unique bonuses
- [ ] Overfeed penalty: -100 intimacy when Satisfied/Stuffed
- [ ] Owner death: -20 intimacy
- [ ] Starvation: -5 per 20s when Very Hungry (0-10)
- [ ] Pet runs away at intimacy 0 (permanently lost)
- [ ] Pet Incubator (643) consumed to hatch
- [ ] Pet accessory equip (gates SupportScript if enabled)
- [ ] No evolution in pre-renewal
- [ ] No auto-loot in pre-renewal (disabled by default)
- [ ] Pet does NOT take damage or fight
- [ ] Each pet has ONE specific food (no universal food)

### Homunculus (already implemented — verify)
- [ ] 4 types: Lif, Amistr, Filir, Vanilmirth
- [ ] Created with Embryo item, random type
- [ ] Hunger/intimacy system (separate from pet system)
- [ ] 12 base skills (3 per type)
- [ ] Auto-attack with ASPD
- [ ] EXP sharing (10%)
- [ ] Evolution at Loyal (911+) + Stone of Sage
- [ ] Owner death: auto-vaporize or standby
- [ ] Full DB persistence

### Falcon/Cart/Mount (already implemented — verify)
- [ ] Falcon: hasFalcon flag, enables Blitz Beat / Spring Trap / Detect
- [ ] Cart: 100 slots, 8000 weight, speed penalty, 5 visual types
- [ ] Mount: +36% speed, ASPD penalty (50% - Cavalry Mastery), +1000 weight
- [ ] Mount: Brandish Spear requires mount
- [ ] Cart: Cart Revolution requires cart

---

## Systems NOT Needed for Pre-Renewal Classic

| Feature | Status | Reason |
|---------|--------|--------|
| Pet evolution | SKIP | Added kRO Oct 2014, post-renewal |
| Pet auto-loot | SKIP | Deprecated in standard pre-renewal (disabled by default in rAthena) |
| Pet active skills (healing, attacking) | SKIP | Disabled by default (`pet_status_support: no`) |
| Mercenary system | DEFER | Lower priority, niche feature |
| Homunculus S (Genetic class) | SKIP | Renewal-only |
