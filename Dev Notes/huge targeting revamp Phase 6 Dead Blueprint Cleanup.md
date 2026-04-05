# Phase 6: Dead Blueprint Cleanup — Step-by-Step Instructions

Phase 6: Dead Blueprint Cleanup — Step-by-Step Instructions

  Do these in order. Save after each step. Test between major steps.

  ---
  STEP 1: Delete BP_OtherPlayerManager from ALL levels

  This actor is dead code — UOtherPlayerSubsystem replaced it in Phase 3.

  1. Open each level that has BP_OtherPlayerManager placed:
    - L_Prontera (prontera_south)
    - L_PrtSouth (if separate)
    - Check ALL zone levels: L_Geffen, L_Payon, etc.
  2. In the World Outliner, search for OtherPlayerManager
  3. Select the actor → press Delete
  4. Save the level (Ctrl+S)
  5. Repeat for every level

  STEP 2: Delete BP_EnemyManager from ALL levels

  Same process — UEnemySubsystem replaced it in Phase 3.

  1. In each level's World Outliner, search for EnemyManager
  2. Select → Delete
  3. Save the level

  STEP 3: Remove dead BP functions from BP_SocketManager

  These 17 functions are NEVER called — events were migrated to C++ subsystems in Phases 2-3. DO NOT delete the 14
  active functions (inventory, shop, chat, stats, hotbar, loot).

  1. Open BP_SocketManager (Content Browser → Content/SabriMMO/Blueprints/BP_SocketManager)
  2. In the My Blueprint panel → Functions section, right-click and Delete each of these:

  Player events (Phase 3 — replaced by OtherPlayerSubsystem):
  - OnPlayerMoved (38 nodes)
  - OnPlayerLeft (10 nodes)

  Combat events (Phase 2 — replaced by CombatActionSubsystem):
  - OnCombatDamage (221 nodes)
  - OnHealthUpdate (50 nodes)
  - OnCombatDeath (25 nodes)
  - OnCombatRespawn (78 nodes)
  - OnAutoAttackStarted (32 nodes)
  - OnAutoAttackStopped (33 nodes)
  - OnTargetLost (33 nodes)
  - OnCombatOutOfRange (30 nodes)
  - OnCombatError (3 nodes)

  Enemy events (Phase 3 — replaced by EnemySubsystem):
  - OnEnemySpawn (41 nodes)
  - OnEnemyMove (49 nodes)
  - OnEnemyDeath (45 nodes)
  - OnEnemyHealthUpdate (42 nodes)

  Also remove if present:
  - FindRemotePlayer (11 nodes) — used by dead OnPlayerMoved
  - EmitPositionUpdate (25 nodes) — replaced by PositionBroadcastSubsystem

  DO NOT DELETE these 14 — they are still actively called:
  - OnInventoryData, OnItemUsed, OnItemEquipped, OnItemDropped, OnInventoryError
  - OnLootDrop
  - OnChatReceived
  - OnPlayerStats
  - OnHotbarData, OnHotbarAllData
  - OnShopData, OnShopBought, OnShopSold, OnShopError

  3. Compile the Blueprint (toolbar → Compile button)
  4. Save the Blueprint

  STEP 4: Remove NameTagWidget WidgetComponent from BP actors

  UNameTagSubsystem now renders all name tags via a single OnPaint overlay.

  BP_MMOCharacter:
  1. Open BP_MMOCharacter → Components panel
  2. Find NameTagWidget (WidgetComponent)
  3. Select → Delete
  4. Check Event Graph → delete any nodes that reference NameTagWidget (Get NameTagWidget → Cast To WBP_PlayerNameTag →
  SetPlayerName). These are typically in BeginPlay.
  5. Compile + Save

  BP_OtherPlayerCharacter:
  1. Open BP_OtherPlayerCharacter → Components panel
  2. Find NameTagWidget (WidgetComponent)
  3. Select → Delete
  4. Check Event Graph → BeginPlay has Delay → Get NameTagWidget → Cast → SetPlayerName. Delete those nodes.
  5. Compile + Save

  BP_EnemyCharacter:
  1. Open BP_EnemyCharacter → Components panel
  2. Find NameTagWidget (WidgetComponent)
  3. Select → Delete
  4. Check Event Graph → BeginPlay has Delay → Get NameTagWidget → Cast → SetPlayerName. Delete those nodes.
  5. Compile + Save

  STEP 5: Delete WBP_PlayerNameTag widget asset

  1. Content Browser → navigate to the WBP_PlayerNameTag asset (search for it)
  2. Right-click → Delete
  3. If it warns about references, those references should already be removed from Step 4. Click through.

  STEP 6: Remove dead components from BP_MMOCharacter (OPTIONAL — do carefully)

  Only do this if you're confident these components aren't used by ANY remaining Blueprint logic:

  - AC_TargetingSystem — replaced by UTargetingSubsystem (Phase 2)
  - AC_CameraController — replaced by UCameraSubsystem (Phase 1)

  1. Open BP_MMOCharacter → Components panel
  2. Find AC_TargetingSystem → Select → Delete
  3. Find AC_CameraController → Select → Delete
  4. Check Event Graph for any broken references (red "Error" nodes). Delete them.
  5. Compile + Save

  Warning: If any Blueprint logic still calls functions on these components, you'll get "Accessed None" errors. If you
  see red nodes after deleting, undo (Ctrl+Z) and skip this step.

  ---
  Testing Checklist

  After completing Phase 6, test these items:

  Core Gameplay (must work)

  - Login → character select → enter game world (no crash)
  - Player appears at correct spawn position
  - WASD movement works
  - Click-to-move works (NavMesh pathfinding)
  - Camera — right-click rotation, scroll zoom
  - Zone transition — warp to another zone and back (entities clean up + rebuild)

  Entity Management (Phase 3 — replaces BP managers)

  - Enemies spawn when entering a zone
  - Enemies move (AI patrol)
  - Enemy death — animation plays, name tag hides
  - ********Enemy respawn — reappears, name tag re-shows
  - Other players appear when joining same zone (test with 2nd client)
  - Other players disappear when leaving

  Combat (Phase 2)

  - Click enemy to attack — player walks to range, attack starts
  - Ranged attack (bow) — attacks from distance, not melee
  - Target frame appears showing enemy name + HP
  - Death — death overlay shows, movement blocked
  - Respawn — death overlay disappears, movement restored
  - Enemy attack animation plays when enemy attacks you

  Name Tags (Phase 5)

  - Local player name visible above head
  - Other player names always visible
  - Monster names — appear ONLY on hover (RO Classic behavior)
  - NPC names — appear ONLY on hover (Kafra, Shop NPCs)
  - Monster name color — grey/white/red based on level difference
  - NPC name color — light blue
  - Name tags disappear when monster dies
  - Name tags reappear when monster respawns

  HUD & UI (should still work — BP bridges active)

  - Inventory (F6) — opens, shows items
  - Equipment (F7) — opens, shows equipped items
  - Hotbar (F5) — skills/items, usable
  - Stat panel (F8) — shows combat stats
  - Shop — talk to shop NPC, buy/sell works
  - Chat — send/receive messages
  - Buff/debuff bars — visible when buffs active

  Hover & Targeting (Phase 2)

  - Hover cursor — crosshair on enemies, text beam on NPCs
  - Hover indicators — highlight on hovered actor




