# Sprite Quality Canonical Settings — Session Resume (2026-04-27, sRGB corrected 2026-04-28)

This document captures everything important from the sprite quality / canonical settings work session. Use it as the single source of truth when continuing.

---

## 1. The Canonical Sprite Atlas Settings (the rule)

**Every sprite atlas texture in the project MUST have these UE5 import settings.** This applies to: body atlases (every class × every gender), weapon overlays (every type × every gender), headgear top/mid/low, hair, garment, shield, and every monster/enemy atlas.

| Setting | Value | UE5 Python |
|---------|-------|-----------|
| Compression | BC7 | `unreal.TextureCompressionSettings.TC_BC7` |
| Filter | Nearest | `unreal.TextureFilter.TF_NEAREST` |
| Mip Gen Settings | SimpleAverage | `unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE` |
| Use New Mip Filter | True | `set_editor_property("use_new_mip_filter", True)` |
| Do Scale Mips For Alpha Coverage | True | `set_editor_property("do_scale_mips_for_alpha_coverage", True)` |
| Alpha Coverage Thresholds | (0, 0, 0, 0.5) | `set_editor_property("alpha_coverage_thresholds", unreal.Vector4(0.0, 0.0, 0.0, 0.5))` |
| Maximum Texture Size | 0 (no cap) | `set_editor_property("max_texture_size", 0)` |
| Never Stream | False | `set_editor_property("never_stream", False)` |
| Texture Group | UI | `unreal.TextureGroup.TEXTUREGROUP_UI` |
| **sRGB** | **True** | `set_editor_property("srgb", True)` — body sprite material samples as Color (sRGB); `srgb=False` makes the sprite invisible (verified 2026-04-28 on ambernite import) |

**Authoritative reference**: `memory/feedback-sprite-texture-group-ui.md`.

### Why each setting matters

- **Mips ON (SimpleAverage)** — without mipmaps, the runtime LODBias slider has nothing to bias.
- **Use New Mip Filter ON** — UE5's modern processing pipeline; required for proper alpha-coverage mips.
- **Alpha Coverage W=0.5** — pixel-art sprites have hard transparent edges; without coverage scaling, lower mips get halos. W=0.5 preserves the count of alpha ≥ 0.5 pixels across mip levels.
- **MaxTextureSize=0** — a fixed cap (e.g. 4096) downscales non-square atlases (walk 8192×12288, attack 8192×10240) by a different factor than square idle atlases (8192×8192) → walking sprite looks blurrier than idle. With 0, the mip pyramid handles resolution uniformly.
- **NeverStream=false** — with `NeverStream=true`, UE5 keeps ALL mips of every loaded texture resident regardless of LODBias. The slider would change visual quality but NOT memory. With `NeverStream=false`, the streaming system honors LODBias and unloads high mips, dropping VRAM by 4×–64× per tier.

### Forbidden combinations

- `never_stream=True` + `mip_gen_settings=SimpleAverage` — slider changes visuals but not VRAM.
- `mip_gen_settings=NoMipmaps` — slider is cosmetic only; quality fixed at source resolution.
- `max_texture_size=4096` (or any fixed cap) — non-square atlases downscale unevenly.
- `do_scale_mips_for_alpha_coverage=False` with mips on — visible halos around sprite edges.
- `lod_group != TEXTUREGROUP_UI` — wrong memory pool, wrong LOD bias defaults.

---

## 2. Runtime Sprite Quality Slider System

Located in **Options > Video > Sprite Quality**. 5 tiers, each maps to a `LODBias`:

| Tier | LODBias | Use Case |
|------|---------|----------|
| Ultra | 0 | High-end GPUs, full source resolution |
| High | 1 | Balanced |
| **Medium** (default) | **2** | Default for most users |
| Low | 3 | Older / mobile GPUs |
| Very Low | 4 | Minimum-spec rescue mode |

**Implementation files:**
- `client/SabriMMO/Source/SabriMMO/Sprite/SpriteAtlasData.h/.cpp` — static `GlobalLODBias`, applied in `EnsureTextureLoaded`
- `client/SabriMMO/Source/SabriMMO/UI/OptionsSubsystem.h/.cpp` — `GetSpriteQuality/SetSpriteQuality` (clamp 0-4), `ApplySpriteQualityToLoadedTextures` iterates `/Game/SabriMMO/Sprites/Atlases/`
- `client/SabriMMO/Source/SabriMMO/UI/SOptionsWidget.cpp` — dropdown
- Default `iSpriteQuality = 2` (Medium)

