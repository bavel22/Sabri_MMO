# Phase 4: Persistent Socket Connection — Execution Plan v2

**Status**: NOT STARTED
**Created**: 2026-03-10
**Based on**: Full codebase audit, FSocketIONative API research, BP_SocketManager Blueprint analysis (unrealMCP), server zone handler analysis, plugin documentation review
**Strategic Plan**: `docsNew/05_Development/Strategic_Implementation_Plan_v3.md` (Phase 4)
**Previous Plan**: `docsNew/05_Development/Persistent_Socket_Connection_Plan.md` (superseded by this file)

---

## Progress Tracker

| Sub-Phase | Status | Completed | Notes |
|-----------|--------|-----------|-------|
| **4a: SocketEventRouter** | NOT STARTED | — | Multi-dispatch layer (foundation) |
| **4b: Socket on GameInstance** | NOT STARTED | — | FSocketIONative + ConnectSocket/DisconnectSocket |
| **4c: Migrate 15 C++ Subsystems** | NOT STARTED | — | Replace FindSocketIOComponent → EventRouter |
| ↳ ZoneTransitionSubsystem | NOT STARTED | — | First subsystem (critical path) |
| ↳ CombatStatsSubsystem | NOT STARTED | — | Simple, easy to verify |
| ↳ BasicInfoSubsystem | NOT STARTED | — | Reference impl (most events) |
| ↳ BuffBarSubsystem | NOT STARTED | — | |
| ↳ DamageNumberSubsystem | NOT STARTED | — | |
| ↳ WorldHealthBarSubsystem | NOT STARTED | — | |
| ↳ CastBarSubsystem | NOT STARTED | — | |
| ↳ SkillVFXSubsystem | NOT STARTED | — | |
| ↳ InventorySubsystem | NOT STARTED | — | |
| ↳ EquipmentSubsystem | NOT STARTED | — | |
| ↳ HotbarSubsystem | NOT STARTED | — | |
| ↳ ShopSubsystem | NOT STARTED | — | |
| ↳ KafraSubsystem | NOT STARTED | — | |
| ↳ SkillTreeSubsystem | NOT STARTED | — | Most complex emitter |
| ↳ LoginFlowSubsystem | NOT STARTED | — | Detection logic update |
| **4d: BP Event Handler Bridge** | NOT STARTED | — | MultiplayerEventSubsystem + PositionBroadcastSubsystem |
| ↳ MultiplayerEventSubsystem | NOT STARTED | — | Bridge: C++ → BP_SocketManager functions |
| ↳ PositionBroadcastSubsystem | NOT STARTED | — | 30Hz position timer |
| ↳ BP_SocketManager modifications | NOT STARTED | — | Remove socket/bindings, keep handlers |
| **4e: Server + Zone Flow** | NOT STARTED | — | Remove reconnectBuffCache, verify zone:warp/ready |
| **4f: Cleanup + Docs** | NOT STARTED | — | Remove old code, update all docs |

---

## Problem Statement

Every zone change causes a full **socket disconnect → reconnect → player:join** cycle because the socket lives on `BP_SocketManager` (a per-level Actor destroyed by `OpenLevel()`). This:

1. **Loses all in-memory state**: buffs, status effects, auto-attack state, active casts, enemy aggro
2. **Spams player:left/joined**: other players see "leave and rejoin" on every zone change
3. **Triggers unnecessary DB operations**: disconnect handler saves stats/position, player:join reloads everything
4. **Blocks future social features**: party, guild, and chat state need persistence across zones
5. **Requires band-aid code**: `reconnectBuffCache` (30s TTL), stability delays, health check ticks

**Industry standard** (WoW, FFXIV, rAthena/Hercules): persistent connection at session level. Zone changes are a server-side bucket move, not a disconnect.

