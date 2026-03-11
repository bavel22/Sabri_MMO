/**
 * Generate item list .md files by category — v2 with specific subtypes
 * e.g. "Ahlspiess -> Two-Handed Spear" instead of just "weapon"
 */
const fs = require('fs');
const path = require('path');

const { RO_MONSTER_TEMPLATES } = require('../server/src/ro_monster_templates.js');
const { RO_ITEM_NAME_TO_ID } = require('../server/src/ro_item_mapping.js');

// ── Parse init.sql for implemented items with descriptions ──
const initSql = fs.readFileSync(path.join(__dirname, '..', 'database', 'init.sql'), 'utf8');
const implementedItems = {};
const insertRegex = /\((\d+),\s*'([^']*(?:''[^']*)*)',\s*'([^']*(?:''[^']*)*)',\s*'([^']*(?:''[^']*)*)'/g;
let match;
while ((match = insertRegex.exec(initSql)) !== null) {
  const id = parseInt(match[1]);
  const name = match[2].replace(/''/g, "'");
  const desc = match[3].replace(/''/g, "'");
  implementedItems[name] = { id, name, description: desc };
}

// ── Collect all item names ──
const allDropNames = new Set();
for (const [key, mon] of Object.entries(RO_MONSTER_TEMPLATES)) {
  if (Array.isArray(mon.drops)) for (const d of mon.drops) allDropNames.add(d.itemName);
  if (Array.isArray(mon.mvpDrops)) for (const d of mon.mvpDrops) allDropNames.add(d.itemName);
}
for (const name of Object.keys(implementedItems)) allDropNames.add(name);
for (const name of Object.keys(RO_ITEM_NAME_TO_ID)) allDropNames.add(name);

// ══════════════════════════════════════════════════════════════
// MASTER TYPE MAP — every item -> specific subtype
// ══════════════════════════════════════════════════════════════

const typeMap = {};

// ── DAGGERS ──
for (const n of [
  'Knife','Cutter','Main Gauche','Dirk','Stiletto','Gladius','Damascus',
  'Combat Knife','Counter Dagger','Cursed Dagger','Poison Knife',
  'Cinquedea','Infiltrator','Grimtooth','Sucsamad','Katzbalger','Krasnaya',
  'Stiletto Fang','Rustic Shiv','Keen Edge','Scalpel','Khukri',
  'Assassin Dagger','Assasin Dagger','Bazerald','Mama\'s Knife',
  'Coward','Silver Knife Of Chaste','Sword Accessory',
]) typeMap[n] = 'Dagger';

// ── ONE-HANDED SWORDS ──
for (const n of [
  'Sword','Blade','Saber','Falchion','Scimiter','Cutlas','Tsurugi',
  'Ring Pommel Saber','Schweizersabel','Hae Dong Gum','Town Sword',
  'Broad Sword','Edge','Nagan','Immaterial Sword','Solar Sword',
  'Moonlight Sword','Iron Cleaver','Crescent Saber','Orcish Sword',
  'Curved Sword','Forturn Sword','Bloody Edge','Lapier','Jitte',
  'Blade Of Pinwheel','Fright Paper Blade','Ice Falchon','Flame Brand',
  'Fire Brand','Altas Weapon','Sword Of Grave Keeper',
]) typeMap[n] = 'One-Handed Sword';

// ── TWO-HANDED SWORDS ──
for (const n of [
  'Slayer','Bastard Sword','Two Hand Sword','Claymore','Zweihander',
  'Mysteltainn','Dragon Slayer','Masamune','Muramasa','Katana',
  'Blade Lost In Darkness','Executioner','Murasame',
]) typeMap[n] = 'Two-Handed Sword';

// ── ONE-HANDED SPEARS ──
for (const n of [
  'Javelin','Spear','Pike','Brionac','Shine Spear Blade',
]) typeMap[n] = 'One-Handed Spear';

// ── TWO-HANDED SPEARS ──
for (const n of [
  'Lance','Halberd','Partizan','Trident','Glaive','Guisarme',
  'Gae Bolg','Ahlspiess','Spectral Spear','Pole Axe',
  'Longinus\'s Spear','Berdysz','Crescent Scythe',
]) typeMap[n] = 'Two-Handed Spear';

// ── ONE-HANDED AXES ──
for (const n of [
  'Axe','Orcish Axe','Vecer Axe','War Axe','Cleaver','Rusty Cleaver',
]) typeMap[n] = 'One-Handed Axe';

// ── TWO-HANDED AXES ──
for (const n of [
  'Battle Axe','Buster','Two Handed Axe','Brood Axe','Great Axe',
  'Doom Slayer','Tomahawk','Giant Axe',
]) typeMap[n] = 'Two-Handed Axe';

// ── MACES ──
for (const n of [
  'Mace','Club','Flail','Morning Star','Sword Mace','Long Mace',
  'Hammer','Hammer Of Blacksmith','Golden Mace','Grand Cross',
  'Iron Cane','Iron Driver','Spike','Stunner','Slash',
]) typeMap[n] = 'Mace';

// ── STAVES / RODS ──
for (const n of [
  'Rod','Wand','Staff','Arc Wand','Mighty Staff','Healing Staff',
  'Blessed Wand','Bone Wand','Wizardy Staff','Dea Staff','Croce Staff',
  'La\'cryma Stick','Piercing Staff','Walking Stick',
  'Staff Of Bordeaux','Staff Of Soul','Staff Of Wing',
  'Survival Rod','Survival Rod2','Hypnotist\'s Staff','Azoth',
]) typeMap[n] = 'Staff';

// ── BOWS ──
for (const n of [
  'Bow','Arbalest','CrossBow','Composite Bow','Gust Bow','Burning Bow',
  'Frozen Bow','Earth Bow','Luna Bow','Great Bow','Balistar',
  'Hunting Longbow','Hurricane Fury','Orc Archer Bow','Bow Of Rudra',
  'Windhawk',
]) typeMap[n] = 'Bow';

// ── KATARS ──
for (const n of [
  'Katar','Jur','Jamadhar','Various Jur','Drill Katar','Cakram',
  'Katar Of Cold Icicle','Katar Of Piercing Wind','Katar Of Raging Blaze',
  'Katar Of Thornbush','Asura','Sabbath','Blood Tears','Bloody Roar',
]) typeMap[n] = 'Katar';

// ── KNUCKLE / FIST WEAPONS ──
for (const n of [
  'Waghnakh','Knuckle Duster','Combo Battle Glove','Electric Fist',
  'Seismic Fist','Icicle Fist','Magma Fist','Claw','Finger',
  'Hora','Kaiser Knuckle',
]) typeMap[n] = 'Knuckle';

// ── MUSICAL INSTRUMENTS ──
for (const n of [
  'Guitar','Guitar Of Blue Solo','Guitar Of Gentle Breeze','Guitar Of Passion',
  'Guitar Of Vast Land','Base Guitar','Berserk Guitar',
  'Lute','Mandolin','Harp','Oriental Lute','Violin','Musika',
]) typeMap[n] = 'Musical Instrument';

// ── WHIPS ──
for (const n of [
  'Whip Of Earth','Whip Of Ice Piece','Whip Of Red Flame',
  'Lariat','Chemeti','Carrot Whip','Bladed Whip','Jump Rope',
  'Short Daenggie','Danggie','Rante','Rope','Fan',
  'Queen\'s Whip',
]) typeMap[n] = 'Whip';

// ── BOOKS (Sage weapons) ──
for (const n of [
  'Book','Bible','Book Of Billows','Book Of Blazing Sun','Book Of Devil',
  'Book Of Gust Of Wind','Book Of Mother Earth','Book Of The Apocalypse',
  'Diary Of Great Sage','Encyclopedia','Memorize Book',
]) typeMap[n] = 'Book';

// ── HUUMA SHURIKEN (Ninja weapons) ──
for (const n of [
  'Huuma Bird Wing','Huuma Blaze','Huuma Calm Mind','Huuma Giant Wheel',
]) typeMap[n] = 'Huuma Shuriken';

// ── GUNS (Gunslinger weapons) ──
for (const n of [
  'Six Shooter','Gate Keeper','Gate KeeperDD','Crimson Bolt','Drifter',
  'The Garrison','The Cyclone',
]) typeMap[n] = 'Revolver';
for (const n of [
  'Air Rifle','Long Barrel','Destroyer',
]) typeMap[n] = 'Rifle';
for (const n of [
  'Muscle Cutter',
]) typeMap[n] = 'Shotgun';

// ── SPECIAL / UNIQUE WEAPONS ──
for (const n of [
  'Heart Breaker','Dragon Killer','Guillotine','Ginnungagap',
  'Zephyrus','Hell Fire','Krishna','Kronos','Krieg','Quadrille',
  'Kakkung','Kandura','Iron Fist',
]) typeMap[n] = 'Special Weapon';

// ══════════════════════════════════════════════════════════════
// ARMOR (Body)
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Linen Tunic','Quilted Vest','Ringweave Hauberk','Silk Robe',
  'Cotton Shirt','Padded Armor','Chain Mail','Full Plate Armor','Coat',
  'Silver Robe','Saint Robe','Mage Coat','Holy Robe','Glittering Clothes',
  'Formal Suit','Ninja Suit','Adventure Suit','Plate Armor','Meteo Plate Armor',
  'Taegeuk Plate','Sniping Suit','Thief Clothes','Dullahan Armor',
  'Clothes Of The Lord','Flame Sprits Armor','Water Sprits Armor',
  'Wind Sprits Armor','Earth Sprits Armor','Celestial Robe','Divine Cloth',
  'Limpid Celestial Robe','Wooden Mail','Falcon Robe','Robe Of Casting',
  'Leaf Clothes','Valkyrie Armor','Leather Jacket','G Strings',
  'Wedding Dress','Worn Out Prison Uniform','Old Japaness Clothes',
  "Adventurere's Suit","Goibne's Armor",'Ulfhedinn','Tights',
  'Scapulare','Tatters Clothes',
]) typeMap[n] = 'Body Armor';

// ══════════════════════════════════════════════════════════════
// SHIELDS
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Guard','Buckler','Shield','Mirror Shield','Stone Buckler',
  'Strong Shield','Platinum Shield','Thorny Buckler','Holy Guard',
  'Herald Of GOD',
]) typeMap[n] = 'Shield';

