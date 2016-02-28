// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Framework/GGGameState.h"
#include "Game/Actor/GGSpritePool.h"

AGGGameState::AGGGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{	
	PrimaryActorTick.bCanEverTick = true;
}

void AGGGameState::BeginPlay()
{
	Super::BeginPlay();	
	
	// search actor list for manager objects
	for (TActorIterator<AGGSpritePool> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		LevelSpritePool = *ActorItr;
		break;
	}
}

TArray<AGGCharacter*>& AGGGameState::GetCharacterList()
{
    return CharacterList;
}

void AGGGameState::UpdateCharacterList()
{
    CharacterList.Reset();
    for (TActorIterator<AGGCharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AGGCharacter* character = *ActorItr;
        if (character)
        {
            CharacterList.Add(character);
        }
    }
    GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Red, TEXT("game state is ticking"));
}