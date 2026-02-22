# Development Environment Setup Guide

## Prerequisites

| Software | Version | Purpose |
|----------|---------|---------|
| **Windows 11** | — | Development OS |
| **Visual Studio 2022** | Community+ | C++ compilation, UE5 integration |
| **Unreal Engine 5.7** | 5.7 | Game client |
| **Node.js** | 18+ LTS | Server runtime |
| **PostgreSQL** | 15.4+ | Database |
| **Redis** | 7.2+ | Cache |
| **Git** | 2.40+ | Version control |

## Step 1: Clone the Repository

```bash
git clone <repository-url> C:\Sabri_MMO
```

## Step 2: Database Setup

### Install PostgreSQL
1. Download from https://www.postgresql.org/download/windows/
2. Install with default settings, set password for `postgres` user
3. Verify: `psql -U postgres -c "SELECT version();"`

### Create Database
```sql
-- In psql or pgAdmin:
CREATE DATABASE sabri_mmo;
```

### Run Schema
```bash
psql -U postgres -d sabri_mmo -f C:\Sabri_MMO\database\init.sql
```

This creates:
- `users` table
- `characters` table  
- `items` table (with 16 seed items)
- `character_inventory` table
- Required indexes

**Note**: The server also auto-creates/migrates tables on startup, so running `init.sql` is optional but recommended.

### Default Credentials (Development Only)
```
Host: localhost
Port: 5432
Database: sabri_mmo
User: postgres
Password: goku22
```

## Step 3: Redis Setup

### Install Redis (Windows)
- Download from https://github.com/microsoftarchive/redis/releases
- Or use WSL: `sudo apt install redis-server`

### Start Redis
```bash
redis-server
```

### Verify
```bash
redis-cli ping
# Expected: PONG
```

## Step 4: Server Setup

### Install Dependencies
```bash
cd C:\Sabri_MMO\server
npm install
```

### Configure Environment
The `.env` file should already exist at `server/.env`:
```env
PORT=3001
DB_HOST=localhost
DB_PORT=5432
DB_NAME=sabri_mmo
DB_USER=postgres
DB_PASSWORD=goku22
JWT_SECRET=your-super-secret-jwt-key-change-this-in-production-2026
```

### Start Server
```bash
# Option A: Batch file
cd server
start-server.bat

# Option B: Manual
cd server
node src/index.js

# Option C: Dev mode (auto-restart on changes)
cd server
npm run dev
```

### Verify Server
```bash
curl http://localhost:3001/health
```

Expected:
```json
{"status":"OK","timestamp":"...","message":"Server is running and connected to database"}
```

## Step 5: UE5 Client Setup

### Open Project
1. Launch Epic Games Launcher → Unreal Engine 5.7
2. Browse to `C:\Sabri_MMO\client\SabriMMO\SabriMMO.uproject`
3. Wait for shader compilation (first launch takes several minutes)

### Install Required Plugins
The following plugins should be enabled in the `.uproject`:
- **StateTree** (built-in)
- **GameplayStateTree** (built-in)
- **ModelingToolsEditorMode** (built-in)
- **VisualStudioTools** (Marketplace)
- **RemoteControl** (built-in)

### SocketIOClient Plugin
The project uses the **SocketIOClient** plugin for UE5. If not already installed:
1. Download from UE Marketplace or GitHub
2. Place in `client/SabriMMO/Plugins/SocketIOClient/`
3. Rebuild the project

### Build C++ Code
1. Open Visual Studio from UE5 (Tools → Open Visual Studio)
2. Build Solution (Ctrl+Shift+B)
3. Or: In UE5 Editor, click Compile button in toolbar

### Play in Editor
1. Open the game level (likely `ThirdPersonMap` or main game map)
2. Ensure server is running on port 3001
3. Click Play (Alt+P)
4. Login screen should appear

### Test Credentials
```
Username: testplayer
Password: password123
```

Create test users via:
```bash
cd C:\Sabri_MMO
node database/create_test_users.js
```

## Step 6: Create Test Users (Optional)

### Via SQL
```bash
psql -U postgres -d sabri_mmo -f database/create_test_users.sql
```

### Via Script
```bash
cd C:\Sabri_MMO
node database/create_test_users.js
```

### Via API
```bash
curl -X POST http://localhost:3001/api/auth/register ^
  -H "Content-Type: application/json" ^
  -d "{\"username\":\"testplayer\",\"email\":\"test@test.com\",\"password\":\"password123\"}"
```

## Development Workflow

```
1. Start PostgreSQL service (if not running)
2. Start Redis: redis-server
3. Start Server: cd server && node src/index.js
4. Open UE5 Editor → Load project
5. Click Play to test
6. Monitor server/logs/server.log for debugging
7. Use pgAdmin for database inspection
```

## Useful Commands

### Server
```bash
# Kill all Node processes
taskkill /F /IM node.exe

# Check port 3001
netstat -ano | findstr :3001

# View server logs
type server\logs\server.log

# Start with debug logging
set LOG_LEVEL=DEBUG && node server/src/index.js
```

### Database
```bash
# Connect to database
psql -U postgres -d sabri_mmo

# View all users
psql -U postgres -d sabri_mmo -c "SELECT * FROM users;"

# View all characters
psql -U postgres -d sabri_mmo -c "SELECT character_id, name, class, level FROM characters;"

# View items
psql -U postgres -d sabri_mmo -c "SELECT item_id, name, item_type FROM items ORDER BY item_id;"
```

### Redis
```bash
# View all cached positions
redis-cli KEYS 'player:*'

# View specific player position
redis-cli GET 'player:1:position'

# Clear all cache
redis-cli FLUSHALL
```

## Troubleshooting

### Server won't start
- Check if port 3001 is already in use: `netstat -ano | findstr :3001`
- Kill existing Node processes: `taskkill /F /IM node.exe`
- Verify PostgreSQL is running: `psql -U postgres -c "SELECT 1;"`
- Verify Redis is running: `redis-cli ping`
- Check `.env` file exists and has correct credentials

### UE5 can't connect to server
- Verify server is running: `curl http://localhost:3001/health`
- Check Windows Firewall allows port 3001
- Check UE5 Output Log for HTTP/Socket errors
- Ensure SocketIOClient plugin is installed and enabled

### Database connection errors
- Verify PostgreSQL service: `net start postgresql-x64-15` (Windows)
- Check `server/.env` credentials match PostgreSQL setup
- Ensure `sabri_mmo` database exists

### C++ compilation errors
- UE5 5.7 + MSVC 14.38: May need `FormatStringSan.h` patch (see memory notes)
- `HttpManager.h` shadowing: File renamed to `MMOHttpManager.h` to avoid engine collision

---

**Last Updated**: 2026-02-17
