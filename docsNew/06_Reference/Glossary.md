# Glossary

## Game Terms

| Term | Definition |
|------|-----------|
| **ASPD** | Attack Speed. Scale 0–190 where higher is faster. ASPD 180 = 1 attack/sec, 190 = 2/sec. Derived from AGI + DEX. |
| **Auto-Attack** | RO-style: click target once, character paths to target and attacks at ASPD intervals until stopped. |
| **Base Stats** | The 6 allocatable stats: STR, AGI, VIT, INT, DEX, LUK. |
| **Derived Stats** | Stats calculated from base stats: statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, ASPD, maxHP, maxSP. |
| **Drop Table** | Per-enemy list of items with chance percentages, rolled on enemy death. |
| **RO-Style** | Game mechanics inspired by Ragnarok Online (2002 MMORPG). |
| **Server-Authoritative** | All game state (combat, stats, inventory) is computed and validated on the server. Clients send inputs; server sends results. |
| **Stat Points** | Points available for allocation to base stats. New characters start with 48. |

## Architecture Terms

| Term | Definition |
|------|-----------|
| **Blueprint (BP)** | UE5 visual scripting asset. Prefix: `BP_` for actors, `WBP_` for widgets, `ABP_` for animations, `BPI_` for interfaces. |
| **Combat Tick** | Server loop running every 50ms that processes all active auto-attacks. |
| **connectedPlayers** | Server-side `Map<charId, PlayerData>` tracking all online players. |
| **Event Dispatcher** | UE5 delegate that Blueprints can bind to for event-driven updates. |
| **GameInstance** | UE5 object that persists across level loads. Used for auth state and character data. |
| **IMC** | Input Mapping Context — UE5 Enhanced Input configuration asset. |
| **Manager Pattern** | Singleton actor that tracks/spawns/destroys a collection of similar actors. |
| **Socket.io** | Real-time bidirectional event-based communication library (WebSocket upgrade). |
| **USTRUCT** | UE5 C++ struct with reflection support, usable in Blueprints. |

## ID Ranges

| Type | Range | Notes |
|------|-------|-------|
| User IDs | 1+ | PostgreSQL SERIAL |
| Character IDs | 1+ | PostgreSQL SERIAL |
| Enemy IDs | 2,000,001+ | Server-assigned, increments from `nextEnemyId` |
| Item IDs (consumable) | 1001–1005 | Seed data |
| Item IDs (etc/loot) | 2001–2008 | Seed data |
| Item IDs (weapon) | 3001–3006 | Seed data |
| Item IDs (armor) | 4001–4003 | Seed data |
| Inventory IDs | 1+ | PostgreSQL SERIAL |

## File Naming Conventions

| Prefix | Meaning | Example |
|--------|---------|---------|
| `BP_` | Blueprint actor | `BP_MMOCharacter` |
| `WBP_` | Widget Blueprint | `WBP_GameHUD` |
| `ABP_` | Animation Blueprint | `ABP_unarmed` |
| `AC_` | Actor Component | `AC_HUDManager` |
| `BPI_` | Blueprint Interface | `BPI_Damageable` |
| `A` prefix (C++) | Actor class | `ASabriMMOCharacter` |
| `U` prefix (C++) | UObject class | `UMMOGameInstance` |
| `F` prefix (C++) | Struct | `FCharacterData` |
| `I` prefix (C++) | Interface | `ICombatDamageable` |
| `E` prefix (C++) | Enum | `EEndPlayReason` |

## Socket.io Event Naming Convention

```
domain:action
```

| Domain | Events |
|--------|--------|
| `player:` | join, joined, position, moved, left, stats, request_stats, allocate_stat |
| `combat:` | attack, stop_attack, auto_attack_started, auto_attack_stopped, damage, health_update, out_of_range, target_lost, death, respawn, error |
| `enemy:` | spawn, move, death, health_update |
| `chat:` | message, receive |
| `inventory:` | load, data, use, used, equip, equipped, drop, dropped, error |
| `loot:` | drop |

## Stat Formulas (RO-Style)

| Derived Stat | Formula |
|-------------|---------|
| statusATK | `STR + floor(STR/10)² + floor(DEX/5) + floor(LUK/3)` |
| statusMATK | `INT + floor(INT/7)²` |
| Hit | `Level + DEX` |
| Flee | `Level + AGI` |
| softDEF | `floor(VIT×0.5 + VIT²/150)` |
| softMDEF | `floor(INT×0.5)` |
| Critical | `floor(LUK×0.3)` |
| ASPD | `min(190, floor(170 + AGI×0.4 + DEX×0.1))` |
| maxHP | `100 + VIT×8 + Level×10` |
| maxSP | `50 + INT×5 + Level×5` |
| Attack Interval (ms) | `(200 - ASPD) × 50` |
| Physical Damage | `max(1, floor((statusATK + weaponATK) × variance) - targetSoftDEF)` |
| Critical Damage | `damage × 1.4` |
| Damage Variance | `0.8 + random() × 0.2` |

---

**Last Updated**: 2026-02-17
