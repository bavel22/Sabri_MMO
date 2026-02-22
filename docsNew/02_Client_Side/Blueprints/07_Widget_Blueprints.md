# Widget Blueprints (UMG)

All widgets are located at `/Game/SabriMMO/Widgets/`. All inherit from `UserWidget`.

---

## WBP_LoginScreen

**Path**: `/Game/SabriMMO/Widgets/WBP_LoginScreen`  
**Created By**: `BP_GameFlow` (Event BeginPlay, when not authenticated)  
**Nodes**: 39

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `Username` | string | Entered username text |
| `Password` | string | Entered password text |

### Widget Children (referenced in graph)

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `TB_Username` | TextBox | Username input field |
| `TB_Password` | TextBox | Password input field |
| `LoginButton` | Button | Triggers login |
| `Txt_Error` | TextBlock | Error message display |

### Event Graph Flow

```
Event Construct
    ↓
Get Player Controller → Set Input Mode UI Only
Get Player Controller → Set bShowMouseCursor = true
TB_Username → Set User Focus (auto-focus username field)

On Clicked (LoginButton)
    ↓
Get TB_Username → GetText → ToString → Set Username
Get TB_Password → GetText → ToString → Set Password
    ↓
Branch: Username != "" AND Password != ""?
    ├─ TRUE:
    │   Txt_Error → Set Visibility = Hidden
    │   Login User (UHttpManager static call, Username, Password)
    │   Txt_Error → SetText("Logging in...")
    │   Txt_Error → Set Visibility = Visible
    └─ FALSE:
        Txt_Error → SetText("Please enter username and password")
        Txt_Error → Set Visibility = Visible
```

**Note**: Login result is handled by `BP_GameFlow` via `OnLoginSuccess`/`OnLoginFailed` delegates on GameInstance.

---

## WBP_CharacterSelect

**Path**: `/Game/SabriMMO/Widgets/WBP_CharacterSelect`  
**Created By**: `BP_GameFlow.OnCharacterListReceivedEvent`  
**Nodes**: 31

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `Character Id` | int | Selected character ID |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `CharacterListScrollBox` | ScrollBox | Container for character entries |
| `CreateCharacterButton` | Button | Opens character creation |
| `EnterWorldButton` | Button | Enters game with selected character |

### Event Graph Flow

```
Event Construct
    ↓
Get Game Instance → Cast To MMOGameInstance → Get CharacterList
    ↓
For Each Loop (CharacterList):
    Create WBP_CharacterEntry Widget
    → Setup Character Data (pass FCharacterData struct)
    → CharacterListScrollBox → Add Child

On Clicked (CreateCharacterButton)
    ↓
Remove from Parent (self)
Create WBP_CreateCharacter Widget → Add to Viewport

On Clicked (EnterWorldButton)
    ↓
Get Game Instance → Cast To MMOGameInstance → Get Selected Character
    → Break Character Data → CharacterId
    ↓
Branch: CharacterId > 0?
    ├─ TRUE:
    │   Remove from Parent (self)
    │   Get Player Controller → Set Input Mode Game Only
    │   Open Level (by Name) — loads game level
    └─ FALSE:
        Print String "No character selected"
```

---

## WBP_CharacterEntry

**Path**: `/Game/SabriMMO/Widgets/WBP_CharacterEntry`  
**Created By**: `WBP_CharacterSelect` (in character list loop)  
**Nodes**: Event Graph 10, Function `SetupCharacterData` 12

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `CharacterId` | int | Character's server ID |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `SelectButton` | Button | Selects this character |
| `CharacterNameText` | TextBlock | Displays character name |
| `ClassText` | TextBlock | Displays character class |
| `LevelText` | TextBlock | Displays character level |

### Function: SetupCharacterData

```
SetupCharacterData(CharData: FCharacterData)
    ↓
Break Character Data
Set CharacterId = CharData.CharacterId
CharacterNameText → SetText(CharData.Name)
ClassText → SetText(CharData.CharacterClass)
LevelText → SetText("Lv. " + CharData.Level)
```

### Event Graph

```
On Clicked (SelectButton)
    ↓
Get Game Instance → Cast To MMOGameInstance
    → Select Character (CharacterId)
```

---

## WBP_CreateCharacter

**Path**: `/Game/SabriMMO/Widgets/WBP_CreateCharacter`  
**Created By**: `WBP_CharacterSelect` (Create button)  
**Nodes**: 30

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `NameInput` | EditableTextBox | Character name input |
| `ClassDropDown` | ComboBoxString | Class selection dropdown |
| `CreateButton` | Button | Create character |
| `CancelButton` | Button | Return to character select |

### Event Graph Flow

```
Event Construct
    ↓
ClassDropDown → Add Options: "warrior", "mage", "archer", "healer", "priest"
ClassDropDown → Set Selected Option = "warrior"

On Clicked (CreateButton)
    ↓
Get NameInput → Get Text → Print (debug)
Get Game Instance → Cast To MMOGameInstance
Get ClassDropDown → Get Selected Option
    ↓
Create Character (UHttpManager static call, Name, Class)
Remove from Parent (self)

On Clicked (CancelButton)
    ↓
Remove from Parent (self)
Create WBP_CharacterSelect Widget → Add to Viewport
```

---

## WBP_GameHUD

**Path**: `/Game/SabriMMO/Widgets/WBP_GameHUD`  
**Created By**: `AC_HUDManager.InitializeHUD`  
**Nodes**: 3 (logic handled externally by AC_HUDManager)

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `CurrentHealth` | real | Local player current HP |
| `MaxHealth` | real | Local player max HP |
| `CurrentMana` | real | Local player current MP |
| `MaxMana` | real | Local player max MP |
| `TargetCurrentHealth` | real | Target frame current HP |
| `TargetMaxHealth` | real | Target frame max HP |
| `TargetCurrentMana` | real | Target frame current MP |
| `TargetMaxMana` | real | Target frame max MP |

