// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Component/GGMeleeAttackComponent.h"
#include "Game/Data/GGMeleeAttackData.h"
#include "GGSlashAttackComponent.generated.h"

/**
 * Weapon class handling our assault character's normal attack, consist of multiple combo attack settings to be used
 */
class UCharacterMovementComponent;
UCLASS()
class GG_API UGGSlashAttackComponent : public UGGMeleeAttackComponent
{
	GENERATED_BODY()
public:
	//********************************
	// Component specifications
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category ="GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*> GroundNormalAttacks;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*> GroundMovingAttacks;
		
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		UGGMeleeAttackData* GroundChargedAttack;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		TArray<UGGMeleeAttackData*> AirNormalAttacks;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GGAttack|SlashAttack")
		UGGMeleeAttackData* AirChargedAttack;
	
	//********************************
	// Component states 
protected:
	/** For determining state exit */
	UPROPERTY(Transient, Replicated)
		uint8 bModeChargedAttack : 1;
	UPROPERTY(Transient, Replicated)
		uint8 bModeMobileAttack : 1;		
	UPROPERTY(Transient, Replicated)
		uint8 bModeChargedAttack_Queued : 1;
	UPROPERTY(Transient, Replicated)
		uint8 bModeMobileAttack_Queued : 1;
	
	/** States of each individual clients */
	/** Aerial attack or not is a client side state, not replicated */
	uint8 bModeAerialAttack : 1;
	/** For setting chain timed attack */
	uint8 bQueuedAttack : 1;
	/** Combo index */
	int32 NextAttackIndex;

	TWeakObjectPtr<UGGMeleeAttackData> LastInitiatedAttack;
	float CurrentTimeStamp;
public:	
	TWeakObjectPtr<UCharacterMovementComponent> OwnerMovement;

	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//********************************
	// Input entry point (for calling from in owning character)
	void LocalAttemptsAttack(bool InIsCharged, bool InIsMobile);

protected:
	
	// ******** MeleeAttackComponent interface ********
	//********************************
	// Landing attacks
	virtual void HitTarget(const FMeleeHitNotify& InHitNotify) override;

	//********************************
	// Launching attacks
	virtual void PushAttackRequest() override;

	/** Actual method that processes an attack instruction, index is used to represent the specific attack used */
	virtual void InitiateAttack() override;
	virtual void FinalizeAttack() override;
	
	UFUNCTION()
		void UseQueuedAttack();

	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
		
	//********************************
	// Child specific utilities
	// For client transmission to server
	uint8 GetEncryptedAttackIdentifier(bool InIsCharged, bool InIsMobile) const;
	// For server identification
	void DecryptAttackIdentifier(const uint8 InIdentifier, bool& OutIsCharged, bool& OutIsMobile);
	bool GetOwnerGroundState() const;
	// For picking the right attack from current states
	UGGMeleeAttackData* GetAttackToUse();

public:
	bool ShouldRemainInState() const;
	virtual UPaperFlipbook* GetCurrentAnimation() const;
	virtual UPaperFlipbook* GetEffectAnimation() const;
};
