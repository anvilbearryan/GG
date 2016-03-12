// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGMinionBase.h"
#include "GGPunchMinion.generated.h"

class UGGFlipbookFlashHandler;
class UGGNpcMeleeAttackComponent;
class UPaperFlipbook;

UCLASS()
class GG_API AGGPunchMinion : public AGGMinionBase
{
	GENERATED_BODY()
	
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
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		float MaxPrepareTime;
	float TimePreparedFor;
	// ********************************

	// Evasion properties 	

	// Start moving towards target if target is further than this range (stop target getting too far away)
	UPROPERTY(Category = "GGAI|Evade", EditAnywhere)
		FVector2D EvasionTriggerRange;
	// Range when we consider target is close enough and stop chasing him in evasion range
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
		float StartupDelay;
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		UPaperFlipbook* AttackFlipbook;
	TWeakObjectPtr<UGGNpcMeleeAttackComponent> AttackComponent;

	// ********************************

	// AActor interface
	virtual void PostInitializeComponents() override;

	// ********************************

	// Movement
	virtual void OnReachWalkingBound() override;

	virtual void OnSensorActivate_Implementation() override;
	virtual void OnSensorAlert_Implementation() override;
	virtual void OnSensorUnalert_Implementation() override;
	virtual void OnSensorDeactivate_Implementation() override;

	virtual void TickAnimation(float DeltaSeconds) override;

	virtual void TickPatrol(float DeltaSeconds) override;
	virtual void TickPrepareAttack(float DeltaSeconds) override;
	virtual void TickEvade(float DeltaSeconds) override;

	/**	For 1-directional behaviour */
	void SyncFlipbookComponentWithTravelDirection();

	virtual void MinionAttack_Internal(uint8 InInstruction) override;

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
