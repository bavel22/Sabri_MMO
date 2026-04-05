# Party System -- Deep Research (Pre-Renewal)

> **Sources**: iRO Wiki Classic, iRO Wiki (Renewal reference for comparison), rAthena source (`party.cpp`, `party.conf`, `inter_athena.conf`), Ragnarok Fandom Wiki, StrategyWiki, RateMyServer, WarpPortal Forums, rAthena GitHub Issues, Hercules source reference.
> **Scope**: Pre-renewal (Classic) Ragnarok Online party mechanics, verified against rAthena pre-re configuration and official iRO documentation.
> **Date**: 2026-03-22

---

## Overview

The party system in Ragnarok Online Classic allows up to 12 players to group together for cooperative gameplay. Parties provide shared experience, coordinated item distribution, a dedicated chat channel, real-time HP/SP monitoring of allies, and enable party-restricted skills like Devotion and Gospel. The system is designed to incentivize group play through EXP bonuses while preventing power-leveling abuse through level range restrictions.

Key design principles:
- **Server-authoritative**: All EXP distribution, member management, and share settings are validated server-side.
- **Same-map requirement**: EXP sharing only occurs between members on the same map.
- **Level-gated sharing**: Even Share requires all online members to be within 15 base levels of each other.
- **Persistent across sessions**: Party membership survives logout/login. Members show as offline until they reconnect.
- **Leader-centric management**: Only the party leader can invite, kick, change EXP mode, and transfer leadership.

---

## Party Creation & Management

### Create Party

| Property | Value |
|----------|-------|
| **Command** | `/organize "Party Name"` |
| **Requirement** | Novice Basic Skill Level 7 |
| **Join requirement** | Novice Basic Skill Level 5 |
| **Max members** | 12 (leader + 11 members) |
| **Name length** | Up to 24 characters |
| **Creator role** | Becomes Party Leader automatically |
| **Settings at creation** | Item share mode is chosen at creation and **cannot be changed** without disbanding and re-creating the party |

The party creator must have Basic Skill Level 7. The target of an invite must have Basic Skill Level 5 to accept. There is no level requirement to *join* a party -- the level restriction only applies to activating Even Share EXP distribution.

A character can only belong to one party at a time. Characters from the same account cannot join the same party (rAthena `block_account_in_same_party: yes`).

### Invite Mechanics

| Property | Value |
|----------|-------|
| **Command** | `/invite "CharacterName"` or `/pinvite "CharacterName"` |
| **UI method** | "Party Invitation" button in party window |
| **Range** | Remote -- no map proximity required. Can invite from any map. |
| **Who can invite** | Party leader only |
| **Target requirements** | Must be online, must not already be in a party, must have Basic Skill Lv5 |
| **Invite expiry** | 30 seconds (rAthena default) |

The invited player receives a popup dialog with Accept/Reject options. If they do not respond within the timeout, the invite expires silently.

### Kick / Leave Mechanics

**Kick (Expel)**:
- Only the party leader can kick members.
- Command: Right-click the member's name in the party window (Alt+Z) and select "Kick from Party" or use `/expel "CharacterName"`.
- The kicked player receives a notification that they were removed.
- The leader cannot kick themselves (must use Leave instead).

**Leave**:
- Any member can leave voluntarily.
- Command: `/leave` or click "Party withdrew" in the party window.
- All remaining party members are notified.

**Offline removal**: If a member disconnects, they remain in the party as "offline." They are NOT automatically removed. Their party membership persists in the database and is restored when they log back in.

### Leader Transfer (Delegation)

| Property | Value |
|----------|-------|
| **Method** | Right-click target member in party window, select "party represent delegation" |
| **Requirement** | Current leader must be on the same map as the target (rAthena `change_party_leader_samemap: yes`) |
| **Who can transfer** | Current leader only |
| **Target** | Any online party member |

### Party Dissolution

A party is automatically disbanded when:
- The last member leaves or is kicked (0 members remaining).
- All members disconnect and the party has been empty for an extended period (server-dependent; rAthena keeps the party in DB until explicitly disbanded).

Manual dissolution is not a single command in Classic RO. The leader must kick all members or all members must leave individually. Some servers provide a `/party_disband` or equivalent admin command, but this is not part of the official Classic client.

