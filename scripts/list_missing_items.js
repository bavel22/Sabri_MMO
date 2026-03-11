const { RO_MONSTER_TEMPLATES } = require('../server/src/ro_monster_templates.js');
const { RO_ITEM_NAME_TO_ID } = require('../server/src/ro_item_mapping.js');

const allDropNames = new Set();
const implementedNames = new Set(Object.keys(RO_ITEM_NAME_TO_ID));

for (const [key, mon] of Object.entries(RO_MONSTER_TEMPLATES)) {
  if (Array.isArray(mon.drops)) for (const d of mon.drops) allDropNames.add(d.itemName);
  if (Array.isArray(mon.mvpDrops)) for (const d of mon.mvpDrops) allDropNames.add(d.itemName);
}

const missing = [...allDropNames].filter(n => !implementedNames.has(n) && !n.endsWith(' Card')).sort();

// Known RO item categories
const weaponKeywords = ['Sword','Blade','Dagger','Axe','Mace','Staff','Wand','Rod','Bow','Spear','Lance','Katar','Jur','Guitar','Whip','Lute','Violin','Harp','Mandolin','Book','Bible','Fist','Knuckle','Huuma','Pistol','Rifle','Revolver','Gatling','Shotgun','Grenade'];
const armorKeywords = ['Armor','Mail','Plate','Robe','Coat','Suit','Clothes','Jacket','Shirt'];
const shieldKeywords = ['Shield','Buckler','Guard'];
const headKeywords = ['Hat','Helm','Cap','Crown','Tiara','Circlet','Bonnet','Mask','Band','Beret','Fedora','Goggles','Ribbon'];
const footKeywords = ['Sandals','Shoes','Boots','Greave'];
const garmentKeywords = ['Hood','Muffler','Manteau','Cape','Cloak'];
const accKeywords = ['Ring','Earring','Clip','Necklace','Brooch','Rosary','Bracelet','Glove','Belt'];

const weapons = new Set([
  'Knife','Dagger','Damascus','Dirk','Stiletto','Gladius','Main Gauche','Falchion',
  'Sword','Blade','Broad Sword','Tsurugi','Bastard Sword','Two Hand Sword','Claymore','Zweihander',
  'Slayer','Katzbalger','Mysteltainn','Moonlight Sword','Solar Sword','Immaterial Sword',
  'Saber','Cutlas','Curved Sword','Scimiter','Ring Pommel Saber','Schweizersabel',
  'Katana','Muramasa','Masamune','Hae Dong Gum','Town Sword','Forturn Sword',
  'Mace','Sword Mace','Long Mace','Morning Star','Flail','Golden Mace','Grand Cross',
  'Staff','Arc Wand','Mighty Staff','Wizardy Staff','Healing Staff','Blessed Wand',
  'Bone Wand','Dea Staff','Croce Staff','Piercing Staff','Walking Stick',
  'Staff Of Bordeaux','Staff Of Soul','Staff Of Wing','Survival Rod','Survival Rod2',
  'Rod','Wand','Bow','Arbalest','CrossBow','Composite Bow','Gust Bow','Burning Bow',
  'Frozen Bow','Earth Bow','Luna Bow','Great Bow','Balistar',
  'Spear','Javelin','Pike','Lance','Halberd','Partizan','Trident','Glaive','Guisarme',
  'Pole Axe','Brionac','Gae Bolg','Ahlspiess','Spectral Spear',
  'Axe','Battle Axe','Buster','Two Handed Axe','Brood Axe','Great Axe','Doom Slayer',
  'Orcish Axe','Vecer Axe','Tomahawk','War Axe','Orcish Sword',
  'Club','Hammer','Hammer Of Blacksmith',
  'Katar','Jur','Jamadhar','Various Jur','Drill Katar','Cakram','Waghnakh',
  'Katar Of Cold Icicle','Katar Of Piercing Wind','Katar Of Raging Blaze','Katar Of Thornbush',
  'Combo Battle Glove','Electric Fist','Seismic Fist','Icicle Fist','Magma Fist',
  'Guitar','Guitar Of Blue Solo','Guitar Of Gentle Breeze','Guitar Of Passion','Base Guitar','Berserk Guitar',
  'Lute','Mandolin','Harp','Oriental Lute','Violin',
  'Whip','Lariat','Chemeti','Carrot Whip','Bladed Whip','Jump Rope',
  'Whip Of Ice Piece','Whip Of Red Flame',
  'Huuma Bird Wing','Huuma Blaze','Huuma Calm Mind','Huuma Giant Wheel',
  'Book','Bible','Book Of Billows','Book Of Blazing Sun','Book Of Devil',
  'Book Of Gust Of Wind','Book Of Mother Earth','Book Of The Apocalypse',
  'Diary Of Great Sage','Encyclopedia','Memorize Book',
  'Six Shooter','Gate Keeper','Gate KeeperDD','Air Rifle','Destroyer',
  'Long Barrel','Crimson Bolt','Drifter','The Garrison','The Cyclone',
  'Muscle Cutter','Sabbath','Edge','Nagan','Blood Tears','Bloody Roar',
  'Sucsamad','Dragon Slayer','Krasnaya','Altas Weapon','Dragon Killer',
  'Heart Breaker','Grimtooth','Cinquedea','Asura','Guillotine','Spike',
  'Infiltrator','Combat Knife','Cursed Dagger','Counter Dagger','Poison Knife',
  'Assassin Dagger','Scalpel','Skewer','Iron Cane','Iron Driver','Iron Fist',
  'Lapier','Rapier','Krieg','Krishna','Kronos','Quadrille',
  'Flame Brand','Ice Falchon','Wind Hawk','Windhawk',
  'Orc Archer Bow','Bow Of Rudra','Hurricane Fury',
  'Trident','Brionac','Gae Bolg','Longinus Spear',
  'Bloody Edge','Blade Lost In Darkness','Blade Of Pinwheel',
  'Fright Paper Blade','Shine Spear Blade',
  'Short Daenggie','Danggie','Rante','Rope','Fan',
  'Crescent Scythe','Hell Fire','Zephyrus','Ginnungagap',
  'Jitte','Khukri','Kandura','Kakkung','Hora',
  'Azoth','Stunner','Veteran Hammer',
]);

