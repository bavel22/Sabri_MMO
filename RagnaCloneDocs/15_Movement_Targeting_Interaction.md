# 15 -- Movement, Targeting, Interaction & Multiplayer Sync

> Ragnarok Online Classic (Pre-Renewal) comprehensive reference for all movement, targeting,
> interaction, pathfinding, name tag, and multiplayer synchronization systems.
> Sources: rAthena source (path.cpp, unit.cpp, clif.cpp), iRO Wiki Classic, Ragnarok Research Lab,
> OpenKore packet tables, rAthena forums, WarpPortal forums, RateMyServer, Sabri_MMO RagnaCloneDocs.

---

## Table of Contents

1. [Character Movement](#1-character-movement)
2. [Other Player Movement (Multiplayer Sync)](#2-other-player-movement-multiplayer-sync)
3. [Enemy Movement](#3-enemy-movement)
4. [Click-to-Move System](#4-click-to-move-system)
5. [Targeting System](#5-targeting-system)
6. [Auto-Attacking](#6-auto-attacking)
7. [Range Checking](#7-range-checking)
8. [Hover Detection & Cursor System](#8-hover-detection--cursor-system)
9. [Click to Attack / Stop](#9-click-to-attack--stop)
10. [Click to Interact (NPCs)](#10-click-to-interact-npcs)
11. [Attack Animations](#11-attack-animations)
12. [Pathfinding (Tile-Based, Not NavMesh)](#12-pathfinding-tile-based-not-navmesh)
13. [Name Tags](#13-name-tags)
14. [Player/Enemy Multiplayer Sync (Packets)](#14-playerenemy-multiplayer-sync-packets)

---

## 1. Character Movement

### 1.1 Grid-Based World

RO Classic uses an **isometric 2D tile grid** overlaid on 3D terrain. Every map is laid out in an X,Y coordinate system where each cell has a distinct integer (X,Y) position. Movement is cell-to-cell, not free-form.

**Cell Properties:**
- Each cell has walkability flags stored in the map's GAT (Ground Altitude Table) file
- Cells can be: walkable, non-walkable (walls/obstacles), water, shootable-through-but-not-walkable
- The server checks `CELL_CHKNOPASS` to verify walkability before allowing movement
- Some cells are "snipeable" (ranged attacks can pass through but characters cannot walk on them)

**Cell Size (RO vs UE5 mapping):**
- 1 RO cell = 50 UE units in Sabri_MMO's implementation
- RO maps range from ~100x100 to ~400x400 cells
- Prontera town map: 312 x 392 cells

### 1.2 Movement Speed

Movement speed in RO is measured as **milliseconds per cell** -- the time to traverse one adjacent tile.

**Base Speed:**
- All player characters (regardless of class or stats): **150 ms per cell** (~6.67 cells/second)
- **AGI does NOT affect movement speed** -- AGI only affects ASPD (attack speed) and Flee
- Movement speed is ONLY modified by specific skills, items, and status effects

**Speed Modifiers (ms per cell):**

| Source | Speed (ms/cell) | Effective Multiplier | Notes |
|--------|----------------|---------------------|-------|
| No modifier (base) | 150 | 1.0x | All classes |
| Increase AGI (skill) | 110 | ~1.36x faster | Does not stack with Peco ride |
| Peco Peco Ride (Knight) | 110 | ~1.36x faster | Does not stack with Increase AGI |
| Increase AGI + Peco Ride | 70 | ~2.14x faster | These two DO stack with each other |
| Speed Potion | 60 | ~2.5x faster | Does NOT stack with ANY other speed modifier. Cancels speed-reducing effects. Lasts 5 seconds |
| Pushcart Lv1 penalty | 225 | 0.67x slower | -50% speed. Improves with skill level |
| Pushcart Lv5 penalty | 157 | 0.95x slower | -5% speed |
| Curse status | 225 | 0.67x slower | Movement slowed to 2/3 |
| Quagmire | ~250 | 0.60x slower | Movement significantly slowed |
| Decrease AGI (debuff) | ~195 | 0.77x slower | Reduces movement speed |

**Key Rules:**
- Speed Potions override ALL other speed effects (both bonuses and penalties)
- Increase AGI and Peco Peco Ride stack additively
- Most other movement speed bonuses do NOT stack -- only the strongest applies
- Equipment that gives "movement speed +X%" (e.g., Moonlight Flower Card) acts like Increase AGI and does not stack with it
- **Weight does NOT directly affect movement speed** in pre-renewal (only blocks attacks/skills at 90%+)

### 1.3 Walking vs Running

**There is no separate "run" state in RO Classic.** Characters have only one movement speed. The visual movement animation plays at the same rate regardless of speed modifiers -- the character simply covers more ground per animation cycle when speed-buffed.

**Diagonal Movement:**
- Characters CAN move diagonally (8-directional movement on the grid)
- Diagonal movement costs the same "distance" as cardinal movement in terms of cell counting (Chebyshev distance)
- However, diagonal movement takes **longer in real time** because the physical distance is ~1.41x longer
- Moving 5 cells diagonally takes ~1,123 ms vs 5 cells straight takes ~827 ms (at 150ms/cell base)

### 1.4 Movement Interaction with Skills/Attacks

**Movement cancels:**
- Moving while casting a skill **interrupts the cast** (if movement exceeds a small threshold)
- Auto-attack is interrupted by movement -- clicking the ground stops attacking
- Some skills have a "walk delay" that prevents movement for a short time after use

**Movement blocks:**
- Stun, Freeze, Stone Curse, Deep Sleep: character cannot move
- Ankle Snare (Hunter trap): rooted in place
- Spider Web: rooted
- Weight >= 90%: movement still allowed, but attacks/skills blocked
- Weight >= 100%: cannot pick up items (movement still works)

**Sitting:**
- `Insert` key or `/sit` command toggles sitting
- Sitting doubles natural HP/SP regeneration
- Cannot move, attack, or use skills while sitting
- Must stand up first (another Insert press or click to move)

---

## 2. Other Player Movement (Multiplayer Sync)

### 2.1 Server-Authoritative Position

RO uses a **server-authoritative movement model**. All movement is validated server-side:

1. Client sends a walk request packet (`CZ_REQUEST_MOVE` / packet 0x0437) containing destination (x, y)
2. Server runs `path_search()` to validate the path from current position to destination
3. If valid, server stores the walkpath and begins moving the unit along it tick-by-tick
4. Server sends `ZC_NOTIFY_PLAYERMOVE` (packet 0x0087, 12 bytes) back to the requesting client confirming the walk
5. Server sends `ZC_NOTIFY_MOVE` (packet 0x0086) to ALL OTHER clients in the area, containing: unit ID, start position, destination position, and walk start time

### 2.2 Client-Side Rendering of Remote Players

When the client receives a `ZC_NOTIFY_MOVE` packet for another player:
1. The client knows the remote player's **start position** and **destination position**
2. The client also knows the entity's **movement speed** (received in the spawn/appearance packet)
3. The client locally computes the path between start and destination
4. The client animates the remote player walking along that path at the appropriate speed
5. **No interpolation in the traditional FPS sense** -- the client simply plays the walk animation from A to B at the known speed

**This is NOT interpolation/extrapolation like in FPS games.** It is **path-based animation**: the server tells clients "entity X is walking from (x1,y1) to (x2,y2) at speed S", and the client animates accordingly.

### 2.3 Position Lag (Nagle Algorithm Issue)

RO Classic suffers from a well-known "position lag" problem:
- The game uses TCP (not UDP) for all communication
- Windows TCP implements the **Nagle Algorithm** by default, which buffers small packets and sends them in batches
- This delays the ACK of TCP transmissions by up to **200ms**
- The RO server does NOT send new movement/position data until the previous transmission is ACKed
- Result: other players and monsters appear 100-200ms behind their actual server position
- **Fix**: disabling Nagle algorithm (TCP_NODELAY) in the Windows registry reduces this lag significantly

### 2.4 What Happens During Lag/Packet Loss

- Since RO uses TCP, **packets cannot be lost** -- they are retransmitted automatically
- However, retransmission adds latency, causing remote entities to "freeze" momentarily
- When the delayed packets finally arrive, the remote entity "warps" to their new position
- The client does NOT do prediction or dead reckoning -- it only renders what the server tells it
- If a player disconnects mid-walk, the server stops their movement at their last validated position

### 2.5 Position Update Frequency

- The server does NOT send periodic position updates at a fixed rate
- Instead, it sends a move packet **once per walk command** (when an entity starts walking to a new destination)
- For entities that stop moving, no further packets are sent
- For continuously moving entities (e.g., chasing player), a new walk packet is sent each time the path is recalculated
- The effective update rate depends on entity movement speed and path changes, not a fixed tick

---

## 3. Enemy Movement

### 3.1 Monster Speed Values

Monster movement speed is stored in the mob database as `walkSpeed` (milliseconds per cell), same unit as player speed:

| Monster | walkSpeed (ms/cell) | Effective Speed |
|---------|--------------------|--------------------|
| Poring | 400 | Very slow (2.5 cells/sec) |
| Red Plant | 2000 | Immobile (plant) |
| Pupa | 1000 | Immobile (egg) |
| Lunatic | 200 | Normal (5 cells/sec) |
| Zombie | 400 | Slow (2.5 cells/sec) |
| Archer Skeleton | 300 | Moderate (3.3 cells/sec) |
| Hunter Fly | 150 | Fast (6.67 cells/sec, same as players) |
| Condor | 150 | Fast |
| Osiris (MVP) | 100 | Very fast (10 cells/sec) |

### 3.2 Wander Movement (IDLE State)

When a monster has no target and is in IDLE state:

1. **Pause**: Wait 3-8 seconds (random within range) between wanders
2. **Pick Target**: Choose a random walkable cell within 3-5 cells of current position
3. **Clamp**: Ensure target is within `wanderRadius` of the original spawn point (prevents infinite drift)
4. **Walk Speed**: Move at **60% of normal chase speed** -- wander is visually slower/more relaxed
5. **Arrive**: When within ~0.2 cells of target, stop and begin new pause timer
6. **Disabled For**: Plants (`MD_CANMOVE` not set), mobs with `MD_NORANDOMWALK` flag, slave mobs (AI code 24)

**In Sabri_MMO Implementation:**
- Wander speed: `moveSpeed * 0.6`
- Wander radius: 100-300 UE units of current position
- Clamped to `wanderRadius` from spawn coordinates

### 3.3 Chase Movement (CHASE State)

When a monster is chasing a target:

1. **Full Speed**: Move at 100% of `walkSpeed`-derived speed
2. **Direction**: Move directly toward target's current position each AI tick
3. **AI Tick**: Enemy AI runs at **200ms intervals** (5 ticks/second)
4. **Step Size**: `moveSpeed * (tickInterval / 1000)` UE units per tick
5. **Path Update**: Recalculate direction to target every tick (simple chase, not A* pathfinding for mobs in Sabri_MMO)
6. **Give Up Distance**: If monster travels beyond `chaseRange + 200` UE units from its aggro origin, it gives up and returns to IDLE
7. **Chase Range**: Default 600 UE units (12 RO cells). Bosses: 800-1200 UE units
8. **Hit Stun Pause**: After taking damage, movement pauses for `damageMotion` ms (but state checks continue)

### 3.4 Monster Speed Formula (Sabri_MMO)

```
moveSpeed_UE_per_sec = (50 / walkSpeed) * 1000
```

| walkSpeed | moveSpeed (UE/sec) | Cells/sec |
|-----------|--------------------|-----------|
| 400 | 125 | 2.5 |
| 300 | 167 | 3.3 |
| 200 | 250 | 5.0 |
| 150 | 333 | 6.67 |
| 100 | 500 | 10.0 |

### 3.5 Return-to-Spawn Behavior

When a monster gives up chase:
- Returns to IDLE state at current position (does NOT teleport back to spawn)
- Wanders back toward spawn point over time via the wander radius clamp
- Each wander step clamps destination to be within `wanderRadius` of original spawn
- Over multiple wander cycles, the monster naturally drifts back toward its spawn area

---

## 4. Click-to-Move System

### 4.1 How Click-to-Move Works (Step by Step)

1. **Player left-clicks** on the ground at a destination cell
2. **Client** determines the destination cell coordinates (X, Y) from the click position using isometric projection math
3. **Client** sends `CZ_REQUEST_MOVE` packet (0x0437) to the server with destination (X, Y)
4. **Server** receives the request and calls `unit_walktobl()` or `unit_walktoxy()`
5. **Server** calls `path_search()` which uses the **A* algorithm** to find a valid path from current position to destination
6. **Path validation**: checks each cell along the path for `CELL_CHKNOPASS` (walkability)
7. If path is valid: server stores the walkpath (array of direction values, max `MAX_WALKPATH` = 32 steps)
8. Server begins moving the unit along the path, one cell per `walkSpeed` ms
9. **Server** sends `ZC_NOTIFY_PLAYERMOVE` to the moving client (your own walk confirmation)
10. **Server** sends `ZC_NOTIFY_MOVE` to all other clients in view range (so they see you walk)
11. **Client** begins the walk animation toward the destination
12. If destination is unreachable (no valid path), the server ignores the request

### 4.2 Continuous Movement (Hold Click)

- Holding the left mouse button causes **continuous movement**: the client sends repeated walk requests toward the cursor position
- The cursor position is constantly re-evaluated, so the character follows the mouse
- This creates smooth "drag to move" behavior
- New walk requests are sent when the character reaches the current destination or changes direction

### 4.3 Path Limitations

- `MAX_WALKPATH = 32` cells -- paths longer than 32 cells are truncated
- For very long distances, the client must send multiple walk requests as the character progresses
- Paths cannot cross non-walkable cells
- Paths can include diagonal movement (8 directions)
- The server re-validates each step as the character walks (checking for newly spawned obstacles)

### 4.4 Ground Cell Highlighting

- Walkable cells under the cursor are **highlighted** (the ground cursor, toggled with Alt+Home)
- Non-walkable cells show **red highlighting** or no highlight
- This gives the player visual feedback about where they can walk
- The ground cursor is a small translucent grid square that follows the mouse

---

## 5. Targeting System

### 5.1 Click-to-Target (No Tab-Targeting in Classic)

RO Classic uses a **purely click-based targeting system**.

**Target Acquisition Methods:**
- **Left-click on monster**: Begin auto-attacking that monster
- **Left-click on NPC**: Open dialog window
- **Left-click on ground**: Walk to that position
- **Left-click on ground item**: Walk to and pick up that item
- **Right-click on player**: Open context menu (Whisper, Trade, Party Invite, Block)
- **Skill hotkey then left-click**: Cast selected skill on target

**There is no traditional tab-targeting** in RO Classic (pre-renewal). There is no keyboard shortcut to cycle through nearby enemies. Some private servers and later official patches added limited "nearest target" functionality, but classic RO relies entirely on mouse-click targeting.

### 5.2 Target Priority (Overlapping Entities)

When multiple entities overlap on the screen, clicks resolve in this priority order:
1. **NPC** (highest priority)
2. **Player characters**
3. **Monsters**
4. **Ground items**
5. **Ground** (movement, lowest priority)

### 5.3 Target Locking

RO Classic does NOT have a "target lock" system in the traditional MMO sense:
- Clicking an enemy starts auto-attack, which continues until you click elsewhere or the target dies
- There is no visual "selected target" indicator (no bracket around the enemy, no target frame)
- The only indication of your current target is that your character is walking toward / attacking it
- Some private servers added target lock features, but official pre-renewal did not have this

### 5.4 Skill Targeting Modes

**Single-Target Offensive Skills:**
1. Press skill hotkey (F1-F9)
2. Cursor changes to **crosshair/targeting cursor**
3. Left-click on a valid target (monster or player in PvP)
4. If target is out of range: character walks toward target, then casts when in range
5. Right-click or Escape cancels targeting mode

**Single-Target Support Skills:**
1. Press skill hotkey
2. Cursor changes to targeting cursor
3. By default, clicking a player opens whisper window instead of casting
4. Hold **Shift** and click to force-cast on the player
5. `/noshift` (or `/ns`) removes the Shift requirement permanently

**Ground-Target Skills (AoE):**
1. Press skill hotkey
2. A colored targeting grid appears on the ground, following the cursor
3. Grid shows the AoE area (3x3, 5x5, 7x7 cells etc.)
4. Color: red for offensive, green for supportive, blue for utility
5. Left-click to cast at that position
6. Right-click or Escape cancels
7. The targeting grid texture is `magic_target.tga` (player-customizable)

**Self-Cast Skills:**
- Execute immediately on hotkey press, no targeting needed
- Examples: Increase AGI (self), Endure, Two-Hand Quicken

### 5.5 Walk-to-Target (Auto-Approach)

When a player attempts to attack or cast a skill on a target that is **out of range**:
- The character automatically **walks toward the target** until within range
- Once in range, the attack or skill executes
- This is handled server-side: the server queues the action and processes it when the character arrives in range
- If the target moves during approach, the character adjusts its path to follow

---

## 6. Auto-Attacking

### 6.1 How Auto-Attack Starts

1. **Click on Enemy**: Left-click on a monster starts the auto-attack sequence
2. Character walks toward the target if out of attack range
3. Once in range, character faces the target and begins the attack loop
4. Each attack deals damage based on the ATK formula

### 6.2 The `/noctrl` Command

By default, attacking a non-aggressive monster requires holding **Ctrl+Click**:
- Without `/noctrl`: single left-click on a passive monster does nothing (or walks near it). Must Ctrl+Click to attack passive mobs. Aggressive mobs can be attacked with plain left-click.
- With `/noctrl` enabled (`/nc`): single left-click attacks ANY monster, aggressive or passive
- Most players enable `/noctrl` immediately and never turn it off

### 6.3 Continuous Attack Loop

Once auto-attack is engaged:

1. Character checks if target is alive and in range
2. If in range: perform one attack (deal damage), play attack animation
3. Wait for `attackDelay` ms (determined by ASPD)
4. Repeat from step 1
5. Continue until:
   - Target dies (character stops, stands idle)
   - Player clicks on ground (walk command, cancels attack)
   - Player uses a skill (skill takes priority, then attack may resume)
   - Player clicks a different target (switch target)
   - Target moves out of range (character chases if attack was initiated)

**Attack Delay Formula:**
```
attackDelay_ms = (200 - ASPD) * 10
```

| ASPD | Delay (ms) | Hits/sec |
|------|-----------|----------|
| 150 | 500 | 2.0 |
| 160 | 400 | 2.5 |
| 170 | 300 | 3.33 |
| 180 | 200 | 5.0 |
| 185 | 150 | 6.67 |
| 190 | 100 | 10.0 (cap) |

### 6.4 ASPD Formula (Pre-Renewal)

```
ASPD = 200 - (WD - floor((WD * AGI / 25) + (WD * DEX / 100)) / 10) * (1 - SM)
```

Where:
- `WD` = Weapon Delay = `50 * BTBA` (Base Time Between Attacks, class+weapon specific)
- `SM` = Speed Modifier (sum of all ASPD% bonuses)
- ASPD is capped at 190

**Dual Wield (Assassin):**
```
BTBA_dual = 0.7 * (BTBA_right + BTBA_left)
```

**Speed Modifier Sources (additive, only strongest potion applies):**

| Source | SM Value |
|--------|----------|
| Concentration Potion | +0.10 |
| Awakening Potion | +0.15 |
| Berserk Potion | +0.20 |
| Poison Bottle (Assassin Cross) | +0.25 |
| Two-Hand Quicken | +0.30 |
| Adrenaline Rush | +0.25-0.30 |
| Spear Quicken | +0.21-0.30 |
| Frenzy (Lord Knight) | +0.30 (sets ASPD to 190) |
| Doppelganger Card | +0.10 |
| Peco Peco Ride | -0.50 (penalty) |

### 6.5 When Auto-Attack Stops

Auto-attack stops when:
- Target dies (character returns to idle)
- Player clicks on ground (movement command overrides attack)
- Player clicks on a different monster (switches target)
- Player presses a skill hotkey (enters skill targeting mode)
- Target leaves the map (zone change, teleport, death)
- Player is stunned, frozen, or otherwise CC'd
- Player weight >= 90% (attack blocked)
- Player sits down (Insert key)

### 6.6 Target Chase During Auto-Attack

If the target moves out of attack range while auto-attacking:
- Character automatically chases the target
- Character walks toward target until back in range
- Auto-attack resumes when in range again
- This chase is persistent -- character follows until target dies, player clicks elsewhere, or range limit exceeded

---

## 7. Range Checking

### 7.1 Distance Measurement: Chebyshev Distance

RO uses **Chebyshev distance** (also called "chessboard distance") for all range calculations:

```
distance(A, B) = max(abs(A.x - B.x), abs(A.y - B.y))
```

This means:
- Moving 5 cells east = distance 5
- Moving 5 cells diagonally (5 east, 5 north) = distance 5 (NOT 7!)
- A diagonal counts the same as a straight line for range purposes
- This creates **square-shaped** range areas, not circular

**Example:**
```
Entity at (10, 10), range check of 3 cells:
Covers all cells where max(|x-10|, |y-10|) <= 3
= (7,7) to (13,13) = a 7x7 square centered on (10,10)
```

### 7.2 Melee vs Ranged Classification

The game classifies attacks based on the **distance at time of hit**, not weapon type:

| Distance (cells) | Classification | Effects |
|-------------------|---------------|---------|
| 0-3 cells | **Short Range / Melee** | Blocked by Safety Wall. Not blocked by Pneuma. |
| 4+ cells | **Long Range / Ranged** | Blocked by Pneuma. Not blocked by Safety Wall. |

**Important:** A bow attack from 3 cells away is classified as MELEE. A whip attack from 4 cells away is classified as RANGED. The weapon does not determine the classification -- only the distance does.

### 7.3 Weapon Attack Ranges

| Weapon Type | Default Range (cells) |
|-------------|----------------------|
| Bare Fist | 1 |
| Dagger | 1 |
| One-Handed Sword | 1 |
| Two-Handed Sword | 1 |
| One-Handed Spear | 2 (melee, can hit at 2 cells) |
| Two-Handed Spear | 3 (still melee at 3) |
| One-Handed Axe | 1 |
| Two-Handed Axe | 1 |
| Mace | 1 |
| Knuckle/Claw | 1 |
| Katar | 1 |
| Staff/Rod | 1 |
| Book | 1 |
| Bow | 5 (ranged, > 3 cells = long range) |
| Musical Instrument | 2 |
| Whip | 3 |

**Range Modifiers:**
- Some weapons have innate range bonuses (e.g., Long Mace: +2 range)
- Snake Head Hat: +1 range
- "Dance" technique: +1 effective attack cell (moving during attack animation to extend reach)

### 7.4 Skill Ranges

Each skill has its own defined range (independent of weapon range):

| Skill | Range (cells) |
|-------|---------------|
| Bash | Melee (weapon range) |
| Magnum Break | 3x3 AoE around self |
| Bowling Bash | Melee + knockback |
| Fire Bolt | 9 |
| Cold Bolt | 9 |
| Lightning Bolt | 9 |
| Jupitel Thunder | 9 |
| Storm Gust | 9 (ground target) |
| Lord of Vermillion | 9 (ground target) |
| Heal | 9 |
| Blessing | 9 |
| Double Strafe | Weapon range (bow = 5) |
| Arrow Shower | 9 (ground target) |
| Sonic Blow | 1 (melee) |
| Backstab | 1 (melee, must be behind target) |
| Grimtooth | 7 (ranged from Hide) |
| Mammonite | Melee (weapon range) |

### 7.5 Monster Attack Ranges

Monsters have their own `attackRange` defined in the mob database:
- Most melee mobs: 1 cell
- Archer Skeleton, Orc Archer: 9 cells (ranged)
- Hydra: 1 cell (melee, immobile turret)
- Mandragora, Flora, Geographer: 4 cells (immobile + ranged)

### 7.6 Aggro Range

Aggressive monsters scan for players within `aggroRange` (defined per monster):
- Typical passive mob: 0 (no aggro scan)
- Typical aggressive mob: 8-12 cells
- Boss/MVP: 15-20 cells
- Cast Sensor mobs: detect casting within 8-10 cells

---

## 8. Hover Detection & Cursor System

### 8.1 Cursor Types

The RO client changes the mouse cursor based on what entity is under it:

| Cursor Type | Visual | When Shown |
|-------------|--------|------------|
| **Normal Arrow** | Standard pointer | Default state, hovering over nothing interactable |
| **Move Cursor** | Directional arrow / footstep icon | When clicking ground to walk (during click, not hover) |
| **Attack Cursor (Sword)** | Sword / crossed swords icon | Hovering over a hostile monster |
| **Talk Cursor (Speech Bubble)** | Speech bubble icon | Hovering over an NPC |
| **Pickup Cursor (Hand)** | Open hand / grab icon | Hovering over a ground item |
| **Cast/Targeting Cursor** | Crosshair | When in skill targeting mode |
| **Camera Rotate** | Curved double arrows (left/right) | When holding right mouse button to rotate camera |
| **Resize** | Vertical double arrows | When hovering over chat window resize edge |
| **Busy/Hourglass** | Hourglass | During loading or processing |

### 8.2 Entity Identification on Hover

**Monsters:**
- Cursor changes to **sword** when hovering over a monster
- Monster name appears above the monster sprite
- If `/monsterhp` is enabled, an HP bar appears under the monster name
- No tooltip or detailed info on hover (must use external databases or wiki)

**NPCs:**
- Cursor changes to **speech bubble** when hovering over an NPC
- NPC name is always visible above the NPC sprite (white text)
- Some NPCs have additional markers (quest icons: `!` for new quest, `?` for turn-in)

**Other Players:**
- Cursor remains the **normal arrow** when hovering over other players
- Player information (name, guild, party) is always visible above their sprite
- Right-click opens context menu (does not change cursor on hover)

**Ground Items:**
- Cursor changes to **hand/grab** icon when hovering over a dropped item
- Item name appears above the item on the ground

### 8.3 Entity Highlighting

- RO Classic does NOT highlight entity sprites on hover (no glow, no outline)
- The cursor change is the sole visual indicator of what will be interacted with
- Entity names are always displayed, not just on hover
- Some custom/private clients added sprite highlighting, but official clients do not

---

## 9. Click to Attack / Stop

### 9.1 Starting an Attack

**Default behavior (without `/noctrl`):**
- Left-click on aggressive monster: walk toward it, auto-attack begins when in range
- Left-click on passive monster: walk near it (does NOT attack)
- Ctrl + Left-click on any monster: walk toward it, auto-attack begins when in range
- Ctrl + Hold Left-click: continuous attack (character keeps attacking)

**With `/noctrl` enabled:**
- Left-click on ANY monster: walk toward it, auto-attack begins when in range
- No need for Ctrl modifier
- This is the universally preferred mode

### 9.2 Stopping an Attack

- **Click on ground**: Walk command cancels attack state. Character stops attacking and walks to clicked position
- **Click on a different monster**: Switches attack target to the new monster
- **Press Escape**: Does NOT stop attacking (closes UI windows only)
- **Use a skill**: Temporarily interrupts auto-attack to execute the skill. After skill execution, auto-attack may or may not resume depending on the skill
- **Sit down (Insert)**: Stops attacking, character sits
- **There is no explicit "stop attack" button** in RO Classic -- you stop attacking by giving a different command (walk, sit, etc.)

### 9.3 Attack State Persistence

- Once auto-attack is engaged, it persists until explicitly interrupted
- Character continues attacking the same target indefinitely
- If target moves out of range, character chases and resumes attacking
- If target dies while `/noctrl` is on, character stops (does NOT auto-target the next enemy)
- Character faces the target during the entire attack loop

---

## 10. Click to Interact (NPCs)

### 10.1 NPC Interaction Mechanics

1. **Click on NPC**: Left-click on an NPC sprite to interact
2. **Walk to NPC**: If the NPC is more than ~2-3 cells away, the character walks toward the NPC first
3. **Interaction Range**: Must be within ~2 cells of the NPC to open dialogue (exact value varies, typically 2-4 cells depending on NPC type)
4. **Dialog Opens**: An NPC dialog window opens (modal -- blocks other interactions)
5. **Movement Locked**: Character cannot move while NPC dialog is open
6. **WARNING**: If the player is actively walking when NPC dialogue triggers, it can cause a disconnect in some client versions

### 10.2 Walk-to-Interact

When an NPC is clicked from far away:
1. Client sends a walk request toward the NPC's position
2. Character walks toward the NPC
3. Upon reaching interaction range, the interaction triggers automatically
4. This is called "walk-to-interact" or "action queuing" -- the click queues both the walk and the interact

### 10.3 NPC Interaction Types

| NPC Type | Interaction | Range |
|----------|-------------|-------|
| Dialog NPC | Opens conversation window | ~2-4 cells |
| Shop NPC | Opens buy/sell window | ~2-4 cells |
| Kafra Employee | Opens service menu (save, storage, teleport) | ~2-4 cells |
| Job Change NPC | Opens job change dialog | ~2-4 cells |
| Warp NPC | Teleports player to destination | 1 cell (overlap trigger in Sabri_MMO) |
| Quest NPC | Opens quest dialog | ~2-4 cells |
| Refiner NPC | Opens refining interface | ~2-4 cells |

### 10.4 NPC Dialog System

- NPC dialogs use a **modal window** that pauses normal gameplay
- Dialog has "Next" and "Close" buttons
- Some dialogs have multiple-choice selections (numbered list)
- Some dialogs have text input fields (e.g., naming a party, entering an amount)
- Pressing Escape or clicking "Close" ends the dialog
- After dialog closes, character can resume normal activity

---

## 11. Attack Animations

### 11.1 Facing Direction

- When attacking, the character sprite automatically **faces toward the target**
- RO sprites have **8 facing directions** (N, NE, E, SE, S, SW, W, NW)
- The direction is determined by the relative position of attacker to target
- Direction updates each time an attack action occurs
- Idle characters face the direction of their last action

### 11.2 Attack Animation Timing

The attack animation is tied to ASPD:

```
animationDuration_ms = attackMotion (from mob/player data)
```

For players:
- The attack animation plays once per attack
- Animation speed is **NOT** directly scaled by ASPD in the original client
- Instead, the animation plays at a fixed duration, and the delay between attacks is what changes with ASPD
- At very high ASPD, the animation may not fully complete before the next attack begins (causing the "machine gun" visual effect)

For monsters:
- `attackMotion` field defines animation duration in ms (e.g., Poring: 672ms, Baphomet: 768ms)
- `attackDelay` defines the time between attacks (e.g., Poring: 1872ms, Baphomet: 768ms)
- `damageMotion` defines the hit-stun animation duration (e.g., Poring: 480ms)

### 11.3 Attack Motion vs Attack Delay

```
attackDelay: Total time between attacks (includes animation + wait)
attackMotion: Duration of the attack animation itself
damageMotion: Duration of hit-stun animation when taking damage

Timeline of one attack cycle:
|--attackMotion--|----------wait----------|
|<-----------attackDelay---------------->|

Damage is dealt at a specific frame within attackMotion (usually ~50-75% through)
```

### 11.4 Dual Wield Attack Animation (Assassin)

When dual wielding (Assassin/Assassin Cross):
- Both hands hit **per attack cycle** (not alternating)
- Right hand attacks first, left hand follows within the same cycle
- Attack animation may show a double-swing visual
- Each hand has independent damage calculation
- The combined ASPD uses: `BTBA = 0.7 * (BTBA_right + BTBA_left)`
- ASPD cap for dual wield: 190 (same as single wield in Sabri_MMO, 190 vs 195 officially debated)

### 11.5 Ranged Attack Animations

For ranged weapons (bows):
- Attack animation shows the character drawing and releasing the bow
- A projectile sprite (arrow) travels from the character to the target
- Damage is dealt when the projectile reaches the target (slight delay)
- Arrow type affects the visual (fire arrow = flame trail, etc.)

---

## 12. Pathfinding (Tile-Based, Not NavMesh)

### 12.1 Overview: RO Does NOT Use NavMesh

RO uses **tile-based A* pathfinding** on a 2D grid, NOT a navigation mesh. Every map is a grid of cells with walkability flags. The pathfinding algorithm operates on this 2D integer grid.

### 12.2 The A* Algorithm in rAthena (path.cpp)

rAthena's `path_search()` function implements A* with the following characteristics:

**Algorithm:**
1. Start from source cell (sx, sy), goal is destination cell (dx, dy)
2. Use a **binary heap (BHEAP)** as the open set for efficient node retrieval
3. **Heuristic**: Uses a modified heuristic based on coordinate differences (similar to Manhattan distance but adapted for 8-directional movement)
4. **Movement**: 8-directional (N, NE, E, SE, S, SW, W, NW)
5. **Walkability Check**: Each candidate cell is checked with `CELL_CHKNOPASS`
6. **Diagonal Restriction**: Diagonal movement is blocked if EITHER of the two adjacent cardinal cells is non-walkable (prevents cutting corners through walls)
7. Reconstruct path as an array of direction values when goal is reached

**Constants:**
```
MAX_WALKPATH = 32    // Maximum path length (32 cells/steps)
8 directions:
  0 = South
  1 = Southwest
  2 = West
  3 = Northwest
  4 = North
  5 = Northeast
  6 = East
  7 = Southeast
```

**Walkpath Data Structure:**
```c
struct walkpath_data {
    unsigned short path_len;    // Total number of steps
    unsigned short path_pos;    // Current position in path
    unsigned char path[MAX_WALKPATH];  // Array of direction values (0-7)
};
```

### 12.3 Shootpath vs Walkpath

rAthena has two pathfinding modes:

**Walkpath (`path_search` with `walkpath_data`):**
- Used for character/monster movement
- Respects all walkability flags
- Prevents diagonal corner-cutting
- Limited to `MAX_WALKPATH` = 32 steps
- Uses A* algorithm

**Shootpath (`path_search_long` with `shootpath_data`):**
- Used for ranged attacks and line-of-sight checks
- Checks if a straight line from A to B is unobstructed
- Uses Bresenham's line algorithm (not A*)
- Only checks if cells along the line are shootable (different flag than walkable)
- Used to determine if ranged attacks/skills can hit a target

### 12.4 Corner-Cutting Prevention

Diagonal movement is restricted to prevent characters from walking through wall corners:

```
To move diagonally from (x,y) to (x+1,y+1):
- Cell (x+1, y) must be walkable  AND
- Cell (x, y+1) must be walkable

If either adjacent cardinal cell is blocked, diagonal movement is denied.
```

This prevents the classic "wall clip" exploit where characters could squeeze through diagonal wall gaps.

### 12.5 Path Length Limitation

With `MAX_WALKPATH = 32`:
- Characters can plan paths up to 32 cells long in a single request
- For destinations further away, the client must send multiple sequential walk requests
- The client typically sends a new walk request as the character approaches the end of the current path
- This is invisible to the player -- movement appears seamless

### 12.6 Sabri_MMO Adaptation Notes

Since Sabri_MMO uses UE5 with a 3D world (not a 2D tile grid):
- The project does NOT use tile-based A* pathfinding
- UE5's built-in navigation system (NavMesh) or simple direct-line movement is used instead
- Range checking uses UE unit distances (1 RO cell = 50 UE units)
- The server uses distance calculations rather than cell-grid pathfinding
- Monster chase uses direct movement toward target position each tick

---

## 13. Name Tags

### 13.1 Player Name Tags

Player characters display identifying information above their sprite:

**Display Elements (top to bottom, default `/font` setting):**
1. **Guild Emblem** -- 24x24 pixel icon (if in a guild)
2. **Guild Name** -- in brackets, e.g., `[GuildName]` (if in a guild)
3. **Character Name** -- the character's display name
4. **Guild Title/Position** -- if set by guild master

The `/font` command toggles whether these elements appear **above** or **below** the character sprite.

The `/showname` command changes the display format:
- Level 0: Character Name only
- Level 1: Character Name + Guild Name
- Level 2: Character Name + Guild Name + Guild Title
- Level 3: Character Name + Guild Name + Guild Title + Guild Emblem (full display)

### 13.2 Name Tag Colors

**Player Name Colors:**

| Relationship | Name Color | Notes |
|-------------|------------|-------|
| Self | White | Your own character's name |
| Other Player (neutral) | White | Default color for all other players |
| Party Member | Green | Party members' names appear green |
| Guild Member | Light Green / Cyan | Same guild members |
| Enemy (PvP map) | Red / Pink | Players on PvP maps who can attack you |
| GM (Game Master) | Yellow / Gold | Special GM tag with distinctive color |

**Monster Name Colors:**

| Monster Type | Name Color | Additional Indicator |
|-------------|------------|---------------------|
| Normal Monster | White | Plain text |
| Aggressive Monster | White | Same as normal (no color distinction) |
| Mini-Boss | White | Grey skull icon next to name |
| MVP Boss | White | Gold skull icon next to name |
| Guardian (WoE) | White | Castle defense marker |

**NPC Names:**
- Always displayed in **white text** above the NPC sprite
- NPC names are visible at all times (not just on hover)
- Quest NPCs may have additional `!` or `?` markers

### 13.3 Monster Display Elements

Monsters display:
1. **Monster Name** -- always visible above the sprite
2. **HP Bar** -- shown if `/monsterhp` is enabled (optional, off by default)
3. **Level** -- NOT shown by default in classic RO. Some private servers show it
4. **Element/Race** -- NOT shown in classic UI. Players must know from external sources

### 13.4 Level 99 Aura

Characters at base level 99 display a special **aura effect** around their sprite:
- A glowing ring/particle effect at the character's feet
- Can be reduced with `/aura` or disabled with `/aura2`
- Serves as a visual "max level" indicator to other players

### 13.5 Party HP Bars

When in a party:
- Party members' HP bars are visible above their sprites (small green bar under their name)
- Non-party players do NOT show HP bars
- The `/showname` setting affects what additional info appears
- `Alt+End` toggles the display of player HP/SP bars overhead

---

## 14. Player/Enemy Multiplayer Sync (Packets)

### 14.1 Key Network Packets (from rAthena / OpenKore)

**Movement Packets:**

| Packet ID | Name | Direction | Size | Description |
|-----------|------|-----------|------|-------------|
| 0x0085 | `CZ_REQUEST_MOVE` | Client->Server | 5 | Player requests to walk to (x, y) |
| 0x0087 | `ZC_NOTIFY_PLAYERMOVE` | Server->Client | 12 | Confirms YOUR walk (start pos + dest pos + timestamp) |
| 0x0086 | `ZC_NOTIFY_MOVE` | Server->Client | 16 | Notifies OTHER CLIENTS about a unit's walk (unit ID + start + dest + timestamp) |
| 0x0088 | `ZC_STOPMOVE` | Server->Client | 10 | Unit stopped moving at (x, y) |

**Spawn/Appear/Vanish Packets:**

| Packet ID | Name | Direction | Size | Description |
|-----------|------|-----------|------|-------------|
| 0x0078 | `ZC_NOTIFY_STANDENTRY` | Server->Client | Variable | Entity appears standing (spawn/enter view range). Contains: unit ID, speed, body state, health state, effect state, job class, hair info, weapon, shield, headgear, walk speed, position, gender, etc. |
| 0x007B | `ZC_NOTIFY_MOVEENTRY` | Server->Client | Variable | Entity appears while walking. Same data as STANDENTRY plus destination coordinates |
| 0x0079 | `ZC_NOTIFY_NEWENTRY` | Server->Client | Variable | New entity spawns into existence (e.g., monster respawn, player login). Same structure as STANDENTRY |
| 0x0080 | `ZC_NOTIFY_VANISH` | Server->Client | 7 | Entity disappears. Contains: unit ID + vanish type (0=out of sight, 1=died, 2=logged out) |

**Combat/Action Packets:**

| Packet ID | Name | Direction | Size | Description |
|-----------|------|-----------|------|-------------|
| 0x0089 | `CZ_REQUEST_ACT` | Client->Server | 7 | Player requests action (attack target ID, action type) |
| 0x008A | `ZC_NOTIFY_ACT` | Server->Client | 29 | Action result: source ID, target ID, server tick, source speed, target speed, damage, number of hits, action type (attack, pickup, sit, stand, etc.) |
| 0x0088 | `ZC_STOPMOVE` | Server->Client | 10 | Stop movement notification |

### 14.2 Spawn/Despawn Flow

**When a player enters a zone:**
1. Server adds player to zone's connected player list
2. Server sends `ZC_NOTIFY_STANDENTRY` (or `NEWENTRY`) to ALL existing players in the zone
3. Server sends existing players' `ZC_NOTIFY_STANDENTRY` packets to the newly joined player
4. Server sends existing monsters' `ZC_NOTIFY_STANDENTRY` packets to the newly joined player
5. Server sends existing NPCs' data to the newly joined player

**When a player leaves view range:**
1. Server sends `ZC_NOTIFY_VANISH` (type 0 = out of sight) to the departing player's client
2. Server sends `ZC_NOTIFY_VANISH` to all players who could previously see the departing entity

**When an entity dies:**
1. Server sends `ZC_NOTIFY_VANISH` (type 1 = died) to all players in view range
2. For monsters: respawn timer starts; on respawn, `ZC_NOTIFY_NEWENTRY` is sent to players in range
3. For players: death screen shown, respawn at save point

### 14.3 Health/Damage Updates

**Damage Broadcasting:**
- Every `ZC_NOTIFY_ACT` packet (action type = attack) includes the damage dealt
- Clients receiving this packet display the damage number and update the target's HP bar
- HP percentage is included in spawn packets for monsters (so HP bars are accurate on first sight)

**HP Update Packets:**
- `ZC_NOTIFY_ACT` includes source/target IDs and damage
- Party member HP is sent via party-specific packets when HP changes
- Monster HP percentage updates are sent as needed (when damage occurs)
- Player HP is primarily tracked locally + verified server-side

### 14.4 View Range / Area of Interest

- The server only sends packets to clients within a certain range of the event
- Typical view range: ~14-18 cells (varies by packet type)
- Entities outside view range are not tracked client-side
- When entities enter view range, they are spawned (STANDENTRY/MOVEENTRY)
- When entities leave view range, they are despawned (VANISH)

### 14.5 Position Synchronization Summary

**How RO Keeps Everyone In Sync:**
1. All movement is **server-authoritative** -- client requests, server validates
2. Server stores definitive position for every entity
3. Walk commands include both **start** and **destination** positions
4. Clients animate between start and destination using the entity's known speed
5. Combat checks use the **server-side position**, not the client's animated position
6. If client position drifts from server (due to lag), the server sends correction packets
7. No client-side prediction or extrapolation -- clients only render what the server tells them

### 14.6 Zone Transition Sync

When a player changes zones:
- In Sabri_MMO: persistent socket connection survives zone transitions
- In original RO: the client reconnects to a new map server (each map runs on its own thread/process in Aegis)
- Character data is transferred from old map to new map via inter-server communication
- All entity data for the new zone is sent fresh (new STANDENTRY packets for everything)
- The old zone sends VANISH packets to remaining players

---

## Appendix A: RO Classic Movement Commands Summary

| Command | Effect |
|---------|--------|
| Left-click ground | Walk to cell |
| Hold left-click | Continuous movement following cursor |
| Shift + left-click direction | Change facing direction without moving |
| Left-click monster | Attack monster (with `/noctrl`) or walk near (without) |
| Ctrl + left-click monster | Force attack (without `/noctrl`) |
| Left-click NPC | Walk to NPC and interact |
| Left-click item | Walk to item and pick up |
| Right-click player | Open context menu |
| Shift + right-click player | Auto-follow player |
| Right-click + drag | Rotate camera |
| Mouse wheel | Zoom in/out |
| Ctrl + right-click + move | Zoom without wheel |
| Shift + mouse wheel | Adjust camera height |
| Double right-click | Reset camera to default north-facing view |
| Insert | Sit/stand toggle |
| Alt+Home | Toggle ground cursor |
| `/noctrl` or `/nc` | Attack any mob with single left-click |
| `/noshift` or `/ns` | Cast support skills on players without Shift |
| `/snap` | Toggle position snap in combat |

## Appendix B: ASPD Base Weapon Delay Table (BTBA, seconds)

| Class | Bare Hand | Dagger | 1H Sword | 2H Sword | Spear | Mace | Axe | Bow | Staff | Knuckle | Katar |
|-------|-----------|--------|----------|----------|-------|------|-----|-----|-------|---------|-------|
| Novice | 0.50 | 0.55 | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Swordman | 0.40 | 0.65 | 0.70 | 0.60 | 0.65 | 0.70 | 0.80 | -- | -- | -- | -- |
| Mage | 0.35 | -- | -- | -- | -- | -- | -- | -- | 0.65 | -- | -- |
| Archer | 0.50 | -- | -- | -- | -- | -- | -- | 0.70 | -- | -- | -- |
| Thief | 0.40 | 0.50 | 0.70 | -- | -- | -- | -- | 0.85 | -- | -- | -- |
| Merchant | 0.40 | 0.65 | 0.55 | -- | -- | 0.65 | 0.70 | -- | -- | -- | -- |
| Acolyte | 0.40 | 0.60 | 0.70 | -- | -- | 0.70 | -- | -- | -- | -- | -- |
| Knight | 0.30 | -- | 0.55 | 0.45 | 0.45 | 0.55 | 0.80 | -- | -- | -- | -- |
| Assassin | 0.35 | 0.40 | 0.55 | -- | -- | -- | -- | -- | -- | -- | 0.45 |

## Appendix C: Sabri_MMO Implementation Mapping

| RO Classic Concept | Sabri_MMO Implementation |
|--------------------|--------------------------|
| Cell grid | UE5 3D world, 1 cell = 50 UE units |
| A* pathfinding | UE5 NavMesh or direct movement |
| Click-to-move | WASD + click-to-move hybrid |
| walkSpeed (ms/cell) | `moveSpeed = (50 / walkSpeed) * 1000` UE/sec |
| Attack range (cells) | Range in UE units (cells * 50) |
| Chebyshev distance | Max(abs(dx), abs(dy)) or UE5 distance / 50 |
| Position packets | Socket.io events (`player:moved`, `enemy:position`) |
| Spawn packets | Socket.io events (`player:joined`, `enemy:spawn`) |
| Vanish packets | Socket.io events (`player:left`, `enemy:death`) |
| ASPD auto-attack | 50ms combat tick loop, ASPD-based timing |
| NPC interaction | Click-interact system, `UItemInspectSubsystem` |
| Name tags | UE5 widget components above actors |
| Cursor changes | UE5 cursor type switching on hover |

---

## Sources

- [Ragnarok Research Lab - Movement and Pathfinding](https://ragnarokresearchlab.github.io/game-mechanics/movement/)
- [iRO Wiki Classic - ASPD](https://irowiki.org/classic/ASPD)
- [iRO Wiki Classic - Movement Speed](https://irowiki.org/classic/Movement_Speed)
- [iRO Wiki - Basic Game Control](https://irowiki.org/wiki/Basic_Game_Control)
- [iRO Wiki - Attacks](https://irowiki.org/wiki/Attacks)
- [iRO Wiki - Skill Types](https://irowiki.org/wiki/Skill_Types)
- [rAthena - path.cpp (GitHub)](https://github.com/rathena/rathena/blob/master/src/map/path.cpp)
- [rAthena - unit.cpp (GitHub)](https://github.com/rathena/rathena/blob/master/src/map/unit.cpp)
- [rAthena - clif.cpp (GitHub)](https://github.com/rathena/rathena/blob/master/src/map/clif.cpp)
- [OpenKore - packetlist.txt (GitHub)](https://github.com/OpenKore/openkore/blob/master/tables/packetlist.txt)
- [AnnieRuru - RO Distance Measurement (Blog)](https://annieruru.blogspot.com/2019/01/ragnarok-online-use-number-of-squares.html)
- [PlayRagnarok.com - Gameplay Guide](https://www.playragnarok.com/gameguide/howtoplay_gameplay02.aspx)
- [Steam Community - RO Basic Controls Guide](https://steamcommunity.com/sharedfiles/filedetails/?id=190560301)
- [WarpPortal Forums - Position Lag Fix](https://forums.warpportal.com/index.php?/topic/217602-the-position-lag-fix/)
- [WarpPortal Forums - ASPD Discussion](https://forums.warpportal.com/index.php?/topic/90773-classic-aspd-question/)
- [RateMyServer - Pre-Renewal Mechanics](https://ratemyserver.net/)
- [Ragnarok Online Encyclopedia - Attack Range](https://ragnarok-online-encyclopedia.fandom.com/wiki/Attack_Range)
- [rAthena Forums - Walk Path](https://rathena.org/board/topic/87886-rathena-walk-path/)
