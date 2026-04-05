# Dead Code Detection Audit — Server

**Date**: 2026-03-23
**Scope**: `server/src/index.js` (32,566 lines), all `ro_*.js` data modules (5,910 lines), standalone scripts
**Method**: 7-pass analysis — function catalog, call graph, unreachable code, duplicates, commented-out code, unused module exports, disabled feature flags

---

## Summary

| Category | Items | Est. Lines |
|----------|------:|----------:|
| Dead functions (defined, never called) | 7 | ~80 |
| Dead function chain (only caller is itself dead) | 1 | ~16 |
| Unused imports (imported, never referenced) | 6 | ~6 |
| Dead constant (defined, never referenced) | 1 | ~6 |
| Unnecessary wrapper (1-line pass-through) | 1 | ~3 |
| Unused data module files (never required) | 3 | ~11,588 |
| Unused data module exports (exported, never imported) | 10 | ~128 |
| PvP-guarded dead code (behind `PVP_ENABLED = false`) | 5 blocks | ~236 |
| **Total estimated dead/dormant code** | | **~12,063** |

**Bug found during audit**: Line 3775 calls `calculateASPD(player)` which is neither defined in `index.js` nor imported — runtime crash if `recalcPetBonuses()` executes.

---

## Pass 1-2: Dead Functions in `index.js`

### Completely Dead (defined, zero calls)

| Function | Line | Size | Notes |
|----------|-----:|-----:|-------|
| `applyStatusWithDevotionCheck()` | 1633 | 8 | Wrapper for `applyStatusEffect` + Devotion CC break. Written but all 49 `applyStatusEffect` call sites use the direct function instead. |
| `processCardMagicReflection()` | 3315 | 9 | Returns reflected magic damage (Maya Card). Comment says "Phase 4" but magic reflection is never applied in any damage path. |
| `canSeeHiddenTarget()` | 3324 | 11 | Checks `cardIntravision` (Maya Purple Card). Written but never called from combat/targeting code. |
| `trackEnemyCombatDamage()` | 28656 | 11 | Tracks per-player damage on an enemy. Superseded by inline logic in `setEnemyAggro()` which does the same `inCombatWith.set()` at line 28673. |
| `getPlayerZone()` | 5121 | 4 | Returns player zone from `connectedPlayers` map. Never called — all callers read `player.zone` directly. |
| `isZoneActive()` | 5125 | 7 | Checks if any player is in a zone. Never called — `getActiveZones()` (which IS used) provides the same info as a Set. |
| `removePlayerPosition()` | 31945 | 6 | Redis position cleanup. Never called — `redisClient.del()` is called inline at disconnect. |
| `getPlayersInZone()` | 31951 | 17 | Redis-based zone player lookup. Never called — the `connectedPlayers` Map is used directly everywhere. |

### Dead Chain (only called from dead code)

| Function | Line | Size | Called By |
|----------|-----:|-----:|-----------|
| `handleDevotionCCBreak()` | 1617 | 16 | Only called from `applyStatusWithDevotionCheck()` (line 1636), which is itself dead. |

**Subtotal: 8 dead functions, ~89 lines**

---

## Pass 2 (continued): Unused Imports

These names are destructured from `require()` but never referenced anywhere in `index.js`:

| Import | Source Module | Line |
|--------|-------------|-----:|
| `BREAKABLE_STATUSES` | `ro_status_effects.js` | 50 |
| `getMonsterSkills` | `ro_monster_skills.js` | 36 |
| `isPlayerClassSkill` | `ro_monster_skills.js` | 36 |
| `PET_DB` | `ro_pet_data.js` | 74 |
| `getPetByEggItem` | `ro_pet_data.js` | 74 |
| `CLASS_SKILLS` | `ro_skill_data.js` | 30 |

**Impact**: No runtime cost (unused destructured properties are a no-op in Node.js), but adds cognitive overhead and false positives in search results.

---

## Pass 2 (continued): Dead Constants

| Constant | Line | Size | Notes |
|----------|-----:|-----:|-------|
| `SPEED_TONICS` | 446 | 6 | Custom ASPD potion data with non-RO item names (`veil_draught`, `dusk_tincture`, `ember_salve`, `grey_catalyst`). Never referenced. Superseded by the RO-authentic `aspd_potion` buff system in `ro_buff_system.js`. |

---

## Pass 2 (continued): Unnecessary Wrapper

| Function | Line | Wraps | Called |
|----------|-----:|-------|------:|
| `calculateDerivedStats()` | 2562 | `return roDerivedStats(stats);` | ~20 sites |

