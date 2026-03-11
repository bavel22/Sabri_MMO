# Skill Tree UI Rewrite Plan — Complete Reference

**Status**: Complete (2026-03-10)
**Created**: 2026-03-10
**Last Updated**: 2026-03-10

---

## Table of Contents
1. [Context & Problem Statement](#context--problem-statement)
2. [RO Classic Research Findings](#ro-classic-research-findings)
3. [Current Implementation Analysis](#current-implementation-analysis)
4. [Design Specification](#design-specification)
5. [Implementation Phases](#implementation-phases)
6. [Detailed Code Specifications](#detailed-code-specifications)
7. [File Change Summary](#file-change-summary)
8. [Design Decisions & Rationale](#design-decisions--rationale)
9. [Verification Checklist](#verification-checklist)

---

## 1. Context & Problem Statement

### What's Wrong Now

The current skill tree UI (`SSkillTreeWidget`) has these critical deficiencies:

1. **No tree structure**: Uses `SWrapBox` (flowing wrap layout) — skills are 140px wide cards that wrap left-to-right with no spatial relationship to each other
2. **No prerequisite visualization**: Players cannot see which skills require which other skills. The only indication is a `bCanLearn` boolean that greys out skills — but no lines, arrows, or labels show WHY a skill is locked
3. **No hover tooltips**: All info is crammed inline (description, SP cost), making each skill slot tall and the UI cluttered
4. **No per-level breakdowns**: Players can only see current level's SP cost. They have no way to know what level 5 vs level 10 of Bash does without external wikis
5. **No visual state differentiation**: Skills use subtle background color differences that are hard to distinguish (dark blue vs dark green vs dark brown)
6. **Excessive per-slot content**: Each 140px slot shows icon, name, level, SP, full description text, learn button, AND 9 hotbar quick-assign buttons — too much visual noise

### What We Want

An RO Classic-style skill tree with:
- Grid layout where skills sit at their `treeRow/treeCol` positions
- Visual connecting lines from prerequisite skills to dependent skills
- Rich hover tooltips showing ALL per-level data (SP cost, cast time, cooldown, damage for every level)
- Clear visual states: locked/learnable/learned/maxed
- Compact cells with icons still visible and draggable to hotbar
- Existing tab system, drag-to-hotbar, and RO brown/gold theme preserved

---

## 2. RO Classic Research Findings

### 2.1 Visual Layout History

**Pre-Renewal / Classic (before Episode 13.1, June 2009)**:
- Simple vertical scrollable list — no tree, no lines
- Skills shown with icon, name, and current level
- Opened with Alt+S

**Post-Episode 13.1 / Renewal (2009+)**:
- Replaced list with a **visual tree/grid layout**
- Skills arranged in a **7-column grid** using `Pos` values from `skilltreeview.lua`
- Position mapping: `column = Pos % 7`, `row = Pos / 7` (integer division)
- Skills flow top-to-bottom, prerequisites above dependents
- **Connecting lines** drawn between prerequisite and dependent skills
- Classic list view still accessible via minimize button
- Both "Full Skill Tree" and "Mini Skill Tree" window modes

### 2.2 Grid Position Mapping (from `skilltreeview.lua`)

Example Mage tree grid positions:
```
Pos 1  = Row 0, Col 1: Stone Curse
Pos 2  = Row 0, Col 2: Cold Bolt
Pos 3  = Row 0, Col 3: Lightning Bolt
Pos 4  = Row 0, Col 4: Napalm Beat
Pos 5  = Row 0, Col 5: Fire Bolt
Pos 6  = Row 0, Col 6: Sight
Pos 8  = Row 1, Col 1: SP Recovery
Pos 9  = Row 1, Col 2: Frost Diver     (requires Cold Bolt)
Pos 10 = Row 1, Col 3: Thunder Storm   (requires Lightning Bolt)
Pos 11 = Row 1, Col 4: Soul Strike     (requires Napalm Beat)
Pos 12 = Row 1, Col 5: Fire Ball       (requires Fire Bolt)
Pos 13 = Row 1, Col 6: Energy Coat
Pos 18 = Row 2, Col 4: Safety Wall     (requires Napalm Beat + Soul Strike)
Pos 19 = Row 2, Col 5: Fire Wall       (requires Sight + Fire Ball)
```

This creates a clear downward flow showing skill progression paths.

### 2.3 Skill Icons
- **Size**: 24x24 pixels (standard), some at 29x29
- **Format**: BMP in client GRF archives, PNG on wikis
- Each icon occupies one cell in the grid
- Over 509 unique skill icons cataloged

### 2.4 Prerequisite Lines
- **Vertical lines** connect parent skills (above) to dependents (below)
- Lines flow **downward** following the tree hierarchy
- Multiple prerequisites: multiple lines converge on the dependent skill (e.g., Safety Wall needs both Napalm Beat Lv 7 AND Soul Strike Lv 5)
- Prerequisite level is shown in **tooltip text** (grey, e.g., `"Skill Requirement : Sight 1, Fire Ball 5"`)
- Skills with unmet prerequisites are visually dimmed/greyed

### 2.5 Tooltip Format (from `skilldescript.lua`)

The tooltip uses inline color codes (`^RRGGBB` format). Structure:

**Fire Bolt tooltip example:**
```
Fire Bolt
MAX Lv : 10
Skill Form: Offensive        (grey text)
Property: Fire               (red text: ^bb0000)
Target: Enemy                (grey text)
Description: Summon bolts of pure flame to
strike at an enemy. Each bolt inflicts
an amount of damage equal to the caster's Matk.
[Lv 1]: 12sp, 0.7sec cast, 1 Bolt
[Lv 2]: 14sp, 1.4sec cast, 2 Bolts
[Lv 3]: 16sp, 2.1sec cast, 3 Bolts
...
[Lv 10]: 30sp, 7 sec cast, 10 Bolts
```

**Increase Agility tooltip (with prerequisites):**
```
Increase Agility
MAX Lv : 10
Skill Requirement : Heal 3   (grey text)
Skill Form: Supportive
Target: Player
Description: Increase targeted character's
Movement and Attack Speeds...
[Lv 1]: 18sp, +3 Agi
[Lv 2]: 21sp, +4 Agi
...
[Lv 10]: 45sp, +12 Agi
```

**Key observations:**
- Shows ALL levels, not just current/next — complete level table at a glance
- Tooltip does NOT dynamically update based on player's current level
- SP costs shown inline with each level entry
- Cast times shown per level where applicable
- Prerequisites listed with required levels
- Color codes: `^000000` black, `^777777` grey (descriptions/values), `^bb0000` red (fire), `^0000bb` blue (water), `^bbbb00` yellow (wind)

### 2.6 Skill Level-Up Interaction

**Renewal model (staged allocation)**:
1. Click skill to increment level by 1 (tentative)
2. "Apply" button confirms all pending allocations permanently
3. "Reset" button returns unconfirmed allocations
4. Once applied, permanent (only undone by special item "Neuralizer")

**RateMyServer simulator model:**
- +/- buttons next to each skill
- Level shown as "Current/Max" (e.g., "5/10")
- Dependent skills auto-unlock when prerequisites met

**Our approach**: Keep current immediate-learn model (click Learn → server commits immediately) since we already have "Reset All" functionality. No staged allocation needed.

### 2.7 Skill Point Display
- Remaining skill points shown in the window
- Tracked per job tier for multi-job characters
- Notification when all points exhausted
- Notification icon when new point earned (job level up)

### 2.8 Tab System
- Tabs separate skills by **job progression tier**
- Tab 1: Novice + First Job skills
- Tab 2: Second Job skills
- Tab 3: Third Job skills (Renewal only)
- Misc tab: Quest skills
- TAB key cycles between tabs

### 2.9 Skill States (Visual)
| State | Visual |
|-------|--------|
| Not learnable (prereqs unmet) | Greyed/dimmed icon, cannot click |
| Learnable (prereqs met, have points) | Full-color, clickable |
| Learned (1+ points) | Full-color + "Current/Max" level |
| Maxed | Highlighted text, no further allocation |
| Quest skills | "(Quest Skill)" label, not learnable via points |
| On cooldown (hotbar only) | Greyed icon on hotbar |

### 2.10 Drag to Hotbar
- Click-hold skill icon in tree → drag to hotbar slot
- Hotbar has 9 slots per row, multiple rows (F12 cycles)
- Only learned skills (level >= 1) can be placed
- Both skills and items can occupy hotbar

### 2.11 Right-Click Detailed Info
- Right-clicking skill icon opens detailed popup window
- Shows: skill name, ID, type, max level, target, range, element, AoE, prerequisites, SP cost table, cast time table, damage formula, status effects, knockback, required items

### 2.12 RO Client Data Files
| File | Purpose |
|------|---------|
| `skilltreeview.lua` | Grid positions (Pos), max levels, prerequisites per job |
| `skilldescript.lua` | Tooltip text with color codes |
| `skillinfolist.lua` | Skill metadata: name, MaxLv, SpAmount array, prerequisites |
| `skillinfo_f.lua` | Helper functions |

### Sources
- iRO Wiki: https://irowiki.org/wiki/Skills
- rAthena Wiki: https://github.com/rathena/rathena/wiki/Adding_new_skills
- RO Client Scripts: https://github.com/scriptord3/Ragnarok-Client-Scripts
- ROenglishPRE: https://github.com/zackdreaver/ROenglishPRE
- RateMyServer: https://ratemyserver.net/skill_sim.php
- Divine Pride: https://www.divine-pride.net/tools/skilltree/
- RO Episode 13.1 Notes: http://renewal.playragnarok.com/news/updatedetail.aspx?id=69

---

## 3. Current Implementation Analysis

### 3.1 Architecture Overview

```
USkillTreeSubsystem (UWorldSubsystem)
├── Manages data: SkillGroups[], LearnedSkills, SkillPoints, ActiveBuffs, CooldownExpiry
├── Socket events: skill:data, skill:learn, skill:used, skill:error, hotbar:alldata, etc.
├── Widget lifecycle: ShowWidget(), HideWidget(), ToggleWidget()
├── Skill targeting: BeginTargeting(), CancelTargeting(), UseSkill(), UseSkillOnTarget/Ground()
├── Icon cache: GetOrCreateIconBrush(), IconBrushCache, IconTextureCache (GC-rooted)
├── Drag-to-hotbar: StartSkillDrag(), CancelSkillDrag()
└── Creates/manages:
    ├── SSkillTreeWidget (main panel, Z=20)
    ├── SSkillTargetingOverlay (fullscreen click-to-cast, Z=100)
    └── Drag cursor overlay
```

### 3.2 Current Data Structures

**`FSkillPrerequisite`** (in `SkillTreeSubsystem.h`):
```cpp
int32 RequiredSkillId = 0;
int32 RequiredLevel = 0;
```

**`FSkillEntry`** (in `SkillTreeSubsystem.h`):
```cpp
int32 SkillId, MaxLevel, CurrentLevel, Range, TreeRow, TreeCol
int32 SpCost, NextSpCost, CastTime, Cooldown, EffectValue
FString Name, DisplayName, Type, TargetType, Element, Description, Icon, IconPath
bool bCanLearn
TArray<FSkillPrerequisite> Prerequisites
// MISSING: TArray<FSkillLevelInfo> AllLevels  <-- needs to be added
```

**`FSkillClassGroup`** (in `SkillTreeSubsystem.h`):
```cpp
FString ClassId, ClassDisplayName
TArray<FSkillEntry> Skills
```

### 3.3 Current Widget (SSkillTreeWidget)

**Header** (`SSkillTreeWidget.h`):
- Deferred rebuild flags: `bPendingFullRebuild`, `bPendingGridRebuild`
- Window drag: `bIsDragging`, `DragOffset`, `DragStartWidgetPos`, `WidgetPosition(300,100)`, `CurrentSize(460,500)`
- Skill drag: `bSkillDragInitiated`, `DragSourceSkillId/Name/Icon`, `SkillDragStartPos`, `SkillDragThreshold=5.f`
- Widget refs: `ClassTabsContainer (SVerticalBox)`, `SkillContentBox (SVerticalBox)`
- `ActiveClassTab = 0`

**Construction** (`SSkillTreeWidget.cpp:Construct`):
```
SBox (460x500)
└── SBorder (Gold Trim, 2px)
    └── SBorder (Dark Inset, 1px)
        └── SBorder (Brown Panel)
            └── SVerticalBox
                ├── Title Bar (dark bg): "Skill Tree" + "Points: N" + X close
                ├── ClassTabsContainer (SVerticalBox, dynamic tabs)
                ├── Gold Divider
                ├── SScrollBox (vertical)
                │   └── SkillContentBox (SVerticalBox)
                │       └── SWrapBox (130px wide skill slots)
                ├── Gold Divider
                └── Bottom Bar: "Remaining Skill Points: N" + Reset All
```

**Grid rebuild** (`DoRebuildSkillGrid`):
1. Sorts skills by `treeRow` then `treeCol`
2. Creates `SWrapBox` with `.UseAllottedSize(true)`
3. For each skill: builds a 140px-wide slot with:
   - Row 1: Icon (24x24, draggable if learned active/toggle) + Name (bold, auto-wrap)
   - Row 2: "Lv X/X" + "SP: N"
   - Row 3: Description (dim, auto-wrap, takes LOTS of space)
   - Row 4: Learn/Level Up button (green, if canLearn && !maxed)
   - Row 5: Hotbar quick-assign [1]-[9] buttons (if learned active/toggle)
4. Wraps each slot in gold-dark border → state-colored border

**Color palette** (`SKColors` namespace):
```cpp
PanelBrown    (0.43, 0.29, 0.17)
PanelDark     (0.22, 0.14, 0.08)
PanelMedium   (0.33, 0.22, 0.13)
GoldTrim      (0.72, 0.58, 0.28)
GoldDark      (0.50, 0.38, 0.15)
GoldHighlight (0.92, 0.80, 0.45)
GoldDivider   (0.60, 0.48, 0.22)
TextPrimary   (0.96, 0.90, 0.78)
TextBright    (1.00, 1.00, 1.00)
TextDim       (0.65, 0.55, 0.40)
TextGreen     (0.30, 0.85, 0.30)
TextRed       (0.85, 0.25, 0.25)
SkillActive   (0.28, 0.42, 0.60)   // blue-grey for active skills
SkillPassive  (0.35, 0.50, 0.30)   // green-grey for passive skills
SkillLocked   (0.25, 0.20, 0.15, 0.8)  // dark brown, 80% alpha
SkillLearned  (0.18, 0.35, 0.18)   // dark green
SkillMaxed    (0.50, 0.38, 0.15)   // gold
TabActive     (0.50, 0.38, 0.15)   // gold
TabInactive   (0.28, 0.19, 0.10)   // dark brown
ButtonLearn   (0.22, 0.50, 0.22)   // green
ButtonReset   (0.60, 0.20, 0.15)   // red
```

### 3.4 Server Skill Data (`ro_skill_data.js`)

86+ skills defined across 7 first classes (novice + 6 first jobs) + 12 second classes (in separate file).

Each skill definition:
```javascript
{
    id: 103,
    name: 'bash',
    displayName: 'Bash',
    classId: 'swordsman',
    maxLevel: 10,
    type: 'active',
    targetType: 'single',
    element: 'neutral',
    range: 150,
    description: 'Smash a target for increased damage.',
    icon: 'bash',
    treeRow: 0,
    treeCol: 1,
    prerequisites: [],
    levels: genLevels(10, i => ({
        level: i+1,
        spCost: i < 5 ? 8 : 15,
        castTime: 0,
        afterCastDelay: 0,
        cooldown: 700,
        effectValue: 130 + i*30,
        duration: 0
    }))
}
```

**Key per-level data available but NOT currently sent to client:**
- `spCost` — SP cost at each level
- `castTime` — Cast time in ms at each level
- `cooldown` — Cooldown in ms at each level
- `effectValue` — Damage/effect percentage at each level
- `duration` — Buff/effect duration in ms at each level
- `afterCastDelay` — After-cast delay in ms at each level

### 3.5 Server `skill:data` Event Handler (index.js line 2950-3015)

Current payload per skill:
```javascript
{
    skillId, name, displayName, maxLevel, currentLevel,
    type, targetType, element, range, description, icon,
    treeRow, treeCol, prerequisites,
    spCost,      // current level's SP cost
    nextSpCost,  // next level's SP cost
    castTime,    // current level's cast time
    cooldown,    // current level's cooldown
    effectValue, // current level's effect value
    canLearn     // boolean
}
```

**Missing**: `allLevels` array — the full per-level breakdown

### 3.6 Subsystem Public API (stays unchanged)

| Method | Purpose |
|--------|---------|
| `ToggleWidget()` | Show/hide panel |
| `ShowWidget()` / `HideWidget()` | Add/remove from viewport |
| `RequestSkillData()` | Emit `skill:data` |
| `LearnSkill(skillId)` | Emit `skill:learn` |
| `ResetAllSkills()` | Emit `skill:reset` |
| `AssignSkillToHotbar(skillId, name, slotIndex)` | Emit `hotbar:save_skill` |
| `TryUseHotbarSkill(slotIndex)` | Called by Blueprint (0-based) |
| `UseSkill(skillId)` | Route to targeting or immediate use |
| `UseSkillOnTarget(skillId, targetId, isEnemy)` | Emit `skill:use` |
| `UseSkillOnGround(skillId, groundPos)` | Emit `skill:use` with coords |
| `IsSkillOnCooldown(skillId)` | Check client-side cooldown |
| `GetSkillCooldownRemaining(skillId)` | Time (sec) until ready |
| `FindSkillEntry(skillId)` | Return FSkillEntry* or nullptr |
| `BeginTargeting(skillId)` | Enter targeting mode |
| `CancelTargeting()` | Exit targeting mode |
| `GetOrCreateIconBrush(contentPath)` | Load/cache icon texture+brush |
| `StartSkillDrag(skillId, name, icon)` | Begin drag-to-hotbar |
| `CancelSkillDrag()` | Cancel drag |

### 3.7 Socket Events (stays unchanged)

**Client → Server**: `skill:data`, `skill:learn`, `skill:reset`, `skill:use`, `hotbar:save_skill`
**Server → Client**: `skill:data`, `skill:learned`, `skill:refresh`, `skill:reset_complete`, `skill:error`, `skill:used`, `skill:cooldown_started`, `skill:effect_damage`, `skill:buff_applied`, `skill:buff_removed`, `hotbar:alldata`

### 3.8 Existing Tooltip Patterns (reference implementations)

**`SEquipmentWidget`** uses Slate's built-in `SToolTip` with a `BuildTooltip()` method that returns a custom `SBorder`-based layout. Applied via `.ToolTip(SNew(SToolTip)[BuildTooltip(item)])` on the slot widget.

**`SInventoryWidget`** uses the same pattern: `.ToolTip(SNew(SToolTip)[BuildTooltipContent(item)])`.

This is the established project pattern for tooltips and should be reused.

### 3.9 Existing OnPaint Patterns (reference implementations)

**`SWorldHealthBarOverlay::OnPaint`** uses `FSlateDrawElement::MakeBox` to draw filled rectangles for HP/SP bars. This is the exact technique needed for drawing connecting lines between grid cells.

**`SCastBarOverlay::OnPaint`** similarly uses `MakeBox` for cast progress bars.

Both use `FCoreStyle::Get().GetBrush("GenericWhiteBox")` as the brush and set color via the tint parameter.

### 3.10 Grid Dimensions per Class

From analysis of `ro_skill_data.js` `treeRow`/`treeCol` values:

| Class | Max Row | Max Col | Grid Size | # Skills |
|-------|---------|---------|-----------|----------|
| Novice | 1 | 1 | 2x2 | 3 |
| Swordsman | 2 | 2 | 3x3 | 7 |
| Mage | 2 | 4 | 3x5 | 13 |
| Archer | 2 | 2 | 3x3 | 6 |
| Acolyte | 5 | 4 | 6x5 | 13 |
| Thief | 2 | 2 | 3x3 | 6 |
| Merchant | 2 | 2 | 3x3 | 8 |

Second classes (from `ro_skill_data_2nd.js`): typically 3x3 to 4x4.

**Maximum**: Acolyte at 6 rows x 5 cols — WILL require vertical scrolling within the grid area.

---

## 4. Design Specification

### 4.1 New Window Layout

```
+============================================+ 540px wide
| Title Bar: "Skill Tree"  Points: N    [X]  | dark bg, gold text
+--------------------------------------------+
| [Novice] [Swordsman] [Mage]  ...          | class tabs, gold/dark
+============================================+
|                                            |
|   Grid Area (scrollable vertically)        |
|                                            |
|   [Icon]    [Icon]    [Icon]    ...       | Row 0
|    Name      Name      Name              |
|   Lv X/X    Lv X/X    Lv X/X            |
|      |         |                          | connecting lines
|      v         v                          |
|   [Icon]    [Icon]    [Icon]    ...       | Row 1
|    Name      Name      Name              |
|   Lv X/X    Lv X/X    Lv X/X            |
|                \       /                  | L-shaped lines
|                 v     v                   |
|              [Icon]    [Icon]             | Row 2
|               Name      Name             |
|              Lv X/X    Lv X/X            |
|                                            |
+============================================+
| Remaining Skill Points: N   [Reset All]    | bottom bar
+============================================+ 560px tall
```

### 4.2 Cell Design (78 x 88px)

```
+---------------------------+  78px wide
|                           |
|      [  Icon  ]          |  32x32, centered
|      [ 32x32  ]          |  draggable if learned active/toggle
|                           |
|      Bash                |  name, 7pt bold, centered
|                           |  single line, clipped with "..."
|  Lv 5/10         [+]    |  level text + learn button
|                           |  [+] only if canLearn && !maxed
+---------------------------+  88px tall
```

**Icon display rules:**
- Always visible (requirement: icon must be visible for drag-to-hotbar)
- Draggable only if: `bLearned && Skill.Type != "passive" && IconBrush != nullptr`
- Same drag mechanics as current: `OnMouseButtonDown` → threshold → `StartSkillDrag()`
- Locked skills: icon gets `ColorAndOpacity(0.4, 0.4, 0.4, 0.6)` tint (desaturated)

**Learn button `[+]`:**
- Size: 16x14px
- Background: `ButtonLearn` (green)
- Text: "+" in white, bold 7pt
- Only visible when `bCanLearn && !bMaxed`
- Calls `Sub->LearnSkill(SkillId)` on click

### 4.3 Tooltip Design (SSkillTooltipWidget, ~280px wide)

```
+--------------------------------------------+
| [Icon 24x24]  FIRE BOLT                    |  gold 10pt bold
|               Max Level: 10                |  dim text, 7pt
+--------------------------------------------+  gold divider
| Type: Active Offensive                     |  7pt white
| Element: Fire                              |  color-coded (orange for fire)
| Target: Single Enemy                       |  7pt white
| Range: 9 cells                             |  7pt dim (only if range > 0)
+--------------------------------------------+  gold divider
| Prerequisites:                             |  7pt bold, gold
|   (none)                                   |  or:
|   Cold Bolt Lv 5          MET              |  green text if met
|   Fire Ball Lv 4          NOT MET          |  red text if not met
+--------------------------------------------+  gold divider
| Water bolt magic. +1 bolt per level.       |  7pt dim, wrapped
+--------------------------------------------+  gold divider
| Lv | SP  | Cast  | CD   | Effect          |  6pt bold header
|----|-----|-------|------|---------|         |  gold divider
|  1 |  14 | 0.7s  | 0.0s | 1 bolt |        |  6pt, dim
|  2 |  16 | 1.4s  | 0.0s | 2 bolts|        |
|  3 |  18 | 2.1s  | 0.0s | 3 bolts|        |
| >4 |  20 | 2.8s  | 0.0s | 4 bolts|        |  current level = gold bg row
|  5 |  22 | 3.5s  | 0.0s | 5 bolts|        |  next level = green tint
| ...                                        |
| 10 |  30 | 7.0s  | 0.0s |10 bolts|        |
+--------------------------------------------+
| Duration: N/A                              |  only for buffs/ground effects
+--------------------------------------------+
```

**Dynamic columns in per-level table:**
- Always show: Lv, SP
- Show Cast only if any level has castTime > 0
- Show CD only if any level has cooldown > 0
- Show Effect (effectValue) always
- Show Duration only if any level has duration > 0
- This prevents empty columns for passive skills

**Current level highlighting:**
- Row where `Level == Skill.CurrentLevel` gets gold background tint `(0.50, 0.38, 0.15, 0.3)`
- Row where `Level == Skill.CurrentLevel + 1` gets green tint `(0.22, 0.50, 0.22, 0.15)` (next level preview)

**Prerequisite display logic:**
- For each `FSkillPrerequisite` in the skill's prerequisites array:
  - Look up the prerequisite skill by ID via `Sub->FindSkillEntry(RequiredSkillId)`
  - Get prerequisite's current level from `Sub->LearnedSkills[RequiredSkillId]`
  - If current >= required: display in green with "MET"
  - If current < required: display in red with "NOT MET (Lv X/Y)"
- If no prerequisites: show "(none)" in dim text

### 4.4 Element Color Map

```cpp
static FLinearColor GetElementColor(const FString& Element)
{
    if (Element == TEXT("fire"))    return FLinearColor(1.0f, 0.4f, 0.2f);   // orange
    if (Element == TEXT("water"))   return FLinearColor(0.3f, 0.6f, 1.0f);   // blue
    if (Element == TEXT("wind"))    return FLinearColor(0.5f, 0.9f, 0.3f);   // yellow-green
    if (Element == TEXT("earth"))   return FLinearColor(0.7f, 0.5f, 0.2f);   // brown
    if (Element == TEXT("holy"))    return FLinearColor(1.0f, 1.0f, 0.6f);   // yellow
    if (Element == TEXT("ghost"))   return FLinearColor(0.7f, 0.4f, 0.9f);   // purple
    if (Element == TEXT("poison"))  return FLinearColor(0.5f, 0.8f, 0.2f);   // green
    if (Element == TEXT("undead"))  return FLinearColor(0.5f, 0.2f, 0.5f);   // dark purple
    return SKColors::TextPrimary;  // neutral — default cream text
}
```

### 4.5 Connecting Lines Specification

**Data structures for line drawing:**
```cpp
struct FSkillCellInfo
{
    int32 SkillId = 0;
    int32 Row = 0;
    int32 Col = 0;
    FVector2D CellCenter;     // center relative to grid container
    bool bPrerequisitesMet = false;
};

struct FPrereqLine
{
    int32 FromSkillId;   // prerequisite skill (parent, higher in tree)
    int32 ToSkillId;     // dependent skill (child, lower in tree)
    bool bMet;           // whether the prerequisite level requirement is satisfied
    FVector2D FromCenter; // cached from CellInfos
    FVector2D ToCenter;   // cached from CellInfos
};
```

**Cell center position calculation:**
```cpp
float CenterX = GRID_PADDING + Col * (CELL_WIDTH + CELL_HGAP) + CELL_WIDTH * 0.5f;
float CenterY = Row * (CELL_HEIGHT + CELL_VGAP) + CELL_HEIGHT * 0.5f;
```

**Line drawing algorithm (in OnPaint):**

For each `FPrereqLine`:

1. **Determine line color:**
   ```cpp
   FLinearColor LineColor = Line.bMet
       ? FLinearColor(0.3f, 0.8f, 0.3f, 0.8f)   // green, slightly transparent
       : FLinearColor(0.5f, 0.3f, 0.3f, 0.6f);   // red-grey, more transparent
   ```

2. **Compute exit/entry points:**
   ```cpp
   FVector2D ExitPoint(Line.FromCenter.X, Line.FromCenter.Y + CELL_HEIGHT * 0.5f);  // bottom of parent
   FVector2D EntryPoint(Line.ToCenter.X, Line.ToCenter.Y - CELL_HEIGHT * 0.5f);      // top of child
   ```

3. **Same column** (simple vertical line):
   ```cpp
   if (FMath::IsNearlyEqual(ExitPoint.X, EntryPoint.X, 1.f))
   {
       // Single vertical line
       DrawBox(ExitPoint.X - 1.f, ExitPoint.Y, 2.f, EntryPoint.Y - ExitPoint.Y, LineColor);
   }
   ```

4. **Different column** (L-shaped connector):
   ```cpp
   else
   {
       float MidY = (ExitPoint.Y + EntryPoint.Y) * 0.5f;  // midpoint between rows
       // Vertical down from parent
       DrawBox(ExitPoint.X - 1.f, ExitPoint.Y, 2.f, MidY - ExitPoint.Y, LineColor);
       // Horizontal across
       float MinX = FMath::Min(ExitPoint.X, EntryPoint.X);
       float MaxX = FMath::Max(ExitPoint.X, EntryPoint.X);
       DrawBox(MinX - 1.f, MidY - 1.f, MaxX - MinX + 2.f, 2.f, LineColor);
       // Vertical down to child
       DrawBox(EntryPoint.X - 1.f, MidY, 2.f, EntryPoint.Y - MidY, LineColor);
   }
   ```

5. **DrawBox helper** (wraps `FSlateDrawElement::MakeBox`):
   ```cpp
   void DrawLineBox(FSlateWindowElementList& OutDrawElements, int32 LayerId,
       const FGeometry& Geo, float X, float Y, float W, float H, FLinearColor Color) const
   {
       FSlateDrawElement::MakeBox(
           OutDrawElements, LayerId,
           Geo.ToPaintGeometry(FVector2D(W, H), FSlateLayoutTransform(FVector2f(X, Y))),
           FCoreStyle::Get().GetBrush("GenericWhiteBox"),
           ESlateDrawEffect::None, Color);
   }
   ```

### 4.6 Visual Skill States

| State | Background Color | Icon Tint | Name Color | Level Color | Border |
|-------|-----------------|-----------|-----------|-------------|--------|
| **Locked** | `SkillLocked (0.25,0.20,0.15,0.8)` | `(0.4,0.4,0.4,0.6)` desaturated | `TextDim` | `TextDim` | `GoldDark` at 50% alpha |
| **Learnable (active)** | `SkillActive (0.28,0.42,0.60)` | Full color `(1,1,1,1)` | `TextBright` | `TextPrimary` | `GoldDark` |
| **Learnable (passive)** | `SkillPassive (0.35,0.50,0.30)` | Full color | `TextBright` | `TextPrimary` | `GoldDark` |
| **Learned** | `SkillLearned (0.18,0.35,0.18)` | Full color | `TextBright` | `TextGreen` | `GoldDark` |
| **Maxed** | `SkillMaxed (0.50,0.38,0.15)` | Full color | `GoldHighlight` | `GoldHighlight` | `GoldTrim` |

### 4.7 Grid Constants

```cpp
static constexpr float CELL_WIDTH  = 78.f;   // each skill cell width
static constexpr float CELL_HEIGHT = 88.f;   // each skill cell height
static constexpr float CELL_HGAP   = 14.f;   // horizontal gap between cells
static constexpr float CELL_VGAP   = 24.f;   // vertical gap (where lines are drawn)
static constexpr float GRID_PADDING = 8.f;   // padding around the grid

// Window dimensions
static constexpr float WINDOW_WIDTH  = 540.f;  // was 460
static constexpr float WINDOW_HEIGHT = 560.f;  // was 500
```

**Width calculation verification:**
- 5 cols × 78px = 390px
- 4 gaps × 14px = 56px
- 2 × padding 8px = 16px
- Grid content: 462px
- Panel borders + insets: ~20px
- Scrollbar: ~16px
- **Total needed: ~498px** → 540px window gives comfortable margin

---

## 5. Implementation Phases

### Phase 1: Server — Send Full Levels Array

- [ ] Modify `skill:data` handler in `server/src/index.js` (line ~2983)
- [ ] Add `allLevels` field to each skill entry in the payload
- [ ] No other server changes needed

### Phase 2: Client Data Structures

- [ ] Add `FSkillLevelInfo` struct to `SkillTreeSubsystem.h`
- [ ] Add `AllLevels` field to `FSkillEntry`
- [ ] Update `HandleSkillData()` in `SkillTreeSubsystem.cpp` to parse `allLevels`

### Phase 3: Create SSkillTooltipWidget

- [ ] Create `SSkillTooltipWidget.h` — class declaration
- [ ] Create `SSkillTooltipWidget.cpp` — full tooltip layout:
  - Header (icon + name + max level)
  - Skill info (type, element, target, range)
  - Prerequisites (with met/unmet coloring)
  - Description
  - Per-level table (all levels, current highlighted)
- [ ] Use existing `GetOrCreateIconBrush()` for tooltip icon
- [ ] Use `SKColors` namespace for all colors

### Phase 4: Rewrite SSkillTreeWidget

- [ ] Rewrite `SSkillTreeWidget.h`:
  - Add OnPaint override declaration
  - Add `FSkillCellInfo` and `FPrereqLine` structs
  - Add `CellInfos` and `PrereqLines` arrays
  - Add grid constants
  - Add `GridContainer` shared pointer for geometry reference
  - Update window size constants
- [ ] Rewrite `SSkillTreeWidget.cpp`:
  - `Construct()`: same structure but with wider window
  - `DoRebuildSkillGrid()`: complete rewrite to grid layout
  - `OnPaint()`: new method for drawing connecting lines
  - Cell building: compact 78x88 cells with icon+name+level+learn
  - Tooltip integration via `SToolTip` on each cell
  - Keep all mouse handlers for window drag and skill icon drag
  - Keep deferred rebuild pattern
  - Keep TWeakObjectPtr/TWeakPtr safety

### Phase 5: Testing & Verification

- [ ] Start server, verify `allLevels` in `skill:data` response
- [ ] Launch client, open skill tree, verify grid layout
- [ ] Test each class tab (all grid sizes)
- [ ] Verify prerequisite lines (color, position, L-shapes)
- [ ] Test hover tooltips (all data present, per-level table)
- [ ] Test learn button, drag-to-hotbar, reset all
- [ ] Test zone transitions
- [ ] Visual polish pass

---

## 6. Detailed Code Specifications

### 6.1 Server Change (`server/src/index.js`)

**Location**: Inside the `socket.on('skill:data', ...)` handler, line ~2983-3004

**Current code** (the `push` call):
```javascript
skillTree[skill.classId].push({
    skillId: skill.id,
    name: skill.name,
    displayName: skill.displayName,
    maxLevel: skill.maxLevel,
    currentLevel,
    type: skill.type,
    targetType: skill.targetType,
    element: skill.element,
    range: skill.range,
    description: skill.description,
    icon: skill.icon,
    treeRow: skill.treeRow,
    treeCol: skill.treeCol,
    prerequisites: skill.prerequisites,
    spCost: levelData ? levelData.spCost : 0,
    nextSpCost: nextLevelData ? nextLevelData.spCost : 0,
    castTime: levelData ? levelData.castTime : 0,
    cooldown: levelData ? levelData.cooldown : 0,
    effectValue: levelData ? levelData.effectValue : 0,
    canLearn: canLearnSkill(skill.id, learnedSkills, jobClass, player.skillPoints || 0).ok
});
```

**Add after `effectValue` line and before `canLearn`:**
```javascript
allLevels: skill.levels.map(l => ({
    level: l.level,
    spCost: l.spCost,
    castTime: l.castTime || 0,
    cooldown: l.cooldown || 0,
    effectValue: l.effectValue || 0,
    duration: l.duration || 0,
    afterCastDelay: l.afterCastDelay || 0
})),
```

### 6.2 Client Struct Addition (`SkillTreeSubsystem.h`)

**Add before `FSkillEntry`:**
```cpp
// Per-level data for tooltip display
USTRUCT(BlueprintType)
struct FSkillLevelInfo
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Level = 0;
    UPROPERTY(BlueprintReadOnly) int32 SpCost = 0;
    UPROPERTY(BlueprintReadOnly) int32 CastTime = 0;
    UPROPERTY(BlueprintReadOnly) int32 Cooldown = 0;
    UPROPERTY(BlueprintReadOnly) int32 EffectValue = 0;
    UPROPERTY(BlueprintReadOnly) int32 Duration = 0;
    UPROPERTY(BlueprintReadOnly) int32 AfterCastDelay = 0;
};
```

**Add to `FSkillEntry` (after `Prerequisites`):**
```cpp
UPROPERTY(BlueprintReadOnly)
TArray<FSkillLevelInfo> AllLevels;
```

### 6.3 Client Parsing (`SkillTreeSubsystem.cpp`)

**In `HandleSkillData()`, after parsing `Prerequisites`**, add:
```cpp
// Parse per-level data for tooltip
const TArray<TSharedPtr<FJsonValue>>* LevelsArray = nullptr;
if (S->TryGetArrayField(TEXT("allLevels"), LevelsArray))
{
    for (const TSharedPtr<FJsonValue>& LevelVal : *LevelsArray)
    {
        const TSharedPtr<FJsonObject>* LevelObjPtr = nullptr;
        if (!LevelVal->TryGetObject(LevelObjPtr) || !LevelObjPtr) continue;
        const TSharedPtr<FJsonObject>& L = *LevelObjPtr;
        FSkillLevelInfo Info;
        double T = 0;
        T = 0; L->TryGetNumberField(TEXT("level"), T); Info.Level = (int32)T;
        T = 0; L->TryGetNumberField(TEXT("spCost"), T); Info.SpCost = (int32)T;
        T = 0; L->TryGetNumberField(TEXT("castTime"), T); Info.CastTime = (int32)T;
        T = 0; L->TryGetNumberField(TEXT("cooldown"), T); Info.Cooldown = (int32)T;
        T = 0; L->TryGetNumberField(TEXT("effectValue"), T); Info.EffectValue = (int32)T;
        T = 0; L->TryGetNumberField(TEXT("duration"), T); Info.Duration = (int32)T;
        T = 0; L->TryGetNumberField(TEXT("afterCastDelay"), T); Info.AfterCastDelay = (int32)T;
        Entry.AllLevels.Add(Info);
    }
}
```

### 6.4 SSkillTooltipWidget Header

```cpp
// SSkillTooltipWidget.h — RO Classic-style skill tooltip with per-level breakdown
#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

struct FSkillEntry;
class USkillTreeSubsystem;

class SSkillTooltipWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSkillTooltipWidget) {}
        SLATE_ARGUMENT(const FSkillEntry*, SkillData)
        SLATE_ARGUMENT(USkillTreeSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TSharedRef<SWidget> BuildHeader(const FSkillEntry& Skill, USkillTreeSubsystem* Sub);
    TSharedRef<SWidget> BuildSkillInfo(const FSkillEntry& Skill);
    TSharedRef<SWidget> BuildPrerequisites(const FSkillEntry& Skill, USkillTreeSubsystem* Sub);
    TSharedRef<SWidget> BuildDescription(const FSkillEntry& Skill);
    TSharedRef<SWidget> BuildLevelTable(const FSkillEntry& Skill);

    static FLinearColor GetElementColor(const FString& Element);
    static FString FormatTime(int32 TimeMs);
    static FString GetSkillFormLabel(const FString& Type);
    static FString GetTargetLabel(const FString& TargetType);
};
```

### 6.5 SSkillTooltipWidget Implementation Notes

**`BuildHeader`**: Icon (24x24 via `Sub->GetOrCreateIconBrush()`) + skill name (gold 10pt bold) + "Max Level: N" (dim 7pt)

**`BuildSkillInfo`**: Type line + Element line (color-coded) + Target line + Range line (only if > 0)

**`BuildPrerequisites`**: For each `FSkillPrerequisite`:
- Find prerequisite skill via `Sub->FindSkillEntry(prereq.RequiredSkillId)`
- Get current level via `Sub->LearnedSkills` map (or 0 if not found)
- Green text if met, red text if not met
- Show "(none)" if no prerequisites

**`BuildLevelTable`**:
- Determine which columns to show (check if ANY level has non-zero cast/cooldown/duration)
- Build header row with column labels
- For each level in `AllLevels`:
  - Background tint: gold if `Level == CurrentLevel`, green if `Level == CurrentLevel + 1`, else transparent
  - Format cast time as seconds: `FormatTime(castTime)` → "0.7s" or "0.0s"
  - Format cooldown similarly
  - EffectValue displayed as-is (could be %, flat value, # bolts — depends on skill)
  - Duration formatted as seconds: "3.0s", "30.0s", or "-" if 0
- If `AllLevels` is empty (server didn't send): show "No detailed level data available"

**`FormatTime` helper:**
```cpp
static FString FormatTime(int32 TimeMs)
{
    if (TimeMs <= 0) return TEXT("0.0s");
    float Seconds = TimeMs / 1000.f;
    return FString::Printf(TEXT("%.1fs"), Seconds);
}
```

**`GetSkillFormLabel` helper:**
```cpp
static FString GetSkillFormLabel(const FString& Type)
{
    if (Type == TEXT("active")) return TEXT("Active");
    if (Type == TEXT("passive")) return TEXT("Passive");
    if (Type == TEXT("toggle")) return TEXT("Toggle");
    return Type;
}
```

### 6.6 SSkillTreeWidget Header Changes

Key additions to the header (everything else stays):

```cpp
// OnPaint for drawing prerequisite lines
virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

// Grid layout tracking
struct FSkillCellInfo
{
    int32 SkillId = 0;
    int32 Row = 0;
    int32 Col = 0;
    FVector2D CellCenter;
    bool bPrerequisitesMet = false;
};
TArray<FSkillCellInfo> CellInfos;

struct FPrereqLine
{
    int32 FromSkillId;
    int32 ToSkillId;
    bool bMet;
    FVector2D FromCenter;
    FVector2D ToCenter;
};
TArray<FPrereqLine> PrereqLines;

// Grid constants
static constexpr float CELL_WIDTH   = 78.f;
static constexpr float CELL_HEIGHT  = 88.f;
static constexpr float CELL_HGAP    = 14.f;
static constexpr float CELL_VGAP    = 24.f;
static constexpr float GRID_PADDING = 8.f;

// Window size (wider)
FVector2D CurrentSize = FVector2D(540.0, 560.0);

// Grid container pointer for OnPaint geometry reference
TSharedPtr<SWidget> GridArea;
```

### 6.7 DoRebuildSkillGrid Algorithm (Pseudocode)

```
1. Clear SkillContentBox
2. Get active class group's skills
3. Determine maxRow and maxCol
4. Clear CellInfos and PrereqLines arrays

5. Create 2D skill lookup: TMap<int32, TMap<int32, const FSkillEntry*>> SkillGrid
   For each skill: SkillGrid[skill.TreeRow][skill.TreeCol] = &skill

6. Create GridVBox = SNew(SVerticalBox)

7. For row = 0..maxRow:
   a. Create RowHBox = SNew(SHorizontalBox)
   b. For col = 0..maxCol:
      i. Compute CellCenter:
         X = GRID_PADDING + col * (CELL_WIDTH + CELL_HGAP) + CELL_WIDTH * 0.5f
         Y = row * (CELL_HEIGHT + CELL_VGAP) + CELL_HEIGHT * 0.5f
      ii. If skill exists at [row][col]:
          - Build compact cell widget (icon + name + level + learn button)
          - Attach SToolTip with SSkillTooltipWidget
          - Store FSkillCellInfo { SkillId, Row, Col, CellCenter, bPrereqsMet }
          - For each prerequisite of this skill:
            - Find prerequisite cell center
            - Check if prerequisite met (LearnedSkills[prereqId] >= requiredLevel)
            - Store FPrereqLine { FromSkillId, ToSkillId, bMet, FromCenter, ToCenter }
      iii. If no skill at [row][col]:
           - Add empty SBox with WidthOverride(CELL_WIDTH), HeightOverride(CELL_HEIGHT)
      iv. Add cell to RowHBox with Padding(CELL_HGAP/2, 0)
   c. Add RowHBox to GridVBox with Padding(0, CELL_VGAP/2)

8. Wrap GridVBox in SScrollBox (vertical)
9. Store GridArea = reference to the grid container for OnPaint

10. Invalidate(EInvalidateWidgetReason::Layout)
```

### 6.8 OnPaint Implementation (Pseudocode)

```cpp
int32 SSkillTreeWidget::OnPaint(...) const
{
    // First, paint all children (the grid cells)
    int32 MaxLayerId = SCompoundWidget::OnPaint(...);

    // Then draw prerequisite lines BEHIND the cells
    // We use LayerId (base) which is below MaxLayerId (cells)
    // Actually we need lines BEHIND cells, so draw at LayerId
    // and cells paint at higher layers naturally

    // Problem: OnPaint draws AFTER children. Lines would be ON TOP.
    // Solution: Use a separate paint widget UNDER the grid for lines.
    // OR: Accept lines on top (they're thin 2px, not visually intrusive)
    // OR: Use a custom SPanel that paints its background before children

    // Simplest correct approach: draw lines at LayerId (same as background)
    // The cell SBorder widgets paint at LayerId+1 and above, so lines appear behind cells.

    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

    for (const FPrereqLine& Line : PrereqLines)
    {
        FLinearColor LineColor = Line.bMet
            ? FLinearColor(0.3f, 0.8f, 0.3f, 0.8f)
            : FLinearColor(0.5f, 0.3f, 0.3f, 0.6f);

        // Exit point: bottom center of parent cell
        float FromX = Line.FromCenter.X;
        float FromY = Line.FromCenter.Y + CELL_HEIGHT * 0.5f;

        // Entry point: top center of child cell
        float ToX = Line.ToCenter.X;
        float ToY = Line.ToCenter.Y - CELL_HEIGHT * 0.5f;

        float LineW = 2.f;

        if (FMath::IsNearlyEqual(FromX, ToX, 2.f))
        {
            // Same column — simple vertical line
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(LineW, ToY - FromY),
                    FSlateLayoutTransform(FVector2f(FromX - LineW*0.5f, FromY))
                ),
                WhiteBrush, ESlateDrawEffect::None, LineColor);
        }
        else
        {
            // Different column — L-shaped connector
            float MidY = (FromY + ToY) * 0.5f;
            float MinX = FMath::Min(FromX, ToX);
            float MaxX = FMath::Max(FromX, ToX);

            // Vertical down from parent
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(LineW, MidY - FromY),
                    FSlateLayoutTransform(FVector2f(FromX - LineW*0.5f, FromY))
                ),
                WhiteBrush, ESlateDrawEffect::None, LineColor);

            // Horizontal across
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(MaxX - MinX + LineW, LineW),
                    FSlateLayoutTransform(FVector2f(MinX - LineW*0.5f, MidY - LineW*0.5f))
                ),
                WhiteBrush, ESlateDrawEffect::None, LineColor);

            // Vertical down to child
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(LineW, ToY - MidY),
                    FSlateLayoutTransform(FVector2f(ToX - LineW*0.5f, MidY))
                ),
                WhiteBrush, ESlateDrawEffect::None, LineColor);
        }
    }

    return MaxLayerId;
}
```

**Important note on line z-ordering**: In Slate, `OnPaint` is called for each widget, and `SCompoundWidget::OnPaint` draws children. If we draw lines in `OnPaint` AFTER calling the parent implementation, lines will be ON TOP of cells. This is actually fine — the lines are thin (2px) and semi-transparent, so they look natural overlaying cell borders. If we want lines BEHIND cells, we need either:
1. A custom `SPanel` subclass that paints background before children (more complex)
2. A separate overlay widget under the grid (adds complexity)
3. Accept lines on top (simplest, visually acceptable)

**Recommendation**: Lines on top with semi-transparency looks good and is the simplest approach. The green/red 2px lines overlaying the gold cell borders create a nice visual effect.

### 6.9 OnPaint Coordinate System

**Critical detail**: The `CellCenter` positions stored in `CellInfos` are relative to the grid container widget. But `OnPaint` receives `AllottedGeometry` for the SSkillTreeWidget (which includes title bar, tabs, etc.). The line positions need to be offset by the grid container's position within the overall widget.

**Solution**: The grid container is inside a `SScrollBox` which is inside the main `SVerticalBox`. We need to compute the offset from the widget root to the grid area. The simplest approach:

1. Store the grid content as a custom `SCompoundWidget` subclass (let's call it `SSkillGridPanel`) that has its own `OnPaint`
2. This panel is what gets placed inside the `SScrollBox`
3. Its `OnPaint` draws lines relative to its own geometry (no offset calculation needed)
4. The cells are children of this panel

This is cleaner than trying to calculate offsets through the scroll box, title bar, and tabs.

**Revised architecture**: Rather than putting `OnPaint` on `SSkillTreeWidget` (the outer window), create an inner `SSkillGridPanel` class (can be defined in the .cpp file, doesn't need its own header) that handles the grid layout and line drawing.

```cpp
// In SSkillTreeWidget.cpp (file-local class)
class SSkillGridPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSkillGridPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void RebuildGrid(USkillTreeSubsystem* Sub, const FSkillClassGroup& Group);

    virtual int32 OnPaint(...) const override;

    TArray<FPrereqLine> PrereqLines;
    // ... cell info arrays
};
```

The `SSkillTreeWidget` creates and holds a `TSharedPtr<SSkillGridPanel>` which sits inside the scroll box. `DoRebuildSkillGrid` calls `GridPanel->RebuildGrid(Sub, Group)`.

---

## 7. File Change Summary

| File | Action | Est. Lines | Description |
|------|--------|-----------|-------------|
| `server/src/index.js` | MODIFY | +8 | Add `allLevels` array to `skill:data` payload |
| `client/.../UI/SkillTreeSubsystem.h` | MODIFY | +15 | Add `FSkillLevelInfo` struct, `AllLevels` field on `FSkillEntry` |
| `client/.../UI/SkillTreeSubsystem.cpp` | MODIFY | +15 | Parse `allLevels` JSON array in `HandleSkillData` |
| `client/.../UI/SSkillTreeWidget.h` | REWRITE | ~100 | Grid panel ref, updated window size, removed old SWrapBox refs |
| `client/.../UI/SSkillTreeWidget.cpp` | REWRITE | ~750 | Grid layout, inner SSkillGridPanel with OnPaint lines, compact cells, tooltip integration |
| `client/.../UI/SSkillTooltipWidget.h` | CREATE | ~35 | Tooltip widget class declaration |
| `client/.../UI/SSkillTooltipWidget.cpp` | CREATE | ~280 | Header, info, prerequisites, description, per-level table |

**Total estimated**: ~1,200 lines of code changes

---

## 8. Design Decisions & Rationale

### 8.1 SToolTip with custom content (vs manual popup)
**Decision**: Use Slate's built-in `SToolTip` system with `SSkillTooltipWidget` as content.
**Rationale**: Matches existing pattern in `SEquipmentWidget` and `SInventoryWidget`. Handles positioning, delay, hit testing, z-ordering automatically. No need to manage viewport overlays, mouse tracking, or show/hide timing manually.

### 8.2 OnPaint for lines (vs separate overlay widget)
**Decision**: Use `OnPaint` override on an inner `SSkillGridPanel` widget.
**Rationale**: Gives precise z-order control. Lines draw relative to the grid's own geometry (no offset calculations through scroll box/title bar). Matches established pattern from `SWorldHealthBarOverlay` and `SCastBarOverlay`. Grid panel is a file-local class (no extra header needed).

### 8.3 Nested SVerticalBox/SHorizontalBox (vs SGridPanel)
**Decision**: Manual grid with nested VBox/HBox.
**Rationale**: `SGridPanel` exists in UE5 but has limited cell sizing control and doesn't support empty cells elegantly. Manual nesting matches the project's coding style and makes position calculation straightforward for OnPaint. Each cell is a simple `SBox` with fixed size overrides.

### 8.4 Separate tooltip file (vs inline in SSkillTreeWidget.cpp)
**Decision**: `SSkillTooltipWidget` in its own .h/.cpp files.
**Rationale**: The tooltip has ~280 lines of layout code (header, info, prerequisites, description, per-level table). Keeping it inline would make `SSkillTreeWidget.cpp` 1000+ lines. Separate files are cleaner, more maintainable, and allow the tooltip to be reused elsewhere (e.g., hotbar hover in the future).

### 8.5 78px cell width
**Decision**: Each grid cell is 78x88px.
**Rationale**: Fits 5 columns within 540px window with gaps and padding. Wide enough for a 32x32 icon + truncated name + level text + micro learn button. Compact enough to show the full tree structure at a glance without excessive scrolling.

### 8.6 No staged allocation (vs RO's Apply/Reset model)
**Decision**: Keep current immediate-learn model (click Learn → server commits).
**Rationale**: We already have "Reset All" for undoing. Staged allocation adds UI complexity (Apply/Reset buttons, tentative state tracking, uncommitted changes visualization). Can be added later if players request it.

### 8.7 Remove hotbar quick-assign [1]-[9] from cells
**Decision**: Remove the per-cell hotbar buttons (previously showed [1]-[9] for each learned active skill).
**Rationale**: Takes up significant vertical space in each cell. Players can drag-to-hotbar instead (the primary interaction). The hotbar UI itself should handle slot assignment. Simplifies cell layout to fit within 88px height.

### 8.8 No pulse animation initially
**Decision**: Skip "learnable skill pulse" animation for initial implementation.
**Rationale**: Implementing pulsing in pure Slate requires timer-driven alpha modulation via `BorderBackgroundColor_Lambda` reading `FPlatformTime::Seconds()`. Adds complexity. The bright vs dimmed color difference is already enough visual distinction. Can be added as a polish pass later.

---

## 9. Verification Checklist

### Server
- [ ] `skill:data` response includes `allLevels` array for each skill
- [ ] Each level entry has: level, spCost, castTime, cooldown, effectValue, duration, afterCastDelay
- [ ] Existing fields (spCost, nextSpCost, castTime, cooldown, effectValue) still present for backward compat
- [ ] No server crashes or performance issues

### Grid Layout
- [ ] Novice tab: 2x2 grid, 3 skills positioned correctly
- [ ] Swordsman tab: 3x3 grid, 7 skills positioned correctly
- [ ] Mage tab: 3x5 grid, 13 skills positioned correctly
- [ ] Acolyte tab: 6x5 grid, 13 skills, scrolls vertically
- [ ] Empty cells: blank spaces where no skill exists (no shifted/missing cells)
- [ ] Tab switching rebuilds grid correctly for each class

### Connecting Lines
- [ ] Lines connect prerequisite to dependent skill visually
- [ ] Same-column prerequisites: straight vertical line
- [ ] Different-column prerequisites: L-shaped connector
- [ ] Green lines for met prerequisites
- [ ] Red-grey lines for unmet prerequisites
- [ ] Multi-prerequisite skills show multiple incoming lines:
  - Fire Wall (needs Fire Ball Lv 5 + Sight Lv 1) — 2 lines
  - Safety Wall (needs Napalm Beat Lv 7 + Soul Strike Lv 5) — 2 lines
  - Endure (needs Provoke Lv 5) — 1 line
  - Magnum Break (needs Bash Lv 5) — 1 line
- [ ] Lines update color when skill is learned (changes from red to green)

### Hover Tooltips
- [ ] Tooltip appears on hover over any skill cell
- [ ] Tooltip shows: icon, name, max level
- [ ] Tooltip shows: type (Active/Passive/Toggle), element (color-coded), target, range
- [ ] Tooltip shows: prerequisites with met/unmet status
- [ ] Tooltip shows: description text
- [ ] Tooltip shows: per-level table with ALL levels
- [ ] Current level row highlighted in gold
- [ ] Next level row has subtle green tint
- [ ] Dynamic columns: cast time hidden for skills with 0 cast time
- [ ] Duration column shown only for buff/ground skills
- [ ] Tooltip handles empty `AllLevels` gracefully (shows fallback text)

### Skill States
- [ ] Locked skills: dimmed/greyed icon, dark brown background
- [ ] Learnable active skills: blue-grey background, bright icon
- [ ] Learnable passive skills: green-grey background, bright icon
- [ ] Learned skills: dark green background, green level text
- [ ] Maxed skills: gold background, gold text

### Interactions
- [ ] Learn button `[+]` works for learnable non-maxed skills
- [ ] After learning: grid updates, lines update, tooltip updates
- [ ] Drag icon to hotbar: still works for learned active/toggle skills
- [ ] Window drag by title bar: still works
- [ ] Tab switching: all class tabs work
- [ ] Reset All button: resets skills, entire grid updates
- [ ] X close button: hides widget
- [ ] K key toggle: opens/closes skill tree

### Edge Cases
- [ ] Skill with no prerequisites (e.g., Bash): no incoming lines, not locked
- [ ] Skill at max level: gold state, no learn button, tooltip shows all levels
- [ ] Passive skill: no drag-to-hotbar, no hotbar assign, type shows "Passive"
- [ ] Toggle skill: draggable to hotbar, type shows "Toggle"
- [ ] First-time open (no data yet): shows "Loading skill tree..."
- [ ] Zone transition: widget survives, data re-fetches correctly
- [ ] Novice class (tiny tree): grid centers nicely, not stretched

### Performance
- [ ] No frame drops with 13 skills + lines + tooltip visible
- [ ] No GC issues (icon textures GC-rooted via UPROPERTY TMap)
- [ ] No crashes on rapid tab switching
- [ ] SToolTip doesn't cause memory leaks
