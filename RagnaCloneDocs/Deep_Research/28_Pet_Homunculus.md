# Pet & Homunculus Systems -- Deep Research (Pre-Renewal)

> Comprehensive reference for replicating RO Classic (pre-Renewal) Pet and Homunculus systems.
> All formulas, stats, and mechanics verified against rAthena pre-re source (`pet.cpp`, `pet_db.yml`, `homunculus.cpp`, `homunculus_db.txt`), iRO Wiki Classic, RateMyServer, and community guides.
>
> Sources:
> - [rAthena pet_db.yml (pre-re)](https://github.com/rathena/rathena/blob/master/db/pre-re/pet_db.yml)
> - [rAthena pet.cpp](https://github.com/rathena/rathena/blob/master/src/map/pet.cpp)
> - [rAthena homunculus.cpp](https://github.com/rathena/rathena/blob/master/src/map/homunculus.cpp)
> - [rAthena homunculus_db.txt](https://github.com/flaviojs/rathena-commits/blob/master/db/homunculus_db.txt)
> - [iRO Wiki Classic Pet System](https://irowiki.org/classic/Pet_System)
> - [iRO Wiki Classic Homunculus System](https://irowiki.org/classic/Homunculus_System)
> - [iRO Wiki Cute Pet System](https://irowiki.org/wiki/Cute_Pet_System)
> - [iRO Wiki Homunculus System](https://irowiki.org/wiki/Homunculus_System)
> - [RateMyServer Pet Guide](https://write.ratemyserver.net/ragnoark-online-how-to/pet-guides/)
> - [RateMyServer Homunculus Bible](https://write.ratemyserver.net/ragnoark-online-tips-and-tricks/the-big-fat-homunculus-bible/)
> - [RateMyServer Homunculus EXP Table](https://ratemyserver.net/index.php?page=misc_table_exp&op=20)
> - [Ragnarok Fandom Wiki - Cute Pet System](https://ragnarok.fandom.com/wiki/Cute_Pet_System)
> - [ROGGH Homunculus Guide](https://roggh.com/homunculus-and-homunculus-s-guide/)

---

## PET SYSTEM

### Overview (Taming, Ownership, Following)

The Cute Pet System allows players to tame specific monsters and keep them as non-combat companions. Pets:
- Follow the player at 2-3 cell distance
- Provide stat bonuses at high intimacy (Cordial/Loyal)
- Can wear a unique accessory that changes their appearance
- Exist as Pet Egg items in inventory when not summoned
- Display emotes and idle animations based on intimacy level
- Do NOT attack monsters -- they are purely cosmetic/stat companions
- Only ONE pet can be active (summoned) at a time
- Teleport to owner if distance exceeds ~15 cells

**Lifecycle:** Taming Item -> Pet Egg (in inventory) -> Pet Incubator (hatches egg) -> Active Pet -> Return to Egg (preserves all state)

---

### Taming System (Taming Items, Catch Rate Formula, Monster Restrictions)

**Process:**
1. Player must have a Taming Item specific to the target monster in inventory
2. Player targets a tameable monster on screen
3. Player double-clicks the Taming Item in inventory
4. A slot machine mini-game animation plays (purely visual -- outcome is server-determined)
5. On success: monster is removed from world, a Pet Egg item is added to inventory
6. On failure: nothing happens
7. The Taming Item is consumed regardless of success or failure

**Restrictions:**
- Boss/MVP monsters cannot be tamed
- Each monster can only be tamed by its specific Taming Item
- Monster must be alive and visible on screen
- Only one pet active at a time

**Capture Rate Formula (rAthena `pet.cpp`):**

```
CaptureRate = (BaseCaptureRate + (PlayerBaseLv - MonsterLv) * 30 + PlayerLUK * 20)
              * (200 - MonsterHPPercent) / 100
```

Where:
- `BaseCaptureRate`: per-pet value from pet_db (10000 = 100%)
- `MonsterHPPercent`: `floor((currentHP / maxHP) * 100)`
- Result is clamped to minimum 1 (always at least a tiny chance)
- Lower monster HP = higher capture rate (at 1% HP, multiplier is ~1.99x)

**Examples:**
- Poring (base 2000/10000 = 20%): At full HP = 20%, at 50% HP = 30%, at 1% HP = ~39.8%
- Baphomet Jr. (base 200/10000 = 2%): At full HP = 2%, at 50% HP = 3%, at 1% HP = ~3.98%

---

### Complete Tameable Monster List

56 pre-renewal pets. All data from rAthena `db/pre-re/pet_db.yml`.

**Capture Rate scale:** 10000 = 100%. So 2000 = 20%, 500 = 5%, etc.

#### Common Pets (Easy Capture, Starter Pets)

| MobID | Monster | Taming Item (ID) | Food (ID) | Accessory (ID) | Capture Rate | Hunger Decay/min | Loyal Bonus |
|-------|---------|-------------------|-----------|-----------------|-------------|------------------|-------------|
| 1002 | Poring | Unripe Apple (619) | Apple Juice (531) | Backpack (10013) | 2000 (20%) | 3 | LUK+2, CRIT+1 |
| 1113 | Drops | Orange Juice (620) | Yellow Herb (508) | Backpack (10013) | 1500 (15%) | 3 | HIT+3, ATK+3 |
| 1031 | Poporing | Bitter Herb (621) | Green Herb (511) | Backpack (10013) | 1000 (10%) | 2 | LUK+2, Poison Resist+10% |
| 1063 | Lunatic | Rainbow Carrot (622) | Carrot Juice (534) | Silk Ribbon (10007) | 1500 (15%) | 3 | CRIT+2, ATK+2 |
| 1049 | Picky | Earthworm the Dude (623) | Red Herb (507) | Tiny Egg Shell (10012) | 2000 (20%) | 3 | STR+1, ATK+5 |
| 1011 | Chonchon | Rotten Fish (624) | Pet Food (537) | Monster Oxygen Mask (10002) | 1500 (15%) | 3 | AGI+1, FLEE+2 |
| 1042 | Steel Chonchon | Rusty Iron (625) | Iron Ore (1002) | Monster Oxygen Mask (10002) | 1000 (10%) | 2 | FLEE+6, AGI-1 |
| 1035 | Hunter Fly | Monster Juice (626) | Red Gemstone (716) | Monster Oxygen Mask (10002) | 500 (5%) | 2 | FLEE-5, Perfect Dodge+2 |
| 1167 | Savage Babe | Sweet Milk (627) | Pet Food (537) | Green Lace (10015) | 1500 (15%) | 4 | VIT+1, MaxHP+50 |
| 1107 | Baby Desert Wolf | Well-Dried Bone (628) | Pet Food (537) | Transparent Head Protector (10003) | 1000 (10%) | 3 | INT+1, MaxSP+50 |
| 1052 | Rocker | Singing Flower (629) | Pet Food (537) | Rocker Glasses (10014) | 1500 (15%) | 3 | HP Regen+5%, MaxHP+25 |
| 1014 | Spore | Dew Laden Moss (630) | Pet Food (537) | Bark Shorts (10017) | 1500 (15%) | 2 | HIT+5, ATK-2 |
| 1077 | Poison Spore | Deadly Noxious Herb (631) | Pet Food (537) | Bark Shorts (10017) | 1000 (10%) | 2 | STR+1, INT+1 |
| 1019 | Peco Peco | Fat Chubby Earthworm (632) | Pet Food (537) | Battered Pot (10010) | 1000 (10%) | 3 | MaxHP+150, MaxSP-10 |
| 1056 | Smokie | Sweet Potato (633) | Pet Food (537) | Red Scarf (10019) | 1000 (10%) | 2 | AGI+1, Perfect Dodge+1 |

#### Moderate Pets (Medium Capture, Useful Bonuses)

| MobID | Monster | Taming Item (ID) | Food (ID) | Accessory (ID) | Capture Rate | Hunger Decay/min | Loyal Bonus |
|-------|---------|-------------------|-----------|-----------------|-------------|------------------|-------------|
| 1057 | Yoyo | Tropical Banana (634) | Banana Juice (532) | Monkey Circlet (10018) | 1000 (10%) | 3 | CRIT+3, LUK-1 |
| 1023 | Orc Warrior | Orc Trophy (635) | Pet Food (537) | Wild Flower (10009) | 500 (5%) | 3 | ATK+10, DEF-3 |
| 1026 | Munak | No Recipient (636) | Pet Food (537) | Punisher (10008) | 500 (5%) | 2 | INT+1, DEF+1 |
| 1028 | Dokebi | Old Broom (637) | Pet Food (537) | Wig (10005) | 500 (5%) | 2 | MATK+1%, ATK-1% |
| 1170 | Sohee | Silver Knife of Chastity (638) | Pet Food (537) | Golden Bell (10016) | 500 (5%) | 2 | STR+1, DEX+1 |
| 1029 | Isis | Armlet of Obedience (639) | Pet Food (537) | Queen's Hair Ornament (10006) | 500 (5%) | 2 | ATK+1%, MATK-1% |
| 1155 | Green Petite | Shining Stone (640) | Pet Food (537) | Stellar Hairpin (10011) | 500 (5%) | 2 | DEF/MDEF-2, ASPD+1% |
| 1188 | Bongun | Her Heart (659) | Pet Food (537) | Grave Keeper's Sword (10020) | 500 (5%) | 3 | VIT+1, Stun Resist+1% |

#### Rare Pets (Hard Capture, Powerful Bonuses)

| MobID | Monster | Taming Item (ID) | Food (ID) | Accessory (ID) | Capture Rate | Hunger Decay/min | Loyal Bonus |
|-------|---------|-------------------|-----------|-----------------|-------------|------------------|-------------|
| 1109 | Deviruchi | Contract in Shadow (641) | Shoot (711) | Pacifier (10004) | 500 (5%) | 2 | ATK/MATK+1%, MaxHP/MaxSP-3% |
| 1101 | Baphomet Jr. | Book of the Devil (642) | Honey (518) | Skull Helm (10001) | 200 (2%) | 2 | DEF+1, MDEF+1 |
| 1200 | Zealotus | Forbidden Red Candle (660) | Immortal Heart (929) | None | 300 (3%) | 1 | Demi-Human ATK+2% |
| 1275 | Alice | Soft Apron (661) | White Potion (504) | None | 800 (8%) | 2 | MDEF+1, Demi-Human Resist+1% |
| 1179 | Whisper | Ghost Bandana (12363) | Spirit Liquor (6100) | Whisper Mask (10027) | 500 (5%) | 2 | FLEE+7, DEF-3 |
| 1040 | Golem | Magical Lithography (12371) | Mystic Frozen (6111) | Iron Wrist (10035) | 500 (5%) | 2 | MaxHP+100, FLEE-5 |

#### Late-Game / Special Pets

| MobID | Monster | Taming Item (ID) | Food (ID) | Accessory (ID) | Capture Rate | Hunger Decay/min | Loyal Bonus |
|-------|---------|-------------------|-----------|-----------------|-------------|------------------|-------------|
| 1370 | Succubus | Boy's Pure Heart (12373) | Vital Flower (6113) | Black Butterfly Mask (10037) | 200 (2%) | 1 | 5% chance HP drain on attack |
| 1374 | Incubus | Girl's Naivety (12370) | Vital Flower (6110) | Incubus Horn (10034) | 50 (0.5%) | 1 | MaxSP+3% |
| 1379 | Nightmare Terror | Hell Contract (12372) | Fresh Plant (6112) | Hell Horn (10036) | 200 (2%) | 1 | Sleep Immunity |
| 1299 | Goblin Leader | Staff of Leader (12364) | Big Cell (6104) | Crown of Leader (10028) | 50 (0.5%) | 1 | Demi-Human DMG+3% |
| 1504 | Dullahan | Sealed Book (12367) | Heavy Arrow (6107) | Masquerade (10031) | 200 (2%) | 1 | CRIT DMG+5% |
| 1148 | Medusa | Splendid Mirror (12368) | Apple (6108) | Queen's Coronet (10032) | 200 (2%) | 1 | VIT+1, Stone Resist+5% |
| 1495 | Stone Shooter | Glistening Coat (12369) | Plant Bottle (6109) | Egg Shell (10033) | 500 (5%) | 2 | Fire Resist+3% |
| 1837 | Imp | Ice Fireworks (12374) | Flame Gemstone (6114) | Horn Barrier (10038) | 200 (2%) | 1 | Fire Resist+2%, Fire DMG+1% |

#### Japanese/Korean Expansion Pets

| MobID | Monster | Taming Item (ID) | Food (ID) | Accessory (ID) | Capture Rate | Loyal Bonus |
|-------|---------|-------------------|-----------|-----------------|-------------|-------------|
| 1208 | Wanderer | Vagabond's Skull (14574) | Spirit Liquor (7824) | None | 800 (8%) | AGI+3, DEX+1 |
| 1143 | Marionette | Puppet String (12361) | Star Candy (6098) | Star Clip (10025) | 500 (5%) | SP Regen+3% |
| 1401 | Shinobi | Ninja Scroll (12362) | Rice Ball (6099) | Shinobi Belt (10026) | 500 (5%) | AGI+2 |
| 1564 | Wicked Nymph | Moonlight Pendant (12365) | Morning Dew (6105) | Fairy Bubble (10029) | 500 (5%) | MaxSP+30, SP Regen+5% |
| 1404 | Miyabi Ningyo | Girl's Diary (12366) | Well-Baked Cookie (6106) | Summer Fan (10030) | 200 (2%) | INT+1, Cast Time-3% |
| 1505 | Loli Ruri | Pumpkin Candle (12360) | Small Snow Flower (6097) | Fairy's Leaf (10024) | 200 (2%) | MaxHP+3% |
| 1630 | Bacsojin | Shiny Feather (12357) | Elixir (6094) | Hyper Chick Hat (10021) | 300 (3%) | (No bonus) |
| 1513 | Mao Guai | Fan of Wind (12358) | Steamed Bun (6095) | Dragon Scale Scarf (10022) | 500 (5%) | MaxSP+10 |
| 1586 | Leaf Cat | Tiny Leaf (12359) | Fish Tail (6096) | Leaf Cat Ball (10023) | 200 (2%) | Brute Resist+3% |

#### Event / Goblin Variant Pets

| MobID | Monster | Taming Item (ID) | Food (ID) | Capture Rate | Loyal Bonus |
|-------|---------|-------------------|-----------|-------------|-------------|
| 1815 | Rice Cake | (Event) | Green Herb (511) | 2000 (20%) | Neutral Resist+1%, MaxHP-1% |
| 1245 | Christmas Goblin | Sweet Candy Cane (12225) | Scell (911) | 2000 (20%) | MaxHP+30, Water Resist+1% |
| 1519 | Green Maiden | Tantan Noodles (12395) | Bun (6115) | 2000 (20%) | DEF+1, Demi-Human Resist+1% |
| 1122 | Knife Goblin | Knife Goblin Ring (14569) | Goblin BBQ (7821) | 800 (8%) | (No bonus) |
| 1123 | Flail Goblin | Flail Goblin Ring (14570) | Goblin BBQ (7821) | 800 (8%) | (No bonus) |
| 1125 | Hammer Goblin | Hammer Goblin Ring (14571) | Goblin BBQ (7821) | 800 (8%) | (No bonus) |
| 1384 | Red Deleter | Deleter Egg (14572) | Flame Heart (7822) | 800 (8%) | (No bonus) |
| 1382 | Diabolic | Evil Horn (14573) | Diabolic Lollipop (7823) | 800 (8%) | (No bonus) |

---

### Feeding System

**Hunger Bar:** 0-100

| Status | Range | Feeding Effect |
|--------|-------|----------------|
| Stuffed | 91-100 | **OVERFEED** -- intimacy decreases by 100 per feed |
| Satisfied | 76-90 | **OVERFEED** -- intimacy decreases by 100 per feed |
| Neutral | 26-75 | Moderate intimacy gain (75% of base) |
| Hungry | 11-25 | **OPTIMAL** -- full intimacy gain per feed |
| Very Hungry | 0-10 | Reduced intimacy gain (50% of base), starvation begins |

**Feeding mechanics:**
- Each feeding increases hunger by **20 points** (`PET_HUNGER.FEED_AMOUNT`)
- Each pet has a specific food item that must be used (not interchangeable)
- "Pet Food" (item 537) is used by many pets but is a specific item, not a universal food

**Overfeeding escalation:**
1. First overfeed: Pet shows `/ok` emote -- intimacy -100
2. Second consecutive overfeed: Pet shows `/hmm` or `/pif` emote -- intimacy -100
3. Third consecutive overfeed: Pet shows `/omg` emote -- **pet runs away permanently** (deleted)

**Food items by type:**
- Apple Juice (531): Poring
- Yellow Herb (508): Drops
- Green Herb (511): Poporing, Rice Cake
- Carrot Juice (534): Lunatic
- Red Herb (507): Picky
- Pet Food (537): Chonchon, Savage Babe, Baby Desert Wolf, Rocker, Spore, Poison Spore, Peco Peco, Smokie, Orc Warrior, Munak, Dokebi, Sohee, Isis, Green Petite, Bongun
- Iron Ore (1002): Steel Chonchon
- Red Gemstone (716): Hunter Fly
- Banana Juice (532): Yoyo
- Shoot (711): Deviruchi
- Honey (518): Baphomet Jr.
- Immortal Heart (929): Zealotus
- White Potion (504): Alice
- Scell (911): Christmas Goblin

---

### Intimacy System

**Intimacy Range:** 0-1000

| Status | Range | Behavior |
|--------|-------|----------|
| Awkward | 0-99 | May flee; unhappy emotes; no bonuses |
| Shy | 100-249 | Uses unhappy emotes; no bonuses |
| Neutral | 250-749 | Friendly emotes; no bonuses |
| Cordial | 750-909 | Affectionate emotes; **stat bonuses active** |
| Loyal | 910-1000 | Talks often; **full stat bonuses active** |

**Initial intimacy:** Default 250 for most pets (within the Neutral range). rAthena uses `startIntimacy` per pet entry (all default to 250). iRO Wiki states random 100-399 when first hatched.

**Intimacy gain from feeding (per-pet configurable):**
- `intimacyFed`: Base intimacy gain when fed in "Hungry" zone (11-25 hunger). Ranges from 10-50 per pet.
- At "Very Hungry" (0-10): 50% of `intimacyFed`
- At "Neutral" (26-75): 75% of `intimacyFed`
- At "Satisfied" / "Stuffed" (76-100): `intimacyOverfed` (typically -100)

**Intimacy loss:**
- Owner death (unevolved pet): **-20** (`intimacyOwnerDie`)
- Owner death (evolved pet): **-1**
- Overfeeding: **-100** per overfeed (`intimacyOverfed`)
- Starvation (hunger = 0): **-20 every 20 seconds**
- Returning pet to egg: **No loss**
- Trading pet egg: **Intimacy resets** in most implementations

**Intimacy to Loyal timeline (at optimal feeding):**
- Starting at 250, need 660 more points to reach Loyal (910)
- With `intimacyFed` = 50 (Poring), ~13 feedings at optimal timing
- With `intimacyFed` = 10 (Sohee), ~66 feedings at optimal timing
- At one feed per hunger cycle, this can take hours to days depending on the pet

---

### Pet Bonuses (Stat Bonuses at Loyal Intimacy, Per Pet Type)

Pet bonuses activate at **Cordial (750+)** intimacy and are at full strength at **Loyal (910+)**. In classic pre-renewal, bonuses are binary: active when Cordial+ or inactive when below.

**Bonus types implemented in the system:**

| Bonus Key | Effect |
|-----------|--------|
| `str`, `agi`, `vit`, `int`, `dex`, `luk` | Flat stat bonus |
| `atk` | Flat ATK bonus |
| `hit` | Flat HIT bonus |
| `flee` | Flat FLEE bonus |
| `def`, `mdef` | Flat DEF/MDEF bonus |
| `critical` | Flat CRIT bonus |
| `perfectDodge` | Flat Perfect Dodge bonus |
| `maxHP`, `maxSP` | Flat MaxHP/MaxSP bonus |
| `maxHPPercent`, `maxSPPercent` | Percentage MaxHP/MaxSP bonus |
| `atkPercent`, `matkPercent` | Percentage ATK/MATK bonus |
| `aspdPercent` | Percentage ASPD bonus |
| `hpRegenPercent`, `spRegenPercent` | Percentage HP/SP regen bonus |
| `poisonResist`, `fireResist`, `waterResist`, `neutralResist` | Elemental resist |
| `demiHumanResist`, `bruteResist` | Race resist |
| `stunResist`, `stoneResist`, `sleepImmune` | Status resist |
| `castTimePercent` | Cast time reduction (negative = faster) |
| `critDmgPercent` | Critical damage bonus |
| `hpDrainChance`, `hpDrainAmount` | HP drain on auto-attack |
| `raceDemiHumanPercent`, `raceDemiHumanDmg`, `fireDmgPercent` | Offensive race/element damage |

**Most impactful pets for combat:**
1. **Orc Warrior**: ATK+10, DEF-3 (pure physical attackers)
2. **Peco Peco**: MaxHP+150 (tanks, pre-cast survival)
3. **Golem**: MaxHP+100, FLEE-5 (pure HP tank)
4. **Succubus**: 5% HP drain on attack (sustain)
5. **Goblin Leader**: Demi-Human DMG+3% (PvP)
6. **Dullahan**: CRIT DMG+5% (crit builds)

---

### Pet Equipment/Accessories

Each pet has a unique equippable accessory. Not all pets have accessories (some have `equipItemId: 0`).

**Accessory mechanics:**
- One accessory per pet
- Changes the pet's visual appearance/sprite
- Obtained from monster drops, quests, or NPC shops
- Stays equipped when pet is returned to egg
- Can be unequipped via pet command menu

**In pre-renewal, accessories are primarily cosmetic.** The stat bonuses come from the pet's intimacy level, not from the accessory itself. However, some servers implement additional accessory-specific bonuses as "pet support scripts" (e.g., Smokie's Red Scarf granting Perfect Hide, Sohee's Golden Bell healing owner when HP < 33%).

**Accessory support scripts (from rAthena `pet_db.yml` SupportScript field -- server-configurable):**

| Pet | Accessory | Support Script Effect |
|-----|-----------|---------------------|
| Smokie | Red Scarf | Owner can use Perfect Hide (undetectable except by Boss) |
| Sohee | Golden Bell | Heals owner 400 HP when owner HP < 33%, 60s cooldown |
| Isis | Queen's Hair Ornament | Casts Magnificat Lv2 every 60s when owner HP & SP < 50% |
| Peco Peco | Battered Pot | +25% movement speed for 20s every 20s |
| Rocker | Rocker Glasses | All Stats +1 for 10s every 50s |
| Picky | Tiny Egg Shell | STR+3 for 10s every 50s |
| Lunatic | Silk Ribbon | LUK+3 for 10s every 50s |

> **Implementation Note:** Support scripts are optional -- many private servers and Classic-focused servers do not implement them, using only the base Loyal bonuses. For a faithful pre-renewal implementation, the Loyal bonuses are the primary system; support scripts are an enhancement.

---

### Pet Performance (Random Actions Based on Intimacy)

Pets periodically perform random emote/action animations based on their current intimacy level:

| Intimacy Level | Emotes / Actions |
|---------------|-----------------|
| Awkward (0-99) | Angry face, slumping, "..." bubble |
| Shy (100-249) | Frowning, turning away, "?" bubble |
| Neutral (250-749) | Nodding, smiling, "!" bubble |
| Cordial (750-909) | Hearts, dancing, happy face |
| Loyal (910-1000) | Multiple hearts, jumping, talking |

Performance frequency increases with intimacy. Players can also trigger a "Performance" command to request an emote, with a small intimacy gain at certain levels.

---

### Pet Evolution (Renewal Feature -- Documented for Future Reference)

Pet evolution is a **Renewal** feature, NOT part of Classic pre-Renewal. Documented here for future planning.

**Requirements for evolution:**
- Pet must be at **Loyal** intimacy (910+)
- Pet must be summoned (active)
- Specific items are required per pet (varies)

**Known evolution paths (from rAthena `pet_db.yml` EvolveData):**

| Base Pet | Evolved Form | Required Items |
|----------|-------------|----------------|
| Poring (1002) | Mastering | 3 Unripe Apple + 3 Leaf of Yggdrasil + 100 Sticky Mucus + Poring Card |
| Lunatic (1063) | Leaf Lunatic | 3 Rainbow Carrot + 777 Grasshopper Leg + 200 Yellow Herb + Metaller Card |
| Drops (1113) | Sweet Drops | (specific items, varies by server) |
| Dokebi (1028) | Cat o' Nine Tails | (specific items) |
| Isis (1029) | Evolved Isis | (specific items) |
| Yoyo (1057) | Choco | (specific items) |
| Smokie (1056) | Evolved Smokie | (specific items) |
| Rocker (1052) | Metaller | 3 Singing Flower + specific materials |

**Evolution effects:**
- New visual sprite (2 variants per evolved form)
- Enhanced stat bonuses
- Reduced intimacy loss on owner death: **-1** instead of **-20**
- **Auto-feed** capability (pet feeds itself automatically when hungry)
- Slower hunger decay
- **Intimacy resets** after evolution -- must rebuild to Loyal again
- Evolved pets take significantly longer to reach Loyal (1-2 months with auto-feed)

---

### Pet Hunger Decay

Each pet has a `hungryDelay` (time interval) and `fullnessDecay` (amount lost per interval):

- **hungryDelay**: How often hunger decreases, in milliseconds. Default 60000ms (60 seconds).
- **fullnessDecay**: How many hunger points are lost per interval. Ranges from 1-4 per pet.

**Decay rates by pet category:**
- Fast hunger (4/min): Savage Babe
- Normal hunger (3/min): Poring, Drops, Lunatic, Picky, Chonchon, Peco Peco, Yoyo, Orc Warrior, Bongun, Baby Desert Wolf, Rocker
- Slow hunger (2/min): Poporing, Steel Chonchon, Hunter Fly, Spore, Poison Spore, Smokie, Green Petite, Munak, Dokebi, Sohee, Isis, Deviruchi, Baphomet Jr., most expansion pets
- Very slow hunger (1/min): Zealotus, Succubus, Incubus, Nightmare Terror, Goblin Leader, Dullahan, Medusa, Imp, Marionette, Miyabi Ningyo, Loli Ruri

**Starvation timeline:**
- At 3 decay/min, a fully fed pet (hunger 100) reaches 0 in ~33 minutes
- At 1 decay/min, a fully fed pet reaches 0 in ~100 minutes
- Once at 0 hunger, intimacy drops by 20 every 20 seconds (60/min) -- pet quickly reaches Awkward and risks running away

---

### Pet Death/Release

**Pets cannot die in combat** -- they are non-combat companions. However, pets can be permanently lost through:

1. **Running away (intimacy reaches 0):** The pet is permanently deleted. Cannot be recovered.
2. **Overfeeding 3 times consecutively:** Third consecutive overfeed triggers permanent loss (pet shows `/omg` emote and vanishes).
3. **Extended starvation:** At 0 hunger, intimacy drops by 20 every 20 seconds. Once intimacy reaches 0, pet runs away.

**Owner death effect:** When the owner dies, the active pet loses `intimacyOwnerDie` points (typically -20). The pet does NOT vanish on owner death -- it remains active but unhappier.

**Pet release (voluntary):** Some servers implement a "Release" option in the pet menu that permanently dismisses the pet. This is distinct from "Return to Egg" which preserves the pet.

---

## HOMUNCULUS SYSTEM

### Overview (Alchemist-Only Companion)

The Homunculus is a combat companion exclusive to **Alchemist** and **Biochemist/Creator** classes. Unlike pets, homunculi:
- **Fight alongside the player** with their own attacks and skills
- **Gain EXP and level up** (separate EXP from owner)
- **Have their own stats** that grow randomly per level
- **Can evolve** into a stronger form with a 4th skill
- **Have their own HP/SP** and can die
- **Are permanently bound** to the Alchemist (one per character)

**Prerequisites:**
- Must be Alchemist or higher class
- Must learn **Bioethics** skill (quest-based, unlocks homunculus management skills)
- Must craft an **Embryo** item (first summon only)

---

### 4 Types: Lif, Amistr, Filir, Vanilmirth

When first summoned with Call Homunculus + Embryo, the type is random:
- **25% chance for each type**
- **50% chance for each sprite variant** within a type (2 visual variants per type)
- The type is permanent -- it cannot be changed once summoned

| Type | Role | Race | Element | Food Item | Strengths |
|------|------|------|---------|-----------|-----------|
| **Lif** | Support/Healer | Demi-Human | Neutral | Pet Food (537) | Healing owner, movement speed buff, SP recovery |
| **Amistr** | Tank/Defense | Brute | Neutral | Zargon (912) | Highest HP, high VIT/DEF, position swap |
| **Filir** | DPS/Speed | Brute | Neutral | Garlet (910) | Highest AGI/ASPD, multi-hit attack, FLEE buff |
| **Vanilmirth** | Magic/Balanced | Formless | Neutral | Scell (911) | Uniform stat growth, random magic attacks, highest stat variance |

---

### Base Stats Per Type, Growth Tables

**Starting Stats (Level 1):**

| Stat | Lif | Amistr | Filir | Vanilmirth |
|------|-----|--------|-------|------------|
| HP | 150 | 320 | 90 | 80 |
| SP | 40 | 10 | 25 | 11 |
| STR | 17 | 20 | 29 | 11 |
| AGI | 20 | 17 | 35 | 11 |
| VIT | 15 | 35 | 9 | 11 |
| INT | 35 | 11 | 8 | 11 |
| DEX | 24 | 24 | 30 | 11 |
| LUK | 12 | 12 | 9 | 11 |

**HP/SP Growth Per Level (min-max range, uniform random):**

| Stat | Lif | Amistr | Filir | Vanilmirth |
|------|-----|--------|-------|------------|
| HP | 60-100 (avg 80) | 80-130 (avg 105) | 45-75 (avg 60) | 60-120 (avg 90) |
| SP | 4-9 (avg 6.5) | 1-4 (avg 2.5) | 3-6 (avg 4.5) | 1-6 (avg 3.5) |

**Stat Growth Probability Tables (per level-up, each stat rolls independently):**

**Lif:**

| Stat | +0 | +1 | +2 | Avg/Lv |
|------|-----|-----|-----|--------|
| STR | 33.33% | 60.00% | 6.67% | 0.73 |
| AGI | 33.33% | 60.00% | 6.67% | 0.73 |
| VIT | 33.33% | 60.00% | 6.67% | 0.73 |
| INT | 35.29% | 58.82% | 5.88% | 0.71 |
| DEX | 26.67% | 66.67% | 6.67% | 0.80 |
| LUK | 26.67% | 66.67% | 6.67% | 0.80 |

**Amistr:**

| Stat | +0 | +1 | +2 | Avg/Lv |
|------|-----|-----|-----|--------|
| STR | 15.38% | 76.92% | 7.69% | 0.92 |
| AGI | 35.29% | 58.82% | 5.88% | 0.71 |
| VIT | 35.29% | 58.82% | 5.88% | 0.71 |
| INT | 90.00% | 10.00% | 0% | 0.10 |
| DEX | 41.17% | 58.82% | 0% | 0.59 |
| LUK | 41.17% | 58.82% | 0% | 0.59 |

**Filir:**

| Stat | +0 | +1 | +2 | Avg/Lv |
|------|-----|-----|-----|--------|
| STR | 35.29% | 58.82% | 5.88% | 0.71 |
| AGI | 15.38% | 76.92% | 7.69% | 0.92 |
| VIT | 90.00% | 10.00% | 0% | 0.10 |
| INT | 41.17% | 58.82% | 0% | 0.59 |
| DEX | 35.29% | 58.82% | 5.88% | 0.71 |
| LUK | 41.17% | 58.82% | 0% | 0.59 |

**Vanilmirth (identical for all 6 stats):**

| Stat | +0 | +1 | +2 | +3 | Avg/Lv |
|------|-----|-----|-----|-----|--------|
| All | 30.00% | 33.33% | 33.33% | 3.33% | 1.10 |

**Approximate Stat Ranges at Level 99 (5th-95th percentile):**

| Stat | Lif | Amistr | Filir | Vanilmirth |
|------|-----|--------|-------|------------|
| HP | 7,790-8,170 | 10,361-10,840 | 5,820-6,110 | 8,321-9,449 |
| SP | 650-704 | 250-260 | 461-471 | 335-373 |
| STR | 71-85 | 104-118 | 89-107 | 105-133 |
| AGI | 79-93 | 77-95 | 119-133 | 105-133 |
| VIT | 74-88 | 95-113 | 14-23 | 105-133 |
| INT | 95-113 | 16-25 | 58-73 | 105-133 |
| DEX | 94-111 | 74-89 | 90-108 | 105-133 |
| LUK | 82-99 | 62-77 | 59-74 | 105-133 |

---

### Homunculus Skills (Complete List Per Type)

#### Lif Skills

| Skill | Type | Max Lv | SP Cost | Description |
|-------|------|--------|---------|-------------|
| **Healing Hands** | Active | 5 | 13/16/19/22/25 | Heals owner HP. Requires 1 Condensed Red Potion per cast. Formula: `floor((BaseLv + INT) / 5) * SkillLv * 3 * (1 + RecoveryBonus%) + MATK` |
| **Urgent Escape** | Active | 5 | 20/25/30/35/40 | Increases Lif's AND owner's movement speed by 10-50% for 40/35/30/25/20s. Cooldown: 60/70/80/90/120s |
| **Brain Surgery** | Passive | 5 | -- | +1-5% MaxSP, +2-10% Healing Hands effectiveness, +3-15% SP Recovery rate |
| **Mental Charge** | Active (Evolved) | 3 | 70/80/90 | Uses MATK instead of ATK for attacks for 1/3/5 min. +30/60/90 VIT, +20/40/60 INT. Requires evolution. |

#### Amistr Skills

| Skill | Type | Max Lv | SP Cost | Description |
|-------|------|--------|---------|-------------|
| **Castling** | Active | 5 | 10 (all levels) | Swaps Amistr and owner position. Success rate: 20/40/60/80/100% |
| **Amistr Bulwark** | Active | 5 | 20 (all levels) | +10/15/20/25/30 VIT for 40/35/30/25/20s. Cooldown: 60/70/80/90/120s |
| **Adamantium Skin** | Passive | 5 | -- | +2/4/6/8/10% MaxHP, +5/10/15/20/25% HP Recovery, +4/8/12/16/20 DEF |
| **Blood Lust** | Active (Evolved) | 3 | 120 | +30/40/50% ATK, 20% lifesteal on each hit. Requires evolution. Intimacy requirement: 50+ (Awkward). |

#### Filir Skills

| Skill | Type | Max Lv | SP Cost | Description |
|-------|------|--------|---------|-------------|
| **Moonlight** | Offensive | 5 | 4/8/12/16/20 | Pecks target: 1/2/2/2/3 hits at 220/330/440/550/660% ATK total damage |
| **Flitting (Fleeting Move)** | Active | 5 | 20 (all levels) | +3/6/9/12/15% ASPD, +10/15/20/25/30% ATK for 60/55/50/45/40s. Cooldown: 60/70/80/90/120s |
| **Accelerated Flight** | Active | 5 | 20 (all levels) | +20/30/40/50/60 FLEE for 60/55/50/45/40s. Cooldown: 60/70/80/90/120s |
| **S.B.R.44** | Offensive (Evolved) | 3 | 1 | Deals `Intimacy * 100/200/300` damage (piercing, ignores DEF). Drops intimacy to 2. Requires >= 3 intimacy. Requires evolution. |

#### Vanilmirth Skills

| Skill | Type | Max Lv | SP Cost | Description |
|-------|------|--------|---------|-------------|
| **Caprice** | Offensive | 5 | 22/24/26/28/30 | Randomly casts Lv1-5 Fire Bolt, Cold Bolt, Lightning Bolt, or Earth Spike on target. Bolt level range: 1 / 1-2 / 1-3 / 1-4 / 1-5. |
| **Chaotic Blessings** | Supportive | 5 | 40 (all levels) | Randomly casts Heal on a random target (enemy, Vanilmirth itself, or owner). Heal level scales with skill level. |
| **Instruction Change** | Passive | 5 | -- | +1/2/3/4/5 INT, +1/1/2/3/4 STR, +1/2/3/4/5% Potion Creation success rate |
| **Self-Destruction** | Offensive (Evolved) | 3 | 1 | Deals `MaxHP * (1/1.5/2)` piercing damage in 9x9 AoE. Drops intimacy to 1. Requires >= 450 intimacy. Requires evolution. |

**Skill Points:**
- Homunculus earns **1 skill point every 3 levels**
- Total skill points at Lv99: **33**
- No "Job Level" equivalent -- just base level

---

### Feeding System (Food Type, Hunger, Intimacy)

**Hunger Range:** 0-100%

| Food Item | Homunculus Type |
|-----------|----------------|
| Pet Food (537) | Lif |
| Zargon (912) | Amistr |
| Garlet (910) | Filir |
| Scell (911) | Vanilmirth |

**Hunger mechanics:**
- Each feeding increases hunger by **10 points** (different from pets which gain 20)
- Hunger decreases by **1 point per minute** while the homunculus is active (summoned)
- Maximum hunger: 100

**Intimacy gain from feeding (based on hunger when fed):**

| Hunger Range | Intimacy Change | Notes |
|-------------|----------------|-------|
| 1-10% | +0.5 | Starvation zone, small gain |
| 11-25% | +1.0 | **Optimal** -- best intimacy/feed ratio |
| 26-75% | +0.75 | Moderate gain |
| 76-90% | -0.05 | Slight loss, avoid |
| 91-100% | -0.5 | Overfed, lose intimacy |
| 0% (starving) | -1.0 per tick | Automatic decay, no feeding involved |

**Starvation penalty:**
- Loses **18 intimacy per hour** of starvation (1 every 200 seconds)
- After **24 hours** of starvation: ~432 intimacy lost
- If intimacy reaches 0: **Homunculus is permanently abandoned** (cannot be recovered, must create new Embryo)

**Optimal feeding strategy:**
- Feed when hunger is between 11-25% for maximum intimacy gain
- One feed per ~35-75 minutes depending on timing
- From Neutral (250) to Loyal (910) requires 660 intimacy points = ~660 optimal feeds

---

### EXP System (How Homunculus Gains EXP)

**Level cap:** 99

**EXP distribution when owner kills a monster:**
- Player receives **90%** of Base EXP and Job EXP
- Homunculus receives **10%** of Base EXP (regardless of damage contribution)
- Homunculus does NOT receive Job EXP (it has no job)

**EXP table:**
Uses the same pre-renewal EXP curve. Total EXP to level 99: **~203,562,540**

The EXP required per level follows an exponential curve. Approximate formula:
```
ExpToNextLevel(level) = floor(21 * level^2.8 + 100)
```

**Key EXP milestones (approximate):**

| Level | Cumulative EXP | EXP to Next |
|-------|---------------|-------------|
| 10 | ~13,000 | ~2,500 |
| 20 | ~120,000 | ~14,000 |
| 30 | ~540,000 | ~42,000 |
| 40 | ~1,600,000 | ~95,000 |
| 50 | ~3,800,000 | ~180,000 |
| 60 | ~7,900,000 | ~310,000 |
| 70 | ~14,600,000 | ~500,000 |
| 80 | ~24,800,000 | ~760,000 |
| 90 | ~40,200,000 | ~1,100,000 |
| 99 | ~203,562,540 | -- |

**Homunculus does NOT gain EXP when:**
- The owner doesn't get the kill
- The owner is dead
- The homunculus is vaporized (not summoned)

---

### Evolution System (Requirements, Stat Changes)

**Requirements to evolve:**
1. Homunculus intimacy must be **Loyal (911+)**
2. Player must have a **Stone of Sage** item in inventory
3. No level requirement
4. Double-click the Stone of Sage to trigger evolution

**Stone of Sage sources:**
- Gemini S58 (Biolabs dungeon)
- Various Thanatos Tower monsters
- Can also be crafted or purchased on some servers

**Evolution effects:**
- Random bonus of **+1 to +10** for each stat (STR/AGI/VIT/INT/DEX/LUK)
- Increased Max HP and Max SP
- New visual sprite (2 variants per evolved type)
- Unlocks the **4th skill** (ultimate/evolved skill)
- **Intimacy resets to 10** ("Hate" status) -- must rebuild to Loyal
- Level and EXP are preserved

**Evolved form names:**

| Base Form | Evolved Form | Internal Name |
|-----------|-------------|---------------|
| Lif | H-Lif | Eira |
| Amistr | H-Amistr | Bayeri |
| Filir | H-Filir | Sera |
| Vanilmirth | H-Vanilmirth | Dieter |

**Evolved skill intimacy requirements:**
- Lif (Mental Charge): Requires **Awkward** (50+ intimacy) to unlock
- Amistr (Blood Lust): Requires **Awkward** (50+ intimacy) to unlock
- Filir (S.B.R.44): Requires only **2 intimacy** (minimum)
- Vanilmirth (Self-Destruction): Requires **450 intimacy** (half)

> **Note:** Homunculus S (mutation to 5 new types: Bayeri, Sera, Dieter, Eleanor, Eira) is a Renewal/3rd class (Geneticist) feature. Not applicable to pre-Renewal.

---

### AI System (Behavior Modes: Follow, Attack, Standby)

Homunculus behavior is controlled by an AI script system. In original RO, players could write custom Lua AI scripts. For our server-authoritative implementation, the AI logic runs server-side.

**Four behavior states:**

| State | Code | Behavior |
|-------|------|----------|
| **IDLE** | 0 | Homunculus is standing still, no target |
| **FOLLOW** | 1 | Following owner at 2-3 cell distance |
| **CHASE** | 2 | Moving toward a target to attack |
| **ATTACK** | 3 | Actively attacking a target |

**Default AI behavior:**
1. **Idle/Follow:** Follow owner at 2-3 cells distance. Teleport to owner if distance > 15 cells.
2. **Combat (aggressive):** When owner attacks a monster, homunculus targets the same monster
3. **Auto-skill:** Use offensive skills when available (Moonlight/Caprice) with cooldown timers
4. **Heal:** Lif's Healing Hands triggers when owner HP < threshold (configurable)
5. **Buff:** Amistr Bulwark / Flitting / Accelerated Flight used off cooldown in combat
6. **Flee:** If homunculus HP < threshold, return to owner / flee behavior

**Default AI problems (from original RO):**
- Filir and Vanilmirth: Kill-steal everything in range (too aggressive)
- Lif and Amistr: Don't attack at all (too passive)
- Solution: Custom AI scripts (Lua in original, server-side logic in our implementation)

**AI toggle command:** `/hoai` toggles between Default AI and User AI

**Recommended server-side AI approach:**
```
Priority:
1. If owner has target and homunculus has no target -> attack owner's target
2. If homunculus is being attacked and has no other target -> attack attacker
3. If owner is being attacked and homunculus has no target -> attack owner's attacker
4. If in Standby mode -> do nothing (follow only)
5. If HP < 25% -> retreat to owner
6. Use skills based on cooldown timers (configurable per skill)
```

---

### Homunculus Commands

| Command | Hotkey | Effect |
|---------|--------|--------|
| Attack target | Alt + Right Click (monster) | Order homunculus to attack specific target |
| Attack (aggressive) | Alt + Double Right Click | Priority attack order |
| Follow / Standby | Alt + T | Toggle between follow mode and stationary mode |
| Move to location | Alt + Right Click (ground) | Move to specific location (15-tile max range) |
| Open info window | Alt + R | Display homunculus stats/skills window |
| Toggle AI | /hoai | Switch between default and custom AI |
| Feed | Via homunculus info window | Feed the homunculus its food item |

---

### Vaporize/Resurrect Mechanics

**Vaporize (Rest):**
- SP Cost: **50**
- Requirement: Homunculus HP must be >= **80% of MaxHP**
- Effect: Stores the homunculus in a "vaporized" state. It disappears from the field.
- Vaporized homunculus does NOT lose intimacy
- Vaporized homunculus does NOT lose hunger (hunger timer pauses)
- Can be re-summoned with Call Homunculus (no Embryo needed)

**Auto-Vaporize on owner death:**
- If owner dies and homunculus HP >= 80%: Homunculus auto-vaporizes
- If owner dies and homunculus HP < 80%: Homunculus stays active and can continue fighting
- Owner death does NOT reduce homunculus intimacy

**Call Homunculus (Re-summon):**
- SP Cost: **10**
- First use: Requires an Embryo item (consumed, random type selected)
- Subsequent uses: No Embryo needed (bound homunculus is recalled)
- Re-summons at last known HP/SP/stats
- If homunculus died: Must use Homunculus Resurrection first

**Homunculus Resurrection:**
- SP Cost: **74** (all levels)
- Prerequisite: Bioethics Lv1
- Max Level: **5**
- Effect: Revives dead homunculus at a percentage of MaxHP

| Level | HP Restored |
|-------|-------------|
| 1 | 20% |
| 2 | 40% |
| 3 | 60% |
| 4 | 80% |
| 5 | 99% |

**Permanent loss:**
- If intimacy reaches 0 (through starvation), the homunculus is **permanently abandoned**
- Cannot be recovered with Call Homunculus or Resurrection
- Must craft a new Embryo and summon a completely new homunculus

---

### Homunculus Stat Formulas (Derived Combat Stats)

```
ATK  = 2 * Level + STR
MATK = Level + INT + floor((LUK + INT + DEX) / 3)
HIT  = Level + DEX + 150
CRIT = floor(LUK / 3) + 1
DEF  = VIT + floor(Level / 2)
softDEF = VIT + floor(AGI / 2)
MDEF = floor((VIT + Level) / 4) + floor(INT / 2)
softMDEF = floor((VIT + INT) / 2)
FLEE = Level + AGI
ASPD = max(100, 200 - floor((1000 - 4*AGI - DEX) * 700 / 1000 / 10))
```

**Base ASPD:** 130 (1400ms attack interval)

**Combat rules:**
- Hit rate capped at **95%** (5% guaranteed miss)
- Can hit Ghost-property monsters
- FLEE is NOT reduced by mob count (unlike players)
- Homunculus has no equipment -- stats are purely from level-up growth
- Homunculus race and element affect incoming damage (e.g., Amistr as Brute takes extra from Insect-advantage cards)

---

### Embryo Creation

**Required materials:**
| Material | Item ID | Notes |
|----------|---------|-------|
| Medicine Bowl | 7134 | Consumed |
| Glass Tube | 7133 | Consumed |
| Morning Dew of Yggdrasil | 7135 | Consumed |
| Seed of Life | 7136 | Consumed |
| Potion Creation Guide | 7144 | **NOT consumed** (reusable) |

**Creation process:**
- Use the **Prepare Potion** skill with all materials in inventory
- Success rate depends on creator's **INT**, **DEX**, and **LUK**
- Failed attempts consume all materials except the Potion Creation Guide
- Materials purchased from the Al de Baran Alchemist Guild NPC

---

### Homunculus Intimacy System

**Intimacy Range:** 0-1000

| Status | Range | Notes |
|--------|-------|-------|
| Hate with Passion | 1-3 | Extremely dangerous -- close to permanent loss |
| Hate | 4-10 | Post-evolution reset lands here (10) |
| Awkward | 11-100 | Mental Charge / Blood Lust unlock at 50+ |
| Shy | 101-250 | |
| Neutral | 251-750 | Starting intimacy for new homunculus |
| Cordial | 751-910 | |
| Loyal | 911-1000 | Required for evolution. S.B.R.44 requires only 2+. Self-Destruction requires 450+. |

**Intimacy changes:**
- Feeding at optimal hunger (11-25%): +1.0
- Feeding at near-starving (1-10%): +0.5
- Feeding at moderate hunger (26-75%): +0.75
- Feeding when satisfied (76-90%): -0.05
- Feeding when stuffed (91-100%): -0.5
- Starvation (0% hunger): -1 per tick (every ~200 seconds = 18/hour)
- Owner death: **No intimacy loss** (unlike pets)
- Homunculus death: **No intimacy loss**
- Vaporize: **No intimacy loss**

---

## Implementation Checklist

### Pet System

- [x] `ro_pet_data.js` -- Complete pet database (56 pets with all fields)
- [x] `PET_INTIMATE` / `PET_HUNGER` constants
- [x] `calculateCaptureRate()` -- rAthena-accurate capture formula
- [x] `calculateFeedIntimacyChange()` -- Feeding intimacy logic
- [x] `pet:tame` socket handler -- Taming with item consumption
- [x] `pet:incubate` socket handler -- Egg hatching
- [x] `pet:return_to_egg` socket handler -- Return to egg
- [x] `pet:feed` socket handler -- Feeding with hunger/intimacy update
- [x] Hunger decay timer (per-pet `hungryDelay` and `fullnessDecay`)
- [x] Starvation intimacy loss (-20 every 20 seconds at 0 hunger)
- [x] Pet runs away at 0 intimacy (permanent deletion)
- [x] `activePets` Map for in-memory tracking
- [x] DB persistence (`character_pets` table)
- [x] Pet bonus application in `getEffectiveStats()`
- [x] Disconnect save (hunger + intimacy)
- [ ] Pet overfeeding escalation (3-strike runaway mechanic)
- [ ] Pet rename command
- [ ] Pet performance/emote command (small intimacy gain)
- [ ] Pet accessory equip/unequip socket handlers
- [ ] Pet accessory support scripts (Smokie Perfect Hide, etc.)
- [ ] Pet following position broadcast to zone (client-side visual)
- [ ] Pet egg trade (intimacy reset on trade)
- [ ] Owner death intimacy loss hook in death handler
- [ ] Pet evolution system (Renewal feature -- future)
- [ ] Client-side pet actor spawning (UE5 BP actor following player)
- [ ] Pet UI panel (feed, performance, rename, equip accessory)
- [ ] Pet emote display system (emotes based on intimacy tier)

### Homunculus System

- [x] `ro_homunculus_data.js` -- 4 types with base stats, growth tables, skills
- [x] `rollStatGrowth()` / `applyLevelUpGrowth()` -- Level-up stat growth
- [x] `calculateHomunculusStats()` -- Derived combat stats
- [x] `getExpToNextLevel()` -- EXP curve
- [x] `getIntimacyBracket()` -- Intimacy status lookup
- [x] `getFeedIntimacyChange()` -- Feed intimacy change by hunger
- [x] `character_homunculus` DB table
- [x] Homunculus auto-attack in combat tick (ASPD 130, 1400ms)
- [x] EXP sharing (10% to homunculus)
- [x] Level-up with random stat growth
- [x] `homunculus:feed` socket handler (type-specific food, hunger/intimacy)
- [x] `homunculus:command` socket handler (follow/attack/standby)
- [x] Hunger tick (60s decay, starvation death)
- [x] Persistence on disconnect (HP/SP/EXP/stats/intimacy/hunger)
- [x] Call Homunculus (create + Embryo / re-summon)
- [x] Rest/Vaporize (HP >= 80% requirement)
- [x] Homunculus Resurrection (20-100% HP by skill level)
- [ ] Homunculus skill casting (Moonlight, Caprice, Healing Hands, etc.)
- [ ] Homunculus AI loop (target selection, skill usage, retreat)
- [ ] Homunculus evolution (Stone of Sage, stat bonus, 4th skill unlock)
- [ ] Homunculus auto-vaporize on owner death (HP >= 80% check)
- [ ] Homunculus permanent abandonment at 0 intimacy
- [ ] Client-side homunculus actor spawning
- [ ] Homunculus position broadcast to zone
- [ ] Homunculus info window (stats, skills, feed button)
- [ ] Homunculus skill point allocation UI
- [ ] Enemy targeting of homunculus (monsters can attack it)
- [ ] Homunculus taking damage / death handling
- [ ] Castling (Amistr position swap)
- [ ] Evolved skill implementations (Mental Charge, Blood Lust, S.B.R.44, Self-Destruction)

---

## Gap Analysis

### Current Implementation Status

**What exists (from server `index.js` + `ro_pet_data.js` + `ro_homunculus_data.js`):**

**Pet System -- ~70% server-side complete:**
- Full pet database with 56 pets
- Core taming, hatching, feeding, return-to-egg handlers
- Hunger decay with starvation intimacy loss
- Pet runs away mechanic
- Bonus application in stats
- DB persistence

**Homunculus System -- ~50% server-side complete:**
- Full data definitions with growth tables
- Basic combat tick (auto-attack)
- Feeding, commanding, summoning, vaporizing, resurrection
- EXP sharing and level-up
- DB persistence

### Critical Gaps

1. **No client-side actors** -- Neither pets nor homunculi have UE5 actors. Players cannot see their companions. This is the largest gap for user experience.

2. **No homunculus skill casting** -- Moonlight, Caprice, Healing Hands, etc. are defined in data but have no combat execution code. This makes the homunculus a basic auto-attacker only.

3. **No homunculus AI loop** -- Currently just attacks owner's target. No intelligent target selection, skill usage, retreat behavior, or priority system.

4. **No homunculus evolution** -- Stone of Sage trigger, stat bonuses, 4th skill unlock, and intimacy reset are not implemented.

5. **No pet position broadcast** -- Pets exist server-side but their position is never sent to clients. No visual representation.

6. **No pet accessory system** -- Equip/unequip handlers not implemented. Support scripts (Smokie Perfect Hide, etc.) not coded.

7. **No homunculus damage reception** -- Monsters cannot target or damage the homunculus. It's an invulnerable attacker.

8. **No pet/homunculus UI panels** -- No client-side widgets for managing pets (feed, rename, accessory) or homunculus (stats window, skill allocation, feed).

### Implementation Priority (Recommended Order)

| Priority | System | Effort | Impact |
|----------|--------|--------|--------|
| P0 | Client-side pet/homunculus actor + position broadcast | High | Essential for any visual presence |
| P1 | Homunculus skill casting (Moonlight, Caprice, Healing Hands) | Medium | Core combat functionality |
| P1 | Homunculus AI loop (target selection, skill usage) | Medium | Makes homunculus useful |
| P2 | Homunculus damage reception + death handling | Medium | Balances the system |
| P2 | Homunculus evolution (Stone of Sage) | Low | Endgame content |
| P2 | Pet UI panel (feed, accessory, rename) | Medium | Player convenience |
| P3 | Pet overfeeding 3-strike mechanic | Low | Polish |
| P3 | Pet accessory support scripts | Low | Enhancement |
| P3 | Pet evolution (Renewal feature) | Low | Future content |
| P3 | Homunculus stats/skills UI panel | Medium | Player management |
| P4 | Pet emotes / performance animations | Low | Polish |
| P4 | Homunculus S mutation (Renewal) | High | Future content |
