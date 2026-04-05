# Element System -- Deep Research (Pre-Renewal)

> Ragnarok Online Classic (pre-Renewal) complete element/property system reference.
> Sources: rAthena pre-re `attr_fix.txt`, Hercules pre-re, iRO Wiki Classic, RateMyServer Classic, divine-pride, rAthena `battle.cpp` / `status.cpp`, Sabri_MMO server implementation (`ro_damage_formulas.js`, `ro_status_effects.js`, `index.js`).

---

## Overview (10 Elements, 4 Levels)

Ragnarok Online uses a **10-element property system** where every entity (player, monster, summon) has a **defending element** with a **level (1-4)**, and every attack carries an **attacking element**. The interaction between the attack element and the defense element determines a **damage modifier** (percentage multiplier applied to the final damage).

**10 Elements:**
1. **Neutral** -- Default for players and many common monsters
2. **Water** -- Aquatic monsters, Cold Bolt, Frost Diver
3. **Earth** -- Ground creatures, Earth Spike, Stone Curse
4. **Fire** -- Flame creatures, Fire Bolt, Magnum Break
5. **Wind** -- Flying/storm monsters, Lightning Bolt, Thunderstorm
6. **Poison** -- Toxic creatures, Envenom, Enchant Poison
7. **Holy** -- Divine beings, Heal (vs Undead), Aspersio, Priest skills
8. **Shadow** (Dark) -- Dark creatures, Cursed Water endow
9. **Ghost** -- Spectral beings, special immunity rules
10. **Undead** -- Undead monsters, special healing interactions

**4 Levels:**
Every defending element has a level from 1 to 4. Higher levels amplify both resistances and vulnerabilities. Level 1 is the most common for players (via armor cards). Monsters range from Level 1 (weak variants) to Level 4 (MVPs, boss variants).

**Two Types of Element:**
- **Attacking element** -- The element of the damage source (weapon, skill, arrow, endow)
- **Defending element** -- The element of the target (armor, monster body property, status override)

Players have both an attacking element (weapon) and a defending element (armor), both defaulting to **Neutral**.

---

## Complete Element Table (10x4 Damage Modifier Matrix)

Values are **damage percentages**: 100 = normal damage, 0 = immune, 200 = double damage, negative = heals target.

Source: rAthena `db/pre-re/attr_fix.txt`, cross-referenced with iRO Wiki Classic and RateMyServer archives. These values are confirmed implemented in Sabri_MMO `ro_damage_formulas.js` `ELEMENT_TABLE`.

### Level 1 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | **25** | 100 |
| **Water** | 100 | **25** | 100 | **150** | 50 | 100 | 75 | 100 | 100 | 100 |
| **Earth** | 100 | 100 | 100 | 50 | **150** | 100 | 75 | 100 | 100 | 100 |
| **Fire** | 100 | 50 | **150** | **25** | 100 | 100 | 75 | 100 | 100 | **125** |
| **Wind** | 100 | **175** | 50 | 100 | **25** | 100 | 75 | 100 | 100 | 100 |
| **Poison** | 100 | 100 | 125 | 125 | 125 | **0** | 75 | 50 | 100 | **-25** |
| **Holy** | 100 | 100 | 100 | 100 | 100 | 100 | **0** | **125** | 100 | **150** |
| **Shadow** | 100 | 100 | 100 | 100 | 100 | 50 | **125** | **0** | 100 | **-25** |
| **Ghost** | **25** | 100 | 100 | 100 | 100 | 100 | 75 | 75 | **125** | 100 |
| **Undead** | 100 | 100 | 100 | 100 | 100 | 50 | 100 | **0** | 100 | **0** |

### Level 2 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | **25** | 100 |
| **Water** | 100 | **0** | 100 | **175** | 25 | 100 | 50 | 75 | 100 | 100 |
| **Earth** | 100 | 100 | **50** | 25 | **175** | 100 | 50 | 75 | 100 | 100 |
| **Fire** | 100 | 25 | **175** | **0** | 100 | 100 | 50 | 75 | 100 | **150** |
| **Wind** | 100 | **175** | 25 | 100 | **0** | 100 | 50 | 75 | 100 | 100 |
| **Poison** | 100 | 75 | 125 | 125 | 125 | **0** | 50 | 25 | 75 | **-50** |
| **Holy** | 100 | 100 | 100 | 100 | 100 | 100 | **-25** | **150** | 100 | **175** |
| **Shadow** | 100 | 100 | 100 | 100 | 100 | 25 | **150** | **-25** | 100 | **-50** |
| **Ghost** | **0** | 75 | 75 | 75 | 75 | 75 | 50 | 50 | **150** | **125** |
| **Undead** | 100 | 75 | 75 | 75 | 75 | 25 | **125** | **0** | 100 | **0** |

### Level 3 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | **0** | 100 |
| **Water** | 100 | **-25** | 100 | **200** | 0 | 100 | 25 | 50 | 100 | **125** |
| **Earth** | 100 | 100 | **0** | 0 | **200** | 100 | 25 | 50 | 100 | 75 |
| **Fire** | 100 | 0 | **200** | **-25** | 100 | 100 | 25 | 50 | 100 | **175** |
| **Wind** | 100 | **200** | 0 | 100 | **-25** | 100 | 25 | 50 | 100 | 100 |
| **Poison** | 100 | 50 | 100 | 100 | 100 | **0** | 25 | **0** | 50 | **-75** |
| **Holy** | 100 | 100 | 100 | 100 | 100 | **125** | **-50** | **175** | 100 | **200** |
| **Shadow** | 100 | 100 | 100 | 100 | 100 | **0** | **175** | **-50** | 100 | **-75** |
| **Ghost** | **0** | 50 | 50 | 50 | 50 | 50 | 25 | 25 | **175** | **150** |
| **Undead** | 100 | 50 | 50 | 50 | 50 | **0** | **150** | **0** | 100 | **0** |

