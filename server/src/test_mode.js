// Test Mode for UI Testing
// Provides mock data and test endpoints for automated UI testing

const express = require('express');
const router = express.Router();

// Mock test data
const MOCK_DATA = {
    // Test character for UI testing
    testCharacter: {
        character_id: 999,
        user_id: 999,
        name: "TestPlayer_UI",
        class: "warrior",
        level: 10,
        x: 0, y: 0, z: 300,
        health: 500, max_health: 500,
        mana: 200, max_mana: 200,
        str: 20, agi: 15, vit: 18,
        int_stat: 10, dex: 12, luk: 8,
        stat_points: 20,
        zuzucoin: 5000,
        created_at: new Date(),
        last_played: new Date()
    },

    // Test shop data
    testShop: {
        shopName: "Test Weapon Shop",
        shopId: 999,
        playerZuzucoin: 5000,
        items: [
            {
                item_id: 1,
                name: "Iron Sword",
                price: 1000,
                description: "Basic sword for beginners",
                item_type: "weapon",
                weapon_type: "sword",
                atk: 25,
                required_level: 5
            },
            {
                item_id: 2,
                name: "Health Potion",
                price: 50,
                description: "Restores 100 HP",
                item_type: "consumable",
                quantity: 1,
                required_level: 1
            },
            {
                item_id: 3,
                name: "Iron Armor",
                price: 1500,
                description: "Basic armor for warriors",
                item_type: "armor",
                def: 15,
                required_level: 8
            }
        ]
    },

    // Test inventory data
    testInventory: {
        items: [
            {
                inventory_id: 9991,
                item_id: 1,
                name: "Iron Sword",
                quantity: 1,
                equipped: false,
                item_type: "weapon",
                atk: 25,
                weapon_type: "sword",
                description: "Basic sword for beginners"
            },
            {
                inventory_id: 9992,
                item_id: 2,
                name: "Health Potion",
                quantity: 5,
                equipped: false,
                item_type: "consumable",
                description: "Restores 100 HP"
            },
            {
                inventory_id: 9993,
                item_id: 3,
                name: "Iron Armor",
                quantity: 1,
                equipped: true,
                item_type: "armor",
                def: 15,
                equip_slot: "armor",
                description: "Basic armor for warriors"
            }
        ],
        zuzucoin: 5000
    },

    // Test combat data
    testCombat: {
        attacker: {
            characterId: 999,
            name: "TestPlayer_UI",
            x: 100, y: 100, z: 300,
            hp: 450, max_hp: 500,
            targetId: 888
        },
        target: {
            characterId: 888,
            name: "TestEnemy_UI",
            x: 200, y: 100, z: 300,
            hp: 200, max_hp: 300,
            isEnemy: true
        },
        damage: 75
    }
};

// Test endpoints for UI testing
router.get('/setup', (req, res) => {
    res.json({
        status: 'success',
        message: 'Test mode ready',
        mockData: MOCK_DATA
    });
});

router.get('/character', (req, res) => {
    res.json({
        status: 'success',
        character: MOCK_DATA.testCharacter
    });
});

router.get('/shop', (req, res) => {
    res.json({
        status: 'success',
        shop: MOCK_DATA.testShop
    });
});

router.get('/inventory', (req, res) => {
    res.json({
        status: 'success',
        inventory: MOCK_DATA.testInventory
    });
});

router.get('/combat', (req, res) => {
    res.json({
        status: 'success',
        combat: MOCK_DATA.testCombat
    });
});