### Widget Children (inferred from AC_HUDManager usage)

| Widget Name | Type | Purpose |
|-------------|------|---------|
| HP ProgressBar | ProgressBar | Player HP bar (red) |
| MP ProgressBar | ProgressBar | Player MP bar (blue) |
| HP Text | TextBlock | "CurrentHP / MaxHP" |
| MP Text | TextBlock | "CurrentMP / MaxMP" |
| Player Name | TextBlock | Local player name |
| Target Frame (container) | Panel | Target HP/name display |
| Target HP ProgressBar | ProgressBar | Target HP bar |
| Target Name Text | TextBlock | Target name |
| Target HP Text | TextBlock | Target "HP / MaxHP" |

**Note**: WBP_GameHUD has minimal own logic — all updates come from `AC_HUDManager` functions that directly set widget properties.

---

## WBP_PlayerNameTag

**Path**: `/Game/SabriMMO/Widgets/WBP_PlayerNameTag`  
**Used As**: World-space WidgetComponent on BP_MMOCharacter, BP_OtherPlayerCharacter, BP_EnemyCharacter  
**Nodes**: Event Graph 3, Function `SetPlayerName` 3

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `PlayerName` | text | Display name text |

### Function: SetPlayerName

```
SetPlayerName(NewName: Text)
    ↓
Set PlayerName = NewName
(Text widget bound to PlayerName variable via binding)
```

---

## WBP_ChatWidget

**Path**: `/Game/SabriMMO/Widgets/WBP_ChatWidget`  
**Created By**: `AC_HUDManager.InitializeHUD`  
**Nodes**: 17

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `CurrentChannel` | string | Active chat channel (default: "GLOBAL") |
| `SocketManagerRef` | object | Reference to BP_SocketManager |
| `HUDManagerRef` | object (AC_HUDManager) | Reference to HUD manager |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `SendButton` | Button | Send chat message |
| `ChatInput` | EditableText | Message input field |
| `ChatScrollBox` | ScrollBox | Message history container |

### Event Graph Flow

```
Event Construct
    ↓
Get Actor Of Class(BP_MMOCharacter) → Get HUDManager → Set HUDManagerRef

On Clicked (SendButton)
    ↓
HUDManagerRef → Send Chat Message Request

OnTextCommitted (ChatInput) [Enter Key]
    ↓
Switch on ETextCommit
    ├─ OnEnter: HUDManagerRef → Send Chat Message Request
    └─ Other: (ignore)
```

---

## WBP_ChatMessageLine

**Path**: `/Game/SabriMMO/Widgets/WBP_ChatMessageLine`  
**Created By**: `AC_HUDManager.AddChatMessage`  
**Nodes**: Event Graph 3, Function 5

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `MessageText` | string | Full formatted message text |

### Function: Get_LineMessageText_Text
Binding function that converts `MessageText` string to Text for display.

---

## WBP_StatAllocation

**Path**: `/Game/SabriMMO/Widgets/WBP_StatAllocation`  
**Created By**: `AC_HUDManager.ToggleStats`  
**Nodes**: 36

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `SocketManagerRef` | object | Socket manager reference |
| `CurrentStatPoints` | int | Available stat points |
| `HUDManagerRef` | object (AC_HUDManager) | HUD manager reference |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `STR_Button` | Button | Allocate STR point |
| `AGI_Button` | Button | Allocate AGI point |
| `VIT_Button` | Button | Allocate VIT point |
| `INT_Button` | Button | Allocate INT point |
| `DEX_Button` | Button | Allocate DEX point |
| `LUK_Button` | Button | Allocate LUK point |
| `CloseButton` | Button | Close stat window |
| `STR_Value` | TextBlock | STR stat display |
| `AGI_Value` | TextBlock | AGI stat display |
| `VIT_Value` | TextBlock | VIT stat display |
| `INT_Value` | TextBlock | INT stat display |
| `DEX_Value` | TextBlock | DEX stat display |
| `LUK_value` | TextBlock | LUK stat display |
| `ATK_Value` | TextBlock | ATK = WeaponATK + StatusATK |
| `DEF_Value` | TextBlock | SoftDEF display |
| `HIT_Value` | TextBlock | Hit stat display |
| `FLEE_Value` | TextBlock | Flee stat display |
| `ASPD_Value` | TextBlock | ASPD stat display |
| `CRIT_Value` | TextBlock | Critical rate display (NEW) |
| `StatPointsText` | TextBlock | Remaining stat points |

### Event Graph Flow

```
Event Construct
    ↓
Get Actor Of Class(BP_MMOCharacter) → Get HUDManager → Set HUDManagerRef

On Clicked (STR_Button) → HUDManagerRef → SendStatIncreaseRequest("str")
On Clicked (AGI_Button) → HUDManagerRef → SendStatIncreaseRequest("agi")
On Clicked (VIT_Button) → HUDManagerRef → SendStatIncreaseRequest("vit")
On Clicked (INT_Button) → HUDManagerRef → SendStatIncreaseRequest("int")
On Clicked (DEX_Button) → HUDManagerRef → SendStatIncreaseRequest("dex")
On Clicked (LUK_Button) → HUDManagerRef → SendStatIncreaseRequest("luk")
On Clicked (CloseButton) → HUDManagerRef → ToggleStats
```

---

## WBP_TargetHealthBar

