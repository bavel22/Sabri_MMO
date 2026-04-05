# todo

todo


#16: 
Resizable/moveable widget (reference the aura chat implementation)

17.
 attacking an enemy, then stopping attacking, and attacking it again after it moves, the remote player and local animation still points to teh location you first attacked the enemy

18.
 sometimes hard to start attacking?

COMPLETE*****19.
Hotbar / Quick-Use Slots — Drag consumables to numbered slots (1–9), press key to use. Builds directly on existing inventory drag system. fully saved/loaded from db

COMPLETE*****20.
NPC Shop — Buy/sell items. Needs a new shop:buy / shop:sell server event, NPC actor, and shop widget. Natural inventory extension.

21.
Skills / Abilities — Active skills (beyond auto-attack). Needs a skill bar, cooldown system, and server-side skill handlers.

22.
Quest System — Basic accept/complete quests from NPCs. Needs quest tracking, DB table, and quest log UI

23.
Map Area Expansion — More zones, enemies, loot variety. No new systems needed — just content.

COMPLETED****24.
putting stats in should instantly raise max health/mana accordingly

25.
we need to be able to select the quantity of items we are buying/selling

26.
chatgtp/other ai to create ragnarok online like (chibi style) icon art with transparent background for each item. save and put in /assets. find the matching item_id in database, and add the item id and point to new icon image in DT_ItemIcons.

27.

class and job_class in DB is redundant, need to combine into 1, use job_class probably but need to move class functionality to job_class (character creation?)

COMPELTED***** 28.
Socket Events Added
exp:gain — Sent to killer with base/job EXP gained + current progress
exp:level_up — Broadcast to all players on level up (with chat announcement)
job:change / job:changed / job:error — Class progression system
Next Steps (Client Blueprint)
To display the EXP system in-game, you'll need to:

Bind exp:gain in your Socket.io Blueprint to show floating EXP text (e.g., "+26 Base EXP +20 Job EXP")
Bind exp:level_up to trigger level-up VFX/SFX and show a notification
Parse the exp field from player:stats to populate EXP bars in your HUD
Rebuild C++ to pick up the new FCharacterData fields

28. Add more rows to DT_SkillIcons as you import more skill icon textures. The row name must match the skill ID from ro_skill_data.js.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
