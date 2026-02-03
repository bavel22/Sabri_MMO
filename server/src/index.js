const express = require('express');
const cors = require('cors');
const { Pool } = require('pg');
const bcrypt = require('bcrypt');
const jwt = require('jsonwebtoken');
const rateLimit = require('express-rate-limit');
const fs = require('fs');
const path = require('path');
require('dotenv').config();

// Setup file logging
const logsDir = path.join(__dirname, '..', 'logs');
if (!fs.existsSync(logsDir)) {
    fs.mkdirSync(logsDir, { recursive: true });
}
const logFile = fs.createWriteStream(path.join(logsDir, 'server.log'), { flags: 'a' });

// Log levels: DEBUG, INFO, WARN, ERROR
const LOG_LEVEL = process.env.LOG_LEVEL || 'INFO';
const LOG_LEVELS = { DEBUG: 0, INFO: 1, WARN: 2, ERROR: 3 };

function shouldLog(level) {
    return LOG_LEVELS[level] >= LOG_LEVELS[LOG_LEVEL];
}

function formatLog(level, message) {
    return `[${new Date().toISOString()}] [${level}] ${message}`;
}

const logger = {
    debug: (...args) => {
        if (shouldLog('DEBUG')) {
            const msg = formatLog('DEBUG', args.join(' '));
            console.log(msg);
            logFile.write(msg + '\n');
        }
    },
    info: (...args) => {
        if (shouldLog('INFO')) {
            const msg = formatLog('INFO', args.join(' '));
            console.log(msg);
            logFile.write(msg + '\n');
        }
    },
    warn: (...args) => {
        if (shouldLog('WARN')) {
            const msg = formatLog('WARN', args.join(' '));
            console.warn(msg);
            logFile.write(msg + '\n');
        }
    },
    error: (...args) => {
        if (shouldLog('ERROR')) {
            const msg = formatLog('ERROR', args.join(' '));
            console.error(msg);
            logFile.write(msg + '\n');
        }
    }
};

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());

// Rate limiting
const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 100, // Limit each IP to 100 requests per windowMs
    message: 'Too many requests from this IP, please try again later.'
});
app.use('/api/', limiter);

// Request logging middleware
app.use((req, res, next) => {
    logger.info(`${req.method} ${req.url} - ${req.ip}`);
    next();
});

// Database connection
const pool = new Pool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
});

// Input validation
function validateRegisterInput(req, res, next) {
    const { username, email, password } = req.body;
    
    // Username validation
    if (!username || username.length < 3 || username.length > 50) {
        return res.status(400).json({ error: 'Username must be between 3 and 50 characters' });
    }
    
    // Email validation
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!email || !emailRegex.test(email)) {
        return res.status(400).json({ error: 'Valid email is required' });
    }
    
    // Password validation
    if (!password || password.length < 8) {
        return res.status(400).json({ error: 'Password must be at least 8 characters long' });
    }
    
    // Password strength check
    if (!/(?=.*[a-zA-Z])(?=.*\d)/.test(password)) {
        return res.status(400).json({ error: 'Password must contain at least one letter and one number' });
    }
    
    next();
}

// JWT middleware
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1]; // Bearer TOKEN
    
    if (!token) {
        return res.status(401).json({ error: 'Access token required' });
    }
    
    jwt.verify(token, process.env.JWT_SECRET, (err, user) => {
        if (err) {
            return res.status(403).json({ error: 'Invalid or expired token' });
        }
        req.user = user;
        next();
    });
}

// Health check endpoint
app.get('/health', async (req, res) => {
  try {
    logger.debug('Health check requested');
    const result = await pool.query('SELECT NOW()');
    logger.info('Health check passed');
    res.json({ 
      status: 'OK', 
      timestamp: result.rows[0].now,
      message: 'Server is running and connected to database'
    });
  } catch (err) {
    logger.error('Health check failed:', err.message);
    res.status(500).json({ 
      status: 'ERROR', 
      message: 'Database connection failed',
      error: err.message 
    });
  }
});

