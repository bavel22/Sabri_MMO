# 09 -- NPC, Dialogue, Quest & Shop System: UE5 C++ Implementation Guide

> Complete implementation reference for the NPC interaction pipeline, JSON dialogue trees,
> server-authoritative shop economy, quest state machine, and all associated Slate widgets.
> All code follows existing Sabri_MMO patterns (UWorldSubsystem, event wrapping, RO Classic theme).

---

## Table of Contents

1. [NPC Architecture](#1-npc-architecture)
2. [Dialogue System](#2-dialogue-system)
3. [Shop System](#3-shop-system)
4. [Quest System -- Server](#4-quest-system-server)
5. [Quest System -- Client](#5-quest-system-client)
6. [Job Change Quest Template](#6-job-change-quest-template)
7. [Refine NPC](#7-refine-npc)
8. [Adding New NPC Template](#8-adding-new-npc-template)

---

## 1. NPC Architecture

### 1.1 Server-Side NPC Registry

All NPC definitions live in a dedicated data module loaded at server startup.

**File: `server/src/ro_npc_data.js`**

```javascript
// ============================================================
// NPC Type Constants
// ============================================================

const NPC_TYPES = {
  SHOP_TOOL:    'shop_tool',
  SHOP_WEAPON:  'shop_weapon',
  SHOP_ARMOR:   'shop_armor',
  KAFRA:        'kafra',
  JOB_CHANGE:   'job_change',
  SKILL_QUEST:  'skill_quest',
  REFINE:       'refine',
  STYLIST:      'stylist',
  GUIDE:        'guide',
  QUEST:        'quest',
  GUILD:        'guild',
  ARENA:        'arena',
  HEALER:       'healer',
  INN:          'inn',
  GENERIC:      'generic'
};

// ============================================================
// NPC Data Structure
// ============================================================
//
// Every NPC entry has this shape:
//
// {
//   npcId:          number,       // Unique across all NPCs
//   type:           string,       // One of NPC_TYPES
//   name:           string,       // Display name
//   spriteId:       number,       // RO sprite ID (used by client for model selection)
//   zone:           string,       // zone_name matching ZONE_REGISTRY
//   position:       { x, y, z }, // UE world coordinates
//   facing:         number,       // 0-7 clockwise from North
//   interactRadius: number,       // UE units -- max click distance
//   shopId:         string|null,  // Key into SHOP_REGISTRY
//   dialogueId:     string|null,  // Key into DIALOGUE_REGISTRY
//   questId:        string|null,  // Key into QUEST_REGISTRY
//   isActive:       boolean       // false = hidden/disabled
// }

const NPC_REGISTRY = {
  // ---- Prontera Tool Dealer ----
  'prt_tool_dealer': {
    npcId: 1001,
    type: NPC_TYPES.SHOP_TOOL,
    name: 'Tool Dealer',
    spriteId: 83,
    zone: 'prt_fild08',
    position: { x: 134, y: 221, z: 0 },
    facing: 4,
    interactRadius: 300,
    shopId: 'prt_tool',
    dialogueId: 'prt_tool_welcome',
    questId: null,
    isActive: true
  },

  // ---- Prontera Weapon Dealer ----
  'prt_weapon_dealer': {
    npcId: 1002,
    type: NPC_TYPES.SHOP_WEAPON,
    name: 'Weapon Dealer',
    spriteId: 83,
    zone: 'prt_fild08',
    position: { x: 250, y: 180, z: 0 },
    facing: 4,
    interactRadius: 300,
    shopId: 'prt_weapon',
    dialogueId: 'prt_weapon_welcome',
    questId: null,
    isActive: true
  },

  // ---- Swordsman Guild Master ----
  'izlude_swordsman_master': {
    npcId: 2001,
    type: NPC_TYPES.JOB_CHANGE,
    name: 'Master Swordsman',
    spriteId: 104,
    zone: 'izlude',
    position: { x: 100, y: 340, z: 0 },
    facing: 4,
    interactRadius: 300,
    shopId: null,
    dialogueId: 'swordsman_job_change',
    questId: 'quest_swordsman_change',
    isActive: true
  },

  // ---- Prontera Refiner (Hollegren) ----
  'prt_refiner': {
    npcId: 3001,
    type: NPC_TYPES.REFINE,
    name: 'Hollegren',
    spriteId: 71,
    zone: 'prt_fild08',
    position: { x: 400, y: 300, z: 0 },
    facing: 4,
    interactRadius: 300,
    shopId: null,
    dialogueId: 'refine_welcome',
    questId: null,
    isActive: true
  }
  // ... hundreds more NPCs follow the same pattern
};

module.exports = { NPC_TYPES, NPC_REGISTRY };
```

**Server `npc:interact` handler in `index.js`:**

```javascript
const { NPC_REGISTRY } = require('./ro_npc_data');
const { DIALOGUE_REGISTRY } = require('./ro_dialogue_data');

socket.on('npc:interact', ({ npcId }) => {
  const player = players[socket.id];
  if (!player) return;

  const npc = NPC_REGISTRY[npcId];
  if (!npc || !npc.isActive) {
    return socket.emit('npc:error', { message: 'NPC not found.' });
  }

  // Zone check -- NPC must be in the player's current zone
  if (npc.zone !== player.zone_name) {
    return socket.emit('npc:error', { message: 'NPC is not in your zone.' });
  }

  // Range check
  const dx = player.lastX - npc.position.x;
  const dy = player.lastY - npc.position.y;
  const dist = Math.sqrt(dx * dx + dy * dy);
  if (dist > npc.interactRadius) {
    return socket.emit('npc:error', { message: 'Too far from NPC.' });
  }

  // Initialize dialogue state on the socket
  const dialogueId = npc.dialogueId;
  if (dialogueId && DIALOGUE_REGISTRY[dialogueId]) {
    socket.dialogueState = {
      npcId,
      dialogueId,
      currentPage: 0,
      variables: {},
      questContext: npc.questId || null
    };
    sendDialoguePage(socket, player);
  } else {
    socket.emit('npc:error', { message: 'This NPC has nothing to say.' });
  }
});
```

### 1.2 Client NPC Base Class -- `AMMONPC`

This replaces per-type actor classes with a single data-driven NPC actor. The existing `AKafraNPC` remains as a special case; new generic NPCs use `AMMONPC`.

**File: `client/SabriMMO/Source/SabriMMO/MMONPC.h`**

```cpp
// MMONPC.h -- Generic data-driven NPC actor.
// Place in level, set NpcId per instance (must match server NPC_REGISTRY key).
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().
// Routes to the appropriate subsystem based on NPC type.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MMONPC.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ENPCType : uint8
{
    ShopTool,
    ShopWeapon,
    ShopArmor,
    Kafra,
    JobChange,
    SkillQuest,
    Refine,
    Stylist,
    Guide,
    Quest,
    Healer,
    Generic
};

UCLASS()
class SABRIMMO_API AMMONPC : public AActor
{
    GENERATED_BODY()

public:
    AMMONPC();

    // ---- Designer-facing properties (set per instance in level) ----

    /** Must match a key in the server's NPC_REGISTRY. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
    FString NpcId;

    /** Display name shown above the NPC (rendered by WorldHealthBarSubsystem). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
    FString NPCDisplayName = TEXT("NPC");

    /** NPC type -- determines which subsystem handles interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
    ENPCType NPCType = ENPCType::Generic;

    /** Max click distance in UE units. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
    float InteractionRadius = 300.f;

    /** Called from BP_MMOCharacter click chain. */
    UFUNCTION(BlueprintCallable, Category = "NPC")
    void Interact();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UCapsuleComponent* CapsuleComp;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComp;

private:
    double LastInteractTime = 0.0;
};
```

**File: `client/SabriMMO/Source/SabriMMO/MMONPC.cpp`**

```cpp
// MMONPC.cpp -- Generic NPC actor implementation.
// Follows the same collision/click pattern as KafraNPC.

#include "MMONPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UI/DialogueSubsystem.h"
#include "UI/KafraSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMONPC, Log, All);

AMMONPC::AMMONPC()
{
    PrimaryActorTick.bCanEverTick = false;

    // ---- Root capsule (blocks player movement) ----
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    CapsuleComp->InitCapsuleSize(42.f, 96.f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
    CapsuleComp->SetSimulatePhysics(false);
    CapsuleComp->SetEnableGravity(false);
    RootComponent = CapsuleComp;

    // ---- Visual mesh (trace-only for click detection) ----
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        MeshComp->SetStaticMesh(CylinderMesh.Object);
    }
    MeshComp->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
}

void AMMONPC::BeginPlay()
{
    Super::BeginPlay();
}

void AMMONPC::Interact()
{
    // Spam guard -- IA_Attack fires every frame while held
    const double Now = FPlatformTime::Seconds();
    if (Now - LastInteractTime < 1.0)
    {
        return;
    }
    LastInteractTime = Now;

    UWorld* World = GetWorld();
    if (!World) return;

    // Range check
    APlayerController* PC = World->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        const float Distance = FVector::Dist(
            PC->GetPawn()->GetActorLocation(), GetActorLocation());
        if (Distance > InteractionRadius)
        {
            UE_LOG(LogMMONPC, Log, TEXT("Player too far from %s (%.0f > %.0f)"),
                *NPCDisplayName, Distance, InteractionRadius);
            return;
        }
    }

    UE_LOG(LogMMONPC, Log, TEXT("Interact() on %s [%s]"), *NPCDisplayName, *NpcId);

    // Route to Kafra subsystem for Kafra type (backward compat)
    if (NPCType == ENPCType::Kafra)
    {
        if (UKafraSubsystem* KafraSub = World->GetSubsystem<UKafraSubsystem>())
        {
            if (!KafraSub->IsWidgetVisible())
            {
                KafraSub->RequestOpenKafra(NpcId);
            }
        }
        return;
    }

    // All other NPC types route through DialogueSubsystem
    if (UDialogueSubsystem* DialogueSub = World->GetSubsystem<UDialogueSubsystem>())
    {
        if (!DialogueSub->IsDialogueOpen())
        {
            DialogueSub->RequestInteract(NpcId);
        }
    }
}
```

### 1.3 BP_MMOCharacter Click Integration

The existing click chain in BP_MMOCharacter already does a line trace under cursor. Add a cast check for `AMMONPC`:

```
GetHitResultUnderCursor(ECC_Visibility)
  |
  v
Hit Actor -- Cast To AMMONPC --> success --> AMMONPC->Interact()
                              |
                              +--> Cast To AKafraNPC --> success --> AKafraNPC->Interact()
                              |
                              +--> (existing enemy/player handling)
```

In C++ this would be handled in `ASabriMMOCharacter::HandleLeftClickTarget()`:

```cpp
// In the existing click-target resolution logic:
if (AMMONPC* NPC = Cast<AMMONPC>(HitActor))
{
    NPC->Interact();
    return; // Do not start auto-attack
}
```

---

## 2. Dialogue System

### 2.1 JSON Dialogue Tree Schema

Dialogue trees are stored server-side in `server/src/ro_dialogue_data.js`. Each tree is an object with an array of pages.

**File: `server/src/ro_dialogue_data.js`**

```javascript
// ============================================================
// Dialogue Page Schema
// ============================================================
//
// {
//   pageIndex:    number,          // 0-based page number
//   speaker:      string|null,     // NPC name or null for narration
//   portrait:     string|null,     // Portrait asset key (e.g. "kafra_01")
//   text:         string,          // Dialogue text (supports \n, ^RRGGBB color codes)
//   hasNext:      boolean,         // true = show "Next" button
//   choices:      array|null,      // [{ text, nextPage?, action?, params? }]
//   action:       string|null,     // Terminal action: "close", "open_shop", "start_quest",
//                                  //   "change_job", "open_refine", "heal"
//   actionParams: object|null,     // Parameters for the action
//   conditions:   object|null,     // Prerequisites to show this page
//   failPage:     number|null      // Page to jump to if conditions fail
// }
//
// Choice entry schema:
// {
//   text:     string,              // Button label
//   nextPage: number|undefined,    // Jump to this page
//   action:   string|undefined,    // Terminal action instead of page jump
//   params:   object|undefined,    // Action parameters
//   conditions: object|undefined   // Show choice only if met
// }
//
// Condition schema:
// {
//   minBaseLevel:  number|undefined,
//   minJobLevel:   number|undefined,
//   currentClass:  string|undefined,     // "novice", "swordsman", etc.
//   hasItem:       [itemId, qty]|undefined,
//   hasZeny:       number|undefined,
//   questComplete: string|undefined,     // questId that must be completed
//   questActive:   string|undefined,     // questId that must be active
//   questStep:     { questId, step }|undefined
// }

const DIALOGUE_REGISTRY = {

  // ============================================================
  // Tool Dealer -- simple shop greeting
  // ============================================================
  'prt_tool_welcome': {
    pages: [
      {
        pageIndex: 0,
        speaker: 'Tool Dealer',
        portrait: null,
        text: 'Welcome to the Prontera Tool Shop!\nHow may I help you today?',
        hasNext: false,
        choices: [
          { text: 'Buy items',  action: 'open_shop', params: { shopId: 'prt_tool', mode: 'buy' } },
          { text: 'Sell items', action: 'open_shop', params: { shopId: 'prt_tool', mode: 'sell' } },
          { text: 'Leave',      action: 'close' }
        ],
        action: null,
        actionParams: null,
        conditions: null,
        failPage: null
      }
    ]
  },

  // ============================================================
  // Swordsman Job Change -- multi-page with conditions
  // ============================================================
  'swordsman_job_change': {
    pages: [
      {
        pageIndex: 0,
        speaker: 'Master Swordsman',
        portrait: 'swordsman_master',
        text: 'Greetings, adventurer.\nYou wish to walk the path of the sword?',
        hasNext: false,
        choices: [
          { text: 'Yes, I want to become a Swordsman.',  nextPage: 1 },
          { text: 'No, I am just looking around.',        action: 'close' }
        ],
        action: null,
        conditions: null,
        failPage: null
      },
      {
        pageIndex: 1,
        speaker: 'Master Swordsman',
        portrait: 'swordsman_master',
        text: 'To join our guild, you must have reached\n^FF0000Job Level 10^000000 as a Novice\nand allocated all your skill points.',
        hasNext: true,
        choices: null,
        action: null,
        conditions: {
          minJobLevel: 10,
          currentClass: 'novice'
        },
        failPage: 5
      },
      {
        pageIndex: 2,
        speaker: 'Master Swordsman',
        portrait: 'swordsman_master',
        text: 'Very well. Your determination is clear.\nSpeak with the ^0000FFTest Guide^000000 in the next room\nto begin your trial.',
        hasNext: false,
        choices: [
          { text: 'I am ready.',  action: 'start_quest', params: { questId: 'quest_swordsman_change' } },
          { text: 'Let me prepare first.', action: 'close' }
        ],
        action: null,
        conditions: null,
        failPage: null
      },
      // ... page 3: quest completion dialogue
      {
        pageIndex: 3,
        speaker: 'Master Swordsman',
        portrait: 'swordsman_master',
        text: 'You have completed the trial!\nFrom this day forward, you are a ^FF0000Swordsman^000000.\nMay your blade strike true.',
        hasNext: false,
        choices: null,
        action: 'change_job',
        actionParams: { newClass: 'swordsman' },
        conditions: {
          questComplete: 'quest_swordsman_change'
        },
        failPage: 4
      },
      // ... page 4: quest still in progress
      {
        pageIndex: 4,
        speaker: 'Master Swordsman',
        portrait: 'swordsman_master',
        text: 'You have not yet completed the trial.\nReturn when you have passed the test.',
        hasNext: false,
        choices: null,
        action: 'close',
        conditions: null,
        failPage: null
      },
      // ... page 5: fail page (conditions not met)
      {
        pageIndex: 5,
        speaker: 'Master Swordsman',
        portrait: 'swordsman_master',
        text: 'You are not yet ready.\nReturn when you have reached Job Level 10\nas a Novice.',
        hasNext: false,
        choices: null,
        action: 'close',
        conditions: null,
        failPage: null
      }
    ]
  },

  // ============================================================
  // Refine NPC greeting
  // ============================================================
  'refine_welcome': {
    pages: [
      {
        pageIndex: 0,
        speaker: 'Hollegren',
        portrait: 'refiner',
        text: 'I am Hollegren, master of the forge.\nBring me your equipment and the proper ores,\nand I will refine them for you.',
        hasNext: false,
        choices: [
          { text: 'Refine equipment', action: 'open_refine', params: {} },
          { text: 'Tell me about refinement', nextPage: 1 },
          { text: 'Leave', action: 'close' }
        ],
        action: null,
        conditions: null,
        failPage: null
      },
      {
        pageIndex: 1,
        speaker: 'Hollegren',
        portrait: 'refiner',
        text: 'Each refinement attempt requires the correct ore.\n\n^FFFF00Phracon^000000 for Lv1 weapons\n^FFFF00Emveretarcon^000000 for Lv2 weapons\n^FFFF00Oridecon^000000 for Lv3-4 weapons\n^FFFF00Elunium^000000 for all armor\n\nBe warned -- beyond the safety limit,\nfailure will ^FF0000destroy^000000 your equipment!',
        hasNext: false,
        choices: [
          { text: 'I understand. Let us begin.', action: 'open_refine', params: {} },
          { text: 'I need to prepare.', action: 'close' }
        ],
        action: null,
        conditions: null,
        failPage: null
      }
    ]
  }
};

module.exports = { DIALOGUE_REGISTRY };
```

### 2.2 Server Dialogue State Machine

The server tracks dialogue state per-socket in memory (not persisted to DB).

```javascript
// ============================================================
// Dialogue engine -- added to index.js
// ============================================================

// Per-socket state (set on npc:interact, cleared on npc:close):
// socket.dialogueState = {
//   npcId:        string,
//   dialogueId:   string,
//   currentPage:  number,
//   variables:    {},
//   questContext: string|null
// };

function sendDialoguePage(socket, player) {
  const state = socket.dialogueState;
  if (!state) return;

  const dialogue = DIALOGUE_REGISTRY[state.dialogueId];
  if (!dialogue) {
    socket.emit('npc:close', {});
    socket.dialogueState = null;
    return;
  }

  const page = dialogue.pages.find(p => p.pageIndex === state.currentPage);
  if (!page) {
    socket.emit('npc:close', {});
    socket.dialogueState = null;
    return;
  }

  // Evaluate conditions -- if they fail, jump to failPage
  if (page.conditions && !evaluateConditions(page.conditions, player, socket)) {
    if (page.failPage !== null && page.failPage !== undefined) {
      state.currentPage = page.failPage;
      return sendDialoguePage(socket, player);
    }
    // No failPage -- just close
    socket.emit('npc:close', {});
    socket.dialogueState = null;
    return;
  }

  // Filter choices by conditions
  let visibleChoices = null;
  if (page.choices) {
    visibleChoices = page.choices.filter(c => {
      if (!c.conditions) return true;
      return evaluateConditions(c.conditions, player, socket);
    }).map((c, i) => ({
      index: i,
      text: c.text
    }));
  }

  // Send the page to the client
  socket.emit('npc:dialogue', {
    npcId:     state.npcId,
    speaker:   page.speaker,
    portrait:  page.portrait,
    text:      page.text,
    hasNext:   page.hasNext === true,
    choices:   visibleChoices,
    inputType: page.inputType || null  // "text" or "number" for input pages
  });
}

function evaluateConditions(cond, player, socket) {
  if (cond.minBaseLevel && player.base_level < cond.minBaseLevel) return false;
  if (cond.minJobLevel  && player.job_level  < cond.minJobLevel)  return false;
  if (cond.currentClass && player.job_class  !== cond.currentClass) return false;
  if (cond.hasZeny      && player.zeny       < cond.hasZeny)       return false;

  if (cond.hasItem) {
    const [itemId, qty] = cond.hasItem;
    const owned = getInventoryItemCount(player, itemId);
    if (owned < qty) return false;
  }

  if (cond.questComplete) {
    const qp = player.questProgress?.[cond.questComplete];
    if (!qp || qp.status !== 'completed') return false;
  }

  if (cond.questActive) {
    const qp = player.questProgress?.[cond.questActive];
    if (!qp || qp.status !== 'active') return false;
  }

  if (cond.questStep) {
    const { questId, step } = cond.questStep;
    const qp = player.questProgress?.[questId];
    if (!qp || qp.currentStep < step) return false;
  }

  return true;
}

// ---- npc:next -- player clicks "Next" button ----
socket.on('npc:next', () => {
  const state = socket.dialogueState;
  if (!state) return;
  const player = players[socket.id];
  if (!player) return;

  const dialogue = DIALOGUE_REGISTRY[state.dialogueId];
  const page = dialogue?.pages.find(p => p.pageIndex === state.currentPage);
  if (!page || !page.hasNext) return;

  // Advance to next sequential page
  state.currentPage = state.currentPage + 1;
  sendDialoguePage(socket, player);
});

// ---- npc:choice -- player picks a menu option ----
socket.on('npc:choice', ({ choiceIndex }) => {
  const state = socket.dialogueState;
  if (!state) return;
  const player = players[socket.id];
  if (!player) return;

  const dialogue = DIALOGUE_REGISTRY[state.dialogueId];
  const page = dialogue?.pages.find(p => p.pageIndex === state.currentPage);
  if (!page || !page.choices) return;

  // Filter visible choices (same logic as send)
  const visibleChoices = page.choices.filter(c => {
    if (!c.conditions) return true;
    return evaluateConditions(c.conditions, player, socket);
  });

  const choice = visibleChoices[choiceIndex];
  if (!choice) return;

  // Choice leads to an action
  if (choice.action) {
    executeDialogueAction(socket, player, choice.action, choice.params || {});
    return;
  }

  // Choice leads to another page
  if (choice.nextPage !== undefined) {
    state.currentPage = choice.nextPage;
    sendDialoguePage(socket, player);
    return;
  }
});

// ---- npc:close -- player closes dialogue ----
socket.on('npc:close', () => {
  socket.dialogueState = null;
  socket.emit('npc:close', {});
});

// ---- npc:input -- player submits text/number input ----
socket.on('npc:input', ({ value }) => {
  const state = socket.dialogueState;
  if (!state) return;
  const player = players[socket.id];
  if (!player) return;

  // Store the input value in dialogue variables
  state.variables.lastInput = value;

  // Advance to next page (input pages always advance sequentially)
  state.currentPage = state.currentPage + 1;
  sendDialoguePage(socket, player);
});

// ============================================================
// Dialogue Action Executor
// ============================================================

function executeDialogueAction(socket, player, action, params) {
  switch (action) {
    case 'close':
      socket.dialogueState = null;
      socket.emit('npc:close', {});
      break;

    case 'open_shop':
      socket.dialogueState = null;
      socket.emit('npc:close', {});
      openShopForPlayer(socket, player, params.shopId, params.mode || 'buy');
      break;

    case 'start_quest':
      startQuest(socket, player, params.questId);
      socket.dialogueState = null;
      socket.emit('npc:close', {});
      break;

    case 'change_job':
      changePlayerJob(socket, player, params.newClass);
      socket.dialogueState = null;
      socket.emit('npc:close', {});
      break;

    case 'open_refine':
      socket.dialogueState = null;
      socket.emit('npc:close', {});
      openRefineForPlayer(socket, player);
      break;

    case 'heal':
      player.health = player.maxHealth;
      player.mana = player.maxMana;
      broadcastToZone(player.zone_name, 'combat:health_update', {
        characterId: player.character_id,
        health: player.health,
        maxHealth: player.maxHealth,
        mana: player.mana,
        maxMana: player.maxMana
      });
      socket.dialogueState = null;
      socket.emit('npc:close', {});
      break;

    default:
      socket.dialogueState = null;
      socket.emit('npc:close', {});
  }
}
```

### 2.3 Socket Events Summary

| Direction | Event | Payload | When |
|-----------|-------|---------|------|
| Client->Server | `npc:interact` | `{ npcId }` | Player clicks NPC |
| Client->Server | `npc:next` | `{}` | Player clicks "Next" |
| Client->Server | `npc:choice` | `{ choiceIndex }` | Player selects menu option |
| Client->Server | `npc:input` | `{ value }` | Player submits input |
| Client->Server | `npc:close` | `{}` | Player closes dialogue |
| Server->Client | `npc:dialogue` | `{ npcId, speaker, portrait, text, hasNext, choices, inputType }` | Server sends dialogue page |
| Server->Client | `npc:close` | `{}` | Server closes dialogue |
| Server->Client | `npc:error` | `{ message }` | Interaction failed |

### 2.4 Client `UDialogueSubsystem`

**File: `client/SabriMMO/Source/SabriMMO/UI/DialogueSubsystem.h`**

```cpp
// DialogueSubsystem.h -- UWorldSubsystem managing NPC dialogue state,
// socket event wrapping (npc:dialogue/close/error), and SDialogueWidget lifecycle.
// Follows the exact same pattern as KafraSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "DialogueSubsystem.generated.h"

class USocketIOClientComponent;
class SDialogueWidget;

USTRUCT()
struct FDialogueChoice
{
    GENERATED_BODY()

    int32 Index = 0;
    FString Text;
};

UCLASS()
class SABRIMMO_API UDialogueSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Dialogue state (read by widget via lambdas) ----
    FString CurrentNpcId;
    FString CurrentSpeaker;
    FString CurrentPortrait;       // Asset key for portrait texture
    FString CurrentText;           // Full text (may contain ^RRGGBB color codes)
    bool    bHasNext = false;
    TArray<FDialogueChoice> CurrentChoices;
    FString CurrentInputType;      // "" = none, "text", "number"
    FString ErrorMessage;
    double  ErrorExpireTime = 0.0;

    // ---- Typewriter state ----
    FString TypewriterFullText;    // The complete text to reveal
    FString TypewriterVisibleText; // Currently visible portion
    int32   TypewriterIndex = 0;   // Characters revealed so far
    bool    bTypewriterActive = false;
    float   TypewriterSpeed = 0.03f; // Seconds per character

    // ---- Public API ----
    void RequestInteract(const FString& NpcId);
    void SendNext();
    void SendChoice(int32 ChoiceIndex);
    void SendInput(const FString& Value);
    void SendClose();
    void SkipTypewriter();         // Instantly reveal all text
    bool IsDialogueOpen() const;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    // ---- Socket event wrapping ----
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event handlers ----
    void HandleNpcDialogue(const TSharedPtr<FJsonValue>& Data);
    void HandleNpcClose(const TSharedPtr<FJsonValue>& Data);
    void HandleNpcError(const TSharedPtr<FJsonValue>& Data);

    // ---- Widget lifecycle ----
    void ShowWidget();
    void HideWidget();

    // ---- Typewriter timer ----
    void TickTypewriter();

    // ---- State ----
    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;
    FTimerHandle TypewriterTimer;

    TSharedPtr<SDialogueWidget> DialogueWidget;
    TSharedPtr<SWidget> AlignmentWrapper;
    TSharedPtr<SWidget> ViewportOverlay;

    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

**File: `client/SabriMMO/Source/SabriMMO/UI/DialogueSubsystem.cpp`**

```cpp
// DialogueSubsystem.cpp -- NPC dialogue state machine, socket wrapping,
// typewriter text reveal, and SDialogueWidget overlay lifecycle.

#include "DialogueSubsystem.h"
#include "SDialogueWidget.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogue, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UDialogueSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UDialogueSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UDialogueSubsystem::TryWrapSocketEvents),
        0.5f, true
    );

    UE_LOG(LogDialogue, Log, TEXT("DialogueSubsystem started -- waiting for SocketIO bindings..."));
}

void UDialogueSubsystem::Deinitialize()
{
    HideWidget();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
        World->GetTimerManager().ClearTimer(TypewriterTimer);
    }

    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

// ============================================================
// Find SocketIO component (same pattern as KafraSubsystem)
// ============================================================

USocketIOClientComponent* UDialogueSubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
        {
            return Comp;
        }
    }
    return nullptr;
}

// ============================================================
// Event wrapping
// ============================================================

void UDialogueSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;

    TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
    if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

    if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

    CachedSIOComponent = SIOComp;

    WrapSingleEvent(TEXT("npc:dialogue"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleNpcDialogue(D); });
    WrapSingleEvent(TEXT("npc:close"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleNpcClose(D); });
    WrapSingleEvent(TEXT("npc:error"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleNpcError(D); });

    bEventsWrapped = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }

    UE_LOG(LogDialogue, Log, TEXT("DialogueSubsystem -- events wrapped."));
}

void UDialogueSubsystem::WrapSingleEvent(
    const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
    if (!NativeClient.IsValid()) return;

    TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
    FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
    if (Existing)
    {
        OriginalCallback = Existing->Function;
    }

    NativeClient->OnEvent(EventName,
        [OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
        {
            if (OriginalCallback) OriginalCallback(Event, Message);
            if (OurHandler) OurHandler(Message);
        },
        TEXT("/"),
        ESIOThreadOverrideOption::USE_GAME_THREAD
    );
}

// ============================================================
// Event Handlers
// ============================================================

void UDialogueSubsystem::HandleNpcDialogue(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FString Str;
    if (Obj->TryGetStringField(TEXT("npcId"), Str))    CurrentNpcId = Str;
    if (Obj->TryGetStringField(TEXT("speaker"), Str))  CurrentSpeaker = Str;
    if (Obj->TryGetStringField(TEXT("portrait"), Str)) CurrentPortrait = Str;
    if (Obj->TryGetStringField(TEXT("text"), Str))     CurrentText = Str;

    bool bNext = false;
    Obj->TryGetBoolField(TEXT("hasNext"), bNext);
    bHasNext = bNext;

    FString InputType;
    if (Obj->TryGetStringField(TEXT("inputType"), InputType))
    {
        CurrentInputType = InputType;
    }
    else
    {
        CurrentInputType.Empty();
    }

    // Parse choices
    CurrentChoices.Empty();
    const TArray<TSharedPtr<FJsonValue>>* ChoicesArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("choices"), ChoicesArr) && ChoicesArr)
    {
        for (const TSharedPtr<FJsonValue>& ChoiceVal : *ChoicesArr)
        {
            const TSharedPtr<FJsonObject>* ChoiceObj = nullptr;
            if (ChoiceVal.IsValid() && ChoiceVal->TryGetObject(ChoiceObj) && ChoiceObj)
            {
                FDialogueChoice Choice;
                double Idx = 0;
                (*ChoiceObj)->TryGetNumberField(TEXT("index"), Idx);
                Choice.Index = (int32)Idx;
                (*ChoiceObj)->TryGetStringField(TEXT("text"), Choice.Text);
                CurrentChoices.Add(Choice);
            }
        }
    }

    // Start typewriter effect
    TypewriterFullText = CurrentText;
    TypewriterVisibleText.Empty();
    TypewriterIndex = 0;
    bTypewriterActive = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TypewriterTimer,
            FTimerDelegate::CreateUObject(this, &UDialogueSubsystem::TickTypewriter),
            TypewriterSpeed, true
        );
    }

    ErrorMessage.Empty();
    ShowWidget();

    UE_LOG(LogDialogue, Log, TEXT("Dialogue page: [%s] %s"),
        *CurrentSpeaker, *CurrentText.Left(60));
}