**Path**: `/Game/SabriMMO/Widgets/WBP_TargetHealthBar`  
**Used As**: World-space WidgetComponent on BP_OtherPlayerCharacter and BP_EnemyCharacter  
**Nodes**: Event Graph 3, Function `UpdateHealth` 4

### Function: UpdateHealth

```
UpdateHealth(Current: float, Max: float)
    ↓
HealthBar (ProgressBar) → Set Percent = Current / Max
```

---

## WBP_InventoryWindow

**Path**: `/Game/SabriMMO/Widgets/WBP_InventoryWindow`  
**Created By**: `AC_HUDManager.ToggleInventory`  
**Nodes**: 34

### Variables

| Variable | Type | Purpose |
|----------|------|------|
| `InventoryItems` | string | Raw JSON inventory data |
| `bIsOpen` | bool | Open state flag |
| `HUDManagerRef` | object (AC_HUDManager) | HUD manager reference |
| `EquippedWeaponInventoryId` | int | inventory_id of currently equipped weapon (0 = none) |
| `EquippedArmorInventoryId` | int | inventory_id of currently equipped armor (0 = none) |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|--------|
| `CloseButton` | Button | Close inventory |
| `InventoryGrid` | WrapBox/Panel | Container for inventory slots |
| `UnEquipWeaponButton` | Button | Shows equipped weapon name; click to unequip. Shows "No Weapon" when empty |
| `UnEquipArmorButton` | Button | Shows equipped armor name; click to unequip. Shows "No Armor" when empty |
| `ZuzucoinText` | TextBlock | Top-right: shows current Zuzucoin balance (e.g. "1500 Z"). Gold color. Set by `AC_HUDManager.PopulateInventory` on inventory open and by `AC_HUDManager.UpdateZuzucoinEverywhere` on transactions |

### Functions

| Function | Nodes | Description |
|----------|-------|-------------|
| `SetEqippedSlotIcons` *(typo in BP — missing 'u' in Equipped)* | 40 | Sets icon textures on the weapon and armor equipment display slots using DT_ItemIcons lookup by item_id. Called by `AC_HUDManager.PopulateInventory` after inventory data is parsed |
| `UpdateZuzucoin` | 8 | Receives `Amount (int)`, formats as "Amount Z", sets `ZuzucoinText`. Called by `AC_HUDManager.PopulateInventory` (on open) and `AC_HUDManager.UpdateZuzucoinEverywhere` (on buy/sell) |

### Event Graph Flow (34 nodes)

```
Event Construct
    ↓
Get Actor Of Class(BP_MMOCharacter) → Get HUDManager → Set HUDManagerRef

On Clicked (CloseButton)
    ↓
HUDManagerRef → Toggle Inventory

On Clicked (UnEquipWeaponButton)
    ↓
IsValid(HUDManagerRef)?
    ├─ TRUE: EquippedWeaponInventoryId > 0?
    │   ├─ TRUE: HUDManagerRef → Send Equip Request(EquippedWeaponInventoryId, false)
    │   └─ FALSE: Print "No weapon equipped" (debug)
    └─ FALSE: Print "HUDManagerRef invalid"

On Clicked (UnEquipArmorButton)
    ↓
IsValid(HUDManagerRef)?
    ├─ TRUE: EquippedArmorInventoryId > 0?
    │   ├─ TRUE: HUDManagerRef → Send Equip Request(EquippedArmorInventoryId, false)
    │   └─ FALSE: Print "No armor equipped" (debug)
    └─ FALSE: Print "HUDManagerRef invalid"
```

**NEW #3**: `UnEquipWeaponButton` and `UnEquipArmorButton` text must be set by `AC_HUDManager.PopulateInventory` — "No Weapon" / "No Armor" when nothing equipped.

---

## WBP_InventorySlot

**Path**: `/Game/SabriMMO/Widgets/WBP_InventorySlot`  
**Created By**: `AC_HUDManager.PopulateInventory` (one per item)  
**Nodes**: ~80 (58 base + icon/right-click/drag additions)

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `InventoryId` | int | Database inventory entry ID |
| `ItemId` | int | Item definition ID — used for DT_ItemIcons lookup |
| `ItemName` | string | Display name |
| `ItemType` | string | weapon/armor/consumable/etc |
| `Quantity` | int | Stack count |
| `bIsEquipped` | bool | Whether currently equipped |
| `EquipSlot` | string | Equipment slot name |
| `ItemDescription` | string | Item description text |
| `ATK` | int | Attack power |
| `DEF` | int | Defense power |
| `WeaponType` | string | dagger/one_hand_sword/bow |
| `WeaponRange` | int | Attack range |
| `AspdModifier` | int | ASPD modifier |
| `RequiredLevel` | int | Required level to equip |
| `ParentWindowInventoryWindowRef` | object | Parent inventory window ref (passed during creation) |
| `OnSlotAction` | Multicast Delegate | Fired when slot is double-clicked (left) |
| `ContextMenuRef` | object (WBP_ContextMenu) | Reference to the currently open context menu (for cleanup) |
| `Mouse Event` | struct (PointerEvent) | Cached pointer event used in right-click/drag detection |

**Note**: `HUDManagerRef` is NOT a variable on WBP_InventorySlot. It is accessed through `ParentWindowInventoryWindowRef.HUDManagerRef` (a property on the parent WBP_InventoryWindow).

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `ItemIconImage` | Image | Item icon texture (z-order: behind all overlays) |
| `EquippedIndicator` | Widget | Shows "E" or highlight when equipped |
| `QuantityText` | TextBlock | Shows stack count (bottom-right) |

### Functions

