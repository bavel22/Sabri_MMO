# AC_HUDManager Refactoring Plan

## Overview

AC_HUDManager is the most function-rich Blueprint in Sabri_MMO (25 functions, 10 variables). It manages ALL HUD widgets as a single ActorComponent attached to BP_MMOCharacter. This document outlines the phased refactoring plan to improve stability, performance, and maintainability.

## Current State Analysis

### Architecture
```
BP_SocketManager (server events)
    ↓ calls MMOCharacterHudManager
AC_HUDManager (25 functions, single component)
    ├── HUD Lifecycle (InitializeHUD, InitializeChatWindow)
    ├── Health Display (UpdateLocalPlayerHealth/Mana, UpdateTargetHealth/Mana, Show/HideTargetFrame)
    ├── Combat UI (ShowDeathOverlay, HideDeathOverlay, SendRespawnRequest, ShowDamageNumbers)
    ├── Inventory (ToggleInventory, PopulateInventory, HandleSlotAction, SendEquip/Use/Drop)
    ├── Stats (ToggleStats, UpdateStats, SendStatIncreaseRequest)
    ├── Chat (ChatReceived, AddChatMessage, SendChatMessageRequest)
    └── Loot (ShowLootPopup)
```

### Identified Issues

| # | Issue | Severity | Category |
|---|-------|----------|----------|
| 1 | **God Component** — 25 functions across 7 UI domains in one component | Medium | Architecture |
| 2 | **No null checks** — Widget refs used without IsValid guards | High | Stability |
| 3 | **No JSON error handling** — Parse failures crash functions | High | Stability |
| 4 | **Duplicate creation** — ShowDeathOverlay can create multiple overlays | Medium | Bug |
| 5 | **Unused Event Tick** — Tick stub exists but is disconnected | Low | Performance |
| 6 | **No widget pooling** — Damage numbers, chat lines, inventory slots created/destroyed repeatedly | Medium | Performance |
| 7 | **Mixed concerns** — Data processing (JSON) mixed with widget management | Low | Architecture |
| 8 | **BP_SocketManager coupling** — Missing IsValid checks before HUDManager calls | High | Stability |
| 9 | **No error recovery** — If a widget fails to create, subsequent operations crash | Medium | Stability |
| 10 | **Large functions** — PopulateInventory (111 nodes), UpdateStats (106 nodes) | Low | Maintainability |

### Anti-Patterns Present

| Anti-Pattern | Where | Impact |
|-------------|-------|--------|
| God Component | AC_HUDManager itself | Hard to maintain, test, extend |
| Missing null guards | All 25 functions | Crash risk |
| No error handling | JSON parsing in 6+ functions | Crash risk |
| Widget spawn/destroy spam | ShowDamageNumbers, AddChatMessage, PopulateInventory | GC pressure |

---

## Phase 1: Stabilization (IMMEDIATE — 1-2 days)

**Goal**: Prevent crashes without changing architecture.

### 1.1 AC_HUDManager Safety Nets

| Task | Functions Affected | Pattern |
|------|-------------------|---------|
| Remove unused Event Tick | Event Graph | Delete node |
| Add SocketManagerRef validation | Event Graph | IsValid after GetActorOfClass |
| GameHudRef null guard | 6 functions (health/target) | IsValid → Return if null |
| SocketManagerRef null guard | 6 functions (send/emit) | IsValid → Return if null |
| ChatWindowRef null guard | 2 functions (chat) | IsValid → Return if null |
| DeathOverlayRef guards | ShowDeathOverlay, HideDeathOverlay | Duplicate guard + null guard |
| InventoryWindowRef guard | PopulateInventory | IsValid → Return if null |
| StatAllocationWindowRef guard | UpdateStats | IsValid → Return if null |

### 1.2 BP_SocketManager Safety Nets

| Task | Functions Affected |
|------|-------------------|
| Add IsValid(MMOCharacterHudManager) | OnCombatDeath, OnCombatDamage, OnAutoAttackStarted, OnAutoAttackStopped, OnTargetLost, OnInventoryData, OnLootDrop, OnChatReceived, OnPlayerStats, OnCombatRespawn |

**Note**: OnHealthUpdate already has bHUDManagerReady buffering — no change needed.

### 1.3 Quick Wins

| Task | Benefit |
|------|---------|
| Remove disconnected Event Tick | Eliminates unnecessary per-frame overhead |
| Add duplicate creation guard to ShowDeathOverlay | Prevents multiple death overlays stacking |

