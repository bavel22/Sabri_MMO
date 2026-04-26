// Add spriteClass/weaponMode to Batch 12
const fs = require('fs');
const path = 'C:/Sabri_MMO/server/src/ro_monster_templates.js';

const ADDITIONS = {
  petit_:       { spriteClass: 'petit_',      weaponMode: 0 },
  brilight:     { spriteClass: 'brilight',    weaponMode: 0 },
  explosion:    { spriteClass: 'explosion',   weaponMode: 0 },
  poison_toad:  { spriteClass: 'poison_toad', weaponMode: 0 },
  drosera:      { spriteClass: 'drosera',     weaponMode: 0 },
  isis:         { spriteClass: 'isis',        weaponMode: 0 },
  deviace:      { spriteClass: 'deviace',     weaponMode: 0 },
  // wild_ginseng: already has spriteClass: 'hermit_plant' (inherits)
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
