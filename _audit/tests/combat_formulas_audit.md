# Combat Damage Formulas Audit

Generated: 2026-03-22
Source: `server/src/ro_damage_formulas.js` (primary), `server/src/index.js` (wrappers + combat tick)

---

## 1. File Map and Function Index

### `ro_damage_formulas.js` (1080 lines) — Pure formula module

| Function | Lines | Purpose |
|----------|-------|---------|
| `calculateMaxHP()` | 181-190 | Class-aware MaxHP: BaseHP formula + VIT scaling + transcendent 1.25x |
| `calculateMaxSP()` | 197-203 | Class-aware MaxSP: linear BaseSP + INT scaling + transcendent 1.25x |
| `calculateASPD()` | 219-256 | Weapon-type ASPD with dual wield, mount penalty |
| `calculateDerivedStats()` | 267-348 | StatusATK, MATK min/max, HIT, FLEE, CRI, PD, softDEF, softMDEF, ASPD, MaxHP, MaxSP |
| `getElementModifier()` | 361-366 | 10x10x4 element table lookup |
| `getSizePenalty()` | 378-381 | Weapon-type vs target-size penalty lookup |
| `calculateHitRate()` | 394-410 | HIT vs FLEE with multi-attacker penalty, hitRatePercent bonus |
| `calculateCritRate()` | 422-426 | Attacker CRI - target LUK shield |
| `calculatePhysicalDamage()` | 440-905 | **MAIN** physical damage pipeline (10+ steps) |
| `calculateMagicalDamage()` | 919-1054 | **MAIN** magical damage pipeline |

### `index.js` — Wrappers and Combat Integration

| Function | Lines | Purpose |
|----------|-------|---------|
| `calculateSkillDamage()` | 1814-1821 | Wrapper: calls `calculatePhysicalDamage` with `isSkill:true` |
| `calculateMagicSkillDamage()` | 1824-1833 | Wrapper: calls `calculateMagicalDamage` (roMagicalDamage) |
| `calculatePhysicalDamage()` | 2578-2628 | Bridge: builds attacker/target objects, delegates to `roPhysicalDamage()` |
| `calculateEnemyDamage()` | 29878-29913 | Enemy-vs-player auto-attack (isNonElemental:true) |
| `calculateHealAmount()` | 1859-1870 | RO Classic Heal formula |
| `getEffectiveStats()` | 3781-3832 | Aggregates base + equip + card + buff + passive + pet stats |
| `getAttackerInfo()` | 3835-3893 | Builds attacker info with weapon, cards, refine, ammo |
| `getEnemyTargetInfo()` | 3896-3913 | Enemy target info with element, size, race, numAttackers |
| `getPlayerTargetInfo()` | 3916-3950 | Player target info with element resist, card DEF mods |
| `getAttackIntervalMs()` | 456-474 | ASPD to attack interval conversion |
| `checkMagicRodAbsorption()` | 1837-1856 | Magic Rod absorption check |
| `getPassiveSkillBonuses()` | 641-863 | All passive skill bonuses (mastery, race ATK/DEF, etc.) |
| `getCombinedModifiers()` | 1704-1806 | Merges status effect mods + buff mods |
| `getBuffStatModifiers()` | 1809-1811 | Alias for getCombinedModifiers |

---

## 2. Element Table (ro_damage_formulas.js:26-147)

10 attack elements x 10 defense elements x 4 defense levels. ELEMENT_TABLE[atkEle][defEle][defLv-1].

Key values verified against rAthena `db/pre-re/attr_fix.yml`:
- Neutral vs Ghost: [25, 25, 0, 0] (75-100% reduction)
- Fire vs Water Lv1: 50% (resistant)
- Fire vs Undead Lv4: 200% (super effective)
- Holy vs Shadow Lv4: 200%
- Holy vs Holy Lv4: -100% (heals target)
- Poison vs Poison: [0,0,0,0] (immune)
- Poison vs Undead: [-25,-50,-75,-100] (heals)
- Shadow vs Shadow Lv4: -100%
- Ghost vs Ghost: [125,150,175,200] (super effective)
- Undead vs Undead: [0,0,0,0] (immune)
- Undead vs Shadow: [0,0,0,0] (immune)

**Finding**: The table matches rAthena pre-renewal canonical values.

---

## 3. Size Penalty Table (ro_damage_formulas.js:153-174)

