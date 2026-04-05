# Document 1: Server Equipment Stats Pipeline Fix

**Dependency**: None — implement this first
**Implements before**: Document 2 (Formula Fixes), Document 3 (Client UI)

---

## Bugs Fixed

| # | Bug | Impact |
|---|-----|--------|
| 1 | `perfect_dodge_bonus` from equipment completely ignored | Items with PD bonus have no effect |
| 2 | `matk` from equipment completely ignored | Weapon MATK bonuses do nothing for magic damage |
| 3 | `mdef` from equipment completely ignored | Players have 0 hard MDEF vs magic regardless of gear |
| 4 | `weaponElement` never set from equipped weapon | All weapons deal neutral element damage |
| 5 | `armorElement` never set from equipped armor | Players always have neutral element defense |
| 6 | Weapon unequip doesn't reset `weaponElement` | Stale element after weapon removal |
| 7 | `hit_bonus, flee_bonus, critical_bonus, perfect_dodge_bonus` missing from `getPlayerInventory()` query | Client tooltips can't display these bonuses |
| 8 | `buildFullStatsPayload` doesn't send `hardMdef` or `weaponMATK` | Client has no MDEF/MATK equipment data |
| 9 | Two-handed weapon check missing in equip handler | Can equip shield + 2H weapon simultaneously |

---

## Files Modified

| File | Changes |
|------|---------|
| `server/src/index.js` | 21 changes across 8 code sections |

---

## Implementation Steps

All changes are in `server/src/index.js`.

---

### Step 1: Weapon Loading on Join (~line 1944)

Add `i.element, i.matk` to the weapon query, and capture them.

**CURRENT** (lines 1944-1949):
```javascript
            weaponResult = await pool.query(
                `SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range, i.weapon_level FROM character_inventory ci
                 JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon'
                 LIMIT 1`,
                [characterId]
            );
```

**NEW**:
```javascript
            weaponResult = await pool.query(
                `SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range, i.weapon_level,
                        i.element, i.matk
                 FROM character_inventory ci
                 JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon'
                 LIMIT 1`,
                [characterId]
            );
```

---

### Step 2: Capture Weapon Element & MATK on Join

After the weapon is loaded (after line 1958, where `weaponType` and `weaponLevel` are set), the code currently sets:
```javascript
        let weaponType = null;
        let weaponLevel = 1;
        if (weaponResult && weaponResult.rows.length > 0) {
            weaponType = weaponResult.rows[0].weapon_type;
            weaponLevel = weaponResult.rows[0].weapon_level || 1;
        }
```

**ADD** two new variables before this block (around line 1963), and set them inside the if-block:

**FIND** the block starting with `let weaponType = null;` and **REPLACE** with:
```javascript
        let weaponType = null;
        let weaponLevel = 1;
        let weaponElement = 'neutral';
        let weaponMATK = 0;
        if (weaponResult && weaponResult.rows.length > 0) {
            weaponType = weaponResult.rows[0].weapon_type;
            weaponLevel = weaponResult.rows[0].weapon_level || 1;
            weaponElement = weaponResult.rows[0].element || 'neutral';
            weaponMATK = weaponResult.rows[0].matk || 0;
        }
```

---

### Step 3: Add Armor Element Loading on Join (NEW query)

**INSERT** a new block after the weapon type/level block (after Step 2), before the equipment bonuses loading:

```javascript
        // Load equipped armor element
        let armorElement = { type: 'neutral', level: 1 };
        try {
            const armorResult = await pool.query(
                `SELECT i.element FROM character_inventory ci
                 JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'armor'
                 LIMIT 1`,
                [characterId]
            );
            if (armorResult.rows.length > 0 && armorResult.rows[0].element) {
                armorElement = { type: armorResult.rows[0].element, level: 1 };
            }
        } catch (err) {
            logger.debug(`[ITEMS] No equipped armor element for char ${characterId}`);
        }
```

---

### Step 4: Equipment Bonuses Init on Join (~line 1988)

Add `perfectDodge` to the bonuses object and a new `hardMdef` variable.

