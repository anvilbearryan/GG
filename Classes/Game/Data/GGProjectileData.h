// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GGProjectileData.generated.h"

/**
 * 
 */
UCLASS()
class GG_API UGGProjectileData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float LaunchSpeed;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		FVector GravityDirection;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float GravityStrength;	
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		UPaperFlipbook* LaunchEffect;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		UPaperSprite* TravelSprite;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		UPaperFlipbook* ImpactEffect;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		uint8 bVelocityDictatesRotation;

private:
	FVector Gravity_Internal;
	
public:
	void RecalculateCaches();

	FORCEINLINE FVector GetGravityVector() const
	{
		return FVector(Gravity_Internal);
	}
};
