# Sabri_MMO Dashboard

## Quick Links
- [Session Tracker](Session%20Tracker.md) — All 58 Claude Code sessions with resume IDs
- [Workflow Guide](Workflow%20Guide.md) — How to use Obsidian + Claude Code together
- [Prompt Library](../_prompts/README.md) — Reusable prompts that work
- [Documentation Index](DocsNewINDEX.md) — Master doc navigation
- [Project Overview](../docsNew/00_Project_Overview.md) — Full system inventory
- [Global Rules](../docsNew/00_Global_Rules/Global_Rules.md) — Design standards
- [CLAUDE.md](../CLAUDE.md) — Claude Code project instructions

## Current Stats (2026-04-04)
| Metric | Count |
|--------|-------|
| Server lines | ~34,500 + 12 modules (~6,300) |
| Socket events | 103 |
| REST endpoints | 11 |
| C++ Subsystems | 36 |
| Skill definitions | 293 (69 + 224) |
| Buff types | 95 |
| Status effects | 10 |
| Items in DB | 6,169 |
| Cards | 538 |
| Monster templates | 509 |
| Active spawns | 46 |
| Zones | 4 |
| VFX configs | 97 |
| Classes | 19 (6 first + 13 second) |
| Textures | 1,061 (AI + RO originals) |
| Material variants | 2,700+ |
| Decal instances | 91 |
| Scatter meshes | 135 (75 V1 + 60 V3) |
| Environment scripts | 80+ |

## What's Implemented
- [x] All 6 first classes + 13 second classes
- [x] Full combat pipeline (physical + magical)
- [x] 95 buff types + 10 status effects
- [x] Party system (21/21 RO features)
- [x] Card system (538/538, all deferred systems)
- [x] Dual wield (Assassin)
- [x] Combo system (Monk)
- [x] Performance system (Bard/Dancer, 9 ensembles)
- [x] Trap system (Hunter)
- [x] Pet + Homunculus + Companion visuals
- [x] Cart + Vending + Item Appraisal
- [x] Forging + Refining
- [x] Monster skill system (40+ NPC_ skills)
- [x] Persistent socket (survives zone transitions)
- [x] 33 C++ Slate subsystems (all BPs replaced)
- [x] Abracadabra (145 skills + 6 special effects)
- [x] Whisper, MVP, death penalty
- [x] 3D-to-2D sprite pipeline (Tripo3D+Mixamo+Blender, validated Mage+Swordsman)
- [x] SpriteCharacterActor UE5 integration (dual atlas, weapon modes, combat hooks)
- [x] Code audit pass (43 audit files across security, performance, integration, monsters)
- [x] Weapon sprite overlay system (dual-pass render, per-frame depth ordering, remote sync)
- [x] Animation standardization (17-anim standard for ALL classes, targetType-driven cast)
- [x] Archer_f + bow weapon overlay sprites (2272 total)
- [x] Skeleton enemy sprite (humanoid, Mixamo-rigged, pure C++ actor)
- [x] Poring enemy sprite (blob, shape key anims, render_monster.py)
- [x] NavMesh pathfinding implemented (server-side recast-navigation, enemy AI movement)
- [x] Hair sprite layer system (separate layer, 9-color tinting, hides_hair headgear flag, female atlas done)
- [x] Headgear sprite layer (holdout occlusion, always_front depth, multi-slot blocking, Egg Shell Hat)
- [x] Render pipeline standardized (render_blend_all_visible.py + .blend + --cel-shadow 0.92/0.98)
- [x] Shared armature system (base_m/base_f for all classes, ~5 min per new class)
- [x] Ground texture system (1061 textures, 17 material versions, 2700+ variants, ComfyUI pipeline)
- [x] DBuffer decal system (5 parent materials, 91 RO-texture instances)
- [x] Landscape Grass V3 (60 AI sprites, 13 zone GrassTypes, paintable + random placement)
- [x] 3D world building guides (5 _meta docs: landscape, materials, scatter, decals, lighting)
- [x] Map system — minimap (134x134, 5 zoom, draggable, SceneCapture), world map (12x8 grid, 62 zones), loading screen (Ken Burns, sparkles), Guide NPC marks, /where command, preferences persistence
- [x] Kafra Storage — account-shared 300-slot storage, 10 socket handlers, split/sort/auto-stack/search QoL, 40z fee
- [x] Player-to-Player Trading — 10 items + zeny, two-step confirm (OK/Trade), 13 cancel paths, atomic transfer, /trade command, trade logs
- [x] Right-click player context menu — Trade/Party Invite/Whisper, camera drag vs quick-click detection, FMenuBuilder popup

## What's Next

