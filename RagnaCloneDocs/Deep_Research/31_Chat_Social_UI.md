# Chat, Social & UI -- Deep Research (Pre-Renewal)

> **Scope**: Comprehensive reference for all chat, social, emote, UI/UX, character creation, and sitting/standing systems in Ragnarok Online Classic (pre-renewal).
> **Sources**: iRO Wiki Classic, rAthena/Hercules source, Ragnarok Fandom Wiki, RateMyServer, StrategyWiki, official playragnarok.com, WarpPortal forums, TalonRO Wiki, community guides.
> **Purpose**: Definitive implementation spec for replicating these systems in Sabri_MMO.

---

## Table of Contents

1. [Chat System](#1-chat-system)
2. [Emote System](#2-emote-system)
3. [Social Systems](#3-social-systems)
4. [UI/UX System](#4-uiux-system)
5. [Character Creation](#5-character-creation)
6. [Sitting/Standing System](#6-sittingstanding-system)
7. [Implementation Checklist](#7-implementation-checklist)
8. [Gap Analysis](#8-gap-analysis)

---

## 1. Chat System

### 1.1 Chat Channels

RO Classic uses a channel-based chat system where message routing is determined by a prefix typed before the message or a keyboard shortcut. There is no channel-selection dropdown -- channels are inline.

| Channel | Prefix / Trigger | Color in Chat Log | Overhead Bubble | Range | Notes |
|---------|-----------------|-------------------|-----------------|-------|-------|
| **Public (Normal)** | (none -- just type and press Enter) | White | Yes -- white text above head | ~14x14 cells (~700 UE units) | Default channel. All nearby players see the message. |
| **Whisper (PM)** | Type name in left input box, or `/w "name" message` | Yellow / Gold | No | Server-wide (unlimited) | Private message to one player. Both sender and recipient see the message in yellow. |
| **Party** | `%` prefix before message, or Ctrl+Enter to toggle party mode | Green | Yes -- green bubble | Unlimited (party members only) | Only visible to party members regardless of map. |
| **Guild** | `$` prefix or `/gc message`, or Alt+Enter to toggle guild mode | Bright Green / Cyan | No | Unlimited (guild members only) | Only visible to guild members regardless of map. |
| **Broadcast (GM)** | `/b message` or `/nb message` (no name) | Yellow with special formatting | No (appears as system text) | Server-wide | GM-only. `/b` includes GM name, `/nb` omits it. |
| **Local Broadcast (GM)** | `/lb message` or `/nlb message` | Yellow | No | Current map only | GM-only, map-scoped. |

**Pre-renewal chat did NOT have:**
- Clan chat (`/cl`) -- added in Renewal
- Battle chat (`/battlechat`) -- added for Battlegrounds in Renewal
- System message channel in the chat window -- system messages (item pickup, EXP gain, damage log) were NOT shown in pre-renewal chat. This was added in the Renewal UI update (2009+).

**Chat Text Colors in Message Window:**

| Message Type | Color |
|-------------|-------|
| Own public chat | White |
| Others' public chat | White |
| Whisper sent | Yellow / Gold |
| Whisper received | Yellow / Gold |
| Party chat | Green |
| Guild chat | Bright Green / Cyan |
| System messages | Light Blue / Cyan |
| GM broadcasts | Yellow (bold/special) |
| Error messages | Red |
| Item links | Blue (clickable, Renewal+) |

### 1.2 Chat Window Structure

**Components:**
- **Message Display Area**: Scrollable text area showing all chat messages. Pre-renewal had no filtering tabs -- all messages appeared in a single stream (public, whisper, party, guild mixed together, differentiated by color).
- **Input Bar**: Single-line text input at the bottom of the chat window.
- **Whisper Target Field**: A small text input field to the LEFT of the main input bar. Typing a name here and pressing Enter sends the main input as a whisper to that name. Equivalent to `/w "name" message`.
- **Lock Icon**: Locks/unlocks the chat box for resizing/splitting (post-Renewal feature; pre-renewal had fixed behavior).

**Chat Window Controls:**

| Key | Action |
|-----|--------|
| Enter | Opens/focuses the chat input bar. Press Enter again to send the message. |
| F10 | Cycles chat window height (3 sizes: small ~4 lines, medium ~8 lines, large ~16 lines) |
| F11 | Show/hide all chat windows |
| Page Up / Page Down | Scroll through message history |
| Backspace | Reply to last whisper received (fills the whisper target field) |
| Ctrl+Enter | Toggles party chat mode (all subsequent messages sent as party chat) |
| Alt+Enter | Toggles guild chat mode (all subsequent messages sent as guild chat) |

**Display Behavior:**
- New messages appear at the bottom; older messages scroll up.
- The chat window has a slight semi-transparent dark background for readability.
- When not focused, the chat window still shows incoming messages.
- Messages have a maximum display length -- very long messages are wrapped to multiple lines.
- There is no character limit enforced client-side for typing, but the server caps message length (approximately 255 characters).

### 1.3 Overhead Chat Bubbles

- Public chat messages appear as white text above the character's head.
- Party chat appears as green text above the character's head.
- Guild chat does NOT produce overhead bubbles.
- Whispers do NOT produce overhead bubbles.
- Bubbles fade out after approximately 5 seconds.
- Text has a slight black outline/shadow for readability against any background.
- Multiple rapid messages replace the previous bubble (only one bubble visible per character at a time).

**Bubble Toggle Commands:**
- `/notalkmsg` -- Disables overhead chat bubbles (messages still appear in chat log).
- `/notalkmsg2` -- Hides ALL chat (both window messages and bubbles).

### 1.4 Chat Commands (Complete Pre-Renewal List)

#### Communication Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `/w "name" message` | `/whisper` | Send a private whisper to the named character. Name must be in quotes if it contains spaces. |
| `% message` | (Ctrl+Enter toggle) | Send message to party members only. |
| `$ message` | `/gc message` | Send message to guild members only. |
| `/chat` | Alt+C | Create a chat room above your character. |
| `/q` | -- | Leave a chat room you are currently in. |
| `/ex [name]` | -- | Block whispers from the specified character. |
| `/exall` | -- | Block ALL incoming whispers from everyone. |
| `/in [name]` | -- | Unblock whispers from the specified character. |
| `/inall` | -- | Unblock all whispers (reverse of `/exall`). |
| `/hi [message]` | -- | Send a greeting message to ALL online friends simultaneously. |
| `/em [message]` | `/am` | Set an auto-reply message. When someone whispers you, they automatically receive this message. Used for AFK notifications. |
| `/savechat` | -- | Save the current chat log to a text file on the client machine. |

#### Party & Guild Commands

| Command | Description |
|---------|-------------|
| `/organize [name]` | Create a new party with the given name. |
| `/invite [name]` | Invite the named player to your party. |
| `/leave` | Leave your current party. |
| `/expel [name]` | Expel the named player from your party (leader only). |
| `/accept` | Enable incoming party invitations. |
| `/refuse` | Auto-decline all incoming party invitations. |
| `/guild [name]` | Create a new guild (requires an Emperium item in inventory). |
| `/breakguild [name]` | Disband the guild (guild master only, requires typing the guild name exactly). |
| `/emblem` | Toggle guild emblem display above characters on/off. |
| `/call` | Teleport all guild members to the Guild Master's location (WoE only). |

#### Display & Settings Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `/effect` | -- | Toggle spell visual effects on/off. |
| `/mineffect` | `/minimize` | Reduce particle/spell graphics to minimal versions. |
| `/bgm` | `/music` | Toggle background music on/off. |
| `/bv [0-127]` | -- | Set BGM volume (0 = mute, 127 = max). |
| `/sound` | -- | Toggle sound effects on/off. |
| `/v [0-127]` | -- | Set SFX volume (0 = mute, 127 = max). |
| `/fog` | -- | Toggle fog rendering on/off. |
| `/camera` | -- | Toggle camera zoom capability. |
| `/lightmap` | -- | Toggle map lighting effects. |
| `/quake` | -- | Toggle screen shake effects. |
| `/skip` | -- | Toggle frame skip (performance option). |
| `/window` | -- | Toggle window snap/docking behavior. |
| `/font` | -- | Toggle description text positioning. |
| `/stateinfo` | -- | Toggle status icon hover descriptions. |
| `/monsterhp` | -- | Toggle monster HP bars overhead. |
| `/showname` | -- | Change character name font style. |
| `/aura` | -- | Reduce Lv99+ character aura effect. |
| `/aura2` | -- | Disable Lv99+ character aura completely. |
| `/eqopen` | -- | Make your equipment viewable by other players who inspect you. |

#### Combat Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `/noctrl` | `/nc` | Auto-attack without holding Ctrl. Left-clicking a monster starts attacking. |
| `/noshift` | `/ns` | Use support skills on players without holding Shift. |
| `/battlemode` | `/bm` | Enable battle keyboard layout (QWERTY = hotbar). See Hotbar section. |
| `/skillfail` | `/sf` | Toggle red text notification when a skill fails. |
| `/miss` | -- | Toggle miss notification display. |
| `/snap` | -- | Toggle position snapping in combat. |
| `/skillsnap` | -- | Toggle snap for skill targeting. |
| `/itemsnap` | -- | Toggle snap for item pickup. |
| `/notrade` | `/nt` | Auto-decline all incoming trade requests. |

#### Navigation & Information Commands

| Command | Description |
|---------|-------------|
| `/where` | Display your current map name and cell coordinates in the chat log. |
| `/who` | Display the number of players currently online. |
| `/memo` | Memorize current location as a Warp Portal destination (Acolyte/Priest skill). |
| `/loginout` or `/li` | Show guild/friends online status notifications. |

#### Utility Commands

| Command | Description |
|---------|-------------|
| `/sit` | Sit down (same as Insert key). |
| `/stand` | Stand up. |
| `/bangbang` | Rotate character clockwise. |
| `/bingbing` | Rotate character counter-clockwise. |
| `/emotion` | Open the emote selection window. |
| `/check [text]` | Creates green-colored text in chat. |
| `/set1` | Macro: enables `/noctrl` + `/showname` + `/skillfail`. |
| `/set2` | Macro: enables `/skillfail` + `/showname` + `/noctrl` + `/q1` + `/q2` + `/window`. |
| `/tip` | Open "Tip of the Day" dialog. |
| `/h` or `/help` | Show list of available commands. |
| `/buildinfo` | Show client build version. |

#### Skill Shortcut Commands

| Command | Description |
|---------|-------------|
| `/q1` or `/quickspell1` | Right-click uses the skill in F9 slot. |
| `/q2` or `/quickspell2` | Mouse wheel up/down uses F7/F8 slot skills. |
| `/q3` or `/quickspell3` | Enables both `/q1` and `/q2` simultaneously. |

#### Ranking Commands

| Command | Description |
|---------|-------------|
| `/alchemist` | Show top 10 Alchemists ranking. |
| `/blacksmith` | Show top 10 Blacksmiths ranking. |
| `/taekwon` | Show top 10 Taekwon Kids ranking. |
| `/pk` | Show top 10 Player Killers ranking. |

#### AI Commands (Homunculus/Mercenary)

| Command | Description |
|---------|-------------|
| `/hoai` | Toggle Homunculus AI mode (custom AI script support). |
| `/merai` | Toggle Mercenary AI mode. |
| `/traceai` | Save Homunculus AI debug log to file. |

#### GM-Only Broadcast Commands

| Command | Description |
|---------|-------------|
| `/b [message]` or `@broadcast` | Global broadcast with GM name (yellow text). |
| `/nb [message]` | Global broadcast without GM name (yellow text). |
| `/lb [message]` or `@localbroadcast` | Map-only broadcast with GM name (yellow text). |
| `/nlb [message]` | Map-only broadcast without GM name (yellow text). |
| `@kami [message]` | Global announcement without name (yellow). |
| `@kamib [message]` | Global announcement (blue text). |
| `@kamic [RRGGBB] [message]` | Global announcement in specified hex color. |
| `@lkami [message]` | Local (map-only) announcement without name. |

### 1.5 Chat Room System

Chat rooms are private conversation spaces created by players. They function separately from the main chat system.

**Creation:**
- Created via `/chat` command or Alt+C hotkey.
- A clickable speech bubble icon appears above the creator's character head.
- The creator sets:
  - **Title**: Free-text title displayed on the speech bubble (max ~40 characters).
  - **Maximum Members**: 1-20 players.
  - **Password**: Optional. If set, the room is "private" and requires the password to enter.
  - If no password, the room is "public" and anyone can click to enter.

**Behavior:**
- Other players click the speech bubble to enter the room.
- If the room has a password, a password prompt dialog appears before entry.
- Chat within the room is completely private -- messages are visible ONLY to members inside the room.
- Room chat does NOT appear in the main chat window.
- The main chat window continues to work normally while in a chat room.
- Each player can create a maximum of one chat room at a time.

**Member Management:**
- The room owner (creator) can:
  - **Kick** members by right-clicking their name in the member list.
  - **Change settings** (title, member limit, password) via right-click -> Room Setup.
  - **Transfer ownership** to another member.
- If the owner leaves, ownership transfers to the next member automatically.
- Members leave by closing the chat room window or typing `/q`.

**Common Uses:**
- Private group conversations.
- AFK signage (create a chat room titled "AFK" or "BRB" -- the speech bubble serves as a visible notice).
- Deal/trade organization (title: "Buying +7 Blade [4] - PM me").
- Party recruitment (title: "LF>Priest for Odin's Temple party").
- The chat room bubble was the primary "signage" system before the vending shop title was introduced.

**Save Chat:**
- Right-clicking the member list provides a "Save Chat as Text File" option.
- Saves the entire room conversation to a text file on the client machine.

### 1.6 Chat Filters and Blocking

**Whisper Blocking:**

| Command | Effect |
|---------|--------|
| `/ex [name]` | Block whispers from a specific character. Their messages are silently dropped. |
| `/exall` | Block ALL incoming whispers from ALL players. |
| `/in [name]` | Unblock a specific character (reverse of `/ex`). |
| `/inall` | Unblock all whispers (reverse of `/exall`). |

**Behavior Details:**
- Block lists are **per-character**, not per-account. Blocking "SomePlayer" on your Knight does not block them on your Wizard.
- When blocked, the sender receives the message: "Name is ignoring you" (or similar notification that the whisper was not delivered).
- `/exall` is commonly used to avoid spam/bot whispers.
- The block list persists across sessions (saved server-side or in client data files depending on implementation).
- Blocking a character blocks both their whispers AND their public speech from appearing in your chat log.

**Other Filter Commands:**
- `/notrade` or `/nt` -- Auto-decline all incoming trade requests.
- `/refuse` -- Auto-decline all incoming party invitations.
- `/notalkmsg` -- Hides overhead chat bubbles (messages still appear in the chat log).
- `/notalkmsg2` -- Hides ALL chat messages (both window and bubbles).

---

## 2. Emote System

### 2.1 Overview

Emotes in RO Classic are small animated icons that appear above a character's head. They are purely cosmetic and have no gameplay effect. Emotes are triggered via slash commands or Alt+number keyboard shortcuts.

**Access Methods:**
- **Alt+L**: Opens the Emote Window, showing a grid of all available emote icons. Click one to display it.
- **Alt+M**: Opens the Macro Window, where players can assign emotes, phrases, or commands to Alt+0 through Alt+9.
- **Slash commands**: Type the emote command in the chat input (e.g., `/lv` for heart).
- **Alt+Number**: By default, Alt+1 through Alt+0 are mapped to common emotes. These can be reassigned via the Macro Window.

### 2.2 Complete Emote List

#### Standard Emotes (Slash Commands)

| ID | Command | Default Alt Key | Visual Description |
|----|---------|----------------|-------------------|
| 0 | `/!` | -- | Exclamation mark (surprise) |
| 1 | `/?` | -- | Question mark (confusion) |
| 2 | `/ho` | -- | Music note (humming/bored) |
| 3 | `/lv` | Alt+1 | Beating heart (love) |
| 4 | `/lv2` | -- | Exploding/sparkling heart (deep love) |
| 5 | `/swt` | -- | Sweat drop (nervous/awkward) |
| 6 | `/ic` | -- | Light bulb (idea/realization) |
| 7 | `/an` | -- | Anger vein (irritated) |
| 8 | `/ag` | -- | Storm cloud (furious) |
| 9 | `/$` | -- | Dollar sign (money/greed) |
| 10 | `/...` | -- | Three dots (speechless/ellipsis) |
| 11 | `/scissors` | -- | Scissors (Rock-Paper-Scissors) |
| 12 | `/rock` | -- | Rock fist (Rock-Paper-Scissors) |
| 13 | `/paper` | -- | Paper hand (Rock-Paper-Scissors) |
| 14 | `/thx` | -- | Thank you / grateful face |
| 15 | `/wah` | -- | Panic / distress face |
| 16 | `/sry` | -- | Sorry / apologetic face |
| 17 | `/heh` | -- | Laughing / amused face |
| 18 | `/swt2` | -- | Explosive sweat drop |
| 19 | `/hmm` | -- | Thinking / pondering face |
| 20 | `/no1` | -- | Thumbs up wink (praise/"you're #1") |
| 21 | `/??` | -- | Shaking head (disagreement/disapproval) |
| 22 | `/omg` | -- | Shocked / alarmed face |
| 23 | `/oh` | -- | Red circle sign (OK/correct) |
| 24 | `/X` | -- | Red X sign (no/refuse/wrong) |
| 25 | `/hlp` | -- | Help signs (urgent plea for help) |
| 26 | `/go` | -- | Explosive "Go!" sign (encouragement) |
| 27 | `/sob` | -- | Crying face (sobbing) |
| 28 | `/gg` | -- | Evil giggle (mischief/grinning) |
| 29 | `/kis` | -- | Kiss facing right |
| 30 | `/kis2` | -- | Kiss facing left |
| 31 | `/pif` | -- | Blowing smoke (cigarette puff) |
| 32 | `/ok` | -- | Head nod (agreement/confirmation) |

#### Extended Emotes (/e# Commands)

| Command | Alt Command | Visual Description |
|---------|------------|-------------------|
| `/bzz` | `/e1` | Angry buzzing vein |
| `/rice` | `/e2` | Drooly face (hungry) |
| `/awsm` | `/e3` | Heart eyes (amazed/awestruck) |
| `/meh` | `/e4` | Tongue out (indifferent/teasing) |
| `/shy` | `/e5` | Blushing face (embarrassed) |
| `/pat` | `/e6` | Petting hand (comfort) |
| `/mp` | `/e7` | SP low indicator (out of mana) |
| `/slur` | `/e8` | Lustful/dazed face |
| `/com` | `/e9` | Beckoning finger (come here) |
| `/yawn` | `/e10` | Big yawn (sleepy/bored) |
| `/grat` | `/e11` | Congratulations sparkle |
| `/hp` | `/e12` | HP low indicator (low health warning) |
| `/fsh` | `/e13` | Shiny eyes (desire/want) |
| `/spin` | `/e14` | Dizzy spiral (confused/spinning) |
| `/sigh` | `/e15` | Exhale puff (tired/resigned) |
| `/dum` | `/e16` | Blank expression (dumbfounded) |
| `/crwd` | `/e17` | Loud clamoring (noise) |
| `/desp` | `/otl` or `/e18` | Giving up / OTL stick figure (despair) |
| `/dice` | `/e19` | Random dice roll (shows 1-6 result) |
| `/e20` | -- | Pointing upward |
| `/hum` | `/e27` | Annoyed expression |
| `/abs` | `/e28` | Soul leaving body (extreme embarrassment) |
| `/oops` | `/e29` | Red-faced mistake |
| `/spit` | `/e30` | Spitting/vomiting |
| `/ene` | `/e31` | Blessed/energized face |
| `/panic` | `/e32` | Frightened expression |
| `/whisp` | `/e33` | Spinning whisper icon |
| `/dbc` | -- | Logs breaking apart |

#### Rock-Paper-Scissors Special Commands

| Command | Shortcut | Action |
|---------|----------|--------|
| `/bawi` | Ctrl+= | Rock (fist raise) |
| `/bo` | Ctrl+\ | Paper (open palm) |
| `/gawi` | Ctrl+- | Scissors (peace sign) |

### 2.3 Emote Display Mechanics

- Emotes appear as small animated sprites (approximately 32x32 pixels) above the character's head.
- Each emote animation plays once and lasts approximately 2-3 seconds.
- Only one emote can display at a time per character -- a new emote replaces the current one.
- Emotes are visible to all nearby players within render range (~14x14 cells).
- Emotes are client-side broadcast -- the server relays the emote ID to all nearby clients.
- The `/dice` emote is special: it displays a random number (1-6) above the character's head, determined server-side.
- Emotes can be dragged from the Emote Window to hotbar slots for quick access.

### 2.4 Macro System (Alt+M)

- The Macro Window allows assigning content to Alt+0 through Alt+9 (10 macro slots).
- Each slot can contain:
  - An emote (drag from Emote Window).
  - A text phrase (typed in -- pressing the Alt+# key sends the phrase to public chat).
  - A slash command (e.g., `/sit`).
- Default Alt+1 is mapped to the heart emote (`/lv`). The rest are unassigned by default.
- Macros persist across sessions (saved in client configuration).

---

## 3. Social Systems

### 3.1 Friend List

**Access:** Alt+H hotkey opens the Friend List window.

**Capacity:** Maximum **40 friends** per character.

**Scope:** Friend lists are **character-bound**, not account-bound. Each character on the same account has a separate friend list.

**Adding Friends:**
1. Right-click another player's character on screen.
2. Select "Register as Friend" from the context menu.
3. The target player receives a confirmation dialog: "[Name] wants to register you as a friend. Accept?"
4. **Both players must accept** -- friendship is mutual. If either declines, the friend entry is not created.
5. Alternative: Open the Friend Setup window (Ctrl+I) and type a character name to send a friend request.

**Friend List Display:**
- Online friends show an "ON" indicator / icon next to their name.
- Offline friends show no indicator (or a dimmed/gray appearance).
- Friends are listed alphabetically.

**Notifications:**
- When a friend logs in, a notification appears in the chat log: "[FriendName] has logged in."
- When a friend logs out: "[FriendName] has logged out."
- These notifications only appear while you are online.

**Interactions:**
- Double-click a friend's name to open a whisper window to them.
- Right-click a friend's name for options:
  - **Whisper** -- Opens whisper input targeting that friend.
  - **Remove** -- Removes the friend from your list (does NOT require the other player's consent for removal).
  - **Block** -- Adds the friend to your block list.

**`/hi` Command:**
- `/hi [message]` sends a greeting message to ALL online friends simultaneously.
- Appears as a whisper from you to each friend.
- Commonly used when logging in to say hello to all friends at once.

### 3.2 Block / Ignore List

**Commands:**

| Command | Effect |
|---------|--------|
| `/ex [name]` | Block the named character. Their whispers are silently dropped. Their public chat is hidden from your chat log. |
| `/exall` | Block ALL incoming whispers from all players. Public chat still visible. |
| `/in [name]` | Unblock a specific character. |
| `/inall` | Unblock all whispers (reverse of `/exall`). |

**Behavior:**
- Block list is **per-character**, not per-account.
- The blocked player receives a message indicating their whisper was not delivered (e.g., "[YourName] is not receiving whispers" or "[YourName] is ignoring you").
- Block lists persist across sessions.
- There is no hard cap documented for the block list size.
- Blocking via right-click context menu on a player character is also available (same as `/ex [name]`).
- Blocking does NOT prevent you from seeing the player's character in the game world -- it only filters their chat messages.

### 3.3 Marriage System

The Marriage System allows two players to become spouses, unlocking exclusive marriage skills and participating in the adoption system.

#### Requirements

| Requirement | Details |
|-------------|---------|
| Base Level | Both players must be Base Level 45 or higher. |
| Gender | One male, one female character (same-sex marriage not supported in classic). Pre-2015, gender was account-locked; post-2015, gender is per-character. |
| Adoption Status | Neither player can be an adopted "Baby" character. |
| Marriage Status | Neither player can already be married. |

#### Required Items

| Item | Source | Cost |
|------|--------|------|
| Wedding Dress (bride) | Wedding Shop Dealer NPC (Prontera or Lighthalzen) | ~43,000z |
| Tuxedo (groom) | Wedding Shop Dealer NPC | ~43,000z |
| Diamond Ring x2 | Wedding Shop Dealer NPC | ~45,000z each |
| Written Oath of Marriage / Marriage Covenant | Wedding Present NPC (prt_in 287/162) or Cash Shop | 20,000,000z (NPC) or 500 coins (~$5 USD, Cash Shop) |

Both bride and groom each need their own Marriage Covenant (or Written Oath).

#### Ceremony Process

1. **Prepare items**: Both players acquire their respective outfit (dress/tuxedo), a Diamond Ring each, and a Marriage Covenant each.
2. **Go to Prontera Church**: Located in the northeast area of Prontera city.
3. **Talk to the Marriage NPC**: Either the groom or bride initiates by speaking to the NPC first.
4. **Partner confirms**: The other spouse must speak to the same NPC **within 1 minute** or the application expires.
5. **Proceed to altar**: Both players walk to the front of the church where the Priest NPC delivers the ceremony.
6. **Final confirmation**: The Priest asks both parties to confirm. Both must agree.
7. **Marriage complete**: Ceremony plays out, both characters are placed in formal wear.

#### Post-Ceremony Effects

- Both characters are placed in their wedding outfit (dress/tuxedo) and **cannot attack or use skills for 1 in-game hour** after the ceremony.
- Diamond Rings transform into **Wedding Rings**.
- Wedding Rings must be **identified using a Magnifier** before marriage skills become available.
- Both spouses must equip their Wedding Ring to use marriage skills.

#### Marriage Skills

| Skill | Effect | Cost | Cast Time |
|-------|--------|------|-----------|
| **I Will Follow You** (Call Partner) | Teleport your spouse to your location. | -- | 15 seconds |
| **I'll Save You** | Restore HP to your spouse. Consumes 10% of YOUR Max HP. | 10% Max HP | Instant |
| **I'll Sacrifice Myself for You** | Restore SP to your spouse. Consumes 10% of YOUR Max SP. | 10% Max SP | Instant |

These skills are ONLY available while the Wedding Ring is equipped.

#### Divorce

- **Location**: Niflheim -- inside the house at coordinates (167, 161), upstairs.
- **NPC**: A Deviruchi NPC handles divorce proceedings.
- **Cost**: 2,500,000 Zeny.
- **Effect**: Both players lose their married status, Wedding Rings are destroyed, and all marriage skills are removed. Any adopted children (Baby characters) lose their adopted status.

### 3.4 Adoption System

The Adoption System allows a married couple to "adopt" another player's Novice or First Class character, converting them into a Baby class.

#### Requirements

| Who | Requirement |
|-----|-------------|
| **Parents** | Must be married (active marriage). Both parents must be Base Level 70+. Both must have Wedding Rings equipped. |
| **Child** | Must be a Novice or First Class (Swordman, Mage, Archer, Acolyte, Merchant, Thief). Cannot already be adopted. Cannot be a Transcendent (Rebirth) class. |

#### Adoption Process

1. One parent right-clicks the child-to-be character.
2. Selects "Adopt" from the context menu.
3. The child receives a confirmation dialog.
4. If the child accepts, they become a Baby class (e.g., Baby Swordman, Baby Mage, etc.).
5. Their character sprite shrinks to a smaller "chibi" size.

#### Baby Class Penalties

| Penalty | Details |
|---------|---------|
| **HP/SP Reduction** | Baby characters have only **75% of normal Max HP and Max SP**. |
| **Stat Cap** | Cannot increase any base stat above **80** (vs. 99 for normal characters). |
| **Weight Reduction** | Carrying capacity reduced by **1,200** weight units. |
| **No Rebirth** | Cannot Transcend / Rebirth. Cannot access Transcendent skills or the +100 bonus stat points from rebirth. |
| **Forge/Brew Penalty** | Adopted Blacksmiths and Alchemists have **50% reduced** Forge/Brewing success rates. |
| **Size** | Character is treated as **Small** size (normal is Medium), which affects damage calculations from size modifiers. |

#### Family Skills

| Skill | User | Effect |
|-------|------|--------|
| **"Mom, Dad, I love you!"** | Child | Parents receive **no death penalty** for 2 minutes. Costs 10% of the child's Max SP. |
| **"Mom, Dad, I miss you!"** | Child | Summons both parents to the child's location (must be on the same map). |
| **"Go! Parents Go!"** | Child (affects parents) | Increases ALL parent stats by +3 for 60 seconds. Affects parents within a 7x7 cell radius. |

---

## 4. UI/UX System

### 4.1 Main HUD Elements

The RO Classic HUD is minimalist and modular. Most elements are small, draggable windows that players arrange freely. The default layout:
- Basic Info Window: top-center
- Chat Window: bottom-left
- Minimap: top-right
- Hotbar: bottom-center

#### Basic Info Window (Alt+V)

The always-visible status display showing at-a-glance character information.

| Field | Format | Details |
|-------|--------|---------|
| Character Name | Text | Display name |
| Base Level | `Lv XX` | 1-99 in pre-renewal |
| Job Class | Text | e.g., "Swordman", "Wizard" |
| HP Bar | Green bar + `current / max` | Turns yellow at ~25%, red at ~10% |
| SP Bar | Blue bar + `current / max` | Blue fill with numeric overlay |
| Base EXP Bar | Yellow/orange bar + `XX.XX%` | Progress to next base level |
| Job EXP Bar | Yellow/orange bar + `XX.XX%` | Progress to next job level |
| Weight | `current / max` | Color-coded by weight threshold |
| Zeny | Gold number | Currency with comma separators |

**Features:**
- Small dropdown arrow provides access to other windows (Status, Inventory, Equipment, Skill, Map, Quest, Guild, Party, Options).
- Minimize button collapses to just the name/level line.
- HP/SP bars show both colored fill and numeric overlay.

**Weight Thresholds:**
- 0-49%: Normal (white text)
- 50-69%: Warning (yellow text, no mechanical penalty)
- 70-89%: Overweight -- HP/SP natural regen disabled, item-creation skills blocked
- 90-100%: Severely Overweight -- cannot attack or use any skills

#### Minimap (Ctrl+Tab)

- Top-right corner, 2D overhead view of current map.
- Player = white arrow/dot pointing in facing direction.
- Other players = white dots.
- Party members = pink/magenta dots.
- NPCs = colored dots (varies by type -- see NPC icon table below).
- Warp portals = red dots.
- Map name displayed at top.

**NPC Minimap Icons:**

| Icon | NPC Type |
|------|----------|
| Magnifying glass | Guide NPC |
| Bed | Inn / Save Point |
| Chest | Storage NPC (Kafra) |
| Potion | Tool Dealer |
| Sword | Weapon Dealer |
| Shield | Armor Dealer |
| Hammer | Repairman |
| Yellow + | Quest NPC |
| Red dot | Portal/Warp |
| Blue dot | Generic NPC |

**Controls:**
- Ctrl+Tab cycles opacity: Opaque -> Semi-transparent -> Transparent -> Hidden -> Opaque.
- Scroll wheel zooms in/out.

#### World Map (Ctrl+~)

- Full-screen overlay showing the entire game world.
- Click a region to zoom in.
- Party members shown as pink dots.
- Zone names labeled. Warp connections shown between maps.
- Click outside or Escape to close.

### 4.2 Equipment Window (Alt+Q / Ctrl+Q)

**Equipment Slots (Classic Layout):**

| Slot | Position | Notes |
|------|----------|-------|
| Top Headgear | Top-center | Hats, helms, crowns |
| Mid Headgear | Center-left | Glasses, masks (eye area) |
| Lower Headgear | Bottom-left | Mouth accessories, pipes |
| Armor (Body) | Center | Main body armor |
| Weapon (Right Hand) | Right | All weapon types |
| Shield (Left Hand) | Left | Shields; empty for two-handed weapons |
| Garment | Back-center | Cloaks, mantles, robes |
| Shoes (Footgear) | Bottom-center | Boots, sandals |
| Accessory 1 | Bottom-right | Rings, clips |
| Accessory 2 | Bottom-right-2 | Second accessory |

**Features:**
- Character sprite preview in the center (shows equipped headgears/garments).
- Drag-and-drop from inventory to equip.
- Right-click to unequip back to inventory.
- Hover for item tooltip with stats, card slots, requirements.
- Empty slots shown as faded outlines.
- Card slots displayed as small squares on item tooltips (0-4 cards).

### 4.3 Inventory Window (Alt+E / Ctrl+E)

**Layout:**
- Grid of item icons (~10 columns wide).
- Three tabs: **Item** (consumables/usables) | **Equip** (weapons/armor) | **Etc** (materials/misc).
  - Renewal added a **Favorites** tab -- not present in pre-renewal.
- Weight display at bottom: `current / max`.
- Zeny display at bottom.

**Item Display:**
- Each slot shows the item icon.
- Stackable items show count in bottom-right corner.
- Equipment shows refine level as `+X` prefix on the name.
- Right-click to use consumables or equip/unequip gear.
- Drag to trade window, hotbar, equipment window, or ground (drop).
- Items fill from top-left. Dropping/using items leaves gaps that fill on next pickup.

**Item Tooltips:**
- Item name (white text for normal items).
- Item type and subtype.
- Weight.
- Description text.
- Stats/effects.
- Card slots: `[Card Name]` if filled, `[Slot Available]` if empty.
- Class/level requirements.
- Sell price.

**Capacity:** Unlimited slots in pre-renewal -- limited only by weight. (Renewal added 100-slot cap per tab.)

### 4.4 Skill Tree Window (Alt+S)

**Layout:**
- Tree-style layout with skill icons connected by prerequisite lines.
- Each icon shows `current level / max level`.
- Skills with unmet prerequisites are grayed out.
- Skills that can be leveled show a "+" button or highlighted border.
- Skill points remaining displayed at top.

**Interaction:**
- Click "+" to add one skill point (confirmation dialog appears).
- Right-click a skill for detailed description: name, type (passive/active/supportive), target type, SP cost per level, cast time, cooldown, damage formula, range.
- Drag skill icon to hotbar to assign it to a slot.

**Skill Type Visual Indicators:**
- Passive: Blue icon border.
- Active Offensive: Red/orange icon border.
- Supportive: Green icon border.
- Toggle: Yellow icon border with on/off state.

### 4.5 Status Window (Alt+A / Ctrl+A)

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

**Additional Info:**
- Status Points remaining to allocate.
- Guild name (if applicable).
- "+" buttons next to each base stat for allocation.
- Stat increase cost shown (increases with stat value).
- Bonus column shows equipment/buff modifiers in green.

**Stat Point Cost Formula (Classic Pre-Renewal):**
```
Cost = floor((current_base_stat - 1) / 10) + 2
```
Stat 1->2 costs 2 points; stat 10->11 costs 3; stat 20->21 costs 4, etc.

### 4.6 Quest Window (Alt+U)

**Layout:**
- Two tabs: **Active** and **Inactive**.
- Active quests shown in a scrollable list with name and brief status.
- Right-click a quest to move between Active/Inactive (no effect on quest progress).
- First 5 active quests can appear as a sidebar tracker on the right side of the screen.

**Quest Entry Details:**
- Quest name.
- Objectives (kill X, collect Y, visit NPC Z).
- Progress indicators (e.g., "5/10 Poring killed").
- Map name and NPC for next step.
- Time limit (if applicable).

**Visual Feedback:**
- Kill counter popup above character head when a quest monster is killed.
- Scroll icon at bottom-right when an objective is completed.
- Quest NPCs: yellow "!" above head (new quest), "?" above head (turn-in ready).

### 4.7 Party Window (Alt+Z)

**Display per Member:**

| Element | Details |
|---------|---------|
| Character name | Text |
| Job class icon | Small sprite next to name |
| HP bar | Green bar (no numeric value in classic) |
| Map name | Which map the member is on |
| Online indicator | Highlighted if online |

**Features:**
- Maximum 12 party members.
- Party leader indicated by crown/star icon.
- Lock icon at bottom toggles between "click name to whisper" and "click name to cast skill on member".
- Party share settings: EXP share (Even Share / Each Take), Item share (Individual / Shared).
- Right-click member name for options: Whisper, Expel, Follow.
- Click the circle/map indicator to open world map centered on that member.
- Mini party display: compact HP-bar-only version for less screen clutter.
- Heal skills can be targeted by clicking party member HP bars directly.

### 4.8 Guild Window (Alt+G)

**Tabs:**

| Tab | Contents |
|-----|----------|
| **Guild Info** | Guild name, level, EXP bar, emblem (24x24 BMP/GIF), master name, member count, average level, notice/announcement |
| **Members** | Full member list: name, class, level, title/position, online status, contribution |
| **Position** | Define guild titles (up to 20), set permissions (invite, expel, storage access), set EXP tax rate per position |
| **Skills** | Guild skills tree (Guild Extension, Guardian Research, etc.) |
| **Expulsion** | List of expelled members with reason |
| **Notice** | Guild master writes announcements (title + body) shown to members on login |
| **Alliance** | Allied and enemy guild lists |

**Guild Emblem:**
- 24x24 pixels, BMP or GIF format (animated GIF supported).
- Hot pink (RGB 255,0,255) renders as transparent.
- Displayed above character name, in guild window, and during WoE.
- Maximum guild size: 16 base, up to 56 with Guild Extension skill levels.

### 4.9 Options / Settings

Accessed via Alt+O or Ctrl+O. Settings include:

- BGM Volume (0-127)
- SFX Volume (0-127)
- Effect Toggle (on/off)
- Minimap Transparency
- Chat Display Options
- Snap Settings (position snap, skill snap, item snap)
- Character Name Display Style

Most settings are also accessible via slash commands (see section 1.4).

### 4.10 Hotbar System (F12 to toggle)

#### Pre-Renewal Hotbar

- **3 rows** of 9 slots each = **27 total shortcut slots**.
- Pressing F12 cycles which row is visible: Row 1 -> Row 2 -> Row 3 -> Row 1.
- Only **one row visible at a time** in pre-renewal (Renewal allowed 4 rows visible simultaneously).
- Slots are bound to **F1-F9** keys. The active row determines which row F1-F9 triggers.

**Row Key Mappings (Default, non-Battle Mode):**

| Row | Keys | Visible |
|-----|------|---------|
| Row 1 | F1-F9 | Default visible row |
| Row 2 | F1-F9 | Visible after pressing F12 once |
| Row 3 | F1-F9 | Visible after pressing F12 twice |

#### Battle Mode (/bm or /battlemode)

When Battle Mode is enabled, ALL three hotbar rows are active simultaneously via the keyboard:

| Row | Keys (Battle Mode) |
|-----|-------------------|
| Row 1 | Z, X, C, V, B, N, M, < (comma), > (period) |
| Row 2 | Q, W, E, R, T, Y, U, I, O |
| Row 3 | A, S, D, F, G, H, J, K, L |

**Battle Mode Rules:**
- Normal typing is disabled while in Battle Mode.
- To type in chat, press **Enter** first to open the chat input bar, type your message, then press Enter to send.
- After sending, the keyboard returns to Battle Mode mapping.
- Battle Mode is toggled via `/bm` command.
- Battle Mode is the standard setup for PvP and high-intensity PvE play.

#### Slot Contents

Hotbar slots can contain:
- **Skills** (dragged from Skill Tree window).
- **Consumable items** (dragged from Inventory).
- **Equipment items** (for quick weapon-swapping).
- **Emotes** (dragged from Emote window).

#### Visual Feedback on Hotbar Slots

- **Cooldown overlay**: Clock-wipe darkening animation sweeps across the slot icon during cooldown.
- **SP cost**: Small text below the skill icon showing SP cost (optional display).
- **Greyed out**: Slot appears dimmed when SP is insufficient or skill is on cooldown.
- **Item count**: Consumable items show remaining stack count on the slot.
- **Skill level**: Small text showing the skill level.

### 4.11 Window Behavior & Management

#### Dragging
- All RO windows are draggable by their title bar.
- Windows can be placed anywhere on screen.
- Windows are clamped to viewport bounds (cannot be dragged off-screen).
- Each window remembers its last position (saved to client config).

#### Resizing
- Most windows have fixed sizes (Status, Equipment, Skill Tree, Inventory).
- Chat Window is vertically resizable via F10 (three height presets) and horizontally by dragging the edge.

#### Z-Order & Focus
- Clicking a window brings it to the front.
- Most recently interacted window is always on top.
- Modal dialogs (NPC dialog, trade confirmation) block input to all other windows.
- Game world remains clickable through gaps between windows.
- Overlapping windows do NOT consume clicks meant for windows beneath them -- only the topmost window at the click position receives the event.

#### Window Snapping
- `/window` toggles snap behavior.
- When enabled, dragging windows near each other snaps edges together.
- Windows also snap to screen edges.
- Snap threshold: approximately 10 pixels.

#### Keyboard Shortcuts (Complete Reference)

**Window Toggles:**

| Shortcut | Window |
|----------|--------|
| Alt+A / Ctrl+A | Status Window |
| Alt+E / Ctrl+E | Inventory |
| Alt+Q / Ctrl+Q | Equipment |
| Alt+S | Skill Tree |
| Alt+V | Basic Info Window |
| Alt+Z | Party Window |
| Alt+G | Guild Window |
| Alt+H | Friends List |
| Alt+I | Friends Setup |
| Alt+U | Quest Window |
| Alt+O / Ctrl+O | Options Window |
| Alt+C | Chat Room |
| Alt+J | Pet Window |
| Alt+R | Homunculus Window |
| Alt+W | Cart Window (Merchant class) |
| Alt+L | Emote Window |
| Alt+M | Macro Window |
| Alt+P | Party Setup |
| Alt+Y | Command List |
| Alt+T | Standby/Idle Mode |

**Function Keys:**

| Key | Action |
|-----|--------|
| F1-F9 | Activate hotbar slot 1-9 |
| F10 | Cycle chat window height |
| F11 | Show/hide all chat windows |
| F12 | Cycle hotbar row (1->2->3->1) |

**Other Shortcuts:**

| Shortcut | Action |
|----------|--------|
| Insert | Sit / Stand toggle |
| Escape | Open game menu / close topmost window |
| Tab | Cycle focus between open windows |
| Page Up / Down | Scroll text in focused window |
| Ctrl+Tab | Cycle minimap transparency |
| Ctrl+~ | Open world map |
| Ctrl+Enter | Party chat mode toggle |
| Alt+Enter | Guild chat mode toggle |
| Enter | Open/focus chat input bar |
| Backspace | Reply to last whisper (fills whisper target) |
| Alt+End | Toggle player HP/SP bars overhead |
| Alt+Home | Toggle ground cursor |
| Shift+Click Dir | Change character facing direction |
| Shift+Right-Click | Auto-follow target player |
| Print Screen | Screenshot |

#### Position Persistence
- Window positions saved to `savedata/OptionInfo.lua` in the client.
- Positions persist across sessions.
- Each character can have different window layouts.
- Positions reset if client resolution changes.

---

## 5. Character Creation

### 5.1 Character Creation Screen

The character creation screen presents a simple interface where players configure their new character before entering the game.

**Required Fields:**
1. Character Name
2. Hair Style
3. Hair Color
4. Starting Stat Distribution (9 points among STR/AGI/VIT/INT/DEX/LUK)

### 5.2 Gender Selection

**Pre-2015 (Classic Era):**
- Character gender was **locked to the account gender**, which was selected during account registration.
- Players could NOT choose a different gender per character.
- To create a character of the opposite gender, a separate account was required.

**Post-2015 Patch:**
- Gender can be specified per character during creation.
- No longer locked to account gender.
- Both male and female characters can exist on the same account.

**For Sabri_MMO Implementation:** Allow per-character gender selection (the post-2015 behavior is more player-friendly and modern).

### 5.3 Hair Styles

Players choose from hairstyles numbered **1 through 19** (classic pre-renewal). Each style has both male and female variants with different sprite art.

| Style # | Male Description | Female Description |
|---------|-----------------|-------------------|
| 1 | Short conservative cut | Short bob |
| 2 | Swept forward / spiky | Long tied back |
| 3 | Spiked up | Twin tails (pigtails) |
| 4 | Bowl cut | Side ponytail |
| 5 | Long flowing | Long wavy |
| 6 | Combed back | Short curly |
| 7 | Pompadour | Long straight with bangs |
| 8 | Mohawk-style | Bun/updo |
| 9 | Short messy | Short pixie |
| 10 | Medium swept | Medium with ribbon |
| 11 | Spiky wild | Twin braids |
| 12 | Bandana/headband | Long with headband |
| 13-19 | Various additional styles | Various additional styles |

**Post-creation hair changes:** Hair style and color can be changed in-game by visiting a **Stylist NPC** for 100,000 Zeny. The Stylist offers all 19 styles plus up to 8 additional premium styles in some server versions (styles 20-27 added in later patches).

### 5.4 Hair Colors

Players select from a palette of hair colors during character creation. The classic pre-renewal palette offers **9 base colors** (indices 0-8).

| Index | Color Name | Approximate RGB | Description |
|-------|-----------|-----------------|-------------|
| 0 | Default/Brown | -- | Default brown (varies by job class sprite) |
| 1 | Light Brown | -- | Lighter brown/sandy |
| 2 | Red/Auburn | -- | Red-tinted brown |
| 3 | Blond/Yellow | -- | Golden blonde |
| 4 | Light Blue | -- | Cool blue tint |
| 5 | Green | -- | Forest green tint |
| 6 | Purple/Violet | -- | Deep purple |
| 7 | Black | -- | Dark/jet black |
| 8 | White/Silver | -- | White or light gray |

**Technical Note:** Hair colors are stored as palette files (`.pal`) in the client data. Each hair style has a separate palette file per color index. The palette maps pixel indices in the hair sprite to specific RGB colors. Exact RGB values vary between sprite sets, so colors look slightly different across job classes.

**In-game color changes:** Same as hair style -- Stylist NPC for 100,000 Zeny provides access to all palette colors.

### 5.5 Name Restrictions

| Rule | Details |
|------|---------|
| **Length** | Minimum 4 characters, maximum 23 characters. |
| **Starting character** | Cannot start with a number. Must begin with a letter. |
| **Allowed characters** | Letters (A-Z, a-z), numbers (0-9), spaces, and underscores. No special/ASCII characters (!, @, #, etc.). |
| **Spaces** | Allowed in the name (e.g., "My Knight"). However, some server implementations disallow spaces. |
| **Uniqueness** | Character names must be globally unique across all characters on the server. |
| **Case sensitivity** | Names are case-insensitive for uniqueness checks ("Knight" and "knight" are the same). |
| **Profanity filter** | Server-side filter blocks offensive names. |

### 5.6 Starting Stat Distribution

- During character creation, players distribute **9 stat points** among STR, AGI, VIT, INT, DEX, and LUK.
- Each stat starts at 1 and can be raised to a maximum of 9 during creation.
- The total must equal 9 additional points (base stats of 1 each + 9 distributed = each stat between 1 and 9, summing to 15 total stat points across 6 stats).
- This initial distribution has minimal impact on gameplay but determines starting effectiveness.

---

## 6. Sitting/Standing System

### 6.1 Overview

Sitting is a core survival mechanic in RO Classic. Characters can sit down to increase their natural HP/SP regeneration rate. This is the primary way to recover between fights without using consumables.

**How to Sit/Stand:**
- **Insert key**: Toggles sit/stand.
- **`/sit` command**: Sit down.
- **`/stand` command**: Stand up.
- Clicking to move, using a skill, or being attacked automatically makes the character stand up.

### 6.2 Regeneration Bonus While Sitting

Sitting doubles the frequency of natural regeneration ticks. The regeneration amount per tick stays the same; the interval between ticks is halved.

#### HP Regeneration

| State | Regen Interval | Formula per Tick |
|-------|---------------|-----------------|
| **Standing / Walking** | Every **6 seconds** | `HPR = max(1, floor(MaxHP / 200)) + floor(VIT / 5)` |
| **Sitting** | Every **3 seconds** | Same formula, but ticks twice as often |

**Effective HP Regen Rate:**
- Standing: `HPR / 6` HP per second.
- Sitting: `HPR / 3` HP per second (2x standing rate).

#### SP Regeneration

| State | Regen Interval | Formula per Tick |
|-------|---------------|-----------------|
| **Standing / Walking** | Every **8 seconds** | `SPR = 1 + floor(MaxSP / 100) + floor(INT / 6)` |
| **Sitting** | Every **4 seconds** | Same formula, but ticks twice as often |

**High INT Bonus (Pre-Renewal):**
If INT >= 120: `SPR += floor(INT / 2 - 56)`

**Effective SP Regen Rate:**
- Standing: `SPR / 8` SP per second.
- Sitting: `SPR / 4` SP per second (2x standing rate).

#### Modifier Interactions

| Modifier | Effect on Regen |
|----------|----------------|
| **Magnificat** (Priest skill) | Halves the regen interval. Stacks with sitting: Standing + Magnificat = 4s HP / 4s SP. Sitting + Magnificat = 1.5s HP / 2s SP. |
| **Increase HP Recovery** (Priest skill) | Increases HPR by `(5*SkillLv + VIT*SkillLv/5) per tick. Applied before modifier step. |
| **Weight >= 70%** | Natural HP/SP regen is **completely disabled**. Sitting does not help. |
| **Weight >= 90%** | No regen, no attacking, no skills. |
| **Guild Castle Ownership** | Doubles SP regen while inside the guild's castle. |
| **Poison status** | HP regen disabled (HP actively drains). |

### 6.3 Skill Interactions with Sitting

Several skills interact specifically with the sitting state:

| Skill | Interaction |
|-------|------------|
| **Spirits Recovery** (Monk) | Restores additional HP and SP every 10 seconds while sitting. Works even during Fury (Zen) status. Requires Monk sitting specifically. |
| **Absorb Spirits** (Monk) | Can be used on targets to absorb spirit spheres. Sitting is a prerequisite state for some Monk meditation skills. |
| **Play Dead** (Novice) | Forces the character into a prone position (similar to sitting). Resets aggro on all monsters targeting the character. Cannot act while in Play Dead. HP/SP regen continues at the sitting rate. |
| **Zen / Critical Explosion** (Monk) | Requires sitting to use certain combo prerequisites in some implementations. The Monk class has unique sitting-related gameplay. |
| **Auto-stand triggers** | Using ANY skill, clicking to move, being knocked back, or being hit by certain status effects (Stun, Freeze, etc.) automatically forces the character to stand. |

### 6.4 Sitting Visual

- The character sprite changes to a sitting pose (legs crossed or kneeling depending on job class and gender).
- An audible "sitting down" sound plays.
- The character's name tag and all overhead elements remain visible.
- Other players can see your character sitting.
- Sitting characters are still targetable by enemies and players.

### 6.5 Sitting Restrictions

- Cannot sit while performing a skill animation.
- Cannot sit while in certain buff states (e.g., Endure movement lock, Cart Revolution animation).
- Cannot sit on cells occupied by certain objects (NPCs, portals) in some implementations.
- Cannot sit while vending (vending automatically puts the character in a sitting-like pose).
- Sitting is instantly cancelled by: moving, attacking, using skills, being hit by displacement effects (knockback), or Stun/Freeze/Stone.

---

## 7. Implementation Checklist

### Chat System

- [ ] **Public chat** -- Type + Enter, white text, ~14x14 cell range broadcast, overhead white bubble
- [ ] **Whisper system** -- `/w "name" message`, yellow text, server-wide delivery, whisper target input field
- [ ] **Party chat** -- `%` prefix or Ctrl+Enter toggle, green text, green overhead bubble, party-wide
- [ ] **Guild chat** -- `$` prefix or Alt+Enter toggle, cyan/green text, no bubble, guild-wide
- [ ] **Chat window** -- Scrollable message area, F10 resize (3 heights), F11 show/hide, Page Up/Down scroll
- [ ] **Overhead chat bubbles** -- White for public, green for party, ~5s fade, one bubble per character
- [ ] **Whisper blocking** -- `/ex [name]`, `/exall`, `/in [name]`, `/inall`, per-character block list
- [ ] **Auto-reply** -- `/em [message]` or `/am` sets AFK auto-response
- [ ] **Backspace reply** -- Backspace fills whisper target with last whisper sender
- [ ] **Chat rooms** -- `/chat` or Alt+C, title/password/member limit, speech bubble above head, private conversation
- [ ] **Chat room management** -- Owner kick, settings change, ownership transfer, `/q` to leave
- [ ] **`/savechat`** -- Save chat log to text file
- [ ] **GM broadcasts** -- `/b`, `/nb`, `/lb`, `/nlb`, `@kami`, `@kamib`, `@kamic`, `@lkami`

### Chat Commands

- [ ] `/noctrl` (auto-attack), `/noshift` (support without Shift), `/battlemode` (keyboard remap)
- [ ] `/effect`, `/mineffect` (VFX toggles)
- [ ] `/bgm`, `/bv`, `/sound`, `/v` (audio controls)
- [ ] `/where` (show map/coordinates)
- [ ] `/who` (player count)
- [ ] `/sit`, `/stand` (sitting toggle)
- [ ] `/organize`, `/invite`, `/leave`, `/expel` (party management)
- [ ] `/guild`, `/breakguild`, `/emblem`, `/call` (guild management)
- [ ] `/notrade` (auto-decline trades)
- [ ] `/accept`, `/refuse` (party invitation toggle)
- [ ] `/set1`, `/set2` (macro presets)
- [ ] `/q1`, `/q2`, `/q3` (skill shortcut modes)
- [ ] Ranking commands: `/alchemist`, `/blacksmith`, `/taekwon`, `/pk`
- [ ] AI commands: `/hoai`, `/merai`, `/traceai`

### Emote System

- [ ] **33+ standard emotes** via slash commands (/lv, /!, /?, etc.)
- [ ] **20+ extended emotes** via /e# commands
- [ ] **Rock-Paper-Scissors** emotes (Ctrl+=, Ctrl+\, Ctrl+-)
- [ ] **Emote Window** (Alt+L) -- grid of clickable emote icons
- [ ] **Macro Window** (Alt+M) -- assign emotes/phrases/commands to Alt+0-9
- [ ] **Emote display** -- 32x32 animated sprite above head, ~2-3 second duration, single emote at a time
- [ ] **/dice** emote -- server-determined random 1-6 result
- [ ] **Emote drag to hotbar** -- emotes assignable to hotbar slots

### Social Systems

- [ ] **Friend List** (Alt+H) -- 40 friends max, per-character, mutual acceptance required
- [ ] **Online status** -- "ON" indicator, login/logout chat notifications
- [ ] **Friend interactions** -- whisper, remove, block via right-click
- [ ] **`/hi` command** -- greeting to all online friends
- [ ] **Block list** -- `/ex`, `/exall`, `/in`, `/inall`, per-character persistence
- [ ] **Marriage system** -- Lv45+ requirement, ceremony at Prontera Church, item requirements
- [ ] **Marriage skills** -- Call Partner, HP share, SP share (require Wedding Ring equipped)
- [ ] **Divorce** -- Niflheim NPC, 2,500,000z cost
- [ ] **Adoption system** -- Married Lv70+ parents, Novice/1st Class child, Baby class conversion
- [ ] **Baby class penalties** -- 75% HP/SP, stat cap 80, -1200 weight, no rebirth, 50% forge/brew
- [ ] **Family skills** -- "Mom Dad I love you", "Mom Dad I miss you", "Go Parents Go"

### UI/UX System

- [ ] **Basic Info Window** (Alt+V) -- HP/SP bars, EXP bars, weight, zeny, level, class
- [ ] **Minimap** (Ctrl+Tab) -- Player/party/NPC dots, opacity cycling, zoom
- [ ] **World Map** (Ctrl+~) -- Full-screen overlay, party member dots
- [ ] **Equipment Window** (Alt+Q) -- 10 slots, sprite preview, drag-and-drop
- [ ] **Inventory Window** (Alt+E) -- 3 tabs (Item/Equip/Etc), grid layout, tooltips
- [ ] **Skill Tree Window** (Alt+S) -- Tree layout, prerequisite lines, skill point allocation
- [ ] **Status Window** (Alt+A) -- 6 stats + derived stats, stat point allocation
- [ ] **Quest Window** (Alt+U) -- Active/Inactive tabs, tracker sidebar
- [ ] **Party Window** (Alt+Z) -- Member list, HP bars, map names, leader icon
- [ ] **Guild Window** (Alt+G) -- 7 tabs, emblem, positions, skills
- [ ] **Options Window** (Alt+O) -- Volume, effects, display settings
- [ ] **Window dragging** -- All windows draggable by title bar, clamped to viewport
- [ ] **Window Z-ordering** -- Click to bring to front, modal dialogs block all
- [ ] **Window snapping** -- `/window` toggle, 10px threshold
- [ ] **Position persistence** -- Per-character window layout saving

### Hotbar System

- [ ] **3 rows x 9 slots** = 27 total (pre-renewal)
- [ ] **F12 cycling** -- Rotate visible row: 1->2->3->1
- [ ] **F1-F9 activation** -- Active row determines which slots F-keys trigger
- [ ] **Battle Mode** (/bm) -- All 3 rows mapped to Q-O / A-L / Z-M simultaneously
- [ ] **Battle Mode chat** -- Enter key opens chat input, keyboard returns to BM after sending
- [ ] **Slot types** -- Skills, consumables, equipment, emotes
- [ ] **Visual feedback** -- Cooldown clock-wipe, SP cost text, grayed when insufficient SP, item count, skill level

### Character Creation

- [ ] **Gender selection** -- Per-character (not account-locked)
- [ ] **19 hair styles** -- Male and female variants
- [ ] **9 hair colors** -- Palette indices 0-8
- [ ] **Character name** -- 4-23 chars, no starting numbers, no special chars, globally unique
- [ ] **Starting stat distribution** -- 9 points among 6 stats
- [ ] **Stylist NPC** -- In-game hair/color changes for 100,000z

### Sitting/Standing System

- [ ] **Insert key toggle** + `/sit` and `/stand` commands
- [ ] **HP regen**: 6s standing -> 3s sitting (2x rate)
- [ ] **SP regen**: 8s standing -> 4s sitting (2x rate)
- [ ] **Magnificat interaction** -- Halves interval further
- [ ] **Overweight block** -- No regen at 70%+ weight
- [ ] **Auto-stand triggers** -- Move, attack, skill use, knockback, stun/freeze/stone
- [ ] **Sitting visual** -- Character sprite changes to sitting pose
- [ ] **Monk Spirits Recovery** -- Extra HP/SP regen while sitting every 10s
- [ ] **Play Dead** -- Novice skill, resets aggro, regen continues at sitting rate

---

## 8. Gap Analysis

### Currently Implemented in Sabri_MMO

Based on the project's existing code and documentation:

| Feature | Status | Notes |
|---------|--------|-------|
| Public chat | DONE | White text, chat window, overhead bubbles |
| Party chat | DONE | % prefix, green text, Ctrl+Enter |
| Whisper system | DONE | `/w` command, yellow text |
| Guild chat | NOT STARTED | $ prefix, Alt+Enter |
| Chat window resize (F10) | DONE | 3 height presets |
| Whisper blocking (/ex, /exall) | DONE | Per-character block list |
| Auto-reply (/em) | NOT STARTED | AFK auto-response |
| Chat rooms | NOT STARTED | Full chat room system |
| Battle Mode (/bm) | NOT STARTED | Keyboard remapping |
| Emote system | NOT STARTED | Emote display + window + commands |
| Macro system (Alt+M) | NOT STARTED | Emote/phrase/command assignment |
| Friend list | NOT STARTED | Full friend list system |
| Marriage system | NOT STARTED | Full marriage + skills + divorce |
| Adoption system | NOT STARTED | Baby class + family skills |
| Hotbar | DONE | 3 rows, F12 cycling, F1-F9 |
| Sitting / standing | DONE | Insert toggle, regen bonus |
| Basic Info Window | DONE | HP/SP/EXP/weight/zeny |
| Equipment Window | DONE | 10 slots, drag-and-drop |
| Inventory Window | DONE | 3 tabs, grid, tooltips |
| Skill Tree Window | DONE | Tree layout, prerequisites |
| Status Window | DONE | Stats + derived stats |
| Party Window | DONE | Member list, HP bars |
| Quest Window | NOT STARTED | Quest tracking system |
| Guild Window | NOT STARTED | Full guild UI |
| Minimap | NOT STARTED | Map overlay with dots |
| World Map | NOT STARTED | Full-screen map overlay |
| Window snapping | NOT STARTED | Snap-to-edge/snap-to-window |
| Window position persistence | PARTIAL | Some windows save position |
| GM broadcasts | PARTIAL | Server can broadcast, no GM commands |

### Priority Implementation Order

1. **Emote System** -- High social value, relatively simple (sprite display + commands).
2. **Battle Mode** -- Essential for competitive play, high skill ceiling content.
3. **Friend List** -- Core social feature with online notifications.
4. **Guild Chat** -- Required before guild system implementation.
5. **Chat Rooms** -- Social hub feature, AFK signage.
6. **Auto-Reply (/em)** -- Small feature, high utility.
7. **Minimap** -- Navigation essential, moderate complexity.
8. **Quest Window** -- Required for quest content.
9. **Guild Window** -- Required for guild system.
10. **Marriage System** -- Social feature, moderate complexity.
11. **Adoption System** -- Depends on marriage, lower priority.
12. **World Map** -- Nice-to-have, lower priority.
13. **Window Snapping** -- Polish feature, lowest priority.

### Key Implementation Notes

1. **Chat rooms should use Socket.io rooms** -- Each chat room maps to a Socket.io room for message isolation. Room metadata (title, password, members, owner) stored server-side in memory (not DB -- chat rooms are transient).

2. **Emotes are a simple broadcast** -- Client sends `emote:display` with emote ID, server broadcasts to nearby players. No persistence needed.

3. **Friend list needs DB tables** -- `character_friends` table with `(character_id, friend_character_id)` + mutual acceptance flag. Online status from `connectedPlayers` map.

4. **Marriage needs DB support** -- `marriages` table with `(husband_id, wife_id, date)`. Marriage skills gated by `player.marriedTo` field + Wedding Ring equipped check.

5. **Battle Mode is client-only** -- No server changes needed. The client remaps keyboard input to different hotbar rows. The server just receives `skill:use` events as normal.

6. **Block list should be server-validated** -- When a whisper is sent, the server checks the target's block list before delivery. Client-side filtering alone is insufficient (bot whispers bypass client filters).

---

*Deep research document compiled from iRO Wiki Classic, rAthena source, Ragnarok Fandom Wiki, RateMyServer, StrategyWiki, official playragnarok.com, WarpPortal forums, TalonRO Wiki, and community guides.*
