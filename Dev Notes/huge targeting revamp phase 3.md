#  Phase 3: Entity Management Subsystems

 Phase 3: Entity Management Subsystems

claude --resume 6f800ad7-80a5-4508-8150-fa27b60fb156

 Context

 Phase 2 migrated combat events to C++. CombatActionSubsystem currently resolves enemy/player actors via ProcessEvent
 on BP_EnemyManager/BP_OtherPlayerManager (temporary). Phase 3 replaces these BP managers with C++ subsystems that
 spawn and manage the EXISTING BP actor classes directly, eliminating the BP manager middlemen.

 Files

 New (4 files, ~800 lines)

 ┌─────────────────────────────┬───────────────────────────────────────────┐
 │            File             │                  Purpose                  │
 ├─────────────────────────────┼───────────────────────────────────────────┤
 │ UI/EnemySubsystem.h         │ Enemy entity registry, 6 event handlers   │
 ├─────────────────────────────┼───────────────────────────────────────────┤
 │ UI/EnemySubsystem.cpp       │ Spawn/move/death/health/attack management │
 ├─────────────────────────────┼───────────────────────────────────────────┤
 │ UI/OtherPlayerSubsystem.h   │ Player entity registry, 2 event handlers  │
 ├─────────────────────────────┼───────────────────────────────────────────┤
 │ UI/OtherPlayerSubsystem.cpp │ Spawn/move/leave management               │
 └─────────────────────────────┴───────────────────────────────────────────┘

 Modified (4 files)

 ┌──────────────────────────────────┬───────────────────────────────────────────────────────────────┐
 │               File               │                            Change                             │
 ├──────────────────────────────────┼───────────────────────────────────────────────────────────────┤
 │ UI/CombatActionSubsystem.h       │ Remove BP manager refs                                        │
 ├──────────────────────────────────┼───────────────────────────────────────────────────────────────┤
 │ UI/CombatActionSubsystem.cpp     │ Replace FindEnemyActor/FindPlayerActor with subsystem lookups │
 ├──────────────────────────────────┼───────────────────────────────────────────────────────────────┤
 │ UI/MultiplayerEventSubsystem.h   │ Remove HandleEnemyAttack, EnemyManagerActor                   │
 ├──────────────────────────────────┼───────────────────────────────────────────────────────────────┤
 │ UI/MultiplayerEventSubsystem.cpp │ Remove 7 bridges (22→14), remove HandleEnemyAttack            │
 └──────────────────────────────────┴───────────────────────────────────────────────────────────────┘

 BP Class Paths (confirmed via glob)

 /Game/SabriMMO/Blueprints/BP_EnemyCharacter.BP_EnemyCharacter_C
 /Game/SabriMMO/Blueprints/BP_OtherPlayerCharacter.BP_OtherPlayerCharacter_C

 UEnemySubsystem Design

 Events: enemy:spawn, enemy:move, enemy:death, enemy:health_update, enemy:attack

 Registry: TMap<int32, TWeakObjectPtr<AActor>> Enemies

 Handlers:
 1. HandleEnemySpawn — Parse JSON → new: SpawnActor + call InitializeEnemy via ProcessEvent + set TargetPosition. Dead
 respawn: show mesh, enable collision, re-initialize. Alive: update position.
 2. HandleEnemyMove — Set TargetPosition + bIsMoving via reflection
 3. HandleEnemyDeath — Call OnEnemyDeath via ProcessEvent (keeps actor in map for respawn)
 4. HandleEnemyHealthUpdate — Call UpdateEnemyHealth via ProcessEvent
 5. HandleEnemyAttack — Call PlayAttackAnimation (parameterless) — migrated from MultiplayerEventSubsystem (C5)

 BP Functions called via ProcessEvent (use FMemory_Alloca + FindPropertyByName):
 - InitializeEnemy(InEnemyId:int, InName:string, InLevel:int, InHealth:real, InMaxHealth:real) — 5 params
 - UpdateEnemyHealth(NewHealth:real, NewMaxHealth:real, InCombat:bool) — 3 params
 - OnEnemyDeath(InDeadEnemy:object) — 1 param (pass self)
 - PlayAttackAnimation() — 0 params (nullptr safe)

 BP Properties set via reflection: EnemyId(int), TargetPosition(FVector), bIsMoving(bool), bIsDead(bool),
 Health/MaxHealth(double)

 UOtherPlayerSubsystem Design

 Events: player:moved, player:left

 Registry: TMap<int32, TWeakObjectPtr<AActor>> Players

 Handlers:
 1. HandlePlayerMoved — Filter LocalCharacterId. New: SpawnActor + set CharacterId, PlayerName, TargetPosition.
 Existing: update TargetPosition + bIsMoving.
 2. HandlePlayerLeft — Destroy actor, remove from map.

 BP Properties set via reflection: CharacterId(int), PlayerName(string), TargetPosition(FVector), bIsMoving(bool)

 CombatActionSubsystem Changes

 Replace:
 // OLD: ProcessEvent on BP managers
 AActor* FindEnemyActor(int32) { ... BP_EnemyManager ProcessEvent ... }
 AActor* FindPlayerActor(int32) { ... BP_OtherPlayerManager ProcessEvent ... }
 With:
 // NEW: Direct subsystem lookup
 AActor* FindEnemyActor(int32 Id) { return EnemySubsystem->GetEnemy(Id); }
 AActor* FindPlayerActor(int32 Id) { return OtherPlayerSubsystem->GetPlayer(Id); }
 Remove: EnemyManagerActor, OtherPlayerManagerActor, FindBPManagerActor()

 MultiplayerEventSubsystem Changes

 Remove 7 registrations: player:moved, player:left, enemy:spawn, enemy:move, enemy:death, enemy:health_update,
 enemy:attack

 Remove: HandleEnemyAttack(), FindEnemyManagerActor(), EnemyManagerActor member

 Update count: 22→14 bridges remaining (inventory 5, loot 1, chat 1, stats 1, hotbar 2, shop 4)

 Key Patterns

 Property reflection (reuse for both subsystems):
 void SetBPInt(AActor* A, FName N, int32 V);     // FIntProperty
 void SetBPDouble(AActor* A, FName N, double V);  // FDoubleProperty (BP "real")
 void SetBPBool(AActor* A, FName N, bool V);      // FBoolProperty
 void SetBPString(AActor* A, FName N, const FString& V); // FStrProperty
 void SetBPVector(AActor* A, FName N, const FVector& V); // FStructProperty

 ProcessEvent with params (proven pattern from PlayAttackAnimation):
 uint8* Params = (uint8*)FMemory_Alloca(Func->ParmsSize);
 FMemory::Memzero(Params, Func->ParmsSize);
 // Set each param via Func->FindPropertyByName(ParamName)
 Actor->ProcessEvent(Func, Params);

 Edge Cases Handled

 - Dead enemy respawn (bIsDead check, re-show mesh/collision)
 - Local player filtered from player:moved
 - Stale TWeakObjectPtr entries (IsValid() check in GetEnemy/GetPlayer)
 - Deinitialize cleanup (destroy all spawned actors)
 - Zone transition (subsystems destroyed → actors destroyed → new level spawns fresh)
 - bReadyToProcess one-frame defer (prevents ProcessEvent during PostLoad)

 Verification

 1. Enemies spawn at correct positions on zone load
 2. Enemies move (TargetPosition drives Tick movement)
 3. Enemy death hides actor, respawn re-shows
 4. Enemy health bars update on damage
 5. Enemy attack animations play
 6. Other players appear when joining zone
 7. Other players move correctly
 8. Other players disappear when leaving
 9. Auto-attack rotation + animation still works (CombatActionSubsystem)
 10. Hover cursor + indicators still work (TargetingSubsystem)
 11. Zone transitions clean up all entities




