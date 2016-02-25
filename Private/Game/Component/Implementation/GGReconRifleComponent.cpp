// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/Implementation/GGReconRifleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Game/Data/GGProjectileData.h"
#include "Game/Data/GGTriggerAnimData.h"

void UGGReconRifleComponent::PostInitProperties()
{
	Super::PostInitProperties();
	if (ProjectileData_Normal) 
	{
		ProjectileData_Normal->RecalculateCaches();
	}
	if (ProjectileData_Charged)
	{
		ProjectileData_Charged->RecalculateCaches();
	}
}

void UGGReconRifleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifeTimeReplicatedProps(OutLifeTimeProps);
	DOREPLIFETIME_COND(UGGReconRifleComponent, bModeChargedAttack, COND_SkipOwner);
}

void UGGReconRifleComponent::LocalAttemptsAttack(bool InIsCharged, uint8 AimLevel)
{	
	// no need for rejection, complicated combo timing not implemented for shooting
	if (CanQueueShots())
	{
		LocalInitiateAttack(GetEncryptedAttackIdentifier(InIsCharged, AimLevel));
	}
}

void UGGReconRifleComponent::PushAttackRequest()
{
	if (bWeaponIsFiring)
	{
		if (GetOwnerRole() > ROLE_SimulatedProxy)
		{
			bool locCharged;
			uint8 locAimLevel;
			DecryptAttackIdentifier(AttackIdentifier, locCharged, locAimLevel);
			bModeChargedAttack_Queued = locCharged;
			// Note WeaponAimLevel is not adjusted
			SumAttackQueue++;
		}
		bQueuedAttack = true;
	}
	else
	{
		if (GetOwnerRole() > ROLE_SimulatedProxy)
		{
			bool locCharged;
			uint8 locAimLevel;
			DecryptAttackIdentifier(AttackIdentifier, locCharged, locAimLevel);
			bModeChargedAttack = locCharged;
			WeaponAimLevel = locAimLevel;
			SumAttackQueue++;
		}
		InitiateAttack();
	}
}

void UGGReconRifleComponent::InitiateAttack()
{
	LastUsedProjectileData = GetProjectileDataToUse();
	BodyAnimDataInUse = GetTriggerAnimDataToUse();
	bWeaponIsFiring = true;
	// Timestamp is used for setting a dynamic timer to quit state based on time lapsed from last attack
	TimeOfLastAttack = GetWorld()->GetTimeSeconds();
	// Actual "spawning" of projectile bodies

	ProcessedAttackQueue++;	
	// set timer to check for queue
	float locFiringDelay = (bQueuedAttack && bModeChargedAttack_Queued) || (!bQueuedAttack && FiringLength_Charged) ?
		 FiringLength_Charged : FiringLength_Normal;
	GetWorld()->GetTimerManager().SetTimer(AttackQueueHandle, this, &UGGReconRifleComponent::HandleQueuedAttack, locFiringDelay);
	SetComponentTickEnabled(true);
	Super::InitiateAttack();
}

void UGGReconRifleComponent::FinalizeAttack()
{
	// tick is disabled thorugh a combined condition, here we check the other which is if all projectilese are deactivated
	if (UpdatedProjectile.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
	Super::FinalizeAttack();
}

void UGGReconRifleComponent::HandleQueuedAttack()
{
	if (bQueuedAttack)
	{
		InitiateAttack();
		bQueuedAttack = false;
	}
	else
	{
		bWeaponIsFiring = false;
	}
}

void UGGReconRifleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

const int32 BIT_CHARGED = 128;
uint8 UGGReconRifleComponent::GetEncryptedAttackIdentifier(bool InIsCharged, uint8 AimLevel) const
{
	uint8 Index = AimLevel;
	if (InIsCharged)
	{
		Index |= BIT_CHARGED;
	}
	return Index;
}

void UGGReconRifleComponent::DecryptAttackIdentifier(const uint8 InIdentifier, bool & OutIsCharged, uint8 & OutAimLevel)
{
	OutIsCharged = !!(InIdentifier & BIT_CHARGED);
	OutAimLevel = InIdentifier & (BIT_CHARGED - 1); // all the right bits
}

bool UGGReconRifleComponent::GetOwnerGroundState() const
{
	UCharacterMovementComponent* loc_CMC = OwnerMovement.Get();
	if (loc_CMC == nullptr)
	{
		AGGCharacter* loc_Character = static_cast<AGGCharacter*>(GetOwner());
		if (loc_Character)
		{
			loc_CMC = loc_Character->GetCharacterMovement();
		}
		if (loc_CMC)
		{
			return  loc_CMC->IsMovingOnGround();
		}
	}
	else
	{
		return  loc_CMC->IsMovingOnGround();
	}
	UE_LOG(GGWarning, Warning, TEXT("SlashAtkComponent has no owner / charactermovement component in owner heirarchy"));
	// always return true if not charactermovement 
	return true;
}

const float SMALL_SPEED = 25.f;
bool UGGReconRifleComponent::CanQueueShots() const
{
	if (GetOwner() == nullptr)
	{
		return false;
	}
	bool onGround = GetOwnerGroundState();
	if (!onGround)
	{
		// don't care if mid-air
		return true;
	}
	bool moving = FMath::Abs(GetOwner()->GetVelocity().Y) > SMALL_SPEED;
	if (!moving)
	{
		return true;
	}
	// we are moving, check the arrays of time register
	float CurrentTime = GetWorld()->GetTimeSeconds();
	for (int32 i = MovingAttackTimeRegister.Num() - 1; i >= 0; i--)
	{
		if (CurrentTime - MovingAttackTimeRegister[i] > MovingAttackCountDuration)
		{
			MovingAttackTimeRegister.Pop(false);
		}
	}
	return MovingAttackTimeRegister.Num() < MaxMovingAttackCount;
}

UGGProjectileData* UGGReconRifleComponent::GetProjectileDataToUse() const
{
	bool locCharged = bQueuedAttack ? bModeChargedAttack_Queued : bModeChargedAttack;

	return locCharged ? ProjectileData_Charged : ProjectileData_Normal;
}

UGGTriggerAnimData* UGGReconRifleComponent::GetTriggerAnimDataToUse() const
{
	bool locCharged = bQueuedAttack ? bModeChargedAttack_Queued : bModeChargedAttack;

	return locCharged ? TriggerAnimData_Charged : TriggerAnimData_Normal;
}

UPaperFlipbook * UGGReconRifleComponent::GetCurrentBodyAnimation() const
{
	if (BodyAnimDataInUse == nullptr || GetOwner() == nullptr)
	{
		return nullptr;
	}
	bool locOnGround = GetOwnerGroundState();
	if (!locOnGround)
	{
		return BodyAnimDataInUse->GetAirFlipbook();
	}
	bool locDashing = OwnerMovement.Get()->IsCrouching();
	if (locDashing)
	{
		return BodyAnimDataInUse->GetDashFlipbook();
	}
	return BodyAnimDataInUse->GetGroundFlipbook(GetOwner()->GetVelocity().Y);
}

UPaperFlipbook * UGGReconRifleComponent::GetEffectAnimation() const
{
	if (LastUsedProjectileData != nullptr)
	{
		return LastUsedProjectileData->ImpactEffect;
	}
	return nullptr;
}