**CURRENT** (line 1988-1990):
```javascript
        const equipmentBonuses = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0 };
        let hardDef = 0;
```

**NEW**:
```javascript
        const equipmentBonuses = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0, perfectDodge: 0 };
        let hardDef = 0;
        let hardMdef = 0;
```

---

### Step 5: Equipment Bonuses Query on Join (~line 1993)

Add `i.mdef, i.perfect_dodge_bonus` to the SELECT.

**CURRENT** (lines 1993-1995):
```javascript
            const equipResult = await pool.query(
                `SELECT i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true`,
                [characterId]
            );
```

**NEW**:
```javascript
            const equipResult = await pool.query(
                `SELECT i.def, i.mdef, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus, i.perfect_dodge_bonus
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true`,
                [characterId]
            );
```

---

### Step 6: Equipment Bonuses Loop on Join (~lines 2001-2012)

Add `perfectDodge` and `hardMdef` accumulation. Find the end of the for loop (after the `hardDef += row.def || 0;` line).

**CURRENT** (end of loop, around line 2012):
```javascript
                equipmentBonuses.critical += row.critical_bonus || 0;
                hardDef += row.def || 0;
```

**NEW**:
```javascript
                equipmentBonuses.critical += row.critical_bonus || 0;
                equipmentBonuses.perfectDodge += row.perfect_dodge_bonus || 0;
                hardDef += row.def || 0;
                hardMdef += row.mdef || 0;
```

---

### Step 7: Player Object Initialization (~lines 2029-2067)

Add `hardMdef`, `weaponMATK` as new fields, and change the hardcoded `weaponElement`/`armorElement` to use loaded values.

**CURRENT** (selected lines from the `connectedPlayers.set()` call):
```javascript
            equipmentBonuses,                       // ~line 2043
            hardDef,                                // ~line 2044
```

**ADD after `hardDef,`**:
```javascript
            hardMdef,
            weaponMATK,
```

**CURRENT** (lines 2059-2061):
```javascript
            weaponElement: 'neutral',
            weaponLevel,
            armorElement: { type: 'neutral', level: 1 },
```

**NEW** (use loaded variables instead of hardcoded):
```javascript
            weaponElement,
            weaponLevel,
            armorElement,
```

---

### Step 8: `inventory:equip` Handler — Main Query (~line 6563)

Add `i.element, i.two_handed, i.perfect_dodge_bonus` to the SELECT. Note: `i.matk` and `i.mdef` are already selected.

**CURRENT** (lines 6563-6567):
```javascript
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.is_equipped, i.item_type, i.equip_slot, i.name,
                        i.atk, i.def, i.matk, i.mdef,
                        i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                        i.required_level, i.weapon_type, i.aspd_modifier, i.weapon_range, i.weapon_level
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );
```

**NEW**:
```javascript
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.is_equipped, i.item_type, i.equip_slot, i.name,
                        i.atk, i.def, i.matk, i.mdef,
                        i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                        i.perfect_dodge_bonus,
                        i.required_level, i.weapon_type, i.aspd_modifier, i.weapon_range, i.weapon_level,
                        i.element, i.two_handed
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );
```

---

### Step 9: `inventory:equip` Handler — Ensure Defaults (~lines 6591-6592)

Add `perfectDodge` to the default equipmentBonuses, and ensure `hardMdef`/`weaponMATK`.

**CURRENT** (lines 6591-6592):
```javascript
            if (!player.equipmentBonuses) player.equipmentBonuses = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0 };
            if (player.hardDef === undefined) player.hardDef = 0;
```

**NEW**:
```javascript
            if (!player.equipmentBonuses) player.equipmentBonuses = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0, perfectDodge: 0 };
            if (player.hardDef === undefined) player.hardDef = 0;
            if (player.hardMdef === undefined) player.hardMdef = 0;
            if (player.weaponMATK === undefined) player.weaponMATK = 0;
```

---

### Step 10: `inventory:equip` Handler — removeOldBonuses (~lines 6596-6608)

Add `perfectDodge` and `hardMdef` removal.

