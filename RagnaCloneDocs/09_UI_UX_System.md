# 09 — UI/UX System

## Ragnarok Online Classic: Complete User Interface & User Experience Reference

This document is the authoritative reference for recreating the Ragnarok Online Classic UI/UX in Unreal Engine 5 (Slate C++). It covers every window, panel, interaction pattern, visual style, and system message in the original game, plus UE5-specific implementation guidance for Sabri_MMO.

---

## Table of Contents

1. [Main HUD](#1-main-hud)
2. [Chat System](#2-chat-system)
3. [Window Behavior](#3-window-behavior)
4. [Targeting System](#4-targeting-system)
5. [Damage Numbers](#5-damage-numbers)
6. [Status Display (Buffs/Debuffs)](#6-status-display-buffsdebuffs)
7. [Death UI](#7-death-ui)
8. [Vending UI](#8-vending-ui)
9. [Notifications](#9-notifications)
10. [UE5 Implementation](#10-ue5-implementation)

---

## 1. Main HUD

The RO Classic HUD is minimalist by modern standards. Most elements are small, draggable windows that players arrange to taste. The default layout places the Basic Info window at the top-center, the chat box at the bottom-left, the minimap at the top-right, and the hotbar at the bottom-center.

### 1.1 Basic Info Window (Alt+V)

The Basic Info window is the single most important HUD element. It is always visible and provides at-a-glance character status.

**Display Fields:**
| Field | Format | Details |
|-------|--------|---------|
| Character Name | Text | Character's display name |
| Base Level | `Lv XX` | Current base level (1-99 Classic, 1-175 Renewal) |
| Job Class | Text | Current job class name (e.g., "Swordman", "Wizard") |
| HP Bar | Green bar + `current / max` | Green fill, numeric overlay. Bar turns yellow at ~25%, red at ~10% |
| SP Bar | Blue bar + `current / max` | Blue fill, numeric overlay |
| Base EXP Bar | Yellow/orange bar + `XX.XX%` | Progress toward next base level |
| Job EXP Bar | Yellow/orange bar + `XX.XX%` | Progress toward next job level |
| Weight | `current / max` | Current carry weight vs maximum. Turns yellow at 50%, orange at 70%, red at 90% |
| Zeny | Gold number | Current currency with comma separators |

**Additional Features:**
- A small dropdown arrow/button provides access to other windows: Status, Inventory, Equipment, Skill, Map, Quest, Guild, Party, Options, Navigation
- The window has a minimize button that collapses it to just the name/level line
- The HP and SP bars show both the colored fill and numeric `current/max` overlay
- Weight display warns the player visually when approaching overweight thresholds

**Weight Thresholds:**
- **0-49%**: Normal (white text)
- **50-69%**: Cautionary (no penalty, but visual warning)
- **70-89%**: Minor Overweight — HP/SP natural regen disabled, item-creation skills blocked
- **90-100%**: Major Overweight — cannot attack or use any skills

### 1.2 Minimap (Ctrl+Tab to toggle transparency)

**Display:**
- Occupies the top-right corner of the screen
- Shows a 2D overhead view of the entire current map
- Player position indicated by a white arrow/dot pointing in the facing direction
- Other players shown as white dots
- Party members shown as pink/magenta dots
- NPCs shown as colored dots/icons (varies by type)
- Warp portals shown as red dots
- Map name displayed at the top of the minimap

**NPC Minimap Icons (post-Navigation System):**
| Icon | NPC Type |
|------|----------|
| Magnifying glass | Guide NPC |
| Bed | Inn / Save Point |
| Chest | Storage NPC (Kafra/Zonda) |
| Potion | Tool Dealer |
| Sword | Weapon Dealer |
| Shield | Armor Dealer |
| Hammer | Repairman |
| Yellow + | Quest NPC |
| Red dot | Portal/Warp |
| Blue dot | Generic NPC |

**Controls:**
- Ctrl+Tab cycles minimap opacity: Opaque -> Semi-transparent -> Transparent -> Hidden -> Opaque
- Scroll wheel on minimap zooms in/out
- Right-click on minimap marks a navigation waypoint (with Navigation System)
- Clicking an NPC icon on the minimap activates navigation path to that NPC

### 1.3 World Map (Ctrl+~)

- Full-screen overlay showing the entire game world
- Click on a region to zoom into that area
- Party members shown as pink dots on the world map
- Map names labeled on each zone
- Warp connections shown between adjacent maps
- Click outside or press Escape to close

### 1.4 Chat Window

See [Section 2: Chat System](#2-chat-system) for full details.

**Default Position:** Bottom-left of screen, resizable vertically with F10.

### 1.5 Status Window (Alt+A / Ctrl+A)

The Status Window shows detailed character statistics.

**Layout (Two Columns):**

| Left Column | Right Column |
|-------------|-------------|
| STR: `base + bonus` | ATK: `statusATK + weaponATK` |
| AGI: `base + bonus` | MATK: `min ~ max` |
| VIT: `base + bonus` | HIT: `value` |
| INT: `base + bonus` | CRITICAL: `value` |
| DEX: `base + bonus` | DEF: `hardDEF + softDEF` |
| LUK: `base + bonus` | MDEF: `hardMDEF + softMDEF` |
| | FLEE: `flee + perfectDodge` |
| | ASPD: `value` |

**Additional Info Displayed:**
- Status Points remaining (available to allocate)
- Guild name (if applicable)
- "+" buttons next to each base stat for stat point allocation
- Each stat increase shows the cost in status points (increases with stat value)
- Stat bonus column shows equipment/buff modifiers in green

**Stat Point Cost Formula (Classic):**
```
Cost = floor((current_base_stat - 1) / 10) + 2
```
So stat 1->2 costs 2 points, stat 10->11 costs 3 points, stat 20->21 costs 4 points, etc.

### 1.6 Equipment Window (Alt+Q / Ctrl+Q)

**Equipment Slots (Classic Layout):**
| Slot | Position | Notes |
|------|----------|-------|
| Top Headgear | Top-center | Hats, helms, crowns |
| Mid Headgear | Center-left | Glasses, masks (covers eyes) |
| Lower Headgear | Bottom-left | Masks, pipes (covers mouth) |
| Armor (Body) | Center | Main body armor, determines element |
| Weapon (Right Hand) | Right | Swords, daggers, staves, bows, etc. |
| Shield (Left Hand) | Left | Shields (unavailable with two-handed weapons) |
| Garment | Back-center | Cloaks, mantles, robes |
| Shoes (Footgear) | Bottom-center | Boots, sandals, shoes |
| Accessory 1 | Bottom-right | Rings, brooches, clips |
| Accessory 2 | Bottom-right-2 | Second accessory slot |

**Features:**
- Character sprite preview in the center of the equipment window (shows equipped headgears/garments)
- Drag-and-drop from inventory to equip
- Right-click to unequip back to inventory
- Hovering over a slot shows item tooltip with stats
- Empty slots shown as faded outlines of the equipment type
- Card slots shown as small squares on each item tooltip (0-4 cards depending on item)
- Equipment comparison tooltip when hovering equipped vs. inventory item

**Gear Switching (Post-Renewal):**
- Two equipment sets can be stored and switched via hotkey
- Each set has its own headgear, armor, weapon, shield, garment, shoes, and accessories

### 1.7 Inventory Window (Alt+E / Ctrl+E)

**Layout:**
- Grid of item icons (approximately 10 columns wide)
- Tabs at the top: **Item** (consumables/usables) | **Equip** (weapons/armor) | **Etc** (materials/misc) | **Favorites** (Renewal)
- Weight display at the bottom: `current / max`
- Zeny display at the bottom

**Item Display:**
- Each slot shows the item icon
- Stackable items show count in bottom-right of icon
- Equipment items show refine level as `+X` prefix
- Right-click to use consumables or equip/unequip gear
- Drag-and-drop to trade window, hotbar, equipment window, or ground (drop)
- Items are NOT sorted into a fixed grid — they fill from top-left, and dropping/using leaves gaps that fill on next pickup

**Item Tooltips:**
- Item name (colored by rarity: white=common, green=uncommon, blue=rare, purple=epic, orange=MVP)
- Item type and subtype
- Weight
- Description text
- Stats/effects
- Card slots (shown as `[Card Name]` or `[Slot Available]`)
- Requirements (level, class)
- Sell price

**Capacity:** Unlimited slots (Classic) — limited only by weight. Renewal added a 100-slot cap per tab.

### 1.8 Skill Tree Window (Alt+S)

**Layout:**
- Tree-style layout showing skill icons connected by prerequisite lines
- Each skill icon shows current level / max level
- Skills with unmet prerequisites are grayed out
- Skills that can be leveled show a "+" button or are highlighted
- Skill points remaining displayed at the top

**Interaction:**
- Click "+" to add one skill point (confirmation dialog)
- Right-click a skill to see detailed description: name, type (passive/active/supportive), target type, SP cost per level, cast time, cooldown, damage formula, range
- Drag a skill icon to the hotbar to assign it
- Apply/Reset buttons for pending skill point changes

**Skill Tree Variants:**
- **Full Skill Tree**: Shows all skills for current and prerequisite classes in a large scrollable tree
- **Mini Skill Tree**: Compact list view showing only learned skills with level indicators

**Skill Types (Visual Indicators):**
- Passive skills: Blue icon border
- Active offensive skills: Red/orange icon border
- Supportive skills: Green icon border
- Toggle skills: Yellow icon border with on/off indicator

### 1.9 Hotbar / Shortcut Bar (F12 to toggle)

**Layout:**
- 1 row of 9 slots, mapped to F1-F9 by default
- Pressing F12 cycles through 1-4 visible rows (4 total rows available)
- Total: 4 rows x 9 slots = 36 shortcut slots

**Row Key Mappings (Default):**
| Row | Keys | Notes |
|-----|------|-------|
| Row 1 | F1-F9 | Always visible first |
| Row 2 | F1-F9 (second bar) | Visible when F12 pressed twice |
| Row 3 | F1-F9 (third bar) | Visible when F12 pressed thrice |
| Row 4 | F1-F9 (fourth bar) | Visible when F12 pressed four times |

**Slot Contents:**
- Skills (dragged from Skill Tree)
- Consumable items (dragged from Inventory)
- Equipment items (for quick weapon-swapping)
- Emotes (dragged from Emote window)

**Battle Mode (/battlemode or /bm):**
When enabled, reassigns keyboard for combat:
- Row 1: Q, W, E, R, T, Y, U, I, O
- Row 2: A, S, D, F, G, H, J, K, L
- Row 3: Z, X, C, V, B, N, M (partial)
- Normal typing disabled in battle mode — must use Enter to open chat first

**Visual Feedback:**
- Cooldown overlay (clock-wipe darkening animation)
- SP cost shown below skill icon (optional)
- Greyed out when insufficient SP or skill on cooldown
- Item count shown for consumables
- Skill level shown in small text

### 1.10 Party Window (Alt+Z)

**Display per Member:**
| Element | Details |
|---------|---------|
| Character name | Text |
| Job class icon | Small sprite next to name |
| HP bar | Green bar (no numeric value in classic) |
| SP bar | Blue bar (added in Renewal) |
| Map name | Shows which map the member is on |
| Online indicator | Highlighted if online |

**Features:**
- Maximum 12 party members
- Party leader indicated by a crown/star icon
- Lock icon at bottom: toggles between "click name to whisper" and "click name to cast skill on member"
- Party share settings: EXP share (Even Share / Each Take), Item share (Individual / Shared)
- Right-click member name for options: Whisper, Expel, Follow
- Click the circle/map indicator to open world map centered on that member
- Mini party display: compact HP-bar-only version for less screen clutter
- Heal skills can be targeted by clicking party member HP bars directly

### 1.11 Guild Window (Alt+G)

**Tabs:**
| Tab | Contents |
|-----|----------|
| **Guild Info** | Guild name, level, EXP bar, emblem (24x24), master name, member count, average level, notice/announcement |
| **Members** | Full member list with name, class, level, title/position, online status (highlighted), contribution |
| **Position** | Define guild titles (up to 20), set permissions (invite, expel, storage access), set EXP tax rate per position |
| **Skills** | Guild skills tree (Guild Extension, Guardian Research, etc.) |
| **Expulsion** | List of expelled members with reason |
| **Notice** | Guild master can write announcements (title + body) shown on login |
| **Alliance** | Allied and enemy guilds |

**Guild Emblem:**
- 24x24 pixels, BMP or GIF format (animated GIF supported)
- Hot pink (RGB 255,0,255) renders as transparent
- Displayed above character name, on guild window, and in WoE
- Maximum guild size: 16 base, up to 56 with Guild Extension skill

### 1.12 Quest Log (Alt+U)

**Layout:**
- Two tabs: **Active** and **Inactive**
- Active quests shown in a scrollable list with quest name and brief status
- Right-click a quest to move between Active and Inactive tabs (no effect on quest progress)
- First 5 active quests can appear as a sidebar tracker on the right side of the screen

**Quest Entry Details:**
- Quest name
- Quest objectives (kill X monsters, collect Y items, visit NPC Z)
- Progress indicators (e.g., "5/10 Poring killed")
- Map name and NPC for next step
- Time limit (if applicable)

**Visual Feedback:**
- Kill counter popup appears above character head when a quest monster is killed
- Scroll icon appears at bottom-right when a quest objective is completed
- Quest NPCs have a yellow "!" icon above their head (new quest) or "?" (turn-in ready)

### 1.13 Friends List (Alt+H)

**Features:**
- Character-bound list (each character has separate friends)
- Maximum 40 friends per character
- Online friends show "ON" indicator next to their name
- Login/logout notifications appear in chat box
- Double-click a friend's name to open whisper window
- Right-click options: Whisper, Remove, Block

**Adding Friends:**
- Right-click another player's character -> "Register as Friend"
- Requires mutual acceptance (both players must confirm)

**Block List:**
- `/ex [name]` blocks whispers from a specific character
- `/exall` blocks all whispers
- `/in [name]` unblocks a specific character
- `/inall` unblocks all whispers
- Block list is per-character, not per-account

---

## 2. Chat System

### 2.1 Chat Window Structure

**Components:**
- **Message Display Area**: Scrollable text area showing all chat messages
- **Input Bar**: Text input field at the bottom
- **Whisper Target Field**: Small input field to the left of the main input bar for whisper recipient name
- **Tabs**: Player-created tabs to filter message types
- **Lock Icon**: Locks/unlocks tab splitting (allows multiple independent chat boxes)

**Chat Window Controls:**
- F10: Cycle chat window height (3 sizes: small, medium, large)
- F11: Show/hide all chat windows
- Ctrl+F11 / Shift+F11: Toggle individual chat boxes
- Alt+F10: Dedicated chat window toggle
- Page Up/Down: Scroll through message history

### 2.2 Chat Channels & Commands

| Channel | Prefix/Command | Color | Overhead Bubble | Range |
|---------|---------------|-------|----------------|-------|
| Public (Normal) | (none — just type) | White | Yes — white text above head | ~14x14 cells (~700 UE units) |
| Whisper (PM) | Type name in left box, or `/w "name" msg` | Yellow/Gold | No | Unlimited (server-wide) |
| Party | `%` prefix or Ctrl+Enter | Green | Yes — green overhead bubble | Unlimited (party members) |
| Guild | `$` or `/gc` prefix, or Alt+Enter | Light Green/Cyan | No | Unlimited (guild members) |
| Clan | `/cl` prefix | Teal | No | Unlimited (clan members) |
| Battle (BG) | `/battlechat` toggle | Orange | No | Battlegrounds only |

**Chat Text Colors in Message Window:**
| Message Type | Color |
|-------------|-------|
| Your own public chat | White |
| Others' public chat | White |
| Whisper sent | Yellow |
| Whisper received | Yellow |
| Party chat | Green |
| Guild chat | Bright Green / Cyan |
| System messages | Light Blue / Cyan |
| Item links | Blue (clickable) |
| GM broadcasts | Yellow with special formatting |
| Error messages | Red |
| Combat/Battle log | Gray / Light Gray |
| NPC dialog | White (in separate NPC dialog window) |

### 2.3 Overhead Chat Bubbles

- Public chat messages appear as text above the character's head
- Text fades after approximately 5 seconds
- White text with slight black outline/shadow for readability
- Party chat bubbles appear in green
- Guild chat does NOT show overhead bubbles
- Whispers do NOT show overhead bubbles
- `/notalkmsg` disables overhead bubbles
- `/notalkmsg2` hides all chat messages (window + bubbles)

### 2.4 Slash Commands (Complete List)

#### Communication Commands
| Command | Description |
|---------|-------------|
| `/w "name" message` | Whisper to player |
| `/gc message` or `$ message` | Guild chat |
| `/cl message` | Clan chat |
| `% message` | Party chat |
| `/chat` | Create a chat room |
| `/q` | Leave a chat room |
| `/ex [name]` | Block whispers from character |
| `/exall` | Block all whispers |
| `/in [name]` | Allow whispers from character |
| `/inall` | Allow all whispers |
| `/hi [message]` | Send greeting to all online friends |
| `/em [message]` | Set auto-reply message (AFK) |
| `/savechat` | Save chat log to text file |

#### Party & Guild Commands
| Command | Description |
|---------|-------------|
| `/organize [name]` | Create a party |
| `/invite [name]` | Invite player to party |
| `/leave` | Leave current party |
| `/expel [name]` | Expel from party |
| `/accept` | Enable party invitations |
| `/refuse` | Refuse all party invitations |
| `/guild [name]` | Create guild (requires Emperium) |
| `/breakguild [name]` | Disband guild |
| `/emblem` | Toggle guild emblem display |
| `/call` | Teleport guild members to Guild Master |

#### Display & Settings Commands
| Command | Description |
|---------|-------------|
| `/effect` | Toggle spell visual effects |
| `/mineffect` or `/minimize` | Reduce particle/spell graphics |
| `/bgm` or `/music` | Toggle background music |
| `/bv [0-127]` | Set BGM volume |
| `/sound` | Toggle sound effects |
| `/v [0-127]` | Set SFX volume |
| `/fog` | Toggle fog rendering |
| `/camera` | Toggle camera zoom capability |
| `/lightmap` | Toggle map lighting effects |
| `/quake` | Toggle screen shake effects |
| `/skip` | Toggle frame skip |
| `/window` | Toggle window snap/docking |
| `/font` | Toggle description text positioning |
| `/stateinfo` | Toggle status icon descriptions |
| `/monsterhp` | Toggle monster HP bars |
| `/showname` | Change character name font style |
| `/aura` | Reduce Lv99+ character aura effect |
| `/aura2` | Disable Lv99+ character aura completely |
| `/eqopen` | Make equipment viewable by others |

#### Combat Commands
| Command | Description |
|---------|-------------|
| `/noctrl` or `/nc` | Auto-attack without holding Ctrl |
| `/noshift` or `/ns` | Use support skills on players without Shift |
| `/battlemode` or `/bm` | Enable battle keyboard (QWERTY = hotbar) |
| `/battlechat` | Toggle battlegrounds chat |
| `/skillfail` | Toggle red text on skill failure |
| `/miss` | Toggle miss notification display |
| `/snap` | Toggle position snap in combat |
| `/skillsnap` | Toggle snap for skill targeting |
| `/itemsnap` | Toggle snap for item pickup |
| `/notrade` or `/nt` | Auto-decline trade requests |

#### Navigation & Information Commands
| Command | Description |
|---------|-------------|
| `/where` | Display current map name and coordinates |
| `/who` or `/w` | Display number of players online |
| `/memo` | Memorize current location as warp point |
| `/navi [map] [x] [y]` or `/navigation` | Open navigation system |
| `/navi2` or `/navigation2` | Navigation with visible route overlay |
| `/hunting` | Check hunting quest progress |
| `/loginout` or `/li` | Show guild/friends online status |

#### Utility Commands
| Command | Description |
|---------|-------------|
| `/sit` | Sit down (also Insert key) |
| `/stand` | Stand up |
| `/bangbang` | Rotate character clockwise |
| `/bingbing` | Rotate character counter-clockwise |
| `/emotion` | Open emote selection window |
| `/check [text]` | Creates green-colored text |
| `/set1` | Macro: enables /noctrl + /showname + /skillfail |
| `/tip` | Open "Tip of the Day" dialog |
| `/h` or `/help` | Show available commands |
| `/buildinfo` | Show client build version |

#### Skill Shortcut Commands
| Command | Description |
|---------|-------------|
| `/q1` or `/quickspell1` | Right-click uses F9 skill |
| `/q2` or `/quickspell2` | Mouse wheel uses F7/F8 skills |
| `/q3` or `/quickspell3` | Enables both /q1 and /q2 |

#### Rankings
| Command | Description |
|---------|-------------|
| `/alchemist` | Top 10 Alchemists ranking |
| `/blacksmith` | Top 10 Blacksmiths ranking |
| `/taekwon` | Top 10 Taekwon Kids ranking |
| `/pk` | Top 10 Player Killers ranking |

#### AI Commands
| Command | Description |
|---------|-------------|
| `/hoai` | Toggle Homunculus AI mode |
| `/merai` | Toggle Mercenary AI mode |
| `/traceai` | Save Homunculus debug log |

### 2.5 Emotes (Complete List)

Emotes are triggered via `/command` or Alt+# shortcuts. The Emote window opens with Alt+L. Macros (Alt+M) allow binding emotes to Alt+0-9.

| ID | Command | Alt Key | Description |
|----|---------|---------|-------------|
| 0 | `/!` | — | Exclamation mark (surprise) |
| 1 | `/?` | — | Question mark (confusion) |
| 2 | `/ho` | — | Music note (humming/bored) |
| 3 | `/lv` | Alt+1 | Beating heart (love) |
| 4 | `/lv2` | — | Exploding heart (deep love) |
| 5 | `/swt` | — | Sweat drop (nervous/awkward) |
| 6 | `/ic` | — | Light bulb (idea/realization) |
| 7 | `/an` | — | Anger vein (irritated) |
| 8 | `/ag` | — | Storm cloud (furious) |
| 9 | `/$` | — | Dollar sign (money/greed) |
| 10 | `/...` | — | Three dots (speechless) |
| 11 | `/scissors` | — | Scissors (RPS) |
| 12 | `/rock` | — | Rock (RPS) |
| 13 | `/paper` | — | Paper (RPS) |
| 14 | `/thx` | — | Thank you face |
| 15 | `/wah` | — | Panic/distress look |
| 16 | `/sry` | — | Sorry/apologetic face |
| 17 | `/heh` | — | Laughing/amused face |
| 18 | `/swt2` | — | Explosive sweat drop |
| 19 | `/hmm` | — | Thinking/pondering face |
| 20 | `/no1` | — | Thumbs up wink (praise) |
| 21 | `/?? ` | — | Shaking head (disagreement) |
| 22 | `/omg` | — | Shocked/alarmed face |
| 23 | `/oh` | — | Red circle sign |
| 24 | `/X` | — | Red X sign (no/refuse) |
| 25 | `/hlp` | — | Help signs (urgent plea) |
| 26 | `/go` | — | Explosive "Go!" sign |
| 27 | `/sob` | — | Crying face |
| 28 | `/gg` | — | Evil giggle (mischief) |
| 29 | `/kis` | — | Kiss facing right |
| 30 | `/kis2` | — | Kiss facing left |
| 31 | `/pif` | — | Blowing smoke |
| 32 | `/ok` | — | Head nod (agreement) |
| — | `/bzz` or `/e1` | — | Angry buzzing vein |
| — | `/rice` or `/e2` | — | Drooly face |
| — | `/awsm` or `/e3` | — | Heart eyes (amazed) |
| — | `/meh` or `/e4` | — | Tongue out (indifferent) |
| — | `/shy` or `/e5` | — | Blushing face |
| — | `/pat` or `/e6` | — | Petting hand |
| — | `/mp` or `/e7` | — | SP low indicator |
| — | `/slur` or `/e8` | — | Lustful/dazed face |
| — | `/com` or `/e9` | — | Beckoning finger (come here) |
| — | `/yawn` or `/e10` | — | Big yawn (sleepy) |
| — | `/grat` or `/e11` | — | Congratulations sparkle |
| — | `/hp` or `/e12` | — | HP low indicator |
| — | `/fsh` or `/e13` | — | Shiny eyes (desire) |
| — | `/spin` or `/e14` | — | Dizzy spiral |
| — | `/sigh` or `/e15` | — | Exhale puff (tired) |
| — | `/dum` or `/e16` | — | Blank expression (dumb) |
| — | `/crwd` or `/e17` | — | Loud clamoring |
| — | `/desp` or `/otl` or `/e18` | — | Giving up / OTL stick figure |
| — | `/dice` or `/e19` | — | Random dice roll (1-6) |
| — | `/e20` | — | Pointing upward |
| — | `/hum` or `/e27` | — | Annoyed expression |
| — | `/abs` or `/e28` | — | Soul leaving body (embarrassment) |
| — | `/oops` or `/e29` | — | Red-faced mistake |
| — | `/spit` or `/e30` | — | Spitting/vomiting |
| — | `/ene` or `/e31` | — | Blessed/energized face |
| — | `/panic` or `/e32` | — | Frightened expression |
| — | `/whisp` or `/e33` | — | Spinning whisper icon |
| — | `/dbc` | — | Logs breaking apart |

**Rock-Paper-Scissors Special Commands:**
| Command | Shortcut | Action |
|---------|----------|--------|
| `/bawi` | Ctrl+= | Rock (fist raise) |
| `/bo` | Ctrl+\ | Paper |
| `/gawi` | Ctrl+- | Scissors |

### 2.6 Chat Rooms

- Created with `/chat` command or Alt+C
- A clickable speech bubble appears above the creator's head
- Other players click the bubble to enter the room
- Rooms have a title (set by creator), password (optional), and member limit
- Chat within the room is private — only visible to members inside
- Creator can kick members and change settings
- Room chat does not appear in the main chat window

---

## 3. Window Behavior

### 3.1 Dragging

- All RO windows are draggable by their title bar
- Click and hold the title bar to drag
- Windows can be placed anywhere on screen
- Windows cannot be dragged off-screen (clamped to viewport bounds)
- Each window remembers its last position (saved in client config)

### 3.2 Resizing

- Most windows have fixed sizes (Status, Equipment, Skill Tree, Inventory)
- The Chat Window is vertically resizable via F10 (three height presets) and horizontally by dragging the edge
- The Quest sidebar is fixed width

### 3.3 Z-Order & Focus

- Clicking a window brings it to the front (top of Z-order)
- The most recently interacted window is always on top
- Modal dialogs (NPC dialog, trade confirmation, delete confirmation) block input to all other windows
- The game world remains clickable through gaps between windows
- Overlapping windows do not consume clicks meant for windows beneath them — only the topmost window at the click position receives the event

### 3.4 Window Snapping / Docking

- `/window` command toggles snap behavior
- When enabled, dragging windows near each other causes them to snap edges together
- Windows also snap to screen edges
- Snap threshold is approximately 10 pixels

### 3.5 Keyboard Shortcuts (Complete Reference)

#### Window Toggle Shortcuts
| Shortcut | Window | Alt Shortcut |
|----------|--------|--------------|
| Alt+A / Ctrl+A | Status Window | Also Alt+V (Basic Info) |
| Alt+E / Ctrl+E | Inventory | |
| Alt+Q / Ctrl+Q | Equipment | |
| Alt+S | Skill Tree | |
| Alt+V | Basic Info Window | |
| Alt+Z | Party Window | |
| Alt+G | Guild Window | |
| Alt+H | Friends List | |
| Alt+I | Friends Setup | |
| Alt+U | Quest Window | |
| Alt+O / Ctrl+O | Options Window | |
| Alt+C | Chat Room | |
| Alt+J | Pet Window | |
| Alt+R | Homunculus Window | |
| Alt+W | Cart Window (Merchant) | |
| Alt+L | Emote Window | |
| Alt+M | Macro Window | |
| Alt+B | Instance Window | |
| Alt+D | Detail Arrange | |
| Alt+P | Party Setup | |
| Alt+Y | Command List | |
| Alt+T | Standby/Idle Mode | |

#### Function Keys
| Key | Action |
|-----|--------|
| F1-F9 | Use hotbar slot 1-9 |
| F10 | Cycle chat window height |
| F11 | Show/hide all chat windows |
| F12 | Cycle hotbar rows (1->2->3->4->hidden) |

#### Other Shortcuts
| Shortcut | Action |
|----------|--------|
| Insert | Sit / Stand toggle |
| Escape | Open game menu / close topmost window |
| Tab | Cycle focus between open windows |
| Page Up / Down | Scroll text in focused window |
| Ctrl+Tab | Cycle minimap transparency |
| Ctrl+~ | Open world map |
| Ctrl+Enter | Party chat mode |
| Alt+Enter | Guild chat mode |
| Enter | Open/focus chat input bar |
| Backspace | Reply to last whisper (default, rebindable) |
| Alt+End | Toggle player HP/SP bars overhead |
| Alt+Home | Toggle ground cursor |
| Shift+Click Dir | Change character facing direction |
| Shift+Right-Click | Auto-follow target player |
| Print Screen | Screenshot |

### 3.6 Position Persistence

- Window positions are saved to `savedata/OptionInfo.lua` in the client
- Positions persist across sessions
- Each character can have different window layouts
- Positions reset if the client resolution changes
- Default positions are defined in the client data files

---

## 4. Targeting System

### 4.1 Click-to-Target

RO uses a purely click-based targeting system with no tab-targeting.

**Target Acquisition:**
- Left-click on a monster to begin attacking it (auto-attack)
- Left-click on an NPC to begin dialog
- Left-click on ground to walk to that position
- Left-click on an item on the ground to pick it up
- Right-click on another player to open context menu (Whisper, Trade, Party Invite, Block, etc.)
- Left-click while in skill targeting mode to apply the skill

**Target Priority (when entities overlap):**
1. NPC (highest priority for interaction)
2. Players (context menu)
3. Monsters (attack)
4. Ground items (pickup)
5. Ground (movement)

### 4.2 Cursor Types

The cursor changes contextually based on what is under the mouse.

| Cursor | Context | Visual |
|--------|---------|--------|
| Normal | Default / hovering over ground | Standard arrow |
| Move | Clicking ground to walk | Directional arrow / footstep |
| Attack | Hovering over hostile monster | Sword/crosshair cursor |
| Talk | Hovering over NPC | Speech bubble cursor |
| Pickup | Hovering over ground item | Hand/grab cursor |
| Cast | In skill targeting mode | Crosshair / targeting cursor |
| Rotate Camera | Holding right mouse button | Curved double arrows (left/right) |
| Resize | On chat window resize edge | Vertical resize arrows |
| Busy/Wait | Loading or processing | Hourglass |

### 4.3 Target Info Display

When a monster or player is targeted:
- **Monster:** Name appears above the monster, HP bar shown (if `/monsterhp` enabled). Element and size info shown in some clients.
- **Player:** Name, guild emblem, guild name, party name shown above character. HP bar visible if they are in your party.
- **NPC:** NPC name shown above them at all times.

### 4.4 Skill Targeting Modes

**Single Target Skills:**
1. Press skill hotkey (or click in skill tree)
2. Cursor changes to crosshair/targeting cursor
3. Left-click on a valid target (monster, player, self depending on skill type)
4. Skill executes on that target
5. Right-click or Escape cancels targeting mode

**Ground-Target Skills (AoE):**
1. Press skill hotkey
2. A colored targeting grid/circle appears on the ground following the cursor
3. The grid shows the AoE size (e.g., 5x5 cells, 7x7 cells)
4. Grid cells are colored based on skill: red for offensive, green for supportive, blue for utility
5. Left-click to cast at that position
6. Right-click or Escape cancels
7. The targeting grid file is `magic_target.tga` (customizable by players)

**Self-Cast Skills:**
- Skills that target self execute immediately when the hotkey is pressed
- No targeting mode needed
- Examples: Increase AGI (on self), Blessing (on self), Heal (on self)

**Shift+Click Modifier:**
- By default, clicking a player with a support skill opens the whisper window
- Hold Shift to force-cast the skill on the player instead
- `/noshift` command removes the need to hold Shift

### 4.5 Attack Indicators

- **Auto-Attack Range**: Not visually shown, but enforced server-side (1 cell melee, variable for ranged)
- **Skill Range**: Some custom clients show range circles; official client does not show range indicators
- **Cast Circle**: White/colored circle appears under the caster during casting time
- **AoE Indicator**: Ground effect appears at target location during and after skill execution

---

## 5. Damage Numbers

### 5.1 Display Mechanics

Damage numbers are sprite-based floating text that appear above targets when damage is dealt.

**Animation:**
1. Number appears slightly above the target's head
2. Rises upward in a slight arc (inverted U trajectory)
3. Scales down slightly during the arc
4. Fades to transparent over ~1-1.5 seconds
5. Multiple damage numbers stack vertically (newest on top)

### 5.2 Color Coding

| Color | Meaning | Details |
|-------|---------|---------|
| **White** | Normal physical damage | Standard melee or ranged auto-attack hit |
| **Yellow** | Multi-hit total | Displayed after Double Attack, multi-hit skills — shows combined total above individual white numbers |
| **Red spike bubble** | Critical hit | Normal damage number surrounded by a red starburst/spike effect |
| **Blue** | Healing | HP recovery amount (Heal, potions) |
| **Red "Miss"** | Miss / Dodge | Word "Miss" in red text appears above the *attacker* (not the target) |
| **Green** | Skill damage (some clients) | Magical skill damage differentiated from physical |
| **Purple/Violet** | Holy/Shadow damage (some clients) | Element-specific in custom clients |
| **Light Yellow** | Individual multi-hit numbers | Faded/small numbers for each hit before the yellow total |

### 5.3 Special Damage Displays

| Type | Visual |
|------|--------|
| **Critical** | Red starburst/spike bubble effect around the damage number |
| **Double Attack** | Two small white numbers followed by one larger yellow total |
| **Triple Attack** | Three small numbers + yellow total |
| **Skill Damage** | Skill name text may appear briefly above/below the damage number |
| **Combo** | Sequential numbers with final total |
| **Lucky Dodge** | "Lucky" text in yellow (perfect dodge via LUK) |
| **Immune/Ghost** | "0" displayed (damage reduced to zero by element) |

### 5.4 Damage Number Font

- Custom bitmap font (not system font)
- Numbers are rendered as sprites from `damage.spr` / `damage.act` files
- Each digit (0-9) plus "Miss", comma, and special characters are separate sprite frames
- Font is crisp and readable even at small sizes
- Numbers use black outline/shadow for readability against any background

---

## 6. Status Display (Buffs/Debuffs)

### 6.1 Icon Display Location

- Status icons appear in a vertical column on the **right side of the screen**
- Icons stack from top to bottom
- When there are more icons than vertical space, they wrap to a second column to the left
- Each icon is approximately 24x24 pixels

### 6.2 Icon Behavior

- **Duration indicator:** Icons gradually become more opaque/faded as the buff duration expires (fills like a clock wipe from bottom to top, or fades)
- **Hover tooltip:** Hovering over a status icon shows the effect name and remaining duration (if `/stateinfo` is enabled)
- **Flash/pulse:** Some icons flash briefly when first applied
- **Removal animation:** Icons simply disappear when the effect ends (no special animation)

### 6.3 Buff Categories & Icons

**Positive Buffs (Common):**
| Buff | Icon Description | Source |
|------|-----------------|--------|
| Blessing | Blue cross/angel wings | Priest |
| Increase AGI | Green up-arrow / wind swirl | Priest |
| Angelus | Blue shield icon | Priest |
| Kyrie Eleison | Golden barrier icon | Priest |
| Impositio Manus | Red fist icon | Priest |
| Suffragium | Purple magic circle | Priest |
| Assumptio | Golden glow | High Priest |
| Adrenaline Rush | Red muscle/fist | Blacksmith |
| Weapon Perfection | Orange sword | Blacksmith |
| Over Thrust | Red gears | Blacksmith |
| Endure | Brown shield | Swordsman |
| Provoke | Red anger icon | Swordsman |
| Two-Hand Quicken | Blue blade swirl | Knight |
| Concentration | Blue eye | Hunter |
| Wind Walk | Green boots | Bard/Dancer |
| Poem of Bragi | Blue music note | Bard |
| Assassin Cross of Sunset | Red sunset | Bard |
| Gloria | Yellow halo | Priest |
| Magnificat | Blue prayer hands | Priest |
| Energy Coat | Blue energy shield | Sage |

**Negative Debuffs (Status Ailments):**
| Debuff | Icon Description | Visual on Character |
|--------|-----------------|-------------------|
| Poison | Purple skull/drop | Character turns purple-green, periodic damage numbers |
| Stun | Yellow stars | Character freezes, stars orbit head |
| Freeze | Blue snowflake | Character encased in ice block, cannot move |
| Stone (Petrify) | Gray rock | Character turns gray, cannot act |
| Sleep | Blue "Zzz" | Character falls over with "Zzz" above |
| Blind | Black circle | Screen darkens for affected player, character has dark aura |
| Silence | Red X on mouth | Red "!" appears if player tries to cast |
| Curse | Purple skull | Character moves very slowly, LUK reduced to 0 |
| Bleeding | Red drop | Periodic red damage numbers, blood drip effect |
| Confusion | Spiral eyes | Movement directions randomized |
| Hallucination | Wavy eye | Screen distortion effect, fake damage numbers |
| Decrease AGI | Red down-arrow | Movement visibly slowed |
| Divest (Strip) | Broken item icon | Specific equipment slot emptied |

### 6.4 Character Visual Effects for Statuses

Many buffs and debuffs also produce visual effects on the character sprite:

| Effect | Visual |
|--------|--------|
| Blessing | Brief golden sparkle on application |
| Increase AGI | Green wind swirl around feet |
| Kyrie Eleison | White barrier sphere around character |
| Assumptio | Upward golden pillar on application |
| Energy Coat | Blue translucent energy shell |
| Endure | Brief brown flash |
| Provoke | Red anger effect on target |
| Freeze | Blue ice encasing character |
| Stone | Gray stone overlay, character immobilized |
| Poison | Green/purple bubbles around character |
| Stun | Yellow stars orbiting head |
| Hiding/Cloaking | Character becomes semi-transparent |
| Level 99 Aura | Swirling golden/blue aura around character (permanent) |

---

## 7. Death UI

### 7.1 Death Sequence

1. **Death Animation**: Character falls to the ground (death sprite animation)
2. **Screen Effect**: No screen fade or gray-out in Classic RO — the character simply lies on the ground
3. **Input Disabled**: Cannot move, attack, use skills, or use items
4. **Chat Available**: Dead players can still type in chat
5. **Inventory Locked**: Cannot access inventory or equipment

### 7.2 Death Penalties (Classic/Pre-Renewal)

| Condition | Base EXP Loss | Job EXP Loss |
|-----------|--------------|-------------|
| Death to monster (PvM) | 1% of current level's total EXP | 0% |
| Death to player (PvP) | 0% (PvP maps) | 0% |
| Death in WoE (Guild War) | 0% | 0% |
| Novice class (base level <= 99) | 0% (Novices are exempt) | 0% |

- EXP cannot drop below 0% of current level (you cannot de-level from death)
- The EXP bar visually decreases when you respawn

### 7.3 Resurrection Options

When dead, the player sees a dialog/menu with options:

**Option 1: Return to Save Point**
- Always available
- Teleports to last saved Kafra/save point
- Full HP and SP restored
- EXP penalty already applied

**Option 2: Wait for Resurrection**
- Stay dead on the ground waiting for a Priest/High Priest
- **Resurrection skill** (Priest Lv1-4): Revives with 10%/30%/50%/80% HP, 0 SP
- **Redemptio skill** (High Priest): Mass resurrection in 15x15 area, revives with 50% HP, caster reduced to 1 HP/SP and loses 1% base EXP
- **Yggdrasil Leaf** item: Another player uses it on you, revives with full HP
- No SP restored on resurrection (except specific skill levels)
- Resurrected players keep their position (do not teleport to save point)
- Resurrection restores a portion of the lost EXP (varies by skill level)

**Option 3: Token of Siegfried (Item)**
- Consumes a Token of Siegfried from inventory
- Self-resurrection with full HP/SP
- Available in some server configurations

### 7.4 Death UI Dialog

The death dialog is a simple centered window:
```
┌──────────────────────────────────┐
│        You have died.            │
│                                  │
│   [Return to Save Point]        │
│                                  │
│   (or wait for resurrection)     │
└──────────────────────────────────┘
```

- The dialog appears after a short delay (1-2 seconds) following death
- "Return to Save Point" is a clickable button
- The "wait for resurrection" text is informational only
- If a Priest casts Resurrection on you, an additional dialog appears asking to accept or decline

### 7.5 Resurrection Acceptance Dialog

```
┌──────────────────────────────────┐
│   [PlayerName] is offering       │
│   to resurrect you.              │
│                                  │
│   [Accept]        [Decline]      │
└──────────────────────────────────┘
```

---

## 8. Vending UI

### 8.1 Player Vending (Merchant Class Only)

**Requirements:**
- Must be Merchant class or higher (Blacksmith, Whitesmith, Alchemist, Creator, etc.)
- Must have learned the **Vending** skill
- Must have items in **Pushcart** (separate from inventory — items must be moved to cart first)
- Must be at least 4 cells away from any NPC
- Character must remain online while vending (AFK vending)

**Setup Process:**
1. Use Vending skill (from skill tree or hotbar)
2. **Item Selection Window** opens: shows all items in Pushcart
3. Select items to sell (up to Vending skill level + 2 items, max 12)
4. Set individual price for each item (1z to 1,000,000,000z)
5. Enter shop title (displayed above character head)
6. Confirm to open shop

**Vending Shop Title:**
- Appears as a banner/sign above the character's head
- Maximum ~40 characters
- Visible to all nearby players
- Character sits on the ground with the cart while vending

**Commission Fee:**
- Items priced over 10,000,000z incur a 5% commission fee
- Vendor receives 95% of the listed price for high-value items

### 8.2 Browsing Player Shops

**Interaction:**
1. Click on a vending character (or their shop title)
2. **Shop Browse Window** opens showing:
   - Shop title at top
   - List of items for sale: Item Name | Price | Quantity Available
   - Total Zeny display (your current funds)

**Buying:**
1. Click an item in the shop list
2. Select quantity (slider or number input)
3. Total cost calculated and displayed
4. Click "Buy" to purchase
5. Zeny deducted, item added to your inventory
6. If the vendor sells out of all items, the shop closes automatically

### 8.3 Buying Store (Renewal Feature)

**Requirements:**
- Must have learned **Open Buying Store** skill
- Must have at least one copy of each item you want to buy already in inventory
- Must have sufficient Zeny for all listed buy orders

**Setup:**
1. Use Open Buying Store skill
2. Select items to buy and set prices
3. Enter store title
4. Store opens — displays with a different color sign than vending shops

**Interaction for Sellers:**
1. Click on a buying store character
2. See list of items they want to buy with offered prices
3. Select items from your inventory to sell to them
4. Confirm transaction

### 8.4 NPC Shops

**Buy from NPC:**
1. Click NPC -> Select "Buy" option
2. Shop window shows: Item Name | Price
3. Select items and quantities
4. Click "Buy" -> Zeny deducted, items added to inventory

**Sell to NPC:**
1. Click NPC -> Select "Sell" option
2. Your inventory opens with sell prices shown
3. Select items and quantities to sell
4. Click "Sell" -> Items removed, Zeny added
5. NPC sell price is typically 50% of the item's base price (varies)

**Deal/Trade Window:**
1. Right-click player -> "Trade" (or `/deal`)
2. Both players see a split window: Your offer (left) | Their offer (right)
3. Drag items from inventory to your offer side
4. Enter Zeny amount to include
5. Both players must click "Accept" for the trade to complete
6. Click "Trade" to finalize after both accept
7. Two-step confirmation prevents scams

---

## 9. Notifications

### 9.1 Level Up

**Base Level Up:**
- Golden pillar of light shoots up from character
- "Base Level Up!" text appears above character in large yellow font
- Sparkle/confetti particle effect surrounds character
- All nearby players see the effect
- Sound effect plays (fanfare)
- Chat message: "Base Level Up! [XX]"

**Job Level Up:**
- Similar but smaller effect — blue/white pillar
- "Job Level Up!" text above character
- Sound effect (slightly different from base level)
- Chat message: "Job Level Up! [XX]"

### 9.2 Item Pickup

- Item name appears in chat window: "Picked up [Item Name]" (or "Obtained [Item Name]")
- For rare items: larger font or different color in chat
- For cards: special sound effect + chat notification
- For Zeny: "Picked up XXX Zeny"
- No overhead popup — notification is in the chat window only

### 9.3 Quest Notifications

- **New Quest Available**: Yellow "!" marker above quest NPC
- **Quest Objective Complete**: Pop-up notification above character head (kill counter: "5/10 Poring")
- **Quest Turn-in Ready**: Yellow "?" marker above turn-in NPC, scroll icon at bottom-right of screen
- **Quest Complete**: System message in chat, scroll icon notification

### 9.4 System Messages

System messages appear in the chat window in special colors:

| Message Type | Color | Example |
|-------------|-------|---------|
| Server announcement | Yellow (bold) | "Server will restart in 10 minutes" |
| Login notification | White | "[FriendName] has logged in" |
| Party notification | Green | "[Name] has joined the party" |
| Guild notification | Cyan | "[Name] has joined the guild" |
| Trade notification | White | "Trade complete" |
| Error message | Red | "You cannot use this item" |
| Item obtained | White | "Picked up Red Potion" |
| Skill failure | Red | "Skill failed" (with /skillfail enabled) |
| Weight warning | Red | "You are overweight!" |
| Combat log | Gray | "You dealt 150 damage to Poring" |
| EXP gained | White/Yellow | "+50 Base EXP, +25 Job EXP" |
| Zeny gained | White | "Picked up 150 Zeny" |

### 9.5 Connection Notifications

- "Connected to server" — displayed on login
- "Disconnected from server" — displayed when connection is lost
- "Server is full" — displayed when server population is at max
- Map loading progress bar during zone transitions

---

## 10. UE5 Implementation

This section provides Unreal Engine 5 / Slate C++ implementation guidance specific to Sabri_MMO.

### 10.1 Architecture: UWorldSubsystem Pattern

Every UI panel in Sabri_MMO follows the **UWorldSubsystem + Slate widget** pattern. This keeps UI logic in C++, avoids Blueprint widget overhead, and supports multiplayer PIE testing.

**Pattern:**
```
UWorldSubsystem (C++)
├── Owns Slate widget lifecycle (Create / Destroy)
├── Listens to Socket.io events via SocketManager
├── Bridges server data → widget display
├── Handles show/hide toggle (keyboard shortcut)
└── SCompoundWidget (Slate)
    ├── Pure rendering — no game logic
    ├── Reads data from subsystem
    ├── Implements OnPaint() for custom rendering
    └── Handles mouse/keyboard input for UI interaction
```

**Existing Subsystems (reference implementations):**

| Subsystem | Widget | Z-Order | Toggle |
|-----------|--------|---------|--------|
| `BasicInfoSubsystem` | `SBasicInfoWidget` | Z=10 | Always visible |
| `CombatStatsSubsystem` | `SCombatStatsWidget` | Z=12 | F8 |
| `InventorySubsystem` | `SInventoryWidget` | Z=14 | F6 |
| `EquipmentSubsystem` | `SEquipmentWidget` | Z=15 | F7 |
| `HotbarSubsystem` | `SHotbarRowWidget` x4 | Z=16 | F5 |
| `SkillTreeSubsystem` | `SSkillTreeWidget` | Z=20 | K (Blueprint IMC) |
| `DamageNumberSubsystem` | `SDamageNumberOverlay` | Z=20 | Always visible |
| `CastBarSubsystem` | `SCastBarOverlay` | Z=25 | Always visible |
| `KafraSubsystem` | `SKafraWidget` | Z=19 | NPC interaction |
| `WorldHealthBarSubsystem` | `SWorldHealthBarOverlay` | Z=8 | Always visible |
| `ZoneTransitionSubsystem` | `SLoadingOverlayWidget` | Z=50 | Auto (zone change) |

**Z-Order Strategy:**
- Z=5-10: Persistent HUD (basic info, health bars)
- Z=10-20: Toggleable panels (inventory, equipment, skills, stats)
- Z=20-25: Overlays (damage numbers, cast bars)
- Z=25-30: Keybind config, modal dialogs
- Z=50: Loading overlay (blocks everything)

### 10.2 Slate (C++) Approach

**Why Slate over UMG/Blueprints:**
- No reflection overhead — raw C++ performance
- Full control over rendering via `OnPaint()`
- Multiplayer-safe (no global widget singletons)
- Deterministic lifecycle management
- Hot-reload friendly with Live Coding

**Widget Creation Pattern:**
```cpp
// In UMySubsystem::Initialize(FSubsystemCollectionBase& Collection)
void UMySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    // NEVER use GEngine->GameViewport — use World->GetGameViewport()
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    TSharedRef<SMyWidget> Widget = SNew(SMyWidget)
        .OwningSubsystem(this);

    ViewportClient->AddViewportWidgetContent(
        SNew(SWeightedWidget)
        .Widget(Widget)
        .Weight(MY_ZORDER)
    );

    MyWidgetPtr = Widget;
}
```

**Key Rules:**
1. **NEVER use `GEngine->GameViewport`** — it is a global singleton pointing to PIE-0's viewport. Always use `World->GetGameViewport()`.
2. **NEVER use `GEngine->AddOnScreenDebugMessage`** for per-player feedback.
3. Always access world-scoped objects through `GetWorld()`.
4. Every new widget must be tested with 2+ PIE instances.

### 10.3 Input Mode Management

RO-style games need both game input (click-to-move, camera) and UI input (window interaction) simultaneously.

**Input Mode: GameAndUI**
```cpp
// Set in PlayerController — allows both game clicks and UI interaction
FInputModeGameAndUI InputMode;
InputMode.SetWidgetToFocus(nullptr); // Don't lock focus to any widget
InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
InputMode.SetHideCursorDuringCapture(false); // CRITICAL — always show cursor
PlayerController->SetInputMode(InputMode);
PlayerController->bShowMouseCursor = true;
```

**Input Priority:**
1. Modal dialog widgets (NPC dialog, trade) — consume all input
2. Focused UI widgets (inventory drag, chat input) — consume relevant input
3. Game world (click-to-move, click-to-attack) — receives remaining input

**Keyboard Shortcut Handling:**
- Subsystems register for key events in `SetupPlayerInputComponent` on the character class
- Use Enhanced Input with `IMC_MMOCharacter` Input Mapping Context
- UI toggle keys (F5-F8, K) call subsystem `ToggleVisibility()` functions
- Chat input captures keyboard when focused, releases on Enter/Escape

### 10.4 Drag-and-Drop System

**Implemented Pattern (Inventory/Equipment/Hotbar):**
```
FDraggedItem struct (CharacterData.h)
├── ItemData: FInventoryItem
├── Source: EItemDragSource (Inventory, Equipment, Hotbar)
├── SourceSlot: int32
└── SourceRow: int32 (for hotbar)

Drag State Owner: InventorySubsystem (canonical)
├── StartDrag(FDraggedItem)
├── GetDraggedItem() → optional<FDraggedItem>
├── EndDrag()
└── IsDragging() → bool
```

**Drag Flow:**
1. Mouse down on item icon → start drag, store `FDraggedItem` in `InventorySubsystem`
2. Widget renders a "ghost" icon following the cursor during drag
3. Mouse up over a valid drop target → execute drop action
4. Drop targets validate the operation (e.g., can this item go in this equipment slot?)
5. Send server event (e.g., `inventory:move`, `hotbar:save`, `inventory:equip`)
6. EndDrag() clears the drag state

**Cross-Widget Drops:**
- Inventory → Equipment: `inventory:equip`
- Equipment → Inventory: `inventory:unequip`
- Inventory → Hotbar: `hotbar:save`
- Skill Tree → Hotbar: `hotbar:save_skill`
- Inventory → Ground (off any panel): `inventory:drop`

### 10.5 World-to-Screen Projection

Used by `DamageNumberSubsystem`, `WorldHealthBarSubsystem`, and `CastBarSubsystem` to render UI elements at world positions.

**Pattern:**
```cpp
// In widget's OnPaint():
FVector2D ScreenPos;
bool bOnScreen = false;

APlayerController* PC = GetWorld()->GetFirstPlayerController();
if (PC)
{
    FVector WorldLocation = TargetActor->GetActorLocation() + FVector(0, 0, HeightOffset);
    bOnScreen = PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPos);
}

if (bOnScreen)
{
    // Apply DPI scaling
    float DPIScale = /* get from geometry */;
    ScreenPos /= DPIScale;

    // Render at ScreenPos
    FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry,
        DamageText, Font, ScreenPos, Color);
}
```

**Performance Considerations:**
- Project only for visible actors (frustum cull first)
- Limit the number of simultaneous floating elements (pool damage numbers, cap at ~20)
- Use `OnPaint()` for batch rendering instead of individual widgets per damage number

### 10.6 RO Brown/Gold UI Theme

The classic RO UI has a distinctive visual style that should be replicated.

**Color Palette:**
| Element | Color (Approx. Hex) | Description |
|---------|---------------------|-------------|
| Window Background | `#2B1F14` / `#3D2E1E` | Dark brown, semi-transparent |
| Window Border | `#8B7355` / `#A0875C` | Medium golden-brown |
| Window Title Bar | `#5C4033` / `#6B4C35` | Darker brown gradient |
| Title Text | `#FFD700` / `#E8C44A` | Gold/amber |
| Body Text | `#FFFFFF` | White |
| Disabled Text | `#808080` | Gray |
| HP Bar Fill | `#00B050` / `#228B22` | Green |
| HP Bar Low | `#FF4444` | Red (below 25%) |
| SP Bar Fill | `#4169E1` / `#3366CC` | Blue |
| EXP Bar Fill | `#DAA520` / `#FFB347` | Gold/orange |
| Button Normal | `#5C4033` | Brown |
| Button Hover | `#7B5B3A` | Lighter brown |
| Button Pressed | `#3D2E1E` | Darker brown |
| Button Text | `#FFD700` | Gold |
| Item Slot BG | `#1A1A1A` | Near-black |
| Item Slot Border | `#555555` | Dark gray |
| Tooltip BG | `#000000CC` | Black, 80% opacity |
| Scrollbar Track | `#2B1F14` | Dark brown |
| Scrollbar Thumb | `#8B7355` | Golden-brown |
| Tab Active | `#5C4033` | Brown (raised) |
| Tab Inactive | `#2B1F14` | Dark brown (recessed) |

**Window Frame Structure:**
```
┌─ Title Bar (gradient brown, gold text, close/minimize buttons) ─┐
│                                                                  │
│  Window Content Area                                             │
│  (dark brown background, white text)                             │
│                                                                  │
│  ┌─ Sub-panels ─┐                                               │
│  │ Darker inset │  Scrollbar (brown track, gold thumb)          │
│  └──────────────┘                                               │
│                                                                  │
│  [Button 1] [Button 2]     (brown bg, gold text, gold border)  │
└──────────────────────────────────────────────────────────────────┘
```

**Style Notes:**
- Window backgrounds have slight texture/noise (parchment-like feel)
- Borders are 2-3px with subtle bevel/emboss effect (light on top-left, shadow on bottom-right)
- Close button: small "X" in top-right corner, red on hover
- Minimize button: small "-" next to close button
- Buttons have subtle gradient (lighter at top, darker at bottom)
- Hover states lighten the element slightly
- Press states darken and remove the bevel (flat appearance)
- Selected tabs appear raised/lighter, unselected tabs appear recessed/darker
- HP/SP bars have dark navy borders (RO Classic style: navy border, green HP, blue SP)

### 10.7 Performance: Widget Pooling

**Damage Numbers:**
- Pre-allocate a pool of N damage number render slots (e.g., 20-30)
- When a new damage number is needed, reuse the oldest expired slot
- Each slot tracks: text, color, world position, animation timer, opacity
- All slots rendered in a single `OnPaint()` pass — no per-number widget allocation

**World Health Bars:**
- Only render for actors within camera view frustum
- Maximum visible bars capped (e.g., 50)
- Priority: party members > targeted enemy > nearby enemies > nearby players
- Bars are rendered as primitives in `OnPaint()`, not as separate widgets

**Hotbar:**
- 4 rows x 9 slots = 36 max widgets
- Use `SCanvas` or custom `OnPaint()` rather than 36 individual `SButton` widgets
- Icon textures loaded once and cached (brushes stored in subsystem)

**General Optimization Rules:**
1. Prefer `OnPaint()` custom rendering over deeply nested Slate widget trees
2. Batch draw calls: render all items of the same type in one pass
3. Cache frequently accessed data (don't query `GameInstance` every frame)
4. Use `SetVisibility(EVisibility::Collapsed)` instead of removing/re-adding widgets
5. Avoid `Tick()` on widgets — use timers or event dispatchers
6. Invalidation box: wrap volatile widget sections in `SInvalidationPanel` for Slate batching

### 10.8 Implementation Roadmap: Remaining RO UI Windows

Based on current Sabri_MMO implementation status and this document, the following windows remain to be built:

| Window | Priority | Subsystem Pattern | Notes |
|--------|----------|-------------------|-------|
| Chat Window (multi-tab) | High | `ChatSubsystem` | Expand existing global chat to tabs, whisper, party, guild |
| Party Window | High | `PartySubsystem` | HP bars, member list, share settings |
| Status Window (full) | Medium | Expand `CombatStatsSubsystem` | Add stat allocation "+" buttons |
| Death/Respawn Dialog | Medium | `DeathSubsystem` | Save point return, resurrection acceptance |
| Quest Log | Medium | `QuestSubsystem` | Active/inactive tabs, objective tracking |
| Minimap | Medium | `MinimapSubsystem` | 2D overhead map, player/NPC dots |
| World Map | Low | `WorldMapSubsystem` | Full-screen, click-to-zoom |
| Guild Window | Low | `GuildSubsystem` | Tabs: Info, Members, Position, Skills |
| Friends List | Low | `FriendsSubsystem` | Online status, whisper shortcut |
| Trade Window | Medium | `TradeSubsystem` | Two-panel drag-and-drop, dual confirmation |
| Vending Setup/Browse | Low | `VendingSubsystem` | Merchant-only, shop title, price setting |
| NPC Shop (Buy/Sell) | Medium | `NPCShopSubsystem` | Item list, quantity, Zeny |
| Notification System | Medium | Integrated | Level up, item pickup, quest progress |

### 10.9 Multiplayer-Safe Widget Checklist

Every new UI widget or subsystem MUST pass this checklist before shipping:

- [ ] Uses `World->GetGameViewport()` instead of `GEngine->GameViewport`
- [ ] Uses `GetWorld()->GetFirstPlayerController()` scoped to its own world
- [ ] No static or global state shared between PIE instances
- [ ] Tested with 2+ PIE instances simultaneously
- [ ] `FInputModeGameAndUI` with `SetHideCursorDuringCapture(false)`
- [ ] Widget properly destroyed in `Deinitialize()` (no dangling Slate references)
- [ ] `OnMouseButtonDown` returns `FReply::Unhandled()` for clicks outside interactive areas
- [ ] Z-order does not conflict with existing widgets
- [ ] Socket event listeners unregistered in `Deinitialize()` to prevent crashes

---

## Appendix A: RO Classic UI Dimensions Reference

These are approximate pixel dimensions for common UI elements at 1024x768 (RO's native resolution):

| Element | Width | Height |
|---------|-------|--------|
| Basic Info Window | 280px | 120px |
| Status Window | 280px | 350px |
| Equipment Window | 280px | 350px |
| Inventory Window | 280px | 350px |
| Skill Tree Window | 280px | 400px |
| Hotbar (1 row) | 295px | 35px |
| Chat Window (default) | 400px | 150px |
| Minimap | 128px | 128px |
| Party Member Entry | 200px | 30px |
| Quest Sidebar | 180px | variable |
| Status Icon | 24px | 24px |
| Item Icon (inventory) | 24px | 24px |
| NPC Dialog Window | 400px | 250px |
| Trade Window | 400px | 300px |

**Scaling Note:** Modern implementations should scale all UI elements relative to viewport resolution. A 1920x1080 monitor should render UI at roughly 2x the original dimensions, with a user-configurable UI scale slider (50%-200%).

---

## Appendix B: Chat Color Quick Reference (Implementation)

```cpp
// Suggested FLinearColor constants for Sabri_MMO chat system
namespace ChatColors
{
    const FLinearColor Public      = FLinearColor::White;                         // #FFFFFF
    const FLinearColor WhisperSent = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);     // #FFD700 Gold
    const FLinearColor WhisperRecv = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);     // #FFD700 Gold
    const FLinearColor Party       = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);      // #00FF00 Green
    const FLinearColor Guild       = FLinearColor(0.0f, 1.0f, 0.8f, 1.0f);      // #00FFCC Cyan-green
    const FLinearColor System      = FLinearColor(0.53f, 0.81f, 0.98f, 1.0f);   // #87CEEB Light blue
    const FLinearColor Error       = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);      // #FF3333 Red
    const FLinearColor Combat      = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);      // #B3B3B3 Light gray
    const FLinearColor ItemLink    = FLinearColor(0.4f, 0.6f, 1.0f, 1.0f);      // #6699FF Blue
    const FLinearColor GMBroadcast = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);      // #FFFF00 Bright yellow
    const FLinearColor QuestUpdate = FLinearColor(1.0f, 0.65f, 0.0f, 1.0f);     // #FFA500 Orange
}
```

---

## Appendix C: Damage Number Color Quick Reference (Implementation)

```cpp
// Suggested FLinearColor constants for Sabri_MMO damage number system
namespace DamageColors
{
    const FLinearColor Normal      = FLinearColor::White;                         // Standard hit
    const FLinearColor Critical    = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);      // Yellow + red burst
    const FLinearColor MultiTotal  = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);      // Yellow combined total
    const FLinearColor MultiSingle = FLinearColor(0.8f, 0.8f, 0.8f, 0.6f);      // Faded white per-hit
    const FLinearColor Heal        = FLinearColor(0.3f, 1.0f, 0.3f, 1.0f);      // Green for healing
    const FLinearColor Miss        = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);      // Red "Miss" text
    const FLinearColor SkillMagic  = FLinearColor(0.6f, 0.8f, 1.0f, 1.0f);      // Light blue for magic
    const FLinearColor Immune      = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);      // Gray "0" for immune
}
```

---

## Appendix D: Status Icon Z-Order & Screen Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ [Minimap]                                    [Buff][Buff][Buff] │  <- Status icons (right edge)
│                                              [Buff][Buff]       │
│                                              [Debf]             │
│                                                                 │
│                         GAME WORLD                              │
│                                                                 │
│                    [Name]                                       │
│                    [Chat Bubble Text]                            │
│                    [Cast Bar ████░░]                             │
│                    [HP ████████░░░░]    <- World health bars     │
│                    [SP ████░░░░░░░░]                             │
│                    CHARACTER SPRITE                              │
│                    [Damage Numbers Float Up]                     │
│                                                                 │
│                                              [Quest Sidebar]    │
│ [Party Window]                               [Quest 1: 5/10]   │
│ [Member1 ████]                               [Quest 2: Done!]  │
│ [Member2 ██░░]                                                  │
│                                                                 │
│ ┌─Chat Window──────────────────┐  [F1][F2][F3]...[F9] Hotbar   │
│ │ [Public] System: Welcome!    │                                │
│ │ [Name]: Hello everyone       │  [Basic Info Window]           │
│ │ [Whisper from X]: Hey        │  Lv 50 Wizard    HP: 2000/3000│
│ └──────────────────────────────┘  SP: 500/800  Weight: 150/1200│
│ [Whisper To:___] [Message:_________________________] [Send]     │
└─────────────────────────────────────────────────────────────────┘
```

---

*Document Version: 1.0*
*Last Updated: 2026-03-08*
*Research Sources: iRO Wiki, Ragnarok Fandom Wiki, StrategyWiki, rAthena forums, OnlineGameCommands.com, WarpPortal support, RateMyServer.Net*
