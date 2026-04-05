# Item No-Equip Restrictions

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Inventory_System](../03_Server_Side/Inventory_System.md)
> **Source**: rAthena `item_noequip.txt` format
> **Status**: Reference data — not yet implemented in server

Forbidden Items Database — defines restrictions on equipment, items, and cards in map types and zones.

## Structure

```
ItemID,Flag
```

## Flag Legend (Bitmask)

| Flag | Restriction |
|------|-------------|
| 1 | Restricted in normal maps |
| 2 | Restricted in PVP |
| 4 | Restricted in GVG |
| 8 | Restricted in Battlegrounds |
| 16 | Restricted in WoE:TE castles |
| 32 | Restricted in zone 1 |
| 64 | Restricted in zone 2 |

> **Future**: When PvP/WoE is implemented, this data will be used to enforce equipment restrictions per map type. See [PvP_Guild_WoE](../../RagnaCloneDocs/08_PvP_Guild_WoE.md).
