// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcRangedAttackComponent.h"
#include "Game/Component/GGPooledSpriteComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Data/GGGameTypes.h"
#include "Game/Framework/GGGameState.h"
#include "Game/Data/GGProjectileData.h"
#include "Game/Actor/GGSpritePool.h"

// Sets default values for this component's properties
UGGNpcRangedAttackComponent::UGGNpcRangedAttackComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UGGNpcRangedAttackComponent::LaunchProjectile(UGGProjectileData * InData,
	const FVector & InLaunchLocation, const FVector & InLaunchDirection, FVector InLaunchScale, FQuat InLaunchRotation)
{
	if (!SpritePool.IsValid())
	{
		AGGGameState* loc_GS = GetWorld()->GetGameState<AGGGameState>();
		if (loc_GS)
		{
			SpritePool = loc_GS->GetSpritePool();
		}
		if (SpritePool == nullptr)
		{
			UE_LOG(GGWarning, Warning, TEXT("Sprite pool cant be located"));
		}
	}
	if (SpritePool.IsValid())
	{// if the ptr is valid, or not valid but set to be valid in Find
		UGGPooledSpriteComponent* spriteInstance = SpritePool.Get()->CheckoutInstance();
		// initialize spriteInstance
		spriteInstance->PreCheckout();
		// set sprite transform, scale (face left / face right)  is copied from owning character's body directly
		FTransform spriteTransform = FTransform(InLaunchRotation, InLaunchLocation, InLaunchScale);

		// apply transform						
		spriteInstance->SetWorldTransform(spriteTransform, false, nullptr, ETeleportType::TeleportPhysics);
		// set display information
		spriteInstance->SetSprite(InData->TravelSprite);
		// initialize physics
		spriteInstance->SetCollisionObjectType(ProjectileObjectType);
		//spriteInstance->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		for (auto type : TargetObjectTypes)
		{
			spriteInstance->SetCollisionResponseToChannel(type, ECollisionResponse::ECR_Block);
		}
		// configure FLaunchedProjectile and add to array
		UpdatedProjectiles.Add(FLaunchedProjectile(spriteInstance, InLaunchDirection, InData, GetWorld()->GetTimeSeconds()));
	}
}
// Called every frame
void UGGNpcRangedAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	float CurrentTime = GetWorld()->GetTimeSeconds();
	for (int32 i = UpdatedProjectiles.Num() - 1; i >= 0; --i)
	{
		FLaunchedProjectile& projectle = UpdatedProjectiles[i];
		// no substepping, lazy!
		// sweep component
		FVector delta = projectle.CurrentVelocity * DeltaTime + projectle.ContinualAcceleration * DeltaTime * DeltaTime * 0.5f;
		FHitResult hitResult;
		projectle.SpriteBody->AddWorldOffset(delta, true, &hitResult, ETeleportType::None);
		// inefficient since it may not be necessary, but makes code cleaner doing so here collision handling
		projectle.CurrentVelocity += projectle.ContinualAcceleration * DeltaTime;
		if (hitResult.bBlockingHit)
		{	
			// damage check by everyone			
			AGGCharacter* target = Cast<AGGCharacter>(hitResult.GetActor());
			// need check immunity for consumption for accurate collision count
			projectle.CurrentCollisionCount++;
			if (target && target->IsLocallyControlled())
			{
				FGGDamageReceivingInfo locInfo = TranslateHitResult(projectle, hitResult);
				target->LocalReceiveDamage(locInfo);
			}			
		}
		// handle impact effects

		// check collision cleanup for everyone		
		if (projectle.CurrentCollisionCount > projectle.ProjectileData->Penetration)
		{
			// cleanup
			projectle.SpriteBody->PreCheckin();
			// no longer needs update				
			if (SpritePool.IsValid())
			{
				SpritePool.Get()->CheckinInstance(projectle.SpriteBody);
			}
			else
			{
				projectle.SpriteBody->DestroyComponent();
			}
			UpdatedProjectiles.RemoveAtSwap(i, 1, false);
		}
		else if (CurrentTime > projectle.Lifespan + projectle.SpawnTime)
		{	// check lifespan cleanup for everyone			
			projectle.SpriteBody->PreCheckin();
			// no longer needs update				
			if (SpritePool.IsValid())
			{
				SpritePool.Get()->CheckinInstance(projectle.SpriteBody);
			}
			else
			{
				projectle.SpriteBody->DestroyComponent();
			}
			UpdatedProjectiles.RemoveAtSwap(i, 1, false);
		}
	}	
}

FGGDamageReceivingInfo UGGNpcRangedAttackComponent::TranslateHitResult(const FLaunchedProjectile& InProjectile, const FHitResult& InHitResult) const
{
	FGGDamageReceivingInfo result;
	result.Direct_BaseMultiplier = InProjectile.ProjectileData->GetDirectDamageBase() + DirectWeaponDamageBase;
	result.Indirect_BaseMultiplier = InProjectile.ProjectileData->GetIndirectDamageBase()
		+ IndirectWeaponDamageBase;

	result.ImpactDirection = FGGDamageReceivingInfo ::ConvertDeltaPosition(InProjectile.CurrentVelocity);
	result.Type = InProjectile.ProjectileData->Type;
	return result;
}