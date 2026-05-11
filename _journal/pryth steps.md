Here's the step-by-step from where you are. I'll cover what to do now, what's optional, and what to skip until later.

  Phase 1 — Verify textures imported (critical)

  The brick and pebble textures need to be in UE5's Content Browser as .uasset files, not just sitting as PNGs on disk.

  1. Open UE5 Content Browser → navigate to Content/SabriMMO/Textures/Environment/Ground/
  2. Look for:
    - T_Brick_RO
    - T_Pebble_RO
  3. If both are present → skip to Phase 2.
  4. If missing:
    a. Open Windows File Explorer to C:\Sabri_MMO\_tools\ground_texture_output\
    b. Drag T_Brick_RO.png and T_Pebble_RO.png directly into the UE5 Content Browser at
  Content/SabriMMO/Textures/Environment/Ground/
    c. UE5 shows an import dialog → leave defaults → click "Import All"
    d. Right-click each new texture in Content Browser → Save
    e. Re-run the MI script to populate the now-existing texture slots:
  exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_mi_paved_town.py").read())
    f. Watch the Output Log — should see [OK] BrickTexture = ... and [OK] PebbleTexture = ... instead of [-] skipped

  Phase 2 — Create paint Layer Infos

  This tells UE5 to track per-pixel weights for each paint layer.

  1. Select your Landscape Actor in the level
  2. Press Shift+2 (or Modes panel → Landscape) to enter Landscape Mode
  3. Click the Paint tab (between Sculpt and Manage)
  4. The Target Layers panel on the right should show 6 layer names that come from M_Landscape_RO_18:
    - Brick
    - Cobble
    - Pebble
    - GrassDense
    - FlowerPatch
    - Debris

  Each will have a + icon next to it (no Layer Info yet).
  5. For each of the 3 pavement layers (Brick, Cobble, Pebble):
    a. Click the + icon next to the layer name
    b. Choose "Weight-Blended Layer (Normal)" (NOT non-weight-blended)
    c. UE5 prompts for a save location → save into Content/SabriMMO/Materials/Environment/v3/LayerInfos/ (create the folder if
   it doesn't exist)
    d. Suggested file names:
        - LI_Brick
      - LI_Cobble
      - LI_Pebble
    e. Click Save

  Repeat for all 3.
  6. Skip GrassDense / FlowerPatch / Debris for now — those are for grass sprite scatter and need the manual Grass Output node
   first (see Phase 5, deferred).

  Phase 3 — Paint a base layer first (important)

  UE5 paint layers stack — before painting Brick or Pebble, you MUST paint the entire Landscape with Cobble first, otherwise
  the unpainted areas show as black/error material.

  1. In the Target Layers panel, click the "Cobble" layer to select it
  2. Brush settings (top of viewport):
    - Brush Size: ~2000 (large, fills fast). Use [ ] to resize live.
    - Brush Falloff: 0.5 (medium soft edges)
    - Tool Strength: 1.0 (full opacity)
  3. Left-click and drag across the entire Landscape to paint the whole thing as Cobble. The viewport updates in real-time —
  you should see the cobblestone texture appear.
  4. Verify: rotate the camera, fly around — there should be no black/missing areas. If there are, paint over them with
  Cobble.

  Phase 4 — Paint the districts

  Now layer Brick on the plaza, Pebble on side paths.

  4a. Paint the plaza floor as Brick

  1. Click the Brick layer in Target Layers
  2. Reduce Brush Size to ~600-1000 (plaza-sized, not whole-town)
  3. Brush Falloff: 0.3 (sharper edges for more rectangular plaza)
  4. Aim brush at the plaza area (center of town, around (0, 0) if you used the layout from §10.1 of the build plan)
  5. Left-click and drag to paint
  6. The brick texture fades in as you paint
  7. Adjust shape by painting more, or Shift+Click+drag to erase

  4b. Paint side paths as Pebble

  8. Click Pebble layer
  9. Brush Size: ~300 (path-width)
  10. Brush Falloff: 0.6 (soft edges so paths blend into surrounding Cobble)
  11. Paint thin strips between residential buildings, around the well, alleys
  12. Adjust as needed

  4c. Tweak boundaries

  - Brush Strength: 0.5 to soften edges between layers
  - Paint over harsh boundaries to add gradient
  - Shift+Click on any layer to erase

  Phase 5 — DEFERRED: Grass Output node (manual)

  This is for adding 3D grass sprite scatter to residential side-yards. Not needed yet — defer until your level is mostly
  built and you want the polish.

  When you're ready:
  1. Open M_Landscape_RO_18 in Material Editor (double-click the asset)
  2. Right-click in empty space → search "Grass Output" → add LandscapeGrassOutput node
  3. Connect the existing pin outputs to grass varieties:
    - grass_dense_final (around line 752 in the script — visible as a node) → first grass variety input
    - flower_final → flower variety input
    - debris_final → debris/leaf input
  4. For each input, set the Grass Type to one of the existing GT_V3_* assets (e.g., GT_V3_Grassland for GrassDense)
  5. Click Apply + Save
  6. Back in Landscape Paint mode, create Layer Infos for GrassDense / FlowerPatch / Debris (same way as Phase 2)
  7. Paint residential side-yards as GrassDense → 3D grass sprites spawn

  Why this is manual: UE5's Python API can't configure LandscapeGrassOutput — it crashes the material compile. This is a
  documented limitation.

  Phase 6 — Test in-game

  8. Save the level (Ctrl+S)
  9. Click Play (or Alt+P) to enter PIE
  10. Walk across the painted districts
  11. Verify the textures look right at player camera height/angle
  12. Check seams between paint layers — if they're harsh, return to Paint mode and brush over with low strength

  Common issues + fixes

  Symptom: Black or default-grey area
  Likely cause: Layer painted but Layer Info not created
  Fix: Phase 2 step 5 — create the Layer Info
  ────────────────────────────────────────
  Symptom: Black in painted area
  Likely cause: Texture slot empty on MI
  Fix: Phase 1 step 4 — import texture, re-run MI script
  ────────────────────────────────────────
  Symptom: Painted Brick area shows cobble
  Likely cause: Layer name mismatch with material parameter
  Fix: Verify Layer Info name matches Brick exactly (case-sensitive)
  ────────────────────────────────────────
  Symptom: Hard rectangular edges between paints
  Likely cause: Brush Falloff too low
  Fix: Raise to 0.5+
  ────────────────────────────────────────
  Symptom: Whole Landscape uniform, paint does nothing
  Likely cause: Material isn't M_Landscape_RO_18 (or MI parent)
  Fix: Open MI, check Parent = M_Landscape_RO_18
  ────────────────────────────────────────
  Symptom: All paint disappears when Cobble selected
  Likely cause: Painting WITH Cobble layer set to 0 strength
  Fix: Verify Tool Strength = 1.0
  ────────────────────────────────────────
  Symptom: Tiles visible at distance
  Likely cause: Camera-distance LOD issue
  Fix: Normal — Landscape uses LOD; reduces detail at distance
  ────────────────────────────────────────
  Symptom: One area looks darker than rest
  Likely cause: MacroNoiseScale working as intended
  Fix: This is desired — adds organic variation

  What to do after Phase 4 lands

  Once you have Cobble + Brick + Pebble painted convincingly:

  13. Save the level + map (Ctrl+Shift+S to save all)
  14. Take a top-down screenshot for reference (View → Toggle Game View → orbit camera up)
  15. Decide on Phase 5 (grass sprites) — only needed if you want visible grass in residential side-yards
  16. Move to Phase F (AWaterArea north moat) — separate task, doesn't affect paint layers
  17. Move to Phase E (3D prop placement) — buildings, fountain, statue, market stalls

  If you're ready to move to the next zone-building phase, let me know which you want to tackle:
  - Phase A (server registration) — register Pryth in ZONE_REGISTRY so you can warp to it
  - Phase F (north moat AWaterArea)
  - Phase E (3D props from the imported library)

  Or if you hit any issue in Phase 1-4, paste the error / screenshot and I'll diagnose.