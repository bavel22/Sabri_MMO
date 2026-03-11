const fs = require('fs');
const content = fs.readFileSync('server/src/ro_monster_templates.js', 'utf8');
// Find all itemName values - handle escaped quotes
const regex = /itemName:\s*'((?:[^'\\]|\\.)*)'/g;
const names = new Set();
let m;
while ((m = regex.exec(content)) !== null) {
  const name = m[1].replace(/\\'/g, "'");
  names.add(name);
}
const withApostrophe = [...names].filter(n => n.includes("'"));
console.log('Total unique items (proper parse): ' + names.size);
console.log('Items with apostrophes: ' + withApostrophe.length);
withApostrophe.sort().forEach(n => console.log('  ' + n));
