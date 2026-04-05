# 2-2 and 2-2 classes research and fixes

2-2 and 2-2 classes research and fixes

can you do a full research on ragnarok online classic and ensure these skills funciton correctly. identify any gaps in what we have, do we need any new systems to handle these skills. make sure you understand 100% what these skills should do, use deep research. leave no gaps. document fully in a .md for implementation later. i want you to do a deep research of rangarok online classic for all of these systems you mentioned. i want you to gather all the information
    necessary on this in order to replicate this in my game. i want you to understand exactly how all of these function for every aspect of the game. full research all these functionalities and make sure you understand 100% how this functionality works and make sure there are zero gaps. make sure to call any skills related to these systems that may already exist and use the documentation that is written already. review all existing code, skills, documentation, etc. can you do the same analysis for the following clases. spawn an agent teams with a member for each class. create a .md file for each class. knight,
Crusader,
Wizard,
Sage,
Hunter,
Bard,
Dancer,
Priest,
Monk,
Assassin,
Rogue,
Blacksmith,
Alchemist

┌───────┬────────────┬──────────────────────────────┬──────────────────────────────────────────────────────────────┐
  │ Agent │   Class    │         Output File          │                         Focus Areas                          │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 1     │ Knight     │ Knight_Class_Research.md     │ Peco mount, Bowling Bash gutter lines, Two-Hand Quicken,     │
  │       │            │                              │ spear skills, Counter Attack                                 │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 2     │ Crusader   │ Crusader_Class_Research.md   │ Shield skills, Grand Cross self-damage, Devotion redirect,   │
  │       │            │                              │ Auto Guard, Reflect Shield, Defender                         │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 3     │ Wizard     │ Wizard_Class_Research.md     │ Ground-effect AoE system, Ice Wall obstacles, Quagmire,      │
  │       │            │                              │ Storm Gust freeze, Meteor Storm                              │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 4     │ Sage       │ Sage_Class_Research.md       │ Endow element system, Dispel, Magic Rod/Spell Breaker, Free  │
  │       │            │                              │ Cast, Auto Spell, Land Protector                             │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 5     │ Hunter     │ Hunter_Class_Research.md     │ Trap system architecture, Falcon system, Beast Strafing,     │
  │       │            │                              │ Ankle Snare, Detect hidden                                   │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 6     │ Bard       │ Bard_Class_Research.md       │ Song/Performance system, Ensemble (Bard+Dancer), SP drain,   │
  │       │            │                              │ movement lock, Bragi                                         │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 7     │ Dancer     │ Dancer_Class_Research.md     │ Dance system, Ensemble counterpart, Please Don't Forget Me,  │
  │       │            │                              │ Ugly Dance, gender lock                                      │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 8     │ Priest     │ Priest_Class_Research.md     │ Magnus Exorcismus, Safety Wall, Kyrie Eleison barrier,       │
  │       │            │                              │ Sanctuary ground heal, Resurrection                          │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 9     │ Monk       │ Monk_Class_Research.md       │ Spirit Sphere system, Combo chain system, Asura Strike,      │
  │       │            │                              │ Steel Body, Body Relocation                                  │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 10    │ Assassin   │ Assassin_Class_Research.md   │ Sonic Blow, Grimtooth, Enchant Poison, Venom skills (dual    │
  │       │            │                              │ wield already implemented)                                   │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 11    │ Rogue      │ Rogue_Class_Research.md      │ Plagiarism skill copy system, Strip Equipment, Backstab      │
  │       │            │                              │ positioning, Tunnel Drive                                    │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 12    │ Blacksmith │ Blacksmith_Class_Research.md │ Forging/Refining crafting system, Cart system, Adrenaline    │
  │       │            │                              │ Rush, Weapon Perfection                                      │
  ├───────┼────────────┼──────────────────────────────┼──────────────────────────────────────────────────────────────┤
  │ 13    │ Alchemist  │ Alchemist_Class_Research.md  │ Homunculus AI companion system, Potion crafting/Pharmacy,    │
  │       │            │                              │ Acid Terror, Bio Cannibalize                                 │
  └───────┴────────────┴──────────────────────────────┴──────────────────────────────────────────────────────────────┘

  Each agent is:
  - Web searching iROWiki, RateMyServer, and RO Classic databases for every skill formula and mechanic
  - Reading our existing ro_skill_data.js, index.js, ro_damage_formulas.js, and parent class audit docs
  - Identifying gaps and new systems needed
  - Writing a comprehensive .md to docsNew/05_Development/

do all these documents contain all the information needed to implement the skills and systems? can you design
  individual prompts that i can give you to fully implement each one of these invidivudal 1 by 1 in the right order.
  determine any systems that are shared and should be built at the time time. define the proper order to run each
  prompt in
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