const armors = new Set([
  'Cotton Shirt','Padded Armor','Chain Mail','Full Plate Armor','Coat','Silk Robe',
  'Silver Robe','Saint Robe','Mage Coat','Holy Robe','Glittering Clothes','Formal Suit',
  'Ninja Suit','Adventure Suit','Plate Armor','Meteo Plate Armor',
  'Taegeuk Plate','Sniping Suit','Thief Clothes','Dullahan Armor','Clothes Of The Lord',
  'Flame Sprits Armor','Water Sprits Armor','Wind Sprits Armor','Earth Sprits Armor',
  'Celestial Robe','Divine Cloth','Limpid Celestial Robe','Wooden Mail','Falcon Robe',
  'Robe Of Casting','Leaf Clothes','Valkyrie Armor','Leather Jacket',
  'G Strings','Wedding Dress','Worn Out Prison Uniform','Old Japaness Clothes',
  'Adventurere\'s Suit',
]);

const shields = new Set([
  'Guard','Buckler','Shield','Mirror Shield','Stone Buckler','Strong Shield',
  'Platinum Shield','Thorny Buckler','Holy Guard','Herald Of GOD',
]);

const headgears = new Set([
  'Hat','Cap','Helm','Biretta','Crown','Tiara','Goggle','Circlet','Bandana',
  'Beret','Fedora','Headlamp','Corsair','Bone Helm','Gemmed Crown','Gemmed Sallet',
  'Coronet','Fillet','Hair Band','Hair Protector','Fin Helm','Poo Poo Hat',
  'Pumpkin Head','Spinx Helm','Safety Helmet','Nurse Cap','Ph.D Hat',
  'Magestic Goat','Big Sis\' Ribbon','Ear Of Puppy','Pirate Bandana','Panda Cap',
  'Bongun Hat','Viking Helm','Super Novice Hat','Hat Of Cake','Suspicious Hat',
  'Cone Hat','Angry Mouth','Tutankhamen\'s Mask','Masquerade','Loard Circlet',
  'Blue Coif','Binoculars','Cigar','Smoking Pipe','Gangster Patch','Ganster Mask',
  'Gas Mask','Granpa Beard','Ghost Bandana','Dark Blindfold','Festival Mask',
  'Santa\'s Hat','Pumpkin Bucket','Bib','Poring Hat','Assassin Mask',
  'Indian Hair Piece','Pacifier','Mini Propeller',
  'Goblin Mask 01','Goblin Mask 02','Goblin Mask 03','Goblin Mask 04','Goblini Mask',
  'Nose Ring','Sacred Masque','Fricca Circlet','Magni Cap',
  'Joker Jester','Transparent Headgear','White Mask','Black Mask',
  'Red Bandana','Ulle Cap','Banana Hat','Wing Of Eagle','Stop Post','Wig',
  'Galapago Cap','Monkey Circlet','Munak Turban',
  'Elven Ears','Candle','Rune Of Darkness','Mr Scream','Pumpkin Head',
  'Welding Mask','Boy\'s Naivety','Puppy Love','Ribbon',
  'Odin\'s Blessing','Goibne\'s Helmet','Sacred Marks',
  'Striped Socks','Red Socks With Holes',
]);