**CURRENT** (lines 6596-6608):
```javascript
            const removeOldBonuses = (old) => {
                player.equipmentBonuses.str -= old.str_bonus || 0;
                player.equipmentBonuses.agi -= old.agi_bonus || 0;
                player.equipmentBonuses.vit -= old.vit_bonus || 0;
                player.equipmentBonuses.int -= old.int_bonus || 0;
                player.equipmentBonuses.dex -= old.dex_bonus || 0;
                player.equipmentBonuses.luk -= old.luk_bonus || 0;
                player.equipmentBonuses.maxHp -= old.max_hp_bonus || 0;
                player.equipmentBonuses.maxSp -= old.max_sp_bonus || 0;
                player.equipmentBonuses.hit -= old.hit_bonus || 0;
                player.equipmentBonuses.flee -= old.flee_bonus || 0;
                player.equipmentBonuses.critical -= old.critical_bonus || 0;
                player.hardDef -= old.def || 0;
            };
```

**NEW**:
```javascript
            const removeOldBonuses = (old) => {
                player.equipmentBonuses.str -= old.str_bonus || 0;
                player.equipmentBonuses.agi -= old.agi_bonus || 0;
                player.equipmentBonuses.vit -= old.vit_bonus || 0;
                player.equipmentBonuses.int -= old.int_bonus || 0;
                player.equipmentBonuses.dex -= old.dex_bonus || 0;
                player.equipmentBonuses.luk -= old.luk_bonus || 0;
                player.equipmentBonuses.maxHp -= old.max_hp_bonus || 0;
                player.equipmentBonuses.maxSp -= old.max_sp_bonus || 0;
                player.equipmentBonuses.hit -= old.hit_bonus || 0;
                player.equipmentBonuses.flee -= old.flee_bonus || 0;
                player.equipmentBonuses.critical -= old.critical_bonus || 0;
                player.equipmentBonuses.perfectDodge -= old.perfect_dodge_bonus || 0;
                player.hardDef -= old.def || 0;
                player.hardMdef -= old.mdef || 0;
            };
```

---

### Step 11: Shield Block Check for Two-Handed Weapons (NEW)

**INSERT** right after the level requirement check (~line 6588, after the `return;` of the level check), before the `equipmentBonuses` ensure block:

```javascript
            // Two-handed weapon validation
            if (equip && item.equip_slot === 'shield') {
                // Block shield equip if a two-handed weapon is equipped
                const twoHandCheck = await pool.query(
                    `SELECT i.two_handed FROM character_inventory ci
                     JOIN items i ON ci.item_id = i.item_id
                     WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon' AND i.two_handed = true`,
                    [characterId]
                );
                if (twoHandCheck.rows.length > 0) {
                    socket.emit('inventory:error', { message: 'Cannot equip shield with a two-handed weapon' });
                    return;
                }
            }
```

---

### Step 12: Old Accessory Query — Add Missing Columns (~line 6629)

The query that fetches old accessory data for `removeOldBonuses` needs `i.mdef, i.perfect_dodge_bonus`.

**CURRENT** (around line 6629-6637):
```javascript
                            const oldAccData = await pool.query(
                                `SELECT i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus
                                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                                 WHERE ci.inventory_id = $1`,
                                [oldAcc.inventory_id]
                            );
```

**NEW**:
```javascript
                            const oldAccData = await pool.query(
                                `SELECT i.def, i.mdef, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                                        i.perfect_dodge_bonus
                                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                                 WHERE ci.inventory_id = $1`,
                                [oldAcc.inventory_id]
                            );
```

---

### Step 13: Old Same-Slot Query — Add Missing Columns (~line 6647)

The query that fetches old items in the same slot for `removeOldBonuses` needs `i.mdef, i.perfect_dodge_bonus`.

**CURRENT** (around lines 6647-6654):
```javascript
                    const oldEquipped = await pool.query(
                        `SELECT ci.inventory_id, i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                                i.equip_slot, i.atk, i.aspd_modifier, i.weapon_range
                         FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                         WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = $2`,
                        [characterId, item.equip_slot]
                    );
