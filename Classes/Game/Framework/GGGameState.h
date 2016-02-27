// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameState.h"
#include "Game/Actor/GGCharacter.h"
#include "GGGameState.generated.h"

/**
 *  GameState should be the place to store all the pointer to the level's manager classes that will be used by actors of the level.
 */
class AGGSpritePool;

UCLASS(Blueprintable, ClassGroup="GG|Framework")
class GG_API AGGGameState : public AGameState
{
	GENERATED_BODY()
	
    TArray<AGGCharacter*> CharacterList;
public:
	AGGGameState(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

    TArray<AGGCharacter*>& GetCharacterList();

	AGGSpritePool* LevelSpritePool;

    UFUNCTION(BlueprintCallable, Category ="GG|Framework")
    void UpdateCharacterList();
};
