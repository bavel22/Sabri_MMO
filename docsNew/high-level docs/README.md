# Sabri MMO

A class-based action MMORPG built with Unreal Engine 5, Node.js, and PostgreSQL.

## Project Overview

**Game Type**: Class-based action MMORPG  
**Engine**: Unreal Engine 5 (Client)  
**Backend**: Node.js + Express (Server)  
**Database**: PostgreSQL  
**Target Scale**: 1000-5000 concurrent players  
**Timeline**: 6 months (MVP) → 12 months (Full release)  
**Team Size**: 1 developer (solo)  

## Technology Stack

### Client (UE5)
- **Engine**: Unreal Engine 5.3.2
- **Languages**: C++, Blueprint
- **Systems**: UMG UI, Niagara VFX, Control Rig Animation
- **Networking**: HTTP requests, WebSocket integration

### Server (Node.js)
- **Runtime**: Node.js 18.19.0 LTS
- **Framework**: Express.js 4.18.2
- **Real-time**: Socket.io 4.7.4
- **Database**: PostgreSQL 15.4
- **Cache**: Redis 7.2.3

### Development
- **Platform**: Windows 11 (Development) → Linux (Production)
- **Version Control**: Git
- **Containerization**: Docker
- **AI Assistance**: ChatGPT/GPT-4, GitHub Copilot

## Implemented Features

### Authentication System
- JWT-based authentication
- User login/registration endpoints
- Token validation middleware
- Secure password hashing with bcrypt

### Character Management
- FCharacterData struct (BlueprintType)
- MMOGameInstance with character storage
- HTTP endpoints: GetCharacters, CreateCharacter
- Character selection and persistence

### UI Widgets
- **WBP_LoginScreen**: Login screen with username/password input, error handling
- **WBP_CharacterSelect**: Main selection screen with scrollable list
- **WBP_CharacterEntry**: Individual character display row
- **WBP_CreateCharacter**: Character creation dialog with name/class selection
- Mouse cursor control and input mode switching

### Blueprint Integration
- BP_GameFlow with event bindings
- OnLoginSuccess → GetCharacters → Show UI
- OnCharacterListReceived → Populate list
- OnCharacterCreated → Refresh list
- IsAuthenticated check for level persistence
- Event binding sequence for clean initialization

### Character Position Persistence
- Auto-save position every 5 seconds while playing
- Load saved position on character spawn
- Fallback to Player Start for new characters (0,0,0 check)
- Server endpoint: PUT /api/characters/:id/position
- Database storage for X, Y, Z coordinates

### Server Logging System
- Comprehensive logging with DEBUG/INFO/WARN/ERROR levels
- File output to server/logs/server.log
- Timestamped log entries
- HTTP request logging middleware
- Configurable log level via environment variable

### Top-Down Movement System
- **Click-to-Move**: Left-click on ground to move character
- **WASD Movement**: Traditional keyboard controls
- **Movement Cancel**: WASD input cancels click-to-move navigation
- **Character Rotation**: Character smoothly faces movement direction
- **NavMesh Pathfinding**: Automatic obstacle avoidance and pathfinding

### Camera System
- **Spring Arm Architecture**: Camera boom with independent rotation
- **Right-Click Rotation**: Hold right-click to rotate camera around character
- **Mouse Scroll Zoom**: Zoom in/out with scroll wheel (200-1500 units)
- **Fixed Camera**: Camera stays stationary when player moves/turns
- **Lag/Smoothing**: Camera follows with smooth interpolation

### Game World Integration
- Enter World button functionality complete
- Level loading with Open Level node
- Character spawn at Player Start location
- Character possession on spawn
- GameInstance persistence across level loads
- Top-down camera and mouse controls ready

### Database
- PostgreSQL characters table
- JWT-authenticated API endpoints
- Character data: id, name, class, level

## Project Structure

```
Sabri_MMO/
├── README.md                    # This file
├── MMO_Development_Plan.md      # Comprehensive development plan
├── Technology_Stack.md          # Technology stack documentation
├── Project_File_Structure.md    # Complete file structure guide
├── .gitignore                   # Git ignore file
├── docs/                        # Documentation
├── client/                      # UE5 client project
├── server/                      # Node.js server project
├── database/                    # Database files and migrations
├── assets/                      # Game assets and resources
├── tools/                       # Development tools and utilities
├── tests/                       # Test files and test data
├── scripts/                     # Build and deployment scripts
├── config/                      # Configuration files
└── deployment/                  # Deployment configurations
```

## Development Phases

### Phase 0: Pre-Development (Week 0)
- Project planning and setup
- Environment configuration
- Learning and resource gathering

### Phase 1: Foundation (Weeks 1-4)
- Database and backend setup
- UE5 client foundation
- Basic UI and authentication
- Core combat system

### Phase 2: World & Content (Weeks 5-8)
- Zone system and world building
- NPCs and quest system
- Inventory and item system
- Advanced features and polish

### Phase 3: Social & Economy (Weeks 9-12)
- Chat and communication
- Guild system
- Economy and trading
- Advanced content and polish