```

**NEW**:
```javascript
                    const oldEquipped = await pool.query(
                        `SELECT ci.inventory_id, i.def, i.mdef, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                                i.perfect_dodge_bonus,
                                i.equip_slot, i.atk, i.aspd_modifier, i.weapon_range
                         FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                         WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = $2`,
                        [characterId, item.equip_slot]
                    );
```

---

### Step 14: Two-Handed Weapon — Force Unequip Shield (NEW)

**INSERT** after the same-slot unequip DB update (after the `await pool.query(UPDATE ... SET is_equipped = false ...` for non-accessory items, around line 6665), but before the "Equip the new item" DB update:

```javascript
                    // Two-handed weapon: force-unequip shield
                    if (item.equip_slot === 'weapon' && item.two_handed) {
                        const shieldResult = await pool.query(
                            `SELECT ci.inventory_id, i.def, i.mdef,
                                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                    i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                                    i.perfect_dodge_bonus
                             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                             WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'shield'`,
                            [characterId]
                        );
                        for (const shield of shieldResult.rows) {
                            removeOldBonuses(shield);
                        }
                        if (shieldResult.rows.length > 0) {
                            await pool.query(
                                `UPDATE character_inventory SET is_equipped = false, equipped_position = NULL
                                 WHERE character_id = $1 AND is_equipped = true
                                 AND item_id IN (SELECT item_id FROM items WHERE equip_slot = 'shield')`,
                                [characterId]
                            );
                            logger.info(`[ITEMS] ${player.characterName} auto-unequipped shield (two-handed weapon equipped)`);
                        }
                    }
```

---

### Step 15: Apply Bonuses on Equip — Add New Fields (~lines 6677-6696)

After the existing bonus application lines (after `player.hardDef += item.def || 0;`), add the new fields.

**CURRENT** (end of bonus application, around line 6688-6696):
```javascript
                player.equipmentBonuses.critical += item.critical_bonus || 0;
                player.hardDef += item.def || 0;

                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = item.atk || 0;
                    player.attackRange = item.weapon_range ? item.weapon_range * COMBAT.MELEE_RANGE : COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = item.aspd_modifier || 0;
                    player.weaponType = item.weapon_type || null;
                    player.weaponLevel = item.weapon_level || 1;
                }
```

**NEW**:
```javascript
                player.equipmentBonuses.critical += item.critical_bonus || 0;
                player.equipmentBonuses.perfectDodge += item.perfect_dodge_bonus || 0;
                player.hardDef += item.def || 0;
                player.hardMdef += item.mdef || 0;

                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = item.atk || 0;
                    player.attackRange = item.weapon_range ? item.weapon_range * COMBAT.MELEE_RANGE : COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = item.aspd_modifier || 0;
                    player.weaponType = item.weapon_type || null;
                    player.weaponLevel = item.weapon_level || 1;
                    player.weaponElement = item.element || 'neutral';
                    player.weaponMATK = item.matk || 0;
                }

                if (item.equip_slot === 'armor') {
                    player.armorElement = { type: item.element || 'neutral', level: 1 };
                }
```

---

### Step 16: Unequip Path — Reset Weapon/Armor Fields (~lines 6698-6714)

Add element and MATK resets for weapon, and armor element reset.

**CURRENT** (lines 6708-6714):
```javascript
                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = 0;
                    player.attackRange = COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = 0;
                    player.weaponType = null;
                    player.weaponLevel = 1;
                }
```

**NEW**:
```javascript
                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = 0;
                    player.attackRange = COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = 0;
                    player.weaponType = null;
                    player.weaponLevel = 1;
                    player.weaponElement = 'neutral';
                    player.weaponMATK = 0;
                }

                if (item.equip_slot === 'armor') {
                    player.armorElement = { type: 'neutral', level: 1 };
                }
