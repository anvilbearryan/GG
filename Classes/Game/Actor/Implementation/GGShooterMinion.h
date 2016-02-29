// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGMinionBase.h"
#include "GGShooterMinion.generated.h"

/**
 * 
 */
UCLASS()
class GG_API AGGShooterMinion : public AGGMinionBase
{
	GENERATED_BODY()

public:
	virtual void OnReachWalkingBound() override;

	virtual void OnSensorActivate_Implementation() override;
	virtual void OnSensorAlert_Implementation() override;
	virtual void OnSensorUnalert_Implementation() override;
	virtual void OnSensorDeactivate_Implementation() override;

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
	virtual void TickPatrol(float DeltaSeconds) override;

	/** Attack preparation properties */
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		FVector2D AttackMinRange;
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		FVector2D AttackMaxRange;
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		float TurnPauseAim;
	virtual void TickPrepareAttack(float DeltaSeconds) override;

	/** Attack properties */
	// = Duration of Evasion phase
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
	float AttackCooldown;
	FTimerHandle AttackHandle;
	UFUNCTION()
		void SequenceCastAttack();
	UFUNCTION(Category = "GGAI|Attack", BlueprintImplementableEvent)
		void OnCastAttack();
	UFUNCTION()
		void CompleteAttack();
	UFUNCTION(Category="GGAI|Attack", BlueprintImplementableEvent)
		void OnCompleteAttack();

	/** Evasion phase properties */
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
	virtual void TickEvade(float DeltaSeconds) override;

	bool IsTargetInSuppliedRange(const FVector2D& Range) const;

	bool IsFacingTarget() const;

	FTimerHandle TurnHandle;

	void SequenceTurnFacingDirection(float TotalTimeToTake, float FlipDelay);
	UFUNCTION()
		void FlipFlipbookComponent();

	void SyncFlipbookComponentWithTravelDirection();

	virtual void ReceiveDamage(FGGDamageInformation& DamageInfo) override;

	virtual void PlayDeathSequence() override;
};