const footgears = new Set([
  'Sandals','Shoes','Boots','High Fashion Sandals','Chrystal Pumps',
  'Fricco Shoes','Valkyrie Shoes','Vidar\'s Boots',
  'Tiger Footskin','Goibne\'s Combat Boots',
]);

const garments = new Set([
  'Hood','Muffler','Manteau','Cape Of Ancient Lord','Ragamuffin Cape',
  'Clack Of Servival','Undershirt','Morpheus\'s Shawl','Morrigane\'s Manteau',
  'Valkyrie Manteau','Vali\'s Manteau',
  'Pauldron','Shoulder Protection','Wedding Veil','Sway Apron',
  'Bark Shorts','Dragon Wing','Rider Insignia','Shinobi\'s Sash','Bowman Scarf',
  'Old Manteau','Goibne\'s Shoulder Arms',
]);

const accessories = new Set([
  'Clip','Earring','Ring','Glove','Rosary','Brooch','Necklace','Belt',
  'Safety Ring','Critical Ring','Gold Ring','Silver Ring','Diamond Ring',
  'Angelic Chain','Spiritual Ring','Lunatic Brooch',
  'Armlet Of Obedience','Armlet Of Prisoner','Cursed Lucky Brooch',
  'Golden Bracelet','Lesser Elemental Ring','Ring Of Flame Lord',
  'Ring Of Resonance','Ring Of Rogue','Orleans Glove','Librarian Glove',
  'Morpheus\'s Armlet','Morpheus\'s Ring','Morrigane\'s Belt',
  'Morrigane\'s Pendant','Morrigane\'s Helm','Morpheus\'s Hood',
  'Cuffs','Executioner\'s Mitten','Mitten Of Presbyter',
  'Thimble Of Archer','Spectacles','Manacles',
  'Orleans Server','Tiger Skin Panties','Star Dust Blade','Western Grace',
  'Flower Ring','Right Epsilon','Spirit Chain','Satanic Chain',
  'Matyr\'s Flea Guard',
]);

const consumables = new Set([
  'White Potion','Blue Potion','Panacea','Royal Jelly',
  'Grape Juice','Apple Juice','Banana Juice','Carrot Juice',
  'Yggdrasilberry','Seed Of Yggdrasil','Leaf Of Yggdrasil',
  'Anodyne','Aloebera','Berserk','Holy Water',
  'Monster\'s Feed','Pet Food','Pet Incubator','Fruit Of Mastela',
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
  'Rent Scroll','Rent Spell Book','Speed Up Potion','Concentration Potion',
  'Awakening Potion','Token Of Siegfried',
  'Milk Bottle','Boody Red','Red Potion',
]);

const results = { Weapons: [], Armor: [], Shields: [], Headgear: [], Footgear: [], Garments: [], Accessories: [], Consumables: [], 'Etc/Crafting': [] };

for (const name of missing) {
  if (weapons.has(name)) results.Weapons.push(name);
  else if (armors.has(name)) results.Armor.push(name);
  else if (shields.has(name)) results.Shields.push(name);
  else if (headgears.has(name)) results.Headgear.push(name);
  else if (footgears.has(name)) results.Footgear.push(name);
  else if (garments.has(name)) results.Garments.push(name);
  else if (accessories.has(name)) results.Accessories.push(name);
  else if (consumables.has(name)) results.Consumables.push(name);
  else results['Etc/Crafting'].push(name);
}

for (const [cat, items] of Object.entries(results)) {
  if (items.length) console.log(cat + ' (' + items.length + '): ' + items.join(', '));
  console.log('');
}
console.log('TOTAL: ' + missing.length);
