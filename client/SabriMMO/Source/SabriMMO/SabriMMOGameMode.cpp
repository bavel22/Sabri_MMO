// Copyright Epic Games, Inc. All Rights Reserved.

#include "SabriMMOGameMode.h"
#include "SabriMMOCharacter.h"

ASabriMMOGameMode::ASabriMMOGameMode()
{
	// Do NOT set DefaultPawnClass here.
	// The Level Blueprint spawns + possesses BP_MMOCharacter from saved character data.
	// Setting DefaultPawnClass causes a duplicate pawn at the PlayerStart/origin.
	DefaultPawnClass = nullptr;
}
