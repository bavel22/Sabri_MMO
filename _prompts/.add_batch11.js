// Add spriteClass/weaponMode/spriteTint to Batch 11
const fs = require('fs');
const path = 'C:/Sabri_MMO/server/src/ro_monster_templates.js';

const ADDITIONS = {
  plasma_r:  { spriteClass: 'plasma', weaponMode: 0, spriteTint: [1.5, 0.6, 0.6] },  // red plasma
  plasma_b:  { spriteClass: 'plasma', weaponMode: 0, spriteTint: [0.6, 0.6, 1.5] },  // blue plasma
  novus_:    { spriteClass: 'novus',  weaponMode: 0 },                                 // variant inheritance
  petit:     { spriteClass: 'petit',  weaponMode: 0 },
  galion:    { spriteClass: 'galion', weaponMode: 0 },
  // blazzer: SKIPPED — no GLB available
  // dragon_egg: already has spriteClass from earlier
};

let src = fs.readFileSync(path, 'utf8');

let updated = 0;
for (const [key, cfg] of Object.entries(ADDITIONS)) {
  const headerPattern = new RegExp(`RO_MONSTER_TEMPLATES\\['${key}'\\]\\s*=\\s*\\{`);
  const headerMatch = src.match(headerPattern);
  if (!headerMatch) {
    console.error(`[SKIP] ${key}: header not found`);
    continue;
  }
  const headerIdx = headerMatch.index;
  const endIdx = src.indexOf('};', headerIdx);
  const block = src.slice(headerIdx, endIdx);
  if (block.includes('spriteClass')) {
    console.log(`[SKIP] ${key}: already has spriteClass`);
    continue;
  }
  const modesMatch = block.match(/(\s+)modes: \{[^}]*\},\n/);
  if (!modesMatch) {
    console.error(`[SKIP] ${key}: modes line not found`);
    continue;
  }
  const insertPoint = headerIdx + modesMatch.index + modesMatch[0].length;
  const indent = modesMatch[1];
  let line = `${indent}spriteClass: '${cfg.spriteClass}', weaponMode: ${cfg.weaponMode}`;
  if (cfg.spriteTint) {
    line += `, spriteTint: [${cfg.spriteTint.join(', ')}]`;
  }
  line += `,\n`;
  src = src.slice(0, insertPoint) + line + src.slice(insertPoint);
  updated++;
  console.log(`[OK] ${key}: added`);
}

fs.writeFileSync(path, src);
console.log(`\nUpdated ${updated} templates.`);
