# Socket.io Event Reference — Detailed Payloads

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Networking_Protocol](../04_Integration/Networking_Protocol.md) | [Multiplayer_Architecture](../01_Architecture/Multiplayer_Architecture.md) | [NodeJS_Server](../03_Server_Side/NodeJS_Server.md)
> **Total**: 106 `socket.on(...)` handlers in `server/src/index.js` (verified 2026-04-15).
> **Note**: This reference covers core events (player, combat, enemy, chat, inventory, shop, hotbar, card, loot). Events added in later phases (homunculus, party, vending, cart, crafting, pet, mount, warp portal, identify, storage, trading, map, ground_item, audio/options, skill targeting) are implemented and broadcast but per-payload docs live in their respective system docs / skills (e.g., `/sabrimmo-storage`, `/sabrimmo-trading`, `/sabrimmo-map`, `/sabrimmo-item-drop-system`). This file focuses on the core MMO event set.

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
**Client Action**: Parse `zuzucoin` → store in `InventorySubsystem` for inventory and shop display.

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

> **Note**: All combat events below are handled directly by `UCombatActionSubsystem` (C++) via `USocketEventRouter`. They are NOT bridged through `MultiplayerEventSubsystem` to Blueprints. BP_SocketManager's combat handler functions (OnCombatDamage, etc.) are dead code as of Phase 2 migration (2026-03-13).

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
    "isCritical": false,
    "isMiss": false,
    "hitType": "normal",
    "element": "neutral",
    "damage2": 3,
    "isDualWield": true,
    "isCritical2": false,
    "element2": "fire",
    "targetHealth": 42,
    "targetMaxHealth": 50,
    "attackerX": 100.0, "attackerY": 200.0, "attackerZ": 300.0,
    "targetX": 150.0, "targetY": 250.0, "targetZ": 300.0,
    "timestamp": 1739793600000
}
```
**Note**: `attackerX/Y/Z` and `targetX/Y/Z` included so remote clients can rotate attacker/target to face each other. **Dual wield fields**: `damage2` = left-hand damage (0 if not dual wielding), `isDualWield` = true when dual wielding, `isCritical2` = cosmetic mirror of right crit, `element2` = left weapon element.
**Client Handler**: `UCombatActionSubsystem::HandleCombatDamage` — disables bOrientRotationToMovement, rotates attacker to face target (yaw only), calls PlayAttackAnimation via property reflection.

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
**Client Handler**: `UCombatActionSubsystem::HandleOutOfRange` — re-enables bOrientRotationToMovement, calls SimpleMoveToLocation toward target with NavMesh projection.

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
**Client Handler**: `UCombatActionSubsystem::HandleCombatDeath` — shows SDeathOverlayWidget (Z=40) for local player death.

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
**Client Handler**: `UCombatActionSubsystem::HandleCombatRespawn` — teleports pawn, ground snap via ZoneTransitionSubsystem::SnapLocationToGround, hides death overlay.

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
    "templateId": "skeleton",
    "name": "Skeleton",
    "level": 15,
    "health": 900,
    "maxHealth": 900,
    "x": 500, "y": 500, "z": 300,
    "spriteClass": "skeleton",
    "weaponMode": 0,
    "spriteTint": [0.3, 0.3, 0.45]
}
```
**Fields**:
- `spriteClass` — atlas manifest name (e.g., `"skeleton"`, `"poring"`, `"eclipse"`). Omitted or `null` for enemies still using the default BP mesh.
- `weaponMode` — 0=unarmed, 1=onehand, 2=twohand, 3=bow. Selects the animation set from the atlas config.
- `spriteTint` — optional `[r,g,b]` multiplier (0.0–1.0 per channel) for recolored variants sharing a parent atlas. Added 2026-04-15. Client applies via `SetLayerTint(ESpriteLayer::Body, ...)` and re-applies on respawn. Omit the field entirely for un-tinted enemies.

**Emitted from 4 server locations** (all carry the same payload shape): `spawnEnemy()` on startup, `processEnemyDeathFromSkill()` respawn, the `player:join` zone loop (client may drop this during `OpenLevel`), and the `zone:ready` zone loop (**this is the one clients actually consume**).

### enemy:move (Server → All)
```json
{
    "enemyId": 2000001,
    "x": 520.5, "y": 510.3, "z": 300.0,
    "targetX": 600.0, "targetY": 550.0,
    "isMoving": true
}
```
**Broadcast rate**: Every 200ms during wander/chase movement. `isMoving: false` sent when stopped (arrived at destination, entered attack state, or gave up chase). Optional `knockback: true` on Fire Wall pushback.

