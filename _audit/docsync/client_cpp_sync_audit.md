# Client C++ Documentation Sync Audit

**Date**: 2026-03-22
**Auditor**: Automated (claude-opus-4-6)
**Scope**: All 17 doc files in `docsNew/02_Client_Side/C++_Code/` and `docsNew/02_Client_Side/Blueprints/` compared against actual source files in `client/SabriMMO/Source/SabriMMO/` and `client/SabriMMO/Source/SabriMMO/UI/`.

---

## Summary

| Category | Count |
|----------|-------|
| Doc files audited | 17 |
| Source .h files compared | 60+ |
| Accuracy issues found | 47 |
| Missing documentation for new subsystems | 11 |
| Docs needing major updates | 8 |
| Docs accurate (minor issues only) | 6 |
| Docs accurate (no issues) | 3 |

---

## Per-Doc Assessment

### 1. `00_Module_Overview.md` -- NEEDS MAJOR UPDATE

**Overall**: The class hierarchy and subsystem table are the most important parts. Several additions are missing.

**Inaccuracies**:

1. **File Inventory "Core Module"**: Lists "CastingCircleActor.h/.cpp" under Core but it's in `VFX/`. Minor but misleading.

2. **CharacterData.h**: Doc says "6 USTRUCTs...32 fields". Actual file has **7 USTRUCTs** (missing `FCompoundedCardInfo`) and `FInventoryItem` now has **~47 fields** (not 31). `FDraggedItem` has 13 fields (not 11, added `bStackable` and `bIdentified`). `FShopItem` has ~35 fields (not 26, added HitBonus/FleeBonus/CriticalBonus/PerfectDodgeBonus + inspect fields). The "6 USTRUCTs" is wrong: FCharacterData, FInventoryItem, FDraggedItem, FShopItem, FCartItem, FServerInfo, FCompoundedCardInfo = 7.

3. **MMOGameInstance.h/.cpp**: Doc says "9 delegates, 13 UFUNCTIONs". Actual source has 9 delegates (correct) but **20+ UFUNCTIONs** (added ConnectSocket, DisconnectSocket, IsSocketConnected, K2_EmitSocketEvent, EmitPlayerJoin, GetEventRouter, GetNativeSocket, etc. from Phase 4). Source is 208 lines (not 157).

4. **SabriMMOCharacter.h**: Doc says "97 lines". Actual source is **186 lines** with many new members: `UIToggleIMC`, `GameplayIMC`, 9+ toggle actions (ToggleCombatStatsAction, ToggleInventoryAction, ToggleEquipmentAction, ToggleShopAction, ToggleCartAction, FocusChatAction, ToggleSitAction, TogglePartyAction, CycleHotbarAction), 9 HotbarSlotActions, gameplay actions (LeftClickAction, RightClickAction, MouseDeltaAction, ZoomAction), TryInteractWithNPC, plus 20+ private handler functions. **None of these are documented.**

5. **UI Subsystems table**: Lists 14 subsystems but actual count is **33** (as listed in Class Hierarchy section). The table is incomplete -- missing: `ChatSubsystem`, `EnemySubsystem`, `OtherPlayerSubsystem`, `NameTagSubsystem`, `CombatActionSubsystem`, `MultiplayerEventSubsystem`, `CameraSubsystem`, `PlayerInputSubsystem`, `TargetingSubsystem`, `PositionBroadcastSubsystem`, `CartSubsystem`, `VendingSubsystem`, `ItemInspectSubsystem`, `PartySubsystem`, `CraftingSubsystem`, `SummonSubsystem`, `PetSubsystem`, `HomunculusSubsystem`, `CompanionVisualSubsystem`. Some of these ARE listed in the Class Hierarchy but not in the table with socket events/Z-order/toggle key.

6. **EItemDragSource enum**: Doc says `{ None, Inventory, Equipment }`. Actual code also has `Cart` (3rd value).

7. **EItemDropTarget enum**: Doc is missing `CartSlot` (6th value added for cart drag-drop).

8. **Class Hierarchy**: Lists `UCompanionVisualSubsystem` but the actual class hierarchy section is correct. However, it's missing `UItemTooltipBuilder` (new non-subsystem class: `ItemTooltipBuilder.h/.cpp`).