```

---

### Step 17: `getEffectiveStats()` — Add New Fields (~lines 899-928)

Add `bonusPerfectDodge`, `weaponMATK`, `jobClass`, `weaponType`, `isTranscendent`, and `hardMdef` to the return object.

**CURRENT** (lines 899-928):
```javascript
function getEffectiveStats(player) {
    const bonuses = player.equipmentBonuses || {};
    const passive = getPassiveSkillBonuses(player);
    const buffMods = getBuffStatModifiers(player);
    return {
        str: (player.stats.str || 1) + (bonuses.str || 0) + (buffMods.strBonus || 0),
        agi: (player.stats.agi || 1) + (bonuses.agi || 0) + (buffMods.agiBonus || 0),
        vit: (player.stats.vit || 1) + (bonuses.vit || 0) + (buffMods.vitBonus || 0),
        int: (player.stats.int || 1) + (bonuses.int || 0) + (buffMods.intBonus || 0),
        dex: (player.stats.dex || 1) + (bonuses.dex || 0) + (buffMods.dexBonus || 0) + (passive.bonusDEX || 0),
        luk: (player.stats.luk || 1) + (bonuses.luk || 0) + (buffMods.lukBonus || 0),
        level: player.stats.level || 1,
        weaponATK: player.stats.weaponATK || 0,
        passiveATK: passive.bonusATK,
        statPoints: player.stats.statPoints || 0,
        bonusHit: (bonuses.hit || 0) + (passive.bonusHIT || 0) + (buffMods.bonusHit || 0),
        bonusFlee: (bonuses.flee || 0) + (passive.bonusFLEE || 0) + (buffMods.bonusFlee || 0),
        bonusCritical: (bonuses.critical || 0) + (buffMods.bonusCritical || 0),
        bonusMaxHp: (bonuses.maxHp || 0) + (buffMods.bonusMaxHp || 0),
        bonusMaxSp: (bonuses.maxSp || 0) + (buffMods.bonusMaxSp || 0),
        buffAtkMultiplier: buffMods.atkMultiplier,
        buffDefMultiplier: buffMods.defMultiplier,
        buffBonusMDEF: buffMods.bonusMDEF,
        passiveRaceATK: passive.raceATK,
        passiveRaceDEF: passive.raceDEF,
        doubleAttackChance: passive.doubleAttackChance || 0,
        bonusRange: passive.bonusRange || 0,
        fatalBlowChance: passive.fatalBlowChance || 0,
    };
}
```

**NEW**:
```javascript
function getEffectiveStats(player) {
    const bonuses = player.equipmentBonuses || {};
    const passive = getPassiveSkillBonuses(player);
    const buffMods = getBuffStatModifiers(player);
    const jobClass = player.jobClass || 'novice';
    return {
        str: (player.stats.str || 1) + (bonuses.str || 0) + (buffMods.strBonus || 0),
        agi: (player.stats.agi || 1) + (bonuses.agi || 0) + (buffMods.agiBonus || 0),
        vit: (player.stats.vit || 1) + (bonuses.vit || 0) + (buffMods.vitBonus || 0),
        int: (player.stats.int || 1) + (bonuses.int || 0) + (buffMods.intBonus || 0),
        dex: (player.stats.dex || 1) + (bonuses.dex || 0) + (buffMods.dexBonus || 0) + (passive.bonusDEX || 0),
        luk: (player.stats.luk || 1) + (bonuses.luk || 0) + (buffMods.lukBonus || 0),
        level: player.stats.level || 1,
        weaponATK: player.stats.weaponATK || 0,
        passiveATK: passive.bonusATK,
        statPoints: player.stats.statPoints || 0,
        bonusHit: (bonuses.hit || 0) + (passive.bonusHIT || 0) + (buffMods.bonusHit || 0),
        bonusFlee: (bonuses.flee || 0) + (passive.bonusFLEE || 0) + (buffMods.bonusFlee || 0),
        bonusCritical: (bonuses.critical || 0) + (buffMods.bonusCritical || 0),
        bonusPerfectDodge: (bonuses.perfectDodge || 0),
        bonusMaxHp: (bonuses.maxHp || 0) + (buffMods.bonusMaxHp || 0),
        bonusMaxSp: (bonuses.maxSp || 0) + (buffMods.bonusMaxSp || 0),
        buffAtkMultiplier: buffMods.atkMultiplier,
        buffDefMultiplier: buffMods.defMultiplier,
        buffAspdMultiplier: buffMods.aspdMultiplier || 1,
        buffBonusMDEF: buffMods.bonusMDEF,
        passiveRaceATK: passive.raceATK,
        passiveRaceDEF: passive.raceDEF,
        doubleAttackChance: passive.doubleAttackChance || 0,
        bonusRange: passive.bonusRange || 0,
        fatalBlowChance: passive.fatalBlowChance || 0,
        // Equipment properties for derived stat formulas
        jobClass,
        weaponType: player.weaponType || 'bare_hand',
        weaponMATK: player.weaponMATK || 0,
        hardMdef: player.hardMdef || 0,
    };
}
```

---

### Step 18: `buildFullStatsPayload()` — Add New Fields (~lines 1145-1177)

Add `hardMdef` and `weaponMATK` to the stats object.

**CURRENT** `stats` object in `buildFullStatsPayload` (lines 1157-1163):
```javascript
        stats: {
            ...player.stats,
            str: effectiveStats.str, agi: effectiveStats.agi, vit: effectiveStats.vit,
            int: effectiveStats.int, dex: effectiveStats.dex, luk: effectiveStats.luk,
            hardDef: player.hardDef || 0,
            passiveATK: effectiveStats.passiveATK || 0
        },
