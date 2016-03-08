// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/Implementation/GGReconRifleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Actor/GGMinionBase.h"

#include "Net/UnrealNetwork.h"

#include "Game/Data/GGProjectileData.h"
#include "Game/Data/GGTriggerAnimData.h"

#include "Game/Framework/GGGameState.h"
#include "Game/Actor/GGSpritePool.h"
#include "Game/Component/GGPooledSpriteComponent.h"
#include "PaperFlipbookComponent.h"


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
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGGReconRifleComponent, bModeChargedAttack, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UGGReconRifleComponent, WeaponAimLevel, COND_SkipOwner);
}

void UGGReconRifleComponent::LocalAttemptsAttack(bool InIsCharged, uint8 AimLevel)
{	
	// no need for rejection, complicated combo timing not implemented for shooting
	if (CanQueueShots())
	{		
		LocalInitiateAttack(GetEncryptedAttackIdentifier(InIsCharged, AimLevel));
	}
}

void UGGReconRifleComponent::HitTarget(const FRangedHitNotify &InHitNotify)
{
	Super::HitTarget(InHitNotify);
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

const float ROOT_TWO = 1.41421356f;
void UGGReconRifleComponent::InitiateAttack()
{
	LastUsedProjectileData = GetProjectileDataToUse();
	BodyAnimDataInUse = GetTriggerAnimDataToUse(WeaponAimLevel);
	bWeaponIsFiring = true;
	// Timestamp is used for setting a dynamic timer to quit state based on time lapsed from last attack
	TimeOfLastAttack = GetWorld()->GetTimeSeconds();
	// Actual "spawning" of projectile bodies
	if (SpritePool.IsValid() || FindSpritePoolReference())
	{
		// if the ptr is valid, or not valid but set to be valid in Find
		UGGPooledSpriteComponent* spriteInstance = SpritePool.Get()->CheckoutInstance();		
		AGGCharacter* loc_Char = GetTypedOwner();
		if (LastUsedProjectileData && spriteInstance && loc_Char)
		{
			// initialize spriteInstance
			spriteInstance->PreCheckout();
			// set sprite transform, scale (face left / face right)  is copied from owning character's body directly
			FTransform spriteTransform = loc_Char->GetBodyTransform();
			FVector aimDirection = GetAimDirection(WeaponAimLevel);
			if (aimDirection.Z != 0.f && LastUsedProjectileData->bVelocityDictatesRotation)
			{
				// rotation may need adjustment to point up/down, depending on the ProjectileData used
				spriteTransform.ConcatenateRotation(
					FQuat(0.f, ROOT_TWO * 0.5f, 0.f, FMath::Sign(aimDirection.Z) * ROOT_TWO * 0.5f));
			}
			// add location offset
			spriteTransform.AddToTranslation(GetAimOffset(WeaponAimLevel));
			// apply transform						
			spriteInstance->SetWorldTransform(spriteTransform, false, nullptr, ETeleportType::TeleportPhysics);
			// set display information
			spriteInstance->SetSprite(LastUsedProjectileData->TravelSprite);
			// initialize physics
			spriteInstance->SetCollisionObjectType(ProjectileObjectType);
			//spriteInstance->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			for (auto type : TargetObjectTypes)
			{
				spriteInstance->SetCollisionResponseToChannel(type, ECollisionResponse::ECR_Block);
			}			
			// configure FLaunchedProjectile and add to array
			UpdatedProjectiles.Add(FLaunchedProjectile(spriteInstance, aimDirection, LastUsedProjectileData, TimeOfLastAttack));
		}
	}
	if (IsOwnerMoving())
	{		
		MovingAttackTimeRegister.Add(GetWorld()->GetTimeSeconds());
	}

	ProcessedAttackQueue++;	
	// set timer to check for queue
	float locFiringDelay = (bQueuedAttack && bModeChargedAttack_Queued) || (!bQueuedAttack && bModeChargedAttack) ?
		 FiringLength_Charged : FiringLength_Normal;
	GetWorld()->GetTimerManager().SetTimer(
		AttackQueueHandle, this, &UGGReconRifleComponent::HandleQueuedAttack, locFiringDelay);
	SetComponentTickEnabled(true);
	Super::InitiateAttack();
}

void UGGReconRifleComponent::FinalizeAttack()
{
	// tick is disabled thorugh a combined condition, here we check the other which is if all projectilese are deactivated
	if (UpdatedProjectiles.Num() == 0)
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
		FinalizeAttack();
	}
}

void UGGReconRifleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// unlike melee, queues are handled using timers instead so we only need to handle projectiles update here
	float CurrentTime = GetWorld()->GetTimeSeconds();
	for (int32 i = UpdatedProjectiles.Num() - 1; i >=0; --i)
	{
		auto& projectile = UpdatedProjectiles[i];
		// no substepping, lazy!
		// sweep component
		FVector delta = projectile.CurrentVelocity * DeltaTime + projectile.ContinualAcceleration * DeltaTime * DeltaTime * 0.5f;
		FHitResult hitResult;
		projectile.SpriteBody->AddWorldOffset(delta, true, &hitResult, ETeleportType::None);
		projectile.CurrentVelocity += projectile.ContinualAcceleration * DeltaTime; // inefficient since it may not be necessary, but makes code cleaner doing so here
		// collision handling
		if (hitResult.bBlockingHit)
		{
			if (bIsLocalInstruction && projectile.ProjectileData)
			{// handle damage only if local
				FRangedHitNotify notifier;
				notifier.Target = hitResult.Actor.Get();
				notifier.DamageBaseMultipliers = FRangedHitNotify::CompressedDamageLevels(
					DirectWeaponDamageBase + projectile.ProjectileData->GetDirectDamageBase(),
					IndirectWeaponDamageBase + projectile.ProjectileData->GetIndirectDamageBase());

				notifier.HitVelocity = projectile.CurrentVelocity;
				notifier.DamageCategory = EGGDamageType::Standard;
				LocalHitTarget(notifier);
			}
			// handle impact effects

			// check collision cleanup for everyone
			projectile.CurrentCollisionCount++;
			if (projectile.CurrentCollisionCount > projectile.ProjectileData->Penetration)
			{
				// cleanup
				projectile.SpriteBody->PreCheckin();
				// no longer needs update				
				if (SpritePool.IsValid())
				{
					SpritePool.Get()->CheckinInstance(projectile.SpriteBody);
				}
				else
				{
					projectile.SpriteBody->DestroyComponent();
				}
				UpdatedProjectiles.RemoveAtSwap(i, 1, false);
			}			
		}
		else if (CurrentTime > projectile.Lifespan + projectile.SpawnTime)
		{	// check lifespan cleanup for everyone			
			projectile.SpriteBody->PreCheckin();
			// no longer needs update				
			if (SpritePool.IsValid())
			{
				SpritePool.Get()->CheckinInstance(projectile.SpriteBody);
			}
			else
			{
				projectile.SpriteBody->DestroyComponent();
			}
			UpdatedProjectiles.RemoveAtSwap(i, 1, false);
		}
	}
}

const int32 BIT_CHARGED_GGRIFLE = 128;
uint8 UGGReconRifleComponent::GetEncryptedAttackIdentifier(bool InIsCharged, uint8 AimLevel) const
{
	uint8 Index = bWeaponIsFiring ? WeaponAimLevel : AimLevel;
	if (InIsCharged)
	{
		Index |= BIT_CHARGED_GGRIFLE;
	}
	return Index;
}

void UGGReconRifleComponent::DecryptAttackIdentifier(const uint8 InIdentifier, bool & OutIsCharged, uint8 & OutAimLevel)
{
	OutIsCharged = !!(InIdentifier & BIT_CHARGED_GGRIFLE);
	OutAimLevel = InIdentifier & (BIT_CHARGED_GGRIFLE - 1); // all the right bits
}

