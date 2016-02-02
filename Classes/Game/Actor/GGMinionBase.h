// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "GGMinionBase.generated.h"

/**
 * Minion's struct class to contain information on the platform standing on
 */
USTRUCT()
struct FGGBasePlatform
{
    GENERATED_BODY()
    
    UPrimitiveComponent* PlatformPrimitive;
    uint8 bIsDynamic : 1;
    
};
/**
 * AI base class that abstracts the way character sensing notification events are handled
 */
class UGGCharacterSensingComponent;
class AGGCharacter;
class UActorComponent;

UCLASS()
class GG_API AGGMinionBase : public AActor
{
	GENERATED_BODY()

public:	
    static FName CapsuleComponentName;
    // Sets default values for this actor's properties
	AGGMinionBase();

    /** Method that is called even on late join clients, we bind the delegates here although only the server "should" be aware of those */
	virtual void PostInitializeComponents() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;
    
    UPROPERTY(VisibleAnywhere, Category="GGAI")
    UCapsuleComponent* MovementCapsule;
    // not uproperty as its added in Blueprint and cached in PostInitialize to easily use subclasses
    UGGCharacterSensingComponent* Sensor;
	UPROPERTY(Category ="GGAI|Input", BlueprintReadWrite)
    FVector TravelDirection;
    UFUNCTION(Category="GGAI|Task", BlueprintImplementableEvent, BlueprintCallable)
        void SwitchToPatrol();
    UFUNCTION(Category="GGAI|Task", BlueprintImplementableEvent, BlueprintCallable)
        void SwitchToAttackPreparation();
    UFUNCTION(Category="GGAI|Task", BlueprintImplementableEvent, BlueprintCallable)
        void SwitchToAttack();
    UFUNCTION(Category="GGAI|Task", BlueprintImplementableEvent, BlueprintCallable)
        void SwitchToEvade();
    UFUNCTION(Category="GGAI|Task", BlueprintImplementableEvent, BlueprintCallable)
        void SwitchToInactive();
    
    UPROPERTY(Category="GGAI|State", EditDefaultsOnly, BlueprintReadOnly)
    TEnumAsByte<EGGAIActionState::Type> ActionState;
    
    AGGCharacter* Target;
    
    UFUNCTION(Category="GGAI|State", BlueprintCallable)
    void TransitToActionState(TEnumAsByte<EGGAIActionState::Type> newState);
    UFUNCTION(Category="GGAI|State", BlueprintImplementableEvent)
    void OnStateTransition(EGGAIActionState::Type newState);
    
    /** Utility */
    UFUNCTION(Category="GGAI|Utility", BlueprintPure, BlueprintCallable, meta=(DisplayName="GetTargetLocation"))
    FVector GGGetTargetLocation() const;
    
    float GetHalfHeight() const;
    float GetHalfWidth() const;
    
    /** Movement related */
    FGGBasePlatform BasePlatform;
    
    void SetMovementBase(UPrimitiveComponent* NewBase, UActorComponent* MovementComponent);
};

namespace GGMovementBaseUtils
{
    /** Ensure that BasedObjectTick ticks after NewBase */
    GG_API void AddTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* NewBase);
    
    /** Remove tick dependency of BasedObjectTick on OldBase */
    GG_API void RemoveTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* OldBase);
    
}