// ══════════════════════════════════════════════════════════════
// HEADGEAR — Upper
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Hat','Cap','Helm','Biretta','Crown','Tiara','Circlet','Bandana',
  'Beret','Fedora','Corsair','Bone Helm','Gemmed Crown','Gemmed Sallet',
  'Coronet','Fillet','Hair Band','Hair Protector','Fin Helm','Poo Poo Hat',
  'Pumpkin Head','Spinx Helm','Safety Helmet','Nurse Cap','Ph.D Hat',
  'Magestic Goat',"Big Sis' Ribbon",'Pirate Bandana','Panda Cap',
  'Bongun Hat','Viking Helm','Super Novice Hat','Hat Of Cake',
  'Suspicious Hat','Cone Hat','Loard Circlet','Blue Coif',
  'Joker Jester','Ulle Cap','Banana Hat','Galapago Cap',
  'Monkey Circlet','Munak Turban','Fricca Circlet','Magni Cap',
  "Odin's Blessing","Goibne's Helmet",'Sacred Masque','Wig',
  'Wing Of Eagle','Poring Hat','Sacred Marks','Indian Hair Piece',
  'Assassin Mask',"Santa's Hat",'Pumpkin Bucket',"Boy's Naivety",
  'Puppy Love','Mr Scream','Stop Post','Transparent Headgear',
  'Red Bandana','Elven Ears','Cat Hairband','Ribbon','Green Feeler',
  'Fancy Flower','Rune Of Darkness','Magician Hat','White Mask',
]) typeMap[n] = 'Upper Headgear';