---

## 3. ZonePreloadSubsystem (async loading)

`client/SabriMMO/Source/SabriMMO/UI/ZonePreloadSubsystem.h/.cpp`

Per-world subsystem, **3-tier handle system**:
1. **PinnedHandles** — local player class always pinned
2. **ActiveHandles** — current zone + adjacent
3. **LruCache** — 4 GB budget

**APIs**: `RequestClassPreload`, `RequestLayerPreload`, `PinClass`, `BeginZone`, `HandleAdjacentClasses` (predictive preload). Built on `FStreamableManager` + `FStreamableHandle`.

Server emits `zone:adjacent_classes` after `zone:ready` (in `server/src/index.js`) so the client can preload neighbour-zone class atlases before they're needed.

---

## 4. Path C — Deferred Equipment Swap

Fixes the pop-in/pop-out flicker when changing weapons or hair.

`client/SabriMMO/Source/SabriMMO/Sprite/SpriteCharacterActor.h/.cpp`:
- Added `FSpriteLayerState::PendingLayerAtlasRegistry`, `PendingSwapTexture`, `PendingSwapTimeoutSeconds`
- Added `FinalizeEquipmentSwap()`
- `LoadEquipmentLayer` uses a temp registry; if the new texture is still streaming, defers the swap
- `Tick` polls pending swaps; finalizes on streaming complete or 1s timeout

**Critical helmet-flicker fix**: only create a new `MaterialInst` if `!IsValid(L.MaterialInst)`. Without this guard, `LoadEquipmentLayer` created a fresh dynamic material instance on every call (e.g. on inventory open), causing a one-frame flash.

---

## 5. Migration Script Inventory

All in `client/SabriMMO/Scripts/Environment/`. **All files updated this session to use the canonical settings.**

### Phase migration (apply settings to existing atlases)
- `migrate_sprite_quality_test.py` — small subset (~356 atlases): priest_f body, all female weapons, hair style_01 female, 5 enemies. Run first to validate.
- `migrate_sprite_quality_full.py` — every sprite atlas (~2,700). Wave-based, idempotent, safe to interrupt.
- `migrate_test_01_priest.py` … `migrate_test_07_enemies.py` — split version of test phase, one batch per script (priest, weapons A/B/C/D, hair, enemies). Use these if the single-script version OOMs.
- `migrate_test_verify.py` — verifies every test atlas matches canonical settings AND has multiple mip levels (smoking-gun test that the recompile finished).

### Recovery (re-import from PNG when atlases are broken)
- `migrate_test_recover.py` — re-imports the 356 test atlases.
- `migrate_recover_01_priest.py` … `migrate_recover_07_enemies.py` — split per-batch recovery; same scope as the test phase. All use `apply_canonical_settings(asset_path)` (renamed from `apply_original_settings` this session).

### Streaming toggle (deprecated — settings now applied via the migration scripts)
- `enable_streaming_*.py` (18 scripts) — flipped `never_stream=True → False` project-wide. Was used after the project-wide settings change to free VRAM.
- `clear_maxsize_*.py` (17 scripts) — set `max_texture_size = 0` project-wide. Was used after discovering that fixed caps caused non-square downscale issues.

### One-off import scripts (header docs updated, code already correct)
- `import_batch3.py` … `import_batch9.py`, `import_batch10.py`–`13.py`, `import_new_batch.py`, `import_enemy_sprites.py`, `reimport_hermit_plant.py`, all `import_weapon_*.py`, `fix_knuckle_atlas_settings.py`, `2D animations/scripts/import_knuckle_f.py`.

---

## 6. What Was Done This Session

- Established canonical settings (table above) and applied them across the project.
- Built the runtime Sprite Quality slider, ZonePreloadSubsystem, Path C deferred swap.
- Fixed helmet flicker via the `IsValid(MaterialInst)` guard.
- Despawned high-level enemies in `prt_south` for VRAM testing (commented out in `server/src/index.js` — restorable).
- **Documentation sweep (last task)** — updated every sprite-atlas reference in:
  - 7 `migrate_recover_*.py` scripts (renamed function `apply_original_settings → apply_canonical_settings`).
  - `migrate_test_recover.py` (function renamed + docstring + log lines).
  - `migrate_sprite_quality_full.py` + `migrate_sprite_quality_test.py` (removed half-source `MaxTextureSize` cap, flipped `never_stream=True → False`, new idempotency check).
  - 7 `migrate_test_*.py` phase scripts (same change).
  - `migrate_test_verify.py` (inverted `max_texture_size`/`never_stream` checks; added `lod_group` check).
  - 12 import scripts (`import_batch3..9`, `import_new_batch`, `import_enemy_sprites`, 3 weapon imports, `reimport_hermit_plant`, `fix_knuckle_atlas_settings`, `2D animations/scripts/import_knuckle_f.py`) — docstrings updated, code was already correct.
  - 2 prompt files (`_prompts/female_weapon_atlas_prompt.md`, `_prompts/resume_sprite_pipeline_session.md`).
  - Memory file `feedback-sprite-texture-group-ui.md` is the authoritative reference.