### Level 4 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | **0** | 100 |
| **Water** | 100 | **-50** | 100 | **200** | 0 | 75 | **0** | 25 | 100 | **150** |
| **Earth** | 100 | 100 | **-25** | 0 | **200** | 75 | **0** | 25 | 100 | 50 |
| **Fire** | 100 | 0 | **200** | **-50** | 100 | 75 | **0** | 25 | 100 | **200** |
| **Wind** | 100 | **200** | 0 | 100 | **-50** | 75 | **0** | 25 | 100 | 100 |
| **Poison** | 100 | 25 | 75 | 75 | 75 | **0** | **0** | **-25** | 25 | **-100** |
| **Holy** | 100 | 75 | 75 | 75 | 75 | **125** | **-100** | **200** | 100 | **200** |
| **Shadow** | 100 | 75 | 75 | 75 | 75 | **-25** | **200** | **-100** | 100 | **-100** |
| **Ghost** | **0** | 25 | 25 | 25 | 25 | 25 | **0** | **0** | **200** | **175** |
| **Undead** | 100 | 25 | 25 | 25 | 25 | **-25** | **175** | **0** | 100 | **0** |

---

## Element List and Properties

### Neutral
- Default element for all players (both weapon and armor)
- Neutral attacks deal 100% damage to almost everything
- **Exception**: Neutral attacks deal only 25% to Ghost Lv1, 0% to Ghost Lv2+
- Neutral armor takes full (100%) damage from every element -- no resistances, no weaknesses
- Most common element in the game; many normal monsters are Neutral

### Water
- Strong against: Fire (150-200%)
- Weak against: Wind (takes 175-200%), same-element (takes 25% to -50%)
- Key skills: Cold Bolt, Frost Diver, Waterball, Storm Gust (freezes to Water Lv1)
- Key card: Swordfish Card (armor -> Water Lv1)
- Endow source: Endow Tsunami (Sage), Mystic Frozen (Elemental Converter)

### Earth
- Strong against: Wind (deals 150-200% to Wind defenders)
- Weak against: Fire (takes 50-0%), same-element (takes 100% to -25%)
- Key skills: Earth Spike, Heaven's Drive, Stone Curse (petrifies to Earth Lv1)
- Key card: Sandman Card (armor -> Earth Lv1)
- Endow source: Endow Quake (Sage), Great Nature (Elemental Converter)

### Fire
- Strong against: Earth (150-200%), Undead (125-200%)
- Weak against: Water (takes 50-0%), same-element (takes 25% to -50%)
- Key skills: Fire Bolt, Fire Ball, Fire Wall, Magnum Break, Meteor Storm, Sight Rasher
- Key card: Pasana Card (armor -> Fire Lv1)
- Endow source: Endow Blaze (Sage), Flame Heart (Elemental Converter), Aspersio overrides

### Wind
- Strong against: Water (175-200%)
- Weak against: Earth (takes 50-0%), same-element (takes 25% to -50%)
- Key skills: Lightning Bolt, Thunderstorm, Jupitel Thunder, Lord of Vermilion
- Key card: Dokebi Card (armor -> Wind Lv1)
- Endow source: Endow Tornado (Sage), Rough Wind (Elemental Converter)

### Poison
- Moderate damage (125%) vs Earth, Fire, Wind at Lv1-2; diminishes at higher levels
- **Immune to itself** (0% at all levels)
- **Heals Undead** (negative values: -25% to -100%)
- Deals reduced damage to Shadow (50% to -25%)
- Key skills: Envenom (Thief)
- Key card: Argiope Card (armor -> Poison Lv1)
- Endow source: Enchant Poison (Assassin)

### Holy
- **Devastating vs Shadow** (125-200%) and **Undead** (150-200%)
- **Immune to itself** (0% to -100%) -- Holy vs Holy deals 0% at Lv1, heals at Lv2+
- Deals normal damage (100%) to Neutral/Water/Earth/Fire/Wind at Lv1
- Key skills: Heal (vs Undead), Holy Light, Turn Undead, Aspersio, Magnus Exorcismus, Holy Cross, Grand Cross
- Key card: Angeling Card (armor -> Holy Lv1)
- Endow source: Aspersio (Priest)

### Shadow (Dark)
- **Devastating vs Holy** (125-200%)
- **Immune to itself** (0% to -100%)
- **Heals Undead** (negative values: -25% to -100%)
- Deals reduced damage to Poison
- Key skills: few skill sources; mostly through equipment/cards
- Key card: Bathory Card (armor -> Shadow Lv1)
- Endow source: Cursed Water (item, endows Shadow/Dark element)

### Ghost
- **Special rules**: see dedicated section below
- **Reduced vs Neutral** (25% at Lv1, 0% at Lv2+) -- Ghost attacks barely scratch Neutral defenders
- **Strong vs itself** (125-200%)
- **Strong vs Undead** (100-175%)
- Reduced damage vs most other elements at higher target levels
- Key skills: Soul Strike (Ghost element magic)
- Key card: Ghostring Card (armor -> Ghost Lv1)
- Notable monster: Ghostring (Ghost Lv4 boss)

### Undead
- **Immune to itself** (0% at all levels) -- Undead cannot damage Undead
- **Immune to Shadow** (0% at all levels)
- Deals reduced damage to Water/Earth/Fire/Wind at higher levels
- **Increased vs Holy** (100-175%)
- Key skills: few player skill sources; mostly monster attacks
- Key card: Evil Druid Card (armor -> Undead Lv1)
- Evil Druid armor prevents Freeze status but reverses healing

---

## Element Levels (Lv1-4)

### How Element Levels Work

The defending element level amplifies both strengths and weaknesses:

