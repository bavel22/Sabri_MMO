# RO Pre-Renewal Advanced Combat Mechanics Research

Deep research on combat mechanics beyond standard damage formulas.
Sources: rAthena source code, iROWiki, RateMyServer, WarpPortal Forums, Ragnarok Research Lab.

---

## 1. Walk Delay / Hit Stun (bNoWalkDelay — Eddga Card)

### What is Walk Delay?

Walk delay (also called "hit stun" or "flinch") is the brief period after taking damage where a character **cannot move**. The character is locked in place for the duration of their **dMotion** (damage motion) animation. They still take full damage — they just cannot walk.

### Duration & Calculation

- **Base duration = target's dMotion value** (damage motion animation time in ms)
- Player dMotion is determined by the client sprite data (typically ~230ms for most classes)
- Monster dMotion varies per monster (defined in mob_db, e.g., Poring = 480ms, Baphomet = 576ms)
- rAthena config `pc_damage_walk_delay_rate: 20` — player walk delay = 20% of dMotion (effectively ~46ms for players)
- rAthena config `damage_walk_delay_rate: 100` — monster walk delay = 100% of dMotion
- Multi-hit skills add: `(number_of_hits - 1) * 200ms` additional delay (official: 200ms per extra hit)
- `default_walk_delay: 300` — minimum walk delay fallback (300ms)

### What Walk Delay Affects

- **Movement only** — you cannot walk/move while flinching
- **Does NOT block skills** — you can still cast skills and use items during walk delay
- **Does NOT block attacking** — you can still auto-attack
- Essentially: you are "rooted in place" but can still act

### What Cancels Walk Delay

- **Endure skill** — prevents walk delay for its duration (7 + 3*SkillLv seconds), limited to 7 monster hits, unlimited player hits
- **bNoWalkDelay bonus** — permanent/infinite Endure (Eddga Card)
- **Boss monsters** — immune to walk delay in renewal (configurable in pre-renewal)
- Certain movement skills (SC_RUN, SC_WUGDASH) also grant implicit Endure

### Endure Skill Details (Pre-Renewal)

| Level | Duration | MDEF Bonus | SP Cost |
|-------|----------|------------|---------|
| 1     | 10s      | +1         | 10      |
| 2     | 13s      | +2         | 10      |
| 3     | 16s      | +3         | 10      |
| 4     | 19s      | +4         | 10      |
| 5     | 22s      | +5         | 10      |
| 6     | 25s      | +6         | 10      |
| 7     | 28s      | +7         | 10      |
| 8     | 31s      | +8         | 10      |
| 9     | 34s      | +9         | 10      |
| 10    | 37s      | +10        | 10      |

- Hit limit: **7 monster hits** (then Endure wears off). NO limit for player hits.
- Cooldown: 10 seconds
- Prerequisite: Provoke Lv 5

### Eddga Card (bNoWalkDelay)

- **Slot**: Footgear
- **Effect**: Permanent Endure (no hit limit, no duration limit)
- **Penalty**: MaxHP -25%
- **rAthena script**: `bonus bNoWalkDelay; bonus bMaxHPrate,-25;`

### Implementation Notes for Server

```
bNoWalkDelay = permanent Endure = character never has walk delay when hit

When a character takes damage:
  1. Play dMotion animation (cosmetic — still plays even with Endure)
  2. IF NOT hasEndure AND NOT hasNoWalkDelay:
     walkDelay = dMotion * (pc_damage_walk_delay_rate / 100)
     Block movement for walkDelay ms
  3. IF hasEndure:
     Decrement hit counter (for Endure skill, not bNoWalkDelay)
     Do NOT block movement
```

---

## 2. Knockback System (bNoKnockback — RSX-0806, bAddSkillBlow)

### Which Skills Cause Knockback (Pre-Renewal)