// ── HEADGEAR — Mid ──
for (const n of [
  'Binoculars','Gangster Patch','Cigar','Smoking Pipe','Gas Mask',
  'Ghost Bandana','Dark Blindfold','Masquerade','Festival Mask',
  'Goblin Mask 01','Goblin Mask 02','Goblin Mask 03','Goblin Mask 04',
  'Goblini Mask','Black Mask','Ear Of Puppy','Mini Propeller',
  'Nose Ring','Pacifier','Bib','Goggle',
  'Striped Socks','Red Socks With Holes',
]) typeMap[n] = 'Mid Headgear';

// ── HEADGEAR — Lower ──
for (const n of [
  'Pierrot Nose','Horrendous Mouth','Angry Mouth','Ganster Mask',
  'Granpa Beard',
]) typeMap[n] = 'Lower Headgear';

// ══════════════════════════════════════════════════════════════
// FOOTGEAR
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Sandals','Shoes','Boots','High Fashion Sandals','Chrystal Pumps',
  'Fricco Shoes','Valkyrie Shoes',"Vidar's Boots",
  'Tiger Footskin',"Goibne's Combat Boots",
]) typeMap[n] = 'Footgear';

// ══════════════════════════════════════════════════════════════
// GARMENTS
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Hood','Muffler','Manteau','Cape Of Ancient Lord','Ragamuffin Cape',
  'Clack Of Servival','Undershirt',"Morpheus's Shawl","Morrigane's Manteau",
  'Valkyrie Manteau',"Vali's Manteau",'Pauldron','Shoulder Protection',
  'Wedding Veil','Sway Apron','Bark Shorts','Dragon Wing','Rider Insignia',
  "Shinobi's Sash",'Bowman Scarf','Old Manteau',"Goibne's Shoulder Arms",
]) typeMap[n] = 'Garment';

