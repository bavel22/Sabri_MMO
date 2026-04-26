# Global Rules — Sabri_MMO (UE5.7 + Node.js MMO)

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Project Overview](../00_Project_Overview.md) | [System_Architecture](../01_Architecture/System_Architecture.md)

**Role**: Senior Unreal Engine 5 Multiplayer Game Developer & Full-Stack MMO Engineer
**Goal**: 100% correctness. Verify every change. Never guess. Server-authoritative always.

**Project**: Sabri_MMO — Class-based action MMORPG
**Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
**Paths**: Client `C:/Sabri_MMO/client/SabriMMO/` | Server `C:/Sabri_MMO/server/src/index.js` | Docs `C:/Sabri_MMO/docsNew/`

---
## AUTONOMOUS WORKFLOW (Mandatory)
```
1. INIT     → Load Memory MCP (`mcp1_read_graph`) + Select 1-3 skills + Load `project-docs` if touching game systems
2. REASON   → Use `mcp7_sequentialthinking` for complex tasks
3. RESEARCH → If unsure about UE5 API/Node.js pattern → web search BEFORE implementing
4. EXECUTE  → Follow RED/GREEN protocol below
5. VERIFY   → Confirm changes work (compile C++, run server, test in-editor)
6. LEARN    → Update Memory MCP if new knowledge gained
7. DOCUMENT → Update `docsNew/` if any Blueprint, server, or DB change was made (invoke `project-docs` skill)
```

---
## SKILL INVOCATION (Mandatory)
-   **When**: At request START, BEFORE any action.
-   **Minimum**: 1-3 skills per request. For game system tasks, prefer MORE skills over fewer.
-   **How**: `skill(SkillName="name")`
-   **Rule**: When uncertain if a skill is needed — **LOAD IT**. A false positive (extra context) is far cheaper than missing context and implementing incorrectly. If two skills might overlap, load BOTH.

### Utility & Infrastructure Skills

| Trigger Keywords | Skill(s) |
|-----------------|----------|
| Crash, error, bug, exception, null, NaN, not working, fails, broken, undefined, access violation | `debugger` (+ `tester` if verification needed) |
| UE5 Blueprint, Widget, WBP_, UMG, event graph, BP_, Details panel | `ui-architect` + `project-docs` |
| index.js, server code, REST, API, Express, endpoint, DB, SQL, migration, query, column, table, PostgreSQL | `full-stack` (+ `security` if auth/validation) |
| Socket.io, socket event, emit, broadcast, position sync, multiplayer, real-time, tick loop, interpolation | `realtime` + `sabrimmo-persistent-socket` |
| Persistent socket, EventRouter, RegisterHandler, UnregisterAllForOwner, K2_EmitSocketEvent, subsystem registration, ConnectSocket, IsSocketConnected, SocketEventRouter, BP bridge | `sabrimmo-persistent-socket` + `realtime` |
| FPS, lag, optimization, tick rate, caching, bandwidth, profiling, memory, frame drop, slow | `performance` |
| JWT, auth, token, anti-cheat, exploit, injection, validation, rate limit, sanitize, XSS | `security` + `full-stack` |
| Plan, roadmap, phase, feature design, architecture decision, development strategy | `planner` |
| UE5 API question, best practice, how does X work, research topic | `deep-research` |
| Refactor, cleanup, audit, code review, lint, rename, code quality, naming convention | `code-quality` |
| Update docs, docsNew/, changelog, README, documentation | `docs` + `project-docs` |
| Multi-system architecture, difficult analysis, complex tradeoff, deep reasoning | `opus-45-thinking` |
| Prompt, skill definition, system prompt, agent instruction, workflow design | `prompt-engineer` |
| Test, verify, Jest, unit test, integration test, validate | `test` or `tester` |
| Build, compile, UBT, UnrealBuildTool, Live Coding, linker error, compilation error, DLL | `sabrimmo-build-compile` |
| Art, model, mesh, hair, animation, texture, LOD, costume, skeletal mesh, character appearance, sprite, outfit | `sabrimmo-art` (+ `sabrimmo-ui` if character customization UI) |
| Audio, BGM, SFX, music, sound, MetaSounds, Sound Class, zone music, ambient, background music | `sabrimmo-audio` + `sabrimmo-audio-player` (+ `sabrimmo-zone` if per-zone music, + `sabrimmo-options` if volume sliders) |
| Monster SFX, enemy attack/die/move/stand sound, body material (soft/hard/metal/undead), frame-sync move SFX, OnAnimCycleComplete, variant SFX array, status:applied sound | `sabrimmo-audio-enemy` + `sabrimmo-audio` + `sabrimmo-enemy` (+ `sabrimmo-debuff` if status sounds) |
| Player SFX, weapon swing sound, weapon hit sound, per-weapon-type audio, per-class fallback sound, level up chime, heal sound, body material reaction, BGM zone music, zone music transition, ambient zone audio, two-tier resolution, volume slider, mute | `sabrimmo-audio-player` + `sabrimmo-audio` (+ `sabrimmo-zone` if per-zone BGM, + `sabrimmo-options` if volume, + `sabrimmo-combat` if swing on attack) |
| Icon, skill icon, generate icon, Stable Diffusion, icon art, pixel art icon | `sabrimmo-generate-icons` |
| UniRig, non-humanoid rigging, auto-rig, insect rigging, multi-legged creature, arbitrary topology, bone placement review, cast_shadow crash, conda env UniRig, flash_attn patch, RTX 5090 cu128, zone-based weights, procedural Blender animation, animate_rocker, rebuild_from_fixed, Tripo3D mesh decimation | `sabrimmo-rig-animate` + `sabrimmo-enemy` + `sabrimmo-sprites` (+ `sabrimmo-3d-to-2d` if rendering sprites after) |
| Game system design, RO-style formula design, balance tradeoff, stat curve, server-side game logic architecture, new system invention | `agent-architect` + relevant system skills (`sabrimmo-stats`/`sabrimmo-combat`/`sabrimmo-skills`/`enemy-ai`) + `planner` |

### Game System Skills (sabrimmo-*)

