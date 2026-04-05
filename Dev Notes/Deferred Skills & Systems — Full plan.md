# Deferred Skills & Systems — Full plan

   # Deferred Skills & Systems — Full RO Classic Audit & Remediation Plan

     **Date:** 2026-03-16
     **Scope:** Every deferred, stubbed, or partially-implemented skill system compared against Ragnarok Online
     Classic (pre-renewal) expected behavior.
     **Sources:** iRO Wiki Classic, RateMyServer, rAthena pre-renewal DB, RagnaCloneDocs/, community guides.

     ---

     ## Table of Contents

     1. [Tier 0 — Critical Bugs (Fix Immediately)](#tier-0--critical-bugs)
     2. [Tier 1 — High Priority (Core Mechanic Missing)](#tier-1--high-priority)
     3. [Tier 2 — Medium Priority (Implementable Now)](#tier-2--medium-priority)
     4. [Tier 3 — Low Priority / Deferred (Need New Systems)](#tier-3--low-priority--deferred)
     5. [Data Corrections (ro_skill_data_2nd.js)](#data-corrections)
     6. [Cross-Cutting Systems Required](#cross-cutting-systems)
     7. [Dependency Graph](#dependency-graph)
     8. [Implementation Phases](#implementation-phases)

     ---

     ## Tier 0 — Critical Bugs

     These are bugs in EXISTING code that produce incorrect behavior right now. No new systems needed — just fixes.

     ### T0-1. Chemical Protection is Dispellable (WRONG)

     **Bug:** All 4 CP buffs (`chemical_protection_helm/shield/armor/weapon`) are NOT in the `UNDISPELLABLE` set at
     line ~11463 of `index.js`. Sage's Dispell currently removes them.
     **RO Classic:** Chemical Protection CANNOT be dispelled. It is one of the few buffs immune to Dispell.
     **Fix:** Add all 4 CP buff names to the `UNDISPELLABLE` set:
     ```js
     const UNDISPELLABLE = new Set([
         'hindsight', 'play_dead', 'auto_berserk', 'devotion_protection',
         'chemical_protection_helm', 'chemical_protection_shield',
         'chemical_protection_armor', 'chemical_protection_weapon'
     ]);
     ```
     **Estimate:** 5 minutes. **Files:** `index.js` (~line 11463)

     ---

     ### T0-2. Plagiarism `checkPlagiarismCopy()` is Dead Code (NEVER CALLED)

     **Bug:** The `checkPlagiarismCopy()` function is defined at line ~203 of `index.js` but has **zero call
     sites**. Plagiarism (Rogue skill 1714) is completely non-functional — no skill is ever copied when the Rogue
     takes damage from a skill.
     **RO Classic:** When a Rogue/Stalker is hit by a single-target skill, they automatically copy it (if copyable
     and no Preserve buff active). The copied skill replaces any previously copied skill.
     **Fix:** Add a call to `checkPlagiarismCopy(targetPlayer, skillId, skillLevel)` in every single-target skill
     damage path where the target is a player. The call should go after damage is applied but before the function
     returns.
     **Integration points:** Every `skill:effect_damage` emission where the target is a player (bolt skills, Soul
     Strike, Bash, Pierce, etc.). The function already handles the whitelist check (`PLAGIARISM_COPYABLE_SKILLS`)
     and level cap.
     **Estimate:** 1-2 hours (find all single-target skill damage paths, add the call). **Files:** `index.js`

     ---

     ### T0-3. Plagiarism Has No DB Persistence

     **Bug:** `player.plagiarizedSkill` is set to `null` on `player:join` and never loaded from DB. If a Rogue
     copies a skill then relogs, the skill is lost permanently.
     **RO Classic:** Plagiarized skill persists across logout. rAthena stores it as `CLONE_SKILL` / `CLONE_SKILL_LV`
      in the character global registry.
     **Fix:**
     1. DB migration: `ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_id INTEGER DEFAULT NULL,
     ADD COLUMN IF NOT EXISTS plagiarized_skill_level INTEGER DEFAULT 0;`
     2. Add same columns to server startup auto-creation block
     3. On `checkPlagiarismCopy()`: persist to DB after setting `player.plagiarizedSkill`
     4. On `player:join`: load from DB and restore `player.plagiarizedSkill`
     5. Emit `plagiarism:data` event on join so client can display it
     6. On `skill:reset`: clear DB columns
     **Estimate:** 1-2 hours. **Files:** `index.js`, new migration file

     ---

     ### T0-4. Potion Pitcher VIT Formula Wrong

     **Bug:** At line ~14939, the heal formula uses `(100 + targetVIT) / 100` but RO Classic uses `(100 + targetVIT
     * 2) / 100`. The VIT multiplier should be **2x VIT**, not 1x.
     **RO Classic formula (3-step):**
     ```
     Step 1: HP = PotionBase * (100 + PitcherLv*10 + PotionResearchLv*5) / 100
     Step 2: HP = HP * (100 + targetVIT*2) / 100       [HP potions]
        OR:  SP = SP * (100 + targetINT*2) / 100       [Blue Potion at Lv5]
     Step 3: HP = HP * (100 + IncHPRecoveryLv*10) / 100 [if target has passive]
     ```
     **Additional gaps:**
     - Blue Potion (Lv5) should use target INT, not VIT, for the stat multiplier
     - Inc HP Recovery bonus (Step 3) is not applied
     - Homunculus targets should receive 3x healing (not implemented)
     **Estimate:** 30 minutes. **Files:** `index.js` (~line 14939)

     ---

     ### T0-5. Marine Sphere HP Data Off by +400

     **Bug:** In `ro_skill_data_2nd.js`, Marine Sphere effectValue formula gives 2800/3200/3600/4000/4400. RO
     Classic HP values are 2400/2800/3200/3600/4000. Formula should be `2000 + 400*(i+1)`.
     **Fix:** Change effectValue in skill data.
     **Estimate:** 5 minutes. **Files:** `ro_skill_data_2nd.js`

     ---

     ## Tier 1 — High Priority

     Core mechanics that are partially implemented — the cast/buff handler exists but the actual functional effect
     is missing.

     ### T1-1. Devotion Damage Redirect Not Implemented

     **Current:** Cast handler (line ~10443) works correctly — validates target, creates `devotionLinks[]`, sets
     `devotedTo`, applies buff. But the **damage redirect** at line ~20974 only checks break conditions; damage
     still hits the protected target normally.
     **RO Classic:** ALL physical AND magical damage to the protected target is transferred to the Crusader. The
     target's DEF/MDEF is used for reduction, not the Crusader's. If the Crusader dies from redirected damage,
     excess does NOT pass to the target.
     **Gaps:**
     1. Damage redirect not wired — core mechanic entirely missing
     2. Break condition uses HP < 25% threshold (non-canonical; should break on Crusader death or target
     out-of-range + taking damage)
     3. Auto Guard/Reflect Shield/Defender interaction through Devotion not implemented
     4. Cast interrupt immunity for devoted players not implemented
     5. Range break logic wrong (should only break when out-of-range AND damage occurs, not proactively)
     **Fix:** In every damage application path (physical auto-attack, skill damage, magic damage), check if the
     target has `devotedTo` set. If so, redirect the final damage to the Crusader instead. Use the target's DEF/MDEF
      for reduction calculations. Remove Devotion link if Crusader dies.
     **Estimate:** 4-6 hours. **Files:** `index.js` (combat tick, skill damage paths)

     ---

     ### T1-2. Magic Rod Absorption Not Wired

     **Current:** Buff (`magic_rod`) applies correctly with duration (400-1200ms) and `absorbPct` (20-100%). But NO
     code anywhere checks for this buff when a player is hit by single-target magic.
     **RO Classic:**
     - Absorbs single-target magic only (bolts, Soul Strike, Frost Diver, Jupitel Thunder)
     - Does NOT absorb ground AoE (Storm Gust, LoV, Meteor Storm, Thunderstorm)
     - Negates 100% of absorbed spell damage
     - Restores `floor(spell_SP_cost * absorbPct / 100)` SP to the target
     - Can absorb multiple spells during its short duration
     - Movement cancels the buff
     **Fix:** Add absorption check at the top of every single-target magic damage calculation:
     ```js
     if (hasBuff(target, 'magic_rod')) {
         const rodBuff = target.activeBuffs.find(b => b.type === 'magic_rod');
         const spRestore = Math.floor(spCost * rodBuff.absorbPct / 100);
         target.sp = Math.min(target.sp + spRestore, target.maxSP);
         // emit absorption event, skip damage
         return;
     }
     ```
     **Integration points:** Cold Bolt, Fire Bolt, Lightning Bolt, Soul Strike, Frost Diver, Napalm Beat
     (single-target), Jupiter Thunder, Holy Light, and future single-target magic skills.
     **Estimate:** 2-3 hours. **Files:** `index.js`

     ---

     ### T1-3. Homunculus Not Targetable by Enemies

     **Current:** Homunculus auto-attacks enemies but enemies never target the homunculus. `hState.hpCurrent` is
     never reduced by enemy attacks. Aggro always goes to the owner.
     **RO Classic:** Enemies CAN target and attack homunculi. Aggressive AI attacks homunculi in range. Amistr is
     specifically designed as a tank. FLEE applies. When HP reaches 0, homunculus dies and must be resurrected.
     **Gaps:**
     1. Enemy AI `findAggroTarget()` does not consider homunculi
     2. No damage reception path for homunculus (nothing reduces `hpCurrent`)
     3. No aggro split — all aggro goes to owner via `setEnemyAggro()`
     4. No homunculus death from damage
     5. No FLEE calculation for homunculus dodge
     6. No `homunculus:died` event broadcast
     **Fix:** Extend enemy AI to include homunculi as valid targets. Add damage application path. Add death
     handling.
     **Estimate:** 4-6 hours. **Files:** `index.js` (enemy AI tick, combat tick)

     ---

     ### T1-4. Moonlit Water Mill / Unbarring Octave Have No Effect

     **Current:** Both performances start correctly via `startPerformance()` and drain SP, but
     `PERFORMANCE_BUFF_MAP` maps them to `null` (line ~795). `calculatePerformanceEffects()` has no case for them —
     returns `{}`.
     **RO Classic:**
     - **Moonlit Water Mill (Bard):** All enemies in AoE cannot use normal attacks (auto-attack blocked)
     - **Unbarring Octave (Dancer):** All enemies in AoE cannot use skills
     **Fix:** Add buff entries in `PERFORMANCE_BUFF_MAP` and cases in `calculatePerformanceEffects()`. The combat
     tick needs to check for these debuffs on enemies before allowing auto-attacks/skills.
     **Estimate:** 2-3 hours. **Files:** `index.js`

     ---

     ## Tier 2 — Medium Priority

     Can be implemented with existing systems or minor extensions. No major new infrastructure needed.

     ### T2-1. Sight Blaster Proximity Trigger (ID 813)

     **Current:** Buff applies for 120s with 100% MATK. No proximity check exists.
     **RO Classic:** Fire orb triggers automatically when any enemy enters 3x3 cells (~150 UE). Deals 100% MATK fire
      damage + 3 cell knockback. Consumed after trigger. Can hit multiple enemies simultaneously.
     **Fix:** Add `checkSightBlasterTriggers()` to the main tick loop (can share infrastructure with existing
     Ruwach/Sight detection ticks):
     ```
     For each player with sight_blaster buff:
       For each enemy in same zone within 150 UE:
         Calculate 100% MATK fire damage
         Apply damage + knockback 3 cells
         Remove sight_blaster buff
         Break (consumed)
     ```
     **Estimate:** 1-2 hours. **Files:** `index.js`

     ---

     ### T2-2. Ore Discovery Passive (ID 1208)

     **Current:** Stub handler (redundant for passive). No passive effect in `getPassiveSkillBonuses()` or monster
     death code.
     **RO Classic:** On killing blow, chance to drop a random ore from `IG_FINDINGORE` group (Iron Ore, Coal, Steel,
      Phracon, Emveretarcon, Oridecon, Elunium, Star Crumb, Gold, Emperium). Estimated 1-5% chance.
     **Fix:** Add ore item group constant. Hook into enemy death handler — if killer has Ore Discovery learned, roll
      chance, award random ore to inventory.
     **Estimate:** 1 hour. **Files:** `index.js`

     ---

     ### T2-3. Remove Trap — Rogue Version (ID 1708)

     **Current:** Stub. Hunter version (ID 905) IS implemented and works.
     **RO Classic:** Rogue version can remove ANY trap (not just own), including enemy Hunter traps. Returns 1 Trap
     item. Range ~100-150 UE.
     **Fix:** Copy Hunter handler logic with modified ownership rules: Rogue can remove any non-Rogue trap.
     Hunter-class can only remove own traps; Rogue-class can remove any trap.
     **Estimate:** 30 minutes. **Files:** `index.js`

     ---

     ### T2-4. Weapon Repair (ID 1209)

     **Current:** Stub. Weapon break from Power Thrust exists (`player.weaponBroken` flag).
     **RO Classic:** Active skill, 5s cast, SP 30. Repairs broken weapon/armor. Material cost by weapon level:
     Lv1=Iron Ore, Lv2=Iron, Lv3=Steel, Lv4=Rough Oridecon. Armor=Steel. Can target self or allies.
     **Fix:**
     1. Add handler: identify broken equipment slot, check material in BS inventory, consume material, clear broken
     flag, restore equipment stats
     2. Add broken state DB persistence (currently runtime only)
     3. Add armor break support (currently only weapon break exists)
     **Estimate:** 2-3 hours. **Files:** `index.js`

     ---

     ### T2-5. Create Elemental Converter (ID 1420)

     **Current:** Stub.
     **RO Classic:** Quest skill, SP 30, instant. Creates one of 4 converters from materials:
     - Fire: 3 Scorpion Tail (904) + 1 Blank Scroll (7433)
     - Water: 3 Snail Shell (946) + 1 Blank Scroll (7433)
     - Earth: 3 Horn (947) + 1 Blank Scroll (7433)
     - Wind: 3 Rainbow Shell (1013) + 1 Blank Scroll (7433)

     Converters are consumable items that self-endow the corresponding element for 5 minutes (pre-renewal).
     **Fix:** Add converter recipes, material validation, converter item creation. Client needs a simple 4-option
     selection popup.
     **Estimate:** 2-3 hours (server) + 1-2 hours (client popup). **Files:** `index.js`, `ro_item_effects.js`

     ---

     ### T2-6. Blade Stop (ID 1609)

     **Current:** Stub.
     **RO Classic:** SP 10, instant, costs 1 sphere. Enters "catching" stance for 300+200*lv ms. If enemy melee
     auto-attack lands during window, both are root-locked for 10000+10000*lv ms. Monk can use level-gated skills
     during lock: Lv2+ Finger Offensive, Lv3+ Investigate, Lv4+ Chain Combo (bypasses Triple Attack prereq), Lv5+
     Asura Strike (no cast time).
     **Gaps:**
     1. Catching stance state + timer
     2. Auto-attack tick hook to detect melee hit on catching player
     3. Root lock for both players (movement + attack blocked)
     4. Level-based skill whitelist during lock
     5. Combo system integration (Chain Combo from lock at Lv4)
     6. Asura Strike cast time bypass at Lv5
     **Estimate:** 4-6 hours (complex). **Files:** `index.js`

     ---

     ### T2-7. Homunculus Skill Casting

     **Current:** Homunculi only auto-attack. 8 skills defined in `ro_homunculus_data.js` but no execution handlers.
     **RO Classic skills per type:**

     | Type | Skill 1 | Skill 2 | Passive |
     |------|---------|---------|---------|
     | Lif | Healing Hands (heal owner) | Urgent Escape (+movespeed) | Brain Surgery (+MaxSP, +heal%) |
     | Amistr | Castling (swap pos w/ owner) | Bulwark (+VIT self) | Adamantium Skin (+HP, +DEF) |
     | Filir | Moonlight (phys damage) | Flitting (+ASPD, +ATK) | Accelerated Flight (+FLEE) |
     | Vanilmirth | Caprice (random bolt) | Chaotic Blessings (random heal) | Instruction Change (+INT, +brew%) |

     **Fix:**
     1. Add skill handlers for each active skill
     2. Add skill AI (automatic usage based on HP/SP/cooldowns)
     3. Wire passives into `calculateHomunculusStats()`
     4. Wire Instruction Change's +brew% into Pharmacy success rate
     **Estimate:** 6-8 hours. **Files:** `index.js`, `ro_homunculus_data.js`

     ---

     ### T2-8. Pharmacy — Crafting System (ID 1800)

     **Current:** Stub.
     **RO Classic:** Opens crafting interface. 13+ recipes (Red/Yellow/White/Blue Potion, Alcohol, Acid Bottle,
     Bottle Grenade, Plant Bottle, Marine Sphere Bottle, Glistening Coat, Anodyne, Aloevera, Embryo). Consumes
     Medicine Bowl + ingredients on every attempt. Success rate formula:
     ```
     Rate% = PharmacyLv*3 + PotionResearchLv*1 + floor(JobLv*0.2) + floor(DEX*0.1) + floor(LUK*0.1) +
     floor(INT*0.05) + ItemDifficultyModifier
     ```
     **Fix:**
     1. Define `PHARMACY_RECIPES` constant with all recipes, materials, difficulty modifiers
     2. Add handler: validate recipe, check materials, roll success, consume materials (always), create product (on
     success)
     3. Client: crafting UI popup showing available recipes based on ingredients in inventory
     4. Wire Potion Research passive into success rate
     **Estimate:** 4-5 hours (server) + 3-4 hours (client UI). **Files:** `index.js`, new client subsystem

     ---

     ## Tier 3 — Low Priority / Deferred

     Require major new systems that don't exist yet. Document expected behavior for future implementation.

     ### T3-1. Party System (blocks 12+ skills)

     **Skills blocked:** Redemptio (1018), Ki Translation (1614), all 8 Ensemble skills, Potion Pitcher ally
     targeting, CP x4 ally targeting, Devotion party validation, Gangster's Paradise proximity.
     **RO Classic party mechanics:**
     - Max 12 members
     - EXP sharing: "Each Take" (killer gets all) or "Even Share" (split among nearby members within 15 levels)
     - Party buff range: ~31x31 cells (~1550 UE)
     - Leader can change sharing mode, kick members
     - Party chat channel
     - Party window shows member HP bars

     ---

     ### T3-2. Ensemble System (8 Bard + 8 Dancer skills)

     **Requires:** Party system first.
     **RO Classic ensemble rules:**
     - Both Bard AND Dancer in same party, adjacent (1 cell), correct weapons (instrument/whip)
     - Effective level = floor((BardLv + DancerLv) / 2)
     - AoE: 9x9 cells centered between performers
     - Both drain SP independently
     - Both immobilized during ensemble
     - Either can cancel (Adaptation, weapon switch, stun, death)

     **8 Ensemble skills:**

     | Skill | Effect |
     |-------|--------|
     | Lullaby | Sleep enemies in AoE |
     | Mr. Kim A Rich Man | +20-60% EXP from kills in AoE |
     | Eternal Chaos | Zero VIT-based soft DEF for enemies |
     | Drum on Battlefield | +ATK/+DEF for allies (50-150 ATK, 4-12 DEF) |
     | Ring of Nibelungen | +ATK for Weapon Lv4 wielders (75-175 ATK, ignores DEF) |
     | Loki's Veil | Block ALL skill usage in AoE (allies and enemies) |
     | Into the Abyss | Remove gemstone requirements for skills in AoE |
     | Invulnerable Siegfried | +40-80% element resist, +10-50% status resist |

     ---

     ### T3-3. Sitting System (blocks 4+ skills)

     **Skills blocked:** Gangster's Paradise (1715), Spirits Recovery (1606), standard sitting regen bonus.
     **RO Classic sitting mechanics:**
     - Toggle via `/sit` command or Insert key
     - While sitting: HP/SP regen rate DOUBLED (2x multiplier)
     - Cannot move, attack, or use most skills while sitting
     - Can use items while sitting
     - Can be attacked while sitting
     - Visual: character sits on ground
     - Server tracks `player.isSitting` flag
     **Implementation:** Add `player.isSitting` flag, `/sit` toggle event, block movement/attack while sitting,
     double regen tick rate, client animation.

     ---

     ### T3-4. Summon Entity System (Flora + Marine Sphere)

     **Skills blocked:** Summon Flora (1803), Summon Marine Sphere (1807).
     **Required infrastructure:**
     - Entity registry similar to `enemies` Map but for player-summoned creatures
     - Entity HP, position, zone, attack timers, expiry timers
     - Client-side entity spawning, HP bars, death/expiry removal

     **Summon Flora plants by level:**

     | Lv | Plant | Max Count | Duration | HP | ATK |
     |----|-------|-----------|----------|----|-----|
     | 1 | Mandragora | 5 | 300s | 2430 | 26-35 |
     | 2 | Hydra | 4 | 240s | 2630 | 22-28 |
     | 3 | Flora | 3 | 180s | 2830 | 242-273 |
     | 4 | Parasite | 2 | 120s | 3030 | 215-430 |
     | 5 | Geographer | 1 | 60s | 3230 | 467-621 |

     **Marine Sphere:** HP 2400-4000 by level, self-destructs for remaining HP as fire AoE (11x11), ignores DEF.

     ---

     ### T3-5. Vending System (ID 605)

     **Requires:** Cart/Pushcart system.
     **RO Classic:** Merchant opens player shop from cart inventory. Slots = 2 + VendingLv. Custom shop name. 5% tax
      on items >10M zeny. Shop persists while character is logged in (AFK vending). Other players click vendor to
     browse/buy.

     ---

     ### T3-6. Item Identification System (ID 606)

     **RO Classic:** Equipment drops can be "unidentified" — showing generic name, can't be equipped, stats hidden.
     Item Appraisal (SP 10, instant) or Magnifier item (40z consumable) identifies them.

     ---

     ### T3-7. Cart System (ID 607 + Merchant skills)

     **RO Classic:** Pushcart = separate 100-slot / 8000-weight inventory. Required for Vending, Cart Revolution
     (damage scales with cart weight), Change Cart (cosmetic appearance by base level). Cart models: default
     (Lv1-40), wooden (41-65), decorated (66-80), panda (81-90), large wheeled (91-99).

     ---

     ### T3-8. Ground Loot System (blocks Greed)

     **RO Classic:** Items drop on the ground with pickup priority timer. Greed (Blacksmith quest skill) picks up
     all items in 5x5 AoE. Currently all loot goes directly to killer's inventory.

     ---

     ### T3-9. Quest Skill Acquisition

     **RO Classic:** Quest skills are learned from NPCs after completing fetch quests or meeting requirements (not
     from skill tree). Quest skills include: First Aid, Play Dead, Arrow Crafting, Charge Attack, Shrink, Sight
     Blaster, Pang Voice, Charming Wink, Ki Translation, Ki Explosion, Redemptio, Close Confine, Greed, Scribble,
     Create Converter, Elemental Change, Bioethics.
     **Current:** Quest skills are learnable directly from skill tree (no NPC gate).

     ---

     ### T3-10. Abracadabra Random Skill Pool (ID 1416)

     **Current:** Deducts SP but casts no random skill. Cooldown wrong (3000ms, should be 0).
     **RO Classic:** Randomly casts one skill from a weighted pool. Pool includes regular player skills + 13
     exclusive effects (Coma, Class Change, Mono Cell, Rejuvenation, Suicide, Monster Chant, Grim Reaper, etc.). 2
     Yellow Gemstone catalyst.
     **Recommendation:** Implement simplified version first — random bolt/offensive spell from player's learned
     skills. Add exclusive effects incrementally.

     ---

     ### T3-11. Elemental Change (ID 1421)

     **Current:** Stub.
     **RO Classic:** Quest skill (one of 4 variants: Fire/Water/Earth/Wind). Permanently changes monster element to
     Lv1 of chosen type. Success rate: 5-45% base by Endow level + stats. Boss immune. Consumes matching Elemental
     Converter.

     ---

     ### T3-12. Homunculus Evolution

     **RO Classic:** Requires Loyal intimacy (911+) + Stone of Sage item. Random +1 to +10 for each stat. Unlocks
     4th skill. Intimacy resets to 10. Purely endgame feature.

     ---

     ### T3-13. Redemptio (ID 1018)

     **Requires:** Party system.
     **RO Classic:** Quest skill, SP 400, 4s cast. Resurrects ALL dead party members in 15x15 AoE at 50% HP. Caster
     HP/SP set to 1 (does NOT die). EXP penalty: 1% base EXP, reduced by 0.2% per member resurrected (5+ = no
     penalty).

     ---

     ### T3-14. Scribble / Graffiti (ID 1717)

     **RO Classic:** Quest skill, SP 15, 1 Red Gemstone. Places custom text (20 chars) on ground for 180s. Purely
     cosmetic — zero gameplay effect. Lowest priority of all deferred skills.

     ---

     ## Data Corrections

     These are wrong values in `ro_skill_data_2nd.js` that should be fixed regardless of whether the skill handler
     is implemented.

     ### Ensemble Skill Prerequisites (ALL WRONG)

     All 16 ensemble skills currently point to Music Lessons / Dance Lessons as prerequisites instead of their
     correct skill tree parents:

     | Ensemble | Bard Current | Bard Correct | Dancer Current | Dancer Correct |
     |----------|-------------|-------------|----------------|----------------|
     | Lullaby (1530/1550) | Music Lessons 10 | A Whistle (1507) Lv10 | Dance Lessons 10 | Humming (1522) Lv10 |
     | Mr. Kim (1531/1551) | Music Lessons 5 | Inv. Siegfried (1537) Lv3 | Dance Lessons 5 | Inv. Siegfried (1557)
     Lv3 |
     | Eternal Chaos (1532/1552) | Music Lessons 5 | Loki's Veil (1535) Lv1 | Dance Lessons 5 | Loki's Veil (1555)
     Lv1 |
     | Drum (1533/1553) | Music Lessons 5 | Apple of Idun (1508) Lv10 | Dance Lessons 5 | Service for You (1521)
     Lv10 |
     | Nibelungen (1534/1554) | Music Lessons 10 | Drum (1533) Lv3 | Dance Lessons 10 | Drum (1553) Lv3 |
     | Loki's Veil (1535/1555) | Music Lessons 10 | ACoS (1502) Lv10 | Dance Lessons 10 | PDFM (1527) Lv10 |
     | Into Abyss (1536/1556) | Music Lessons 5 | Lullaby (1530) Lv1 | Dance Lessons 5 | Lullaby (1550) Lv1 |
     | Siegfried (1537/1557) | Music Lessons 10 | Bragi (1501) Lv10 | Dance Lessons 10 | Fortune's Kiss (1528) Lv10
     |

     ### Other Data Fixes

     | Skill | Field | Current | Correct | Reason |
     |-------|-------|---------|---------|--------|
     | Abracadabra (1416) | cooldown | 3000 | 0 | rAthena pre-renewal shows 0 for all timing |
     | Magic Rod (1404) | cooldown | 500 | afterCastDelay: 500, cooldown: 0 | Semantic: it's an ACD, not a cooldown
     |
     | Sight Blaster (813) | castTime | 700 | 2000 | Pre-renewal RateMyServer value |
     | Marine Sphere (1807) | effectValue | 2800+400*lv | 2000+400*lv | HP values off by +400 |
     | Lullaby (1530/1550) | maxLevel | 1 | 5 | RO Classic has 5 levels |
     | Ensemble effectValues | various | various | see ensemble section | ATK/EXP/resist values wrong |

     ---

     ## Cross-Cutting Systems Required

     ### Systems That Block Multiple Skills

     | System | Skills Blocked | Estimate | Phase |
     |--------|---------------|----------|-------|
     | **Party System** | 12+ skills (ensembles, Redemptio, Ki Translation, ally targeting) | 40-60 hours | Phase 7
     |
     | **Sitting System** | 4 skills + double regen | 6-8 hours | Phase 7 |
     | **Summon Entity System** | Flora, Marine Sphere | 12-16 hours | Phase 8 |
     | **Crafting UI** | Pharmacy, Create Converter | 4-6 hours | Phase 7 |
     | **Cart/Pushcart System** | Vending, Change Cart | 20-30 hours | Phase 9 |
     | **Ground Loot System** | Greed | 15-20 hours | Phase 10 |
     | **Item Identification** | Item Appraisal | 10-15 hours | Phase 8 |
     | **Quest Skill NPCs** | ~17 quest skills | 8-12 hours | Phase 8 |

     ---

     ## Dependency Graph

     ```
     Party System ──┬── Ensemble Infrastructure ── All 8 Ensemble Skills
                    ├── Redemptio
                    ├── Ki Translation
                    ├── Potion Pitcher (ally targeting)
                    ├── Chemical Protection (ally targeting)
                    ├── Devotion (party validation)
                    └── Gangster's Paradise (+ Sitting)

     Sitting System ──┬── Spirits Recovery
                      ├── Gangster's Paradise (+ Party)
                      └── Double regen bonus (all classes)

     Crafting UI ──┬── Pharmacy (+ recipes)
                   └── Create Elemental Converter

     Summon Entity System ──┬── Summon Flora
                            └── Summon Marine Sphere

     Cart System ──┬── Vending
                   └── Change Cart

     Ground Loot ──── Greed

     Quest System ──┬── Elemental Change (variant selection)
                    ├── Arrow Crafting (already implemented, just needs NPC gate)
                    └── ~15 other quest skills (NPC gate only)
     ```

     ---

     ## Implementation Phases

     ### Phase A: Critical Bug Fixes (Tier 0) — Do First
     **Scope:** 5 fixes, ~3-4 hours total
     1. T0-1: CP UNDISPELLABLE fix (5 min)
     2. T0-2: Wire `checkPlagiarismCopy()` call sites (1-2 hours)
     3. T0-3: Plagiarism DB persistence (1-2 hours)
     4. T0-4: Potion Pitcher VIT formula + Blue Potion INT + Inc HP Recovery step (30 min)
     5. T0-5: Marine Sphere HP data fix (5 min)
     6. Data corrections: ensemble prerequisites, Abracadabra cooldown, Sight Blaster castTime (30 min)

     ### Phase B: High Priority Mechanics (Tier 1) — Next
     **Scope:** 4 systems, ~12-18 hours total
     1. T1-1: Devotion damage redirect (4-6 hours)
     2. T1-2: Magic Rod absorption in magic damage paths (2-3 hours)
     3. T1-3: Homunculus enemy targeting + damage reception (4-6 hours)
     4. T1-4: Moonlit Water Mill / Unbarring Octave effects (2-3 hours)

     ### Phase C: Quick Wins (Tier 2, no new systems) — Then
     **Scope:** 4 skills, ~4-5 hours total
     1. T2-1: Sight Blaster proximity trigger (1-2 hours)
     2. T2-2: Ore Discovery passive (1 hour)
     3. T2-3: Remove Trap Rogue handler (30 min)
     4. T2-4: Weapon Repair handler + armor break (2-3 hours)

     ### Phase D: Medium Systems (Tier 2, minor new infra)
     **Scope:** 3 systems, ~14-19 hours total
     1. T2-5: Create Elemental Converter + crafting popup (3-5 hours)
     2. T2-6: Blade Stop counter system (4-6 hours)
     3. T2-8: Pharmacy crafting system (7-9 hours)

     ### Phase E: Homunculus Completion
     **Scope:** 2 systems, ~10-14 hours total
     1. T2-7: Homunculus skill casting + AI (6-8 hours)
     2. T3-12: Homunculus evolution (4-6 hours)

     ### Phase F: Sitting System + Dependents
     **Scope:** 1 system + 2 skills, ~8-12 hours total
     1. T3-3: Sitting system core (player.isSitting, /sit toggle, 2x regen, client anim) (6-8 hours)
     2. Spirits Recovery passive wiring (1-2 hours)
     3. Gangster's Paradise AI hook (1-2 hours, partial — full needs party)

     ### Phase G: Party System + Dependents (Major Phase)
     **Scope:** 1 major system + many skill unlocks, ~50-70 hours total
     1. Party CRUD, member tracking, syncing, chat channel
     2. EXP sharing modes
     3. Ally targeting for: Potion Pitcher, CP x4, Ki Translation, Redemptio
     4. Ensemble system infrastructure
     5. All 8 ensemble skill handlers
     6. Devotion party validation
     7. Gangster's Paradise full implementation

     ### Phase H: Economy Systems (Major Phase)
     **Scope:** Cart, Vending, Ground Loot, ~55-80 hours total
     1. Cart/Pushcart system (inventory, weight, slots)
     2. Vending player shops
     3. Change Cart cosmetics
     4. Ground loot system
     5. Greed skill

     ### Phase I: Remaining Deferred
     1. Item Identification system + Item Appraisal
     2. Quest Skill NPC acquisition gates
     3. Abracadabra random skill pool (simplified first)
     4. Elemental Change variants
     5. Summon entity system + Flora + Marine Sphere
     6. Scribble (lowest priority)

     ---

     ## Master Checklist

     | # | Item | Tier | Est. Hours | Phase | Status |
     |---|------|------|-----------|-------|--------|
     | T0-1 | CP UNDISPELLABLE | 0 | 0.1 | A | TODO |
     | T0-2 | Plagiarism call sites | 0 | 1.5 | A | TODO |
     | T0-3 | Plagiarism DB persist | 0 | 1.5 | A | TODO |
     | T0-4 | Potion Pitcher formula | 0 | 0.5 | A | TODO |
     | T0-5 | Marine Sphere HP data | 0 | 0.1 | A | TODO |
     | DATA | Ensemble prereqs (16) | 0 | 0.5 | A | TODO |
     | DATA | Abracadabra cooldown | 0 | 0.1 | A | TODO |
     | DATA | Sight Blaster castTime | 0 | 0.1 | A | TODO |
     | T1-1 | Devotion redirect | 1 | 5 | B | TODO |
     | T1-2 | Magic Rod absorption | 1 | 2.5 | B | TODO |
     | T1-3 | Homunculus targeting | 1 | 5 | B | TODO |
     | T1-4 | MWM/UO effects | 1 | 2.5 | B | TODO |
     | T2-1 | Sight Blaster trigger | 2 | 1.5 | C | TODO |
     | T2-2 | Ore Discovery passive | 2 | 1 | C | TODO |
     | T2-3 | Remove Trap Rogue | 2 | 0.5 | C | TODO |
     | T2-4 | Weapon Repair | 2 | 2.5 | C | TODO |
     | T2-5 | Create Converter | 2 | 4 | D | TODO |
     | T2-6 | Blade Stop | 2 | 5 | D | TODO |
     | T2-8 | Pharmacy | 2 | 8 | D | TODO |
     | T2-7 | Homunculus skills | 2 | 7 | E | TODO |
     | T3-12 | Homunculus evolution | 3 | 5 | E | TODO |
     | T3-3 | Sitting system | 3 | 7 | F | TODO |
     | -- | Spirits Recovery | 3 | 1.5 | F | TODO |
     | -- | Gangster's Paradise | 3 | 1.5 | F | TODO |
     | T3-1 | Party system | 3 | 55 | G | TODO |
     | T3-2 | Ensemble system | 3 | 20 | G | TODO |
     | T3-13 | Redemptio | 3 | 2 | G | TODO |
     | -- | Ki Translation | 3 | 1 | G | TODO |
     | -- | Ally targeting x6 | 3 | 4 | G | TODO |
     | T3-7 | Cart system | 3 | 25 | H | TODO |
     | T3-5 | Vending | 3 | 20 | H | TODO |
     | T3-8 | Ground loot | 3 | 17 | H | TODO |
     | -- | Greed | 3 | 2 | H | TODO |
     | T3-6 | Item identification | 3 | 12 | I | TODO |
     | T3-9 | Quest skill NPCs | 3 | 10 | I | TODO |
     | T3-10 | Abracadabra | 3 | 8 | I | TODO |
     | T3-11 | Elemental Change | 3 | 4 | I | TODO |
     | T3-4 | Summon entities | 3 | 14 | I | TODO |
     | T3-14 | Scribble | 3 | 3 | I | TODO |

     **Total estimated: ~250-300 hours across all phases**
     **Phases A-C (critical fixes + quick wins): ~20-25 hours**
     **Phases D-F (medium systems): ~30-40 hours**
     **Phases G-I (major systems): ~200-240 hours**













Skills_Systems_Remediation_Plan

claude --resume b39b5a45-396a-469c-acda-e56500a7f424

● Plan written to docsNew/05_Development/Deferred_Skills_Systems_Remediation_Plan.md.

  Here's the summary of findings across all 42 deferred items:

  5 Critical Bugs (Tier 0 — fix immediately, ~4 hours):
  - CP buffs are dispellable (should be immune)
  - Plagiarism checkPlagiarismCopy() is dead code — 0 call sites, skill completely broken
  - Plagiarism has no DB persistence — copied skill lost on relog
  - Potion Pitcher uses 1x VIT instead of 2x VIT in heal formula
  - Marine Sphere HP data off by +400 per level

  4 High Priority gaps (Tier 1 — core mechanic missing, ~15 hours):
  - Devotion damage redirect not implemented (cast handler works, damage still hits target)
  - Magic Rod absorption never triggers (buff applies, no check in magic damage paths)
  - Homunculus invincible — enemies can't target them (Amistr tank role broken)
  - Moonlit Water Mill / Unbarring Octave performances drain SP but apply no effect

  8 Medium Priority (Tier 2 — implementable now, ~30 hours):
  - Sight Blaster, Ore Discovery, Remove Trap Rogue, Weapon Repair, Create Converter, Blade Stop, Homunculus skills,
  Pharmacy

  25+ Low/Deferred items (Tier 3 — need party/sitting/cart/summon systems, ~200+ hours)

  16 ensemble skill data corrections — all prerequisites point to Music/Dance Lessons instead of correct skill tree
  parents.

  The plan is organized into 9 implementation phases (A through I), with Phases A-C being the most impactful for the
  least effort.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
