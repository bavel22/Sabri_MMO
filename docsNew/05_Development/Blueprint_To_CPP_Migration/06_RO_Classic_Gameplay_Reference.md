# RO Classic Gameplay Reference -- Pre-Renewal Behavior Specification

**Purpose:** Comprehensive reference for how all migrated systems behave in the original Ragnarok Online Classic (pre-renewal). Every C++ subsystem built during the Blueprint migration must reproduce these behaviors exactly unless explicitly noted otherwise.

**Scope:** Movement, auto-attack, targeting, cursors, entity synchronization, name tags, animations, pathfinding, and range checking.

---

## 1. Movement System

### Click-to-Move

The primary movement method in RO Classic is click-to-move. The player left-clicks a position on the ground, and the character pathfinds to that location using A* on a 2D tile walkability grid.

- **Pathfinding:** Original RO uses a 2D tile-based A* algorithm on a walkability grid embedded in the map's `.gat` file. Each cell is marked walkable, unwalkable, or water. The client computes the path and sends each waypoint to the server for validation.
- **Path display:** A small white cursor marker (crosshair dot) briefly appears at the clicked ground position, then fades.
- **Path cancellation:** Clicking a new position cancels the current path and starts a new one. The character immediately begins walking toward the new destination. There is no queuing of movement commands.
- **Arrival:** When the character reaches the destination, it stops and enters idle animation. If the destination cell is unwalkable, the character stops at the nearest walkable cell.

### WASD Movement

Original RO Classic does not have WASD movement. This is a modern addition in Sabri_MMO.

- **Override behavior:** Pressing any WASD key immediately cancels any active click-to-move path. The character moves in the pressed direction relative to the camera facing.
- **Release behavior:** Releasing all WASD keys returns the character to idle. It does not resume any previous click-to-move path.
- **Priority:** WASD takes absolute priority over click-to-move. While any WASD key is held, click-to-move input is either ignored or immediately overridden.

### Movement Speed

