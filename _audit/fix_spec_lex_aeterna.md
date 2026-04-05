# Lex Aeterna Fix Specification

**Generated**: 2026-03-23
**Scope**: 20 missing LA checks found in Round 1 audit
**Priority**: HIGH (skill accuracy), affects all classes

---

## 1. Canonical LA Pattern (Reference)

Source: `executePhysicalSkillOnEnemy()`, lines 1947-1954:

```js
// Lex Aeterna: double damage on first hit, then consume
if (result.damage > 0 && enemy.activeBuffs) {
    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
    if (lexBuff) {
        result.damage = result.damage * 2;
        removeBuff(enemy, 'lex_aeterna');
        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
    }
}
```

**Pattern placement**: After `calculateSkillDamage()` returns and the `!isMiss` guard, BEFORE `enemy.health = Math.max(0, enemy.health - damage)`.

**Key rules** (rAthena pre-renewal `battle_calc_damage`):
- LA doubles the FIRST damaging hit, then is consumed
- LA does NOT apply to: MISS, 0 damage, self-damage, reflect damage, DoT ticks
- Multi-hit skills: LA doubles only the FIRST hit, not all hits
- Works on both enemies and players (PvP targets)
- MISC damage type (Throw Stone, traps): LA still applies in pre-renewal (rAthena `battle_calc_misc_attack` checks SC_AETERNA)

---

## 2. Recommended Approach: Option A (Individual Inline Checks)

**Rejected alternatives:**
- **Option B** (route through `executePhysicalSkillOnEnemy`): Not feasible. Many handlers (Bash, Magnum Break, Arrow Shower, Envenom) have custom damage formulas, AoE loops, multi-target PvP paths, or split/bonus damage that doesn't fit the standard single-target flow.
- **Option C** (add to `calculateSkillDamage`/`calculatePhysicalDamage`): These are pure functions that return results without side effects. LA requires `removeBuff()` + `broadcastToZone()` side effects and entity state mutation. Adding it here would violate the function's contract and create hidden coupling.

**Option A** is the established codebase pattern (40+ existing inline LA checks). Each handler gets an inline check after damage calculation, before HP subtraction.

---

## 3. Per-Path Analysis and Fix Code

### 3.1 Bash (ID 101) — HIGH

**File**: `server/src/index.js`
**Handler start**: Line 9599
**Damage application**: Line 9688 (`target.health = Math.max(0, target.health - bashDmg)`)
**Supports PvP**: Yes (enemy and player targets)
**Multi-hit**: No (single hit)
**Current state**: No LA check anywhere in the handler

**RO Classic rule**: YES, Bash should check LA. Standard single-hit physical skill.

**Fix**: Insert after line 9685 (Energy Coat), before line 9688 (damage application):

```js
            // Lex Aeterna: double damage on first hit, then consume
            if (bashDmg > 0 && target.activeBuffs) {
                const lexBuff = target.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                if (lexBuff) {
                    bashDmg = bashDmg * 2;
                    removeBuff(target, 'lex_aeterna');
                    broadcastToZone(bashZone, 'skill:buff_removed', { targetId, isEnemy, buffName: 'lex_aeterna', reason: 'consumed' });
                }
            }
```

**Insert after**: `bashDmg = applyEnergyCoat(target, bashDmg, targetId, bashZone);` block (line 9685)
**Insert before**: `target.health = Math.max(0, target.health - bashDmg);` (line 9688)

Note: Uses `bashDmg` (the mutable damage variable), `target` (works for both enemy and player), `isEnemy` (already in scope).

---

### 3.2 Magnum Break (ID 102) — HIGH

**File**: `server/src/index.js`
**Handler start**: Line 9867
**Damage application (enemies)**: Line 9928 (`enemy.health = Math.max(0, enemy.health - damage)`)
**Damage application (PvP)**: Line 9998 (`ptarget.health = Math.max(0, ptarget.health - pvpMbDmg)`)
**Multi-hit**: No (single hit per target, AoE)
**Current state**: No LA check

