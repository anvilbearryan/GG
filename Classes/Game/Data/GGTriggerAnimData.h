// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "Game/Data/GGGameTypes.h"
#include "GGTriggerAnimData.generated.h"

/**
 * 
 */
UCLASS()
class GG_API UGGTriggerAnimData : public UDataAsset
{
	GENERATED_BODY()
	
	/** The blended data's size required for state to be non-neutral */
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		float RunMargin;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* StandFlipbook;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		float NormalRunSpeed;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* RunFlipbook;

	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* AirFlipbook;

	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* DashFlipbook;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* WallSlideFlipbook;
	
public:
	FORCEINLINE UPaperFlipbook* GetGroundFlipbook(const float InSpeed) const
	{
		return FMath::Abs(InSpeed) > RunMargin ? RunFlipbook : StandFlipbook;
	}

	FORCEINLINE UPaperFlipbook* GetAirFlipbook() const
	{
		return AirFlipbook;
	}

	FORCEINLINE UPaperFlipbook* GetDashFlipbook() const
	{
		return DashFlipbook;
	}
	
	FORCEINLINE UPaperFlipbook* GetWallSlideFlipbook() const
	{
		return WallSlideFlipbook;
	}
};
