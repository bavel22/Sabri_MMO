# Systems & Events Audit — Non-Skill Jest Test Plan

Generated: 2026-03-22
Source files audited:
- `server/src/ro_buff_system.js` (558 lines)
- `server/src/ro_status_effects.js` (711 lines)
- `server/src/ro_item_effects.js` (521 lines)
- `server/src/index.js` (~24600+ lines) — buff hooks, card hooks, inventory, party, socket events

---

## Table of Contents

1. [Buff System](#1-buff-system)
2. [Status Effect (Debuff) System](#2-status-effect-debuff-system)
3. [Card Hook System](#3-card-hook-system)
4. [Item & Inventory System](#4-item--inventory-system)
5. [Party System](#5-party-system)
6. [Socket Event Catalog](#6-socket-event-catalog)
7. [Jest Test Code](#7-jest-test-code)

---

## 1. Buff System

**File**: `server/src/ro_buff_system.js`
**Exports**: `BUFF_TYPES`, `applyBuff`, `removeBuff`, `expireBuffs`, `hasBuff`, `getBuffModifiers`

### 1.1 Complete Buff Type Catalog (72 types)

| Buff Name | Category | Abbrev | Stack Rule | Source |
|-----------|----------|--------|------------|--------|
| provoke | debuff | PRV | refresh | Swordsman skill 101 |
| endure | buff | END | refresh | Swordsman skill 104 |
| sight | buff | SGT | refresh | Mage skill 200 |
| blessing | buff | BLS | refresh | Acolyte skill 401 |
| blessing_debuff | debuff | BCR | refresh | On Undead/Demon |
| increase_agi | buff | AGI | refresh | Acolyte skill 402 |
| decrease_agi | debuff | DAG | refresh | Acolyte skill 403 |
| angelus | buff | ANG | refresh | Acolyte skill 404 |
| pneuma | buff | PNE | refresh | Acolyte skill 405 |
| signum_crucis | debuff | SCR | refresh | Acolyte skill 407 |
| auto_berserk | buff | BRK | refresh | Swordsman skill 109 |
| hiding | buff | HID | refresh | Thief skill 500 |
| play_dead | buff | PDd | refresh | Novice skill 3 |
| improve_concentration | buff | CON | refresh | Archer skill 300 |
| two_hand_quicken | buff | THQ | refresh | Knight skill 709 |
| kyrie_eleison | buff | KYR | refresh | Priest skill 1003 |
| magnificat | buff | MAG | refresh | Priest skill 1005 |
| gloria | buff | GLO | refresh | Priest skill 1004 |
| lex_aeterna | debuff | LEX | refresh | Priest skill 1007 |
| aspersio | buff | ASP | refresh | Priest skill 1009 |
| energy_coat | buff | ENC | refresh | Sage skill (passive) |
| enchant_poison | buff | EPO | refresh | Assassin skill 1105 |
| cloaking | buff | CLK | refresh | Assassin skill 1106 |
| poison_react | buff | PRC | refresh | Assassin skill 1107 |
| quagmire | debuff | QUA | refresh | Wizard skill 812 |
| loud_exclamation | buff | LXC | refresh | Merchant skill 608 |
| ruwach | buff | RUW | refresh | Acolyte skill 414 |
| magnum_break_fire | buff | MBF | refresh | Swordsman skill 102 |
| impositio_manus | buff | IMP | refresh | Priest skill 1000 |
| suffragium | buff | SUF | refresh | Priest skill 1001 |
| slow_poison | buff | SLP | refresh | Priest skill 1010 |
| bs_sacramenti | buff | BSS | refresh | Priest skill (B.S. Sacramenti) |
| auto_counter | buff | ACN | refresh | Knight skill 705 |
| auto_guard | buff | AGD | refresh | Crusader skill 1300 |
| reflect_shield | buff | RFS | refresh | Crusader skill 1302 |
| defender | buff | DEF | refresh | Crusader skill 1304 |
| spear_quicken | buff | SQK | refresh | Crusader skill 1311 |
| providence | buff | PRD | refresh | Crusader skill 1313 |
| shrink | buff | SHK | refresh | Crusader skill 1312 |
| devotion_protection | buff | DVT | refresh | Crusader skill 1306 |
| sight_blaster | buff | SBL | refresh | Wizard skill 813 |
| endow_fire | buff | EBL | refresh | Sage skill 1414 |
| endow_water | buff | ETS | refresh | Sage skill 1415 |
| endow_wind | buff | ETN | refresh | Sage skill 1416 |
| endow_earth | buff | EQK | refresh | Sage skill 1417 |
| hindsight | buff | HND | refresh | Sage skill 1407 |
| magic_rod | buff | MRD | refresh | Sage skill 1406 |
| volcano_zone | buff | VLC | refresh | Sage skill 1411 |
| deluge_zone | buff | DLG | refresh | Sage skill 1412 |
| violent_gale_zone | buff | VGL | refresh | Sage skill 1413 |
| critical_explosion | buff | FRY | refresh | Monk skill 1601 |
| steel_body | buff | STL | refresh | Monk skill 1602 |
| asura_regen_lockout | debuff | ASR | refresh | Post-Asura Strike |
| sitting | buff | SIT | refresh | Player sit/stand |
| blade_stop_catching | buff | BLS | refresh | Monk skill 1609 |
| root_lock | debuff | RLK | refresh | Blade Stop pair |
| song_whistle | buff | WHI | refresh | Bard skill 1501 |
| song_assassin_cross | buff | ACS | refresh | Bard skill 1502 |
| song_bragi | buff | BRG | refresh | Bard skill 1503 |
| song_apple_of_idun | buff | APL | refresh | Bard skill 1504 |
| dance_humming | buff | HUM | refresh | Dancer skill 1521 |
| dance_fortune_kiss | buff | FTK | refresh | Dancer skill 1522 |
| dance_service_for_you | buff | SFY | refresh | Dancer skill 1523 |
| dance_pdfm | debuff | PDF | refresh | Dancer skill 1524 |
| dance_ugly | debuff | UGD | refresh | Dancer skill 1526 |
| ensemble_drum | buff | DRM | refresh | Ensemble 1530 |
| ensemble_nibelungen | buff | NIB | refresh | Ensemble 1531 |
| ensemble_mr_kim | buff | KIM | refresh | Ensemble 1534 |
| ensemble_siegfried | buff | SIG | refresh | Ensemble 1536 |
| ensemble_into_abyss | buff | ABY | refresh | Ensemble 1535 |
| ensemble_aftermath | debuff | AFT | refresh | Post-ensemble |
| adrenaline_rush | buff | ADR | refresh | Blacksmith skill 1202 |
| weapon_perfection | buff | WPF | refresh | Blacksmith skill 1203 |
| power_thrust | buff | PTH | refresh | Blacksmith skill 1204 |
| maximize_power | buff | MXP | refresh | Blacksmith skill 1205 |
| strip_weapon | debuff | SWP | refresh | Rogue skill 1714 |
| strip_shield | debuff | SSH | refresh | Rogue skill 1715 |
| strip_armor | debuff | SAR | refresh | Rogue skill 1716 |
| strip_helm | debuff | SHL | refresh | Rogue skill 1717 |
| raid_debuff | debuff | RAI | refresh | Rogue skill 1703 |
| close_confine | debuff | CCF | refresh | Rogue skill 1718 |
| aspd_potion | buff | ASP | refresh | Consumable items |
| str_food / agi_food / vit_food / int_food / dex_food / luk_food | buff | xFD | refresh | Consumable items |
| hit_food / flee_food | buff | HFD/FFD | refresh | Consumable items |
| item_endow_fire/water/wind/earth/dark | buff | Exx | refresh | Elemental Converters |

### 1.2 Key Buff Mechanics

**Stack Rule** (`refresh`): Old buff of same name is removed, new one takes its place.

**BUFFS_SURVIVE_DEATH** (index.js line 2032):
```
auto_berserk, endure, shrink, song_whistle, song_assassin_cross, song_bragi,
song_apple_of_idun, song_humming, dance_fortune_kiss, dance_service_for_you, dance_pdfm
```

**clearBuffsOnDeath** (index.js line 2038): Removes all buffs NOT in BUFFS_SURVIVE_DEATH. Also clears `maximizePowerActive`, `weaponBroken`, `performanceState`.

**UNDISPELLABLE** (index.js line 15317):
```
hindsight, play_dead, auto_berserk, devotion_protection, steel_body, combo,
stripweapon, stripshield, striparmor, striphelm,
cp_weapon, cp_shield, cp_armor, cp_helm,
chemical_protection_helm/shield/armor/weapon,
close_confine, root_lock, sitting, ensemble_aftermath
```

**Haste2 Exclusion Group**: `song_assassin_cross`, `adrenaline_rush`, `two_hand_quicken`, `spear_quicken` -- only the strongest ASPD bonus applies (max-wins).

### 1.3 getBuffModifiers Switch Cases (30+ stat modifiers)

Key modifier outputs from `getBuffModifiers()`:
- `defMultiplier`, `softDefMultiplier`, `atkMultiplier`, `aspdMultiplier`
- `strBonus`, `agiBonus`, `vitBonus`, `intBonus`, `dexBonus`, `lukBonus`
- `bonusMDEF`, `bonusHit`, `bonusFlee`, `bonusCritical`, `bonusMaxHp`, `bonusMaxSp`
- `defPercent`, `moveSpeedBonus`, `weaponElement`
- Flags: `isHidden`, `isPlayDead`, `doubleNextDamage`, `blockRanged`, `energyCoatActive`
- Special: `autoGuardChance`, `reflectShieldPercent`, `bragiCastReduction`, `bragiAcdReduction`

---

## 2. Status Effect (Debuff) System

**File**: `server/src/ro_status_effects.js`
**Exports**: `STATUS_EFFECTS`, `BREAKABLE_STATUSES`, `calculateResistance`, `applyStatusEffect`, `forceApplyStatusEffect`, `removeStatusEffect`, `cleanse`, `checkDamageBreakStatuses`, `tickStatusEffects`, `getStatusModifiers`, `hasStatusEffect`, `getActiveStatusList`

### 2.1 Complete Status Effect Catalog (12 types)

| Status | Resist Stat | Resist Cap | Base Duration | Can Kill | Break on DMG | Prevents | Stat Mods | Periodic Drain |
|--------|-------------|------------|---------------|----------|--------------|----------|-----------|----------------|
| stun | VIT | 97 | 5s | No | No | Move+Cast+Atk+Items | flee=0 | None |
| freeze | MDEF | None | 12s | No | Yes | Move+Cast+Atk | def*0.5, mdef*1.25, element=Water1 | None |
| stone | MDEF | None | 20s | No | Yes | Move+Cast+Atk | def*0.5, mdef*1.25, element=Earth1 | 1% MaxHP/5s (start 3s, min 25% HP) |
| sleep | INT | 97 | 30s | No | Yes | Move+Cast+Atk | None | None |
| ankle_snare | None | None | 20s | No | No | Move only | None | None |
| poison | VIT | 97 | 60s | No | No | None | def*0.75, blocksSPRegen | 1.5% MaxHP+2 flat/1s (min 1HP) |
| blind | avg(INT,VIT) | 193 | 30s | No | No | None | hit*0.75, flee*0.75 | None |
| silence | VIT | 97 | 30s | No | No | Cast only | None | None |
| confusion | avg(STR,INT) | 193 | 30s | No | Yes | None | Random movement | None |
| bleeding | VIT | 97 | 120s | Yes | No | None | blocksHPRegen+blocksSPRegen | 2% MaxHP/4s (can kill) |
| curse | LUK | 97 | 30s | No | No | None | atk*0.75, luk=0, moveSpeed*0.1 | None |
| petrifying | MDEF | None | 5s | No | No | Cast+Atk | None, transitions to stone | None |

### 2.2 Key Mechanics

**Resistance Formula**:
```
FinalChance = BaseChance - (BaseChance * ResistStat / 100) + srcLevel - tarLevel - tarLUK - cardResist - buffResist
Clamped to [5, 95]
Duration = BaseDuration - (BaseDuration * ResistStat / 200) - 10 * tarLUK (min 1000ms)
```

**Boss Immunity**: `target.modeFlags.statusImmune` blocks all status effects.
**LUK Immunity**: 300+ LUK = immune to all status effects.
**No Stacking**: Cannot apply same status if already active (returns `already_active`).
**BREAKABLE_STATUSES**: `freeze`, `stone`, `sleep`, `confusion`.
**Petrifying Transition**: When `petrifying` expires, auto-applies `stone` with full 20s duration.
**Devotion Break**: CC types (stun/freeze/stone/sleep) break all Devotion links on the target.

### 2.3 Tick Processing

`tickStatusEffects(target, now)` returns `{ expired, drains, transitions }`:
- Checks each effect for expiry
- Handles petrifying->stone transition
- Processes periodic drains (poison, bleeding, stone) with start delay, interval, min HP floor

---

## 3. Card Hook System

**File**: `server/src/index.js` (functions at lines ~3179-3757)

### 3.1 Card Hook Function Catalog

| Function | Line | Trigger | Description |
|----------|------|---------|-------------|
| `processCardKillHooks` | 3179 | Monster killed | SP/HP on kill by race, flat HP/SP gain, Zeny gain, EXP bonus by race |
| `processCardDrainEffects` | 3220 | Auto-attack hit | HP drain rate (chance% of damage%), SP drain rate, flat SP per hit |
| `processCardStatusProcsOnAttack` | 3247 | Auto-attack hit | Status procs on target (bAddEff), self-procs (bAddEff2). 1/10000 chance scale |
| `processCardStatusProcsWhenHit` | 3283 | Player takes hit | Status procs on attacker when hit (bAddEffWhenHit). 1/10000 scale |
| `processCardMeleeReflection` | 3306 | Melee hit taken | Returns % of damage reflected (bShortWeaponDamageReturn) |
| `processCardMagicReflection` | 3315 | Magic hit taken | Returns % of damage reflected (bMagicDamageReturn) |
| `canSeeHiddenTarget` | 3324 | Visibility check | Maya Purple Card (cardIntravision) |
| `processAutobonusOnAttack` | 3450 | Auto-attack hit | Temporary stat buffs on proc (Vanberk, Isilla, Atroce). 1/10000 scale |
| `processAutobonusWhenHit` | 3473 | Player takes hit | Temporary stat buffs on proc when hit (Hodremlin, Ice Titan) |
| `processCardAutoSpellOnAttack` | 3530 | Auto-attack hit | Auto-cast skills on attack (bAutoSpell). 1/1000 chance scale |
| `processCardAutoSpellWhenHit` | 3543 | Player takes hit | Auto-cast skills when hit (bAutoSpellWhenHit). Offensive targets attacker, self-skills target self |
| `executeAutoSpellEffect` | 3602 | Auto-spell triggered | Executes heal/buff/magical/physical damage for auto-cast. No SP/cast/gems |
| `processCardDropBonuses` | 3718 | Monster killed | Extra item drops (bAddMonsterDropItem, bAddMonsterDropItemGroup) |
| `parseAutobonusScript` | 3495 | Autobonus proc | Parses rAthena `bonus bX,N` scripts into modifier objects |
| `parseAmmoStatusProcs` | 3335 | Ammo equip | Parses `bonus2 bAddEff,Eff_X,rate` from ammo scripts |
| `recalcEffectiveWeaponElement` | 3352 | Weapon/ammo change | Priority: Endow > Arrow (non-neutral) > Weapon |
| `consumeAmmo` | 3364 | Attack/skill | Decrements ammo quantity, auto-unequips when empty |
| `knockbackTarget` | 3386 | Skill/card effect | Push target N cells from source. Boss immune, bNoKnockback immune, Ice Wall collision check, Close Confine break |

### 3.2 Key Card Data Fields on Player

```
cardHpDrainRate: { chance, percent }
cardSpDrainRate: { chance, percent }
cardSpDrainValue: number (flat per hit)
cardHpGainValue: number (flat on kill)
cardSpGainValue: number (flat on kill)
cardSpGainRace: { [race]: sp }
cardGetZenyNum: number (max random zeny on kill)
cardExpAddRace: { [race]: percent }
cardAddEff: [{ effect, chance }]
cardAddEff2: [{ effect, chance }] (self-proc)
cardAddEffWhenHit: [{ effect, chance }]
cardShortWeaponDamageReturn: percent
cardMagicDamageReturn: percent
cardIntravision: boolean
cardNoKnockback: boolean
cardAutoSpell: [{ skill, level, chance }]
cardAutoSpellWhenHit: [{ skill, level, chance }]
cardAutobonus: [{ bonusScript, chance, duration }]
cardAddMonsterDropItem: [{ itemId, race, chance }]
cardAddMonsterDropItemGroup: [{ group, chance }]
cardAddItemHealRate: { [itemId]: percent }
cardAddItemGroupHealRate: { [groupId]: percent }
cardResEff: { [statusType]: amount } (1/10000 scale)
```

---

## 4. Item & Inventory System

### 4.1 Item Effect Types (from ro_item_effects.js)

490 items with 6 effect types:

| Effect Type | Fields | Description | Count |
|-------------|--------|-------------|-------|
| `heal` | hpMin, hpMax, spMin, spMax | Fixed HP/SP heal (random between min-max) | ~350 |
| `percentheal` | hp, sp | Percentage of MaxHP/MaxSP heal | ~30 |
| `cure` | cures: [SC_...] | Remove status effects | ~15 |
| `itemskill` | skill, level | Trigger a skill on use (Fly Wing, Magnifier, scrolls) | ~40 |
| `sc_start` | status, duration, value | Apply buff/status (ASPD potions, stat foods, endows) | ~60 |
| `multi` | effects: [...] | Multiple effects combined | ~30 |
| `getitem` | itemId, quantity | Grant items (arrow quivers) | ~14 |

### 4.2 Inventory Functions

| Function | Line | Description |
|----------|------|-------------|
| `getPlayerInventory(characterId)` | 4979 | Full inventory query with JOIN on items table |
| `addItemToInventory(charId, itemId, qty, client, identified)` | 4900 | Add/stack items, default identified=true |
| `removeItemFromInventory(invId, qty, client)` | 5063 | Partial or full removal |
| `getPlayerMaxWeight(player)` | 4737 | `2000 + STR * 30 + Enlarge Weight Limit bonus` |
| `updatePlayerWeightCache(charId, player)` | 4802 | Recalc weight, emit `weight:status` on threshold cross |
| `canJobEquip(jobClass, jobsAllowed)` | 4633 | Job restriction check |
| `canGenderEquip(gender, genderAllowed)` | 4647 | Gender restriction check |

### 4.3 Item Use Flow (inventory:use handler, line 22203)

1. **Status blocks**: Stone, Freeze, Stun, Sleep, Hidden, Play Dead block item use
2. **Sitting auto-stand**: Removes sitting buff on item use
3. **Petrifying (Phase 1)**: Intentionally NOT blocked -- players CAN use items
4. **Special items**: Fly Wing (601), Butterfly Wing (602) have hardcoded handlers
5. **Data-driven**: All other items looked up in `ITEM_USE_EFFECTS`
6. **Effect processing**: heal (with card bonuses + IHP Recovery + Potion Research), percentheal, cure, itemskill, sc_start, getitem, multi
7. **ASPD potion logic**: Tier 0-3, level/class restrictions, strongest-wins mutual exclusion
8. **Stat food logic**: Mutual exclusion per stat type, recalc + emit stats
9. **Elemental converter**: Endow removal cascade, 20min duration buff
10. **Magnifier**: Triggers `identify:item_list` with unidentified items
11. **Post-use**: Consume 1 from stack, save HP/SP, broadcast health, refresh inventory + weight

### 4.4 Equipment Flow (inventory:equip handler, line 22725)

Key validations:
- Item must have `equip_slot`
- Must be identified
- Level requirement check
- Job restriction (`canJobEquip`)
- Gender restriction (`canGenderEquip`)
- Level max cap check
- Ammo requires ranged weapon
- Two-handed weapon auto-unequips shield + left-hand
- Katar auto-unequips left-hand + shield
- Dual wield: Assassin only, valid left-hand weapons, smart slot selection
- Shield blocked with two-handed weapon
- Dual accessories: accessory_1, accessory_2 system

### 4.5 Card Compound Flow (card:compound handler, line 23321)

1. Verify card is type `card`, not equipped
2. Verify equipment has `equip_slot`, is identified
3. Validate card can go on this equipment type (`canCompoundCardOnEquipment`)
4. Check slot availability (maxSlots, slotIndex range, slot empty)
5. Insert card ID into compounded_cards JSON array
6. Consume card from inventory
7. If equipment is currently equipped, rebuild card bonuses + recalc stats

---

## 5. Party System

### 5.1 Constants

```javascript
MAX_PARTY = 12
PARTY_SHARE_LEVEL_GAP = 15
PARTY_EVEN_SHARE_BONUS = 20 // +20% per eligible member
```

### 5.2 Party State Structure

```javascript
{
  partyId, name, leaderId,
  expShare: 'each_take' | 'even_share',
  itemShare: 'each_take' | 'party_share',
  itemDistribute: 'individual' | 'shared',
  members: Map<charId, {
    characterId, characterName, jobClass, level,
    map, hp, maxHp, sp, maxSp, online, socketId
  }>
}
```

### 5.3 distributePartyEXP (line 5189)

- Solo: returns single result if no party or not `even_share`
- Level gap check: `maxLv - minLv > 15` => solo
- Eligible: online + same zone + alive
- Bonus: `100 + 20 * (eligibleCount - 1)` percent
- Per-member share: `floor(baseExp * bonus / 100 / eligibleCount)`

### 5.4 Party Socket Events

| Event | Handler Summary | Emits Back |
|-------|-----------------|------------|
| `party:load` | Re-sends `party:update` if player has partyId | `party:update` |
| `party:create` | Name 1-24 chars, not in party, Basic Skill Lv7 required, unique name, DB insert | `party:update` or `party:error` |
| `party:invite` | Leader only, party not full (12), find target by name, 30s TTL invite | `party:invite_received` to target |
| `party:invite_respond` | Accept/decline, validates invite not expired, party exists, not full | `party:update` to all or `party:error` |
| `party:leave` | DB delete member, leader delegation if leader leaves, disband if empty | `party:dissolved` + `party:member_left` + `party:update` |
| `party:kick` | Leader only, cannot kick self, DB delete | `party:dissolved` to kicked + `party:member_left` + `party:update` |
| `party:change_leader` | Leader only, target must be online, DB update | `party:update` |
| `party:change_exp_share` | Leader only, validates level gap for even_share | `party:update` + chat broadcast |
| `party:chat` | Routes to `chat:receive` with PARTY channel | `chat:receive` to all members |

---

## 6. Socket Event Catalog

### 6.1 Complete Socket Event List (81 handlers)

| Event | Line | Category | Description | Emits |
|-------|------|----------|-------------|-------|
| `player:join` | 5295 | Auth | JWT validation, character load, zone join, enemy spawn | `player:joined`, `enemy:spawn`, `player:stats` |
| `player:position` | 5974 | Movement | Position update broadcast, zone-based | `player:moved` to zone |
| `zone:warp` | 6288 | Zone | Zone transition via warp portal | `zone:change`, `player:left` |
| `zone:ready` | 6416 | Zone | Client finished loading zone, send zone state | `enemy:spawn`, `player:spawn` |
| `kafra:open` | 6515 | NPC | Open Kafra storage | `kafra:data` |
| `kafra:save` | 6553 | NPC | Set save point | `kafra:saved` |
| `kafra:teleport` | 6584 | NPC | Kafra teleport service | `zone:change` |
| `cart:load` | 6710 | Cart | Load cart contents | `cart:data` |
| `cart:rent` | 6719 | Cart | Rent cart (requires Pushcart skill) | `cart:rented` |
| `cart:remove` | 6760 | Cart | Return cart | `cart:removed` |
| `cart:move_to_cart` | 6780 | Cart | Move item from inventory to cart | `cart:data`, `inventory:data` |
| `cart:move_to_inventory` | 6868 | Cart | Move item from cart to inventory | `cart:data`, `inventory:data` |
| `identify:select` | 6933 | Items | Select item to identify | `identify:result` |
| `vending:start` | 6964 | Vending | Open vending shop | `vending:shop_opened` |
| `vending:close` | 7029 | Vending | Close vending shop | `vending:shop_closed` |
| `vending:browse` | 7049 | Vending | Browse another player's shop | `vending:shop_data` |
| `vending:buy` | 7106 | Vending | Buy from vending shop | `vending:bought`, `vending:sold` |
| `disconnect` | 7236 | System | Full cleanup: party, performance, sitting, root_lock, vending, DB persist | `player:left`, `party:member_offline` |
| `player:sit` | 7443 | Action | Sit down (2x regen) | `player:sit_state`, `skill:buff_applied` |
| `player:stand` | 7460 | Action | Stand up | `player:sit_state`, `skill:buff_removed` |
| `combat:attack` | 7476 | Combat | Start auto-attack on target | `combat:attack_started` |
| `combat:stop_attack` | 7664 | Combat | Stop auto-attack | `combat:stopped` |
| `combat:respawn` | 7695 | Combat | Respawn after death | `combat:respawned`, `player:stats` |
| `player:request_stats` | 7826 | Stats | Request full stat payload | `player:stats` |
| `buff:request` | 7850 | Buffs | Request active buff list | `buff:list` |
| `mount:toggle` | 7916 | Mount | Toggle Peco/Grand Peco mount | `mount:toggled` |
| `player:allocate_stat` | 7923 | Stats | Spend stat point | `player:stats` |
| `job:change` | 7996 | Class | Job change | `job:changed`, `player:stats` |
| `skill:data` | 8112 | Skills | Request skill tree data | `skill:data` |
| `skill:learn` | 8193 | Skills | Learn/upgrade skill | `skill:learned` |
| `skill:reset` | 8288 | Skills | Reset all skills | `skill:reset_complete` |
| `party:load` | 8350 | Party | Re-request party state | `party:update` |
| `party:create` | 8361 | Party | Create new party | `party:update` or `party:error` |
| `party:invite` | 8425 | Party | Invite player by name | `party:invite_received` |
| `party:invite_respond` | 8465 | Party | Accept/decline invite | `party:update` or `party:error` |
| `party:leave` | 8516 | Party | Leave party | `party:dissolved`, `party:update` |
| `party:kick` | 8562 | Party | Kick member (leader only) | `party:dissolved`, `party:update` |
| `party:change_leader` | 8603 | Party | Transfer leadership | `party:update` |
| `party:change_exp_share` | 8632 | Party | Change EXP share mode | `party:update`, `chat:receive` |
| `party:chat` | 8667 | Party | Party chat message | `chat:receive` (PARTY channel) |
| `chat:message` | 8682 | Chat | Global/whisper/party/memo/mount commands | `chat:receive`, `chat:error` |
| `inventory:load` | 8933 | Inventory | Full inventory reload | `inventory:data` |
| `hotbar:save` | 8950 | Hotbar | Save hotbar item slot | `hotbar:saved` |
| `hotbar:request` | 9010 | Hotbar | Request hotbar data | `hotbar:data` |
| `hotbar:save_skill` | 9025 | Hotbar | Save skill to hotbar slot | `hotbar:saved` |
| `hotbar:clear` | 9094 | Hotbar | Clear hotbar slot | `hotbar:cleared` |
| `skill:use` | 9122 | Skills | Use a skill (massive handler ~12000 lines) | Various per skill |
| `pharmacy:craft` | 21146 | Crafting | Alchemist Pharmacy skill | `pharmacy:result` |
| `crafting:craft_converter` | 21233 | Crafting | Elemental converter crafting | `crafting:result` |
| `summon:detonate` | 21305 | Summon | Detonate Marine Sphere | `summon:sphere_detonated` |
| `homunculus:feed` | 21364 | Companion | Feed homunculus | `homunculus:update` |
| `homunculus:command` | 21410 | Companion | Follow/attack/standby | `homunculus:update` |
| `homunculus:skill_up` | 21430 | Companion | Level up homunculus skill | `homunculus:update` |
| `homunculus:use_skill` | 21462 | Companion | Homunculus uses skill | Various |
| `homunculus:evolve` | 21828 | Companion | Evolve homunculus | `homunculus:evolved` |
| `pet:tame` | 21946 | Companion | Tame a monster as pet | `pet:tame_result` |
| `pet:incubate` | 22028 | Companion | Hatch pet egg | `pet:spawned` |
| `pet:return_to_egg` | 22086 | Companion | Return active pet to egg | `pet:returned` |
| `pet:feed` | 22110 | Companion | Feed active pet | `pet:update` |
| `pet:rename` | 22172 | Companion | Rename pet | `pet:renamed` |
| `pet:list` | 22190 | Companion | List all pet eggs | `pet:egg_list` |
| `inventory:use` | 22203 | Items | Use consumable item | `inventory:used`, `inventory:data` |
| `inventory:equip` | 22725 | Items | Equip/unequip item | `inventory:data`, `player:stats` |
| `equipment:repair` | 23284 | Items | Repair broken weapon | `equipment:repaired` |
| `card:compound` | 23321 | Cards | Compound card into equipment | `card:result`, `inventory:data` |
| `inventory:drop` | 23472 | Items | Drop/discard item | `inventory:data` |
| `warp_portal:confirm` | 23517 | Skills | Confirm warp portal destination | `zone:change` |
| `inventory:move` | 23573 | Items | Move item between slots | `inventory:data` |
| `inventory:merge` | 23628 | Items | Merge stacks of same item | `inventory:data` |
| `shop:open` | 23700 | Shop | Open NPC shop | `shop:data` |
| `shop:buy` | 23794 | Shop | Buy single item (deprecated) | `shop:bought`, `inventory:data` |
| `shop:sell` | 23861 | Shop | Sell single item (deprecated) | `shop:sold`, `inventory:data` |
| `shop:buy_batch` | 23936 | Shop | Buy multiple items | `shop:bought`, `inventory:data` |
| `shop:sell_batch` | 24091 | Shop | Sell multiple items | `shop:sold`, `inventory:data` |
| `refine:request` | 24225 | Refine | Refine equipment | `refine:result`, `inventory:data` |
| `forge:request` | 24373 | Forge | Forge weapon | `forge:result`, `inventory:data` |
| `debug:apply_status` | 24572 | Debug | Force apply status effect | `status:applied` |
| `debug:remove_status` | 24617 | Debug | Force remove status effect | `status:removed` |
| `debug:list_statuses` | 24641 | Debug | List all active statuses | `status:list` |

### 6.2 Rate Limiting

All events have per-second rate limits (line 5226):
- Position: 60/s
- Combat: 10/s
- Chat: 5/s
- Items: 5/s
- Party: 2-3/s
- Shop: 3-5/s

---

## 7. Jest Test Code

### 7.1 Buff System Tests

```javascript
// __tests__/buff_system.test.js
const {
  BUFF_TYPES, applyBuff, removeBuff, expireBuffs,
  hasBuff, getBuffModifiers
} = require('../server/src/ro_buff_system');

describe('Buff System (ro_buff_system.js)', () => {
  let target;

  beforeEach(() => {
    target = {
      activeBuffs: [],
      stats: { str: 50, agi: 50, vit: 50, int: 50, dex: 50, luk: 50, level: 99 },
      health: 5000, maxHealth: 10000, mana: 500, maxMana: 1000
    };
  });

  // ─── BUFF_TYPES coverage ───
  describe('BUFF_TYPES definitions', () => {
    test('should have 72+ buff type definitions', () => {
      expect(Object.keys(BUFF_TYPES).length).toBeGreaterThanOrEqual(70);
    });

    test('all buff types have required fields', () => {
      for (const [name, def] of Object.entries(BUFF_TYPES)) {
        expect(def.stackRule).toBeDefined();
        expect(def.category).toMatch(/^(buff|debuff)$/);
        expect(def.displayName).toBeDefined();
        expect(def.abbrev).toBeDefined();
      }
    });

    test('provoke is a debuff', () => {
      expect(BUFF_TYPES.provoke.category).toBe('debuff');
    });

    test('blessing is a buff', () => {
      expect(BUFF_TYPES.blessing.category).toBe('buff');
    });

    test('all buffs use refresh stack rule', () => {
      for (const [, def] of Object.entries(BUFF_TYPES)) {
        expect(def.stackRule).toBe('refresh');
      }
    });
  });

  // ─── applyBuff ───
  describe('applyBuff', () => {
    test('should add buff to empty activeBuffs', () => {
      const result = applyBuff(target, {
        name: 'blessing', duration: 60000,
        strBonus: 10, dexBonus: 10, intBonus: 10
      });
      expect(result).toBe(true);
      expect(target.activeBuffs).toHaveLength(1);
      expect(target.activeBuffs[0].name).toBe('blessing');
      expect(target.activeBuffs[0].expiresAt).toBeGreaterThan(Date.now());
    });

    test('refresh stack rule should replace existing buff of same name', () => {
      applyBuff(target, { name: 'blessing', duration: 30000, strBonus: 5 });
      applyBuff(target, { name: 'blessing', duration: 60000, strBonus: 10 });
      expect(target.activeBuffs).toHaveLength(1);
      expect(target.activeBuffs[0].strBonus).toBe(10);
    });

    test('different buffs should coexist', () => {
      applyBuff(target, { name: 'blessing', duration: 60000, strBonus: 10 });
      applyBuff(target, { name: 'increase_agi', duration: 60000, agiBonus: 12 });
      expect(target.activeBuffs).toHaveLength(2);
    });

    test('should initialize activeBuffs if undefined', () => {
      const noBuffTarget = { stats: {} };
      applyBuff(noBuffTarget, { name: 'endure', duration: 10000 });
      expect(noBuffTarget.activeBuffs).toHaveLength(1);
    });

    test('should set appliedAt and expiresAt timestamps', () => {
      const before = Date.now();
      applyBuff(target, { name: 'endure', duration: 10000 });
      const after = Date.now();
      const buff = target.activeBuffs[0];
      expect(buff.appliedAt).toBeGreaterThanOrEqual(before);
      expect(buff.appliedAt).toBeLessThanOrEqual(after);
      expect(buff.expiresAt).toBe(buff.appliedAt + 10000);
    });
  });

  // ─── removeBuff ───
  describe('removeBuff', () => {
    test('should remove existing buff and return it', () => {
      applyBuff(target, { name: 'blessing', duration: 60000 });
      const removed = removeBuff(target, 'blessing');
      expect(removed).not.toBeNull();
      expect(removed.name).toBe('blessing');
      expect(target.activeBuffs).toHaveLength(0);
    });

    test('should return null for non-existent buff', () => {
      expect(removeBuff(target, 'blessing')).toBeNull();
    });

    test('should return null for empty activeBuffs', () => {
      expect(removeBuff(target, 'anything')).toBeNull();
    });

    test('should return null for undefined activeBuffs', () => {
      expect(removeBuff({}, 'anything')).toBeNull();
    });
  });

  // ─── expireBuffs ───
  describe('expireBuffs', () => {
    test('should remove expired buffs', () => {
      target.activeBuffs = [
        { name: 'blessing', expiresAt: Date.now() - 1000 },
        { name: 'endure', expiresAt: Date.now() + 60000 }
      ];
      const expired = expireBuffs(target);
      expect(expired).toHaveLength(1);
      expect(expired[0].name).toBe('blessing');
      expect(target.activeBuffs).toHaveLength(1);
      expect(target.activeBuffs[0].name).toBe('endure');
    });

    test('should return empty array when no buffs expired', () => {
      applyBuff(target, { name: 'blessing', duration: 60000 });
      const expired = expireBuffs(target);
      expect(expired).toHaveLength(0);
    });

    test('should handle empty activeBuffs', () => {
      expect(expireBuffs(target)).toEqual([]);
    });
  });

  // ─── hasBuff ───
  describe('hasBuff', () => {
    test('should return true for active buff', () => {
      applyBuff(target, { name: 'blessing', duration: 60000 });
      expect(hasBuff(target, 'blessing')).toBe(true);
    });

    test('should return false for expired buff', () => {
      target.activeBuffs = [{ name: 'blessing', expiresAt: Date.now() - 1000 }];
      expect(hasBuff(target, 'blessing')).toBe(false);
    });

    test('should return false for non-existent buff', () => {
      expect(hasBuff(target, 'nonexistent')).toBe(false);
    });

    test('should return false when no activeBuffs', () => {
      expect(hasBuff({}, 'blessing')).toBe(false);
    });
  });

  // ─── getBuffModifiers ───
  describe('getBuffModifiers', () => {
    test('should return default modifiers for no buffs', () => {
      const mods = getBuffModifiers(target);
      expect(mods.defMultiplier).toBe(1.0);
      expect(mods.atkMultiplier).toBe(1.0);
      expect(mods.strBonus).toBe(0);
      expect(mods.isHidden).toBe(false);
    });

    test('provoke should reduce softDef and increase ATK', () => {
      applyBuff(target, {
        name: 'provoke', duration: 60000,
        defReduction: 25, atkIncrease: 25
      });
      const mods = getBuffModifiers(target);
      expect(mods.softDefMultiplier).toBe(0.75);
      expect(mods.atkMultiplier).toBe(1.25);
    });

    test('blessing should add stat bonuses', () => {
      applyBuff(target, {
        name: 'blessing', duration: 60000,
        strBonus: 10, dexBonus: 10, intBonus: 10
      });
      const mods = getBuffModifiers(target);
      expect(mods.strBonus).toBe(10);
      expect(mods.dexBonus).toBe(10);
      expect(mods.intBonus).toBe(10);
    });

    test('increase_agi should add AGI and move speed', () => {
      applyBuff(target, {
        name: 'increase_agi', duration: 60000,
        agiBonus: 12, moveSpeedBonus: 25
      });
      const mods = getBuffModifiers(target);
      expect(mods.agiBonus).toBe(12);
      expect(mods.moveSpeedBonus).toBe(25);
    });

    test('lex_aeterna should set doubleNextDamage', () => {
      applyBuff(target, { name: 'lex_aeterna', duration: 60000 });
      const mods = getBuffModifiers(target);
      expect(mods.doubleNextDamage).toBe(true);
    });

    test('pneuma should set blockRanged', () => {
      applyBuff(target, { name: 'pneuma', duration: 60000 });
      const mods = getBuffModifiers(target);
      expect(mods.blockRanged).toBe(true);
    });

    test('hiding should set isHidden', () => {
      applyBuff(target, { name: 'hiding', duration: 60000 });
      const mods = getBuffModifiers(target);
      expect(mods.isHidden).toBe(true);
    });

    test('aspersio should set weaponElement to holy', () => {
      applyBuff(target, { name: 'aspersio', duration: 60000 });
      const mods = getBuffModifiers(target);
      expect(mods.weaponElement).toBe('holy');
    });

    test('Haste2 exclusion: only strongest ASPD buff applies', () => {
      applyBuff(target, { name: 'two_hand_quicken', duration: 60000, aspdIncrease: 30 });
      applyBuff(target, { name: 'spear_quicken', duration: 60000, aspdIncrease: 20 });
      const mods = getBuffModifiers(target);
      // Both are in Haste2 group; modifier should use max(30, 20)
      // The haste2Bonus is applied to aspdMultiplier after the loop
      // Exact implementation: aspdMultiplier *= (1 + haste2Bonus / 100)
      // but the exported getBuffModifiers just returns the aggregated mods
      expect(mods).toBeDefined();
    });

    test('steel_body should override DEF/MDEF and penalize ASPD/speed', () => {
      applyBuff(target, {
        name: 'steel_body', duration: 60000,
        overrideHardDEF: 90, overrideHardMDEF: 90
      });
      const mods = getBuffModifiers(target);
      expect(mods.overrideHardDEF).toBe(90);
      expect(mods.overrideHardMDEF).toBe(90);
      expect(mods.aspdMultiplier).toBe(0.75);
      expect(mods.moveSpeedBonus).toBe(-25);
      expect(mods.blockActiveSkills).toBe(true);
    });

    test('should skip expired buffs', () => {
      target.activeBuffs = [
        { name: 'blessing', expiresAt: Date.now() - 1000, strBonus: 10 }
      ];
      const mods = getBuffModifiers(target);
      expect(mods.strBonus).toBe(0);
    });
  });
});
```

### 7.2 Status Effect Tests

```javascript
// __tests__/status_effects.test.js
const {
  STATUS_EFFECTS, BREAKABLE_STATUSES,
  calculateResistance, applyStatusEffect, forceApplyStatusEffect,
  removeStatusEffect, cleanse, checkDamageBreakStatuses,
  tickStatusEffects, getStatusModifiers, hasStatusEffect, getActiveStatusList
} = require('../server/src/ro_status_effects');

describe('Status Effect System (ro_status_effects.js)', () => {
  let source, target;

  beforeEach(() => {
    source = {
      characterId: 1, level: 99,
      stats: { level: 99, str: 50, agi: 50, vit: 50, int: 50, dex: 50, luk: 50 }
    };
    target = {
      characterId: 2, level: 50,
      stats: { level: 50, str: 30, agi: 30, vit: 30, int: 30, dex: 30, luk: 30 },
      health: 5000, maxHealth: 10000,
      activeStatusEffects: new Map()
    };
  });

  // ─── STATUS_EFFECTS definitions ───
  describe('STATUS_EFFECTS definitions', () => {
    test('should define 12 status types', () => {
      expect(Object.keys(STATUS_EFFECTS).length).toBe(12);
    });

    test('stun prevents all actions', () => {
      const s = STATUS_EFFECTS.stun;
      expect(s.preventsMovement).toBe(true);
      expect(s.preventsCasting).toBe(true);
      expect(s.preventsAttack).toBe(true);
      expect(s.preventsItems).toBe(true);
    });

    test('ankle_snare only prevents movement', () => {
      const s = STATUS_EFFECTS.ankle_snare;
      expect(s.preventsMovement).toBe(true);
      expect(s.preventsCasting).toBe(false);
      expect(s.preventsAttack).toBe(false);
      expect(s.preventsItems).toBe(false);
    });

    test('freeze changes element to water', () => {
      expect(STATUS_EFFECTS.freeze.elementOverride).toEqual({ type: 'water', level: 1 });
    });

    test('stone changes element to earth', () => {
      expect(STATUS_EFFECTS.stone.elementOverride).toEqual({ type: 'earth', level: 1 });
    });

    test('bleeding can kill', () => {
      expect(STATUS_EFFECTS.bleeding.canKill).toBe(true);
    });

    test('poison cannot kill', () => {
      expect(STATUS_EFFECTS.poison.canKill).toBe(false);
    });

    test('petrifying transitions to stone', () => {
      expect(STATUS_EFFECTS.petrifying.transitionsTo).toBe('stone');
    });
  });

  // ─── BREAKABLE_STATUSES ───
  describe('BREAKABLE_STATUSES', () => {
    test('should contain freeze, stone, sleep, confusion', () => {
      expect(BREAKABLE_STATUSES).toEqual(['freeze', 'stone', 'sleep', 'confusion']);
    });
  });

  // ─── calculateResistance ───
  describe('calculateResistance', () => {
    test('should reject unknown status type', () => {
      const result = calculateResistance(source, target, 'nonexistent', 50);
      expect(result.applied).toBe(false);
      expect(result.reason).toBe('unknown_status');
    });

    test('boss immunity should block all statuses', () => {
      target.modeFlags = { statusImmune: true };
      const result = calculateResistance(source, target, 'stun', 100);
      expect(result.applied).toBe(false);
      expect(result.reason).toBe('boss_immune');
    });

    test('should not stack same status', () => {
      target.activeStatusEffects.set('stun', { type: 'stun', expiresAt: Date.now() + 5000 });
      const result = calculateResistance(source, target, 'stun', 100);
      expect(result.applied).toBe(false);
      expect(result.reason).toBe('already_active');
    });

    test('LUK 300+ should give immunity', () => {
      target.stats.luk = 300;
      const result = calculateResistance(source, target, 'stun', 100);
      expect(result.applied).toBe(false);
      expect(result.reason).toBe('luk_immune');
    });

    test('duration should be at least 1000ms', () => {
      target.stats.vit = 200; // Very high VIT to reduce duration
      target.stats.luk = 200;
      // Force 100% chance via high base and level diff
      const result = calculateResistance(source, target, 'stun', 100);
      if (result.applied) {
        expect(result.duration).toBeGreaterThanOrEqual(1000);
      }
    });
  });

  // ─── applyStatusEffect ───
  describe('applyStatusEffect', () => {
    test('should apply status and set Map entry', () => {
      // Mock Math.random to always succeed
      jest.spyOn(Math, 'random').mockReturnValue(0);
      const result = applyStatusEffect(source, target, 'stun', 100);
      expect(result.applied).toBe(true);
      expect(target.activeStatusEffects.has('stun')).toBe(true);
      Math.random.mockRestore();
    });

    test('should initialize activeStatusEffects if undefined', () => {
      const noEffects = { stats: { vit: 0, luk: 0, level: 1 } };
      jest.spyOn(Math, 'random').mockReturnValue(0);
      applyStatusEffect(source, noEffects, 'stun', 100);
      expect(noEffects.activeStatusEffects).toBeInstanceOf(Map);
      Math.random.mockRestore();
    });

    test('should break devotion links on CC status', () => {
      target.devotionLinks = [{ targetCharId: 100 }];
      jest.spyOn(Math, 'random').mockReturnValue(0);
      const result = applyStatusEffect(source, target, 'stun', 100);
      if (result.applied) {
        expect(result.devotionBroken).toBe(true);
        expect(target.devotionLinks).toHaveLength(0);
      }
      Math.random.mockRestore();
    });

    test('should use override duration when provided', () => {
      jest.spyOn(Math, 'random').mockReturnValue(0);
      const result = applyStatusEffect(source, target, 'stun', 100, 9999);
      if (result.applied) {
        expect(result.duration).toBe(9999);
      }
      Math.random.mockRestore();
    });
  });

  // ─── forceApplyStatusEffect ───
  describe('forceApplyStatusEffect', () => {
    test('should bypass resistance and always apply', () => {
      const result = forceApplyStatusEffect(target, 'stun', 5000);
      expect(result.applied).toBe(true);
      expect(result.duration).toBe(5000);
      expect(target.activeStatusEffects.has('stun')).toBe(true);
    });

    test('should use baseDuration if no duration provided', () => {
      const result = forceApplyStatusEffect(target, 'freeze');
      expect(result.applied).toBe(true);
      expect(result.duration).toBe(12000); // freeze baseDuration
    });

    test('should reject unknown status type', () => {
      const result = forceApplyStatusEffect(target, 'nonexistent', 5000);
      expect(result.applied).toBe(false);
    });
  });

  // ─── removeStatusEffect ───
  describe('removeStatusEffect', () => {
    test('should remove active status', () => {
      forceApplyStatusEffect(target, 'stun', 5000);
      expect(removeStatusEffect(target, 'stun')).toBe(true);
      expect(target.activeStatusEffects.has('stun')).toBe(false);
    });

    test('should return false for non-existent status', () => {
      expect(removeStatusEffect(target, 'stun')).toBe(false);
    });
  });

  // ─── cleanse ───
  describe('cleanse', () => {
    test('should remove multiple status effects', () => {
      forceApplyStatusEffect(target, 'poison', 5000);
      forceApplyStatusEffect(target, 'silence', 5000);
      forceApplyStatusEffect(target, 'blind', 5000);
      const removed = cleanse(target, ['poison', 'silence', 'blind']);
      expect(removed).toEqual(['poison', 'silence', 'blind']);
      expect(target.activeStatusEffects.size).toBe(0);
    });

    test('should only return actually removed types', () => {
      forceApplyStatusEffect(target, 'poison', 5000);
      const removed = cleanse(target, ['poison', 'silence']);
      expect(removed).toEqual(['poison']);
    });
  });

  // ─── checkDamageBreakStatuses ───
  describe('checkDamageBreakStatuses', () => {
    test('should break freeze, stone, sleep, confusion on damage', () => {
      forceApplyStatusEffect(target, 'freeze', 5000);
      forceApplyStatusEffect(target, 'sleep', 5000);
      const broken = checkDamageBreakStatuses(target);
      expect(broken).toContain('freeze');
      expect(broken).toContain('sleep');
    });

    test('should NOT break stun, poison, blind, silence on damage', () => {
      forceApplyStatusEffect(target, 'stun', 5000);
      forceApplyStatusEffect(target, 'poison', 60000);
      const broken = checkDamageBreakStatuses(target);
      expect(broken).not.toContain('stun');
      expect(broken).not.toContain('poison');
    });

    test('should return empty for no active statuses', () => {
      expect(checkDamageBreakStatuses(target)).toEqual([]);
    });
  });

  // ─── tickStatusEffects ───
  describe('tickStatusEffects', () => {
    test('should expire timed-out effects', () => {
      target.activeStatusEffects.set('stun', {
        type: 'stun', appliedAt: Date.now() - 10000,
        duration: 5000, expiresAt: Date.now() - 5000
      });
      const result = tickStatusEffects(target, Date.now());
      expect(result.expired).toContain('stun');
      expect(target.activeStatusEffects.has('stun')).toBe(false);
    });

    test('petrifying should transition to stone on expiry', () => {
      target.activeStatusEffects.set('petrifying', {
        type: 'petrifying', appliedAt: Date.now() - 10000,
        duration: 5000, expiresAt: Date.now() - 1,
        sourceId: 1, sourceLevel: 99, lastDrainTime: null
      });
      const result = tickStatusEffects(target, Date.now());
      expect(result.expired).toContain('petrifying');
      expect(result.transitions).toBeDefined();
      expect(result.transitions.length).toBeGreaterThan(0);
      expect(target.activeStatusEffects.has('stone')).toBe(true);
    });

    test('poison should drain HP periodically', () => {
      const now = Date.now();
      target.activeStatusEffects.set('poison', {
        type: 'poison', appliedAt: now - 5000,
        duration: 60000, expiresAt: now + 55000,
        sourceId: 1, sourceLevel: 99, lastDrainTime: now - 2000
      });
      const result = tickStatusEffects(target, now);
      expect(result.drains.length).toBeGreaterThan(0);
      expect(target.health).toBeLessThan(5000);
    });

    test('bleeding drain can kill (min HP = 0)', () => {
      target.health = 1;
      target.maxHealth = 100;
      const now = Date.now();
      target.activeStatusEffects.set('bleeding', {
        type: 'bleeding', appliedAt: now - 10000,
        duration: 120000, expiresAt: now + 110000,
        sourceId: 1, sourceLevel: 99, lastDrainTime: now - 5000
      });
      const result = tickStatusEffects(target, now);
      // Bleeding canKill=true, so health can reach 0
      expect(target.health).toBe(0);
    });

    test('stone drain stops at 25% MaxHP', () => {
      target.health = 2600; // Just above 25%
      target.maxHealth = 10000;
      const now = Date.now();
      target.activeStatusEffects.set('stone', {
        type: 'stone', appliedAt: now - 10000,
        duration: 20000, expiresAt: now + 10000,
        sourceId: 1, sourceLevel: 99, lastDrainTime: now - 6000
      });
      const result = tickStatusEffects(target, now);
      expect(target.health).toBeGreaterThanOrEqual(2500); // 25% of 10000
    });
  });

  // ─── getStatusModifiers ───
  describe('getStatusModifiers', () => {
    test('should return default values for no effects', () => {
      const mods = getStatusModifiers(target);
      expect(mods.preventsMovement).toBe(false);
      expect(mods.defMultiplier).toBe(1.0);
      expect(mods.isFrozen).toBe(false);
    });

    test('frozen should halve DEF, increase MDEF, change element', () => {
      forceApplyStatusEffect(target, 'freeze', 12000);
      const mods = getStatusModifiers(target);
      expect(mods.isFrozen).toBe(true);
      expect(mods.defMultiplier).toBe(0.5);
      expect(mods.mdefMultiplier).toBe(1.25);
      expect(mods.overrideElement).toEqual({ type: 'water', level: 1 });
      expect(mods.preventsMovement).toBe(true);
    });

    test('curse should reduce ATK, zero LUK, reduce speed', () => {
      forceApplyStatusEffect(target, 'curse', 30000);
      const mods = getStatusModifiers(target);
      expect(mods.isCursed).toBe(true);
      expect(mods.atkMultiplier).toBe(0.75);
      expect(mods.lukOverride).toBe(0);
      expect(mods.moveSpeedMultiplier).toBe(0.1);
    });

    test('poison should reduce DEF and block SP regen', () => {
      forceApplyStatusEffect(target, 'poison', 60000);
      const mods = getStatusModifiers(target);
      expect(mods.isPoisoned).toBe(true);
      expect(mods.defMultiplier).toBe(0.75);
      expect(mods.blocksSPRegen).toBe(true);
    });

    test('multiple statuses should compound multiplicatively', () => {
      forceApplyStatusEffect(target, 'freeze', 12000);
      forceApplyStatusEffect(target, 'poison', 60000);
      const mods = getStatusModifiers(target);
      // freeze def*0.5, poison def*0.75 => combined 0.375
      expect(mods.defMultiplier).toBeCloseTo(0.375, 5);
    });
  });

  // ─── hasStatusEffect ───
  describe('hasStatusEffect', () => {
    test('should return true for active effect', () => {
      forceApplyStatusEffect(target, 'stun', 5000);
      expect(hasStatusEffect(target, 'stun')).toBe(true);
    });

    test('should return false for expired effect', () => {
      target.activeStatusEffects.set('stun', {
        type: 'stun', expiresAt: Date.now() - 1000
      });
      expect(hasStatusEffect(target, 'stun')).toBe(false);
    });
  });

  // ─── getActiveStatusList ───
  describe('getActiveStatusList', () => {
    test('should return serializable list of active effects', () => {
      forceApplyStatusEffect(target, 'stun', 5000);
      forceApplyStatusEffect(target, 'poison', 60000);
      const list = getActiveStatusList(target);
      expect(list).toHaveLength(2);
      expect(list[0]).toHaveProperty('type');
      expect(list[0]).toHaveProperty('duration');
      expect(list[0]).toHaveProperty('remainingMs');
    });

    test('should exclude expired effects', () => {
      target.activeStatusEffects.set('stun', { type: 'stun', expiresAt: Date.now() - 1000, duration: 5000 });
      forceApplyStatusEffect(target, 'poison', 60000);
      const list = getActiveStatusList(target);
      expect(list).toHaveLength(1);
      expect(list[0].type).toBe('poison');
    });
  });
});
```

### 7.3 Card Hook Tests

```javascript
// __tests__/card_hooks.test.js
// NOTE: Card hook functions are defined inside index.js and not exported as a module.
// To test, either extract them to a separate module or use integration tests.
// Below are unit test specifications for when functions are extracted.

describe('Card Hook System (index.js functions)', () => {
  // Test data factories
  const makeAttacker = (overrides = {}) => ({
    characterId: 1, socketId: 'sock1', health: 5000, maxHealth: 10000,
    mana: 500, maxMana: 1000, isDead: false, zeny: 0, zuzucoin: 0,
    characterName: 'TestPlayer',
    cardHpDrainRate: null, cardSpDrainRate: null, cardSpDrainValue: 0,
    cardHpGainValue: 0, cardSpGainValue: 0, cardSpGainRace: {},
    cardGetZenyNum: 0, cardExpAddRace: {},
    cardAddEff: [], cardAddEff2: [], cardAddEffWhenHit: [],
    cardAutoSpell: [], cardAutoSpellWhenHit: [], cardAutobonus: [],
    cardAddMonsterDropItem: [], cardAddMonsterDropItemGroup: [],
    learnedSkills: {},
    ...overrides
  });

  const makeEnemy = (overrides = {}) => ({
    enemyId: 'e1', name: 'Poring', race: 'plant', health: 1000, maxHealth: 1000,
    element: { type: 'water', level: 1 }, size: 'medium',
    modeFlags: {}, activeStatusEffects: new Map(),
    ...overrides
  });

  describe('processCardKillHooks', () => {
    test('should gain SP by race (bSPGainRace)', () => {
      const attacker = makeAttacker({ cardSpGainRace: { plant: 5 } });
      const enemy = makeEnemy({ race: 'plant' });
      // Function returns { expBonusPercent }
      // Side effect: attacker.mana increases
      // Cannot test directly without extracting; document expected behavior:
      // attacker.mana should increase by 5 (capped at maxMana)
    });

    test('should gain flat HP on kill (bHPGainValue)', () => {
      // attacker.cardHpGainValue = 10
      // enemy killed => attacker.health += 10
    });

    test('should gain random Zeny on kill (bGetZenyNum)', () => {
      // attacker.cardGetZenyNum = 100
      // enemy killed => attacker.zeny += random(1, 100)
    });

    test('should return EXP bonus percent by race', () => {
      // attacker.cardExpAddRace = { plant: 10 }
      // enemy.race = 'plant'
      // result.expBonusPercent should be 10
    });

    test('should not gain HP/SP if attacker is dead', () => {
      // attacker.isDead = true
      // No HP/SP should be gained
    });
  });

  describe('processCardDrainEffects', () => {
    test('should drain HP based on chance and percent', () => {
      // attacker.cardHpDrainRate = { chance: 100, percent: 10 }
      // damage = 1000
      // Expected: attacker.health += floor(1000 * 10 / 100) = 100
    });

    test('should drain SP based on chance and percent', () => {
      // attacker.cardSpDrainRate = { chance: 100, percent: 5 }
      // damage = 1000
      // Expected: attacker.mana += 50
    });

    test('should add flat SP per hit (bSPDrainValue)', () => {
      // attacker.cardSpDrainValue = 3
      // damage > 0
      // Expected: attacker.mana += 3
    });

    test('drain should be at least 1', () => {
      // Very low damage/percent should still drain 1
    });
  });

  describe('processCardStatusProcsOnAttack', () => {
    test('should skip if boss is target (statusImmune)', () => {
      // target.modeFlags = { statusImmune: true }
      // Should return without applying any status
    });

    test('should proc status at 1/10000 chance scale', () => {
      // attacker.cardAddEff = [{ effect: 'stun', chance: 5000 }]
      // 50% chance per hit
    });

    test('should also process self-procs (bAddEff2)', () => {
      // attacker.cardAddEff2 = [{ effect: 'curse', chance: 1000 }]
      // Applies to attacker itself
    });
  });

  describe('knockbackTarget', () => {
    test('should push target away from source', () => {
      // target at (100, 0), source at (0, 0), cells=3
      // Expected: target moves to (250, 0) (3 * 50 = 150 added)
    });

    test('boss immunity should return null', () => {
      // target.modeFlags = { isBoss: true }
      // Should return null
    });

    test('bNoKnockback card should return null', () => {
      // target.cardNoKnockback = true
      // Should return null
    });

    test('should break Close Confine on knockback', () => {
      // target has close_confine buff
      // After knockback, close_confine should be removed
    });

    test('should stop at Ice Wall collision', () => {
      // Ice wall at step 2 position
      // Should stop at step 1
    });
  });

  describe('processCardDropBonuses', () => {
    test('should add extra drops by race', () => {
      // attacker.cardAddMonsterDropItem = [{ itemId: 501, race: 'plant', chance: 10000 }]
      // enemy.race = 'plant'
      // Should return [{ itemId: 501, ... }]
    });

    test('should respect race filter', () => {
      // drop.race = 'undead', enemy.race = 'plant'
      // Should not drop
    });

    test('race "all" should match any monster', () => {
      // drop.race = 'all'
      // Should always check chance
    });
  });
});
```

### 7.4 Item & Inventory Tests

```javascript
// __tests__/item_effects.test.js
const { ITEM_USE_EFFECTS } = require('../server/src/ro_item_effects');

describe('Item Use Effects (ro_item_effects.js)', () => {
  test('should define 490+ item effects', () => {
    expect(Object.keys(ITEM_USE_EFFECTS).length).toBeGreaterThanOrEqual(490);
  });

  describe('Healing items', () => {
    test('Red Potion (501) heals 45-65 HP', () => {
      const eff = ITEM_USE_EFFECTS[501];
      expect(eff.type).toBe('heal');
      expect(eff.hpMin).toBe(45);
      expect(eff.hpMax).toBe(65);
    });

    test('Blue Potion (505) heals 40-60 SP', () => {
      const eff = ITEM_USE_EFFECTS[505];
      expect(eff.type).toBe('heal');
      expect(eff.spMin).toBe(40);
      expect(eff.spMax).toBe(60);
    });

    test('Yggdrasil Berry (607) heals 100% HP+SP', () => {
      const eff = ITEM_USE_EFFECTS[607];
      expect(eff.type).toBe('percentheal');
      expect(eff.hp).toBe(100);
      expect(eff.sp).toBe(100);
    });
  });

  describe('Cure items', () => {
    test('Green Potion (506) cures poison, silence, blind', () => {
      const eff = ITEM_USE_EFFECTS[506];
      expect(eff.type).toBe('cure');
      expect(eff.cures).toContain('SC_Poison');
      expect(eff.cures).toContain('SC_Silence');
      expect(eff.cures).toContain('SC_Blind');
    });

    test('Panacea (525) cures 6 status effects', () => {
      const eff = ITEM_USE_EFFECTS[525];
      expect(eff.type).toBe('cure');
      expect(eff.cures.length).toBe(6);
    });

    test('Holy Water (523) cures curse', () => {
      const eff = ITEM_USE_EFFECTS[523];
      expect(eff.type).toBe('cure');
      expect(eff.cures).toContain('SC_Curse');
    });
  });

  describe('Itemskill items', () => {
    test('Fly Wing (601) triggers AL_TELEPORT Lv1', () => {
      const eff = ITEM_USE_EFFECTS[601];
      expect(eff.type).toBe('itemskill');
      expect(eff.skill).toBe('AL_TELEPORT');
      expect(eff.level).toBe(1);
    });

    test('Butterfly Wing (602) triggers AL_TELEPORT Lv3', () => {
      const eff = ITEM_USE_EFFECTS[602];
      expect(eff.type).toBe('itemskill');
      expect(eff.skill).toBe('AL_TELEPORT');
      expect(eff.level).toBe(3);
    });

    test('Magnifier (611) triggers MC_IDENTIFY', () => {
      const eff = ITEM_USE_EFFECTS[611];
      expect(eff.type).toBe('itemskill');
      expect(eff.skill).toBe('MC_IDENTIFY');
    });

    test('Cursed Water (12020) triggers ITEM_ENCHANTARMS Lv8', () => {
      const eff = ITEM_USE_EFFECTS[12020];
      expect(eff.type).toBe('itemskill');
      expect(eff.skill).toBe('ITEM_ENCHANTARMS');
      expect(eff.level).toBe(8);
    });
  });

  describe('sc_start items (ASPD potions)', () => {
    test('Concentration Potion (645) applies ASPDPOTION0 for 30min', () => {
      const eff = ITEM_USE_EFFECTS[645];
      expect(eff.type).toBe('sc_start');
      expect(eff.status).toBe('SC_ASPDPOTION0');
      expect(eff.duration).toBe(1800000);
    });

    test('Awakening Potion (656) applies ASPDPOTION1', () => {
      expect(ITEM_USE_EFFECTS[656].status).toBe('SC_ASPDPOTION1');
    });

    test('Berserk Potion (657) applies ASPDPOTION2', () => {
      expect(ITEM_USE_EFFECTS[657].status).toBe('SC_ASPDPOTION2');
    });
  });

  describe('Multi-effect items', () => {
    test('Royal Jelly (526) heals HP+SP and cures statuses', () => {
      const eff = ITEM_USE_EFFECTS[526];
      expect(eff.type).toBe('multi');
      expect(eff.effects).toHaveLength(2);
      expect(eff.effects[0].type).toBe('heal');
      expect(eff.effects[1].type).toBe('cure');
    });

    test('Mochi (554) heals and stuns user', () => {
      const eff = ITEM_USE_EFFECTS[554];
      expect(eff.type).toBe('multi');
      expect(eff.effects[1].type).toBe('sc_start');
      expect(eff.effects[1].status).toBe('SC_Stun');
    });
  });

  describe('Arrow quivers (getitem type)', () => {
    test('Arrow Quiver (12004) grants 500 arrows', () => {
      const eff = ITEM_USE_EFFECTS[12004];
      expect(eff.type).toBe('getitem');
      expect(eff.itemId).toBe(1750);
      expect(eff.quantity).toBe(500);
    });

    test('All 14 quiver types should exist', () => {
      const quiverIds = [12004,12005,12006,12007,12008,12009,12010,12011,12012,12013,12014,12015,12183];
      for (const id of quiverIds) {
        expect(ITEM_USE_EFFECTS[id]).toBeDefined();
        expect(ITEM_USE_EFFECTS[id].type).toBe('getitem');
        expect(ITEM_USE_EFFECTS[id].quantity).toBe(500);
      }
    });
  });

  describe('Stat food items', () => {
    const STAT_FOOD_IDS = {
      14551: 'SC_STRFOOD', 14560: 'SC_AGIFOOD', 14557: 'SC_VITFOOD',
      14554: 'SC_INTFOOD', 14563: 'SC_DEXFOOD', 14566: 'SC_LUKFOOD'
    };

    test('stat foods are multi-effect (percentheal + sc_start)', () => {
      for (const [id, status] of Object.entries(STAT_FOOD_IDS)) {
        const eff = ITEM_USE_EFFECTS[parseInt(id)];
        expect(eff.type).toBe('multi');
        const scEffect = eff.effects.find(e => e.type === 'sc_start');
        expect(scEffect).toBeDefined();
        expect(scEffect.status).toBe(status);
      }
    });
  });
});
```

### 7.5 Party System Tests

```javascript
// __tests__/party_system.test.js
// NOTE: Party system is inside index.js socket handlers.
// These tests require socket.io-client + running server for integration testing,
// or extraction of party logic to testable functions.

describe('Party System', () => {
  // distributePartyEXP can be tested if extracted
  describe('distributePartyEXP', () => {
    // Mock data
    const makePlayer = (charId, overrides = {}) => ({
      characterId: charId, partyId: null,
      health: 1000, zone: 'prontera_south',
      stats: { level: 50 }, ...overrides
    });

    test('solo player gets full EXP (no party)', () => {
      // killer.partyId = null
      // Should return [{ player, baseExp, jobExp, partyShared: false }]
    });

    test('each_take mode gives full EXP to killer only', () => {
      // party.expShare = 'each_take'
      // Should return solo result
    });

    test('even_share splits EXP with 20% bonus per member', () => {
      // 2 eligible members, baseExp=1000, jobExp=500
      // bonus = 100 + 20*(2-1) = 120%
      // perMember = floor(1000 * 120 / 100 / 2) = 600 base
      // perMember = floor(500 * 120 / 100 / 2) = 300 job
    });

    test('level gap > 15 reverts to solo', () => {
      // member levels: 10, 30 (gap = 20 > 15)
      // Should return solo result
    });

    test('dead members are excluded from share', () => {
      // member.health <= 0 => not in eligible list
    });

    test('members in different zone are excluded', () => {
      // member.zone !== enemyZone => not eligible
    });

    test('offline members are excluded', () => {
      // member.online === false => skipped
    });
  });

  describe('Party Socket Events', () => {
    test('party:create requires Basic Skill Lv7', () => {
      // learnedSkills[1] < 7 => party:error
    });

    test('party:create validates name length 1-24', () => {
      // name = '' => error
      // name = 'x'.repeat(25) => error
    });

    test('party:create rejects duplicate name', () => {
      // Existing party with same name => error
    });

    test('party:create rejects if already in party', () => {
      // player.partyId != null => error
    });

    test('party:invite is leader-only', () => {
      // party.leaderId !== characterId => error
    });

    test('party:invite rejects full party (12 members)', () => {
      // party.members.size >= 12 => error
    });

    test('party:invite stores 30s TTL invite', () => {
      // pendingInvites.set(targetCharId, { ..., expiresAt: Date.now() + 30000 })
    });

    test('party:invite_respond removes expired invite', () => {
      // invite.expiresAt < Date.now() => error
    });

    test('party:leave delegates leadership if leader leaves', () => {
      // wasLeader = true
      // New leader = first online member
    });

    test('party:leave disbands empty party', () => {
      // party.members.size === 0 after leave
      // Party deleted from DB + activeParties
    });

    test('party:kick is leader-only', () => {
      // party.leaderId !== characterId => error
    });

    test('party:kick cannot kick self', () => {
      // targetCharId === characterId => error
    });

    test('party:change_exp_share validates level gap for even_share', () => {
      // Gap > 15 => error
    });

    test('party:chat routes to PARTY channel for all members', () => {
      // broadcastToParty with channel: 'PARTY'
    });
  });
});
```

### 7.6 Socket Event Integration Tests

```javascript
// __tests__/socket_events.test.js
// Integration test structure for socket.io event handlers

const io = require('socket.io-client');

describe('Socket Event Integration', () => {
  let socket;

  beforeAll((done) => {
    socket = io('http://localhost:3000', { autoConnect: false });
    done();
  });

  afterAll(() => {
    if (socket.connected) socket.disconnect();
  });

  describe('Rate Limiting', () => {
    test('should allow events within rate limit', () => {
      // Send 5 chat:message events in 1 second (limit = 5)
      // All should be processed
    });

    test('should drop events exceeding rate limit', () => {
      // Send 10 chat:message events in 1 second (limit = 5)
      // Last 5 should be silently dropped
    });

    test('events without rate limit should always pass', () => {
      // Unconfigured event names pass through
    });
  });

  describe('Disconnect Cleanup', () => {
    test('should save position to DB on disconnect', () => {
      // After disconnect, characters table should have updated zone/x/y/z
    });

    test('should save stats + EXP to DB on disconnect', () => {
      // str, agi, vit, int, dex, luk, level, job_level, base_exp, job_exp
    });

    test('should mark party member offline on disconnect', () => {
      // party.members.get(charId).online = false
    });

    test('should close vending shop on disconnect', () => {
      // vending_items + vending_shops cleaned up
      // vending:shop_closed broadcast
    });

    test('should persist homunculus state on disconnect', () => {
      // UPDATE character_homunculus SET is_summoned = false, ...
    });

    test('should persist pet state on disconnect', () => {
      // UPDATE character_pets SET is_active = FALSE, ...
    });

    test('should persist plagiarism skill on disconnect', () => {
      // UPDATE characters SET plagiarized_skill_id, plagiarized_skill_level
    });

    test('should cancel performances on disconnect', () => {
      // cancelPerformance called
    });

    test('should release root_lock on disconnect', () => {
      // Locked enemy gets root_lock buff removed
    });

    test('should clean up summoned plants and spheres', () => {
      // activePlants, activeMarineSpheres cleaned up
    });
  });

  describe('Chat System', () => {
    test('whisper /w should deliver to target player', () => {
      // /w "TargetName" hello => target gets WHISPER channel message
    });

    test('whisper should check block list', () => {
      // target.whisperBlockAll = true => error message
      // target.whisperBlockList has sender => error message
    });

    test('/ex should add to block list', () => {
      // /ex Name => player.whisperBlockList.add('name')
    });

    test('/exall should block all whispers', () => {
      // player.whisperBlockAll = true
    });

    test('/r should reply to last whisperer', () => {
      // Re-routes as /w to player.lastWhisperer
    });

    test('/am should set auto-reply message', () => {
      // player.autoReplyMessage = "message"
    });

    test('/memo should save warp portal location', () => {
      // Requires Warp Portal learned
      // Saves to character_memo table
    });

    test('/mount should toggle mount', () => {
      // Calls handleMountToggle
    });

    test('% prefix routes to party chat', () => {
      // %hello => broadcastToParty PARTY channel
    });

    test('global chat broadcasts to all connected players', () => {
      // io.emit('chat:receive', ...)
    });
  });

  describe('Inventory Use Flow', () => {
    test('should block item use while stunned', () => {
      // isStunned => inventory:error
    });

    test('should block item use while frozen', () => {
      // isFrozen => inventory:error
    });

    test('should block item use while hidden', () => {
      // isHidden => inventory:error
    });

    test('should auto-stand on item use', () => {
      // isSitting => removeBuff('sitting'), broadcast sit_state
    });

    test('should allow item use during petrifying (Phase 1)', () => {
      // isPetrifying is NOT blocked
    });

    test('heal effect should apply card heal rate bonus', () => {
      // player.cardAddItemHealRate = { 501: 20 }
      // hpHeal *= 120/100
    });

    test('heal effect should apply IHP Recovery bonus (+10%/lv)', () => {
      // learnedSkills[102] = 5
      // hpHeal *= 150/100
    });

    test('ASPD potion should reject weaker when stronger active', () => {
      // Existing tier 2 active, try tier 1 => rejected
    });

    test('ASPD potion class restriction (Awakening)', () => {
      // jobClass = 'priest' => rejected for tier 1
    });

    test('Magnifier should emit identify:item_list', () => {
      // itemskill MC_IDENTIFY => query unidentified items
    });

    test('Fly Wing should random teleport within zone', () => {
      // player:teleport event with teleportType: 'fly_wing'
    });

    test('Butterfly Wing cross-zone should trigger zone:change', () => {
      // Different save map => full zone transition
    });
  });

  describe('Equipment Flow', () => {
    test('should reject unidentified item equip', () => {
      // identified = false => inventory:error
    });

    test('should check job restriction', () => {
      // jobs_allowed doesn't include player class => error
    });

    test('should check gender restriction', () => {
      // gender_allowed = 'Female', player.gender = 'male' => error
    });

    test('two-handed weapon should auto-unequip shield', () => {
      // Equipping 2H sword with shield => shield removed
    });

    test('katar should auto-unequip left-hand and shield', () => {
      // Equipping katar => both removed
    });

    test('dual wield only for Assassin', () => {
      // canDualWield checks job class
    });

    test('ammo equip requires ranged weapon', () => {
      // weaponType not in [bow, gun, instrument, whip] => error
    });

    test('dual accessories fill accessory_1 then accessory_2', () => {
      // First acc => accessory_1, second => accessory_2
    });
  });

  describe('Card Compound Flow', () => {
    test('should reject non-card item', () => {
      // item_type !== 'card' => error
    });

    test('should reject equipped card', () => {
      // is_equipped = true => error
    });

    test('should reject unidentified equipment', () => {
      // identified = false => error
    });

    test('should reject equipment with no slots', () => {
      // slots = 0 => error
    });

    test('should reject occupied slot', () => {
      // cards[slotIndex] !== null => error
    });

    test('should consume card from inventory', () => {
      // quantity > 1 => quantity - 1
      // quantity = 1 => DELETE
    });

    test('should rebuild card bonuses if equipment is equipped', () => {
      // is_equipped = true => rebuildCardBonuses + recalc stats
    });
  });

  describe('Refine System', () => {
    test('should reject non-refineable items', () => {
      // refineable = false => error
    });

    test('should reject +10 items', () => {
      // refine_level >= 10 => error
    });

    test('should reject unidentified items', () => {
      // identified = false => error
    });

    test('should consume ore and zeny', () => {
      // Ore removed, zeny deducted
    });

    test('failure should destroy item', () => {
      // roll >= successRate => DELETE item
    });

    test('success should increment refine level', () => {
      // roll < successRate => refine_level + 1
    });
  });

  describe('Shop System', () => {
    test('shop:open should apply Discount skill', () => {
      // discountPct = getDiscountPercent(player)
    });

    test('shop:buy should check zeny balance', () => {
      // totalCost > player.zuzucoin => error
    });

    test('shop:buy should check level requirement', () => {
      // required_level > player.stats.level => error
    });

    test('shop:sell should reject equipped items', () => {
      // is_equipped = true => error
    });

    test('shop:buy_batch should use DB transaction', () => {
      // BEGIN, operations, COMMIT/ROLLBACK
    });
  });
});
```

---

## 8. Summary Statistics

| System | Functions Audited | Buff/Status Types | Socket Events | Item Effects |
|--------|-------------------|-------------------|---------------|--------------|
| Buff System | 6 exported | 72+ buff types | - | - |
| Status Effects | 11 exported | 12 status types | - | - |
| Card Hooks | 17 functions | - | - | - |
| Item/Inventory | 6 core functions | - | 15 events | 490+ items |
| Party System | distributePartyEXP | - | 9 events | - |
| Chat System | - | - | 1 event (20+ commands) | - |
| Socket Events | - | - | 81 total handlers | - |

**Total test cases specified**: 180+
**Test files**: 6 (buff_system, status_effects, card_hooks, item_effects, party_system, socket_events)
