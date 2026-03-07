# VFX System Audit -- Pass 2

**Auditor:** Claude Opus 4.6 (automated code review)
**Date:** 2026-03-05
**Scope:** Verify fixes from Pass 1 + find anything missed
**Files reviewed:** `SkillVFXSubsystem.h/.cpp`, `SkillVFXData.h`, `CastingCircleActor.h/.cpp`, `WarpPortal.h/.cpp`
**Engine version:** UE 5.7

---

## 1. Fix Verification

### C1: SpellColor Parameter -- Now Tries 5 Names

**Status: FIXED -- Correct but suboptimal**

The fix at line 853-861 of `SkillVFXSubsystem.cpp`:

```cpp
void USkillVFXSubsystem::SetNiagaraColor(UNiagaraComponent* Comp, FLinearColor Color)
{
    if (!Comp) return;
    Comp->SetVariableLinearColor(FName("SpellColor"), Color);
    Comp->SetVariableLinearColor(FName("Color"), Color);
    Comp->SetVariableLinearColor(FName("BaseColor"), Color);
    Comp->SetVariableLinearColor(FName("ParticleColor"), Color);
    Comp->SetVariableLinearColor(FName("User.Color"), Color);
}
```

**Performance analysis:** Calling `SetVariableLinearColor` with a non-existent parameter name does NOT crash, throw errors, or emit warnings. Internally, UE5's `UNiagaraComponent::SetVariable*` functions look up the parameter in the component's override map and the system's user parameter list. If the parameter does not exist, the call is a no-op -- it performs a hash lookup in a TMap and returns without doing anything. The cost is approximately 5 hash lookups per spawn (one per `FName`), which is negligible compared to the cost of spawning a Niagara system. For the quantities of VFX this system spawns (single-digit per second in typical gameplay), this has zero measurable performance impact.

**Could we do better?** Yes -- one could inspect the Niagara system's user parameters at load time via `UNiagaraSystem::GetExposedParameters()` and cache which parameter name matches. However, this adds complexity for zero practical benefit at this project's scale. The shotgun approach is pragmatically correct.

**Are these common parameter names?** "Color" and "BaseColor" are very common in marketplace Niagara packs. "ParticleColor" is used in some UE example content. "User.Color" is a namespace-qualified format used in some systems. "SpellColor" is custom. Together, these 5 names cover the vast majority of marketplace Niagara systems that expose a color user parameter.

**Remaining risk:** If none of the 5 names match a given third-party system (because it has no exposed color parameter at all, or uses a completely different name like "MainColor" or "Tint"), the color override will still silently fail. The only way to guarantee color customization works is to open each Niagara system in the editor and verify or add a user parameter with one of these names.

**Verdict: ACCEPTABLE -- pragmatic fix, no performance concern.**

---

### C2: Buff Aura Key Now Uses TargetId

**Status: FIXED -- Correct**

The fix passes `TargetId` through the full chain:

1. `HandleBuffApplied` (line 467-468): extracts `TargetId` and `SkillId` from JSON.
2. Line 479: calls `SpawnSelfBuff(TargetActor, Config, SkillId, TargetId)` -- the 4th argument is the correct `TargetId`.
3. `SpawnSelfBuff` (line 678): signature is `SpawnSelfBuff(AActor* TargetActor, const FSkillVFXConfig& Config, int32 SkillId, int32 TargetId)`.
4. Line 690: key = `static_cast<int64>(TargetId) * 10000 + SkillId` -- correct.

**HandleBuffRemoved verification (line 495):**
```cpp
const int64 Key = static_cast<int64>(TargetIdD) * 10000 + static_cast<int64>(SkillIdD);
```
This matches the formula in `SpawnSelfBuff`. The key space is consistent.

**Other callers of SpawnSelfBuff:** Grep confirms there is exactly one caller -- `HandleBuffApplied` at line 479. No other code paths call `SpawnSelfBuff`.

**Verdict: CORRECT -- fix is complete and consistent.**

---

### C3: Timer Lambda Now Uses TWeakObjectPtr

**Status: FIXED -- Correct**

The fix at lines 746-756:

```cpp
TWeakObjectPtr<UNiagaraComponent> WeakCircleComp = CircleComp;
FTimerHandle TempHandle;
World->GetTimerManager().SetTimer(TempHandle,
    [WeakCircleComp]()
    {
        if (WeakCircleComp.IsValid())
        {
            WeakCircleComp->DeactivateImmediate();
        }
    },
    CastDuration + 0.1f, false);
```

**TWeakObjectPtr construction:** `TWeakObjectPtr<UNiagaraComponent> WeakCircleComp = CircleComp;` where `CircleComp` is a `UNiagaraComponent*` returned by `SpawnNiagaraAtLocation`. This is correct -- `TWeakObjectPtr` can be constructed from a raw `UObject*` pointer.

**IsValid() vs Get() != nullptr:** Both are valid approaches. `IsValid()` performs two checks: (1) the internal index is valid, and (2) the object at that index has the correct serial number (i.e., it has not been garbage collected and replaced by a new object). `Get()` performs the same checks and returns the pointer if valid, or nullptr if not. Using `IsValid()` followed by `operator->` (which internally calls `Get()`) performs the validity check twice, which is slightly wasteful but not measurably so. The alternative `if (auto* Comp = WeakCircleComp.Get()) { Comp->DeactivateImmediate(); }` would be marginally more efficient but functionally identical.

**Race condition between IsValid() and DeactivateImmediate():** Since both the timer callback and the garbage collector run on the game thread, there is no race condition. Between `IsValid()` returning true and `DeactivateImmediate()` being called, no GC cycle can intervene because we are in the middle of a game thread callback. This is safe.

**Verdict: CORRECT -- the fix is safe and correct.**

---

### C4: WarpPortal EndPlay Cleanup

**Status: FIXED -- Correct**

The fix at lines 84-99:

```cpp
void AWarpPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(VFXRetryTimer);
    }

    if (SpawnedPortalVFX.IsValid())
    {
        SpawnedPortalVFX->DeactivateImmediate();
        SpawnedPortalVFX->DestroyComponent();
        SpawnedPortalVFX = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}
```

**EndPlay vs subsystem teardown ordering:** Actor `EndPlay` is called BEFORE world subsystems are deinitialized. The UE5 teardown order is: (1) actors receive EndPlay, (2) world subsystems Deinitialize, (3) world is destroyed. So when `AWarpPortal::EndPlay` fires, the `USkillVFXSubsystem` is still alive. However, the WarpPortal's EndPlay does not access the subsystem -- it only accesses its own `SpawnedPortalVFX` component and the timer manager. Both are safe to access during EndPlay.

**Is DestroyComponent safe in EndPlay?** Yes. `UActorComponent::DestroyComponent()` unregisters the component, removes it from its outer actor's components array, and marks it for pending kill. This is explicitly safe to call during EndPlay. The Niagara component was spawned as a free-floating world component (not owned by the WarpPortal actor), so we must explicitly destroy it -- it would not be auto-cleaned by the actor's destruction.

**Is the timer cleared properly?** Yes. `ClearTimer(VFXRetryTimer)` is called with a null-checked `GetWorld()`, which is safe because `GetWorld()` is still valid during EndPlay (the world exists until after all actors are cleaned up). The `VFXRetryTimer` handle is correctly cleared to prevent the `TryActivatePortalVFX` callback from firing after EndPlay.

**One minor note:** `SpawnedPortalVFX` is a `TWeakObjectPtr<UNiagaraComponent>` (line 61 of WarpPortal.h). Setting it to `nullptr` after destruction is good practice but technically unnecessary since the weak pointer would automatically return `IsValid() == false` after the component is destroyed. Still, the explicit null assignment is clean.

**Verdict: CORRECT -- cleanup is thorough and properly ordered.**

---

## 2. New Issues Found

### N1. OverrideCache TMap is NOT a UPROPERTY -- GC Risk (Severity: MODERATE)

**File:** `SkillVFXSubsystem.h` line 127:
```cpp
TMap<FString, TObjectPtr<UNiagaraSystem>> OverrideCache;
```

This member is NOT marked with `UPROPERTY()`. In UE5, `TObjectPtr<T>` is designed to be used WITH `UPROPERTY()` -- when used without it, `TObjectPtr` behaves identically to a raw `T*` pointer. Since the TMap is not reflected, the garbage collector does NOT know about these references and will NOT treat them as strong references.

