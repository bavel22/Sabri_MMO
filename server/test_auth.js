const http = require('http');

// Test data
const testData = {
    username: 'testuser',
    email: 'test@example.com',
    password: 'password123'
};

const postData = JSON.stringify(testData);

// Options for POST request
const options = {
    hostname: 'localhost',
    port: 3001,
    path: '/api/auth/register',
    method: 'POST',
    headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(postData)
    }
};

console.log('Testing registration endpoint...');

const req = http.request(options, (res) => {
    console.log(`Status: ${res.statusCode}`);
    console.log(`Headers: ${JSON.stringify(res.headers)}`);
    
    let data = '';
    
    res.on('data', (chunk) => {
        data += chunk;
    });
    
    res.on('end', () => {
        console.log('Response:', data);
        
        // Test login with the registered user
        if (res.statusCode === 201) {
            console.log('\nTesting login endpoint...');
            testLogin();
        }
    });
});

req.on('error', (e) => {
    console.error(`Problem with request: ${e.message}`);
});

// Write data to request body
req.write(postData);
req.end();

function testLogin() {
    const loginData = JSON.stringify({
        username: 'testuser',
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
    
    const loginReq = http.request(loginOptions, (res) => {
        console.log(`Status: ${res.statusCode}`);
        
        let data = '';
        res.on('data', (chunk) => {
            data += chunk;
        });
        
        res.on('end', () => {
            console.log('Response:', data);
            
            // Test token verification
            if (res.statusCode === 200) {
                const response = JSON.parse(data);
                const token = response.token;
                console.log('\nTesting token verification...');
                testTokenVerification(token);
            }
        });
    });
    
    loginReq.on('error', (e) => {
        console.error(`Problem with login request: ${e.message}`);
    });
    
    loginReq.write(loginData);
    loginReq.end();
}

function testTokenVerification(token) {
    const verifyOptions = {
        hostname: 'localhost',
        port: 3001,
        path: '/api/auth/verify',
        method: 'GET',
        headers: {
            'Authorization': `Bearer ${token}`
        }
    };
    
    const verifyReq = http.request(verifyOptions, (res) => {
        console.log(`Status: ${res.statusCode}`);
        
        let data = '';
        res.on('data', (chunk) => {
            data += chunk;
        });
        
        res.on('end', () => {
            console.log('Response:', data);
        });
    });
    
    verifyReq.on('error', (e) => {
        console.error(`Problem with verification request: ${e.message}`);
    });
    
    verifyReq.end();
}
