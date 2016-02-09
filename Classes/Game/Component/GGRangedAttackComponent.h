// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"
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
class AActor;
class UProjectile;

UCLASS(Blueprintable, ClassGroup = "GG|Attack", meta=(BlueprintSpawnableComponent))
class GG_API UGGRangedAttackComponent : public UActorComponent
{
	GENERATED_BODY()

		DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStatusUpdateEventSignature);

public:
	UGGRangedAttackComponent();

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
	UFUNCTION(BlueprintCallable, Category ="GGAttack")
		void LocalHitTarget(AActor* target);
	//	TODO: Change parameter type to enemy base class
	UFUNCTION(Server, Reliable, WithValidation, Category = "GGAttack|Replication")
		void ServerHitTarget(AActor* target);
	bool ServerHitTarget_Validate(AActor* target);
	void ServerHitTarget_Implementation(AActor* target);

	//	Specification
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGAttack|Specification")
		FVector LaunchOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGAttack|Specification")
		TSubclassOf<AActor> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> HitChannel;
	/** Spawn projectile delay */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGAttack|Specification")
		float StartUp;
    /** Animation cooldown. NOTE: concurrent timer with Retrigger */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGAttack|Specification")
		float Cooldown;
    /** Re-using cooldown. NOTE: concurrent timer with Cooldown */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GGAttack|Specification")
		float Retrigger;

	//	Just so we don't need to reconstruct everytime
	FActorSpawnParameters ProjectileSpawnParam;

	//	States
	uint32 bIsReadyToBeUsed : 1;
	uint32 bIsLocalInstruction : 1;
	float TimeLapsed;
	FVector LaunchOffsetMultiplier;
	FTimerHandle StateTimerHandle;

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
	UPROPERTY(BlueprintAssignable, Category ="GGAttack")
		FStatusUpdateEventSignature OnFinalizeAttack;
	/** Where all updates are handled, consider moving the StartUp -> Active part via timer */
	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Spawn and configure projectile */
    UFUNCTION(BlueprintImplementableEvent, Category="GGAttack")
        void LaunchProjectile(bool bIsFromLocal);
    /** opportunity in BP to catch the event and do effects */
    UFUNCTION(BlueprintImplementableEvent, Category ="GGAttack")
        void OnLaunchProjectile();
protected:
	UFUNCTION()
		virtual void HitTarget(AActor* target);
	UFUNCTION()
		virtual void ResetForRetrigger();
};