**Automatic leader transfer**: If the leader leaves (via `/leave` or disconnect while other members remain online), leadership automatically transfers to the first remaining member in the member list.

---

## EXP Share Settings

### Each Take (Individual EXP)

| Property | Value |
|----------|-------|
| **Behavior** | Each player receives EXP only for monsters they personally dealt damage to |
| **Distribution** | Based on damage contribution (percentage of total damage dealt) |
| **Party bonus** | None -- standard solo EXP rules apply |
| **Level restriction** | None -- any level difference is allowed |
| **Default mode** | This is the default mode when a party is created |

In Each Take mode, EXP is awarded based on damage contribution. The player who dealt the most damage receives the largest share. If multiple non-party players attack the same monster, the tap bonus still applies (see below).

### Even Share

| Property | Value |
|----------|-------|
| **Behavior** | All EXP from kills by any party member is pooled and divided equally |
| **Distribution** | Equal split among all eligible members on the same map |
| **Party bonus** | +20% EXP per member starting from member #2 |
| **Level restriction** | All online members must be within 15 base levels of each other |
| **Map restriction** | Only members on the same map as the kill receive shared EXP |
| **Changeable** | Yes, by party leader at any time (if level restriction is met) |

### Even Share Formula

The pre-renewal Even Share formula, verified against iRO Wiki Classic and rAthena source:

```
Total EXP = Monster_Base_EXP * (1.0 + 0.20 * (eligible_member_count - 1))
Per-member EXP = floor(Total EXP / eligible_member_count) + 1
```

**Breakdown by party size** (eligible members on same map):

| Members | Total Multiplier | Per-Member Share | Effective Per-Person |
|---------|-----------------|-----------------|---------------------|
| 1 | 100% | 100% + 1 | 100% (solo) |
| 2 | 120% | 60% + 1 | 60% |
| 3 | 140% | ~47% + 1 | 47% |
| 4 | 160% | 40% + 1 | 40% |
| 5 | 180% | 36% + 1 | 36% |
| 6 | 200% | ~33% + 1 | 33% |
| 7 | 220% | ~31% + 1 | 31% |
| 8 | 240% | 30% + 1 | 30% |
| 9 | 260% | ~29% + 1 | 29% |
| 10 | 280% | 28% + 1 | 28% |
| 11 | 300% | ~27% + 1 | 27% |
| 12 | 320% | ~27% + 1 | 27% |

The "+1" EXP is per the iRO Classic wiki formula: `(EXP to be Shared / Number of Party Members) + 1 EXP`. This ensures that even with very low EXP monsters, each member receives at least 1 EXP.

Both Base EXP and Job EXP follow the same formula and distribution rules.

**rAthena implementation** (from `party.cpp`):
```cpp
double bonus = 100 + battle_config.party_even_share_bonus * c;
// where c = number of eligible party members
// party_even_share_bonus default = 0 in rAthena (must be configured)
// Official iRO Classic uses effectively party_even_share_bonus = 20
```

Note: rAthena's default `party_even_share_bonus` is 0 (no bonus). To replicate official iRO Classic behavior, it must be set to 20. Many private servers use different values (10, 15, 25).

### Level Range Restriction for Even Share

| Property | Value |
|----------|-------|
| **Range** | 15 base levels (rAthena `party_share_level: 15` in `inter_athena.conf`) |
| **Check** | `max_level - min_level <= 15` across ALL online party members |
| **When checked** | When leader attempts to switch to Even Share mode |
| **Offline members** | Excluded from the level check (only online members count) |
| **Join restriction** | None -- any level can JOIN a party. The restriction only blocks Even Share activation. |

**Example**: A party with levels 90, 98, 100, 102, 105 can use Even Share (range = 105 - 90 = 15). Adding a level 89 member would make the range 16, blocking Even Share.

**Note on 10 vs 15**: Some sources (including iRO Wiki Classic) reference a 10-level range. The official iRO Classic server historically used 15 levels (confirmed by rAthena default `party_share_level: 15` and the iRO Wiki main party page). The 10-level reference appears on some wiki pages and may reflect specific server configurations or episode-specific changes. **Use 15 for authentic Classic behavior.**

