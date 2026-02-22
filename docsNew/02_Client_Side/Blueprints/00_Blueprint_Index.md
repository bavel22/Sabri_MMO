# Blueprint Documentation Index

## Data Source

All Blueprint documentation in this folder was **read directly from the UE5 Editor** using the unrealMCP MCP server tools (`read_blueprint_content`, `get_blueprint_function_details`, `analyze_blueprint_graph`). Every variable, function, node, component, and connection documented here reflects the actual Blueprint state as of 2026-02-17.

**Content Path**: `client/SabriMMO/Content/SabriMMO/` (`.uasset` files not in git)

## Known Blueprints

### Game Actors

| Blueprint | Parent | Purpose |
|-----------|--------|---------|
| `BP_MMOCharacter` | BP_ThirdPersonCharacter_C → ASabriMMOCharacter (C++) | Local player character — top-down movement, click-to-move, camera, Socket.io position broadcast |
| `BP_OtherPlayerCharacter` | ACharacter | Remote player actor — receives position from server, interpolates movement, displays name tag |
| `BP_OtherPlayerManager` | AActor | Singleton manager — spawns/updates/destroys remote player actors via Map<charId, ActorRef> |
| `BP_SocketManager` | AActor | Socket.io connection hub — binds all socket events, parses JSON, dispatches to other Blueprints |
| `BP_EnemyCharacter` | ACharacter | Client-side enemy actor — health bar, name tag, hover indicator, death visual |
| `BP_EnemyManager` | AActor | Singleton manager — spawns/updates/destroys enemy actors |
| `BP_ThirdPersonCharacter` | ASabriMMOCharacter (C++) | Base character class (touch input stubs). Parent of BP_MMOCharacter. Implements BPI_TouchInterface |
| `BP_GameFlow` | AActor | Orchestrates login → character select → enter world flow |

### Widget Blueprints (UMG)

| Widget | Purpose |
|--------|---------|
| `WBP_LoginScreen` | Login form with username/password fields, login/register buttons, error display |
| `WBP_CharacterSelect` | Character list screen with scrollable entries, create/delete/enter buttons |
| `WBP_CharacterEntry` | Individual character row in selection list (name, class, level) |
| `WBP_CreateCharacter` | Character creation dialog (name input, class dropdown) |
| `WBP_GameHUD` | In-game HUD — HP bar (red), MP bar (blue), player name, level display |
| `WBP_PlayerNameTag` | Floating name tag widget above characters (3D widget component) |
| `WBP_ChatWidget` | Chat interface — message list, input field, send button, channel selector |
| `WBP_ChatMessageLine` | Individual chat message line (sender name, message text, timestamp) |
| `WBP_StatAllocation` | Stat point allocation UI — shows base/derived stats, +/- buttons per stat |
| `WBP_TargetHealthBar` | Health bar displayed on targeted enemy/player (world-space WidgetComponent) |
| `WBP_InventoryWindow` | Inventory window with item grid, equipped weapon/armor icon slots, close button |
| `WBP_InventorySlot` | Individual item slot in inventory — icon display, double-click equip/use, right-click context menu, drag-and-drop source/target |
| `WBP_ContextMenu` | Right-click context menu for inventory slots (Equip/Unequip/Use/Drop actions) |
| `WBP_DragVisual` | Ghost drag widget shown during inventory drag-and-drop (icon + name, 0.7 opacity) |
| `WBP_ItemTooltip` | Hover tooltip showing item stats (ATK/DEF/Range/ASPD/Description/ReqLevel) |
| `WBP_DeathOverlay` | Death screen overlay with respawn button |
| `WBP_LootPopup` | Loot notification popup with auto-fade |
| `WBP_DamageNumber` | Floating damage number with fade-out |
| `WBP_Hotbar` | Hotbar bar with 9 slots — displays assigned consumable/item icons with quantity and key labels (1–9) |
| `WBP_HotbarSlot` | Individual hotbar slot — accepts drag-and-drop from WBP_InventorySlot; shows item icon, quantity, key label; triggers item use on hotbar key press |

