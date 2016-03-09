// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Data/GGGameTypes.h"
#include "Game/Data/GGNpcAnimBlendspaceData.h"
#include "Game/Component/GGAIMovementComponent.h"

// Sets default values for this component's properties
UGGNpcLocomotionAnimComponent::UGGNpcLocomotionAnimComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

UPaperFlipbook* UGGNpcLocomotionAnimComponent::GetCurrentAnimation() const
{
	if (LocomotionData)
	{
		const AGGMinionBase* loc_MinionOwner = Cast<AGGMinionBase>(GetOwner());
		if (loc_MinionOwner)
		{
			FVector loc_Velocity = loc_MinionOwner->GetVelocity();
			const UGGAIMovementComponent* loc_AIMovement = loc_MinionOwner->GetAIMovement();
			bool loc_bOnGround = loc_AIMovement->IsMovingOnGround();
			// we have all the conditions enough to deduce the locomotion substate
			if (loc_bOnGround)
			{
				return LocomotionData->GetGroundFlipbook(loc_Velocity.Y * loc_MinionOwner->GetPlanarForwardVector().Y);
			}
			else
			{
				return LocomotionData->GetAirFlipbook(loc_Velocity.Z);
			}
		}
	}
	return nullptr;
}

UPaperFlipbook* UGGNpcLocomotionAnimComponent::GetDeathFlipbook(EGGDamageType type) const
{
	if (LocomotionData)
	{
		switch (type)
		{
		case EGGDamageType::Standard:
			return LocomotionData->GetStandardDeathFlipbook();
		case EGGDamageType::Slash:
			return LocomotionData->GetSlashDeathFlipbook();
		}
	}
	return nullptr;
}