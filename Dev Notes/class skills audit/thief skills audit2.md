# Thief Skill Fixes 2

claude --resume 45342877-a0f5-4b2e-a4ba-3fabbf60bc8b


 1. Envenom (Thief, ID 504) — Poison Effect

  Verdict: Server-side works correctly. Poison triggers with (10+4*Lv)% chance, 60s duration, boss/undead immunity, 1.5%
   HP/s drain, -25% DEF, SP regen blocked.

  BUG FOUND & FIXED: No combat log message when you poison an enemy. ChatSubsystem.cpp:645 filtered out all isEnemy=true
   events. Now shows "[Monster] is afflicted with poison" when the caster applies a status effect. Also added targetName
   to server broadcasts for Envenom/Sand Attack/Throw Stone.

  2. Hiding (Thief, ID 503) — Movement Block

  Server-side: Correct. 7-point enforcement is properly implemented — player:position_rejected fires when hidden.

  BUG FOUND & FIXED: Client-side had no movement blocking. The player could visually walk around while hidden (server
  rejected all updates silently). Fixed in 3 places:
  - PlayerInputSubsystem::OnLeftClickFromCharacter — blocks click-to-move/attack/interact
  - PlayerInputSubsystem::OnWalkPollTick — cancels in-progress pathfollowing
  - SabriMMOCharacter::DoMove — blocks WASD movement

  3. Back Slide (Thief, ID 507) — 4 bugs fixed

  ┌───────────────────────────────┬─────────────────────────────────────┬───────────────────────────────────────────┐
  │              Bug              │               Impact                │                    Fix                    │
  ├───────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────────┤
  │ Missing characterId in        │ Teleport silently ignored — skill   │ Added characterId + teleportType to emit  │
  │ player:teleport               │ did nothing                         │                                           │
  ├───────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────────┤
  │ lastDirX/lastDirY never set   │ Always slid in same direction       │ Now computed from position deltas in      │
  │                               │                                     │ player:position handler                   │
  ├───────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────────┤
  │ No server position update     │ Range checks, AI, subsequent skills │ Now updates player.lastX/Y/Z + Redis      │
  │                               │  used wrong position                │ cache                                     │
  ├───────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────────┤
  │ No broadcast to other players │ Other players didn't see the        │ Now broadcasts player:moved to zone       │
  │                               │ movement                            │                                           │
  └───────────────────────────────┴─────────────────────────────────────┴───────────────────────────────────────────┘

  Same characterId bug also fixed in: Acolyte Teleport (2 emits), Rogue Back Stab, Rogue Intimidate.

  4. Play Dead (Novice, ID 3) — Movement Block

  Server-side: Correct. All 7 enforcement points + 3 AI check points + 3 break conditions work properly.

  BUG FOUND & FIXED: Same client-side gap as Hiding — player could visually walk around while playing dead. Fixed in the
   same 3 locations with HasBuff("play_dead") checks.

  Files Changed

  ┌─────────────────────────────────────┬────────────────────────────────────────────────────────────────────────────┐
  │                File                 │                                   Change                                   │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │                                     │ Direction tracking in position handler, Back Slide rewrite,                │
  │ server/src/index.js                 │ Teleport/BackStab/Intimidate characterId fix, targetName on status         │
  │                                     │ broadcasts                                                                 │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │ client/.../ChatSubsystem.cpp        │ Enemy status effect combat log (poison/blind/stun/etc.)                    │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │ client/.../BuffBarSubsystem.h       │ Added HasBuff(FString) public API                                          │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │ client/.../BuffBarSubsystem.cpp     │ Implemented HasBuff()                                                      │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │ client/.../PlayerInputSubsystem.h   │ Added BuffBarSubsystem.h include                                           │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │ client/.../PlayerInputSubsystem.cpp │ Play Dead / Hiding guards in click handler + walk poll                     │
  ├─────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────┤
  │ client/.../SabriMMOCharacter.cpp    │ Play Dead / Hiding guard in WASD movement

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