| Function | Nodes | Description |
|----------|-------|-------------|
| `SetItemIcon` | 8 | Reads `ItemId`, looks up `DT_ItemIcons` row (row name = ItemId as string), breaks `S_ItemIconRow` → sets `ItemIconImage` brush texture |
| `OnMouseButtonDown` | 55 | Override: handles right-click (create WBP_ContextMenu) and left-click (Detect Drag If Pressed). Returns Handled/Unhandled reply |
| `OnDragDetected` | 34 | Override: creates WBP_DragVisual + BP_InventoryDragDrop payload, returns drag operation |
| `OnDrop` | 33 | Override: casts operation to BP_InventoryDragDrop, calls SendMoveItemRequest if valid target |
| `OnMouseButtonDoubleClick` | 16 | Override: fires OnSlotAction delegate on left double-click |

### Event Graph Flow

```
Event Construct
    ↓
Branch: bIsEquipped?
    ├─ TRUE: EquippedIndicator → Set Visibility = Visible
    └─ FALSE: EquippedIndicator → Set Visibility = Hidden
QuantityText → SetText(Quantity as text)

On Mouse Button Double Click (MyGeometry, MouseEvent)
    ↓
Break PointerEvent → Effecting Button
    └─ Left Mouse Button: Call OnSlotAction (InventoryId, ItemType, NOT bIsEquipped)

On Mouse Button Down (MyGeometry, MouseEvent)
    ↓
Break PointerEvent → Effecting Button
    ├─ Right Mouse Button:
    │   NULL GUARD: IsValid(ParentWindowInventoryWindowRef) → FALSE: Return Unhandled
    │   NULL GUARD: IsValid(HUDManagerRef) → FALSE: Return Unhandled
    │   Create WBP_ContextMenu → Set InventoryId, ItemType, bIsEquipped, HUDManagerRef
    │   Add to Viewport → Set Position in Viewport (cursor Screen Space Position)
    │   Return → Handled
    ├─ Left Mouse Button:
    │   Detect Drag If Pressed (PointerEvent, Left Mouse Button) → Return Reply
    └─ Other: Return Unhandled

On Drag Detected (MyGeometry, PointerEvent)
    ↓
Create WBP_DragVisual → Set DragName=ItemName, DragIcon=(DT_ItemIcons lookup by ItemId)
Create Drag Drop Operation (Class: BP_InventoryDragDrop, Visual: WBP_DragVisual)
Cast → Set DragInventoryId, DragItemId, DragItemName
Return Operation

On Drop (MyGeometry, PointerEvent, Operation)
    ↓
Cast To BP_InventoryDragDrop
    ├─ Cast Failed: Return false
    └─ Success:
        Branch: DragInventoryId == InventoryId (self)? → TRUE: Return false
        NULL GUARD: IsValid(ParentWindowInventoryWindowRef) → FALSE: Return false
        NULL GUARD: IsValid(HUDManagerRef) → FALSE: Return false
        HUDManagerRef → Send Move Item Request(DragInventoryId, InventoryId)
        Return true

Event On Mouse Enter
    ↓
NULL GUARD: IsValid(ParentWindowInventoryWindowRef) → FALSE: Print + Return
    ↓
NULL GUARD: IsValid(ParentWindowInventoryWindowRef.HUDManagerRef) → FALSE: Print + Return
    ↓
ParentWindowInventoryWindowRef.HUDManagerRef → Show Item Tooltip(
    InventoryId, ItemName, ItemType,
    ATK, DEF, ItemDescription,
    WeaponRange, AspdModifier, RequiredLevel)

Event On Mouse Leave
    ↓
NULL GUARD: IsValid(ParentWindowInventoryWindowRef) → FALSE: Print + Return
    ↓
NULL GUARD: IsValid(ParentWindowInventoryWindowRef.HUDManagerRef) → FALSE: Print + Return
    ↓
ParentWindowInventoryWindowRef.HUDManagerRef → Hide Item Tooltip
```

---

## WBP_ContextMenu

**Path**: `/Game/SabriMMO/Widgets/WBP_ContextMenu`  
**Created By**: `WBP_InventorySlot.On Mouse Button Down` (right-click)  
**Added To**: Viewport directly (positioned at cursor via Set Position in Viewport)  
**Nodes**: 64

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `InventoryId` | int | Inventory entry being acted on |
| `ItemType` | string | weapon/armor/consumable/etc |
| `bIsEquipped` | bool | Whether item is currently equipped |
| `HUDManagerRef` | object (AC_HUDManager) | Reference for calling action functions |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|--------|
| `MenuBackground` | Border | Dark semi-transparent panel |
| `ActionButton1` | Button | Equip/Unequip/Use based on item type |
| `Action1Text` | TextBlock | Dynamic label set in Event Construct |
| `ActionButton2` | Button | Secondary action (hidden for most types) |
| `Action2Text` | TextBlock | Dynamic label for button 2 |
| `DropButton` | Button | Always shows "Drop" |

**Note**: There is **no DismissButton**. The menu dismisses automatically when the mouse leaves the widget via `Event On Mouse Leave → Remove from Parent`.

### Event Graph Flow

```
Event Construct
    ↓
Branch: ItemType == "consumable"?
    ├─ TRUE: Action1Text → "Use", ActionButton2 → Hidden
    └─ FALSE:
        Branch: ItemType == "weapon" OR "armor"?
        ├─ TRUE: Action1Text → (bIsEquipped ? "Unequip" : "Equip"), ActionButton2 → Hidden
        └─ FALSE (etc/misc): ActionButton1 → Hidden, ActionButton2 → Hidden

On Clicked (ActionButton1)
    ↓
NULL GUARD: IsValid(HUDManagerRef) → FALSE: Print + Return
Branch: ItemType == "consumable"?
    ├─ TRUE: HUDManagerRef → Send Use Item Request(InventoryId)
    └─ FALSE: HUDManagerRef → Send Equip Request(InventoryId, NOT bIsEquipped)
Remove from Parent (self)

On Clicked (DropButton)
    ↓
NULL GUARD: IsValid(HUDManagerRef) → FALSE: Print + Return
HUDManagerRef → Send Drop Request(InventoryId)
Remove from Parent (self)

Event On Mouse Leave
    ↓
Remove from Parent (self)
```

