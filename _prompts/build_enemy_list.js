// Generator: scan templates + GLBs + atlases → structured enemy list.
// Variant-inheritance: meta_X, provoke_X, and same-name _suffix variants
// automatically share their base's sprite.
const { RO_MONSTER_TEMPLATES } = require('../server/src/ro_monster_templates');
const fs = require('fs');
const path = require('path');

// ─── GLB filename (stem) → template key (typos, renamings, variants) ───
// Values may be a string (one template) or array (one GLB serves many templates via tint/share).
const GLB_ALIAS = {
    // Single-template typos / renames
    drainiliar:        'drainliar',
    eggrya:            'eggyra',
    skeleton_t_pose:   'skeleton',
    thief_bug_f:       'thief_bug_',
    thief_bug_male:    'thief_bug__',
    wormtail:          'worm_tail',
    boa:               'snake',
    orc_warrior:       'ork_warrior',
    marlin:            'marse',
    santa_poring:      'poring_',
    wind_crystal:      'crystal_1',
    earth_crystal:     'crystal_2',
    fire_crystal:      'crystal_3',
    ice_crystal:       'crystal_4',
    goblin_xmas:       'gobline_xmas',
    'orc baby':        'orc_baby',
    bogun:             'bon_gun',
    blazer:            'blazzer',
    myst_case:         'mystcase',
    swordfish:         'sword_fish',
    sandman:           'sand_man',
    sage_worm:         'sageworm',
    miyabi_doll:       'miyabi_ningyo',
    holden:            'mole',
    beetle_king:       'kind_of_beetle',
    dumpling_child:    'rice_cake_boy',
    firelock_soldier:  'antique_firelock',
    jing_guai:         'li_me_mang_ryang',
    baphomet_jr:       'baphomet_',
    hermit_plant:      'wild_ginseng',
    green_maiden:      'chung_e',
    nereid:            'neraid',

    // Multi-template shares (one GLB serves many templates via tint/variant)
    goblin:            ['goblin_1', 'goblin_2', 'goblin_3', 'goblin_4', 'goblin_5'],
    kobold:            ['kobold_1', 'kobold_2', 'kobold_3'],
    plasma:            ['plasma_r', 'plasma_b', 'plasma_g', 'plasma_p'],
    novus_blue:        ['novus'],
    novus_green:       ['novus_'],
    petite_blue:       ['petit'],
    petite_green:      ['petit_'],
    soldier_skeleton:  ['soldier_skeleton', 'archer_skeleton'],
    whisper:           ['whisper', 'whisper_boss'],
    hill_wind:         ['hill_wind', 'hill_wind_1'],
    savage:            ['savage', 'm_savage'],
    desert_wolf:       ['desert_wolf', 'm_desert_wolf'],
};

// ─── Preset suggestions ───
const PRESET_SUGGESTIONS = {
    poring: 'blob', drops: 'blob', poporing: 'blob', plankton: 'blob',
    marin: 'blob', marina: 'blob', eggyra: 'blob', stapo: 'blob',
    fabre: 'caterpillar', worm_tail: 'caterpillar', snake: 'caterpillar',
    anacondaq: 'caterpillar', hode: 'caterpillar', sandman: 'caterpillar',
    lunatic: 'rabbit', smokie: 'rabbit', tarou: 'rabbit', coco: 'rabbit',
    martin: 'rabbit', eclipse: 'rabbit',
    pupa: 'egg', thief_bug_egg: 'egg', ant_egg: 'egg', pecopeco_egg: 'egg',
    picky: 'egg', picky_: 'egg', ambernite: 'egg', shellfish: 'egg',
    cornutus: 'egg',
    roda_frog: 'frog', toad: 'frog', kukre: 'frog', vadon: 'frog',
    thara_frog: 'frog', aster: 'frog', crab: 'frog',
    wilow: 'tree', poison_spore: 'tree', spore: 'tree', elder_wilow: 'tree',
    golem: 'tree', rafflesia: 'tree', antonio: 'tree', mastering: 'tree',
    condor: 'bird', pecopeco: 'bird', peco_peco: 'bird',
    hornet: 'flying_insect', creamy: 'flying_insect', chonchon: 'flying_insect',
    steel_chonchon: 'flying_insect', dragon_fly: 'flying_insect',
    dustiness: 'flying_insect', stainer: 'flying_insect', anopheles: 'flying_insect',
    farmiliar: 'bat', drainliar: 'bat',
    savage_babe: 'quadruped', yoyo: 'quadruped', desert_wolf_b: 'quadruped',
    wolf: 'quadruped', desert_wolf: 'quadruped', vagabond_wolf: 'quadruped',
    raggler: 'quadruped', caramel: 'quadruped', savage: 'quadruped',
    bigfoot: 'quadruped', sasquatch: 'quadruped',
    mandragora: 'plant', flora: 'plant', muka: 'plant', hydra: 'plant',
    rocker: 'biped_insect', vocal: 'biped_insect',
    thief_bug: 'biped_insect', thief_bug_: 'biped_insect', thief_bug__: 'biped_insect',
    andre: 'biped_insect', deniro: 'biped_insect', piere: 'biped_insect',
    vitata: 'biped_insect', horn: 'biped_insect', metaller: 'biped_insect',
    goblin_5: 'biped_insect', goblin_archer: 'biped_insect', argos: 'biped_insect',
    mantis: 'biped_insect', scorpion: 'biped_insect', zerom: 'biped_insect',
    skeleton: 'biped_insect', orc_skeleton: 'biped_insect',
    soldier_skeleton: 'biped_insect', skel_worker: 'biped_insect',
    munak: 'biped_insect', ork_warrior: 'biped_insect', orc_zombie: 'biped_insect',
    zombie: 'biped_insect', orc_baby: 'biped_insect',
    angeling: '?', ghostring: '?',
    crystal_1: '?', crystal_2: '?', crystal_3: '?', crystal_4: '?',
    cookie_xmas: '?', orc_xmas: 'biped_insect',
    poring_: 'blob',
};

