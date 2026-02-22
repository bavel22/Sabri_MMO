# In-Game Test Checklist — Sabri_MMO

Use this checklist after any feature change to verify previously working systems are not regressed. Run a full pass before any release or major feature branch merge.

**Legend**: ✅ Pass | ❌ Fail | ⏭ Skip (not applicable) | 🔁 Retest needed

---

## 1. Authentication Flow

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 1.1 | Launch game — Login screen appears | WBP_LoginScreen shown, mouse visible | |
| 1.2 | Enter valid credentials → Login | Transitions to WBP_CharacterSelect | |
| 1.3 | Enter invalid credentials → Login | Error message shown (Txt_Error visible) | |
| 1.4 | Leave username/password blank → Login | "Please enter username and password" shown | |
| 1.5 | Click Create Character | WBP_CreateCharacter opens | |
| 1.6 | Create character with name + class → Submit | Returns to CharacterSelect with new character listed | |
| 1.7 | Select character → Enter World | Game level loads, player spawns at correct location | |

---

## 2. Player Movement & Camera

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 2.1 | Left-click ground → Player walks there | Click-to-move works, character navigates | |
| 2.2 | Right-click drag → Camera rotates | Camera rotates horizontally around player | |
| 2.3 | Mouse wheel scroll in/out → Camera zooms | Arm length changes, clamped at min/max | |
| 2.4 | Player stops at destination | No oscillation or sliding past target | |
| 2.5 | Player name tag follows character | WBP_PlayerNameTag visible above head | |

---

## 3. HUD & UI

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 3.1 | HP/MP bars visible on screen | WBP_GameHUD shows red HP bar and blue MP bar | |
| 3.2 | HP bar updates when taking damage | Bar reduces proportionally | |
| 3.3 | MP bar updates when using SP potion | Bar increases | |
| 3.4 | Press I or inventory key → Inventory opens | WBP_InventoryWindow appears with items | |
| 3.5 | Press I again → Inventory closes | Window removed from viewport | |
| 3.6 | Press S or stat key → Stat window opens | WBP_StatAllocation appears with stats | |
| 3.7 | Press S again → Stat window closes | Window removed | |

---

## 4. Inventory System

### 4a. Basic Display

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 4.1 | Open inventory → Items displayed | Each item shows icon, quantity text, equipped indicator | |
| 4.2 | Item icons show correct textures | Each slot shows matching Icon_* asset from DT_ItemIcons | |
| 4.3 | Equipped items show equipped indicator | "E" badge or highlight visible on equipped weapon/armor | |
| 4.4 | Stack count shown for consumables | QuantityText shows correct number | |
| 4.5 | Equipped weapon shown in weapon slot | UnEquipWeaponButton shows weapon icon + name | |
| 4.6 | Equipped armor shown in armor slot | UnEquipArmorButton shows armor icon + name | |
| 4.7 | No weapon equipped → weapon slot shows "No Weapon" | UnEquipWeaponButton label = "No Weapon" | |

### 4b. Item Hover Tooltip

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 4.8 | Hover over weapon slot → Tooltip shows | WBP_ItemTooltip appears with ATK, Range, ASPD | |
| 4.9 | Hover over armor slot → Tooltip shows | Tooltip shows DEF value | |
| 4.10 | Hover over consumable → Tooltip shows | Tooltip shows description, no ATK/DEF section | |
| 4.11 | Mouse leaves slot → Tooltip disappears | WBP_ItemTooltip removed from viewport | |
| 4.12 | Tooltip does not persist after closing inventory | No orphaned tooltip on screen | |

### 4c. Right-Click Context Menu

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 4.13 | Right-click weapon (unequipped) → Context menu | WBP_ContextMenu appears with "Equip" + "Drop" | |
| 4.14 | Right-click weapon (equipped) → Context menu | Context menu shows "Unequip" + "Drop" | |
| 4.15 | Right-click consumable → Context menu | Context menu shows "Use" + "Drop" | |
| 4.16 | Click outside context menu → Menu closes | DismissButton click removes menu | |
| 4.17 | Click Equip → Item equipped | Item becomes equipped, indicator shows, equipped slot updates | |
| 4.18 | Click Unequip → Item unequipped | Indicator hidden, slot shows "No Weapon/Armor" | |
| 4.19 | Click Use (consumable) → Item used | HP/SP restored, stack count decreases by 1 | |
| 4.20 | Click Drop → Item removed from inventory | Item no longer in grid after refresh | |

### 4d. Double-Click

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 4.21 | Double-click unequipped weapon | Equip request sent, item becomes equipped | |
| 4.22 | Double-click equipped weapon | Unequip request sent | |
| 4.23 | Double-click consumable | Use request sent, HP/SP restored | |

### 4e. Drag & Drop

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 4.24 | Left-click hold + drag item → Drag visual appears | WBP_DragVisual shows item icon + name at 0.7 opacity | |
| 4.25 | Drag item to another slot → Drop | Slots swap positions in grid after inventory:data refresh | |
| 4.26 | Drag item to same slot → No action | No server request sent, no visual change | |
| 4.27 | Release drag outside any slot → No error | Drag cancelled, no crash, no orphaned widgets | |

### 4f. Equip Slot Buttons

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 4.28 | Click UnEquipWeaponButton with weapon equipped | Weapon unequipped, slot shows "No Weapon" | |
| 4.29 | Click UnEquipWeaponButton with nothing equipped | Print debug (no crash) | |
| 4.30 | Click UnEquipArmorButton with armor equipped | Armor unequipped, slot shows "No Armor" | |