### Phase 4: Production & Launch (Weeks 13-16)
- Production infrastructure
- Analytics and monitoring
- Community and launch preparation
- Launch and post-launch support

## Getting Started

### Prerequisites

- **Windows 11** (Development)
- **Visual Studio 2022** with C++ development tools
- **Unreal Engine 5.3.2**
- **Node.js 18.19.0 LTS**
- **PostgreSQL 15.4**
- **Git**

### Server Setup

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd Sabri_MMO
   ```

2. **Set up the database**
   ```bash
   # Install PostgreSQL and Redis
   # Create game database
   # Run migrations from database/migrations/
   # Load seed data from database/seeds/
   ```

3. **Set up the server**
   ```bash
   cd server
   npm install
   cp .env.example .env
   # Configure .env with your database settings
   npm start
   ```

4. **Set up the client**
   ```bash
   # Open UE5 project in client/
   # Configure project settings
   # Build and run
   ```

### Environment Variables

Create a `.env` file in the `server/` directory:

```env
# Database
DATABASE_URL=postgresql://username:password@localhost:5432/sabri_mmo
REDIS_URL=redis://localhost:6379

# Authentication
JWT_SECRET=your-secret-key
JWT_EXPIRES_IN=24h

# Server
PORT=3001
NODE_ENV=development

# Client
CLIENT_URL=http://localhost:3000
```

## Development Workflow

### Daily Routine
1. **Morning Standup**: Update progress and set priorities
2. **Development**: Work on current sprint tasks
3. **Testing**: Test new features and fix bugs
4. **Evening Wrap-up**: Document progress and plan next day

### Git Workflow
```bash
# Create feature branch
git checkout -b feature/feature-name

# Make changes and commit
git add .
git commit -m "feat: add feature description"

