# Pryth — Build Plan (Capital Town)

| Field                | Value                                          |
| -------------------- | ---------------------------------------------- |
| Zone name (server)   | `Pryth`                                        |
| Display name         | `Pryth`                                        |
| Level asset          | `L_Pryth` (in `Content/SabriMMO/Levels/`)      |
| Type                 | `town`                                         |
| RO Classic reference | Prontera (`prontera`) — pastoral capital       |
| Fidelity target      | High (1-2 weeks, screenshot-match)             |
| Status               | Build plan landed 2026-05-08 — Phase A pending |
| Owner                | pladr                                          |

## Implementation status (2026-05-10)

| Done        | Item                                                                                              | Where                                                                                                                              |
| ----------- | ------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| ✓           | Build plan doc                                                                                    | `docsNew/05_Development/Pryth_Build_Plan.md` (this file)                                                                           |
| ✓           | Phase A — Server registration (zone + 4 warps + ZONE_INFO + grid row 4 col 10)                    | `server/src/ro_zone_data.js`, `ro_world_map_data.js`                                                                               |
| ✓           | Phase B — UE5 level scaffold (`L_Pryth.umap` exists)                                              | `Content/SabriMMO/Levels/L_Pryth.umap`                                                                                             |
| ✓           | Phase C (partial) — `M_Landscape_RO_18` master + `MI_Landscape_PavedTown_v1` MI + Layer Infos    | `Materials/Environment/M_Landscape_RO_18`, `Materials/Environment/v3/MI_Landscape_PavedTown_v1`, `Levels/L_Pryth_sharedassets/`    |
| in-progress | Phase C — paint Cobble base, then Brick on plaza + Pebble on side paths                           | UE5 Landscape Mode → Paint tab                                                                                                     |
| pending     | Phase D — Decals (`MI_RODecal_*`) + painted GrassDense in residential side-yards                  | UE5 level                                                                                                                          |
| pending     | Phase E — Place reusable 3D props (houses/fountain/market stalls)                                 | UE5 level (~100 Hunyuan3D GLBs imported)                                                                                          |
| pending     | Phase F — North moat (`AWaterArea` actor)                                                         | UE5 level                                                                                                                          |
| ✓           | Phase G (partial) — NavMesh exported (7934 verts / 5859 faces)                                    | `server/navmesh/Pryth.obj`                                                                                                         |
| pending     | Phase G — Pryth zone preset in `PostProcessSubsystem.cpp::ApplyZonePreset`                        | C++                                                                                                                                |
| ✓           | Phase H (partial) — Audio mappings (`bgm_08` + market/birds/bell ambient)                         | `AudioSubsystem.cpp`                                                                                                               |
| pending     | Phase H — NPCs (1 Kafra + 3 shop dealers + 1 Guide)                                               | `ro_npc_data.js`, `ro_shop_data.js`, `ro_dialogue_data.js` + UE5 level                                                            |
| deferred    | Phase B (future) — Bespoke Hunyuan3D pass (~12 signature assets)                                  | `_tools/hunyuan_asset_pipeline.py`                                                                                                 |

### Warp arrival distance fix (2026-05-09)

First-pass warp coordinates placed arrival points within the outbound `AWarpPortal` trigger sphere → instant re-warp loop. Fixed by moving destinations further from outbound portals:

| Warp                     | Final destX/Y/Z         | Distance to outbound in Pryth                                                            |
| ------------------------ | ----------------------- | ---------------------------------------------------------------------------------------- |
| `prtsouth_to_pryth`      | `(-7500, 395, 70)`      | **911u** from `(-8410, 395, 20)` ✓                                                        |
| `grassfield05_to_pryth`  | `(-530, -7800, 100)`    | **483u** from `(-530, -8280, 50)` — marginal, watch for re-warp if AWarpPortal sphere ≥ 480u |

Lesson captured in `feedback-warp-arrival-distance.md` and added to `sabrimmo-zone` skill Common Issues. Always place arrivals ≥1.5× trigger radius (preferably 2-3×) from the outbound portal.

## TL;DR

A high-fidelity capital-town zone modeled on Ragnarok Online Classic's Prontera. Pastoral medieval feel — warm-toned brick-and-timber houses arranged in residential and commercial districts, a central plaza with a stone fountain and a warrior statue, market stalls under striped awnings, lampposts at intersections, and a single defensive moat (AWaterArea) running across the northern edge spanned by 2-3 stone bridges.

Outdoor archetype: Landscape Actor with sculpted terrain (flat center, gentle rolling edges, steeper north bank where the moat sits), `M_Landscape_RO_17` with the warm grassland MI variant, `GT_V3_Grassland` painted scatter, 40-60 RO-original ground decals (dirt + cobblestone wear + flower scatter). NPCs: 1 Kafra (central plaza), 3 shop dealers (weapon / armor / tool), 1 Guide NPC. BGM `bgm_08 "Theme of Prontera"` + market chatter + birds + church bell ambient stack.

**Phase A** (this build, ~3-5 days) = reuse-only with the 100+ existing Hunyuan3D imported assets. The library already has every house, fountain, statue, well, market stall, awning, lamppost, banner, fence, and cart needed for a Prontera-style town. Pryth ships looking nearly indistinguishable from Prontera, with the moat as its visual signature.

**Phase B** (deferred, ~1 additional week) = ~30 bespoke Pryth-signature assets generated via Hunyuan3D 2.1 + 3D Coat hand-paint cohesion: a unique castle keep, signature gates, a central church or temple, Pryth-flagged banners, a named statue, a signature fountain. Replaces the reused Prontera-tagged props so every screenshot reads as "this is Pryth, not Prontera with a moat."

## Table of contents

