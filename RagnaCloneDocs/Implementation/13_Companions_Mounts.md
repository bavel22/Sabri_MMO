# 13 -- Pets, Homunculus, Mounts, Falcon & Cart: UE5 C++ Implementation Guide

> Complete implementation guide for companion, mount, falcon, and cart systems in Sabri_MMO.
> All logic is server-authoritative. The client is presentation + input only.
> Stack: UE5.7 (C++ Slate) | Node.js + Express + Socket.io | PostgreSQL

---

## Table of Contents

1. [Pet System (Server)](#1-pet-system-server)
2. [Pet System (Client)](#2-pet-system-client)
3. [Homunculus System (Server)](#3-homunculus-system-server)
4. [Homunculus System (Client)](#4-homunculus-system-client)
5. [Mount System](#5-mount-system)
6. [Falcon System (Hunter)](#6-falcon-system-hunter)
7. [Cart System (Merchant)](#7-cart-system-merchant)
8. [DB Schema & Migrations](#8-db-schema--migrations)

---

## 1. Pet System (Server)

### 1.1 Data File: `server/src/ro_pet_data.js`

Create a new data module alongside existing `ro_monster_templates.js`, `ro_item_mapping.js`, etc.

```javascript
// server/src/ro_pet_data.js
// Pet definitions: taming items, food, bonuses, hunger rates
// Sourced from iRO Wiki Classic / rAthena

const PET_DATA = {
    1002: { // Poring
        monsterId: 1002,
        name: 'Poring',
        eggItemId: 9001,            // Pet Egg item ID in items table
        tamingItem: 619,            // Unripe Apple
        food: 531,                  // Apple Juice
        accessoryItemId: 10013,     // Backpack
        hungerRate: 3,              // Points lost per minute
        tamingBaseChance: 20,       // Base % chance (0-100)
        intimacyGainPerFeed: {
            veryHungry: 2,          // hunger 1-10
            hungry: 5,              // hunger 11-25 (optimal)
            neutral: 3,             // hunger 26-75
            satisfied: 0,           // hunger 76-90
            stuffed: -5             // hunger 91-100
        },
        bonusCordial: { luk: 1, crit: 1 },
        bonusLoyal:   { luk: 2, crit: 1 }
    },
    1113: { // Drops
        monsterId: 1113,
        name: 'Drops',
        eggItemId: 9002,
        tamingItem: 508,            // Orange Juice
        food: 508,                  // Yellow Herb (reused ID example)
        accessoryItemId: 10013,
        hungerRate: 4,
        tamingBaseChance: 15,
        intimacyGainPerFeed: {
            veryHungry: 2, hungry: 5, neutral: 3, satisfied: 0, stuffed: -5
        },
        bonusCordial: { hit: 2, atk: 2 },
        bonusLoyal:   { hit: 3, atk: 3 }
    },
    // ... ~34 total entries following the same structure
    // See RagnaCloneDocs/12_Pets_Homunculus_Companions.md Section 1.10 for full list
};

module.exports = { PET_DATA };
```

### 1.2 Taming (Capture) Logic

Add to `server/src/index.js` in the socket event handler section:

```javascript
// ============================================================
// PET EVENTS
// ============================================================
const { PET_DATA } = require('./ro_pet_data');

// In-memory map of currently summoned pets per character
// Key: characterId, Value: { petId, monsterId, name, intimacy, hunger, accessoryId,
//   x, y, z, hungerTimer, starvationTimer, overfeedCount }
const activePets = new Map();

socket.on('pet:tame', async ({ targetEnemyId, tamingItemId }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    // 1. Validate the taming item exists in player inventory
    const invResult = await pool.query(
        `SELECT * FROM character_inventory
         WHERE character_id = $1 AND item_id = $2 AND quantity > 0
         LIMIT 1`,
        [player.characterId, tamingItemId]
    );
    if (invResult.rows.length === 0) {
        return socket.emit('pet:tame_result', {
            success: false, reason: 'missing_taming_item'
        });
    }

    // 2. Find the enemy instance and validate it is tameable
    const enemy = activeEnemies.get(targetEnemyId);
    if (!enemy || enemy.isDead) {
        return socket.emit('pet:tame_result', {
            success: false, reason: 'invalid_target'
        });
    }

    const petDef = Object.values(PET_DATA).find(
        p => p.monsterId === enemy.monsterId && p.tamingItem === tamingItemId
    );
    if (!petDef) {
        return socket.emit('pet:tame_result', {
            success: false, reason: 'wrong_taming_item'
        });
    }

    // 3. Consume the taming item regardless of outcome
    await pool.query(
        `UPDATE character_inventory
         SET quantity = quantity - 1
         WHERE id = $1`,
        [invResult.rows[0].id]
    );
    // Clean up zero-quantity rows
    await pool.query(
        `DELETE FROM character_inventory WHERE id = $1 AND quantity <= 0`,
        [invResult.rows[0].id]
    );

    // 4. Calculate capture chance (rAthena refined formula)
    const monsterHPPercent = Math.floor((enemy.hp / enemy.maxHp) * 100);
    const captureChance = Math.floor(
        (petDef.tamingBaseChance
            + (player.baseLevel - enemy.level) * 30
            + player.luk * 20
        ) * (200 - monsterHPPercent) / 100
    );
    // Clamp to 1-100 (always at least 1% chance, never above 100%)
    const finalChance = Math.max(1, Math.min(100, captureChance));
    const roll = Math.floor(Math.random() * 100) + 1;
    const success = roll <= finalChance;

    if (!success) {
        return socket.emit('pet:tame_result', { success: false, reason: 'failed' });
    }

    // 5. Remove the enemy from the world
    activeEnemies.delete(targetEnemyId);
    broadcastToZone(player.zone, 'enemy:death', {
        enemyId: targetEnemyId, killerId: player.characterId
    });

    // 6. Create the pet record in DB
    const initialIntimacy = 100 + Math.floor(Math.random() * 300); // 100-399
    const petName = petDef.name; // Default name = monster name

    const petInsert = await pool.query(
        `INSERT INTO character_pets
            (character_id, monster_id, pet_name, intimacy, hunger, is_summoned)
         VALUES ($1, $2, $3, $4, 50, FALSE)
         RETURNING pet_id`,
        [player.characterId, petDef.monsterId, petName, initialIntimacy]
    );
    const petId = petInsert.rows[0].pet_id;

    // 7. Add Pet Egg item to inventory
    const eggInsert = await pool.query(
        `INSERT INTO character_inventory
            (character_id, item_id, quantity, pet_id)
         VALUES ($1, $2, 1, $3)
         RETURNING id`,
        [player.characterId, petDef.eggItemId, petId]
    );

    // 8. Update pet record with egg inventory reference
    await pool.query(
        `UPDATE character_pets SET egg_item_id = $1 WHERE pet_id = $2`,
        [eggInsert.rows[0].id, petId]
    );

    socket.emit('pet:tame_result', {
        success: true,
        petId,
        eggItemId: eggInsert.rows[0].id,
        petName,
        monsterId: petDef.monsterId
    });

    // Refresh inventory on client
    emitInventoryData(socket, player.characterId);
});
```

### 1.3 Summoning and Returning

```javascript
socket.on('pet:summon', async ({ petId }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    // Prevent summoning if a pet is already active
    if (activePets.has(player.characterId)) {
        return socket.emit('pet:error', { reason: 'pet_already_summoned' });
    }

    // Load pet from DB and verify ownership
    const petResult = await pool.query(
        `SELECT * FROM character_pets
         WHERE pet_id = $1 AND character_id = $2`,
        [petId, player.characterId]
    );
    if (petResult.rows.length === 0) {
        return socket.emit('pet:error', { reason: 'pet_not_found' });
    }

    const pet = petResult.rows[0];
    const petDef = PET_DATA[pet.monster_id];

    // Mark as summoned in DB
    await pool.query(
        `UPDATE character_pets SET is_summoned = TRUE WHERE pet_id = $1`,
        [petId]
    );

    // Create in-memory pet state
    const petState = {
        petId: pet.pet_id,
        monsterId: pet.monster_id,
        name: pet.pet_name,
        intimacy: pet.intimacy,
        hunger: pet.hunger,
        accessoryId: pet.accessory_id,
        x: player.lastX + 150,  // Spawn offset behind player
        y: player.lastY + 150,
        z: player.lastZ,
        overfeedCount: 0,
        hungerTimer: null,
        starvationTimer: null
    };

    activePets.set(player.characterId, petState);

    // Start hunger decay timer (once per 60 seconds)
    petState.hungerTimer = setInterval(() => {
        tickPetHunger(player.characterId);
    }, 60000);

    // Notify owner
    socket.emit('pet:summoned', {
        petId: petState.petId,
        monsterId: petState.monsterId,
        name: petState.name,
        intimacy: petState.intimacy,
        hunger: petState.hunger,
        accessoryId: petState.accessoryId,
        x: petState.x, y: petState.y, z: petState.z
    });

    // Notify other players in zone
    broadcastToZoneExcept(player.zone, socket.id, 'pet:other_summoned', {
        ownerId: player.characterId,
        petId: petState.petId,
        monsterId: petState.monsterId,
        name: petState.name,
        accessoryId: petState.accessoryId,
        x: petState.x, y: petState.y, z: petState.z
    });

    // Apply intimacy bonuses if applicable
    applyPetBonuses(player, petState);
});

socket.on('pet:return', async ({ petId }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const petState = activePets.get(player.characterId);
    if (!petState || petState.petId !== petId) {
        return socket.emit('pet:error', { reason: 'pet_not_active' });
    }

    // Persist state to DB
    await pool.query(
        `UPDATE character_pets
         SET intimacy = $1, hunger = $2, is_summoned = FALSE
         WHERE pet_id = $3`,
        [petState.intimacy, petState.hunger, petId]
    );

    // Clear timers
    if (petState.hungerTimer) clearInterval(petState.hungerTimer);
    if (petState.starvationTimer) clearInterval(petState.starvationTimer);

    // Remove bonuses from owner
    removePetBonuses(player, petState);

    activePets.delete(player.characterId);

    socket.emit('pet:returned', { petId });
    broadcastToZoneExcept(player.zone, socket.id, 'pet:other_returned', {
        ownerId: player.characterId, petId
    });
});
```

### 1.4 Feeding and Hunger Tick

```javascript
socket.on('pet:feed', async ({ petId }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const petState = activePets.get(player.characterId);
    if (!petState || petState.petId !== petId) {
        return socket.emit('pet:error', { reason: 'pet_not_active' });
    }

    const petDef = PET_DATA[petState.monsterId];

    // Check player has the correct food item
    const foodResult = await pool.query(
        `SELECT id, quantity FROM character_inventory
         WHERE character_id = $1 AND item_id = $2 AND quantity > 0
         LIMIT 1`,
        [player.characterId, petDef.food]
    );
    if (foodResult.rows.length === 0) {
        return socket.emit('pet:error', { reason: 'no_food' });
    }

    // Consume one food item
    await pool.query(
        `UPDATE character_inventory SET quantity = quantity - 1 WHERE id = $1`,
        [foodResult.rows[0].id]
    );
    await pool.query(
        `DELETE FROM character_inventory WHERE id = $1 AND quantity <= 0`,
        [foodResult.rows[0].id]
    );

    // Determine intimacy change based on hunger bracket
    const oldHunger = petState.hunger;
    let intimacyDelta = 0;
    let emote = 'happy';

    if (oldHunger <= 10) {
        intimacyDelta = petDef.intimacyGainPerFeed.veryHungry;
        emote = 'grateful';
    } else if (oldHunger <= 25) {
        intimacyDelta = petDef.intimacyGainPerFeed.hungry;
        emote = 'happy';
    } else if (oldHunger <= 75) {
        intimacyDelta = petDef.intimacyGainPerFeed.neutral;
        emote = 'ok';
    } else if (oldHunger <= 90) {
        intimacyDelta = petDef.intimacyGainPerFeed.satisfied;
        emote = 'hmm';
    } else {
        // Stuffed (91-100) -- overfeeding
        intimacyDelta = petDef.intimacyGainPerFeed.stuffed;
        petState.overfeedCount++;
        if (petState.overfeedCount === 1) emote = 'ok';
        else if (petState.overfeedCount === 2) emote = 'pif';
        else if (petState.overfeedCount >= 3) {
            // Pet runs away permanently
            emote = 'omg';
            await petFlees(player, petState);
            return socket.emit('pet:fled', { petId, emote });
        }
    }

    // Apply hunger increase (+20 per feed, clamped 0-100)
    petState.hunger = Math.min(100, petState.hunger + 20);

    // Apply intimacy change (clamped 0-1000)
    const oldIntimacy = petState.intimacy;
    petState.intimacy = Math.max(0, Math.min(1000, petState.intimacy + intimacyDelta));

    // Clear starvation timer if hunger is no longer 0
    if (petState.hunger > 0 && petState.starvationTimer) {
        clearInterval(petState.starvationTimer);
        petState.starvationTimer = null;
    }

    // Check if intimacy bracket changed -- reapply bonuses
    if (getIntimacyBracket(oldIntimacy) !== getIntimacyBracket(petState.intimacy)) {
        removePetBonuses(player, petState);
        applyPetBonuses(player, petState);
    }

    socket.emit('pet:fed', {
        petId,
        hunger: petState.hunger,
        intimacy: petState.intimacy,
        intimacyLevel: getIntimacyBracket(petState.intimacy),
        emote
    });

    emitInventoryData(socket, player.characterId);
});

function tickPetHunger(characterId) {
    const petState = activePets.get(characterId);
    if (!petState) return;

    const petDef = PET_DATA[petState.monsterId];
    petState.hunger = Math.max(0, petState.hunger - petDef.hungerRate);

    // Find the player socket
    const playerSocket = findSocketByCharacterId(characterId);
    if (playerSocket) {
        playerSocket.emit('pet:hunger_tick', {
            petId: petState.petId,
            hunger: petState.hunger
        });
    }

    // Start starvation timer if hunger reaches 0
    if (petState.hunger <= 0 && !petState.starvationTimer) {
        petState.starvationTimer = setInterval(async () => {
            petState.intimacy = Math.max(0, petState.intimacy - 20);
            if (playerSocket) {
                playerSocket.emit('pet:intimacy_changed', {
                    petId: petState.petId,
                    intimacy: petState.intimacy,
                    level: getIntimacyBracket(petState.intimacy)
                });
            }
            if (petState.intimacy <= 0) {
                const player = findPlayerByCharacterId(characterId);
                if (player) await petFlees(player, petState);
            }
        }, 20000); // Every 20 seconds
    }
}

function getIntimacyBracket(intimacy) {
    if (intimacy >= 910) return 'loyal';
    if (intimacy >= 750) return 'cordial';
    if (intimacy >= 250) return 'neutral';
    if (intimacy >= 100) return 'shy';
    return 'awkward';
}

async function petFlees(player, petState) {
    // Permanently delete the pet
    await pool.query(`DELETE FROM character_pets WHERE pet_id = $1`, [petState.petId]);
    // Delete the egg item from inventory
    await pool.query(
        `DELETE FROM character_inventory WHERE pet_id = $1`,
        [petState.petId]
    );

    if (petState.hungerTimer) clearInterval(petState.hungerTimer);
    if (petState.starvationTimer) clearInterval(petState.starvationTimer);
    activePets.delete(player.characterId);

    const playerSocket = findSocketByCharacterId(player.characterId);
    if (playerSocket) {
        playerSocket.emit('pet:fled', { petId: petState.petId });
        emitInventoryData(playerSocket, player.characterId);
    }
    broadcastToZoneExcept(player.zone, playerSocket?.id, 'pet:other_returned', {
        ownerId: player.characterId, petId: petState.petId
    });
}
```

### 1.5 Pet Bonuses

```javascript
function applyPetBonuses(player, petState) {
    const petDef = PET_DATA[petState.monsterId];
    const bracket = getIntimacyBracket(petState.intimacy);
    let bonuses = null;

    if (bracket === 'loyal') bonuses = petDef.bonusLoyal;
    else if (bracket === 'cordial') bonuses = petDef.bonusCordial;

    if (!bonuses) return;

    // Store active bonuses on petState for removal later
    petState.activeBonuses = { ...bonuses };

    // Apply to player stats (existing recalcStats pattern)
    for (const [stat, value] of Object.entries(bonuses)) {
        if (player[stat] !== undefined) {
            player[stat] += value;
        }
    }

    // Emit updated stats to client
    const playerSocket = findSocketByCharacterId(player.characterId);
    if (playerSocket) emitPlayerStats(playerSocket, player);
}

function removePetBonuses(player, petState) {
    if (!petState.activeBonuses) return;

    for (const [stat, value] of Object.entries(petState.activeBonuses)) {
        if (player[stat] !== undefined) {
            player[stat] -= value;
        }
    }
    petState.activeBonuses = null;

    const playerSocket = findSocketByCharacterId(player.characterId);
    if (playerSocket) emitPlayerStats(playerSocket, player);
}
```

### 1.6 Pet Position Following (Server-Side)

```javascript
// Called from the existing player:position handler
// After updating player.lastX/lastY/lastZ:
function updatePetPosition(player) {
    const petState = activePets.get(player.characterId);
    if (!petState) return;

    const dx = player.lastX - petState.x;
    const dy = player.lastY - petState.y;
    const dist = Math.sqrt(dx * dx + dy * dy);

    // Teleport if too far (>750 UE units = ~15 RO cells)
    if (dist > 750) {
        petState.x = player.lastX + 100;
        petState.y = player.lastY + 100;
        petState.z = player.lastZ;
    }
    // Move toward player if > follow distance (150 UE units = ~3 cells)
    else if (dist > 150) {
        const moveSpeed = 200; // UE units per position tick
        const angle = Math.atan2(dy, dx);
        petState.x += Math.cos(angle) * Math.min(moveSpeed, dist - 100);
        petState.y += Math.sin(angle) * Math.min(moveSpeed, dist - 100);
        petState.z = player.lastZ;
    }

    // Throttled broadcast (piggyback on player position, every 500ms at most)
    broadcastToZone(player.zone, 'pet:position', {
        ownerId: player.characterId,
        petId: petState.petId,
        x: petState.x, y: petState.y, z: petState.z
    });
}
```

---

## 2. Pet System (Client)

### 2.1 Pet Data Structs

Add to `CharacterData.h`:

```cpp
// ============================================================
// Pet data -- mirrors server pet:summoned payload
// ============================================================

UENUM()
enum class EPetIntimacyLevel : uint8
{
    Awkward,    // 0-99
    Shy,        // 100-249
    Neutral,    // 250-749
    Cordial,    // 750-909
    Loyal       // 910-1000
};

USTRUCT()
struct FPetData
{
    GENERATED_BODY()

    int32 PetId = 0;
    int32 MonsterId = 0;
    FString Name;
    int32 Intimacy = 250;
    int32 Hunger = 50;
    int32 AccessoryId = 0;
    EPetIntimacyLevel IntimacyLevel = EPetIntimacyLevel::Neutral;

    bool IsValid() const { return PetId > 0; }
};
```

### 2.2 UPetSubsystem

**File:** `client/SabriMMO/Source/SabriMMO/Companions/PetSubsystem.h`

```cpp
// PetSubsystem.h -- UWorldSubsystem managing pet summoning, feeding,
// status display, and the pet actor in the world.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "PetSubsystem.generated.h"

class USocketIOClientComponent;
class APetActor;
class SPetStatusWidget;

UCLASS()
class SABRIMMO_API UPetSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Current pet data (read by UI) ----
    FPetData ActivePet;
    bool bHasPet = false;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Commands (called from UI/hotkeys) ----
    void RequestSummonPet(int32 PetId);
    void RequestReturnPet();
    void RequestFeedPet();
    void RequestRenamePet(const FString& NewName);
    void RequestPerformance();

    // ---- Widget ----
    void ShowPetStatusWindow();
    void HidePetStatusWindow();
    bool IsStatusWindowVisible() const;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event handlers ----
    void HandlePetSummoned(const TSharedPtr<FJsonValue>& Data);
    void HandlePetReturned(const TSharedPtr<FJsonValue>& Data);
    void HandlePetFed(const TSharedPtr<FJsonValue>& Data);
    void HandlePetFled(const TSharedPtr<FJsonValue>& Data);
    void HandlePetHungerTick(const TSharedPtr<FJsonValue>& Data);
    void HandlePetIntimacyChanged(const TSharedPtr<FJsonValue>& Data);
    void HandlePetPosition(const TSharedPtr<FJsonValue>& Data);
    void HandlePetTameResult(const TSharedPtr<FJsonValue>& Data);

    // Other players' pets
    void HandleOtherPetSummoned(const TSharedPtr<FJsonValue>& Data);
    void HandleOtherPetReturned(const TSharedPtr<FJsonValue>& Data);

    // ---- Pet actor management ----
    void SpawnPetActor(int32 MonsterId, const FVector& Location);
    void DespawnPetActor();

    // ---- State ----
    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    UPROPERTY()
    APetActor* LocalPetActor = nullptr;

    // Other players' pet actors: OwnerId -> PetActor
    UPROPERTY()
    TMap<int32, APetActor*> OtherPetActors;

    TSharedPtr<SPetStatusWidget> PetStatusWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 2.3 PetSubsystem.cpp (Key Methods)

```cpp
// PetSubsystem.cpp

#include "PetSubsystem.h"
#include "PetActor.h"
#include "SocketIOClientComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "MMOGameInstance.h"

bool UPetSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    // Only create in game levels (same pattern as BasicInfoSubsystem)
    // Skip login level
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->FindComponentByClass<USocketIOClientComponent>())
            return true;
    }
    return false;
}

void UPetSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    // Deferred bind -- wait for SocketManager to exist
    InWorld.GetTimerManager().SetTimer(BindCheckTimer, [this]()
    {
        TryWrapSocketEvents();
    }, 0.5f, true);
}

void UPetSubsystem::HandlePetSummoned(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
    if (!Obj) return;

    ActivePet.PetId = Obj->GetIntegerField(TEXT("petId"));
    ActivePet.MonsterId = Obj->GetIntegerField(TEXT("monsterId"));
    ActivePet.Name = Obj->GetStringField(TEXT("name"));
    ActivePet.Intimacy = Obj->GetIntegerField(TEXT("intimacy"));
    ActivePet.Hunger = Obj->GetIntegerField(TEXT("hunger"));
    ActivePet.AccessoryId = Obj->GetIntegerField(TEXT("accessoryId"));
    bHasPet = true;

    const float PetX = Obj->GetNumberField(TEXT("x"));
    const float PetY = Obj->GetNumberField(TEXT("y"));
    const float PetZ = Obj->GetNumberField(TEXT("z"));

    SpawnPetActor(ActivePet.MonsterId, FVector(PetX, PetY, PetZ));
}

void UPetSubsystem::HandlePetReturned(const TSharedPtr<FJsonValue>& Data)
{
    ActivePet = FPetData();
    bHasPet = false;
    DespawnPetActor();
}

void UPetSubsystem::RequestSummonPet(int32 PetId)
{
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetNumberField(TEXT("petId"), PetId);

    TSharedRef<FJsonValueObject> Val = MakeShared<FJsonValueObject>(Payload);
    SIO->EmitNative(TEXT("pet:summon"), Val);
}

void UPetSubsystem::RequestFeedPet()
{
    if (!bHasPet) return;
    USocketIOClientComponent* SIO = FindSocketIOComponent();
    if (!SIO) return;

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetNumberField(TEXT("petId"), ActivePet.PetId);

    TSharedRef<FJsonValueObject> Val = MakeShared<FJsonValueObject>(Payload);
    SIO->EmitNative(TEXT("pet:feed"), Val);
}
```

### 2.4 Pet Actor with Follow AI

**File:** `client/SabriMMO/Source/SabriMMO/Companions/PetActor.h`

```cpp
// PetActor.h -- Cosmetic pet companion that follows the owner.
// No combat, no HP bar. Displays nameplate and idle/walk animations.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PetActor.generated.h"

class USkeletalMeshComponent;
class UWidgetComponent;

UCLASS()
class SABRIMMO_API APetActor : public AActor
{
    GENERATED_BODY()

public:
    APetActor();

    virtual void Tick(float DeltaTime) override;

    // ---- Configuration ----
    void Initialize(int32 InMonsterId, const FString& InPetName, int32 InOwnerId);
    void SetTargetLocation(const FVector& NewTarget);
    void TeleportToOwner(const FVector& OwnerLocation);

    UPROPERTY(VisibleAnywhere)
    USkeletalMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere)
    UWidgetComponent* NameplateWidget;

    // ---- State ----
    int32 MonsterId = 0;
    int32 OwnerId = 0;
    FString PetName;

private:
    // ---- Follow AI (client-side interpolation) ----
    FVector TargetLocation = FVector::ZeroVector;
    FVector ServerLocation = FVector::ZeroVector;

    float FollowDistance = 150.0f;      // 3 RO cells (50 UE units per cell)
    float TeleportDistance = 750.0f;    // 15 RO cells
    float MoveSpeed = 300.0f;           // UE units per second
    float IdleTimer = 0.0f;

    bool bIsMoving = false;

    // Smooth follow toward owner pawn (local pet only)
    void FollowOwnerPawn(float DeltaTime);
    // Interpolate toward server position (other players' pets)
    void InterpolateToTarget(float DeltaTime);

    bool bIsLocalPet = false;
};
```

**File:** `client/SabriMMO/Source/SabriMMO/Companions/PetActor.cpp`

```cpp
#include "PetActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

APetActor::APetActor()
{
    PrimaryActorTick.bCanEverTick = true;

    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PetMesh"));
    RootComponent = MeshComponent;

    NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Nameplate"));
    NameplateWidget->SetupAttachment(RootComponent);
    NameplateWidget->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen);
    NameplateWidget->SetDrawSize(FVector2D(200.f, 30.f));
}

void APetActor::Initialize(int32 InMonsterId, const FString& InPetName, int32 InOwnerId)
{
    MonsterId = InMonsterId;
    PetName = InPetName;
    OwnerId = InOwnerId;

    // Determine if this is the local player's pet
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    // Compare OwnerId with local character ID from GameInstance
    // (actual implementation reads from UMMOGameInstance)
    bIsLocalPet = (OwnerId == GetLocalCharacterId());

    // Load monster mesh based on MonsterId
    // (asset lookup from a data table or direct path convention)
    // e.g., "/Game/SabriMMO/Monsters/SK_Poring" for MonsterId 1002
}

void APetActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsLocalPet)
    {
        FollowOwnerPawn(DeltaTime);
    }
    else
    {
        InterpolateToTarget(DeltaTime);
    }
}

