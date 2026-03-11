/**
 * Rebuild all 9 item markdown files from rAthena YAML data + item_descriptions.json.
 *
 * Sources:
 *   - item_db_equip.yml  (equipment: weapons, armor, shields, headgear, footgear, garments, accessories)
 *   - item_db_usable.yml (consumables, scrolls, ammo, boxes)
 *   - item_db_etc.yml    (etc, crafting, monster drops)
 *   - item_descriptions.json (flavor text from ROenglishRE)
 *
 * Outputs:
 *   - accessories.md, armor.md, consumables.md, etc_crafting.md,
 *     footgear.md, garments.md, headgear.md, shields.md, weapons.md
 */

const fs = require('fs');
const path = require('path');

const ITEMS_DIR = path.join(__dirname, '..', 'docsNew', 'items');

// ── Load descriptions ──────────────────────────────────────────────────
const descData = JSON.parse(fs.readFileSync(path.join(ITEMS_DIR, 'item_descriptions.json'), 'utf8'));
const descById = descData.items; // { "501": { name, description, fullDescription[] } }

// Build name→description lookup (case-insensitive)
const descByName = {};
for (const [id, item] of Object.entries(descById)) {
    const key = item.name.toLowerCase().trim();
    // Prefer first entry (lower ID = more canonical)
    if (!descByName[key]) {
        descByName[key] = { id: parseInt(id), ...item };
    }
}

// ── Parse existing MD files to preserve "Implemented" items ────────────
function parseExistingMd(filename) {
    const filepath = path.join(ITEMS_DIR, filename);
    if (!fs.existsSync(filepath)) return { implemented: [], notImplemented: [] };
    const content = fs.readFileSync(filepath, 'utf8');
    const implemented = [];
    const notImplemented = [];

    let section = null;
    for (const line of content.split('\n')) {
        if (line.startsWith('## Implemented')) { section = 'impl'; continue; }
        if (line.startsWith('## Not Yet Implemented')) { section = 'notimpl'; continue; }
        if (line.startsWith('#')) { section = null; continue; }

        const match = line.match(/^(.+?)\s*->\s*(.+?)\s*->\s*(.+)$/);
        if (match) {
            const entry = { name: match[1].trim(), type: match[2].trim(), desc: match[3].trim() };
            if (section === 'impl') implemented.push(entry);
            else if (section === 'notimpl') notImplemented.push(entry);
        }
    }
    return { implemented, notImplemented };
}

// ── Simple YAML parser for rAthena item_db format ──────────────────────
function parseRathenaYaml(filename) {
    const filepath = path.join(ITEMS_DIR, filename);
    const content = fs.readFileSync(filepath, 'utf8');
    const items = [];
    let current = null;
    let inLocations = false;
    let inJobs = false;

    for (const line of content.split('\n')) {
        // New item entry
        if (line.match(/^\s{2}- Id:\s*(\d+)/)) {
            if (current) items.push(current);
            current = {
                id: parseInt(line.match(/Id:\s*(\d+)/)[1]),
                aegisName: '',
                name: '',
                type: '',
                subType: '',
                locations: [],
                buy: 0,
                weight: 0,
                attack: 0,
                defense: 0,
                slots: 0,
                weaponLevel: 0,
                range: 0,
            };
            inLocations = false;
            inJobs = false;
            continue;
        }
        if (!current) continue;

        // End of Locations/Jobs block
        if (inLocations && !line.match(/^\s{6,}/)) inLocations = false;
        if (inJobs && !line.match(/^\s{6,}/)) inJobs = false;

        if (line.match(/^\s+AegisName:\s*(.+)/)) current.aegisName = line.match(/AegisName:\s*(.+)/)[1].trim();
        else if (line.match(/^\s+Name:\s*(.+)/)) current.name = line.match(/Name:\s*(.+)/)[1].trim();
        else if (line.match(/^\s+Type:\s*(.+)/)) current.type = line.match(/Type:\s*(.+)/)[1].trim();
        else if (line.match(/^\s+SubType:\s*(.+)/)) current.subType = line.match(/SubType:\s*(.+)/)[1].trim();
        else if (line.match(/^\s+Buy:\s*(\d+)/)) current.buy = parseInt(line.match(/Buy:\s*(\d+)/)[1]);
        else if (line.match(/^\s+Weight:\s*(\d+)/)) current.weight = parseInt(line.match(/Weight:\s*(\d+)/)[1]);
        else if (line.match(/^\s+Attack:\s*(\d+)/)) current.attack = parseInt(line.match(/Attack:\s*(\d+)/)[1]);
        else if (line.match(/^\s+Defense:\s*(\d+)/)) current.defense = parseInt(line.match(/Defense:\s*(\d+)/)[1]);
        else if (line.match(/^\s+Slots:\s*(\d+)/)) current.slots = parseInt(line.match(/Slots:\s*(\d+)/)[1]);
        else if (line.match(/^\s+WeaponLevel:\s*(\d+)/)) current.weaponLevel = parseInt(line.match(/WeaponLevel:\s*(\d+)/)[1]);
        else if (line.match(/^\s+Range:\s*(\d+)/)) current.range = parseInt(line.match(/Range:\s*(\d+)/)[1]);
        else if (line.match(/^\s+Locations:/)) { inLocations = true; inJobs = false; }
        else if (line.match(/^\s+Jobs:/)) { inJobs = true; inLocations = false; }
        else if (inLocations) {
            const locMatch = line.match(/^\s+(\w+):\s*true/);
            if (locMatch) current.locations.push(locMatch[1]);
        }
    }
    if (current) items.push(current);
    return items;
}