- **Base speed:** 150 speed units (RO internal). This translates to a fixed walk speed that is the same for all characters at level 1 with no buffs.
- **AGI influence:** In RO Classic, AGI does NOT directly affect movement speed. Movement speed is modified only by specific skills and items:
  - **Increase AGI** (Acolyte buff): +25% movement speed
  - **Decrease AGI** (debuff): -25% movement speed
  - **Speed Potion / Awakening Potion:** +25% movement speed (does not stack with Increase AGI's speed bonus)
  - **Peco Peco Riding** (Knight/Crusader): +25% movement speed
  - **Cart** (Merchant classes): -50% movement speed when pushing a cart (offset by Cart Revolution passive)
- **Speed cap:** Movement speed bonuses generally do not stack. The fastest a character can move is approximately +25% above base speed.

### Diagonal Movement

- **Same speed as cardinal:** In the original RO, diagonal movement covers the same distance per second as cardinal movement. The character does not move 1.41x faster diagonally (which would happen with naive vector addition on a grid). The server clamps movement speed regardless of direction.
- **Sabri_MMO equivalent:** NavMesh pathfinding handles this automatically -- `SimpleMoveToLocation` produces a path that moves at constant speed regardless of direction.

### Movement Blocking Conditions

Movement is completely blocked (character cannot walk) when any of the following are active:

| Condition | Source | Duration |
|-----------|--------|----------|
| Stun | Status effect (Bash Lv10, Hammer Fall, etc.) | 1-5 seconds |
| Freeze | Status effect (Frost Diver, Cold Bolt) | Until thawed or duration expires |
| Stone Curse (phase 2) | Status effect (Stone Curse skill) | Until cured or duration expires |
| Ankle Snare | Trap skill (Hunter) | Trap duration or until broken |
| Close Confine | Rogue skill | Until duration expires or either party moves out of range |
| Sleep | Status effect | Until damage wakes or duration expires |

### Movement Speed Reduction Conditions

| Condition | Effect | Source |
|-----------|--------|--------|
| Curse | Extremely slow movement (roughly -75% speed) | Status effect, some monsters |
| Quagmire | -50% movement speed while in area | Sage ground AoE |
| Decrease AGI | -25% movement speed | Skill debuff |
| Poison | No direct speed reduction, but HP drain over time | Status effect |

### Movement Interrupts Casting

In RO Classic, moving while casting a skill interrupts the cast -- the skill fails and goes on cooldown. This applies to all skills with a cast time greater than 0.

Exception: Equipping a **Phen Card** (or Bloody Butterfly Card in some servers) grants the **Cast Uninterruptible by Movement** effect, allowing the character to walk while casting without interrupting the skill. The character still stops for the cast animation, but subsequent movement does not cancel it.

---

## 2. Auto-Attack System

### Initiating Auto-Attack

- **Click enemy:** Left-clicking a hostile monster or player (in PvP) initiates auto-attack.
- **Walk-to-range:** If the target is beyond attack range, the character first walks toward the target. The client pathfinds to a cell adjacent to the target (melee) or within weapon range (ranged). Once in range, the character stops walking and begins the attack loop.
- **No explicit "target then attack":** There is no two-step process of "select target, then press attack." Clicking an enemy is simultaneously targeting and attacking.

### Attack Loop

Once in range, the attack loop runs on the server:

1. **ASPD timer:** The server calculates the delay between attacks based on ASPD (Attack Speed). Formula:
   - Original RO: `delay = (200 - ASPD) * 10` milliseconds (at frame-level precision)
   - Sabri_MMO: `delay = (200 - ASPD) * 50` milliseconds (scaled for smoother feel)
   - Example: ASPD 175 = 1250ms between attacks, ASPD 185 = 750ms, ASPD 195 = 250ms
2. **Per-attack tick:** On each attack tick, the server rolls:
   - **HIT vs FLEE:** Attacker's HIT stat vs target's FLEE stat. `hitRate = 80 + HIT - FLEE`, clamped to 5-100%.
   - **Critical check:** `critRate = CRI` (from LUK and bonuses), clamped to minimum 1%.
   - **Damage calculation:** If hit succeeds, full physical damage formula (ATK, DEF, element, size modifiers, card modifiers, etc.)
3. **Miss:** If the attack misses, the server sends a damage event with `damage: 0` and a miss flag. The client displays "Miss" text above the attacker.
4. **Hit:** If the attack hits, the server sends a damage event with the calculated damage. The client displays the damage number above the target and plays the attack animation on the attacker.
5. **Critical hit:** If the attack is critical, it bypasses FLEE (always hits) and ignores DEF. The client displays the damage number with a distinct visual effect (larger text, different color, starburst).

### Attack Continuation and Termination

Auto-attack continues indefinitely until one of these occurs:

| Termination Condition | What Happens |
|----------------------|-------------|
| Target dies | Server sends `combat:death` or `combat:target_lost`. Client returns to idle. |
| Player clicks elsewhere | Player clicks ground (walk) or another entity (retarget). Server receives `combat:stop_attack`. |
| Player moves (WASD) | WASD input cancels auto-attack. Client sends `combat:stop_attack`. |
| Player uses a skill | Skill use interrupts auto-attack for the cast duration. Auto-attack may resume after skill completes (depends on implementation). |
| Player disconnects | Server cleanup stops the attack loop. |
| Target moves out of range | If the target walks/is knocked back out of range, the attacker chases. The server sends `combat:out_of_range`, and the client initiates chase movement. Once back in range, attacks resume. |
| Player is stunned/frozen | Status effects that prevent action stop the attack loop until the effect expires. |
| Player dies | Death stops all actions. |

### ASPD (Attack Speed)

ASPD determines how fast the character attacks. Range: 0-195 (or 190 for dual wield).

- **Formula (pre-renewal):**
  ```
  WD = WeaponDelay (from ASPD_BASE_DELAYS table, per class and weapon type)
  speedReduction = floor((WD * AGI / 25 + WD * DEX / 100) / 10)
  ASPD = 200 - (WD - speedReduction) * (1 - buffSpeedModifier)
  ```
- **Weapon delay varies by class and weapon type:** A Swordsman with a One-Handed Sword has a different base delay than an Archer with a Bow or an Assassin with a Katar.
- **ASPD cap:** 190 for dual wield, 195 for single wield. The server hard-caps at these values.
- **Dual wield ASPD:** `WD = floor((WD_right + WD_left) * 7 / 10)`. This means dual wield characters attack slightly slower per-hand but hit twice per cycle.
- **AGI is the primary ASPD stat.** DEX contributes to a lesser degree. Buffs (Increase AGI, Two-Hand Quicken, Adrenaline Rush) apply a multiplier.

### Facing

The attacking character always faces the target during auto-attack. If the target moves to a different relative position, the attacker rotates to face the new direction. This rotation is instant (snaps to face target), not interpolated.

---

## 3. Targeting

### Click-Based Targeting (No Tab-Target)

RO Classic uses pure click-based targeting. There is no tab-targeting or target-cycling keybind in the original game.

- **Target acquisition:** The target is acquired by left-clicking an entity. This immediately initiates the appropriate action (attack for enemies, interact for NPCs).
- **Target indicator:** A small selection circle (highlight ring) appears under the targeted entity. This is visible only to the local player.
- **Target clearing:** The target is cleared when:
  - The entity dies
  - The player clicks on empty ground
  - The player clicks a different entity
  - The entity moves out of visual range
  - The player changes zones

### Target Information Display

When an entity is targeted (or hovered):
- **Monsters:** Name and HP bar are visible. The HP bar shows as a colored bar above or below the entity's name tag.
- **Players:** Name is always visible (name tag). HP bar is only visible for party members.
- **NPCs:** Name is always visible. No HP bar (NPCs are not damageable).

### Skill Targeting

Skill targeting is a separate mode that overrides normal click behavior:

1. **Player presses skill hotkey** (e.g., 1-9 on hotbar)
2. **Cursor changes** to indicate targeting mode:
   - **Single-target skill:** Cursor becomes a crosshair/targeting reticle. Next click on a valid target (enemy, player, or self) activates the skill.
   - **Ground AoE skill:** Cursor shows a targeting area indicator (circle/square on the ground). Next click on the ground activates the skill at that position.
   - **Self-cast skill:** No targeting mode. Skill activates immediately on the caster.
3. **Cancel targeting:** Right-click or pressing Escape cancels targeting mode and returns to normal cursor.
4. **Invalid target:** Clicking an invalid target (e.g., friendly player for an offensive skill) produces an error message and keeps targeting mode active.

---

## 4. Cursor System

The cursor changes appearance based on what the mouse is hovering over, providing immediate visual feedback about the available action.

### Cursor Types

| Cursor | Visual | Shown When |
|--------|--------|------------|
| Normal | Default arrow pointer | Hovering over empty ground, UI elements, or nothing |
| Attack | Sword icon (or crosshair for ranged) | Hovering over a hostile monster or enemy player (PvP) |
| Talk/Interact | Speech bubble icon | Hovering over an NPC (shop, quest, Kafra, warp attendant) |
| Pickup | Open hand icon | Hovering over a ground item (lootable drop) |
| Skill Target | Crosshair/targeting reticle | In skill targeting mode (after pressing a skill hotkey) |
| Rotate | Double-headed arrow | Holding right mouse button for camera rotation (3D clients only) |
| Disabled | Circle with line / grayed arrow | Hovering over an unwalkable area or invalid target |

### Cursor Priority

When multiple entities overlap under the cursor (stacked on top of each other), the cursor shows the icon for the highest-priority entity:

1. **NPC** (highest) -- always shows talk/interact cursor
2. **Player** -- shows attack cursor if in PvP zone, otherwise default
3. **Monster/Enemy** -- shows attack cursor
4. **Ground Item** -- shows pickup cursor
5. **Ground** (lowest) -- shows normal cursor

### Cursor and UI Interaction

- When the mouse is over a UI panel (inventory, skill tree, equipment, etc.), the cursor is the normal arrow regardless of what is behind the panel in the 3D world.
- Clicks on UI panels are consumed by the UI and do not pass through to the game world.
- Some UE5 implementations use `FReply::Handled()` vs `FReply::Unhandled()` to manage this click-through behavior.

---

## 5. Entity Name Tags

### Player Name Tags

- **Text content:** The character's name (e.g., "SwordMaster")
- **Color:** White for all players by default
- **Party members:** Green text (if party system is implemented)
- **PvP hostile players:** Red text (only in PvP-enabled zones)
- **Guild name:** Displayed below the character name in a smaller font, blue text
- **Guild emblem:** Small icon displayed next to the guild name (if guild system is implemented)
- **GM characters:** Name displayed in yellow or gold text (special privilege indicator)

### Monster Name Tags

- **Text content:** Monster name (e.g., "Poring", "Baphomet")
- **Color:** White for passive/non-aggressive monsters
- **Aggressive monsters:** Some clients display aggressive monster names in a different shade or with an icon indicator
- **Boss monsters:** Name in distinctive color (often red or with a special marker)
- **Level display:** Some RO clients show "Lv.XX" after the monster name (e.g., "Poring Lv.1"). This is optional and varies by client.
- **HP bar:** A thin HP bar appears below the monster's name when the monster has been damaged or is being targeted.

### NPC Name Tags

- **Text content:** NPC's name (e.g., "Tool Dealer", "Kafra", "Knight Job NPC")
- **Color:** Green text, always visible
- **No HP bar:** NPCs do not show HP bars (they are not damageable)

### Dead Entity Name Tags

- **Monsters:** Name tag is hidden when the monster dies. The death animation plays, and the corpse fades or disappears after a timer.
- **Players:** Name tag remains visible but may be dimmed/grayed for dead players. In some clients, the name stays visible so party members can find the body for resurrection skills.

### Rendering Properties

- **Billboard:** Name tags always face the camera. As the camera rotates, name tags rotate to remain readable. This is standard billboard behavior.
- **Text outline:** Name text has a 1-pixel black outline (or shadow) for readability against any background (bright sky, dark dungeon, etc.).
- **Scale with distance:** Name tags maintain roughly the same screen-space size regardless of camera distance. Some implementations use a fixed world-space size with a minimum screen-space clamp.
- **Culling:** Name tags are not rendered for entities outside the camera frustum. There is also a maximum distance beyond which name tags are hidden (typically 30-50 meters / 3000-5000 UE units).
- **No occlusion by default:** In the original RO (2.5D isometric), name tags are always visible because the camera angle prevents entities from hiding behind walls. In a 3D client like Sabri_MMO, name tags may optionally be occluded by geometry, but this is a design choice -- original RO does not occlude them.
- **Z-ordering:** When multiple name tags overlap on screen, there is no strict render order guarantee. In practice, name tags for closer entities render on top of those for farther entities.

---

## 6. Remote Player Synchronization

### How Other Players Appear

When a player enters a zone, the server sends data about all other players already in that zone:

1. **Server sends `player:moved`** for each existing player in the zone. Each message contains: `{ id, x, y, z, name, jobClass, level, hairStyle, hairColor, gender, ... }`.
2. **Client spawns a remote player actor** for each message. The actor is placed at the received position with the appropriate skeletal mesh, hair style, hair color, and gender configuration.
3. **Name tag appears** above the spawned actor with the character's name.

### Position Updates

- **Update rate:** The server rebroadcasts `player:moved` to all players in the same zone at approximately 15-30 Hz (dependent on the position update rate of the moving player).
- **Payload:** `{ id, x, y, z }` (or `{ id, x, y, z, name, jobClass, ... }` for full updates).
- **Client interpolation:** The client does not teleport remote players to each new position. Instead, it sets the new position as an interpolation target and smoothly moves the actor toward it over time. This produces visually smooth movement even at lower update rates.
- **Interpolation speed:** The interpolation speed is tuned to slightly exceed the character's maximum movement speed, so the remote player's visual position stays close to the server's authoritative position without rubber-banding.

### Player Join and Leave

- **New player enters zone:** Server broadcasts `player:moved` (with full data) to all existing players. Existing players' clients spawn a new remote player actor.
- **Player leaves zone:** Server broadcasts `player:left` with `{ id }`. All clients in the zone destroy the corresponding remote player actor.
- **Zone transition:** When the local player changes zones, all remote player actors from the old zone are destroyed. The new zone's remote players are received as `player:moved` messages.

### What Is NOT Visible for Remote Players

In RO Classic, limited information is shown about other players:

- **HP/SP bars:** Not visible unless the player is in your party.
- **Buffs/debuffs:** Buff icons are not visible on other players (only your own are shown). Some buffs have visible VFX (Increase AGI glow, Blessing sparkle) that other players can see.
- **Equipment details:** Weapon and shield are visible on the character model. Armor is shown as a sprite change. Headgear is visible. Stats and exact item details are not shown.
- **Level and class:** Not directly displayed in the name tag (though the character model and equipment imply class). Some clients show class/level on hover or via the `/who` command.

---

## 7. Enemy Synchronization

### Server-Controlled Enemies

All enemy behavior is server-authoritative. The client is purely a display layer for enemies.

### Spawn

- **Server spawns enemies** at predefined spawn points defined per zone. Each zone has a configured set of spawn points with: template ID (monster type), position, spawn count, respawn timer.
- **Server broadcasts `enemy:spawn`** to all players in the zone when an enemy spawns. Payload: `{ id, templateId, x, y, z, name, level, hp, maxHp, element, race, size, ... }`.
- **Client creates enemy actor** at the received position with the appropriate skeletal mesh and stats. The enemy immediately enters idle animation.
- **On zone entry:** The server sends `enemy:spawn` for all currently alive enemies in the zone. This ensures players joining a zone see all existing enemies.

### Movement

- **Server AI drives movement:** The server's AI loop (200ms tick) determines enemy movement based on AI code (idle wander, aggro chase, return to spawn, etc.).
- **Server broadcasts `enemy:move`** when an enemy's position changes: `{ id, x, y, z }`.
- **Client interpolates:** The client smoothly moves the enemy actor toward each new position, identical to remote player interpolation.
- **Idle wander:** Non-aggressive enemies periodically pick a random nearby walkable position and walk to it. This creates natural-looking idle behavior.
- **Aggro chase:** When an enemy aggros on a player (based on AI code, detect range, and aggro conditions), it chases the player. Movement updates become more frequent during chase.

### Attack

- **Server determines when enemy attacks:** When an enemy is in attack range of its target, the server runs the enemy's attack loop (similar to player auto-attack, but with the enemy's ASPD).
- **Server broadcasts `enemy:attack`** with `{ enemyId }` to trigger the attack animation on the client.
- **Client plays attack animation:** The enemy actor's attack montage plays. The enemy faces its target during the attack.
- **Damage event:** The server also sends `combat:damage` (with the enemy as attacker and the player as target) containing the damage amount. The client shows the damage number above the player.

