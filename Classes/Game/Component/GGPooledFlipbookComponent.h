// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PaperFlipbookComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGPooledFlipbookComponent.generated.h"
/**
* A 2-D box GGCharacter sensing component using dynamic delegates for event binding
* TODO: Cache pointer to GameState Character array
*/
UCLASS(Blueprintable, ClassGroup = "GG|Sprite", meta=(BlueprintSpawnableComponent) )
class GG_API UGGPooledFlipbookComponent : public UPaperFlipbookComponent
{
	GENERATED_BODY()
};