void APetActor::FollowOwnerPawn(float DeltaTime)
{
    // Find the local player pawn
    APawn* OwnerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!OwnerPawn) return;

    const FVector OwnerLocation = OwnerPawn->GetActorLocation();
    const FVector CurrentLocation = GetActorLocation();
    const FVector ToOwner = OwnerLocation - CurrentLocation;
    const float DistanceToOwner = ToOwner.Size2D();

    // Teleport if too far
    if (DistanceToOwner > TeleportDistance)
    {
        const FVector Offset = -OwnerPawn->GetActorForwardVector() * FollowDistance;
        SetActorLocation(OwnerLocation + Offset);
        bIsMoving = false;
        return;
    }

    // Move toward owner if beyond follow distance
    if (DistanceToOwner > FollowDistance)
    {
        const FVector Direction = ToOwner.GetSafeNormal2D();
        const float MoveAmount = FMath::Min(MoveSpeed * DeltaTime, DistanceToOwner - FollowDistance * 0.8f);
        FVector NewLocation = CurrentLocation + Direction * MoveAmount;
        NewLocation.Z = OwnerLocation.Z; // Match owner Z

        SetActorLocation(NewLocation);

        // Face movement direction
        if (!Direction.IsNearlyZero())
        {
            SetActorRotation(Direction.Rotation());
        }

        bIsMoving = true;
        IdleTimer = 0.0f;
    }
    else
    {
        bIsMoving = false;
        IdleTimer += DeltaTime;

        // Play idle emote every 10-20 seconds
        if (IdleTimer > 10.0f + FMath::FRand() * 10.0f)
        {
            // Trigger random idle animation
            IdleTimer = 0.0f;
        }
    }
}