### In-Game Testing Pass
**Class skills to test:**
- [x] Sage (IDs 1400-1421)
- [x] Hunter (IDs 900-917)
- [x] Bard (IDs 1500-1537)
- [x] Dancer (IDs 1520-1557)
- [ ] Priest (IDs 1000-1018)
- [ ] Monk (IDs 1600-1615)
- [ ] Assassin (IDs 1100-1111)
- [ ] Rogue (IDs 1700-1718)
- [ ] Blacksmith (IDs 1200-1230)
- [ ] Alchemist (IDs 1800-1815)

**Systems to test:**
- [x] Account creation (register, login, character create/delete, JWT auth)
- [x] Party (EXP share, chat, HP sync, invites)
- [ ] Dual wield (Assassin left-hand, per-hand cards, mastery penalties)
- [ ] MVP (announcements, rewards, slave spawning)
- [ ] Pets (taming, feeding, hunger/intimacy, bonuses)
- [ ] Homunculus (combat, skills, evolution, feeding)
- [ ] Refining (success rates, overupgrade bonus, safe limits)
- [ ] ASPD potions (delay reduction, stacking, class restrictions)

### VFX Bugs & Remaining Work
See [Skill_VFX_Execution_Plan](../docsNew/05_Development/Skill_VFX_Execution_Plan.md) for full plan (Phases 6/7/11 still pending).

**Bugs to fix:**
- [ ] Frost Diver VFX never wears off (persists after debuff expires)
- [ ] Frozen/Stone Cursed enemies keep walking to previous target instead of stopping immediately
- [ ] Frost Bolt and Fire Bolt missing impact VFX on hit
- [ ] Fireball not visible to other clients (should show projectile from caster to enemy on all screens)
- [ ] Soul Strike projectiles don't travel all the way to the enemy
- [ ] Stone Curse reuses Frost Diver's Cascade effect (need different VFX, can't control color)

**Previously fixed (done):**
- [x] Provoke uses P_Enrage_Base, 1s flash above enemy head
- [x] Cold Bolt/Fire Bolt: projectile destroyed on arrival, 1 projectile per spell level, spawns even if enemy dies
- [x] Lightning Bolt persists longer (was disappearing too fast)
- [x] Frost Diver VFX lasts full debuff duration
- [x] Fire Ball: single projectile → target → single explosion
- [x] Soul Strike: 1 projectile per level, starts at player, stops at enemy (bolt+fireball hybrid)
- [x] Thunderstorm loops for full spell cycle, scales with skill level
- [x] Stone Curse VFX shows (behaves like Frost Diver)
- [x] Fire Wall displays at ground-click location (like Safety Wall targeting)
- [x] Skills don't trigger auto-attacks (separate trigger system)
- [x] Fire Wall damage/knockback matches RO Classic (not instant kill)
- [x] Safety Wall and Fire Wall properly expire after duration
- [x] Magnum Break ground-targeted VFX even with no enemies nearby
- [x] Endure VFX visible to other players
- [x] Casting circles shown to other players
- [x] AoE ground target indicator matches RO Classic
- [x] Magnum Break target circle sized correctly
- [x] First Aid shows green healing numbers
- [x] Buff system persists across zone transitions

### Sprite Pipeline — Next Steps
- [x] Validate all Swordsman atlas animations working in-game
- [x] Standardize animation set per class — **17-anim standard for ALL classes** (replaces old fighter/mage split)
- [x] **Pack mage_f atlases** (2336 sprites → per-animation PNGs, validated in UE5)
- [x] **Weapon sprite overlay system** (dual-pass render, depth ordering, dagger on swordsman validated)
- [x] **Clean up mage animations** → unified to 17-anim standard
- [x] **Clean up variants logic** in C++ (simplified for 17-anim standard)
- [x] **Archer_f + bow weapon overlay** (2272 sprites, bow .blend template, Bow weapon mode in C++/server)
- [x] **Monster sprite — humanoid** (Skeleton: Mixamo-rigged, pure C++ sprite actor)
- [x] **Monster sprite — non-humanoid** (Poring: shape key anims via render_monster.py, --model-rotation -90)
- [x] **Enemy sprite integration** (EnemySubsystem: sprite enemies as pure C++ actors, no BP_EnemyCharacter)
- [x] **Re-render swordsman_m + mage_f** with new 17-animation standard set
- [ ] **More weapon .blend templates** (sword, staff, rod, katar, spear, mace, axe — dagger + bow done)
  - [x] Figure out weapon positioning across different classes/genders
- [x] **NavMesh pathfinding** — implemented server-side (recast-navigation, de-aggro, all movement patched)
- [ ] Equipment layer system — headgear + hair DONE, shield + garment remaining
- [ ] Scale production: batch remaining classes through Tripo3D+Mixamo+render
- [x] Hair color tint system (9 RO colors, material TintColor multiply)
- [x] Headgear sprite layer (holdout occlusion, Egg Shell Hat)
- [ ] Male hair style 1 atlas (female done, male not yet)
- [ ] More enemy sprites (additional monsters)

