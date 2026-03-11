#!/usr/bin/env node
/**
 * Resolve name mismatches between ro_monster_templates.js drop names
 * and rAthena canonical item names.
 */

const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

// Parse rAthena YAML
const ITEM_FILES = [
  '../docsNew/items/item_db_usable.yml',
  '../docsNew/items/item_db_equip.yml',
  '../docsNew/items/item_db_etc.yml',
];

const allItems = new Map();
const nameIndex = new Map();     // lowercase name -> item
const aegisIndex = new Map();    // aegis variants -> item
const wordIndex = new Map();     // individual words -> items

for (const file of ITEM_FILES) {
  const filePath = path.resolve(__dirname, file);
  const content = fs.readFileSync(filePath, 'utf8');
  const parsed = yaml.load(content);
  if (parsed && parsed.Body) {
    for (const item of parsed.Body) {
      allItems.set(item.Id, item);
      nameIndex.set(item.Name.toLowerCase(), item);
      // Build aegis name variants
      const aegis = item.AegisName.replace(/_/g, ' ').toLowerCase();
      aegisIndex.set(aegis, item);
    }
  }
}

// Parse monster drops
const monsterContent = fs.readFileSync(path.resolve(__dirname, '../server/src/ro_monster_templates.js'), 'utf8');
const itemNameRegex = /itemName:\s*['"](.*?)['"]/g;
const dropNames = new Set();
let match;
while ((match = itemNameRegex.exec(monsterContent)) !== null) {
  dropNames.add(match[1]);
}

// Try to resolve each unmatched name
const rathenaNameSet = new Set();
for (const [id, item] of allItems) {
  rathenaNameSet.set ? rathenaNameSet.add(item.Name) : null;
}

// Known name mappings (monster_templates name -> rAthena name)
const KNOWN_MAPPINGS = {
  'Elunium Stone': 'Rough Elunium',
  'Oridecon Stone': 'Rough Oridecon',
  'Crystal Jewel': 'Crystal_Jewel_',  // might be different
  'Old Violet Box': 'Old_Violet_Box',
  'Yggdrasilberry': 'Yggdrasilberry',
  'Seed Of Yggdrasil': 'Seed_Of_Yggdrasil',
  'Sparkling Dust': 'Sparkling_Dust',
  'Worm Peelings': 'Worm_Peelings',
  'Yellow Live': 'Yellow_Live',
  'Fruit Of Mastela': 'Fruit_Of_Mastela',
};

const resolved = [];
const unresolved = [];

for (const name of dropNames) {
  // Direct match
  if (nameIndex.has(name.toLowerCase())) {
    continue; // Already matched
  }

  // Try removing apostrophes
  const noApostrophe = name.replace(/'/g, '');
  if (nameIndex.has(noApostrophe.toLowerCase())) {
    resolved.push({ dropName: name, rathenaName: nameIndex.get(noApostrophe.toLowerCase()).Name, rathenaId: nameIndex.get(noApostrophe.toLowerCase()).Id, method: 'apostrophe' });
    continue;
  }

  // Try replacing ' with backslash (escape issues)
  const cleaned = name.replace(/\\/g, "'");
  if (nameIndex.has(cleaned.toLowerCase())) {
    resolved.push({ dropName: name, rathenaName: nameIndex.get(cleaned.toLowerCase()).Name, rathenaId: nameIndex.get(cleaned.toLowerCase()).Id, method: 'backslash' });
    continue;
  }

  // Try aegis name match
  const asAegis = name.replace(/ /g, '_').toLowerCase();
  if (aegisIndex.has(name.toLowerCase())) {
    resolved.push({ dropName: name, rathenaName: aegisIndex.get(name.toLowerCase()).Name, rathenaId: aegisIndex.get(name.toLowerCase()).Id, method: 'aegis' });
    continue;
  }

  // Try with "Of" -> "of"
  const lowerOf = name.replace(/ Of /g, ' of ');
  if (nameIndex.has(lowerOf.toLowerCase())) {
    resolved.push({ dropName: name, rathenaName: nameIndex.get(lowerOf.toLowerCase()).Name, rathenaId: nameIndex.get(lowerOf.toLowerCase()).Id, method: 'casing' });
    continue;
  }

  // Try " Card" suffix match (card names)
  if (name.endsWith(' Card')) {
    const cardName = name.toLowerCase();
    if (nameIndex.has(cardName)) {
      resolved.push({ dropName: name, rathenaName: nameIndex.get(cardName).Name, rathenaId: nameIndex.get(cardName).Id, method: 'card' });
      continue;
    }
  }

  // Known mappings
  if (KNOWN_MAPPINGS[name]) {
    const mappedName = KNOWN_MAPPINGS[name];
    const mapped = nameIndex.get(mappedName.toLowerCase());
    if (mapped) {
      resolved.push({ dropName: name, rathenaName: mapped.Name, rathenaId: mapped.Id, method: 'known' });
      continue;
    }
  }

  // Try partial matching - find closest
  let bestMatch = null;
  let bestScore = 0;
  const dropLower = name.toLowerCase().replace(/[^a-z0-9 ]/g, '');

  for (const [rName, item] of nameIndex) {
    const rLower = rName.replace(/[^a-z0-9 ]/g, '');
    if (rLower === dropLower) {
      bestMatch = item;
      bestScore = 100;
      break;
    }
    // Check if contains
    if (dropLower.includes(rLower) || rLower.includes(dropLower)) {
      const score = Math.min(dropLower.length, rLower.length) / Math.max(dropLower.length, rLower.length) * 100;
      if (score > bestScore && score > 70) {
        bestScore = score;
        bestMatch = item;
      }
    }
  }

  if (bestMatch && bestScore >= 85) {
    resolved.push({ dropName: name, rathenaName: bestMatch.Name, rathenaId: bestMatch.Id, method: 'fuzzy(' + Math.round(bestScore) + '%)' });
  } else {
    unresolved.push(name);
  }
}

console.log('\n=== NAME RESOLUTION RESULTS ===');
console.log('Additional matches resolved: ' + resolved.length);
console.log('Still unresolved: ' + unresolved.length);

console.log('\n--- Resolved Names ---');
for (const r of resolved) {
  console.log('  "' + r.dropName + '" -> "' + r.rathenaName + '" (ID:' + r.rathenaId + ') [' + r.method + ']');
}

console.log('\n--- Still Unresolved (' + unresolved.length + ') ---');
for (const name of unresolved.sort()) {
  console.log('  "' + name + '"');
}

// Write resolution map
const mapPath = path.resolve(__dirname, '../docsNew/items/name_resolution_map.json');
const mapData = {
  resolved: resolved,
  unresolved: unresolved,
  totalDropNames: dropNames.size,
  directMatches: dropNames.size - resolved.length - unresolved.length,
  additionalResolved: resolved.length,
  stillUnresolved: unresolved.length,
};
fs.writeFileSync(mapPath, JSON.stringify(mapData, null, 2));
console.log('\nName resolution map written to: ' + mapPath);
