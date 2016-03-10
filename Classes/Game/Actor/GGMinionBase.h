// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGDamageableActor.h"
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
class UActorComponent;
class UGGNpcLocomotionAnimComponent;
class UGGAIMovementComponent;
class UPaperFlipbookComponent;
class UGGFlipbookFlashHandler;

UCLASS()
class GG_API AGGMinionBase : public AGGDamageableActor
{
	GENERATED_BODY()

	//********************************
	// Composition	
	static FName CapsuleComponentName;
protected:
	UPROPERTY(VisibleAnywhere, Category = "GGAI")
		UCapsuleComponent* MovementCapsule;
	TWeakObjectPtr<UGGAIMovementComponent> MovementComponent;

	/** Defaults to the first flipbook component in the component hierarchy */
	TWeakObjectPtr<UPaperFlipbookComponent> FlipbookComponent;
	
	TWeakObjectPtr<UGGCharacterSensingComponent> Sensor;

	/** Defaults to the first animator in the component hierarchy */
	TWeakObjectPtr<UGGNpcLocomotionAnimComponent> PrimitiveAnimator;
	
	TWeakObjectPtr<UGGFlipbookFlashHandler> FlashHandler;

	// ********************************
public:
	// Receive damage properties 	
	// TODO: Do minions have damage immune?
	UPROPERTY(EditAnywhere, Category = "GG|Damage")
		float SecondsFlashesOnReceiveDamage;

	//********************************
	//	Movement states    	
	FORCEINLINE UGGAIMovementComponent* GetAIMovement() const
	{
		return MovementComponent.Get();
	}
	/** Traveling direction, pawn MovementInput analogy */
	UPROPERTY(Category = "GGAI|Input", BlueprintReadWrite, Transient)
		FVector TravelDirection;
	FGGBasePlatform BasePlatform;
protected:
	uint8 bReachedWalkingBound : 1;

	//********************************
	//	Character sensing states	
	TWeakObjectPtr<AActor> Target;

	//********************************
	//	Behaviour states
	UPROPERTY(Category = "GGAI|State", EditDefaultsOnly, BlueprintReadOnly, Replicated)
		EGGAIActionState ActionState;
	uint8 bIsBehaviourTickEnabled : 1;
	FTimerHandle BehaviourHandle;
	uint8 AttackInstructionCache;

public:
    // Sets default values for this actor's properties
	AGGMinionBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Method that is called even on late join clients, we bind the delegates here although only the server "should" be aware of those */
	virtual void PostInitializeComponents() override;	

	//********************************
	//	Movement callbacks
	void SetMovementBase(UPrimitiveComponent* NewBase, UActorComponent* InMovementComponent);
	virtual void OnReachWalkingBound();

	//********************************
	//	Damage interface
	virtual void CommenceDamageReaction(const FGGDamageDealingInfo& InDamageInfo) override;
	virtual void OnCompleteDamageReaction() override;
	virtual void CommenceDeathReaction() override;
	virtual void OnCompleteDeathReaction() override;

	//********************************
	//	Sensing callbacks
	UFUNCTION(Category = "GGAI|Task", BlueprintNativeEvent)
		void OnSensorActivate();
	virtual void OnSensorActivate_Implementation();
	UFUNCTION(Category = "GGAI|Task", BlueprintNativeEvent)
		void OnSensorAlert();
	virtual void OnSensorAlert_Implementation();
	UFUNCTION(Category = "GGAI|Task", BlueprintNativeEvent)
		void OnSensorUnalert();
	virtual void OnSensorUnalert_Implementation();
	UFUNCTION(Category = "GGAI|Task", BlueprintNativeEvent)
		void OnSensorDeactivate();
	virtual void OnSensorDeactivate_Implementation();

	//********************************
	//	Npc generic behaviour interface
	virtual void Tick(float DeltaSeconds) override;
private:
	// detail Behviour ticks are separated based on mode and implemented separately in subclasses
	void TickBehaviour(float DeltaSeconds);
	// contains logic for basic data updating
	void TickData_Internal(float DeltaSeconds);
protected:
	// for additional data updating
	virtual void TickData(float DeltaSeconds);
	// Animation is bit complex to interface out, hence virtual for subclassing access
	virtual void TickAnimation(float DeltaSeconds);

	virtual UGGNpcLocomotionAnimComponent* GetActiveLocomotionAnimator();

protected:
	/** Individual tick methods for different continuous states */
	virtual void TickPatrol(float DeltaSeconds);
	virtual void TickPrepareAttack(float DeltaSeconds);
	virtual void TickEvade(float DeltaSeconds);
	void PauseBehaviourTick(float Duration = 0.f);
	UFUNCTION()
		void EnableBehaviourTick();

	//********************************
	//	Npc generic attack interface
	UFUNCTION(NetMulticast, reliable)
		void MulticastAttack(uint8 InInstruction);
	void MulticastAttack_Implementation(uint8 InInstruction);

	virtual void MinionAttack_Internal(uint8 InInstruction);

    //********************************
	//	General utilities         
	static FVector Right;
	static FVector Left;

	UFUNCTION(Category="GGAI|Utility", BlueprintPure, BlueprintCallable, meta=(DisplayName="GetTargetLocation"))
		FVector GGGetTargetLocation() const;
public:    
    float GetHalfHeight() const;
    float GetHalfWidth() const;
       
	FVector GetPlanarForwardVector() const;
};

namespace GGMovementBaseUtils
{
    /** Ensure that BasedObjectTick ticks after NewBase */
    GG_API void AddTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* NewBase);
    
    /** Remove tick dependency of BasedObjectTick on OldBase */
    GG_API void RemoveTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* OldBase);
    
}