void UDialogueSubsystem::HandleNpcClose(const TSharedPtr<FJsonValue>& Data)
{
    CurrentNpcId.Empty();
    CurrentSpeaker.Empty();
    CurrentText.Empty();
    CurrentChoices.Empty();
    bHasNext = false;
    bTypewriterActive = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TypewriterTimer);
    }

    HideWidget();
    UE_LOG(LogDialogue, Log, TEXT("Dialogue closed."));
}

void UDialogueSubsystem::HandleNpcError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FString Msg;
    if ((*ObjPtr)->TryGetStringField(TEXT("message"), Msg))
    {
        ErrorMessage = Msg;
        ErrorExpireTime = FPlatformTime::Seconds() + 4.0;
        UE_LOG(LogDialogue, Warning, TEXT("NPC error: %s"), *Msg);
    }
}

// ============================================================
// Typewriter
// ============================================================

void UDialogueSubsystem::TickTypewriter()
{
    if (!bTypewriterActive) return;

    if (TypewriterIndex >= TypewriterFullText.Len())
    {
        bTypewriterActive = false;
        TypewriterVisibleText = TypewriterFullText;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(TypewriterTimer);
        }
        return;
    }

    // Skip color codes (^RRGGBB = 7 chars)
    while (TypewriterIndex < TypewriterFullText.Len() &&
           TypewriterFullText[TypewriterIndex] == '^')
    {
        // Copy the entire color code block
        int32 CodeEnd = FMath::Min(TypewriterIndex + 7, TypewriterFullText.Len());
        for (int32 i = TypewriterIndex; i < CodeEnd; ++i)
        {
            TypewriterVisibleText.AppendChar(TypewriterFullText[i]);
        }
        TypewriterIndex = CodeEnd;
    }

    if (TypewriterIndex < TypewriterFullText.Len())
    {
        TypewriterVisibleText.AppendChar(TypewriterFullText[TypewriterIndex]);
        TypewriterIndex++;
    }
}

