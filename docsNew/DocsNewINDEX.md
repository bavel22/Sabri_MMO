# Sabri_MMO Documentation Index

> Master navigation for all project documentation. Last updated: 2026-03-20.

---

## Quick Links

- [Dashboard](../_journal/Dashboard.md) — Current stats, what's done, what's next
- [Project Overview](00_Project_Overview.md) — Full system inventory, tech stack, all implemented features
- [README / Quick Start](high-level%20docs/README.md) — Setup instructions, project structure
- [Prompt Library](../_prompts/README.md) — Reusable Claude Code prompts that work
- [Global Rules](00_Global_Rules/Global_Rules.md) — Design standards, coding rules, naming conventions
- [RUNNING Guide](high-level%20docs/RUNNING.md) — Step-by-step server/client startup

---

## 00 — Global Rules & Overview

| Document | Description |
|----------|-------------|
| [00_Project_Overview.md](00_Project_Overview.md) | Executive summary, tech stack, all implemented features, directory structure |
| [Global_Rules.md](00_Global_Rules/Global_Rules.md) | Design standards, naming conventions, architecture mandates |
| [unrealMCP_Mandate.md](00_Global_Rules/unrealMCP_Mandate.md) | Blueprint workflow: always read via unrealMCP before modifying |

---

## 01 — Architecture

| Document | Description | Related |
|----------|-------------|---------|
| [System_Architecture.md](01_Architecture/System_Architecture.md) | Three-tier architecture (UE5 ↔ Node.js ↔ PostgreSQL/Redis), server authority | [Multiplayer_Architecture](01_Architecture/Multiplayer_Architecture.md), [Networking_Protocol](04_Integration/Networking_Protocol.md) |
| [Multiplayer_Architecture.md](01_Architecture/Multiplayer_Architecture.md) | Socket.io event flow, persistent socket, zone-scoped broadcasting | [System_Architecture](01_Architecture/System_Architecture.md), [Networking_Protocol](04_Integration/Networking_Protocol.md) |
| [Database_Architecture.md](01_Architecture/Database_Architecture.md) | PostgreSQL schema (users, characters, items, inventory, hotbar, cart, parties, homunculus, vending), Redis caching | [Inventory_System](03_Server_Side/Inventory_System.md), [NodeJS_Server](03_Server_Side/NodeJS_Server.md) |

---

## 02 — Client Side

### C++ Code Documentation

| Document | Description | Source Files |
|----------|-------------|-------------|
| [00_Module_Overview.md](02_Client_Side/C++_Code/00_Module_Overview.md) | Module structure, build dependencies, file inventory | `SabriMMO.Build.cs`, `SabriMMO.h` |
| [01_CharacterData.md](02_Client_Side/C++_Code/01_CharacterData.md) | `FCharacterData`, `FServerInfo`, `FInventoryItem` structs | `CharacterData.h` |
| [02_MMOGameInstance.md](02_Client_Side/C++_Code/02_MMOGameInstance.md) | Auth state, persistent socket, character data, EventRouter | `MMOGameInstance.h/cpp` |
| [03_MMOHttpManager.md](02_Client_Side/C++_Code/03_MMOHttpManager.md) | REST API client (BlueprintFunctionLibrary) | `MMOHttpManager.h/cpp` |
| [04_SabriMMOCharacter.md](02_Client_Side/C++_Code/04_SabriMMOCharacter.md) | Local player pawn — movement, stats, BeginPlay | `SabriMMOCharacter.h/cpp` |
| [05_SabriMMOPlayerController.md](02_Client_Side/C++_Code/05_SabriMMOPlayerController.md) | Input mapping (click-to-move + WASD) | `SabriMMOPlayerController.h/cpp` |
| [06_CombatVariant.md](02_Client_Side/C++_Code/06_CombatVariant.md) | Variant_Combat system (standalone combat prototype) | `Variant_Combat/` |
| [07_DamageNumbers_Slate_UI.md](02_Client_Side/C++_Code/07_DamageNumbers_Slate_UI.md) | `DamageNumberSubsystem` + `SDamageNumberOverlay` | `UI/DamageNumberSubsystem.*`, `UI/SDamageNumberOverlay.*` |
| [08_BasicInfo_Slate_UI.md](02_Client_Side/C++_Code/08_BasicInfo_Slate_UI.md) | `BasicInfoSubsystem` + `SBasicInfoWidget` (HP/SP/EXP panel) | `UI/BasicInfoSubsystem.*`, `UI/SBasicInfoWidget.*` |
| [09_SkillTree_Slate_UI.md](02_Client_Side/C++_Code/09_SkillTree_Slate_UI.md) | `SkillTreeSubsystem` + `SSkillTreeWidget` | `UI/SkillTreeSubsystem.*`, `UI/SSkillTreeWidget.*` |
| [10_ShopSubsystem.md](02_Client_Side/C++_Code/10_ShopSubsystem.md) | `ShopSubsystem` + `SShopWidget` (NPC shops) | `UI/ShopSubsystem.*`, `UI/SShopWidget.*` |
| [10_WorldHealthBar_Slate_UI.md](02_Client_Side/C++_Code/10_WorldHealthBar_Slate_UI.md) | `WorldHealthBarSubsystem` + `SWorldHealthBarOverlay` | `UI/WorldHealthBarSubsystem.*`, `UI/SWorldHealthBarOverlay.*` |
| [11_CastBarSubsystem.md](02_Client_Side/C++_Code/11_CastBarSubsystem.md) | `CastBarSubsystem` + `SCastBarOverlay` (cast time bars) | `UI/CastBarSubsystem.*`, `UI/SCastBarOverlay.*` |
| [12_HotbarSubsystem.md](02_Client_Side/C++_Code/12_HotbarSubsystem.md) | `HotbarSubsystem` + `SHotbarRowWidget` (4-row skill hotbar) | `UI/HotbarSubsystem.*`, `UI/SHotbarRowWidget.*` |
| [13_EquipmentSubsystem.md](02_Client_Side/C++_Code/13_EquipmentSubsystem.md) | `EquipmentSubsystem` + `SEquipmentWidget` | `UI/EquipmentSubsystem.*`, `UI/SEquipmentWidget.*` |
| [14_BuffBarSubsystem.md](02_Client_Side/C++_Code/14_BuffBarSubsystem.md) | `BuffBarSubsystem` + `SBuffBarWidget` (buff icons + timers) | `UI/BuffBarSubsystem.*`, `UI/SBuffBarWidget.*` |
| [16_PetSubsystem.md](02_Client_Side/C++_Code/16_PetSubsystem.md) | `PetSubsystem` (pet taming, feeding, commands) | `UI/PetSubsystem.*` |
| [17_HomunculusSubsystem.md](02_Client_Side/C++_Code/17_HomunculusSubsystem.md) | `HomunculusSubsystem` (homunculus management) | `UI/HomunculusSubsystem.*` |

#### Undocumented Subsystems (source exists, no dedicated doc yet)