### Death

- **Server determines death:** When enemy HP reaches 0, the server marks it as dead.
- **Server broadcasts `enemy:death`** with `{ id, killerId }`. The killer is relevant for EXP distribution and loot drops.
- **Client death visuals:**
  1. Death animation plays (fall-over, dissolve, or fade depending on the monster).
  2. Collision is disabled (the corpse is not targetable or clickable).
  3. Corpse remains visible for a short duration (1-3 seconds in original RO, configurable).
  4. Corpse fades out and the actor is destroyed or recycled.
- **Name tag hidden:** The monster's name tag disappears on death.
- **HP bar hidden:** The HP bar above the monster disappears on death.

### Respawn

- **Server respawn timer:** After an enemy dies, the server starts a respawn timer (varies per monster: 5 seconds for weak monsters, minutes for bosses, etc.).
- **Server broadcasts `enemy:spawn`** when the respawn timer completes. This is the same event used for initial spawn -- the client cannot distinguish between a fresh spawn and a respawn.
- **Client creates/recycles actor:** The client either creates a new actor or reuses a pooled actor at the spawn position.
- **MVPs/Bosses:** Boss monsters have much longer respawn timers (60-120 minutes) and may spawn at random positions within a zone rather than fixed spawn points.

---

## 8. Attack Animations