### 3D World — Next Steps
- [ ] Apply materials and decals to Prontera zone (first complete zone)
- [ ] Posterized lighting (top improvement from UE5 material research)
- [ ] Material Parameter Collection for global tint/time-of-day
- [ ] Test grass density at scale (performance check)
- [ ] Build additional zone landscapes (Payon, Geffen, Morroc, etc.)

### Audit Follow-ups
- [ ] Review audit results in `_audit/` and triage fixes (43 files, 2 master reports)

### Map System — Next Steps
- [ ] Generate world map illustration (prompt ready: `_prompts/world_map_generation_prompt.md`)
- [ ] Generate loading screen art (12 prompts ready: `_prompts/loading_screen_generation_prompts.md`)
- [ ] Commit map system code (6 new + 2 modified files, untracked)

### Economy Systems
- [x] Kafra Storage — implemented 2026-04-03 (prompt: `_prompts/implement_kafra_storage.md`)
- [x] Player-to-Player Trading — implemented 2026-04-03 (prompt: `_prompts/implement_player_trading.md`)
- [ ] Right-click context menu on other players (trade, party invite, whisper — requires PlayerInputSubsystem right-click routing)

### UX / Polish
- [ ] ESC → character select screen
- [ ] Fix character floating off ground / terrain clipping when moving north
- [ ] Attack impact effects / damage number improvements (make combat more exciting)
- [ ] Right-click context menu on other players (trade, party invite, whisper)

### Remaining Features
- [ ] Sound effects / music system (needs research + implementation plan)
- [ ] Client-side homunculus actor + position broadcast
- [ ] PvP system
- [ ] War of Emperium / Guild system
- [ ] Quest system
- [ ] Additional zones (Payon, Geffen, Alberta, etc.)
- [ ] Niagara VFX build (Phases 6/7/11 pending — 97 configs defined, assets mapped)
- [ ] Guild storage + trading
- [ ] More NPC shops per zone

## Recent Sessions
<!-- Add links to session logs here, newest first -->
- **2026-04-03** — Kafra Storage (account-shared 300-slot, 10 handlers, split/sort/search QoL) + Player-to-Player Trading (two-step confirm, 13 cancel paths, atomic transfer, /trade command). 8 bugs found and fixed. ([daily note](2026-04-03.md))
- **2026-04-02** — Complete map system: minimap (134x134, 5 zoom, draggable), world map (12x8 grid, 62 zones), loading screen (Ken Burns, sparkles), Guide NPC marks, /where command. Planned Kafra Storage + Player Trading (prompts created). ([daily note](2026-04-02.md))
- **2026-03-30 to 2026-04-01** — Ground texture & material system: 1000+ AI textures, 608 RO originals, 17 material versions, 2700+ variants, DBuffer decals (91), Landscape Grass V3 (60 sprites, 13 zones), 80+ scripts, 3 new skills, 5 _meta guides ([daily note](2026-03-30.md))
- **2026-03-29** — Hair sprite layer committed, render pipeline overhaul (--cel-shadow 0.92), parallax fix (0.3→0.01), 5 hair visibility bugs fixed, 5 female classes re-rendered ([daily note](2026-03-29.md))
- **2026-03-27** — NavMesh pathfinding implemented (recast-navigation), headgear sprite layer (holdout occlusion, Egg Shell Hat), hair sprite layer started, 5 female class + weapon re-renders, shared armature system ([daily note](2026-03-27.md))
- **2026-03-26** — Animation standardization (17-anim standard for ALL classes), archer_f + bow weapon sprites, skeleton + poring enemy sprites (pure C++ actors), NavMesh pathfinding planned ([daily note](2026-03-26.md))
- **2026-03-25** — Weapon sprite overlay system SOLVED (dual-pass render + 5 C++ fixes + depth ordering + remote sync), swordsman anims simplified 19→14, mage atlases packed & validated ([daily note](2026-03-25.md))
- **2026-03-24** — Swordsman v2 atlases validated in-game, Mage v2 config built (34 anims, 2336 sprites rendered), standardized fighter vs mage animation sets ([daily note](2026-03-24.md))
- **2026-03-23** — 3D-to-2D sprite pipeline (Mage 168 sprites + Swordsman 768 sprites), UE5 SpriteCharacterActor, ComfyUI abandoned, deep+broad code audits (43 files) ([daily note](2026-03-23.md))
- **2026-03-22** — Bard/Dancer/Ensemble comprehensive fix, party system polish ([daily note](2026-03-22.md))
- **2026-03-21** — In-game testing: Sage (22 skills) + Hunter (18 skills) passed, account creation tested, Obsidian vault setup ([daily note](2026-03-21.md))
- **2026-03-20** — Skill icon application (all class skills), generic project progress, documentation overhaul ([daily note](2026-03-20.md))

## Useful Claude Code Commands
```bash
# Resume a previous session
claude --resume <session-id>

# Start with specific context
claude "load /sabrimmo-skills /sabrimmo-combat then fix X"

# Quick server test
cd server && npm run dev
```
