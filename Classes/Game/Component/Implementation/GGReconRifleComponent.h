// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Component/GGRangedAttackComponent.h"
#include "Game/Data/GGProjectileData.h"
#include "GGReconRifleComponent.generated.h"

/**
 * Responsible for both the launch and update of projectiles
 */	
class UGGTriggerAnimData;
class UGGPooledSpriteComponent;
class UCharacterMovementComponent;
class AGGSpritePool;
class AGGCharacter;

/** Contains information necessary in updating projectile's state and handling events*/
USTRUCT()
struct FLaunchedProjectile
{
	GENERATED_BODY()

	// safe guard in case sprite component is destroyed somehow
	UGGPooledSpriteComponent* SpriteBody;
	// this is stored to make delayed launch possible
	FVector LaunchDirection;
	FVector ContinualAcceleration;
	// since we are using PaperSpriteComponent, they do not store velocity by default unlike an AActor
	FVector CurrentVelocity;
	UGGProjectileData* ProjectileData;

	int8 CurrentCollisionCount;

	FLaunchedProjectile()
	{
		SpriteBody = nullptr;
		LaunchDirection = FVector::ZeroVector;
		ContinualAcceleration = FVector::ZeroVector;
		ProjectileData = nullptr;
		CurrentCollisionCount = 0;
	}

	FLaunchedProjectile(UGGPooledSpriteComponent* body, const FVector& direction, UGGProjectileData* data)
		: LaunchDirection(direction.X, direction.Y, direction.Z), CurrentCollisionCount(0)
	{
		SpriteBody = body;
		ProjectileData = data;
		if (ProjectileData != nullptr)
		{
			ContinualAcceleration = ProjectileData->GetGravityVector();
			ContinualAcceleration.Y = FMath::Sign(LaunchDirection.Y) * ContinualAcceleration.Y;
			CurrentVelocity = LaunchDirection * ProjectileData->LaunchSpeed;
		}
	}
};

UCLASS()
class GG_API UGGReconRifleComponent : public UGGRangedAttackComponent
{
	GENERATED_BODY()
public:	
	UPROPERTY(EditDefaultsOnly, Category = "GGAttack|Specification")
		UGGProjectileData* ProjectileData_Normal;
	UPROPERTY(EditDefaultsOnly, Category = "GGAttack|Specification")
		UGGTriggerAnimData* TriggerAnimData_Normal;
	UPROPERTY(EditDefaultsOnly, Category = "GGAttack|Specification")
		UGGProjectileData* ProjectileData_Charged;
	UPROPERTY(EditDefaultsOnly, Category = "GGAttack|Specification")
		UGGTriggerAnimData* TriggerAnimData_Charged;	
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 MaxMovingAttackCount;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		float MovingAttackCountDuration;
	/** The position we launch the projectile when shooting straight, defaults for Right */
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		FVector AimOffset_Neutral;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		FVector AimOffset_Up;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		FVector AimOffset_Down;
	/** The variable position we launch the projectile with */
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		float LaunchOffset_Random;
	float FiringLength_Normal;
	float FiringLength_Charged;

	//********************************
	// Component states 
protected:
	// No queued version, we don't allow immediate aim change animation-wise. Also replicated in owning character for locomotion animation
	UPROPERTY(Transient, Replicated)
	uint8 WeaponAimLevel;

	UGGProjectileData* LastUsedProjectileData;
	UGGTriggerAnimData* BodyAnimDataInUse;

	TArray<float, TInlineAllocator<8>> MovingAttackTimeRegister;

	/** For determining state exit */
	UPROPERTY(Transient, Replicated)
		uint8 bModeChargedAttack : 1;
	UPROPERTY(Transient, Replicated)
		uint8 bModeChargedAttack_Queued : 1;
	
	
	/** For determining we queue attacks or start instantly, not replicated. In state terms, 
	 *	this represent whether we are still in the middle of 1 shoot action.
	 */
	uint8 bWeaponIsFiring : 1;
	uint8 bQueuedAttack : 1;
	FTimerHandle AttackQueueHandle;
	TWeakObjectPtr<AGGSpritePool> SpritePool;
	/** sprites updated by this component */	
	TArray<FLaunchedProjectile, TInlineAllocator<16>> UpdatedProjectiles;
	float TimeOfLastAttack;

public:
	// this is needed, to determine which shoot animation to use on fire
	TWeakObjectPtr<UCharacterMovementComponent> OwnerMovement;

	virtual void PostInitProperties() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//********************************
	/** Input entry point (for calling from in owning character). Only charge and aim information are important, 
	 *	rest can be polled from owning character 
	 */
	void LocalAttemptsAttack(bool InIsCharged, uint8 AimLevel);

protected:

	// ******** RangedAttackComponent interface ********
	//********************************
	// Landing attacks
	virtual void HitTarget(const FRangedHitNotify &InHitNotify) override;

	//********************************
	// Launching attacks
	virtual void PushAttackRequest() override;

	/** Actual method that processes an attack instruction, index is used to represent the specific attack used */
	virtual void InitiateAttack() override;
	virtual void FinalizeAttack() override;
	/** On top of handling queued attacks, this is also the place to reset states to allow attacks for retriggering */
	UFUNCTION()
		void HandleQueuedAttack();

	/** Component tick updates individual sprite's movement */
	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//********************************
	// Child specific utilities
	// For client transmission to server
	uint8 GetEncryptedAttackIdentifier(bool InIsCharged, uint8 AimLevel) const;
	// For server identification
	void DecryptAttackIdentifier(const uint8 InIdentifier, bool& OutIsCharged, uint8 &OutAimLevel);
	
	bool GetOwnerGroundState() const;
	
	bool CanQueueShots();

	// use params when possible for more options in network implementation	
	FVector GetAimOffset(uint8 InAimLevel) const;
	FVector GetAimDirection(uint8 InAimLevel) const;

	bool FindSpritePoolReference();

	// For picking the right projectile from current states
	UGGProjectileData* GetProjectileDataToUse() const;
	UGGTriggerAnimData* GetTriggerAnimDataToUse() const;

	AGGCharacter* GetTypedOwner() const;

public:
	virtual UPaperFlipbook* GetCurrentBodyAnimation() const;
	virtual UPaperFlipbook* GetEffectAnimation() const;
};
