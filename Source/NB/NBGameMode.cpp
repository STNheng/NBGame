// Copyright Epic Games, Inc. All Rights Reserved.

#include "NBGameMode.h"
#include "NBCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANBGameMode::ANBGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
