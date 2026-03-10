# 07 - UI/HUD System: UE5 C++ Implementation Guide (Pure Slate)

> **Scope**: Complete Slate C++ implementation guide for all UI/HUD systems in Sabri_MMO.
> **Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
> **Audience**: AI coding assistant (Claude Code) -- every pattern shown here must be achievable without manual Blueprint input from the user.
> **Reference**: `RagnaCloneDocs/09_UI_UX_System.md` for RO Classic UI/UX specifications.

---

## Table of Contents

1. [Slate Architecture](#1-slate-architecture)
2. [Base Widget Patterns](#2-base-widget-patterns)
3. [Chat System](#3-chat-system)
4. [Minimap](#4-minimap)
5. [World-Space UI](#5-world-space-ui)
6. [Notification System](#6-notification-system)
7. [Window Management](#7-window-management)
8. [Death / Respawn UI](#8-death--respawn-ui)
9. [Adding a New UI Panel (Template)](#9-adding-a-new-ui-panel-template)

---

## 1. Slate Architecture

### 1.1 Why Slate Over UMG

Sabri_MMO uses pure Slate C++ for all UI. UMG (Unreal Motion Graphics / Blueprint Widgets) is never used for in-game HUD panels. The reasons:

| Factor | Slate (C++) | UMG (Blueprints) |
|--------|-------------|-------------------|
| Performance | No UObject reflection overhead, raw C++ rendering | UObject wrapper per widget, GC pressure |
| Multiplayer safety | No global singletons; world-scoped via `UWorldSubsystem` | `UUserWidget` often references global viewport |
| `OnPaint()` control | Full access to `FSlateDrawElement` for batch rendering | Must use Canvas panels or custom `NativePaint` |
| Hot-reload | Live Coding recompiles C++ in-place | Blueprint widgets need editor restart for struct changes |
| Deterministic lifecycle | `Deinitialize()` guaranteed cleanup | Widget GC timing unpredictable |
| Drag-and-drop | Direct `FReply::Handled().CaptureMouse()` | Requires `UDragDropOperation` UObject |

**Rule**: Every new UI panel in Sabri_MMO is a `UWorldSubsystem` + `SCompoundWidget` pair. No exceptions.

### 1.2 Widget Hierarchy

Every UI element follows this ownership chain:

```
UWorld
  -> UWorldSubsystem (C++ class, one per world)
       -> Owns TSharedPtr<SMyWidget>
       -> Owns TSharedPtr<SWidget> ViewportOverlay (SWeakWidget wrapper)
       -> Adds/removes via UGameViewportClient::AddViewportWidgetContent()
            -> SMyWidget : SCompoundWidget
                 -> Child Slate widgets (SBorder, STextBlock, SProgressBar, etc.)
```

**Critical rule**: Never store Slate widgets in `static` or global variables. Every widget is owned by its subsystem, which is owned by its `UWorld`. This ensures multiplayer PIE correctness.

### 1.3 Z-Ordering Table

All viewport widgets are layered by Z-order integer passed to `AddViewportWidgetContent()`. Lower numbers render behind higher numbers.

| Z-Order | Widget | Subsystem | Visibility |
|---------|--------|-----------|------------|
| 5 | Login flow widgets | `LoginFlowSubsystem` | Login level only |
| 8 | World health bars | `WorldHealthBarSubsystem` | Always (game levels) |
| 10 | Basic info (HP/SP/EXP) | `BasicInfoSubsystem` | Always (game levels) |
| 11 | Minimap | `MinimapSubsystem` | Always (game levels) |
| 12 | Combat stats | `CombatStatsSubsystem` | F8 toggle |
| 13 | Chat window | `ChatSubsystem` | Always (game levels) |
| 14 | Inventory | `InventorySubsystem` | F6 toggle |
| 15 | Equipment | `EquipmentSubsystem` | F7 toggle |
| 16 | Hotbar rows | `HotbarSubsystem` | F5 cycle |
| 17 | Party window | `PartySubsystem` | Alt+Z toggle |
| 18 | Notification feed | `NotificationSubsystem` | Always (game levels) |
| 19 | Kafra dialog | `KafraSubsystem` | NPC interaction |
| 19 | Shop dialog | `ShopSubsystem` | NPC interaction |
| 20 | Damage numbers | `DamageNumberSubsystem` | Always (game levels) |
| 20 | Skill tree | `SkillTreeSubsystem` | K toggle |
| 22 | Death/respawn overlay | `DeathSubsystem` | On death |
| 25 | Cast bar overlay | `CastBarSubsystem` | Always (game levels) |
| 30 | Hotbar keybind config | `HotbarSubsystem` | Gear icon |
| 40 | Tooltip overlay | Shared | On hover |
| 50 | Loading overlay | `ZoneTransitionSubsystem` | Zone transitions |

**Z-Order Strategy**:
- **5-10**: Persistent background HUD (always visible, non-interactive overlays)
- **10-20**: Toggleable panels (inventory, equipment, skills, stats, chat)
- **20-25**: World-projected overlays (damage numbers, cast bars)
- **25-30**: Configuration modals (keybind editor)
- **40**: Tooltips (always on top of panels, below loading)
- **50**: Loading overlay (blocks everything)

### 1.4 Input Mode Management

RO-style games require simultaneous game input (click-to-move, camera rotation) and UI input (window interaction, chat typing). Sabri_MMO uses `FInputModeGameAndUI` at all times during gameplay.

```cpp
// Set once in PlayerController::BeginPlay() or Level Blueprint
FInputModeGameAndUI InputMode;
InputMode.SetWidgetToFocus(nullptr);          // No widget locks focus
InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
InputMode.SetHideCursorDuringCapture(false);  // CRITICAL: always show cursor
PlayerController->SetInputMode(InputMode);
PlayerController->bShowMouseCursor = true;
```

**Input priority stack** (highest to lowest):
1. Modal dialogs (death overlay, NPC dialog, trade confirmation) -- consume all input
2. Chat input field when focused -- consumes keyboard only
3. Topmost interactive panel at click position -- consumes that click
4. Game world -- receives all remaining input (click-to-move, click-to-attack)

**Key rule**: Widgets that do NOT need to consume clicks in empty areas must return `FReply::Unhandled()` from `OnMouseButtonDown`. Only consume the click if it hits an interactive element (title bar drag, button, slot).

### 1.5 Multiplayer-Safe Widget Rules

These rules are mandatory for every widget and subsystem. Violating them causes PIE instance 2+ to malfunction.

1. **NEVER use `GEngine->GameViewport`** -- it points to PIE-0 only. Always use `World->GetGameViewport()`.
2. **NEVER use `GEngine->AddOnScreenDebugMessage`** -- it is global. Use `UE_LOG` or widget text.
3. **Always access world-scoped objects through `GetWorld()`** -- never through global pointers.
4. **No `static` Slate widget pointers** -- each PIE world needs its own widget instances.
5. **Every new widget must be tested with 2+ PIE instances** before considering it done.

---

## 2. Base Widget Patterns

### 2.1 RO Classic Color Palette

All Sabri_MMO panels share a common color palette defined in a namespace. Use `FLinearColor` directly (not `FColor`, which applies sRGB-to-linear conversion making colors too dark).

```cpp
// MMOUIColors.h -- Shared color palette for all Sabri_MMO Slate widgets
#pragma once
#include "Math/Color.h"

namespace ROColors
{
    // ---- Panel backgrounds ----
    static const FLinearColor PanelBrown     (0.43f, 0.29f, 0.17f, 1.0f);  // Main window fill
    static const FLinearColor PanelDark      (0.22f, 0.14f, 0.08f, 1.0f);  // Title bar, inset areas
    static const FLinearColor PanelMedium    (0.33f, 0.22f, 0.13f, 1.0f);  // Secondary areas
    static const FLinearColor PanelLight     (0.50f, 0.36f, 0.22f, 1.0f);  // Hover highlight

    // ---- Gold trim + highlights ----
    static const FLinearColor GoldTrim       (0.72f, 0.58f, 0.28f, 1.0f);  // Outer border
    static const FLinearColor GoldDark       (0.50f, 0.38f, 0.15f, 1.0f);  // Bar borders, dividers
    static const FLinearColor GoldHighlight  (0.92f, 0.80f, 0.45f, 1.0f);  // Active tab, labels
    static const FLinearColor GoldDivider    (0.60f, 0.48f, 0.22f, 1.0f);  // Horizontal separators

    // ---- Bar fill colors ----
    static const FLinearColor HPGreen        (0.00f, 0.69f, 0.31f, 1.0f);  // HP bar (healthy)
    static const FLinearColor HPYellow       (0.85f, 0.75f, 0.10f, 1.0f);  // HP bar (< 25%)
    static const FLinearColor HPRed          (0.85f, 0.15f, 0.15f, 1.0f);  // HP bar (< 10%)
    static const FLinearColor SPBlue         (0.20f, 0.45f, 0.90f, 1.0f);  // SP bar
    static const FLinearColor EXPYellow      (0.90f, 0.75f, 0.10f, 1.0f);  // Base EXP bar
    static const FLinearColor JobExpOrange   (0.90f, 0.55f, 0.10f, 1.0f);  // Job EXP bar
    static const FLinearColor WeightGreen    (0.25f, 0.75f, 0.20f, 1.0f);  // Weight (normal)
    static const FLinearColor BarBg          (0.10f, 0.07f, 0.04f, 1.0f);  // Bar background

    // ---- Button colors ----
    static const FLinearColor ButtonNormal   (0.36f, 0.25f, 0.13f, 1.0f);
    static const FLinearColor ButtonHover    (0.48f, 0.36f, 0.23f, 1.0f);
    static const FLinearColor ButtonPressed  (0.24f, 0.18f, 0.12f, 1.0f);
    static const FLinearColor ButtonDisabled (0.30f, 0.25f, 0.20f, 0.6f);

    // ---- Text colors ----
    static const FLinearColor TextPrimary    (0.96f, 0.90f, 0.78f, 1.0f);  // Body text
    static const FLinearColor TextBright     (1.00f, 1.00f, 1.00f, 1.0f);  // White overlay text
    static const FLinearColor TextGold       (0.95f, 0.82f, 0.48f, 1.0f);  // Gold accents
    static const FLinearColor TextDisabled   (0.50f, 0.50f, 0.50f, 1.0f);  // Grayed out
    static const FLinearColor TextShadow     (0.00f, 0.00f, 0.00f, 0.85f); // Drop shadow

    // ---- Item / Slot colors ----
    static const FLinearColor SlotBg         (0.10f, 0.10f, 0.10f, 1.0f);  // Item slot background
    static const FLinearColor SlotBorder     (0.33f, 0.33f, 0.33f, 1.0f);  // Item slot border
    static const FLinearColor SlotHover      (0.50f, 0.40f, 0.20f, 0.5f);  // Slot hover highlight

    // ---- Tab colors ----
    static const FLinearColor TabActive      (0.36f, 0.25f, 0.13f, 1.0f);  // Selected tab
    static const FLinearColor TabInactive    (0.22f, 0.14f, 0.08f, 1.0f);  // Unselected tab

    // ---- Tooltip ----
    static const FLinearColor TooltipBg      (0.00f, 0.00f, 0.00f, 0.85f);
    static const FLinearColor TooltipBorder  (0.50f, 0.40f, 0.20f, 1.0f);

    // ---- Close button ----
    static const FLinearColor CloseNormal    (0.60f, 0.20f, 0.20f, 1.0f);
    static const FLinearColor CloseHover     (0.90f, 0.25f, 0.25f, 1.0f);
}

// ---- Chat channel colors (from RO Classic) ----
namespace ChatColors
{
    static const FLinearColor Public       = FLinearColor::White;
    static const FLinearColor WhisperSent  (1.00f, 0.84f, 0.00f, 1.0f);   // Gold
    static const FLinearColor WhisperRecv  (1.00f, 0.84f, 0.00f, 1.0f);   // Gold
    static const FLinearColor Party        (0.00f, 1.00f, 0.00f, 1.0f);   // Green
    static const FLinearColor Guild        (0.00f, 1.00f, 0.80f, 1.0f);   // Cyan-green
    static const FLinearColor System       (0.53f, 0.81f, 0.98f, 1.0f);   // Light blue
    static const FLinearColor Error        (1.00f, 0.20f, 0.20f, 1.0f);   // Red
    static const FLinearColor Combat       (0.70f, 0.70f, 0.70f, 1.0f);   // Light gray
    static const FLinearColor ItemLink     (0.40f, 0.60f, 1.00f, 1.0f);   // Blue
    static const FLinearColor GMBroadcast  (1.00f, 1.00f, 0.00f, 1.0f);   // Bright yellow
    static const FLinearColor QuestUpdate  (1.00f, 0.65f, 0.00f, 1.0f);   // Orange
}
```

### 2.2 SMMOPanel -- Reusable Draggable/Closeable Base Window

This is the foundational panel class for all toggleable RO-style windows. It provides:
- Gold-bordered brown panel frame
- Draggable title bar
- Close button (X)
- Optional minimize button
- Edge resize
- Position persistence via GConfig

**Complete header** (`SMMOPanel.h`):

```cpp
// SMMOPanel.h -- Reusable draggable/closeable RO-style window base class.
// All toggleable HUD panels (inventory, equipment, stats, chat, etc.)
// inherit from this class to get consistent frame, drag, close, and resize behavior.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SBox;

// Resize edge flags (bitmask)
enum class EMMOResizeEdge : uint8
{
    None   = 0,
    Left   = 1 << 0,
    Right  = 1 << 1,
    Top    = 1 << 2,
    Bottom = 1 << 3,
};
ENUM_CLASS_FLAGS(EMMOResizeEdge);

DECLARE_DELEGATE(FOnMMOPanelClosed);

class SMMOPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMMOPanel)
        : _Title(FText::GetEmpty())
        , _InitialSize(FVector2D(280.0, 350.0))
        , _InitialPosition(FVector2D(100.0, 100.0))
        , _MinSize(FVector2D(150.0, 100.0))
        , _bCanResize(true)
        , _bCanClose(true)
        , _bCanMinimize(false)
        , _ConfigKey(TEXT(""))
        {}

        // Window title text
        SLATE_ATTRIBUTE(FText, Title)

        // Initial size in logical pixels
        SLATE_ARGUMENT(FVector2D, InitialSize)

        // Initial position in logical pixels
        SLATE_ARGUMENT(FVector2D, InitialPosition)

        // Minimum size when resizing
        SLATE_ARGUMENT(FVector2D, MinSize)

        // Whether the window can be resized by dragging edges
        SLATE_ARGUMENT(bool, bCanResize)

        // Whether to show the close (X) button
        SLATE_ARGUMENT(bool, bCanClose)

        // Whether to show the minimize (-) button
        SLATE_ARGUMENT(bool, bCanMinimize)

        // GConfig key for persisting position (empty = no persistence)
        SLATE_ARGUMENT(FString, ConfigKey)

        // The content to display inside the panel body
        SLATE_DEFAULT_SLOT(FArguments, Content)

        // Called when the close button is clicked
        SLATE_EVENT(FOnMMOPanelClosed, OnClosed)

    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Programmatic position/size control
    void SetPanelPosition(const FVector2D& NewPosition);
    void SetPanelSize(const FVector2D& NewSize);
    FVector2D GetPanelPosition() const { return WidgetPosition; }
    FVector2D GetPanelSize() const { return CurrentSize; }

    // Minimize / restore
    void SetMinimized(bool bMinimize);
    bool IsMinimized() const { return bIsMinimized; }

protected:
    // Override in subclasses to build custom title bar content (right side)
    virtual TSharedRef<SWidget> BuildTitleBarExtras();

private:
    // ---- Title bar ----
    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildCloseButton();
    TSharedRef<SWidget> BuildMinimizeButton();

    // ---- Drag state ----
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D DragStartWidgetPos = FVector2D::ZeroVector;

    // ---- Resize state ----
    bool bIsResizing = false;
    bool bCanResize = true;
    EMMOResizeEdge ActiveResizeEdge = EMMOResizeEdge::None;
    FVector2D ResizeStartMouse = FVector2D::ZeroVector;
    FVector2D ResizeStartSize = FVector2D::ZeroVector;
    FVector2D ResizeStartPos = FVector2D::ZeroVector;
    static constexpr float RESIZE_GRAB_ZONE = 6.0f;

    // ---- Minimize state ----
    bool bIsMinimized = false;
    bool bCanMinimize = false;
    TSharedPtr<SWidget> ContentArea;

    // ---- Layout ----
    FVector2D WidgetPosition;
    FVector2D CurrentSize;
    FVector2D MinSize;
    TSharedPtr<SBox> RootSizeBox;

    void ApplyLayout();
    void ClampToViewport();
    EMMOResizeEdge HitTestEdges(const FGeometry& MyGeometry,
                                const FVector2D& ScreenPos) const;

    // ---- Config persistence ----
    FString ConfigKey;
    void SavePositionToConfig();
    void LoadPositionFromConfig();

    // ---- Close callback ----
    FOnMMOPanelClosed OnClosed;
    bool bCanClose = true;

    // ---- Title ----
    TAttribute<FText> TitleText;

    // ---- Mouse interaction overrides ----
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
                                     const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
                                    const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry,
                                const FPointerEvent& MouseEvent) override;
    virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry,
                                       const FPointerEvent& CursorEvent) const override;
};
```

**Complete implementation** (`SMMOPanel.cpp`):

```cpp
// SMMOPanel.cpp -- Reusable draggable/closeable RO-style window base.

#include "SMMOPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Misc/ConfigCacheIni.h"
#include "Framework/Application/SlateApplication.h"

// Include the shared color palette (or define inline as shown in Section 2.1)
// #include "MMOUIColors.h"

// Inline palette for self-contained compilation:
namespace ROColors
{
    static const FLinearColor PanelBrown     (0.43f, 0.29f, 0.17f, 1.0f);
    static const FLinearColor PanelDark      (0.22f, 0.14f, 0.08f, 1.0f);
    static const FLinearColor PanelMedium    (0.33f, 0.22f, 0.13f, 1.0f);
    static const FLinearColor GoldTrim       (0.72f, 0.58f, 0.28f, 1.0f);
    static const FLinearColor GoldHighlight  (0.92f, 0.80f, 0.45f, 1.0f);
    static const FLinearColor TextPrimary    (0.96f, 0.90f, 0.78f, 1.0f);
    static const FLinearColor TextShadow     (0.00f, 0.00f, 0.00f, 0.85f);
    static const FLinearColor CloseNormal    (0.60f, 0.20f, 0.20f, 1.0f);
    static const FLinearColor CloseHover     (0.90f, 0.25f, 0.25f, 1.0f);
    static const FLinearColor ButtonNormal   (0.36f, 0.25f, 0.13f, 1.0f);
    static const FLinearColor ButtonHover    (0.48f, 0.36f, 0.23f, 1.0f);
}

// ============================================================
// Construction
// ============================================================

void SMMOPanel::Construct(const FArguments& InArgs)
{
    TitleText     = InArgs._Title;
    CurrentSize   = InArgs._InitialSize;
    WidgetPosition = InArgs._InitialPosition;
    MinSize       = InArgs._MinSize;
    bCanResize    = InArgs._bCanResize;
    bCanClose     = InArgs._bCanClose;
    bCanMinimize  = InArgs._bCanMinimize;
    ConfigKey     = InArgs._ConfigKey;
    OnClosed      = InArgs._OnClosed;

    // Load persisted position if config key is set
    if (!ConfigKey.IsEmpty())
    {
        LoadPositionFromConfig();
    }

    // Build the content area (caller's slot content)
    ContentArea = InArgs._Content.Widget;

    ChildSlot
    [
        SAssignNew(RootSizeBox, SBox)
        .WidthOverride(CurrentSize.X)
        .HeightOverride(CurrentSize.Y)
        [
            // Outer gold trim border (2px)
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(ROColors::GoldTrim)
            .Padding(FMargin(2.0f))
            [
                // Inner dark inset (1px)
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(ROColors::PanelDark)
                .Padding(FMargin(1.0f))
                [
                    // Main brown panel
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                    .BorderBackgroundColor(ROColors::PanelBrown)
                    .Padding(FMargin(0.0f))
                    [
                        SNew(SVerticalBox)

                        // Title bar
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            BuildTitleBar()
                        ]

                        // Content area (collapsible)
                        + SVerticalBox::Slot()
                        .FillHeight(1.0f)
                        [
                            ContentArea.IsValid()
                                ? ContentArea.ToSharedRef()
                                : SNullWidget::NullWidget
                        ]
                    ]
                ]
            ]
        ]
    ];

    ApplyLayout();
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SMMOPanel::BuildTitleBar()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(ROColors::PanelDark)
        .Padding(FMargin(6.0f, 3.0f))
        [
            SNew(SHorizontalBox)

            // Title text (left)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(TitleText)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(ROColors::TextShadow)
            ]

            // Subclass extras (right, before buttons)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                BuildTitleBarExtras()
            ]

            // Minimize button (optional)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(4.0f, 0.0f, 0.0f, 0.0f)
            [
                bCanMinimize ? BuildMinimizeButton() : SNullWidget::NullWidget
            ]

            // Close button (optional)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(2.0f, 0.0f, 0.0f, 0.0f)
            [
                bCanClose ? BuildCloseButton() : SNullWidget::NullWidget
            ]
        ];
}

TSharedRef<SWidget> SMMOPanel::BuildTitleBarExtras()
{
    // Default: nothing. Subclasses override to add tabs, icons, etc.
    return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SMMOPanel::BuildCloseButton()
{
    return SNew(SBox)
        .WidthOverride(16.0f)
        .HeightOverride(16.0f)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor_Lambda([this]() -> FSlateColor {
                return FSlateColor(this->IsHovered()
                    ? ROColors::CloseHover
                    : ROColors::CloseNormal);
            })
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .OnMouseButtonDown_Lambda([this](const FGeometry&,
                                              const FPointerEvent& Event) -> FReply {
                if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
                {
                    OnClosed.ExecuteIfBound();
                    return FReply::Handled();
                }
                return FReply::Unhandled();
            })
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("X")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                .ColorAndOpacity(FSlateColor(FLinearColor::White))
            ]
        ];
}

TSharedRef<SWidget> SMMOPanel::BuildMinimizeButton()
{
    return SNew(SBox)
        .WidthOverride(16.0f)
        .HeightOverride(16.0f)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(ROColors::ButtonNormal)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .OnMouseButtonDown_Lambda([this](const FGeometry&,
                                              const FPointerEvent& Event) -> FReply {
                if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
                {
                    SetMinimized(!bIsMinimized);
                    return FReply::Handled();
                }
                return FReply::Unhandled();
            })
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("-")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                .ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
            ]
        ];
}

// ============================================================
// Minimize / Restore
// ============================================================

void SMMOPanel::SetMinimized(bool bMinimize)
{
    bIsMinimized = bMinimize;
    if (ContentArea.IsValid())
    {
        ContentArea->SetVisibility(bIsMinimized
            ? EVisibility::Collapsed
            : EVisibility::Visible);
    }

    // When minimized, shrink to just the title bar height
    if (bIsMinimized && RootSizeBox.IsValid())
    {
        RootSizeBox->SetHeightOverride(28.0f);
    }
    else
    {
        ApplyLayout();
    }
}

// ============================================================
// Programmatic position / size
// ============================================================

void SMMOPanel::SetPanelPosition(const FVector2D& NewPosition)
{
    WidgetPosition = NewPosition;
    ApplyLayout();
}

void SMMOPanel::SetPanelSize(const FVector2D& NewSize)
{
    CurrentSize.X = FMath::Max(NewSize.X, MinSize.X);
    CurrentSize.Y = FMath::Max(NewSize.Y, MinSize.Y);
    ApplyLayout();
}

// ============================================================
// Layout application
// ============================================================

void SMMOPanel::ApplyLayout()
{
    const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
    SetRenderTransform(FSlateRenderTransform(Pos));

    if (RootSizeBox.IsValid() && !bIsMinimized)
    {
        RootSizeBox->SetWidthOverride(CurrentSize.X);
        RootSizeBox->SetHeightOverride(CurrentSize.Y);
    }
}

void SMMOPanel::ClampToViewport()
{
    // Get viewport size from SlateApplication
    FVector2D ViewportSize = FVector2D::ZeroVector;
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication& SlateApp = FSlateApplication::Get();
        if (SlateApp.GetActiveTopLevelWindow().IsValid())
        {
            ViewportSize = SlateApp.GetActiveTopLevelWindow()
                           ->GetClientSizeInScreen();
        }
    }

    if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return;

    // Clamp position so the window stays on screen
    WidgetPosition.X = FMath::Clamp(WidgetPosition.X,
                                     0.0, ViewportSize.X - 50.0);
    WidgetPosition.Y = FMath::Clamp(WidgetPosition.Y,
                                     0.0, ViewportSize.Y - 20.0);
}

// ============================================================
// Edge hit-test for resize
// ============================================================

EMMOResizeEdge SMMOPanel::HitTestEdges(const FGeometry& MyGeometry,
                                        const FVector2D& ScreenPos) const
{
    if (!bCanResize) return EMMOResizeEdge::None;

    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
    const FVector2D Size = MyGeometry.GetLocalSize();
    EMMOResizeEdge Edge = EMMOResizeEdge::None;

    if (LocalPos.X < RESIZE_GRAB_ZONE)             Edge |= EMMOResizeEdge::Left;
    if (LocalPos.X > Size.X - RESIZE_GRAB_ZONE)    Edge |= EMMOResizeEdge::Right;
    if (LocalPos.Y < RESIZE_GRAB_ZONE)             Edge |= EMMOResizeEdge::Top;
    if (LocalPos.Y > Size.Y - RESIZE_GRAB_ZONE)    Edge |= EMMOResizeEdge::Bottom;

    return Edge;
}

// ============================================================
// Mouse interaction -- drag (title bar) + resize (edges)
// ============================================================

FReply SMMOPanel::OnMouseButtonDown(const FGeometry& MyGeometry,
                                     const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return FReply::Unhandled();

    const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();

    // Check for resize edges first
    EMMOResizeEdge Edge = HitTestEdges(MyGeometry, ScreenPos);
    if (Edge != EMMOResizeEdge::None)
    {
        bIsResizing = true;
        ActiveResizeEdge = Edge;
        ResizeStartMouse = ScreenPos;
        ResizeStartSize = CurrentSize;
        ResizeStartPos = WidgetPosition;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    // Check for title bar drag (top 24px of local space)
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
    if (LocalPos.Y < 24.0f)
    {
        bIsDragging = true;
        DragOffset = ScreenPos;
        DragStartWidgetPos = WidgetPosition;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    // Click inside panel body -- handle but let subclass content process
    return FReply::Unhandled();
}

FReply SMMOPanel::OnMouseButtonUp(const FGeometry& MyGeometry,
                                    const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return FReply::Unhandled();

    if (bIsDragging || bIsResizing)
    {
        bIsDragging = false;
        bIsResizing = false;
        ActiveResizeEdge = EMMOResizeEdge::None;

        ClampToViewport();
        ApplyLayout();
        SavePositionToConfig();

        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SMMOPanel::OnMouseMove(const FGeometry& MyGeometry,
                                const FPointerEvent& MouseEvent)
{
    const float DPIScale = (MyGeometry.GetLocalSize().X > 0.0f)
        ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X)
        : 1.0f;

    if (bIsDragging)
    {
        const FVector2D AbsDelta =
            MouseEvent.GetScreenSpacePosition() - DragOffset;
        WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
        ClampToViewport();
        ApplyLayout();
        return FReply::Handled();
    }

    if (bIsResizing)
    {
        const FVector2D Delta =
            (MouseEvent.GetScreenSpacePosition() - ResizeStartMouse) / DPIScale;
        FVector2D NewSize = ResizeStartSize;
        FVector2D NewPos = ResizeStartPos;

        if (EnumHasAnyFlags(ActiveResizeEdge, EMMOResizeEdge::Right))
            NewSize.X = ResizeStartSize.X + Delta.X;
        if (EnumHasAnyFlags(ActiveResizeEdge, EMMOResizeEdge::Bottom))
            NewSize.Y = ResizeStartSize.Y + Delta.Y;
        if (EnumHasAnyFlags(ActiveResizeEdge, EMMOResizeEdge::Left))
        {
            NewSize.X = ResizeStartSize.X - Delta.X;
            NewPos.X = ResizeStartPos.X + Delta.X;
        }
        if (EnumHasAnyFlags(ActiveResizeEdge, EMMOResizeEdge::Top))
        {
            NewSize.Y = ResizeStartSize.Y - Delta.Y;
            NewPos.Y = ResizeStartPos.Y + Delta.Y;
        }

        // Clamp to minimum
        if (NewSize.X < MinSize.X)
        {
            if (EnumHasAnyFlags(ActiveResizeEdge, EMMOResizeEdge::Left))
                NewPos.X = ResizeStartPos.X + (ResizeStartSize.X - MinSize.X);
            NewSize.X = MinSize.X;
        }
        if (NewSize.Y < MinSize.Y)
        {
            if (EnumHasAnyFlags(ActiveResizeEdge, EMMOResizeEdge::Top))
                NewPos.Y = ResizeStartPos.Y + (ResizeStartSize.Y - MinSize.Y);
            NewSize.Y = MinSize.Y;
        }

        CurrentSize = NewSize;
        WidgetPosition = NewPos;
        ApplyLayout();
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FCursorReply SMMOPanel::OnCursorQuery(const FGeometry& MyGeometry,
                                       const FPointerEvent& CursorEvent) const
{
    if (bIsResizing || bIsDragging)
        return FCursorReply::Unhandled();

    EMMOResizeEdge Edge =
        HitTestEdges(MyGeometry, CursorEvent.GetScreenSpacePosition());

    if (Edge == (EMMOResizeEdge::Left | EMMOResizeEdge::Top) ||
        Edge == (EMMOResizeEdge::Right | EMMOResizeEdge::Bottom))
        return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);

    if (Edge == (EMMOResizeEdge::Right | EMMOResizeEdge::Top) ||
        Edge == (EMMOResizeEdge::Left | EMMOResizeEdge::Bottom))
        return FCursorReply::Cursor(EMouseCursor::ResizeSouthWest);

    if (EnumHasAnyFlags(Edge, EMMOResizeEdge::Left | EMMOResizeEdge::Right))
        return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);

    if (EnumHasAnyFlags(Edge, EMMOResizeEdge::Top | EMMOResizeEdge::Bottom))
        return FCursorReply::Cursor(EMouseCursor::ResizeUpDown);

    return FCursorReply::Unhandled();
}

// ============================================================
// Config persistence
// ============================================================

void SMMOPanel::SavePositionToConfig()
{
    if (ConfigKey.IsEmpty()) return;

    GConfig->SetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".X")),
                       WidgetPosition.X, GGameUserSettingsIni);
    GConfig->SetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".Y")),
                       WidgetPosition.Y, GGameUserSettingsIni);
    GConfig->SetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".W")),
                       CurrentSize.X, GGameUserSettingsIni);
    GConfig->SetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".H")),
                       CurrentSize.Y, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void SMMOPanel::LoadPositionFromConfig()
{
    if (ConfigKey.IsEmpty()) return;

    double X = WidgetPosition.X, Y = WidgetPosition.Y;
    double W = CurrentSize.X, H = CurrentSize.Y;

    GConfig->GetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".X")),
                       X, GGameUserSettingsIni);
    GConfig->GetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".Y")),
                       Y, GGameUserSettingsIni);
    GConfig->GetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".W")),
                       W, GGameUserSettingsIni);
    GConfig->GetDouble(TEXT("SabriMMO.UI"), *(ConfigKey + TEXT(".H")),
                       H, GGameUserSettingsIni);

    WidgetPosition = FVector2D(X, Y);
    CurrentSize.X = FMath::Max(W, MinSize.X);
    CurrentSize.Y = FMath::Max(H, MinSize.Y);
}
```

**Usage example** -- wrapping any content in the standard panel:

```cpp
TSharedRef<SMMOPanel> Panel = SNew(SMMOPanel)
    .Title(FText::FromString(TEXT("Inventory")))
    .InitialSize(FVector2D(280, 350))
    .InitialPosition(FVector2D(400, 100))
    .bCanResize(true)
    .bCanClose(true)
    .ConfigKey(TEXT("Inventory"))
    .OnClosed_Lambda([this]() { HideWidget(); })
    [
        // Your content goes here
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("Items go here")))
        ]
    ];
```

### 2.3 SMMOButton

Styled button matching the RO brown/gold theme:

```cpp
// Inline helper -- builds a themed button with hover/press states
static TSharedRef<SWidget> BuildMMOButton(
    const FText& Label,
    FOnClicked OnClicked,
    bool bEnabled = true)
{
    auto ButtonColor = MakeShared<FLinearColor>(ROColors::ButtonNormal);

    return SNew(SBox)
        .HeightOverride(26.0f)
        .Padding(FMargin(2.0f))
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor_Lambda([ButtonColor, bEnabled]() -> FSlateColor {
                return bEnabled ? FSlateColor(*ButtonColor) :
                                  FSlateColor(ROColors::ButtonDisabled);
            })
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(FMargin(12.0f, 2.0f))
            .OnMouseButtonDown_Lambda([OnClicked, bEnabled](
                const FGeometry&, const FPointerEvent& Event) -> FReply {
                if (!bEnabled) return FReply::Unhandled();
                if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
                {
                    OnClicked.ExecuteIfBound();
                    return FReply::Handled();
                }
                return FReply::Unhandled();
            })
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FSlateColor(bEnabled
                    ? ROColors::GoldHighlight
                    : ROColors::TextDisabled))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(ROColors::TextShadow)
            ]
        ];
}
```

### 2.4 SMMOProgressBar

Themed progress bar with gold border, dark background, colored fill, and text overlay:

```cpp
// Inline helper -- builds an RO-style bar with label + value overlay
static TSharedRef<SWidget> BuildMMOProgressBar(
    TAttribute<FText> Label,
    const FSlateColor& FillColor,
    TAttribute<TOptional<float>> Percent,
    TAttribute<FText> ValueText,
    float BarHeight = 14.0f,
    float LabelWidth = 48.0f)
{
    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0, 0, 4, 0)
        [
            SNew(SBox)
            .WidthOverride(LabelWidth)
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                .ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(ROColors::TextShadow)
            ]
        ]

        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .HeightOverride(BarHeight)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(ROColors::GoldDark)
                .Padding(FMargin(1.0f))
                [
                    SNew(SOverlay)

                    + SOverlay::Slot()
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                        .BorderBackgroundColor(ROColors::BarBg)
                    ]

                    + SOverlay::Slot()
                    [
                        SNew(SProgressBar)
                        .Percent(Percent)
                        .FillColorAndOpacity(FillColor)
                    ]

                    + SOverlay::Slot()
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(ValueText)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
                        .ColorAndOpacity(FSlateColor(ROColors::TextBright))
                        .ShadowOffset(FVector2D(1, 1))
                        .ShadowColorAndOpacity(ROColors::TextShadow)
                    ]
                ]
            ]
        ];
}
```

### 2.5 SMMOTooltip

Tooltip popup that follows the cursor. Shown by the shared tooltip overlay at Z=40.

```cpp
// Tooltip content builder -- call from any widget's OnMouseEnter
struct FMMOTooltipData
{
    FString ItemName;
    FLinearColor NameColor = FLinearColor::White;   // Rarity color
    FString TypeLine;           // "Weapon - One-Handed Sword"
    FString Description;        // Flavor text
    TArray<FString> StatLines;  // "+10 ATK", "+5 DEF", etc.
    int32 Weight = 0;
    int32 SellPrice = 0;
    int32 Slots = 0;           // Card slots (0-4)
};

// Build tooltip widget from data
static TSharedRef<SWidget> BuildMMOTooltip(const FMMOTooltipData& Data)
{
    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

    // Item name (colored by rarity)
    Content->AddSlot().AutoHeight().Padding(0, 0, 0, 2)
    [
        SNew(STextBlock)
        .Text(FText::FromString(Data.ItemName))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        .ColorAndOpacity(FSlateColor(Data.NameColor))
    ];

    // Type line
    if (!Data.TypeLine.IsEmpty())
    {
        Content->AddSlot().AutoHeight()
        [
            SNew(STextBlock)
            .Text(FText::FromString(Data.TypeLine))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            .ColorAndOpacity(FSlateColor(ROColors::TextPrimary))
        ];
    }

    // Stats
    for (const FString& Line : Data.StatLines)
    {
        Content->AddSlot().AutoHeight()
        [
            SNew(STextBlock)
            .Text(FText::FromString(Line))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 1.0f, 0.3f, 1.0f)))
        ];
    }

    // Description
    if (!Data.Description.IsEmpty())
    {
        Content->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Data.Description))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
            .ColorAndOpacity(FSlateColor(ROColors::TextPrimary))
            .AutoWrapText(true)
        ];
    }

    // Weight + Sell price bottom line
    Content->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(
            TEXT("Weight: %d    Sell: %d z"), Data.Weight, Data.SellPrice)))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
        .ColorAndOpacity(FSlateColor(ROColors::TextDisabled))
    ];

    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(ROColors::TooltipBorder)
        .Padding(FMargin(1.0f))
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(ROColors::TooltipBg)
            .Padding(FMargin(8.0f, 6.0f))
            [
                Content
            ]
        ];
}
```

### 2.6 SMMOIconSlot

Reusable 32x32 or 24x24 icon slot used in inventory grids, hotbar, equipment, and skill tree.

```cpp
// Icon slot with border, background, hover highlight, and click/drag support
static TSharedRef<SWidget> BuildMMOIconSlot(
    int32 SlotIndex,
    TAttribute<const FSlateBrush*> IconBrush,    // Item/skill icon or null
    TAttribute<FText> StackCountText,             // "x5" for stackables
    TFunction<FReply(int32)> OnSlotClicked,       // Left click handler
    TFunction<FReply(int32)> OnSlotRightClicked,  // Right click handler
    float SlotSize = 32.0f)
{
    return SNew(SBox)
        .WidthOverride(SlotSize)
        .HeightOverride(SlotSize)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(ROColors::SlotBorder)
            .Padding(FMargin(1.0f))
            [
                SNew(SOverlay)

                // Dark background
                + SOverlay::Slot()
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                    .BorderBackgroundColor(ROColors::SlotBg)
                ]

                // Icon image
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(SImage)
                    .Image(IconBrush)
                ]

                // Stack count (bottom-right)
                + SOverlay::Slot()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Bottom)
                .Padding(FMargin(0, 0, 2, 1))
                [
                    SNew(STextBlock)
                    .Text(StackCountText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
                    .ColorAndOpacity(FSlateColor(FLinearColor::White))
                    .ShadowOffset(FVector2D(1, 1))
                    .ShadowColorAndOpacity(FLinearColor::Black)
                ]
            ]
        ];
}
```

---

## 3. Chat System

### 3.1 Architecture Overview

The chat system consists of a `UChatSubsystem` (UWorldSubsystem) and an `SChatWidget` (Slate). The subsystem handles socket events, message storage, /command parsing, and chat bubble management. The widget renders the message list, tabs, and input field.

**Socket events consumed**:
- `chat:message` -- incoming public/party/guild/system messages
- `chat:whisper` -- incoming whisper
- `chat:error` -- failed whisper (player offline, etc.)
- `chat:system` -- server announcements

**Socket events emitted**:
- `chat:send` -- `{ channel, message, targetName? }`

### 3.2 UChatSubsystem Header

```cpp
// ChatSubsystem.h -- UWorldSubsystem for RO-style multi-tab chat

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "ChatSubsystem.generated.h"

class USocketIOClientComponent;
class SChatWidget;

// Chat channel enum
UENUM()
enum class EChatChannel : uint8
{
    Public,
    Whisper,
    Party,
    Guild,
    System,
    Combat,
    Error,
    COUNT
};

// Single chat message entry
struct FChatMessage
{
    EChatChannel Channel = EChatChannel::Public;
    FString SenderName;
    FString Content;
    double Timestamp = 0.0;
    FLinearColor DisplayColor = FLinearColor::White;

    // For whispers: who the whisper is from/to
    FString WhisperTarget;
    bool bIsOutgoing = false;  // true = we sent it, false = we received it
};

// Chat bubble (world-space text above character)
struct FChatBubble
{
    FString Text;
    int32 CharacterId = 0;
    double SpawnTime = 0.0;
    EChatChannel Channel = EChatChannel::Public;
    static constexpr float BUBBLE_LIFETIME = 5.0f;
};

UCLASS()
class SABRIMMO_API UChatSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- public API ----
    void SendMessage(const FString& RawInput);
    void SendWhisper(const FString& TargetName, const FString& Message);

    // ---- data access (read by SChatWidget) ----
    const TArray<FChatMessage>& GetMessages() const { return MessageLog; }
    const TArray<FChatBubble>& GetActiveBubbles() const { return ActiveBubbles; }

    // ---- widget visibility ----
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const { return bWidgetAdded; }

private:
    // ---- socket event wrapping ----
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- event handlers ----
    void HandleChatMessage(const TSharedPtr<FJsonValue>& Data);
    void HandleChatWhisper(const TSharedPtr<FJsonValue>& Data);
    void HandleChatError(const TSharedPtr<FJsonValue>& Data);
    void HandleChatSystem(const TSharedPtr<FJsonValue>& Data);

    // ---- slash command parsing ----
    bool TryParseSlashCommand(const FString& Input);

    // ---- message storage ----
    void AddMessage(const FChatMessage& Message);
    void AddBubble(int32 CharacterId, const FString& Text, EChatChannel Channel);
    void CleanExpiredBubbles();

    static constexpr int32 MAX_MESSAGE_LOG = 500;
    TArray<FChatMessage> MessageLog;
    TArray<FChatBubble> ActiveBubbles;

    // ---- state ----
    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    int32 LocalCharacterId = 0;
    FString LocalPlayerName;
    FString LastWhisperSender;  // For /reply

    FTimerHandle BindCheckTimer;
    FTimerHandle BubbleCleanTimer;

    TSharedPtr<SChatWidget> ChatWidgetPtr;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 3.3 SChatWidget -- Complete Implementation

```cpp
// SChatWidget.h -- RO-style multi-tab chat window with input bar

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UChatSubsystem;

// Chat tab filter -- which channels appear in each tab
struct FChatTabFilter
{
    FString TabName;
    TSet<EChatChannel> VisibleChannels;
};

class SChatWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SChatWidget) {}
        SLATE_ARGUMENT(UChatSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Scroll to bottom when new message arrives
    void ScrollToBottom();

    // Focus the input field (called when Enter is pressed)
    void FocusInput();

private:
    TWeakObjectPtr<UChatSubsystem> OwningSubsystem;

    // ---- Tabs ----
    TArray<FChatTabFilter> Tabs;
    int32 ActiveTabIndex = 0;

    // ---- Widget references ----
    TSharedPtr<SBox> RootSizeBox;
    TSharedPtr<SScrollBox> MessageScrollBox;
    TSharedPtr<SEditableTextBox> InputField;
    TSharedPtr<SEditableTextBox> WhisperTargetField;

    // ---- Layout ----
    FVector2D WidgetPosition = FVector2D(10.0, 500.0);
    FVector2D CurrentSize = FVector2D(400.0, 200.0);
    int32 HeightPreset = 0;  // Cycled by F10: 0=small, 1=medium, 2=large
    static constexpr float HEIGHT_PRESETS[] = { 150.0f, 250.0f, 400.0f };

    // ---- Drag state ----
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D DragStartWidgetPos = FVector2D::ZeroVector;

    // ---- Builders ----
    TSharedRef<SWidget> BuildTabBar();
    TSharedRef<SWidget> BuildMessageArea();
    TSharedRef<SWidget> BuildInputBar();
    void RebuildMessageList();

    // ---- Tab helpers ----
    void SetActiveTab(int32 Index);
    bool ShouldShowMessage(const FChatMessage& Message) const;

    // ---- Input handling ----
    void OnInputCommitted(const FText& Text, ETextCommit::Type CommitType);
    void CycleHeight();

    // ---- Mouse interaction ----
    void ApplyLayout();
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
                                     const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
                                    const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry,
                                const FPointerEvent& MouseEvent) override;
};
```

**Implementation** (`SChatWidget.cpp`):

```cpp
// SChatWidget.cpp -- RO-style multi-tab chat window

#include "SChatWidget.h"
#include "ChatSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"

// Re-use the shared color palette
namespace ROColors
{
    static const FLinearColor PanelDark      (0.22f, 0.14f, 0.08f, 1.0f);
    static const FLinearColor PanelBrown     (0.43f, 0.29f, 0.17f, 1.0f);
    static const FLinearColor GoldTrim       (0.72f, 0.58f, 0.28f, 1.0f);
    static const FLinearColor GoldHighlight  (0.92f, 0.80f, 0.45f, 1.0f);
    static const FLinearColor TextPrimary    (0.96f, 0.90f, 0.78f, 1.0f);
    static const FLinearColor TextShadow     (0.00f, 0.00f, 0.00f, 0.85f);
    static const FLinearColor TabActive      (0.36f, 0.25f, 0.13f, 1.0f);
    static const FLinearColor TabInactive    (0.22f, 0.14f, 0.08f, 1.0f);
    static const FLinearColor BarBg          (0.10f, 0.07f, 0.04f, 1.0f);
}

namespace ChatColors
{
    static const FLinearColor Public       = FLinearColor::White;
    static const FLinearColor WhisperSent  (1.00f, 0.84f, 0.00f, 1.0f);
    static const FLinearColor WhisperRecv  (1.00f, 0.84f, 0.00f, 1.0f);
    static const FLinearColor Party        (0.00f, 1.00f, 0.00f, 1.0f);
    static const FLinearColor Guild        (0.00f, 1.00f, 0.80f, 1.0f);
    static const FLinearColor System       (0.53f, 0.81f, 0.98f, 1.0f);
    static const FLinearColor Error        (1.00f, 0.20f, 0.20f, 1.0f);
    static const FLinearColor Combat       (0.70f, 0.70f, 0.70f, 1.0f);
}

// Constexpr array definition (required outside class in C++17)
constexpr float SChatWidget::HEIGHT_PRESETS[];

// ============================================================
// Construction
// ============================================================

void SChatWidget::Construct(const FArguments& InArgs)
{
    OwningSubsystem = InArgs._Subsystem;

    // Define default tabs with channel filters
    Tabs.Add({ TEXT("General"), { EChatChannel::Public, EChatChannel::System,
                                   EChatChannel::Error, EChatChannel::Combat } });
    Tabs.Add({ TEXT("Party"),   { EChatChannel::Party } });
    Tabs.Add({ TEXT("Guild"),   { EChatChannel::Guild } });
    Tabs.Add({ TEXT("Whisper"), { EChatChannel::Whisper } });

    CurrentSize = FVector2D(400.0, HEIGHT_PRESETS[HeightPreset]);

    ChildSlot
    [
        SAssignNew(RootSizeBox, SBox)
        .WidthOverride(CurrentSize.X)
        .HeightOverride(CurrentSize.Y)
        [
            // Outer gold trim
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(ROColors::GoldTrim)
            .Padding(FMargin(1.5f))
            [
                // Main panel
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(ROColors::PanelDark)
                .Padding(FMargin(0.0f))
                [
                    SNew(SVerticalBox)

                    // Tab bar
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        BuildTabBar()
                    ]

                    // Message area
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    [
                        BuildMessageArea()
                    ]

                    // Input bar
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        BuildInputBar()
                    ]
                ]
            ]
        ]
    ];

    ApplyLayout();
}

// ============================================================
// Tab bar -- General | Party | Guild | Whisper
// ============================================================

TSharedRef<SWidget> SChatWidget::BuildTabBar()
{
    TSharedRef<SHorizontalBox> TabRow = SNew(SHorizontalBox);

    for (int32 i = 0; i < Tabs.Num(); ++i)
    {
        const int32 TabIndex = i;
        const FString& TabName = Tabs[i].TabName;

        TabRow->AddSlot()
        .AutoWidth()
        [
            SNew(SBox)
            .Padding(FMargin(1.0f, 0.0f))
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor_Lambda([this, TabIndex]() -> FSlateColor {
                    return FSlateColor(ActiveTabIndex == TabIndex
                        ? ROColors::TabActive
                        : ROColors::TabInactive);
                })
                .Padding(FMargin(8.0f, 3.0f))
                .OnMouseButtonDown_Lambda([this, TabIndex](
                    const FGeometry&, const FPointerEvent& Event) -> FReply {
                    if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
                    {
                        SetActiveTab(TabIndex);
                        return FReply::Handled();
                    }
                    return FReply::Unhandled();
                })
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TabName))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                    .ColorAndOpacity_Lambda([this, TabIndex]() -> FSlateColor {
                        return FSlateColor(ActiveTabIndex == TabIndex
                            ? ROColors::GoldHighlight
                            : ROColors::TextPrimary);
                    })
                ]
            ]
        ];
    }

    // Fill remaining space
    TabRow->AddSlot().FillWidth(1.0f)
    [
        SNullWidget::NullWidget
    ];

    return TabRow;
}

// ============================================================
// Message area -- scrollable list of chat messages
// ============================================================

TSharedRef<SWidget> SChatWidget::BuildMessageArea()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(FLinearColor(0.05f, 0.03f, 0.02f, 0.90f))
        .Padding(FMargin(4.0f, 2.0f))
        [
            SAssignNew(MessageScrollBox, SScrollBox)
            .ScrollBarVisibility(EVisibility::Visible)
        ];
}

