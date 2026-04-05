# Monster Skill System

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Enemy_System](Enemy_System.md) | [Skill_System](Skill_System.md) | [Combat_System](Combat_System.md)

**Implemented:** 2026-03-17
**Status:** ACTIVE — 23 monster templates with skill data, 7 execution functions, Plagiarism integration

## Overview

The Monster Skill System enables monsters to cast skills during combat, matching Ragnarok Online Classic pre-renewal behavior. Monsters can use both monster-only NPC_ skills (elemental attacks, status infliction, AoE) and player-class skills (Fire Bolt, Heal, Water Ball) that can be copied by Rogues via Plagiarism.

## Architecture

| File | Role |
|------|------|
| `server/src/ro_monster_skills.js` | Data: `NPC_SKILLS` (40+ monster-only skill defs) + `MONSTER_SKILL_DB` (per-monster skill entries) |
| `server/src/index.js` | Logic: 7 functions for skill selection/execution + AI state hooks |

## How It Works

### AI Tick Flow (200ms cycle)

```
For each alive enemy:
  1. CC/movement checks (stun, freeze, close confine)
  2. IF in ATTACK state:
     a. Check active casting → processMonsterCasting()
     b. Look up MONSTER_SKILL_DB[templateId]
     c. For each skill rule (top to bottom):
        - Match state? Skip if not
        - Cooldown expired? Skip if not
        - Condition met? Skip if not
        - Roll rate (0-10000)? Skip if fails
        - SELECTED → execute skill, skip auto-attack
     d. No skill selected → normal auto-attack
  3. IF in IDLE state:
     a. Check idle skills (e.g., Teleport on rudeattacked)
```

### Skill Execution Types

| isPlayerSkill | Route | Plagiarism Copyable |
|---------------|-------|---------------------|
| `true` | `executeMonsterPlayerSkill()` → uses SKILL_MAP + damage formulas | YES — calls `checkPlagiarismCopy()` even on miss (skill does not need to connect) |
| `false` | `executeNPCSkill()` → uses NPC_SKILLS type handlers | NO |

### Casting System

Skills with `castTime > 0` enter a casting state:
- `enemy._casting` holds cast data (skill, target, startTime, castTime, cancelable)
- `processMonsterCasting()` checks each tick
- If `cancelable: true` and enemy takes damage → cast interrupted
- If `cancelable: false` → cast always completes (boss abilities)
- Emits `enemy:casting` and `enemy:cast_interrupted` events

## Data Format

### MONSTER_SKILL_DB Entry
```javascript
{
    skillId: 184,              // Skill ID (NPC_* or player skill ID)
    skillName: 'NPC_WATERATTACK',
    level: 1,                  // Can exceed player max
    state: 'attack',           // AI state trigger
    rate: 2000,                // 20% chance per tick (out of 10000)
    castTime: 0,               // 0 = instant, >0 = casting delay (ms)
    delay: 5000,               // Reuse cooldown (ms)
    cancelable: true,          // Can be interrupted
    target: 'target',          // target, self, friend, randomtarget
    condition: 'always',       // Trigger condition
    conditionValue: 0,         // Condition parameter
    isPlayerSkill: false,      // true = copyable by Plagiarism
    damageType: 'physical',    // physical, magical, heal, status, utility
    emotion: -1,               // Emote to display (-1 = none)
}
```

### Currently Active Monsters with Skills

| Monster | ID | Zone | Skills |
|---------|-----|------|--------|
| Hornet | 1004 | prontera_south | NPC_POISON |
| Familiar | 1005 | prontera_south | NPC_BLINDATTACK |
| Spore | 1014 | prontera_north | NPC_WATERATTACK |
| Zombie | 1015 | prontera_north | NPC_DARKNESSATTACK, NPC_STUNATTACK |
| Skeleton | 1076 | prontera_north | NPC_STUNATTACK |
| Creamy | 1018 | prontera_north | AL_TELEPORT (copyable) |
| Poporing | 1031 | prontera_north | NPC_POISONATTACK |
| Poison Spore | 1077 | prontera_north | NPC_POISON, NPC_POISONATTACK |
| Smokie | 1056 | prontera_north | NPC_COMBOATTACK |
| Yoyo | 1057 | prontera_north | NPC_COMBOATTACK |

### Boss/MVP Skills (future zones)

| Boss | Skills | Notes |
|------|--------|-------|
| Osiris (1038) | AL_TELEPORT, AL_HEAL Lv11, NPC_CURSEATTACK, NPC_DARKNESSATTACK | Heals at <50% HP |
| Baphomet (1039) | NPC_EARTHQUAKE, AL_HEAL Lv11, NPC_DARKNESSATTACK, NPC_SUMMONSLAVE | Quake at <80% HP, summons Bapho Jr |
| Drake (1112) | MG_WATERBALL Lv9, NPC_CURSEATTACK, NPC_STUNATTACK | Water Ball copyable |
| Angeling (1096) | AL_HEAL Lv9, AL_HOLYLIGHT Lv5 | Both copyable |

## Socket Events

| Event | Direction | When |
|-------|-----------|------|
| `enemy:skill_used` | Server → Zone | Monster uses any skill |
| `enemy:casting` | Server → Zone | Monster starts casting (castTime > 0) |
| `enemy:cast_interrupted` | Server → Zone | Monster cast interrupted by damage |
| `enemy:emotion` | Server → Zone | Monster displays emote |

## Condition Types Reference

| Condition | Parameters | Description |
|-----------|------------|-------------|
| `always` | — | No condition, pure rate check |
| `myhpltmaxrate` | conditionValue=% | HP below threshold |
| `myhpinrate` | conditionValue=min%, val1=max% | HP in range |
| `rudeattacked` | — | Can't reach target |
| `onspawn` | — | Just spawned |
| `slavelt` | conditionValue=count | Fewer slaves than threshold |
| `attackpcgt` | conditionValue=count | Many players attacking |
| `closedattacked` | — | Hit by melee |
| `longrangeattacked` | — | Hit by ranged |
| `afterskill` | conditionValue=skillId | Just cast specific skill |
| `friendhpltmaxrate` | conditionValue=% | Ally HP below threshold |

## Adding Skills to a New Monster

1. Find the monster's template ID (from `ro_monster_templates.js`)
2. Look up the monster's skills in rAthena's `pre-re/mob_skill_db.txt`
3. Add entry to `MONSTER_SKILL_DB` in `ro_monster_skills.js`:
   - Set `isPlayerSkill: true` for player skills, `false` for NPC_ skills
   - Order entries by priority (conditional/HP skills first)
   - Use rAthena's rate values (out of 10000)
   - Set appropriate delay (cooldown) per rAthena data

## Limitations

- **Slave summoning** — Stub only; needs master-slave tracking
- **Metamorphosis** — Stub only; needs template swap mechanism
- **Ground-target monster skills** — Monsters can't place ground effects yet
- **Client cast bar** — `enemy:casting` event emitted but no client UI handler
- **Monster skill VFX** — `enemy:skill_used` emitted but no client VFX mapping
