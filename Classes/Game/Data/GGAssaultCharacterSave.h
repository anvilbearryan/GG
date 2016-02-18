// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/SaveGame.h"
#include "GGAssaultCharacterSave.generated.h"

/**
 * 
 */
UCLASS()
class GG_API UGGAssaultCharacterSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = GGData)
		FString SaveSlotName;
	UPROPERTY(VisibleAnywhere, Category = GGData)
		uint32 UserIndex;
	int32 AssaultHealthModifier;

	UGGAssaultCharacterSave();
};
