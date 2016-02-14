// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Component/GGMeleeAttackComponent.h"
#include "Game/Data/GGMeleeAttackData.h"
#include "GGSlashAttackComponent.generated.h"

/**
 * Weapon class handling our assault character's normal attack, consist of multiple combo attack settings to be used
 */
UCLASS()
class GG_API UGGSlashAttackComponent : public UGGMeleeAttackComponent
{
	GENERATED_BODY()
public:
	/** We shouldn't have an 8-hit combo... */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category ="GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*, TInlineAllocator<8>> GroundNormalAttacks;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*, TInlineAllocator<8>> GroundMovingAttacks;
		
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		UGGMeleeAttackData* GroundChargedAttack;
	/** Similarly give 8-hit soft cap */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*, TInlineAllocator<8>> AirNormalAttacks;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		UGGMeleeAttackData* AirChargedAttack;
	
	int32 PendingAttackIndex;
	float CurrentTimeStamp;
	FTimerHandle LaunchTimer;

	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