// Authentication endpoints
app.post('/api/auth/register', validateRegisterInput, async (req, res) => {
    try {
        const { username, email, password } = req.body;
        logger.info(`Registration attempt for username: ${username}`);
        
        // Check if user already exists
        const existingUser = await pool.query(
            'SELECT user_id FROM users WHERE username = $1 OR email = $2',
            [username, email]
        );
        
        if (existingUser.rows.length > 0) {
            logger.warn(`Registration failed: Username or email already exists - ${username}`);
            return res.status(409).json({ error: 'Username or email already exists' });
        }
        
        // Hash password
        const saltRounds = 10;
        const passwordHash = await bcrypt.hash(password, saltRounds);
        
        // Create user
        const result = await pool.query(
            'INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING user_id, username, email, created_at',
            [username, email, passwordHash]
        );
        
        const user = result.rows[0];
        logger.info(`User registered successfully: ${username} (ID: ${user.user_id})`);
        
        // Create JWT token
        const token = jwt.sign(
            { user_id: user.user_id, username: user.username },
            process.env.JWT_SECRET,
            { expiresIn: '24h' }
        );
        
        res.status(201).json({
            message: 'User registered successfully',
            user: {
                user_id: user.user_id,
                username: user.username,
                email: user.email,
                created_at: user.created_at
            },
            token
        });
        
    } catch (err) {
        logger.error('Registration error:', err.message);
        res.status(500).json({ error: 'Registration failed' });
    }
});

app.post('/api/auth/login', async (req, res) => {
    try {
        const { username, password } = req.body;
        logger.info(`Login attempt for username: ${username}`);
        
        // Validate input
        if (!username || !password) {
            logger.warn('Login failed: Missing username or password');
            return res.status(400).json({ error: 'Username and password are required' });
        }
        
        // Find user
        const result = await pool.query(
            'SELECT user_id, username, email, password_hash FROM users WHERE username = $1',
            [username]
        );
        
        if (result.rows.length === 0) {
            logger.warn(`Login failed: User not found - ${username}`);
            return res.status(401).json({ error: 'Invalid credentials' });
        }
        
        const user = result.rows[0];
        
        // Verify password
        const validPassword = await bcrypt.compare(password, user.password_hash);
        
        if (!validPassword) {
            logger.warn(`Login failed: Invalid password - ${username}`);
            return res.status(401).json({ error: 'Invalid credentials' });
        }
        
        // Update last_login timestamp
        await pool.query(
            'UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE user_id = $1',
            [user.user_id]
        );
        
        // Create JWT token
        const token = jwt.sign(
            { user_id: user.user_id, username: user.username },
            process.env.JWT_SECRET,
            { expiresIn: '24h' }
        );
        
        logger.info(`Login successful: ${username} (ID: ${user.user_id})`);
        
        res.json({
            message: 'Login successful',
            user: {
                user_id: user.user_id,
                username: user.username,
                email: user.email
            },
            token
        });
        
    } catch (err) {
        logger.error('Login error:', err.message);
        res.status(500).json({ error: 'Login failed' });
    }
});

app.get('/api/auth/verify', authenticateToken, async (req, res) => {
    try {
        logger.debug(`Token verification for user ID: ${req.user.user_id}`);
        // Token is valid, return user info
        const result = await pool.query(
            'SELECT user_id, username, email, created_at FROM users WHERE user_id = $1',
            [req.user.user_id]
        );
        
        if (result.rows.length === 0) {
            logger.warn(`Token verification failed: User not found - ${req.user.user_id}`);
            return res.status(404).json({ error: 'User not found' });
        }
        
        const user = result.rows[0];
        logger.debug(`Token verified for user: ${user.username}`);
        
        res.json({
            message: 'Token is valid',
            user: {
                user_id: user.user_id,
                username: user.username,
                email: user.email,
                created_at: user.created_at
            }
        });
        
    } catch (err) {
        logger.error('Token verification error:', err.message);
        res.status(500).json({ error: 'Token verification failed' });
    }
});

// Character endpoints
app.get('/api/characters', authenticateToken, async (req, res) => {
    try {
        logger.debug(`Fetching characters for user ID: ${req.user.user_id}`);
        const result = await pool.query(
            'SELECT character_id, name, class, level, x, y, z, health, mana, created_at FROM characters WHERE user_id = $1 ORDER BY created_at DESC',
            [req.user.user_id]
        );
        
        logger.info(`Retrieved ${result.rows.length} characters for user ${req.user.user_id}`);
        res.json({
            message: 'Characters retrieved successfully',
            characters: result.rows
        });
    } catch (err) {
        logger.error('Get characters error:', err.message);
        res.status(500).json({ error: 'Failed to retrieve characters' });
    }
});