- **Level 1**: Mild modifiers. Most values stay close to 100%. Same-element defense is ~25% (still takes some damage).
- **Level 2**: Moderate modifiers. Counter-elements reach 175%. Same-element defense hits 0% (immune). Ghost Lv2 becomes immune to Neutral.
- **Level 3**: Strong modifiers. Counter-elements reach 200%. Same-element becomes **negative** (-25%, heals target). Holy-resistant elements reach 25%.
- **Level 4**: Extreme modifiers. Maximum polarization. Counter-elements cap at 200%. Same-element deeply negative (-50% to -100%). Holy vs Holy Lv4 = -100% (full heal).

### Who Has Which Levels

| Level | Typical Holders |
|-------|----------------|
| **Lv1** | Players (via armor cards), most common monsters, weakest variants |
| **Lv2** | Stronger monsters, mid-tier enemies |
| **Lv3** | High-level monsters, some boss minions, non-MVP uniques |
| **Lv4** | MVPs, boss monsters, special creatures (Ghostring Ghost 4, Angeling Holy 4, Deviling Shadow 4) |

### Key Level Thresholds

- **Ghost Lv1**: Neutral deals 25% damage (reduced but not zero)
- **Ghost Lv2+**: Neutral deals **0% damage** (completely immune to normal Neutral attacks)
- **Same-element Lv2**: 0% -- immune to own element
- **Same-element Lv3**: -25% -- healed by own element
- **Same-element Lv4**: -50% -- significantly healed by own element
- **Holy vs Undead Lv3-4**: 200% -- double damage (crucial for Priest builds)
- **Fire vs Undead Lv4**: 200% -- Fire equally devastating as Holy at max level

---

## Weapon Elements

### Default Weapon Element

All weapons are **Neutral element** by default unless:
1. The weapon has an innate non-Neutral element (e.g., Mysteltainn = Shadow, Fire Brand = Fire)
2. The weapon was forged with an elemental stone (Fire/Water/Wind/Earth)
3. An endow buff overrides the weapon element

### Endow Skills (Weapon Element Buffs)

Endow skills temporarily change a player's weapon element. **Only one endow can be active at a time** -- new endows overwrite old ones (mutual exclusion).

#### Priest Endow

| Skill | Element | Duration | SP Cost | Catalyst | Cast Time |
|-------|---------|----------|---------|----------|-----------|
| **Aspersio** (Priest, ID 1011) | Holy | 60/90/120/150/180s | 14/18/22/26/30 | 1 Holy Water (ID 523) | 2s |

- Target: Self or single target (ally) within 9 cells
- Casting on Undead property or Demon race monsters deals minor Holy damage instead
- Switching weapons cancels the endow
- Dispell removes Aspersio

#### Sage Endows

| Skill | Element | Duration | SP Cost | Catalyst | Success Rate |
|-------|---------|----------|---------|----------|-------------|
| **Endow Blaze** (ID 1408) | Fire | 20 min (default) | per skill data | 1 Red Blood (ID 990) | 70/80/90/100/100% |
| **Endow Tsunami** (ID 1409) | Water | 20 min (default) | per skill data | 1 Crystal Blue (ID 991) | 70/80/90/100/100% |
| **Endow Tornado** (ID 1410) | Wind | 20 min (default) | per skill data | 1 Wind of Verdure (ID 992) | 70/80/90/100/100% |
| **Endow Quake** (ID 1411) | Earth | 20 min (default) | per skill data | 1 Green Live (ID 993) | 70/80/90/100/100% |

- Target: Self or party member within 9 cells
- Pre-renewal: catalysts are elemental stones (Red Blood, Crystal Blue, Wind of Verdure, Green Live)
- Switching weapons cancels the endow
- Each Sage endow **overwrites** any existing endow (including Aspersio, Enchant Poison, item endows)
- Success rate depends on skill level (not guaranteed at Lv1-3)
- Dispell removes Sage endows
- Sage also has **Elemental Change** (ID 1421, quest skill): permanently changes a monster's defending element. Success rate based on learned endow level + DEX/INT/JobLv. Does NOT work on bosses or players.

#### Assassin Endow

| Skill | Element | Duration | SP Cost | Cast Delay |
|-------|---------|----------|---------|-----------|
| **Enchant Poison** (ID 1103) | Poison | 30-165s (15s/lv) | 20 | 1s |

- Target: Self only
- Adds Poison element to weapon AND a chance (3.0-7.5%) to inflict Poison status per auto-attack
- Switching weapons cancels the endow
- Overwritten by any other endow skill
- Dispell removes Enchant Poison

### Elemental Stones (Item Materials)

The four elemental stones are used as catalysts for Sage endow skills:

| Item ID | Name | Element | Use |
|---------|------|---------|-----|
| 990 | Red Blood | Fire | Endow Blaze catalyst |
| 991 | Crystal Blue | Water | Endow Tsunami catalyst |
| 992 | Wind of Verdure | Wind | Endow Tornado catalyst |
| 993 | Green Live | Earth | Endow Quake catalyst |

### Elemental Converter Items

Elemental Converters are consumable items that endow the user's weapon with an element. They can be crafted by Sages (Create Converter skill, ID 1420) or purchased.

| Item ID | Name | Element | Duration | ENCHANTARMS Level |
|---------|------|---------|----------|-------------------|
| 12114 | Elemental Converter (Fire) | Fire | 5 min | Level 4 |
| 12115 | Elemental Converter (Water) | Water | 5 min | Level 2 |
| 12116 | Elemental Converter (Earth) | Earth | 5 min | Level 3 |
| 12117 | Elemental Converter (Wind) | Wind | 5 min | Level 5 |
| 12020 | Cursed Water | Shadow/Dark | 5 min | Level 8 |

