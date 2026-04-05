# slotted cards and items name research

claude --resume 4d4152cc-5080-429a-b504-9fd1e3e9b3ac

i want you to do a deep research of ragarok online classic and see how compound cards into items changes the item names. i want you to gather all the information
    necessary on this in order to replicate this in my game. i want you to understand exactly how this functions for compound 1, 2, 3, and 4, cards in different types of items, displaying in the ui, and anything else .full research all these functionalities and make sure you understand 100% how this functionality works and make sure tehre are zero gaps. make a full detailed
  plan with everything documented in a .md file. this plan should contain all the details you need to then implement, track progress, and ensure there has been nothing
  missed. is this just visual in the UI or is this actually changing the item?



Plan written to docsNew/05_Development/Card_Naming_System_Plan.md.

  To answer your question directly: The card naming is 100% visual / client-side only. The actual item in the database is never renamed. The server stores the base item ID
  + an array of compounded card IDs, and the client constructs the display name at render time (e.g., "Triple Bloody Boned Blade [4]").

  Key findings:

  1. Each card has a fixed naming word — either a prefix ("Bloody", "Cranial", "Immune") or suffix ("of Counter", "of Hermes", "of Ghost"). This is per-card, not
  per-equipment-type.
  2. Duplicate cards get multipliers — Double/Triple/Quadruple instead of repeating the word. Applied per unique card type.
  3. Slot count [N] always shows total slots, not remaining empty slots.
  4. Good news: your infrastructure is 90% built already. The DB columns exist (just empty), the server already reads/sends the data, the client struct has the fields, and
  all widgets use GetDisplayName(). Only two things are actually needed:
    - Populate the DB with prefix/suffix text for all ~538 cards (SQL migration)
    - Fix GetDisplayName() to count duplicates and add Double/Triple/Quadruple multipliers (~40 lines of C++)




 Implementation complete. Here's what was done:

  Changes Made

  1. Database Migration — database/migrations/populate_card_naming.sql
  - 441 cards populated: 327 prefix cards, 114 suffix cards
  - Data sourced from eAthena cardprefixnametable.txt + cardpostfixnametable.txt (canonical RO Classic client files)
  - Already executed on your local DB

  2. Client GetDisplayName() Rewrite — CharacterData.h:128-196
  - Counts unique cards with a TMap<int32, FCardNaming> keyed by card ItemId
  - Tracks insertion order (slot 0 first) for deterministic display
  - Duplicate cards get multipliers: Double/Triple/Quadruple (not repeated text)
  - Prefix cards concatenated before base name, suffix cards after
  - Slot count [N] always shows total slots

  3. Server-side Reference — server/src/ro_card_prefix_suffix.js
  - Complete lookup data (for future server-side name generation if needed, e.g. chat/trade)
  - Utility functions: isPostfixCard(), getCardNamingText(), getCardCompoundName()

  What Did NOT Change (already working)

  - Server index.js — already reads card_prefix/card_suffix from DB and sends them
  - All Slate UI widgets — already call GetDisplayName()
  - InventorySubsystem JSON parsing — already parses card_prefix/card_suffix

  Examples of How Names Display

  ┌───────────────────────────────┬─────────────────────────────────────────┐
  │         Cards in Item         │              Display Name               │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ 1× Hydra in Blade             │ Bloody Blade [4]                        │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ 3× Hydra + 1× Skel Worker     │ Triple Bloody Boned Blade [4]           │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ 4× Hydra                      │ Quadruple Bloody Blade [4]              │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ +7, 2× Hydra + 2× Skel Worker │ +7 Double Bloody Double Boned Blade [4] │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ 1× Thara Frog in Buckler      │ Cranial Buckler [1]                     │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ 1× Raydric in Muffler         │ Immune Muffler [1]                      │
  ├───────────────────────────────┼─────────────────────────────────────────┤
  │ 1× Verit in Shoes (suffix)    │ Shoes of Counter [1]                    │
  └───────────────────────────────┴─────────────────────────────────────────┘

  Note: This is a header-only change in CharacterData.h — you'll need to restart the UE5 editor (not Live Coding) since it's a header modification.




 Audit Complete — All Gaps Fixed

  Files modified (audit fixes):

  ┌──────────────────────────┬────────────────────────────┬───────────────────────────────────┐
  │           File           │            Line            │                Fix                │
  ├──────────────────────────┼────────────────────────────┼───────────────────────────────────┤
  │ SEquipmentWidget.cpp:261 │ Equipment slot label       │ Item.Name → Item.GetDisplayName() │
  ├──────────────────────────┼────────────────────────────┼───────────────────────────────────┤
  │ SEquipmentWidget.cpp:435 │ Legacy tooltip (dead code) │ Item.Name → Item.GetDisplayName() │
  ├──────────────────────────┼────────────────────────────┼───────────────────────────────────┤
  │ SInventoryWidget.cpp:409 │ Legacy tooltip (dead code) │ Item.Name → Item.GetDisplayName() │
  ├──────────────────────────┼────────────────────────────┼───────────────────────────────────┤
  │ SShopWidget.cpp:900      │ Sell item list display     │ Item.Name → Item.GetDisplayName() │
  ├──────────────────────────┼────────────────────────────┼───────────────────────────────────┤
  │ SShopWidget.cpp:921      │ Sell popup title capture   │ Item.Name → Item.GetDisplayName() │
  ├──────────────────────────┼────────────────────────────┼───────────────────────────────────┤
  │ ShopSubsystem.cpp:427    │ Sell cart name storage     │ Item.Name → Item.GetDisplayName() │
  └──────────────────────────┴────────────────────────────┴───────────────────────────────────┘

  Verified as correct (no fix needed):

  ┌────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────┐
  │              File              │                                   Why correct                                    │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ ItemTooltipBuilder.cpp         │ Already uses GetDisplayName()                                                    │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ SItemInspectWidget.cpp         │ Already uses GetDisplayName()                                                    │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ SCardCompoundPopup.cpp         │ Card base names (cards don't have prefixes on themselves)                        │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ SShopWidget.cpp buy side       │ NPC shop items are never compounded                                              │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ ShopSubsystem.cpp:371 buy cart │ NPC shop items, base name correct                                                │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ HotbarSubsystem.cpp            │ Stores base name for server save (intentional)                                   │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ SInventoryWidget.cpp:319       │ 2-char icon fallback (not a name display)                                        │
  ├────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────┤
  │ Server index.js                │ All inventory:data emissions use getPlayerInventory() which includes card naming │
  └────────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────┘

  Full file change list:
  1. CharacterData.h — GetDisplayName() rewritten with multiplier system
  2. SEquipmentWidget.cpp — 2 fixes (slot label + legacy tooltip)
  3. SInventoryWidget.cpp — 1 fix (legacy tooltip)
  4. SShopWidget.cpp — 2 fixes (sell item display + sell popup title)
  5. ShopSubsystem.cpp — 1 fix (sell cart name)
  6. database/migrations/populate_card_naming.sql — 441 cards populated (already run)
  7. server/src/ro_card_prefix_suffix.js — Reference data (new file)
  8. docsNew/05_Development/Card_Naming_System_Plan.md — Full plan doc (new file)

  The system is now 100% complete across all layers. Every UI surface that displays item names will show the correct RO Classic card-prefixed names with
  Double/Triple/Quadruple multipliers.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
