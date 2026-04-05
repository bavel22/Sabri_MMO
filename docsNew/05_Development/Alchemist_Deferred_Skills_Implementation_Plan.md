# Alchemist Deferred Skills — Full Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Alchemist_Class_Research](Alchemist_Class_Research.md) | [Skill_System](../03_Server_Side/Skill_System.md)

**Date:** 2026-03-17
**Status:** COMPLETED — Pharmacy crafting, Summon Flora, Marine Sphere, Potion Pitcher ally targeting, CP ally targeting all implemented
**Scope:** Pharmacy (1800), Potion Pitcher ally targeting (1806), CP ally targeting (1808-1811), Summon Flora (1803), Summon Marine Sphere (1807)
**Sources:** rAthena pre-renewal `skill_db.yml`, iRO Wiki Classic, RateMyServer brew calculator

---

## Blocking Analysis: NOTHING IS BLOCKED

Every system needed for full Alchemist implementation already exists:

| Required System | Existing Infrastructure | Location |
|----------------|------------------------|----------|
| Crafting pattern | Arrow Crafting (skill 305) | `index.js:17244-17309` |
| Item creation | `addItemToInventory()` | `index.js:3990-4065` |
| Item lookup | `itemDefinitions.get(id)` | `index.js:3644-3936` |
| Ally targeting | Heal (400) + Resurrection (1017) | `index.js:9851-9945, 14708-14742` |
| Ground effects | `createGroundEffect()` | `ro_ground_effects.js:117-204` |
| Summon entities | Homunculus system | `index.js:17645-17764` |
| Success rate RNG | Multiple existing examples | Combat formulas |

---

## Implementation Order

```
Phase A0: Shared Crafting Popup ────────── (client UI — used by Arrow Crafting + Pharmacy)
Phase A: Pharmacy Crafting ─────────────── (depends on A0 for UI)
Phase B: Ally Targeting ────────────────── (standalone, no deps)
Phase C: Summon Flora ──────────────────── (standalone, no deps)
Phase D: Summon Marine Sphere ──────────── (standalone, no deps)
```

All 4 phases are independent and can be implemented in any order. Recommended order is A→B→C→D because:
- Pharmacy is needed to CREATE the catalysts for other skills
- Ally targeting is the simplest upgrade (pattern already proven)
- Flora is more complex but uses existing patterns
- Marine Sphere has unique detonation logic

---

## Phase A0: Shared Crafting Popup Widget (Client)

### Overview
Build a reusable `SCraftingPopup` Slate widget + `CraftingSubsystem` that handles BOTH Arrow Crafting (305) and Pharmacy (1800). Arrow Crafting currently has a working server handler but NO client UI — the `arrow_crafting:recipes` event fires but nothing listens for it. This phase fixes that gap AND provides the UI for Pharmacy.

### Reference Pattern: SCardCompoundPopup
The existing card compound popup (`SCardCompoundPopup.h/cpp`) is the exact template:
- Fullscreen semi-transparent backdrop (click outside to dismiss)
- Centered popup with RO 3-layer frame (gold/dark/brown)
- Scrollable list of selectable items
- Click-to-select with server emit
- Status message area for success/failure
- Dismiss via X button, Escape key, or backdrop click
- Z-layer 23 (above inventory)

### A0-1: CraftingSubsystem (UWorldSubsystem)

**File:** `client/SabriMMO/Source/SabriMMO/UI/CraftingSubsystem.h/cpp`

**Responsibilities:**
- Register socket handlers: `arrow_crafting:recipes`, `arrow_crafting:result`, `pharmacy:recipes`, `pharmacy:result`
- Store received recipe list in `TArray<FCraftingRecipe>` struct
- Show/hide `SCraftingPopup` overlay at Z=23
- Emit `skill:use` with `sourceInventoryId` (Arrow Crafting) or `pharmacy:craft` with `recipeId` (Pharmacy)

