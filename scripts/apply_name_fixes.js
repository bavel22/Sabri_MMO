/**
 * apply_name_fixes.js
 *
 * Reads name_fixes.json (wrong_name -> correct_name mappings) and applies them
 * to all itemName fields in ro_monster_templates.js.
 *
 * Only modifies exact itemName value matches — does not touch monster names,
 * comments, or other string fields.
 *
 * Usage: node scripts/apply_name_fixes.js
 */

'use strict';

const fs = require('fs');
const path = require('path');

const FIXES_PATH = path.join(__dirname, 'output', 'name_fixes.json');
const TEMPLATES_PATH = path.join(__dirname, '..', 'server', 'src', 'ro_monster_templates.js');

// Load the name fixes mapping
const nameFixes = JSON.parse(fs.readFileSync(FIXES_PATH, 'utf8'));
const fixCount = Object.keys(nameFixes).length;
console.log(`Loaded ${fixCount} name fixes from ${FIXES_PATH}`);

// Load the monster templates file
let content = fs.readFileSync(TEMPLATES_PATH, 'utf8');
const originalLength = content.length;
console.log(`Loaded ro_monster_templates.js (${originalLength} characters)`);

let totalReplacements = 0;
const replacementDetails = [];

// For each wrong_name -> correct_name pair, find and replace in itemName fields
for (const [wrongName, correctName] of Object.entries(nameFixes)) {
    // Escape special regex characters in the wrong name
    const escapedWrongName = wrongName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

    // In the file, single quotes within item names are escaped as \'
    // So we need to match the wrong name as it would appear inside single-quoted strings.
    // The JSON key is the "plain text" version; in the file it would have \' for apostrophes.
    // Convert the wrong name to how it appears in the file (inside single quotes).
    const wrongNameInFile = wrongName.replace(/'/g, "\\'");
    const correctNameInFile = correctName.replace(/'/g, "\\'");

    // Escape for regex after converting apostrophes
    const escapedWrongNameInFile = wrongNameInFile.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

    // Match: itemName: 'WRONG_NAME' (with single quotes)
    // The pattern captures the surrounding structure to ensure we only replace itemName values
    const singleQuotePattern = new RegExp(
        `(itemName:\\s*')${escapedWrongNameInFile}(')`,'g'
    );

    // Match: itemName: "WRONG_NAME" (with double quotes, just in case)
    const doubleQuotePattern = new RegExp(
        `(itemName:\\s*")${escapedWrongNameInFile}(")`,'g'
    );

    let count = 0;

    // Replace single-quoted occurrences
    content = content.replace(singleQuotePattern, (match, prefix, suffix) => {
        count++;
        return `${prefix}${correctNameInFile}${suffix}`;
    });

    // Replace double-quoted occurrences
    content = content.replace(doubleQuotePattern, (match, prefix, suffix) => {
        count++;
        return `${prefix}${correctNameInFile}${suffix}`;
    });

    if (count > 0) {
        totalReplacements += count;
        replacementDetails.push({ wrongName, correctName, count });
    }
}

// Write the modified file back
fs.writeFileSync(TEMPLATES_PATH, content, 'utf8');

// Report results
console.log(`\n=== Results ===`);
console.log(`Total replacements made: ${totalReplacements}`);
console.log(`Unique names fixed: ${replacementDetails.length} / ${fixCount}`);
console.log(`File size: ${originalLength} -> ${content.length} characters`);

if (replacementDetails.length > 0) {
    console.log(`\nDetailed replacements:`);
    for (const { wrongName, correctName, count } of replacementDetails) {
        console.log(`  "${wrongName}" -> "${correctName}" (${count} occurrence${count > 1 ? 's' : ''})`);
    }
}

// Report names that had no matches (not necessarily a problem - they might not be in drop tables)
const unmatched = Object.entries(nameFixes).filter(
    ([wrong]) => !replacementDetails.find(d => d.wrongName === wrong)
);
if (unmatched.length > 0) {
    console.log(`\n${unmatched.length} names from fixes had no matches in the template file (not in any drop table).`);
}