// ══════════════════════════════════════════════════════════════
// ACCESSORIES
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Clip','Earring','Ring','Glove','Rosary','Brooch','Necklace','Belt',
  'Safety Ring','Critical Ring','Gold Ring','Silver Ring','Diamond Ring',
  'Angelic Chain','Spiritual Ring','Lunatic Brooch','Skul Ring',
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
]) typeMap[n] = 'Accessory';

// ══════════════════════════════════════════════════════════════
// CONSUMABLES — Healing
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Crimson Vial','Amber Elixir','Golden Salve','Azure Philter','Roasted Haunch',
  'Red Herb','Yellow Herb','Green Herb','Blue Herb','Hinalle',
  'Apple','Banana','Carrot','Grape','Meat','Orange','Strawberry','Sweet Potato',
  'Honey','Orange Juice','Sweet Milk','Rainbow Carrot','Cacao','Unripe Apple',
  'Center Potion','Karvodailnirol','Leaflet Of Hinal',
  'White Potion','Blue Potion','Panacea','Royal Jelly',
  'Grape Juice','Apple Juice','Banana Juice','Carrot Juice',
  'Yggdrasilberry','Seed Of Yggdrasil','Leaf Of Yggdrasil',
  'Anodyne','Aloebera','Fruit Of Mastela',
  'Aloe','Leaflet Of Aloe','White Herb','Bitter Herb',
  'Well Baked Cookie','Candy','Chocolate','White Chocolate',
  'Piece Of Cake','Bun','Baked Yam','Rice Ball',
  'Prickly Fruit','Lemon','Nice Sweet Potato','Hard Peach','Ice Cream',
  'Milk','Tantanmen','Prawn','Delicious Fish','Popped Rice','Cheese',
  'White Slim Potion','Yellow Slim Potion','Valhalla Flower',
  'Milk Bottle','Boody Red','Red Potion','Chilli','Tropical Banana',
  'Candy Striper','New Year Rice Cake 1','New Year Rice Cake 2',
  'Pumpkin',
]) typeMap[n] = 'Healing Item';

// ── CONSUMABLES — Usable Items ──
for (const n of [
  'Wing Of Butterfly','Wing Of Fly','Berserk',
  'Holy Water','Pet Food','Pet Incubator',"Monster's Feed",
  'Poison Bottle','Branch Of Dead Tree',
  'Mushroom Of Thief 1','Mushroom Of Thief 2',
  'Rent Scroll','Rent Spell Book',
]) typeMap[n] = 'Usable Item';

// ── CONSUMABLES — Ammunition ──
for (const n of [
  'Arrow','Fire Arrow','Silver Arrow','Crystal Arrow','Stone Arrow',
  'Silence Arrow','Oridecon Arrow','Steel Arrow','Incisive Arrow',
  'Arrow Of Shadow','Arrow Of Wind','Poison Arrow',
  'Ori Arrow Container','Imma Arrow Container','Steel Arrow Container',
]) typeMap[n] = 'Ammunition';

// ── CONSUMABLES — Scrolls ──
for (const n of [
  'Wind Scroll 1 3','Wind Scroll 1 5',
  'Cold Scroll 1 3','Cold Scroll 1 5','Cold Scroll 2 1','Cold Scroll 2 5',
  'Fire Scroll 1 3','Fire Scroll 1 5','Fire Scroll 2 5','Fire Scroll 3 5',
  'Earth Scroll 1 3','Earth Scroll 1 5',
  'Holy Scroll 1 3','Holy Scroll 1 5',
  'Ghost Scroll 1 3','Ghost Scroll 1 5',
  'Tree Of Archer 1','Tree Of Archer 2','Tree Of Archer 3',
]) typeMap[n] = 'Scroll';

