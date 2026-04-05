# Plan: Foundation Systems + Crusader Skills Audit Fixes


claude --resume 8325f9cf-2a2a-4fea-a4ae-77dd494c82f5

 Here is Claude's plan:
╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌
 Plan: Foundation Systems + Crusader Skills Audit Fixes

 Context

 Deep audit of all 14 Crusader skills (IDs 1300-1313) + 3 shared Knight skills revealed 21 bugs across the Crusader
 class. However, 6 of those bugs are caused by missing foundational systems that also affect other classes. Fixing
 foundations first ensures Crusader fixes work correctly and benefits all classes simultaneously.

 Audit doc: docsNew/05_Development/Crusader_Skills_Audit_And_Fix_Plan.md

 ---
 Part 1: Foundation System Fixes (6 items, affects ALL classes)

 F1. Equipment Change → Buff Cancellation Hook

 File: server/src/index.js — inventory:equip handler (~line 16597)
 Problem: Unequipping shield/weapon doesn't cancel dependent buffs. Exploitable.
 Fix: After equipment changes complete (~line 17036, after stats recalc), add:
 // Shield-dependent buffs: cancel if no shield
 if (!player.equippedShield && !player.hasShield) {
     for (const sb of ['auto_guard', 'reflect_shield', 'defender', 'shrink']) {
         if (hasBuff(player, sb)) { removeBuff(player, sb); broadcast buff_removed }
     }
 }
 // 2H Sword-dependent: cancel if weapon changed
 if (player.weaponType !== 'two_hand_sword') {
     if (hasBuff(player, 'two_hand_quicken')) { removeBuff; broadcast }
 }
 // 2H Spear-dependent: cancel if weapon changed
 if (player.weaponType !== 'two_hand_spear') {
     if (hasBuff(player, 'spear_quicken')) { removeBuff; broadcast }
 }
 // Axe/Mace-dependent: cancel if weapon changed
 if (!['axe', 'mace', 'one_hand_axe', 'two_hand_axe'].includes(player.weaponType)) {
     if (hasBuff(player, 'adrenaline_rush')) { removeBuff; broadcast }
     if (hasBuff(player, 'adrenaline_rush_full')) { removeBuff; broadcast }
 }

 F2. ASPD Buff Mutual Exclusion

 File: server/src/index.js — Spear Quicken handler (~line 10538), Two-Hand Quicken handler (~line 10662), Adrenaline
 Rush handler (~line 15326)
 Problem: All 3 ASPD buffs can stack. RO Classic allows only one.
 Fix: In each handler, before applying the buff, remove the other two:
 const ASPD_BUFFS = ['spear_quicken', 'two_hand_quicken', 'adrenaline_rush', 'adrenaline_rush_full',
 'one_hand_quicken'];
 for (const ab of ASPD_BUFFS) {
     if (ab !== thisBuffName && hasBuff(player, ab)) { removeBuff; broadcast reason: 'replaced' }
 }

 F3. Enemy Soft DEF/MDEF Computation

 File: server/src/index.js — enemy spawn initialization (~line 3239)
 Problem: Enemies have raw stats but no derived softDef/softMDef. Grand Cross and potentially other skills need these.
 Fix: After enemy stats are copied from template, compute derived values:
 // Compute soft defenses from enemy stats (same formula as players)
 enemy.softDef = Math.floor(enemy.stats.vit * 0.5 + Math.max(0, enemy.stats.vit - 20) * 0.3);
 enemy.softMDef = Math.floor(enemy.stats.int + Math.floor(enemy.stats.vit / 5) + Math.floor(enemy.stats.dex / 5) +
 Math.floor(enemy.level / 4));
 This uses the same pre-renewal formula as players. Also set on any enemy that respawns.

 F4. Shield Weight/Refine Tracking

 File: server/src/index.js — inventory:equip handler, shield equip section (~line 16784)
 Problem: Shield weight and refine not stored. Shield Boomerang needs them.
 Fix: When equipping a shield, store additional fields:
 player.equippedShieldWeight = item.weight || 0;
 player.equippedShieldRefine = item.refine_level || 0;
 player.equippedShield = true;  // (already exists)
 On unequip shield: reset both to 0.
 On player:join initialization: also load from equipped shield in inventory.

 F5. Refine ATK Bonus Calculation

 File: server/src/index.js or server/src/ro_damage_formulas.js
 Problem: Weapon refine level is stored but no ATK bonus calculated.
 Fix: In the damage calculation or in getAttackerInfo(), compute refine bonus:
 const REFINE_ATK = [0, 2, 3, 5, 7]; // per weapon level 0-4
 const refineBonus = (player.equippedWeaponRight?.refineLevel || 0) * (REFINE_ATK[player.weaponLevel] || 2);
 Add refineBonus to the ATK pipeline after DEF reduction (same as rAthena). This applies to ALL physical damage, so all
  melee/ranged classes benefit.

 F6. Decrease AGI Full Buff Strip List

 File: server/src/index.js — Decrease AGI handler (~line 8344)
 Problem: Only strips increase_agi. rAthena strips 10 buffs.
 Fix: Replace single removal with full list:
 const DEC_AGI_STRIPS = ['increase_agi', 'spear_quicken', 'two_hand_quicken', 'one_hand_quicken', 'adrenaline_rush',
 'adrenaline_rush_full', 'cart_boost'];
 for (const sb of DEC_AGI_STRIPS) {
     if (hasBuff(target, sb)) { removeBuff(target, sb); broadcast buff_removed reason: 'decrease_agi' }
 }

 ---
 Part 2: Crusader Skill Fixes (8 phases)

 A. Skill Data Fixes (ro_skill_data_2nd.js) — 8 changes

 All data-only, no handler logic:
 1. Reflect Shield (1307): prereqs [{1301,3},{1305,3}] → [{1305,3}] (remove AG prereq, keep SB Lv3 only)
 2. Heal Crusader (1311): prereqs [{1300,5}] → [{1300,10},{413,5}] (Faith Lv10 + Demon Bane Lv5)
 3. Heal Crusader (1311): afterCastDelay 0 → 1000
 4. Cure Crusader (1312): afterCastDelay 0 → 1000
 5. Providence (1308): afterCastDelay 0 → 3000
 6. Devotion (1306): afterCastDelay 0 → 3000
 7. Defender (1309): afterCastDelay 0 → 800
 8. Spear Quicken (1310): description → '+ASPD with 2H Spear. Requires two-handed spear.'

 B. Grand Cross Full Rewrite (index.js ~line 10163)

 Replace the entire Grand Cross handler. Key changes:
 - ATK = WeaponATK with DEX-based variance + size penalty, NOT StatusATK
 - MATK = use pre-computed gcEffStats.matkMin/matkMax (correct INT/7 and INT/5)
 - DEF = hard DEF as %, soft DEF as flat on ATK. Hard MDEF as %, soft MDEF as flat on MATK
 - AoE = 41-cell diamond (|dx|+|dy| <= 4) instead of 9-cell cross
 - Hit count = 3 ticks with 300ms intervals via setTimeout
 - Self-damage = per-tick: floor(gcDamageVsSelf / 2) * holyVsCaster * faithResist
 - Add refine bonus after DEF reduction
 - Element modifier applied to each target

 C. Shield Boomerang Fix (index.js ~line 10321)

 Replace executePhysicalSkillOnEnemy call with custom damage:
 - Element = force 'neutral' (not weapon element)
 - Damage source = batk (STR-based status ATK) + floor(shieldWeight/10) + shieldRefine*5
 - Does NOT use weapon ATK, masteries, Provoke bonus, Impositio Manus
 - Apply neutral element modifier vs target element
 - Keep Lex Aeterna check, knockback/stun from executePhysicalSkillOnEnemy still needed for emit logic
 - Depends on F4 (shield weight/refine tracking)

 D. Defender ASPD Penalty (index.js + ro_buff_system.js)

 - Handler: Store aspdPenalty: [20,15,10,5,0][learnedLevel-1] in buff
 - getBuffModifiers: mods.aspdMultiplier *= (1 - buff.aspdPenalty / 100)

 E. Shrink Knockback Scaling (index.js ~line 20832)

 - Store Auto Guard skill level in buff: add skillLevel: learnedLevel to auto_guard buff
 - Change enemy pipeline: Math.random() < 0.5 → Math.random() * 100 < 5 * (agBuff.skillLevel || 10)

 F. Devotion Damage Redirect (index.js ~line 20816)

 Insert BEFORE Auto Guard in enemy attack pipeline:
 - Check atkTarget.devotedTo → find Crusader
 - Verify: alive, link not expired, in range
 - If valid: redirect damage to Crusader, target takes 0
 - If out-of-range: break that link
 - If Crusader HP < 25% or dead: break ALL links
 - Remove old incomplete Devotion check at ~line 20942

 G. Devotion Break on CC (index.js)

 In applyStatusEffect() success path, after status is applied:
 - If target has devotionLinks and status is stun/freeze/stone/sleep → break all links

 H. Auto Guard Stores Skill Level (index.js ~line 10333)

 Add skillLevel: learnedLevel to the auto_guard buff so Shrink can read it in Phase E.

 ---
 Execution Order

 Foundation (do first — all classes benefit):
   F1. Equipment change buff cancellation     [index.js]
   F2. ASPD buff mutual exclusion             [index.js]
   F3. Enemy soft DEF/MDEF computation        [index.js]
   F4. Shield weight/refine tracking          [index.js]
   F5. Refine ATK bonus calculation           [index.js or ro_damage_formulas.js]
   F6. Decrease AGI full buff strip           [index.js]

 Crusader skills (do second):
   A.  Skill data fixes                       [ro_skill_data_2nd.js]
   B.  Grand Cross full rewrite               [index.js]  (depends on F3, F5)
   C.  Shield Boomerang fix                   [index.js]  (depends on F4)
   D.  Defender ASPD penalty                  [index.js + ro_buff_system.js]
   E.  Shrink knockback scaling               [index.js]  (depends on H)
   F.  Devotion damage redirect               [index.js]
   G.  Devotion break on CC                   [index.js]
   H.  Auto Guard stores skill level          [index.js]

 Files Modified

 ┌─────────────────────────────────┬──────────────────────────────────┐
 │              File               │             Changes              │
 ├─────────────────────────────────┼──────────────────────────────────┤
 │ server/src/index.js             │ F1-F6, B-H (bulk of work)        │
 ├─────────────────────────────────┼──────────────────────────────────┤
 │ server/src/ro_skill_data_2nd.js │ Phase A (8 data fixes)           │
 ├─────────────────────────────────┼──────────────────────────────────┤
 │ server/src/ro_buff_system.js    │ Phase D (Defender ASPD modifier) │
 └─────────────────────────────────┴──────────────────────────────────┘

 Verification

 After all fixes:
 - Start server with cd server && npm run dev
 - Create a Crusader character, learn skills
 - Test Grand Cross: damage should use WeaponATK + MATK(INT/7 to INT/5), hit 3 times, 41-cell diamond AoE
 - Test Shield Boomerang: damage should use STR + shield weight, always Neutral element
 - Test Defender: Lv1 should reduce ASPD by 20%, Lv5 should have no ASPD penalty
 - Test Shrink: at AG Lv5, knockback should proc ~25% of blocks (not 50%)
 - Test Devotion: damage on ally should redirect to Crusader
 - Test equip shield → Auto Guard → unequip shield → Auto Guard should cancel
 - Test Two-Hand Quicken → Spear Quicken → THQ should be replaced
 - Test Decrease AGI on a buffed player → should strip all ASPD buffs

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
