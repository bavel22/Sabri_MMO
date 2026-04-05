# Alchemist Skills Audit, Crafting, Summoning Systems

 claude --resume 702d761a-efa0-4bcb-a343-ccc6bdbb2cc2

All skill definitions now match rAthena. Here's the final verification matrix:

  ┌──────────────────────┬───────┬──────┬─────┬──────────┬────────────┬────────────┬───────────┬───────────────┐
  │        Skill         │  SP   │ Cast │ ACD │    CD    │ effectVal  │  Duration  │  Prereq   │ rAthena Match │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1800 Pharmacy        │ 5     │ 0    │ 0   │ 0        │ 3*lv       │ 0          │ 1805 Lv5  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1801 Acid Terror     │ 15    │ 1000 │ 0   │ 0        │ 140+i*40   │ 0          │ 1800 Lv5  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1802 Demonstration   │ 10    │ 1000 │ 0   │ 0        │ 120+i*20   │ 40k+i*5k   │ 1800 Lv4  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1803 Summon Flora    │ 20    │ 2000 │ 0   │ 0        │ i+1        │ 0          │ 1800 Lv6  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1804 Axe Mastery     │ 0     │ 0    │ 0   │ 0        │ 3*lv       │ 0          │ none      │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1805 Potion Research │ 0     │ 0    │ 0   │ 0        │ 5*lv       │ 0          │ none      │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1806 Potion Pitcher  │ 1     │ 0    │ 500 │ 0        │ 110+i*10   │ 0          │ 1800 Lv3  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1807 Marine Sphere   │ 10    │ 2000 │ 0   │ 0        │ 2400+i*400 │ 0          │ 1800 Lv2  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1808 CP Helm         │ 25    │ 2000 │ 0   │ 0        │ 0          │ 120k*(i+1) │ 1800 Lv2  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1809 CP Shield       │ 25    │ 2000 │ 0   │ 0        │ 0          │ 120k*(i+1) │ 1808 Lv3  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1810 CP Armor        │ 25    │ 2000 │ 0   │ 0        │ 0          │ 120k*(i+1) │ 1809 Lv3  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1811 CP Weapon       │ 30    │ 2000 │ 0   │ 0        │ 0          │ 120k*(i+1) │ 1810 Lv3  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1812 Bioethics       │ 0     │ 0    │ 0   │ 0        │ 0          │ 0          │ none      │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1813 Call Homunculus │ 10    │ 0    │ 0   │ 0        │ 0          │ 0          │ 1812+1814 │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1814 Rest            │ 50    │ 0    │ 0   │ 0        │ 0          │ 0          │ 1812 Lv1  │ ✓             │
  ├──────────────────────┼───────┼──────┼─────┼──────────┼────────────┼────────────┼───────────┼───────────────┤
  │ 1815 Resurrect Homu  │ 74-50 │ 2000 │ 0   │ 140k-20k │ 20+i*20    │ 0          │ 1813 Lv1  │ ✓             │
  └──────────────────────┴───────┴──────┴─────┴──────────┴────────────┴────────────┴───────────┴───────────────┘

  All 16 skill definitions now match rAthena pre-renewal skill_db.yml. Bold values are ones that were fixed during
  this audit. Total fixes applied across all sessions:

  ┌─────┬──────────────────────────────────┬───────────────────┬────────────────────────────────┐
  │  #  │               Fix                │       File        │             Source             │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 1   │ Demo prereq Lv2→Lv4              │ skill_data_2nd.js │ rAthena skill_tree.yml         │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 2   │ Demo effectVal 100→120 base      │ skill_data_2nd.js │ rAthena + RateMyServer         │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 3   │ CP Helm SP 20→25                 │ skill_data_2nd.js │ rAthena + RateMyServer         │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 4   │ Resurrect castTime 3000→2000     │ skill_data_2nd.js │ rAthena skill_db.yml           │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 5   │ AT ACD 500→0                     │ skill_data_2nd.js │ rAthena (no AfterCastActDelay) │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 6   │ Demo ACD 500→0                   │ skill_data_2nd.js │ rAthena                        │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 7   │ Pitcher castTime 500→0           │ skill_data_2nd.js │ rAthena (no CastTime)          │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 8   │ CP x4 ACD 500→0                  │ skill_data_2nd.js │ rAthena                        │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 9   │ Flora/Marine ACD 500→0           │ skill_data_2nd.js │ rAthena                        │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 10  │ Catalyst IDs swapped (AT/Demo)   │ index.js          │ rAthena item DB                │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 11  │ CP catalyst 7137→7139            │ index.js          │ rAthena Coating_Bottle         │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 12  │ AT Pneuma check added            │ index.js          │ iRO Wiki Classic               │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 13  │ Demo Lex Aeterna on tick         │ index.js          │ RO mechanic                    │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 14  │ AT armor break 30s→120s          │ index.js          │ RO Classic                     │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 15  │ AT bleeding 60s→120s             │ index.js          │ rAthena Duration2              │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 16  │ Demo tick 500ms→1000ms           │ index.js          │ rAthena Unit/Interval          │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 17  │ CP immune to Dispell             │ index.js          │ iRO Wiki Classic Dispell       │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 18  │ Pitcher VIT scaling              │ index.js          │ RateMyServer + RO Classic      │
  ├─────┼──────────────────────────────────┼───────────────────┼────────────────────────────────┤
  │ 19  │ Potion Research in inventory:use │ index.js          │ RO Classic mechanic            │
  └─────┴──────────────────────────────────┴───────────────────┴────────────────────────────────┘

  18 fixes total. All implemented Alchemist skills (Acid Terror, Demonstration, Potion Pitcher, CP x4, Call
  Homunculus, Rest, Resurrect Homunculus, Axe Mastery, Potion Research) now match RO Classic pre-renewal 100%. The 3
   deferred stubs (Pharmacy, Summon Flora, Summon Marine Sphere) have correct data definitions but no handlers yet —
   they require crafting/summon systems that don't exist.



  /sabrimmo-crafting

  File: ~/.claude/skills/sabrimmo-crafting/SKILL.md

  Documents the complete crafting system:
  - Server: PHARMACY_RECIPES (14 recipes), success rate formula (rAthena-verified), pharmacy:craft handler, Arrow
  Crafting handler
  - Client: CraftingSubsystem (4 EventRouter handlers), SCraftingPopup (reusable popup)
  - Full socket event flow (recipes → popup → craft → result)
  - Recipe data tables, Creation Guide requirements, consumption rules
  - How to add new recipes to either system

  /sabrimmo-summons

  File: ~/.claude/skills/sabrimmo-summons/SKILL.md

  Documents the summon entity system:
  - Runtime registries (activePlants/playerPlants, activeMarineSpheres/playerSpheres)
  - Flora plant data (5 types, HP formula, duration, element, Geographer heal)
  - Marine Sphere detonation (fire 11x11, damage=HP, auto + manual trigger)
  - Combat tick integration (auto-attack 1500ms, expiry check, Geographer 5s heal)
  - Cleanup lifecycle (disconnect, expiry, type replacement)
  - Socket events for spawn/attack/remove/explode
  - How to extend with new summon types


