// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "GGDamageReceiveComponent.generated.h"


UCLASS( Blueprintable, ClassGroup=("GG|Components"), meta=(BlueprintSpawnableComponent) )
class GG_API UGGDamageReceiveComponent : public UActorComponent
{
	GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHpEventSignature);
    
    int32 Hp_Current;
    float Hp_CurrentEstimate;
    int32 Hp_Recoverable;

    int32 StandardHpRegenPerSecond_Buff;
    int32 StandardHpRegenPerSecond_Debuff;
public:
    UPROPERTY(EditAnywhere, Category="GG|Damage", meta=(DisplayName="Max Hp"))
        int32 Hp_Max;
    UPROPERTY(EditAnywhere, Category="GG|Damage", meta=(DisplayName="Base Hp regen per second"))
        int32 StandardHpRegenPerSecond_Base;
    UPROPERTY(BlueprintAssignable)
        FHpEventSignature OnMaxedHp;
    UPROPERTY(BlueprintAssignable)
        FHpEventSignature OnZeroedHp;
    
	// Sets default values for this component's properties
	UGGDamageReceiveComponent();

    FORCEINLINE int32 GetCurrentHp()
    {
        return Hp_Current;
    }

    UFUNCTION(BlueprintCallable, Category ="GG|Damage")
    void InitializeHpState();
    
    // Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

    /** Entry method for other classes to call, translates the received compressed data to FGGDamageInformation for handling */
    UFUNCTION()
    void HandleDamageData(int32 DamageData);

protected:
    /**  Process damage information struct into this entity */
    UFUNCTION()
    void ApplyDamageInformation(FGGDamageInformation& information);
};