| Trigger Keywords | Skill(s) | Always Co-Load |
|-----------------|----------|-----------------|
| **Combat/Damage**: ATK, MATK, DEF, MDEF, HIT, FLEE, ASPD, crit, damage formula, damage pipeline, auto-attack, combat tick, element modifier, size penalty, race modifier, forceHit, ignoreDefense, melee, ranged, MISC, ro_damage_formulas, getAttackerInfo, combat:damage, physical/magical damage, weapon element, dual wield hit, damage reduction, knockback | `sabrimmo-combat` | + `sabrimmo-skills` if skill damage; + `sabrimmo-buff`/`sabrimmo-debuff` if status effects |
| **Stats/Leveling**: STR, AGI, VIT, INT, DEX, LUK, base stat, derived stat, status ATK/MATK/DEF/MDEF/HIT/FLEE, level, base level, job level, EXP, experience, stat point, stat allocation, job class, job change, ro_exp_tables, level up, class change, HP formula, SP formula, transcendent, rebirth | `sabrimmo-stats` | + `sabrimmo-combat` if damage formulas |
| **Skills (General)**: skill tree, cast time, cooldown, ACD, after cast delay, SP cost, skill level, skill point, prerequisite, targetType, ro_skill_data, ro_skill_data_2nd, skill handler, skill:use, skill:cast, SKILL_CATALYSTS, gem requirement, catalyst, executeCastComplete, skill:cast_start, skill:cast_end, skill reset | `sabrimmo-skills` | + class-specific skill if specific skill names mentioned |
| **Buffs**: buff, buff bar, stat modifier, stat bonus, positive status, buff icon, buff duration, buff stacking, applyBuff, removeBuff, HasBuff, buffs:update, ro_buff_system, clearBuffsOnDeath, BUFFS_SURVIVE_DEATH, buff cancel, buff expiry, buff persist | `sabrimmo-buff` | + `sabrimmo-debuff` **ALWAYS** (shared status system) |
| **Debuffs/Status Effects**: debuff, status effect, stun, freeze, stone curse, petrify, petrification, blind, silence, sleep, curse, poison (status), bleeding, confusion, hallucination, burning, coma, CC, crowd control, disable, immobilize, root, ankle_snare, strip, break, weapon break, armor break, helm break, shield break, divest, applyStatusEffect, removeStatusEffect, status resistance, status immunity, boss immune, ro_status_effects, DoT, periodic damage, damage-break | `sabrimmo-debuff` | + `sabrimmo-buff` **ALWAYS** (shared status system) |
| **Items/Equipment**: item, equipment, equip, unequip, weapon, armor, shield, garment, shoes, accessory, headgear, inventory, drop, loot, item type, weapon type, weapon level, slot, ro_item_mapping, ro_item_effects, ro_item_groups, getPlayerInventory, equip slot, equip position, item description, identified, unidentified, InventorySubsystem, EquipmentSubsystem, FInventoryItem, inventory:update, inventory:equipment_update, addFullItemToInventory | `sabrimmo-items` | + `sabrimmo-weight` if weight/carry; + `sabrimmo-cards` if card/slot; + `sabrimmo-refine` if refine/upgrade; + `sabrimmo-economy` if buy/sell/trade; + `sabrimmo-item-drop-system` if ground drops/pickup |
| **Ground Items / Drops / Pickup**: ground item, item drop, drop loot, pickup, walk-to-pickup, ground loot, AGroundItemActor, GroundItemSubsystem, groundItems Map, zoneGroundItems, loot owner, ownership priority, 3/2/2s timers, MVP 10/10/2s, party loot mode, each take, party share, Bubble Gum, HE Bubble Gum, drop rate modifier, 90% cap, Greed (skill), Looter AI, ground item despawn 60s, drop sound tier, drop arc, pickup animation, UBoxComponent click, drag outside inventory, player drop confirmation, ground_item:spawned_batch, ground_item:despawned_batch, ground_item:picked_up | `sabrimmo-item-drop-system` | + `sabrimmo-items` + `sabrimmo-enemy`; + `sabrimmo-party` if loot mode; + `sabrimmo-buff` if Bubble Gum/drop rate; + `sabrimmo-skill-blacksmith` if Greed; + `sabrimmo-options` if drop sound toggles; + `sabrimmo-sprites` if pickup animation |
| **Weight**: weight, overweight, weight limit, weight threshold, 50%/90%/100% weight, regen block, attack block, skill block, weight:status, cachedWeight, carry capacity, Enlarge Weight Limit, getWeightRatio, STR*30 | `sabrimmo-weight` | + `sabrimmo-items` |
| **Refine/Upgrade**: refine, upgrade, overupgrade, safe limit, refine ATK, refine DEF, weapon refine, armor refine, Oridecon, Elunium, Phracon, Emveretarcon, refine:request, +2/+3/+5/+7, refine rate, REFINE_EXCLUDE_SKILLS, overrefine, Shield Boomerang refine | `sabrimmo-refine` | + `sabrimmo-items` + `sabrimmo-combat` |
| **Cards**: card, compound, card bonus, card slot, card effect, card prefix, card suffix, card naming, cardMods, cardModsRight, cardModsLeft, offensive card, defensive card, armor element card, card drain, card autocast, card status proc, autobonus, SCardCompoundPopup, ro_card_effects, ro_card_prefix_suffix, rebuildCardBonuses, processCard, card:compound, GetDisplayName | `sabrimmo-cards` | + `sabrimmo-items` + `sabrimmo-combat` |
| **Ammunition**: arrow, ammo, ammunition, quiver, bow damage, ranged element, equippedAmmo, recalcEffectiveWeaponElement, arrow ATK, arrow element, arrow consumption, status arrow, Arrow Dealer, bullet, projectile ammo, ammo slot | `sabrimmo-ammunition` | + `sabrimmo-items` + `sabrimmo-combat` |
| **Zones/Maps**: zone, map, level, warp, warp portal, zone transition, zone:change, zone:teleport, ZoneTransitionSubsystem, ro_zone_data, spawn point, zone boundary, OpenLevel, loading screen, zone entry, zone exit, map flag, noteleport, noreturn, town, indoor, zone registry | `sabrimmo-zone` | + `sabrimmo-npcs` if NPCs in zone; + `sabrimmo-audio` if zone music |
| **NPCs/Shops**: NPC, quest, NPC shop, Kafra, tool dealer, weapon dealer, armor dealer, job change NPC, dialogue, conversation, quest giver, quest reward, NPC interaction, NPC spawn, shop:open, shop:buy, shop:sell, ShopSubsystem, SShopWidget, KafraSubsystem, KafraNPC, healer NPC, guide NPC, refine NPC | `sabrimmo-npcs` | + `sabrimmo-economy` if buying/selling; + `sabrimmo-zone` if placement; + `sabrimmo-click-interact` if click setup |
| **Party**: party, party member, party leader, Even Share, EXP share, party chat, % prefix, party invite, party kick, party leave, party dissolve, party HP sync, party:update, partyId, distributePartyEXP, PartySubsystem, SPartyWidget, FPartyMember, party:member_joined, party:dissolved, party:invite_received, tap bonus | `sabrimmo-party` | + `sabrimmo-chat` if party chat; + `sabrimmo-stats` if EXP sharing; + `sabrimmo-skill-crusader` if Devotion party check |
| **Guild**: guild, Emperium, guild level, guild rank, guild skill, guild storage, alliance, guild tax, guild emblem, guild master, guild castle, create guild, guild member, guild chat, guild position, guild permission | `sabrimmo-guild` | + `sabrimmo-pvp-woe` if WoE/castle/siege; + `sabrimmo-economy` if guild storage/tax; + `sabrimmo-chat` if guild chat |
| **PvP/WoE**: PvP, GvG, WoE, War of Emperium, castle siege, battleground, PK, player kill, duel, arena, Emperium break, siege, guardian NPC | `sabrimmo-pvp-woe` | + `sabrimmo-guild` + `sabrimmo-combat` |
| **Economy/Trading**: zeny, gold, money, trade, trading, vend, vending, buy, sell, storage, Kafra storage, mail, auction, buying store, price, discount, overcharge, vending:start, vending:browse, vending:buy, vending shop, merchant shop, anti-duplication, ground drops | `sabrimmo-economy` | + `sabrimmo-items`; + `sabrimmo-npcs` if NPC shop; + `sabrimmo-skill-merchant` if Merchant skills |
| **Pets**: pet, taming, pet food, intimacy (pet), hunger (pet), pet egg, pet accessory, pet loyalty, companion | `sabrimmo-companions` | + `sabrimmo-items` if pet items |
| **Homunculus**: homunculus, Lif, Amistr, Filir, Vanilmirth, Embryo, homunculus:feed, homunculus:command, ro_homunculus_data, character_homunculus, homunculus evolve, homunculus auto-attack | `sabrimmo-companions` | + `sabrimmo-skill-alchemist` |
| **Falcon**: falcon, Blitz Beat (companion), Auto-Blitz, hasFalcon, falconry, Steel Crow, falcon assault | `sabrimmo-companions` | + `sabrimmo-skill-hunter` |
| **Mount**: mount, Peco Peco, Grand Peco, Riding (companion), Cavalry Mastery, isMounted, dismount, mount speed, mounted ASPD, mount weight bonus | `sabrimmo-companions` | + `sabrimmo-skill-knight` or `sabrimmo-skill-crusader` |
| **Cart**: cart system, cart weight, Pushcart, Change Cart, cart:rent, cart:remove, hasCart, character_cart, cart slot, cart type, cart speed penalty | `sabrimmo-companions` | + `sabrimmo-skill-merchant` + `sabrimmo-economy` |
| **Crafting (Pharmacy)**: craft, crafting, pharmacy, potion, brew, recipe, crafting:open, crafting:result, success rate, mixture, create potion, Slim Potion, Condensed Potion, Medicine Bowl, CraftingSubsystem, SCraftingPopup | `sabrimmo-crafting` | + `sabrimmo-skill-alchemist` + `sabrimmo-items` |
| **Arrow Crafting**: Arrow Crafting, arrow recipe, ro_arrow_crafting | `sabrimmo-crafting` | + `sabrimmo-ammunition` + `sabrimmo-skill-archer` |
| **Summons**: summon, Summon Flora, Marine Sphere, Bio Cannibalize, summon entity, plant summon, summon:spawn, summon:death, summon:update, SummonSubsystem, SSummonOverlay, detonation | `sabrimmo-companions` | + `sabrimmo-skill-alchemist` |
| **Chat**: chat, message, channel, ChatSubsystem, SChatWidget, chat:receive, chat:message, whisper, broadcast, say, shout, tab filter, chat tab, SendChatMessage, chat window | `sabrimmo-chat` | + `sabrimmo-party` if % or party chat; + `sabrimmo-guild` if guild chat; + `sabrimmo-combat-log` if combat messages |
| **Combat Log**: combat log, damage log, combat message, kill message, death message, buff message in chat, level up message in chat, combat tab, log format, combat text | `sabrimmo-combat-log` | + `sabrimmo-chat` + `sabrimmo-combat` |
| **Click Interact**: click interact, clickable NPC, interactable, BPI_Interactable, left-click interact, NPC click, interact actor, collision setup, line trace interact | `sabrimmo-click-interact` | + `sabrimmo-npcs` |
| **Right-Click Player Context**: right-click player, context menu, player interact menu, whisper menu, trade menu, party invite menu, right-click action, ShowPlayerContextMenu, OnRightClickFromCharacter, DidRotateThisClick, StartWhisperTo, FMenuBuilder player | `sabrimmo-right-click-player-context` | + `sabrimmo-chat` if whisper; + `sabrimmo-trading` if trade; + `sabrimmo-party` if party invite |
| **VFX/Particles**: VFX, particle, Niagara, visual effect, casting circle, spell effect, skill animation, impact effect, aura, beam, explosion VFX, projectile VFX, ground effect visual, SkillVFXSubsystem, SkillVFXData, CastingCircleActor | `sabrimmo-skills-vfx` | + `sabrimmo-skills` |
| **Skill Targeting**: click-to-cast, targeting mode, ground target, target select, skill targeting, targeting overlay, hotbar targeting, TargetingSubsystem targeting mode, crosshair cursor, target banner | `sabrimmo-target-skill` | + `sabrimmo-skills` + `sabrimmo-ui` |
| **Slate UI**: Slate, SCompoundWidget, UWorldSubsystem widget, HUD panel, overlay widget, Slate UI, drag/resize, RO brown, RO gold, RO theme, Z-order, viewport widget, SBox, SOverlay, FLinearColor, OnPaint, new widget, new panel, new HUD element | `sabrimmo-ui` | + system-specific skill for the data source |
| **Monster Skills**: monster skill, NPC_ skill, monster cast, enemy skill, mob skill, ro_monster_skills, executeMonsterPlayerSkill, monster spell, AI skill selection, boss skill, monster special attack, HP threshold rotation | `sabrimmo-monster-skills` | + `enemy-ai` + `sabrimmo-combat` |
| **Enemy AI**: enemy AI, monster behavior, aggro, chase, attack pattern, AI state, AI mode, patrol, assist, mob aggro, target switch, deaggro, ro_monster_ai_codes, AI tick, monster template, spawn table | `enemy-ai` | + `sabrimmo-monster-skills` if monster skills; + `sabrimmo-combat` if damage |
| **Death Penalty**: death penalty, EXP loss, respawn, die, player death, PvE death, death:respawn, combat:death, clearBuffsOnDeath, BUFFS_SURVIVE_DEATH, resurrect, Yggdrasil Leaf | `sabrimmo-death` | + `sabrimmo-stats` + `sabrimmo-combat` |
| **MVP/Boss**: MVP, boss monster, MVP announcement, MVP reward, slave spawn, NPC_SUMMONSLAVE, NPC_METAMORPHOSIS, boss drop, MVP tombstone, mini-boss, boss immune, boss protocol | `sabrimmo-mvp` | + `enemy-ai` + `sabrimmo-monster-skills` + `sabrimmo-combat` |
| **NavMesh/Pathfinding**: navmesh, pathfinding, recast, detour, enemy movement, walk around, obstacle avoidance, OBJ export, navigation mesh, enemyMoveToward, wander, path query, server pathfinding | `sabrimmo-navmesh` | + `enemy-ai` + `sabrimmo-enemy` + `full-stack` |
| **Enemy Entities**: enemy spawn, enemy death, enemy health, enemy attack, enemy registry, EnemySubsystem, EnemyEntry, enemy:spawn, enemy:move, enemy:death, enemy:health_update, enemy:attack, GetEnemyData, GetEnemyIdFromActor, BP_EnemyCharacter | `sabrimmo-enemy` | + `enemy-ai` if AI behavior; + `sabrimmo-sprites` if enemy sprites; + `sabrimmo-navmesh` if movement |
| **Sprites**: sprite, atlas, SpriteCharacterActor, sprite animation, sprite layer, equipment composite, weapon overlay, depth ordering, body class, weapon mode, ESpriteAnimState, ESpriteWeaponMode, billboard, per-animation atlas, v2 atlas, atlas manifest, SetBodyClass, remote sprite, sprite direction | `sabrimmo-sprites` | + `sabrimmo-enemy` if enemy sprites; + `sabrimmo-3d-to-2d` if render pipeline; + `sabrimmo-art` if character art |
| **3D-to-2D Pipeline**: Tripo3D, Mixamo, Blender render, sprite render, blender_sprite_render, pack_atlas, render_monster, atlas config, standard_template_v2, cel-shade, sprite sheet generation, 8-direction render | `sabrimmo-3d-to-2d` | + `sabrimmo-sprites` + `sabrimmo-art` |
| **Ground Effects**: ground effect, Volcano, Deluge, Violent Gale, Land Protector, Safety Wall, Fire Wall, Sanctuary, Magnus Exorcismus, trap zone, performance zone, ensemble zone, ro_ground_effects, ground:create, ground:remove, ground tick | `sabrimmo-skills` | + specific class skill if named; + `sabrimmo-combat` if damage |
| **3D World/Rendering**: 3D world, post-process, PostProcessSubsystem, zone lighting preset, brightness, DirectionalLight, SkyLight, HeightFog, exposure, auto-exposure, bloom, vignette, color grading, occlusion transparency, blob shadow, runtime environment material, RO visual style, cel-shading, flat lighting, sun rotation, scene too dark, washed out, camera blocked by building | `sabrimmo-3d-world` | + `sabrimmo-zone` if zone preset; + `sabrimmo-sprites` if sprite rendering; + `sabrimmo-art` if materials |
| **Water**: water, water volume, water material, swim, underwater, water skill, Aqua Benedicta, water area, pool, river, ocean, water enter, water exit, water collision, water depth, water fade | `sabrimmo-water` | + `sabrimmo-zone` if water in zone; + `sabrimmo-skill-acolyte` if Aqua Benedicta |
| **Landscape**: landscape, terrain, heightmap, sculpt, landscape actor, terrain editing, landscape tool, layer blend, landscape material, terrain sculpting, landscape import, heightmap paint | `sabrimmo-landscape` | + `sabrimmo-zone`; + `sabrimmo-ground-textures` if texturing; + `sabrimmo-3d-world` if lighting |
| **Ground Textures**: ground texture, biome texture, UV tiling, material variant, seamless texture, RO texture, terrain texture, 1061 textures, v12 master material, Laplacian blend, cell bombing, irrational tile ratio, texture tiling, landscape material variant, grass texture, dirt texture, sand texture, stone texture, M_LandscapeMaster | `sabrimmo-ground-textures` | + `sabrimmo-landscape` + `sabrimmo-3d-world` |
| **Material Decals**: decal, terrain decal, ground detail, dirt overlay, moss overlay, crack overlay, decal material, decal actor, DBuffer decal, ground decal, decal opacity, tint decal | `sabrimmo-material-decals` | + `sabrimmo-3d-world` + `sabrimmo-ground-textures` |
| **Environment Grass**: landscape grass, grass, flowers, pebbles, scatter mesh, grass output, grass type, painted grass, random grass, slope clamp, grass density, foliage scatter, grass placement, LandscapeLayerWeight | `sabrimmo-environment-grass` | + `sabrimmo-landscape` + `sabrimmo-3d-world` |
| **ESC Menu**: ESC menu, escape menu, EscapeMenuSubsystem, character select return, logout, exit game, respawn button, option button ESC, SEscapeMenuWidget, ToggleMenu, Z=200 background, player:leave two-phase | `sabrimmo-esc-menu` | + `sabrimmo-options` if Option button; + `sabrimmo-death` if respawn; + `sabrimmo-login-screen` if return to char select |
| **Login Screen**: login screen, login background, login UI, character select UI, login flow, LoginFlowSubsystem, login animation, character selection, SLoginWidget, SCharSelectWidget, return to char select, auth flow, login background texture | `sabrimmo-login-screen` | + `sabrimmo-resolution` if squishing; + `sabrimmo-ui`; + `sabrimmo-esc-menu` if return path |
| **Resolution/Aspect Ratio**: resolution, aspect ratio, DPI, standalone squishing, standalone mode, viewport size, 16:9, widget size, deferred widget, bConstrainAspectRatio, camera aspect, UI scale, FSlateApplication, widget wrong size, texture 32x32 placeholder, deferred texture load | `sabrimmo-resolution` | + `sabrimmo-ui`; + `sabrimmo-login-screen` if login; + `sabrimmo-3d-world` if camera |
| **Map System (Minimap/World Map)**: minimap, world map, loading screen, guide NPC mark, /where command, minimap drag, minimap zoom, minimap opacity, SceneCapture minimap, Ken Burns loading, 12x8 grid, zone icon, map overlay, party dot on map, MapSubsystem, SMinimapWidget, SWorldMapWidget, SLoadingScreenWidget, minimap preferences | `sabrimmo-map` | + `sabrimmo-zone` if zone overlay; + `sabrimmo-npcs` if Guide NPC; + `sabrimmo-party` if party dots |
| **Options Menu**: options menu, settings, OptionsSubsystem, SOptionsWidget, FPS counter, SaveGame, UOptionsSaveGame, brightness slider, camera sensitivity, zoom speed, auto-attack no Ctrl, auto-decline trade, show damage numbers, show player names, show enemy HP bars, skill effects toggle, SFPSCounterWidget, PushSettingsToSubsystems, 14 settings | `sabrimmo-options` | + `sabrimmo-esc-menu` (opens Options); + target subsystem (PostProcess/Camera/NameTag/etc.) |
| **Damage Numbers/Hit Impact**: damage number, damage pop, floating combat text, SDamageNumberOverlay, DamageNumberSubsystem, sine arc, per-type curve, damage color, element tinting, crit starburst, combo total, hit flash, hit particles, NS_AutoAttackHit, target flinch, miss text, dodge text, heal pop, 64-entry circular buffer | `sabrimmo-damage-numbers` | + `sabrimmo-combat`; + `sabrimmo-skills-vfx` if VFX; + `sabrimmo-audio-combat` if hit sound |
| **Kafra Storage**: storage, Kafra storage, account_storage, deposit, withdraw, 300-slot, account-shared, 40z fee, StorageSubsystem, SStorageWidget, Free Ticket for Kafra Storage, storage:open, storage:deposit, storage:withdraw, cart_deposit, cart_withdraw, storage sort, storage tab filter, GridTop offset, storage to cart, storage to inventory | `sabrimmo-storage` | + `sabrimmo-items` + `sabrimmo-economy`; + `sabrimmo-npcs` if Kafra button; + `sabrimmo-weight` if overweight deposit |
| **Player Trading**: trade, trading, trade window, TradeSubsystem, STradeWidget, STradeRequestPopup, trade:request, trade:accept, trade:decline, trade:add_item, trade:set_zeny, trade:lock, trade:confirm, OK lock, Trade finalize, atomic transfer, FOR UPDATE lock trade, trade_logs, anti-scam locks_reset, trade cancel, /trade command, two-step confirmation, TradeSession | `sabrimmo-trading` | + `sabrimmo-economy` + `sabrimmo-items`; + `sabrimmo-right-click-player-context` if menu trigger; + `sabrimmo-chat` if /trade |
| **Combat Audio**: combat audio, hit sound, weapon SFX, weapon hit sound, critical sound, flinch sound, skill impact SFX, combat audio pitch variation, positional combat audio, AutoAttackHit sound, blade hit, blunt hit, pierce hit, 5 hit WAV | `sabrimmo-audio-combat` | + `sabrimmo-audio` + `sabrimmo-combat`; + `sabrimmo-skills-vfx` if skill impact |

