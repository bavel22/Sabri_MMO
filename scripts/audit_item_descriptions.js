/**
 * audit_item_descriptions.js
 *
 * Compares rAthena YAML item stats (Attack, Defense, MagicAttack, and
 * Script-based stat bonuses) against the numbers found in the
 * item_descriptions.json fullDescription text.
 *
 * Usage:  node scripts/audit_item_descriptions.js
 * Output: scripts/output/item_audit_report.json
 */

const fs   = require('fs');
const path = require('path');
const yaml = require('js-yaml');

// ────────────────────────────────────────────
// Paths
// ────────────────────────────────────────────
const YAML_PATH  = path.join(__dirname, '..', 'docsNew', 'items', 'item_db_equip.yml');
const DESC_PATH  = path.join(__dirname, '..', 'docsNew', 'items', 'item_descriptions.json');
const OUT_PATH   = path.join(__dirname, 'output', 'item_audit_report.json');

// ────────────────────────────────────────────
// 1. Load rAthena YAML
// ────────────────────────────────────────────
console.log('Loading YAML...');
const yamlText  = fs.readFileSync(YAML_PATH, 'utf8');
// rAthena uses multiple YAML documents separated by ---
// The item data is in the document with Header.Type == ITEM_DB
const docs = yaml.loadAll(yamlText);
let yamlItems = [];
for (const doc of docs) {
  if (doc && doc.Body && Array.isArray(doc.Body)) {
    yamlItems = yamlItems.concat(doc.Body);
  }
}
console.log(`  Loaded ${yamlItems.length} items from YAML`);

// ────────────────────────────────────────────
// 2. Load descriptions
// ────────────────────────────────────────────
console.log('Loading descriptions...');
const descData = JSON.parse(fs.readFileSync(DESC_PATH, 'utf8'));
const descriptions = descData.items; // keyed by item_id string
console.log(`  Loaded ${Object.keys(descriptions).length} descriptions`);

// ────────────────────────────────────────────
// 3. Filter to equipment only (Weapon or Armor)
// ────────────────────────────────────────────
const equipItems = yamlItems.filter(i => {
  const t = (i.Type || '').toLowerCase();
  return t === 'weapon' || t === 'armor';
});
console.log(`  ${equipItems.length} equipment items (Weapon + Armor)`);

// ────────────────────────────────────────────
// 4. Parse the YAML Script field for stat bonuses
//    rAthena bonus syntax: bonus bStr,2; bonus bAgi,1; etc.
// ────────────────────────────────────────────
const SCRIPT_BONUS_MAP = {
  bstr:  'str',  bagi:  'agi',  bvit:  'vit',
  bint:  'int',  bdex:  'dex',  bluk:  'luk',
  bdef:  'def',  bdef2: 'def',  bmdef: 'mdef', bmdef2: 'mdef',
  bhit:  'hit',  bflee: 'flee', bflee2: 'flee',
  bcritical: 'crit',
  bmaxhp: 'maxhp', bmaxsp: 'maxsp',
  bbaseatk: 'atk', batkrate: 'atk_rate', bmatk: 'matk',
  bmatkrate: 'matk_rate',
  bsplashrange: 'splash',
  bperfectdodgeadd: 'perfect_dodge',
  bcastrate: 'cast_rate',
  baspdrate: 'aspd_rate',
  baspd: 'aspd',
  bhprecovrate: 'hp_regen_rate',
  bsprecovrate: 'sp_regen_rate',
  bhpgainvalue: 'hp_drain',
  bspgainvalue: 'sp_drain',
  blongatkrate: 'long_range_rate',
  bshortatkrate: 'short_range_rate',
  bcritatkrate: 'crit_damage_rate',
  ballstats: 'all_stats',
  batkele: 'element',
};

function parseScriptBonuses(scriptText) {
  if (!scriptText) return {};
  const bonuses = {};
  // Match: bonus bStat,value;
  const re = /bonus\s+(b\w+)\s*,\s*(-?\d+)\s*;/gi;
  let m;
  while ((m = re.exec(scriptText)) !== null) {
    const key = m[1].toLowerCase();
    const val = parseInt(m[2], 10);
    const statName = SCRIPT_BONUS_MAP[key];
    if (statName && statName !== 'element') {
      bonuses[statName] = (bonuses[statName] || 0) + val;
    }
  }
  return bonuses;
}