void APetActor::InterpolateToTarget(float DeltaTime)
{
    // Smooth interpolation toward last known server position
    const FVector CurrentLocation = GetActorLocation();
    const FVector ToTarget = TargetLocation - CurrentLocation;
    const float Distance = ToTarget.Size();

    if (Distance > TeleportDistance)
    {
        SetActorLocation(TargetLocation);
        return;
    }

    if (Distance > 5.0f) // Dead zone
    {
        const float InterpSpeed = 5.0f;
        const FVector NewLocation = FMath::VInterpTo(
            CurrentLocation, TargetLocation, DeltaTime, InterpSpeed);
        SetActorLocation(NewLocation);

        const FVector Direction = ToTarget.GetSafeNormal2D();
        if (!Direction.IsNearlyZero())
        {
            SetActorRotation(Direction.Rotation());
        }
    }
}

void APetActor::SetTargetLocation(const FVector& NewTarget)
{
    TargetLocation = NewTarget;
}

void APetActor::TeleportToOwner(const FVector& OwnerLocation)
{
    const FVector Offset = FVector(-FollowDistance, -FollowDistance, 0.f);
    SetActorLocation(OwnerLocation + Offset);
    TargetLocation = OwnerLocation + Offset;
}
```

### 2.5 Pet Status Window (Slate)

**File:** `client/SabriMMO/Source/SabriMMO/Companions/SPetStatusWidget.h`

```cpp
// SPetStatusWidget.h -- Draggable pet info window showing name, intimacy,
// hunger bar, and command buttons (Feed/Performance/Return/Rename).

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UPetSubsystem;

class SPetStatusWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPetStatusWidget) {}
        SLATE_ARGUMENT(UPetSubsystem*, PetSubsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateClipRectangleList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    UPetSubsystem* PetSubsystem = nullptr;

    // Draggable title bar (same pattern as SBasicInfoWidget)
    FVector2D DragOffset = FVector2D::ZeroVector;
    bool bIsDragging = false;

    // Command button callbacks
    FReply OnFeedClicked();
    FReply OnPerformanceClicked();
    FReply OnReturnClicked();
    FReply OnRenameClicked();
};
```

Widget Z-order: **Z=18** (between hotbar Z=16 and Kafra Z=19).

---

## 3. Homunculus System (Server)

### 3.1 Data File: `server/src/ro_homunculus_data.js`

```javascript
// server/src/ro_homunculus_data.js

const HOMUNCULUS_TYPES = {
    lif: {
        race: 'demihuman',
        element: 'neutral',
        food: 537, // Pet Food
        baseStats: { hp: 150, sp: 40, str: 12, agi: 20, vit: 15, int: 35, dex: 24, luk: 12 },
        growthAvg: { hp: 80, sp: 6.5, str: 0.67, agi: 0.67, vit: 0.67, int: 0.71, dex: 0.80, luk: 0.80 },
        // Stat growth probabilities: [chance_0, chance_1, chance_2]
        growthProb: {
            str: [0.33, 0.60, 0.07], agi: [0.33, 0.60, 0.07],
            vit: [0.33, 0.60, 0.07], int: [0.29, 0.64, 0.07],
            dex: [0.20, 0.73, 0.07], luk: [0.20, 0.73, 0.07]
        },
        hpGrowthRange: [60, 100],   // Min/max HP gain per level
        spGrowthRange: [4, 9],
        skills: {
            1: { id: 'healing_hands',   maxLv: 5, type: 'active',  spCost: [13,16,19,22,25] },
            2: { id: 'urgent_escape',   maxLv: 5, type: 'active',  spCost: [20,20,20,20,20] },
            3: { id: 'brain_surgery',   maxLv: 5, type: 'passive' },
            4: { id: 'mental_charge',   maxLv: 3, type: 'active',  evolved: true, intimacyReq: 50 }
        }
    },
    amistr: {
        race: 'brute',
        element: 'neutral',
        food: 912, // Zargon
        baseStats: { hp: 320, sp: 10, str: 20, agi: 17, vit: 35, int: 11, dex: 24, luk: 12 },
        growthAvg: { hp: 105, sp: 2.5, str: 0.92, agi: 0.71, vit: 0.71, int: 0.10, dex: 0.59, luk: 0.59 },
        growthProb: {
            str: [0.15, 0.77, 0.08], agi: [0.35, 0.59, 0.06],
            vit: [0.35, 0.59, 0.06], int: [0.90, 0.10, 0.00],
            dex: [0.41, 0.59, 0.00], luk: [0.41, 0.59, 0.00]
        },
        hpGrowthRange: [80, 130],
        spGrowthRange: [1, 4],
        skills: {
            1: { id: 'castling',          maxLv: 5, type: 'active',  spCost: [10,10,10,10,10] },
            2: { id: 'amistr_bulwark',    maxLv: 5, type: 'active',  spCost: [20,20,20,20,20] },
            3: { id: 'adamantium_skin',   maxLv: 5, type: 'passive' },
            4: { id: 'blood_lust',        maxLv: 3, type: 'active',  evolved: true, intimacyReq: 50 }
        }
    },
    filir: {
        race: 'brute',
        element: 'neutral',
        food: 910, // Garlet
        baseStats: { hp: 90, sp: 25, str: 29, agi: 35, vit: 9, int: 8, dex: 30, luk: 9 },
        growthAvg: { hp: 60, sp: 4.5, str: 0.71, agi: 0.92, vit: 0.10, int: 0.59, dex: 0.71, luk: 0.59 },
        growthProb: {
            str: [0.35, 0.59, 0.06], agi: [0.15, 0.77, 0.08],
            vit: [0.90, 0.10, 0.00], int: [0.41, 0.59, 0.00],
            dex: [0.35, 0.59, 0.06], luk: [0.41, 0.59, 0.00]
        },
        hpGrowthRange: [45, 75],
        spGrowthRange: [3, 6],
        skills: {
            1: { id: 'moonlight',          maxLv: 5, type: 'offensive', spCost: [4,4,4,4,4] },
            2: { id: 'flitting',           maxLv: 5, type: 'active',    spCost: [30,30,30,30,30] },
            3: { id: 'accelerated_flight', maxLv: 5, type: 'active',    spCost: [30,30,30,30,30] },
            4: { id: 'sbr44',             maxLv: 3, type: 'offensive',  evolved: true, intimacyReq: 2 }
        }
    },
    vanilmirth: {
        race: 'formless',
        element: 'neutral',
        food: 911, // Scell
        baseStats: { hp: 80, sp: 11, str: 11, agi: 11, vit: 11, int: 11, dex: 11, luk: 11 },
        growthAvg: { hp: 90, sp: 3.5, str: 1.1, agi: 1.1, vit: 1.1, int: 1.1, dex: 1.1, luk: 1.1 },
        growthProb: {
            str: [0.30, 0.33, 0.33, 0.04],  // Can gain +3
            agi: [0.30, 0.33, 0.33, 0.04],
            vit: [0.30, 0.33, 0.33, 0.04],
            int: [0.30, 0.33, 0.33, 0.04],
            dex: [0.30, 0.33, 0.33, 0.04],
            luk: [0.30, 0.33, 0.33, 0.04]
        },
        hpGrowthRange: [60, 120],
        spGrowthRange: [2, 5],
        skills: {
            1: { id: 'caprice',            maxLv: 5, type: 'offensive', spCost: [22,24,26,28,30] },
            2: { id: 'chaotic_blessings',  maxLv: 5, type: 'supportive' },
            3: { id: 'instruction_change', maxLv: 5, type: 'passive' },
            4: { id: 'self_destruction',   maxLv: 3, type: 'offensive', evolved: true, intimacyReq: 450 }
        }
    }
};

