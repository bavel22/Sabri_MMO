#   Plan Document Created

  Plan Document Created

claude --resume e1272615-9d2f-487b-824e-a55f2309034f

  docsNew/05_Development/BP_Bridge_Migration_Plan.md — contains everything needed:

  - Full analysis of all 14 bridges (which are redundant, which need new C++ handlers)
  - 6 phases: A (remove 10), B (hotbar:data), C (inventory:used), D (loot:drop), E (chat:receive), F (final cleanup)
  - Exact server payloads for each event
  - Implementation notes per phase
  - Testing checklists
  - File references

  How to Start the Next Conversation

  Say this to Claude Code:

  Load skills /sabrimmo-persistent-socket and /sabrimmo-ui then read the BP bridge migration plan at
  docsNew/05_Development/BP_Bridge_Migration_Plan.md. Implement Phase A (remove 10 redundant bridges from
  MultiplayerEventSubsystem). Build with SabriMMOEditor target. Then proceed to Phase B.




Done. Build succeeded, all changes applied:

  Phase A — Removed 10 redundant bridges (inventory:data/equipped/dropped/error, player:stats, hotbar:alldata,
  shop:data/bought/sold/error)

  Phase B — Added HandleHotbarData() to HotbarSubsystem for single-slot updates (hotbar:data), removed that bridge

  Bridge count: 14 → 3 (inventory:used, loot:drop, chat:receive)

  You can reopen the editor and test. Key things to verify:
  - Inventory (F6) shows items, equip/unequip/drop works
  - Stat panel (F8) updates correctly
  - Hotbar loads all slots on login, drag item/skill to slot updates correctly
  - NPC shop buy/sell works
  - HP/SP/EXP bars correct on login
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