| Subsystem | Source Files | Role |
|-----------|-------------|------|
| `LoginFlowSubsystem` | `UI/LoginFlowSubsystem.*`, `UI/SLoginWidget.*` | Auth flow state machine, 5 login screens |
| `InventorySubsystem` | `UI/InventorySubsystem.*`, `UI/SInventoryWidget.*` | Item grid, drag-drop, equip/use/drop |
| `CombatStatsSubsystem` | `UI/CombatStatsSubsystem.*`, `UI/SCombatStatsWidget.*`, `UI/SAdvancedStatsWidget.*` | ATK/DEF/HIT/FLEE stats panel + advanced element/race/size details |
| `ChatSubsystem` | `UI/ChatSubsystem.*` | Chat window, 3 tabs, combat log formatting |
| `EnemySubsystem` | `UI/EnemySubsystem.*`, `UI/SSenseResultPopup.*` | Enemy actor registry, 5 event handlers, Sense popup |
| `OtherPlayerSubsystem` | `UI/OtherPlayerSubsystem.*` | Other player actor registry, 2 event handlers |
| `NameTagSubsystem` | `UI/NameTagSubsystem.*` | Single OnPaint overlay for all entity name tags |
| `CombatActionSubsystem` | `UI/CombatActionSubsystem.*` | 10 combat event handlers, target frame, death overlay |
| `MultiplayerEventSubsystem` | `UI/MultiplayerEventSubsystem.*` | Outbound emit helpers (0 bridges) |
| `CameraSubsystem` | `UI/CameraSubsystem.*` | Right-click yaw rotation, scroll zoom |
| `PlayerInputSubsystem` | `UI/PlayerInputSubsystem.*` | Click-to-move/attack/interact |
| `TargetingSubsystem` | `UI/TargetingSubsystem.*` | 30Hz hover trace, cursor switching |
| `PositionBroadcastSubsystem` | `UI/PositionBroadcastSubsystem.*` | 30Hz position broadcasting |
| `ZoneTransitionSubsystem` | `UI/ZoneTransitionSubsystem.*` | Zone change/teleport handling |
| `CartSubsystem` | `UI/CartSubsystem.*`, `UI/SCartWidget.*` | Cart inventory management |
| `VendingSubsystem` | `UI/VendingSubsystem.*`, `UI/SVendingSetupPopup.*`, `UI/SVendingBrowsePopup.*` | Vending shop system |
| `ItemInspectSubsystem` | `UI/ItemInspectSubsystem.*`, `UI/SItemInspectWidget.*` | Item inspection detail panel |
| `PartySubsystem` | `UI/PartySubsystem.*`, `UI/SPartyWidget.*` | Party management + HP bars |
| `PlayerInputSubsystem` (context menu) | `UI/PlayerInputSubsystem.*` | Right-click player context menu (Trade/Party Invite/Whisper) |
| `CraftingSubsystem` | `UI/CraftingSubsystem.*`, `UI/SCraftingPopup.*` | Pharmacy/crafting UI |
| `SummonSubsystem` | `UI/SummonSubsystem.*`, `UI/SSummonOverlay.*` | Summon Flora/Marine Sphere overlay |
| `CompanionVisualSubsystem` | `UI/CompanionVisualSubsystem.*` | Pet/homunculus/mount visual actors |
| `KafraSubsystem` | `UI/KafraSubsystem.*`, `UI/SKafraWidget.*` | Kafra NPC service dialog |

#### Other Client Files

| File | Role |
|------|------|
| `SocketEventRouter.*` | Multi-handler Socket.io dispatch for subsystems |
| `OtherCharacterMovementComponent.*` | Remote player interpolation + floor-snap |
| `CompanionVisualActor.*` | Visual actor for pets/homunculus/mounts |
| `ShopNPC.*`, `KafraNPC.*`, `WarpPortal.*` | Interactable world actors |
| `ItemTooltipBuilder.*` | Shared tooltip construction utility |
| `SkillDragDropOperation.*` | Skill drag-drop between hotbar/skill tree |
| `SSkillTargetingOverlay.*` | Click-to-cast targeting overlay |
| `SIdentifyPopup.*` | Item appraisal UI |
| `SCardCompoundPopup.*` | Card compound UI |
| `VFX/SkillVFXSubsystem.*` | 97+ skill VFX configs (Niagara) |
| `VFX/SkillVFXData.*` | VFX config structs |
| `VFX/CastingCircleActor.*` | Casting circle ground effect |

### Blueprint Documentation (Legacy)

> These docs cover Blueprint assets that have largely been **replaced by C++ Slate subsystems**. They are kept for historical reference but may not reflect current functionality.

| Document | Status |
|----------|--------|
| [00_Blueprint_Index.md](02_Client_Side/Blueprints/00_Blueprint_Index.md) | Legacy — most BPs replaced by C++ |
| [01_BP_GameFlow.md](02_Client_Side/Blueprints/01_BP_GameFlow.md) | **Replaced** by `LoginFlowSubsystem` (C++) |
| [02_BP_SocketManager.md](02_Client_Side/Blueprints/02_BP_SocketManager.md) | **Replaced** by persistent socket on `MMOGameInstance` |
| [03_BP_MMOCharacter.md](02_Client_Side/Blueprints/03_BP_MMOCharacter.md) | Still active (player pawn Blueprint) |
| [04_BP_OtherPlayerManager.md](02_Client_Side/Blueprints/04_BP_OtherPlayerManager_and_Character.md) | **Replaced** by `OtherPlayerSubsystem` (C++) |
| [05_BP_EnemyManager.md](02_Client_Side/Blueprints/05_BP_EnemyManager_and_Character.md) | **Replaced** by `EnemySubsystem` (C++) |
| [06_Actor_Components.md](02_Client_Side/Blueprints/06_Actor_Components.md) | **Replaced** — AC_HUDManager, AC_TargetingSystem, AC_CameraController all replaced by C++ subsystems |
| [07_Widget_Blueprints.md](02_Client_Side/Blueprints/07_Widget_Blueprints.md) | **Replaced** — all 23 WBP_* widgets replaced by C++ Slate |
| [08_BPI_Damageable.md](02_Client_Side/Blueprints/08_BPI_Damageable_Interface.md) | Still relevant (Blueprint interfaces) |
| [09_Blueprint_Cross_Reference.md](02_Client_Side/Blueprints/09_Blueprint_Cross_Reference.md) | Partially outdated |

---

## 03 — Server Side

| Document | Description | Related |
|----------|-------------|---------|
| [NodeJS_Server.md](03_Server_Side/NodeJS_Server.md) | Server overview (~32,200 lines), key sections, data modules | [System_Architecture](01_Architecture/System_Architecture.md) |
| [Combat_System.md](03_Server_Side/Combat_System.md) | Auto-attack loop, damage pipeline, ASPD, element/size tables | [Skill_System](03_Server_Side/Skill_System.md), [Status_Effect_Buff_System](03_Server_Side/Status_Effect_Buff_System.md) |
| [Skill_System.md](03_Server_Side/Skill_System.md) | 293 skill definitions, handlers, cast times, cooldowns | [Combat_System](03_Server_Side/Combat_System.md), class research docs in [05_Development/](05_Development/) |
| [Enemy_System.md](03_Server_Side/Enemy_System.md) | 509 monster templates, AI state machine, spawning | [Monster_Skill_System](03_Server_Side/Monster_Skill_System.md) |
| [Monster_Skill_System.md](03_Server_Side/Monster_Skill_System.md) | 40+ NPC_ skills, summoning, metamorphosis, Plagiarism integration | [Enemy_System](03_Server_Side/Enemy_System.md) |
| [Status_Effect_Buff_System.md](03_Server_Side/Status_Effect_Buff_System.md) | 95 buff types, 10 status effects, modifier merging | [Combat_System](03_Server_Side/Combat_System.md), [Skill_System](03_Server_Side/Skill_System.md) |
| [EXP_Leveling_System.md](03_Server_Side/EXP_Leveling_System.md) | EXP tables, base/job leveling, death penalty | — |
| [Inventory_System.md](03_Server_Side/Inventory_System.md) | Items, equipment, weight thresholds, card compounds | [Card_System](03_Server_Side/Card_System.md), [Database_Architecture](01_Architecture/Database_Architecture.md) |
| [Card_System.md](03_Server_Side/Card_System.md) | 538 cards, compound system, bonus aggregation, combat hooks | [Inventory_System](03_Server_Side/Inventory_System.md), [Combat_System](03_Server_Side/Combat_System.md) |
| [Pet_System.md](03_Server_Side/Pet_System.md) | Pet taming, feeding, loyalty, stat bonuses | — |
| [API_Documentation.md](03_Server_Side/API_Documentation.md) | REST API endpoints (11 routes) | [Authentication_Flow](04_Integration/Authentication_Flow.md), [API_Reference](06_Reference/API_Reference.md) |

---

## 04 — Integration

| Document | Description | Related |
|----------|-------------|---------|
| [Authentication_Flow.md](04_Integration/Authentication_Flow.md) | JWT login/register, token validation, socket auth | [API_Documentation](03_Server_Side/API_Documentation.md), [LoginFlowSubsystem](02_Client_Side/C++_Code/02_MMOGameInstance.md) |
| [Networking_Protocol.md](04_Integration/Networking_Protocol.md) | 79 socket events, JSON formats, REST endpoints | [Event_Reference](06_Reference/Event_Reference.md), [Multiplayer_Architecture](01_Architecture/Multiplayer_Architecture.md) |

