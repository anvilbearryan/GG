// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameInstance.h"
#include "GGGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class GG_API UGGGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	FStreamableManager StreamableManager;
};