// ── CONSUMABLES — Boxes / Containers ──
for (const n of [
  'Old Blue Box','Old Violet Box','Old Card Album',
  'Gift Box','Gift Box 1','Gift Box 2','Gift Box 3','Gift Box 4',
  'Treasure Box',
]) typeMap[n] = 'Box / Container';

// ══════════════════════════════════════════════════════════════
// ETC — Gemstones
// ══════════════════════════════════════════════════════════════
for (const n of [
  'Yellow Gemstone','Azure Jewel','Bluish Green Jewel','Cardinal Jewel',
  'White Jewel','Blue Gemstone','Red Gemstone','Gemstone',
  'Blue Jewel','Red Jewel','Dark Red Jewel','Golden Jewel',
  'Skyblue Jewel','Violet Jewel','Scarlet Jewel','Cat Eyed Stone',
  'Crystal Jewel','Agate','Citrine','Biotite','Olivine',
  'Muscovite','Phlogopite','Turquoise','Pyroxene','Rose Quartz',
]) typeMap[n] = 'Gemstone';

// ── ETC — Ores & Crafting Minerals ──
for (const n of [
  'Iron Ore','Iron','Phracon','Oridecon Stone','Oridecon','Elunium',
  'Elunium Stone','Emveretarcon','Steel','Coal','Star Crumb','Star Dust',
  'Rough Wind','Flame Heart','Mistic Frozen','Great Nature',
  'Crystal Blue','Wind Of Verdure','Gold','Gold Lux',
  'Oridecon Hammer','Detrimindexta','Cyfar',
]) typeMap[n] = 'Ore / Crafting Mineral';

// ── ETC — Elemental items ──
for (const n of [
  'Burning Heart','Burning Horse Shoe','Cold Magma','Live Coal',
  'Ice Heart','Ice Piece','Ice Scale','Frozen Heart','Frozen Rose',
  'Flaming Ice','Dragon Breath',
]) typeMap[n] = 'Elemental Material';