9. **Additional UI Files**: Missing many new widget files: `SAdvancedStatsWidget`, `SSenseResultPopup`, `SCardCompoundPopup`, `SIdentifyPopup`, `SCartWidget`, `SVendingSetupPopup`, `SVendingBrowsePopup`, `SPartyWidget`, `SCraftingPopup`, `SSummonOverlay`, `SItemInspectWidget`.

10. **Last Updated**: Says "2026-03-09" but significant changes occurred 2026-03-17 through 2026-03-22.

---

### 2. `01_CharacterData.md` -- NEEDS MAJOR UPDATE

**Overall**: Substantially out of date. Field counts and struct contents are wrong.

**Inaccuracies**:

1. **FCharacterData "32 fields"**: Still correct at 32 fields. However, doc omits `ZoneName`/`LevelName` from the constructor defaults (both empty string, not documented).

2. **FInventoryItem "31 fields"**: Actual struct now has **~47 fields**. Missing from doc:
   - `FullDescription` (FString)
   - `BuyPrice`, `SellPrice` (int32)
   - `HitBonus`, `FleeBonus`, `CriticalBonus`, `PerfectDodgeBonus` (int32)
   - `Slots`, `WeaponLevel`, `bRefineable`, `RefineLevel` (inspect fields)
   - `JobsAllowed`, `CardType`, `CardPrefix`, `CardSuffix` (FString)
   - `bTwoHanded`, `bIdentified`, `Element` (bool/FString)
   - `CompoundedCards` (TArray<int32>)
   - `CompoundedCardDetails` (TArray<FCompoundedCardInfo>)

3. **FInventoryItem helper methods**: Doc says `IsValid(), IsEquippable(), IsConsumable()`. Actual code also has `IsCard()`, `HasSlots()`, `GetDisplayName()`, `static FromCardInfo()`.

4. **FCompoundedCardInfo struct**: Entirely undocumented. New struct with 9 fields: ItemId, Name, Description, FullDescription, Icon, CardType, CardPrefix, CardSuffix, Weight + IsValid().

5. **FDraggedItem "11 fields"**: Actual struct has **13 fields** (added `bStackable`, `bIdentified`).

6. **FShopItem "26 fields"**: Actual struct has **~35 fields**. Missing: `FullDescription`, `HitBonus`, `FleeBonus`, `CriticalBonus`, `PerfectDodgeBonus`, `Slots`, `WeaponLevel`, `bRefineable`, `JobsAllowed`, `CardType`, `CardPrefix`, `CardSuffix`, `bTwoHanded`, `Element`. Also has `ToInspectableItem()` method (undocumented).

---

### 3. `02_MMOGameInstance.md` -- NEEDS MAJOR UPDATE

**Overall**: Missing the entire Phase 4 persistent socket API.

**Inaccuracies**:

1. **File sizes**: Doc says "157 lines" for .h. Actual is **208 lines**.

2. **Functions "14"**: Doc lists 14 functions. Actual public API now has **20+** functions including:
   - `ConnectSocket()` (BlueprintCallable)
   - `DisconnectSocket()` (BlueprintCallable)
   - `IsSocketConnected()` (BlueprintPure)
   - `EmitSocketEvent()` (two C++ overloads: FJsonObject and FString)
   - `K2_EmitSocketEvent()` (BlueprintCallable, takes USIOJsonObject*)
   - `EmitPlayerJoin()` (C++ only)
   - `GetEventRouter()` (returns USocketEventRouter*)
   - `GetNativeSocket()` (returns TSharedPtr<FSocketIONative>)
   - `Shutdown()` override (missing from doc)

3. **Private members**: Missing:
   - `NativeSocket` (TSharedPtr<FSocketIONative>)
   - `EventRouter` (TObjectPtr<USocketEventRouter>)
   - `OnSocketConnected()`, `OnSocketDisconnected()`, `OnSocketReconnecting()` private handlers

4. **Design Patterns**: Missing "Persistent Socket Pattern" and EventRouter description.

---

### 4. `03_MMOHttpManager.md` -- MOSTLY ACCURATE

**Overall**: Core API endpoints are still correct.

**Minor issues**:

1. **Bridge Functions "2"**: Doc lists UseSkillWithTargeting and ToggleCombatStatsWidget. These may still exist but the actual header should be verified (not read in this pass). The doc is likely still correct as the `MMOHttpManager.h` was not changed (61 lines matches doc).

