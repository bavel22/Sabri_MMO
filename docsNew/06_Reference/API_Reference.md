# API Reference — Quick Lookup

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [API_Documentation](../03_Server_Side/API_Documentation.md) | [Authentication_Flow](../04_Integration/Authentication_Flow.md)
> **Total**: 11 REST endpoints

## REST Endpoints

| Method | URL | Auth | Purpose |
|--------|-----|------|---------|
| `GET` | `/health` | No | Server health check |
| `POST` | `/api/auth/register` | No | Create account |
| `POST` | `/api/auth/login` | No | Login, get JWT |
| `GET` | `/api/auth/verify` | JWT | Verify token |
| `GET` | `/api/characters` | JWT | List user's characters |
| `POST` | `/api/characters` | JWT | Create character |
| `GET` | `/api/characters/:id` | JWT | Get single character |
| `PUT` | `/api/characters/:id/position` | JWT | Save position |
| `GET` | `/api/test` | No | Simple test endpoint |

## Socket.io Events — Complete Catalog

### Player Events
| Event | Dir | Payload Keys |
|-------|-----|-------------|
| `player:join` | C→S | characterId, token, characterName |
| `player:joined` | S→C | success |
| `player:position` | C→S | characterId, x, y, z |
| `player:moved` | S→C* | characterId, characterName, x, y, z, health, maxHealth, timestamp |
| `player:left` | S→All | characterId, characterName |
| `player:stats` | S→C | characterId, stats, derived |
| `player:request_stats` | C→S | _(empty)_ |
| `player:allocate_stat` | C→S | statName, amount |

### Combat Events
| Event | Dir | Payload Keys |
|-------|-----|-------------|
| `combat:attack` | C→S | targetCharacterId OR targetEnemyId |
| `combat:stop_attack` | C→S | _(empty)_ |
| `combat:auto_attack_started` | S→C | targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs |
| `combat:auto_attack_stopped` | S→C | reason, oldTargetId?, oldIsEnemy? |
| `combat:damage` | S→All | attackerId, attackerName, targetId, targetName, isEnemy, damage, targetHealth, targetMaxHealth, attackerX/Y/Z, targetX/Y/Z, timestamp |
| `combat:health_update` | S→All | characterId, health, maxHealth, mana, maxMana |
| `combat:out_of_range` | S→C | targetId, isEnemy, targetX/Y/Z, distance, requiredRange |
| `combat:target_lost` | S→C | reason, isEnemy |
| `combat:death` | S→All | killedId, killedName, killerId, killerName, isEnemy, targetHealth, targetMaxHealth, timestamp |
| `combat:respawn` | C→S / S→All | characterId, characterName, health, maxHealth, mana, maxMana, x, y, z, teleport, timestamp |
| `combat:error` | S→C | message |

### Enemy Events
| Event | Dir | Payload Keys |
|-------|-----|-------------|
| `enemy:spawn` | S→All | enemyId, templateId, name, level, health, maxHealth, x, y, z |
| `enemy:move` | S→All | enemyId, x, y, z, targetX?, targetY?, isMoving |
| `enemy:death` | S→All | enemyId, enemyName, killerId, killerName, isEnemy, isDead, exp, timestamp |
| `enemy:health_update` | S→All | enemyId, health, maxHealth, inCombat |

### Chat Events
| Event | Dir | Payload Keys |
|-------|-----|-------------|
| `chat:message` | C→S | channel, message |
| `chat:receive` | S→All | type, channel, senderId, senderName, message, timestamp |

### Inventory Events
| Event | Dir | Payload Keys |
|-------|-----|-------------|
| `inventory:load` | C→S | _(empty)_ |
| `inventory:data` | S→C | items[] |
| `inventory:use` | C→S | inventoryId |
| `inventory:used` | S→C | inventoryId, itemId, itemName, healed, spRestored, health, maxHealth, mana, maxMana |
| `inventory:equip` | C→S | inventoryId, equip |
| `inventory:equipped` | S→C | inventoryId, itemId, itemName, equipped, slot, weaponType, attackRange, aspd, attackIntervalMs |
| `inventory:drop` | C→S | inventoryId, quantity? |
| `inventory:dropped` | S→C | inventoryId, itemId, itemName, quantity |
| `inventory:error` | S→C | message |

### Loot Events
| Event | Dir | Payload Keys |
|-------|-----|-------------|
| `loot:drop` | S→C | enemyId, enemyName, items[] |

**Legend**: C→S = Client to Server, S→C = Server to Client, S→C* = Server to all except sender, S→All = Server to all clients

---

**Last Updated**: 2026-02-17