### enemy:attack (Server → All)
```json
{
    "enemyId": 2000001,
    "targetId": 1,
    "attackMotion": 672
}
```
**Trigger**: Enemy AI ATTACK state deals damage to a player. `targetId` is the player character ID. `attackMotion` is the animation duration in ms (from RO template). Client can use this for enemy attack animation playback.

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
**Emitted by**: `UHotbarSubsystem` (called from SHotbarRowWidget drag-drop)
**Handler**: Validates slot (0–8), verifies ownership, UPSERTs `character_hotbar`. Pass `inventoryId: 0` to clear a slot.

### hotbar:alldata (Server → Client)
_(Renamed from `hotbar:data` to avoid C++ SocketIO `NativeClient->OnEvent` overwriting BP handler)_
```json
{
    "slots": [
        {
            "slot_index": 1,
            "inventory_id": 123,
            "item_id": 1001,
            "item_name": "Crimson Vial",
            "quantity": 5,
            "slot_type": "item",
            "skill_id": 0,
            "skill_name": ""
        },
        {
            "slot_index": 3,
            "inventory_id": null,
            "item_id": null,
            "item_name": "",
            "quantity": 0,
            "slot_type": "skill",
            "skill_id": 2,
            "skill_name": "First Aid"
        }
    ]
}
```
**Sent by**: Server automatically after every `inventory:load` response, 0.6s after `player:join`, on `hotbar:request`, and after `hotbar:save_skill`.
**Handler**: `UHotbarSubsystem::HandleHotbarAllData` → populates all 4 hotbar rows from server data
**Note**: Only occupied slots are included. Slot indices are 1-9. `slot_type` is `"item"` or `"skill"`.

### hotbar:save_skill (Client → Server)
```json
{ "slotIndex": 1, "skillId": 2, "skillName": "First Aid" }
```
**Emitted by**: `USkillTreeSubsystem::AssignSkillToHotbar` (from Slate quick-assign buttons)
**Handler**: Validates slot (1–9), verifies skill learned, UPSERTs `character_hotbar` with `slot_type='skill'`, NULLs item fields. Emits `hotbar:alldata`.

### hotbar:request (Client → Server)
_(Empty payload)_
**Emitted by**: `USkillTreeSubsystem` 3s after socket events are wrapped (ensures HUD is initialized)
**Handler**: Re-fetches hotbar from DB and emits `hotbar:alldata`.

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
**Trigger**: `UShopSubsystem` emits when player interacts with `AShopNPC`.

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
**Client Action**: `UShopSubsystem::HandleShopData` → creates `SShopWidget`, populates items, sets zuzucoin.

### shop:buy (Client → Server)
```json
{ "shopId": 1, "itemId": 1001, "quantity": 1 }
```

### shop:bought (Server → Client)
```json
{ "itemId": 1001, "itemName": "Crimson Vial", "quantity": 1, "totalCost": 100, "newZuzucoin": 1400 }
```
**Client Action**: `UShopSubsystem` / `UInventorySubsystem` update zuzucoin display in their respective Slate widgets.

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
**Client Action**: `UShopSubsystem` displays error message in `SShopWidget` status text.

---

## Card Events

### card:compound (Client → Server)
```json
{
    "cardInventoryId": 42,
    "equipInventoryId": 15,
    "slotIndex": 0
}
```
- `cardInventoryId` (int): Inventory ID of the card to compound.
- `equipInventoryId` (int): Inventory ID of the equipment receiving the card.
- `slotIndex` (int): Which card slot on the equipment to insert into (0-based).

### card:result (Server → Client)
```json
{
    "success": true,
    "cardName": "Poring Card",
    "equipmentName": "Sword [3]",
    "slotIndex": 0,
    "message": "Poring Card has been compounded into Sword [3] (slot 0)."
}
```
- `success` (bool): Whether the compounding succeeded.
- `cardName` (string): Display name of the card.
- `equipmentName` (string): Display name of the target equipment.
- `slotIndex` (int): Which slot the card was inserted into.
- `message` (string): Human-readable result message for UI display.

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

## Skill Events

