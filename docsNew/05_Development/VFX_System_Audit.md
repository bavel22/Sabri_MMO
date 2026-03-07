# VFX System Audit -- Sabri_MMO SkillVFXSubsystem

**Auditor:** Claude Opus 4.6 (automated code review)
**Date:** 2026-03-05
**Scope:** All C++ files in `client/SabriMMO/Source/SabriMMO/VFX/`, plus `WarpPortal.h/.cpp`
**Engine version:** UE 5.7

---

## 1. Executive Summary

**Overall verdict: NOT production-ready. Functional prototype with several critical and many moderate issues.**

The VFX system is a well-structured first pass. The architecture (UWorldSubsystem wrapping socket events, dispatching to parameterized Niagara templates) is fundamentally sound and follows the project's established patterns. The skill-to-template mapping in `SkillVFXData.h` is clean and expandable. All referenced Niagara assets from marketplace packs (Free_Magic, Mixed_Magic, Vefects/Zap, NiagaraExamples) physically exist on disk.

However, there are **4 critical issues** that will cause runtime failures, **8 moderate risks** that will cause subtle bugs or performance problems at scale, and **several missing features** that any MMO VFX system needs before shipping. The most dangerous single issue is that `SetVariableLinearColor("SpellColor")` will silently do nothing on every third-party Niagara system -- because none of them expose a user parameter named "SpellColor."

---

## 2. Architecture Review

### 2.1 Subsystem Pattern -- CORRECT

`UWorldSubsystem` is the right choice for a per-level VFX manager. It matches the project's established pattern (CastBarSubsystem, SkillTreeSubsystem, InventorySubsystem, etc.) and provides:
- Automatic creation per game world
- Proper lifecycle (OnWorldBeginPlay/Deinitialize)
- World-scoped access via `World->GetSubsystem<USkillVFXSubsystem>()`
- No cross-level state bleeding

The alternative -- GameInstanceSubsystem -- would be wrong because VFX state (active circles, persistent effects) is per-level. A standalone manager actor would also work but would add unnecessary actor iteration overhead and break the established subsystem pattern.

**Rating: CORRECT**

### 2.2 Asset Loading -- RISKY

The system uses `LoadObject<UNiagaraSystem>()` synchronously in `OnWorldBeginPlay`. This is acceptable for a small number of assets (8 primary + fallbacks = ~14 calls) during level load, when the player is already seeing a loading screen. `LoadObject` internally calls `FindObject` first, so repeat loads are essentially free lookups.

However, there are two problems:

1. **Per-skill `LoadObject` calls during gameplay** (line 588, 606, 624, 643, 660): Every time `SpawnBoltFromSky`, `SpawnProjectileEffect`, etc. fire, if the skill config has a `NiagaraOverridePath`, it calls `LoadObject` synchronously. For skills like Cold Bolt (which fires 1-10 bolts per cast across all players in the zone), this means 10+ synchronous `LoadObject` calls per cast. While `LoadObject` is fast for already-loaded assets, it still performs a string-based lookup through the asset registry on every call.

2. **No async loading**: The system does not use `FSoftObjectPath`, `TSoftObjectPtr`, or `FStreamableManager` for any asset loading. If an asset is not already in memory (e.g., first use of a per-skill override), `LoadObject` will cause a synchronous load that can hitch the game thread. UE5.5+ provides `FSoftObjectPath::LoadAsync` specifically for this case.

**Rating: RISKY**

### 2.3 Socket Event Wrapping -- CORRECT (with caveats)

The `WrapSingleEvent` pattern is identical to the established pattern in `CastBarSubsystem.cpp` (lines 147-179) and works correctly:
- Saves the existing callback from `EventFunctionMap`
- Replaces it with a combined callback that calls the original first, then the VFX handler
- Uses `ESIOThreadOverrideOption::USE_GAME_THREAD` to ensure UObject access safety

**Caveats:**
- The wrapping order depends on subsystem initialization order. If `SkillVFXSubsystem` wraps before `CastBarSubsystem`, the chain will be `VFX -> original`, not `CastBar -> original -> VFX`. The "gate" check (`EventFunctionMap.Contains("combat:health_update")`) ensures Blueprint events are bound first, but the order between C++ subsystems is not guaranteed.
- All three subsystems (CastBar, SkillTree, SkillVFX) capture `this` in lambdas stored in the socket callback map. If the subsystem is destroyed before the socket is cleaned up, these become dangling references. The project mitigates this by having the subsystem outlive the socket (subsystem lives for the world lifetime), but there is no explicit safety check.

**Rating: CORRECT**

### 2.4 Casting Circle Material -- MISSING

The code references `M_CastingCircle` at path `/Game/SabriMMO/VFX/Materials/M_CastingCircle.M_CastingCircle`. **This file does not exist on disk.** The `Content/SabriMMO/` directory has no `VFX` subfolder. The `CastingCircleActor::Initialize` has a fallback that also loads this same path (line 37), so the decal-based casting circle will always fail to create a material and will be invisible.

