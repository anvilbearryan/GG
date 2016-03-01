// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "PaperFlipbook.h"
#include "GGNpcAnimBlendspaceData.generated.h"

/**
 * 
 */
UCLASS()
class GG_API UGGNpcAnimBlendspaceData : public UDataAsset
{
	GENERATED_BODY()			
	/** Sets to large value if entity does not have run animation */
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		float RunMargin;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		float AirStillMargin;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		uint8 bDirectionSensitive_Horizontal : 1;	
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		uint8 bDirectionSensitive_Vertical : 1;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* StandFlipbook;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* RunFlipbook_Forward;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* RunFlipbook_Backward;
		
	/** The flipbook defaults to if not direction sensitive */
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* AirFlipbook_Neutral;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* AirFlipbook_Rise;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* AirFlipbook_Fall;

	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* DeathFlipbook_Standard;
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UPaperFlipbook* DeathFlipbook_Slash;

public:
	FORCEINLINE UPaperFlipbook* GetGroundFlipbook(float InSignedSpeed) const
	{
		if (bDirectionSensitive_Horizontal && !!RunFlipbook_Backward && InSignedSpeed < -RunMargin)
		{
			return RunFlipbook_Backward;
		}
		if (!!RunFlipbook_Forward && FMath::Abs(InSignedSpeed) > RunMargin)
		{
			return RunFlipbook_Forward;
		}
		return StandFlipbook;
	}
	FORCEINLINE UPaperFlipbook* GetAirFlipbook(float InSignedSpeed) const
	{
		if (bDirectionSensitive_Vertical)
		{
			if (!!AirFlipbook_Rise && InSignedSpeed > AirStillMargin)
			{
				return AirFlipbook_Rise;
			}
			if (!!AirFlipbook_Fall && InSignedSpeed < -AirStillMargin)
			{
				return AirFlipbook_Fall;
			}
		}
		return AirFlipbook_Neutral;
	}
	FORCEINLINE UPaperFlipbook* GetStandardDeathFlipbook()
	{
		return !!DeathFlipbook_Standard ? DeathFlipbook_Standard : DeathFlipbook_Slash;
	}
	FORCEINLINE UPaperFlipbook* GetSlashDeathFlipbook()
	{
		return !!DeathFlipbook_Slash ? DeathFlipbook_Slash : DeathFlipbook_Standard;
	}
};