```

**NEW**:
```javascript
        stats: {
            ...player.stats,
            str: effectiveStats.str, agi: effectiveStats.agi, vit: effectiveStats.vit,
            int: effectiveStats.int, dex: effectiveStats.dex, luk: effectiveStats.luk,
            hardDef: player.hardDef || 0,
            hardMdef: player.hardMdef || 0,
            weaponMATK: player.weaponMATK || 0,
            passiveATK: effectiveStats.passiveATK || 0
        },
```

---

### Step 19: `getAttackerInfo()` — Add `weaponMATK` (~lines 931-942)

**CURRENT**:
```javascript
function getAttackerInfo(player, cachedPassive) {
    const passive = cachedPassive || getPassiveSkillBonuses(player);
    return {
        weaponType: player.weaponType || 'bare_hand',
        weaponElement: player.weaponElement || 'neutral',
        weaponLevel: player.weaponLevel || 1,
        buffMods: getBuffStatModifiers(player),
        cardMods: player.cardMods || null,
        passiveRaceATK: passive.raceATK || null,
        race: 'demihuman'
    };
}
```

**NEW**:
```javascript
function getAttackerInfo(player, cachedPassive) {
    const passive = cachedPassive || getPassiveSkillBonuses(player);
    return {
        weaponType: player.weaponType || 'bare_hand',
        weaponElement: player.weaponElement || 'neutral',
        weaponLevel: player.weaponLevel || 1,
        weaponMATK: player.weaponMATK || 0,
        buffMods: getBuffStatModifiers(player),
        cardMods: player.cardMods || null,
        passiveRaceATK: passive.raceATK || null,
        race: 'demihuman'
    };
}
```

---

### Step 20: `getPlayerTargetInfo()` — Add `hardMdef` (~lines 964-981)

**CURRENT**:
```javascript
function getPlayerTargetInfo(player, targetCharId, cachedPassive) {
    let numAttackers = 0;
    for (const [, atkState] of autoAttackState.entries()) {
        if (!atkState.isEnemy && atkState.targetCharId === targetCharId) {
            numAttackers++;
        }
    }
    const passive = cachedPassive || getPassiveSkillBonuses(player);
    return {
        element: player.armorElement || { type: 'neutral', level: 1 },
        size: 'medium',
        race: 'demihuman',
        numAttackers: Math.max(1, numAttackers),
        buffMods: getBuffStatModifiers(player),
        passiveRaceDEF: passive.raceDEF || null
    };
}
```

**NEW**:
```javascript
function getPlayerTargetInfo(player, targetCharId, cachedPassive) {
    let numAttackers = 0;
    for (const [, atkState] of autoAttackState.entries()) {
        if (!atkState.isEnemy && atkState.targetCharId === targetCharId) {
            numAttackers++;
        }
    }
    const passive = cachedPassive || getPassiveSkillBonuses(player);
    return {
        element: player.armorElement || { type: 'neutral', level: 1 },
        size: 'medium',
        race: 'demihuman',
        hardMdef: player.hardMdef || 0,
        numAttackers: Math.max(1, numAttackers),
        buffMods: getBuffStatModifiers(player),
        passiveRaceDEF: passive.raceDEF || null
    };
}
```

---

### Step 21: `getPlayerInventory()` — Add Missing Columns (~lines 1632-1683)

Add `i.hit_bonus, i.flee_bonus, i.critical_bonus, i.perfect_dodge_bonus` to the SELECT.

**CURRENT** (lines 1639-1641):
```javascript
                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                    i.max_hp_bonus, i.max_sp_bonus, i.required_level, i.stackable, i.icon,