In practice, the Niagara-based casting circle (`NS_Free_Magic_Circle1`) will be used instead (since it exists), so this is not a blocker -- but the decal fallback path is completely broken.

**Rating: WRONG (for decal path) / CORRECT (for Niagara fallback)**

---

## 3. Code Review -- Per-File Issues

### 3.1 `SkillVFXSubsystem.h`

| Line | Issue | Severity |
|------|-------|----------|
| 127 | `ActiveCastingCircles` uses `TWeakObjectPtr<ACastingCircleActor>` -- correct, prevents preventing GC | OK |
| 130 | `ActivePersistentEffects` keyed by `int32` -- persistent effects from `HandleGroundEffectCreated` use server `effectId`, but `SpawnGroundPersistent` uses `SkillId * 100000 + counter`. These are two different key spaces in the same map. No collision risk if effectIds < 100000, but fragile. | Moderate |
| 133 | `ActiveBuffAuras` uses `int64` key = `TargetId * 10000 + SkillId`. But in `SpawnSelfBuff` (line 691), the key is hardcoded as `0 * 10000 + SkillId` with a `// TODO: use actual target ID` comment. This means all buff auras for all targets of the same skill overwrite each other. | **Critical** |
| 133 | Casting circles also piggyback on `ActiveBuffAuras` (key = `CasterId * 10000 + 9999`). If a caster has buff SkillId 9999, the keys collide. | Moderate |

### 3.2 `SkillVFXSubsystem.cpp`

| Line | Issue | Severity |
|------|-------|----------|
| 38-41 | `LoadObject` in a lambda inside `OnWorldBeginPlay` is fine for initialization | OK |
| 62-66 | Fallback asset loading -- good defensive pattern | OK |
| 73-78 | Double-check for `NS_CastingCircle` with two paths -- good | OK |
| 174 | Gate check on `combat:health_update` -- matches CastBarSubsystem pattern | OK |
| 298 | `bHasGround && (GX != 0 || GY != 0 || GZ != 0)` -- legitimate position at world origin (0,0,0) will be treated as "no ground position." Unlikely but technically incorrect. | Low |
| 386 | `RemoveCastingCircle(AttackerIdD)` called in `HandleSkillEffectDamage` -- correct, cast is complete when damage arrives | OK |
| 588 | `LoadObject` called per-bolt-hit for override paths -- should cache | Moderate |
| 614-616 | `SetVariableVec3("TargetPosition")` and `SetVariableFloat("ProjectileSpeed")` -- these will only work if the Niagara system has user parameters with exactly these names. Third-party systems from Free_Magic/Mixed_Magic almost certainly do NOT have parameters named "TargetPosition" or "ProjectileSpeed." | **Critical** |
| 691 | `int64 Key = static_cast<int64>(0) * 10000 + SkillId;` -- hardcoded 0 instead of target ID. Every player/enemy that gets the same buff will share one key, causing only the last one to be tracked and the others to leak. | **Critical** |
| 748-756 | Timer lambda captures `CircleComp` as a raw `UNiagaraComponent*`. If the component is garbage collected before the timer fires (e.g., level transition, actor destroyed), this is a dangling pointer dereference. The `!CircleComp->IsBeingDestroyed()` check is insufficient because the pointer itself may be invalid. Should use `TWeakObjectPtr<UNiagaraComponent>`. | **Critical** |
| 826-832 | `SpawnSystemAtLocation` with `bAutoDestroy=true` and `ENCPoolMethod::AutoRelease` -- correct combination for one-shot effects | OK |
| 840-851 | `SpawnSystemAttached` with `ENCPoolMethod::AutoRelease` -- correct for buff auras that should return to pool when detached | OK |
| 856 | `SetNiagaraColor()` now tries 8+ parameter names (SpellColor, Color, BaseColor, ParticleColor, User.Color, User.BaseColor, Tint, ColorMultiplier) + `SetColorParameter`. **FIXED 2026-03-06** | ~~High~~ → OK |
| 870-878 | `FindEnemyActorById` now uses **property reflection** (`FindPropertyByName("EnemyId")`) instead of actor tags. Still O(N) scan but more reliable. **FIXED 2026-03-06** | Moderate (perf) |
| 887-889 | `FindPlayerActorById` for the local player uses `GetFirstPlayerController()` -- correct for single-player-per-client architecture | OK |

### 3.3 `SkillVFXData.h`

| Line | Issue | Severity |
|------|-------|----------|
| 92 | `NiagaraOverridePath` is not a `UPROPERTY` -- this means it cannot be edited in Blueprints or the Details panel. Intentional (code-only config), but limits artist iteration. | Low |
| 117-185 | Static TMap with lazy initialization inside an `inline` function in a namespace -- the `static` variables inside the `inline` function are guaranteed to be initialized exactly once per translation unit in C++11+. Since this header is included in multiple `.cpp` files, each gets its own copy of the static `Configs` map. This wastes memory but is not a correctness bug because each copy is identical. However, in UE5's module system this can cause issues with GENERATED_BODY types in static storage. | Low |
| 117-185 | The skill config table is hardcoded in C++. Adding a new skill requires recompiling. No Data Table, Data Asset, or JSON config support. | Moderate (expandability) |