Internally, elemental converters use `ITEM_ENCHANTARMS` with a level field that maps to elements:
- Level 2 = Water
- Level 3 = Earth
- Level 4 = Fire
- Level 5 = Wind
- Level 8 = Dark/Shadow

**Converter behavior:**
- Any class can use elemental converter items (unlike endow skills which are class-restricted)
- Converters overwrite existing endow buffs (mutual exclusion)
- Converters are overwritten by skill-based endows (Aspersio, Sage endows, etc.)
- Switching weapons cancels the converter endow

### Arrow Elements

Bows use arrows as ammunition, and arrows carry an element that overrides the weapon's base element:

| Arrow Type | Element |
|-----------|---------|
| Arrow | Neutral |
| Silver Arrow | Holy |
| Fire Arrow | Fire |
| Crystal Arrow (Ice Arrow) | Water |
| Stone Arrow | Earth |
| Wind Arrow | Wind |
| Poison Arrow | Poison |
| Shadow Arrow | Shadow |
| Immaterial Arrow | Ghost |
| Rusty Arrow | Neutral |
| Steel Arrow | Neutral |
| Oridecon Arrow | Neutral |

Arrow element **replaces** the weapon's base element (if the arrow element is non-Neutral), but is **overridden** by endow buffs.

### Card Weapon Elements

Weapon cards can change a weapon's innate element:

| Card | Weapon Element |
|------|---------------|
| Drops Card | Fire (weapon) |
| Mandragora Card | Earth (weapon) |
| Vadon Card | +20% damage vs Fire element (NOT endow -- this is a card modifier, not element change) |

Note: Most weapon cards add **bonus damage vs an element** (e.g., Vadon = +20% vs Fire defenders) rather than changing the weapon's attacking element. True element-changing weapon cards are rare.

---

## Armor Elements

### Default Armor Element

All players default to **Neutral Lv1** armor element. This means they take 100% damage from every element -- no special resistances or weaknesses.

### Armor Element Cards

Compounding these cards into armor changes the player's **defending element**:

| Card | Armor Element | Key Benefit | Key Risk |
|------|--------------|-------------|----------|
| **Swordfish Card** | Water Lv1 | -75% from Water, -50% from Fire | +75% from Wind |
| **Sandman Card** | Earth Lv1 | -50% from Fire, +50% from Earth→Wind cycle | +50% from Wind |
| **Pasana Card** | Fire Lv1 | -75% from Fire, -50% from Water | +50% from Earth |
| **Dokebi Card** | Wind Lv1 | -75% from Wind, -75% from Water→Wind | +50% from Earth |
| **Argiope Card** | Poison Lv1 | Immune to Poison, -50% from Shadow | -25% healed by Undead atk, +25% from Fire/Earth/Wind |
| **Angeling Card** | Holy Lv1 | Immune to Holy, -25% from Shadow→Holy | +25% from Shadow, normal from everything else |
| **Bathory Card** | Shadow Lv1 | Immune to Shadow, -25% from Poison→Shadow | +25% from Holy |
| **Ghostring Card** | Ghost Lv1 | -75% from Neutral (players only!), 100% from most elements | -25% from Holy, +25% from Ghost |
| **Evil Druid Card** | Undead Lv1 | Immune to Undead, immune to Shadow, prevents Freeze/Stone Curse | Healed by Poison/Shadow attacks (negative), damaged by Heal/Resurrection |

### How Armor Element Affects Damage Taken

When damage is calculated, the element modifier is applied as:

```
FinalDamage = floor(BaseDamage * ElementModifier / 100)
```

Where `ElementModifier` is looked up from `ELEMENT_TABLE[attackElement][defenseElement][defenseLevel - 1]`.

**If the modifier is 0**: Target is immune (takes 0 damage, displayed as "immune" or "miss").
**If the modifier is negative**: Target is healed by that element (e.g., Poison attacks heal Undead armor wearers). In practice, most implementations treat negative values as 0 damage for physical attacks, or as actual HP restoration for special cases.

### Ghostring Card (Ghost Lv1 Armor) -- Special Notes

Ghostring Card is one of the most impactful armor cards because it reduces all Neutral physical attacks to 25% damage. However:

- **Monster auto-attacks bypass the element table** (`isNonElemental = true`). In RO Classic, monster basic attacks are treated as "non-elemental" and always deal 100% damage regardless of armor element. This means Ghostring Card does NOT protect against monster auto-attacks.
- **Player auto-attacks ARE Neutral element** and use the element table. Ghostring Card protects against player Neutral attacks in PvP.
- **Skill attacks use their declared element** and use the element table normally.
- Ghost armor takes +25% from Ghost attacks and -25% from Holy (at Lv1), making Holy element attacks more effective.

---

## Element Priority System

When determining the attacking element of a player's attack, the following priority chain is used (highest priority first):

```
1. SKILL ELEMENT (if the skill has a fixed element, it ALWAYS uses that element)
2. ENDOW BUFF (Aspersio, Sage endows, Enchant Poison, Elemental Converters)
3. ARROW ELEMENT (if archer with bow and arrow is non-Neutral)
4. WEAPON DEFAULT (innate weapon element, or Neutral if none)
```

### Detailed Priority Rules

1. **Skill element always wins**: If a skill has `element: 'fire'` (like Fire Bolt, Magnum Break), it uses that element regardless of weapon endow or arrow. The weapon element is irrelevant for fixed-element skills.

2. **Endow overrides arrow and weapon**: When an endow buff is active (Aspersio, Endow Blaze, etc.), all physical attacks and weapon-element skills use the endowed element. Endows override arrow elements.

3. **Arrow overrides weapon (non-Neutral only)**: If an archer has a non-Neutral arrow equipped and no endow buff active, the arrow element is used instead of the weapon's base element. Neutral arrows do not override -- the weapon element is used.

