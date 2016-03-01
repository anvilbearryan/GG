// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "Paperflipbook.h"
#include "GGAnimBlendspaceData.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = GGAnimation)
class GG_API UGGAnimBlendspaceData : public UDataAsset
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
		UPaperFlipbook* AirRiseFlipbook;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		float AirStillMargin;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* AirStillFlipbook;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* AirFallFlipbook;

	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* DashFlipbook;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* WallSlideFlipbook;
	
public:	
	FORCEINLINE UPaperFlipbook* GetGroundFlipbook(const float InSpeed) const
	{
		return FMath::Abs(InSpeed) > RunMargin ? RunFlipbook : StandFlipbook;
	}
	FORCEINLINE UPaperFlipbook* GetAirFlipbook(const float InSpeed) const
	{
		if (InSpeed > AirStillMargin)
		{
			return AirRiseFlipbook;
		}
		if (InSpeed < -AirStillMargin)
		{
			return AirFallFlipbook;
		}
		return AirStillFlipbook;
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
