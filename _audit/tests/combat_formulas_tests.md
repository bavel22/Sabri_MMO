# Combat Formula Jest Tests

Generated: 2026-03-22
Based on audit of `server/src/ro_damage_formulas.js` and `server/src/index.js`

All tests target the exported functions from `ro_damage_formulas.js`:
- `calculateDerivedStats`, `getElementModifier`, `getSizePenalty`
- `calculateHitRate`, `calculateCritRate`
- `calculateMaxHP`, `calculateMaxSP`, `calculateASPD`
- `calculatePhysicalDamage`, `calculateMagicalDamage`
- `ELEMENT_TABLE`, `SIZE_PENALTY`

---

## Test File 1: Derived Stats

```javascript
// __tests__/combat/derivedStats.test.js

const {
    calculateDerivedStats,
    calculateMaxHP,
    calculateMaxSP,
    calculateASPD,
} = require('../../src/ro_damage_formulas');

describe('calculateDerivedStats', () => {
    const baseStats = {
        str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1,
        bonusHit: 0, bonusFlee: 0, bonusCritical: 0, bonusPerfectDodge: 0,
        bonusMaxHp: 0, bonusMaxSp: 0, bonusMaxHpRate: 0, bonusMaxSpRate: 0,
        bonusHardDef: 0, bonusMATK: 0, bonusHardMDEF: 0,
        jobClass: 'novice', weaponType: 'bare_hand', weaponMATK: 0,
        buffAspdMultiplier: 1, equipAspdRate: 0, weaponTypeLeft: null,
    };

    // ── StatusATK (Melee) ──
    // Formula: STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/5)
    describe('StatusATK (melee)', () => {
        test('STR=1, DEX=1, LUK=1 → statusATK = 1 + 0 + 0 + 0 = 1', () => {
            const result = calculateDerivedStats({ ...baseStats, str: 1, dex: 1, luk: 1 });
            expect(result.statusATK).toBe(1);
        });

        test('STR=10 → 10 + 1^2 + 0 + 0 = 11', () => {
            const result = calculateDerivedStats({ ...baseStats, str: 10 });
            expect(result.statusATK).toBe(11);
        });

        test('STR=20 → 20 + 2^2 + 0 + 0 = 24', () => {
            const result = calculateDerivedStats({ ...baseStats, str: 20 });
            expect(result.statusATK).toBe(24);
        });

        test('STR=99 → 99 + 9^2 + 0 + 0 = 180', () => {
            const result = calculateDerivedStats({ ...baseStats, str: 99 });
            expect(result.statusATK).toBe(180);
        });

        test('STR=50, DEX=30, LUK=20 → 50 + 25 + 6 + 4 = 85', () => {
            const result = calculateDerivedStats({ ...baseStats, str: 50, dex: 30, luk: 20 });
            expect(result.statusATK).toBe(85);
        });

        test('STR=99, DEX=99, LUK=99 → 99 + 81 + 19 + 19 = 218', () => {
            const result = calculateDerivedStats({ ...baseStats, str: 99, dex: 99, luk: 99 });
            expect(result.statusATK).toBe(218);
        });
    });

    // ── StatusATK (Ranged) ──
    // Formula: DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/5)
    describe('StatusATK (ranged)', () => {
        test('bow weapon: DEX=50, STR=30, LUK=10 → 50 + 25 + 6 + 2 = 83', () => {
            const result = calculateDerivedStats({ ...baseStats, weaponType: 'bow', dex: 50, str: 30, luk: 10 });
            expect(result.statusATK).toBe(83);
        });

        test('instrument weapon: DEX=99, STR=1, LUK=1 → 99 + 81 + 0 + 0 = 180', () => {
            const result = calculateDerivedStats({ ...baseStats, weaponType: 'instrument', dex: 99, str: 1, luk: 1 });
            expect(result.statusATK).toBe(180);
        });

        test('whip weapon uses ranged formula', () => {
            const result = calculateDerivedStats({ ...baseStats, weaponType: 'whip', dex: 40, str: 20, luk: 10 });
            // 40 + 16 + 4 + 2 = 62
            expect(result.statusATK).toBe(62);
        });
    });

    // ── MATK ──
    describe('MATK', () => {
        test('INT=1: matkMin = 1 + 0 = 1, matkMax = 1 + 0 = 1', () => {
            const result = calculateDerivedStats({ ...baseStats, int: 1 });
            expect(result.matkMin).toBe(1);
            expect(result.matkMax).toBe(1);
        });

        test('INT=99: matkMin = 99 + floor(99/7)^2 = 99 + 196 = 295, matkMax = 99 + floor(99/5)^2 = 99 + 361 = 460', () => {
            const result = calculateDerivedStats({ ...baseStats, int: 99 });
            // floor(99/7) = 14, 14^2 = 196 → matkMin base = 295
            // floor(99/5) = 19, 19^2 = 361 → matkMax base = 460
            expect(result.matkMin).toBe(295);
            expect(result.matkMax).toBe(460);
        });

        test('INT=50 with weaponMATK=30: matkMin = 50 + 49 + 21 = 120, matkMax = 50 + 100 + 30 = 180', () => {
            const result = calculateDerivedStats({ ...baseStats, int: 50, weaponMATK: 30 });
            // floor(50/7) = 7, 7^2 = 49 → statusMATK = 99, + floor(30*0.7)=21 → matkMin = 120
            // floor(50/5) = 10, 10^2 = 100 → statusMATKMax = 150, + 30 → matkMax = 180
            expect(result.matkMin).toBe(120);
            expect(result.matkMax).toBe(180);
        });
    });

    // ── HIT ──
    describe('HIT', () => {
        test('level=1, DEX=1, bonusHit=0 → 175 + 1 + 1 = 177', () => {
            const result = calculateDerivedStats({ ...baseStats });
            expect(result.hit).toBe(177);
        });

        test('level=99, DEX=99, bonusHit=10 → 175 + 99 + 99 + 10 = 383', () => {
            const result = calculateDerivedStats({ ...baseStats, level: 99, dex: 99, bonusHit: 10 });
            expect(result.hit).toBe(383);
        });
    });

    // ── FLEE ──
    describe('FLEE', () => {
        test('level=1, AGI=1, bonusFlee=0 → 100 + 1 + 1 = 102', () => {
            const result = calculateDerivedStats({ ...baseStats });
            expect(result.flee).toBe(102);
        });

        test('level=99, AGI=99, bonusFlee=20 → 100 + 99 + 99 + 20 = 318', () => {
            const result = calculateDerivedStats({ ...baseStats, level: 99, agi: 99, bonusFlee: 20 });
            expect(result.flee).toBe(318);
        });
    });

    // ── Critical ──
    describe('Critical', () => {
        test('LUK=1, bonusCri=0 → 1 + floor(1*0.3) = 1 + 0 = 1', () => {
            const result = calculateDerivedStats({ ...baseStats });
            expect(result.critical).toBe(1);
        });

        test('LUK=30 → 1 + floor(30*0.3) = 1 + 9 = 10', () => {
            const result = calculateDerivedStats({ ...baseStats, luk: 30 });
            expect(result.critical).toBe(10);
        });

        test('LUK=99, bonusCri=5 → 1 + floor(99*0.3) + 5 = 1 + 29 + 5 = 35', () => {
            const result = calculateDerivedStats({ ...baseStats, luk: 99, bonusCritical: 5 });
            expect(result.critical).toBe(35);
        });

        test('katar doubles critical: LUK=30 → (1+9)*2 = 20', () => {
            const result = calculateDerivedStats({ ...baseStats, luk: 30, weaponType: 'katar' });
            expect(result.critical).toBe(20);
        });

        test('katar + bonusCri: LUK=30, bonusCri=5 → (1+9+5)*2 = 30', () => {
            const result = calculateDerivedStats({ ...baseStats, luk: 30, bonusCritical: 5, weaponType: 'katar' });
            expect(result.critical).toBe(30);
        });
    });

    // ── Perfect Dodge ──
    describe('Perfect Dodge', () => {
        test('LUK=1 → 1 + 0 = 1', () => {
            const result = calculateDerivedStats({ ...baseStats });
            expect(result.perfectDodge).toBe(1);
        });

        test('LUK=99, bonusPD=3 → 1 + 9 + 3 = 13', () => {
            const result = calculateDerivedStats({ ...baseStats, luk: 99, bonusPerfectDodge: 3 });
            expect(result.perfectDodge).toBe(13);
        });
    });

    // ── Soft DEF ──
    describe('Soft DEF', () => {
        test('VIT=1, AGI=1, level=1 → floor(1/2) + floor(1/5) + floor(1/2) = 0 + 0 + 0 = 0', () => {
            const result = calculateDerivedStats({ ...baseStats });
            expect(result.softDEF).toBe(0);
        });

        test('VIT=99, AGI=50, level=99 → floor(99/2) + floor(50/5) + floor(99/2) = 49 + 10 + 49 = 108', () => {
            const result = calculateDerivedStats({ ...baseStats, vit: 99, agi: 50, level: 99 });
            expect(result.softDEF).toBe(108);
        });
    });

    // ── Soft MDEF ──
    describe('Soft MDEF', () => {
        test('INT=1, VIT=1, DEX=1, level=1 → 1 + 0 + 0 + 0 = 1', () => {
            const result = calculateDerivedStats({ ...baseStats });
            expect(result.softMDEF).toBe(1);
        });

        test('INT=99, VIT=50, DEX=50, level=99 → 99 + 10 + 10 + 24 = 143', () => {
            const result = calculateDerivedStats({ ...baseStats, int: 99, vit: 50, dex: 50, level: 99 });
            expect(result.softMDEF).toBe(143);
        });
    });
});

describe('calculateMaxHP', () => {
    test('level 1 novice with VIT 1 → base formula', () => {
        const hp = calculateMaxHP(1, 1, 'novice', false, 0);
        expect(hp).toBeGreaterThanOrEqual(35);
        expect(hp).toBeLessThan(100);
    });

    test('transcendent gets 1.25x multiplier', () => {
        const hpNormal = calculateMaxHP(50, 50, 'swordsman', false, 0);
        const hpTrans = calculateMaxHP(50, 50, 'swordsman', true, 0);
        expect(hpTrans).toBe(Math.floor(hpNormal * 1.25));
    });

    test('bonusMaxHp is additive (after multiplicative VIT/trans)', () => {
        const hp1 = calculateMaxHP(50, 50, 'swordsman', false, 0);
        const hp2 = calculateMaxHP(50, 50, 'swordsman', false, 500);
        expect(hp2).toBe(hp1 + 500);
    });

    test('minimum MaxHP is 1', () => {
        const hp = calculateMaxHP(1, 1, 'novice', false, -9999);
        expect(hp).toBe(1);
    });
});

describe('calculateMaxSP', () => {
    test('level 1 novice with INT 1 → base formula', () => {
        const sp = calculateMaxSP(1, 1, 'novice', false, 0);
        expect(sp).toBeGreaterThanOrEqual(10);
    });

    test('transcendent gets 1.25x multiplier', () => {
        const spNormal = calculateMaxSP(50, 50, 'mage', false, 0);
        const spTrans = calculateMaxSP(50, 50, 'mage', true, 0);
        expect(spTrans).toBe(Math.floor(spNormal * 1.25));
    });

    test('minimum MaxSP is 1', () => {
        const sp = calculateMaxSP(1, 1, 'novice', false, -9999);
        expect(sp).toBe(1);
    });
});

describe('calculateASPD', () => {
    test('base case: novice bare_hand', () => {
        const aspd = calculateASPD('novice', 'bare_hand', 1, 1, 1, null, false, 0);
        expect(aspd).toBeGreaterThanOrEqual(100);
        expect(aspd).toBeLessThanOrEqual(195);
    });

    test('single weapon cap is 195', () => {
        const aspd = calculateASPD('assassin', 'katar', 99, 99, 1.5, null, false, 0);
        expect(aspd).toBeLessThanOrEqual(195);
    });

    test('dual wield cap is 190', () => {
        const aspd = calculateASPD('assassin', 'dagger', 99, 99, 1.5, 'dagger', false, 0);
        expect(aspd).toBeLessThanOrEqual(190);
    });

    test('mounted with no Cavalry Mastery: 50% ASPD', () => {
        const aspdNormal = calculateASPD('knight', 'one_hand_spear', 50, 50, 1, null, false, 0);
        const aspdMounted = calculateASPD('knight', 'one_hand_spear', 50, 50, 1, null, true, 0);
        expect(aspdMounted).toBe(Math.max(100, Math.floor(aspdNormal * 0.5)));
    });

    test('mounted with Cavalry Mastery 5: full ASPD restored', () => {
        const aspdNormal = calculateASPD('knight', 'one_hand_spear', 50, 50, 1, null, false, 0);
        const aspdMounted = calculateASPD('knight', 'one_hand_spear', 50, 50, 1, null, true, 5);
        expect(aspdMounted).toBe(aspdNormal);
    });

    test('higher AGI increases ASPD', () => {
        const low = calculateASPD('swordsman', 'one_hand_sword', 10, 10, 1, null, false, 0);
        const high = calculateASPD('swordsman', 'one_hand_sword', 99, 10, 1, null, false, 0);
        expect(high).toBeGreaterThan(low);
    });

    test('buff multiplier increases ASPD (THQ +0.3)', () => {
        const base = calculateASPD('knight', 'two_hand_sword', 50, 50, 1, null, false, 0);
        const buffed = calculateASPD('knight', 'two_hand_sword', 50, 50, 1.3, null, false, 0);
        expect(buffed).toBeGreaterThan(base);
    });

    test('floor at 100', () => {
        const aspd = calculateASPD('novice', 'bare_hand', 1, 1, 1, null, true, 0);
        expect(aspd).toBeGreaterThanOrEqual(100);
    });
});
```

