# Full Verification Checklist — All 82 Commits

**Last Updated**: 2026-03-18
**Scope**: Every client-visible feature across the entire project history

Prerequisites: Server running (`cd server && npm run dev`), editor open, character created.

---

## 1. Login Flow (commits e4b61a2 → da6c20d)

### Login Screen
- [ ] Username + password fields visible
- [ ] "Remember Me" checkbox works
- [ ] Tab key moves between fields
- [ ] Enter key submits login
- [ ] Error message shows for wrong credentials
- [ ] Successful login transitions to server select

### Server Select
- [ ] Server list loads from REST API
- [ ] Population count visible
- [ ] Server status (online/offline) indicator
- [ ] Click to select, double-click or button to connect

### Character Select
- [ ] 3x3 character card grid displays
- [ ] Character details panel (HP/SP, stats, class, level)
- [ ] Delete character with password confirmation
- [ ] Select character → enter world

### Character Create
- [ ] Name field (unique, case-insensitive)
- [ ] Gender toggle (male/female)
- [ ] Hair style picker (1-19)
- [ ] Hair color picker (0-8)
- [ ] Create → character appears in select screen

### Loading Overlay
- [ ] "Please Wait" overlay with progress bar during transitions
- [ ] Disappears after world loads

---

## 2. Movement & Camera (commits cd72d9e → f9c7917)

### Click-to-Move
- [ ] Left-click ground → character walks to point
- [ ] Path follows NavMesh (no walking through walls)
- [ ] Character faces movement direction

### WASD Movement
- [ ] IA_Move input works (if enabled)

### Camera
- [ ] Right-click drag rotates camera yaw
- [ ] Scroll wheel zooms in/out (200-1500 units)
- [ ] Fixed -55 degree pitch maintained
- [ ] Camera survives zone transitions

### Ground Snap
- [ ] Character spawns at ground level (not floating)
- [ ] After zone transition, character is on ground

---

## 3. Multiplayer (commits 79a78db → 707ae3d)

### Other Players
- [ ] Other players visible when they enter your zone
- [ ] Other player name tags visible (white text, 4-pass outline)
- [ ] Other players move smoothly (interpolation)
- [ ] Other players disappear on disconnect/zone change

### Position Sync
- [ ] Your position broadcasts at 30Hz
- [ ] Other players see your movement in near-real-time

### Zone Transitions
- [ ] Walk into warp portal → loading screen → new zone
- [ ] Socket stays connected (no disconnect/reconnect)
- [ ] Buffs persist across zone changes
- [ ] Other players in new zone appear immediately

---

## 4. Combat System (commits a3c6aaa → 2afce20)

### Auto-Attack
- [ ] Click enemy → character walks to range → starts attacking
- [ ] Attack animation plays
- [ ] Damage numbers appear (white for normal, yellow for crit)
- [ ] Enemy HP bar decreases
- [ ] ASPD determines attack speed

### Enemy AI
- [ ] Passive enemies wander, attack when hit
- [ ] Aggressive enemies chase on sight
- [ ] Enemies assist same-type allies (within range)
- [ ] Hit stun pauses enemy movement
- [ ] Enemies give up chase beyond chase range
- [ ] Random target switching (bosses)

### Death & Respawn
- [ ] Player dies → death overlay appears (Z=40)
- [ ] Click respawn → teleport to save point, full HP
- [ ] **Death penalty**: EXP bar should decrease by 1% on PvE death
- [ ] **Novice exempt**: Novice class should NOT lose EXP on death
- [ ] Enemy death → death animation, drops loot

### Target Frame
- [ ] Click enemy → target frame shows (name, HP bar)
- [ ] Target frame updates as enemy takes damage
- [ ] Target frame clears on enemy death or deselect

### Damage Numbers
- [ ] White damage numbers for normal hits
- [ ] Yellow for critical hits
- [ ] Green for heals
- [ ] Red for poison/bleed DoT ticks
- [ ] "MISS" text for missed attacks
- [ ] Numbers float up and fade

---

## 5. Stats & Leveling (commits 5bd450e → 56d6133)