**Plugin author recommendation** ([getnamo/SocketIOClient-Unreal](https://github.com/getnamo/SocketIOClient-Unreal)): "Consider switching to C++ and using FSocketIONative, which doesn't depend on using an actor component" for persistent connections across level transitions.

---

## Architecture: Current → Target

```
CURRENT (per-level, destroyed on OpenLevel):
─────────────────────────────────────────────
UE5 Level (L_Prontera)
  └── BP_SocketManager (Actor)
       └── USocketIOClientComponent
            └── FSocketIONative ← DESTROYED on OpenLevel

UMMOGameInstance ← SURVIVES OpenLevel
  └── Auth token, character data, zone state (no socket)

15 C++ subsystems: FindSocketIOComponent()
  └── TActorIterator iteration (fragile, per-level)
  └── WrapSingleEvent chain (brittle, order-dependent)
  └── EventFunctionMap.Contains("combat:health_update") readiness gate

TARGET (persistent, survives OpenLevel):
────────────────────────────────────────
UMMOGameInstance ← SURVIVES OpenLevel
  └── TSharedPtr<FSocketIONative> NativeSocket ← PERSISTS
  └── USocketEventRouter* EventRouter ← Multi-dispatch

15 C++ subsystems: GI->GetEventRouter()->RegisterHandler()
  └── Direct GameInstance access (stable)
  └── RegisterHandler (multi-dispatch, no chaining)
  └── GI->IsSocketConnected() readiness check
```

---

## Critical Technical Discoveries

### 1. FSocketIONative::OnEvent() REPLACES Previous Handler

```cpp
// SocketIONative.cpp line 309:
EventFunctionMap.Add(EventName, BoundEvent);  // TMap::Add = REPLACE
```

15+ events are handled by MULTIPLE subsystems simultaneously. Current `WrapSingleEvent` works around this by capturing the previous callback in a closure chain. We need a proper multi-dispatch layer (Phase 4a).

**Multi-handler events confirmed:**

| Event | Subsystems (count) |
|-------|-------------------|
| `combat:health_update` | BasicInfo, WorldHealthBar, (BP) = 3 |
| `combat:damage` | BasicInfo, WorldHealthBar, DamageNumber, (BP) = 4 |
| `skill:effect_damage` | BasicInfo, WorldHealthBar, DamageNumber, SkillVFX, SkillTree = 5 |
| `combat:death` | BasicInfo, WorldHealthBar, (BP) = 3 |
| `combat:respawn` | BasicInfo, WorldHealthBar, (BP) = 3 |
| `player:stats` | BasicInfo, CombatStats, (WorldHealthBar) = 3 |
| `skill:cast_start` | CastBar, SkillVFX = 2 |
| `skill:cast_complete` | CastBar, SkillVFX = 2 |
| `skill:buff_applied` | BuffBar, SkillVFX = 2 |
| `skill:buff_removed` | BuffBar, SkillVFX = 2 |
| `inventory:data` | Inventory, Equipment, BasicInfo = 3 |
| `shop:bought` | Shop, BasicInfo = 2 |
| `shop:sold` | Shop, BasicInfo = 2 |

### 2. USocketIOClientComponent::NativeClient is `protected`

```cpp
// SocketIOClientComponent.h line 486-495:
protected:
    TSharedPtr<FSocketIONative> NativeClient;
```

A subclass CAN access it. But we don't need to — we use `FSocketIONative` directly on GameInstance.

### 3. BP_SocketManager Handler Functions All Take `(Data: string)`

Confirmed via unrealMCP for all 36 functions. This means the ProcessEvent bridge is simple:

```cpp
struct { FString Data; } Params;
Params.Data = JsonString;
BPSocketManager->ProcessEvent(Func, &Params);
```

### 4. Server zone:warp + zone:ready Already Work for Persistent Sockets

- `zone:warp` (lines 1986-2102): Moves player between zone buckets WITHOUT disconnect. Changes Socket.io rooms, broadcasts player:left to old zone, saves to DB.
- `zone:ready` (lines 2107-2191): Sends zone enemies, other players, metadata, buff:list. Works independently of player:join.
- No major server changes needed — the disconnect handler simply stops firing during zone transitions.

### 5. Events During Level Transition Are Safely Lost

During `OpenLevel()` (~0.5-2s), no UWorld/subsystems exist. Socket events may arrive but:
- EventRouter's `Owner` weak pointers are stale → callbacks silently skipped
- `zone:ready` (emitted after level loads) resends ALL zone data
- No data loss for any gameplay-critical information

---

## Required Skills & Documentation

### Skills to Load Before Each Phase

| Phase | Skills | Why |
|-------|--------|-----|
| 4a | `/sabrimmo-ui` | UObject creation patterns, subsystem lifecycle |
| 4b | `/full-stack`, `/realtime` | Socket.io connection, server event architecture |
| 4c | `/sabrimmo-ui`, `/realtime` | Subsystem event wrapping, emit patterns |
| 4d | `/ui-architect` | Blueprint function calls from C++, unrealMCP |
| 4e | `/full-stack` | Server index.js zone handlers |
| 4f | `/docs`, `/code-quality` | Documentation updates, code cleanup |

### Documentation References

| Document | Path | When Needed |
|----------|------|-------------|
| Strategic Plan | `docsNew/05_Development/Strategic_Implementation_Plan_v3.md` | Overall context |
| Project Overview | `docsNew/00_Project_Overview.md` | System inventory |
| BP_SocketManager Doc | `docsNew/02_Client_Side/Blueprints/02_BP_SocketManager.md` | Phase 4d (all 36 functions documented) |
| Zone Setup Guide | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` | Phase 4f (update instructions) |
| BasicInfo Subsystem Doc | `docsNew/02_Client_Side/C++_Code/08_BasicInfo_Slate_UI.md` | Phase 4c (reference subsystem pattern) |
| Buff System Doc | `docsNew/03_Server_Side/Status_Effect_Buff_System.md` | Phase 4e (reconnectBuffCache removal) |
| Global Rules | `docsNew/00_Global_Rules/Global_Rules.md` | All phases (coding standards) |

### Source Files Quick Reference

| File | Role in This Plan |
|------|-------------------|
| `client/.../MMOGameInstance.h/.cpp` | Phase 4b: Add FSocketIONative, EventRouter, connect/disconnect |
| `client/.../UI/*Subsystem.h/.cpp` (14 pairs) | Phase 4c: Replace FindSocketIOComponent pattern |
| `client/.../VFX/SkillVFXSubsystem.h/.cpp` | Phase 4c: Same migration |
| `client/.../UI/LoginFlowSubsystem.h/.cpp` | Phase 4c: Update detection logic |
| `server/src/index.js` | Phase 4e: Remove reconnectBuffCache |
| Plugin: `Plugins/SocketIOClient/.../SocketIONative.h` | API reference for FSocketIONative |
| Plugin: `Plugins/SocketIOClient/.../SocketIOClient.h` | ISocketIOClientModule factory |

### Memory Files

| File | Content |
|------|---------|
| `~/.claude/projects/C--Sabri-MMO/memory/MEMORY.md` | Zone system, UI subsystems, socket patterns |
| `~/.claude/projects/C--Sabri-MMO/memory/status-buff-system.md` | Buff reconnect cache details |

---

## Phase 4a: Create SocketEventRouter (1 day)

### Goal
Create a centralized event dispatch system that replaces the fragile `WrapSingleEvent` chain. Multiple handlers can register for the same event. Handlers are tagged with an owner for lifecycle management.

### Files to Create

**`client/SabriMMO/Source/SabriMMO/SocketEventRouter.h`**
**`client/SabriMMO/Source/SabriMMO/SocketEventRouter.cpp`**

### Design

```cpp
UCLASS()
class SABRIMMO_API USocketEventRouter : public UObject
{
    GENERATED_BODY()
public:
    // Register a handler for an event. Returns HandlerId for targeted removal.
    uint32 RegisterHandler(const FString& EventName, UObject* Owner,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);

    // Unregister all handlers owned by a specific UObject
    // Called in every subsystem's Deinitialize() to prevent dangling references
    void UnregisterAllForOwner(UObject* Owner);

    // Bind this router to a FSocketIONative instance
    // Called once after socket creation in GameInstance::ConnectSocket()
    void BindToNativeClient(TSharedPtr<class FSocketIONative> NativeClient);

    // Unbind all native listeners (called on disconnect/shutdown)
    void UnbindFromNativeClient();

private:
    struct FHandler
    {
        uint32 HandlerId;
        TWeakObjectPtr<UObject> Owner;
        TFunction<void(const TSharedPtr<FJsonValue>&)> Callback;
    };

    struct FEventEntry
    {
        TArray<FHandler> Handlers;
    };

    // EventName → shared entry (TSharedPtr for stable lambda capture)
    TMap<FString, TSharedPtr<FEventEntry>> HandlerMap;

    // Track which events have native listeners bound
    TSet<FString> BoundNativeEvents;

    // The native client we're listening on
    TSharedPtr<class FSocketIONative> CachedNative;

    uint32 NextHandlerId = 1;

    // Internal: bind a single native listener for an event name
    void EnsureNativeBinding(const FString& EventName);

    // Internal: dispatch to all registered handlers
    void DispatchEvent(const FString& EventName, const TSharedPtr<FJsonValue>& Data);
};
```

### Key Implementation Details

1. `RegisterHandler()` adds to `HandlerMap[EventName]->Handlers` and calls `EnsureNativeBinding(EventName)`
2. `EnsureNativeBinding()` checks `BoundNativeEvents`. If not yet bound, registers ONE native listener:
   ```cpp
   TSharedPtr<FEventEntry> StableEntry = HandlerMap[EventName];
   CachedNative->OnEvent(EventName, [this, EventName, StableEntry](const FString& Evt, const TSharedPtr<FJsonValue>& Data) {
       for (auto& H : StableEntry->Handlers)
       {
           if (H.Owner.IsValid()) H.Callback(Data);
       }
   }, TEXT("/"), ESIOThreadOverrideOption::USE_GAME_THREAD);
   ```
3. Lambda captures `TSharedPtr<FEventEntry>` — stable even when TArray grows
4. `UnregisterAllForOwner()` removes matching handlers and cleans up empty entries
5. `UnbindFromNativeClient()` calls `CachedNative->UnbindEvent()` for all bound events

### Verification
- [ ] Create files, compile successfully
- [ ] Unit test: Register 2 handlers for "test:event", call DispatchEvent, verify both fire
- [ ] Unit test: UnregisterAllForOwner removes correct handlers, other handlers survive

### Risk: LOW — Purely additive, no existing code modified.

---

## Phase 4b: Move Socket to GameInstance (1 day)

### Goal
Create the persistent `FSocketIONative` on `UMMOGameInstance`. Connect on login, disconnect on logout. Expose accessors for subsystems.

### Files to Modify

**`client/SabriMMO/Source/SabriMMO/MMOGameInstance.h`** — Add:
```cpp
#include "SocketEventRouter.h"

// Forward declarations
class FSocketIONative;

// In class body:
public:
    // ---- Persistent Socket Connection ----
    void ConnectSocket();
    void DisconnectSocket();
    TSharedPtr<FSocketIONative> GetNativeSocket() const;
    USocketEventRouter* GetEventRouter() const;
    bool IsSocketConnected() const;
    void EmitSocketEvent(const FString& EventName, const TSharedPtr<FJsonObject>& Payload);
    void EmitSocketEvent(const FString& EventName, const FString& StringPayload);

private:
    TSharedPtr<FSocketIONative> NativeSocket;

    UPROPERTY()
    USocketEventRouter* EventRouter = nullptr;

    bool bHasEmittedPlayerJoin = false;
    void EmitPlayerJoin();
```

**`client/SabriMMO/Source/SabriMMO/MMOGameInstance.cpp`** — Add:
```cpp
#include "SocketIONative.h"
#include "SocketIOClient.h"  // ISocketIOClientModule

void UMMOGameInstance::Init()
{
    Super::Init();
    LoadRememberedUsername();
    EventRouter = NewObject<USocketEventRouter>(this);
}

void UMMOGameInstance::ConnectSocket()
{
    if (NativeSocket.IsValid() && NativeSocket->bIsConnected) return;

    NativeSocket = ISocketIOClientModule::Get().NewValidNativePointer();
    NativeSocket->bCallbackOnGameThread = true;
    NativeSocket->bUnbindEventsOnDisconnect = false;  // CRITICAL: keep handlers
    NativeSocket->MaxReconnectionAttempts = 0;  // Infinite reconnection
    NativeSocket->ReconnectionDelay = 2000;     // 2s between attempts

    EventRouter->BindToNativeClient(NativeSocket);

    bHasEmittedPlayerJoin = false;
    NativeSocket->OnConnectedCallback = [this](const FString& SocketId, const FString& SessionId) {
        UE_LOG(LogTemp, Log, TEXT("[Socket] Connected: %s"), *SocketId);
        EmitPlayerJoin();
    };

    NativeSocket->OnDisconnectedCallback = [this](const ESIOConnectionCloseReason Reason) {
        UE_LOG(LogTemp, Warning, TEXT("[Socket] Disconnected (reason: %d)"), (int32)Reason);
        bHasEmittedPlayerJoin = false;
    };

    FString Url = GetServerSocketUrl();
    UE_LOG(LogTemp, Log, TEXT("[Socket] Connecting to: %s"), *Url);
    NativeSocket->Connect(Url);
}

void UMMOGameInstance::EmitPlayerJoin()
{
    if (!NativeSocket.IsValid() || !NativeSocket->bIsConnected) return;

    auto Payload = MakeShared<FJsonObject>();
    FCharacterData Char = GetSelectedCharacter();
    Payload->SetStringField(TEXT("characterId"), FString::FromInt(Char.CharacterId));
    Payload->SetStringField(TEXT("token"), GetAuthHeader());  // "Bearer <jwt>"
    Payload->SetStringField(TEXT("characterName"), Char.Name);

    NativeSocket->Emit(TEXT("player:join"), MakeShared<FJsonValueObject>(Payload));
    bHasEmittedPlayerJoin = true;
    UE_LOG(LogTemp, Log, TEXT("[Socket] Emitted player:join for character: %s"), *Char.Name);
}

void UMMOGameInstance::DisconnectSocket()
{
    if (NativeSocket.IsValid())
    {
        EventRouter->UnbindFromNativeClient();
        NativeSocket->SyncDisconnect();
        ISocketIOClientModule::Get().ReleaseNativePointer(NativeSocket);
        NativeSocket = nullptr;
        bHasEmittedPlayerJoin = false;
    }
}

void UMMOGameInstance::Logout()
{
    DisconnectSocket();
    SaveRememberedUsername();
    ClearAuthData();
    SelectedServer = FServerInfo();
    ServerList.Empty();
}
```

**`client/SabriMMO/Source/SabriMMO/UI/LoginFlowSubsystem.cpp`** — Modify `OnPlayCharacter()`:
```cpp
void ULoginFlowSubsystem::OnPlayCharacter(const FCharacterData& Character)
{
    UMMOGameInstance* GI = GetGI();
    if (!GI) return;

    GI->SelectCharacter(Character.CharacterId);
    GI->SaveRememberedUsername();

    // Set zone/level data
    GI->PendingLevelName = Character.LevelName;
    GI->PendingZoneName = Character.ZoneName;
    GI->CurrentZoneName = Character.ZoneName;
    GI->PendingSpawnLocation = FVector(Character.X, Character.Y, Character.Z);
    GI->bIsZoneTransitioning = true;

    // Connect persistent socket BEFORE opening game level
    GI->ConnectSocket();

    TransitionTo(ELoginFlowState::EnteringWorld);
}
```

### Verification
- [ ] Compile successfully
- [ ] Login → verify socket connects (server log shows player:join)
- [ ] Verify `GI->IsSocketConnected()` returns true from any subsystem
- [ ] Verify EventRouter is accessible from subsystems
- [ ] Note: BP_SocketManager still exists and connects too (two connections temporarily). This is expected and harmless during transition.

### Risk: MEDIUM — Changes connection lifecycle. Mitigated by coexistence with old system.

---

## Phase 4c: Migrate 15 C++ Subsystems (2-3 days)

### Goal
Replace `FindSocketIOComponent()` + `WrapSingleEvent()` in all 15 subsystems with `EventRouter->RegisterHandler()`. Replace `EmitNative()` with `GI->EmitSocketEvent()`.

### Pattern Change (Applied to EVERY Subsystem)

**REMOVE from each subsystem .h:**
```cpp
// DELETE these:
class USocketIOClientComponent;                          // Forward decl
USocketIOClientComponent* FindSocketIOComponent() const; // Method
void TryWrapSocketEvents();                              // Method
void WrapSingleEvent(...);                               // Method
bool bEventsWrapped = false;                             // Member
FTimerHandle BindCheckTimer;                             // Member
TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent; // Member
```

**REMOVE from each subsystem .cpp:**
```cpp
// DELETE these functions entirely:
FindSocketIOComponent() { ... }
WrapSingleEvent() { ... }
TryWrapSocketEvents() { ... }  // Including timer setup in OnWorldBeginPlay
```

**ADD to each subsystem .cpp:**
```cpp
#include "MMOGameInstance.h"  // For GetEventRouter(), EmitSocketEvent()

void UXxxSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
    if (!GI || !GI->GetEventRouter()) return;

    auto* Router = GI->GetEventRouter();

    // Register event handlers (replaces WrapSingleEvent calls)
    Router->RegisterHandler(TEXT("event:name"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEventName(D); });
    // ... repeat for each event

    // Populate initial data from GameInstance
    PopulateFromGameInstance();

    // Request fresh data from server (replaces post-wrap emit)
    if (GI->IsSocketConnected())
    {
        GI->EmitSocketEvent(TEXT("some:request"), TEXT("{}"));
    }

    ShowWidget();
}

void UXxxSubsystem::Deinitialize()
{
    HideWidget();
    // CRITICAL: Unregister to prevent dangling lambdas
    if (UWorld* World = GetWorld())
    {
        if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
        {
            if (auto* Router = GI->GetEventRouter())
                Router->UnregisterAllForOwner(this);
        }
    }
    Super::Deinitialize();
}
```

**Replace EmitNative calls:**
```cpp
// OLD: CachedSIOComponent->EmitNative(TEXT("zone:warp"), Payload);
// NEW: GI->EmitSocketEvent(TEXT("zone:warp"), Payload);

// Helper to get GI:
UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
if (GI) GI->EmitSocketEvent(TEXT("zone:warp"), Payload);
```

### Subsystem Migration Order & Checklist

Migrate one at a time. Compile + test after each.

| # | Subsystem | Events IN | Events OUT | Test Method |
|---|-----------|-----------|------------|-------------|
| 1 | ZoneTransitionSubsystem | zone:change, zone:error, player:teleport | zone:warp, zone:ready | Warp through portal |
| 2 | CombatStatsSubsystem | player:stats | player:request_stats, player:allocate_stat | F8 toggle, allocate stat |
| 3 | BasicInfoSubsystem | 11 events (health, damage, death, respawn, stats, exp, etc.) | player:request_stats, inventory:load | HP/SP/EXP bars visible + updating |
| 4 | BuffBarSubsystem | status:applied/removed, buff:list, skill:buff_applied/removed | buff:request | Apply buff (Provoke), check icons |
| 5 | DamageNumberSubsystem | combat:damage, skill:effect_damage | (none) | Attack enemy, see damage numbers |
| 6 | WorldHealthBarSubsystem | combat:health_update, combat:damage/death/respawn, enemy:health_update, skill:effect_damage | (none) | Floating HP bars visible |
| 7 | CastBarSubsystem | skill:cast_start/complete/interrupted/interrupted_broadcast | (none) | Cast a skill, see cast bar |
| 8 | SkillVFXSubsystem | skill:cast_start/complete, skill:effect_damage, skill:buff_applied/removed, ground_effect:removed | (none) | Cast skill, see VFX |
| 9 | InventorySubsystem | inventory:data/equipped/dropped/error | inventory:use/equip/drop/move/load | F6 toggle, use item |
| 10 | EquipmentSubsystem | inventory:data | (none) | F7 toggle, see equipment |
| 11 | HotbarSubsystem | hotbar:alldata | hotbar:request/save/save_skill/clear | Hotbar visible, click slots |
| 12 | ShopSubsystem | shop:data/bought/sold/error | shop:open/buy_batch/sell_batch | Click shop NPC, buy/sell |
| 13 | KafraSubsystem | kafra:data/saved/teleported/error | kafra:open/save/teleport | Click Kafra NPC |
| 14 | SkillTreeSubsystem | skill:data/learned/learn_error, hotbar:alldata, + many more | skill:data/learn/reset/use, hotbar:save_skill/request | K toggle, cast skills |
| 15 | LoginFlowSubsystem | (none) | (none) | Login flow works without SocketManager detection |

### LoginFlowSubsystem Detection Update (Step 15)

```cpp
// OLD (line 44-51 of LoginFlowSubsystem.cpp):
for (TActorIterator<AActor> It(&InWorld); It; ++It)
{
    if (It->GetName().Contains(TEXT("SocketManager")))
    {
        return;  // Skip login UI in game level
    }
}

// NEW:
UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
if (GI && GI->IsSocketConnected())
{
    UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Socket connected — game level. Skipping login UI."));
    return;
}
```

### Verification (per subsystem)
- [ ] Compile succeeds after each migration
- [ ] Subsystem's widget displays correctly
- [ ] Events fire and update the widget (test each listened event)
- [ ] Emit events work (for subsystems that send)
- [ ] No crashes on zone transition
- [ ] No crashes on Deinitialize (check for dangling references)

### Risk: HIGH — 28 files modified. Mitigated by one-at-a-time migration with testing after each.

---

## Phase 4d: BP Event Handler Bridge (2-3 days)

### Goal
Move BP_SocketManager's 30 event handlers to C++ without rewriting the Blueprint logic. A new `UMultiplayerEventSubsystem` receives events from EventRouter and calls BP_SocketManager's handler functions via `ProcessEvent`.

### Strategy: C++ Bridge → Blueprint Functions

**Why NOT full C++ rewrite:**
- BP_SocketManager has 1,100+ Blueprint nodes across 36 functions
- `OnCombatDamage` alone is 221 nodes (attack animations, actor rotation, target frame, BPI_Damageable)
- Full C++ rewrite would take 1-2 weeks and risk breaking working game logic
- Bridge approach: change event DELIVERY only, preserve ALL Blueprint HANDLING

**How the bridge works:**
```
BEFORE: Server → Socket → USocketIOClientComponent → BP BindEventToFunction → OnCombatDamage(Data)
AFTER:  Server → Socket → FSocketIONative → EventRouter → MultiplayerEventSubsystem → ProcessEvent → OnCombatDamage(Data)
```

### Files to Create

**`client/SabriMMO/Source/SabriMMO/UI/MultiplayerEventSubsystem.h`**
**`client/SabriMMO/Source/SabriMMO/UI/MultiplayerEventSubsystem.cpp`**

```cpp
UCLASS()
class SABRIMMO_API UMultiplayerEventSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // Emit combat events (called from BP_MMOCharacter)
    void EmitAttack(int32 TargetId, bool bIsEnemy);
    void EmitStopAttack();

private:
    void RegisterSocketEvents();
    void FindBlueprintActors();

    // Bridge: calls BP_SocketManager function by name with raw JSON data
    void CallBPHandler(const FString& FunctionName, const FString& JsonData);
    void CallBPHandler(const FString& FunctionName, const TSharedPtr<FJsonValue>& Data);

    // Individual handlers (registered with EventRouter)
    void HandlePlayerMoved(const TSharedPtr<FJsonValue>& Data);
    void HandlePlayerLeft(const TSharedPtr<FJsonValue>& Data);
    void HandlePlayerJoined(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyMove(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
    void HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data);
    void HandleAutoAttackStopped(const TSharedPtr<FJsonValue>& Data);
    void HandleTargetLost(const TSharedPtr<FJsonValue>& Data);
    void HandleOutOfRange(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatError(const TSharedPtr<FJsonValue>& Data);
    void HandleItemUsed(const TSharedPtr<FJsonValue>& Data);
    void HandleItemEquipped(const TSharedPtr<FJsonValue>& Data);
    void HandleLootDrop(const TSharedPtr<FJsonValue>& Data);
    void HandleChatReceive(const TSharedPtr<FJsonValue>& Data);

    // Actor references (found after level loads)
    TWeakObjectPtr<AActor> SocketManagerActor;  // BP_SocketManager (handler shell)
    FTimerHandle FindActorsTimer;
};
```

**ProcessEvent bridge implementation:**
```cpp
void UMultiplayerEventSubsystem::CallBPHandler(const FString& FunctionName, const FString& JsonData)
{
    if (!SocketManagerActor.IsValid()) return;

    UFunction* Func = SocketManagerActor->FindFunction(*FunctionName);
    if (!Func)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MPEvent] Function not found: %s"), *FunctionName);
        return;
    }

    // All BP_SocketManager handler functions take (Data: FString)
    struct { FString Data; } Params;
    Params.Data = JsonData;
    SocketManagerActor->ProcessEvent(Func, &Params);
}

void UMultiplayerEventSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
    // Convert FJsonValue back to JSON string for BP consumption
    FString JsonString;
    auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
    FJsonSerializer::Serialize(Data.ToSharedRef(), TEXT(""), Writer);
    CallBPHandler(TEXT("OnCombatDamage"), JsonString);
}
```

**`client/SabriMMO/Source/SabriMMO/UI/PositionBroadcastSubsystem.h`**
**`client/SabriMMO/Source/SabriMMO/UI/PositionBroadcastSubsystem.cpp`**

```cpp
UCLASS()
class SABRIMMO_API UPositionBroadcastSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    void BroadcastPosition();  // Called every 33ms (~30Hz)
    FTimerHandle PositionTimer;
    int32 LocalCharacterId = 0;
};
```

### BP_SocketManager Modifications (UE5 Editor)

**Remove from BeginPlay Event Graph:**
- All `Bind Event to Function` calls (28+ bindings)
- `Connect` call on SocketIO component
- `Bind Event to On Connected` delegate

**Remove from EndPlay:**
- `Disconnect` call on SocketIO component
- Timer cleanup for `EmitPositionUpdate`

**Remove functions:**
- `EmitPositionUpdate` (moved to PositionBroadcastSubsystem)
- `EmitAttack`, `EmitStartAttack`, `EmitStopAttack` (moved to MultiplayerEventSubsystem)
- `EmitShop` (already in ShopSubsystem)

**KEEP all handler functions:**
- `OnCombatDamage`, `OnPlayerMoved`, `OnEnemySpawn`, etc.
- These are called via ProcessEvent bridge from C++

**KEEP all variable references:**
- `OtherPlayerManagerRef`, `EnemyManagerRef`, `MMOCharaterRef`, `MMOCharacterHudManager`
- These must be set in BeginPlay (after Delay, same as now — just without the socket stuff)

### Verification
- [ ] Remote players appear, move, and disappear correctly
- [ ] Enemies spawn, move, attack, and die correctly
- [ ] Combat damage triggers attack animations on local + remote players
- [ ] Auto-attack state (started/stopped/target_lost) works
- [ ] Death overlay shows and hides
- [ ] Respawn teleports pawn and restores health
- [ ] Loot popups appear
- [ ] Chat messages display
- [ ] Position broadcasts at 30Hz (check server logs)
- [ ] Item use notifications (heal/SP) appear

### Risk: HIGH — Complex 221-node function path. Mitigated by bridge approach (all BP logic unchanged).

---

## Phase 4e: Server + Zone Flow (half day)

### Goal
Clean up server band-aids and verify the persistent socket zone change flow.

### Server Changes (`server/src/index.js`)

| Change | Lines (approx) | What to Do |
|--------|----------------|------------|
| Remove `reconnectBuffCache` Map | ~131-134 | Delete Map declaration and TTL constant |
| Remove buff cache population on disconnect | ~2449-2465 | Delete caching code in disconnect handler |
| Remove buff cache restoration on player:join | ~1807-1823 | Delete restoration code |
| Remove buff cache TTL setTimeout | ~2459-2464 | Delete cleanup timer |
| Verify `zone:warp` works without disconnect | ~1986-2102 | Read-only verification (already correct) |
| Verify `zone:ready` works without player:join | ~2107-2191 | Read-only verification (already correct) |

### Client Changes (`ZoneTransitionSubsystem.cpp`)

Modify `CheckTransitionComplete()`:
```cpp
// OLD: waits for bEventsWrapped (socket reconnect + BP binding)
if (!bEventsWrapped) { return; }

// NEW: socket is persistent, just wait for pawn
// No need to wait for events — they're always registered
```

Remove from `OnWorldBeginPlay`:
```cpp
// OLD: polling timer for socket events
InWorld.GetTimerManager().SetTimer(BindCheckTimer, ..., 0.5f, true);

// NEW: Register with EventRouter directly (from Phase 4c)
```

### Verification
- [ ] Warp to new zone: server logs show NO disconnect event
- [ ] Server logs show NO `player:join` on zone change (only initial login)
- [ ] Other players see `player:left` when you warp out, `player:moved` when you arrive
- [ ] Buffs persist through zone change (cast Provoke → warp → check buff bar)
- [ ] Closing the client DOES trigger server disconnect handler
- [ ] Stats/health saved correctly on actual disconnect

### Risk: MEDIUM — Events during level transition. Mitigated by zone:ready resending all data.

---

## Phase 4f: Cleanup + Docs (half day)

### Code Cleanup

| Item | Action |
|------|--------|
| `BP_SocketManager` Blueprint | Verify all socket code removed (Phase 4d). May delete entirely in future. |
| `reconnectBuffCache` in index.js | Already removed (Phase 4e) |
| `zoneTransitioning` Set in index.js | Remove if unused |
| Old comments referencing BP_SocketManager | Update in all 7 C++ files |
| `#include "SocketIOClientComponent.h"` | Remove from all subsystems that no longer need it |

### Documentation Updates

| Document | Update |
|----------|--------|
| `CLAUDE.md` | Architecture section: GameInstance socket, no BP_SocketManager |
| `docsNew/00_Project_Overview.md` | Update system inventory |
| `docsNew/01_Architecture/System_Architecture.md` | Update socket architecture diagram |
| `docsNew/02_Client_Side/Blueprints/02_BP_SocketManager.md` | Mark as handler-only shell, document bridge |
| `docsNew/02_Client_Side/C++_Code/02_MMOGameInstance.md` | Document new socket members |
| `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` | Remove BP_SocketManager placement step |
| This file | Update progress tracker |
| Memory: `MEMORY.md` | Update socket architecture, subsystem pattern |

### Final Verification (Full Regression)

1. [ ] Login → character select → enter world (socket connects once)
2. [ ] Walk around, verify position broadcasting (30Hz)
3. [ ] Attack enemy, verify combat damage + animations + damage numbers
4. [ ] Cast skill, verify VFX + cast bar + damage numbers
5. [ ] Open inventory (F6), equip item, use consumable
6. [ ] Open equipment (F7)
7. [ ] Open shop (click NPC), buy/sell
8. [ ] Use Kafra NPC
9. [ ] Apply buff (Provoke), check buff bar
10. [ ] **Warp to new zone** — verify NO disconnect, buffs persist, enemies spawn
11. [ ] **Warp back** — verify same behavior
12. [ ] Chat message sends and receives
13. [ ] Close client — verify server disconnect handler fires correctly
14. [ ] Re-login — verify clean state

---

## Risk Assessment

| Phase | Risk | Key Concern | Mitigation |
|-------|------|-------------|------------|
| 4a | LOW | Purely additive | None needed |
| 4b | MEDIUM | Connection lifecycle change | Coexist with old BP_SocketManager temporarily |
| 4c | HIGH | 28 files, multi-handler dispatch | Migrate one subsystem at a time, test after each |
| 4d | HIGH | 221-node OnCombatDamage path | Bridge approach preserves all BP logic unchanged |
| 4e | MEDIUM | Events during level transition | zone:ready resends all data; no actual data loss |
| 4f | LOW | Documentation cleanup | After all verification passes |

### What Could Go Wrong

| Issue | Impact | How to Detect | How to Fix |
|-------|--------|--------------|------------|
| EventRouter lambda captures stale `this` | Crash | Crash on zone transition | Ensure `UnregisterAllForOwner(this)` in every Deinitialize |
| FSocketIONative not thread-safe | Race condition | Intermittent crashes | Already mitigated: `bCallbackOnGameThread = true` |
| BP_SocketManager handler functions not found | Silent failure | Features stop working | Check function names match exactly (unrealMCP) |
| Two socket connections during transition | Server confusion | Duplicate player entries | Remove BP_SocketManager Connect before Phase 4d |
| Events arrive during OpenLevel gap | Missing data | Enemies/players not appearing | zone:ready resends everything |
| `bUnbindEventsOnDisconnect` still true | Events lost on network hiccup | Features stop after network glitch | Set to false in ConnectSocket() |

---

## Dependency Graph

```
Phase 4a (SocketEventRouter) ←── MUST be first
  │
  v
Phase 4b (Socket on GameInstance)
  │
  ├──→ Phase 4c (Migrate 15 C++ subsystems)
  │      │
  │      v (all 15 done)
  │
  ├──→ Phase 4d (BP event handler bridge)
  │      │
  │      v (bridge working)
  │
  v
Phase 4e (Server + zone flow) ←── needs 4c + 4d complete
  │
  v
Phase 4f (Cleanup + docs) ←── last
```

Phase 4c and 4d CAN overlap — 4c migrates C++ subsystems while 4d adds the BP bridge. But 4c should ideally finish first to validate the EventRouter pattern before trusting it for the BP bridge.

---

## Time Estimate

| Phase | Effort | Running Total |
|-------|--------|--------------|
| 4a | 1 day | 1 day |
| 4b | 1 day | 2 days |
| 4c | 2-3 days | 4-5 days |
| 4d | 2-3 days | 6-8 days |
| 4e | 0.5 day | 6.5-8.5 days |
| 4f | 0.5 day | **7-9 days total** |
