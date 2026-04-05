# Item Tooltip & Inspect System — Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Inventory_System](../03_Server_Side/Inventory_System.md) | [Card_System](../03_Server_Side/Card_System.md)

**Date**: 2026-03-12
**Status**: IMPLEMENTED
**Scope**: Hover tooltip (simple) + Right-click inspect window (detailed) + Card slot visualization
**Affects**: Server (index.js, DB schema), Client (CharacterData.h, InventorySubsystem, SInventoryWidget, SEquipmentWidget, SShopWidget, new SItemInspectWidget)

---

## Table of Contents

1. [Research Summary](#1-research-summary)
2. [Current State Analysis](#2-current-state-analysis)
3. [Feature Scope](#3-feature-scope)
4. [Server-Side Changes](#4-server-side-changes)
5. [Client-Side Changes](#5-client-side-changes)
6. [SItemInspectWidget Design](#6-siteminspectwidget-design)
7. [Card Slot Visualization](#7-card-slot-visualization)
8. [Integration Points](#8-integration-points)
9. [Implementation Phases](#9-implementation-phases)
10. [Completion Checklist](#10-completion-checklist)

---

## 1. Research Summary

### 1.1 RO Classic Hover Behavior
- In classic RO, hovering over an item shows **only the item name** (with refinement, card prefix/suffix, slot count)
- No rich tooltip popup on hover in the original game
- **Our adaptation**: Show a simple tooltip with Name + short Description + key stats (ATK/DEF/Weight). This is already partially implemented and provides a better modern UX while staying true to RO's minimalist feel.

### 1.2 RO Classic Right-Click Inspect Window
Based on reference screenshots (`troubleshootin pictures/itemInspectWithSlots.png` and `itemInspectNoSlots.png`):

**Layout (equipment with slots — "Clip of Bigmouth"):**
```
+--------------------------------------------------+
| Clip of Bigmouth                            [X]  |  ← Title bar + close button
+--------------------------------------------------+
|  +--------+                                      |
|  |  ICON  |  A versatile accessory that you      |
|  | ~80x80 |  can attach to pretty much anything. |
|  +--------+  Maximum SP + 10                     |
|              Class : Accessory                   |
|              Weight : 10                         |
|              ...                           [↑↓]  |  ← Scroll arrows
+--------------------------------------------------+
| [Berzebub Card] [◇] [◇] [◇]                    |  ← Card slot footer
+--------------------------------------------------+
```

**Layout (card item — "Berzebub Card"):**
```
+--------------------------------------------------+
| Berzebub Card                               [X]  |
+--------------------------------------------------+
|  +--------+                                      |
|  |  ICON  |  Reduce Casting Time by 30%.         |
|  | ~80x80 |  Class : Card                        |
|  +--------+  Compound on : Accessory             |
|                                                  |
+--------------------------------------------------+
                                 (no card slot footer)
```

### 1.3 Item Name Formatting (RO Classic)
Full format: `+[Refine] [Multiplier] [Prefix] BaseName [Suffix] [SlotCount]`

| Example | Meaning |
|---------|---------|
| `Sword [3]` | Base weapon, 3 empty card slots |
| `+7 Sword [3]` | Refined to +7, 3 slots |
| `+7 Bloody Sword [3]` | +7 refine, Hydra Card compounded (prefix = "Bloody") |
| `+7 Double Bloody Sword [3]` | +7, two Hydra Cards |
| `Shield of Endure [1]` | Endure Card compounded (suffix = "of Endure") |

### 1.4 Description Field Format (from `full_description` column)
The `full_description` column in the `items` table contains multi-line text with sections separated by underscores. Examples:

**Consumable (Red Potion 501):**
```
A bottle of potion made from grinded Red Herbs.
________________________
Type: Restorative
Heal: 45 ~ 65 HP
Weight: 7
```

**Weapon (Sword 1101):**
```
A basic one-handed sword.
________________________
Type: One-Handed Sword
Attack: 25
Weight: 50
Weapon Level: 1
Refinable: Yes
________________________
Requirement:
Base level 2
Novice, Swordman, Merchant and Thief classes
```

**Card (Poring Card 4001):**
```
LUK +2
Perfect Dodge +1
________________________
Type: Card
Compound on: Armor
Weight: 1
```

### 1.5 Card Slot System
- Equipment has 0-4 card slots (stored in `items.slots` column)
- Weapons: 0-4 slots. Armor/Shield/Headgear/Garment/Footgear/Accessory: 0-1 slots.
- Each card has a `card_type` (weapon/armor/shield/garment/footgear/headgear/accessory) — it can only be compounded into matching equipment
- Each card has `card_prefix` and `card_suffix` fields for name formatting
- Compounding is **permanent** — cards cannot be removed
- If equipment is destroyed (refine failure), all compounded cards are lost

---

## 2. Current State Analysis

### 2.1 What Exists

| Component | State | Details |
|-----------|-------|---------|
| **Hover tooltip (Inventory)** | Exists | Name, Type, Description, ATK/DEF/Weight/RequiredLv. Gold border, dark BG. (`SInventoryWidget::BuildTooltip()` line 398) |
| **Hover tooltip (Equipment)** | Exists | Name, Description, ATK/DEF/Weight (NO RequiredLevel). (`SEquipmentWidget::BuildTooltip()` line 403) |
| **Hover tooltip (Shop)** | Exists | Desc, ATK, DEF, MATK, MDEF, Weight, RequiredLv — 7 conditional fields. (`SShopWidget::BuildItemTooltip()` line 1002) |
| **Hover tooltip (Hotbar)** | None | No text tooltip on hotbar item/skill slots. Only hover highlight. |
| **Right-click (Inventory)** | Consumed silently | `OnMouseButtonDown` line 673 returns `FReply::Handled()` for non-LMB — blocks right-click but does nothing |
| **Right-click (Equipment)** | Shows hover tooltip | Right-click triggers `SetToolTip(BuildTooltip(Item))` (line 609) |
| **Right-click (Shop)** | None | No right-click handling in SShopWidget |
| **Right-click (Hotbar)** | Clears slot | Right-click calls `ClearSlot()` (line 556) — MUST NOT BREAK THIS |
| **Inspect window** | None | No detailed item inspect widget exists |
| **Card slot display** | None | No card slot visualization anywhere |
| **Card compound tracking** | None | `character_inventory` has no card tracking columns |

### 2.2 FInventoryItem Fields (Current — 31 fields)
```cpp
// CharacterData.h lines 34-74
InventoryId, ItemId, Name, Description, ItemType, EquipSlot,
Quantity, bIsEquipped, EquippedPosition, SlotIndex,
Weight, Price, ATK, DEF, MATK, MDEF,
StrBonus, AgiBonus, VitBonus, IntBonus, DexBonus, LukBonus,
MaxHPBonus, MaxSPBonus, RequiredLevel, bStackable, MaxStack,
Icon, WeaponType, ASPDModifier, WeaponRange
```

### 2.2b FShopItem Fields (Separate struct — 24 fields)
```cpp
// CharacterData.h lines 122-154 — used by SShopWidget, NOT FInventoryItem
ItemId, Name, Description, ItemType, Icon,
BuyPrice, SellPrice, Weight,
ATK, DEF, MATK, MDEF,
EquipSlot, WeaponType, WeaponRange, ASPDModifier,
RequiredLevel, bStackable,
StrBonus, AgiBonus, VitBonus, IntBonus, DexBonus, LukBonus,
MaxHPBonus, MaxSPBonus
```
**Key differences from FInventoryItem**: No InventoryId, Quantity, bIsEquipped, EquippedPosition, SlotIndex, MaxStack, Price. Has BuyPrice/SellPrice instead.

### 2.2c FHotbarSlot Fields (Lightweight — in HotbarSubsystem.h)
```cpp
// Only stores: RowIndex, SlotIndex, SlotType ("item"/"skill"),
// Item fields: InventoryId, ItemId, ItemName, ItemIcon, Quantity
// Skill fields: SkillId, SkillName, SkillIcon
```
**For inspect**: Must look up full FInventoryItem from InventorySubsystem by InventoryId.

### 2.3 Server Query (Current — `getPlayerInventory()`)
```sql
SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
       ci.equipped_position,
       i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
       i.atk, i.def, i.matk, i.mdef,
       i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
       i.max_hp_bonus, i.max_sp_bonus, i.required_level, i.stackable, i.icon,
       i.weapon_type, i.aspd_modifier, i.weapon_range,
       i.buy_price, i.sell_price
FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1
ORDER BY ci.slot_index ASC, ci.created_at ASC
```

### 2.3 Z-Order Map (Verified)
| Z | Widget | Notes |
|---|--------|-------|
| 5 | Login widgets | Login level only |
| 8 | WorldHealthBar | Floating HP bars |
| 10 | BasicInfo | HP/SP/EXP panel |
| 11 | BuffBar | Buff icons |
| 12 | CombatStats | F8 stats panel |
| 14 | Inventory | F6 toggle |
| 15 | Equipment | F7 toggle |
| 16 | Hotbar rows | Always visible |
| 18 | Shop | NPC shops |
| 19 | Kafra | Kafra dialog |
| 20 | SkillTree + DamageNumber | K toggle + floating numbers |
| **22** | **ItemInspect (NEW)** | **Right-click inspect window** |
| 25 | CastBar | Cast progress |
| 30 | Hotbar Keybind | Key config popup |
| 50 | Drag cursors, Loading overlays | High priority overlays |
| 100 | SkillTree Targeting | Click-to-cast overlay |

### 2.4 Missing Data (Not sent to client)
| Field | DB Column | Needed For |
|-------|-----------|-----------|
| `full_description` | `items.full_description` | Detailed inspect description |
| `slots` | `items.slots` | Card slot display (0-4) |
| `weapon_level` | `items.weapon_level` | Refine bonus calc, inspect display |
| `refineable` | `items.refineable` | Inspect display |
| `refine_level` | **MISSING** (needs new column on `character_inventory`) | Item name prefix (+7), refine bonus |
| `jobs_allowed` | `items.jobs_allowed` | Class restriction display |
| `card_type` | `items.card_type` | Card compound validation |
| `card_prefix` | `items.card_prefix` | Item name formatting |
| `card_suffix` | `items.card_suffix` | Item name formatting |
| `two_handed` | `items.two_handed` | Inspect display |
| `element` | `items.element` | Inspect display |
| `compounded_cards` | **MISSING** (needs new column/table) | Card slot display + effects |
| `buy_price` | `items.buy_price` | Already in query but not parsed by client |
| `sell_price` | `items.sell_price` | Already in query but not parsed by client |

---

## 3. Feature Scope

### 3.1 Hover Tooltip (Enhanced Simple)
**What it shows** (keep existing pattern, enhance):
- **Item Name** — formatted with refine level, card prefix/suffix, slot count: `+7 Bloody Sword [3]`
- **Item Type** — formatted nicely: "One-Handed Sword", "Body Armor", etc.
- **Short Description** — the `description` field (1-2 lines)
- **Key Stats** — ATK, DEF, MATK, MDEF (only if > 0)
- **Weight** — always shown

**Where it works**:
- [x] Inventory grid slots
- [x] Equipment panel slots
- [ ] Shop item list rows (BUY mode)
- [ ] Shop item list rows (SELL mode)
- [ ] Hotbar slots (items only, not skills)
- [ ] Card slots inside inspect window (filled slots)

### 3.2 Right-Click Inspect Window
**What it shows**:
- **Title bar** — Formatted item name + close [X] button
- **Large icon** — ~80x80px on the left
- **Detailed description** — `full_description` field, nicely formatted with section dividers
- **Card slot footer** — Only for equipment with `slots > 0`. Shows filled/empty slots.

**Where it works** (right-click opens inspect):
- [ ] Inventory grid slots
- [ ] Equipment panel slots
- [ ] Shop item list rows
- [ ] Hotbar item slots
- [ ] Card slots inside inspect window (filled slots → opens card inspect)

### 3.3 Card Slot Visualization (inside Inspect Window)
- Footer row at bottom of inspect window
- Number of slots matches `item.slots` (0-4)
- **Empty slot**: Diamond/rhombus shape in light brown/gold, slightly translucent
- **Filled slot**: Card icon (small) + card name text. Hovering shows card tooltip. Right-clicking opens card inspect.
- Only shown for equipment items with `slots > 0`

---

## 4. Server-Side Changes

### 4.1 Database Migration: `add_item_inspect_fields.sql`

```sql
-- Add refine_level to character_inventory
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS refine_level INTEGER DEFAULT 0;

-- Add compounded_cards as JSONB array to character_inventory
-- Format: [null, 4036, null, null] — index = slot position, value = card item_id or null
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS compounded_cards JSONB DEFAULT '[]'::jsonb;
```

**Why JSONB for compounded_cards:**
- Simple to read/write (single column, no JOINs)
- Array index = slot index (0 to slots-1)
- `null` = empty slot, integer = card item_id
- Example: `[4036, null, null]` = Hydra Card in slot 0, slots 1-2 empty
- Equipment max 4 slots so array is tiny
- Easy to validate: `jsonb_array_length(compounded_cards) <= items.slots`

### 4.2 Expand `getPlayerInventory()` Query

Add these fields to the SELECT:
```sql
i.full_description, i.slots, i.weapon_level, i.refineable,
i.jobs_allowed, i.card_type, i.card_prefix, i.card_suffix,
i.two_handed, i.element,
ci.refine_level, ci.compounded_cards
```

### 4.3 Expand `shop:data` Response

Shop items also need `full_description`, `slots`, `weapon_level`, `refineable`, `jobs_allowed`, `card_type`, `card_prefix`, `card_suffix`, `two_handed`, `element` for the inspect window.

### 4.4 New Socket Events (Future — Card Compounding)

**Note**: Card compounding functionality (the actual compound action) is NOT in scope for this plan. This plan covers the **display/visualization** only. The following events will be needed later:

| Event | Direction | Payload |
|-------|-----------|---------|
| `card:compound` | Client → Server | `{ cardInventoryId, equipInventoryId, slotIndex }` |
| `card:result` | Server → Client | `{ success, message, updatedItem }` |

---

## 5. Client-Side Changes

### 5.1 Expand `FInventoryItem` Struct (CharacterData.h)

Add these new fields after `WeaponRange`:
```cpp
// --- New fields for inspect/tooltip ---
FString FullDescription;        // Multi-line formatted description from DB
int32 Slots = 0;               // Number of card slots (0-4)
int32 WeaponLevel = 0;         // Weapon level (1-4), 0 for non-weapons
bool bRefineable = false;       // Can this item be refined?
int32 RefineLevel = 0;         // Current refine level (+0 to +10)
FString JobsAllowed;            // "Swordman,Merchant,Thief" or "All"
FString CardType;               // For cards: "weapon", "armor", etc.
FString CardPrefix;             // Card prefix name ("Bloody", "Titan")
FString CardSuffix;             // Card suffix name ("of Endure", "of Warmth")
bool bTwoHanded = false;        // Is this a two-handed weapon?
FString Element;                // Weapon element: "neutral", "fire", etc.
int32 BuyPrice = 0;            // NPC buy price
int32 SellPrice = 0;           // NPC sell price
TArray<int32> CompoundedCards;  // Card item_ids per slot (-1 = empty, >0 = card ID)
```

### 5.2 Expand `ParseItemFromJson()` (InventorySubsystem.cpp)

Add parsing for all new fields:
```cpp
// New fields
if (Obj->TryGetStringField(TEXT("full_description"), Str)) Item.FullDescription = Str;
if (Obj->TryGetNumberField(TEXT("slots"), Val)) Item.Slots = (int32)Val;
if (Obj->TryGetNumberField(TEXT("weapon_level"), Val)) Item.WeaponLevel = (int32)Val;
if (Obj->TryGetBoolField(TEXT("refineable"), bBool)) Item.bRefineable = bBool;
if (Obj->TryGetNumberField(TEXT("refine_level"), Val)) Item.RefineLevel = (int32)Val;
if (Obj->TryGetStringField(TEXT("jobs_allowed"), Str)) Item.JobsAllowed = Str;
if (Obj->TryGetStringField(TEXT("card_type"), Str)) Item.CardType = Str;
if (Obj->TryGetStringField(TEXT("card_prefix"), Str)) Item.CardPrefix = Str;
if (Obj->TryGetStringField(TEXT("card_suffix"), Str)) Item.CardSuffix = Str;
if (Obj->TryGetBoolField(TEXT("two_handed"), bBool)) Item.bTwoHanded = bBool;
if (Obj->TryGetStringField(TEXT("element"), Str)) Item.Element = Str;
if (Obj->TryGetNumberField(TEXT("buy_price"), Val)) Item.BuyPrice = (int32)Val;
if (Obj->TryGetNumberField(TEXT("sell_price"), Val)) Item.SellPrice = (int32)Val;

// Compounded cards (JSONB array → TArray<int32>)
const TArray<TSharedPtr<FJsonValue>>* CardsArray = nullptr;
if (Obj->TryGetArrayField(TEXT("compounded_cards"), CardsArray) && CardsArray)
{
    for (const TSharedPtr<FJsonValue>& CardVal : *CardsArray)
    {
        if (CardVal->IsNull())
            Item.CompoundedCards.Add(-1); // empty slot
        else
            Item.CompoundedCards.Add((int32)CardVal->AsNumber());
    }
}
```

### 5.3 Item Name Formatting Helper

Add a static helper function (could live in `FInventoryItem` or a utility header):

```cpp
// Returns formatted display name: "+7 Bloody Sword [3]"
FString GetDisplayName() const
{
    FString Result;

    // Refine prefix
    if (RefineLevel > 0)
        Result = FString::Printf(TEXT("+%d "), RefineLevel);

    // Card prefix(es) — simplified for now (full multiplier logic later)
    // TODO: count duplicate cards for Double/Triple/Quadruple prefix
    // For now, concatenate unique prefixes
    // ... (see Phase 2 implementation details)

    // Base name
    Result += Name;

    // Card suffix(es)
    // ... (see Phase 2 implementation details)

    // Slot count
    if (Slots > 0)
        Result += FString::Printf(TEXT(" [%d]"), Slots);

    return Result;
}
```

### 5.4 FullDescription Formatting Helper

The `full_description` field uses `________________________` (24 underscores) as section dividers. We need a parser that converts this to styled Slate widgets:

```cpp
// Parse full_description into sections for rich rendering
// Input: "A basic sword.\n________________________\nType: One-Handed Sword\nAttack: 25\n..."
// Output: Array of sections, each section = array of text lines
// Dividers become gold horizontal lines in the UI
```

**Parsing rules:**
1. Split by `________________________` into sections
2. Within each section, split by newlines
3. Lines matching `Label : Value` pattern → render Label in dim, Value in bright
4. Lines not matching → render as plain description text
5. Each `________________________` becomes a gold divider widget

---

## 6. SItemInspectWidget Design

### 6.1 Architecture

| Component | Purpose |
|-----------|---------|
| `SItemInspectWidget` | `SCompoundWidget` — the inspect popup window |
| `UItemInspectSubsystem` | `UWorldSubsystem` — manages widget lifecycle, provides `ShowInspect(item)` / `HideInspect()` |

**Why a separate subsystem** (not bolted onto InventorySubsystem):
- The inspect window can be opened from ANY context (inventory, equipment, shop, hotbar, card slots inside inspect)
- Clean separation of concerns
- Simple API: any widget calls `GetWorld()->GetSubsystem<UItemInspectSubsystem>()->ShowInspect(ItemData)`

### 6.2 ItemInspectSubsystem

```
UItemInspectSubsystem : UWorldSubsystem
├── ShowInspect(const FInventoryItem& Item)  — show inspect for item
├── ShowInspect(int32 ItemId)                — show inspect for card by ID (lookup from InventorySubsystem or itemDefinitions)
├── HideInspect()                            — close inspect window
├── IsInspectVisible() const                 — is the inspect window showing?
├── Widget: TSharedPtr<SItemInspectWidget>
├── AlignmentWrapper: TSharedPtr<SWidget>
├── ViewportOverlay: TSharedPtr<SWidget>
├── bWidgetAdded: bool
└── Z-Order: 22 (above skill tree Z=20, below cast bar Z=25)
```

**Lifecycle:**
- `ShouldCreateSubsystem` — only create in game worlds (check `GI->IsSocketConnected()`)
- `OnWorldBeginPlay` — no socket events needed (inspect is purely client-side, reads item data)
- Widget is created on first `ShowInspect()` call, then shown/hidden
- Widget is positioned at screen center (or near the right-clicked item, offset to not overlap)

### 6.3 SItemInspectWidget Layout

```
+============================================================+
| [Icon] Item Name                                      [X]  |  ← Title bar (draggable)
+============================================================+
|  +----------+  Description line 1                          |
|  |          |  Description line 2                          |
|  |   ICON   |  ──────────────────── (gold divider)         |
|  |  80×80   |  Type: One-Handed Sword                      |
|  |          |  Attack: 25                                  |
|  +----------+  Weight: 50                                  |
|                Weapon Level: 1                             |
|                Refinable: Yes                              |
|                ──────────────────── (gold divider)          |
|                Requirement:                                |
|                Base level 2                                |
|                Novice, Swordman, Merchant and Thief classes |
+============================================================+
| [Card1] [◇ Empty] [◇ Empty]                               |  ← Card slot footer (only if slots > 0)
+============================================================+
```

**Dimensions:**
- Width: 340px (fixed)
- Height: auto-fit content (Strategy B from sabrimmo-ui)
- Min height: ~120px
- Max height: ~400px (use scroll if content exceeds)
- Draggable by title bar
- Close button [X] in title bar

**Visual Styling (RO Classic Theme):**
- 3-layer frame: Gold outer → Dark inner → Brown panel (standard)
- Title bar: PanelDark background, gold text
- Icon area: Dark border, item icon texture (80x80)
- Description: TextPrimary color for values, TextDim for labels
- Dividers: GoldDivider color
- Card slot footer: PanelMedium background

### 6.4 SItemInspectWidget Construction Pseudocode

```
Construct(Item)
├── Title Bar
│   ├── Small Icon (24x24) — same as grid icon
│   ├── Formatted Item Name — GetDisplayName() in GoldHighlight
│   └── Close [X] Button — clears inspect
├── Content Area (SScrollBox if needed)
│   ├── Icon Column (left, 80x80)
│   │   └── Item Icon Texture or Placeholder
│   └── Description Column (right, fill)
│       └── FormatFullDescription(FullDescription)
│           ├── Section 1: Flavor/effect text (TextPrimary)
│           ├── Gold Divider
│           ├── Section 2: Stats (Label: dim, Value: bright)
│           ├── Gold Divider
│           └── Section 3: Requirements (TextDim)
└── Card Slot Footer (only if Slots > 0)
    └── Horizontal row of slot widgets
        ├── Filled: Card mini-icon (24x24) + card name + hover tooltip + right-click inspect
        └── Empty: Diamond shape in GoldDark, slightly translucent
```

### 6.5 FullDescription Renderer

The `FormatFullDescription` function takes the multi-line `full_description` text and produces a `TSharedRef<SVerticalBox>` with styled sections:

```cpp
TSharedRef<SWidget> FormatFullDescription(const FString& FullDesc)
{
    // 1. Split by "________________________" (24+ underscores)
    TArray<FString> Sections;
    FullDesc.ParseIntoArray(Sections, TEXT("________________________"));

    TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);

    for (int32 i = 0; i < Sections.Num(); i++)
    {
        FString Section = Sections[i].TrimStartAndEnd();
        if (Section.IsEmpty()) continue;

        // Add gold divider between sections (not before first)
        if (i > 0)
        {
            VBox->AddSlot().AutoHeight().Padding(0, 2)
            [ BuildGoldDivider() ];
        }

        // Split section into lines
        TArray<FString> Lines;
        Section.ParseIntoArrayLines(Lines);

        for (const FString& Line : Lines)
        {
            FString Trimmed = Line.TrimStartAndEnd();
            if (Trimmed.IsEmpty()) continue;

            // Check for "Label : Value" pattern (colon with space before it)
            int32 ColonIdx;
            if (Trimmed.FindChar(':', ColonIdx) && ColonIdx > 0)
            {
                FString Label = Trimmed.Left(ColonIdx).TrimEnd();
                FString Value = Trimmed.Mid(ColonIdx + 1).TrimStart();
                // Render as Label (dim) + Value (bright)
                VBox->AddSlot().AutoHeight()
                [ BuildLabelValueRow(Label, Value) ];
            }
            else
            {
                // Plain text line
                VBox->AddSlot().AutoHeight()
                [ BuildPlainTextLine(Trimmed) ];
            }
        }
    }
    return VBox;
}
```

---

## 7. Card Slot Visualization

### 7.1 Card Slot Footer Widget

The footer appears at the bottom of the inspect window for equipment with `Slots > 0`.

```
+----------------------------------------------------------+
| [Card1_Icon Card1_Name] [◇] [◇] [◇]                    |
+----------------------------------------------------------+
```

**Layout**: `SHorizontalBox` with one slot per card slot.

### 7.2 Empty Slot Appearance
- Diamond/rhombus shape
- Color: `GoldDark` with 50% opacity
- Size: ~24x24px area
- No interaction (no hover, no click)
- We draw this using a rotated SBox or a custom diamond brush

**Implementation options for diamond shape:**
1. **Rotated SBox**: 16x16 SBox with 45° render transform → appears as diamond
2. **Unicode character**: `◇` (U+25C7) or `◆` (U+25C6) rendered as STextBlock
3. **Custom texture**: Import a small diamond PNG

**Decision**: Use **Unicode diamond character** `◇` (simplest, no texture needed, matches RO style).

### 7.3 Filled Slot Appearance
- Small card icon (24x24) if available, else colored square placeholder
- Card name text next to icon
- **Hover**: Shows simple tooltip (card name + short description)
- **Right-click**: Opens a new inspect window for the card item

**Data flow for filled slots:**
1. `Item.CompoundedCards[slotIndex]` gives the `card_item_id`
2. Need to look up the card's full item data to display name, icon, description
3. **Option A**: Include card item data inline in the inventory payload (nested objects)
4. **Option B**: Client looks up card data from a local cache/map

**Decision**: **Option A** — Server includes card data inline. When sending `compounded_cards`, also send a `compounded_card_details` array with card name, description, icon, full_description for each non-null slot. This avoids the client needing a separate item database.

### 7.4 Server Payload Enhancement for Card Details

In `getPlayerInventory()`, after fetching the main items, for any item with non-empty `compounded_cards`, fetch card details:

```javascript
// For each item with compounded cards, resolve card details
for (const item of items) {
    if (item.compounded_cards && item.compounded_cards.length > 0) {
        item.compounded_card_details = [];
        for (const cardId of item.compounded_cards) {
            if (cardId) {
                const cardDef = itemDefinitions.get(cardId);
                item.compounded_card_details.push(cardDef ? {
                    item_id: cardId,
                    name: cardDef.name,
                    description: cardDef.description,
                    full_description: cardDef.full_description,
                    icon: cardDef.icon,
                    card_type: cardDef.card_type,
                    card_prefix: cardDef.card_prefix,
                    card_suffix: cardDef.card_suffix,
                    weight: cardDef.weight
                } : null);
            } else {
                item.compounded_card_details.push(null);
            }
        }
    }
}
```

### 7.5 Client-Side Card Data Struct

Add a lightweight struct for card details:
```cpp
USTRUCT()
struct FCompoundedCardInfo
{
    GENERATED_BODY()
    int32 ItemId = 0;
    FString Name;
    FString Description;
    FString FullDescription;
    FString Icon;
    FString CardType;
    FString CardPrefix;
    FString CardSuffix;
    int32 Weight = 0;
    bool IsValid() const { return ItemId > 0; }
};

// In FInventoryItem:
TArray<FCompoundedCardInfo> CompoundedCardDetails;  // Parallel to CompoundedCards
```

---

## 8. Integration Points

### 8.1 SInventoryWidget Integration

**Hover tooltip** (already exists at line 383 — enhance):
- Update `BuildTooltip()` to use formatted name (`GetDisplayName()`)
- Add slot count display
- Keep simple: name, type, short desc, key stats, weight

**Right-click inspect** (NEW):
```cpp
// In OnMouseButtonDown or OnMouseButtonUp for grid slots:
if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
{
    // Find which item was right-clicked
    if (Item.IsValid())
    {
        if (UItemInspectSubsystem* InspectSub = GetWorld()->GetSubsystem<UItemInspectSubsystem>())
        {
            InspectSub->ShowInspect(Item);
        }
        return FReply::Handled();
    }
}
```

### 8.2 SEquipmentWidget Integration

**Hover tooltip** (already exists — enhance same as inventory):
- Update to use formatted name

**Right-click inspect** (replace current right-click tooltip):
- Same pattern as inventory: call `InspectSub->ShowInspect(Item)`
- Remove current right-click tooltip behavior (line 609)

### 8.3 SShopWidget Integration

**Hover tooltip** (already exists — enhance):
- `BuildItemTooltip()` exists at line 1002 with 7 fields. Enhance to use formatted name and match shared tooltip style.
- Shop items are `FShopItem` structs (NOT `FInventoryItem`). Two approaches:
  - **Option A**: Add `FShopItem::ToInventoryItem()` conversion method
  - **Option B**: Make `ShowInspect()` accept both types via overload
  - **Option C**: Add `FShopItem` fields to match new FInventoryItem fields, then convert

**Decision**: **Option A** — Add `FInventoryItem FShopItem::ToInspectableItem() const` that copies shared fields. Simple, no interface changes. `FShopItem` also needs new fields added (FullDescription, Slots, WeaponLevel, etc.) and server `shop:data` expanded.

**Right-click inspect** (NEW — no right-click handler exists currently):
- Add RMB check in `SShopWidget::OnMouseButtonDown`
- Convert FShopItem → FInventoryItem → `ShowInspect()`

### 8.4 SHotbarRowWidget Integration

**Hover tooltip** (NEW — no text tooltip exists, only hover highlight):
- For item slots: look up full `FInventoryItem` from `InventorySubsystem` by `FHotbarSlot::InventoryId`
- For skill slots: no item tooltip (skill tooltip is separate concern)
- Add `SetToolTip()` in `BuildSlot()` for item slots

**Right-click behavior** (EXISTING — clears slot at line 556):
- Right-click currently calls `ClearSlot()` — this is useful and must be preserved
- **Decision**: Do NOT repurpose right-click for inspect on hotbar. Instead:
  - **Hover tooltip** shows item name + desc (simple, always visible)
  - **Shift+Right-click** or **Middle-click** opens inspect (if desired)
  - OR: keep right-click = clear, rely on hover tooltip for info. Inspect not strictly needed here since the same item is in inventory.
- **Final decision**: Right-click stays as clear slot. Hover tooltip added for item slots. No inspect from hotbar (items can be inspected in inventory).

### 8.4 SHotbarRowWidget Integration

**Hover tooltip** (for item slots only, not skills):
- Check if hotbar slot contains an item (not a skill)
- Look up item from InventorySubsystem
- Show same tooltip

**Right-click inspect** (for item slots):
- Same pattern: lookup item → `ShowInspect()`

### 8.5 SInventoryWidget Right-Click Fix

**Current behavior**: `OnMouseButtonDown` line 673 returns `FReply::Handled()` for ALL non-LMB buttons, silently consuming right-clicks. Must add an explicit RMB check BEFORE this fallback:

```cpp
// Add BEFORE the final return FReply::Handled()
if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
{
    int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
    FInventoryItem Item = GetItemAtSlot(SlotIdx);
    if (Item.IsValid())
    {
        if (UItemInspectSubsystem* InspectSub = GetWorld()->GetSubsystem<UItemInspectSubsystem>())
        {
            InspectSub->ShowInspect(Item);
        }
        return FReply::Handled();
    }
}
```

### 8.6 Card Slot Interaction (Inside Inspect Window)

**Hover tooltip on filled card slot**:
- Use Slate native `SToolTip` on the card slot widget
- Shows card name + short description

**Right-click on filled card slot**:
- Opens a NEW inspect window (replaces current one, or opens a second one)
- Uses the `CompoundedCardDetails[slotIndex]` data
- **Decision**: Replace the current inspect with the card inspect (one window at a time). Close button or clicking outside returns to the previous item, OR just show card inspect independently.

---

## 9. Implementation Phases

### Phase 1: Data Pipeline (Server + Client Struct)
**Goal**: Get all necessary item data flowing from server to client.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 1.1 | Create DB migration: `add_item_inspect_fields.sql` | `database/migrations/` | [x] |
| 1.2 | Run migration (add `refine_level`, `compounded_cards` to `character_inventory`) | DB | [x] |
| 1.3 | Expand `getPlayerInventory()` SELECT to include 10 new item fields + 2 new CI fields | `server/src/index.js` ~line 1623 | [x] |
| 1.4 | Add card detail resolution for compounded cards (inline in inventory response) | `server/src/index.js` after getPlayerInventory | [x] |
| 1.5 | Expand `shop:data` handler to include 10 new item fields | `server/src/index.js` ~line 6853 | [x] |
| 1.6 | Add `FCompoundedCardInfo` struct to CharacterData.h | `CharacterData.h` | [x] |
| 1.7 | Expand `FInventoryItem` struct with 15 new fields | `CharacterData.h` lines 69-70 | [x] |
| 1.8 | Expand `FShopItem` struct with new fields (FullDescription, Slots, WeaponLevel, etc.) | `CharacterData.h` lines 122-154 | [x] |
| 1.9 | Add `FShopItem::ToInspectableItem()` conversion method | `CharacterData.h` | [x] |
| 1.10 | Add `GetDisplayName()` helper to `FInventoryItem` | `CharacterData.h` | [x] |
| 1.11 | Expand `ParseItemFromJson()` to parse all new fields | `InventorySubsystem.cpp` ~line 94 | [x] |
| 1.12 | Expand shop item parser in `ShopSubsystem.cpp` to parse new fields | `ShopSubsystem.cpp` | [x] |
| 1.13 | Verify data flow: server sends → client receives all new fields | Test | [x] |

### Phase 2: Hover Tooltip Enhancement
**Goal**: Update existing tooltips to show formatted names and consistent info across all widgets.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 2.1 | Extract shared `BuildItemTooltip(const FInventoryItem&)` into a static/free function to avoid duplication across 4 widgets | New: `UI/ItemTooltipBuilder.h/.cpp` or inline in a shared header | [x] |
| 2.2 | Shared tooltip shows: GetDisplayName(), Type (formatted), short Description, ATK/DEF/MATK/MDEF (if >0), Weight, RequiredLevel (if >1) | `ItemTooltipBuilder` | [x] |
| 2.3 | Replace `SInventoryWidget::BuildTooltip()` with shared builder | `SInventoryWidget.cpp` ~line 398 | [x] |
| 2.4 | Replace `SEquipmentWidget::BuildTooltip()` with shared builder (add missing RequiredLevel) | `SEquipmentWidget.cpp` ~line 403 | [x] |
| 2.5 | Update `SShopWidget::BuildItemTooltip()` to use shared builder (convert FShopItem first) | `SShopWidget.cpp` ~line 1002 | [x] |
| 2.6 | Add hover tooltip to `SHotbarRowWidget` item slots (lookup FInventoryItem from InventorySubsystem by InventoryId) | `SHotbarRowWidget.cpp` ~line 144 | [x] |
| 2.7 | Verify all 4 widgets show consistent tooltip style | Test | [x] |

### Phase 3: Item Inspect Subsystem
**Goal**: Create the subsystem that manages the inspect overlay.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 3.1 | Create `ItemInspectSubsystem.h` header | `UI/ItemInspectSubsystem.h` | [x] |
| 3.2 | Create `ItemInspectSubsystem.cpp` implementation | `UI/ItemInspectSubsystem.cpp` | [x] |
| 3.3 | Implement `ShouldCreateSubsystem()` — game world only | `ItemInspectSubsystem.cpp` | [x] |
| 3.4 | Implement `ShowInspect(const FInventoryItem&)` | `ItemInspectSubsystem.cpp` | [x] |
| 3.5 | Implement `HideInspect()` | `ItemInspectSubsystem.cpp` | [x] |
| 3.6 | Viewport overlay at Z=22 with alignment wrapper | `ItemInspectSubsystem.cpp` | [x] |
| 3.7 | Handle "Escape" key to close inspect (optional) | `ItemInspectSubsystem.cpp` | [x] |

### Phase 4: SItemInspectWidget
**Goal**: Build the inspect window widget.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 4.1 | Create `SItemInspectWidget.h` header | `UI/SItemInspectWidget.h` | [x] |
| 4.2 | Create `SItemInspectWidget.cpp` implementation | `UI/SItemInspectWidget.cpp` | [x] |
| 4.3 | Implement `Construct()` with 3-layer RO frame | `SItemInspectWidget.cpp` | [x] |
| 4.4 | Build title bar (small icon + formatted name + [X] close) | `SItemInspectWidget.cpp` | [x] |
| 4.5 | Build content area (large icon left + description right) | `SItemInspectWidget.cpp` | [x] |
| 4.6 | Implement `FormatFullDescription()` parser (section splitter + label:value styling) | `SItemInspectWidget.cpp` | [x] |
| 4.7 | Build icon rendering (texture lookup via InventorySubsystem icon cache, or own cache) | `SItemInspectWidget.cpp` | [x] |
| 4.8 | Implement `UpdateItem()` to swap displayed item without recreating widget | `SItemInspectWidget.cpp` | [x] |
| 4.9 | Implement drag-by-title-bar (DPI-correct) | `SItemInspectWidget.cpp` | [x] |
| 4.10 | Implement close [X] button | `SItemInspectWidget.cpp` | [x] |
| 4.11 | Implement click-outside-to-close (optional) | `SItemInspectWidget.cpp` | [x] |
| 4.12 | Add scroll support for long descriptions (SScrollBox) | `SItemInspectWidget.cpp` | [x] |

### Phase 5: Card Slot Footer
**Goal**: Add card slot visualization to the inspect window.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 5.1 | Build `BuildCardSlotFooter()` method | `SItemInspectWidget.cpp` | [x] |
| 5.2 | Render empty slots as diamond `◇` characters | `SItemInspectWidget.cpp` | [x] |
| 5.3 | Render filled slots with card icon + name | `SItemInspectWidget.cpp` | [x] |
| 5.4 | Add hover tooltip to filled card slots | `SItemInspectWidget.cpp` | [x] |
| 5.5 | Add right-click inspect to filled card slots (opens card inspect) | `SItemInspectWidget.cpp` | [x] |
| 5.6 | Style footer: PanelMedium background, gold border top | `SItemInspectWidget.cpp` | [x] |

### Phase 6: Right-Click Integration
**Goal**: Wire up right-click in all item-showing widgets to open inspect.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 6.1 | Add RMB check to `SInventoryWidget::OnMouseButtonDown` BEFORE the fallback `return FReply::Handled()` at line 673 | `SInventoryWidget.cpp` | [x] |
| 6.2 | Replace RMB handler in `SEquipmentWidget` (line 609: currently shows tooltip → change to ShowInspect) | `SEquipmentWidget.cpp` | [x] |
| 6.3 | Add RMB handler to `SShopWidget::OnMouseButtonDown` — convert FShopItem → FInventoryItem → ShowInspect | `SShopWidget.cpp` | [x] |
| 6.4 | **Hotbar**: Do NOT add inspect on right-click (right-click = ClearSlot, line 556). Tooltip-only for hotbar items. | — | [N/A] |
| 6.5 | Test: right-click in inventory, equipment, and shop opens correct inspect | Test | [x] |
| 6.6 | Test: right-click in hotbar still clears slot (no regression) | Test | [x] |

### Phase 7: Polish & Edge Cases
**Goal**: Handle all edge cases and polish the UX.

| # | Task | File(s) | Status |
|---|------|---------|--------|
| 7.1 | Handle items with no `full_description` (fallback to `description`) | `SItemInspectWidget.cpp` | [x] |
| 7.2 | Handle items with no icon (show placeholder with item type color) | `SItemInspectWidget.cpp` | [x] |
| 7.3 | Handle empty CompoundedCards (slots exist but no cards compound data from server) | `SItemInspectWidget.cpp` | [x] |
| 7.4 | Ensure inspect window doesn't go off-screen | `SItemInspectWidget.cpp` | [x] |
| 7.5 | Ensure only one inspect window at a time (close previous when opening new) | `ItemInspectSubsystem.cpp` | [x] |
| 7.6 | Close inspect window on zone transition | `ItemInspectSubsystem.cpp` | [x] |
| 7.7 | Close inspect window if inspected item is consumed/dropped/equipped | `ItemInspectSubsystem.cpp` | [x] |
| 7.8 | Ensure inspect doesn't block game input (SelfHitTestInvisible on wrapper) | `ItemInspectSubsystem.cpp` | [x] |
| 7.9 | Update documentation | `docsNew/` | [x] |

---

## 10. Completion Checklist

### Server
- [x] DB migration run successfully (`refine_level`, `compounded_cards`)
- [x] `getPlayerInventory()` returns all 15 new fields
- [x] Card details resolved inline for compounded cards
- [x] `shop:data` returns new fields
- [x] Server starts without errors after changes
- [x] Existing inventory/equipment/shop functionality unchanged (no regressions)

### Client — Data
- [x] `FInventoryItem` has all 15 new fields
- [x] `FCompoundedCardInfo` struct defined
- [x] `ParseItemFromJson()` handles all new fields
- [x] `GetDisplayName()` formats names correctly (refine + prefix + base + suffix + slots)
- [x] Compounded card details parsed correctly

### Client — Hover Tooltip
- [x] Shared `BuildItemTooltip()` exists (no duplication across 4 widgets)
- [x] Inventory tooltip uses shared builder with `GetDisplayName()`
- [x] Equipment tooltip uses shared builder (now includes RequiredLevel)
- [x] Shop tooltip enhanced to use shared builder (FShopItem→FInventoryItem conversion)
- [x] Hotbar tooltip works for item slots (lookup from InventorySubsystem by InventoryId)
- [x] All tooltips use `FLinearColor` (not `FColor`)

### Client — Inspect Window
- [x] `ItemInspectSubsystem` creates/shows/hides correctly
- [x] Widget at Z=22, alignment wrapper, SelfHitTestInvisible
- [x] Title bar shows icon + formatted name + [X] close
- [x] Large icon renders (80x80, texture or placeholder)
- [x] `FormatFullDescription()` handles sections, dividers, label:value pairs
- [x] Scroll works for long descriptions
- [x] Draggable by title bar (DPI-correct)
- [x] Close button works
- [x] Only one inspect open at a time

### Client — Card Slots
- [x] Card slot footer appears for equipment with `slots > 0`
- [x] Empty slots show `◇` diamond in gold
- [x] Filled slots show card icon + name
- [x] Filled slots have hover tooltip (card name + description)
- [x] Filled slots have right-click inspect (opens card inspect)
- [x] Footer hidden for items with `slots == 0`

### Client — Right-Click Integration
- [x] Right-click works in Inventory (added before fallback FReply::Handled at line 673)
- [x] Right-click works in Equipment (replaced tooltip-on-RMB with ShowInspect)
- [x] Right-click works in Shop (new handler, converts FShopItem)
- [x] Right-click in Hotbar still clears slot (NO CHANGE — no regression)
- [x] Right-click on card slot in inspect opens card inspect

### Edge Cases & Regressions
- [x] Item with no `full_description` falls back to `description`
- [x] Item with no icon shows colored placeholder
- [x] Equipment with 0 slots: no footer
- [x] Card items: no footer (cards don't have slots)
- [x] Consumables: no footer
- [x] Etc items: no footer
- [x] Inspect closes on zone transition
- [x] Inspect closes if item is consumed/dropped
- [x] Inspect doesn't go off-screen
- [x] No regressions in drag-and-drop (right-click doesn't interfere with LMB drag in inventory)
- [x] No regressions in hotbar right-click → ClearSlot still works
- [x] No regressions in equipment right-click (now opens inspect instead of tooltip)
- [x] Inventory double-click use/equip still works (LMB OnMouseButtonUp handler unchanged)
- [x] Shop quantity dialog still works (LMB buy/sell flow unchanged)
- [x] Hotbar item slots show tooltip even after item is consumed (graceful handling if InventoryId no longer exists)

---

## Appendix A: File Inventory

### Files to CREATE
| File | Purpose |
|------|---------|
| `database/migrations/add_item_inspect_fields.sql` | DB migration |
| `client/.../UI/ItemInspectSubsystem.h` | Inspect subsystem header |
| `client/.../UI/ItemInspectSubsystem.cpp` | Inspect subsystem impl |
| `client/.../UI/SItemInspectWidget.h` | Inspect widget header |
| `client/.../UI/SItemInspectWidget.cpp` | Inspect widget impl |
| `client/.../UI/ItemTooltipBuilder.h` | Shared tooltip builder (static/free function, avoids duplication) |
| `client/.../UI/ItemTooltipBuilder.cpp` | Shared tooltip builder impl |

### Files to MODIFY
| File | Changes |
|------|---------|
| `server/src/index.js` | Expand `getPlayerInventory()` query (~line 1623), add card detail resolution, expand `shop:data` (~line 6853) |
| `client/.../CharacterData.h` | Add `FCompoundedCardInfo`, expand `FInventoryItem` (+15 fields, `GetDisplayName()`), expand `FShopItem` (+10 fields, `ToInspectableItem()`) |
| `client/.../UI/InventorySubsystem.cpp` | Expand `ParseItemFromJson()` (~line 94) |
| `client/.../UI/ShopSubsystem.cpp` | Expand shop item parser to parse new fields |
| `client/.../UI/SInventoryWidget.cpp` | Replace `BuildTooltip()` with shared builder, add RMB handler before line 673 |
| `client/.../UI/SEquipmentWidget.cpp` | Replace `BuildTooltip()` with shared builder, replace RMB handler at line 609 |
| `client/.../UI/SShopWidget.cpp` | Replace `BuildItemTooltip()` with shared builder, add RMB handler |
| `client/.../UI/SHotbarRowWidget.cpp` | Add item hover tooltip in `BuildSlot()` (~line 144), NO right-click changes |
| `client/.../SabriMMO.Build.cs` | No changes expected (SlateCore already included) |

### Files to READ (Reference)
| File | Why |
|------|-----|
| `UI/SSkillTooltipWidget.h/.cpp` | Tooltip styling patterns |
| `UI/SBasicInfoWidget.h/.cpp` | DPI-correct drag, viewport overlay |
| `UI/SCombatStatsWidget.h/.cpp` | Auto-height Strategy B |
| `UI/SkillTreeSubsystem.h/.cpp` | Subsystem lifecycle reference |

---

## Appendix B: Color Palette (Inspect Window)

Reuses existing ROColors namespace plus inspect-specific additions:

```cpp
namespace InspectColors
{
    // Inherit from ROColors
    static const FLinearColor PanelBrown    = ROColors::PanelBrown;
    static const FLinearColor PanelDark     = ROColors::PanelDark;
    static const FLinearColor PanelMedium   = ROColors::PanelMedium;
    static const FLinearColor GoldTrim      = ROColors::GoldTrim;
    static const FLinearColor GoldDark      = ROColors::GoldDark;
    static const FLinearColor GoldHighlight = ROColors::GoldHighlight;
    static const FLinearColor GoldDivider   = ROColors::GoldDivider;
    static const FLinearColor TextPrimary   = ROColors::TextPrimary;
    static const FLinearColor TextBright    = ROColors::TextBright;
    static const FLinearColor TextShadow    = ROColors::TextShadow;

    // Inspect-specific
    static const FLinearColor LabelText     (0.65f, 0.55f, 0.40f, 1.f);  // Dim labels
    static const FLinearColor ValueText     (0.85f, 0.80f, 0.70f, 1.f);  // Stat values
    static const FLinearColor SlotEmpty     (0.50f, 0.38f, 0.15f, 0.50f); // Empty diamond
    static const FLinearColor SlotFilled    (0.72f, 0.58f, 0.28f, 1.f);  // Filled card bg
    static const FLinearColor CardNameText  (0.80f, 0.70f, 0.50f, 1.f);  // Card name in slot
    static const FLinearColor CloseButton   (0.85f, 0.25f, 0.20f, 1.f);  // [X] button
}
```

---

## Appendix C: Interaction Flow Diagrams

### Right-Click → Inspect Flow
```
User right-clicks item in Inventory
  → SInventoryWidget::OnMouseButtonDown detects RMB
  → Finds FInventoryItem for clicked slot
  → Gets UItemInspectSubsystem from World
  → Calls InspectSub->ShowInspect(Item)
  → Subsystem creates/updates SItemInspectWidget
  → Widget renders: title + icon + description + card slots
  → Widget added to viewport at Z=22
```

### Card Slot Right-Click Flow
```
User right-clicks filled card slot in Inspect window
  → SItemInspectWidget detects RMB on card slot
  → Gets CompoundedCardDetails[slotIndex]
  → Builds FInventoryItem from FCompoundedCardInfo
  → Calls InspectSub->ShowInspect(CardAsItem)
  → Widget updates to show card info (no slot footer)
```

### Close Inspect Flow
```
User clicks [X] or presses Escape or right-clicks another item
  → InspectSub->HideInspect()
  → Widget removed from viewport
  → bWidgetAdded = false
```
