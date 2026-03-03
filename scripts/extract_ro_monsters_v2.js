/**
 * RO Monster Data Extractor v2 — Fixed Parser
 * 
 * Fixes from v1:
 * - Race: "Demihuman" (no hyphen) now maps correctly
 * - Class: Uses explicit "Class: Boss" field, not AI guessing
 * - MVP: Uses MvpExp > 0 OR Modes.Mvp flag
 * - Drops: Fixed parser to handle indentation edge cases
 * - Filtering: Excludes G_ (guard), EVENT_, YOURNAME, HIDDEN mobs
 * - MvpDrops: Now parsed separately from regular drops
 * 
 * Source: https://raw.githubusercontent.com/rathena/rathena/master/db/pre-re/mob_db.yml
 * Usage: node scripts/extract_ro_monsters_v2.js
 */

const https = require('https');
const fs = require('fs');
const path = require('path');

const MOB_DB_URL = 'https://raw.githubusercontent.com/rathena/rathena/master/db/pre-re/mob_db.yml';

function fetchUrl(url) {
    return new Promise((resolve, reject) => {
        https.get(url, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => resolve(data));
            res.on('error', reject);
        }).on('error', reject);
    });
}

function parseRathenaMobDb(yamlContent) {
    const monsters = [];
    const lines = yamlContent.split('\n');
    let currentMob = null;
    let currentSection = null; // 'drops', 'mvpdrops', 'modes', 'racegroups', null
    let currentDrop = null;
    let drops = [];
    let mvpDrops = [];
    let modes = {};
    let raceGroups = {};

    function finalizeMob() {
        if (!currentMob) return;
        if (currentDrop && currentSection === 'drops') drops.push(currentDrop);
        if (currentDrop && currentSection === 'mvpdrops') mvpDrops.push(currentDrop);
        currentMob.drops = drops;
        currentMob.mvpDrops = mvpDrops;
        currentMob.modes = modes;
        currentMob.raceGroups = raceGroups;
        monsters.push(currentMob);
    }

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const trimmed = line.trim();

        if (trimmed === '' || trimmed.startsWith('#') || trimmed === 'Body:' ||
            trimmed.startsWith('Header:') || trimmed.startsWith('Type:') || trimmed.startsWith('Version:')) {
            continue;
        }

        // New monster entry
        if (trimmed.startsWith('- Id:')) {
            finalizeMob();
            currentMob = { Id: parseInt(trimmed.replace('- Id:', '').trim()) };
            currentSection = null;
            currentDrop = null;
            drops = [];
            mvpDrops = [];
            modes = {};
            raceGroups = {};
            continue;
        }

        if (!currentMob) continue;

        // Section headers
        if (trimmed === 'Drops:') { currentSection = 'drops'; currentDrop = null; continue; }
        if (trimmed === 'MvpDrops:') { currentSection = 'mvpdrops'; currentDrop = null; continue; }
        if (trimmed === 'Modes:') { currentSection = 'modes'; continue; }
        if (trimmed === 'RaceGroups:') { currentSection = 'racegroups'; continue; }

        // Parse section content
        if (currentSection === 'drops' || currentSection === 'mvpdrops') {
            if (trimmed.startsWith('- Item:')) {
                if (currentDrop) {
                    if (currentSection === 'drops') drops.push(currentDrop);
                    else mvpDrops.push(currentDrop);
                }
                currentDrop = { Item: trimmed.replace('- Item:', '').trim() };
                continue;
            }
            if (currentDrop) {
                if (trimmed.startsWith('Rate:')) {
                    currentDrop.Rate = parseInt(trimmed.replace('Rate:', '').trim());
                    continue;
                }
                if (trimmed.startsWith('StealProtected:')) {
                    currentDrop.StealProtected = trimmed.includes('true');
                    continue;
                }
            }
            // Check if we've left the drops section (a non-drop property)
            if (!trimmed.startsWith('- Item:') && !trimmed.startsWith('Rate:') && !trimmed.startsWith('StealProtected:') && !trimmed.startsWith('-')) {
                if (currentDrop) {
                    if (currentSection === 'drops') drops.push(currentDrop);
                    else mvpDrops.push(currentDrop);
                    currentDrop = null;
                }
                currentSection = null;
                // Fall through to parse the property below
            } else {
                continue;
            }
        }

        if (currentSection === 'modes') {
            if (trimmed.includes(':') && !trimmed.startsWith('- ')) {
                const [key, val] = trimmed.split(':').map(s => s.trim());
                if (['Aggressive', 'Detector', 'CastSensorIdle', 'CastSensorChase',
                    'ChangeChase', 'ChangeTargetMelee', 'ChangeTargetChase', 'Angry',
                    'NoRandomWalk', 'Assist', 'Looter', 'Immovable', 'TeleportBlock',
                    'FixedItemDrop', 'IgnoreMagic', 'IgnoreMelee', 'IgnoreRanged',
                    'Mvp', 'IgnoreMisc', 'KnockBackImmune', 'NoCast', 'CanMove',
                    'CanAttack', 'SensorIdle'].includes(key)) {
                    modes[key] = val === 'true';
                    continue;
                }
            }
            currentSection = null;
            // Fall through
        }

        if (currentSection === 'racegroups') {
            if (trimmed.includes(':') && !trimmed.startsWith('- ')) {
                const [key, val] = trimmed.split(':').map(s => s.trim());
                raceGroups[key] = val === 'true';
                continue;
            }
            currentSection = null;
        }

        // Parse mob properties (top-level)
        if (trimmed.includes(':') && !trimmed.startsWith('- ')) {
            const colonIdx = trimmed.indexOf(':');
            const key = trimmed.substring(0, colonIdx).trim();
            const val = trimmed.substring(colonIdx + 1).trim();
            if (val === '') continue;

            const numericFields = ['Level', 'Hp', 'Sp', 'BaseExp', 'JobExp', 'MvpExp',
                'Attack', 'Attack2', 'Defense', 'MagicDefense',
                'Str', 'Agi', 'Vit', 'Int', 'Dex', 'Luk',
                'AttackRange', 'SkillRange', 'ChaseRange',
                'ElementLevel', 'WalkSpeed', 'AttackDelay',
                'AttackMotion', 'ClientAttackMotion', 'DamageMotion'];

            if (numericFields.includes(key)) {
                currentMob[key] = parseInt(val);
            } else {
                currentMob[key] = val;
            }
        }
    }

    finalizeMob();
    return monsters;
}