---

## 05 — Development Plans & Audits

### Strategic Plans

| Document | Status | Description |
|----------|--------|-------------|
| [Strategic_Implementation_Plan_v2.md](05_Development/Strategic_Implementation_Plan_v2.md) | COMPLETED | Original phases 1-7 |
| [Strategic_Implementation_Plan_v3.md](05_Development/Strategic_Implementation_Plan_v3.md) | COMPLETED | Updated plan with second classes |
| [Stategic_Implementation_plan_v3_phase5.md](05_Development/Stategic_Implementation_plan_v3_phase5_details_first_class_skills.md) | COMPLETED | First class skill details |

### Blueprint-to-C++ Migration (COMPLETED)

| Document | Status |
|----------|--------|
| [00_Master_Migration_Plan.md](05_Development/Blueprint_To_CPP_Migration/00_Master_Migration_Plan.md) | COMPLETED |
| [01_Local_Player_Input_Movement.md](05_Development/Blueprint_To_CPP_Migration/01_Local_Player_Input_Movement_Plan.md) | COMPLETED (Phase 1) |
| [02_Targeting_Hover_Combat.md](05_Development/Blueprint_To_CPP_Migration/02_Targeting_Hover_Combat_Plan.md) | COMPLETED (Phase 2) |
| [03_Entity_Management.md](05_Development/Blueprint_To_CPP_Migration/03_Entity_Management_Plan.md) | COMPLETED (Phase 3) |
| [04_Name_Tags_Interaction.md](05_Development/Blueprint_To_CPP_Migration/04_Name_Tags_Interaction_Plan.md) | COMPLETED (Phase 3) |
| [05_Socket_Event_Migration.md](05_Development/Blueprint_To_CPP_Migration/05_Socket_Event_Migration_Plan.md) | COMPLETED (Phase 4) |
| [06_RO_Classic_Gameplay_Reference.md](05_Development/Blueprint_To_CPP_Migration/06_RO_Classic_Gameplay_Reference.md) | Reference doc |
| [07_Audit_Report_And_Fixes.md](05_Development/Blueprint_To_CPP_Migration/07_Audit_Report_And_Fixes.md) | COMPLETED |
| [08_Migration_Status.md](05_Development/Blueprint_To_CPP_Migration/08_Migration_Status_And_Next_Phase.md) | Final status report |
| [BP_Bridge_Migration_Plan.md](05_Development/BP_Bridge_Migration_Plan.md) | COMPLETED — all bridges removed |

### System Implementation Plans

| Document | Status | Description |
|----------|--------|-------------|
| [Persistent_Socket_Connection_Plan.md](05_Development/Persistent_Socket_Connection_Plan.md) | COMPLETED (v1) | Original persistent socket plan |
| [Persistent_Socket_Connection_Plan_v2.md](05_Development/Persistent_Socket_Connection_Plan_v2.md) | COMPLETED | Refined persistent socket with EventRouter |
| [Dual_Wield_System_Plan.md](05_Development/Dual_Wield_System_Plan.md) | COMPLETED | Assassin dual wield (8 phases) |
| [Card_Naming_System_Plan.md](05_Development/Card_Naming_System_Plan.md) | COMPLETED | Card prefix/suffix naming |
| [Card_Deferred_Systems_Plan.md](05_Development/Card_Deferred_Systems_Implementation_Plan.md) | COMPLETED | Card auto-spell, drain, status procs |
| [Cart_Vending_System_Plan.md](05_Development/Cart_Vending_System_Implementation_Plan.md) | COMPLETED | Cart inventory + vending shops |
| [Item_Tooltip_Inspect_Plan.md](05_Development/Item_Tooltip_Inspect_Plan.md) | COMPLETED | Item inspection panel |
| [Refine_ATK_And_Party_System_Plan.md](05_Development/Refine_ATK_And_Party_System_Plan.md) | COMPLETED | Refine bonuses + party system |
| [Deferred_Systems_Remediation_Plan.md](05_Development/Deferred_Systems_Comprehensive_Remediation_Plan.md) | COMPLETED (10/10 phases) | Magic Rod, ensembles, ASPD potions, Abracadabra, etc. |
| [Deferred_Skills_Remediation.md](05_Development/Deferred_Skills_Systems_Remediation_Plan.md) | COMPLETED | Deferred skills across all classes |
| [Alchemist_Deferred_Skills.md](05_Development/Alchemist_Deferred_Skills_Implementation_Plan.md) | COMPLETED | Pharmacy, Summon Flora, Marine Sphere |
| [Phase_7_9_12_Completion.md](05_Development/Phase_7_9_12_Completion_Plan.md) | COMPLETED | Whisper, MVP, death penalty, pets |
| [Phase_16_Companions.md](05_Development/Phase_16_Companions_Plan.md) | COMPLETED | Pet/homunculus/mount visual actors |
| [Login_CharSelect_Redesign.md](05_Development/Login_CharSelect_Redesign_Plan.md) | COMPLETED | Pure C++ Slate login flow |
| [Skill_Tree_UI_Rewrite.md](05_Development/Skill_Tree_UI_Rewrite_Plan.md) | COMPLETED | C++ Slate skill tree |
| [AC_HUDManager_Refactoring.md](05_Development/AC_HUDManager_Refactoring_Plan.md) | COMPLETED | Replaced by C++ subsystems |

### Second Class Research & Audits

| Document | Class | Status |
|----------|-------|--------|
| [Knight_Class_Research.md](05_Development/Knight_Class_Research.md) | Knight | COMPLETED — 21 issues fixed |
| [Knight_Skills_Audit.md](05_Development/Knight_Skills_Audit_And_Fix_Plan.md) | Knight | COMPLETED |
| [Crusader_Class_Research.md](05_Development/Crusader_Class_Research.md) | Crusader | COMPLETED — 21 bugs fixed |
| [Crusader_Skills_Audit.md](05_Development/Crusader_Skills_Audit_And_Fix_Plan.md) | Crusader | COMPLETED |
| [Wizard_Class_Research.md](05_Development/Wizard_Class_Research.md) | Wizard | COMPLETED — 34 bugs fixed |
| [Wizard_Skills_Audit.md](05_Development/Wizard_Skills_Audit_And_Fix_Plan.md) | Wizard | COMPLETED |
| [Sage_Class_Research.md](05_Development/Sage_Class_Research.md) | Sage | COMPLETED — 25 bugs fixed |
| [Sage_Skills_Audit.md](05_Development/Sage_Skills_Comprehensive_Audit.md) | Sage | COMPLETED |
| [Hunter_Class_Research.md](05_Development/Hunter_Class_Research.md) | Hunter | COMPLETED — 13 bugs fixed |
| [Hunter_Skills_Audit.md](05_Development/Hunter_Skills_Audit_And_Fix_Plan.md) | Hunter | COMPLETED |
| [Bard_Class_Research.md](05_Development/Bard_Class_Research.md) | Bard | COMPLETED — 22+1 bugs fixed |
| [Bard_Skills_Audit.md](05_Development/Bard_Skills_Comprehensive_Audit.md) | Bard | COMPLETED |
| [Dancer_Class_Research.md](05_Development/Dancer_Class_Research.md) | Dancer | COMPLETED |
| [Dancer_Skills_Audit.md](05_Development/Dancer_Skills_Audit_And_Fix_Plan.md) | Dancer | COMPLETED |
| [Priest_Class_Research.md](05_Development/Priest_Class_Research.md) | Priest | COMPLETED |
| [Priest_Skills_Audit.md](05_Development/Priest_Skills_Audit_And_Fix_Plan.md) | Priest | COMPLETED |
| [Monk_Class_Research.md](05_Development/Monk_Class_Research.md) | Monk | COMPLETED — 12 bugs + 3 deferred skills |
| [Monk_Skills_Audit.md](05_Development/Monk_Skills_Audit_And_Fix_Plan.md) | Monk | COMPLETED |
| [Assassin_Class_Research.md](05_Development/Assassin_Class_Research.md) | Assassin | COMPLETED |
| [Assassin_Skills_Audit.md](05_Development/Assassin_Skills_Audit_And_Fix_Plan.md) | Assassin | COMPLETED |
| [Rogue_Class_Research.md](05_Development/Rogue_Class_Research.md) | Rogue | COMPLETED — 19/19 skills, 27 issues |
| [Rogue_Skills_Audit.md](05_Development/Rogue_Skills_Audit_And_Fix_Plan.md) | Rogue | COMPLETED |
| [Rogue_Skills_Remaining.md](05_Development/Rogue_Skills_Remaining_Systems_Plan.md) | Rogue | COMPLETED |
| [Rogue_Final_Three_Skills.md](05_Development/Rogue_Final_Three_Skills_Plan.md) | Rogue | COMPLETED |
| [Blacksmith_Class_Research.md](05_Development/Blacksmith_Class_Research.md) | Blacksmith | COMPLETED — 10 bugs fixed |
| [Blacksmith_Skills_Audit.md](05_Development/Blacksmith_Skills_Audit_And_Fix_Plan.md) | Blacksmith | COMPLETED |
| [Alchemist_Class_Research.md](05_Development/Alchemist_Class_Research.md) | Alchemist | COMPLETED |
| [Alchemist_Skills_Audit.md](05_Development/Alchemist_Skills_Audit_And_Fix_Plan.md) | Alchemist | COMPLETED |