```

**NEW**:
```javascript
                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                    i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus, i.perfect_dodge_bonus,
                    i.required_level, i.stackable, i.icon,
```

---

## Testing Checklist

### Equipment Pipeline
- [ ] Equip weapon with MATK (e.g., Rod item_id 1601) → `player.weaponMATK` is set, magic damage uses it
- [ ] Equip weapon with element (e.g., Fire Sword) → `player.weaponElement` is set, damage uses fire element
- [ ] Unequip weapon → `weaponElement` resets to 'neutral', `weaponMATK` resets to 0
- [ ] Equip armor with element (e.g., Water armor) → `player.armorElement` updated
- [ ] Unequip armor → `armorElement` resets to `{ type: 'neutral', level: 1 }`
- [ ] Equip item with MDEF bonus → `player.hardMdef` increases
- [ ] Unequip item with MDEF → `player.hardMdef` decreases
- [ ] Equip item with perfect dodge bonus → `player.equipmentBonuses.perfectDodge` increases
- [ ] Unequip → decreases

### Two-Handed Weapon Check
- [ ] Equip two-handed weapon with shield equipped → shield auto-unequips, bonuses removed
- [ ] Try equipping shield with two-handed weapon equipped → error message, equip rejected
- [ ] Equip two-handed weapon with no shield → works normally

### Stats Pipeline
- [ ] `player:stats` event includes `hardMdef` and `weaponMATK` in `stats` object
- [ ] `getEffectiveStats()` includes `bonusPerfectDodge`, `weaponMATK`, `jobClass`, `weaponType`, `hardMdef`
- [ ] `getAttackerInfo()` includes `weaponMATK`
- [ ] `getPlayerTargetInfo()` includes `hardMdef`

### Inventory Display
- [ ] `getPlayerInventory()` query returns `hit_bonus, flee_bonus, critical_bonus, perfect_dodge_bonus`
- [ ] Items with these bonuses show correct values in the inventory:data payload

### Regression
- [ ] Normal stat bonuses (STR/AGI/VIT/INT/DEX/LUK) still work correctly on equip/unequip
- [ ] HardDef still accumulates correctly
- [ ] Dual accessory system still works (swap accessory_1/accessory_2)
- [ ] `player:stats` and `combat:health_update` still emitted after every equip change
- [ ] HP/SP capping still works after equip changes maxHP/maxSP

---

## Data Flow After Implementation

```
Player equips item
    ↓
Server reads ALL stat columns from DB (incl. mdef, matk, perfect_dodge_bonus, element, two_handed)
    ↓
removeOldBonuses subtracts old item's mdef, perfectDodge (+ existing str/agi/vit/int/dex/luk/maxHp/maxSp/hit/flee/crit/def)
    ↓
New bonuses applied: perfectDodge, hardMdef, weaponElement, armorElement, weaponMATK
    ↓
Two-handed check: auto-unequip shield if 2H weapon, block shield if 2H equipped
    ↓
getEffectiveStats() → includes bonusPerfectDodge, weaponMATK, jobClass, weaponType, hardMdef
    ↓
calculateDerivedStats(effectiveStats) → uses these new fields (Document 2)
    ↓
buildFullStatsPayload() → sends hardMdef, weaponMATK in stats object
    ↓
Client receives player:stats → parses new fields (Document 3)
    ↓
Stats UI displays correct MDEF, MATK, Perfect Dodge (Document 3)
```