// ── Mapping Functions ──────────────────────────────────────────

function mapRace(race) {
    const map = {
        'Formless': 'formless', 'Undead': 'undead', 'Brute': 'brute',
        'Plant': 'plant', 'Insect': 'insect', 'Fish': 'fish',
        'Demon': 'demon', 'Demihuman': 'demihuman', 'DemiHuman': 'demihuman',
        'Demi-Human': 'demihuman', 'Angel': 'angel', 'Dragon': 'dragon'
    };
    return map[race] || 'formless';
}

function mapSize(size) {
    return { 'Small': 'small', 'Medium': 'medium', 'Large': 'large' }[size] || 'medium';
}

function mapElement(element, level) {
    const map = {
        'Neutral': 'neutral', 'Water': 'water', 'Earth': 'earth',
        'Fire': 'fire', 'Wind': 'wind', 'Poison': 'poison',
        'Holy': 'holy', 'Dark': 'shadow', 'Shadow': 'shadow',
        'Ghost': 'ghost', 'Undead': 'undead'
    };
    return { type: map[element] || 'neutral', level: level || 1 };
}

function mapMonsterClass(mob) {
    // Use explicit Class field from rAthena
    if (mob.MvpExp && mob.MvpExp > 0) return 'mvp';
    if (mob.modes && mob.modes.Mvp) return 'mvp';
    if (mob.Class === 'Boss') return 'boss';
    if (mob.Class === 'Guardian') return 'guardian';
    return 'normal';
}