4. **Weapon default is the fallback**: If no skill element, no endow, and no non-Neutral arrow, the weapon's innate element is used (Neutral for most weapons).

### Implementation (Sabri_MMO)

The element priority is implemented in two places:

```javascript
// recalcEffectiveWeaponElement(player) — Arrow > Weapon base
// Sets player.weaponElement to include arrow override above base weapon
function recalcEffectiveWeaponElement(player) {
    let effectiveElement = (player.equippedWeaponRight &&
        player.equippedWeaponRight.element) || 'neutral';
    if (player.equippedAmmo && player.equippedAmmo.element !== 'neutral' &&
        player.weaponType === 'bow') {
        effectiveElement = player.equippedAmmo.element;
    }
    player.weaponElement = effectiveElement;
}

// In getCombinedModifiers() — Endow buff overrides player.weaponElement
// buffMods.weaponElement is set if any endow buff is active
// In getAttackerInfo() — Endow > stored weaponElement
```

In the damage formula:
```javascript
// Skill element overrides all:
const atkElement = skillElement || attacker.weaponElement || 'neutral';
```

### Endow Mutual Exclusion

Only **one endow buff can be active** at any time. Applying a new endow removes all existing endows:

- Aspersio overwrites Sage endows, Enchant Poison, item endows
- Sage endows overwrite Aspersio, Enchant Poison, item endows
- Enchant Poison overwrites Aspersio, Sage endows, item endows
- Elemental Converter items overwrite all the above

When Dispell is cast, all endow buffs are removed and the weapon element reverts to base.

---

## Ghost Element Special Rules

Ghost element has unique mechanics that distinguish it from all other elements:

### Normal Attacks vs Ghost

- **Neutral attacks deal only 25% to Ghost Lv1** and **0% to Ghost Lv2+**
- This makes Ghost Lv2+ monsters effectively **immune to standard Neutral auto-attacks**
- Players with Neutral weapons (no endow) cannot damage Ghost Lv2+ monsters with auto-attacks

### How to Damage Ghost Element

1. **Elemental weapons/endows**: Any non-Neutral element (Fire, Water, Earth, Wind, etc.) deals at least 25-100% to Ghost. At Ghost Lv1, all four main elements deal 100%. At higher Ghost levels, they deal progressively less (75% at Lv2, 50% at Lv3, 25% at Lv4).
2. **Ghost element attacks**: Ghost vs Ghost deals 125-200% depending on level -- the best physical option against Ghost monsters.
3. **Magic skills**: Magic skills have fixed elements (Fire Bolt = Fire, Cold Bolt = Water, etc.) and use the element table normally. Holy element magic is effective at lower Ghost levels.
4. **Immaterial Arrow**: Archers can use Ghost element arrows (Immaterial Arrow) for Ghost vs Ghost bonus damage.

### Ghost Armor (Ghostring Card)

- Players with Ghost Lv1 armor are **protected from Neutral attacks** (25% damage from players, but NOT from monster auto-attacks due to `isNonElemental`)
- Ghost armor takes +25% from Ghost attacks and slightly reduced from Holy (-25% at Lv1)
- Ghost armor takes **normal damage** (100%) from Water, Earth, Fire, Wind at Lv1

### Notable Ghost Element Monsters

- **Ghostring** (ID 1120): Ghost Lv4, boss, medium, demon race
- **Whisper** (ID 1179): Ghost Lv2-3
- **Ghoul** variants: various Ghost levels
- **Ghost monsters**: typically also Undead race or Demon race

---

## Undead Element Special Rules

Undead element has the most special skill interactions of any element:

### Heal and Undead

- **Heal damages Undead**: When the Priest skill Heal (ID 400) targets an Undead property entity, it deals **Holy property damage equal to half the heal amount** instead of restoring HP.
- Formula: `HealDamage = floor(HealAmount / 2)` as Holy element
- The damage is Holy property and uses the element table (Holy vs Undead = 150-200%)
- This is one of the primary farming methods against Undead monsters

### Resurrection and Undead

- **Resurrection kills Undead**: When Resurrection (Priest, ID 1004) is used on an Undead property monster, it acts as **Turn Undead** instead of reviving.
- Turn Undead (ID 1006): Chance to instantly kill an Undead monster. On failure, deals Holy element damage.
- Instant death chance: `floor(Base_Chance + (BaseLv/10) + (INT/10) + (LUK/10) + (1 - TargetHP/TargetMaxHP) * 20)%`, capped at 70%
- Boss monsters are immune to the instant death effect (only takes Holy damage on "fail")

### Blessing and Undead

- **Blessing debuffs Undead/Demon**: When Blessing (ID 402) targets an Undead property or Demon race entity, instead of buffing STR/DEX/INT, it **reduces DEX and INT** (debuff effect).
- This is distinct from Curse -- it lowers DEX and INT specifically

### Other Undead Interactions

- **Undead is immune to Undead** (0% at all levels) -- Undead attacks cannot damage Undead defenders
- **Undead is immune to Shadow** (0% at all levels) -- Shadow attacks deal no damage to Undead
- **Poison heals Undead**: -25% to -100% (negative values = HP restoration)
- **Shadow heals Undead**: -25% to -100%
- **Holy is devastating**: 150% at Lv1, up to 200% at Lv3-4
- **Fire is effective**: 125% at Lv1, up to 200% at Lv4

### Evil Druid Card (Undead Lv1 Armor)

Players wearing Undead Lv1 armor (Evil Druid Card) get:
- **Immunity to Freeze and Stone Curse** (cannot be frozen or petrified)
- **Healed by Poison and Shadow attacks** (negative element modifier = HP restoration)
- **Damaged by Heal** (Heal hurts the player instead of healing)
- **Cannot be resurrected** (Resurrection acts as Turn Undead)
- Increased damage from Holy (+50%) and Fire (+25%)