// ────────────────────────────────────────────
// 5. Parse description text for stat values
//    Two kinds of numbers:
//    a) Base stats:   "Attack: 130"  "Defense: 30"
//    b) Bonus stats:  "STR +2"  "DEF +5"  "ATK +10"  "MATK +105"
// ────────────────────────────────────────────
const DESC_BASE_PATTERNS = [
  { key: 'desc_base_atk', re: /Attack\s*:\s*(\d+)/i },
  { key: 'desc_base_def', re: /Defense\s*:\s*(\d+)/i },
];

const DESC_BONUS_PATTERNS = [
  { key: 'atk',   re: /ATK\s*\+\s*(\d+)/gi },
  { key: 'matk',  re: /MATK\s*\+\s*(\d+)/gi },
  { key: 'def',   re: /DEF\s*\+\s*(\d+)/gi },
  { key: 'mdef',  re: /MDEF\s*\+\s*(\d+)/gi },
  { key: 'str',   re: /STR\s*\+\s*(\d+)/gi },
  { key: 'agi',   re: /AGI\s*\+\s*(\d+)/gi },
  { key: 'vit',   re: /VIT\s*\+\s*(\d+)/gi },
  { key: 'int',   re: /INT\s*\+\s*(\d+)/gi },
  { key: 'dex',   re: /DEX\s*\+\s*(\d+)/gi },
  { key: 'luk',   re: /LUK\s*\+\s*(\d+)/gi },
  { key: 'hit',   re: /HIT\s*\+\s*(\d+)/gi },
  { key: 'flee',  re: /FLEE\s*\+\s*(\d+)/gi },
  { key: 'crit',  re: /Critical\s*\+\s*(\d+)/gi },
  { key: 'maxhp', re: /MaxHP\s*\+\s*(\d+)/gi },
  { key: 'maxsp', re: /MaxSP\s*\+\s*(\d+)/gi },
  { key: 'perfect_dodge', re: /Perfect\s*Dodge\s*\+\s*(\d+)/gi },
  { key: 'aspd',  re: /ASPD\s*\+\s*(\d+)/gi },
];

// Also match percentage patterns: "ATK +10%", "MATK +15%"
const DESC_RATE_PATTERNS = [
  { key: 'atk_rate',   re: /ATK\s*\+\s*(\d+)\s*%/gi },
  { key: 'matk_rate',  re: /MATK\s*\+\s*(\d+)\s*%/gi },
  { key: 'hp_regen_rate', re: /HP\s*(?:recovery|regen)\s*\+\s*(\d+)\s*%/gi },
  { key: 'sp_regen_rate', re: /SP\s*(?:recovery|regen)\s*\+\s*(\d+)\s*%/gi },
  { key: 'cast_rate',  re: /Cast\s*(?:Time|Delay)\s*-\s*(\d+)\s*%/gi },
];

function parseDescriptionStats(fullDescription) {
  if (!fullDescription || !Array.isArray(fullDescription)) return null;
  const text = fullDescription.join('\n');

  const result = { base: {}, bonuses: {}, rates: {} };

  // Base stats (Attack: N, Defense: N)
  for (const { key, re } of DESC_BASE_PATTERNS) {
    const m = text.match(re);
    if (m) result.base[key] = parseInt(m[1], 10);
  }

  // Bonus stats (STR +2, DEF +5, etc.)
  // Need to be careful: "DEF +5" is a bonus, "Defense: 30" is the base.
  // Also need to exclude percentage matches from flat matches.
  for (const { key, re } of DESC_BONUS_PATTERNS) {
    re.lastIndex = 0;
    let m;
    let total = 0;
    let found = false;
    while ((m = re.exec(text)) !== null) {
      // Check if followed by % — if so, skip (it's a rate, not flat)
      const afterMatch = text.substring(m.index + m[0].length);
      if (afterMatch.match(/^\s*%/)) continue;
      total += parseInt(m[1], 10);
      found = true;
    }
    if (found) result.bonuses[key] = total;
  }

  // Rate patterns
  for (const { key, re } of DESC_RATE_PATTERNS) {
    re.lastIndex = 0;
    let m;
    let total = 0;
    let found = false;
    while ((m = re.exec(text)) !== null) {
      total += parseInt(m[1], 10);
      found = true;
    }
    if (found) result.rates[key] = total;
  }

  return result;
}

