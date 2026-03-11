#!/usr/bin/env node
/**
 * Parse rAthena pre-renewal item database YAML files
 * Cross-reference against init.sql items and monster drop tables
 * Output comprehensive gap analysis
 */

const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

// ========== PARSE RATHENA YAML FILES ==========
const ITEM_FILES = [
  '../docsNew/items/item_db_usable.yml',
  '../docsNew/items/item_db_equip.yml',
  '../docsNew/items/item_db_etc.yml',
];

const allItems = new Map();

for (const file of ITEM_FILES) {
  const filePath = path.resolve(__dirname, file);
  const content = fs.readFileSync(filePath, 'utf8');
  const parsed = yaml.load(content);

  if (parsed && parsed.Body) {
    for (const item of parsed.Body) {
      allItems.set(item.Id, item);
    }
  }
}

console.log('\n=== RATHENA PRE-RENEWAL ITEM DATABASE ===');
console.log('Total items parsed: ' + allItems.size);

// ========== CATEGORIZE BY TYPE ==========
const byType = {};
const weaponsBySubType = {};

for (const [id, item] of allItems) {
  const type = item.Type || 'Etc';
  if (!byType[type]) byType[type] = [];
  byType[type].push(item);

  if (type === 'Weapon' && item.SubType) {
    if (!weaponsBySubType[item.SubType]) weaponsBySubType[item.SubType] = [];
    weaponsBySubType[item.SubType].push(item);
  }
}

console.log('\n--- Items by Type ---');
for (const [type, items] of Object.entries(byType).sort((a, b) => b[1].length - a[1].length)) {
  const idRange = items.map(i => i.Id).sort((a, b) => a - b);
  console.log('  ' + type + ': ' + items.length + ' items (ID ' + idRange[0] + ' - ' + idRange[idRange.length - 1] + ')');
}

console.log('\n--- Weapons by SubType ---');
for (const [subType, items] of Object.entries(weaponsBySubType).sort((a, b) => b[1].length - a[1].length)) {
  console.log('  ' + subType + ': ' + items.length);
}

// ========== CROSS-REFERENCE WITH MONSTER DROPS ==========
const monsterTemplatesPath = path.resolve(__dirname, '../server/src/ro_monster_templates.js');
const monsterContent = fs.readFileSync(monsterTemplatesPath, 'utf8');

const itemNameRegex = /itemName:\s*['"](.*?)['"]/g;
const dropItemNames = new Set();
const dropFrequency = {};
let match;
while ((match = itemNameRegex.exec(monsterContent)) !== null) {
  dropItemNames.add(match[1]);
  dropFrequency[match[1]] = (dropFrequency[match[1]] || 0) + 1;
}
console.log('\n=== MONSTER DROP CROSS-REFERENCE ===');
console.log('Unique item names in monster drops: ' + dropItemNames.size);

// Build rAthena name -> item lookup
const rathenaNameToItem = new Map();
for (const [id, item] of allItems) {
  rathenaNameToItem.set(item.Name, item);
}

let foundCount = 0;
let notFoundCount = 0;
const notFoundNames = [];
for (const name of dropItemNames) {
  if (rathenaNameToItem.has(name)) {
    foundCount++;
  } else {
    notFoundCount++;
    notFoundNames.push(name);
  }
}
console.log('Drop items found in rAthena DB: ' + foundCount);
console.log('Drop items NOT found in rAthena DB: ' + notFoundCount);

if (notFoundNames.length > 0 && notFoundNames.length <= 50) {
  console.log('\n--- Unmatched drop item names ---');
  for (const name of notFoundNames) {
    console.log('  "' + name + '"');
  }
}

// ========== LOAD EXISTING DB ITEMS ==========
const initSqlPath = path.resolve(__dirname, '../database/init.sql');
const initSqlContent = fs.readFileSync(initSqlPath, 'utf8');

const existingItems = new Map();
const itemIdNameRegex = /VALUES\s*\(\s*(\d+)\s*,\s*'([^']+)'/g;
let sqlMatch;
while ((sqlMatch = itemIdNameRegex.exec(initSqlContent)) !== null) {
  existingItems.set(parseInt(sqlMatch[1]), sqlMatch[2]);
}
console.log('\n=== EXISTING DATABASE (init.sql) ===');
console.log('Total items in init.sql: ' + existingItems.size);

const existingNameToId = new Map();
for (const [id, name] of existingItems) {
  existingNameToId.set(name, id);
}