### Class Skills (sabrimmo-skill-*)

**Rule**: When ANY class name or specific skill name below appears, ALWAYS co-load `sabrimmo-skills`. Also co-load: `sabrimmo-combat` if the skill deals damage, `sabrimmo-buff` if it applies a buff, `sabrimmo-debuff` if it applies a status effect/CC.

| Class Names & Specific Skill Names | Skill |
|-------------------------------------|-------|
| **Novice**, Super Novice, Play Dead, Basic Skill, First Aid, job change gate, IDs 1-3 | `sabrimmo-skill-novice` |
| **Swordsman**, Bash, Magnum Break, Provoke, Endure, Sword Mastery, Two-Handed Sword Mastery, Increase HP Recovery, Increase Recuperative Power, Moving HP Recovery, Fatal Blow, Auto Berserk, IDs 100-109 | `sabrimmo-skill-swordsman` |
| **Mage**, Fire Bolt, Cold Bolt, Lightning Bolt, Soul Strike, Napalm Beat, Fire Ball, Fire Wall, Thunderstorm, Stone Curse, Frost Diver, Sight (Mage), Energy Coat, IDs 200-213 | `sabrimmo-skill-mage` |
| **Archer**, Double Strafe, Arrow Shower, Arrow Repel, Improve Concentration, Owl's Eye, Vulture's Eye, IDs 300-306 | `sabrimmo-skill-archer` |
| **Acolyte**, Heal, Holy Light, Blessing, Increase AGI, Decrease AGI, Angelus, Pneuma, Safety Wall, Warp Portal, Teleport, Aqua Benedicta, Signum Crucis, Ruwach, Cure, IDs 400-414 | `sabrimmo-skill-acolyte` |
| **Thief**, Envenom, Detoxify, Hiding, Steal, Double Attack, Improve Dodge, Pick Stone, Sand Attack, Throw Stone, Back Slide, IDs 500-509 | `sabrimmo-skill-thief` |
| **Merchant**, Mammonite, Cart Revolution, Discount, Overcharge, Pushcart, Vending (skill), Loud Exclamation, Change Cart, Identify, Enlarge Weight Limit, IDs 600-609 | `sabrimmo-skill-merchant` |
| **Knight**, Lord Knight, Pierce, Bowling Bash, Brandish Spear, Two-Hand Quicken, THQ, Auto Counter, Charge Attack, Spear Stab, Spear Boomerang, Spear Mastery, Riding (Knight), Cavalry Mastery, One-Hand Quicken, IDs 700-710 | `sabrimmo-skill-knight` |
| **Wizard**, High Wizard, Storm Gust, Lord of Vermillion, LoV, Meteor Storm, Ice Wall, Quagmire, Jupitel Thunder, Water Ball, Earth Spike, Heaven's Drive, Fire Pillar, Frost Nova, Sight Blaster, Sightrasher, Sense, Napalm Vulcan, IDs 800-813 | `sabrimmo-skill-wizard` |
| **Hunter**, Sniper, trap, Ankle Snare, Blast Mine, Claymore Trap, Freezing Trap, Land Mine, Sandman, Flasher, Shockwave Trap, Skid Trap, Talk Trap, Blitz Beat, falcon (Hunter), Auto-Blitz, Detect, Spring Trap, Remove Trap, Beast Bane, Falconry Mastery, Phantasmic Arrow, Steel Crow, IDs 900-917 | `sabrimmo-skill-hunter` |
| **Priest**, High Priest, Sanctuary, Magnus Exorcismus, Kyrie Eleison, Lex Aeterna, Lex Divina, Resurrection, Turn Undead, Aspersio, B.S. Sacramenti, Gloria, Magnificat, Impositio Manus, Suffragium, Recovery, Status Recovery, Increase SP Recovery, Mace Mastery, IDs 1000-1018 | `sabrimmo-skill-priest` |
| **Assassin**, Assassin Cross, Sonic Blow, Cloaking, Enchant Poison, Poison React, Venom Dust, Venom Splasher, Grimtooth, Katar Mastery, Right Hand Mastery, Left Hand Mastery, katar, dual wield (Assassin), Enchant Deadly Poison, EDP, Soul Destroyer, IDs 1100-1111 | `sabrimmo-skill-assassin` |
| **Blacksmith**, Whitesmith, Adrenaline Rush, Weapon Perfection, Power Thrust, Over Thrust, Maximize Power, Hammer Fall, forge, forging, Weaponry Research, Weapon Research, Hilt Binding, Skin Tempering, Iron Tempering, Steel Tempering, Cart Termination, IDs 1200-1230 | `sabrimmo-skill-blacksmith` |
| **Crusader**, Paladin, Grand Cross, Shield Boomerang, Holy Cross, Shield Charge, Devotion, Auto Guard, Reflect Shield, Defender, Providence, Faith, Shrink, Spear Quicken, IDs 1300-1313 | `sabrimmo-skill-crusader` |
| **Sage**, Professor, Endow, Flame Launcher, Frost Weapon, Lightning Loader, Seismic Weapon, Volcano, Deluge, Violent Gale, Land Protector, Hindsight, Auto Spell, Dispel, Dispell, Magic Rod, Spell Breaker, Free Cast, Dragonology, Advanced Book, Abracadabra, IDs 1400-1421 | `sabrimmo-skill-sage` |
| **Bard**, Clown, Minstrel, song, Musical Strike, Frost Joker, Adaptation to Circumstances, Encore, Pang Voice, A Poem of Bragi, Bragi, Apple of Idun, A Whistle, Assassin Cross of Sunset, Lullaby, Mr. Kim, Eternal Chaos, Drum on the Battlefield, Ring of Nibelungen, Loki's Veil, Into the Abyss, Invulnerable Siegfried, ensemble, performance, IDs 1500-1537 | `sabrimmo-skill-bard` |
| **Dancer**, Gypsy, dance, Slinging Arrow, Scream, Charming Wink, Service for You, Please Don't Forget Me, PDFM, Fortune's Kiss, Humming, Ugly Dance, Wink of Charm, Hip Shaker, Dazzler, Gypsy's Kiss, Slow Grace, Hermode's Rod, IDs 1520-1557 | `sabrimmo-skill-dancer` |
| **Monk**, Champion, Triple Attack, Chain Combo, Combo Finish, Asura Strike, Extremity Fist, Guillotine Fist, Steel Body, Mental Strength, Fury, Critical Explosion, spirit sphere, Finger Offensive, Throw Spirit Sphere, Investigate, Occult Impaction, Ki Explosion, Ki Translation, Spirits Recovery, Blade Stop, Iron Fists, Dodge (Monk), combo chain, combo window, sitting, IDs 1600-1615 | `sabrimmo-skill-monk` |
| **Rogue**, Stalker, Back Stab, Raid, Intimidate, Divest, Strip Helm, Strip Shield, Strip Armor, Strip Weapon, Close Confine, Plagiarism, Plagiarize, copy skill, Snatcher, auto-steal, Tunnel Drive, Steal Coin, Compulsion Discount, Double Strafe (Rogue), IDs 1700-1718 | `sabrimmo-skill-rogue` |
| **Alchemist**, Creator, Biochemist, Acid Terror, Acid Demonstration, Demonstration, Potion Pitcher, Chemical Protection, CP Helm/Shield/Armor/Weapon, Pharmacy (Alchemist), Axe Mastery (Alchemist), Potion Research, Call Homunculus, Rest Homunculus, Resurrect Homunculus, IDs 1800-1815 | `sabrimmo-skill-alchemist` |