**Data struct:**
```cpp
struct FCraftingRecipe
{
    int32 OutputItemId;
    FString OutputName;
    FString OutputIcon;
    int32 OutputQuantity;
    int32 SuccessRate;          // 0-100% (Pharmacy only, Arrow Crafting = 100%)
    FString SourceItemName;     // Arrow Crafting: source item name
    int32 SourceInventoryId;    // Arrow Crafting: inventory_id to pass back
    int32 RecipeId;             // Pharmacy: output item ID to pass back
};
```

**Socket event flow:**

```
Arrow Crafting:
  Client: skill:use {skillId:305} → Server: arrow_crafting:recipes {craftable:[...]}
  Client: skill:use {skillId:305, sourceInventoryId:X} → Server: arrow_crafting:result {success, msg}

Pharmacy:
  Client: skill:use {skillId:1800} → Server: pharmacy:recipes {craftable:[...]}
  Client: pharmacy:craft {recipeOutputId:X} → Server: pharmacy:result {success, msg, outputName, qty}
```

### A0-2: SCraftingPopup (Slate Widget)

**File:** `client/SabriMMO/Source/SabriMMO/UI/SCraftingPopup.h/cpp`

**Layout (mirrors SCardCompoundPopup):**
```
┌──────────────────────────────┐
│ [icon] Craft: Arrow Crafting │  ← Title bar + X button
├──────────────────────────────┤
│ Select an item to craft:     │
├──────────────────────────────┤
│ [🏹] Fire Arrow ×600         │  ← Scrollable recipe list
│     from Red Blood           │
│ [🏹] Silver Arrow ×600       │
│     from Silver Robe[2]      │
│ [⚗️] Acid Bottle             │  ← Pharmacy recipes show success %
│     Rate: 67%                │
│ ...                          │
├──────────────────────────────┤
│ Success! Crafted 600x ...    │  ← Status message (green/red)
└──────────────────────────────┘
```

**SLATE_ARGS:**
```cpp
SLATE_BEGIN_ARGS(SCraftingPopup)
    : _Subsystem(nullptr)
{}
    SLATE_ARGUMENT(UCraftingSubsystem*, Subsystem)
    SLATE_ARGUMENT(FString, Title)           // "Arrow Crafting" or "Pharmacy"
    SLATE_ARGUMENT(TArray<FCraftingRecipe>, Recipes)
    SLATE_ARGUMENT(bool, ShowSuccessRate)    // true for Pharmacy, false for Arrow Crafting
SLATE_END_ARGS()
```

**Click handler:** When player clicks a recipe row:
- Arrow Crafting: `CraftingSubsystem->EmitArrowCraft(sourceInventoryId)`
- Pharmacy: `CraftingSubsystem->EmitPharmacyCraft(recipeOutputId)`

