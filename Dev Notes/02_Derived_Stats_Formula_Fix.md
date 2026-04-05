# Document 2: Derived Stats Formula Corrections

**Dependency**: Document 1 (Server Equipment Pipeline Fix) must be implemented first
**Implements before**: Document 3 (Client UI Updates)

---

## Bugs Fixed

| # | Bug | Impact |
|---|-----|--------|
| 1 | Soft MDEF formula ignores VIT/DEX/Level — only uses INT | Soft MDEF undervalued for high-VIT/DEX chars |
| 2 | `buffBonusMDEF` silently dropped — never applied to softMDEF | MDEF from buffs (Endure) invisible |
| 3 | `matkMin`/`matkMax` not computed in `calculateDerivedStats()` | Client can't display MATK range |
| 4 | `perfectDodge` doesn't include equipment bonus | Equipment PD bonuses have no effect on derived stat |
| 5 | MaxHP/MaxSP use simplified class-agnostic formulas | Swordsman and Mage get identical HP at same VIT |
| 6 | ASPD uses custom sqrt formula instead of weapon-type base delay | ASPD ignores weapon type entirely |

---

## Files Modified

| File | Changes |
|------|---------|
| `server/src/ro_damage_formulas.js` | Rewrite `calculateDerivedStats()` |
| `server/src/ro_exp_tables.js` | Add `HP_SP_COEFFICIENTS`, `ASPD_BASE_DELAYS`, `TRANS_TO_BASE_CLASS` |
| `server/src/index.js` | Update `buildFullStatsPayload()` to apply `buffBonusMDEF` and include new derived fields |

---

## Implementation Steps

---

### Step 1: Add Data Tables to `ro_exp_tables.js`

Add three new exported constants at the bottom of the file, before the `module.exports` line.

**ADD** the following before `module.exports`:

```javascript
// ============================================================
// HP/SP class coefficients (RO pre-renewal)
// BaseHP = 35 + (BaseLv * HP_JOB_B) + sum(round(HP_JOB_A * i) for i=2..BaseLv)
// MaxHP = floor(BaseHP * (1 + VIT * 0.01) * TransMod) + bonusMaxHp
// Same pattern for SP with INT instead of VIT
// ============================================================
const HP_SP_COEFFICIENTS = {
    novice:      { HP_JOB_A: 0.0,  HP_JOB_B: 5,   SP_JOB_A: 0.0,  SP_JOB_B: 2 },
    super_novice:{ HP_JOB_A: 0.0,  HP_JOB_B: 5,   SP_JOB_A: 0.0,  SP_JOB_B: 2 },
    swordsman:   { HP_JOB_A: 0.7,  HP_JOB_B: 5,   SP_JOB_A: 0.2,  SP_JOB_B: 2 },
    mage:        { HP_JOB_A: 0.3,  HP_JOB_B: 5,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    archer:      { HP_JOB_A: 0.5,  HP_JOB_B: 5,   SP_JOB_A: 0.4,  SP_JOB_B: 2 },
    thief:       { HP_JOB_A: 0.5,  HP_JOB_B: 5,   SP_JOB_A: 0.3,  SP_JOB_B: 2 },
    merchant:    { HP_JOB_A: 0.4,  HP_JOB_B: 5,   SP_JOB_A: 0.3,  SP_JOB_B: 2 },
    acolyte:     { HP_JOB_A: 0.4,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    knight:      { HP_JOB_A: 1.5,  HP_JOB_B: 5,   SP_JOB_A: 0.4,  SP_JOB_B: 2 },
    crusader:    { HP_JOB_A: 1.1,  HP_JOB_B: 7,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    wizard:      { HP_JOB_A: 0.55, HP_JOB_B: 5,   SP_JOB_A: 1.0,  SP_JOB_B: 2 },
    sage:        { HP_JOB_A: 0.75, HP_JOB_B: 5,   SP_JOB_A: 0.8,  SP_JOB_B: 2 },
    hunter:      { HP_JOB_A: 0.85, HP_JOB_B: 5,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    bard:        { HP_JOB_A: 0.75, HP_JOB_B: 3,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    dancer:      { HP_JOB_A: 0.75, HP_JOB_B: 3,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    blacksmith:  { HP_JOB_A: 0.9,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    alchemist:   { HP_JOB_A: 0.9,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    assassin:    { HP_JOB_A: 1.1,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    rogue:       { HP_JOB_A: 0.85, HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    priest:      { HP_JOB_A: 0.75, HP_JOB_B: 5,   SP_JOB_A: 0.8,  SP_JOB_B: 2 },
    monk:        { HP_JOB_A: 0.9,  HP_JOB_B: 6.5, SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    // Transcendent classes use same coefficients as base class (TransMod=1.25 applied separately)
    lord_knight:    { HP_JOB_A: 1.5,  HP_JOB_B: 5,   SP_JOB_A: 0.4,  SP_JOB_B: 2 },
    paladin:        { HP_JOB_A: 1.1,  HP_JOB_B: 7,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    high_wizard:    { HP_JOB_A: 0.55, HP_JOB_B: 5,   SP_JOB_A: 1.0,  SP_JOB_B: 2 },
    scholar:        { HP_JOB_A: 0.75, HP_JOB_B: 5,   SP_JOB_A: 0.8,  SP_JOB_B: 2 },
    sniper:         { HP_JOB_A: 0.85, HP_JOB_B: 5,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    minstrel:       { HP_JOB_A: 0.75, HP_JOB_B: 3,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    gypsy:          { HP_JOB_A: 0.75, HP_JOB_B: 3,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    whitesmith:     { HP_JOB_A: 0.9,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    biochemist:     { HP_JOB_A: 0.9,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    assassin_cross: { HP_JOB_A: 1.1,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    stalker:        { HP_JOB_A: 0.85, HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    high_priest:    { HP_JOB_A: 0.75, HP_JOB_B: 5,   SP_JOB_A: 0.8,  SP_JOB_B: 2 },
    champion:       { HP_JOB_A: 0.9,  HP_JOB_B: 6.5, SP_JOB_A: 0.5,  SP_JOB_B: 2 },
    // High Novice and High First classes use same coefficients
    high_novice:    { HP_JOB_A: 0.0,  HP_JOB_B: 5,   SP_JOB_A: 0.0,  SP_JOB_B: 2 },
    high_swordsman: { HP_JOB_A: 0.7,  HP_JOB_B: 5,   SP_JOB_A: 0.2,  SP_JOB_B: 2 },
    high_mage:      { HP_JOB_A: 0.3,  HP_JOB_B: 5,   SP_JOB_A: 0.6,  SP_JOB_B: 2 },
    high_archer:    { HP_JOB_A: 0.5,  HP_JOB_B: 5,   SP_JOB_A: 0.4,  SP_JOB_B: 2 },
    high_thief:     { HP_JOB_A: 0.5,  HP_JOB_B: 5,   SP_JOB_A: 0.3,  SP_JOB_B: 2 },
    high_merchant:  { HP_JOB_A: 0.4,  HP_JOB_B: 5,   SP_JOB_A: 0.3,  SP_JOB_B: 2 },
    high_acolyte:   { HP_JOB_A: 0.4,  HP_JOB_B: 5,   SP_JOB_A: 0.5,  SP_JOB_B: 2 },
};

// ============================================================
// ASPD base weapon delays (BTBA * 50 = WeaponDelay in ticks)
// ASPD = 200 - (WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SpeedMod)
// ============================================================
const ASPD_BASE_DELAYS = {
    novice:     { bare_hand: 50, dagger: 55 },
    swordsman:  { bare_hand: 40, dagger: 65, one_hand_sword: 70, two_hand_sword: 60, spear: 65, mace: 70, axe: 80 },
    mage:       { bare_hand: 35, dagger: 60, staff: 65 },
    archer:     { bare_hand: 50, dagger: 55, bow: 70 },
    thief:      { bare_hand: 40, dagger: 50, one_hand_sword: 70, bow: 85 },
    merchant:   { bare_hand: 40, dagger: 65, one_hand_sword: 55, mace: 65, axe: 70 },
    acolyte:    { bare_hand: 40, dagger: 60, mace: 70, staff: 65 },
    knight:     { bare_hand: 38, dagger: 60, one_hand_sword: 55, two_hand_sword: 50, spear: 55, mace: 60, axe: 70 },
    wizard:     { bare_hand: 35, dagger: 58, staff: 60 },
    hunter:     { bare_hand: 48, dagger: 55, bow: 60 },
    assassin:   { bare_hand: 38, dagger: 45, one_hand_sword: 65, katar: 42 },
    blacksmith: { bare_hand: 38, dagger: 60, one_hand_sword: 52, mace: 60, axe: 62 },
    priest:     { bare_hand: 40, mace: 62, staff: 60, knuckle: 55 },
    crusader:   { bare_hand: 38, dagger: 62, one_hand_sword: 58, two_hand_sword: 55, spear: 58, mace: 62 },
    sage:       { bare_hand: 35, dagger: 58, staff: 60, book: 58 },
    bard:       { bare_hand: 45, dagger: 55, bow: 62, instrument: 58 },
    dancer:     { bare_hand: 45, dagger: 55, bow: 62, whip: 58 },
    rogue:      { bare_hand: 38, dagger: 48, one_hand_sword: 62, bow: 75 },
    monk:       { bare_hand: 36, mace: 60, staff: 62, knuckle: 42 },
    alchemist:  { bare_hand: 38, dagger: 60, one_hand_sword: 52, mace: 60, axe: 62 },
};

// Transcendent class → base class mapping (for HP/SP coefficients and ASPD lookups)
const TRANS_TO_BASE_CLASS = {
    lord_knight:    'knight',
    paladin:        'crusader',
    high_wizard:    'wizard',
    scholar:        'sage',
    sniper:         'hunter',
    minstrel:       'bard',
    gypsy:          'dancer',
    whitesmith:     'blacksmith',
    biochemist:     'alchemist',
    assassin_cross: 'assassin',
    stalker:        'rogue',
    high_priest:    'priest',
    champion:       'monk',
    high_novice:    'novice',
    high_swordsman: 'swordsman',
    high_mage:      'mage',
    high_archer:    'archer',
    high_thief:     'thief',
    high_merchant:  'merchant',
    high_acolyte:   'acolyte',
};

const TRANSCENDENT_CLASSES = new Set([
    'lord_knight', 'paladin', 'high_wizard', 'scholar', 'sniper',
    'minstrel', 'gypsy', 'whitesmith', 'biochemist',
    'assassin_cross', 'stalker', 'high_priest', 'champion'
]);
```