---

## WBP_DragVisual

**Path**: `/Game/SabriMMO/Widgets/WBP_DragVisual`  
**Created By**: `WBP_InventorySlot.On Drag Detected`  
**Purpose**: Ghost widget shown during inventory drag operations. Displays the item icon and name.

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `DragIcon` | Texture2D Object Reference | Icon texture to display |
| `DragName` | string | Item name label |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|--------|
| `DragIconImage` | Image | 64×64 icon, Opacity: 0.7 |
| `DragItemNameText` | TextBlock | Small item name below icon |

### Event Construct

```
Event Construct → DragIconImage → Set Brush from Texture(DragIcon)
              → DragItemNameText → SetText(DragName)
```

---

## BP_InventoryDragDrop

**Path**: `/Game/SabriMMO/Blueprints/BP_InventoryDragDrop`  
**Parent Class**: `DragDropOperation`  
**Created By**: `WBP_InventorySlot.On Drag Detected`  
**Purpose**: Payload object passed during inventory drag-and-drop. Carries the source slot data so the drop target knows which inventory entry is being moved.

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `DragInventoryId` | int | `inventory_id` of the item being dragged (source slot) |
| `DragItemId` | int | `item_id` for icon/name display during drag |
| `DragItemName` | string | Item name for display in WBP_DragVisual |
| `DragItemType` | string | Item type string (consumable/weapon/armor/etc) — used by hotbar drop handler to validate droppable types |
| `DragQuantity` | int | Current stack count of the dragged item — used by hotbar slot display after drop |

### Usage Pattern

```
On Drag Detected (WBP_InventorySlot)
    ↓
Create Drag Drop Operation (Class: BP_InventoryDragDrop)
    → Cast To BP_InventoryDragDrop
    → Set DragInventoryId = InventoryId (self)
    → Set DragItemId = ItemId (self)
    → Set DragItemName = ItemName (self)
    → Return as Operation

On Drop (WBP_InventorySlot)
    ↓
Cast Operation To BP_InventoryDragDrop
    → Get DragInventoryId (source)
    → Compare to InventoryId (self = target)
    → HUDManagerRef → SendMoveItemRequest(source, target)
```

---

## WBP_Hotbar

**Path**: `/Game/SabriMMO/Widgets/WBP_Hotbar`  
**Created By**: `AC_HUDManager.InitializeHotbar`  
**Nodes**: Event Graph 77, Functions 2

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `HUDManagerRef` | object (AC_HUDManager) | Reference to HUD manager for item use operations |
| `HotbarSlots` | object[] (array of WBP_HotbarSlot) | Ordered array of all 9 slot widget references (populated in Event Construct) |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|--------|
| `WBP_HotbarSlotslot1` | WBP_HotbarSlot | Slot index 0, bound to key `1` |
| `WBP_HotbarSlotslot2` | WBP_HotbarSlot | Slot index 1, bound to key `2` |
| `WBP_HotbarSlotslot3` | WBP_HotbarSlot | Slot index 2, bound to key `3` |
| `WBP_HotbarSlotslot4` | WBP_HotbarSlot | Slot index 3, bound to key `4` |
| `WBP_HotbarSlotslot5` | WBP_HotbarSlot | Slot index 4, bound to key `5` |
| `WBP_HotbarSlotslot6` | WBP_HotbarSlot | Slot index 5, bound to key `6` |
| `WBP_HotbarSlotslot7` | WBP_HotbarSlot | Slot index 6, bound to key `7` |
| `WBP_HotbarSlotslot8` | WBP_HotbarSlot | Slot index 7, bound to key `8` |
| `WBP_HotbarSlotslot9` | WBP_HotbarSlot | Slot index 8, bound to key `9` |

### Functions

| Function | Nodes | Description |
|----------|-------|-------------|
| `GetSlotAtIndex` | 6 | Returns WBP_HotbarSlot at given 0-based index from the `HotbarSlots` array. Called by `AC_HUDManager.PopulateHotbarFromServer` and `UseHotbarSlot` |
| `SyncWithInventory` | 47 | Iterates all 9 slots; for each slot with a valid `HotbarInventoryId`: updates `HotbarQuantity` display. Called by `AC_HUDManager.SyncHotbarWithInventory` after inventory changes |

### Event Graph Flow (77 nodes)

```
Event Construct
    ↓
[Build HotbarSlots array]
    Add WBP_HotbarSlotslot1 through WBP_HotbarSlotslot9 to HotbarSlots array
    ↓
[Set SlotIndex and key label on each slot]
    For each slot (0–8):
        Set SlotIndex = 0..8
        Get SlotKeyText widget → SetText("1".."9") (displays key binding label)
```

**Note**: `HUDManagerRef` is set externally by `AC_HUDManager.InitializeHotbar` after widget creation.

---

## WBP_HotbarSlot

**Path**: `/Game/SabriMMO/Widgets/WBP_HotbarSlot`  
**Created By**: Placed directly in `WBP_Hotbar` Designer as named children (not dynamically created)  
**Nodes**: Event Graph 8, Functions 5

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `SlotIndex` | int | 0-based hotbar slot position (0 = key 1 ... 8 = key 9) |
| `HotbarInventoryId` | int | `inventory_id` of the assigned item (0 = empty) |
| `HotbarItemId` | int | `item_id` of the assigned item (0 = empty) |
| `HotbarItemName` | string | Display name of the assigned item |
| `HotbarQuantity` | int | Current stack count — updated on inventory sync |
| `HUDManagerRef` | object (AC_HUDManager) | Reference for calling `SendSaveHotbarSlotRequest` on drop |