// ============================================================
// Input bar -- whisper target field + message input
// ============================================================

TSharedRef<SWidget> SChatWidget::BuildInputBar()
{
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(ROColors::PanelBrown)
        .Padding(FMargin(4.0f, 3.0f))
        [
            SNew(SHorizontalBox)

            // Whisper target field (narrow, left side)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 4, 0)
            [
                SNew(SBox)
                .WidthOverride(80.0f)
                [
                    SAssignNew(WhisperTargetField, SEditableTextBox)
                    .HintText(FText::FromString(TEXT("Whisper to...")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ForegroundColor(FSlateColor(ChatColors::WhisperSent))
                    .BackgroundColor(FSlateColor(ROColors::BarBg))
                ]
            ]

            // Message input field (fills remaining width)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SAssignNew(InputField, SEditableTextBox)
                .HintText(FText::FromString(TEXT("Press Enter to chat...")))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                .ForegroundColor(FSlateColor(FLinearColor::White))
                .BackgroundColor(FSlateColor(ROColors::BarBg))
                .OnTextCommitted(FOnTextCommitted::CreateRaw(
                    this, &SChatWidget::OnInputCommitted))
            ]
        ];
}

// ============================================================
// Tab switching
// ============================================================

void SChatWidget::SetActiveTab(int32 Index)
{
    if (Index >= 0 && Index < Tabs.Num())
    {
        ActiveTabIndex = Index;
        RebuildMessageList();
    }
}

bool SChatWidget::ShouldShowMessage(const FChatMessage& Message) const
{
    if (ActiveTabIndex < 0 || ActiveTabIndex >= Tabs.Num())
        return true;

    return Tabs[ActiveTabIndex].VisibleChannels.Contains(Message.Channel);
}

// ============================================================
// Rebuild the visible message list for current tab
// ============================================================

void SChatWidget::RebuildMessageList()
{
    if (!MessageScrollBox.IsValid()) return;

    MessageScrollBox->ClearChildren();

    UChatSubsystem* Sub = OwningSubsystem.Get();
    if (!Sub) return;

    const TArray<FChatMessage>& AllMessages = Sub->GetMessages();

    for (const FChatMessage& Msg : AllMessages)
    {
        if (!ShouldShowMessage(Msg)) continue;

        // Format: "[SenderName]: Content" or "System: Content"
        FString DisplayText;
        if (Msg.Channel == EChatChannel::Whisper)
        {
            if (Msg.bIsOutgoing)
                DisplayText = FString::Printf(TEXT("To [%s]: %s"),
                    *Msg.WhisperTarget, *Msg.Content);
            else
                DisplayText = FString::Printf(TEXT("From [%s]: %s"),
                    *Msg.SenderName, *Msg.Content);
        }
        else if (Msg.Channel == EChatChannel::System ||
                 Msg.Channel == EChatChannel::Error)
        {
            DisplayText = Msg.Content;
        }
        else
        {
            DisplayText = FString::Printf(TEXT("[%s]: %s"),
                *Msg.SenderName, *Msg.Content);
        }

        MessageScrollBox->AddSlot()
        .Padding(FMargin(0, 1))
        [
            SNew(STextBlock)
            .Text(FText::FromString(DisplayText))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            .ColorAndOpacity(FSlateColor(Msg.DisplayColor))
            .AutoWrapText(true)
            .ShadowOffset(FVector2D(1, 1))
            .ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.6f))
        ];
    }

    // Auto-scroll to bottom
    MessageScrollBox->ScrollToEnd();
}

