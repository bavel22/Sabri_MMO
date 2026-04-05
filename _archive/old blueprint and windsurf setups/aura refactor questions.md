# aura refactor questions:

aura refactor questions:

Add an Event Dispatcher: OnTargetSelected(TargetActor: Actor, TargetId: Integer)
what am i supposed to use this for?


Regarding #1 — AC_ClickToMove: The previous guide extracted camera and targeting into components but left click-to-move/pathfinding in BP_MMOCharacter. Want me to write the detailed steps for extracting that into AC_ClickToMove as well?


need more details on this:

i did not refactor BP_socketManager

PHASE 6: Decouple Widgets from Networking
Step 6.1: Create BPI_GameAction Interface
Content Browser → right-click /SabriMMO/ → create a new folder Interfaces
Right-click the folder → Blueprints → Blueprint Interface → name it BPI_GameAction
Add these functions to the interface:
RequestSendChat(Message: String, Channel: String)
RequestUseItem(InventoryId: Integer)
RequestEquipItem(InventoryId: Integer)
RequestUnequipItem(InventoryId: Integer)
RequestDropItem(InventoryId: Integer)
RequestAllocateStat(StatName: String)
RequestRespawn()
None of these have return values — they're all events (fire-and-forget to the networking layer)
Save
Step 6.2: Implement the Interface on Socket Handlers
Open AC_InventorySocketHandler
Class Settings → Interfaces → Add BPI_GameAction
Implement the interface events:
RequestUseItem → build JSON with the inventory ID → emit inventory:use via SocketIORef
RequestEquipItem → build JSON → emit inventory:equip
RequestDropItem → build JSON → emit inventory:drop
RequestUnequipItem → build JSON → emit inventory:unequip
Compile and save
Do the same for chat (on whatever component handles chat) and stats (on the combat handler or a new stats handler).

Step 6.3: Update Widgets to Use the Interface
WBP_InventoryWindow:

Open WBP_InventoryWindow
Remove the SocketIORef variable
In the UseItem function: Instead of calling SocketManager directly, call RequestUseItem via the BPI_GameAction interface on GetOwningPlayerPawn (or get the handler component another way — see note below)
Do the same for EquipItem, UnEquipItem, DropItem
Compile and save
Note on routing: The simplest approach is to have BP_MMOCharacter implement BPI_GameAction and route calls to the appropriate handler component. That way widgets just do: GetOwningPlayerPawn → Call Interface Function.

WBP_ChatWidget:

Remove direct SocketManager reference
In SendChatMessage, call RequestSendChat via interface on the owning player
WBP_StatAllocation:

Remove SocketManagerRef
Stat allocation button events → call RequestAllocateStat via interface
WBP_DeathOverlay:

Remove SocketManager reference
Respawn button → call RequestRespawn via interface
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
