// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/BoxComponent.h"
#include "GGNpcCollisionBoxComponent.generated.h"

/**
 * A collision box tied to Npcs that damages players on contact
 */
class AGGCharacter;
UCLASS()
class GG_API UGGNpcCollisionBoxComponent : public UBoxComponent
{
	GENERATED_BODY()
	
	AGGCharacter* Victom;
public:
	UGGNpcCollisionBoxComponent();

	void OnOverlapCharacter(AGGCharacter* InCharacter);
	
	
};
