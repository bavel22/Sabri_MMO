# WBP_PlayerNameTag Widget

## Overview

`WBP_PlayerNameTag` is a User Widget Blueprint that displays a player's character name above their head in the game world. It is used by `BP_OtherPlayerCharacter` to show the names of remote players.

---

## File Location

```
Content/Blueprints/Widgets/WBP_PlayerNameTag.uasset
```

---

## Widget Structure

### Designer Layout

```
CanvasPanel (Root)
└── TextBlock (Player Name Text)
    - Anchors: Center
    - Alignment: (0.5, 0.5)
    - Color: White (or visible color)
    - Font Size: 16-20
    - Shadow: Enabled (for readability)
```

### Recommended Settings

| Property | Value | Description |
|----------|-------|-------------|
| **Canvas Panel** | | |
| Position X/Y | 0, 0 | Centered in widget |
| Size X/Y | 200, 50 | Widget dimensions |
| **Text Block** | | |
| Text | (empty) | Set dynamically |
| Justification | Center | Center-aligned text |
| Color | White (FFFFFF) | High visibility |
| Shadow Offset | X=1, Y=1 | Readability shadow |
| Shadow Color | Black | Contrast shadow |

---

## Function: SetPlayerName

### Purpose

Updates the displayed player name text dynamically.

### Function Definition

| Property | Value |
|----------|-------|
| **Function Name** | `SetPlayerName` |
| **Access** | Public |
| **Pure** | No |
| **Const** | No |

### Input Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| `NewName` | Text | The player name to display |

### Implementation

```
Function: SetPlayerName
    Input: NewName (Text)
    ↓
Get TextBlock (reference to Text Block in Designer)
    ↓
Set Text
    Target: TextBlock
    In Text: NewName
```

---

## Usage in BP_OtherPlayerCharacter

### Widget Component Setup

**In BP_OtherPlayerCharacter Blueprint:**

1. **Add Component**: Widget
2. **Name**: `NameTagWidget`
3. **Details Settings**:

| Property | Value |
|----------|-------|
| Widget Class | `WBP_PlayerNameTag` |
| Space | Screen |
| Draw at Desired Size | ✓ Checked |
| Draw Size | X=200, Y=50 |
| Pivot | X=0.5, Y=1.0 |

### Transform Settings

| Property | Value | Description |
|----------|-------|-------------|
| Location X/Y/Z | 0, 0, 250 | Above character head |
| Rotation | 0, 0, 0 | Default |
| Scale | 1, 1, 1 | Default size |

**Note**: Z=250 places the name tag above the character's head. Adjust based on character height.

### Setting the Name

**Event BeginPlay Flow:**

```
Event BeginPlay
    ↓
Delay (0.1 seconds)  ← Wait for PlayerName variable to be set
    ↓
Get User Widget Object (from NameTagWidget component)
    ↓
Cast To WBP_PlayerNameTag
    ↓
Set Player Name (function call)
    ↓
PlayerName variable (from BP_OtherPlayerCharacter) → NewName input
```

**Blueprint Nodes:**

1. **Delay**: Prevents race condition where BeginPlay runs before PlayerName is set
2. **Get User Widget Object**: Gets the widget instance from the component
3. **Cast To WBP_PlayerNameTag**: Ensures we have the correct widget type
4. **Set Player Name**: Calls the custom function to update the text

---

## Data Flow

### Setting Name from Spawn

```
BP_OtherPlayerManager.SpawnOrUpdatePlayer
    ↓
Spawn Actor from Class (BP_OtherPlayerCharacter)
    ↓
Set PlayerName variable = playerName input parameter
    ↓
Add to OtherPlayers map
    ↓
Event BeginPlay fires on spawned actor
    ↓
Delay 0.1s
    ↓
Get Widget → Cast → SetPlayerName(PlayerName)
    ↓
Widget displays player name above character
```

---

## Why Screen Space?

| Space Type | Behavior | Use Case |
|------------|----------|----------|
| **World** | Rotates with world, can be obscured | 3D elements attached to objects |
| **Screen** | Always faces camera, always visible | UI elements, name tags |

**Screen space is used because:**
- Name tag always faces the player camera
- Never obscured by world geometry
- Stays readable from any angle
- Automatically scales with distance

---

## Troubleshooting

### Issue: Name tag not visible

**Check:**
1. Widget Class is set to WBP_PlayerNameTag
2. Space is set to Screen
3. Text Block text is empty (not hardcoded)
4. Text color contrasts with background

### Issue: Name shows "Unknown" or empty

**Check:**
1. PlayerName variable is set in SpawnOrUpdatePlayer
2. Delay in BeginPlay is long enough (try 0.2s)
3. characterName is sent from server
4. OnPlayerMoved parses the name correctly

### Issue: Name tag too small/big

**Fix:**
- Adjust Scale on NameTagWidget component
- Or adjust Draw Size property

### Issue: Name tag in wrong position

**Fix:**
- Adjust Z location (higher = above head, lower = closer to head)
- Check Pivot settings (0.5, 1.0 = bottom center)

---

## Related Files

| File | Purpose |
|------|---------|
| `BP_OtherPlayerCharacter` | Uses WBP_PlayerNameTag |
| `BP_OtherPlayerManager` | Sets PlayerName when spawning |
| `BP_SocketManager` | Parses characterName from JSON |
| `server/src/index.js` | Sends characterName in events |

---

## Best Practices

1. **Use Screen Space** for name tags (not World Space)
2. **Always use a Delay** in BeginPlay before setting widget text
3. **Keep text color high contrast** (white with black shadow)
4. **Center-align text** for consistent positioning
5. **Test with long names** to ensure widget size is sufficient
6. **Shadow improves readability** against varying backgrounds

---

**Last Updated**: 2026-02-04
**Version**: 1.0
**Status**: Complete
