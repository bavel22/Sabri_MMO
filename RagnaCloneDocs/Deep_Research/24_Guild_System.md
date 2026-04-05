# Guild System -- Deep Research (Pre-Renewal)

> **Sources**: rAthena pre-re source (guild.cpp, guild.conf, guild_skill_tree.yml, mmo.hpp), iRO Wiki Classic, RateMyServer skill DB, Ragnarok Wiki (Fandom), GameFAQs Guild FAQ, rAthena GitHub issues/board.
> **Verified against**: rAthena pre-renewal config, iRO Classic wiki, RateMyServer pre-re guild EXP table and skill database.

---

## Overview

The Guild System in Ragnarok Online (pre-renewal) is a social and competitive framework allowing players to form permanent organizations of up to 56 members. Guilds serve as the foundation for War of Emperium (castle siege PvP), provide an EXP tax/leveling system that unlocks guild skills, and offer organizational tools (titles, permissions, storage, emblems, alliances). Unlike parties (temporary, capped at 12), guilds are persistent, database-backed entities with hierarchical rank structures and a dedicated skill tree.

Key differences from renewal:
- Guild Storage Expansion is a **renewal-only** skill -- pre-renewal guilds have NO guild storage (only castle Kafra access).
- Charge Shout Flag, Charge Shout Beating, and Emergency Move are **renewal-only** skills.
- Guild Extension adds **+4 members per level** (iRO Classic), not +6 (rAthena renewal default).
- Great Leadership, Wounds of Glory, Soul of Cold, and Sharp Hawk Eyes have **5 levels** in pre-renewal (scaling +1 stat per level), not 1 level with flat +5.
- Restore has a **10-second cast time** in pre-renewal (reduced to 1 second in renewal).
- Maximum guild level is **50**, with 50 skill points total.

---

## Guild Creation & Management

### Requirements

| Requirement | Detail |
|-------------|--------|
| **Item** | 1x Emperium (item ID 714, consumed on creation, NOT returned on dissolution) |
| **Command** | `/guild "Guild Name"` while Emperium is in inventory |
| **Level** | No base/job level requirement for creation |
| **Existing guild** | Character must NOT already be in a guild |

The creating character becomes the **Guild Master** permanently. In classic RO, there is no built-in guild master transfer mechanism -- the only way to "transfer" is to disband and recreate. Later patches (and rAthena) added a transfer feature with a configurable cooldown (default: 1440 minutes / 24 hours, blocked during WoE hours).

### Guild Name

- Maximum **24 characters** (rAthena `guild_name` VARCHAR(24))
- Must be unique across all guilds on the server
- **Cannot be changed** after creation
- Name becomes available for reuse after dissolution

### Guild Emblem