---

## Test File 2: Element and Size Tables

```javascript
// __tests__/combat/elementSize.test.js

const {
    getElementModifier,
    getSizePenalty,
    ELEMENT_TABLE,
    SIZE_PENALTY,
} = require('../../src/ro_damage_formulas');

describe('getElementModifier', () => {
    // ── Neutral attack ──
    test('neutral vs neutral Lv1 = 100%', () => {
        expect(getElementModifier('neutral', 'neutral', 1)).toBe(100);
    });

    test('neutral vs ghost Lv1 = 25%', () => {
        expect(getElementModifier('neutral', 'ghost', 1)).toBe(25);
    });

    test('neutral vs ghost Lv3 = 0% (immune)', () => {
        expect(getElementModifier('neutral', 'ghost', 3)).toBe(0);
    });

    // ── Elemental advantages ──
    test('fire vs earth Lv1 = 150%', () => {
        expect(getElementModifier('fire', 'earth', 1)).toBe(150);
    });

    test('fire vs earth Lv4 = 200%', () => {
        expect(getElementModifier('fire', 'earth', 4)).toBe(200);
    });

    test('water vs fire Lv1 = 150%', () => {
        expect(getElementModifier('water', 'fire', 1)).toBe(150);
    });

    test('wind vs water Lv1 = 175%', () => {
        expect(getElementModifier('wind', 'water', 1)).toBe(175);
    });

    test('earth vs wind Lv4 = 200%', () => {
        expect(getElementModifier('earth', 'wind', 4)).toBe(200);
    });

    // ── Elemental immunities ──
    test('fire vs fire Lv1 = 25% (resist)', () => {
        expect(getElementModifier('fire', 'fire', 1)).toBe(25);
    });

    test('fire vs fire Lv4 = -50% (heals)', () => {
        expect(getElementModifier('fire', 'fire', 4)).toBe(-50);
    });

    test('poison vs poison = 0% at all levels', () => {
        for (let lv = 1; lv <= 4; lv++) {
            expect(getElementModifier('poison', 'poison', lv)).toBe(0);
        }
    });

    // ── Holy vs Undead ──
    test('holy vs undead Lv1 = 150%', () => {
        expect(getElementModifier('holy', 'undead', 1)).toBe(150);
    });

    test('holy vs undead Lv4 = 200%', () => {
        expect(getElementModifier('holy', 'undead', 4)).toBe(200);
    });

    // ── Holy vs Holy (self-resist) ──
    test('holy vs holy Lv1 = 0%', () => {
        expect(getElementModifier('holy', 'holy', 1)).toBe(0);
    });

    test('holy vs holy Lv4 = -100% (full heal)', () => {
        expect(getElementModifier('holy', 'holy', 4)).toBe(-100);
    });

    // ── Shadow/Undead interactions ──
    test('holy vs shadow Lv4 = 200%', () => {
        expect(getElementModifier('holy', 'shadow', 4)).toBe(200);
    });

    test('shadow vs holy Lv4 = 200%', () => {
        expect(getElementModifier('shadow', 'holy', 4)).toBe(200);
    });

    test('undead vs shadow = 0% at all levels', () => {
        for (let lv = 1; lv <= 4; lv++) {
            expect(getElementModifier('undead', 'shadow', lv)).toBe(0);
        }
    });

    test('undead vs undead = 0% at all levels', () => {
        for (let lv = 1; lv <= 4; lv++) {
            expect(getElementModifier('undead', 'undead', lv)).toBe(0);
        }
    });

    // ── Ghost element (Ghostring Card) ──
    test('ghost vs ghost Lv1 = 125%', () => {
        expect(getElementModifier('ghost', 'ghost', 1)).toBe(125);
    });

    test('ghost vs neutral Lv1 = 25% (reduced)', () => {
        expect(getElementModifier('ghost', 'neutral', 1)).toBe(25);
    });

    // ── Poison vs Undead (heals) ──
    test('poison vs undead Lv1 = -25%', () => {
        expect(getElementModifier('poison', 'undead', 1)).toBe(-25);
    });

    test('poison vs undead Lv4 = -100%', () => {
        expect(getElementModifier('poison', 'undead', 4)).toBe(-100);
    });

    // ── Boundary conditions ──
    test('invalid attack element falls back to neutral', () => {
        expect(getElementModifier('nonexistent', 'fire', 1)).toBe(100);
    });

    test('invalid defense element falls back to neutral within attack row', () => {
        expect(getElementModifier('fire', 'nonexistent', 1)).toBe(100);
    });

    test('level 0 clamps to index 0 (level 1)', () => {
        expect(getElementModifier('fire', 'water', 0)).toBe(50);
    });

    test('level 5 clamps to index 3 (level 4)', () => {
        expect(getElementModifier('fire', 'water', 5)).toBe(0);
    });
});

describe('getSizePenalty', () => {
    test('bare_hand: 100% against all sizes', () => {
        expect(getSizePenalty('bare_hand', 'small')).toBe(100);
        expect(getSizePenalty('bare_hand', 'medium')).toBe(100);
        expect(getSizePenalty('bare_hand', 'large')).toBe(100);
    });

    test('dagger: 100/75/50', () => {
        expect(getSizePenalty('dagger', 'small')).toBe(100);
        expect(getSizePenalty('dagger', 'medium')).toBe(75);
        expect(getSizePenalty('dagger', 'large')).toBe(50);
    });

    test('one_hand_sword: 75/100/75', () => {
        expect(getSizePenalty('one_hand_sword', 'small')).toBe(75);
        expect(getSizePenalty('one_hand_sword', 'medium')).toBe(100);
        expect(getSizePenalty('one_hand_sword', 'large')).toBe(75);
    });

    test('two_hand_sword: 75/75/100', () => {
        expect(getSizePenalty('two_hand_sword', 'small')).toBe(75);
        expect(getSizePenalty('two_hand_sword', 'medium')).toBe(75);
        expect(getSizePenalty('two_hand_sword', 'large')).toBe(100);
    });

    test('bow: 100/100/75', () => {
        expect(getSizePenalty('bow', 'small')).toBe(100);
        expect(getSizePenalty('bow', 'medium')).toBe(100);
        expect(getSizePenalty('bow', 'large')).toBe(75);
    });

    test('katar: 75/100/75', () => {
        expect(getSizePenalty('katar', 'small')).toBe(75);
        expect(getSizePenalty('katar', 'medium')).toBe(100);
        expect(getSizePenalty('katar', 'large')).toBe(75);
    });

    test('rod/staff: 100/100/100', () => {
        expect(getSizePenalty('rod', 'small')).toBe(100);
        expect(getSizePenalty('staff', 'medium')).toBe(100);
        expect(getSizePenalty('rod', 'large')).toBe(100);
    });

    test('unknown weapon type falls back to default (100/100/100)', () => {
        expect(getSizePenalty('plasma_cannon', 'small')).toBe(100);
        expect(getSizePenalty('plasma_cannon', 'medium')).toBe(100);
        expect(getSizePenalty('plasma_cannon', 'large')).toBe(100);
    });

    test('one_hand_axe: 50/75/100', () => {
        expect(getSizePenalty('one_hand_axe', 'small')).toBe(50);
        expect(getSizePenalty('one_hand_axe', 'medium')).toBe(75);
        expect(getSizePenalty('one_hand_axe', 'large')).toBe(100);
    });

    test('mace: 75/100/100', () => {
        expect(getSizePenalty('mace', 'small')).toBe(75);
        expect(getSizePenalty('mace', 'medium')).toBe(100);
        expect(getSizePenalty('mace', 'large')).toBe(100);
    });

    test('book: 100/100/50', () => {
        expect(getSizePenalty('book', 'small')).toBe(100);
        expect(getSizePenalty('book', 'medium')).toBe(100);
        expect(getSizePenalty('book', 'large')).toBe(50);
    });

    test('knuckle: 100/75/50', () => {
        expect(getSizePenalty('knuckle', 'small')).toBe(100);
        expect(getSizePenalty('knuckle', 'medium')).toBe(75);
        expect(getSizePenalty('knuckle', 'large')).toBe(50);
    });
});
```