### Local Player Attack Animation

When the server confirms that the local player's attack hit (or missed), the client plays an attack animation:

1. **Rotate to face target:** The player character instantly snaps to face the target. This rotation is immediate (not interpolated).
2. **Play attack montage:** An animation montage plays based on weapon type:
   - **Unarmed:** Punch animation
   - **One-Handed Sword / Dagger:** Slash animation (horizontal swing)
   - **Two-Handed Sword:** Heavy slash animation (overhead or diagonal swing)
   - **Spear / Lance:** Thrust animation (forward stab)
   - **Axe:** Chop animation (downward swing)
   - **Mace / Rod:** Swing animation (overhead bash)
   - **Bow:** Draw and release animation (pull string, release arrow)
   - **Katar:** Dual stab animation (alternating thrusts)
   - **Book:** Swing animation (similar to mace)
   - **Knuckle / Fist:** Rapid punch animation (like unarmed but faster)
3. **Animation speed scales with ASPD:** Higher ASPD = faster animation playback. The attack montage's play rate is adjusted so that the animation completes within the ASPD interval. This prevents the animation from being cut short or leaving a gap.
4. **Animation does not interrupt movement visually:** In the original 2D RO, the character stops walking to attack, then resumes. In 3D Sabri_MMO, the same behavior applies: the character stops at the attack position, plays the animation, then remains stopped until the next attack or until movement input.

