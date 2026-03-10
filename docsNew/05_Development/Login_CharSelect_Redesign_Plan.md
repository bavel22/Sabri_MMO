> **COMPLETED** — This redesign plan has been fully implemented. The pure C++ Slate login flow (`LoginFlowSubsystem` + 5 Slate widgets) is live and replaces the old Blueprint-based `BP_GameFlow` + `WBP_LoginScreen` system. See `docsNew/02_Client_Side/C++_Code/` for current subsystem docs.

# Login, Character Select & Game Entry — Complete Redesign Plan (Historical)

## Table of Contents
1. [Executive Summary](#1-executive-summary)
2. [Current State Analysis](#2-current-state-analysis)
3. [Gaps & Issues](#3-gaps--issues)
4. [Design Decisions](#4-design-decisions)
5. [Target Architecture](#5-target-architecture)
6. [Implementation Phases](#6-implementation-phases)
7. [Phase 1: Database & Server Foundation](#phase-1-database--server-foundation)
8. [Phase 2: C++ Infrastructure](#phase-2-c-infrastructure)
9. [Phase 3: Login Screen (Slate)](#phase-3-login-screen-slate)
10. [Phase 4: Server Select Screen (Slate)](#phase-4-server-select-screen-slate)
11. [Phase 5: Character Select Screen (Slate)](#phase-5-character-select-screen-slate)
12. [Phase 6: Character Creation Screen (Slate)](#phase-6-character-creation-screen-slate)
13. [Phase 7: Loading / Transition Screen (Slate)](#phase-7-loading--transition-screen-slate)
14. [Phase 8: Game Entry Flow & Socket Integration](#phase-8-game-entry-flow--socket-integration)
15. [Phase 9: Launcher (Deferred / Phase 2 Project)](#phase-9-launcher-deferred--phase-2-project)
16. [Phase 10: Blueprint Cleanup & Level Setup](#phase-10-blueprint-cleanup--level-setup)
17. [File Manifest](#file-manifest)
18. [Skills & Rules Required](#skills--rules-required)
19. [User Actions Required](#user-actions-required)
20. [Verification Checklist](#verification-checklist)

---

## 1. Executive Summary

This plan redesigns the complete login-to-game-world flow for Sabri_MMO, replacing the current Blueprint-based UMG widget system with a pure C++ Slate implementation. The redesign covers:

- **Login screen** with username/password, remember username, and error feedback
- **Server selection** screen with player counts and status indicators
- **Character selection** screen with a card grid (up to 9 slots), character detail panel, and delete functionality
- **Character creation** screen with name, job class, gender, hair style, and hair color
- **Loading/transition** overlay with connection status feedback
- **Game entry** flow with proper JWT validation and data loading

All UI will be pure C++ Slate widgets following the established `SWidget + UWorldSubsystem` pattern. No new Blueprints are needed. The existing Blueprint widgets (WBP_LoginScreen, WBP_CharacterSelect, WBP_CharacterEntry, WBP_CreateCharacter) will be retired. BP_GameFlow will be replaced by a C++ `ULoginFlowSubsystem`.

**Launcher recommendation**: Deferred to a separate project phase. The in-engine login flow handles all gameplay-critical functionality. A standalone launcher (for patching, news, social links) is a polish feature that can be built as an Electron app later. If launching on Steam, Steam handles updates — the launcher becomes optional branding/news surface.

---

## 2. Current State Analysis

### 2.1 Current Flow (Blueprint-Based)

```
L_Startup level loads
    → BP_GameFlow.BeginPlay
    → Creates WBP_LoginScreen (UMG widget)
    → User enters credentials
    → UHttpManager::LoginUser() [C++ HTTP call]
    → OnLoginSuccess delegate → BP_GameFlow removes login widget
    → UHttpManager::GetCharacters() [C++ HTTP call]
    → OnCharacterListReceived → BP_GameFlow creates WBP_CharacterSelect
    → User picks character → GameInstance.SelectCharacter()
    → WBP_CharacterSelect calls Open Level → loads game level
    → BP_SocketManager.BeginPlay → emits player:join
```

### 2.2 Current Components

| Component | Type | Location | Status |
|-----------|------|----------|--------|
| BP_GameFlow | Blueprint Actor | Placed in L_Startup | Orchestrates flow via 4 delegate bindings |
| WBP_LoginScreen | UMG Widget (Blueprint) | Binary .uasset | Username/password/login button only |
| WBP_CharacterSelect | UMG Widget (Blueprint) | Binary .uasset | Scrollbox of character entries |
| WBP_CharacterEntry | UMG Widget (Blueprint) | Binary .uasset | Name/class/level + select button per character |
| WBP_CreateCharacter | UMG Widget (Blueprint) | Binary .uasset | Name + class dropdown only |
| UMMOGameInstance | C++ | MMOGameInstance.h/.cpp | Auth state, character list, selected character |
| UHttpManager | C++ BlueprintFunctionLibrary | MMOHttpManager.h/.cpp | HTTP calls (login, register, get/create chars) |
| FCharacterData | C++ Struct | CharacterData.h | Character data for client-side storage |

### 2.3 Server Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| POST | `/api/auth/register` | Create account |
| POST | `/api/auth/login` | Authenticate, get JWT |
| GET | `/api/auth/verify` | Validate token (never called by client) |
| GET | `/api/characters` | List user's characters |
| POST | `/api/characters` | Create character |
| GET | `/api/characters/:id` | Get single character (never called by client) |
| PUT | `/api/characters/:id/position` | Save position |
| Socket | `player:join` | Enter game world |

### 2.4 Database Tables

**users**: user_id, username, email, password_hash, created_at, last_login
**characters**: 27 columns total (after migrations + server auto-columns): identity, RO 6-stats, position, HP/SP, EXP, job progression, zuzucoin, timestamps

---

## 3. Gaps & Issues

### 3.1 Security (Critical)

| # | Issue | Impact |
|---|-------|--------|
| S1 | `player:join` socket event does NOT verify the JWT token | Any client can impersonate any character |
| S2 | No character ownership check on `player:join` | A user can join with another user's characterId |
| S3 | No rate limiting on Socket.io events | Spam attacks on combat/position/inventory events |
| S4 | No logout endpoint — JWT stays valid for 24h | Cannot force session termination |
| S5 | No token refresh mechanism | 24h sessions expire silently |

### 3.2 Missing Features

| # | Feature | RO Classic Has It | Current Status |
|---|---------|-------------------|----------------|
| F1 | Character deletion | Yes (with timer + email verification) | No endpoint or UI |
| F2 | Server selection | Yes (server list with population) | Hardcoded localhost:3001 |
| F3 | Registration UI | Yes (separate registration) | C++ function exists, no UI |
| F4 | Hair style / hair color | Yes (at character creation) | Not in DB or creation flow |
| F5 | Gender selection | Yes (account-level in RO classic) | Not in DB or creation flow |
| F6 | Character slot limit | Yes (3-15 per server) | Unlimited characters |
| F7 | Global character name uniqueness | Yes | Only unique per user |
| F8 | Remember username | Yes ("Save ID" checkbox) | Not implemented |
| F9 | Character detail panel | Yes (stats, level, class, map) | Not on character select |
| F10 | Loading/connecting feedback | Yes ("Please wait..." modal) | No transition feedback |
| F11 | Error feedback on char creation | Yes | Errors only in Output Log |

### 3.3 Technical Debt

| # | Issue | Impact |
|---|-------|--------|
| T1 | Manual JSON parsing in MMOHttpManager | Fragile, breaks on field reordering |
| T2 | Hardcoded `http://localhost:3001` in 6+ places | Cannot configure server address |
| T3 | `class` column vs `job_class` column confusion | Two disconnected class systems |
| T4 | Legacy `experience` column (unused) | Confuses developers |
| T5 | Redundant weapon query in player:join (steps 3 & 4 identical) | Wasted DB round-trip |
| T6 | GET /api/characters returns only 10 columns, FCharacterData has 12 fields | Under-parsed response, missing job/EXP data |
| T7 | OnLoginFailed delegate carries no error reason | UI cannot show specific error messages |
| T8 | RegisterUser() discards the returned token | Requires separate login after registration |
| T9 | All login/charselect UI in binary .uasset files | Not in source control, not diffable |

### 3.4 Architecture Concerns

| # | Concern | Recommendation |
|---|---------|----------------|
| A1 | Two-level approach (L_Startup → game level) | **Keep it.** This is standard for UE5 MMOs. The login level is lightweight, the game level has all gameplay actors. GameInstance persists across the transition. |
| A2 | BP_GameFlow orchestrates via delegates | **Replace with C++ UWorldSubsystem.** Eliminates Blueprint dependency for critical flow. |
| A3 | WBP widgets are UMG (Blueprint) | **Replace with Slate (C++).** Follows established project pattern, source-controllable. |
| A4 | Socket.io connection established in game level only | **Keep it.** Socket.io is not needed during login (REST API handles auth). Connect only after entering the game world. |

---

## 4. Design Decisions

### 4.1 Launcher — Defer to Phase 2

**Recommendation: Do NOT build a launcher now.**

Reasons:
- A launcher is a **separate application** (Electron, CEF, or native) — completely different tech stack from UE5
- The in-engine login flow handles all gameplay-critical functionality
- If launching on Steam, Steam handles patching/updates — the launcher becomes a branding/news surface only
- Many successful Steam MMOs (FFXIV, GW2, BDO) have their own launchers, but these were built after the core game was functional
- Building a launcher now would delay actual gameplay improvements

**Deferred launcher plan** (see Phase 9): Electron-based app with news panel, social links, PLAY button, patch status. Launches the UE5 .exe. Can be built independently without touching the game code.

### 4.2 Pure C++ Slate (No New Blueprints)

All new UI will be pure C++ Slate widgets following the established pattern:
- `SLoginWidget` + `ULoginFlowSubsystem` (orchestrator for entire flow)
- `SServerSelectWidget` (panel within the login flow)
- `SCharacterSelectWidget` (panel within the login flow)
- `SCharacterCreateWidget` (panel within the login flow)
- `SLoadingOverlayWidget` (reusable "please wait" overlay)

The `ULoginFlowSubsystem` replaces BP_GameFlow and manages all screen transitions, HTTP calls, and state.

### 4.3 Level Architecture — Keep Two Levels

```
L_Login (renamed from L_Startup)
    ├── Camera actor (looking at background scene or static image)
    ├── ULoginFlowSubsystem manages all Slate UI
    └── No gameplay actors, no socket connection

L_Game (existing game level)
    ├── BP_SocketManager
    ├── BP_MMOCharacter (spawned by GameMode)
    ├── BP_OtherPlayerManager
    ├── BP_EnemyManager
    └── All HUD subsystems (BasicInfo, CombatStats, etc.)
```

The user will need to:
1. Rename L_Startup to L_Login (or create a new L_Login level)
2. Remove BP_GameFlow from the level (C++ subsystem replaces it)
3. Set up a camera + background for the login screen ambiance

### 4.4 RO-Inspired Visual Design

Following the reference images:

| Screen | Layout |
|--------|--------|
| **Login** | Full-screen background image/scene. Compact login form (lower-left or center). Game logo at top. "Remember Username" checkbox. Login + Exit buttons. |
| **Server Select** | Same background. Compact dialog with scrollable server list. Each row: name + population + status. Cancel button. |
| **Character Select** | Full scene change — bright, colorful background. 3x3 card grid (9 slots). Right-side detail panel with stats. "Play" and "Delete" buttons. Empty slots show "Create" prompt. |
| **Character Create** | Dialog overlay on character select background. Character preview area. Name field, job selector, gender toggle, hair style/color sliders. OK + Cancel buttons. |
| **Loading** | Overlay on current background. "Please wait... Connecting to server..." with progress bar. |

### 4.5 Character Slot System

Following RO classic (expanded):
- **9 character slots** per account per server (3x3 grid)
- Server enforces the limit in POST `/api/characters`
- Empty slots are clickable to create a new character
- Characters display: name, job class, base level, job level

### 4.6 Character Deletion — RO Style

Following RO classic:
- Click "Delete" on selected character → confirmation dialog with email/password entry
- Server marks character with `delete_date` (current time + 24 hours)
- Character shows countdown timer on the character select screen
- After timer expires, user clicks "Delete" again to permanently remove
- Characters in guilds cannot be deleted (future feature gate)

**Simplified for now**: Password confirmation + immediate deletion (no timer). Timer system can be added later.

---

## 5. Target Architecture

### 5.1 New C++ Files

```
client/SabriMMO/Source/SabriMMO/
├── UI/
│   ├── LoginFlowSubsystem.h/.cpp       ← Orchestrator (replaces BP_GameFlow)
│   ├── SLoginWidget.h/.cpp              ← Login form
│   ├── SServerSelectWidget.h/.cpp       ← Server list
│   ├── SCharacterSelectWidget.h/.cpp    ← Character grid + detail panel
│   ├── SCharacterCreateWidget.h/.cpp    ← Character creation form
│   └── SLoadingOverlayWidget.h/.cpp     ← "Please wait" overlay
```

### 5.2 Modified C++ Files

```
MMOHttpManager.h/.cpp       ← New endpoints, proper JSON parsing, configurable URL
MMOGameInstance.h/.cpp       ← Server selection state, remember username, error delegates
CharacterData.h              ← Add hair_style, hair_color, gender fields
SabriMMO.Build.cs            ← Already has Slate deps, verify
```

### 5.3 Server Changes

```
server/src/index.js          ← New endpoints, JWT socket validation, char deletion
```

### 5.4 Database Changes

```
database/migrations/add_character_customization.sql   ← hair_style, hair_color, gender, delete_date
database/init.sql                                      ← Update for clean installs
```

### 5.5 Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ L_Login Level                                                    │
│                                                                  │
│  ULoginFlowSubsystem (UWorldSubsystem)                          │
│  ├── State Machine: Login → ServerSelect → CharSelect → Loading │
│  ├── Owns: SLoginWidget, SServerSelectWidget,                   │
│  │         SCharacterSelectWidget, SCharacterCreateWidget,       │
│  │         SLoadingOverlayWidget                                 │
│  ├── Calls: UHttpManager (static functions)                     │
│  ├── Reads: UMMOGameInstance (auth state, character list)        │
│  └── Triggers: UGameplayStatics::OpenLevel() to load game level │
│                                                                  │
│  UMMOGameInstance (persists across levels)                        │
│  ├── AuthToken, Username, UserId                                │
│  ├── SelectedServerId, SelectedServerUrl                        │
│  ├── CharacterList (TArray<FCharacterData>)                     │
│  ├── SelectedCharacter (FCharacterData)                         │
│  └── bRememberUsername (saved to GameUserSettings.ini)           │
│                                                                  │
└──────────────────────────┬──────────────────────────────────────┘
                           │ OpenLevel("L_Game")
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ L_Game Level                                                     │
│                                                                  │
│  BP_SocketManager                                                │
│  ├── Reads: GameInstance.AuthToken, SelectedCharacter            │
│  ├── Connects to Socket.io at SelectedServerUrl                  │
│  └── Emits: player:join { characterId, token, characterName }   │
│                                                                  │
│  BP_MMOCharacter (spawned by GameMode)                           │
│  ├── All gameplay HUD subsystems initialize                     │
│  └── Game begins                                                │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.6 State Machine

```
┌─────────┐   Login     ┌──────────────┐   Select    ┌───────────────┐
│  INIT   │ ──Success──→│ SERVER_SELECT │ ──Server──→│ CHAR_SELECT   │
└─────────┘             └──────────────┘             └───────────────┘
     │                        │                        │       │
     │                        │ Cancel                 │       │ Create
     │                        ▼                        │       ▼
     │                   ┌─────────┐                   │  ┌───────────────┐
     │                   │  LOGIN  │←──────────────────┘  │ CHAR_CREATE   │
     │                   └─────────┘      Back/Logout     └───────────────┘
     │                                                         │
     │                                                    OK/Cancel
     │                                                         │
     │                                                         ▼
     │                                                   CHAR_SELECT
     │
     │   ┌──────────┐
     └──→│ LOADING  │──→ OpenLevel("L_Game")
          └──────────┘
```

States:
- **LOGIN**: Show SLoginWidget. On success → SERVER_SELECT.
- **SERVER_SELECT**: Show SServerSelectWidget. On select → LOADING (fetching characters) → CHAR_SELECT.
- **CHAR_SELECT**: Show SCharacterSelectWidget. On play → LOADING (entering world). On create → CHAR_CREATE.
- **CHAR_CREATE**: Show SCharacterCreateWidget. On OK → LOADING (creating) → CHAR_SELECT. On cancel → CHAR_SELECT.
- **LOADING**: Show SLoadingOverlayWidget with status text. Transitions to next state on completion.

---

## 6. Implementation Phases

| Phase | Name | Dependencies | Estimated Scope |
|-------|------|--------------|-----------------|
| 1 | Database & Server Foundation | None | 1 file modified (index.js), 1 migration, init.sql update |
| 2 | C++ Infrastructure | Phase 1 | MMOHttpManager, MMOGameInstance, CharacterData.h, Build.cs |
| 3 | Login Screen (Slate) | Phase 2 | SLoginWidget + LoginFlowSubsystem (partial) |
| 4 | Server Select Screen (Slate) | Phase 3 | SServerSelectWidget + LoginFlowSubsystem (partial) |
| 5 | Character Select Screen (Slate) | Phase 4 | SCharacterSelectWidget + LoginFlowSubsystem (partial) |
| 6 | Character Creation Screen (Slate) | Phase 5 | SCharacterCreateWidget |
| 7 | Loading/Transition Screen (Slate) | Phase 3 | SLoadingOverlayWidget |
| 8 | Game Entry Flow & Socket Integration | Phase 5, 7 | LoginFlowSubsystem completion, OpenLevel, socket changes |
| 9 | Launcher (Deferred) | All above | Separate Electron project |
| 10 | Blueprint Cleanup & Level Setup | All above | Remove old BP widgets, configure levels |

---

## Phase 1: Database & Server Foundation

### 1.1 Database Migration: `add_character_customization.sql`

Create `database/migrations/add_character_customization.sql`:

```sql
-- Migration: Add character customization fields
-- Required for: Character creation redesign (hair, gender, appearance)

-- Hair style (integer ID, 1-19 in RO classic, expandable)
ALTER TABLE characters ADD COLUMN IF NOT EXISTS hair_style INTEGER DEFAULT 1;

-- Hair color (integer ID, palette index)
ALTER TABLE characters ADD COLUMN IF NOT EXISTS hair_color INTEGER DEFAULT 0;

-- Gender ('male' or 'female')
ALTER TABLE characters ADD COLUMN IF NOT EXISTS gender VARCHAR(10) DEFAULT 'male';

-- Soft-delete timestamp for pending deletion (NULL = not marked for deletion)
ALTER TABLE characters ADD COLUMN IF NOT EXISTS delete_date TIMESTAMP DEFAULT NULL;

-- Enforce global character name uniqueness
-- First check for existing duplicates and handle them
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint WHERE conname = 'characters_name_unique'
    ) THEN
        ALTER TABLE characters ADD CONSTRAINT characters_name_unique UNIQUE (name);
    END IF;
END $$;

-- Add index for pending deletions (for cleanup queries)
CREATE INDEX IF NOT EXISTS idx_characters_delete_date ON characters (delete_date)
WHERE delete_date IS NOT NULL;
```

### 1.2 Update `database/init.sql`

Add the new columns to the characters CREATE TABLE so clean installs include them:

```sql
-- Add after max_mana line:
hair_style INTEGER DEFAULT 1,
hair_color INTEGER DEFAULT 0,
gender VARCHAR(10) DEFAULT 'male',
delete_date TIMESTAMP DEFAULT NULL,
-- Add UNIQUE constraint on name:
UNIQUE (name)
```

### 1.3 Server Changes: `server/src/index.js`

#### 1.3.1 Auto-add new columns on startup

Add to the `ensureColumnsExist()` block (near line 5689):

```javascript
{ name: 'hair_style', type: 'INTEGER', default: 1 },
{ name: 'hair_color', type: 'INTEGER', default: 0 },
{ name: 'gender', type: "VARCHAR(10)", default: "'male'" },
{ name: 'delete_date', type: 'TIMESTAMP', default: 'NULL' },
```

#### 1.3.2 New endpoint: `GET /api/servers`

```javascript
// Server list endpoint
app.get('/api/servers', (req, res) => {
    // For now, return a single server. Later: query a server registry.
    const servers = [
        {
            id: 1,
            name: 'Sabri',
            host: process.env.SERVER_HOST || 'localhost',
            port: parseInt(process.env.PORT) || 3001,
            status: 'online',
            population: connectedPlayers.size,
            maxPopulation: 1000,
            region: 'Local'
        }
    ];
    res.json({ servers });
});
```

#### 1.3.3 Updated endpoint: `GET /api/characters`

Expand the SELECT query to return all fields the character select screen needs:

```javascript
app.get('/api/characters', authenticateToken, async (req, res) => {
    const result = await pool.query(
        `SELECT character_id, name, class, level, x, y, z,
                health, max_health, mana, max_mana,
                str, agi, vit, int_stat, dex, luk,
                stat_points, zuzucoin,
                job_level, base_exp, job_exp, job_class, skill_points,
                hair_style, hair_color, gender,
                delete_date, created_at, last_played
         FROM characters
         WHERE user_id = $1
         ORDER BY character_id ASC
         LIMIT 9`,
        [req.user.user_id]
    );
    res.json({ message: 'Characters retrieved successfully', characters: result.rows });
});
```

#### 1.3.4 Updated endpoint: `POST /api/characters`

Add character limit, customization fields, global name uniqueness:

```javascript
app.post('/api/characters', authenticateToken, async (req, res) => {
    const { name, characterClass, hairStyle, hairColor, gender } = req.body;

    // Validate name
    if (!name || name.length < 2 || name.length > 24) {
        return res.status(400).json({ error: 'Name must be 2-24 characters' });
    }

    // Validate name format (alphanumeric + spaces, no special chars)
    if (!/^[a-zA-Z0-9 ]+$/.test(name)) {
        return res.status(400).json({ error: 'Name can only contain letters, numbers, and spaces' });
    }

    // Check character limit (9 per account)
    const countResult = await pool.query(
        'SELECT COUNT(*) FROM characters WHERE user_id = $1 AND delete_date IS NULL',
        [req.user.user_id]
    );
    if (parseInt(countResult.rows[0].count) >= 9) {
        return res.status(400).json({ error: 'Maximum 9 characters per account' });
    }

    // Check global name uniqueness
    const nameCheck = await pool.query(
        'SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1)',
        [name.trim()]
    );
    if (nameCheck.rows.length > 0) {
        return res.status(409).json({ error: 'Character name is already taken' });
    }

    // Validate customization
    const validHairStyle = Math.max(1, Math.min(19, parseInt(hairStyle) || 1));
    const validHairColor = Math.max(0, Math.min(8, parseInt(hairColor) || 0));
    const validGender = (gender === 'female') ? 'female' : 'male';

    // All characters start as Novice (RO classic style)
    // The characterClass field is kept for display purposes but job_class is always 'novice'
    const validClasses = ['novice'];
    const charClass = 'novice';

    const result = await pool.query(
        `INSERT INTO characters
         (user_id, name, class, level, x, y, z, health, mana,
          job_level, base_exp, job_exp, job_class, skill_points, stat_points,
          hair_style, hair_color, gender, max_health, max_mana)
         VALUES ($1, $2, $3, 1, 0, 0, 0, 100, 100,
                 1, 0, 0, 'novice', 0, 48,
                 $4, $5, $6, 100, 100)
         RETURNING character_id, name, class, level, job_level, job_class,
                   hair_style, hair_color, gender, created_at`,
        [req.user.user_id, name.trim(), charClass,
         validHairStyle, validHairColor, validGender]
    );

    res.status(201).json({
        message: 'Character created successfully',
        character: result.rows[0]
    });
});
```

#### 1.3.5 New endpoint: `DELETE /api/characters/:id`

```javascript
app.delete('/api/characters/:id', authenticateToken, async (req, res) => {
    const characterId = parseInt(req.params.id);
    const { password } = req.body;

    if (!password) {
        return res.status(400).json({ error: 'Password required to delete character' });
    }

    // Verify password
    const userResult = await pool.query(
        'SELECT password_hash FROM users WHERE user_id = $1',
        [req.user.user_id]
    );
    if (userResult.rows.length === 0) {
        return res.status(404).json({ error: 'User not found' });
    }

    const validPassword = await bcrypt.compare(password, userResult.rows[0].password_hash);
    if (!validPassword) {
        return res.status(401).json({ error: 'Incorrect password' });
    }

    // Verify character belongs to user and is not already deleted
    const charResult = await pool.query(
        'SELECT character_id, name FROM characters WHERE character_id = $1 AND user_id = $2 AND deleted = FALSE',
        [characterId, req.user.user_id]
    );
    if (charResult.rows.length === 0) {
        return res.status(404).json({ error: 'Character not found' });
    }

    // Check if character is currently online
    if (connectedPlayers.has(characterId)) {
        return res.status(409).json({ error: 'Character is currently online. Log out first.' });
    }

    // Soft delete — mark as deleted instead of removing rows
    // Inventory, hotbar, and skill data are preserved for potential restoration
    await pool.query('UPDATE characters SET deleted = TRUE WHERE character_id = $1', [characterId]);

    res.json({ message: 'Character deleted successfully', characterName: charResult.rows[0].name });
});
```

#### 1.3.6 JWT Validation on `player:join` Socket Event

Add JWT verification at the top of the `player:join` handler:

```javascript
socket.on('player:join', async (data) => {
    const { characterId, token, characterName } = data;

    // SECURITY: Verify JWT token
    let decoded;
    try {
        decoded = jwt.verify(token, process.env.JWT_SECRET);
    } catch (err) {
        socket.emit('player:join_error', { error: 'Invalid or expired token' });
        return;
    }

    // SECURITY: Verify character belongs to the authenticated user
    const ownerCheck = await pool.query(
        'SELECT 1 FROM characters WHERE character_id = $1 AND user_id = $2',
        [characterId, decoded.user_id]
    );
    if (ownerCheck.rows.length === 0) {
        socket.emit('player:join_error', { error: 'Character does not belong to this account' });
        return;
    }

    // ... rest of existing player:join handler ...
});
```

#### 1.3.7 Fix Redundant Weapon Query

Merge steps 3 and 4 of `player:join` into a single query:

```javascript
// BEFORE (two separate queries):
// Step 3: SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range ...
// Step 4: SELECT i.weapon_type ... (REDUNDANT)

// AFTER (single query):
const weaponResult = await pool.query(
    `SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range
     FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
     WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon'
     LIMIT 1`,
    [characterId]
);
// Use weaponResult for both ATK setup AND passive skill weapon type check
```

---

## Phase 2: C++ Infrastructure

### 2.1 Update `CharacterData.h`

Add new fields to `FCharacterData`:

```cpp
// Add after SkillPoints:
UPROPERTY(BlueprintReadWrite, Category = "Appearance")
int32 HairStyle = 1;

UPROPERTY(BlueprintReadWrite, Category = "Appearance")
int32 HairColor = 0;

UPROPERTY(BlueprintReadWrite, Category = "Appearance")
FString Gender = TEXT("male");

// Add for character select display:
UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 MaxHealth = 100;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 MaxMana = 100;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 Str = 1;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 Agi = 1;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 Vit = 1;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 IntStat = 1;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 Dex = 1;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 Luk = 1;

UPROPERTY(BlueprintReadWrite, Category = "Stats")
int32 StatPoints = 48;

UPROPERTY(BlueprintReadWrite, Category = "Economy")
int32 Zuzucoin = 0;

// For deletion tracking
UPROPERTY(BlueprintReadWrite, Category = "Meta")
FString DeleteDate;  // Empty = not marked for deletion

UPROPERTY(BlueprintReadWrite, Category = "Meta")
FString CreatedAt;

UPROPERTY(BlueprintReadWrite, Category = "Meta")
FString LastPlayed;
```

Add a new struct for server list:

```cpp
USTRUCT(BlueprintType)
struct FServerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) int32 ServerId = 0;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString Host;
    UPROPERTY(BlueprintReadWrite) int32 Port = 3001;
    UPROPERTY(BlueprintReadWrite) FString Status;      // "online", "offline", "maintenance"
    UPROPERTY(BlueprintReadWrite) int32 Population = 0;
    UPROPERTY(BlueprintReadWrite) int32 MaxPopulation = 1000;
    UPROPERTY(BlueprintReadWrite) FString Region;
};
```

### 2.2 Rewrite `MMOHttpManager.h/.cpp`

Complete rewrite with:

**Header changes:**
```cpp
// New delegates with error info
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginFailedWithReason, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterCreateFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeleteSuccess, const FString&, CharacterName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeleteFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerListReceived, const TArray<FServerInfo>&, Servers);

// New functions
UFUNCTION(BlueprintCallable, Category = "MMO|HTTP")
static void GetServerList(UObject* WorldContextObject);

UFUNCTION(BlueprintCallable, Category = "MMO|HTTP")
static void DeleteCharacter(UObject* WorldContextObject, int32 CharacterId, const FString& Password);

// Configurable server URL
static FString GetServerBaseUrl();
static void SetServerBaseUrl(const FString& Url);
```

**Implementation changes:**
- Replace ALL manual JSON parsing with `FJsonSerializer::Deserialize` + `FJsonObject`
- Replace hardcoded `http://localhost:3001` with `GetServerBaseUrl()`
- Parse ALL character fields from the expanded GET /api/characters response
- Add proper error message extraction from server responses
- Use `TWeakObjectPtr<UObject>` for WorldContextObject in lambdas

### 2.3 Update `MMOGameInstance.h/.cpp`

Add:

```cpp
// Server selection
UPROPERTY() FServerInfo SelectedServer;
UPROPERTY() TArray<FServerInfo> ServerList;

// Remember username (persisted)
UPROPERTY() bool bRememberUsername = false;
UPROPERTY() FString RememberedUsername;

// New delegates
UPROPERTY(BlueprintAssignable) FOnLoginFailedWithReason OnLoginFailedWithReason;
UPROPERTY(BlueprintAssignable) FOnServerListReceived OnServerListReceived;
UPROPERTY(BlueprintAssignable) FOnCharacterDeleteSuccess OnCharacterDeleteSuccess;
UPROPERTY(BlueprintAssignable) FOnCharacterDeleteFailed OnCharacterDeleteFailed;
UPROPERTY(BlueprintAssignable) FOnCharacterCreateFailed OnCharacterCreateFailed;

// Methods
void SelectServer(const FServerInfo& Server);
FString GetServerSocketUrl() const;  // Returns "http://host:port" for socket.io
void SaveRememberedUsername();
void LoadRememberedUsername();
void Logout();  // Clears all auth state, transitions back to login
```

Save/load remembered username via `GGameUserSettingsIni` or `UGameUserSettings`.

### 2.4 Verify `SabriMMO.Build.cs`

Ensure these modules are present (most already are):

```csharp
"Slate",
"SlateCore",
"UMG",
"Json",
"JsonUtilities",
"HTTP",
"InputCore",
```

---

## Phase 3: Login Screen (Slate)

### 3.1 Files

- `UI/SLoginWidget.h` / `UI/SLoginWidget.cpp`
- `UI/LoginFlowSubsystem.h` / `UI/LoginFlowSubsystem.cpp` (partial — login state only)

### 3.2 SLoginWidget Design

```
┌─────────────────────────────────────────────────────────────┐
│                     [Full-screen background]                 │
│                                                              │
│                    ┌─ SABRI MMO LOGO ─┐                     │
│                    └──────────────────┘                      │
│                                                              │
│  ┌──────────────────────────┐                                │
│  │ ##Login                  │                                │
│  │                          │                                │
│  │  Username  [___________] │                                │
│  │  Password  [___________] │                                │
│  │  [x] Remember Username   │                                │
│  │                          │                                │
│  │  [Error message area]    │                                │
│  │                          │                                │
│  │        [Login]  [Exit]   │                                │
│  └──────────────────────────┘                                │
│                                                              │
│  v1.0.0            (c) 2026 Sabri MMO                       │
└─────────────────────────────────────────────────────────────┘
```

### 3.3 SLoginWidget Features

- Username `SEditableTextBox` — pre-filled if "Remember Username" was checked
- Password `SEditableTextBox` with password masking (no echo)
- "Remember Username" `SCheckBox` — persisted via GameInstance → GameUserSettings
- Error text area — hidden by default, shown on failure with specific error message
- Login button — calls `UHttpManager::LoginUser()`, shows "Logging in..." state
- Exit button — calls `FPlatformMisc::RequestExit(false)` (quits game)
- Enter key submits the form (bind to OnKeyDown)
- Tab key moves between username and password fields
- Auto-focus: if username is pre-filled, focus password field; otherwise focus username
- RO Classic gold/brown theme for the dialog frame (3-layer border pattern)
- Background: full-screen `SImage` with a background texture (or transparent to show level camera)
- Game logo: `SImage` at top center

### 3.4 LoginFlowSubsystem (Login State)

```cpp
enum class ELoginFlowState : uint8
{
    Login,
    ServerSelect,
    CharacterSelect,
    CharacterCreate,
    Loading,
    EnteringWorld
};
```

The subsystem:
1. Creates on `OnWorldBeginPlay` if the world is the login level
2. Sets initial state to `Login`
3. Shows `SLoginWidget`
4. Binds to `UMMOGameInstance::OnLoginSuccess` → transitions to `ServerSelect`
5. Binds to `UMMOGameInstance::OnLoginFailedWithReason` → shows error on SLoginWidget

---

## Phase 4: Server Select Screen (Slate)

### 4.1 Files

- `UI/SServerSelectWidget.h` / `UI/SServerSelectWidget.cpp`

### 4.2 SServerSelectWidget Design

```
┌─────────────────────────────────────────────────────────────┐
│                     [Same background]                        │
│                                                              │
│         ┌──────────────────────────────┐                    │
│         │ ##Server Select              │                    │
│         │                              │                    │
│         │  ┌────────────────────────┐  │                    │
│         │  │ Sabri (42)     Online  │  │  ← selected       │
│         │  │ Local (0)      Online  │  │                    │
│         │  │                        │  │                    │
│         │  │                        │  │                    │
│         │  └────────────────────────┘  │                    │
│         │                              │                    │
│         │        [Connect]  [Cancel]   │                    │
│         └──────────────────────────────┘                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 4.3 SServerSelectWidget Features

- Scrollable server list — each row shows: server name, population count, status (Online/Offline/Maintenance)
- Selected row highlighted in gold
- Click to select, double-click to connect
- "Connect" button — stores selected server in GameInstance, fetches characters
- "Cancel" button — returns to login state (calls `GameInstance->Logout()`)
- Status colors: Online = green text, Maintenance = yellow, Offline = red
- Population shown as "(42/1000)" format
- Currently single server, but architecture supports any number

---

## Phase 5: Character Select Screen (Slate)

### 5.1 Files

- `UI/SCharacterSelectWidget.h` / `UI/SCharacterSelectWidget.cpp`

### 5.2 SCharacterSelectWidget Design

```
┌─────────────────────────────────────────────────────────────────────┐
│                    [Bright/colorful background]                      │
│                                                                      │
│  ┌─────────────────────────────────────┐  ┌──────────────────────┐  │
│  │  Character Slots (3x3 grid)         │  │  Character Details    │  │
│  │  ┌─────┐ ┌─────┐ ┌─────┐          │  │                      │  │
│  │  │ Char│ │ Char│ │Empty│          │  │  Name: SwordMaster   │  │
│  │  │  1  │ │  2  │ │ +   │          │  │  Class: Swordsman    │  │
│  │  │ Lv52│ │ Lv13│ │     │          │  │  Base Lv: 52         │  │
│  │  └─────┘ └─────┘ └─────┘          │  │  Job Lv: 38          │  │
│  │  ┌─────┐ ┌─────┐ ┌─────┐          │  │                      │  │
│  │  │Empty│ │Empty│ │Empty│          │  │  STR: 45  AGI: 23    │  │
│  │  │ +   │ │ +   │ │ +   │          │  │  VIT: 30  INT: 1     │  │
│  │  └─────┘ └─────┘ └─────┘          │  │  DEX: 38  LUK: 1     │  │
│  │  ┌─────┐ ┌─────┐ ┌─────┐          │  │                      │  │
│  │  │Empty│ │Empty│ │Empty│          │  │  HP: 2340/2340       │  │
│  │  │ +   │ │ +   │ │ +   │          │  │  SP: 120/120         │  │
│  │  └─────┘ └─────┘ └─────┘          │  │  Zuzucoin: 15,420    │  │
│  │                                     │  │                      │  │
│  └─────────────────────────────────────┘  │  [PLAY]   [Delete]   │  │
│                                            └──────────────────────┘  │
│                                                                      │
│  [Back to Server Select]                              SABRI MMO v1.0 │
└─────────────────────────────────────────────────────────────────────┘
```

### 5.3 Character Card Content

Each occupied card shows:
- Character name (bold, centered)
- Job class (e.g., "Novice", "Swordsman")
- Base Level / Job Level
- Hair style indicator (colored circle or icon matching hair color)
- Gender indicator (small icon)
- Gold border if selected

Each empty card shows:
- "+" icon centered
- "Create" text below
- Dimmed/muted appearance
- Clickable to enter character creation

### 5.4 Detail Panel Content (Right Side)

When a character is selected:
- Character name (large, gold text)
- Job class + gender
- Base Level / Job Level
- Base EXP / Job EXP (with progress bars)
- All 6 stats: STR, AGI, VIT, INT, DEX, LUK
- HP / Max HP, SP / Max SP
- Stat Points remaining
- Zuzucoin amount
- Created date
- Last played date

Buttons:
- **PLAY** — large, prominent gold/green button → enters game world with selected character
- **Delete** — smaller, red-tinted button → opens confirmation dialog

### 5.5 Character Deletion Flow

1. User clicks "Delete" on a selected character
2. A confirmation dialog overlays: "Delete [CharName]? Enter your password to confirm."
3. Password input field + "Confirm Delete" + "Cancel" buttons
4. On confirm: calls `UHttpManager::DeleteCharacter(CharId, Password)`
5. On success: refresh character list, show brief "Character deleted" message
6. On failure: show error (wrong password, character online, etc.)

### 5.6 Character Select Features

- Auto-selects the first character on load (or the last-played character)
- Empty slot click → transitions to CHAR_CREATE state
- "Back to Server Select" → transitions back to SERVER_SELECT state
- Keyboard shortcuts: Enter = Play, Delete = initiate deletion, Escape = back

---

## Phase 6: Character Creation Screen (Slate)

### 6.1 Files

- `UI/SCharacterCreateWidget.h` / `UI/SCharacterCreateWidget.cpp`

### 6.2 SCharacterCreateWidget Design

```
┌─────────────────────────────────────────────────────────────┐
│  [Character Select background dimmed]                        │
│                                                              │
│       ┌──────────────────────────────────────┐              │
│       │ ##Create Character                   │              │
│       │                                      │              │
│       │  ┌──────────────────┐                │              │
│       │  │                  │                │              │
│       │  │  [Character      │   Name:        │              │
│       │  │   Preview        │   [__________] │              │
│       │  │   Area]          │                │              │
│       │  │                  │   Class: Novice│              │
│       │  │   ◄ rotate ►    │                │              │
│       │  └──────────────────┘   Gender:      │              │
│       │                         (M) (F)      │              │
│       │                                      │              │
│       │  Hair Style  ◄ [====o====] ►        │              │
│       │  Hair Color  ◄ [====o====] ►        │              │
│       │                                      │              │
│       │  [Error message area]                │              │
│       │                                      │              │
│       │              [Create]  [Cancel]       │              │
│       └──────────────────────────────────────┘              │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 6.3 SCharacterCreateWidget Features

- **Name field**: `SEditableTextBox`, 2-24 characters, alphanumeric + spaces
- **Class display**: Shows "Novice" (locked for now). Future: dropdown for expanded starting classes.
- **Gender toggle**: Male / Female radio buttons or toggle. Changes the preview character.
- **Hair Style slider**: Left/right arrows or `SSlider` cycling through styles 1-19. Live preview updates.
- **Hair Color slider**: Left/right arrows or `SSlider` cycling through colors 0-8. Live preview updates.
- **Character Preview area**: For Phase 1, this is a **placeholder area** showing a text description (e.g., "Male Novice, Hair Style 3, Brown"). Future: 3D render target with character model.
- **Error text**: Hidden by default. Shows validation errors (name taken, too short, etc.)
- **Create button**: Validates locally, calls `UHttpManager::CreateCharacter()`, shows loading
- **Cancel button**: Returns to CHAR_SELECT state

### 6.4 Future Expansion Areas (UI Scaffolded, Not Functional)

These areas are built into the layout but disabled/hidden for now:
- Stat distribution panel (RO classic had this, post-renewal removed it)
- Starting map selection (for multi-zone spawning)
- Character appearance slots (headgear preview)

---

## Phase 7: Loading / Transition Screen (Slate)

### 7.1 Files

- `UI/SLoadingOverlayWidget.h` / `UI/SLoadingOverlayWidget.cpp`

### 7.2 SLoadingOverlayWidget Design

```
┌─────────────────────────────────────────────────────────────┐
│  [Current background stays visible, slightly dimmed]         │
│                                                              │
│         ┌──────────────────────────────┐                    │
│         │ ##Status                     │                    │
│         │                              │                    │
│         │  Connecting to server...     │                    │
│         │  [========>        ]         │                    │
│         │                              │                    │
│         └──────────────────────────────┘                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 7.3 SLoadingOverlayWidget Features

- Reusable overlay shown during ANY async operation
- Semi-transparent dark background behind the dialog
- Status text (configurable): "Authenticating...", "Connecting to server...", "Loading characters...", "Creating character...", "Entering world..."
- Animated progress bar (indeterminate mode — no exact percentage)
- Auto-timeout after 15 seconds → show error "Connection timed out. Please try again." with a "Back" button
- The LoginFlowSubsystem controls when to show/hide this overlay and what text to display

---

## Phase 8: Game Entry Flow & Socket Integration

### 8.1 LoginFlowSubsystem — Complete State Machine

The subsystem manages the entire flow from login to entering the game world:

```cpp
void ULoginFlowSubsystem::TransitionTo(ELoginFlowState NewState)
{
    // Hide current widget(s)
    HideAllWidgets();

    CurrentState = NewState;

    switch (NewState)
    {
    case ELoginFlowState::Login:
        ShowLoginWidget();
        break;

    case ELoginFlowState::ServerSelect:
        ShowLoadingOverlay(TEXT("Fetching server list..."));
        UHttpManager::GetServerList(GetWorld());
        // On callback: HideLoadingOverlay(), ShowServerSelectWidget()
        break;

    case ELoginFlowState::CharacterSelect:
        ShowLoadingOverlay(TEXT("Loading characters..."));
        UHttpManager::GetCharacters(GetWorld());
        // On callback: HideLoadingOverlay(), ShowCharacterSelectWidget()
        break;

    case ELoginFlowState::CharacterCreate:
        ShowCharacterCreateWidget();
        break;

    case ELoginFlowState::EnteringWorld:
        ShowLoadingOverlay(TEXT("Entering world..."));
        // Brief delay then OpenLevel
        GetWorld()->GetTimerManager().SetTimer(EnterWorldTimer, [this]()
        {
            UGameplayStatics::OpenLevel(GetWorld(), TEXT("L_Game"));
        }, 0.5f, false);
        break;
    }
}
```

### 8.2 Level Transition

When transitioning from L_Login to L_Game:

1. `LoginFlowSubsystem::TransitionTo(EnteringWorld)` shows loading overlay
2. After brief delay, calls `UGameplayStatics::OpenLevel(GetWorld(), TEXT("L_Game"))`
3. `UMMOGameInstance` persists across the transition (holds auth token, selected character, selected server)
4. L_Game loads → BP_SocketManager connects to socket.io at `GameInstance->GetServerSocketUrl()`
5. BP_SocketManager emits `player:join` with token + characterId
6. Server validates JWT + character ownership (new in Phase 1)
7. Server loads all character data and responds with events
8. HUD subsystems initialize and display

### 8.3 BP_SocketManager Changes

The only Blueprint change needed:

Currently BP_SocketManager connects to a hardcoded URL. It needs to read the server URL from GameInstance:

**Option A (preferred — no Blueprint change needed):**
The socket URL is configured in the SocketIOClient plugin settings or read from a config. Since BP_SocketManager uses the SocketIOClient plugin, the connection URL can be set dynamically.

**Option B (minimal Blueprint change):**
Add a C++ function to `UMMOGameInstance`:
```cpp
UFUNCTION(BlueprintCallable)
FString GetServerSocketUrl() const
{
    return FString::Printf(TEXT("http://%s:%d"), *SelectedServer.Host, SelectedServer.Port);
}
```

BP_SocketManager reads this in BeginPlay before connecting. This is a single node change in an existing Blueprint.

### 8.4 Return to Login (Disconnect / Logout)

If the player disconnects or logs out from the game:
1. Game level can call `UGameplayStatics::OpenLevel(GetWorld(), TEXT("L_Login"))`
2. `UMMOGameInstance::Logout()` clears auth state but preserves `RememberedUsername`
3. L_Login loads → `LoginFlowSubsystem` initializes → shows login screen
4. If `bRememberUsername`, the username field is pre-filled

---

## Phase 9: Launcher (Deferred / Phase 2 Project)

### 9.1 Recommendation

**Build the launcher AFTER the core game login flow is working.** The launcher is a separate application that sits outside of UE5. It does not affect gameplay.

### 9.2 Launcher Design (When Built)

Based on the RO launcher reference image:

```
┌─────────────────────────────────────────────────────────────┐
│  [X]                                          SABRI MMO     │
│                                                              │
│  ┌──────────────────────────┐  ┌───────────────────────┐   │
│  │                          │  │ NEWS & UPDATES         │   │
│  │   Welcome Banner /       │  │                        │   │
│  │   Game Artwork           │  │ v1.2.0 - March 2026   │   │
│  │   (rotating carousel)    │  │ - Added Knight class   │   │
│  │                          │  │ - Fixed combat bugs    │   │
│  └──────────────────────────┘  │ - New dungeon: Payon   │   │
│                                 │                        │   │
│  ┌────────┐ ┌────────┐ ┌────┐  │ v1.1.0 - Feb 2026    │   │
│  │Website │ │ Wiki   │ │Help│  │ - Skill system launch │   │
│  └────────┘ └────────┘ └────┘  └───────────────────────┘   │
│                                                              │
│  [Status: Up to date]         🎮 Discord  📺 YouTube       │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                   ▶  P L A Y                          │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  v1.0.0                              (c) 2026 Sabri MMO    │
└─────────────────────────────────────────────────────────────┘
```

### 9.3 Launcher Tech Stack

- **Electron** (JavaScript/HTML/CSS) — cross-platform, easy to style, web views for news
- Alternative: **CEF (Chromium Embedded Framework)** for C++ integration
- Alternative: **.NET MAUI / WPF** for Windows-only

### 9.4 Launcher Features

| Feature | Purpose |
|---------|---------|
| Welcome banner | Game artwork, rotating promotional images |
| News panel | Patch notes, events, maintenance schedules (fetched from REST API) |
| Website button | Opens browser to game website |
| Wiki button | Opens browser to game wiki |
| Support button | Opens browser to support page or Discord |
| Discord link | Opens Discord invite link |
| Social media icons | YouTube, Twitter/X, etc. |
| Version display | Current client version |
| Update status | "Up to date" / "Updating..." with progress bar |
| PLAY button | Launches the UE5 game executable |
| Settings | Game resolution, audio settings (optional) |

### 9.5 Steam Compatibility

If launching on Steam:
- Steam handles game updates (no patching needed in launcher)
- The launcher becomes a **branded news/community hub** before the game opens
- Steam can be configured to launch the launcher first, which then launches the game .exe
- Many Steam MMOs do this: FFXIV, GW2, BDO, Warframe
- The launcher is **optional** — the game can also be launched directly via Steam

---

## Phase 10: Blueprint Cleanup & Level Setup

### 10.1 Level Setup (USER ACTION REQUIRED)

The user needs to do these in the Unreal Editor:

1. **Open L_Startup level** (or create L_Login as a copy)
2. **Delete BP_GameFlow** from the level (the C++ LoginFlowSubsystem replaces it)
3. **Set up background camera**:
   - Place a `CineCameraActor` or use the existing camera
   - Point it at a visually appealing scene (or a backdrop image)
   - This becomes the login screen background
4. **Optionally place a background image**:
   - Create a simple plane mesh with a material showing the login artwork
   - Position it in front of the camera
   - Or: the SLoginWidget can display a full-screen image via `SImage` + `FSlateBrush`
5. **Verify DefaultEngine.ini**:
   - `GameDefaultMap` should point to the login level
   - `GameInstanceClass` should be `/Script/SabriMMO.MMOGameInstance`
6. **Verify the game level name** — update the `OpenLevel` call in LoginFlowSubsystem to match the actual game level asset name

### 10.2 Old Blueprint Widgets (Can Be Deleted Later)

These Blueprint widgets will no longer be used once the Slate system is complete:

| Widget | Status |
|--------|--------|
| WBP_LoginScreen | Replaced by SLoginWidget |
| WBP_CharacterSelect | Replaced by SCharacterSelectWidget |
| WBP_CharacterEntry | Replaced by SCharacterSelectWidget (built-in cards) |
| WBP_CreateCharacter | Replaced by SCharacterCreateWidget |
| BP_GameFlow | Replaced by ULoginFlowSubsystem |

**Do NOT delete these until the new system is fully tested.** They serve as a fallback.

### 10.3 BP_SocketManager Update

The only change needed in BP_SocketManager:
- Read the server URL from `UMMOGameInstance::GetServerSocketUrl()` instead of using a hardcoded URL
- This is a single node swap in the Blueprint's BeginPlay event

---

## File Manifest

### New Files (C++)

| File | Purpose |
|------|---------|
| `UI/LoginFlowSubsystem.h` | Login flow state machine, widget lifecycle |
| `UI/LoginFlowSubsystem.cpp` | Implementation |
| `UI/SLoginWidget.h` | Login form Slate widget |
| `UI/SLoginWidget.cpp` | Implementation |
| `UI/SServerSelectWidget.h` | Server selection Slate widget |
| `UI/SServerSelectWidget.cpp` | Implementation |
| `UI/SCharacterSelectWidget.h` | Character grid + detail panel Slate widget |
| `UI/SCharacterSelectWidget.cpp` | Implementation |
| `UI/SCharacterCreateWidget.h` | Character creation form Slate widget |
| `UI/SCharacterCreateWidget.cpp` | Implementation |
| `UI/SLoadingOverlayWidget.h` | Reusable loading overlay Slate widget |
| `UI/SLoadingOverlayWidget.cpp` | Implementation |

### New Files (Database)

| File | Purpose |
|------|---------|
| `database/migrations/add_character_customization.sql` | Hair, gender, delete_date, name uniqueness |

### Modified Files (C++)

| File | Changes |
|------|---------|
| `CharacterData.h` | Add FServerInfo struct, expand FCharacterData with appearance/stats/meta fields |
| `MMOHttpManager.h` | New endpoints, new delegates, configurable URL |
| `MMOHttpManager.cpp` | Complete rewrite with proper JSON parsing |
| `MMOGameInstance.h` | Server selection, remember username, new delegates, Logout() |
| `MMOGameInstance.cpp` | Implementation |
| `SabriMMO.Build.cs` | Verify existing deps (should already be complete) |

### Modified Files (Server)

| File | Changes |
|------|---------|
| `server/src/index.js` | New columns, GET /api/servers, expanded GET /api/characters, POST /api/characters with customization, DELETE /api/characters/:id, JWT validation on player:join, fix redundant query |

### Modified Files (Database)

| File | Changes |
|------|---------|
| `database/init.sql` | Add hair_style, hair_color, gender, delete_date, UNIQUE(name) to characters table |

---

## Skills & Rules Required

When executing this plan, load these skills:

| Phase | Skills Needed |
|-------|---------------|
| Phase 1 (Server/DB) | `/full-stack` |
| Phase 2 (C++ Infra) | `/sabrimmo-ui` (for patterns) |
| Phase 3-7 (Slate widgets) | `/sabrimmo-ui` |
| Phase 8 (Integration) | `/realtime`, `/full-stack` |
| Phase 10 (Blueprint) | `/ui-architect` |
| All phases | `/project-docs` (update docs after each phase) |

---

## User Actions Required

These are the ONLY things the user must do manually in the Unreal Editor:

| # | Action | When | Why |
|---|--------|------|-----|
| U1 | Remove BP_GameFlow from L_Startup level | After Phase 3 is complete | C++ subsystem replaces it |
| U2 | Set up background camera/image in L_Login | After Phase 3 is complete | Login screen needs a visual background |
| U3 | Verify game level name matches OpenLevel call | After Phase 8 | Level name is in binary .uasset, not readable from code |
| U4 | Update BP_SocketManager to read server URL from GameInstance | After Phase 8 | Single node change in Blueprint |
| U5 | Run database migration | Before testing Phase 1 | `\i database/migrations/add_character_customization.sql` |
| U6 | Restart server after index.js changes | After Phase 1 | Server needs to pick up new endpoints |

Everything else is implementable from code without opening the Unreal Editor.

---

## Verification Checklist

### Phase 1 Complete When:
- [ ] Migration runs without errors
- [ ] `GET /api/servers` returns server list JSON
- [ ] `GET /api/characters` returns all 30 fields per character
- [ ] `POST /api/characters` accepts hair_style, hair_color, gender
- [ ] `POST /api/characters` enforces 9-character limit and global name uniqueness
- [ ] `DELETE /api/characters/:id` requires password and works
- [ ] `player:join` validates JWT and character ownership
- [ ] Redundant weapon query merged into one

### Phase 2 Complete When:
- [ ] `FCharacterData` has all new fields (appearance, stats, meta)
- [ ] `FServerInfo` struct exists
- [ ] `UHttpManager` uses `FJsonSerializer` for ALL responses
- [ ] `UHttpManager` uses configurable server URL
- [ ] All new endpoints callable: GetServerList, DeleteCharacter
- [ ] `UMMOGameInstance` has server selection, remember username, Logout()
- [ ] Project compiles cleanly

### Phase 3 Complete When:
- [ ] `SLoginWidget` renders with RO theme (gold/brown)
- [ ] Username + password fields work with keyboard (Enter to submit, Tab to switch)
- [ ] "Remember Username" persists across game sessions
- [ ] Login success → transitions to server select
- [ ] Login failure → shows specific error message
- [ ] Exit button closes the game
- [ ] Loading overlay shows during authentication

### Phase 4 Complete When:
- [ ] `SServerSelectWidget` shows server list from API
- [ ] Server rows show name, population, status
- [ ] Selected server highlighted
- [ ] Connect button fetches characters
- [ ] Cancel button returns to login

### Phase 5 Complete When:
- [ ] 3x3 character grid renders correctly
- [ ] Occupied slots show name, class, level
- [ ] Empty slots show "+" create prompt
- [ ] Selected character shows full stats in detail panel
- [ ] "Play" button enters the game world
- [ ] "Delete" button opens confirmation dialog
- [ ] Character deletion works with password confirmation
- [ ] "Back" button returns to server select

### Phase 6 Complete When:
- [ ] Character creation form has: name, class (Novice), gender, hair style, hair color
- [ ] Sliders cycle through valid ranges
- [ ] Name validation shows errors (too short, taken, invalid chars)
- [ ] "Create" sends request to server with all fields
- [ ] On success: returns to character select with new character in list
- [ ] "Cancel" returns to character select

### Phase 7 Complete When:
- [ ] Loading overlay shows during ALL async operations
- [ ] Status text updates per operation
- [ ] Progress bar animates (indeterminate)
- [ ] 15-second timeout shows error with back button
- [ ] Overlay properly hides when operation completes

### Phase 8 Complete When:
- [ ] Full flow works: Login → Server Select → Character Select → Play → Game loads
- [ ] Character creation flow works within the character select screen
- [ ] Character deletion flow works
- [ ] Game level loads with correct character data
- [ ] Socket.io connects to selected server URL
- [ ] `player:join` succeeds with JWT validation
- [ ] All HUD subsystems (BasicInfo, CombatStats, etc.) initialize correctly
- [ ] Logout/disconnect returns to login screen

### Phase 10 Complete When:
- [ ] BP_GameFlow removed from login level
- [ ] Old WBP widgets marked as deprecated (not deleted yet)
- [ ] BP_SocketManager reads server URL from GameInstance
- [ ] Documentation updated for all changes

---

## Design Differences from RO Classic

| Feature | RO Classic | Sabri_MMO Design | Reason |
|---------|------------|-------------------|--------|
| Starting stats | 48 points on 3 axes at creation | All stats start at 1, distributed on level-up | Post-renewal approach; prevents newbie mistakes |
| Gender | Account-level (all chars same gender) | Per-character | Modern standard, more flexibility |
| Character slots | 3 (classic) to 15 (expanded) | 9 per account per server | Good middle ground |
| Character deletion | 24h timer + email verification | Password confirmation + immediate delete | Simplified; timer can be added later |
| Launcher/Patcher | GRF patcher with news panel | Deferred to Phase 2 project | In-engine flow first, launcher is polish |
| Login screen | 2D background image | UE5 level with camera (can be 3D scene) | Takes advantage of UE5 capabilities |
| Character preview | 2D sprite | Placeholder text (future: 3D render target) | 3D preview requires render target setup |
| Server population | Descriptive labels (Comfortable/Normal/Congested) | Numeric count + bar | More informative |
| Class at creation | Always Novice | Novice only (locked, future: expanded) | Matches RO classic, UI scaffolded for expansion |

---

**Last Updated**: 2026-03-03
**Version**: 1.0
**Status**: Plan — Awaiting Approval