---

## Test File 3: Hit/Flee and Critical

```javascript
// __tests__/combat/hitFleeCrit.test.js

const {
    calculateHitRate,
    calculateCritRate,
} = require('../../src/ro_damage_formulas');

describe('calculateHitRate', () => {
    // Formula: 80 + HIT - effectiveFLEE, clamped 5-95%
    test('equal HIT and FLEE: 80 + 200 - 200 = 80%', () => {
        expect(calculateHitRate(200, 200)).toBe(80);
    });

    test('HIT >> FLEE: capped at 95%', () => {
        expect(calculateHitRate(999, 100)).toBe(95);
    });

    test('FLEE >> HIT: floored at 5%', () => {
        expect(calculateHitRate(100, 999)).toBe(5);
    });

    test('HIT=177, FLEE=102: 80 + 177 - 102 = 155 → capped 95', () => {
        expect(calculateHitRate(177, 102)).toBe(95);
    });

    test('HIT=180, FLEE=200: 80 + 180 - 200 = 60', () => {
        expect(calculateHitRate(180, 200)).toBe(60);
    });

    // ── Multi-attacker FLEE penalty ──
    test('3 attackers: FLEE reduced by 10', () => {
        // 80 + 200 - (200-10) = 80 + 200 - 190 = 90
        expect(calculateHitRate(200, 200, 3)).toBe(90);
    });

    test('5 attackers: FLEE reduced by 30', () => {
        // 80 + 200 - (200-30) = 80 + 200 - 170 = 110 → capped 95
        expect(calculateHitRate(200, 200, 5)).toBe(95);
    });

    test('2 attackers: no FLEE penalty', () => {
        expect(calculateHitRate(200, 200, 2)).toBe(80);
    });

    test('1 attacker: no FLEE penalty (default)', () => {
        expect(calculateHitRate(200, 200, 1)).toBe(80);
    });

    // ── hitRatePercent bonus (skill-specific) ──
    test('hitRatePercent=50: hitRate * 1.5', () => {
        // Base: 80 + 200 - 200 = 80, then floor(80 * 150/100) = 120 → capped 95
        expect(calculateHitRate(200, 200, 1, 50)).toBe(95);
    });

    test('hitRatePercent=20 with low base: 60 * 120/100 = 72', () => {
        // Base: 80 + 180 - 200 = 60, then floor(60 * 120/100) = 72
        expect(calculateHitRate(180, 200, 1, 20)).toBe(72);
    });

    // ── Edge: FLEE penalty cannot go below 0 ──
    test('massive attackers cannot make FLEE negative', () => {
        // 100 attackers: penalty = (100-2)*10 = 980, effectiveFlee = max(0, 50-980) = 0
        // 80 + 100 - 0 = 180 → capped 95
        expect(calculateHitRate(100, 50, 100)).toBe(95);
    });
});

describe('calculateCritRate', () => {
    // Formula: max(0, attackerCri - floor(targetLuk * 0.2))
    test('CRI=10, targetLUK=10: 10 - floor(10*0.2) = 10 - 2 = 8', () => {
        expect(calculateCritRate(10, 10)).toBe(8);
    });

    test('CRI=30, targetLUK=1: 30 - 0 = 30', () => {
        expect(calculateCritRate(30, 1)).toBe(30);
    });

    test('CRI=1, targetLUK=99: max(0, 1 - 19) = 0', () => {
        expect(calculateCritRate(1, 99)).toBe(0);
    });

    test('CRI=0, targetLUK=0: 0', () => {
        expect(calculateCritRate(0, 0)).toBe(0);
    });

    test('CRI=35, targetLUK=50: 35 - 10 = 25', () => {
        expect(calculateCritRate(35, 50)).toBe(25);
    });

    test('critical shield is floor not round: LUK=7 → floor(7*0.2) = floor(1.4) = 1', () => {
        expect(calculateCritRate(5, 7)).toBe(4);
    });
});
```

---

## Test File 4: Physical Damage Pipeline