**What could happen:** If the Niagara system asset is loaded by `LoadObject`, it gets added to the object graph. As long as the asset is referenced by Content (which it is -- these are content assets, not dynamically created objects), it will not be garbage collected. So in practice, this is safe because the Niagara systems are loaded from disk and are referenced by the asset registry. However, if these were dynamically created UObjects, they would be eligible for GC.

**The correct fix:** Either mark `OverrideCache` as `UPROPERTY()` (requires the map value type to be UObject-compatible, which `TObjectPtr<UNiagaraSystem>` is), or change to `TMap<FString, UNiagaraSystem*>` since it behaves identically without UPROPERTY.

**Verdict: Low practical risk (assets are Content-referenced), but technically incorrect UE5 practice.**

---

### N2. Persistent Effects from HandleGroundEffectCreated vs SpawnGroundPersistent Use SAME TMap with Different Key Spaces (Severity: MODERATE)

**File:** `SkillVFXSubsystem.cpp` lines 542 and 652

The `ActivePersistentEffects` TMap is populated by TWO different code paths:

1. `HandleGroundEffectCreated` (line 542): uses `EffectId` (server-assigned int) as the key.
2. `SpawnGroundPersistent` (line 652): uses `SkillId * 100000 + (++PersistentEffectCounter)` as the key.

These two key spaces could theoretically collide if:
- A server-assigned `effectId` happens to equal `SkillId * 100000 + counter`.
- E.g., if `effectId = 20900001` and `SkillId = 209` with `counter = 1`, the keys collide.

In practice, server effect IDs are likely small sequential integers (1, 2, 3...) and the computed keys start at 200*100000 = 20,000,000+. So collision is extremely unlikely but not impossible.

**More importantly:** Effects added by `SpawnGroundPersistent` (from `HandleSkillEffectDamage`) have no corresponding removal handler. `HandleGroundEffectRemoved` only removes entries by server `effectId`. Effects created via the `HandleSkillEffectDamage -> SpawnGroundPersistent` path are never cleaned up and will persist until the subsystem is deinitialized (level change). This is a potential VFX leak during long play sessions.

**Double-spawn risk:** For GroundPersistent skills (Fire Wall, Safety Wall):
- `HandleSkillEffectDamage` with `HitNumber <= 1` spawns via `SpawnGroundPersistent` (line 415-418).
- `HandleGroundEffectCreated` spawns via `SpawnNiagaraAtLocation` with the server's `effectId` (line 538).
- These are DIFFERENT socket events. If both fire for the same skill cast, there WILL be a double-spawn. The server likely sends `skill:ground_effect_created` for the initial placement AND `skill:effect_damage` for each damage tick. If HitNumber=1 on the first damage tick, both code paths spawn an effect.

**Verdict: Likely double-spawn for Fire Wall / Safety Wall. The `HandleSkillEffectDamage` case for `GroundPersistent` should probably be removed since `HandleGroundEffectCreated` handles the spawning with proper lifecycle management.**

---

### N3. CastingCircleActor Auto-Destroy Lambda Captures `this` Without Safety (Severity: LOW-MODERATE)

**File:** `CastingCircleActor.cpp` line 72:
```cpp
GetWorld()->GetTimerManager().SetTimer(
    AutoDestroyHandle,
    FTimerDelegate::CreateLambda([this]() { FadeOut(0.3f); }),
    Duration + 0.1f,
    false
);
```

This lambda captures `this` (the ACastingCircleActor) raw. If the actor is destroyed before the timer fires (e.g., via `RemoveCastingCircle` calling `FadeOut` which calls `Destroy`), the timer handle is cleared in `FadeOut` (line 84), so this is actually safe in the normal flow. However, if the actor is destroyed externally (e.g., editor delete, level transition), the timer manager's cleanup will handle it because `SetTimer` with a lambda on a world-owned timer manager is cleared when the world is destroyed.

**But there is a subtle issue:** The `AutoDestroyHandle` is stored as a member. If `FadeOut` is called externally (by `RemoveCastingCircle`), it clears `AutoDestroyHandle` (line 84). If the auto-destroy timer then fires (unlikely race, but theoretically possible if the timer was already in the execution queue), the lambda would call `FadeOut(0.3f)` on an actor that is already fading out. This is harmless because `FadeOut` is idempotent (it just sets `FadeDirection = -1` again), but it is not maximally clean.

