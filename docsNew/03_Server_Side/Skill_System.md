# Skill System — Ragnarok Online Classic

## Overview

Server-authoritative skill system implementing RO pre-renewal class skills with full prerequisite chains, SP costs per level, and skill point allocation. Skills are defined in JavaScript data files and synced to PostgreSQL on startup.

## Architecture

```
Client (UE5 Slate)                    Server (Node.js)
─────────────────                    ────────────────
SSkillTreeWidget ◄─── data ───── skill:data event
  │                                    │
  │ click "Learn"                      │ canLearnSkill() validation
  │                                    │   - class progression check
  ▼                                    │   - prerequisite check
SkillTreeSubsystem ──► emit ──► skill:learn event
  │                                    │
  │ ◄── skill:learned ────────────────┘
  │ ◄── skill:data (refresh) ─────────┘
```

## Database Tables

### `skills` (static definitions, synced from JS on startup)
| Column | Type | Description |
|--------|------|-------------|
| skill_id | INTEGER PK | Unique skill ID (range-based per class) |
| internal_name | VARCHAR(100) | Snake_case internal name |
| display_name | VARCHAR(100) | Human-readable name |
| class_id | VARCHAR(30) | Class this skill belongs to |
| max_level | INTEGER | Maximum learnable level |
| skill_type | VARCHAR(20) | active, passive, toggle |
| target_type | VARCHAR(20) | none, self, single, ground, aoe |
| element | VARCHAR(20) | neutral, fire, water, wind, earth, holy, dark, ghost, undead, poison |
| skill_range | INTEGER | Range in game units |
| description | TEXT | Skill description |
| icon | VARCHAR(100) | Icon identifier |
| tree_row / tree_col | INTEGER | Position in skill tree UI |

### `skill_prerequisites`
| Column | Type | Description |
|--------|------|-------------|
| skill_id | INTEGER FK | The skill that has the prerequisite |
| required_skill_id | INTEGER FK | The prerequisite skill |
| required_level | INTEGER | Required level of prerequisite |

### `skill_levels` (per-level data)
| Column | Type | Description |
|--------|------|-------------|
| skill_id | INTEGER FK | Skill reference |
| level | INTEGER | Skill level (1 to max_level) |
| sp_cost | INTEGER | SP cost at this level |
| cast_time_ms | INTEGER | Cast time in milliseconds |
| cooldown_ms | INTEGER | Cooldown in milliseconds |
| effect_value | INTEGER | Generic effect value (damage %, heal amount, etc.) |
| duration_ms | INTEGER | Buff/debuff duration |

### `character_skills`
| Column | Type | Description |
|--------|------|-------------|
| character_id | INTEGER FK | Character reference |
| skill_id | INTEGER FK | Learned skill |
| level | INTEGER | Current learned level |

## Skill ID Ranges

| Range | Class |
|-------|-------|
| 1-99 | Novice |
| 100-199 | Swordsman |
| 200-299 | Mage |
| 300-399 | Archer |
| 400-499 | Acolyte |
| 500-599 | Thief |
| 600-699 | Merchant |
| 700-799 | Knight |
| 800-899 | Wizard |
| 900-999 | Hunter |
| 1000-1099 | Priest |
| 1100-1199 | Assassin |
| 1200-1299 | Blacksmith |
| 1300-1399 | Crusader |
| 1400-1499 | Sage |
| 1500-1519 | Bard |
| 1520-1539 | Dancer |
| 1600-1699 | Monk |
| 1700-1799 | Rogue |
| 1800-1899 | Alchemist |

## Class Progression Chains

Second-class characters have access to skills from all previous classes:

| Class | Available Skill Trees |
|-------|----------------------|
| Knight | Novice → Swordsman → Knight |
| Crusader | Novice → Swordsman → Crusader |
| Wizard | Novice → Mage → Wizard |
| Sage | Novice → Mage → Sage |
| Hunter | Novice → Archer → Hunter |
| Bard | Novice → Archer → Bard |
| Dancer | Novice → Archer → Dancer |
| Priest | Novice → Acolyte → Priest |
| Monk | Novice → Acolyte → Monk |
| Assassin | Novice → Thief → Assassin |
| Rogue | Novice → Thief → Rogue |
| Blacksmith | Novice → Merchant → Blacksmith |
| Alchemist | Novice → Merchant → Alchemist |

Cross-class prerequisites are supported (e.g., Knight's Two-Hand Quicken requires Swordsman's Two-Handed Sword Mastery Lv1).

## Socket.io Events

### `skill:data` (Client → Server → Client)
**Request**: Client emits `skill:data` with `{}`
**Response**: Server sends back:
```json
{
  "characterId": 1,
  "jobClass": "swordsman",
  "skillPoints": 5,
  "skillTree": {
    "novice": [{ "skillId": 1, "displayName": "Basic Skill", "currentLevel": 9, "maxLevel": 9, "canLearn": false, ... }],
    "swordsman": [{ "skillId": 103, "displayName": "Bash", "currentLevel": 3, "maxLevel": 10, "canLearn": true, ... }]
  },
  "learnedSkills": { "1": 9, "103": 3 }
}
```