### Stat Allocation
- [ ] Open stats panel (F8)
- [ ] Six stats visible: STR, AGI, VIT, INT, DEX, LUK
- [ ] Plus buttons to allocate stat points
- [ ] Cost increases with stat value (RO formula)
- [ ] Derived stats update: ATK, MATK, DEF, MDEF, HIT, FLEE, ASPD, CRIT

### EXP & Leveling
- [ ] Kill enemy → EXP gain notification
- [ ] Base EXP bar fills
- [ ] Job EXP bar fills
- [ ] Level up → full heal, stat points granted, announcement
- [ ] Job level up → skill points granted

### Basic Info Widget (Z=10)
- [ ] HP bar (red), SP bar (blue), Base EXP bar (yellow), Job EXP bar (orange)
- [ ] Draggable panel
- [ ] Values update in real-time
- [ ] Character name and level displayed

---

## 6. Skills (commits ea9e4f7 → 2afce20)

### Skill Tree (K key)
- [ ] Opens skill tree panel
- [ ] Shows class-appropriate skills
- [ ] Skill prerequisites shown (lines/arrows)
- [ ] Click to learn skill (if points available)
- [ ] Skill tooltip on hover (description, damage, SP cost, cast time per level)
- [ ] Drag skill to hotbar

### Casting
- [ ] Click-to-cast targeting (single target skills)
- [ ] Ground AoE targeting circle (Thunderstorm, etc.)
- [ ] Cast bar appears during cast time
- [ ] DEX reduces cast time
- [ ] Cast interrupted by stun/freeze
- [ ] SP deducted on cast COMPLETE (not start)

### Skill VFX
- [ ] Bolt skills: bolts from sky hit enemy
- [ ] Fire Ball: projectile + explosion
- [ ] Soul Strike: multi-hit projectiles
- [ ] Thunderstorm: ground rain effect
- [ ] Fire Wall: persistent fire barrier
- [ ] Heal: green light burst
- [ ] Buff skills: persistent VFX on buffed target

### Walk-to-Cast
- [ ] Out of range → character walks to range, then casts
- [ ] Works for both single-target and ground AoE

---

## 7. Hotbar (commits 7c4dab5 → 28c9181)

- [ ] F5 cycles visibility (show row 1 → show rows 1+2 → hide)
- [ ] 4 rows x 9 slots
- [ ] Drag skill from skill tree → hotbar slot
- [ ] Drag item from inventory → hotbar slot
- [ ] Press 1-9 to activate row 1 slots
- [ ] Alt+1-9 for row 2
- [ ] Keybind customization panel
- [ ] Hotbar persists (server-saved)

---

## 8. Inventory & Equipment (commits d2ae909 → cd81ab6)

### Inventory (F6)
- [ ] Grid of item icons
- [ ] Hover tooltip (name, description, stats, weight)
- [ ] Right-click item → inspect popup (Z=22)
- [ ] Double-click consumable → use
- [ ] Drag item to equipment slot → equip
- [ ] Weight display (current/max)

### Equipment (F7)
- [ ] 10 equipment slots visible (head, body, weapon, shield, etc.)
- [ ] Equip weapon → ATK changes
- [ ] Equip armor → DEF changes
- [ ] Dual wield for Assassin (left hand slot)
- [ ] Ammo slot for archers
- [ ] Card slot diamonds visible on equipment

### Card Compound
- [ ] Double-click card → compound popup (Z=23)
- [ ] Shows eligible equipment list
- [ ] Select equipment → compound card into it
- [ ] Card name appears in item tooltip

### Item Inspect (Z=22)
- [ ] Right-click item → inspect window
- [ ] Shows: title, icon, description, card slots
- [ ] Close button works

### Loot Notifications
- [ ] Kill enemy → loot notification appears (bottom-right)
- [ ] Shows item icon, name, quantity
- [ ] Fades after 4 seconds
- [ ] Max 8 notifications stacked

---

## 9. Buffs & Status Effects (commits bc7d86c → 56d6133)

### Buff Bar (Z=11)
- [ ] Active buffs shown as text abbreviations with countdown
- [ ] Buff appears on cast (Blessing, AGI Up, Endure, etc.)
- [ ] Countdown decrements
- [ ] Buff disappears on expiry
- [ ] Buffs persist across zone changes