Blueprint-to-C++ Migration — ALL 6 PHASES COMPLETE

  ┌─────────┬────────────────────────────────┬──────────┐
  │  Phase  │             Scope              │  Status  │
  ├─────────┼────────────────────────────────┼──────────┤
  │ Phase 4 │ Persistent socket, EventRouter │ COMPLETE │
  ├─────────┼────────────────────────────────┼──────────┤
  │ Phase 1 │ Camera, input, movement        │ COMPLETE │
  ├─────────┼────────────────────────────────┼──────────┤
  │ Phase 2 │ Targeting, combat actions      │ COMPLETE │
  ├─────────┼────────────────────────────────┼──────────┤
  │ Phase 3 │ Entity management              │ COMPLETE │
  ├─────────┼────────────────────────────────┼──────────┤
  │ Phase 5 │ RO Classic name tags           │ COMPLETE │
  ├─────────┼────────────────────────────────┼──────────┤
  │ Phase 6 │ Dead BP cleanup                │ COMPLETE │
  └─────────┴────────────────────────────────┴──────────┘

  What was achieved

  - 25 C++ subsystems running all gameplay logic
  - BP_SocketManager reduced from 31 bridges to 14 (inventory, shop, chat, stats, hotbar, loot)
  - ~700+ BP nodes replaced with C++ (OnCombatDamage 221, BP managers ~300, input ~200)
  - Dead code removed: BP_OtherPlayerManager, BP_EnemyManager, AC_TargetingSystem, AC_CameraController,
  WBP_PlayerNameTag, 17 dead BP_SocketManager functions
  - Server fix: zone:ready sends health to player's own socket
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
