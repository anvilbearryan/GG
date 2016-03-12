// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGMinionBase.h"
#include "GGMeleeDrone.generated.h"

class UGGFlipbookFlashHandler;
class UPaperFlipbook;

UCLASS()
class GG_API AGGMeleeDrone : public AGGMinionBase
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
	UPROPERTY(Category = "GGAI|Patrol", EditAnywhere)
		FVector2D PatrolRange;
	UPROPERTY(Category = "GGAI|Patrol", EditAnywhere)
		float HoverDistanceZ;
	// Position centred by the patrol
	FVector CentreGuardPosition;
	float DestinationY;
	float CurrentDirectionY;
	float TImeWalkedContinually;

	/** Attack preparation properties */	
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		float AttackMaxDistance;
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		float TurnPauseAim;
	UPROPERTY(Category = "GGAI|PrepareAttack", EditAnywhere)
		float MaxPrepareTime;
	float TimePreparedFor;
	// ********************************

	// Evasion properties 	
	// Since we rely on body collision we don't need to evaed, just stand when need
	UPROPERTY(Category = "GGAI|Evade", EditAnywhere)
		float TurnPauseEvade;
	float TimeEvadedFor;

	// ********************************

	// Attack properties 		

	// = Duration of Evasion phase
	UPROPERTY(Category = "GGAI|Attack", EditAnywhere)
		float AttackCooldown;
	
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

	//********************************
	//	Utils
	/**	For directional flipbook behaviour */
	void SyncFlipbookComponentWithTravelDirection();
		
	bool IsFacingTarget() const;

	float DistanceFromGround() const;

	//********************************
	//	Turning method
	FTimerHandle TurnHandle;

	void SequenceTurnFacingDirection(float TotalTimeToTake, float FlipDelay);
	UFUNCTION()
		void FlipFlipbookComponent();	
};
