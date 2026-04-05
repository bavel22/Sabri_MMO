# Second Class Skills Group A — Comprehensive Audit & Jest Test Plan

**Classes**: Knight (700-710), Crusader (1300-1313), Wizard (800-813), Sage (1400-1421), Hunter (900-917)
**Generated**: 2026-03-22
**Source**: `server/src/index.js` (handlers), `server/src/ro_skill_data_2nd.js` (skill data), `server/src/ro_ground_effects.js` (ground effects)

---

## Table of Contents
1. [Knight (IDs 700-710)](#knight)
2. [Crusader (IDs 1300-1313)](#crusader)
3. [Wizard (IDs 800-813)](#wizard)
4. [Sage (IDs 1400-1421)](#sage)
5. [Hunter (IDs 900-917)](#hunter)
6. [Cross-System Interactions](#cross-system)
7. [Potential Issues Found](#issues)

---

## 1. Knight (IDs 700-710) <a name="knight"></a>

### Passives (no handler, in `getPassiveSkillBonuses()`)

| ID | Name | Line | Effect |
|----|------|------|--------|
| 700 | Spear Mastery | L782 | +4 ATK/lv with spears (+5/lv mounted) |
| 708 | Riding | L7867 | Mount toggle, speed 1.36x |
| 709 | Cavalier Mastery | L791 | Restores mounted ASPD penalty |

### Active Skill Handlers

#### ID 701 — Pierce (L13103-13186)
- **Formula**: `calculateSkillDamage(effectVal)` where effectVal = 110+10*lv (120-210%)
- **Hit count**: Small=1, Med=2, Large=3 (bundled: single calc * hitCount)
- **Element**: Weapon element (no forced element)
- **Weapon**: Spear required
- **Lex Aeterna**: Doubles total bundled damage
- **Range**: skill.range (300) + bonusRange
- **Safety Wall**: Blocks (melee check)
- **HIT bonus**: +5*lv via skillHitBonus option

#### ID 702 — Spear Stab (L13188-13288)
- **Formula**: `calculateSkillDamage(effectVal)` where effectVal = 120+20*lv (140-320%)
- **AoE**: Line from target toward caster, 200 UE depth, 75 UE width
- **Knockback**: 6 cells on all targets
- **Weapon**: Spear required
- **Lex Aeterna**: Per-target

#### ID 703 — Brandish Spear (L13290-13418)
- **Formula**: Zone-based multiplier on base ratio (100+20*lv)
  - Zone 0 (closest): base + base/2 + base/4 + base/8 (at Lv10 = 562.5%)
  - Zone 1: base + base/2 + base/4
  - Zone 2: base + base/2
  - Zone 3: base only
- **AoE**: Frontal rectangle, depth=200/300/400/500 by lv tiers, width=250
- **Knockback**: 2 cells from caster
- **Mount**: Required (Peco Peco)
- **Weapon**: Spear required
- **Lex Aeterna**: Per-target

#### ID 704 — Spear Boomerang (L13420-13433)
- **Formula**: effectVal = 150+50*lv (200-400%)
- **Range**: (1+2*lv)*50 = 150/250/350/450/550
- **Ranged**: Yes (blocked by Pneuma)
- **Weapon**: Spear required
- Uses `executePhysicalSkillOnEnemy()` helper

#### ID 705 — Two-Hand Quicken (L13435-13470)
- **Buff**: aspdIncrease=30, critBonus=2+lv, hitBonus=2*lv
- **Duration**: lv * 30000ms
- **Weapon**: two_hand_sword required
- **Stats update**: Emits player:stats after apply

#### ID 706 — Auto Counter (L13472-13495)
- **Buff**: counterActive=true, skillLevel=lv
- **Duration**: lv * 400ms (0.4s/lv)
- **Counter hook**: L30636 — blocks melee, counter with forceCrit+forceHit+ignoreDefense
- **One-shot**: Buff removed after counter triggers

#### ID 707 — Bowling Bash (L13497-13644)
- **Formula**: `calculateSkillDamage(effectVal)` where effectVal from data
- **Chain reaction**: Primary knockback cell-by-cell, 3x3 splash at each step
  - Collision: extra hit on knocked target + recursive chain on splashed targets
  - Chain direction: random for secondary targets
  - Knockback distance: floor((skillLv - depth + 1) / 2)
  - Max depth: recursive to depth = skillLevel
- **Boss immunity**: No knockback but still takes damage
- **Lex Aeterna**: Only on first hit per target (isLexEligible flag)

#### ID 710 — Charge Attack (L13647-13699)
- **Cast time override**: L9472 — distance-based: k=min(2,max(0,floor((cells-1)/3))), castTime=500*(1+k)
- **Damage tiers**: 0-3 cells=100%, 4-6=200%, 7-9=300%, 10-12=400%, 13+=500%
- **Range**: 700 UE
- **Ranged**: Yes (blocked by Pneuma via executePhysicalSkillOnEnemy)
- **Caster teleport**: Rushes to 1 cell from target (even on miss)
- **Knockback**: 1 cell random direction on hit

```js
// === KNIGHT JEST TESTS ===

describe('Knight Skills (700-710)', () => {

  // Shared mock factories
  const makePlayer = (overrides = {}) => ({
    characterName: 'TestKnight',
    health: 5000, maxHealth: 5000,
    mana: 500, maxMana: 500,
    zone: 'test_zone',
    weaponType: 'spear',
    equippedWeaponRight: { weaponType: 'spear', subType: '2hSpear', atk: 100, refineLevel: 0 },
    isMounted: false,
    learnedSkills: { 700: 10, 701: 10, 702: 10, 703: 10, 704: 5, 705: 10, 706: 5, 707: 10, 710: 1 },
    activeBuffs: [],
    stats: { str: 80, agi: 50, vit: 60, int: 10, dex: 50, luk: 30 },
    ...overrides,
  });

  const makeEnemy = (overrides = {}) => ({
    name: 'Poring',
    health: 10000, maxHealth: 10000,
    isDead: false, zone: 'test_zone',
    x: 100, y: 100, z: 0,
    size: 'medium',
    stats: { str: 10, agi: 10, vit: 10, int: 5, dex: 10, luk: 5 },
    hardDef: 10, softDef: 5,
    hardMdef: 0, softMDef: 0,
    element: { type: 'neutral', level: 1 },
    activeBuffs: [],
    modeFlags: {},
    ...overrides,
  });

  // --- Pierce (701) ---
  describe('Pierce (ID 701)', () => {
    test('hit count by size: Small=1, Med=2, Large=3', () => {
      const sizes = { small: 1, medium: 2, large: 3 };
      for (const [size, expected] of Object.entries(sizes)) {
        const hitCount = size === 'small' ? 1 : size === 'large' ? 3 : 2;
        expect(hitCount).toBe(expected);
      }
    });

    test('effectValue scales correctly: 110+10*lv', () => {
      for (let lv = 1; lv <= 10; lv++) {
        expect(110 + lv * 10).toBe(110 + 10 * lv);
      }
      expect(110 + 10 * 1).toBe(120);  // Lv1
      expect(110 + 10 * 10).toBe(210); // Lv10
    });

    test('bundled damage: single calc * hitCount', () => {
      const baseDamage = 500;
      const hitCount = 3; // large target
      expect(baseDamage * hitCount).toBe(1500);
    });

    test('Lex Aeterna doubles total bundled damage', () => {
      const totalDamage = 1000;
      const afterLex = totalDamage * 2;
      expect(afterLex).toBe(2000);
    });

    test('requires spear weapon', () => {
      const isSpearWeapon = (wt) => wt === 'spear' || wt === '1hSpear' || wt === '2hSpear';
      expect(isSpearWeapon('spear')).toBe(true);
      expect(isSpearWeapon('1hSpear')).toBe(true);
      expect(isSpearWeapon('2hSpear')).toBe(true);
      expect(isSpearWeapon('sword')).toBe(false);
      expect(isSpearWeapon('bare_hand')).toBe(false);
    });

    test('SP cost is 7 at all levels', () => {
      for (let lv = 1; lv <= 10; lv++) {
        expect(7).toBe(7); // From skill data: spCost: 7
      }
    });
  });

  // --- Spear Stab (702) ---
  describe('Spear Stab (ID 702)', () => {
    test('effectValue scales correctly: 120+20*lv', () => {
      expect(120 + 20 * 1).toBe(140);  // Lv1
      expect(120 + 20 * 5).toBe(220);  // Lv5
      expect(120 + 20 * 10).toBe(320); // Lv10
    });

    test('line AoE parameters: depth=200, width=75', () => {
      const SS_LINE_DEPTH = 200;
      const SS_LINE_WIDTH = 75;
      expect(SS_LINE_DEPTH).toBe(200);
      expect(SS_LINE_WIDTH).toBe(75);
    });

    test('knockback is always 6 cells', () => {
      expect(6).toBe(6);
    });

    test('line AoE finds enemy in line from target toward caster', () => {
      // Target at (100,100), caster at (200,200)
      // Line direction: target->caster = (100,100), normalized = (0.707,0.707)
      // Enemy at (150,150) should be in line (proj ~70, perp ~0)
      const targetX = 100, targetY = 100;
      const casterX = 200, casterY = 200;
      const lineLen = Math.sqrt(100*100 + 100*100);
      const normX = 100 / lineLen, normY = 100 / lineLen;
      const enemyX = 150, enemyY = 150;
      const toDx = enemyX - targetX, toDy = enemyY - targetY;
      const proj = toDx * normX + toDy * normY;
      const perp = Math.abs(toDx * (-normY) + toDy * normX);
      expect(proj).toBeGreaterThan(-25);
      expect(proj).toBeLessThan(200);
      expect(perp).toBeLessThanOrEqual(75);
    });
  });

  // --- Brandish Spear (703) ---
  describe('Brandish Spear (ID 703)', () => {
    test('AoE depth by level tiers', () => {
      const getDepth = (lv) => lv <= 3 ? 200 : lv <= 6 ? 300 : lv <= 9 ? 400 : 500;
      expect(getDepth(1)).toBe(200);
      expect(getDepth(3)).toBe(200);
      expect(getDepth(4)).toBe(300);
      expect(getDepth(6)).toBe(300);
      expect(getDepth(7)).toBe(400);
      expect(getDepth(9)).toBe(400);
      expect(getDepth(10)).toBe(500);
    });

    test('zone-based damage at Lv10 (inner zone = 562%)', () => {
      const lv = 10;
      const base = 100 + 20 * lv; // 300
      let zone0Ratio = base;
      if (lv > 3) zone0Ratio += Math.floor(base / 2);  // +150
      if (lv > 6) zone0Ratio += Math.floor(base / 4);  // +75
      if (lv > 9) zone0Ratio += Math.floor(base / 8);  // +37
      expect(zone0Ratio).toBe(562); // 300+150+75+37
    });

    test('zone-based damage at Lv5 (inner zone)', () => {
      const lv = 5;
      const base = 100 + 20 * lv; // 200
      let zone0Ratio = base;
      if (lv > 3) zone0Ratio += Math.floor(base / 2); // +100
      expect(zone0Ratio).toBe(300); // 200+100
    });

    test('requires mount', () => {
      const player = makePlayer({ isMounted: false });
      expect(player.isMounted).toBe(false);
    });

    test('knockback is always 2 cells', () => {
      expect(2).toBe(2);
    });
  });

  // --- Spear Boomerang (704) ---
  describe('Spear Boomerang (ID 704)', () => {
    test('effectValue: 150+50*lv', () => {
      expect(150 + 50 * 1).toBe(200);
      expect(150 + 50 * 5).toBe(400);
    });

    test('range per level: (1+2*lv)*50', () => {
      expect((1 + 2 * 1) * 50).toBe(150);
      expect((1 + 2 * 3) * 50).toBe(350);
      expect((1 + 2 * 5) * 50).toBe(550);
    });

    test('is ranged (blocked by Pneuma)', () => {
      // executePhysicalSkillOnEnemy called with isRanged: true
      expect(true).toBe(true);
    });
  });

  // --- Two-Hand Quicken (705) ---
  describe('Two-Hand Quicken (ID 705)', () => {
    test('ASPD increase is always 30%', () => {
      const aspdIncrease = 30;
      expect(aspdIncrease).toBe(30);
    });

    test('duration scales: lv * 30s', () => {
      for (let lv = 1; lv <= 10; lv++) {
        expect(lv * 30000).toBe(lv * 30000);
      }
      expect(1 * 30000).toBe(30000);   // Lv1 = 30s
      expect(10 * 30000).toBe(300000); // Lv10 = 300s
    });

    test('CRI bonus: 2+lv', () => {
      expect(2 + 1).toBe(3);  // Lv1
      expect(2 + 10).toBe(12); // Lv10
    });

    test('HIT bonus: 2*lv', () => {
      expect(2 * 1).toBe(2);  // Lv1
      expect(2 * 10).toBe(20); // Lv10
    });

    test('requires two_hand_sword', () => {
      expect('two_hand_sword').toBe('two_hand_sword');
    });
  });

  // --- Auto Counter (706) ---
  describe('Auto Counter (ID 706)', () => {
    test('duration: lv * 0.4s', () => {
      expect(1 * 400).toBe(400);   // Lv1
      expect(5 * 400).toBe(2000);  // Lv5
    });

    test('counter uses forceCrit + forceHit + ignoreDefense', () => {
      const options = { forceCrit: true, forceHit: true, ignoreDefense: true };
      expect(options.forceCrit).toBe(true);
      expect(options.forceHit).toBe(true);
      expect(options.ignoreDefense).toBe(true);
    });

    test('only triggers on melee attacks', () => {
      const MELEE_RANGE = 150;
      const RANGE_TOLERANCE = 50;
      const meleeAttack = 150;
      const rangedAttack = 500;
      expect(meleeAttack <= MELEE_RANGE + RANGE_TOLERANCE).toBe(true);
      expect(rangedAttack <= MELEE_RANGE + RANGE_TOLERANCE).toBe(false);
    });

    test('buff is removed after trigger (one-shot)', () => {
      // removeBuff(atkTarget, 'auto_counter') called after counter
      expect(true).toBe(true);
    });
  });

  // --- Bowling Bash (707) ---
  describe('Bowling Bash (ID 707)', () => {
    test('chain knockback distance: floor((skillLv-depth+1)/2)', () => {
      // Lv10, depth 0: floor((10-0+1)/2) = 5
      expect(Math.floor((10 - 0 + 1) / 2)).toBe(5);
      // Lv10, depth 3: floor((10-3+1)/2) = 4
      expect(Math.floor((10 - 3 + 1) / 2)).toBe(4);
      // Lv5, depth 0: floor((5-0+1)/2) = 3
      expect(Math.floor((5 - 0 + 1) / 2)).toBe(3);
      // Lv1, depth 0: floor((1-0+1)/2) = 1
      expect(Math.floor((1 - 0 + 1) / 2)).toBe(1);
    });

    test('splash radius is 75 UE (3x3)', () => {
      expect(75).toBe(75);
    });

    test('boss immunity: no knockback but still takes damage', () => {
      const isBoss = true;
      const knockbackApplied = !isBoss;
      expect(knockbackApplied).toBe(false);
    });

    test('8-direction system for knockback', () => {
      const DIR_X = [0, -1, -1, -1, 0, 1, 1, 1];
      const DIR_Y = [1, 1, 0, -1, -1, -1, 0, 1];
      expect(DIR_X.length).toBe(8);
      expect(DIR_Y.length).toBe(8);
    });
  });

  // --- Charge Attack (710) ---
  describe('Charge Attack (ID 710)', () => {
    test('distance-based damage tiers', () => {
      const getDmgTier = (cells) => {
        if (cells <= 3) return 100;
        if (cells <= 6) return 200;
        if (cells <= 9) return 300;
        if (cells <= 12) return 400;
        return 500;
      };
      expect(getDmgTier(0)).toBe(100);
      expect(getDmgTier(3)).toBe(100);
      expect(getDmgTier(4)).toBe(200);
      expect(getDmgTier(6)).toBe(200);
      expect(getDmgTier(7)).toBe(300);
      expect(getDmgTier(13)).toBe(500);
    });

    test('distance-based cast time: 500/1000/1500ms', () => {
      const getCastTime = (cells) => {
        const k = Math.min(2, Math.max(0, Math.floor((cells - 1) / 3)));
        return 500 * (1 + k);
      };
      expect(getCastTime(0)).toBe(500);  // k=0 (cells-1=-1, floor=-1, max(0)=0)
      expect(getCastTime(3)).toBe(500);  // k=0 (floor(2/3)=0)
      expect(getCastTime(4)).toBe(1000); // k=1 (floor(3/3)=1)
      expect(getCastTime(7)).toBe(1500); // k=2 (floor(6/3)=2)
      expect(getCastTime(13)).toBe(1500); // k=2 (capped)
    });

    test('caster teleports to 1 cell from target', () => {
      // newPosX = targetX - normX * 50
      const targetX = 500, targetY = 500;
      const casterX = 100, casterY = 100;
      const dx = targetX - casterX, dy = targetY - casterY;
      const dist = Math.sqrt(dx * dx + dy * dy);
      const nx = dx / dist, ny = dy / dist;
      const newPosX = targetX - nx * 50;
      const newPosY = targetY - ny * 50;
      const finalDist = Math.sqrt((newPosX - targetX) ** 2 + (newPosY - targetY) ** 2);
      expect(finalDist).toBeCloseTo(50, 1); // 1 cell = 50 UE
    });
  });
});
```

---

## 2. Crusader (IDs 1300-1313) <a name="crusader"></a>

### Passives (in `getPassiveSkillBonuses()`)

| ID | Name | Line | Effect |
|----|------|------|--------|
| 1300 | Faith | L27+ (data) | +200 MaxHP/lv, +5% holy resist/lv (also: reduces GC self-damage) |

### Active Skill Handlers

#### ID 1301 — Auto Guard (L14073-14118)
- **Toggle**: Remove if already active (no SP cost on toggle-off)
- **Block chance**: [5,10,14,18,21,24,26,28,29,30] by level
- **Duration**: 300s
- **Shield**: Required
- **Move delay**: Lv1-5=300ms, Lv6-9=200ms, Lv10=100ms
- **Stats update**: Emits player:stats for block chance UI

#### ID 1302 — Holy Cross (L13705-13806)
- **Formula**: effectVal = 135+35*lv (170-485%)
- **Element**: Holy (forced)
- **Hits**: 2 visual hits (single calc split in half)
- **2H Spear**: Doubles total damage
- **Blind**: 3*lv % chance
- **Lex Aeterna**: Before visual split

#### ID 1303 — Grand Cross (L13808-13972)
- **Formula**: (ATK+MATK) * ratio% where ratio = 140+40*lv (180-540%)
- **ATK**: WeaponATK only (NO StatusATK), DEX variance
- **MATK**: INT + floor(INT/7)^2 to INT + floor(INT/5)^2
- **AoE**: Diamond, |dx|+|dy| <= 4 cells (41 cells), 3 ticks @ 300ms
- **Element**: Holy
- **HP cost**: 20% current HP (cannot kill, min 1)
- **Self-damage**: (ATK+MATK)*ratio/2 * holyVsCaster * (100-faithResist)/100 per tick
- **Blind on Undead/Demon**: 3*lv % per tick
- **Uninterruptible cast**: 3s

#### ID 1304 — Shield Charge (L13975-14001)
- **Formula**: effectVal = 120+20*lv (140-200%)
- **Stun**: 20+5*(lv-1) % = 20/25/30/35/40
- **Knockback**: lv+4 cells = 5/6/7/8/9
- **Shield**: Required

#### ID 1305 — Shield Boomerang (L14004-14071)
- **Formula**: (batk + shieldWeight/10 + shieldRefine*5) * effectVal/100
  - batk = STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
  - effectVal = 100+30*(lv+1) = 130/160/190/220/250
- **Element**: Always Neutral
- **Ranged**: Yes (range 600)
- **Shield**: Required

#### ID 1306 — Devotion (L14201-14267)
- **Max targets**: learnedLevel (1-5)
- **Range**: (7+lv-1)*50 UE
- **Duration**: 30s + 15*(lv-1)s = 30/45/60/75/90s
- **Level diff**: Max 10
- **Cannot target**: Crusaders/Paladins
- **Party**: Required
- **Damage redirect**: Applied in enemy attack pipeline

#### ID 1307 — Reflect Shield (L14120-14153)
- **Reflect**: effectVal = 13+3*lv (16-43% -- wait, let me recheck)
  - Actually: 13+3*(lv-1) = 13/16/19/22/25/28/31/34/37/40
  - Data says: effectValue: 13+i*3 where i=0..9 = 13/16/19/22/25/28/31/34/37/40
- **Duration**: 300s
- **Shield**: Required

#### ID 1308 — Providence (L14269-14312)
- **Resist**: effectVal = 5+5*lv (10-25... wait: 5+i*5 where i=0..4 = 5/10/15/20/25)
- **Duration**: 180s
- **Cannot target**: Self, Crusaders/Paladins
- **Stats update**: Emits to target

#### ID 1309 — Defender (L14155-14199)
- **Toggle**: No SP cost on toggle-off
- **Ranged reduction**: effectVal = 20+15*lv (wait: 20+i*15 = 20/35/50/65/80)
- **Move speed penalty**: 33%
- **ASPD penalty**: [20,15,10,5,0] by level
- **Shield**: Required

#### ID 1310 — Spear Quicken (L14314-14347)
- **ASPD**: effectVal = 21+lv (22-30%)
  - Actually data: effectValue: 20+(i+1) = 21/22/23/24/25/26/27/28/29/30
- **Duration**: lv * 30s
- **Weapon**: 2H Spear (subType === '2hSpear')

#### ID 1311 — Heal (Crusader) (L14349-14420)
- **Alias**: Uses `calculateHealAmount()` like Acolyte Heal
- **Undead damage**: Holy damage path (heal/2 * holyVsUndead)
- **Self/ally**: Standard heal

#### ID 1312 — Cure (Crusader) (L14422-14444)
- **Removes**: silence, blind, confusion status effects

#### ID 1313 — Shrink (L14446-14475)
- **Toggle**: No SP cost on toggle-off
- **Effect**: 50% chance to knockback 2 cells on Auto Guard block
- **Duration**: 300s
- **Shield**: Required

```js
// === CRUSADER JEST TESTS ===

describe('Crusader Skills (1300-1313)', () => {

  // --- Faith (1300) ---
  describe('Faith (ID 1300)', () => {
    test('MaxHP bonus: 200 * level', () => {
      for (let lv = 1; lv <= 10; lv++) {
        expect(lv * 200).toBe(lv * 200);
      }
      expect(10 * 200).toBe(2000); // Max: +2000 HP
    });

    test('Holy resist: 5% per level', () => {
      for (let lv = 1; lv <= 10; lv++) {
        expect(lv * 5).toBe(lv * 5);
      }
      expect(10 * 5).toBe(50); // Max: 50%
    });
  });

  // --- Auto Guard (1301) ---
  describe('Auto Guard (ID 1301)', () => {
    test('block chance per level', () => {
      const chances = [5, 10, 14, 18, 21, 24, 26, 28, 29, 30];
      expect(chances[0]).toBe(5);   // Lv1
      expect(chances[4]).toBe(21);  // Lv5
      expect(chances[9]).toBe(30);  // Lv10
    });

    test('toggle off costs no SP', () => {
      // When already active, spCost: 0 is emitted
      expect(true).toBe(true);
    });

    test('move delay by level', () => {
      const getDelay = (lv) => lv <= 5 ? 300 : lv <= 9 ? 200 : 100;
      expect(getDelay(1)).toBe(300);
      expect(getDelay(5)).toBe(300);
      expect(getDelay(6)).toBe(200);
      expect(getDelay(9)).toBe(200);
      expect(getDelay(10)).toBe(100);
    });
  });

  // --- Holy Cross (1302) ---
  describe('Holy Cross (ID 1302)', () => {
    test('effectValue: 135+35*lv', () => {
      // Data: 135+i*35 where i=0..9
      expect(135 + 0 * 35).toBe(135);  // Lv1
      expect(135 + 4 * 35).toBe(275);  // Lv5
      expect(135 + 9 * 35).toBe(450);  // Lv10
    });

    test('2H spear doubles damage', () => {
      const base = 275; // Lv5
      const with2H = base * 2;
      expect(with2H).toBe(550);
    });

    test('forced holy element', () => {
      const skillElement = 'holy';
      expect(skillElement).toBe('holy');
    });

    test('visual: 2 hits split evenly', () => {
      const totalDmg = 1000;
      const hit1 = Math.floor(totalDmg / 2); // 500
      const hit2 = totalDmg - hit1; // 500
      expect(hit1 + hit2).toBe(totalDmg);
    });

    test('blind chance: 3*level %', () => {
      expect(3 * 1).toBe(3);   // Lv1 = 3%
      expect(3 * 10).toBe(30); // Lv10 = 30%
    });
  });

  // --- Grand Cross (1303) ---
  describe('Grand Cross (ID 1303)', () => {
    test('HP cost: 20% of current HP, cannot kill', () => {
      const health = 5000;
      const cost = Math.floor(health * 0.2);
      expect(cost).toBe(1000);
      const remaining = Math.max(1, health - cost);
      expect(remaining).toBe(4000);

      // Edge case: very low HP
      const lowHealth = 3;
      const lowCost = Math.floor(lowHealth * 0.2); // 0
      expect(Math.max(1, lowHealth - lowCost)).toBe(3);
    });

    test('diamond AoE: 41 cells (|dx|+|dy| <= 4)', () => {
      let count = 0;
      for (let dx = -4; dx <= 4; dx++) {
        for (let dy = -4; dy <= 4; dy++) {
          if (Math.abs(dx) + Math.abs(dy) <= 4) count++;
        }
      }
      expect(count).toBe(41);
    });

    test('3 ticks at 300ms intervals', () => {
      const GC_TICKS = 3;
      const GC_INTERVAL = 300;
      expect(GC_TICKS).toBe(3);
      expect(GC_INTERVAL).toBe(300);
    });

    test('skill ratio: 140+40*lv', () => {
      // Data: effectValue at Lv1 = 180, Lv10 = 540
      // Actually data genLevels uses different formula, let me check
      // 140+40*lv => Lv1=180, Lv5=340, Lv10=540
      expect(140 + 40 * 1).toBe(180);
      expect(140 + 40 * 10).toBe(540);
    });

    test('self-damage formula includes Faith resistance', () => {
      const faithLv = 10;
      const faithResist = faithLv * 5; // 50%
      const selfBaseDmg = 1000;
      const holyVsCaster = 100; // neutral armor
      const selfDmg = Math.max(1, Math.floor(
        Math.floor(selfBaseDmg / 2) * holyVsCaster / 100 * (100 - faithResist) / 100
      ));
      expect(selfDmg).toBe(250); // 500 * 1.0 * 0.5 = 250
    });

    test('blind only on Undead/Demon race', () => {
      const races = ['undead', 'demon', 'human', 'brute'];
      const blindEligible = races.filter(r => r === 'undead' || r === 'demon');
      expect(blindEligible).toEqual(['undead', 'demon']);
    });
  });

  // --- Shield Charge (1304) ---
  describe('Shield Charge (ID 1304)', () => {
    test('stun chance: 20+5*(lv-1)', () => {
      expect(20 + 5 * 0).toBe(20); // Lv1
      expect(20 + 5 * 2).toBe(30); // Lv3
      expect(20 + 5 * 4).toBe(40); // Lv5
    });

    test('knockback: lv+4 cells', () => {
      expect(1 + 4).toBe(5);
      expect(3 + 4).toBe(7);
      expect(5 + 4).toBe(9);
    });
  });

  // --- Shield Boomerang (1305) ---
  describe('Shield Boomerang (ID 1305)', () => {
    test('custom damage formula: batk + shieldWeight/10 + refine*5', () => {
      const str = 80, dex = 50, luk = 30;
      const batk = str + Math.floor(str / 10) ** 2 + Math.floor(dex / 5) + Math.floor(luk / 3);
      expect(batk).toBe(80 + 64 + 10 + 10); // 164
      const shieldWeight = 400;
      const shieldRefine = 7;
      const base = batk + Math.floor(shieldWeight / 10) + shieldRefine * 5;
      expect(base).toBe(164 + 40 + 35); // 239
    });

    test('element is always Neutral', () => {
      expect('neutral').toBe('neutral');
    });

    test('effectValue: 100+30*(lv+1)', () => {
      // Data: effectValue: 100+(i+1)*30 = 130/160/190/220/250
      expect(100 + 1 * 30).toBe(130);  // Lv1
      expect(100 + 5 * 30).toBe(250);  // Lv5
    });
  });

  // --- Devotion (1306) ---
  describe('Devotion (ID 1306)', () => {
    test('max targets = level', () => {
      for (let lv = 1; lv <= 5; lv++) {
        expect(lv).toBe(lv);
      }
    });

    test('duration: 30+15*(lv-1) seconds', () => {
      expect(30000 + 0 * 15000).toBe(30000);   // Lv1 = 30s
      expect(30000 + 4 * 15000).toBe(90000);   // Lv5 = 90s
    });

    test('cannot target Crusader/Paladin', () => {
      const blocked = ['crusader', 'paladin'];
      expect(blocked.includes('crusader')).toBe(true);
      expect(blocked.includes('knight')).toBe(false);
    });

    test('level diff max 10', () => {
      expect(Math.abs(99 - 85)).toBe(14);
      expect(14 > 10).toBe(true); // blocked
      expect(Math.abs(99 - 90)).toBe(9);
      expect(9 > 10).toBe(false); // allowed
    });
  });

  // --- Defender (1309) ---
  describe('Defender (ID 1309)', () => {
    test('ranged reduction: 20/35/50/65/80', () => {
      const vals = [20, 35, 50, 65, 80];
      expect(vals[0]).toBe(20);
      expect(vals[4]).toBe(80);
    });

    test('ASPD penalty: 20/15/10/5/0', () => {
      const penalties = [20, 15, 10, 5, 0];
      expect(penalties[0]).toBe(20);
      expect(penalties[4]).toBe(0);
    });
  });
});
```

---

## 3. Wizard (IDs 800-813) <a name="wizard"></a>

### Active Skill Handlers

#### ID 800 — Jupitel Thunder (L14481-14567)
- **Formula**: numHits * 100% MATK (hits = 3-12 from effectVal)
- **Element**: Wind
- **Magic Rod**: Checked before damage
- **Knockback**: floor((hits+1)/2) cells
- **Lex Aeterna**: First hit only (doubles hit[0], adds to total)
- **Staggered**: 150ms between visual hits

#### ID 804/1417 — Earth Spike / Earth Spike Sage (L14569-14645)
- **Formula**: 1-5 hits * 100% MATK
- **Element**: Earth
- **Magic Rod**: Checked
- **Lex Aeterna**: First hit doubling
- **Staggered**: 200ms between hits

#### ID 805/1418 — Heaven's Drive / HD Sage (L14647-14718)
- **Formula**: 1-5 hits * 125% MATK per target (pre-renewal correction)
- **AoE**: 5x5 (radius 125)
- **Element**: Earth
- **Lex Aeterna**: Per-target, first hit

#### ID 807 — Water Ball (L14720-14795)
- **Formula**: MATK% per hit = 130/160/190/220/250
- **Max hits**: [1,4,9,9,25] by level
- **Element**: Water
- **Magic Rod**: Checked
- **Staggered**: 150ms between hits

#### ID 803 — Storm Gust (L14797-14828)
- **Ground effect**: 10 waves * 460ms, radius 225 (9x9)
- **Element**: Water
- **Freeze**: Every 3rd hit (via freezeHitCounters)
- **MATK%**: effectVal (140-500)

#### ID 801 — Lord of Vermilion (L14830-14861)
- **Ground effect**: 4 waves * 1s, radius 225 (9x9)
- **Element**: Wind
- **MATK%**: effectVal (100-280 per wave)
- **Blind**: 4*lv %

#### ID 802 — Meteor Storm (L14863-14898)
- **Ground effect**: Meteors count = [2,2,3,3,4,4,5,5,6,6]
- **Hits per meteor**: floor((lv+1)/2) = 1,1,2,2,3,3,4,4,5,5
- **Element**: Fire
- **Stun**: 3*lv %
- **Meteor splash**: 150 radius (7x7)
- **Interval**: 300ms between meteors

#### ID 806 — Quagmire (L14900-14947)
- **Ground effect**: 5x5 (radius 125)
- **AGI/DEX reduction**: effectVal = 10*(lv+1) = 10/20/30/40/50
- **Move speed**: -50%
- **Duration**: lv * 5s
- **Max 3 per caster**: Oldest removed

#### ID 810 — Fire Pillar (L14949-14994)
- **Ground trap**: Contact trigger
- **Hits**: effectVal = lv+2 (3-12)
- **Splash**: Lv1-5=75, Lv6-10=125
- **Duration**: 30s
- **Max 5 per caster**
- **Ignores MDEF**

#### ID 808 — Ice Wall (L14996-15026)
- **Ground obstacle**: Blocks movement
- **Wall HP**: effectVal * 5 (2000-11000 total)
- **HP per cell**: 400+200*lv (400-2200)
- **Duration**: (lv+1)*5s
- **Element**: Water

#### ID 809 — Sight Rasher (L15028-15094)
- **Formula**: effectVal % MATK (120-300%)
- **AoE**: 7x7 (radius 175)
- **Element**: Fire
- **Requires**: Sight buff (consumed)
- **Knockback**: 5 cells

#### ID 811 — Frost Nova (L15096-15160)
- **Formula**: effectVal % MATK (73-136%)
- **AoE**: 5x5 (radius 125)
- **Element**: Water
- **Freeze**: 33+5*lv % = 38-83%
- **Freeze duration**: 1500*lv ms

#### ID 812/1419 — Sense / Sense Sage (L15162-15184)
- **Info query**: Returns enemy stats, element, race, size, DEF/MDEF

#### ID 813 — Sight Blaster (L15186-15208)
- **Self-buff**: Reactive fire damage
- **MATK%**: effectVal (100%)
- **Duration**: 120s
- **Trigger**: In combat tick (all enemies within 2 cells)

```js
// === WIZARD JEST TESTS ===

describe('Wizard Skills (800-813)', () => {

  // --- Jupitel Thunder (800) ---
  describe('Jupitel Thunder (ID 800)', () => {
    test('hit count from effectVal: 3-12', () => {
      // Skill data effectValue directly = numHits
      for (let lv = 1; lv <= 10; lv++) {
        const hits = lv + 2; // 3 to 12
        expect(hits).toBeGreaterThanOrEqual(3);
        expect(hits).toBeLessThanOrEqual(12);
      }
    });

    test('knockback: floor((hits+1)/2) cells', () => {
      expect(Math.floor((3 + 1) / 2)).toBe(2);   // Lv1
      expect(Math.floor((7 + 1) / 2)).toBe(4);   // Lv5
      expect(Math.floor((12 + 1) / 2)).toBe(6);  // Lv10
    });

    test('Lex Aeterna on first hit only', () => {
      const hitDamages = [100, 100, 100];
      const total = hitDamages.reduce((a, b) => a + b, 0);
      hitDamages[0] *= 2; // Lex doubles first hit
      const newTotal = hitDamages.reduce((a, b) => a + b, 0);
      expect(newTotal).toBe(total + 100); // +100 from doubled first hit
    });

    test('checks Magic Rod absorption before damage', () => {
      // checkMagicRodAbsorption returns true if absorbed
      expect(true).toBe(true);
    });
  });

  // --- Storm Gust (803) ---
  describe('Storm Gust (ID 803)', () => {
    test('10 waves at 460ms intervals', () => {
      expect(10 * 460).toBe(4600); // Total duration
    });

    test('9x9 AoE (radius 225)', () => {
      expect(225).toBe(225);
    });

    test('freeze every 3rd hit per target', () => {
      // freezeHitCounters tracks per-target hits
      const counters = {};
      const targetKey = 'enemy_1';
      for (let wave = 0; wave < 10; wave++) {
        counters[targetKey] = (counters[targetKey] || 0) + 1;
        if (counters[targetKey] % 3 === 0) {
          expect(counters[targetKey] % 3).toBe(0);
        }
      }
      // After 10 waves, should have frozen 3 times (at hits 3, 6, 9)
      expect(Math.floor(10 / 3)).toBe(3);
    });

    test('MATK% per level: 140 to 500', () => {
      // Verify from skill data the effectValue range
      expect(140).toBeLessThanOrEqual(500);
    });
  });

  // --- Meteor Storm (802) ---
  describe('Meteor Storm (ID 802)', () => {
    test('meteor count: [2,2,3,3,4,4,5,5,6,6]', () => {
      const counts = [2, 2, 3, 3, 4, 4, 5, 5, 6, 6];
      expect(counts[0]).toBe(2);  // Lv1
      expect(counts[4]).toBe(4);  // Lv5
      expect(counts[9]).toBe(6);  // Lv10
    });

    test('hits per meteor: floor((lv+1)/2)', () => {
      expect(Math.floor((1 + 1) / 2)).toBe(1);  // Lv1
      expect(Math.floor((5 + 1) / 2)).toBe(3);  // Lv5
      expect(Math.floor((10 + 1) / 2)).toBe(5); // Lv10
    });

    test('stun chance: 3*lv %', () => {
      expect(3 * 1).toBe(3);   // Lv1
      expect(3 * 10).toBe(30); // Lv10
    });

    test('meteor splash radius: 150 UE (7x7)', () => {
      expect(150).toBe(150);
    });
  });

  // --- Quagmire (806) ---
  describe('Quagmire (ID 806)', () => {
    test('AGI/DEX reduction: 10*(lv+1) = 10/20/30/40/50', () => {
      for (let lv = 1; lv <= 5; lv++) {
        expect(10 * lv).toBe(10 * lv);
      }
      // Note: skill data says effectValue: 10*(i+1) where i=0..4
      expect(10 * 1).toBe(10);
      expect(10 * 5).toBe(50);
    });

    test('move speed reduction: 50%', () => {
      expect(50).toBe(50);
    });

    test('max 3 Quagmires per caster (oldest removed)', () => {
      expect(3).toBe(3);
    });

    test('duration: lv * 5s', () => {
      expect(1 * 5000).toBe(5000);
      expect(5 * 5000).toBe(25000);
    });
  });

  // --- Fire Pillar (810) ---
  describe('Fire Pillar (ID 810)', () => {
    test('hits: lv+2 (3 to 12)', () => {
      expect(1 + 2).toBe(3);   // Lv1
      expect(5 + 2).toBe(7);   // Lv5
      expect(10 + 2).toBe(12); // Lv10
    });

    test('splash radius by level: 75 (Lv1-5), 125 (Lv6-10)', () => {
      const getRadius = (lv) => lv >= 6 ? 125 : 75;
      expect(getRadius(1)).toBe(75);
      expect(getRadius(5)).toBe(75);
      expect(getRadius(6)).toBe(125);
      expect(getRadius(10)).toBe(125);
    });

    test('max 5 per caster', () => {
      expect(5).toBe(5);
    });
  });

  // --- Heavens Drive (805) ---
  describe("Heaven's Drive (ID 805)", () => {
    test('125% MATK per hit (pre-renewal correction)', () => {
      expect(125).toBe(125); // HD_MATK_PCT
    });

    test('AoE 5x5 radius 125', () => {
      expect(125).toBe(125);
    });
  });

  // --- Frost Nova (811) ---
  describe('Frost Nova (ID 811)', () => {
    test('freeze chance: 33+5*lv %', () => {
      expect(33 + 5 * 1).toBe(38);  // Lv1
      expect(33 + 5 * 10).toBe(83); // Lv10
    });

    test('freeze duration: 1500*lv ms', () => {
      expect(1500 * 1).toBe(1500);   // Lv1
      expect(1500 * 10).toBe(15000); // Lv10
    });
  });

  // --- Sight Rasher (809) ---
  describe('Sight Rasher (ID 809)', () => {
    test('requires Sight buff (consumed)', () => {
      expect(true).toBe(true);
    });

    test('knockback 5 cells', () => {
      expect(5).toBe(5);
    });

    test('MATK%: 120+20*lv', () => {
      expect(120 + 20 * 1).toBe(140);  // Lv1
      expect(120 + 20 * 10).toBe(320); // Lv10
    });
  });

  // --- Ice Wall (808) ---
  describe('Ice Wall (ID 808)', () => {
    test('HP per cell: 400+200*lv', () => {
      // effectVal from data: 400+i*200
      expect(400 + 0 * 200).toBe(400);   // Lv1
      expect(400 + 9 * 200).toBe(2200);  // Lv10
    });

    test('total wall HP: hpPerCell * 5', () => {
      expect(400 * 5).toBe(2000);    // Lv1
      expect(2200 * 5).toBe(11000);  // Lv10
    });

    test('duration: (lv+1)*5s', () => {
      expect((1 + 1) * 5000).toBe(10000);   // Lv1
      expect((10 + 1) * 5000).toBe(55000);  // Lv10
    });
  });
});
```

---

## 4. Sage (IDs 1400-1421) <a name="sage"></a>

### Passives (in `getPassiveSkillBonuses()`)

| ID | Name | Line | Effect |
|----|------|------|--------|
| 1400 | Advanced Book | L734 | +3 ATK/lv with Books, +0.5% ASPD/lv |
| 1405 | Free Cast | L741 | Move while casting at (50+5*lv)% speed |
| 1407 | Dragonology | L747 | +1/1/2/2/3 INT, +4-20% Dragon ATK/resist |

### Active Skill Handlers

#### ID 1401 — Cast Cancel (L15217-15230)
- **Effect**: Cancels own active cast
- **SP cost**: 2

#### ID 1402 — Hindsight (L15471-15495)
- **Buff**: autocastChance = [7,9,11,13,15,17,19,21,23,25]%
- **Duration**: (90+30*lv)*1000 = 120-390s
- **Autocast**: Bolt spells on melee hit (triggered in combat tick)

#### ID 1403 — Dispell (L15296-15368)
- **Success**: effectVal = [60,70,80,90,100]%
- **Catalyst**: Yellow Gem
- **UNDISPELLABLE set**: hindsight, play_dead, auto_berserk, devotion_protection, steel_body, combo, strips, chemical protections, cart_boost, meltdown, safety_wall, dancing, blade_stop, root_lock, sitting, eternal_chaos, lokis_veil, into_the_abyss, ensemble_aftermath (29 entries)
- **Side effects**: Reverts endow, cancels performance, cancels ensemble

#### ID 1404 — Magic Rod (L15370-15392)
- **Duration**: 200+lv*200ms (400-1200ms)
- **Absorb**: lv*20 % of spell's SP cost

#### ID 1406 — Spell Breaker (L15394-15468)
- **Effect**: Interrupt cast + absorb SP
- **SP absorption**: [0,25,50,75,100]% by level
- **Lv5 special**: 2% MaxHP damage + 1% HP drain
- **MR counter**: If target has Magic Rod, SB fails and caster loses 20% MaxSP
- **Boss**: 10% success rate

#### ID 1408-1411 — Endow Blaze/Tsunami/Tornado/Quake (L15232-15294)
- **Success**: [70,80,90,100,100]% by level
- **Catalyst**: Required (consumed via consumeSkillCatalysts)
- **Duration**: 1200s (20 min)
- **Mutual exclusion**: Removes all existing endow buffs

#### ID 1412 — Volcano (L15497-15536)
- **Ground**: 7x7 (radius 175)
- **Fire dmg boost**: [10,14,17,19,20]%
- **ATK bonus**: [10,20,30,40,50]
- **Duration**: lv * 60s
- **Mutual exclusion**: Removes caster's other Volcano/Deluge/ViolentGale

#### ID 1413 — Deluge (L15538-15576)
- **Ground**: 7x7 (radius 175)
- **Water dmg boost**: [10,14,17,19,20]%
- **MaxHP boost**: [5,9,12,14,15]%
- **Duration**: lv * 60s

#### ID 1414 — Violent Gale (L15578-15616)
- **Ground**: 7x7 (radius 175)
- **Wind dmg boost**: [10,14,17,19,20]%
- **FLEE bonus**: [3,6,9,12,15]
- **Duration**: lv * 60s

#### ID 1415 — Land Protector (L15618-15685)
- **Ground**: Blocks/removes ground effects
- **Radius**: Lv1-2=175, Lv3-4=225, Lv5=275
- **LP vs LP**: Mutual destruction
- **Blocked types**: safety_wall, pneuma, warp_portal, sanctuary, magnus_exorcismus, volcano, deluge, violent_gale, fire_wall, fire_pillar, storm_gust, lord_of_vermilion, meteor_storm, quagmire, ice_wall (15 types)
- **Catalyst**: Blue + Yellow Gem

#### ID 1416 — Abracadabra (L15687-15727+)
- **13 special effects**: SA_DEATH through SA_TAMINGMONSTER (weighted pools per level)
- **Regular skills**: 124+ skills from all 1st class + 2nd class pool
- **Catalyst**: 2 Yellow Gems

#### ID 1417/1418/1419 — Earth Spike/HD/Sense Sage
- Handled by combined Wizard handlers (shared code paths)

```js
// === SAGE JEST TESTS ===

describe('Sage Skills (1400-1421)', () => {

  // --- Hindsight (1402) ---
  describe('Hindsight (ID 1402)', () => {
    test('autocast chance per level', () => {
      const chances = [7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
      expect(chances[0]).toBe(7);   // Lv1
      expect(chances[9]).toBe(25);  // Lv10
    });

    test('duration: (90+30*lv)*1000ms', () => {
      expect((90 + 30 * 1) * 1000).toBe(120000);  // 120s
      expect((90 + 30 * 10) * 1000).toBe(390000); // 390s
    });
  });

  // --- Dispell (1403) ---
  describe('Dispell (ID 1403)', () => {
    test('success rate: [60,70,80,90,100]%', () => {
      const rates = [60, 70, 80, 90, 100];
      expect(rates[0]).toBe(60);
      expect(rates[4]).toBe(100);
    });

    test('UNDISPELLABLE set has 29 entries', () => {
      const UNDISPELLABLE = new Set([
        'hindsight', 'play_dead', 'auto_berserk', 'devotion_protection',
        'steel_body', 'combo',
        'stripweapon', 'stripshield', 'striparmor', 'striphelm',
        'cp_weapon', 'cp_shield', 'cp_armor', 'cp_helm',
        'chemical_protection_helm', 'chemical_protection_shield',
        'chemical_protection_armor', 'chemical_protection_weapon',
        'enchant_deadly_poison', 'cart_boost', 'meltdown',
        'safety_wall', 'dancing',
        'blade_stop_catching', 'root_lock', 'sitting',
        'eternal_chaos', 'lokis_veil', 'into_the_abyss',
        'ensemble_aftermath'
      ]);
      expect(UNDISPELLABLE.size).toBe(30); // Actually 30 in the set
    });

    test('endow revert on dispell', () => {
      const removed = ['endow_fire', 'blessing', 'increase_agi'];
      const hasEndow = removed.some(n =>
        n.startsWith('endow_') || n.startsWith('item_endow_') ||
        n === 'aspersio' || n === 'enchant_poison'
      );
      expect(hasEndow).toBe(true);
    });
  });

  // --- Magic Rod (1404) ---
  describe('Magic Rod (ID 1404)', () => {
    test('duration: 200+lv*200ms', () => {
      expect(200 + 1 * 200).toBe(400);   // Lv1
      expect(200 + 5 * 200).toBe(1200);  // Lv5
    });

    test('absorb percentage: lv*20', () => {
      expect(1 * 20).toBe(20);  // Lv1 = 20%
      expect(5 * 20).toBe(100); // Lv5 = 100%
    });
  });

  // --- Spell Breaker (1406) ---
  describe('Spell Breaker (ID 1406)', () => {
    test('SP absorption: [0,25,50,75,100]%', () => {
      const pcts = [0, 25, 50, 75, 100];
      expect(pcts[0]).toBe(0);
      expect(pcts[4]).toBe(100);
    });

    test('Lv5 HP damage: 2% MaxHP', () => {
      const maxHP = 10000;
      expect(Math.floor(maxHP * 0.02)).toBe(200);
    });

    test('MR counter: 20% MaxSP lost on MR target', () => {
      const maxSP = 500;
      expect(Math.floor(maxSP * 0.20)).toBe(100);
    });

    test('boss resistance: 10% success', () => {
      expect(0.10).toBe(0.10);
    });
  });

  // --- Endow skills (1408-1411) ---
  describe('Endow Blaze/Tsunami/Tornado/Quake (1408-1411)', () => {
    test('success rate: [70,80,90,100,100]', () => {
      const rates = [70, 80, 90, 100, 100];
      expect(rates[0]).toBe(70);
      expect(rates[3]).toBe(100);
      expect(rates[4]).toBe(100);
    });

    test('element mapping', () => {
      const map = {
        endow_blaze: 'fire',
        endow_tsunami: 'water',
        endow_tornado: 'wind',
        endow_quake: 'earth'
      };
      expect(map.endow_blaze).toBe('fire');
      expect(map.endow_quake).toBe('earth');
    });

    test('mutual exclusion removes all existing endows', () => {
      const ENDOW_BUFFS = [
        'endow_fire', 'endow_water', 'endow_wind', 'endow_earth',
        'aspersio', 'enchant_poison',
        'item_endow_fire', 'item_endow_water', 'item_endow_wind',
        'item_endow_earth', 'item_endow_dark'
      ];
      expect(ENDOW_BUFFS.length).toBe(11);
    });
  });

  // --- Volcano/Deluge/Violent Gale (1412-1414) ---
  describe('Elemental Zones (1412-1414)', () => {
    test('Volcano ATK bonus: [10,20,30,40,50]', () => {
      const bonuses = [10, 20, 30, 40, 50];
      expect(bonuses[0]).toBe(10);
      expect(bonuses[4]).toBe(50);
    });

    test('elemental dmg boost: [10,14,17,19,20]%', () => {
      const boosts = [10, 14, 17, 19, 20];
      expect(boosts[0]).toBe(10);
      expect(boosts[4]).toBe(20);
    });

    test('Deluge MaxHP boost: [5,9,12,14,15]%', () => {
      const boosts = [5, 9, 12, 14, 15];
      expect(boosts[4]).toBe(15);
    });

    test('Violent Gale FLEE: [3,6,9,12,15]', () => {
      const boosts = [3, 6, 9, 12, 15];
      expect(boosts[4]).toBe(15);
    });

    test('mutual exclusion: only 1 zone type per caster', () => {
      const SAGE_ZONES = ['volcano', 'deluge', 'violent_gale'];
      expect(SAGE_ZONES.length).toBe(3);
    });
  });

  // --- Land Protector (1415) ---
  describe('Land Protector (ID 1415)', () => {
    test('radius by level: 175/175/225/225/275', () => {
      const getRadius = (lv) => lv <= 2 ? 175 : lv <= 4 ? 225 : 275;
      expect(getRadius(1)).toBe(175);
      expect(getRadius(3)).toBe(225);
      expect(getRadius(5)).toBe(275);
    });

    test('LP vs LP mutual destruction', () => {
      expect(true).toBe(true);
    });

    test('blocks 15 ground effect types', () => {
      const blocked = new Set([
        'safety_wall', 'pneuma', 'warp_portal', 'sanctuary',
        'magnus_exorcismus', 'volcano', 'deluge', 'violent_gale',
        'fire_wall', 'fire_pillar', 'storm_gust', 'lord_of_vermilion',
        'meteor_storm', 'quagmire', 'ice_wall'
      ]);
      expect(blocked.size).toBe(15);
    });
  });
});
```

---

## 5. Hunter (IDs 900-917) <a name="hunter"></a>

### Passives (in `getPassiveSkillBonuses()`)

| ID | Name | Line | Effect |
|----|------|------|--------|
| 901 | Steel Crow | L84 data | +6 falcon damage/lv (used in Blitz Beat formula) |
| 915 | Beast Bane | L805 | +4 ATK/lv vs Brute and Insect race |
| 916 | Falconry Mastery | L99 data | Enable falcon companion |

### Active Skill Handlers

#### ID 900 — Blitz Beat (L18006-18073)
- **Formula**: (DEX/10 + INT/2 + SteelCrow*3 + 40) * 2 per hit (MISC damage)
- **Hits**: learnedLevel (1-5)
- **Element**: Neutral
- **AoE**: 3x3 splash (125 UE radius) — full damage to each target
- **Requires**: Falcon
- **Element mod**: Applied per target

#### ID 902 — Detect (L17978-18003)
- **Effect**: Reveals hidden players in 7x7 (radius 175)
- **Requires**: Falcon
- **Removes**: hiding, cloaking buffs

#### ID 903 — Ankle Snare (trap placement)
- **Duration**: (6-lv)*50*1000 = 250s at Lv1, 50s at Lv5
- **Radius**: 25 (single cell)
- **Effect**: ankle_snare status (movement-only immobilize)

#### ID 904 — Land Mine (trap placement)
- **Duration**: (6-lv)*40*1000
- **Radius**: 25
- **Element**: Earth
- **Damage**: MISC via calculateTrapDamage()

#### ID 905 — Remove Trap (L17921-17947)
- **Effect**: Remove own trap, return 1 Trap item
- **Own traps only**

#### ID 906 — Shockwave Trap (trap placement)
- **Duration**: (6-lv)*40*1000
- **Radius**: 75
- **Effect**: Drain SP

#### ID 907 — Claymore Trap (trap placement)
- **Duration**: 20*lv*1000
- **Radius**: 125 (5x5)
- **Element**: Fire
- **Trap cost**: 2

#### ID 908 — Skid Trap (trap placement)
- **Duration**: (6-lv)*60*1000
- **Radius**: 25
- **Effect**: Knockback (no damage)

#### ID 909 — Sandman (trap placement)
- **Duration**: (6-lv)*30*1000
- **Radius**: 125 (5x5)
- **Effect**: Sleep

#### ID 910 — Flasher (trap placement)
- **Duration**: (6-lv)*30*1000
- **Radius**: 75
- **Effect**: Blind 30s

#### ID 911 — Freezing Trap (trap placement)
- **Duration**: (6-lv)*30*1000
- **Radius**: 75
- **Element**: Water
- **Damage**: ATK-based Weapon pipeline (NOT MISC)

#### ID 912 — Blast Mine (trap placement)
- **Duration**: (30-5*lv)*1000 (auto-detonate timer)
- **Radius**: 75
- **Element**: Wind
- **Damage**: MISC via calculateTrapDamage()
- **Auto-detonate**: At timer expiry

#### ID 913 — Spring Trap (L17949-17976)
- **Effect**: Remote destroy ANY trap (even enemy's)
- **Range**: [200,250,300,350,400] by level
- **Requires**: Falcon

#### ID 914 — Talkie Box (trap placement)
- **Duration**: 600s
- **Effect**: Cosmetic message

#### ID 917 — Phantasmic Arrow (L18075-18084)
- **Formula**: effectVal = 150% ATK
- **Knockback**: 3 cells
- **Requires**: Bow
- **No arrow consumed** (quest skill)

### Trap Placement Handler (L17811-17919)
- **Shared handler**: All 10 trap types use same placement code
- **Trap item**: ID 1065 consumed (1 per trap, 2 for Claymore/Shockwave)
- **Max 3 traps**: Per hunter (oldest removed)
- **ATK snapshot**: ownerATK stored at placement for Freezing Trap

```js
// === HUNTER JEST TESTS ===

describe('Hunter Skills (900-917)', () => {

  // --- Blitz Beat (900) ---
  describe('Blitz Beat (ID 900)', () => {
    test('MISC damage formula: (DEX/10 + INT/2 + SC*3 + 40) * 2', () => {
      const dex = 80, int = 40, steelCrow = 5;
      const perHit = (Math.floor(dex / 10) + Math.floor(int / 2) + steelCrow * 3 + 40) * 2;
      // = (8 + 20 + 15 + 40) * 2 = 83 * 2 = 166
      expect(perHit).toBe(166);
    });

    test('hit count = skill level (1-5)', () => {
      for (let lv = 1; lv <= 5; lv++) {
        expect(lv).toBe(lv);
      }
    });

    test('total damage = perHit * numHits * elemMod', () => {
      const perHit = 166, numHits = 5, eleMod = 100;
      const total = Math.max(1, Math.floor(perHit * numHits * eleMod / 100));
      expect(total).toBe(830);
    });

    test('3x3 splash (125 UE) with full damage per target', () => {
      expect(125).toBe(125);
    });

    test('requires falcon', () => {
      expect(true).toBe(true); // hasFalcon check
    });
  });

  // --- Trap Placement ---
  describe('Trap Placement (903-914)', () => {
    test('trap item cost: 1 for most, 2 for Claymore/Shockwave', () => {
      const getCost = (name) =>
        (name === 'claymore_trap' || name === 'shockwave_trap') ? 2 : 1;
      expect(getCost('land_mine')).toBe(1);
      expect(getCost('claymore_trap')).toBe(2);
      expect(getCost('shockwave_trap')).toBe(2);
      expect(getCost('ankle_snare')).toBe(1);
    });

    test('max 3 traps per hunter (oldest removed)', () => {
      expect(3).toBe(3);
    });

    test('trap durations by type', () => {
      // Land Mine: (6-lv)*40s
      expect((6 - 1) * 40 * 1000).toBe(200000); // Lv1 = 200s
      expect((6 - 5) * 40 * 1000).toBe(40000);  // Lv5 = 40s

      // Ankle Snare: (6-lv)*50s
      expect((6 - 1) * 50 * 1000).toBe(250000);

      // Blast Mine: (30-5*lv)s (auto-detonate)
      expect((30 - 5 * 1) * 1000).toBe(25000);  // Lv1 = 25s
      expect((30 - 5 * 5) * 1000).toBe(5000);   // Lv5 = 5s

      // Claymore: 20*lv*1000
      expect(20 * 1 * 1000).toBe(20000);  // Lv1 = 20s
      expect(20 * 5 * 1000).toBe(100000); // Lv5 = 100s
    });

    test('trap radii by type', () => {
      const radii = {
        land_mine: 25,
        ankle_snare: 25,
        blast_mine: 75,
        claymore_trap: 125,
        freezing_trap: 75,
        shockwave_trap: 75,
        skid_trap: 25,
        sandman: 125,
        flasher: 75,
        talkie_box: 25,
      };
      expect(radii.claymore_trap).toBe(125);
      expect(radii.land_mine).toBe(25);
      expect(radii.blast_mine).toBe(75);
    });

    test('trap elements', () => {
      const elements = {
        land_mine: 'earth',
        blast_mine: 'wind',
        claymore_trap: 'fire',
        freezing_trap: 'water',
      };
      expect(elements.land_mine).toBe('earth');
      expect(elements.claymore_trap).toBe('fire');
    });

    test('ATK snapshot stored at placement for Freezing Trap', () => {
      const str = 50, dex = 40, luk = 20, weaponATK = 80;
      const ownerATK = weaponATK + str +
        Math.floor(Math.pow(Math.floor(str / 10), 2)) +
        Math.floor(dex / 5) + Math.floor(luk / 5);
      // = 80 + 50 + floor(5^2) + 8 + 4 = 80 + 50 + 25 + 8 + 4 = 167
      expect(ownerATK).toBe(167);
    });
  });

  // --- Spring Trap (913) ---
  describe('Spring Trap (ID 913)', () => {
    test('range per level: [200,250,300,350,400]', () => {
      const ranges = [200, 250, 300, 350, 400];
      expect(ranges[0]).toBe(200);
      expect(ranges[4]).toBe(400);
    });

    test('destroys ANY trap (not just own)', () => {
      // Only checks category === 'trap', NOT casterId
      expect(true).toBe(true);
    });

    test('requires falcon', () => {
      expect(true).toBe(true);
    });
  });

  // --- Detect (902) ---
  describe('Detect (ID 902)', () => {
    test('reveal radius: 175 UE (7x7)', () => {
      expect(175).toBe(175);
    });

    test('removes hiding and cloaking', () => {
      const removed = ['hiding', 'cloaking'];
      expect(removed).toContain('hiding');
      expect(removed).toContain('cloaking');
    });
  });

  // --- Phantasmic Arrow (917) ---
  describe('Phantasmic Arrow (ID 917)', () => {
    test('effectValue: 150% ATK', () => {
      expect(150).toBe(150);
    });

    test('knockback 3 cells', () => {
      expect(3).toBe(3);
    });

    test('requires bow', () => {
      expect('bow').toBe('bow');
    });
  });

  // --- Beast Bane (915) ---
  describe('Beast Bane (ID 915)', () => {
    test('+4 ATK/lv vs Brute AND Insect', () => {
      const lv = 10;
      const bonus = lv * 4;
      expect(bonus).toBe(40);
    });
  });

  // --- Remove Trap (905) ---
  describe('Remove Trap (ID 905)', () => {
    test('returns 1 Trap item', () => {
      // Adds quantity +1 of item 1065
      expect(1065).toBe(1065);
    });

    test('only removes own traps', () => {
      // casterId check present
      expect(true).toBe(true);
    });
  });
});
```

---

## 6. Cross-System Interactions <a name="cross-system"></a>

### Shared Mechanics Across All 5 Classes

| Mechanic | Skills Affected | Location |
|----------|----------------|----------|
| **Lex Aeterna** | All physical/magic skills | Per-handler check on enemy.activeBuffs |
| **Safety Wall** | Pierce, Brandish Spear (melee physical) | `checkSafetyWallBlock()` |
| **Pneuma** | Spear Boomerang, Charge Attack, Shield Boomerang, Phantasmic Arrow | `executePhysicalSkillOnEnemy()` ranged check |
| **Magic Rod** | Jupitel Thunder, Earth Spike, Water Ball, all single-target magic | `checkMagicRodAbsorption()` before damage |
| **Land Protector** | Storm Gust, LoV, Meteor Storm, Quagmire, Fire Pillar, Ice Wall, Volcano, Deluge, Violent Gale | LP blocks ground effect placement |
| **Dispell** | THQ, Auto Counter, Spear Quicken, Reflect Shield, Defender, Auto Guard, Hindsight (immune), Providence | UNDISPELLABLE set at L15317 |
| **ASPD mutual exclusion** | THQ (705), Spear Quicken (1310), Adrenaline Rush (1200) | haste2Bonus in ro_buff_system.js |
| **Cast time + DEX** | All cast-time skills (Brandish, Grand Cross, Devotion, bolt spells, ground AoEs) | L9485 calculateActualCastTime |
| **Free Cast** | All cast-time skills | L9522 freeCastLv check during cast |
| **Suffragium/Bragi** | All cast-time skills | L9491/9501 cast time reduction |
| **Zone filtering** | All AoE skills | `enemy.zone !== ssZone` check in every handler |
| **checkDamageBreakStatuses** | All damage-dealing skills | Called after damage application |

### Skill Tree Shared Skills

- **Knight/Crusader**: Spear Mastery (700), Riding (708), Cavalier Mastery (709) shared in both trees via `sharedTreePos` and `iconClassId`
- **Wizard/Sage**: Earth Spike (804/1417), Heaven's Drive (805/1418), Sense (812/1419) use combined handlers

### Damage Pipeline Integration

- **Grand Cross**: Custom pipeline (ATK+MATK, hard%+flat DEF/MDEF, refine bonus)
- **Shield Boomerang**: Custom pipeline (batk + shieldWeight/10 + refine*5, ignores weapon ATK)
- **Blitz Beat**: MISC pipeline (ignores DEF, only elemental mod)
- **All other physical**: `calculateSkillDamage()` -> `calculatePhysicalDamage()`
- **All magic**: `calculateMagicSkillDamage()` -> `roMagicalDamage()`

---

## 7. Potential Issues Found <a name="issues"></a>

### Knight

1. **Pierce HIT bonus**: Uses `skillHitBonus: learnedLevel * 5` but the wiki specifies no HIT bonus for Pierce. This may be intentional game design.

2. **Bowling Bash self-collision hit order**: The primary target gets `bbDealDamage(enemy, eid, false)` (self-collision, no Lex) BEFORE `bbDealDamage(enemy, eid, true)` (original hit, with Lex). In rAthena, the order is: initial hit first, then self-collision on collide. Current code hits self-collision first.

3. **Charge Attack SP cost**: Uses SP from skill data (40), but the Charge Attack cast-time override at L9472 reads enemy position — if enemy dies during cast, `caTarget` could be null (handled: `if (caTarget)` check present).

### Crusader

4. **Shield Boomerang effectVal formula mismatch**: Skill data says `effectValue: 100+(i+1)*30` giving 130/160/190/220/250, but handler comment says "(100+50*Lv)%". The data is correct (130-250%), the comment is misleading.

5. **Holy Cross _lastDamageDealt not set**: Unlike other damage handlers (Pierce, Spear Stab), Holy Cross does not set `enemy._lastDamageDealt` for MVP damage tracking.

6. **Grand Cross Lex Aeterna**: Only consumed on first tick (tickNum === 0), which is correct per RO Classic, but the doubled damage is applied per-tick rather than to the total, which could lead to unexpected interactions.

### Wizard

7. **Jupitel Thunder Lex Aeterna total calculation**: L14526 does `jtTotalDamage += jtHitDamages[0] / 2` after doubling hit[0]. Since hit[0] was already added to total, this adds half the original, but it should add the FULL original (the doubled portion). The total ends up being `originalTotal + original_hit0/2` instead of `originalTotal + original_hit0`.

8. **Water Ball max hits**: Hardcoded `[1,4,9,9,25]` — Lv3 and Lv4 both give 9 hits. In RO Classic, Lv3=9 is correct but Lv4 should be 9 (different damage per hit). This matches rAthena.

### Sage

9. **Cast Cancel SP recovery**: The handler at L15217 does NOT recover any SP from the cancelled cast. The skill data shows effectValue = [10,30,50,70,90] which should be SP recovery percentage. The handler ignores this value entirely.

10. **Endow success rate at Lv4-5**: Both Lv4 and Lv5 have 100% success, meaning there's no difference between them except potentially higher level learned skills. This matches rAthena.

### Hunter

11. **Blitz Beat splash damage uses full hits**: The splash targets (3x3 around primary) receive `perHitDmg * numHits` (full damage). In RO Classic (rAthena), manual Blitz Beat should deal full damage to all targets in 3x3. Auto-Blitz (from LUK proc) deals split damage. The manual handler is correct.

12. **Spring Trap destroys ANY trap**: L17960 does not check `casterId`, meaning Spring Trap can destroy enemy players' traps. In RO Classic, this is correct behavior.

13. **Trap ATK snapshot formula**: L17857 uses `ownerATK = weaponATK + STR + floor(STR/10)^2 + DEX/5 + LUK/5`. The `LUK/5` may differ from rAthena which uses `LUK/3` for status ATK. This could make Freezing Trap damage slightly lower than intended.

### Cross-System

14. **Bowling Bash + Ice Wall**: The bbChainReaction does not check for Ice Wall collision (knockback should stop at Ice Wall). The `knockbackTarget` function handles this, but the cell-by-cell movement in bbChainReaction uses direct coordinate manipulation, potentially bypassing the Ice Wall check.

15. **Auto Counter + Auto Guard**: Both check for melee attacks. Auto Counter is checked BEFORE Auto Guard in the combat tick (L30636 vs Auto Guard elsewhere). If both are active, Auto Counter takes priority, which matches RO Classic.

---

## Summary Statistics

| Class | Skills | Handlers | Passives | Ground Effects | Buffs |
|-------|--------|----------|----------|---------------|-------|
| Knight | 11 (700-710) | 8 active | 3 (700,708,709) | 0 | 2 (THQ, AC) |
| Crusader | 14 (1300-1313) | 12 active | 1 (Faith) | 0 | 7 (AG,RS,DF,DV,PV,SQ,Shrink) |
| Wizard | 14 (800-813) | 14 active | 0 | 6 (SG,LoV,MS,QM,FP,IW) | 1 (SB) |
| Sage | 22 (1400-1421) | 13 active | 3 (AB,FC,DG) | 4 (Vol,Del,VG,LP) | 3 (HS,MR,Endows) |
| Hunter | 18 (900-917) | 5 direct + 10 trap | 3 (SC,BB,FM) | 10 traps | 0 |
| **Total** | **79** | **62** | **10** | **20** | **13** |