function mapAiType(ai, modes) {
    const aiNum = parseInt(ai) || 0;
    const isAggressive = modes && modes.Aggressive;

    // Aggressive: attacks players on sight
    if (isAggressive || [4, 5, 7, 9, 13, 17, 19, 20, 21, 24, 25, 26, 27].includes(aiNum)) {
        return 'aggressive';
    }
    // Reactive: attacks when provoked, chases
    if ([3].includes(aiNum)) {
        return 'reactive';
    }
    // Passive: does not attack unless attacked first
    return 'passive';
}

function convertRange(cells) {
    return (cells || 1) * 50;
}

function convertToASPD(attackDelay) {
    if (!attackDelay || attackDelay <= 0) return 170;
    return Math.max(100, Math.min(190, Math.round(200 - (attackDelay / 50))));
}

function calculateRespawnMs(mob) {
    const cls = mapMonsterClass(mob);
    if (cls === 'mvp') return 3600000;
    if (cls === 'boss') return 1800000;
    const level = mob.Level || 1;
    if (level <= 10) return 5000 + level * 500;
    if (level <= 30) return 8000 + level * 300;
    if (level <= 60) return 12000 + level * 200;
    return 15000 + level * 200;
}

function cleanItemName(aegisName) {
    return (aegisName || '')
        .replace(/_/g, ' ')
        .replace(/\s+/g, ' ')
        .trim();
}

function generateTemplate(mob) {
    const element = mapElement(mob.Element, mob.ElementLevel);
    const monsterClass = mapMonsterClass(mob);
    const aiType = mapAiType(mob.Ai, mob.modes);
    const aspd = convertToASPD(mob.AttackDelay);

    return {
        id: mob.Id,
        aegisName: mob.AegisName || '',
        name: mob.Name,
        level: mob.Level || 1,
        maxHealth: mob.Hp || 50,
        baseExp: mob.BaseExp || 0,
        jobExp: mob.JobExp || 0,
        mvpExp: mob.MvpExp || 0,
        attack: mob.Attack || 1,
        attack2: mob.Attack2 || mob.Attack || 1,
        defense: mob.Defense || 0,
        magicDefense: mob.MagicDefense || 0,
        str: mob.Str || 0,
        agi: mob.Agi || 0,
        vit: mob.Vit || 0,
        int: mob.Int || 0,
        dex: mob.Dex || 0,
        luk: mob.Luk || 0,
        attackRange: convertRange(mob.AttackRange),
        aggroRange: (aiType === 'passive' && !(mob.modes && mob.modes.CastSensorIdle)) ? 0 : convertRange(mob.SkillRange || 10),
        chaseRange: convertRange(mob.ChaseRange || 12),
        aspd: aspd,
        walkSpeed: mob.WalkSpeed || 200,
        attackDelay: mob.AttackDelay || 1500,
        attackMotion: mob.AttackMotion || 500,
        damageMotion: mob.DamageMotion || 300,
        size: mapSize(mob.Size),
        race: mapRace(mob.Race),
        element: element,
        monsterClass: monsterClass,
        aiType: aiType,
        respawnMs: calculateRespawnMs(mob),
        raceGroups: mob.raceGroups || {},
        stats: {
            str: mob.Str || 0,
            agi: mob.Agi || 0,
            vit: mob.Vit || 0,
            int: mob.Int || 0,
            dex: mob.Dex || 0,
            luk: mob.Luk || 0,
            level: mob.Level || 1,
            weaponATK: mob.Attack || 0
        },
        modes: {
            aggressive: !!(mob.modes && mob.modes.Aggressive),
            assist: !!(mob.modes && mob.modes.Assist),
            detector: !!(mob.modes && mob.modes.Detector),
            looter: !!(mob.modes && mob.modes.Looter),
            noRandomWalk: !!(mob.modes && mob.modes.NoRandomWalk),
            immovable: !!(mob.modes && mob.modes.Immovable),
            castSensorIdle: !!(mob.modes && mob.modes.CastSensorIdle),
            castSensorChase: !!(mob.modes && mob.modes.CastSensorChase),
            changeChase: !!(mob.modes && mob.modes.ChangeChase),
            changeTargetMelee: !!(mob.modes && mob.modes.ChangeTargetMelee),
            changeTargetChase: !!(mob.modes && mob.modes.ChangeTargetChase),
            angry: !!(mob.modes && mob.modes.Angry),
            mvp: monsterClass === 'mvp'
        },
        drops: (mob.drops || []).map(d => ({
            itemName: cleanItemName(d.Item),
            rate: (d.Rate || 0) / 100, // rAthena rate is per 10000, convert to percentage
            stealProtected: d.StealProtected || false
        })),
        mvpDrops: (mob.mvpDrops || []).map(d => ({
            itemName: cleanItemName(d.Item),
            rate: (d.Rate || 0) / 100,
        }))
    };
}

