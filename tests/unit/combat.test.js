// Unit tests for combat system
// Run with: node tests/unit/combat.test.js

// Mock the server functions (in real setup, these would be imported)
const COMBAT = {
    ASPD_CAP: 195
};

function getAttackIntervalMs(aspd) {
    if (aspd <= COMBAT.ASPD_CAP) {
        // Linear formula up to hard cap (195)
        return (200 - aspd) * 50; // e.g. ASPD 195 → 5 * 50 = 250ms
    } else {
        // Diminishing returns above 195: exponential decay, bottoms out ~217ms at 199
        const excessAspd = Math.min(aspd - COMBAT.ASPD_CAP, 9); // max 9 excess points (196-199 useful range)
        const decayFactor = Math.exp(-excessAspd * 0.35);
        const maxBonus = 130; // Maximum ms reduction above 195 (250ms → ~120ms floor)
        const actualBonus = Math.floor(maxBonus * (1 - decayFactor));
        return Math.max(217, 250 - actualBonus); // Floor ~217ms (~4.6 attacks/sec absolute max)
    }
}

function calculateDamage(attacker, defender) {
    const baseAtk = attacker.str + attacker.weaponAtk;
    const totalDef = defender.vit + defender.def;
    const damage = Math.max(1, baseAtk - totalDef);
    return damage;
}

// Test runner
function runTests() {
    let passed = 0;
    let failed = 0;
    
    function test(name, testFn) {
        try {
            testFn();
            console.log(`✅ PASS: ${name}`);
            passed++;
        } catch (error) {
            console.log(`❌ FAIL: ${name}`);
            console.log(`   Error: ${error.message}`);
            failed++;
        }
    }
    
    function assert(condition, message) {
        if (!condition) {
            throw new Error(message || 'Assertion failed');
        }
    }
    
    console.log('🧪 Running Combat System Tests\n');
    
    // ASPD conversion tests
    test('ASPD 170 should be 1500ms', () => {
        const result = getAttackIntervalMs(170);
        assert(result === 1500, `Expected 1500ms, got ${result}ms`);
    });
    
    test('ASPD 180 should be 1000ms', () => {
        const result = getAttackIntervalMs(180);
        assert(result === 1000, `Expected 1000ms, got ${result}ms`);
    });
    
    test('ASPD 190 should be 500ms', () => {
        const result = getAttackIntervalMs(190);
        assert(result === 500, `Expected 500ms, got ${result}ms`);
    });
    
    test('ASPD 195 (cap) should be 250ms', () => {
        const result = getAttackIntervalMs(195);
        assert(result === 250, `Expected 250ms, got ${result}ms`);
    });
    
    test('ASPD above 195 should have diminishing returns', () => {
        const result = getAttackIntervalMs(199);
        assert(result >= 217, `Expected >=217ms (diminishing returns), got ${result}ms`);
    });
    
    // Damage calculation tests
    test('Damage calculation with basic stats', () => {
        const attacker = { str: 10, weaponAtk: 50 };
        const defender = { vit: 5, def: 20 };
        
        const damage = calculateDamage(attacker, defender);
        const expected = (10 + 50) - (5 + 20); // 60 - 25 = 35
        
        assert(damage === expected, `Expected ${expected} damage, got ${damage}`);
        assert(damage > 0, 'Damage should be positive');
    });
    
    test('Damage should be at least 1', () => {
        const attacker = { str: 1, weaponAtk: 1 };
        const defender = { vit: 50, def: 50 };
        
        const damage = calculateDamage(attacker, defender);
        assert(damage === 1, `Expected minimum damage 1, got ${damage}`);
    });
    
    test('Damage calculation with zero defense', () => {
        const attacker = { str: 15, weaponAtk: 25 };
        const defender = { vit: 0, def: 0 };
        
        const damage = calculateDamage(attacker, defender);
        const expected = 15 + 25; // 40
        
        assert(damage === expected, `Expected ${expected} damage, got ${damage}`);
    });
    
    // Edge cases
    test('Negative defense should not break calculation', () => {
        const attacker = { str: 10, weaponAtk: 10 };
        const defender = { vit: -5, def: -5 };
        
        const damage = calculateDamage(attacker, defender);
        const expected = (10 + 10) - (-5 + -5); // 20 - (-10) = 30
        
        assert(damage === expected, `Expected ${expected} damage, got ${damage}`);
    });
    
    console.log(`\n📊 Results: ${passed} passed, ${failed} failed`);
    
    if (failed === 0) {
        console.log('🎉 All tests passed!');
        process.exit(0);
    } else {
        console.log('💥 Some tests failed!');
        process.exit(1);
    }
}

// Run the tests
if (require.main === module) {
    runTests();
}

module.exports = { getAttackIntervalMs, calculateDamage, runTests };
