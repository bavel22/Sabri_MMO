#  Summary of what you can delete from the Content Browser:

 Summary of what you can delete from the Content Browser:

  9 Blueprint assets (entire files):
  - Components/AC_HUDManager (remove from BP_MMOCharacter first)
  - Components/AC_TargetingSystem
  - Components/AC_CameraController
  - Blueprints/BP_SocketManager
  - Blueprints/BP_OtherPlayerManager
  - Blueprints/BP_EnemyManager
  - Blueprints/BP_GameFlow
  - Blueprints/BP_InventoryDragDrop
  - Blueprints/BP_RemoteCharacter

  23 UMG widgets (entire files):
  - All WBP_* widgets listed above

  One manual step:
  - Open BP_MMOCharacter → delete HUDManager component → Compile & Save (do this before deleting AC_HUDManager asset)

  That's 32 dead assets you can remove. Everything is handled by C++ Slate now.

✻ Cogitated for 1m 50s

❯ is there anymore cleanup i can do in my content browser?

● Search(pattern: "**/*.uasset", path: "client\SabriMMO\Content\SabriMMO")
  ⎿  Found 100 files (ctrl+o to expand)

● Yes, a few more things:

  Dead Input Actions (removed from IMC_MMOCharacter in Phase 1 — C++ handles all input now):
  - Input/IA_ClickToMove — replaced by PlayerInputSubsystem
  - Input/IA_Attack — replaced by PlayerInputSubsystem
  - Input/IA_CameraRotate — replaced by CameraSubsystem
  - Input/IA_Look — replaced by CameraSubsystem
  - Input/IA_Zoom — replaced by CameraSubsystem

  Keep: IA_Move (still used), IA_ToggleInventory, IA_OpenStats, IA_ToggleSkillTree, IA_Modifier_Alt, IA_Hotbar_1-9

  Obsolete skill icons (already in _Obsolete folder — replaced by generated icons):
  - Delete the entire Assets/Skill_Icons/_Obsolete/ folder (16 old icons)

  Possibly dead data tables (C++ loads icons directly from content paths, not via DataTable):
  - Data/DT_ItemIcons + Data/S_ItemIconRow
  - Data/DT_SkillIcons + Data/S_SkillIconRow

  Possibly dead Blueprints (verify before deleting):
  - BP_FlowController — couldn't load, likely broken/orphaned
  - BP_MMOPlayerController — there's also PC_MMOPlayerController (the active one). Check if BP_MMOPlayerController is
  used anywhere
  - BP_ThirdPersonPlayerController — UE5 template default, likely unused
  - BP_ThirdPersonGameMode — UE5 template default, likely unused
  - FPlayerJoinData (structure) — player:join handled by C++ GameInstance now
  - BPI_GameAction (interface) — check if any BP still implements it

  Keep (parent class):
  - BP_ThirdPersonCharacter — BP_MMOCharacter inherits from it, do not delete

  Optional (test helpers):
  - BP_AuthTester, BP_CharacterTester, BP_NetworkTester — not used in gameplay, but useful for debugging. Your call.

  Entire Blueprint Assets — Safe to DELETE

  ┌───────────────────────┬─────────────────────────────────────────────────────┬───────────────────────────────────┐
  │         Asset         │                       Reason                        │            Replaced By            │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ BP_SocketManager      │ All functions deleted, SocketIO disconnected        │ C++ subsystems via GameInstance   │
  │                       │                                                     │ socket                            │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ AC_HUDManager         │ All 42 functions replaced                           │ 15+ C++ Slate subsystems          │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ AC_TargetingSystem    │ Hover detection, range checks                       │ TargetingSubsystem (Phase 2)      │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ AC_CameraController   │ Camera rotation, zoom                               │ CameraSubsystem (Phase 1)         │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ BP_OtherPlayerManager │ Player spawn/remove/get                             │ OtherPlayerSubsystem (Phase 3)    │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ BP_EnemyManager       │ Enemy spawn/remove/get                              │ EnemySubsystem (Phase 3)          │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ BP_GameFlow           │ Login/char select flow (74 nodes)                   │ LoginFlowSubsystem                │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ BP_InventoryDragDrop  │ Empty DragDropOperation                             │ InventorySubsystem drag state     │
  ├───────────────────────┼─────────────────────────────────────────────────────┼───────────────────────────────────┤
  │ BP_RemoteCharacter    │ Old remote player (unused — BP_OtherPlayerCharacter │ OtherPlayerSubsystem              │
  │                       │  is active)                                         │                                   │
  └───────────────────────┴─────────────────────────────────────────────────────┴───────────────────────────────────┘

  UMG Widgets — Safe to DELETE

  ┌─────────────────────┬─────────────────────────────────────────────────┐
  │       Widget        │                   Replaced By                   │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_GameHUD         │ BasicInfoSubsystem (Z=10)                       │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_ChatWidget      │ ChatSubsystem (Z=13)                            │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_ChatMessageLine │ ChatSubsystem (inline STextBlock)               │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_LootPopup       │ SLootNotificationOverlay (Z=17)                 │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_LootItemLine    │ SLootNotificationOverlay                        │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_DamageNumber    │ DamageNumberSubsystem (Z=20)                    │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_DeathOverlay    │ CombatActionSubsystem (Z=40)                    │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_InventoryWindow │ InventorySubsystem (Z=14)                       │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_InventorySlot   │ SInventoryWidget                                │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_ContextMenu     │ ItemInspectSubsystem (Z=22)                     │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_DragVisual      │ InventorySubsystem drag cursor                  │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_ItemTooltip     │ ItemTooltipBuilder                              │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_Hotbar          │ HotbarSubsystem (Z=16)                          │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_HotbarSlot      │ SHotbarRowWidget                                │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_Shop            │ ShopSubsystem (Z=18)                            │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_ShopItem        │ SShopWidget                                     │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_ShopSellSlot    │ SShopWidget                                     │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_StatAllocation  │ CombatStatsSubsystem (Z=12)                     │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_TargetHealthBar │ CombatActionSubsystem + WorldHealthBarSubsystem │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_LoginScreen     │ LoginFlowSubsystem (SLoginWidget)               │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_CharacterSelect │ LoginFlowSubsystem (SCharacterSelectWidget)     │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_CharacterEntry  │ SCharacterSelectWidget                          │
  ├─────────────────────┼─────────────────────────────────────────────────┤
  │ WBP_CreateCharacter │ LoginFlowSubsystem (SCharacterCreateWidget)     │
  └─────────────────────┴─────────────────────────────────────────────────┘

  KEEP — Still Active

  ┌─────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────┐
  │            Asset            │                                        Why                                        │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ BP_MMOCharacter             │ PlayAttackAnimation (called via ProcessEvent), SpringArm, Camera, BPI_Damageable. │
  │                             │  Remove HUDManager component                                                      │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ BP_OtherPlayerCharacter     │ PlayAttackAnimation, HoverOverIndicator (TargetingSubsystem), BPI_Damageable.     │
  │                             │ UpdateHealthBar is dead — can delete that function                                │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ BP_EnemyCharacter           │ PlayAttackAnimation, InitializeEnemy, OnEnemyDeath, UpdateEnemyHealth,            │
  │                             │ HoverOverIndicator. All still used by EnemySubsystem via ProcessEvent             │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ BP_MMOGameMode /            │ Game mode setup                                                                   │
  │ GM_MMOGameMode              │                                                                                   │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ PC_MMOPlayerController      │ Active player controller                                                          │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ BP_NPCShop                  │ NPC shop actor                                                                    │
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────┤
  │ WBP_HoverIndicator          │ Used by WidgetComponents on characters for hover effect                           │
  └─────────────────────────────┴───────────────────────────────────────────────────────────────────────────────────┘

  Let me clean up the one dead function on BP_OtherPlayerCharacter:

● unrealMCP - delete_function (MCP)(blueprint_name: "/Game/SabriMMO/Blueprints/BP_OtherPlayerCharacter", function_name:
                                   "UpdateHealthBar")
  ⎿  {
       "status": "success",
       "result": {
         "success": true,
         "function_name": "UpdateHealthBar"
       }
     }

● Done. Deleted UpdateHealthBar from BP_OtherPlayerCharacter.

  Summary of what you can delete from the Content Browser:

  9 Blueprint assets (entire files):
  - Components/AC_HUDManager (remove from BP_MMOCharacter first)
  - Components/AC_TargetingSystem
  - Components/AC_CameraController
  - Blueprints/BP_SocketManager
  - Blueprints/BP_OtherPlayerManager
  - Blueprints/BP_EnemyManager
  - Blueprints/BP_GameFlow
  - Blueprints/BP_InventoryDragDrop
  - Blueprints/BP_RemoteCharacter

  23 UMG widgets (entire files):
  - All WBP_* widgets listed above

  One manual step:
  - Open BP_MMOCharacter → delete HUDManager component → Compile & Save (do this before deleting AC_HUDManager asset)

  That's 32 dead assets you can remove. Everything is handled by C++ Slate now.

✻ Cogitated for 1m 50s

❯ is there anymore cleanup i can do in my content browser?

● Search(pattern: "**/*.uasset", path: "client\SabriMMO\Content\SabriMMO")
  ⎿  Found 100 files (ctrl+o to expand)

● Yes, a few more things:

  Dead Input Actions (removed from IMC_MMOCharacter in Phase 1 — C++ handles all input now):
  - Input/IA_ClickToMove — replaced by PlayerInputSubsystem
  - Input/IA_Attack — replaced by PlayerInputSubsystem
  - Input/IA_CameraRotate — replaced by CameraSubsystem
  - Input/IA_Look — replaced by CameraSubsystem
  - Input/IA_Zoom — replaced by CameraSubsystem

  Keep: IA_Move (still used), IA_ToggleInventory, IA_OpenStats, IA_ToggleSkillTree, IA_Modifier_Alt, IA_Hotbar_1-9

  Obsolete skill icons (already in _Obsolete folder — replaced by generated icons):
  - Delete the entire Assets/Skill_Icons/_Obsolete/ folder (16 old icons)

  Possibly dead data tables (C++ loads icons directly from content paths, not via DataTable):
  - Data/DT_ItemIcons + Data/S_ItemIconRow
  - Data/DT_SkillIcons + Data/S_SkillIconRow

  Possibly dead Blueprints (verify before deleting):
  - BP_FlowController — couldn't load, likely broken/orphaned
  - BP_MMOPlayerController — there's also PC_MMOPlayerController (the active one). Check if BP_MMOPlayerController is
  used anywhere
  - BP_ThirdPersonPlayerController — UE5 template default, likely unused
  - BP_ThirdPersonGameMode — UE5 template default, likely unused
  - FPlayerJoinData (structure) — player:join handled by C++ GameInstance now
  - BPI_GameAction (interface) — check if any BP still implements it

  Keep (parent class):
  - BP_ThirdPersonCharacter — BP_MMOCharacter inherits from it, do not delete

  Optional (test helpers):
  - BP_AuthTester, BP_CharacterTester, BP_NetworkTester — not used in gameplay, but useful for debugging. Your call.






---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
