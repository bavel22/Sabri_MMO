# card system implementation

card system implementation

claude --resume 4a1da25f-f1c2-4b18-8ab1-04c6a52a50d6


  Files Created

  ┌───────────────────────────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────────────┐
  │                       File                        │                                          Purpose                                          │
  ├───────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────────┤
  │ server/src/ro_card_effects.js                     │ Card script parser — parses 538 cards' rAthena scripts into structured effects at startup │
  ├───────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────────┤
  │ database/migrations/add_card_compound_support.sql │ Populates card_type column, adds index for equipped cards                                 │
  ├───────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────────┤
  │ ~/.claude/skills/sabrimmo-cards.md                │ Full skill documentation                                                                  │
  └───────────────────────────────────────────────────┴───────────────────────────────────────────────────────────────────────────────────────────┘

  Files Modified

  ┌──────────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
  │               File               │                                                              Changes                                                              │
  ├──────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
  │                                  │ Import ro_card_effects, rebuildCardBonuses() function, card:compound handler, getEffectiveStats() merges cardBonuses,             │
  │ server/src/index.js              │ getPlayerTargetInfo() includes cardDefMods, player:join calls rebuildCardBonuses, equip/unequip calls rebuildCardBonuses, enemy   │
  │                                  │ attackerInfo includes size                                                                                                        │
  ├──────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
  │ server/src/ro_damage_formulas.js │ Step 8c defensive card mods, bonusMaxHpRate/bonusMaxSpRate in calculateDerivedStats()                                             │
  ├──────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
  │ CLAUDE.md                        │ Added /sabrimmo-cards to skill tables                                                                                             │
  └──────────────────────────────────┴───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  What's Now Functional

  1. Card compounding (card:compound event) — validate, insert, consume card, permanent
  2. Card stat bonuses on equip — flat stats (STR/VIT/ATK/DEF/MaxHP/etc.) from DB columns
  3. Offensive card combat mods — race/element/size % damage bonuses (Hydra, Vadon, Minorous, etc.)
  4. Defensive card combat mods — race/element/size % damage reduction (Thara Frog, Raydric, etc.)
  5. Armor element cards — Ghostring (ghost), Pasana (fire), Swordfish (water), etc.
  6. Percentage HP/SP cards — Pecopeco (+10% HP), Ghostring (-25% HP), Tao Gunka (+100% HP)
  7. Full stat recalculation on compound/equip/unequip/join



part 2 - compounding

deep research how this ui should work. see https://irowiki.org/wiki/Card_System#:~:text=By%20double%2Dclicking%20the%20card,a%20card%20to%20go%20in. The equipment has
  to be slotted and unequipped in your inventory. The card needs to be in your inventory as well. It also has to be compatible with your equipment, like a Mantis card
  goes into an accessory, so you wouldn't be able to put it in anything else. Double click the card and you will be able to add it to your equips. do full research on
  this and make sure is fully functional. deep reserach rangarok online classic to ensure you fully understand how this works. there should be no gaps in this. should
  work like 1 above but instead of right click, it should be double click on the card. example: Double click on the card. If you have any piece of equipment that the card
  can be put onto with an empty slot, you can choose to compound it onto the equipment. Example: You have 1 Mantis Card, and 1 Glove [1[. When you double click on the
  Mantis card, which is for accessories, then there will be a box for you to choose to put the Mantis card on Glove [1]




  New Files

  - UI/SCardCompoundPopup.h — Popup widget class declaration
  - UI/SCardCompoundPopup.cpp — Full popup implementation:
    - RO brown/gold 3-layer frame centered on screen
    - Semi-transparent backdrop (click outside to dismiss)
    - Title bar with card icon + name + close button
    - Scrollable list of eligible equipment (icon + display name + slot diamonds)
    - Click equipment row → instant compound (no confirmation, matches RO Classic)
    - Escape key dismisses
    - Status message area for server error feedback

  Modified Files

  - UI/InventorySubsystem.h — Added compound methods, card:result handler, popup state members
  - UI/InventorySubsystem.cpp — Added:
    - card:result event registration in OnWorldBeginPlay
    - FindEligibleEquipment() — filters inventory for matching unequipped slotted equipment
    - BeginCardCompound() — shows popup at Z=23
    - HideCardCompoundPopup() — removes overlay
    - EmitCardCompound() — sends card:compound socket event
    - HandleCardResult() — auto-closes on success, shows error on failure
    - Cleanup in Deinitialize()
  - UI/SInventoryWidget.cpp — Added IsCard() branch in OnMouseButtonDoubleClick (line 850)

  Flow

  1. Player double-clicks a card on the Etc tab in inventory
  2. Client filters for unequipped equipment with matching slot type + open card slots
  3. If none found → nothing happens (RO Classic silent behavior)
  4. If found → centered popup appears with equipment list
  5. Player clicks equipment → card:compound emitted with { cardInventoryId, equipInventoryId, slotIndex }
  6. Server responds card:result → popup auto-closes on success
  7. Server sends inventory:data → grid rebuilds showing card consumed + equipment updated
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
