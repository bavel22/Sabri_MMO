// Generate the master enemy markdown table from the dump.
const fs = require('fs');

const list = JSON.parse(fs.readFileSync('C:/Sabri_MMO/_prompts/.enemy_dump.json', 'utf8'));

function classTag(t) {
    if (t.monsterClass === 'mvp')  return 'MVP';
    if (t.monsterClass === 'boss') return 'BOSS';
    return '';
}

function escape(s) {
    return String(s || '').replace(/\|/g, '\\|');
}

function spriteStatus(e) {
    if (e.hasAtlas) {
        if (e.sharedWith)  return `done (shares \`${e.sharedWith}\`)`;
        if (e.spriteTint)  return 'done (tinted)';
        return 'done';
    }
    if (e.glb) {
        if (e.sharedWith)  return `pending (shares \`${e.sharedWith}\` GLB)`;
        return 'pending (GLB ready)';
    }
    return 'no GLB';
}

function makeRow(e) {
    const lvl = String(e.level).padStart(3, ' ');
    const cls = classTag(e);
    const klass = cls ? ` **${cls}**` : '';
    return `| ${lvl} | ${e.id} | \`${escape(e.key)}\` | ${escape(e.name)}${klass} | ${escape(e.race)} | ${escape(e.element)} | ${escape(e.size)} | ${escape(e.preset)} | ${spriteStatus(e)} |`;
}

const HEADER = `| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |`;

const buckets = [
    { label: '1-10',   min: 1,   max: 10 },
    { label: '11-20',  min: 11,  max: 20 },
    { label: '21-30',  min: 21,  max: 30 },
    { label: '31-40',  min: 31,  max: 40 },
    { label: '41-50',  min: 41,  max: 50 },
    { label: '51-60',  min: 51,  max: 60 },
    { label: '61-70',  min: 61,  max: 70 },
    { label: '71-80',  min: 71,  max: 80 },
    { label: '81-90',  min: 81,  max: 90 },
    { label: '91-100', min: 91,  max: 100 },
    { label: '101+',   min: 101, max: 999 },
];

const totalDone   = list.filter(e => e.hasAtlas).length;
const totalReady  = list.filter(e => e.glb && !e.hasAtlas).length;
const totalNoGlb  = list.filter(e => !e.glb).length;
const totalShared = list.filter(e => e.sharedWith).length;

let md = '';
md += '\n\n---\n\n';
md += `## MASTER ENEMY LIST (Generated 2026-04-18)\n\n`;
md += `Cross-referenced with \`server/src/ro_monster_templates.js\` (${list.length} templates), `;
md += `\`2D animations/3d_models/enemies/\` (GLBs), and `;
md += `\`client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/\` (atlases).\n\n`;
md += `### Summary\n`;
md += `- **${totalDone}** enemies rendered or inheriting a rendered sprite (\`done\`)\n`;
md += `- **${totalReady}** enemies with GLB ready or inheriting a GLB, sprite pending (\`pending\`)\n`;
md += `- **${totalNoGlb}** enemies with no GLB yet (\`no GLB\`)\n`;
md += `- ${totalShared} of these inherit sprite from a base variant (\`shares X\` in the Sprite column)\n`;
md += `- **${list.length}** total monster templates\n\n`;
md += `### Sprite Column Legend\n`;
md += `- \`done\` — atlas exists in \`Body/enemies/{name}/\`\n`;
md += `- \`done (shares X)\` — auto-inherits X's atlas via variant rule (meta_X / provoke_X / same-name X_ suffix). NO extra work needed; just set \`spriteClass: 'X'\` in the template\n`;
md += `- \`done (tinted)\` — uses another monster's atlas with \`spriteTint\` recolor\n`;
md += `- \`pending (GLB ready)\` — GLB file exists; just needs render + pack\n`;
md += `- \`pending (shares X GLB)\` — no own GLB, but base variant has one; when X is rendered, this entry becomes done automatically\n`;
md += `- \`no GLB\` — needs Tripo3D model first\n\n`;
md += `### Variant-Inheritance Rules\n`;
md += `The matcher auto-detects these sprite-sharing patterns:\n`;
md += `- \`meta_X\`, \`provoke_X\` — always share X's sprite (metamorphosis/provoked forms are visually identical)\n`;
md += `- \`X_\` (single trailing \`_\`) — shares X's sprite ONLY when the display name is identical (e.g. \`fabre_\` = "Fabre" shares \`fabre\`; \`poring_\` = "Santa Poring" does NOT share \`poring\`)\n`;
md += `- Chains cascade: \`meta_picky_\` → \`picky_\` → \`picky\` (stops at the first ancestor with a sprite)\n\n`;
md += `### Preset Column\n`;
md += `Suggested \`render_monster.py\` preset. \`?\` means unknown — pick based on the model when you have it. `;
md += `Available presets: blob, caterpillar, rabbit, egg, frog, tree, bird, flying_insect, bat, quadruped, plant, biped_insect.\n\n`;
md += `### Class Tags\n`;
md += `**BOSS** = boss-protocol (knockback/status immune). **MVP** = MVP fanfare on death.\n\n`;

for (const b of buckets) {
    const subset = list.filter(e => e.level >= b.min && e.level <= b.max);
    if (!subset.length) continue;
    const done = subset.filter(e => e.hasAtlas).length;
    const ready = subset.filter(e => e.glb && !e.hasAtlas).length;
    md += `### Level ${b.label} (${subset.length} total — ${done} done, ${ready} GLB ready)\n\n`;
    md += HEADER + '\n';
    for (const e of subset) md += makeRow(e) + '\n';
    md += '\n';
}

md += `### Note on Unmapped GLB Files\n`;
md += `- \`rocker-fixed.glb\`, \`rocker_rigged.glb\` — render-experiment variants of \`rocker.glb\`, ignored\n`;
md += `- All other GLBs are now bound to a template (via direct name match, spriteClass, or explicit alias in \`build_enemy_list.js\`)\n\n`;
md += `### How To Use This List\n`;
md += `1. Pick the next \`pending (GLB ready)\` row (top-most / lowest level).\n`;
md += `2. Run the render command using the suggested preset (see top of this file).\n`;
md += `3. Pack with \`pack_atlas.py\` to \`Body/enemies/{name}/\`.\n`;
md += `4. Add \`spriteClass: '{name}', weaponMode: 0\` to the template in \`ro_monster_templates.js\`.\n`;
md += `5. Any \`pending (shares {name} GLB)\` entries will auto-update to \`done (shares {name})\` on next regen.\n`;
md += `6. Import PNGs into UE5 (UserInterface2D, Nearest, NoMipmaps, Never Stream). Restart server.\n\n`;
md += `### Regenerating This List\n`;
md += `After rendering more sprites or adding more GLBs, run:\n`;
md += `\`\`\`bash\n`;
md += `cd /c/Sabri_MMO\n`;
md += `node _prompts/build_enemy_list.js       # → _prompts/.enemy_dump.json\n`;
md += `node _prompts/build_enemy_markdown.js   # → _prompts/.enemy_table.md\n`;
md += `\`\`\`\n`;
md += `Then replace this section of \`enemy_sprite_session_resume.md\` (everything after the top-level separator that introduces "MASTER ENEMY LIST") with the contents of \`.enemy_table.md\`.\n`;

fs.writeFileSync('C:/Sabri_MMO/_prompts/.enemy_table.md', md);
console.log('Wrote', md.length, 'chars,', md.split('\n').length, 'lines');