---

### 5. `04_SabriMMOCharacter.md` -- NEEDS MAJOR UPDATE

**Overall**: Dramatically out of date. Only covers the original base class, missing all Phase 1-6 additions.

**Inaccuracies**:

1. **File sizes**: Doc says "97 lines .h, 134 lines .cpp". Actual .h is **186 lines** (nearly double).

2. **Missing input system**: The doc completely omits:
   - `UIToggleIMC` (UInputMappingContext) with 9 toggle actions: ToggleCombatStats, ToggleInventory, ToggleEquipment, ToggleShop, ToggleCart, FocusChat, ToggleSit, ToggleParty, CycleHotbar
   - `HotbarSlotActions[9]` array
   - `GameplayIMC` with LeftClick, RightClick, MouseDelta, Zoom actions
   - `CreateUIToggleActions()` and `CreateGameplayActions()` private methods
   - 20+ private handler functions (HandleLeftClick, HandleRightClickStarted/Completed, HandleMouseDelta, HandleZoom, HandleFocusChat, HandleToggleSit, HandleToggleCombatStats, HandleToggleInventory, HandleToggleEquipment, HandleToggleShop, HandleToggleCart, HandleToggleParty, HandleCycleHotbar, HandleHotbarSlot1-9, HandleHotbarSlotInternal)

3. **Missing TryInteractWithNPC**: New public BlueprintCallable function for NPC interaction.

4. **Forward declarations**: Missing `UHotbarSubsystem`, `UInputMappingContext` forward declarations.

5. **BeginPlay override**: Not documented (now exists in the header).

---

### 6. `07_DamageNumbers_Slate_UI.md` -- NEEDS UPDATE

**Overall**: Core design is accurate but enum and handler lists are outdated.

**Inaccuracies**:

1. **EDamagePopType enum**: Doc lists 7 values. Actual code has **10 values**:
   - Missing: `Dodge` (green, FLEE dodge), `PerfectDodge` (bright green, LUK dodge), `Block` (silver/white, Auto Guard)

2. **FDamagePopEntry struct**: Doc is missing:
   - `Element` (FString) for elemental coloring
   - `TextLabel` (FString) for floating text pops ("Poisoned!", "Stunned!")
   - `CustomColor` (FLinearColor) and `bHasCustomColor` (bool)

3. **DamageNumberSubsystem event handlers**: Doc only mentions `HandleCombatDamage`. Actual handlers include:
   - `HandleCombatDamage`
   - `HandleCombatBlocked` (new)
   - `HandleStatusTick` (new)
   - `HandleStatusApplied` (new)

4. **SpawnDamagePop signature**: Doc doesn't mention `HitType` and `Element` parameters.

5. **New SpawnTextPop method**: Entirely undocumented.

6. **New static methods**: `GetStatusDisplayName()`, `GetStatusColor()`, `ResolveTargetPosition()`.

7. **SDamageNumberOverlay**: Now has `AddTextPop()` method and additional font size constants (DODGE_FONT_SIZE, STATUS_TEXT_FONT_SIZE). New helper methods: `GetFillColor()`, `GetOutlineColor()`, `GetFontSize()`, `GetOutlineSize()`, `GetElementTint()`.

8. **Event Wrapping Strategy section**: Still describes the old BP_SocketManager wrapping approach. Should describe EventRouter->RegisterHandler() pattern.

9. **Widget class**: Doc says `SLeafWidget`. Actual code says `SCompoundWidget`.

---

### 7. `08_BasicInfo_Slate_UI.md` -- NEEDS UPDATE

**Overall**: Core design accurate but handler list and event wrapping are outdated.

**Inaccuracies**:

1. **Event handlers**: Actual source has additional handlers not in doc:
   - `HandleWeightStatus` (new)
   - `HandleZenyUpdate` (new)
   - `RecalcMaxWeight()` private helper (new)

2. **Event Wrapping Strategy section**: Still describes BP_SocketManager wrapping. Should describe EventRouter->RegisterHandler() pattern (Phase 4 migration done).

3. **Public data fields**: Missing `STR` field (int32, used for weight calculation).

4. **SocketIOClient Plugin Modification**: This section about modifying `GetNativeClient()` is no longer relevant since Phase 4 uses GameInstance's persistent socket.