# Push and create pull request
git push origin feature/feature-name
```

### AI Assistance
- Use AI for code generation (95% success rate for Node.js)
- Review and test all AI-generated code
- Document AI workflows and prompts
- Maintain code quality standards

## Testing

### Backend Tests
```bash
cd server
npm test
npm run test:coverage
npm run test:integration
```

### Load Testing
```bash
cd tools
node load_tester.js
```

### Database Tests
```bash
cd database
psql -d sabri_mmo -f tests/test_queries.sql
```

## Deployment

### Development
```bash
docker-compose -f docker-compose.dev.yml up
```

### Staging
```bash
docker-compose -f docker-compose.staging.yml up
```

### Production
```bash
docker-compose -f docker-compose.prod.yml up
```

## Monitoring

### Application Metrics
- **Prometheus**: Metrics collection
- **Grafana**: Visualization
- **Custom Dashboard**: Game-specific metrics

### Error Tracking
- **Sentry**: Error monitoring
- **Custom Logging**: Application logs
- **Performance Monitoring**: Response times

## Contributing

This is a solo development project. For future collaboration:

1. Follow the established coding standards
2. Write comprehensive tests
3. Update documentation
4. Use semantic versioning
5. Follow the Git workflow

## License

This project is proprietary. All rights reserved.

## Support

For questions or support:
- Check the documentation in `docs/`
- Review the development plan in `MMO_Development_Plan.md`
- Refer to the technology stack in `Technology_Stack.md`

## Progress

### Current Status: Phase 1 - Foundation (In Progress)
- [x] Project structure created
- [x] Technology stack defined
- [x] Git repository configured
- [x] Database schema implemented
- [x] Authentication system working
- [x] Character Select UI complete
- [x] Character creation flow complete
- [x] Enter World functionality complete
- [x] Game world integration complete
- [x] Character spawn and possession working
- [x] Save character position on exit
- [x] Load character position on spawn
- [x] Server logging system with levels
- [x] Login screen widget (WBP_LoginScreen)
- [x] Login widget blueprint integration
- [x] Top-down click-to-move controls
- [x] WASD movement with click-to-move cancel
- [x] Spring Arm camera system
- [x] Independent camera rotation (right-click)
- [x] Mouse scroll zoom (min/max)
- [x] Character faces movement direction
- [x] NavMesh pathfinding
- [x] Movement acceleration for smooth animations

### Phase 2: Real-Time Multiplayer (Complete)
- [x] Socket.io server integration
- [x] Socket.io UE5 client
- [x] Redis player position cache
- [x] Server tick loop (20-30Hz)
- [x] Player position broadcast system
- [x] Spawn other players in world
- [x] Smooth interpolation for remote players
- [x] Remote player walk/run animations (CharacterMovement-based)
- [x] Correct initial spawn position from database
- [x] Player disconnect handling
- [ ] Client-side prediction
- [ ] Server reconciliation

### Phase 3: Combat System (Complete)
- [x] Server-side combat events (combat:attack, combat:damage, combat:death, combat:respawn)
- [x] Server attack validation (range, cooldown, dead-check, self-attack prevention)
- [x] Health/mana tracking from database on join
- [x] Health sync broadcast (combat:health_update) to all players
- [x] Game HUD (WBP_GameHUD) with HP/MP bars and player name
- [x] RO-style auto-attack (click target → path → auto-attack at ASPD intervals)
- [x] Attack speed system (ASPD 0-190 scale, derived from AGI+DEX)
- [x] Range check with tolerance (MELEE_RANGE: 100, RANGE_TOLERANCE: 50)
- [x] Combat damage with positions for remote player rotation
- [x] Player kill crash fix (target_lost + death race condition resolved)
- [x] Death/respawn with full HP restore and teleport to spawn
- [x] Kill messages in COMBAT chat channel
- [ ] Combat animations (attack montage, hit reaction)

### Socket.io Real-Time Multiplayer
- Socket.io server with Node.js
- UE5 SocketIOClient plugin integration
- BP_SocketManager actor for client connection
- player:join event (characterId + token authentication)
- player:position event (30Hz position updates)
- player:moved broadcast (other player updates)
- player:left event (disconnect handling)
- BP_OtherPlayerCharacter with CharacterMovement-based movement
- BP_OtherPlayerManager for player tracking with correct spawn positions
- Real-time position sync working
- Socket.io event binding in Blueprints
- Tested with 5 concurrent players
- Player name tags above characters (WBP_PlayerNameTag)
- Initial position broadcast from database on player:join
- Remote players animate walk/run via CharacterMovement + ABP_unarmed

### Chat System
- Real-time global chat between all players
- WBP_ChatWidget UI with scrollable message history
- WBP_ChatMessageLine for individual messages
- Enter key and Send button support
- Player names displayed with messages
- Expandable architecture for multiple channels (ZONE, PARTY, GUILD, TELL, COMBAT)
- Server-side chat:message and chat:receive events
- JSON communication protocol for chat data

### Stat System (RO-Style)
- 6 base stats: STR, AGI, VIT, INT, DEX, LUK
- Derived stat calculations (statusATK, ASPD, maxHP, flee, hit, critical, etc.)
- Stat point allocation via WBP_StatAllocation widget
- Stats loaded from database on join, saved on disconnect
- player:request_stats event for on-demand stat loading
- C++ USTRUCTs: FPlayerBaseStats, FPlayerDerivedStats in PlayerStats.h

### Enemy Combat System (Complete)
- 6 enemy templates: Blobby, Hoplet, Crawlid, Shroomkin, Buzzer, Mosswort
- 12 enemy spawn locations with configurable wander radius
- Server-side enemy spawning, health tracking, death, and respawn
- Player vs enemy auto-attack with ASPD-based timing
- Enemy health bars, name tags, hover indicators (Blueprint)
- BP_EnemyCharacter actor + BP_EnemyManager singleton
- Enemy death: collision disabled, mesh hidden, respawn timer
- Multi-player enemy kill crash fix (no target_lost on death, only enemy:death)
- Enemy wandering AI (RO-style): random wander within spawn radius, 3-8s pauses
- enemy:spawn, enemy:move, enemy:death, enemy:health_update events

### Combat System (Server-Side)
- Server-authoritative combat with attack validation
- RO-style auto-attack: click once → path → auto-attack loop at ASPD intervals
- Supports both player targets (targetCharacterId) and enemy targets (targetEnemyId)
- combat:attack, combat:stop_attack, combat:damage, combat:death, combat:respawn events
- combat:auto_attack_started, combat:auto_attack_stopped, combat:target_lost events
- Range check (100 units melee) with tolerance (50 units padding)
- ASPD timing (default 180 = 1 hit/sec, cap 190 = 2/sec)
- Combat tick loop runs every 50ms, processes all active auto-attackers
- Health/mana loaded from database, tracked in memory, synced to all clients
- Death broadcast with kill message in COMBAT chat channel
- Respawn restores full HP/MP, teleports to spawn point (0,0,300), saves to database
- WBP_GameHUD with HP bar (red), MP bar (blue), player name display
- Comprehensive logging: [RECV], [SEND], [BROADCAST], [COMBAT] for all events
- parseInt() fix for all client-supplied IDs (prevents type mismatch bugs)

### Items & Inventory System (Server-Side Complete)
- 16 base items: 5 consumables, 8 loot/etc, 5 weapons, 3 armor
- PostgreSQL `items` table (definitions) + `character_inventory` table (per-character)
- Auto-create tables and seed items on server startup
- Enemy drop tables on all 6 enemy templates (chance-based rolling)
- Loot goes to killer on enemy death via `loot:drop` event
- Inventory Socket.io events: load, use consumable, equip/unequip, drop/discard
- Consumable use: server-authoritative HP/SP restoration
- Equipment: weapon ATK updates derived stats, armor DEF (future integration)
- Equipped weapon ATK loaded from DB on player join
- Stackable items with max stack limits

### Known Issues
- None currently — all critical bugs resolved (see docs/Bug_Fix_Notes.md)

---

**Last Updated**: 2026-02-11
**Version**: 1.1.0
**Status**: Items & Inventory Server-Side Complete, Blueprint UI Next