**UPDATE** the `module.exports` to include the new constants:

Find the existing `module.exports` block and add:
```javascript
    HP_SP_COEFFICIENTS,
    ASPD_BASE_DELAYS,
    TRANS_TO_BASE_CLASS,
    TRANSCENDENT_CLASSES,
```

---

### Step 2: Import New Constants in `ro_damage_formulas.js`

At the top of `server/src/ro_damage_formulas.js`, add the import.

**FIND** the existing `require('./ro_exp_tables')` line (or add one if not present):

**ADD/UPDATE** to include:
```javascript
const {
    HP_SP_COEFFICIENTS,
    ASPD_BASE_DELAYS,
    TRANS_TO_BASE_CLASS,
    TRANSCENDENT_CLASSES,
} = require('./ro_exp_tables');
```

---

### Step 3: Rewrite `calculateDerivedStats()` in `ro_damage_formulas.js`

Replace the entire function (lines 178-231).

**CURRENT** (lines 178-231):
```javascript
function calculateDerivedStats(stats) {
    const {
        str = 1, agi = 1, vit = 1, int: intStat = 1, dex = 1, luk = 1,
        level = 1,
        bonusHit = 0, bonusFlee = 0, bonusCritical = 0,
        bonusMaxHp = 0, bonusMaxSp = 0
    } = stats;

    // Status ATK (simplified from pre-renewal formula)
    const statusATK = str + Math.floor(Math.pow(Math.floor(str / 10), 2)) + Math.floor(dex / 5) + Math.floor(luk / 3);

    // Status MATK
    const statusMATK = intStat + Math.floor(Math.pow(Math.floor(intStat / 7), 2));

    // HIT = 175 + Level + DEX + bonus (base 175 ensures ~95% hit rate at equal level/dex)
    const hit = 175 + level + dex + bonusHit;

    // Flee = 100 + Level + AGI + bonus
    const flee = 100 + level + agi + bonusFlee;

    // Critical = 1 + LUK*0.3 + bonus
    const critical = 1 + Math.floor(luk * 0.3) + bonusCritical;

    // Perfect Dodge = 1 + LUK/10
    const perfectDodge = 1 + Math.floor(luk / 10);

    // Soft DEF from VIT (Pre-Renewal)
    const softDEF = Math.floor(vit / 2) + Math.max(1, Math.floor((vit * 2 - 1) / 3));

    // Soft MDEF from INT (Pre-Renewal)
    // INT + floor(INT/2)... simplified
    const softMDEF = Math.floor(intStat / 2) + Math.max(0, Math.floor((intStat * 2 - 1) / 4));

    // ASPD (simplified - weapon-specific values would come from class tables)
    const agiContribution = Math.floor(Math.sqrt(agi) * 1.2);
    const dexContribution = Math.floor(Math.sqrt(dex) * 0.6);
    const aspd = Math.min(195, Math.floor(170 + agiContribution + dexContribution));

    // Max HP (simplified formula)
    const maxHP = 100 + vit * 8 + level * 10 + bonusMaxHp;

    // Max SP (simplified formula)
    const maxSP = 50 + intStat * 5 + level * 5 + bonusMaxSp;

    return {
        statusATK,
        statusMATK,
        hit,
        flee,
        critical,
        perfectDodge,
        softDEF,
        softMDEF,
        aspd,
        maxHP,
        maxSP
    };
}
```