// ── ETC — Monster Parts (loot / vendor) ──
for (const n of [
  'Jellopy','Fluff','Shell','Feather','Mushroom Spore','Talon','Tree Root',
  'Tooth Of Bat','Bee Sting',"Grasshopper's Leg",'Skel Bone','Decayed Nail',
  'Sticky Webfoot','Spawn','Yoyo Tail','Raccoon Leaf','Stem',
  'Powder Of Butterfly','Poison Spore','Shoot','Resin',
  'Feather Of Birds','Bill Of Birds',"Animal's Skin",'Flower','Clover',
  'Four Leaf Clover','Chrysalis','Sticky Mucus','Garlet','Empty Bottle',
  'Wooden Block','Gloopy Residue','Viscous Slime','Chitin Shard',
  'Downy Plume','Spore Cluster','Barbed Limb','Verdant Leaf','Silken Tuft',
  'Acorn','Zargon','Yellow Live','Claw Of Desert Wolf','Claw Of Garm',
  'Claw Of Wolves','Claw Of Monkey','Claw Of Rat',
  'Fox Tail','Gill','Ghoul Leg','Ectoplasm','Immortal Heart',
  'Rotten Bandage','Skull','Scorpion\'s Tail','Scropion\'s Nipper',
  'Tentacle','Reptile Tongue','Scale Of Snakes','Snail\'s Shell',
  'Rat Tail','Tail Of Steel Scorpion','Talon Of Griffin','Bear\'s Foot',
  'Wild Boar\'s Mane','Leopard Skin','Leopard Talon',
  'Cobold Hair','Dokkaebi Horn','Orcish Cuspid','Orcish Voucher',
  'Ogre Tooth','Evil Horn','Horn','Horn Of Succubus','Inccubus Horn',
  'Dragon Canine','Dragon Fang','Dragon Horn','Dragon Scale','Dragon\'s Skin',
  'Dragon Train','Fin','Fish Tail','Heart Of Mermaid',
  'Lip Of Ancient Fish','Tooth Of Ancient Fish','Tooth Of',
  'Clam Shell','Flesh Of Clam','Crap Shell','Colorful Shell','Conch',
  'Round Shell','Scales Shell','Glitter Shell','Sea Otter Leather',
  'Sea Witch Foot','Wing Of Moth','Wing Of Red Bat','Moth Dust',
  'Insect Feeler','Limb Of Mantis','Jaws Of Ant','Beetle Nipper','Nipper',
  'Sharp Feeler','Porcupine Spike','Cactus Needle','Worm Peelings',
  'Earthworm Peeling','Earthworm The Dude','Thin N\' Long Tongue',
  'Posionous Canine','Poison Toad\'s Skin','Poisonous Gas','Sticky Poison',
  'Pointed Scale','Sharp Scale','Sharp Leaf','Sharp Gear','Sharpened Cuspid',
  'Solid Peeling','Solid Shell','Solid Twig','Soft Feather','Soft Leaf',
  'Soft Silk Cloth','Tough Scalelike Stem','Tough Vines','Slender Snake',
  'Browny Root','Root Of Maneater','Blossom Of Maneater',
  'Great Leaf','Tree Knot','Thin Stem','Germinating Sprout',
  'Heart Of Tree','Singing Flower','Singing Plant',
  'Black Bear\'s Skin','Tiger\'s Skin','Gaoat\'s Skin',
  'Goat\'s Horn','Snowy Horn','Peco Wing Feather',
  'Blue Feather','Red Feather','Golden Hair','Glossy Hair','Long Hair','Hot Hair',
  'Moustache Of Mole','Nail Of Mole','Short Leg',
  'Harpy\'s Claw','Harpy\'s Feather',
  'Petite DiablOfs Horn','Petite DiablOfs Wing',
  'Anolian Skin','Lizard Scruff','Fang Of Garm',
  'Scale Of Red Dragon','Inverse Scale',
  'Dragon Fly Wing','Single Cell','Spawns',
  'Large Jellopy','Dew Laden Moss','Spiderweb',
  'Broken Shell','Animal Blood','Mud Lump','Sand Lump','Dried Sand',
  'Fluorescent Liquid','Monster Juice','Mould Powder',
  'Cloud Piece','Star Sparkling','Sparkling Dust',
  'An Eye Of Dullahan','Bloody Rune','Bloody Page','Bloody Iron Ball',
  'Evil Mind','Dark Crystal Fragment',
  'Shining Scales','Shining Stone','Glass Bead',
  'Tri Headed Dragon Head','Tendon','Long Limb',
  'Stone','Stone Heart','Stone Piece','Stone Of Intelligence',
  'Grit','Fine Grit','Fine Sand',
  'Nail Of Orc','Nail Of Loki',
  'Broken Armor Piece','Broken Needle','Broken Pharaoh Symbol',
  'Broken Shuriken','Broken Steel Piece','Broken Wine Vessel',
  'Boroken Shiled Piece','Vroken Sword',
  'Burnt Parts','Rust Suriken','Screw','Spanner',
  'Mystery Iron Bit','Mystery Piece','Sturdy Iron Piece','Lusty Iron',
  'Old Broom','Old Frying Pan','Old Hilt','Old Pick',
  'Old Steel Plate','Old Portrait','Old Magic Book','Old Magic Circle',
  'Battered Kettle','Battered Pot','Black Ladle','Spoon Stub','Wheel',
  'Electric Eel','Electric Wire','Bomb Wick','Detonator','Matchstick',
  'Rotten Fish','Rotten Meat','Rotten Rope','Rotten Scale',
  'Scell','Brigan','Cyfar','Yarn','Spool',
  'Silk Ribbon','Red Silk Seal','Green Lace',
  'Piece Of Black Cloth','Transparent Cloth',
  'Candy','Bat Cage',
  'Log','Branch','Bamboo Basket','Bamboo Cut','Burn Tree',
  'Horseshoe','Reins','Chain','Tangled Chain',
  'Contracts In Shadow','Shadow Walk','Tablet',
  'Packing Paper','Packing Ribbon','Smooth Paper','Worn Out Page',
  'Worn Out Scroll','Hardback','Bookclip In Memory',
  'Girl\'s Diary','Exercise','Magic Study Vol1',
  'Egg Shell','Tiny Egg Shell','Piece Of Egg Shell',
  'Fruit Shell','Little Blacky Ghost',
  'Air Pollutant','Thunder P',
  'Alchol','Deadly Noxious Herb','Poison Powder',
  'Dusk','Grave','Mantle','Tube','Starsand Of Witch',
  'Twilight Desert','Sandstorm',
  'Will Of Darkness','Water Of Darkness','Unholy Touch',
  'Piece Of Memory Blue','Piece Of Memory Green',
  'Piece Of Memory Purple','Piece Of Memory Red',
  'Loki\'s Whispers','Kiss Of Angel',
  'Mother\'s Nightmare','Foolishness Of Blind',
  'Eagle Eyes','White Platter','Yellow Plate',
  'Distorted Portrait','Illusion Flower','Witherless Rose','Sunflower',
  'Red Frame','Red Square Bag','Rouge',
  'Ruber','Erde','Izidor',
  'Lantern','Pocket Watch','Needle Of Alarm','Needle Pouch',
  'Chinese Ink','Oil Paper','Flexible String',
  'Skull','Smasher','Butcher','Punisher',
  'Spiky Heel','Skirt Of Virgin','Mink Coat',
  'Cursed Seal','Prohibition Red Candle','Rune Of Darkness',
  'Hanging Doll','Stuffed Doll','Marionette Doll',
  'Baphomet Doll','Poring Doll','Munak Doll',
  'Black Kitty Doll',
  'Icarus Wing','Jubilee','Kamaitachi',
  'Double Bound','Ankle','Byeorrun Gum',
  'Rojerta Piece','Apple Of Archer',
  'Red Muffler','Red Scarf',
  'Pair Of Red Ribbon',
]) typeMap[n] = 'Monster Drop / Loot';

