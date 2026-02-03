# Server Logging System

## Overview

Comprehensive logging infrastructure for the Node.js server with multiple log levels, file output, and HTTP request tracking. Essential for debugging, monitoring, and auditing server operations.

## Features

- **4 Log Levels**: DEBUG, INFO, WARN, ERROR
- **Dual Output**: Console and file
- **Timestamped Entries**: ISO format with millisecond precision
- **Configurable**: Log level via environment variable
- **HTTP Request Logging**: Automatic request/response tracking

## Log Levels

| Level | Value | Usage |
|-------|-------|-------|
| DEBUG | 0 | Detailed debugging information |
| INFO | 1 | General operational information |
| WARN | 2 | Warning conditions, validation failures |
| ERROR | 3 | Error conditions requiring attention |

## Implementation

### Logger Object

```javascript
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
```

### File Setup

```javascript
const fs = require('fs');
const path = require('path');

const logsDir = path.join(__dirname, '..', 'logs');
if (!fs.existsSync(logsDir)) {
    fs.mkdirSync(logsDir, { recursive: true });
}
const logFile = fs.createWriteStream(
    path.join(logsDir, 'server.log'), 
    { flags: 'a' }
);
```

### HTTP Request Middleware

```javascript
app.use((req, res, next) => {
    logger.info(`${req.method} ${req.url} - ${req.ip}`);
    next();
});
```

## Log Output Format

### Example Log Entries

```
[2026-02-03T01:31:24.310Z] [INFO] MMO Server running on port 3001
[2026-02-03T01:31:24.311Z] [INFO] Database: sabri_mmo
[2026-02-03T01:31:24.311Z] [INFO] Log level: INFO
[2026-02-03T01:32:15.123Z] [INFO] GET /health - ::1
[2026-02-03T01:35:45.456Z] [INFO] POST /api/auth/login - ::1
[2026-02-03T01:35:45.789Z] [INFO] Login successful: testplayer (ID: 2)
[2026-02-03T01:35:50.123Z] [WARN] Login failed: Invalid password - baduser
[2026-02-03T01:36:00.456Z] [ERROR] Database connection error: Connection refused
[2026-02-03T01:40:10.789Z] [INFO] Position saved for character 10: X=123.45, Y=67.89, Z=0.00
```

## Endpoint Logging

### Authentication Endpoints

**Register:**
```javascript
logger.info(`Registration attempt for username: ${username}`);
// ... validation ...
if (existingUser.rows.length > 0) {
    logger.warn(`Registration failed: Username or email already exists - ${username}`);
}
// ... success ...
logger.info(`User registered successfully: ${username} (ID: ${user.user_id})`);
```

**Login:**
```javascript
logger.info(`Login attempt for username: ${username}`);
// ... validation ...
if (result.rows.length === 0) {
    logger.warn(`Login failed: User not found - ${username}`);
}
if (!validPassword) {
    logger.warn(`Login failed: Invalid password - ${username}`);
}
// ... success ...
logger.info(`Login successful: ${username} (ID: ${user.user_id})`);
```

### Character Endpoints

**Get Characters:**
```javascript
logger.debug(`Fetching characters for user ID: ${req.user.user_id}`);
// ... query ...
logger.info(`Retrieved ${result.rows.length} characters for user ${req.user.user_id}`);
```

**Create Character:**
```javascript
logger.info(`Character creation attempt: ${name} (class: ${characterClass}) for user ${req.user.user_id}`);
// ... validation ...
logger.warn(`Character creation failed: Name already exists - ${name}`);
// ... success ...
logger.info(`Character created successfully: ${name} (ID: ${character.character_id})`);
```

**Save Position:**
```javascript
logger.debug(`Saving position for character ${characterId}: X=${x}, Y=${y}, Z=${z}`);
// ... validation ...
logger.warn(`Invalid coordinates for character ${characterId}: x=${x}, y=${y}, z=${z}`);
// ... success ...
logger.info(`Position saved for character ${characterId}: X=${x}, Y=${y}, Z=${z}`);
```

## Configuration

### Environment Variable

```env
LOG_LEVEL=DEBUG
```

Options:
- `DEBUG` - All messages
- `INFO` - Info, Warn, Error (default)
- `WARN` - Warn, Error only
- `ERROR` - Errors only

### Production Recommendations

**Development:**
```env
LOG_LEVEL=DEBUG
```

**Production:**
```env
LOG_LEVEL=INFO
```

**Minimal:**
```env
LOG_LEVEL=WARN
```

## Log File Management

### Location
```
server/
├── logs/
│   └── server.log
├── src/
│   └── index.js
```

### File Rotation (Future Enhancement)

```javascript
// Daily rotation example
const date = new Date().toISOString().split('T')[0];
const logFile = fs.createWriteStream(
    path.join(logsDir, `server-${date}.log`), 
    { flags: 'a' }
);
```

### Viewing Logs

**Real-time:**
```bash
tail -f server/logs/server.log
```

**Search:**
```bash
grep "ERROR" server/logs/server.log
grep "Login successful" server/logs/server.log
```

**Last 100 lines:**
```bash
tail -n 100 server/logs/server.log
```

## Benefits

### Debugging
- Trace request flow through system
- Identify error locations quickly
- Reproduce issues from log history

### Monitoring
- Track authentication success rates
- Monitor database performance
- Detect unusual activity patterns

### Auditing
- Security event tracking
- User action history
- Compliance requirements

## Integration with Server Start

```javascript
app.listen(PORT, () => {
    logger.info(`MMO Server running on port ${PORT}`);
    logger.info(`Database: ${process.env.DB_NAME}`);
    logger.info(`Log level: ${LOG_LEVEL}`);
});
```

## Files

- `server/src/index.js` - Logger implementation
- `server/logs/server.log` - Log output (auto-created)

## Batch File Support

The `start-server.bat` maintains the console window open to show logs in real-time:

```batch
@echo off
taskkill /F /IM node.exe 2>nul
cd /d "%~dp0"
cmd /k "node src/index.js"
```

Logs appear both in console and in `server/logs/server.log`.