### First Class Skill Audits

| Document | Class | Status |
|----------|-------|--------|
| [Novice_Skills_Audit.md](05_Development/Novice_Skills_Audit_And_Fix_Plan.md) | Novice | COMPLETED |
| [Swordsman_Skills_Audit.md](05_Development/Swordsman_Skills_Audit_And_Fix_Plan.md) | Swordsman | COMPLETED |
| [Swordsman_Skills_Deep_Audit.md](05_Development/Swordsman_Skills_Deep_Audit_Plan.md) | Swordsman | COMPLETED |
| [Mage_Skills_Audit.md](05_Development/Mage_Skills_Audit_And_Fix_Plan.md) | Mage | COMPLETED |
| [Mage_Skills_Comprehensive_v2.md](05_Development/Mage_Skills_Comprehensive_Audit_v2.md) | Mage | COMPLETED |
| [Archer_Skills_Audit.md](05_Development/Archer_Skills_Audit_And_Fix_Plan.md) | Archer | COMPLETED |
| [Acolyte_Skills_Audit.md](05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md) | Acolyte | COMPLETED |
| [Acolyte_Skills_Comprehensive_v2.md](05_Development/Acolyte_Skills_Comprehensive_Audit_v2.md) | Acolyte | COMPLETED |
| [Acolyte_Skills_Full_v2.md](05_Development/Acolyte_Skills_Full_Audit_v2.md) | Acolyte | COMPLETED |
| [Thief_Skills_Audit.md](05_Development/Thief_Skills_Audit_And_Fix_Plan.md) | Thief | COMPLETED |
| [Thief_Skills_Deep_Audit.md](05_Development/Thief_Skills_Deep_Audit_Plan.md) | Thief | COMPLETED |
| [Merchant_Skills_Audit.md](05_Development/Merchant_Skills_Audit_And_Fix_Plan.md) | Merchant | COMPLETED |
| [Merchant_Skills_Comprehensive.md](05_Development/Merchant_Skills_Comprehensive_Audit.md) | Merchant | COMPLETED |

### Verification & Post-Implementation

| Document | Status |
|----------|--------|
| [Deferred_Systems_Post_Audit.md](05_Development/Deferred_Systems_Post_Implementation_Audit.md) | COMPLETED — all 38 items verified |
| [Deferred_Systems_Final_Verification.md](05_Development/Deferred_Systems_Final_Verification.md) | COMPLETED — zero remaining |
| [Full_Verification_Checklist.md](05_Development/Full_Verification_Checklist.md) | COMPLETED |
| [Skill_Fix_Execution_Plan.md](05_Development/Skill_Fix_Execution_Plan.md) | COMPLETED |

### Research Documents

| Document | Description |
|----------|-------------|
| [2nd_Job_Skills_Complete_Research.md](05_Development/2nd_Job_Skills_Complete_Research.md) | Comprehensive second class skill research |
| [Hunter_Bard_Dancer_Skills_Research.md](05_Development/Hunter_Bard_Dancer_Skills_Research.md) | Performance class research |
| [Second_Class_Implementation_Prompts.md](05_Development/Second_Class_Implementation_Prompts.md) | Implementation prompt templates |
| [RO_Resource_Equipment_Systems_Research.md](05_Development/RO_Resource_Equipment_Systems_Research.md) | RO Classic equipment system research |
| [Item_Stat_Script_Audit.md](05_Development/Item_Stat_Script_Audit.md) | Item script parsing audit |

### VFX Documentation

| Document | Description | Related |
|----------|-------------|---------|
| [Skill_VFX_Implementation_Plan.md](05_Development/Skill_VFX_Implementation_Plan.md) | Niagara architecture, per-skill VFX specs | [VFX_Asset_Reference](05_Development/VFX_Asset_Reference.md) |
| [VFX_Asset_Reference.md](05_Development/VFX_Asset_Reference.md) | 1,574 VFX assets cataloged and mapped to RO skills | [Skill_VFX_Implementation_Plan](05_Development/Skill_VFX_Implementation_Plan.md) |
| [Skill_VFX_Research_Findings.md](05_Development/Skill_VFX_Research_Findings.md) | 138-source research on RO effects and UE5 Niagara | — |
| [Skill_VFX_Execution_Plan.md](05_Development/Skill_VFX_Execution_Plan.md) | Step-by-step VFX build status | FUTURE PLAN |
| [VFX_System_Audit.md](05_Development/VFX_System_Audit.md) | VFX system audit pass 1 | [VFX_System_Audit_Pass2](05_Development/VFX_System_Audit_Pass2.md) |
| [VFX_System_Audit_Pass2.md](05_Development/VFX_System_Audit_Pass2.md) | VFX system audit pass 2 | — |

### Guides & Testing

| Document | Description |
|----------|-------------|
| [Setup_Guide.md](05_Development/Setup_Guide.md) | Development environment setup |
| [Zone_System_UE5_Setup_Guide.md](05_Development/Zone_System_UE5_Setup_Guide.md) | Level Blueprint, zone registry, warp placement |
| [Comprehensive_Testing_Framework.md](05_Development/Comprehensive_Testing_Framework.md) | Testing architecture and approach |
| [Test_Checklist.md](05_Development/Test_Checklist.md) | Manual test checklist |
| [UI_Testing_System.md](05_Development/UI_Testing_System.md) | Automated UI testing system |
| [Troubleshooting.md](05_Development/Troubleshooting.md) | Common issues and solutions |

---

## 06 — Reference

| Document | Description | Related |
|----------|-------------|---------|
| [Event_Reference.md](06_Reference/Event_Reference.md) | All 79 socket.io events (client↔server) | [Networking_Protocol](04_Integration/Networking_Protocol.md) |
| [API_Reference.md](06_Reference/API_Reference.md) | All 11 REST API endpoints | [API_Documentation](03_Server_Side/API_Documentation.md) |
| [ID_Reference.md](06_Reference/ID_Reference.md) | Skill IDs, item IDs, monster IDs, class IDs | — |
| [Configuration_Reference.md](06_Reference/Configuration_Reference.md) | Server config, .env variables | — |
| [Glossary.md](06_Reference/Glossary.md) | RO Classic and project terminology | — |

## 06 — Audit Reports

| Document | Description |
|----------|-------------|
| [Codebase_Audit_Report.md](06_Audit/Codebase_Audit_Report.md) | Full codebase audit |
| [Skills_VFX_System_Audit.md](06_Audit/Skills_VFX_System_Audit.md) | Skills and VFX system audit |

---

## Items Database

