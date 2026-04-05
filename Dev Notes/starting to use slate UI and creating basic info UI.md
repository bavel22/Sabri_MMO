# request to create slate UI:

request to create slate UI:

can you take a look at the referenced screenshot:

create a UI using slate in the style of the screenshot, not exact copy but similar style. you should create something that you 100% can create yourself. if you look at the screenshot you will see that the UI "basic info" contains the player name and the job_class. it also contains hp/sp(mp) bars with number values below the bars. it also contains the base and job lvl and their associated exp bars for the current progress in the level. finally it diplays the weight and zeny (in our game zeny is called zuzucoin). please create this UI in slate to be displayed on the player's UI. this info should all be the local player's info.


all the info on this basic info UI should be populated/updated dynamically. we already have the player name and job_class of the player defined in the database, name never changes and job class changes very very infrequently. We already have HP and SP(mp in our game). currently there are separate UIs used for HP and MP, but i want you to ignore those and construct completely new UIs using slate. the HP and SP should be updated live (if the player takes damage the HP value and bar should be updated. Also both base and job exp bars and values should be updated live evertime there is an exp gain/loss. Also the zeny(zuzucoin) should also be updated everytime there is an update to the player's zuzucoin.

create this UI in a way that the play can drag it around the screen from the top bad and position it where they want on the screen, the default location should be somewhere near the top left of the screen.

You should create this UI 100% using slate, without needing me to be involved. you should connect to existing socket emits/requests/receives.

double check to make sure everything is working properly and there are no gaps in this
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
