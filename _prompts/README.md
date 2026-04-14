# Prompt Library

Reusable prompts that have produced good results with Claude Code.

## How to Use
1. Copy the prompt from any file here
2. Paste into Claude Code
3. Modify the specifics (class name, system name, etc.)

## Categories

### Research Prompts
- [GENERIC RESEARCH PROMPT.md](GENERIC%20RESEARCH%20PROMPT.md) — Deep research template for any RO Classic system
- [system review and research agaisnt ragnarok online.md](system%20review%20and%20research%20agaisnt%20ragnarok%20online.md) — System review vs RO Classic
- [items research.md](items%20research.md) — Item system research
- [jobs research.md](jobs%20research.md) — Job/class system research

### Implementation Prompts
- [can you make a plan to fully imple.md](%20can%20you%20make%20a%20plan%20to%20fully%20imple.md) — Full implementation planning
- [skilltree redesign.md](skilltree%20redesign.md) — UI redesign template
- [update documentation and skills.md](update%20documentation%20and%20skills.md) — Doc/skill update template

### Session Management Prompts
- [session_organization.md](session_organization.md) — Organize previous day's work into journal, tracker, and dashboard

### Asset / Art Generation Prompts
- [using icons.md](using%20icons.md) — Icon integration
- [tools/item_icon_prompts.md](../tools/item_icon_prompts.md) — AI image generation prompts for item icons
- [world_map_generation_prompt.md](world_map_generation_prompt.md) — Midjourney/SDXL/DALL-E prompts for RO-style hand-painted world map illustration (4096x4096)
- [loading_screen_generation_prompts.md](loading_screen_generation_prompts.md) — 12 anime-style loading screen scene prompts (1920x1080, Midjourney + SDXL templates)

### Sprite Pipeline Prompts
- [weapon_sprite_overlay_next_session.md](weapon_sprite_overlay_next_session.md) — **RESOLVED** (2026-03-25). Weapon overlay system is working. Kept as template for multi-session continuation prompts. See `docsNew/05_Development/Weapon_Sprite_Overlay_Pipeline.md` for final pipeline doc.
- [3d art deep research.md](3d%20art%20deep%20research.md) — 3D art pipeline research

### Server / Infrastructure Prompts
- [implement_navmesh_pathfinding.md](implement_navmesh_pathfinding.md) — Server-side NavMesh pathfinding for enemy monsters. Full implementation prompt with all code locations, coordinate system, and step-by-step instructions. Plan doc: `docsNew/05_Development/NavMesh_Pathfinding_Implementation_Plan.md`
- [implement_kafra_storage.md](implement_kafra_storage.md) — Complete Kafra Storage implementation (account-shared 300-slot storage via Kafra NPCs, 7 phases, server+client+DB)
- [implement_player_trading.md](implement_player_trading.md) — Complete P2P trading system (10 slots/side, Zeny exchange, two-step confirmation, anti-scam, 12 phases)

### Combat Feel / UI Polish Prompts
- [implement_ro_damage_numbers.md](implement_ro_damage_numbers.md) — Initial RO Classic damage number rewrite (sine arc + drift + scale shrink, per-type curves, color spec, crit starburst). Template for "rewrite an animation system to match a reference spec."
- [implement_damage_number_remaining_features.md](implement_damage_number_remaining_features.md) — 6-phase implementation plan (hit flash, particles, starburst, combo total, flinch, sounds) with file matrix and dependency order. Template for any multi-phase feature broken across files.
- [implement_hit_impact_features.md](implement_hit_impact_features.md) — Tighter step-by-step companion to the 6-phase plan, with "Pre-Implementation: Read These Files First" block. Template for "implement steps from a plan doc" follow-up prompts.

### Skill Implementation Pattern
The pattern that worked for every class:
1. `/deep-research` — gather rAthena + iRO Wiki + RateMyServer data
2. Create `CLASS_Class_Research.md` with findings
3. `/sabrimmo-skill-CLASS` — load the skill context
4. Implement handlers in `index.js`
5. Create `CLASS_Skills_Audit_And_Fix_Plan.md` with bug list
6. Fix all bugs, verify against rAthena source

See [Second_Class_Implementation_Prompts.md](../docsNew/05_Development/Second_Class_Implementation_Prompts.md) for the actual templates used.
