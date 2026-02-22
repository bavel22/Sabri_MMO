# Socket.io Event Reference — Detailed Payloads

## Player Events

### player:join (Client → Server)
```json
{
    "characterId": 1,
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "characterName": "TestHero"
}
```
**Server Action**: Load character from DB (position, health, stats, equipped weapon), cache position in Redis, store in `connectedPlayers`, send existing enemies, broadcast initial position to others.

### player:joined (Server → Client)
```json
{ "success": true, "zuzucoin": 1500 }
```
**Client Action**: Parse `zuzucoin` → store in `AC_HUDManager.PlayerZuzucoin` for inventory and shop display.

### player:position (Client → Server, ~30Hz)
```json
{
    "characterId": 1,
    "x": 150.5,
    "y": -300.2,
    "z": 300.0
}
```
**Server Action**: Cache in Redis (`setEx` 300s TTL), broadcast `player:moved` to all except sender.

### player:moved (Server → All Except Sender)
```json
{
    "characterId": 1,
    "characterName": "TestHero",
    "x": 150.5,
    "y": -300.2,
    "z": 300.0,
    "health": 95,
    "maxHealth": 100,
    "timestamp": 1739793600000
}
```

### player:left (Server → All)
```json
{
    "characterId": 1,
    "characterName": "TestHero"
}
```
**Trigger**: Socket disconnect. Server saves health/mana/stats to DB, clears auto-attack state, removes from enemy combat sets.

### player:stats (Server → Client)
```json
{
    "characterId": 1,
    "stats": {
        "str": 5, "agi": 3, "vit": 2, "int": 1, "dex": 4, "luk": 1,
        "level": 1, "weaponATK": 17, "statPoints": 42, "hardDef": 15
    },
    "derived": {
        "statusATK": 6, "statusMATK": 1, "hit": 5, "flee": 4,
        "softDEF": 1, "softMDEF": 0, "critical": 0, "aspd": 176,
        "maxHP": 116, "maxSP": 55
    }
}
```
**Sent**: On join, after stat allocation, after equip/unequip, on `player:request_stats`.
**Note**: `aspd` is the final value including weapon modifier (base ASPD + weaponAspdMod, capped at 195). `hardDef` is total DEF from all equipped armor.

### player:request_stats (Client → Server)
_(Empty payload)_

### player:allocate_stat (Client → Server)
```json
{
    "statName": "str",
    "amount": 1
}
```
**Valid stats**: `str`, `agi`, `vit`, `int`, `dex`, `luk`  
**Server Action**: Validate stat name, check points available, increment stat, decrement `statPoints`, recalculate derived, save to DB, emit `player:stats`.

---

## Combat Events

### combat:attack (Client → Server)
```json
// Attack a player:
{ "targetCharacterId": 2 }

// Attack an enemy:
{ "targetEnemyId": 2000001 }
```
**Server Action**: Validate target exists/alive/not-self, set `autoAttackState`, emit `combat:auto_attack_started`. If switching targets, clean up old state and emit `combat:auto_attack_stopped`.

### combat:auto_attack_started (Server → Client)
```json
{
    "targetId": 2000001,
    "targetName": "Blobby",
    "isEnemy": true,
    "attackRange": 150,
    "aspd": 180,
    "attackIntervalMs": 1000
}
```

### combat:auto_attack_stopped (Server → Client)
```json
{
    "reason": "Player stopped",
    "oldTargetId": 2000001,
    "oldIsEnemy": true
}
```
**Reasons**: "Player stopped", "Switched target", "You died", "Target disconnected"

### combat:damage (Server → All)
```json
{
    "attackerId": 1,
    "attackerName": "TestHero",
    "targetId": 2000001,
    "targetName": "Blobby",
    "isEnemy": true,
    "damage": 5,
    "targetHealth": 45,
    "targetMaxHealth": 50,
    "attackerX": 100.0, "attackerY": 200.0, "attackerZ": 300.0,
    "targetX": 150.0, "targetY": 250.0, "targetZ": 300.0,
    "timestamp": 1739793600000
}
```
**Note**: `attackerX/Y/Z` and `targetX/Y/Z` included so remote clients can rotate attacker/target to face each other.

### combat:health_update (Server → All)
```json
{
    "characterId": 1,
    "health": 95,
    "maxHealth": 100,
    "mana": 100,
    "maxMana": 100
}
```
**Sent**: On join (to self + broadcast), after damage, after consumable use, after respawn.

### combat:out_of_range (Server → Client)
```json
{
    "targetId": 2000001,
    "isEnemy": true,
    "targetX": 500.0, "targetY": 500.0, "targetZ": 300.0,
    "distance": 250.5,
    "requiredRange": 100
}
```
**Note**: `requiredRange` = `attackRange - RANGE_TOLERANCE` (so client moves INSIDE range, not to boundary).