### Remote Player Attack Animation

When another player in the zone attacks, the client receives a `combat:damage` event from the server. The remote player's attack animation is triggered by this event:

1. **Identify the attacker:** Parse `attackerId` from the event. Find the corresponding remote player actor.
2. **Rotate to face target:** The remote player actor snaps to face the damage target.
3. **Play attack montage:** Same weapon-based montage selection as the local player.
4. **Timing:** The animation plays when the `combat:damage` event arrives, not when the attack was initiated. This means there is a slight network delay between the actual attack and the animation playing on other clients. This is acceptable and matches original RO behavior.

### Enemy Attack Animation

When an enemy attacks, the client receives `enemy:attack` from the server:

1. **Find enemy actor:** Look up the enemy by `enemyId` in the enemy registry.
2. **Play attack animation:** The enemy's attack montage plays. Most enemies have a single generic attack animation.
3. **Face target:** The enemy rotates to face its target (if target information is available in the event).

### Multi-Hit Animations

Some attacks produce multiple hits:

- **Double Attack (Thief passive):** The attack animation plays twice in rapid succession (or a special "double slash" animation plays once). Two damage numbers appear.
- **Dual Wield (Assassin):** Right-hand and left-hand attacks produce two separate damage numbers. The animation may be a single attack animation with two damage events, or two rapid animations. Sabri_MMO shows two damage numbers with a Z offset (left-hand number slightly lower).
- **Multi-hit skills (bolt spells, Arrow Shower, etc.):** Each hit spawns a separate damage number with a 200ms delay between each. Skill-specific VFX (fire bolts, cold bolts, etc.) are separate from the attack animation system.

