# Pet System — Server Documentation

**Implemented**: 2026-03-18
**Source**: `server/src/ro_pet_data.js` (pet database), `server/src/index.js` (handlers)

## Overview

RO Classic pre-renewal pet system. 48 tameable monsters with unique taming items, food, accessories, and stat bonuses. Pets have hunger/intimacy mechanics and can permanently run away if neglected.

## Data File: `ro_pet_data.js`

- **48 pets** defined in `PET_DB` (keyed by monster template ID)
- Constants: `PET_INTIMATE` (intimacy levels), `PET_HUNGER` (hunger thresholds)
- Helpers: `getPetData()`, `getPetByTamingItem()`, `getPetByEggItem()`, `getIntimacyLevel()`, `getHungerLevel()`, `calculateCaptureRate()`, `calculateFeedIntimacyChange()`

## Database: `character_pets`

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| id | SERIAL PK | — | Pet row ID |
| character_id | INTEGER | — | Owner character |
| mob_id | INTEGER | — | Monster template ID |
| egg_item_id | INTEGER | — | Egg item ID (9001-9056) |
| pet_name | VARCHAR(24) | '' | Custom name |
| intimacy | INTEGER | 250 | 0-1000 (Neutral start) |
| hunger | INTEGER | 100 | 0-100 |
| equip_item_id | INTEGER | 0 | Equipped accessory |
| is_hatched | BOOLEAN | FALSE | Out of egg |
| is_active | BOOLEAN | FALSE | Following player |

Auto-created at server startup.

## Socket Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `pet:tame` | C→S | `{ targetEnemyId, tamingItemId }` | Attempt to tame monster |
| `pet:tamed` | S→C | `{ success, mobId, petName, eggItemId }` | Taming succeeded |
| `pet:tame_failed` | S→C | `{ success: false, mobId, petName }` | Taming failed |
| `pet:incubate` | C→S | `{ petId }` | Hatch egg with Pet Incubator |
| `pet:hatched` | S→C | `{ petId, mobId, name, hunger, intimacy, intimacyLevel }` | Pet hatched |
| `pet:feed` | C→S | `{}` | Feed active pet |
| `pet:fed` | S→C | `{ hunger, intimacy, intimacyLevel, intimacyChange, emote }` | Pet fed |
| `pet:return_to_egg` | C→S | `{}` | Return pet to egg |
| `pet:returned` | S→C | `{ petId }` | Pet returned |
| `pet:rename` | C→S | `{ name }` | Rename pet |
| `pet:renamed` | S→C | `{ petId, name }` | Pet renamed |
| `pet:list` | C→S / S→C | `{ pets: [...] }` | List all owned pets |
| `pet:hunger_update` | S→C | `{ hunger, intimacy, hungerLevel, intimacyLevel }` | Periodic update |
| `pet:ran_away` | S→C | `{ petId, petName }` | Pet permanently lost |
| `pet:error` | S→C | `{ message }` | Error message |

## Capture Rate Formula (Pre-Renewal Legacy)

```
rate = (baseRate + (playerLevel - monsterLevel) * 30 + playerLUK * 20)
       * (200 - monsterHPPercent) / 100
```

- `baseRate` from `PET_DB[mobId].captureRate` (out of 10000)
- Lower monster HP → higher success (nearly 2x at 1% HP)
- Player level and LUK give significant bonuses
- Taming item consumed regardless of result
- Boss monsters cannot be tamed

## Hunger System (0-100)

| Status | Range | Feeding Effect |
|--------|-------|---------------|
| Stuffed | 91-100 | **Overfeed**: intimacy -100 |
| Satisfied | 76-90 | **Overfeed**: intimacy -100 |
| Neutral | 26-75 | Intimacy gain × 0.75 |
| Hungry | 11-25 | **Optimal**: full intimacy gain |
| Very Hungry | 0-10 | Starvation: -5 intimacy / 20 seconds |

- Hunger decays by `fullnessDecay` (1-7) every `hungryDelay` ms (default 60s)
- Feed adds +20 hunger (cap 100)
- Each pet has a specific food item (no universal food)

## Intimacy System (0-1000)

| Level | Range | Effects |
|-------|-------|---------|
| Awkward | 0-99 | May run away |
| Shy | 100-249 | No bonuses |
| Neutral | 250-749 | Starting default |
| Cordial | 750-909 | **Stat bonuses active** |
| Loyal | 910-1000 | **Full stat bonuses** |

### Intimacy Changes
- Feed at Hungry: +IntimacyFed (10-50, varies per pet)
- Feed at Neutral: +IntimacyFed × 0.75
- Feed at Very Hungry: +IntimacyFed × 0.5
- Overfeed (Satisfied/Stuffed): **-100**
- Owner death: **-20**
- Starvation: **-5 per 20 seconds**
- Reaches 0: **Pet runs away permanently** (DB row deleted)

## Stat Bonuses

Bonuses from `PET_DB[mobId].bonuses` applied when intimacy >= 750 (Cordial).

Integrated into `getEffectiveStats(player)` via `player.petBonuses`:
- Flat stats: str, agi, vit, int, dex, luk
- Derived: hit, flee, critical, perfectDodge, atk, def, mdef
- Percentage: maxHP, maxSP, maxHPPercent, maxSPPercent, atkPercent, matkPercent

Recalculated via `recalcPetBonuses()` when:
- Pet hatched or returned to egg
- Intimacy crosses 750 threshold
- `player:stats` emitted to client on change

## Pet Does NOT

- Take damage or have HP (cannot be attacked by monsters)
- Fight or deal damage
- Give EXP or drops when killed (cannot be killed)
- Evolve (pre-renewal — evolution added kRO Oct 2014)
- Auto-loot items (disabled by default in pre-renewal)

## Client Subsystem

`PetSubsystem` (Z=21) — pure C++ Slate:
- 8 event handlers registered via SocketEventRouter
- Inline `SPetWidget`: RO brown/gold theme, hunger/intimacy bars, feed/return buttons
- Auto-shows on `pet:hatched`, toggle via `ToggleWidget()`
- `FeedPet()`, `ReturnPetToEgg()`, `RenamePet()`, `RequestPetList()` emit to server
