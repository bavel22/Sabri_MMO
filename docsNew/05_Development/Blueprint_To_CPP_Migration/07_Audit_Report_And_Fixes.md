# Blueprint-to-C++ Migration — Audit Report & Fixes

**Date:** 2026-03-13
**Audited By:** Cross-referencing all 7 plan documents against actual codebase source code
**Result:** 24 issues found (6 CRITICAL, 4 HIGH, 8 MEDIUM, 6 LOW). All fixes documented below.
**Post-Audit Fix:** Component 19 gap (target frame widget) resolved — added STargetFrameWidget spec to Plan 02 Section 6.

---

## CRITICAL Issues (Must Fix Before Implementation)

### C1. Auto-Attack State Ownership Conflict (Plans 01 + 02)

**Problem:** Both `UPlayerInputSubsystem` (Plan 01) and `UCombatActionSubsystem` (Plan 02) register for the same 5 combat events (`combat:auto_attack_started`, `combat:auto_attack_stopped`, `combat:target_lost`, `combat:out_of_range`, `combat:error`). Both store `bIsAutoAttacking` and `AttackRange`. Both call `SimpleMoveToLocation` on `combat:out_of_range`. This creates triple-handling (Plan 01 + Plan 02 + existing BP bridge).

**Fix:** Single owner for auto-attack state. Architecture split:
- `UPlayerInputSubsystem`: Handles INPUT only. Detects clicks, emits `combat:attack` / `combat:stop_attack`. Does NOT listen for server combat response events.
- `UCombatActionSubsystem`: Handles ALL server combat responses. Owns `bIsAutoAttacking`, `AttackRange`, auto-attack state machine. Listens for `auto_attack_started/stopped`, `target_lost`, `out_of_range`, `damage`, `death`, `respawn`, `error`.
- `UTargetingSubsystem`: Handles hover detection, cursor changes, target indicator visuals. Reads attack range from `UCombatActionSubsystem`. Does NOT own auto-attack state.

Plan 01 Section 10 (socket events) must be removed entirely. Plan 01 only emits events, never listens. Plan 02 is the sole listener for all combat server responses.

---

### C2. Enhanced Input Priority Does NOT Block Lower-Priority Mappings

**Problem:** UE5 Enhanced Input evaluates higher-priority IMCs first but does NOT automatically prevent lower-priority IMCs from also firing. Both C++ and BP handlers fire on the same left-click, causing double `combat:attack` emissions and double `SimpleMoveToLocation` calls.

**Fix:** Set `bConsumeInput = true` on every key mapping in the C++ IMC:
```cpp
FEnhancedActionKeyMapping& Mapping = GameplayIMC->MapKey(LeftClickAction, EKeys::LeftMouseButton);
Mapping.bConsumeInput = true;
```
Apply to ALL actions: left click, right click, mouse move, mouse wheel, WASD. This prevents the BP's IMC_MMOCharacter from also firing. Additionally, in Phase 1 testing, immediately remove the conflicting IA_ClickToMove and IA_Attack from IMC_MMOCharacter in the editor (don't defer to Phase 6).

---

### C3. Double Animation/Death During Transition Period

**Problem:** When `UCombatActionSubsystem` is implemented (Phase 3) but the BP bridge is still active, `combat:damage` fires BOTH the new C++ handler AND the 215-node BP `OnCombatDamage`. Attack animations play twice, death overlay appears twice.

**Fix:** Remove each BP bridge line from `MultiplayerEventSubsystem` IMMEDIATELY when its C++ replacement is implemented — not batched at the end. Specifically:
- When `UCombatActionSubsystem::HandleCombatDamage` is implemented → remove `combat:damage` bridge
- When `HandleAutoAttackStarted/Stopped` implemented → remove those bridges
- When `HandleCombatDeath/Respawn` implemented → remove those bridges
- When `UEnemySubsystem::HandleEnemySpawn` implemented → remove `enemy:spawn` bridge
- etc.