16 weapon types defined. Values match rAthena:
- Dagger: 100/75/50 (S/M/L)
- 1H Sword: 75/100/75
- Bow: 100/100/75
- Katar: 75/100/75
- Bare hand/Rod/Staff: 100/100/100

**Finding**: `fist` and `knuckle` are duplicated (both 100/75/50). Correct per rAthena.

---

## 4. Physical Damage Pipeline (ro_damage_formulas.js:440-905)

### Step 1: Perfect Dodge (line 481-492)
- PD from `calculateDerivedStats()` = 1 + floor(LUK/10) + bonusPD
- Bypassed by `forceHit` or `forceCrit`
- Roll: `Math.random()*100 < pd`
- Returns immediately with `hitType:'perfectDodge'`, `isMiss:true`

### Step 2: Critical Check (line 500-521)
- `forceCrit` flag forces critical (Auto Counter, Sharp Shooting)
- Natural crits: **only auto-attacks** (`!isSkill` guard at line 503)
- Card crit race bonus (`bCriticalAddRace`) added per target race
- Card ranged crit bonus (`bCriticalLong`) added for bow/gun
- Effective crit = `calculateCritRate(attackerCri + extraCrit, targetLuk)`
- CritShield = `floor(targetLUK * 0.2)`
- Roll: `Math.random()*100 < effectiveCrit`

### Step 3: Hit/Miss (line 530-543)
- Skipped for criticals and `forceHit`
- Applies buff modifiers: `hitMultiplier`, `fleeMultiplier` (Blind: 0.75 each)
- Multi-attacker FLEE penalty: -10 per attacker beyond 2
- Hit rate formula: `80 + HIT - effectiveFLEE`, clamped 5-95%
- `hitRatePercent` bonus (Bash, Magnum Break): `hitRate * (100 + hitRatePercent) / 100`

### Step 4: ATK Calculation (line 546-627)

**StatusATK** (from `calculateDerivedStats`):
- Melee: `STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/5)`
- Ranged: `DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/5)`

**WeaponATK with variance** (line 564-597):
1. Size penalty applied to weaponATK: `sizedWeaponATK = floor(weaponATK * sizePenalty / 100)`
2. Stat bonus: `weaponStatBonus = floor(sizedWeaponATK * primaryStat / 200)`
3. `atkMax = sizedWeaponATK + weaponStatBonus`
4. `atkMin = min(floor(secondaryStat * (0.8 + 0.2*WL)), sizedWeaponATK)`
5. Critical/MaxPower: always use atkMax
6. Normal: random between atkMin and atkMax

**ArrowATK** (line 599-612):
- Critical/MaxPower: full arrowATK added
- Normal: `random(0, arrowATK-1)` variance
- Subject to size penalty: `floor(arrowContrib * sizePenalty / 100)`

**totalATK = statusATK + variancedWeaponATK** (line 615)

**Critical damage bonus** (line 618-622):
- `+40%` base + `equipCritAtkRate` (e.g., Muramasa)
- `totalATK = floor(totalATK * (100 + 40 + critAtkRate) / 100)`

**Buff ATK multiplier** (line 625-626):
- Provoke/Auto Berserk atkMultiplier applied

### Step 5: Skill Multiplier (line 631-633)
- `totalATK = floor(totalATK * skillMultiplier / 100)` when isSkill and skillMult != 100

### Step 5b: Card bSkillAtk (line 638-641)
- Per-skill damage % bonus (e.g., +15% Bash from a specific card)

### Step 6: Card Modifiers (line 648-701)
- **Race, element, size**: additive within category, multiplicative between
- `totalATK = floor(totalATK * (100 + raceBonus) / 100)` etc.
- Passive race ATK (Demon Bane): flat additive after card multipliers (line 661-666)
- Boss/Normal class bonus (line 671-675): Abysmal Knight, Turtle General
- Sub-race bonus (line 680-683): Goblin/Orc
- Specific monster bonus (line 688-691): Crab Card, Aster Card
- Ranged ATK bonus (line 696-701): Archer Skeleton Card for bow/gun

### Step 7: Element Modifier (line 711-727)
- Computed but **deferred** to Step 8i
- `isNonElemental`: monster auto-attacks bypass element table entirely (always 100%)
- Element <= 0: immune (return damage 0)

### Step 8: DEF Reduction (line 730-805)

