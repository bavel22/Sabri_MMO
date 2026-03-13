#!/usr/bin/env node
/**
 * audit_item_v2.js
 *
 * Comprehensive 3-source audit: DB stat columns vs full_description text
 * for ALL equipment items in the sabri_mmo database.
 *
 * Compares:
 *   1. DB stat columns (the actual gameplay values)
 *   2. full_description column (parsed for stat mentions)
 *
 * Detects:
 *   - Mismatches: description says X but DB has Y
 *   - Missing in desc: DB has nonzero value, description doesn't mention it
 *   - Extra in desc: description mentions stat, DB has 0
 *
 * Usage: node scripts/audit_item_v2.js
 * Output: scripts/output/item_audit_v2_report.json + stdout summary
 */

const fs   = require('fs');
const path = require('path');
const { Pool } = require('C:/Sabri_MMO/server/node_modules/pg');

// ============================================================
// Database connection
// ============================================================
const pool = new Pool({
  host: 'localhost',
  port: 5432,
  database: 'sabri_mmo',
  user: 'postgres',
  password: 'goku22',
});

const OUTPUT_PATH = path.join(__dirname, 'output', 'item_audit_v2_report.json');

// ============================================================
// Description parsing — extract stat values from full_description text
// ============================================================

/**
 * Parse a full_description text and extract all stat references.
 * Returns { base: {}, flat: {}, rate: {}, special: {} }
 *
 * IMPORTANT parsing rules:
 * 1. Rental items often list stats TWICE (header blurb + bonus section).
 *    We deduplicate by only parsing the BONUS SECTION (between the first
 *    pair of ________________________ dividers).
 * 2. "Reduces after attack delay by N%" can appear as a "Random chance"
 *    proc — we only count the first (permanent) instance.
 * 3. Cast time: "Reduces cast time by N%" → DB stores as NEGATIVE (e.g., -3).
 *    We store the parsed value as negative to match DB convention.
 */