// ────────────────────────────────────────────
// 6. Compare and build report
// ────────────────────────────────────────────
console.log('Comparing stats...');

const mismatches = [];
const missingDescriptions = [];
const statMismatches = [];
let totalEquip = equipItems.length;
let totalWithDescMismatch = 0;
let totalMissingDesc = 0;

// Helper: determine equip_slot from YAML Locations
function getEquipSlot(item) {
  const loc = item.Locations;
  if (!loc) return 'unknown';
  const keys = Object.keys(loc).map(k => k.toLowerCase());
  if (keys.includes('right_hand') || keys.includes('both_hand')) return 'weapon';
  if (keys.includes('left_hand')) return 'shield';
  if (keys.includes('armor') || keys.includes('body')) return 'armor';
  if (keys.includes('head_top')) return 'head_top';
  if (keys.includes('head_mid')) return 'head_mid';
  if (keys.includes('head_low')) return 'head_low';
  if (keys.includes('garment') || keys.includes('robe')) return 'garment';
  if (keys.includes('shoes') || keys.includes('footgear')) return 'footgear';
  if (keys.includes('accessory') || keys.includes('right_accessory') || keys.includes('left_accessory') || keys.includes('both_accessory')) return 'accessory';
  return keys.join(',');
}

for (const item of equipItems) {
  const id      = item.Id;
  const name    = item.Name || '';
  const type    = (item.Type || '').toLowerCase();
  const slot    = getEquipSlot(item);
  const yamlAtk = item.Attack  || 0;
  const yamlDef = item.Defense || 0;
  const yamlMatk = item.MagicAttack || 0;
  const yamlScriptBonuses = parseScriptBonuses(item.Script || '');

  const desc = descriptions[String(id)];
  if (!desc || !desc.fullDescription || desc.fullDescription.length === 0) {
    totalMissingDesc++;
    missingDescriptions.push({
      item_id: id,
      name: name,
      item_type: type,
      equip_slot: slot,
      yaml_atk: yamlAtk,
      yaml_def: yamlDef,
      yaml_matk: yamlMatk,
    });
    continue;
  }

  const descStats = parseDescriptionStats(desc.fullDescription);
  if (!descStats) continue;

  const fullText = desc.fullDescription.join('\n');
  const snippet  = fullText.substring(0, 200);

  const itemMismatches = [];

  // ── Compare base ATK ──
  if (descStats.base.desc_base_atk !== undefined) {
    if (yamlAtk !== descStats.base.desc_base_atk) {
      itemMismatches.push({
        stat: 'base_atk',
        yaml_value: yamlAtk,
        description_value: descStats.base.desc_base_atk,
      });
    }
  }

  // ── Compare base DEF ──
  if (descStats.base.desc_base_def !== undefined) {
    if (yamlDef !== descStats.base.desc_base_def) {
      itemMismatches.push({
        stat: 'base_def',
        yaml_value: yamlDef,
        description_value: descStats.base.desc_base_def,
      });
    }
  }

  // ── Compare bonus stats from Script vs description ──
  // Collect all stat keys from both sources
  const allStatKeys = new Set([
    ...Object.keys(yamlScriptBonuses),
    ...Object.keys(descStats.bonuses),
  ]);

  for (const statKey of allStatKeys) {
    // Skip 'atk' and 'def' bonus comparisons — they overlap with base stats
    // and the description format is inconsistent (some show base, some show bonus)
    const yamlVal = yamlScriptBonuses[statKey] || 0;
    const descVal = descStats.bonuses[statKey] || 0;

    if (yamlVal !== descVal) {
      // Only report if at least one side has a non-zero value
      if (yamlVal !== 0 || descVal !== 0) {
        itemMismatches.push({
          stat: 'bonus_' + statKey,
          yaml_script_value: yamlVal,
          description_value: descVal,
        });
      }
    }
  }

  if (itemMismatches.length > 0) {
    totalWithDescMismatch++;
    mismatches.push({
      item_id: id,
      name: name,
      item_type: type,
      equip_slot: slot,
      yaml_atk: yamlAtk,
      yaml_def: yamlDef,
      yaml_matk: yamlMatk,
      yaml_script_bonuses: Object.keys(yamlScriptBonuses).length > 0 ? yamlScriptBonuses : undefined,
      description_stats: descStats,
      mismatched_fields: itemMismatches,
      description_text_snippet: snippet,
    });
  }
}

