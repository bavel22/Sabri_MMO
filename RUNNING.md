# Running the Sabri MMO Project

Quick start guide for running the development environment.

## Prerequisites

- **Node.js** (v18+) - [Download](https://nodejs.org/)
- **PostgreSQL** (v14+) - [Download](https://www.postgresql.org/download/)
- **Unreal Engine 5.4+** - Epic Games Launcher
- **Git** - [Download](https://git-scm.com/)

---

## 1. Start the Database (PostgreSQL)

### Windows
```bash
# Start PostgreSQL service
net start postgresql-x64-15

# Or via Services app
services.msc → Find "PostgreSQL" → Start
```

### Verify Connection
```bash
psql -U postgres -d sabri_mmo -c "SELECT version();"
```

Default credentials (development only):
- **User:** postgres
- **Password:** goku22
- **Database:** sabri_mmo
- **Port:** 5432

---

## 2. Start the Server

### Option A: Batch File (Windows)
```bash
cd server
start-server.bat
```

### Option B: Manual Start
```bash
cd server
npm install        # First time only
node src/index.js
```

### Verify Server is Running
```bash
curl http://localhost:3001/health
```

Expected response:
```json
{"status":"OK","timestamp":"...","message":"Server is running"}
```

### Server Log Location
```
server/logs/server.log
```

---

## 3. Open pgAdmin 4 (Database Admin)

### Launch
- Start Menu → pgAdmin 4
- Or: `C:\Program Files\PostgreSQL\15\pgAdmin 4\bin\pgAdmin4.exe`

### Connect to Database
1. Click "Add New Server"
2. **General Tab:**
   - Name: `Sabri MMO`
3. **Connection Tab:**
   - Host: `localhost`
   - Port: `5432`
   - Database: `sabri_mmo`
   - Username: `postgres`
   - Password: `goku22`
   - Save password: ✓

### Useful Queries
```sql
-- View all users
SELECT * FROM users;

-- View all characters
SELECT * FROM characters;

-- View characters with owner
SELECT c.character_id, c.name, u.username 
FROM characters c 
JOIN users u ON c.user_id = u.user_id;
```

---

## 4. Open Unreal Engine Project

1. Launch Epic Games Launcher
2. Open Unreal Engine 5.4
3. Select "Browse" and navigate to:
   ```
   client/SabriMMO/SabriMMO.uproject
   ```
4. Click Open

### Play In Editor (PIE)
1. Open `ThirdPersonMap`
2. Click ▶️ "Play" button
3. Login screen appears
4. Test credentials:
   - Username: `testplayer`
   - Password: `password123`

---

## Project URLs

| Service | URL | Purpose |
|---------|-----|---------|
| Game Client | N/A (UE5 Editor) | Play the game |
| Game Server | http://localhost:3001 | API endpoints |
| Health Check | http://localhost:3001/health | Verify server |
| pgAdmin 4 | http://localhost:5050 (or app) | Database admin |

---

## File Structure

```
Sabri_MMO/
├── server/              # Node.js backend
│   ├── src/
│   │   └── index.js     # Main server file
│   ├── logs/            # Server logs
│   └── start-server.bat # Quick start script
├── client/              # UE5 project
│   └── SabriMMO/
│       ├── SabriMMO.uproject
│       └── Content/
├── database/
│   └── init.sql         # Database schema
├── docs/                # Documentation
└── README.md            # Project overview
```

---

## Common Issues

### Server won't start
```bash
# Check if port 3001 is in use
netstat -ano | findstr :3001

# Kill existing node processes
taskkill /F /IM node.exe
```

### Database connection failed
- Verify PostgreSQL service is running
- Check credentials in `server/.env`
- Ensure database `sabri_mmo` exists

### UE5 can't connect
- Verify server is running: `curl http://localhost:3001/health`
- Check Windows Firewall (allow port 3001)
- Check UE5 Output Log for errors

---

## Development Workflow

1. **Start PostgreSQL** (if not running)
2. **Start Server** (`server/start-server.bat`)
3. **Open UE5** (load `SabriMMO.uproject`)
4. **Click Play** to test
5. **Use pgAdmin** to inspect data as needed

---

## Environment Variables (server/.env)

```env
PORT=3001
DB_HOST=localhost
DB_PORT=5432
DB_NAME=sabri_mmo
DB_USER=postgres
DB_PASSWORD=goku22
JWT_SECRET=your_secret_key_here
LOG_LEVEL=INFO
```

---

## Documentation

- `README.md` - Project overview and features
- `docs/` - Detailed feature documentation
- `docs/Server_API.md` - API reference
- `docs/Database_Schema.md` - Database documentation
