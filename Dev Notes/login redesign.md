#  can you fully analyze how the login, character select/creation and loading of the character for my current game. This was originally designed

 can you fully analyze how the login, character select/creation and loading of the character for my current game. This was originally designed
  earlier in the project but now that the entire project is more developed, can we recreate these features to properly support the current state
  of the game? (you can use unrealMCP to determine the current usage or look at the documentation or the current c++ code and server code along
  with the DB). Do a deep analysis of what we have currently and determine how this should be improved to be more industry standard and more
  in-line with ragnarok online classic. should we create a launcher for the game to be downloaded? this launcher could have a link to our
  website, wiki, support button, be able to download updates for the game and make sure verything is is sync witht the current version of the
  game. links to my social media for my game, (discord, etc) some background image in the launcher with a welcome screen. a panel on the right
  side for news/updates/patch notes - make a modern launcher design that looks good. is a launcher even needed if eventually we want to launch on
  steam? the launcher button should have  abig play button. see C:\Sabri_MMO\troubleshootin\rolauncher.png pictures for reference. Once clicked
  this should launch the login screen for the game. this should have username and password and a login button (it should have an option to
  remember username), it should also have an exit button to exit the game and the entire background should have a picture/graphic for the
  game.... see C:\Sabri_MMO\troubleshootin pictures\Rologinscreen.png. once you click login, the promp shoudl change to please wait while it
  connects to the server list or provide an error message. see C:\Sabri_MMO\troubleshootin\pleasewait.png. this should load the server lists.
  currently we would only have a single server called local but this should eventually be able to support multiple servers. see
  C:\Sabri_MMO\troubleshootin\serverlist.png. once you select a server, it should display the please wait screen again while loading your
  characters on that servers and show the character select screen, see ref: C:\Sabri_MMO\troubleshootin pictures\characterselect.png. this should
  have slots filled with existing characters, if that character is clicked on it will highlight it and show the stats of that character on the
  right. fully research this character select screen and how it should work in raganarok online classic. there should be a delete button to
  delete the character and a PLAY button to join the game with taht character. this should now load the game with that character. on the
  character select screeen, if you click an empty character slot, it should allow you to create a neww character. it will bring you to a new UI
  to create that new character. this should have a spot for name, show all potential jobs but only allow creation of novice jobs for now. select
  male or female, hair style, and hair color (we can expand these functionalities later but just build the ui to hold these areas for now.) there
  should be an avater visible of the new character being created. see ref : C:\Sabri_MMO\troubleshootin pictures\charactercreation.png. you can
  either click ok to create teh character and go back to the character select screen and the new character should now be part of your character
  list. or click cancel to cancel the ccharacter creation. make sure to fully research everything we have discussed here and compare to ragnarok
  online classic. make sure everything is 100% functional and suggest any design changes to the current setup of the game. make sure there are no
  gaps and all the necessary features of a proper game launch/login/character select/creation and entering world is fully setup and functional.
  be mindful of the current setup where there is a level to handle the login/character select and once you select a character, the next map/level
  loads teh character into the game. is this the proper way to do it or should this be changed- do some research on what is best for an unreal
  engine 5 game keeping in mind that i want you to do most of this without me - ideally should not need any blueprints - but let me know if there
  is anything i need to do - research this and make a full analysis and plan and document this in a .md file - this plan should be fully
  complete and inlcude everything you need to properly build/change/redsign this entire process. i should be able to tell you to run this plan
  and you will load the necessary skills and/or rules to fully do this without any additional instructions.
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