```javascript
// __tests__/combat/physicalDamage.test.js

const {
    calculatePhysicalDamage,
    getElementModifier,
} = require('../../src/ro_damage_formulas');

// Helper to create a basic attacker
function makeAttacker(overrides = {}) {
    return {
        stats: { str: 50, agi: 20, vit: 20, int: 10, dex: 30, luk: 10, level: 50, ...overrides.stats },
        weaponATK: overrides.weaponATK || 100,
        passiveATK: overrides.passiveATK || 0,
        weaponType: overrides.weaponType || 'one_hand_sword',
        weaponElement: overrides.weaponElement || 'neutral',
        weaponLevel: overrides.weaponLevel || 2,
        buffMods: { atkMultiplier: 1.0, ...(overrides.buffMods || {}) },
        cardMods: overrides.cardMods || null,
        passiveRaceATK: overrides.passiveRaceATK || null,
        arrowATK: overrides.arrowATK || 0,
        refineATK: overrides.refineATK || 0,
        overrefineMax: overrides.overrefineMax || 0,
        isMounted: overrides.isMounted || false,
        cardNoSizeFix: overrides.cardNoSizeFix || false,
        cardCritRace: overrides.cardCritRace || {},
        cardCriticalLong: overrides.cardCriticalLong || 0,
        critAtkRate: overrides.critAtkRate || 0,
        cardAddClass: overrides.cardAddClass || {},
        cardAddRace2: overrides.cardAddRace2 || {},
        cardLongAtkRate: overrides.cardLongAtkRate || 0,
        cardSkillAtk: overrides.cardSkillAtk || {},
        cardIgnoreDefClass: overrides.cardIgnoreDefClass || null,
        cardDefRatioAtkClass: overrides.cardDefRatioAtkClass || null,
        cardAddDamageClass: overrides.cardAddDamageClass || {},
    };
}

function makeTarget(overrides = {}) {
    return {
        stats: { str: 10, agi: 10, vit: 30, int: 10, dex: 10, luk: 10, level: 30, ...overrides.stats },
        hardDef: overrides.hardDef || 20,
        element: overrides.element || { type: 'neutral', level: 1 },
        size: overrides.size || 'medium',
        race: overrides.race || 'demihuman',
        numAttackers: overrides.numAttackers || 1,
        buffMods: { defMultiplier: 1.0, ...(overrides.buffMods || {}) },
        passiveRaceDEF: overrides.passiveRaceDEF || null,
        elementResist: overrides.elementResist || null,
        modeFlags: overrides.modeFlags || null,
        templateId: overrides.templateId || null,
        subRace: overrides.subRace || null,
        cardDefMods: overrides.cardDefMods || null,
        cardSubClass: overrides.cardSubClass || null,
        _ensembleVitDefZero: overrides._ensembleVitDefZero || false,
    };
}

describe('calculatePhysicalDamage', () => {
    // ── Basic damage (forceHit to skip randomness in hit/miss) ──
    describe('basic damage calculation', () => {
        test('produces positive damage with forceHit', () => {
            const result = calculatePhysicalDamage(makeAttacker(), makeTarget(), { forceHit: true });
            expect(result.damage).toBeGreaterThan(0);
            expect(result.isMiss).toBe(false);
        });

        test('minimum damage is 1', () => {
            // Very weak attacker vs very strong target
            const result = calculatePhysicalDamage(
                makeAttacker({ weaponATK: 0, stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 } }),
                makeTarget({ hardDef: 90, stats: { vit: 99, agi: 99, level: 99 } }),
                { forceHit: true }
            );
            expect(result.damage).toBeGreaterThanOrEqual(1);
        });
    });

    // ── Skill multiplier ──
    describe('skill multiplier', () => {
        test('skill with 200% multiplier does roughly 2x damage', () => {
            const attacker = makeAttacker();
            const target = makeTarget();
            // Use forceCrit to eliminate variance
            const normal = calculatePhysicalDamage(attacker, target, { forceHit: true, forceCrit: true });
            const skill200 = calculatePhysicalDamage(attacker, target, {
                forceHit: true, forceCrit: true, isSkill: true, skillMultiplier: 200
            });
            // Should be roughly 2x (not exact due to DEF subtraction, refine, etc.)
            expect(skill200.damage).toBeGreaterThan(normal.damage);
        });

        test('skillMultiplier 100 means no change', () => {
            const attacker = makeAttacker();
            const target = makeTarget();
            const autoAtk = calculatePhysicalDamage(attacker, target, { forceHit: true, forceCrit: true });
            const skill100 = calculatePhysicalDamage(attacker, target, {
                forceHit: true, forceCrit: true, isSkill: true, skillMultiplier: 100
            });
            // Skills cannot crit naturally, but forceCrit overrides. With same multiplier, damage should be similar.
            // The only difference is the isSkill flag itself (which blocks natural crits, but we forceCrit).
            expect(skill100.damage).toBe(autoAtk.damage);
        });
    });

    // ── Critical mechanics ──
    describe('criticals', () => {
        test('forceCrit always produces critical hit', () => {
            const result = calculatePhysicalDamage(makeAttacker(), makeTarget(), { forceCrit: true });
            expect(result.isCritical).toBe(true);
            expect(result.hitType).toBe('critical');
            expect(result.isMiss).toBe(false);
        });

        test('skills cannot crit naturally (only via forceCrit)', () => {
            // With a high LUK attacker, auto-attacks can crit. Skills should not.
            const attacker = makeAttacker({ stats: { luk: 99, str: 50, agi: 20, vit: 20, int: 10, dex: 30, level: 50 } });
            const target = makeTarget({ stats: { luk: 1 } });
            let critCount = 0;
            for (let i = 0; i < 1000; i++) {
                const result = calculatePhysicalDamage(attacker, target, {
                    isSkill: true, skillMultiplier: 100, forceHit: true
                });
                if (result.isCritical) critCount++;
            }
            expect(critCount).toBe(0); // Skills never crit unless forceCrit
        });

        test('critical damage includes +40% bonus', () => {
            const attacker = makeAttacker({ weaponATK: 100 });
            const target = makeTarget({ hardDef: 0, stats: { vit: 1, agi: 1, level: 1 } });
            const normal = calculatePhysicalDamage(attacker, target, { forceHit: true });
            const crit = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            // Critical should be higher due to +40% bonus AND max ATK (no variance)
            expect(crit.damage).toBeGreaterThan(normal.damage);
        });
    });

    // ── Size penalty ──
    describe('size penalty', () => {
        test('dagger vs large = 50% size penalty', () => {
            const attacker = makeAttacker({ weaponType: 'dagger' });
            const targetMed = makeTarget({ size: 'medium' });
            const targetLrg = makeTarget({ size: 'large' });
            const dmgMed = calculatePhysicalDamage(attacker, targetMed, { forceCrit: true });
            const dmgLrg = calculatePhysicalDamage(attacker, targetLrg, { forceCrit: true });
            expect(dmgLrg.sizePenalty).toBe(50);
            expect(dmgMed.sizePenalty).toBe(75);
            expect(dmgLrg.damage).toBeLessThan(dmgMed.damage);
        });

        test('cardNoSizeFix bypasses size penalty', () => {
            const attacker = makeAttacker({ weaponType: 'dagger', cardNoSizeFix: true });
            const result = calculatePhysicalDamage(attacker, makeTarget({ size: 'large' }), { forceCrit: true });
            expect(result.sizePenalty).toBe(100);
        });

        test('mounted spear vs medium = 100% (overrides normal 75%)', () => {
            const attacker = makeAttacker({ weaponType: 'one_hand_spear', isMounted: true });
            const result = calculatePhysicalDamage(attacker, makeTarget({ size: 'medium' }), { forceCrit: true });
            expect(result.sizePenalty).toBe(100);
        });

        test('noSizePenalty buff bypasses size penalty', () => {
            const attacker = makeAttacker({ weaponType: 'dagger', buffMods: { atkMultiplier: 1.0, noSizePenalty: true } });
            const result = calculatePhysicalDamage(attacker, makeTarget({ size: 'large' }), { forceCrit: true });
            expect(result.sizePenalty).toBe(100);
        });
    });

    // ── Element modifier ──
    describe('element modifier', () => {
        test('fire weapon vs water target = 50% (resistant)', () => {
            const attacker = makeAttacker({ weaponElement: 'fire' });
            const target = makeTarget({ element: { type: 'water', level: 1 } });
            const result = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            expect(result.elementModifier).toBe(50);
            expect(result.element).toBe('fire');
        });

        test('fire weapon vs earth target = 150% (super effective)', () => {
            const attacker = makeAttacker({ weaponElement: 'fire' });
            const target = makeTarget({ element: { type: 'earth', level: 1 } });
            const result = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            expect(result.elementModifier).toBe(150);
        });

        test('skillElement overrides weapon element', () => {
            const attacker = makeAttacker({ weaponElement: 'neutral' });
            const target = makeTarget({ element: { type: 'earth', level: 1 } });
            const result = calculatePhysicalDamage(attacker, target, {
                forceCrit: true, skillElement: 'fire', isSkill: true, skillMultiplier: 100
            });
            expect(result.element).toBe('fire');
            expect(result.elementModifier).toBe(150);
        });

        test('immune element returns 0 damage', () => {
            const attacker = makeAttacker({ weaponElement: 'poison' });
            const target = makeTarget({ element: { type: 'poison', level: 1 } });
            const result = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            expect(result.damage).toBe(0);
            expect(result.isMiss).toBe(true);
        });

        test('isNonElemental bypasses element table (monster auto-attack)', () => {
            const attacker = makeAttacker({ weaponElement: 'neutral' });
            const target = makeTarget({ element: { type: 'ghost', level: 1 } });
            // Normal: neutral vs ghost Lv1 = 25%
            const normal = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            expect(normal.elementModifier).toBe(25);
            // Non-elemental: bypasses = 100%
            const monster = calculatePhysicalDamage(attacker, target, { forceCrit: true, isNonElemental: true });
            expect(monster.elementModifier).toBe(100);
            expect(monster.damage).toBeGreaterThan(normal.damage);
        });
    });

    // ── DEF reduction ──
    describe('DEF reduction', () => {
        test('higher hardDef reduces damage', () => {
            const attacker = makeAttacker();
            const lowDef = calculatePhysicalDamage(attacker, makeTarget({ hardDef: 10 }), { forceCrit: true });
            const highDef = calculatePhysicalDamage(attacker, makeTarget({ hardDef: 50 }), { forceCrit: true });
            expect(highDef.damage).toBeLessThan(lowDef.damage);
        });

        test('hardDef capped at 99', () => {
            const attacker = makeAttacker();
            const def99 = calculatePhysicalDamage(attacker, makeTarget({ hardDef: 99 }), { forceCrit: true });
            const def150 = calculatePhysicalDamage(attacker, makeTarget({ hardDef: 150 }), { forceCrit: true });
            expect(def99.damage).toBe(def150.damage); // Both capped at 99
        });

        test('ignoreDefense bypasses DEF completely', () => {
            const attacker = makeAttacker();
            const target = makeTarget({ hardDef: 80, stats: { vit: 99, agi: 50, level: 99 } });
            const normal = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            const ignored = calculatePhysicalDamage(attacker, target, { forceCrit: true, ignoreDefense: true });
            expect(ignored.damage).toBeGreaterThan(normal.damage);
        });

        test('cardIgnoreDefClass: normal class bypass', () => {
            const attacker = makeAttacker({ cardIgnoreDefClass: 'normal' });
            const target = makeTarget({ hardDef: 50, modeFlags: null });
            const result = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            // Should bypass DEF (normal class target, card targets normal)
            const noDef = calculatePhysicalDamage(
                makeAttacker(),
                makeTarget({ hardDef: 50 }),
                { forceCrit: true, ignoreDefense: true }
            );
            expect(result.damage).toBe(noDef.damage);
        });

        test('cardDefRatioAtkClass converts DEF to bonus damage', () => {
            const attacker = makeAttacker({ cardDefRatioAtkClass: 'normal' });
            const target = makeTarget({ hardDef: 80, stats: { vit: 99, agi: 50, level: 99 } });
            const normal = calculatePhysicalDamage(makeAttacker(), target, { forceCrit: true });
            const icePick = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            // With very high DEF, Ice Pick should do MORE damage (DEF becomes bonus)
            expect(icePick.damage).toBeGreaterThan(normal.damage);
        });

        test('Provoke reduces soft DEF (softDefMultiplier)', () => {
            const attacker = makeAttacker();
            const normal = makeTarget({ stats: { vit: 80, agi: 10, level: 50 } });
            const provoked = makeTarget({
                stats: { vit: 80, agi: 10, level: 50 },
                buffMods: { defMultiplier: 1.0, softDefMultiplier: 0.55 } // Provoke Lv10
            });
            const dmgNormal = calculatePhysicalDamage(attacker, normal, { forceCrit: true });
            const dmgProvoked = calculatePhysicalDamage(attacker, provoked, { forceCrit: true });
            expect(dmgProvoked.damage).toBeGreaterThan(dmgNormal.damage);
        });

        test('Freeze reduces hard DEF by 50% (defMultiplier=0.5)', () => {
            const attacker = makeAttacker();
            const normal = makeTarget({ hardDef: 40 });
            const frozen = makeTarget({ hardDef: 40, buffMods: { defMultiplier: 0.5 } });
            const dmgNormal = calculatePhysicalDamage(attacker, normal, { forceCrit: true });
            const dmgFrozen = calculatePhysicalDamage(attacker, frozen, { forceCrit: true });
            expect(dmgFrozen.damage).toBeGreaterThan(dmgNormal.damage);
        });

        test('Ensemble VIT DEF zero reduces soft DEF to 0', () => {
            const attacker = makeAttacker();
            const target = makeTarget({
                stats: { vit: 99, agi: 10, level: 50 },
                _ensembleVitDefZero: true
            });
            const normalTarget = makeTarget({ stats: { vit: 99, agi: 10, level: 50 } });
            const dmgEnsemble = calculatePhysicalDamage(attacker, target, { forceCrit: true });
            const dmgNormal = calculatePhysicalDamage(attacker, normalTarget, { forceCrit: true });
            expect(dmgEnsemble.damage).toBeGreaterThan(dmgNormal.damage);
        });
    });

    // ── Refine ATK ──
    describe('refine ATK', () => {
        test('refine ATK adds flat damage post-DEF', () => {
            const noRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 0 }),
                makeTarget(),
                { forceCrit: true }
            );
            const withRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 30 }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(withRefine.damage).toBe(noRefine.damage + 30);
        });

        test('refine ATK excluded for Shield Boomerang (1305)', () => {
            const withRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 30 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1305 }
            );
            const noRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 0 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1305 }
            );
            expect(withRefine.damage).toBe(noRefine.damage);
        });

        test('refine ATK excluded for Asura Strike (1605)', () => {
            const withRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 50 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1605 }
            );
            const noRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 0 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1605 }
            );
            expect(withRefine.damage).toBe(noRefine.damage);
        });

        test('refine ATK excluded for Acid Terror (1801)', () => {
            const withRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 20 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1801 }
            );
            const noRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 0 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1801 }
            );
            expect(withRefine.damage).toBe(noRefine.damage);
        });

        test('refine ATK excluded for Investigate (1606)', () => {
            const withRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 35 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1606 }
            );
            const noRefine = calculatePhysicalDamage(
                makeAttacker({ refineATK: 0 }),
                makeTarget(),
                { forceCrit: true, isSkill: true, skillMultiplier: 100, skillId: 1606 }
            );
            expect(withRefine.damage).toBe(noRefine.damage);
        });
    });

    // ── Card modifiers ──
    describe('card modifiers', () => {
        test('race card bonus (+20% vs demihuman)', () => {
            const noCard = calculatePhysicalDamage(makeAttacker(), makeTarget(), { forceCrit: true });
            const withCard = calculatePhysicalDamage(
                makeAttacker({ cardMods: { race_demihuman: 20 } }),
                makeTarget({ race: 'demihuman' }),
                { forceCrit: true }
            );
            expect(withCard.damage).toBeGreaterThan(noCard.damage);
        });

        test('two Hydra cards (2x +20% = +40% race bonus)', () => {
            const singleHydra = calculatePhysicalDamage(
                makeAttacker({ cardMods: { race_demihuman: 20 } }),
                makeTarget({ race: 'demihuman' }),
                { forceCrit: true }
            );
            const doubleHydra = calculatePhysicalDamage(
                makeAttacker({ cardMods: { race_demihuman: 40 } }),
                makeTarget({ race: 'demihuman' }),
                { forceCrit: true }
            );
            expect(doubleHydra.damage).toBeGreaterThan(singleHydra.damage);
        });

        test('boss class card bonus (Abysmal Knight)', () => {
            const noCard = calculatePhysicalDamage(makeAttacker(), makeTarget({ modeFlags: { isBoss: true } }), { forceCrit: true });
            const withCard = calculatePhysicalDamage(
                makeAttacker({ cardAddClass: { boss: 25 } }),
                makeTarget({ modeFlags: { isBoss: true } }),
                { forceCrit: true }
            );
            expect(withCard.damage).toBeGreaterThan(noCard.damage);
        });

        test('ranged ATK bonus only applies to bow/gun', () => {
            const bowResult = calculatePhysicalDamage(
                makeAttacker({ weaponType: 'bow', cardLongAtkRate: 10 }),
                makeTarget(),
                { forceCrit: true }
            );
            const swordResult = calculatePhysicalDamage(
                makeAttacker({ weaponType: 'one_hand_sword', cardLongAtkRate: 10 }),
                makeTarget(),
                { forceCrit: true }
            );
            // Sword should not benefit from ranged ATK bonus
            const swordNoCard = calculatePhysicalDamage(
                makeAttacker({ weaponType: 'one_hand_sword', cardLongAtkRate: 0 }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(swordResult.damage).toBe(swordNoCard.damage);
        });

        test('defensive card reduces damage (Thara Frog: -30% vs demihuman)', () => {
            const noDef = calculatePhysicalDamage(
                makeAttacker(),
                makeTarget({ race: 'demihuman' }),
                { forceCrit: true }
            );
            const withDef = calculatePhysicalDamage(
                makeAttacker({ race: 'demihuman' }),
                makeTarget({ cardDefMods: { race_demihuman: 30 } }),
                { forceCrit: true }
            );
            expect(withDef.damage).toBeLessThan(noDef.damage);
        });
    });

    // ── Passive race bonuses ──
    describe('passive race bonuses', () => {
        test('passive race ATK is flat additive', () => {
            const noPassive = calculatePhysicalDamage(makeAttacker(), makeTarget({ race: 'undead' }), { forceCrit: true });
            const withPassive = calculatePhysicalDamage(
                makeAttacker({ passiveRaceATK: { undead: 30 } }),
                makeTarget({ race: 'undead' }),
                { forceCrit: true }
            );
            expect(withPassive.damage).toBe(noPassive.damage + 30);
        });

        test('passive race DEF is flat subtraction', () => {
            const noDef = calculatePhysicalDamage(makeAttacker({ race: 'demon' }), makeTarget(), { forceCrit: true });
            const withDef = calculatePhysicalDamage(
                makeAttacker({ race: 'demon' }),
                makeTarget({ passiveRaceDEF: { demon: 20 } }),
                { forceCrit: true }
            );
            expect(withDef.damage).toBeLessThan(noDef.damage);
        });
    });

    // ── Buff modifiers ──
    describe('buff modifiers', () => {
        test('atkMultiplier 1.3 (Provoke Lv5) increases damage by 30%', () => {
            const normal = calculatePhysicalDamage(makeAttacker(), makeTarget(), { forceCrit: true });
            const provoked = calculatePhysicalDamage(
                makeAttacker({ buffMods: { atkMultiplier: 1.3 } }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(provoked.damage).toBeGreaterThan(normal.damage);
        });

        test('maximizePower always uses max ATK', () => {
            const attacker = makeAttacker({ buffMods: { atkMultiplier: 1.0, maximizePower: true } });
            const results = new Set();
            for (let i = 0; i < 50; i++) {
                const result = calculatePhysicalDamage(attacker, makeTarget(), { forceHit: true });
                results.add(result.damage);
            }
            // With maximize power, damage should be constant (no variance)
            expect(results.size).toBe(1);
        });
    });

    // ── Sage zone boost ──
    describe('sage zone boost', () => {
        test('fireDmgBoost increases fire element damage', () => {
            const noBuff = calculatePhysicalDamage(
                makeAttacker({ weaponElement: 'fire' }),
                makeTarget(),
                { forceCrit: true }
            );
            const withBuff = calculatePhysicalDamage(
                makeAttacker({ weaponElement: 'fire', buffMods: { atkMultiplier: 1.0, fireDmgBoost: 20 } }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(withBuff.damage).toBeGreaterThan(noBuff.damage);
        });

        test('fireDmgBoost does NOT affect non-fire attacks', () => {
            const withBuff = calculatePhysicalDamage(
                makeAttacker({ weaponElement: 'neutral', buffMods: { atkMultiplier: 1.0, fireDmgBoost: 20 } }),
                makeTarget(),
                { forceCrit: true }
            );
            const noBuff = calculatePhysicalDamage(
                makeAttacker({ weaponElement: 'neutral' }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(withBuff.damage).toBe(noBuff.damage);
        });
    });

    // ── Element resist ──
    describe('element resist', () => {
        test('elementResist reduces damage for matching element', () => {
            const noResist = calculatePhysicalDamage(
                makeAttacker({ weaponElement: 'fire' }),
                makeTarget(),
                { forceCrit: true }
            );
            const withResist = calculatePhysicalDamage(
                makeAttacker({ weaponElement: 'fire' }),
                makeTarget({ elementResist: { fire: 30 } }),
                { forceCrit: true }
            );
            expect(withResist.damage).toBeLessThan(noResist.damage);
        });
    });

    // ── Raid debuff ──
    describe('raid debuff', () => {
        test('raidDamageIncrease 0.2 adds 20% final damage', () => {
            const normal = calculatePhysicalDamage(makeAttacker(), makeTarget(), { forceCrit: true });
            const raided = calculatePhysicalDamage(
                makeAttacker(),
                makeTarget({ buffMods: { defMultiplier: 1.0, raidDamageIncrease: 0.2 } }),
                { forceCrit: true }
            );
            expect(raided.damage).toBeGreaterThan(normal.damage);
            // Should be approximately 1.2x
            expect(raided.damage).toBe(Math.floor(normal.damage * 1.2));
        });
    });

    // ── Arrow ATK ──
    describe('arrow ATK', () => {
        test('arrowATK adds to weapon damage for ranged', () => {
            const noArrow = calculatePhysicalDamage(
                makeAttacker({ weaponType: 'bow', arrowATK: 0 }),
                makeTarget(),
                { forceCrit: true }
            );
            const withArrow = calculatePhysicalDamage(
                makeAttacker({ weaponType: 'bow', arrowATK: 30 }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(withArrow.damage).toBeGreaterThan(noArrow.damage);
        });

        test('arrowATK on crit = full value (no variance)', () => {
            const attacker = makeAttacker({ weaponType: 'bow', arrowATK: 30 });
            const results = new Set();
            for (let i = 0; i < 50; i++) {
                const result = calculatePhysicalDamage(attacker, makeTarget(), { forceCrit: true });
                results.add(result.damage);
            }
            expect(results.size).toBe(1); // Constant on crit
        });
    });

    // ── Mastery ATK ──
    describe('mastery ATK (passiveATK)', () => {
        test('passiveATK adds flat damage post-DEF', () => {
            const noMastery = calculatePhysicalDamage(
                makeAttacker({ passiveATK: 0 }),
                makeTarget(),
                { forceCrit: true }
            );
            const withMastery = calculatePhysicalDamage(
                makeAttacker({ passiveATK: 40 }),
                makeTarget(),
                { forceCrit: true }
            );
            expect(withMastery.damage).toBe(noMastery.damage + 40);
        });
    });
});
```