| Skill | Cells | Direction | Notes |
|-------|-------|-----------|-------|
| **Arrow Repel** | 6 | Away from caster | Archer quest skill |
| **Arrow Shower** | 2 | Away from caster | AoE |
| **Magnum Break** | 2 | Away from caster | 5x5 AoE around self |
| **Bowling Bash** | 5 | Direction caster faces | Gutter line removed |
| **Brandish Spear** | 3 | Away from caster | Frontal AoE |
| **Cart Revolution** | 2 | Away from caster | AoE |
| **Spear Stab** | 6 | Away from caster | Linear AoE |
| **Jupitel Thunder** | Level-dependent | Away from caster | See table below |
| **Fire Wall** | 1 | Away from wall | Per hit (up to 14 hits) |
| **Storm Gust** | 2 | Away from center | AoE, also freezes |
| **Magnum Break (old)** | 2 | Away from caster | — |
| **Sight Blaster** | 3 | Away from caster | 3x3 trigger |
| **Sightrasher** | 5 | Away from caster | — |
| **Phantasmic Arrow** | 2 | Away from caster | Ranged |
| **Axe Boomerang** | Variable | Toward caster | Pulls target in |
| **Back Slide** | 5 | Behind self | Self-knockback |
| **Smite** | Variable | Away from caster | Shield skill |
| **Sanctuary** | 1 | Away from center | Undead only |
| **Self-Destruction** | Variable | Away from exploder | Monster skill |
| **Skid Trap** | Variable | Away from trap | Hunter trap |
| **Flip Tatami** | 2 | Away from caster | Ninja |
| **Triangle Shot** | 3 | Away from caster | — |

### Jupitel Thunder Knockback Per Level

| Level | Hits | Knockback (cells) |
|-------|------|--------------------|
| 1     | 3    | 2                  |
| 2     | 4    | 3                  |
| 3     | 5    | 2                  |
| 4     | 6    | 4                  |
| 5     | 7    | 2                  |
| 6     | 8    | 5                  |
| 7     | 9    | 2                  |
| 8     | 10   | 6                  |
| 9     | 11   | 2                  |
| 10    | 12   | 7                  |

### Knockback Direction Calculation

- Direction is calculated from **attacker position to target position** (8 directional — N, NE, E, SE, S, SW, W, NW)
- Uses lookup tables: `dirx[8] = {0,-1,-1,-1,0,1,1,1}`, `diry[8] = {1,1,0,-1,-1,-1,0,1}`
- Target is pushed **away** from the attacker/source in the computed direction
- Special case: If target is on same cell as caster, knockback defaults to **West**
- **Bowling Bash exception**: knockback direction = **direction the caster is facing** (not relative to target)

### Can Knockback Push Through Walls?

- **No** — knockback stops at unwalkable cells (walls, obstacles)
- rAthena config `path_blown_halt: 1` — unit stops immediately at wall (official behavior)
- If `path_blown_halt: 0` — old Athena behavior where diagonal knockback could "slide" along walls
- `path_blownpos()` function checks walkability of each cell in the knockback path

### What Prevents Knockback?

| Source | Effect |
|--------|--------|
| **RSX-0806 Card** (Armor) | `bNoKnockback` — complete knockback immunity |
| **GTB (Golden Thief Bug) Card** | Does NOT prevent knockback (only blocks magic) |
| **Boss protocol monsters** | Immune to knockback (flag-based) |
| **Certain skills** | Some skills have NoKnockback flag in skill_db |
| **Weight** | Does NOT affect knockback |

### RSX-0806 Card

- **Slot**: Armor
- **Effect**: VIT +3, immune to knockback
- **rAthena script**: `bonus bVit,3; bonus bNoKnockback;`

### bAddSkillBlow

- **Syntax**: `bonus2 bAddSkillBlow,sk,n;` — adds n cells knockback when using skill sk
- Used to give skills knockback that normally don't have it, or increase existing knockback
- Example: Could add knockback to Bash (which normally has none)

---

## 3. Splash / AoE Auto-Attack (bSplashRange — Baphomet Card)