### Functions

| Function | Nodes | Description |
|----------|-------|-------------|
| `SetSlotData` | 22 | Sets all slot variables (`HotbarInventoryId`, `HotbarItemId`, `HotbarItemName`, `HotbarQuantity`) and updates the visual — sets item icon from DT_ItemIcons lookup (by `HotbarItemId`) and sets quantity text |
| `ClearSlot` | 9 | Resets all slot variables to 0/empty string, hides item icon, clears quantity text. Called on Event Construct and when item is removed |
| `UpdateQuantity` | 11 | Sets `HotbarQuantity` to new value and updates quantity TextBlock display. Called during inventory sync |
| `OnDragOver` | 4 | Override: returns true to allow drag operations over this slot |
| `OnDrop` | 41 | Override: casts dropped `BP_InventoryDragDrop` operation. Gets `DragInventoryId`, `DragItemId`, `DragItemName`. Calls `HUDManagerRef → SendSaveHotbarSlotRequest(SlotIndex, DragInventoryId, DragItemId, DragItemName)`. Calls `SetSlotData` locally to update visual immediately |

### Event Graph Flow (8 nodes)

```
Event Construct
    ↓
Clear Slot (resets all visuals to empty state)
    ↓
Get Actor Of Class(BP_MMOCharacter) → Get HUDManager → Set HUDManagerRef
```

**Note**: The `SlotKeyText` widget (TextBlock) is set from the parent `WBP_Hotbar` Event Construct, not here — parent sets both `SlotIndex` and the key label text on each child slot.

---

## WBP_ItemTooltip

**Path**: `/Game/SabriMMO/Widgets/WBP_ItemTooltip`  
**Created By**: `AC_HUDManager.ShowItemTooltip` (on mouse enter WBP_InventorySlot)  
**Nodes**: 49

### Variables

| Variable | Type | Purpose |
|----------|------|--------|
| `TooltipItemName` | string | Item name to display |
| `TooltipItemType` | string | Item type label |
| `TooltipATK` | int | ATK value (weapon) |
| `TooltipDEF` | int | DEF value (armor) |
| `TooltipDescription` | string | Flavor/description text |
| `TooltipRequiredLevel` | int | Required level |
| `TooltipWeaponRange` | int | Attack range (weapon) |
| `TooltipASPDModifier` | int | ASPD modifier (weapon) |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|--------|
| `ItemNameText` | TextBlock | Shows `TooltipItemName` |
| `ItemTypeText` | TextBlock | Shows `TooltipItemType` (with Append) |
| `StatsText` | TextBlock | Shows weapon stats: "ATK: X \| Range: Y" (visible if ATK > 0) |
| `DefText` | TextBlock | Shows armor DEF (visible if DEF > 0, hidden otherwise) |
| `DescriptionText` | TextBlock | Shows `TooltipDescription` |
| `ReqLevelText` | TextBlock | Shows "Req Lv: X" |

### Event Graph Flow

```
Event Construct
    ↓
Set ItemNameText → TooltipItemName
Set ItemTypeText → Append(TooltipItemType)
Branch: TooltipATK > 0?
    ├─ TRUE: StatsText → Format "ATK: {ATK} | Range: {WeaponRange}"
    └─ FALSE: (no weapon stats shown)
Branch: TooltipDEF > 0?
    ├─ TRUE: DefText → Set Visibility = Visible → SetText(TooltipDEF)
    └─ FALSE: DefText → Set Visibility = Hidden
Set DescriptionText → TooltipDescription
Set ReqLevelText → Append "Req Lv: " + TooltipRequiredLevel
```

---

## WBP_DeathOverlay

**Path**: `/Game/SabriMMO/Widgets/WBP_DeathOverlay`  
**Created By**: `AC_HUDManager.ShowDeathOverlay`  
**Nodes**: 16

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `HUDManagerRef` | object (AC_HUDManager) | HUD manager reference |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `RespawnButton` | Button | Triggers respawn |
| Death message text | TextBlock | "You died" message |

### Event Graph Flow

```
Event Construct
    ↓
Get Player Controller → Set Input Mode UI Only
Get Player Controller → Set bShowMouseCursor = true
RespawnButton → Set User Focus
Get Actor Of Class(BP_MMOCharacter) → Get HUDManager → Set HUDManagerRef

On Clicked (RespawnButton)
    ↓
HUDManagerRef → Send Respawn Request
```

---

## WBP_LootPopup

**Path**: `/Game/SabriMMO/Widgets/WBP_LootPopup`  
**Created By**: `AC_HUDManager.ShowLootPopup`  
**Nodes**: 22

### Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `FadeTimer` | real | (set by creator) | Countdown timer for auto-fade |
| `FadeDuration` | real | (set by creator) | Total fade duration |
| `InDeltaTime` | real | — | Cached delta time |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `LootRoot` | Panel | Container for loot item entries |

### Event Graph Flow

```
Event Tick
    ↓
Set InDeltaTime = DeltaTime
Branch: FadeTimer > 0?
    ├─ TRUE:
    │   FadeTimer -= InDeltaTime
    │   Branch: FadeTimer <= 0?
    │   ├─ TRUE: Remove from Parent (self)
    │   └─ FALSE: LootRoot → Set Render Opacity = FadeTimer / FadeDuration
    └─ FALSE:
        LootRoot → Set Render Opacity = 1.0
```

---

## WBP_DamageNumber

