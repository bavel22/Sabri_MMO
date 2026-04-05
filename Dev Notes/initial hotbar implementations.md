#  1. Database: Run migration, verify \d character_hotbar shows row_index column and new PK

 1. Database: Run migration, verify \d character_hotbar shows row_index column and new PK
 2. Server: Start server, check startup logs for "row_index column verified". Test hotbar:save with rowIndex=1 via socket client
 3. Client compile: Build succeeds with all new files
 4. F5 toggle: Press F5 repeatedly — cycles through 1/2/3/4 rows visible / hidden
 5. Drag item to hotbar: Open inventory (F6), drag a consumable to a hotbar slot — icon appears, server persists
 6. Drag skill to hotbar: Open skill tree (K), drag a learned skill to hotbar — icon appears with skill level
 7. Activate slot: Press 1-9 to use items/cast skills from Row 0. Hold Alt+1-9 for Row 1
 8. Quantity sync: Use a consumable via hotbar — quantity badge updates. When stack depletes, slot auto-clears
 9. Cooldown display: Cast a skill with cooldown — overlay appears and counts down
 10. Persistence: Log out and back in — hotbar slots restored from server
 11. Keybind config: Open config panel, rebind a slot, verify new key works
 12. Multi-row drag: Each row independently draggable to any screen position
 13. Right-click clear: Right-click a slot — it clears
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