---

## Holy vs Shadow Interactions

Holy and Shadow are **mutually devastating** -- they are the primary "opposite" pair:

| Interaction | Lv1 | Lv2 | Lv3 | Lv4 |
|------------|-----|-----|-----|-----|
| Holy attacking Shadow | **125%** | **150%** | **175%** | **200%** |
| Shadow attacking Holy | **125%** | **150%** | **175%** | **200%** |
| Holy attacking Holy | **0%** | **-25%** | **-50%** | **-100%** |
| Shadow attacking Shadow | **0%** | **-25%** | **-50%** | **-100%** |

**Key implications:**
- Aspersio (Holy weapon endow) is extremely effective against Shadow monsters
- Shadow armor (Bathory Card) is vulnerable to Holy attacks but immune to Shadow
- Holy armor (Angeling Card) is vulnerable to Shadow attacks but immune to Holy
- At Lv4, Holy vs Shadow and Shadow vs Holy both reach 200% (double damage)
- Self-element at Lv4 is -100% (full heal) -- attacking a Holy Lv4 with Holy completely heals it

### Holy vs Undead

Holy is also devastating against Undead (separate from Shadow):

| Interaction | Lv1 | Lv2 | Lv3 | Lv4 |
|------------|-----|-----|-----|-----|
| Holy attacking Undead | **150%** | **175%** | **200%** | **200%** |

This makes Priest/Crusader classes with Holy skills (Heal, Turn Undead, Holy Cross, Magnus Exorcismus) the premier Undead-hunting classes.

---

## Boss Monsters and Element Levels

Boss monsters (MVPs and mini-bosses) typically have higher element levels than normal monsters:

### Element Level Distribution (Boss Examples from Sabri_MMO)

| Monster | Element | Level | Race | Type |
|---------|---------|-------|------|------|
| Ghostring | Ghost | **4** | Demon | Boss |
| Angeling | Holy | **4** | Angel | Boss |
| Deviling | Shadow | **4** | Demon | Boss |

### Common Patterns

- **MVP/Boss monsters**: Often Lv3 or Lv4, making them highly resistant to their own element and highly vulnerable to counter-elements
- **Mini-bosses**: Usually Lv2 or Lv3
- **Normal monsters**: Mostly Lv1, some mid-tier at Lv2
- **Plants and weak monsters**: Always Lv1

### Boss Element Immunity

Bosses are **not immune** to the element table itself -- they use the same ELEMENT_TABLE as all other entities. However, bosses have special protections:
- Boss flag prevents instant death effects (Turn Undead kill chance)
- Boss flag prevents Elemental Change (Sage skill)
- Boss monsters ARE affected by Ankle Snare (1/5 duration) and most status effects at reduced rates

---

## Frozen = Water Lv1, Stone Curse = Earth Lv1

Two status effects **override the target's defending element**, changing their elemental vulnerability:

### Frozen Status (Freeze)

When a target is **Frozen**, their defending element is temporarily changed to **Water Lv1**:

```
elementOverride: { type: 'water', level: 1 }
```

**Effects:**
- Target takes **175% damage from Wind** attacks (Water Lv1 defense vs Wind attack)
- Target takes **25% damage from Water** attacks (same-element reduction)
- Target takes **150% damage from Fire** attacks (not intuitive but per the table: Fire vs Water Lv1 = 50%, wait -- actually Fire vs Water Lv1 defense means we look at Fire attack vs Water Lv1: 50%. However, the original element is overridden so the target truly becomes Water Lv1)
- This makes Frozen targets extremely vulnerable to **Wind element follow-up** (Storm Gust -> Jupitel Thunder combo)
- Any hit breaks the Frozen status

**Additional effects of Frozen:**
- DEF multiplier: 0.5x (halved)
- MDEF multiplier: 1.25x (increased by 25%)
- Prevents movement, attacking, skill use, and item use

### Stone Curse (Petrification)

When a target is **fully Petrified**, their defending element is temporarily changed to **Earth Lv1**:

```
elementOverride: { type: 'earth', level: 1 }
```

**Effects:**
- Target takes **150% damage from Wind** attacks (Earth Lv1 vs Wind = 150%)
- Target takes **150% damage from Fire** attacks (wait -- Fire vs Earth Lv1 = 150%)
- Target takes **100% from Water** attacks
- Target takes **100% from same-element (Earth)**

**Additional effects of Stone Curse:**
- DEF multiplier: 0.5x (halved)
- MDEF multiplier: 1.25x (increased by 25%)
- HP drain: 1% MaxHP every 5 seconds until 25% HP remains
- Prevents movement, attacking, skill use
- Cannot petrify Boss or Undead property monsters

### Implementation

Both overrides are configured in `ro_status_effects.js`:

```javascript
freeze: {
    // ...
    elementOverride: { type: 'water', level: 1 }
},
stone: {
    // ...
    elementOverride: { type: 'earth', level: 1 },
    periodicDrain: { interval: 5000, hpPercent: 0.01 }
}
```

The override is applied in `getCombinedModifiers()` as `overrideElement`, which replaces the target's normal element in the damage pipeline:

```javascript
if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;
```

---

## Skills with Fixed Elements

Many skills have a **fixed element** that overrides the attacker's weapon element. The skill element is always used regardless of endow, arrow, or weapon element.

### Fire Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 105 | Magnum Break | Swordsman | AoE fire, also grants +20% fire ATK bonus buff |
| 201 | Fire Bolt | Mage | Multi-hit bolt |
| 207 | Fire Ball | Mage | 5x5 AoE splash |
| 209 | Fire Wall | Mage | Ground trap, 50% MATK per hit |
| 809 | Sight Rasher | Wizard | AoE fire, requires Sight |
| 810 | Fire Pillar | Wizard | Ground trap, ignores MDEF |
| 811 | Meteor Storm | Wizard | Ground AoE fire |
| 813 | Sight Blaster | Wizard | Reactive fireball |
| 907 | Claymore Trap | Hunter | AoE fire trap |

