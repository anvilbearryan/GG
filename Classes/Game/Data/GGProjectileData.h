// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GGProjectileData.generated.h"

/**
 * 
 */
class UPaperFlipbook;
class UPaperSprite;
UCLASS()
class GG_API UGGProjectileData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		float LaunchSpeed;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		FVector GravityDirection;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		float GravityStrength;	
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		UPaperFlipbook* LaunchEffect;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		UPaperSprite* TravelSprite;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		UPaperFlipbook* ImpactEffect;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		uint8 bVelocityDictatesRotation : 1;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Ranged")
		int32 Penetration;

private:
	FVector Gravity_Internal;
	
public:
	void RecalculateCaches();

	FORCEINLINE FVector GetGravityVector() const
	{
		return Gravity_Internal;
	}
};
