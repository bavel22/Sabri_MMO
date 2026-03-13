#!/usr/bin/env node
/**
 * audit_item_v3.js
 *
 * Comprehensive full-property audit of ALL equipment items in sabri_mmo DB.
 * Compares full_description text against EVERY DB column, not just stat bonuses.
 *
 * Checks:
 *   Property Group 1 — Base properties (from Type section of description):
 *     - Type (weapon type / armor slot)
 *     - Attack / Defense
 *     - Weight
 *     - Weapon Level
 *     - Element
 *     - Refinable
 *     - Position (headgear)
 *     - Slots
 *
 *   Property Group 2 — Requirements (from Requirements section):
 *     - Base level (required_level / equip_level_min)
 *
 *   Property Group 3 — Stat bonuses (from Bonus section):
 *     - STR/AGI/VIT/INT/DEX/LUK, MDEF, HIT, Flee, Critical, Perfect Dodge
 *     - ASPD rate, HP/SP regen rate, cast rate, SP consumption, heal power
 *     - MaxHP/MaxSP flat and rate, crit ATK rate, MATK flat, ATK/DEF bonus
 *
 * Issue classification:
 *   TRUE_MISMATCH    — description contradicts DB value
 *   COSMETIC         — wording difference, same meaning (e.g., dark vs Shadow)
 *   KNOWN_LIMITATION — no DB column for this property (e.g., flat ASPD)
 *   CONDITIONAL      — bonus from refine/conditional script, not a permanent stat
 *
 * Usage: node scripts/audit_item_v3.js
 * Output: scripts/output/item_audit_v3_report.json + stdout summary
 */

const fs   = require('fs');
const path = require('path');
const { Pool } = require('C:/Sabri_MMO/server/node_modules/pg');

const pool = new Pool({
  host: 'localhost',
  port: 5432,
  database: 'sabri_mmo',
  user: 'postgres',
  password: 'goku22',
});

const OUTPUT_PATH = path.join(__dirname, 'output', 'item_audit_v3_report.json');

// ============================================================
// Element name mapping: DB value -> Description value
// ============================================================
const ELEMENT_DB_TO_DESC = {
  dark:    'shadow',
  earth:   'earth',
  fire:    'fire',
  ghost:   'ghost',
  holy:    'holy',
  neutral: 'neutral',
  poison:  'poison',
  undead:  'undead',
  water:   'water',
  wind:    'wind',
};

// ============================================================
// Weapon type mapping: DB weapon_type + two_handed + sub_type -> Description "Type:" value
// ============================================================
function getExpectedDescType(row) {
  const wt = row.weapon_type;
  const th = row.two_handed;
  const st = row.sub_type;

  if (row.item_type === 'armor') {
    const slot = row.equip_slot;
    if (slot === 'shield')    return 'shield';
    if (slot === 'armor')     return 'armor';
    if (slot === 'garment')   return 'garment';
    if (slot === 'footgear')  return 'footgear';
    if (slot === 'accessory') return 'accessory';
    if (slot === 'head_top' || slot === 'head_mid' || slot === 'head_low') return 'headgear';
    return null;
  }

  if (!wt) return null;

  // Gun sub-types map directly
  if (wt === 'gun') {
    if (st === 'Revolver')  return 'revolver';
    if (st === 'Rifle')     return 'rifle';
    if (st === 'Shotgun')   return 'shotgun';
    if (st === 'Gatling')   return 'gatling gun';
    if (st === 'Grenade')   return 'grenade launcher';
    return wt;
  }

  // Weapons that can be one-handed or two-handed
  if (wt === 'one_hand_sword') return th ? 'two-handed sword' : 'one-handed sword';
  if (wt === 'two_hand_sword') return 'two-handed sword';
  if (wt === 'axe')            return th ? 'two-handed axe' : 'one-handed axe';
  if (wt === 'spear')          return th ? 'two-handed spear' : 'one-handed spear';
  if (wt === 'staff')          return th ? 'two-handed staff' : 'one-handed staff';

  // Direct mappings
  const directMap = {
    dagger:     'dagger',
    mace:       'mace',
    bow:        'bow',
    knuckle:    'knuckle',
    instrument: 'musical instrument',
    whip:       'whip',
    book:       'book',
    katar:      'katar',
    huuma:      'huuma shuriken',
  };
  return directMap[wt] || wt;
}

// ============================================================
// Headgear position mapping: DB equip_slot -> Description "Position:" value
// ============================================================
const HEADGEAR_POS_MAP = {
  head_top: 'upper',
  head_mid: 'middle',
  head_low: 'lower',
};