// Homunculus EXP table (same as player, total 203,562,540 at lv99)
// Use the existing ro_exp_tables if homunculus shares the same table,
// or define a separate one here.

module.exports = { HOMUNCULUS_TYPES };
```

### 3.2 Stat Growth on Level-Up

```javascript
const { HOMUNCULUS_TYPES } = require('./ro_homunculus_data');

function rollStatGrowth(probArray) {
    // probArray = [chance_0, chance_1, chance_2] or [c0, c1, c2, c3]
    const roll = Math.random();
    let cumulative = 0;
    for (let i = 0; i < probArray.length; i++) {
        cumulative += probArray[i];
        if (roll < cumulative) return i;
    }
    return 0;
}

function homunculusLevelUp(homunculus) {
    const typeDef = HOMUNCULUS_TYPES[homunculus.type];

    // Roll HP/SP gains
    const hpGain = typeDef.hpGrowthRange[0]
        + Math.floor(Math.random() * (typeDef.hpGrowthRange[1] - typeDef.hpGrowthRange[0] + 1));
    const spGain = typeDef.spGrowthRange[0]
        + Math.floor(Math.random() * (typeDef.spGrowthRange[1] - typeDef.spGrowthRange[0] + 1));

    homunculus.hpMax += hpGain;
    homunculus.spMax += spGain;

    // Roll each stat independently
    const statGains = {};
    for (const stat of ['str', 'agi', 'vit', 'int', 'dex', 'luk']) {
        const gain = rollStatGrowth(typeDef.growthProb[stat]);
        const dbStat = stat === 'int' ? 'int_stat' : stat;
        homunculus[dbStat] += gain;
        statGains[stat] = gain;
    }

    homunculus.level++;
    homunculus.hpCurrent = homunculus.hpMax; // Full heal on level up

    // Award 1 skill point every 3 levels
    if (homunculus.level % 3 === 0) {
        homunculus.skillPoints++;
    }

    // Recalculate derived stats
    recalcHomunculusStats(homunculus);

    return { hpGain, spGain, statGains };
}

function recalcHomunculusStats(h) {
    // Formulas from Section 2.8 of the reference doc
    const intStat = h.int_stat || h.int || 0;

    h.atk  = Math.floor((h.str + h.dex + h.luk) / 3) + Math.floor(h.level / 10);
    h.matk = h.level + intStat + Math.floor((intStat + h.dex + h.luk) / 3)
             + Math.floor(h.level / 10) * 2;
    h.hit  = h.level + h.dex + 150;
    h.crit = Math.floor(h.luk / 3) + 1;
    h.def  = (h.vit + Math.floor(h.level / 10)) * 2
             + Math.floor((h.agi + Math.floor(h.level / 10)) / 2)
             + Math.floor(h.level / 2);
    h.flee = h.level + h.agi + Math.floor(h.level / 10);
    h.aspd = 130; // Fixed base
}
```

### 3.3 Homunculus Socket Events (Server)

```javascript
// In-memory map: characterId -> homunculus state
const activeHomunculi = new Map();

socket.on('homunculus:summon', async () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    // Class restriction: Alchemist/Biochemist/Creator only
    const allowedClasses = ['alchemist', 'biochemist', 'creator', 'geneticist'];
    if (!allowedClasses.includes(player.jobClass)) {
        return socket.emit('homunculus:error', { reason: 'class_restricted' });
    }

    // Check if already summoned
    if (activeHomunculi.has(player.characterId)) {
        return socket.emit('homunculus:error', { reason: 'already_summoned' });
    }

    // Check for existing homunculus in DB
    let homunResult = await pool.query(
        `SELECT * FROM character_homunculus WHERE character_id = $1`,
        [player.characterId]
    );

    if (homunResult.rows.length === 0) {
        // First time: need Embryo item
        const embryoResult = await pool.query(
            `SELECT id FROM character_inventory
             WHERE character_id = $1 AND item_id = 7142 AND quantity > 0
             LIMIT 1`,
            [player.characterId]
        );
        if (embryoResult.rows.length === 0) {
            return socket.emit('homunculus:error', { reason: 'no_embryo' });
        }

        // Consume Embryo
        await pool.query(
            `UPDATE character_inventory SET quantity = quantity - 1 WHERE id = $1`,
            [embryoResult.rows[0].id]
        );
        await pool.query(
            `DELETE FROM character_inventory WHERE id = $1 AND quantity <= 0`,
            [embryoResult.rows[0].id]
        );

        // Random type selection (25% each)
        const types = ['lif', 'amistr', 'filir', 'vanilmirth'];
        const chosenType = types[Math.floor(Math.random() * 4)];
        const spriteVariant = Math.random() < 0.5 ? 1 : 2;
        const typeDef = HOMUNCULUS_TYPES[chosenType];

        // Create homunculus record
        const insertResult = await pool.query(
            `INSERT INTO character_homunculus
                (character_id, type, sprite_variant, name, level,
                 hp_current, hp_max, sp_current, sp_max,
                 str, agi, vit, int_stat, dex, luk,
                 intimacy, hunger, is_alive, is_summoned)
             VALUES ($1, $2, $3, $4, 1,
                     $5, $5, $6, $6,
                     $7, $8, $9, $10, $11, $12,
                     250, 50, TRUE, TRUE)
             RETURNING *`,
            [
                player.characterId, chosenType, spriteVariant,
                typeDef.race === 'demihuman' ? 'Lif' : chosenType.charAt(0).toUpperCase() + chosenType.slice(1),
                typeDef.baseStats.hp, typeDef.baseStats.sp,
                typeDef.baseStats.str, typeDef.baseStats.agi,
                typeDef.baseStats.vit, typeDef.baseStats.int,
                typeDef.baseStats.dex, typeDef.baseStats.luk
            ]
        );
        homunResult = { rows: [insertResult.rows[0]] };
    } else {
        // Existing homunculus: just mark as summoned
        const h = homunResult.rows[0];
        if (!h.is_alive) {
            return socket.emit('homunculus:error', { reason: 'homunculus_dead' });
        }
        await pool.query(
            `UPDATE character_homunculus SET is_summoned = TRUE WHERE homunculus_id = $1`,
            [h.homunculus_id]
        );
    }

    const h = homunResult.rows[0];

    // Build in-memory state
    const hState = {
        homunculusId: h.homunculus_id,
        type: h.type,
        spriteVariant: h.sprite_variant,
        name: h.name,
        level: h.level,
        experience: Number(h.experience),
        hpCurrent: h.hp_current,
        hpMax: h.hp_max,
        spCurrent: h.sp_current,
        spMax: h.sp_max,
        str: h.str, agi: h.agi, vit: h.vit,
        int_stat: h.int_stat, dex: h.dex, luk: h.luk,
        intimacy: h.intimacy,
        hunger: h.hunger,
        isEvolved: h.is_evolved,
        skills: {
            1: h.skill_1_level, 2: h.skill_2_level,
            3: h.skill_3_level, 4: h.skill_4_level
        },
        skillPoints: h.skill_points,
        // Position near owner
        x: player.lastX + 200,
        y: player.lastY,
        z: player.lastZ,
        // Combat state
        targetId: null,
        command: 'follow',  // 'follow' | 'attack' | 'standby' | 'move'
        attackTimer: 0,
        // Derived stats (calculated)
        atk: 0, matk: 0, hit: 0, crit: 0, def: 0, flee: 0, aspd: 130
    };

    recalcHomunculusStats(hState);
    activeHomunculi.set(player.characterId, hState);

    // Start hunger tick
    hState.hungerTimer = setInterval(() => {
        tickHomunculusHunger(player.characterId);
    }, 60000);

    // Emit to owner
    socket.emit('homunculus:summoned', {
        type: hState.type,
        spriteVariant: hState.spriteVariant,
        name: hState.name,
        level: hState.level,
        hp: hState.hpCurrent, maxHp: hState.hpMax,
        sp: hState.spCurrent, maxSp: hState.spMax,
        stats: {
            str: hState.str, agi: hState.agi, vit: hState.vit,
            int: hState.int_stat, dex: hState.dex, luk: hState.luk,
            atk: hState.atk, matk: hState.matk, hit: hState.hit,
            crit: hState.crit, def: hState.def, flee: hState.flee
        },
        skills: hState.skills,
        skillPoints: hState.skillPoints,
        intimacy: hState.intimacy,
        evolved: hState.isEvolved,
        experience: hState.experience,
        x: hState.x, y: hState.y, z: hState.z
    });

    // Notify other players
    broadcastToZoneExcept(player.zone, socket.id, 'homunculus:other_summoned', {
        ownerId: player.characterId,
        type: hState.type,
        spriteVariant: hState.spriteVariant,
        name: hState.name,
        level: hState.level,
        hp: hState.hpCurrent, maxHp: hState.hpMax,
        x: hState.x, y: hState.y, z: hState.z
    });
});

socket.on('homunculus:vaporize', async () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const hState = activeHomunculi.get(player.characterId);
    if (!hState) return;

    // Requirement: HP >= 80% of MaxHP
    if (hState.hpCurrent < Math.floor(hState.hpMax * 0.8)) {
        return socket.emit('homunculus:error', { reason: 'hp_too_low' });
    }

    // Persist to DB
    await persistHomunculusState(player.characterId, hState, false);

    if (hState.hungerTimer) clearInterval(hState.hungerTimer);
    activeHomunculi.delete(player.characterId);

    socket.emit('homunculus:vaporized', {});
    broadcastToZoneExcept(player.zone, socket.id, 'homunculus:other_vaporized', {
        ownerId: player.characterId
    });
});

