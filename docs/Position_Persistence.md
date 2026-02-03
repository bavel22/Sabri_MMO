# Character Position Persistence

## Overview

System for saving and loading character positions in the game world. Enables players to resume gameplay from their last saved location.

## Features

- **Auto-save**: Position saved every 5 seconds while playing
- **Load on spawn**: Characters appear at saved position when entering world
- **Fallback handling**: New characters spawn at Player Start location
- **Database storage**: Persistent storage in PostgreSQL

## Architecture

```
Level Blueprint (Timer: 5s)
    ↓
Get Player Location
    ↓
HttpManager::SaveCharacterPosition()
    ↓
PUT /api/characters/:id/position (JWT)
    ↓
PostgreSQL UPDATE
```

## Data Flow

### Save Position Flow

1. **Timer Trigger** (Level Blueprint)
   - Every 5 seconds
   - Gets player character location
   - Calls HTTP endpoint

2. **HTTP Request**
   ```cpp
   HttpManager::SaveCharacterPosition(
       WorldContextObject,
       CharacterId,
       X, Y, Z
   );
   ```

3. **Server Processing**
   ```javascript
   app.put('/api/characters/:id/position', authenticateToken, async (req, res) => {
       const { x, y, z } = req.body;
       // Validate coordinates are numbers
       // Verify character belongs to user
       // UPDATE characters SET x=$1, y=$2, z=$3 WHERE character_id=$4
   });
   ```

4. **Database Update**
   ```sql
   UPDATE characters SET x = $1, y = $2, z = $3 WHERE character_id = $4
   ```

### Load Position Flow

1. **Character Select**
   - User selects character in WBP_CharacterSelect
   - Character data (including X, Y, Z) stored in GameInstance

2. **Enter World**
   - User clicks "Enter World" button
   - Level loads (ThirdPersonMap)

3. **Spawn Logic** (Level Blueprint)
   ```
   Event BeginPlay
       ↓
   Get Game Instance → Get SelectedCharacter
       ↓
   Check: X==0 AND Y==0 AND Z==0?
       ├─ TRUE → Use Player Start position
       └─ FALSE → Use Saved Position (X, Y, Z)
   ```

## Blueprint Implementation

### Level Blueprint (ThirdPersonMap)

**Auto-Save Timer Setup:**
```
Event BeginPlay
    ↓
Set Timer by Function Name
    ├─ Function Name: "SaveCharacterPosition"
    ├─ Time: 5.0
    ├─ Looping: true
    └─ Initial Start Delay: 5.0
```

**SaveCharacterPosition Function:**
```
Custom Event: SaveCharacterPosition
    ↓
Get Player Character
    ↓
Get Actor Location → Break Vector (X, Y, Z)
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Get SelectedCharacter → CharacterId
    ↓
HttpManager::SaveCharacterPosition(CharacterId, X, Y, Z)
```

**Spawn Character at Level Start:**
```
Event BeginPlay
    ↓
Delay (0.5s)  [Wait for level to fully load]
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Get SelectedCharacter
    ↓
Branch (CharacterData.X == 0 AND 
        CharacterData.Y == 0 AND 
        CharacterData.Z == 0)
    ├─ TRUE:
    │   Get Player Start → Get Transform
    │   Spawn Actor (Character Class)
    │   Possess (Get Player Controller)
    └─ FALSE:
        Make Vector (X, Y, Z)
        Spawn Actor at Location
        Possess (Get Player Controller)
```

## Server Endpoint

### PUT /api/characters/:id/position

**Headers:**
```
Authorization: Bearer <jwt_token>
Content-Type: application/json
```

**Request:**
```json
{
    "x": 123.45,
    "y": 67.89,
    "z": 0.00
}
```

**Validation:**
```javascript
if (typeof x !== 'number' || typeof y !== 'number' || typeof z !== 'number') {
    return res.status(400).json({ 
        error: 'Invalid coordinates. x, y, z must be numbers' 
    });
}
```

**Success Response (200):**
```json
{
    "message": "Position saved successfully",
    "position": { "x": 123.45, "y": 67.89, "z": 0.00 }
}
```

**Security:**
- JWT authentication required
- Character must belong to authenticated user
```javascript
const charCheck = await pool.query(
    'SELECT character_id FROM characters WHERE character_id = $1 AND user_id = $2',
    [characterId, req.user.user_id]
);
```

## Database Schema

```sql
CREATE TABLE characters (
    character_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id),
    name VARCHAR(50) NOT NULL,
    class VARCHAR(20) DEFAULT 'warrior',
    level INTEGER DEFAULT 1,
    x FLOAT DEFAULT 0,      -- Position X coordinate
    y FLOAT DEFAULT 0,      -- Position Y coordinate
    z FLOAT DEFAULT 0,      -- Position Z coordinate
    health INTEGER DEFAULT 100,
    mana INTEGER DEFAULT 100,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## C++ Implementation

### HttpManager

**Declaration:**
```cpp
UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void SaveCharacterPosition(
    UObject* WorldContextObject, 
    int32 CharacterId, 
    float X, 
    float Y, 
    float Z
);
```

**Implementation:**
```cpp
void UHttpManager::SaveCharacterPosition(
    UObject* WorldContextObject, 
    int32 CharacterId, 
    float X, 
    float Y, 
    float Z
)
{
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindStatic(
        &UHttpManager::OnSavePositionResponse
    );
    
    FString Url = FString::Printf(
        TEXT("http://localhost:3001/api/characters/%d/position"), 
        CharacterId
    );
    Request->SetURL(Url);
    Request->SetVerb(TEXT("PUT"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // Add JWT token
    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
    {
        if (GI->IsAuthenticated())
        {
            Request->SetHeader(TEXT("Authorization"), GI->GetAuthHeader());
        }
    }
    
    // JSON payload
    FString JsonPayload = FString::Printf(
        TEXT("{\"x\":%f,\"y\":%f,\"z\":%f}"), 
        X, Y, Z
    );
    Request->SetContentAsString(JsonPayload);
    
    Request->ProcessRequest();
}
```

## Position Validation

### New Character Detection

A character is considered "new" (no saved position) when:
```
X == 0 AND Y == 0 AND Z == 0
```

### Fallback to Player Start

```cpp
// In Level Blueprint
if (CharacterData.X == 0.0f && 
    CharacterData.Y == 0.0f && 
    CharacterData.Z == 0.0f)
{
    // Use Player Start transform
    APlayerStart* PlayerStart = FindPlayerStart();
    SpawnLocation = PlayerStart->GetActorLocation();
}
else
{
    // Use saved position
    SpawnLocation = FVector(CharacterData.X, CharacterData.Y, CharacterData.Z);
}
```

## Considerations

### Performance
- Save every 5 seconds balances data safety vs. server load
- Position updates are lightweight (single row update)
- No impact on client performance (async HTTP call)

### Data Consistency
- Position saved continuously while playing
- On unexpected disconnect, max 5 seconds of progress lost
- Graceful degradation if server unavailable

### Future Enhancements
- Save on specific events ( entering buildings, completing quests)
- Zone-based position validation
- Anti-cheat position verification
- Last save timestamp tracking

## Files

### Server
- `server/src/index.js` - PUT /api/characters/:id/position endpoint

### Client C++
- `client/SabriMMO/Source/SabriMMO/HttpManager.h/.cpp`
- `client/SabriMMO/Source/SabriMMO/CharacterData.h`

### Blueprints
- `client/SabriMMO/Content/Maps/ThirdPersonMap_BuiltData.uasset` (Level Blueprint)