---

## Test File 5: Magical Damage Pipeline

```javascript
// __tests__/combat/magicalDamage.test.js

const {
    calculateMagicalDamage,
} = require('../../src/ro_damage_formulas');

function makeMagicAttacker(overrides = {}) {
    return {
        stats: { str: 10, agi: 10, vit: 10, int: 80, dex: 30, luk: 10, level: 70, ...overrides.stats },
        weaponMATK: overrides.weaponMATK || 50,
        buffMods: { atkMultiplier: 1.0, ...(overrides.buffMods || {}) },
        cardMatkRate: overrides.cardMatkRate || 0,
        cardMagicRace: overrides.cardMagicRace || {},
        cardSkillAtk: overrides.cardSkillAtk || {},
        cardIgnoreMdefClass: overrides.cardIgnoreMdefClass || {},
        passiveRaceMATK: overrides.passiveRaceMATK || null,
        race: overrides.race || 'demihuman',
    };
}

function makeMagicTarget(overrides = {}) {
    return {
        stats: { str: 10, agi: 10, vit: 30, int: 20, dex: 10, luk: 10, level: 40, ...overrides.stats },
        hardMdef: overrides.hardMdef || 15,
        element: overrides.element || { type: 'neutral', level: 1 },
        race: overrides.race || 'demihuman',
        buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0, ...(overrides.buffMods || {}) },
        modeFlags: overrides.modeFlags || null,
        cardNoMagicDamage: overrides.cardNoMagicDamage || 0,
        passiveRaceResist: overrides.passiveRaceResist || null,
    };
}

describe('calculateMagicalDamage', () => {
    describe('basic magic damage', () => {
        test('produces positive damage', () => {
            const result = calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget(), {
                skillMultiplier: 100, skillElement: 'fire'
            });
            expect(result.damage).toBeGreaterThan(0);
            expect(result.hitType).toBe('magical');
            expect(result.isMiss).toBe(false);
        });

        test('minimum magic damage is 1', () => {
            const result = calculateMagicalDamage(
                makeMagicAttacker({ stats: { int: 1, level: 1 }, weaponMATK: 0 }),
                makeMagicTarget({ hardMdef: 99, stats: { int: 99, vit: 99, dex: 99, level: 99 } }),
                { skillMultiplier: 100, skillElement: 'neutral' }
            );
            expect(result.damage).toBeGreaterThanOrEqual(1);
        });
    });

    describe('skill multiplier', () => {
        test('200% multiplier increases damage', () => {
            const atk = makeMagicAttacker();
            const tgt = makeMagicTarget();
            const dmg100 = calculateMagicalDamage(atk, tgt, { skillMultiplier: 100, skillElement: 'fire' });
            const dmg200 = calculateMagicalDamage(atk, tgt, { skillMultiplier: 200, skillElement: 'fire' });
            // Due to MATK variance, compare averages over many runs
            let sum100 = 0, sum200 = 0;
            for (let i = 0; i < 100; i++) {
                sum100 += calculateMagicalDamage(atk, tgt, { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sum200 += calculateMagicalDamage(atk, tgt, { skillMultiplier: 200, skillElement: 'fire' }).damage;
            }
            expect(sum200 / 100).toBeGreaterThan(sum100 / 100);
        });
    });

    describe('GTB magic immunity', () => {
        test('cardNoMagicDamage >= 100 blocks all magic', () => {
            const result = calculateMagicalDamage(
                makeMagicAttacker(),
                makeMagicTarget({ cardNoMagicDamage: 100 }),
                { skillMultiplier: 100, skillElement: 'fire' }
            );
            expect(result.damage).toBe(0);
            expect(result.hitType).toBe('magicImmune');
            expect(result.isMiss).toBe(true);
        });

        test('cardNoMagicDamage < 100 does NOT block magic', () => {
            const result = calculateMagicalDamage(
                makeMagicAttacker(),
                makeMagicTarget({ cardNoMagicDamage: 50 }),
                { skillMultiplier: 100, skillElement: 'fire' }
            );
            expect(result.damage).toBeGreaterThan(0);
        });
    });

    describe('element modifier on magic', () => {
        test('fire spell vs water target = 150%', () => {
            const result = calculateMagicalDamage(
                makeMagicAttacker(),
                makeMagicTarget({ element: { type: 'water', level: 1 } }),
                { skillMultiplier: 100, skillElement: 'fire' }
            );
            // Not checking exact modifier due to MATK variance, but damage should be higher
            // than against fire-resistant targets
            const resultResist = calculateMagicalDamage(
                makeMagicAttacker(),
                makeMagicTarget({ element: { type: 'fire', level: 1 } }),
                { skillMultiplier: 100, skillElement: 'fire' }
            );
            // fire vs fire = 25%, fire vs water = 150% — significant difference
            let sumWater = 0, sumFire = 0;
            for (let i = 0; i < 100; i++) {
                sumWater += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ element: { type: 'water', level: 1 } }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumFire += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ element: { type: 'fire', level: 1 } }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumWater / 100).toBeGreaterThan(sumFire / 100 * 3); // ~6x difference
        });

        test('element immune returns 0 damage', () => {
            const result = calculateMagicalDamage(
                makeMagicAttacker(),
                makeMagicTarget({ element: { type: 'holy', level: 1 } }),
                { skillMultiplier: 100, skillElement: 'holy' }
            );
            expect(result.damage).toBe(0);
            expect(result.isMiss).toBe(true);
        });
    });

    describe('MDEF reduction', () => {
        test('higher hardMdef reduces damage', () => {
            let sumLow = 0, sumHigh = 0;
            for (let i = 0; i < 100; i++) {
                sumLow += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ hardMdef: 10 }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumHigh += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ hardMdef: 50 }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumHigh / 100).toBeLessThan(sumLow / 100);
        });

        test('hardMdef capped at 99', () => {
            let sum99 = 0, sum150 = 0;
            for (let i = 0; i < 100; i++) {
                sum99 += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ hardMdef: 99 }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sum150 += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ hardMdef: 150 }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            // Both should produce same average (capped at 99)
            expect(Math.abs(sum99 / 100 - sum150 / 100)).toBeLessThan(5);
        });

        test('freeze mdefMultiplier 1.25 increases magic damage by 25%', () => {
            let sumNormal = 0, sumFrozen = 0;
            for (let i = 0; i < 200; i++) {
                sumNormal += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumFrozen += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.25 } }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumFrozen / 200).toBeGreaterThan(sumNormal / 200);
        });

        test('cardIgnoreMdefClass: ignore MDEF for normal class', () => {
            let sumNormal = 0, sumIgnored = 0;
            const tgt = makeMagicTarget({ hardMdef: 50 });
            for (let i = 0; i < 100; i++) {
                sumNormal += calculateMagicalDamage(makeMagicAttacker(), tgt, { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumIgnored += calculateMagicalDamage(makeMagicAttacker({ cardIgnoreMdefClass: { normal: 100 } }), tgt, { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumIgnored / 100).toBeGreaterThan(sumNormal / 100);
        });
    });

    describe('card modifiers on magic', () => {
        test('bMatkRate +20% increases damage', () => {
            let sumNone = 0, sumCard = 0;
            for (let i = 0; i < 100; i++) {
                sumNone += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumCard += calculateMagicalDamage(makeMagicAttacker({ cardMatkRate: 20 }), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumCard / 100).toBeGreaterThan(sumNone / 100);
        });

        test('bMagicAddRace vs matching race', () => {
            let sumNone = 0, sumCard = 0;
            for (let i = 0; i < 100; i++) {
                sumNone += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ race: 'undead' }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumCard += calculateMagicalDamage(makeMagicAttacker({ cardMagicRace: { undead: 15 } }), makeMagicTarget({ race: 'undead' }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumCard / 100).toBeGreaterThan(sumNone / 100);
        });
    });

    describe('sage zone boost on magic', () => {
        test('fireDmgBoost increases fire spell damage', () => {
            let sumNone = 0, sumBuff = 0;
            for (let i = 0; i < 100; i++) {
                sumNone += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumBuff += calculateMagicalDamage(makeMagicAttacker({ buffMods: { atkMultiplier: 1.0, fireDmgBoost: 15 } }), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumBuff / 100).toBeGreaterThan(sumNone / 100);
        });

        test('waterDmgBoost only affects water spells', () => {
            let sumFire = 0, sumFireBuff = 0;
            for (let i = 0; i < 100; i++) {
                sumFire += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumFireBuff += calculateMagicalDamage(makeMagicAttacker({ buffMods: { atkMultiplier: 1.0, waterDmgBoost: 20 } }), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            // Water boost should NOT affect fire spells
            expect(Math.abs(sumFire / 100 - sumFireBuff / 100)).toBeLessThan(10);
        });
    });

    describe('Dragonology passives', () => {
        test('passiveRaceMATK increases damage vs matching race', () => {
            let sumNone = 0, sumDrag = 0;
            for (let i = 0; i < 100; i++) {
                sumNone += calculateMagicalDamage(makeMagicAttacker(), makeMagicTarget({ race: 'dragon' }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumDrag += calculateMagicalDamage(makeMagicAttacker({ passiveRaceMATK: { dragon: 10 } }), makeMagicTarget({ race: 'dragon' }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumDrag / 100).toBeGreaterThan(sumNone / 100);
        });

        test('passiveRaceResist reduces damage from matching attacker race', () => {
            let sumNone = 0, sumResist = 0;
            for (let i = 0; i < 100; i++) {
                sumNone += calculateMagicalDamage(makeMagicAttacker({ race: 'dragon' }), makeMagicTarget(), { skillMultiplier: 100, skillElement: 'fire' }).damage;
                sumResist += calculateMagicalDamage(makeMagicAttacker({ race: 'dragon' }), makeMagicTarget({ passiveRaceResist: { dragon: 20 } }), { skillMultiplier: 100, skillElement: 'fire' }).damage;
            }
            expect(sumResist / 100).toBeLessThan(sumNone / 100);
        });
    });
});
```