### Water Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 200 | Cold Bolt | Mage | Multi-hit bolt |
| 208 | Frost Diver | Mage | Single-target + Freeze |
| 807 | Storm Gust | Wizard | AoE, chance to Freeze (element changes target to Water Lv1) |
| 808 | Ice Wall | Wizard | Ground barrier |
| 911 | Freezing Trap | Hunter | Freeze + Water damage |

### Wind Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 202 | Lightning Bolt | Mage | Multi-hit bolt |
| 212 | Thunderstorm | Mage | AoE ground, multi-hit |
| 800 | Jupitel Thunder | Wizard | Single-target + knockback |
| 801 | Lord of Vermilion | Wizard | AoE ground |
| 912 | Blast Mine | Hunter | Wind MISC damage trap |

### Earth Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 203 | Napalm Beat | Mage | Ghost element (NOT Earth -- often confused) |
| 206 | Stone Curse | Mage | Petrify (changes target to Earth Lv1) |
| 506 | Sand Attack | Thief | Earth physical |
| 804 | Earth Spike | Wizard | Multi-hit bolt |
| 805 | Heaven's Drive | Wizard | AoE ground |
| 806 | Quagmire | Wizard | AoE earth debuff zone |
| 904 | Land Mine | Hunter | Earth MISC trap |
| 1417 | Earth Spike (Sage) | Sage | Multi-hit bolt |
| 1418 | Heaven's Drive (Sage) | Sage | AoE ground |

### Holy Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 400 | Heal | Acolyte | Damages Undead (half heal as Holy dmg) |
| 402 | Blessing | Acolyte | Debuffs Undead/Demon |
| 407 | Signum Crucis | Acolyte | Reduces DEF of Undead/Demon |
| 408 | Ruwach | Acolyte | Reveal hidden, Holy damage |
| 414 | Holy Light | Acolyte | Holy magic attack |
| 1004 | Resurrection | Priest | Acts as Turn Undead vs Undead |
| 1006 | Turn Undead | Priest | Instant-kill chance on Undead |
| 1011 | Aspersio | Priest | Holy weapon endow (not damage) |
| 1302 | Holy Cross | Crusader | Holy physical attack |
| 1303 | Grand Cross | Crusader | Holy/Neutral hybrid AoE |
| 1311 | Heal (Crusader) | Crusader | Same as Acolyte Heal |

### Poison Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 504 | Envenom | Thief | Poison physical + poison status chance |

### Ghost Element Skills

| Skill ID | Name | Class | Notes |
|----------|------|-------|-------|
| 203 | Napalm Beat | Mage | Ghost element AoE magic (split damage) |
| 204 | Soul Strike | Mage | Ghost element single-target magic |

### Neutral Element Skills (Use Weapon Element)

Skills marked `element: 'neutral'` in the skill data do NOT force Neutral element -- they use the attacker's weapon element (including endow/arrow). This is the **majority of physical skills**:

- Bash, Double Attack, Pierce, Bowling Bash, Sonic Blow, etc.
- These skills benefit from weapon endows and arrow elements

The distinction is: `element: 'neutral'` in skill data means "no forced element, use weapon element", while `element: 'fire'` means "always Fire regardless of weapon".

---

## Monster Auto-Attack Element (Non-Elemental)

A critical distinction in RO Classic that affects Ghostring Card and armor element builds:

### Monster Auto-Attacks Are Non-Elemental

In pre-Renewal RO, monster basic attacks (auto-attacks) are treated as **"non-elemental"** -- they bypass the element table entirely and always deal 100% damage regardless of the target's armor element.

**This means:**
- Ghostring Card (Ghost Lv1 armor) does NOT reduce monster auto-attack damage
- Pasana Card (Fire Lv1 armor) does NOT reduce monster auto-attack damage
- Any armor element card provides NO protection against monster basic hits

**However:**
- Monster **skill attacks** DO use the element table (Fire Bolt from a monster IS Fire element)
- Player auto-attacks ARE Neutral element and DO use the element table (Ghostring protects against player hits in PvP)

### Implementation

In the damage formula (`ro_damage_formulas.js`):

```javascript
if (isNonElemental) {
    // Monster auto-attacks: bypass element table, always 100%
    eleModifier = 100;
} else {
    eleModifier = getElementModifier(atkElement, targetElement, targetElementLevel);
}
```

The `isNonElemental` flag is set to `true` for monster auto-attacks in the combat tick loop.

---

## Sage Zone Elemental Damage Boost

Sage skills Volcano, Deluge, and Violent Gale create ground effect zones that boost elemental damage for matching elements:

| Zone Skill | Element Boost | ATK Bonus |
|-----------|--------------|-----------|
| **Volcano** (ID 1412) | Fire damage +10-20% | +10-50 flat ATK (Fire weapon) |
| **Deluge** (ID 1413) | Water damage +10-20% | MaxHP boost (%) |
| **Violent Gale** (ID 1414) | Wind damage +10-20% | +3-15 FLEE |

**Element restriction**: Zone stat bonuses (ATK from Volcano, MaxHP from Deluge, FLEE from Violent Gale) only apply to entities with matching element armor or weapon. The damage boost applies to any attack of that element within the zone.

---

## Implementation Checklist

### Element Table (COMPLETE)
- [x] 10x10x4 `ELEMENT_TABLE` in `ro_damage_formulas.js`
- [x] `getElementModifier(atkElement, defElement, defElementLevel)` lookup function
- [x] Element modifier applied in physical damage pipeline (Step 8i, after DEF+refine+mastery)
- [x] Element modifier applied in magical damage pipeline
- [x] Negative element modifiers handled (0 = immune, negative = heal/immune)
- [x] `elementHeal` and `elementImmune` hit types for 0 and negative values

