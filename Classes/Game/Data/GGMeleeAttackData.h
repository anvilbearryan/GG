// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
//#include "Game/Utility/GGFunctionLibrary.h"
#include "Game/Data/GGGameTypes.h"
#include "GGMeleeAttackData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FGGMeleeHitDefinition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		FGGCollisionShapeParser AttackShape;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float Duration;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		FVector HitboxCentre;
	
	FCollisionShape AttackShapeInternal;

	FGGMeleeHitDefinition() {}
};

UCLASS()
class GG_API UGGMeleeAttackData : public UDataAsset
{
	GENERATED_BODY()
	/** 
	* The stages of variation, from StartUp to multiple stages of active, then the Cooldown phase 
	* A visualized timeline is as below:
	* |---StartUp---Active0---...---ActiveN---HardCooldown-------SoftCooldown-------------|
	* |------ComboLag----------ComboWindow-------------------|
	* Note it is possible for PostActiveCancelLag + ComboWindow to be larger than full duration for chaining purposes.
	*/
private:
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float Startup;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		TArray<FGGMeleeHitDefinition> ActiveDefinitions;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float ComboLag;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float ComboWindow;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float HardCooldown;	
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		float SoftCooldown;
	/** Private caches for state calculation through TimeStamp arguement */
	float SumActiveDuration;
	float TimeMark_EndStartup;
	float TimeMark_EndActive;	
	float TimeMark_FullDuration;
	/** The move's duration if a combo is pending */
	float TimeMark_MinDuration;
	float TimeMark_BeginComboable;
	float TimeMark_EndCComboable;
	

public:
	virtual void PostInitProperties() override;

	FORCEINLINE bool IsInComboWindow(const float InTimeStamp) const
	{
		return InTimeStamp <= TimeMark_EndCancellable && InTimeStamp >= TimeMark_BeginCancellable;
	}
};