---

## Phase 2: Performance Quick Wins (Pre-Alpha — 1-2 weeks)

**Goal**: Reduce GC pressure and improve combat UI responsiveness.

### 2.1 Widget Pooling for Damage Numbers
```
Current: ShowDamageNumbers → Create Widget → Add to Viewport → (widget self-destructs after animation)
Proposed: Pre-create pool of 10-15 WBP_DamageNumber widgets
         ShowDamageNumbers → Get from pool → Set position/text → Play animation → Return to pool
```
**Impact**: Eliminates widget creation during combat (most frequent spawn)

### 2.2 Chat Message Line Recycling
```
Current: AddChatMessage → Create WBP_ChatMessageLine → Add to ScrollBox → Never removed
Proposed: Cap at 100 messages. When exceeding, remove oldest → recycle widget
```
**Impact**: Prevents memory growth during long play sessions

### 2.3 Inventory Slot Recycling
```
Current: PopulateInventory → Clear all children → Create new slots for each item
Proposed: Maintain pool of WBP_InventorySlot widgets. Show/hide + update data instead of create/destroy
```
**Impact**: Eliminates 111-node function running create+destroy every inventory refresh

---

## Phase 3: Component Decomposition (Post-Alpha — 2-3 weeks)

**Goal**: Split God Component into single-responsibility components.

### 3.1 Proposed Component Architecture
```
BP_MMOCharacter
├── AC_HUDManager (Coordinator — lightweight, 5-8 functions)
│   ├── InitializeHUD()
│   ├── GetHealthManager() → AC_HealthDisplayManager
│   ├── GetInventoryManager() → AC_InventoryUIManager
│   ├── GetStatsManager() → AC_StatsUIManager
│   ├── GetChatManager() → AC_ChatUIManager
│   └── GetCombatUIManager() → AC_CombatUIManager
│
├── AC_HealthDisplayManager (NEW — extracted from AC_HUDManager)
│   ├── UpdateLocalPlayerHealth()
│   ├── UpdateLocalPlayerMana()
│   ├── UpdateTargetHealth()
│   ├── UpdateTargetMana()
│   ├── ShowTargetFrame()
│   └── HideTargetFrame()
│
├── AC_CombatUIManager (NEW — extracted from AC_HUDManager)
│   ├── ShowDeathOverlay()
│   ├── HideDeathOverlay()
│   ├── SendRespawnRequest()
│   └── ShowDamageNumbers() + widget pool
│
├── AC_InventoryUIManager (NEW — extracted from AC_HUDManager)
│   ├── ToggleInventory()
│   ├── PopulateInventory()
│   ├── HandleSlotAction()
│   ├── SendEquipRequest()
│   ├── SendUseItemRequest()
│   └── SendDropRequest()
│
├── AC_StatsUIManager (NEW — extracted from AC_HUDManager)
│   ├── ToggleStats()
│   ├── UpdateStats()
│   └── SendStatIncreaseRequest()
│
├── AC_ChatUIManager (NEW — extracted from AC_HUDManager)
│   ├── InitializeChatWindow()
│   ├── ChatReceived()
│   ├── AddChatMessage()
│   └── SendChatMessageRequest()
│
├── AC_CameraController (existing)
└── AC_TargetingSystem (existing)
```

### 3.2 Migration Strategy
1. Create new component (e.g., AC_HealthDisplayManager)
2. Move variables (GameHudRef) and functions (UpdateLocalPlayerHealth, etc.)
3. Update BP_SocketManager to call new component instead of AC_HUDManager
4. Test thoroughly
5. Repeat for next component
6. Final: AC_HUDManager becomes thin coordinator

### 3.3 BP_SocketManager Update Strategy
```
Current:  Get MMOCharacterHudManager → Call Function
Proposed: Get MMOCharaterRef → Get HealthDisplayManager → Call Function
          Get MMOCharaterRef → Get CombatUIManager → Call Function
          etc.
```

**Alternative**: AC_HUDManager remains the single entry point (facade pattern) and internally delegates to sub-components. This minimizes BP_SocketManager changes.

---

## Phase 4: Event Bus Architecture (Pre-Beta — 2-3 weeks)

**Goal**: Decouple BP_SocketManager from direct HUD component calls.