function parseDescription(fullDesc) {
  if (!fullDesc) return null;

  // Split on ________________________ dividers to isolate sections
  const sections = fullDesc.split(/_{10,}/);
  // Typical layout:
  //   sections[0] = flavor text (may include rental header stats)
  //   sections[1] = bonus section (STR +2, DEF +5, etc.)
  //   sections[2] = type/attack/defense/weight section
  //   sections[3] = requirements section
  // For items with no bonuses, sections[1] may be the type section directly.

  // The BONUS section is the one that comes BEFORE the "Type:" line.
  // The TYPE section contains "Type:", "Attack:", "Defense:", "Weight:" lines.
  let bonusSection = '';
  let typeSection = '';

  for (let i = 0; i < sections.length; i++) {
    const s = sections[i].trim();
    if (/^\s*Type\s*:/m.test(s)) {
      typeSection = s;
      // The bonus section is the one right before this, if it exists
      if (i > 0) {
        bonusSection = sections[i - 1].trim();
      }
      break;
    }
  }

  // If no type section found, use the full text (shouldn't happen for equipment)
  const fullText = fullDesc;

  const result = {
    base: {},    // "Attack: N", "Defense: N"
    flat: {},    // "STR +N", "MaxHP +N", etc.
    rate: {},    // "MATK +N%", "MaxHP +N%", etc.
    special: {}, // "Increases healing by N%", "SP consumption +N%", etc.
  };

  // ── Base stats (from the TYPE section: "Attack: N", "Defense: N") ──
  const atkMatch = typeSection.match(/Attack\s*:\s*(\d+)/i);
  if (atkMatch) result.base.atk = parseInt(atkMatch[1], 10);

  const defMatch = typeSection.match(/Defense\s*:\s*(\d+)/i);
  if (defMatch) result.base.def = parseInt(defMatch[1], 10);

  // ── Parse from BONUS section only (avoids rental header duplication) ──
  const text = bonusSection;

  // ── "All Stats +N" → expands to STR/AGI/VIT/INT/DEX/LUK ──
  const allStatsRe = /All\s+Stats?\s+([+-])\s*(\d+)/gi;
  let asm;
  while ((asm = allStatsRe.exec(text)) !== null) {
    const sign = asm[1] === '-' ? -1 : 1;
    const val = parseInt(asm[2], 10) * sign;
    for (const s of ['str', 'agi', 'vit', 'int', 'dex', 'luk']) {
      result.flat[s] = (result.flat[s] || 0) + val;
    }
  }

  // ── Flat bonus stats (STR +N, AGI -N, etc.) ──
  // Must exclude percentage matches (where followed by %)
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
    { key: 'matk_flat',        re: /\bMATK\s+([+-])\s*(\d+)/gi },  // flat MATK (no %)
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
      // Check if followed by % — if so, it's a rate, not flat
      const afterIdx = m.index + m[0].length;
      const afterChar = text.substring(afterIdx).match(/^\s*(%)/);
      if (afterChar) {
        // This is a percentage — store in rate instead
        if (key === 'matk_flat') {
          result.rate['matk_rate'] = (result.rate['matk_rate'] || 0) + val;
        } else if (key === 'max_hp') {
          result.rate['max_hp_rate'] = (result.rate['max_hp_rate'] || 0) + val;
        } else if (key === 'max_sp') {
          result.rate['max_sp_rate'] = (result.rate['max_sp_rate'] || 0) + val;
        } else if (key === 'atk_bonus') {
          result.rate['atk_rate'] = (result.rate['atk_rate'] || 0) + val;
        }
        continue;
      }
      result.flat[key] = (result.flat[key] || 0) + val;
    }
  }

  // ── "Reduces after attack delay by N%" ──
  // Can be positive or negative: "by 3%", "by -25%"
  // IMPORTANT: Skip lines with "Random chance" or "chance to" — those are procs, not permanent.
  // Only count the FIRST non-proc match.
  const aspdDelayRe = /Reduces?\s+(?:after\s+)?attack\s+delay\s+by\s+(-?\d+)\s*%/gi;
  let asdm;
  while ((asdm = aspdDelayRe.exec(text)) !== null) {
    // Check if preceded by "chance to" or "Random" on the same line
    const lineStart = text.lastIndexOf('\n', asdm.index);
    const linePrefix = text.substring(lineStart + 1, asdm.index);
    if (/(?:chance\s+to|Random)/i.test(linePrefix)) {
      continue; // Skip proc-based ASPD changes
    }
    result.rate['aspd_rate'] = (result.rate['aspd_rate'] || 0) + parseInt(asdm[1], 10);
    break; // Only count the first permanent instance
  }

  // ── "HP recovery +N%" / "SP recovery +N%" ──
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

  // ── "Reduces cast time by N%" ──
  // DB stores as NEGATIVE (e.g., -3 means "3% faster casting").
  // We store as NEGATIVE to match DB convention.
  const castRe = /Reduces?\s+cast\s+time\s+by\s+(\d+)\s*%/gi;
  let crm;
  while ((crm = castRe.exec(text)) !== null) {
    result.rate['cast_rate'] = (result.rate['cast_rate'] || 0) - parseInt(crm[1], 10);
  }

  // ── "SP consumption +N%" / "SP consumption -N%" ──
  const spConsRe = /SP\s+consumption\s+([+-])\s*(\d+)\s*%/gi;
  let scm;
  while ((scm = spConsRe.exec(text)) !== null) {
    const sign = scm[1] === '-' ? -1 : 1;
    result.rate['use_sp_rate'] = (result.rate['use_sp_rate'] || 0) + parseInt(scm[2], 10) * sign;
  }

  // ── "Increases healing by N%" ──
  const healRe = /Increases?\s+healing\s+(?:effectiveness\s+)?by\s+(\d+)\s*%/gi;
  let hm;
  while ((hm = healRe.exec(text)) !== null) {
    result.special['heal_power'] = (result.special['heal_power'] || 0) + parseInt(hm[1], 10);
  }

  // ── "Increases critical damage by N%" ──
  const critDmgRe = /Increases?\s+critical\s+damage\s+by\s+(\d+)\s*%/gi;
  let cdm;
  while ((cdm = critDmgRe.exec(text)) !== null) {
    result.rate['crit_atk_rate'] = (result.rate['crit_atk_rate'] || 0) + parseInt(cdm[1], 10);
  }

  // ── "Restores N SP every" / "Restores N HP every" (flat drain, not rate — skip for now) ──

  return result;
}

// ============================================================
// Comparison logic
// ============================================================

/**
 * Map from description parsed keys to DB column names.
 * Each entry: { descKey, descCategory, dbColumn, label }
 */
