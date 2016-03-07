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
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Score)
	int32 CurrentScore;
	
	UFUNCTION()
		void OnRep_Score();
};
