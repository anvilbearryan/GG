// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGMeleeAttackComponent.generated.h"

/**
 * Base class representing a melee component, start with the simplest stab type with BP derived instance 
 * defining the trace area. Whole attack action with this component contains 3 stages, namely
 * StartUp, Active and Cooldown.
 * StartUp: broadcasts OnInitiateAttack delegate, supposingly this should makes the character immobile until  
 *			the move finishes caused by TimeLapsed passing the whole move duration.
 * Active:	during active, this component queries for overlap in tick, using the configured data in BP for 
 *			opposing entities. Results are added to an TArray for ignored entities.
 *			As such, we do not allow for multiple hit on same entity in same action.
 * Cooldown:Time at which this component does nothing, just ticking with TimeLapsed increment & wait for finish
 * The component does not check properly whether it is a local player, instead, whoever manages to call 
 * LocalInitiateAttack is considered as local player. In this project, it would be the owning player character
 * that has input enabled to reach this method.
 */

USTRUCT()
struct FMeleeHitNotify
{
	GENERATED_BODY();
	/** The target receiving the damage*/
	UPROPERTY()
		AActor* Target;	
	UPROPERTY()
		int16 DamageDealt;
	/** So that simulated clients can receives all damage info */
	UPROPERTY()
		TEnumAsByte<EGGDamageType::Type> DamageCategory;

	FORCEINLINE bool HasValidData() const
	{
		return Target != nullptr && DamageDealt > 0;
	}
};

class AActor;
UCLASS(Abstract, Blueprintable, ClassGroup = "GG|Attack", meta=(BlueprintSpawnableComponent))
class GG_API UGGMeleeAttackComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStatusUpdateEventSignature);

public:
	//********************************
	//	Specification
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> DamageChannel;
protected:
	//********************************
	//	States	
	/** Cache containing what we last hit. Note: Only valid on owner and server */
	UPROPERTY(Transient)
		FMeleeHitNotify MostRecentHitNotify;
	
	/** Current attack state, attack requests queued until toggled, this way we do not need to keep passing around the attack identifier */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_AttackToggle)
		uint8 bAttackToggle : 1;	
	/** Cache indicates whether this component has hit test responsibility */
	uint8 bIsLocalInstruction : 1;
	/** Main variable of Communication between server and owning client.
	*	Meaningless by itself, subclasses interprets this value to determine which attack is used by the local player
	*/
	uint8 AttackIdentifier;
	TArray<AActor*, TInlineAllocator<8>> AffectedEntities;

public:
	UGGMeleeAttackComponent();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	//********************************
	// Landing attacks
	/**
	* Hit detection is done in owning client for co-op smooth experience and reports to the server to handle
	* neccesary change in target state.
	*/
	UFUNCTION()
		void LocalHitTarget(const FMeleeHitNotify &InHitNotify);
	//	TODO: Change parameter type to enemy base class
	UFUNCTION(Server, Reliable, WithValidation, Category = "GGAttack|Replication")
		void ServerHitTarget(FMeleeHitNotify OwnerHitNotify);
	bool ServerHitTarget_Validate(FMeleeHitNotify OwnerHitNotify);
	void ServerHitTarget_Implementation(FMeleeHitNotify OwnerHitNotify);

	UFUNCTION()
		virtual void HitTarget(const FMeleeHitNotify& InHitNotify);

	//********************************
	// Launching attacks
	UFUNCTION()
		void OnRep_AttackToggle();
	
	/** Local entry point for starting an attack, calls ServerMethod for replication */
	UFUNCTION()
		void LocalInitiateAttack(uint8 Identifier);
	/**	Server handles attack instruction from client, updates replicated (Skip owner) fields if necessary for replication */
	UFUNCTION(Server, Unreliable, WithValidation, Category = "GGAttack|Replication")
		void ServerInitiateAttack(uint8 Identifier);
	bool ServerInitiateAttack_Validate(uint8 Identifier);
	void ServerInitiateAttack_Implementation(uint8 Identifier);

	virtual void PushAttackRequest() PURE_VIRTUAL(UGGMeleeAttackComponent::PushAttackRequest, );

protected:
	/** Actual method that begins an attack move */
	UFUNCTION()
		virtual void InitiateAttack();
	/** Final exit point of attack move */
	UFUNCTION()
		virtual void FinalizeAttack();

public:
	//	for actual functionality , BPs should bind to this delegate while C++ subclasses overrides the base methods
	UPROPERTY(BlueprintAssignable, Category ="GGAttack")
		FStatusUpdateEventSignature OnInitiateAttack;
	UPROPERTY(BlueprintAssignable, Category ="GGAttack")
		FStatusUpdateEventSignature OnFinalizeAttack;

protected:
	//********************************
	// General utilities
	/** UFUNCTION, so that timers can be set */
	UFUNCTION()
		void SetControllerIgnoreMoveInput();
	UFUNCTION()
		void SetControllerReceiveMoveInput();
};
