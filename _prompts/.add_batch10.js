// Add spriteClass/weaponMode to the 11 enemies in batch 10
const fs = require('fs');
const path = 'C:/Sabri_MMO/server/src/ro_monster_templates.js';

const ADDITIONS = {
  magmaring: { spriteClass: 'magmaring', weaponMode: 0 },
  argiope: { spriteClass: 'argiope', weaponMode: 0 },
  kapha: { spriteClass: 'kapha', weaponMode: 0 },
  hunter_fly: { spriteClass: 'hunter_fly', weaponMode: 0 },
  alligator: { spriteClass: 'alligator', weaponMode: 0 },
  stone_shooter: { spriteClass: 'stone_shooter', weaponMode: 0 },
  novus: { spriteClass: 'novus', weaponMode: 0 },
  side_winder: { spriteClass: 'side_winder', weaponMode: 0 },
  punk: { spriteClass: 'punk', weaponMode: 0 },
  choco: { spriteClass: 'choco', weaponMode: 0 },
  sageworm: { spriteClass: 'sageworm', weaponMode: 0 },
};

let src = fs.readFileSync(path, 'utf8');

let updated = 0;
for (const [key, { spriteClass, weaponMode }] of Object.entries(ADDITIONS)) {
  // Find the template block for this key
  const headerPattern = new RegExp(`RO_MONSTER_TEMPLATES\\['${key}'\\]\\s*=\\s*\\{`);
  const headerMatch = src.match(headerPattern);
  if (!headerMatch) {
    console.error(`[SKIP] ${key}: header not found`);
    continue;
  }
  const headerIdx = headerMatch.index;

  // Find end of block (next blank line or closing brace-semicolon)
  const endIdx = src.indexOf('};', headerIdx);
  if (endIdx === -1) {
    console.error(`[SKIP] ${key}: end of block not found`);
    continue;
  }
  const block = src.slice(headerIdx, endIdx);

  // Skip if already has spriteClass
  if (block.includes('spriteClass')) {
    console.log(`[SKIP] ${key}: already has spriteClass`);
    continue;
  }

  // Find the "modes:" line within block and insert after it
  const modesMatch = block.match(/(\s+)modes: \{[^}]*\},\n/);
  if (!modesMatch) {
    console.error(`[SKIP] ${key}: modes line not found`);
    continue;
  }
  const insertPoint = headerIdx + modesMatch.index + modesMatch[0].length;
  const indent = modesMatch[1];
  const newLine = `${indent}spriteClass: '${spriteClass}', weaponMode: ${weaponMode},\n`;
  src = src.slice(0, insertPoint) + newLine + src.slice(insertPoint);
  updated++;
  console.log(`[OK] ${key}: added spriteClass`);
}

fs.writeFileSync(path, src);
console.log(`\nUpdated ${updated} templates.`);
