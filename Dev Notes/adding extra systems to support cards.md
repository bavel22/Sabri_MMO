# adding extra systems to support cards

adding extra systems to support cards

claude --resume 74b11a9b-46fd-4753-96e7-9a11b3fb7f8a

there are sitll many cards that dont apply the correct stats or special affect. for example zombie master card. are
  you able to understand and implement what this card is supposed to do based on the script, or do we need some other
  way to ensure that all these specialized card with unique effects properly work

Deferred items (20) — need future game systems:

  ┌─────────────────────────────────────────────┬────────────────────────────┐
  │                 Card Effect                 │         Waiting On         │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardIgnoreDefClass (Samurai Spector)        │ DEF bypass formula branch  │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardDefRatioAtkClass (Thanatos)             │ DEF-to-damage conversion   │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardNoWalkDelay (Eddga)                     │ Hit stun system            │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardNoGemStone (Mistress)                   │ Gem consumption system     │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardNoKnockback (RSX-0806)                  │ Knockback system           │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardSplashRange (Baphomet)                  │ Splash auto-attack system  │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardIntravision (Maya Purple)               │ PvP hidden detection       │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardSpeedRate (Moonlight Flower)            │ Server-side movement speed │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardUnbreakableArmor/Weapon                 │ Equipment break system     │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardSpVanishRate (Dark Priest)              │ PvP SP destruction         │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardBreakWeaponRate/ArmorRate (MasterSmith) │ Equipment break system     │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardAutoSpell/WhenHit (26+23 cards)         │ Auto-cast skill trigger    │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardSkillGrant (17 cards)                   │ Card skill management      │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardAddSkillBlow                            │ Knockback on skill         │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardAutobonus (5 cards)                     │ Temp buff proc system      │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardMagicDamageReturn (Maya)                │ Enemy magic attacks        │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardAddMonsterDropItemGroup                 │ Item group lookup          │
  ├─────────────────────────────────────────────┼────────────────────────────┤
  │ cardAddItemGroupHealRate                    │ Item group lookup          │
  └─────────────────────────────────────────────┴────────────────────────────┘

  These 20 are all parsed, stored on the player, and aggregated correctly — they just need the underlying game system to
   exist first. When those systems are built, the card hooks are already in place and just need a single-line read.

✻ Cooked for 11m 33s

❯ i want you to do a deep research of rangarok online classic for all of these systems you mentioned. i want you to
gather all the information
    necessary on this in order to replicate this in my game. i want you to understand exactly how all of these function
 for every aspect of the game. full research all these functionalities and make sure you understand 100% how this
functionality works and make sure tehre are zero gaps. make sure to call any skills related to these systems that may
already exist and use the documentation that is written already. make a full detailed
  plan with everything documented in one or more .md files. this plan should contain all the details you need to then
implement, track progress, and ensure there has been nothing
  missed and there are no gaps. you can defer the PVP systems for now, everything else should be considered




 Implementation Complete — 13/14 Systems Done

  ┌─────────────────┬────────────────────────────────────────────────────────────────────────────────┬─────────────┐
  │     System      │                                 What was built                                 │   Cards     │
  │                 │                                                                                │  activated  │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ D: DEF          │ bIgnoreDefClass skips both DEF types, bDefRatioAtkClass converts DEF to bonus  │ 2           │
  │ Ignore/Damage   │                                                                                │             │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ M: SP Vanish    │ Auto-attack proc destroys target SP %                                          │ 1           │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ A: Walk Delay   │ 46ms movement block on damage, bNoWalkDelay + Endure bypass                    │ 1           │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ J: Movement     │ moveSpeed in player:stats, bSpeedRate formula                                  │ 1           │
  │ Speed           │                                                                                │             │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ K: Intravision  │ canSeeHiddenTarget() utility for hidden visibility                             │ 1           │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ H: Gem          │ SKILL_CATALYSTS table, check+consume in skill:use, bNoGemStone bypass          │ 1           │
  │ Consumption     │                                                                                │             │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ N: Item Groups  │ New ro_item_groups.js, heal rate + drop group hooks                            │ 7           │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │                 │ Full auto-spell system: processCardAutoSpellOnAttack/WhenHit,                  │             │
  │ E: Auto-Cast    │ executeAutoSpellEffect (magic/phys/heal/buff), multi-hit bolts, no             │ 49          │
  │                 │ SP/cast/gems                                                                   │             │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ F: Skill Grants │ cardGrantedSkills map built in rebuild, merged in skill:use handler            │ 17          │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ G: Autobonus    │ processAutobonusOnAttack/WhenHit, parseAutobonusScript, temporary buffs        │ 5           │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ B: Knockback    │ knockbackTarget() function, hooked into Magnum Break, bNoKnockback check       │ 3           │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ C: Splash       │ 3x3 AoE auto-attack, independent damage calc per splash target, no procs on    │ 1           │
  │                 │ splash                                                                         │             │
  ├─────────────────┼────────────────────────────────────────────────────────────────────────────────┼─────────────┤
  │ I: Equipment    │ DB migration, equipment:repair handler, infrastructure for combat break procs  │ 7           │
  │ Break           │                                                                                │             │
  └─────────────────┴────────────────────────────────────────────────────────────────────────────────┴─────────────┘

  Only System L (Magic Reflection — Maya Card) deferred because enemies don't cast magic yet.

  Files modified: index.js, ro_damage_formulas.js, ro_card_effects.js, ro_status_effects.js
  Files created: ro_item_groups.js, database/migrations/add_equipment_break.sql



update documentation and skills. create any new skills with the sabrimmo- prefix that you deem helpful or necessary
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