// ── Filtering ──────────────────────────────────────────────────

function shouldInclude(mob) {
    if (!mob.Level || !mob.Hp || mob.Hp <= 0) return false;
    if (!mob.Name) return false;

    const aegis = (mob.AegisName || '').toUpperCase();
    
    // Exclude guard/event/special variants
    if (aegis.startsWith('G_')) return false;       // Guard variants (no drops, summoned)
    if (aegis.startsWith('R_')) return false;        // Arena/PvP variants (no drops)
    if (aegis.startsWith('A_')) return false;        // Arena variants
    if (aegis.startsWith('EVENT_')) return false;    // Event mobs
    if (aegis.startsWith('E_')) return false;        // Event prefix
    if (aegis.startsWith('EM_')) return false;       // Event MVP prefix
    if (aegis.startsWith('YOURNAME')) return false;
    if (aegis.includes('HIDDEN')) return false;
    if (aegis.startsWith('EMPELIUM')) return false;  // Emperium (WoE object)
    if (aegis.startsWith('EMPERIUM')) return false;
    if (aegis.startsWith('TREASURE_')) return false;
    if (aegis.startsWith('GUARDIAN_')) return false;
    if (aegis.startsWith('BARRICADE')) return false;
    if (aegis.startsWith('S_EMPEL')) return false;
    if (aegis.startsWith('OBJ_')) return false;
    if (aegis.startsWith('SWITCH')) return false;    // Non-combat objects
    if (aegis.startsWith('SOCCER')) return false;    // Non-combat objects
    if (aegis.startsWith('DREAM_')) return false;    // Dream variants
    
    // Exclude specific IDs known to be special/non-combat mobs
    if (mob.Id >= 1900 && mob.Id <= 1999) return false; // WoE/Guild mobs
    
    // Exclude Guardian/Battlefield class
    if (mob.Class === 'Guardian') return false;
    if (mob.Class === 'Battlefield') return false;
    
    // Must be in classic range (ID 1001-1850 covers all pre-renewal monsters)
    if (mob.Id < 1001 || mob.Id > 1850) return false;
    
    // Level must be 1-134
    if (mob.Level < 1 || mob.Level > 134) return false;
    
    // Exclude mobs with 0 BaseExp AND 0 JobExp AND no drops (non-killable objects)
    if ((mob.BaseExp || 0) === 0 && (mob.JobExp || 0) === 0 && (!mob.drops || mob.drops.length === 0)) return false;
    
    return true;
}

// ── Main ───────────────────────────────────────────────────────