| Document | Description |
|----------|-------------|
| [COMPLETE_ITEM_RESEARCH_REPORT.md](items/COMPLETE_ITEM_RESEARCH_REPORT.md) | Master item research report |
| [RO_Pre_Renewal_Equipment_Complete_Database.md](items/RO_Pre_Renewal_Equipment_Complete_Database.md) | Full pre-renewal equipment database |
| [weapons_complete_database.md](items/weapons_complete_database.md) | Complete weapon database |
| [weapons.md](items/weapons.md) | Weapon reference |
| [armor.md](items/armor.md) | Armor reference |
| [shields.md](items/shields.md) | Shield reference |
| [headgear.md](items/headgear.md) | Headgear reference |
| [footgear.md](items/footgear.md) | Footgear reference |
| [garments.md](items/garments.md) | Garment reference |
| [accessories.md](items/accessories.md) | Accessory reference |
| [consumables.md](items/consumables.md) | Consumable items |
| [etc_crafting.md](items/etc_crafting.md) | Etc and crafting materials |
| [cards.md](items/cards.md) | Card reference |
| [cards_complete_reference.md](items/cards_complete_reference.md) | Complete card database (538 cards) |
| [RO_Classic_Usable_Misc_Items_Complete.md](items/RO_Classic_Usable_Misc_Items_Complete.md) | Usable and misc items |
| [missing_items_report.md](items/missing_items_report.md) | Items not yet in database |
| [item_noequip.md](items/item_noequip.md) | Item equip restrictions by map type (PvP/GvG/WoE) — reference data |

---

## UI Style Guide

| Document | Description |
|----------|-------------|
| [ro_classic_ui_slate_style_guide.md](UI/ro_classic_ui_slate_style_guide.md) | RO Classic brown/gold Slate theme, colors, fonts, widget patterns |

---

## High-Level Docs (Legacy)

| Document | Description | Status |
|----------|-------------|--------|
| [README.md](high-level%20docs/README.md) | Project overview and quick start | CURRENT |
| [RUNNING.md](high-level%20docs/RUNNING.md) | Step-by-step startup guide | Needs UE version update |
| [MMO_Development_Plan.md](high-level%20docs/MMO_Development_Plan.md) | Original 6-month plan | HISTORICAL |
| [MMO_Development_Plan_Revised.md](high-level%20docs/MMO_Development_Plan_Revised.md) | Revised solo-dev plan | HISTORICAL |

---

## RagnaCloneDocs (RO Classic Reference)

Game design reference — 14 design docs + 14 implementation guides. These are reference material, not project docs.

### Design Reference
| Document | Topic |
|----------|-------|
| [00_Master_Build_Plan.md](../RagnaCloneDocs/00_Master_Build_Plan.md) | Master build plan |
| [01_Stats_Leveling_JobSystem.md](../RagnaCloneDocs/01_Stats_Leveling_JobSystem.md) | Stats, leveling, job system |
| [02_Combat_System.md](../RagnaCloneDocs/02_Combat_System.md) | Combat formulas and mechanics |
| [03_Skills_Complete.md](../RagnaCloneDocs/03_Skills_Complete.md) | All RO Classic skills |
| [04_Monsters_EnemyAI.md](../RagnaCloneDocs/04_Monsters_EnemyAI.md) | Monster behavior and AI |
| [05_Items_Equipment_Cards.md](../RagnaCloneDocs/05_Items_Equipment_Cards.md) | Items, equipment, card system |
| [06_World_Maps_Zones.md](../RagnaCloneDocs/06_World_Maps_Zones.md) | World structure and zones |
| [07_NPCs_Quests_Shops.md](../RagnaCloneDocs/07_NPCs_Quests_Shops.md) | NPCs, quests, shops |
| [08_PvP_Guild_WoE.md](../RagnaCloneDocs/08_PvP_Guild_WoE.md) | PvP, guild, War of Emperium |
| [09_UI_UX_System.md](../RagnaCloneDocs/09_UI_UX_System.md) | UI/UX design |
| [10_Art_Animation_VFX_Pipeline.md](../RagnaCloneDocs/10_Art_Animation_VFX_Pipeline.md) | Art and animation pipeline |
| [11_Multiplayer_Networking.md](../RagnaCloneDocs/11_Multiplayer_Networking.md) | Multiplayer networking design |
| [12_Pets_Homunculus_Companions.md](../RagnaCloneDocs/12_Pets_Homunculus_Companions.md) | Pets, homunculus, companions |
| [13_Economy_Trading_Vending.md](../RagnaCloneDocs/13_Economy_Trading_Vending.md) | Economy and trading |
| [14_Audio_Music_SFX.md](../RagnaCloneDocs/14_Audio_Music_SFX.md) | Audio and music |
| [15_Movement_Targeting_Interaction.md](../RagnaCloneDocs/15_Movement_Targeting_Interaction.md) | Movement and targeting |

### Implementation Guides
| Document | Topic |
|----------|-------|
| [00_Master_Implementation_Plan.md](../RagnaCloneDocs/Implementation/00_Master_Implementation_Plan.md) | Master implementation plan |
| [01_Core_Architecture.md](../RagnaCloneDocs/Implementation/01_Core_Architecture.md) | Core architecture guide |
| [02_Stats_Class_System.md](../RagnaCloneDocs/Implementation/02_Stats_Class_System.md) | Stats and class implementation |
| [03_Combat_System.md](../RagnaCloneDocs/Implementation/03_Combat_System.md) | Combat implementation |
| [04_Skill_System.md](../RagnaCloneDocs/Implementation/04_Skill_System.md) | Skill system implementation |
| [05_Enemy_Monster_System.md](../RagnaCloneDocs/Implementation/05_Enemy_Monster_System.md) | Enemy/monster implementation |
| [06_Inventory_Equipment.md](../RagnaCloneDocs/Implementation/06_Inventory_Equipment.md) | Inventory implementation |
| [07_UI_HUD_System.md](../RagnaCloneDocs/Implementation/07_UI_HUD_System.md) | UI/HUD implementation |
| [08_Zone_World_System.md](../RagnaCloneDocs/Implementation/08_Zone_World_System.md) | Zone/world implementation |
| [09_NPC_Quest_System.md](../RagnaCloneDocs/Implementation/09_NPC_Quest_System.md) | NPC/quest implementation |
| [10_Party_Guild_Social.md](../RagnaCloneDocs/Implementation/10_Party_Guild_Social.md) | Social systems implementation |
| [11_PvP_WoE.md](../RagnaCloneDocs/Implementation/11_PvP_WoE.md) | PvP/WoE implementation |
| [12_Economy_Vending.md](../RagnaCloneDocs/Implementation/12_Economy_Vending.md) | Economy implementation |
| [13_Companions_Mounts.md](../RagnaCloneDocs/Implementation/13_Companions_Mounts.md) | Companions implementation |
| [14_Art_Audio_Pipeline.md](../RagnaCloneDocs/Implementation/14_Art_Audio_Pipeline.md) | Art/audio pipeline |

---

## Database Migrations

25 migration files in `database/migrations/`, listed in recommended execution order:

| Migration | Purpose |
|-----------|---------|
| `add_exp_leveling_system.sql` | EXP tables, base/job level columns |
| `add_class_skill_system.sql` | Class and skill point columns |
| `add_character_hotbar.sql` | Hotbar persistence table |
| `add_hotbar_multirow.sql` | 4-row hotbar support |
| `add_zeny_column.sql` | Zeny (currency) column |
| `add_ro_drop_items.sql` | 126 RO drop items |
| `add_equipped_position.sql` | Dual-accessory support |
| `add_character_customization.sql` | Hair style/color, gender |
| `add_soft_delete.sql` | Soft-delete flag for characters |
| `add_zone_system.sql` | Zone name columns |
| `migrate_to_canonical_ids.sql` | Migrate to rAthena canonical item IDs |
| `add_item_inspect_fields.sql` | Item inspect metadata |
| `fix_item_element_matk_twohanded.sql` | Fix item element/MATK/two-handed |
| `fix_item_script_column_audit.sql` | Fix item script columns |
| `add_card_compound_support.sql` | Card compound slots on inventory |
| `fix_card_perfect_dodge_bonus.sql` | Card perfect dodge fix |
| `populate_card_naming.sql` | Card prefix/suffix naming data |
| `add_equipment_rate_columns.sql` | Equipment drop rate columns |
| `add_equipment_break.sql` | Equipment break system |
| `add_stone_item.sql` | Stone of Sage item |
| `add_forge_columns.sql` | Forged weapon metadata |
| `add_homunculus_table.sql` | Homunculus companion table |
| `add_cart_vending_identify.sql` | Cart, vending, identify tables |
| `add_party_system.sql` | Party/party_members tables |
| `add_plagiarism_columns.sql` | Plagiarism skill columns |