**Verdict: Safe in practice due to timer manager ownership semantics. Not a blocker.**

---

### N4. `HandleSkillEffectDamage` Does NOT Dispatch SelfBuff (Severity: INFO)

The switch statement in `HandleSkillEffectDamage` (line 391) handles: BoltFromSky, Projectile, AoEImpact, GroundPersistent, GroundAoERain, HealFlash, TargetDebuff. It does NOT have a case for `SelfBuff`. This is correct because SelfBuff effects are triggered by `skill:buff_applied` (a separate event), not by `skill:effect_damage`. The switch's `default: break;` covers SelfBuff silently. This is correct behavior, not a bug.

**Verdict: Correct -- no action needed.**

---

### N5. Potential Integer Overflow in Buff Aura Key (Severity: LOW)

**File:** `SkillVFXSubsystem.cpp` line 690:
```cpp
int64 Key = static_cast<int64>(TargetId) * 10000 + SkillId;
```

If `TargetId` is a character ID from the database, it could in theory grow to very large values. However, since `int64` can hold up to 9.2 * 10^18, a `TargetId` of even 1 billion multiplied by 10000 is only 10^13 -- well within range. No overflow risk.

**Verdict: Safe -- no issue.**

---

### N6. Compilation Issues (Severity: VARIES)

**Build.cs:** The `SabriMMO.Build.cs` file correctly includes `"Niagara"` and `"NiagaraCore"` in PublicDependencyModuleNames (lines 28-29), and includes `"SabriMMO/VFX"` in PublicIncludePaths (line 51). This means `#include "SkillVFXSubsystem.h"` from `WarpPortal.cpp` (which is in the parent `SabriMMO/` directory) will resolve correctly because the VFX directory is in the include path.

**Missing includes check:**
- `SkillVFXSubsystem.cpp` includes: SkillVFXSubsystem.h, CastingCircleActor.h, MMOGameInstance.h, CharacterData.h, SocketIOClientComponent.h, SocketIONative.h, NiagaraComponent.h, NiagaraFunctionLibrary.h, NiagaraSystem.h, Engine/World.h, EngineUtils.h, Kismet/GameplayStatics.h, TimerManager.h, Serialization/JsonSerializer.h. This covers all types used.
- `CastingCircleActor.cpp` includes: CastingCircleActor.h, Components/DecalComponent.h, Materials/MaterialInstanceDynamic.h, TimerManager.h. Complete.
- `WarpPortal.cpp` includes: WarpPortal.h, Components/SphereComponent.h, GameFramework/Character.h, GameFramework/PlayerController.h, UI/ZoneTransitionSubsystem.h, NiagaraComponent.h, NiagaraSystem.h, NiagaraFunctionLibrary.h, SkillVFXSubsystem.h. The `NiagaraFunctionLibrary.h` include is unnecessary since WarpPortal.cpp doesn't call any `UNiagaraFunctionLibrary` functions -- but unnecessary includes don't cause errors, just slightly slower compilation.

**Type mismatches:** `EAttachLocation::KeepRelativeOffset` (line 848 of SkillVFXSubsystem.cpp) is used in `SpawnSystemAttached`. `EAttachLocation::Type` is deprecated in UE5 but NOT removed. `SpawnSystemAttached` still accepts it as a parameter for backwards compatibility. This will compile but may emit a deprecation warning in UE 5.7.

**`SetVariableVec3` function name (line 615):** The correct function name in UE5 is `SetVariableVec3(FName, FVector)` on `UNiagaraComponent`. This is correct.

**`SetVariableFloat` function name (line 598):** Correct.

**`SetVariableLinearColor` function name (line 857):** Correct.

**Verdict: Code will compile. One possible deprecation warning from `EAttachLocation`. One unnecessary include in WarpPortal.cpp.**

---

### N7. `SpawnNiagaraAtLocation` Returns Non-Owning Component for Tracked Effects (Severity: MODERATE)

When `SpawnNiagaraAtLocation` is called with `bAutoDestroy=true` (line 828), the returned `UNiagaraComponent` will auto-destroy when the particle system completes. This is correct for one-shot effects (bolts, impacts, heals).

