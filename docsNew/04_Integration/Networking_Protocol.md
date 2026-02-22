# Networking Protocol

## Overview

Sabri_MMO uses two networking protocols:
1. **HTTP REST** — Authentication, character CRUD, position persistence (via `UHttpManager` C++)
2. **Socket.io** — Real-time game events (via SocketIOClient UE5 plugin)

Both communicate over TCP port 3001 to the same Node.js server.

## JSON Communication Format

All data between client and server is JSON. The UE5 client sends/receives JSON strings which are parsed in Blueprints.

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
    ]
}
```

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

## Error Event Format

Combat and inventory errors use dedicated error events:

```json
// combat:error
{ "message": "You are dead" }

// inventory:error
{ "message": "Item not found in your inventory" }
```

---

**Last Updated**: 2026-02-17