---

## Dev Notes (`Dev Notes/`)

Historical development session notes, Claude session logs, prompts, and implementation tracking. Each `.md` file was converted from the original `.txt` session notes.

### Numbered Fix Documents
| Document | Description | Related |
|----------|-------------|---------|
| [01_Server_Equipment_Pipeline_Fix.md](../Dev%20Notes/01_Server_Equipment_Pipeline_Fix.md) | Server equipment stats pipeline fix | [Inventory_System](03_Server_Side/Inventory_System.md) |
| [02_Derived_Stats_Formula_Fix.md](../Dev%20Notes/02_Derived_Stats_Formula_Fix.md) | Derived stats formula corrections | [Combat_System](03_Server_Side/Combat_System.md) |
| [03_Client_Stats_UI_Update.md](../Dev%20Notes/03_Client_Stats_UI_Update.md) | Client stats UI + tooltip updates | [BasicInfo_Slate_UI](02_Client_Side/C++_Code/08_BasicInfo_Slate_UI.md) |
| [RO_Advanced_Combat_Mechanics_Research.md](../Dev%20Notes/RO_Advanced_Combat_Mechanics_Research.md) | Deep research on pre-renewal combat mechanics | [Combat_System](03_Server_Side/Combat_System.md) |

### First Class Skill Fix Sessions
| Document | Description |
|----------|-------------|
| [Novice Fixes.md](../Dev%20Notes/Novice%20Fixes.md) | Novice skill fix session |
| [Swordsman Fixes.md](../Dev%20Notes/Swordsman%20Fixes.md) | Swordsman skill fix session |
| [Mage Fixes.md](../Dev%20Notes/Mage%20Fixes.md) | Mage skill fix session |
| [Archer Fixes.md](../Dev%20Notes/Archer%20Fixes.md) | Archer skill fix session |
| [Acolyte Fixes.md](../Dev%20Notes/Acolyte%20Fixes.md) | Acolyte skill fix session |
| [Thief Fixes.md](../Dev%20Notes/Thief%20Fixes.md) | Thief skill fix session |
| [Merchant Fixes.md](../Dev%20Notes/Merchant%20Fixes.md) | Merchant skill fix session |
| [first class skills fixes - generic.md](../Dev%20Notes/first%20class%20skills%20fixes%20-%20generic.md) | Cross-class generic skill fixes |
| [first class skills deferred systems.md](../Dev%20Notes/first%20class%20skills%20deferred%20systems.md) | Deferred skill systems from first class phase |

### Second Class Implementation Phases
| Document | Description |
|----------|-------------|
| [2-1 and 2-2 skills implementation overall implementation and prompts.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20overall%20implementation%20and%20prompts.md) | Overall plan + prompts for 2nd class skills |
| [2-1 and 2-2 skills implementation P0.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P0.md) | Phase 0: Foundation (data fixes, ground effects, mount) |
| [2-1 and 2-2 skills implementation P1.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P1.md) | Phase 1: Assassin + Priest + Knight |
| [2-1 and 2-2 skills implementation P2.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P2.md) | Phase 2: Crusader + Wizard + Sage |
| [2-1 and 2-2 skills implementation P3.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P3.md) | Phase 3: Monk + Hunter |
| [2-1 and 2-2 skills implementation P4.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P4.md) | Phase 4: Bard + Dancer |
| [2-1 and 2-2 skills implementation P5.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P5.md) | Phase 5: Blacksmith + Rogue |
| [2-1 and 2-2 skills implementation P6.md](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P6.md) | Phase 6: Alchemist + Homunculus |
| [2-2 and 2-2 classes research and fi.md](../Dev%20Notes/2-2%20and%202-2%20classes%20research%20and%20fi.md) | Second class research findings |
| [2 1 handed weapons research.md](../Dev%20Notes/2%201%20handed%20weapons%20research.md) | Two-handed/one-handed weapon research |
| [Deferred Skills & Systems — Full plan.md](../Dev%20Notes/Deferred%20Skills%20&%20Systems%20—%20Full%20plan.md) | Full deferred systems remediation plan |

### System Implementation Sessions
| Document | Description |
|----------|-------------|
| [card system implementation.md](../Dev%20Notes/card%20system%20implementation.md) | Card compound system implementation |
| [adding extra systems to support cards.md](../Dev%20Notes/adding%20extra%20systems%20to%20support%20cards.md) | Card support systems (auto-spell, drain, procs) |
| [slotted cards and items name research.md](../Dev%20Notes/slotted%20cards%20and%20items%20name%20research.md) | Card naming prefix/suffix research |
| [applying card icons to all cards and where they should display.md](../Dev%20Notes/applying%20card%20icons%20to%20all%20cards%20and%20where%20they%20should%20display.md) | Card icon display implementation |
| [card icon generation.md](../Dev%20Notes/card%20icon%20generation.md) | AI card icon generation session |
| [ammunition system.md](../Dev%20Notes/ammunition%20system.md) | Arrow/ammo system implementation |
| [weight system implementation.md](../Dev%20Notes/weight%20system%20implementation.md) | Weight threshold system |
| [vending system implementation.md](../Dev%20Notes/vending%20system%20implementation.md) | Vending shop implementation |
| [party and refine systems.md](../Dev%20Notes/party%20and%20refine%20systems.md) | Party + refine ATK implementation |
| [shop implementation.md](../Dev%20Notes/shop%20implementation.md) | NPC shop system |
| [equipment and stat pipeline fixes.md](../Dev%20Notes/equipment%20and%20stat%20pipeline%20fixes.md) | Equipment stat pipeline fixes |
| [inserting cards and items into player inventory.md](../Dev%20Notes/inserting%20cards%20and%20items%20into%20player%20inventory.md) | Item insertion system |
| [item stats and scripts audit.md](../Dev%20Notes/item%20stats%20and%20scripts%20audit.md) | Item stat/script audit session |
| [enhanced item inspect and ui feature.md](../Dev%20Notes/enhanced%20item%20inspect%20and%20ui%20feature.md) | Item inspect panel implementation |
| [item icon generation.md](../Dev%20Notes/item%20icon%20generation.md) | AI item icon generation session |
| [fixing more stat on items.md](../Dev%20Notes/%20fixing%20more%20stat%20on%20items.md) | Additional item stat fixes |

### UI & Client Sessions
| Document | Description |
|----------|-------------|
| [login redesign.md](../Dev%20Notes/login%20redesign.md) | Login flow C++ Slate redesign |
| [starting to use slate UI and creating basic info UI.md](../Dev%20Notes/starting%20to%20use%20slate%20UI%20and%20creating%20basic%20info%20UI.md) | First Slate UI implementation |
| [health and mana bar redesign.md](../Dev%20Notes/health%20and%20mana%20bar%20redesign.md) | HP/SP bar redesign |
| [initial hotbar implementations.md](../Dev%20Notes/initial%20hotbar%20implementations.md) | Hotbar system implementation |
| [initial skilltree design.md](../Dev%20Notes/initial%20skilltree%20design.md) | Skill tree UI design |
| [initial skills and VFX implementation bug tracker.md](../Dev%20Notes/initial%20skills%20and%20VFX%20implementation%20bug%20tracker.md) | Skills + VFX bug tracking |
| [skills VFX troubleshooting.md](../Dev%20Notes/skills%20VFX%20troubleshooting.md) | VFX troubleshooting session |
| [note on creating new zones.md](../Dev%20Notes/note%20on%20creating%20new%20zones.md) | Zone creation notes |