// ── Categorize equipment items ─────────────────────────────────────────
function categorizeEquipItem(item) {
    const locs = item.locations;

    if (item.type === 'Weapon') return 'weapons';

    // Shields: Left_Hand
    if (locs.includes('Left_Hand')) return 'shields';

    // Armor (body): Armor location
    if (locs.includes('Armor')) return 'armor';

    // Footgear: Shoes
    if (locs.includes('Shoes')) return 'footgear';

    // Garment
    if (locs.includes('Garment')) return 'garments';

    // Headgear: Head_Top, Head_Mid, Head_Low
    if (locs.includes('Head_Top') || locs.includes('Head_Mid') || locs.includes('Head_Low')) return 'headgear';

    // Accessories
    if (locs.includes('Both_Accessory') || locs.includes('Right_Accessory') || locs.includes('Left_Accessory')) return 'accessories';

    // Shadow gear, costume — skip or put in accessories
    if (locs.some(l => l.startsWith('Shadow_') || l.startsWith('Costume_'))) return null;

    // Both_Hand weapons (2h)
    if (locs.includes('Both_Hand') || locs.includes('Right_Hand')) return 'weapons';

    return null;
}

// ── Get weapon subtype display name ────────────────────────────────────
function getWeaponSubType(subType) {
    const map = {
        'Fist': 'Knuckle',
        'Dagger': 'Dagger',
        '1hSword': 'One-Handed Sword',
        '2hSword': 'Two-Handed Sword',
        '1hSpear': 'One-Handed Spear',
        '2hSpear': 'Two-Handed Spear',
        '1hAxe': 'One-Handed Axe',
        '2hAxe': 'Two-Handed Axe',
        'Mace': 'Mace',
        'Staff': 'Staff',
        'Bow': 'Bow',
        'Knuckle': 'Knuckle',
        'Musical': 'Musical Instrument',
        'Whip': 'Whip',
        'Book': 'Book',
        'Katar': 'Katar',
        'Revolver': 'Revolver',
        'Rifle': 'Rifle',
        'Gatling': 'Gatling Gun',
        'Shotgun': 'Shotgun',
        'Grenade': 'Grenade Launcher',
        'Huuma': 'Huuma Shuriken',
        '2hStaff': 'Two-Handed Staff',
    };
    return map[subType] || subType || 'Weapon';
}

// ── Get headgear subtype from locations ────────────────────────────────
function getHeadgearSubType(locs) {
    const hasTop = locs.includes('Head_Top');
    const hasMid = locs.includes('Head_Mid');
    const hasLow = locs.includes('Head_Low');

    if (hasTop && hasMid && hasLow) return 'Upper/Mid/Lower Headgear';
    if (hasTop && hasMid) return 'Upper/Mid Headgear';
    if (hasMid && hasLow) return 'Mid/Lower Headgear';
    if (hasTop) return 'Upper Headgear';
    if (hasMid) return 'Mid Headgear';
    if (hasLow) return 'Lower Headgear';
    return 'Headgear';
}

// ── Get ammo subtype ───────────────────────────────────────────────────
function getAmmoSubType(subType) {
    const map = {
        'Arrow': 'Arrow',
        'Dagger': 'Throwing Dagger',
        'Bullet': 'Bullet',
        'Shell': 'Shell',
        'Shuriken': 'Shuriken',
        'Kunai': 'Kunai',
        'Cannonball': 'Cannonball',
        'Throwweapon': 'Throwing Weapon',
    };
    return map[subType] || 'Ammunition';
}