**NEW**:
```javascript
function calculateDerivedStats(stats) {
    const {
        str = 1, agi = 1, vit = 1, int: intStat = 1, dex = 1, luk = 1,
        level = 1,
        bonusHit = 0, bonusFlee = 0, bonusCritical = 0,
        bonusPerfectDodge = 0,
        bonusMaxHp = 0, bonusMaxSp = 0,
        // New fields from Document 1
        jobClass = 'novice',
        weaponType = 'bare_hand',
        weaponMATK = 0,
        buffAspdMultiplier = 1,
    } = stats;

    // Status ATK (pre-renewal melee formula)
    const statusATK = str + Math.floor(Math.pow(Math.floor(str / 10), 2)) + Math.floor(dex / 5) + Math.floor(luk / 3);

    // Status MATK + equipment MATK → min/max range
    const statusMATK = intStat + Math.floor(Math.pow(Math.floor(intStat / 7), 2));
    const statusMATKMax = intStat + Math.floor(Math.pow(Math.floor(intStat / 5), 2));
    const matkMin = statusMATK + Math.floor(weaponMATK * 0.7);
    const matkMax = statusMATKMax + weaponMATK;

    // HIT = 175 + Level + DEX + bonus
    const hit = 175 + level + dex + bonusHit;

    // Flee = 100 + Level + AGI + bonus
    const flee = 100 + level + agi + bonusFlee;

    // Critical = 1 + LUK*0.3 + bonus
    const critical = 1 + Math.floor(luk * 0.3) + bonusCritical;

    // Perfect Dodge = 1 + LUK/10 + equipment bonus
    const perfectDodge = 1 + Math.floor(luk / 10) + bonusPerfectDodge;

    // Soft DEF from VIT (Pre-Renewal): floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)
    const softDEF = Math.floor(vit / 2) + Math.floor(agi / 5) + Math.floor(level / 2);

    // Soft MDEF (Pre-Renewal): INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)
    const softMDEF = intStat + Math.floor(vit / 5) + Math.floor(dex / 5) + Math.floor(level / 4);

    // ASPD — weapon-type-aware (RO pre-renewal)
    const aspd = calculateASPD(jobClass, weaponType, agi, dex, buffAspdMultiplier);

    // MaxHP — class-aware (RO pre-renewal)
    const isTranscendent = TRANSCENDENT_CLASSES.has(jobClass);
    const maxHP = calculateMaxHP(level, vit, jobClass, isTranscendent, bonusMaxHp);

    // MaxSP — class-aware (RO pre-renewal)
    const maxSP = calculateMaxSP(level, intStat, jobClass, isTranscendent, bonusMaxSp);

    return {
        statusATK,
        statusMATK,     // kept for backward compat (= matkMin base component)
        matkMin,
        matkMax,
        hit,
        flee,
        critical,
        perfectDodge,
        softDEF,
        softMDEF,
        aspd,
        maxHP,
        maxSP
    };
}
```