**Hard DEF** (line 734-777):
- Steel Body override: `overrideHardDEF` replaces equipment DEF
- Capped at 99
- Angelus: `defPercent` bonus multiplied into soft DEF
- Strip shield: `hardDefReduction` reduces raw hard DEF
- `totalATK = floor(totalATK * (100 - hardDef) / 100)`

**Soft DEF** (line 746, 778):
- `softDEF = floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)`
- Angelus multiplier, vitMultiplier (strip armor), softDefMultiplier (provoke), ensemble VIT zero
- Flat subtraction: `totalATK = totalATK - effectiveSoftDef`

**DEF bypass** (line 749-767):
- `ignoreDefense` option (Auto Counter)
- `bIgnoreDefClass` card (Samurai Spector) — skips both hard + soft
- `bDefRatioAtkClass` card (Thanatos/Ice Pick) — converts DEF into bonus: `ATK * combinedDEF / 100`

**Passive race DEF** (line 782-787): Divine Protection, flat subtraction, minimum 1

### Post-DEF Bonuses (line 789-811):
- **Refine ATK**: flat `refATK` added, excluded for Shield Boomerang/Acid Terror/Investigate/Asura Strike (IDs 1305, 1801, 1606, 1605)
- **Overupgrade**: random `1 to overrefineMax` added per hit
- **Mastery ATK**: passiveATK added after DEF

### Step 8c: Defensive Card Mods (line 818-825)
- Thara Frog (race), Raydric (element), etc.
- `totalATK = floor(totalATK * (100 - raceRed) / 100)`

### Step 8d: Boss/Normal Class Defense (line 830-834): Alice Card

### Step 8e: Ranged Damage Reduction (line 839-845): Horn Card, Alligator Card

### Step 8f: Monster-specific DEF (line 850-853): flat subtraction

### Step 8g: Sage Zone Boost (line 857-866): Volcano/Deluge/Violent Gale damage %

### Step 8h: Dragonology race resist (line 871-874): defender passive

### Step 8i: Element Modifier Applied (line 880)
- `totalATK = floor(totalATK * eleModifier / 100)` — applied LAST per rAthena order
- Plus element resistance (Skin Tempering): `floor(totalATK * (100 - resistPct) / 100)`

### Step 8j: Raid Debuff (line 892-895): +20% incoming damage final multiplier

### Step 9: Final (line 900-904)
- `damage = max(1, totalATK)` — minimum 1 damage

---

## 5. Magical Damage Pipeline (ro_damage_formulas.js:919-1054)

### GTB Check (line 939-944): `cardNoMagicDamage >= 100` = immune

### MATK Calculation (line 947-949)
- `matkMin = INT + floor(INT/7)^2 + floor(weaponMATK * 0.7)`
- `matkMax = INT + floor(INT/5)^2 + weaponMATK`
- `matk = matkMin + random(0, matkMax - matkMin)`

### Skill Multiplier (line 952): `floor(matk * skillMultiplier / 100)`

### Card Modifiers:
- `bMatkRate` (line 955-957): percentage MATK boost
- Buff ATK multiplier (line 960-961): Provoke etc.
- `bSkillAtk` (line 964-967): per-skill damage bonus
- `bMagicAddRace` (line 970-974): magic race bonus

### Element Modifier (line 977-988): same lookup as physical

### MDEF Reduction (line 991-1022):
- Hard MDEF: `floor(totalDamage * (100 - hardMdef) / 100)`, capped 99
- Card ignore MDEF (`bIgnoreMdefClass`): partial percentage reduction
- Soft MDEF: `INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)`, flat subtraction
- Strip Helm `intMultiplier` reduces INT-based soft MDEF
- Buff bonus MDEF: flat subtraction
- Status MDEF multiplier: freeze/stone = 1.25x incoming magic

### Sage Zone Boost (line 1024-1033): fire/water/wind damage boost

### Dragonology magic ATK (line 1036-1038): +2%/lv vs dragon, attacker passive

### Dragonology resist (line 1041-1044): -4%/lv vs dragon, defender passive

### Raid Debuff (line 1047-1050): +20% final multiplier

### Final: `max(1, totalDamage)` (line 1052)

---

## 6. Derived Stat Formulas (ro_damage_formulas.js:267-348)