### Distance Requirement (Same Map)

| Property | Value |
|----------|-------|
| **Map requirement** | Members must be on the **same map** to receive shared EXP |
| **Cell distance** | No cell-distance restriction in pre-renewal. Entire map counts. |
| **Cross-map** | Members on different maps receive nothing from that kill |

In pre-renewal Classic RO, there is **no screen-range or cell-distance restriction** within the same map for Even Share. If you are on the same map, you receive your share regardless of how far apart you are on that map. This is different from Renewal, which introduced a 30x30 cell range restriction.

### Tap Bonus (Non-Party Mechanic, Interacts with Party EXP)

The tap bonus is a **separate mechanic from party EXP sharing** that applies regardless of party membership:

| Property | Value |
|----------|-------|
| **Bonus per attacker** | +25% EXP per additional player who hit the monster or was hit by its normal attacks |
| **Applies to** | All players who engaged the monster, party or not |
| **Cap** | 580% maximum (approximately 20 additional attackers beyond the first) |
| **Stacks with Even Share** | Yes -- tap bonus expands the total EXP pool BEFORE Even Share division |

**Formula**:
```
Tap_Modifier = 1.0 + 0.25 * (attacker_count - 1)
Capped at: Tap_Modifier <= 5.80
```

**Example**: If 3 players (not in a party) attack a monster worth 100 Base EXP:
- Tap bonus: 100 * (1 + 0.25 * 2) = 150 total EXP
- Each player receives EXP proportional to their damage contribution

**Interaction with Even Share**: If 3 party members in Even Share attack a monster worth 100 Base EXP:
- Tap bonus: 100 * 1.50 = 150
- Even Share bonus: 150 * 1.40 = 210 total
- Per member: floor(210 / 3) + 1 = 71 EXP each

**Note**: In Renewal, the tap bonus was reduced to 5% per attacker (max 100%). Pre-renewal uses 25% (max 580%).

---

## Item Share Settings

Item share mode is set **at party creation** and **cannot be changed** without disbanding and re-creating the party.

### Each Take

| Property | Value |
|----------|-------|
| **Behavior** | The player who killed the monster has first-pick priority on dropped items |
| **Priority window** | Brief delay before other players can pick up |
| **After priority** | Items become available to all nearby players |

### Party Share

| Property | Value |
|----------|-------|
| **Behavior** | Any party member can immediately pick up dropped items with no priority delay |
| **No priority** | First come, first served |

### Individual

| Property | Value |
|----------|-------|
| **Behavior** | Whoever picks up the item keeps it |
| **No redistribution** | Standard pickup behavior |

### Shared (Random Distribution)

| Property | Value |
|----------|-------|
| **Behavior** | When a party member picks up an item, it is randomly assigned to one party member |
| **Distribution** | Random selection among all eligible party members |
| **Announcement** | Party chat announces which member received the item (rAthena `show_party_share_picker: yes`) |

**rAthena configuration** (`party_item_share_type`):
- 0: Normal (random party member)
- 1: Disable item share for non-mob drops (pet drops, player drops excluded)
- 2: Round Robin (items distributed evenly in order)
- 3: Combined (1 + 2)

---

## Party Chat

| Property | Value |
|----------|-------|
| **Prefix** | `%` before the message |
| **Alternative** | `/p <message>` or press `Ctrl+Enter` to toggle party chat mode |
| **Color** | Green text in the chat window |
| **Range** | Cross-map -- all party members receive the message regardless of location |
| **Visibility** | Only visible to party members |

**Examples**:
- `%Hello team` -- sends "Hello team" to party chat
- `/p Let's move to the next floor` -- sends to party chat via command

**Guild chat** uses `$` prefix or `Alt+Enter` for comparison.

---

## Party Member Display

### Party Window (Alt+Z)

The party window is the primary interface for party management, opened with `Alt+Z`. It displays:

| Element | Details |
|---------|---------|
| **Member list** | All 12 members listed with character name |
| **HP bar** | Real-time HP bar for each member, color-coded (green > yellow > red) |
| **SP bar** | SP status visible in the party window |
| **Map name** | Shows which map each member is currently on |
| **Online status** | Online members shown normally; offline members greyed out |
| **Dead members** | Dead members show a skull icon or greyed-out HP bar |
| **Job class icon** | Small class icon next to each member name |
| **Leader indicator** | Leader has a crown icon or distinct marker |
| **Level** | Member's base level displayed |