socket.on('homunculus:command', ({ command, targetId, x, y, z }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const hState = activeHomunculi.get(player.characterId);
    if (!hState) return;

    hState.command = command; // 'attack', 'move', 'standby', 'follow'
    if (command === 'attack' && targetId) {
        hState.targetId = targetId;
    } else if (command === 'move' && x !== undefined) {
        hState.moveTarget = { x, y, z };
    } else if (command === 'standby') {
        hState.targetId = null;
    } else if (command === 'follow') {
        hState.targetId = null;
    }
});
```

### 3.4 Homunculus Combat Integration

Add to the existing 50ms combat tick loop:

```javascript
// Inside the existing combat tick interval (every 50ms)
function tickHomunculusCombat() {
    for (const [characterId, hState] of activeHomunculi) {
        if (!hState || hState.hpCurrent <= 0) continue;

        const player = findPlayerByCharacterId(characterId);
        if (!player) continue;

        // ATTACK mode
        if (hState.command === 'attack' && hState.targetId) {
            const enemy = activeEnemies.get(hState.targetId);
            if (!enemy || enemy.isDead) {
                hState.command = 'follow';
                hState.targetId = null;
                continue;
            }

            // Move toward target
            const dx = enemy.x - hState.x;
            const dy = enemy.y - hState.y;
            const dist = Math.sqrt(dx * dx + dy * dy);

            if (dist > 150) { // Attack range
                // Move toward enemy
                const moveSpeed = 250;
                const angle = Math.atan2(dy, dx);
                hState.x += Math.cos(angle) * Math.min(moveSpeed * 0.05, dist - 100);
                hState.y += Math.sin(angle) * Math.min(moveSpeed * 0.05, dist - 100);
            } else {
                // Attack timer (ASPD 130 = 1400ms per attack)
                hState.attackTimer += 50;
                if (hState.attackTimer >= 1400) {
                    hState.attackTimer = 0;

                    // Hit calculation
                    const hitChance = Math.min(95, Math.max(5,
                        hState.hit - enemy.flee + 80));
                    if (Math.random() * 100 < hitChance) {
                        // Damage calculation
                        const baseDmg = hState.atk;
                        const variance = Math.floor(baseDmg * 0.15);
                        const damage = Math.max(1, baseDmg
                            - Math.floor(Math.random() * variance)
                            - Math.floor(enemy.def * 0.7));

                        enemy.hp -= damage;
                        broadcastToZone(player.zone, 'homunculus:attack', {
                            ownerId: characterId,
                            targetId: hState.targetId,
                            damage,
                            x: hState.x, y: hState.y, z: hState.z
                        });

                        // Aggro the enemy back (use existing setEnemyAggro)
                        setEnemyAggro(enemy, characterId, 'homunculus');

                        // Check kill
                        if (enemy.hp <= 0) {
                            handleEnemyDeath(enemy, player, hState);
                        }
                    }
                }
            }
        }
        // FOLLOW mode
        else if (hState.command === 'follow') {
            updateCompanionPosition(hState, player);
        }

        // Broadcast position (throttled)
        if (Date.now() - (hState.lastPosBroadcast || 0) > 500) {
            broadcastToZone(player.zone, 'homunculus:position', {
                ownerId: characterId,
                x: hState.x, y: hState.y, z: hState.z
            });
            hState.lastPosBroadcast = Date.now();
        }
    }
}

// EXP distribution when homunculus gets the kill or assists
function handleEnemyDeath(enemy, player, hState) {
    const template = monsterTemplates[enemy.monsterId];
    const totalBaseExp = template.baseExp || 0;
    const totalJobExp = template.jobExp || 0;

    // 90% to player, 10% to homunculus
    const playerBaseExp = Math.floor(totalBaseExp * 0.9);
    const playerJobExp = Math.floor(totalJobExp * 0.9);
    const homunBaseExp = Math.floor(totalBaseExp * 0.1);

    // Grant player EXP (existing function)
    grantPlayerExp(player, playerBaseExp, playerJobExp);

    // Grant homunculus EXP
    hState.experience += homunBaseExp;

    // Check level up (use existing EXP table or homunculus-specific one)
    const expNeeded = getHomunculusExpForLevel(hState.level + 1);
    if (hState.experience >= expNeeded && hState.level < 99) {
        const gains = homunculusLevelUp(hState);
        const playerSocket = findSocketByCharacterId(player.characterId);
        if (playerSocket) {
            playerSocket.emit('homunculus:leveled_up', {
                level: hState.level,
                stats: {
                    str: hState.str, agi: hState.agi, vit: hState.vit,
                    int: hState.int_stat, dex: hState.dex, luk: hState.luk
                },
                hpMax: hState.hpMax, spMax: hState.spMax,
                skillPoints: hState.skillPoints,
                gains
            });
        }
    }
}
```

### 3.5 Homunculus Evolution

```javascript
socket.on('homunculus:evolve', async () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const hState = activeHomunculi.get(player.characterId);
    if (!hState) return;

    if (hState.isEvolved) {
        return socket.emit('homunculus:error', { reason: 'already_evolved' });
    }
    if (hState.intimacy < 911) {
        return socket.emit('homunculus:error', { reason: 'intimacy_too_low' });
    }

    // Check for Stone of Sage item
    const stoneResult = await pool.query(
        `SELECT id FROM character_inventory
         WHERE character_id = $1 AND item_id = 7321 AND quantity > 0
         LIMIT 1`,
        [player.characterId]
    );
    if (stoneResult.rows.length === 0) {
        return socket.emit('homunculus:error', { reason: 'no_stone_of_sage' });
    }

    // Consume item
    await pool.query(
        `UPDATE character_inventory SET quantity = quantity - 1 WHERE id = $1`,
        [stoneResult.rows[0].id]
    );

    // Roll evolution bonus stats (+1 to +10 each)
    const bonusStats = {
        str: 1 + Math.floor(Math.random() * 10),
        agi: 1 + Math.floor(Math.random() * 10),
        vit: 1 + Math.floor(Math.random() * 10),
        int: 1 + Math.floor(Math.random() * 10),
        dex: 1 + Math.floor(Math.random() * 10),
        luk: 1 + Math.floor(Math.random() * 10)
    };

    // Apply bonuses
    hState.str += bonusStats.str;
    hState.agi += bonusStats.agi;
    hState.vit += bonusStats.vit;
    hState.int_stat += bonusStats.int;
    hState.dex += bonusStats.dex;
    hState.luk += bonusStats.luk;
    hState.isEvolved = true;
    hState.intimacy = 10; // Reset to Hate
    hState.spriteVariant = Math.random() < 0.5 ? 1 : 2; // New sprite roll

    recalcHomunculusStats(hState);

    // Persist evolution to DB
    await pool.query(
        `UPDATE character_homunculus SET
            is_evolved = TRUE, intimacy = 10,
            sprite_variant = $2,
            evolution_bonus_str = $3, evolution_bonus_agi = $4,
            evolution_bonus_vit = $5, evolution_bonus_int = $6,
            evolution_bonus_dex = $7, evolution_bonus_luk = $8,
            str = $9, agi = $10, vit = $11, int_stat = $12, dex = $13, luk = $14
         WHERE homunculus_id = $1`,
        [
            hState.homunculusId, hState.spriteVariant,
            bonusStats.str, bonusStats.agi, bonusStats.vit,
            bonusStats.int, bonusStats.dex, bonusStats.luk,
            hState.str, hState.agi, hState.vit,
            hState.int_stat, hState.dex, hState.luk
        ]
    );

    socket.emit('homunculus:evolved', {
        bonusStats,
        newSprite: hState.spriteVariant,
        intimacy: hState.intimacy,
        stats: {
            str: hState.str, agi: hState.agi, vit: hState.vit,
            int: hState.int_stat, dex: hState.dex, luk: hState.luk,
            atk: hState.atk, matk: hState.matk
        }
    });
});
```

---

## 4. Homunculus System (Client)

### 4.1 UHomunculusSubsystem

**File:** `client/SabriMMO/Source/SabriMMO/Companions/HomunculusSubsystem.h`

```cpp
// HomunculusSubsystem.h -- UWorldSubsystem managing homunculus state,
// combat commands, skill use, evolution, and the homunculus actor.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "HomunculusSubsystem.generated.h"

class USocketIOClientComponent;
class AHomunculusActor;
class SHomunculusStatusWidget;

UENUM()
enum class EHomunculusCommand : uint8
{
    Follow,
    Attack,
    Standby,
    Move
};

USTRUCT()
struct FHomunculusData
{
    GENERATED_BODY()

    FString Type;            // lif, amistr, filir, vanilmirth
    int32 SpriteVariant = 1;
    FString Name;
    int32 Level = 1;
    int64 Experience = 0;
    int32 HPCurrent = 0;
    int32 HPMax = 1;
    int32 SPCurrent = 0;
    int32 SPMax = 1;

    int32 STR = 0;
    int32 AGI = 0;
    int32 VIT = 0;
    int32 INT = 0;
    int32 DEX = 0;
    int32 LUK = 0;
    int32 ATK = 0;
    int32 MATK = 0;
    int32 HIT = 0;
    int32 CRIT = 0;
    int32 DEF = 0;
    int32 FLEE = 0;

    int32 Intimacy = 250;
    bool bIsEvolved = false;

    TMap<int32, int32> Skills; // Slot (1-4) -> Level
    int32 SkillPoints = 0;

    bool IsValid() const { return !Type.IsEmpty(); }
};

UCLASS()
class SABRIMMO_API UHomunculusSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    FHomunculusData ActiveHomunculus;
    bool bHasHomunculus = false;
    EHomunculusCommand CurrentCommand = EHomunculusCommand::Follow;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Commands ----
    void RequestSummon();
    void RequestVaporize();
    void RequestResurrect();
    void RequestFeed();
    void RequestEvolve();
    void SendCommand(EHomunculusCommand Command, int32 TargetId = 0,
        const FVector& Location = FVector::ZeroVector);
    void RequestUseSkill(int32 SkillSlot, int32 TargetId = 0,
        const FVector& Location = FVector::ZeroVector);
    void RequestAllocateSkillPoint(int32 SkillSlot);

    // ---- Widget ----
    void ToggleStatusWindow(); // Alt+R hotkey
    void ShowStatusWindow();
    void HideStatusWindow();

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event handlers ----
    void HandleSummoned(const TSharedPtr<FJsonValue>& Data);
    void HandleVaporized(const TSharedPtr<FJsonValue>& Data);
    void HandleDied(const TSharedPtr<FJsonValue>& Data);
    void HandleResurrected(const TSharedPtr<FJsonValue>& Data);
    void HandleFed(const TSharedPtr<FJsonValue>& Data);
    void HandleExpGained(const TSharedPtr<FJsonValue>& Data);
    void HandleLeveledUp(const TSharedPtr<FJsonValue>& Data);
    void HandleEvolved(const TSharedPtr<FJsonValue>& Data);
    void HandleSkillUsed(const TSharedPtr<FJsonValue>& Data);
    void HandlePosition(const TSharedPtr<FJsonValue>& Data);
    void HandleHPChanged(const TSharedPtr<FJsonValue>& Data);
    void HandleSPChanged(const TSharedPtr<FJsonValue>& Data);
    void HandleStatUpdate(const TSharedPtr<FJsonValue>& Data);
    void HandleIntimacyChanged(const TSharedPtr<FJsonValue>& Data);

    // Other players' homunculi
    void HandleOtherSummoned(const TSharedPtr<FJsonValue>& Data);
    void HandleOtherVaporized(const TSharedPtr<FJsonValue>& Data);

    void SpawnHomunculusActor(const FString& Type, int32 SpriteVariant,
        const FVector& Location);
    void DespawnHomunculusActor();

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    UPROPERTY()
    AHomunculusActor* LocalHomunculusActor = nullptr;

    UPROPERTY()
    TMap<int32, AHomunculusActor*> OtherHomunculusActors;

    TSharedPtr<SHomunculusStatusWidget> StatusWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 4.2 Homunculus Actor

The `AHomunculusActor` extends the same follow AI pattern as `APetActor` but adds combat capabilities. It implements `BPI_Damageable` to show damage numbers and `BPI_Targetable` for enemy AI targeting.

