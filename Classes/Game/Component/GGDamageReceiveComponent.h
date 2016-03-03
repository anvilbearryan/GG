// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGDamageReceiveComponent.generated.h"


UCLASS( Blueprintable, ClassGroup=("GG|Components"), meta=(BlueprintSpawnableComponent) )
class GG_API UGGDamageReceiveComponent : public UActorComponent
{
	GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHpEventSignature);
	
	UPROPERTY(replicated)
		int32 Hp_Current;
    float Hp_CurrentEstimate;
	int32 Hp_Recoverable;

    int32 StandardHpRegenPerSecond_Buff;
    int32 StandardHpRegenPerSecond_Debuff;

public:
    UPROPERTY(EditAnywhere, replicated, Category="GG|Damage", meta=(DisplayName="Max Hp"))
        int32 Hp_Max;
    UPROPERTY(EditAnywhere, Category="GG|Damage", meta=(DisplayName="Base Hp regen per second"))
        int32 StandardHpRegenPerSecond_Base;
    UPROPERTY(BlueprintAssignable)
        FHpEventSignature OnMaxedHp;
    UPROPERTY(BlueprintAssignable)
        FHpEventSignature OnZeroedHp;
    
	// Sets default values for this component's properties
	UGGDamageReceiveComponent();

	void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const;

    UFUNCTION(BlueprintCallable, Category ="GG|Damage")
		void InitializeHpState();
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

    /**  Process damage information struct into this entity */
    UFUNCTION()
		void ApplyDamageInformation(const FGGDamageReceivingInfo& information);

	/** Handy hp getter */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GG|Damage")
		int32 GetCurrentHp() const;
};
