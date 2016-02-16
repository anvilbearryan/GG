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
	UPROPERTY(EditAnywhere, Category = "GGAttack|Melee")
		uint8 bRootsOnUse : 1;	
	/** Private caches for state calculation through TimeStamp arguement */
	float SumActiveDuration;
	float TimeMark_EndStartup;
	float TimeMark_EndActive;	
	float TimeMark_FullDuration;
	/** The move's duration if a combo is pending */
	float TimeMark_MinDuration;
	float TimeMark_BeginComboable;
	float TimeMark_EndComboable;
	

public:
	virtual void PostInitProperties() override;

	FORCEINLINE bool IsRootingMove() const
	{
		return bRootsOnUse;
	}

	FORCEINLINE bool IsInActivePhase(float InTimeStamp)const
	{
		return InTimeStamp >= TimeMark_EndStartup && InTimeStamp < TimeMark_EndActive;
	}
	
	FORCEINLINE const FGGMeleeHitDefinition* GetActiveDefinition(float InTimeStamp) const
	{
		InTimeStamp -= TimeMark_EndStartup;
		int32 ItrLength = ActiveDefinitions.Num();
		for (int32 i = 0; i < ItrLength; i++)
		{
			InTimeStamp -= ActiveDefinitions[i].Duration;
			if (InTimeStamp <= 0.f)
			{
				return &ActiveDefinitions[i];
			}
		}
		return nullptr;
	}

	FORCEINLINE bool ChangesAttackDefinition(float InCurrentTime, float DeltaTime) const
	{
		// we don't check for abnormal arguements, assume InCurrentTime > TimeMark_EndStartUp < TimeMark_EndActive
		InCurrentTime -= TimeMark_EndStartup;
		int32 ItrLength = ActiveDefinitions.Num();
		for (int32 i = 0; i < ItrLength; i++)
		{
			float Duration = ActiveDefinitions[i].Duration;
			if (InCurrentTime >= Duration)
			{
				InCurrentTime -= Duration;
			}
			else
			{
				return InCurrentTime + DeltaTime > Duration;
			}
		}
		return false;
	}

	FORCEINLINE bool IsInComboWindow(float InTimeStamp) const
	{
		return InTimeStamp < TimeMark_EndComboable && InTimeStamp >= TimeMark_BeginComboable;
	}

	FORCEINLINE bool HasPastHardCooldown(float InTimeStamp) const
	{
		return InTimeStamp >= TimeMark_MinDuration;
	}

	FORCEINLINE bool HasPastSoftCooldown(float InTimeStamp) const
	{
		return InTimeStamp >= TimeMark_FullDuration;
	}

	FORCEINLINE float TimeToLaunchCombo(float InTimeStamp) const
	{
		return TimeMark_MinDuration - InTimeStamp;
	}
};
