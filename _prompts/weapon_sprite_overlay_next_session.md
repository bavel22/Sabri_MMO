# Weapon Sprite Overlay — Next Session Prompt

## Paste this into a new Claude Code session to continue:

---

I need to implement a weapon sprite overlay system for my 2D sprite-based MMO (Ragnarok Online style). The body sprites work perfectly — each animation is a separate atlas PNG at 1024px cells, rendered from 8 directions via Blender from Mixamo-rigged 3D models.

**The goal**: Render weapon sprites (dagger, sword, etc.) as transparent overlays that align pixel-perfectly with the body sprite, showing the weapon in the character's hand across all animations.

**Load these skills first**: `/sabrimmo-3d-to-2d`

**Critical context to read**:
- `memory/weapon-overlay-attempts-2026-03-25.md` — EVERYTHING we tried that didn't work (4 approaches, all failed on alignment)
- `memory/feedback-remote-sprite-rules.md` — 7 rules for remote sprite system (all implemented and working)
- `memory/sprite-pipeline-session.md` — Full sprite pipeline architecture

**What already works**:
- Body sprite system (v2 per-animation atlases, 1024px, 19 animations, weapon-mode switching, remote player sync)
- C++ equipment layer infrastructure (LoadEquipmentLayer, ResolveLayerAtlas, lazy loading, equipment change detection for local + remote)
- Server view_sprite system (DB, inventory query, player:appearance events)
- Blender .blend template rendering with Blender 5.x action_slot fix (animations play correctly)

**What doesn't work**:
- The weapon overlay sprites don't align with the body sprites because the two are rendered by different pipelines with different centering/scaling transforms (height 0.989 vs 0.853)

**The root cause**: `center_and_normalize()` in the body render script measures the character's bounding box and scales to height 2.0. The .blend template render does the same but gets a DIFFERENT height measurement (0.853 instead of 0.989) because the .blend was saved in an attack pose. Different heights → different scale factors → weapon overlay is positioned/scaled wrong relative to the body.

**Approaches NOT to retry** (all failed, documented in memory):
1. Programmatic equipment attachment with `--equip-offset/rotation/scale` (centering mismatch)
2. Subtraction method (body occlusion makes weapon invisible during attacks)
3. Separate render scripts for body vs weapon (different centering parameters)

**Most promising approach to try**:
Modify `blender_sprite_render_v2.py` to render from a .blend template directly (instead of importing a fresh FBX). The .blend file has the weapon already positioned by the user. The script would:
1. Open the .blend file (weapon attached to hand bone)
2. Apply ALL the same processing as the body render (cel-shade, outline, center_and_normalize, auto_frame_camera)
3. Render in TWO passes from the SAME scene state:
   - Pass 1: ALL meshes visible → body+weapon sprites (these become the new body sprites)
   - Pass 2: Body hidden, weapon only → weapon overlay sprites
4. Both passes use IDENTICAL centering/camera → guaranteed pixel alignment
5. The key Blender 5.x fix: `animation_data.action_slot = action.slots[0]` must be applied when switching actions

**Key files**:
- `2D animations/blender_sprite_render_v2.py` — main render script (body sprites work perfectly)
- `2D animations/render_equipment_from_blend.py` — .blend template renderer (has action_slot fix, animations work)
- `2D animations/render_blend_all_visible.py` — renders all visible meshes from .blend (has action_slot fix)
- `2D animations/3d_models/weapon_templates/dagger_on_swordsman.blend` — user-positioned dagger template
- `client/SabriMMO/Source/SabriMMO/Sprite/SpriteCharacterActor.cpp` — C++ sprite system (fully working)

**The dagger .blend file contains**:
- Armature: `Armature`
- Body mesh: `tripo_node_c27f772a-*` (parent=Armature, no parent_bone)
- Dagger mesh: `tripo_node_b49c0203-*` (parent=Armature, parent_bone=mixamorig:RightHand)
- Detection: `obj.parent_bone` distinguishes equipment from body meshes

**Research findings (from production games)**:

RO Classic uses an "anchor point" system (ACT file format) where body sprites define attachment pixel coordinates per frame, and equipment sprites are positioned at `body_position + body_anchor - equipment_anchor`. Dead Cells renders body+weapon from the SAME 3D scene as a single sprite. Stardew Valley uses hardcoded pixel offsets per frame.

**The recommended approach (from research)**: Dual-pass rendering from the SAME Blender scene:
1. Open the .blend template (weapon already positioned on hand bone)
2. Import all animation FBXs (use `action_slot` fix for Blender 5.x)
3. Center/normalize using body mesh ONLY (hide equipment during centering)
4. Set camera using body bounds ONLY
5. **Pass 1**: Body visible, equipment hidden → render body sprite
6. **Pass 2**: Body hidden, equipment visible → render weapon sprite
7. BOTH passes use IDENTICAL camera position, ortho_scale, centering — guaranteed pixel alignment
8. Output to parallel directories: `body/` and `weapon/`

This is essentially what `render_blend_all_visible.py` + `--hide-equipment` already does — but body and weapon need to be rendered as TWO outputs per frame from the SAME scene, not two SEPARATE render runs. The key is that centering/camera are computed ONCE and frozen, then both passes render from that frozen state.

**Alternative approach**: Export bone anchor positions (2D pixel coords of mixamorig:RightHand projected through camera) during body render, then position weapon sprites at runtime using the anchor offset. More flexible (equipment can be different sizes) but requires runtime compositing math.

**Start by**: Reading `memory/weapon-overlay-attempts-2026-03-25.md` for full context, then implement the dual-pass approach by modifying `render_blend_all_visible.py` to output BOTH body and weapon sprites in a single render run.

---