void UDialogueSubsystem::SkipTypewriter()
{
    if (!bTypewriterActive) return;
    bTypewriterActive = false;
    TypewriterVisibleText = TypewriterFullText;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TypewriterTimer);
    }
}

// ============================================================
// Public API
// ============================================================

void UDialogueSubsystem::RequestInteract(const FString& NpcId)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("npcId"), NpcId);

    CachedSIOComponent->EmitNative(TEXT("npc:interact"), Payload);
    UE_LOG(LogDialogue, Log, TEXT("Requesting NPC interact: %s"), *NpcId);
}

void UDialogueSubsystem::SendNext()
{
    if (!CachedSIOComponent.IsValid()) return;

    // If typewriter is still running, skip it first
    if (bTypewriterActive)
    {
        SkipTypewriter();
        return;
    }

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    CachedSIOComponent->EmitNative(TEXT("npc:next"), Payload);
}

void UDialogueSubsystem::SendChoice(int32 ChoiceIndex)
{
    if (!CachedSIOComponent.IsValid()) return;

    // Skip typewriter if still running before allowing choice
    if (bTypewriterActive)
    {
        SkipTypewriter();
    }

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetNumberField(TEXT("choiceIndex"), ChoiceIndex);
    CachedSIOComponent->EmitNative(TEXT("npc:choice"), Payload);
}

void UDialogueSubsystem::SendInput(const FString& Value)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("value"), Value);
    CachedSIOComponent->EmitNative(TEXT("npc:input"), Payload);
}

void UDialogueSubsystem::SendClose()
{
    if (!CachedSIOComponent.IsValid()) return;

    bTypewriterActive = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TypewriterTimer);
    }

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    CachedSIOComponent->EmitNative(TEXT("npc:close"), Payload);
}

bool UDialogueSubsystem::IsDialogueOpen() const
{
    return bWidgetAdded;
}

// ============================================================
// Widget Lifecycle
// ============================================================

void UDialogueSubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    DialogueWidget = SNew(SDialogueWidget).Subsystem(this);

    AlignmentWrapper =
        SNew(SBox)
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [
            DialogueWidget.ToSharedRef()
        ];

    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
    ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 20);
    bWidgetAdded = true;

    // Lock player movement while dialogue is open
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (ACharacter* Char = PC->GetCharacter())
        {
            if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
            {
                MovComp->DisableMovement();
            }
        }
    }

    UE_LOG(LogDialogue, Log, TEXT("Dialogue widget shown (Z=20)."));
}

void UDialogueSubsystem::HideWidget()
{
    if (!bWidgetAdded) return;

    UWorld* World = GetWorld();
    if (World)
    {
        UGameViewportClient* ViewportClient = World->GetGameViewport();
        if (ViewportClient && ViewportOverlay.IsValid())
        {
            ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
        }
    }

    DialogueWidget.Reset();
    AlignmentWrapper.Reset();
    ViewportOverlay.Reset();
    bWidgetAdded = false;

    // Unlock player movement
    if (World)
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ACharacter* Char = PC->GetCharacter())
            {
                if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
                {
                    MovComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
    }

    UE_LOG(LogDialogue, Log, TEXT("Dialogue widget hidden."));
}
```

### 2.5 Client `SDialogueWidget`

**File: `client/SabriMMO/Source/SabriMMO/UI/SDialogueWidget.h`**

```cpp
// SDialogueWidget.h -- Slate widget for NPC dialogue.
// RO Classic themed: speaker name, portrait, scrolling text, choice buttons.
// Typewriter text reveal. Draggable via title bar.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UDialogueSubsystem;
class SVerticalBox;
class STextBlock;
class SEditableTextBox;
class SBox;

class SDialogueWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDialogueWidget) : _Subsystem(nullptr) {}
        SLATE_ARGUMENT(UDialogueSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UDialogueSubsystem> OwningSubsystem;

    // Widget references
    TSharedPtr<STextBlock>       SpeakerText;
    TSharedPtr<STextBlock>       BodyText;
    TSharedPtr<SVerticalBox>     ChoiceContainer;
    TSharedPtr<SBox>             NextButtonBox;
    TSharedPtr<SBox>             InputBox;
    TSharedPtr<SEditableTextBox> InputField;

    // Build methods
    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildPortraitAndText();
    TSharedRef<SWidget> BuildChoiceArea();
    TSharedRef<SWidget> BuildInputArea();
    TSharedRef<SWidget> BuildBottomButtons();
    TSharedRef<SWidget> BuildDialogueButton(const FText& Label, FOnClicked OnClicked);

    // Color code parser: converts "^FF0000Red^000000" to styled text
    FString StripColorCodes(const FString& RawText) const;

    // Drag state
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
    FVector2D WidgetPosition = FVector2D(200.0, 300.0);

    void ApplyLayout();

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
};
```

**File: `client/SabriMMO/Source/SabriMMO/UI/SDialogueWidget.cpp`**

```cpp
// SDialogueWidget.cpp -- NPC dialogue window.
// Layout: Title bar (speaker name + close) | Portrait + text area | Choices | Next/Close buttons
// RO Classic brown/gold theme matching SKafraWidget.

#include "SDialogueWidget.h"
#include "DialogueSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Colors (shared palette)
// ============================================================
namespace DialogueColors
{
    static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
    static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
    static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
    static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
    static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
    static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
    static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
    static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
    static const FLinearColor ButtonBg      (0.33f, 0.22f, 0.13f, 1.f);
    static const FLinearColor ChoiceHover   (0.40f, 0.28f, 0.16f, 1.f);
    static const FLinearColor PortraitBg    (0.18f, 0.12f, 0.07f, 1.f);
}

// ============================================================
// Construct
// ============================================================

void SDialogueWidget::Construct(const FArguments& InArgs)
{
    OwningSubsystem = InArgs._Subsystem;

    ChildSlot
    .HAlign(HAlign_Left)
    .VAlign(VAlign_Top)
    [
        SNew(SBox)
        .WidthOverride(420.f)
        [
            // 3-layer frame: Gold -> Dark -> Brown
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(DialogueColors::GoldTrim)
            .Padding(FMargin(2.f))
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(DialogueColors::PanelDark)
                .Padding(FMargin(1.f))
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                    .BorderBackgroundColor(DialogueColors::PanelBrown)
                    .Padding(FMargin(8.f))
                    [
                        SNew(SVerticalBox)

                        // Title bar (speaker name + close button)
                        + SVerticalBox::Slot().AutoHeight()
                        [ BuildTitleBar() ]

                        // Gold divider
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
                        [
                            SNew(SBox).HeightOverride(1.f)
                            [
                                SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                                .BorderBackgroundColor(DialogueColors::GoldDivider)
                            ]
                        ]

                        // Portrait + text area
                        + SVerticalBox::Slot().AutoHeight()
                        [ BuildPortraitAndText() ]

                        // Choice buttons
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
                        [ BuildChoiceArea() ]

                        // Input area (hidden unless inputType is set)
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
                        [ BuildInputArea() ]

                        // Gold divider
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
                        [
                            SNew(SBox).HeightOverride(1.f)
                            [
                                SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                                .BorderBackgroundColor(DialogueColors::GoldDivider)
                            ]
                        ]

                        // Next / Close buttons
                        + SVerticalBox::Slot().AutoHeight()
                        [ BuildBottomButtons() ]
                    ]
                ]
            ]
        ]
    ];

    ApplyLayout();
}

// ============================================================
// Title Bar
// ============================================================

TSharedRef<SWidget> SDialogueWidget::BuildTitleBar()
{
    UDialogueSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
        [
            SAssignNew(SpeakerText, STextBlock)
            .Text_Lambda([Sub]() -> FText
            {
                if (!Sub || Sub->CurrentSpeaker.IsEmpty())
                    return FText::FromString(TEXT("NPC"));
                return FText::FromString(Sub->CurrentSpeaker);
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
            .ColorAndOpacity(FSlateColor(DialogueColors::GoldHighlight))
            .ShadowOffset(FVector2D(1, 1))
            .ShadowColorAndOpacity(DialogueColors::TextShadow)
        ]

        // Close button
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .ContentPadding(FMargin(4.f, 2.f))
            .OnClicked_Lambda([this]() -> FReply
            {
                if (auto* Sub = OwningSubsystem.Get())
                {
                    Sub->SendClose();
                }
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("X")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FSlateColor(DialogueColors::GoldHighlight))
            ]
        ];
}

// ============================================================
// Portrait + Text Area
// ============================================================

TSharedRef<SWidget> SDialogueWidget::BuildPortraitAndText()
{
    UDialogueSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SHorizontalBox)

        // Portrait placeholder (64x64 dark box)
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
        [
            SNew(SBox)
            .WidthOverride(64.f)
            .HeightOverride(64.f)
            .Visibility_Lambda([Sub]() -> EVisibility
            {
                if (!Sub || Sub->CurrentPortrait.IsEmpty())
                    return EVisibility::Collapsed;
                return EVisibility::Visible;
            })
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(DialogueColors::PortraitBg)
                .Padding(FMargin(2.f))
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    // Portrait texture would be loaded here from CurrentPortrait key.
                    // For now, show the first letter of the speaker name as placeholder.
                    SNew(STextBlock)
                    .Text_Lambda([Sub]() -> FText
                    {
                        if (!Sub || Sub->CurrentSpeaker.IsEmpty())
                            return FText::FromString(TEXT("?"));
                        return FText::FromString(Sub->CurrentSpeaker.Left(1));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                    .ColorAndOpacity(FSlateColor(DialogueColors::GoldHighlight))
                ]
            ]
        ]

        // Text area with scrollbox
        + SHorizontalBox::Slot().FillWidth(1.f)
        [
            SNew(SBox)
            .MinDesiredHeight(80.f)
            .MaxDesiredHeight(160.f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SAssignNew(BodyText, STextBlock)
                    .Text_Lambda([this, Sub]() -> FText
                    {
                        if (!Sub) return FText::GetEmpty();
                        // Show typewriter text if active, else full text
                        const FString& DisplayText = Sub->bTypewriterActive
                            ? Sub->TypewriterVisibleText
                            : Sub->CurrentText;
                        return FText::FromString(StripColorCodes(DisplayText));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                    .ColorAndOpacity(FSlateColor(DialogueColors::TextPrimary))
                    .ShadowOffset(FVector2D(1, 1))
                    .ShadowColorAndOpacity(DialogueColors::TextShadow)
                    .AutoWrapText(true)
                ]
            ]
        ];
}

// ============================================================
// Choice Area
// ============================================================

TSharedRef<SWidget> SDialogueWidget::BuildChoiceArea()
{
    UDialogueSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SBox)
        .Visibility_Lambda([Sub]() -> EVisibility
        {
            if (!Sub || Sub->CurrentChoices.Num() == 0)
                return EVisibility::Collapsed;
            // Hide choices while typewriter is still running
            if (Sub->bTypewriterActive)
                return EVisibility::Collapsed;
            return EVisibility::Visible;
        })
        [
            SAssignNew(ChoiceContainer, SVerticalBox)
        ];

    // Note: Choices are rebuilt dynamically each frame via the visibility lambda
    // and the ChoiceContainer is populated by a Tick or on dialogue page change.
    // A cleaner approach: rebuild in HandleNpcDialogue after typewriter completes.
    // For simplicity, we populate in Construct and the container refreshes via
    // the subsystem's CurrentChoices array using _Lambda bindings.
}

// ============================================================
// Input Area
// ============================================================

TSharedRef<SWidget> SDialogueWidget::BuildInputArea()
{
    UDialogueSubsystem* Sub = OwningSubsystem.Get();

    return SAssignNew(InputBox, SBox)
        .Visibility_Lambda([Sub]() -> EVisibility
        {
            if (!Sub || Sub->CurrentInputType.IsEmpty())
                return EVisibility::Collapsed;
            return EVisibility::Visible;
        })
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
            [
                SAssignNew(InputField, SEditableTextBox)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]

            + SHorizontalBox::Slot().AutoWidth()
            [
                BuildDialogueButton(
                    FText::FromString(TEXT("Submit")),
                    FOnClicked::CreateLambda([this]() -> FReply
                    {
                        if (auto* Sub = OwningSubsystem.Get())
                        {
                            if (InputField.IsValid())
                            {
                                Sub->SendInput(InputField->GetText().ToString());
                            }
                        }
                        return FReply::Handled();
                    })
                )
            ]
        ];
}

// ============================================================
// Bottom Buttons (Next / Close)
// ============================================================

TSharedRef<SWidget> SDialogueWidget::BuildBottomButtons()
{
    UDialogueSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SHorizontalBox)

        // "Next" button -- visible when hasNext is true and typewriter is done
        + SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
        [
            SAssignNew(NextButtonBox, SBox)
            .Visibility_Lambda([Sub]() -> EVisibility
            {
                if (!Sub) return EVisibility::Collapsed;
                // Show Next when hasNext OR when typewriter is still running (click to skip)
                if (Sub->bHasNext || Sub->bTypewriterActive)
                    return EVisibility::Visible;
                return EVisibility::Collapsed;
            })
            [
                BuildDialogueButton(
                    FText::FromString(TEXT("Next")),
                    FOnClicked::CreateLambda([this]() -> FReply
                    {
                        if (auto* Sub = OwningSubsystem.Get())
                        {
                            Sub->SendNext(); // Skips typewriter or advances page
                        }
                        return FReply::Handled();
                    })
                )
            ]
        ]

        // "Close" button -- always visible
        + SHorizontalBox::Slot().AutoWidth()
        [
            BuildDialogueButton(
                FText::FromString(TEXT("Close")),
                FOnClicked::CreateLambda([this]() -> FReply
                {
                    if (auto* Sub = OwningSubsystem.Get())
                    {
                        Sub->SendClose();
                    }
                    return FReply::Handled();
                })
            )
        ];
}

// ============================================================
// Reusable Button Builder (same style as SKafraWidget)
// ============================================================

TSharedRef<SWidget> SDialogueWidget::BuildDialogueButton(
    const FText& Label, FOnClicked OnClicked)
{
    return SNew(SButton)
        .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
        .OnClicked(OnClicked)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(DialogueColors::ButtonBg)
            .Padding(FMargin(10.f, 4.f))
            .HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FSlateColor(DialogueColors::TextPrimary))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(DialogueColors::TextShadow)
            ]
        ];
}