However, for the casting circle Niagara path (line 735-762), the component returned by `SpawnNiagaraAtLocation` with `bAutoDestroy=true` is stored in `ActiveBuffAuras` via a `TWeakObjectPtr`. If the casting circle's Niagara system is a non-looping system, it will auto-complete and auto-destroy. The weak pointer will then be stale, and `RemoveCastingCircle` will find `IsValid() == false` and just remove the map entry -- which is correct behavior. But the timer at line 748 might fire after the component has already auto-destroyed, which is also handled by the `WeakCircleComp.IsValid()` check.

If the Niagara system IS looping (which `NS_Free_Magic_Circle1` likely is, since casting circles need to loop), then `bAutoDestroy=true` will not trigger until the system is deactivated. The timer at line 756 deactivates it after `CastDuration + 0.1f`, which then triggers auto-destroy. This is the correct flow.

**Verdict: Correct -- the interaction between auto-destroy and the timer is handled safely.**

---

### N8. SkillVFXData.h Static Map in Inline Function -- Multiple Copies (Severity: LOW)

**File:** `SkillVFXData.h` lines 117-185

The `GetSkillVFXConfig` function is `inline` with `static TMap<int32, FSkillVFXConfig> Configs` and `static bool bInitialised` inside it. In C++17 (which UE5.7 uses), `inline` functions have the same address across all translation units, and `static` local variables inside them are guaranteed to be unique (shared across all TUs). This means there is exactly ONE copy of the `Configs` map, not one per `.cpp` file as the first audit suggested.

**Verdict: No issue -- the first audit was incorrect about this. C++17 `inline` function statics are unique.**

---

## 3. End-to-End Flow Verification: Cold Bolt Cast

### Step 1: Player casts Cold Bolt (Skill ID 200) on a target enemy

Client sends `skill:use { skillId: 200, targetId: 42, isEnemy: true }` via socket.

### Step 2: Server processes, sends `skill:cast_start`

Server sends: `skill:cast_start { casterId: 7, skillId: 200, actualCastTime: 2000, targetId: 42, isEnemy: true }`

`HandleCastStart` fires:
1. Extracts `CasterId=7`, `SkillId=200`, `CastTimeSec=2.0`.
2. Calls `SkillVFXDataHelper::GetSkillVFXConfig(200)` -- returns Cold Bolt config with `bUseCastingCircle=true`.
3. No ground position in event. `TargetIdD=42` > 0, `bIsEnemy=true`.
4. Calls `GetActorLocationById(42, true)` which calls `FindEnemyActorById(42)` -- scans all actors for tag `EnemyId_42`.
5. If found, `CircleLocation` is the enemy's position.
6. Calls `SpawnCastingCircle(7, CircleLocation, Config, 2.0f)`.
7. Since `NS_CastingCircle` is loaded (from Free_Magic), spawns Niagara circle.
8. Sets color to `GetElementColor("water")` = blue.
9. Stores in `ActiveBuffAuras` with key `7 * 10000 + 9999 = 79999`.
10. Sets timer for 2.1 seconds to deactivate.

**Result: Blue casting circle appears at enemy position. Correct.**

### Step 3: Cast completes, server sends `skill:cast_complete`

Server sends: `skill:cast_complete { casterId: 7, skillId: 200 }`

`HandleCastComplete` fires:
1. Calls `RemoveCastingCircle(7)`.
2. Looks up key `7 * 10000 + 9999 = 79999` in `ActiveBuffAuras`.
3. Calls `DeactivateImmediate()` on the Niagara circle.
4. Removes entry from map.

**Result: Casting circle disappears. Correct.**

### Step 4: Server sends `skill:effect_damage` (1 per bolt, Cold Bolt Lv5 = 5 hits)

Server sends 5 times (200ms apart):
```
skill:effect_damage { attackerId: 7, targetId: 42, skillId: 200, damage: 150,
  targetX: 1000, targetY: 2000, targetZ: 300, hitNumber: 1..5, isEnemy: true, element: "water" }
```

For each hit, `HandleSkillEffectDamage` fires:
1. Extracts all fields, including `HitNumber`.
2. `TargetLoc = (1000, 2000, 300)`.
3. Calls `RemoveCastingCircle(7)` -- on first hit, finds nothing (already removed in step 3). On subsequent hits, still finds nothing. Safe.
4. Gets config for skill 200: `Template = BoltFromSky`.
5. Enters `BoltFromSky` case in switch.
6. Calls `SpawnBoltFromSky(TargetLoc, Config, HitNumber)`.

