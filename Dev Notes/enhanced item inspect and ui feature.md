# deep research ragnarok online classic item UI and interactions. understand how slots in items work.

deep research ragnarok online classic item UI and interactions. understand how slots in items work.
setup tooltip on hover for all items (show simple description) 
setup right click inspect for items -show detailed description  see for ref of one with slots and one without slots: 
either should work for items in inventory or equipment screen, shops, and anywhere else you deem necessary
essentially the detailed inspect screen should show a larger version of the icon on the left, a title bar with the item name. and on the right the full detailed description, nicely formatted . if there are slots on the item, there should be a footer bar showing the available slots (some items can have 1, 2, 3, or 4 slots).
The slots are meant to hold card items, these add the card affect to the item slot.
if the slot is filled, the icon of the card should show in the slot and then the slot should have the same tooltip on hover and right click inspect functionality that all items have.
full research all these functionalities and make sure you understand 100% how this functionality works and make sure tehre are zero gaps. make a full detailed plan with everything documented in a .md file. this plan should contain all the details you need to then implement, track progress, and ensure there has been nothing missed.

claude --resume 58a3398e-4bed-458e-a5e0-1a29050885af

Key Features

TESTED****  - Hover tooltip: Consistent across inventory, equipment, shop, hotbar — shows formatted name (+7 Bloody Sword [3]), type, description, ATK/MATK/DEF/MDEF, weight, required
   level
TESTED****- Right-click inspect: RO Classic style window with large icon placeholder, formatted full_description with section dividers and label:value styling, gold dividers
 
TESTED****- - Card slot footer: Diamond ◇ for empty slots, card name text for filled slots with hover tooltip + right-click to inspect the card
TESTED****-   - Draggable title bar with DPI-correct positioning
 TESTED****-  - Hotbar preserved: Right-click still clears slots (no regression)
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