### Architecture & Migration Sessions
| Document | Description |
|----------|-------------|
| [Socket rewrite.md](../Dev%20Notes/Socket%20rewrite.md) | Persistent socket rewrite session |
| [BP bridge migration for hotbar inventory loot chat.md](../Dev%20Notes/BP%20bridge%20migration%20for%20hotbar%20inventory%20loot%20chat.md) | Blueprint bridge removal session |
| [blueprint to c++ project conversion notes on using socketIO.md](../Dev%20Notes/blueprint%20to%20c++%20project%20conversion%20notes%20on%20using%20socketIO.md) | Blueprint→C++ conversion notes |
| [delete all useless stuff from editor and blueprint- full cplusplus transiton.md](../Dev%20Notes/delete%20all%20useless%20stuff%20from%20editor%20and%20blueprint-%20full%20cplusplus%20transiton.md) | Dead Blueprint cleanup |
| [huge targeting revamp.md](../Dev%20Notes/huge%20targeting%20revamp.md) | Targeting system revamp Phase 1 |
| [huge targeting revamp phase 2.md](../Dev%20Notes/huge%20targeting%20revamp%20phase%202.md) | Targeting revamp Phase 2 |
| [huge targeting revamp phase 3.md](../Dev%20Notes/huge%20targeting%20revamp%20phase%203.md) | Targeting revamp Phase 3 |
| [huge targeting revamp phase 5.md](../Dev%20Notes/huge%20targeting%20revamp%20phase%205.md) | Targeting revamp Phase 5 |
| [huge targeting revamp phase 5 plus research.md](../Dev%20Notes/huge%20targeting%20revamp%20phase%205%20plus%20research.md) | Targeting revamp Phase 5 + research |
| [huge targeting revamp Phase 6 Dead Blueprint Cleanup.md](../Dev%20Notes/huge%20targeting%20revamp%20Phase%206%20Dead%20Blueprint%20Cleanup.md) | Targeting revamp Phase 6 cleanup |
| [Db backup and restore.md](../Dev%20Notes/Db%20backup%20and%20restore.md) | Database backup/restore procedures |
| [saved claude sessions.md](../Dev%20Notes/saved%20claude%20sessions.md) | Index of saved Claude Code sessions |

### Class Skills Audit Sessions (`Dev Notes/class skills audit/`)
| Document | Class |
|----------|-------|
| [swordsman skills audit.md](../Dev%20Notes/class%20skills%20audit/swordsman%20skills%20audit.md) | Swordsman |
| [acolyte and mage skills audit.md](../Dev%20Notes/class%20skills%20audit/acolyte%20and%20mage%20skills%20audit.md) | Acolyte + Mage |
| [Archer skill audit.md](../Dev%20Notes/class%20skills%20audit/Archer%20skill%20audit.md) | Archer |
| [thief skills audit.md](../Dev%20Notes/class%20skills%20audit/thief%20skills%20audit.md) | Thief (pass 1) |
| [thief skills audit2.md](../Dev%20Notes/class%20skills%20audit/thief%20skills%20audit2.md) | Thief (pass 2) |
| [merchant skills audit.md](../Dev%20Notes/class%20skills%20audit/merchant%20skills%20audit.md) | Merchant |
| [knight skill audit.md](../Dev%20Notes/class%20skills%20audit/knight%20skill%20audit.md) | Knight |
| [Crusader Skills Audit and Plan Foundation Systems.md](../Dev%20Notes/class%20skills%20audit/Crusader%20Skills%20Audit%20and%20Plan%20Foundation%20Systems.md) | Crusader |
| [wizard skill audit.md](../Dev%20Notes/class%20skills%20audit/wizard%20skill%20audit.md) | Wizard |
| [sage skill audit.md](../Dev%20Notes/class%20skills%20audit/sage%20skill%20audit.md) | Sage |
| [hunter skill audit.md](../Dev%20Notes/class%20skills%20audit/hunter%20skill%20audit.md) | Hunter |
| [bard skill audit.md](../Dev%20Notes/class%20skills%20audit/bard%20skill%20audit.md) | Bard |
| [dancer skills audit.md](../Dev%20Notes/class%20skills%20audit/dancer%20skills%20audit.md) | Dancer |
| [monk skill audit.md](../Dev%20Notes/class%20skills%20audit/monk%20skill%20audit.md) | Monk |
| [assasin skill audit.md](../Dev%20Notes/class%20skills%20audit/assasin%20skill%20audit.md) | Assassin |
| [rogue skills audit and monster skills implementation.md](../Dev%20Notes/class%20skills%20audit/rogue%20skills%20audit%20and%20monster%20skills%20implementation.md) | Rogue + Monster Skills |
| [blacksmith skills audit.md](../Dev%20Notes/class%20skills%20audit/blacksmith%20skills%20audit.md) | Blacksmith |
| [alchemist skills audit.md](../Dev%20Notes/class%20skills%20audit/alchemist%20skills%20audit.md) | Alchemist |

### Useful Prompts (`Dev Notes/useful prompts/`)
Prompt templates used during development with Claude Code.

| Document | Purpose |
|----------|---------|
| [GENERIC RESEARCH PROMPT.md](GENERIC%20RESEARCH%20PROMPT.md) | Generic deep research prompt template |
| [items research.md](items%20research.md) | Item system research prompt |
| [jobs research.md](jobs%20research.md) | Job/class system research prompt |
| [skilltree redesign.md](skilltree%20redesign.md) | Skill tree UI redesign prompt |
| [system review and research agaisnt ragnarok online.md](system%20review%20and%20research%20agaisnt%20ragnarok%20online.md) | System review vs RO Classic prompt |
| [update documentation and skills.md](update%20documentation%20and%20skills.md) | Documentation update prompt |
| [using icons.md](using%20icons.md) | Icon integration prompt |
| [can you make a plan to fully imple.md](%20can%20you%20make%20a%20plan%20to%20fully%20imple.md) | Implementation planning prompt |

### Old Blueprint & Windsurf Setups (`Dev Notes/old blueprint and windsurf setups/`)
Early scratch notes from before the C++ migration. Historical only.

| Document | Description |
|----------|-------------|
| [1.md](1.md) | Early setup notes (part 1) |
| [2.md](2.md) | Early setup notes (part 2) |
| [aura refactor questions.md](aura%20refactor%20questions.md) | Aura/VFX refactor questions |
| [todo progress tracker.md](todo%20progress%20tracker.md) | Early todo/progress tracker |
| [when target dies we should hide Tar.md](when%20target%20dies%20we%20should%20hide%20Tar.md) | Target death UI bug note |

---

## Testing Documentation (`tests/client/`)

UE5 client-side testing guides and coverage reports.

| Document | Description |
|----------|-------------|
| [README_UI_TESTS.md](../tests/client/README_UI_TESTS.md) | UI testing setup overview using UE5 Automation System |
| [COMPLETE_SETUP_GUIDE.md](../tests/client/COMPLETE_SETUP_GUIDE.md) | Complete test environment setup guide |
| [IMPLEMENTATION_GUIDE.md](../tests/client/IMPLEMENTATION_GUIDE.md) | Test implementation guide |
| [SERVER_TESTING_QUICK_START.md](../tests/client/SERVER_TESTING_QUICK_START.md) | Server testing quick start |
| [SERVER_TEST_COVERAGE.md](../tests/client/SERVER_TEST_COVERAGE.md) | Server test coverage report |
| [UI_TEST_COVERAGE.md](../tests/client/UI_TEST_COVERAGE.md) | UI test coverage report |
| [UI_Test_Functions.md](../tests/client/UI_Test_Functions.md) | UI test function reference |
| [BP_AutomationTestLibrary.md](../tests/client/BP_AutomationTestLibrary.md) | Blueprint automation test library docs |
| [BP_AutomationTestLibrary_Implementation.md](../tests/client/BP_AutomationTestLibrary_Implementation.md) | Implementation details for automation test library |
| [TestEnvironment_Setup.md](../tests/client/TestEnvironment_Setup.md) | Test environment setup reference |
| [CRASH_TROUBLESHOOTING.md](../tests/client/CRASH_TROUBLESHOOTING.md) | Crash troubleshooting during tests |

