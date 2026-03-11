/**
 * Generate item list .md files by category
 * Reads implemented items from init.sql (with descriptions) and
 * unimplemented items from monster drop tables
 */
const fs = require('fs');
const path = require('path');

const { RO_MONSTER_TEMPLATES } = require('../server/src/ro_monster_templates.js');
const { RO_ITEM_NAME_TO_ID } = require('../server/src/ro_item_mapping.js');

// ── Parse init.sql for implemented items with descriptions ──
const initSql = fs.readFileSync(path.join(__dirname, '..', 'database', 'init.sql'), 'utf8');

const implementedItems = {};

// Match all INSERT INTO items lines — capture values between parentheses
const insertRegex = /\((\d+),\s*'([^']*(?:''[^']*)*)',\s*'([^']*(?:''[^']*)*)',\s*'([^']*(?:''[^']*)*)'/g;
let match;
while ((match = insertRegex.exec(initSql)) !== null) {
  const id = parseInt(match[1]);
  const name = match[2].replace(/''/g, "'");
  const desc = match[3].replace(/''/g, "'");
  const itemType = match[4].replace(/''/g, "'");

  // Also try to get equip_slot, atk, def from the full line
  implementedItems[name] = { id, name, description: desc, itemType };
}

// Second pass for more details (equip_slot, atk, def, weapon_type)
const detailRegex = /\((\d+),\s*'([^']*(?:''[^']*)*)',\s*'([^']*(?:''[^']*)*)',\s*'(\w+)',\s*'(\w+)',\s*(\d+),\s*(\d+)(?:,\s*(\d+))?/g;
while ((match = detailRegex.exec(initSql)) !== null) {
  const name = match[2].replace(/''/g, "'");
  if (implementedItems[name]) {
    implementedItems[name].equipSlot = match[5];
  }
}

// ── Collect all drop names from monster templates ──
const allDropNames = new Set();
for (const [key, mon] of Object.entries(RO_MONSTER_TEMPLATES)) {
  if (Array.isArray(mon.drops)) for (const d of mon.drops) allDropNames.add(d.itemName);
  if (Array.isArray(mon.mvpDrops)) for (const d of mon.mvpDrops) allDropNames.add(d.itemName);
}

// Also add all implemented items
for (const name of Object.keys(implementedItems)) {
  allDropNames.add(name);
}
// Add items from RO_ITEM_NAME_TO_ID mapping
for (const name of Object.keys(RO_ITEM_NAME_TO_ID)) {
  allDropNames.add(name);
}

// ── Category sets ──
const weapons = new Set([
  'Rustic Shiv','Keen Edge','Stiletto Fang','Iron Cleaver','Crescent Saber','Hunting Longbow',
  'Knife','Cutter','Main Gauche','Falchion','Mace','Rod','Bow','Javelin','Spear','Axe','Club',
  'Wand','Guitar Of Vast Land','Whip Of Earth',
  'Ahlspiess','Air Rifle','Altas Weapon','Arbalest','Arc Wand','Asura','Azoth','Balistar',
  'Base Guitar','Bastard Sword','Battle Axe','Berserk Guitar','Bible','Blade',
  'Blade Lost In Darkness','Blade Of Pinwheel','Bladed Whip','Blessed Wand','Blood Tears',
  'Bloody Edge','Bloody Roar','Bone Wand','Book','Book Of Billows','Book Of Blazing Sun',
  'Book Of Devil','Book Of Gust Of Wind','Book Of Mother Earth','Book Of The Apocalypse',
  'Bow Of Rudra','Brionac','Broad Sword','Brood Axe','Burning Bow','Buster','Cakram',
  'Carrot Whip','Chemeti','Cinquedea','Claymore','Combat Knife','Combo Battle Glove',
  'Composite Bow','Counter Dagger','Crescent Scythe','Crimson Bolt','Croce Staff','CrossBow',
  'Cursed Dagger','Curved Sword','Cutlas','Dagger','Damascus','Danggie','Dea Staff','Destroyer',
  'Diary Of Great Sage','Dirk','Doom Slayer','Dragon Killer','Dragon Slayer','Drifter',
  'Drill Katar','Earth Bow','Edge','Electric Fist','Encyclopedia','Fan','Flail','Forturn Sword',
  'Fright Paper Blade','Frozen Bow','Gae Bolg','Gate Keeper','Gate KeeperDD','Ginnungagap',
  'Gladius','Glaive','Golden Mace','Grand Cross','Great Axe','Great Bow','Grimtooth',
  'Guillotine','Guisarme','Guitar','Guitar Of Blue Solo','Guitar Of Gentle Breeze',
  'Guitar Of Passion','Gust Bow','Hae Dong Gum','Halberd','Hammer','Hammer Of Blacksmith',
  'Harp','Healing Staff','Heart Breaker','Hell Fire','Hora','Hurricane Fury',
  'Huuma Bird Wing','Huuma Blaze','Huuma Calm Mind','Huuma Giant Wheel','Ice Falchon',
  'Icicle Fist','Immaterial Sword','Infiltrator','Iron Cane','Iron Driver','Jamadhar','Jitte',
  'Jump Rope','Jur','Kakkung','Kandura','Katana','Katar','Katar Of Cold Icicle',
  'Katar Of Piercing Wind','Katar Of Raging Blaze','Katar Of Thornbush','Katzbalger','Khukri',
  'Krasnaya','Krieg','Krishna','Kronos','Lance','Lapier','Lariat','Long Barrel','Long Mace',
  'Luna Bow','Lute','Magma Fist','Mandolin','Masamune','Memorize Book','Mighty Staff',
  'Moonlight Sword','Morning Star','Muramasa','Muscle Cutter','Mysteltainn','Nagan',
  'Orc Archer Bow','Orcish Axe','Orcish Sword','Oriental Lute','Partizan','Piercing Staff',
  'Pike','Poison Knife','Pole Axe','Quadrille','Rante','Ring Pommel Saber','Rope','Sabbath',
  'Saber','Scalpel','Schweizersabel','Scimiter','Seismic Fist','Shine Spear Blade',
  'Short Daenggie','Six Shooter','Skewer','Slayer','Solar Sword','Spectral Spear','Spike',
  'Staff','Staff Of Bordeaux','Staff Of Soul','Staff Of Wing','Stiletto','Sucsamad',
  'Survival Rod','Survival Rod2','Sword Mace','The Cyclone','The Garrison','Tomahawk',
  'Town Sword','Trident','Tsurugi','Two Hand Sword','Two Handed Axe','Various Jur',
  'Vecer Axe','Violin','Waghnakh','Walking Stick','War Axe','Whip Of Ice Piece',
  'Whip Of Red Flame','Windhawk','Wizardy Staff','Zephyrus','Zweihander',
]);

const armors = new Set([
  'Linen Tunic','Quilted Vest','Ringweave Hauberk','Silk Robe',
  'Cotton Shirt','Padded Armor','Chain Mail','Full Plate Armor','Coat',
  'Silver Robe','Saint Robe','Mage Coat','Holy Robe','Glittering Clothes','Formal Suit',
  'Ninja Suit','Adventure Suit','Plate Armor','Meteo Plate Armor',
  'Taegeuk Plate','Sniping Suit','Thief Clothes','Dullahan Armor','Clothes Of The Lord',
  'Flame Sprits Armor','Water Sprits Armor','Wind Sprits Armor','Earth Sprits Armor',
  'Celestial Robe','Divine Cloth','Limpid Celestial Robe','Wooden Mail','Falcon Robe',
  'Robe Of Casting','Leaf Clothes','Valkyrie Armor','Leather Jacket',
  'G Strings','Wedding Dress','Worn Out Prison Uniform','Old Japaness Clothes',
  "Adventurere's Suit",
]);

const shields = new Set([
  'Guard','Buckler','Shield','Mirror Shield','Stone Buckler','Strong Shield',
  'Platinum Shield','Thorny Buckler','Holy Guard','Herald Of GOD',
]);

const headgears = new Set([
  'Hat','Cap','Helm','Biretta','Crown','Tiara','Goggle','Circlet','Bandana','Ribbon',
  'Cat Hairband','Green Feeler','Pierrot Nose','Horrendous Mouth','Fancy Flower',
  'Beret','Fedora','Headlamp','Corsair','Bone Helm','Gemmed Crown','Gemmed Sallet',
  'Coronet','Fillet','Hair Band','Hair Protector','Fin Helm','Poo Poo Hat',
  'Pumpkin Head','Spinx Helm','Safety Helmet','Nurse Cap','Ph.D Hat',
  'Magestic Goat',"Big Sis' Ribbon",'Ear Of Puppy','Pirate Bandana','Panda Cap',
  'Bongun Hat','Viking Helm','Super Novice Hat','Hat Of Cake','Suspicious Hat',
  'Cone Hat','Angry Mouth',"Tutankhamen's Mask",'Masquerade','Loard Circlet',
  'Blue Coif','Binoculars','Cigar','Smoking Pipe','Gangster Patch','Ganster Mask',
  'Gas Mask','Granpa Beard','Ghost Bandana','Dark Blindfold','Festival Mask',
  "Santa's Hat",'Pumpkin Bucket','Bib','Poring Hat','Assassin Mask',
  'Indian Hair Piece','Pacifier','Mini Propeller',
  'Goblin Mask 01','Goblin Mask 02','Goblin Mask 03','Goblin Mask 04','Goblini Mask',
  'Nose Ring','Sacred Masque','Fricca Circlet','Magni Cap',
  'Joker Jester','Transparent Headgear','White Mask','Black Mask',
  'Red Bandana','Ulle Cap','Banana Hat','Wing Of Eagle','Stop Post','Wig',
  'Galapago Cap','Monkey Circlet','Munak Turban',
  'Elven Ears','Rune Of Darkness','Mr Scream','Pumpkin Head',
  "Boy's Naivety",'Puppy Love',
  "Odin's Blessing","Goibne's Helmet",'Sacred Marks',
  'Striped Socks','Red Socks With Holes','Skul Ring',
]);

const footgears = new Set([
  'Sandals','Shoes','Boots','High Fashion Sandals','Chrystal Pumps',
  'Fricco Shoes','Valkyrie Shoes',"Vidar's Boots",
  'Tiger Footskin',"Goibne's Combat Boots",
]);

const garments = new Set([
  'Hood','Muffler','Manteau','Cape Of Ancient Lord','Ragamuffin Cape',
  'Clack Of Servival','Undershirt',"Morpheus's Shawl","Morrigane's Manteau",
  'Valkyrie Manteau',"Vali's Manteau",
  'Pauldron','Shoulder Protection','Wedding Veil','Sway Apron',
  'Bark Shorts','Dragon Wing','Rider Insignia',"Shinobi's Sash",'Bowman Scarf',
  'Old Manteau',"Goibne's Shoulder Arms",
]);

const accessories = new Set([
  'Clip','Earring','Ring','Glove','Rosary','Brooch','Necklace','Belt',
  'Safety Ring','Critical Ring','Gold Ring','Silver Ring','Diamond Ring',
  'Angelic Chain','Spiritual Ring','Lunatic Brooch',
  'Armlet Of Obedience','Armlet Of Prisoner','Cursed Lucky Brooch',
  'Golden Bracelet','Lesser Elemental Ring','Ring Of Flame Lord',
  'Ring Of Resonance','Ring Of Rogue','Orleans Glove','Librarian Glove',
  "Morpheus's Armlet","Morpheus's Ring","Morrigane's Belt",
  "Morrigane's Pendant","Morrigane's Helm","Morpheus's Hood",
  'Cuffs',"Executioner's Mitten",'Mitten Of Presbyter',
  'Thimble Of Archer','Spectacles','Manacles',
  'Orleans Server','Tiger Skin Panties','Star Dust Blade','Western Grace',
  'Flower Ring','Right Epsilon','Spirit Chain','Satanic Chain',
  "Matyr's Flea Guard",
]);

const consumables = new Set([
  'Crimson Vial','Amber Elixir','Golden Salve','Azure Philter','Roasted Haunch',
  'Red Herb','Yellow Herb','Green Herb','Blue Herb','Hinalle',
  'Apple','Banana','Carrot','Grape','Meat','Orange','Strawberry','Sweet Potato',
  'Honey','Orange Juice','Sweet Milk','Rainbow Carrot','Cacao','Unripe Apple',
  'Center Potion','Karvodailnirol','Leaflet Of Hinal',
  'Wing Of Butterfly','Wing Of Fly','Wind Scroll 1 3',
  'Tree Of Archer 1','Tree Of Archer 2','Tree Of Archer 3',
  'White Potion','Blue Potion','Panacea','Royal Jelly',
  'Grape Juice','Apple Juice','Banana Juice','Carrot Juice',
  'Yggdrasilberry','Seed Of Yggdrasil','Leaf Of Yggdrasil',
  'Anodyne','Aloebera','Berserk','Holy Water',
  "Monster's Feed",'Pet Food','Pet Incubator','Fruit Of Mastela',
  'Aloe','Leaflet Of Aloe','Well Baked Cookie','Candy','Chocolate',
  'White Chocolate','Piece Of Cake','Bun','Baked Yam','Rice Ball',
  'Prickly Fruit','Lemon','Nice Sweet Potato','Hard Peach','Ice Cream',
  'Milk','Tantanmen','Prawn','Delicious Fish','Popped Rice','Cheese',
  'White Slim Potion','Yellow Slim Potion','Valhalla Flower',
  'Poison Bottle','Branch Of Dead Tree',
  'Mushroom Of Thief 1','Mushroom Of Thief 2','White Herb','Bitter Herb',
  'Old Blue Box','Old Violet Box','Old Card Album',
  'Gift Box','Gift Box 1','Gift Box 2','Gift Box 3','Gift Box 4',
  'Treasure Box','Pumpkin','Chilli','Tropical Banana',
  'Fire Arrow','Silver Arrow','Crystal Arrow','Stone Arrow',
  'Silence Arrow','Oridecon Arrow','Steel Arrow','Incisive Arrow',
  'Ori Arrow Container','Imma Arrow Container','Steel Arrow Container',
  'Arrow Of Shadow','Arrow Of Wind',
  'Cold Scroll 1 3','Cold Scroll 1 5','Cold Scroll 2 1','Cold Scroll 2 5',
  'Fire Scroll 1 3','Fire Scroll 1 5','Fire Scroll 2 5','Fire Scroll 3 5',
  'Earth Scroll 1 3','Earth Scroll 1 5','Wind Scroll 1 5',
  'Holy Scroll 1 3','Holy Scroll 1 5','Ghost Scroll 1 3','Ghost Scroll 1 5',
  'New Year Rice Cake 1','New Year Rice Cake 2','Candy Striper',
  'Rent Scroll','Rent Spell Book',
  'Milk Bottle','Boody Red','Red Potion',
]);

// ── Categorize all items ──
const categories = {
  weapons: [],
  armor: [],
  shields: [],
  headgear: [],
  footgear: [],
  garments: [],
  accessories: [],
  consumables: [],
  cards: [],
  etc_crafting: [],
};

const allItems = [...allDropNames].sort();

for (const name of allItems) {
  const impl = implementedItems[name];
  const isCard = name.endsWith(' Card');

  let desc = impl ? impl.description : 'No description available';
  let type = 'etc';

  if (isCard) {
    type = 'card';
    categories.cards.push({ name, type, desc });
  } else if (weapons.has(name)) {
    type = 'weapon';
    categories.weapons.push({ name, type, desc });
  } else if (armors.has(name)) {
    type = 'armor';
    categories.armor.push({ name, type, desc });
  } else if (shields.has(name)) {
    type = 'shield';
    categories.shields.push({ name, type, desc });
  } else if (headgears.has(name)) {
    type = 'headgear';
    categories.headgear.push({ name, type, desc });
  } else if (footgears.has(name)) {
    type = 'footgear';
    categories.footgear.push({ name, type, desc });
  } else if (garments.has(name)) {
    type = 'garment';
    categories.garments.push({ name, type, desc });
  } else if (accessories.has(name)) {
    type = 'accessory';
    categories.accessories.push({ name, type, desc });
  } else if (consumables.has(name)) {
    type = 'consumable';
    categories.consumables.push({ name, type, desc });
  } else {
    type = 'etc';
    categories.etc_crafting.push({ name, type, desc });
  }
}

// ── Write .md files ──
const outDir = path.join(__dirname, '..', 'docsNew', 'items');
if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

const fileMap = {
  weapons: 'weapons.md',
  armor: 'armor.md',
  shields: 'shields.md',
  headgear: 'headgear.md',
  footgear: 'footgear.md',
  garments: 'garments.md',
  accessories: 'accessories.md',
  consumables: 'consumables.md',
  cards: 'cards.md',
  etc_crafting: 'etc_crafting.md',
};

const titleMap = {
  weapons: 'Weapons',
  armor: 'Armor',
  shields: 'Shields',
  headgear: 'Headgear',
  footgear: 'Footgear',
  garments: 'Garments',
  accessories: 'Accessories',
  consumables: 'Consumables',
  cards: 'Cards',
  etc_crafting: 'Etc / Crafting Materials',
};

for (const [cat, items] of Object.entries(categories)) {
  const fileName = fileMap[cat];
  const title = titleMap[cat];
  const implemented = items.filter(i => implementedItems[i.name]);
  const unimplemented = items.filter(i => !implementedItems[i.name]);

  let content = `# ${title}\n\n`;
  content += `Total: ${items.length} (${implemented.length} implemented, ${unimplemented.length} not yet in DB)\n\n`;
  content += `Format: item_name -> item_type -> description\n\n`;

  if (implemented.length > 0) {
    content += `## Implemented\n\n`;
    for (const item of implemented) {
      content += `${item.name} -> ${item.type} -> ${item.desc}\n`;
    }
    content += `\n`;
  }

  if (unimplemented.length > 0) {
    content += `## Not Yet Implemented\n\n`;
    for (const item of unimplemented) {
      content += `${item.name} -> ${item.type} -> ${item.desc}\n`;
    }
    content += `\n`;
  }

  fs.writeFileSync(path.join(outDir, fileName), content, 'utf8');
  console.log(`Wrote ${fileName}: ${items.length} items (${implemented.length} implemented, ${unimplemented.length} unimplemented)`);
}

console.log(`\nTotal items across all categories: ${allItems.length}`);
console.log(`Output directory: ${outDir}`);