bool UGGReconRifleComponent::GetOwnerGroundState() const
{
	UCharacterMovementComponent* loc_CMC = OwnerMovement.Get();
	if (loc_CMC == nullptr)
	{
		AGGCharacter* loc_Character = GetTypedOwner();
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

const float SMALL_SPEED_GGRIFLE = 25.f;
bool UGGReconRifleComponent::IsOwnerMoving() const
{
	return FMath::Abs(GetOwner()->GetVelocity().Y) > SMALL_SPEED_GGRIFLE;
}

bool UGGReconRifleComponent::CanQueueShots()
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
	bool moving = IsOwnerMoving();
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

FVector UGGReconRifleComponent::GetAimOffset(uint8 InAimLevel) const
{
	if (InAimLevel == 0)
	{
		return AimOffset_Down + FVector::RightVector * LaunchOffset_Random * FMath::RoundToFloat(FMath::FRandRange(-1.f, 1.f));
	}
	if (InAimLevel == 1)
	{
		AGGCharacter* loc_Char = GetTypedOwner();
		if (loc_Char)
		{
			return loc_Char->GetPlanarForwardVector().Y * AimOffset_Neutral 
				+ FVector::UpVector * LaunchOffset_Random * FMath::RoundToFloat(FMath::FRandRange(-1.f, 1.f));
		}
		else
		{
			UE_LOG(GGWarning, Warning, TEXT("Can't find owning character..."));
		}
	}
	return AimOffset_Up + FVector::RightVector * LaunchOffset_Random * FMath::RoundToFloat(FMath::FRandRange(-1.f, 1.f));
}

FVector UGGReconRifleComponent::GetAimDirection(uint8 InAimLevel) const
{
	if (InAimLevel == 0)
	{
		return FVector::UpVector * -1.f;
	}
	if (InAimLevel == 1)
	{
		AGGCharacter* loc_Char = GetTypedOwner();
		if (loc_Char)
		{
			return loc_Char->GetPlanarForwardVector();
		}
		else
		{
			UE_LOG(GGWarning, Warning, TEXT("Can't find owning character..."));
		}
	}
	return FVector::UpVector;
}

// return true if found
bool UGGReconRifleComponent::FindSpritePoolReference()
{
	AGGGameState* loc_GS = GetWorld()->GetGameState<AGGGameState>();
	if (loc_GS)
	{
		SpritePool = loc_GS->GetSpritePool();
	}
	return SpritePool.IsValid();
}

UGGProjectileData* UGGReconRifleComponent::GetProjectileDataToUse() const
{
	bool locCharged = bQueuedAttack ? bModeChargedAttack_Queued : bModeChargedAttack;

	return locCharged ? ProjectileData_Charged : ProjectileData_Normal;
}

UGGTriggerAnimData* UGGReconRifleComponent::GetTriggerAnimDataToUse(uint8 InAimLevel) const
{
	bool locCharged = bQueuedAttack ? bModeChargedAttack_Queued : bModeChargedAttack;
	if (InAimLevel == 0)
	{
		return locCharged ? TriggerAnimData_Charged_Down : TriggerAnimData_Normal_Down;
	}
	if (InAimLevel == 1)
	{
		return locCharged ? TriggerAnimData_Charged_Neutral: TriggerAnimData_Normal_Neutral;
	}
	return locCharged ? TriggerAnimData_Charged_Up : TriggerAnimData_Normal_Up;
}

AGGCharacter* UGGReconRifleComponent::GetTypedOwner() const
{
	return static_cast<AGGCharacter*>(GetOwner());
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
	if (LastUsedProjectileData)
	{
		return LastUsedProjectileData->LaunchEffect;
	}
	return nullptr;
}