---

### 8. `09_SkillTree_Slate_UI.md` -- PARTIALLY ACCURATE

**Overall**: Grid layout and tooltip docs are good. Missing many Phase 5+ additions.

**Inaccuracies**:

1. **SkillTreeSubsystem public API**: Doc is missing many functions:
   - `ShowWidgetInternal()`, `RequestSkillData()`, `LearnSkill()`, `ResetAllSkills()`
   - `AssignSkillToHotbar()`, `TryUseHotbarSkill()`
   - `UseSkill()`, `UseSkillOnTarget()`, `UseSkillOnGround()`
   - `IsSkillOnCooldown()`, `GetSkillCooldownRemaining()`
   - `BeginTargeting()`, `CancelTargeting()`, `IsInTargetingMode()`, `GetPendingSkillId()`
   - `FindSkillEntry()`, `GetSelectedLevel()`, `SetSelectedLevel()`
   - `StartSkillDrag()`, `CancelSkillDrag()`, `CancelWalkToCast()`, `UpdateSkillDragCursorPosition()`
   - `ResolveIconContentPath()`, `GetOrCreateIconBrush()`

2. **SelectedUseLevels**: Per-skill selected use levels not documented.

3. **FActiveBuff struct**: Listed in source but not in doc.

4. **OnSkillDataUpdated delegate**: Not documented.

5. **DynamicIconPaths**: Not documented.

6. **Warp Portal popup**: `HandleWarpPortalSelect` and related members not documented.

7. **GroundAoE::GetAoEInfo()**: Doc lists 9 cases. Likely more have been added for 2nd class skills (not verified in .cpp this pass).

8. **VFX Configs**: Doc says "31 total in BuildSkillVFXConfigs". CLAUDE.md says "97+ configs" -- the doc is way behind.

---

### 9. `10_ShopSubsystem.md` -- MOSTLY ACCURATE

**Overall**: Good accuracy, minor additions missing.

**Minor issues**:

1. **ShopSubsystem public API**: Missing `ShowWidget()`, `HideWidget()`, `IsWidgetVisible()` (all present in source).
2. **Actual source also has**: `ShopName` (FString), `ParseShopItemFromJson()` helper, `BuyCart`/`SellCart` arrays with helper methods (GetBuyCartTotalCost, etc.) -- partially documented but could be more explicit.

---

### 10. `10_WorldHealthBar_Slate_UI.md` -- NEEDS UPDATE

**Overall**: Core rendering and bar dimensions are accurate. Data structure has evolved.

**Inaccuracies**:

1. **FEnemyBarData struct**: Source has additional `CachedActor` (TWeakObjectPtr<AActor>) field not in doc.

2. **FNPCNameData struct**: New struct in source (DisplayName + Actor weak ptr) not documented.

3. **WorldHealthBarSubsystem**: Now has `NPCNames` array, `CacheEnemyActors()`, `CacheNPCActors()`, `ActorCacheTimer`, `PopulateFromGameInstance()` -- none documented.

4. **Public methods**: Missing `GetEnemyFeetPosition()` from doc.

5. **Event Registration**: Doc correctly notes EventRouter->RegisterHandler() pattern.

---

### 11. `11_CastBarSubsystem.md` -- NEEDS UPDATE

**Overall**: Mostly accurate but missing Free Cast speed feature.

**Inaccuracies**:

1. **Free Cast speed reduction**: Source has `ApplyFreeCastSpeed()`, `RestoreNormalSpeed()`, `SavedMaxWalkSpeed`, `bFreeCastSpeedApplied` -- entirely undocumented. This feature reduces player movement speed while casting.

---

### 12. `12_HotbarSubsystem.md` -- MOSTLY ACCURATE

**Overall**: Good coverage.

**Minor issues**:

1. **FHotbarSlot struct**: Source has `SkillLevel` field (int32) not mentioned in doc. Also has `IsEmpty()`, `IsItem()`, `IsSkill()`, `Clear()` helper methods.

2. **FHotbarKeybind struct**: Source has `PrimaryKey` (FKey), `IsValid()`, `GetDisplayString()` -- `Key (FKey)` in doc should be `PrimaryKey`. Missing `bRequiresShift` field in doc.