// ============================================================
// Description parser — extracts ALL properties
// ============================================================
function parseDescription(fullDesc) {
  if (!fullDesc) return null;

  const sections = fullDesc.split(/_{10,}/);

  // Find the TYPE section (contains "Type:", "Attack:", "Defense:", "Weight:", etc.)
  let bonusSection = '';
  let typeSection  = '';
  let reqSection   = '';

  for (let i = 0; i < sections.length; i++) {
    const s = sections[i].trim();
    if (/^\s*Type\s*:/m.test(s)) {
      typeSection = s;
      if (i > 0) bonusSection = sections[i - 1].trim();
      // Requirements section is the next one (if it exists)
      if (i + 1 < sections.length) reqSection = sections[i + 1].trim();
      break;
    }
  }

  const result = {
    // Group 1: Base properties from type section
    props: {},
    // Group 2: Requirements
    reqs: {},
    // Group 3: Stat bonuses from bonus section
    base: {},
    flat: {},
    rate: {},
    special: {},
  };

  // ─── GROUP 1: Parse TYPE section properties ───
  const typeLine = typeSection.match(/Type\s*:\s*(.+)/i);
  if (typeLine) result.props.type = typeLine[1].trim().toLowerCase();

  const atkMatch = typeSection.match(/Attack\s*:\s*(\d+)/i);
  if (atkMatch) result.base.atk = parseInt(atkMatch[1], 10);

  const defMatch = typeSection.match(/Defense\s*:\s*(\d+)/i);
  if (defMatch) result.base.def = parseInt(defMatch[1], 10);

  const weightMatch = typeSection.match(/Weight\s*:\s*(\d+)/i);
  if (weightMatch) result.props.weight = parseInt(weightMatch[1], 10);

  const wlvMatch = typeSection.match(/Weapon\s+Level\s*:\s*(\d+)/i);
  if (wlvMatch) result.props.weapon_level = parseInt(wlvMatch[1], 10);

  const elemMatch = typeSection.match(/Element\s*:\s*(\w+)/i);
  if (elemMatch) result.props.element = elemMatch[1].trim().toLowerCase();

  const refMatch = typeSection.match(/Refinable\s*:\s*(Yes|No)/i);
  if (refMatch) result.props.refinable = refMatch[1].toLowerCase() === 'yes';

  // Slots — can be "Slots: N" in type section or rarely in bonus section
  const slotsMatch = typeSection.match(/Slots\s*:\s*(\d+)/i);
  if (slotsMatch) result.props.slots = parseInt(slotsMatch[1], 10);
  // Also check bonus section for Slots:
  if (!slotsMatch) {
    const slotsMatch2 = bonusSection.match(/Slots\s*:\s*(\d+)/i);
    if (slotsMatch2) result.props.slots = parseInt(slotsMatch2[1], 10);
  }

  // Headgear position — "Position: Upper", "Position: Upper, Middle", etc.
  const posMatch = typeSection.match(/Position\s*:\s*(.+)/i);
  if (posMatch) result.props.position = posMatch[1].trim().toLowerCase();

  // ─── GROUP 2: Parse REQUIREMENTS section ───
  // "Requirement:" section is typically after type section divider
  const fullReqText = reqSection || '';
  const baseLvMatch = fullReqText.match(/Base\s+level\s+(\d+)/i);
  if (baseLvMatch) result.reqs.base_level = parseInt(baseLvMatch[1], 10);
  // Also check main text (sometimes requirement is inline)
  if (!baseLvMatch) {
    const alt = fullDesc.match(/Base\s+level\s+(\d+)/i);
    if (alt) result.reqs.base_level = parseInt(alt[1], 10);
  }

  // ─── GROUP 3: Parse BONUS section stats ───
  const text = bonusSection;

  // "All Stats +N" → STR/AGI/VIT/INT/DEX/LUK
  const allStatsRe = /All\s+Stats?\s+([+-])\s*(\d+)/gi;
  let asm;
  while ((asm = allStatsRe.exec(text)) !== null) {
    const sign = asm[1] === '-' ? -1 : 1;
    const val = parseInt(asm[2], 10) * sign;
    for (const s of ['str', 'agi', 'vit', 'int', 'dex', 'luk']) {
      result.flat[s] = (result.flat[s] || 0) + val;
    }
  }

  // Flat bonus stats
  const flatPatterns = [
    { key: 'str',              re: /\bSTR\s+([+-])\s*(\d+)/gi },
    { key: 'agi',              re: /\bAGI\s+([+-])\s*(\d+)/gi },
    { key: 'vit',              re: /\bVIT\s+([+-])\s*(\d+)/gi },
    { key: 'int',              re: /\bINT\s+([+-])\s*(\d+)/gi },
    { key: 'dex',              re: /\bDEX\s+([+-])\s*(\d+)/gi },
    { key: 'luk',              re: /\bLUK\s+([+-])\s*(\d+)/gi },
    { key: 'atk_bonus',        re: /\bATK\s+([+-])\s*(\d+)/gi },
    { key: 'def_bonus',        re: /\bDEF\s+([+-])\s*(\d+)/gi },
    { key: 'mdef',             re: /\bMDEF\s+([+-])\s*(\d+)/gi },
    { key: 'matk_flat',        re: /\bMATK\s+([+-])\s*(\d+)/gi },
    { key: 'hit',              re: /\bHIT\s+([+-])\s*(\d+)/gi },
    { key: 'flee',             re: /\bFlee\s+([+-])\s*(\d+)/gi },
    { key: 'critical',         re: /\bCritical\s+([+-])\s*(\d+)/gi },
    { key: 'perfect_dodge',    re: /\bPerfect\s+Dodge\s+([+-])\s*(\d+)/gi },
    { key: 'aspd_flat',        re: /\bASPD\s+([+-])\s*(\d+)/gi },
    { key: 'max_hp',           re: /\bMaxHP\s+([+-])\s*(\d+)/gi },
    { key: 'max_sp',           re: /\bMaxSP\s+([+-])\s*(\d+)/gi },
  ];

  for (const { key, re } of flatPatterns) {
    re.lastIndex = 0;
    let m;
    while ((m = re.exec(text)) !== null) {
      const sign = m[1] === '-' ? -1 : 1;
      const val = parseInt(m[2], 10) * sign;
      const afterIdx = m.index + m[0].length;
      const afterChar = text.substring(afterIdx).match(/^\s*(%)/);
      if (afterChar) {
        // Percentage — store in rate
        if (key === 'matk_flat')   result.rate['matk_rate']   = (result.rate['matk_rate'] || 0) + val;
        else if (key === 'max_hp') result.rate['max_hp_rate'] = (result.rate['max_hp_rate'] || 0) + val;
        else if (key === 'max_sp') result.rate['max_sp_rate'] = (result.rate['max_sp_rate'] || 0) + val;
        else if (key === 'atk_bonus') result.rate['atk_rate'] = (result.rate['atk_rate'] || 0) + val;
        continue;
      }
      result.flat[key] = (result.flat[key] || 0) + val;
    }
  }

  // "Reduces after attack delay by N%"
  const aspdDelayRe = /Reduces?\s+(?:after\s+)?attack\s+delay\s+by\s+(-?\d+)\s*%/gi;
  let asdm;
  while ((asdm = aspdDelayRe.exec(text)) !== null) {
    const lineStart = text.lastIndexOf('\n', asdm.index);
    const linePrefix = text.substring(lineStart + 1, asdm.index);
    if (/(?:chance\s+to|Random)/i.test(linePrefix)) continue;
    result.rate['aspd_rate'] = (result.rate['aspd_rate'] || 0) + parseInt(asdm[1], 10);
    break;
  }

  // HP/SP recovery
  const hpRecovRe = /HP\s+recovery\s+([+-])\s*(\d+)\s*%/gi;
  let hrm;
  while ((hrm = hpRecovRe.exec(text)) !== null) {
    const sign = hrm[1] === '-' ? -1 : 1;
    result.rate['hp_regen_rate'] = (result.rate['hp_regen_rate'] || 0) + parseInt(hrm[2], 10) * sign;
  }

  const spRecovRe = /SP\s+recovery\s+([+-])\s*(\d+)\s*%/gi;
  let srm;
  while ((srm = spRecovRe.exec(text)) !== null) {
    const sign = srm[1] === '-' ? -1 : 1;
    result.rate['sp_regen_rate'] = (result.rate['sp_regen_rate'] || 0) + parseInt(srm[2], 10) * sign;
  }

  // Cast time — rAthena convention: positive = reduction.
  // "Reduces cast time by N%" — DB stores as POSITIVE (e.g., 15 = 15% reduction).
  // Some older items may have DB as negative (e.g., -3). We'll handle both in comparison.
  const castRe = /Reduces?\s+(?:variable\s+)?cast\s+time(?:\s+of\s+all\s+skills)?\s+by\s+(\d+)\s*%/gi;
  let crm;
  while ((crm = castRe.exec(text)) !== null) {
    result.rate['cast_rate_reduction'] = (result.rate['cast_rate_reduction'] || 0) + parseInt(crm[1], 10);
  }

  // SP consumption
  const spConsRe = /SP\s+consumption\s+([+-])\s*(\d+)\s*%/gi;
  let scm;
  while ((scm = spConsRe.exec(text)) !== null) {
    const sign = scm[1] === '-' ? -1 : 1;
    result.rate['use_sp_rate'] = (result.rate['use_sp_rate'] || 0) + parseInt(scm[2], 10) * sign;
  }

  // Heal power
  const healRe = /Increases?\s+healing\s+(?:effectiveness\s+)?by\s+(\d+)\s*%/gi;
  let hm;
  while ((hm = healRe.exec(text)) !== null) {
    result.special['heal_power'] = (result.special['heal_power'] || 0) + parseInt(hm[1], 10);
  }

  // Critical damage
  const critDmgRe = /Increases?\s+critical\s+damage\s+by\s+(\d+)\s*%/gi;
  let cdm;
  while ((cdm = critDmgRe.exec(text)) !== null) {
    result.rate['crit_atk_rate'] = (result.rate['crit_atk_rate'] || 0) + parseInt(cdm[1], 10);
  }

  return result;
}

