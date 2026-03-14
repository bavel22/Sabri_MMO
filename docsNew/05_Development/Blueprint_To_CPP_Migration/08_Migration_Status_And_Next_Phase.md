# Blueprint-to-C++ Migration — Full Status & Next Phase Plan

**Last Updated:** 2026-03-14
**Status:** Phases 1-4 COMPLETE. Phase 5 (Name Tags + Interaction) is next.

---

## Table of Contents

1. [Migration Overview](#1-migration-overview)
2. [Completed Phase Summary](#2-completed-phase-summary)
3. [Current Architecture (Post Phase 4)](#3-current-architecture-post-phase-4)
4. [Remaining 14 BP Bridges](#4-remaining-14-bp-bridges)
5. [Phase 5: Name Tags + Interaction — Detailed Plan](#5-phase-5-name-tags--interaction--detailed-plan)
6. [Phase 6: Cleanup — Overview](#6-phase-6-cleanup--overview)
7. [Hard-Won Lessons](#7-hard-won-lessons)
8. [Verification Checklists](#8-verification-checklists)

---

## 1. Migration Overview

### Goal
Replace Blueprint-based gameplay systems with pure C++ UWorldSubsystems and Slate UI. Blueprints remain only for level setup, animation graphs, and materials.

### Progress

| Phase | Scope | Status | Date | Key Files |
|-------|-------|--------|------|-----------|
| **Phase 4** | Persistent socket, EventRouter | **COMPLETE** | 2026-03-14 | MMOGameInstance, SocketEventRouter, MultiplayerEventSubsystem, PositionBroadcastSubsystem |
| **Phase 1** | Camera, input, movement | **COMPLETE** | 2026-03-14 | CameraSubsystem, PlayerInputSubsystem |
| **Phase 2** | Targeting, combat actions | **COMPLETE** | 2026-03-13 | TargetingSubsystem, CombatActionSubsystem |
| **Phase 3** | Entity management | **COMPLETE** | 2026-03-14 | EnemySubsystem, OtherPlayerSubsystem |
| **Phase 5** | Name tags (RO Classic) | **COMPLETE** | 2026-03-14 | NameTagSubsystem, SNameTagOverlay |
| **Phase 6** | Dead BP code removal | **PLANNED** | — | Remove BP managers, dead event graph nodes |

### Metrics

| Metric | Before Migration | After Phase 4 |
|--------|-----------------|---------------|
| BP_SocketManager bridges | 31 events | 14 events |
| C++ subsystems | 15 | 24 |
| BP event graph nodes replaced | 0 | ~700+ (OnCombatDamage 215, BP managers ~300, input ~200) |
| Entity management | Blueprint (BP_EnemyManager, BP_OtherPlayerManager) | C++ (UEnemySubsystem, UOtherPlayerSubsystem) |

---

## 2. Completed Phase Summary

### Phase 4: Persistent Socket (COMPLETE)

**Problem:** Socket.io connection lived on BP_SocketManager's SocketIOClientComponent, which was destroyed on level transitions. Required disconnect/reconnect on every zone change.

**Solution:**
- Moved socket to `UMMOGameInstance` (survives OpenLevel)
- Created `USocketEventRouter` for multi-handler dispatch (multiple subsystems per event)
- `MultiplayerEventSubsystem` bridges remaining events to BP_SocketManager via ProcessEvent
- `PositionBroadcastSubsystem` handles 30Hz position updates
- BP_SocketManager's SocketIOClientComponent disconnected (handler shell only)

**Pattern established:** All subsystems use `Router->RegisterHandler()` in `OnWorldBeginPlay`, `Router->UnregisterAllForOwner(this)` in `Deinitialize`.

### Phase 1: Camera + Input + Movement (COMPLETE)

**Problem:** Camera rotation, scroll zoom, click-to-move, and WASD were in Blueprint event graphs (BP_MMOCharacter, AC_CameraController).

**Solution:**
- `UCameraSubsystem` — right-click yaw rotation, scroll zoom (200-1500), fixed -55° pitch
- `UPlayerInputSubsystem` — click-to-move (NavMesh), click-to-attack (emit only), click-to-interact, walk-to-NPC/enemy

**Key lessons:** Input MUST be bound via `ASabriMMOCharacter::SetupPlayerInputComponent` (NOT from subsystem). Camera finds BP "SpringArm" by name (NOT C++ `GetCameraBoom()`). `SimpleMoveToLocation` destinations must be projected onto NavMesh.

### Phase 2: Targeting + Combat Actions (COMPLETE)

**Problem:** Hover detection (AC_TargetingSystem ~180 nodes), combat response handling (OnCombatDamage 215 nodes) were in Blueprint.

**Solution:**
- `UTargetingSubsystem` (Z=n/a) — 30Hz hover trace, cursor switching (Enemy=Crosshairs, NPC=TextEditBeam), hover indicator WidgetComponents, pauses during skill targeting
- `UCombatActionSubsystem` (Z=9 target frame, Z=40 death overlay) — 10 combat event handlers, bOrientRotationToMovement toggling, attack animation via ProcessEvent reflection, RestoreOrientToMovement() public API

**Key patterns:** Property reflection for BP actor communication (SetBPInt/Double/Bool/String/Vector). PlayAttackAnimation via FindFunction + ProcessEvent. bReadyToProcess one-frame defer prevents ProcessEvent during PostLoad.

### Phase 3: Entity Management (COMPLETE)

**Problem:** BP_EnemyManager and BP_OtherPlayerManager managed entity lifecycle. CombatActionSubsystem resolved actors via ProcessEvent on these BP managers (temporary Phase 2 bridge).

**Solution:**
- `UEnemySubsystem` — TMap registry, 5 event handlers (spawn/move/death/health/attack), spawns BP_EnemyCharacter via LoadClass + SpawnActor, calls InitializeEnemy/UpdateEnemyHealth/OnEnemyDeath/PlayAttackAnimation via ProcessEvent
- `UOtherPlayerSubsystem` — TMap registry, 2 event handlers (moved/left), spawns BP_OtherPlayerCharacter, filters local player, 200-unit snap threshold for teleports
- `CombatActionSubsystem` — FindEnemyActor/FindPlayerActor now use direct subsystem TMap lookups
- `MultiplayerEventSubsystem` — 7 bridges removed (22→14)

**Post-implementation fixes:**
- Death blocks movement (IsDead() in PlayerInputSubsystem + SabriMMOCharacter::DoMove)
- Ranged attacks emit immediately + use server-provided AttackRange
- Remote player snap on teleport/zone transition (200-unit threshold)
- **Critical crash fix:** CallBPFunction uses Memzero + explicit FString placement new (NOT InitializeStruct/DestroyStruct — caused 0xc0000409 stack buffer overrun)

---

## 3. Current Architecture (Post Phase 4)

### 24 C++ Subsystems

| Subsystem | Z-Order | Events | Role |
|-----------|---------|--------|------|
| LoginFlowSubsystem | 5, loading=50 | — | Login state machine |
| WorldHealthBarSubsystem | 8 | 2 | Floating HP/SP bars (OnPaint) |
| CombatActionSubsystem | 9, death=40 | 10 | Combat responses, target frame, death overlay |
| BasicInfoSubsystem | 10 | 2 | HP/SP/EXP bars |
| BuffBarSubsystem | 11 | 3 | Status/buff icons with countdown |
| CombatStatsSubsystem | 12 | 1 | F8 stats panel |
| InventorySubsystem | 14 | 6 | F6 inventory, drag state |
| EquipmentSubsystem | 15 | 2 | F7 equipment |
| HotbarSubsystem | 16, keybind=30 | 3 | F5 cycle, 4×9 slots |
| ShopSubsystem | 18 | 4 | NPC shop buy/sell |
| KafraSubsystem | 19 | — | Kafra NPC dialog |
| SkillTreeSubsystem | 20 | 4 | Skill tree UI + targeting mode |
| DamageNumberSubsystem | 20 | 2 | Floating damage numbers |
| ItemInspectSubsystem | 22 | — | Right-click item inspect |
| CastBarSubsystem | 25 | 2 | Cast bar overlay |
| ZoneTransitionSubsystem | 50 | 3 | Zone transitions, loading overlay |
| SkillVFXSubsystem | n/a | 2 | Niagara VFX for skills |
| MultiplayerEventSubsystem | n/a | 14 bridges | BP_SocketManager event bridge |
| PositionBroadcastSubsystem | n/a | — | 30Hz position broadcast |
| EnemySubsystem | n/a | 5 | Enemy entity registry + lifecycle |
| OtherPlayerSubsystem | n/a | 2 | Other player entity registry + lifecycle |
| TargetingSubsystem | n/a | — | 30Hz hover trace, cursor switching |
| PlayerInputSubsystem | n/a | — | Click-to-move/attack/interact |
| CameraSubsystem | n/a | — | Right-click rotation, scroll zoom |

### Event Flow

```
Server (Socket.io)
    ↓
GameInstance Persistent Socket (FSocketIONative)
    ↓
USocketEventRouter (multi-handler dispatch)
    ↓ dispatches to ALL registered handlers
    ├── CombatActionSubsystem (10 combat events)
    ├── EnemySubsystem (5 enemy events)
    ├── OtherPlayerSubsystem (2 player events)
    ├── InventorySubsystem (5 inventory events + card:result)
    ├── BasicInfoSubsystem (player:joined, combat:health_update)
    ├── CombatStatsSubsystem (player:stats)
    ├── DamageNumberSubsystem (combat:damage, skill:effect_damage)
    ├── BuffBarSubsystem (buff:list, buff:update, status:update)
    ├── HotbarSubsystem (hotbar:data, hotbar:alldata, hotbar:request)
    ├── SkillTreeSubsystem (skill:data, skill:learn_result, skill:effect_damage, skill:cast_start)
    ├── CastBarSubsystem (skill:cast_start, skill:cast_complete)
    ├── SkillVFXSubsystem (skill:effect_damage, skill:cast_start)
    ├── ZoneTransitionSubsystem (zone:change, zone:error, zone:teleport)
    ├── WorldHealthBarSubsystem (combat:damage, combat:health_update)
    └── MultiplayerEventSubsystem (14 events → BP_SocketManager via ProcessEvent)
```

---

## 4. Remaining 14 BP Bridges

These events are still forwarded from `MultiplayerEventSubsystem` to `BP_SocketManager` via ProcessEvent:

| # | Socket Event | BP Function | Category | Notes |
|---|-------------|-------------|----------|-------|
| 1 | `inventory:data` | `OnInventoryData` | Inventory | Full inventory refresh |
| 2 | `inventory:used` | `OnItemUsed` | Inventory | Item use feedback |
| 3 | `inventory:equipped` | `OnItemEquipped` | Inventory | Equipment visual update (65 nodes!) |
| 4 | `inventory:dropped` | `OnItemDropped` | Inventory | Item drop feedback |
| 5 | `inventory:error` | `OnInventoryError` | Inventory | Error display |
| 6 | `loot:drop` | `OnLootDrop` | Loot | Loot pickup notification |
| 7 | `chat:receive` | `OnChatReceived` | Chat | Chat message display |
| 8 | `player:stats` | `OnPlayerStats` | Stats | Stats HUD update |
| 9 | `hotbar:data` | `OnHotbarData` | Hotbar | Single slot update |
| 10 | `hotbar:alldata` | `OnHotbarAllData` | Hotbar | Full hotbar refresh |
| 11 | `shop:data` | `OnShopData` | Shop | Shop inventory display |
| 12 | `shop:bought` | `OnShopBought` | Shop | Purchase result |
| 13 | `shop:sold` | `OnShopSold` | Shop | Sell result |
| 14 | `shop:error` | `OnShopError` | Shop | Shop error display |

**Note:** Many of these are already partially handled by C++ subsystems (InventorySubsystem, ShopSubsystem, etc.) which register directly with the EventRouter. The BP bridges exist for legacy BP_SocketManager functions that update equipment visuals, loot VFX, chat UI, etc. These will be removed when the C++ subsystems fully replace the BP handler logic.

---

## 5. Phase 5: Name Tags + Interaction — Detailed Plan

**Full spec:** `04_Name_Tags_Interaction_Plan.md`
**RO Classic research:** Deep research from roBrowser source, rAthena, iRO Wiki (see Section 5.3 sources)
**Dependencies:** Phases 1-4 (all complete)
**Estimated scope:** ~500 lines new code, ~80 lines modifications

### 5.1 What Gets Built

| File | Purpose |
|------|---------|
| `UI/NameTagSubsystem.h` | UWorldSubsystem — entity registration API, TArray of FNameTagEntry, hover-to-show for monsters/NPCs |
| `UI/NameTagSubsystem.cpp` | Lifecycle, registration, color management, level-based monster color calculation |
| `UI/SNameTagOverlay.cpp` (inline in .cpp) | SCompoundWidget with OnPaint — renders ALL visible name tags in one draw call with 4-pass black outline |

### 5.2 Architecture

Replace N per-actor WidgetComponents (`WBP_PlayerNameTag`) with a single Slate OnPaint overlay that renders all name tags in one pass. Same pattern as `WorldHealthBarSubsystem` (Z=8).

**CRITICAL RO Classic behavior:** Monster and NPC names are **HOVER-ONLY** (shown when mouse cursor is over the entity). Player names are **ALWAYS visible**. This is a fundamental difference from the current WBP_PlayerNameTag which shows all names always.

```
Registration:
  EnemySubsystem::HandleEnemySpawn → NameTagSubsystem::RegisterEntity(actor, "Poring", level=5, MONSTER)
  OtherPlayerSubsystem::HandlePlayerMoved → NameTagSubsystem::RegisterEntity(actor, "player5", PLAYER)
  Local pawn BeginPlay → NameTagSubsystem::RegisterEntity(pawn, "player4", LOCAL_PLAYER)
  NPC actors BeginPlay → NameTagSubsystem::RegisterEntity(actor, "Kafra", NPC)

Rendering (every frame in OnPaint):
  For each registered entity:
    1. Skip if actor invalid (TWeakObjectPtr)
    2. Skip if not visible (dead monster, hidden/cloaked)
    3. For MONSTER/NPC type: skip if NOT hovered (check TargetingSubsystem hover actor)
    4. For PLAYER type: always render
    5. Project actor world position → screen position
    6. Cull if off-screen
    7. Draw name text with 4-pass black outline (cardinal offset 1px)
    8. Draw sub-text below name (guild name, party name, NPC title)
```

### 5.3 RO Classic Name Tag Reference (Pre-Renewal)

**Sources:** roBrowser source (EntityDisplay.js, EntityLife.js), rAthena GitHub, iRO Wiki Classic

#### Visibility Rules

| Entity Type | Name Visibility | Trigger |
|-------------|----------------|---------|
| **Player (self)** | Always visible | Automatic |
| **Other players** | Always visible | Automatic |
| **Monsters** | **Hover only** | Mouse cursor over monster |
| **NPCs** | **Hover only** | Mouse cursor over NPC |
| **Dead monsters** | Hidden (sprite fades) | On death |
| **Dead players** | Visible (lying sprite) | Until respawn |
| **Hidden/Cloaked** | Invisible (alpha=0) | Hiding, Cloaking skills |
| **Vending shops** | Always visible (sign) | Vendor active |

#### Name Colors (exact values from roBrowser/rAthena)

| Entity Type | Color | Hex | Notes |
|-------------|-------|-----|-------|
| Player (default) | White | `#FFFFFF` | Self and other players |
| Party member | Blue | `#5599FF` | "nickname of adventurers in same party display in blue" |
| GM | Yellow | `#FFFF00` | Via `<admin>` tag in clientinfo.xml |
| Monster (base) | Light pink | `#FFC6C6` | roBrowser default monster name color |
| Monster (low level) | Grey | `#C0C0C0` | Monster significantly below player level |
| Monster (same level) | White | `#FFFFFF` | Monster roughly equal to player level |
| Monster (high level) | Red | `#FF0000` | Monster significantly above player level |
| NPC | Light blue | `#94BDF7` | Distinguishes from players and monsters |

#### Text Rendering (exact from roBrowser EntityDisplay.js)

| Property | Value |
|----------|-------|
| Font | Gulim (official RO), Arial 12px (fallback) |
| Shadow/Outline | **4-pass black outline**: draw text 4× at +1/-1 px cardinal offsets in `#000000`, then draw colored text on top |
| Position | **Below character** by default (~15px below entity anchor). `/font` command toggles above/below |
| Alignment | Horizontally centered on entity |
| Billboarding | 3D-to-2D matrix projection (always faces camera) |
| Background | **None** — floating text with outline only |

#### Player Name Format (2 lines)

```
Line 1: CharacterName (PartyName)     ← party name in parentheses if in party
Line 2: GuildName [GuildTitle]         ← guild rank/title in brackets, 24x24 emblem to left
```

#### Monster HP Bar (optional, toggled via /monsterhp)

| Property | Value |
|----------|-------|
| Normal HP color | Pink `#FF00E7` |
| Low HP color (<25%) | Yellow `#FFFF00` |
| Border | Dark blue `#10189C` |
| Background (empty) | Dark grey `#424242` |
| Dimensions | ~60px wide × 5px tall |
| Position | Above monster sprite, centered |

### 5.4 Registration Points

| System | When | Call |
|--------|------|------|
| `EnemySubsystem::HandleEnemySpawn` | After SpawnActor + InitializeEnemy | `RegisterEntity(actor, name, level, EEntityType::Monster)` |
| `EnemySubsystem::HandleEnemyDeath` | On death | `SetVisible(actor, false)` |
| `EnemySubsystem::HandleEnemySpawn` (respawn) | On dead respawn | `SetVisible(actor, true)` |
| `OtherPlayerSubsystem::HandlePlayerMoved` | On first spawn | `RegisterEntity(actor, playerName, EEntityType::Player)` |
| `OtherPlayerSubsystem::HandlePlayerLeft` | Before Destroy | `UnregisterEntity(actor)` |
| Local pawn | After spawn in zone | `RegisterEntity(pawn, charName, EEntityType::LocalPlayer)` |
| NPC actors | BeginPlay (C++ NPC classes) | `RegisterEntity(self, npcName, EEntityType::NPC)` |

### 5.5 Hover Integration with TargetingSubsystem

`TargetingSubsystem` already does 30Hz hover traces and tracks the currently hovered actor. `NameTagSubsystem` reads the hovered actor during OnPaint:

```cpp
// In SNameTagOverlay::OnPaint:
UTargetingSubsystem* TargetSub = World->GetSubsystem<UTargetingSubsystem>();
AActor* HoveredActor = TargetSub ? TargetSub->GetHoveredActor() : nullptr;

for (const FNameTagEntry& Entry : Entries)
{
    // Players: always render
    // Monsters/NPCs: only render if Entry.Actor == HoveredActor
    if (Entry.Type == EEntityType::Monster || Entry.Type == EEntityType::NPC)
    {
        if (Entry.Actor.Get() != HoveredActor) continue;
    }
    // ... render name tag
}
```

**Note:** `TargetingSubsystem` may need a public `GetHoveredActor()` method added (currently hover state is internal).

### 5.6 Level-Based Monster Color Calculation

```cpp
FLinearColor GetMonsterNameColor(int32 PlayerLevel, int32 MonsterLevel)
{
    int32 Diff = MonsterLevel - PlayerLevel;
    if (Diff <= -10) return FLinearColor(0.75f, 0.75f, 0.75f); // Grey — much lower
    if (Diff >= 10)  return FLinearColor(1.0f, 0.0f, 0.0f);    // Red — much higher
    return FLinearColor(1.0f, 1.0f, 1.0f);                      // White — similar level
}
```

### 5.7 Files Modified

| File | Change |
|------|--------|
| `EnemySubsystem.cpp` | Add NameTagSubsystem registration in HandleEnemySpawn, visibility toggle in HandleEnemyDeath |
| `OtherPlayerSubsystem.cpp` | Add NameTagSubsystem registration in HandlePlayerMoved, unregister in HandlePlayerLeft |
| `TargetingSubsystem.h` | Add `AActor* GetHoveredActor() const` public method |

### 5.8 Blueprint Cleanup (after Phase 5 verified)

- Remove WidgetComponent `NameTagWidget` from BP_MMOCharacter, BP_OtherPlayerCharacter, BP_EnemyCharacter
- Delete `WBP_PlayerNameTag` UMG widget
- Remove name tag setup nodes from BeginPlay event graphs

### 5.9 Testing Checklist

1. **Player names always visible** — local + other players show name below character
2. **Monster names hover-only** — name appears only when mouse hovers over monster
3. **NPC names hover-only** — name appears only when mouse hovers over NPC
4. **Monster name color** — grey for low-level, white for same, red for high-level
5. **Player name format** — "Name (PartyName)" line 1, "GuildName [Title]" line 2
6. **4-pass black outline** — readable against any background
7. **Party member blue** — party members show `#5599FF` blue name
8. **NPC light blue** — NPCs show `#94BDF7` name
9. **Death behavior** — monster names disappear on death, reappear on respawn
10. **Hiding/Cloaking** — hidden entities have no name tag
11. **Screen-space projection** — names billboard toward camera
12. **Performance** — no FPS drop with 50+ entities visible
13. **Zone transitions** — name tags clean up and rebuild correctly

---

## 6. Phase 6: Cleanup — Overview

Remove dead Blueprint code that has been fully replaced by C++ subsystems:

| Remove | Replaced By | Condition |
|--------|-------------|-----------|
| AC_TargetingSystem (component) | UTargetingSubsystem | Phase 2 complete |
| AC_CameraController (component) | UCameraSubsystem | Phase 1 complete |
| BP_OtherPlayerManager (actor) | UOtherPlayerSubsystem | Phase 3 complete |
| BP_EnemyManager (actor) | UEnemySubsystem | Phase 3 complete |
| WBP_PlayerNameTag (widget) | UNameTagSubsystem | Phase 5 complete |
| Movement nodes in BP_MMOCharacter | UPlayerInputSubsystem | Phase 1 complete |
| Combat nodes in BP_MMOCharacter | UCombatActionSubsystem | Phase 2 complete |
| OnCombatDamage in BP_SocketManager (215 nodes) | UCombatActionSubsystem | Phase 2 complete |
| OnPlayerMoved/Left in BP_SocketManager | UOtherPlayerSubsystem | Phase 3 complete |
| OnEnemySpawn/Move/Death/Health in BP_SocketManager | UEnemySubsystem | Phase 3 complete |

**Important:** Remove dead code from levels (delete BP_OtherPlayerManager and BP_EnemyManager actor instances from ALL level maps).

---

## 7. Hard-Won Lessons

### Build System
- **ALWAYS build `SabriMMOEditor` target** (NOT `SabriMMO`). Editor loads `UnrealEditor-SabriMMO.dll`, not `SabriMMO.exe`. Building the wrong target causes all changes to silently not load.
- **Header changes → CLOSE EDITOR, rebuild, reopen.** Live Coding crashes on header changes (0xc0000409 during re-instancing).
- **Clean Live Coding patches** after a crash: `rm -f Binaries/Win64/UnrealEditor-SabriMMO.patch_*`

### ProcessEvent Stack Safety
- **NEVER use `InitializeStruct`/`DestroyStruct` on `FMemory_Alloca` buffers.** `InitializeStruct` uses `GetStructureSize()` which includes alignment padding beyond `ParmsSize`. This overflows the stack buffer and corrupts the `/GS` security cookie, causing `0xc0000409` — a silent process termination with NO crash dump.
- **Safe pattern:** `FMemory::Memzero` + explicit `new (&offset) FString()` placement new, then `param.~FString()` after ProcessEvent.
- **Diagnose 0xc0000409:** Windows Event Viewer → Application → Event ID 1000 (NOT UE crash dumps — none are generated).

### Input System
- Input MUST be bound via `ASabriMMOCharacter::SetupPlayerInputComponent` (subsystem binding doesn't work)
- Camera must find BP "SpringArm" by name (not C++ `GetCameraBoom()` — wrong component)
- `SimpleMoveToLocation` destinations must be projected onto NavMesh via `ProjectPointToNavigation`
- Zoom uses `MouseScrollUp` + `MouseScrollDown` with Negate modifier (NOT `MouseWheelAxis`)

### Entity Management
- BP reflection helpers: `SetBPInt/Double/Bool/String/Vector`, `GetBPBool` — anonymous namespace in each .cpp
- `CallBPFunction` with params: Memzero + FString placement new (see EnemySubsystem.cpp)
- `bReadyToProcess` one-frame defer via `SetTimerForNextTick` prevents ProcessEvent during PostLoad
- TWeakObjectPtr for actor registries — auto-nulls on destroy
- 200-unit snap threshold for teleports (Fly Wing, zone transitions)
- Always emit `combat:attack` immediately on click — server handles range

### Multiplayer Safety
- Never use `GEngine->GameViewport` — use `World->GetGameViewport()` (multiplayer-safe)
- `StopPawnMovement`: use `PC->StopMovement()` + `CMC->StopMovementImmediately()` (NOT `SimpleMoveToLocation(self)`)
- Disable `bOrientRotationToMovement` BEFORE `StopPawnMovement` to prevent rotation snap

---

## 8. Verification Checklists

### Phase Completion Verification

For each phase, verify:
- [ ] All new files exist with correct content
- [ ] All modified files have correct changes (no stale code)
- [ ] Dead code removed (no leftover BP manager references)
- [ ] CLAUDE.md updated with new subsystem entries
- [ ] Memory file created/updated
- [ ] Audit doc updated (07_Audit_Report_And_Fixes.md)
- [ ] Build succeeds with `SabriMMOEditor` target
- [ ] Editor opens without crash
- [ ] PIE: subsystem log messages appear (OnWorldBeginPlay)
- [ ] PIE: basic functionality works (enemies spawn, players appear, combat works)
- [ ] PIE: zone transition works (subsystems reinitialize correctly)
