# Mount, Falcon, Trap & Performance -- Deep Research (Pre-Renewal)

> Sources: iRO Wiki Classic, iRO Wiki, RateMyServer Skill DB, rAthena pre-re source, GameFAQs guides, divine-pride.net, WarpPortal Forums
> All data is Pre-Renewal unless noted. Verified against multiple independent sources.

---

## Table of Contents

1. [Mount System](#mount-system)
2. [Falcon System](#falcon-system-hunter)
3. [Trap System](#trap-system-hunter)
4. [Performance System](#performance-system-barddancer)
5. [Implementation Checklist](#implementation-checklist)
6. [Gap Analysis](#gap-analysis)

---

## MOUNT SYSTEM

### Peco Peco (Knight)

The mount system allows Knight and Lord Knight characters to ride Peco Peco birds. The Peco Peco Ride skill (ID 708, rA: KN_RIDING) is a passive skill with max level 1.

**Prerequisites:**
- Knight class or higher (Lord Knight)
- Endure Lv 1

**Effects when mounted:**

| Attribute | Effect |
|-----------|--------|
| Movement Speed | Increased, approximately equal to Increase Agility effect |
| Weight Limit | +1,000 |
| Spear vs Medium | 100% damage (normally 75% for spears) |
| Spear Mastery | +1 ATK per Spear Mastery skill level (on top of the base +4/lv) |
| ASPD | **Reduced to 50%** of normal (halved attack speed) |

**Speed Bonus Detail:**
The speed bonus is approximately +25% movement speed, equivalent to the Increase Agility buff. The exact movement speed value matches the AGI buff speed tier. In rAthena source, the mounted speed is handled via a status change that applies a speed bonus roughly equivalent to `SC_INCREASEAGI`.

**Weight Capacity:**
The +1,000 weight bonus is applied while mounted. Dismounting removes this bonus -- if inventory weight exceeds the new (lower) limit after dismounting, the player enters overweight state. Players should manage inventory before dismounting.

**ASPD Penalty:**
Without Cavalier Mastery, ASPD is halved (50% of normal). This is a multiplicative penalty applied to the final ASPD calculation:
```
Mounted ASPD = Normal ASPD * (50% + CavalierMastery_Recovery%)
```

### Grand Peco (Crusader)

Crusaders and Paladins ride the Grand Peco (also called Swift Peco), which is visually distinct from the Knight's Peco Peco but mechanically identical.

**Prerequisites:**
- Crusader class or higher (Paladin)
- Faith Lv 1

**Differences from Knight mount:**

| Aspect | Knight | Crusader |
|--------|--------|----------|
| Mount Name | Peco Peco | Grand Peco (Swift Peco) |
| Visual Sprite | Standard Peco Peco | Different sprite (Grand Peco model) |
| Prerequisite | Endure Lv 1 | Faith Lv 1 |
| Rental NPC | Prontera Chivalry (55, 348) | Prontera Church (232, 318) |
| Speed Bonus | ~+25% | ~+25% (identical) |
| Weight Bonus | +1,000 | +1,000 (identical) |
| ASPD Penalty | -50% | -50% (identical) |
| Spear vs Medium | 100% | 100% (identical) |

The Crusader's mount originally shared the same sprite as the Knight's Peco Peco. It was updated to the Grand Peco sprite before the introduction of Paladins. Lord Knights and Paladins ride armored versions of their respective mounts.

**There is no gameplay difference between the Knight's Peco Peco and the Crusader's Grand Peco aside from aesthetics.** All mechanical effects (speed, weight, ASPD penalty, spear bonuses) are identical.

### Cavalier Mastery

**Skill ID:** 709 (rA: KN_CAVALIERMASTERY)
**Type:** Passive | **Max Level:** 5 | **Prerequisite:** Peco Peco Ride Lv 1

Cavalier Mastery recovers the ASPD lost from riding:

| Level | ASPD Recovery | Effective ASPD While Mounted |
|-------|--------------|------------------------------|
| 0 (no skill) | 0% | 50% of normal |
| 1 | +10% | 60% of normal |
| 2 | +20% | 70% of normal |
| 3 | +30% | 80% of normal |
| 4 | +40% | 90% of normal |
| 5 | +50% | 100% of normal (full recovery) |

At Cavalier Mastery Lv 5, the ASPD penalty from riding is completely negated.

Both Knight and Crusader share this same skill (shared in skill tree via `sharedTreePos`). The skill is essential for any mounted build -- without it, auto-attack DPS is halved.

### Mounting Requirements (Riding Skill, NPC Rental)

**Skill Requirement:**
- Must have Peco Peco Ride (ID 708) skill learned (passive, Lv 1)
- This skill is a 2nd-class skill requiring Endure Lv 1 (Knight) or Faith Lv 1 (Crusader)

**NPC Rental Process:**
1. Speak to the Peco Peco rental NPC at the designated location
2. The mount is applied instantly -- no Zeny cost in most pre-renewal implementations
3. The mount persists through map changes and re-login (saved to character state)
4. To dismount, speak to the same NPC or use a command

**Rental Locations:**

| Class | Mount | NPC Location | Map Coordinates |
|-------|-------|-------------|-----------------|
| Knight | Peco Peco | Prontera Chivalry | prontera (55, 348) |
| Lord Knight | Peco Peco | Prontera Chivalry | prontera (55, 348) |
| Crusader | Grand Peco | Prontera Church | prontera (232, 318) |
| Paladin | Grand Peco | Prontera Church | prontera (232, 318) |

**Cost:** Free in most pre-renewal implementations. Some servers charge a nominal fee.

### Indoor Map Restrictions

In **pre-renewal classic**, there are **no indoor map restrictions** for Peco Peco mounts. Players can ride their mount in any map, including indoor dungeons, castles, and buildings.

Indoor mount restrictions were introduced in **Renewal** for Dragon mounts (Rune Knight) and Gryphon mounts (Royal Guard), but the original Peco Peco system had no such limitation.

> **Implementation Note:** For authenticity, do NOT implement indoor restrictions for Peco Peco/Grand Peco. This was not a classic mechanic.

### Mount Visual Change

When a player mounts a Peco Peco:
- The character sprite changes completely to a mounted sprite (taller model)
- The character sits atop the bird with a riding animation
- Movement animation changes to the mount's walking/running animation
- Attack animation changes to mounted attack animation
- All equipped weapons and armor remain visible on the mounted sprite
- The mount has its own idle animation
- Lord Knight and Paladin ride armored versions of the mount

Other players see the mounted sprite. The falcon is NOT visible while mounted (in classic, falcon and mount were mutually exclusive for Rangers, but for Hunters and Knights this is N/A since they are different classes entirely).

### Spear Damage Bonus While Mounted (vs Medium Size)

In pre-renewal, spear weapons have a **75% damage modifier vs Medium size** enemies by default. When mounted on a Peco Peco, this modifier changes to **100%**.

This is a significant damage boost because many common PvE monsters are Medium size. Combined with the Spear Mastery bonus (+1 extra ATK per Spear Mastery level while mounted, for a total of +5 ATK/lv instead of +4), mounted spear Knights deal considerably more damage to Medium targets.

**Size Modifiers for Spears:**

| Size | Unmounted | Mounted |
|------|-----------|---------|
| Small | 75% | 75% |
| Medium | 75% | **100%** |
| Large | 100% | 100% |

### Dismount Mechanics

- Dismount by talking to the rental NPC again
- Some servers support `/mount` chat command
- Dismounting removes the +1,000 weight bonus -- inventory may become overweight
- Dismounting instantly restores normal ASPD (Cavalier Mastery no longer relevant)
- Dismounting restores normal movement speed (mount speed bonus removed)
- Skills requiring mount (Brandish Spear) become unusable after dismounting
- Spear vs Medium bonus reverts to 75%

### Skill Interactions While Mounted

| Skill | Mounted Interaction |
|-------|---------------------|
| **Brandish Spear** | **Requires** mount -- cannot use unmounted |
| Spear Boomerang | Usable while mounted (ranged, blocked by Pneuma) |
| Charge Attack | Usable while mounted (ranged, blocked by Pneuma) |
| Grand Cross | Usable (no special interaction) |
| Bowling Bash | Usable (ASPD penalty affects usage speed without Cavalier Mastery) |
| Pierce | Usable (benefits from Medium size bonus) |
| Spear Stab | Usable |
| All melee skills | Affected by ASPD penalty unless Cavalier Mastery Lv5 |
| Increase AGI | Speed bonus stacks additively (mount speed + AGI speed) |

---

## FALCON SYSTEM (Hunter)

### Falcon Rental (NPC, Cost)

**Skill Requirement:** Falconry Mastery (rA: HT_FALCON, ID 127)
- Type: Passive | Max Level: 1
- Prerequisite: Beast Bane Lv 1
- Enables renting and commanding a Falcon

**Rental Method:**
1. **Hunter Guild NPC:** Rent a falcon at the Hunter Guild for **2,500 Zeny**
2. **Falcon Pipes:** Consumable items purchasable outside the Archer Guild and inside the Hunter Guild for **10,000 Zeny** each. Can be used from anywhere to summon a falcon without visiting the NPC.

**Maintenance:** Falcons require no food, no intimacy management, and no special attention. They remain summoned until the player unequips them or logs off (persists through map changes).

**Classes:** Hunter, Sniper (Renewal: Ranger with Hawk Mastery)

### Blitz Beat Skill (Manual Activation, INT-based Damage)

**Skill ID:** rA: HT_BLITZBEAT (129)
**Type:** Offensive | **Max Level:** 5 | **Target:** Single (3x3 splash)

**SP Cost:** `7 + (SkillLv * 3)` = 10 / 13 / 16 / 19 / 22

| Level | Hits | SP Cost | Cast Time | Cast Delay |
|-------|------|---------|-----------|------------|
| 1 | 1 | 10 | 1.5s | 1.0s |
| 2 | 2 | 13 | 1.5s | 1.0s |
| 3 | 3 | 16 | 1.5s | 1.0s |
| 4 | 4 | 19 | 1.5s | 1.0s |
| 5 | 5 | 22 | 1.5s | 1.0s |

**Range:** `5 + Vulture's Eye level` cells

**Damage Formula (Pre-Renewal / Classic -- INT-based):**
```
DamagePerHit = 80 + (SteelCrowLv * 6) + (Floor(INT / 2) * 2) + (Floor(DEX / 10) * 2)
TotalDamage  = DamagePerHit * NumberOfHits
```

Simplified:
```
DamagePerHit = 80 + (SteelCrowLv * 6) + INT + Floor(DEX / 5)
```

> **Note on INT vs AGI:** The original pre-renewal formula uses **INT**. A May 2020 update changed this to AGI for Renewal servers. For classic implementation, always use INT.

**Damage Properties:**
- Neutral element
- MISC damage type (ignores DEF and MDEF)
- Does NOT benefit from ATK cards, weapon element, or size modifiers
- Ignores accuracy check (always hits)
- Bypasses Guard, Defending Aura, Parrying in PvP/WoE
- Does not inflict status ailments

**Manual Cast vs Auto-Cast Differences:**

| Property | Manual Cast | Auto-Cast |
|----------|------------|-----------|
| SP Cost | 10-22 | **0** (free) |
| Cast Time | 1.5s | **0** (instant) |
| Cast Delay | 1.0s | **0** |
| Damage Split | **Not split** (full damage to each target) | **Split** (divided by number of targets in 3x3) |
| Hit Count | Based on skill level | Based on Job Level |
| Trigger | Player activates skill | Triggers on normal bow attack |

### Auto Blitz Beat (Passive Trigger, LUK-based Chance, Formula)

Auto Blitz Beat is a passive effect that triggers Blitz Beat automatically during normal bow attacks.

**Trigger Chance Formula:**
```
AutoBlitzChance = Floor(LUK / 3)    (percentage)
```

| LUK | Chance |
|-----|--------|
| 3 | 1% |
| 30 | 10% |
| 45 | 15% |
| 60 | 20% |
| 75 | 25% |
| 99 | 33% |

**Trigger Rules:**
- Only triggers on **normal attacks with a bow** (not skills, not daggers, not fists)
- Triggers even if the normal attack **misses** or is **blocked** by a skill
- Essentially: a chance roll for every arrow fired
- Does not consume SP
- No cast time, no cast delay
- Can trigger during cast delay of other skills

**Auto-Cast Hit Count (Based on Job Level):**
```
AutoBlitzHits = min(BlitzBeatSkillLv, Floor((JobLevel + 9) / 10))
```

Simplified hit table:

| Job Level | Max Auto-Blitz Hits |
|-----------|---------------------|
| 1-9 | 1 |
| 10-19 | 2 |
| 20-29 | 3 |
| 30-39 | 4 |
| 40+ | 5 |

The actual hit count is capped by the learned Blitz Beat skill level. So a Job 50 character with Blitz Beat Lv 3 will only auto-blitz 3 hits.

**Auto-Cast Damage Split:**
Auto Blitz Beat damage is **divided by the number of enemies** in the 3x3 AoE. If 3 enemies are hit, each receives 1/3 of the total damage. Manual Blitz Beat does NOT split damage.

### Steel Crow Passive (Falcon Damage Bonus)

**Skill ID:** rA: HT_STEELCROW (128)
**Type:** Passive | **Max Level:** 10 | **Prerequisite:** Falconry Mastery Lv 1

Increases falcon damage per hit by a flat bonus:

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +Damage/hit | 6 | 12 | 18 | 24 | 30 | 36 | 42 | 48 | 54 | 60 |

The bonus is `SteelCrowLv * 6` added to each hit of Blitz Beat (both manual and auto-cast).

At max level, Steel Crow adds 60 damage per hit. With 5-hit Blitz Beat, that is +300 total bonus damage.

### Falcon vs Peco (Mutually Exclusive? Class-Specific)

In pre-renewal classic, falcons and Peco Peco mounts are **class-specific** and thus naturally exclusive:

| Feature | Classes | Notes |
|---------|---------|-------|
| Falcon | Hunter, Sniper | Requires Falconry Mastery |
| Peco Peco | Knight, Lord Knight | Requires Riding (via Endure) |
| Grand Peco | Crusader, Paladin | Requires Riding (via Faith) |
| Cart | Merchant tree | Requires Pushcart |

Since no class in pre-renewal can learn both Falconry Mastery and Peco Peco Ride, the question of mutual exclusivity is moot -- they are inherently separated by class.

**Renewal Note:** In Renewal, Rangers can learn both Falcon/Hawk Mastery and Warg Mastery, but cannot have both summoned simultaneously (Rangers toggle between falcon and warg). This is a Renewal mechanic not applicable to pre-renewal.

**Cart + Falcon restriction:** The reference docs mention "Cannot rent a falcon while a cart is equipped." This applies only to hypothetical multi-class scenarios (not relevant in pre-renewal where Hunters never have carts).

### Falcon Attack Animation

- The falcon sits perched on the player's right shoulder during idle and movement
- When Blitz Beat triggers (manual or auto), the falcon visually flies from the player toward the target
- The falcon strikes the target with a diving attack animation
- After the strike, the falcon returns to the player's shoulder
- Other players can see the falcon on the character and its attack animation
- The falcon sprite differs between Hunter and Sniper classes

### Detect Skill (Falcon Utility)

**Skill ID:** rA: HT_DETECTING (130)
**Type:** Active | **Max Level:** 4 | **SP Cost:** 8

| Level | Range |
|-------|-------|
| 1 | 2 cells |
| 2 | 4 cells |
| 3 | 6 cells |
| 4 | 8 cells |

Sends the falcon to reveal hidden/cloaked enemies in a 3x3 area around the target cell. Requires a falcon to use.

### Falcon Assault (Sniper Skill)

**Type:** Offensive | **Max Level:** 5 | **Target:** Single
**Prerequisites:** Blitz Beat Lv 5, Steel Crow Lv 3

**Damage Formula:**
```
Damage = ((Floor(AGI/2)*2 + Floor(DEX/10)*2 + BlitzBeatLv*20 + SteelCrowLv*6) * SkillLevel + SteelCrowLv*6) * (SteelCrowLv/20 + SkillLevel + BaseLv/50)
```

**SP Cost:** `26 + (SkillLv * 4)` = 30 / 34 / 38 / 42 / 46

Single hit (not multi-hit like Blitz Beat). Requires a falcon. Has variable cast time (reduced by DEX).

---

## TRAP SYSTEM (Hunter)

### Trap Placement Mechanics (Cell-Based, Max Active Traps)

**Placement Rules:**
- Each trap occupies **1 cell** on the ground
- Traps can only be placed up to **2 cells** from the caster (range: 3 cells)
- Traps can only be placed up to **2 cells** from players, monsters, or other traps
- Cannot place a trap directly on top of a player, monster, or existing trap
- Traps cannot stack on the same cell
- Placing a trap in the AoE of an existing trap may destroy the existing trap (varies by type)

**Maximum Active Traps:**
- In rAthena default configuration, there is no hardcoded maximum number of active traps
- The practical limit is constrained by trap duration and SP cost
- The `skill_max_trap` and `global_max_trap` config options exist but default to 0 (disabled)
- rAthena `ActiveInstance` per skill_db can set per-skill limits
- Common server-enforced limit: 2-3 of the same trap type active at once (varies by server)

> **Implementation Recommendation:** Implement a configurable `MAX_ACTIVE_TRAPS` per trap type. Suggested defaults: 2 per trap type, with a global limit of ~5 total traps per player.

**Trap Item Consumption:**

| Trap Skill | Items Consumed |
|------------|---------------|
| Skid Trap | 1 Trap |
| Land Mine | 1 Trap |
| Ankle Snare | 1 Trap |
| Sandman | 1 Trap |
| Flasher | 1 Trap |
| Freezing Trap | **2 Traps** |
| Blast Mine | **2 Traps** |
| Claymore Trap | **2 Traps** |
| Shockwave Trap | **2 Traps** |
| Remove Trap | 0 (recovers trap item) |
| Spring Trap | 0 (destroys trap remotely) |
| Talkie Box | 1 Trap |
| Detect | 0 (falcon skill, no trap item) |

**Special Alloy Trap:** An alternative trap item with weight 0.1 (vs regular Trap weight 1.0). If Trap Research is learned, any trap that costs 2 regular Traps can be made with 1 Special Alloy Trap instead.

**Trap Durability:** All traps have **3,500 HP** and can be destroyed by attacks.

**Trap Visibility:**
- Traps are visible to the Hunter who placed them
- Traps are visible to party members
- Traps are **invisible** to enemies in PvP/WoE (they appear as a generic ground indicator)
- In PvE, traps are visible on the ground cell

**WoE (War of Emperium):**
- Trap duration is **4x longer** in WoE
- Traps affect allies, enemies, and the user in PvP environments

### Trap Types -- Complete Reference

#### Skid Trap

**Skill ID:** 115 (rA: HT_SKIDTRAP) | **Element:** None | **SP Cost:** 10 | **Trap Cost:** 1

| Level | Slide Distance | Trap Duration |
|-------|---------------|---------------|
| 1 | 6 cells | 5 minutes |
| 2 | 7 cells | 4 minutes |
| 3 | 8 cells | 3 minutes |
| 4 | 9 cells | 2 minutes |
| 5 | 10 cells | 1 minute |

- Slides enemy in a direction determined by the trap's position relative to the caster
- After the slide, the enemy is immobilized for **4 seconds**
- Does NOT push Boss monsters
- No damage

#### Land Mine

**Skill ID:** 116 (rA: HT_LANDMINE) | **Element:** Earth | **SP Cost:** 10 | **Trap Cost:** 1

**Damage Formula:**
```
Damage = [DEX * (3 + BaseLv / 100) * (1 + INT / 35)] * SkillLv +/- 10% + (TrapResearchLv * 40)
```

| Level | Damage Multiplier | Trap Duration | Stun Chance |
|-------|-------------------|---------------|-------------|
| 1 | 1x | 200s | ~50% |
| 2 | 2x | 160s | ~60% |
| 3 | 3x | 120s | ~70% |
| 4 | 4x | 80s | ~80% |
| 5 | 5x | 40s | ~90% |

Duration formula: `240 - (SkillLv * 40)` seconds

- Stun does NOT affect Boss monsters (damage still applies)
- AoE: single target (the monster that steps on it)
- Damage type: Melee (ignores accuracy check)
- Ignores Guard, Parrying, Weapon Blocking

#### Ankle Snare

**Skill ID:** 117 (rA: HT_ANKLESNARE) | **Element:** None | **SP Cost:** 12 | **Trap Cost:** 1

**Effect Duration Formula:**
```
EffectDuration = BaseDuration - (TargetAGI / 10)    (seconds)
```

| Level | Base Effect Duration | Trap Duration |
|-------|---------------------|---------------|
| 1 | 4s | 250s |
| 2 | 8s | 200s |
| 3 | 12s | 150s |
| 4 | 16s | 100s |
| 5 | 20s | 50s |

- Immobilizes the target (cannot move, CAN still attack and use skills)
- **Boss monsters:** Trapped for **1/5** of the normal duration
- Minimum duration: `3000ms + 30ms * CasterBaseLv`
- AoE: 3x3 trigger area (traps 1 target)
- Does NOT deal damage
- Prerequisite: Skid Trap Lv 1

#### Shockwave Trap

**Skill ID:** 118 (rA: HT_SHOCKWAVE) | **Element:** None | **SP Cost:** 45 | **Trap Cost:** 2

| Level | SP Drain | Trap Duration |
|-------|----------|---------------|
| 1 | 20% of target SP | 200s |
| 2 | 35% | 160s |
| 3 | 50% | 120s |
| 4 | 65% | 80s |
| 5 | 80% | 40s |

Duration formula: `240 - (SkillLv * 40)` seconds

- Drains SP from all enemies in the area of effect
- Does NOT deal HP damage
- Particularly effective vs magic-using monsters/players

#### Sandman

**Skill ID:** 119 (rA: HT_SANDMAN) | **Element:** None | **SP Cost:** 12 | **Trap Cost:** 1

| Level | Sleep Chance | Trap Duration |
|-------|-------------|---------------|
| 1 | 50% | 150s |
| 2 | 60% | 120s |
| 3 | 70% | 90s |
| 4 | 80% | 60s |
| 5 | 90% | 30s |

Duration formula: `180 - (SkillLv * 30)` seconds

- AoE: **3x3** cells
- Sleep status lasts until hit or until natural expiry
- Does NOT affect Boss monsters

#### Flasher

**Skill ID:** 120 (rA: HT_FLASHER) | **Element:** None | **SP Cost:** 12 | **Trap Cost:** 1

| Level | Blind Chance | Trap Duration | Blind Duration |
|-------|-------------|---------------|----------------|
| 1 | 50% | 150s | 18s |
| 2 | 60% | 120s | 18s |
| 3 | 70% | 90s | 18s |
| 4 | 80% | 60s | 18s |
| 5 | 90% | 30s | 18s |

Duration formula: `180 - (SkillLv * 30)` seconds

- AoE: **3x3** cells
- Does NOT blind Boss monsters

#### Freezing Trap

**Skill ID:** 121 (rA: HT_FREEZINGTRAP) | **Element:** Water | **SP Cost:** 10 | **Trap Cost:** 2

**Damage Formula:**
```
Damage = PlayerATK + (TrapResearchLv * 40)
```

| Level | Freeze Chance | Freeze Duration | Trap Duration |
|-------|-------------|-----------------|---------------|
| 1 | 50% | 6s | 150s |
| 2 | 60% | 9s | 120s |
| 3 | 70% | 12s | 90s |
| 4 | 80% | 15s | 60s |
| 5 | 90% | 18s | 30s |

Trap duration formula: `180 - (SkillLv * 30)` seconds

- AoE: **3x3** cells
- Water element piercing physical damage
- Does NOT freeze Boss monsters or Undead-element monsters (damage still applies)
- Ignores the "always takes 1 damage" flag

#### Blast Mine

**Skill ID:** 122 (rA: HT_BLASTMINE) | **Element:** Wind | **SP Cost:** 10 | **Trap Cost:** 2

**Damage Formula:**
```
Damage = [DEX * (3 + BaseLv / 100) * (1 + INT / 35)] * SkillLv +/- 10% + (TrapResearchLv * 40)
```

| Level | Damage Multiplier | Trap Duration |
|-------|-------------------|---------------|
| 1 | 1x | 25s |
| 2 | 2x | 20s |
| 3 | 3x | 15s |
| 4 | 4x | 10s |
| 5 | 5x | 5s |

Duration formula: `30 - (SkillLv * 5)` seconds

- AoE: **3x3** cells
- Wind element piercing damage
- Damage type: Melee (ignores accuracy check)
- **Unique:** This trap gets pushed back 3 cells when struck by physical damage (the only trap with this property)
- Activates on enemy contact OR when duration expires (detonates on timeout)

#### Claymore Trap

**Skill ID:** 123 (rA: HT_CLAYMORETRAP) | **Element:** Fire | **SP Cost:** 15 | **Trap Cost:** 2

**Damage Formula:**
```
Damage = [DEX * (3 + BaseLv / 85) * (1.1 + INT / 35)] * SkillLv +/- 10% + (TrapResearchLv * 40)
```

Note: Claymore uses `BaseLv / 85` instead of `BaseLv / 100` and `1.1` instead of `1` in the INT multiplier, making it stronger than Blast Mine/Land Mine.

| Level | Damage Multiplier | Trap Duration |
|-------|-------------------|---------------|
| 1 | 1x | 20s |
| 2 | 2x | 40s |
| 3 | 3x | 60s |
| 4 | 4x | 80s |
| 5 | 5x | 100s |

- AoE: **5x5** cells (largest trap AoE)
- Fire element piercing damage
- Knockback based on direction relative to trap position
- Cannot stack; destroys other traps in its AoE

#### Remove Trap

**Skill ID:** 124 (rA: HT_REMOVETRAP) | **SP Cost:** 5 | **Trap Cost:** 0

- Recovers the player's own trap and returns the trap item(s) to inventory
- Can only target your own traps
- Range: melee (adjacent cells)

#### Talkie Box

**Skill ID:** 125 (rA: HT_TALKIEBOX) | **SP Cost:** 1 | **Trap Cost:** 1

- Displays a player-defined text message when an enemy steps on it
- Purely cosmetic/utility -- no damage or status effect

#### Spring Trap

**Skill ID:** 131 (rA: HT_SPRINGTRAP) | **SP Cost:** 10 | **Trap Cost:** 0

| Level | Range |
|-------|-------|
| 1 | 4 cells |
| 2 | 5 cells |
| 3 | 6 cells |
| 4 | 7 cells |
| 5 | 8 cells |

- **Requires:** Falconry Mastery Lv 1, Remove Trap Lv 1, and an active falcon
- Sends the falcon to **destroy** a trap at range
- The trap is destroyed (NOT recovered -- unlike Remove Trap)
- Can destroy traps set by **other Hunters/Snipers** (not just your own)
- Cannot destroy traps set by monsters or Rogue class
- Prerequisite: Remove Trap Lv 1

### Trap Damage Formula Summary

**DEX/INT-based traps** (Land Mine, Blast Mine, Claymore):
```
// Land Mine & Blast Mine
Damage = [DEX * (3 + BaseLv/100) * (1 + INT/35)] * SkillLv +/- 10% + TrapResearch*40

// Claymore (slightly stronger formula)
Damage = [DEX * (3 + BaseLv/85) * (1.1 + INT/35)] * SkillLv +/- 10% + TrapResearch*40
```

**ATK-based traps** (Freezing Trap):
```
Damage = PlayerATK + TrapResearch*40
```

**Stat breakpoints for INT:** Every 35 INT = +1 multiplier (35, 70, 105, 140).
**DEX:** Primary scaling stat for trap damage.

### Trap Interactions

**Arrow Shower pushing traps:**
- Arrow Shower can push traps in the knockback direction, similar to how it pushes monsters
- This allows Hunters to push traps underneath monsters (which is otherwise impossible due to placement restrictions)
- Stacking multiple traps and pushing them all at once with Arrow Shower is a classic Hunter tactic

**Detonator (Sniper skill):**
- Sniper skill that remotely activates traps from a distance
- Can detonate Sandman, Flasher, and other traps on command

**Land Protector (Sage):**
- Destroys all traps within its AoE
- Prevents new trap placement within the area

**Boss Monster Interactions:**

| Effect | vs Boss |
|--------|---------|
| Ankle Snare | 1/5 duration |
| Skid Trap | No pushback |
| Sandman | No sleep |
| Flasher | No blind |
| Freezing Trap | No freeze (damage still applies) |
| Land Mine | No stun (damage still applies) |
| Blast Mine | Full damage |
| Claymore Trap | Full damage |
| Shockwave Trap | SP drain still works |

### Trap Research (Sniper Passive)

**Max Level:** 5

| Level | +Trap Damage | Other |
|-------|-------------|-------|
| 1 | +40 | Unlocks Special Alloy Trap usage |
| 2 | +80 | |
| 3 | +120 | |
| 4 | +160 | |
| 5 | +200 | |

Adds flat damage to all damaging traps. Also enables using Special Alloy Traps (1 SAT = 2 regular Traps for skills that consume 2).

---

## PERFORMANCE SYSTEM (Bard/Dancer)

### How Performances Work (Ground AoE, Follows Caster)

Performances are ground-effect skills that create a **7x7 AoE** (solo) or **9x9 AoE** (ensemble) centered on the caster. The AoE follows the caster as they move (unlike fixed ground effects like Storm Gust).

**Key Characteristics:**
- Affects ALL entities in the AoE (allies, enemies, party members, non-party members)
- The **caster is NOT affected** by their own solo performance
- In ensembles, one of the two performers IS affected (the non-initiator)
- Requires the correct weapon type: **Instrument** (Bard) or **Whip** (Dancer)
- Switching weapons cancels the performance immediately

**Aftereffect (Lingering Duration):**
When a player leaves the performance AoE, the buff/debuff effect persists for approximately **20 seconds**. If they re-enter before the 20 seconds expire, the buff does NOT restart -- it continues from where it was. Example: if a player leaves at 19 seconds remaining, re-entering only gives 1 more second, not a fresh 20 seconds.

### SP Drain Per Tick

During a performance, the caster's SP does NOT regenerate naturally. Additionally, SP is drained at a rate specific to each skill. Most performances drain **1 SP every 3-5 seconds**.

**SP Drain Rates by Skill:**

| Skill | SP Drain Rate |
|-------|--------------|
| A Whistle | 1 SP / 5s |
| Assassin Cross of Sunset | 1 SP / 3s |
| A Poem of Bragi | 1 SP / 3s |
| The Apple of Idun | 1 SP / 3s |
| Humming | 1 SP / 5s |
| Please Don't Forget Me | 1 SP / 3s |
| Fortune's Kiss | 1 SP / 5s |
| Service For You | 1 SP / 5s |
| All Ensemble Skills | 1 SP / 3s (both performers drain) |

The performance automatically ends when the caster runs out of SP.

### Movement Speed Reduction While Performing

While performing a solo song or dance, the caster moves at **significantly reduced speed**. The reduction is partially mitigated by Music Lessons (Bard) or Dancing Lessons (Dancer).

**Movement Speed Formula While Performing:**
```
PerformingSpeed = BaseWalkSpeed * (25 + 2.5 * MusicLessonsLv) / 100
```

| Music/Dancing Lessons Lv | Walk Speed While Performing |
|--------------------------|----------------------------|
| 0 | 25% of normal |
| 1 | 27.5% |
| 2 | 30% |
| 3 | 32.5% |
| 4 | 35% |
| 5 | 37.5% |
| 6 | 40% |
| 7 | 42.5% |
| 8 | 45% |
| 9 | 47.5% |
| 10 | **50% of normal** (maximum) |

At Music Lessons Lv 10, the performer walks at half speed. Without the skill, they move at only 25% speed.

### Cancel Conditions

Performances can be cancelled by:

| Condition | Effect |
|-----------|--------|
| **Adaptation to Circumstances** (Amp) | Active cancel skill. Cannot be used in the first 5 seconds of a performance. SP cost: 1. |
| **Weapon swap** | Switching from Instrument/Whip to any other weapon (e.g., Bow, Dagger) cancels the performance. Used for "Dagger/Bow Amping" technique. |
| **Dispel** (Sage skill) | Cancels active performance |
| **Heavy damage** | Taking damage exceeding 25% of MaxHP in a single hit cancels the performance |
| **Silence status** | Does **NOT** cancel solo performances (contrary to intuition) |
| **SP depletion** | Running out of SP ends the performance |
| **New performance** | Starting a new song/dance auto-cancels the previous one |
| **Death** | Obviously cancels the performance |

**Adaptation to Circumstances (Amp):**
- Skill ID: rA: BA_ADAPTATION (304)
- SP Cost: 1
- Cancels current performance immediately
- **Cannot be used within the first 5 seconds** of starting a performance
- Has no cooldown after the initial 5-second lockout

**Encore:**
- Skill ID: rA: BA_ENCORE (305)
- SP Cost: Half of the last performance's SP cost
- Replays the last performance used at half SP cost
- Clears the remembered skill after use (cannot Encore twice in a row)

### Song Overlap -- Dissonance / Ugly Dance

When two performances of the **same class** overlap on the ground:

| Overlap Type | Result |
|-------------|--------|
| Two Bard songs overlap | Overlapping cells become **Dissonance Lv 1** |
| Two Dancer dances overlap | Overlapping cells become **Ugly Dance (Hip Shaker) Lv 1** |
| Bard song + Dancer dance overlap | No conflict -- both effects apply independently |

**Dissonance:**
- Deals MISC damage (ignores DEF and MDEF) to all enemies in the overlap area
- Damage ticks periodically

**Ugly Dance (Hip Shaker):**
- Drains SP from all enemies in the overlap area
- SP drain ticks periodically

> **Important:** This is an anti-stacking mechanic. Two Bards cannot buff the same area with different songs -- the overlap zone converts to a damage/debuff effect. This encourages positioning strategy.

### Ensemble Mechanics (Midpoint, Both Must Maintain)

Ensembles (duet skills) require both a Bard and a Dancer working together.

**Requirements:**
- A Bard/Minstrel and a Dancer/Gypsy must be in the **same party**
- Both must have at least Lv 1 of the ensemble skill
- Both must have the correct weapon equipped (Instrument + Whip)
- They must be standing **adjacent** to each other (within 1 cell)

**Starting an Ensemble:**
- Either the Bard or Dancer can initiate the ensemble
- The other partner is **forced** to participate automatically (they cannot refuse)
- The non-initiator can cancel using Adaptation to Circumstances (Amp)

**Skill Level Calculation:**
```
EnsembleLevel = min(BardSkillLv, DancerSkillLv)
```
The **lower** of the two partners' skill levels is used. (Some sources say average rounded down -- the rAthena implementation uses minimum.)

**Ensemble AoE:**
- **9x9 cells** (larger than solo 7x7)
- Centered at the **midpoint** between the two performers
- The midpoint is calculated as the average of both performers' positions:
```
MidpointX = Floor((BardX + DancerX) / 2)
MidpointY = Floor((BardY + DancerY) / 2)
```

**Ensemble Ground Effect:**
- Unlike solo performances which **follow the caster**, ensembles create a **stationary ground effect** at the midpoint
- The effect does NOT move if the performers move
- If either performer moves too far from the midpoint (out of range), the ensemble ends

**Movement Restrictions During Ensemble:**
- Both performers **cannot move** during an ensemble
- Both performers **cannot attack** during an ensemble
- They CAN still use **Musical Strike** (Bard) and **Slinging Arrow** (Dancer) during ensembles
- They can use Adaptation to break the ensemble

**SP Drain:**
- Both performers drain SP at the ensemble's rate (typically 1 SP / 3s each)
- SP does not regenerate during ensemble
- If either performer runs out of SP, the ensemble ends

**Ensemble Skills List:**

| Skill | AoE | SP (Bard+Dancer) | Effect |
|-------|-----|------------------|--------|
| Lullaby | 9x9 | 20-40 | Sleep enemies (caster's party members immune) |
| Mr. Kim A Rich Man (Mental Sensing) | 9x9 | 62-86 | +EXP 20-60% for party |
| Eternal Chaos (Down Tempo) | 9x9 | 30-120 | Reduce enemy DEF to 0 |
| A Drum on the Battlefield (Battle Theme) | 9x9 | 38-50 | +ATK, +DEF |
| The Ring of Nibelungen (Harmonic Lick) | 9x9 | 48-64 | Lv4 weapon bonus damage |
| Loki's Veil (Classical Pluck) | 9x9 | 15 | Disable ALL skills (enemies AND allies) |
| Into the Abyss (Power Cord) | 9x9 | 10-70 | No gemstone costs for skills |
| Invulnerable Siegfried (Acoustic Rhythm) | 9x9 | 20-56 | Element resist 60-80%, status resist 10-50% |

### Solo Performance Skills -- Complete Reference

**Bard Solo Skills (Instrument Required):**

| Skill | ID | AoE | SP | Effect |
|-------|-----|-----|----|--------|
| A Whistle (Perfect Tablature) | 319 | 7x7 | 22-40 | +FLEE, +Perfect Dodge |
| Assassin Cross of Sunset (Magic Strings) | 320 | 7x7 | 40-85 | Reduces After-Attack Delay |
| A Poem of Bragi | 321 | 7x7 | 40-85 | Reduces Cast Time and Skill Delay |
| The Apple of Idun (Song of Lutie) | 322 | 7x7 | 40-85 | +MaxHP, +HP Recovery |

**Dancer Solo Skills (Whip Required):**

| Skill | ID | AoE | SP | Effect |
|-------|-----|-----|----|--------|
| Humming (Focus Ballet) | 327 | 7x7 | 22-60 | +HIT |
| Please Don't Forget Me (Slow Grace) | 328 | 7x7 | 28-65 | -Enemy ASPD and Movement Speed |
| Fortune's Kiss (Lady Luck) | 329 | 7x7 | 40-85 | +CRIT |
| Service For You (Gypsy's Kiss) | 330 | 7x7 | 40-87 | +MaxSP, -SP Consumption |

### Performance Utility Skills

| Skill | Class | Effect |
|-------|-------|--------|
| Musical Strike (Melody Strike) | Bard | Ranged attack, usable during performance/ensemble |
| Slinging Arrow (Throw Arrow) | Dancer | Ranged attack, usable during performance/ensemble |
| Frost Joker (Unbarring Octave) | Bard | Screen-wide freeze chance (BLOCKED during performance) |
| Scream (Dazzler) | Dancer | Screen-wide stun chance (BLOCKED during performance) |
| Pang Voice | Bard | Confusion on target (BLOCKED during performance) |
| Charming Wink | Dancer | Charm on target (BLOCKED during performance) |

> **Critical Rule:** Only Musical Strike, Slinging Arrow, and Adaptation to Circumstances can be used during an active performance. All other skills (including Frost Joker, Scream, Pang Voice, Charming Wink) are BLOCKED while performing.

---

## Implementation Checklist

### Mount System

- [ ] `Riding` passive skill (ID 708) -- enables mount/dismount
- [ ] `CavalierMastery` passive skill (ID 709) -- ASPD recovery per level
- [ ] Mount NPC at designated location (Prontera Chivalry / Church)
- [ ] `/mount` chat command for mounting/dismounting
- [ ] Movement speed bonus while mounted (~+25%, equivalent to Increase AGI)
- [ ] Weight limit bonus +1,000 while mounted
- [ ] ASPD penalty: `ASPD * (50% + CavalierMastery * 10%)`
- [ ] Spear vs Medium size modifier: 100% when mounted (normally 75%)
- [ ] Spear Mastery bonus: +5 ATK/lv mounted (vs +4 unmounted)
- [ ] Brandish Spear: require mounted state
- [ ] Visual sprite change when mounted (mounted animation set)
- [ ] Mount persists through map changes and re-login
- [ ] Dismount removes weight bonus (handle overweight)
- [ ] Server: `player.isMounted` flag, saved to DB
- [ ] Client: mounted sprite/animation system
- [ ] Grand Peco vs Peco Peco: visual difference only (same mechanics)

### Falcon System

- [ ] `FalconryMastery` passive skill (ID 127/rA:127) -- enables falcon
- [ ] Falcon rental NPC at Hunter Guild (2,500z)
- [ ] Falcon Pipes consumable item (10,000z, summon from anywhere)
- [ ] `BlitzBeat` skill (ID 129/rA:129) -- manual cast, 1-5 hits
- [ ] INT-based damage formula: `80 + SteelCrow*6 + INT + Floor(DEX/5)` per hit
- [ ] Manual Blitz Beat: full damage to each target in 3x3
- [ ] Auto Blitz Beat: trigger on normal bow attack, chance = `LUK/3`%
- [ ] Auto Blitz Beat: hit count = `min(BB_Lv, Floor((JobLv+9)/10))`
- [ ] Auto Blitz Beat: damage split by number of targets
- [ ] Auto Blitz Beat: no SP cost, no cast time, no delay
- [ ] Auto Blitz Beat: can trigger on miss/blocked attack
- [ ] `SteelCrow` passive (ID 128/rA:128) -- +6 damage/lv per falcon hit
- [ ] `Detect` skill (ID 130/rA:130) -- falcon reveals hidden enemies
- [ ] `FalconAssault` (Sniper) -- single hit falcon skill
- [ ] Falcon visual: perched on shoulder, attack animation
- [ ] Server: `player.hasFalcon` flag, saved to DB
- [ ] Client: falcon sprite on character model

### Trap System

- [ ] Trap item consumption (1 or 2 per trap, see table)
- [ ] Trap placement: 3-cell range, 2-cell minimum from entities
- [ ] Trap cell occupancy (1 cell, no stacking)
- [ ] Trap durability: 3,500 HP
- [ ] Trap visibility system (visible to owner/party, hidden in PvP)
- [ ] Max active traps per player (configurable)
- [ ] **Skid Trap** (115): slide 6-10 cells, 4s immobilize, no boss push
- [ ] **Land Mine** (116): Earth, DEX/INT formula, stun chance, no boss stun
- [ ] **Ankle Snare** (117): immobilize, AGI-based duration, 1/5 vs boss
- [ ] **Shockwave Trap** (118): SP drain 20-80%, 2 traps
- [ ] **Sandman** (119): sleep 50-90% chance, 3x3, no boss
- [ ] **Flasher** (120): blind 50-90% chance, 3x3, 18s duration, no boss
- [ ] **Freezing Trap** (121): Water, ATK-based damage, freeze, 2 traps, no boss freeze
- [ ] **Blast Mine** (122): Wind, DEX/INT formula, 3x3, 2 traps, pushed by physical
- [ ] **Claymore Trap** (123): Fire, enhanced formula, 5x5, 2 traps, knockback
- [ ] **Remove Trap** (124): recover own trap items
- [ ] **Talkie Box** (125): display message, 1 trap
- [ ] **Spring Trap** (131): falcon destroys traps at range, requires falcon
- [ ] Arrow Shower trap pushing mechanic
- [ ] Trap Research (Sniper): +40 damage/lv, Special Alloy Trap
- [ ] WoE: 4x duration, affects allies
- [ ] Boss monster reduced effects (see interaction table)

### Performance System

- [ ] Solo performance AoE: 7x7, follows caster
- [ ] Ensemble AoE: 9x9, stationary at midpoint
- [ ] SP drain per tick (1 SP / 3-5s depending on skill)
- [ ] SP regen blocked during performance
- [ ] Movement speed reduction: `(25 + 2.5 * MusicLessonsLv)%` of normal
- [ ] Cancel: Adaptation (5s cooldown after start), weapon swap, Dispel, death
- [ ] Cancel: heavy damage (>25% MaxHP single hit)
- [ ] Silence does NOT cancel solo performance
- [ ] New performance auto-cancels previous
- [ ] Aftereffect: 20-second lingering buff after leaving AoE
- [ ] Aftereffect does NOT restart on re-entry
- [ ] Song overlap: two Bard songs -> Dissonance Lv1
- [ ] Song overlap: two Dancer dances -> Ugly Dance Lv1
- [ ] Caster excluded from own solo performance buff
- [ ] Ensemble: both performers in same party
- [ ] Ensemble: adjacent cells, correct weapons
- [ ] Ensemble: skill level = min(Bard_Lv, Dancer_Lv)
- [ ] Ensemble: midpoint = avg(positions)
- [ ] Ensemble: movement locked, attack locked
- [ ] Ensemble: Musical Strike / Slinging Arrow usable during ensemble
- [ ] Ensemble: Frost Joker/Scream/Pang Voice/Charming Wink BLOCKED during performance
- [ ] Encore: replays last performance at half SP, clears memory
- [ ] 4 Bard solo skills, 4 Dancer solo skills, 8 ensemble skills
- [ ] Weapon requirement: Instrument (Bard) / Whip (Dancer)

---

## Gap Analysis

### Current Implementation Status (from MEMORY.md / project state)

**Mount System:**
- [x] `/mount` chat command implemented
- [x] Riding speed: 1.36x (~36%) -- slightly higher than reference (+25% / Increase AGI)
- [x] Cavalier Mastery ASPD recovery implemented
- [x] Spear vs Medium 100% when mounted
- [x] Brandish Spear requires mount
- [ ] **GAP:** Weight limit +1,000 while mounted -- verify implementation
- [ ] **GAP:** Spear Mastery +1 ATK/lv bonus while mounted -- verify +5 vs +4
- [ ] **GAP:** Mount sprite change on client (visual only, may need art)
- [ ] **GAP:** Mount persistence through map changes -- verify DB save
- [ ] **GAP:** Dismount overweight handling

**Falcon System:**
- [x] Blitz Beat manual cast implemented
- [x] Auto Blitz Beat passive trigger implemented
- [x] Steel Crow passive implemented
- [x] INT-based damage formula (classic)
- [x] Auto-blitz job level hit count formula
- [ ] **GAP:** Falcon rental NPC (Hunter Guild, 2,500z) -- verify
- [ ] **GAP:** Falcon Pipes consumable item (10,000z)
- [ ] **GAP:** Detect skill -- verify implementation
- [ ] **GAP:** Falcon visual on client (shoulder perch, attack animation)
- [ ] **GAP:** Auto-blitz damage split vs manual no-split -- verify
- [ ] **GAP:** Falcon Assault (Sniper) -- not yet implemented (Sniper is future)

**Trap System:**
- [x] Ankle Snare implemented with `ankle_snare` status (movement-only, boss 1/5, AGI reduction)
- [x] Freezing Trap ATK-based damage implemented
- [x] Land Mine / Blast Mine / Claymore DEX/INT formulas
- [x] Most trap damage types and elements
- [ ] **GAP:** Trap placement distance rules (2-cell minimum from entities)
- [ ] **GAP:** Max active traps per player (configurable limit)
- [ ] **GAP:** Trap durability (3,500 HP, destructible)
- [ ] **GAP:** Trap visibility system (hidden from enemies in PvP)
- [ ] **GAP:** Arrow Shower trap pushing mechanic
- [ ] **GAP:** Claymore enhanced formula (`BaseLv/85` and `1.1 + INT/35`)
- [ ] **GAP:** Spring Trap (falcon remote trap destroy)
- [ ] **GAP:** Talkie Box
- [ ] **GAP:** Blast Mine pushed by physical attacks
- [ ] **GAP:** Shockwave Trap SP drain -- verify
- [ ] **GAP:** WoE 4x trap duration

**Performance System:**
- [x] Performance ground AoE follows caster (7x7 solo)
- [x] SP drain per tick implemented
- [x] Movement speed reduction during performance
- [x] Song overlap -> Dissonance Lv1
- [x] Caster excluded from own performance buff
- [x] Cancel conditions (weapon swap, Dispel, heavy damage, Adaptation 5s lockout)
- [x] Ensemble midpoint calculation and stationary AoE
- [x] Ensemble movement/attack lock
- [x] Musical Strike / Slinging Arrow usable during ensemble
- [x] Frost Joker/Scream/Pang Voice blocked during performance
- [x] Encore (half SP, clears memory)
- [x] Aftereffect 20-second lingering
- [x] ASPD exclusion groups (ACoS/AR/THQ/SQ)
- [ ] **GAP:** Aftereffect does NOT restart on re-entry -- verify this edge case
- [ ] **GAP:** Ugly Dance from two overlapping Dancer dances -- verify (distinct from Bard Dissonance)
- [ ] **GAP:** Silence does NOT cancel performance -- verify
- [ ] **GAP:** Performance visual effects on client (ground indicator, musical notes)

### Priority Gaps (Most Impactful)

1. **Mount visual sprite** -- critical for player experience, requires art assets
2. **Falcon visual** -- falcon perched on shoulder, attack animation
3. **Trap placement rules** -- 2-cell minimum prevents exploits
4. **Trap max active limit** -- prevents trap spam
5. **Arrow Shower trap pushing** -- core Hunter tactic
6. **Mount weight bonus** -- affects inventory management
7. **Trap durability** -- allows counterplay (destroy traps)

### Verified Correct Implementations

Based on MEMORY.md session notes:
- Auto Blitz Beat formula: `floor((jobLv+9)/10)` hits (verified rAthena)
- Blitz Beat damage: `(DEX/10+INT/2+SC*3+40)*2` (project uses slightly different constant layout but equivalent)
- Ankle Snare: `stun` -> `ankle_snare` status (movement-only, boss 1/5) -- correct
- Freezing Trap: ATK-based (not DEX/INT) -- correct per iRO Wiki
- Performance movement speed: `(25+2.5*ML)%` -- correct
- Ensemble stationary at midpoint -- correct
- Frost Joker/Scream blocked during performance -- correct (rAthena verified)

---

## Sources

- [Peco Peco Ride - iRO Wiki Classic](https://irowiki.org/classic/Peco_Peco_Ride)
- [Peco Peco Ride - iRO Wiki](https://irowiki.org/wiki/Peco_Peco_Ride)
- [Cavalier Mastery - rAthena Pre-renewal Database](https://pre.pservero.com/skill/KN_CAVALIERMASTERY)
- [Blitz Beat - iRO Wiki Classic](https://irowiki.org/classic/Blitz_Beat)
- [Blitz Beat - iRO Wiki](https://irowiki.org/wiki/Blitz_Beat)
- [Hunter - iRO Wiki Classic](https://irowiki.org/classic/Hunter)
- [Falconry Mastery - iRO Wiki](https://irowiki.org/wiki/Falconry_Mastery)
- [Ankle Snare - iRO Wiki](https://irowiki.org/wiki/Ankle_Snare)
- [Claymore Trap - iRO Wiki](https://irowiki.org/wiki/Claymore_Trap)
- [Blast Mine - iRO Wiki](https://irowiki.org/wiki/Blast_Mine)
- [Land Mine - iRO Wiki](https://irowiki.org/wiki/Land_Mine)
- [Freezing Trap - iRO Wiki](https://irowiki.org/wiki/Freezing_Trap)
- [Sandman - iRO Wiki](https://irowiki.org/wiki/Sandman)
- [Skid Trap - iRO Wiki](https://irowiki.org/wiki/Skid_Trap)
- [Shockwave Trap - iRO Wiki](https://irowiki.org/wiki/Shockwave_Trap)
- [Flasher - iRO Wiki](https://irowiki.org/wiki/Flasher)
- [Spring Trap - iRO Wiki](https://irowiki.org/wiki/Spring_Trap)
- [Ensemble Skill - iRO Wiki](https://irowiki.org/wiki/Ensemble_Skill)
- [Bard - iRO Wiki](https://irowiki.org/wiki/Bard)
- [Dancer - iRO Wiki](https://irowiki.org/wiki/Dancer)
- [Arrow Shower - iRO Wiki](https://irowiki.org/wiki/Arrow_Shower)
- [Ragnarok Online Mount System - LuffyKudo](https://luffykudo.wordpress.com/2024/06/01/ragnarok-online-mount-system/)
- [Pecopeco Mounts - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Pecopeco_Mounts)
- [rAthena skill.conf - GitHub](https://github.com/rathena/rathena/blob/master/conf/battle/skill.conf)
- [Bard/Dancer/Clown/Gypsy Guide - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/35759)
- [Ensembler Support Bard/Dancer Guide - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/32769)
- [Trap Hunter Guide - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/25790)
- [rAthena Trap Code Improvements](https://github.com/rathena/rathena/commit/bcba19025366f8bfe44b978b8bd620b03c1121a0)
- [rAthena ASPD Issue #4788](https://github.com/rathena/rathena/issues/4788)
- [rAthena Trap Damage Inaccuracies Issue #8227](https://github.com/rathena/rathena/issues/8227)