void SChatWidget::ScrollToBottom()
{
    RebuildMessageList();
}

// ============================================================
// Input handling
// ============================================================

void SChatWidget::FocusInput()
{
    if (InputField.IsValid())
    {
        FSlateApplication::Get().SetKeyboardFocus(InputField);
    }
}

void SChatWidget::OnInputCommitted(const FText& Text, ETextCommit::Type CommitType)
{
    if (CommitType != ETextCommit::OnEnter) return;

    FString RawInput = Text.ToString().TrimStartAndEnd();
    if (RawInput.IsEmpty()) return;

    UChatSubsystem* Sub = OwningSubsystem.Get();
    if (!Sub) return;

    // Check if whisper target field has content
    FString WhisperTarget;
    if (WhisperTargetField.IsValid())
    {
        WhisperTarget = WhisperTargetField->GetText().ToString().TrimStartAndEnd();
    }

    if (!WhisperTarget.IsEmpty())
    {
        Sub->SendWhisper(WhisperTarget, RawInput);
    }
    else
    {
        Sub->SendMessage(RawInput);
    }

    // Clear input
    if (InputField.IsValid())
    {
        InputField->SetText(FText::GetEmpty());
    }
}

void SChatWidget::CycleHeight()
{
    HeightPreset = (HeightPreset + 1) % 3;
    CurrentSize.Y = HEIGHT_PRESETS[HeightPreset];
    ApplyLayout();
}