function inferPreset(t) {
    if (PRESET_SUGGESTIONS[t.key]) return PRESET_SUGGESTIONS[t.key];
    const k = t.key.toLowerCase();
    if (k.includes('egg') || k.endsWith('_egg')) return 'egg';
    if (k.includes('plant') || k.includes('mushroom')) return 'tree';
    if (k.includes('skeleton') || k.includes('skel')) return 'biped_insect';
    if (k.includes('zombie')) return 'biped_insect';
    if (k.includes('orc') || k.includes('ork')) return 'biped_insect';
    if (k.includes('goblin')) return 'biped_insect';
    if (k.includes('wolf')) return 'quadruped';
    if (k.includes('bat')) return 'bat';
    if (k.includes('snake') || k.includes('worm')) return 'caterpillar';
    if (k.includes('fish') || k.includes('frog') || k.includes('toad')) return 'frog';
    if (k.includes('butterfly') || k.includes('dragonfly') || k.includes('moth')) return 'flying_insect';
    if (k.includes('bird') || k.includes('eagle') || k.includes('hawk')) return 'bird';
    if (k.includes('wraith') || k.includes('ghost')) return '?';
    const race = (t.race || '').toLowerCase();
    if (race === 'plant') return 'tree';
    if (race === 'fish') return 'frog';
    if (race === 'insect') return 'biped_insect';
    if (race === 'brute' && (t.size === 'small' || t.size === 'medium')) return 'quadruped';
    if (race === 'demihuman') return 'biped_insect';
    if (race === 'undead') return 'biped_insect';
    return '?';
}

// ─── Variant-inheritance: find a chain of base templates to inherit sprite from ───
// Returns an ordered list of candidate base keys (most specific first).
//   meta_picky_ → ['picky_', 'picky']  (so we can fall through)
//   fabre_      → ['fabre']            (same-name variant)
//   meta_fabre  → ['fabre']
function findBaseCandidates(key, templates) {
    const self = templates[key];
    if (!self) return [];
    const out = [];

    let base = key;
    let viaPrefix = null;
    if (base.startsWith('meta_'))         { base = base.slice(5); viaPrefix = 'meta'; }
    else if (base.startsWith('provoke_')) { base = base.slice(8); viaPrefix = 'provoke'; }

    // Prefix strip always creates an inheritance chain regardless of name match
    if (viaPrefix) {
        if (templates[base]) out.push(base);
        let trim = base;
        while (trim.endsWith('_')) {
            trim = trim.slice(0, -1);
            if (templates[trim] && !out.includes(trim)) out.push(trim);
        }
    } else {
        // No prefix: only inherit from same-NAME variants (fabre_ → fabre, not poring_ → poring)
        let trim = base;
        while (trim.endsWith('_')) {
            trim = trim.slice(0, -1);
            const baseTpl = templates[trim];
            if (baseTpl && baseTpl.name === self.name && !out.includes(trim)) {
                out.push(trim);
            }
        }
    }

    return out;
}