// ── Get usable item type display name ──────────────────────────────────
function getUsableType(item) {
    const name = item.name.toLowerCase();
    if (name.includes('arrow') || name.includes('quiver') || item.type === 'Ammo') {
        return 'Ammunition';
    }
    if (name.includes('scroll') || name.includes('magic scroll')) return 'Scroll';
    if (name.includes('box') || name.includes('album')) return 'Box / Container';
    if (name.includes('potion') || name.includes('herb') || name.includes('heal')) return 'Healing Item';

    // General usable type guessing
    if (item.type === 'Healing' || item.type === 'Delayconsume') return 'Healing Item';
    if (item.type === 'Usable') return 'Usable Item';
    if (item.type === 'Cash') return 'Cash Item';
    return 'Usable Item';
}

// ── Get etc item type display name ─────────────────────────────────────
function getEtcType(item) {
    const name = item.name.toLowerCase();
    if (name.includes('gemstone') || name.includes('jewel')) return 'Gemstone';
    if (name.includes('ore') || name.includes('oridecon') || name.includes('elunium') ||
        name.includes('phracon') || name.includes('emveretarcon')) return 'Ore / Crafting Mineral';
    if (name.includes('dyestuff')) return 'Upgrade Material';
    if (name.includes('doll')) return 'Collectible Doll';
    if (name.includes('cookbook') || name.includes('cooking') || name.includes('dish')) return 'Cooking Material';
    if (name.includes('heart') && (name.includes('burning') || name.includes('frozen') || name.includes('flame'))) return 'Elemental Material';
    return 'Monster Drop / Loot';
}

// ── Lookup description for an item name ────────────────────────────────
function lookupDesc(name, id) {
    // Try by ID first
    if (id && descById[String(id)]) {
        const d = descById[String(id)].description;
        if (d) return d;
    }
    // Try by name
    const key = name.toLowerCase().trim();
    if (descByName[key] && descByName[key].description) {
        return descByName[key].description;
    }
    return '';
}

// ── Main ───────────────────────────────────────────────────────────────
console.log('Parsing YAML databases...');
const equipItems = parseRathenaYaml('item_db_equip.yml');
const usableItems = parseRathenaYaml('item_db_usable.yml');
const etcItems = parseRathenaYaml('item_db_etc.yml');

console.log(`  Equipment: ${equipItems.length}, Usable: ${usableItems.length}, Etc: ${etcItems.length}`);

// Categorize all items into buckets
const buckets = {
    accessories: new Map(),   // name -> { type, desc, id }
    armor: new Map(),
    consumables: new Map(),
    etc_crafting: new Map(),
    footgear: new Map(),
    garments: new Map(),
    headgear: new Map(),
    shields: new Map(),
    weapons: new Map(),
};

// Process equipment items
for (const item of equipItems) {
    if (!item.name) continue;
    const category = categorizeEquipItem(item);
    if (!category) continue;

    const bucket = buckets[category];
    // Skip duplicates (slotted variants have same Name)
    if (bucket.has(item.name)) continue;

    let specificType;
    if (category === 'weapons') specificType = getWeaponSubType(item.subType);
    else if (category === 'headgear') specificType = getHeadgearSubType(item.locations);
    else if (category === 'armor') specificType = 'Body Armor';
    else if (category === 'shields') specificType = 'Shield';
    else if (category === 'footgear') specificType = 'Footgear';
    else if (category === 'garments') specificType = 'Garment';
    else if (category === 'accessories') specificType = 'Accessory';
    else specificType = category;

    const desc = lookupDesc(item.name, item.id);
    bucket.set(item.name, { type: specificType, desc, id: item.id });
}

// Process ammo (from equip YAML — Ammo type)
for (const item of equipItems) {
    if (item.type !== 'Ammo' || !item.name) continue;
    if (buckets.consumables.has(item.name)) continue;
    const subType = getAmmoSubType(item.subType);
    const desc = lookupDesc(item.name, item.id);
    buckets.consumables.set(item.name, { type: subType, desc, id: item.id });
}

// Process usable items
for (const item of usableItems) {
    if (!item.name) continue;
    if (buckets.consumables.has(item.name)) continue;

    const specificType = getUsableType(item);
    const desc = lookupDesc(item.name, item.id);
    buckets.consumables.set(item.name, { type: specificType, desc, id: item.id });
}

// Process etc items
for (const item of etcItems) {
    if (!item.name) continue;
    if (buckets.etc_crafting.has(item.name)) continue;

    const specificType = getEtcType(item);
    const desc = lookupDesc(item.name, item.id);
    buckets.etc_crafting.set(item.name, { type: specificType, desc, id: item.id });
}