// ============================================================
// Layout + Drag
// ============================================================

void SChatWidget::ApplyLayout()
{
    const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
    SetRenderTransform(FSlateRenderTransform(Pos));

    if (RootSizeBox.IsValid())
    {
        RootSizeBox->SetWidthOverride(CurrentSize.X);
        RootSizeBox->SetHeightOverride(CurrentSize.Y);
    }
}

FReply SChatWidget::OnMouseButtonDown(const FGeometry& MyGeometry,
                                       const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return FReply::Unhandled();

    // Title / tab bar area = top 22px -> drag handle
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(
        MouseEvent.GetScreenSpacePosition());
    if (LocalPos.Y < 22.0f)
    {
        bIsDragging = true;
        DragOffset = MouseEvent.GetScreenSpacePosition();
        DragStartWidgetPos = WidgetPosition;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    return FReply::Unhandled();
}

FReply SChatWidget::OnMouseButtonUp(const FGeometry& MyGeometry,
                                     const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        bIsDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SChatWidget::OnMouseMove(const FGeometry& MyGeometry,
                                 const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        const float DPIScale = (MyGeometry.GetLocalSize().X > 0.0f)
            ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X)
            : 1.0f;
        const FVector2D Delta =
            MouseEvent.GetScreenSpacePosition() - DragOffset;
        WidgetPosition = DragStartWidgetPos + Delta / DPIScale;
        ApplyLayout();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}
```

### 3.4 Slash Command Parsing

In `UChatSubsystem::SendMessage()`, check for slash commands before sending to server:

```cpp
bool UChatSubsystem::TryParseSlashCommand(const FString& Input)
{
    if (!Input.StartsWith(TEXT("/"))) return false;

    FString Command, Args;
    Input.Split(TEXT(" "), &Command, &Args);
    Command = Command.ToLower();

    if (Command == TEXT("/w") || Command == TEXT("/whisper"))
    {
        // /w "PlayerName" message
        FString TargetName, Message;
        // Parse quoted name or single-word name
        if (Args.StartsWith(TEXT("\"")))
        {
            int32 CloseQuote = Args.Find(TEXT("\""), ESearchCase::IgnoreCase,
                                          ESearchDir::FromStart, 1);
            if (CloseQuote > 0)
            {
                TargetName = Args.Mid(1, CloseQuote - 1);
                Message = Args.Mid(CloseQuote + 2);
            }
        }
        else
        {
            Args.Split(TEXT(" "), &TargetName, &Message);
        }
        if (!TargetName.IsEmpty())
        {
            SendWhisper(TargetName, Message);
        }
        return true;
    }

    if (Command == TEXT("/reply") || Command == TEXT("/r"))
    {
        if (!LastWhisperSender.IsEmpty())
        {
            SendWhisper(LastWhisperSender, Args);
        }
        return true;
    }

    // % prefix = party chat
    if (Input.StartsWith(TEXT("%")))
    {
        FString PartyMsg = Input.RightChop(1).TrimStart();
        // Emit: chat:send { channel: "party", message: PartyMsg }
        if (CachedSIOComponent.IsValid())
        {
            TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
            Payload->SetStringField(TEXT("channel"), TEXT("party"));
            Payload->SetStringField(TEXT("message"), PartyMsg);
            // Serialize and emit
        }
        return true;
    }

    // $ prefix = guild chat
    if (Input.StartsWith(TEXT("$")) || Command == TEXT("/gc"))
    {
        FString GuildMsg = Command == TEXT("/gc")
            ? Args
            : Input.RightChop(1).TrimStart();
        // Emit guild chat similarly
        return true;
    }

    // /sit, /stand, /where, etc. -- handle locally or emit to server
    return false;  // Unknown command -- send as public chat
}
```

### 3.5 Chat Bubbles

Chat bubbles are rendered by the `SWorldHealthBarOverlay` (or a dedicated `SChatBubbleOverlay`). The subsystem stores active bubbles and the overlay reads them each paint frame:

```cpp
void UChatSubsystem::AddBubble(int32 CharacterId, const FString& Text,
                                EChatChannel Channel)
{
    // Only public and party chat create overhead bubbles
    if (Channel != EChatChannel::Public && Channel != EChatChannel::Party)
        return;

    FChatBubble Bubble;
    Bubble.Text = Text;
    Bubble.CharacterId = CharacterId;
    Bubble.SpawnTime = FPlatformTime::Seconds();
    Bubble.Channel = Channel;

    ActiveBubbles.Add(MoveTemp(Bubble));
}

void UChatSubsystem::CleanExpiredBubbles()
{
    const double Now = FPlatformTime::Seconds();
    ActiveBubbles.RemoveAll([Now](const FChatBubble& B) {
        return (Now - B.SpawnTime) > FChatBubble::BUBBLE_LIFETIME;
    });
}
```

---

## 4. Minimap

### 4.1 Architecture

- `UMinimapSubsystem` (UWorldSubsystem) -- tracks player position, NPC positions, warp portal positions, zone bounds
- `SMinimapWidget` (SCompoundWidget) -- renders the 2D overhead map via `OnPaint()`

### 4.2 UMinimapSubsystem

```cpp
// MinimapSubsystem.h

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MinimapSubsystem.generated.h"

class SMinimapWidget;

// Marker types for minimap dots
enum class EMinimapMarkerType : uint8
{
    Player,         // White arrow (local player)
    OtherPlayer,    // White dot
    PartyMember,    // Magenta dot
    NPC,            // Blue dot (or typed icon)
    WarpPortal,     // Red dot
    Enemy,          // Red small dot (optional, only when targeted)
};

struct FMinimapMarker
{
    EMinimapMarkerType Type = EMinimapMarkerType::OtherPlayer;
    FVector2D WorldPosition = FVector2D::ZeroVector;
    float Rotation = 0.0f;  // For player arrow
    FString DisplayName;
};

UCLASS()
class SABRIMMO_API UMinimapSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Data access for widget ----
    FString ZoneName;
    FVector2D MapBoundsMin = FVector2D(-5000, -5000);
    FVector2D MapBoundsMax = FVector2D(5000, 5000);
    float ZoomLevel = 1.0f;

    TArray<FMinimapMarker> Markers;
    FMinimapMarker LocalPlayerMarker;

    void UpdateMarkers();   // Called on timer, refreshes from world actors
    void CycleOpacity();    // Ctrl+Tab
    void AdjustZoom(float Delta);  // Mouse wheel

private:
    void ShowWidget();
    void HideWidget();
    void RefreshFromWorld();
    void CacheActorPositions();

    bool bWidgetAdded = false;
    int32 OpacityPreset = 0;  // 0=opaque, 1=semi, 2=transparent, 3=hidden
    FTimerHandle UpdateTimer;

    TSharedPtr<SMinimapWidget> MinimapWidgetPtr;
    TSharedPtr<SWidget> ViewportOverlay;
};
```

### 4.3 SMinimapWidget Rendering

The minimap uses `OnPaint()` for efficient batch rendering of all markers:

```cpp
// In SMinimapWidget::OnPaint():
int32 SMinimapWidget::OnPaint(const FPaintArgs& Args,
    const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId,
    const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const float MapSize = FMath::Min(LocalSize.X, LocalSize.Y);

    // Draw background (dark semi-transparent circle or square)
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
        AllottedGeometry.ToPaintGeometry(),
        FCoreStyle::Get().GetBrush("GenericWhiteBox"),
        ESlateDrawEffect::None,
        FLinearColor(0.0f, 0.0f, 0.0f, 0.6f * OpacityMultiplier));
    LayerId++;

    UMinimapSubsystem* Sub = OwningSubsystem.Get();
    if (!Sub) return LayerId;

    // Convert world position to minimap position
    auto WorldToMinimap = [&](const FVector2D& WorldPos) -> FVector2D {
        const FVector2D Range = Sub->MapBoundsMax - Sub->MapBoundsMin;
        FVector2D Normalized = (WorldPos - Sub->MapBoundsMin) / Range;
        Normalized.Y = 1.0f - Normalized.Y;  // Flip Y for screen coords
        return Normalized * MapSize;
    };

    // Draw markers
    for (const FMinimapMarker& Marker : Sub->Markers)
    {
        FVector2D MarkerPos = WorldToMinimap(Marker.WorldPosition);

        FLinearColor DotColor;
        float DotSize = 4.0f;

        switch (Marker.Type)
        {
        case EMinimapMarkerType::OtherPlayer:
            DotColor = FLinearColor::White; break;
        case EMinimapMarkerType::PartyMember:
            DotColor = FLinearColor(1.0f, 0.0f, 1.0f, 1.0f); break;
        case EMinimapMarkerType::NPC:
            DotColor = FLinearColor(0.3f, 0.5f, 1.0f, 1.0f); break;
        case EMinimapMarkerType::WarpPortal:
            DotColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); break;
        default:
            DotColor = FLinearColor::White; break;
        }

        // Draw dot as small box
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(
                FVector2f(DotSize, DotSize),
                FSlateLayoutTransform(FVector2f(
                    MarkerPos.X - DotSize * 0.5f,
                    MarkerPos.Y - DotSize * 0.5f))),
            FCoreStyle::Get().GetBrush("GenericWhiteBox"),
            ESlateDrawEffect::None, DotColor);
    }
    LayerId++;

    // Draw local player arrow (larger, rotated)
    // ... (render rotated triangle using FSlateDrawElement::MakeRotatedBox)

    // Draw zone name at top
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 9);
    FSlateDrawElement::MakeText(OutDrawElements, LayerId,
        AllottedGeometry.ToPaintGeometry(),
        FText::FromString(Sub->ZoneName).ToString(), Font,
        ESlateDrawEffect::None, FLinearColor::White);

    return LayerId;
}
```

---

## 5. World-Space UI

### 5.1 Pattern: OnPaint Overlay

All world-space UI elements (damage numbers, health bars, nameplates, cast bars) share a common pattern: a fullscreen transparent `SCompoundWidget` added to the viewport at a specific Z-order. The widget overrides `OnPaint()` to render at projected screen positions. No individual child widgets are created per entity.

**Benefits**:
- Single draw call batch for all elements of a type
- No widget allocation/destruction overhead
- Precise control over animation timing
- Pool-based with fixed upper bounds

### 5.2 Damage Numbers (Existing)

Already implemented in `DamageNumberSubsystem` + `SDamageNumberOverlay`. Key design:

- **Pool size**: 64 entries (circular buffer)
- **Lifetime**: 1.3 seconds per pop-up
- **Animation**: Rise 85px, fan-out per digit, fade after 65% lifetime
- **Colors**: Yellow (normal), white (crit), red (player hit), light blue (miss), green (heal/dodge)
- **Stack detection**: Numbers within 60px and 0.8s are offset vertically

### 5.3 Health Bars (Existing)

Already implemented in `WorldHealthBarSubsystem` + `SWorldHealthBarOverlay`. Key design:

- **Rendered via** `OnPaint()` using `FSlateDrawElement::MakeBox()` for bar segments
- **RO Classic colors**: Navy border (2px), green HP fill, blue SP fill, pink enemy HP
- **Actor caching**: `CacheEnemyActors()` finds enemy actors by name pattern on a 2-second timer
- **Position source**: Real-time from cached `AActor*` (smooth), fallback to socket position data

### 5.4 Nameplates

Nameplates (character name + guild emblem above characters) are rendered in the same `SWorldHealthBarOverlay::OnPaint()` pass, positioned above the health bars:

```cpp
// In SWorldHealthBarOverlay::OnPaint(), for each visible character:

// Draw nameplate text centered above HP bar
const FString DisplayName = CharacterName;
FSlateDrawElement::MakeText(OutDrawElements, LayerId,
    AllottedGeometry.ToPaintGeometry(
        FVector2f(0, 0),
        FSlateLayoutTransform(FVector2f(NameX, NameY))),
    DisplayName, NameFont,
    ESlateDrawEffect::None,
    FLinearColor::White);
```

### 5.5 Cast Bars (Existing)

Already implemented in `CastBarSubsystem` + `SCastBarOverlay`. Key design:

- **World-projected**: Cast bar rendered at caster's head position
- **Progress**: `(CurrentTime - CastStartTime) / CastDuration`
- **Visual**: Gold border, dark background, white fill

### 5.6 Ground Targeting Circle (Existing)

Already implemented in `SSkillTargetingOverlay`. Renders a colored circle on the ground following the cursor for AoE skill placement.

### 5.7 Performance Guidelines for 100+ Elements

When rendering many world-space elements simultaneously:

1. **Frustum culling**: Only project positions for actors within the camera frustum. Use `APlayerController::ProjectWorldLocationToScreen()` -- it returns `false` for off-screen positions.

2. **Distance culling**: Skip entities beyond a maximum render distance (e.g., 5000 UE units).

3. **Pool with hard cap**: Pre-allocate N render slots. Reuse the oldest expired slot for new entries.

4. **Priority sorting**: When more entities exist than render slots, prioritize:
   - Party members (always show)
   - Current target (always show)
   - Damaged enemies (show briefly)
   - Nearby entities (distance-based)

5. **Batch rendering**: All elements of one type rendered in a single `OnPaint()` call. Never create individual `SWidget` per floating element.

6. **Cache projections**: If multiple overlays need the same world-to-screen projection (nameplate + HP bar + cast bar for the same character), cache the projected position and share it.

---

## 6. Notification System

### 6.1 Architecture

The notification system is a lightweight subsystem that renders temporary banners and feed items. It does not use the `SMMOPanel` base class -- it renders directly via `OnPaint()` on a fullscreen overlay.

### 6.2 Notification Types

| Type | Visual | Duration | Position |
|------|--------|----------|----------|
| Level Up | Large gold text + sparkle border | 4 seconds | Center screen |
| Job Level Up | Medium blue text | 3 seconds | Center screen |
| Item Pickup | Small text line | 3 seconds | Right side feed |
| Quest Progress | Small text line | 3 seconds | Right side feed |
| System Message | Yellow text | 5 seconds | Top center |
| Error | Red text | 3 seconds | Center screen |

### 6.3 UNotificationSubsystem

```cpp
// NotificationSubsystem.h

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NotificationSubsystem.generated.h"

