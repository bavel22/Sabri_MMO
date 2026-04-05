# huge targeting revamp phase 5 plus research

claude --resume 6f800ad7-80a5-4508-8150-fa27b60fb156

Next Phase: Phase 5 (Name Tags + Interaction)

  - Replace N per-actor WBP_PlayerNameTag WidgetComponents with a single UNameTagSubsystem + Slate OnPaint overlay
  - ~400 lines new code, ~50 lines modifications
  - Full spec already exists in 04_Name_Tags_Interaction_Plan.md
  - Registration hooks into existing EnemySubsystem and OtherPlayerSubsystem



  Key Corrections from RO Classic Research

  ┌───────────────────────────────────────┬─────────────────────────────────────────────────────────────────────────┐
  │           What We Had Wrong           │                            RO Classic Actual                            │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Monster names always visible          │ Hover-only — must mouse over to see name                                │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ NPC names always visible, green       │ Hover-only, light blue #94BDF7                                          │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Party members = green name            │ Blue #5599FF                                                            │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ PvP enemies = red name                │ No color change — identified by guild emblem only                       │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Names positioned above character      │ Below character by default (/font toggles)                              │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Simple text shadow                    │ 4-pass black outline — drawn 4× at ±1px cardinal offsets                │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Monster name color = fixed            │ Level-based — grey/white/red based on level difference                  │
  │ white/yellow                          │                                                                         │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Background behind name                │ No background — floating text with outline only                         │
  ├───────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────┤
  │ Boss/MVP = gold name                  │ Same level-based coloring — bosses distinguished by sprite, not name    │
  │                                       │ color                                                                   │
  └───────────────────────────────────────┴─────────────────────────────────────────────────────────────────────────┘

  Documents Updated

  1. 08_Migration_Status_And_Next_Phase.md — Phase 5 section completely rewritten with accurate RO Classic data, hover
  integration with TargetingSubsystem, level-based color formula, exact hex values from roBrowser source
  2. 04_Name_Tags_Interaction_Plan.md — Sections 1.1-1.3 rewritten with verified pre-renewal behavior

  Phase 5 Key Architecture Change

  The biggest impact: monsters and NPCs are hover-only, which means NameTagSubsystem must integrate with
  TargetingSubsystem (which already tracks the hovered actor at 30Hz). A GetHoveredActor() public method will be added
  to TargetingSubsystem for this integration.



  New files (2):
  - UI/NameTagSubsystem.h — subsystem with registration API (RegisterEntity, UnregisterEntity, SetVisible, UpdateName)
  - UI/NameTagSubsystem.cpp — SNameTagOverlay OnPaint widget renders all name tags in one draw call at Z=7

  Modified files (2):
  - EnemySubsystem.cpp — registers enemies on spawn, hides on death, shows on respawn
  - OtherPlayerSubsystem.cpp — registers other players on spawn, unregisters on leave

  RO Classic behavior:
  - Player names: always visible, white #FFFFFF
  - Monster names: hover-only (reads from TargetingSubsystem::GetHoveredActor()), level-based color (grey/white/red)
  - NPC names: hover-only, light blue #94BDF7
  - 4-pass black outline rendering (±1px cardinal offsets)
  - Party members: blue #5599FF (ready for when party system is added)

  You can open the editor and test. Hover over monsters to see their name tags, and other players should always show
  their names.


---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