| Stat | Formula | Line |
|------|---------|------|
| StatusATK (melee) | `STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/5)` | 289-290 |
| StatusATK (ranged) | `DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/5)` | 289-290 |
| MATK min | `INT + floor(INT/7)^2 + floor(wMATK*0.7)` | 294-296 |
| MATK max | `INT + floor(INT/5)^2 + wMATK` | 295-297 |
| HIT | `175 + BaseLv + DEX + bonusHit` | 301 |
| FLEE | `100 + BaseLv + AGI + bonusFlee` | 305 |
| Critical | `1 + floor(LUK*0.3) + bonusCri` (katar: x2) | 310-311 |
| Perfect Dodge | `1 + floor(LUK/10) + bonusPD` | 315 |
| Soft DEF | `floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)` | 319 |
| Soft MDEF | `INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)` | 323 |
| MaxHP | `(35 + BaseLv*HP_JOB_B + sum(HP_JOB_A*i)) * (1+VIT*0.01) * transMod + bonusHP` | 181-189 |
| MaxSP | `(10 + BaseLv*SP_JOB) * (1+INT*0.01) * transMod + bonusSP` | 197-202 |
| ASPD | `200 - floor((WD - reduction) * (1 - speedMod))` | 219-256 |

---

## 7. ASPD System (ro_damage_formulas.js:219-256)

- Base weapon delay (WD) from class-specific table
- Dual wield: `WD = floor((WD_right + WD_left) * 7 / 10)`
- AGI reduction: `floor(WD * AGI / 25)`
- DEX reduction: `floor(WD * DEX / 100)`
- Total: `floor((agiReduction + dexReduction) / 10)`
- Speed mod: `max(0, buffAspdMultiplier - 1)` (e.g., THQ 1.3 -> 0.3)
- `rawASPD = 200 - floor((WD - totalReduction) * (1 - speedMod))`
- Cap: single 195, dual 190
- Mount penalty: `rawASPD = floor(rawASPD * (0.5 + cavalierMasteryLv * 0.1))`

### Attack Interval (index.js:456-474)
- ASPD <= 195: `(200 - aspd) * 50` ms (ASPD 195 -> 250ms)
- ASPD > 195: diminishing returns with exponential decay, floor 217ms
- ASPD Potion reduction: `max(217, floor(interval * (1 - reduction)))`

---

## 8. Heal Formula (index.js:1858-1870)

`heal = floor((baseLv + INT) / 8) * (4 + skillLevel * 8)`
Then: `heal = floor(heal * (100 + healPower) / 100)` if healPower != 0
Where `healPower = equipHealPower + cardHealPower`

---

## 9. Enemy-vs-Player Damage (index.js:29878-29913)

- Uses `calculatePhysicalDamage()` with `isNonElemental: true`
- Monster auto-attacks bypass element table entirely (always 100%)
- Weapon type inferred from attackRange: bow if > melee+tolerance, else bare_hand
- No card mods on enemies
- Race and size passed from template

---

## 10. Dual Wield System (index.js:25019-25246)

- Right hand: normal damage calc, per-hand card mods (`cardModsRight`)
- Double Attack pre-roll suppresses crits on main hit
- Right hand mastery penalty: `50 + rightHandMasteryLv * 10` percent (50-100%)
- Left hand: `forceHit:true` (always hits), `forceCrit:false` (never crits)
- Left hand mastery penalty: `30 + leftHandMasteryLv * 10` percent (30-80%)
- Left hand uses own weapon ATK, element, level, card mods (`cardModsLeft`)
- Both hands hit per combat tick cycle

---

## 11. Refine System (index.js:3864-3883)

**Refine ATK per level**: `[0, 2, 3, 5, 7]` for weapon level 1/2/3/4
- Weapon Lv1 +7 refine = +14 ATK
- Weapon Lv4 +10 refine = +70 ATK

**Safe limits**: `[0, 7, 6, 5, 4]` for weapon level 1/2/3/4

**Overupgrade bonus table** (random 1..max per hit):
| WLv | +8 | +9 | +10 |
|-----|----|----|-----|
| 1   | 3  | 6  | 9   |
| 2   | 5  | 10 | 15  |  (safe 6, +7 starts)
| 3   | 8  | 16 | 24  |  (safe 5, +6 starts)
| 4   | 13 | 26 | 39  |  (safe 4, +5 starts)

**Excluded skills**: Shield Boomerang (1305), Acid Terror (1801), Investigate (1606), Asura Strike (1605)

---

## 12. Combat-Relevant Buff Modifiers