### Drag & Drop Objects

| Blueprint | Parent | Purpose |
|-----------|--------|------|
| `BP_InventoryDragDrop` | DragDropOperation | Payload for inventory drag-and-drop — carries DragInventoryId, DragItemId, DragItemName, DragItemType, DragQuantity from source slot to inventory or hotbar target |

### Animation Blueprints

| Blueprint | Purpose |
|-----------|---------|
| `ABP_unarmed` | Unarmed character animation — idle, walk, run based on velocity |

### Actor Components

| Component | Purpose |
|-----------|---------|
| `AC_HUDManager` | Actor component managing ALL HUD widgets (33 functions) — HP/MP bars, chat, inventory, stats, hotbar, death overlay, loot, damage numbers, target frame |
| `AC_CameraController` | Actor component handling camera rotation (right-click hold) and mouse wheel zoom (4 functions) |
| `AC_TargetingSystem` | Actor component handling hover detection over players/enemies and attack range checks (3 functions, 13 variables including delegates) |

### Interfaces (Blueprint)

| Interface | Purpose |
|-----------|---------|
| `BPI_Damageable` | Blueprint interface for unified damage handling — `ReceiveDamageVisual`, `UpdateHealthDisplay`, `GetHealthInfo` |

## Socket.io Event Binding Pattern

All Socket.io events in Blueprints follow this pattern:

```
BP_SocketManager — Event BeginPlay or OnSocketConnected:
    → Get Socket IO Client Component
    → Bind Event to Function
        • Event Name: "event:name" (String)
        • Function: Custom function with input param named "Data" (String type)
    
Custom Function — OnEventName(Data: String):
    → Parse JSON String to Struct/Variables
    → Execute game logic (spawn actors, update UI, etc.)
```

**Critical**: The function parameter MUST be named exactly `Data` with type `String` for Socket.io plugin compatibility.

## Blueprint ↔ C++ Integration Points

| Blueprint | C++ Class Used | How |
|-----------|---------------|-----|
| All Widgets | `UMMOGameInstance` | Cast to get auth state, character data |
| Login/Register | `UHttpManager` | Static function calls (LoginUser, RegisterUser, etc.) |
| BP_MMOCharacter | `ASabriMMOCharacter` | Inherits movement, camera, input |
| BP_OtherPlayerCharacter | None (pure Blueprint) | Uses base CharacterMovementComponent for AddMovementInput interpolation |
| BP_EnemyCharacter | None (pure Blueprint) | Uses CharacterMovement for interpolation |

### Data Assets (Inventory System)

| Asset | Type | Path | Purpose |
|-------|------|------|---------|
| `S_ItemIconRow` | Blueprint Struct | `/Game/SabriMMO/Data/S_ItemIconRow` | Row struct for DT_ItemIcons — single field: ItemIcon (Texture2D) |
| `DT_ItemIcons` | Data Table | `/Game/SabriMMO/Data/DT_ItemIcons` | Maps item_id (string row name) → Texture2D icon. 22 rows covering all consumables, loot, weapons, and armor |

Icon assets location: `/Game/SabriMMO/Assets/Item_Icons/` — 22 `.uasset` files named `Icon_[ItemName]`.

## Conventions

- **Prefix `BP_`**: Game actor Blueprints
- **Prefix `WBP_`**: Widget Blueprints (UMG)
- **Prefix `ABP_`**: Animation Blueprints
- **Prefix `AC_`**: Actor Component Blueprints
- **Prefix `BPI_`**: Blueprint Interfaces

---

**Last Updated**: 2026-02-20 (Feature #19 Hotbar: added WBP_Hotbar and WBP_HotbarSlot; updated BP_InventoryDragDrop with DragItemType+DragQuantity; AC_HUDManager updated to 33 functions)