### Class Cross-Loading Rules (First→Second Class)

When working on a second class, ALSO load its first-class parent (shared prereq skills):

| Second Class | Also Load | Shared Skills |
|-------------|-----------|---------------|
| `sabrimmo-skill-knight` | `sabrimmo-skill-swordsman` | Sword Mastery, Provoke, Endure |
| `sabrimmo-skill-crusader` | `sabrimmo-skill-swordsman` + `sabrimmo-skill-knight` | Riding, Cavalry Mastery, Spear Mastery, Sword Mastery |
| `sabrimmo-skill-wizard` | `sabrimmo-skill-mage` | All bolt spells, Fire Wall, Sight |
| `sabrimmo-skill-sage` | `sabrimmo-skill-mage` + `sabrimmo-skill-wizard` | Earth Spike, Heaven's Drive, Fire Pillar |
| `sabrimmo-skill-hunter` | `sabrimmo-skill-archer` | Improve Concentration, Vulture's Eye, Owl's Eye |
| `sabrimmo-skill-bard` | `sabrimmo-skill-archer` | All Archer passives |
| `sabrimmo-skill-dancer` | `sabrimmo-skill-archer` | All Archer passives |
| `sabrimmo-skill-priest` | `sabrimmo-skill-acolyte` | All Acolyte skills |
| `sabrimmo-skill-monk` | `sabrimmo-skill-acolyte` | Pneuma, Warp Portal, Heal |
| `sabrimmo-skill-assassin` | `sabrimmo-skill-thief` | Double Attack, Hiding, Envenom |
| `sabrimmo-skill-rogue` | `sabrimmo-skill-thief` | Steal, Hiding, Double Attack, Improve Dodge |
| `sabrimmo-skill-blacksmith` | `sabrimmo-skill-merchant` | Enlarge Weight Limit, Pushcart |
| `sabrimmo-skill-alchemist` | `sabrimmo-skill-merchant` | Enlarge Weight Limit, Pushcart |