// ────────────────────────────────────────────
// 7. Sort and categorize mismatches
// ────────────────────────────────────────────
// Separate base stat mismatches (ATK/DEF) from bonus stat mismatches
const baseStatMismatches = [];
const bonusStatMismatches = [];

for (const item of mismatches) {
  const hasBase  = item.mismatched_fields.some(f => f.stat === 'base_atk' || f.stat === 'base_def');
  const hasBonus = item.mismatched_fields.some(f => f.stat.startsWith('bonus_'));

  if (hasBase) baseStatMismatches.push(item);
  if (hasBonus) bonusStatMismatches.push(item);
}

// Build per-stat summary
const statSummary = {};
for (const item of mismatches) {
  for (const field of item.mismatched_fields) {
    if (!statSummary[field.stat]) statSummary[field.stat] = 0;
    statSummary[field.stat]++;
  }
}

// ────────────────────────────────────────────
// 8. Write report
// ────────────────────────────────────────────
const report = {
  summary: {
    total_equipment: totalEquip,
    items_with_any_mismatch: totalWithDescMismatch,
    items_with_base_stat_mismatch: baseStatMismatches.length,
    items_with_bonus_stat_mismatch: bonusStatMismatches.length,
    items_missing_description: totalMissingDesc,
    mismatch_counts_by_stat: statSummary,
  },
  base_stat_mismatches: baseStatMismatches,
  bonus_stat_mismatches: bonusStatMismatches,
  missing_descriptions: missingDescriptions,
};

// Ensure output dir exists
const outDir = path.dirname(OUT_PATH);
if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

fs.writeFileSync(OUT_PATH, JSON.stringify(report, null, 2));
console.log(`\nReport written to: ${OUT_PATH}`);

// ────────────────────────────────────────────
// 9. Print summary
// ────────────────────────────────────────────
console.log('\n=== AUDIT SUMMARY ===');
console.log(`Total equipment items (YAML):    ${totalEquip}`);
console.log(`Items with ANY mismatch:         ${totalWithDescMismatch}`);
console.log(`  - Base stat (ATK/DEF) mismatch:  ${baseStatMismatches.length}`);
console.log(`  - Bonus stat mismatch:           ${bonusStatMismatches.length}`);
console.log(`Items missing description:        ${totalMissingDesc}`);
console.log(`\nMismatch counts by stat type:`);
for (const [stat, count] of Object.entries(statSummary).sort((a, b) => b[1] - a[1])) {
  console.log(`  ${stat}: ${count}`);
}

// Print first 10 base stat mismatches as examples
if (baseStatMismatches.length > 0) {
  console.log(`\n=== FIRST 20 BASE STAT MISMATCHES ===`);
  for (const item of baseStatMismatches.slice(0, 20)) {
    const baseFields = item.mismatched_fields.filter(f => f.stat === 'base_atk' || f.stat === 'base_def');
    for (const f of baseFields) {
      console.log(`  [${item.item_id}] ${item.name} (${item.item_type}/${item.equip_slot}): ${f.stat} yaml=${f.yaml_value} desc=${f.description_value}`);
    }
  }
  if (baseStatMismatches.length > 20) {
    console.log(`  ... and ${baseStatMismatches.length - 20} more`);
  }
}