app.post('/api/characters', authenticateToken, async (req, res) => {
    try {
        const { name, characterClass } = req.body;
        logger.info(`Character creation attempt: ${name} (class: ${characterClass}) for user ${req.user.user_id}`);
        
        // Validation
        if (!name || name.length < 2 || name.length > 50) {
            logger.warn(`Character creation failed: Invalid name length - ${name}`);
            return res.status(400).json({ error: 'Character name must be between 2 and 50 characters' });
        }
        
        // Check if character name already exists for this user
        const existingChar = await pool.query(
            'SELECT character_id FROM characters WHERE user_id = $1 AND name = $2',
            [req.user.user_id, name]
        );
        
        if (existingChar.rows.length > 0) {
            logger.warn(`Character creation failed: Name already exists - ${name}`);
            return res.status(409).json({ error: 'You already have a character with this name' });
        }
        
        // Validate class (optional field)
        const validClasses = ['warrior', 'mage', 'archer', 'healer', 'priest'];
        const charClass = characterClass && validClasses.includes(characterClass.toLowerCase()) 
            ? characterClass.toLowerCase() 
            : 'warrior';
        
        // Create character
        const result = await pool.query(
            `INSERT INTO characters (user_id, name, class, level, x, y, z, health, mana) 
             VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) 
             RETURNING character_id, name, class, level, x, y, z, health, mana, created_at`,
            [req.user.user_id, name, charClass, 1, 0, 0, 0, 100, 100]
        );
        
        const character = result.rows[0];
        logger.info(`Character created successfully: ${name} (ID: ${character.character_id})`);
        
        res.status(201).json({
            message: 'Character created successfully',
            character
        });
        
    } catch (err) {
        logger.error('Create character error:', err.message);
        res.status(500).json({ error: 'Failed to create character' });
    }
});

app.get('/api/characters/:id', authenticateToken, async (req, res) => {
    try {
        const characterId = req.params.id;
        logger.debug(`Fetching character ${characterId} for user ${req.user.user_id}`);
        
        const result = await pool.query(
            'SELECT character_id, name, class, level, x, y, z, health, mana, created_at FROM characters WHERE character_id = $1 AND user_id = $2',
            [characterId, req.user.user_id]
        );
        
        if (result.rows.length === 0) {
            logger.warn(`Character not found: ${characterId} for user ${req.user.user_id}`);
            return res.status(404).json({ error: 'Character not found' });
        }
        
        logger.debug(`Character retrieved: ${result.rows[0].name}`);
        res.json({
            message: 'Character retrieved successfully',
            character: result.rows[0]
        });
        
    } catch (err) {
        logger.error('Get character error:', err.message);
        res.status(500).json({ error: 'Failed to retrieve character' });
    }
});

// Save character position
app.put('/api/characters/:id/position', authenticateToken, async (req, res) => {
    try {
        const characterId = req.params.id;
        const { x, y, z } = req.body;
        logger.debug(`Saving position for character ${characterId}: X=${x}, Y=${y}, Z=${z}`);
        
        // Validate coordinates
        if (typeof x !== 'number' || typeof y !== 'number' || typeof z !== 'number') {
            logger.warn(`Invalid coordinates for character ${characterId}: x=${x}, y=${y}, z=${z}`);
            return res.status(400).json({ error: 'Invalid coordinates. x, y, z must be numbers' });
        }
        
        // Verify character belongs to user
        const charCheck = await pool.query(
            'SELECT character_id FROM characters WHERE character_id = $1 AND user_id = $2',
            [characterId, req.user.user_id]
        );
        
        if (charCheck.rows.length === 0) {
            logger.warn(`Position save failed: Character not found - ${characterId}`);
            return res.status(404).json({ error: 'Character not found' });
        }
        
        // Update position
        await pool.query(
            'UPDATE characters SET x = $1, y = $2, z = $3 WHERE character_id = $4',
            [x, y, z, characterId]
        );
        
        logger.info(`Position saved for character ${characterId}: X=${x}, Y=${y}, Z=${z}`);
        res.json({
            message: 'Position saved successfully',
            position: { x, y, z }
        });
        
    } catch (err) {
        logger.error('Save position error:', err.message);
        res.status(500).json({ error: 'Failed to save position' });
    }
});

// Test endpoint - will be replaced with auth later
app.get('/api/test', (req, res) => {
  logger.debug('Test endpoint accessed');
  res.json({ message: 'Hello from MMO Server!' });
});

// Start server
app.listen(PORT, () => {
  logger.info(`MMO Server running on port ${PORT}`);
  logger.info(`Database: ${process.env.DB_NAME}`);
  logger.info(`Log level: ${LOG_LEVEL}`);
});
