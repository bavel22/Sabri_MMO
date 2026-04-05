# Card Deferred Systems — Full Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Card_System](../03_Server_Side/Card_System.md) | [Combat_System](../03_Server_Side/Combat_System.md)

**Date:** 2026-03-13
**Status:** COMPLETED — All 20 deferred card effect types implemented (auto-spell, drain, status procs, etc.)
**Scope:** All 20 deferred card effect types (parsed + stored, awaiting game system hooks)
**Excludes:** PvP-only systems (deferred separately)

---

## Table of Contents

1. [System A: Walk Delay / Hit Stun](#system-a-walk-delay--hit-stun)
2. [System B: Knockback](#system-b-knockback)
3. [System C: Splash Auto-Attack](#system-c-splash-auto-attack)
4. [System D: DEF Ignore & DEF-to-Damage](#system-d-def-ignore--def-to-damage)
5. [System E: Auto-Cast / Auto-Spell](#system-e-auto-cast--auto-spell)
6. [System F: Card Skill Grants](#system-f-card-skill-grants)
7. [System G: Autobonus Temp Procs](#system-g-autobonus-temp-procs)
8. [System H: Gem/Catalyst Consumption](#system-h-gemcatalyst-consumption)
9. [System I: Equipment Breaking](#system-i-equipment-breaking)
10. [System J: Movement Speed](#system-j-movement-speed)
11. [System K: Intravision / Hidden Detection](#system-k-intravision--hidden-detection)
12. [System L: Magic Damage Reflection](#system-l-magic-damage-reflection)
13. [System M: SP Vanish](#system-m-sp-vanish)
14. [System N: Item Group Healing & Drops](#system-n-item-group-healing--drops)
15. [Implementation Priority & Dependencies](#implementation-priority--dependencies)
16. [Cards Unlocked Per System](#cards-unlocked-per-system)
17. [Progress Tracker](#progress-tracker)

---

## System A: Walk Delay / Hit Stun

**Cards:** Eddga (`bNoWalkDelay`)
**Prerequisite:** None

### RO Classic Mechanics

- **Walk delay** = brief movement lock after taking damage. Does NOT block skills, items, or attacking — only movement.
- **Duration** = target's `dMotion` value (defined per monster, ~230ms for players) × `walk_delay_rate` (20% for players = ~46ms effective).
- **Multi-hit skills** add `(hits - 1) * 200ms` additional delay.
- **Endure** (Swordsman buff) prevents walk delay: 7+3×SkillLv seconds, limited to 7 monster hits (unlimited player hits), also grants +1-10 MDEF.
- `bNoWalkDelay` = permanent infinite Endure effect (no hit limit, no duration). Eddga Card provides this at the cost of MaxHP -25%.

### Implementation Plan

**Server (`index.js`):**
1. Add `player.walkDelayUntil = 0` field (timestamp when movement resumes).
2. When player takes physical damage: set `player.walkDelayUntil = now + 46` (46ms for players). If multi-hit: add `(hits - 1) * 200`.
3. Skip walk delay if `player.cardNoWalkDelay === true` OR player has Endure buff (check `hasBuff(player, 'endure')`).
4. In `player:position` handler: if `now < player.walkDelayUntil`, reject the position update (client rubber-bands).

**Files:**
- `server/src/index.js`: position handler + damage handlers

**Estimated effort:** Small (10 lines)

---

## System B: Knockback

**Cards:** RSX-0806 (`bNoKnockback`), Poisonous Toad/Bongun (`bAddSkillBlow`)
**Prerequisite:** None

### RO Classic Mechanics

- **37+ skills** cause knockback. Key first-class skills:
  | Skill | Cells | Direction |
  |-------|-------|-----------|
  | Magnum Break | 2 | Away from caster |
  | Arrow Shower | 2 | Away from caster |
  | Cart Revolution | 2 | Away from caster |
  | Arrow Repel | 6 | Away from caster |
  | Jupitel Thunder | 2-7 (by level) | Away from caster |
  | Bowling Bash | 5 | Caster facing direction |
  | Spear Stab | 6 | Away from caster |
  | Fire Wall | 1 per hit | Away from wall |
  | Bash + Fatal Blow | 5 | Away from caster |
- **Direction:** 8-directional, calculated from source → target position (push away from source). Same-cell default = West.
- **Cannot push through walls** — check walkability cell by cell (`path_blownpos()`).
- **RSX-0806 Card** (`bNoKnockback`) = complete immunity. Boss monsters also immune.
- GTB does NOT prevent knockback. Weight is irrelevant.
- `bAddSkillBlow` adds N cells of knockback to a specific skill.

### Implementation Plan

**Server (`index.js`):**
1. Add `knockbackTarget(target, sourcePos, cells, zone, io)` function:
   - Calculate direction vector from source to target (8-directional snap).
   - Move target `cells` cells in that direction, checking walkability per cell.
   - Update target position in server state.
   - Broadcast `combat:knockback { targetId, isEnemy, newX, newY, newZ, cells }` to zone.
2. Add `bNoKnockback` check: if `target.cardNoKnockback === true` OR `target.modeFlags?.isBoss`, skip knockback.
3. Call `knockbackTarget()` from skill handlers that have knockback (Magnum Break, Arrow Repel, Jupitel Thunder, Fire Wall hit, Fatal Blow proc, etc.).
4. For `bAddSkillBlow`: check `attacker.cardAddSkillBlow[]` for extra cells.

**Client (UE5):**
1. Handle `combat:knockback` event: lerp/teleport target actor to new position.

**Files:**
- `server/src/index.js`: new function + skill handler modifications
- Client: `DamageNumberSubsystem` or new handler for knockback event

**Estimated effort:** Medium (50-80 lines server, 20 lines client event handling)

---

## System C: Splash Auto-Attack

**Cards:** Baphomet (`bSplashRange`)
**Prerequisite:** System B (knockback) helpful but not required

### RO Classic Mechanics

- **Range 1** = 3×3 area around primary target (8 surrounding cells). Only highest `bSplashRange` applies.
- Each secondary target gets **independent damage calculation**: independent HIT check, independent crit roll, full 100% ATK.
- Splash does NOT trigger on-hit proc effects (status procs, drain, auto-cast). Treated as a pseudo-skill internally.
- Works on **auto-attacks only**, not skills.
- Baphomet Card also gives HIT -10 penalty.

### Implementation Plan

**Server (`index.js`):**
1. In auto-attack tick, after right-hand damage is applied to primary target:
   - Check `attacker.cardSplashRange > 0`.
   - Find all enemies within `splashRange` cells of the primary target (not the attacker).
   - For each secondary target: run `calculatePhysicalDamage()` independently.
   - Apply damage, broadcast `combat:damage` for each secondary target with `hitType: 'splash'`.
   - Do NOT call `processCardDrainEffects`, `processCardStatusProcsOnAttack` for splash hits.
2. Range calculation: `splashRange = 1` means within ~150 UE units of primary target's position (tunable).

**Files:**
- `server/src/index.js`: auto-attack tick splash loop

**Estimated effort:** Medium (40-60 lines)

---

## System D: DEF Ignore & DEF-to-Damage

**Cards:** Samurai Spector (`bIgnoreDefClass`), Memory of Thanatos (`bDefRatioAtkClass`)
**Prerequisite:** None

### RO Classic Mechanics

#### DEF Ignore (Samurai Spector)
- Ignores **both Hard DEF and Soft DEF** — target treated as having 0 DEF.
- Works on both auto-attacks AND skills (all physical damage).
- `Class_Normal` = only normal monsters, not bosses.
- Samurai Spector side effects: HP regen disabled, 666 HP drain per 10 seconds (already parsed as `bHPLossRate`).

#### DEF-to-Damage (Thanatos Card / Ice Pick)
- Pre-renewal formula: `ATK * (HardDEF + SoftDEF) / 100` **replaces** normal DEF reduction step.
- Can deal **LESS damage** than normal against low-DEF targets (breakeven ~150 combined DEF).
- Thanatos Card: `Class_All`, DEF -30, FLEE -30, drains 1 SP per attack (already parsed as `bSPDrainValue`).

### Implementation Plan

**Server (`ro_damage_formulas.js`):**
1. **DEF Ignore:** In `calculatePhysicalDamage()`, before Step 8 (DEF reduction):
   ```javascript
   // Check bIgnoreDefClass
   if (attacker.cardIgnoreDefClass) {
       const targetClass = (target.modeFlags?.isBoss) ? 'boss' : 'normal';
       if (attacker.cardIgnoreDefClass === targetClass || attacker.cardIgnoreDefClass === 'all') {
           // Skip BOTH hard DEF and soft DEF steps entirely
           skipDEF = true;
       }
   }
   ```
2. **DEF-to-Damage:** In same location:
   ```javascript
   if (attacker.cardDefRatioAtkClass && !skipDEF) {
       const targetClass = (target.modeFlags?.isBoss) ? 'boss' : 'normal';
       if (attacker.cardDefRatioAtkClass === targetClass || attacker.cardDefRatioAtkClass === 'all') {
           // Replace DEF reduction with DEF-as-bonus
           const combinedDEF = rawHardDef + defDerived.softDEF;
           totalATK = Math.floor(totalATK * combinedDEF / 100);
           skipDEF = true; // Skip normal DEF steps
       }
   }
   ```

**Files:**
- `server/src/ro_damage_formulas.js`: Steps 8/8a modifications

**Estimated effort:** Small (15-20 lines)

---

## System E: Auto-Cast / Auto-Spell

**Cards:** 26 bAutoSpell + 23 bAutoSpellWhenHit = 49 cards
**Prerequisite:** Skill system must be able to execute skills programmatically

### RO Classic Mechanics

#### Auto-Cast on Attack (bAutoSpell)
- Triggers on **successful physical auto-attack hit** only (attack must connect, not on miss).
- **No SP consumed**, **no cast time**, **no gem/catalyst cost**.
- After-cast delay still applies but does not prevent subsequent procs.
- **Targeting:** Offensive skills → cast on enemy. Support/self skills → cast on self. Ground skills → cast at target's position.
- **Multiple cards can proc on same hit** — each checked independently via `chance/1000` roll.
- Bolts fire all hits for their level (Fire Bolt Lv3 = 3 hits).

#### Auto-Cast When Hit (bAutoSpellWhenHit)
- Triggers on **physical damage taken** by default. `bonus5` variant can include magic.
- **No SP**, **no cast time**, **no gems**.
- Offensive skills → cast on attacker. Self/support → cast on self.

#### Key Cards

| Card | Type | Skill | Level | Chance |
|------|------|-------|-------|--------|
| Gryphon | onAttack | Bowling Bash | 5 | 1% |
| Poisonous Toad | onAttack | Poison | 1 | 2% |
| Mutant Dragonoid | onAttack | Fire Ball | 3-5 | 5% |
| Enchanted Peach Tree | onAttack | Heal | 1-10 | 2% |
| Wind Ghost | onAttack | Jupitel Thunder | 3-10 | 2% |
| Dark Lord | onHit | Meteor Storm | 5 | 10% |
| Grand Peco | onHit | Gloria | 1 | 5% |
| Amon Ra | onHit | Kyrie Eleison | 10 | 3-10% |
| Loli Ruri | onHit | Heal | 1 | 50% (self) |
| Clock | onHit | Auto Guard | 3-10 | 3% |
| Geographer | onHit | Blessing | 2-10 | 3% |

### Implementation Plan

**Server (`index.js`):**
1. Create `processCardAutoSpellOnAttack(attacker, target, targetIsEnemy, zone)`:
   ```javascript
   function processCardAutoSpellOnAttack(attacker, target, targetIsEnemy, zone) {
       if (!attacker.cardAutoSpell || attacker.cardAutoSpell.length === 0) return;
       for (const proc of attacker.cardAutoSpell) {
           const chance = proc.chance; // chance/1000
           if (Math.random() * 1000 >= chance) continue;
           // Resolve skill level (may be expression like "3+7*(maxed)")
           const level = resolveAutoSpellLevel(proc.level, attacker);
           // Execute the skill effect (damage/heal/buff) without SP/cast/gems
           executeAutoSpellEffect(attacker, target, targetIsEnemy, proc.skill, level, zone);
       }
   }
   ```
2. Create `processCardAutoSpellWhenHit(target, attacker, attackerIsEnemy, zone)`:
   - Same pattern but target is the card owner, attacker is the enemy.
   - Offensive skills → target the attacker. Self/support → target self.
3. Create `executeAutoSpellEffect(caster, target, isEnemy, skillName, level, zone)`:
   - Look up skill in `ro_skill_data.js`.
   - For damage skills: call `calculatePhysicalDamage` or `calculateMagicalDamage` with skill multiplier.
   - For heal skills: call `calculateHealAmount` equivalent.
   - For buff skills: call `applyBuff`.
   - Broadcast `skill:effect_damage` for damage/heals, `skill:buff_applied` for buffs.
   - **No SP deduction, no cast time, no gem consumption.**
4. Call `processCardAutoSpellOnAttack` from auto-attack tick after damage (where drain/status procs already are).
5. Call `processCardAutoSpellWhenHit` from enemy-hits-player handler (where `processCardStatusProcsWhenHit` already is).

**Skill resolution for conditional levels** (`3+7*(getskilllv("WZ_JUPITEL") == 10)`):
- Parse the level expression at rebuild time using player's learned skills.
- Store resolved integer level in `cardAutoSpell[].resolvedLevel`.

**Files:**
- `server/src/index.js`: new functions + auto-attack/enemy-attack hook calls

**Estimated effort:** Large (100-150 lines)

---

## System F: Card Skill Grants

**Cards:** 17 cards (Smokie→Hiding, Creamy→Teleport, Sidewinder→Double Attack, etc.)
**Prerequisite:** Skill system must support temporary skill additions

### RO Classic Mechanics

- Card grants a **usable skill** while equipped. Active skills cost SP. Passive skills just work.
- Granted skills appear in the Skill Window and can be hotkeyed.
- **Level rule:** Higher level wins. If player has Hiding Lv10 and card grants Lv1, player keeps Lv10.
- Sidewinder Card special: grants Double Attack Lv1 + `bDoubleRate,5` that works with **all weapon types** (not just daggers).
- Skills are removed immediately on unequip.

### Complete Card → Skill List

| Card | Skill | Level | Type |
|------|-------|-------|------|
| Poporing (4033) | Detoxify | 1 | Active |
| Creamy (4040) | Teleport | 1 | Active |
| Smokie (4044) | Hiding | 1 | Active (toggle) |
| Poison Spore (4048) | Envenom | 3 | Active |
| Vitata (4053) | Heal | 1 | Active |
| Pirate Skeleton (4073) | Discount | 5 | Passive |
| Marine Sphere (4084) | Magnum Break | 3 | Active |
| Frilldora (4088) | Cloaking | 1 | Active (toggle) |
| Obeaune (4093) | Cure | 1 | Active |
| Horong (4103) | Sight | 1 | Active |
| Sidewinder (4117) | Double Attack | 1 | Passive |
| Joker (4139) | Steal | 1 | Active |
| Beholder (4356) | Cast Cancel | 1 | Active |
| Lord Knight (4357) | Berserk (Frenzy) | 1 | Active |
| Assassin Cross (4359) | Cloaking | 3 | Active (toggle) |
| Seeker (4414) | Stone Curse | 1 | Active |
| Stapo (4424) | Find Stone | 1 | Active |

### Implementation Plan

**Server (`index.js`):**
1. In `rebuildCardBonuses`, after aggregating `cardSkillGrant[]`:
   - Build `player.cardGrantedSkills = {}` map of `{ skillId: level }`.
   - For each grant: resolve skill name → skill ID via `ro_skill_data.js`.
   - Merge with player's learned skills: `effectiveLevel = max(learned, cardGranted)`.
2. In `skill:use` handler, when checking if player knows skill:
   - Also check `player.cardGrantedSkills[skillId]`.
3. When sending `player:stats` or skill list: include card-granted skills in the payload.

**Client (UE5):**
1. Parse card-granted skills from `player:stats` payload.
2. Show them in skill tree (grayed-out "Equipment" badge) and allow hotbar placement.

**Files:**
- `server/src/index.js`: rebuildCardBonuses + skill:use handler
- Client: SkillTreeSubsystem + HotbarSubsystem

**Estimated effort:** Medium (40-60 lines server, 20 lines client)

---

## System G: Autobonus Temp Procs

**Cards:** Vanberk, Isilla, Hodremlin, Ice Titan, Atroce (5 cards)
**Prerequisite:** Buff system

### RO Classic Mechanics

- `autobonus "{ bonus script }",rate,duration,flags,"{ visual script }"` — on attack.
- `autobonus2 "{ bonus script }",rate,duration,flags,"{ visual script }"` — when attacked.
- Rate: `N/10000` (10000 = 100%).
- Duration: in milliseconds.
- Bonus script re-evaluates on stat recalculation (persists during duration).
- Can re-proc while active (refreshes duration).

### The 5 Cards

| Card | Type | Bonus | Chance | Duration |
|------|------|-------|--------|----------|
| Vanberk | onAttack | +100 CRIT | 0.05% | 5s |
| Isilla | onAttack | -50% cast time, +30 FLEE | 5% (magic only) | 5s |
| Atroce | onAttack | +100% ASPD | 0.05% | 10s |
| Hodremlin | onHit | +30 Perfect Dodge | 0.03% | 10s |
| Ice Titan | onHit | +10 DEF | 0.03% | 10s |

### Implementation Plan

**Server (`index.js`):**
1. Parse autobonus scripts into structured data at `rebuildCardBonuses` time:
   - `{ bonuses: { critical: 100 }, chance: 5, durationMs: 5000, triggerType: 'onAttack' }`
2. On auto-attack hit (or when hit), check each autobonus entry:
   - Roll `chance/10000`.
   - If success: apply as a temporary buff via `applyBuff()` with the parsed bonuses.
   - Use buff type `'autobonus_N'` (indexed) with the duration.
3. The buff system already handles expiry and stat recalculation.

**Files:**
- `server/src/index.js`: autobonus parsing in rebuild + proc checks in combat tick

**Estimated effort:** Medium (50-70 lines)

---

## System H: Gem/Catalyst Consumption

**Cards:** Mistress (`bNoGemStone`)
**Prerequisite:** None (just need to add gem checks to existing skill handlers)

### RO Classic Mechanics

#### Skills Requiring Gems (Pre-Renewal)

| Gem | Item ID | Skills |
|-----|---------|--------|
| Blue Gemstone | 717 | Warp Portal, Safety Wall, Sanctuary, Resurrection, Magnus Exorcismus, Fire Pillar (Lv6-10) |
| Red Gemstone | 716 | Stone Curse, Venom Dust, Venom Splasher |
| Yellow Gemstone | 715 | Hocus Pocus (×2), Dispell |
| Blue + Yellow | 717+715 | Land Protector, Ganbantein |
| Holy Water | 523 | Aspersio |

#### Mistress Card Effect
- Removes gem costs entirely with **+25% SP cost penalty** (already parsed as `bUseSPrate,25` — but WAIT, GTB has `bUseSPrate,100` and Mistress has `bNoGemStone` + separate SP penalty).
- Exceptions: Hocus Pocus still needs 1 Yellow (reduced from 2), Ganbantein still needs full gems.

### Implementation Plan

**Server (`index.js`):**
1. In `ro_skill_data.js`, add `catalysts` field to skill definitions:
   ```javascript
   { id: 'WZ_FIREPILLAR', ..., catalysts: [{ itemId: 717, quantity: 1, minLevel: 6 }] }
   ```
2. In `skill:use` handler, before SP check:
   - Look up `skill.catalysts`.
   - If catalysts required AND player does NOT have `cardNoGemStone`:
     - Query player inventory for required items.
     - If missing: emit `skill:error { message: 'Missing Blue Gemstone' }`.
     - If present: consume after cast completes.
   - If `cardNoGemStone === true`: skip catalyst check entirely.
3. Consume catalysts in `executeCastComplete` (after SP deduction) by calling `removeItemFromInventory`.

**Files:**
- `server/src/ro_skill_data.js`: add `catalysts` field to gem-requiring skills
- `server/src/index.js`: catalyst check + consumption in skill handler

**Estimated effort:** Medium (40-50 lines)

---

## System I: Equipment Breaking

**Cards:** `bUnbreakableArmor` (3 cards), `bUnbreakableWeapon` (3 cards), `bBreakWeaponRate` (MasterSmith), `bBreakArmorRate` (MasterSmith)
**Prerequisite:** Equipment system must support "broken" state

### RO Classic Mechanics

- **Official `equip_natural_break_rate = 0`** — normal auto-attacks do NOT break equipment.
- **Self-break:** Over Thrust adds 0.1% per attack to break YOUR weapon (Axes/Maces/Books/Staves exempt).
- **Target-break cards:** MasterSmith Card = 10% weapon / 7% armor per hit.
- **Skills:** Meltdown (Whitesmith buff, duration-based), Acid Terror (5-45% armor break).
- **Broken state:** Equipment turns red in UI, becomes unequipped, stats zeroed. NOT destroyed — repairable.
- **Repair:** Blacksmith skill "Repair Weapon" (material cost) or Repairman NPC (5,000z).
- **Protection:** `bUnbreakableArmor/Weapon` = immune to combat breaking (NOT refine breaking).

### Implementation Plan

**Database:**
1. Add `is_broken BOOLEAN DEFAULT FALSE` to `character_inventory`.

**Server (`index.js`):**
1. On auto-attack hit, check `attacker.cardBreakWeaponRate > 0`:
   - Roll `chance = cardBreakWeaponRate / 10000`.
   - If success AND target has equipped weapon AND weapon is NOT `bUnbreakableWeapon`:
     - Set `is_broken = TRUE` on the weapon inventory row.
     - Unequip the weapon server-side.
     - Emit `inventory:equip_broken { inventoryId, slot: 'weapon' }`.
2. Same for `cardBreakArmorRate` targeting armor slot.
3. `bUnbreakableArmor/Weapon` checks already parsed and stored — just add the guard.
4. Add `equipment:repair` socket event handler for NPC repair service.

**Files:**
- `database/migrations/add_equipment_break.sql`
- `server/src/index.js`: break checks in combat + repair handler

**Estimated effort:** Medium (60-80 lines + migration)

---

## System J: Movement Speed

**Cards:** Moonlight Flower (`bSpeedRate,25`)
**Prerequisite:** Server-side movement speed tracking

### RO Classic Mechanics

- **Base speed:** 150ms per cell (`DEFAULT_WALK_SPEED = 150`).
- **Speed cap:** 20ms/cell minimum.
- **bSpeedRate formula:** `final_speed = base_speed * (100 - speed_rate) / 100`. Only highest value used.
- **Moonlight Flower:** `bSpeedRate,25` = 112ms/cell (~33% faster).
- **Increase AGI:** ~110ms/cell. Stacks with Peco Peco ride.
- **Curse status:** +300ms to walk speed (massive slowdown).
- AGI stat does NOT affect movement speed — only ASPD.

### Implementation Plan

**Server (`index.js`):**
1. Calculate `player.moveSpeed` in stat rebuild:
   ```javascript
   let moveSpeed = 150; // base ms per cell
   const speedRate = player.cardSpeedRate || 0; // only highest from cards
   if (speedRate > 0) moveSpeed = Math.max(20, Math.floor(150 * (100 - speedRate) / 100));
   // Curse: moveSpeed += 300
   player.moveSpeed = moveSpeed;
   ```
2. Include `moveSpeed` in `player:stats` payload.
3. Client adjusts character movement speed based on server value.
4. Server validates position updates against moveSpeed (anti-speed-hack).

**Client (UE5):**
1. Parse `moveSpeed` from `player:stats`.
2. Adjust `CharacterMovementComponent->MaxWalkSpeed` accordingly.

**Files:**
- `server/src/index.js`: stat calculation + stats payload
- Client: `BasicInfoSubsystem` or character movement

**Estimated effort:** Small (20 lines server, 10 lines client)

---

## System K: Intravision / Hidden Detection

**Cards:** Maya Purple (`bIntravision`)
**Prerequisite:** Hiding/Cloaking system (already implemented)

### RO Classic Mechanics

- Passive, always active while equipped (headgear slot).
- Detects **Hiding** and **Cloaking** (player sees hidden targets as translucent).
- Does **NOT** detect **Chase Walk** (Stalker skill — immune to all detection).
- No range limit within screen view. Personal vision only (not shared with party).

### Implementation Plan

**Server (`index.js`):**
1. In `findAggroTarget()` (enemy AI), already checks `enemy.modeFlags.detector`. Add:
   ```javascript
   // For player-targeted detection (PvP future): player.cardIntravision
   ```
2. In multiplayer visibility system: when sending `player:position` updates, for hidden players:
   - If observer has `cardIntravision === true`: include the hidden player in their updates (with a `isTranslucent: true` flag).
   - If observer does NOT have intravision: exclude hidden players.
3. Current enemies can't hide, so intravision is mainly relevant for:
   - Future PvP (seeing hidden Assassins/Thieves).
   - Future boss monster hide mechanics.

**Files:**
- `server/src/index.js`: position broadcast visibility filter

**Estimated effort:** Small (15 lines)

---

## System L: Magic Damage Reflection

**Cards:** Maya (`bMagicDamageReturn,50`), Cat O' Nine Tails (`bMagicDamageReturn,5`), Frus (`bMagicDamageReturn,refine*2`)
**Prerequisite:** Enemy magic attacks (enemies currently only use physical attacks)

### RO Classic Mechanics

- Maya Card: 50% **chance** to reflect single-target magic spells entirely back at caster.
- AoE/ground-target spells are NOT reflected.
- Multiple sources add chances (50% + 5% = 55% chance).
- The reflected spell deals full damage to the original caster.

### Implementation Plan

**Deferred until enemies gain magic attacks.** The `processCardMagicReflection` function and `player.cardMagicDamageReturn` field already exist. When enemy spellcasting is added:

1. Before applying magic damage to player: check `player.cardMagicDamageReturn`.
2. Roll `chance = cardMagicDamageReturn` (percentage).
3. If success AND spell is single-target: redirect damage to caster.
4. Broadcast reflected damage as `skill:effect_damage` from player to enemy.

**Estimated effort:** Small once enemy magic exists (20 lines)

---

## System M: SP Vanish

**Cards:** Dark Priest (`bSPVanishRate,50,10`)
**Prerequisite:** None for PvE (monsters have SP in DB)

### RO Classic Mechanics

- `bonus2 bSPVanishRate,chance,percent` = `chance/10` % to destroy `percent`% of target's current SP.
- Dark Priest: 5% chance to destroy 10% of target SP.
- Works on **normal attacks only** (not skills).
- Works on monsters (they have SP in their DB template), most impactful in PvP.

### Implementation Plan

**Server (`index.js`):**
1. In auto-attack tick, after damage (where drain effects already are):
   ```javascript
   if (attacker.cardSpVanishRate && !isMiss) {
       if (Math.random() * 100 < attacker.cardSpVanishRate.chance / 10) {
           const spLost = Math.floor((target.mana || 0) * attacker.cardSpVanishRate.percent / 100);
           if (spLost > 0) target.mana = Math.max(0, (target.mana || 0) - spLost);
       }
   }
   ```
2. For PvE: most enemies don't track mana, so this is a no-op. For PvP: applies to players.

**Files:**
- `server/src/index.js`: auto-attack tick

**Estimated effort:** Tiny (5 lines)

---

## System N: Item Group Healing & Drops

**Cards:** `bAddItemGroupHealRate` (5 cards), `bAddMonsterDropItemGroup` (2 cards)
**Prerequisite:** Item group definitions

### RO Classic Item Groups

| Group | Items |
|-------|-------|
| `IG_POTION` | Red Potion (501), Orange Potion (502), Yellow Potion (503), White Potion (504) |
| `IG_JUICE` | Apple Juice (531), Banana Juice (532), Carrot Juice (533) |
| `IG_CANDY` | Candy (529), Candy Cane (530) |
| `IG_FOOD` | 22 items: Apple (512), Banana (513), Royal Jelly (526), etc. |
| `IG_RECOVERY` | 14 items: Red Herb (507)...Royal Jelly (526), Rice Cake (528) |

### Implementation Plan

**Server (`server/src/ro_item_groups.js` — new file):**
1. Define item group → item ID mappings:
   ```javascript
   const ITEM_GROUPS = {
       'IG_Potion': [501, 502, 503, 504],
       'IG_Juice': [531, 532, 533],
       'IG_Candy': [529, 530],
       'IG_Food': [512, 513, 514, ...],
       'IG_Recovery': [507, 508, 509, ...],
   };
   ```
2. In `inventory:use` heal handler: check if used item's ID is in any group that has a card heal bonus:
   ```javascript
   for (const [group, itemIds] of Object.entries(ITEM_GROUPS)) {
       if (itemIds.includes(item.item_id) && player.cardAddItemGroupHealRate[group]) {
           hpHeal = Math.floor(hpHeal * (100 + player.cardAddItemGroupHealRate[group]) / 100);
       }
   }
   ```
3. For `bAddMonsterDropItemGroup`: on kill, check `player.cardAddMonsterDropItemGroup[]`:
   - Roll chance (10000 = 100% activation).
   - Pick random item from the group using internal drop rates.
   - Add to inventory.

**Files:**
- `server/src/ro_item_groups.js` (new)
- `server/src/index.js`: import + heal handler + kill handler

**Estimated effort:** Medium (new file 30 lines + 30 lines hooks)

---

## Implementation Priority & Dependencies

```
Priority 1 (No dependencies, easy wins):
├── System D: DEF Ignore + DEF-to-Damage (15 lines, 2 cards)
├── System M: SP Vanish (5 lines, 1 card)
├── System A: Walk Delay (10 lines, 1 card)
└── System J: Movement Speed (20 lines, 1 card)

Priority 2 (Small effort, moderate impact):
├── System K: Intravision (15 lines, 1 card)
├── System H: Gem Consumption (50 lines, 1 card)
└── System N: Item Groups (60 lines, 7 cards)

Priority 3 (Medium effort, high card count):
├── System E: Auto-Cast (150 lines, 49 cards) ← BIGGEST IMPACT
├── System F: Skill Grants (60 lines, 17 cards)
└── System G: Autobonus Procs (70 lines, 5 cards)

Priority 4 (Medium effort, infrastructure):
├── System B: Knockback (80 lines, 2 cards + many skills)
├── System C: Splash Auto-Attack (60 lines, 1 card)
└── System I: Equipment Breaking (80 lines + migration, 7 cards)

Priority 5 (Deferred — needs future systems):
└── System L: Magic Reflection (needs enemy magic, 3 cards)
```

---

## Cards Unlocked Per System

| System | Cards Unlocked | Total Effect Types |
|--------|---------------|-------------------|
| D: DEF Ignore/Damage | Samurai Spector, Thanatos | 2 |
| M: SP Vanish | Dark Priest | 1 |
| A: Walk Delay | Eddga | 1 |
| J: Movement Speed | Moonlight Flower | 1 |
| K: Intravision | Maya Purple | 1 |
| H: Gem Consumption | Mistress | 1 |
| N: Item Groups | Galapago, Dumpling Child, Spring Rabbit, Hermit Plant, Blazer, Tengu + Snowier | 7 |
| E: Auto-Cast | 26 bAutoSpell + 23 bAutoSpellWhenHit | 49 |
| F: Skill Grants | Smokie, Creamy, Sidewinder, etc. | 17 |
| G: Autobonus | Vanberk, Isilla, Hodremlin, Ice Titan, Atroce | 5 |
| B: Knockback | RSX-0806, Poisonous Toad, Bongun | 3 (+ many skills) |
| C: Splash | Baphomet | 1 |
| I: Equipment Break | Golem, Zipper Bear, Cornutus, Baby Leopard, RSX-0806, MasterSmith, Randgris | 7 |
| L: Magic Reflect | Maya, Cat O' Nine Tails, Frus | 3 |
| **TOTAL** | | **~100 cards fully activated** |

---

## Progress Tracker

| # | System | Status | Cards Fixed |
|---|--------|--------|-------------|
| D | DEF Ignore + DEF-to-Damage | [x] DONE (2026-03-13) | 2 |
| M | SP Vanish | [x] DONE (2026-03-13) | 1 |
| A | Walk Delay | [x] DONE (2026-03-13) | 1 |
| J | Movement Speed | [x] DONE (2026-03-13) | 1 |
| K | Intravision | [x] DONE (2026-03-13) | 1 |
| H | Gem Consumption | [x] DONE (2026-03-13) | 1 |
| N | Item Groups | [x] DONE (2026-03-13) | 7 |
| E | Auto-Cast | [x] DONE (2026-03-13) | 49 |
| F | Skill Grants | [x] DONE (2026-03-13) | 17 |
| G | Autobonus Procs | [x] DONE (2026-03-13) | 5 |
| B | Knockback | [x] DONE (2026-03-13) | 3 |
| C | Splash Auto-Attack | [x] DONE (2026-03-13) | 1 |
| I | Equipment Breaking | [x] DONE (2026-03-13) | 7 |
| L | Magic Reflection | [ ] DEFERRED (needs enemy magic) | 3 |

**13/14 systems implemented. 96 cards fully activated. Only magic reflection deferred (needs enemy spellcasters).**
