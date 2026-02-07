# Ragnarok Online — Complete Game Design Reference

> **Purpose**: This document serves as the authoritative reference for building Sabri_MMO to play like Ragnarok Online (RO). Every system, mechanic, and feature should be modeled after RO's design philosophy unless explicitly stated otherwise.

---

## Table of Contents

1. [Core Design Philosophy](#core-design-philosophy)
2. [Character Stats System](#character-stats-system)
3. [Combat System](#combat-system)
4. [Targeting & Auto-Attack](#targeting--auto-attack)
5. [Attack Speed (ASPD)](#attack-speed-aspd)
6. [Damage Calculation](#damage-calculation)
7. [Skills System](#skills-system)
8. [Job/Class System](#jobclass-system)
9. [Equipment System](#equipment-system)
10. [Monster/NPC System](#monsternpc-system)
11. [PvP System](#pvp-system)
12. [Party System](#party-system)
13. [Guild System](#guild-system)
14. [Map & Zone System](#map--zone-system)
15. [Economy & Trading](#economy--trading)
16. [UI/HUD Design](#uihud-design)
17. [Chat System](#chat-system)
18. [Death & Respawn](#death--respawn)
19. [Leveling & Experience](#leveling--experience)
20. [Cards & Enchantment System](#cards--enchantment-system)
21. [Quest System](#quest-system)
22. [Storage & Inventory](#storage--inventory)
23. [Server Architecture Implications](#server-architecture-implications)
24. [Implementation Priority](#implementation-priority)

---

## Core Design Philosophy

Ragnarok Online is a **click-to-move, auto-attack MMORPG** with the following core principles:

- **Simple controls, deep systems**: Left-click to move/attack, right-click for context. Depth comes from builds, gear, and skill usage.
- **Sticky targeting**: Click once to engage; your character handles the rest (pathing, following, attacking).
- **Server-authoritative**: All combat calculations happen server-side. Client sends intents, server validates.
- **Stat-driven combat**: Everything is math — damage, hit chance, dodge chance, attack speed, cast time — all derived from stats and gear.
- **Social gameplay**: Parties, guilds, trading, sitting, chatting — the world is a social space.
- **Grind-based progression**: Kill monsters → gain EXP → level up → allocate stat points → become stronger.
- **Risk vs reward**: Stronger monsters give more EXP/loot but can kill you. Death has penalties (EXP loss).

---

## Character Stats System

### Base Stats (6 Primary Stats)

Every character has 6 stats that the player allocates points into when leveling up:

| Stat | Abbreviation | Primary Effect | Secondary Effects |
|------|-------------|----------------|-------------------|
| **Strength** | STR | Physical attack power (melee) | Weight capacity, status ATK |
| **Agility** | AGI | Attack speed (ASPD), Flee (dodge) | Movement doesn't change in RO but AGI = core for ASPD |
| **Vitality** | VIT | Max HP, HP recovery, physical defense | Status resistance, healing effectiveness |
| **Intelligence** | INT | Magic attack (MATK), Max SP/Mana | SP recovery, magic defense, cast time reduction |
| **Dexterity** | DEX | Hit rate (accuracy), cast time reduction | Minimum attack power, slight ASPD bonus, ranged ATK |
| **Luck** | LUK | Critical hit rate, perfect dodge | Status resistance, ATK bonus (small) |

### Stat Point Allocation

- **Per level**: Player receives a fixed number of stat points (typically starts at ~5-9 per level at low levels)
- **Increasing cost**: Raising a stat costs more points at higher values
  - Stat 1→2: 2 points
  - Stat 2→3: 3 points
  - Stat 10→11: 11 points
  - Cost to raise stat from N to N+1 = `floor((N+1)/10) + 2` (approximate — varies by implementation)
- **Max base stat**: 99 (before bonuses from gear)
- **Stats are permanent** (unless a stat reset item is used)

### Derived Stats (Calculated from Base Stats + Equipment)

| Derived Stat | Formula (Simplified) | Description |
|-------------|----------------------|-------------|
| **ATK** | `STR + WeaponATK + BonusATK` | Physical attack power |
| **MATK** | `INT + WeaponMATK + BonusMATK` | Magic attack power |
| **DEF** | `VIT_based + EquipDEF` | Physical damage reduction |
| **MDEF** | `INT_based + EquipMDEF` | Magic damage reduction |
| **HIT** | `BaseLv + DEX + BonusHIT` | Accuracy (must exceed target FLEE to hit) |
| **FLEE** | `BaseLv + AGI + BonusFLEE` | Dodge rate |
| **ASPD** | `200 - (BaseASPD - AGI*4 - DEX*0.2)` | Attack speed (higher = faster, cap 190) |
| **Critical** | `LUK * 0.3 + BonusCRI` | Critical hit chance (%) |
| **Perfect Dodge** | `LUK * 0.1` | Chance to dodge any attack (ignores HIT) |
| **Max HP** | `BaseHP(Lv, Class) + VIT * HPPerVIT` | Maximum hit points |
| **Max SP** | `BaseSP(Lv, Class) + INT * SPPerINT` | Maximum skill points (mana) |
| **HP Regen** | `VIT_based + Bonus` | HP regenerated per tick (every 10 seconds) |
| **SP Regen** | `INT_based + Bonus` | SP regenerated per tick |
| **Cast Time** | `BaseCast * (1 - DEX/150 - INT/...)` | Time to cast a skill |

---

## Combat System

### Overview

RO combat is **auto-attack based** with **skill interrupts**. The flow:

1. Player **left-clicks** an enemy (monster or player in PvP zone)
2. Character **paths toward** the target automatically
3. When **in range**, character begins **auto-attacking** at their ASPD rate
4. Player can **use skills** (hotbar) during auto-attack, which interrupts the attack animation
5. Auto-attack **resumes** after skill animation completes
6. If target **moves out of range**, character **follows** automatically
7. Combat continues until:
   - Target dies
   - Player clicks elsewhere (ground or another target)
   - Player presses a movement key
   - Player uses an escape skill

### Attack Types

| Type | Range | Stat Used | Examples |
|------|-------|-----------|---------|
| **Melee** | ~1-2 tiles (close range) | STR + WeaponATK | Sword, Axe, Mace, Dagger, Knuckle, Spear (close) |
| **Ranged** | Weapon-dependent (4-14 tiles) | DEX + WeaponATK | Bow (long), Gun, Spear (thrown) |
| **Magic** | Skill-dependent | INT + MATK | Bolt spells, AoE spells |

### Hit/Miss System

- **Hit check**: `HIT - FLEE = % chance to hit` (roughly)
  - If HIT > FLEE: guaranteed hit (95% cap, 5% minimum miss)
  - If FLEE > HIT: increasing miss chance
- **Perfect Dodge**: Separate check — LUK-based, bypasses HIT calculation
- **Critical Hits**: Bypass FLEE entirely (always hit), deal 40% bonus damage, no DEF reduction
- **Miss**: Displays "Miss" text above target, no damage dealt

### Combat Feedback (Visual)

- **Damage numbers** float up from target when hit (white for normal, yellow for critical)
- **"Miss"** text when attack misses
- **"CRITICAL"** text on critical hits
- **Hit animation** on target (brief red flash or stagger)
- **Attack animation** on attacker (swing/stab based on weapon type)
- **HP bar** above target decreases in real-time

---

## Targeting & Auto-Attack

### Targeting Flow (CRITICAL — This is the core interaction)

```
HOVER over enemy:
  → Show enemy name below them
  → Show cursor change (sword cursor or attack cursor)
  → Show small targeting indicator (arrow pointing down above head)

LEFT-CLICK on enemy:
  → Set as current target
  → Targeting arrow stays permanently
  → Character begins pathing toward target
  → When in range: auto-attack begins
  → Attack repeats at ASPD interval
  → If target moves: follow and continue attacking

CANCEL auto-attack by:
  → Left-clicking ground (move command)
  → Left-clicking different target (switch target)
  → Pressing Escape
  → Character dies
  → Target dies
```

### Hover Effects (Before Clicking)

- **Cursor change**: Normal cursor → Attack cursor (sword icon) when hovering over enemy
- **Name display**: Enemy name appears below them (monster name + level, or player name)
- **Arrow indicator**: Small downward-pointing arrow above their head
- **All disappear** when cursor moves away from the entity

### Active Target Effects (After Clicking)

- **Arrow stays**: Targeting arrow remains above target's head during combat
- **Character follows**: Auto-pathfinds to stay in attack range
- **Auto-attack loop**: Attacks fire at ASPD interval without further input
- **HP bar visible**: Target's HP bar shown (either above their head or in a target frame on HUD)

### Target Frame (HUD Element)

In RO, when you click a monster, a small info window appears showing:
- Monster/player name
- HP bar (for monsters you can see approximate HP)
- Level (for monsters)

### Range Behavior

- **Melee**: Character walks up to target until adjacent (~150 Unreal units)
- **Ranged**: Character walks until within weapon range, then stops and attacks
- **If target moves during attack**: Character finishes current attack animation, then follows
- **Re-engage**: Once in range again, auto-attack resumes immediately

---

## Attack Speed (ASPD)

### How ASPD Works in RO

- **ASPD** is a value from **0 to 190** (cap)
- Higher ASPD = faster attacks
- **Delay between attacks** = `200 - ASPD` (in "ticks" — roughly translates to milliseconds × 10)
  - ASPD 150 → 50 ticks → ~500ms between attacks (2 attacks/sec)
  - ASPD 170 → 30 ticks → ~300ms between attacks (3.3 attacks/sec)
  - ASPD 190 → 10 ticks → ~100ms between attacks (10 attacks/sec, cap)
  - ASPD 130 → 70 ticks → ~700ms between attacks (1.4 attacks/sec)

### ASPD Calculation (Simplified)

```
BaseASPD = WeaponBaseDelay (depends on weapon type + class)
ASPD = 200 - (BaseASPD - floor(AGI * 4 + DEX * 0.2)) / 10
ASPD = min(ASPD, 190)  // hard cap
```

### Weapon Base Delays (Examples)

| Weapon Type | Base Delay | Typical ASPD Range |
|-------------|------------|-------------------|
| Dagger | Low (fast) | 160-185 |
| Sword (1H) | Medium | 150-175 |
| Sword (2H) | High (slow) | 140-165 |
| Axe (1H) | Medium-High | 145-165 |
| Axe (2H) | Very High | 130-155 |
| Mace | Medium | 150-170 |
| Spear (1H) | Medium | 145-170 |
| Spear (2H) | High | 135-160 |
| Bow | Medium | 150-175 |
| Knuckle/Fist | Very Low (fastest) | 170-190 |
| Staff/Rod | Very High (slowest) | 125-150 |
| Gun | Medium | 155-175 |
| Katana | Medium | 150-170 |

### ASPD for Our Implementation

- Store `baseASPD` per weapon type in a config/database table
- Calculate actual ASPD from AGI + DEX + weapon + buffs
- Convert to attack interval in milliseconds: `attackInterval = (200 - ASPD) * 50` ms (tune as needed)
- Server validates attack timing (prevents speed hacking)
- Client uses interval for animation timing

---

## Damage Calculation

### Physical Damage (Melee/Ranged)

```
Base ATK = StatusATK + WeaponATK
StatusATK = STR + floor(STR/10)^2 + DEX/5 + LUK/3  (melee)
           or DEX + floor(DEX/10)^2 + STR/5 + LUK/3  (ranged)

Raw Damage = ATK * SkillModifier
Damage = Raw Damage - TargetDEF

Variance: ATK has a range [MinATK, MaxATK]
  MinATK = StatusATK + WeaponATK * 0.8 (approximately)
  MaxATK = StatusATK + WeaponATK * 1.0

Final Damage = max(1, Damage)  // minimum 1 damage
```

### Magic Damage

```
MATK = INT + floor(INT/7)^2 + WeaponMATK
Damage = MATK * SkillModifier - TargetMDEF
```

### Critical Hits

```
Critical Damage = ATK * 1.4 (40% bonus)
Critical ignores: target FLEE, target DEF (hard DEF portion)
Critical chance = LUK * 0.3 + BonusCRI - TargetLUK * 0.2
```

### Elemental Modifiers

| Attacking Element vs Defending Element | Modifier |
|---------------------------------------|----------|
| Fire vs Earth | 200% |
| Water vs Fire | 200% |
| Earth vs Wind | 200% |
| Wind vs Water | 200% |
| Holy vs Shadow | 200% |
| Shadow vs Holy | 100% (in some versions) |
| Same element | 0% (immune) |
| Neutral vs all | 100% (normal) |

### Size Modifiers

| Weapon Type | vs Small | vs Medium | vs Large |
|-------------|----------|-----------|----------|
| Dagger | 100% | 75% | 50% |
| Sword (1H) | 75% | 100% | 75% |
| Sword (2H) | 75% | 75% | 100% |
| Axe (1H) | 50% | 75% | 100% |
| Spear (1H) | 75% | 75% | 100% |
| Bow | 100% | 100% | 75% |
| Staff | 100% | 100% | 100% |

---

## Skills System

### Skill Types

| Type | Targeting | Description | Example |
|------|-----------|-------------|---------|
| **Passive** | None | Always active, no activation | Sword Mastery (+ATK with swords) |
| **Active - Self** | Self | Cast on yourself | Increase AGI (buff) |
| **Active - Target** | Single enemy/ally | Click skill then click target | Bash (damage), Heal (restore HP) |
| **Active - Ground** | Area on ground | Click skill then click location | Storm Gust (AoE at location) |
| **Active - Self AoE** | Around caster | Cast centered on self | Magnum Break (AoE around caster) |
| **Toggle** | Self | On/Off persistent effect | Auto Guard (chance to block) |

### Skill Usage Flow

```
1. Player presses hotkey (F1-F9, or clicks skill bar)
2. Skill type determines next action:
   - SELF: Cast immediately on self
   - TARGET: Cursor changes to targeting mode → click enemy/ally
   - GROUND: Cursor changes to AoE targeting → click ground location
3. Cast time begins (if any) — shown as cast bar above character
4. SP (mana) consumed when cast begins
5. Skill effect triggers after cast time completes
6. Cooldown begins (if any)
7. After-cast delay (global) before next skill can be used
```

### Skill Properties

| Property | Description |
|----------|-------------|
| **SP Cost** | Mana consumed to use the skill |
| **Cast Time** | Time to channel before effect (interruptible by damage in some versions) |
| **After-Cast Delay** | Global cooldown after casting before any skill can be used again |
| **Cooldown** | Individual skill cooldown (some skills have this) |
| **Range** | Maximum distance to target/ground point |
| **Area of Effect** | Size of the AoE (for ground/self AoE skills) |
| **Skill Level** | 1-10 typically, higher = more damage/effect + more SP cost |
| **Element** | Some skills have a fixed element (Fire Bolt = Fire) |
| **Damage Modifier** | % of ATK/MATK used as base damage |

### Skill Hotbar

- **F1-F9** (or 1-9) for quick skill activation
- Skills can be dragged from skill window to hotbar slots
- Items can also go in hotbar slots (potions, etc.)
- Multiple hotbar rows can be cycled (F12 to switch bars)

---

## Job/Class System

### RO Class Progression

```
Novice (Lv 1-10)
  ├── Swordsman → Knight / Crusader → Lord Knight / Paladin
  ├── Mage → Wizard / Sage → High Wizard / Professor
  ├── Archer → Hunter / Bard/Dancer → Sniper / Clown/Gypsy
  ├── Thief → Assassin / Rogue → Assassin Cross / Stalker
  ├── Acolyte → Priest / Monk → High Priest / Champion
  └── Merchant → Blacksmith / Alchemist → Whitesmith / Creator
```

### Class Design Principles

- Each class has a **unique skill tree** (20-40 skills)
- **Base class** teaches fundamentals (Swordsman: basic sword skills)
- **Advanced class** specializes (Knight: mount combat, Crusader: holy/defense)
- **Transcendent class** adds powerful skills on top of advanced
- **Stat build** varies within same class (AGI Knight vs STR Knight vs Hybrid)

### For Our Implementation

- Start with 1-2 base classes
- Design skill trees that are extensible
- Store class data in database tables
- Skills reference class_id for unlock requirements

---

## Equipment System

### Equipment Slots

| Slot | Examples |
|------|---------|
| **Weapon** (Right Hand) | Sword, Dagger, Staff, Bow, Axe |
| **Shield** (Left Hand) | Buckler, Guard, Shield (only with 1H weapon) |
| **Head (Upper)** | Helm, Hat, Crown |
| **Head (Mid)** | Glasses, Monocle |
| **Head (Lower)** | Mask, Pipe, Scarf |
| **Armor** | Plate, Robe, Suit |
| **Garment** (Cape/Mantle) | Muffler, Hood, Manteau |
| **Footgear** (Shoes) | Boots, Sandals, Shoes |
| **Accessory 1** | Ring, Clip, Rosary, Glove |
| **Accessory 2** | Ring, Clip, Rosary, Glove |

### Equipment Properties

- **Base stats**: ATK, DEF, MATK, MDEF, etc.
- **Weight**: Contributes to carry weight
- **Required level**: Minimum base level to equip
- **Class restriction**: Only certain classes can equip certain items
- **Slots**: Equipment can have 0-4 card slots
- **Refinement**: +1 to +10 (increases stats, higher = risk of breaking)
- **Element**: Weapons/armor can have elemental properties

### Card System (Slot into Equipment)

- Cards drop from monsters (rare)
- Each card gives specific bonuses when slotted
- Card effects stack with equipment base stats
- Cards are permanent once slotted (unless special NPC removes them)

---

## Monster/NPC System

### Monster Properties

| Property | Description |
|----------|-------------|
| **Name** | Display name |
| **Level** | Monster's level |
| **HP** | Hit points |
| **Base EXP** | EXP given on kill |
| **Job EXP** | Job EXP given on kill |
| **ATK Range** | [Min, Max] attack damage |
| **DEF/MDEF** | Defenses |
| **Element** | Monster's element + level (Fire Lv 2, etc.) |
| **Size** | Small, Medium, Large (affects weapon damage modifiers) |
| **Race** | Formless, Undead, Brute, Plant, Insect, Fish, Demon, Demi-Human, Angel, Dragon |
| **Movement Speed** | How fast it walks |
| **Attack Speed** | ASPD equivalent |
| **Attack Range** | Melee (1) or Ranged (3+) |
| **Skills** | Some monsters use skills |
| **Drops** | Item drop table with % chances |
| **Spawn Map** | Which maps they appear on |
| **Spawn Count** | How many on the map at once |
| **Respawn Time** | Time to respawn after killed |

### Monster AI Behaviors

| AI Type | Behavior |
|---------|----------|
| **Passive** | Wanders randomly, only attacks if attacked first |
| **Aggressive** | Attacks players on sight within detection range |
| **Looter** | Picks up items from ground |
| **Assist** | Helps nearby same-type monsters when they're attacked |
| **Cast** | Uses skills (healing, buffs, magic attacks) |
| **Boss** | Unique spawn, powerful, long respawn timer, special drops |
| **MVP** (Most Valuable Player) | Boss that grants MVP rewards to top damage dealer |

### NPC Types

| NPC Type | Function |
|----------|----------|
| **Shop** | Buy/sell items |
| **Storage** | Access personal storage |
| **Tool Dealer** | Sell consumables |
| **Weapon/Armor Dealer** | Sell equipment |
| **Kafra/Storage** | Teleport + storage services |
| **Quest NPC** | Give/complete quests |
| **Class Change NPC** | Change job/class |
| **Refiner** | Upgrade equipment (+1 to +10) |
| **Card Remover** | Remove cards from equipment |
| **Healer** | Fully heal HP/SP |

---

## PvP System

### PvP Zones

- **Normal Maps**: PvP **disabled** — cannot attack other players
- **PvP Maps**: Free-for-all PvP enabled upon entering
- **Guild vs Guild (GvG/WoE)**: Guild-organized castle sieges (scheduled events)
- **Duel**: 1v1 challenge system (both players must accept)
- **Arena**: Dedicated PvP arena maps

### PvP Rules

- **No EXP loss** on death in PvP maps (in most versions)
- **No item drop** on PvP death (in most versions)
- **Friendly fire**: Cannot attack party members
- **Guild protection**: Cannot attack guild members
- **Resurrection**: Can be resurrected by Priest skill in PvP

### War of Emperium (WoE/GvG)

- **Scheduled events**: Specific days/times (e.g., Wed/Sat 8-9 PM)
- **Castle sieges**: Guilds fight to control castles
- **Emperium**: Destroy the Emperium crystal to claim the castle
- **Castle benefits**: Owning guild gets treasure chests, guild dungeon access
- **Alliance system**: Guilds can form alliances

### For Our Implementation

- Add `pvpEnabled` flag per map/zone
- Server checks zone before allowing `combat:attack` on players
- Monster attacks work everywhere
- Future: GvG system as a major feature

---

## Party System

### Party Features

- **Max size**: 12 members
- **EXP sharing**: Split EXP among party members (configurable)
  - **Each Take**: Everyone gets full EXP (no sharing)
  - **Even Share**: EXP split evenly (only if within level range)
- **Item sharing**: Configurable (each take, party share, etc.)
- **Party buffs**: Some skills affect party members (Blessing, Increase AGI)
- **Party UI**: Party member list with HP bars
- **Party chat**: Dedicated chat channel

### Party Commands

- `/organize [party_name]` — Create party
- `/invite [player_name]` — Invite to party
- `/leave` — Leave party
- `/kick [player_name]` — Kick from party (leader only)

---

## Guild System

### Guild Features

- **Guild creation**: Requires specific item + zeny
- **Guild levels**: 1-50 (unlock more member slots, guild skills)
- **Max members**: Starts at 16, increases with guild level (up to 76)
- **Guild skills**: Guild-wide buffs during GvG
- **Guild emblem**: Custom image displayed on characters
- **Guild storage**: Shared storage accessible by members
- **Tax system**: % of member's zeny income goes to guild fund

### Guild Ranks

- **Guild Master**: Full control
- **Officers**: Can invite/kick
- **Members**: Basic access
- **Positions**: Customizable rank names and permissions

---

## Map & Zone System

### Map Structure

- **World map**: Grid of connected map instances
- **Map size**: Typically 300×300 to 512×512 tiles
- **Portals/Warps**: Transition points between maps (walk to edge or NPC warp)
- **Indoor maps**: Dungeons, buildings (separate instances)
- **Field maps**: Open world areas with monsters
- **Town maps**: Safe zones with NPCs, shops, no monsters

### Zone Properties

| Property | Description |
|----------|-------------|
| **PvP Enabled** | Can players attack each other? |
| **Monster Spawns** | Which monsters spawn here |
| **BGM** | Background music track |
| **Weather** | Rain, snow, night, fog effects |
| **Min Level** | Recommended minimum level |
| **Connected Maps** | Adjacent maps (portals) |
| **Safe Zone** | No combat at all (towns) |
| **Indoor/Outdoor** | Affects some skills and teleport |

### Map Types

- **Towns**: Prontera, Geffen, Payon, Alberta, etc. (safe zones, NPCs, shops)
- **Fields**: Open areas outside towns (monsters, resources)
- **Dungeons**: Multi-floor instanced areas (harder monsters, better loot)
- **PvP Arenas**: Dedicated combat zones
- **Castles**: GvG/WoE locations

---

## Economy & Trading

### Currency

- **Zeny**: Primary currency (gold coins)
- Monsters drop zeny on death
- NPC shops accept/give zeny
- Max: 2,000,000,000 zeny per character

### Trading

- **Player-to-player trade**: Open trade window, both confirm
- **Vending**: Set up a shop (sit + vend), other players browse and buy
- **Auction**: Auction house for rare items (some versions)
- **Mail**: Send items/zeny via in-game mail

### Item Rarity

| Rarity | Color | Examples |
|--------|-------|---------|
| Common | White | Potions, arrows, basic gear |
| Uncommon | Green | Better equipment |
| Rare | Blue | Slotted equipment, good cards |
| Epic | Purple | MVP drops, rare cards |
| Legendary | Orange | God items (WoE crafted) |

---

## UI/HUD Design

### Main HUD Elements (RO-Style)

```
┌─────────────────────────────────────────────────────────────┐
│ [Character Name]              [Mini-Map]                     │
│ HP ████████████████░░░░  4523/6000                           │
│ SP ████████████░░░░░░░░  234/500                             │
│ Base Lv: 75    Job Lv: 42                                    │
│ Base EXP: ████████░░░░  78.3%                                │
│ Job EXP:  ██████░░░░░░  62.1%                                │
│                                                               │
│ [Hotbar: F1][F2][F3][F4][F5][F6][F7][F8][F9]                │
│                                                               │
│                    [Game World]                                │
│                                                               │
│                                                               │
│ [Chat Window]                                                 │
│ > General | Party | Guild | Whisper                           │
│ [Player1]: Hello!                                             │
│ [Player2]: LFP for Orc Dungeon                               │
│ > [Type here...]                                              │
└─────────────────────────────────────────────────────────────┘
```

### HUD Components

| Component | Location | Content |
|-----------|----------|---------|
| **Status bars** | Top-left | HP, SP, EXP bars with numbers |
| **Character info** | Top-left | Name, Base Lv, Job Lv |
| **Mini-map** | Top-right | Current map overview, player dot, party dots |
| **Hotbar** | Bottom-center | 9 skill/item slots (F1-F9), switchable rows |
| **Chat window** | Bottom-left | Tabbed chat (General, Party, Guild, Whisper) |
| **Menu buttons** | Bottom-right | Inventory, Equipment, Skills, Quest, Map, Settings |
| **Target frame** | Near character | Target's name + HP bar when targeting |
| **Party frame** | Left side | Party member names + HP bars |
| **Buff bar** | Top or near HP | Active buff/debuff icons with timers |
| **Damage numbers** | Floating above targets | Damage dealt, healing, misses |
| **System messages** | Chat or center | Level up, item acquired, combat log |

### Floating UI Elements (In-World)

- **Player name** above character head (always visible)
- **Guild name** below player name (if in guild)
- **Monster name** shown on hover
- **Target arrow** above targeted entity
- **Damage numbers** float up and fade
- **Chat bubbles** above character when speaking (optional)
- **Emote bubbles** above character head

---

## Chat System

### Chat Channels

| Channel | Command | Visibility |
|---------|---------|------------|
| **General/Normal** | (just type) | All players on same map |
| **Party** | `/p [message]` | Party members only |
| **Guild** | `/g [message]` | Guild members only |
| **Whisper** | `/w [name] [message]` | Single target player |
| **Trade** | `/trade [message]` | All players (trade channel) |
| **Shout/Global** | `/shout [message]` | All players server-wide (costs zeny) |

### Chat Features

- **Chat bubbles**: Text appears above character in-world
- **Chat tabs**: Filter messages by channel
- **Chat history**: Scroll up to see previous messages
- **Block/Mute**: Ignore specific players
- **System messages**: Level up, item drops, combat results

---

## Death & Respawn

### Death Mechanics

- **On death**: Character falls down, cannot act
- **EXP penalty**: Lose 1% of current Base EXP (PvE deaths only)
- **No item drop**: Items stay in inventory (most versions)
- **Respawn options**:
  - Click "Return to Save Point" → respawn at last saved location (Kafra save)
  - Wait for Priest to Resurrect (restores some EXP penalty)
- **Death animation**: Character collapses on ground
- **Tombstone**: Appears at death location (optional)

### For Our Implementation

- Show death overlay ("You Died" / "Return to Save Point" button)
- EXP penalty on PvE death
- No penalty on PvP death
- Respawn at save point (configurable per character)
- Future: Resurrection skill for Priest class

---

## Leveling & Experience

### Dual Level System

- **Base Level**: Character's main level (1-99, or 1-255 in renewal)
  - Increases stats, HP/SP
  - Gives stat points to allocate
- **Job Level**: Class-specific level (1-50 for 2nd class)
  - Gives skill points
  - Required to change to next class tier

### EXP System

- **Base EXP**: For base level (from killing monsters)
- **Job EXP**: For job level (from killing monsters)
- **EXP curve**: Exponential — each level requires more EXP
- **Party bonus**: Extra EXP when in a party (varies by party size)
- **Level range**: EXP reduced if monster is too far below/above your level

### EXP Table (Simplified Example)

| Base Level | EXP Required |
|-----------|--------------|
| 1→2 | 9 |
| 10→11 | 2,000 |
| 20→21 | 20,000 |
| 50→51 | 500,000 |
| 70→71 | 3,000,000 |
| 90→91 | 20,000,000 |
| 98→99 | 100,000,000 |

---

## Cards & Enchantment System

### Card System

- Monsters have a chance to drop their card (0.01% - 1% typically)
- Cards slot into equipment with open slots
- Each card provides unique bonuses:
  - **Weapon cards**: +ATK, elemental damage, racial bonus, etc.
  - **Armor cards**: +DEF, HP, resistances
  - **Garment cards**: FLEE, resistance, special effects
  - **Shield cards**: Damage reduction from specific races/elements
  - **Headgear cards**: Various utility effects
  - **Accessory cards**: Various stat/utility bonuses

### Refinement/Upgrade System

- Equipment can be refined from +0 to +10
- Higher refinement = better stats BUT risk of breaking
  - +1 to +4: Safe (100% success)
  - +5 to +7: Moderate risk
  - +8 to +10: High risk (can destroy the item)
- Special items can protect against breaking (Enriched Oridecon/Elunium)

---

## Quest System

### Quest Types

| Type | Description |
|------|-------------|
| **Main Story** | Sequential story quests |
| **Side Quests** | Optional NPC quests |
| **Daily Quests** | Repeatable once per day |
| **Hunting Quests** | Kill X of monster Y |
| **Collection Quests** | Gather X items |
| **Delivery Quests** | Bring item to NPC |
| **Instance Quests** | Complete a dungeon instance |

### Quest UI

- **Quest log**: List of active quests with progress
- **Quest tracker**: On-screen tracking of current objectives
- **Quest markers**: NPCs with quests show "!" above head
- **Turn-in markers**: NPCs to turn in quests show "?" above head

---

## Storage & Inventory

### Inventory

- **Weight-based**: Items have weight, character has max carry weight
  - Max Weight = `300 + STR * 30` (approximately)
  - Over 50% weight: HP/SP regen halved
  - Over 90% weight: Cannot attack or use skills
- **Item types**: Equipment, consumables, cards, quest items, misc
- **Stack**: Consumables and materials stack (up to 30,000)
- **Equipment**: Does not stack (each item is unique due to slots/refinement/cards)

### Storage (Kafra)

- **Personal storage**: 300-600 slots (depending on version)
- **Accessible via Kafra NPC** in towns
- **Shared across characters** on same account
- **Small zeny fee** to access

---

## Server Architecture Implications

### What Must Be Server-Authoritative

| System | Why |
|--------|-----|
| **Damage calculation** | Prevent damage hacks |
| **Hit/miss/crit** | Prevent accuracy hacks |
| **Attack speed** | Prevent speed hacks |
| **Movement speed** | Prevent speed hacks |
| **Item drops** | Prevent dupe exploits |
| **EXP gains** | Prevent EXP hacks |
| **Stat allocation** | Prevent stat hacks |
| **Skill usage** | Validate SP cost, cooldowns, range |
| **Trade** | Prevent dupe exploits |
| **PvP zone check** | Enforce PvP rules |

### What Can Be Client-Side

| System | Why |
|--------|-----|
| **Pathfinding** | Client calculates path, server validates position |
| **Animation** | Visual only, no gameplay impact |
| **Hover effects** | UI only |
| **Sound effects** | Local |
| **Camera** | Local |
| **Chat input** | Client formats, server broadcasts |

### Network Events Needed (Expanded)

```
Client → Server:
  combat:attack {targetId}              // Request to start auto-attacking target
  combat:stop_attack {}                 // Stop auto-attacking
  combat:use_skill {skillId, targetId?, x?, y?}  // Use a skill
  player:position {x, y, z}            // Position update
  player:join {characterId, token}      // Enter world

Server → Client:
  combat:attack_result {attackerId, targetId, damage, isCrit, isMiss, targetHP}
  combat:health_update {characterId, hp, maxHp, sp, maxSp}
  combat:death {killedId, killerId}
  combat:respawn {characterId, x, y, z, hp, maxHp}
  combat:skill_effect {casterId, skillId, targetId?, x?, y?, damage?, effect?}
  combat:aspd_update {characterId, aspd}
  entity:spawn {entityId, type, name, level, hp, x, y, z}
  entity:move {entityId, x, y, z}
  entity:die {entityId}
  entity:despawn {entityId}
```

---

## Implementation Priority

### Phase 1: Core Combat (Current)
1. ✅ Server combat events (basic)
2. ✅ Game HUD (HP/MP bars)
3. **→ RO-style targeting (click-to-target, auto-path, auto-attack)**
4. **→ Attack speed system (ASPD)**
5. **→ Hover effects (name + arrow)**
6. **→ Fix HP bar updates**
7. Death/respawn UI

### Phase 2: Stats & Damage
1. 6 base stats (STR, AGI, VIT, INT, DEX, LUK)
2. Derived stat calculations
3. Proper damage formula
4. Hit/miss/crit system
5. Stat point allocation UI

### Phase 3: Equipment & Items
1. Equipment slots
2. Weapon types with different ASPD/range
3. Basic item drops from monsters
4. Inventory system
5. Equipment UI

### Phase 4: Monsters & NPCs
1. Monster spawning system
2. Monster AI (passive, aggressive)
3. Monster stats + drop tables
4. NPC interaction system
5. Shop NPC

### Phase 5: Skills
1. Skill data structure
2. Target skills
3. Ground AoE skills
4. Self-buff skills
5. Skill hotbar UI
6. Skill tree UI

### Phase 6: Classes
1. Base class selection
2. Skill trees per class
3. Class-restricted equipment
4. Class change NPC

### Phase 7: Social
1. Party system
2. Guild system
3. Trading
4. Vending

### Phase 8: World
1. Multiple maps
2. Map transitions/warps
3. PvP zones
4. Town safe zones
5. Dungeons

### Phase 9: Economy
1. Zeny currency
2. NPC shops
3. Player trading
4. Item marketplace/vending

### Phase 10: Polish
1. Card system
2. Refinement
3. Quest system
4. Achievement system
5. WoE/GvG

---

**Last Updated**: 2026-02-06
**Reference Game**: Ragnarok Online (Gravity Co., 2002)
**Our Adaptation**: Sabri_MMO — UE5 3D implementation of RO-style mechanics