### How Baphomet Card Splash Works

- **rAthena bonus**: `bonus bSplashRange,1;` — adds splash range 1 to normal attacks
- Range 1 = **3x3 area** around the PRIMARY TARGET (not around the attacker)
- Range 2 = 5x5 area, Range 3 = 7x7 area, etc.
- Only the **highest** bSplashRange among all equipped items applies (they do NOT stack)
- `bSplashAddRange` is the additive version (stacks)

### Baphomet Card Stats

- **Slot**: Weapon
- **Effect**: Splash Range 1 (3x3 area attacks), HIT -10
- **rAthena script**: `bonus bSplashRange,1; bonus bHit,-10;`

### Splash Damage Calculation

- Splash damage is treated as a **separate skill** (not a normal attack) by the engine
- Each secondary target gets an **independent damage calculation**:
  - Independent HIT check against each target's FLEE (splash CAN miss)
  - Independent Critical check per target (main target crit does NOT guarantee splash crit)
  - Full ATK is calculated independently for each target (100% damage, not reduced)
- Damage = **100% of attack power** (same base as the main hit, but recalculated)

### Card Effects on Splash Targets

- **Splash does NOT trigger attack-proc card effects** on secondary targets
  - Since splash is internally treated as a "skill," and skills don't proc other skills
  - Status-applying cards (like Coma, Stun, Freeze from autocast cards) do NOT apply to splash targets
- **Racial/elemental/size damage cards DO apply** (they modify base ATK before splash calculation)
- **Double Attack does NOT proc** on splash targets (rAthena issue #7611 confirms this is correct behavior)
- **Critical hits are independent** per target — each splash target rolls its own crit chance

### Skills vs Auto-Attacks

- Splash from bSplashRange applies to **normal auto-attacks only**
- Skills that already have their own AoE (like Bowling Bash) are NOT affected by bSplashRange
- Skills use their own built-in splash/AoE mechanics

### Important Behaviors

- Splash damage requires HIT to land (unlike the primary attack which might have other modifiers)
- The Baphomet Card's HIT -10 penalty makes splash targets harder to hit
- Splash targets must be within line of sight of the primary target (controlled by `map_foreachinshootrange`)

---

## 4. DEF Ignore (bIgnoreDefClass — Samurai Specter Card)

### What Is Ignored — BOTH Hard DEF and Soft DEF

In pre-renewal rAthena, `bIgnoreDefClass` **ignores BOTH hard DEF and soft DEF**:

```
Pre-renewal damage with DEF: ATK * [(4000+HardDEF) / (4000+HardDEF*10)] - SoftDEF
With bIgnoreDefClass:        ATK (no DEF reduction applied at all)
```

- **Hard DEF** (equipment DEF) — multiplicative reduction, IGNORED
- **Soft DEF** (VIT-based DEF) — subtractive reduction, IGNORED
- Result: damage hits as if target has 0 DEF/0 VIT DEF

### Skills vs Auto-Attacks

- Works on **both normal attacks AND skills** (any physical attack)
- The flag is checked during `battle_calc_weapon_attack()` which handles all physical damage

### Class Types

- `Class_Normal` = Normal monsters (and players in PvP contexts)
- `Class_Boss` = Boss/MVP monsters
- `Class_All` = Both (used by some renewal items)

### Samurai Specter Card

- **Slot**: Weapon
- **Effect**: Ignore DEF of Normal-class targets, HP recovery disabled, drain 666 HP every 10s, drain 999 HP on unequip
- **rAthena script**: `bonus bIgnoreDefClass,Class_Normal; bonus bNoRegen,1; bonus2 bHPLossRate,666,10000; bonus bUnbreakableWeapon,0;`
- **Key limitation**: Only works against `Class_Normal` — does NOT work against Boss monsters

### Other DEF-Ignoring Effects

| Source | Ignores | Target |
|--------|---------|--------|
| Samurai Specter Card | Both DEF | Class_Normal only |
| Ice Pick (weapon) | Uses DEF-to-ATK (see section 5) | Class_Normal |
| Occult Impaction (skill) | Both DEF | All targets |
| Acid Demonstration | Both DEF | All targets |
| Thanatos Card | Uses DEF-to-ATK (see section 5) | Class_All |

---

## 5. DEF-to-Damage (bDefRatioAtkClass — Thanatos Card / Ice Pick)

### How bDefRatioAtkClass Works (Pre-Renewal)

This is the "piercing" mechanic — instead of DEF reducing damage, DEF INCREASES damage.

**Pre-renewal formula (from rAthena source + Playtester confirmation):**

```
Normal damage:  ATK * [(100 - HardDEF) / 100] - SoftDEF
Piercing damage: ATK * [(HardDEF + SoftDEF) / 100]
```

Where:
- `HardDEF` = equipment DEF (capped at 100 in pre-renewal for the percentage formula)
- `SoftDEF` = VIT-based DEF = floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)