// ─── Scan filesystem ───
const glbDir = 'C:/Sabri_MMO/2D animations/3d_models/enemies';
const glbFiles = fs.readdirSync(glbDir).filter(f => f.toLowerCase().endsWith('.glb'));
const glbStemToFile = new Map();
glbFiles.forEach(f => glbStemToFile.set(path.parse(f).name.toLowerCase(), f));

const atlasDir = 'C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies';
const atlasSet = new Set(
    fs.readdirSync(atlasDir)
        .filter(f => fs.statSync(path.join(atlasDir, f)).isDirectory())
        .map(s => s.toLowerCase())
);

// ─── Template → GLB mapping (direct, before inheritance) ───
const templateToGlb = new Map();

for (const [glbStem, templKeys] of Object.entries(GLB_ALIAS)) {
    if (!glbStemToFile.has(glbStem)) continue;
    const file = glbStemToFile.get(glbStem);
    const keys = Array.isArray(templKeys) ? templKeys : [templKeys];
    for (const templKey of keys) {
        if (RO_MONSTER_TEMPLATES[templKey] && !templateToGlb.has(templKey)) {
            templateToGlb.set(templKey, file);
        }
    }
}
for (const [glbStem, file] of glbStemToFile.entries()) {
    if (RO_MONSTER_TEMPLATES[glbStem] && !templateToGlb.has(glbStem)) {
        templateToGlb.set(glbStem, file);
    }
}
for (const [key, t] of Object.entries(RO_MONSTER_TEMPLATES)) {
    if (templateToGlb.has(key)) continue;
    const sc = (t.spriteClass || '').toLowerCase();
    if (sc && glbStemToFile.has(sc)) templateToGlb.set(key, glbStemToFile.get(sc));
}

// ─── Build list with variant inheritance ───
const list = [];
for (const [key, t] of Object.entries(RO_MONSTER_TEMPLATES)) {
    const ownGlb = templateToGlb.get(key) || null;
    const ownAtlasKey = (t.spriteClass || key).toLowerCase();
    const ownAtlas = atlasSet.has(ownAtlasKey);

    // If this template has no direct sprite/GLB, walk the base-variant chain
    let effectiveGlb = ownGlb;
    let effectiveAtlas = ownAtlas;
    let sharedWith = null;
    if (!ownGlb && !ownAtlas) {
        for (const baseKey of findBaseCandidates(key, RO_MONSTER_TEMPLATES)) {
            const baseTpl = RO_MONSTER_TEMPLATES[baseKey];
            const baseGlb = templateToGlb.get(baseKey) || null;
            const baseAtlasKey = ((baseTpl && baseTpl.spriteClass) || baseKey).toLowerCase();
            const baseAtlas = atlasSet.has(baseAtlasKey);
            if (baseGlb || baseAtlas) {
                effectiveGlb = baseGlb;
                effectiveAtlas = baseAtlas;
                sharedWith = baseKey;
                break;
            }
        }
    }

    const entry = {
        key, id: t.id, name: t.name || key, level: t.level || 0,
        spriteClass: t.spriteClass || null,
        spriteTint: t.spriteTint || null,
        monsterClass: t.monsterClass || 'normal',
        race: t.race || '',
        element: t.element ? (t.element.type + (t.element.level || '')) : '',
        size: t.size || '',
        attackRange: t.attackRange || 0,
        glb: effectiveGlb,
        hasAtlas: effectiveAtlas,
        sharedWith,
        ownGlb: !!ownGlb,
    };
    entry.preset = inferPreset(entry);
    list.push(entry);
}
list.sort((a, b) => (a.level - b.level) || a.id - b.id);

const done = list.filter(e => e.hasAtlas).length;
const ready = list.filter(e => !e.hasAtlas && e.glb).length;
const pending = list.filter(e => !e.hasAtlas && !e.glb).length;
console.log('Total:', list.length, '| Done:', done, '| GLB ready:', ready, '| No GLB:', pending);

const orphanGlbs = glbFiles.filter(g => ![...templateToGlb.values()].includes(g));
console.log('Orphan GLBs:', orphanGlbs);

fs.writeFileSync('C:/Sabri_MMO/_prompts/.enemy_dump.json', JSON.stringify(list, null, 2));