### Status Effects
- [ ] Stun: character can't move/attack/cast
- [ ] Freeze: character frozen in place
- [ ] Poison: periodic HP drain (green number)
- [ ] Blind: accuracy reduced
- [ ] Silence: can't use skills
- [ ] Stone Curse: two-phase petrification

### Buff/Debuff VFX
- [ ] Frost Diver freeze VFX on target
- [ ] Stone Curse petrification VFX
- [ ] Sight/Ruwach reveal aura

---

## 10. Chat System (commits a3c6aaa → 2afce20)

### Global Chat
- [ ] Type message → press Enter → appears in chat
- [ ] Other players see your messages
- [ ] Own messages show immediately (local echo)

### Party Chat
- [ ] `%message` sends to party only
- [ ] Party messages in blue

### Whisper (NEW — this session)
- [ ] `/w "PlayerName" hello` sends whisper
- [ ] Received whisper: `( From PlayerName: ) hello` in yellow
- [ ] Sent whisper: `( To PlayerName: ) hello` in yellow
- [ ] `/w OfflinePlayer msg` → error: "There is no such character name or the user is offline."
- [ ] `/ex PlayerName` → blocks whispers from that player
- [ ] `/exall` → blocks all whispers
- [ ] `/in PlayerName` → unblocks
- [ ] `/inall` → unblocks all
- [ ] `/am Away message` → auto-reply set
- [ ] `/r hello` → replies to last whisperer

### Combat Log
- [ ] Click "Combat" tab in chat
- [ ] Shows: auto-attack damage, skill damage, buff/debuff applied/removed, death/respawn
- [ ] Filters by local player only

### Chat Tabs
- [ ] "All" tab shows everything
- [ ] "System" tab shows system messages
- [ ] "Combat" tab shows combat events

---

## 11. Party System (commit bc7d86c area)

### Party Management
- [ ] Create party (via command or UI)
- [ ] Invite player by name
- [ ] Accept/decline invite popup
- [ ] Party widget (Z=12) shows member list
- [ ] Member HP bars visible (green, red at 25%)
- [ ] Leader crown icon
- [ ] Context menu: kick, delegate leader, leave
- [ ] Party dissolves when leader leaves (or delegates)

### Party EXP
- [ ] Even Share: EXP split by level ratio (+20% bonus per member)
- [ ] Members within 15 levels share EXP

### Party Chat
- [ ] `%message` routes to party
- [ ] Party members see blue messages

---

## 12. Shop System (commit 94c4ba0)

- [ ] Click Shop NPC → shop window opens (Z=18)
- [ ] Buy tab: list of items with prices
- [ ] Sell tab: your inventory items with sell prices
- [ ] Buy/sell batch operations
- [ ] Discount/Overcharge skill bonuses apply
- [ ] Zeny deducted/added correctly

---

## 13. Kafra System (commit 4097156)

- [ ] Click Kafra NPC → Kafra dialog opens (Z=19)
- [ ] Save point option
- [ ] Teleport service (list of saved locations)
- [ ] Cancel button closes dialog

---

## 14. Zone System (commits 4097156 → 707ae3d)

### Warp Portals
- [ ] Walk into warp portal → zone transition
- [ ] Loading overlay during transition
- [ ] Spawn at correct position in new zone
- [ ] Enemies in new zone appear

### Current Zones (4)
- [ ] `prontera` — Town (no enemies, NPCs present)
- [ ] `prontera_south` — Starter field (low-level enemies)
- [ ] `prontera_north` — Higher field (mid-level enemies)
- [ ] `prt_dungeon_01` — Dungeon (indoor enemies)

---

## 15. Name Tags (commit 707ae3d)

- [ ] Player names always visible (white text)
- [ ] Monster names visible on hover only
- [ ] NPC names visible on hover (light blue)
- [ ] 4-pass black outline for readability
- [ ] Level-based colors for monsters (grey/white/red)

---

## 16. Targeting (commit f9c7917)

