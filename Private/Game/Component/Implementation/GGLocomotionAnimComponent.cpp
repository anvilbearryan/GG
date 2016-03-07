// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/Implementation/GGLocomotionAnimComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Component/GGCharacterMovementComponent.h"
#include "Game/Data/GGAnimBlendspaceData.h"
// Sets default values for this component's properties
UGGLocomotionAnimComponent::UGGLocomotionAnimComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

// Called by Character
UPaperFlipbook* UGGLocomotionAnimComponent::GetCurrentAnimation() const
{
	if (IsActive() && GetOwner() && LocomotionData)
	{
		AGGCharacter* loc_OwningCharacter = static_cast<AGGCharacter*>(GetOwner());		
		UGGCharacterMovementComponent* loc_CharacterMovement = loc_OwningCharacter ? 
			static_cast<UGGCharacterMovementComponent*>(loc_OwningCharacter->GetCharacterMovement()) : nullptr;
		if (loc_CharacterMovement != nullptr)
		{
			FVector loc_Velocity = loc_OwningCharacter->GetVelocity();
			bool loc_bOnGround = loc_CharacterMovement->IsMovingOnGround();
			bool loc_bWallSlide = loc_OwningCharacter->bWallSlidedThisTick;
			bool loc_IsDashing = loc_OwningCharacter->bPerformDashedAction;
			// we have all the conditions enough to deduce the locomotion substate
			if (loc_bOnGround)
			{
				if (loc_IsDashing)
				{
					return LocomotionData->GetDashFlipbook();
				}
				return LocomotionData->GetGroundFlipbook(loc_Velocity.Y);
			}
			if (loc_bWallSlide)
			{
				return LocomotionData->GetWallSlideFlipbook();
			}
			else
			{
				return LocomotionData->GetAirFlipbook(loc_Velocity.Z);
			}
		}
	}
	return nullptr;
}