### combat:target_lost (Server → Client)
```json
{
    "reason": "Target died",
    "isEnemy": false
}
```
**Reasons**: "Target died", "Target disconnected", "Target respawned", "Enemy died", "Enemy gone"  
**Important**: NOT sent on enemy death (only `enemy:death` is sent) to avoid Blueprint race condition crash.

### combat:death (Server → All)
```json
{
    "killedId": 2,
    "killedName": "OtherPlayer",
    "killerId": 1,
    "killerName": "TestHero",
    "isEnemy": false,
    "targetHealth": 0,
    "targetMaxHealth": 100,
    "timestamp": 1739793600000
}
```
**Side Effects**: Kill message broadcast in COMBAT chat channel, health saved to DB.

### combat:respawn (Client → Server, then Server → All)

Client sends:
_(Empty or minimal payload)_

Server broadcasts:
```json
{
    "characterId": 1,
    "characterName": "TestHero",
    "health": 100,
    "maxHealth": 100,
    "mana": 100,
    "maxMana": 100,
    "x": 0, "y": 0, "z": 300,
    "teleport": true,
    "timestamp": 1739793600000
}
```
**Server Action**: Restore full HP/MP, update Redis position to spawn point, save to DB, stop all attackers targeting this player.

### combat:error (Server → Client)
```json
{ "message": "You are dead" }
```
**Messages**: "You are dead", "Cannot attack yourself", "Target not found", "Target is already dead", "Enemy not found", "Enemy is already dead", "You are not dead", "Not enough stat points", "Invalid stat: X"

---

## Enemy Events

### enemy:spawn (Server → All)
```json
{
    "enemyId": 2000001,
    "templateId": "blobby",
    "name": "Blobby",
    "level": 1,
    "health": 50,
    "maxHealth": 50,
    "x": 500, "y": 500, "z": 300
}
```
**Sent**: On server startup (12 initial enemies), on respawn timer, and to joining players for existing enemies.

### enemy:move (Server → All)
```json
{
    "enemyId": 2000001,
    "x": 520.5, "y": 510.3, "z": 300.0,
    "targetX": 600.0, "targetY": 550.0,
    "isMoving": true
}
```
**Broadcast rate**: Every 200ms during wander movement. `isMoving: false` sent when arrived at destination.

### enemy:death (Server → All)
```json
{
    "enemyId": 2000001,
    "enemyName": "Blobby",
    "killerId": 1,
    "killerName": "TestHero",
    "isEnemy": true,
    "isDead": true,
    "exp": 10,
    "timestamp": 1739793600000
}
```
**Side Effects**: All auto-attackers cleared (without sending `combat:target_lost`), combat set cleared, loot rolled for killer, respawn timer scheduled.

### enemy:health_update (Server → All)
```json
{
    "enemyId": 2000001,
    "health": 45,
    "maxHealth": 50,
    "inCombat": true
}
```

---

## Chat Events

### chat:message (Client → Server)
```json
{
    "channel": "GLOBAL",
    "message": "Hello everyone!"
}
```
**Channels**: Currently only `GLOBAL` implemented. Future: ZONE, PARTY, GUILD, TELL.

### chat:receive (Server → All)
```json
{
    "type": "chat:receive",
    "channel": "GLOBAL",
    "senderId": 1,
    "senderName": "TestHero",
    "message": "Hello everyone!",
    "timestamp": 1739793600000
}
```
**System messages** (kills): `senderId: 0, senderName: "SYSTEM", channel: "COMBAT"`

---

## Inventory Events

### inventory:load (Client → Server)
_(Empty payload)_

### inventory:data (Server → Client)
```json
{
    "items": [
        {
            "inventory_id": 1, "item_id": 3001, "quantity": 1,
            "is_equipped": true, "slot_index": -1,
            "name": "Rustic Shiv", "description": "A crude but swift dagger.",
            "item_type": "weapon", "equip_slot": "weapon",
            "weight": 40, "price": 50, "atk": 17, "def": 0,
            "matk": 0, "mdef": 0,
            "str_bonus": 0, "agi_bonus": 0, "vit_bonus": 0,
            "int_bonus": 0, "dex_bonus": 0, "luk_bonus": 0,
            "max_hp_bonus": 0, "max_sp_bonus": 0,
            "required_level": 1, "stackable": false, "icon": "knife",
            "weapon_type": "dagger", "aspd_modifier": 5, "weapon_range": 150
        }
    ]
}
```

### inventory:use (Client → Server)
```json
{ "inventoryId": 5 }
```

### inventory:used (Server → Client)
```json
{
    "inventoryId": 5,
    "itemId": 1001,
    "itemName": "Crimson Vial",
    "healed": 50,
    "spRestored": 0,
    "health": 100,
    "maxHealth": 100,
    "mana": 100,
    "maxMana": 100
}
```

### inventory:equip (Client → Server)
```json
{
    "inventoryId": 1,
    "equip": true
}
```

