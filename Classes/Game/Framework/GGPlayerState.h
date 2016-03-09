// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerState.h"
#include "GGPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GG_API AGGPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** Score in parent class is a float... */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_PlayerScore)
	int32 PlayerScore;
	
	UFUNCTION()
		void OnRep_PlayerScore();
};
