# 2-1 and 2-2 skills implementation Phase 5

2-1 and 2-2 skills implementation Phase 5

claude --resume 46fcda86-837e-460d-bc78-92a92b921b35

  Phase 5 — Economy-adjacent:
Do these one by one in order.
  - 5A: Blacksmith combat skills
  - 5B: Rogue (Plagiarism + Strip + combat)
  - 5C: Blacksmith Forging & Refining (economy system)


    ---

     ## PHASE 5: Economy-Adjacent Classes

     ---

     ### Prompt 5A: Blacksmith — Combat Skills

     **Dependencies:** Phase 0A

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff

     Read the full research document: docsNew/05_Development/Blacksmith_Class_Research.md
     Read server/src/index.js for existing Merchant skill handlers
     Read server/src/ro_skill_data_2nd.js for Blacksmith skill definitions (IDs 1200-1230)
     Read docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md

     Implement Blacksmith COMBAT skills only (forging/refining deferred to Prompt 5C):

     BUFFS:
     1. **Adrenaline Rush (1200)** — Party ASPD buff
        - Increases ASPD for caster and party members
        - ONLY works with axes and maces (validate weapon type)
        - Duration per level from research doc
        - Apply as buff, integrate with ASPD calculation
        - Party-wide effect (if party system exists, otherwise self only)

     2. **Power Thrust / Over Thrust (1202)** — ATK% buff
        - Increases ATK by 5% per level (5/10/15/20/25%)
        - Party-wide effect
        - Duration per level from research doc
        - RISK: 0.1% chance per attack to break weapon (weapon break system)
        - Implement weapon break as rare event: on each attack, roll 0.1%, if break,
          unequip weapon and set weapon durability to 0 (or flag as broken)

     3. **Maximize Power (1203)** — Always max ATK
        - Toggle skill: while active, always use max ATK value (no random range)
        - Drains SP per second while active (from research doc)
        - In damage calc: if maximizePowerActive, use maxATK instead of random(minATK, maxATK)

     4. **Weapon Perfection (1201)** — Ignore size penalty
        - While active: weapon size penalty is removed (all size modifiers = 100%)
        - Duration per level from research doc
        - Integrate with size modifier in damage calculation

     OFFENSIVE:
     5. **Hammerfall (1206)** — AoE stun
        - 5x5 AoE around target
        - Stun chance per level from research doc
        - Low damage, primarily CC skill

     PASSIVES (add to getPassiveSkillBonuses()):
     6. **Weaponry Research (1207)** — Flat ATK + HIT bonus
        - +2 ATK per level, +1 HIT per level

     7. **Hilt Binding (1208)** — ATK + status resist
        - +1 ATK per level, +1% status resistance per level

     8. **Skin Tempering (1209)** — Fire + Neutral resistance
        - +4% fire resistance per level (add to element resistance in damage pipeline)
        - +1% neutral resistance per level
        - Implement element resistance check in damage-received pipeline

     Shared skill access: Blacksmith needs Merchant skill tree access (Discount, Overcharge,
     Pushcart, Cart Revolution, Vending, Mammonite, etc.)
     ```

     ---

     ### Prompt 5B: Rogue — Plagiarism + Strip + Combat

     **Dependencies:** Phase 0A

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

     Read the full research document: docsNew/05_Development/Rogue_Class_Research.md
     Read server/src/index.js for Thief skill handlers and hiding system
     Read server/src/ro_skill_data_2nd.js for Rogue skill definitions (IDs 1700-1718)
     Read docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md

     Implement ALL Rogue skills:

     PLAGIARISM SYSTEM:
     1. Add to player object:
        - player.plagiarismSkill = null (or { skillId, skillLevel, skillName })

     2. **Plagiarism (passive)** — Copy last offensive skill used on you
        - When player is hit by a 1st or 2nd class offensive skill:
        - Store that skill in plagiarismSkill (overwrite previous)
        - Copied skill level is capped at Plagiarism level
        - Only offensive skills can be copied (from whitelist in research doc)
        - Cannot copy: 3rd class skills, boss skills, transcendent skills
        - Persist plagiarismSkill in database (survives logout)
        - Hook into damage-received pipeline: after taking skill damage, check if copyable

     3. **Using copied skill** — Rogue can use the plagiarized skill
        - When Rogue uses skill:use with the plagiarized skillId, execute it
        - Use the stored level (capped at Plagiarism level)
        - Normal SP cost and cooldown apply
        - Copied skill uses Rogue's own stats for damage calculation

     4. **Preserve (quest skill)** — Prevent Plagiarism overwrite
        - While active: new skills won't overwrite current plagiarismSkill
        - Toggle buff

     STRIP EQUIPMENT SYSTEM:
     5. **Divest Weapon / Strip Weapon (1708)** — Remove target's weapon
     6. **Divest Shield / Strip Shield (1709)** — Remove target's shield
     7. **Divest Armor / Strip Armor (1710)** — Remove target's armor
     8. **Divest Helm / Strip Helm (1711)** — Remove target's helm
        - All 4 follow same pattern:
        - Success rate: 50*(SkillLv+1) + 2*(casterDEX - targetDEX), per 1000
        - Duration: 60000 + 15000*SkillLv + 500*(DEX diff) ms
        - On success: temporarily remove equipment (set equipped_position to null for duration)
        - On expiry: re-equip automatically
        - If target has Chemical Protection for that slot: strip fails
        - Apply as debuff type 'stripped_weapon' etc.
        - Monster strip effects: ATK -25% (weapon), DEF -15% (shield), VIT -40% (armor), INT -40% (helm)

     9. **Full Strip (if Rogue or Stalker)** — Strip all 4 at once
        - Combined check, each slot rolls independently

     OFFENSIVE:
     10. **Back Stab (1701)** — Behind-target attack
         - Must be behind target (check facing direction vs caster position)
         - Always hits (ignore Flee)
         - Dagger: 2 hits, Bow: 50% damage
         - Can be used from Hiding
         - 340-700% ATK per level from research doc

     11. **Raid (1703)** — AoE from Hiding
         - Must be in Hiding
         - 3x3 AoE around caster
         - 140-300% ATK (pre-renewal values!)
         - Chance to Stun + Blind
         - Breaks Hiding after use

     12. **Mug / Steal Coin (1706)** — Steal zeny from target
         - Success formula from research doc
         - Gain zeny, deduct from target (monster drops zeny directly)

     13. **Sword Mastery (1700)** — Passive ATK with swords
         - Add to getPassiveSkillBonuses()

     14. **Tunnel Drive (1704)** — Move while hidden
         - Allows movement during Hiding
         - Reduced movement speed per level
         - SP drain while moving in Hiding
         - Hook into movement: if hiding and tunnelDrive learned, allow movement

     15. **Intimidate / Snatch (1705)** — Attack + random teleport both
         - Physical attack, then both caster and target teleport to random location
         - Can be used from Hiding

     PASSIVES:
     16. **Compulsion Discount (1712)** — NPC shop discount
         - Reduce NPC buy prices by 1% per level
         - Hook into shop buy handler

     17. **Gangster's Paradise (1713)** — No aggro while sitting
         - When sitting near 2+ Rogues/Thieves: immune to monster aggro
         - Simple check in enemy AI aggro loop

     18. **Snatcher (1707)** — Auto-steal on attack
         - Chance to auto-steal items on melee attack
         - Uses existing Steal formula from Thief
         - Hook into auto-attack success
     ```

     ---

     ### Prompt 5C: Blacksmith — Forging & Refining System

     **Dependencies:** Prompt 5A (Blacksmith combat skills)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-items, /sabrimmo-economy, /full-stack

     Read the full research document: docsNew/05_Development/Blacksmith_Class_Research.md
     (Focus on: Forging System Architecture, Refining System sections)
     Read server/src/index.js for inventory system

     Implement the FORGING and REFINING systems:

     FORGING SYSTEM:
     1. Forging handler for skill:use with forge skills:
        - Smith Dagger, Smith Sword, Smith Two-Handed Sword, Smith Axe, Smith Mace,
          Smith Knucklebrace, Smith Spear
        - Each creates a specific weapon type

     2. Forging recipe validation:
        - Base material: Iron (for Lv1 weapons), Steel (Lv2), etc.
        - Optional: Element stones (Flame Heart, Mystic Frozen, Rough Wind, Great Nature)
        - Optional: Star Crumbs (1-3, for stat bonuses)
        - Optional: Anvil type affects success rate
        - Consume all materials on attempt (even if fail)

     3. Success rate formula (from rAthena source in research doc):
        - Base rate depends on weapon type and DEX/LUK/JobLv
        - Element stones increase difficulty
        - Star Crumbs increase difficulty
        - Anvil bonus: Iron +0, Golden +10, Oridecon +20, Emperium +50

     4. On success:
        - Create weapon item with forger's name embedded
        - If element stone used: weapon has that element
        - If Star Crumbs used: weapon gets bonus ATK (+5/+10/+15 per crumb)
        - Add to forger's inventory

     5. On failure:
        - All materials consumed, no item created
        - Notify player of failure

     REFINING SYSTEM (can be NPC-triggered):
     6. Refine handler (NPC service or skill):
        - Target: equipment in inventory
        - Material: Phracon (Lv1), Emveretarcon (Lv2), Oridecon (Lv3-4 weapon),
          Elunium (armor)

     7. Refine success rates (per level, from research doc):
        - Safe limit: +4 for armor, +4 for Lv1 weapons, +5 for Lv2, +6 for Lv3, +7 for Lv4
        - Below safe limit: 100% success
        - Above safe limit: decreasing chance per level (from data tables)
        - On failure: equipment is DESTROYED

     8. Refine bonuses:
        - Weapons: +ATK per refine level (varies by weapon level)
        - Armor: +DEF per refine level
        - Over-refine bonus (above safe limit): extra random ATK bonus

     9. Store refineLevel on items in database (character_inventory.refine_level column)

     10. **Repair Weapon** — Fix broken weapons
         - Broken weapons (from Power Thrust break): restore to usable state
         - Material cost from research doc

     DEFERRED (mark as TODO):
     - Greed skill (AoE loot pickup — needs loot drop system)
     - Cart system integration (Cart Revolution damage scaling with cart weight)
     ```




  Audit Results & Fixes

  Issues Found & Fixed:

  1. elementResist not flowing through damage pipeline (5A) — The calculatePhysicalDamage wrapper in index.js built a
  target object but dropped elementResist. Fixed: added elementResist: targetInfo.elementResist || null to the target
  object construction.
  2. Back Stab not using proper forceHit (5B) — Was manually overriding miss results to 50% damage instead of passing
  forceHit: true to the damage calc. Fixed: now passes { forceHit: true } as skillOptions to calculateSkillDamage, which
   properly bypasses FLEE check.
  3. Forged weapon element not persisted (5C) — Element stone choice was logged but not stored in DB. Fixed: added
  forged_element column to DB migration and INSERT query.
  4. Forged weapon star crumb bonus not persisted (5C) — Star crumb ATK (+5/+10/+40) was logged but lost. Fixed: added
  forged_star_crumbs column to DB migration and INSERT query.
  5. Forger name not persisted (5C) — Fixed: added forged_by column to DB migration and INSERT query.
  6. Forged weapon bonuses not loaded on equip (5C) — Weapon load query didn't read forge columns. Fixed: updated query
  to include ci.forged_element, ci.forged_star_crumbs, ci.forged_by, and wired effectiveElement override and
  starCrumbATK addition to weapon ATK on load.
  7. getPlayerInventory not including forge columns (5C) — Fixed: added ci.forged_by, ci.forged_element,
  ci.forged_star_crumbs to SELECT.
  8. Strip Shield hardDefReduction not consumed (5B) — The buff set the value but the damage pipeline never read it.
  Fixed: added hardDefReduction application in ro_damage_formulas.js before hard DEF is applied.
  9. Strip Armor vitMultiplier not consumed (5B) — VIT reduction wasn't affecting soft DEF. Fixed: applied vitMul to
  effectiveSoftDef calculation.
  10. Strip Helm intMultiplier not consumed (5B) — INT reduction wasn't affecting soft MDEF. Fixed: applied intMul to
  effectiveSoftMdef in magical damage pipeline.
  11. vitMultiplier/intMultiplier not in getCombinedModifiers (5B) — Fixed: added both fields to the merged modifier
  object.

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
