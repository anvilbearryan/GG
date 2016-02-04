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
class UGGAnimatorComponent;
class UGGAIMovementComponent;
class UPaperFlipbookComponent;

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

    /**
     ******** Movement ********
     */
    UPROPERTY(VisibleAnywhere, Category="GGAI")
    UCapsuleComponent* MovementCapsule;
   
    /** The desired travel direction set using AI logic input */
	UPROPERTY(Category ="GGAI|Input", BlueprintReadWrite)
    FVector TravelDirection;

    UGGAIMovementComponent* MovementComponent;
    
    FGGBasePlatform BasePlatform;
    
    void SetMovementBase(UPrimitiveComponent* NewBase, UActorComponent* InMovementComponent);
    
    virtual void WalkingReachesCliff();
    
    UFUNCTION(BlueprintImplementableEvent, Category="GGAI|Movement")
    virtual void OnWalkingReachesCliff();
    
    /**
     ******** AI Behaviour ********
     */
    UPROPERTY(Category="GGAI|State", EditDefaultsOnly, BlueprintReadOnly)
    TEnumAsByte<EGGAIActionState::Type> ActionState;
    
    UFUNCTION(Category="GGAI|State", BlueprintCallable)
    void TransitToActionState(TEnumAsByte<EGGAIActionState::Type> newState);
    UFUNCTION(Category="GGAI|State", BlueprintImplementableEvent)
    void OnStateTransition(EGGAIActionState::Type newState);
    
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
    
    /** Behaviour fields*/
    uint32 bIsBehaviourTickEnabled : 1;
    FTimerHandle BehaviourHandle;
protected:
    void PauseBehaviourTick(float Duration);
private:
    UFUNCTION()
    void EnableBehaviourTick();
protected:
    /** Individual tick methods for different continuous states */
    virtual void TickPatrol();
    virtual void TickPrepareAttack();
    virtual void TickEvade();

public:
    /**
     ******** Character Sensing ********
     */
    UGGCharacterSensingComponent* Sensor;
    AGGCharacter* Target;
    
    /**
     ******** Animator ********
     */
    UPaperFlipbookComponent* FlipbookComponent;
    UGGAnimatorComponent* AnimatorComponent;
    
    /**
     ******** Utility helpers ********
     */
    UFUNCTION(Category="GGAI|Utility", BlueprintPure, BlueprintCallable, meta=(DisplayName="GetTargetLocation"))
    FVector GGGetTargetLocation() const;
    
    float GetHalfHeight() const;
    float GetHalfWidth() const;
    
    static FVector Right;
    static FVector Left;
    UFUNCTION()
    FVector GetPlanarForwardVector() const;
};

namespace GGMovementBaseUtils
{
    /** Ensure that BasedObjectTick ticks after NewBase */
    GG_API void AddTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* NewBase);
    
    /** Remove tick dependency of BasedObjectTick on OldBase */
    GG_API void RemoveTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* OldBase);
    
}