const COMPARISON_MAP = [
  // Base stats
  { descKey: 'atk',           descCat: 'base', dbCol: 'atk',               label: 'Base ATK (Attack: N)' },
  { descKey: 'def',           descCat: 'base', dbCol: 'def',               label: 'Base DEF (Defense: N)' },

  // Flat bonuses → DB columns
  { descKey: 'str',           descCat: 'flat', dbCol: 'str_bonus',          label: 'STR bonus' },
  { descKey: 'agi',           descCat: 'flat', dbCol: 'agi_bonus',          label: 'AGI bonus' },
  { descKey: 'vit',           descCat: 'flat', dbCol: 'vit_bonus',          label: 'VIT bonus' },
  { descKey: 'int',           descCat: 'flat', dbCol: 'int_bonus',          label: 'INT bonus' },
  { descKey: 'dex',           descCat: 'flat', dbCol: 'dex_bonus',          label: 'DEX bonus' },
  { descKey: 'luk',           descCat: 'flat', dbCol: 'luk_bonus',          label: 'LUK bonus' },
  { descKey: 'mdef',          descCat: 'flat', dbCol: 'mdef',              label: 'MDEF bonus' },
  { descKey: 'hit',           descCat: 'flat', dbCol: 'hit_bonus',          label: 'HIT bonus' },
  { descKey: 'flee',          descCat: 'flat', dbCol: 'flee_bonus',         label: 'Flee bonus' },
  { descKey: 'critical',      descCat: 'flat', dbCol: 'critical_bonus',     label: 'Critical bonus' },
  { descKey: 'perfect_dodge', descCat: 'flat', dbCol: 'perfect_dodge_bonus',label: 'Perfect Dodge bonus' },
  { descKey: 'max_hp',        descCat: 'flat', dbCol: 'max_hp_bonus',       label: 'MaxHP flat bonus' },
  { descKey: 'max_sp',        descCat: 'flat', dbCol: 'max_sp_bonus',       label: 'MaxSP flat bonus' },

  // Rate bonuses → DB columns
  { descKey: 'aspd_rate',     descCat: 'rate', dbCol: 'aspd_rate',          label: 'ASPD rate (after attack delay)' },
  { descKey: 'hp_regen_rate', descCat: 'rate', dbCol: 'hp_regen_rate',      label: 'HP recovery rate %' },
  { descKey: 'sp_regen_rate', descCat: 'rate', dbCol: 'sp_regen_rate',      label: 'SP recovery rate %' },
  { descKey: 'cast_rate',     descCat: 'rate', dbCol: 'cast_rate',          label: 'Cast time reduction %' },
  { descKey: 'use_sp_rate',   descCat: 'rate', dbCol: 'use_sp_rate',        label: 'SP consumption rate %' },
  { descKey: 'max_hp_rate',   descCat: 'rate', dbCol: 'max_hp_rate',        label: 'MaxHP rate %' },
  { descKey: 'max_sp_rate',   descCat: 'rate', dbCol: 'max_sp_rate',        label: 'MaxSP rate %' },
  { descKey: 'crit_atk_rate', descCat: 'rate', dbCol: 'crit_atk_rate',      label: 'Critical ATK rate %' },
  { descKey: 'matk_rate',     descCat: 'rate', dbCol: null,                 label: 'MATK rate % (info only)' },

  // Special
  { descKey: 'heal_power',    descCat: 'special', dbCol: 'heal_power',      label: 'Heal power %' },
];

// DB columns that have no direct description mapping but we still check
// if they are nonzero (to flag "DB has value, desc doesn't mention it")
const DB_ONLY_COLUMNS = [
  // atk_bonus and def_bonus in desc are tricky — ATK +N in desc can mean
  // the script bonus added to items.atk, not a separate column.
  // We handle these specially.
];

// Description keys that map to ATK/DEF bonus from script (not base ATK/DEF)
// "ATK +N" in the bonus section = items.atk includes base + script bBaseAtk
// "DEF +N" in bonus section = items.def includes base + script bDef
// These are informational — we note them but don't flag as errors

/**
 * Compare one item's description-parsed stats against DB values.
 * Returns { mismatches: [], missing_in_desc: [], extra_in_desc: [] }
 */
