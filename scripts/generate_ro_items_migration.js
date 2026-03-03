/**
 * Generate SQL migration for all RO drop items from zone 1-3 monsters
 * Also generates an item name → ID mapping for the server
 * 
 * Usage: node scripts/generate_ro_items_migration.js
 * Output: database/migrations/add_ro_drop_items.sql
 */
const fs = require('fs');
const path = require('path');

// ============================================================
// All RO items from zone 1-3 monster drops, categorized
// ID scheme: 1xxx=consumable, 2xxx=etc, 3xxx=weapon, 4xxx=armor/equip, 5xxx=card
// ============================================================

const ALL_ITEMS = [
    // ── CONSUMABLES (1006-1033) ──────────────────────────────────
    // Herbs
    { id: 1006, name: 'Red Herb', desc: 'A small red medicinal herb. Restores 18 HP.', type: 'consumable', weight: 3, price: 18, stackable: true, maxStack: 99, icon: 'red_herb' },
    { id: 1007, name: 'Yellow Herb', desc: 'A yellow medicinal herb. Restores 38 HP.', type: 'consumable', weight: 5, price: 25, stackable: true, maxStack: 99, icon: 'yellow_herb' },
    { id: 1008, name: 'Green Herb', desc: 'A green herb that cures poison.', type: 'consumable', weight: 3, price: 10, stackable: true, maxStack: 99, icon: 'green_herb' },
    { id: 1009, name: 'Blue Herb', desc: 'A blue medicinal herb. Restores 15 SP.', type: 'consumable', weight: 5, price: 30, stackable: true, maxStack: 99, icon: 'blue_herb' },
    { id: 1010, name: 'Hinalle', desc: 'A rare herb with strong healing properties.', type: 'consumable', weight: 5, price: 50, stackable: true, maxStack: 99, icon: 'hinalle' },
    // Food
    { id: 1011, name: 'Apple', desc: 'A fresh, juicy apple. Restores 16 HP.', type: 'consumable', weight: 2, price: 7, stackable: true, maxStack: 99, icon: 'apple' },
    { id: 1012, name: 'Banana', desc: 'A ripe banana. Restores 17 HP.', type: 'consumable', weight: 2, price: 15, stackable: true, maxStack: 99, icon: 'banana' },
    { id: 1013, name: 'Carrot', desc: 'A fresh orange carrot. Restores 18 HP.', type: 'consumable', weight: 2, price: 5, stackable: true, maxStack: 99, icon: 'carrot' },
    { id: 1014, name: 'Grape', desc: 'A cluster of sweet grapes. Restores 25 SP.', type: 'consumable', weight: 2, price: 100, stackable: true, maxStack: 99, icon: 'grape' },
    { id: 1015, name: 'Meat', desc: 'A slab of fresh meat. Restores 70 HP.', type: 'consumable', weight: 15, price: 25, stackable: true, maxStack: 99, icon: 'meat' },
    { id: 1016, name: 'Orange', desc: 'A sweet citrus fruit. Restores 19 HP.', type: 'consumable', weight: 2, price: 10, stackable: true, maxStack: 99, icon: 'orange' },
    { id: 1017, name: 'Strawberry', desc: 'A ripe strawberry. Restores 16 SP.', type: 'consumable', weight: 2, price: 15, stackable: true, maxStack: 99, icon: 'strawberry' },
    { id: 1018, name: 'Sweet Potato', desc: 'A baked sweet potato. Restores 25 HP.', type: 'consumable', weight: 10, price: 20, stackable: true, maxStack: 99, icon: 'sweet_potato' },
    { id: 1019, name: 'Honey', desc: 'Sweet golden honey. Restores 70 HP and 20 SP.', type: 'consumable', weight: 10, price: 100, stackable: true, maxStack: 99, icon: 'honey' },
    { id: 1020, name: 'Orange Juice', desc: 'Freshly squeezed orange juice. Restores 25 HP.', type: 'consumable', weight: 10, price: 75, stackable: true, maxStack: 99, icon: 'orange_juice' },
    { id: 1021, name: 'Sweet Milk', desc: 'Warm sweet milk. Restores 50 HP.', type: 'consumable', weight: 10, price: 60, stackable: true, maxStack: 99, icon: 'sweet_milk' },
    { id: 1022, name: 'Rainbow Carrot', desc: 'A rare multicolored carrot.', type: 'consumable', weight: 2, price: 200, stackable: true, maxStack: 99, icon: 'rainbow_carrot' },
    { id: 1023, name: 'Cacao', desc: 'Raw cacao beans. Restores 10 HP.', type: 'consumable', weight: 2, price: 30, stackable: true, maxStack: 99, icon: 'cacao' },
    { id: 1024, name: 'Unripe Apple', desc: 'A small, sour green apple.', type: 'consumable', weight: 2, price: 5, stackable: true, maxStack: 99, icon: 'unripe_apple' },
    // Potions & Medicine
    { id: 1025, name: 'Center Potion', desc: 'A balanced recovery potion. Restores 150 HP.', type: 'consumable', weight: 7, price: 50, stackable: true, maxStack: 99, icon: 'center_potion' },
    { id: 1026, name: 'Karvodailnirol', desc: 'A bitter antidote herb. Cures all status effects.', type: 'consumable', weight: 7, price: 75, stackable: true, maxStack: 99, icon: 'karvodailnirol' },
    { id: 1027, name: 'Leaflet Of Hinal', desc: 'A healing herb leaf. Restores minor HP.', type: 'consumable', weight: 1, price: 20, stackable: true, maxStack: 99, icon: 'leaflet_of_hinal' },
    // Utility
    { id: 1028, name: 'Wing Of Butterfly', desc: 'Teleports you to your save point.', type: 'consumable', weight: 5, price: 150, stackable: true, maxStack: 99, icon: 'wing_of_butterfly' },
    { id: 1029, name: 'Wing Of Fly', desc: 'Teleports you to a random location on the map.', type: 'consumable', weight: 5, price: 30, stackable: true, maxStack: 99, icon: 'wing_of_fly' },
    { id: 1030, name: 'Wind Scroll 1 3', desc: 'A wind magic scroll. Casts a small wind spell.', type: 'consumable', weight: 1, price: 50, stackable: true, maxStack: 99, icon: 'wind_scroll' },
    // Stat Food (archer skill manuals)
    { id: 1031, name: 'Tree Of Archer 1', desc: 'Archer training manual Vol.1. Increases DEX temporarily.', type: 'consumable', weight: 5, price: 250, stackable: true, maxStack: 99, icon: 'tree_of_archer_1' },
    { id: 1032, name: 'Tree Of Archer 2', desc: 'Archer training manual Vol.2. Increases DEX temporarily.', type: 'consumable', weight: 5, price: 300, stackable: true, maxStack: 99, icon: 'tree_of_archer_2' },
    { id: 1033, name: 'Tree Of Archer 3', desc: 'Archer training manual Vol.3. Increases DEX temporarily.', type: 'consumable', weight: 5, price: 350, stackable: true, maxStack: 99, icon: 'tree_of_archer_3' },

    // ── ETC / LOOT ITEMS (2009-2058) ─────────────────────────────
    // Monster drops (common materials)
    { id: 2009, name: 'Jellopy', desc: 'A small, worthless gelatinous substance from a Poring.', type: 'etc', weight: 1, price: 2, stackable: true, maxStack: 999, icon: 'jellopy' },
    { id: 2010, name: 'Fluff', desc: 'Soft, fluffy material from a Fabre.', type: 'etc', weight: 1, price: 3, stackable: true, maxStack: 999, icon: 'fluff' },
    { id: 2011, name: 'Shell', desc: 'A hard protective shell fragment.', type: 'etc', weight: 2, price: 14, stackable: true, maxStack: 999, icon: 'shell' },
    { id: 2012, name: 'Feather', desc: 'A light, downy feather.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'feather' },
    { id: 2013, name: 'Mushroom Spore', desc: 'Tiny spores released by a mushroom creature.', type: 'etc', weight: 1, price: 10, stackable: true, maxStack: 999, icon: 'mushroom_spore' },
    { id: 2014, name: 'Talon', desc: 'A sharp talon from a bird of prey.', type: 'etc', weight: 1, price: 8, stackable: true, maxStack: 999, icon: 'talon' },
    { id: 2015, name: 'Tree Root', desc: 'A fibrous tree root segment.', type: 'etc', weight: 1, price: 3, stackable: true, maxStack: 999, icon: 'tree_root' },
    { id: 2016, name: 'Tooth Of Bat', desc: 'A sharp fang from a bat creature.', type: 'etc', weight: 1, price: 10, stackable: true, maxStack: 999, icon: 'tooth_of_bat' },
    { id: 2017, name: 'Bee Sting', desc: 'A venomous stinger from a hornet.', type: 'etc', weight: 1, price: 8, stackable: true, maxStack: 999, icon: 'bee_sting' },
    { id: 2018, name: "Grasshopper's Leg", desc: 'A long, jointed leg from a grasshopper.', type: 'etc', weight: 1, price: 12, stackable: true, maxStack: 999, icon: 'grasshoppers_leg' },
    { id: 2019, name: 'Skel Bone', desc: 'A weathered bone from an undead skeleton.', type: 'etc', weight: 1, price: 15, stackable: true, maxStack: 999, icon: 'skel_bone' },
    { id: 2020, name: 'Decayed Nail', desc: 'A rotting fingernail from a zombie.', type: 'etc', weight: 1, price: 12, stackable: true, maxStack: 999, icon: 'decayed_nail' },
    { id: 2021, name: 'Sticky Webfoot', desc: 'A sticky webbed foot from a frog.', type: 'etc', weight: 1, price: 6, stackable: true, maxStack: 999, icon: 'sticky_webfoot' },
    { id: 2022, name: 'Spawn', desc: 'Frog eggs in a gelatinous mass.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'spawn' },
    { id: 2023, name: 'Yoyo Tail', desc: 'A furry tail from a monkey-like creature.', type: 'etc', weight: 1, price: 8, stackable: true, maxStack: 999, icon: 'yoyo_tail' },
    { id: 2024, name: 'Raccoon Leaf', desc: 'A magical leaf carried by a raccoon.', type: 'etc', weight: 1, price: 10, stackable: true, maxStack: 999, icon: 'raccoon_leaf' },
    { id: 2025, name: 'Stem', desc: 'A thick plant stem from a mandragora.', type: 'etc', weight: 1, price: 3, stackable: true, maxStack: 999, icon: 'stem' },
    { id: 2026, name: 'Powder Of Butterfly', desc: 'Shimmering dust from butterfly wings.', type: 'etc', weight: 1, price: 40, stackable: true, maxStack: 999, icon: 'powder_of_butterfly' },
    { id: 2027, name: 'Poison Spore', desc: 'A toxic mushroom spore.', type: 'etc', weight: 1, price: 7, stackable: true, maxStack: 999, icon: 'poison_spore_item' },
    { id: 2028, name: 'Shoot', desc: 'A young plant shoot from a mandragora.', type: 'etc', weight: 1, price: 3, stackable: true, maxStack: 999, icon: 'shoot' },
    { id: 2029, name: 'Resin', desc: 'Sticky tree sap from a willow.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'resin' },
    { id: 2030, name: 'Feather Of Birds', desc: 'A large flight feather from a condor.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'feather_of_birds' },
    { id: 2031, name: 'Bill Of Birds', desc: 'A hard beak fragment from a bird.', type: 'etc', weight: 1, price: 10, stackable: true, maxStack: 999, icon: 'bill_of_birds' },
    { id: 2032, name: "Animal's Skin", desc: 'Tanned hide from a wild animal.', type: 'etc', weight: 1, price: 18, stackable: true, maxStack: 999, icon: 'animals_skin' },
    { id: 2033, name: 'Flower', desc: 'A beautiful wild flower.', type: 'etc', weight: 1, price: 3, stackable: true, maxStack: 999, icon: 'flower' },
    { id: 2034, name: 'Clover', desc: 'A common three-leaf clover.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'clover' },
    { id: 2035, name: 'Four Leaf Clover', desc: 'An extremely rare lucky clover. Highly valued.', type: 'etc', weight: 1, price: 500, stackable: true, maxStack: 999, icon: 'four_leaf_clover' },
    { id: 2036, name: 'Chrysalis', desc: 'A cocoon shell from an insect larva.', type: 'etc', weight: 1, price: 4, stackable: true, maxStack: 999, icon: 'chrysalis' },
    { id: 2037, name: 'Sticky Mucus', desc: 'Thick, gooey substance from a slime creature.', type: 'etc', weight: 1, price: 7, stackable: true, maxStack: 999, icon: 'sticky_mucus' },
    { id: 2038, name: 'Garlet', desc: 'A small throwing stone used as ammunition.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'garlet' },
    { id: 2039, name: 'Empty Bottle', desc: 'An empty glass bottle. Can be used for brewing.', type: 'etc', weight: 2, price: 3, stackable: true, maxStack: 999, icon: 'empty_bottle' },
    // Ores & Minerals
    { id: 2040, name: 'Iron Ore', desc: 'Unrefined iron ore. Used for smithing.', type: 'etc', weight: 15, price: 25, stackable: true, maxStack: 999, icon: 'iron_ore' },
    { id: 2041, name: 'Iron', desc: 'A refined iron ingot. Used for crafting.', type: 'etc', weight: 30, price: 50, stackable: true, maxStack: 999, icon: 'iron' },
    { id: 2042, name: 'Phracon', desc: 'A mineral used to upgrade level 1 weapons.', type: 'etc', weight: 20, price: 100, stackable: true, maxStack: 999, icon: 'phracon' },
    { id: 2043, name: 'Oridecon Stone', desc: 'A rare ore used in weapon forging. Needs refining.', type: 'etc', weight: 30, price: 500, stackable: true, maxStack: 999, icon: 'oridecon_stone' },
    { id: 2044, name: 'Wind Of Verdure', desc: 'Crystallized wind essence. Used for elemental enchanting.', type: 'etc', weight: 1, price: 75, stackable: true, maxStack: 999, icon: 'wind_of_verdure' },
    // Gemstones
    { id: 2045, name: 'Yellow Gemstone', desc: 'A sparkling yellow gem used in magic.', type: 'etc', weight: 3, price: 300, stackable: true, maxStack: 999, icon: 'yellow_gemstone' },
    { id: 2046, name: 'Azure Jewel', desc: 'A brilliant blue jewel. Extremely valuable.', type: 'etc', weight: 3, price: 3000, stackable: true, maxStack: 999, icon: 'azure_jewel' },
    { id: 2047, name: 'Bluish Green Jewel', desc: 'A rare teal-colored gemstone.', type: 'etc', weight: 3, price: 3000, stackable: true, maxStack: 999, icon: 'bluish_green_jewel' },
    { id: 2048, name: 'Cardinal Jewel', desc: 'A deep red precious stone.', type: 'etc', weight: 3, price: 3000, stackable: true, maxStack: 999, icon: 'cardinal_jewel' },
    { id: 2049, name: 'White Jewel', desc: 'A pure white gemstone of great value.', type: 'etc', weight: 3, price: 3000, stackable: true, maxStack: 999, icon: 'white_jewel' },
    // Misc materials
    { id: 2050, name: 'Zargon', desc: 'A mysterious crystallized substance.', type: 'etc', weight: 5, price: 200, stackable: true, maxStack: 999, icon: 'zargon' },
    { id: 2051, name: 'Yellow Live', desc: 'A golden magical essence.', type: 'etc', weight: 5, price: 100, stackable: true, maxStack: 999, icon: 'yellow_live' },
    { id: 2052, name: 'Wooden Block', desc: 'A small carved wooden block.', type: 'etc', weight: 1, price: 5, stackable: true, maxStack: 999, icon: 'wooden_block' },
    { id: 2053, name: 'Arrow', desc: 'A basic wooden arrow for bows.', type: 'etc', weight: 0, price: 1, stackable: true, maxStack: 999, icon: 'arrow' },
    // Dolls / Taming items
    { id: 2054, name: 'Chonchon Doll', desc: 'A cute doll shaped like a Chonchon.', type: 'etc', weight: 10, price: 200, stackable: true, maxStack: 99, icon: 'chonchon_doll' },
    { id: 2055, name: 'Grasshopper Doll', desc: 'A doll shaped like a grasshopper.', type: 'etc', weight: 10, price: 200, stackable: true, maxStack: 99, icon: 'grasshopper_doll' },
    { id: 2056, name: 'Monkey Doll', desc: 'A plush monkey doll.', type: 'etc', weight: 10, price: 200, stackable: true, maxStack: 99, icon: 'monkey_doll' },
    { id: 2057, name: 'Raccoondog Doll', desc: 'A doll shaped like a raccoon dog.', type: 'etc', weight: 10, price: 200, stackable: true, maxStack: 99, icon: 'raccoondog_doll' },
    { id: 2058, name: 'Spore Doll', desc: 'A doll shaped like a mushroom spore.', type: 'etc', weight: 10, price: 200, stackable: true, maxStack: 99, icon: 'spore_doll' },

    // ── WEAPONS (3007-3020) ──────────────────────────────────────
    { id: 3007, name: 'Knife', desc: 'A basic utility knife.', type: 'weapon', slot: 'weapon', weight: 40, price: 25, atk: 17, reqLv: 1, icon: 'knife_ro', weaponType: 'dagger', aspdMod: 5, weaponRange: 150 },
    { id: 3008, name: 'Cutter', desc: 'A sharp cutting blade.', type: 'weapon', slot: 'weapon', weight: 40, price: 150, atk: 30, reqLv: 1, icon: 'cutter_ro', weaponType: 'dagger', aspdMod: 5, weaponRange: 150 },
    { id: 3009, name: 'Main Gauche', desc: 'A parrying dagger with a wide guard.', type: 'weapon', slot: 'weapon', weight: 60, price: 500, atk: 43, reqLv: 12, icon: 'main_gauche_ro', weaponType: 'dagger', aspdMod: 5, weaponRange: 150 },
    { id: 3010, name: 'Falchion', desc: 'A curved single-edged sword.', type: 'weapon', slot: 'weapon', weight: 60, price: 600, atk: 49, reqLv: 18, icon: 'falchion_ro', weaponType: 'one_hand_sword', aspdMod: 0, weaponRange: 150 },
    { id: 3011, name: 'Mace', desc: 'A sturdy blunt weapon with a heavy head.', type: 'weapon', slot: 'weapon', weight: 80, price: 750, atk: 37, reqLv: 2, icon: 'mace', weaponType: 'mace', aspdMod: -2, weaponRange: 150 },
    { id: 3012, name: 'Rod', desc: 'A basic wooden staff for mages.', type: 'weapon', slot: 'weapon', weight: 40, price: 25, atk: 15, matk: 45, reqLv: 1, icon: 'rod', weaponType: 'staff', aspdMod: -3, weaponRange: 150 },
    { id: 3013, name: 'Bow', desc: 'A basic wooden bow.', type: 'weapon', slot: 'weapon', weight: 50, price: 500, atk: 15, reqLv: 4, icon: 'bow_ro', weaponType: 'bow', aspdMod: -3, weaponRange: 800 },
    { id: 3014, name: 'Javelin', desc: 'A light throwing spear.', type: 'weapon', slot: 'weapon', weight: 70, price: 75, atk: 28, reqLv: 1, icon: 'javelin', weaponType: 'spear', aspdMod: -3, weaponRange: 150 },
    { id: 3015, name: 'Spear', desc: 'A long-reaching polearm weapon.', type: 'weapon', slot: 'weapon', weight: 85, price: 800, atk: 44, reqLv: 4, icon: 'spear', weaponType: 'spear', aspdMod: -3, weaponRange: 150 },
    { id: 3016, name: 'Axe', desc: 'A heavy chopping axe.', type: 'weapon', slot: 'weapon', weight: 80, price: 250, atk: 38, reqLv: 3, icon: 'axe', weaponType: 'axe', aspdMod: -2, weaponRange: 150 },
    { id: 3017, name: 'Club', desc: 'A simple wooden club.', type: 'weapon', slot: 'weapon', weight: 70, price: 60, atk: 23, reqLv: 1, icon: 'club', weaponType: 'mace', aspdMod: -2, weaponRange: 150 },
    { id: 3018, name: 'Wand', desc: 'A magical wand for spellcasting.', type: 'weapon', slot: 'weapon', weight: 40, price: 200, atk: 25, matk: 45, reqLv: 1, icon: 'wand', weaponType: 'staff', aspdMod: -3, weaponRange: 150 },
    { id: 3019, name: 'Guitar Of Vast Land', desc: 'A musical instrument that resonates with earth energy.', type: 'weapon', slot: 'weapon', weight: 70, price: 2000, atk: 50, reqLv: 27, icon: 'guitar_of_vast_land', weaponType: 'instrument', aspdMod: -3, weaponRange: 150 },
    { id: 3020, name: 'Whip Of Earth', desc: 'A whip imbued with earth elemental power.', type: 'weapon', slot: 'weapon', weight: 40, price: 2500, atk: 55, reqLv: 27, icon: 'whip_of_earth', weaponType: 'whip', aspdMod: -3, weaponRange: 150 },

    // ── ARMOR / HEADGEAR / FOOTGEAR / ACCESSORIES (4004-4014) ────
    { id: 4004, name: 'Guard', desc: 'A small wooden shield.', type: 'armor', slot: 'shield', weight: 30, price: 100, def: 3, reqLv: 1, icon: 'guard' },
    { id: 4005, name: 'Hat', desc: 'A simple cloth hat.', type: 'armor', slot: 'head_top', weight: 20, price: 150, def: 2, reqLv: 1, icon: 'hat' },
    { id: 4006, name: 'Sandals', desc: 'Light open-toed footwear.', type: 'armor', slot: 'footgear', weight: 20, price: 125, def: 1, reqLv: 1, icon: 'sandals' },
    { id: 4007, name: 'Silk Robe', desc: 'A fine silk robe offering magical protection.', type: 'armor', slot: 'armor', weight: 40, price: 4000, def: 3, reqLv: 1, icon: 'silk_robe' },
    { id: 4008, name: 'Ribbon', desc: 'A pretty hair ribbon.', type: 'armor', slot: 'head_top', weight: 10, price: 25, def: 1, reqLv: 1, icon: 'ribbon' },
    { id: 4009, name: 'Cat Hairband', desc: 'A cute headband with cat ears.', type: 'armor', slot: 'head_top', weight: 10, price: 500, def: 1, reqLv: 1, icon: 'cat_hairband' },
    { id: 4010, name: 'Skul Ring', desc: 'A ring shaped like a tiny skull.', type: 'armor', slot: 'accessory', weight: 10, price: 300, def: 0, reqLv: 1, icon: 'skul_ring' },
    { id: 4011, name: 'Green Feeler', desc: 'An antenna-like headpiece from an insect.', type: 'armor', slot: 'head_top', weight: 10, price: 500, def: 1, reqLv: 1, icon: 'green_feeler' },
    { id: 4012, name: 'Pierrot Nose', desc: 'A round red clown nose.', type: 'armor', slot: 'head_low', weight: 10, price: 100, def: 0, reqLv: 1, icon: 'pierrot_nose' },
    { id: 4013, name: 'Horrendous Mouth', desc: 'A scary mouth-shaped mask.', type: 'armor', slot: 'head_low', weight: 10, price: 100, def: 0, reqLv: 1, icon: 'horrendous_mouth' },
    { id: 4014, name: 'Fancy Flower', desc: 'A beautiful flower worn as a headpiece.', type: 'armor', slot: 'head_top', weight: 10, price: 1000, def: 1, reqLv: 1, icon: 'fancy_flower' },

    // ── MONSTER CARDS (5001-5023) ────────────────────────────────
    { id: 5001, name: 'Poring Card', desc: 'A card with the image of a Poring. LUK +2, Perfect Dodge +1.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'poring_card' },
    { id: 5002, name: 'Lunatic Card', desc: 'A card with the image of a Lunatic. LUK +1, Critical +1, Flee +1.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'lunatic_card' },
    { id: 5003, name: 'Fabre Card', desc: 'A card with the image of a Fabre. VIT +1, Max HP +100.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'fabre_card' },
    { id: 5004, name: 'Pupa Card', desc: 'A card with the image of a Pupa. Max HP +700.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'pupa_card' },
    { id: 5005, name: 'Drops Card', desc: 'A card with the image of a Drops. DEX +1, HIT +3.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'drops_card' },
    { id: 5006, name: 'Chonchon Card', desc: 'A card with the image of a Chonchon. AGI +1, Flee +2.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'chonchon_card' },
    { id: 5007, name: 'Condor Card', desc: 'A card with the image of a Condor. Flee +5.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'condor_card' },
    { id: 5008, name: 'Wilow Card', desc: 'A card with the image of a Willow. Max SP +80.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'wilow_card' },
    { id: 5009, name: 'Roda Frog Card', desc: 'A card with the image of a Roda Frog. Max HP +400, Max SP +50.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'roda_frog_card' },
    { id: 5010, name: 'Hornet Card', desc: 'A card with the image of a Hornet. STR +1, ATK +3.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'hornet_card' },
    { id: 5011, name: 'Rocker Card', desc: 'A card with the image of a Rocker. DEX +1, ATK +5.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'rocker_card' },
    { id: 5012, name: 'Farmiliar Card', desc: 'A card with the image of a Familiar. Drains 5% HP on attack.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'farmiliar_card' },
    { id: 5013, name: 'Savage Babe Card', desc: 'A card with the image of a Savage Babe. 5% chance to stun.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'savage_babe_card' },
    { id: 5014, name: 'Spore Card', desc: 'A card with the image of a Spore. VIT +2.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'spore_card' },
    { id: 5015, name: 'Zombie Card', desc: 'A card with the image of a Zombie. HP Recovery +20%.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'zombie_card' },
    { id: 5016, name: 'Skeleton Card', desc: 'A card with the image of a Skeleton. ATK +10, HIT +5.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'skeleton_card' },
    { id: 5017, name: 'Creamy Card', desc: 'A card with the image of a Creamy. Teleport Lv1.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'creamy_card' },
    { id: 5018, name: 'Poporing Card', desc: 'A card with the image of a Poporing. Detoxify Lv1.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'poporing_card' },
    { id: 5019, name: 'Pecopeco Card', desc: 'A card with the image of a Peco Peco. Max HP +10%.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'pecopeco_card' },
    { id: 5020, name: 'Mandragora Card', desc: 'A card with the image of a Mandragora. INT +1, Max SP +50.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'mandragora_card' },
    { id: 5021, name: 'Poison Spore Card', desc: 'A card with the image of a Poison Spore. 5% poison on attack.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'poison_spore_card' },
    { id: 5022, name: 'Smokie Card', desc: 'A card with the image of a Smokie. Hiding Lv1.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'smokie_card' },
    { id: 5023, name: 'Yoyo Card', desc: 'A card with the image of a Yoyo. AGI +1, Perfect Dodge +5.', type: 'card', weight: 1, price: 4500, stackable: false, maxStack: 1, icon: 'yoyo_card' },
];

// ============================================================
// Existing items to add as drops on appropriate RO monsters
// Maps existing item_id → list of monster template keys + drop chance
// ============================================================
const EXISTING_ITEM_DROPS = {
    // 1001 Crimson Vial → low-level healer drop
    1001: [
        { monster: 'poring', chance: 0.05 },
        { monster: 'lunatic', chance: 0.04 },
        { monster: 'fabre', chance: 0.04 },
        { monster: 'drops', chance: 0.05 },
        { monster: 'chonchon', chance: 0.06 },
        { monster: 'spore', chance: 0.07 },
        { monster: 'zombie', chance: 0.08 },
        { monster: 'skeleton', chance: 0.08 },
    ],
    // 1002 Amber Elixir → mid-level drop
    1002: [
        { monster: 'rocker', chance: 0.03 },
        { monster: 'hornet', chance: 0.03 },
        { monster: 'zombie', chance: 0.04 },
        { monster: 'skeleton', chance: 0.04 },
        { monster: 'creamy', chance: 0.04 },
        { monster: 'pecopeco', chance: 0.05 },
        { monster: 'poison_spore', chance: 0.05 },
    ],
    // 1005 Roasted Haunch → animal/beast drops
    1005: [
        { monster: 'savage_babe', chance: 0.06 },
        { monster: 'condor', chance: 0.04 },
        { monster: 'pecopeco', chance: 0.05 },
        { monster: 'yoyo', chance: 0.04 },
    ],
    // 2001 Gloopy Residue (jellopy equivalent) → slime creatures
    2001: [
        { monster: 'poring', chance: 0.50 },
        { monster: 'drops', chance: 0.50 },
        { monster: 'poporing', chance: 0.40 },
    ],
    // 2002 Viscous Slime (sticky mucus equivalent) → slime/poison creatures
    2002: [
        { monster: 'poring', chance: 0.15 },
        { monster: 'poporing', chance: 0.20 },
        { monster: 'poison_spore', chance: 0.15 },
    ],
    // 2003 Chitin Shard (shell equivalent) → armored creatures
    2003: [
        { monster: 'pupa', chance: 0.40 },
        { monster: 'chonchon', chance: 0.25 },
    ],
    // 2004 Downy Plume (feather equivalent) → flying/feathered
    2004: [
        { monster: 'lunatic', chance: 0.15 },
        { monster: 'condor', chance: 0.20 },
    ],
    // 2005 Spore Cluster (mushroom_spore equivalent) → mushroom types
    2005: [
        { monster: 'spore', chance: 0.40 },
        { monster: 'poison_spore', chance: 0.35 },
    ],
    // 2006 Barbed Limb (insect_leg equivalent) → insect types
    2006: [
        { monster: 'hornet', chance: 0.30 },
        { monster: 'chonchon', chance: 0.25 },
        { monster: 'rocker', chance: 0.20 },
    ],
    // 2007 Verdant Leaf (green_herb equivalent) → plant-adjacent
    2007: [
        { monster: 'fabre', chance: 0.15 },
        { monster: 'roda_frog', chance: 0.10 },
        { monster: 'mandragora', chance: 0.15 },
    ],
    // 2008 Silken Tuft (fluff equivalent) → soft/fluffy creatures
    2008: [
        { monster: 'fabre', chance: 0.40 },
        { monster: 'lunatic', chance: 0.15 },
    ],
    // 3001 Rustic Shiv → blade-dropping creatures
    3001: [
        { monster: 'poring', chance: 0.01 },
        { monster: 'skeleton', chance: 0.02 },
    ],
    // 3004 Iron Cleaver → undead/warrior
    3004: [
        { monster: 'skeleton', chance: 0.01 },
        { monster: 'zombie', chance: 0.01 },
    ],
    // 4001 Linen Tunic → common armor drop
    4001: [
        { monster: 'pupa', chance: 0.02 },
        { monster: 'spore', chance: 0.01 },
    ],
    // 4002 Quilted Vest → mid armor drop
    4002: [
        { monster: 'skeleton', chance: 0.01 },
        { monster: 'zombie', chance: 0.01 },
    ],
};

// ============================================================
// Generate SQL migration
// ============================================================
function generateSQL() {
    let sql = `-- ============================================================
-- RO Drop Items Migration
-- Generated by scripts/generate_ro_items_migration.js
-- Adds all 126 unique drop items from zone 1-3 RO monsters
-- ============================================================

`;

    // Group items by type for readability
    const groups = {
        consumable: { label: 'Consumables', items: [] },
        etc: { label: 'Etc / Loot Items', items: [] },
        weapon: { label: 'Weapons', items: [] },
        armor: { label: 'Armor / Headgear / Footgear / Accessories', items: [] },
        card: { label: 'Monster Cards', items: [] },
    };

    for (const item of ALL_ITEMS) {
        groups[item.type].items.push(item);
    }

    for (const [type, group] of Object.entries(groups)) {
        if (group.items.length === 0) continue;
        sql += `-- ${group.label} (${group.items.length} items)\n`;

        if (type === 'weapon') {
            // Weapons have extra columns
            sql += `INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, atk, matk, required_level, stackable, max_stack, icon, weapon_type, aspd_modifier, weapon_range) VALUES\n`;
            const rows = group.items.map((item, i) => {
                const comma = i < group.items.length - 1 ? ',' : '';
                return `(${item.id}, '${esc(item.name)}', '${esc(item.desc)}', 'weapon', '${item.slot}', ${item.weight}, ${item.price}, ${item.atk}, ${item.matk || 0}, ${item.reqLv}, false, 1, '${item.icon}', '${item.weaponType}', ${item.aspdMod}, ${item.weaponRange})${comma}`;
            });
            sql += rows.join('\n') + '\nON CONFLICT (item_id) DO NOTHING;\n\n';
        } else if (type === 'armor') {
            // Armor has equip_slot and def
            sql += `INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, def, required_level, stackable, max_stack, icon) VALUES\n`;
            const rows = group.items.map((item, i) => {
                const comma = i < group.items.length - 1 ? ',' : '';
                return `(${item.id}, '${esc(item.name)}', '${esc(item.desc)}', 'armor', '${item.slot}', ${item.weight}, ${item.price}, ${item.def}, ${item.reqLv}, false, 1, '${item.icon}')${comma}`;
            });
            sql += rows.join('\n') + '\nON CONFLICT (item_id) DO NOTHING;\n\n';
        } else {
            // Consumables, etc, cards
            sql += `INSERT INTO items (item_id, name, description, item_type, weight, price, stackable, max_stack, icon) VALUES\n`;
            const rows = group.items.map((item, i) => {
                const comma = i < group.items.length - 1 ? ',' : '';
                return `(${item.id}, '${esc(item.name)}', '${esc(item.desc)}', '${item.type}', ${item.weight}, ${item.price}, ${item.stackable}, ${item.maxStack}, '${item.icon}')${comma}`;
            });
            sql += rows.join('\n') + '\nON CONFLICT (item_id) DO NOTHING;\n\n';
        }
    }

    return sql;
}

function esc(str) {
    return str.replace(/'/g, "''");
}

// ============================================================
// Generate item name → ID mapping (JS object for server)
// ============================================================
function generateMapping() {
    const mapping = {};
    for (const item of ALL_ITEMS) {
        mapping[item.name] = item.id;
    }
    return mapping;
}

// ============================================================
// Generate existing items extra drops config (JS object for server)
// ============================================================
function generateExistingDropsConfig() {
    return EXISTING_ITEM_DROPS;
}

// ============================================================
// Main: write outputs
// ============================================================
const sqlOutput = generateSQL();
const sqlPath = path.join(__dirname, '..', 'database', 'migrations', 'add_ro_drop_items.sql');
fs.writeFileSync(sqlPath, sqlOutput, 'utf-8');
console.log(`✓ SQL migration written to: ${sqlPath}`);
console.log(`  Total items: ${ALL_ITEMS.length}`);

const mappingOutput = generateMapping();
const mappingPath = path.join(__dirname, '..', 'server', 'src', 'ro_item_mapping.js');
const mappingJS = `// Auto-generated by scripts/generate_ro_items_migration.js
// Maps RO drop item names → database item_ids
// DO NOT EDIT MANUALLY — regenerate with: node scripts/generate_ro_items_migration.js

const RO_ITEM_NAME_TO_ID = ${JSON.stringify(mappingOutput, null, 4)};

// Existing game items to add as extra drops on RO monsters
// Maps item_id → array of { monster: templateKey, chance: dropChance }
const EXISTING_ITEM_EXTRA_DROPS = ${JSON.stringify(generateExistingDropsConfig(), null, 4)};

module.exports = { RO_ITEM_NAME_TO_ID, EXISTING_ITEM_EXTRA_DROPS };
`;
fs.writeFileSync(mappingPath, mappingJS, 'utf-8');
console.log(`✓ Item mapping written to: ${mappingPath}`);
console.log(`  Mapped names: ${Object.keys(mappingOutput).length}`);
console.log(`  Existing items with extra drops: ${Object.keys(EXISTING_ITEM_DROPS).length}`);

// Verify no duplicate IDs
const ids = ALL_ITEMS.map(i => i.id);
const dupes = ids.filter((id, idx) => ids.indexOf(id) !== idx);
if (dupes.length > 0) {
    console.error(`✗ DUPLICATE IDs found: ${dupes.join(', ')}`);
    process.exit(1);
} else {
    console.log(`✓ No duplicate item IDs`);
}

// Summary by type
const byType = {};
for (const item of ALL_ITEMS) {
    byType[item.type] = (byType[item.type] || 0) + 1;
}
console.log('\nBreakdown by type:');
for (const [type, count] of Object.entries(byType)) {
    console.log(`  ${type}: ${count} items`);
}
