# 2-1 and 2-2 skills implementation Phase 6

2-1 and 2-2 skills implementation Phase 6

claude --resume e6e2dc9c-9795-4c3c-8fb8-73e85553b955

  Phase 6 — Deferred (highest complexity):
Do these one by one in order.
  - 6A: Alchemist combat + Potion Pitcher
  - 6B: Alchemist Homunculus AI companion



   ---

     ## PHASE 6: Highest Complexity (Defer)

     ---

     ### Prompt 6A: Alchemist — Combat Skills + Potion Pitcher

     **Dependencies:** Phase 0A, 0B (ground effects)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-items

     Read the full research document: docsNew/05_Development/Alchemist_Class_Research.md
     Read server/src/index.js for Merchant skill handlers
     Read server/src/ro_skill_data_2nd.js for Alchemist skill definitions (IDs 1800-1815)
     Read docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md

     Implement Alchemist COMBAT skills (Homunculus deferred to Prompt 6B):

     OFFENSIVE:
     1. **Acid Terror (1805)** — Armor break attack
        - ATK% per level: 140-300% from research doc
        - Ignores hard DEF (DEF from equipment, not VIT)
        - Force hit (always hits, ignore Flee)
        - Chance to break target's armor: 3-13% per level
        - Chance to inflict Bleeding status: 3-15% per level
        - Consumes 1 Acid Bottle from inventory
        - Implement armor break: if roll succeeds, unequip target's armor temporarily

     2. **Demonstration / Acid Demonstration (1806)** — Ground fire zone
        - Use ground effect system
        - Fire element ground zone, ticks damage over duration
        - ATK% per tick: 120-200% from research doc
        - Chance to break target's weapon: 1-5% per level
        - Duration: 40-60s per level
        - Consumes 1 Bottle Grenade from inventory

     3. **Bio Cannibalize / Summon Flora (1807)** — Summon plant monsters
        - Summon a plant ally at target location
        - Plant type varies by level (1=Mandragora, 2=Hydra, 3=Flora, 4=Parasite, 5=Geographer)
        - Each plant has HP from research doc, attacks enemies
        - Lv5 Geographer heals allies
        - Plants are AI-controlled (basic attack nearest enemy)
        - Implement as simplified enemy-like entities allied to caster
        - Max active: 1 per level

     4. **Sphere Mine / Marine Sphere (1808)** — Summon bomb
        - Summon Marine Sphere at target location
        - Sphere has HP, self-destructs when attacked or after timer
        - Self-destruct: 11x11 AoE fire damage
        - Marine Sphere HP and damage per level from research doc

     SUPPORT:
     5. **Potion Pitcher (1803)** — Throw healing potion at ally
        - Target an ally (or self), heal them based on potion type
        - Potion type by level: Lv1=Red, Lv2=Orange, Lv3=Yellow, Lv4=White, Lv5=Blue
        - Heal amount = base potion heal * effectiveness% per level (110-150%)
        - Consumes the actual potion item from inventory
        - Can target from range (5-9 cells)

     6. **Chemical Protection Helm (1809)**
     7. **Chemical Protection Shield (1810)**
     8. **Chemical Protection Armor (1811)**
     9. **Chemical Protection Weapon (1812)**
        - All 4: prevent equipment from being stripped or broken for duration
        - Duration: 120s * SkillLv from research doc
        - Each consumes 1 Glistening Coat
        - Apply as buff type, check in strip/break handlers

     CRAFTING:
     10. **Pharmacy / Prepare Potion (1802)** — Craft potions
         - Open crafting interface (or use skill on materials)
         - Recipe list from research doc (20+ recipes)
         - Success rate formula: base + DEX/LUK/INT/JobLv contributions + Learning Potion bonus
         - On success: create potion item(s)
         - On failure: materials consumed, no output
         - Consumes: ingredients + Medicine Bowl (or Mortar as tool)

     PASSIVES:
     11. **Axe Mastery (1800)** — ATK bonus with axes
         - +3 ATK per level
         - Add to getPassiveSkillBonuses()

     12. **Potion Research / Learning Potion (1801)** — Brewing success + potion effectiveness
         - +1% brew success rate per level
         - +5% potion heal effectiveness per level
         - Integrate with Pharmacy success and potion healing calc
     ```

     ---

     ### Prompt 6B: Alchemist — Homunculus System

     **Dependencies:** Prompt 6A, requires significant infrastructure

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-companions

     Read the full research document: docsNew/05_Development/Alchemist_Class_Research.md
     (Focus on: Homunculus System Architecture section)
     Read RagnaCloneDocs/12_Pets_Homunculus_Companions.md

     Implement the HOMUNCULUS companion system:

     NOTE: This is the MOST complex system across all second classes. It involves AI
     companions with their own stats, skills, leveling, and behavior. Plan for significant
     development time.

     CORE HOMUNCULUS SYSTEM:
     1. Database: Create homunculus table
        - homunculus_id, owner_character_id, type (lif/amistr/filir/vanilmirth)
        - level, exp, hp, sp, str, agi, vit, int, dex, luk
        - intimacy (0-1000), hunger (0-100)
        - skill_1_level, skill_2_level (each type has 2 skills)
        - is_alive, is_summoned

     2. Homunculus types (base stats from research doc):
        - Lif: Support healer, high VIT/INT
        - Amistr: Tank, high VIT/STR, defense skills
        - Filir: DPS, high AGI/STR, attack skills
        - Vanilmirth: Magic DPS, high INT, random AoE

     3. Stat growth: each level up, roll for stat gains
        - Each stat has min/max growth range per type (from research doc tables)
        - Growth probability tables from research doc

     4. Intimacy system (0-1000):
        - Starts at 100 on creation
        - Feed: +1 per feed (max 1 feed per interval)
        - Die: -20 intimacy, kill summon
        - At 910+ intimacy: can evolve (transform to stronger version)
        - At 0 intimacy: homunculus leaves permanently (DELETED)

     5. Hunger system (0-100):
        - Decreases over time (1 per 60 seconds)
        - Feed to restore hunger
        - If hunger reaches 0: intimacy decreases rapidly

     SKILLS:
     6. **Bioethics (1813)** — Prerequisite passive, unlocks homunculus skills
     7. **Call Homunculus (1814)** — Summon your homunculus
        - Summon stored homunculus into world as entity
        - Spawns near owner, follows owner
        - Requires Embryo item (consumed on first creation, not on re-summon)
     8. **Rest (1815)** — Dismiss homunculus
        - Store homunculus back (HP/SP preserved)
     9. **Resurrect Homunculus** — Revive dead homunculus
        - Consumes Embryo item
        - Revives with partial HP

     HOMUNCULUS AI:
     10. Simplified AI (not full enemy AI):
         - IDLE: follow owner, stay within 3 cells
         - ATTACK: when owner attacks, homunculus attacks same target
         - SKILL: use skills on cooldown (automatic or manual control)
         - RETURN: if too far from owner (>15 cells), teleport back

     11. Homunculus-specific skills (4 types, 2 skills each):
         - Lif: Healing Hands (heal owner), Emergency Avoid (flee buff)
         - Amistr: Castling (swap position with owner), Adamantium Skin (DEF buff)
         - Filir: Moonlight (attack), Flitting (ASPD buff)
         - Vanilmirth: Caprice (random element attack), Chaotic Blessings (random heal/damage)

     12. EXP sharing: Homunculus gets 10% of EXP from kills while summoned
         - Owner gets 90% (reduced from normal 100%)
         - Homunculus levels up independently

     Socket.io events:
     - `homunculus:summon` { characterId, homunculusData }
     - `homunculus:dismiss` { characterId }
     - `homunculus:died` { characterId, homunculusId }
     - `homunculus:levelup` { characterId, homunculusId, newLevel, stats }
     - `homunculus:skill` { characterId, homunculusId, skillId, targetId }
     - `homunculus:move` { characterId, homunculusId, position }

     Mark as TODO: Homunculus evolution system, Vanilmirth random skill targeting


     ```