enum class ENotificationType : uint8
{
    BaseLevelUp,
    JobLevelUp,
    ItemPickup,
    QuestProgress,
    SystemMessage,
    ErrorMessage,
};

struct FNotificationEntry
{
    ENotificationType Type = ENotificationType::SystemMessage;
    FString Text;
    double SpawnTime = 0.0;
    float Duration = 3.0f;
    FLinearColor Color = FLinearColor::White;
};

UCLASS()
class SABRIMMO_API UNotificationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public API ----
    void ShowNotification(ENotificationType Type, const FString& Text);
    void ShowLevelUp(int32 NewLevel, bool bIsJobLevel);
    void ShowItemPickup(const FString& ItemName, int32 Quantity);

    // ---- Data access for overlay widget ----
    TArray<FNotificationEntry> ActiveNotifications;

private:
    void TryWrapSocketEvents();
    void HandleExpLevelUp(const TSharedPtr<FJsonValue>& Data);
    void CleanExpired();

    // ... standard subsystem boilerplate (timer, socket component, overlay)
};
```

### 6.4 Level Up Banner

The level-up banner is rendered as a centered, large-font text with a gold border that scales up from 0 to 1 over 0.3 seconds and then fades out:

```cpp
// In SNotificationOverlay::OnPaint():
for (const FNotificationEntry& N : Sub->ActiveNotifications)
{
    if (N.Type != ENotificationType::BaseLevelUp &&
        N.Type != ENotificationType::JobLevelUp) continue;

    const float Age = (float)(Now - N.SpawnTime);
    const float Alpha = (Age < 0.3f) ? (Age / 0.3f) :
                         FMath::Clamp(1.0f - (Age - N.Duration + 1.0f), 0.0f, 1.0f);
    const float Scale = (Age < 0.3f) ? FMath::Lerp(0.5f, 1.0f, Age / 0.3f) : 1.0f;

    FLinearColor BannerColor = (N.Type == ENotificationType::BaseLevelUp)
        ? FLinearColor(1.0f, 0.84f, 0.0f, Alpha)   // Gold
        : FLinearColor(0.5f, 0.7f, 1.0f, Alpha);    // Light blue

    // Render centered text with outline
    // ...
}
```

### 6.5 Item Pickup Feed

A vertical list on the right side of the screen, with entries sliding in from the right and fading out:

```
                                          [+ Red Potion x5]
                                          [+ Jellopy x3]
                                          [+ 150 Zeny]