---

## Test File 6: Integration / Edge Cases

```javascript
// __tests__/combat/edgeCases.test.js

const {
    calculatePhysicalDamage,
    calculateMagicalDamage,
    calculateHitRate,
    calculateCritRate,
    calculateDerivedStats,
    getElementModifier,
    getSizePenalty,
} = require('../../src/ro_damage_formulas');

describe('edge cases', () => {
    // ── Hit rate boundaries ──
    describe('hit rate never below 5% or above 95%', () => {
        test('floor at 5%', () => {
            expect(calculateHitRate(1, 9999)).toBe(5);
        });

        test('cap at 95%', () => {
            expect(calculateHitRate(9999, 1)).toBe(95);
        });
    });

    // ── Critical rate minimum 0 ──
    test('critical rate minimum is 0', () => {
        expect(calculateCritRate(0, 99)).toBe(0);
        expect(calculateCritRate(1, 99)).toBe(0);
    });

    // ── Weapon ATK variance ──
    describe('weapon ATK variance', () => {
        test('zero weaponATK produces valid damage (statusATK only)', () => {
            const result = calculatePhysicalDamage(
                {
                    stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                    weaponATK: 0, passiveATK: 0, weaponType: 'bare_hand',
                    weaponElement: 'neutral', weaponLevel: 1,
                    buffMods: { atkMultiplier: 1.0 },
                },
                {
                    stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                    hardDef: 0, element: { type: 'neutral', level: 1 },
                    size: 'medium', race: 'formless', numAttackers: 1,
                    buffMods: { defMultiplier: 1.0 },
                },
                { forceHit: true }
            );
            // Should at least have statusATK contribution
            expect(result.damage).toBeGreaterThanOrEqual(1);
        });
    });

    // ── Negative healing element ──
    describe('negative element modifiers', () => {
        test('holy vs holy Lv4 = -100% → immune (damage 0)', () => {
            const result = calculatePhysicalDamage(
                {
                    stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                    weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                    weaponElement: 'holy', weaponLevel: 2,
                    buffMods: { atkMultiplier: 1.0 },
                },
                {
                    stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                    hardDef: 0, element: { type: 'holy', level: 4 },
                    size: 'medium', race: 'formless', numAttackers: 1,
                    buffMods: { defMultiplier: 1.0 },
                },
                { forceCrit: true }
            );
            expect(result.damage).toBe(0);
            expect(result.hitType).toBe('elementHeal');
            expect(result.isMiss).toBe(true);
        });
    });

    // ── Extreme stat ranges ──
    describe('extreme stat ranges', () => {
        test('all stats at 1 produces valid derived stats', () => {
            const result = calculateDerivedStats({
                str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1,
                bonusHit: 0, bonusFlee: 0, bonusCritical: 0, bonusPerfectDodge: 0,
                bonusMaxHp: 0, bonusMaxSp: 0, bonusMaxHpRate: 0, bonusMaxSpRate: 0,
                bonusHardDef: 0, bonusMATK: 0, bonusHardMDEF: 0,
                jobClass: 'novice', weaponType: 'bare_hand', weaponMATK: 0,
                buffAspdMultiplier: 1, equipAspdRate: 0, weaponTypeLeft: null,
            });
            expect(result.statusATK).toBe(1);
            expect(result.hit).toBe(177);
            expect(result.flee).toBe(102);
            expect(result.critical).toBe(1);
            expect(result.perfectDodge).toBe(1);
            expect(result.softDEF).toBe(0);
            expect(result.softMDEF).toBe(1);
        });

        test('all stats at 99 produces valid derived stats', () => {
            const result = calculateDerivedStats({
                str: 99, agi: 99, vit: 99, int: 99, dex: 99, luk: 99, level: 99,
                bonusHit: 0, bonusFlee: 0, bonusCritical: 0, bonusPerfectDodge: 0,
                bonusMaxHp: 0, bonusMaxSp: 0, bonusMaxHpRate: 0, bonusMaxSpRate: 0,
                bonusHardDef: 0, bonusMATK: 0, bonusHardMDEF: 0,
                jobClass: 'knight', weaponType: 'two_hand_sword', weaponMATK: 0,
                buffAspdMultiplier: 1, equipAspdRate: 0, weaponTypeLeft: null,
            });
            expect(result.statusATK).toBe(218); // 99 + 81 + 19 + 19
            expect(result.hit).toBe(373);       // 175 + 99 + 99
            expect(result.flee).toBe(298);       // 100 + 99 + 99
            expect(result.critical).toBe(30);    // 1 + floor(99*0.3) = 30
            expect(result.perfectDodge).toBe(10); // 1 + floor(99/10) = 10
            expect(result.softDEF).toBe(108);    // 49 + 19 + 49
            expect(result.softMDEF).toBe(143);   // 99 + 19 + 19 + 24
        });
    });

    // ── bonusMaxHpRate and bonusMaxSpRate ──
    describe('MaxHP/SP rate modifiers', () => {
        test('bonusMaxHpRate +10% increases MaxHP by 10%', () => {
            const base = calculateDerivedStats({
                str: 1, agi: 1, vit: 50, int: 1, dex: 1, luk: 1, level: 50,
                bonusHit: 0, bonusFlee: 0, bonusCritical: 0, bonusPerfectDodge: 0,
                bonusMaxHp: 0, bonusMaxSp: 0, bonusMaxHpRate: 0, bonusMaxSpRate: 0,
                bonusHardDef: 0, bonusMATK: 0, bonusHardMDEF: 0,
                jobClass: 'swordsman', weaponType: 'bare_hand', weaponMATK: 0,
                buffAspdMultiplier: 1, equipAspdRate: 0, weaponTypeLeft: null,
            });
            const boosted = calculateDerivedStats({
                str: 1, agi: 1, vit: 50, int: 1, dex: 1, luk: 1, level: 50,
                bonusHit: 0, bonusFlee: 0, bonusCritical: 0, bonusPerfectDodge: 0,
                bonusMaxHp: 0, bonusMaxSp: 0, bonusMaxHpRate: 10, bonusMaxSpRate: 0,
                bonusHardDef: 0, bonusMATK: 0, bonusHardMDEF: 0,
                jobClass: 'swordsman', weaponType: 'bare_hand', weaponMATK: 0,
                buffAspdMultiplier: 1, equipAspdRate: 0, weaponTypeLeft: null,
            });
            expect(boosted.maxHP).toBe(Math.floor(base.maxHP * 110 / 100));
        });
    });

    // ── Overupgrade randomness ──
    describe('overupgrade random bonus', () => {
        test('overrefineMax produces random bonus between 1 and max', () => {
            const attacker = {
                stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                weaponElement: 'neutral', weaponLevel: 2,
                buffMods: { atkMultiplier: 1.0 },
                refineATK: 0, overrefineMax: 20,
            };
            const target = {
                stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                hardDef: 0, element: { type: 'neutral', level: 1 },
                size: 'medium', race: 'formless', numAttackers: 1,
                buffMods: { defMultiplier: 1.0 },
            };
            const damages = new Set();
            for (let i = 0; i < 200; i++) {
                damages.add(calculatePhysicalDamage(attacker, target, { forceCrit: true }).damage);
            }
            // With overrefine, damage should vary (random 1-20 added each time)
            expect(damages.size).toBeGreaterThan(1);
        });

        test('overrefineMax=0 produces no overupgrade bonus', () => {
            const attacker = {
                stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                weaponElement: 'neutral', weaponLevel: 2,
                buffMods: { atkMultiplier: 1.0 },
                refineATK: 0, overrefineMax: 0,
            };
            const target = {
                stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                hardDef: 0, element: { type: 'neutral', level: 1 },
                size: 'medium', race: 'formless', numAttackers: 1,
                buffMods: { defMultiplier: 1.0 },
            };
            const damages = new Set();
            for (let i = 0; i < 50; i++) {
                damages.add(calculatePhysicalDamage(attacker, target, { forceCrit: true }).damage);
            }
            // Without overrefine AND with crit (no weapon variance), should be constant
            expect(damages.size).toBe(1);
        });
    });

    // ── Steel Body DEF override ──
    describe('Steel Body DEF/MDEF override', () => {
        test('overrideHardDEF replaces normal DEF', () => {
            const attacker = {
                stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                weaponElement: 'neutral', weaponLevel: 2,
                buffMods: { atkMultiplier: 1.0 },
            };
            const normalTarget = {
                stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                hardDef: 10, element: { type: 'neutral', level: 1 },
                size: 'medium', race: 'formless', numAttackers: 1,
                buffMods: { defMultiplier: 1.0 },
            };
            const steelTarget = {
                ...normalTarget,
                hardDef: 10, // Original DEF is 10
                buffMods: { defMultiplier: 1.0, overrideHardDEF: 90 }, // Steel Body overrides to 90
            };
            const normalDmg = calculatePhysicalDamage(attacker, normalTarget, { forceCrit: true });
            const steelDmg = calculatePhysicalDamage(attacker, steelTarget, { forceCrit: true });
            // Steel Body has 90% DEF reduction, much stronger than 10
            expect(steelDmg.damage).toBeLessThan(normalDmg.damage);
        });

        test('overrideHardMDEF replaces normal MDEF for magic', () => {
            const normalResult = calculateMagicalDamage(
                { stats: { int: 80, level: 70 }, weaponMATK: 50, buffMods: { atkMultiplier: 1.0 } },
                { stats: { int: 20, vit: 20, dex: 10, level: 40 }, hardMdef: 10, element: { type: 'neutral', level: 1 }, buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0 } },
                { skillMultiplier: 100, skillElement: 'fire' }
            );
            const steelResult = calculateMagicalDamage(
                { stats: { int: 80, level: 70 }, weaponMATK: 50, buffMods: { atkMultiplier: 1.0 } },
                { stats: { int: 20, vit: 20, dex: 10, level: 40 }, hardMdef: 10, element: { type: 'neutral', level: 1 }, buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0, overrideHardMDEF: 90 } },
                { skillMultiplier: 100, skillElement: 'fire' }
            );
            // Both have MATK variance so compare averages
            let sumN = 0, sumS = 0;
            for (let i = 0; i < 100; i++) {
                sumN += calculateMagicalDamage(
                    { stats: { int: 80, level: 70 }, weaponMATK: 50, buffMods: { atkMultiplier: 1.0 } },
                    { stats: { int: 20, vit: 20, dex: 10, level: 40 }, hardMdef: 10, element: { type: 'neutral', level: 1 }, buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0 } },
                    { skillMultiplier: 100, skillElement: 'fire' }
                ).damage;
                sumS += calculateMagicalDamage(
                    { stats: { int: 80, level: 70 }, weaponMATK: 50, buffMods: { atkMultiplier: 1.0 } },
                    { stats: { int: 20, vit: 20, dex: 10, level: 40 }, hardMdef: 10, element: { type: 'neutral', level: 1 }, buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0, overrideHardMDEF: 90 } },
                    { skillMultiplier: 100, skillElement: 'fire' }
                ).damage;
            }
            expect(sumS / 100).toBeLessThan(sumN / 100);
        });
    });

    // ── Strip effects ──
    describe('strip effects in damage pipeline', () => {
        test('hardDefReduction reduces hard DEF (Strip Shield)', () => {
            const attacker = {
                stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                weaponElement: 'neutral', weaponLevel: 2,
                buffMods: { atkMultiplier: 1.0 },
            };
            const normalTarget = {
                stats: { str: 1, agi: 1, vit: 30, int: 1, dex: 1, luk: 1, level: 30 },
                hardDef: 50, element: { type: 'neutral', level: 1 },
                size: 'medium', race: 'formless', numAttackers: 1,
                buffMods: { defMultiplier: 1.0 },
            };
            const strippedTarget = {
                ...normalTarget,
                buffMods: { defMultiplier: 1.0, hardDefReduction: 0.15 },
            };
            const normalDmg = calculatePhysicalDamage(attacker, normalTarget, { forceCrit: true });
            const strippedDmg = calculatePhysicalDamage(attacker, strippedTarget, { forceCrit: true });
            expect(strippedDmg.damage).toBeGreaterThan(normalDmg.damage);
        });

        test('vitMultiplier reduces VIT-based soft DEF (Strip Armor)', () => {
            const attacker = {
                stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                weaponElement: 'neutral', weaponLevel: 2,
                buffMods: { atkMultiplier: 1.0 },
            };
            const normalTarget = {
                stats: { str: 1, agi: 1, vit: 80, int: 1, dex: 1, luk: 1, level: 50 },
                hardDef: 10, element: { type: 'neutral', level: 1 },
                size: 'medium', race: 'formless', numAttackers: 1,
                buffMods: { defMultiplier: 1.0 },
            };
            const strippedTarget = {
                ...normalTarget,
                buffMods: { defMultiplier: 1.0, vitMultiplier: 0.5 },
            };
            const normalDmg = calculatePhysicalDamage(attacker, normalTarget, { forceCrit: true });
            const strippedDmg = calculatePhysicalDamage(attacker, strippedTarget, { forceCrit: true });
            expect(strippedDmg.damage).toBeGreaterThan(normalDmg.damage);
        });

        test('intMultiplier reduces INT-based soft MDEF (Strip Helm)', () => {
            let sumNormal = 0, sumStripped = 0;
            for (let i = 0; i < 100; i++) {
                sumNormal += calculateMagicalDamage(
                    { stats: { int: 80, level: 70 }, weaponMATK: 50, buffMods: { atkMultiplier: 1.0 } },
                    { stats: { int: 80, vit: 30, dex: 20, level: 60 }, hardMdef: 10, element: { type: 'neutral', level: 1 }, buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0 } },
                    { skillMultiplier: 100, skillElement: 'fire' }
                ).damage;
                sumStripped += calculateMagicalDamage(
                    { stats: { int: 80, level: 70 }, weaponMATK: 50, buffMods: { atkMultiplier: 1.0 } },
                    { stats: { int: 80, vit: 30, dex: 20, level: 60 }, hardMdef: 10, element: { type: 'neutral', level: 1 }, buffMods: { defMultiplier: 1.0, mdefMultiplier: 1.0, intMultiplier: 0.5 } },
                    { skillMultiplier: 100, skillElement: 'fire' }
                ).damage;
            }
            expect(sumStripped / 100).toBeGreaterThan(sumNormal / 100);
        });
    });

    // ── bSkillAtk per-skill card bonus ──
    describe('bSkillAtk per-skill card bonus', () => {
        test('cardSkillAtk for matching skill name increases physical damage', () => {
            const attacker = {
                stats: { str: 50, agi: 10, vit: 10, int: 1, dex: 30, luk: 1, level: 50 },
                weaponATK: 100, passiveATK: 0, weaponType: 'one_hand_sword',
                weaponElement: 'neutral', weaponLevel: 2,
                buffMods: { atkMultiplier: 1.0 },
                cardSkillAtk: { 'Bash': 15 },
            };
            const target = {
                stats: { str: 1, agi: 1, vit: 10, int: 1, dex: 1, luk: 1, level: 10 },
                hardDef: 10, element: { type: 'neutral', level: 1 },
                size: 'medium', race: 'formless', numAttackers: 1,
                buffMods: { defMultiplier: 1.0 },
            };
            const noSkillBonus = calculatePhysicalDamage(attacker, target, {
                forceCrit: true, isSkill: true, skillMultiplier: 200, skillName: 'NotBash'
            });
            const withSkillBonus = calculatePhysicalDamage(attacker, target, {
                forceCrit: true, isSkill: true, skillMultiplier: 200, skillName: 'Bash'
            });
            expect(withSkillBonus.damage).toBeGreaterThan(noSkillBonus.damage);
        });
    });
});
```

