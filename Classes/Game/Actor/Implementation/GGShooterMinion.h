// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGMinionBase.h"
#include "GGShooterMinion.generated.h"

class UGGNpcRangedAttackComponent;
class UGGProjectileData;
class UGGFlipbookFlashHandler;
class UPaperFlipbook;

UCLASS()
class GG_API AGGShooterMinion : public AGGMinionBase
{
	GENERATED_BODY()

public:

	// ********************************
	// Patrol properties 	

	/** Patrol properties
	*	A "patrol" is the action of wandering without a specific target.
	*	In a patrol, the entity travels in the same direction until it reaches a wall where it can no longer go forward.
	*	Possible brakes are taken based on predefined property.
	*/
	UPROPERTY(Category = "GGAI|Patrol", EditAnywhere)
		float PauseDuration;
	UPROPERTY(Category = "GGAI|Patrol", EditAnywhere)
		uint32 bTakesTimeBasedPause : 1;
	UPROPERTY(Category = "GGAI|Patrol", EditAnywhere)
		float TimeBasedPauseInterval;
	UPROPERTY(Category = "GGAI|Patrol", EditAnywhere)
		float TurnPausePatrol;
	float TimeWalkedContinually;

	/** Attack preparation properties */
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		FVector2D AttackMinRange;
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		FVector2D AttackMaxRange;
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		float TurnPauseAim;

	// ********************************

	// Evasion properties 	

	// Range to trigger evasion, should be smaller than EvadeSafeRange
	UPROPERTY(Category = "GGAI|Evade", EditAnywhere)
		FVector2D EvasionTriggerRange;
	// Range when triggered evasion is turned off, should be larger than EvadeTriggerRange
	UPROPERTY(Category = "GGAI|Evade", EditAnywhere)
		FVector2D EvasionSafeRange;
	UPROPERTY(Category = "GGAI|Evade", EditAnywhere)
		float TurnPauseEvade;
	uint32 bIsInActiveEvasionMode : 1;
	float TimeEvadedFor;
	
	// ********************************
	
	// Attack properties 		
	
	// = Duration of Evasion phase
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		float AttackCooldown;
	/** The delay from changing animation to shooting the first projectile */
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		float ShootStartupDelay;
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		int32 NumberOfShotsPerAttack;		
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		float DelayBetweenShots;
	/** Default transform faces right */
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		FVector ShootOffset;

	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		UGGProjectileData* AttackProjectileData;
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		UPaperFlipbook* AttackFlipbook;
	TWeakObjectPtr<UGGNpcRangedAttackComponent> RangedAttackComponent;
	/** This action handle controls possible interrupts, this way no matter what state we are in we only need to clear this timer */
	FTimerHandle ActionHandle;
	int32 CurrentAttackCount;

	// ********************************
	
	// Receive damage properties 	
	// TODO: Do minions have damage immune?
	UPROPERTY(EditAnywhere, Category = "GG|Damage")
		float SecondsFlashesOnReceiveDamage;
	TWeakObjectPtr<UGGFlipbookFlashHandler> FlashHandler;

	// ********************************

	// AActor interface
	virtual void PostInitializeComponents() override;
	
	// ********************************

	// Movement
	virtual void OnReachWalkingBound() override;
	
	// ********************************

	// Damage
	virtual void CommenceDamageReaction(const FGGDamageDealingInfo& InDamageInfo) override;
	virtual void CommenceDeathReaction() override;
	virtual	void OnCompleteDeathReaction() override;

	// ********************************

	// Character sensing
	virtual void OnSensorActivate_Implementation() override;
	virtual void OnSensorAlert_Implementation() override;
	virtual void OnSensorUnalert_Implementation() override;
	virtual void OnSensorDeactivate_Implementation() override;

	// ********************************

	// Animation
	virtual void TickAnimation(float DeltaSeconds) override;

	// ********************************

	// Behaviour
	virtual void TickPatrol(float DeltaSeconds) override;
	virtual void TickPrepareAttack(float DeltaSeconds) override;
	virtual void TickEvade(float DeltaSeconds) override;
		
	/**	For 1-directional behaviour */
	void SyncFlipbookComponentWithTravelDirection();

	// ********************************

	// Attacking
	virtual void MinionAttack_Internal(uint8 InInstruction) override;

	UFUNCTION()
		void ShootForward();
	UFUNCTION()
		void CompleteAttack();

	//********************************

	//	One-way facing enemy utility, potentiall abstract into base
	bool IsTargetInSuppliedRange(const FVector2D& Range) const;

	bool IsFacingTarget() const;

	//********************************
	//	Turning method
	FTimerHandle TurnHandle;

	void SequenceTurnFacingDirection(float TotalTimeToTake, float FlipDelay);
	UFUNCTION()
		void FlipFlipbookComponent();
};