### `skill:learn` (Client → Server)
**Payload**: `{ "skillId": 103 }`
**Validation**: Class access, prerequisites met, skill points > 0, not at max level
**Response**: `skill:learned` on success, `skill:error` on failure

### `skill:learned` (Server → Client)
```json
{ "skillId": 103, "skillName": "Bash", "newLevel": 4, "maxLevel": 10, "skillPoints": 4 }
```

### `skill:reset` (Client → Server)
Refunds all invested skill points, deletes all character_skills rows.
**Response**: `skill:reset_complete` with `{ "skillPoints": 15, "refundedPoints": 10 }`

### `skill:use` (Client → Server)
**Payload**: `{ "skillId": 2 }`
**Validation**: Skill exists, player has learned it, sufficient SP
**Effect**: Deducts SP, applies skill effect (e.g., First Aid heals 5 HP), emits `skill:used` + `combat:health_update`
**Skill Effects Implemented**:
| Skill | Effect |
|-------|--------|
| First Aid (id: 2) | Heals `effectValue` HP (5 at Lv1) |

### `skill:used` (Server → Client)
```json
{
  "skillId": 2, "skillName": "First Aid", "level": 1,
  "spCost": 3, "remainingMana": 97, "maxMana": 100
}
```
Followed by `combat:health_update` with updated HP/SP values.

### `skill:error` (Server → Client)
```json
{ "message": "Requires Bash level 5 (current: 0)" }
```
**Messages**: "Invalid skill ID", "Unknown skill", "Skill not learned", "Not enough SP (need X, have Y)"

## Hotbar Skill Assignment

### Quick-Assign from Skill Tree (C++ Slate)
The `SSkillTreeWidget` displays 9 numbered buttons [1]-[9] under each learned active/toggle skill. Clicking a button calls `USkillTreeSubsystem::AssignSkillToHotbar(skillId, displayName, slotIndex)` which emits `hotbar:save_skill`.

**Slot Indexing**: Buttons are **1-indexed** (1-9), matching the BP hotbar `WBP_HotbarSlot` indices.

### `hotbar:save_skill` (Client → Server)
```json
{ "slotIndex": 1, "skillId": 2, "skillName": "First Aid" }
```
**Validation**: Slot 1-9, skill learned by character
**DB**: UPSERTs `character_hotbar` with `slot_type='skill'`, clears `inventory_id`/`item_id`
**Response**: `hotbar:alldata` with full hotbar state

### `hotbar:alldata` (Server → Client)
Replaces the old `hotbar:data` event name to avoid C++ SocketIO event binding conflicts.
```json
{
  "slots": [
    { "slot_index": 1, "slot_type": "item", "inventory_id": 5, "item_id": 1001, "item_name": "Crimson Vial", "quantity": 3, "skill_id": 0, "skill_name": "" },
    { "slot_index": 3, "slot_type": "skill", "inventory_id": null, "item_id": null, "item_name": "", "quantity": 0, "skill_id": 2, "skill_name": "First Aid" }
  ]
}
```
**Sent**: After `inventory:load`, 0.6s after `player:join`, on `hotbar:request`, after `hotbar:save_skill`
**BP Handler**: `BP_SocketManager.OnHotbarAllData` → `AC_HUDManager.PopulateHotbarFromServer`

## Server Files

| File | Purpose |
|------|---------|
| `server/src/ro_skill_data.js` | Novice + 6 First Class skill definitions, lookup maps, validation functions |
| `server/src/ro_skill_data_2nd.js` | 12 Second Class skill definitions (Knight through Alchemist) |
| `server/src/index.js` | Socket events (skill:data, skill:learn, skill:reset), DB migration, skill data sync |
| `database/migrations/add_class_skill_system.sql` | Standalone migration SQL |

## Client Files (UE5 C++ Slate)

| File | Purpose |
|------|---------|
| `UI/SkillTreeSubsystem.h/.cpp` | UWorldSubsystem — Socket.io event binding, data parsing, emit actions |
| `UI/SSkillTreeWidget.h/.cpp` | SCompoundWidget — RO Classic themed, draggable, class tabs, skill grid, learn buttons, hotbar quick-assign row |
| `UI/SkillDragDropOperation.h` | UDragDropOperation subclass for dragging skills to UMG hotbar |

## Skill Count Summary

| Category | Count |
|----------|-------|
| **Novice** | 3 skills |
| **First Classes** (6) | 43 skills |
| **Second Classes** (13) | 93 skills |
| **Total** | **139 skills** |

## Future Expansion

- **More skill effects**: Damage skills (Bash), buffs (Increase AGI, Blessing), AoE damage, status effects
- **Cast time system**: Cast bar UI + interruptible casting
- **Cooldown tracking**: Per-skill cooldown timers
- **Skill icons in hotbar**: DataTable lookup (`DT_SkillIcons`) by skill ID for hotbar display
- **Drag-drop from Slate to UMG hotbar**: `USkillDragDropOperation` created but UMG OnDrop needs Blueprint implementation
- **Visual effects**: Particle/Niagara effects per skill
- **Quest skills**: Skills learned through quests instead of skill points
- **Skill point cost scaling**: Some RO skills cost more than 1 point at higher levels

---

**Last Updated**: 2026-02-25 (Added skill:use effect application, hotbar:alldata event, quick-assign slot fix)  
**Previous**: 2026-02-23