### Party Window Buttons/Features

| Feature | Description |
|---------|-------------|
| **Lock icon** | Toggles skill targeting mode. When locked, clicking a party member name casts the selected skill on them (e.g., Heal, Blessing) instead of opening a PM window |
| **Party Invitation** | Button to invite a new member |
| **Party withdrew** | Button to leave the party |
| **Delegate Leader** | Right-click context menu option to transfer leadership |
| **Kick from Party** | Right-click context menu option (leader only) to remove a member |

### Skill Targeting Through Party Window

When the lock icon is enabled:
- Clicking a party member's name in the window targets them with the currently selected skill.
- This is essential for Priests/Acolytes to Heal party members without clicking their sprite on screen.
- Works with: Heal, Blessing, Increase AGI, Decrease AGI, Assumption, Kyrie Eleison, and all other player-targeted skills.
- Especially useful in crowded combat situations where clicking the correct sprite is difficult.

### Minimap Display

| Element | Details |
|---------|---------|
| **Party member dots** | Party members appear as pink/magenta dots on the minimap and world map |
| **World map** | `Ctrl+~` opens the world map showing party members as pink dots across all maps |
| **Update rate** | Party dots update at approximately 1-second intervals (rAthena `party_update_interval: 1000`) |

### HP Bar Mode (rAthena Configuration)

| Setting | Value | Description |
|---------|-------|-------------|
| `party_hp_mode: 0` | Aegis mode | HP bar updates every time HP changes (bandwidth intensive, real-time) |
| `party_hp_mode: 1` | rAthena mode | HP bar updates with minimap dots (up to 1-second delay, bandwidth efficient) |

---

## Party-Specific Skills

Several skills in pre-renewal Ragnarok Online have party-specific requirements or behaviors:

### Skills Requiring Party Membership

| Skill | Class | Party Requirement | Details |
|-------|-------|-------------------|---------|
| **Devotion** (1312) | Crusader/Paladin | Target must be in same party | Redirects all damage from target to caster. Target must be within 10 base levels of caster. Breaks if target leaves party, moves too far, or uses certain skills. |
| **Gospel / Battle Chant** (1319) | Paladin | Positive effects hit party members only | Randomly applies one of many positive effects to nearby party members and negative effects to nearby enemies every 10 seconds. |
| **Lullaby** (306) | Bard + Dancer ensemble | Party members immune to sleep effect | Ensemble skill that has a chance to put nearby enemies to sleep. Party members of the performers are immune to the sleep chance. |
| **Invulnerable Siegfried** (308) | Bard + Dancer ensemble | Party members get element resist + status resist | Grants nearby party members 60-80% elemental resistance and 10-50% status resistance. |

### Skills with Party Interaction (No Party Requirement)

| Skill | Class | Interaction | Details |
|-------|-------|-------------|---------|
| **Heal** (28) | Acolyte/Priest | Can target party members | Heals targeted player. Can be cast via party window (lock icon). |
| **Blessing** (34) | Acolyte/Priest | Can target party members | Buffs STR/INT/DEX. On Undead/Demon targets, reduces stats instead. |
| **Increase AGI** (29) | Acolyte/Priest | Can target party members | Increases AGI and movement speed. |
| **Decrease AGI** (30) | Acolyte/Priest | Can target party members | Reduces AGI and movement speed on enemies or players. |
| **Kyrie Eleison** (1011) | Priest | Can target party members | Barrier that absorbs a number of hits. |
| **Assumptio** (1017) | Priest/High Priest | Can target party members | Halves damage taken. Cannot be used in WoE. |
| **Magnificat** (1014) | Priest | AoE buff, benefits party | Doubles SP recovery rate for all nearby players (not party-restricted). |
| **Gloria** (1015) | Priest | AoE buff, benefits party | Increases LUK by 30 for nearby players. |
| **Providence** (1313) | Crusader | Can target party members | Grants Holy/Demon resistance to target. |
| **Potion Pitcher** (1015) | Alchemist | Can target party members | Throws a potion at target player to heal them. |
| **Warp Portal** (27) | Acolyte/Priest | Any player can enter | Not party-restricted; any player can walk into the portal. |