---

## 5. Combat System

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 5.1 | Click enemy → Target frame appears | WBP_GameHUD target frame visible with enemy name + HP | |
| 5.2 | Auto-attack starts → Character attacks | Attack animation plays at ASPD rate | |
| 5.3 | Enemy takes damage → HP bar updates | Target frame HP reduces | |
| 5.4 | Enemy dies → Target frame hides | Target cleared, frame hidden | |
| 5.5 | Damage number appears on hit | Floating red/white number pops above target | |
| 5.6 | Move out of range → Character moves toward target | Client moves into attack range before attacking | |
| 5.7 | Stop attacking (click ground) → Auto-attack stops | No more attack events sent | |
| 5.8 | Player takes damage → HP bar decreases | WBP_GameHUD local HP bar reduces | |
| 5.9 | Player dies → Death overlay appears | WBP_DeathOverlay shown, respawn button visible | |
| 5.10 | Click Respawn → Player respawns | Player teleports to spawn, HP restored, overlay gone | |

---

## 6. Loot System

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 6.1 | Kill enemy → Loot popup appears | WBP_LootPopup shows dropped items with names + qty | |
| 6.2 | Loot popup auto-fades | Popup opacity reduces and widget removed after duration | |
| 6.3 | Item added to inventory after kill | Open inventory: new item present in grid with icon | |
| 6.4 | Stackable item drops on existing stack | Quantity increases on existing slot (no duplicate slot) | |

---

## 7. Stats System

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 7.1 | Open stat window → All values shown | STR/AGI/VIT/INT/DEX/LUK, derived stats, ATK, DEF, CRIT all populated | |
| 7.2 | Stat points > 0 → + buttons visible | Allocation buttons enabled | |
| 7.3 | Click + button for a stat → Stat increases | Server returns updated stats, display refreshes | |
| 7.4 | Equip weapon → ATK updates in stat window | ATK = WeaponATK + StatusATK, displayed correctly | |
| 7.5 | Stat points = 0 → No more allocation | Points display shows 0 | |

---

## 8. Chat System

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 8.1 | Type in chat → Press Enter → Message sent | Message appears in chat log with correct format | |
| 8.2 | Click Send button → Message sent | Same as above | |
| 8.3 | Receive message from another player | Message appears in chat scroll box | |
| 8.4 | Chat auto-scrolls to latest message | Bottom of scroll box always visible after new message | |

---

## 9. Multiplayer (Requires 2+ Clients)

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 9.1 | Second player joins → Appears in world | BP_OtherPlayerCharacter spawned, name tag visible | |
| 9.2 | Remote player moves → Interpolates smoothly | No teleporting, smooth movement |  |
| 9.3 | Remote player takes damage → HP bar updates | WBP_TargetHealthBar reduces on remote character | |
| 9.4 | Remote player dies → Death animation/visual | Character falls/disappears correctly | |
| 9.5 | Remote player respawns → Teleports to spawn | SetActorLocation with teleport, no walking from origin | |
| 9.6 | Player disconnects → Despawned from world | No ghost player left behind | |
| 9.7 | Enemy attacked by another player → HP visible | Health bar shows correct reduced HP | |

---

## 10. Enemy AI

| # | Test | Expected Result | Status |
|---|------|----------------|--------|
| 10.1 | Enemies spawn at correct locations | All 12 enemies visible at spawn positions on join | |
| 10.2 | Passive enemy wanders | Enemy moves within wander radius, returns if too far | |
| 10.3 | Buzzer (aggressive) chases player in range | Buzzer moves toward player when within 500 range | |
| 10.4 | Enemy respawns after death | After respawn timer, new enemy appears at spawn point | |
| 10.5 | Enemy attacks player in melee range | Player takes damage, HP bar decreases | |
| 10.6 | Hover over enemy → Hover indicator shows | WBP_HoverOverIndicator visible on hovered enemy | |

---

## 11. Regression Quick Test (Smoke Test — ~5 min)

Run after every feature change before committing:

1. [ ] Login → Enter World (auth + character select works)
2. [ ] Walk to an enemy → Kill it (movement + combat + loot works)
3. [ ] Open inventory → See item with icon → Right-click → Equip (inventory UI works)
4. [ ] Open stat window → Verify stats populated (stats system works)
5. [ ] Send a chat message → See it in chat (chat works)
6. [ ] Die + respawn (death flow works)

---

## Known Issues & Notes

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|
| Mosswort HP = 5 (likely typo, should be 50–80) | Low | Open | In ENEMY_TEMPLATES server code |
| `HoeveredTargetEnemyRef` typo in AC_TargetingSystem | Low | Open | Variable name typo, no functional impact |

---

## Related Files

| File | Purpose |
|------|---------|
| `docsNew/06_Reference/ID_Reference.md` | Item and enemy ID tables |
| `docsNew/03_Server_Side/Inventory_System.md` | Inventory socket events |
| `docsNew/03_Server_Side/Combat_System.md` | Combat formulas and events |
| `docsNew/02_Client_Side/Blueprints/07_Widget_Blueprints.md` | Widget documentation |
| `docsNew/05_Development/Troubleshooting.md` | Known bugs and fixes |

---

**Last Updated**: 2026-02-19  
**Version**: 1.0  
**Status**: Complete — covers all implemented features as of Phase D