```cpp
// HomunculusActor.h -- Combat companion with own HP bar and attack animations.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomunculusActor.generated.h"

UCLASS()
class SABRIMMO_API AHomunculusActor : public AActor
{
    GENERATED_BODY()

public:
    AHomunculusActor();
    virtual void Tick(float DeltaTime) override;

    void Initialize(const FString& InType, int32 InSpriteVariant,
        const FString& InName, int32 InOwnerId);
    void SetTargetLocation(const FVector& NewTarget);
    void UpdateHP(int32 NewHP, int32 NewMaxHP);
    void PlayAttackAnimation();
    void PlaySkillEffect(int32 SkillSlot);
    void PlayDeathAnimation();

    UPROPERTY(VisibleAnywhere)
    USkeletalMeshComponent* MeshComponent;

    // State
    FString HomunculusType;
    int32 SpriteVariant = 1;
    FString HomunculusName;
    int32 OwnerId = 0;
    int32 CurrentHP = 0;
    int32 MaxHP = 1;

private:
    // Same follow/interpolation logic as PetActor
    void FollowOwnerPawn(float DeltaTime);
    void InterpolateToTarget(float DeltaTime);

    FVector TargetLocation = FVector::ZeroVector;
    float FollowDistance = 200.0f;
    float TeleportDistance = 750.0f;
    float MoveSpeed = 350.0f;
    bool bIsLocalHomunculus = false;
};
```

The HP bar for the homunculus is rendered by `WorldHealthBarSubsystem` -- add a `TMap<int32, FCompanionBarData> CompanionHealthMap` keyed by owner character ID, drawn below the homunculus actor using the same world-to-screen projection.

---

## 5. Mount System

### 5.1 Server Mount State

```javascript
// Add to player state (onlinePlayers map entries)
// player.isMounted = false
// player.mountType = null  // 'pecopeco' or 'grandpeco'
// player.cavalierMasteryLevel = 0

socket.on('mount:ride', async () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    // Class restriction
    const mountClasses = {
        'knight':    'pecopeco',
        'lord_knight': 'pecopeco',
        'crusader':  'grandpeco',
        'paladin':   'grandpeco'
    };

    const mountType = mountClasses[player.jobClass];
    if (!mountType) {
        return socket.emit('mount:error', { reason: 'class_restricted' });
    }

    if (player.isMounted) {
        return socket.emit('mount:error', { reason: 'already_mounted' });
    }

    // Check Peco Peco Ride skill (ID varies by implementation)
    // For now assume the skill check is done
    player.isMounted = true;
    player.mountType = mountType;

    // Apply mount effects
    player.weightLimit += 1000;  // +1000 weight capacity

    // ASPD penalty: 50% base, restored by Cavalier Mastery
    const cavalierLv = player.cavalierMasteryLevel || 0;
    const aspdMultiplier = 0.5 + (cavalierLv * 0.1); // Lv5 = 100%
    player.mountAspdMultiplier = aspdMultiplier;

    // Speed bonus (handled by movement speed calc)
    player.mountSpeedBonus = 1.25; // ~25% faster movement

    // Recalculate stats
    recalcPlayerStats(player);

    socket.emit('mount:mounted', { mountType });
    broadcastToZoneExcept(player.zone, socket.id, 'mount:other_mounted', {
        characterId: player.characterId,
        mountType
    });
});

socket.on('mount:dismount', async () => {
    const player = onlinePlayers.get(socket.id);
    if (!player || !player.isMounted) return;

    player.isMounted = false;
    player.mountType = null;

    // Remove mount effects
    player.weightLimit -= 1000;
    player.mountAspdMultiplier = 1.0;
    player.mountSpeedBonus = 1.0;

    recalcPlayerStats(player);

    // Weight check: if overweight after losing +1000, warn client
    socket.emit('mount:dismounted', {});
    broadcastToZoneExcept(player.zone, socket.id, 'mount:other_dismounted', {
        characterId: player.characterId
    });
});
```

### 5.2 ASPD Integration

In the existing ASPD calculation (combat tick), wrap the final ASPD with the mount multiplier:

```javascript
// In calculateASPD() or wherever ASPD is computed:
function getEffectiveASPD(player) {
    let aspd = calculateBaseASPD(player); // existing formula
    if (player.isMounted) {
        aspd = Math.floor(aspd * (player.mountAspdMultiplier || 0.5));
    }
    return aspd;
}
```

### 5.3 Client Mount System

The mount system does NOT use a separate subsystem. It is handled as a visual state change on `BP_MMOCharacter` and other player actors, triggered by socket events in the existing character Blueprint or via a lightweight C++ component.

**Mount state in GameInstance:**

Add to `UMMOGameInstance`:

```cpp
// In MMOGameInstance.h
UPROPERTY()
bool bIsMounted = false;

UPROPERTY()
FString MountType; // "pecopeco" or "grandpeco"
```

**Client mesh swap approach:**

When `mount:mounted` is received, the client swaps the character's skeletal mesh to a mounted variant. This is handled in Blueprint because mesh assets are binary and cannot be referenced from C++ without hard-coded paths.

**C++ helper for mesh swap:**

```cpp
// MountComponent.h -- Actor component handling mount visual state.
// Attach to BP_MMOCharacter.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MountComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMountStateChanged, bool, bIsMounted);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SABRIMMO_API UMountComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMountComponent();

    // Called by socket event handler
    UFUNCTION(BlueprintCallable, Category="Mount")
    void SetMountState(bool bMounted, const FString& InMountType);

    UFUNCTION(BlueprintPure, Category="Mount")
    bool IsMounted() const { return bIsMounted; }

    UFUNCTION(BlueprintPure, Category="Mount")
    FString GetMountType() const { return MountType; }

    // Event for Blueprint to react (swap mesh, change anim)
    UPROPERTY(BlueprintAssignable, Category="Mount")
    FOnMountStateChanged OnMountStateChanged;

    // Mesh references set in Blueprint defaults
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mount")
    USkeletalMesh* MountedMesh_PecoPeco;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mount")
    USkeletalMesh* MountedMesh_GrandPeco;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mount")
    UAnimBlueprint* MountedAnimBlueprint;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mount")
    USkeletalMesh* UnmountedMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mount")
    UAnimBlueprint* UnmountedAnimBlueprint;

private:
    bool bIsMounted = false;
    FString MountType;
};
```

```cpp
// MountComponent.cpp

#include "MountComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UMountComponent::UMountComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMountComponent::SetMountState(bool bMounted, const FString& InMountType)
{
    if (bIsMounted == bMounted) return;

    bIsMounted = bMounted;
    MountType = bMounted ? InMountType : FString();

    // Swap mesh on owner Character
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (OwnerChar && OwnerChar->GetMesh())
    {
        USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh();

        if (bMounted)
        {
            USkeletalMesh* TargetMesh = (InMountType == TEXT("grandpeco"))
                ? MountedMesh_GrandPeco
                : MountedMesh_PecoPeco;

            if (TargetMesh)
            {
                MeshComp->SetSkeletalMesh(TargetMesh);
            }
            if (MountedAnimBlueprint)
            {
                MeshComp->SetAnimInstanceClass(MountedAnimBlueprint->GeneratedClass);
            }
        }
        else
        {
            if (UnmountedMesh)
            {
                MeshComp->SetSkeletalMesh(UnmountedMesh);
            }
            if (UnmountedAnimBlueprint)
            {
                MeshComp->SetAnimInstanceClass(UnmountedAnimBlueprint->GeneratedClass);
            }
        }
    }

    // Broadcast for any Blueprint listeners
    OnMountStateChanged.Broadcast(bIsMounted);
}
```

**Socket event handling** (in the character Blueprint or `ASabriMMOCharacter` extension):

When `mount:mounted` / `mount:dismounted` arrives via socket, call `MountComponent->SetMountState(true/false, MountType)`. For other players, `mount:other_mounted` triggers the same call on `BP_OtherPlayer` via the OtherPlayerManager.

---

## 6. Falcon System (Hunter)

### 6.1 Server Falcon State

The falcon is NOT a separate entity. It is a boolean flag on the player that enables Blitz Beat auto-proc.

```javascript
// Player state additions:
// player.hasFalcon = false
// player.blitzBeatLevel = 0
// player.steelCrowLevel = 0

socket.on('falcon:rent', () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const allowedClasses = ['hunter', 'sniper', 'ranger'];
    if (!allowedClasses.includes(player.jobClass)) {
        return socket.emit('falcon:error', { reason: 'class_restricted' });
    }

    // Classic restriction: no falcon + cart at same time
    if (player.hasCart) {
        return socket.emit('falcon:error', { reason: 'has_cart' });
    }

    player.hasFalcon = true;

    socket.emit('falcon:rented', {});
    broadcastToZoneExcept(player.zone, socket.id, 'falcon:other_rented', {
        characterId: player.characterId
    });
});

socket.on('falcon:release', () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    player.hasFalcon = false;

    socket.emit('falcon:released', {});
    broadcastToZoneExcept(player.zone, socket.id, 'falcon:other_released', {
        characterId: player.characterId
    });
});
```

### 6.2 Blitz Beat Auto-Proc

Integrate into the existing auto-attack handler in the combat tick:

```javascript
// Inside the auto-attack damage section, after dealing normal attack damage:
function checkBlitzBeatAutoProc(player, targetEnemy) {
    if (!player.hasFalcon) return;
    if (player.blitzBeatLevel <= 0) return;

    // Auto-proc chance = LUK / 3 (percentage)
    const procChance = Math.floor(player.luk / 3);
    if (Math.random() * 100 >= procChance) return;

    // Cast delay check (1 second between auto-procs)
    const now = Date.now();
    if (player.lastBlitzBeatTime && (now - player.lastBlitzBeatTime) < 1000) return;
    player.lastBlitzBeatTime = now;

    // Hit count based on Job Level
    let maxHits;
    if (player.jobLevel >= 40) maxHits = 5;
    else if (player.jobLevel >= 30) maxHits = 4;
    else if (player.jobLevel >= 20) maxHits = 3;
    else if (player.jobLevel >= 10) maxHits = 2;
    else maxHits = 1;

    const hits = Math.min(maxHits, player.blitzBeatLevel);

    // Damage formula (Classic INT-based):
    // DamagePerHit = (SkillLevel * 20) + (SteelCrowLv * 6) + Floor(INT/2)*2 + Floor(DEX/10)*2
    const steelCrowLv = player.steelCrowLevel || 0;
    const intStat = player.int || 1;
    const damagePerHit = (player.blitzBeatLevel * 20)
        + (steelCrowLv * 6)
        + Math.floor(intStat / 2) * 2
        + Math.floor(player.dex / 10) * 2;

    // Auto-cast damage is divided by number of targets in AoE
    // For single target auto-proc, no division needed
    const totalDamage = damagePerHit * hits;

    targetEnemy.hp -= totalDamage;

    const playerSocket = findSocketByCharacterId(player.characterId);
    broadcastToZone(player.zone, 'combat:blitz_beat', {
        playerId: player.characterId,
        targetId: targetEnemy.id,
        hits,
        totalDamage,
        x: targetEnemy.x,
        y: targetEnemy.y,
        z: targetEnemy.z
    });

    setEnemyAggro(targetEnemy, player.characterId, 'falcon');

    if (targetEnemy.hp <= 0) {
        handleEnemyDeath(targetEnemy, player);
    }
}
```

### 6.3 Client Falcon Visual

The falcon is a cosmetic attachment on the character mesh. It does NOT get its own actor.

