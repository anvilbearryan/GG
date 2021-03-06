// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"
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
class AActor;

UCLASS(Blueprintable, ClassGroup = "GG|Attack", meta=(BlueprintSpawnableComponent))
class GG_API UGGMeleeAttackComponent : public UActorComponent
{
	GENERATED_BODY()

		DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStatusUpdateEventSignature);

public:
	UGGMeleeAttackComponent();

	virtual void BeginPlay() override;
	//============
	// Netwokring interface
	//============

	/** Local entry point for starting an attack, calls ServerMethod for replication */
	UFUNCTION(BlueprintCallable, Category ="GGAttack|Input")
		void LocalInitiateAttack();

	/**	Server receives attack instruction from client, calls MulticastInitiateAttack for remote replication */
	UFUNCTION(Server, Reliable, WithValidation, Category = "GGAttack|Replication")
		void ServerInitiateAttack();
	bool ServerInitiateAttack_Validate();
	void ServerInitiateAttack_Implementation();

	/**
	* Replicates instruction to remote simulated proxies, unfortunately there is no "except owning client type"
	* RPC so its done in the body manually
	*/
	UFUNCTION(NetMulticast, Reliable, Category = "GGAttack|Replication")
		void MulticastInitiateAttack();
	void MulticastInitiateAttack_Implementation();
	/**
	* Hit detection is done in owning client for co-op smooth experience and reports to the server to handle
	* neccesary change in target state.
	*/
	UFUNCTION()
		void LocalHitTarget(AActor* target);
	//	TODO: Change parameter type to enemy base class
	UFUNCTION(Server, Reliable, WithValidation, Category = "GGAttack|Replication")
		void ServerHitTarget(AActor* target);
	bool ServerHitTarget_Validate(AActor* target);
	void ServerHitTarget_Implementation(AActor* target);

	//	Specification
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		FVector HitboxCentre;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<EGGShape::Type> HitboxShape;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		FVector HitboxHalfExtent;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> HitChannel;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		float StartUp;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		float Active;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		float Cooldown;

	//	Not exposed to blueprint
	FCollisionShape Hitbox;

	//	States
	uint32 bIsReadyToBeUsed : 1;
	uint32 bIsLocalInstruction : 1;
	float TimeLapsed;
	FVector HitboxOffsetMultiplier;
	FTimerHandle StateTimerHandle;

	TArray<AActor*> AffectedEntities;

protected:
	/** Actual method that processes an attack instruction, should not be called directly from owning actor */
	UFUNCTION()
		virtual void InitiateAttack();
	UFUNCTION()
		virtual void FinalizeAttack();

public:
	//	Sub-classes / Owning actors should bind to this delegate for functionality
	UPROPERTY(BlueprintAssignable, Category ="GGAttack")
		FStatusUpdateEventSignature OnInitiateAttack;
	UPROPERTY(BlueprintAssignable, Category ="GGAttack")
		FStatusUpdateEventSignature OnFinalizeAttack;
	/** Where all updates are handled, consider moving the StartUp -> Active part via timer */
	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UFUNCTION(BlueprintCallable, Category ="GGAttack")
		virtual void HitTarget(AActor* target);
};