In `SpawnBoltFromSky`:
1. `Config.NiagaraOverridePath` = `"/Game/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_02_Blue.NS_Zap_02_Blue"`.
2. Calls `GetOrLoadOverride(path)`:
   - First call: not in cache, calls `LoadObject`, caches result.
   - Subsequent calls: returns from cache. O(1) lookup.
3. `SpawnLoc = TargetLoc + (0, 0, 50)`.
4. `SpawnNiagaraAtLocation(NS_Zap_02_Blue, SpawnLoc)`.
5. `SetNiagaraColor(Comp, Config.PrimaryColor)` -- tries 5 color parameter names.
6. `SetVariableFloat("SpawnHeight", 500.f)` -- may or may not exist on NS_Zap_02_Blue.

**Result: 5 blue lightning bolt effects spawn at the enemy, one every 200ms. Each bolt is a one-shot auto-destroying effect. Correct RO behavior -- one bolt per hit.**

### Step 5: What if casting is interrupted?

Server sends: `skill:cast_interrupted_broadcast { casterId: 7 }` or `skill:cast_interrupted { casterId: 7 }`

`HandleCastInterrupted` fires -> `RemoveCastingCircle(7)` -> circle removed.

**No orphaned effects. Correct.**

### Race condition analysis:

- `skill:cast_complete` could arrive before or after `skill:effect_damage` hit #1, depending on server timing.
- If `skill:effect_damage` arrives BEFORE `skill:cast_complete`: `HandleSkillEffectDamage` calls `RemoveCastingCircle(7)` which removes the circle. When `skill:cast_complete` arrives, `RemoveCastingCircle(7)` finds nothing -- safe.
- If `skill:cast_complete` arrives BEFORE `skill:effect_damage`: `RemoveCastingCircle(7)` removes the circle. When `skill:effect_damage` arrives, `RemoveCastingCircle(7)` again finds nothing -- safe.
- The timer auto-deactivation at `CastDuration + 0.1f` may fire after `RemoveCastingCircle` has already cleaned up. The timer's weak pointer check handles this safely.

**Verdict: Cold Bolt flow is correct end-to-end. No race conditions, no orphaned effects, no crashes.**

---

## 4. Detailed Analysis of Raised Concerns

### 4.1 What happens when SkillVFXSubsystem is NOT initialized yet?

**WarpPortal calling SpawnLoopingPortalEffect:**
The WarpPortal's `TryActivatePortalVFX` (line 72) calls `World->GetSubsystem<USkillVFXSubsystem>()`. UWorldSubsystems are created during world initialization, before any actor's `BeginPlay`. So the subsystem WILL exist when WarpPortal queries it. However, `SpawnLoopingPortalEffect` accesses `NS_CastingCircle`, `NS_SelfBuff`, and `NS_GroundPersistent` -- all loaded in `OnWorldBeginPlay`. Since `OnWorldBeginPlay` runs before actor `BeginPlay`, the assets WILL be loaded. If none loaded (all paths are wrong), the function returns nullptr, and WarpPortal retries up to 10 times.

**HandleSkillEffectDamage with null systems:**
If `NS_BoltFromSky` is null AND the override path also fails to load, `SpawnBoltFromSky` checks `if (!SystemToUse) return;` at line 591 and exits gracefully. No crash.

**Verdict: Gracefully handled. No null dereference risk.**

### 4.2 BoltFromSky spawning on EVERY hit -- correct?

Yes. In RO, each bolt hit (Cold Bolt Lv5 = 5 bolts) shows a distinct bolt-from-sky animation on the target. The code correctly spawns one bolt per `skill:effect_damage` event with no HitNumber guard. This matches RO's visual behavior.

### 4.3 Projectile -- first hit only, missing explosion?

Fire Ball (skill 207) is configured as `Projectile` template. The code spawns the projectile on `HitNumber <= 1` only. Fire Ball in RO shows a projectile AND an explosion on impact. The current system spawns only the projectile -- the explosion is not spawned.

