# BP_MMOCharacter

## Overview

`BP_MMOCharacter` is the local player-controlled character in the MMO. It handles player input, camera control, and network position synchronization.

---

## File Location

```
Content/Blueprints/BP_MMOCharacter.uasset
```

---

## Parent Class

`Character` (from Engine)

---

## Purpose

- Local player avatar controlled by the player
- Sends position updates to server at 30Hz
- Displays player name tag above head
- Handles all player input (movement, combat)

---

## Components

| Component | Type | Purpose |
|-----------|------|---------|
| `CapsuleComponent` | CapsuleComponent | Collision and movement base |
| `SkeletalMesh` | SkeletalMeshComponent | Visual character mesh |
| `SpringArm` | SpringArmComponent | Camera boom for top-down view |
| `Camera` | CameraComponent | Player view camera |
| `CharacterMovement` | CharacterMovementComponent | Movement physics |
| `NameTagWidget` | WidgetComponent | Player name display above head |

---

## Variables

| Variable | Type | Category | Purpose |
|----------|------|----------|---------|
| `PlayerName` | String | Name Tag | Character name from GameInstance |

---

## Name Tag Widget Setup

### Component Configuration

**Details Panel - User Interface:**
| Property | Value |
|----------|-------|
| Widget Class | `WBP_PlayerNameTag` |
| Space | Screen |
| Draw at Desired Size | ✓ Checked |
| Draw Size | X=200, Y=50 |

**Details Panel - Transform:**
| Property | Value | Description |
|----------|-------|-------------|
| Location X | 0 | Centered on character |
| Location Y | 0 | Centered on character |
| Location Z | 250 | Above character head |
| Rotation | 0, 0, 0 | Default |
| Scale | 1, 1, 1 | Default size |

---

## Event Graph

### Event BeginPlay - Set Name Tag

```
Event BeginPlay
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Get SelectedCharacter → Break FCharacterData
    ↓
Get Name → Set PlayerName (variable)
    ↓
Delay (0.1 seconds)  ← Wait for variable replication
    ↓
Get NameTagWidget (component) → Get User Widget Object
    ↓
Cast to WBP_PlayerNameTag
    ↓
Set Player Name (function)
    ↓
PlayerName variable → NewName input
```

**Purpose**: Retrieves character name from GameInstance and sets it on the name tag widget.

---

## Input Mapping

**Enhanced Input (Default):**
- **W/A/S/D** - Movement (forward/left/backward/right)
- **Left Click** - Interact / Attack
- **Shift** - Sprint (optional)
- **Mouse** - Camera rotation (if enabled)

---

## Network Position Sync

**Position updates sent via BP_SocketManager:**
- Event Tick checks `UpdateInterval` (0.033s = 30Hz)
- When interval elapsed: Get Actor Location → Emit `player:position`
- Format: `{characterId, x, y, z}`

**Not handled in BP_MMOCharacter directly** - BP_SocketManager manages all Socket.io communication.

---

## Comparison with BP_OtherPlayerCharacter

| Feature | BP_MMOCharacter (Local) | BP_OtherPlayerCharacter (Remote) |
|---------|------------------------|----------------------------------|
| Input | Yes (WASD / Click) | No (server-driven) |
| Camera | Yes (SpringArm) | No |
| Position Source | Player input | Server broadcasts |
| Network | Sends position (30Hz) | Receives position |
| Interpolation | No (direct control) | Yes (smooth follow) |
| Name Tag | Yes (WBP_PlayerNameTag) | Yes (WBP_PlayerNameTag) |

---

## Related Files

| File | Purpose |
|------|---------|
| `BP_OtherPlayerCharacter` | Remote player representation |
| `BP_SocketManager` | Handles position emit/receive |
| `WBP_PlayerNameTag` | Name display widget |
| `MMOGameInstance` | Stores SelectedCharacter data |
| `ABP_unarmed` | Animation Blueprint (shared) |

---

## Troubleshooting

### Issue: Name tag not visible

**Check:**
1. NameTagWidget component exists
2. Widget Class is WBP_PlayerNameTag
3. Space is set to Screen
4. Delay in BeginPlay is long enough (try 0.2s)

### Issue: Name shows "Unknown" or empty

**Check:**
1. PlayerName variable set correctly
2. GameInstance.SelectedCharacter.Name has value
3. Cast to MMOGameInstance succeeds

### Issue: Position not syncing to server

**Check:**
1. BP_SocketManager exists in level
2. IsConnected is true
3. CharacterId is set from GameInstance

---

## Performance Notes

- Local player uses full CharacterMovement physics
- No interpolation (direct player control)
- Camera attached via SpringArm (smooth follow optional)

---

**Last Updated**: 2026-02-04
**Version**: 1.0
**Status**: Complete with Name Tags