### 4.1 C++ Event Dispatchers
```cpp
// In MMOGameInstance or dedicated UGameEventBus
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthUpdated, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryUpdated, const FString&, InventoryJSON);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageDealt, int32, Damage, bool, IsCritical, FVector, WorldPos);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDeath, const FString&, KillerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsUpdated, const FString&, StatsJSON);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChatMessage, const FString&, Channel, const FString&, Sender, const FString&, Message);
```

### 4.2 Flow Change
```
Current:  BP_SocketManager → Get HUDManager → Call Function directly
Proposed: BP_SocketManager → Broadcast Event → Any listener handles it
          AC_HealthDisplayManager binds to OnHealthUpdated
          AC_CombatUIManager binds to OnDamageDealt, OnPlayerDeath
          AC_InventoryUIManager binds to OnInventoryUpdated
          etc.
```

### 4.3 Benefits
- BP_SocketManager doesn't need HUDManager reference at all
- New UI components can listen without modifying SocketManager
- Easy to add analytics, logging, or other listeners
- Testable in isolation

---

## Phase 5: Advanced Optimization (Pre-Release)

### 5.1 Async JSON Processing
Move heavy JSON parsing (PopulateInventory 111 nodes, UpdateStats 106 nodes) to C++ for performance.

### 5.2 C++ Data Models
```cpp
USTRUCT(BlueprintType)
struct FInventorySlotData {
    UPROPERTY(BlueprintReadWrite) int32 InventoryId;
    UPROPERTY(BlueprintReadWrite) FString ItemName;
    UPROPERTY(BlueprintReadWrite) FString ItemType;
    UPROPERTY(BlueprintReadWrite) int32 Quantity;
    UPROPERTY(BlueprintReadWrite) bool bIsEquipped;
    UPROPERTY(BlueprintReadWrite) int32 ATK;
    UPROPERTY(BlueprintReadWrite) int32 DEF;
    // etc.
};
```

### 5.3 UI Update Batching
Batch rapid health updates during combat instead of updating every event.

---

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| Component-Based | AC_HUDManager is a component on BP_MMOCharacter |
| Manager | Centralizes all UI management |
| Event-Driven | Responds to Socket.io events |
| Widget Composition | Uses child widgets (WBP_InventorySlot, WBP_ChatMessageLine) |
| Facade (Phase 3) | AC_HUDManager delegates to sub-components |
| Observer (Phase 4) | Event bus decouples producers from consumers |
| Object Pooling (Phase 2) | Widget recycling for damage numbers, chat, inventory |

## Anti-Patterns to Address

| Anti-Pattern | Phase | Resolution |
|-------------|-------|------------|
| God Component | Phase 3 | Split into 5 focused components |
| Missing null guards | Phase 1 | Add IsValid checks |
| Widget spawn spam | Phase 2 | Object pooling |
| Direct coupling | Phase 4 | Event bus |
| Blueprint-heavy JSON parsing | Phase 5 | Move to C++ |

---

## Priority Matrix

| Phase | Effort | Risk | Impact | When |
|-------|--------|------|--------|------|
| Phase 1: Stabilization | Low (1-2 days) | Very Low | High (prevents crashes) | **NOW** |
| Phase 2: Performance | Medium (1-2 weeks) | Low | Medium (smoother combat) | Pre-Alpha |
| Phase 3: Decomposition | High (2-3 weeks) | Medium | High (maintainability) | Post-Alpha |
| Phase 4: Event Bus | High (2-3 weeks) | Medium | Medium (extensibility) | Pre-Beta |
| Phase 5: Advanced | Medium (1-2 weeks) | Low | Medium (performance) | Pre-Release |

---

## Related Files

| File | Purpose |
|------|---------|
| `Content/SabriMMO/Components/AC_HUDManager.uasset` | The component being refactored |
| `Content/SabriMMO/Blueprints/BP_SocketManager.uasset` | Primary caller of HUDManager functions |
| `Content/SabriMMO/Blueprints/BP_MMOCharacter.uasset` | Owner of HUDManager component |
| `docsNew/02_Client_Side/Blueprints/06_Actor_Components.md` | Current AC_HUDManager documentation |
| `docsNew/02_Client_Side/Blueprints/02_BP_SocketManager.md` | SocketManager documentation |
| `docsNew/02_Client_Side/Blueprints/07_Widget_Blueprints.md` | Widget documentation |

---

**Last Updated**: 2026-02-17
**Version**: 1.0
**Status**: Phase 1 In Progress
