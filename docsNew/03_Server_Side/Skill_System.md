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
| **Novice** | 3 skills (IDs 1-3) |
| **Swordsman** | 10 skills (IDs 100-109) |
| **Mage** | 14 skills (IDs 200-213) |
| **Archer** | 7 skills (IDs 300-306) |
| **Acolyte** | 14 skills (IDs 400-413) |
| **Thief** | 10 skills (IDs 500-509) |
| **Merchant** | 10 skills (IDs 600-609) |
| **First Class Total** | **68 skills** |
| **Second Classes** (13) | 83 skills |
| **Grand Total** | **151 skills** |

## Implemented Skill Handlers (Phase 5)

All 6 first classes are fully playable with working handlers:

### Novice
| ID | Name | Type | Handler |
|----|------|------|---------|
| 2 | First Aid | active (self heal) | Heals effectValue HP, green heal numbers |

### Swordsman (10 skills: 7 active + 3 passive)
| ID | Name | Type | Handler |
|----|------|------|---------|
| 100 | Sword Mastery | passive | +4 ATK/lv with dagger/1H sword |
| 101 | 2H Sword Mastery | passive | +4 ATK/lv with 2H sword |
| 102 | Increase HP Recovery | passive | +5 HP/regen tick/lv |
| 103 | Bash | active (single) | Physical damage + Fatal Blow stun chance |
| 104 | Provoke | active (single) | Debuff: -DEF%, +ATK% on target |
| 105 | Magnum Break | active (ground AoE) | Fire AoE 300 radius centered on ground |
| 106 | Endure | active (self buff) | +MDEF, walk through attacks |
| 107 | Moving HP Recovery | passive | Allow HP regen while moving |
| 108 | Auto Berserk | active (toggle) | +32% ATK when HP < 25%, dynamic toggle |
| 109 | Fatal Blow | passive | +5% stun chance/lv on Bash hits |

### Mage (14 skills: 12 active + 2 passive)
| ID | Name | Type | Handler |
|----|------|------|---------|
| 200 | Cold Bolt | active (single, cast) | Water bolt ×N hits |
| 201 | Fire Bolt | active (single, cast) | Fire bolt ×N hits |
| 202 | Lightning Bolt | active (single, cast) | Wind bolt ×N hits |
| 203 | Napalm Beat | active (single, cast) | Ghost AoE damage |
| 204 | Increase SP Recovery | passive | +3 SP/regen tick/lv |
| 205 | Sight | active (self buff) | Reveal hidden, 10s duration |
| 206 | Stone Curse | active (single, cast) | Petrify target |
| 207 | Fire Ball | active (single, cast) | Fire projectile + explosion |
| 208 | Frost Diver | active (single, cast) | Freeze target |
| 209 | Fire Wall | active (ground, cast) | Persistent fire zone, knockback |
| 210 | Soul Strike | active (single, cast) | Ghost multi-hit projectile |
| 211 | Safety Wall | active (ground, cast) | Block melee attacks, ground effect |
| 212 | Thunderstorm | active (ground, cast) | Wind AoE rain ×N strikes |
| 213 | Energy Coat | active (self buff) | Dynamic phys dmg reduction by SP% tier (-6% to -30%), SP drain per hit (1-3%), 5min |

### Archer (7 skills: 4 active + 2 passive + 1 deferred)
| ID | Name | Type | Handler |
|----|------|------|---------|
| 300 | Owl's Eye | passive | +1 DEX/lv |
| 301 | Vulture's Eye | passive | +1 HIT/lv, +10 range/lv (bow) |
| 302 | Improve Concentration | active (self buff) | +AGI%/+DEX% of base stats, reveal hidden |
| 303 | Double Strafe | active (single) | 2 hits with 200ms stagger, requires bow |
| 304 | Arrow Shower | active (ground AoE) | Ranged AoE 400 radius, requires bow |
| 305 | Arrow Crafting | deferred | "Not yet implemented" |
| 306 | Arrow Repel | active (single) | Ranged attack + 250 unit knockback |

### Acolyte (14 skills: 12 active + 2 passive)
| ID | Name | Type | Handler |
|----|------|------|---------|
| 400 | Heal | active (single/self) | RO formula: floor((baseLv+INT)/8)×(4+Lv×8). Damages undead (holy) |
| 401 | Divine Protection | passive | +3 raceDEF/lv vs undead/demon |
| 402 | Blessing | active (single/self) | Buff +STR/DEX/INT, sends updated stats |
| 403 | Increase AGI | active (single/self) | Buff +AGI +25% movespeed, removes Decrease AGI |
| 404 | Decrease AGI | active (single) | Debuff -AGI -25% movespeed, removes Increase AGI |
| 405 | Cure | active (single/self) | Cleanses silence/blind/confusion |
| 406 | Angelus | active (self buff) | +DEF% |
| 407 | Signum Crucis | active (AoE) | -DEF% on undead/demon in 500 radius |
| 408 | Ruwach | active (self buff) | Reveal hidden, 10s duration |
| 409 | Teleport | active (self) | Lv1: random in zone, Lv2: save point (zone change) |
| 410 | Warp Portal | active (ground) | Portal to save point (ground effect) |
| 411 | Pneuma | active (ground) | Block ranged attacks (ground effect, checked in auto-attack tick) |
| 412 | Aqua Benedicta | active (self) | Simplified: creates Holy Water (chat msg) |
| 413 | Demon Bane | passive | +3 raceATK/lv vs undead/demon |