### Critical Nuance — When Piercing REDUCES Damage

There is a well-documented behavior where **piercing can actually deal LESS damage than normal attacks** against low-DEF targets:

```
Example: Target with HardDEF=25, SoftDEF=10
Normal:   ATK * (100-25)/100 - 10 = ATK * 0.75 - 10
Piercing: ATK * (25+10)/100       = ATK * 0.35

If ATK = 1000:
Normal:   1000 * 0.75 - 10 = 740
Piercing: 1000 * 0.35      = 350  (LESS damage!)
```

Piercing becomes effective only when `(HardDEF + SoftDEF) > (100 - HardDEF)`, i.e., when combined DEF exceeds ~50+. The breakpoint is roughly **150+ combined DEF** for the card to outperform normal attacks.

### Memory of Thanatos Card

- **Slot**: Weapon
- **Effect**: Piercing damage (bDefRatioAtkClass) against ALL classes, DEF -30, FLEE -30, drain 1 SP per attack
- **rAthena script**: `bonus bDefRatioAtkClass,Class_All; bonus bDef,-30; bonus bFlee2,-30; bonus2 bSPDrainValue,-1,0;`

### Ice Pick (Weapon)

- **Weapon type**: One-Handed Dagger (ATK 70, Weight 60, Weapon Level 4)
- **Effect**: Same piercing mechanic as Thanatos card — `bonus bDefRatioAtkClass,Class_Normal;`
- **Key difference from Thanatos**: Only works on Class_Normal (not boss monsters)

### Occult Impaction (Investigate) Comparison

The Monk skill Occult Impaction uses a DIFFERENT formula — it's not bDefRatioAtkClass:

```
Pre-renewal Occult Impaction formula:
Damage = ATK * SkillMultiplier * (EnemyHardDEF + EnemySoftDEF) / 100
```

Where SkillMultiplier = varies by level. Normal DEF reduction is completely skipped.
If enemy total DEF = 0, the skill misses entirely.

---

## 6. Damage Reflection (bShortWeaponDamageReturn, bMagicDamageReturn)

### Physical Melee Reflection (bShortWeaponDamageReturn)

**How it works:**
```
reflectedDamage = incomingDamage * reflectPercent / 100
```

**Key mechanics:**
- Reflected damage is a **special damage type** — neither physical nor magical (since Dec 2021 iRO patch)
- Reflected damage is **NOT reduced by the attacker's DEF** — it ignores all defense
- Reflected damage is **NOT reduced by racial/elemental/size reductions**
- Reflected damage **CAN kill** the attacker
- Works on **melee physical attacks** — both auto-attacks AND melee skills (BF_SHORT flag)
- Does **NOT work** on ranged physical attacks (use bLongWeaponDamageReturn for that)
- Damage is reflected based on the **final damage dealt** (after all reductions to the defender)

**Reflect Sources:**