// ============================================================
// Issue object constructor
// ============================================================
function issue(property, dbColumn, descValue, dbValue, category, detail) {
  return { property, db_column: dbColumn, desc_value: descValue, db_value: dbValue, category, detail: detail || null };
}

// ============================================================
// Full comparison
// ============================================================
function compareItem(dbRow, parsed) {
  const issues = [];

  // ─── GROUP 1: Base properties ───

  // 1a. Type
  if (parsed.props.type != null) {
    const expectedType = getExpectedDescType(dbRow);
    if (expectedType != null) {
      const descType = parsed.props.type;
      // Handle "costume" type — separate category
      if (descType === 'costume') {
        // Costume items have a different equip pattern, skip type check
      } else if (descType !== expectedType) {
        // Check if it's a two-handed mismatch — description says Two-Handed but DB has two_handed=false, etc.
        issues.push(issue(
          'Type', 'weapon_type/equip_slot/two_handed',
          descType, expectedType, 'TRUE_MISMATCH',
          `Desc says "${parsed.props.type}", expected "${expectedType}" based on DB weapon_type=${dbRow.weapon_type}, two_handed=${dbRow.two_handed}, equip_slot=${dbRow.equip_slot}`
        ));
      }
    }
  }

  // 1b. Weight
  if (parsed.props.weight != null) {
    const dbWeight = dbRow.weight || 0;
    if (parsed.props.weight !== dbWeight) {
      issues.push(issue('Weight', 'weight', parsed.props.weight, dbWeight, 'TRUE_MISMATCH'));
    }
  } else {
    // No weight in description — flag if DB has weight (for equipment this is almost always present)
    // Actually most items DO have weight in desc, so only flag if DB weight is nonzero and desc doesn't mention it
    // This is common for items whose descriptions don't follow the standard template — skip flagging
  }

  // 1c. Weapon Level
  if (parsed.props.weapon_level != null) {
    const dbWlv = dbRow.weapon_level || 0;
    if (parsed.props.weapon_level !== dbWlv) {
      issues.push(issue('Weapon Level', 'weapon_level', parsed.props.weapon_level, dbWlv, 'TRUE_MISMATCH'));
    }
  } else if (dbRow.item_type === 'weapon' && dbRow.weapon_level && dbRow.weapon_level > 0) {
    // Weapon has weapon_level in DB but desc doesn't mention it — this is unusual
    // Only flag for actual weapons, not armor
    // Don't flag — many weapons legitimately don't show Weapon Level in desc template
  }

  // 1d. Element
  if (parsed.props.element != null) {
    const dbElem = (dbRow.element || 'neutral').toLowerCase();
    const descElem = parsed.props.element;
    const dbElemNormalized = ELEMENT_DB_TO_DESC[dbElem] || dbElem;
    if (descElem !== dbElemNormalized) {
      // Check if it's the dark/shadow cosmetic difference but with wrong mapping
      issues.push(issue('Element', 'element', descElem, dbElem, 'TRUE_MISMATCH',
        `Desc says "${descElem}", DB has "${dbElem}" (normalized: "${dbElemNormalized}")`));
    }
  } else if (dbRow.element && dbRow.element !== 'neutral' && dbRow.item_type === 'weapon') {
    // DB has non-neutral element but desc doesn't mention — could be a real issue
    issues.push(issue('Element (missing in desc)', 'element', null, dbRow.element, 'TRUE_MISMATCH',
      `DB has element="${dbRow.element}" but description does not show Element: line`));
  }

  // 1e. Refinable
  if (parsed.props.refinable != null) {
    const dbRef = dbRow.refineable;
    if (dbRef != null && parsed.props.refinable !== dbRef) {
      issues.push(issue('Refinable', 'refineable', parsed.props.refinable, dbRef, 'TRUE_MISMATCH'));
    }
  }

  // 1f. Slots (only check if desc explicitly states "Slots: N")
  if (parsed.props.slots != null) {
    const dbSlots = dbRow.slots || 0;
    if (parsed.props.slots !== dbSlots) {
      issues.push(issue('Slots', 'slots', parsed.props.slots, dbSlots, 'TRUE_MISMATCH'));
    }
  }

  // 1g. Headgear Position
  if (parsed.props.position != null) {
    const dbSlot = dbRow.equip_slot;
    if (dbSlot && dbSlot.startsWith('head_')) {
      const expectedPos = HEADGEAR_POS_MAP[dbSlot];
      if (expectedPos) {
        const descPos = parsed.props.position;
        // Position can be compound: "upper, middle" — check if the expected position is included
        if (!descPos.includes(expectedPos)) {
          issues.push(issue('Headgear Position', 'equip_slot', descPos, dbSlot + ' (' + expectedPos + ')', 'TRUE_MISMATCH',
            `DB equip_slot="${dbSlot}" expects position="${expectedPos}" but desc says "${descPos}"`));
        }
      }
    }
  }

  // ─── GROUP 2: Requirements ───

  // 2a. Base Level
  if (parsed.reqs.base_level != null) {
    // Check against both required_level and equip_level_min — use whichever is nonzero
    const dbReqLv = dbRow.required_level || dbRow.equip_level_min || 0;
    if (dbReqLv > 0 && parsed.reqs.base_level !== dbReqLv) {
      issues.push(issue('Required Level', 'required_level/equip_level_min', parsed.reqs.base_level, dbReqLv, 'TRUE_MISMATCH',
        `Desc says "Base level ${parsed.reqs.base_level}", DB required_level=${dbRow.required_level}, equip_level_min=${dbRow.equip_level_min}`));
    }
  } else {
    const dbReqLv = dbRow.required_level || dbRow.equip_level_min || 0;
    if (dbReqLv > 1) {
      // DB has a level requirement but desc doesn't mention one — could be real
      // Don't flag for level 1 (default/no requirement)
      issues.push(issue('Required Level (missing in desc)', 'required_level/equip_level_min', null, dbReqLv, 'TRUE_MISMATCH',
        `DB has required level ${dbReqLv} but description does not mention "Base level N"`));
    }
  }

  // ─── GROUP 3: Stat bonuses ───

  // 3a. Base ATK/DEF (from "Attack: N" / "Defense: N" in type section)
  if (parsed.base.atk != null) {
    const dbAtk = dbRow.atk || 0;
    // If desc also has "ATK +N" bonus, then DB atk may = base + bonus
    const descBonusAtk = parsed.flat.atk_bonus ?? 0;
    if (parsed.base.atk !== dbAtk) {
      // Check if DB atk = desc base + desc bonus ATK
      if (descBonusAtk !== 0 && parsed.base.atk + descBonusAtk === dbAtk) {
        // Resolved: DB includes script bonus in atk column
      } else {
        issues.push(issue('Base ATK', 'atk', parsed.base.atk, dbAtk, 'TRUE_MISMATCH',
          descBonusAtk !== 0
            ? `Desc has "Attack: ${parsed.base.atk}" + "ATK +${descBonusAtk}" = ${parsed.base.atk + descBonusAtk}, DB atk=${dbAtk}`
            : null));
      }
    }
  }

  if (parsed.base.def != null) {
    const dbDef = dbRow.def || 0;
    const descBonusDef = parsed.flat.def_bonus ?? 0;
    if (parsed.base.def !== dbDef) {
      if (descBonusDef !== 0 && parsed.base.def + descBonusDef === dbDef) {
        // Resolved: DB includes script bonus in def column
      } else {
        issues.push(issue('Base DEF', 'def', parsed.base.def, dbDef, 'TRUE_MISMATCH',
          descBonusDef !== 0
            ? `Desc has "Defense: ${parsed.base.def}" + "DEF +${descBonusDef}" = ${parsed.base.def + descBonusDef}, DB def=${dbDef}`
            : null));
      }
    }
  }

  // 3b. Flat stat bonuses
  const STAT_MAP = [
    { descKey: 'str',           dbCol: 'str_bonus',          label: 'STR bonus' },
    { descKey: 'agi',           dbCol: 'agi_bonus',          label: 'AGI bonus' },
    { descKey: 'vit',           dbCol: 'vit_bonus',          label: 'VIT bonus' },
    { descKey: 'int',           dbCol: 'int_bonus',          label: 'INT bonus' },
    { descKey: 'dex',           dbCol: 'dex_bonus',          label: 'DEX bonus' },
    { descKey: 'luk',           dbCol: 'luk_bonus',          label: 'LUK bonus' },
    { descKey: 'mdef',          dbCol: 'mdef',               label: 'MDEF bonus' },
    { descKey: 'hit',           dbCol: 'hit_bonus',          label: 'HIT bonus' },
    { descKey: 'flee',          dbCol: 'flee_bonus',         label: 'Flee bonus' },
    { descKey: 'critical',      dbCol: 'critical_bonus',     label: 'Critical bonus' },
    { descKey: 'perfect_dodge', dbCol: 'perfect_dodge_bonus',label: 'Perfect Dodge bonus' },
    { descKey: 'max_hp',        dbCol: 'max_hp_bonus',       label: 'MaxHP flat bonus' },
    { descKey: 'max_sp',        dbCol: 'max_sp_bonus',       label: 'MaxSP flat bonus' },
  ];

  for (const { descKey, dbCol, label } of STAT_MAP) {
    const descVal = parsed.flat[descKey] ?? null;
    const dbVal = dbRow[dbCol] || 0;

    if (descVal != null && descVal !== 0) {
      if (dbVal === 0) {
        issues.push(issue(label + ' (in desc, not in DB)', dbCol, descVal, dbVal, 'CONDITIONAL',
          `Description mentions ${label}=${descVal} but DB ${dbCol}=0. May be from script/conditional.`));
      } else if (descVal !== dbVal) {
        issues.push(issue(label, dbCol, descVal, dbVal, 'TRUE_MISMATCH'));
      }
    } else if (dbVal !== 0) {
      issues.push(issue(label + ' (in DB, not in desc)', dbCol, null, dbVal, 'TRUE_MISMATCH',
        `DB has ${dbCol}=${dbVal} but description does not mention it.`));
    }
  }

  // 3c. "ATK +N" bonus — this is the script bBaseAtk, rolled into items.atk.
  // We already checked base ATK with bonus reconciliation above.
  // But "ATK +N" on accessories/armor is a separate bonus that should map to... nothing directly
  // (items.atk is 0 for armor). For weapons, it's folded into items.atk.
  // For armor/accessories with "ATK +N", there's no separate DB column — it's in the script.
  // Don't double-flag.
  if (parsed.flat.atk_bonus != null && parsed.flat.atk_bonus !== 0) {
    if (dbRow.item_type === 'armor') {
      // ATK +N on armor is purely from script — no DB column for it
      // Already covered by the script, not a data error
    }
  }

  // 3d. "DEF +N" bonus on weapons/accessories — script bonus, no separate column.
  // For weapons, "DEF +N" is a bonus effect (script bDef), not the weapon's base defense.
  // Don't flag it as missing from items.def since weapons don't have base DEF.
  // For armor, "DEF +N" might be rolled into items.def (already checked above as base DEF + bonus).

  // 3e. Rate bonuses
  const RATE_MAP = [
    { descKey: 'aspd_rate',     dbCol: 'aspd_rate',     label: 'ASPD rate %' },
    { descKey: 'hp_regen_rate', dbCol: 'hp_regen_rate', label: 'HP recovery rate %' },
    { descKey: 'sp_regen_rate', dbCol: 'sp_regen_rate', label: 'SP recovery rate %' },
    { descKey: 'use_sp_rate',   dbCol: 'use_sp_rate',   label: 'SP consumption rate %' },
    { descKey: 'max_hp_rate',   dbCol: 'max_hp_rate',   label: 'MaxHP rate %' },
    { descKey: 'max_sp_rate',   dbCol: 'max_sp_rate',   label: 'MaxSP rate %' },
    { descKey: 'crit_atk_rate', dbCol: 'crit_atk_rate', label: 'Critical ATK rate %' },
  ];

  for (const { descKey, dbCol, label } of RATE_MAP) {
    const descVal = parsed.rate[descKey] ?? null;
    const dbVal = dbRow[dbCol] || 0;

    if (descVal != null && descVal !== 0) {
      if (dbVal === 0) {
        issues.push(issue(label + ' (in desc, not in DB)', dbCol, descVal, dbVal, 'CONDITIONAL',
          `Description mentions ${label}=${descVal} but DB ${dbCol}=0. May be from script/conditional.`));
      } else if (descVal !== dbVal) {
        issues.push(issue(label, dbCol, descVal, dbVal, 'TRUE_MISMATCH'));
      }
    } else if (dbVal !== 0) {
      // Only flag if cast_rate is not handled by the special cast_rate logic below
      if (dbCol !== 'cast_rate' || parsed.rate.cast_rate_reduction == null) {
        issues.push(issue(label + ' (in DB, not in desc)', dbCol, null, dbVal, 'TRUE_MISMATCH',
          `DB has ${dbCol}=${dbVal} but description does not mention it.`));
      }
    }
  }

  // 3f. Cast rate — special handling for sign convention
  // rAthena convention: positive = reduction. So DB cast_rate=15 means "15% reduction".
  // But some older items have DB cast_rate as NEGATIVE (e.g., -3 = 3% reduction).
  // Description always says "Reduces cast time by N%".
  const descCastReduction = parsed.rate.cast_rate_reduction ?? null;
  const dbCastRate = dbRow.cast_rate || 0;

  if (descCastReduction != null && descCastReduction > 0) {
    if (dbCastRate === 0) {
      issues.push(issue('Cast rate (in desc, not in DB)', 'cast_rate', descCastReduction, dbCastRate, 'CONDITIONAL',
        `Desc says "Reduces cast time by ${descCastReduction}%" but DB cast_rate=0.`));
    } else {
      // Match if DB is positive and equals desc, OR DB is negative and abs equals desc
      const absDb = Math.abs(dbCastRate);
      if (absDb !== descCastReduction) {
        issues.push(issue('Cast rate', 'cast_rate', descCastReduction, dbCastRate, 'TRUE_MISMATCH',
          `Desc says "${descCastReduction}% reduction", DB cast_rate=${dbCastRate} (abs=${absDb}).`));
      }
      // else: match — either DB=+15 and desc=15, or DB=-15 and desc=15 → both mean 15% reduction
    }
  } else if (dbCastRate !== 0 && descCastReduction == null) {
    issues.push(issue('Cast rate (in DB, not in desc)', 'cast_rate', null, dbCastRate, 'TRUE_MISMATCH',
      `DB has cast_rate=${dbCastRate} but description does not mention cast time reduction.`));
  }

  // 3g. Heal power
  const descHeal = parsed.special.heal_power ?? null;
  const dbHeal = dbRow.heal_power || 0;
  if (descHeal != null && descHeal !== 0) {
    if (dbHeal === 0) {
      issues.push(issue('Heal power (in desc, not in DB)', 'heal_power', descHeal, dbHeal, 'CONDITIONAL'));
    } else if (descHeal !== dbHeal) {
      issues.push(issue('Heal power', 'heal_power', descHeal, dbHeal, 'TRUE_MISMATCH'));
    }
  } else if (dbHeal !== 0) {
    issues.push(issue('Heal power (in DB, not in desc)', 'heal_power', null, dbHeal, 'TRUE_MISMATCH'));
  }

  // 3h. Flat MATK
  const descMatkFlat = parsed.flat.matk_flat ?? null;
  const dbMatk = dbRow.matk || 0;
  if (descMatkFlat != null && descMatkFlat !== 0) {
    if (dbMatk === 0) {
      issues.push(issue('MATK flat (in desc, not in DB)', 'matk', descMatkFlat, dbMatk, 'CONDITIONAL',
        'Desc has flat MATK +N but DB matk=0. May be in script only.'));
    } else if (descMatkFlat !== dbMatk) {
      issues.push(issue('MATK flat', 'matk', descMatkFlat, dbMatk, 'TRUE_MISMATCH'));
    }
  } else if (dbMatk !== 0 && descMatkFlat == null) {
    const descMatkRate = parsed.rate.matk_rate ?? null;
    if (descMatkRate == null) {
      issues.push(issue('MATK (in DB, not in desc)', 'matk', null, dbMatk, 'TRUE_MISMATCH'));
    }
  }

  // 3i. Flat ASPD (ASPD +N without %) — no DB column exists
  const descAspdFlat = parsed.flat.aspd_flat ?? null;
  if (descAspdFlat != null && descAspdFlat !== 0) {
    issues.push(issue('ASPD flat (no DB column)', 'none', descAspdFlat, null, 'KNOWN_LIMITATION',
      'Desc has ASPD +N flat. DB only stores aspd_rate (%). These are different stats.'));
  }

  // 3j. MATK rate % — no direct DB column
  const descMatkRate = parsed.rate.matk_rate ?? null;
  if (descMatkRate != null && descMatkRate !== 0) {
    // Informational only — no matk_rate DB column
    issues.push(issue('MATK rate % (no DB column)', 'none', descMatkRate, null, 'KNOWN_LIMITATION',
      'Desc has MATK +N%. No matk_rate column in DB — stored in script.'));
  }

  // 3k. ATK rate % — no direct DB column
  const descAtkRate = parsed.rate.atk_rate ?? null;
  if (descAtkRate != null && descAtkRate !== 0) {
    issues.push(issue('ATK rate % (no DB column)', 'none', descAtkRate, null, 'KNOWN_LIMITATION',
      'Desc has ATK +N%. No atk_rate column in DB — stored in script.'));
  }

  return issues;
}