---

### Step 4: Add Helper Functions in `ro_damage_formulas.js`

**ADD** the following three functions right before `calculateDerivedStats()` (around line 175):

```javascript
// ============================================================
// Class-aware MaxHP (RO pre-renewal)
// ============================================================
function calculateMaxHP(baseLevel, vit, jobClass, isTranscendent, bonusMaxHp) {
    const coeff = HP_SP_COEFFICIENTS[jobClass] || HP_SP_COEFFICIENTS['novice'];
    let baseHP = 35 + Math.floor(baseLevel * coeff.HP_JOB_B);
    for (let i = 2; i <= baseLevel; i++) {
        baseHP += Math.round(coeff.HP_JOB_A * i);
    }
    const transMod = isTranscendent ? 1.25 : 1.0;
    const maxHP = Math.floor(baseHP * (1 + vit * 0.01) * transMod) + (bonusMaxHp || 0);
    return Math.max(1, maxHP);
}

// ============================================================
// Class-aware MaxSP (RO pre-renewal)
// ============================================================
function calculateMaxSP(baseLevel, intStat, jobClass, isTranscendent, bonusMaxSp) {
    const coeff = HP_SP_COEFFICIENTS[jobClass] || HP_SP_COEFFICIENTS['novice'];
    let baseSP = 10 + Math.floor(baseLevel * coeff.SP_JOB_B);
    for (let i = 2; i <= baseLevel; i++) {
        baseSP += Math.round(coeff.SP_JOB_A * i);
    }
    const transMod = isTranscendent ? 1.25 : 1.0;
    const maxSP = Math.floor(baseSP * (1 + intStat * 0.01) * transMod) + (bonusMaxSp || 0);
    return Math.max(1, maxSP);
}

// ============================================================
// Weapon-type-aware ASPD (RO pre-renewal)
// ASPD = 200 - (WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SpeedMod)
// ============================================================
function calculateASPD(jobClass, weaponType, agi, dex, buffAspdMultiplier) {
    // Resolve transcendent class to base class for ASPD table lookup
    const baseClass = TRANS_TO_BASE_CLASS[jobClass] || jobClass;
    const classDelays = ASPD_BASE_DELAYS[baseClass] || ASPD_BASE_DELAYS['novice'];
    const wt = weaponType || 'bare_hand';
    const baseDelay = classDelays[wt] || classDelays['bare_hand'] || 50;

    // WeaponDelay = base delay (already in the table as the BTBA value)
    const WD = baseDelay;
    const agiReduction = Math.floor(WD * agi / 25);
    const dexReduction = Math.floor(WD * dex / 100);
    const totalReduction = Math.floor((agiReduction + dexReduction) / 10);

    // Speed modifier from buffs (e.g., Two-Hand Quicken +0.30, Adrenaline Rush +0.30)
    const speedMod = Math.max(0, (buffAspdMultiplier || 1) - 1); // 1.3 → 0.3

    const rawASPD = 200 - Math.floor((WD - totalReduction) * (1 - speedMod));
    return Math.min(195, Math.max(100, rawASPD));
}
```