// ============================================================
// Color Code Stripper
// ============================================================

FString SDialogueWidget::StripColorCodes(const FString& RawText) const
{
    // RO color codes: ^RRGGBB (7 characters total including ^)
    // For basic Slate STextBlock we strip them. A Rich Text Block could render them.
    FString Result;
    Result.Reserve(RawText.Len());

    for (int32 i = 0; i < RawText.Len(); ++i)
    {
        if (RawText[i] == '^' && i + 6 < RawText.Len())
        {
            // Check if next 6 chars are hex digits
            bool bIsColorCode = true;
            for (int32 j = 1; j <= 6; ++j)
            {
                TCHAR C = RawText[i + j];
                if (!FChar::IsHexDigit(C))
                {
                    bIsColorCode = false;
                    break;
                }
            }
            if (bIsColorCode)
            {
                i += 6; // Skip the 6 hex digits (loop will increment past ^)
                continue;
            }
        }
        Result.AppendChar(RawText[i]);
    }
    return Result;
}

// ============================================================
// Drag support (title bar only -- same pattern as SKafraWidget)
// ============================================================

FReply SDialogueWidget::OnMouseButtonDown(
    const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return FReply::Unhandled();

    const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);

    // Title bar is top ~28px
    if (LocalPos.Y < 28.f)
    {
        bIsDragging = true;
        DragOffset = ScreenPos;
        DragStartWidgetPos = WidgetPosition;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    return FReply::Unhandled();
}

