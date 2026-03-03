# EXP & Leveling System — Ragnarok Online Classic

## Overview

Sabri_MMO uses a **dual progression system** identical to Ragnarok Online Pre-Renewal Classic:
- **Base Level** (1–99): Grants stat points on level up, affects HP/SP/Hit/Flee
- **Job Level** (1–10/50): Grants skill points on level up, tied to job class

## Architecture

### Server-Authoritative
All EXP calculations, level-up checks, and stat point grants happen **exclusively on the server**. The client only receives results via Socket.io events.

### Files
| File | Purpose |
|------|---------|
| `server/src/ro_exp_tables.js` | Complete EXP tables (base 1-99, novice/1st/2nd class job) |
| `server/src/index.js` | EXP processing, level-up logic, Socket.io events |
| `database/migrations/add_exp_leveling_system.sql` | DB migration for EXP columns |
| `client/SabriMMO/Source/SabriMMO/CharacterData.h` | C++ struct with EXP fields |

## Database Schema (characters table)

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| `level` | INTEGER | 1 | Base level (1–99) |
| `job_level` | INTEGER | 1 | Job level (1–10 novice, 1–50 others) |
| `base_exp` | BIGINT | 0 | Current base EXP toward next level |
| `job_exp` | BIGINT | 0 | Current job EXP toward next job level |
| `job_class` | VARCHAR(30) | 'novice' | Current job class name |
| `stat_points` | INTEGER | 48 | Unspent stat points |
| `skill_points` | INTEGER | 0 | Unspent skill points |

## EXP Tables

### Base EXP (Levels 1–99)
- Source: Verified rAthena Pre-Renewal Classic data
- Level 1→2: 9 EXP
- Level 98→99: 99,999,998 EXP
- Major spikes at levels 5, 29, 39, 49, 59, 70, 90
- Full table in `ro_exp_tables.js` → `BASE_EXP_TABLE`

### Novice Job EXP (Job Levels 1–10)
- Total to reach Job 10: 1,151 EXP
- Major spike at Job Level 5 (+127.5%)
- Full table in `ro_exp_tables.js` → `NOVICE_JOB_EXP_TABLE`

### First Class Job EXP (Job Levels 1–50)
- Applies to: Swordsman, Mage, Archer, Acolyte, Thief, Merchant
- Total to reach Job 50: 3,753,621 EXP
- Full table in `ro_exp_tables.js` → `FIRST_CLASS_JOB_EXP_TABLE`

### Second Class Job EXP (Job Levels 1–50)
- Applies to: Knight, Wizard, Hunter, Priest, Assassin, Blacksmith, etc.
- Total to reach Job 50: 16,488,271 EXP
- Full table in `ro_exp_tables.js` → `SECOND_CLASS_JOB_EXP_TABLE`

## Level-Up Mechanics

### Base Level Up
- **Stat Points**: `floor(newLevel / 5) + 3` per level (RO classic formula)
  - Level 2: +3 pts, Level 10: +5 pts, Level 50: +13 pts, Level 99: +22 pts
- **Full Heal**: HP and SP restored to max on level up
- **Derived Stats Recalculated**: maxHP, maxSP, Hit, Flee all update with level

### Job Level Up
- **Skill Points**: +1 per job level up (always)
- No stat changes on job level up

## Class Progression

### Tier 0: Novice
- Starting class for all characters
- Max Job Level: 10
- Job change available at Job Level 10

### Tier 1: First Classes
- Swordsman, Mage, Archer, Acolyte, Thief, Merchant
- Max Job Level: 50
- Job change to 2nd class available at Job Level 40+
- Job level and job EXP reset to 1/0 on class change

### Tier 2: Second Classes
| First Class | Second Class Options |
|-------------|---------------------|
| Swordsman | Knight, Crusader |
| Mage | Wizard, Sage |
| Archer | Hunter, Bard, Dancer |
| Acolyte | Priest, Monk |
| Thief | Assassin, Rogue |
| Merchant | Blacksmith, Alchemist |

## Socket.io Events

### `exp:gain` (Server → Client)
Sent to killer after monster death.
```json
{
    "characterId": 123,
    "baseExpGained": 26,
    "jobExpGained": 20,
    "enemyName": "Poring",
    "enemyLevel": 1,
    "exp": { /* buildExpPayload */ },
    "baseLevelUps": [],
    "jobLevelUps": []
}
```

### `exp:level_up` (Server → All)
Sent when any level up occurs.
```json
{
    "characterId": 123,
    "characterName": "PlayerName",
    "baseLevelUps": [{ "newLevel": 5, "statPointsGained": 4 }],
    "jobLevelUps": [{ "newJobLevel": 3, "skillPointsGained": 1 }],
    "totalStatPoints": 4,
    "totalSkillPoints": 1,
    "exp": { /* buildExpPayload */ }
}
```

### `player:stats` (Server → Client)
Now includes `exp` field:
```json
{
    "characterId": 123,
    "stats": { "str": 5, "agi": 3, ... },
    "derived": { "statusATK": 10, ... },
    "exp": {
        "baseLevel": 5,
        "jobLevel": 3,
        "baseExp": 50,
        "jobExp": 15,
        "baseExpNext": 77,
        "jobExpNext": 28,
        "jobClass": "novice",
        "jobClassDisplayName": "Novice",
        "maxBaseLevel": 99,
        "maxJobLevel": 10,
        "statPoints": 60,
        "skillPoints": 2
    }
}
```

### `job:change` (Client → Server)
Request a job change.
```json
{ "targetClass": "swordsman" }
```

### `job:changed` (Server → All)
Confirmation of job change.
```json
{
    "characterId": 123,
    "oldClass": "novice",
    "newClass": "swordsman",
    "newClassDisplayName": "Swordsman",
    "jobLevel": 1,
    "jobExp": 0,
    "jobExpNext": 30,
    "maxJobLevel": 50,
    "skillPoints": 9,
    "exp": { /* buildExpPayload */ }
}
```

### `job:error` (Server → Client)
```json
{ "message": "Requires Novice Job Level 10 (current: 5)" }
```

## Data Persistence

- **On Monster Kill**: EXP saved to DB immediately after processing
- **On Disconnect**: Full stats + EXP saved
- **Periodic Save**: Every 60 seconds for all connected players
- **On Level Up**: Immediate save with updated stats

## Blueprint Integration (Client)

### C++ Struct: `FCharacterData`
New fields added:
- `int32 JobLevel` — Current job level
- `FString JobClass` — Current job class name
- `int64 BaseExp` — Current base EXP
- `int64 JobExp` — Current job EXP
- `int32 SkillPoints` — Unspent skill points

### Socket Events to Bind in Blueprint
1. **`exp:gain`** — Update EXP bars, show floating EXP text
2. **`exp:level_up`** — Play level-up VFX/SFX, show notification
3. **`job:changed`** — Update class display, refresh UI
4. **`player:stats`** → `exp` field — Initialize/refresh EXP display