### Party EXP Sharing and Support Class Viability

The Even Share system is critical for support classes (Priest, Bard, Dancer) that deal minimal damage themselves. Without Even Share, these classes would receive almost no EXP in group play because EXP is damage-based in Each Take mode. Even Share ensures they receive an equal portion regardless of damage output, making full-support builds viable.

---

## Party Booking System

The Party Booking system was added to Ragnarok Online in a later patch but existed in some form during the Classic era on official servers.

| Property | Value |
|----------|-------|
| **Open search** | `/booking` command or "Booking" button in character window |
| **Create listing** | `/recruit` or `/partyrecruit` command |
| **Max class slots** | Up to 6 different job classes per listing |
| **Search parameters** | Level range, job class, map/dungeon area |
| **Listing display** | Shows party leader name, desired classes, level range, and target area |
| **Contact** | Players can whisper the listing creator to request an invite |
| **Auto-join** | Some versions support automatic join if listing is set to "auto" |

**Note**: The booking system is a **UI convenience feature** and does not affect any game mechanics. It is essentially a bulletin board for party recruitment. It was introduced in a mid-life patch and may not have been present in the earliest versions of Classic RO. For a faithful pre-renewal implementation, this feature is **optional** and low priority.

---

## Idle Penalty (rAthena Feature)

rAthena provides an idle penalty system that can disable EXP/item sharing for inactive party members:

| Setting | Default | Description |
|---------|---------|-------------|
| `idle_no_share` | `no` | Seconds of inactivity before a member is considered idle and excluded from sharing. Set to `no` to disable. |

This prevents AFK leeching where a player joins a party and goes idle while others farm. This is a server configuration option, not an official Gravity feature, but many pre-renewal private servers enable it with values like 120-300 seconds.

---

## All Party-Related Commands

| Command | Description |
|---------|-------------|
| `/organize "Name"` | Create a new party with the given name |
| `/invite "CharName"` | Invite a player to your party (leader only) |
| `/pinvite "CharName"` | Alternative invite command |
| `/leave` | Leave the current party |
| `/expel "CharName"` | Kick a member from the party (leader only) |
| `%message` | Send a message to party chat |
| `/p message` | Alternative party chat command |
| `Ctrl+Enter` | Toggle party chat input mode |
| `/booking` | Open party booking search window |
| `/recruit` | Open party booking listing creation window |
| `/partyrecruit` | Alternative booking creation command |
| `Alt+Z` | Toggle party window |
| `Ctrl+~` | Open world map (shows party member locations) |

---

## rAthena Configuration Reference

### party.conf (Battle Configuration)

| Setting | Default | Description |
|---------|---------|-------------|
| `show_steal_in_same_party` | `no` | Show stealer name in party chat when Steal/Gank used |
| `party_update_interval` | `1000` | Minimap dot update frequency in milliseconds |
| `party_hp_mode` | `0` | HP bar update method (0=per-change, 1=with dots) |
| `show_party_share_picker` | `yes` | Announce item recipient in party share mode |
| `show_picker.item_type` | `112` | Bitmask of item types to announce |
| `party_item_share_type` | `0` | Distribution method (0=random, 1=no non-mob, 2=round robin, 3=1+2) |
| `idle_no_share` | `no` | Idle timeout for share exclusion (seconds or no) |
| `party_even_share_bonus` | `0` | EXP bonus % per member in Even Share (set to 20 for iRO Classic) |
| `display_party_name` | `yes` | Show party name even if in a guild |
| `block_account_in_same_party` | `yes` | Prevent same-account characters in same party |
| `change_party_leader_samemap` | `yes` | Require leader to be on same map for delegation |

### inter_athena.conf

| Setting | Default | Description |
|---------|---------|-------------|
| `party_share_level` | `15` | Max base level difference for Even Share |

---

## Implementation Checklist