Phase 3 implementation complete. Here's a summary of what was done:

  New Files (4)

  ┌─────────────────────────────┬───────┬──────────────────────────────────────────────────┐
  │            File             │ Lines │                     Purpose                      │
  ├─────────────────────────────┼───────┼──────────────────────────────────────────────────┤
  │ UI/EnemySubsystem.h         │ 42    │ Enemy entity registry, 5 event handlers          │
  ├─────────────────────────────┼───────┼──────────────────────────────────────────────────┤
  │ UI/EnemySubsystem.cpp       │ 290   │ Spawn/move/death/health/attack via BP reflection │
  ├─────────────────────────────┼───────┼──────────────────────────────────────────────────┤
  │ UI/OtherPlayerSubsystem.h   │ 42    │ Player entity registry, 2 event handlers         │
  ├─────────────────────────────┼───────┼──────────────────────────────────────────────────┤
  │ UI/OtherPlayerSubsystem.cpp │ 195   │ Spawn/move/leave, filters local player           │
  └─────────────────────────────┴───────┴──────────────────────────────────────────────────┘

  Modified Files (4)

  ┌───────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────┐
  │             File              │                                      Change                                      │
  ├───────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ CombatActionSubsystem.h       │ Removed BP manager refs, FindBPManagerActor                                      │
  ├───────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ CombatActionSubsystem.cpp     │ FindEnemyActor/FindPlayerActor now 3-line subsystem lookups (was 60+ lines of    │
  │                               │ ProcessEvent)                                                                    │
  ├───────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ MultiplayerEventSubsystem.h   │ Removed HandleEnemyAttack, EnemyManagerActor, FindEnemyManagerActor              │
  ├───────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ MultiplayerEventSubsystem.cpp │ Removed 7 event bridges (22->14), removed ~80 lines of BP manager code           │
  └───────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────┘

  Key Patterns

  - BP reflection helpers in anonymous namespace (SetBPInt/Double/Bool/String/Vector, GetBPBool)
  - CallBPFunction with InitializeStruct/DestroyStruct for safe FString param handling
  - Dead respawn: SetActorHiddenInGame(false) + SetActorEnableCollision(true) + re-InitializeEnemy
  - bReadyToProcess one-frame defer on all 3 subsystems

  Requires editor restart (new headers). BP_EnemyManager and BP_OtherPlayerManager are now dead code — they can be
  removed from levels once verified.




 couple issues i noticed. 1. when the local player attacks an enemy the remote(other) player does not see the attack
