// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGRangedAttackComponent.generated.h"

/**
 * Base class representing ranged attack component, carries the bag of functions that are needed to take in
 * damage information from spawned projectiles to send to server as RPC. Key timing of events in a shoot
 * includes the StartUp, Cooldown and Retrigger time.
 
 * StartUp: broadcasts OnInitiateAttack delegate, supposingly this should makes the character immobile until  
 *			the move finishes caused by TimeLapsed passing the whole StartUp duration. At the end up Startup,
 *			the projectile is launched.
 * Cooldown:Time after StartUp the OnFinalizeAttack delegate is called.
 * Retrigger:	Activation is not immediately reset on cooldown, instead must wait for Reload until next launch.

 * The component does not check properly whether it is a local player, instead, whoever manages to call 
 * LocalInitiateAttack is considered as local player. In this project, it would be the owning player character
 * that has input enabled to reach this method.
 */

USTRUCT()
struct FRangedHitNotify
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
	UPROPERTY()
		FVector_NetQuantize HitPosition;

	FORCEINLINE bool HasValidData() const
	{
		return Target != nullptr && DamageDealt > 0;
	}
	/*
	UPROPERTY()
	uint8 Coord_0;
	UPROPERTY()
	uint32 Coord_1;
	const float Z_UNIT = 12.5f;
	FORCEINLINE void SetPosition(FVector position)
	{
		Coord_0 = 0;
		Coord_1 = 0;

		int32 Z = FMath::Clamp<int32>(FMath::Round(position.Z / Z_UNIT), -(1 << 13) + 1 , (1 << 13) - 1);
		Coord_0 |= Z < 0 ? (1 << 7) : 0;
		
		int32 Y = position.Y;

	}*/
};

class UGGPooledSpriteComponent;
UCLASS(Abstract, Blueprintable, ClassGroup = "GG|Attack", meta=(BlueprintSpawnableComponent))
class GG_API UGGRangedAttackComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStatusUpdateEventSignature);

public:
	//********************************
	//	Specification
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> DamageChannel;

	//********************************
	//	States
	UPROPERTY(Transient)
		 FRangedHitNotify MostRecentHitNotify;
	/** Unlike melee where latency could drop RepNotify counts, a counter is replicated for ranged to give a chance of catching up without disrupting movement */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_AttackQueue)
		int32 SumAttackQueue;
	/** To work out how far we are actually behind and resync if necessary */
	int32 ProcessedAttackQueue;
	uint8 AttackIdentifier;
	/** Cache indicates whether this component has hit test responsibility */
	uint8 bIsLocalInstruction : 1;

	UGGRangedAttackComponent();

	//============
	// Netwokring interface
	//============

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	//********************************
	// Landing attacks
	/**
	* Hit detection is done in owning client for co-op smooth experience and reports to the server to handle
	* neccesary change in target state.
	*/
	UFUNCTION()
		void LocalHitTarget(const FRangedHitNotify &InHitNotify);
	//	TODO: Change parameter type to enemy base class
	UFUNCTION(Server, Reliable, WithValidation, Category = "GGAttack|Replication")
		void ServerHitTarget(FRangedHitNotify LocalsHitNotify);
	bool ServerHitTarget_Validate(FRangedHitNotify LocalsHitNotify);
	void ServerHitTarget_Implementation(FRangedHitNotify LocalsHitNotify);

	UFUNCTION()
		virtual void HitTarget(const FRangedHitNotify &InHitNotify);

	//********************************
	// Launching attacks
	
	virtual void PushAttackRequest() PURE_VIRTUAL(UGGMeleeAttackComponent::PushAttackRequest, );

	UFUNCTION()
		void OnRep_AttackQueue(int32 OldValue);
	/** Local entry point for starting an attack, calls ServerMethod for replication. Identifier represents shoot direction */
	UFUNCTION()
		void LocalInitiateAttack(uint8 Identifier);
	/**	Server handles attack instruction from client, updates replicated (Skip owner) fields if necessary for replication */
	UFUNCTION(Server, Unreliable, WithValidation, Category = "GGAttack|Replication")
		void ServerInitiateAttack(uint8 Identifier);
	bool ServerInitiateAttack_Validate(uint8 Identifier);
	void ServerInitiateAttack_Implementation(uint8 Identifier);


protected:
	/** Actual method that processes an attack instruction, should not be called directly from owning actor */
	UFUNCTION()
		virtual void InitiateAttack();
	UFUNCTION()
		virtual void FinalizeAttack();

public:
	//	Sub-classes / Owning actors should bind to this delegate in BP for animation state transition
	UPROPERTY(BlueprintAssignable, Category ="GGAttack")
		FStatusUpdateEventSignature OnInitiateAttack;
	UPROPERTY(BlueprintAssignable, Category = "GGAttack")
		FStatusUpdateEventSignature OnFinalizeAttack;
};