// Mock socket events for testing
router.post('/socket-event', (req, res) => {
    const { eventName, data } = req.body;
    
    // Simulate server response
    let responseData = null;
    
    switch (eventName) {
        case 'player:join':
            responseData = {
                characterId: MOCK_DATA.testCharacter.character_id,
                name: MOCK_DATA.testCharacter.name,
                class: MOCK_DATA.testCharacter.class,
                level: MOCK_DATA.testCharacter.level,
                x: MOCK_DATA.testCharacter.x,
                y: MOCK_DATA.testCharacter.y,
                z: MOCK_DATA.testCharacter.z,
                health: MOCK_DATA.testCharacter.health,
                max_health: MOCK_DATA.testCharacter.max_health,
                mana: MOCK_DATA.testCharacter.mana,
                max_mana: MOCK_DATA.testCharacter.max_mana,
                str: MOCK_DATA.testCharacter.str,
                agi: MOCK_DATA.testCharacter.agi,
                vit: MOCK_DATA.testCharacter.vit,
                int_stat: MOCK_DATA.testCharacter.int_stat,
                dex: MOCK_DATA.testCharacter.dex,
                luk: MOCK_DATA.testCharacter.luk,
                stat_points: MOCK_DATA.testCharacter.stat_points,
                zuzucoin: MOCK_DATA.testCharacter.zuzucoin
            };
            break;
            
        case 'shop:open':
            responseData = MOCK_DATA.testShop;
            break;
            
        case 'shop:buy':
            const { itemId, quantity } = data;
            const item = MOCK_DATA.testShop.items.find(i => i.item_id === itemId);
            if (item) {
                const totalCost = item.price * quantity;
                const newZuzucoin = MOCK_DATA.testShop.playerZuzucoin - totalCost;
                responseData = {
                    itemId: itemId,
                    itemName: item.name,
                    quantity: quantity,
                    totalCost: totalCost,
                    newZuzucoin: newZuzucoin
                };
                
                // Update mock data
                MOCK_DATA.testShop.playerZuzucoin = newZuzucoin;
                MOCK_DATA.testInventory.zuzucoin = newZuzucoin;
                
                // Add item to inventory
                if (!MOCK_DATA.testInventory.items.find(i => i.item_id === itemId)) {
                    MOCK_DATA.testInventory.items.push({
                        inventory_id: 9999 + MOCK_DATA.testInventory.items.length,
                        item_id: itemId,
                        name: item.name,
                        quantity: quantity,
                        equipped: false,
                        item_type: item.item_type,
                        description: item.description
                    });
                }
            }
            break;
            
        case 'inventory:load':
            responseData = MOCK_DATA.testInventory;
            break;
            
        case 'combat:attack':
            responseData = MOCK_DATA.testCombat;
            break;
            
        default:
            return res.json({
                status: 'error',
                message: `Unknown test event: ${eventName}`
            });
    }
    
    res.json({
        status: 'success',
        eventName: eventName,
        data: responseData
    });
});

// Reset test data
router.post('/reset', (req, res) => {
    // Reset to original values
    MOCK_DATA.testCharacter.zuzucoin = 5000;
    MOCK_DATA.testCharacter.health = MOCK_DATA.testCharacter.max_health;
    MOCK_DATA.testCharacter.mana = MOCK_DATA.testCharacter.max_mana;
    MOCK_DATA.testShop.playerZuzucoin = 5000;
    MOCK_DATA.testInventory.zuzucoin = 5000;
    
    // Reset inventory to original state
    MOCK_DATA.testInventory.items = [
        {
            inventory_id: 9991,
            item_id: 1,
            name: "Iron Sword",
            quantity: 1,
            equipped: false,
            item_type: "weapon",
            atk: 25,
            weapon_type: "sword",
            description: "Basic sword for beginners"
        },
        {
            inventory_id: 9992,
            item_id: 2,
            name: "Health Potion",
            quantity: 5,
            equipped: false,
            item_type: "consumable",
            description: "Restores 100 HP"
        },
        {
            inventory_id: 9993,
            item_id: 3,
            name: "Iron Armor",
            quantity: 1,
            equipped: true,
            item_type: "armor",
            def: 15,
            equip_slot: "armor",
            description: "Basic armor for warriors"
        }
    ];
    
    res.json({
        status: 'success',
        message: 'Test data reset to original values'
    });
});

module.exports = router;
module.exports.MOCK_DATA = MOCK_DATA;