### Overlapping Skills — ALWAYS Load Both

These skill pairs share underlying systems. When EITHER skill's keywords appear, load BOTH:

| Pair | Why They Overlap |
|------|------------------|
| `sabrimmo-buff` ↔ `sabrimmo-debuff` | Share `ro_status_effects.js`/`ro_buff_system.js`, same apply/remove engine. Many skills apply both buffs AND debuffs. |
| `sabrimmo-combat` ↔ `sabrimmo-skills` | Most skills deal damage through the combat pipeline. Any offensive skill needs both. |
| `sabrimmo-items` ↔ `sabrimmo-weight` | Weight is a core item property. Equip/unequip/drop all affect weight. |
| `sabrimmo-items` ↔ `sabrimmo-economy` | All trading/vending/shopping involves item transfer. |
| `sabrimmo-chat` ↔ `sabrimmo-combat-log` | Combat log outputs to the Chat widget (Combat tab). |
| `sabrimmo-persistent-socket` ↔ `realtime` | Both deal with Socket.io — persistent-socket is architecture, realtime is events/protocol. |
| `enemy-ai` ↔ `sabrimmo-monster-skills` | Monster AI executes monster skills in the AI tick loop. |
| `sabrimmo-skill-bard` ↔ `sabrimmo-skill-dancer` | 8 ensemble skills require both classes, shared performance system. |
| `sabrimmo-skill-wizard` ↔ `sabrimmo-skill-sage` | Sage shares Wizard handlers for Earth Spike, Heaven's Drive, Fire Pillar. |
| `sabrimmo-skill-knight` ↔ `sabrimmo-skill-crusader` | Crusader shares Knight's Riding, Cavalry Mastery, Spear Mastery. |
| `sabrimmo-companions` ↔ `sabrimmo-skill-hunter` | Falcon system is both a companion and Hunter feature. |
| `sabrimmo-companions` ↔ `sabrimmo-skill-alchemist` | Homunculus is both a companion and Alchemist feature. |
| `sabrimmo-companions` ↔ `sabrimmo-skill-knight`/`crusader` | Mount is both a companion and Knight/Crusader feature. |
| `sabrimmo-companions` ↔ `sabrimmo-skill-merchant` | Cart is both a companion feature and Merchant skill system. |
| `sabrimmo-crafting` ↔ `sabrimmo-skill-alchemist` | Pharmacy is Alchemist skill 1800. |
| `sabrimmo-crafting` ↔ `sabrimmo-ammunition` | Arrow Crafting produces arrows (shared crafting+ammo). |
| `sabrimmo-companions` ↔ `sabrimmo-skill-alchemist` (summons) | Summon Flora/Marine Sphere are Alchemist skills. |
| `sabrimmo-economy` ↔ `sabrimmo-skill-merchant` | Vending/Discount/Overcharge are Merchant skills tied to economy. |
| `sabrimmo-economy` ↔ `sabrimmo-npcs` | NPC shops are part of the economy system. |
| `sabrimmo-guild` ↔ `sabrimmo-pvp-woe` | WoE requires guilds; guild castles are WoE objectives. |
| `sabrimmo-npcs` ↔ `sabrimmo-click-interact` | NPCs are clickable interactables (shared click detection). |
| `sabrimmo-right-click-player-context` ↔ `sabrimmo-chat` | Whisper option prefills chat input via `StartWhisperTo()`. |
| `sabrimmo-right-click-player-context` ↔ `sabrimmo-trading` | Trade option calls `TradeSubsystem::RequestTrade()`. |
| `sabrimmo-right-click-player-context` ↔ `sabrimmo-party` | Party Invite option calls `PartySubsystem::InvitePlayer()`. |
| `sabrimmo-target-skill` ↔ `sabrimmo-skills` | Targeting mode is part of skill execution flow. |
| `sabrimmo-skills-vfx` ↔ `sabrimmo-skills` | VFX accompanies skill execution (visual feedback). |
| `sabrimmo-chat` ↔ `sabrimmo-party` | Party chat (`%` prefix) flows through both systems. |
| `sabrimmo-zone` ↔ `sabrimmo-npcs` | NPCs are placed within zones; zone entry loads NPC data. |
| `sabrimmo-items` ↔ `sabrimmo-cards` | Cards compound into item slots; card bonuses are item properties. |
| `sabrimmo-items` ↔ `sabrimmo-refine` | Refining modifies item stats (ATK/DEF bonuses). |
| `sabrimmo-items` ↔ `sabrimmo-ammunition` | Arrows are equippable items in the ammo slot. |
| `sabrimmo-sprites` ↔ `sabrimmo-enemy` | Enemy sprites use SpriteCharacterActor as primary actor (no BP_EnemyCharacter). |
| `sabrimmo-sprites` ↔ `sabrimmo-3d-to-2d` | 3D-to-2D pipeline produces atlases consumed by the sprite runtime system. |
| `sabrimmo-navmesh` ↔ `enemy-ai` | NavMesh pathfinding replaces straight-line movement in the AI tick loop. |
| `sabrimmo-navmesh` ↔ `sabrimmo-enemy` | NavMesh affects enemy spawn position snapping and movement broadcasting. |
| `sabrimmo-death` ↔ `sabrimmo-stats` | Death penalty involves EXP loss calculation based on level/class. |
| `sabrimmo-mvp` ↔ `sabrimmo-monster-skills` | MVP monsters have skill rotations and HP threshold triggers. |
| `sabrimmo-mvp` ↔ `enemy-ai` | MVP slave spawning (NPC_SUMMONSLAVE) integrates with AI lifecycle. |
| `sabrimmo-storage` ↔ `sabrimmo-items` | Storage reuses `FInventoryItem` and item attribute preservation (refine/cards/forge). |
| `sabrimmo-storage` ↔ `sabrimmo-economy` | 40z access fee, account-shared storage, mutual exclusion with vending. |
| `sabrimmo-trading` ↔ `sabrimmo-items` | Trade transfers preserve item attributes; `trade_flag` bitmask blocks untradeable items. |
| `sabrimmo-trading` ↔ `sabrimmo-economy` | Zeny exchange, atomic transfers, anti-duplication uses same patterns as economy. |
| `sabrimmo-options` ↔ `sabrimmo-esc-menu` | ESC menu "Option" button opens the Options panel. |
| `sabrimmo-options` ↔ `sabrimmo-3d-world` | Brightness slider pushes to PostProcessSubsystem. |
| `sabrimmo-3d-world` ↔ `sabrimmo-zone` | Each zone has its own post-process preset loaded on transition. |
| `sabrimmo-3d-world` ↔ `sabrimmo-sprites` | Sprite rendering depends on exposure/lighting from post-process presets. |
| `sabrimmo-landscape` ↔ `sabrimmo-ground-textures` | Landscape material variants use ground texture system. |
| `sabrimmo-landscape` ↔ `sabrimmo-environment-grass` | Grass output is configured on the landscape material. |
| `sabrimmo-ground-textures` ↔ `sabrimmo-material-decals` | Decals overlay on top of ground textures; original RO textures used for both. |
| `sabrimmo-map` ↔ `sabrimmo-zone` | World map grid entries come from the zone registry. |
| `sabrimmo-map` ↔ `sabrimmo-npcs` | Guide NPC marks display on the world map. |
| `sabrimmo-damage-numbers` ↔ `sabrimmo-combat` | Damage numbers render results of the combat pipeline. |
| `sabrimmo-audio-combat` ↔ `sabrimmo-combat` | Combat events trigger hit sounds via the audio subsystem. |
| `sabrimmo-audio` ↔ `sabrimmo-audio-player` / `sabrimmo-audio-enemy` / `sabrimmo-audio-combat` | Parent audio skill + the three subskills share AudioSubsystem, SoundBase loading, attenuation. Load parent WITH any subskill touched. |
| `sabrimmo-audio-player` ↔ `sabrimmo-options` | Volume sliders (Master/BGM/SFX/Ambient/UI) persist via UOptionsSaveGame and push to AudioSubsystem components. |
| `sabrimmo-audio-player` ↔ `sabrimmo-zone` | BGM switches on zone transition via Zone_BGM_Table. Ambient layers also swap per zone. |
| `sabrimmo-audio-player` ↔ `sabrimmo-login-screen` | Login screen has its own BGM (separate from in-game zone music). |
| `sabrimmo-audio-enemy` ↔ `sabrimmo-enemy` | Monster SFX timing ties to attack/die/move events on `FEnemyEntry`. Move SFX fires on OnAnimCycleComplete; status sounds fire on status:applied. |
| `sabrimmo-audio-enemy` ↔ `sabrimmo-debuff` | Status:applied events drive freeze/stun/etc. SFX via audio-enemy. |
| `sabrimmo-item-drop-system` ↔ `sabrimmo-items` | Ground items become `FInventoryItem` on pickup (preserves refine/cards). |
| `sabrimmo-item-drop-system` ↔ `sabrimmo-enemy` | Enemy death triggers drop rolls and spawns ground item actors. |
| `sabrimmo-item-drop-system` ↔ `sabrimmo-party` | Party loot modes (Each Take / Party Share / Individual / Shared) gate pickup eligibility. |
| `sabrimmo-item-drop-system` ↔ `sabrimmo-buff` | Bubble Gum / HE Bubble Gum multiply drop rates (90% cap). |
| `sabrimmo-item-drop-system` ↔ `sabrimmo-skill-blacksmith` | Greed is the Blacksmith AoE pickup skill. |
| `sabrimmo-item-drop-system` ↔ `sabrimmo-options` | Drop sound tier toggles live in OptionsSubsystem (6 tiers). |
| `sabrimmo-rig-animate` ↔ `sabrimmo-3d-to-2d` | UniRig produces skeletons + animations that feed into the render/pack atlas pipeline. |
| `sabrimmo-rig-animate` ↔ `sabrimmo-enemy` | Non-humanoid monsters rigged via UniRig render as `SpriteCharacterActor` enemies. |
| `sabrimmo-rig-animate` ↔ `sabrimmo-sprites` | Rigged animations drive per-animation atlas folders consumed by the sprite runtime system. |
| `sabrimmo-login-screen` ↔ `sabrimmo-resolution` | Login screen uses deferred widget creation due to standalone viewport timing. |
| `sabrimmo-esc-menu` ↔ `sabrimmo-login-screen` | Return-to-char-select flow transitions through LoginFlowSubsystem. |
| `sabrimmo-esc-menu` ↔ `sabrimmo-death` | Respawn button in ESC menu during death state. |
| `sabrimmo-right-click-player-context` ↔ `sabrimmo-storage` | Storage mutually excludes with right-click trade option. |

