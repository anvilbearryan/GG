// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGRangedAttackComponent.generated.h"

/**
 * Notifier struct sent from the attack component's owner to the server to inform hit events
 */

USTRUCT()
struct FRangedHitNotify
{
	GENERATED_BODY();
	/** The target receiving the damage*/
	UPROPERTY()
		AActor* Target;
	UPROPERTY()
		uint16 DamageBaseMultipliers;
	/** So that simulated clients can receives all damage info */
	UPROPERTY()
		EGGDamageType DamageCategory;
	UPROPERTY()
		FVector_NetQuantize HitVelocity;

	static FORCEINLINE uint16 CompressedDamageLevels(int32 DirectDamageLevel, int32 IndirectDamageLevel)
	{
		uint16 result = (DirectDamageLevel & 255);
		result |= (IndirectDamageLevel & 255) << 8;
		return result;
	}
	FORCEINLINE bool HasValidData() const
	{
		return Target != nullptr && DamageBaseMultipliers != 0;
	}
	FORCEINLINE uint8 GetDirectBaseMultiplier() const
	{
		return (DamageBaseMultipliers & 255);
	}
	FORCEINLINE uint8 GetIndirectBaseMultiplier() const
	{
		return (DamageBaseMultipliers >> 8) & 255;
	}
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
		TArray<TEnumAsByte<ECollisionChannel>> TargetObjectTypes;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> ProjectileObjectType;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 DirectWeaponDamageBase;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 IndirectWeaponDamageBase;
protected:
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
public:
	UGGRangedAttackComponent();

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

	virtual FGGDamageDealingInfo TranslateNotify(const FRangedHitNotify& InHitNotify);
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