// ── ETC — Cooking / Food Ingredients ──
for (const n of [
  'Cookbook06','Cookbook07','Cookbook08','Cookbook09','Cookbook10',
  'Agi Dish07','Agi Dish10','Dex Dish10','Int Dish10','Vit Dish10',
  'Fantastic Cooking Kits','High end Cooking Kits','Imperial Cooking Kits',
  'Great Chef Orleans01','Legend Of Kafra01',
  'Honey Jar',
]) typeMap[n] = 'Cooking Material';

// ── ETC — Quest / Special Items ──
for (const n of [
  'Key Of Clock Tower','Underground Key','Emperium',
  'Inspector Certificate','Lab Staff Record',
  'Patriotism Marks','Union Of Tribe','Voucher Of Orcish Hero',
  'Piece Of Crest1','Piece Of Crest2','Piece Of Crest3','Piece Of Crest4',
  'Piece Of Bone Armor','Amulet','Armlet Of Prisoner',
  'Legacy Of Dragon','Oldman\'s Romance','Sweet Gents',
  'Dullahan\'s Helm','Worn Out Prison Uniform',
  'Centimental Flower','Centimental Leaf','Ment',
  'Elder Pixie\'s Beard','Tatters Clothes',
  'Fatty Chubby Earthworm','Prawn',
  'Golden Bell','Golden Gear',
]) typeMap[n] = 'Quest Item';

// ── ETC — Refining / Upgrade Materials ──
for (const n of [
  'Transparent Plate01','Transparent Plate02',
  'Transparent Plate03','Transparent Plate04',
  'Vesper Core01','Vesper Core02','Vesper Core03','Vesper Core04',
  'Dragonball Blue','Dragonball Green','Dragonball Red','Dragonball Yellow',
  'Fragment Of Crystal','Crystal Mirror',
  'Darkgreen Dyestuffs','Black Dyestuffs',
]) typeMap[n] = 'Upgrade Material';

// ── ETC — Misc collectibles / dolls ──
for (const n of [
  'Chonchon Doll','Grasshopper Doll','Monkey Doll','Raccoondog Doll','Spore Doll',
  'Meat Dumpling Doll',
]) typeMap[n] = 'Collectible Doll';

// ── ETC — Equipment-like quest items (not equippable but named like equips) ──
for (const n of [
  'Gungnir','Hakujin','Murasame','Battle Hook','Rusty Cleaver',
  'Holy Bonnet','Holy Arrow Quiver','Divine Cross',
  'Longinus\'s Spear','Giant Axe',
  'Young Twig','Booby Trap','Pill',
  'Alice\'s Apron','Mementos','Weihna','Well Dried Bone',
  'Queen\'s Hair Ornament','Turtle Shell','Tengu\'s Nose',
]) typeMap[n] = 'Quest Item';

// ── Fix uncategorized items ──
for (const n of [
  'Angel\'s Arrival','Angel\'s Protection','Angel\'s Safeguard','Angel\'s Warmth',
  'Antenna','Anti Spell Bead','Coral Reef','Great Wing',
  'House Auger','Ora Ora','Penetration',
]) typeMap[n] = 'Monster Drop / Loot';