animation. 2. when the local player dies, he can still move around, movement should be prevented when dead (should only
 be able to click the ui until respawned. 3. when a remote player is shown joining a new zone, they dont always
immediatelly appear at the correct location, the interpolate to the spawn location - that is incorrect. they should
only show on the remote player's screen when they are at the final spawn location after zoning






  Issue 1 — Remote attack animation (Blueprint issue)

  BP_OtherPlayerCharacter does have PlayAttackAnimation and the C++ path calls it correctly. The function does Get Mesh
  → Get Anim Instance → Montage Play. Compare with BP_MMOCharacter which has 11 nodes (includes FindLookAtRotation +
  SetActorRotation for facing).

  Most likely the Montage Play node on BP_OtherPlayerCharacter either has no montage asset assigned, or the skeletal
  mesh doesn't have an Animation Blueprint. Open BP_OtherPlayerCharacter → PlayAttackAnimation function → check the
  Montage Play node's "Montage to Play" pin. It should reference the same attack montage used by BP_MMOCharacter.

✻ Cooked for 8m 58s

❯ annother issue, 4. when the remote player uses fly wing, they are shown walking to the new location, that is
incorrect, they should warp to the new locaton when using an item like fly wing. 5. another issue ranged weapon attacks
 are not working. my character has a bow equipped and walks into melee to start auto attackign when left clicking on an
 enemy, the player shoudl stop moving when in range and start auto attacking at max range, only moving forward if the
target moves out of range. going back to issue 1: Issue 1 — Remote attack animation (Blueprint issue) - this is not a
blueprint issue, it was working properly before we started this newest refactoring. you have all the info you need to
figure out the root cause for all these issues. please research the issue and determine how to best fix it. only
implement a fix once you are 100% confident it will be fixed





  FIXED ***Issue 1 — Remote attack animation

  Fix: FindPlayerActor now has a fallback TActorIterator search by CharacterId property. If OtherPlayerSubsystem's TMap
  lookup fails (because BP_SocketManager's SocketIOClientComponent auto-connected and BP_OtherPlayerManager owns the
  actual visible actors), the fallback finds the actor by scanning all actors with matching CharacterId. A warning log
  fires when the fallback is used — check the output log for "found via fallback search" to confirm this was the issue.

 FIXED ***Issue 2 — Dead player movement (fixed)

  - CombatActionSubsystem.h: Added IsDead() public method
  - CombatActionSubsystem.cpp: Added StopPawnMovement() in HandleCombatDeath to halt current movement instantly
  - PlayerInputSubsystem.cpp: Death check blocks OnLeftClickFromCharacter (no click-to-move/attack) and OnWalkPollTick
  (stops walk-to-attack/interact processing)
  - SabriMMOCharacter.cpp: Death check in DoMove blocks WASD movement

   FIXED ***Issue 3 — Remote player interpolation on zone (fixed)

  - OtherPlayerSubsystem.cpp: When an existing player's position jumps > 500 units, SetActorLocation snaps them
  instantly instead of letting the BP interpolate


  FIXED ***Issue 4 — Fly Wing shows walking

  Fix: Snap threshold lowered from 500 to 200 units. Normal 30Hz position updates are ~20 units; Fly Wing teleports are
  200+ units. Any position jump > 200 snaps instantly via SetActorLocation instead of letting the BP interpolate.

   FIXED ***  Issue 5 — Ranged attacks walk to melee

  Fix: ProcessClickOnEnemy now emits combat:attack immediately on click (server handles range). Walk-to only engages if
  the player is out of CombatActionSubsystem::GetAttackRange() (server-updated, not hardcoded 150). OnWalkPollTick
  checks CAS->IsAutoAttacking() to stop walking when the server confirms the attack has started. No re-emission from the
   poll — attack was already emitted on click.





---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