**Path**: `/Game/SabriMMO/Widgets/WBP_DamageNumber`  
**Created By**: `AC_HUDManager.ShowDamageNumbers`  
**Nodes**: 17

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `DamageAmount` | int | Damage value to display |
| `FloatSpeed` | real | Upward float speed |
| `FadeDuration` | real | Time before removal |
| `ElapsedTime` | real | Time since creation |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `DamageText` | TextBlock | Shows damage number |

### Event Graph Flow

```
Event Tick
    ↓
ElapsedTime += DeltaTime
Branch: ElapsedTime >= FadeDuration?
    ├─ TRUE: Remove from Parent (self-destruct)
    └─ FALSE: DamageText → Set Render Opacity = 1.0 - (ElapsedTime / FadeDuration)
```

---

## WBP_Shop

**Path**: `/Game/SabriMMO/Widgets/WBP_Shop`  
**Created By**: `AC_HUDManager.OpenShop`  
**Purpose**: Main NPC shop window. Two tabs: Buy (WBP_ShopItem list) and Sell (WBP_ShopSellSlot list). Displays current Zuzucoin balance.

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `ShopId` | int | Active shop's ID |
| `ShopName` | string | Shop display name |
| `HUDManagerRef` | object (AC_HUDManager) | For calling CloseShop, SendPopulateShopSellRequest, SendBuyRequest |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `ShopTitleText` | TextBlock | Shop name header |
| `ZuzucoinText` | TextBlock | Player's current balance — "1500 Z" in gold. Updated by `UpdateZuzucoinDisplay` |
| `StatusText` | TextBlock | Error/status messages ("Not enough Zuzucoin", etc.) |
| `CloseButton` | Button | Calls `HUDManagerRef → CloseShop` |
| `BuyTab` | Button | Switches to buy view |
| `SellTab` | Button | Switches to sell view — triggers `SendPopulateShopSellRequest` |
| `ShopItemsList` | ScrollBox/Panel | Container for `WBP_ShopItem` children (buy tab) |
| `SellItemsList` | ScrollBox/Panel | Container for `WBP_ShopSellSlot` children (sell tab) |

### Functions

| Function | Nodes | Description |
|----------|-------|-------------|
| `UpdateZuzucoin` | 8 | Receives `Amount (int)`, formats as "Amount Z", sets `ZuzucoinText`. Called by `AC_HUDManager` after open or transaction |
| `SetShopData` | 16 | Called by `AC_HUDManager.OpenShop`. Sets `ShopId`, `ShopName`, populates `ShopItemsList` by creating one `WBP_ShopItem` per item in the data array |
| `ShowStatus` | 6 | Sets `StatusText` to message, makes it visible. Fades/clears after delay |

### Design Patterns
| Pattern | Application |
|---------|-------------|
| **Widget Composition** | Child WBP_ShopItem and WBP_ShopSellSlot widgets per item |
| **Dependency Injection** | HUDManagerRef passed at creation time |
| **Event-Driven** | Zuzucoin updates triggered by server events, not Tick |

---

## WBP_ShopItem

**Path**: `/Game/SabriMMO/Widgets/WBP_ShopItem`  
**Created By**: `WBP_Shop.SetShopData` (one per shop item)  
**Purpose**: Single row in the shop's Buy tab. Displays item info + buy price. Buy button emits `shop:buy`.

### Variables (Expose on Spawn: ✓ for all)

| Variable | Type | Purpose |
|----------|------|---------|
| `ItemId` | int | Item definition ID |
| `ShopId` | int | Parent shop ID (needed for buy request) |
| `ItemName` | string | Display name |
| `ItemType` | string | consumable/weapon/armor/etc |
| `BuyPrice` | int | Cost to purchase (item.price × 2) |
| `Quantity` | int | Purchase quantity (default 1) |
| `HUDManagerRef` | object (AC_HUDManager) | For calling `SendBuyRequest` or similar |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `ItemIconImage` | Image | 64×64 item icon |
| `ItemNameText` | TextBlock | Item name |
| `ItemTypeText` | TextBlock | Item type label |
| `BuyPriceText` | TextBlock | "Buy: Nz" in gold color |
| `BuyButton` | Button | Triggers buy action |

### Event Graph Flow

```
Event Construct
    → Set ItemNameText, ItemTypeText, BuyPriceText from variables
    → Set ItemIconImage from DT_ItemIcons lookup by ItemId

On Clicked (BuyButton)
    → NULL GUARD: IsValid(HUDManagerRef) → FALSE: Return
    → Format JSON: {shopId, itemId, quantity: 1}
    → HUDManagerRef.SocketManagerRef → Emit("shop:buy", json)
       OR: HUDManagerRef → SendBuyRequest(ShopId, ItemId, 1)
```

---

## WBP_ShopSellSlot

**Path**: `/Game/SabriMMO/Widgets/WBP_ShopSellSlot`  
**Created By**: `WBP_Shop` sell tab populate logic  
**Purpose**: Single row in the shop's Sell tab. Shows inventory item with sell price. Sell button emits `shop:sell`.

### Variables (Expose on Spawn: ✓ for all)

| Variable | Type | Purpose |
|----------|------|---------|
| `InventoryId` | int | Database inventory entry ID |
| `ItemId` | int | Item definition ID |
| `ItemName` | string | Display name |
| `Quantity` | int | Current stack count |
| `SellPrice` | int | Sell value (item.price × quantity) |
| `HUDManagerRef` | object (AC_HUDManager) | For sell request |

### Widget Children

| Widget Name | Type | Purpose |
|-------------|------|---------|
| `ItemIconImage` | Image | 64×64 item icon |
| `ItemNameText` | TextBlock | Item name + quantity |
| `SellPriceText` | TextBlock | "Sell: Nz" in gold color |
| `SellButton` | Button | Triggers sell action |