### Thief (10 skills: 8 active + 2 passive)
| ID | Name | Type | Handler |
|----|------|------|---------|
| 500 | Double Attack | passive | +5% double-hit chance/lv (dagger only) |
| 501 | Improve Dodge | passive | +3 FLEE/lv |
| 502 | Steal | active (single) | Chance = effectVal + DEX/2 - enemyLv/3 |
| 503 | Hiding | active (toggle) | Invisible, broken by damage/skill use/Sight/Ruwach |
| 504 | Envenom | active (single) | Physical + poison element + apply poison status |
| 505 | Detoxify | active (single/self) | Cleanses poison |
| 506 | Sand Attack | active (single) | Physical + earth element + apply blind status |
| 507 | Back Slide | active (self) | Teleport 250 units backward |
| 508 | Throw Stone | active (single) | Fixed 50+STR damage, range 700 |
| 509 | Pick Stone | active (self) | Simplified: picks up stone (chat msg) |

### Merchant (10 skills: 5 active + 3 passive + 3 deferred)
| ID | Name | Type | Handler |
|----|------|------|---------|
| 600 | Enlarge Weight Limit | passive | +200 weight/lv |
| 601 | Discount | passive | Reduce NPC buy prices |
| 602 | Overcharge | passive | Increase NPC sell prices |
| 603 | Mammonite | active (single) | Physical + costs Lv×100 zeny, DB update |
| 604 | Pushcart | passive | Cart storage |
| 605 | Vending | deferred | "Not yet implemented" |
| 606 | Item Appraisal | deferred | "Not yet implemented" |
| 607 | Change Cart | deferred | "Not yet implemented" |
| 608 | Cart Revolution | active (ground AoE) | AoE 300 radius + knockback |
| 609 | Loud Exclamation | active (self buff) | +4 STR, 5min duration |

## Passive Skill Engine

`getPassiveSkillBonuses(player)` in `index.js` returns a bonuses object based on learned passive skills:

| Passive | Effect | Weapon Gate |
|---------|--------|-------------|
| Sword Mastery (100) | +4 ATK/lv | dagger, one_hand_sword |
| 2H Sword Mastery (101) | +4 ATK/lv | two_hand_sword |
| HP Recovery (102) | +5 HP/regen tick/lv | — |
| Moving HP Recovery (107) | Allow HP regen while moving | — |
| Fatal Blow (109) | +5% stun chance/lv on Bash | — |
| SP Recovery (204) | +3 SP/regen tick/lv | — |
| Owl's Eye (300) | +1 DEX/lv | — |
| Vulture's Eye (301) | +1 HIT/lv, +10 range/lv | bow only (range) |
| Divine Protection (401) | +3 raceDEF/lv vs undead/demon | — |
| Demon Bane (413) | +3 raceATK/lv vs undead/demon | — |
| Double Attack (500) | +5% double-hit/lv | dagger only |
| Improve Dodge (501) | +3 FLEE/lv | — |

`getEffectiveStats(player)` merges equipment bonuses + passive bonuses + buff modifiers into a single stats object used by all combat/stat calculations.

### Race ATK/DEF in Damage Formula

`ro_damage_formulas.js` has two new steps:
- **Step 6b** (after card modifiers): `attacker.passiveRaceATK[targetRace]` adds flat ATK bonus
- **After soft DEF**: `target.passiveRaceDEF[attacker.race]` subtracts flat DEF bonus

### Double Attack in Auto-Attack Tick

After normal auto-attack damage on enemy, checks `doubleAttackChance` from passive. If triggered, calculates second hit with 200ms delay, broadcasts `hitType: 'doubleAttack'`.

### HP Regen Movement Blocking (RO Classic)

`player.lastMoveTime` is set on position updates when actual movement > 5 UE units is detected. HP natural regen tick (6s) skips players who moved within last 4 seconds UNLESS they have `movingHPRecovery` passive (Swordsman skill 107).

### Hidden Player AI Integration

- `findAggroTarget()`: skips hidden players unless enemy has `detector` mode flag
- CHASE state: drops target and returns to IDLE if target becomes hidden
- ATTACK state: drops target and returns to IDLE if target becomes hidden
- Hiding breaks on: taking damage, using any offensive skill (except Hiding toggle itself)

### Auto Berserk Dynamic Toggle

`checkAutoBerserk(player, characterId, zone)` is called whenever player HP changes:
- HP drops below 25% → activate ATK bonus (32%)
- HP rises above 25% → deactivate ATK bonus
- Called in: enemy damage handler, heal skill, HP natural regen, skill-based regen

### executePhysicalSkillOnEnemy Helper

Shared helper for single-target physical skills (Envenom, Sand Attack, Mammonite). Handles: enemy lookup, range check, SP deduct, damage calc, aggro, damage-break statuses, broadcast, death check. Returns `{ result, enemy, attackerPos, targetPos, zone }` for callers to add unique post-damage behavior.

## Future Expansion

- **Quest skills**: Skills learned through quests instead of skill points
- **Skill point cost scaling**: Some RO skills cost more than 1 point at higher levels
- **Second class skill handlers**: Knight, Wizard, Hunter, Priest, Assassin, Blacksmith, etc.
- **Ground effect interactions**: Players entering Warp Portal, Pneuma blocking ranged skills
- **Energy Coat SP drain**: ~~Drain 3% SP per hit absorbed~~ IMPLEMENTED — `applyEnergyCoat()` in index.js, dynamic SP% tiers

---

**Last Updated**: 2026-03-10 (Phase 5: Passive Skills & First Class Completion — all 6 first classes playable)
**Previous**: 2026-02-25
