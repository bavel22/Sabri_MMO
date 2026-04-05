# Cascade Chat Conversation

  Note: _This is purely the output of the chat conversation and does not contain any raw data, codebase snippets, etc. used to generate the output._

### User Input

@[conversation:"Combat System Debugging"] lets add functionality for being able to attack/kill enemies. update their health, and health bars, and every other interacting between player and enemy. give me detaield step by step instructions for the blueprints.

### Planner Response

Let me first review the referenced conversation and the current codebase to understand what's already implemented.

*Searched trajectory*

*Found code context*

### Planner Response

Let me gather the full picture of what's already implemented server-side and what Blueprint structures exist.

*Viewed [index.js](file:///C:/Sabri_MMO/server/src/index.js) *

*Viewed [SocketIO_RealTime_Multiplayer.md](file:///C:/Sabri_MMO/docs/SocketIO_RealTime_Multiplayer.md) *

*Viewed [README.md](file:///C:/Sabri_MMO/README.md) *

### Planner Response

Let me check existing Blueprint documentation and the full enemy combat flow in the server code.

*Searched filesystem*

*Viewed [UI_Widgets.md](file:///C:/Sabri_MMO/docs/UI_Widgets.md) *

*Viewed [BP_MMOCharacter.md](file:///C:/Sabri_MMO/docs/BP_MMOCharacter.md) *

*Viewed [BP_OtherPlayerCharacter.md](file:///C:/Sabri_MMO/docs/BP_OtherPlayerCharacter.md) *

*Viewed [BP_OtherPlayerManager.md](file:///C:/Sabri_MMO/docs/BP_OtherPlayerManager.md) *

*Viewed [index.js](file:///C:/Sabri_MMO/server/src/index.js) *

*Invoked skill: ui-architect*

### Planner Response

Now I have a complete picture of the server-side enemy combat system and the existing Blueprint architecture. Let me create a comprehensive plan and provide detailed instructions.

*Updated todo list*

### Planner Response

Here is the complete, detailed step-by-step Blueprint instruction guide for the full **Player vs Enemy Combat System**. The server already handles all the logic — this is purely client-side Blueprint work.

---

# Player vs Enemy Combat System — Blueprint Instructions

## Server Events Reference (Already Implemented)

| Event | Direction | Key Fields |
|-------|-----------|------------|
| `enemy:spawn` | Server→Client | `enemyId, templateId, name, level, health, maxHealth, x, y, z` |
| `enemy:health_update` | Server→Client | `enemyId, health, maxHealth, inCombat` |
| `enemy:death` | Server→Client | `enemyId, enemyName, killerId, killerName, isEnemy, exp` |
| `combat:attack` | Client→Server | `targetEnemyId` (for enemies) |
| `combat:damage` | Server→Client | `attackerId, targetId, targetName, isEnemy, damage, targetHealth, targetMaxHealth, ...` |
| `combat:auto_attack_started` | Server→Client | `targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs` |
| `combat:auto_attack_stopped` | Server→Client | `reason` |
| `combat:target_lost` | Server→Client | `reason, isEnemy` |
| `combat:out_of_range` | Server→Client | `targetId, isEnemy, targetX, targetY, targetZ, distance, requiredRange` |

---

# PART 1: Create BP_EnemyCharacter

This is the actor that represents each enemy in the world. Very similar to BP_OtherPlayerCharacter but for enemies.

## Step 1.1: Create the Blueprint

1. In Content Browser → right-click → **Blueprint Class** → select **Character** as parent
2. Name it: `BP_EnemyCharacter`
3. Save to: `Content/Blueprints/BP_EnemyCharacter`

## Step 1.2: Add Components

Open BP_EnemyCharacter and add these components in the **Components** panel:

1. **Mesh** (inherited from Character) — in Details panel:
   - **Skeletal Mesh Asset**: Pick an enemy mesh (or temporarily use the same mesh as BP_MMOCharacter)
   - **Animation Mode**: Use Animation Blueprint
   - **Anim Class**: `ABP_unarmed` (or a dedicated enemy ABP if you have one)
   - **Location**: `(0, 0, -90)` relative to capsule

2. Click **Add Component** → **Widget Component** → name it `NameTagWidget`
   - **Details → User Interface → Widget Class**: `WBP_PlayerNameTag`
   - **Space**: Screen
   - **Draw at Desired Size**: ✓ Checked
   - **Transform → Location Z**: `120` (above head)

3. Click **Add Component** → **Widget Component** → name it `HealthBarWidget`
   - **Details → User Interface → Widget Class**: `WBP_TargetHealthBar`
   - **Space**: Screen
   - **Draw at Desired Size**: ✓ Checked
   - **Transform → Location Z**: `140` (above name tag)
   - **Visibility**: **Hidden** (default — only show when in combat)

4. Click **Add Component** → **Widget Component** → name it `HoverOverIndicator`
   - **Details → User Interface → Widget Class**: `WBP_PlayerNameTag` (or a dedicated selection ring widget)
   - **Space**: Screen
   - **Draw at Desired Size**: ✓ Checked
   - **Transform → Location Z**: `0` (at feet)
   - **Visibility**: **Hidden** (default)

## Step 1.3: CharacterMovement Settings

Select the **CharacterMovement** component → Details panel:

| Setting | Value | Section |
|---------|-------|---------|
| **Max Walk Speed** | `200` | Character Movement: Walking |
| **Max Acceleration** | `1024` | Character Movement: Walking |
| **Orient Rotation to Movement** | ✓ Checked | Rotation Settings |

## Step 1.4: Class Defaults

Click **Class Defaults** (top toolbar) → Details panel:

| Setting | Value |
|---------|-------|
| **Auto Possess AI** | Spawned |
| **AI Controller Class** | AIController |
| **Use Controller Rotation Yaw** | ✗ Unchecked |

## Step 1.5: Variables

Open **My Blueprint** panel → Variables → click **+** for each:

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `EnemyId` | Integer | 0 | Unique server-assigned ID |
| `EnemyName` | String | "" | Display name (e.g., "Blobby") |
| `EnemyLevel` | Integer | 1 | Display level |
| `TargetPosition` | Vector | (0,0,0) | Current movement target |
| `Health` | Float | 0 | Current health |
| `MaxHealth` | Float | 0 | Max health |
| `bIsDead` | Boolean | false | Death state (renamed from `IsDead` in Phase 1.3) |
| `bIsTargeted` | Boolean | false | Currently targeted (renamed from `IsTargeted` in Phase 1.3) |
| `bIsActiveTarget` | Boolean | false | Currently selected by local player (renamed from `IsActiveTarget` in Phase 1.3) |
| `bIsMoving` | Boolean | false | Optimization flag — skips Tick movement when stationary (Phase 2.1) |

## Step 1.6: Event BeginPlay

In the **Event Graph**:

```
Event BeginPlay
    ↓
Get Actor Location → Set TargetPosition
```

**Node-by-node:**
1. Find `Event BeginPlay` (already exists)
2. Drag exec pin → right-click → search **"Get Actor Location"**
3. From the Return Value → drag → right-click → search **"Set TargetPosition"** (your variable)

## Step 1.7: Event Tick — Movement Toward TargetPosition (with bIsMoving Optimization)

Same movement logic as BP_OtherPlayerCharacter, with bIsMoving guard:

```
Event Tick
    ↓
Branch: bIsDead?
    ├─ TRUE: (do nothing)
    └─ FALSE:
        Branch: bIsMoving?
            ├─ FALSE: (skip all movement logic)
            └─ TRUE:
                Get Actor Location → CurrentPos
                    ↓
                TargetPosition - CurrentPos → Direction
                    ↓
                Break Vector → Make Vector(X, Y, 0.0) → FlatDirection
                    ↓
                Vector Length(FlatDirection) → Distance
                    ↓
                Branch: Distance > 10.0?
                    ├─ TRUE: Normalize(FlatDirection) → Add Movement Input (WorldDirection, Scale=1.0)
                    └─ FALSE: Set bIsMoving = false (arrived at target)
```

> **Phase 1.2 Note:** Debug PrintString nodes ("current positon:" and "target position:") that fired every frame were removed from this Tick.

**Node-by-node:**
1. From `Event Tick` exec → drag → **Branch** → Condition: drag `bIsDead` variable
2. **FALSE** exec → **Branch** → Condition: drag `bIsMoving` variable
3. **TRUE** exec → **Get Actor Location** → promote to local var or just use the pin
3. Drag `TargetPosition` from My Blueprint
4. Right-click → search **"Subtract (Vector)"** → plug `TargetPosition` into A and `CurrentPos` into B
5. **Break Vector** the result → connect X and Y to a **Make Vector** node (Z pin = `0.0`)
6. From Make Vector → **Vector Length** → promote to local `Distance`
7. **Branch**: `Distance > 10.0`
8. TRUE → **Normalize** the FlatDirection vector → **Add Movement Input** (WorldDirection = normalized, ScaleValue = `1.0`)

## Step 1.8: Hover Events (Mouse-Over Selection)

These require cursor events to be enabled. In the **Capsule Component** details:
- **Generate Overlap Events**: ✓ Checked

In **Class Defaults** or the root component:
- Make sure the collision preset responds to **Visibility** trace channel

**Event Graph — Cursor Over:**

```
Event OnBeginCursorOver
    ↓
HoverOverIndicator → Set Visibility (New Visibility: Visible)
```

```
Event OnEndCursorOver
    ↓
Branch: IsActiveTarget?
    ├─ TRUE: (do nothing — keep visible)
    └─ FALSE: HoverOverIndicator → Set Visibility (New Visibility: Hidden)
```

**Node-by-node:**
1. Right-click Event Graph → search **"Event OnBeginCursorOver"**
2. Drag exec → drag `HoverOverIndicator` from Components panel → **Set Visibility** → New Visibility: **Visible**
3. Right-click → search **"Event OnEndCursorOver"**
4. Drag exec → **Branch** → Condition: `IsActiveTarget`
5. FALSE exec → `HoverOverIndicator` → **Set Visibility** → New Visibility: **Hidden**

## Step 1.9: Functions

### Function: `InitializeEnemy`

1. My Blueprint → Functions → **+** → Name: `InitializeEnemy`
2. Inputs:
   - `InEnemyId` (Integer)
   - `InName` (String)
   - `InLevel` (Integer)
   - `InHealth` (Float)
   - `InMaxHealth` (Float)

**Inside the function:**

```
Entry (InEnemyId, InName, InLevel, InHealth, InMaxHealth)
    ↓
Set EnemyId = InEnemyId
Set EnemyName = InName
Set EnemyLevel = InLevel
Set Health = InHealth
Set MaxHealth = InMaxHealth
Set bIsDead = false
Set bIsMoving = true  ← (Phase 2.1: start moving after initialization)
    ↓
NameTagWidget → Get User Widget Object → Cast to WBP_PlayerNameTag
    → Set Player Name: InName + " Lv." + InLevel (use Append nodes)
    ↓
HealthBarWidget → Get User Widget Object → Cast to WBP_TargetHealthBar
    → UpdateHealth(InHealth, InMaxHealth)
```

### Function: `UpdateEnemyHealth`

1. My Blueprint → Functions → **+** → Name: `UpdateEnemyHealth`
2. Inputs: `NewHealth` (Float), `NewMaxHealth` (Float), `InCombat` (Boolean)

**Inside:**

```
Entry (NewHealth, NewMaxHealth, InCombat)
    ↓
Set Health = NewHealth
Set MaxHealth = NewMaxHealth
    ↓
HealthBarWidget → Set Visibility: (InCombat ? Visible : Hidden)
    ↓
HealthBarWidget → Get User Widget Object → Cast to WBP_TargetHealthBar
    → UpdateHealth(NewHealth, NewMaxHealth)
```

**Node-by-node for visibility:**
1. Drag `InCombat` → **Select** node (Boolean) → True: `Visible`, False: `Hidden`
2. Wire the output to `HealthBarWidget` → **Set Visibility** → New Visibility pin

### Function: `OnEnemyDeath`

1. My Blueprint → Functions → **+** → Name: `OnEnemyDeath`
2. No inputs needed

**Inside:**

```
Entry
    ↓
Set bIsDead = true
Set bIsActiveTarget = false
Set bIsMoving = false
    ↓
HealthBarWidget → Set Visibility: Hidden
HoverOverIndicator → Set Visibility: Hidden
NameTagWidget → Set Visibility: Hidden
```

**Compile & Save** BP_EnemyCharacter.

---

# PART 2: Create BP_EnemyManager

Singleton manager for all enemies — mirrors BP_OtherPlayerManager but uses `EnemyId` keys.

## Step 2.1: Create the Blueprint

1. Content Browser → right-click → **Blueprint Class** → parent: **Actor**
2. Name: `BP_EnemyManager`
3. Save to: `Content/Blueprints/BP_EnemyManager`

## Step 2.2: Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `EnemyClass` | Class (Actor) | `BP_EnemyCharacter` | Class to spawn |
| `EnemyMap` | **Map** (Integer → Actor) | Empty | Maps enemyId → spawned actor |

**To create a Map variable:**
1. Click **+** next to Variables → Name: `EnemyMap`
2. In Details panel → Variable Type → click the type dropdown → select **Integer**
3. Click the small icon next to the type → change from "Single" to **Map**
4. The Value type → set to **Actor** (Object Reference)

## Step 2.3: Function — `SpawnOrUpdateEnemy`

1. My Blueprint → Functions → **+** → Name: `SpawnOrUpdateEnemy`
2. Inputs:
   - `InEnemyId` (Integer)
   - `InName` (String)
   - `InLevel` (Integer)
   - `InHealth` (Float)
   - `InMaxHealth` (Float)
   - `InX` (Float)
   - `InY` (Float)
   - `InZ` (Float)
3. Outputs:
   - `EnemyActor` (BP_EnemyCharacter Object Reference)

**Inside:**

```
Entry (InEnemyId, InName, InLevel, InHealth, InMaxHealth, InX, InY, InZ)
    ↓
EnemyMap → Find (Key: InEnemyId) → Return Value, Was Found
    ↓
Branch: Was Found?
    ├─ TRUE (existing enemy — update position):
    │   Cast Return Value to BP_EnemyCharacter → EnemyRef
    │       ↓
    │   EnemyRef → Set TargetPosition (Make Vector: InX, InY, InZ)
    │   EnemyRef → Set IsDead = false
    │   EnemyRef → Set Health = InHealth
    │   EnemyRef → Set MaxHealth = InMaxHealth
    │   EnemyRef → Set EnemyActor (return) = EnemyRef
    │
    └─ FALSE (new enemy — spawn):
        Make Vector (InX, InY, InZ) → Make Transform (Location)
            ↓
        Spawn Actor from Class
            Class: EnemyClass
            Spawn Transform: the Make Transform above
            ↓
        Cast spawned actor to BP_EnemyCharacter → NewEnemy
            ↓
        NewEnemy → InitializeEnemy(InEnemyId, InName, InLevel, InHealth, InMaxHealth)
        NewEnemy → Set TargetPosition (Make Vector: InX, InY, InZ)
            ↓
        EnemyMap → Add (Key: InEnemyId, Value: NewEnemy)
            ↓
        Set EnemyActor (return) = NewEnemy
```

**Critical**: The `Spawn Actor from Class` must use **Make Transform** with `InX, InY, InZ` as the location — NOT spawn at origin!

## Step 2.4: Function — `GetEnemyActor`

1. My Blueprint → Functions → **+** → Name: `GetEnemyActor`
2. Inputs: `InEnemyId` (Integer)
3. Outputs: `EnemyActor` (BP_EnemyCharacter Object Reference), `WasFound` (Boolean)

**Inside:**

```
Entry (InEnemyId)
    ↓
EnemyMap → Find (Key: InEnemyId) → Return Value, Was Found
    ↓
Branch: Was Found?
    ├─ TRUE: Cast to BP_EnemyCharacter → Set EnemyActor = result, Set WasFound = true
    └─ FALSE: Set WasFound = false
```

## Step 2.5: Function — `RemoveEnemy`

1. My Blueprint → Functions → **+** → Name: `RemoveEnemy`
2. Inputs: `InEnemyId` (Integer)

**Inside:**

```
Entry (InEnemyId)
    ↓
EnemyMap → Find (Key: InEnemyId) → Return Value, Was Found
    ↓
Branch: Was Found?
    ├─ TRUE:
    │   Cast to BP_EnemyCharacter → OnEnemyDeath()
    │   Destroy Actor (Return Value)
    │   EnemyMap → Remove (Key: InEnemyId)
    └─ FALSE: Print String: "Enemy not found for removal"
```

## Step 2.6: Place in Level

1. Drag `BP_EnemyManager` into the **Enter World** level (same level as BP_SocketManager and BP_OtherPlayerManager)
2. Only ONE instance

**Compile & Save** BP_EnemyManager.

---

# PART 3: Bind Enemy Events in BP_SocketManager

## Step 3.1: Add New Variables

Open **BP_SocketManager** → My Blueprint → Variables → add:

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `TargetEnemyId` | Integer | 0 | Currently targeted enemy's ID |
| `IsTargetingEnemy` | Boolean | false | True if target is an enemy (not a player) |

## Step 3.2: Bind Events in OnSocketConnected

In the `OnSocketConnected` event chain (where you already bind `player:moved`, `player:left`, `combat:damage`, etc.), add these **3 new bindings** at the end:

```
... (existing bindings)
    ↓
SocketIOClient → Bind Event to Function
    Event Name: "enemy:spawn"
    Function Name: "OnEnemySpawn"
    ↓
SocketIOClient → Bind Event to Function
    Event Name: "enemy:death"
    Function Name: "OnEnemyDeath"
    ↓
SocketIOClient → Bind Event to Function
    Event Name: "enemy:health_update"
    Function Name: "OnEnemyHealthUpdate"
```

**Node-by-node (repeat for each):**
1. Drag from the last exec pin in the binding chain
2. Right-click → search **"Bind Event to Function"** (SocketIO Functions category)
3. **Event Name** pin: type the event name string
4. **Function Name** pin: type the function name string

## Step 3.3: Create `OnEnemySpawn` Function

1. My Blueprint → Functions → **+** → Name: `OnEnemySpawn`
2. Inputs: **Name** = `Data`, **Type** = `String` (MUST be exactly "Data")

**Inside:**

```
Entry (Data: String)
    ↓
Print String: "OnEnemySpawn: " + Data
    ↓
Value From Json String (Data) → As Object
    ↓
Try Get String Field ("enemyId") → To Integer → EnemyId
Try Get String Field ("name") → EnemyName
Try Get String Field ("level") → To Integer → EnemyLevel
Try Get String Field ("health") → To Float → EnemyHealth
Try Get String Field ("maxHealth") → To Float → EnemyMaxHealth
Try Get String Field ("x") → To Float → SpawnX
Try Get String Field ("y") → To Float → SpawnY
Try Get String Field ("z") → To Float → SpawnZ
    ↓
Get Actor of Class → BP_EnemyManager → Cast to BP_EnemyManager
    ↓
SpawnOrUpdateEnemy(EnemyId, EnemyName, EnemyLevel, EnemyHealth, EnemyMaxHealth, SpawnX, SpawnY, SpawnZ)
```

**Node-by-node:**
1. From Entry exec → **Print String** (for debugging)
2. Right-click → search **"Value From Json String"** (SIOJ|Json category) → plug `Data`
3. From Return Value → **As Object**
4. From the Object → **Try Get String Field** → Field Name: `"enemyId"` → from String output → **To Integer (int)** → promote to local variable `EnemyId`
5. Repeat for each field (name, level, health, maxHealth, x, y, z)
6. Right-click → **Get Actor of Class** → select `BP_EnemyManager`
7. **Cast to BP_EnemyManager** → call **SpawnOrUpdateEnemy** with all parsed values

## Step 3.4: Create `OnEnemyDeath` Function

1. My Blueprint → Functions → **+** → Name: `OnEnemyDeath`
2. Inputs: `Data` (String)

**Inside:**

```
Entry (Data: String)
    ↓
Print String: "OnEnemyDeath: " + Data
    ↓
Value From Json String (Data) → As Object
    ↓
Try Get String Field ("enemyId") → To Integer → DeadEnemyId
Try Get String Field ("enemyName") → DeadEnemyName
Try Get String Field ("killerId") → To Integer → KillerId
Try Get String Field ("killerName") → KillerName
Try Get String Field ("exp") → To Integer → ExpGained
    ↓
Get Actor of Class → BP_EnemyManager → Cast to BP_EnemyManager
    → GetEnemyActor(DeadEnemyId) → EnemyActor, WasFound
        ↓
    Branch: WasFound?
        ├─ TRUE:
        │   EnemyActor → OnEnemyDeath()
        │
        │   ★ If this enemy was our target, clear targeting:
        │   Branch: TargetEnemyId == DeadEnemyId?
        │       ├─ TRUE:
        │       │   Set TargetEnemyId = 0
        │       │   Set IsTargetingEnemy = false
        │       │   Set bIsAutoAttacking = false
        │       │   Get GameHudRef → HideTargetFrame
        │       └─ FALSE: (skip)
        │
        └─ FALSE: (enemy not found, skip)
    ↓
★ If LOCAL player was the killer, show EXP notification:
Get Game Instance → MMOGameInstance → GetCurrentCharacterId → LocalCharId
    ↓
Branch: KillerId == LocalCharId?
    ├─ TRUE: Print String: "You gained " + ExpGained + " EXP!" (or show in chat)
    └─ FALSE: (skip)
```

**Note:** Do NOT call `RemoveEnemy`/`DestroyActor` here — the enemy respawns after a timer. Instead just call `OnEnemyDeath()` which hides the widgets and sets `IsDead = true`. When `enemy:spawn` fires again with the same ID, `SpawnOrUpdateEnemy` will reset it.

## Step 3.5: Create `OnEnemyHealthUpdate` Function

1. My Blueprint → Functions → **+** → Name: `OnEnemyHealthUpdate`
2. Inputs: `Data` (String)

**Inside:**

```
Entry (Data: String)
    ↓
Value From Json String (Data) → As Object
    ↓
Try Get String Field ("enemyId") → To Integer → EnemyId
Try Get String Field ("health") → To Float → NewHealth
Try Get String Field ("maxHealth") → To Float → NewMaxHealth
Try Get String Field ("inCombat") → To Boolean → InCombat
    ↓
Get Actor of Class → BP_EnemyManager → Cast to BP_EnemyManager
    → GetEnemyActor(EnemyId) → EnemyActor, WasFound
        ↓
    Branch: WasFound?
        ├─ TRUE:
        │   EnemyActor → UpdateEnemyHealth(NewHealth, NewMaxHealth, InCombat)
        │
        │   ★ Also update HUD target frame if this is our current target:
        │   Branch: TargetEnemyId == EnemyId?
        │       ├─ TRUE:
        │       │   Get GameHudRef → UpdateTargetHealth(EnemyActor.EnemyName, NewHealth, NewMaxHealth, 0, 0)
        │       └─ FALSE: (skip)
        │
        └─ FALSE: (skip)
```

**For `inCombat` boolean parsing**: The `Try Get String Field` returns a string `"true"` or `"false"`. To convert to Boolean:
1. From the String output → **Equal (String)** → compare against `"true"`
2. The Boolean result is your `InCombat` value

**Compile & Save** BP_SocketManager.

---

# PART 4: Click-to-Target Enemies & Emit `combat:attack`

This is where the player clicks an enemy to start attacking. The logic goes in **BP_MMOCharacter** (or wherever your click-handling currently lives).

## Step 4.1: Left-Click Hit Detection

You likely already have a left-click handler for click-to-move. You need to add a check: "Did I click on an enemy?"

**In BP_MMOCharacter Event Graph (or wherever your IA_ClickToMove / left-click input action is):**

After the input fires, before doing click-to-move:

```
Input Action: IA_ClickToMove (or Left Mouse Button)
    ↓
Get Player Controller → Get Hit Result Under Cursor by Channel (Visibility)
    → Break Hit Result → Hit Actor
        ↓
    Is Valid (Hit Actor)?
        ├─ FALSE: (do regular click-to-move)
        └─ TRUE:
            ★ TRY CAST TO ENEMY:
            Cast to BP_EnemyCharacter (Hit Actor)
                ├─ SUCCESS:
                │   ↓
                │   Get EnemyId from cast result → TargetEnemyId (local variable)
                │       ↓
                │   ★ Tell BP_SocketManager to attack this enemy:
                │   Get Actor of Class → BP_SocketManager → Cast
                │       ↓
                │   Set TargetEnemyId = TargetEnemyId (on SocketManager)
                │   Set IsTargetingEnemy = true
                │       ↓
                │   ★ Emit combat:attack with targetEnemyId:
                │   Construct Json Object
                │       → Set String Field ("targetEnemyId", TargetEnemyId → To String)
                │       → Construct Json Object Value
                │       → Get SocketIO → Emit ("combat:attack", Json Value)
                │       ↓
                │   ★ Show target frame on HUD:
                │   Get GameHudRef → UpdateTargetHealth(
                │       EnemyRef.EnemyName + " Lv." + EnemyRef.EnemyLevel,
                │       EnemyRef.Health,
                │       EnemyRef.MaxHealth,
                │       0, 0)
                │       ↓
                │   ★ Set HoverOverIndicator visible on enemy:
                │   EnemyRef → Set IsActiveTarget = true
                │   EnemyRef → HoverOverIndicator → Set Visibility: Visible
                │
                ├─ FAIL (not an enemy):
                │   ★ TRY CAST TO OTHER PLAYER:
                │   Cast to BP_OtherPlayerCharacter (Hit Actor)
                │       ├─ SUCCESS: (existing player targeting logic — emit with targetCharacterId)
                │       └─ FAIL: (empty click — do click-to-move, clear target)
```

**CRITICAL**: When emitting `combat:attack` for an enemy, the JSON field MUST be `"targetEnemyId"` (not `"targetCharacterId"`). The server checks `targetEnemyId` first to route to enemy combat logic.

## Step 4.2: Clear Previous Target Before Setting New One

Before setting the new target, clear the old one:

```
★ BEFORE the cast chain above, insert:
    ↓
Get Actor of Class → BP_SocketManager → Cast
    ↓
Branch: IsTargetingEnemy AND TargetEnemyId > 0?
    ├─ TRUE:
    │   Get Actor of Class → BP_EnemyManager → Cast
    │       → GetEnemyActor(old TargetEnemyId) → OldEnemy, WasFound
    │       → Branch: WasFound?
    │           └─ TRUE: OldEnemy → Set IsActiveTarget = false
    │                    OldEnemy → HoverOverIndicator → Set Visibility: Hidden
    └─ FALSE: (skip)
```

This ensures only one enemy has the selection ring visible at a time.

---

# PART 5: Handle `combat:damage` for Enemies

Your existing `OnCombatDamage` handler (bound to `combat:damage`) already handles player-vs-player. Now add the enemy branch.

## Step 5.1: Modify Existing `OnCombatDamage`

After parsing the JSON fields, add a branch on `isEnemy`:

```
OnCombatDamage (Data: String)
    ↓
Parse JSON:
    attackerId → Integer
    targetId → Integer
    targetName → String
    isEnemy → Boolean (from string "true"/"false")
    damage → Integer
    targetHealth → Float
    targetMaxHealth → Float
    ↓
Branch: isEnemy?
    ├─ TRUE (enemy was hit):
    │   ↓
    │   ★ Update enemy health bar:
    │   Get Actor of Class → BP_EnemyManager → Cast
    │       → GetEnemyActor(targetId) → EnemyActor, WasFound
    │       → Branch: WasFound?
    │           └─ TRUE:
    │               EnemyActor → UpdateEnemyHealth(targetHealth, targetMaxHealth, true)
    │
    │   ★ Update HUD target frame if this is our target:
    │   Branch: TargetEnemyId == targetId?
    │       └─ TRUE:
    │           Get GameHudRef → UpdateTargetHealth(targetName, targetHealth, targetMaxHealth, 0, 0)
    │
    │   ★ Spawn damage number (optional):
    │   Branch: WasFound?
    │       └─ TRUE:
    │           EnemyActor → Get Actor Location → spawn WBP_DamageNumber at location
    │
    │   ★ Play attack animation on LOCAL player (if we're the attacker):
    │   Get Game Instance → MMOGameInstance → GetCurrentCharacterId → LocalCharId
    │   Branch: attackerId == LocalCharId?
    │       └─ TRUE:
    │           Get Player Character → Get Mesh → Get Anim Instance
    │               → Montage Play (your attack montage, e.g. AM_Attack)
    │
    │   ★ Play attack animation on REMOTE player (if a remote player is attacking):
    │   Branch: attackerId != LocalCharId?
    │       └─ TRUE:
    │           (Use FindRemotePlayer(attackerId) to get BP_OtherPlayerCharacter
    │            → Get Mesh → Get Anim Instance → Montage Play)
    │
    └─ FALSE (player was hit):
        (your existing player-vs-player damage handling)
```

**For `isEnemy` boolean parsing:**
1. `Try Get String Field ("isEnemy")` → String result
2. **Equal (String)**: compare against `"true"` → Boolean output

---

# PART 6: Handle `combat:out_of_range` for Enemies

Your existing `OnCombatOutOfRange` handler needs to support enemy targets too. When out of range, the local player should auto-walk toward the enemy.

## Step 6.1: Modify `OnCombatOutOfRange`

```
OnCombatOutOfRange (Data: String)
    ↓
Parse JSON:
    targetId → Integer
    isEnemy → Boolean
    targetX → Float
    targetY → Float
    targetZ → Float
    requiredRange → Float
    ↓
★ Move the local player character toward the target position:
Get Player Character → Simple Move to Location
    (or use your existing pathfinding/click-to-move system)
    Destination: Make Vector (targetX, targetY, targetZ)
```

The server will keep sending `combat:out_of_range` each combat tick until the player is close enough. Once in range, it automatically starts dealing damage (no need to re-emit `combat:attack`).

**Node-by-node:**
1. Parse JSON as usual
2. Right-click → search **"Get Player Controller"** (not Get Player Character)
3. From Player Controller → search **"Simple Move to Location"**
   - **Goal**: Make Vector (targetX, targetY, targetZ)
4. This uses NavMesh pathfinding to walk toward the enemy

---

# PART 7: Handle `combat:auto_attack_started` & `combat:auto_attack_stopped`

## Step 7.1: `OnAutoAttackStarted`

If you don't already have this handler, create it and bind to `combat:auto_attack_started`:

```
OnAutoAttackStarted (Data: String)
    ↓
Parse JSON:
    targetId → Integer
    targetName → String
    isEnemy → Boolean
    attackRange → Float
    ↓
Set bIsAutoAttacking = true
    ↓
Branch: isEnemy?
    ├─ TRUE:
    │   Set TargetEnemyId = targetId
    │   Set IsTargetingEnemy = true
    │   ↓
    │   Get GameHudRef → UpdateTargetHealth(targetName, ...) [show target frame]
    │   ↓
    │   Get BP_EnemyManager → GetEnemyActor(targetId) → EnemyActor
    │       → EnemyActor → Set IsActiveTarget = true
    │       → HoverOverIndicator → Set Visibility: Visible
    └─ FALSE:
        (existing player target logic)
        Set IsTargetingEnemy = false
```

## Step 7.2: `OnAutoAttackStopped` and `OnTargetLost`

Both should clear enemy target state:

```
OnAutoAttackStopped / OnTargetLost (Data: String)
    ↓
Set bIsAutoAttacking = false
    ↓
★ Clear enemy target if applicable:
Branch: IsTargetingEnemy AND TargetEnemyId > 0?
    ├─ TRUE:
    │   Get BP_EnemyManager → GetEnemyActor(TargetEnemyId) → EnemyActor, WasFound
    │       → Branch: WasFound?
    │           └─ TRUE:
    │               EnemyActor → Set bIsActiveTarget = false
    │               EnemyActor → HoverOverIndicator → Set Visibility: Hidden
    └─ FALSE: (skip)
    ↓
Set TargetEnemyId = 0
Set IsTargetingEnemy = false
    ↓
Get GameHudRef → HideTargetFrame
```

---

# PART 8: Handle `combat:stop_attack` (Player Manually Stops)

When the player clicks on empty ground (not an enemy/player), they should stop attacking:

## Step 8.1: In BP_MMOCharacter Click-to-Move

When the player clicks and the hit result is NOT an enemy or player:

```
... (after Cast to BP_EnemyCharacter FAILS and Cast to BP_OtherPlayerCharacter FAILS)
    ↓
★ Empty ground clicked — stop attacking if currently auto-attacking:
Get Actor of Class → BP_SocketManager → Cast
    ↓
Branch: bIsAutoAttacking?
    ├─ TRUE:
    │   Get SocketIO → Emit ("combat:stop_attack", empty Json)
    │   (Server will respond with combat:auto_attack_stopped)
    └─ FALSE: (just do click-to-move)
    ↓
(continue with regular click-to-move pathfinding)
```

---

# PART 9: Enemy Respawn Handling

When an enemy respawns, the server emits `enemy:spawn` again with the same `enemyId`. Your existing `OnEnemySpawn` handler calls `SpawnOrUpdateEnemy`, which finds the existing entry in `EnemyMap` and resets it.

## Step 9.1: Update `SpawnOrUpdateEnemy` TRUE Branch for Respawn

In the TRUE branch (enemy already exists in map), add after setting position/health:

```
... (existing TRUE branch)
    EnemyRef → Set IsDead = false
    EnemyRef → Set Health = InHealth
    EnemyRef → Set MaxHealth = InMaxHealth
    EnemyRef → Set TargetPosition = Make Vector(InX, InY, InZ)
    ↓
★ Teleport the actor to the spawn position (don't let it walk from death spot):
    EnemyRef → Set Actor Location
        New Location: Make Vector(InX, InY, InZ)
        Sweep: false
        Teleport: true ← CRITICAL
    ↓
★ Re-show name tag:
    EnemyRef → NameTagWidget → Set Visibility: Visible
    ↓
★ Reset health bar (hidden by default, shown when combat starts):
    EnemyRef → HealthBarWidget → Set Visibility: Hidden
    EnemyRef → HealthBarWidget → Get User Widget Object → Cast to WBP_TargetHealthBar
        → UpdateHealth(InHealth, InMaxHealth)
```

---

# PART 10: Summary Checklist

| Blueprint | What to Create/Modify |
|-----------|-----------------------|
| **BP_EnemyCharacter** (NEW) | Actor with mesh, name tag, health bar, hover indicator, movement tick, click events |
| **BP_EnemyManager** (NEW) | Singleton manager with EnemyMap, SpawnOrUpdateEnemy, GetEnemyActor, RemoveEnemy |
| **BP_SocketManager** (MODIFY) | Add `TargetEnemyId`, `IsTargetingEnemy` vars. Bind `enemy:spawn`, `enemy:death`, `enemy:health_update`. Create OnEnemySpawn, OnEnemyDeath, OnEnemyHealthUpdate functions. Modify OnCombatDamage for `isEnemy` branch. Modify OnAutoAttackStopped/OnTargetLost for enemy cleanup |
| **BP_MMOCharacter** (MODIFY) | Left-click: Cast to BP_EnemyCharacter first, emit `combat:attack` with `targetEnemyId`. Clear target on empty click + emit `combat:stop_attack` |
| **Level** | Place BP_EnemyManager in Enter World level |

## Event Flow Summary

```
1. Player joins → server sends enemy:spawn for all existing enemies
   → OnEnemySpawn → BP_EnemyManager.SpawnOrUpdateEnemy → enemies appear in world

2. Player clicks enemy → Cast to BP_EnemyCharacter → emit combat:attack {targetEnemyId}
   → Server responds: combat:auto_attack_started

3. Server combat tick: if out of range → combat:out_of_range → player walks toward enemy
   → if in range → combat:damage broadcast

4. combat:damage (isEnemy=true) → update enemy health bar + HUD target frame + damage numbers

5. enemy:health_update → update overhead health bar visibility (inCombat flag)

6. Enemy HP reaches 0 → enemy:death broadcast → OnEnemyDeath → hide enemy widgets
   → combat:target_lost sent to all attackers → clear targeting state

7. After respawnMs → server emits enemy:spawn again → SpawnOrUpdateEnemy resets enemy
```

After completing all 10 parts, **Compile & Save** every modified Blueprint, then test by clicking on an enemy in the world!

*Updated todo list*

### Planner Response

All 10 parts of the Blueprint instructions are complete above. To recap what you need to build:

**New Blueprints to create:**
- **BP_EnemyCharacter** — Character actor with mesh, name tag, health bar, hover indicator, movement tick, and cursor events
- **BP_EnemyManager** — Singleton manager with `EnemyMap` (Integer → Actor), spawn/update/lookup/remove functions

**Existing Blueprints to modify:**
- **BP_SocketManager** — Bind 3 new events (`enemy:spawn`, `enemy:death`, `enemy:health_update`), add `isEnemy` branch to `OnCombatDamage`, update `OnAutoAttackStopped`/`OnTargetLost` for enemy cleanup
- **BP_MMOCharacter** — Left-click: Cast to `BP_EnemyCharacter` first → emit `combat:attack` with `targetEnemyId`. On empty ground click → emit `combat:stop_attack`

**Don't forget:**
- Place `BP_EnemyManager` in the Enter World level
- `combat:attack` for enemies uses field `"targetEnemyId"` (NOT `"targetCharacterId"`)
- Don't destroy enemies on death — just hide widgets + set `IsDead=true`. Server re-emits `enemy:spawn` on respawn
- The `enemy:spawn` respawn path must use `Set Actor Location` with **Teleport = true**

Let me know when you're ready to start, if you need any part clarified in more detail, or if you hit any issues during implementation!