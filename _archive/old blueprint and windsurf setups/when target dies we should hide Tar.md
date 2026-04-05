when target dies we should hide TargetInfo on the GameHud widget of the local player

when target dies we need to make sure we stop following/attacking them - make sure this doesn't crash game

game still crashed when target dies while multiple players attacking it



4.1. the character is following the respawned player again... the player is being teleported on their on screen, however, the respawned player is still showing walking back to the respawn location on respawn. please give detailed instructions on how to fix this, what you suggested previously did not fix it







 issues for later- document for now but we can fix these later:

1.GameHud updates should only ever happen for the local player, they should apply to each remote player

 	a. right now the target info hp is being updated for other remote players attacking a different target

2.HoverOverIndicator should be individual for each local player, they show over remote players and enemies but really they should only show for the local players based on the local player hovering/targetting.

3\. probably just hide and dont use the target info in gamehud, just use the targetHealthBar above player and enemy health