function compareItem(dbRow, descParsed) {
  const mismatches = [];
  const missing_in_desc = [];
  const extra_in_desc = [];

  for (const mapping of COMPARISON_MAP) {
    const descVal = descParsed[mapping.descCat]?.[mapping.descKey] ?? null;
    const dbVal = mapping.dbCol ? (dbRow[mapping.dbCol] || 0) : null;

    // Skip MATK rate — no direct DB column (matk column is flat, rates aren't stored there)
    if (mapping.dbCol === null) {
      // Info-only: just note if description mentions it
      if (descVal !== null && descVal !== 0) {
        // This is informational — MATK +N% is in desc but we don't have a matk_rate DB col
        // (it might be in script). Don't flag as error.
      }
      continue;
    }

    if (descVal !== null && descVal !== 0) {
      // Description mentions this stat
      if (dbVal === 0) {
        extra_in_desc.push({
          stat: mapping.label,
          db_column: mapping.dbCol,
          desc_value: descVal,
          db_value: dbVal,
        });
      } else if (descVal !== dbVal) {
        mismatches.push({
          stat: mapping.label,
          db_column: mapping.dbCol,
          desc_value: descVal,
          db_value: dbVal,
          diff: descVal - dbVal,
        });
      }
    } else {
      // Description does NOT mention this stat
      if (dbVal !== 0) {
        missing_in_desc.push({
          stat: mapping.label,
          db_column: mapping.dbCol,
          db_value: dbVal,
          desc_value: descVal,
        });
      }
    }
  }

  // ── Special handling: "ATK +N" bonus vs base ATK ──
  // "ATK +N" in description bonus section means the SCRIPT adds bBaseAtk.
  // items.atk = YAML Attack + script bBaseAtk.
  // "Attack: N" = usually YAML base Attack.
  // If desc has BOTH "Attack: N" and "ATK +N", then items.atk should = N + bonus.
  const descBaseAtk = descParsed.base.atk ?? null;
  const descBonusAtk = descParsed.flat.atk_bonus ?? null;
  const dbAtk = dbRow.atk || 0;

  if (descBaseAtk !== null && descBonusAtk !== null) {
    // Both present: items.atk should = descBaseAtk (YAML) + descBonusAtk (script bBaseAtk)?
    // Actually no — items.atk in DB = YAML Attack + bBaseAtk from script.
    // "Attack: N" in desc = the official tooltip value (which is YAML Attack).
    // "ATK +N" = the bonus from script.
    // So DB atk = descBaseAtk (base) — the bBaseAtk is ALREADY included in items.atk
    // BUT sometimes desc "Attack:" shows the YAML base, and items.atk includes bBaseAtk too.
    // This is complex — we already flag via base ATK comparison. Note the ATK +N as context.
  }

  // "DEF +N" in bonus section — similar to ATK. Items.def = YAML Defense + bDef from script.
  // "Defense: N" in desc = YAML base Defense value.
  // If desc has both "Defense: N" and "DEF +N", items.def likely = Defense + DEF_bonus.
  const descBaseDef = descParsed.base.def ?? null;
  const descBonusDef = descParsed.flat.def_bonus ?? null;
  const dbDef = dbRow.def || 0;

  if (descBonusDef !== null && descBonusDef !== 0) {
    if (descBaseDef !== null) {
      // Check if DB def = base + bonus
      const expectedDef = descBaseDef + descBonusDef;
      if (dbDef === expectedDef) {
        // Perfect — DB includes both. Remove any base DEF mismatch we flagged.
        const idx = mismatches.findIndex(m => m.db_column === 'def' && m.stat.includes('Base DEF'));
        if (idx >= 0) {
          mismatches[idx].note = `DB def (${dbDef}) = desc base (${descBaseDef}) + desc bonus DEF (${descBonusDef}). This is expected.`;
          mismatches[idx].resolved = true;
        }
      }
    }
    // If DEF +N is in desc but items.def doesn't reflect it, it's a real mismatch
    // (already caught by base DEF comparison)
  }

  // Similarly for ATK +N bonus
  if (descBonusAtk !== null && descBonusAtk !== 0) {
    if (descBaseAtk !== null) {
      const expectedAtk = descBaseAtk + descBonusAtk;
      // If DB atk is just the base (YAML Attack), that's also valid — script adds separately
      // If DB atk = base + bonus, that means bBaseAtk was folded in
    }
    // Note: "ATK +N" in desc usually means script has `bonus bBaseAtk,N;`
    // which gets added to items.atk during migration. So items.atk = YAML Attack + N.
    // The "Attack: X" line in desc is the tooltip base (sometimes = YAML Attack, sometimes different).
  }

  // ── Special: flat MATK (MATK +N without %) ──
  // Description "MATK +N" (flat, no %) — this is bMatk bonus.
  // items.matk should be 0 for pre-renewal (cleared), so flat MATK is in the script.
  // Flag if desc has flat MATK but DB matk is different.
  const descMatkFlat = descParsed.flat.matk_flat ?? null;
  const dbMatk = dbRow.matk || 0;
  if (descMatkFlat !== null && descMatkFlat !== 0) {
    if (dbMatk === 0) {
      extra_in_desc.push({
        stat: 'MATK flat bonus',
        db_column: 'matk',
        desc_value: descMatkFlat,
        db_value: dbMatk,
        note: 'Description has flat MATK +N but DB matk=0. May be in script only.',
      });
    } else if (dbMatk !== descMatkFlat) {
      mismatches.push({
        stat: 'MATK flat bonus',
        db_column: 'matk',
        desc_value: descMatkFlat,
        db_value: dbMatk,
      });
    }
  } else if (dbMatk !== 0 && descMatkFlat === null) {
    // DB has matk but desc doesn't mention flat MATK
    // Check if desc has MATK +N% (rate) — that's a different thing
    const descMatkRate = descParsed.rate.matk_rate ?? null;
    if (descMatkRate === null) {
      missing_in_desc.push({
        stat: 'MATK (flat)',
        db_column: 'matk',
        db_value: dbMatk,
        desc_value: null,
      });
    }
  }

  // ── Special: "ATK +N" bonus ──
  // This is tricky: desc "ATK +N" = script bBaseAtk.
  // In DB, items.atk = YAML Attack + bBaseAtk.
  // We already compared "Attack: X" (base) vs items.atk.
  // If ATK +N is in desc, items.atk should be > "Attack: X" by that amount.
  // Flag ATK +N presence for info but handle carefully.
  if (descBonusAtk !== null && descBonusAtk !== 0 && descBaseAtk !== null) {
    const expectedTotal = descBaseAtk + descBonusAtk;
    if (dbAtk !== expectedTotal && dbAtk !== descBaseAtk) {
      // Already flagged via base ATK mismatch — add note
      const idx = mismatches.findIndex(m => m.db_column === 'atk' && m.stat.includes('Base ATK'));
      if (idx >= 0) {
        mismatches[idx].note = `Desc has both "Attack: ${descBaseAtk}" and "ATK +${descBonusAtk}". DB atk=${dbAtk}. Expected base=${descBaseAtk} or total=${expectedTotal}.`;
      }
    } else if (dbAtk === expectedTotal) {
      // DB atk = base + bonus — mark base mismatch as resolved
      const idx = mismatches.findIndex(m => m.db_column === 'atk' && m.stat.includes('Base ATK'));
      if (idx >= 0) {
        mismatches[idx].note = `DB atk (${dbAtk}) = desc base (${descBaseAtk}) + desc bonus ATK (${descBonusAtk}). This is expected.`;
        mismatches[idx].resolved = true;
      }
    }
  }

  // ── Special: ASPD flat (ASPD +N without %) ──
  // "ASPD +N" flat — this is bAspd (flat ASPD increase), distinct from aspd_rate
  // DB has aspd_rate but not a separate aspd_flat column.
  const descAspdFlat = descParsed.flat.aspd_flat ?? null;
  if (descAspdFlat !== null && descAspdFlat !== 0) {
    // No DB column for flat ASPD — note as extra
    extra_in_desc.push({
      stat: 'ASPD flat bonus (no DB column)',
      db_column: 'aspd_rate (closest)',
      desc_value: descAspdFlat,
      db_value: dbRow.aspd_rate || 0,
      note: 'Description has flat ASPD +N. DB only has aspd_rate (%). These are different stats.',
    });
  }

  // ── Special: "DEF +N" bonus without base Defense ──
  if (descBonusDef !== null && descBonusDef !== 0 && descBaseDef === null) {
    // Desc mentions DEF +N but no "Defense: N" line
    // The DB def should include this bonus. Check if DB def = bonus.
    // Already handled: MDEF/DEF comparison maps DEF bonus vs def_bonus...
    // Wait — we don't have a separate def_bonus column. items.def = combined.
    // And our comparison map compares desc "def_bonus" (DEF +N) against items.mdef? No.
    // Actually DEF +N isn't in our COMPARISON_MAP for flat → def column.
    // items.def includes YAML Defense + script bDef. "DEF +N" = the script part.
    // This is inherently complex. Note it.
  }

  return {
    mismatches: mismatches.filter(m => !m.resolved),
    resolved_mismatches: mismatches.filter(m => m.resolved),
    missing_in_desc,
    extra_in_desc,
  };
}