### Hit Reactions

When an entity takes damage:

- **Flash/blink:** The damaged entity briefly flashes (color shift or opacity blink) to indicate being hit. This is typically a 100-200ms red tint or white flash on the mesh.
- **No knockback (melee):** Standard melee attacks do not knock back the target. The target stays in place.
- **Knockback (specific skills):** Some skills (Bash Lv10, Bowling Bash, etc.) apply knockback. The target slides backward 1-6 cells.
- **Flinch animation:** Some implementations play a flinch/stagger animation on hit, but original RO's 2D sprites do not show an explicit flinch for standard hits.

---

## 9. NavMesh vs Grid Pathfinding

### Original RO: 2D Tile Grid

RO Classic uses a 2D tile-based system:

- **Walkability grid:** Each map has a `.gat` file defining a grid of cells. Each cell is marked as walkable, unwalkable (wall), water, or snipable (shootable but not walkable -- e.g., cliff edges).
- **Cell size:** Each cell is approximately 5x5 game units (small, high-resolution grid).
- **A* pathfinding:** The client runs A* on the walkability grid to find the shortest path from the character's current cell to the destination cell. Diagonal movement is allowed.
- **Path length limit:** There is a maximum path length (typically ~14 cells per pathfinding request). For longer distances, the client chains multiple pathfinding requests.
- **Server validation:** The server checks that each movement packet follows the walkability grid. It does not re-run A* but verifies that each step is to an adjacent walkable cell.

### Sabri_MMO: 3D NavMesh

Sabri_MMO uses UE5's navigation mesh system:

- **NavMesh volumes:** `NavMeshBoundsVolume` actors define the navigable area in each level. The engine generates a navigation mesh from the level geometry at build time.
- **`SimpleMoveToLocation`:** The primary pathfinding function. Takes a target position and pathfinds via the NavMesh. Handles obstacle avoidance and path smoothing automatically.
- **`UNavigationSystemV1`:** The underlying system that manages NavMesh data, path queries, and agent movement.
- **NavMesh agent:** The player pawn's `UCapsuleComponent` dimensions define the NavMesh agent size. The NavMesh is generated with these dimensions, so paths avoid areas too narrow for the character.
- **Real-time path following:** `UPathFollowingComponent` (part of `UCharacterMovementComponent`) handles smooth path following with acceleration, deceleration, and corner rounding.