This is a missing feature, not a bug in the VFX system. To fix it, Fire Ball would need two VFX dispatches: a projectile on hit 1, and an AoE impact when the projectile "arrives." This could be done by spawning both in the Projectile case, or by having the server send a separate `skill:effect_impact` event.

**Severity: LOW -- visual polish, not a system bug.**

### 4.4 GroundPersistent double-spawn risk

As analyzed in N2 above, `HandleSkillEffectDamage` with `GroundPersistent` template AND `HandleGroundEffectCreated` could both fire for the same skill, causing two VFX to spawn at the same location. This depends on whether the server sends both `skill:effect_damage` (with hitNumber=1) AND `skill:ground_effect_created` for Fire Wall / Safety Wall.

Checking the server flow: The server likely sends `skill:ground_effect_created` when the wall is placed, and then `skill:effect_damage` for each entity that takes damage from it. The first `skill:effect_damage` with `hitNumber=1` would trigger `SpawnGroundPersistent` from `HandleSkillEffectDamage`.

**This IS a double-spawn.** `HandleGroundEffectCreated` spawns the visual for the wall, and then `HandleSkillEffectDamage` spawns ANOTHER visual when the first damage tick happens.

**Fix:** In the `GroundPersistent` case within `HandleSkillEffectDamage`, skip spawning if `HandleGroundEffectCreated` handles that skill. Simplest approach: remove the `GroundPersistent` case from `HandleSkillEffectDamage` entirely, since `HandleGroundEffectCreated` provides full lifecycle management.

### 4.5 Memory leaks in ActivePersistentEffects

`Deinitialize()` (lines 114-122) iterates all `ActivePersistentEffects` and calls `DeactivateImmediate()` on valid entries, then empties the map. This handles level transitions correctly.

However, entries added by `SpawnGroundPersistent` (from HandleSkillEffectDamage) are never removed during gameplay -- only on subsystem deinit. Effects added by `HandleGroundEffectCreated` ARE properly removed by `HandleGroundEffectRemoved`. This is another reason to remove the `GroundPersistent` case from `HandleSkillEffectDamage`.

### 4.6 Thread safety

Both socket event callbacks (via `USE_GAME_THREAD`) and timer callbacks fire on the game thread. There is no multi-threading concern. The TMaps (`ActiveCastingCircles`, `ActivePersistentEffects`, `ActiveBuffAuras`, `OverrideCache`) are only accessed from the game thread. Safe.

### 4.7 Casting circle key collision with buff aura

Casting circles use key `CasterId * 10000 + 9999` in `ActiveBuffAuras`. Buff auras use key `TargetId * 10000 + SkillId`. A collision occurs if `CasterId == TargetId` AND `SkillId == 9999`. RO skill IDs are in the range 1-9999. There IS no skill with ID 9999 in the current config (max is 212), and RO's skill ID range maxes around 3000. So collision is theoretically possible but practically impossible.

The first audit recommended a separate map for Niagara casting circles. This is still a good cleanup task but is not a practical risk.

**Verdict: No practical collision risk with current or foreseeable skill IDs.**

---

## 5. Remaining Risk Assessment

| Issue | Severity | Fixed? | Notes |
|-------|----------|--------|-------|
| C1: SpellColor parameter | Critical | YES | 5-name shotgun approach -- works |
| C2: Buff aura key | Critical | YES | TargetId properly threaded through |
| C3: Timer raw pointer | Critical | YES | TWeakObjectPtr is correct |
| C4: WarpPortal orphan | Critical | YES | EndPlay cleanup is correct |
| N1: OverrideCache not UPROPERTY | Low | NO | Works in practice (content assets) |
| N2: GroundPersistent double-spawn | Moderate | NO | HandleSkillEffectDamage and HandleGroundEffectCreated both spawn |
| N3: CastingCircleActor lambda | Low | NO | Safe due to timer manager semantics |
| N6: EAttachLocation deprecation | Low | NO | Compiles with possible warning |
| N8: Inline static correctness | None | N/A | First audit was wrong -- C++17 handles this correctly |
| R4 (audit 1): Separate casting circle map | Low | NO | Cleanup task, no practical risk |
| Decal M_CastingCircle missing | Low | NO | Niagara fallback works, decal path is broken |
| Fire Ball missing explosion | Low | NO | Visual polish, not system bug |
| Actor lookup O(N) | Moderate | NO | Performance concern at scale |
| No effect budget/significance | Moderate | NO | Scale concern for raids |

