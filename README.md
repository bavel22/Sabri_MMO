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
- [ ] Save character position on exit
- [ ] Load character position on spawn

### Next Steps
1. Save character position on exit
2. Load character position on spawn
3. Implement top-down movement controls
4. Add basic camera follow system

---

**Last Updated**: 2026-02-02  
**Version**: 0.2.0  
**Status**: Phase 1 - Character System Complete