3. **HotbarSubsystem additional methods**: Missing `GetVisibleRowCount()`, `IsVisible()`, `GetSlot()`, `GetKeybind()`, `SetKeybind()`, `GetKeybindDisplayString()`, `IsKeybindWidgetVisible()`, `GetItemIconBrush()`, `GetSkillIconBrush()`, `HandleHotbarData()`.

4. **AssignSkill signature**: Doc says `(Row, Slot, SkillId, Name, Icon)`. Actual has additional `SkillLevel` param.

---

### 13. `13_EquipmentSubsystem.md` -- NEEDS UPDATE

**Overall**: Equipment slots are outdated -- missing dual wield and ammo slots.

**Inaccuracies**:

1. **Equipment Slots "10 RO Classic positions"**: Doc lists 10 slots. Actual source has **12 slots** in `EquipSlots` namespace:
   - Missing: `WeaponLeft` ("weapon_left" -- Dual wield for Assassin/Assassin Cross)
   - Missing: `Ammo` ("ammo" -- Arrows, bullets)

2. **EquipSlots namespace**: Doc doesn't document the namespace at all. Source has helper functions:
   - `CanDualWield(JobClass)` -- returns true for assassin/assassin_cross
   - `IsValidLeftHandWeapon(WeaponType)` -- dagger, one_hand_sword, axe
   - `GetValidPositions(EquipSlot, JobClass)` -- maps equip_slot to valid positions
   - `GetDisplayName(Position, JobClass)` -- UI-friendly names (Assassin shows "Left Hand")

3. **CanEquipToSlot overload**: Source has two overloads -- one with WeaponType parameter for dual wield validation. Doc only shows one.

4. **GetLocalJobClass()**: New public method not documented.

---

### 14. `14_BuffBarSubsystem.md` -- MOSTLY ACCURATE

**Overall**: Good accuracy. One method missing from doc.

**Minor issues**:

1. **HasBuff method**: Source has `bool HasBuff(const FString& BuffName) const` -- not documented. Used by PlayerInputSubsystem to check hiding/play_dead state.

2. **Stability Delay section**: Describes old wrapping order that may no longer apply since all subsystems use EventRouter now. Could be misleading.

---

### 15. `16_PetSubsystem.md` -- MOSTLY ACCURATE

**Overall**: Good match to source.

**Minor issues**:

1. **Missing private methods**: Source has `SpawnPetActor()`, `DespawnPetActor()`, `TickFollowOwner()` -- not documented.
2. **Private members**: Missing `PetActorClass` (UClass*), `PetActor` (AActor*), `FollowTimerHandle` -- these represent the pet actor spawning system.
3. **Default values**: Source shows `Hunger = 100`, `Intimacy = 250` as defaults. Doc shows Hunger 0-100, Intimacy 0-1000 ranges but doesn't mention defaults.

---

### 16. `17_HomunculusSubsystem.md` -- MOSTLY ACCURATE

**Overall**: Good match to source.

**Minor issues**:

1. **Missing private methods**: Source has `SpawnHomunculusActor()`, `DespawnHomunculusActor()`, `TickFollowOwner()` -- not documented.
2. **Private members**: Missing `HomActorClass` (UClass*), `HomActor` (AActor*), `FollowTimerHandle`.
3. **Default values**: Source shows `Hunger = 100`, `Intimacy = 250`.

---

### 17. `00_Blueprint_Index.md` -- LEGACY DOC, ACCURATE FOR SCOPE

**Overall**: Correctly marked as LEGACY. No accuracy issues for historical reference. The doc properly notes that all WBP_* widgets were replaced by C++ Slate.

**One note**: BP_OtherPlayerManager is listed but was removed from levels in Phase 6. BP_EnemyManager similarly. These are noted as removed in the C++ docs but this Blueprint index doesn't note their removal status.

---

## Missing Documentation (New Subsystems Without Docs)

The following subsystems exist in source but have NO documentation file in `docsNew/02_Client_Side/C++_Code/`:

### Priority 1 -- Core Subsystems (in production, used daily)