- [ ] Hover enemy → crosshair cursor
- [ ] Hover NPC → text cursor
- [ ] Hover indicator widget on target
- [ ] Targeting pauses during skill targeting mode

---

## 17. Monster Skills & MVP (commit 2afce20 — NEW)

### Monster Skills
- [ ] Zombie uses NPC_STUNATTACK (stuns player)
- [ ] Hornet uses NPC_POISON (poisons player)
- [ ] Familiar uses NPC_BLINDATTACK
- [ ] Creamy uses AL_TELEPORT (teleports away when chased)
- [ ] Monsters with cast times show cast bar

### Cast Sensor
- [ ] Monsters with castSensorIdle (AI code 9/21) aggro when you start casting nearby

### MVP System (test when MVPs are spawned)
- [ ] MVP death → server-wide `[MVP] PlayerName has defeated BossName!`
- [ ] Highest damage dealer gets MVP bonus EXP
- [ ] MVP drops rolled for winner
- [ ] MVP respawns in 120-130 minutes (variance)

---

## 18. Death Penalty (commit 2afce20 — NEW)

- [ ] Die to monster → lose 1% base EXP + 1% job EXP
- [ ] EXP bar visibly decreases
- [ ] Novice class: NO EXP loss on death
- [ ] EXP cannot go below 0 (no deleveling)
- [ ] Die from poison/bleeding DoT → same penalty
- [ ] PvP death (when implemented): NO penalty

---

## 19. Non-Elemental Attacks (commit 2afce20 — NEW)

- [ ] Monster basic attacks deal full damage regardless of armor element
- [ ] Player Neutral attacks are affected by armor element table
- [ ] Ghostring Card (Ghost armor): reduces player attacks, does NOT reduce monster attacks

---

## 20. Pet System (commit 2afce20 — NEW)

### Taming
- [ ] Use taming item on correct monster → capture attempt
- [ ] Success: monster disappears, egg in inventory
- [ ] Failure: taming item consumed, monster remains
- [ ] Lower HP = higher capture rate

### Hatching
- [ ] Use Pet Incubator (643) on egg → pet hatched
- [ ] Pet widget appears (Z=21)
- [ ] Pet placeholder actor spawns near player

### Feeding
- [ ] Click "Feed" → uses pet's specific food item
- [ ] Hunger bar increases
- [ ] Intimacy changes based on hunger bracket
- [ ] Overfeeding (when full) → -100 intimacy!
- [ ] Emote feedback: thumbs up (good) / hmm (overfed)

### Hunger/Intimacy
- [ ] Hunger decreases over time (every 60s)
- [ ] Starvation (hunger 0-10) → intimacy drops every 20s
- [ ] Intimacy reaches 0 → "Pet has run away!" (permanent loss)
- [ ] Owner death → -20 intimacy

### Pet Bonuses
- [ ] At Cordial (750+) intimacy → stat bonuses active
- [ ] Poring: LUK +2, CRIT +1
- [ ] Orc Warrior: ATK +10, DEF -3
- [ ] Check stats panel for bonus changes

### Pet Commands
- [ ] "Return to Egg" → pet deactivates, actor despawns
- [ ] Rename pet (once)

### Pet Following
- [ ] Pet placeholder follows behind player
- [ ] Smooth interpolation movement
- [ ] Despawns on return/ran_away

---

## 21. Homunculus Client (commit 2afce20 — NEW)

### Summoning
- [ ] Alchemist uses Call Homunculus → `homunculus:summoned` event
- [ ] Homunculus widget appears (Z=22)
- [ ] Placeholder actor spawns on opposite side from pet
- [ ] Name, type, level displayed

### Status Bars
- [ ] HP bar (red)
- [ ] SP bar (blue)
- [ ] Hunger bar (green)
- [ ] All update in real-time

### Commands
- [ ] "Feed" button → feeds homunculus
- [ ] "Vaporize (Rest)" → homunculus returns (requires HP >= 80%)
- [ ] Actor despawns on vaporize/death

---

## 22. Companion Visuals (commit 2afce20 — NEW)

### Cart (Merchant class)
- [ ] Rent cart → brown box appears behind player
- [ ] Box trails with smooth interpolation
- [ ] Remove cart → box disappears