FReply SDialogueWidget::OnMouseButtonUp(
    const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        bIsDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SDialogueWidget::OnMouseMove(
    const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
        const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f)
            ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
        WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
        ApplyLayout();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void SDialogueWidget::ApplyLayout()
{
    const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
    SetRenderTransform(FSlateRenderTransform(Pos));
}
```

---

## 3. Shop System

### 3.1 Server Shop Registry

**File: `server/src/ro_shop_data.js`**

```javascript
// ============================================================
// Shop Registry -- each shop is an array of purchasable items
// ============================================================
//
// buy_price:  NPC buy price (what the player pays). null = cannot buy.
// sell_price: Auto-calculated as floor(buy_price / 2) unless overridden.
// stock:      -1 = unlimited, positive = limited quantity (restocks daily).

const SHOP_REGISTRY = {
  'prt_tool': [
    { itemId: 1750, buyPrice: 1,     stock: -1 },  // Arrow
    { itemId: 501,  buyPrice: 10,    stock: -1 },  // Red Potion
    { itemId: 502,  buyPrice: 50,    stock: -1 },  // Orange Potion
    { itemId: 503,  buyPrice: 180,   stock: -1 },  // Yellow Potion
    { itemId: 504,  buyPrice: 1200,  stock: -1 },  // White Potion
    { itemId: 506,  buyPrice: 40,    stock: -1 },  // Green Potion
    { itemId: 645,  buyPrice: 800,   stock: -1 },  // Concentration Potion
    { itemId: 656,  buyPrice: 1500,  stock: -1 },  // Awakening Potion
    { itemId: 611,  buyPrice: 40,    stock: -1 },  // Magnifier
    { itemId: 601,  buyPrice: 60,    stock: -1 },  // Fly Wing
    { itemId: 602,  buyPrice: 300,   stock: -1 },  // Butterfly Wing
    { itemId: 1065, buyPrice: 100,   stock: -1 },  // Trap
  ],

  'prt_weapon': [
    { itemId: 1201, buyPrice: 50,      stock: -1 },  // Knife [3]
    { itemId: 1101, buyPrice: 101,     stock: -1 },  // Sword [3]
    { itemId: 1701, buyPrice: 1000,    stock: -1 },  // Bow [3]
    { itemId: 1601, buyPrice: 50,      stock: -1 },  // Rod [3]
    { itemId: 1204, buyPrice: 1250,    stock: -1 },  // Cutter [3]
    { itemId: 1104, buyPrice: 1500,    stock: -1 },  // Falchion [3]
    { itemId: 1116, buyPrice: 2000,    stock: -1 },  // Katana [3]
    { itemId: 1207, buyPrice: 2400,    stock: -1 },  // Main Gauche [3]
    { itemId: 1107, buyPrice: 2900,    stock: -1 },  // Blade [3]
    { itemId: 1110, buyPrice: 10000,   stock: -1 },  // Rapier [2]
    { itemId: 1113, buyPrice: 17000,   stock: -1 },  // Scimiter [2]
    { itemId: 1122, buyPrice: 24000,   stock: -1 },  // Ring Pommel Saber [2]
    { itemId: 1126, buyPrice: 49000,   stock: -1 },  // Saber [2]
    { itemId: 1301, buyPrice: 500,     stock: -1 },  // Axe [3]
  ],

  'prt_armor': [
    { itemId: 2301, buyPrice: 10,      stock: -1 },  // Cotton Shirt
    { itemId: 2303, buyPrice: 200,     stock: -1 },  // Jacket
    { itemId: 2401, buyPrice: 400,     stock: -1 },  // Sandals
    { itemId: 2101, buyPrice: 500,     stock: -1 },  // Guard
    { itemId: 2220, buyPrice: 1000,    stock: -1 },  // Hat
    { itemId: 2501, buyPrice: 1000,    stock: -1 },  // Hood
    { itemId: 2305, buyPrice: 1000,    stock: -1 },  // Adventurer's Suit
    { itemId: 2403, buyPrice: 3500,    stock: -1 },  // Shoes
    { itemId: 2503, buyPrice: 5000,    stock: -1 },  // Muffler
    { itemId: 2328, buyPrice: 5500,    stock: -1 },  // Wooden Mail
    { itemId: 2307, buyPrice: 10000,   stock: -1 },  // Mantle
    { itemId: 2226, buyPrice: 12000,   stock: -1 },  // Cap
    { itemId: 2103, buyPrice: 14000,   stock: -1 },  // Buckler
    { itemId: 2309, buyPrice: 22000,   stock: -1 },  // Coat
    { itemId: 2312, buyPrice: 48000,   stock: -1 },  // Padded Armor
    { itemId: 2314, buyPrice: 65000,   stock: -1 },  // Chain Mail
  ]
};

module.exports = { SHOP_REGISTRY };
```

### 3.2 Server Shop Transaction Handlers

```javascript
// ============================================================
// Shop open -- sends item catalog to client
// ============================================================

function openShopForPlayer(socket, player, shopId, mode) {
  const shopItems = SHOP_REGISTRY[shopId];
  if (!shopItems) {
    return socket.emit('shop:error', { message: 'Shop not found.' });
  }

  // Calculate Discount level for buy prices
  const discountLevel = getSkillLevel(player, 'MC_DISCOUNT') || 0;
  const discountRate = discountLevel > 0
    ? Math.min(24, 5 + discountLevel * 2) / 100
    : 0;

  // Calculate Overcharge level for sell prices
  const overchargeLevel = getSkillLevel(player, 'MC_OVERCHARGE') || 0;
  const overchargeRate = overchargeLevel > 0
    ? Math.min(24, 5 + overchargeLevel * 2) / 100
    : 0;

  const catalogItems = shopItems.map(si => {
    const itemData = ro_item_mapping[si.itemId];
    if (!itemData) return null;

    const baseBuyPrice = si.buyPrice || itemData.buy_price || 0;
    const baseSellPrice = Math.floor(baseBuyPrice / 2);

    return {
      itemId:        si.itemId,
      name:          itemData.name,
      description:   itemData.description || '',
      itemType:      itemData.item_type || 'etc',
      icon:          itemData.icon || '',
      buyPrice:      Math.floor(baseBuyPrice * (1 - discountRate)),
      sellPrice:     Math.floor(baseSellPrice * (1 + overchargeRate)),
      weight:        itemData.weight || 0,
      atk:           itemData.atk || 0,
      def:           itemData.def || 0,
      matk:          itemData.matk || 0,
      mdef:          itemData.mdef || 0,
      equipSlot:     itemData.equip_slot || '',
      weaponType:    itemData.weapon_type || '',
      weaponRange:   itemData.weapon_range || 0,
      aspdModifier:  itemData.aspd_modifier || 0,
      requiredLevel: itemData.required_level || 1,
      stackable:     itemData.stackable || false,
      stock:         si.stock
    };
  }).filter(Boolean);

  socket.emit('shop:data', {
    shopId,
    mode,           // 'buy' or 'sell'
    npcName:        'Shop',
    items:          catalogItems,
    playerZeny:     player.zeny,
    discountLevel,
    overchargeLevel
  });
}

// ============================================================
// shop:buy -- player purchases items from NPC
// ============================================================

socket.on('shop:buy', ({ shopId, items }) => {
  const player = players[socket.id];
  if (!player) return;

  const shopItems = SHOP_REGISTRY[shopId];
  if (!shopItems) {
    return socket.emit('shop:error', { message: 'Invalid shop.' });
  }

  const discountLevel = getSkillLevel(player, 'MC_DISCOUNT') || 0;
  const discountRate = discountLevel > 0
    ? Math.min(24, 5 + discountLevel * 2) / 100
    : 0;

  let totalCost = 0;
  let totalWeight = 0;
  const purchasedItems = [];

  for (const { itemId, quantity } of items) {
    if (quantity < 1 || quantity > 30000) continue;

    const shopItem = shopItems.find(si => si.itemId === itemId);
    if (!shopItem) continue;

    const itemData = ro_item_mapping[itemId];
    if (!itemData) continue;

    const baseBuyPrice = shopItem.buyPrice || itemData.buy_price || 0;
    const unitPrice = Math.floor(baseBuyPrice * (1 - discountRate));
    totalCost += unitPrice * quantity;
    totalWeight += (itemData.weight || 0) * quantity;

    purchasedItems.push({ itemId, quantity, unitPrice, name: itemData.name });
  }

  // Validate zeny
  if (player.zeny < totalCost) {
    return socket.emit('shop:error', { message: 'Not enough Zeny.' });
  }

  // Validate weight
  const currentWeight = calculatePlayerWeight(player);
  const maxWeight = 2000 + (player.str || 1) * 30;
  if (currentWeight + totalWeight > maxWeight) {
    return socket.emit('shop:error', { message: 'You are carrying too much.' });
  }

  // Execute transaction
  player.zeny -= totalCost;
  for (const { itemId, quantity } of purchasedItems) {
    addItemToInventory(player, itemId, quantity);
  }

  // Persist zeny to DB
  updateCharacterZeny(player.character_id, player.zeny);

  socket.emit('shop:bought', {
    success: true,
    items: purchasedItems,
    totalCost,
    newZuzucoin: player.zeny
  });

  // Refresh inventory
  sendInventoryData(socket, player);
});

// ============================================================
// shop:sell -- player sells items to NPC
// ============================================================

socket.on('shop:sell', ({ items }) => {
  const player = players[socket.id];
  if (!player) return;

  const overchargeLevel = getSkillLevel(player, 'MC_OVERCHARGE') || 0;
  const overchargeRate = overchargeLevel > 0
    ? Math.min(24, 5 + overchargeLevel * 2) / 100
    : 0;

  let totalEarned = 0;
  const soldItems = [];

  for (const { inventoryId, quantity } of items) {
    if (quantity < 1) continue;

    const invItem = player.inventory?.find(i => i.inventory_id === inventoryId);
    if (!invItem) continue;
    if (invItem.is_equipped) continue; // Cannot sell equipped items
    if (invItem.quantity < quantity) continue;

    const itemData = ro_item_mapping[invItem.item_id];
    if (!itemData) continue;

    const baseSellPrice = itemData.sell_price || Math.floor((itemData.buy_price || 0) / 2);
    if (baseSellPrice <= 0) continue; // Quest items with no sell value

    const unitPrice = Math.floor(baseSellPrice * (1 + overchargeRate));
    totalEarned += unitPrice * quantity;

    removeItemFromInventory(player, invItem.item_id, quantity);
    soldItems.push({ inventoryId, itemId: invItem.item_id, quantity, unitPrice, name: itemData.name });
  }

  player.zeny += totalEarned;
  updateCharacterZeny(player.character_id, player.zeny);

  socket.emit('shop:sold', {
    success: true,
    items: soldItems,
    totalEarned,
    newZuzucoin: player.zeny
  });

  sendInventoryData(socket, player);
});
```

### 3.3 Client `UShopSubsystem`

**File: `client/SabriMMO/Source/SabriMMO/UI/ShopSubsystem.h`**

```cpp
// ShopSubsystem.h -- UWorldSubsystem managing NPC shop state,
// socket events (shop:data/bought/sold/error), cart management, and SShopWidget lifecycle.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"          // FShopItem, FCartItem
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "ShopSubsystem.generated.h"

class USocketIOClientComponent;
class SShopWidget;

UENUM()
enum class EShopMode : uint8
{
    Buy,
    Sell
};

UCLASS()
class SABRIMMO_API UShopSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Shop state (read by widget) ----
    FString CurrentShopId;
    FString ShopNpcName;
    EShopMode CurrentMode = EShopMode::Buy;
    TArray<FShopItem> ShopCatalog;      // NPC's item list (buy mode)
    TArray<FCartItem> BuyCart;           // Player's buy cart
    TArray<FCartItem> SellCart;          // Player's sell cart
    int32 PlayerZeny = 0;
    int32 DiscountLevel = 0;
    int32 OverchargeLevel = 0;
    FString StatusMessage;
    double StatusExpireTime = 0.0;

    // ---- Cart API ----
    void AddToBuyCart(int32 ItemId, int32 Quantity);
    void RemoveFromBuyCart(int32 ItemId);
    void AddToSellCart(int32 InventoryId, int32 Quantity);
    void RemoveFromSellCart(int32 InventoryId);
    int32 GetBuyCartTotal() const;
    int32 GetSellCartTotal() const;
    int32 GetBuyCartWeight() const;

    // ---- Transaction API ----
    void ConfirmBuy();
    void ConfirmSell();
    void CloseShop();
    void SwitchMode(EShopMode NewMode);

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandleShopData(const TSharedPtr<FJsonValue>& Data);
    void HandleShopBought(const TSharedPtr<FJsonValue>& Data);
    void HandleShopSold(const TSharedPtr<FJsonValue>& Data);
    void HandleShopError(const TSharedPtr<FJsonValue>& Data);

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    TSharedPtr<SShopWidget> ShopWidget;
    TSharedPtr<SWidget> AlignmentWrapper;
    TSharedPtr<SWidget> ViewportOverlay;

    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 3.4 Client `SShopWidget`

**File: `client/SabriMMO/Source/SabriMMO/UI/SShopWidget.h`**

```cpp
// SShopWidget.h -- Slate widget for NPC shop interface.
// Two tabs: Buy (NPC catalog) and Sell (player inventory).
// Item grid, quantity selector, running total, Zeny display.
// RO Classic brown/gold theme. Draggable via title bar.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UShopSubsystem;
class SVerticalBox;
class SScrollBox;

class SShopWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SShopWidget) : _Subsystem(nullptr) {}
        SLATE_ARGUMENT(UShopSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void RebuildItemList();
    void RebuildCartDisplay();

private:
    TWeakObjectPtr<UShopSubsystem> OwningSubsystem;

    // Widget references
    TSharedPtr<SVerticalBox>  ItemListContainer;
    TSharedPtr<SVerticalBox>  CartContainer;
    TSharedPtr<SWidget>       BuyTabContent;
    TSharedPtr<SWidget>       SellTabContent;

    // Build methods
    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildTabButtons();
    TSharedRef<SWidget> BuildItemGrid();
    TSharedRef<SWidget> BuildCartSection();
    TSharedRef<SWidget> BuildTotalRow();
    TSharedRef<SWidget> BuildShopButton(const FText& Label, FOnClicked OnClicked);
    TSharedRef<SWidget> BuildItemRow(int32 ItemId, const FString& Name,
        int32 Price, int32 Weight, bool bIsBuyMode);

    // Drag state
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
    FVector2D WidgetPosition = FVector2D(400.0, 150.0);

    void ApplyLayout();

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
};
```

The `SShopWidget.cpp` follows the same construction pattern as `SDialogueWidget.cpp` and `SKafraWidget.cpp`:

- 3-layer gold/dark/brown border frame
- Title bar with shop name + close button
- Buy/Sell tab toggle buttons
- Scrollable item list with name, price, weight columns
- Quantity +/- buttons per item row
- Cart section showing selected items
- Running total at bottom (total price, total weight)
- Confirm button and Zeny display
- Draggable via title bar (same 28px region pattern)

### 3.5 Shop Socket Events Summary

| Direction | Event | Payload |
|-----------|-------|---------|
| Server->Client | `shop:data` | `{ shopId, mode, npcName, items[], playerZeny, discountLevel, overchargeLevel }` |
| Client->Server | `shop:buy` | `{ shopId, items: [{ itemId, quantity }] }` |
| Client->Server | `shop:sell` | `{ items: [{ inventoryId, quantity }] }` |
| Server->Client | `shop:bought` | `{ success, items[], totalCost, newZuzucoin }` |
| Server->Client | `shop:sold` | `{ success, items[], totalEarned, newZuzucoin }` |
| Server->Client | `shop:error` | `{ message }` |

### 3.6 Discount / Overcharge Formulas

```
Discount Rate = min(24, 5 + DiscountLevel * 2) / 100
  -> Level 1:  7%,  Level 5: 15%,  Level 10: 24%
  -> Effective buy price = floor(base_price * (1 - rate))

Overcharge Rate = min(24, 5 + OverchargeLevel * 2) / 100
  -> Level 1:  7%,  Level 5: 15%,  Level 10: 24%
  -> Effective sell price = floor(base_sell * (1 + rate))

Base sell price = floor(buy_price / 2)
```

---

## 4. Quest System -- Server

### 4.1 Database Schema

**File: `database/migrations/add_quest_system.sql`**

```sql
-- ============================================================
-- Quest definitions (static, loaded at server start)
-- ============================================================

CREATE TABLE IF NOT EXISTS quests (
    quest_id        VARCHAR(64) PRIMARY KEY,
    quest_type      VARCHAR(32) NOT NULL,     -- 'job_change','headgear','access','bounty','story','platinum_skill'
    name            VARCHAR(128) NOT NULL,
    description     TEXT,
    min_base_level  INTEGER NOT NULL DEFAULT 0,
    min_job_level   INTEGER NOT NULL DEFAULT 0,
    required_class  VARCHAR(32),              -- NULL = any class
    is_repeatable   BOOLEAN NOT NULL DEFAULT FALSE,
    cooldown_hours  INTEGER NOT NULL DEFAULT 0,
    rewards_json    JSONB NOT NULL DEFAULT '{}',
    steps_json      JSONB NOT NULL DEFAULT '[]',
    created_at      TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- Per-character quest progress
-- ============================================================

CREATE TABLE IF NOT EXISTS quest_progress (
    character_id    INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    quest_id        VARCHAR(64) NOT NULL REFERENCES quests(quest_id),
    status          VARCHAR(16) NOT NULL DEFAULT 'active',  -- 'active','completed','failed','abandoned'
    current_step    INTEGER NOT NULL DEFAULT 0,
    progress_json   JSONB NOT NULL DEFAULT '{}',
    started_at      TIMESTAMP DEFAULT NOW(),
    completed_at    TIMESTAMP,
    last_completed_at TIMESTAMP,              -- for repeatable quests
    PRIMARY KEY (character_id, quest_id)
);

CREATE INDEX IF NOT EXISTS idx_quest_progress_character ON quest_progress(character_id);
CREATE INDEX IF NOT EXISTS idx_quest_progress_status ON quest_progress(character_id, status);
```

### 4.2 Quest Data Structure

**File: `server/src/ro_quest_data.js`**

```javascript
// ============================================================
// Quest Step Types
// ============================================================

const QUEST_STEP_TYPES = {
  KILL:     'kill',       // Kill N monsters of a specific type
  COLLECT:  'collect',    // Collect N of a specific item
  TALK:     'talk',       // Talk to a specific NPC
  REACH:    'reach',      // Enter a specific zone
  DELIVER:  'deliver',    // Have specific item + talk to NPC
  SURVIVE:  'survive',    // Remain in zone for N seconds without dying
  CUSTOM:   'custom'      // Server-side flag set by special event handlers
};

// ============================================================
// Quest Step Schema
// ============================================================
//
// {
//   stepIndex:    number,
//   type:         string,          // One of QUEST_STEP_TYPES
//   description:  string,          // Shown in quest log
//   target:       string|number,   // Monster template ID, item ID, NPC ID, or zone name
//   quantity:     number,          // How many (kills, items, etc.)
//   zone:         string|null,     // Required zone for this step (null = any)
//   onComplete:   string|null      // Action on step complete: 'next_step', 'complete_quest'
// }

// ============================================================
// Reward Schema
// ============================================================
//
// {
//   baseExp:   number|undefined,
//   jobExp:    number|undefined,
//   zeny:      number|undefined,
//   items:     [{ itemId, quantity }]|undefined,
//   changeJob: string|undefined,     // New class name for job change quests
//   unlockZone: string|undefined,    // Zone access granted
//   learnSkill: number|undefined     // Skill ID granted
// }

const QUEST_REGISTRY = {

  // ============================================================
  // Swordsman Job Change Quest
  // ============================================================
  'quest_swordsman_change': {
    questId: 'quest_swordsman_change',
    questType: 'job_change',
    name: 'Path of the Swordsman',
    description: 'Prove your worth to the Swordsman Guild in Izlude.',
    minBaseLevel: 1,
    minJobLevel: 10,
    requiredClass: 'novice',
    isRepeatable: false,
    cooldownHours: 0,
    rewards: {
      changeJob: 'swordsman',
      baseExp: 0,
      jobExp: 0,
      zeny: 0,
      items: []
    },
    steps: [
      {
        stepIndex: 0,
        type: QUEST_STEP_TYPES.TALK,
        description: 'Speak with the Test Guide in the Swordsman Guild.',
        target: 'izlude_test_guide',
        quantity: 1,
        zone: 'izlude',
        onComplete: 'next_step'
      },
      {
        stepIndex: 1,
        type: QUEST_STEP_TYPES.REACH,
        description: 'Complete the obstacle course.',
        target: 'izlude_training_ground',
        quantity: 1,
        zone: 'izlude_training_ground',
        onComplete: 'next_step'
      },
      {
        stepIndex: 2,
        type: QUEST_STEP_TYPES.TALK,
        description: 'Return to the Master Swordsman.',
        target: 'izlude_swordsman_master',
        quantity: 1,
        zone: 'izlude',
        onComplete: 'complete_quest'
      }
    ]
  },

  // ============================================================
  // Prontera Bounty Board (Repeatable)
  // ============================================================
  'bounty_prt_poring': {
    questId: 'bounty_prt_poring',
    questType: 'bounty',
    name: 'Poring Extermination',
    description: 'Eliminate 150 Porings in the Prontera fields.',
    minBaseLevel: 1,
    minJobLevel: 1,
    requiredClass: null,
    isRepeatable: true,
    cooldownHours: 24,
    rewards: {
      baseExp: 5000,
      jobExp: 2500,
      zeny: 1500,
      items: []
    },
    steps: [
      {
        stepIndex: 0,
        type: QUEST_STEP_TYPES.KILL,
        description: 'Kill 150 Porings.',
        target: 1002,         // Poring monster template ID
        quantity: 150,
        zone: null,           // Any zone
        onComplete: 'complete_quest'
      }
    ]
  },

  // ============================================================
  // Item collection quest example
  // ============================================================
  'quest_headgear_sunglasses': {
    questId: 'quest_headgear_sunglasses',
    questType: 'headgear',
    name: 'Sunglasses Crafting',
    description: 'Collect materials to craft Sunglasses.',
    minBaseLevel: 1,
    minJobLevel: 1,
    requiredClass: null,
    isRepeatable: false,
    cooldownHours: 0,
    rewards: {
      items: [{ itemId: 2201, quantity: 1 }]  // Sunglasses
    },
    steps: [
      {
        stepIndex: 0,
        type: QUEST_STEP_TYPES.COLLECT,
        description: 'Collect 1 One-Carat Diamond and 50 Feathers.',
        target: [
          { itemId: 721, quantity: 1 },    // One-Carat Diamond
          { itemId: 949, quantity: 50 }    // Feather
        ],
        quantity: 1,
        zone: null,
        onComplete: 'next_step'
      },
      {
        stepIndex: 1,
        type: QUEST_STEP_TYPES.TALK,
        description: 'Bring materials to the Sunglasses Trader in Alberta.',
        target: 'alberta_sunglasses_trader',
        quantity: 1,
        zone: 'alberta',
        onComplete: 'complete_quest'
      }
    ]
  }
};

module.exports = { QUEST_STEP_TYPES, QUEST_REGISTRY };
```

### 4.3 Server Quest Engine

```javascript
// ============================================================
// Quest engine -- added to index.js
// ============================================================

const { QUEST_REGISTRY, QUEST_STEP_TYPES } = require('./ro_quest_data');

// Load quest progress from DB into player object on join
async function loadQuestProgress(player) {
  const result = await pool.query(
    'SELECT quest_id, status, current_step, progress_json, started_at, completed_at, last_completed_at FROM quest_progress WHERE character_id = $1',
    [player.character_id]
  );
  player.questProgress = {};
  for (const row of result.rows) {
    player.questProgress[row.quest_id] = {
      status: row.status,
      currentStep: row.current_step,
      progress: row.progress_json || {},
      startedAt: row.started_at,
      completedAt: row.completed_at,
      lastCompletedAt: row.last_completed_at
    };
  }
}

// ---- Start a quest ----
function startQuest(socket, player, questId) {
  const quest = QUEST_REGISTRY[questId];
  if (!quest) {
    return socket.emit('quest:error', { message: 'Quest not found.' });
  }

  // Check if already active or completed (non-repeatable)
  const existing = player.questProgress?.[questId];
  if (existing) {
    if (existing.status === 'active') {
      return socket.emit('quest:error', { message: 'Quest already active.' });
    }
    if (existing.status === 'completed' && !quest.isRepeatable) {
      return socket.emit('quest:error', { message: 'Quest already completed.' });
    }
    if (existing.status === 'completed' && quest.isRepeatable && quest.cooldownHours > 0) {
      const lastDone = new Date(existing.lastCompletedAt || existing.completedAt);
      const cooldownEnd = new Date(lastDone.getTime() + quest.cooldownHours * 3600000);
      if (new Date() < cooldownEnd) {
        return socket.emit('quest:error', { message: 'Quest on cooldown.' });
      }
    }
  }

  // Check prerequisites
  if (quest.minBaseLevel > 0 && (player.base_level || 1) < quest.minBaseLevel) {
    return socket.emit('quest:error', { message: `Requires Base Level ${quest.minBaseLevel}.` });
  }
  if (quest.minJobLevel > 0 && (player.job_level || 1) < quest.minJobLevel) {
    return socket.emit('quest:error', { message: `Requires Job Level ${quest.minJobLevel}.` });
  }
  if (quest.requiredClass && player.job_class !== quest.requiredClass) {
    return socket.emit('quest:error', { message: `Requires class: ${quest.requiredClass}.` });
  }

  // Initialize progress
  const progress = {
    status: 'active',
    currentStep: 0,
    progress: {},
    startedAt: new Date().toISOString(),
    completedAt: null,
    lastCompletedAt: existing?.lastCompletedAt || null
  };

  // Initialize step-specific counters
  const firstStep = quest.steps[0];
  if (firstStep.type === QUEST_STEP_TYPES.KILL) {
    progress.progress.killCount = 0;
  }
  if (firstStep.type === QUEST_STEP_TYPES.COLLECT) {
    progress.progress.collectedItems = {};
  }

  player.questProgress = player.questProgress || {};
  player.questProgress[questId] = progress;

  // Persist to DB
  pool.query(
    `INSERT INTO quest_progress (character_id, quest_id, status, current_step, progress_json, started_at)
     VALUES ($1, $2, 'active', 0, $3, NOW())
     ON CONFLICT (character_id, quest_id)
     DO UPDATE SET status = 'active', current_step = 0, progress_json = $3, started_at = NOW(), completed_at = NULL`,
    [player.character_id, questId, JSON.stringify(progress.progress)]
  );

  socket.emit('quest:started', {
    questId,
    name: quest.name,
    description: quest.description,
    steps: quest.steps.map(s => ({
      stepIndex: s.stepIndex,
      type: s.type,
      description: s.description,
      quantity: s.quantity
    })),
    currentStep: 0,
    progress: progress.progress
  });

  console.log(`[Quest] ${player.name} started quest: ${quest.name}`);
}

// ---- Update quest progress on monster kill ----
function updateQuestKillProgress(player, socket, monsterTemplateId) {
  if (!player.questProgress) return;

  for (const [questId, qp] of Object.entries(player.questProgress)) {
    if (qp.status !== 'active') continue;

    const quest = QUEST_REGISTRY[questId];
    if (!quest) continue;

    const step = quest.steps[qp.currentStep];
    if (!step || step.type !== QUEST_STEP_TYPES.KILL) continue;
    if (step.target !== monsterTemplateId) continue;

    qp.progress.killCount = (qp.progress.killCount || 0) + 1;

    // Emit progress update
    socket.emit('quest:progress', {
      questId,
      currentStep: qp.currentStep,
      progress: qp.progress,
      stepDescription: step.description,
      current: qp.progress.killCount,
      required: step.quantity
    });

    // Check step completion
    if (qp.progress.killCount >= step.quantity) {
      advanceQuestStep(socket, player, questId);
    }

    // Persist progress
    pool.query(
      'UPDATE quest_progress SET progress_json = $1, current_step = $2 WHERE character_id = $3 AND quest_id = $4',
      [JSON.stringify(qp.progress), qp.currentStep, player.character_id, questId]
    );
  }
}

// ---- Update quest progress on item pickup ----
function updateQuestCollectProgress(player, socket, itemId, newQuantity) {
  if (!player.questProgress) return;

  for (const [questId, qp] of Object.entries(player.questProgress)) {
    if (qp.status !== 'active') continue;

    const quest = QUEST_REGISTRY[questId];
    if (!quest) continue;

    const step = quest.steps[qp.currentStep];
    if (!step || step.type !== QUEST_STEP_TYPES.COLLECT) continue;

    // step.target is an array of { itemId, quantity } for collect steps
    const targets = Array.isArray(step.target) ? step.target : [{ itemId: step.target, quantity: step.quantity }];
    const isRelevant = targets.some(t => t.itemId === itemId);
    if (!isRelevant) continue;

    // Check if ALL required items are collected
    let allCollected = true;
    for (const t of targets) {
      const owned = getInventoryItemCount(player, t.itemId);
      if (owned < t.quantity) {
        allCollected = false;
        break;
      }
    }

    socket.emit('quest:progress', {
      questId,
      currentStep: qp.currentStep,
      progress: qp.progress,
      stepDescription: step.description,
      current: allCollected ? 1 : 0,
      required: 1
    });

    if (allCollected && step.onComplete === 'complete_quest') {
      // Do NOT auto-complete collect quests -- require NPC turn-in
    }
  }
}

// ---- Advance to next step or complete quest ----
function advanceQuestStep(socket, player, questId) {
  const quest = QUEST_REGISTRY[questId];
  const qp = player.questProgress[questId];
  if (!quest || !qp) return;

  const step = quest.steps[qp.currentStep];
  if (!step) return;

  if (step.onComplete === 'complete_quest') {
    completeQuest(socket, player, questId);
    return;
  }

  // Advance to next step
  qp.currentStep += 1;
  qp.progress = {}; // Reset step-specific progress

  const nextStep = quest.steps[qp.currentStep];
  if (nextStep) {
    if (nextStep.type === QUEST_STEP_TYPES.KILL) {
      qp.progress.killCount = 0;
    }
    if (nextStep.type === QUEST_STEP_TYPES.COLLECT) {
      qp.progress.collectedItems = {};
    }

    socket.emit('quest:progress', {
      questId,
      currentStep: qp.currentStep,
      progress: qp.progress,
      stepDescription: nextStep.description,
      current: 0,
      required: nextStep.quantity
    });
  }

  pool.query(
    'UPDATE quest_progress SET current_step = $1, progress_json = $2 WHERE character_id = $3 AND quest_id = $4',
    [qp.currentStep, JSON.stringify(qp.progress), player.character_id, questId]
  );
}

// ---- Complete quest and grant rewards ----
function completeQuest(socket, player, questId) {
  const quest = QUEST_REGISTRY[questId];
  const qp = player.questProgress[questId];
  if (!quest || !qp) return;

  qp.status = 'completed';
  qp.completedAt = new Date().toISOString();
  qp.lastCompletedAt = qp.completedAt;

  const rewards = quest.rewards || {};

  // Grant EXP
  if (rewards.baseExp) grantBaseExp(player, socket, rewards.baseExp);
  if (rewards.jobExp) grantJobExp(player, socket, rewards.jobExp);

  // Grant Zeny
  if (rewards.zeny) {
    player.zeny += rewards.zeny;
    updateCharacterZeny(player.character_id, player.zeny);
  }

  // Grant items
  if (rewards.items && rewards.items.length > 0) {
    for (const { itemId, quantity } of rewards.items) {
      addItemToInventory(player, itemId, quantity);
    }
    sendInventoryData(socket, player);
  }

  // Job change
  if (rewards.changeJob) {
    changePlayerJob(socket, player, rewards.changeJob);
  }

  // Zone unlock
  if (rewards.unlockZone) {
    player.unlockedZones = player.unlockedZones || [];
    if (!player.unlockedZones.includes(rewards.unlockZone)) {
      player.unlockedZones.push(rewards.unlockZone);
    }
  }

  // Persist
  pool.query(
    `UPDATE quest_progress SET status = 'completed', completed_at = NOW(), last_completed_at = NOW(),
     progress_json = $1, current_step = $2 WHERE character_id = $3 AND quest_id = $4`,
    [JSON.stringify(qp.progress), qp.currentStep, player.character_id, questId]
  );

  socket.emit('quest:completed', {
    questId,
    name: quest.name,
    rewards: {
      baseExp: rewards.baseExp || 0,
      jobExp: rewards.jobExp || 0,
      zeny: rewards.zeny || 0,
      items: rewards.items || [],
      changeJob: rewards.changeJob || null
    }
  });

  console.log(`[Quest] ${player.name} completed quest: ${quest.name}`);
}

// ---- Socket event: quest:accept ----
socket.on('quest:accept', ({ questId }) => {
  const player = players[socket.id];
  if (!player) return;
  startQuest(socket, player, questId);
});

// ---- Socket event: quest:abandon ----
socket.on('quest:abandon', ({ questId }) => {
  const player = players[socket.id];
  if (!player) return;

  const qp = player.questProgress?.[questId];
  if (!qp || qp.status !== 'active') return;

  qp.status = 'abandoned';
  pool.query(
    "UPDATE quest_progress SET status = 'abandoned' WHERE character_id = $1 AND quest_id = $2",
    [player.character_id, questId]
  );

  socket.emit('quest:abandoned', { questId });
});

// ---- Socket event: quest:turn_in ----
socket.on('quest:turn_in', ({ questId }) => {
  const player = players[socket.id];
  if (!player) return;

  const quest = QUEST_REGISTRY[questId];
  const qp = player.questProgress?.[questId];
  if (!quest || !qp || qp.status !== 'active') return;

  const step = quest.steps[qp.currentStep];
  if (!step) return;

  // Verify completion conditions for the current step
  if (step.type === QUEST_STEP_TYPES.COLLECT) {
    const targets = Array.isArray(step.target) ? step.target : [{ itemId: step.target, quantity: step.quantity }];
    for (const t of targets) {
      const owned = getInventoryItemCount(player, t.itemId);
      if (owned < t.quantity) {
        return socket.emit('quest:error', { message: `You still need more ${ro_item_mapping[t.itemId]?.name || 'items'}.` });
      }
    }
    // Consume the items
    for (const t of targets) {
      removeItemFromInventory(player, t.itemId, t.quantity);
    }
    sendInventoryData(socket, player);
  }

  advanceQuestStep(socket, player, questId);
});

// ---- Socket event: quest:request_log ----
socket.on('quest:request_log', () => {
  const player = players[socket.id];
  if (!player) return;

  const questLog = [];
  for (const [questId, qp] of Object.entries(player.questProgress || {})) {
    const quest = QUEST_REGISTRY[questId];
    if (!quest) continue;

    const currentStepData = quest.steps[qp.currentStep];
    questLog.push({
      questId,
      name: quest.name,
      description: quest.description,
      questType: quest.questType,
      status: qp.status,
      currentStep: qp.currentStep,
      totalSteps: quest.steps.length,
      stepDescription: currentStepData?.description || '',
      progress: qp.progress,
      rewards: quest.rewards,
      startedAt: qp.startedAt,
      completedAt: qp.completedAt
    });
  }

  socket.emit('quest:log', { quests: questLog });
});
```

### 4.4 Quest Hook Integration Points

These calls must be added to existing server code:

```javascript
// In the enemy death handler (after EXP grant):
updateQuestKillProgress(player, socket, enemy.templateId);

// In addItemToInventory (after item is added):
updateQuestCollectProgress(player, socket, itemId, newQuantity);

// In npc:interact handler (for TALK step type):
function checkQuestTalkStep(player, socket, npcId) {
  if (!player.questProgress) return;
  for (const [questId, qp] of Object.entries(player.questProgress)) {
    if (qp.status !== 'active') continue;
    const quest = QUEST_REGISTRY[questId];
    if (!quest) continue;
    const step = quest.steps[qp.currentStep];
    if (!step || step.type !== QUEST_STEP_TYPES.TALK) continue;
    if (step.target !== npcId) continue;
    advanceQuestStep(socket, player, questId);
  }
}

// In zone:ready handler (for REACH step type):
function checkQuestReachStep(player, socket, zoneName) {
  if (!player.questProgress) return;
  for (const [questId, qp] of Object.entries(player.questProgress)) {
    if (qp.status !== 'active') continue;
    const quest = QUEST_REGISTRY[questId];
    if (!quest) continue;
    const step = quest.steps[qp.currentStep];
    if (!step || step.type !== QUEST_STEP_TYPES.REACH) continue;
    if (step.target !== zoneName) continue;
    advanceQuestStep(socket, player, questId);
  }
}
```

### 4.5 Quest Socket Events Summary

| Direction | Event | Payload |
|-----------|-------|---------|
| Client->Server | `quest:accept` | `{ questId }` |
| Client->Server | `quest:abandon` | `{ questId }` |
| Client->Server | `quest:turn_in` | `{ questId }` |
| Client->Server | `quest:request_log` | `{}` |
| Server->Client | `quest:started` | `{ questId, name, description, steps[], currentStep, progress }` |
| Server->Client | `quest:progress` | `{ questId, currentStep, progress, stepDescription, current, required }` |
| Server->Client | `quest:completed` | `{ questId, name, rewards }` |
| Server->Client | `quest:abandoned` | `{ questId }` |
| Server->Client | `quest:error` | `{ message }` |
| Server->Client | `quest:log` | `{ quests[] }` |

---

## 5. Quest System -- Client

### 5.1 `UQuestSubsystem`

**File: `client/SabriMMO/Source/SabriMMO/UI/QuestSubsystem.h`**

```cpp
// QuestSubsystem.h -- UWorldSubsystem managing quest state on the client.
// Wraps quest:* socket events. Owns both the quest tracker sidebar
// (SQuestTrackerWidget, Z=13) and the full quest log (SQuestLogWidget, Z=22).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "QuestSubsystem.generated.h"

class USocketIOClientComponent;
class SQuestTrackerWidget;
class SQuestLogWidget;

UENUM()
enum class EQuestStatus : uint8
{
    Active,
    Completed,
    Failed,
    Abandoned
};

USTRUCT()
struct FQuestStepInfo
{
    GENERATED_BODY()

    int32 StepIndex = 0;
    FString Type;           // "kill", "collect", "talk", "reach"
    FString Description;
    int32 Quantity = 1;
};

USTRUCT()
struct FQuestEntry
{
    GENERATED_BODY()

    FString QuestId;
    FString Name;
    FString Description;
    FString QuestType;
    EQuestStatus Status = EQuestStatus::Active;
    int32 CurrentStep = 0;
    int32 TotalSteps = 1;
    FString StepDescription;    // Current step description
    int32 ProgressCurrent = 0;  // e.g. 45 kills
    int32 ProgressRequired = 0; // e.g. 150 kills
    TArray<FQuestStepInfo> Steps;
};

UCLASS()
class SABRIMMO_API UQuestSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Quest data (read by widgets) ----
    TArray<FQuestEntry> ActiveQuests;
    TArray<FQuestEntry> CompletedQuests;
    FString NotificationMessage;
    double NotificationExpireTime = 0.0;

    // ---- Public API ----
    void RequestQuestLog();
    void AcceptQuest(const FString& QuestId);
    void AbandonQuest(const FString& QuestId);
    void TurnInQuest(const FString& QuestId);
    void ToggleQuestLog();

    bool IsQuestLogVisible() const;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // Event handlers
    void HandleQuestStarted(const TSharedPtr<FJsonValue>& Data);
    void HandleQuestProgress(const TSharedPtr<FJsonValue>& Data);
    void HandleQuestCompleted(const TSharedPtr<FJsonValue>& Data);
    void HandleQuestAbandoned(const TSharedPtr<FJsonValue>& Data);
    void HandleQuestLog(const TSharedPtr<FJsonValue>& Data);
    void HandleQuestError(const TSharedPtr<FJsonValue>& Data);

    // Widget management
    void ShowTracker();
    void HideTracker();
    void ShowQuestLog();
    void HideQuestLog();

    bool bEventsWrapped = false;
    bool bTrackerAdded = false;
    bool bQuestLogAdded = false;
    FTimerHandle BindCheckTimer;

    // Tracker sidebar (always visible, Z=13)
    TSharedPtr<SQuestTrackerWidget> TrackerWidget;
    TSharedPtr<SWidget> TrackerWrapper;
    TSharedPtr<SWidget> TrackerOverlay;

    // Full quest log (toggled, Z=22)
    TSharedPtr<SQuestLogWidget> QuestLogWidget;
    TSharedPtr<SWidget> QuestLogWrapper;
    TSharedPtr<SWidget> QuestLogOverlay;

    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

**File: `client/SabriMMO/Source/SabriMMO/UI/QuestSubsystem.cpp`** (key methods)

```cpp
// QuestSubsystem.cpp -- Quest state management, socket wrapping,
// tracker + quest log widget lifecycle.

#include "QuestSubsystem.h"
#include "SQuestTrackerWidget.h"
#include "SQuestLogWidget.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogQuest, Log, All);

// ============================================================
// Lifecycle (same pattern as all other subsystems)
// ============================================================

bool UQuestSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UQuestSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UQuestSubsystem::TryWrapSocketEvents),
        0.5f, true
    );
}

