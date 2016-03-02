// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcRangedAttackComponent.h"
#include "Game/Component/GGPooledSpriteComponent.h"
#include "Game/Actor/GGCharacter.h"

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
	if (SpritePool.IsValid())
	{
		// if the ptr is valid, or not valid but set to be valid in Find
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
		// no substepping, lazy!
		// sweep component
		FVector delta = UpdatedProjectiles[i].CurrentVelocity * DeltaTime + UpdatedProjectiles[i].ContinualAcceleration * DeltaTime * DeltaTime * 0.5f;
		FHitResult hitResult;
		UpdatedProjectiles[i].SpriteBody->AddWorldOffset(delta, true, &hitResult, ETeleportType::None);
		// inefficient since it may not be necessary, but makes code cleaner doing so here collision handling
		UpdatedProjectiles[i].CurrentVelocity += UpdatedProjectiles[i].ContinualAcceleration * DeltaTime;
		if (hitResult.bBlockingHit)
		{	
			// damage check by everyone		
			UpdatedProjectiles[i].CurrentCollisionCount++;		
			AGGCharacter* target = Cast<AGGCharacter>(hitResult.GetActor());
			if (target && target->IsLocallyControlled())
			{
				UE_LOG(GGMessage, Log, TEXT("local player takes damage"));
			}
			
		}
		// handle impact effects

		// check collision cleanup for everyone		
		if (UpdatedProjectiles[i].CurrentCollisionCount > UpdatedProjectiles[i].ProjectileData->Penetration)
		{
			// cleanup
			UpdatedProjectiles[i].SpriteBody->PreCheckin();
			// no longer needs update				
			if (SpritePool.IsValid())
			{
				SpritePool.Get()->CheckinInstance(UpdatedProjectiles[i].SpriteBody);
			}
			else
			{
				UpdatedProjectiles[i].SpriteBody->DestroyComponent();
			}
			UpdatedProjectiles.RemoveAtSwap(i, 1, false);
		}
		else if (CurrentTime > UpdatedProjectiles[i].Lifespan + UpdatedProjectiles[i].SpawnTime)
		{	// check lifespan cleanup for everyone			
			UpdatedProjectiles[i].SpriteBody->PreCheckin();
			// no longer needs update				
			if (SpritePool.IsValid())
			{
				SpritePool.Get()->CheckinInstance(UpdatedProjectiles[i].SpriteBody);
			}
			else
			{
				UpdatedProjectiles[i].SpriteBody->DestroyComponent();
			}
			UpdatedProjectiles.RemoveAtSwap(i, 1, false);
		}
	}	
}

