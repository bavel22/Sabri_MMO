# Chat System Documentation

## Overview
Real-time multiplayer chat system with expandable channel architecture. Built using Socket.io for server communication and UMG widgets for the UI.

## Features
- ✅ Global chat between all connected players
- ✅ Real-time message broadcasting
- ✅ Enter key and Send button support
- ✅ Player names displayed with messages
- ✅ Scrollable message history
- ✅ Expandable architecture for multiple channels

## Architecture

### Server Side (Node.js)
- **Event Handler**: `chat:message` - Receives messages from clients
- **Broadcast**: `chat:receive` - Sends messages to all players
- **Channel Support**: Currently GLOBAL, expandable to ZONE, PARTY, GUILD, TELL, COMBAT

### Client Side (UE5 Blueprints)
- **WBP_ChatWidget**: Main chat interface
- **WBP_ChatMessageLine**: Individual message widget
- **BP_SocketManager**: Handles Socket.io communication

## Event Protocol

### Client → Server: chat:message
```json
{
  "characterId": "1",
  "channel": "GLOBAL",
  "message": "Hello world"
}
```

### Server → Client: chat:receive
```json
{
  "type": "chat:receive",
  "channel": "GLOBAL",
  "senderId": "1",
  "senderName": "PlayerName",
  "message": "Hello world",
  "timestamp": 1234567890
}
```

## Implementation Details

### WBP_ChatWidget Structure
```
CanvasPanel
└── Border (ChatBackground)
    └── VerticalBox
        ├── ScrollBox (ChatScrollBox)
        │   └── (Dynamic WBP_ChatMessageLine widgets)
        └── HorizontalBox (InputArea)
            ├── EditableText (ChatInput)
            └── Button (SendButton)
```

### Key Functions
- **SendChatMessage**: Sends message to server
- **AddChatMessage**: Displays received message
- **OnChatReceived**: Handles server messages

### Variables
- `ChatInput`: EditableText for typing
- `ChatScrollBox`: ScrollBox container
- `CurrentChannel`: String (default "GLOBAL")

## Setup Instructions

### 1. Server Setup
```javascript
// In server/src/index.js
socket.on('chat:message', (data) => {
    // Handler already implemented
    // Supports channel expansion
});
```

### 2. Client Setup
1. Create WBP_ChatWidget and WBP_ChatMessageLine
2. Add to viewport in Level Blueprint
3. Bind Socket.io events in Event Construct
4. Wire up input handling (Enter key, Send button)

### 3. Testing
1. Start server: `npm run dev`
2. Play with multiple players
3. Messages should appear in real-time with player names

## Future Expansion Plans

### Planned Channels
- **ZONE Chat**: Messages to players within X units radius
- **PARTY Chat**: Group/team messages
- **GUILD Chat**: Guild member messages  
- **TELL**: Private 1:1 messages
- **COMBAT**: Combat log messages
- **SYSTEM**: Server announcements

### UI Enhancements
- **Channel Tabs**: Switch between chat channels
- **Chat History**: Persistent message storage
- **Player Names**: Color coding by channel
- **Timestamps**: Message time display
- **Chat Commands**: `/tell`, `/party`, `/guild` etc.

### Server Enhancements
- **Chat Filters**: Profanity, spam protection
- **Chat Logs**: Database storage
- **Mute System**: Player moderation
- **Chat Channels**: Database-driven channel management

### Implementation Notes for Expansion
- Server already has switch statement for channel routing
- Client uses CurrentChannel variable for channel selection
- UI can be extended with tabs for channel switching
- Message format supports all planned features

## Troubleshooting

### Common Issues
1. **Messages not appearing**: Check Socket.io connection
2. **Enter key not working**: Verify OnTextCommitted with Switch node
3. **Widget not visible**: Check ZOrder in Add to Viewport
4. **Player names not showing**: Verify characterName in connectedPlayers

### Debug Tips
- Add Print String nodes to trace function calls
- Check server logs for chat messages
- Verify JSON format matches protocol
- Test with 2+ players for multiplayer

## Files Modified
- `server/src/index.js` - Added chat event handlers
- `Content/Blueprints/Widgets/WBP_ChatWidget.uasset` - Main chat UI
- `Content/Blueprints/Widgets/WBP_ChatMessageLine.uasset` - Message widget
- `Enter World.umap` - Level Blueprint (widget creation)

## Version
- Created: 2026-02-05
- Status: ✅ Complete (Global chat)
- Next: Channel expansion