| Source | Reflect % | Type |
|--------|-----------|------|
| Orc Lord Card (Armor) | 30% melee | bShortWeaponDamageReturn |
| High Orc Card (Shield) | 5% melee | bShortWeaponDamageReturn |
| Shield Reflect skill (Crusader) | 13-40% melee (Lv 1-10) | Status-based reflect |

**Stacking behavior:**
- Multiple bShortWeaponDamageReturn sources are **additive** (Orc Lord 30% + High Orc 5% = 35% total)
- Shield Reflect (skill) and equipment reflect are counted **separately** — they produce two separate reflect damage instances (not combined)
- Example: Shield Reflect 40% + High Orc 5% = two hits back: one for 40% and one for 5%

**Edge case with defensive skills:**
- If the incoming attack is completely blocked (Auto Guard, Parrying, Kyrie Eleison), reflect should NOT trigger (since no damage was dealt)
- There was a known rAthena bug where reflect still triggered on blocked attacks — this was fixed

### Magic Reflection (bMagicDamageReturn)

**How Maya Card works:**
- **Effect**: n% **chance** to reflect single-target magic back at caster
- This is a **chance-based redirect**, not a percentage damage reflect
- When triggered, the **entire spell is redirected** back at the caster at full damage
- The caster takes their own spell's full damage (reduced by their own MDEF)

**Key mechanics:**
- Only reflects **single-target** magic spells (Fire Bolt, Jupitel Thunder, Holy Light, etc.)
- Does **NOT reflect AoE/ground-targeted** spells (Storm Gust, Meteor Storm, Magnus Exorcismus, etc.)
- **Exception**: Skills that require targeting a specific player (like Crimson Rock) CAN be reflected
- Multiple magic reflect sources have their **chances added** (+9 Magic Reflector 9% + Maya 50% = 59% chance)

**Maya Card:**
- **Slot**: Shield
- **Effect**: 50% chance to reflect single-target magic
- **rAthena script**: `bonus bMagicDamageReturn,50;`

### Shield Reflect (Crusader Skill) Details

| Level | Reflect % | SP Cost | Duration |
|-------|-----------|---------|----------|
| 1     | 13%       | 35      | 5 min    |
| 2     | 16%       | 40      | 5 min    |
| 3     | 19%       | 45      | 5 min    |
| 4     | 22%       | 50      | 5 min    |
| 5     | 25%       | 55      | 5 min    |
| 6     | 28%       | 60      | 5 min    |
| 7     | 31%       | 65      | 5 min    |
| 8     | 34%       | 70      | 5 min    |
| 9     | 37%       | 75      | 5 min    |
| 10    | 40%       | 80      | 5 min    |

- Requires a Shield equipped
- Only reflects short-range physical (melee) damage
- Status-based reflect (separate from equipment reflect)

---

## 7. SP Vanish (bSPVanishRate — Dark Priest Card)

### How bSPVanishRate Works

```
bonus2 bSPVanishRate, x, n;
```
- **x** = chance in tenths of a percent (x/10 = actual percentage)
- **n** = percentage of target's CURRENT SP destroyed
- Triggers on **normal attacks only** (not skills)

### Dark Priest Card

- **Slot**: Weapon
- **Effect**: 5% chance to destroy 10% of target's current SP when attacking
- **Class bonus**: Sage/Scholar gain 1 SP per physical attack on monsters
- **rAthena script**: `bonus2 bSPVanishRate,50,10;` (50/10 = 5% chance, 10% SP)

### Does It Work on Monsters?