```

Each entry fades over 3 seconds and slides up as new entries push from the bottom.

---

## 7. Window Management

### 7.1 Open/Close Tracking

A central manager (or each subsystem independently) tracks which panels are open. In Sabri_MMO, each subsystem has `ShowWidget()` / `HideWidget()` / `IsWidgetVisible()` and toggles are handled by keyboard shortcuts.

### 7.2 Keyboard Shortcuts

Current implementation maps shortcuts to subsystem toggle functions:

| Key | Action | Subsystem |
|-----|--------|-----------|
| F5 | Cycle hotbar rows (1, 2, 3, 4, hidden) | `HotbarSubsystem` |
| F6 | Toggle inventory | `InventorySubsystem` |
| F7 | Toggle equipment | `EquipmentSubsystem` |
| F8 | Toggle combat stats | `CombatStatsSubsystem` |
| F10 | Cycle chat height | `ChatSubsystem` |
| K | Toggle skill tree | `SkillTreeSubsystem` (Blueprint IMC) |
| Alt+A | Toggle status window | `CombatStatsSubsystem` (expand) |
| Alt+E | Toggle inventory | `InventorySubsystem` |
| Alt+Q | Toggle equipment | `EquipmentSubsystem` |
| Alt+Z | Toggle party window | `PartySubsystem` |
| Enter | Focus chat input | `ChatSubsystem` |
| Escape | Close topmost window / open menu | Window Manager |

**Keyboard shortcut registration** is done in `ASabriMMOCharacter::SetupPlayerInputComponent` via Enhanced Input:

```cpp
// In SetupPlayerInputComponent:
if (UEnhancedInputComponent* EIC =
    Cast<UEnhancedInputComponent>(PlayerInputComponent))
{
    // F6 -> Inventory toggle
    EIC->BindAction(IA_ToggleInventory, ETriggerEvent::Started,
        this, &ASabriMMOCharacter::ToggleInventory);
    // ... repeat for each shortcut
}

