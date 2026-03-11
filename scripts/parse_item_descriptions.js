/**
 * Parse itemInfo_true_V5.lua from ROenglishRE and extract item descriptions.
 *
 * Output: docsNew/items/item_descriptions.json
 * Format: { items: { "501": { name, description, fullDescription[] }, ... }, metadata: { ... } }
 *
 * - `description` = first line(s) before the first "________________________" separator (flavor text)
 * - `fullDescription` = all lines with color codes stripped
 */

const fs = require('fs');
const path = require('path');

const INPUT = path.join(__dirname, '..', 'docsNew', 'items', 'itemInfo_source.lua');
const OUTPUT = path.join(__dirname, '..', 'docsNew', 'items', 'item_descriptions.json');

function stripMarkup(str) {
    return str
        .replace(/\^[0-9a-fA-F]{6}/g, '')            // ^XXXXXX color codes
        .replace(/<NAVI>.*?<\/NAVI>/g, '')             // <NAVI>...</NAVI> tags
        .replace(/<INFO>.*?<\/INFO>/g, '')             // <INFO>...</INFO> tags
        .trim();
}

function parseLua(content) {
    const items = {};
    let count = 0;
    let noDescCount = 0;

    // Match each item block: [ID] = { ... }
    // We'll use a state machine approach since regex can't handle nested braces well
    const lines = content.split('\n');
    let currentId = null;
    let inIdentifiedDesc = false;
    let inUnidentifiedDesc = false;
    let descLines = [];
    let identifiedName = null;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];

        // Match item ID: [1478] = {
        const idMatch = line.match(/^\s*\[(\d+)\]\s*=\s*\{/);
        if (idMatch) {
            // Save previous item if any
            if (currentId !== null && identifiedName !== null) {
                saveItem(items, currentId, identifiedName, descLines);
                count++;
            }
            currentId = idMatch[1];
            identifiedName = null;
            descLines = [];
            inIdentifiedDesc = false;
            inUnidentifiedDesc = false;
            continue;
        }

        // Match identifiedDisplayName
        const nameMatch = line.match(/identifiedDisplayName\s*=\s*"([^"]*)"/);
        if (nameMatch && currentId) {
            identifiedName = nameMatch[1];
            continue;
        }

        // Skip unidentifiedDescriptionName blocks entirely
        if (line.match(/unidentifiedDescriptionName\s*=\s*\{/) && currentId) {
            // Single-line format: { "text" },
            if (line.match(/\{[^}]*\}/)) continue;
            inUnidentifiedDesc = true;
            continue;
        }
        if (inUnidentifiedDesc) {
            if (line.match(/^\s*\}/)) inUnidentifiedDesc = false;
            continue;
        }

        // Match start of identifiedDescriptionName = {
        if (line.match(/identifiedDescriptionName\s*=\s*\{/) && currentId) {
            inIdentifiedDesc = true;
            // Check for single-line format: identifiedDescriptionName = { "text" },
            const singleLine = line.match(/identifiedDescriptionName\s*=\s*\{\s*"([^"]*)"\s*\}/);
            if (singleLine) {
                descLines.push(singleLine[1]);
                inIdentifiedDesc = false;
            }
            continue;
        }

        // Collect description lines
        if (inIdentifiedDesc && currentId) {
            // Check for closing brace
            if (line.match(/^\s*\}/)) {
                inIdentifiedDesc = false;
                continue;
            }
            // Extract quoted string
            const strMatch = line.match(/^\s*"([^"]*)"/);
            if (strMatch) {
                descLines.push(strMatch[1]);
            }
        }
    }

    // Save last item
    if (currentId !== null && identifiedName !== null) {
        saveItem(items, currentId, identifiedName, descLines);
        count++;
    }

    function saveItem(items, id, name, allDescLines) {
        // Split at first separator to get flavor text vs stat info
        const flavorLines = [];
        const fullDesc = [];

        let hitSeparator = false;
        for (const raw of allDescLines) {
            const cleaned = stripMarkup(raw);
            fullDesc.push(cleaned);
            if (!hitSeparator) {
                if (raw.includes('________________________') || cleaned === '________________________') {
                    hitSeparator = true;
                } else if (cleaned && cleaned !== '' && cleaned !== '_') {
                    flavorLines.push(cleaned);
                }
            }
        }

        const description = flavorLines.join(' ').trim();
        if (!description) noDescCount++;

        items[id] = {
            name: name,
            description: description || '',
            fullDescription: fullDesc
        };
    }

    return { items, count, noDescCount };
}

// Main
console.log('Reading itemInfo lua file...');
const content = fs.readFileSync(INPUT, 'utf8');
console.log(`File size: ${(content.length / 1024 / 1024).toFixed(1)}MB, ${content.split('\n').length} lines`);

console.log('Parsing...');
const { items, count, noDescCount } = parseLua(content);

console.log(`Parsed ${count} items`);
console.log(`  With descriptions: ${count - noDescCount}`);
console.log(`  Without descriptions: ${noDescCount}`);

// Verify Ahlspiess
if (items['1478']) {
    console.log('\nVerification - Ahlspiess (1478):');
    console.log(`  Name: ${items['1478'].name}`);
    console.log(`  Description: ${items['1478'].description}`);
}

// Sample a few more
for (const id of ['501', '4001', '7001', '1101', '2101']) {
    if (items[id]) {
        console.log(`  [${id}] ${items[id].name}: "${items[id].description.substring(0, 80)}..."`);
    }
}

const output = {
    metadata: {
        source: 'ROenglishRE/itemInfo_true_V5.lua (zackdreaver/ROenglishRE)',
        generatedAt: new Date().toISOString(),
        totalItems: count,
        withDescription: count - noDescCount,
        withoutDescription: noDescCount
    },
    items: items
};

fs.writeFileSync(OUTPUT, JSON.stringify(output, null, 2), 'utf8');
const fileSize = fs.statSync(OUTPUT).size;
console.log(`\nWritten to ${OUTPUT} (${(fileSize / 1024 / 1024).toFixed(1)}MB)`);