async function main() {
    console.log('Fetching rAthena pre-renewal mob database...');
    const yamlContent = await fetchUrl(MOB_DB_URL);
    console.log(`Downloaded ${yamlContent.length} bytes`);

    console.log('Parsing YAML...');
    const allMonsters = parseRathenaMobDb(yamlContent);
    console.log(`Parsed ${allMonsters.length} total monsters`);

    const filtered = allMonsters.filter(shouldInclude);
    console.log(`Filtered to ${filtered.length} relevant monsters`);

    // Sort by level then ID
    filtered.sort((a, b) => (a.Level || 0) - (b.Level || 0) || a.Id - b.Id);

    const templates = filtered.map(generateTemplate);

    // ── Stats ──────────────────────────────────────────────
    const levelGroups = {};
    const classCounts = {};
    const raceCounts = {};
    const elementCounts = {};
    const aiCounts = {};
    let zeroDropCount = 0;

    for (const t of templates) {
        const range = t.level <= 10 ? '01-10' : t.level <= 20 ? '11-20' : t.level <= 30 ? '21-30' :
            t.level <= 40 ? '31-40' : t.level <= 50 ? '41-50' : t.level <= 60 ? '51-60' :
            t.level <= 70 ? '61-70' : t.level <= 85 ? '71-85' : '86+';
        levelGroups[range] = (levelGroups[range] || 0) + 1;
        classCounts[t.monsterClass] = (classCounts[t.monsterClass] || 0) + 1;
        raceCounts[t.race] = (raceCounts[t.race] || 0) + 1;
        elementCounts[t.element.type] = (elementCounts[t.element.type] || 0) + 1;
        aiCounts[t.aiType] = (aiCounts[t.aiType] || 0) + 1;
        if (t.drops.length === 0) zeroDropCount++;
    }

    console.log('\nLevel distribution:', levelGroups);
    console.log('Class breakdown:', classCounts);
    console.log('Race breakdown:', raceCounts);
    console.log('Element breakdown:', elementCounts);
    console.log('AI breakdown:', aiCounts);
    console.log('Zero-drop monsters:', zeroDropCount);

    // ── Generate Output File ───────────────────────────────
    const outputPath = path.join(__dirname, '..', 'server', 'src', 'ro_monster_templates.js');

    let output = `/**
 * Ragnarok Online Monster Templates — Pre-Renewal Database
 * 
 * Auto-generated from rAthena pre-renewal mob_db.yml
 * Source: https://github.com/rathena/rathena/blob/master/db/pre-re/mob_db.yml
 * 
 * Total monsters: ${templates.length}
 * Level range: ${templates[0]?.level || 1} - ${templates[templates.length - 1]?.level || 134}
 * 
 * Generated: ${new Date().toISOString()}
 * 
 * Structure per template:
 *   id, aegisName, name, level, maxHealth, baseExp, jobExp, mvpExp,
 *   attack, attack2, defense, magicDefense,
 *   str, agi, vit, int, dex, luk,
 *   attackRange, aggroRange, chaseRange,
 *   aspd, walkSpeed, attackDelay, attackMotion, damageMotion,
 *   size, race, element {type, level}, monsterClass, aiType,
 *   respawnMs, raceGroups, stats {}, modes {}, drops [], mvpDrops []
 */

'use strict';

`;

    // Build the templates object as proper JS (not JSON) for readability
    output += `const RO_MONSTER_TEMPLATES = {};\n\n`;

    for (const t of templates) {
        const key = t.aegisName.toLowerCase();
        output += `// ──── ${t.name} (ID: ${t.id}) ──── Level ${t.level} | HP ${t.maxHealth.toLocaleString()} | ${t.monsterClass.toUpperCase()} | ${t.race}/${t.element.type}${t.element.level} | ${t.aiType}\n`;
        output += `RO_MONSTER_TEMPLATES['${key}'] = {\n`;
        output += `    id: ${t.id}, name: '${t.name.replace(/'/g, "\\'")}', aegisName: '${t.aegisName}',\n`;
        output += `    level: ${t.level}, maxHealth: ${t.maxHealth}, baseExp: ${t.baseExp}, jobExp: ${t.jobExp}, mvpExp: ${t.mvpExp},\n`;
        output += `    attack: ${t.attack}, attack2: ${t.attack2}, defense: ${t.defense}, magicDefense: ${t.magicDefense},\n`;
        output += `    str: ${t.str}, agi: ${t.agi}, vit: ${t.vit}, int: ${t.int}, dex: ${t.dex}, luk: ${t.luk},\n`;
        output += `    attackRange: ${t.attackRange}, aggroRange: ${t.aggroRange}, chaseRange: ${t.chaseRange},\n`;
        output += `    aspd: ${t.aspd}, walkSpeed: ${t.walkSpeed}, attackDelay: ${t.attackDelay}, attackMotion: ${t.attackMotion}, damageMotion: ${t.damageMotion},\n`;
        output += `    size: '${t.size}', race: '${t.race}', element: { type: '${t.element.type}', level: ${t.element.level} },\n`;
        output += `    monsterClass: '${t.monsterClass}', aiType: '${t.aiType}', respawnMs: ${t.respawnMs},\n`;

        // Race groups
        const rgKeys = Object.keys(t.raceGroups);
        if (rgKeys.length > 0) {
            output += `    raceGroups: { ${rgKeys.map(k => `${k}: true`).join(', ')} },\n`;
        } else {
            output += `    raceGroups: {},\n`;
        }

        // Stats block
        output += `    stats: { str: ${t.stats.str}, agi: ${t.stats.agi}, vit: ${t.stats.vit}, int: ${t.stats.int}, dex: ${t.stats.dex}, luk: ${t.stats.luk}, level: ${t.stats.level}, weaponATK: ${t.stats.weaponATK} },\n`;

        // Modes (only include true values for readability)
        const trueModes = Object.entries(t.modes).filter(([, v]) => v).map(([k]) => `${k}: true`);
        if (trueModes.length > 0) {
            output += `    modes: { ${trueModes.join(', ')} },\n`;
        } else {
            output += `    modes: {},\n`;
        }

        // Drops
        if (t.drops.length > 0) {
            output += `    drops: [\n`;
            for (const d of t.drops) {
                output += `        { itemName: '${d.itemName.replace(/'/g, "\\'")}', rate: ${d.rate}${d.stealProtected ? ', stealProtected: true' : ''} },\n`;
            }
            output += `    ],\n`;
        } else {
            output += `    drops: [],\n`;
        }

        // MVP drops
        if (t.mvpDrops.length > 0) {
            output += `    mvpDrops: [\n`;
            for (const d of t.mvpDrops) {
                output += `        { itemName: '${d.itemName.replace(/'/g, "\\'")}', rate: ${d.rate} },\n`;
            }
            output += `    ],\n`;
        } else {
            output += `    mvpDrops: [],\n`;
        }

        output += `};\n\n`;
    }

    // Summary comments
    output += `// ════════════════════════════════════════════════════════════\n`;
    output += `// SUMMARY\n`;
    output += `// ════════════════════════════════════════════════════════════\n`;
    output += `// Total monsters: ${templates.length}\n`;
    for (const [range, count] of Object.entries(levelGroups)) {
        output += `// Level ${range}: ${count}\n`;
    }
    output += `//\n`;
    for (const [cls, count] of Object.entries(classCounts)) {
        output += `// ${cls}: ${count}\n`;
    }
    output += `//\n`;
    for (const [race, count] of Object.entries(raceCounts)) {
        output += `// ${race}: ${count}\n`;
    }
    output += `//\n`;
    output += `// Monsters with drops: ${templates.length - zeroDropCount}\n`;
    output += `// Monsters without drops: ${zeroDropCount}\n`;

    output += `\nmodule.exports = { RO_MONSTER_TEMPLATES };\n`;

    fs.writeFileSync(outputPath, output, 'utf8');
    console.log(`\nGenerated: ${outputPath}`);
    console.log(`File size: ${(output.length / 1024).toFixed(1)} KB`);

    // Summary JSON
    const summaryPath = path.join(__dirname, '..', 'server', 'src', 'ro_monsters_summary.json');
    const summary = templates.map(t => ({
        id: t.id, aegisName: t.aegisName, name: t.name, level: t.level, hp: t.maxHealth,
        baseExp: t.baseExp, jobExp: t.jobExp, mvpExp: t.mvpExp,
        attack: t.attack, defense: t.defense, magicDefense: t.magicDefense,
        element: t.element.type, elementLevel: t.element.level,
        race: t.race, size: t.size, class: t.monsterClass, aiType: t.aiType,
        dropCount: t.drops.length, mvpDropCount: t.mvpDrops.length
    }));
    fs.writeFileSync(summaryPath, JSON.stringify(summary, null, 2), 'utf8');
    console.log(`Generated summary: ${summaryPath}`);
}

main().catch(err => {
    console.error('Fatal error:', err);
    process.exit(1);
});