```cpp
// FalconComponent.h -- Attached to BP_MMOCharacter and BP_OtherPlayer.
// Manages the falcon mesh visibility on the character's shoulder.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FalconComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SABRIMMO_API UFalconComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFalconComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Falcon")
    void SetFalconVisible(bool bVisible);

    UFUNCTION(BlueprintPure, Category="Falcon")
    bool HasFalcon() const { return bHasFalcon; }

    // Play Blitz Beat fly-to-target animation
    UFUNCTION(BlueprintCallable, Category="Falcon")
    void PlayBlitzBeatAnimation(const FVector& TargetLocation);

    // Mesh set in Blueprint defaults
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Falcon")
    UStaticMesh* FalconMesh;

    // Socket name on the character skeleton to attach the falcon
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Falcon")
    FName AttachSocketName = TEXT("RightShoulder");

private:
    UPROPERTY()
    UStaticMeshComponent* FalconMeshComponent = nullptr;

    bool bHasFalcon = false;
};
```

```cpp
// FalconComponent.cpp

#include "FalconComponent.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

UFalconComponent::UFalconComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFalconComponent::BeginPlay()
{
    Super::BeginPlay();

    // Create the falcon mesh component attached to the character skeleton
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (OwnerChar && FalconMesh)
    {
        FalconMeshComponent = NewObject<UStaticMeshComponent>(OwnerChar,
            TEXT("FalconMeshComp"));
        FalconMeshComponent->SetStaticMesh(FalconMesh);
        FalconMeshComponent->SetVisibility(false);
        FalconMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FalconMeshComponent->AttachToComponent(
            OwnerChar->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            AttachSocketName
        );
        FalconMeshComponent->RegisterComponent();
    }
}

void UFalconComponent::SetFalconVisible(bool bVisible)
{
    bHasFalcon = bVisible;
    if (FalconMeshComponent)
    {
        FalconMeshComponent->SetVisibility(bVisible);
    }
}

void UFalconComponent::PlayBlitzBeatAnimation(const FVector& TargetLocation)
{
    if (!FalconMeshComponent || !bHasFalcon) return;

    // Detach falcon, fly to target via timeline, return to shoulder
    // Implementation: use a simple FTimerHandle + FMath::VInterpTo
    // or a Niagara effect for the falcon swooping in
    // The actual animation is best done in Blueprint via a Timeline node
    // triggered by an event dispatcher here.
}
```

When `combat:blitz_beat` is received by the client, the `DamageNumberSubsystem` shows damage numbers at the target position, and `FalconComponent::PlayBlitzBeatAnimation()` plays the visual effect.

---

## 7. Cart System (Merchant)

### 7.1 Server Cart State

```javascript
// Player state additions:
// player.hasCart = false
// player.cartInventory = []        // Array of { inventoryId, itemId, quantity, weight }
// player.cartWeight = 0
// player.cartMaxWeight = 8000
// player.cartMaxSlots = 100
// player.pushcartLevel = 0         // 1-10, affects speed penalty

socket.on('cart:rent', () => {
    const player = onlinePlayers.get(socket.id);
    if (!player) return;

    const cartClasses = ['merchant', 'blacksmith', 'whitesmith', 'mastersmith',
                         'alchemist', 'biochemist', 'creator', 'geneticist'];
    if (!cartClasses.includes(player.jobClass)) {
        return socket.emit('cart:error', { reason: 'class_restricted' });
    }

    // Classic restriction: no cart + falcon at same time (for Hunters only, N/A here)
    if (player.hasCart) {
        return socket.emit('cart:error', { reason: 'already_has_cart' });
    }

    player.hasCart = true;
    player.cartInventory = [];
    player.cartWeight = 0;

    // Apply speed penalty based on Pushcart skill level
    const speedPenalty = Math.max(0, 45 - (player.pushcartLevel * 5));
    player.cartSpeedPenalty = speedPenalty / 100; // 0.0 to 0.45

    socket.emit('cart:rented', {
        maxWeight: player.cartMaxWeight,
        maxSlots: player.cartMaxSlots,
        speedPenalty
    });
    broadcastToZoneExcept(player.zone, socket.id, 'cart:other_rented', {
        characterId: player.characterId,
        cartStyle: getCartStyle(player.baseLevel)
    });
});

function getCartStyle(baseLevel) {
    if (baseLevel >= 131) return 9;
    if (baseLevel >= 121) return 8;
    if (baseLevel >= 111) return 7;
    if (baseLevel >= 101) return 6;
    if (baseLevel >= 91) return 5;
    if (baseLevel >= 81) return 4;
    if (baseLevel >= 66) return 3;
    if (baseLevel >= 41) return 2;
    return 1;
}

socket.on('cart:move_to_cart', async ({ inventoryId, amount }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player || !player.hasCart) return;

    // Validate item exists in player inventory
    const invResult = await pool.query(
        `SELECT ci.*, i.weight, i.name, i.item_id
         FROM character_inventory ci
         JOIN items i ON ci.item_id = i.item_id
         WHERE ci.id = $1 AND ci.character_id = $2 AND ci.quantity >= $3`,
        [inventoryId, player.characterId, amount]
    );
    if (invResult.rows.length === 0) {
        return socket.emit('cart:error', { reason: 'invalid_item' });
    }

    const item = invResult.rows[0];
    const addedWeight = item.weight * amount;

    // Check cart capacity
    if (player.cartWeight + addedWeight > player.cartMaxWeight) {
        return socket.emit('cart:error', { reason: 'cart_overweight' });
    }
    if (player.cartInventory.length >= player.cartMaxSlots) {
        return socket.emit('cart:error', { reason: 'cart_full' });
    }

    // Remove from inventory
    if (item.quantity === amount) {
        await pool.query(`DELETE FROM character_inventory WHERE id = $1`, [inventoryId]);
    } else {
        await pool.query(
            `UPDATE character_inventory SET quantity = quantity - $1 WHERE id = $2`,
            [amount, inventoryId]
        );
    }

    // Add to cart (in-memory, persisted on disconnect/zone change)
    const existingCartSlot = player.cartInventory.find(
        ci => ci.itemId === item.item_id
    );
    if (existingCartSlot && item.stackable) {
        existingCartSlot.quantity += amount;
    } else {
        player.cartInventory.push({
            itemId: item.item_id,
            name: item.name,
            quantity: amount,
            weight: item.weight
        });
    }
    player.cartWeight += addedWeight;

    // Emit updated cart and inventory
    socket.emit('cart:data', {
        items: player.cartInventory,
        currentWeight: player.cartWeight,
        maxWeight: player.cartMaxWeight
    });
    emitInventoryData(socket, player.characterId);
});

socket.on('cart:move_from_cart', async ({ itemId, amount }) => {
    const player = onlinePlayers.get(socket.id);
    if (!player || !player.hasCart) return;

    const cartSlot = player.cartInventory.find(ci => ci.itemId === itemId);
    if (!cartSlot || cartSlot.quantity < amount) {
        return socket.emit('cart:error', { reason: 'invalid_cart_item' });
    }

    // Check player weight limit
    const addedWeight = cartSlot.weight * amount;
    // (weight check uses existing inventory weight logic)

    // Remove from cart
    cartSlot.quantity -= amount;
    player.cartWeight -= addedWeight;
    if (cartSlot.quantity <= 0) {
        player.cartInventory = player.cartInventory.filter(ci => ci.itemId !== itemId);
    }

    // Add to player inventory
    await pool.query(
        `INSERT INTO character_inventory (character_id, item_id, quantity)
         VALUES ($1, $2, $3)
         ON CONFLICT (character_id, item_id)
         DO UPDATE SET quantity = character_inventory.quantity + $3`,
        [player.characterId, itemId, amount]
    );

    socket.emit('cart:data', {
        items: player.cartInventory,
        currentWeight: player.cartWeight,
        maxWeight: player.cartMaxWeight
    });
    emitInventoryData(socket, player.characterId);
});
```

### 7.2 Cart Revolution Damage Formula

```javascript
// In the skill execution handler for Cart Revolution (skill ID varies):
function executeCartRevolution(player, targets) {
    if (!player.hasCart) return { damage: 0, error: 'no_cart' };

    // Damage% = 150 + (100 * CurrentCartWeight / MaxCartWeight)
    const damagePercent = 150 + Math.floor(100 * player.cartWeight / player.cartMaxWeight);
    const baseDamage = player.atk; // Use existing physical ATK calc

    for (const target of targets) { // 3x3 AoE
        const damage = Math.max(1, Math.floor(baseDamage * damagePercent / 100) - target.def);
        target.hp -= damage;

        // Knockback 2 cells (100 UE units)
        const dx = target.x - player.lastX;
        const dy = target.y - player.lastY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist > 0) {
            target.x += (dx / dist) * 100;
            target.y += (dy / dist) * 100;
        }

        broadcastToZone(player.zone, 'skill:effect_damage', {
            skillId: CART_REVOLUTION_ID,
            casterId: player.characterId,
            targetId: target.id,
            damage,
            targetX: target.x, targetY: target.y, targetZ: target.z
        });
    }
}
```

### 7.3 Client Cart Visual and UI

**Cart visual:** Attach a `UStaticMeshComponent` to the character, trailing behind. The mesh changes based on `getCartStyle(baseLevel)`. Use a Blueprint-configurable component similar to `UMountComponent`.

**Cart Inventory UI:**

**File:** `client/SabriMMO/Source/SabriMMO/Companions/CartSubsystem.h`

```cpp
// CartSubsystem.h -- UWorldSubsystem managing cart rental state,
// cart inventory, and the cart storage UI window.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "CartSubsystem.generated.h"

class USocketIOClientComponent;
class SCartInventoryWidget;

USTRUCT()
struct FPushcartItem
{
    GENERATED_BODY()

    int32 ItemId = 0;
    FString Name;
    int32 Quantity = 0;
    int32 Weight = 0;

    bool IsValid() const { return ItemId > 0; }
};

UCLASS()
class SABRIMMO_API UCartSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    bool bHasCart = false;
    TArray<FPushcartItem> CartItems;
    int32 CurrentWeight = 0;
    int32 MaxWeight = 8000;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // Commands
    void RequestMoveToCart(int32 InventoryId, int32 Amount);
    void RequestMoveFromCart(int32 ItemId, int32 Amount);
    void ToggleCartWindow();  // Alt+W hotkey

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandleCartData(const TSharedPtr<FJsonValue>& Data);
    void HandleCartRented(const TSharedPtr<FJsonValue>& Data);
    void HandleCartError(const TSharedPtr<FJsonValue>& Data);

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    TSharedPtr<SCartInventoryWidget> CartWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

Widget Z-order: **Z=17** (between hotbar Z=16 and pet status Z=18).

---

## 8. DB Schema & Migrations

### 8.1 Migration File: `database/migrations/add_companion_systems.sql`

```sql
-- Migration: add_companion_systems.sql
-- Adds tables for pet, homunculus, and cart persistence.
-- Run against sabri_mmo database.

-- ============================================================
-- PET SYSTEM
-- ============================================================

CREATE TABLE IF NOT EXISTS character_pets (
    pet_id          SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    monster_id      INTEGER NOT NULL,
    pet_name        VARCHAR(24) NOT NULL,
    intimacy        INTEGER NOT NULL DEFAULT 250,
    hunger          INTEGER NOT NULL DEFAULT 50,
    accessory_id    INTEGER DEFAULT NULL,
    is_summoned     BOOLEAN NOT NULL DEFAULT FALSE,
    renamed         BOOLEAN NOT NULL DEFAULT FALSE,
    egg_item_id     INTEGER DEFAULT NULL,
    created_at      TIMESTAMP DEFAULT NOW(),
    updated_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT chk_pet_intimacy CHECK (intimacy >= 0 AND intimacy <= 1000),
    CONSTRAINT chk_pet_hunger CHECK (hunger >= 0 AND hunger <= 100)
);

CREATE INDEX IF NOT EXISTS idx_pet_character ON character_pets(character_id);
CREATE INDEX IF NOT EXISTS idx_pet_summoned ON character_pets(character_id, is_summoned);

