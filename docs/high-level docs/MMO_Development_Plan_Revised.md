# MMO Development Plan - Solo Dev Revision

## Philosophy: Build a Working Foundation, Expand Later

**Original plan tried to build a full MMORPG in 6 months. This revised plan focuses on a playable multiplayer prototype you can actually finish and iterate on.**

---

## Revised Target Specifications

```
Game Type: Multiplayer action RPG (NOT massive yet)
Engine: Unreal Engine 5 (Client)
Backend: Node.js + Express (Server)
Database: PostgreSQL + Redis
Target Scale: 10-50 concurrent players (MVP) → 100-200 (v1.0)
Timeline: 6 months (MVP) → 12 months (Early Access)
Team Size: 1 developer (solo)
Platform: Windows 11 (Development) → Steam Early Access
```

---

## Phase 0: Foundation (Week 1)

### Goals
- Get UE5 talking to your server
- One working login endpoint
- One test zone with movement

### Tasks
**Day 1-2: Project Setup**
- Install PostgreSQL, Redis, Node.js
- Create Git repo with folders: `/server`, `/client`, `/docs`
- Create basic Express server with health check

**Day 3-4: Database Schema (Minimal)**
```sql
-- Only what you need for MVP
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE characters (
    character_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id),
    name VARCHAR(50) NOT NULL,
    class VARCHAR(20) DEFAULT 'warrior',
    level INTEGER DEFAULT 1,
    x FLOAT DEFAULT 0,
    y FLOAT DEFAULT 0,
    z FLOAT DEFAULT 0,
    health INTEGER DEFAULT 100,
    mana INTEGER DEFAULT 100,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Day 5-7: UE5 Basic Setup**
- New UE5 Third Person project
- C++ HttpManager class (GET/POST with JSON)
- Login widget → calls server → stores JWT
- Display "Login Success" or error

---

## Phase 1: Core Loop (Months 2-3)

### Goal: Player can login, move around, see other players

**Week 2-3: Character System**
- Server: Register/login endpoints with JWT
- Server: Create character endpoint
- UE5: Character select screen (list characters from API)
- UE5: Spawn into test level

**Week 4-6: Real-time Movement (The Hard Part)**
```
Architecture: Authoritative Server
- Client sends inputs to server ("W pressed", "W released")
- Server calculates position, broadcasts to all players in zone
- Client predicts movement locally for responsiveness
- Server position is truth - client reconciles if wrong

Tech:
- Socket.io for real-time (not REST polling)
- Server runs tick loop (20-30Hz)
- Redis stores player positions (fast reads/writes)
- PostgreSQL saves positions on logout only
```

**Week 7-8: Basic Combat**
- Simple hitscan or projectile
- Server validates: "Can player X hit player Y?" (range check, LoS)
- Health updates broadcast to nearby players
- Death = respawn at spawn point

**MVP Deliverable:**
- Login → Character Select → Enter World
- See other players moving in real-time
- Attack and kill each other
- Basic UI: HP bar, mana bar, chat

---

## Phase 2: Content (Months 4-5)

### Goal: Make it feel like a game, not a tech demo

**Asset Strategy (CRITICAL - You're Solo)**
```
DO NOT:
- Model characters from scratch (100+ hours each)
- Animate walk cycles (learn MotionBuilder? No.)
- Create textures, VFX, sounds

DO:
- UE5 Mannequin (free, rigged, animated)
- Mixamo animations (free, automatic retargeting)
- Quixel Megascans (free with UE5, environments)
- Marketplace asset packs ($30-100, complete systems)
- Freesound.org for SFX

Budget: $200-500 for core asset packs
```

**Month 4: World Building**
- One 500m x 500m zone (not multiple zones yet)
- Procedural terrain or Marketplace environment pack
- 3 enemy types (re-use same mesh, different materials/scale)
- 5 quests (kill X, collect Y, talk to Z)

**Month 5: Progression**
- Experience/leveling system
- 10 abilities total (2 per class, 5 classes)
- Basic inventory (no crafting yet)
- 20 items (weapons/armor with stat modifiers)

---

## Phase 3: Polish & Early Access (Month 6+)

### Goal: Steam-worthy but minimal

**Week 21-22: Monetization Setup**
```
Options:
1. One-time purchase ($10-20) - Simple, guaranteed revenue
2. Cosmetic shop - Requires more content, ongoing work
3. Subscription - Hard to justify with limited content