**Result handler:** On `arrow_crafting:result` or `pharmacy:result`:
- Success: Show green text "Crafted X" + refresh inventory (automatic via `inventory:data`)
- Failure: Show red text "Failed to create X. Materials lost."
- Keep popup open for multiple crafts (don't auto-dismiss)

### A0-3: Skill Trigger Integration

**In SkillTreeSubsystem or HotbarSubsystem:** When skill 305 or 1800 is used:
- Emit `skill:use` as normal (server decides what to send back)
- `CraftingSubsystem` receives `arrow_crafting:recipes` or `pharmacy:recipes` event
- `CraftingSubsystem` creates and shows `SCraftingPopup`

No special client-side interception needed — the server already sends the recipe list on first `skill:use` call without a `sourceInventoryId`.

### A0-4: Effort Estimate
- `CraftingSubsystem.h/cpp`: ~200 lines (event handlers, popup management, emit helpers)
- `SCraftingPopup.h/cpp`: ~300 lines (following SCardCompoundPopup pattern exactly)
- SkillTree integration: ~10 lines (none needed — server handles it)
- **Total: ~510 lines (C++)**

---

## Phase A: Pharmacy (1800) — Potion Crafting System

### Overview
Alchemists craft potions/bottles/catalysts from ingredients. Success is RNG-based. Failure consumes all ingredients.

### A1: Recipe Data (constant in index.js)

```js
const PHARMACY_RECIPES = {
    // Key: output item ID. Each recipe lists ingredients, guide requirement, and rate modifier.
    // Potion Creation Guide (7144) recipes:
    501: { name: 'Red Potion', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1093, qty: 1 }], rateMod: 2000 },
    // 713 = Empty Potion Bottle, 1093 = Red Herb
    502: { name: 'Orange Potion', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1093, qty: 1 }], rateMod: 2000 },
    // (orange potion has different herb)
    503: { name: 'Yellow Potion', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1094, qty: 1 }], rateMod: 2000 },
    504: { name: 'White Potion', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1095, qty: 1 }], rateMod: 2000 },
    505: { name: 'Blue Potion', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1096, qty: 1 }, { id: 1097, qty: 1 }], rateMod: 0 },
    970: { name: 'Alcohol', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 7126, qty: 1 }, { id: 1033, qty: 5 }, { id: 7033, qty: 5 }], rateMod: 1000 },
    7135: { name: 'Bottle Grenade', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1059, qty: 1 }, { id: 970, qty: 1 }], rateMod: 0 },
    7136: { name: 'Acid Bottle', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 929, qty: 1 }], rateMod: 0 },
    7137: { name: 'Plant Bottle', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1033, qty: 2 }], rateMod: 0 },
    7138: { name: 'Marine Sphere Bottle', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 1055, qty: 1 }, { id: 1051, qty: 1 }], rateMod: 0 },
    7139: { name: 'Glistening Coat', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 950, qty: 1 }, { id: 1044, qty: 1 }, { id: 970, qty: 1 }], rateMod: -1000 },
    605: { name: 'Anodyne', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 970, qty: 1 }, { id: 708, qty: 1 }], rateMod: 0 },
    606: { name: 'Aloevera', guide: 7144, ingredients: [{ id: 713, qty: 1 }, { id: 518, qty: 1 }, { id: 704, qty: 1 }], rateMod: 0 },
    7142: { name: 'Embryo', guide: 7144, ingredients: [{ id: 7134, qty: 1 }, { id: 7133, qty: 1 }, { id: 7140, qty: 1 }, { id: 7141, qty: 1 }], rateMod: 0, requiresBioethics: true },
    // Condensed Potion Creation Guide (7145) recipes:
    545: { name: 'Condensed Red Potion', guide: 7145, ingredients: [{ id: 501, qty: 1 }, { id: 952, qty: 1 }], rateMod: 0 },
    546: { name: 'Condensed Yellow Potion', guide: 7145, ingredients: [{ id: 503, qty: 1 }, { id: 1048, qty: 1 }], rateMod: -500 },
    547: { name: 'Condensed White Potion', guide: 7145, ingredients: [{ id: 504, qty: 1 }, { id: 1061, qty: 1 }], rateMod: -1000 },
};
```

**Note:** Item IDs above are rAthena canonical. Some ingredient IDs will need verification against our `items` table. Missing items need migration INSERT.

### A2: Success Rate Formula (rAthena-verified)

```js
function calculateBrewRate(player, recipe) {
    const pharmacyLv = player.learnedSkills?.[1800] || 0;
    const potResLv = player.learnedSkills?.[1805] || 0;
    const stats = getEffectiveStats(player);
    const jobLv = player.jobLevel || 1;

    // rAthena formula: base_make_per (in 0.01% units — 10000 = 100%)
    let rate = (potResLv * 50) + (pharmacyLv * 300) + (jobLv * 20)
             + Math.floor(stats.int / 2) * 10 + (stats.dex * 10) + (stats.luk * 10);

    // Per-recipe modifier (varies by potion type)
    rate += recipe.rateMod || 0;

    // Random fluctuation for some recipes (simplified: ±500 variation)
    if (recipe.rateMod > 0) rate += Math.floor(Math.random() * 1000) + 10;
    else if (recipe.rateMod < 0) rate -= Math.floor(Math.random() * 1000) + 10;

    // Baby/Adopted class penalty: -30%
    if (player.isAdopted) rate = Math.floor(rate * 0.7);

    // Clamp to [0, 10000] (0-100%)
    return Math.max(0, Math.min(10000, rate));
}
```

### A3: Handler Implementation

The handler follows the Arrow Crafting pattern:

1. Player uses Pharmacy skill → server sends available recipes based on inventory
2. Player selects recipe → `pharmacy:craft` event with `{ recipeOutputId }`
3. Server validates: guide in inventory, all ingredients available, Medicine Bowl available
4. Consume Medicine Bowl + all ingredients (success or fail)
5. Roll success rate
6. On success: `addItemToInventory()` for output item
7. Broadcast result + update inventory

**Two socket events needed:**
- `pharmacy:recipes` (server → client) — list of craftable items based on current inventory
- `pharmacy:craft` (client → server) — attempt to craft a specific recipe
- `pharmacy:result` (server → client) — success/failure with item info

### A4: Database Migration

```sql
-- Ensure Pharmacy catalyst/ingredient items exist in items table
-- Medicine Bowl, Creation Guides, Empty Bottles, herbs, etc.
-- Most should already exist from the 6169-item rAthena import.
-- Verify and add any missing:
INSERT INTO items (item_id, name, item_type, weight, price, stackable, max_stack)
VALUES
    (7134, 'Medicine Bowl', 'misc', 1, 8, true, 9999),
    (7144, 'Potion Creation Guide', 'misc', 0, 200000, false, 1),
    (7145, 'Condensed Potion Creation Guide', 'misc', 0, 480000, false, 1)
ON CONFLICT (item_id) DO NOTHING;
```

### A5: Effort Estimate
- Recipe data constant: ~50 lines
- Success rate function: ~25 lines
- Pharmacy handler (recipe list + craft attempt): ~80 lines
- DB migration: ~10 lines
- **Total: ~165 lines**

---

## Phase B: Ally Targeting — Potion Pitcher + CP Skills

### Overview
Upgrade Potion Pitcher (1806) and Chemical Protection x4 (1808-1811) from self-only to targeting allies (party/guild members). The Heal skill handler is the exact pattern to follow.

### B1: Potion Pitcher Ally Targeting

Current handler (self-only) at `index.js:17502-17583`. Upgrade to:

1. If `targetId && !isEnemy` → look up `connectedPlayers.get(targetId)` as target
2. If no targetId or targetId === characterId → self-target (current behavior)
3. Range check: position distance ≤ 900 UE (9 cells) + RANGE_TOLERANCE
4. Apply heal to target instead of self
5. Send `combat:health_update` to target's socket
6. VIT scaling uses TARGET's VIT, not caster's

**Pattern source:** Heal handler at `index.js:9851-9945`

Changes needed:
```js
// Replace: const zone = player.zone || 'prontera_south';
// With ally targeting block:
const pitcherTarget = (targetId && !isEnemy && targetId !== characterId)
    ? connectedPlayers.get(targetId) : player;
const pitcherTargetId = (targetId && !isEnemy) ? targetId : characterId;
if (!pitcherTarget) { socket.emit('skill:error', { message: 'Target not found' }); return; }

// Range check for non-self targets
if (pitcherTargetId !== characterId) {
    const ap = await getPlayerPosition(characterId);
    const tp = await getPlayerPosition(pitcherTargetId);
    if (!ap || !tp) return;
    const d = Math.sqrt((ap.x-tp.x)**2 + (ap.y-tp.y)**2);
    if (d > skillRange + COMBAT.RANGE_TOLERANCE) {
        socket.emit('combat:out_of_range', { ... });
        return;
    }
}

// VIT scaling uses TARGET's VIT
const targetVIT = getEffectiveStats(pitcherTarget).vit || 0;
```

### B2: Chemical Protection Ally Targeting

Current handler (self-only) at `index.js:17586-17634`. Upgrade to:

1. If `targetId && !isEnemy` → look up target in `connectedPlayers`
2. Range check: ≤ 150 UE (1 cell, per rAthena `Range: 1`)
3. Apply buff to target instead of self
4. Send buff notification to target's socket

**Note:** CP range is 1 cell (150 UE). The alchemist must be adjacent to the target. This is intentionally short range — it's a melee-range support skill.

### B3: Homunculus Targeting for Potion Pitcher

In RO Classic, Potion Pitcher heals homunculus at 3x effectiveness. If the player targets their own homunculus:
```js
if (targetId && activeHomunculi.has(characterId)) {
    const homu = activeHomunculi.get(characterId);
    // Apply heal to homunculus at 3x effectiveness
    const homuHeal = finalHeal * 3;
    homu.hpCurrent = Math.min(homu.hpMax, homu.hpCurrent + homuHeal);
    socket.emit('homunculus:health_update', { hp: homu.hpCurrent, maxHp: homu.hpMax });
}
```

### B4: Effort Estimate
- Potion Pitcher ally targeting: ~30 lines changed
- CP ally targeting: ~25 lines changed
- Homunculus healing: ~15 lines
- **Total: ~70 lines**

---

## Phase C: Summon Flora / Bio Cannibalize (1803)

### Overview
Summon plant monsters that auto-attack nearby enemies. Different plant type per skill level. Plants are runtime-only entities (no DB persistence — they expire).

### C1: Plant Data Table

```js
const FLORA_PLANTS = {
    1: { monsterId: 1020, name: 'Mandragora', maxCount: 5, duration: 300000, atkMin: 26, atkMax: 35, element: 'earth', elementLv: 3, attackRange: 200 },
    2: { monsterId: 1068, name: 'Hydra', maxCount: 4, duration: 240000, atkMin: 22, atkMax: 28, element: 'water', elementLv: 2, attackRange: 200 },
    3: { monsterId: 1118, name: 'Flora', maxCount: 3, duration: 180000, atkMin: 242, atkMax: 273, element: 'earth', elementLv: 1, attackRange: 200 },
    4: { monsterId: 1500, name: 'Parasite', maxCount: 2, duration: 120000, atkMin: 215, atkMax: 430, element: 'wind', elementLv: 2, attackRange: 200 },
    5: { monsterId: 1368, name: 'Geographer', maxCount: 1, duration: 60000, atkMin: 467, atkMax: 621, element: 'earth', elementLv: 3, attackRange: 200, healer: true },
};
```

### C2: Runtime Registry

```js
// Map: plantId → { id, ownerId, zone, x, y, z, type, name, hp, maxHp, atkMin, atkMax,
//                   element, createdAt, expiresAt, lastAttackTime, targetEnemyId }
const activePlants = new Map();
let nextPlantId = 1;

// Reverse lookup: characterId → Set of plantIds
const playerPlants = new Map();
```

No DB persistence needed — plants are temporary and expire. On server restart they're gone (acceptable — same as ground effects).

### C3: HP Formula (rAthena-verified)

```
MaxHP = 2230 + 200 * SkillLevel + 10 * CasterBaseLevel
```

### C4: Handler Implementation

1. Validate: ground position, Plant Bottle catalyst (7137), zone not town
2. Check max count: `playerPlants.get(characterId).size < plantData.maxCount`
3. If summoning different type than existing → remove all existing plants
4. Create plant entry in `activePlants` Map
5. Broadcast `summon:plant_spawned` to zone (type, name, position, HP)
6. Start auto-attack behavior in combat tick

### C5: Auto-Attack Tick (200ms interval in combat loop)

```js
// In the main combat tick loop, add plant attack processing:
for (const [plantId, plant] of activePlants.entries()) {
    // Expiry check
    if (Date.now() >= plant.expiresAt) {
        activePlants.delete(plantId);
        broadcastToZone(plant.zone, 'summon:plant_removed', { plantId });
        continue;
    }
    // Find nearest enemy in range (plants auto-target)
    // Attack at ~1.5s intervals (ASPD 130 equivalent)
    // Damage = random(atkMin, atkMax) with element
    // Aggro goes to OWNER, not plant
    // Geographer: heal nearby allies below 60% HP every 5s
}
```

### C6: Cleanup Hooks
- Owner death → remove all plants
- Owner disconnect → remove all plants
- Owner zone change → remove all plants
- New summon of different type → remove existing plants

### C7: Socket Events
- `summon:plant_spawned` — { plantId, type, name, x, y, z, hp, maxHp, ownerId }
- `summon:plant_removed` — { plantId, reason }
- `summon:plant_attack` — { plantId, targetId, damage }
- `summon:plant_health` — { plantId, hp, maxHp }

### C8: Client Impact
- Client needs to render plant actors at broadcast positions
- No UE5 actor class exists yet → **Client actor is deferred** (server logic works, client won't see plants visually)
- Damage numbers and aggro still function correctly

### C9: Effort Estimate
- Plant data constant: ~15 lines
- Registry + cleanup: ~40 lines
- Handler (spawn logic): ~60 lines
- Auto-attack tick: ~50 lines
- Geographer heal: ~20 lines
- Cleanup hooks: ~30 lines
- **Total: ~215 lines**

---

## Phase D: Summon Marine Sphere / Sphere Mine (1807)

### Overview
Summon an explosive sphere that detonates on taking damage or after 30 seconds. Fire AoE 11x11 damage based on remaining HP.

### D1: Runtime Registry

```js
// Map: sphereId → { id, ownerId, zone, x, y, z, hp, maxHp, createdAt, expiresAt }
const activeMarineSpheres = new Map();
let nextSphereId = 1;

// Reverse: characterId → Set of sphereIds
const playerSpheres = new Map();
```

### D2: HP Formula (rAthena-verified)

```
MaxHP = 2000 + 400 * SkillLevel
```

### D3: Handler Implementation

1. Validate: ground position, Marine Sphere Bottle (7138) catalyst
2. Check max count: ≤ 3 active spheres (rAthena `ActiveInstance: 3`)
3. Create sphere entry with full HP and 30s expiry
4. Broadcast `summon:sphere_spawned` to zone

### D4: Detonation Logic

Sphere explodes when:
- Any damage is received (enemy attack, player click, etc.)
- 30 second timer expires
- Owner uses another sphere mine (optional: push existing to explode)

```js
function detonateMarineSphere(sphere) {
    const zone = sphere.zone;
    const aoERadius = 550; // 11x11 cells ≈ 550 UE

    // Fire element AoE damage = remaining HP
    const damage = sphere.hp;

    // Hit all enemies in range
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.zone !== zone) continue;
        const dx = enemy.x - sphere.x, dy = enemy.y - sphere.y;
        if (Math.sqrt(dx*dx + dy*dy) > aoERadius) continue;

        // Apply fire element vs enemy defense element
        let fireDmg = applyElementTable(damage, 'fire', enemy.element?.type || 'neutral', enemy.element?.level || 1);
        enemy.health = Math.max(0, enemy.health - fireDmg);
        // Aggro to owner
        setEnemyAggro(enemy, sphere.ownerId, 'skill');
        // Broadcast damage, death check...
    }

    // Remove sphere
    activeMarineSpheres.delete(sphere.id);
    broadcastToZone(zone, 'summon:sphere_exploded', { sphereId: sphere.id, x: sphere.x, y: sphere.y, z: sphere.z, damage });
}
```

### D5: Tick Integration

In the combat tick loop:
```js
// Marine Sphere expiry check (30s auto-detonate)
for (const [sid, sphere] of activeMarineSpheres.entries()) {
    if (Date.now() >= sphere.expiresAt) {
        detonateMarineSphere(sphere);
    }
}
```

### D6: Enemy Interaction

When an enemy attacks a marine sphere (it's a valid target):
- Actually, in RO the sphere triggers Self Destruction when ANY damage hits it
- This means the enemy AI needs to be able to target spheres
- **Simplified approach**: Spheres auto-detonate after 30s. Manual detonation requires player to "tap" them (click the sphere) via a `summon:detonate` event.

### D7: Socket Events
- `summon:sphere_spawned` — { sphereId, x, y, z, hp, maxHp, ownerId }
- `summon:sphere_exploded` — { sphereId, x, y, z, damage }
- `summon:sphere_removed` — { sphereId, reason }
- `summon:detonate` (client → server) — { sphereId } (manual trigger)

### D8: Client Impact
- Same as Flora — no UE5 actor yet, server logic works
- Explosion VFX would need fire AoE visual

### D9: Effort Estimate
- Registry: ~20 lines
- Handler (spawn): ~40 lines
- Detonation function: ~50 lines
- Tick integration: ~15 lines
- Manual detonate event: ~15 lines
- Cleanup hooks: ~20 lines
- **Total: ~160 lines**

---

## Summary: Total Implementation Effort

| Phase | Skill(s) | Server (JS) | Client (C++) | Blocked By |
|-------|----------|-------------|-------------|------------|
| A0: Crafting Popup | 305 + 1800 | 0 | ~510 | Nothing — SCardCompoundPopup is template |
| A: Pharmacy | 1800 | ~165 | 0 | A0 for UI |
| B: Ally Targeting | 1806, 1808-1811 | ~70 | 0 | Nothing — Heal pattern exists |
| C: Summon Flora | 1803 | ~215 | 0 | Nothing — Homunculus pattern exists |
| D: Marine Sphere | 1807 | ~160 | 0 | Nothing — similar to Flora |
| **Total** | **7 skills + UI** | **~610 JS** | **~510 C++** | **NO BLOCKERS** |

### Dependencies Graph

```
A0 (Crafting Popup) ──→ A (Pharmacy server handler)
                    ──→ Also fixes Arrow Crafting (305) client gap

B, C, D are fully independent of each other and of A/A0.

Recommended order: A0 → A → B → C → D
```

### Limitations / Known Gaps

| Gap | Impact | When to Address |
|-----|--------|----------------|
| No 3D mesh actors for summons | SSummonOverlay renders markers/HP/names at projected positions; 3D models are cosmetic upgrade | Optional polish |
| Geographer heal doesn't have visual | Heal effect applies but no visual feedback to healed players | When VFX system covers summons |
| Marine Sphere can't be targeted by enemies | Spheres only auto-detonate or manual detonate — no enemy targeting | Minor — auto-detonate covers 99% of use |
| No "tap to move" for Marine Sphere | In RO you can click spheres to push them 7 cells before detonation | Low priority cosmetic feature |
| Recipe ingredients may not all exist in DB | Need to verify all ingredient items are in the `items` table | Run migration check before Phase A |

### Verification Checklist (RO Classic Compliance)

For each implemented skill, verify against rAthena:

- [ ] Pharmacy: Success rate formula matches `(PotResLv*50)+(PharmLv*300)+(JobLv*20)+(INT/2*10)+(DEX*10)+(LUK*10)+rateMod`
- [ ] Pharmacy: All ingredients consumed on both success AND failure
- [ ] Pharmacy: Medicine Bowl consumed per attempt
- [ ] Pharmacy: Creation Guide checked but NOT consumed
- [ ] Pharmacy: Embryo requires Bioethics skill learned
- [ ] Potion Pitcher: Targets party/guild members or self
- [ ] Potion Pitcher: 3x healing on Homunculus targets
- [ ] CP skills: Range 1 cell (must be adjacent to target)
- [ ] CP skills: Cannot be cast on enemies
- [ ] Summon Flora: Plant type determined by skill level
- [ ] Summon Flora: Max count decreases with level (5/4/3/2/1)
- [ ] Summon Flora: Different type replaces existing plants
- [ ] Summon Flora: HP = 2230 + 200*Lv + 10*BaseLv
- [ ] Summon Flora: Duration = 300/240/180/120/60 seconds
- [ ] Summon Flora: Aggro goes to caster, not plant
- [ ] Summon Flora: Geographer heals allies below 60% HP
- [ ] Marine Sphere: Max 3 active
- [ ] Marine Sphere: 30s duration, detonates on expiry
- [ ] Marine Sphere: Fire element 11x11 AoE
- [ ] Marine Sphere: Damage = remaining HP
- [ ] Marine Sphere: HP = 2000 + 400*Lv