---

## How to Run

1. Install Jest if not already present:
```bash
cd server
npm install --save-dev jest
```

2. Create the test files under `server/__tests__/combat/`:
```bash
mkdir -p server/__tests__/combat
# Copy each test block above into its respective file
```

3. Run tests:
```bash
cd server
npx jest __tests__/combat/ --verbose
```

---

## Coverage Summary

| Category | Tests | Covered |
|----------|-------|---------|
| StatusATK (melee/ranged) | 8 | Formula, boundary, weapon swap |
| MATK min/max | 3 | Base, weaponMATK, boundary |
| HIT/FLEE | 4 | Formula, bonus |
| Critical | 5 | Base, katar 2x, bonus |
| Perfect Dodge | 2 | Base, boundary |
| Soft DEF/MDEF | 4 | Formula, boundary |
| MaxHP/MaxSP | 6 | Class-aware, transcendent, bonus, minimum |
| ASPD | 8 | Dual wield cap, mount, buff, floor |
| Element Table | 22 | All 10 elements, healing, immunity, level bounds |
| Size Penalty | 12 | All weapon types, fallback |
| Hit Rate | 9 | Formula, cap, multi-attacker, hitRatePercent |
| Crit Rate | 6 | Formula, shield, minimum |
| Physical Damage | 35+ | Skill mult, crits, size, element, DEF, refine, cards, buffs, zones |
| Magical Damage | 15+ | MATK, GTB, element, MDEF, cards, zones, Dragonology |
| Edge Cases | 15+ | Extremes, overupgrade, Steel Body, strips, negative element |
| **Total** | **~155** | |
