// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGCharacter.h"
#include "GGAssaultCharacter.generated.h"

/**
 * 
 */
class UGGSlashAttackComponent;
UCLASS()
class GG_API AGGAssaultCharacter : public AGGCharacter
{
	GENERATED_BODY()
public:

	TWeakObjectPtr<UGGSlashAttackComponent> NormalSlashAttackComponent;
	
	// Override to obtain reference to attack and health components
	virtual void PostInitializeComponents() override;
	
	virtual void ReceiveDamage(int32 DamageData) override;
};
