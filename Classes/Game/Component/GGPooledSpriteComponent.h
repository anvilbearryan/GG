// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PaperSpriteComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGPooledSpriteComponent.generated.h"
/**
* A 2-D box GGCharacter sensing component using dynamic delegates for event binding
* TODO: Cache pointer to GameState Character array
*/
UCLASS(Blueprintable, ClassGroup = "GG|Sprite", meta=(BlueprintSpawnableComponent) )
class GG_API UGGPooledSpriteComponent : public UPaperSpriteComponent
{
	GENERATED_BODY()

public:
	int32 Index;
	UGGPooledSpriteComponent();

	void PreCheckout();
	/** Checkin (returns to pool) is set as UFunction to allow timerrs */
	UFUNCTION()
		void PreCheckin();
};
