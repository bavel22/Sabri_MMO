# applying card icons to all cards and where they should display

Resume this session with:
claude --resume c68817f7-6517-4997-bea0-4fcfc6634d47


  1. PNG copied to client/SabriMMO/Content/SabriMMO/Assets/Item_Icons/cards/Icon_Card.png

  2. GetOrCreateItemIconBrush() modified in InventorySubsystem.cpp (line 340-364) — any icon name ending in _card now routes to
  /Game/SabriMMO/Assets/Item_Icons/cards/Icon_Card instead of trying to find individual per-card assets.

  Coverage — all card display locations

  All of these call GetOrCreateItemIconBrush(), so they'll all show the card icon automatically:

  ┌────────────────────────────────────────┬─────────────────────────────────────┬─────────┐
  │                Location                │               Widget                │  Size   │
  ├────────────────────────────────────────┼─────────────────────────────────────┼─────────┤
  │ Inventory grid (Etc tab)               │ SInventoryWidget                    │ 28x28   │
  ├────────────────────────────────────────┼─────────────────────────────────────┼─────────┤
  │ Right-click item inspect               │ SItemInspectWidget                  │ 128x128 │
  ├────────────────────────────────────────┼─────────────────────────────────────┼─────────┤
  │ Card compound popup (card row)         │ SCardCompoundPopup                  │ 20x20   │
  ├────────────────────────────────────────┼─────────────────────────────────────┼─────────┤
  │ Shop buy/sell list                     │ SShopWidget                         │ 24x24   │
  ├────────────────────────────────────────┼─────────────────────────────────────┼─────────┤
  │ Filled card slot → right-click inspect │ SItemInspectWidget via FromCardInfo │ 128x128 │
  ├────────────────────────────────────────┼─────────────────────────────────────┼─────────┤
  │ Hover tooltip (if icon added later)    │ ItemTooltipBuilder                  │ —       │
  └────────────────────────────────────────┴─────────────────────────────────────┴─────────┘

  You need to do in UE5 Editor

  1. Open the UE5 Editor
  2. Navigate to Content/SabriMMO/Assets/Item_Icons/cards/
  3. The Icon_Card.png should auto-import, or right-click → Import the PNG
  4. After import, select the texture and set these mandatory settings:
    - Compression Settings → BC7
    - Mip Gen Settings → NoMipmaps
    - Texture Group → UI
    - Never Stream → true (checked)
  5. Save the asset

  Once imported, all 538 card items will display your blue diamond card icon everywhere in the game. The cache means the texture is loaded only once and reused for every
  card.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