// ========== LOAD ITEM MAPPING ==========
let itemMapping = {};
try {
  const mappingPath = path.resolve(__dirname, '../server/src/ro_item_mapping.js');
  const mappingContent = fs.readFileSync(mappingPath, 'utf8');
  const mappingMatch = mappingContent.match(/module\.exports\s*=\s*(\{[\s\S]*?\n\})/);
  if (mappingMatch) {
    itemMapping = eval('(' + mappingMatch[1] + ')');
  }
} catch (e) {
  console.log('Could not load ro_item_mapping.js');
}
console.log('Items in ro_item_mapping.js: ' + Object.keys(itemMapping).length);

// ========== GAP ANALYSIS ==========
console.log('\n========================================');
console.log('=== COMPREHENSIVE GAP ANALYSIS ===');
console.log('========================================');

const dropsNotInDb = [];
const dropsInDb = [];
for (const name of dropItemNames) {
  const mappedId = itemMapping[name];
  const isInDb = mappedId && existingItems.has(mappedId);
  const rathenaItem = rathenaNameToItem.get(name);

  if (isInDb) {
    dropsInDb.push({ name, dbId: mappedId, rathenaId: rathenaItem ? rathenaItem.Id : null });
  } else {
    dropsNotInDb.push({
      name,
      rathenaId: rathenaItem ? rathenaItem.Id : null,
      rathenaType: rathenaItem ? (rathenaItem.Type || 'Etc') : 'Unknown',
      rathenaSubType: rathenaItem ? (rathenaItem.SubType || '') : '',
      buy: rathenaItem ? (rathenaItem.Buy || 0) : 0,
      weight: rathenaItem ? (rathenaItem.Weight || 0) : 0,
      attack: rathenaItem ? (rathenaItem.Attack || 0) : 0,
      defense: rathenaItem ? (rathenaItem.Defense || 0) : 0,
      slots: rathenaItem ? (rathenaItem.Slots || 0) : 0,
      frequency: dropFrequency[name] || 0,
      script: rathenaItem ? (rathenaItem.Script || '') : ''
    });
  }
}

console.log('\nDrop items IN database: ' + dropsInDb.length + ' / ' + dropItemNames.size);
console.log('Drop items MISSING from database: ' + dropsNotInDb.length + ' / ' + dropItemNames.size);

const missingByType = {};
for (const item of dropsNotInDb) {
  const type = item.rathenaType;
  if (!missingByType[type]) missingByType[type] = [];
  missingByType[type].push(item);
}

console.log('\n--- Missing drop items by rAthena type ---');
for (const [type, items] of Object.entries(missingByType).sort((a, b) => b[1].length - a[1].length)) {
  console.log('  ' + type + ': ' + items.length + ' items');
}

console.log('\n--- TOP 50 MOST-REFERENCED MISSING ITEMS ---');
const sorted = dropsNotInDb.sort((a, b) => b.frequency - a.frequency).slice(0, 50);
for (const item of sorted) {
  console.log('  [' + item.frequency + 'x] ' + item.name + ' (ID:' + (item.rathenaId || 'N/A') + ' Type:' + item.rathenaType + ' ATK:' + item.attack + ' DEF:' + item.defense + ' Buy:' + item.buy + 'z W:' + (item.weight/10));
}

// ========== CHECK EXISTING DB ITEMS AGAINST RATHENA ==========
console.log('\n=== EXISTING DB ITEMS vs RATHENA ===');
let matchCount = 0;
let mismatchCount = 0;
const mismatches = [];
for (const [dbId, dbName] of existingItems) {
  const rathenaItem = rathenaNameToItem.get(dbName);
  if (rathenaItem) {
    if (rathenaItem.Id !== dbId) {
      mismatchCount++;
      mismatches.push({ dbName, dbId, rathenaId: rathenaItem.Id });
    } else {
      matchCount++;
    }
  }
}
console.log('DB items matching rAthena IDs: ' + matchCount);
console.log('DB items with DIFFERENT rAthena IDs: ' + mismatchCount);
if (mismatches.length > 0) {
  console.log('\n--- ID Mismatches (DB vs rAthena) ---');
  for (const m of mismatches.slice(0, 30)) {
    console.log('  "' + m.dbName + '": DB ID=' + m.dbId + ', rAthena ID=' + m.rathenaId);
  }
}

// Count DB items NOT in rAthena at all
let notInRathena = 0;
const customItems = [];
for (const [dbId, dbName] of existingItems) {
  if (!rathenaNameToItem.has(dbName)) {
    notInRathena++;
    customItems.push({ id: dbId, name: dbName });
  }
}
console.log('\nDB items NOT found in rAthena (custom): ' + notInRathena);
if (customItems.length > 0) {
  for (const item of customItems) {
    console.log('  ID:' + item.id + ' "' + item.name + '"');
  }
}