Recommendation: One-time purchase + cosmetic DLC later
```

**Week 23-24: Steam Integration**
- Steam SDK integration
- Achievements (10-15 simple ones)
- Steam multiplayer (relay for NAT traversal)

**Week 25-26: Launch Prep**
- Server hosting (AWS/Hetzner/OVH)
- Monitoring (Datadog/ Grafana + alerts)
- Backup automation
- Beta with friends (10-20 people)

---

## Technical Architecture (Revised)

### Authoritative Server Flow
```
1. Client sends: {"type": "move", "direction": "forward", "delta": 0.016}
2. Server validates: Is player allowed to move? (not stunned, alive, etc.)
3. Server calculates: New position based on speed + delta
4. Server broadcasts: {"type": "player_moved", "id": 123, "pos": [x,y,z]}
5. Client receives: Updates other players' positions
6. Client reconciles: If server position differs from prediction, snap smoothly
```

### Database Strategy
```
Redis (Hot data - fast access):
- Player positions (updates 20-30x/second)
- Session tokens
- Online player list per zone

PostgreSQL (Persistent data):
- User accounts
- Character data (saved on logout)
- Inventory
- Quest progress
```

### Instancing Strategy (Scalability)
```
Problem: 50+ players in one zone = lag
Solution: Channel-based instancing

- Each zone has multiple "channels" (copies)
- Players distributed across channels
- Friends can switch to same channel
- No hand-written sharding code needed
```

---

## Revised Success Metrics

| Milestone | Date | Criteria |
|-----------|------|----------|
| Tech Demo | Month 1 | Login, move, see other players |
| Combat Demo | Month 2 | Fight and kill other players |
| Alpha | Month 3 | 10 quests, 5 abilities, 1 zone |
| Beta | Month 4 | 50 concurrent players stable |
| Early Access | Month 6 | On Steam, $10 price point |
| Full Release | Month 12 | 3 zones, 100+ items, guilds |

---

## What Was Cut (For Now)

| Feature | Original | Revised | Why |
|---------|----------|---------|-----|
| Zones | 3+ at launch | 1 at launch | Assets take forever |
| Quests | 20+ | 10 | Quality over quantity |
| Items | 100+ | 20 | Expand post-launch |
| Guilds | Month 3 | Month 9+ | Social features last |
| Housing | Month 5 | Post-launch | Nice to have |
| PvP rankings | Month 5 | Post-launch | Needs population |
| Economy | Month 5 | Post-launch | Requires playerbase |
| 1000+ players | Month 6 | Year 2 | Scale with revenue |

---

## Budget Estimate

```
One-time costs:
- Asset packs (Marketplace): $300-500
- Steam fee: $100
- Domain/hosting setup: $50

Monthly costs (50-100 players):
- Server (VPS): $50-100
- Database hosting: $20-50
- Monitoring: $20
Total: ~$100-170/month

Break-even: Sell 20-30 copies/month at $10
```

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Run out of time | Cut features, not quality. Launch with 1 zone working > 3 zones broken. |
| Can't make assets | Budget $300 for Marketplace. Use free Mixamo/Quixel. |
| Server costs | Start with $50/month VPS. Scale only when making money. |
| Nobody plays | Start Discord/community NOW. Build with players, not in isolation. |
| Burnout | 6 months is long for solo dev. Take weekends off. |

---

## Next Steps (Do Today)

1. **Create project folders** and Git repo
2. **Install dependencies**: PostgreSQL, Redis, Node.js
3. **Write "Hello World"** Express endpoint
4. **Test HTTP** from UE5 to your server
5. **Celebrate** - you just proved the stack works

---

**Bottom line: Build less, but make it work. You can always add zones and features. You can't recover from a half-finished broken game.**