1. [Overview](#1-overview)
2. [Architecture decisions](#2-architecture-decisions)
3. [Server changes](#3-server-changes)
4. [UE5 level setup](#4-ue5-level-setup)
5. [Materials & decals](#5-materials--decals)
6. [Lighting & post-process](#6-lighting--post-process)
7. [Audio](#7-audio)
8. [North moat (AWaterArea)](#8-north-moat-awaterarea)
9. [3D asset strategy](#9-3d-asset-strategy-phase-a-reuse--phase-b-bespoke)
10. [Layout sketch & districts](#10-layout-sketch--districts)
11. [NavMesh export](#11-navmesh-export)
12. [Phased schedule](#12-phased-schedule)
13. [Test matrix](#13-test-matrix)
14. [Risk register](#14-risk-register)
15. [Appendix A — Existing imported asset inventory](#appendix-a--existing-imported-asset-inventory)
16. [Appendix B — Hunyuan3D per-asset prompts (Phase B)](#appendix-b--hunyuan3d-per-asset-prompts-phase-b-deferred)
17. [Appendix C — Decal type → density formula](#appendix-c--decal-type--density-formula)
18. [Appendix D — Outdoor light placement guide](#appendix-d--outdoor-light-placement-guide)
19. [Appendix E — Cross-references](#appendix-e--cross-references)
20. [Glossary](#glossary)

---

## 1. Overview

### What this zone is

A faithful capital-town zone in the spirit of Ragnarok Online Classic's Prontera. In the RO universe, Prontera is the central kingdom — a walled medieval town with a central fountain plaza, a castle to the north, residential and commercial districts to the south, and a defensive moat. Visually it's defined by:

- Warm-toned cobblestone main streets and packed-dirt side paths
- Brick-and-timber houses with red-tile or thatch roofs (`house_prontera_*` family)
- A central plaza with a stone fountain and a warrior statue on a pedestal
- Striped market awnings, baskets of produce, barrels and crates lining a market square
- Iron lampposts at intersections (Amatsu-style works fine — close enough silhouette)
- Flagged banners hanging from second-story balconies
- A single moat at the northern edge, dark blue water, spanned by 2-3 stone bridges
- Surrounding wall geometry suggesting fortification (north only — south and east/west are open / fenced)
- Warm gold afternoon light, low contrast, slight haze

Pryth is the player's hub town — a non-combat zone with the full Kafra service suite, basic shops, and a Guide NPC for navigating the world. It sits on the world map between `prontera_south` (the player's first field zone) and `GrassField05` (a connecting field zone), making it a natural traversal node.

### Why this zone

Pryth exercises every system in the outdoor-town archetype:

- **Landscape Actor** with sculpted terrain (flat plaza + gentle slopes + sharper north bank for the moat)
- **`M_Landscape_RO_17`** master with warm grassland MI variant — first town zone using the v17 paint-layer system
- **DBuffer decals** (RO-original textures only) — densest visible decal pass yet
- **Landscape Grass V3** painted scatter — first town to use painted clumps rather than uniform random
- **AWaterArea mixed-mode** — second deployment after SewerDungeon01, first outdoor / clean-blue palette
- **`AMMONPC` data-driven NPCs** — first town to populate Kafra + shop + Guide via the unified `AMMONPC` actor (rather than the legacy `AKafraNPC` + `AShopNPC` per-type actors)
- **PostProcess pastoral preset** — first dedicated outdoor town preset (`prontera` exists but Pryth gets its own signature warmth)
- **Audio: market + birds + bell ambient stack** — already mapped in `AudioSubsystem.cpp` as the "Prontera (5 zones)" entry, just add Pryth to that map

A successful build here is the template for `GrassField07_Town`, all major capital-town zones in the future (Geffen, Payon, Morroc, Lutie, etc.), and any zone of similar archetype.

### Naming

| Item | Value |
|---|---|
| Zone name (server `ro_zone_data.js` key) | `Pryth` |
| Display name (UI strings) | `Pryth` |
| UE5 level filename | `L_Pryth.umap` |
| UE5 level path | `Content/SabriMMO/Levels/L_Pryth` |
| Warp ID (south gate exit) | `pryth_to_prtsouth` |
| Warp ID (west gate exit) | `pryth_to_grassfield05` |
| Reverse warp from prontera_south | `prtsouth_to_pryth` |
| Reverse warp from GrassField05 | `grassfield05_to_pryth` |
| Water area ID (north moat) | `pryth_moat_north` |

Naming follows the newer PascalCase convention used by `SewerDungeon01`, `GrassField05`, `GrassField07`. Older zones (`prontera`, `prontera_south`, `prt_dungeon_01`) retain their snake_case names — that's a historical convention, not actively maintained.

### Fidelity target

**High** — 1-2 week build, side-by-side screenshot validation against RO Classic Prontera references.

Acceptance criteria for Phase A (reuse-only, ~3-5 days):

- Visually reads as "this is clearly a pastoral medieval capital town" to anyone who's played RO
- Layout has clear district zoning (civic / residential / plaza / commercial)
- Decal placement is per-camera-pass (walked the level from player POV, refined where the eye lingers)
- North moat is the visual signature — clearly bounded, not an overlooked perimeter feature
- Lighting tuned per RO atmospheric reference (warm gold, low contrast, slight haze)
- All NPCs are clickable and reachable via `AMMONPC` → `DialogueSubsystem` / `KafraSubsystem`
- Phase A acceptable to ship as-is; Phase B turns it bespoke later

Acceptance criteria for Phase B (bespoke, deferred):

- ~30 Pryth-signature assets generated via Hunyuan3D 2.1 + 3D Coat hand-paint cohesion
- Replaces all `house_prontera_*` props with Pryth-tagged equivalents
- Castle keep at the north end (not present in Phase A — north end is the moat + a low wall in Phase A)
- Central church / temple replaces a generic large building in the civic district
- Named statue (e.g., founding king) replaces `statue_warrior`
- Pryth-flagged banners replace `banner_horizontal`
- Signature fountain replaces `fountain_basin` / `fountain_stone`

---

## 2. Architecture decisions

### Decision matrix

| Decision | Choice | Rationale |
|---|---|---|
| Ground actor | **Landscape Actor** | Outdoor town. Pryth needs sculpted terrain (flat plaza + rolling edges + steeper north bank for moat). Slope-detection in `M_Landscape_RO_17` gives free walkable/unwalkable visual cue. Painted grass layers give controlled clumps in residential side-yards. |
| Wall actor | **Static mesh modular wall** at the north edge only | The moat needs a low stone wall on the south side of the water (between town and moat). South / east / west edges use `fence_wooden` / `fence_stone` for soft boundaries. |
| Water | **Single `AWaterArea` actor** at the north edge | Outdoor blue palette — leave defaults (shallow `(0.08, 0.18, 0.5)`, deep `(0.03, 0.08, 0.35)`, abyss `(0.005, 0.015, 0.05)`). NOT the sewer green-black. Auto-detect deep cells via raycast → spawns `NavArea_Null` boxes that cut the navmesh AND server-side OBJ. Players cannot walk into the moat (deep water = unwalkable). Aqua Benedicta + Water Ball gated automatically. |
| Bridges | **`bridge_stone_arch` × 2-3** | Existing imported asset. Place across the moat at evenly spaced points. Each bridge top is walkable, sides cut into navmesh modifiers if needed. |
| Lighting | **Outdoor preset** — DirectionalLight + SkyLight + warm fog | `PostProcessSubsystem::SetupSceneLighting` already handles outdoor zones via the `flags.indoor === false` branch. Pryth uses the same template. Lampposts as decoration only — no Point Light per lamppost (would over-light at night). |
| Post-process | **Per-zone preset in `PostProcessSubsystem.cpp`** | Pastoral town palette: Bloom 0.4, Vignette 0.25, Exposure +1.5, WhiteTemp 6800K (warm), GainHighlights `(1.03, 1.0, 0.96)` warm. Distinct from `prontera` zone preset (Pryth slightly warmer / softer). |
| Fog | **Default outdoor fog** | Density 0.004, falloff 0.5, warm inscatter `(0.7, 0.65, 0.5)`. Standard outdoor — no override needed unless Phase B adds atmospheric haze. |
| BGM | `bgm_08 "Theme of Prontera"` | Canonical RO town BGM. Already imported. |
| Ambient | `se_prtmarket01` + `se_prtbird_01` + `se_prtchbell_01` | Market chatter + birds + church bell. Already in the `Prontera (5 zones)` ambient bundle in `AudioSubsystem.cpp`. Add `Pryth` to that bundle. |
| Map flag — `town` | **`true`** | No enemy spawns — non-combat zone. |
| Map flag — `indoor` | **`false`** | Outdoor lighting active. |
| Map flag — `nosave` | **`false`** | Kafra save IS allowed (it's a Kafra hub). |
| Map flag — `noteleport` | **`false`** | Fly Wing allowed (standard town behavior). |
| Map flag — `noreturn` | **`false`** | Butterfly Wing allowed. |
| Map flag — `pvp` | **`false`** | PvE town. |
| NPCs | **1 Kafra + 3 shop dealers + 1 Guide** | Standard town service set. Place via `AMMONPC` (data-driven) using the `NPC_REGISTRY` keys defined in `ro_npc_data.js`. |
| Enemy spawns | **None** | Towns have no enemies. `enemySpawns: []`, no `spawnPool`. |

### Why use a Landscape Actor

The `sabrimmo-landscape` skill states: Landscape Actor is the right call for outdoor zones with sculpted terrain, grass scatter, and slope-based gameplay. Pryth has all three:

- Sculpted terrain (flat plaza + rolling residential edges + steeper north bank for moat)
- Grass scatter (painted GT_V3_Grassland in residential side-yards, NOT in plaza or commercial)
- Slope-based gameplay (the moat bank is steep enough — >45° — that the slope-rock material kicks in automatically as a "you can't climb here" cue, even before AWaterArea's nav cuts make it formally unwalkable)

A static-mesh-only floor approach would waste the slope detection, the painted grass system, and the LOD controls Landscape provides. Use the Landscape.

### Why a single `AWaterArea` instead of multiple canals

The user's spec: "single canal/moat at the north end of town." A single actor at the north edge:

- Auto-detects deep cells via raycast (the moat bank is sculpted steep enough that raycasts miss the floor → flagged deep → cut from navmesh)
- One AWaterArea = one entry in `pryth.activeWaterAreas` map per player → simple state tracking
- Single visual material instance = uniform appearance across the moat
- Single `WaterAreaId` for `water:enter` / `water:exit` events

If the design later expands to inner canals (Phase B), additional AWaterArea actors can be placed and the server's reference-counted state handles overlap correctly.

### Why no enemy spawns

Pryth is a non-combat town. Per the user's spec, there are no enemy spawns. This is consistent with `prontera` (the canonical town in the existing registry) which also has empty `enemySpawns: []` and no `spawnPool[]`. Players use Pryth for shops, Kafra services, and traversal — not combat.

### Why NPCs go in Phase A, not Phase B

Even with reuse-only assets, the town isn't testable as a town until at least the Kafra works. The Kafra needs a `kafraNpcs[]` registry entry, an `AMMONPC` (or `AKafraNPC`) actor placed in the level, and a destination list. Same for shops and Guide — without them, Pryth is just a level. Phase A includes the full NPC suite.

---

## 3. Server changes

### 3.1 `Pryth` registry entry

Add to `server/src/ro_zone_data.js` `ZONE_REGISTRY`:

```javascript
Pryth: {
    name: 'Pryth',
    displayName: 'Pryth',
    type: 'town',
    flags: {
        noteleport: false,    // Fly Wing allowed (standard town)
        noreturn: false,      // Butterfly Wing allowed
        nosave: false,        // Kafra save allowed
        pvp: false,
        town: true,           // No enemy spawns
        indoor: false         // Outdoor lighting active
    },
    defaultSpawn: { x: 0, y: -3500, z: 100 },  // south plaza, near south gate arrival point
    levelName: 'L_Pryth',
    warps: [
        // South gate → prontera_south (reciprocal of prtsouth_to_pryth)
        {
            id: 'pryth_to_prtsouth',
            x: 0, y: -4500, z: 100,
            radius: 250,
            destZone: 'prontera_south',
            destX: 0, destY: 11500, destZ: 30   // adjust to actual L_PrtSouth north-edge AWarpPortal
        },
        // West gate → GrassField05 (reciprocal of grassfield05_to_pryth)
        {
            id: 'pryth_to_grassfield05',
            x: -4500, y: 0, z: 100,
            radius: 250,
            destZone: 'GrassField05',
            destX: 13000, destY: 1830, destZ: 150  // adjust to actual L_GrassField05 east-edge AWarpPortal
        }
    ],
    kafraNpcs: [
        {
            id: 'kafra_pryth_1',
            name: 'Kafra Employee',
            x: 200, y: 0, z: 100,                  // central plaza, slightly east of fountain
            destinations: [
                { zone: 'prontera', displayName: 'Prontera', cost: 1200 },
                { zone: 'GrassField05', displayName: 'GrassField05', cost: 600 },
                { zone: 'prontera_south', displayName: 'Prontera South Field', cost: 600 }
            ]
        }
    ],
    enemySpawns: []   // Town zone — no enemies
}
```

**Coordinate placeholders**: All `x/y/z` values are educated guesses. They MUST be reconciled with actual UE5 actor positions in Phase 4 (UE5 level setup) — the server validates `radius * 2` (default 500u with `radius: 250`) tolerance between server warp coords and the AWarpPortal actor's UE5 location. Use unrealMCP `find_actors_by_name("WarpPortal")` after placement to get the actual coords, then update this block.

### 3.2 Reverse warps

Add to `prontera_south.warps[]` in `ro_zone_data.js`:

```javascript
{
    id: 'prtsouth_to_pryth',
    x: 0, y: 11500, z: 30,             // north edge of L_PrtSouth — adjust to actual AWarpPortal location
    radius: 250,
    destZone: 'Pryth',
    destX: 0, destY: -4500, destZ: 100
}
```

Add to `GrassField05.warps[]` in `ro_zone_data.js`:

```javascript
{
    id: 'grassfield05_to_pryth',
    x: 13000, y: 1830, z: 150,         // east edge of L_GrassField05 — adjust to actual AWarpPortal location
    radius: 250,
    destZone: 'Pryth',
    destX: -4500, destY: 0, destZ: 100
}
```

Both reverse warps need `AWarpPortal` actors placed in `L_PrtSouth` and `L_GrassField05` respectively. **This means edits to two existing levels in addition to creating L_Pryth.** Note these explicitly in Phase B's checklist — easy to forget.

### 3.3 `ro_world_map_data.js`

Add to `ZONE_INFO`:

```javascript
Pryth: { displayName: 'Pryth', category: 'town', levelRange: '—', biome: 'pastoral' },
```

Place on `WORLD_MAP_GRID`. Suggested placement: between `prontera_south` and `GrassField05` on the grid. The grid is 12x8 (cols × rows). Look at the existing grid layout and pick a `null` ocean cell that visually places Pryth in the right corridor. Verify with:

```bash
node -e "const d = require('./server/src/ro_world_map_data'); const flat = d.WORLD_MAP_GRID.flat().filter(Boolean); const dupes = flat.filter((z,i) => flat.indexOf(z) !== i); console.log(dupes.length ? 'DUPES: ' + dupes : 'OK')"
```

Should print `OK` (no duplicates).

### 3.4 NPC registry entries (`ro_npc_data.js`)

Add 5 new NPC entries — 1 Kafra (note: Kafra is also wired through `kafraNpcs[]` on the zone, but the NPC actor in-level uses `AMMONPC` with `NPCType = ENPCType::Kafra` per the `sabrimmo-npcs` skill OR the legacy `AKafraNPC`. For Pryth, follow whatever pattern exists in `L_Prontera` — likely `AKafraNPC`):

```javascript
// 1 Kafra (legacy AKafraNPC pattern)
'pryth_kafra_1': {
    npcId: 9100,
    type: NPC_TYPES.KAFRA,
    name: 'Kafra Employee',
    spriteId: 114,                      // canonical Kafra sprite
    zone: 'Pryth',
    position: { x: 200, y: 0, z: 100 },
    facing: 4,
    interactRadius: 300,
    shopId: null,
    dialogueId: null,                   // KafraSubsystem handles dialog
    questId: null,
    isActive: true
},

// 3 shop NPCs
'pryth_weapon_dealer': {
    npcId: 9101,
    type: NPC_TYPES.SHOP_WEAPON,
    name: 'Weapon Dealer',
    spriteId: 80,
    zone: 'Pryth',
    position: { x: 600, y: 0, z: 100 },
    facing: 4,
    interactRadius: 300,
    shopId: 'pryth_weapon',             // entry into SHOP_REGISTRY
    dialogueId: null,                   // ShopSubsystem opens directly
    questId: null,
    isActive: true
},
'pryth_armor_dealer': {
    npcId: 9102,
    type: NPC_TYPES.SHOP_ARMOR,
    name: 'Armor Dealer',
    spriteId: 75,
    zone: 'Pryth',
    position: { x: 600, y: 200, z: 100 },
    facing: 4,
    interactRadius: 300,
    shopId: 'pryth_armor',
    dialogueId: null,
    questId: null,
    isActive: true
},
'pryth_tool_dealer': {
    npcId: 9103,
    type: NPC_TYPES.SHOP_TOOL,
    name: 'Tool Dealer',
    spriteId: 90,
    zone: 'Pryth',
    position: { x: 600, y: -200, z: 100 },
    facing: 4,
    interactRadius: 300,
    shopId: 'pryth_tool',
    dialogueId: null,
    questId: null,
    isActive: true
},

// 1 Guide
'pryth_guide': {
    npcId: 9104,
    type: NPC_TYPES.GUIDE,
    name: 'Pryth Guide',
    spriteId: 105,
    zone: 'Pryth',
    position: { x: -800, y: -1000, z: 100 },
    facing: 6,
    interactRadius: 300,
    shopId: null,
    dialogueId: 'pryth_guide_dialog',
    questId: null,
    isActive: true
}
```

### 3.5 Shop registry entries (`ro_shop_data.js`)

Three new entries — start with the same item set as Prontera's equivalent dealers, adjust later:

```javascript
'pryth_weapon': [
    { itemId: 1101, buyPrice: 1500, stock: -1 },   // Sword (basic)
    { itemId: 1201, buyPrice: 700,  stock: -1 },   // Knife
    { itemId: 1301, buyPrice: 4000, stock: -1 },   // Axe
    // ... copy from prontera_weapon shop
],
'pryth_armor': [
    { itemId: 2101, buyPrice: 500,  stock: -1 },   // Guard
    { itemId: 2301, buyPrice: 4500, stock: -1 },   // Adventurer's Suit
    // ... copy from prontera_armor shop
],
'pryth_tool': [
    { itemId: 501, buyPrice: 50,    stock: -1 },   // Red Potion
    { itemId: 502, buyPrice: 200,   stock: -1 },   // Orange Potion
    { itemId: 1750, buyPrice: 3,    stock: -1 },   // Arrow
    // ... copy from prontera_tool shop
]
```

Recommend duplicating `prontera_weapon` / `prontera_armor` / `prontera_tool` shop entries as the Phase A starting point — both towns serve the same low-level player needs.

### 3.6 Dialogue tree for Guide NPC (`ro_dialogue_data.js`)

Add `'pryth_guide_dialog'` entry. Pattern follows `prontera_guide_dialog`. Provides 6 facility location marks (Weapon Shop, Armor Shop, Tool Shop, Inn, Kafra, Blacksmith). Each marker emitted via `map:mark` event when player selects the corresponding choice. See `sabrimmo-map` skill section "Guide NPC System" for the full pattern.

### 3.7 Database — no migration needed

`zone_name` is a free-text VARCHAR — `Pryth` is supported automatically. `level_name` is computed on-the-fly from `getZone(zone_name).levelName`. No DB column required.

### 3.8 Map flags rationale

| Flag | Value | Source |
|---|---|---|
| `town: true` | true | Capital town — no enemies |
| `indoor: false` | false | Outdoor sky / sun lighting active |
| `nosave: false` | false | Kafra save allowed (RO Classic standard for towns) |
| `noteleport: false` | false | Fly Wing allowed |
| `noreturn: false` | false | Butterfly Wing allowed |
| `pvp: false` | false | No PvP in PvE towns |

---

## 4. UE5 level setup

### 4.1 Duplicate `L_Prontera` → `L_Pryth`

**Never create a level from `File > New Level`** — the Level Blueprint is critical and easy to forget, and missing it causes a 10-second hang on the loading screen with no pawn appearing.

Steps:

1. UE5 Content Browser → `Content/SabriMMO/Levels/`
2. Right-click `L_Prontera` → Duplicate
3. Rename copy to `L_Pryth`
4. Open `L_Pryth`
5. Select all geometry actors copied from L_Prontera (houses, fountain, statues, walls, ground, lighting, AWarpPortals, AKafraNPC) — delete them all
6. Keep these intact:
   - Level Blueprint (verify in 4.3)
   - World Settings GameMode override
   - Existing `NavMeshBoundsVolume` (resize later)

### 4.2 World Settings — verify

Window → World Settings:

| Setting | Value |
|---|---|
| GameMode Override | `GM_MMOGameMode` |
| Default Pawn Class (in `GM_MMOGameMode` → Class Defaults) | **None** (NOT BP_MMOCharacter) |

If `Default Pawn Class` is set, the GameMode spawns a duplicate pawn at world origin alongside the Level Blueprint's spawn. Keep it None. The C++ base class `ASabriMMOGameMode` already sets it to `nullptr` — verify the BP doesn't override.

### 4.3 Level Blueprint — verify (CRITICAL)

Open the Level Blueprint (Window → Level Blueprint) and verify the 3 sections from `L_Prontera` were copied:

**A. Spawn and Possess Character (Event BeginPlay):**

1. `Event BeginPlay` → `Delay 0.2s` → `Cast To MMOGameInstance` (from `Get Game Instance`)
2. `Get Selected Character` → `Break Character Data` → extract `CharacterId`, `X`, `Y`, `Z`
3. Branch: `CharacterId > 0`
4. Branch: `Vector Length Squared(Make Vector(X,Y,Z)) != 0.0`
   - True: SpawnActor `BP_MMOCharacter` at `(X, Y, Z)` → Possess
   - False: SpawnActor `BP_MMOCharacter` at default `(0, -3500, 200)` → Possess
5. `Set Timer by Function Name` ("SaveCharacterPosition", 5.0s, Looping=true)

**B. SaveCharacterPosition (Custom Event, every 5s):**

- Cast → Get Selected Character → CharacterId, then Get Player Character(0) → Get Actor Location → Save Character Position

**C. Cleanup (Event End Play):**

- Clear Timer by Function Name "SaveCharacterPosition"

If any of the 3 sections is missing, abort — re-duplicate from `L_Prontera`. Don't try to rebuild the BP from scratch.

### 4.4 Required actors checklist

Per `Zone_System_UE5_Setup_Guide.md`:

- [ ] **Level Blueprint** with the 3 sections above (verified)
- [ ] **GameMode Override** = `GM_MMOGameMode`, **DefaultPawnClass = None** (verified)
- [ ] **NavMeshBoundsVolume** sized to cover the playable area (~10000 × 10000 × 1000 UU)
- [ ] **DirectionalLight** (auto-spawned by `PostProcessSubsystem`, but place a reference one in level for editor visibility)
- [ ] **SkyLight** (auto-spawned)
- [ ] **SkyAtmosphere** + **VolumetricCloud** (optional, for visual polish)
- [ ] **ExponentialHeightFog** (auto-configured)
- [ ] **Landscape Actor** (created in Phase C)
- [ ] **AWarpPortal** at south gate (`pryth_to_prtsouth`)
- [ ] **AWarpPortal** at west gate (`pryth_to_grassfield05`)
- [ ] **AWaterArea** at north moat (`pryth_moat_north`)
- [ ] **AKafraNPC** (or `AMMONPC` with `NPCType = ENPCType::Kafra`) at central plaza
- [ ] **AShopNPC** × 3 (or `AMMONPC` with `NPCType = ENPCType::SHOP_*`)
- [ ] **AMMONPC** × 1 for Guide (with `NPCType = ENPCType::Guide`)

Do **NOT** place these (removed in Phase 6, replaced by C++ subsystems):

- `BP_OtherPlayerManager`
- `BP_EnemyManager`

`BP_SocketManager` is dead code, kept in levels for compatibility — do not place a new one.

### 4.5 Greybox layout strategy (Day 1)

Block out the town footprint with simple cubes. Goal of greybox: validate the layout walks well, district zoning feels right, the central plaza is the right size, the moat reads as a real defensive feature.

Greybox sizes:
- Town footprint: 10000 × 10000 UU (10000 across × 10000 deep)
- Central plaza: 1500 × 1500 UU (open plaza centered on origin)
- Building blocks: 600 × 600 × 800 UU (greybox cube per building, will be replaced with `house_prontera_*` in Phase E)
- Moat strip: 3000 wide × 500 deep × 100 below ground (between Y=4000 and Y=4500)
- Bridges over moat: 200 wide × 800 long × 50 thick (3 bridges spaced 1500 apart)
- Roads: 400 wide cobblestone strips (greybox = darker material)

Don't apply final materials yet — greybox is for layout iteration only. Walk the level (PIE or Simulate) to validate the scale before starting Phase C.

---

## 5. Materials & decals

### 5.1 Landscape material — paint-driven pavements (v18)

**RO Classic Prontera is a PAVED town.** Players walk on warm-tan cobblestone everywhere except residential gardens. Pryth follows the same convention but with **3 distinct pavement types per district** (per the user spec):

- **Brick** — formal civic plaza floor (warm terracotta rectangular brick pattern)
- **Cobble** — main streets and default everywhere else (rounded stone)
- **Pebble** — residential side paths (small scattered river pebbles)

These three pavements coexist via UE5's **Landscape paint layer system**. A new master material `M_Landscape_RO_18` (extends v17) drives base-color blending from `LandscapeLayerSample` nodes — paint a region as one of the 3 layers, the material samples that pavement's texture there, with smooth blends at boundaries.

**Build the master + MI** via two Python scripts:

```python
# In UE5 Python console:
exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_landscape_ro_18.py").read())
exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_mi_paved_town.py").read())
```

**Create** (this is what `create_mi_paved_town.py` does): `MI_Landscape_PavedTown_v1` in `Content/SabriMMO/Materials/Environment/v3/`
**Parent**: `M_Landscape_RO_18`
**Reuse**: This MI is generic — applies to Pryth, future Prontera-clone, paved Geffen, any pastoral capital town. Per-zone tweaks (e.g., cooler WarmthTint for Geffen) via duplicating + overriding 1-2 params, OR creating child MIs parented to this one.

**v18 texture slots** (12 total — 9 from v17 + 3 NEW pavement slots):

| Slot | Texture | Role |
|---|---|---|
| `BrickTexture` (NEW) | `Ground/T_Brick_RO` | Painted "Brick" layer — plaza floor |
| `CobbleTexture` (NEW) | `Ground_Seamless/T_Cobble_RO_2k` | Painted "Cobble" layer — streets |
| `PebbleTexture` (NEW) | `Ground/T_Pebble_RO` | Painted "Pebble" layer — side paths |
| `GrassWarmTexture` | `Ground_Seamless/T_Cobble_RO_2k` | Default unpainted base (cobble for safety) |
| `GrassCoolTexture` | `Biomes/urban/T_Urban_Cobble_A` | Default cobble variation |
| `DirtTexture` | `ROZones/prontera_fields/T_Prt_Path_A` | Worn dirt for residential side strips + slope dirt |
| `RockTexture` | `ROZones/prontera_fields/T_Prt_Cliff_A` | Warm cliff for moat bank (slope-rock kick-in) |
| `GrassWarmNormal/CoolNormal/DirtNormal/RockNormal/GrassAO` | matching normal+AO maps | Subtle bump + crevice darkening |

**Texture generation**: `T_Brick_RO` and `T_Pebble_RO` don't ship with the project — they're generated via `_tools/generate_pryth_pavements.py` (run once, ~1 min on idle ComfyUI). Output PNGs go to `_tools/ground_texture_output/`; drag them into `Content/SabriMMO/Textures/Environment/Ground/` to import. Then re-run `create_mi_pryth.py` and the new slots populate.

**Override parameters** (tuned for stone primary):

| Parameter | Value | Reason |
|---|---|---|
| `WarmthTint` | `(1.00, 0.98, 0.93)` | Slightly warm-neutral — stone reads less yellow than grass |
| `SaturationMult` | `0.92` | Stone is muted vs lush grass |
| `BrightnessOffset` | `+0.02` | Marginal lift |
| `ContrastBoost` | `1.0` | Default soft pastoral mood |
| `GrassVariantBalance` | `0.50` | Balanced default-cobble A / B blend |
| `GrassNoiseScale` | `0.004` | Mid-low frequency variation |
| `MacroNoiseScale` | `0.0008` | Macro brightness variation across the town |
| `BrickTileSize` (NEW) | `3000` | Bricks read mid-tight |
| `CobbleTileSize` (NEW) | `2500` | Rounded cobble |
| `PebbleTileSize` (NEW) | `2000` | Small pebbles read tightest |
| `GrassWarmTileSize` | `2500` | Default-cobble tile |
| `GrassCoolTileSize` | `3247` | Irrational ratio |
| `DirtTileSize` | `3347` | Irrational ratio |
| `RockTileSize` | `2891` | Irrational ratio |
| `SlopeThreshold` | `0.60` | Rock kicks in on moat bank |
| `SlopeTransitionWidth` | `0.15` | Soft transition |
| `SlopeNoiseAmount` | `0.06` | Organic variation in slope cutoff |
| `SlopeNoiseFreq` | `0.012` | — |
| `DirtOnSlopes` | `0.10` | Paved → cliff is sharp |
| `DirtAmount` | `0.20` | Paved areas wear with foot traffic |
| `Roughness` | `0.92` | Stone is near-fully diffuse |
| `NormalStrength` | `0.85` | Cobblestone bumps read clearly |
| `AOStrength` | `0.55` | Crevices darken |
| `UVDistortStrength` | `45` | Less distortion — paved looks regular |

**Painter workflow** (after applying the MI to the Landscape):

1. **Shift+2** → Landscape Mode → **Paint** tab
2. Layer Infos to create (click `+` next to each → Create Layer Info → save):
   - `LI_Brick_Pryth` — paint plaza floor
   - `LI_Cobble_Pryth` — paint streets, main thoroughfares
   - `LI_Pebble_Pryth` — paint residential side paths
   - `LI_GrassDense_Pryth` — paint residential side-yards (drives Grass Output sprite scatter, separate from base color — see §5.5)
   - `LI_FlowerPatch_Pryth`, `LI_Debris_Pryth` — optional, also for sprite scatter
3. Select a layer, brush-paint with left-click. `[ ]` to resize brush
4. Result: each painted region samples its pavement texture; unpainted regions fall through to the v17 4-slot baseline (cobble in GrassWarm/Cool slots)

### 5.2 District-level surface variation

The same MI applies across the whole Landscape, but **decals** + **painted layers** create per-district variation on top of the cobble base:

| District | Base | Visual variation |
|---|---|---|
| Plaza (center) | Cobble | Path decals (worn-cobble pattern) + cracks + dark wagon stains converge into "old worn paving" |
| Main streets | Cobble | Lighter decal density — newer cobble |
| Commercial | Cobble | Path + dark stains (wagon ruts) — heavy traffic wear |
| Residential side-yards | Cobble + painted GrassDense | Painted GrassDense layer spawns 3D grass sprites; dirt decals create worn paths between houses |
| Civic district | Cobble | Cracks + path decals — formal but aged |
| Moat bank (north slope) | Slope-rock auto-applied | Moss decals on the wall base |

**Open question**: M_Landscape_RO_17's paint layer system may also blend a different BASE TEXTURE under painted areas (in addition to driving the grass-sprite Output node). If it does, painting GrassDense over a side-yard transitions the base from cobble → grass texture for a true "garden" feel. Verify in PIE — if the base doesn't transition, the grass sprites still sit on cobble and read as "potted garden in courtyard," which is also fine.

### 5.3 Wall material (north stone wall)

The low stone wall between the town and the moat uses `MI_Wall_Stone_Warm` — an existing instance from `Content/SabriMMO/Materials/Environment/`. If absent, create from `M_Equipment_Master` with a warm stone albedo + `Roughness=0.92` + `Metallic=0`.

### 5.4 Decal selection — pastoral town palette

Drop **40-60 decals total** across the town. Distribution:

| Decal MI base | Count | Tint | Use |
|---|---|---|---|
| `MI_RODecal_Dirt_*` | 12-18 | Default warm brown `(0.85, 0.80, 0.72)` | Worn paths in commercial district, around fountain, in front of shops |
| `MI_RODecal_Path_*` | 8-12 | Default | Cobblestone wear pattern in plaza + main street |
| `MI_RODecal_Moss_*` | 6-10 | Default | Walls of houses (low Z), bases of fences, around well, north stone wall |
| `MI_RODecal_Cracks_*` | 4-6 | Default | Broken cobblestone in plaza, north wall stress cracks |
| `MI_RODecal_DarkStain_*` | 3-5 | Default | Old wagon-wheel wear on commercial market floor, splash near fountain |
| `MI_RODecal_FlowerScatter_*` (if exists; else use Moss with green tint) | 7-9 | Default or `(0.65, 0.95, 0.55)` flower-green | Residential side-yards, plaza edges, near Guide NPC |

**Total: 40-60 decals**

**Rules** (from `_meta/04_Decals_Guide.md` and `feedback-decal-textures` memory):

- Pitch = **−90°** (projects straight down)
- Yaw = random 0-360°
- Scale: X = 1.5-5.0 (projection depth), Y/Z = 1.5-5.0 (footprint)
- Place slightly above ground surface (Z+5)
- Opacity stays at 0.35 (default — never crank above 0.5 or it looks painted-on)
- Slight overlap is good — natural coverage
- Mix 3+ types in the same area for richness
- **NEVER use AI-generated decal textures** (`Textures/Environment/Decals/`) — they look garish. Use only `MI_RODecal_*` (RO original textures from `RO_Original/`).

### 5.5 Landscape Grass V3 — painted layers

Apply `GT_V3_Grassland` via the Grass Output node in `M_Landscape_RO_17`. Use the **painted** placement method (Method B from `sabrimmo-environment-grass`) so grass clumps appear only where you want them — in residential side-yards, around the plaza edges, near the well, NOT in the plaza center or commercial floor.

After applying material to Landscape:
1. Shift+2 → Landscape Mode → Paint tab
2. Three layer infos to create:
   - `LI_GrassDense_Pryth` (paint where thick grass should be)
   - `LI_FlowerPatch_Pryth` (paint where flowers should be — near Guide NPC, plaza edges)
   - `LI_Debris_Pryth` (paint where rocks/leaves scatter — north moat bank, edges)
3. Click + next to each → Create Layer Info → save
4. Select layer, paint with left-click, [ ] to resize brush

**CRITICAL**: The Grass Output node MUST be added **manually in Material Editor** (NOT via Python). Python causes material compile failure. From the skill: *"The `LandscapeGrassOutput` node CANNOT be configured via Python."*

---

## 6. Lighting & post-process

### 6.1 PostProcess zone preset

Add a new `else if` block in `PostProcessSubsystem.cpp::ApplyZonePreset()`:

```cpp
else if (ZoneName == TEXT("Pryth"))
{
    // Pastoral capital town — warm gold afternoon
    Bloom = 0.40f;
    Vignette = 0.25f;
    ExposureBias = 1.5f;       // outdoor — compensate for sprite tonemapping
    WhiteTemp = 6800.f;        // warm
    GainHighlights = FVector4(1.03f, 1.0f, 0.96f, 1.0f);  // warm gold cast
}
```

This is similar to (but slightly warmer than) the existing `prontera` preset (`6800K`, `(1.03, 1.0, 0.96)`). Pryth's preset is intentionally near-identical because it's a faithful pastoral mimic — the player should feel the family resemblance.

If you want Pryth distinct from Prontera, push the warmth further: `WhiteTemp = 7200.f` (golden afternoon) and `GainHighlights = FVector4(1.05f, 1.01f, 0.94f, 1.0f)`. Test in PIE before committing.

### 6.2 Auto-lighting (outdoor)

`PostProcessSubsystem::SetupSceneLighting` handles outdoor zones automatically when `flags.indoor === false`:

- DirectionalLight: Intensity 3.14 lux, Color `(1.0, 0.95, 0.85)` warm white, Pitch −50°, Yaw 135°
- SkyLight: Intensity 2.5, Color `(0.95, 0.92, 0.85)` warm, CapturedScene source
- HeightFog: Density 0.004, Falloff 0.5, Inscattering `(0.7, 0.65, 0.5)` warm haze

No fog override needed for Pryth (defaults are appropriate). The Sewer used a heavy fog override — Pryth doesn't.

### 6.3 Directional shadow cap

The 2026-05-08 `SetDynamicShadowDistanceMovableLight(7000.f) + 3 cascades` config (in `PostProcessSubsystem.cpp::SetupSceneLighting`) applies to all outdoor zones. No additional config needed for Pryth.

### 6.4 Per-zone lampposts (optional, NOT recommended for Phase A)

The asset library has `lamppost_amatsu`. You can place 8-12 lampposts at street corners, but **do NOT** add a Point Light per lamppost in Phase A. Reason: at night (if a day/night cycle ever ships), 12 always-on Point Lights create over-lighting. Better to leave them as decoration only and add light-source emitters in Phase B if a day/night system is added.

---

## 7. Audio

### 7.1 BGM mapping

Add to `AudioSubsystem.cpp::OnWorldBeginPlay()` `ZoneToBgmMap.Add(...)` block:

```cpp
ZoneToBgmMap.Add(TEXT("Pryth"), BgmRoot + TEXT("bgm_08"));   // Theme of Prontera
```

Place this entry alongside the existing 5 Prontera-family entries (`prontera`, `prontera_south`, `prontera_north`, `prt_dungeon_01`, `prt_fild*`). The track is already imported.

### 7.2 Ambient layers

Pryth uses the same ambient stack as the Prontera family (market chatter + birds + church bell). Add to the `ZoneAmbientMap.Add(...)` block:

```cpp
ZoneAmbientMap.Add(TEXT("Pryth"), { PrtMarket, PrtBirds, PrtBell });
```

`PrtMarket`, `PrtBirds`, `PrtBell` are already-defined local FStrings at the top of the ambient block (paths to `se_prtmarket01`, `se_prtbird_01`, `se_prtchbell_01`). All three WAVs are already imported.

### 7.3 3D positional ambient (optional, Phase B)

For added polish, you could place a 3D positional ambient source at the central fountain (water gurgling) and at the church bell tower (bell tolls). The 3D ambient framework supports this via `Zone3DAmbientMap`. Defer to Phase B — Phase A's market+birds+bell stack already gives Pryth the right pastoral feel.

### 7.4 Recompile

Audio mappings live in `AudioSubsystem.cpp::OnWorldBeginPlay()` — a single function. Edit + Live Coding (CPP-only changes) OR full editor build (`SabriMMOEditor Win64 Development`). **Live Coding is unreliable** per `sabrimmo-build-compile`; prefer closing the editor and rebuilding.

---

## 8. North moat (AWaterArea)

### 8.1 Single AWaterArea actor

Place ONE `AWaterArea` actor at the north edge of Pryth. Place Actors → search "WaterArea" → drag into level.

**Properties**:

| Property | Value |
|---|---|
| `Location` | `(0, 4250, 100)` (center of moat strip, water surface at Z=100) |
| `Rotation` | `(0, 0, 0)` |
| `Scale` | `(1, 1, 1)` |
| `WaterAreaId` | `"pryth_moat_north"` |
| `WaterExtent` | `(1500, 250)` (3000 wide × 500 deep half-extents) |
| `bAutoDetectDeep` | **true** (default) |
| `bIsDeep` | false (auto-detect overrides) |
| `DeepDepthThreshold` | 200 (default — cells with floor 200u below water = deep) |
| `DepthSampleResolution` | 16 (default — 16x16 = 256 raycast samples) |
| `ShallowColor` | `(0.08, 0.18, 0.5)` (default — outdoor blue) |
| `DeepColor` | `(0.03, 0.08, 0.35)` (default) |
| `AbyssColor` | `(0.005, 0.015, 0.05)` (default) |
| `AbyssOpacity` | 1.0 (default — fully opaque at depth) |
| `AbyssDepth` | 900 (default) |
| `WaveSpeed` | 0.5 (default) |

### 8.2 Sculpt the moat bank

The Landscape needs to be sculpted under the AWaterArea so:

- Floor under the moat (Y=4000 to 4500, X=-1500 to 1500) sits at Z=−200 to Z=−400 (200-400u below water surface at Z=100)
- This makes raycasts under the water surface either MISS the floor (no floor → cell flagged deep) OR hit floor below `DeepDepthThreshold` (cell flagged deep)
- The moat bank rises sharply from Z=−400 to Z=100 at the moat edge — the sharp slope triggers the slope-rock material kick-in

After sculpting:
1. Save level
2. **Nudge any AWaterArea property** (e.g. tweak `WaterExtent` X by 1u back and forth) to retrigger `OnConstruction`
3. The depth scan re-runs, nav-modifier boxes are re-spawned for the deep cells
4. Build → Build Navigation in UE5 — the green nav overlay should now have a hole where the moat is

If the moat doesn't appear deep (no nav holes after Build Paths), the floor under the water is too shallow — drop it further.

### 8.3 Stone bridges

Place 2-3 `bridge_stone_arch` static meshes spanning the moat:

| Bridge | Position |
|---|---|
| West bridge | `(-1000, 4250, 100)` |
| Center bridge | `(0, 4250, 100)` |
| East bridge | `(1000, 4250, 100)` |

Each bridge is ~800 UU long (rotated 0° to span the Y axis). The bridge top is walkable — UE5 navmesh sees the bridge surface as walkable, and the deep-water nav-cut applies only to the water cells, NOT the bridge cells (the bridge is above the water and acts as a navigable platform).

If the navmesh doesn't generate over the bridge tops, ensure `bridge_stone_arch` has collision enabled on `ECC_Visibility` (default for static meshes) AND that the bridge mesh's bounding box is detectable by the navmesh build pass.

### 8.4 Server-side water tracking

Pryth's moat works the same as SewerDungeon01's canals — `water:enter` / `water:exit` events fire on overlap, server tracks `player.activeWaterAreas` Map for the player. Aqua Benedicta and Water Ball are gated automatically (player must be in the moat to cast — but realistically players won't be in the moat because it's deep / unwalkable).

**No `waterAreas[]` entry needed in `ro_zone_data.js`** — the AWaterArea actor handles its own visual + nav-cut behavior, and the navmesh-validation backstop handles deep blocking on its own. (Same pattern as SewerDungeon01 — see `sabrimmo-water` skill section "Caveat: terrain edits after placement" for the rationale.)

### 8.5 Outdoor blue palette — leave defaults

The default AWaterArea palette is already outdoor-blue. **Do NOT** override `ShallowColor` / `DeepColor` / `AbyssColor` — defaults are correct for a pastoral town moat.

---

## 9. 3D asset strategy (Phase A reuse + Phase B bespoke)

### 9.1 Phase A — reuse-only (this build, 3-5 days)

The imported library has nearly everything Pryth needs. Use the existing assets (paths under `3d art/Imported_to_UE5/Final/...`):

**Architecture (houses + civic):**
- `house_prontera_small.glb`
- `house_prontera_medium.glb`
- `house_prontera_tall.glb`
- `house_prontera_wide.glb`
- `house_prontera_corner.glb`

**Centerpiece (plaza features):**
- `fountain_basin.glb` OR `fountain_stone.glb` (pick one — `fountain_stone` is more screenshot-correct for Prontera)
- `statue_warrior.glb` (on `statue_pedestal.glb`)
- `well_wooden.glb` (residential well, paired with bucket)
- `well_stone.glb` (alternative)

**Props (commercial market):**
- `market_stall_red.glb` × 4
- `market_stall_blue.glb` × 4
- `awning_market.glb` × 6
- `awning_striped.glb` × 4
- `barrel_wooden.glb` + `barrel_open.glb` + `barrel_large.glb`
- `crate_small.glb` + `crate_wooden.glb` + `crate_large.glb`
- `sack_burlap.glb` + `sack_open.glb`
- `basket_woven.glb` + `basket_apple.glb`
- `cart_kafra.glb` (near Kafra NPC)
- `cart_merchant.glb` (commercial district)

**Props (atmospheric):**
- `lamppost_amatsu.glb` × 8-12 at intersections
- `banner_horizontal.glb` × 6-8 hanging from upper floors
- `sign_planted.glb` × 4 (district signs: Plaza, Market, Residential, Civic)
- `bench_wooden.glb` × 4 (seating in plaza)
- `table_wooden.glb` × 2 (commercial market)
- `haystack.glb` × 2 (residential edges)
- `fence_wooden.glb` (south + east + west soft boundaries — long sections)
- `fence_long.glb` + `fence_stone.glb` + `fence_long.glb` (variation)

**Terrain (north moat + bank):**
- `bridge_stone_arch.glb` × 3 (over moat)
- `rock_mossy_large.glb` × 4 (decorative, north bank)
- `rock_split.glb` × 3 (variation)
- `boulder_huge.glb` × 1 (focal, NW corner of town)
- `path_stone.glb` (walkways)

**Special:**
- `kafra_pad.glb` (small mat under Kafra NPC for visual emphasis — already imported as a special asset)

This list is ~80-100 placed assets. Total time to lay them out: 1-2 days.

### 9.2 Phase B — bespoke (deferred, ~1 week)

Generate via `_tools/hunyuan_asset_pipeline.py`:

| Asset | Replaces | Why |
|---|---|---|
| `pryth_castle_keep` | (not in Phase A — Phase B adds it) | Signature north-end civic building behind the moat. Multi-tier stone keep with crenelated parapets. |
| `pryth_main_gate` | A generic doorway prop | Signature south-gate entrance, two stone towers framing the warp portal. |
| `pryth_church` | A generic civic building | Central church with steeple, replaces a `house_prontera_corner` in the civic district. |
| `pryth_fountain` | `fountain_stone` | Distinctive Pryth-flagged fountain (e.g., 4-tier basin with named statues at corners). |
| `pryth_statue_founder` | `statue_warrior` | Founding king or first Pryth Kafra statue. Named, distinctive silhouette. |
| `pryth_banner_flag` × 3 variants | `banner_horizontal` | Pryth-flagged banners (different color/sigil per district). |
| `pryth_lamppost` | `lamppost_amatsu` | Pryth-distinct lamppost design (e.g., dragon-coiled iron post). |
| `pryth_house_a/b/c` (3 variants) | `house_prontera_small/medium/tall` | Bespoke 3-variant house family with Pryth roof tiles + window patterns. |

Total: ~12 unique assets + variants → ~20-25 GLBs after color variant generation.

**Process** (from `sabrimmo-3d-world` "CANONICAL Asset Generation Pipeline"):

1. Define asset list in `_tools/hunyuan_pryth_assets.py` (mirrors `hunyuan_v3_regional.py`)
2. Run multi-input perspective generation (2 strategies × 2 seeds per asset)
3. Quality-aware scoring → pick best mesh candidate
4. Decimate FIRST to 5K faces, THEN texture (30× speedup)
5. **Mandatory 3D Coat hand-paint cohesion pass** (see `sabrimmo-3d-world` "Cohesion Layer (Required for RO Classic Style)") — without it, even Hunyuan-generated assets lean photoreal and break the RO Classic style
6. Re-import to UE5
7. Replace placeholder assets in `L_Pryth`

Per-asset prompts → see Appendix B.

### 9.3 Phase B is optional

Phase A is a complete shippable town. Phase B is a polish pass that makes Pryth visually distinct from Prontera. The user has flagged "plan for maximum bespoke" but explicitly chose "reuse for now" — Phase A ships first, Phase B happens when bandwidth allows.

---

## 10. Layout sketch & districts

### 10.1 Top-down layout

```
                            NORTH (positive Y)
                                  ↑
    ────────────────────────────────────────────────────────────
    |                         MOAT                              |   Y = 4500 (north edge)
    |  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  |
    |  [west bridge]       [center bridge]      [east bridge]   |
    |  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  |   Y = 4000 (south edge of moat)
    |  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  |   stone wall
    |                                                            |
    |              CIVIC DISTRICT (north)                       |   Y = 3000 to 4000
    |   [house_prontera_corner × 4]   [house_prontera_wide × 2] |
    |   [Phase B: castle keep replaces center pair]              |
    |                                                            |
    |              RESIDENTIAL — UPPER (mid-north)              |   Y = 1500 to 3000
    |   [house_prontera_tall × 6]                                |
    |                                                            |
    |              CENTRAL PLAZA                                  |   Y = -500 to 1500
    |   ┌─────────────────────────┐                              |
    |   │     [statue_warrior]    │   [Kafra Employee] (200, 0)  |
    |   │    [fountain_stone]     │                              |
    |   │                         │   [Weapon Dealer] (600, 0)   |
    |   └─────────────────────────┘   [Armor Dealer] (600, 200)  |
    |                                  [Tool Dealer] (600, -200) |
    |                                                            |
    |              COMMERCIAL DISTRICT                            |   Y = -1500 to -500
    |   [market_stall_red × 4] [market_stall_blue × 4]           |
    |   [awnings + barrels + crates + sacks + baskets]            |
    |   [cart_merchant + cart_kafra]                             |
    |                                                            |
    |              [Pryth Guide NPC] (-800, -1000)               |
    |                                                            |
    |              RESIDENTIAL — LOWER (south)                   |   Y = -3000 to -1500
    |   [house_prontera_small × 8]   [house_prontera_medium × 6] |
    |                                                            |
    |              [Default spawn here] (0, -3500, 100)          |   Y = -3500 (warp arrival)
    |                                                            |
    |          [SOUTH GATE — pryth_to_prtsouth] (0, -4500, 100)  |   Y = -4500 (south edge)
    |                                                            |
    ────────────────────────────────────────────────────────────
                  ↑                                ↑
                  WEST GATE                       EAST EDGE
                  (-4500, 0, 100)                 (4500, 0, 100)
                  → GrassField05                   (closed — fences only)
                                  ↓
                            SOUTH (negative Y)
```

### 10.2 District zoning rules

Base surface is **cobblestone everywhere** (paved town). Decals + paint layers create per-district variation.

| District | Y range | Base | Decals | Grass paint | Lighting cue |
|---|---|---|---|---|---|
| Moat + bank | 4000 to 4500 | Slope-rock auto | Cracks + Moss on stone wall | Debris paint (rocks) | Bright reflective water |
| Civic (north) | 3000 to 4000 | Cobble | Path + Cracks (formal-but-aged) | None | Standard outdoor |
| Residential upper | 1500 to 3000 | Cobble | Dirt (worn paths between houses) + Moss + FlowerScatter | **Painted GrassDense in side-yards** | Warm afternoon |
| Plaza (center) | -500 to 1500 | Cobble | Path (worn-cobble pattern) + DarkStain (wagon wear) + Cracks | None (paved) | Warmest — sun pool effect |
| Commercial (south) | -1500 to -500 | Cobble | Dirt + DarkStain (heavy wagon wear) | None (paved) | Standard outdoor |
| Residential lower | -3000 to -1500 | Cobble | Dirt + Moss + FlowerScatter | **Painted GrassDense in side-yards** | Warm afternoon |

**Rule of thumb**: paved areas get worn-stone decals (Path/DarkStain/Cracks), residential side-yards get organic decals (Dirt/Moss/FlowerScatter) PLUS the painted GrassDense layer that spawns 3D grass sprites.

### 10.3 NPC placement (matches server registry coords)

| NPC | World coord (server) | Notes |
|---|---|---|
| Kafra Employee | `(200, 0, 100)` | Central plaza, slightly east of fountain. Matches `kafra_pryth_1.position`. |
| Weapon Dealer | `(600, 0, 100)` | East of plaza, lined up with Kafra. |
| Armor Dealer | `(600, 200, 100)` | NE of plaza. |
| Tool Dealer | `(600, -200, 100)` | SE of plaza. |
| Pryth Guide | `(-800, -1000, 100)` | West of commercial district, between plaza and lower residential. |

`AMMONPC` (or legacy `AKafraNPC` / `AShopNPC`) actors placed at these UE5 world coords. The `NpcId` property on each actor MUST match the `NPC_REGISTRY` key exactly (case-sensitive) — `pryth_kafra_1`, `pryth_weapon_dealer`, etc.

### 10.4 Walking the layout

Before placing real assets in Phase E, walk the greybox in PIE:

- Spawn at `(0, -3500, 100)` (south arrival point)
- Walk north to plaza — should take ~10 seconds at default move speed
- Click Kafra → opens dialog
- Walk further north past upper residential to civic district
- Cross center bridge → arrive at moat north edge (impassable due to navmesh cut, but you can stand on the bridge)
- Walk south back to plaza, then west to GrassField05 gate (-4500, 0)
- Walk south to spawn

Total walk time: ~30-45 seconds. If anything feels too long or too short, scale the footprint (currently 10000 × 10000 UU). The Sewer plan used 6500 × 6500 UU; Prontera-scale is closer to 10000 × 10000 — adjust if Pryth feels cramped or sprawling.

---

## 11. NavMesh export

### 11.1 Build paths in editor

After Phase E (asset placement), Phase F (water), Phase G (lighting):

1. Open `L_Pryth` in UE5 Editor
2. Build → Build Navigation
3. Press **P** to visualize the navmesh (green overlay)
4. Verify:
   - Plaza is fully walkable (uniform green)
   - Moat is NOT walkable (hole in green where deep cells were detected)
   - Bridges ARE walkable (green stripe across the moat)
   - South residential is walkable
   - Buildings are NOT walkable (each has a navmesh hole)
   - Decorative props (barrels, crates) are NOT walkable

If the moat doesn't have a navmesh hole, retrigger AWaterArea OnConstruction by nudging a property (Section 8.2).

### 11.2 Export OBJ

Run console command in UE5 editor (` ` ` to open):

```
ExportNavMesh Pryth
```

Output: `client/server/navmesh/Pryth.obj` (path is off by one — see `sabrimmo-navmesh` skill "Known Issue: Export Output Path").

### 11.3 Copy to server

```bash
copy "C:\Sabri_MMO\client\server\navmesh\Pryth.obj" "C:\Sabri_MMO\server\navmesh\Pryth.obj"
```

### 11.4 Delete cache

```bash
del "C:\Sabri_MMO\server\navmesh\.cache\Pryth.navmesh"
```

(File may not exist on first export — that's fine.)

### 11.5 Restart server

```bash
cd C:\Sabri_MMO\server
npm run dev
```

Startup logs should show:

```
[NavMesh] Pryth loaded — N polys, M triangles
```

If the server warns "no navmesh for Pryth", the OBJ wasn't copied. Check the path in step 11.3.

### 11.6 Test in PIE

Spawn in `L_Pryth` (or warp from prontera_south). Click somewhere:

- Click on plaza → player walks
- Click in moat → click ignored OR player path-finds around the moat (good — server rejected the position)
- Click on a bridge → player walks across
- Click on a building → server rejects ("off_navmesh"), client should snap to nearest walkable spot

If the player walks INTO the moat, the navmesh OBJ wasn't rebuilt with the moat cuts. Re-export and copy.

---

## 12. Phased schedule

| Phase | Section | Time estimate | Recompile? |
|---|---|---|---|
| **A — Server registration** | 3 | 1-2 hours | No (server only) |
| **B — UE5 level scaffold** | 4 | 30 min | No |
| **C — Landscape + ground textures** | 5 | 2-4 hours (sculpting + MI tuning) | No |
| **D — Decals + grass scatter** | 5 | 1-2 hours | No |
| **E — Place reusable 3D props** | 9.1 + 10 | 4-8 hours (90 placements) | No |
| **F — North moat (AWaterArea)** | 8 | 30 min | No |
| **G — Lighting, PostProcess, NavMesh** | 6 + 11 | 1 hour | **Yes — `SabriMMOEditor`** |
| **H — Audio + NPCs + finishing** | 7 + 3.4 + 13 | 2-4 hours | **Yes — `SabriMMOEditor`** |
| **B — Bespoke Hunyuan3D pass** (deferred) | 9.2 + Appendix B | ~1 week | No (assets only) |

**Total Phase A**: 11-22 hours = ~3-5 working days for a focused build. Matches the user's "reuse-only Phase A is fast" expectation.

**Recompile required** after Phase G (PostProcess preset) and Phase H (Audio mappings). Both are CPP-only changes in `AudioSubsystem.cpp` and `PostProcessSubsystem.cpp`. Build the **editor target**:

```bash
"C:/UE_5.7/Engine/Build/BatchFiles/Build.bat" SabriMMOEditor Win64 Development "C:/Sabri_MMO/client/SabriMMO/SabriMMO.uproject" -waitmutex
```

---

## 13. Test matrix

| # | Test | Expected result |
|---|---|---|
| 1 | Login with character at Pryth saved location | Player loads `L_Pryth` directly, no redirect |
| 2 | Walk into south gate warp from inside Pryth | Loading screen → `L_PrtSouth` loads → spawn at north edge |
| 3 | Walk into north edge warp from inside L_PrtSouth | Loading screen → `L_Pryth` loads → spawn at south gate `(0, -4500, 100)` |
| 4 | Walk into west gate warp from inside Pryth | Loading screen → `L_GrassField05` loads → spawn at east edge |
| 5 | Walk into east edge warp from inside L_GrassField05 | Loading screen → `L_Pryth` loads → spawn at west gate `(-4500, 0, 100)` |
| 6 | Click Kafra Employee in central plaza | Kafra dialog opens (Save / Teleport / Storage / Cart / Cancel) |
| 7 | Kafra → Save | "Save point set!" — character's `save_map` updated to `Pryth` |
| 8 | Die in field → respawn | Respawn at Pryth save point if Pryth was last save |
| 9 | Kafra → Teleport → Prontera | Zeny deducted (1200z), zone transition to Prontera |
| 10 | Click Weapon Dealer | Shop window opens with weapons |
| 11 | Click Armor Dealer | Shop window opens with armors |
| 12 | Click Tool Dealer | Shop window opens with potions / arrows |
| 13 | Click Pryth Guide | Dialog opens with 6 facility location choices |
| 14 | Guide → "Where is the Kafra?" | Map mark appears at Kafra position, blinks 30s |
| 15 | Use Fly Wing in plaza | Random teleport ±2000 from `defaultSpawn` (within Pryth) |
| 16 | Use Butterfly Wing | Teleport to save point (may be Pryth itself = no-op or self-tp) |
| 17 | Walk to moat — try to enter water | Player path-finds around (server rejects position with `off_navmesh`) |
| 18 | Walk across center bridge | Player walks freely across to north bank, can't go further (north is the moat boundary) |
| 19 | Cast Aqua Benedicta while standing on bridge | Server rejects — player not in water |
| 20 | Cast Aqua Benedicta while standing in shallow water (if any) | Server allows — player in water |
| 21 | Open World Map (M) | World map shows Pryth at its grid position with white "you are here" dot |
| 22 | Open Minimap | Minimap shows surrounding area with NPC dots, warp portals as red dots, Kafra mark visible |
| 23 | /where command | Returns `You are in Pryth at (X, Y)` |
| 24 | Disconnect + reconnect | Reconnects to last position in Pryth |
| 25 | 2 PIE instances, both in Pryth | Both see each other, see each other walking, see each other clicking NPCs (no double-action) |
| 26 | 2 PIE instances, one in Pryth and one in prontera_south | Each sees only their zone's players |
| 27 | Walk to commercial district at night (if day/night exists) | Lampposts visible but unlit (Phase A — Phase B adds lights) |
| 28 | Performance: 50+ visible buildings + 20+ decals + grass scatter | Maintain 60 FPS at 1080p Medium |

**Acceptance**: 28/28 pass for Phase A ship.

---

## 14. Risk register

| # | Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| 1 | Coordinates don't match between server `warps[]` and UE5 `AWarpPortal` actors | High | Players get "Too far from warp" error | Use unrealMCP `find_actors_by_name("WarpPortal")` to get actual actor positions, update server registry to match. Tolerance is `radius * 2` = 500u with `radius: 250` |
| 2 | NavMesh OBJ stale after terrain changes | Medium | Players walk into moat or get stuck | Re-export OBJ + delete cache after every sculpt or asset move that changes walkability |
| 3 | Pryth zone preset overlaps with `prontera` preset | Low | Visual confusion in PIE | Test side-by-side; push Pryth WhiteTemp to 7200K if too similar |
| 4 | Audio bundle (`Prontera (5 zones)`) extension breaks if `ZoneAmbientMap.Add` is in wrong block | Low | Pryth has no ambient audio | Add inside the existing block; verify with grep after edit |
| 5 | Kafra teleport destinations include zones not yet linked from Pryth | Low | Players use Kafra to reach unreachable zones | Phase A starts with conservative destinations: Prontera, GrassField05, prontera_south. Add more in Phase B |
| 6 | AWaterArea raycast samples miss moat floor | Medium | Moat doesn't auto-detect deep, no nav cut | Sculpt floor 200+ UU below water surface; nudge AWaterArea property to retrigger OnConstruction after each sculpt change |
| 7 | Painted Landscape Grass not appearing | Medium | Town looks barren | Verify Grass Output node added MANUALLY (not via Python). Verify Layer Info created for each paint layer |
| 8 | Phase B bespoke pass collides with Phase A (assets get replaced mid-build) | Low | Phase B never started, no harm | Document Phase B in this plan but don't start until Phase A ships completely |
| 9 | Naming convention drift — someone uses `pryth` (lowercase) somewhere | Low | Server fails to find zone | Standardize on `Pryth` (PascalCase) everywhere — case-sensitive |
| 10 | Recompile of `SabriMMO` instead of `SabriMMOEditor` | Medium | Editor runs stale code | `sabrimmo-build-compile` rule: ALWAYS build `SabriMMOEditor` for PIE testing |
| 11 | The `prtsouth_to_pryth` warp added to `prontera_south.warps[]` displaces an existing warp at the same edge | Medium | Existing warps fail | Pick a fresh edge for the AWarpPortal, verify `find_actors_by_name("WarpPortal")` shows no overlap |
| 12 | Live Coding crash after CPP edit + header change | Low | Editor crashes during PIE | Close editor + full rebuild. Per `sabrimmo-build-compile`: "Header changes (.h) → CLOSE EDITOR" |
| 13 | Phase B (Hunyuan3D) ComfyUI crashes during long batch | Medium | Asset gen halts | Use `_tools/comfyui_supervisor.py` per `sabrimmo-3d-world` "ComfyUI Resilience" |
| 14 | Phase B Hunyuan3D produces cube artifacts on multi-object subjects | Medium | Bad assets in library | Use multi-input perspective generation (2 strategies × 2 seeds) per pipeline standard |

---

## Appendix A — Existing imported asset inventory

Run a fresh Glob at session start to confirm — the library grows. As of 2026-05-07 (last confirmed inventory):

### `3d art/Imported_to_UE5/Final/architecture/`

```
house_prontera_small.glb       house_prontera_medium.glb    house_prontera_tall.glb
house_prontera_wide.glb        house_prontera_corner.glb
hut_payon.glb                  hut_payon_long.glb           hut_payon_small.glb
house_morroc.glb               house_morroc_arch.glb        house_morroc_dome.glb
house_alberta_wood.glb
dock_wooden.glb                tower_geffen.glb             tower_gate.glb
wall_stone_battlement.glb      shrine_small.glb
tent_morroc.glb
```

For Pryth Phase A: use the 5 `house_prontera_*` and `wall_stone_battlement` (north wall behind moat).

### `3d art/Imported_to_UE5/Final/centerpiece/`

```
fountain_stone.glb
statue_warrior.glb
well_wooden.glb
```

For Pryth Phase A: all three (fountain in plaza, statue on plaza pedestal, well in residential).

### `3d art/Imported_to_UE5/Final/props/`

```
barrel_wooden.glb              barrel_open.glb              barrel_large.glb
crate_wooden.glb               crate_small.glb              crate_large.glb
sack_burlap.glb                sack_open.glb
basket_woven.glb               basket_apple.glb
cart_wooden.glb                cart_kafra.glb               cart_merchant.glb
market_stall.glb               stall_market_red.glb         stall_market_blue.glb
awning_striped.glb             awning_market.glb
lamppost_amatsu.glb            sign_planted.glb             banner_horizontal.glb
table_wooden.glb               bench_wooden.glb
haystack.glb
fence_wooden.glb               fence_long.glb               fence_stone.glb
```

For Pryth Phase A: most of this list.

### `3d art/Imported_to_UE5/Final/terrain/`

```
rock_small.glb                 rock_medium.glb              rock_large.glb
rock_mossy_large.glb           rock_split.glb               rocks_cluster.glb
boulder_huge.glb
bridge_wooden.glb              bridge_stone_arch.glb
stairs_wooden.glb              path_stone.glb
well_stone.glb                 fountain_basin.glb           statue_pedestal.glb
cliff_face_low.glb
```

For Pryth Phase A: `bridge_stone_arch` × 3 (over moat), rocks for north bank.

### `3d art/Imported_to_UE5/Final/special/`

```
anvil_blacksmith.glb
table_alchemy.glb
kafra_pad.glb
emperium_pedestal.glb
```

For Pryth Phase A: `kafra_pad` under Kafra NPC.

### `3d art/Imported_to_UE5/Final/vegetation/`

```
tree_oak.glb                   tree_oak_old.glb             tree_oak_dense.glb
tree_oak_yellow.glb            tree_oak_lush.glb
tree_dead.glb                  tree_dead_white.glb
tree_willow.glb                tree_bamboo.glb
tree_sakura_pink.glb           tree_sakura_full.glb
bush_round.glb                 bush_wide.glb                bush_sparse.glb
mushroom_giant.glb             mushroom_brown.glb           mushroom_blue.glb
reeds_cattails.glb
```

For Pryth Phase A: `tree_oak_lush` × 8-12 in residential edges, `bush_round` + `bush_wide` for soft ground vegetation.

---

## Appendix B — Hunyuan3D per-asset prompts (Phase B, deferred)

For the ~12 bespoke Pryth assets. Each follows the multi-input pattern (2 strategies × 2 seeds) per `sabrimmo-3d-world` "Two perspective strategies (only these two)".

### `pryth_castle_keep`

**Subject**: "stylized medieval stone keep tower with crenelated parapets, three tiers, slate roof, narrow arched windows, warm sandstone walls, viewable from outside"

**Notes**: Place at north edge behind moat. Replaces a `house_prontera_corner` in the civic district. Should read as "the seat of power" from across the moat.

### `pryth_main_gate`

**Subject**: "stylized medieval stone gatehouse, two square towers framing a wooden double-door arch, crenelated tops, warm sandstone, banner-hung from upper window"

**Notes**: Place at south gate (where the AWarpPortal is). Acts as a visual frame for the warp.

### `pryth_church`

**Subject**: "stylized small medieval stone church, single nave with steeply pitched red-tile roof, square bell tower with pointed spire, rose window over arched door, warm sandstone walls, pastoral village church"

**Notes**: Replaces a `house_prontera_corner` in civic district.

### `pryth_fountain`

**Subject**: "stylized 4-tier stone basin fountain, octagonal base, four warrior statues at corners pointing outward, ornate central spire, water cascading down tiers"

**Notes**: Replaces `fountain_stone` in central plaza.

### `pryth_statue_founder`

**Subject**: "stylized medieval king bronze statue on a stepped marble pedestal, full figure in robes and crown, holding a scepter, looking south, weathered patina"

**Notes**: Replaces `statue_warrior` next to fountain.

### `pryth_banner_flag` (3 variants — gold / red / blue)

**Subject**: "stylized medieval cloth banner with embroidered Pryth crest, hanging from a horizontal pole, deep gold cloth with darker trim and crest, slight wind sway"

**Notes**: 3 color variants (gold / red / blue) for different districts.

### `pryth_lamppost`

**Subject**: "stylized iron medieval lamppost, single vertical post topped with a glass-and-iron lantern, dragon-coiled iron base, weathered patina"

**Notes**: Replaces `lamppost_amatsu` at street corners.

### `pryth_house_a` / `pryth_house_b` / `pryth_house_c`

**Subjects**:

- **a** (small): "stylized medieval brick-and-timber cottage, single story, sloped red-tile roof, small front door, warm walls, wooden window shutters"
- **b** (medium): "stylized medieval two-story brick-and-timber house, sloped red-tile roof with chimney, balcony with hanging banner, warm walls"
- **c** (tall): "stylized medieval three-story brick-and-timber townhouse, narrow tall silhouette, sloped red-tile roof, multiple windows, warm walls"

**Notes**: Replaces `house_prontera_small/medium/tall`.

### Pipeline notes

For each asset:

1. SDXL Base → multi-input perspective generation (2 strategies × 2 seeds = 4 candidates)
2. rembg BiRefNet pre-mask
3. Validate masked input (3 checks: coverage / cut-off / multi-object)
4. Hy3D21MeshGenerator + Hy3D21VAEDecode (`mc_algo="mc"`)
5. Hy3D21MeshlibDecimate to 5K faces
6. Hy3D21MeshUVWrap + 6-camera multi-view bake + InPaint
7. **3D Coat hand-paint cohesion pass** (mandatory — without it Pryth assets lean photoreal)
8. Re-import to UE5

Per-asset time on RTX 5090: ~4-5 min generation + ~1 hour 3D Coat. Total Phase B time: ~12 assets × 1 hour = ~12 hours hand-paint + ~1 hour generation = **2-day focused effort or 1 week part-time**.

---

## Appendix C — Decal type → density formula

| Decal type | Density per district | Total |
|---|---|---|
| Dirt patches | Plaza 8 / Commercial 6 / Residential 4 = 18 | 12-18 |
| Path / cobble wear | Plaza 6 / Main streets 4 = 10 | 8-12 |
| Moss | Walls of houses 4 / Fence bases 3 / North wall 2 / Well 1 = 10 | 6-10 |
| Cracks | Plaza 3 / North wall 2 = 5 | 4-6 |
| Dark stain (wagon wheels) | Commercial 3 / Plaza 1 = 4 | 3-5 |
| Flower scatter | Residential 5 / Plaza edges 3 = 8 | 7-9 |
| **Total** | — | **40-60** |

**Per-camera-pass refinement**: After initial placement, walk the level from player POV. Note where the eye lingers (e.g., approaching the fountain, exiting Kafra dialog) and add 1-2 more decals in those sightlines. Over ~3 passes, the budget drifts up to ~60.

---

## Appendix D — Outdoor light placement guide

Pryth uses the auto-spawned outdoor lighting from `PostProcessSubsystem::SetupSceneLighting` — DirectionalLight + SkyLight + warm fog. **No additional Point Lights or Spotlights are needed in Phase A.**

If Phase B adds a day/night cycle:

- **Lampposts** become Point Light sources at night (intensity 200, radius 400 UU, warm orange `(1.0, 0.7, 0.3)`)
- **Plaza fountain** gets a Point Light from below (intensity 100, radius 200, soft cyan `(0.7, 0.85, 1.0)`) for nighttime ambient pool
- **Castle keep windows** (Phase B asset) get Spotlight + emissive material panels

For Phase A, leave all of this as decoration. The Directional + Sky + Fog combination from `SetupSceneLighting` is sufficient.

---

## Appendix E — Cross-references

| Topic | Skill | Doc |
|---|---|---|
| Zone registry, warps, level setup | `sabrimmo-zone` | `Zone_System_UE5_Setup_Guide.md` |
| Post-process zone presets | `sabrimmo-3d-world` | `RO_Classic_Visual_Style_Research.md` |
| Landscape Actor + sculpting | `sabrimmo-landscape` | `_meta/01_Landscape_Guide.md` |
| Ground textures + materials | `sabrimmo-ground-textures` | `Ground_Texture_System_Research.md` |
| Landscape Grass V3 + paint layers | `sabrimmo-environment-grass` | `_meta/03_Scatter_Objects_Guide.md` |
| DBuffer decals | `sabrimmo-material-decals` | `_meta/04_Decals_Guide.md` |
| AWaterArea mixed-mode | `sabrimmo-water` | `Water_System_Research.md`, `memory/water-system-mixed-mode.md` |
| NavMesh export, OBJ format | `sabrimmo-navmesh` | `memory/navmesh-system.md` |
| BGM + ambient mapping | `sabrimmo-audio-player` | `RO_Audio_System_Research.md` |
| Map system (minimap, world map) | `sabrimmo-map` | `Map_System_Implementation_Plan.md` |
| Click-interact NPC pipeline | `sabrimmo-click-interact` | — |
| `AMMONPC` + dialogue + shop + quest | `sabrimmo-npcs` | `RagnaCloneDocs/08_NPCs_Quests.md` |
| Kafra storage integration | `sabrimmo-storage` | — |
| Persistent socket + EventRouter | `sabrimmo-persistent-socket` | `memory/persistent-socket.md` |
| Build, recompile, Live Coding | `sabrimmo-build-compile` | — |
| Hunyuan3D pipeline (Phase B) | `sabrimmo-3d-world`, `sabrimmo-art` | `3d art/3D_World_Asset_Pipeline_2026.md` |
| Sewer Dungeon F1 build (template) | — | `Sewer_Dungeon_01_Build_Plan.md` |

---

## Glossary

| Term | Meaning |
|---|---|
| **Pryth** | Capital town zone — non-combat, Kafra hub, modeled on Prontera |
| **Phase A** | Reuse-only build — uses existing imported Hunyuan3D library, ~3-5 days |
| **Phase B** | Bespoke pass — generate ~30 Pryth-signature assets via Hunyuan3D + 3D Coat, ~1 week, deferred |
| **Moat** | The single AWaterArea at the north edge of Pryth — auto-detected deep cells, navmesh-cut, outdoor blue palette |
| **AWaterArea** | C++ actor for water (visual + nav cut + skill gating). Mixed-mode auto-detects per-cell deep via raycast grid |
| **AMMONPC** | Generic data-driven NPC actor. Single class for Kafra, shops, quest givers, guides — `NpcId` matches server registry key |
| **MI_RODecal_*** | Decal Material Instances using ONLY RO original textures (NOT AI-generated). 91 total in `RO_Decals/` |
| **GT_V3_*** | Landscape Grass Type assets. `GT_V3_Grassland` is the pastoral town variant |
| **WarmthTint** | Material parameter on `M_Landscape_RO_17` — multiplies terrain color toward warm or cool |
| **Painted layer** | A grass placement method where grass appears only where you paint via `LandscapeLayerWeight × slope_clamp` (vs. random uniform via `slope Clamp` direct connection) |
| **DefaultPawnClass** | Must be `None` on `GM_MMOGameMode`. Setting it to `BP_MMOCharacter` causes a duplicate pawn at world origin |
| **Level Blueprint** | The per-level Blueprint that spawns and possesses `BP_MMOCharacter`. Easiest to forget, hardest bug to diagnose |
| **`MaxReconnectionAttempts = 0`** | Means infinite reconnects (NOT zero attempts). Set in `MMOGameInstance::ConnectSocket()` |

---

**Last updated**: 2026-05-08 — Build plan landed.
**Status**: Phase A pending. Phase B deferred.
**Next action**: Walk through Phase A — Server registration (Section 3).