### Core Party Management
- [ ] Party creation (`/organize` or UI button)
- [ ] Basic Skill Level 7 check for creation, Level 5 for joining
- [ ] Max 12 members enforced
- [ ] Party name validation (1-24 characters)
- [ ] Item share mode set at creation, immutable after
- [ ] Remote invite (no proximity required)
- [ ] Invite timeout (30 seconds)
- [ ] Accept/Reject invite dialog
- [ ] Prevent inviting players already in a party
- [ ] Prevent same-account characters in same party
- [ ] Leader-only invite permission
- [ ] Kick/expel by leader only
- [ ] Self-leave by any member
- [ ] Automatic leader transfer when leader leaves
- [ ] Manual leader delegation (right-click context menu)
- [ ] Same-map requirement for leader transfer
- [ ] Party dissolution when last member leaves
- [ ] DB persistence (party survives server restart)
- [ ] Offline member tracking (stays in party, shows offline)
- [ ] Party data sent on login (reconnect to existing party)

### EXP Distribution
- [ ] Each Take mode (damage-contribution-based EXP)
- [ ] Even Share mode (equal split with +20%/member bonus)
- [ ] Even Share formula: `floor(MonsterEXP * (1 + 0.20 * (n-1)) / n) + 1`
- [ ] Base EXP and Job EXP both use same formula
- [ ] Level range check: all online members within 15 base levels
- [ ] Same-map filter: only members on kill map receive EXP
- [ ] Leader-only EXP mode switching
- [ ] Validate level range before enabling Even Share
- [ ] Tap bonus integration (+25% per additional attacker, cap 580%)
- [ ] Tap bonus applies to non-party attackers too
- [ ] Dead members excluded from EXP distribution

### Item Distribution
- [ ] Each Take: kill priority for item pickup
- [ ] Party Share: no priority, anyone can pick up
- [ ] Individual: picker keeps item
- [ ] Shared: random distribution to party member
- [ ] Item share picker announcement in party chat
- [ ] Item share mode locked after party creation

### Party Chat
- [ ] `%` prefix sends to party channel
- [ ] `/p` command alternative
- [ ] Cross-map delivery (all members regardless of location)
- [ ] Green text color in chat window
- [ ] Channel identifier in chat display

### Party UI / Display
- [ ] Party window (Alt+Z toggle)
- [ ] Member list with names, levels, class icons
- [ ] Real-time HP bar per member (color-coded: green/yellow/red)
- [ ] SP bar per member
- [ ] Map name per member
- [ ] Online/offline status indicator
- [ ] Dead member indicator (skull icon or grey)
- [ ] Leader indicator (crown icon)
- [ ] Lock icon for skill targeting through window
- [ ] Right-click context menu (kick, delegate, whisper)
- [ ] Party member pink dots on minimap
- [ ] Party member dots on world map
- [ ] 1-second update interval for minimap dots
- [ ] HP bar update mode (real-time vs batched)

### Party-Specific Skill Integration
- [ ] Devotion: require same party + 10 level range
- [ ] Gospel/Battle Chant: positive effects party-only
- [ ] Lullaby: party member sleep immunity
- [ ] Invulnerable Siegfried: party element/status resist
- [ ] Heal/Blessing/IncAGI: targetable through party window
- [ ] Party window skill targeting (lock icon)

### Party Booking (Optional / Low Priority)
- [ ] `/booking` search window
- [ ] `/recruit` listing creation
- [ ] Class filter (up to 6 classes)
- [ ] Level range filter
- [ ] Map/dungeon area filter
- [ ] Listing display and search results
- [ ] Whisper contact from search results

---

## Gap Analysis

### Already Implemented in Sabri_MMO (per MEMORY.md / session notes)

Based on the project's session tracker and implementation notes:

1. **Party creation/invite/kick/leave/delegation** -- COMPLETE (server + client)
2. **Even Share EXP with +20%/member bonus** -- COMPLETE
3. **Level range check (15 levels)** -- COMPLETE
4. **Same-map EXP filtering** -- COMPLETE
5. **Party chat (% prefix)** -- COMPLETE
6. **Party UI (PartySubsystem + SPartyWidget)** -- COMPLETE (HP+SP bars, context menu, invite popup)
7. **P hotkey toggle** -- COMPLETE
8. **EXP gain chat messages** -- COMPLETE
9. **DB persistence (parties + party_members tables)** -- COMPLETE
10. **party:load reconnect on login** -- COMPLETE
11. **Devotion party requirement** -- COMPLETE
12. **Lullaby party immunity** -- COMPLETE
13. **Tap bonus (+25%/attacker)** -- COMPLETE