### Behavioral Equivalence

Despite the different underlying technologies, the player-facing behavior is identical:

| Behavior | Grid Pathfinding | NavMesh Pathfinding |
|----------|-----------------|---------------------|
| Click to walk | A* on tile grid | `SimpleMoveToLocation` on NavMesh |
| Obstacle avoidance | Path follows walkable cells | Path follows NavMesh surface |
| Unwalkable areas | Cells marked unwalkable in `.gat` | Areas outside NavMesh bounds or blocked by geometry |
| Path cancellation | Stop current path, start new A* | `StopMovement()`, then new `SimpleMoveToLocation` |
| Movement speed | Constant per tick | `MaxWalkSpeed` on `UCharacterMovementComponent` |
| Arrival | Character stops at destination cell | Character stops when `EPathFollowingResult::Success` fires |

### Jump

Original RO Classic does not have jumping. Characters are always grounded. Sabri_MMO adds jump for player enjoyment, but it does not affect gameplay (no jump attacks, no jump-over-obstacles). Jump uses `ACharacter::Jump()` with the standard `UCharacterMovementComponent` gravity.

---

## 10. Range Checking

### Range System Overview

All range checks in Sabri_MMO are performed server-side. The client does visual/predictive range checks for responsiveness (e.g., deciding whether to start walking toward a target or attack immediately), but the server has final authority.

### Melee Range

- **Original RO:** 1 cell distance (approximately one tile width, ~5 game units).
- **Sabri_MMO:** 150 Unreal units (configured as `COMBAT.MELEE_RANGE` on the server).
- **Melee weapons:** All daggers, swords, axes, maces, rods, spears (1H and 2H unless explicitly ranged like a Lance), and unarmed attacks use melee range.
- **Effective check:** The server computes Euclidean distance between attacker and target positions. If `distance <= MELEE_RANGE + RANGE_TOLERANCE`, the attack is in range.

### Ranged Attack Range

- **Original RO:** Bows have a base range of approximately 5 cells. The Archer skill **Vulture's Eye** adds +1 cell per skill level (up to +10 cells at Lv10). Some weapons have fixed extended range (e.g., Gunslinger weapons).
- **Sabri_MMO:** Default ranged range is 800 Unreal units (`COMBAT.RANGED_RANGE`). Vulture's Eye and weapon range modifiers adjust this value on the player object (`player.attackRange`).
- **Weapon range from DB:** When a player equips a weapon, the server reads the weapon's `range` column from the items table. This value (already converted from cells to Unreal units by the canonical item data: `cells * 50`) is stored on the player as `player.attackRange`.

### Skill Range

- **Per-skill definition:** Each skill has a `range` value defined in `ro_skill_data.js`. This determines the maximum distance from the caster to the target (or target position for ground AoE).
- **Range values (Sabri_MMO):** Typically 150-900 Unreal units. Examples:
  - Bash: 150 (melee range, same as auto-attack)
  - Fire Bolt: 450 (9 cells equivalent)
  - Heal: 450 (9 cells)
  - Storm Gust: 450 (9 cells, ground AoE)
  - Pneuma: 450 (9 cells, ground placement)
- **Server check:** `executeCastComplete()` re-checks range with `RANGE_TOLERANCE` (50 units) when the cast finishes. If the target moved out of range during the cast time, the skill fails with an out-of-range error.

### Range Tolerance

- **Value:** 50 Unreal units (`COMBAT.RANGE_TOLERANCE`).
- **Purpose:** Provides a small buffer so that clients walking toward a target at max range do not experience flickering in-range/out-of-range states due to position sync latency.
- **Application:** Added to the maximum range for all range checks:
  ```
  actual check: distance <= (maxRange + RANGE_TOLERANCE)
  ```
- **Client-side:** The client uses a slightly smaller range for its predictive check, so it moves the character slightly closer than the server's maximum. This ensures the server never rejects an attack due to the client stopping too early.

### Range and Movement Integration

The interaction between range checking and movement creates the walk-to-attack and walk-to-cast behaviors:

1. **Player clicks enemy (melee):**
   - Client checks: is target within `MELEE_RANGE`?
   - If yes: immediately emit `combat:attack`
   - If no: start walking toward target via `SimpleMoveToLocation`
   - When in range: stop walking, emit `combat:attack`

