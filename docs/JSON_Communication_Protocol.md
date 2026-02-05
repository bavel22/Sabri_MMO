# JSON Communication Protocol

## Overview

JSON is used for all communication between UE5 client and Node.js server via Socket.io.

---

## Client → Server Events

### player:join

```json
{
  "characterId": "1",
  "token": "jwt_token_here",
  "characterName": "PlayerName"
}
```

**Sent**: Once when player enters world
**From**: BP_SocketManager.OnSocketConnected

### player:position

```json
{
  "characterId": "1",
  "x": "100.5",
  "y": "200.3",
  "z": "50.0"
}
```

**Sent**: 30Hz (33ms interval)
**From**: BP_SocketManager.Event Tick

---

## Server → Client Events

### player:moved

```json
{
  "characterId": "2",
  "characterName": "OtherPlayer",
  "x": "150.0",
  "y": "250.0",
  "z": "50.0",
  "timestamp": 1707081600000
}
```

**Received**: When other players move
**Handled**: BP_SocketManager.OnPlayerMoved

### player:left

```json
{
  "characterId": "2",
  "characterName": "OtherPlayer"
}
```

**Received**: When player disconnects
**Handled**: BP_SocketManager.OnPlayerLeft

---

## Blueprint JSON Nodes

| Node | Purpose |
|------|---------|
| Construct Json Object | Create JSON payload |
| Set String Field | Add field to object |
| Construct Json Object Value | Convert to emit format |
| Value From Json String | Parse received JSON |
| Try Get String Field | Extract field value |

---

## Data Flow

```
Client                           Server
------                           ------
  |                                |
  |-- player:join ---------------->|
  |   {characterId, token, name}   |
  |                                |
  |-- player:position (30Hz) ----->|
  |   {characterId, x, y, z}       |
  |                                |
  |<-- player:moved ---------------|
  |   {id, name, x, y, z, time}    |
  |                                |
  |<-- player:left ----------------|
      {characterId, name}
```

---

## Common Issues

**Field names must be lowercase**: `characterId` not `characterID`

**All values are strings**: Even numbers are sent as strings, convert with `To String` / `To Float`

---

**Last Updated**: 2026-02-04
