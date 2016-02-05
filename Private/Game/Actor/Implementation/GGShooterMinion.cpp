// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGShooterMinion.h"
#include "Game/Actor/GGCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"

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
		TravelDirection = GetPlanarForwardVector();
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
			SequenceTurnFacingDirection();
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
			TravelDirection = GetPlanarForwardVector();
			TravelDirection.Y *= IsFacingTarget() ? -1.f : 1.f;
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
				SequenceTurnFacingDirection();
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

void AGGShooterMinion::SequenceTurnFacingDirection()
{
	PauseBehaviourTick(TurnStagger);
	GetWorld()->GetTimerManager().SetTimer(TurnHandle, this, &AGGShooterMinion::FlipFlipbookComponent, TurnStagger * 0.5f);
}

void AGGShooterMinion::FlipFlipbookComponent()
{
	if (FlipbookComponent == nullptr)
	{
		return;
	}
	FVector NewScale = FVector(FlipbookComponent->RelativeScale3D.X, 1.f,1.f);
	FlipbookComponent->SetRelativeScale3D(NewScale);
}

void AGGShooterMinion::SyncFlipbookComponentWithTravelDirection()
{
	float FacingDirection = GetPlanarForwardVector().Y;
	// check for conflict
	if (
		(TravelDirection.Y > 0.f  && FacingDirection < 0.f) ||
		(TravelDirection.Y < 0.f  && FacingDirection > 0.f)
		)
	{
		FlipFlipbookComponent();
	}
}