> **Related**: [UI_Testing_System.md](05_Development/UI_Testing_System.md) | [Comprehensive_Testing_Framework.md](05_Development/Comprehensive_Testing_Framework.md)

---

## Root-Level Documents

Miscellaneous docs at the project root. These are meta/tooling files, not game system documentation.

| Document | Description | Status |
|----------|-------------|--------|
| [CLAUDE.md](../CLAUDE.md) | Claude Code project instructions — loaded automatically | ACTIVE |

### Meta / Tooling (`_meta/`)
| Document | Description |
|----------|-------------|
| [AI_Game_Dev_Workflows_Report.md](../_meta/AI_Game_Dev_Workflows_Report.md) | Report on AI-driven game development workflows with Claude Code |
| [UE5_Project_Documentation_Prompt.md](../_meta/UE5_Project_Documentation_Prompt.md) | Prompt template for UE5 project documentation |
| [README_TESTS.md](../_meta/README_TESTS.md) | Test readme for the project |
| [claude_code_opus46_1m_setup.md](../_meta/claude_code_opus46_1m_setup.md) | Claude Code Opus 4.6 setup notes |
| [mcpSuggestions.md](../_meta/mcpSuggestions.md) | MCP server suggestions |
| [2026-03-20.md](../_meta/2026-03-20.md) | Daily session note |

### Tools & Scripts
| Document | Description |
|----------|-------------|
| [tools/item_icon_prompts.md](../tools/item_icon_prompts.md) | AI image generation prompts for RO-style item icons |
| [scripts/output/unresolved_names.md](../scripts/output/unresolved_names.md) | Unresolved item names from rAthena migration |

---

## Archived Documentation (`_archive/`)

Legacy documentation from early development (February 2026). These have been **superseded** by `docsNew/` but are preserved for historical reference. All systems described here have been significantly expanded since.

### System Docs (Superseded)
| Document | Superseded By |
|----------|--------------|
| [Authentication_System.md](../_archive/Authentication_System.md) | [Authentication_Flow.md](04_Integration/Authentication_Flow.md) |
| [Camera_System.md](../_archive/Camera_System.md) | CameraSubsystem (C++) |
| [Character_Management.md](../_archive/Character_Management.md) | [MMOGameInstance.md](02_Client_Side/C++_Code/02_MMOGameInstance.md) |
| [Character_Select_UI.md](../_archive/Character_Select_UI.md) | LoginFlowSubsystem (C++) |
| [Chat_System.md](../_archive/Chat_System.md) | ChatSubsystem (C++) |
| [Database_Schema.md](../_archive/Database_Schema.md) | [Database_Architecture.md](01_Architecture/Database_Architecture.md) |
| [Enemy_Combat_System.md](../_archive/Enemy_Combat_System.md) | [Enemy_System.md](03_Server_Side/Enemy_System.md) + [Combat_System.md](03_Server_Side/Combat_System.md) |
| [Enhanced_Input_System.md](../_archive/Enhanced_Input_System.md) | PlayerInputSubsystem (C++) |
| [HTTP_Manager.md](../_archive/HTTP_Manager.md) | [MMOHttpManager.md](02_Client_Side/C++_Code/03_MMOHttpManager.md) |
| [Items_Inventory_System.md](../_archive/Items_Inventory_System.md) | [Inventory_System.md](03_Server_Side/Inventory_System.md) |
| [JSON_Communication_Protocol.md](../_archive/JSON_Communication_Protocol.md) | [Networking_Protocol.md](04_Integration/Networking_Protocol.md) |
| [Multiplayer_Architecture.md](../_archive/Multiplayer_Architecture.md) | [Multiplayer_Architecture.md](01_Architecture/Multiplayer_Architecture.md) |
| [Position_Persistence.md](../_archive/Position_Persistence.md) | [Zone_System_UE5_Setup_Guide.md](05_Development/Zone_System_UE5_Setup_Guide.md) |
| [Server_API.md](../_archive/Server_API.md) | [API_Documentation.md](03_Server_Side/API_Documentation.md) |
| [Server_Logging.md](../_archive/Server_Logging.md) | [NodeJS_Server.md](03_Server_Side/NodeJS_Server.md) |
| [SocketIO_RealTime_Multiplayer.md](../_archive/SocketIO_RealTime_Multiplayer.md) | [Event_Reference.md](06_Reference/Event_Reference.md) |
| [Top_Down_Movement_System.md](../_archive/Top_Down_Movement_System.md) | PlayerInputSubsystem + CameraSubsystem (C++) |

### Blueprint Docs (Dead Code)
| Document | Superseded By |
|----------|--------------|
| [AC_HUDManager.md](../_archive/AC_HUDManager.md) | C++ UWorldSubsystems |
| [BPI_Damageable.md](../_archive/BPI_Damageable.md) | [BPI_Damageable_Interface.md](02_Client_Side/Blueprints/08_BPI_Damageable_Interface.md) |
| [BP_MMOCharacter.md](../_archive/BP_MMOCharacter.md) | [BP_MMOCharacter.md](02_Client_Side/Blueprints/03_BP_MMOCharacter.md) |
| [BP_OtherPlayerCharacter.md](../_archive/BP_OtherPlayerCharacter.md) | OtherPlayerSubsystem (C++) |
| [BP_OtherPlayerManager.md](../_archive/BP_OtherPlayerManager.md) | OtherPlayerSubsystem (C++) |
| [Blueprint_Integration.md](../_archive/Blueprint_Integration.md) | [Blueprint_Index.md](02_Client_Side/Blueprints/00_Blueprint_Index.md) |
| [UI_Widgets.md](../_archive/UI_Widgets.md) | Pure C++ Slate widgets |
| [WBP_PlayerNameTag.md](../_archive/WBP_PlayerNameTag.md) | NameTagSubsystem (C++) |
| [Enemy Combat System Blueprints.md](../_archive/Enemy%20Combat%20System%20Blueprints.md) | EnemySubsystem + CombatActionSubsystem (C++) |

### Other Historical
| Document | Notes |
|----------|-------|
| [Bug_Fix_Notes.md](../_archive/Bug_Fix_Notes.md) | Early bug fix notes |
| [Health_Check_Technical_Deep_Dive.md](../_archive/Health_Check_Technical_Deep_Dive.md) | Server health check analysis |
| [Ragnarok_Online_Reference.md](../_archive/Ragnarok_Online_Reference.md) | Early RO reference (superseded by RagnaCloneDocs/) |

### Daily Progress Logs

Day-by-day development logs from the foundation building phase (February 2026):

| Date | Document |
|------|----------|
| 2026-02-01 | [MMO_Development_Progress_2026-02-01.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-01.md) |
| 2026-02-02 | [MMO_Development_Progress_2026-02-02.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-02.md) |
| 2026-02-03 | [MMO_Development_Progress_2026-02-03.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-03.md) |
| 2026-02-04 | [MMO_Development_Progress_2026-02-04.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-04.md) |
| 2026-02-05 | [MMO_Development_Progress_2026-02-05.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-05.md) |
| 2026-02-06 | [MMO_Development_Progress_2026-02-06.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-06.md) |
| 2026-02-08 | [MMO_Development_Progress_2026-02-08.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-08.md) |
| 2026-02-09 | [MMO_Development_Progress_2026-02-09.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-09.md) |
| 2026-02-10 | [MMO_Development_Progress_2026-02-10.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-10.md) |
| 2026-02-11 | [MMO_Development_Progress_2026-02-11.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-11.md) |
| 2026-02-13 | [MMO_Development_Progress_2026-02-13.md](../_archive/Daily%20Progress/MMO_Development_Progress_2026-02-13.md) |

> Also in `_archive/`: [Mutiplayer Architecture.md](../_archive/Mutiplayer%20Architecture.md) (early draft with typo in filename — superseded by [Multiplayer_Architecture.md](01_Architecture/Multiplayer_Architecture.md))