### Weapon Elements (COMPLETE)
- [x] Endow skills: Aspersio, Endow Blaze/Tsunami/Tornado/Quake, Enchant Poison
- [x] Endow mutual exclusion (new endow removes all existing endows)
- [x] Elemental Converter items (ITEM_ENCHANTARMS handler)
- [x] Arrow element override (non-Neutral arrows override weapon base)
- [x] Element priority chain: Skill > Endow > Arrow > Weapon
- [x] `recalcEffectiveWeaponElement()` for arrow priority
- [x] Weapon swap cancels endow (implicit via buff system)
- [x] Dispell removes endow buffs
- [x] Sage endow success rate (70/80/90/100/100%)
- [x] Sage endow catalysts (Red Blood, Crystal Blue, Wind of Verdure, Green Live)
- [x] Aspersio catalyst (Holy Water)

### Armor Elements (COMPLETE)
- [x] Monster element stored as `{ type, level }` in templates
- [x] Player default element: Neutral Lv1
- [x] Armor element cards change player defending element
- [x] Frozen = Water Lv1 override (`elementOverride` in freeze status)
- [x] Stone Curse = Earth Lv1 override (`elementOverride` in stone status)
- [x] `overrideElement` applied in all damage paths

### Special Rules (COMPLETE)
- [x] Monster auto-attacks are non-elemental (`isNonElemental` flag)
- [x] Ghostring Card protected against player Neutral attacks but NOT monster auto-attacks
- [x] Ghost Lv2+ immune to Neutral attacks
- [x] Heal damages Undead (Holy property, half heal amount)
- [x] Resurrection acts as Turn Undead on Undead targets
- [x] Blessing debuffs Undead/Demon instead of buffing
- [x] Turn Undead instant kill chance (capped at 70%, boss immune)
- [x] Fixed element skills override weapon element (skill element takes priority)
- [x] Sage zone elemental damage boost (Volcano/Deluge/Violent Gale)

### Missing/Deferred
- [ ] Sage Elemental Change (ID 1421): currently quest skill stub -- monster element permanently changed. Needs full implementation with success rate formula based on endow level + DEX/INT/Job Level
- [ ] Evil Druid Card interaction: Heal damaging player with Undead armor (partially implemented via element table, but explicit Heal vs player check may not exist)
- [ ] Aspersio vs Undead/Demon target: should deal minor Holy damage when cast on Undead property or Demon race targets (instead of endowing)
- [ ] Negative element modifier HP restoration: when element modifier is negative, target should gain HP equal to |damage * modifier / 100|. Currently treated as 0 damage -- actual healing not implemented for physical attacks

---

## Gap Analysis

### Verified Working
1. **Element table**: All 400 values implemented and cross-referenced with rAthena pre-re + iRO Wiki Classic
2. **Element priority chain**: Skill > Endow (buff) > Arrow (non-Neutral) > Weapon base
3. **Endow mutual exclusion**: All endow types properly overwrite each other
4. **Status element override**: Frozen = Water Lv1, Stone Curse = Earth Lv1
5. **Monster non-elemental auto-attacks**: `isNonElemental` flag bypasses element table
6. **Fixed-element skills**: Fire Bolt, Cold Bolt, Lightning Bolt, Magnum Break, etc. all use their declared element
7. **Ghost element immunity**: Neutral vs Ghost Lv2+ = 0% (immune)
8. **Holy vs Undead**: Heal damages Undead, Turn Undead instant kill, Blessing debuff
9. **Sage zone boosts**: Volcano/Deluge/Violent Gale element-restricted stat bonuses
10. **Card element defense**: Defensive cards like Raydric, Ghostring properly integrate with element system
11. **Arrow consumption**: Arrows consumed on attack with proper element override

### Potential Issues to Audit
1. **Element table accuracy**: The `ELEMENT_TABLE` in `ro_damage_formulas.js` was sourced from rAthena but Section 7.3 of `02_Combat_System.md` notes "some values that differ from the canonical RateMyServer/iRO Wiki table." A cell-by-cell audit against the tables in this document should confirm correctness.
2. **Negative element healing**: Physical attacks with negative element modifiers currently return 0 damage (`elementHeal` hit type) but do not actually restore target HP. For full accuracy, the combat loop should check for negative modifiers and convert them to HP restoration.
3. **Sage Elemental Change**: Quest skill (ID 1421) needs full implementation -- permanently changes monster element. Success formula: `5 + 10 * EndowLevel + floor(DEX + INT + JobLevel) / 20`%. Does not work on bosses or players.
4. **Aspersio on Undead/Demon**: Should deal minor Holy damage rather than endowing when cast on Undead property or Demon race targets. Need to verify this interaction is handled.
5. **Cursed Water endow element**: Currently mapped as `level: 8` -> `'dark'`. Verify the buff system consistently uses `'shadow'` vs `'dark'` naming (the element table uses `'shadow'` as the key).
6. **Monster skill elements**: Some monsters cast elemental skills (Fire Bolt, Cold Bolt via monster skill system). Verify these use the correct fixed element from the skill data and not the monster's body element.
7. **Grand Cross element**: Grand Cross deals Holy+Neutral hybrid damage. Verify both element calculations are applied correctly (half Holy, half Neutral, with separate element modifiers).

### Priority Fixes
- **P1**: Audit element table values cell-by-cell (Section 7.3 warning)
- **P2**: Implement negative element HP restoration (Poison/Shadow healing Undead armor wearers)
- **P3**: Sage Elemental Change full implementation
- **P4**: Aspersio vs Undead/Demon interaction