### Multi-System Task Examples

Load ALL listed skills. When uncertain, load more:
- "Fix Bash stun" → `sabrimmo-skill-swordsman` + `sabrimmo-skills` + `sabrimmo-combat` + `sabrimmo-debuff` + `sabrimmo-buff`
- "Add new skill with VFX" → `sabrimmo-skills` + `sabrimmo-combat` + `sabrimmo-skills-vfx` + `full-stack`
- "Build a new HUD panel" → `sabrimmo-ui` + system-specific skill + `sabrimmo-persistent-socket`
- "Party EXP not working" → `sabrimmo-party` + `sabrimmo-stats` + `debugger`
- "Card bonuses wrong in combat" → `sabrimmo-cards` + `sabrimmo-items` + `sabrimmo-combat` + `debugger`
- "Homunculus not attacking" → `sabrimmo-companions` + `sabrimmo-skill-alchemist` + `sabrimmo-combat` + `debugger`
- "Implement new trap" → `sabrimmo-skill-hunter` + `sabrimmo-skills` + `sabrimmo-combat` + `sabrimmo-debuff`
- "Bard ensemble not working" → `sabrimmo-skill-bard` + `sabrimmo-skill-dancer` + `sabrimmo-skills` + `sabrimmo-buff` + `debugger`
- "Regen not working" → `debugger` + `sabrimmo-weight` + `sabrimmo-buff` + `sabrimmo-debuff`
- "Add vending shop" → `sabrimmo-economy` + `sabrimmo-skill-merchant` + `sabrimmo-items` + `sabrimmo-companions` + `full-stack`
- "Craft potion failing" → `sabrimmo-crafting` + `sabrimmo-skill-alchemist` + `sabrimmo-items` + `debugger`
- "New zone with NPCs" → `sabrimmo-zone` + `sabrimmo-npcs` + `sabrimmo-click-interact` + `sabrimmo-audio`
- "Monster skill rotation" → `sabrimmo-monster-skills` + `enemy-ai` + `sabrimmo-combat` + `sabrimmo-skills`
- "Arrow element not working" → `sabrimmo-ammunition` + `sabrimmo-items` + `sabrimmo-combat` + `debugger`
- "Socket event not arriving" → `debugger` + `sabrimmo-persistent-socket` + `realtime`
- "Enemy walking through walls" → `sabrimmo-navmesh` + `enemy-ai` + `sabrimmo-enemy` + `debugger`
- "Enemy sprite not showing" → `sabrimmo-sprites` + `sabrimmo-enemy` + `debugger`
- "Add new monster sprite" → `sabrimmo-sprites` + `sabrimmo-3d-to-2d` + `sabrimmo-enemy` + `sabrimmo-art`
- "Death penalty not working" → `sabrimmo-death` + `sabrimmo-stats` + `debugger`
- "MVP not spawning slaves" → `sabrimmo-mvp` + `enemy-ai` + `sabrimmo-monster-skills` + `debugger`
- "Ground effect not ticking" → `sabrimmo-skills` + `sabrimmo-combat` + class-specific skill + `debugger`
- "Ground item issues (not dropping, pickup fail, icon missing, floating, sound not playing)" → `sabrimmo-item-drop-system` + `debugger` (+ `sabrimmo-party` if loot modes, + `sabrimmo-buff` if drop rate buff, + `sabrimmo-enemy` if death-triggered drops, + `sabrimmo-options` if drop sounds, + `sabrimmo-items` if pickup-to-inventory)
- "Drop rate verification or rAthena table check" → `sabrimmo-item-drop-system` + `sabrimmo-items` + `enemy-ai`
- "Rig + animate a non-humanoid enemy (UniRig)" → `sabrimmo-rig-animate` + `sabrimmo-enemy` + `sabrimmo-sprites` + `sabrimmo-3d-to-2d` + `sabrimmo-art`
- "UniRig crash / bone placement wrong" → `sabrimmo-rig-animate` + `debugger`
- "BGM not playing / zone music wrong / BGM loop broken" → `sabrimmo-audio-player` + `sabrimmo-audio` + `debugger` (+ `sabrimmo-zone` if per-zone, + `sabrimmo-login-screen` if login BGM, + `sabrimmo-persistent-socket` if zone change)
- "Volume slider / mute / audio options not saving" → `sabrimmo-audio-player` + `sabrimmo-options` + `debugger`
- "Player SFX (swing, hit, level up, heal, equip, potion, UI click)" → `sabrimmo-audio-player` + `sabrimmo-audio` + target system skill (`sabrimmo-combat` / `sabrimmo-items` / `sabrimmo-stats` / `sabrimmo-ui`)
- "Monster SFX (attack, die, move, stand, body material, status)" → `sabrimmo-audio-enemy` + `sabrimmo-audio` + `sabrimmo-enemy` + `debugger` (+ `sabrimmo-debuff` if status sounds)
- "Combat hit / skill impact SFX" → `sabrimmo-audio-combat` + `sabrimmo-audio-player` + `sabrimmo-audio` (+ `sabrimmo-skills-vfx` if skill impact, + `sabrimmo-combat` if auto-attack)
- "Ambient zone audio (water, wind, birds)" → `sabrimmo-audio-player` + `sabrimmo-audio` + `sabrimmo-zone`
- "Design a new game system (balance, RO-style formula, server logic)" → `agent-architect` + `planner` + relevant system skill (`sabrimmo-stats` / `sabrimmo-combat` / `sabrimmo-skills` / `enemy-ai`) + `full-stack`