// ========== WRITE COMPLETE JSON OUTPUT ==========
const outputPath = path.resolve(__dirname, '../docsNew/items/rathena_complete_items.json');
const outputData = {
  metadata: {
    generatedAt: new Date().toISOString(),
    totalRathenaItems: allItems.size,
    itemsInDatabase: existingItems.size,
    itemsInMonsterDrops: dropItemNames.size,
    dropItemsInDb: dropsInDb.length,
    dropItemsMissing: dropsNotInDb.length,
    dbItemsMatchingRathena: matchCount,
    dbItemsMismatchedIds: mismatchCount,
    dbItemsCustom: notInRathena,
  },
  byType: {},
  missingDropItems: dropsNotInDb,
  idMismatches: mismatches,
  customItems: customItems,
};

for (const [type, items] of Object.entries(byType)) {
  outputData.byType[type] = items.length;
}

fs.writeFileSync(outputPath, JSON.stringify(outputData, null, 2));
console.log('\nJSON output written to: ' + outputPath);

// ========== WRITE MARKDOWN REPORT ==========
const reportPath = path.resolve(__dirname, '../docsNew/items/missing_items_report.md');
let report = '# RO Classic Item Gap Analysis Report\n\n';
report += 'Generated: ' + new Date().toISOString() + '\n\n';
report += '## Summary\n\n';
report += '| Metric | Count |\n|--------|-------|\n';
report += '| Total rAthena pre-renewal items | ' + allItems.size + ' |\n';
report += '| Items currently in database | ' + existingItems.size + ' |\n';
report += '| Items in monster drop tables | ' + dropItemNames.size + ' |\n';
report += '| Monster drop items IN DB | ' + dropsInDb.length + ' |\n';
report += '| Monster drop items MISSING from DB | ' + dropsNotInDb.length + ' |\n';
report += '| DB coverage vs rAthena | ' + ((existingItems.size / allItems.size) * 100).toFixed(1) + '% |\n';
report += '| Drop coverage | ' + ((dropsInDb.length / dropItemNames.size) * 100).toFixed(1) + '% |\n';
report += '| DB items with wrong IDs | ' + mismatchCount + ' |\n';
report += '| Custom items (not in rAthena) | ' + notInRathena + ' |\n\n';

report += '## rAthena Items by Type\n\n';
report += '| Type | Count | ID Range |\n|------|-------|----------|\n';
for (const [type, items] of Object.entries(byType).sort((a, b) => b[1].length - a[1].length)) {
  const ids = items.map(i => i.Id).sort((a, b) => a - b);
  report += '| ' + type + ' | ' + items.length + ' | ' + ids[0] + ' - ' + ids[ids.length - 1] + ' |\n';
}

report += '\n## Missing Drop Items by Category\n\n';
for (const [type, items] of Object.entries(missingByType).sort((a, b) => b[1].length - a[1].length)) {
  report += '### ' + type + ' (' + items.length + ' items)\n\n';
  report += '| rAthena ID | Name | SubType | ATK/DEF | Buy | Weight | Slots | Drops |\n';
  report += '|-----------|------|---------|---------|-----|--------|-------|-------|\n';
  for (const item of items.sort((a, b) => b.frequency - a.frequency)) {
    const atkDef = item.attack ? 'ATK:' + item.attack : (item.defense ? 'DEF:' + item.defense : '-');
    report += '| ' + (item.rathenaId || 'N/A') + ' | ' + item.name + ' | ' + (item.rathenaSubType || '-') + ' | ' + atkDef + ' | ' + item.buy + 'z | ' + (item.weight/10).toFixed(1) + ' | ' + item.slots + ' | ' + item.frequency + 'x |\n';
  }
  report += '\n';
}

if (mismatches.length > 0) {
  report += '## ID Mismatches (DB ID differs from rAthena canonical ID)\n\n';
  report += '| Item Name | DB ID | rAthena ID |\n|-----------|-------|------------|\n';
  for (const m of mismatches) {
    report += '| ' + m.dbName + ' | ' + m.dbId + ' | ' + m.rathenaId + ' |\n';
  }
  report += '\n';
}

if (customItems.length > 0) {
  report += '## Custom Items (in DB but not in rAthena)\n\n';
  report += '| DB ID | Name |\n|-------|------|\n';
  for (const item of customItems) {
    report += '| ' + item.id + ' | ' + item.name + ' |\n';
  }
  report += '\n';
}

fs.writeFileSync(reportPath, report);
console.log('Report written to: ' + reportPath);

console.log('\n=== ANALYSIS COMPLETE ===');