- **Yes** — bSPVanishRate works on monsters (they have SP in the database)
- However, monster SP is mostly irrelevant to gameplay (monsters don't visibly "lose" skills from SP drain)
- The effect is **most meaningful in PvP** where draining player SP prevents skill usage
- If you want PvP-only behavior, use `bSPVanishRaceRate` with RC_Player

### Additional SP Vanish Variants

```
bonus3 bSPVanishRate,x,n,bf;     — with battle flag trigger (BF_WEAPON, BF_MAGIC, etc.)
bonus3 bSPVanishRaceRate,r,x,n;  — race-specific (RC_Player, RC_DemiHuman, etc.)
```

### Related: bSPDrainRate

Different from SPVanish — SPDrain **recovers** SP to the attacker:
```
bonus2 bSPDrainRate,x,n;  — x/10% chance to drain n% of DAMAGE dealt as SP to self
```

### Related: bHPVanishRate

Same mechanic but for HP:
```
bonus2 bHPVanishRate,x,n;  — x/10% chance to destroy n% of target's current HP
```

---

## Implementation Priority for Sabri_MMO

### Recommended Order

1. **Walk Delay / Hit Stun** (HIGH) — fundamental combat feel mechanic
2. **Knockback** (HIGH) — many first-class skills need it (Magnum Break, Arrow Shower, Cart Revolution)
3. **Damage Reflection** (MEDIUM) — needed for card system completeness
4. **DEF Ignore** (MEDIUM) — Samurai Specter Card, needed for high-end PvM
5. **Splash Attack** (MEDIUM) — Baphomet Card is iconic
6. **DEF-to-Damage** (LOW) — Thanatos Card / Ice Pick, endgame mechanic
7. **SP Vanish** (LOW) — primarily PvP mechanic

### Server-Side Implementation Summary

```javascript
// Walk Delay — in damage handler
const PLAYER_DMOTION = 230; // ms, from client sprite data
const PC_DAMAGE_WALK_DELAY_RATE = 20; // 20% of dMotion
function applyWalkDelay(target, multiHits = 1) {
  if (target.hasEndure || target.bonuses.bNoWalkDelay) return;
  const baseDelay = PLAYER_DMOTION * (PC_DAMAGE_WALK_DELAY_RATE / 100);
  const multiHitDelay = Math.max(0, (multiHits - 1)) * 200;
  target.walkDelayUntil = Date.now() + baseDelay + multiHitDelay;
}

// Knockback — in skill handler
function applyKnockback(target, source, cells, direction) {
  if (target.bonuses.bNoKnockback) return;
  if (target.isBoss) return;
  // direction = calculated from source→target, or skill-specific
  // Check each cell in path for walkability
  // Move target position
}

// Splash — in auto-attack handler
function applySplash(attacker, primaryTarget, splashRange) {
  if (splashRange <= 0) return;
  const targets = getEntitiesInRange(primaryTarget.pos, splashRange);
  for (const t of targets) {
    if (t === primaryTarget) continue;
    // Independent damage calc, independent HIT check
    const damage = calculatePhysicalDamage(attacker, t, /*isSplash*/true);
    // Do NOT trigger on-hit proc effects
  }
}

// DEF Ignore — in damage calc
function calculatePhysicalDamage(attacker, target, ...) {
  if (attacker.bonuses.bIgnoreDefClass?.includes(target.class)) {
    // Skip DEF reduction entirely
    return atkBeforeDefReduction;
  }
}

// DEF-to-Damage (Piercing) — in damage calc
function calculatePiercingDamage(atk, hardDEF, softDEF) {
  return Math.floor(atk * (hardDEF + softDEF) / 100);
}

// Reflect — in damage handler
function applyReflect(attacker, defender, damage, attackType) {
  if (attackType !== 'melee') return;
  const reflectPct = defender.bonuses.bShortWeaponDamageReturn || 0;
  if (reflectPct > 0) {
    const reflected = Math.floor(damage * reflectPct / 100);
    // Apply reflected damage to attacker — ignores DEF, can kill
    applyDamage(attacker, reflected, 'reflect');
  }
}

// SP Vanish — in auto-attack handler
function applySPVanish(attacker, target) {
  const vanish = attacker.bonuses.bSPVanishRate; // {chance, percent}
  if (!vanish) return;
  if (Math.random() * 1000 < vanish.chance) {
    const spLost = Math.floor(target.currentSP * vanish.percent / 100);
    target.currentSP = Math.max(0, target.currentSP - spLost);
  }
}
```

---

## Sources

- [rAthena item_bonus.txt](https://github.com/rathena/rathena/blob/master/doc/item_bonus.txt) — all bonus definitions
- [rAthena battle.cpp](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp) — damage calculation source
- [rAthena unit.cpp](https://github.com/rathena/rathena/blob/master/src/map/unit.cpp) — walk delay / knockback
- [rAthena pre-re skill_db.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml) — skill knockback values
- [rAthena battle.conf](https://github.com/rathena/rathena/blob/master/conf/battle/battle.conf) — walk delay config
- [rAthena True Hitlock commit](https://github.com/rathena/rathena/commit/503a3ff237f9f31e92904be943699064e2db9027)
- [rAthena Walk Delay Fix commit](https://github.com/rathena/rathena/commit/13651d57e18730d42f55aaebe5e7eb68701b9895)
- [rAthena Endure Effect commit](https://github.com/rathena/rathena/commit/420ebb272b94d025927a7bca4b2faf629ff06999)
- [rAthena Baphomet splash issue #7611](https://github.com/rathena/rathena/issues/7611)
- [rAthena bDefRatioAtkClass issue #2595](https://github.com/rathena/rathena/issues/2595)
- [rAthena bShortWeaponDamageReturn + AutoGuard issue](https://rathena.org/board/topic/126193-auto-guard-parrying-kyrie-eleison-bshortweapondamagereturn/)
- [rAthena reflect damage WoE issue #5331](https://github.com/rathena/rathena/issues/5331)
- [rAthena Thanatos/Ice Pick formula discussion](https://rathena.org/board/topic/131338-thanatosice-pick-formula/)
- [iROWiki DEF](https://irowiki.org/wiki/DEF)
- [iROWiki Reflect Type Damage](https://irowiki.org/wiki/Reflect_Type_Damage)
- [iROWiki Endure](https://irowiki.org/wiki/Endure)
- [iROWiki Occult Impaction](https://irowiki.org/wiki/Occult_Impaction)
- [iROWiki Classic Occult Impaction](https://irowiki.org/classic/Occult_Impaction)
- [iROWiki Arrow Repel](https://irowiki.org/wiki/Arrow_Repel)
- [iROWiki Arrow Shower](https://irowiki.org/wiki/Arrow_Shower)
- [iROWiki Bowling Bash](https://irowiki.org/wiki/Bowling_Bash)
- [iROWiki Magnum Break](https://irowiki.org/wiki/Magnum_Break)
- [iROWiki Jupitel Thunder](https://irowiki.org/wiki/Jupitel_Thunder)
- [iROWiki Cart Revolution](https://irowiki.org/wiki/Cart_Revolution)
- [iROWiki Shield Reflect](https://irowiki.org/wiki/Shield_Reflect)
- [iROWiki Knockback Skills Category](https://irowiki.org/wiki/Category:Knock_Back_Skills)
- [RateMyServer Eddga Card](https://ratemyserver.net/index.php?page=item_db&item_id=4123)
- [RateMyServer Endure](https://ratemyserver.net/index.php?page=skill_db&skid=8)
- [RateMyServer Baphomet Card](https://ratemyserver.net/index.php?page=item_db&item_id=4147)
- [RateMyServer Samurai Specter Card](https://ratemyserver.net/index.php?page=item_db&item_id=4263)
- [RateMyServer Memory of Thanatos Card](https://ratemyserver.net/index.php?page=item_db&item_id=4399)
- [RateMyServer RSX-0806 Card](https://ratemyserver.net/index.php?page=item_db&item_id=4342)
- [RateMyServer Dark Priest Card](https://ratemyserver.net/index.php?page=item_db&item_id=4171)
- [Samurai Specter PvP Test Results](https://blackintels.wordpress.com/2020/10/24/pvp-samurai-spector-test-results/)
- [Ragnarok Research Lab — Movement](https://ragnarokresearchlab.github.io/game-mechanics/movement/)