### inventory:equipped (Server → Client)
```json
{
    "inventoryId": 1,
    "itemId": 3001,
    "itemName": "Rustic Shiv",
    "equipped": true,
    "slot": "weapon",
    "weaponType": "dagger",
    "attackRange": 150,
    "aspd": 185,
    "attackIntervalMs": 750
}
```

### inventory:drop (Client → Server)
```json
{
    "inventoryId": 5,
    "quantity": 10
}
```

### inventory:dropped (Server → Client)
```json
{
    "inventoryId": 5,
    "itemId": 2001,
    "itemName": "Gloopy Residue",
    "quantity": 10
}
```

### inventory:move (Client → Server)
```json
{
    "sourceInventoryId": 3,
    "targetInventoryId": 7
}
```
**Handler**: Swaps `slot_index` between the two rows. Normalizes all `-1` slot indexes first. Emits `inventory:data` on success.

### hotbar:save (Client → Server)
```json
{
    "slotIndex": 0,
    "inventoryId": 5,
    "itemId": 1001,
    "itemName": "Crimson Vial"
}
```
**Emitted by**: `AC_HUDManager.SendSaveHotbarSlotRequest` (called from `WBP_HotbarSlot.OnDrop`)
**Handler**: Validates slot (0–8), verifies ownership, UPSERTs `character_hotbar`. Pass `inventoryId: 0` to clear a slot.

### hotbar:data (Server → Client)
```json
{
    "slots": [
        {
            "slot_index": 0,
            "inventory_id": 123,
            "item_id": 1001,
            "item_name": "Crimson Vial",
            "quantity": 5
        }
    ]
}
```
**Sent by**: Server automatically after every `inventory:load` response, and 0.6s after `player:join` (delayed to allow HUD initialization).
**Handler**: `BP_SocketManager.OnHotbarData` → `AC_HUDManager.PopulateHotbarFromServer(Data)`
**Note**: Only occupied slots are included. Missing slot indices are treated as empty.

### inventory:error (Server → Client)
```json
{ "message": "Item not found in your inventory" }
```
**Messages**: "Invalid inventory ID", "Item not found in your inventory", "Invalid move request", "Items not found in your inventory", "This item cannot be used", "This item cannot be equipped", "Requires level X", "Item not found", "Failed to use item", "Failed to equip item", "Failed to drop item", "Failed to move item"

---

## Shop Events

### shop:open (Client → Server)
```json
{ "shopId": 1 }
```
**Trigger**: `BP_SocketManager.EmitShop(ShopId)` called from `BP_MMOCharacter` on NPC click.

### shop:data (Server → Client)
```json
{
    "shopId": 1,
    "shopName": "General Store",
    "playerZuzucoin": 1500,
    "items": [
        {
            "itemId": 1001,
            "name": "Crimson Vial",
            "description": "Restores 50 HP",
            "itemType": "consumable",
            "buyPrice": 100,
            "sellPrice": 50,
            "requiredLevel": 1,
            "stackable": true
        }
    ]
}
```
**Client Action**: `AC_HUDManager.OpenShop(Data)` → creates `WBP_Shop`, populates items, sets `PlayerZuzucoin`.

### shop:buy (Client → Server)
```json
{ "shopId": 1, "itemId": 1001, "quantity": 1 }
```

### shop:bought (Server → Client)
```json
{ "itemId": 1001, "itemName": "Crimson Vial", "quantity": 1, "totalCost": 100, "newZuzucoin": 1400 }
```
**Client Action**: `AC_HUDManager.UpdateZuzucoinEverywhere(newZuzucoin)` → updates `WBP_Shop.ZuzucoinText` + `WBP_InventoryWindow.ZuzucoinText`.

### shop:sell (Client → Server)
```json
{ "inventoryId": 42, "quantity": 1 }
```

### shop:sold (Server → Client)
```json
{ "inventoryId": 42, "itemName": "Gloopy Residue", "quantity": 1, "sellPrice": 5, "newZuzucoin": 1405 }
```
**Client Action**: Same as `shop:bought` — calls `UpdateZuzucoinEverywhere`.

### shop:error (Server → Client)
```json
{ "message": "Not enough Zuzucoin (need 100, have 50)" }
```
**Client Action**: `AC_HUDManager.ShowShopError(message)` → sets `WBP_Shop.StatusText`.

---

## Loot Events

### loot:drop (Server → Killer Only)
```json
{
    "enemyId": 2000001,
    "enemyName": "Blobby",
    "items": [
        {
            "itemId": 2001,
            "itemName": "Gloopy Residue",
            "quantity": 2,
            "icon": "jellopy",
            "itemType": "etc"
        },
        {
            "itemId": 1001,
            "itemName": "Crimson Vial",
            "quantity": 1,
            "icon": "red_potion",
            "itemType": "consumable"
        }
    ]
}
```

---

**Last Updated**: 2026-02-22 (Added shop events: shop:open, shop:data, shop:bought, shop:sold, shop:error; updated player:joined to include zuzucoin field)

**Previous**: 2026-02-18