// ============================================================
// Main
// ============================================================
async function main() {
  console.log('=== Item Audit V2: DB Columns vs full_description ===\n');

  // Fetch ALL equipment items
  const query = `
    SELECT
      item_id, name, description, item_type, equip_slot, weapon_type, sub_type,
      atk, def, matk, mdef,
      str_bonus, agi_bonus, vit_bonus, int_bonus, dex_bonus, luk_bonus,
      max_hp_bonus, max_sp_bonus, hit_bonus, flee_bonus, critical_bonus,
      perfect_dodge_bonus, aspd_rate, hp_regen_rate, sp_regen_rate,
      cast_rate, use_sp_rate, heal_power, max_hp_rate, max_sp_rate, crit_atk_rate,
      full_description, script
    FROM items
    WHERE item_type IN ('weapon', 'armor')
    ORDER BY item_id
  `;

  const { rows } = await pool.query(query);
  console.log(`Fetched ${rows.length} equipment items from DB\n`);

  const results = {
    mismatches: [],
    missing_in_desc: [],
    extra_in_desc: [],
    no_description: [],
    perfect_match: [],
  };

  // Per-stat counters
  const statCounters = {
    mismatch: {},
    missing: {},
    extra: {},
  };

  function incrStat(category, statLabel) {
    if (!statCounters[category][statLabel]) statCounters[category][statLabel] = 0;
    statCounters[category][statLabel]++;
  }

  for (const row of rows) {
    if (!row.full_description) {
      results.no_description.push({
        item_id: row.item_id,
        name: row.name,
        item_type: row.item_type,
        equip_slot: row.equip_slot,
      });
      continue;
    }

    const descParsed = parseDescription(row.full_description);
    if (!descParsed) continue;

    const comparison = compareItem(row, descParsed);

    const itemSummary = {
      item_id: row.item_id,
      name: row.name,
      item_type: row.item_type,
      equip_slot: row.equip_slot,
      weapon_type: row.weapon_type || null,
    };

    if (comparison.mismatches.length > 0) {
      results.mismatches.push({
        ...itemSummary,
        issues: comparison.mismatches,
        desc_snippet: row.full_description.substring(0, 400),
      });
      for (const m of comparison.mismatches) incrStat('mismatch', m.stat);
    }

    if (comparison.missing_in_desc.length > 0) {
      results.missing_in_desc.push({
        ...itemSummary,
        issues: comparison.missing_in_desc,
      });
      for (const m of comparison.missing_in_desc) incrStat('missing', m.stat);
    }

    if (comparison.extra_in_desc.length > 0) {
      results.extra_in_desc.push({
        ...itemSummary,
        issues: comparison.extra_in_desc,
      });
      for (const m of comparison.extra_in_desc) incrStat('extra', m.stat);
    }

    if (comparison.mismatches.length === 0 &&
        comparison.missing_in_desc.length === 0 &&
        comparison.extra_in_desc.length === 0) {
      results.perfect_match.push(itemSummary);
    }
  }

  // ============================================================
  // Build report
  // ============================================================
  const report = {
    summary: {
      total_equipment: rows.length,
      with_description: rows.length - results.no_description.length,
      no_description: results.no_description.length,
      perfect_match: results.perfect_match.length,
      has_mismatch: results.mismatches.length,
      has_missing_in_desc: results.missing_in_desc.length,
      has_extra_in_desc: results.extra_in_desc.length,
    },
    stats: {
      mismatch_by_stat: statCounters.mismatch,
      missing_by_stat: statCounters.missing,
      extra_by_stat: statCounters.extra,
    },
    mismatches: results.mismatches,
    missing_in_desc: results.missing_in_desc,
    extra_in_desc: results.extra_in_desc,
    no_description: results.no_description,
    // Don't include perfect_match items in JSON (too many) — just the count
  };

  // Ensure output dir
  const outDir = path.dirname(OUTPUT_PATH);
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

  fs.writeFileSync(OUTPUT_PATH, JSON.stringify(report, null, 2));
  console.log(`Report written to: ${OUTPUT_PATH}\n`);

  // ============================================================
  // Human-readable stdout summary
  // ============================================================
  console.log('╔══════════════════════════════════════════════════════╗');
  console.log('║              ITEM AUDIT V2 — SUMMARY                ║');
  console.log('╚══════════════════════════════════════════════════════╝\n');

  console.log(`Total equipment items:       ${report.summary.total_equipment}`);
  console.log(`  With full_description:     ${report.summary.with_description}`);
  console.log(`  Missing full_description:  ${report.summary.no_description}`);
  console.log(`  Perfect match (no issues): ${report.summary.perfect_match}`);
  console.log(`  Has MISMATCH:              ${report.summary.has_mismatch}`);
  console.log(`  Has MISSING in desc:       ${report.summary.has_missing_in_desc}`);
  console.log(`  Has EXTRA in desc:         ${report.summary.has_extra_in_desc}`);

  console.log('\n── MISMATCH breakdown (desc value != DB value) ──');
  const mmSorted = Object.entries(statCounters.mismatch).sort((a, b) => b[1] - a[1]);
  if (mmSorted.length === 0) console.log('  (none)');
  for (const [stat, count] of mmSorted) {
    console.log(`  ${stat}: ${count}`);
  }

  console.log('\n── MISSING IN DESC (DB has value, desc silent) ──');
  const misSorted = Object.entries(statCounters.missing).sort((a, b) => b[1] - a[1]);
  if (misSorted.length === 0) console.log('  (none)');
  for (const [stat, count] of misSorted) {
    console.log(`  ${stat}: ${count}`);
  }

  console.log('\n── EXTRA IN DESC (desc mentions stat, DB has 0) ──');
  const extSorted = Object.entries(statCounters.extra).sort((a, b) => b[1] - a[1]);
  if (extSorted.length === 0) console.log('  (none)');
  for (const [stat, count] of extSorted) {
    console.log(`  ${stat}: ${count}`);
  }

  // ── Top 30 mismatches ──
  if (results.mismatches.length > 0) {
    console.log('\n── TOP 30 MISMATCHES (sample) ──');
    for (const item of results.mismatches.slice(0, 30)) {
      for (const iss of item.issues) {
        const note = iss.note ? ` (${iss.note})` : '';
        console.log(`  [${item.item_id}] ${item.name} — ${iss.stat}: desc=${iss.desc_value} DB=${iss.db_value}${note}`);
      }
    }
    if (results.mismatches.length > 30) {
      console.log(`  ... and ${results.mismatches.length - 30} more items with mismatches`);
    }
  }

  // ── Top 20 missing in desc ──
  if (results.missing_in_desc.length > 0) {
    console.log('\n── TOP 20 MISSING IN DESC (DB has value, desc silent) ──');
    for (const item of results.missing_in_desc.slice(0, 20)) {
      for (const iss of item.issues) {
        console.log(`  [${item.item_id}] ${item.name} — ${iss.stat}: DB=${iss.db_value}, desc doesn't mention`);
      }
    }
    if (results.missing_in_desc.length > 20) {
      console.log(`  ... and ${results.missing_in_desc.length - 20} more items`);
    }
  }

  // ── Top 20 extra in desc ──
  if (results.extra_in_desc.length > 0) {
    console.log('\n── TOP 20 EXTRA IN DESC (desc mentions stat, DB has 0) ──');
    for (const item of results.extra_in_desc.slice(0, 20)) {
      for (const iss of item.issues) {
        const note = iss.note ? ` — ${iss.note}` : '';
        console.log(`  [${item.item_id}] ${item.name} — ${iss.stat}: desc=${iss.desc_value}, DB=${iss.db_value}${note}`);
      }
    }
    if (results.extra_in_desc.length > 20) {
      console.log(`  ... and ${results.extra_in_desc.length - 20} more items`);
    }
  }

  console.log('\n=== AUDIT COMPLETE ===');

  await pool.end();
}

main().catch(err => {
  console.error('FATAL:', err);
  pool.end();
  process.exit(1);
});
