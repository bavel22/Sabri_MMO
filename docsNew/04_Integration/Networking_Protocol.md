# Networking Protocol

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Multiplayer_Architecture](../01_Architecture/Multiplayer_Architecture.md) | [Event_Reference](../06_Reference/Event_Reference.md) | [Authentication_Flow](Authentication_Flow.md)

## Overview

Sabri_MMO uses two networking protocols:
1. **HTTP REST** — Authentication, character CRUD, position persistence (via `UHttpManager` C++)
2. **Socket.io** — Real-time game events (via SocketIOClient UE5 plugin)

Both communicate over TCP port 3001 to the same Node.js server.

## JSON Communication Format

All data between client and server is JSON. The UE5 client sends/receives JSON via `FSocketIONative` on `UMMOGameInstance` (persistent socket), with C++ subsystems registering handlers through `USocketEventRouter`. REST calls use `UMMOHttpManager` (BlueprintFunctionLibrary).

### Position Update (Client → Server)
```json
{
    "characterId": 1,
    "x": 150.5,
    "y": -300.2,
    "z": 300.0
}
```

### Position Broadcast (Server → All Clients)
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

### Combat Damage (Server → All Clients)
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

### Enemy Spawn (Server → All Clients)
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

### Loot Drop (Server → Killer Only)
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
        }
    ]
}
```

### Inventory Data (Server → Client)
```json
{
    "items": [
        {
            "inventory_id": 1,
            "item_id": 3001,
            "quantity": 1,
            "is_equipped": true,
            "slot_index": -1,
            "name": "Rustic Shiv",
            "description": "A crude but swift dagger.",
            "item_type": "weapon",
            "equip_slot": "weapon",
            "weight": 40,
            "price": 50,
            "atk": 17,
            "def": 0,
            "required_level": 1,
            "stackable": false,
            "icon": "knife",
            "weapon_type": "dagger",
            "aspd_modifier": 5,
            "weapon_range": 150
        }
    ],
    "zuzucoin": 1500,
    "currentWeight": 1200,
    "maxWeight": 3500
}
```

### Card Compound (Client → Server)
```json
{
    "cardInventoryId": 456,
    "equipInventoryId": 123,
    "slotIndex": 0
}
```
- **cardInventoryId**: `inventory_id` of the card to compound
- **equipInventoryId**: `inventory_id` of the target equipment
- **slotIndex**: which card slot to insert into (0-based, max = equipment's `slots - 1`)

### Card Compound Result (Server → Client)
```json
{
    "success": true,
    "cardName": "Hydra Card",
    "equipmentName": "Blade [4]",
    "slotIndex": 0,
    "message": "Hydra Card compounded into Blade [4] [slot 1]"
}
```
- **success**: whether the compound succeeded
- **cardName**: display name of the card that was compounded
- **equipmentName**: display name of the target equipment
- **slotIndex**: the slot index the card was inserted into
- **message**: human-readable result message

On success, the server also emits `inventory:data` to refresh the client inventory. If the target equipment is currently equipped, a `player:stats` update is also sent to reflect any stat changes from the newly compounded card.

### Homunculus Events

#### Client → Server
| Event | Payload | Description |
|-------|---------|-------------|
| `homunculus:feed` | `{ characterId }` | Feed homunculus its type-specific food item |
| `homunculus:command` | `{ characterId, command }` | Issue command: `follow`, `attack`, `standby` |
| `homunculus:skill_up` | `{ characterId, skillId }` | Allocate a skill point to a homunculus skill |

#### Server → Client (owner only)
| Event | Payload | Description |
|-------|---------|-------------|
| `homunculus:summoned` | `{ homunculusId, type, name, level, hp, maxHp, sp, maxSp, ... }` | Homunculus created or re-summoned |
| `homunculus:vaporized` | `{ homunculusId }` | Homunculus vaporized (Rest skill) |
| `homunculus:died` | `{ homunculusId }` | Homunculus killed in combat |
| `homunculus:resurrected` | `{ homunculusId, hp, maxHp }` | Homunculus revived (Resurrect Homunculus skill) |
| `homunculus:fed` | `{ homunculusId, hunger, intimacy, fullness }` | Feed result with updated hunger/intimacy |
| `homunculus:leveled_up` | `{ homunculusId, level, stats }` | Homunculus gained a level |
| `homunculus:hunger_tick` | `{ homunculusId, hunger, intimacy }` | Periodic hunger decay (60s interval) |
| `homunculus:command_ack` | `{ command, success }` | Acknowledgement of a command |
| `homunculus:skill_updated` | `{ homunculusId, skillId, level }` | Skill point allocation confirmed |

#### Server → Zone (all players in zone)
| Event | Payload | Description |
|-------|---------|-------------|
| `homunculus:attack` | `{ homunculusId, targetId, damage }` | Homunculus auto-attack damage |
| `homunculus:other_summoned` | `{ ownerId, homunculusId, type, name, x, y, z }` | Another player's homunculus appeared |
| `homunculus:other_dismissed` | `{ ownerId, homunculusId }` | Another player's homunculus dismissed |

### Player Stats (Server → Client)
```json
{
    "characterId": 1,
    "stats": {
        "str": 5, "agi": 3, "vit": 2, "int": 1, "dex": 4, "luk": 1,
        "level": 1, "weaponATK": 17, "statPoints": 42
    },
    "derived": {
        "statusATK": 6, "statusMATK": 1, "hit": 5, "flee": 4,
        "softDEF": 1, "softMDEF": 0, "critical": 0, "aspd": 171,
        "maxHP": 116, "maxSP": 55
    }
}
```

### Chat Message (Server → All Clients)
```json
{
    "type": "chat:receive",
    "channel": "GLOBAL",
    "senderId": 1,
    "senderName": "TestHero",
    "message": "Hello world!",
    "timestamp": 1739793600000
}
```

## Socket.io Emit Patterns

| Pattern | Node.js Code | Who Receives |
|---------|-------------|-------------|
| **To sender only** | `socket.emit(event, data)` | The client that sent the request |
| **To all except sender** | `socket.broadcast.emit(event, data)` | All connected clients except sender |
| **To all clients** | `io.emit(event, data)` | Every connected client including sender |
| **To specific client** | `io.sockets.sockets.get(socketId).emit(event, data)` | One specific client by socket ID |

## ID Conventions

| ID Type | Range | Example |
|---------|-------|---------|
| User ID | 1+ | Auto-increment from `users` table |
| Character ID | 1+ | Auto-increment from `characters` table |
| Enemy ID | 2,000,001+ | Server-assigned, starts at `nextEnemyId = 2000001` |
| Inventory ID | 1+ | Auto-increment from `character_inventory` table |
| Item ID | 1001–4999 | Seeded ranges: 1001–1005 (consumable), 2001–2008 (etc), 3001–3006 (weapon), 4001–4003 (armor) |

**Critical**: All IDs from client are parsed with `parseInt()` on the server to prevent type mismatch bugs.

### Weight Status (Server → Client)
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
Sent on join, inventory mutations, STR allocation, Enlarge Weight Limit learned. Thresholds: 50% stops regen, 90% blocks attack/skills, >100% blocks loot pickup.

## Error Event Format

Combat and inventory errors use dedicated error events:

```json
// combat:error
{ "message": "You are dead" }

// inventory:error
{ "message": "Item not found in your inventory" }
```

## Client-Side Event Routing

Combat events (`combat:damage`, `combat:auto_attack_started/stopped`, `combat:target_lost`, `combat:out_of_range`, `combat:death`, `combat:respawn`, `combat:error`, `combat:health_update`, `enemy:health_update`) are handled directly by `UCombatActionSubsystem` (C++) via `USocketEventRouter`. They are NOT bridged through `MultiplayerEventSubsystem` to Blueprints.

All other game events (enemy:spawn/move/death/attack, player:moved/left, inventory/shop/skill/chat/loot/zone events) are handled by dedicated C++ subsystems via `USocketEventRouter`. All BP_SocketManager bridges have been removed (0 bridges remaining).

---

**Last Updated**: 2026-03-14 (Updated: all BP_SocketManager bridges removed, 0 remaining)