- **Dimensions**: 24x24 pixels
- **Format**: BMP (256 colors / 8-bit indexed) or GIF (animated GIF supported)
- **Transparency**: Hot pink (RGB 255, 0, 255 / hex #FF00FF) renders as transparent in-game
- Stored client-side in an `Emblem/` folder inside the RO installation directory
- Only the Guild Master can change the emblem
- Appears above guild members' character sprites and in the guild window
- In rAthena, `require_glory_guild` config (default: `no`) controls whether the Guild's Glory skill is required to set an emblem. Official servers require it.
- For our implementation: store as base64 in the DB (`emblem_data TEXT`), broadcast to all members and nearby players

### Member Capacity

| Configuration | Base | Extension Skill (Lv10) | Maximum |
|---------------|------|------------------------|---------|
| **iRO Classic (pre-renewal)** | 16 | +4 per level = +40 | **56** |
| **rAthena renewal default** | 16 | +6 per level = +60 | **76** |

**For our implementation**: Use the iRO Classic values: base 16, +4 per Guild Extension level, max 56.

Guild Extension capacity per level (NOT cumulative -- each level replaces the previous bonus):

| Extension Lv | Members Added | Total Capacity |
|--------------|---------------|----------------|
| 0 (base) | 0 | 16 |
| 1 | +4 | 20 |
| 2 | +8 | 24 |
| 3 | +12 | 28 |
| 4 | +16 | 32 |
| 5 | +20 | 36 |
| 6 | +24 | 40 |
| 7 | +28 | 44 |
| 8 | +32 | 48 |
| 9 | +36 | 52 |
| 10 | +40 | 56 |

Formula: `maxMembers = 16 + (extensionLevel * 4)`

### Guild Master Transfer

- **Classic RO**: No built-in transfer. Must disband guild entirely and recreate.
- **rAthena implementation**: `guild:change_master` with configurable cooldown.
  - `guild_leaderchange_delay: 1440` (minutes, default = 24 hours)
  - `guild_leaderchange_woe: no` (blocked during active WoE)
- Old master is demoted to a configurable position (typically Officer / position 1).
- New master is assigned position 0 (Guild Master).

**For our implementation**: Support transfer with 24-hour cooldown, blocked during WoE.

### Guild Dissolution

1. Guild Master must **kick all other members** first (cannot disband with members remaining).
2. Command: `/breakguild "Guild Name"` (must type the exact guild name as confirmation).
3. Emperium used for creation is **NOT returned**.
4. All castle ownership is forfeited immediately.
5. Guild storage items (if any) are lost if not withdrawn before dissolution.
6. Guild name becomes available for reuse.
7. All alliance/enemy relationships are dissolved.

---

## Guild Ranks & Positions

### Position System

The Guild Master can configure **20 title/position slots** (IDs 0-19). Position 0 is always "Guild Master" and cannot be edited.

Each position has the following configurable properties:

| Property | Type | Range | Default (Pos 0) | Default (Pos 1-19) |
|----------|------|-------|------------------|---------------------|
| **Position Name** | string | max 24 chars | "Guild Master" | "Member" |
| **Can Invite** | boolean | true/false | true | false |
| **Can Kick** | boolean | true/false | true | false |
| **Can Storage** | boolean | true/false | true | false |
| **Tax Rate** | integer | 0-50 (%) | 0 | 0 |

### Permission Details

| Permission | Behavior |
|------------|----------|
| **Invite** | Can invite online players to join the guild |
| **Kick** | Can expel members from the guild (cannot kick Guild Master or higher-ranked positions) |
| **Storage** | Can access guild storage (only relevant in renewal or custom implementations) |
| **Tax Rate** | Percentage of Base EXP deducted from members with this position and contributed to guild EXP |

### Position Hierarchy

- Position IDs determine sort order in the guild member list (lower ID = higher rank).
- Position 0 (Guild Master) always has all permissions and cannot be modified.
- The Guild Master can rename positions freely (e.g., "Officer", "Elite", "Recruit", "Probation").
- Members are assigned to positions by the Guild Master via right-click context menu.
- New members default to position 19 (lowest rank).
- Positions are purely organizational -- they grant NO stat bonuses or gameplay advantages.

### Typical Position Setup

| Position ID | Suggested Name | Invite | Kick | Storage | Tax |
|-------------|---------------|--------|------|---------|-----|
| 0 | Guild Master | true | true | true | 0% |
| 1 | Vice Master | true | true | true | 10% |
| 2 | Officer | true | false | true | 10% |
| 3 | Elite Member | false | false | true | 15% |
| 4-18 | Member | false | false | false | 20% |
| 19 | Recruit | false | false | false | 25% |

---

## Guild Tax/EXP System

### How Tax Works

1. Each guild position has a configurable tax rate (0-50%).
2. When a member kills a monster, the tax percentage of their **Base EXP** is deducted.
3. The deducted EXP is added to the guild's cumulative EXP pool.
4. The member receives the remaining EXP (100% - tax%) as their personal Base EXP.
5. **Job EXP is NEVER taxed** -- members always receive 100% of Job EXP.
6. The rAthena config `guild_exp_limit: 50` caps the maximum tax rate at 50%.

### Tax Formula

```
taxedAmount = floor(monsterBaseEXP * (taxRate / 100))
playerReceives = monsterBaseEXP - taxedAmount
guildEXP += taxedAmount
member.contributedEXP += taxedAmount
```

### Guild Leveling

- Guilds start at level 1 and can reach a maximum of **level 50**.
- Each guild level grants **1 skill point** (50 total at max level).
- EXP required scales significantly with each level.

### Guild EXP Table (Pre-Renewal)

Based on rAthena pre-renewal and RateMyServer data:

| Level | EXP to Next Level | Cumulative EXP |
|-------|-------------------|----------------|
| 1->2 | 2,000,000 | 2,000,000 |
| 2->3 | 4,000,000 | 6,000,000 |
| 3->4 | 6,000,000 | 12,000,000 |
| 4->5 | 14,000,000 | 26,000,000 |
| 5->6 | 22,000,000 | 48,000,000 |
| 6->7 | 30,000,000 | 78,000,000 |
| 7->8 | 40,000,000 | 118,000,000 |
| 8->9 | 50,000,000 | 168,000,000 |
| 9->10 | 62,000,000 | 230,000,000 |
| 10->11 | 92,000,000 | 322,000,000 |
| 15->16 | 202,000,000 | ~1.1B |
| 20->21 | 362,000,000 | ~3.6B |
| 25->26 | 602,000,000 | ~8.5B |
| 30->31 | 872,000,000 | ~16.5B |
| 35->36 | 1,200,000,000 | ~28B |
| 40->41 | 1,600,000,000 | ~44B |
| 45->46 | 1,999,999,999 | ~53B (capped) |
| 46-50 | 1,999,999,999 each | ~63B total |

Pre-renewal EXP values are significantly higher than renewal. Levels 45+ are capped at 1,999,999,999 per level (int32 limit).

**For our implementation**: Use a simpler exponential formula that captures the spirit without the extreme values:
```javascript
GUILD_EXP_TABLE[1] = 10000;
for (let i = 2; i <= 50; i++) {
    GUILD_EXP_TABLE[i] = Math.floor(GUILD_EXP_TABLE[i - 1] * 1.35);
}
```
This gives manageable numbers for a smaller-scale server while preserving exponential scaling. Tune the base and multiplier based on server population and average EXP rates.

### Guild Level Benefits

| Benefit | Detail |
|---------|--------|
| Skill points | +1 per level (50 total at Lv50) |
| Prestige | Higher-level guilds appear more established |
| No stat bonuses | Guild level does NOT directly buff member stats |
| No capacity bonus | Capacity is from Guild Extension skill, not guild level |

---

## Guild Skills

Guild skills are learned by the Guild Master using guild skill points (1 per guild level). Only the Guild Master can allocate skill points and cast active guild skills.

### Pre-Renewal Guild Skill Tree

```
                    [Approval] ─────────────────────┐
                    /    |     \                     |
         [Kafra Contract] [Extension Lv10]    [Battle Orders]
              |                                  /        \
      [Guardian Research]              [Regeneration]    ...
              |                            |
      [Strengthen Guardian]           [Restore]
                                          |
                                    [Emergency Call]
                                    (also requires Guardian Research,
                                     Extension Lv5, Battle Orders Lv1,
                                     Regeneration Lv1)

       [Great Leadership] ──── [Sharp Hawk Eyes]
       (5 levels)               (5 levels)

       [Wounds of Glory] ──── [Soul of Cold]
       (5 levels)               (5 levels)

       [Guild's Glory] (emblem)

       [Permanent Development] (economy)
```

### Complete Skill List (Pre-Renewal)

#### Passive Skills

**1. Official Guild Approval (GD_APPROVAL)**
| Property | Value |
|----------|-------|
| Skill ID | 10000 |
| Max Level | 1 |
| Type | Passive |
| Prerequisite | None |
| Effect | Certifies the guild as official. Required for WoE participation and Emperium attacks. Gate prerequisite for most other guild skills. |

**2. Kafra Contract (GD_KAFRACONTRACT)**
| Property | Value |
|----------|-------|
| Skill ID | 10001 |
| Max Level | 1 |
| Type | Passive |
| Prerequisite | Official Guild Approval Lv1 |
| Effect | Enables hiring a Kafra NPC inside guild-owned castles, providing free save/warp/storage services to guild members. |

**3. Guardian Research (GD_GUARDRESEARCH)**
| Property | Value |
|----------|-------|
| Skill ID | 10002 |
| Max Level | 1 |
| Type | Passive |
| Prerequisite | Official Guild Approval Lv1 |
| Effect | Enables hiring Guardian NPCs inside guild-owned castles. Guardians auto-attack enemy players during WoE. |

**4. Strengthen Guardians (GD_GUARDUP)**
| Property | Value |
|----------|-------|
| Skill ID | 10003 |
| Max Level | 3 |
| Type | Passive |
| Prerequisite | None (rAthena pre-re tree) |
| Effect | Increases Guardian NPC stats. Per level: +HP, +ATK, +ASPD to all castle guardians. Exact values are server-side constants tied to the guardian template. |

Note: In rAthena pre-renewal, GD_GUARDUP has MaxLevel 3 with no prerequisite. Some references show MaxLevel 1 with Guardian Research as prereq -- this varies by implementation.

**5. Guild Extension (GD_EXTENSION)**
| Property | Value |
|----------|-------|
| Skill ID | 10004 |
| Max Level | 10 |
| Type | Passive |
| Prerequisite | None |
| Effect | Increases guild member capacity by +4 per level (iRO Classic). See Member Capacity table above. Required at Lv2 for Battle Orders, Lv5 for Regeneration and Emergency Call. |

**6. Guild's Glory (GD_GLORYGUILD)**
| Property | Value |
|----------|-------|
| Skill ID | 10005 |
| Max Level | 1 (iRO) / 0 in rAthena pre-re tree |
| Type | Passive |
| Prerequisite | None |
| Effect | Permits the guild to use a custom guild emblem. In rAthena, `require_glory_guild` config (default: `no`) can bypass this requirement. |

Note: In rAthena pre-re `guild_skill_tree.yml`, GD_GLORYGUILD has MaxLevel 0, meaning it is essentially disabled/free. The emblem feature works without spending a point.

**7. Great Leadership (GD_LEADERSHIP)**
| Property | Value |
|----------|-------|
| Skill ID | 10006 |
| Max Level | 5 (pre-renewal) / 1 (renewal) |
| Type | Passive aura |
| Prerequisite | None |
| Area | 5x5 cells around Guild Master |
| Duration | Permanent while Master is present |
| Effect | +1 STR per skill level to all guild members within range. WoE only. |
| Lv1: +1 STR | Lv2: +2 STR | Lv3: +3 STR | Lv4: +4 STR | Lv5: +5 STR |

**8. Wounds of Glory (GD_GLORYWOUNDS)**
| Property | Value |
|----------|-------|
| Skill ID | 10007 |
| Max Level | 5 (pre-renewal) / 1 (renewal) |
| Type | Passive aura |
| Prerequisite | None |
| Area | 5x5 cells around Guild Master |
| Duration | Permanent while Master is present |
| Effect | +1 VIT per skill level to all guild members within range. WoE only. |
| Lv1: +1 VIT | Lv2: +2 VIT | Lv3: +3 VIT | Lv4: +4 VIT | Lv5: +5 VIT |

**9. Soul of Cold (GD_SOULCOLD)**
| Property | Value |
|----------|-------|
| Skill ID | 10008 |
| Max Level | 5 (pre-renewal) / 1 (renewal) |
| Type | Passive aura |
| Prerequisite | Wounds of Glory Lv1 |
| Area | 5x5 cells around Guild Master |
| Duration | Permanent while Master is present |
| Effect | +1 AGI per skill level to all guild members within range. WoE only. |
| Lv1: +1 AGI | Lv2: +2 AGI | Lv3: +3 AGI | Lv4: +4 AGI | Lv5: +5 AGI |

**10. Sharp Hawk Eyes (GD_HAWKEYES)**
| Property | Value |
|----------|-------|
| Skill ID | 10009 |
| Max Level | 5 (pre-renewal) / 1 (renewal) |
| Type | Passive aura |
| Prerequisite | Great Leadership Lv1 |
| Area | 5x5 cells around Guild Master |
| Duration | Permanent while Master is present |
| Effect | +1 DEX per skill level to all guild members within range. WoE only. |
| Lv1: +1 DEX | Lv2: +2 DEX | Lv3: +3 DEX | Lv4: +4 DEX | Lv5: +5 DEX |

**11. Permanent Development (GD_DEVELOPMENT)**
| Property | Value |
|----------|-------|
| Skill ID | 10014 |
| Max Level | 1 |
| Type | Passive |
| Prerequisite | None |
| Effect | 50% chance to receive a free bonus economy point when investing in a guild castle's Commerce development. |

#### Active Skills (Guild Master Only)

All four active guild skills share a **5-minute global cooldown** (pre-renewal). Using any one prevents all four for 5 minutes.

**12. Battle Orders (GD_BATTLEORDER)**
| Property | Value |
|----------|-------|
| Skill ID | 10010 |
| Max Level | 1 |
| Type | Active, Supportive |
| Prerequisite | Official Guild Approval Lv1, Guild Extension Lv2 |
| SP Cost | 1 |
| Cast Time | None (instant) |
| Cooldown | 5 minutes (shared with Regeneration, Restore, Emergency Call) |
| Duration | 60 seconds (1 minute) |
| Area | 31x31 cells (visible members on Guild Master's screen) |
| Restriction | **WoE maps only** |
| Effect | All guild members (and allied guild members) visible on the Guild Master's screen receive **+5 STR, +5 DEX, +5 INT** for 60 seconds. |

**13. Regeneration (GD_REGENERATION)**
| Property | Value |
|----------|-------|
| Skill ID | 10011 |
| Max Level | 3 |
| Type | Active, Supportive |
| Prerequisite | Official Guild Approval Lv1, Guild Extension Lv5, Battle Orders Lv1 |
| SP Cost | 1 |
| Cast Time | None |
| Cooldown | 5 minutes (shared) |
| Duration | 60 seconds |
| Area | 31x31 cells |
| Restriction | **WoE maps only** |

| Level | Effect |
|-------|--------|
| 1 | HP recovery rate x2 |
| 2 | HP recovery rate x2, SP recovery rate x2 |
| 3 | HP recovery rate x3, SP recovery rate x3 |

**14. Restore (GD_RESTORE)**
| Property | Value |
|----------|-------|
| Skill ID | 10012 |
| Max Level | 1 |
| Type | Active, Supportive |
| Prerequisite | Regeneration Lv1 |
| SP Cost | 1 |
| Cast Time | **10 seconds** (pre-renewal) / 1 second (renewal) |
| Cooldown | 5 minutes (shared) |
| Area | 31x31 cells |
| Restriction | **WoE maps only** |
| Effect | Instantly restores **90% of Max HP** and **90% of Max SP** to all visible guild members. |

**15. Emergency Call / Urgent Call (GD_EMERGENCYCALL)**
| Property | Value |
|----------|-------|
| Skill ID | 10013 |
| Max Level | 1 |
| Type | Active, Supportive |
| Prerequisite | Official Guild Approval Lv1, Guardian Research Lv1, Guild Extension Lv5, Battle Orders Lv1, Regeneration Lv1 |
| SP Cost | 1 |
| Cast Time | **5 seconds** (fixed, uninterruptible) |
| Cooldown | 5 minutes (shared) |
| Restriction | **WoE maps only** |
| Effect | Teleports **ALL online guild members** (regardless of map) to the Guild Master's current location. |

Special notes:
- Cannot be interrupted by damage or status effects
- Affected by Kagehumi (Shadow Trampling) -- a Ninja skill that roots the target
- If the Guild Master is a Taekwon/Star Gladiator with Leap skill points, cast time doubles to 10 seconds
- The most strategically important guild skill in WoE

### Skill Point Budget (Pre-Renewal)

Total available: 50 points (1 per guild level).

Essential skills require the following minimum point investment:

| Skill Path | Points Required |
|------------|-----------------|
| Emergency Call (minimum path) | Approval(1) + GuardResearch(1) + Extension(5) + BattleOrders(1) + Regeneration(1) + EmergencyCall(1) = **10 points** |
| Full active skill set | Above + Restore(1) + Regeneration to Lv3(+2) = **13 points** |
| Kafra Contract | +1 = 14 |
| Strengthen Guardian Lv3 | +3 = 17 |
| Permanent Development | +1 = 18 |
| Guild Extension to Lv10 | +5 more (already 5 from EC path) = 23 |
| Full passive auras (4 skills x 5 levels) | +20 = 43 |
| Guild's Glory | +0 or +1 depending on config |
| **Total for everything** | ~43-44 points (fits within 50) |

This means a max-level guild can learn all pre-renewal guild skills with points to spare.

### Renewal-Only Skills (NOT in pre-renewal)

The following skills exist in rAthena but are renewal additions. Listed for reference:

| Skill | ID | Effect |
|-------|----|--------|
| Guild Storage Expansion | 10016 | Unlocks guild storage: 100/200/300/400/500 slots at Lv1-5 |
| Charge Shout Flag | 10017 | Places "Flag of Assault" during WoE (15 min cooldown) |
| Charge Shout Beating | 10018 | Teleports nearby guild members to the Flag of Assault |
| Emergency Move | 10019 | Movement speed buff for Master + nearby members during WoE |

**For our implementation**: Do NOT include renewal-only skills initially. Add them as expansion content if desired.

---

## Guild Storage

### Pre-Renewal Status

**Pre-renewal guilds have NO guild storage.** Guild Storage Expansion (GD_GUILD_STORAGE) is a renewal-only skill added in 2014. In classic/pre-renewal RO, the only shared storage benefit was:

- **Castle Kafra**: Guilds owning a castle can access a Kafra NPC inside the castle, providing **personal** Kafra storage (not shared guild storage) for free (no Zeny fee).
- The Kafra Contract guild skill enables this castle Kafra.

### Renewal Guild Storage (for reference / future implementation)

| Expansion Level | Capacity |
|-----------------|----------|
| 0 (no skill) | No access |
| 1 | 100 slots |
| 2 | 200 slots |
| 3 | 300 slots |
| 4 | 400 slots |
| 5 | 500 slots |

- Accessible through Kafra NPCs in towns or castle Kafra
- Access controlled by guild position permissions (`canStorage`)
- Items are shared among all members with storage access
- **Zeny cannot be stored** in guild storage (items only)
- Has an item log tracking deposits/withdrawals and who performed them
- Guild Master always has access

### Implementation Decision

**For our game**: Implement guild storage from day one (even though it is technically renewal). Players expect shared guild storage as a basic feature. Use the 5-level expansion skill with 100 slots per level. This is already designed in the implementation doc (`10_Party_Guild_Social.md`).

---

## Guild Chat

### Commands

| Command | Action |
|---------|--------|
| `$message` | Send message to guild chat (dollar sign prefix) |
| `/gc message` | Alternative guild chat command |
| `Alt+Enter` | Toggle guild chat mode (all messages sent to guild) |

### Behavior

- Guild chat messages are received by **all online guild members** regardless of map/zone.
- Guild chat appears in a distinct color (typically **light green** or **bright green**) in the chat window.
- Guild chat channel identifier: `GUILD`

### Guild Notifications

| Event | Notification |
|-------|-------------|
| Member login | "[Name] has connected." shown to all online members |
| Member logout | "[Name] has disconnected." shown to all online members |
| Member map change | "[Name] has moved to [MapName]." (optional, can be disabled) |
| Level up | "[Name] has reached Base Level [X]!" |

Members can disable login/logout/map notifications with `/li` or `/loginout` command.

---

## Guild Emblem System

### Technical Specifications

| Property | Value |
|----------|-------|
| Dimensions | 24x24 pixels |
| Format | BMP (256 colors / 8-bit indexed) or GIF (animated supported) |
| Color depth | 256 colors maximum |
| Transparency key | RGB(255, 0, 255) -- hot pink / magenta (#FF00FF) |
| Max file size | ~2 KB typical (24x24 8-bit BMP = 1,782 bytes + header) |
| Who can change | Guild Master only |

### Display Locations

- Above guild members' character sprites (small icon next to name tag)
- In the guild window header
- On castle ownership displays
- In WoE castle map UI

### Implementation Approach

For our server:
1. Guild Master uploads emblem via client UI
2. Client converts to base64 string
3. Server stores in `guilds.emblem_data` (TEXT column, base64)
4. Server broadcasts `guild:emblem_updated` to all guild members
5. Client decodes base64 and renders as a 24x24 texture
6. Nearby players receive emblem data via `guild:emblem_data` broadcast when they see a guild member
7. Cap base64 payload at ~10,000 characters (safety limit for a 24x24 image)

---

## Alliance & Enemy System

### Alliances

| Property | Value |
|----------|-------|
| Maximum allies | **3** guilds simultaneously (`max_guild_alliance: 3` in rAthena) |
| Proposal command | `/ally` or right-click guild in alliance tab |
| Acceptance | Target guild master must accept the alliance proposal |
| Mutual | Alliance is bidirectional -- both guilds see each other as allied |

Alliance effects:
- Allied guild members appear with **green names** in WoE
- Allied guild members **cannot attack each other** during WoE
- Allied guild members are treated as friendly for AoE skill targeting during WoE
- Battle Orders and other guild aura skills affect allied guild members within range
- Alliance persists until explicitly broken by either guild master

### Enemies / Antagonists

| Property | Value |
|----------|-------|
| Maximum enemies | **3** (shares the 3-slot limit with alliances in some implementations; rAthena `MAX_GUILDALLIANCE 16` covers both types) |
| Declaration command | `/enemy` or right-click guild in alliance tab |
| Acceptance | Unilateral -- one guild can declare another as enemy without consent |

Enemy effects:
- Enemy guild members appear with **red names** everywhere (not just WoE/PvP maps)
- Enemy guild members **can attack each other anywhere** (even on normal maps, outside PvP/WoE)
- Provides persistent open-world PvP between rival guilds

### rAthena Implementation

In rAthena source (`mmo.hpp`): `MAX_GUILDALLIANCE 16` -- this is the maximum total entries in the alliance table, covering both allies and enemies. The `guild.conf` setting `max_guild_alliance: 3` controls the maximum number of **ally** relationships specifically. The total of allies + enemies cannot exceed `MAX_GUILDALLIANCE`.

### Database Schema

```sql
CREATE TABLE IF NOT EXISTS guild_alliances (
    guild_id_1 INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    guild_id_2 INTEGER NOT NULL REFERENCES guilds(guild_id) ON DELETE CASCADE,
    alliance_type VARCHAR(10) NOT NULL DEFAULT 'ally',  -- 'ally' | 'enemy'
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (guild_id_1, guild_id_2)
);
```

---

## Guild Notification System

### Guild Notice / Message Board

- Guild Master can set a **two-part guild notice**:
  - **Title**: up to 60 characters
  - **Body**: up to 240 characters
- Notice is displayed to members when they:
  - Log in
  - Change maps / teleport
  - Warp (Fly Wing, Butterfly Wing, Warp Portal)
- Notice display can be configured with `guild_notice_changemap` setting:
  - `0`: Show only on login
  - `1`: Show on login and map change
  - `2`: Show on login, map change, and teleport (official)
- The notice appears as a system popup in the chat area

### Member Status Broadcasts

The guild system automatically broadcasts:
- Member login/logout
- Member level-up
- Member map changes (optional, toggleable with `/li`)
- Guild level-up (broadcast to all members)
- Skill point allocation
- Emblem changes
- Alliance/enemy changes
- Position/rank changes

---

## Guild Window (Alt+G)

The guild window has multiple tabs:

| Tab | Contents |
|-----|----------|
| **Guild Info** | Guild name, level, EXP bar, member count, master name, alliance list |
| **Guildsmen** | Member list sorted by position, showing name/level/class/online status |
| **Positions** | 20 title slots with name/permissions/tax rate (editable by Master) |
| **Skills** | Guild skill tree with point allocation (Master only) |
| **Expulsion** | Expulsion log / ban list |
| **Notice** | Guild notice title + body (editable by Master) |

---

## Implementation Checklist

### Server-Side

- [ ] **Guild creation** (`guild:create`) -- consume Emperium, create DB records, 20 default positions
- [ ] **Guild invite/join** (`guild:invite`, `guild:invite_respond`) -- permission check, capacity check, 30s timeout
- [ ] **Guild leave** (`guild:leave`) -- GM cannot leave (must transfer or disband)
- [ ] **Guild kick** (`guild:kick`) -- permission check, cannot kick GM
- [ ] **Guild disband** (`guild:disband`) -- must be empty (GM only remaining), confirmation by name
- [ ] **Guild master transfer** (`guild:change_master`) -- 24h cooldown, block during WoE
- [ ] **Position management** (`guild:update_position`, `guild:set_position`) -- GM only, validate ranges
- [ ] **Guild notice** (`guild:set_notice`) -- GM only, 60/240 char limits
- [ ] **Guild emblem** (`guild:set_emblem`) -- GM only, base64 storage + broadcast
- [ ] **Guild EXP tax** -- deduct from Base EXP on monster kill, add to guild EXP, track per-member contribution
- [ ] **Guild leveling** -- level up when EXP threshold reached, +1 skill point, broadcast
- [ ] **Guild skill learning** (`guild:learn_skill`) -- GM only, check prereqs + points + max level
- [ ] **Guild skill effects** -- Battle Orders (WoE buff), Regeneration (WoE HP/SP regen), Restore (WoE instant heal), Emergency Call (WoE teleport all)
- [ ] **Guild aura passives** -- Leadership/Wounds/Cold/HawkEyes: per-tick stat buff for nearby members during WoE
- [ ] **Guild Extension** -- recalculate maxMembers on skill up
- [ ] **Guild storage** (renewal feature) -- 100 slots per skill level, deposit/withdraw, permission check
- [ ] **Alliance system** (`guild:ally`, `guild:enemy`) -- max 3 allies, mutual acceptance, enemy unilateral
- [ ] **Guild chat** -- `$` prefix or `GUILD` channel, broadcast to all online members
- [ ] **Login/disconnect broadcasts** -- member online status changes
- [ ] **Guild data on login** (`guild:data`) -- send full guild state when member connects
- [ ] **Periodic DB persistence** -- save guild EXP/level/contribution on interval (30s) and on disconnect
- [ ] **Load guilds from DB on startup** -- reconstruct in-memory state

### Client-Side (UGuildSubsystem + SGuildWidget)

- [ ] **GuildSubsystem** (UWorldSubsystem) -- socket event handlers, guild state management
- [ ] **SGuildWidget** -- Slate widget with tabs (Info, Members, Positions, Skills, Notice)
- [ ] **Alt+G hotkey** -- toggle guild window
- [ ] **Guild invite popup** -- accept/decline incoming invite
- [ ] **Guild emblem upload** -- file picker or embedded editor
- [ ] **Guild emblem rendering** -- decode base64, display as 24x24 texture above members
- [ ] **Guild chat integration** -- `$` prefix detection in ChatSubsystem, GUILD channel color
- [ ] **Guild notice popup** -- display on login/map change
- [ ] **Position editor** -- name, permissions, tax rate fields (GM only)
- [ ] **Skill tree UI** -- guild skill allocation with prereq visualization (GM only)
- [ ] **Member context menu** -- right-click: set position, kick, whisper
- [ ] **Alliance/enemy management** -- propose/break alliance, declare enemy

### Database

- [ ] `guilds` table -- guild_id, name, master_id, level, exp, exp_next, max_members, skill_points, notice_title, notice_body, emblem_data
- [ ] `guild_positions` table -- guild_id, position_id (0-19), name, can_invite, can_kick, can_storage, tax_rate
- [ ] `guild_members` table -- guild_id, character_id, position_id, contributed_exp
- [ ] `guild_skills` table -- guild_id, skill_id, skill_level
- [ ] `guild_storage` table -- guild_id, item_id, quantity, slot_index, deposited_by
- [ ] `guild_alliances` table -- guild_id_1, guild_id_2, alliance_type ('ally'/'enemy')
- [ ] Auto-create tables at server startup (same pattern as other tables)

---

## Gap Analysis

### Existing Implementation (from `10_Party_Guild_Social.md`)

The implementation doc already provides:
- Complete DB schema for all guild tables
- Socket.io event protocol (22 guild events)
- Server-side code: create, invite, join, leave, kick, disband, master transfer, skill learning, position management, notice, emblem, EXP tax, login data
- Client-side code: GuildSubsystem header + implementation, GuildWidget structure, data structs

### Gaps / Corrections Needed

| Issue | Current State | Correct (Pre-Renewal) | Priority |
|-------|---------------|----------------------|----------|
| **Guild Extension per-level** | Implementation doc uses +6/level (76 max) | iRO Classic uses **+4/level (56 max)** | HIGH |
| **Passive aura skills** | Implementation doc has max Lv1 with flat +5 | Pre-renewal has **max Lv5** with +1/level | MEDIUM |
| **Guild Storage** | Implementation doc includes it | Pre-renewal has **NO guild storage** (renewal feature). Include anyway as QoL. | LOW (keep as-is) |
| **Restore cast time** | Not specified in impl doc | Pre-renewal: **10 seconds** (not 1 second) | MEDIUM |
| **GD_GUARDUP prereqs** | Impl doc has Guardian Research Lv1 as prereq | rAthena pre-re has **no prereq** (MaxLevel 3) | LOW |
| **GD_GLORYGUILD** | Impl doc has max Lv1 | rAthena pre-re has **MaxLevel 0** (disabled/free) | LOW |
| **Skill cooldown relog** | Not addressed | `guild_skill_relog_type: 1` (pre-re) -- cooldown resets on relog | LOW |
| **Active skill WoE restriction** | Impl doc does not enforce WoE-only | All 4 active skills are **WoE maps only** | HIGH (for WoE impl) |
| **Alliance system** | Schema exists but no handler code | Need `guild:ally`, `guild:remove_ally`, `guild:enemy`, `guild:remove_enemy` handlers | MEDIUM |
| **Guild chat** | Not integrated with ChatSubsystem | Need `$` prefix detection and `GUILD` channel routing | MEDIUM |
| **Guild notice display** | Not wired to map change events | Need notice popup on login + map change | LOW |
| **Emergency Call uninterruptible** | Not specified | Must be uninterruptible (cannot be canceled by damage/status) | HIGH (for WoE) |
| **Battle Orders affects allies** | Not documented | Affects allied guild members too, not just own guild | MEDIUM |
| **Permanent Development** | Not in GUILD_SKILL_DEFS | Missing from skill definitions; needs adding | LOW |
| **Guild EXP table** | Uses simplified 1.35x formula | Acceptable for our scale; official values are much larger | OK |
| **Taekwon EC cast penalty** | Not implemented | If GM is Taekwon/Star Gladiator with Leap, Emergency Call cast time doubles | LOW |
| **Guild member status broadcast** | Partial (member_update) | Need login/logout/level-up/map-change notifications to guild chat | MEDIUM |
