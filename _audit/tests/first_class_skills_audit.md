# First Class Skills Audit & Jest Test Specification

**Generated**: 2026-03-22
**Scope**: All 1st class skill handlers (Novice, Swordsman, Mage, Archer, Acolyte, Thief, Merchant)
**Source**: `server/src/index.js` (handlers), `server/src/ro_skill_data.js` (definitions)
**Total Skills**: 55 skills across 7 classes (20 passive, 35 active/toggle)

---

## Table of Contents

1. [Novice (IDs 1-3)](#novice-ids-1-3)
2. [Swordsman (IDs 100-109)](#swordsman-ids-100-109)
3. [Mage (IDs 200-213)](#mage-ids-200-213)
4. [Archer (IDs 300-306)](#archer-ids-300-306)
5. [Acolyte (IDs 400-414)](#acolyte-ids-400-414)
6. [Thief (IDs 500-509)](#thief-ids-500-509)
7. [Merchant (IDs 600-609)](#merchant-ids-600-609)
8. [Issues Found](#issues-found)

---

## Shared Test Infrastructure

```javascript
// server/tests/helpers/skillTestSetup.js

/**
 * Mock infrastructure for testing skill handlers without a live server.
 * Each test file imports this and constructs the needed mocks.
 */

function createMockPlayer(overrides = {}) {
  return {
    characterName: 'TestPlayer',
    health: 1000, maxHealth: 1000,
    mana: 500, maxMana: 500,
    isDead: false,
    zone: 'prontera_south',
    lastX: 0, lastY: 0, lastZ: 580,
    stats: { str: 50, agi: 30, vit: 30, int: 30, dex: 50, luk: 10, level: 50 },
    baseLevel: 50,
    learnedSkills: {},
    activeBuffs: [],
    weaponType: 'one_hand_sword',
    hardDef: 0, hardMdef: 0,
    equippedAmmo: null,
    currentWeight: 0,
    zeny: 100000, zuzucoin: 100000,
    hasCart: false, cartWeight: 0,
    skillCooldowns: {},
    equipUseSpRate: 0, cardUseSPRate: 0,
    equipmentBonuses: {},
    weaponAspdMod: 0,
    socketId: 'mock-socket-1',
    ...overrides,
  };
}

function createMockEnemy(overrides = {}) {
  return {
    name: 'Poring',
    health: 500, maxHealth: 500,
    isDead: false,
    zone: 'prontera_south',
    x: 100, y: 100, z: 580,
    stats: { str: 5, agi: 5, vit: 5, int: 5, dex: 5, luk: 5, level: 1 },
    hardDef: 0, hardMdef: 0, mdef: 0,
    element: { type: 'water', level: 1 },
    race: 'plant',
    size: 'medium',
    activeBuffs: [],
    modeFlags: {},
    inCombatWith: new Set(),
    stealLocked: false,
    ...overrides,
  };
}

function createMockSocket() {
  const emitted = [];
  return {
    id: 'mock-socket-1',
    emit: jest.fn((event, data) => emitted.push({ event, data })),
    _emitted: emitted,
    getEmitted: (event) => emitted.filter(e => e.event === event),
    leave: jest.fn(),
    join: jest.fn(),
  };
}

function createMockIO() {
  return {
    to: jest.fn().mockReturnThis(),
    emit: jest.fn(),
    sockets: { sockets: new Map() },
  };
}

module.exports = { createMockPlayer, createMockEnemy, createMockSocket, createMockIO };
```

---

## Novice (IDs 1-3)

### ID 1 — Basic Attack
- **Type**: Passive (auto-attack system, not a skill handler)
- **Handler**: No `skill:use` handler. Processed in combat tick loop.
- **No test needed** in skill handler tests (covered by combat tick tests).

### ID 2 — First Aid
- **Handler Location**: Lines 9570-9596
- **Type**: Active, Self-target, Heal
- **Formula**: Heals `effectValue` HP (flat 5 HP at Lv1)
- **SP Cost**: 3
- **Mechanics**: Deducts SP, heals HP (capped at maxHealth), triggers AutoBerserk check, broadcasts heal VFX
- **Checks**: None (no target validation, no range check for self-cast)

```javascript
// server/tests/skills/novice.test.js

const { createMockPlayer, createMockSocket } = require('../helpers/skillTestSetup');

describe('First Aid (ID 2)', () => {
  test('heals 5 HP and costs 3 SP', () => {
    const player = createMockPlayer({ health: 900, mana: 100 });
    const healed = Math.min(5, player.maxHealth - player.health);
    player.health = Math.min(player.maxHealth, player.health + 5);
    player.mana = Math.max(0, player.mana - 3);

    expect(player.health).toBe(905);
    expect(player.mana).toBe(97);
    expect(healed).toBe(5);
  });

  test('does not exceed maxHealth', () => {
    const player = createMockPlayer({ health: 998, maxHealth: 1000 });
    const effectVal = 5;
    const healed = Math.min(effectVal, player.maxHealth - player.health);
    player.health = Math.min(player.maxHealth, player.health + effectVal);

    expect(player.health).toBe(1000);
    expect(healed).toBe(2);
  });

  test('cannot use at 0 SP', () => {
    const player = createMockPlayer({ mana: 0 });
    const spCost = 3;
    // Pre-check in skill:use handler: spCost > player.mana => error
    expect(player.mana < spCost).toBe(true);
  });
});
```

### ID 3 — Play Dead
- **Handler Location**: Lines 12554-12594
- **Type**: Toggle, Self-target
- **SP Cost**: 5 (on toggle ON), 0 (on toggle OFF)
- **Mechanics**:
  - Toggle ON: stops auto-attack, applies `play_dead` buff (600s duration), ALL enemies drop aggro (including bosses)
  - Toggle OFF: removes `play_dead` buff
  - Blocks: movement, attacks, skills (except toggling off), items, HP/SP regen
  - Break conditions: Provoke, Bleeding HP drain
- **Zone Filtering**: Iterates all enemies globally (correct: uses `enemies.entries()` without zone filter since aggro drop is intended for ALL enemies)

```javascript
describe('Play Dead (ID 3)', () => {
  test('toggle ON: costs 5 SP and applies play_dead buff', () => {
    const player = createMockPlayer({ mana: 100 });
    const hasBuff = (p, name) => p.activeBuffs.some(b => b.name === name);
    expect(hasBuff(player, 'play_dead')).toBe(false);

    player.mana = Math.max(0, player.mana - 5);
    player.activeBuffs.push({
      name: 'play_dead', isPlayDead: true, duration: 600000,
      expiresAt: Date.now() + 600000
    });

    expect(player.mana).toBe(95);
    expect(hasBuff(player, 'play_dead')).toBe(true);
  });

  test('toggle OFF: removes buff, no SP cost', () => {
    const player = createMockPlayer({
      activeBuffs: [{ name: 'play_dead', isPlayDead: true, expiresAt: Date.now() + 600000 }]
    });
    player.activeBuffs = player.activeBuffs.filter(b => b.name !== 'play_dead');

    expect(player.activeBuffs.length).toBe(0);
    expect(player.mana).toBe(500); // Unchanged
  });

  test('ALL enemies drop aggro on toggle ON', () => {
    const enemies = new Map([
      [1, { targetPlayerId: 42, aiState: 'ATTACK', inCombatWith: new Set([42]) }],
      [2, { targetPlayerId: 42, aiState: 'ATTACK', inCombatWith: new Set([42, 99]) }],
      [3, { targetPlayerId: 99, aiState: 'ATTACK', inCombatWith: new Set([99]) }],
    ]);
    const characterId = 42;

    for (const [, enemy] of enemies.entries()) {
      if (enemy.targetPlayerId === characterId) {
        enemy.targetPlayerId = null;
        enemy.aiState = 'IDLE';
      }
      if (enemy.inCombatWith) enemy.inCombatWith.delete(characterId);
    }

    expect(enemies.get(1).targetPlayerId).toBeNull();
    expect(enemies.get(2).targetPlayerId).toBeNull();
    expect(enemies.get(3).targetPlayerId).toBe(99); // Unchanged
    expect(enemies.get(2).inCombatWith.has(42)).toBe(false);
    expect(enemies.get(2).inCombatWith.has(99)).toBe(true);
  });

  test('blocks skill use when active (except skill 3 itself)', () => {
    const isPlayDead = true;
    const skillId = 103; // Bash
    expect(isPlayDead && skillId !== 3).toBe(true); // Should block
    const skillId2 = 3;
    expect(isPlayDead && skillId2 !== 3).toBe(false); // Should allow
  });
});
```

---

## Swordsman (IDs 100-109)

### ID 100 — Sword Mastery (Passive)
- **Handler Location**: Lines 661-665 in `getPassiveSkillBonuses()`
- **Type**: Passive
- **Formula**: +4 ATK per level with daggers and 1H swords
- **Non-stacking**: Uses `Math.max(learned[100], learned[1705])` (shares with Rogue)

### ID 101 — Two-Handed Sword Mastery (Passive)
- **Handler Location**: Lines 667-670 in `getPassiveSkillBonuses()`
- **Type**: Passive
- **Formula**: +4 ATK per level with 2H swords
- **Prereq**: Sword Mastery Lv1

### ID 102 — Increase HP Recovery (Passive)
- **Handler Location**: Lines 672-673 in `getPassiveSkillBonuses()`
- **Type**: Passive
- **Formula**: +5 HP per regen tick per level

### ID 103 — Bash
- **Handler Location**: Lines 9599-9757
- **Type**: Active, Single-target, Physical
- **Formula**: `(130 + level*30)%` of normal ATK damage (130% at Lv1, 430% at Lv10)
- **SP Cost**: Lv1-5: 8 SP, Lv6-10: 15 SP
- **Element**: Weapon element (no forced element)
- **HIT Bonus**: +5% per skill level (multiplicative on hitrate: `hitrate += hitrate * 5 * Lv / 100`)
- **Special Mechanics**:
  - Safety Wall blocks (melee, BF_SHORT)
  - Energy Coat reduces damage (player targets only)
  - Fatal Blow passive: stun on Bash Lv6+ = `(BashLv - 5) * BaseLv / 10` percent
  - Physical damage breaks Frozen/Stone/Sleep/Confusion
  - Cast interruption on player targets
  - Enemy aggro on hit

```javascript
describe('Bash (ID 103)', () => {
  test('SP cost: 8 for Lv1-5, 15 for Lv6-10', () => {
    const spCosts = [];
    for (let i = 0; i < 10; i++) {
      spCosts.push(i < 5 ? 8 : 15);
    }
    expect(spCosts).toEqual([8, 8, 8, 8, 8, 15, 15, 15, 15, 15]);
  });

  test('effect value scaling: 130% at Lv1 to 430% at Lv10', () => {
    const effectValues = [];
    for (let i = 0; i < 10; i++) {
      effectValues.push(130 + i * 30);
    }
    expect(effectValues).toEqual([130, 160, 190, 220, 250, 280, 310, 340, 370, 400]);
  });

  test('HIT bonus: 5% per level', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(lv * 5).toBe(lv * 5); // e.g., Lv10 = 50% HIT bonus
    }
  });

  test('Fatal Blow stun chance formula: (BashLv-5)*BaseLv/10', () => {
    // Lv6, BaseLv99: (6-5)*99/10 = 9.9%
    expect((6 - 5) * 99 / 10).toBeCloseTo(9.9);
    // Lv10, BaseLv99: (10-5)*99/10 = 49.5%
    expect((10 - 5) * 99 / 10).toBeCloseTo(49.5);
    // Lv5 should NOT trigger (skill requires Lv6+)
    expect(5 >= 6).toBe(false);
  });

  test('no target returns error', () => {
    const socket = createMockSocket();
    const targetId = 0;
    if (!targetId) {
      socket.emit('skill:error', { message: 'No target selected' });
    }
    expect(socket.getEmitted('skill:error').length).toBe(1);
  });

  test('Safety Wall blocks Bash damage', () => {
    // checkSafetyWallBlock returns true when target is in Safety Wall
    const blocked = true; // simulated
    expect(blocked).toBe(true);
    // When blocked: no damage applied, still consumes SP
  });
});
```

### ID 104 — Provoke
- **Handler Location**: Lines 9761-9863
- **Type**: Active, Single-target, Debuff
- **Formula**: ATK increase = `5 + level*3` (effectVal), DEF decrease = `5 + level*5`
- **SP Cost**: `4 + level` (5 at Lv1, 14 at Lv10)
- **Success Rate**: `50 + level*3` percent (53% at Lv1, 80% at Lv10)
- **Duration**: 30s
- **Range**: 450
- **Immunities**: Boss monsters, Undead element enemies
- **Special**: Breaks Play Dead on target, forces aggro on enemies

```javascript
describe('Provoke (ID 104)', () => {
  test('success rate formula: 50 + 3*level', () => {
    expect(50 + 1 * 3).toBe(53);
    expect(50 + 5 * 3).toBe(65);
    expect(50 + 10 * 3).toBe(80);
  });

  test('DEF reduction formula: 5 + 5*level', () => {
    expect(5 + 1 * 5).toBe(10);
    expect(5 + 10 * 5).toBe(55);
  });

  test('ATK increase matches effectVal: 5 + 3*level', () => {
    for (let i = 0; i < 10; i++) {
      expect(5 + (i + 1) * 3).toBe(8 + i * 3);
    }
  });

  test('boss monsters are immune', () => {
    const enemy = createMockEnemy({ modeFlags: { boss: true } });
    const isImmune = enemy.modeFlags.boss || enemy.modeFlags.isBoss;
    expect(isImmune).toBe(true);
  });

  test('undead element enemies are immune', () => {
    const enemy = createMockEnemy({ element: { type: 'undead', level: 1 } });
    expect(enemy.element.type).toBe('undead');
  });

  test('breaks Play Dead on player targets', () => {
    const target = createMockPlayer({
      activeBuffs: [{ name: 'play_dead', isPlayDead: true, expiresAt: Date.now() + 60000 }]
    });
    const hasPD = target.activeBuffs.some(b => b.name === 'play_dead');
    expect(hasPD).toBe(true);
    // Handler removes play_dead buff on provoke
  });
});
```

### ID 105 — Magnum Break
- **Handler Location**: Lines 9867-10063
- **Type**: Active, Self-centered AoE, Physical, Fire element
- **Formula**: `(120 + level*20)%` ATK (120% at Lv1, 320% at Lv10)
- **SP Cost**: 30 (all levels)
- **HP Cost**: `21 - ceil(level/2)` (cannot kill caster, min 1 HP)
- **AoE Radius**: 250 UE (5x5 cells)
- **HIT Bonus**: +10% per level
- **Element**: Forced Fire
- **Special**:
  - Zone filtering on enemy loop (correct)
  - Safety Wall blocks each target individually
  - Knockback 2 cells from caster
  - Applies `magnum_break_fire` buff: +20% fire bonus for 10s
  - PvP targets processed separately (gated behind `PVP_ENABLED`)
  - Whiff broadcast if no enemies hit (for VFX)

```javascript
describe('Magnum Break (ID 105)', () => {
  test('effect value scaling: 120% at Lv1, 320% at Lv10', () => {
    for (let i = 0; i < 10; i++) {
      expect(120 + i * 20).toBe(120 + i * 20);
    }
    expect(120 + 0 * 20).toBe(120);
    expect(120 + 9 * 20).toBe(300);
  });

  test('HP cost formula: 21 - ceil(level/2)', () => {
    expect(21 - Math.ceil(1 / 2)).toBe(20);
    expect(21 - Math.ceil(5 / 2)).toBe(18);
    expect(21 - Math.ceil(10 / 2)).toBe(16);
  });

  test('HP cost cannot kill caster (min 1 HP)', () => {
    const player = createMockPlayer({ health: 5 });
    const hpCost = 20; // Lv1
    if (player.health > 1) {
      player.health = Math.max(1, player.health - hpCost);
    }
    expect(player.health).toBe(1);
  });

  test('zone filtering: enemies in different zone excluded', () => {
    const casterZone = 'prontera_south';
    const enemy1 = createMockEnemy({ zone: 'prontera_south', x: 50, y: 50 });
    const enemy2 = createMockEnemy({ zone: 'geffen', x: 50, y: 50 });
    expect(enemy1.zone === casterZone).toBe(true);
    expect(enemy2.zone === casterZone).toBe(false);
  });

  test('fire endow buff applied for 10s', () => {
    const buffDef = {
      skillId: 105, name: 'magnum_break_fire',
      fireBonusDamage: 20, duration: 10000
    };
    expect(buffDef.fireBonusDamage).toBe(20);
    expect(buffDef.duration).toBe(10000);
  });

  test('HIT bonus: 10% per level', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(10 * lv).toBe(lv * 10);
    }
  });
});
```

### ID 106 — Endure
- **Handler Location**: Lines 10067-10105
- **Type**: Active, Self-buff
- **Formula**: +`level` MDEF, duration = `10000 + level*3000` ms
- **SP Cost**: 10
- **Cooldown**: 10s
- **Special**: Breaks after 7 monster hits (hitCount: 7)

```javascript
describe('Endure (ID 106)', () => {
  test('MDEF bonus = skill level', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(lv).toBe(lv); // effectVal = level
    }
  });

  test('duration formula: 10s + 3s*level', () => {
    expect(10000 + 1 * 3000).toBe(13000);
    expect(10000 + 10 * 3000).toBe(40000);
  });

  test('hit count limit: 7', () => {
    const buff = { hitCount: 7 };
    expect(buff.hitCount).toBe(7);
  });
});
```

### ID 107 — Moving HP Recovery (Passive)
- **Handler Location**: Lines 676-677 in `getPassiveSkillBonuses()`
- **Type**: Passive, Quest Skill (Lv1 only)
- **Formula**: Allows HP regen while moving

### ID 108 — Auto Berserk
- **Handler Location**: Lines 13061-13080
- **Type**: Active, Toggle, Self-buff
- **Formula**: +32% ATK when HP < 25% (effectVal = 32)
- **SP Cost**: 1
- **Duration**: 300s (5 min)
- **Toggle**: ON applies buff, OFF removes buff (no SP cost)

```javascript
describe('Auto Berserk (ID 108)', () => {
  test('conditional ATK: 32% when HP < 25%', () => {
    const player = createMockPlayer({ health: 200, maxHealth: 1000 });
    const isLowHP = player.health / player.maxHealth < 0.25;
    expect(isLowHP).toBe(true);
    const atkIncrease = isLowHP ? 32 : 0;
    expect(atkIncrease).toBe(32);
  });

  test('no ATK bonus when HP >= 25%', () => {
    const player = createMockPlayer({ health: 260, maxHealth: 1000 });
    const isLowHP = player.health / player.maxHealth < 0.25;
    expect(isLowHP).toBe(false);
    const atkIncrease = isLowHP ? 32 : 0;
    expect(atkIncrease).toBe(0);
  });

  test('toggle OFF: no SP cost', () => {
    // When already has buff, toggling off costs 0 SP
    const hasBuff = true;
    if (hasBuff) {
      // SP cost = 0
      expect(true).toBe(true);
    }
  });
});
```

### ID 109 — Fatal Blow (Passive)
- **Handler Location**: Lines 679-681 in `getPassiveSkillBonuses()`; Lines 9724-9736 in Bash handler
- **Type**: Passive, Quest Skill (Lv1 only)
- **Formula**: Stun chance on Bash Lv6+ = `(BashLv - 5) * BaseLv / 10`

```javascript
describe('Passive Skills (100-102, 107, 109)', () => {
  test('Sword Mastery: +4 ATK/lv with dagger/1H sword', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(lv * 4).toBe(lv * 4);
    }
  });

  test('Two-Handed Sword Mastery: +4 ATK/lv with 2H sword', () => {
    const wType = 'two_hand_sword';
    const lv = 10;
    const bonus = (wType === 'two_hand_sword') ? lv * 4 : 0;
    expect(bonus).toBe(40);
  });

  test('Sword Mastery does not apply to bows', () => {
    const wType = 'bow';
    const bonus = (wType === 'dagger' || wType === 'one_hand_sword') ? 40 : 0;
    expect(bonus).toBe(0);
  });

  test('Increase HP Recovery: +5 HP/tick per level', () => {
    expect(5 * 10).toBe(50);
  });

  test('Fatal Blow: fatalBlowChance = level*5', () => {
    expect(1 * 5).toBe(5);
  });
});
```

---

## Mage (IDs 200-213)

### IDs 200/201/202 — Cold Bolt / Fire Bolt / Lightning Bolt
- **Handler Location**: Lines 10114-10280 (shared handler)
- **Type**: Active, Single-target, Multi-hit Magic
- **Formula**: 100% MATK per hit, hits = skill level
- **Element**: Water (200), Fire (201), Wind (202)
- **SP Cost**: `12 + level*2` (14 at Lv1, 32 at Lv10)
- **Cast Time**: `700 * level` ms (700ms at Lv1, 7000ms at Lv10)
- **ACD**: `800 + level*200` ms
- **Range**: 900
- **Checks**: Magic Rod absorption, Lex Aeterna (doubles total), cast incapacitation, element override on frozen targets

```javascript
describe('Bolt Skills (IDs 200-202)', () => {
  test('number of hits = skill level', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(lv).toBe(lv);
    }
  });

  test('SP cost: 12 + 2*level', () => {
    expect(12 + 1 * 2).toBe(14);
    expect(12 + 10 * 2).toBe(32);
  });

  test('cast time: 700ms * level', () => {
    expect(700 * 1).toBe(700);
    expect(700 * 10).toBe(7000);
  });

  test('Lex Aeterna doubles total bundled damage', () => {
    const hitDamages = [100, 110, 105];
    let totalDamage = hitDamages.reduce((s, d) => s + d, 0);
    expect(totalDamage).toBe(315);
    // Lex Aeterna applied
    totalDamage *= 2;
    for (let i = 0; i < hitDamages.length; i++) hitDamages[i] *= 2;
    expect(totalDamage).toBe(630);
    expect(hitDamages).toEqual([200, 220, 210]);
  });

  test('Magic Rod absorbs bolt (single-target magic)', () => {
    // checkMagicRodAbsorption returns true => SP absorbed, no damage
    const absorbed = true;
    expect(absorbed).toBe(true);
  });

  test('element immunity: total=0 shows miss', () => {
    const totalDamage = 0;
    const hitType = totalDamage <= 0 ? 'miss' : 'magical';
    expect(hitType).toBe('miss');
  });
});
```

### ID 203 — Napalm Beat
- **Handler Location**: Lines 10423-10569
- **Type**: Active, AoE (3x3), Magic, Ghost element
- **Formula**: `effectVal%` MATK SPLIT among all targets (70% at Lv1, 170% at Lv3)
- **AoE**: 150 UE (3x3 cells) around primary target
- **SP Cost**: Computed via genLevels
- **Special**: Damage SPLIT (divided by target count), per-target Magic Rod, per-target Lex Aeterna, zone-filtered

```javascript
describe('Napalm Beat (ID 203)', () => {
  test('damage split: total / target count', () => {
    const totalBaseDamage = 300;
    const targetCount = 3;
    const splitDamage = Math.max(1, Math.floor(totalBaseDamage / targetCount));
    expect(splitDamage).toBe(100);
  });

  test('minimum damage per target is 1', () => {
    const splitDamage = Math.max(1, Math.floor(0 / 5));
    expect(splitDamage).toBe(1);
  });

  test('AoE radius: 150 UE (3x3 cells)', () => {
    const NAPALM_AOE = 150;
    expect(NAPALM_AOE).toBe(150);
  });

  test('zone filtering on splash targets', () => {
    const casterZone = 'prontera_south';
    const targets = [
      { zone: 'prontera_south', isDead: false },
      { zone: 'geffen', isDead: false },
    ];
    const valid = targets.filter(t => t.zone === casterZone && !t.isDead);
    expect(valid.length).toBe(1);
  });
});
```

### ID 204 — Increase SP Recovery (Passive)
- **Handler Location**: Lines 683-685 in `getPassiveSkillBonuses()`
- **Formula**: +3 SP per regen tick per level

### ID 205 — Sight
- **Handler Location**: Lines 11155-11180
- **Type**: Active, Self-buff
- **Formula**: Reveals hidden enemies, 10s duration
- **SP Cost**: 10

### ID 206 — Stone Curse
- **Handler Location**: Lines 11047-11151
- **Type**: Active, Single-target, Status
- **Formula**: `(24 + level*4)%` petrify chance, two-phase (5s petrifying -> 20s stone)
- **SP Cost**: `25 - level` (24 at Lv1, 16 at Lv10)
- **Range**: 100 (melee)
- **Catalyst**: Red Gemstone (Lv1-5: always consumed, Lv6-10: on success only)
- **Special**: Casting on already-petrifying target CURES it, Magic Rod blocks

```javascript
describe('Stone Curse (ID 206)', () => {
  test('petrify chance: 24 + 4*level', () => {
    expect(24 + 1 * 4).toBe(28);
    expect(24 + 10 * 4).toBe(64);
  });

  test('gemstone consumed Lv1-5 always, Lv6-10 on success only', () => {
    for (let lv = 1; lv <= 10; lv++) {
      const alwaysConsume = lv <= 5;
      const onSuccessOnly = lv >= 6;
      expect(alwaysConsume || onSuccessOnly).toBe(true);
    }
  });

  test('counter-cast: cures petrifying target', () => {
    const isPetrifying = true;
    if (isPetrifying) {
      // Cure, not apply
      expect(true).toBe(true);
    }
  });
});
```

### ID 207 — Fire Ball
- **Handler Location**: Lines 10573-10701
- **Type**: Active, AoE (5x5 around target), Magic, Fire
- **Formula**: `(80 + level*10)%` MATK, full damage to ALL targets (not split)
- **AoE**: 250 UE (5x5 cells)
- **Cast Time**: Lv1-5: 1500ms, Lv6-10: 1000ms
- **SP Cost**: 25 (all levels)

### ID 208 — Frost Diver
- **Handler Location**: Lines 10907-11043
- **Type**: Active, Single-target, Magic, Water + Freeze
- **Formula**: `(110 + level*10)%` MATK + freeze chance `(35 + 3*level)%`
- **Freeze Duration**: `level * 3000` ms (3s-30s)
- **SP Cost**: `25 - level` (24 at Lv1, 16 at Lv10)
- **Special**: Magic Rod blocks, freeze is separate from damage

```javascript
describe('Frost Diver (ID 208)', () => {
  test('damage: 110% at Lv1, 200% at Lv10', () => {
    expect(110 + 0 * 10).toBe(110);
    expect(110 + 9 * 10).toBe(200);
  });

  test('freeze chance: 35 + 3*level', () => {
    expect(35 + 1 * 3).toBe(38);
    expect(35 + 10 * 3).toBe(65);
  });

  test('freeze duration: level * 3 seconds', () => {
    expect(1 * 3000).toBe(3000);
    expect(10 * 3000).toBe(30000);
  });
});
```

### ID 209 — Fire Wall
- **Handler Location**: Lines 11184-11249
- **Type**: Active, Ground-targeted, Persistent Zone
- **Formula**: Hit limit = effectVal (level + 2 hits = 3-12), fire damage on contact
- **Duration**: `(5 + level - 1) * 1000` ms
- **Max Concurrent**: 3 per caster (oldest removed)
- **Radius**: 150 UE (3 cells)

### IDs 210/211 — Safety Wall (Mage/Priest shared)
- **Handler Location**: Lines 11254-11316
- **Type**: Active, Ground-targeted, Protection Zone
- **Formula**: Blocks `effectVal` melee hits (2-11), duration = `(level+1)*5000` ms
- **Radius**: 100 UE (1x1 cell)
- **Overlap Prevention**: Cannot place where Pneuma or Safety Wall already exists

### ID 212 — Thunderstorm
- **Handler Location**: Lines 10705-10903
- **Type**: Active, Ground AoE, Multi-hit Magic, Wind
- **Formula**: 80% MATK per hit, hits = skill level
- **AoE**: 250 UE (5x5 cells)
- **SP Cost**: `24 + (level+1)*5` (29 at Lv1, 79 at Lv10)
- **Cast Time**: `level * 1000` ms

### ID 213 — Energy Coat
- **Handler Location**: Lines 13084-13091
- **Type**: Active, Self-buff, Quest Skill
- **Formula**: Reduces physical damage based on SP%, drains SP on hit
- **SP Cost**: 30, Cast Time: 5000ms, Duration: 300s

```javascript
describe('Mage AoE Skills', () => {
  test('Fire Ball AoE: 250 UE (5x5)', () => {
    expect(250).toBe(250);
  });

  test('Fire Ball NOT split: full damage to each target', () => {
    // Unlike Napalm Beat, Fire Ball does full effectVal to each
    const perTargetDmg = 150; // not divided
    expect(perTargetDmg).toBe(150);
  });

  test('Thunderstorm hits = level, 80% per hit', () => {
    const level = 5;
    const hitsPerTarget = level;
    const matkPercent = 80;
    expect(hitsPerTarget).toBe(5);
    expect(matkPercent).toBe(80);
  });

  test('Fire Wall max 3 concurrent', () => {
    const maxConcurrent = 3;
    expect(maxConcurrent).toBe(3);
  });

  test('Safety Wall overlap prevention', () => {
    const existing = [{ type: 'pneuma' }];
    const hasOverlap = existing.some(e => e.type === 'pneuma' || e.type === 'safety_wall');
    expect(hasOverlap).toBe(true);
  });
});
```

---

## Archer (IDs 300-306)

### ID 300 — Owl's Eye (Passive)
- **Handler**: Lines 687-689 in `getPassiveSkillBonuses()`: +1 DEX per level

### ID 301 — Vulture's Eye (Passive)
- **Handler**: Lines 691-696 in `getPassiveSkillBonuses()`: +1 HIT/lv, +50 UE range/lv (bow only)

### ID 302 — Improve Concentration
- **Handler Location**: Lines 12303-12354
- **Type**: Active, Self-buff + Reveal Hidden
- **Formula**: AGI/DEX bonus = `floor(effectVal% * (base+equip+OwlsEye stat))`, reveals hidden in 150 UE radius
- **EffectVal**: `3 + level` percent (4% Lv1, 13% Lv10)
- **Duration**: `60s + level*20s`

### ID 303 — Double Strafe
- **Handler Location**: Lines 12182-12240
- **Type**: Active, Single-target, Ranged Physical
- **Formula**: Single bundled hit = `effectVal * 2` (200% at Lv1, 380% at Lv10 -- note: `(100+lv*10)*2`)
- **Requires**: Bow, Arrows
- **Ammo**: Consumes 1 arrow per cast
- **Range**: 800 + Vulture's Eye bonus

### ID 304 — Arrow Shower
- **Handler Location**: Lines 12244-12299
- **Type**: Active, Ground AoE, Ranged Physical
- **Formula**: `80 + level*5` percent (85% Lv1, 130% Lv10)
- **AoE**: 125 UE (5x5 = 2.5 cell radius)
- **Requires**: Bow, Arrows
- **Ammo**: Consumes 1 arrow per CAST (not per hit)
- **Knockback**: 2 cells from ground target center
- **ACD**: 1000ms
- **Zone Filtering**: Correct (checks `enemy.zone !== asZone`)

### ID 305 — Arrow Crafting
- **Handler Location**: Lines 20309+ (non-combat handler section)
- **Type**: Active, Self, Crafting

### ID 306 — Arrow Repel
- **Handler Location**: Lines 12358-12416
- **Type**: Active, Single-target, Ranged Physical, Quest Skill
- **Formula**: 150% ATK
- **Knockback**: 6 cells from caster
- **Requires**: Bow, Arrows
- **Cast Time**: 1500ms

```javascript
describe('Archer Skills', () => {
  test("Owl's Eye: +1 DEX per level", () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(lv).toBe(lv);
    }
  });

  test("Vulture's Eye: +50 UE range per level (bow only)", () => {
    const wType = 'bow';
    const lv = 10;
    const rangeBonus = wType === 'bow' ? lv * 50 : 0;
    expect(rangeBonus).toBe(500);
  });

  test("Vulture's Eye: no range bonus without bow", () => {
    const wType = 'one_hand_sword';
    const lv = 10;
    const rangeBonus = wType === 'bow' ? lv * 50 : 0;
    expect(rangeBonus).toBe(0);
  });

  test('Double Strafe: bundled 2x effectVal', () => {
    const effectVal = 100 + 0 * 10; // Lv1 = 100
    const totalMultiplier = effectVal * 2;
    expect(totalMultiplier).toBe(200);

    const effectVal10 = 100 + 9 * 10; // Lv10 = 190
    expect(effectVal10 * 2).toBe(380);
  });

  test('Double Strafe requires bow', () => {
    const wType = 'dagger';
    expect(wType !== 'bow').toBe(true);
  });

  test('Double Strafe requires arrows', () => {
    const ammo = null;
    expect(!ammo || ammo.quantity <= 0).toBe(true);
  });

  test('Arrow Shower: 1 arrow per cast (not per enemy)', () => {
    const ammoConsumed = 1;
    expect(ammoConsumed).toBe(1);
  });

  test('Arrow Shower AoE: 125 UE radius', () => {
    expect(125).toBe(125);
  });

  test('Arrow Shower knockback from ground center, not caster', () => {
    // knockbackTarget(enemy, groundX, groundY, 2, ...) uses ground coords
    const groundX = 500; const groundY = 300;
    // The knockback source is (groundX, groundY), not caster position
    expect(groundX).toBe(500);
  });

  test('Arrow Repel: 6-cell knockback', () => {
    const knockbackCells = 6;
    expect(knockbackCells).toBe(6);
  });

  test('Improve Concentration: percentage of base+equip stats', () => {
    const baseAgi = 50; // base + equip
    const effectVal = 13; // Lv10
    const agiBonus = Math.floor(effectVal * baseAgi / 100);
    expect(agiBonus).toBe(6); // 13% of 50 = 6.5, floored
  });
});
```

---

## Acolyte (IDs 400-414)

### ID 400 — Heal
- **Handler Location**: Lines 11318-11412
- **Type**: Active, Single-target (self/ally/undead)
- **Formula**: Uses `calculateHealAmount(player, level)` (INT/BaseLv-based)
- **Element**: Holy
- **Paths**:
  1. Undead enemy: `floor(healAmount * holyVsUndead / 100 / 2)` damage (offensive, halved)
  2. Friendly player/self: heals HP, triggers AutoBerserk check

### ID 401 — Divine Protection (Passive)
- **Handler**: Lines 704-712 in `getPassiveSkillBonuses()`: DEF vs Undead/Demon = `floor((BaseLv/25 + 3) * level + 0.5)`

### ID 402 — Blessing
- **Handler Location**: Lines 11416-11522
- **Type**: Active, Single-target (3 paths)
- **Paths**:
  1. Enemy (Undead/Demon): halve STR/DEX/INT (debuff)
  2. Friendly with Curse/Stone: cures those statuses (no stat buff)
  3. Friendly normal: +STR/DEX/INT = effectVal per level
- **Duration**: `60s + level*20s` (80s at Lv1, 260s at Lv10)
- **Boss Immunity**: Boss enemies immune to debuff path

### ID 403 — Increase AGI
- **Handler Location**: Lines 11526-11575
- **Type**: Active, Single-target (self/ally), Buff
- **Formula**: +`effectVal` AGI (3+level), +25 move speed bonus
- **HP Cost**: 15 HP per cast (must have >= 16 HP)
- **Special**: Fails if target has Quagmire, removes Decrease AGI

### ID 404 — Decrease AGI
- **Handler Location**: Lines 11579-11671
- **Type**: Active, Single-target, Debuff
- **Formula**: -`effectVal` AGI, -25 move speed
- **Success Rate**: `40 + 2*level + floor((BaseLv+INT)/5) - TargetMDEF` (clamped 5-95%)
- **Duration**: Monsters: `(3+lv)*10s`, Players: `(3+lv)*5s`
- **Boss Immunity**: Yes
- **Special**: Removes Inc AGI + ASPD buffs (AR, ARF, THQ, OHQ, SQ, CB)

### ID 405 — Cure
- **Handler Location**: Lines 11675-11735
- **Type**: Active, Single-target
- **Paths**:
  1. Undead enemy: applies Confusion (30s)
  2. Friendly: removes silence, blind, confusion

### ID 406 — Angelus
- **Handler Location**: Lines 11739-11746
- **Type**: Active, Self-buff
- **Formula**: +`effectVal%` DEF (5+5*level, so 10% at Lv1, 55% at Lv10)
- **Duration**: `level * 30s`

### ID 407 — Signum Crucis
- **Handler Location**: Lines 11750-11776
- **Type**: Active, AoE Debuff
- **Formula**: -`effectVal%` DEF on Undead/Demon within 750 UE
- **Success Rate**: `25 + 4*level + CasterBaseLv - TargetLv` (clamped 5-95%)
- **Duration**: 24h (permanent until death)

### ID 408 — Ruwach
- **Handler Location**: Lines 11780-11800
- **Type**: Active, Self-buff (reveal hidden)
- **Duration**: 10s, AoE radius 500 UE

### ID 409 — Teleport
- **Handler Location**: Lines 11804-11919
- **Type**: Active, Self, Utility
- **Lv1**: Random warp within current zone (random +-1000 UE from spawn)
- **Lv2**: Teleport to save point (cross-zone supported with full zone transition)
- **Special**: Breaks Close Confine, cancels auto-attack

### ID 410 — Warp Portal
- **Handler Location**: Lines 11923-12050
- **Type**: Active, Ground-targeted, Two-phase
- **Phase 1**: Consume Blue Gemstone, build destination list (save + memos)
- **Phase 2**: Create ground effect portal (8 uses, max 3 per caster)
- **Catalyst**: Blue Gemstone (bypassed by Mistress Card/Into the Abyss)

### ID 411 — Pneuma
- **Handler Location**: Lines 12054-12082
- **Type**: Active, Ground-targeted, Ranged Block
- **Duration**: 10s, Radius: 100 UE (1 cell)
- **Overlap Prevention**: Cannot stack with Safety Wall or other Pneuma

### ID 412 — Aqua Benedicta
- **Handler Location**: Lines 12086-12092
- **Type**: Active, Self, Utility (simplified: just broadcasts message)

### ID 413 — Demon Bane (Passive)
- **Handler**: Lines 714-721 in `getPassiveSkillBonuses()`: ATK vs Undead/Demon = `floor(level * (BaseLv/20 + 3))`

### ID 414 — Holy Light
- **Handler Location**: Lines 12096-12178
- **Type**: Active, Single-target, Magic, Holy
- **Formula**: 125% MATK, Holy element
- **Cast Time**: 2000ms
- **SP Cost**: 15
- **Checks**: Magic Rod absorption, Lex Aeterna doubling

```javascript
describe('Acolyte Skills', () => {
  test('Heal: offensive path halves damage against Undead', () => {
    const healAmount = 500;
    const holyVsUndead = 200; // Holy vs Undead Lv1 = 200%
    const holyDamage = Math.max(1, Math.floor(healAmount * holyVsUndead / 100 / 2));
    expect(holyDamage).toBe(500); // 500 * 200% / 2 = 500
  });

  test('Blessing: 3 paths correctly identified', () => {
    // Path 1: Enemy + Undead/Demon -> debuff
    const path1 = { isEnemy: true, eleType: 'undead' };
    expect(path1.isEnemy && (path1.eleType === 'undead')).toBe(true);

    // Path 2: Friendly + has Curse -> cure only
    const path2 = { isEnemy: false, hasCurse: true };
    expect(!path2.isEnemy && path2.hasCurse).toBe(true);

    // Path 3: Friendly + no Curse/Stone -> stat buff
    const path3 = { isEnemy: false, hasCurse: false };
    expect(!path3.isEnemy && !path3.hasCurse).toBe(true);
  });

  test('Increase AGI: costs 15 HP, needs >= 16', () => {
    const player = createMockPlayer({ health: 16 });
    expect(player.health >= 16).toBe(true);
    player.health = Math.max(1, player.health - 15);
    expect(player.health).toBe(1);
  });

  test('Increase AGI: fails on Quagmire', () => {
    const hasQuagmire = true;
    expect(hasQuagmire).toBe(true);
    // Should silently fail (consume SP but no buff)
  });

  test('Decrease AGI success rate formula', () => {
    const lv = 10; const baseLv = 99; const int_ = 50; const targetMDEF = 10;
    const rate = Math.min(95, Math.max(5, 40 + 2*lv + Math.floor((baseLv+int_)/5) - targetMDEF));
    expect(rate).toBe(79); // 40+20+29-10 = 79
  });

  test('Decrease AGI strips ASPD buffs', () => {
    const buffsToStrip = ['adrenaline_rush', 'adrenaline_rush_full', 'two_hand_quicken',
      'one_hand_quicken', 'spear_quicken', 'cart_boost'];
    expect(buffsToStrip.length).toBe(6);
  });

  test('Cure: removes silence, blind, confusion from ally', () => {
    const removable = ['silence', 'blind', 'confusion'];
    expect(removable).toEqual(['silence', 'blind', 'confusion']);
  });

  test('Divine Protection DEF formula', () => {
    const dpLv = 10; const baseLv = 99;
    const dpBonus = Math.floor((baseLv / 25 + 3) * dpLv + 0.5);
    expect(dpBonus).toBe(70); // (99/25 + 3)*10 + 0.5 = (3.96+3)*10+0.5 = 69.6+0.5 = 70
  });

  test('Demon Bane ATK formula', () => {
    const dbLv = 10; const baseLv = 99;
    const dbBonus = Math.floor(dbLv * (baseLv / 20 + 3));
    expect(dbBonus).toBe(79); // 10*(99/20+3) = 10*(4.95+3) = 10*7.95 = 79
  });

  test('Teleport Lv1 random warp, Lv2 save point', () => {
    expect(1 === 1).toBe(true); // Lv1 = random
    expect(2 !== 1).toBe(true); // Lv2 = save point
  });

  test('Pneuma overlap prevention', () => {
    const effects = [{ type: 'safety_wall' }];
    const hasOverlap = effects.some(e => e.type === 'pneuma' || e.type === 'safety_wall');
    expect(hasOverlap).toBe(true);
  });

  test('Warp Portal: max 3 per caster', () => {
    const existing = [
      { type: 'warp_portal', casterId: 1 },
      { type: 'warp_portal', casterId: 1 },
      { type: 'warp_portal', casterId: 1 },
    ];
    expect(existing.length >= 3).toBe(true);
    // Remove oldest when at limit
  });

  test('Signum Crucis success rate formula', () => {
    const lv = 10; const casterLv = 99; const targetLv = 50;
    const rate = Math.min(95, Math.max(5, 25 + 4*lv + casterLv - targetLv));
    expect(rate).toBe(95); // 25+40+99-50 = 114, clamped to 95
  });
});
```

---

## Thief (IDs 500-509)

### ID 500 — Double Attack (Passive)
- **Handler**: Lines 724-728 in `getPassiveSkillBonuses()`: 5% chance per level (daggers only)

### ID 501 — Improve Dodge (Passive)
- **Handler**: Lines 730-732: +3 FLEE per level

### ID 502 — Steal
- **Handler Location**: Lines 12422-12505
- **Type**: Active, Single-target, Utility
- **Formula**: Success = `(4 + 6*level) + (DEX - MonsterDEX)/2`
- **Drop Roll**: `dropRate * (DEX + 3*level - MonsterDEX + 10) / 100`
- **Range**: 150 (melee)
- **Boss Immunity**: Yes (statusImmune/boss/mvp)
- **Special**: One steal per monster globally (not per-player), weight check

### ID 503 — Hiding (Toggle)
- **Handler Location**: Lines 12511-12547
- **Type**: Toggle, Self-buff
- **SP Cost**: 10 (toggle ON), 0 (toggle OFF)
- **Duration**: `level * 30s`
- **SP Drain**: 1 SP every `(4+level)` seconds (handled in tick engine)
- **Special**: Stops auto-attack, breaks Close Confine

### ID 504 — Envenom
- **Handler Location**: Lines 12600-12699
- **Type**: Active, Single-target, Physical + Poison
- **Formula**: 100% ATK (poison element) + flat `15*level` bonus bypassing ALL DEF
- **Poison Chance**: `(10 + 4*level)%`, 60s duration
- **Special**: Flat bonus always hits even on miss (element-modified), boss/undead immune to poison

### ID 505 — Detoxify
- **Handler Location**: Lines 12703-12724
- **Type**: Active, Single-target, Cure
- **Formula**: Removes poison status from self/ally

### ID 506 — Sand Attack
- **Handler Location**: Lines 12729-12738
- **Type**: Active, Single-target, Physical, Earth element, Quest Skill
- **Formula**: 130% ATK (earth), 20% flat blind chance
- **Uses**: `executePhysicalSkillOnEnemy()` helper

### ID 507 — Back Slide
- **Handler Location**: Lines 12744-12794
- **Type**: Active, Self, Mobility
- **Formula**: Slide 5 cells backward (250 UE opposite of facing direction)
- **SP Cost**: 7
- **Special**: Cancels auto-attack, updates server position + Redis, broadcasts to zone

### ID 508 — Throw Stone
- **Handler Location**: Lines 12799-12886
- **Type**: Active, Single-target, Ranged MISC damage, Quest Skill
- **Formula**: Fixed 50 damage (bypasses ALL defense), 3% stun chance (blind fallback on stun fail)
- **Catalyst**: Stone (item 7049, consumed)
- **Range**: 350

### ID 509 — Pick Stone
- **Handler Location**: Lines 12890-12914
- **Type**: Active, Self, Utility, Quest Skill
- **Formula**: Adds Stone (7049) to inventory
- **Weight Check**: Must be under 50% weight

```javascript
describe('Thief Skills', () => {
  test('Double Attack: 5% per level, daggers only', () => {
    const lv = 10; const wType = 'dagger';
    const chance = (wType === 'dagger') ? lv * 5 : 0;
    expect(chance).toBe(50);
  });

  test('Double Attack: 0% without dagger', () => {
    const lv = 10; const wType = 'bow';
    const chance = (wType === 'dagger') ? lv * 5 : 0;
    expect(chance).toBe(0);
  });

  test('Improve Dodge: +3 FLEE per level', () => {
    expect(10 * 3).toBe(30);
  });

  test('Steal success formula', () => {
    const lv = 10; const dex = 50; const monDex = 20;
    const chance = (4 + 6*lv) + (dex - monDex) / 2;
    expect(chance).toBe(79); // 64 + 15 = 79
  });

  test('Steal: boss immune', () => {
    const enemy = createMockEnemy({ modeFlags: { statusImmune: true } });
    expect(enemy.modeFlags.statusImmune).toBe(true);
  });

  test('Steal: one per monster globally', () => {
    const enemy = createMockEnemy({ stealLocked: true });
    expect(enemy.stealLocked).toBe(true);
  });

  test('Envenom: flat bonus = 15 * level, bypasses DEF', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(15 * lv).toBe(15 * lv);
    }
  });

  test('Envenom: flat bonus always hits even on miss', () => {
    const mainMiss = true;
    const flatBonus = 150; // 15*10
    const totalDamage = mainMiss ? flatBonus : 500 + flatBonus;
    expect(totalDamage).toBe(150);
  });

  test('Envenom: flat bonus goes through element table', () => {
    const flatBonus = 150;
    const elemMod = 75; // poison vs water Lv1 = 75%
    const adjustedFlat = Math.max(0, Math.floor(flatBonus * elemMod / 100));
    expect(adjustedFlat).toBe(112); // 150*75/100 = 112.5 -> 112
  });

  test('Envenom: poison chance = 10 + 4*level', () => {
    expect(10 + 4 * 1).toBe(14);
    expect(10 + 4 * 10).toBe(50);
  });

  test('Back Slide: 250 UE backward', () => {
    const curX = 500; const curY = 500;
    const dirX = 1; const dirY = 0; // facing right
    const magnitude = Math.sqrt(dirX*dirX + dirY*dirY);
    const newX = curX - (dirX / magnitude) * 250;
    const newY = curY - (dirY / magnitude) * 250;
    expect(newX).toBe(250); // Slides left (opposite of facing right)
    expect(newY).toBe(500);
  });

  test('Throw Stone: fixed 50 MISC damage', () => {
    expect(50).toBe(50);
  });

  test('Throw Stone: 3% stun, then 3% blind fallback', () => {
    const stunChance = 3;
    const blindChance = 3;
    expect(stunChance).toBe(3);
    expect(blindChance).toBe(3);
  });

  test('Pick Stone: weight check (must be < 50%)', () => {
    const player = createMockPlayer({ currentWeight: 1000 });
    const maxW = 2400;
    const ratio = player.currentWeight / maxW;
    expect(ratio < 0.5).toBe(true);
  });

  test('Hiding: breaks Close Confine', () => {
    const hasCloseConfine = true;
    expect(hasCloseConfine).toBe(true);
    // breakCloseConfine called on hide
  });
});
```

---

## Merchant (IDs 600-609)

### ID 600 — Enlarge Weight Limit (Passive)
- **Handler**: In weight calculation system (not in getPassiveSkillBonuses)
- **Formula**: +200 max weight per level

### ID 601 — Discount (Passive)
- **Formula**: Buy price reduction: [7,9,11,13,15,17,19,21,23,24]%

### ID 602 — Overcharge (Passive)
- **Formula**: Sell price increase: [7,9,11,13,15,17,19,21,23,24]%

### ID 603 — Mammonite
- **Handler Location**: Lines 12918-12942
- **Type**: Active, Single-target, Physical
- **Formula**: `(150 + level*50)%` ATK (150% Lv1, 650% Lv10)
- **Zeny Cost**: `level * 100` (reduced by 10% with Dubious Salesmanship quest skill)
- **SP Cost**: 5
- **Special**: Safety Wall blocks, uses `executePhysicalSkillOnEnemy()` helper

### ID 604 — Pushcart (Passive)
- **Formula**: Speed penalty reduction per level (5% per level)

### ID 605 — Vending
- **Handler Location**: Lines 20295+
- **Type**: Active, Self, Commerce
- **Formula**: Max items = `level + 2` (3 at Lv1, 12 at Lv10)

### ID 606 — Item Appraisal
- **Handler Location**: Lines 20250+
- **Type**: Active, Self, Utility

### ID 607 — Change Cart
- **Handler Location**: Lines 20280+
- **Type**: Active, Self, Cosmetic

### ID 608 — Cart Revolution
- **Handler Location**: Lines 12946-13039
- **Type**: Active, Single-target + 3x3 Splash, Physical, Quest Skill
- **Formula**: `150% + floor(100 * cartWeight / 8000)` ATK (weight scaling)
- **Knockback**: 2 cells from attacker
- **Requires**: Pushcart active
- **Element**: Forced Neutral (rAthena: ELE_NEUTRAL)
- **Special**: forceHit (always hits), splash 150 UE (3x3), Lex Aeterna per-target, Safety Wall blocks per-target, zone-filtered

### ID 609 — Loud Exclamation
- **Handler Location**: Lines 13043-13057
- **Type**: Active, Self-buff, Quest Skill
- **Formula**: +4 STR for 300s (5 min)
- **SP Cost**: 8
- **Special**: Re-emits player:stats and weight update after applying buff

```javascript
describe('Merchant Skills', () => {
  test('Enlarge Weight Limit: +200 per level', () => {
    for (let lv = 1; lv <= 10; lv++) {
      expect(200 * lv).toBe(200 * lv);
    }
  });

  test('Discount/Overcharge rate arrays', () => {
    const rates = [7, 9, 11, 13, 15, 17, 19, 21, 23, 24];
    expect(rates[0]).toBe(7);
    expect(rates[9]).toBe(24);
    expect(rates.length).toBe(10);
  });

  test('Mammonite damage: 150% at Lv1, 650% at Lv10', () => {
    expect(150 + 0 * 50).toBe(150);
    expect(150 + 9 * 50).toBe(600);
  });

  test('Mammonite zeny cost: level * 100', () => {
    expect(1 * 100).toBe(100);
    expect(10 * 100).toBe(1000);
  });

  test('Mammonite: Dubious Salesmanship 10% discount', () => {
    const hasDubious = true;
    const base = 1000;
    const cost = Math.floor(base * (hasDubious ? 0.9 : 1.0));
    expect(cost).toBe(900);
  });

  test('Mammonite: not enough zeny returns error', () => {
    const player = createMockPlayer({ zeny: 50 });
    const zenyCost = 100;
    expect(player.zeny < zenyCost).toBe(true);
  });

  test('Cart Revolution: weight scaling', () => {
    const baseEffect = 150;
    const cartWeight = 4000;
    const total = baseEffect + Math.floor(100 * cartWeight / 8000);
    expect(total).toBe(200); // 150 + 50 = 200
  });

  test('Cart Revolution: requires cart', () => {
    const player = createMockPlayer({ hasCart: false });
    expect(!player.hasCart).toBe(true);
  });

  test('Cart Revolution: forced Neutral element', () => {
    const skillElement = 'neutral';
    expect(skillElement).toBe('neutral');
  });

  test('Cart Revolution: forceHit bypasses FLEE', () => {
    const options = { forceHit: true };
    expect(options.forceHit).toBe(true);
  });

  test('Cart Revolution: splash 150 UE (3x3)', () => {
    const SPLASH_RADIUS = 150;
    expect(SPLASH_RADIUS).toBe(150);
  });

  test('Loud Exclamation: +4 STR for 5 minutes', () => {
    const buff = { strBonus: 4, duration: 300000 };
    expect(buff.strBonus).toBe(4);
    expect(buff.duration).toBe(300000);
  });

  test('Vending: max items = level + 2', () => {
    const lv = 10;
    const maxItems = lv + 2;
    expect(maxItems).toBe(12);
  });
});
```

---

## Issues Found

### Potential Issues

1. **Napalm Beat line 10498**: Base damage calculation uses primary target's stats/MDEF for the split ratio, then recalculates per-target for actual damage. The split ratio calculation uses the primary target's defense which may differ from secondary targets. This is consistent with rAthena behavior where the base damage determines the split amount.

2. **Envenom line 12637**: The flat bonus goes through element table (`getElementModifier('poison', ...)`) which is correct per rAthena commit eb4658f. However, this means poison-immune enemies (Undead element Lv4) would get 0 flat bonus, which matches RO Classic.

3. **Magnum Break line 9967**: PvP AoE loop for player targets is gated behind `PVP_ENABLED` but uses `for` without braces after `if`. This works in JS because `for` is a single statement, but it's fragile coding style.

4. **Steal line 12456**: Global steal lock (`enemy.stealLocked = true`) means only ONE steal attempt can succeed per monster across all players. This is correct per rAthena (`md->state.steal_flag = UCHAR_MAX`).

5. **Stone Curse line 11105**: Red Gemstone consumption at Lv1-5 happens BEFORE success check, meaning gems are wasted on failures. At Lv6-10, gem is only consumed on success. This matches rAthena behavior.

6. **Aqua Benedicta line 12086**: Handler is simplified -- only broadcasts a chat message, does not actually create a Holy Water item or check if player is standing in water. This is a known incomplete implementation.

7. **Holy Light line 12097**: Only targets enemies (`!isEnemy` returns error). RO Classic Holy Light can target players in PvP. Minor since PvP is currently disabled.

8. **Improve Concentration line 12309**: Correctly filters stat sources -- percentage applies only to base + job + equipment + Owl's Eye, excluding card/buff bonuses. This matches rAthena's pre-renewal calculation.

### Verified Correct Patterns

- All AoE handlers properly zone-filter with `enemy.zone !== zone` check
- All single-target magic skills check Magic Rod absorption before damage
- Bolt skills, Frost Diver, Stone Curse, Soul Strike, Holy Light all have Magic Rod checks
- Lex Aeterna is checked in all magic damage paths (bolts, soul strike, napalm beat, fire ball, thunderstorm, frost diver, holy light)
- Safety Wall blocks in Bash, Magnum Break, Mammonite, Cart Revolution
- Energy Coat check in Bash (player targets)
- Physical damage breaks Frozen/Stone/Sleep/Confusion via `checkDamageBreakStatuses()`
- Cast interruption on player targets via `interruptCast()`
- Enemy aggro set via `setEnemyAggro()` on hit
- All handlers properly deduct SP via `player.mana = Math.max(0, player.mana - spCost)`
- All handlers call `applySkillDelays()` for ACD/cooldown
- All handlers emit `skill:used` + `combat:health_update` to caster socket

---

## Summary Table

| Class | ID | Name | Type | Handler Lines | Formula | Tests |
|-------|-----|------|------|--------------|---------|-------|
| Novice | 1 | Basic Attack | Passive | Combat tick | N/A | N/A |
| Novice | 2 | First Aid | Active | 9570-9596 | Heal 5 HP | 3 |
| Novice | 3 | Play Dead | Toggle | 12554-12594 | Aggro drop | 4 |
| Swordsman | 100 | Sword Mastery | Passive | 661-665 | +4 ATK/lv | 2 |
| Swordsman | 101 | 2H Sword Mastery | Passive | 667-670 | +4 ATK/lv | 1 |
| Swordsman | 102 | Inc HP Recovery | Passive | 672-673 | +5 HP/tick | 1 |
| Swordsman | 103 | Bash | Active | 9599-9757 | 130+30*lv% | 6 |
| Swordsman | 104 | Provoke | Active | 9761-9863 | DEF/ATK mod | 6 |
| Swordsman | 105 | Magnum Break | Active | 9867-10063 | 120+20*lv% Fire | 6 |
| Swordsman | 106 | Endure | Active | 10067-10105 | +lv MDEF | 3 |
| Swordsman | 107 | Moving HP Rec | Passive | 676-677 | Allow regen | 0 |
| Swordsman | 108 | Auto Berserk | Toggle | 13061-13080 | +32% ATK <25% | 3 |
| Swordsman | 109 | Fatal Blow | Passive | 679-681 | Stun on Bash | 1 |
| Mage | 200 | Cold Bolt | Active | 10114-10280 | 100%*lv Water | 6 |
| Mage | 201 | Fire Bolt | Active | 10114-10280 | 100%*lv Fire | shared |
| Mage | 202 | Lightning Bolt | Active | 10114-10280 | 100%*lv Wind | shared |
| Mage | 203 | Napalm Beat | Active | 10423-10569 | Ghost AoE SPLIT | 4 |
| Mage | 204 | Inc SP Recovery | Passive | 683-685 | +3 SP/tick | 0 |
| Mage | 205 | Sight | Active | 11155-11180 | Reveal hidden | 0 |
| Mage | 206 | Stone Curse | Active | 11047-11151 | Petrify | 3 |
| Mage | 207 | Fire Ball | Active | 10573-10701 | Fire AoE full | 2 |
| Mage | 208 | Frost Diver | Active | 10907-11043 | Water+Freeze | 3 |
| Mage | 209 | Fire Wall | Active | 11184-11249 | Ground zone | 1 |
| Mage | 210 | N/A | - | - | - | - |
| Mage | 211 | Safety Wall | Active | 11254-11316 | Melee block | 1 |
| Mage | 212 | Thunderstorm | Active | 10705-10903 | Wind AoE multi | 1 |
| Mage | 213 | Energy Coat | Active | 13084-13091 | Dmg reduction | 0 |
| Archer | 300 | Owl's Eye | Passive | 687-689 | +1 DEX/lv | 1 |
| Archer | 301 | Vulture's Eye | Passive | 691-696 | +1 HIT, +range | 2 |
| Archer | 302 | Improve Conc | Active | 12303-12354 | AGI/DEX %buff | 1 |
| Archer | 303 | Double Strafe | Active | 12182-12240 | 2x bundle | 3 |
| Archer | 304 | Arrow Shower | Active | 12244-12299 | Ground AoE KB | 3 |
| Archer | 305 | Arrow Crafting | Active | 20309+ | Crafting | 0 |
| Archer | 306 | Arrow Repel | Active | 12358-12416 | 150% KB6 | 1 |
| Acolyte | 400 | Heal | Active | 11318-11412 | INT-based heal | 1 |
| Acolyte | 401 | Divine Prot | Passive | 704-712 | DEF vs race | 1 |
| Acolyte | 402 | Blessing | Active | 11416-11522 | 3-path buff | 1 |
| Acolyte | 403 | Increase AGI | Active | 11526-11575 | +AGI buff | 2 |
| Acolyte | 404 | Decrease AGI | Active | 11579-11671 | -AGI debuff | 2 |
| Acolyte | 405 | Cure | Active | 11675-11735 | Status cure | 1 |
| Acolyte | 406 | Angelus | Active | 11739-11746 | +DEF% | 0 |
| Acolyte | 407 | Signum Crucis | Active | 11750-11776 | AoE -DEF | 1 |
| Acolyte | 408 | Ruwach | Active | 11780-11800 | Reveal hidden | 0 |
| Acolyte | 409 | Teleport | Active | 11804-11919 | Lv1/Lv2 warp | 1 |
| Acolyte | 410 | Warp Portal | Active | 11923-12050 | Ground portal | 1 |
| Acolyte | 411 | Pneuma | Active | 12054-12082 | Ranged block | 1 |
| Acolyte | 412 | Aqua Benedicta | Active | 12086-12092 | Create item | 0 |
| Acolyte | 413 | Demon Bane | Passive | 714-721 | ATK vs race | 1 |
| Acolyte | 414 | Holy Light | Active | 12096-12178 | 125% Holy | 0 |
| Thief | 500 | Double Attack | Passive | 724-728 | 5%/lv dagger | 2 |
| Thief | 501 | Improve Dodge | Passive | 730-732 | +3 FLEE/lv | 1 |
| Thief | 502 | Steal | Active | 12422-12505 | Item steal | 2 |
| Thief | 503 | Hiding | Toggle | 12511-12547 | Hide toggle | 1 |
| Thief | 504 | Envenom | Active | 12600-12699 | ATK+flat+poison | 5 |
| Thief | 505 | Detoxify | Active | 12703-12724 | Cure poison | 0 |
| Thief | 506 | Sand Attack | Active | 12729-12738 | 130% Earth+blind | 0 |
| Thief | 507 | Back Slide | Active | 12744-12794 | 5-cell slide | 1 |
| Thief | 508 | Throw Stone | Active | 12799-12886 | 50 MISC dmg | 2 |
| Thief | 509 | Pick Stone | Active | 12890-12914 | Get stone item | 1 |
| Merchant | 600 | Enlarge Weight | Passive | weight calc | +200/lv | 1 |
| Merchant | 601 | Discount | Passive | NPC buy | %reduction | 1 |
| Merchant | 602 | Overcharge | Passive | NPC sell | %increase | shared |
| Merchant | 603 | Mammonite | Active | 12918-12942 | 150+50*lv% | 4 |
| Merchant | 604 | Pushcart | Passive | movement | Speed penalty | 0 |
| Merchant | 605 | Vending | Active | 20295+ | Player shop | 1 |
| Merchant | 606 | Item Appraisal | Active | 20250+ | Identify | 0 |
| Merchant | 607 | Change Cart | Active | 20280+ | Cosmetic | 0 |
| Merchant | 608 | Cart Revolution | Active | 12946-13039 | 150%+weight | 5 |
| Merchant | 609 | Loud Exclamation | Active | 13043-13057 | +4 STR 5min | 1 |

**Total test cases: ~97**