### From `ro_buff_system.js:655+` (getBuffModifiers):
- Provoke: `softDefMultiplier *= (1 - defReduction/100)`, `atkMultiplier *= (1 + atkIncrease/100)`
- Auto Berserk: same as Provoke Lv10 (atkIncrease=32, softDefMultiplier*=0.45)
- Signum Crucis: `defMultiplier *= (1 - defReduction/100)` (hard DEF)
- Angelus: `defPercent` bonus (VIT% defense)
- Blessing: STR/DEX/INT bonus
- Increase AGI: AGI bonus + move speed
- Decrease AGI: AGI/move speed reduction
- THQ: ASPD increase (Haste2 group, strongest wins), CRI bonus, HIT bonus
- Aspersio: weaponElement = 'holy'
- Enchant Poison: weaponElement = 'poison'
- Maximize Power: maximizePower flag (always max ATK)
- Energy Coat: dynamic phys damage reduction based on SP%
- Defender: ranged reduction
- Auto Guard: block chance
- Reflect Shield: reflect percent

### From `ro_status_effects.js:20+` (STATUS_EFFECTS stat modifiers):
- Freeze: defMultiplier 0.5, mdefMultiplier 1.25, element override water Lv1
- Stone: defMultiplier 0.5, mdefMultiplier 1.25, element override earth Lv1
- Poison: defMultiplier 0.75
- Blind: hitMultiplier 0.75, fleeMultiplier 0.75
- Curse: atkMultiplier 0.75, lukOverride 0, moveSpeed 0.1
- Stun: fleeMultiplier 0

---

## 13. Potential Issues Found

### Issue 1: Element modifier position in magic pipeline
In `calculateMagicalDamage()` (line 988), element modifier is applied BEFORE MDEF reduction, while in `calculatePhysicalDamage()` it is applied AFTER DEF (line 880). The magical order is: MATK * skill% * cardMods * element * MDEF. This differs from rAthena where MDEF is applied before element in pre-renewal magic. However, this is a minor ordering difference that mostly affects edge cases.

### Issue 2: Non-elemental handling asymmetry
`isNonElemental` flag exists for physical damage (line 712-714, monster auto-attacks) but NOT for magical damage. This is correct because all magic spells have an inherent element.

### Issue 3: Passive race ATK is additive, not multiplicative
In physical damage Step 6b (line 661-666), `passiveRaceATK` is a flat additive bonus, while card race bonuses are multiplicative percentages. This matches rAthena behavior where mastery/passive bonuses are flat, card bonuses are percentage.

### Issue 4: Minimum damage floor inconsistency
Physical damage floors at `max(1, totalATK)` at line 900. Magical damage also floors at `max(1, totalDamage)` at line 1052. But the MDEF subtraction (line 1011) and soft DEF subtraction (line 778) can drive totalATK negative before the floor — the floor only applies at the very end, which is correct behavior.

### Issue 5: Double Attack suppresses criticals
At index.js:25058-25062, if DA procs, critical is forced to false. This is correct per rAthena pre-renewal (DA has higher priority than crit).

---

## 14. Cross-Reference Verification

| Feature | Physical | Magical | Enemy Dmg | Notes |
|---------|----------|---------|-----------|-------|
| Lex Aeterna 2x | Yes (index.js:1947) | Not in formula | N/A | LA handled in caller, not formula |
| Card race/ele/size ATK | Yes (line 649-656) | bMagicAddRace (970) | No | Correct: enemies have no cards |
| Card DEF race/ele/size | Yes (line 818-825) | No | N/A | Correct: magic ignores phys DEF cards |
| Refine ATK | Yes (line 796-798) | No | No | Correct: refine is physical only |
| Overupgrade | Yes (line 801-804) | No | No | Correct |
| Size penalty | Yes (line 560-563) | No | No | Correct: magic has no size penalty |
| Element modifier | Yes (line 880) | Yes (line 988) | Bypassed (isNonElemental) | Correct |
| Critical bypass | Yes (line 501-521) | No | Yes (enemy auto-attack can crit) | Correct |
| Perfect Dodge | Yes (line 485-492) | No | Yes (player PD vs enemy) | Correct |
| Dual wield mastery | Yes (index.js:25065-25069) | N/A | N/A | Skills use right-hand only |
| Mount spear 100% medium | Yes (line 559-561) | N/A | N/A | Correct |
| Drake noSizeFix | Yes (line 560) | N/A | N/A | Correct |
| Maximize Power max ATK | Yes (line 574, 585-586) | No | No | Correct: physical only |
| Sage zone boost | Yes (line 860-866) | Yes (line 1027-1033) | No | Correct |