Each bridge removal happens in the SAME commit as its C++ replacement.

---

### C4. Phase 3↔4 Circular Dependency

**Problem:** `UCombatActionSubsystem` (Phase 3) needs to reference `UEnemySubsystem::GetEnemy()` for death handling. But `UEnemySubsystem` (Phase 4) says Phase 3 must be complete first.

**Fix:** Merge Phases 3 and 4 into a single implementation phase: "Phase 3: Combat + Entities." Build order within the merged phase:
1. Create `AMMORemotePlayer` and `AMMOEnemyActor` (actor classes, no subsystems yet)
2. Create `UOtherPlayerSubsystem` and `UEnemySubsystem` (entity management)
3. Create `UCombatActionSubsystem` (references entity subsystems)
4. Remove entity + combat bridges from `MultiplayerEventSubsystem`

This eliminates the circular dependency. Revised phase list:
- Phase 1: Camera + Movement
- Phase 2: Targeting + Hover
- Phase 3: Combat + Entities (merged)
- Phase 4: Name Tags + Interaction
- Phase 5: Cleanup

---

### C5. HandleEnemyAttack Breaks in Phase 3 (Not Phase 6)

**Problem:** `MultiplayerEventSubsystem::HandleEnemyAttack()` uses `ProcessEvent` to call `BP_EnemyManager.GetEnemyActor()` and `BP_EnemyCharacter.PlayAttackAnimation()`. When Phase 3 replaces BP actors with C++ actors, these ProcessEvent calls fail.

**Fix:** Migrate `enemy:attack` handler to `UEnemySubsystem` during Phase 3 (merged), not Phase 6. The new handler:
```cpp
void UEnemySubsystem::HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data)
{
    int32 EnemyId = /* parse from Data */;
    if (AMMOEnemyActor* Enemy = GetEnemy(EnemyId))
        Enemy->PlayAttackAnimation();
}
```
Remove the `HandleEnemyAttack` function, `EnemyManagerActor` reference, and `FindEnemyManagerActor()` from `MultiplayerEventSubsystem` in the same step.

---

### C6. NPC Name Rendering Duplication

**Problem:** `WorldHealthBarSubsystem` already renders NPC names via `SWorldHealthBarOverlay::DrawNPCName()` in OnPaint (Z=8). `UNameTagSubsystem` (Phase 4) would render NPC names AGAIN at Z=7.

**Fix:** When `UNameTagSubsystem` is implemented in Phase 4, remove NPC name rendering from `WorldHealthBarSubsystem`:
- Remove `TArray<FNPCNameData> NPCNames` from header
- Remove `CacheNPCActors()` method and timer
- Remove `FNPCNameData` struct
- Remove `DrawNPCName()` from `SWorldHealthBarOverlay`
- Remove the NPC name loop from `OnPaint`

`UNameTagSubsystem` becomes the single source of truth for ALL entity names (players, enemies, NPCs).

---

## HIGH Issues (Should Fix)

### H1. SkillTreeSubsystem Targeting Handoff Rules Missing

**Problem:** `SkillTreeSubsystem` has a full targeting system (`BeginTargeting`, `CancelTargeting`, `SSkillTargetingOverlay`). `UTargetingSubsystem` also does per-tick cursor traces and target management. No handoff protocol defined.

**Fix:** Add to `UTargetingSubsystem::Tick`:
```cpp
// Pause hover detection when skill targeting is active
if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
{
    if (SkillSub->IsInTargetingMode())
        return; // SkillTreeSubsystem owns cursor and click handling
}
```
Additionally, update `SkillTreeSubsystem::GetEnemyIdFromActor()` to handle both `BP_EnemyCharacter` (property reflection) AND `AMMOEnemyActor` (direct C++ member access) during the transition.

---

### H2. Walk-to-Attack vs Walk-to-Cast Mutual Cancellation