void UQuestSubsystem::Deinitialize()
{
    HideTracker();
    HideQuestLog();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

// ============================================================
// Event wrapping (same pattern)
// ============================================================

void UQuestSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;
    TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
    if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;
    if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

    CachedSIOComponent = SIOComp;

    WrapSingleEvent(TEXT("quest:started"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleQuestStarted(D); });
    WrapSingleEvent(TEXT("quest:progress"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleQuestProgress(D); });
    WrapSingleEvent(TEXT("quest:completed"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleQuestCompleted(D); });
    WrapSingleEvent(TEXT("quest:abandoned"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleQuestAbandoned(D); });
    WrapSingleEvent(TEXT("quest:log"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleQuestLog(D); });
    WrapSingleEvent(TEXT("quest:error"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleQuestError(D); });

    bEventsWrapped = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }

    // Request full quest log on connect
    RequestQuestLog();

    // Show the tracker sidebar
    ShowTracker();

    UE_LOG(LogQuest, Log, TEXT("QuestSubsystem -- events wrapped."));
}

// ============================================================
// Event Handlers
// ============================================================

void UQuestSubsystem::HandleQuestStarted(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FQuestEntry Entry;
    Obj->TryGetStringField(TEXT("questId"), Entry.QuestId);
    Obj->TryGetStringField(TEXT("name"), Entry.Name);
    Obj->TryGetStringField(TEXT("description"), Entry.Description);
    Entry.Status = EQuestStatus::Active;

    double Step = 0;
    Obj->TryGetNumberField(TEXT("currentStep"), Step);
    Entry.CurrentStep = (int32)Step;

    // Parse steps array
    const TArray<TSharedPtr<FJsonValue>>* StepsArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("steps"), StepsArr) && StepsArr)
    {
        Entry.TotalSteps = StepsArr->Num();
        for (const TSharedPtr<FJsonValue>& StepVal : *StepsArr)
        {
            const TSharedPtr<FJsonObject>* StepObj = nullptr;
            if (StepVal.IsValid() && StepVal->TryGetObject(StepObj) && StepObj)
            {
                FQuestStepInfo SI;
                double Idx = 0;
                (*StepObj)->TryGetNumberField(TEXT("stepIndex"), Idx);
                SI.StepIndex = (int32)Idx;
                (*StepObj)->TryGetStringField(TEXT("type"), SI.Type);
                (*StepObj)->TryGetStringField(TEXT("description"), SI.Description);
                double Qty = 1;
                (*StepObj)->TryGetNumberField(TEXT("quantity"), Qty);
                SI.Quantity = (int32)Qty;
                Entry.Steps.Add(SI);
            }
        }
    }

    if (Entry.Steps.IsValidIndex(Entry.CurrentStep))
    {
        Entry.StepDescription = Entry.Steps[Entry.CurrentStep].Description;
        Entry.ProgressRequired = Entry.Steps[Entry.CurrentStep].Quantity;
    }

    // Remove any existing entry with same ID, then add fresh
    ActiveQuests.RemoveAll([&](const FQuestEntry& E) { return E.QuestId == Entry.QuestId; });
    ActiveQuests.Add(Entry);

    NotificationMessage = FString::Printf(TEXT("Quest started: %s"), *Entry.Name);
    NotificationExpireTime = FPlatformTime::Seconds() + 5.0;

    UE_LOG(LogQuest, Log, TEXT("Quest started: %s"), *Entry.Name);
}

void UQuestSubsystem::HandleQuestProgress(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FString QuestId;
    Obj->TryGetStringField(TEXT("questId"), QuestId);

    FQuestEntry* Found = ActiveQuests.FindByPredicate(
        [&](const FQuestEntry& E) { return E.QuestId == QuestId; });
    if (!Found) return;

    double Step = 0, Cur = 0, Req = 0;
    Obj->TryGetNumberField(TEXT("currentStep"), Step);
    Obj->TryGetNumberField(TEXT("current"), Cur);
    Obj->TryGetNumberField(TEXT("required"), Req);
    FString StepDesc;
    Obj->TryGetStringField(TEXT("stepDescription"), StepDesc);

    Found->CurrentStep = (int32)Step;
    Found->ProgressCurrent = (int32)Cur;
    Found->ProgressRequired = (int32)Req;
    if (!StepDesc.IsEmpty()) Found->StepDescription = StepDesc;
}

