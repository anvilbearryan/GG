// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGShooterMinion.h"
#include "Game/Actor/GGCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"

void AGGShooterMinion::OnReachWalkingBound()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("OnReachWalkingBound"));
	bReachedWalkingBound = true;
}

void AGGShooterMinion::OnSensorActivate_Implementation()
{
	if (ActionState == EGGAIActionState::Inactive)
	{
		check(Sensor);
		TransitToActionState(EGGAIActionState::Patrol);
		UE_LOG(GGMessage, Log, TEXT("Sensor Activate"));
		Target = Sensor->Target.Get();
	}
}

void AGGShooterMinion::OnSensorAlert_Implementation()
{
	if (ActionState != EGGAIActionState::PrepareAttack && ActionState != EGGAIActionState::Attack)
	{
		check(Sensor);
		TransitToActionState(EGGAIActionState::PrepareAttack);
		UE_LOG(GGMessage, Log, TEXT("Sensor Alert"));
	}
}

void AGGShooterMinion::OnSensorUnalert_Implementation()
{
	if (ActionState != EGGAIActionState::Patrol)
	{
		check(Sensor);
		TransitToActionState(EGGAIActionState::Patrol);
		UE_LOG(GGMessage, Log, TEXT("Sensor Unalert"));
	}
}

void AGGShooterMinion::OnSensorDeactivate_Implementation()
{
	check(Sensor);
	TransitToActionState(EGGAIActionState::Inactive);
	UE_LOG(GGMessage, Log, TEXT("Sensor Deactivate"));
}

void AGGShooterMinion::TickPatrol(float DeltaSeconds)
{
    Super::TickPatrol(DeltaSeconds);
    
	/** state change condition is checked by CharacterSensing component and applied through SwitchToPrepareAttack, 
	*	we need not worry here.
	*/
	TimeWalkedContinually += DeltaSeconds;
	if (bTakesTimeBasedPause && TimeWalkedContinually > TimeBasedPauseInterval)
	{
		// time to take a break
		TimeWalkedContinually = 0;
		PauseBehaviourTick(PauseDuration);
	}
	else
	{		
		if (!bReachedWalkingBound)
		{
			TravelDirection = GetPlanarForwardVector();
		}
		else
		{
			TimeWalkedContinually = 0.f; // reset timer for next cycle
			SequenceTurnFacingDirection(TurnPausePatrol, TurnPausePatrol * 0.5f);
		}
	}
	SyncFlipbookComponentWithTravelDirection();
}

void AGGShooterMinion::TickPrepareAttack(float DeltaSeconds)
{
    Super::TickPrepareAttack(DeltaSeconds);
    
    bool IsInAttackMaxRange = IsTargetInSuppliedRange(AttackMaxRange);
	bool IsInAttackMinRange = IsTargetInSuppliedRange(AttackMinRange);
    if (IsInAttackMaxRange && !IsInAttackMinRange)
    {
        // we may need to do a turn first
        if (IsFacingTarget())
        {
            // can attack
			TransitToActionState(EGGAIActionState::Attack);
		}
		else
		{
			// need to do a turn to reach a shooting position
			SequenceTurnFacingDirection(TurnPauseAim, TurnPauseAim * 0.25f);
		}
    }
	else
	{
		// condition implies either target is outside the max range or inside the min range
		if (!IsInAttackMaxRange)
		{
			// move towards target
			TravelDirection.Y = FMath::Sign(Target->GetActorLocation().Y - GetActorLocation().Y);
		}
		else if (IsInAttackMinRange)
		{
			// move away from target
			TravelDirection.Y = FMath::Sign(GetActorLocation().Y - Target->GetActorLocation().Y);
		}
	}
	SyncFlipbookComponentWithTravelDirection();
}

void AGGShooterMinion::TickEvade(float DeltaSeconds)
{
    Super::TickEvade(DeltaSeconds);
	if (bIsInActiveEvasionMode)
	{
		bool FarEnoughToRelax = !IsTargetInSuppliedRange(EvasionSafeRange);
		if (FarEnoughToRelax)
		{
			// switch back to non-evasion active mode
			bIsInActiveEvasionMode = false;
		}
		else
		{
			// run away from target
			bool bFacingTarget = IsFacingTarget();
			/*  We only set travel direction, and let SyncMovmenetToFacing do the job on flipping. 
			*	If we already reached bounds, velocity should not be non-zero hence we won't turn unnecessarily.
			*/
			if (bFacingTarget)
			{
				TravelDirection = -GetPlanarForwardVector();
			}
			else
			{
				TravelDirection = GetPlanarForwardVector();
			}
		}
	}
	else
	{
		bool NeedsToStartEvasion = IsTargetInSuppliedRange(EvasionTriggerRange);
		if (NeedsToStartEvasion)
		{
			bIsInActiveEvasionMode = true;
		}
		else
		{
			// keep an eye on target
			if (!IsFacingTarget())
			{
				SequenceTurnFacingDirection(TurnPauseEvade, TurnPauseEvade*0.5f);
			}
		}
	}
	SyncFlipbookComponentWithTravelDirection();
}

bool AGGShooterMinion::IsTargetInSuppliedRange(const FVector2D& Range) const
{
    if (Target == nullptr)
    {
        return false;
    }
    FVector TargetPosition = Target->GetActorLocation();
    FVector MyPosition = GetActorLocation();
    
    float dx = TargetPosition.Y - MyPosition.Y;
    float dy = TargetPosition.Z - MyPosition.Z;
    
    return Range.X >= FMath::Abs(dx) && Range.Y >= FMath::Abs(dy);
}

bool AGGShooterMinion::IsFacingTarget() const
{
    FVector Forward = GetPlanarForwardVector();
    FVector RelativePosition = Target->GetActorLocation() - GetActorLocation();
    return FVector::DotProduct(RelativePosition, Forward) > 0.f;
}

void AGGShooterMinion::SequenceTurnFacingDirection(float TotalTimeToComplete, float FlipDelay)
{
	// avoid double turn, only execute if no existing timer on the TurnHandle set
	FTimerManager& tm = GetWorld()->GetTimerManager();
	float nextTurnRemaining = tm.GetTimerRemaining(TurnHandle);
	if (nextTurnRemaining <= 0)
	{
		PauseBehaviourTick(TotalTimeToComplete);
		GetWorld()->GetTimerManager().SetTimer(TurnHandle, this, &AGGShooterMinion::FlipFlipbookComponent, FlipDelay);
	}
}

void AGGShooterMinion::FlipFlipbookComponent()
{
	if (FlipbookComponent == nullptr)
	{
		return;
	}
	// bounds condition are changed when we flip flipbook
	bReachedWalkingBound = false;
	FVector NewScale = FVector(-FlipbookComponent->RelativeScale3D.X, 1.f,1.f);
	FlipbookComponent->SetRelativeScale3D(NewScale);
}

void AGGShooterMinion::SyncFlipbookComponentWithTravelDirection()
{
	float FacingDirection = GetPlanarForwardVector().Y;
	float YVelocity = GetVelocity().Y;
	// check for conflict
	if (
		(YVelocity > 0.f  && FacingDirection < 0.f) ||
		(YVelocity  < 0.f  && FacingDirection > 0.f)
		)
	{
		FlipFlipbookComponent();
	}
}