**Problem:** Both systems call `SimpleMoveToLocation()` on timers. If both active, pawn jitters between two destinations.

**Fix:** Add public cancellation API:
- `USkillTreeSubsystem`: Add `CancelWalkToCast()` public method that calls internal `WalkToCast::Cancel()`
- `UPlayerInputSubsystem::StartAttacking()`: Call `SkillTreeSubsystem->CancelWalkToCast()`
- `SkillTreeSubsystem::BeginTargeting()`: Call `CombatActionSubsystem->StopAutoAttack()` (which tells PlayerInputSubsystem to stop walk-to-attack)

---

### H3. Click-to-Interact Defined Twice (Plans 01 + 04)

**Problem:** Both Plan 01 and Plan 04 specify walk-to-interact with different code.

**Fix:** Plan 01 is canonical owner of click-to-interact (it owns `HandleLeftClick`). Plan 04 Section 7-8 should be removed and replaced with: "See Plan 01 Section 5 for the click-to-interact implementation. UNameTagSubsystem only handles name tag rendering; interaction routing is owned by UPlayerInputSubsystem."

---

### H4. Wrong Socket Event Names

**Problem:** Plans reference `combat:stop` (actual: `combat:stop_attack`), `combat:miss` (doesn't exist), `enemy:respawn` (doesn't exist, uses `enemy:spawn`).

**Fix:** Search-and-replace in all plan documents:
- `combat:stop` → `combat:stop_attack`
- Remove all references to `combat:miss`
- `enemy:respawn` → `enemy:spawn` (respawn is a re-emission of `enemy:spawn`)

---

## MEDIUM Issues

### M1. FindSpringArm May Find Wrong Component
Use `Cast<ASabriMMOCharacter>(Pawn)->GetCameraBoom()` accessor instead of `FindComponentByClass<USpringArmComponent>()`.

### M2. Actor Classification During Transition
Use property reflection (check for `EnemyId` property > 0) for enemy identification, not name-contains checks. This is already proven in `SkillTreeSubsystem::GetEnemyIdFromActor()`.

### M3. TSubclassOf Hardcoded Path Fragile
Use `UPROPERTY(config)` with `DefaultGame.ini` entry, or `FSoftClassPath` with fallback to C++ base class + error log.

### M4. Hover Indicator Ownership
`UTargetingSubsystem` owns hover indicators. For C++ actors (Phase 3), use a Slate overlay ring or add `UStaticMeshComponent` ground ring to actor constructors. Do NOT use WidgetComponents on C++ actors.

### M5. inventory:used No C++ Handler
Add `HandleItemUsed` to `InventorySubsystem` before removing bridge. Shows "Used Red Potion" feedback.

### M6. inventory:equipped Bridge Not Removable Yet
Keep bridge until visual equipment mesh attachment is migrated to C++. Mark as Phase 5 cleanup, not Phase 3.

### M7. AIController Dependency
Add comment to Plan 01 explaining that `SimpleMoveToLocation` works with `PlayerController` via UE5's internal `UNavigationSystemV1` fallback. Consider adding `AIControllerClass` to C++ constructor for explicitness.

### M8. Phase Numbering
Each plan file header should state: "File XX — Master Plan Phase Y: [Name]"

---

## LOW Issues

### L1. Bridge Event Count
Fix log message in `MultiplayerEventSubsystem.cpp` line 138 from "31" to actual count. Update plan references.

### L2. "Already in C++" Table Inflated
Plan 05: Separate into "Dual-handled (bridge + C++)" vs "C++ only (never bridged)" categories.

### L3. MouseMoveAction Wrong IMC
Plan 01: Change `GameplayIMC->MapKey(MouseMoveAction, ...)` to `CameraIMC->MapKey(MouseMoveAction, ...)`.

### L4. Heuristic Actor Caching
Add warning: "Do not ship a build in BP+C++ coexistence state. Keep transition period short."