2. **Player clicks enemy (ranged):**
   - Client checks: is target within `attackRange` (weapon-dependent)?
   - If yes: immediately emit `combat:attack`
   - If no: start walking toward target
   - When in range: stop walking, emit `combat:attack`

3. **Player uses targeted skill:**
   - Client checks: is target within skill's `range`?
   - If yes: immediately emit `skill:use`
   - If no: start walking toward target (walk-to-cast)
   - When in range: stop walking, emit `skill:use`

4. **Target moves during combat:**
   - Server detects target moved out of range
   - Server sends `combat:out_of_range`
   - Client starts chasing (walk toward target)
   - Server stops the attack loop
   - When client reaches range again, it re-emits `combat:attack`

### Visual Range Indicator

Original RO does not display a visual range circle around the character. The player learns attack and skill ranges through experience. Some private servers and modern recreations add optional range circle overlays as quality-of-life features.

In Sabri_MMO, the ground AoE targeting system (from `SkillTreeSubsystem`) shows a targeting circle when casting ground-targeted skills, which serves as an implicit range indicator (if you can see the circle at the target location, it is in range).

---

## Appendix A: Combat Constants (Sabri_MMO Server)

Reference values from `server/src/index.js`:

```
MELEE_RANGE:       150   Unreal units (default melee attack range)
RANGED_RANGE:      800   Unreal units (default ranged attack range)
RANGE_TOLERANCE:    50   Unreal units (buffer added to all range checks)
DEFAULT_ASPD:      175   (0-195 scale)
ASPD_CAP:          195   (hard cap, 190 for dual wield)
COMBAT_TICK_MS:     50   ms (server combat tick interval)
RESPAWN_DELAY_MS: 5000   ms (player respawn delay)
```

Attack interval formula:
```
intervalMs = (200 - ASPD) * 50
```
Examples:
```
ASPD 150 → (200-150)*50 = 2500ms (slow, low level)
ASPD 175 → (200-175)*50 = 1250ms (default)
ASPD 185 → (200-185)*50 =  750ms (moderately fast)
ASPD 190 → (200-190)*50 =  500ms (dual wield cap)
ASPD 195 → (200-195)*50 =  250ms (single wield cap, very fast)
```

---

## Appendix B: Entity Type Summary

Quick reference for all entity types the migrated systems interact with:

| Entity | Original RO | Sabri_MMO Current (Blueprint) | Sabri_MMO Target (C++) |
|--------|-------------|------------------------------|----------------------|
| Local player | 2D sprite on walkability grid | `BP_MMOCharacter` (Blueprint pawn) | `ASabriMMOCharacter` (C++ pawn, already exists) |
| Remote player | 2D sprite, server-broadcast position | `BP_OtherPlayerCharacter` (Blueprint actor) | `AMMORemotePlayer` (C++ ACharacter) |
| Enemy / Monster | 2D sprite, server AI movement | `BP_EnemyCharacter` (Blueprint actor) | `AMMOEnemyActor` (C++ ACharacter) |
| NPC | Static 2D sprite, clickable | `KafraNPC` (C++ actor), various BP NPCs | Same (already C++ where needed) |
| Warp Portal | Invisible trigger zone | `WarpPortal` (C++ actor) | Same (already C++) |
| Ground Item | 2D sprite on ground, pickable | Not yet implemented | Future system |

---

## Appendix C: Camera System Reference

While the camera system is not a multiplayer or entity system, it closely interacts with targeting, name tags, and range checking. Reference values:

- **Camera type:** Third-person boom camera (spring arm + camera component)
- **Default pitch:** Fixed downward angle (approximately 35-45 degrees, configurable)
- **Yaw rotation:** Right-click-drag rotates the camera around the character (yaw only in most implementations)
- **Zoom:** Mouse wheel adjusts boom length (camera distance). Clamped between a minimum (close-up) and maximum (far overview) distance.
- **Original RO camera:** Fixed isometric angle with very limited rotation (only 8 cardinal/diagonal directions in some clients). The 2D sprite-based rendering makes true free camera rotation impractical. Modern 3D recreations (including Sabri_MMO) allow free yaw rotation.
- **Camera during auto-attack:** Camera does not automatically rotate to follow the combat. The player manually controls the camera.
- **Camera on death:** Camera typically stays in place (no special death camera behavior in original RO).