See `CLAUDE.md` → "Multi-System Tasks" for 60+ additional examples.

---
## UE5 BLUEPRINT PROTOCOL (Critical)
Before ANY Blueprint suggestion:
1. **Check built-in settings FIRST** — Is there a checkbox in Details panel that solves it?
2. **Check CharacterMovement** — Max Acceleration, Use Acceleration for Paths, Ground Friction
3. **Check Animation Blueprint** — Velocity-based checks (not input-based)
4. **Provide exact node names** as they appear in UE5 5.7 search
5. **Include pin names** (execution, input, output)
6. **Always end with Compile & Save**
7. **Use correct UE5 5.7 terminology**:
   - "My Blueprint" (not "Blueprint Props")
   - "Details" (not "Properties")
   - "Cast To [Class]" (not "Cast")
   - "Bind Event to Function" with param named "Data" (String) for Socket.io

### NULL GUARD PLACEMENT — MANDATORY IN ALL INSTRUCTIONS
Place a null guard (IsValid node → Branch) at EVERY one of these points:
| When | What to Guard | Pattern |
|------|--------------|---------|
| Start of any function using a widget ref | `InventoryWindowRef`, `GameHudRef`, `StatAllocationWindowRef`, etc. | IsValid → Branch → FALSE: Print error + Return |
| Before emitting any Socket.io event | `SocketManagerRef` | IsValid → Branch → FALSE: Print "SocketManager null" + Return |
| Before calling functions on spawned actors | Any `BP_OtherPlayerCharacter`, `BP_EnemyCharacter` ref | IsValid → Branch → FALSE: skip |
| After `Get Actor Of Class` results | Any actor lookup | IsValid → Branch → FALSE: Print error |
| Before `Cast To` results are used | Cast output object | Cast failed pin → Print or Return |
| Before accessing `HUDManagerRef` from widget | `HUDManagerRef` in any WBP_ | IsValid → Branch → FALSE: Print + Return |
| Before using inventory ID to send equip request | `EquippedWeaponInventoryId > 0` | int > 0 Branch → FALSE: no-op |
**Pattern in instructions**: When writing Blueprint steps, always include "**NULL GUARD**: IsValid([Ref]) → Branch → FALSE: Print String '[Context]: [Ref] is null' → Return Node"

---
## UE5 DESIGN PATTERNS (Mandatory)

Apply these patterns in ALL Blueprint, C++, and server-side work. When suggesting or implementing features, **choose the correct pattern first**, then build.

### 1. Component-Based Architecture
- **Rule**: Favor ActorComponents over monolithic Actors. Each component = one responsibility.
- **Apply**: New gameplay features → create a dedicated `UActorComponent` subclass (C++) or Blueprint Component.
- **Examples**: `UOtherCharacterMovementComponent` (interpolation), `UCombatComponent` (attack logic), `UInventoryComponent` (item management).
- **Anti-pattern**: Stuffing movement + combat + inventory + UI logic into one Actor class.

### 2. Game Instance Pattern
- **Rule**: Use `UMMOGameInstance` for data that persists across levels/maps — auth tokens, character data, stats, settings.
- **Apply**: Any data needed before/after level transitions → store in GameInstance, not in individual Actors.
- **Anti-pattern**: Storing persistent state in PlayerController or GameMode (destroyed on level change).

### 3. Manager Pattern
- **Rule**: Centralized managers coordinate groups of related actors/objects. One manager per domain.
- **Apply**: `UEnemySubsystem` (enemy entity registry), `UOtherPlayerSubsystem` (remote player registry), `InventoryManager`, `QuestManager`.
- **Convention**: Managers hold Maps/Arrays of managed objects, expose `Register`/`Unregister`/`Get` functions. C++ managers are UWorldSubsystems (auto-created, world-lifetime).
- **Anti-pattern**: Each actor independently tracking all other actors of its type.

### 4. Interfaces (UE5 Interfaces)
- **Rule**: Use Blueprint Interfaces (`BPI_`) or C++ interfaces (`IInterface`) to define shared contracts between unrelated classes.
- **Apply**: `BPI_Damageable` (anything that can take damage: players, enemies, destructibles), `BPI_Interactable` (NPCs, chests, doors), `BPI_Targetable` (anything that can be targeted).
- **Benefits**: Decouples systems — combat doesn't need to know if target is player vs enemy, just that it implements `BPI_Damageable`.
- **Anti-pattern**: Long Cast chains (`Cast To BP_Enemy`, `Cast To BP_Player`, `Cast To BP_NPC`...) — use an interface instead.

### 5. Event-Driven Architecture
- **Rule**: Use Event Dispatchers (Blueprint) / Delegates (C++) for loose coupling. Publishers don't know about subscribers.
- **Apply**: `OnPlayerStatsUpdated`, `OnHealthChanged`, `OnItemPickedUp`, `OnCombatStateChanged` — widgets bind to these, not poll on Tick.
- **C++**: `DECLARE_DYNAMIC_MULTICAST_DELEGATE` for Blueprint-bindable delegates.
- **Blueprint**: Event Dispatchers in "My Blueprint" panel → Call, Bind, Unbind.
- **Anti-pattern**: Widgets using Event Tick to poll GameInstance for stat changes. Direct function calls across unrelated systems.

### 6. Object Pooling
- **Rule**: Pre-allocate and recycle frequently spawned/destroyed actors instead of Spawn/Destroy per use.
- **Apply**: Damage numbers, projectiles, particle effects, chat message widgets, floating combat text.
- **Implementation**: Manager holds a TArray of inactive objects → `GetFromPool()` activates one → `ReturnToPool()` hides and resets it.
- **When NOT to use**: Rarely spawned actors (managers, player character). Only pool objects with high spawn/destroy frequency.
- **Anti-pattern**: `SpawnActor` + `DestroyActor` every frame for damage numbers.

### 7. Widget Composition Pattern
- **Rule**: Build complex UIs from small, reusable child widgets. Parent widgets compose children, not inherit.
- **Apply**: `SInventoryWidget` composes slot entries. Each `UWorldSubsystem` owns one `SCompoundWidget` (Slate). Each subsystem = one domain.
- **Data flow**: Subsystem populates data, widget reads it. Delegate callbacks for user interaction.
- **Anti-pattern**: One massive widget with all UI elements. Deeply nested widget inheritance hierarchies.

### 8. Single Responsibility Principle (SRP)
- **Rule**: Every class, component, function, and Blueprint graph serves ONE purpose.
- **Apply**: Split large Event Graphs into Functions/Macros. One function = one action. One component = one system.
- **Metrics**: If a function needs >20 nodes or a C++ function is >50 lines → split it.
- **Anti-pattern**: `HandleEverything()` functions. Event Graph with 200+ nodes and no functions.

### 9. Dependency Injection
- **Rule**: Pass dependencies explicitly (via constructor, function params, or Expose on Spawn) rather than hard-coding `GetAllActorsOfClass` or global lookups.
- **Apply**: Widgets receive their data source via Expose on Spawn. Components receive owning manager reference on init.
- **Acceptable lookups**: `GetGameInstance` (singleton by design), `GetPlayerController(0)` (single local player).
- **Anti-pattern**: Every widget independently calling `GetAllActorsOfClass()` at runtime instead of using subsystem APIs.

### 10. State Machine Pattern
- **Rule**: Use explicit states for entities with complex behavior. Finite State Machines (FSM) for enemies, UI screens, combat phases.
- **Apply**: Enemy AI states (Idle → Patrol → Chase → Attack → Return). Player combat states (Idle → Attacking → Casting → Dead). UI flow states (Login → CharSelect → InGame).
- **Implementation**: Enum variable + Switch node (Blueprint) or `enum class` + switch (C++). Animation Blueprints already use State Machines — mirror this for gameplay logic.
- **Anti-pattern**: Nested boolean spaghetti (`if IsAttacking && !IsDead && !IsCasting && HasTarget...`).

### 11. Structured Logging Strategy
- **Rule**: Use categorized, leveled logging for all systems. Never `Print String` in production — use `UE_LOG` with categories.
- **C++ Categories**: Define per-system: `DECLARE_LOG_CATEGORY_EXTERN(LogCombat, Log, All)` — then `UE_LOG(LogCombat, Warning, TEXT("..."))`.
- **Blueprint**: Use `Print String` ONLY for debug (Development Only = checked). For permanent logging, call C++ log functions.
- **Server (Node.js)**: Use the existing structured logger with levels: `error`, `warn`, `info`, `debug`. Include context: `[Combat] [CharID:123] Damage dealt: 50`.
- **Levels**: `Error` (broken), `Warning` (unexpected but handled), `Log/Info` (important state changes), `Verbose/Debug` (per-frame, disabled in shipping).