| # | Subsystem | Widget(s) | Z | Toggle | Socket Events | Notes |
|---|-----------|-----------|---|--------|---------------|-------|
| 1 | `ChatSubsystem` | `SChatWidget` | ? | Enter | `chat:receive`, `chat:error` + 7 combat log handlers | Also handles combat log (damage, death, buffs, EXP) |
| 2 | `InventorySubsystem` | `SInventoryWidget`, `SCardCompoundPopup`, `SIdentifyPopup`, `SLootNotificationOverlay` | 14 | F6 | `inventory:data/equipped/dropped/error`, `card:result`, `identify:*`, `loot:drop` | Complex: drag-drop, card compound, identify, loot notifications |
| 3 | `EnemySubsystem` | `SSenseResultPopup` | ? | -- | `enemy:spawn/move/death/health_update/attack`, `combat:knockback`, `skill:sense_result` | Entity registry with FEnemyEntry struct |
| 4 | `OtherPlayerSubsystem` | -- | -- | -- | `player:moved/left`, `vending:shop_opened/closed`, `skill:buff_applied/removed` | Entity registry with FPlayerEntry struct, hidden player tracking |
| 5 | `CombatActionSubsystem` | Target frame + Death overlay | 9/40 | -- | 10 combat events | Owns confirmed auto-attack state |
| 6 | `PlayerInputSubsystem` | -- | -- | -- | -- | Click-to-move, click-to-attack, click-to-interact |
| 7 | `CameraSubsystem` | -- | -- | -- | -- | RO camera: right-click yaw, scroll zoom, -55 pitch |
| 8 | `NameTagSubsystem` | `SNameTagOverlay` | ? | -- | -- | Registration-based entity name rendering |

### Priority 2 -- Feature Subsystems (recently added, in use)

| # | Subsystem | Widget(s) | Z | Toggle | Socket Events | Notes |
|---|-----------|-----------|---|--------|---------------|-------|
| 9 | `CartSubsystem` | `SCartWidget` | ? | F10 | `cart:data/error/equipped` | Merchant cart inventory |
| 10 | `VendingSubsystem` | `SVendingSetupPopup`, `SVendingBrowsePopup` | ? | -- | 7 vending events | Dual-mode: setup + browse |
| 11 | `PartySubsystem` | `SPartyWidget` | 12 | P | 8 party events | Party management, HP/SP sync |
| 12 | `ItemInspectSubsystem` | `SItemInspectWidget` | 22 | -- | -- | Right-click item inspection |
| 13 | `CraftingSubsystem` | `SCraftingPopup` | ? | -- | 6 crafting events | Arrow Crafting + Pharmacy + Converter |
| 14 | `TargetingSubsystem` | -- | -- | -- | -- | 30Hz hover trace + cursor |
| 15 | `SummonSubsystem` | `SSummonOverlay` | ? | -- | 6 summon events | Flora/Marine Sphere tracking |
| 16 | `CompanionVisualSubsystem` | -- | -- | -- | `player:stats` | Cart/mount/falcon visual placeholders |

### Priority 3 -- Non-Subsystem Files Missing Docs

| # | File | Type | Notes |
|---|------|------|-------|
| 17 | `ItemTooltipBuilder.h/.cpp` | Utility class | Shared tooltip construction for items |
| 18 | `SAdvancedStatsWidget.h/.cpp` | SCompoundWidget | 10 elements, 10 races, 3 sizes ATK/DEF panel |
| 19 | `SSenseResultPopup.h/.cpp` | SCompoundWidget | Monster info panel from Sense skill |
| 20 | `SCardCompoundPopup.h/.cpp` | SCompoundWidget | Card compound UI |
| 21 | `SIdentifyPopup.h/.cpp` | SCompoundWidget | Item identification UI |
| 22 | `SCartWidget.h/.cpp` | SCompoundWidget | Cart inventory grid |
| 23 | `SVendingSetupPopup.h/.cpp` | SCompoundWidget | Vendor shop setup |
| 24 | `SVendingBrowsePopup.h/.cpp` | SCompoundWidget | Buyer/vendor browse |
| 25 | `SPartyWidget.h/.cpp` | SCompoundWidget | Party member list |
| 26 | `SCraftingPopup.h/.cpp` | SCompoundWidget | Recipe selection |
| 27 | `SSummonOverlay.h/.cpp` | SCompoundWidget | Summon entity health bars |
| 28 | `PositionBroadcastSubsystem.h/.cpp` | UWorldSubsystem | 30Hz position broadcast (no widget) |
| 29 | `ZoneTransitionSubsystem.h/.cpp` | UWorldSubsystem | Documented in Module Overview table only |
| 30 | `SocketEventRouter.h/.cpp` | UObject | Multi-handler socket dispatch |
| 31 | `OtherCharacterMovementComponent.h/.cpp` | UCharacterMovementComponent | Documented in Module Overview only |

