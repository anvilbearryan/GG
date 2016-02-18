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

	/* ******** Attack specifications ******** */
public:
	/** We shouldn't have an 8-hit combo... */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category ="GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*> GroundNormalAttacks;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*> GroundMovingAttacks;
		
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		UGGMeleeAttackData* GroundChargedAttack;
	/** Similarly give 8-hit soft cap */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*> AirNormalAttacks;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		UGGMeleeAttackData* AirChargedAttack;
	
	/* ******** Component states ******** */
private:
	int32 PendingAttackIndex;
	TWeakObjectPtr<UGGMeleeAttackData> LastInitiatedAttack;
	float CurrentTimeStamp;

	uint8 CachedAttackIdentifier;
	uint8 bUsingMovingAttack : 1;
	uint8 bUsingAirAttack : 1;
	
	virtual void PostInitProperties() override;

	/* ******** Input handling interface ******** */
public:
	/** returns the time until the attempt is processed, -1 if denied */
	void LocalAttemptsAttack(bool InIsOnGround, bool InIsCharged, bool InIsMoving);
protected:
	/** For setting chain timed attack */
	uint8 bQueuedAttack :1;
	FTimerHandle AttackQueueHandle;
	uint8 LocalQueuedAttackIdentifier;
	UFUNCTION()
		void UseQueuedAttack();

	/* ******** MeleeAttackComponent interface ******** */	

	/** Actual method that processes an attack instruction, index is used to represent the specific attack used */
	virtual void InitiateAttack(uint8 Identifier) override;
	virtual void FinalizeAttack() override;

	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void HitTarget(AActor* target, uint8 Identifier) override;

	/* SlashAttack utility */
	uint8 GetIndexFromAttackInformation(bool InIsOnGround, bool InIsCharged, bool InIsMoving) const;
	UGGMeleeAttackData* GetAttackDataFromIndex(uint8 Identifier) const;
	const TArray<UGGMeleeAttackData*>* GetAttacksArrayFromIndex(uint8 Identifier) const;
public:
	bool ShouldRemainInState() const;
	virtual UPaperFlipbook* GetCurrentAnimation() const;
	virtual UPaperFlipbook* GetEffectAnimation() const;
};
