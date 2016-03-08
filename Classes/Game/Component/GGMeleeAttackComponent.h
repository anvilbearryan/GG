// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGMeleeAttackComponent.generated.h"

/**
 *	Notifier struct sent around the network from owner to server who then uses it to generate the damage dealing struct
 *	to actually do the damage to target minions on the network. The damage dealing struct contains the damage dealer's
 *	APlayerState so that damage effect is not duplicated on the local owner who does the damage locally for immediate
 *	response.
 */

USTRUCT()
struct FMeleeHitNotify
{
	GENERATED_BODY();
	/** The target receiving the damage*/
	UPROPERTY()
		AActor* Target;
	/***/
	UPROPERTY()
		uint16 DamageBaseMultipliers;
	/** So that simulated clients can receives all damage info */
	UPROPERTY()
		TEnumAsByte<EGGDamageType::Type> DamageCategory;

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
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 DirectWeaponDamageBase;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 IndirectWeaponDamageBase;
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

	virtual FGGDamageDealingInfo TranslateNotify(const FMeleeHitNotify& InHitNotify);
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
