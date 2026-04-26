// Add spriteClass for blazzer
const fs = require('fs');
const path = 'C:/Sabri_MMO/server/src/ro_monster_templates.js';

let src = fs.readFileSync(path, 'utf8');

const key = 'blazzer';
const cfg = { spriteClass: 'blazzer', weaponMode: 0 };

const headerPattern = new RegExp(`RO_MONSTER_TEMPLATES\\['${key}'\\]\\s*=\\s*\\{`);
const headerMatch = src.match(headerPattern);
if (!headerMatch) {
  console.error(`[SKIP] ${key}: header not found`);
  process.exit(1);
}
const headerIdx = headerMatch.index;
const endIdx = src.indexOf('};', headerIdx);
const block = src.slice(headerIdx, endIdx);
if (block.includes('spriteClass')) {
  console.log(`[SKIP] ${key}: already has spriteClass`);
  process.exit(0);
}
const modesMatch = block.match(/(\s+)modes: \{[^}]*\},\n/);
if (!modesMatch) {
  console.error(`[SKIP] ${key}: modes line not found`);
  process.exit(1);
}
const insertPoint = headerIdx + modesMatch.index + modesMatch[0].length;
const indent = modesMatch[1];
const line = `${indent}spriteClass: '${cfg.spriteClass}', weaponMode: ${cfg.weaponMode},\n`;
src = src.slice(0, insertPoint) + line + src.slice(insertPoint);

fs.writeFileSync(path, src);
console.log(`[OK] ${key}: added`);