New Systems Built

  1. Crafting System (Phase A0 + A)

  Client: CraftingSubsystem + SCraftingPopup — reusable crafting popup UI that handles both Arrow Crafting (305) and
   Pharmacy (1800). Fullscreen backdrop, scrollable recipe list, success rate display, click-to-craft, status
  messages. Follows SCardCompoundPopup pattern.

  Server: PHARMACY_RECIPES constant (14 recipes), pharmacy:craft socket handler with rAthena success rate formula,
  Medicine Bowl + ingredient consumption on every attempt (success or fail), addItemToInventory() on success.

  2. Summon Entity System (Phase C + D)

  Runtime registries: activePlants + playerPlants (Flora) and activeMarineSpheres + playerSpheres (Marine Sphere) —
  lightweight Maps with no DB persistence (entities are temporary).

  Flora auto-attack tick: 1500ms interval in combat loop, nearest-enemy targeting, element table applied, aggro
  routed to caster. Geographer heals allies below 60% HP every 5s.

  Marine Sphere detonation: Fire 11x11 AoE dealing damage = remaining HP. Two trigger paths — auto-detonate on 30s
  expiry (combat tick) and manual summon:detonate socket event. Element table applied.

  Cleanup hooks: Both systems clean up on owner disconnect.

  3. Ally Targeting for Support Skills (Phase B)

  Upgraded Potion Pitcher and Chemical Protection x4 from self-only to targeting party/guild members. Uses the same
  connectedPlayers.get(targetId) pattern established by Heal and Resurrection. Includes range checks, target socket
  notifications, and target VIT scaling for Potion Pitcher.

❯ how is the ally targeting different than what the acolyte/priest skills use?

● It's not different — it's the same pattern. Potion Pitcher and CP skills now use the exact same
  connectedPlayers.get(targetId) + range check + target socket notification flow that Heal (400), Blessing (402),
  Increase AGI (405), Resurrection (1017), and other Acolyte/Priest support skills already use.

  No new system was built for ally targeting. I just upgraded the Potion Pitcher and CP handlers from their
  hardcoded player (self-only) to the existing ally resolution pattern. "Phase B" was really just ~70 lines of
  wiring, not a new system.

  So to be precise, the actual new systems are only two:

  1. Crafting System (client popup + server Pharmacy handler + recipe data)
  2. Summon Entity System (plant/sphere registries + combat tick auto-attack/detonate + cleanup)

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