### 12. Code Cleanliness Standards
- **Early returns**: Reduce nesting — validate inputs at top, return/continue early on failure.
- **Extract functions**: If Blueprint node sequence appears twice → extract to a Function. If C++ block repeats → refactor to a method.
- **Comment boxes**: Group related Blueprint nodes with Comment Boxes (C shortcut). Label with system name.
- **Named constants**: No magic numbers. Use `UPROPERTY(EditDefaultsOnly)` constants or `#define`/`constexpr` in C++. Server-side: use the `CONSTANTS` object pattern already in `index.js`.
- **Consistent formatting**: C++ follows UE5 coding standard. JS follows existing `index.js` style. Blueprints flow left-to-right, top-to-bottom.

### Pattern Selection Quick Reference
| Building... | Primary Pattern(s) |
|-------------|-------------------|
| New gameplay feature | Component-Based + SRP + Event-Driven |
| New UI screen | Widget Composition + Dependency Injection + Event-Driven |
| New enemy type | State Machine + Interface (`BPI_Damageable`) + Manager |
| New manager/system | Manager + SRP + Structured Logging |
| Frequent spawn/despawn | Object Pooling + Manager |
| Cross-system communication | Interfaces + Event Dispatchers |
| Persistent data | Game Instance Pattern |
| Debugging/monitoring | Structured Logging |

---
## SERVER-AUTHORITATIVE RULE
All game logic decisions happen on the server (`server/src/index.js`):
- **Combat**: Damage calc, range check, cooldowns, death/respawn
- **Stats**: Base/derived stat calculation, stat point allocation
- **Movement**: Position validation (client sends, server validates)
- **Economy**: Item drops, gold, trading
- **Never** trust client-sent values for authoritative data

---
## SEQUENTIAL THINKING (`mcp7_sequentialthinking`)
-   **Use for**: Any task requiring >3 steps, debugging, architecture decisions, research

| Task Complexity | Thoughts | Trigger |
|-----------------|----------|---------|
| **Trivial** | 0 | Simple lookup, single-file edit, known UE5 setting |
| **Standard** | 1-2 | Multi-file change, Blueprint + server coordination |
| **Complex** | 3-4 | Architecture decision, debugging networking, combat system |
| **Deep** | 5-9 | Research, root cause analysis, multiplayer sync issues |
-   **Advanced features**:
    - `isRevision: true` → Revise previous thinking
    - `branchFromThought` → Explore alternatives
    - `needsMoreThoughts: true` → Extend when problem is bigger than expected

---
## EXECUTION PROTOCOL (RED/GREEN)

### IF FIXING A SERVER BUG
1. **ANALYSIS**: Use `mcp7_sequentialthinking` to reason through root cause
2. **RED PHASE**: Create `scripts/repro_issue.js` that FAILS due to the bug. Run it with `node`.
3. **GREEN PHASE**: Apply fix to `server/src/index.js`
4. **VERIFY**: Run repro script again. Must PASS. Restart server if needed.

### IF FIXING A C++ BUG
1. **ANALYSIS**: Read the .h and .cpp files, check UE5 API docs
2. **FIX**: Apply minimal fix
3. **VERIFY**: Confirm with `Build > Build Solution` or Live Coding in editor

### IF ADDING A FEATURE (Server + Client)
1. **ANALYSIS**: Use `mcp7_sequentialthinking` to outline changes across layers
2. **SERVER FIRST**: Implement Socket.io events / REST endpoints in `index.js`
3. **DATABASE**: Add migrations if new columns/tables needed
4. **CLIENT C++**: Add USTRUCTs, UPROPERTYs, functions if needed
5. **BLUEPRINT**: Provide step-by-step instructions for UE5 editor work
6. **DOCUMENT**: Update relevant `docsNew/` files (invoke `project-docs`)

### UNIVERSAL
-   Use `node` for server scripts, `npx ts-node` for TypeScript utilities
-   Delete temp scripts after success unless told to keep
-   Always restart Node.js server after `index.js` changes

---
## CROSS-LAYER COORDINATION
When a feature spans client ↔ server:
```
1. Define Socket.io event name and JSON payload format
2. Implement server handler in index.js
3. Document in docsNew/04_Integration/Networking_Protocol.md
4. Provide Blueprint instructions for client binding
5. Test end-to-end with at least one client
```

---
## SAFEGUARDS (Anti-Failure)
1. **Iteration Limits**: Max 5 attempts. After 3 failures → use sequential thinking to analyze, then change approach.
2. **Scope Control**: One function/component per edit cycle.
3. **Anti-Hallucination**: Never assume UE5 API or Node.js behavior. Verify via documentation or web search.
4. **Halt & Escalate**: STOP if same error 3+ times, would break existing systems, or need architectural decision.
5. **Minimal Diffs**: Keep edits focused. No drive-by refactoring.
6. **Context Confirmation**: Before major changes, confirm understanding. State assumptions explicitly.
7. **UE5 Settings First**: Always check built-in UE5 settings before suggesting complex workarounds.
8. **Server Authority**: Never put authoritative game logic on the client.

---
## TOOL FIRST STRATEGY
-   Do not rely solely on basic bash commands (`ls`, `cat`, `grep`) for complex tasks.
-   If a task involves parsing, multi-step logic, or heavy file manipulation, **create a temporary utility script** (e.g., `scripts/tool_parser.js`) to do the work reliably.

---
## TRAJECTORY ANALYSIS (Reflection)
Before taking action:
1. Review recent chat history
2. Ask: "Did I try this already? Did it fail?"
3. If previous attempt failed → STOP → Change approach or create diagnostic tool

---
## ALWAYS
-   **Verify**: After EVERY code/config change, run it to confirm it works
-   **Services**: Restart Node.js server after `index.js` changes
-   **Date**: Current year is 2026
-   **Research**: When uncertain about UE5 API → search FIRST, implement SECOND
-   **Documentation**: Update `docsNew/` after ANY Blueprint instruction, server code change, or DB modification
-   **Naming**: Follow project naming conventions (BP_, WBP_, ABP_ prefixes; PascalCase classes; camelCase variables)

---
## POST-TASK OUTPUT
After completing any task:
1. Update Memory MCP if new knowledge gained
2. Update `docsNew/` if game systems changed (invoke `project-docs` skill)
3. Output: Summary, Next Steps, Improvement Suggestions

---
## PROJECT QUICK REFERENCE

### Key Files
| File | Purpose |
|------|---------|
| `server/src/index.js` | ALL server logic (REST + Socket.io + combat + stats + enemies) |
| `server/package.json` | Node.js dependencies |
| `client/SabriMMO/Source/SabriMMO/` | C++ source (GameInstance, HttpManager, Character, etc.) |
| `client/SabriMMO/SabriMMO.uproject` | UE5 project file |
| `database/init.sql` | Database initialization |
| `docsNew/**/*.md` | 30+ documentation files covering all systems (organized by category) |

### C++ Classes
| Class | File | Purpose |
|-------|------|---------|
| `UMMOGameInstance` | `MMOGameInstance.h/cpp` | Auth tokens, character data, stats, persistent state |
| `UHttpManager` | `MMOHttpManager.h/cpp` | HTTP requests to REST API |
| `ASabriMMOCharacter` | `SabriMMOCharacter.h/cpp` | Local player character |
| `ASabriMMOPlayerController` | `SabriMMOPlayerController.h/cpp` | Player input, click-to-move |
| `UOtherCharacterMovementComponent` | `OtherCharacterMovementComponent.h/cpp` | Remote player interpolation |

### Key Socket.io Events
| Event | Direction | Purpose |
|-------|-----------|---------|
| `player:join` | C→S | Authenticate and join world |
| `player:position` | C→S | Send position (30Hz) |
| `player:moved` | S→C | Broadcast other player positions |
| `combat:attack` | C→S | Initiate auto-attack |
| `combat:damage` | S→C | Broadcast damage dealt |
| `combat:health_update` | S→C | Sync HP/MP to all clients |
| `chat:message` | C→S | Send chat message |
| `chat:receive` | S→C | Receive chat message |

### Blueprint Naming
| Prefix | Type | Example |
|--------|------|---------|
| `BP_` | Blueprint Actor | `BP_MMOCharacter`, `BP_EnemyCharacter` |
| `WBP_` | Widget Blueprint (legacy, all replaced by C++ Slate) | — |
| `ABP_` | Animation Blueprint | `ABP_unarmed` |

---
## REFERENCE (Detailed Tables)
See `global_rules_appendix.md` for:
- Research tool selection matrix (all MCP search tools)
- Thinking depth per skill
- Memory MCP store/prune protocol
- Performance directives