### Mount (Knight/Crusader)
- [ ] Toggle mount → golden cylinder appears under player
- [ ] Cylinder follows player position directly
- [ ] Dismount → cylinder disappears

### Falcon (Hunter)
- [ ] Learn Falconry Mastery → grey sphere appears near shoulder
- [ ] Sphere has gentle bobbing motion
- [ ] Persists while hasFalcon is true

---

## 23. Cart & Vending (prior commits)

### Cart System
- [ ] Rent cart from Kafra (800z)
- [ ] Move items to/from cart (100 slots, 8000 weight)
- [ ] Cart speed penalty visible in movement

### Vending
- [ ] Open vending shop (Merchant with cart)
- [ ] Set item prices
- [ ] Other players can browse and buy
- [ ] Auto-close when sold out

### Identification
- [ ] Equipment drops as unidentified
- [ ] Use Magnifier → identify flow
- [ ] Can't equip/refine/compound unidentified items

---

## 24. Refining & Forging (prior commits)

### Refining
- [ ] Refine weapon → +ATK per level (+2/+3/+5/+7 by weapon level)
- [ ] Safe limits (varies by weapon level)
- [ ] Failure above safe limit → weapon destroyed
- [ ] Overupgrade random bonus on hit

### Forging (Blacksmith)
- [ ] Forge weapon from materials
- [ ] Element stones add element
- [ ] Star crumbs add bonus
- [ ] Success rate formula

---

## 25. Ammunition (prior commits)

- [ ] Equip arrows in ammo slot (Archer)
- [ ] Arrow element overrides weapon element
- [ ] 1 arrow consumed per auto-attack
- [ ] 1 arrow consumed per ranged skill
- [ ] Status arrows proc effects (fire, ice, etc.)

---

## 26. Weight System (prior commits)

- [ ] Weight displayed in inventory
- [ ] 50%+ weight: HP/SP regen stops
- [ ] 90%+ weight: can't attack or use skills
- [ ] 100%+ weight: can't pick up loot

---

## 27. Dual Wield (Assassin — prior commits)

- [ ] Equip dagger in right hand
- [ ] Equip second dagger/1H sword in left hand
- [ ] Both weapons hit per auto-attack cycle
- [ ] Left hand: always hits, never crits
- [ ] Stats panel shows ATK(R) / ATK(L)
- [ ] ASPD cap 190 (vs 195 single wield)

---

## 28. Class System Verification

### First Classes (6)
- [ ] Swordsman: Bash, Magnum Break, Endure, Provoke
- [ ] Mage: Fire/Cold/Lightning Bolt, Fire Ball, Soul Strike, Frost Diver, Stone Curse
- [ ] Archer: Double Strafe, Arrow Shower, Arrow Repel
- [ ] Acolyte: Heal, Blessing, Inc AGI, Dec AGI, Cure, Teleport, Pneuma
- [ ] Thief: Steal, Hiding, Envenom, Backslide
- [ ] Merchant: Mammonite, Cart Revolution

### Second Classes (13) — verify skill tree shows correct skills
- [ ] Knight, Crusader, Wizard, Sage, Hunter
- [ ] Bard, Dancer, Priest, Monk, Assassin
- [ ] Rogue, Blacksmith, Alchemist

### Job Change
- [ ] Novice → first class at Job Level 10 (requires Basic Skill Lv9)
- [ ] First → second class at Job Level 40+

---

## Quick Smoke Test (5 minutes)

If short on time, verify these critical paths:

1. [ ] Login → select character → enter world
2. [ ] Move by clicking ground
3. [ ] Attack an enemy → damage numbers appear → enemy dies → EXP gained → loot drops
4. [ ] Open inventory (F6) → items visible
5. [ ] Open skill tree (K) → learn a skill → drag to hotbar → press key to cast
6. [ ] Type in chat → message appears
7. [ ] `/w "YourName" test` → whisper error (self-whisper blocked)
8. [ ] Die to enemy → death overlay → respawn → check EXP decreased
9. [ ] Zone transition via warp portal → new zone loads