Audit Results & Fixes

  Bugs Fixed (6):

  ┌─────┬──────────┬──────────────────────────────────────┬─────────────────────────────────────────────────────────┐
  │  #  │ Severity │                Issue                 │                           Fix                           │
  ├─────┼──────────┼──────────────────────────────────────┼─────────────────────────────────────────────────────────┤
  │ 1   │ MEDIUM   │ ASPD 130 hardcoded as 1400ms (should │ Changed to (200 - aspd) * 50 formula                    │
  │     │          │  be 3500ms)                          │                                                         │
  ├─────┼──────────┼──────────────────────────────────────┼─────────────────────────────────────────────────────────┤
  │ 2   │ LOW      │ Level-up if only processes one level │ Changed to while loop                                   │
  │     │          │  per tick                            │                                                         │
  ├─────┼──────────┼──────────────────────────────────────┼─────────────────────────────────────────────────────────┤
  │ 3   │ LOW      │ Homunculus only got EXP from its own │ Moved 10% EXP hook into processEnemyDeathFromSkill() —  │
  │     │          │  kills                               │ now works for ALL kill sources                          │
  ├─────┼──────────┼──────────────────────────────────────┼─────────────────────────────────────────────────────────┤
  │ 4   │ MINOR    │ No inventory:data after Acid Terror  │ Added inventory refresh + weight update                 │
  │     │          │ consumes Acid Bottle                 │                                                         │
  ├─────┼──────────┼──────────────────────────────────────┼─────────────────────────────────────────────────────────┤
  │     │          │ No inventory:data after              │                                                         │
  │ 5   │ MINOR    │ Demonstration consumes Bottle        │ Added inventory refresh + weight update                 │
  │     │          │ Grenade                              │                                                         │
  ├─────┼──────────┼──────────────────────────────────────┼─────────────────────────────────────────────────────────┤
  │ 6   │ MINOR    │ No currentWeight update after CP     │ Added weight subtraction                                │
  │     │          │ consumes Glistening Coat             │                                                         │
  └─────┴──────────┴──────────────────────────────────────┴─────────────────────────────────────────────────────────┘

  Gap Fixed (1):

  - Added homunculus:skill_up socket handler — players can now allocate skill points to slots 1/2/3

  Remaining Deferred Items (accepted):

  - Homunculus invincible (enemies don't target it) — simplified design, documented
  - Evolution system — future phase
  - Skill execution — future phase
  - Zone transition handling — future phase
  - Auto-resummon on reconnect — future phase
  - Client-side actor — future phase

  Verified Correct (no changes needed):

  - All base stats, food IDs, growth tables, derived formulas in ro_homunculus_data.js
  - Database schema matches handler code perfectly
  - Intimacy brackets, feeding mechanics, hunger decay
  - Call/Rest/Resurrect handlers — all logic correct
  - Chemical Protection integration with Divest — correctly blocks strips
  - SKILL_CATALYSTS entries — all 6 correct
  - Passives (Axe Mastery, Potion Research) — correct formulas

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