### L5. Z=7 Documentation
Update MEMORY.md with Z=7 assignment when UNameTagSubsystem is implemented.

### L6. Single Mesh Enemy Follow-Up
Note in Plan 03: "When per-template enemy meshes are added, switch to runtime mesh loading via TemplateId."

---

## Revised Implementation Order (100% Confidence)

```
PHASE 1: Camera + Movement (Foundation — no dependencies)
├── UCameraSubsystem (right-click rotate, zoom)
├── UPlayerInputSubsystem (click-to-move, WASD, click-to-attack EMIT ONLY)
├── Set bConsumeInput=true on all C++ input mappings
├── Remove IA_ClickToMove, IA_Attack, IA_CameraRotate from IMC_MMOCharacter in editor
├── Set AIControllerClass in ASabriMMOCharacter constructor (optional but recommended)
└── TEST: movement, camera, click emits combat:attack correctly

PHASE 2: Targeting + Hover (depends on Phase 1)
├── UTargetingSubsystem (per-tick cursor trace, hover indicators, cursor changes)
├── SkillTreeSubsystem handoff: pause hover when bIsInTargetingMode
├── Actor classification via property reflection (not name-contains)
├── Remove AC_TargetingSystem component from BP_MMOCharacter
└── TEST: cursor changes, hover indicators, skill targeting still works

PHASE 3: Combat + Entities — MERGED (depends on Phase 2)
├── Step 1: Create AMMORemotePlayer + AMMOEnemyActor (C++ actor classes)
│   ├── Minimal BP subclasses for mesh/anim (BP_MMORemotePlayer, BP_MMOEnemy)
│   ├── UOtherCharacterMovementComponent reuse for floor snap
│   └── ECC_Visibility collision for click detection
├── Step 2: Create UOtherPlayerSubsystem + UEnemySubsystem
│   ├── Register for player:moved, player:left, enemy:spawn/move/death/health_update/attack
│   ├── TMap<int32, TWeakObjectPtr<Actor>> entity registries
│   ├── Remove player/enemy bridges from MultiplayerEventSubsystem (6+1 events)
│   └── Migrate HandleEnemyAttack from MultiplayerEventSubsystem
├── Step 3: Create UCombatActionSubsystem
│   ├── Register for ALL combat events (damage, auto_attack_started/stopped, target_lost, out_of_range, death, respawn, error)
│   ├── Remove combat bridges from MultiplayerEventSubsystem (8 events)
│   ├── HandleCombatDamage: attack animation + rotation (replaces 215-node BP)
│   ├── Death overlay (Slate Z=40), respawn teleport
│   └── Walk-to-attack mutual cancellation with WalkToCast
├── Step 4: Remove BP_OtherPlayerManager + BP_EnemyManager from levels
└── TEST: entities spawn/move/die, combat works, animations play, no duplicates

PHASE 4: Name Tags + Interaction (depends on Phase 3)
├── UNameTagSubsystem + SNameTagOverlay (Slate OnPaint, Z=7)
├── Remove NPC name rendering from WorldHealthBarSubsystem
├── Register entities from UOtherPlayerSubsystem + UEnemySubsystem + NPC BeginPlay
├── Remove NameTagWidget components from all BP actors
└── TEST: all entity names render correctly, NPC names green, no duplicates

PHASE 5: Cleanup
├── Remove AC_CameraController from BP_MMOCharacter
├── Remove dead BP event graph nodes
├── Remove stale MultiplayerEventSubsystem bridges (chat, loot stay)
├── Remove WBP_PlayerNameTag widget Blueprint
├── Update MEMORY.md, CLAUDE.md, docsNew
└── TEST: full regression, no Blueprint dependencies remain
```

---

**Last Updated:** 2026-03-13
**Status:** AUDIT COMPLETE — All fixes documented, ready for implementation
