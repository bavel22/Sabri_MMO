// Copyright Epic Games, Inc. All Rights Reserved.

#include "SabriMMOGameMode.h"
#include "SabriMMOCharacter.h"

ASabriMMOGameMode::ASabriMMOGameMode()
{
	DefaultPawnClass = ASabriMMOCharacter::StaticClass();
}
