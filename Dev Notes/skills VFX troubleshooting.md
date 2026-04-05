# fireball - NS_magma_shot - make this fire from the player and hit on the enemy 

fireball - NS_magma_shot - make this fire from the player and hit on the enemy 

soul strike - NS_magic_bubbles - make these shoot from the player and hit on the enemy - the 1 bubble per level of the skill

fire bolt- Ns_magma_shot_projectile -  strike from above the enemy that you target and come down on the enemy - - this should last longer based on the level of the spell

napalm beat - ns_darK_solo_impact - target on the enemy you hit

Fire Wall  - splineVFX_fire - target where you hit the ground - persists as along as the skill duration

Magnum Break  - NS_Free_Nagic_Buff - target where you hit the ground


 First Aid  - NS_Potion - appear on top of the player - green - large enough to cover player

Bash  - NE_attack01 - appears towards teh enemy that it is used on

Provoke  - P_Enrage_Base - this should appear for 1 second above the enemy head

Endure  -  P_Ice_Proj_charge_01 - plays once on buff cast

Cold Bolt  - P_Elemental_Ice_Proj -  strike from above the enemy that you target and come down on the enemy - - this should last longer based on the level of the spell


Lightning Bolt  NS_Zap_03_Yellow (Vefects) - -  strike from above the enemy that you target and come down on the enemy - - this should last longer based on the level of the spell


Sight -  P_ElementalFire_Lg - - plays once on buff cast

Stone Curse  - NS_dark_Mist - persists as along as the spell duration

Frost Diver  - NS_Ice_Mist - persists as along as the spell duration

Safety Wall  - P_levelUp_Detail - last as long as the safety wall lasts

Thunderstorm - NS_Lightning_Strike (Mixed Magic) - this should last longer based on the level of the spell


Casting Circle - NS_Free_Magic_Circle1 (Free Magic) - this should last the entire duration of the cast.

Warp Portal   - P_AuraCircle_Ice_Base_01  -this should be persistent and always be looping/shown


i dont really like the way you have grouped the shared Niagara system. lets keep this feature as we get more abilities but for now i want you to use the desginated Niagara systems or vfx i told you.





new feature. for spells that have more and more hits as you level them up, the effect should last longer for every level/hit it has/does. you should be able to figure out how long to make it last based on your research on ragnarok online. research this in more depth if you need more clarity. Compare each ability, how long they last (reference your research on ragnarok online), and make sure the visual last an appropriate amount of time, probably the entire duration of the skill.


effect still sometimes missing. they seem to work more often in the dungeon map than prontera south. not sure why, they should always be working no matter what, you really need to find the root cause of this  issue. redesign whatever you need to to make this properly work. if i change zone sometimes the skills effects are showing and then zone back and then they dont show anymore. sometimes they dont show at all until i go to dungeon map.
cold bolt travels to quickly to the enemy and stays alive too log, each cold bolt should be slightly offset in its starting location above the enemy and travel downwards to the enemy so that each projectile is visible individual
fire bolt is still way too small, make this similar size to the user's view as cold bolt (maybe 50 times larger?)
frost diver is still much large, should be maybe 50 time smaller, should be roughtly the same size as the target's mesh
fire wall needs to be cast at the ground location that the player clicks at (replicate teh functionality of safety wall, that is working perfectly) right now firewall sometime sees to cast at teh origin? i dont see anything for provoke when casted, it should be similar size as endure but above the enemy target head.
first aid still seems to have to vfx
i still dont see casting circles for skills like cold bolt while casting.

ensure that targeting is working properly for spells. there are self buffs that should play directly on /above the player, there are targettign spells that should apply to the enemy, and there are ground target spells that should be spawned at the ground location that is clicked.

crash when changing levels:
LoginId:f1f1bacb401c31e12ada5ca0ebda9e17
EpicAccountId:11239aedd1364411828232d0c7ae3575

Unhandled Exception: EXCEPTION_ACCESS_VIOLATION 0x000002491ff67710

UnrealEditor_SocketIOLib!sio::socket::impl::on_close() [C:\Sabri_MMO\client\SabriMMO\Plugins\SocketIOClient\Source\SocketIOLib\Private\sio_socket.cpp:373]
UnrealEditor_SocketIOLib!asio_sockio::detail::wait_handler<std::_Binder<std::_Unforced,void (__cdecl sio::socket::impl::*)(void),sio::socket::impl *> >::do_complete() [C:\Sabri_MMO\client\SabriMMO\Plugins\SocketIOClient\Source\ThirdParty\asio\asio\include\asio\detail\wait_handler.hpp:71]
UnrealEditor_SocketIOLib!asio_sockio::detail::win_iocp_io_context::do_one() [C:\Sabri_MMO\client\SabriMMO\Plugins\SocketIOClient\Source\ThirdParty\asio\asio\include\asio\detail\impl\win_iocp_io_context.ipp:420]
UnrealEditor_SocketIOLib!asio_sockio::io_context::run() [C:\Sabri_MMO\client\SabriMMO\Plugins\SocketIOClient\Source\ThirdParty\asio\asio\include\asio\impl\io_context.ipp:61]
UnrealEditor_SocketIOLib!sio::client_impl<websocketpp::client<websocketpp::config::asio_client> >::run_loop() [C:\Sabri_MMO\client\SabriMMO\Plugins\SocketIOClient\Source\SocketIOLib\Private\internal\sio_client_impl.cpp:261]
UnrealEditor_SocketIOLib!std::thread::_Invoke<std::tuple<std::_Binder<std::_Unforced,void (__cdecl sio::client_impl<websocketpp::client<websocketpp::config::asio_client> >::*)(void),sio::client_impl<websocketpp::client<websocketpp::config::asio_client> > *> () [C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.38.33130\INCLUDE\thread:61]
ucrtbase
kernel32
ntdll

see C:\Sabri_MMO\client\SabriMMO\Saved\Crashes\UECC-Windows-10C51BF94946D43F5EA74DB35BBE256D_0000 for additional logs
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