### Gaps / Not Yet Implemented

| Gap | Priority | Notes |
|-----|----------|-------|
| **Item share mode at creation** | Medium | Currently not selecting item share mode on party creation. Item distribution logic (Each Take vs Party Share vs Shared) not implemented. |
| **Item share mode immutability** | Medium | Item share mode should be locked after creation. |
| **Basic Skill level gate** | Low | Creation requires Basic Skill Lv7, joining requires Lv5. Currently no skill-level check. |
| **Same-account block** | Low | Prevent characters from the same user account joining the same party. |
| **Leader delegation same-map check** | Low | Leader transfer should require same map (rAthena default). |
| **Tap bonus 580% cap** | Low | Current tap bonus has no upper cap. Should cap at 580%. |
| **Dead member exclusion from EXP** | Low | Dead party members should not receive Even Share EXP. Verify this is implemented. |
| **+1 EXP floor** | Low | iRO Classic adds +1 EXP per member to ensure minimum 1 EXP. Verify. |
| **Party booking system** | Very Low | `/booking` and `/recruit` UI. Low priority convenience feature. |
| **Idle penalty** | Very Low | Optional anti-AFK-leech feature. Not official but common on private servers. |
| **Party window lock icon (skill targeting)** | Medium | Click party member name to cast skill on them instead of opening PM. Important for Priest gameplay. |
| **Pink dots on minimap** | Medium | Party members shown as colored dots on minimap/world map. |
| **Party member map name display** | Low | Show which map each member is on in the party window. |
| **Dead member skull icon** | Low | Visual indicator in party window for dead members. |
| **Item share picker announcement** | Low | Chat message announcing which member received a shared item. |
| **EXP mode change chat broadcast** | Low | Notify party when leader changes EXP mode. May already be implemented. |

### Differences from Renewal (for reference, do NOT implement)

| Feature | Pre-Renewal (Classic) | Renewal |
|---------|----------------------|---------|
| Tap bonus | +25% per attacker, cap 580% | +5% per attacker, cap 100% |
| EXP share range | Entire map (no cell distance) | 30x30 cell range |
| Party bonus | +20% per member | Varies by server |
| Level range | 15 base levels | 30 base levels |
| Item share change | Locked at creation | Some servers allow changes |

---

## Sources

- [Party - iRO Wiki Classic](https://irowiki.org/classic/Party)
- [Party - iRO Wiki](https://irowiki.org/wiki/Party)
- [Experience - iRO Wiki Classic](https://irowiki.org/classic/Experience)
- [Experience - iRO Wiki](https://irowiki.org/wiki/Experience)
- [Party System - Ragnarok Fandom Wiki](https://ragnarok.fandom.com/wiki/Party_System)
- [Grouping - iRO Wiki](https://irowiki.org/wiki/Grouping)
- [rAthena party.conf](https://github.com/rathena/rathena/blob/master/conf/battle/party.conf)
- [rAthena party.cpp](https://github.com/rathena/rathena/blob/master/src/map/party.cpp)
- [rAthena inter_athena.conf](https://github.com/rathena/rathena/blob/master/conf/inter_athena.conf)
- [Ragnarok Commands - RateMyServer](https://write.ratemyserver.net/ragnoark-online-tips-and-tricks/ragnarok-commands-commands/)
- [Ragnarok Online Slash Commands](https://www.onlinegamecommands.com/ragnarok-slash-commands/)
- [Devotion - Ragnarok Fandom Wiki](https://ragnarok.fandom.com/wiki/Devotion)
- [Battle Chant - iRO Wiki](https://irowiki.org/wiki/Battle_Chant)
- [Ragnarok Online Party Play - StrategyWiki](https://strategywiki.org/wiki/Ragnarok_Online/Party_Play)
- [Party EXP Share - rAthena Forums](https://rathena.org/board/topic/124268-party-exp-share/)
- [rAthena Party EXP Issues](https://github.com/rathena/rathena/issues/597)