---

### Step 5: Export New Functions from `ro_damage_formulas.js`

Find the `module.exports` at the bottom of `ro_damage_formulas.js` and add the new functions:

```javascript
    calculateMaxHP,
    calculateMaxSP,
    calculateASPD,
```

---

### Step 6: Import New Constants in `index.js`

**FIND** the existing import from `ro_exp_tables` (near the top of `index.js`):

**UPDATE** to include the new exports:
```javascript
const {
    // ... existing imports ...
    TRANSCENDENT_CLASSES,
} = require('./ro_exp_tables');
```

If `TRANSCENDENT_CLASSES` is not already imported, add it. This set is used in `getEffectiveStats` (from Document 1) and may be useful for other checks.

---

### Step 7: Update `buildFullStatsPayload()` in `index.js`

Apply `buffBonusMDEF` to `softMDEF` in the derived output, and ensure `matkMin`/`matkMax` are included.

**CURRENT** `derived` object in `buildFullStatsPayload` (lines 1169-1175):
```javascript
        derived: {
            ...derived,
            statusATK: Math.floor(derived.statusATK * (effectiveStats.buffAtkMultiplier || 1)),
            softDEF: Math.floor(derived.softDEF * (effectiveStats.buffDefMultiplier || 1)),
            aspd: finalAspd
        },
```

**NEW**:
```javascript
        derived: {
            ...derived,
            statusATK: Math.floor(derived.statusATK * (effectiveStats.buffAtkMultiplier || 1)),
            softDEF: Math.floor(derived.softDEF * (effectiveStats.buffDefMultiplier || 1)),
            softMDEF: derived.softMDEF + (effectiveStats.buffBonusMDEF || 0),
            aspd: finalAspd
        },
```

Note: `matkMin` and `matkMax` are automatically included via the `...derived` spread since `calculateDerivedStats()` now returns them.

---

### Step 8: Update `calculateDerivedStats` Wrapper in `index.js` (if needed)

The current wrapper at line 854-856 is:
```javascript
function calculateDerivedStats(stats) {
    return roDerivedStats(stats);
}
```

This passes the `stats` object directly, so it will automatically pass through all the new fields added by `getEffectiveStats()` in Document 1 (`jobClass`, `weaponType`, `weaponMATK`, `bonusPerfectDodge`, `buffAspdMultiplier`). **No change needed** to this wrapper.

However, verify that the `roDerivedStats` import alias at the top of `index.js` is correctly importing `calculateDerivedStats` from `ro_damage_formulas.js`:
```javascript
const {
    ...
    calculateDerivedStats: roDerivedStats,
    ...
} = require('./ro_damage_formulas');
```

---

## Validation of Data Flow

After Documents 1 and 2 are implemented, the full flow is:

```
Player equips a Rod (+30 MATK, +5 MDEF, +2 INT)
    ↓
Server tracks: weaponMATK=30, hardMdef+=5, equipmentBonuses.int+=2
    ↓
getEffectiveStats() returns: { int: base+2+buffInt, weaponMATK: 30, hardMdef: 5, jobClass: 'mage', weaponType: 'staff', ... }
    ↓
calculateDerivedStats() computes:
  - softMDEF = (INT+2) + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)  ← FIXED formula
  - matkMin = (INT+2) + floor((INT+2)/7)^2 + floor(30*0.7)             ← NEW
  - matkMax = (INT+2) + floor((INT+2)/5)^2 + 30                        ← NEW
  - perfectDodge = 1 + floor(LUK/10) + bonusPerfectDodge               ← FIXED
  - maxHP = class-aware formula with HP_JOB_A/B                         ← FIXED
  - maxSP = class-aware formula with SP_JOB_A/B                         ← FIXED
  - aspd = weapon-type-aware formula (staff delay for mage)             ← FIXED
    ↓
buildFullStatsPayload() sends:
  - stats.hardMdef = 5                                    ← NEW (from Doc 1)
  - stats.weaponMATK = 30                                 ← NEW (from Doc 1)
  - derived.matkMin = X, derived.matkMax = Y              ← NEW (from Doc 2)
  - derived.softMDEF = formula + buffBonusMDEF            ← FIXED (from Doc 2)
  - derived.perfectDodge = formula + equipBonus            ← FIXED (from Doc 2)
  - derived.maxHP = class-aware value                     ← FIXED (from Doc 2)
  - derived.maxSP = class-aware value                     ← FIXED (from Doc 2)
  - derived.aspd = weapon-type-aware value                ← FIXED (from Doc 2)
    ↓
Client parses new fields (Document 3)
```

