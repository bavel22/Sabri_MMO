# weight system implementation

weight system implementation


claude --resume de9a6613-ed8d-4b7c-ba35-0f2db306dc59

 Implementation Order

 1. Constants + helper functions (Steps 1–2)
 2. Initialize on join (Step 3)
 3. Regen blocks (Step 5) — easiest to test, low risk
 4. Combat/skill blocks (Steps 6–7) — core gameplay change
 5. Loot pickup block (Step 8)
 6. Mutation point cache updates (Step 4) — makes all above work with live data
 7. weight:status event + STR change (Steps 9, 10)
 8. Refactor duplicate SQL (Step 11)

 Verification

 1. Start server (npm run dev), join with a character
 2. Check logs: should see [WEIGHT] CharName weight: X/Y on join
 3. Buy items from shop until >= 50% weight → verify HP/SP regen stops (watch server logs for regen ticks; they should skip the character)
 4. Buy more items until >= 90% → try to attack an enemy → expect combat:error message. Try to cast a skill → expect skill:error
 5. Sell items back below 90% → attacks and skills should work again
 6. Sell items below 50% → regen should resume
 7. Fill to > 100% (via DB insert if needed) → kill an enemy with loot → loot should NOT be added to inventory
 8. Allocate STR points → verify weight:status updates with new maxWeight




 Weight Threshold System — Implementation Complete

  All 11 steps implemented in server/src/index.js:

  Constants (Step 1): INVENTORY.OVERWEIGHT_50 (0.5) and INVENTORY.OVERWEIGHT_90 (0.9) added.

  Helper functions (Step 2): 5 new functions after getPlayerMaxWeight():
  - getWeightRatio(player) — cached ratio, zero DB queries
  - getWeightStatus(player) — full status object with threshold flags
  - calculatePlayerCurrentWeight(characterId) — DB query (SUM of inventory weights)
  - updatePlayerWeightCache(characterId, player) — recalculate + emit if threshold crossed
  - emitWeightStatus(characterId, player) — sends weight:status event

  Player join (Step 3): currentWeight: 0 on player object, calculated from DB after card rebuild, logged on join, weight:status emitted after player:stats.

  Inventory mutations (Step 4): updatePlayerWeightCache() called at 12 mutation points: skill loot, combat tick loot, Fly Wing, Butterfly Wing, consumable use, card
  compound, item drop, shop:buy, shop:buy_batch, shop:sell, shop:sell_batch.

  Regen blocks (Step 5): All 3 regen intervals (HP/6s, SP/8s, skill/10s) skip at >= 50% weight.

  Attack block (Step 6): combat:attack handler rejects + combat tick stops auto-attack at >= 90%.

  Skill block (Step 7): skill:use handler rejects + executeCastComplete re-checks at >= 90%.

  Loot block (Step 8): Both loot paths (skill kill + combat tick kill) check per-item weight before pickup. Skipped items log [WEIGHT] message. In-loop cached weight
  increment prevents exceeding limit across multi-drop rolls.

  STR/skill change (Step 9): emitWeightStatus after STR allocation and Enlarge Weight Limit (skill 600) learning.

  inventory:data augmented (Step 10): All 14 inventory:data emissions now include currentWeight and maxWeight fields.

  Duplicate SQL removed (Step 11): shop:open and shop:buy_batch now use player.currentWeight cache instead of on-demand DB queries (saves 2 DB queries per shop
  interaction).
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