**NOT changed (different domain — UI Icon/Panel, not sprite atlases):**
- Loading screen / world map / damage number crit-starburst textures (UI Icons, no sprite mip pyramid needed).
- Historical journal entries.
- `RagnaCloneDocs/10_Art_Animation_VFX_Pipeline.md` UI Icon/Panel rows.

---

## 7. Pitfalls / Gotchas Learned

- **`srgb` MUST be True for sprite atlases** — the body sprite material's `TextureSampleParameter2D` uses Sampler type=`Color` (sRGB-expecting). Setting `srgb=False` makes UE5 log `"Sampler type is Color, should be Linear Color"` and the texture is sampled as all-zeros → invisible sprite. The original 2026-04-27 canonical rule incorrectly listed `srgb=False`; corrected 2026-04-28 after 4 enemy folders rendered invisible until flipped.
- `TMGS_ALPHA_COVERAGE` does **not** exist in UE5.7 — use `TMGS_SIMPLE_AVERAGE` + `do_scale_mips_for_alpha_coverage` flag instead.
- UE5 Python strips `b_` prefix from boolean UPROPERTYs: C++ `bDoScaleMipsForAlphaCoverage` → Python `do_scale_mips_for_alpha_coverage`.
- `imported_size` does not exist in UE5.7 — use `blueprint_get_size_x/y` (returns the built texture size).
- UE5 force-rounds non-power-of-2 `max_texture_size` values to the next power of 2 (entered 6144 → got 8192). Use 0 to avoid this entirely.
- Walk/attack atlases blurred more than idle when `MaxTextureSize=4096` was applied — non-square atlases downscale by different factors. Fixed by removing the cap.
- `NeverStream=true` keeps every mip resident regardless of LODBias. Slider becomes cosmetic. Must be `false`.
- User's VCS is **Diversion**, not Dropbox — recovery scripts clean up `.dv-conflict*.uasset` files.
- Big project-wide scripts hang the editor; split into 150–250 atlases per script.
- The user repeatedly emphasizes: **always verify before claiming**. Read filesystem, don't quote stale memory.

---

## 8. Pending / Possible Next Steps

- Re-test the slider end-to-end at every tier (Ultra/High/Medium/Low/Very Low) on busy zones to confirm VRAM curves and visual quality.
- Decide whether to re-spawn the prt_south high-level enemies (currently commented out in `server/src/index.js`).
- Run `migrate_test_verify.py` after any future migration to confirm canonical settings everywhere + multi-mip presence.
- If memory still spikes, audit `ZonePreloadSubsystem` LRU eviction telemetry.

---

## 9. Key Files (quick reference)

| File | Purpose |
|------|---------|
| `memory/feedback-sprite-texture-group-ui.md` | **Authoritative canonical settings rule** |
| `memory/sprite-quality-slider-system.md` | Slider + ZonePreloadSubsystem + Path C documentation |
| `client/SabriMMO/Source/SabriMMO/Sprite/SpriteAtlasData.h/.cpp` | `GlobalLODBias`, `EnsureTextureLoaded` |
| `client/SabriMMO/Source/SabriMMO/Sprite/SpriteCharacterActor.h/.cpp` | Path C deferred swap, helmet flicker fix |
| `client/SabriMMO/Source/SabriMMO/UI/ZonePreloadSubsystem.h/.cpp` | Async preload + LRU cache |
| `client/SabriMMO/Source/SabriMMO/UI/OptionsSubsystem.h/.cpp` | `GetSpriteQuality/SetSpriteQuality`, `ApplySpriteQualityToLoadedTextures` |
| `client/SabriMMO/Source/SabriMMO/UI/SOptionsWidget.cpp` | Sprite Quality dropdown |
| `server/src/index.js` | `zone:adjacent_classes` emit |