-- Add pet_id column to character_inventory for linking egg items to pets
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS pet_id INTEGER DEFAULT NULL;

-- ============================================================
-- HOMUNCULUS SYSTEM
-- ============================================================

CREATE TABLE IF NOT EXISTS character_homunculus (
    homunculus_id       SERIAL PRIMARY KEY,
    character_id        INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    type                VARCHAR(20) NOT NULL,
    sprite_variant      INTEGER NOT NULL DEFAULT 1,
    name                VARCHAR(24) NOT NULL,
    level               INTEGER NOT NULL DEFAULT 1,
    experience          BIGINT NOT NULL DEFAULT 0,
    intimacy            INTEGER NOT NULL DEFAULT 250,
    hunger              INTEGER NOT NULL DEFAULT 50,

    hp_current          INTEGER NOT NULL DEFAULT 0,
    hp_max              INTEGER NOT NULL DEFAULT 0,
    sp_current          INTEGER NOT NULL DEFAULT 0,
    sp_max              INTEGER NOT NULL DEFAULT 0,
    str                 INTEGER NOT NULL DEFAULT 0,
    agi                 INTEGER NOT NULL DEFAULT 0,
    vit                 INTEGER NOT NULL DEFAULT 0,
    int_stat            INTEGER NOT NULL DEFAULT 0,
    dex                 INTEGER NOT NULL DEFAULT 0,
    luk                 INTEGER NOT NULL DEFAULT 0,

    skill_1_level       INTEGER NOT NULL DEFAULT 0,
    skill_2_level       INTEGER NOT NULL DEFAULT 0,
    skill_3_level       INTEGER NOT NULL DEFAULT 0,
    skill_4_level       INTEGER NOT NULL DEFAULT 0,
    skill_points        INTEGER NOT NULL DEFAULT 0,

    is_evolved          BOOLEAN NOT NULL DEFAULT FALSE,
    evolution_bonus_str INTEGER DEFAULT 0,
    evolution_bonus_agi INTEGER DEFAULT 0,
    evolution_bonus_vit INTEGER DEFAULT 0,
    evolution_bonus_int INTEGER DEFAULT 0,
    evolution_bonus_dex INTEGER DEFAULT 0,
    evolution_bonus_luk INTEGER DEFAULT 0,

    is_alive            BOOLEAN NOT NULL DEFAULT TRUE,
    is_summoned         BOOLEAN NOT NULL DEFAULT FALSE,
    is_vaporized        BOOLEAN NOT NULL DEFAULT FALSE,

    x                   FLOAT DEFAULT 0,
    y                   FLOAT DEFAULT 0,
    z                   FLOAT DEFAULT 0,

    created_at          TIMESTAMP DEFAULT NOW(),
    updated_at          TIMESTAMP DEFAULT NOW(),

    CONSTRAINT chk_homun_type CHECK (type IN ('lif', 'amistr', 'filir', 'vanilmirth')),
    CONSTRAINT chk_homun_intimacy CHECK (intimacy >= 0 AND intimacy <= 1000),
    CONSTRAINT chk_homun_hunger CHECK (hunger >= 0 AND hunger <= 100)
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_homun_character
    ON character_homunculus(character_id);

-- ============================================================
-- CART PERSISTENCE (stored on disconnect, loaded on join)
-- ============================================================

CREATE TABLE IF NOT EXISTS character_cart (
    cart_id         SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    item_id         INTEGER NOT NULL,
    quantity        INTEGER NOT NULL DEFAULT 1,
    created_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT chk_cart_quantity CHECK (quantity > 0)
);

CREATE INDEX IF NOT EXISTS idx_cart_character ON character_cart(character_id);

-- ============================================================
-- CHARACTER STATE FLAGS (mount, falcon, cart rental)
-- ============================================================

ALTER TABLE characters ADD COLUMN IF NOT EXISTS is_mounted BOOLEAN DEFAULT FALSE;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS mount_type VARCHAR(20) DEFAULT NULL;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS has_falcon BOOLEAN DEFAULT FALSE;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS has_cart BOOLEAN DEFAULT FALSE;
```

### 8.2 Items Table Additions

```sql
-- Add pet-related items to the items table

-- Pet Eggs (one per pet type, non-stackable, unique per pet instance)
INSERT INTO items (item_id, name, description, item_type, weight, stackable, icon)
VALUES
    (9001, 'Poring Egg', 'A Pet Egg containing a Poring.', 'pet_egg', 0, false, 'pet_egg'),
    (9002, 'Drops Egg', 'A Pet Egg containing a Drops.', 'pet_egg', 0, false, 'pet_egg'),
    -- ... one entry per tameable monster
ON CONFLICT (item_id) DO NOTHING;

-- Pet Incubator
INSERT INTO items (item_id, name, description, item_type, weight, price, stackable, icon)
VALUES (7142, 'Pet Incubator', 'Hatches a Pet Egg.', 'consumable', 30, 3000, true, 'pet_incubator')
ON CONFLICT (item_id) DO NOTHING;

-- Embryo (Homunculus)
INSERT INTO items (item_id, name, description, item_type, weight, stackable, icon)
VALUES (7142, 'Embryo', 'A living embryo for creating a Homunculus.', 'consumable', 30, false, 'embryo')
ON CONFLICT (item_id) DO NOTHING;

-- Stone of Sage (Homunculus Evolution)
INSERT INTO items (item_id, name, description, item_type, weight, stackable, icon)
VALUES (7321, 'Stone of Sage', 'Used to evolve a Homunculus.', 'consumable', 30, false, 'stone_of_sage')
ON CONFLICT (item_id) DO NOTHING;
```

---

## Socket Event Summary

### Pet Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C -> S | `pet:tame` | `{ targetEnemyId, tamingItemId }` |
| C -> S | `pet:summon` | `{ petId }` |
| C -> S | `pet:return` | `{ petId }` |
| C -> S | `pet:feed` | `{ petId }` |
| C -> S | `pet:performance` | `{ petId }` |
| C -> S | `pet:rename` | `{ petId, newName }` |
| S -> C | `pet:tame_result` | `{ success, reason?, petId?, eggItemId? }` |
| S -> C | `pet:summoned` | `{ petId, monsterId, name, intimacy, hunger, accessoryId, x, y, z }` |
| S -> C | `pet:returned` | `{ petId }` |
| S -> C | `pet:fed` | `{ petId, hunger, intimacy, intimacyLevel, emote }` |
| S -> C | `pet:fled` | `{ petId }` |
| S -> C | `pet:hunger_tick` | `{ petId, hunger }` |
| S -> C | `pet:intimacy_changed` | `{ petId, intimacy, level }` |
| S -> C | `pet:position` | `{ ownerId, petId, x, y, z }` |
| S -> Other | `pet:other_summoned` | `{ ownerId, petId, monsterId, name, accessoryId, x, y, z }` |
| S -> Other | `pet:other_returned` | `{ ownerId, petId }` |

### Homunculus Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C -> S | `homunculus:summon` | `{}` |
| C -> S | `homunculus:vaporize` | `{}` |
| C -> S | `homunculus:resurrect` | `{}` |
| C -> S | `homunculus:feed` | `{}` |
| C -> S | `homunculus:command` | `{ command, targetId?, x?, y?, z? }` |
| C -> S | `homunculus:skill` | `{ skillSlot, targetId?, x?, y?, z? }` |
| C -> S | `homunculus:evolve` | `{}` |
| C -> S | `homunculus:allocate_skill` | `{ skillSlot }` |
| S -> C | `homunculus:summoned` | `{ type, spriteVariant, name, level, hp, maxHp, sp, maxSp, stats, skills, skillPoints, intimacy, evolved, experience, x, y, z }` |
| S -> C | `homunculus:vaporized` | `{}` |
| S -> C | `homunculus:died` | `{}` |
| S -> C | `homunculus:resurrected` | `{ hp }` |
| S -> C | `homunculus:fed` | `{ hunger, intimacy }` |
| S -> C | `homunculus:leveled_up` | `{ level, stats, hpMax, spMax, skillPoints, gains }` |
| S -> C | `homunculus:evolved` | `{ bonusStats, newSprite, intimacy, stats }` |
| S -> C | `homunculus:attack` | `{ ownerId, targetId, damage, x, y, z }` |
| S -> C | `homunculus:position` | `{ ownerId, x, y, z }` |
| S -> C | `homunculus:hp_changed` | `{ hp, maxHp }` |

### Mount Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C -> S | `mount:ride` | `{}` |
| C -> S | `mount:dismount` | `{}` |
| S -> C | `mount:mounted` | `{ mountType }` |
| S -> C | `mount:dismounted` | `{}` |
| S -> Other | `mount:other_mounted` | `{ characterId, mountType }` |
| S -> Other | `mount:other_dismounted` | `{ characterId }` |

### Falcon Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C -> S | `falcon:rent` | `{}` |
| C -> S | `falcon:release` | `{}` |
| S -> C | `falcon:rented` | `{}` |
| S -> C | `falcon:released` | `{}` |
| S -> Zone | `combat:blitz_beat` | `{ playerId, targetId, hits, totalDamage, x, y, z }` |

### Cart Events

| Direction | Event | Payload |
|-----------|-------|---------|
| C -> S | `cart:rent` | `{}` |
| C -> S | `cart:move_to_cart` | `{ inventoryId, amount }` |
| C -> S | `cart:move_from_cart` | `{ itemId, amount }` |
| S -> C | `cart:rented` | `{ maxWeight, maxSlots, speedPenalty }` |
| S -> C | `cart:data` | `{ items, currentWeight, maxWeight }` |
| S -> Other | `cart:other_rented` | `{ characterId, cartStyle }` |

---

## File Inventory

### New Server Files

| File | Purpose |
|------|---------|
| `server/src/ro_pet_data.js` | Pet definitions (34 entries) |
| `server/src/ro_homunculus_data.js` | Homunculus type definitions (4 types) |
| `database/migrations/add_companion_systems.sql` | DB migration |

### New Client Files

| File | Purpose |
|------|---------|
| `Companions/PetSubsystem.h/.cpp` | UWorldSubsystem for pet system |
| `Companions/PetActor.h/.cpp` | Cosmetic pet follow actor |
| `Companions/SPetStatusWidget.h/.cpp` | Pet status window (Slate, Z=18) |
| `Companions/HomunculusSubsystem.h/.cpp` | UWorldSubsystem for homunculus |
| `Companions/HomunculusActor.h/.cpp` | Combat companion actor |
| `Companions/SHomunculusStatusWidget.h/.cpp` | Homunculus info window (Slate, Z=18) |
| `Companions/CartSubsystem.h/.cpp` | UWorldSubsystem for pushcart |
| `Companions/SCartInventoryWidget.h/.cpp` | Cart inventory window (Slate, Z=17) |
| `Companions/MountComponent.h/.cpp` | Mount mesh swap component |
| `Companions/FalconComponent.h/.cpp` | Falcon shoulder attachment component |

All client files live under `client/SabriMMO/Source/SabriMMO/Companions/`.

### Modified Existing Files

| File | Changes |
|------|---------|
| `CharacterData.h` | Add `FPetData`, `FPushcartItem`, `FHomunculusData` structs |
| `MMOGameInstance.h` | Add `bIsMounted`, `MountType`, `bHasFalcon`, `bHasCart` |
| `SabriMMOCharacter.h/.cpp` | Add UI toggle hotkeys for pet/cart windows |
| `server/src/index.js` | Import new data modules, add all socket event handlers, integrate combat ticks |
| `WorldHealthBarSubsystem.h/.cpp` | Add `CompanionHealthMap` for homunculus HP bars |
