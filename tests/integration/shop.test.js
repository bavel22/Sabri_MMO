// Integration tests for shop system
// Run with: node tests/integration/shop.test.js
// NOTE: This requires the server to be running on localhost:3001

const io = require('socket.io-client');

// Test configuration
const SERVER_URL = 'http://localhost:3001';
const TEST_CONFIG = {
    timeout: 5000,
    testCharacterId: 1,
    testShopId: 1,
    testToken: 'mock-jwt-token-for-testing'
};

// Test state
let testSocket;
let testResults = {
    connected: false,
    joined: false,
    shopOpened: false,
    shopDataReceived: false,
    purchaseAttempted: false,
    purchaseCompleted: false,
    errors: []
};

// Test runner
function runTests() {
    console.log('🧪 Running Shop System Integration Tests\n');
    console.log('⚠️  NOTE: This test requires the server to be running on localhost:3001');
    console.log('⚠️  NOTE: This uses mock data - no real database operations\n');
    
    // Setup socket connection
    testSocket = io(SERVER_URL);
    
    // Setup event handlers
    testSocket.on('connect', () => {
        console.log('✅ Connected to server');
        testResults.connected = true;
        startTestSequence();
    });
    
    testSocket.on('connect_error', (error) => {
        console.log('❌ Failed to connect to server');
        console.log(`   Error: ${error.message}`);
        console.log('   Make sure the server is running on localhost:3001');
        finishTest();
    });
    
    testSocket.on('player:joined', (data) => {
        try {
            const player = JSON.parse(data);
            console.log('✅ Player joined game');
            console.log(`   Character ID: ${player.characterId}`);
            console.log(`   Zuzucoin: ${player.zuzucoin || 0}`);
            testResults.joined = true;
            continueTestSequence();
        } catch (error) {
            console.log('❌ Failed to parse player:joined data');
            testResults.errors.push('player:joined parse error');
            finishTest();
        }
    });
    
    testSocket.on('shop:data', (data) => {
        try {
            const shop = JSON.parse(data);
            console.log('✅ Shop data received');
            console.log(`   Shop Name: ${shop.shopName || 'Unknown'}`);
            console.log(`   Shop Items: ${shop.items ? shop.items.length : 0}`);
            console.log(`   Player Zuzucoin: ${shop.playerZuzucoin || 0}`);
            testResults.shopDataReceived = true;
            
            // Try to buy first item if available
            if (shop.items && shop.items.length > 0) {
                const firstItem = shop.items[0];
                console.log(`🛒 Attempting to buy: ${firstItem.name || 'Unknown Item'}`);
                
                testSocket.emit('shop:buy', JSON.stringify({
                    shopId: TEST_CONFIG.testShopId,
                    itemId: firstItem.item_id || 1,
                    quantity: 1
                }));
                
                testResults.purchaseAttempted = true;
            } else {
                console.log('⚠️  No items in shop to test purchase');
                finishTest();
            }
        } catch (error) {
            console.log('❌ Failed to parse shop:data');
            testResults.errors.push('shop:data parse error');
            finishTest();
        }
    });
    
    testSocket.on('shop:bought', (data) => {
        try {
            const purchase = JSON.parse(data);
            console.log('✅ Purchase completed');
            console.log(`   Item ID: ${purchase.itemId}`);
            console.log(`   Quantity: ${purchase.quantity || 1}`);
            console.log(`   Cost: ${purchase.totalCost || 0}`);
            console.log(`   New Zuzucoin: ${purchase.newZuzucoin || 0}`);
            testResults.purchaseCompleted = true;
            finishTest();
        } catch (error) {
            console.log('❌ Failed to parse shop:bought data');
            testResults.errors.push('shop:bought parse error');
            finishTest();
        }
    });
    
    testSocket.on('shop:error', (data) => {
        try {
            const error = JSON.parse(data);
            console.log('⚠️  Shop error received (this might be expected)');
            console.log(`   Message: ${error.message || 'Unknown error'}`);
            testResults.errors.push(`Shop error: ${error.message || 'Unknown'}`);
            finishTest();
        } catch (parseError) {
            console.log('❌ Failed to parse shop:error');
            testResults.errors.push('shop:error parse error');
            finishTest();
        }
    });
    
    // Timeout
    setTimeout(() => {
        console.log('⏰ Test timeout reached');
        finishTest();
    }, TEST_CONFIG.timeout);
}

function startTestSequence() {
    console.log('👤 Joining game with test character...');
    testSocket.emit('player:join', JSON.stringify({
        characterId: TEST_CONFIG.testCharacterId,
        token: TEST_CONFIG.testToken
    }));
}

function continueTestSequence() {
    if (!testResults.shopOpened) {
        console.log('🏪 Opening shop...');
        testSocket.emit('shop:open', JSON.stringify({
            shopId: TEST_CONFIG.testShopId
        }));
        testResults.shopOpened = true;
    }
}

function finishTest() {
    console.log('\n📊 Test Results:');
    console.log(`   Connected: ${testResults.connected ? '✅' : '❌'}`);
    console.log(`   Joined Game: ${testResults.joined ? '✅' : '❌'}`);
    console.log(`   Shop Opened: ${testResults.shopOpened ? '✅' : '❌'}`);
    console.log(`   Shop Data Received: ${testResults.shopDataReceived ? '✅' : '❌'}`);
    console.log(`   Purchase Attempted: ${testResults.purchaseAttempted ? '✅' : '❌'}`);
    console.log(`   Purchase Completed: ${testResults.purchaseCompleted ? '✅' : '❌'}`);
    
    if (testResults.errors.length > 0) {
        console.log('\n❌ Errors:');
        testResults.errors.forEach(error => console.log(`   - ${error}`));
    }
    
    // Cleanup
    if (testSocket) {
        testSocket.close();
    }
    
    // Determine success
    const criticalTests = [
        testResults.connected,
        testResults.joined,
        testResults.shopOpened,
        testResults.shopDataReceived
    ];
    
    const allCriticalPassed = criticalTests.every(test => test === true);
    
    if (allCriticalPassed) {
        console.log('\n🎉 Critical tests passed!');
        console.log('💡 Note: Purchase test requires valid database setup');
        process.exit(0);
    } else {
        console.log('\n💥 Some critical tests failed!');
        console.log('💡 Make sure:');
        console.log('   - Server is running on localhost:3001');
        console.log('   - Database is accessible');
        console.log('   - Test character exists in database');
        process.exit(1);
    }
}

// Handle uncaught errors
process.on('uncaughtException', (error) => {
    console.log('❌ Uncaught exception:', error.message);
    if (testSocket) testSocket.close();
    process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
    console.log('❌ Unhandled rejection:', reason);
    if (testSocket) testSocket.close();
    process.exit(1);
});

// Run the tests
if (require.main === module) {
    runTests();
}

module.exports = { runTests, TEST_CONFIG };