---

## Testing Checklist

### Soft MDEF Formula
- [ ] Character with VIT 50, DEX 30, INT 10, Level 50 → softMDEF = 10 + 10 + 6 + 12 = 38 (was ~7 before)
- [ ] Character with INT 1, VIT 1, DEX 1, Level 1 → softMDEF = 1 + 0 + 0 + 0 = 1
- [ ] Pure INT build (INT 99) → softMDEF includes INT as primary contributor

### buffBonusMDEF
- [ ] Apply Endure buff (gives bonusMDEF) → `player:stats` derived.softMDEF increases
- [ ] Remove Endure buff → softMDEF returns to base value

### MATK Min/Max
- [ ] Character with INT 50, no weapon → matkMin = 50 + floor(50/7)^2 = 50+49=99, matkMax = 50 + floor(50/5)^2 = 50+100=150
- [ ] Character with INT 50, Rod with matk=30 → matkMin = 99 + 21 = 120, matkMax = 150 + 30 = 180
- [ ] `player:stats` payload includes `derived.matkMin` and `derived.matkMax`

### Perfect Dodge
- [ ] Equip item with `perfect_dodge_bonus = 3` → perfectDodge increases by 3
- [ ] Unequip → perfectDodge decreases by 3

### Class-Aware MaxHP
- [ ] Swordsman Level 50, VIT 50 → significantly more HP than Mage Level 50, VIT 50
- [ ] Novice Level 1, VIT 1 → MaxHP = floor(40 * 1.01) = 40 (35 + 1*5 = 40 base)
- [ ] Swordsman Level 50, VIT 1 → BaseHP = 35 + 50*5 + sum(round(0.7*i) for i=2..50) → significantly higher than 100 + 1*8 + 50*10 = 608
- [ ] Transcendent class gets 1.25x HP multiplier

### Class-Aware MaxSP
- [ ] Mage with INT 50 gets more SP than Swordsman with INT 50
- [ ] Transcendent class gets 1.25x SP multiplier

### ASPD
- [ ] Swordsman with dagger vs mage with dagger → different ASPD (swordsman delay=65, mage delay=60)
- [ ] Assassin with katar → fast ASPD (base delay=42)
- [ ] ASPD capped at 195
- [ ] ASPD minimum at 100
- [ ] Two-Hand Quicken buff (aspdMultiplier=1.3) → noticeable ASPD increase

### Regression
- [ ] StatusATK still computed correctly
- [ ] HIT and Flee still work
- [ ] Critical rate still correct
- [ ] All buff multipliers still apply (buffAtkMultiplier on statusATK, buffDefMultiplier on softDEF)
- [ ] Equip/unequip still triggers full recalc and emits player:stats
- [ ] HP/SP bars update correctly after equipment changes

### HP/SP Spot Check Values

| Class | Level | VIT/INT | Expected MaxHP | Expected MaxSP |
|-------|-------|---------|---------------|---------------|
| Novice | 1 | 1/1 | 40 | 12 |
| Swordsman | 50 | 50 | ~2,650 | ~185 |
| Mage | 50 | 50 | ~1,640 | ~375 |
| Knight | 99 | 80 | ~12,500 | ~360 |
| Wizard | 99 | 30 | ~3,200 | ~920 |
| Lord Knight (trans) | 99 | 80 | ~15,625 | ~450 |

(These are approximate — verify with iterative formula computation)
