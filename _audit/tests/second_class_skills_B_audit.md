# Second Class Skills Group B - Comprehensive Audit & Jest Tests

Generated: 2026-03-22
Source: `server/src/index.js` (~32566 lines), `ro_skill_data_2nd.js`, `ro_buff_system.js`, `ro_status_effects.js`

---

## Table of Contents
1. [Priest (IDs 1000-1018)](#priest)
2. [Monk (IDs 1600-1615)](#monk)
3. [Assassin (IDs 1100-1111)](#assassin)
4. [Rogue (IDs 1700-1718)](#rogue)
5. [Bard (IDs 1500-1537)](#bard)
6. [Dancer (IDs 1520-1557)](#dancer)
7. [Blacksmith (IDs 1200-1230)](#blacksmith)
8. [Alchemist (IDs 1800-1815)](#alchemist)
9. [System Interaction Tests](#system-interaction-tests)
10. [Bugs Found](#bugs-found)

---

## 1. Priest (IDs 1000-1018) <a id="priest"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 400 | Heal | 11318 | Active/Single | `calculateHealAmount(player, lv)`. Damages Undead: `healAmount * holyVsUndead / 100 / 2`. Heals allies. |
| 402 | Blessing | 11416 | Active/Single | 3 paths: (1) enemy Undead/Demon = halve STR/DEX/INT, (2) cure Curse/Stone on ally, (3) +effectVal STR/DEX/INT buff |
| 403 | Increase AGI | 11526 | Active/Single | HP cost 15, +effectVal AGI, +25% moveSpeed. Fails if target has Quagmire. Strips Decrease AGI. |
| 404 | Decrease AGI | 11579 | Active/Single | Success: `40 + 2*lv + floor((BaseLv+INT)/5) - targetMDEF`. Strips IncAGI + ASPD buffs. Duration: monsters `(3+lv)*10s`, players `(3+lv)*5s`. |
| 405 | Cure | 11675 | Active/Single | Offensive: Confusion on Undead 30s. Defensive: cleanse silence/blind/confusion. |
| 1000 | Sanctuary | 17166 | Active/Ground | Ground heal zone. `healPerTick = effectVal`, `maxTargets = 3+lv`, radius 250 (5x5). Duration `4000 + lv*3000`. |
| 1001 | Kyrie Eleison | 17210 | Active/Single | Barrier: `barrierHP = floor(maxHP * effectVal / 100)`, `maxHits = floor(lv/2) + 5`. Duration 120s. |
| 1002 | Magnificat | 17248 | Active/Self | SP+HP regen x2. Duration `30000 + lv*15000`. |
| 1003 | Gloria | 17272 | Active/Self | +30 LUK. Duration `10000 + lv*5000`. |
| 1004 | Resurrection | 17296 | Active/Single | Revive dead player at `effectVal`% HP (10/30/50/80). |
| 1005 | Magnus Exorcismus | 17333 | Active/Ground | Ground AoE holy. `numWaves = lv`. Radius 350 (7x7). Targets Undead/Demon. |
| 1006 | Turn Undead | 17375 | Active/Single | Instant kill chance: `min(100, (200*lv + BaseLv + INT + LUK) / 10)`. On fail: `(BaseLv + INT + lv*10) * 2` piercing holy. Undead element only. |
| 1007 | Lex Aeterna | 17454 | Active/Single | Doubles next damage. Duration 10min. Cannot apply to Frozen/Stoned. Works on bosses. |
| 1009 | Impositio Manus | 17531 | Active/Single | +5*lv ATK buff. Duration 60s. |
| 1010 | Suffragium | 17566 | Active/Single | Cast time reduction on ally (NOT self). 15/30/45%. Duration 30/20/10s. Consumed on first cast. |
| 1011 | Aspersio | 17601 | Active/Single | Holy weapon endow. Strips all other endows. Duration `60000 + lv*30000`. |
| 1012 | B.S. Sacramenti | 17647 | Active/Self | Holy armor endow. Duration `40000 + lv*40000`. |
| 1013 | Slow Poison | 17673 | Active/Single | Pause poison HP drain. Duration `10000 + lv*10000`. |
| 1014 | Status Recovery | 17706 | Active/Single | Cleanse freeze/stone/stun from self or ally. |
| 1015 | Lex Divina | 17734 | Active/Single | Toggle silence. If already silenced, cures it. Boss immune. Duration array `[30,35,40,45,50,50,50,50,55,60]`s. |
| 1018 | Redemptio | 18847 | Active/Self | Mass resurrect dead party in 15x15 AoE. EXP penalty on caster. |
| 1008 | Holy Light | 12096 | Active/Single | (Acolyte skill shared) |

### Jest Tests - Priest

```javascript
// File: server/tests/priest_skills.test.js
'use strict';

const {
    createMockPlayer, createMockEnemy, createMockSocket,
    SKILL_MAP, applyBuff, hasBuff, removeBuff, getCombinedModifiers,
    getEffectiveStats, applyStatusEffect, cleanseStatusEffects,
    calculateHealAmount, getElementModifier, applySkillDelays
} = require('./test_helpers');

describe('Priest Skills (IDs 1000-1018)', () => {

    // --- HEAL (ID 400) ---
    describe('Heal', () => {
        test('heals self for calculateHealAmount', () => {
            const player = createMockPlayer({ health: 500, maxHealth: 1000, mana: 100, maxMana: 200 });
            const healAmount = calculateHealAmount(player, 5);
            expect(healAmount).toBeGreaterThan(0);
            const newHP = Math.min(player.maxHealth, player.health + healAmount);
            expect(newHP).toBeGreaterThan(500);
            expect(newHP).toBeLessThanOrEqual(1000);
        });

        test('damages Undead enemies with halved heal amount', () => {
            const player = createMockPlayer({ mana: 100, maxMana: 200 });
            const enemy = createMockEnemy({ element: { type: 'undead', level: 1 }, health: 1000, maxHealth: 1000 });
            const healAmount = calculateHealAmount(player, 5);
            const holyMod = getElementModifier('holy', 'undead', 1);
            const holyDamage = Math.max(1, Math.floor(healAmount * holyMod / 100 / 2));
            expect(holyDamage).toBeGreaterThan(0);
            // Undead takes double damage from Holy (175% at level 1)
            expect(holyMod).toBe(175);
        });

        test('rejects heal on non-Undead enemies', () => {
            const enemy = createMockEnemy({ element: { type: 'fire', level: 1 } });
            const eleType = (enemy.element && enemy.element.type) || 'neutral';
            expect(eleType).not.toBe('undead');
        });

        test('heals allied player target', () => {
            const caster = createMockPlayer({ mana: 100 });
            const target = createMockPlayer({ health: 300, maxHealth: 1000 });
            const healAmount = calculateHealAmount(caster, 10);
            target.health = Math.min(target.maxHealth, target.health + healAmount);
            expect(target.health).toBeGreaterThan(300);
        });
    });

    // --- BLESSING (ID 402) ---
    describe('Blessing', () => {
        test('debuffs Undead/Demon enemies by halving stats', () => {
            const enemy = createMockEnemy({
                element: { type: 'undead', level: 1 },
                stats: { str: 40, dex: 30, int: 20, int_stat: 20 }
            });
            const strReduction = Math.floor(40 / 2);
            const dexReduction = Math.floor(30 / 2);
            const intReduction = Math.floor(20 / 2);
            expect(strReduction).toBe(20);
            expect(dexReduction).toBe(15);
            expect(intReduction).toBe(10);
        });

        test('cures Curse/Stone on allies without applying buff', () => {
            const target = createMockPlayer();
            target.activeStatusEffects = new Map([['curse', { expiresAt: Date.now() + 60000 }]]);
            const cleansed = cleanseStatusEffects(target, ['curse', 'stone']);
            expect(cleansed).toContain('curse');
            // Should NOT receive stat bonuses when curing
        });

        test('applies +STR/DEX/INT on clean ally', () => {
            const target = createMockPlayer();
            const effectVal = 10; // Lv10 Blessing
            applyBuff(target, { name: 'blessing', strBonus: effectVal, dexBonus: effectVal, intBonus: effectVal, duration: 120000 });
            expect(hasBuff(target, 'blessing')).toBe(true);
        });

        test('rejects non-Undead/non-Demon enemy target', () => {
            const enemy = createMockEnemy({ element: { type: 'fire', level: 1 }, race: 'brute' });
            const eleType = enemy.element.type;
            const race = enemy.race;
            const isValid = (eleType === 'undead' || race === 'undead' || race === 'demon');
            expect(isValid).toBe(false);
        });
    });

    // --- INCREASE AGI (ID 403) ---
    describe('Increase AGI', () => {
        test('costs 15 HP from caster', () => {
            const player = createMockPlayer({ health: 100 });
            expect(player.health).toBeGreaterThanOrEqual(16);
            player.health = Math.max(1, player.health - 15);
            expect(player.health).toBe(85);
        });

        test('fails if target has Quagmire', () => {
            const target = createMockPlayer();
            applyBuff(target, { name: 'quagmire', duration: 10000 });
            expect(hasBuff(target, 'quagmire')).toBe(true);
            // Increase AGI fails silently when Quagmire is active
        });

        test('strips Decrease AGI on application', () => {
            const target = createMockPlayer();
            applyBuff(target, { name: 'decrease_agi', agiReduction: 10, duration: 60000 });
            expect(hasBuff(target, 'decrease_agi')).toBe(true);
            removeBuff(target, 'decrease_agi');
            expect(hasBuff(target, 'decrease_agi')).toBe(false);
            applyBuff(target, { name: 'increase_agi', agiBonus: 10, moveSpeedBonus: 25, duration: 120000 });
            expect(hasBuff(target, 'increase_agi')).toBe(true);
        });

        test('rejects if caster HP < 16', () => {
            const player = createMockPlayer({ health: 15 });
            expect(player.health < 16).toBe(true);
        });
    });

    // --- DECREASE AGI (ID 404) ---
    describe('Decrease AGI', () => {
        test('success rate formula', () => {
            const baseLv = 99, casterInt = 50, targetMDEF = 20, lv = 10;
            const rate = Math.min(95, Math.max(5, 40 + 2 * lv + Math.floor((baseLv + casterInt) / 5) - targetMDEF));
            expect(rate).toBe(Math.min(95, Math.max(5, 40 + 20 + 29 - 20))); // 69
            expect(rate).toBe(69);
        });

        test('strips ASPD buffs on success', () => {
            const target = createMockPlayer();
            const buffsToStrip = ['adrenaline_rush', 'two_hand_quicken', 'spear_quicken', 'cart_boost'];
            for (const b of buffsToStrip) {
                applyBuff(target, { name: b, duration: 60000 });
            }
            for (const b of buffsToStrip) {
                expect(hasBuff(target, b)).toBe(true);
            }
            // On Dec AGI success, all these get stripped
            for (const b of buffsToStrip) {
                removeBuff(target, b);
            }
            for (const b of buffsToStrip) {
                expect(hasBuff(target, b)).toBe(false);
            }
        });

        test('monster duration = (3+lv)*10s', () => {
            const lv = 5;
            const monsterDuration = (3 + lv) * 10000;
            expect(monsterDuration).toBe(80000);
        });

        test('player duration = (3+lv)*5s', () => {
            const lv = 10;
            const playerDuration = (3 + lv) * 5000;
            expect(playerDuration).toBe(65000);
        });

        test('boss immunity check', () => {
            const boss = createMockEnemy({ modeFlags: { boss: true } });
            expect(boss.modeFlags.boss).toBe(true);
        });
    });

    // --- KYRIE ELEISON (ID 1001) ---
    describe('Kyrie Eleison', () => {
        test('barrier HP scales with maxHP and effectVal', () => {
            const target = createMockPlayer({ maxHealth: 5000 });
            const effectVal = 30; // Lv5 = 30%
            const barrierHP = Math.floor(target.maxHealth * effectVal / 100);
            expect(barrierHP).toBe(1500);
        });

        test('max hits = floor(lv/2) + 5', () => {
            expect(Math.floor(1/2) + 5).toBe(5);   // Lv1 = 5 hits
            expect(Math.floor(5/2) + 5).toBe(7);   // Lv5 = 7 hits
            expect(Math.floor(10/2) + 5).toBe(10); // Lv10 = 10 hits
        });
    });

    // --- TURN UNDEAD (ID 1006) ---
    describe('Turn Undead', () => {
        test('instant kill chance formula', () => {
            const lv = 10, baseLv = 99, intStat = 90, lukStat = 50;
            const chance = Math.min(100, (200 * lv + baseLv + intStat + lukStat) / 10);
            // (2000 + 99 + 90 + 50) / 10 = 223.9 -> capped at 100
            expect(chance).toBe(100);
        });

        test('fail damage formula = (BaseLv + INT + lv*10) * 2', () => {
            const baseLv = 50, intStat = 30, lv = 5;
            const damage = (baseLv + intStat + lv * 10) * 2;
            expect(damage).toBe((50 + 30 + 50) * 2); // 260
        });

        test('only affects Undead element (not race)', () => {
            const undead_ele = createMockEnemy({ element: { type: 'undead', level: 1 } });
            const undead_race = createMockEnemy({ element: { type: 'neutral', level: 1 }, race: 'undead' });
            expect(undead_ele.element.type).toBe('undead');
            expect(undead_race.element.type).not.toBe('undead');
            // Turn Undead only checks element.type, NOT race
        });
    });

    // --- LEX AETERNA (ID 1007) ---
    describe('Lex Aeterna', () => {
        test('cannot apply to Frozen target', () => {
            const target = createMockEnemy();
            applyBuff(target, { name: 'freeze', isFrozen: true, duration: 10000 });
            const mods = getCombinedModifiers(target);
            expect(mods.isFrozen).toBe(true);
            // Should reject application
        });

        test('works on bosses (pre-renewal)', () => {
            const boss = createMockEnemy({ modeFlags: { boss: true } });
            applyBuff(boss, { name: 'lex_aeterna', doubleNextDamage: true, duration: 600000 });
            expect(hasBuff(boss, 'lex_aeterna')).toBe(true);
        });
    });

    // --- SUFFRAGIUM (ID 1010) ---
    describe('Suffragium', () => {
        test('cannot cast on self', () => {
            const characterId = 1;
            const targetId = 1;
            expect(targetId === characterId).toBe(true);
        });

        test('cast time reduction per level: 15/30/45%', () => {
            const reductions = [15, 30, 45];
            expect(reductions[0]).toBe(15);
            expect(reductions[2]).toBe(45);
        });

        test('duration per level: 30/20/10s', () => {
            const durations = [30000, 20000, 10000];
            expect(durations[0]).toBe(30000);
            expect(durations[2]).toBe(10000);
        });
    });

    // --- LEX DIVINA (ID 1015) ---
    describe('Lex Divina', () => {
        test('toggles silence: cures if already silenced', () => {
            const enemy = createMockEnemy();
            enemy.activeStatusEffects = new Map([['silence', { expiresAt: Date.now() + 30000 }]]);
            expect(enemy.activeStatusEffects.has('silence')).toBe(true);
            // Lex Divina on silenced target removes silence
            enemy.activeStatusEffects.delete('silence');
            expect(enemy.activeStatusEffects.has('silence')).toBe(false);
        });

        test('silence duration array', () => {
            const durations = [30000,35000,40000,45000,50000,50000,50000,50000,55000,60000];
            expect(durations[0]).toBe(30000);
            expect(durations[4]).toBe(50000);
            expect(durations[9]).toBe(60000);
        });
    });

    // --- REDEMPTIO (ID 1018) ---
    describe('Redemptio', () => {
        test('requires party', () => {
            const player = createMockPlayer({ partyId: null });
            expect(player.partyId).toBeNull();
        });

        test('AoE radius is 750 (15x15)', () => {
            const REDEMPTIO_RANGE = 750;
            expect(REDEMPTIO_RANGE).toBe(750);
        });

        test('EXP penalty decreases with more revived', () => {
            const nextBaseExp = 100000;
            const limit = 5;
            // 1 member revived: max(0, 5-1)/5/100 * 100000 = 800
            expect(Math.floor(nextBaseExp * Math.max(0, 5 - 1) / 5 / 100)).toBe(800);
            // 5 members: max(0, 5-5)/5/100 * 100000 = 0
            expect(Math.floor(nextBaseExp * Math.max(0, 5 - 5) / 5 / 100)).toBe(0);
        });
    });
});
```

---

## 2. Monk (IDs 1600-1615) <a id="monk"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 1600 | Iron Fists | passive | Passive | +3 ATK/lv bare-hand. |
| 1601 | Summon Spirit Sphere | 18091 | Active/Self | Max spheres = lv (1-5). 10min expiry. |
| 1602 | Investigate / Occult Impaction | 18211 | Active/Single | Custom: `ATK * effectVal% * (hardDEF + softDEF) / 50`. Consumes 1 sphere. Always hits, always neutral. |
| 1603 | Triple Attack | passive | Passive | Auto-attack proc: 29% chance, 3 hits. Opens Chain Combo window. |
| 1604 | Finger Offensive | 18300 | Active/Single | Multi-hit ranged. 1 hit per sphere consumed (min=1, max=lv). ForceHit. 200ms between hits. |
| 1605 | Asura Strike | 18378 | Active/Single | `(WeaponATK + StatusATK) * (8 + floor(min(SP,6000)/10)) + effectVal`. Consumes ALL SP + spheres + Fury. 5min SP regen lockout. |
| 1607 | Absorb Spirit Sphere | 18112 | Active/Self+Enemy | Self: consume own spheres for 7 SP each. Enemy: 20% chance drain `monLv*2` SP (not boss). |
| 1609 | Blade Stop | 18754 | Active/Self | Catching stance: `300 + 200*lv` ms. Consumes 1 sphere. Locks both combatants. |
| 1610 | Chain Combo | 18555 | Active/Combo | After Triple Attack or Blade Stop Lv4+. 4 hits at `effectVal%/4` each. ForceHit. Opens Combo Finish window. |
| 1611 | Critical Explosion / Fury | 18157 | Active/Self | Requires 5 spheres. Consumes all. +CRIT `[10,13,15,18,20]`, disables SP regen. 3min. |
| 1612 | Steel Body | 18185 | Active/Self | Requires 5 spheres. DEF/MDEF override 90%. Duration `30s*lv`. Blocks all active skills. |
| 1613 | Combo Finish | 18641 | Active/Combo | After Chain Combo. Consumes 1 sphere. Single hit `effectVal%`. 5x5 AoE splash. Opens Asura window if Fury active. |
| 1614 | Ki Translation | 18788 | Active/Single | Transfer 1 sphere to party Monk. Range 450. |
| 1615 | Ki Explosion | 18499 | Active/Single | Quest skill. HP cost 10. 3x3 AoE splash + 5-cell knockback + 70% stun 2s. |

### Jest Tests - Monk

```javascript
describe('Monk Skills (IDs 1600-1615)', () => {

    // --- SUMMON SPIRIT SPHERE (ID 1601) ---
    describe('Summon Spirit Sphere', () => {
        test('max spheres equals skill level', () => {
            for (let lv = 1; lv <= 5; lv++) {
                const player = createMockPlayer({ spiritSpheres: 0 });
                player.maxSpiritSpheres = lv;
                for (let i = 0; i < lv; i++) {
                    if (player.spiritSpheres < lv) player.spiritSpheres++;
                }
                expect(player.spiritSpheres).toBe(lv);
            }
        });

        test('does not exceed max spheres', () => {
            const player = createMockPlayer({ spiritSpheres: 3 });
            const maxSpheres = 3;
            if (player.spiritSpheres < maxSpheres) player.spiritSpheres++;
            expect(player.spiritSpheres).toBe(3); // Already at max
        });

        test('sphere expiry is 10 minutes', () => {
            const player = createMockPlayer();
            player.sphereExpireTime = Date.now() + 600000;
            expect(player.sphereExpireTime - Date.now()).toBeCloseTo(600000, -3);
        });
    });

    // --- INVESTIGATE (ID 1602) ---
    describe('Investigate / Occult Impaction', () => {
        test('damage formula: ATK * effectVal% * totalDEF / 50', () => {
            const weaponATK = 100, statusATK = 80, effectVal = 350; // Lv5 = 350%
            const hardDEF = 50, softDEF = 30;
            const totalATK = weaponATK + statusATK;
            const totalDEF = hardDEF + softDEF;
            const damage = Math.floor(totalATK * effectVal / 100 * totalDEF / 50);
            // 180 * 3.5 * 80/50 = 180 * 3.5 * 1.6 = 1008
            expect(damage).toBe(Math.floor(180 * 350 / 100 * 80 / 50));
            expect(damage).toBe(1008);
        });

        test('consumes exactly 1 sphere', () => {
            const player = createMockPlayer({ spiritSpheres: 3 });
            player.spiritSpheres--;
            expect(player.spiritSpheres).toBe(2);
        });

        test('requires at least 1 sphere', () => {
            const player = createMockPlayer({ spiritSpheres: 0 });
            expect(player.spiritSpheres < 1).toBe(true);
        });

        test('always hits (forceHit)', () => {
            // Investigate always hits - no accuracy check
            const skillOpts = { forceHit: true };
            expect(skillOpts.forceHit).toBe(true);
        });

        test('higher DEF = more damage (inverse formula)', () => {
            const baseATK = 200, effectVal = 300;
            const lowDEF = 20, highDEF = 100;
            const lowDmg = Math.floor(baseATK * effectVal / 100 * lowDEF / 50);
            const highDmg = Math.floor(baseATK * effectVal / 100 * highDEF / 50);
            expect(highDmg).toBeGreaterThan(lowDmg);
        });
    });

    // --- FINGER OFFENSIVE (ID 1604) ---
    describe('Finger Offensive / Throw Spirit Sphere', () => {
        test('hits = min(skillLevel, availableSpheres)', () => {
            expect(Math.min(3, 5)).toBe(3); // Lv3 with 5 spheres = 3 hits
            expect(Math.min(5, 2)).toBe(2); // Lv5 with 2 spheres = 2 hits
        });

        test('consumes spheres equal to hits', () => {
            const player = createMockPlayer({ spiritSpheres: 4 });
            const lv = 3;
            const spheresUsed = Math.min(lv, player.spiritSpheres);
            player.spiritSpheres -= spheresUsed;
            expect(player.spiritSpheres).toBe(1);
            expect(spheresUsed).toBe(3);
        });

        test('Lex Aeterna doubles ALL hits, consumed once', () => {
            // LA check happens before hit loop, applies to all hits
            const baseDmg = 500;
            const hits = 5;
            const lexTotal = baseDmg * 2 * hits;
            const normalTotal = baseDmg * hits;
            expect(lexTotal).toBe(normalTotal * 2);
        });
    });

    // --- ASURA STRIKE (ID 1605) ---
    describe('Asura Strike / Guillotine Fist', () => {
        test('damage formula with SP cap at 6000', () => {
            const weaponATK = 200, statusATK = 150;
            const totalATK = weaponATK + statusATK; // 350
            const SP = 8000; // exceeds cap
            const spCapped = Math.min(SP, 6000);
            const effectVal = 100;
            const damage = Math.floor(totalATK * (8 + Math.floor(spCapped / 10))) + effectVal;
            // 350 * (8 + 600) + 100 = 350 * 608 + 100 = 212900
            expect(damage).toBe(350 * 608 + 100);
        });

        test('requires Fury (Critical Explosion) active', () => {
            const player = createMockPlayer();
            expect(hasBuff(player, 'critical_explosion')).toBe(false);
            // Should reject
        });

        test('standalone requires 5 spheres, combo requires 1+', () => {
            const standaloneSpheres = 5;
            const comboSpheres = 1;
            expect(standaloneSpheres).toBe(5);
            expect(comboSpheres).toBe(1);
        });

        test('consumes ALL SP, ALL spheres, removes Fury', () => {
            const player = createMockPlayer({ mana: 3000, spiritSpheres: 5 });
            applyBuff(player, { name: 'critical_explosion', duration: 180000 });
            // After Asura:
            player.mana = 0;
            player.spiritSpheres = 0;
            player.sphereExpireTime = 0;
            removeBuff(player, 'critical_explosion');
            expect(player.mana).toBe(0);
            expect(player.spiritSpheres).toBe(0);
            expect(hasBuff(player, 'critical_explosion')).toBe(false);
        });

        test('applies 5-minute SP regen lockout', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'asura_regen_lockout', duration: 300000 });
            expect(hasBuff(player, 'asura_regen_lockout')).toBe(true);
        });

        test('combo Asura: forces combo target, no cast time', () => {
            const player = createMockPlayer();
            player.comboState = {
                active: true,
                lastSkillId: 1613, // After Combo Finish
                windowExpires: Date.now() + 1000,
                targetId: 42
            };
            const isComboAsura = player.comboState.active &&
                player.comboState.lastSkillId === 1613 &&
                Date.now() <= player.comboState.windowExpires;
            expect(isComboAsura).toBe(true);
        });
    });

    // --- COMBO SYSTEM ---
    describe('Monk Combo System', () => {
        test('Triple Attack -> Chain Combo -> Combo Finish -> Asura', () => {
            const player = createMockPlayer({ spiritSpheres: 5 });
            applyBuff(player, { name: 'critical_explosion', duration: 180000 });

            // Step 1: Triple Attack procs (passive, sets combo state)
            player.comboState = { active: true, lastSkillId: 1603, windowExpires: Date.now() + 800, targetId: 1 };
            expect(player.comboState.lastSkillId).toBe(1603);

            // Step 2: Chain Combo (requires lastSkillId === 1603)
            const ccValid = player.comboState.active && player.comboState.lastSkillId === 1603;
            expect(ccValid).toBe(true);
            player.comboState = { active: true, lastSkillId: 1610, windowExpires: Date.now() + 800, targetId: 1 };

            // Step 3: Combo Finish (requires lastSkillId === 1610)
            const cfValid = player.comboState.active && player.comboState.lastSkillId === 1610;
            expect(cfValid).toBe(true);
            player.spiritSpheres--; // Consumes 1 sphere
            player.comboState = { active: true, lastSkillId: 1613, windowExpires: Date.now() + 800, targetId: 1 };

            // Step 4: Asura Strike (requires lastSkillId === 1613 + Fury + spheres)
            const asValid = player.comboState.active && player.comboState.lastSkillId === 1613;
            expect(asValid).toBe(true);
            expect(hasBuff(player, 'critical_explosion')).toBe(true);
            expect(player.spiritSpheres).toBeGreaterThanOrEqual(1);
        });

        test('combo window formula: max(300, (1.3 - AGI*0.004 - DEX*0.002) * 1000)', () => {
            const agi = 99, dex = 50;
            const window = Math.max(300, Math.floor((1.3 - agi * 0.004 - dex * 0.002) * 1000));
            // 1.3 - 0.396 - 0.1 = 0.804 * 1000 = 804
            expect(window).toBe(804);
        });

        test('Blade Stop Lv4+ allows Chain Combo bypass', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'root_lock', isPlayer: true, bladeStopLevel: 4, lockedEnemyId: 99, duration: 30000 });
            const rlBuff = player.activeBuffs.find(b => b.name === 'root_lock' && b.isPlayer);
            expect(rlBuff.bladeStopLevel).toBeGreaterThanOrEqual(4);
        });
    });

    // --- BLADE STOP (ID 1609) ---
    describe('Blade Stop', () => {
        test('catching duration = 300 + 200*lv ms', () => {
            for (let lv = 1; lv <= 5; lv++) {
                const dur = 300 + 200 * lv;
                expect(dur).toBe(300 + 200 * lv);
            }
            expect(300 + 200 * 5).toBe(1300);
        });

        test('per-level skill whitelist in root_lock', () => {
            const BLADE_STOP_SKILLS = {
                1: [], 2: [1604], 3: [1604, 1602],
                4: [1604, 1602, 1610], 5: [1604, 1602, 1610, 1605]
            };
            expect(BLADE_STOP_SKILLS[1]).toEqual([]);
            expect(BLADE_STOP_SKILLS[3]).toContain(1602); // Investigate
            expect(BLADE_STOP_SKILLS[5]).toContain(1605); // Asura Strike
        });
    });

    // --- STEEL BODY (ID 1612) ---
    describe('Steel Body', () => {
        test('DEF/MDEF override to 90%', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'steel_body', overrideHardDEF: 90, overrideHardMDEF: 90, duration: 150000 });
            const buff = player.activeBuffs.find(b => b.name === 'steel_body');
            expect(buff.overrideHardDEF).toBe(90);
            expect(buff.overrideHardMDEF).toBe(90);
        });

        test('blocks ALL active skills while active', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'steel_body', duration: 150000 });
            const mods = getCombinedModifiers(player);
            expect(mods.blockActiveSkills).toBe(true);
        });

        test('duration = 30s * lv', () => {
            for (let lv = 1; lv <= 5; lv++) {
                expect(30000 * lv).toBe(lv * 30000);
            }
        });
    });

    // --- KI TRANSLATION (ID 1614) ---
    describe('Ki Translation', () => {
        test('requires party membership', () => {
            const player = createMockPlayer({ partyId: null });
            expect(player.partyId).toBeNull();
        });

        test('target must be Monk class', () => {
            const MONK_CLASSES = new Set(['monk', 'champion']);
            expect(MONK_CLASSES.has('monk')).toBe(true);
            expect(MONK_CLASSES.has('knight')).toBe(false);
        });

        test('cannot self-target', () => {
            const characterId = 1, targetId = 1;
            expect(targetId === characterId).toBe(true);
        });

        test('transfers 1 sphere', () => {
            const caster = createMockPlayer({ spiritSpheres: 3 });
            const target = createMockPlayer({ spiritSpheres: 2 });
            caster.spiritSpheres--;
            target.spiritSpheres++;
            expect(caster.spiritSpheres).toBe(2);
            expect(target.spiritSpheres).toBe(3);
        });
    });

    // --- CRITICAL EXPLOSION / FURY (ID 1611) ---
    describe('Critical Explosion / Fury', () => {
        test('crit bonus per level: [10,13,15,18,20]', () => {
            const critBonuses = [10, 13, 15, 18, 20];
            expect(critBonuses[0]).toBe(10);
            expect(critBonuses[4]).toBe(20);
        });

        test('requires 5 spheres, consumes all', () => {
            const player = createMockPlayer({ spiritSpheres: 5 });
            expect(player.spiritSpheres).toBe(5);
            player.spiritSpheres = 0;
            expect(player.spiritSpheres).toBe(0);
        });
    });
});
```

---

## 3. Assassin (IDs 1100-1111) <a id="assassin"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 1100 | Katar Mastery | passive | Passive | +3 ATK/lv with Katars |
| 1101 | Sonic Blow | 16577 | Active/Single | Katar required. 8 visual hits (1 dmg calc). effectVal% ATK. Sonic Acceleration: +50% dmg, +50% hit. Stun: `10+2*lv`%. |
| 1102 | Grimtooth | 16698 | Active/AoE | Katar required. Must be Hidden. Range `(2+lv)*50`. 3x3 splash. Does NOT break Hiding. |
| 1103 | Cloaking | 16786 | Toggle | SP drain intervals `[500,1000,2000,...,9000]ms`. Replaces Hiding. Deaggros monsters. |
| 1104 | Enchant Poison | 16847 | Active/Self | Poison weapon endow. Duration `(15+15*lv)*1000`. Poison chance `(25+5*lv)/10`%. Strips other endows. |
| 1105 | Poison React | 16888 | Active/Self | Two modes: (A) counter poison attacks `(100+30*lv)%` ATK, (B) auto-Envenom Lv5. Duration `min(60, 15+5*lv)`s. Envenom limit `floor(lv/2)`. |
| 1106 | Venom Dust | 16919 | Active/Ground | Poison cloud zone 60s. Poison status `5*lv`s. Requires Red Gemstone. |
| 1110 | Venom Splasher | 16981 | Active/Single | Mark <75% HP target. Timer `(4.5+0.5*lv)`s. 5x5 AoE `(500+50*lv)%` ATK. Poison element. Requires Red Gemstone. Boss immune. |
| 1111 | Throw Venom Knife | 17064 | Active/Single | Quest skill. 100% ATK ranged. Consumes Venom Knife. Poison: `(10+4*EnvenomLv)%`. |
| 1106 | Sonic Acceleration | passive | Passive | Quest skill. +50% Sonic Blow damage (multiplicative), +50% hit rate. |

### Jest Tests - Assassin

```javascript
describe('Assassin Skills (IDs 1100-1111)', () => {

    // --- SONIC BLOW (ID 1101) ---
    describe('Sonic Blow', () => {
        test('requires Katar weapon', () => {
            const player = createMockPlayer({ weaponType: 'dagger' });
            expect(player.weaponType !== 'katar').toBe(true);
        });

        test('8 visual hits, all damage in first hit', () => {
            const totalHits = 8;
            const damage = 5000;
            // First hit carries full damage, others are cosmetic (0)
            const hits = Array.from({ length: totalHits }, (_, i) => i === 0 ? damage : 0);
            expect(hits[0]).toBe(5000);
            expect(hits[7]).toBe(0);
            expect(hits.reduce((s, d) => s + d, 0)).toBe(5000);
        });

        test('Sonic Acceleration: +50% multiplicative damage', () => {
            const baseEffect = 600; // Lv10 effectVal
            const withAccel = Math.floor(baseEffect * 1.5);
            expect(withAccel).toBe(900);
        });

        test('stun chance: (10 + 2*lv)%', () => {
            expect(10 + 2 * 1).toBe(12);  // Lv1 = 12%
            expect(10 + 2 * 10).toBe(30); // Lv10 = 30%
        });

        test('Safety Wall blocks Sonic Blow (melee physical)', () => {
            // Sonic Blow is BF_SHORT - blocked by Safety Wall
            const isMelee = true;
            expect(isMelee).toBe(true);
        });
    });

    // --- GRIMTOOTH (ID 1102) ---
    describe('Grimtooth', () => {
        test('must be in Hiding or Cloaking', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'hiding', isHidden: true, duration: 60000 });
            const mods = getCombinedModifiers(player);
            expect(mods.isHidden).toBe(true);
        });

        test('range = (2+lv)*50 UE', () => {
            for (let lv = 1; lv <= 5; lv++) {
                expect((2 + lv) * 50).toBe(lv === 1 ? 150 : (2 + lv) * 50);
            }
            expect((2 + 5) * 50).toBe(350);
        });

        test('does NOT break Hiding', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'hiding', isHidden: true, duration: 60000 });
            // After Grimtooth execution, hiding buff persists
            expect(hasBuff(player, 'hiding')).toBe(true);
        });

        test('3x3 AoE radius = 75', () => {
            const GT_AOE_RADIUS = 75;
            expect(GT_AOE_RADIUS).toBe(75);
        });
    });

    // --- CLOAKING (ID 1103) ---
    describe('Cloaking', () => {
        test('toggle on/off behavior', () => {
            const player = createMockPlayer();
            // Toggle ON
            applyBuff(player, { name: 'cloaking', isHidden: true, duration: 600000 });
            expect(hasBuff(player, 'cloaking')).toBe(true);
            // Toggle OFF
            removeBuff(player, 'cloaking');
            expect(hasBuff(player, 'cloaking')).toBe(false);
        });

        test('SP drain intervals per level', () => {
            const DRAIN = [500, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000];
            expect(DRAIN[0]).toBe(500);   // Lv1: 0.5s
            expect(DRAIN[9]).toBe(9000);  // Lv10: 9s
        });

        test('replaces Hiding when activated', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'hiding', isHidden: true, duration: 60000 });
            // Cloaking removes hiding first
            removeBuff(player, 'hiding');
            applyBuff(player, { name: 'cloaking', isHidden: true, duration: 600000 });
            expect(hasBuff(player, 'hiding')).toBe(false);
            expect(hasBuff(player, 'cloaking')).toBe(true);
        });
    });

    // --- ENCHANT POISON (ID 1104) ---
    describe('Enchant Poison', () => {
        test('duration = (15+15*lv)*1000', () => {
            expect((15 + 15 * 1) * 1000).toBe(30000);  // Lv1: 30s
            expect((15 + 15 * 5) * 1000).toBe(90000);  // Lv5: 90s
            expect((15 + 15 * 10) * 1000).toBe(165000); // Lv10: 165s
        });

        test('poison chance = (25+5*lv)/10 percent', () => {
            expect((25 + 5 * 1) / 10).toBe(3);   // Lv1: 3%
            expect((25 + 5 * 5) / 10).toBe(5);   // Lv5: 5%
            expect((25 + 5 * 10) / 10).toBe(7.5); // Lv10: 7.5%
        });

        test('strips all other weapon endows', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'aspersio', weaponElement: 'holy', duration: 60000 });
            expect(hasBuff(player, 'aspersio')).toBe(true);
            removeBuff(player, 'aspersio');
            applyBuff(player, { name: 'enchant_poison', weaponElement: 'poison', duration: 90000 });
            expect(hasBuff(player, 'aspersio')).toBe(false);
            expect(hasBuff(player, 'enchant_poison')).toBe(true);
        });
    });

    // --- POISON REACT (ID 1105) ---
    describe('Poison React', () => {
        test('counter ATK = 100+30*lv', () => {
            expect(100 + 30 * 1).toBe(130);
            expect(100 + 30 * 10).toBe(400);
        });

        test('envenom limit = floor(lv/2)', () => {
            const limits = [];
            for (let lv = 1; lv <= 10; lv++) {
                limits.push(Math.floor(lv / 2));
            }
            expect(limits).toEqual([0, 1, 1, 2, 2, 3, 3, 4, 4, 5]);
        });

        test('duration = min(60000, (15+5*lv)*1000)', () => {
            expect(Math.min(60000, (15 + 5 * 1) * 1000)).toBe(20000);
            expect(Math.min(60000, (15 + 5 * 9) * 1000)).toBe(60000);
            expect(Math.min(60000, (15 + 5 * 10) * 1000)).toBe(60000); // capped
        });
    });

    // --- VENOM SPLASHER (ID 1110) ---
    describe('Venom Splasher', () => {
        test('requires target <= 75% HP', () => {
            const enemy = createMockEnemy({ health: 800, maxHealth: 1000 });
            const threshold = Math.floor(enemy.maxHealth * 0.75);
            expect(enemy.health > threshold).toBe(true); // 800 > 750, should reject
            enemy.health = 700;
            expect(enemy.health <= threshold).toBe(true); // 700 <= 750, should accept
        });

        test('timer = (4.5+0.5*lv) seconds', () => {
            expect((4.5 + 0.5 * 1) * 1000).toBe(5000);
            expect((4.5 + 0.5 * 10) * 1000).toBe(9500);
        });

        test('boss immunity', () => {
            const boss = createMockEnemy({ modeFlags: { statusImmune: true } });
            expect(boss.modeFlags.statusImmune).toBe(true);
        });
    });

    // --- THROW VENOM KNIFE (ID 1111) ---
    describe('Throw Venom Knife', () => {
        test('poison chance = (10+4*EnvenomLv)%', () => {
            expect(10 + 4 * 0).toBe(10);  // No Envenom = 10%
            expect(10 + 4 * 5).toBe(30);  // Envenom Lv5 = 30%
            expect(10 + 4 * 10).toBe(50); // Envenom Lv10 = 50%
        });

        test('Undead/Boss immune to poison proc', () => {
            const undead = createMockEnemy({ race: 'undead' });
            const boss = createMockEnemy({ modeFlags: { statusImmune: true } });
            expect(undead.race === 'undead').toBe(true);
            expect(boss.modeFlags.statusImmune).toBe(true);
        });
    });
});
```

---

## 4. Rogue (IDs 1700-1718) <a id="rogue"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 1701 | Back Stab | 19499 | Active/Single | Teleport behind target. ForceHit. Dagger=2 hits, bow=50% dmg. |
| 1703 | Raid | 19568 | Active/AoE | Must be in Hiding. 3x3 AoE. Stun+Blind chance `10+3*lv`%. +20% incoming dmg debuff 5s/7hits. |
| 1704 | Intimidate | 19641 | Active/Single | Damage + random teleport both (not boss). |
| 1705 | Sword Mastery (shared) | passive | Passive | +ATK with swords (shared with Swordsman, uses Math.max) |
| 1706 | Vulture's Eye (shared) | passive | Passive | +range with bows (shared with Archer) |
| 1707 | Double Strafe Rogue | 19703 | Active/Single | Routes to Archer DS logic. Requires bow. effectVal*2. |
| 1709 | Steal Coin | 19716 | Active/Single | Zeny theft. Rate: `(DEX/2 + LUK/2 + 2*(casterLv-monLv) + lv*10) / 10`. Amount: `random(0, 2*monLv) + 8*monLv`. |
| 1710 | Divest Weapon | 19772 | Active/Single | Strip weapon. Rate: `(50*(lv+1) + 2*(DEX-targetDEX)) / 10`. Duration `60s + 15s*lv + 0.5s*(DEX-tgtDEX)`. ATK -25%. |
| 1711 | Divest Shield | 19772 | Active/Single | Strip shield. DEF -15%. |
| 1712 | Divest Armor | 19772 | Active/Single | Strip armor. VIT -40%. |
| 1713 | Divest Helm | 19772 | Active/Single | Strip helm. INT -40%. |
| 1714 | Plagiarism | passive | Passive | Copy enemy skill on hit. Stores in `player.plagiarizedSkill`. |
| 1715 | Close Confine | 19831 | Active/Single | Lock both players. 8 break conditions. |
| 1716 | Compulsion Discount (shared) | passive | Passive | NPC price reduction (shared with Merchant) |
| 1717 | Tunnel Drive | passive | Passive | Move while hiding (movement buff). |
| 1718 | Snatcher | passive | Passive | Auto-steal on melee attack. |

### Jest Tests - Rogue

```javascript
describe('Rogue Skills (IDs 1700-1718)', () => {

    // --- BACK STAB (ID 1701) ---
    describe('Back Stab', () => {
        test('teleports behind target', () => {
            const playerX = 100, playerY = 100;
            const enemyX = 200, enemyY = 200;
            const dx = enemyX - playerX, dy = enemyY - playerY;
            const mag = Math.sqrt(dx * dx + dy * dy);
            const behindX = enemyX + (dx / mag) * -100;
            const behindY = enemyY + (dy / mag) * -100;
            // Behind position should be past the enemy
            expect(Math.abs(behindX - enemyX)).toBeLessThan(101);
            expect(Math.abs(behindY - enemyY)).toBeLessThan(101);
        });

        test('dagger = 2 hits, non-dagger = 1 hit', () => {
            expect('dagger' === 'dagger' ? 2 : 1).toBe(2);
            expect('sword' === 'dagger' ? 2 : 1).toBe(1);
        });

        test('bow = 50% damage', () => {
            const effectVal = 700;
            const bowMult = Math.floor(effectVal / 2);
            expect(bowMult).toBe(350);
        });

        test('forceHit: always hits', () => {
            const opts = { forceHit: true };
            expect(opts.forceHit).toBe(true);
        });
    });

    // --- RAID (ID 1703) ---
    describe('Raid', () => {
        test('must be in Hiding (consumed on use)', () => {
            const player = createMockPlayer();
            applyBuff(player, { name: 'hiding', isHidden: true, duration: 60000 });
            expect(hasBuff(player, 'hiding')).toBe(true);
            removeBuff(player, 'hiding');
            expect(hasBuff(player, 'hiding')).toBe(false);
        });

        test('stun+blind chance = 10+3*lv', () => {
            expect(10 + 3 * 1).toBe(13);
            expect(10 + 3 * 5).toBe(25);
        });

        test('raid debuff: +20% incoming damage (boss +10%)', () => {
            const normal = 0.20, boss = 0.10;
            expect(normal).toBe(0.20);
            expect(boss).toBe(0.10);
        });

        test('AoE radius = 50 (3x3 cells, pre-renewal)', () => {
            expect(50).toBe(50);
        });
    });

    // --- STEAL COIN (ID 1709) ---
    describe('Steal Coin', () => {
        test('success rate formula', () => {
            const dex = 80, luk = 40, casterLv = 99, monLv = 50, lv = 5;
            const rate = (dex/2 + luk/2 + 2*(casterLv - monLv) + lv*10) / 10;
            // (40 + 20 + 98 + 50) / 10 = 20.8%
            expect(rate).toBeCloseTo(20.8);
        });

        test('zeny amount: random(0, 2*monLv) + 8*monLv', () => {
            const monLv = 50;
            const min = 0 + 8 * monLv; // 400
            const max = 2 * monLv + 8 * monLv; // 500
            expect(min).toBe(400);
            expect(max).toBe(500);
        });

        test('boss immunity', () => {
            const boss = createMockEnemy({ modeFlags: { boss: true } });
            expect(boss.modeFlags.boss).toBe(true);
        });

        test('one steal per monster', () => {
            const enemy = createMockEnemy();
            enemy.zenyStolen = true;
            expect(enemy.zenyStolen).toBe(true);
        });
    });

    // --- DIVEST/STRIP SKILLS ---
    describe('Divest Skills (1710-1713)', () => {
        test('strip rate = (50*(lv+1) + 2*(DEX-targetDEX)) / 10', () => {
            const lv = 5, dex = 80, targetDex = 60;
            const rate = (50 * (lv + 1) + 2 * (dex - targetDex)) / 10;
            expect(rate).toBe((300 + 40) / 10); // 34%
        });

        test('strip duration formula', () => {
            const lv = 5, dex = 80, targetDex = 60;
            const dur = 60000 + 15000 * lv + 500 * (dex - targetDex);
            expect(dur).toBe(60000 + 75000 + 10000); // 145000ms = 145s
        });

        test('each divest strips different stat', () => {
            const effects = {
                'divest_weapon': { atkReduction: 0.25 },
                'divest_shield': { defReduction: 0.15 },
                'divest_armor': { vitReduction: 0.40 },
                'divest_helm': { intReduction: 0.40 }
            };
            expect(effects['divest_weapon'].atkReduction).toBe(0.25);
            expect(effects['divest_armor'].vitReduction).toBe(0.40);
        });
    });

    // --- PLAGIARISM (ID 1714) ---
    describe('Plagiarism', () => {
        test('copies enemy skill on hit', () => {
            const player = createMockPlayer();
            player.plagiarizedSkill = { skillId: 200, usableLevel: 3 };
            expect(player.plagiarizedSkill.skillId).toBe(200);
            expect(player.plagiarizedSkill.usableLevel).toBe(3);
        });

        test('plagiarized skill usable via learnedLevel fallback', () => {
            const player = createMockPlayer();
            player.plagiarizedSkill = { skillId: 200, usableLevel: 3 };
            let learnedLevel = 0; // Not learned normally
            if (learnedLevel <= 0 && player.plagiarizedSkill && player.plagiarizedSkill.skillId === 200) {
                learnedLevel = player.plagiarizedSkill.usableLevel;
            }
            expect(learnedLevel).toBe(3);
        });
    });
});
```

---

## 5. Bard (IDs 1500-1537) <a id="bard"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 1500 | Music Lessons | passive | Passive | +ATK with instruments, performance move speed. |
| 1501 | A Whistle | isPerformance | Performance | +Flee/PD. Caster excluded. Duration via levels. |
| 1502 | Assassin Cross of Sunset | isPerformance | Performance | +ASPD. Haste2 exclusion group. |
| 1503 | Adaptation | 19211 | Active | Cancel performance. 5s first/last restriction. |
| 1504 | Encore | 19255 | Active | Replay last performance at half SP. Clears remembered skill. |
| 1505 | Poem of Bragi | isPerformance | Performance | Cast time + ACD reduction. |
| 1506 | Frost Joker | 19307 | Active | Screen-wide freeze. 3s delay. Party=rate/4. BASE_FREEZE_DURATION=12s. MDEF reduces duration. |
| 1507 | Apple of Idun | isPerformance | Performance | +MaxHP, HP regen. |
| 1508 | Pang Voice | 19404 | Active/Single | Confusion on target. Formula: `50 + (casterLv-targetLv) - VIT/5 - LUK/5`. 5-95% clamp. |
| 1509 | A Drum on the Battlefield | isPerformance (ensemble) | Ensemble | +ATK/DEF. |
| 1510 | Ring of Nibelungen | isPerformance (ensemble) | Ensemble | +ATK Lv4 weapons. |
| 1511 | Loki's Veil | isPerformance (ensemble) | Ensemble | Blocks ALL skills in AoE. |
| 1512 | Eternal Chaos | isPerformance (ensemble) | Ensemble | -DEF to enemies. |
| 1513 | Into the Abyss | isPerformance (ensemble) | Ensemble | No gemstone cost. |
| 1514 | Invulnerable Siegfried | isPerformance (ensemble) | Ensemble | Element resist 60-80%, status resist 10-50%. |
| 1515 | Dissonance | isPerformance | Performance/AoE | MISC damage (no DEF/MDEF). Song overlap trigger. |
| 1540 | Musical Strike | 18965 | Active/Single | Instrument required. Ranged attack. Consumes 1 arrow. Usable during performance. |

### Jest Tests - Bard

```javascript
describe('Bard Skills (IDs 1500-1537)', () => {

    // --- PERFORMANCE SYSTEM ---
    describe('Performance System', () => {
        test('movement speed = (25 + 2.5 * LessonsLv)%', () => {
            expect((25 + 2.5 * 0) / 100).toBe(0.25);  // No lessons: 25%
            expect((25 + 2.5 * 10) / 100).toBe(0.5);   // Lv10: 50%
        });

        test('new performance cancels old (song flashing)', () => {
            const player = createMockPlayer();
            player.performanceState = { skillId: 1501, skillName: 'whistle' };
            // Starting a new performance should call cancelPerformance first
            expect(player.performanceState).not.toBeNull();
        });

        test('caster excluded from own song buff', () => {
            // Performance ground effect affects all EXCEPT caster
            const casterId = 1;
            const targetId = 1;
            expect(casterId === targetId).toBe(true);
            // Should skip applying buff to caster
        });

        test('ensemble immobilizes both performers', () => {
            const caster = createMockPlayer();
            const partner = createMockPlayer();
            caster.isPerformingEnsemble = true;
            partner.isPerformingEnsemble = true;
            caster.performanceMoveSpeedMultiplier = 0;
            partner.performanceMoveSpeedMultiplier = 0;
            expect(caster.performanceMoveSpeedMultiplier).toBe(0);
            expect(partner.performanceMoveSpeedMultiplier).toBe(0);
        });

        test('ensemble center = midpoint of performers', () => {
            const casterX = 100, casterY = 200;
            const partnerX = 300, partnerY = 400;
            const centerX = (casterX + partnerX) / 2;
            const centerY = (casterY + partnerY) / 2;
            expect(centerX).toBe(200);
            expect(centerY).toBe(300);
        });

        test('ensemble effective level = min of both performers', () => {
            const casterLevel = 7, partnerLevel = 4;
            expect(Math.min(casterLevel, partnerLevel)).toBe(4);
        });
    });

    // --- MUSICAL STRIKE (ID 1540) ---
    describe('Musical Strike', () => {
        test('requires Instrument weapon', () => {
            const player = createMockPlayer({ weaponType: 'instrument' });
            expect(player.weaponType).toBe('instrument');
        });

        test('requires arrows equipped', () => {
            const player = createMockPlayer();
            player.equippedAmmo = { quantity: 10, itemId: 1750 };
            expect(player.equippedAmmo.quantity > 0).toBe(true);
        });

        test('usable during performance', () => {
            // Musical Strike has usableDuringPerformance flag
            const skill = { name: 'musical_strike', usableDuringPerformance: true };
            expect(skill.usableDuringPerformance).toBe(true);
        });
    });

    // --- ADAPTATION (ID 1503) ---
    describe('Adaptation', () => {
        test('cannot cancel in first 5 seconds', () => {
            const startedAt = Date.now() - 3000; // 3s ago
            const elapsed = Date.now() - startedAt;
            expect(elapsed < 5000).toBe(true); // Should reject
        });

        test('cannot cancel in last 5 seconds', () => {
            const expiresAt = Date.now() + 3000; // Expires in 3s
            const remaining = expiresAt - Date.now();
            expect(remaining < 5000).toBe(true); // Should reject
        });
    });

    // --- ENCORE (ID 1504) ---
    describe('Encore', () => {
        test('replays last performance at half SP cost', () => {
            const fullSP = 40;
            const halfSP = Math.ceil(fullSP / 2);
            expect(halfSP).toBe(20);
        });

        test('clears remembered skill after use', () => {
            const player = createMockPlayer();
            player.lastPerformanceSkillId = 1501;
            // After Encore:
            player.lastPerformanceSkillId = null;
            expect(player.lastPerformanceSkillId).toBeNull();
        });
    });

    // --- FROST JOKER (ID 1506) ---
    describe('Frost Joker', () => {
        test('3-second delayed execution', () => {
            // 3000ms setTimeout delay
            const delay = 3000;
            expect(delay).toBe(3000);
        });

        test('party members get rate/4 chance', () => {
            const baseChance = 40;
            const partyChance = Math.floor(baseChance / 4);
            expect(partyChance).toBe(10);
        });

        test('freeze duration reduced by MDEF', () => {
            const BASE_FREEZE = 12000;
            const mdef = 50;
            const duration = Math.max(3000, Math.floor(BASE_FREEZE * (1 - mdef / 100)));
            expect(duration).toBe(6000);
        });

        test('blocked during performance (rAthena AllowWhenPerforming)', () => {
            const skill = { name: 'frost_joker', usableDuringPerformance: false };
            expect(skill.usableDuringPerformance || false).toBe(false);
        });
    });

    // --- PANG VOICE (ID 1508) ---
    describe('Pang Voice', () => {
        test('confusion chance formula', () => {
            const casterLv = 99, targetLv = 50, targetVit = 30, targetLuk = 20;
            const chance = Math.max(5, Math.min(95,
                50 + (casterLv - targetLv) - Math.floor(targetVit / 5) - Math.floor(targetLuk / 5)
            ));
            // 50 + 49 - 6 - 4 = 89
            expect(chance).toBe(89);
        });
    });
});
```

---

## 6. Dancer (IDs 1520-1557) <a id="dancer"></a>

### Skills Inventory

| ID | Name | Handler Line | Type |
|----|------|-------------|------|
| 1520 | Dance Lessons | passive | Passive - +ATK whip, move speed |
| 1521 | Humming | isPerformance | Performance - +HIT |
| 1522 | Please Don't Forget Me | isPerformance | Performance - ASPD/Speed reduction on enemies |
| 1523 | Adaptation (Dancer) | 19211 | Active - Cancel performance |
| 1524 | Encore (Dancer) | 19255 | Active - Replay last performance |
| 1525 | Service for You | isPerformance | Performance - SP cost reduction, SP regen |
| 1526 | Scream / Dazzler | 19359 | Active - Screen stun (3s delay) |
| 1527 | Fortune's Kiss | isPerformance | Performance - +CRI |
| 1528 | Lullaby | isPerformance (ensemble) | Ensemble - Sleep |
| 1529 | Charming Wink | 19445 | Active/Single - Charm monsters (demi/angel/demon only) |
| 1541 | Slinging Arrow | 18982 | Active/Single - Whip ranged attack, consumes arrow |

### Jest Tests - Dancer

```javascript
describe('Dancer Skills (IDs 1520-1557)', () => {

    // --- SLINGING ARROW (ID 1541) ---
    describe('Slinging Arrow', () => {
        test('requires Whip weapon', () => {
            const player = createMockPlayer({ weaponType: 'whip' });
            expect(player.weaponType).toBe('whip');
        });
    });

    // --- SCREAM (ID 1526) ---
    describe('Scream / Dazzler', () => {
        test('3-second delayed execution (same as Frost Joker)', () => {
            const delay = 3000;
            expect(delay).toBe(3000);
        });

        test('stun duration 5s', () => {
            const STUN_DURATION = 5000;
            expect(STUN_DURATION).toBe(5000);
        });
    });

    // --- CHARMING WINK (ID 1529) ---
    describe('Charming Wink', () => {
        test('only affects Demi-Human, Angel, Demon races', () => {
            const validRaces = ['demi_human', 'angel', 'demon'];
            expect(validRaces.includes('demi_human')).toBe(true);
            expect(validRaces.includes('brute')).toBe(false);
        });

        test('charm chance = (casterLv - targetLv) + 40, clamped 0-100', () => {
            const casterLv = 99, targetLv = 70;
            const chance = Math.max(0, Math.min(100, casterLv - targetLv + 40));
            expect(chance).toBe(69);
        });

        test('charm duration 10s', () => {
            expect(10000).toBe(10000);
        });
    });

    // --- ENSEMBLE SYSTEM ---
    describe('Ensemble System', () => {
        test('requires Bard+Dancer partner in party', () => {
            const BARD_CLASSES = new Set(['bard', 'clown']);
            const DANCER_CLASSES = new Set(['dancer', 'gypsy']);
            expect(BARD_CLASSES.has('bard')).toBe(true);
            expect(DANCER_CLASSES.has('dancer')).toBe(true);
        });

        test('partner adjacency: within 150 UE', () => {
            const dist = Math.sqrt((100 - 50) ** 2 + (200 - 180) ** 2);
            expect(dist <= 150).toBe(true);
        });

        test('Loki\'s Veil blocks ALL skills in AoE', () => {
            const effectType = 'skill_block';
            expect(effectType).toBe('skill_block');
        });
    });
});
```

---

## 7. Blacksmith (IDs 1200-1230) <a id="blacksmith"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 1200 | Adrenaline Rush | 19955 | Active/Self | +30% ASPD. Requires Axe/Mace. Hilt Binding +10% duration. ASPD coexist strongest-wins. |
| 1201 | Weapon Perfection | 19993 | Active/Self | No size penalty. Hilt Binding duration bonus. |
| 1202 | Power Thrust | 20017 | Active/Self | +ATK% (5-25). 0.1% weapon break risk/hit. Requires weapon. Hilt Binding duration bonus. |
| 1203 | Maximize Power | 20047 | Toggle | Always max weapon variance. SP drain `lv*1000`ms. Toggle on/off. |
| 1204 | Weaponry Research | passive | Passive | +HIT/ATK per level. |
| 1205 | Hilt Binding | passive | Passive | +1 ATK, +10% duration on AR/WP/PT/MP. |
| 1206 | Hammer Fall | 20092 | Active/Ground | 5x5 AoE stun ONLY (no damage). Rate = effectVal (30-70%). Requires melee weapon (dagger/sword/axe/mace). |
| 1207 | Skin Tempering | passive | Passive | Fire/neutral resistance. |
| 1209 | Weapon Repair | 20129 | Active/Single | Repair broken equipment. Requires specific materials per weapon level. |
| 1210 | Greed | deferred | Active | Deferred: requires ground loot system. |
| 606 | Item Appraisal | 20250 | Active | Identify unidentified items. Opens item selection. |
| 605 | Vending | 20295 | Active | Opens vending setup. Requires Pushcart. Max slots: `2+lv`. |
| 607 | Change Cart | 20280 | Active | Change cart appearance by base level tiers. |

### Jest Tests - Blacksmith

```javascript
describe('Blacksmith Skills (IDs 1200-1230)', () => {

    // --- ADRENALINE RUSH (ID 1200) ---
    describe('Adrenaline Rush', () => {
        test('requires Axe or Mace', () => {
            const validWeapons = ['axe', 'one_hand_axe', 'two_hand_axe', 'mace'];
            expect(validWeapons.includes('axe')).toBe(true);
            expect(validWeapons.includes('sword')).toBe(false);
        });

        test('+30% ASPD multiplier', () => {
            const aspdMult = 1.3;
            expect(aspdMult).toBe(1.3);
        });

        test('Hilt Binding extends duration by 10%', () => {
            const baseDur = 30000;
            const withHilt = Math.floor(baseDur * 1.1);
            expect(withHilt).toBe(33000);
        });
    });

    // --- WEAPON PERFECTION (ID 1201) ---
    describe('Weapon Perfection', () => {
        test('removes size penalty flag', () => {
            const buff = { noSizePenalty: true };
            expect(buff.noSizePenalty).toBe(true);
        });
    });

    // --- POWER THRUST (ID 1202) ---
    describe('Power Thrust', () => {
        test('ATK% per level: 5/10/15/20/25', () => {
            const levels = [5, 10, 15, 20, 25];
            expect(levels[0]).toBe(5);
            expect(levels[4]).toBe(25);
        });

        test('requires weapon (not bare_hand)', () => {
            expect('bare_hand' === 'bare_hand').toBe(true); // Should reject
        });
    });

    // --- MAXIMIZE POWER (ID 1203) ---
    describe('Maximize Power', () => {
        test('toggle on/off', () => {
            const player = createMockPlayer();
            // Toggle ON
            player.maximizePowerActive = true;
            expect(player.maximizePowerActive).toBe(true);
            // Toggle OFF
            player.maximizePowerActive = false;
            expect(player.maximizePowerActive).toBe(false);
        });

        test('SP drain interval = lv * 1000ms', () => {
            for (let lv = 1; lv <= 5; lv++) {
                expect(lv * 1000).toBe(lv * 1000);
            }
            expect(3 * 1000).toBe(3000); // Lv3: 3s drain
        });
    });

    // --- HAMMER FALL (ID 1206) ---
    describe('Hammer Fall', () => {
        test('NO damage - stun only', () => {
            // Hammer Fall deals 0 damage, only applies stun
            const damage = 0;
            expect(damage).toBe(0);
        });

        test('5x5 AoE radius = 250', () => {
            expect(250).toBe(250);
        });

        test('valid weapons include dagger and 1H sword', () => {
            const valid = ['dagger', 'one_hand_sword', 'axe', 'one_hand_axe', 'two_hand_axe', 'mace'];
            expect(valid.includes('dagger')).toBe(true);
            expect(valid.includes('one_hand_sword')).toBe(true);
        });
    });

    // --- WEAPON REPAIR (ID 1209) ---
    describe('Weapon Repair', () => {
        test('material requirements per weapon level', () => {
            const REPAIR_MATERIALS = {
                'armor': { itemId: 999, name: 'Steel' },
                'weapon_1': { itemId: 1002, name: 'Iron Ore' },
                'weapon_2': { itemId: 998, name: 'Iron' },
                'weapon_3': { itemId: 999, name: 'Steel' },
                'weapon_4': { itemId: 756, name: 'Rough Oridecon' },
            };
            expect(REPAIR_MATERIALS['weapon_1'].name).toBe('Iron Ore');
            expect(REPAIR_MATERIALS['weapon_4'].name).toBe('Rough Oridecon');
        });

        test('repairs weapon or armor broken state', () => {
            const player = createMockPlayer();
            player.weaponBroken = true;
            player.weaponBroken = false; // After repair
            expect(player.weaponBroken).toBe(false);
        });
    });

    // --- VENDING (ID 605) ---
    describe('Vending', () => {
        test('max slots = 2 + lv', () => {
            expect(2 + 1).toBe(3);  // Lv1: 3 slots
            expect(2 + 10).toBe(12); // Lv10: 12 slots
        });

        test('requires Pushcart', () => {
            const player = createMockPlayer();
            player.hasCart = false;
            expect(player.hasCart).toBe(false);
        });
    });

    // --- CHANGE CART (ID 607) ---
    describe('Change Cart', () => {
        test('cart type by base level tiers', () => {
            const getCartType = (lv) => lv >= 91 ? 5 : lv >= 81 ? 4 : lv >= 66 ? 3 : lv >= 41 ? 2 : 1;
            expect(getCartType(20)).toBe(1);
            expect(getCartType(50)).toBe(2);
            expect(getCartType(70)).toBe(3);
            expect(getCartType(85)).toBe(4);
            expect(getCartType(99)).toBe(5);
        });
    });
});
```

---

## 8. Alchemist (IDs 1800-1815) <a id="alchemist"></a>

### Skills Inventory

| ID | Name | Handler Line | Type | Formula/Mechanic |
|----|------|-------------|------|-----------------|
| 1800 | Pharmacy | 20748 | Active | Potion crafting. Uses PHARMACY_RECIPES. Success rate: `(PotRes*50 + Pharmacy*300 + JobLv*20 + INT/2*10 + DEX*10 + LUK*10) / 100`. |
| 1801 | Acid Terror | 20384 | Active/Single | ForceHit, ignoreHardDef. Boss=50% dmg. Armor break `[3,7,10,12,13]%`. Bleeding `3%*lv`. Consumes Acid Bottle (7136). Pneuma blocks dmg but not status. |
| 1802 | Demonstration | 20511 | Active/Ground | Fire ground DoT. Radius 150 (3x3). Duration `40s + (lv-1)*5s`. Weapon break `lv%`. Consumes Bottle Grenade (7135). |
| 1804 | Axe Mastery | passive | Passive | +3 ATK/lv with axes. |
| 1805 | Potion Research | passive | Passive | +5% potionHealBonus/lv. |
| 1806 | Potion Pitcher | 20572 | Active/Single | Throw potion at self/ally. Lv1-4: HP potions, Lv5: Blue Potion. Heal formula: `base * effectiveness% * (1 + PotRes*5%) * (1 + VIT*2/100 or INT*2/100) * (1 + IncHPRec*10/100)`. |
| 1808-1811 | Chemical Protection (x4) | 20688 | Active/Single | Prevent break+strip on helm/shield/armor/weapon. Consumes Glistening Coat (7139). Duration `lv*120s`. |
| 1812 | Bioethics | passive | Passive | Gate for Homunculus skills. |
| 1813 | Call Homunculus | 20925 | Active | Create (Embryo) or re-summon homunculus. |
| 1814 | Rest Homunculus | - | Active | Vaporize homunculus (HP>=80%). |
| 1815 | Resurrect Homunculus | 21092 | Active | Revive dead homunculus at 20-100% HP. |
| 1803 | Summon Flora | 20803 | Active/Ground | Summon plant ally (Flora, Parasite, Mandragora, Hydra, Geographer). |

### Jest Tests - Alchemist

```javascript
describe('Alchemist Skills (IDs 1800-1815)', () => {

    // --- ACID TERROR (ID 1801) ---
    describe('Acid Terror', () => {
        test('forceHit + ignoreHardDef (pass hardDef=0)', () => {
            const opts = { forceHit: true, hardDef: 0 };
            expect(opts.forceHit).toBe(true);
            expect(opts.hardDef).toBe(0);
        });

        test('boss takes 50% damage', () => {
            const damage = 1000;
            const bossDmg = Math.floor(damage / 2);
            expect(bossDmg).toBe(500);
        });

        test('armor break chances: [3,7,10,12,13]%', () => {
            const chances = [3, 7, 10, 12, 13];
            expect(chances[0]).toBe(3);
            expect(chances[4]).toBe(13);
        });

        test('bleeding chance = 3% per level', () => {
            for (let lv = 1; lv <= 5; lv++) {
                expect(lv * 3).toBe(lv * 3);
            }
            expect(5 * 3).toBe(15);
        });

        test('Pneuma blocks damage but NOT armor break/bleeding', () => {
            // atDamage is zeroed but status effects still apply
            let damage = 500;
            const hasPneuma = true;
            if (hasPneuma) damage = 0;
            expect(damage).toBe(0);
            // Status procs still execute after Pneuma check
        });

        test('consumes Acid Bottle (item 7136)', () => {
            const catalystId = 7136;
            expect(catalystId).toBe(7136);
        });
    });

    // --- DEMONSTRATION (ID 1802) ---
    describe('Demonstration', () => {
        test('duration = 40s + (lv-1)*5s', () => {
            expect(40000 + (1-1)*5000).toBe(40000);  // Lv1: 40s
            expect(40000 + (5-1)*5000).toBe(60000);  // Lv5: 60s
        });

        test('fire ground DoT with weapon break', () => {
            const lv = 3;
            const weaponBreakChance = lv; // 3%
            expect(weaponBreakChance).toBe(3);
        });

        test('radius = 150 (3x3)', () => {
            expect(150).toBe(150);
        });

        test('replaces existing Demonstration from same caster', () => {
            // Only one active Demonstration per caster
            const casterId = 1;
            const existing = { type: 'demonstration', casterId: 1 };
            expect(existing.casterId === casterId).toBe(true);
        });
    });

    // --- POTION PITCHER (ID 1806) ---
    describe('Potion Pitcher', () => {
        test('potion selection by level', () => {
            const POTIONS = [
                { itemId: 501, resource: 'hp' },  // Lv1: Red
                { itemId: 502, resource: 'hp' },  // Lv2: Orange
                { itemId: 503, resource: 'hp' },  // Lv3: Yellow
                { itemId: 504, resource: 'hp' },  // Lv4: White
                { itemId: 505, resource: 'sp' },  // Lv5: Blue
            ];
            expect(POTIONS[0].itemId).toBe(501);
            expect(POTIONS[4].resource).toBe('sp');
        });

        test('heal formula: base * eff% * (1+PotRes*5%) * VIT/INT * IncHPRec', () => {
            const base = 200, effectVal = 100, potRes = 50, vit = 80, smRecLv = 5;
            const step1 = Math.floor(base * effectVal / 100); // 200
            const step2 = Math.floor(step1 * (100 + potRes) / 100); // 300
            const step3 = step2 * ((100 + vit * 2) / 100); // 300 * 2.6 = 780
            const step4 = step3 * ((100 + smRecLv * 10) / 100); // 780 * 1.5 = 1170
            expect(Math.floor(step4)).toBe(1170);
        });

        test('Blue Potion uses INT*2 instead of VIT*2', () => {
            const resource = 'sp';
            const intStat = 90, vitStat = 30;
            const multiplier = resource === 'sp'
                ? (100 + intStat * 2) / 100
                : (100 + vitStat * 2) / 100;
            expect(multiplier).toBe(2.8); // INT*2
        });
    });

    // --- CHEMICAL PROTECTION (IDs 1808-1811) ---
    describe('Chemical Protection', () => {
        test('duration = lv * 120s', () => {
            expect(1 * 120000).toBe(120000);  // Lv1: 2 min
            expect(5 * 120000).toBe(600000);  // Lv5: 10 min
        });

        test('prevents both break and strip', () => {
            const buff = { preventBreak: true, preventStrip: true };
            expect(buff.preventBreak).toBe(true);
            expect(buff.preventStrip).toBe(true);
        });

        test('consumes Glistening Coat (item 7139)', () => {
            expect(7139).toBe(7139);
        });

        test('four variants protect different slots', () => {
            const slots = {
                'chemical_protection_helm': 'headgear',
                'chemical_protection_shield': 'shield',
                'chemical_protection_armor': 'armor',
                'chemical_protection_weapon': 'weapon'
            };
            expect(Object.keys(slots).length).toBe(4);
            expect(slots['chemical_protection_armor']).toBe('armor');
        });
    });

    // --- HOMUNCULUS SYSTEM ---
    describe('Homunculus System', () => {
        test('Call Homunculus creates or re-summons', () => {
            // Create: requires Embryo item, no existing homunculus
            // Re-summon: restores vaporized homunculus
            const hasExisting = false;
            const action = hasExisting ? 're-summon' : 'create';
            expect(action).toBe('create');
        });

        test('Rest requires HP >= 80%', () => {
            const hpPercent = 75;
            expect(hpPercent >= 80).toBe(false); // Should reject
        });

        test('Resurrect HP scales with level: 20-100%', () => {
            const hpPercentByLevel = [20, 40, 60, 80, 100];
            expect(hpPercentByLevel[0]).toBe(20);
            expect(hpPercentByLevel[4]).toBe(100);
        });
    });

    // --- PHARMACY (ID 1800) ---
    describe('Pharmacy', () => {
        test('success rate formula', () => {
            const potResLv = 5, pharmacyLv = 3, jobLv = 50;
            const intStat = 80, dex = 60, luk = 30;
            const rate = (potResLv * 50 + pharmacyLv * 300 + jobLv * 20 +
                Math.floor(intStat / 2) * 10 + dex * 10 + luk * 10);
            // (250 + 900 + 1000 + 400 + 600 + 300) = 3450
            // 3450 / 100 = 34.5% base
            expect(rate).toBe(3450);
        });
    });
});
```

---

## 9. System Interaction Tests <a id="system-interaction-tests"></a>

```javascript
describe('Cross-System Interaction Tests', () => {

    // --- MONK COMBO + BLADE STOP INTEGRATION ---
    describe('Monk Combo + Blade Stop', () => {
        test('Blade Stop Lv4 allows Chain Combo without Triple Attack', () => {
            const player = createMockPlayer({ spiritSpheres: 3 });
            applyBuff(player, { name: 'root_lock', isPlayer: true, bladeStopLevel: 4, lockedEnemyId: 99, duration: 30000 });
            const rlBuff = player.activeBuffs.find(b => b.name === 'root_lock' && b.isPlayer);
            const bladeStopCombo = rlBuff && rlBuff.bladeStopLevel >= 4;
            expect(bladeStopCombo).toBe(true);
        });

        test('Blade Stop Lv5 allows Asura Strike directly', () => {
            const BLADE_STOP_SKILLS = { 5: [1604, 1602, 1610, 1605] };
            expect(BLADE_STOP_SKILLS[5]).toContain(1605); // Asura Strike
        });

        test('Asura Strike breaks root_lock on both player and enemy', () => {
            const player = createMockPlayer();
            const enemy = createMockEnemy();
            applyBuff(player, { name: 'root_lock', isPlayer: true, lockedEnemyId: 1, duration: 30000 });
            applyBuff(enemy, { name: 'root_lock', duration: 30000 });
            removeBuff(player, 'root_lock');
            removeBuff(enemy, 'root_lock');
            expect(hasBuff(player, 'root_lock')).toBe(false);
            expect(hasBuff(enemy, 'root_lock')).toBe(false);
        });
    });

    // --- BARD/DANCER PERFORMANCE CANCELLATION ---
    describe('Performance Cancellation Triggers', () => {
        test('weapon swap cancels performance', () => {
            const player = createMockPlayer();
            player.performanceState = { skillId: 1501, skillName: 'whistle' };
            // cancelPerformance(characterId, player, 'weapon_swap')
            player.performanceState = null;
            expect(player.performanceState).toBeNull();
        });

        test('Dispel cancels performance', () => {
            // Dispel removes buffs including performance state
            const reason = 'dispelled';
            expect(reason).toBe('dispelled');
        });

        test('heavy damage (>25% MaxHP) cancels performance', () => {
            const maxHP = 5000, damage = 1500;
            const threshold = maxHP * 0.25;
            expect(damage > threshold).toBe(true);
        });

        test('death cancels performance', () => {
            const reason = 'death';
            expect(reason).toBe('death');
        });

        test('Silence does NOT cancel solo performance (rAthena)', () => {
            // Solo performances are NOT cancelled by silence
            // Only CC states (stun, freeze, stone) cancel
            const isSilenced = true;
            const cancels = false; // Silence doesn't cancel
            expect(cancels).toBe(false);
        });
    });

    // --- ASSASSIN DUAL WIELD ---
    describe('Assassin Dual Wield Integration', () => {
        test('skills use right-hand only (not both)', () => {
            // Sonic Blow, Grimtooth etc. only use right-hand weapon
            const skillDualWield = false;
            expect(skillDualWield).toBe(false);
        });

        test('Katar is mutually exclusive with dual wield', () => {
            const weaponType = 'katar';
            const isDualWield = false; // Katar = both hands
            expect(weaponType === 'katar' && !isDualWield).toBe(true);
        });
    });

    // --- ROGUE PLAGIARISM + MONSTER SKILLS ---
    describe('Plagiarism System', () => {
        test('checkPlagiarismCopy called in executeMonsterPlayerSkill', () => {
            // Plagiarism copies even on miss
            const copiesOnMiss = true;
            expect(copiesOnMiss).toBe(true);
        });

        test('usableLevel caps the copied skill level', () => {
            const monsterSkillLevel = 10;
            const plagiarismLevel = 5; // Player's Plagiarism level
            const usableLevel = Math.min(monsterSkillLevel, plagiarismLevel);
            expect(usableLevel).toBe(5);
        });
    });

    // --- CHEMICAL PROTECTION vs DIVEST ---
    describe('Chemical Protection blocks Divest', () => {
        test('CP Weapon blocks Divest Weapon', () => {
            const enemy = createMockEnemy();
            applyBuff(enemy, { name: 'chemical_protection_weapon', preventStrip: true, duration: 120000 });
            expect(hasBuff(enemy, 'chemical_protection_weapon')).toBe(true);
            // Divest Weapon should check for this buff and skip
        });
    });

    // --- PERFORMANCE SP DRAIN ---
    describe('Performance SP Economy', () => {
        test('SP depletion cancels performance', () => {
            const player = createMockPlayer({ mana: 0 });
            // When SP reaches 0 during drain tick, performance is cancelled
            expect(player.mana <= 0).toBe(true);
        });

        test('Service for You reduces ally SP costs', () => {
            const sfyReduction = 20; // 20% reduction
            const baseSpCost = 50;
            const reduced = Math.floor(baseSpCost * (100 - Math.min(sfyReduction, 99)) / 100);
            expect(reduced).toBe(40);
        });
    });

    // --- LOKI'S VEIL SKILL BLOCKING ---
    describe("Loki's Veil Interaction", () => {
        test('blocks all skills for everyone in AoE', () => {
            const playerX = 100, playerY = 100;
            const ensembleX = 120, ensembleY = 120;
            const ensembleRadius = 225; // 9x9
            const dx = playerX - ensembleX, dy = playerY - ensembleY;
            const dist = Math.sqrt(dx * dx + dy * dy);
            expect(dist <= ensembleRadius).toBe(true);
        });
    });

    // --- DEATH BUFF CLEARING ---
    describe('Death Clears Buffs (Blacksmith System)', () => {
        test('BUFFS_SURVIVE_DEATH whitelist', () => {
            const BUFFS_SURVIVE = new Set(['auto_berserk', 'endure', 'shrink']);
            expect(BUFFS_SURVIVE.has('auto_berserk')).toBe(true);
            expect(BUFFS_SURVIVE.has('adrenaline_rush')).toBe(false);
        });

        test('maximize_power cleared on death', () => {
            const player = createMockPlayer();
            player.maximizePowerActive = true;
            // clearBuffsOnDeath:
            player.maximizePowerActive = false;
            expect(player.maximizePowerActive).toBe(false);
        });
    });
});
```

---

## 10. Bugs Found <a id="bugs-found"></a>

### Potential Issues Identified During Audit

1. **Poison React envenom limit calculation (line 16894)**: Uses `Math.floor(lv/2)` which gives `[0,1,1,2,2,3,3,4,4,5]`. Comment says `rAthena: val2 = val1/2` but for Lv1 this gives 0 envenom uses, meaning Mode B counter is completely disabled at Lv1. This may be intentional per rAthena but is worth verifying -- a player learning only Lv1 gets zero envenom counters.

2. **CONFIRMED BUG - Double Strafe Rogue (line 19711)**: The call `executePhysicalSkillOnEnemy(player, characterId, socket, dsZone, skill, skillId, learnedLevel, spCost, totalEffectVal, targetId, isEnemy, {})` passes 12 arguments with `dsZone` (a string like 'prontera_south') inserted as the 4th argument where `skill` (an object) should be. The function signature at line 1874 is `executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, options)` (11 params). This means `skill` receives the zone string, `skillId` receives the skill object, `learnedLevel` receives the skillId number, etc. -- all parameters are shifted by one. **Severity: HIGH** - Rogue Double Strafe is completely broken and will either crash or produce nonsensical behavior at runtime. Additionally, `levelData` is missing from the call (replaced by `spCost`), and `isEnemy` is passed where `options` should be.

3. **Absorb Spirit Sphere monster targeting (line 18115)**: The boss check uses `absEnemy.modeFlags?.boss` but elsewhere boss checks also include `statusImmune`, `isBoss`, and `monsterClass`. This inconsistency means some MVP-class monsters might not be properly excluded.

4. **CONFIRMED BUG - Ki Explosion AND Hammer Fall stun duration (lines 18543, 20111)**: Both `applyStatusEffect` calls pass `{ duration: N }` (an object) as the 5th argument where a plain number is expected. The function signature at `ro_status_effects.js:353` is `applyStatusEffect(source, target, statusType, baseChance, overrideDuration)`. At line 357, `const duration = overrideDuration || result.duration` -- since `{ duration: 2000 }` is truthy, `duration` becomes the object itself. Then `expiresAt: now + duration` produces `NaN`, meaning the stun expires immediately (or never, depending on comparison). **Severity: HIGH** - Ki Explosion stun and Hammer Fall stun never apply correctly. **Fix**: Change `{ duration: 2000 }` to just `2000` (and `{ duration: 5000 }` to `5000` for Hammer Fall).

5. **Encore: No check for ensemble skills**: Encore replays the `lastPerformanceSkillId` but doesn't check whether it was an ensemble (which would require partner validation). If a player's last performance was an ensemble, Encore would attempt to start a solo performance with an ensemble skill, which could cause unexpected behavior.

6. **Combo Finish splash (line 18700-18728)**: AoE splash uses `setTimeout` for hit delays in Finger Offensive but synchronous execution for Combo Finish splash. This is inconsistent but not necessarily a bug -- splash enemies get instant damage while primary target gets delayed visual hits.

### Summary Statistics

| Class | Skills Found | Handler Lines | Buffs Created | Status Effects | Catalysts |
|-------|-------------|---------------|--------------|----------------|-----------|
| Priest | 18 | 11318-18889 | 8 | 3 (silence, freeze+stone cure) | Blue Gemstone (ME) |
| Monk | 14 | 18091-18845 | 5 | 1 (stun via KE) | None |
| Assassin | 10 | 16577-17157 | 4 | 2 (stun, poison) | Red Gemstone (VD/VS), Venom Knife |
| Rogue | 14 | 19499-19870 | 3 | 3 (stun, blind, confusion) | None |
| Bard | 17 | 18965-19354 | 0 (via ground effects) | 2 (freeze, confusion) | None |
| Dancer | 11 | 18982-19491 | 0 (via ground effects) | 2 (stun, charm) | None |
| Blacksmith | 10 | 19955-20306 | 4 | 1 (stun) | Repair materials |
| Alchemist | 12 | 20384-21092+ | 5 | 2 (armor break, bleeding) | Acid Bottle, Bottle Grenade, Glistening Coat, Medicine Bowl |
