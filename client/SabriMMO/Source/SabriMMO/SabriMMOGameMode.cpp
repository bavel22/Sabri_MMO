// Copyright Epic Games, Inc. All Rights Reserved.

#include "SabriMMOGameMode.h"
#include "SabriMMOCharacter.h"

ASabriMMOGameMode::ASabriMMOGameMode()
{
	// Do NOT set DefaultPawnClass here.
	// BP_SocketManager spawns the player character after socket connection.
	// Setting DefaultPawnClass causes a duplicate pawn at the PlayerStart/origin.
	DefaultPawnClass = nullptr;
}