typeMap['Headlamp'] = 'Upper Headgear';
typeMap['Tutankhamen\'s Mask'] = 'Upper Headgear';
typeMap['Dagger'] = 'Dagger';
typeMap['Skewer'] = 'Dagger';
typeMap['Falken Blitz'] = 'Rifle';
typeMap['Hyper Changer'] = 'Revolver';

// ══════════════════════════════════════════════════════════════
// CARDS — all cards get "Monster Card"
// ══════════════════════════════════════════════════════════════
// (auto-detected by " Card" suffix, no manual mapping needed)

// ══════════════════════════════════════════════════════════════
// CATEGORIZE & WRITE FILES
// ══════════════════════════════════════════════════════════════

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

const weaponTypes = new Set([
  'Dagger','One-Handed Sword','Two-Handed Sword','One-Handed Spear','Two-Handed Spear',
  'One-Handed Axe','Two-Handed Axe','Mace','Staff','Bow','Katar','Knuckle',
  'Musical Instrument','Whip','Book','Huuma Shuriken','Revolver','Rifle','Shotgun',
  'Special Weapon',
]);
const armorTypes = new Set(['Body Armor']);
const shieldTypes = new Set(['Shield']);
const headTypes = new Set(['Upper Headgear','Mid Headgear','Lower Headgear']);
const footTypes = new Set(['Footgear']);
const garmentTypes = new Set(['Garment']);
const accTypes = new Set(['Accessory']);
const consumTypes = new Set(['Healing Item','Usable Item','Ammunition','Scroll','Box / Container']);

const allItems = [...allDropNames].sort();

for (const name of allItems) {
  const impl = implementedItems[name];
  const desc = impl ? impl.description : 'No description available';
  const isCard = name.endsWith(' Card');

  let specificType = typeMap[name] || null;

  if (isCard) {
    specificType = specificType || 'Monster Card';
    categories.cards.push({ name, type: specificType, desc });
  } else if (specificType && weaponTypes.has(specificType)) {
    categories.weapons.push({ name, type: specificType, desc });
  } else if (specificType && armorTypes.has(specificType)) {
    categories.armor.push({ name, type: specificType, desc });
  } else if (specificType && shieldTypes.has(specificType)) {
    categories.shields.push({ name, type: specificType, desc });
  } else if (specificType && headTypes.has(specificType)) {
    categories.headgear.push({ name, type: specificType, desc });
  } else if (specificType && footTypes.has(specificType)) {
    categories.footgear.push({ name, type: specificType, desc });
  } else if (specificType && garmentTypes.has(specificType)) {
    categories.garments.push({ name, type: specificType, desc });
  } else if (specificType && accTypes.has(specificType)) {
    categories.accessories.push({ name, type: specificType, desc });
  } else if (specificType && consumTypes.has(specificType)) {
    categories.consumables.push({ name, type: specificType, desc });
  } else if (specificType) {
    categories.etc_crafting.push({ name, type: specificType, desc });
  } else {
    // Uncategorized — flag it
    categories.etc_crafting.push({ name, type: 'Uncategorized', desc });
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

let uncategorizedCount = 0;

for (const [cat, items] of Object.entries(categories)) {
  const fileName = fileMap[cat];
  const title = titleMap[cat];
  const implemented = items.filter(i => implementedItems[i.name]);
  const unimplemented = items.filter(i => !implementedItems[i.name]);
  const uncategorized = items.filter(i => i.type === 'Uncategorized');
  uncategorizedCount += uncategorized.length;

  let content = `# ${title}\n\n`;
  content += `Total: ${items.length} (${implemented.length} implemented, ${unimplemented.length} not yet in DB)\n\n`;
  content += `Format: item_name -> specific_type -> description\n\n`;

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
  console.log(`Wrote ${fileName}: ${items.length} items (${implemented.length} impl, ${unimplemented.length} unimpl)`);
}

console.log(`\nTotal items: ${allItems.length}`);
console.log(`Uncategorized items: ${uncategorizedCount}`);
if (uncategorizedCount > 0) {
  const uncats = [];
  for (const [cat, items] of Object.entries(categories)) {
    for (const i of items) {
      if (i.type === 'Uncategorized') uncats.push(i.name);
    }
  }
  console.log('Uncategorized:', uncats.join(', '));
}