void UQuestSubsystem::HandleQuestCompleted(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FString QuestId, Name;
    Obj->TryGetStringField(TEXT("questId"), QuestId);
    Obj->TryGetStringField(TEXT("name"), Name);

    // Move from active to completed
    FQuestEntry Completed;
    int32 RemIdx = ActiveQuests.IndexOfByPredicate(
        [&](const FQuestEntry& E) { return E.QuestId == QuestId; });
    if (RemIdx != INDEX_NONE)
    {
        Completed = ActiveQuests[RemIdx];
        ActiveQuests.RemoveAt(RemIdx);
    }
    else
    {
        Completed.QuestId = QuestId;
        Completed.Name = Name;
    }
    Completed.Status = EQuestStatus::Completed;
    CompletedQuests.Add(Completed);

    NotificationMessage = FString::Printf(TEXT("Quest completed: %s"), *Name);
    NotificationExpireTime = FPlatformTime::Seconds() + 5.0;

    UE_LOG(LogQuest, Log, TEXT("Quest completed: %s"), *Name);
}

void UQuestSubsystem::HandleQuestAbandoned(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FString QuestId;
    (*ObjPtr)->TryGetStringField(TEXT("questId"), QuestId);

    ActiveQuests.RemoveAll([&](const FQuestEntry& E) { return E.QuestId == QuestId; });
}

void UQuestSubsystem::HandleQuestLog(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    ActiveQuests.Empty();
    CompletedQuests.Empty();

    const TArray<TSharedPtr<FJsonValue>>* QuestsArr = nullptr;
    if ((*ObjPtr)->TryGetArrayField(TEXT("quests"), QuestsArr) && QuestsArr)
    {
        for (const TSharedPtr<FJsonValue>& QVal : *QuestsArr)
        {
            const TSharedPtr<FJsonObject>* QObj = nullptr;
            if (!QVal.IsValid() || !QVal->TryGetObject(QObj) || !QObj) continue;

            FQuestEntry Entry;
            (*QObj)->TryGetStringField(TEXT("questId"), Entry.QuestId);
            (*QObj)->TryGetStringField(TEXT("name"), Entry.Name);
            (*QObj)->TryGetStringField(TEXT("description"), Entry.Description);
            (*QObj)->TryGetStringField(TEXT("questType"), Entry.QuestType);
            (*QObj)->TryGetStringField(TEXT("stepDescription"), Entry.StepDescription);

            FString StatusStr;
            (*QObj)->TryGetStringField(TEXT("status"), StatusStr);
            if (StatusStr == TEXT("active"))    Entry.Status = EQuestStatus::Active;
            else if (StatusStr == TEXT("completed")) Entry.Status = EQuestStatus::Completed;
            else if (StatusStr == TEXT("failed"))    Entry.Status = EQuestStatus::Failed;
            else Entry.Status = EQuestStatus::Abandoned;

            double CS = 0, TS = 0;
            (*QObj)->TryGetNumberField(TEXT("currentStep"), CS);
            (*QObj)->TryGetNumberField(TEXT("totalSteps"), TS);
            Entry.CurrentStep = (int32)CS;
            Entry.TotalSteps = (int32)TS;

            if (Entry.Status == EQuestStatus::Active)
                ActiveQuests.Add(Entry);
            else if (Entry.Status == EQuestStatus::Completed)
                CompletedQuests.Add(Entry);
        }
    }

    UE_LOG(LogQuest, Log, TEXT("Quest log loaded: %d active, %d completed"),
        ActiveQuests.Num(), CompletedQuests.Num());
}

void UQuestSubsystem::HandleQuestError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FString Msg;
    if ((*ObjPtr)->TryGetStringField(TEXT("message"), Msg))
    {
        NotificationMessage = Msg;
        NotificationExpireTime = FPlatformTime::Seconds() + 4.0;
    }
}

// ============================================================
// Public API
// ============================================================

void UQuestSubsystem::RequestQuestLog()
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    CachedSIOComponent->EmitNative(TEXT("quest:request_log"), Payload);
}

void UQuestSubsystem::AcceptQuest(const FString& QuestId)
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("questId"), QuestId);
    CachedSIOComponent->EmitNative(TEXT("quest:accept"), Payload);
}

void UQuestSubsystem::AbandonQuest(const FString& QuestId)
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("questId"), QuestId);
    CachedSIOComponent->EmitNative(TEXT("quest:abandon"), Payload);
}

void UQuestSubsystem::TurnInQuest(const FString& QuestId)
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("questId"), QuestId);
    CachedSIOComponent->EmitNative(TEXT("quest:turn_in"), Payload);
}

void UQuestSubsystem::ToggleQuestLog()
{
    if (bQuestLogAdded) HideQuestLog();
    else ShowQuestLog();
}

bool UQuestSubsystem::IsQuestLogVisible() const
{
    return bQuestLogAdded;
}

// ============================================================
// Widget Lifecycle -- Tracker (persistent sidebar, Z=13)
// ============================================================

void UQuestSubsystem::ShowTracker()
{
    if (bTrackerAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    TrackerWidget = SNew(SQuestTrackerWidget).Subsystem(this);

    TrackerWrapper =
        SNew(SBox)
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Top)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [
            TrackerWidget.ToSharedRef()
        ];

    TrackerOverlay = SNew(SWeakWidget).PossiblyNullContent(TrackerWrapper);
    ViewportClient->AddViewportWidgetContent(TrackerOverlay.ToSharedRef(), 13);
    bTrackerAdded = true;
}

void UQuestSubsystem::HideTracker()
{
    if (!bTrackerAdded) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (TrackerOverlay.IsValid())
                VC->RemoveViewportWidgetContent(TrackerOverlay.ToSharedRef());
        }
    }
    TrackerWidget.Reset();
    TrackerWrapper.Reset();
    TrackerOverlay.Reset();
    bTrackerAdded = false;
}

// ============================================================
// Widget Lifecycle -- Quest Log (toggled with Alt+U, Z=22)
// ============================================================

void UQuestSubsystem::ShowQuestLog()
{
    if (bQuestLogAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    QuestLogWidget = SNew(SQuestLogWidget).Subsystem(this);

    QuestLogWrapper =
        SNew(SBox)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [
            QuestLogWidget.ToSharedRef()
        ];

    QuestLogOverlay = SNew(SWeakWidget).PossiblyNullContent(QuestLogWrapper);
    ViewportClient->AddViewportWidgetContent(QuestLogOverlay.ToSharedRef(), 22);
    bQuestLogAdded = true;

    // Request fresh data
    RequestQuestLog();
}

void UQuestSubsystem::HideQuestLog()
{
    if (!bQuestLogAdded) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (QuestLogOverlay.IsValid())
                VC->RemoveViewportWidgetContent(QuestLogOverlay.ToSharedRef());
        }
    }
    QuestLogWidget.Reset();
    QuestLogWrapper.Reset();
    QuestLogOverlay.Reset();
    bQuestLogAdded = false;
}

// FindSocketIOComponent and WrapSingleEvent are identical to other subsystems -- omitted for brevity.
```

### 5.2 `SQuestTrackerWidget` -- Persistent Sidebar

**File: `client/SabriMMO/Source/SabriMMO/UI/SQuestTrackerWidget.h`**

```cpp
// SQuestTrackerWidget.h -- Compact quest tracker sidebar, top-right of screen.
// Shows active quests with name, current objective, and progress bar/counter.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UQuestSubsystem;

class SQuestTrackerWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SQuestTrackerWidget) : _Subsystem(nullptr) {}
        SLATE_ARGUMENT(UQuestSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    TWeakObjectPtr<UQuestSubsystem> OwningSubsystem;
};
```

The `OnPaint` override renders each active quest as:

```
  Quest Name                    (gold text, bold)
    > Current objective          (white text)
      [======    ] 45/150        (progress bar + counter)
```

This uses `FSlateDrawElement::MakeBox()` for the progress bar fill and `FSlateDrawElement::MakeText()` for text, following the same OnPaint rendering pattern used by `SWorldHealthBarOverlay` and `SDamageNumberOverlay`.

### 5.3 `SQuestLogWidget` -- Full Quest Journal

**File: `client/SabriMMO/Source/SabriMMO/UI/SQuestLogWidget.h`**

```cpp
// SQuestLogWidget.h -- Full quest journal. Toggle with Alt+U.
// Layout: Two-pane -- quest list on left, detail panel on right.
// Tabs: Active / Completed. Abandon button for active quests.
// RO Classic brown/gold theme. Draggable.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UQuestSubsystem;
class SVerticalBox;

class SQuestLogWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SQuestLogWidget) : _Subsystem(nullptr) {}
        SLATE_ARGUMENT(UQuestSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UQuestSubsystem> OwningSubsystem;

    enum class EQuestTab : uint8 { Active, Completed };
    EQuestTab CurrentTab = EQuestTab::Active;
    int32 SelectedQuestIndex = -1;

    TSharedPtr<SVerticalBox> QuestListContainer;
    TSharedPtr<SVerticalBox> DetailContainer;

    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildTabButtons();
    TSharedRef<SWidget> BuildQuestList();
    TSharedRef<SWidget> BuildDetailPanel();
    TSharedRef<SWidget> BuildQuestButton(const FText& Label, FOnClicked OnClicked);

    void RebuildQuestList();
    void RebuildDetail();

    // Drag (same pattern as all widgets)
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
    FVector2D WidgetPosition = FVector2D(200.0, 100.0);
    void ApplyLayout();

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent) override;
};
```

The quest log layout:

```
+---------------------------------------------+
| Quest Log                              [X]  |
|---------------------------------------------|
| [Active]  [Completed]                       |
|---------------------------------------------|
| Quest List        | Detail Panel            |
|                   |                         |
| > Poring Hunt     | Poring Extermination    |
|   Path of the     | Kill 150 Porings in     |
|   Swordsman       | the Prontera fields.    |
|                   |                         |
|                   | Step 1/1:               |
|                   | Kill 150 Porings.       |
|                   | Progress: 45/150        |
|                   |                         |
|                   | Rewards:                |
|                   |   5,000 Base EXP        |
|                   |   2,500 Job EXP         |
|                   |   1,500 Zeny            |
|                   |                         |
|                   | [Abandon Quest]         |
+---------------------------------------------+
```

### 5.4 Keybind Integration

Add to `ASabriMMOCharacter::SetupPlayerInputComponent()`:

```cpp
// In the Enhanced Input mapping context (IMC_MMOCharacter):
// IA_QuestLog bound to Alt+U

void ASabriMMOCharacter::HandleQuestLogToggle()
{
    if (UWorld* World = GetWorld())
    {
        if (UQuestSubsystem* QuestSub = World->GetSubsystem<UQuestSubsystem>())
        {
            QuestSub->ToggleQuestLog();
        }
    }
}
```

---

## 6. Job Change Quest Template

### 6.1 Complete Novice -> Swordsman Flow

This documents the full end-to-end implementation for one job change quest, serving as the template for all 6 first-class changes.

**Prerequisites checked server-side:**
- Job Level >= 10
- Current class == "novice"
- All 9 Basic Skill points allocated (implied by Job Level 10)

**NPCs involved:**

| NPC Key | Name | Zone | Role |
|---------|------|------|------|
| `izlude_swordsman_master` | Master Swordsman | izlude | Quest giver + completer |
| `izlude_test_guide` | Test Guide | izlude | Step 0 target |
| `izlude_training_ground` | (zone) | izlude_training_ground | Step 1 target |

**Dialogue tree: `swordsman_job_change`**

```
Page 0: "You wish to become a Swordsman?"
  Choice: "Yes" -> Page 1
  Choice: "No"  -> close

Page 1: [CONDITIONS: jobLevel >= 10, class == novice]
  "You must have reached Job Level 10..."
  hasNext: true -> Page 2
  failPage: 5 (not ready)

Page 2: "Speak with the Test Guide..."
  Choice: "I am ready" -> action: start_quest(quest_swordsman_change)
  Choice: "Let me prepare" -> close

Page 3: [CONDITIONS: questComplete(quest_swordsman_change)]
  "You have completed the trial! You are a Swordsman."
  action: change_job(swordsman)
  failPage: 4

Page 4: "You have not yet completed the trial."
  action: close

Page 5: "You are not yet ready. Return at Job Level 10."
  action: close
```

**Quest definition: `quest_swordsman_change`**

```javascript
{
  questId: 'quest_swordsman_change',
  questType: 'job_change',
  name: 'Path of the Swordsman',
  minJobLevel: 10,
  requiredClass: 'novice',
  isRepeatable: false,
  rewards: { changeJob: 'swordsman' },
  steps: [
    { stepIndex: 0, type: 'talk',  target: 'izlude_test_guide',      zone: 'izlude' },
    { stepIndex: 1, type: 'reach', target: 'izlude_training_ground', zone: 'izlude_training_ground' },
    { stepIndex: 2, type: 'talk',  target: 'izlude_swordsman_master', zone: 'izlude' }
  ]
}
```

**Server `changePlayerJob()` function:**

```javascript
async function changePlayerJob(socket, player, newClass) {
  const oldClass = player.job_class;

  // Update in-memory
  player.job_class = newClass;
  player.job_level = 1;
  player.job_exp = 0;
  player.skill_points = 0;

  // Persist to DB
  await pool.query(
    'UPDATE characters SET job_class = $1, job_level = 1, job_exp = 0, skill_points = 0 WHERE id = $2',
    [newClass, player.character_id]
  );

  // Notify client
  socket.emit('player:job_changed', {
    oldClass,
    newClass,
    jobLevel: 1,
    jobExp: 0,
    skillPoints: 0
  });

  // Broadcast to zone (other players see class change)
  broadcastToZoneExcept(player.zone_name, socket.id, 'player:class_update', {
    characterId: player.character_id,
    jobClass: newClass
  });

  // Refresh stats (new class may have different stat bonuses)
  emitPlayerStats(socket, player);

  console.log(`[JobChange] ${player.name}: ${oldClass} -> ${newClass}`);
}
```

### 6.2 Replicating for Other Classes

To add Mage, Archer, Thief, Merchant, or Acolyte job changes, create:

1. **Dialogue tree** in `ro_dialogue_data.js` (copy `swordsman_job_change`, change speaker/text/conditions)
2. **Quest entry** in `ro_quest_data.js` (copy `quest_swordsman_change`, change steps/targets)
3. **NPC entries** in `ro_npc_data.js` (guild master + helper NPCs)
4. **AMMONPC actors** placed in the appropriate zone level in UE5

Each class has unique quest steps:

| Class | Step Types | Key Differences |
|-------|-----------|----------------|
| Swordsman | talk, reach, talk | Obstacle course (reach zone) |
| Mage | talk, collect, talk | Collect solution ingredients |
| Archer | talk, kill, talk | Kill Willows, collect Trunks |
| Thief | talk, collect, talk | Collect mushrooms |
| Merchant | talk, deliver, talk | Deliver parcel + 1,000z fee |
| Acolyte | talk, reach, talk | Pilgrimage to random Ascetic |

---

## 7. Refine NPC

### 7.1 Refinement Data

**File: `server/src/ro_refine_data.js`**

```javascript
// ============================================================
// Refinement System Constants
// ============================================================

const REFINE_ORES = {
  PHRACON:       984,   // Lv1 weapons
  EMVERETARCON:  985,   // Lv2 weapons
  ORIDECON:      984,   // Lv3-4 weapons (using Oridecon ID)
  ELUNIUM:       985,   // All armor (using Elunium ID)
  // Note: Replace with actual item IDs from ro_item_mapping
};

const REFINE_COSTS = {
  1: 50,      // Lv1 weapon
  2: 200,     // Lv2 weapon
  3: 5000,    // Lv3 weapon
  4: 20000,   // Lv4 weapon
  armor: 2000 // All armor
};

const SAFETY_LIMITS = {
  1: 7,   // Lv1 weapon safe to +7
  2: 6,   // Lv2 weapon safe to +6
  3: 5,   // Lv3 weapon safe to +5
  4: 4,   // Lv4 weapon safe to +4
  armor: 4 // All armor safe to +4
};

