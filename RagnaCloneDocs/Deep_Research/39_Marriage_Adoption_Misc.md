# Marriage, Adoption & Misc -- Deep Research (Pre-Renewal)

> Comprehensive reference for replicating the Marriage, Adoption, and miscellaneous social/item systems from Ragnarok Online Classic (pre-renewal) in Sabri_MMO.
> Sources: iRO Wiki Classic, rAthena pre-re source, RateMyServer, divine-pride.net, Ragnarok Fandom Wiki, TalonRO Wiki.

---

## Table of Contents

1. [Marriage System](#marriage-system)
2. [Adoption System](#adoption-system)
3. [Special Items](#special-items)
4. [Rental Items](#rental-items)
5. [Implementation Checklist](#implementation-checklist)
6. [Gap Analysis](#gap-analysis)

---

## MARRIAGE SYSTEM

The marriage system allows two player characters (one male, one female) to become married in-game, granting shared skills, stat bonuses, and the ability to adopt a third player into a "Baby" class family unit.

### Requirements (Level, Items, Cost)

**Character Requirements:**
| Requirement | Value |
|-------------|-------|
| Minimum Base Level | 45 (both characters) |
| Gender | One male + one female character (same-sex marriage not supported in Classic) |
| Class restriction | Neither character can be a Baby class (adopted character) |
| Marriage status | Neither character can already be married |

**Required Items:**

| Item | Item ID | Source | Cost | Notes |
|------|---------|--------|------|-------|
| Wedding Dress | 2338 | Wedding Shop Dealer | ~43,000 Zeny | Equipped by the bride during ceremony |
| Tuxedo | 2337 | Wedding Shop Dealer | ~43,000 Zeny | Equipped by the groom during ceremony |
| Diamond Ring (Bride) | 2634 | Wedding Shop Dealer | ~45,000 Zeny | Transforms into Wedding Ring after ceremony |
| Diamond Ring (Groom) | 2635 | Wedding Shop Dealer | ~45,000 Zeny | Transforms into Wedding Ring after ceremony |
| Marriage Covenant x2 | 6294 | Wedding Present NPC or Cash Shop | 20,000,000 Zeny each (NPC) or 500 KP ($5 USD) each | One per spouse, consumed during ceremony |

**Wedding Shop Dealer Locations:**
- Prontera: `prt_in` (285, 169)
- Lighthalzen: `lhz_in02` (47, 148)

**Marriage Covenant Sources:**
- **Wedding Present NPC** inside Prontera style shop -- 20,000,000 Zeny per covenant (ask for "stamp")
- **Kafra Shop (Cash Shop)** -- 500 coins ($5 USD) per covenant

**Total Minimum Cost (Zeny route):** ~40,176,000 Zeny (2 covenants + dress + tuxedo + 2 rings)

### Wedding Ceremony Process

**Location:** Prontera Church, northeast Prontera City (`prt_church`)

**Step-by-step procedure:**

1. **Preparation**: Both characters must have the required items in their inventory (Wedding Dress or Tuxedo, Diamond Ring, Marriage Covenant).
2. **Registration**: Either the groom or the bride speaks to the Marriage NPC inside Prontera Church to initiate the ceremony.
3. **Spouse Confirmation**: The other spouse must speak to the same NPC within **60 seconds** (1 minute) or the application times out and must be restarted.
4. **Ceremony**: Once both parties have registered, they proceed to the front of the church where the Priest NPC delivers the ceremony dialogue.
5. **Final Confirmation**: The Priest asks both parties one final time if they wish to proceed. Both must confirm "Yes."
6. **Completion**: If both agree, the marriage is complete. The characters are now married.

**Post-Ceremony Effects:**
- Both characters are automatically equipped with their formal wear (Wedding Dress / Tuxedo)
- While wearing formal wear: **cannot attack or use skills** for 1 hour
- **Movement speed is reduced** as long as the formal wear is equipped (can unequip to remove penalty)
- Diamond Rings are consumed and replaced with **Wedding Rings** (must be identified with a Magnifier to activate)
- Marriage Covenants are consumed
- Server announcement is broadcast (server-dependent)

### Marriage Benefits

Marriage grants three categories of benefits: shared skills, stat interaction, and teleportation.

#### Shared Skills (Wedding / Family Skills)

Wedding skills become available when a **Wedding Ring** is equipped. The ring must be identified (use a Magnifier on the Diamond Ring that was transformed during the ceremony). Skills can be cast even if only one spouse has the ring equipped, but only the ring-wearing spouse can use them.

**Wedding Ring Items:**
| Item | Item ID | Slot | Notes |
|------|---------|------|-------|
| Wedding Ring (Bride) | 2634 | Accessory | Transforms from Diamond Ring during ceremony |
| Wedding Ring (Groom) | 2635 | Accessory | Transforms from Diamond Ring during ceremony |

**Marriage Skills (Job Class: Wedding):**

| Skill Name | Skill ID (rAthena) | Type | SP Cost | Cast Time | Cooldown | Effect |
|------------|---------------------|------|---------|-----------|----------|--------|
| Loving Touch | WE_MALE (334) | Supportive | 10% of caster's Max SP | 0 | 0 | Consumes 10% of your own HP to restore 10% of your partner's HP. Both must be on the same map. |
| Undying Love | WE_FEMALE (335) | Supportive | 10% of caster's Max SP | 0 | 0 | Consumes 10% of your own SP to restore 10% of your partner's SP. Both must be on the same map. |
| Romantic Rendezvous / I Will Follow You | WE_CALLPARTNER (336) | Supportive | 1 | 15s (fixed) | 0 | Summons your spouse to your current location. Cannot be used on maps where Warp Portal memo is blocked (WoE castles, dungeons, some special maps). |

**Notes:**
- Loving Touch (WE_MALE) -- originally groom-only; some implementations allow either spouse
- Undying Love (WE_FEMALE) -- originally bride-only; some implementations allow either spouse
- Romantic Rendezvous (WE_CALLPARTNER) -- usable by either spouse. The summoned partner sees a confirmation dialog and can accept or decline. 15-second cast time makes it impractical in combat.
- Skills require the Wedding Ring to be equipped in an accessory slot
- Skills are blocked on `nowarpto` / `nowarp` flagged maps

#### Stat Bonuses

In pre-renewal Classic RO, marriage itself does not grant passive stat bonuses. The benefits are limited to the wedding skills above. Some private servers and later implementations added bonuses like:
- +1 to all stats when near spouse (non-official)
- Shared EXP bonus (non-official)

The official pre-renewal benefit is strictly the wedding skills (HP/SP sharing + teleport).

#### Teleport to Spouse

`WE_CALLPARTNER` (Romantic Rendezvous) is the primary teleport mechanism:
- 15-second fixed cast time (uninterruptible)
- No aftercast delay
- 1 SP cost
- Target (spouse) receives a confirmation popup
- Blocked on: WoE maps, PvP maps, maps with `nomemo` mapflag, maps with `nowarpto` mapflag
- Both characters must be online
- Works cross-map (any map to any allowed map)

### Adoption Skills (Family Skills)

After adopting a child (see Adoption System below), additional family skills become available:

| Skill Name | Skill ID (rAthena) | Who Uses It | Effect |
|------------|---------------------|-------------|--------|
| Call Parent | WE_CALLPARENT (409) | Baby (child) | Summons both parents to the child's location. Same map restrictions as WE_CALLPARTNER. |
| Call Baby | WE_CALLBABY (410) | Either parent | Summons the adopted child to the parent's location. Same restrictions. |
| Happy Break / Baby | WE_BABY (411) | Baby (child) | For 2 minutes, if either parent dies while this buff is active, the parent does not lose any EXP from death. Costs 10% of the baby's Max SP per cast. |

**Happy Break (WE_BABY) Details:**
- Active buff, 2-minute duration
- SP cost: 10% of baby character's Max SP
- Effect: Parents are immune to death EXP penalty while buff is active
- The baby character must be on the same map as the parent(s)
- Buff icon appears on the parents
- Only the Baby class character can cast this

### Divorce (Process, Consequences)

**Divorce NPC Location:** Niflheim (167, 161) -- inside a house, upstairs where a Deviruchi NPC resides.

**Divorce Process:**
1. Either spouse visits the Deviruchi NPC in Niflheim
2. Pay **2,500,000 Zeny** divorce fee
3. Confirm the divorce
4. Marriage is dissolved immediately

**Consequences of Divorce:**
| Consequence | Details |
|-------------|---------|
| Wedding Rings | Destroyed / removed from both characters |
| Wedding Skills | Lost immediately (Loving Touch, Undying Love, Romantic Rendezvous) |
| Adoption status | **Persists** -- if the couple adopted a child, the child remains a Baby class. Adoption cannot be undone. |
| Adoption skills | WE_CALLPARENT, WE_CALLBABY, WE_BABY still function between the original family members |
| Remarriage | Both characters can remarry other characters |
| Re-adoption | The divorced couple **cannot** re-adopt. The adoption slot is permanently consumed. |
| Cost recovery | None -- rings, covenants, and zeny are not refunded |

**Important:** Divorce does NOT revert a Baby class character to a normal character. The adoption is permanent regardless of the parents' marriage status.

### Wedding Rings

| Property | Bride Ring (2634) | Groom Ring (2635) |
|----------|-------------------|-------------------|
| Item ID | 2634 | 2635 |
| Type | Accessory | Accessory |
| Slot | Accessory 1 or 2 | Accessory 1 or 2 |
| Weight | 0 | 0 |
| DEF | 0 | 0 |
| Equip Level | 0 | 0 |
| Refineable | No | No |
| Tradeable | No | No |
| Droppable | No | No |
| Skills granted | Loving Touch (Lv1), Undying Love (Lv1), Romantic Rendezvous (Lv1) | Same |
| Special | Created by identifying Diamond Ring post-ceremony | Same |

**Wedding Ring Creation Process:**
1. During the wedding ceremony, Diamond Rings are consumed from both characters' inventories
2. Wedding Rings are placed in both characters' inventories
3. The Wedding Rings must be **identified using a Magnifier** before the skills become usable
4. Once identified and equipped, the wedding skills are available

---

## ADOPTION SYSTEM

The adoption system allows a married couple to "adopt" a third player character, converting them into a Baby class with reduced stats but unique family skills.

### Requirements (Married Couple + Novice Character)

**Parent Requirements:**
| Requirement | Value |
|-------------|-------|
| Marriage | Must be a married couple (wedding completed) |
| Base Level | Both parents must be **Base Level 70 or higher** |
| Wedding Rings | Both parents must have Wedding Rings **equipped** |
| Party | The child-to-be must be in the same party as the parents |
| Previous adoption | Parents must not have already adopted a child (one adoption per marriage) |

**Child Requirements:**
| Requirement | Value |
|-------------|-------|
| Class | Must be a **Novice** or **First Class** (Swordsman, Mage, Archer, Acolyte, Merchant, Thief) |
| Base Level | No specific level requirement (must have completed job change if First Class) |
| Marriage status | Must NOT be married |
| Adoption status | Must NOT already be adopted / a Baby class |

### Baby Class Creation

**Adoption Process:**
1. Married couple and the child-to-be must all be online and in the same party
2. Both parents must have their Wedding Rings equipped
3. One parent right-clicks the child character and selects **"Adopt <CharacterName>"**
4. The child character receives a confirmation dialog
5. If accepted, the child's class immediately changes to the Baby version

**Alternative method (NPC-based):**
- Some implementations use an NPC at Prontera's Orphanage
- The NPC checks all requirements and processes the adoption

**Class Conversion Table:**

| Original Class | Baby Class | Job ID |
|----------------|------------|--------|
| Novice | Baby Novice | 4023 |
| Swordsman | Baby Swordsman | 4024 |
| Mage | Baby Mage | 4025 |
| Archer | Baby Archer | 4026 |
| Acolyte | Baby Acolyte | 4027 |
| Merchant | Baby Merchant | 4028 |
| Thief | Baby Thief | 4029 |
| Knight | Baby Knight | 4030 |
| Priest | Baby Priest | 4031 |
| Wizard | Baby Wizard | 4032 |
| Blacksmith | Baby Blacksmith | 4033 |
| Hunter | Baby Hunter | 4034 |
| Assassin | Baby Assassin | 4035 |
| Crusader | Baby Crusader | 4037 |
| Monk | Baby Monk | 4038 |
| Sage | Baby Sage | 4039 |
| Rogue | Baby Rogue | 4040 |
| Alchemist | Baby Alchemist | 4041 |
| Bard | Baby Bard | 4042 |
| Dancer | Baby Dancer | 4043 |

**Upon adoption:**
- Character sprite shrinks to a smaller ("chibi") version
- Character size changes from Medium to **Small**
- All stats above 80 are converted back to raw status points
- The character gains access to family skills (WE_CALLPARENT, WE_BABY)
- Class change is irreversible -- there is no way to un-adopt

### Baby Class Restrictions

#### Reduced Stats

| Restriction | Value |
|-------------|-------|
| Max stat cap | **80** (normal characters cap at 99) |
| HP/SP | **75% of normal Max HP and Max SP** (multiplicative reduction) |
| Stats above 80 at adoption | Converted to raw status points (refunded) |

The 75% HP/SP penalty is applied as a multiplier on the calculated Max HP/Max SP. For example, if a normal Knight would have 10,000 Max HP, the Baby Knight has 7,500 Max HP.

#### Limited Skill Levels

In pre-renewal Classic RO, Baby classes have access to the **same skill tree** as their adult counterparts with the **same max skill levels**. The skill tree is not truncated. However:

- Baby classes **cannot Rebirth/Transcend** -- they cannot become Transcendent classes (High Wizard, Lord Knight, etc.)
- This means Baby classes miss out on:
  - Transcendent-exclusive skills (e.g., Berserk for Lord Knight, Soul Destroyer for Assassin Cross)
  - The +25% HP/SP bonus from Transcendence
  - The additional stat points from rebirth
- Baby classes CAN job change to their Second Class normally (Baby Swordsman -> Baby Knight)

#### Equipment Restrictions

Baby classes can equip the **same equipment** as their adult counterparts. There are no Baby-specific equipment restrictions in pre-renewal. However:

- **Size change matters**: Baby characters are Small size, which affects size modifiers in combat (weapons optimized for Medium targets deal less damage when wielded by Babies against Medium enemies in some calculations, though this is a target-size mechanic not a wielder-size issue)

#### Weight Penalty

| Penalty | Value |
|---------|-------|
| Max Weight reduction | **-1,200** from normal maximum weight capacity |

Example: If a normal Swordsman has 10,000 max weight, a Baby Swordsman has 8,800 max weight.

#### Craft Rate Penalty

| Penalty | Value |
|---------|-------|
| Forge success rate (Blacksmith) | **-50%** (halved) |
| Brew success rate (Alchemist) | **-50%** (halved) |

This makes Baby Blacksmiths and Baby Alchemists significantly less effective at crafting.

### Baby Class Benefits

Despite the penalties, Baby classes have some unique advantages:

| Benefit | Details |
|---------|---------|
| Happy Break (WE_BABY) | Protects parents from death EXP loss for 2 minutes |
| Call Parent (WE_CALLPARENT) | Summons both parents to baby's location |
| Cute factor | Smaller sprite is visually distinctive (cosmetic) |
| Full skill access | Same skills as adult counterparts (minus Transcendent) |
| Lower level curve | Same EXP table as normal classes in pre-renewal |

---

## SPECIAL ITEMS

### Dead Branch

| Property | Value |
|----------|-------|
| Item ID | 604 |
| Type | Usable (Delayed Consumable) |
| Weight | 5 |
| Buy Price | 50 Zeny (NPC) |
| Sell Price | 25 Zeny |

**Mechanics:**
- When used, summons **1 random non-MVP monster** at the user's location
- Monster pool: ~463 monsters in pre-renewal (excludes MVPs, event-only monsters, and certain special monsters)
- The summoned monster acts as a normal wild monster -- it can attack, be killed, and drops normal loot
- Summoned monsters give normal EXP and drops
- Can summon mini-bosses (non-MVP boss-protocol monsters)
- **Cannot be used indoors on some maps** (map-dependent restriction)
- Popular uses: farming specific monsters, entertainment, griefing in towns
- Weight limit: Cannot be used at 90%+ weight

**Monster Selection:**
- Equal weight random selection from the eligible monster pool
- The pool includes nearly all field and dungeon monsters from the pre-renewal database
- Excludes: MVPs, quest-specific monsters, event monsters, monsters with the `BOSS` protocol flag set to MVP-tier

**Common Sources:**
- Drops from various monsters (e.g., Deniro, Enchanted Peach Tree)
- Purchased from specific NPCs
- Quest rewards
- Old Blue Box / Gift Box contents

### Bloody Branch

| Property | Value |
|----------|-------|
| Item ID | 12103 |
| Type | Usable (Delayed Consumable) |
| Weight | 20 |
| Buy Price | -- (not NPC-sold) |
| Sell Price | 10 Zeny |

**Mechanics:**
- When used, summons **1 random MVP monster** at the user's location
- Monster pool: All MVP-class monsters available in the pre-renewal database
- The summoned MVP acts as a full MVP -- MVP announcement, MVP EXP, MVP drops, MVP skills
- **Does NOT grant MVP rewards** to the summoner automatically -- the MVP must be killed and the top-damage dealer receives the MVP reward
- Can only be used outdoors or on specific allowed maps
- Weight limit: Cannot be used at 90%+ weight
- The summoned MVP has its full skill set, HP, and combat behavior

**Common Sources:**
- WoE Treasure Boxes (very rare)
- Certain high-level monster drops (extremely rare)
- Quest rewards (rare)
- Cash Shop (some servers)

**Important Implementation Notes:**
- Bloody Branch MVPs should be treated identically to naturally spawned MVPs for rewards, announcements, and combat
- The MVP tombstone/grave marker should appear when a Bloody Branch MVP is killed
- Bloody Branch usage should be logged server-side (anti-abuse)

### Old Blue Box

| Property | Value |
|----------|-------|
| Item ID | 603 |
| Type | Usable |
| Weight | 20 |
| Buy Price | -- |
| Sell Price | 10,000 Zeny |

**Mechanics:**
- When used (opened), produces **1 random item** from a curated loot table
- The loot table includes: consumables, equipment, cards, materials, misc items
- Item pool is weighted -- common items have higher probability, rare items have lower
- **Cannot be opened at 90%+ weight** -- must reduce weight first
- The item appears directly in the character's inventory
- If inventory is full, the item drops on the ground

**Loot Table Characteristics (Pre-Renewal):**
- ~800+ possible items
- Heavily weighted toward consumables and low-value equipment
- Cards are possible but extremely rare
- MVP-exclusive drops are NOT in the pool
- Includes some items not obtainable elsewhere

**Common Sources:**
- Monster drops (various monsters)
- Quest rewards
- Treasure Boxes (WoE)

### Old Purple Box

| Property | Value |
|----------|-------|
| Item ID | 617 |
| Type | Usable |
| Weight | 20 |
| Buy Price | -- |
| Sell Price | 10,000 Zeny |

**Mechanics:**
- When used (opened), produces **1 random item** from a different (generally higher-quality) loot table than Old Blue Box
- Item pool overlaps with Old Blue Box but includes rarer items and excludes some common trash
- **Cannot be opened at 90%+ weight**
- Same inventory/ground-drop behavior as Old Blue Box

**Key Differences from Old Blue Box:**
| Property | Old Blue Box | Old Purple Box |
|----------|-------------|----------------|
| Item ID | 603 | 617 |
| Loot quality | Lower average | Higher average |
| Card chance | Very rare | Slightly better |
| Equipment chance | Moderate | Higher |
| Common consumables | More frequent | Less frequent |
| Rarity | More common drop | Rarer drop |

### Old Card Album

| Property | Value |
|----------|-------|
| Item ID | 616 |
| Type | Usable |
| Weight | 5 |
| Buy Price | -- |
| Sell Price | 10,000 Zeny |

**Mechanics:**
- When used, produces **1 random card** from the card pool
- The card pool includes nearly all monster cards in the pre-renewal database
- Cards are weighted -- common monster cards appear more frequently, MVP cards are extremely rare
- **Cannot be opened at 90%+ weight**
- Very valuable item; commonly obtained from MVPs and WoE Treasure Boxes

### Gift Box

| Property | Value |
|----------|-------|
| Item ID | 644 |
| Type | Usable |
| Weight | 20 |
| Buy Price | -- |
| Sell Price | 2,500 Zeny |

**Mechanics:**
- When used, produces **1 random item** from a smaller, generally lower-quality pool than Old Blue Box
- More common than Old Blue/Purple Box
- Often obtained from monster drops (e.g., Myst Case card effect: chance to get Gift Box on monster kill)

### Treasure Chest (Field / WoE)

There are two categories of "Treasure" items:

**Treasure Box (Item ID 7444):**
- A miscellaneous item (not directly openable by players)
- Appears in WoE castle Treasure Rooms
- Contains items from the castle-specific loot table when collected
- 4 base boxes per castle per day + bonus from Commerce investment

**Treasure Chest (Monster ID 1350):**
- A destructible object that spawns in certain field maps and dungeons
- When "killed" (0 HP), drops items from its loot table
- Has HP, can be attacked, but does not fight back
- Spawns at fixed locations on a respawn timer
- Different Treasure Chest variants exist for different map regions with different loot tables

### Yggdrasil Leaf

| Property | Value |
|----------|-------|
| Item ID | 610 |
| Type | Usable (Delayed Consumable) |
| Weight | 10 |
| Buy Price | -- |
| Sell Price | 2,500 Zeny |

**Mechanics:**
- When used on a dead player character (target), resurrects them with a small amount of HP
- Resurrection amount: **~10% of target's Max HP** (equivalent to Resurrection Lv1)
- The target must be dead (on the ground, showing death sprite)
- Must be used on the dead character's cell (stand on or adjacent to the corpse)
- **Cannot resurrect yourself** -- must be used by another player
- **Cannot be used on Undead-element characters** (e.g., characters wearing Bathory-carded armor)
- Consumed on use (success or failure)
- **Cannot be used in WoE** (some implementations)

**Common Sources:**
- Drops from high-level monsters
- Old Blue Box / Old Purple Box
- Quest rewards
- Treasure Boxes

### Yggdrasil Berry

| Property | Value |
|----------|-------|
| Item ID | 607 |
| Type | Healing (Consumable) |
| Weight | 30 |
| Buy Price | -- |
| Sell Price | 5,000 Zeny |

**Mechanics:**
- When used, **fully restores HP and SP** to maximum
- Instant effect, no cast time
- Subject to the standard item usage delay (~0.5s potion delay)
- **Cannot be used on Undead-element characters**
- One of the most powerful healing items in the game
- Very rare and valuable

**Item Usage Delay:**
- Yggdrasil Berry shares the standard potion/healing item delay
- After using any healing item, there is a brief cooldown before another can be used
- This prevents rapid-fire berry consumption in PvP/WoE

**Common Sources:**
- MVP drops (common MVP drop)
- WoE Treasure Boxes
- Yggdrasil Berry Gathering quest (repeatable)
- Old Purple Box (rare)

### Token of Siegfried

| Property | Value |
|----------|-------|
| Item ID | 7621 |
| Type | Miscellaneous (passive effect) |
| Weight | 1 |
| Buy Price | -- |
| Sell Price | -- (untradeable in most implementations) |

**Mechanics:**
- When the character carrying this item dies, the Token is **automatically consumed** and the character is **resurrected on the spot**
- Resurrection restores HP/SP to a moderate amount (implementation varies: 50% HP, some servers full HP/SP)
- The item is consumed upon death -- single use
- No manual activation required -- triggers automatically on death
- **Does NOT work in WoE** (some implementations)
- No EXP penalty is applied when resurrected by Token of Siegfried

**Implementation Notes:**
- Check for Token of Siegfried in the death handler BEFORE applying death penalties
- If found: consume the item, resurrect at current position, skip EXP loss and respawn flow
- The resurrection should be immediate (no respawn dialog)

**Common Sources:**
- Cash Shop item
- Event rewards
- Quest rewards (rare)

### Gym Pass

| Property | Value |
|----------|-------|
| Item ID | 7776 |
| Type | Miscellaneous (quest item) |
| Weight | 0 |
| Buy Price | -- |
| Sell Price | -- |

**Mechanics:**
- Not directly usable -- must be taken to the NPC **Ripped Cabus** in Payon (173, 141)
- The NPC teaches the skill **Enlarge Weight Limit** (one level per Gym Pass)
- Each level of Enlarge Weight Limit permanently increases Max Weight by **+200**
- Maximum skill level: **10** (total +2,000 weight capacity)
- The Gym Pass is consumed when the NPC teaches the skill
- The weight increase is **permanent** and persists through rebirth/transcendence
- After Transcendence, the character can speak to Ripped Cabus again to re-learn the skill for free (no Gym Pass needed)

**Enlarge Weight Limit Skill:**
| Level | Weight Bonus | Cumulative |
|-------|-------------|------------|
| 1 | +200 | +200 |
| 2 | +200 | +400 |
| 3 | +200 | +600 |
| 4 | +200 | +800 |
| 5 | +200 | +1,000 |
| 6 | +200 | +1,200 |
| 7 | +200 | +1,400 |
| 8 | +200 | +1,800 |
| 9 | +200 | +1,800 |
| 10 | +200 | +2,000 |

**Common Sources:**
- Cash Shop item
- Event rewards
- Quest rewards

### Bubble Gum

| Property | Value |
|----------|-------|
| Item ID | 12210 |
| Type | Usable (Consumable) |
| Weight | 10 |
| Buy Price | -- |
| Sell Price | -- |

**Mechanics:**
- When used, grants a buff that **doubles item drop rates** (+100% drop rate) for **30 minutes**
- The buff applies to all monster kills during the duration
- Affects: normal drops, card drops, MVP drops
- Does NOT affect: quest item drops (some implementations)
- The buff is a personal buff -- does not affect party members
- Stacks with server-wide drop rate events (multiplicative)
- **HE Bubble Gum** (Item ID 12412) is an enhanced version with higher rates (+200%)

**Implementation:**
- Apply as a timed status effect (`sc_itemboost` or custom)
- In the drop calculation: `effectiveRate = baseRate * (1 + bubbleGumBonus)`
- Duration: 1,800,000ms (30 minutes)
- Buff icon should be displayed
- Does not persist through death (some implementations preserve it)

### Battle Manual

| Property | Value |
|----------|-------|
| Item ID | 12208 |
| Type | Usable (Consumable) |
| Weight | 10 |
| Buy Price | -- |
| Sell Price | -- |

**Mechanics:**
- When used, grants a buff that **increases EXP gained by +50%** for **30 minutes**
- Affects both Base EXP and Job EXP from monster kills
- Does NOT affect quest EXP rewards (some implementations)
- Personal buff -- does not affect party members
- Stacks with server-wide EXP events and party bonuses (multiplicative)
- **HE Battle Manual** (Item ID 12411) grants +100% EXP instead of +50%

**Note on naming:** The item description says "increases by 150%" which means the total EXP becomes 150% of normal (i.e., a +50% bonus). Some implementations interpret this as +100% (double), but the canonical rAthena behavior is +50%.

**Implementation:**
- Apply as a timed status effect
- In the EXP calculation: `effectiveEXP = baseEXP * (1 + battleManualBonus)`
- Duration: 1,800,000ms (30 minutes)
- Buff icon should be displayed

**Combined Item Variants:**

| Item | Item ID | Effect | Duration |
|------|---------|--------|----------|
| Battle Manual | 12208 | +50% EXP | 30 min |
| HE Battle Manual | 12411 | +100% EXP | 30 min |
| Battle Manual x3 | 12263 | +50% EXP | 90 min (3x duration) |
| Bubble Gum | 12210 | +100% drop rate | 30 min |
| HE Bubble Gum | 12412 | +200% drop rate | 30 min |
| [7Day] Battle Manual & Bubble Gum | 14799 | +50% EXP + 100% drop | 7 days (rental) |

---

## RENTAL ITEMS

### Time-Limited Items (How They Work)

Rental items are equipment or consumables with a built-in expiration timer. Once obtained (from a box, NPC, or event), the timer begins counting down in real-time (not play-time). When the timer expires, the item is automatically removed from the character's inventory/equipment.

**Core Mechanics:**
| Property | Value |
|----------|-------|
| Timer type | Real-time countdown (continues while logged off) |
| Timer display | Remaining time shown in item tooltip |
| Expiration | Item is automatically deleted when timer reaches 0 |
| Equipment | Auto-unequipped and deleted if equipped when timer expires |
| Tradeable | Rental items are **NOT tradeable, storable, or droppable** |
| Refinement | Rental equipment **cannot be refined** |
| Card slots | Rental equipment **cannot have cards compounded** |
| Stacking | Rental consumables follow normal stacking rules |

**Common Rental Durations:**
| Duration | Examples |
|----------|----------|
| 1 hour | Event rental weapons |
| 7 days | Halter Lead (mount), combat items |
| 14 days | Elemental weapons, special equipment |
| 30 days | Cash Shop premium equipment |

**Implementation Requirements:**
- Store `rental_expire_time` (Unix timestamp) on the item record in the database
- On login: check all items for expired rentals and delete them
- During gameplay: periodic check (every 60s or on item inspection) for expired items
- When an equipped rental item expires: force-unequip, delete, recalculate stats
- Display remaining time in the item tooltip: "Expires in: Xd Xh Xm"

### Rental NPCs

**Common Rental Sources:**

| NPC / Source | Location | Items Offered | Duration |
|-------------|----------|---------------|----------|
| Kafra Shop (Cash Shop) | System | Halter Lead boxes, equipment | 7-30 days |
| Event NPCs | Various towns | Elemental weapons, headgears | 7-14 days |
| Riding NPC | Prontera | Halter Lead (Peco mount for Knights/Crusaders) | 7 days |
| Rental Weapon NPC | Prontera/Payon | Level-appropriate weapons | 7 days |

**Halter Lead (Mount Rental):**
| Property | Value |
|----------|-------|
| Item ID | 12622 |
| Type | Usable |
| Effect | Grants Riding status (mount Peco Peco / Grand Peco) |
| Duration | 7 or 30 days (from box) |
| Requirement | Knight or Crusader class with Riding skill |
| Speed bonus | +36% movement speed while mounted |

**Elemental Rental Weapons (Event Items):**
Some servers offered time-limited elemental weapons during events:
- Fire/Water/Wind/Earth element weapons
- Pre-leveled (e.g., +7 rental weapon)
- 14-day duration
- Cannot be refined further, no card slots
- Powerful for leveling characters

---

## Implementation Checklist

### Marriage System
- [ ] **Database**: Add `marriage` columns to `characters` table: `partner_id` (FK to characters), `married_at` (timestamp)
- [ ] **Wedding Ring items**: Add item IDs 2634 (bride) and 2635 (groom) to item database with accessory type
- [ ] **Marriage Covenant item**: Add item ID 6294 as consumable
- [ ] **Wedding Dress / Tuxedo items**: Add item IDs 2338 and 2337 with movement speed penalty script
- [ ] **Marriage NPC**: Create NPC at Prontera Church with ceremony flow (register -> spouse confirm within 60s -> ceremony -> ring exchange)
- [ ] **Divorce NPC**: Create NPC at Niflheim with 2,500,000 Zeny fee
- [ ] **Wedding skill: Loving Touch (WE_MALE)**: 10% own HP -> 10% partner HP, same-map check
- [ ] **Wedding skill: Undying Love (WE_FEMALE)**: 10% own SP -> 10% partner SP, same-map check
- [ ] **Wedding skill: Romantic Rendezvous (WE_CALLPARTNER)**: 15s cast, summon spouse, map restriction check, confirmation dialog
- [ ] **Skill gating**: Wedding skills only available when Wedding Ring is equipped
- [ ] **Map restrictions**: Block WE_CALLPARTNER on nowarp/nowarpto/WoE maps
- [ ] **Diamond Ring -> Wedding Ring transformation**: Post-ceremony item swap + Magnifier identification requirement

### Adoption System
- [ ] **Database**: Add `parent1_id`, `parent2_id`, `is_baby` columns to `characters` table
- [ ] **Baby class job IDs**: Register job IDs 4023-4043 in class system
- [ ] **Adoption trigger**: Right-click "Adopt" option when parents + child meet all requirements
- [ ] **Stat reduction**: Apply 80 stat cap, 75% HP/SP multiplier, -1,200 weight
- [ ] **Stat refund**: Convert stats above 80 to status points on adoption
- [ ] **Craft penalty**: -50% forge/brew rate for Baby Blacksmith/Alchemist
- [ ] **Adoption skill: Call Parent (WE_CALLPARENT)**: Baby summons both parents
- [ ] **Adoption skill: Call Baby (WE_CALLBABY)**: Parent summons baby
- [ ] **Adoption skill: Happy Break (WE_BABY)**: 2-min buff, parents immune to death EXP loss, 10% MaxSP cost
- [ ] **Baby sprites**: Smaller character sprites for all Baby classes
- [ ] **Size change**: Baby characters = Small size (affects size modifiers in combat)
- [ ] **Rebirth block**: Prevent Baby classes from using Rebirth/Transcendence NPCs

### Special Items
- [ ] **Dead Branch (604)**: Summon random non-MVP monster, build monster pool (~463 entries), 90% weight check
- [ ] **Bloody Branch (12103)**: Summon random MVP, build MVP pool, full MVP behavior (announcement, rewards, skills)
- [ ] **Old Blue Box (603)**: Random item from weighted loot table (~800+ items), 90% weight check
- [ ] **Old Purple Box (617)**: Random item from higher-quality weighted loot table, 90% weight check
- [ ] **Old Card Album (616)**: Random card from weighted card pool, 90% weight check
- [ ] **Gift Box (644)**: Random item from smaller loot table
- [ ] **Yggdrasil Leaf (610)**: Resurrect dead player at ~10% HP, Undead element check, adjacent cell targeting
- [ ] **Yggdrasil Berry (607)**: Full HP + SP restore, item delay, Undead element check
- [ ] **Token of Siegfried (7621)**: Auto-resurrect on death, consume item, skip death penalty, bypass respawn flow
- [ ] **Gym Pass (7776)**: NPC-mediated, teach Enlarge Weight Limit skill (+200 weight/level, max Lv10)
- [ ] **Bubble Gum (12210)**: +100% drop rate buff, 30-minute duration, personal buff
- [ ] **Battle Manual (12208)**: +50% EXP buff, 30-minute duration, personal buff
- [ ] **Loot table data files**: Create `ro_random_items.js` with weighted pools for OBB, OPB, OCA, Gift Box
- [ ] **Monster summon pools**: Create `ro_branch_monsters.js` with Dead Branch and Bloody Branch monster lists

### Rental Items
- [ ] **Database**: Add `rental_expire` (Unix timestamp) column to `character_inventory` table
- [ ] **Expiration check**: Periodic server-side check for expired rental items (every 60s)
- [ ] **Auto-delete on expiry**: Force-unequip + delete + stat recalc when rental expires
- [ ] **Trade/storage block**: Prevent rental items from being traded, stored, dropped, or sold
- [ ] **Refine/card block**: Prevent refining and card compounding on rental items
- [ ] **Tooltip display**: Show "Expires in: Xd Xh Xm" in item tooltip for rental items
- [ ] **Halter Lead (12622)**: Mount system rental (7/30 day variants)

---

## Gap Analysis

### Current Sabri_MMO State vs. Required Systems

| System | Current State | Required Work | Priority |
|--------|--------------|---------------|----------|
| **Marriage DB schema** | Not implemented | Add partner_id, married_at columns | Medium |
| **Wedding NPC** | Not implemented | Create NPC with full ceremony flow | Medium |
| **Divorce NPC** | Not implemented | Create NPC in Niflheim | Low |
| **Wedding skills (3)** | Not implemented | WE_MALE, WE_FEMALE, WE_CALLPARTNER handlers | Medium |
| **Wedding Ring items** | Not in item DB | Add items 2634, 2635 with skill-grant scripts | Medium |
| **Adoption system** | Not implemented | Full system: DB, class conversion, stat changes | Low |
| **Baby class sprites** | Not implemented | Chibi-sized character models (art asset work) | Low |
| **Baby class stats** | Not implemented | 80 cap, 75% HP/SP, -1200 weight | Low |
| **Family skills (3)** | Not implemented | WE_CALLPARENT, WE_CALLBABY, WE_BABY handlers | Low |
| **Dead Branch** | Not implemented | Monster summon pool + handler | Medium |
| **Bloody Branch** | Not implemented | MVP summon pool + handler | Medium |
| **Old Blue Box** | Not implemented | Weighted random item table + handler | Low |
| **Old Purple Box** | Not implemented | Weighted random item table + handler | Low |
| **Old Card Album** | Not implemented | Weighted random card table + handler | Low |
| **Yggdrasil Leaf** | Not implemented | Target-resurrect consumable handler | High |
| **Yggdrasil Berry** | Partially (healing items exist) | Full HP+SP restore, delay, Undead check | High |
| **Token of Siegfried** | Not implemented | Auto-resurrect in death handler | Medium |
| **Gym Pass** | Not implemented | Enlarge Weight Limit skill + NPC | Low |
| **Bubble Gum** | Not implemented | Drop rate buff system | Low |
| **Battle Manual** | Not implemented | EXP buff system | Low |
| **Rental item system** | Not implemented | Expiration timer, trade blocks, auto-delete | Low |
| **Item loot tables** | Not implemented | Data files for OBB/OPB/OCA/Gift Box pools | Low |
| **Branch monster pools** | Not implemented | Data files for Dead Branch/Bloody Branch | Medium |

### Dependencies and Recommended Implementation Order

**Phase 1 -- High Priority (combat-relevant items):**
1. Yggdrasil Berry (full HP/SP restore)
2. Yggdrasil Leaf (resurrect ally)
3. Token of Siegfried (auto-resurrect on death)

**Phase 2 -- Medium Priority (gameplay-enhancing items):**
4. Dead Branch + Bloody Branch (monster summoning)
5. Bubble Gum + Battle Manual (rate buffs)
6. Gym Pass + Enlarge Weight Limit

**Phase 3 -- Social Systems:**
7. Marriage system (DB, NPCs, ceremony)
8. Wedding skills
9. Divorce system

**Phase 4 -- Advanced Social:**
10. Adoption system
11. Baby classes (stats, restrictions, sprites)
12. Family skills

**Phase 5 -- Loot Systems:**
13. Old Blue Box / Old Purple Box / Old Card Album / Gift Box (random item pools)
14. Rental item infrastructure

### Key Architecture Decisions

1. **Marriage storage**: Store `partner_id` on both characters (bidirectional FK). Query with `WHERE partner_id = :characterId` to find spouse.
2. **Baby class identification**: Use `is_baby` boolean flag rather than checking job ID ranges. Simplifies stat calculations.
3. **Random item pools**: Store as JSON data files (like `ro_monster_templates.js`). Each entry: `{ itemId, weight }`. Use weighted random selection.
4. **Branch monster pools**: Derive from existing `ro_monster_templates` -- filter by MVP flag for Bloody Branch, exclude MVPs for Dead Branch.
5. **Rental items**: Add `rental_expire` column to `character_inventory`. NULL = not rental. Server checks on login + periodic tick.
6. **Rate buffs (Bubble Gum / Battle Manual)**: Implement as status effects in the buff system (`sc_expboost`, `sc_itemboost`). Check in EXP distribution and drop calculation functions.
7. **Token of Siegfried**: Check in the death handler (where death penalty / respawn is processed). If token found in inventory, consume it, skip penalty, resurrect at current location.

### Server Code Integration Points

| System | Integration Point in `server/src/index.js` | Section |
|--------|----------------------------------------------|---------|
| Yggdrasil Berry | `inventory:use_item` handler, healing item pipeline | Item effects |
| Yggdrasil Leaf | `inventory:use_item` handler, target-resurrect pipeline | Item effects |
| Token of Siegfried | `handlePlayerDeath()` function, before EXP loss | Death handler |
| Dead/Bloody Branch | `inventory:use_item` handler, monster spawn pipeline | Item effects + Enemy spawning |
| Bubble Gum | `sc_start` handler + drop calculation in `handleEnemyDeath()` | Buff system + Loot |
| Battle Manual | `sc_start` handler + EXP distribution in `distributePartyEXP()` / direct EXP | Buff system + EXP |
| Gym Pass | NPC handler + new passive skill `ENLARGE_WEIGHT_LIMIT` | NPC + Skills |
| Marriage skills | Socket event handlers for WE_MALE/WE_FEMALE/WE_CALLPARTNER | Skill system |
| Wedding ceremony | REST API or socket event for marriage flow | NPC system |
| Adoption | Socket event + class change logic + stat recalculation | Character system |
| Rental items | Login handler + periodic timer + inventory queries | Item system |