// ── Merge with existing MD data (preserve implemented items & custom types) ──
function mergeWithExisting(bucketName, mdFilename) {
    const existing = parseExistingMd(mdFilename);
    const bucket = buckets[bucketName];

    // Build set of implemented item names
    const implementedNames = new Set(existing.implemented.map(e => e.name));

    // For implemented items: keep their existing data, but fill in missing descriptions
    for (const entry of existing.implemented) {
        const fromDb = bucket.get(entry.name);
        if (entry.desc === 'No description available' || !entry.desc) {
            // Try to get from description database
            const newDesc = fromDb ? fromDb.desc : lookupDesc(entry.name, fromDb?.id);
            if (newDesc) entry.desc = newDesc;
        }
        // Preserve the existing type if it's already set and specific
        if (fromDb && (entry.type === bucketName || !entry.type)) {
            entry.type = fromDb.type;
        }
    }

    // For not-yet-implemented items from existing MD: update descriptions and types
    const notImplByName = new Map(existing.notImplemented.map(e => [e.name, e]));

    // Merge: existing not-implemented + new items from DB
    const allNotImpl = new Map();

    // First add all existing not-implemented (preserving custom types)
    for (const entry of existing.notImplemented) {
        const fromDb = bucket.get(entry.name);
        // Update description if missing
        if (entry.desc === 'No description available' || !entry.desc) {
            const newDesc = fromDb ? fromDb.desc : lookupDesc(entry.name, fromDb?.id);
            if (newDesc) entry.desc = newDesc;
            else entry.desc = 'No description available';
        }
        // Update type from DB if current type is generic
        if (fromDb && fromDb.type) {
            entry.type = fromDb.type;
        }
        allNotImpl.set(entry.name, entry);
    }

    // Then add new items from DB that weren't in existing MD
    for (const [name, data] of bucket) {
        if (implementedNames.has(name) || allNotImpl.has(name)) continue;
        allNotImpl.set(name, {
            name,
            type: data.type,
            desc: data.desc || 'No description available'
        });
    }

    // Sort not-implemented alphabetically
    const sortedNotImpl = [...allNotImpl.values()].sort((a, b) =>
        a.name.localeCompare(b.name, 'en', { sensitivity: 'base' })
    );

    return { implemented: existing.implemented, notImplemented: sortedNotImpl };
}

// ── Write MD file ──────────────────────────────────────────────────────
function writeMd(filename, title, data) {
    const totalCount = data.implemented.length + data.notImplemented.length;
    let out = `# ${title}\n\n`;
    out += `Total: ${totalCount} (${data.implemented.length} implemented, ${data.notImplemented.length} not yet in DB)\n\n`;
    out += `Format: item_name -> specific_type -> description\n\n`;

    if (data.implemented.length > 0) {
        out += `## Implemented\n\n`;
        for (const e of data.implemented) {
            out += `${e.name} -> ${e.type} -> ${e.desc}\n`;
        }
        out += `\n`;
    }

    if (data.notImplemented.length > 0) {
        out += `## Not Yet Implemented\n\n`;
        for (const e of data.notImplemented) {
            out += `${e.name} -> ${e.type} -> ${e.desc}\n`;
        }
        out += `\n`;
    }

    const filepath = path.join(ITEMS_DIR, filename);
    fs.writeFileSync(filepath, out, 'utf8');
    console.log(`  ${filename}: ${totalCount} items (${data.implemented.length} impl, ${data.notImplemented.length} not impl)`);
}

// ── Generate all files ─────────────────────────────────────────────────
console.log('\nMerging with existing MD files...');

const configs = [
    ['accessories', 'accessories.md', 'Accessories'],
    ['armor', 'armor.md', 'Armor'],
    ['consumables', 'consumables.md', 'Consumables'],
    ['etc_crafting', 'etc_crafting.md', 'Etc / Crafting Materials'],
    ['footgear', 'footgear.md', 'Footgear'],
    ['garments', 'garments.md', 'Garments'],
    ['headgear', 'headgear.md', 'Headgear'],
    ['shields', 'shields.md', 'Shields'],
    ['weapons', 'weapons.md', 'Weapons'],
];

console.log('\nWriting MD files...');
let grandTotal = 0;
let grandImpl = 0;
for (const [bucketName, mdFile, title] of configs) {
    const data = mergeWithExisting(bucketName, mdFile);
    writeMd(mdFile, title, data);
    grandTotal += data.implemented.length + data.notImplemented.length;
    grandImpl += data.implemented.length;
}

console.log(`\nDone! ${grandTotal} total items across 9 files (${grandImpl} implemented)`);