// Success rates beyond safety limit (refine level -> rate)
const REFINE_RATES = {
  // [weaponLevel][refineLevel] = successRate (0.0 - 1.0)
  1: { 8: 0.60, 9: 0.40, 10: 0.20 },
  2: { 7: 0.60, 8: 0.40, 9: 0.20, 10: 0.20 },
  3: { 6: 0.60, 7: 0.50, 8: 0.20, 9: 0.20, 10: 0.10 },
  4: { 5: 0.60, 6: 0.40, 7: 0.40, 8: 0.20, 9: 0.10, 10: 0.10 },
  armor: { 5: 0.60, 6: 0.40, 7: 0.40, 8: 0.20, 9: 0.20, 10: 0.10 }
};

function getRefineSuccessRate(weaponLevel, currentRefine) {
  const nextRefine = currentRefine + 1;
  const safeLimit = SAFETY_LIMITS[weaponLevel] || SAFETY_LIMITS.armor;

  if (nextRefine <= safeLimit) return 1.0; // Guaranteed success

  const rateTable = REFINE_RATES[weaponLevel] || REFINE_RATES.armor;
  return rateTable[nextRefine] || 0.05; // Default 5% for extreme refines
}

function getRequiredOre(weaponLevel, isArmor) {
  if (isArmor) return REFINE_ORES.ELUNIUM;
  switch (weaponLevel) {
    case 1: return REFINE_ORES.PHRACON;
    case 2: return REFINE_ORES.EMVERETARCON;
    case 3: case 4: return REFINE_ORES.ORIDECON;
    default: return null;
  }
}

function getRefineCost(weaponLevel, isArmor) {
  if (isArmor) return REFINE_COSTS.armor;
  return REFINE_COSTS[weaponLevel] || 0;
}

module.exports = {
  REFINE_ORES, REFINE_COSTS, SAFETY_LIMITS, REFINE_RATES,
  getRefineSuccessRate, getRequiredOre, getRefineCost
};
```

### 7.2 Server Refine Handlers

```javascript
const {
  getRefineSuccessRate, getRequiredOre, getRefineCost
} = require('./ro_refine_data');

// ============================================================
// refine:open -- send player's refinable equipment list
// ============================================================

function openRefineForPlayer(socket, player) {
  const refinableItems = (player.inventory || [])
    .filter(item => {
      if (!item.is_equipped) return false;
      const data = ro_item_mapping[item.item_id];
      if (!data) return false;
      // Must be weapon or armor type
      return data.item_type === 'weapon' || data.item_type === 'armor';
    })
    .map(item => {
      const data = ro_item_mapping[item.item_id];
      const isArmor = data.item_type === 'armor';
      const weaponLevel = data.weapon_level || 1;
      const currentRefine = item.refine_level || 0;
      const successRate = getRefineSuccessRate(
        isArmor ? 'armor' : weaponLevel, currentRefine
      );
      const requiredOreId = getRequiredOre(weaponLevel, isArmor);
      const cost = getRefineCost(weaponLevel, isArmor);
      const playerHasOre = getInventoryItemCount(player, requiredOreId) > 0;

      return {
        inventoryId: item.inventory_id,
        itemId: item.item_id,
        name: data.name,
        refineLevel: currentRefine,
        maxRefine: 10,
        successRate: Math.round(successRate * 100),
        requiredOreId,
        requiredOreName: ro_item_mapping[requiredOreId]?.name || 'Ore',
        playerHasOre,
        cost,
        isArmor,
        weaponLevel: isArmor ? 0 : weaponLevel
      };
    });

  socket.emit('refine:data', {
    items: refinableItems,
    playerZeny: player.zeny
  });
}

// ============================================================
// refine:attempt -- player attempts to refine an item
// ============================================================

socket.on('refine:attempt', ({ inventoryId }) => {
  const player = players[socket.id];
  if (!player) return;

  const invItem = player.inventory?.find(i => i.inventory_id === inventoryId);
  if (!invItem || !invItem.is_equipped) {
    return socket.emit('refine:error', { message: 'Item not found or not equipped.' });
  }

  const data = ro_item_mapping[invItem.item_id];
  if (!data) return;

  const isArmor = data.item_type === 'armor';
  const weaponLevel = data.weapon_level || 1;
  const currentRefine = invItem.refine_level || 0;

  if (currentRefine >= 10) {
    return socket.emit('refine:error', { message: 'Item is already at maximum refinement.' });
  }

  const requiredOreId = getRequiredOre(weaponLevel, isArmor);
  const cost = getRefineCost(weaponLevel, isArmor);

  // Check ore
  if (getInventoryItemCount(player, requiredOreId) < 1) {
    return socket.emit('refine:error', { message: 'You do not have the required ore.' });
  }

  // Check zeny
  if (player.zeny < cost) {
    return socket.emit('refine:error', { message: 'Not enough Zeny.' });
  }

  // Consume ore and zeny
  removeItemFromInventory(player, requiredOreId, 1);
  player.zeny -= cost;
  updateCharacterZeny(player.character_id, player.zeny);

  // Roll for success
  const successRate = getRefineSuccessRate(
    isArmor ? 'armor' : weaponLevel, currentRefine
  );
  const roll = Math.random();
  const success = roll < successRate;

  if (success) {
    invItem.refine_level = currentRefine + 1;

    // Persist refine level to DB
    pool.query(
      'UPDATE character_inventory SET refine_level = $1 WHERE id = $2',
      [invItem.refine_level, invItem.inventory_id]
    );

    socket.emit('refine:result', {
      success: true,
      inventoryId,
      itemName: data.name,
      newRefineLevel: invItem.refine_level,
      newZeny: player.zeny
    });

    // Refresh inventory + stats (refine affects ATK/DEF)
    sendInventoryData(socket, player);
    emitPlayerStats(socket, player);

    console.log(`[Refine] ${player.name}: +${invItem.refine_level} ${data.name} (SUCCESS, rate: ${Math.round(successRate * 100)}%)`);
  } else {
    // FAILURE -- item is destroyed
    const destroyedName = `+${currentRefine} ${data.name}`;
    removeItemFromInventory(player, invItem.item_id, 1, inventoryId);

    socket.emit('refine:result', {
      success: false,
      inventoryId,
      itemName: destroyedName,
      newRefineLevel: 0,
      newZeny: player.zeny,
      destroyed: true
    });

    sendInventoryData(socket, player);
    emitPlayerStats(socket, player);

    console.log(`[Refine] ${player.name}: +${currentRefine} ${data.name} DESTROYED (rate was: ${Math.round(successRate * 100)}%)`);
  }
});
```

### 7.3 Client Refine UI Flow

The refine system uses the existing DialogueSubsystem for the NPC greeting, then opens a dedicated `URefineSubsystem` widget:

1. Player clicks Refine NPC (`AMMONPC` with `NPCType = ENPCType::Refine`)
2. `DialogueSubsystem` sends `npc:interact`, server responds with dialogue
3. Player picks "Refine equipment" choice, triggering `action: 'open_refine'`
4. Server calls `openRefineForPlayer()`, emits `refine:data`
5. `URefineSubsystem` receives `refine:data`, opens `SRefineWidget`

**`SRefineWidget` layout:**

```
+-----------------------------------------------+
| Hollegren's Forge                        [X]  |
|-----------------------------------------------|
| Select equipment to refine:                   |
|                                               |
| +-------------------------------------------+ |
| | +7 Sword [3]      Rate: 60%   Cost: 50z  | |
| |   Requires: Phracon                  [Go] | |
| +-------------------------------------------+ |
| | +4 Chain Mail      Rate: 60%   Cost: 2kz | |
| |   Requires: Elunium              [Go]     | |
| +-------------------------------------------+ |
|                                               |
| Zeny: 125,000                                 |
+-----------------------------------------------+
```

**Socket events for refine:**

| Direction | Event | Payload |
|-----------|-------|---------|
| Server->Client | `refine:data` | `{ items[], playerZeny }` |
| Client->Server | `refine:attempt` | `{ inventoryId }` |
| Server->Client | `refine:result` | `{ success, inventoryId, itemName, newRefineLevel, newZeny, destroyed? }` |
| Server->Client | `refine:error` | `{ message }` |

### 7.4 DB Schema Addition

```sql
-- Add refine_level column to character_inventory
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS refine_level INTEGER NOT NULL DEFAULT 0;
```

---

## 8. Adding New NPC Template -- 5-Step Guide

### Step 1: Define the NPC in the Server Registry

Add an entry to `server/src/ro_npc_data.js`:

```javascript
'my_zone_new_npc': {
    npcId: 9001,                          // Unique across all NPCs
    type: NPC_TYPES.QUEST,                // Or SHOP_TOOL, GENERIC, etc.
    name: 'Mysterious Stranger',
    spriteId: 120,
    zone: 'prt_fild08',                   // Must match ZONE_REGISTRY
    position: { x: 500, y: 300, z: 0 },  // UE world coordinates
    facing: 4,
    interactRadius: 300,
    shopId: null,                          // Set if this NPC has a shop
    dialogueId: 'mysterious_stranger_01',  // Must match DIALOGUE_REGISTRY
    questId: 'quest_mysterious_task',      // Optional quest association
    isActive: true
}
```

### Step 2: Create the Dialogue Tree

Add an entry to `server/src/ro_dialogue_data.js`:

```javascript
'mysterious_stranger_01': {
    pages: [
        {
            pageIndex: 0,
            speaker: 'Mysterious Stranger',
            portrait: null,
            text: 'Psst... adventurer. I have a task for you.\nAre you interested?',
            hasNext: false,
            choices: [
                { text: 'Tell me more.', nextPage: 1 },
                { text: 'No thanks.', action: 'close' }
            ],
            conditions: null,
            failPage: null
        },
        {
            pageIndex: 1,
            speaker: 'Mysterious Stranger',
            portrait: null,
            text: 'I need you to bring me 10 Red Herbs.\nCan you do that?',
            hasNext: false,
            choices: [
                { text: 'I accept!', action: 'start_quest', params: { questId: 'quest_mysterious_task' } },
                { text: 'Maybe later.', action: 'close' }
            ],
            conditions: null,
            failPage: null
        },
        // ... completion pages with conditions
    ]
}
```

### Step 3: Create the Quest (if applicable)

Add an entry to `server/src/ro_quest_data.js`:

```javascript
'quest_mysterious_task': {
    questId: 'quest_mysterious_task',
    questType: 'story',
    name: 'The Mysterious Task',
    description: 'A stranger needs 10 Red Herbs.',
    minBaseLevel: 1,
    minJobLevel: 1,
    requiredClass: null,
    isRepeatable: false,
    cooldownHours: 0,
    rewards: {
        baseExp: 1000,
        jobExp: 500,
        zeny: 2000,
        items: [{ itemId: 501, quantity: 5 }]
    },
    steps: [
        {
            stepIndex: 0,
            type: 'collect',
            description: 'Collect 10 Red Herbs.',
            target: [{ itemId: 507, quantity: 10 }],
            quantity: 1,
            zone: null,
            onComplete: 'next_step'
        },
        {
            stepIndex: 1,
            type: 'talk',
            description: 'Return to the Mysterious Stranger.',
            target: 'my_zone_new_npc',
            quantity: 1,
            zone: 'prt_fild08',
            onComplete: 'complete_quest'
        }
    ]
}
```

### Step 4: Place the NPC Actor in UE5

1. Open the target level in Unreal Editor (e.g., `L_PrtSouth`)
2. Place an `AMMONPC` actor at the desired location
3. In the Details panel, set:
   - **NpcId**: `my_zone_new_npc` (must match server registry key)
   - **NPCDisplayName**: `Mysterious Stranger`
   - **NPCType**: `Quest` (or appropriate type)
   - **InteractionRadius**: `300`
4. Optionally assign a mesh/material to `MeshComp` for visual appearance
5. Save the level

### Step 5: Test the Full Loop

1. Start the server (`npm run dev`)
2. Launch the client, enter the zone where the NPC is placed
3. Click the NPC -- verify `npc:dialogue` event fires and `SDialogueWidget` appears
4. Walk through the dialogue choices
5. Accept the quest -- verify it appears in the quest tracker sidebar
6. Complete the objectives (collect items / kill monsters)
7. Return to NPC -- verify quest completion dialogue and rewards

**Common issues and fixes:**

| Symptom | Cause | Fix |
|---------|-------|-----|
| "NPC not found" error | `NpcId` in UE5 does not match server registry key | Ensure exact string match (case-sensitive) |
| NPC click does nothing | Player too far, or `InteractionRadius` too small | Increase radius or move closer |
| Dialogue shows but choices missing | `conditions` on choices failing | Check player stats against choice conditions |
| Quest progress not updating | Hook not called in enemy death / item pickup | Verify `updateQuestKillProgress()` is called |
| Quest won't complete | Turn-in NPC key doesn't match step target | Ensure `npcId` matches the step's `target` field |
| Widget Z-order conflict | Multiple widgets at same Z-level | Use unique Z values (dialogue=20, shop=18, quest log=22) |

---

## Appendix: Z-Order Reference

| Widget | Z-Level | Toggle |
|--------|---------|--------|
| SWorldHealthBarOverlay | 8 | Always on |
| SBasicInfoWidget | 10 | Always on |
| SCombatStatsWidget | 12 | F8 |
| SQuestTrackerWidget | 13 | Always on |
| SInventoryWidget | 14 | F6 |
| SEquipmentWidget | 15 | F7 |
| SHotbarRowWidget | 16 | F5 |
| SShopWidget | 18 | On shop:data |
| SKafraWidget | 19 | On kafra:data |
| SDialogueWidget | 20 | On npc:dialogue |
| SSkillTreeWidget | 20 | K |
| SDamageNumberOverlay | 20 | Always on |
| SQuestLogWidget | 22 | Alt+U |
| SCastBarOverlay | 25 | On skill:cast_start |
| SHotbarKeybindWidget | 30 | Gear icon |
| SLoadingOverlayWidget | 50 | Zone transitions |

## Appendix: File Summary

### New Server Files

| File | Purpose |
|------|---------|
| `server/src/ro_npc_data.js` | NPC_TYPES, NPC_REGISTRY |
| `server/src/ro_dialogue_data.js` | DIALOGUE_REGISTRY (JSON dialogue trees) |
| `server/src/ro_shop_data.js` | SHOP_REGISTRY (shop inventories) |
| `server/src/ro_quest_data.js` | QUEST_REGISTRY, QUEST_STEP_TYPES |
| `server/src/ro_refine_data.js` | Refine rates, ores, costs, safety limits |
| `database/migrations/add_quest_system.sql` | quests + quest_progress tables |

### New Client C++ Files

| File | Purpose |
|------|---------|
| `MMONPC.h/.cpp` | Generic data-driven NPC actor |
| `UI/DialogueSubsystem.h/.cpp` | Dialogue state machine + socket wrapping |
| `UI/SDialogueWidget.h/.cpp` | Slate dialogue window (portrait, text, choices) |
| `UI/ShopSubsystem.h/.cpp` | Shop state, cart management + socket wrapping |
| `UI/SShopWidget.h/.cpp` | Slate shop interface (buy/sell tabs, cart, total) |
| `UI/QuestSubsystem.h/.cpp` | Quest log state + socket wrapping |
| `UI/SQuestTrackerWidget.h/.cpp` | Persistent quest tracker sidebar (OnPaint) |
| `UI/SQuestLogWidget.h/.cpp` | Full quest journal (two-pane, tabbed) |

### Modified Existing Files

| File | Change |
|------|--------|
| `server/src/index.js` | Add npc:*, shop:*, quest:*, refine:* socket handlers |
| `CharacterData.h` | Already has FShopItem, FCartItem (no changes needed) |
| `SabriMMOCharacter.cpp` | Add AMMONPC cast in click handler + Alt+U keybind |
| `BasicInfoSubsystem.cpp` | Already wraps shop:bought/sold (no changes needed) |