// ============================================================
// Main
// ============================================================
async function main() {
  console.log('=== Item Audit V3: Full Property Comparison ===\n');

  const query = `
    SELECT
      item_id, name, description, item_type, equip_slot, weapon_type, sub_type,
      atk, def, matk, mdef,
      str_bonus, agi_bonus, vit_bonus, int_bonus, dex_bonus, luk_bonus,
      max_hp_bonus, max_sp_bonus, hit_bonus, flee_bonus, critical_bonus,
      perfect_dodge_bonus, aspd_rate, hp_regen_rate, sp_regen_rate,
      cast_rate, use_sp_rate, heal_power, max_hp_rate, max_sp_rate, crit_atk_rate,
      weight, weapon_level, armor_level, element, slots, refineable, two_handed,
      required_level, equip_level_min, equip_level_max,
      full_description, script
    FROM items
    WHERE item_type IN ('weapon', 'armor')
    ORDER BY item_id
  `;

  const { rows } = await pool.query(query);
  console.log(`Fetched ${rows.length} equipment items from DB\n`);

  // Result accumulators
  const allIssues       = [];  // Every issue for every item
  const perfectMatches  = [];
  const noDescription   = [];

  // Category counters
  const categoryCounts = { TRUE_MISMATCH: 0, COSMETIC: 0, KNOWN_LIMITATION: 0, CONDITIONAL: 0 };
  // Property counters (by property name)
  const propertyCounts = {};
  // Per-category per-property
  const categoryPropertyCounts = { TRUE_MISMATCH: {}, COSMETIC: {}, KNOWN_LIMITATION: {}, CONDITIONAL: {} };

  for (const row of rows) {
    if (!row.full_description) {
      noDescription.push({ item_id: row.item_id, name: row.name, item_type: row.item_type, equip_slot: row.equip_slot });
      continue;
    }

    const parsed = parseDescription(row.full_description);
    if (!parsed) continue;

    const issues = compareItem(row, parsed);

    if (issues.length === 0) {
      perfectMatches.push({ item_id: row.item_id, name: row.name, item_type: row.item_type, equip_slot: row.equip_slot });
    } else {
      allIssues.push({
        item_id: row.item_id,
        name: row.name,
        item_type: row.item_type,
        equip_slot: row.equip_slot,
        weapon_type: row.weapon_type || null,
        issues: issues,
      });

      for (const iss of issues) {
        categoryCounts[iss.category] = (categoryCounts[iss.category] || 0) + 1;
        propertyCounts[iss.property] = (propertyCounts[iss.property] || 0) + 1;
        if (!categoryPropertyCounts[iss.category]) categoryPropertyCounts[iss.category] = {};
        categoryPropertyCounts[iss.category][iss.property] = (categoryPropertyCounts[iss.category][iss.property] || 0) + 1;
      }
    }
  }

  // ─── Build report ───
  const totalIssueCount = Object.values(categoryCounts).reduce((a, b) => a + b, 0);

  const report = {
    summary: {
      total_equipment: rows.length,
      with_description: rows.length - noDescription.length,
      no_description: noDescription.length,
      perfect_match: perfectMatches.length,
      items_with_issues: allIssues.length,
      total_issues: totalIssueCount,
      by_category: categoryCounts,
    },
    property_breakdown: propertyCounts,
    category_property_breakdown: categoryPropertyCounts,
    // Group issues by category for the report
    issues_TRUE_MISMATCH:    allIssues.filter(i => i.issues.some(x => x.category === 'TRUE_MISMATCH'))
                                      .map(i => ({ ...i, issues: i.issues.filter(x => x.category === 'TRUE_MISMATCH') })),
    issues_CONDITIONAL:      allIssues.filter(i => i.issues.some(x => x.category === 'CONDITIONAL'))
                                      .map(i => ({ ...i, issues: i.issues.filter(x => x.category === 'CONDITIONAL') })),
    issues_KNOWN_LIMITATION: allIssues.filter(i => i.issues.some(x => x.category === 'KNOWN_LIMITATION'))
                                      .map(i => ({ ...i, issues: i.issues.filter(x => x.category === 'KNOWN_LIMITATION') })),
    issues_COSMETIC:         allIssues.filter(i => i.issues.some(x => x.category === 'COSMETIC'))
                                      .map(i => ({ ...i, issues: i.issues.filter(x => x.category === 'COSMETIC') })),
    no_description: noDescription,
  };

  // Write JSON
  const outDir = path.dirname(OUTPUT_PATH);
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });
  fs.writeFileSync(OUTPUT_PATH, JSON.stringify(report, null, 2));
  console.log(`Report written to: ${OUTPUT_PATH}\n`);

  // ─── Human-readable stdout summary ───
  console.log('================================================================');
  console.log('             ITEM AUDIT V3 — FULL PROPERTY SUMMARY');
  console.log('================================================================\n');

  console.log(`Total equipment items:       ${report.summary.total_equipment}`);
  console.log(`  With full_description:     ${report.summary.with_description}`);
  console.log(`  Missing full_description:  ${report.summary.no_description}`);
  console.log(`  Perfect match (no issues): ${report.summary.perfect_match}`);
  console.log(`  Items with issues:         ${report.summary.items_with_issues}`);
  console.log(`  Total issues found:        ${report.summary.total_issues}`);

  console.log('\n── Issue Count by Category ──');
  for (const [cat, count] of Object.entries(categoryCounts).sort((a, b) => b[1] - a[1])) {
    const pct = ((count / totalIssueCount) * 100).toFixed(1);
    console.log(`  ${cat}: ${count} (${pct}%)`);
  }

  console.log('\n── Issue Count by Property ──');
  const propSorted = Object.entries(propertyCounts).sort((a, b) => b[1] - a[1]);
  for (const [prop, count] of propSorted) {
    console.log(`  ${prop}: ${count}`);
  }

  // ─── TRUE_MISMATCH details ───
  const trueMismatches = report.issues_TRUE_MISMATCH;
  if (trueMismatches.length > 0) {
    console.log('\n================================================================');
    console.log('  TRUE MISMATCHES — description contradicts DB value');
    console.log('================================================================');
    console.log(`  (${trueMismatches.length} items, showing first 40)\n`);

    // Group by property for cleaner output
    const byProp = {};
    for (const item of trueMismatches) {
      for (const iss of item.issues) {
        if (!byProp[iss.property]) byProp[iss.property] = [];
        byProp[iss.property].push({ item_id: item.item_id, name: item.name, ...iss });
      }
    }

    for (const [prop, items] of Object.entries(byProp).sort((a, b) => b[1].length - a[1].length)) {
      console.log(`  ── ${prop} (${items.length} items) ──`);
      for (const it of items.slice(0, 5)) {
        const detail = it.detail ? `  ${it.detail}` : '';
        console.log(`    [${it.item_id}] ${it.name}: desc=${it.desc_value} DB=${it.db_value}${detail}`);
      }
      if (items.length > 5) console.log(`    ... and ${items.length - 5} more`);
    }
  }

  // ─── CONDITIONAL details ───
  const conditionals = report.issues_CONDITIONAL;
  if (conditionals.length > 0) {
    console.log('\n================================================================');
    console.log('  CONDITIONAL — bonus from script/refine, not permanent stat');
    console.log('================================================================');
    console.log(`  (${conditionals.length} items, showing first 20)\n`);

    const byProp = {};
    for (const item of conditionals) {
      for (const iss of item.issues) {
        if (!byProp[iss.property]) byProp[iss.property] = [];
        byProp[iss.property].push({ item_id: item.item_id, name: item.name, ...iss });
      }
    }

    for (const [prop, items] of Object.entries(byProp).sort((a, b) => b[1].length - a[1].length)) {
      console.log(`  ── ${prop} (${items.length} items) ──`);
      for (const it of items.slice(0, 3)) {
        console.log(`    [${it.item_id}] ${it.name}: desc=${it.desc_value} DB=${it.db_value}`);
      }
      if (items.length > 3) console.log(`    ... and ${items.length - 3} more`);
    }
  }

  // ─── KNOWN_LIMITATION details ───
  const limitations = report.issues_KNOWN_LIMITATION;
  if (limitations.length > 0) {
    console.log('\n================================================================');
    console.log('  KNOWN LIMITATIONS — no DB column for this property');
    console.log('================================================================');
    console.log(`  (${limitations.length} items)\n`);

    const byProp = {};
    for (const item of limitations) {
      for (const iss of item.issues) {
        if (!byProp[iss.property]) byProp[iss.property] = [];
        byProp[iss.property].push({ item_id: item.item_id, name: item.name });
      }
    }

    for (const [prop, items] of Object.entries(byProp).sort((a, b) => b[1].length - a[1].length)) {
      console.log(`  ── ${prop} (${items.length} items) ──`);
      for (const it of items.slice(0, 3)) {
        console.log(`    [${it.item_id}] ${it.name}`);
      }
      if (items.length > 3) console.log(`    ... and ${items.length - 3} more`);
    }
  }

  console.log('\n=== AUDIT V3 COMPLETE ===');

  await pool.end();
}

main().catch(err => {
  console.error('FATAL:', err);
  pool.end();
  process.exit(1);
});