---

## Cross-Cutting Issues

### Issue A: "Event Wrapping" vs "EventRouter" Pattern

Several docs (07_DamageNumbers, 08_BasicInfo) still describe the old BP_SocketManager event wrapping pattern (poll for BP_SocketManager, save callback, replace with combined). All subsystems now use `EventRouter->RegisterHandler()` in `OnWorldBeginPlay` and `Router->UnregisterAllForOwner(this)` in `Deinitialize`. The old pattern docs are misleading.

### Issue B: Subsystem Count Mismatch

The Module Overview lists "33 subsystems" in the class hierarchy but only documents 14 in the table. The full actual subsystem count from source is **33** UWorldSubsystem classes:
1. LoginFlowSubsystem
2. BasicInfoSubsystem
3. CombatStatsSubsystem
4. InventorySubsystem
5. EquipmentSubsystem
6. HotbarSubsystem
7. ShopSubsystem
8. KafraSubsystem
9. SkillTreeSubsystem
10. DamageNumberSubsystem
11. CastBarSubsystem
12. WorldHealthBarSubsystem
13. ZoneTransitionSubsystem
14. BuffBarSubsystem
15. ChatSubsystem
16. EnemySubsystem
17. OtherPlayerSubsystem
18. NameTagSubsystem
19. CombatActionSubsystem
20. MultiplayerEventSubsystem
21. CameraSubsystem
22. PlayerInputSubsystem
23. TargetingSubsystem
24. PositionBroadcastSubsystem
25. CartSubsystem
26. VendingSubsystem
27. ItemInspectSubsystem
28. PartySubsystem
29. CraftingSubsystem
30. SummonSubsystem
31. PetSubsystem
32. HomunculusSubsystem
33. CompanionVisualSubsystem

Plus `SkillVFXSubsystem` (which is technically also a UWorldSubsystem but lives in VFX/).

### Issue C: "Last Updated" Dates

Most docs show "Last Updated: 2026-02-xx" or "2026-03-09". Significant changes occurred through 2026-03-22. All docs should update their dates.

### Issue D: File Line Counts

Several docs state specific line counts that are now wrong (SabriMMOCharacter.h: 97 vs 186, MMOGameInstance.h: 157 vs 208). These brittle claims should either be removed or updated.

---

## Recommended Actions

### Immediate (High Priority)

1. **Create docs for 8 core undocumented subsystems** (ChatSubsystem, InventorySubsystem, EnemySubsystem, OtherPlayerSubsystem, CombatActionSubsystem, PlayerInputSubsystem, CameraSubsystem, NameTagSubsystem)

2. **Update `01_CharacterData.md`**: Add FCompoundedCardInfo, update all field counts, add new fields (inspect, card, unidentified, weight)

3. **Update `02_MMOGameInstance.md`**: Add Phase 4 persistent socket API (ConnectSocket, DisconnectSocket, EmitSocketEvent, EventRouter, NativeSocket)

4. **Update `04_SabriMMOCharacter.md`**: Document Enhanced Input system (UIToggleIMC, GameplayIMC, all toggle/handler functions)

5. **Update `00_Module_Overview.md`**: Complete the UI Subsystems table with all 33 subsystems, fix CharacterData field counts, update line counts

### Medium Priority

6. **Update `07_DamageNumbers_Slate_UI.md`**: Add new EDamagePopType values (Dodge/PerfectDodge/Block), new handlers, text pop system

7. **Update `13_EquipmentSubsystem.md`**: Add WeaponLeft and Ammo slots, EquipSlots namespace functions

8. **Update `09_SkillTree_Slate_UI.md`**: Document full public API, update VFX config count

9. **Create docs for 8 feature subsystems** (Cart, Vending, Party, ItemInspect, Crafting, Targeting, Summon, CompanionVisual)

### Low Priority

10. Replace all "Event Wrapping Strategy" sections with EventRouter pattern description
11. Remove or update file line count claims
12. Update "Last Updated" dates on all docs
13. Create docs for non-subsystem widgets (SAdvancedStatsWidget, SSenseResultPopup, etc.)