### 3.4 `CastingCircleActor.h/.cpp`

| Line | Issue | Severity |
|------|-------|----------|
| 36-37 | `static UMaterialInterface* DefaultMat = LoadObject<>()` -- static local inside an actor method. This loads the material once and caches it forever, even across level transitions. The material reference will prevent GC of the material even if the level changes. Not a leak per se (it is intentional caching), but violates UE5 memory management norms. | Low |
| 52-53 | Material parameter names `"ElementColor"` and `"FadeAlpha"` are correct conventions for a custom decal material. However, **the material M_CastingCircle does not exist**, so these parameters are never set. | Moderate |
| 104 | `FadeAlpha += FadeDirection * FadeSpeed * 0.016f` -- assumes the timer fires at exactly 16ms intervals. Timer precision is not guaranteed, especially under load. Should use actual delta time: `World->GetDeltaSeconds()` or `FApp::GetDeltaTime()`. However, for a visual fade this is cosmetic only. | Low |
| 72 | `FTimerDelegate::CreateLambda([this]() { FadeOut(0.3f); })` -- captures `this` (an AActor) in a timer lambda. Safer than a raw UObject pointer capture because `SetTimer` on the actor's world timer manager will automatically clear if the actor is destroyed. | OK |

### 3.5 The "SpellColor" Problem — FIXED (2026-03-06)

~~This was the single most impactful issue in the system.~~ **RESOLVED.**

`SetNiagaraColor()` now tries **8 parameter names** to maximize compatibility with third-party systems:

```cpp
Comp->SetVariableLinearColor(FName("SpellColor"), Color);
Comp->SetVariableLinearColor(FName("Color"), Color);
Comp->SetVariableLinearColor(FName("BaseColor"), Color);
Comp->SetVariableLinearColor(FName("ParticleColor"), Color);
Comp->SetVariableLinearColor(FName("User.Color"), Color);
Comp->SetVariableLinearColor(FName("User.BaseColor"), Color);
Comp->SetVariableLinearColor(FName("Tint"), Color);
Comp->SetVariableLinearColor(FName("ColorMultiplier"), Color);
Comp->SetColorParameter(FName("Color"), Color);  // Legacy Cascade-style
```

**Rating: FIXED** — Most third-party systems expose at least one of these names. Still won't work on systems that only expose color through material instances, but covers the vast majority of cases.

### 3.6 `WarpPortal.h/.cpp`

| Line | Issue | Severity |
|------|-------|----------|
| 23 | `TriggerRadius` used in constructor AND in BeginPlay -- the constructor uses `TriggerRadius` before property values are loaded from the serialized actor, so the radius is always the default 200.f initially. BeginPlay correctly re-sets it. Minor timing issue. | OK (handled) |
| 49-56 | Deferred VFX init with 1-second polling timer, max 10 retries -- reasonable pattern for waiting on subsystem asset loading | OK |
| 59 | `TWeakObjectPtr<UNiagaraComponent> SpawnedPortalVFX` -- stored but never cleaned up. If the WarpPortal actor is destroyed, the Niagara component spawned by `SpawnLoopingPortalEffect` was created with `bAutoDestroy=false` and `ENCPoolMethod::None`, so it will persist in the world as an orphan forever. | Moderate |
| 75 | `VFXSub->SpawnLoopingPortalEffect(GetActorLocation())` spawns a free-floating Niagara component not attached to the WarpPortal actor. If the WarpPortal moves (unlikely but possible in editor), the VFX stays at the old location. | Low |
| 94 | `World->GetFirstPlayerController()` -- correct per project convention (single local player per client) | OK |

---

## 4. Performance Analysis

### 4.1 Bottlenecks