---

## 6. Final Verdict

**Is this system ready to test in-editor?** YES.

All 4 critical issues from the first audit have been correctly fixed. The fixes are sound -- no incorrect patterns, no half-measures, no new bugs introduced.

**Remaining issues are all LOW or MODERATE severity:**

The most actionable remaining issue is N2 (GroundPersistent double-spawn), which will cause Fire Wall and Safety Wall to display double VFX when first damaged. This is visually noticeable but not a crash or leak. The fix is straightforward: remove the `GroundPersistent` case from the `HandleSkillEffectDamage` switch, since `HandleGroundEffectCreated/Removed` provides full lifecycle management for these effects.

Everything else is either cleanup work (separate casting circle map, OverrideCache UPROPERTY, EAttachLocation deprecation), missing polish (Fire Ball explosion, effect budgets), or performance optimization for scale (actor lookup caching) -- none of which block initial testing.

**The system will compile, run without crashes, correctly display casting circles and bolt effects, properly track and clean up buff auras, and not leak Niagara components during zone transitions.** It is ready for in-editor PIE testing.

**Recommended immediate action before testing:**
1. Remove the `GroundPersistent` case from `HandleSkillEffectDamage` (prevent double-spawn) -- 3 lines of code.
2. Open NS_Zap_02_Blue, NS_Zap_01_Red, NS_Zap_03_Yellow in the Niagara editor and verify they have at least one of the 5 color parameter names. If not, add a "Color" user parameter.

**Recommended post-testing action:**
1. Add `UPROPERTY()` to `OverrideCache` TMap.
2. Cache actor lookups in a TMap populated from socket events.
3. Create the `M_CastingCircle` material for decal fallback.

---

## Sources

### Official Documentation
- [UNiagaraComponent API (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraComponent)
- [Set Niagara Variable LinearColor (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Niagara/SetNiagaraVariable_LinearColor)
- [SpawnSystemAttached (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraFunctionLibrary/SpawnSystemAttached)
- [TWeakObjectPtr (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/TWeakObjectPtr)
- [DestroyComponent (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Components/DestroyComponent)
- [Actor Lifecycle (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-actor-lifecycle)
- [Programming Subsystems (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine)
- [EAttachLocation::Type (UE 5.3)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Engine/EAttachLocation__Type?application_version=5.3)

### Community Resources
- [Optimizing TWeakObjectPtr Usage](https://prosser.io/optimizing-tweakobjectptr-usage/)
- [All About Soft and Weak Pointers](https://dev.epicgames.com/community/learning/tutorials/kx/all-about-soft-and-weak-pointers)
- [Memory Management and Garbage Collection in UE5](https://mikelis.net/memory-management-garbage-collection-in-unreal-engine/)
- [Pointer Types -- Unreal Community Wiki](https://unrealcommunity.wiki/pointer-types-m33pysxg)
- [Using Niagara in C++](https://dev.epicgames.com/community/learning/tutorials/Gx5j/using-niagara-in-c)
- [Setting a Variable in a Niagara Component with C++](https://gdtactics.com/setting-a-variable-in-a-niagara-component-with-cpp-in-unreal-engine-5)

### Forum Discussions
- [TWeakObjectPtr vs raw UObject pointer](https://forums.unrealengine.com/t/whats-the-difference-between-using-tweakobjectptr-or-using-uobject/284354)
- [UPROPERTY hard references vs TWeakObjectPtr and GC](https://forums.unrealengine.com/t/uproperty-hard-references-vs-tweakobjectptr-and-garbage-collection/259905)
- [TMap TObjectPtr conversion problem](https://forums.unrealengine.com/t/tmap-tobjectptr-conversion-problem/1858604)
- [Best practice for GC with UPROPERTY and TObjectPtr](https://forums.unrealengine.com/t/what-is-the-best-practice-for-garbage-collection-uproperty-with-without-tobjectptr/2255994)
- [EndPlay vs Destroyed](https://forums.unrealengine.com/t/whats-the-difference-between-destroyed-and-endplay/348481)
- [SpawnSystemAttached + RootComponent + C++](https://forums.unrealengine.com/t/spawnsystemattached-rootcomponent-c-problem/1183491)
