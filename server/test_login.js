const http = require('http');

// Test login
const loginData = JSON.stringify({
    username: 'testplayer',
    password: 'password123'
});

const loginOptions = {
    hostname: 'localhost',
    port: 3001,
    path: '/api/auth/login',
    method: 'POST',
    headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(loginData)
    }
};

console.log('Testing login for testplayer...\n');

const req = http.request(loginOptions, (res) => {
    console.log(`Status: ${res.statusCode}`);
    
    let data = '';
    res.on('data', (chunk) => {
        data += chunk;
    });
    
    res.on('end', () => {
        console.log('Response:', data);
        console.log('\nLogin complete! Run check_database.js to verify last_login was updated.');
    });
});

req.on('error', (e) => {
    console.error(`Error: ${e.message}`);
});

req.write(loginData);
req.end();