### skill:data (Client → Server → Client)
**Request**: `{}` (empty payload)
**Response**:
```json
{
    "characterId": 1,
    "jobClass": "swordsman",
    "skillPoints": 5,
    "skillTree": {
        "novice": [
            {
                "skillId": 1, "name": "basic_skill", "displayName": "Basic Skill",
                "maxLevel": 9, "currentLevel": 9, "type": "passive",
                "targetType": "none", "element": "neutral", "range": 0,
                "description": "Enables basic commands.", "icon": "basic_skill",
                "treeRow": 0, "treeCol": 0, "prerequisites": [],
                "spCost": 0, "nextSpCost": 0, "castTime": 0, "cooldown": 0,
                "effectValue": 0, "canLearn": false
            }
        ],
        "swordsman": [
            {
                "skillId": 103, "name": "bash", "displayName": "Bash",
                "maxLevel": 10, "currentLevel": 3, "type": "active",
                "targetType": "single", "element": "neutral", "range": 150,
                "description": "Smash a target for increased damage.",
                "icon": "bash", "treeRow": 0, "treeCol": 1,
                "prerequisites": [], "spCost": 8, "nextSpCost": 8,
                "castTime": 0, "cooldown": 700, "effectValue": 190, "canLearn": true
            }
        ]
    },
    "learnedSkills": { "1": 9, "103": 3 }
}
```
**Server Action**: Load character skills from DB, build skill tree grouped by class, compute `canLearn` per skill.
**Client Action**: `SkillTreeSubsystem` parses into `SkillGroups` array, broadcasts `OnSkillDataUpdated`.

### skill:learn (Client → Server)
```json
{ "skillId": 103 }
```
**Server Action**: Validate class access, prerequisites, skill points > 0, not max level. On success: upsert `character_skills`, deduct 1 skill point, emit `skill:learned`.

### skill:learned (Server → Client)
```json
{
    "skillId": 103, "skillName": "Bash",
    "newLevel": 4, "maxLevel": 10, "skillPoints": 4
}
```
**Client Action**: Update `LearnedSkills` map, request full `skill:data` refresh.

### skill:refresh (Server → Client)
```json
{ "skillPoints": 4 }
```
**Client Action**: Update `SkillPoints`, request full `skill:data` refresh.

### skill:reset (Client → Server)
**Payload**: `{}` (empty)
**Server Action**: Sum all invested skill point levels, delete all `character_skills` rows, refund points.

### skill:reset_complete (Server → Client)
```json
{ "skillPoints": 15, "refundedPoints": 10 }
```

### skill:use (Client → Server)
```json
{ "skillId": 2 }
```
**Server Action**: Validate skill exists/learned/SP available. Deduct SP. Apply effect (First Aid heals `effectValue` HP). Emit `skill:used` + `combat:health_update`.

### skill:used (Server → Client)
```json
{
    "skillId": 2,
    "skillName": "First Aid",
    "level": 1,
    "spCost": 3,
    "remainingMana": 97,
    "maxMana": 100
}
```

### skill:error (Server → Client)
```json
{ "message": "Requires Bash level 5 (current: 0)" }
```
**Messages**: "Invalid skill ID", "Unknown skill", "Skill not learned", "Not enough SP (need X, have Y)", "Requires [Skill] level X (current: Y)"

---

## Weight Events

### weight:status (Server → Client)
```json
{
    "characterId": 1,
    "currentWeight": 1500,
    "maxWeight": 3500,
    "ratio": 0.429,
    "isOverweight50": false,
    "isOverweight90": false,
    "isOverweight100": false
}
```
**Sent**: On player join (after `player:stats`), after any inventory mutation that changes weight, after STR stat allocation, after learning Enlarge Weight Limit (skill 600).

**Weight Thresholds**:
- `< 50%`: Normal (full regen, all actions)
- `50-89%`: Natural HP/SP/skill regen stops
- `>= 90%`: Cannot attack or use skills (items still usable)
- `> 100%`: Cannot pick up loot from enemy kills

**Note**: `inventory:data` also includes `currentWeight` and `maxWeight` fields in every emission.

**Error Messages (from weight blocks)**:
- `combat:error`: `"Too heavy to attack! Reduce weight below 90%."`
- `skill:error`: `"Too heavy to use skills! Reduce weight below 90%."`
- `skill:cast_failed`: `{ skillId, reason: "Overweight" }` (mid-cast weight check)
- `combat:auto_attack_stopped`: `{ reason: "Overweight" }` (auto-attack interrupted)

---

**Last Updated**: 2026-03-13 (Added CombatActionSubsystem client handler notes for all combat events, marked BP combat handlers as dead code)

**Previous**: 2026-03-12 (Added Weight Events section: weight:status, overweight error messages)
