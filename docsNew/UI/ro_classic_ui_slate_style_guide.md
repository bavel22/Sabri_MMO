# Ragnarok Classic UI Style Specification
**Target Engine:** Unreal Engine 5  
**Framework:** Slate (C++)  
**Visual Era:** Pre-Renewal Classic  
**Theme:** Brown & Gold Ornamental Fantasy UI

---

# 1. GLOBAL STYLE TOKENS

## 1.1 Color Palette

```yaml
Colors:
  PanelBrownBase:        "#6E4B2A"
  PanelBrownDark:        "#4A2F1A"
  PanelBrownLight:       "#8A623C"
  GoldTrim:              "#C9A24E"
  GoldDark:              "#8F6A2A"
  GoldHighlight:         "#F3D27A"
  HPRedBase:             "#9E1C1C"
  HPRedHighlight:        "#D93A3A"
  SPBlueBase:            "#1C3F9E"
  SPBlueHighlight:       "#3A6ED9"
  EXPYellowBase:         "#C9A300"
  EXPYellowHighlight:    "#F2D23A"
  TextPrimary:           "#F5E6C8"
  TextShadow:            "#000000"
```

---

# 2. PANEL CONSTRUCTION

## 2.1 Panel Rules

```yaml
Panel:
  Background:
    Type: Textured
    BaseColor: PanelBrownBase
    Gradient:
      Direction: Vertical
      TopMultiplier: 0.9
      BottomMultiplier: 1.1
    NoiseOverlay: Subtle

  Border:
    Thickness: 10px
    OuterColor: GoldTrim
    InnerInsetColor: PanelBrownDark
    Bevel: Enabled
    CornerDecoration: OrnamentalGoldPlate

  Opacity: 100%
  TransparencyAllowed: false
  BlurAllowed: false
```

## 2.2 9-Slice Scaling

```yaml
Scaling:
  Method: 9-Slice
  BorderPreserve: true
  CornerPreserve: true
  CenterStretch: true
```

---

# 3. TYPOGRAPHY

```yaml
Font:
  Style: FantasySerif or PixelReadable
  Weight: MediumBold
  Outline:
    Enabled: true
    Color: TextShadow
    Thickness: 1px
  PrimaryColor: TextPrimary
  LetterSpacing: 0
  AntiAliasing: On
```

Rules:
- No modern sans-serif fonts
- No ultra-thin fonts
- Numbers must be highly readable

---

# 4. HP / SP / EXP BARS

## 4.1 Bar Container

```yaml
BarContainer:
  Shape: CapsuleRounded
  BorderColor: Black
  BorderThickness: 2px
  BackgroundColor: PanelBrownDark
```

## 4.2 Fill Style

```yaml
BarFill:
  Gradient: Horizontal
  GlossHighlight:
    Enabled: true
    Position: Top25Percent
    Opacity: 0.35
  ShineStyle: Soft
```

## 4.3 Specific Bars

```yaml
HPBar:
  BaseColor: HPRedBase
  HighlightColor: HPRedHighlight

SPBar:
  BaseColor: SPBlueBase
  HighlightColor: SPBlueHighlight

EXPBar:
  BaseColor: EXPYellowBase
  HighlightColor: EXPYellowHighlight
```

## 4.4 Value Text Overlay

```yaml
BarText:
  Alignment: Center
  Format: "Current / Max"
  Color: TextPrimary
  Shadow: Enabled
```

---

# 5. STATUS WINDOW LAYOUT

```yaml
StatusWindow:
  Layout:
    Orientation: Vertical
    Padding: 8px
    SectionSpacing: 6px

  Header:
    CharacterName: Centered
    JobClass: RightAligned
    LevelDisplay: RightAligned

  Portrait:
    Position: TopLeft
    Size: 96x96
    Frame: GoldTrim

  StatList:
    Columns: 2
    Alignment: Tight
    RowSpacing: 2px
    Format: "STAT: Base + Bonus"
```

### Stat Order

```yaml
LeftColumn:
  - STR
  - AGI
  - VIT
  - ATK

RightColumn:
  - DEX
  - INT
  - LUK
  - DEF
  - HIT
  - FLEE
  - CRIT
```

---

# 6. BUTTONS

```yaml
Button:
  Shape: RoundedRectangle
  BorderColor: GoldTrim
  BackgroundColor: PanelBrownBase
  Gradient: Vertical
  GlossHighlight: Top
  Padding: "4px 8px"

  States:
    Idle:
      Brightness: 1.0
    Hover:
      Brightness: 1.15
      Glow: SoftGoldOuter
    Pressed:
      InsetShadow: true
      DepthShift: -2px
```

Rules:
- No neon colors
- No flat modern styling

---

# 7. ICON RULES

```yaml
IconStyle:
  Outline: "1-2px Dark"
  Saturation: High
  Shading: BlockStyle
  DetailLevel: Medium
  ReadabilityPriority: High
  MinimumSize: "32x32"
  PreferredSize: "48x48"
```

Icons must:
- Be slightly exaggerated
- Avoid thin detail
- Maintain strong contrast

---

# 8. SPACING RULES

```yaml
Spacing:
  OuterPadding: 8px
  InnerPadding: 4px
  GridGap: 2px
  SectionGap: 6px
```

Design principle:
- Compact
- Dense
- Minimal whitespace
- No modern airy layout

---

# 9. ANIMATION RULES

```yaml
Animation:
  Allowed:
    - HoverGlow
    - ButtonPressDepth
  Disallowed:
    - SlidingPanels
    - FadeInOutHeavy
    - Bounce
    - BlurTransitions
    - ModernElasticMotion
```

UI should feel mostly static.

---

# 10. SLATE IMPLEMENTATION NOTES (UE5)

## 10.1 Recommended Slate Widgets
- `SBorder` for framed panels
- `SOverlay` for layered bar fill
- `SProgressBar` with custom style
- `FSlateBrush` for 9-slice textures
- `FButtonStyle` for button state handling

## 10.2 Text Implementation
- Use `FSlateFontInfo`
- Apply shadow manually if needed

## 10.3 DPI & Texture Handling
- Export UI textures at 2x resolution
- Disable texture filtering blur
- Use UI texture group

### Icon / UI Texture Import Settings (MANDATORY)

All imported icon and UI textures MUST have these settings to avoid blurriness:

| Setting | Value |
|---------|-------|
| **Compression Settings** | `BC7 Compressed (BC7)` |
| **Mip Gen Settings** | `NoMipmaps` |
| **Texture Group** | `UI` |
| **Never Stream** | `true` (checked) |

**Bulk setup**: Select all textures → Right-click → Asset Actions → Bulk Edit via Property Matrix → set all 4 → Save All.

See also: `/sabrimmo-generate-icons` skill for full icon generation + import workflow.

---

# VISUAL IDENTITY SUMMARY

```yaml
StyleIdentity:
  Era: Early 2000s Korean MMORPG
  Complexity: Medium
  Density: High
  Ornamentation: Moderate
  Transparency: None
  ModernFlatUI: Prohibited
  Minimalism: Prohibited
  ColorTheme: BrownGoldDominant
```