### Event Graph Flow

```
Event Construct
    → Set all text fields from variables
    → Set ItemIconImage from DT_ItemIcons lookup by ItemId

On Clicked (SellButton)
    → NULL GUARD: IsValid(HUDManagerRef) → FALSE: Return
    → Format JSON: {inventoryId, quantity: 1}
    → Emit via SocketManager: "shop:sell"
```

---

## BP_NPCShop (Actor)

**Path**: `/Game/SabriMMO/Blueprints/BP_NPCShop`  
**Parent Class**: `Actor`  
**Purpose**: Placeable world actor representing an NPC shop. When clicked by the local player, calls `BP_SocketManager.EmitShop(ShopId)` to request shop data from server.

### Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|--------|
| `ShopId` | int | 0 | Set per instance in Details panel (1 = General Store, 2 = Weapon Shop) |
| `ShopName` | string | "" | Display name shown above NPC or in interactions |

### Components

| Component | Type | Purpose |
|-----------|------|---------|
| `DefaultSceneRoot` | SceneComponent | Root |
| `ShopMesh` | StaticMeshComponent | Visual representation of the NPC/shop stand |
| `InteractionCollider` | CapsuleComponent/SphereComponent | Click detection area |

### Event Graph

```
Event BeginPlay
    → (Optional: set interaction text, animate idle)

On Clicked (or BPI_Interactable implementation)
    → Get SocketManagerRef (Get Actor Of Class → BP_SocketManager)
    → NULL GUARD: IsValid(SocketManagerRef) → FALSE: Return
    → SocketManagerRef → EmitShop(ShopId)
```

**Note**: The click is handled in `BP_MMOCharacter` (Left-click hit test → Cast To BP_NPCShop → get ShopId → SocketManagerRef → EmitShop). `BP_NPCShop` itself is a passive data holder.

### Instance Placement (in level)

| Actor | ShopId | ShopName | Location |
|-------|--------|----------|---------|
| BP_NPCShop_1 | 1 | General Store | Near player spawn |
| BP_NPCShop_2 | 2 | Weapon Shop | Near player spawn |

---

## Data Assets (Inventory System)

### S_ItemIconRow (Struct)

**Path**: `/Game/SabriMMO/Data/S_ItemIconRow`  
**Type**: Blueprint Struct  
**Used By**: `DT_ItemIcons` (row type), `WBP_InventorySlot.SetItemIcon`

| Field | Type | Description |
|-------|------|-------------|
| `ItemIcon` | Texture 2D (Object Reference) | The icon texture for this item |

### DT_ItemIcons (Data Table)

**Path**: `/Game/SabriMMO/Data/DT_ItemIcons`  
**Row Structure**: `S_ItemIconRow`  
**Used By**: `WBP_InventorySlot.SetItemIcon`, `WBP_InventorySlot.On Drag Detected`  
**Purpose**: Maps `item_id` (as string row name) → `Texture 2D` icon asset.

| Row Name | Icon Asset | Item |
|----------|-----------|------|
| `1001` | Icon_CrimsonVial | Crimson Vial |
| `1002` | Icon_AmberElixir | Amber Elixir |
| `1003` | Icon_GoldenSalve | Golden Salve |
| `1004` | Icon_AzurePhilter | Azure Philter |
| `1005` | Icon_RoastedHaunch | Roasted Haunch |
| `2001` | Icon_GloopyResidue | Gloopy Residue |
| `2002` | Icon_ViscousSlime | Viscous Slime |
| `2003` | Icon_ChitinShard | Chitin Shard |
| `2004` | Icon_DownyPlume | Downy Plume |
| `2005` | Icon_SporeCluster | Spore Cluster |
| `2006` | Icon_BarbedLimb | Barbed Limb |
| `2007` | Icon_VerdantLeaf | Verdant Leaf |
| `2008` | Icon_SilkenTuft | Silken Tuft |
| `3001` | Icon_RusticShiv | Rustic Shiv |
| `3002` | Icon_KeenEdge | Keen Edge |
| `3003` | Icon_StilettoFang | Stiletto Fang |
| `3004` | Icon_IronCleaver | Iron Cleaver |
| `3005` | Icon_CrescentSaber | Crescent Saber |
| `3006` | Icon_HuntingLongbow | Hunting Longbow |
| `4001` | Icon_LinenTunic | Linen Tunic |
| `4002` | Icon_QuiltedVest | Quilted Vest |
| `4003` | Icon_RingweaveHauberk | Ringweave Hauberk |

**Lookup pattern** (used in `SetItemIcon` and `On Drag Detected`):
```
Get ItemId (int) → To String (Integer) → String to Name
    → Get Data Table Row (DT_ItemIcons, Row Name)
    → Break S Item Icon Row → Item Icon (Texture2D)
    → Set Brush from Texture (ItemIconImage)
```

---

**Last Updated**: 2026-02-22 (Zuzucoin Shop/Inventory Fixes: Updated OnShopBought/OnShopSold to call UpdateZuzucoinEverywhere; added zuzucoin parsing to PopulateInventory; corrected function names UpdateZuzucoin vs UpdateZuzucoinDisplay; verified all changes via unrealMCP scans)  

**Previous**: 2026-02-22 (Feature #20 NPC Shop: Added WBP_Shop, WBP_ShopItem, WBP_ShopSellSlot, BP_NPCShop sections; WBP_InventoryWindow updated with ZuzucoinText widget + UpdateZuzucoinDisplay function. ⚠️ unrealMCP scan unavailable — node counts approximate)  
**Source**: Read via unrealMCP `read_blueprint_content` + trajectory context
