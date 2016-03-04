// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GGGamePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GG_API AGGGamePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category=DamageEvents)
		void OnLocalCharacterDealDamage();
	UFUNCTION(BlueprintImplementableEvent, Category = DamageEvents)
		void OnLocalCharacterReceiveDamage();
	UFUNCTION(BlueprintImplementableEvent, Category = DamageEvents)
		void OnRemoteCharacterDealDamage();
	UFUNCTION(BlueprintImplementableEvent, Category = DamageEvents)
		void OnRemoteCharacterReceiveDamage();	
};
