// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameState.h"
#include "Game/Actor/GGCharacter.h"
#include "GGGameState.generated.h"

/**
 * 
 */
UCLASS()
class GG_API AGGGameState : public AGameState
{
	GENERATED_BODY()
	
    TArray<AGGCharacter*> CharacterList;
public:
    TArray<AGGCharacter*>& GetCharacterList();
	
};
