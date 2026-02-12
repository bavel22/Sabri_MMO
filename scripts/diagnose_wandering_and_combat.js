/**
 * Diagnostic script: Tests enemy wandering (enemy:move) and combat stop events
 * Run with: node scripts/diagnose_wandering_and_combat.js
 * 
 * What this does:
 * 1. Connects to the server via Socket.io
 * 2. Listens for enemy:move events (proves wandering works server-side)
 * 3. Listens for combat:auto_attack_stopped and combat:target_lost events
 * 4. Reports results after 30 seconds
 */
const { io } = require('../server/node_modules/socket.io-client');

const SERVER_URL = 'http://localhost:3000';
const TEST_DURATION_MS = 30000;

let enemyMoveCount = 0;
let autoAttackStoppedCount = 0;
let targetLostCount = 0;
let enemySpawnCount = 0;
const enemyMoveLog = [];

console.log(`\n=== Sabri_MMO Diagnostic: Wandering & Combat Events ===`);
console.log(`Connecting to ${SERVER_URL}...`);

const socket = io(SERVER_URL);

socket.on('connect', () => {
    console.log(`✓ Connected! Socket ID: ${socket.id}`);
    console.log(`  Listening for events for ${TEST_DURATION_MS / 1000} seconds...\n`);
});

socket.on('connect_error', (err) => {
    console.error(`✗ Connection failed: ${err.message}`);
    console.log('  Make sure the server is running: node server/src/index.js');
    process.exit(1);
});

// --- Enemy Events ---
socket.on('enemy:spawn', (data) => {
    enemySpawnCount++;
    console.log(`  [enemy:spawn] ${data.name} (ID: ${data.enemyId}) at (${data.x}, ${data.y}, ${data.z})`);
});

socket.on('enemy:move', (data) => {
    enemyMoveCount++;
    const movingStr = data.isMoving ? 'MOVING' : 'STOPPED';
    const msg = `  [enemy:move] #${enemyMoveCount} — Enemy ${data.enemyId} ${movingStr} at (${data.x?.toFixed(0)}, ${data.y?.toFixed(0)})`;
    console.log(msg);
    if (enemyMoveCount <= 5) {
        enemyMoveLog.push(msg);
    }
});

// --- Combat Events ---
socket.on('combat:auto_attack_stopped', (data) => {
    autoAttackStoppedCount++;
    console.log(`  [combat:auto_attack_stopped] reason: ${data.reason}, oldTargetId: ${data.oldTargetId}, oldIsEnemy: ${data.oldIsEnemy}`);
});

socket.on('combat:target_lost', (data) => {
    targetLostCount++;
    console.log(`  [combat:target_lost] reason: ${data.reason}, isEnemy: ${data.isEnemy}`);
});

socket.on('combat:auto_attack_started', (data) => {
    console.log(`  [combat:auto_attack_started] target: ${data.targetName} (ID: ${data.targetId}), isEnemy: ${data.isEnemy}`);
});

// --- Summary after test period ---
setTimeout(() => {
    console.log(`\n=== DIAGNOSTIC RESULTS (${TEST_DURATION_MS / 1000}s) ===`);
    console.log(`  enemy:spawn received:              ${enemySpawnCount}`);
    console.log(`  enemy:move received:               ${enemyMoveCount}`);
    console.log(`  combat:auto_attack_stopped received: ${autoAttackStoppedCount}`);
    console.log(`  combat:target_lost received:        ${targetLostCount}`);
    
    console.log(`\n--- WANDERING DIAGNOSIS ---`);
    if (enemyMoveCount > 0) {
        console.log(`✓ PASS: Server IS broadcasting enemy:move events (${enemyMoveCount} received)`);
        console.log(`  → If enemies aren't moving on screen, the issue is CLIENT-SIDE:`);
        console.log(`    1. Check BP_SocketManager binds "enemy:move" → OnEnemyMove function`);
        console.log(`    2. OnEnemyMove must parse JSON → get enemyId, x, y, z`);
        console.log(`    3. Look up enemy actor via BP_EnemyManager → SetTargetPosition`);
        console.log(`    4. BP_EnemyCharacter Event Tick must interpolate toward TargetPosition`);
    } else {
        console.log(`✗ FAIL: No enemy:move events received in ${TEST_DURATION_MS / 1000}s`);
        console.log(`  → Check server console for [ENEMY AI] log messages`);
        console.log(`  → Enemies may all be in combat (inCombatWith.size > 0) or dead`);
    }
    
    console.log(`\n--- COMBAT STOP DIAGNOSIS ---`);
    console.log(`  To test combat:auto_attack_stopped:`);
    console.log(`  1. Attack an enemy in-game, then click the GROUND`);
    console.log(`     → Client must emit "combat:stop_attack" to server`);
    console.log(`     → Server responds with "combat:auto_attack_stopped"`);
    console.log(`  2. Attack an enemy, then click a DIFFERENT enemy`);
    console.log(`     → Client emits "combat:attack" with new targetEnemyId`);
    console.log(`     → Server emits "combat:auto_attack_stopped" for old target`);
    console.log(`  Check server console for:`);
    console.log(`     [RECV] combat:stop_attack  ← proves client is sending it`);
    console.log(`     [SEND] combat:auto_attack_stopped  ← proves server is responding`);
    
    socket.disconnect();
    process.exit(0);
}, TEST_DURATION_MS);