void ASabriMMOCharacter::ToggleInventory()
{
    if (UInventorySubsystem* Sub =
        GetWorld()->GetSubsystem<UInventorySubsystem>())
    {
        if (Sub->IsWidgetVisible())
            Sub->HideWidget();
        else
            Sub->ShowWidget();
    }
}
```

### 7.3 Position Persistence

All panels using `SMMOPanel` with a non-empty `ConfigKey` automatically save/load position and size to `GameUserSettings.ini`:

```ini
[SabriMMO.UI]
Inventory.X=400.0
Inventory.Y=100.0
Inventory.W=280.0
Inventory.H=350.0
Equipment.X=700.0
Equipment.Y=100.0
...
```

Positions are saved when drag/resize ends. They are loaded in `SMMOPanel::Construct()`.

### 7.4 ESC Behavior

When Escape is pressed, the system closes the topmost open panel (last opened). If no panels are open, it opens the game menu. Implementation:

```cpp
// In ASabriMMOCharacter or a dedicated WindowManagerSubsystem:
void HandleEscapeKey()
{
    UWorld* World = GetWorld();

    // Try closing panels in reverse priority order (last opened first)
    // Check each toggleable subsystem
    if (auto* Shop = World->GetSubsystem<UShopSubsystem>();
        Shop && Shop->IsWidgetVisible())
    {
        Shop->HideWidget(); return;
    }
    if (auto* Kafra = World->GetSubsystem<UKafraSubsystem>();
        Kafra && Kafra->IsWidgetVisible())
    {
        Kafra->HideWidget(); return;
    }
    if (auto* Skill = World->GetSubsystem<USkillTreeSubsystem>();
        Skill && Skill->IsWidgetVisible())
    {
        Skill->HideWidget(); return;
    }
    if (auto* Inv = World->GetSubsystem<UInventorySubsystem>();
        Inv && Inv->IsWidgetVisible())
    {
        Inv->HideWidget(); return;
    }
    if (auto* Equip = World->GetSubsystem<UEquipmentSubsystem>();
        Equip && Equip->IsWidgetVisible())
    {
        Equip->HideWidget(); return;
    }
    if (auto* Stats = World->GetSubsystem<UCombatStatsSubsystem>();
        Stats && Stats->IsWidgetVisible())
    {
        Stats->HideWidget(); return;
    }

    // No panels open -> open game menu
    // OpenGameMenu();
}
```

---

## 8. Death / Respawn UI

### 8.1 Architecture

- `UDeathSubsystem` (UWorldSubsystem) -- listens to `combat:death` and `combat:respawn` events
- `SDeathOverlayWidget` (SCompoundWidget) -- fullscreen semi-transparent overlay with respawn dialog

### 8.2 UDeathSubsystem

```cpp
// DeathSubsystem.h

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "DeathSubsystem.generated.h"

class USocketIOClientComponent;
class SDeathOverlayWidget;

UCLASS()
class SABRIMMO_API UDeathSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public state ----
    bool bIsDead = false;
    bool bResurrectionOffered = false;
    FString ResurrectionOfferer;

    // ---- Actions ----
    void RequestReturnToSavePoint();
    void AcceptResurrection();
    void DeclineResurrection();

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);
    void HandleResurrectionOffer(const TSharedPtr<FJsonValue>& Data);

    void ShowOverlay();
    void HideOverlay();

    bool bEventsWrapped = false;
    bool bOverlayAdded = false;
    int32 LocalCharacterId = 0;

    FTimerHandle BindCheckTimer;
    TSharedPtr<SDeathOverlayWidget> OverlayWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 8.3 SDeathOverlayWidget

```cpp
// SDeathOverlayWidget.h

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UDeathSubsystem;

class SDeathOverlayWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDeathOverlayWidget) {}
        SLATE_ARGUMENT(UDeathSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Update visibility of resurrection prompt
    void RefreshLayout();

private:
    TWeakObjectPtr<UDeathSubsystem> OwningSubsystem;

    TSharedRef<SWidget> BuildDeathDialog();
    TSharedRef<SWidget> BuildResurrectionDialog();
};
```

The death overlay is a fullscreen semi-transparent dark wash with a centered dialog:

```cpp
void SDeathOverlayWidget::Construct(const FArguments& InArgs)
{
    OwningSubsystem = InArgs._Subsystem;

    ChildSlot
    [
        // Fullscreen dark overlay
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f))
        .Visibility(EVisibility::SelfHitTestInvisible)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(350.0f)
            .HeightOverride(180.0f)
            [
                // Gold-bordered dialog using SMMOPanel style
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(ROColors::GoldTrim)
                .Padding(FMargin(2.0f))
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                    .BorderBackgroundColor(ROColors::PanelDark)
                    .Padding(FMargin(20.0f))
                    [
                        SNew(SVerticalBox)

                        // "You have died." text
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .HAlign(HAlign_Center)
                        .Padding(0, 0, 0, 16)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("You have died.")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                            .ColorAndOpacity(FSlateColor(ROColors::HPRed))
                        ]

                        // [Return to Save Point] button
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .HAlign(HAlign_Center)
                        .Padding(0, 0, 0, 10)
                        [
                            // Use BuildMMOButton() from Section 2.3
                            SNew(SBox).HeightOverride(30.0f).WidthOverride(200.0f)
                            [
                                SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                                .BorderBackgroundColor(ROColors::ButtonNormal)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                .OnMouseButtonDown_Lambda([this](const FGeometry&,
                                    const FPointerEvent& E) -> FReply {
                                    if (E.GetEffectingButton() == EKeys::LeftMouseButton)
                                    {
                                        if (UDeathSubsystem* Sub = OwningSubsystem.Get())
                                            Sub->RequestReturnToSavePoint();
                                        return FReply::Handled();
                                    }
                                    return FReply::Unhandled();
                                })
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("Return to Save Point")))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                                    .ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
                                ]
                            ]
                        ]

                        // "(or wait for resurrection)" text
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .HAlign(HAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(
                                TEXT("(or wait for resurrection)")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
                            .ColorAndOpacity(FSlateColor(ROColors::TextDisabled))
                        ]
                    ]
                ]
            ]
        ]
    ];
}
```

### 8.4 Server Integration

The death subsystem emits `combat:respawn_request` to the server when "Return to Save Point" is clicked:

```cpp
void UDeathSubsystem::RequestReturnToSavePoint()
{
    if (!bIsDead) return;
    if (!CachedSIOComponent.IsValid()) return;

    CachedSIOComponent->EmitNative(TEXT("combat:respawn_request"), TEXT("{}"));
    // The server will emit combat:respawn which triggers HandleCombatRespawn
    // and removes the overlay
}
```

---

## 9. Adding a New UI Panel (Template)

This section provides a step-by-step checklist for adding any new UI panel to Sabri_MMO.

### Step 1: Create the Subsystem

Create `UI/MyFeatureSubsystem.h` and `UI/MyFeatureSubsystem.cpp`.

```cpp
// MyFeatureSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "MyFeatureSubsystem.generated.h"

class USocketIOClientComponent;
class SMyFeatureWidget;

UCLASS()
class SABRIMMO_API UMyFeatureSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Public data fields (read by widget) ----
    // Add your data here...

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Widget visibility ----
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const { return bWidgetAdded; }
    void ToggleVisibility();

private:
    // ---- Socket event wrapping (copy from BasicInfoSubsystem) ----
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event handlers ----
    void HandleMyEvent(const TSharedPtr<FJsonValue>& Data);

    // ---- State ----
    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    int32 LocalCharacterId = 0;

    FTimerHandle BindCheckTimer;
    TSharedPtr<SMyFeatureWidget> WidgetPtr;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### Step 2: Implement the Subsystem

```cpp
// MyFeatureSubsystem.cpp
#include "MyFeatureSubsystem.h"
#include "SMyFeatureWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyFeature, Log, All);

bool UMyFeatureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UMyFeatureSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Resolve local character ID
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
    {
        LocalCharacterId = GI->GetSelectedCharacter().CharacterId;
    }

    // Start polling for SocketIO
    InWorld.GetTimerManager().SetTimer(BindCheckTimer,
        FTimerDelegate::CreateUObject(this,
            &UMyFeatureSubsystem::TryWrapSocketEvents),
        0.5f, true);
}

void UMyFeatureSubsystem::Deinitialize()
{
    HideWidget();
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

void UMyFeatureSubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;

    // CRITICAL: Use World->GetGameViewport(), NEVER GEngine->GameViewport
    UGameViewportClient* VC = World->GetGameViewport();
    if (!VC) return;

    WidgetPtr = SNew(SMyFeatureWidget).Subsystem(this);
    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(WidgetPtr);

    // Pick Z-order from the table in Section 1.3
    VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), /*ZOrder=*/ 14);
    bWidgetAdded = true;
}

void UMyFeatureSubsystem::HideWidget()
{
    if (!bWidgetAdded) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (ViewportOverlay.IsValid())
                VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
        }
    }
    WidgetPtr.Reset();
    ViewportOverlay.Reset();
    bWidgetAdded = false;
}

void UMyFeatureSubsystem::ToggleVisibility()
{
    if (bWidgetAdded) HideWidget(); else ShowWidget();
}

// TryWrapSocketEvents, WrapSingleEvent, FindSocketIOComponent
// are identical to BasicInfoSubsystem -- copy the pattern exactly.
```

### Step 3: Create the Widget

```cpp
// SMyFeatureWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UMyFeatureSubsystem;

class SMyFeatureWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMyFeatureWidget) {}
        SLATE_ARGUMENT(UMyFeatureSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UMyFeatureSubsystem> OwningSubsystem;

    // Build your content using SMMOPanel as the container:
    // SNew(SMMOPanel)
    //     .Title(FText::FromString(TEXT("My Feature")))
    //     .InitialSize(FVector2D(280, 350))
    //     .bCanClose(true)
    //     .OnClosed_Lambda([this]() { /* hide via subsystem */ })
    //     [ /* content widgets */ ];
};
```

### Step 4: Register Keyboard Shortcut

In `ASabriMMOCharacter::SetupPlayerInputComponent` (or your Enhanced Input setup):

```cpp
// 1. Create an Input Action asset (IA_ToggleMyFeature) in the editor
// 2. Add it to IMC_MMOCharacter with the desired key binding
// 3. Bind in C++:

EIC->BindAction(IA_ToggleMyFeature, ETriggerEvent::Started,
    this, &ASabriMMOCharacter::ToggleMyFeature);

void ASabriMMOCharacter::ToggleMyFeature()
{
    if (UMyFeatureSubsystem* Sub =
        GetWorld()->GetSubsystem<UMyFeatureSubsystem>())
    {
        Sub->ToggleVisibility();
    }
}
```

### Step 5: Multiplayer-Safe Checklist

Before considering the feature done, verify all items:

- [ ] Uses `World->GetGameViewport()` (never `GEngine->GameViewport`)
- [ ] Uses `GetWorld()->GetFirstPlayerController()` scoped to own world
- [ ] No `static` or global state shared between PIE instances
- [ ] Tested with 2+ PIE instances simultaneously
- [ ] `FInputModeGameAndUI` with `SetHideCursorDuringCapture(false)`
- [ ] Widget properly destroyed in `Deinitialize()` (no dangling Slate references)
- [ ] `OnMouseButtonDown` returns `FReply::Unhandled()` for clicks outside interactive areas
- [ ] Z-order does not conflict with existing widgets (check table in Section 1.3)
- [ ] Socket event listeners cleaned up in `Deinitialize()`
- [ ] Widget reads data from subsystem fields (never directly from socket events)

### Step 6: Add to Documentation

Update the Z-order table in this document and in `CLAUDE.md` MEMORY.md with:
- Subsystem class name
- Widget class name
- Z-order value
- Toggle key

---

*Document Version: 1.0*
*Last Updated: 2026-03-08*
*Source Reference: `RagnaCloneDocs/09_UI_UX_System.md`, existing Sabri_MMO UI subsystem implementations*