| Area | Concern | Impact |
|------|---------|--------|
| `FindEnemyActorById` / `FindPlayerActorById` | O(N) `TActorIterator` scan of ALL actors per call. Called 2x per damage event (attacker + target). With 20 enemies and 10 players all casting simultaneously, this is hundreds of full-world scans per second. | **High at scale** |
| Per-skill `LoadObject` | Called on every bolt hit, every projectile spawn, etc. if the config has `NiagaraOverridePath`. With multi-hit bolt skills (10 hits), this is 10 `LoadObject` calls per cast. | **Moderate** (mitigated by UE's internal FindObject cache) |
| `SkillVFXDataHelper::GetSkillVFXConfig` | `static TMap` lookup -- O(1) amortized. First call initializes all configs. | OK |
| Niagara pooling | `ENCPoolMethod::AutoRelease` is correct for one-shot effects. Pool will grow to match peak concurrent effects, then stabilize. No explicit pool priming, so first uses may allocate. | Low |
| Timer-based fade in CastingCircleActor | 60 Hz timer for alpha interpolation. Acceptable for a few circles, but if 20 players are casting simultaneously, that is 20 actors each with a 60 Hz timer. | Low-Moderate |

### 4.2 Scaling Concerns

The system has no concept of:
- **Effect budgets / significance**: No Niagara Effect Type asset is configured. In a 20-player raid, every player's every bolt gets full VFX. There is no distance-based culling, no max-instance cap, no LOD scaling.
- **Throttling**: No rate limiting on VFX spawns. A Thunderstorm skill hitting 10 enemies with 5 hits each spawns 50 lightning effects within 1 second.
- **View distance culling**: The `bPreCullCheck=true` parameter in `SpawnSystemAtLocation` helps, but there is no explicit distance check before spawning.

### 4.3 Recommended Performance Fixes

1. Cache actor lookups in a `TMap<int32, TWeakObjectPtr<AActor>>` populated on enemy:spawned / player:joined events. Invalidate on death/despawn.
2. Cache `LoadObject` results for per-skill overrides in a `TMap<FString, TObjectPtr<UNiagaraSystem>>`.
3. Configure Niagara Effect Types with significance handlers for distance-based culling.
4. Add a max concurrent VFX count per template type (e.g., max 8 simultaneous bolt effects).

---

## 5. Multiplayer Safety

### 5.1 PIE Testing

The system follows the project's multiplayer-safe coding rules:
- **No `GEngine->GameViewport`** -- correct, uses `GetWorld()` throughout
- **No `GEngine->AddOnScreenDebugMessage`** -- correct, uses `UE_LOG`
- **World-scoped subsystem access** -- `World->GetSubsystem<>()` is per-PIE-world

### 5.2 Potential Issues

| Area | Assessment |
|------|------------|
| `GetFirstPlayerController()` | Used in `FindPlayerActorById` and `WarpPortal`. In PIE with 2+ clients, each PIE world has its own `GetFirstPlayerController()` that returns the correct local player. **Safe.** |
| Static data in `SkillVFXDataHelper` | Static `TMap<int32, FSkillVFXConfig> Configs` is shared across all PIE instances in the same process. Since it is read-only after initialization, this is safe. |
| `LocalCharacterId` | Set per-subsystem instance from `MMOGameInstance`. Each PIE world gets its own subsystem and game instance. **Safe.** |
| Timer captures | Timer lambdas capture `this` (the subsystem) or raw component pointers. Since timers are registered on `World->GetTimerManager()` which is per-PIE-world, they will not cross-fire. **Safe** (except for the raw pointer capture issue noted in Section 3.2). |

**Rating: CORRECT** (with the raw pointer timer lambda caveat)

---

## 6. Comparison to Industry Standard

### 6.1 How Real UE5 Games Handle VFX

| Feature | Industry Standard | This System |
|---------|-------------------|-------------|
| VFX Data Definition | Data Assets / Data Tables (editable in editor without recompile) | Hardcoded C++ static TMap |
| Asset Loading | `TSoftObjectPtr` + `FStreamableManager` async loading with callbacks | Synchronous `LoadObject` |
| Color Parameterization | Custom Niagara systems with exposed User Parameters, or Material Parameter Collections | Calls `SetVariableLinearColor("SpellColor")` which does not match any third-party system |
| Effect Pooling | Niagara Effect Types with significance handlers, budget limits, distance culling | `ENCPoolMethod::AutoRelease` only (no budgets) |
| Actor Lookup | Manager pattern with registered maps (BP_EnemyManager has a TMap) | `TActorIterator` brute-force scan |
| Socket/Network Events | GameplayAbility System with GameplayCues, or Multicast RPCs | Socket.io event wrapping (appropriate for this project's architecture) |
| Casting Indicators | Decal + Niagara with proper materials | Decal material missing; Niagara fallback works |
| Buff Aura Lifecycle | Component-based with proper attach/detach lifecycle | Attached Niagara with broken key tracking |

### 6.2 How This Compares to Ragnarok Online Recreations

There is at least one known "Ragnarok Remastered" UE5 project. The general approach of mapping RO skill IDs to VFX templates is correct and matches how other RO recreations work. The element-color system (fire = red, water = blue, wind = yellow, etc.) is authentic to RO.

The socket-event-driven approach (server broadcasts damage events, client spawns VFX) is appropriate for this project's non-standard networking stack (Socket.io instead of UE5 replication). In a standard UE5 multiplayer game, you would use Multicast RPCs or GameplayCues for cosmetic VFX, but since this project uses Socket.io for all server communication, wrapping socket events is the correct equivalent pattern.

---

## 7. Critical Issues (MUST FIX)

### C1. "SpellColor" Parameter Does Not Exist on Third-Party Systems

**File:** `SkillVFXSubsystem.cpp` line 856
**Impact:** All color customization is silently ignored. Every skill looks the same regardless of element.
**Fix:** Open each Niagara system in the editor and add a User Parameter named "SpellColor" of type Linear Color, wired into the particle color or material. Alternatively, create custom wrapper systems.

### C2. Buff Aura Key Hardcoded to 0

**File:** `SkillVFXSubsystem.cpp` line 691
**Impact:** Only one buff aura per skill ID is tracked globally. If two players both have Endure active, only the second one's aura is tracked. When the first player's buff expires, `HandleBuffRemoved` cannot find or remove their aura (key mismatch), causing the Niagara component to leak and loop forever.
**Fix:** Pass the actual target ID to the key calculation:
```cpp
int64 Key = static_cast<int64>(TargetId) * 10000 + SkillId;
```
This requires passing `TargetId` through to `SpawnSelfBuff`.

### C3. Raw Pointer Captured in Timer Lambda

**File:** `SkillVFXSubsystem.cpp` lines 748-756
**Impact:** If the Niagara component is garbage collected before the timer fires (level transition, zone change, heavy GC pressure), the lambda dereferences a dangling pointer causing a crash.
**Fix:**
```cpp
TWeakObjectPtr<UNiagaraComponent> WeakComp = CircleComp;
FTimerHandle TempHandle;
World->GetTimerManager().SetTimer(TempHandle,
    [WeakComp]()
    {
        if (WeakComp.IsValid())
        {
            WeakComp->DeactivateImmediate();
        }
    },
    CastDuration + 0.1f, false);
```

### C4. Orphaned Niagara Components from WarpPortal

**File:** `WarpPortal.cpp` line 75, `SkillVFXSubsystem.cpp` lines 970-976
**Impact:** `SpawnLoopingPortalEffect` creates a Niagara component with `bAutoDestroy=false` and `ENCPoolMethod::None`, spawned as a free-floating world component (not attached to the WarpPortal). When the WarpPortal is destroyed (level unload, zone transition), the Niagara component persists as an invisible orphan, leaking GPU resources. The `SpawnedPortalVFX` TWeakObjectPtr tracks it but `AWarpPortal::EndPlay` / destructor never deactivates or destroys it.
**Fix:** Add cleanup in WarpPortal:
```cpp
void AWarpPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SpawnedPortalVFX.IsValid())
    {
        SpawnedPortalVFX->DeactivateImmediate();
        SpawnedPortalVFX->DestroyComponent();
    }
    Super::EndPlay(EndPlayReason);
}
```

---

## 8. Recommended Improvements (Should Fix)

### R1. Cache Actor Lookups

Replace `TActorIterator` scans with a `TMap<int32, TWeakObjectPtr<AActor>>` cache, populated from enemy:spawned and player:joined socket events. This changes O(N * calls_per_frame) into O(1) lookups.

### R2. Cache Per-Skill LoadObject Results

Add a `TMap<FString, TObjectPtr<UNiagaraSystem>> OverrideCache` member. On first load, cache the result:
```cpp
UNiagaraSystem* GetOrLoadOverride(const FString& Path)
{
    if (auto* Found = OverrideCache.Find(Path)) return *Found;
    UNiagaraSystem* Loaded = LoadObject<UNiagaraSystem>(nullptr, *Path);
    OverrideCache.Add(Path, Loaded);
    return Loaded;
}
```

### R3. Move Skill VFX Configs to Data Asset

Replace the hardcoded `SkillVFXDataHelper` static TMap with a `UDataAsset` or `UDataTable` that can be edited in the UE5 editor without recompiling. This lets artists tune colors, scales, and template assignments without touching C++.

### R4. Separate Casting Circle Tracking from Buff Auras

The `ActiveBuffAuras` map is overloaded -- it tracks both buff auras AND Niagara-based casting circles (key = CasterId * 10000 + 9999). Create a separate `TMap<int32, TWeakObjectPtr<UNiagaraComponent>> ActiveNiagaraCastingCircles` to avoid key collisions and improve clarity.

### R5. Add Niagara Effect Type Configuration

Create a Niagara Effect Type asset with:
- Max system instance count (e.g., 20 for bolts, 5 for ground persistent)
- Distance-based significance handler (cull effects far from camera)
- Budget scaling to reduce particle counts under load
- Pool priming for commonly used effects

### R6. Create the Missing M_CastingCircle Material

If decal-based casting circles are desired as a fallback, create:
- `Content/SabriMMO/VFX/Materials/M_CastingCircle` -- Deferred Decal material
- Material Domain: Deferred Decal
- Blend Mode: Translucent
- Parameters: `ElementColor` (Vector), `FadeAlpha` (Scalar)
- A circular texture with alpha mask

### R7. Fix CastingCircleActor Fade Timing

Use actual delta time instead of hardcoded 0.016f:
```cpp
void ACastingCircleActor::TickFade()
{
    float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
    FadeAlpha += FadeDirection * FadeSpeed * DeltaTime;
    // ...
}
```

Note: Since `TickFade` is called from a timer at a fixed interval rather than from `Tick()`, `GetDeltaSeconds()` gives the frame delta, not the timer interval. For a timer-based approach, the timer interval itself (0.016f) is technically more accurate. Consider switching to actor tick instead.

### R8. Attach Portal VFX to WarpPortal Actor

Change `SpawnLoopingPortalEffect` to attach the Niagara component to the calling actor's root component, rather than spawning it as a free-floating world component. Alternatively, have WarpPortal use `SpawnSystemAttached` directly.

---

## 9. Missing Features

### M1. Sound Effects

No audio system is integrated. In RO and most MMOs, every skill has an associated sound effect (lightning crack, fire whoosh, heal chime). The VFX system should either integrate sound cues or coordinate with a parallel SFX subsystem.

### M2. Hit Impact Effects

There is no separate impact/hit effect when a projectile reaches its target. The `Projectile` template spawns the projectile but has no on-impact burst. Real implementations spawn a second one-shot effect at the target location on projectile arrival.

### M3. Screen Shake / Camera Effects

No camera shake on large AoE impacts (Thunderstorm, Magnum Break). No screen flash on critical hits. These are standard "juice" effects in MMO combat.

### M4. Effect Queueing / Stacking

When multiple bolt hits arrive in rapid succession (200ms apart), each spawns an independent effect with no awareness of the others. There is no stacking offset, no intensity scaling, no visual differentiation between bolt #1 and bolt #5.

### M5. Death / Despawn VFX

No death effect for enemies or players. When an enemy dies, it presumably just disappears. A death puff, dissolve, or collapse animation is standard.

### M6. Damage Type Visual Feedback

Beyond color (which does not work currently), there is no visual differentiation for miss, critical hit, elemental advantage, or status effect application. These could be Niagara parameter variations or entirely different systems.

### M7. Level-of-Detail for VFX

No LOD system for distant effects. A Thunderstorm cast by a player 5000 UE units away should use fewer particles or be represented by a simple flash.

### M8. Effect Warm-Up for Late Joiners

When a player joins a zone, they receive no information about existing active effects (ground effects, buff auras, casting circles). If a Fire Wall was placed 5 seconds ago, the new player will not see it until it is refreshed or removed.

---

## 10. Expandability Assessment

### Adding a New Skill

**Effort: Low (1 line of code)**

Adding a new skill VFX is a single `Add()` call in `SkillVFXDataHelper::GetSkillVFXConfig`:
```cpp
Add(213, ESkillVFXTemplate::BoltFromSky, FLinearColor::Red, TEXT("fire"), true);
```

This maps cleanly to the 8 template types. The template system is well-designed for this purpose.

**Limitation:** Requires a C++ recompile. Moving to a Data Asset would make this editor-only.

### Adding a New Template Type

**Effort: Moderate (new enum value + new spawn function + new handler logic)**

1. Add to `ESkillVFXTemplate` enum
2. Add a new `NS_*` UPROPERTY and load it in `OnWorldBeginPlay`
3. Add a new `Spawn*` method
4. Add a case to the switch in `HandleSkillEffectDamage`

The pattern is clear and consistent. Approximately 30-50 lines of code per new template.

### Adding a New Niagara System

**Effort: Low (per-skill override path)**

The `NiagaraOverridePath` field already supports per-skill Niagara system overrides. Adding a new visual style for a single skill only requires setting the path in the config.

### Overall Expandability

**Rating: GOOD** -- The template-based architecture with per-skill config overrides is well-suited for an MMO with hundreds of skills. The main friction point is the C++ hardcoding vs. Data Assets.

---

## 11. File-Copied Assets Risk

### Assets Present on Disk

All referenced third-party Niagara systems exist in the Content directory:

| Pack | Path | Status |
|------|------|--------|
| Free_Magic | `Content/Free_Magic/VFX_Niagara/NS_Free_Magic_*.uasset` | 17 files present |
| Mixed_Magic_VFX_Pack | `Content/Mixed_Magic_VFX_Pack/VFX/NS_*.uasset` | 27+ files present |
| Vefects/Zap_VFX | `Content/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_*.uasset` | 7 NS files present |
| NiagaraExamples | `Content/NiagaraExamples/FX_Player/NS_Player_*.uasset` | 7 files present |

### Risk Assessment

**Were these migrated correctly?** The directory structures appear complete (Materials, Textures, Meshes alongside the Niagara systems). This suggests the Migrate tool was used rather than raw file copy, because:
1. Each pack has its supporting assets (materials, textures, meshes) in subdirectories
2. The directory structure matches what Migrate produces
3. No broken reference errors have been reported in the build

**Remaining risks:**
1. **Engine version mismatch**: If these packs were created for UE 4.x or UE 5.0-5.3, some Niagara modules may be deprecated in UE 5.7. The Niagara editor would show warnings or errors when opening these assets.
2. **Material references**: Niagara systems reference materials which reference textures. If any texture or material was not migrated, the system will render with default pink/checkered materials.
3. **Plugin dependencies**: Some marketplace packs require specific plugins. The Free_Magic and Mixed_Magic packs appear to be Niagara-only (no custom plugins).

**Recommendation:** Open each Niagara system in the UE5 editor and check the Message Log for asset reference warnings. Run PIE and visually verify each effect spawns correctly.

**Rating: RISKY but likely fine** -- the directory structure suggests proper migration, but UE version compatibility is unverified.

---

## 12. Summary Ratings Table

| Aspect | Rating |
|--------|--------|
| Architecture (WorldSubsystem) | CORRECT |
| Asset Loading (OnWorldBeginPlay) | RISKY |
| Asset Loading (per-skill override) | RISKY |
| Socket Event Wrapping | CORRECT |
| Niagara Spawning (SpawnSystemAtLocation) | CORRECT |
| Niagara Spawning (SpawnSystemAttached) | CORRECT |
| Niagara Parameters ("SpellColor") | WRONG |
| Component Pooling (AutoRelease) | CORRECT |
| Casting Circle (Niagara path) | CORRECT |
| Casting Circle (Decal fallback) | WRONG (material missing) |
| Persistent Effects (Fire Wall etc.) | CORRECT |
| Buff Auras (key tracking) | WRONG (hardcoded key) |
| WarpPortal VFX | RISKY (orphan leak) |
| File-Copied Assets | RISKY (unverified compatibility) |
| Memory Management | RISKY (raw pointer in timer) |
| Performance at Scale | MISSING (no budgets, O(N) lookups) |
| Multiplayer Safety | CORRECT |
| Expandability | CORRECT |
| Error Handling | CORRECT (graceful null checks) |
| Sound Integration | MISSING |
| Combat Juice (shake, flash) | MISSING |

---

## 13. Sources

### Official Documentation
- [UNiagaraFunctionLibrary::SpawnSystemAtLocation (UE 5.6)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraFunctionLibrary/SpawnSystemAtLocation)
- [Spawn System at Location (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Niagara/SpawnSystematLocation)
- [Scalability and Best Practices for Niagara (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/scalability-and-best-practices-for-niagara)
- [Performance Budgeting Using Effect Types (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/performance-budgeting-using-effect-types-in-niagara-for-unreal-engine)
- [Programming Subsystems (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine)
- [Asynchronous Asset Loading (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/asynchronous-asset-loading-in-unreal-engine)
- [Referencing Assets (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/referencing-assets-in-unreal-engine)
- [Decal Materials (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine)
- [Migrating Assets (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/migrating-assets-in-unreal-engine)
- [Set Niagara Variable LinearColor (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Niagara/SetNiagaraVariable_LinearColor)
- [Controlling Your Niagara Systems (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/controlling-your-niagara-systems)
- [UNiagaraComponentPool (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/Niagara/UNiagaraComponentPool)
- [Spawn System Attached (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Niagara/SpawnSystemAttached)
- [Niagara Debugger (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/niagara-debugger-for-unreal-engine)
- [ENCPoolMethod (UE 4.27 -- still valid)](https://docs.unrealengine.com/4.27/en-US/API/Plugins/Niagara/ENCPoolMethod/)
- [FSoftObjectPath::LoadAsync (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/CoreUObject/FSoftObjectPath/LoadAsync)
- [Set Auto Destroy -- Niagara (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Niagara/SetAutoDestroy)

### Community Tutorials and Articles
- [Using Niagara in C++ -- Community Tutorial](https://dev.epicgames.com/community/learning/tutorials/Gx5j/using-niagara-in-c)
- [Optimizing Niagara -- Systems as a Service](https://dev.epicgames.com/community/learning/tutorials/0q6O/unreal-engine-optimizing-niagara-systems-as-a-service)
- [Optimizing Niagara -- Scalability and Best Practices](https://dev.epicgames.com/community/learning/tutorials/15PL/unreal-engine-optimizing-niagara-scalability-and-best-practices)
- [Niagara Scalability: Effect Types -- Knowledge Base](https://dev.epicgames.com/community/learning/knowledge-base/LJnb/unreal-engine-niagara-scalability-effect-types)
- [Programming Subsystems -- Extra Community Info](https://dev.epicgames.com/community/learning/tutorials/XvMJ/unreal-engine-programming-subsystems-extra-community-info)
- [Setting a Variable in a Niagara Component with C++ -- Game Dev Tactics](https://gdtactics.com/setting-a-variable-in-a-niagara-component-with-cpp-in-unreal-engine-5)
- [Adding User Parameters to Niagara -- Beyond FX](https://blog.beyond-fx.com/articles/real-time-vfx-quick-tip-adding-user-parameters-to-niagara-beyond-fx)
- [Simple Light Aura VFX in UE5 Niagara -- RealTimeVFX.in](https://www.realtimevfx.in/2026/01/simple-light-aura-vfx-in-ue5-niagara.html)
- [Asset Manager for Data Assets and Async Loading -- Tom Looman](https://tomlooman.com/unreal-engine-asset-manager-async-loading/)
- [Hard References and Reasons to Avoid Them -- raharudev](https://raharuu.github.io/unreal/hard-references-reasons-avoid/)
- [Unreal Engine C++ Complete Guide -- Tom Looman](https://tomlooman.com/unreal-engine-cpp-guide/)
- [Soft vs Hard References in Unreal Engine -- Outscal](https://outscal.com/blog/unreal-engine-soft-hard-references-async-loading)
- [Hard vs Soft References -- Quod Soler](https://www.quodsoler.com/blog/understanding-hard-references-and-soft-references-in-unreal-engine)

### Forum Discussions
- [Niagara Effect Not Appearing in Level](https://forums.unrealengine.com/t/niagara-effect-not-appearing-in-level/1196425)
- [Niagara Does Not Play When Running the Game](https://forums.unrealengine.com/t/niagara-does-not-play-when-running-the-game/234975)
- [Niagara Is Auto Destroyed When the Actor Is Destroyed](https://forums.unrealengine.com/t/niagara-is-auto-destroyed-when-the-actor-is-destroyed/1693408)
- [Niagara System Blueprint Deactivate/Destroy](https://forums.unrealengine.com/t/niagara-system-blueprint-deactivate-destroy/442976)
- [Any Infos About the Niagara Pooling System](https://forums.unrealengine.com/t/any-infos-about-the-niagara-pooling-system/145712)
- [Niagara User Parameter Not Visible in Blueprint (UE5.2)](https://forums.unrealengine.com/t/niagara-user-parameter-is-not-visible-in-blueprint-ue5-2-c/1179343)
- [How to Change Niagara User Defined Variable in C++](https://forums.unrealengine.com/t/how-to-change-niagara-user-defined-variable-in-c/1239669)
- [Does a UObject Captured in Lambda by Value Get Retained?](https://forums.unrealengine.com/t/does-a-uobject-captured-in-lambda-by-value-get-retained/598346)
- [TActorIterator Efficiency](https://forums.unrealengine.com/t/tactoriterator-efficiency/295711)
- [Decal Dynamic Material Instance](https://forums.unrealengine.com/t/decal-dynamic-material-instance/350279)
- [SpawnSystemAttached + RootComponent + C++ = Problem](https://forums.unrealengine.com/t/spawnsystemattached-rootcomponent-c-problem/1183491)
- [Optimizing Dozens of Niagara Systems](https://forums.unrealengine.com/t/optimizing-dozens-of-niagara-systems/1795471)
- [FX Spawning Practices -- Real Time VFX](https://realtimevfx.com/t/fx-spawning-practices/7666)
- [Is Anyone Recreating a Map of Ragnarok in UE?](https://forums.unrealengine.com/t/is-anyone-recreating-a-map-of-ragnarok-in-the-ue/502109)
- [Best Practices for Optimizing Niagara VFX](https://forums.unrealengine.com/t/best-practices-for-optimizing-niagara-vfx-for-performance-in-ue5/2656339)
- [Location Issues with Niagara in UE5.5](https://forums.unrealengine.com/t/location-issues-with-the-niagara-effect-and-changes-to-the-unreal-engine-5-5-coordinate-system/2533775)
- [I Found Out You Need to Migrate, Not Export](https://forums.unrealengine.com/t/i-found-out-that-in-order-to-move-copy-blueprint-assets-from-one-project-to-another-you-need-to-migrate-not-export/548234)

### Memory Management and Safety
- [Memory Management and Garbage Collection in UE5 -- Mikelis](https://mikelis.net/memory-management-garbage-collection-in-unreal-engine/)
- [Memory Management -- Unreal Community Wiki](https://unrealcommunity.wiki/memory-management-6rlf3v4i)
- [Garbage Collection -- Unreal Community Wiki](https://unrealcommunity.wiki/garbage-collection-36d1da)
- [Pointer Types -- Unreal Community Wiki](https://unrealcommunity.wiki/pointer-types-m33pysxg)
- [Using C++ Timers in Unreal Engine -- Christina Charlier](https://chrispi.netlify.app/posts/timer-in-unreal/)
- [Understanding Garbage Collection in UE5 C++ -- slicker.me](https://slicker.me/unreal/garbage-collection.htm)

### Multiplayer and Networking
- [Unreal Engine Multiplayer Tips and Tricks -- WizardCell](https://wizardcell.com/unreal/multiplayer-tips-and-tricks/)
- [GASDocumentation -- GameplayAbilitySystem Guide](https://github.com/tranek/GASDocumentation)
- [Is It Possible to Spawn a Client-Side Only Actor?](https://forums.unrealengine.com/t/is-it-possible-to-spawn-in-an-actor-that-only-exists-on-the-client-side/519428)
- [SocketIOClient-Unreal Plugin -- GitHub](https://github.com/getnamo/SocketIOClient-Unreal)
