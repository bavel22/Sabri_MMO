# 04 - Name Tags & Unified Click-to-Interact Pipeline

**Migration Target:** Entity name tags (Slate OnPaint overlay) and unified click-to-interact system
**Status:** PLANNED (updated 2026-03-14 with accurate RO Classic pre-renewal research)
**Dependencies:** UOtherPlayerSubsystem, UEnemySubsystem, ASabriMMOCharacter, NPC actors
**Related Plans:** 01-03 in this migration series

---

## Table of Contents

1. [RO Classic Name Tag Behavior](#1-ro-classic-name-tag-behavior)
2. [Current Blueprint Implementation](#2-current-blueprint-implementation)
3. [New C++ Architecture: UNameTagSubsystem](#3-new-c-architecture-unametagsubsystem)
4. [SNameTagOverlay (Slate OnPaint Widget)](#4-snametagoverlay-slate-onpaint-widget)
5. [Name Tag Colors & Styling](#5-name-tag-colors--styling)
6. [Registration Points](#6-registration-points)
7. [Unified Click-to-Interact Pipeline](#7-unified-click-to-interact-pipeline)
8. [Walk-to-Interact System](#8-walk-to-interact-system)
9. [Collision Setup for Clickable Actors](#9-collision-setup-for-clickable-actors)
10. [Files to Create](#10-files-to-create)
11. [Files to Modify](#11-files-to-modify)
12. [Blueprint Cleanup (Phase 6)](#12-blueprint-cleanup-phase-6)
13. [Performance Considerations](#13-performance-considerations)
14. [Testing Checklist](#14-testing-checklist)

---

## 1. RO Classic Name Tag Behavior

Reference: Pre-Renewal Ragnarok Online name tag conventions.

### 1.1 Player Name Tags (UPDATED — verified via roBrowser source + iRO Wiki)

- Character name displayed **below the character** by default (toggled above/below via `/font` command).
- **Always visible** while on screen.
- White text (`#FFFFFF`) by default for self + other players.
- **Party members: BLUE name** (`#5599FF`) — NOT green. Confirmed: "nickname of adventurers in same party display in blue."
- Guild members: White name (same as default) — identified by guild emblem + guild name line, NOT by name color.
- GMs: Yellow (`#FFFF00`) — controlled via `<admin>` tags in clientinfo.xml.
- **PvP: NO name color change** — ally/enemy identification is by guild emblem only. No red hostile names in classic RO.
- **Display format (2 lines):**
  - Line 1: `CharacterName (PartyName)` — party name in parentheses if in party.
  - Line 2: `GuildName [GuildTitle]` — guild rank/title in brackets, 24x24 emblem to left.
- Name always faces the camera (3D-to-2D matrix projection, billboarded).
- **Font:** Gulim (official RO client font), Arial 12px fallback.
- **Shadow/Outline:** 4-pass black outline — text drawn 4× at ±1px cardinal offsets in `#000000`, then colored text on top. Creates crisp 1px stroke effect.
- **No background** — floating text with outline only.
- `/showname` command toggles bold font with guild title shown above name.

### 1.2 Monster Name Tags (UPDATED — verified via roBrowser + rAthena)

- Monster names are **HOVER-ONLY** — you must move your mouse cursor over the monster to see its name. They are NOT always visible. (Third-party GRF mods can force always-visible, but this is non-standard.)
- Name color is **level-based** (player level vs monster level):
  - **Grey** (`#C0C0C0`): Monster is significantly lower level than player.
  - **White** (`#FFFFFF`): Monster is roughly the same level.
  - **Red** (`#FF0000`): Monster is significantly higher level.
  - Base color (roBrowser default): Light pink (`#FFC6C6`).
- Boss/MVP monsters have no inherent name color difference — same level-based coloring. Distinguished by larger sprites, unique appearance, and boss protocol.
- Dead monsters: name tag disappears as sprite fades out.
- Monster HP bar (optional, toggled via `/monsterhp`):
  - Pink (`#FF00E7`) at normal HP, Yellow (`#FFFF00`) below 25%.
  - Border: dark blue (`#10189C`), background: dark grey (`#424242`).
  - ~60px wide × 5px tall, above sprite, centered.
- Name always faces camera.

### 1.3 NPC Name Tags (UPDATED — verified via roBrowser)

- NPC names are **HOVER-ONLY** — same as monsters, visible only when mouse cursor is over the NPC.
- **Light blue** text (`#94BDF7`) — NOT green. This distinguishes NPCs from white player names and pink/red monster names.
- Some NPCs show a title (e.g., "Tool Dealer", "Kafra Service") but this is displayed in the NPC dialogue, not floating above.
- Name always faces camera.
- No background — same floating text with black outline as all other entities.

---

## 2. Current Blueprint Implementation

### 2.1 WBP_PlayerNameTag Widget

The current system uses a UMG widget attached as a WidgetComponent on each actor:

- **Widget:** `WBP_PlayerNameTag` -- a UMG widget containing a single Text Block.
- **Function:** `SetPlayerName(FText)` -- sets the displayed text.
- **Attachment:** Added as a WidgetComponent on:
  - `BP_MMOCharacter` (local player)
  - `BP_OtherPlayerCharacter` (remote players)
  - `BP_EnemyCharacter` (monsters)
- **Widget space:** Screen mode (always faces camera automatically).
- **Position:** Approximately 120 units above the capsule center.

### 2.2 Name Tag Setup Flow (Blueprint)

**BP_MMOCharacter BeginPlay:**
```
Get NameTagWidget -> Cast To WBP_PlayerNameTag -> SetPlayerName(character name from GameInstance)
```

**BP_OtherPlayerCharacter BeginPlay:**
```
Delay -> Get NameTagWidget -> Cast To WBP_PlayerNameTag -> SetPlayerName(PlayerName)
```
The Delay node is used to allow the PlayerName variable to be set before the name tag reads it.

**BP_EnemyCharacter BeginPlay:**
```
Delay -> Get NameTagWidget -> Cast To WBP_PlayerNameTag -> SetPlayerName("EnemyName Lv.X")
```

### 2.3 Problems with Current Approach

| Problem | Impact |
|---------|--------|
| One UMG widget per actor | UMG overhead scales linearly with entity count |
| WidgetComponent creation/destruction | GC pressure on spawn/despawn of entities |
| No color differentiation | All names are the same color regardless of entity type |
| No party/guild coloring | No social context in name tags |
| Cast chains in Blueprint | Fragile, must be updated for every new NPC type |
| No sub-text support | Cannot show guild names, NPC titles, etc. |
| No distance culling | Names render even for actors far off-screen or very distant |

### 2.4 Click-to-Interact Current Flow

The current interaction system lives in Blueprint on `BP_MMOCharacter`:

- `IA_Attack` input action triggers a cast chain ending at `TryInteractWithNPC(HitActor)`.
- `TryInteractWithNPC` performs sequential casts:
  - `Cast to AShopNPC` -> call `Interact()`
  - `Cast to AKafraNPC` -> call `Interact()`
- Each NPC has a 1-second spam guard (cooldown between interactions).
- NPC range check: client-side `InteractionRadius` (not strictly enforced). Server validates zone membership.
- No walk-to-interact behavior: if out of range, interaction silently fails.
- No unified priority system: NPC vs enemy vs player click handling is ad-hoc.

---

## 3. New C++ Architecture: UNameTagSubsystem

### 3.1 Design Rationale

Replace N per-actor WidgetComponents with a single Slate OnPaint overlay widget that renders ALL name tags in one pass. This follows the same pattern as `WorldHealthBarSubsystem` (Z=8), which already renders floating HP/SP bars via OnPaint.

**Advantages:**
- Single widget renders all name tags -- O(1) widget overhead instead of O(N).
- No per-actor WidgetComponent creation/destruction.
- Consistent with existing Slate overlay patterns in the project.
- Easy to add color differentiation, sub-text, distance culling.
- Centralized registration API -- no scattered Blueprint logic.

### 3.2 UNameTagSubsystem Header

```cpp
// NameTagSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NameTagSubsystem.generated.h"

class SNameTagOverlay;

UCLASS()
class SABRIMMO_API UNameTagSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // --- Registration API ---

    /** Register an entity for name tag rendering. Returns a handle for future updates. */
    int32 RegisterEntity(
        AActor* Actor,
        const FString& DisplayName,
        const FString& SubText,          // Guild name, "Lv.X", NPC title
        const FLinearColor& NameColor,
        const FLinearColor& SubTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
        float VerticalOffset = 120.f
    );

    /** Unregister an entity. Safe to call with an actor that was never registered. */
    void UnregisterEntity(AActor* Actor);

    /** Update the displayed name for a registered entity. */
    void UpdateName(AActor* Actor, const FString& NewName);

    /** Update the sub-text (guild name, NPC title, etc.) for a registered entity. */
    void UpdateSubText(AActor* Actor, const FString& NewSubText);

    /** Update the name color (e.g., when joining a party, entering PvP zone). */
    void UpdateNameColor(AActor* Actor, const FLinearColor& NewColor);

    /** Show or hide the name tag for a specific entity (e.g., hide on death). */
    void SetVisible(AActor* Actor, bool bVisible);

    // --- Lifecycle ---

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // --- Internal access for SNameTagOverlay ---

    struct FNameTagEntry
    {
        TWeakObjectPtr<AActor> Actor;
        FString DisplayName;
        FString SubText;
        FLinearColor NameColor;
        FLinearColor SubTextColor;
        float VerticalOffset;
        bool bVisible;
    };

    const TArray<FNameTagEntry>& GetEntries() const { return Entries; }

private:
    TArray<FNameTagEntry> Entries;
    TSharedPtr<SNameTagOverlay> OverlayWidget;

    /** Find the index of an entry by actor. Returns INDEX_NONE if not found. */
    int32 FindEntryIndex(AActor* Actor) const;

    /** Gate: only create overlay when socket is connected (game world, not login). */
    bool IsInGameWorld() const;
};
```

### 3.3 UNameTagSubsystem Implementation

```cpp
// NameTagSubsystem.cpp

#include "NameTagSubsystem.h"
#include "SNameTagOverlay.h"
#include "MMOGameInstance.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"

void UNameTagSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (!IsInGameWorld()) return;

    UGameViewportClient* ViewportClient = InWorld.GetGameViewport();
    if (!ViewportClient) return;

    OverlayWidget = SNew(SNameTagOverlay).Subsystem(this);

    ViewportClient->AddViewportWidgetContent(
        SNew(SWeakWidget).PossiblyNullContent(OverlayWidget.ToSharedRef()),
        7 // Z-order: below WorldHealthBarSubsystem (Z=8)
    );
}

void UNameTagSubsystem::Deinitialize()
{
    if (OverlayWidget.IsValid())
    {
        UWorld* World = GetWorld();
        if (World)
        {
            UGameViewportClient* ViewportClient = World->GetGameViewport();
            if (ViewportClient)
            {
                ViewportClient->RemoveViewportWidgetContent(
                    SNew(SWeakWidget).PossiblyNullContent(OverlayWidget.ToSharedRef())
                );
            }
        }
        OverlayWidget.Reset();
    }

    Entries.Empty();
    Super::Deinitialize();
}

int32 UNameTagSubsystem::RegisterEntity(
    AActor* Actor,
    const FString& DisplayName,
    const FString& SubText,
    const FLinearColor& NameColor,
    const FLinearColor& SubTextColor,
    float VerticalOffset)
{
    if (!Actor) return INDEX_NONE;

    // Prevent duplicate registration
    int32 ExistingIndex = FindEntryIndex(Actor);
    if (ExistingIndex != INDEX_NONE)
    {
        // Update existing entry instead
        Entries[ExistingIndex].DisplayName = DisplayName;
        Entries[ExistingIndex].SubText = SubText;
        Entries[ExistingIndex].NameColor = NameColor;
        Entries[ExistingIndex].SubTextColor = SubTextColor;
        Entries[ExistingIndex].VerticalOffset = VerticalOffset;
        Entries[ExistingIndex].bVisible = true;
        return ExistingIndex;
    }

    FNameTagEntry Entry;
    Entry.Actor = Actor;
    Entry.DisplayName = DisplayName;
    Entry.SubText = SubText;
    Entry.NameColor = NameColor;
    Entry.SubTextColor = SubTextColor;
    Entry.VerticalOffset = VerticalOffset;
    Entry.bVisible = true;

    return Entries.Add(Entry);
}

void UNameTagSubsystem::UnregisterEntity(AActor* Actor)
{
    int32 Index = FindEntryIndex(Actor);
    if (Index != INDEX_NONE)
    {
        Entries.RemoveAtSwap(Index);
    }
}

void UNameTagSubsystem::UpdateName(AActor* Actor, const FString& NewName)
{
    int32 Index = FindEntryIndex(Actor);
    if (Index != INDEX_NONE)
    {
        Entries[Index].DisplayName = NewName;
    }
}

void UNameTagSubsystem::UpdateSubText(AActor* Actor, const FString& NewSubText)
{
    int32 Index = FindEntryIndex(Actor);
    if (Index != INDEX_NONE)
    {
        Entries[Index].SubText = NewSubText;
    }
}

void UNameTagSubsystem::UpdateNameColor(AActor* Actor, const FLinearColor& NewColor)
{
    int32 Index = FindEntryIndex(Actor);
    if (Index != INDEX_NONE)
    {
        Entries[Index].NameColor = NewColor;
    }
}

void UNameTagSubsystem::SetVisible(AActor* Actor, bool bVisible)
{
    int32 Index = FindEntryIndex(Actor);
    if (Index != INDEX_NONE)
    {
        Entries[Index].bVisible = bVisible;
    }
}

int32 UNameTagSubsystem::FindEntryIndex(AActor* Actor) const
{
    for (int32 i = 0; i < Entries.Num(); ++i)
    {
        if (Entries[i].Actor.Get() == Actor)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

bool UNameTagSubsystem::IsInGameWorld() const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
    return GI && GI->IsSocketConnected();
}
```

---

## 4. SNameTagOverlay (Slate OnPaint Widget)

### 4.1 Header

```cpp
// SNameTagOverlay.h

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UNameTagSubsystem;

class SNameTagOverlay : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNameTagOverlay) {}
        SLATE_ARGUMENT(UNameTagSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled
    ) const override;

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
    TWeakObjectPtr<UNameTagSubsystem> Subsystem;

    /** Cached fonts -- initialized once in Construct. */
    FSlateFontInfo NameFont;
    FSlateFontInfo SubTextFont;

    /** Distance beyond which name tags are culled (in UE units). */
    static constexpr float MaxRenderDistance = 5000.f;

    /** Vertical pixel offset between name and sub-text lines. */
    static constexpr float SubTextVerticalGap = 2.f;
};
```

### 4.2 Implementation

```cpp
// SNameTagOverlay.cpp

#include "SNameTagOverlay.h"
#include "NameTagSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

void SNameTagOverlay::Construct(const FArguments& InArgs)
{
    Subsystem = InArgs._Subsystem;

    // Cache fonts once
    NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
    SubTextFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

    SetVisibility(EVisibility::HitTestInvisible);
}

FVector2D SNameTagOverlay::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    // Fill the entire viewport
    return FVector2D(1920.f, 1080.f);
}

int32 SNameTagOverlay::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    UNameTagSubsystem* Sub = Subsystem.Get();
    if (!Sub) return LayerId;

    UWorld* World = Sub->GetWorld();
    if (!World) return LayerId;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return LayerId;

    APawn* LocalPawn = PC->GetPawn();

    const TSharedRef<FSlateFontMeasure> FontMeasure =
        FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    // Shadow parameters
    const FVector2D ShadowOffset(1.f, 1.f);
    const FLinearColor ShadowColor(0.f, 0.f, 0.f, 0.8f);

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    const TArray<UNameTagSubsystem::FNameTagEntry>& Entries = Sub->GetEntries();

    for (const auto& Entry : Entries)
    {
        // --- Validity checks ---
        AActor* Actor = Entry.Actor.Get();
        if (!Actor) continue;
        if (!Entry.bVisible) continue;

        // --- Distance culling ---
        FVector ActorLocation = Actor->GetActorLocation();
        float Distance = FVector::Dist(CameraLocation, ActorLocation);
        if (Distance > MaxRenderDistance) continue;

        // --- World-to-screen projection ---
        FVector WorldPosition = ActorLocation + FVector(0.f, 0.f, Entry.VerticalOffset);
        FVector2D ScreenPosition;
        bool bOnScreen = UGameplayStatics::ProjectWorldToScreen(PC, WorldPosition, ScreenPosition);
        if (!bOnScreen) continue;

        // Convert from player screen to widget local space
        FVector2D LocalPosition = AllottedGeometry.AbsoluteToLocal(ScreenPosition);

        // --- Draw name text ---
        FString NameText = Entry.DisplayName;
        FVector2D NameSize = FontMeasure->Measure(NameText, NameFont);
        FVector2D NameDrawPos(LocalPosition.X - NameSize.X * 0.5f, LocalPosition.Y - NameSize.Y);

        // Shadow pass
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(NameSize, FSlateLayoutTransform(NameDrawPos + ShadowOffset)),
            NameText,
            NameFont,
            ESlateDrawEffect::None,
            ShadowColor
        );

        // Foreground pass
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(NameSize, FSlateLayoutTransform(NameDrawPos)),
            NameText,
            NameFont,
            ESlateDrawEffect::None,
            Entry.NameColor
        );

        // --- Draw sub-text (if present) ---
        if (!Entry.SubText.IsEmpty())
        {
            FVector2D SubSize = FontMeasure->Measure(Entry.SubText, SubTextFont);
            FVector2D SubDrawPos(
                LocalPosition.X - SubSize.X * 0.5f,
                NameDrawPos.Y + NameSize.Y + SubTextVerticalGap
            );

            // Shadow pass
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId,
                AllottedGeometry.ToPaintGeometry(SubSize, FSlateLayoutTransform(SubDrawPos + ShadowOffset)),
                Entry.SubText,
                SubTextFont,
                ESlateDrawEffect::None,
                ShadowColor
            );

            // Foreground pass
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(SubSize, FSlateLayoutTransform(SubDrawPos)),
                Entry.SubText,
                SubTextFont,
                ESlateDrawEffect::None,
                Entry.SubTextColor
            );
        }
    }

    return LayerId + 2; // Used 2 sub-layers (shadow + foreground)
}
```

### 4.3 Rendering Pipeline (Step-by-Step)

For each registered entity, every frame:

1. **Validity check:** Confirm `TWeakObjectPtr<AActor>` is still valid. Skip stale entries.
2. **Visibility check:** Skip entries where `bVisible == false` (dead enemies, etc.).
3. **Distance culling:** Compute distance from camera to actor. Skip if beyond `MaxRenderDistance` (5000 UE units).
4. **World-to-screen projection:** Use `UGameplayStatics::ProjectWorldToScreen` on `ActorLocation + (0, 0, VerticalOffset)`. If the function returns `false`, the point is behind the camera -- skip.
5. **Coordinate conversion:** Convert screen-space absolute position to widget-local space via `AllottedGeometry.AbsoluteToLocal()`.
6. **Measure text:** Use `FSlateFontMeasure::Measure()` to get the pixel dimensions of the name string.
7. **Center horizontally:** Draw position X = `ProjectedX - TextWidth / 2`.
8. **Draw shadow:** Render the same text at `(X+1, Y+1)` in black with 0.8 alpha on `LayerId`.
9. **Draw foreground:** Render the text at `(X, Y)` in the entry's `NameColor` on `LayerId + 1`.
10. **Draw sub-text:** If `SubText` is non-empty, repeat steps 6-9 below the name text using `SubTextFont` and `SubTextColor`.

---

## 5. Name Tag Colors & Styling

### 5.1 Color Constants

```cpp
namespace NameTagColors
{
    // Player names
    static const FLinearColor PlayerDefault  (0.96f, 0.90f, 0.78f, 1.f); // Warm white (ROColors::TextPrimary)
    static const FLinearColor PartyMember    (0.30f, 0.90f, 0.30f, 1.f); // Green
    static const FLinearColor GuildMember    (0.40f, 0.70f, 1.00f, 1.f); // Light blue
    static const FLinearColor EnemyPlayer    (0.90f, 0.20f, 0.20f, 1.f); // Red (PvP zones)

    // Monsters
    static const FLinearColor MonsterDefault (0.96f, 0.90f, 0.78f, 1.f); // Warm white
    static const FLinearColor MonsterBoss    (1.00f, 0.85f, 0.30f, 1.f); // Gold
    static const FLinearColor MonsterMVP     (1.00f, 0.30f, 0.30f, 1.f); // Red

    // NPCs
    static const FLinearColor NPCName        (0.40f, 0.85f, 0.40f, 1.f); // Green

    // Sub-text (guild names, NPC titles, level text)
    static const FLinearColor SubTextDefault (0.70f, 0.70f, 0.70f, 1.f); // Light gray
    static const FLinearColor GuildNameText  (0.50f, 0.75f, 0.90f, 1.f); // Muted blue
    static const FLinearColor NPCTitleText   (0.60f, 0.80f, 0.60f, 1.f); // Muted green
}
```

### 5.2 Font Specifications

| Element | Font Style | Size | Shadow |
|---------|-----------|------|--------|
| Player/Monster name | Bold | 10 | (1,1) black @ 0.8 alpha |
| NPC name | Bold | 10 | (1,1) black @ 0.8 alpha |
| Sub-text (guild, title, level) | Regular | 8 | (1,1) black @ 0.8 alpha |

### 5.3 Layout

```
                 [Name Text]          <- Bold 10, centered horizontally
              [Sub-Text / Title]      <- Regular 8, centered horizontally, 2px gap below name
```

The name is drawn above the projected screen point. Sub-text is drawn below the name. Both are horizontally centered on the projected world position.

---

## 6. Registration Points

### 6.1 Local Player (ASabriMMOCharacter)

In `ASabriMMOCharacter::BeginPlay()`, after the character data is available:

```cpp
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetGameInstance());
    FString CharName = GI ? GI->GetCharacterName() : TEXT("Player");

    NameTags->RegisterEntity(
        this,
        CharName,
        TEXT(""),                          // No guild yet
        NameTagColors::PlayerDefault,
        NameTagColors::SubTextDefault,
        120.f                              // Above capsule center
    );
}
```

### 6.2 Remote Players (UOtherPlayerSubsystem)

When a remote player is spawned (on `player:joined` or `player:existing` event):

```cpp
// In the spawn function, after setting up the AMMORemotePlayer actor:
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    FLinearColor NameColor = NameTagColors::PlayerDefault;

    // Future: check if in same party -> NameTagColors::PartyMember
    // Future: check if in same guild -> NameTagColors::GuildMember
    // Future: check if PvP zone -> NameTagColors::EnemyPlayer

    NameTags->RegisterEntity(
        RemotePlayerActor,
        PlayerName,
        TEXT(""),                          // Guild name when implemented
        NameColor,
        NameTagColors::SubTextDefault,
        120.f
    );
}
```

On remote player removal (`player:left`):

```cpp
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    NameTags->UnregisterEntity(RemotePlayerActor);
}
```

### 6.3 Enemies (UEnemySubsystem)

When an enemy is spawned (on `enemy:spawn` event):

```cpp
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    FString DisplayName = FString::Printf(TEXT("%s Lv.%d"), *EnemyName, EnemyLevel);

    // Determine color based on enemy type
    FLinearColor NameColor = NameTagColors::MonsterDefault;
    if (bIsMVP)
        NameColor = NameTagColors::MonsterMVP;
    else if (bIsBoss)
        NameColor = NameTagColors::MonsterBoss;

    NameTags->RegisterEntity(
        EnemyActor,
        DisplayName,
        TEXT(""),                          // No sub-text for monsters
        NameColor,
        NameTagColors::SubTextDefault,
        100.f                              // Slightly lower offset for monsters
    );
}
```

On enemy death: hide rather than unregister (actor may linger during death animation):

```cpp
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    NameTags->SetVisible(EnemyActor, false);
}
```

On enemy removal (despawn after death animation):

```cpp
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    NameTags->UnregisterEntity(EnemyActor);
}
```

### 6.4 NPCs (AShopNPC, AKafraNPC, future AMMONPC)

In each NPC's `BeginPlay()`:

```cpp
// AShopNPC::BeginPlay()
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    NameTags->RegisterEntity(
        this,
        NPCName,                           // e.g., "Alberta"
        NPCTitle,                          // e.g., "Tool Dealer"
        NameTagColors::NPCName,
        NameTagColors::NPCTitleText,
        120.f
    );
}

// AKafraNPC::BeginPlay()
UNameTagSubsystem* NameTags = GetWorld()->GetSubsystem<UNameTagSubsystem>();
if (NameTags)
{
    NameTags->RegisterEntity(
        this,
        TEXT("Kafra"),
        TEXT("Kafra Service"),
        NameTagColors::NPCName,
        NameTagColors::NPCTitleText,
        120.f
    );
}
```

NPCs are static and never unregister -- they exist for the lifetime of the level.

### 6.5 Registration Summary

| Entity Type | Register When | Unregister When | Color | Sub-Text |
|-------------|--------------|-----------------|-------|----------|
| Local player | `ASabriMMOCharacter::BeginPlay` | Never (lifetime of pawn) | `PlayerDefault` | Guild name (future) |
| Remote player | `player:joined` / `player:existing` | `player:left` | `PlayerDefault` / `PartyMember` | Guild name (future) |
| Enemy | `enemy:spawn` | `enemy:death` (hide) / despawn (remove) | `MonsterDefault` / `MonsterBoss` / `MonsterMVP` | None |
| Shop NPC | `AShopNPC::BeginPlay` | Never | `NPCName` | NPC title |
| Kafra NPC | `AKafraNPC::BeginPlay` | Never | `NPCName` | "Kafra Service" |

---

## 7. Unified Click-to-Interact Pipeline

### 7.1 Design Goals

Replace the ad-hoc Blueprint cast chains with a single, extensible C++ click handler in `UPlayerInputSubsystem` that:

1. Performs a single hit test under the cursor.
2. Identifies the entity type via C++ casts in a defined priority order.
3. Routes to the correct action (interact, attack, move).
4. Supports walk-to-interact for out-of-range NPCs.

### 7.2 Entity Click Priority (RO Classic Behavior)

In Ragnarok Online, left-click behavior follows this priority:

| Priority | Entity Type | Action | Notes |
|----------|------------|--------|-------|
| 1 | NPC | Interact (walk-to if needed) | Never attack NPCs |
| 2 | Enemy | Start attacking | Auto-attack loop |
| 3 | Other Player | Context menu (future) | No action currently |
| 4 | Ground | Click-to-move | Pathfind to click point |

### 7.3 HandleLeftClick Implementation

```cpp
// In UPlayerInputSubsystem (or new UInteractionSubsystem)

void UPlayerInputSubsystem::HandleLeftClick()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    FHitResult Hit;
    bool bHit = PC->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        true,  // bTraceComplex
        Hit
    );

    if (!bHit || !Hit.bBlockingHit)
    {
        return; // Click hit nothing
    }

    AActor* HitActor = Hit.GetActor();
    if (!HitActor)
    {
        // Hit geometry with no actor -- treat as ground click
        ClickMoveToGround(Hit.Location);
        return;
    }

    // --- Priority 1: NPCs (always interact, never attack) ---
    if (AShopNPC* Shop = Cast<AShopNPC>(HitActor))
    {
        InteractWithNPC(Shop);
        return;
    }
    if (AKafraNPC* Kafra = Cast<AKafraNPC>(HitActor))
    {
        InteractWithNPC(Kafra);
        return;
    }
    // Future: generic AMMONPC base class
    // if (AMMONPC* NPC = Cast<AMMONPC>(HitActor))
    // {
    //     InteractWithNPC(NPC);
    //     return;
    // }

    // --- Priority 2: Enemies (attack) ---
    if (AMMOEnemyActor* Enemy = Cast<AMMOEnemyActor>(HitActor))
    {
        StartAttackingEnemy(Enemy);
        return;
    }

    // --- Priority 3: Other Players (future PvP / context menu) ---
    if (AMMORemotePlayer* OtherPlayer = Cast<AMMORemotePlayer>(HitActor))
    {
        // PvP is not yet implemented.
        // Future: show right-click context menu (party invite, trade, whisper).
        return;
    }

    // --- Priority 4: Ground (movement) ---
    ClickMoveToGround(Hit.Location);
}
```

### 7.4 NPC Interaction Spam Guard

```cpp
// Member variable
TMap<TWeakObjectPtr<AActor>, double> LastInteractionTime;

bool UPlayerInputSubsystem::CanInteract(AActor* NPC) const
{
    const double* LastTime = LastInteractionTime.Find(NPC);
    if (LastTime && (FPlatformTime::Seconds() - *LastTime) < 1.0)
    {
        return false; // 1-second cooldown
    }
    return true;
}

void UPlayerInputSubsystem::RecordInteraction(AActor* NPC)
{
    LastInteractionTime.FindOrAdd(NPC) = FPlatformTime::Seconds();
}
```

### 7.5 Future Extensibility

When new interactable entity types are added (quest NPCs, mailboxes, storage NPCs, etc.), they should either:

1. **Inherit from a common `AMMONPC` base class** with a virtual `Interact()` method, OR
2. **Implement a `BPI_Interactable` interface** with an `Execute_Interact()` function.

Option 2 (interface) is preferred per the project's design patterns ("Interfaces over cast chains"). The HandleLeftClick would then check:

```cpp
if (HitActor->Implements<UBPI_Interactable>())
{
    IBPI_Interactable::Execute_Interact(HitActor, LocalPawn);
    return;
}
```

This eliminates the need for per-NPC-type casts entirely.

---

## 8. Walk-to-Interact System

### 8.1 Problem

In RO Classic, clicking an NPC that is out of range causes the player to walk to the NPC and interact upon arriving. The current implementation either silently fails or requires the player to manually walk close and click again.

### 8.2 Implementation

```cpp
// Member variables for pending interaction
TWeakObjectPtr<AActor> PendingInteractionTarget;
float PendingInteractionRange = 300.f;

void UPlayerInputSubsystem::InteractWithNPC(AActor* NPC)
{
    if (!NPC) return;
    if (!CanInteract(NPC)) return;

    APawn* LocalPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (!LocalPawn) return;

    float InteractionRange = 300.f; // Default; can be overridden per NPC via a property

    float Distance = FVector::Dist(
        LocalPawn->GetActorLocation(),
        NPC->GetActorLocation()
    );

    if (Distance <= InteractionRange)
    {
        // In range -- interact immediately
        ExecuteInteraction(NPC);
    }
    else
    {
        // Out of range -- walk to NPC, then interact
        PendingInteractionTarget = NPC;
        PendingInteractionRange = InteractionRange;

        // Issue move command toward the NPC
        MoveToLocation(NPC->GetActorLocation());
    }
}

void UPlayerInputSubsystem::ExecuteInteraction(AActor* NPC)
{
    RecordInteraction(NPC);

    if (AShopNPC* Shop = Cast<AShopNPC>(NPC))
    {
        Shop->Interact();
    }
    else if (AKafraNPC* Kafra = Cast<AKafraNPC>(NPC))
    {
        Kafra->Interact();
    }
    // Future: AMMONPC base class or BPI_Interactable interface

    PendingInteractionTarget = nullptr;
}
```

### 8.3 Tick-Based Arrival Check

Each tick (or on a timer), check if the player has arrived within range of the pending interaction target:

```cpp
void UPlayerInputSubsystem::CheckPendingInteraction()
{
    AActor* Target = PendingInteractionTarget.Get();
    if (!Target)
    {
        PendingInteractionTarget = nullptr;
        return;
    }

    APawn* LocalPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (!LocalPawn) return;

    float Distance = FVector::Dist(
        LocalPawn->GetActorLocation(),
        Target->GetActorLocation()
    );

    if (Distance <= PendingInteractionRange)
    {
        ExecuteInteraction(Target);
    }
}
```

### 8.4 Cancellation

The pending interaction is cancelled when:

- The player clicks a different target (new click overrides pending).
- The player clicks ground to move elsewhere (explicit cancel).
- The target NPC is destroyed or becomes invalid (weak pointer goes stale).
- The player is stunned, frozen, or otherwise CC'd.

```cpp
void UPlayerInputSubsystem::CancelPendingInteraction()
{
    PendingInteractionTarget = nullptr;
}
```

---

## 9. Collision Setup for Clickable Actors

### 9.1 Requirements

All clickable actors MUST have collision set up so that `GetHitResultUnderCursorByChannel(ECC_Visibility)` can detect them.

### 9.2 Per-Actor Collision Configuration

| Actor Type | Collision Source | Visibility Channel |
|-----------|-----------------|-------------------|
| `AMMOEnemyActor` | Capsule component (inherits from ACharacter) | `ECR_Block` |
| `AMMORemotePlayer` | Capsule component (inherits from ACharacter) | `ECR_Block` |
| `AShopNPC` | Mesh component | Must set `ECR_Block` on `ECC_Visibility` |
| `AKafraNPC` | Mesh component | Must set `ECR_Block` on `ECC_Visibility` |
| Ground/Landscape | Landscape actor | `ECR_Block` (default) |

### 9.3 NPC Collision Setup (Code)

For NPC actors that use a static mesh or skeletal mesh:

```cpp
// In NPC constructor or BeginPlay:
UMeshComponent* Mesh = GetMeshComponent(); // or FindComponentByClass<UStaticMeshComponent>()
if (Mesh)
{
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
```

If an NPC uses a capsule for physical blocking:

```cpp
UCapsuleComponent* Capsule = GetCapsuleComponent();
if (Capsule)
{
    Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
```

### 9.4 Collision Debugging

If click detection is not working for a specific actor type:

1. Enable `show collision` in the editor viewport.
2. Verify the actor has a component responding to `ECC_Visibility`.
3. Check that `SetCollisionEnabled` is not set to `NoCollision`.
4. Ensure no parent class is overriding collision settings in BeginPlay.

---

## 10. Files to Create

| File | Description |
|------|-------------|
| `client/SabriMMO/Source/SabriMMO/UI/NameTagSubsystem.h` | `UNameTagSubsystem` header: registration API, `FNameTagEntry` struct |
| `client/SabriMMO/Source/SabriMMO/UI/NameTagSubsystem.cpp` | `UNameTagSubsystem` implementation: lifecycle, register/unregister, find |
| `client/SabriMMO/Source/SabriMMO/UI/SNameTagOverlay.h` | `SNameTagOverlay` header: Slate widget for OnPaint rendering |
| `client/SabriMMO/Source/SabriMMO/UI/SNameTagOverlay.cpp` | `SNameTagOverlay` implementation: projection, text drawing, culling |

### File Size Estimates

| File | Estimated Lines |
|------|----------------|
| `NameTagSubsystem.h` | ~80 |
| `NameTagSubsystem.cpp` | ~150 |
| `SNameTagOverlay.h` | ~40 |
| `SNameTagOverlay.cpp` | ~120 |
| **Total new code** | **~390** |

---

## 11. Files to Modify

| File | Changes |
|------|---------|
| `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.cpp` | Add `UNameTagSubsystem::RegisterEntity()` call in `BeginPlay()` for local player |
| `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.cpp` | (or equivalent) Add `RegisterEntity()` on remote player spawn, `UnregisterEntity()` on removal |
| `client/SabriMMO/Source/SabriMMO/UI/EnemySubsystem.cpp` | (or equivalent) Add `RegisterEntity()` on enemy spawn, `SetVisible(false)` on death, `UnregisterEntity()` on despawn |
| `client/SabriMMO/Source/SabriMMO/ShopNPC.cpp` | Add `RegisterEntity()` in `BeginPlay()` |
| `client/SabriMMO/Source/SabriMMO/KafraNPC.cpp` | Add `RegisterEntity()` in `BeginPlay()` |
| `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs` | No changes expected (Slate modules already included) |

### Interaction Pipeline Modifications

| File | Changes |
|------|---------|
| `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.cpp` | Move NPC interaction logic from Blueprint `TryInteractWithNPC` to C++ `HandleLeftClick` (or `UPlayerInputSubsystem`) |

Note: The interaction pipeline refactor can either live in a new `UInteractionSubsystem` or be added to the existing `UPlayerInputSubsystem` (if one is created as part of the broader migration). The exact location depends on the state of the input migration at the time of implementation.

---

## 12. Blueprint Cleanup (Phase 6)

After the C++ systems are verified working, the following Blueprint assets should be cleaned up:

### Remove from Blueprints

| Blueprint | Component/Logic to Remove |
|-----------|--------------------------|
| `BP_MMOCharacter` | Remove `NameTagWidget` WidgetComponent; remove `SetPlayerName` logic from BeginPlay |
| `BP_OtherPlayerCharacter` | Remove `NameTagWidget` WidgetComponent; remove `SetPlayerName` logic from BeginPlay |
| `BP_EnemyCharacter` | Remove `NameTagWidget` WidgetComponent; remove `SetPlayerName` logic from BeginPlay |
| `BP_MMOCharacter` | Remove `TryInteractWithNPC` function and its call from `IA_Attack` (if interaction moves to C++) |

### Delete Blueprint Assets

| Asset | Reason |
|-------|--------|
| `WBP_PlayerNameTag` | Fully replaced by `SNameTagOverlay` OnPaint rendering |

### Verification Before Deletion

Before removing any Blueprint component:

1. Confirm the C++ name tag renders correctly for that actor type.
2. Confirm no other Blueprint logic references `NameTagWidget` or `WBP_PlayerNameTag`.
3. Test with both the old and new systems disabled independently.

---

## 13. Performance Considerations

### 13.1 Current System (Per-Actor WidgetComponent)

- Each `WidgetComponent` creates a render target and a full UMG widget tree.
- With 50 entities on screen: 50 UMG widget trees, 50 render targets, 50 billboard updates per frame.
- Memory: ~2-4 KB per widget instance (UMG overhead).
- CPU: UMG layout + render per widget per frame.

### 13.2 New System (Single OnPaint Overlay)

- One Slate widget. One `OnPaint` call per frame.
- For each visible entity: 1 world-to-screen projection, 2-4 `MakeText` calls (name shadow + foreground, optional sub-text shadow + foreground).
- `FSlateDrawElement::MakeText` is highly optimized -- batched by the Slate renderer.
- Distance culling eliminates off-screen and far-away entities before any drawing.
- Memory: effectively zero per-entity overhead (just the `FNameTagEntry` struct, ~100 bytes each).

### 13.3 Expected Improvement

| Metric | Old (50 entities) | New (50 entities) | Improvement |
|--------|-------------------|-------------------|-------------|
| Widget instances | 50 | 1 | 50x fewer |
| Render targets | 50 | 0 | Eliminated |
| Draw calls (text) | 50 (UMG) | ~100-200 (batched MakeText) | Batched, lower overhead |
| Memory per entity | ~2-4 KB | ~100 bytes | ~20-40x less |

### 13.4 Scalability

The OnPaint approach scales linearly with visible entity count, but the per-entity cost is trivially small (one projection + a few text draws). Testing should verify smooth performance with 100+ entities on screen, which is plausible in crowded zones.

---

## 14. Testing Checklist

### Name Tag Rendering

- [ ] Local player name displayed above character head
- [ ] Local player name uses `PlayerDefault` color (warm white)
- [ ] Other player names displayed correctly with correct color
- [ ] Enemy names show "Name Lv.X" format
- [ ] Boss/MVP enemies show gold/red name color
- [ ] NPC names displayed in green
- [ ] NPC sub-text (title) displayed below name in muted green
- [ ] All names face camera correctly (screen-space rendering, no rotation artifacts)
- [ ] Names have black shadow for readability against bright and dark backgrounds
- [ ] Names disappear for dead enemies (bVisible set to false)
- [ ] Names reappear if enemy respawns at same actor (re-registration)
- [ ] Names visible at reasonable distance (up to 5000 UE units)
- [ ] Names culled beyond max render distance (no rendering, no projection)
- [ ] Names culled when actor is behind camera (ProjectWorldToScreen returns false)
- [ ] Names do not overlap with WorldHealthBarSubsystem HP bars (Z-order 7 vs 8)
- [ ] No name tags visible on login screen (gated by `IsSocketConnected()`)

### Registration / Lifecycle

- [ ] Local player name tag registered on BeginPlay
- [ ] Remote player name tag registered on spawn, unregistered on `player:left`
- [ ] Enemy name tag registered on spawn, hidden on death, unregistered on despawn
- [ ] NPC name tags registered on BeginPlay, persist for level lifetime
- [ ] Zone transition: old entries cleared (subsystem re-initializes), new entries registered
- [ ] No crashes from stale `TWeakObjectPtr` after actor destruction
- [ ] Duplicate registration calls update existing entry (no duplicate name tags)

### Click-to-Interact

- [ ] Click NPC within range: interaction triggers immediately
- [ ] Click NPC outside range: player walks to NPC, interaction triggers on arrival
- [ ] Click enemy: auto-attack starts (not interact)
- [ ] Click other player: no action (future PvP placeholder)
- [ ] Click ground: click-to-move triggers
- [ ] Entity priority correct: NPC > Enemy > Player > Ground
- [ ] NPC interaction 1-second spam guard works
- [ ] Walk-to-interact cancelled by clicking a different target
- [ ] Walk-to-interact cancelled by clicking ground
- [ ] Walk-to-interact handles NPC destruction mid-walk (weak pointer invalidation)

### Performance

- [ ] No FPS drop with 50+ entities with name tags
- [ ] No memory growth from repeated entity registration/unregistration
- [ ] Profiler shows single OnPaint call (not N widget updates)