This is a pure pass-through to `roDerivedStats` (imported from `ro_damage_formulas.js`). The 20 call sites could use `roDerivedStats` directly, but since the wrapper is thin (3 lines including `}`) and all call sites use the wrapper consistently, this is low priority. Listed for completeness.

---

## Pass 3: Unreachable Code

**No genuine unreachable code found.** All `return` statements followed by code are early-return guard clauses (`if (!x) return;`) where the subsequent code is the main function body — standard pattern.

No `if (false)` blocks, no code after unconditional `return`/`throw` at the same indentation level, no impossible switch cases found.

---

## Pass 4: Duplicate Logic

### `trackEnemyCombatDamage()` vs inline in `setEnemyAggro()`

`trackEnemyCombatDamage()` (line 28656) and `setEnemyAggro()` (line 28673) both write to `enemy.inCombatWith`. The dedicated function was likely written first, then `setEnemyAggro()` absorbed the same logic inline. The standalone function became dead.

### ASPD Calculation Pattern (duplicated ~15 times)

```js
const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
```

This exact expression appears 15 times across `index.js`. Not dead code per se, but a strong candidate for extraction into a helper to reduce duplication and prevent drift (e.g., the `calculateASPD` bug at line 3775 exists because this pattern isn't centralized).

### Ground Effect Functions (index.js vs ro_ground_effects.js)

`index.js` defines its own ground effect management at lines 571-637:
- `createGroundEffect()` — 22 lines
- `removeGroundEffect()` — 1 line
- `getGroundEffectsAtPosition()` — 10 lines
- `countGroundEffects()` — 6 lines
- `removeOldestGroundEffect()` — 17 lines

Meanwhile, `ro_ground_effects.js` exports equivalent functions that are never imported. The two implementations are not identical — the `index.js` versions are simpler and tightly coupled to the local `activeGroundEffects` Map, while `ro_ground_effects.js` has a more complete API with immunity tracking, tick processing, and trap triggering. The module API was apparently written as a future refactor target that was never adopted.

---

## Pass 5: Commented-Out Code

**No significant commented-out code blocks found.** The ~30 comment lines containing code-like syntax are all rAthena/eAthena source references (e.g., `// rAthena: rate = 5+5*skilllv`) serving as documentation of the original formulas. These are valuable reference comments, not dead code.

---

## Pass 6: Unused Data Module Exports & Files

### Completely Unused Files (never `require()`d)

| File | Lines | Notes |
|------|------:|-------|
| `ro_item_mapping.js` | 357 | Explicitly marked as replaced at line 4543: "Runtime name->id lookup built from itemDefinitions (replaces ro_item_mapping.js)". The runtime `itemNameToId` Map (built from DB) serves the same purpose. |
| `ro_card_prefix_suffix.js` | 541 | Card prefix/suffix name data. Not required by any `.js` file in the project. Likely written for a planned card naming display feature. |
| `ro_monsters_summary.json` | 10,690 | Monster data summary. Not required by any file. Appears to be a reference/export document, not runtime data. |

**Subtotal: 3 files, ~11,588 lines** (though `ro_monsters_summary.json` is data, not code)

### Unused Exports from `ro_ground_effects.js`

Only `calculateTrapDamage` is imported. These 8 exports are never used:

| Export | Lines | Size | Notes |
|--------|------:|-----:|-------|
| `processGroundEffectTick()` | 428-496 | 69 | Complete tick loop with expiry, wave tracking, onTick callbacks. `index.js` implements ground effect ticking inline in its main tick loops instead. |
| `findEntitiesInEffect()` | 389-419 | 31 | Entity-in-radius query. `index.js` does inline distance checks in each ground effect handler. |
| `hasImmunity()` | 363-368 | 6 | Immunity window check. |
| `recordHit()` | 375-377 | 3 | Immunity hit recording. |
| `shouldTrapTrigger()` | 507-514 | 8 | Trap activation check. |
| `triggerTrap()` | 519-523 | 5 | Trap state mutation. |
| `getActiveEffects()` | 528-530 | 3 | Map accessor. |
| `getEffect()` | 535-537 | 3 | Single effect lookup. |

**Subtotal: ~128 lines of unused module code**

### Unused Exports from `ro_damage_formulas.js`

| Export | Notes |
|--------|-------|
| `calculateMaxHP()` | Exported but never imported. HP/SP calculations are done differently in `index.js`. |
| `calculateMaxSP()` | Same — exported but never imported. |

These functions exist in the module but are not destructured in the `require()` at line 40-47, so they have zero impact on `index.js`.

---

## Pass 7: PvP-Guarded Dead Code

`PVP_ENABLED = false` (line 161) gates 5 code blocks that are currently unreachable:

| Location | Lines | Size | Context |
|----------|------:|-----:|---------|
| 9967-10030 | 64 | Magnum Break PvP splash damage on nearby players |
| 10480-10492 | 13 | Napalm Beat PvP splash on players in range |
| 10685-10695 | 11 | Frost Diver PvP targeting of players |
| 10820-10897 | 78 | Thunderstorm PvP damage on players in AoE |
| 27805-27869 | 65 | Fire Wall PvP damage on players entering zone |

Plus 5 scattered guard lines (early returns when `!PVP_ENABLED`).

**Subtotal: ~236 lines of unreachable PvP code**

**Recommendation**: Keep but mark with `// [PVP-FUTURE]` comments. This code is intentionally written for future PvP activation and follows the correct patterns (zone filtering, damage calc, status application). Removing it would require reimplementation when PvP is enabled.

---

## Bug Found: Undefined Function Call

**File**: `server/src/index.js`
**Line**: 3775
**Severity**: Runtime crash (ReferenceError)

```js
function recalcPetBonuses(player, characterId) {
    player.petBonuses = getPetBonuses(characterId);
    const socket = io.sockets.sockets.get(player.socketId);
    if (socket) {
        const effectiveStats = getEffectiveStats(player);
        const derived = calculateDerivedStats(player.stats, player);
        const finalAspd = calculateASPD(player);  // <-- UNDEFINED
        socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd));
    }
}
```

`calculateASPD` is exported from `ro_damage_formulas.js` but never imported in `index.js`. Every other ASPD calculation in the file uses the inline pattern:
```js
const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
```

This will throw `ReferenceError: calculateASPD is not defined` whenever a pet's bonuses are recalculated (pet feed, pet equip change, pet intimacy change).

**Fix**: Replace line 3775 with:
```js
const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
```

---

## Actionable Recommendations

### Priority 1: Fix Bug (immediate)
1. **Fix `calculateASPD` crash** at line 3775 — replace with inline ASPD pattern

### Priority 2: Remove Dead Functions (safe, no behavioral change)
2. Remove `applyStatusWithDevotionCheck()` (line 1633-1639)
3. Remove `handleDevotionCCBreak()` (line 1617-1632) — only caller is #2
4. Remove `processCardMagicReflection()` (line 3315-3318)
5. Remove `canSeeHiddenTarget()` (line 3324-3329)
6. Remove `trackEnemyCombatDamage()` (line 28656-28664)
7. Remove `getPlayerZone()` (line 5121-5124)
8. Remove `isZoneActive()` (line 5125-5131)
9. Remove `removePlayerPosition()` (line 31945-31949)
10. Remove `getPlayersInZone()` (line 31951-31967)

### Priority 3: Clean Up Dead Constants & Imports (safe, cosmetic)
11. Remove `SPEED_TONICS` constant (line 446-451)
12. Remove unused imports: `BREAKABLE_STATUSES`, `getMonsterSkills`, `isPlayerClassSkill`, `PET_DB`, `getPetByEggItem`, `CLASS_SKILLS`

### Priority 4: Remove Dead Files (safe, reduces repo size)
13. Delete `server/src/ro_item_mapping.js` (357 lines, explicitly replaced)
14. Delete `server/src/ro_monsters_summary.json` (10,690 lines, unreferenced data)
15. Consider deleting `server/src/ro_card_prefix_suffix.js` (541 lines) — or mark as "future: card naming display"
16. Consider deleting `server/check_ranges.js` (standalone dev script, not part of server)

### Priority 5: Reduce Duplication (improves maintainability)
17. Extract ASPD calculation into a helper: `function getFinalASPD(player, derived) { return Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0)); }` — would eliminate 15 repetitions
18. Consider adopting `ro_ground_effects.js` API or removing its unused exports to reduce confusion about which ground effect system is canonical

### Priority 6: Future PvP Code (keep, annotate)
19. Add `// [PVP-FUTURE]` markers to the 5 PvP-guarded blocks for clarity

---

## Methodology Notes

- **156 top-level function definitions** were cataloged from `index.js`
- Each function name was searched for all references (definition + calls + comments)
- Functions with reference count = 1 (definition only) were confirmed dead
- Functions with count = 2 were manually verified to distinguish definition+call from definition+comment
- All `require()` statements were cross-referenced with their destructured exports
- All 22 `server/src/*.js` files were checked for cross-references
- No false positives: every item listed was verified by reading the surrounding code