**RO Classic rule**: YES. AoE physical skill. LA should apply to the FIRST target hit that has the debuff — since this is AoE, each enemy with LA gets it consumed individually.

**Fix (enemy path)**: Insert after line 9925 (`continue` after miss), before line 9927:

```js
                // Lex Aeterna: double damage on first hit, then consume
                if (damage > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        mbResult.damage = damage * 2;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(mbZone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Wait -- the `damage` here is destructured from `mbResult`. We need a mutable variable. Looking at the code: line 9907 destructures `{ damage, isCritical, isMiss, hitType: mbHitType }`. This means `damage` is a local `const` from destructuring.

**Corrected fix (enemy path)**: Insert after line 9925 (miss-continue), before line 9927. Need to use a mutable variable:

```js
                // Lex Aeterna: double damage on first hit, then consume
                let mbDmg = damage;
                if (mbDmg > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        mbDmg = mbDmg * 2;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(mbZone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Then update lines 9927-9929 to use `mbDmg` instead of `damage`:

```js
                enemy._lastDamageDealt = mbDmg;
                enemy.health = Math.max(0, enemy.health - mbDmg);
                totalDamageDealt += mbDmg;
```

And update the broadcast at line 9950: `damage` -> `mbDmg` (or `damage: mbDmg`).

**Fix (PvP path)**: Insert after line 9996 (miss-continue), before line 9998. Same pattern using `pvpMbDmg`:

```js
                // Lex Aeterna: double damage on first hit, then consume (PvP)
                let pvpMbDmgFinal = pvpMbDmg;
                if (pvpMbDmgFinal > 0 && ptarget.activeBuffs) {
                    const lexBuff = ptarget.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        pvpMbDmgFinal = pvpMbDmgFinal * 2;
                        removeBuff(ptarget, 'lex_aeterna');
                        broadcastToZone(mbZone, 'skill:buff_removed', { targetId: pid, isEnemy: false, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Then update line 9998: `ptarget.health = Math.max(0, ptarget.health - pvpMbDmgFinal);` and line 10008+10014.

---

### 3.3 Double Strafe (ID 303) — HIGH

**File**: `server/src/index.js`
**Handler start**: Line 12182
**Damage application**: Line 12218 (`enemy.health = Math.max(0, enemy.health - result.damage)`)
**Multi-hit**: Visually 2 hits but BUNDLED as single damage packet (line 12208: `totalMultiplier = effectVal * 2`)
**Current state**: No LA check

**RO Classic rule**: YES. Bundled damage skill. LA doubles the single bundled damage packet.

**Fix**: Insert after line 12216 (`if (!result.isMiss) {`), before line 12217:

```js
                // Lex Aeterna: double damage on first hit, then consume
                if (result.damage > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        result.damage = result.damage * 2;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(dsZone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

---

### 3.4 Arrow Shower (ID 304) — HIGH

**File**: `server/src/index.js`
**Handler start**: Line 12244
**Damage application**: Line 12271 (`enemy.health = Math.max(0, enemy.health - result.damage)`)
**Multi-hit**: No (single hit per target, AoE)
**Current state**: No LA check

**RO Classic rule**: YES. AoE ranged physical skill. Each target with LA gets it consumed.

**Fix**: Insert after line 12269 (`if (!result.isMiss) {`), before line 12270:

```js
                    // Lex Aeterna: double damage on first hit, then consume
                    if (result.damage > 0 && enemy.activeBuffs) {
                        const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                        if (lexBuff) {
                            result.damage = result.damage * 2;
                            removeBuff(enemy, 'lex_aeterna');
                            broadcastToZone(asZone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                        }
                    }
```

---

### 3.5 Back Stab (ID 1701) — HIGH

**File**: `server/src/index.js`
**Handler start**: Line 19499
**Damage application**: Line 19544 (`enemy.health = Math.max(0, enemy.health - hitDamage)`)
**Multi-hit**: YES — 2 hits with daggers, 1 hit otherwise (line 19525: `numHits = isDagger ? 2 : 1`)
**Current state**: No LA check

**RO Classic rule**: YES. Physical skill. For multi-hit (dagger), LA should double ONLY the first hit per rAthena.

**Fix**: Insert after line 19542 (`const hitDamage = result.damage;`), before line 19543. Use a flag to track if LA was consumed on the first hit:

Before the `for` loop (after line 19539), add:

```js
            let bsLexConsumed = false;
```

Then inside the loop, after line 19542 and before line 19543:

```js
                // Lex Aeterna: double FIRST hit only, then consume
                let bsHitDmg = hitDamage;
                if (!bsLexConsumed && bsHitDmg > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        bsHitDmg = bsHitDmg * 2;
                        bsLexConsumed = true;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Then update line 19543-19544 to use `bsHitDmg`:

```js
                totalDamage += bsHitDmg;
                enemy.health = Math.max(0, enemy.health - bsHitDmg);
```

And update the broadcast at line 19551: `damage: bsHitDmg`.

---

### 3.6 Raging Trifecta Blow / Triple Attack (ID 1603) — HIGH

**File**: `server/src/index.js`
**Handler location**: Lines 25590-25656 (auto-attack tick passive proc)
**Damage application**: Line 25618 (`enemy.health = Math.max(0, enemy.health - taTotalDmg)`)
**Multi-hit**: YES — 3 hits
**Current state**: No LA check

**RO Classic rule**: YES — but with a critical caveat. In the current codebase, Triple Attack fires AFTER the normal auto-attack has already dealt damage and consumed LA (line 25072-25079). So the normal auto-attack eats the LA before Triple Attack even starts. This is actually a DESIGN BUG: in rAthena, Triple Attack REPLACES the normal auto-attack (it's not additional damage on top).

However, fixing the replacement behavior is a separate bug. For the LA fix specifically: if the normal auto-attack path is later fixed to not fire when TA procs, then TA needs its own LA check.

**Fix**: Insert before line 25618, using a flag for first-hit-only:

```js
                        // Lex Aeterna: double FIRST hit only, then consume
                        let taLexConsumed = false;
                        for (let ti = 0; ti < taHits.length; ti++) {
                            if (!taLexConsumed && taHits[ti].damage > 0 && enemy.activeBuffs) {
                                const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                                if (lexBuff) {
                                    taHits[ti].damage = taHits[ti].damage * 2;
                                    taTotalDmg = taHits.reduce((sum, h) => sum + h.damage, 0);
                                    taLexConsumed = true;
                                    removeBuff(enemy, 'lex_aeterna');
                                    broadcastToZone(atkEnemyZone, 'skill:buff_removed', { targetId: enemy.enemyId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                                }
                            }
                        }
```

**Note**: This fix should be paired with a TODO comment noting that the normal auto-attack should NOT fire when TA procs (separate bug). Until that's fixed, LA will be consumed by the normal auto-attack hit at line 25072, so this TA LA check will never trigger (harmless but correct for future).

---

### 3.7 Arrow Repel (ID 306) — MEDIUM

**File**: `server/src/index.js`
**Handler start**: Line 12358
**Damage application**: Line 12390 (`enemy.health = Math.max(0, enemy.health - result.damage)`)
**Multi-hit**: No
**Current state**: No LA check

**RO Classic rule**: YES. Standard single-target ranged physical skill.

**Fix**: Insert after line 12388 (`if (!result.isMiss) {`), before line 12389:

```js
                // Lex Aeterna: double damage on first hit, then consume
                if (result.damage > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        result.damage = result.damage * 2;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(arZone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

---

### 3.8 Envenom (ID 500) — MEDIUM

**File**: `server/src/index.js`
**Handler start**: Line 12600
**Damage application**: Line 12655 (`enemy.health = Math.max(0, enemy.health - totalDamage)`)
**Multi-hit**: No
**Current state**: No LA check

**RO Classic rule**: YES. Physical skill with bonus flat damage. LA should double the `totalDamage` (ATK portion + flat bonus).

**Fix**: Insert after line 12651 (`totalDamage = Math.max(1, totalDamage);`), before line 12653:

```js
            // Lex Aeterna: double damage on first hit, then consume
            if (totalDamage > 0 && enemy.activeBuffs) {
                const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                if (lexBuff) {
                    totalDamage = totalDamage * 2;
                    removeBuff(enemy, 'lex_aeterna');
                    broadcastToZone(player.zone || 'prontera_south', 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                }
            }
```

---

### 3.9 Heal vs Undead — MEDIUM

**File**: `server/src/index.js`
**Handler start**: Line 11318
**Damage application**: Line 11347 (`enemy.health = Math.max(0, enemy.health - holyDamage)`)
**Multi-hit**: No
**Current state**: No LA check

**RO Classic rule**: YES. Heal offensive path deals holy damage to Undead. In rAthena, Heal damage is affected by LA (`battle_calc_magic_attack` handles SC_AETERNA for all BF_MAGIC including heal damage).

**Fix**: Insert after line 11344 (`const holyDamage = ...`), before line 11346:

```js
                // Lex Aeterna: double damage on first hit, then consume
                let healDmg = holyDamage;
                if (healDmg > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        healDmg = healDmg * 2;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(player.zone || 'prontera_south', 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Then update lines 11346-11347 to use `healDmg`:

```js
                enemy._lastDamageDealt = healDmg;
                enemy.health = Math.max(0, enemy.health - healDmg);
```

And update the broadcast at line 11356: `damage: healDmg`.

---

### 3.10 Turn Undead (fail path) — MEDIUM

**File**: `server/src/index.js`
**Handler start**: Line 17375
**Damage application (fail)**: Line 17425 (`tuEnemy.health = Math.max(0, tuEnemy.health - tuDmg)`)
**Damage application (success)**: Line 17407 (instant kill — N/A)
**Multi-hit**: No
**Current state**: No LA check

**RO Classic rule**: PARTIAL. The fail path deals holy piercing damage. In rAthena, this damage IS affected by LA. The instant-kill success path is not damage-based (it sets HP to 0), so LA is irrelevant there. However, the instant kill success path SHOULD consume LA without doubling (since the target dies anyway, LA just gets wasted).

**Fix (fail path)**: Insert after line 17424 (`const tuDmg = ...`), before line 17425:

```js
                // Lex Aeterna: double damage on first hit, then consume
                let tuFinalDmg = tuDmg;
                if (tuFinalDmg > 0 && tuEnemy.activeBuffs) {
                    const lexBuff = tuEnemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        tuFinalDmg = tuFinalDmg * 2;
                        removeBuff(tuEnemy, 'lex_aeterna');
                        broadcastToZone(tuZone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Then update line 17425: `tuEnemy.health = Math.max(0, tuEnemy.health - tuFinalDmg);`
And update broadcast at line 17437: `damage: tuFinalDmg`.

**Fix (success path)**: No change needed. Instant kill sets HP=0 regardless. LA is consumed by any subsequent death processing or becomes irrelevant.

---

### 3.11 Ki Explosion Splash (ID 1608) — MEDIUM

**File**: `server/src/index.js`
**Handler start**: Line 18499
**Primary target**: Line 18504 — goes through `executePhysicalSkillOnEnemy` which HAS LA check. OK.
**Splash damage application**: Line 18523 (`enemy.health = Math.max(0, enemy.health - keSplashResult.damage)`)
**Multi-hit**: No (single hit per splash target)
**Current state**: Primary target has LA (via executePhysicalSkillOnEnemy). Splash targets do NOT.

**RO Classic rule**: YES. AoE splash damage should check LA on each splash target individually.

**Fix**: Insert after line 18522 (`if (!keSplashResult.isMiss) {`), before line 18523:

```js
                        // Lex Aeterna: double splash damage, then consume
                        if (keSplashResult.damage > 0 && enemy.activeBuffs) {
                            const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                            if (lexBuff) {
                                keSplashResult.damage = keSplashResult.damage * 2;
                                removeBuff(enemy, 'lex_aeterna');
                                broadcastToZone(keZone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                            }
                        }
```

---

### 3.12 Raid (ID 1703) — MEDIUM

**File**: `server/src/index.js`
**Handler start**: Line 19568
**Damage application**: Line 19597 (`if (hitDamage > 0) enemy.health = Math.max(0, enemy.health - hitDamage)`)
**Multi-hit**: No (single hit per target, AoE)
**Current state**: No LA check

**RO Classic rule**: YES. AoE physical skill.

**Fix**: Insert after line 19596 (`const hitDamage = result.isMiss ? 0 : result.damage;`), before line 19597:

```js
                // Lex Aeterna: double damage on first hit, then consume
                let raidDmg = hitDamage;
                if (raidDmg > 0 && enemy.activeBuffs) {
                    const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        raidDmg = raidDmg * 2;
                        removeBuff(enemy, 'lex_aeterna');
                        broadcastToZone(zone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

Then update line 19597: `if (raidDmg > 0) enemy.health = Math.max(0, enemy.health - raidDmg);`
And update broadcast at line 19605: `damage: raidDmg`.

---

### 3.13 Intimidate (ID 1707) — MEDIUM

**NOTE**: The audit lists ID 1707 but the skill name `intimidate` is at ID 1704 in the handler. ID 1707 is `sightless_mind` (Rogue). The handler we need is `intimidate` at line 19641.

**File**: `server/src/index.js`
**Handler start**: Line 19641
**Damage application**: Line 19655 (`if (hitDamage > 0) enemy.health = Math.max(0, enemy.health - hitDamage)`)
**Multi-hit**: No
**Current state**: No LA check

**RO Classic rule**: YES. Standard single-target physical skill.

**Fix**: Insert after line 19654 (`const hitDamage = result.isMiss ? 0 : result.damage;`), before line 19655:

```js
            // Lex Aeterna: double damage on first hit, then consume
            let intDmg = hitDamage;
            if (intDmg > 0 && enemy.activeBuffs) {
                const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                if (lexBuff) {
                    intDmg = intDmg * 2;
                    removeBuff(enemy, 'lex_aeterna');
                    broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                }
            }
```

Then update line 19655: `if (intDmg > 0) enemy.health = Math.max(0, enemy.health - intDmg);`
And update broadcast at line 19663: `damage: intDmg`.

---

### 3.14 PvP Auto-Attack — MEDIUM

**File**: `server/src/index.js`
**Handler location**: Lines 26050-26250 (combat tick, player target path)
**Damage application**: Line 26188 (`target.health = Math.max(0, target.health - pvpDmg)`)
**Multi-hit**: No (single hit per tick)
**Current state**: No LA check. The PvE auto-attack path HAS LA (line 25072), but PvP path does not.

**RO Classic rule**: YES. Auto-attacks check LA regardless of target type.

**Fix**: Insert after line 26186 (after the miss-continue block), before line 26188:

```js
            // Lex Aeterna: double damage on first hit, then consume (PvP auto-attack)
            let pvpFinalDmg = pvpDmg;
            if (pvpFinalDmg > 0 && target.activeBuffs) {
                const lexBuff = target.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                if (lexBuff) {
                    pvpFinalDmg = pvpFinalDmg * 2;
                    removeBuff(target, 'lex_aeterna');
                    broadcastToZone(pvpAtkZone, 'skill:buff_removed', { targetId: atkState.targetCharId, isEnemy: false, buffName: 'lex_aeterna', reason: 'consumed' });
                }
            }
```

Then update line 26188: `target.health = Math.max(0, target.health - pvpFinalDmg);`
And update `damagePayload.damage` (after line 26189 assignment): `damagePayload.damage = pvpFinalDmg;` or set before payload construction.

**Note**: `pvpDmg` is destructured as a const at line 26160. Need the mutable `pvpFinalDmg` approach.

---

### 3.15 Sonic Blow (ID 1107) — ALREADY HAS LA

**File**: `server/src/index.js`
**Handler start**: Line 16577
**LA check**: Lines 16626-16633 (confirmed present and correct)
**Current state**: ALREADY IMPLEMENTED. No fix needed.

The audit item "Sonic Blow splash" was misleading. Sonic Blow is a single-target 8-hit skill with NO splash mechanic. The 8 visual hits are cosmetic (lines 16658-16672, `damage: 0`). The actual damage is a single calc applied once (line 16635). LA is correctly checked against this single damage packet.

**Verdict**: SKIP. No fix needed.

---

### 3.16 Venom Splasher Detonation (ID 1110) — MEDIUM

**File**: `server/src/index.js`
**Handler location**: Lines 24818-24894 (combat tick detonation loop)
**Damage application**: Line 24872 (`target.health = Math.max(0, target.health - vsResult.damage)`)
**Multi-hit**: No (single hit per target, AoE splash)
**Current state**: No LA check

**RO Classic rule**: YES. AoE physical skill damage. Each target with LA gets it consumed.

**Fix**: Insert after line 24871 (`if (!vsResult.isMiss) {`), before line 24872:

```js
                // Lex Aeterna: double damage on first hit, then consume
                if (vsResult.damage > 0 && target.activeBuffs) {
                    const lexBuff = target.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                    if (lexBuff) {
                        vsResult.damage = vsResult.damage * 2;
                        removeBuff(target, 'lex_aeterna');
                        broadcastToZone(vsZone, 'skill:buff_removed', { targetId: tid, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                    }
                }
```

---

### 3.17 Throw Stone (ID 1) — LOW

**File**: `server/src/index.js`
**Handler start**: Line 12799
**Damage application**: Line 12840 (`enemy.health = Math.max(0, enemy.health - stoneDamage)`)
**Multi-hit**: No
**Current state**: No LA check

**RO Classic rule**: YES. In rAthena pre-renewal, MISC damage is affected by LA (`battle_calc_misc_attack` checks SC_AETERNA). Throw Stone deals fixed 50 MISC damage.

**Fix**: Insert after line 12837 (`const stoneDamage = 50;`), before line 12839:

```js
            // Lex Aeterna: double damage on first hit, then consume
            let tsDmg = stoneDamage;
            if (enemy.activeBuffs) {
                const lexBuff = enemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
                if (lexBuff) {
                    tsDmg = tsDmg * 2;
                    removeBuff(enemy, 'lex_aeterna');
                    broadcastToZone(player.zone || 'prontera_south', 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
                }
            }
```

Then update lines 12839-12840:

```js
            enemy._lastDamageDealt = tsDmg;
            enemy.health = Math.max(0, enemy.health - tsDmg);
```

And update broadcast at line 12857: `damage: tsDmg`.

---

## 4. Paths That Should NOT Check LA

These were listed in the audit but should be verified as correct exclusions:

### 4.1 DoT Ticks (Poison, Bleeding, etc.)
- DoT ticks should NOT check LA. In rAthena, `status_change_timer` does not check SC_AETERNA.
- Current behavior: Correct (no LA check in DoT tick handlers).

### 4.2 Reflect Damage (Kaite, Magic Rod)
- Reflected damage should NOT check LA on the reflector. LA only applies to offensive actions, not reflections.
- Current behavior: Correct.

### 4.3 Grand Cross Self-Damage
- Self-damage from Grand Cross should NOT check LA. In rAthena, self-damage path skips SC_AETERNA.
- Current behavior: Correct (line 13912 only checks LA on enemy targets in the GC AoE loop).

---

## 5. Multi-Hit Verification Summary

Skills that deal multiple hits and need "first-hit-only" LA logic:

| Skill | Hits | Handler | Fix Pattern |
|-------|------|---------|-------------|
| Back Stab | 1-2 (dagger=2) | Line 19540 loop | `bsLexConsumed` flag |
| Triple Attack | 3 | Line 25606 loop | `taLexConsumed` flag + recalculate total |
| Sonic Blow | 8 visual, 1 actual | Line 16620 | ALREADY HAS LA (single calc) |
| Chain Combo | 4 | Line 18595 loop | ALREADY HAS LA (via existing check) |

All other affected skills are single-hit (per target) and need standard inline LA check.

---

## 6. Final Implementation Checklist

| # | Path | Priority | Lines | Fix Type | Multi-Hit |
|---|------|----------|-------|----------|-----------|
| 1 | Bash (enemy+PvP) | HIGH | ~9685 | Inline, mutable var | No |
| 2 | Magnum Break (enemy) | HIGH | ~9925 | Inline, mutable var | No (AoE) |
| 3 | Magnum Break (PvP) | HIGH | ~9996 | Inline, mutable var | No (AoE) |
| 4 | Double Strafe | HIGH | ~12216 | Inline, `result.damage` | No (bundled) |
| 5 | Arrow Shower | HIGH | ~12269 | Inline, `result.damage` | No (AoE) |
| 6 | Back Stab | HIGH | ~19542 | Flag + mutable var | YES (2 hits) |
| 7 | Triple Attack | HIGH | ~25617 | Flag + recalc | YES (3 hits) |
| 8 | Arrow Repel | MEDIUM | ~12388 | Inline, `result.damage` | No |
| 9 | Envenom | MEDIUM | ~12651 | Inline, `totalDamage` | No |
| 10 | Heal vs Undead | MEDIUM | ~11344 | Mutable var | No |
| 11 | Turn Undead (fail) | MEDIUM | ~17424 | Mutable var | No |
| 12 | Ki Explosion splash | MEDIUM | ~18522 | Inline, `keSplashResult` | No (AoE) |
| 13 | Raid | MEDIUM | ~19596 | Mutable var | No (AoE) |
| 14 | Intimidate | MEDIUM | ~19654 | Mutable var | No |
| 15 | PvP auto-attack | MEDIUM | ~26186 | Mutable var | No |
| 16 | Venom Splasher det. | MEDIUM | ~24871 | Inline, `vsResult` | No (AoE) |
| 17 | Throw Stone | LOW | ~12837 | Mutable var | No |

**Total fixes needed: 17** (Sonic Blow already has LA, 2 Magnum Break paths count separately, Bash enemy+PvP in same handler)

**Sonic Blow**: SKIP (already implemented at lines 16626-16633)
**Adjusted count from original 20**: 3 items were either already fixed (Sonic Blow), duplicates (Magnum Break counted once as enemy+PvP), or misidentified.

---

## 7. Testing Strategy

After implementation, test with these scenarios:

1. **Basic check**: Cast Lex Aeterna on target, use each skill. Verify damage is 2x and buff is consumed.
2. **Miss check**: Cast LA, then miss with Bash. Verify LA is NOT consumed (0 damage).
3. **Multi-hit check (Back Stab)**: Use dagger Back Stab on LA target. Verify first hit is 2x, second hit is normal.
4. **AoE check (Magnum Break)**: LA on one enemy, none on another. Only the LA target gets 2x.
5. **PvP check**: LA on player target, auto-attack. Verify 2x damage and consumption.
6. **MISC check (Throw Stone)**: LA on target, throw stone. Verify 100 damage (50*2).
7. **No double-dip**: Ensure LA is consumed after first hit and not applied again on subsequent hits in the same skill execution.
