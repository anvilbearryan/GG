// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGShooterMinion.h"
#include "Game/Actor/GGCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGAnimatorComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"

void AGGShooterMinion::OnReachWalkingBound()
{
	GEngine->AddOnScreenDebugMessage(3, 1.5f, FColor::Cyan, TEXT("OnReachWalkingBound"));
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
	if (ActionState == EGGAIActionState::Patrol)
	{
		check(Sensor);
		if (IsFacingTarget())
		{
			TransitToActionState(EGGAIActionState::PrepareAttack);
			UE_LOG(GGMessage, Log, TEXT("Sensor Alert"));
		}
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
			SequenceCastAttack();
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
			if (!bReachedWalkingBound)
			{
				TravelDirection.Y = FMath::Sign(GetActorLocation().Y - Target->GetActorLocation().Y);
			}
			else if (!IsFacingTarget())
			{
				SequenceTurnFacingDirection(TurnPauseAim, TurnPauseAim * 0.25f);
			}
			else
			{
				// we are forced to shoot
				SequenceCastAttack();
			}
		}
	}
	SyncFlipbookComponentWithTravelDirection();
}

void AGGShooterMinion::SequenceCastAttack()
{
	UE_LOG(GGMessage, Log, TEXT("Entity: uses attack"));
	TransitToActionState(EGGAIActionState::Attack);
	if (AnimatorComponent)
	{
		//	Animator will switch back to locomotion thereafter as should have been configured in Attack state
		AnimatorComponent->PerformAction(EGGActionCategory::Attack);				
	}
	if (FlipbookComponent)
	{
		FlipbookComponent->OnFinishedPlaying.AddDynamic(this, &AGGShooterMinion::CompleteAttack);
	}
	OnCastAttack();
}

void AGGShooterMinion::CompleteAttack()
{
	UE_LOG(GGMessage, Log, TEXT("Entity: completes attack"));
	FlipbookComponent->OnFinishedPlaying.RemoveDynamic(this, &AGGShooterMinion::CompleteAttack);
	TransitToActionState(EGGAIActionState::Evade);	
	OnCompleteAttack();
}

void AGGShooterMinion::TickEvade(float DeltaSeconds)
{
    Super::TickEvade(DeltaSeconds);
	TimeEvadedFor += DeltaSeconds;
	// escape analysis
	if (TimeEvadedFor >= AttackCooldown)
	{
		TimeEvadedFor = 0.f;
		TransitToActionState(EGGAIActionState::PrepareAttack);
		// don't bother moving this tick, pointless
		return;
	}
	// evasion instruction
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
	UE_LOG(GGMessage, Log, TEXT("Entity: turns"));
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

void AGGShooterMinion::ReceiveDamage(FGGDamageInformation & DamageInfo)
{
	Super::ReceiveDamage(DamageInfo);
	// play damage taking event
	FVector2D impactDirection = DamageInfo.GetImpactDirection();
	float forwardY = GetPlanarForwardVector().Y;
	// USE X, because its 2D vector!
	if (impactDirection.X * forwardY > 0.f)
	{
		FlipFlipbookComponent();
	}
}

void AGGShooterMinion::PlayDeathSequence()
{
	TransitToActionState(EGGAIActionState::Inactive);
	if (AnimatorComponent)
	{
		AnimatorComponent->PerformAction(EGGActionCategory::Death);
		AnimatorComponent->AlterActionMode(EGGActionMode::Mode0);
	}
}
