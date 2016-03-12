// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGMeleeDrone.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGAIMovementComponent.h"

void AGGMeleeDrone::PostInitializeComponents()
{
	Super::PostInitializeComponents();	
}

void AGGMeleeDrone::OnReachWalkingBound()
{
	bReachedWalkingBound = true;	
}

void AGGMeleeDrone::OnSensorActivate_Implementation()
{
	if (ActionState == EGGAIActionState::Inactive)
	{
		check(Sensor.IsValid());
		Target = static_cast<AActor*>(Sensor.Get()->Target.Get());
		ActionState = EGGAIActionState::Patrol;
		if (CentreGuardPosition == FVector::ZeroVector)
		{
			CentreGuardPosition = GetActorLocation();
			CurrentDirectionY = GetPlanarForwardVector().Y;
			DestinationY = CurrentDirectionY * PatrolRange.X + CentreGuardPosition.Y;
		}
		EnableBehaviourTick();

		CurrentDirectionY = (GetActorLocation().Y - CentreGuardPosition.Y) < 0 ? 1.f : -1.f;
	}
}

void AGGMeleeDrone::OnSensorAlert_Implementation()
{
	if (ActionState == EGGAIActionState::Patrol)
	{
		check(Sensor.IsValid());
		if (IsFacingTarget())
		{
			ActionState = EGGAIActionState::PrepareAttack;
		}
	}
	else if (ActionState == EGGAIActionState::Inactive)
	{
		check(Sensor.IsValid());
		Target = static_cast<AActor*>(Sensor.Get()->Target.Get());
		if (IsFacingTarget())
		{			
			ActionState = EGGAIActionState::PrepareAttack;
		}
		else
		{
			ActionState = EGGAIActionState::Patrol;
		}
		EnableBehaviourTick();
	}
}

void AGGMeleeDrone::OnSensorUnalert_Implementation()
{
	if (ActionState != EGGAIActionState::Patrol)
	{
		check(Sensor.IsValid());
		ActionState = EGGAIActionState::Patrol;
	}
}

void AGGMeleeDrone::OnSensorDeactivate_Implementation()
{
	check(Sensor.IsValid());
	ActionState = EGGAIActionState::Inactive;
	PauseBehaviourTick();
}

void AGGMeleeDrone::TickAnimation(float DeltaSeconds)
{
	if (ActionState != EGGAIActionState::Attack)
	{
		Super::TickAnimation(DeltaSeconds);
		// sync flipbook facing
		SyncFlipbookComponentWithTravelDirection();
	}
}

void AGGMeleeDrone::TickPatrol(float DeltaSeconds)
{
	Super::TickPatrol(DeltaSeconds);

	/** state change condition is checked by CharacterSensing component and applied through SwitchToPrepareAttack,
	*	we need not worry here.
	*/
	TImeWalkedContinually += DeltaSeconds;
	if (bTakesTimeBasedPause && TImeWalkedContinually > TimeBasedPauseInterval)
	{
		// time to take a break
		TImeWalkedContinually = 0;
		PauseBehaviourTick(PauseDuration);
	}
	else if (!bReachedWalkingBound)
	{
		//************ Rebase Z, constrain rect
		float dy = DestinationY - GetActorLocation().Y;
		float dz = DistanceFromGround() - HoverDistanceZ;
		if ( (DestinationY - CentreGuardPosition.Y) * CurrentDirectionY > PatrolRange.X)
		{
			DestinationY = CentreGuardPosition.Y - CurrentDirectionY * PatrolRange.X;
			CurrentDirectionY *= -1.f;
			TImeWalkedContinually = 0.f; // reset timer for next cycle
			SequenceTurnFacingDirection(TurnPausePatrol, TurnPausePatrol * 0.25f);
			UE_LOG(GGMessage, Log, TEXT("Flip from patrol"));
		}		
		TravelDirection = FVector(0.f, CurrentDirectionY, -FMath::Clamp(dz, -0.05f, 0.05f));
		TravelDirection.X = 0.f;
	}
	else
	{
		TImeWalkedContinually = 0.f; // reset timer for next cycle
		float myY = GetActorLocation().Y;
		if (myY > CentreGuardPosition.Y && CurrentDirectionY > 0.f)
		{
			DestinationY = CentreGuardPosition.Y - PatrolRange.X;
			CurrentDirectionY = -1.f;
			SequenceTurnFacingDirection(TurnPausePatrol, TurnPausePatrol * 0.25f);
			TImeWalkedContinually = 0.f;
		}
		else if (myY < CentreGuardPosition.Y && CurrentDirectionY < 0.f)
		{
			DestinationY = CentreGuardPosition.Y + PatrolRange.X;
			CurrentDirectionY = 1.f;
			SequenceTurnFacingDirection(TurnPausePatrol, TurnPausePatrol * 0.25f);
			TImeWalkedContinually = 0.f;
		}
		else
		{
			float dz = DistanceFromGround() - HoverDistanceZ;
			TravelDirection = FVector(0.f, CurrentDirectionY, -FMath::Clamp(dz, -0.05f, 0.05f));
		}
	}
}

void AGGMeleeDrone::TickPrepareAttack(float DeltaSeconds)
{
	Super::TickPrepareAttack(DeltaSeconds);
	TimePreparedFor += DeltaSeconds;
	if (IsFacingTarget())
	{
		// need to get closer
		FVector ds = Target->GetActorLocation() - GetActorLocation();
		if (ds.GetAbsMax() < AttackMaxDistance || TimePreparedFor > MaxPrepareTime)
		{
			TravelDirection = FVector::ZeroVector;
			ActionState = EGGAIActionState::Evade;
			TimePreparedFor = 0.f;
		}
		else
		{
			TravelDirection = GetPlanarForwardVector();
			TravelDirection.Z = FMath::Abs(ds.Z) > AttackMaxDistance ? FMath::Sign(ds.Z) : 0.f;
			TravelDirection.X = 0.f; // secure			
		}		
	}
	else
	{
		// must look at target to attack
		SequenceTurnFacingDirection(TurnPauseAim, TurnPauseAim * 0.25f);
		UE_LOG(GGMessage, Log, TEXT("Flip from prepare"));
	}	
}

void AGGMeleeDrone::TickEvade(float DeltaSeconds)
{
	Super::TickEvade(DeltaSeconds);
	TimeEvadedFor += DeltaSeconds;
	// escape analysis
	if (TimeEvadedFor >= AttackCooldown)
	{
		TimeEvadedFor = 0.f;
		ActionState = EGGAIActionState::PrepareAttack;
		// don't bother moving this tick, pointless
	}
	else if (!IsFacingTarget())
	{
		// turn to look at target
		SequenceTurnFacingDirection(TurnPauseEvade, TurnPauseEvade*0.15f);

		UE_LOG(GGMessage, Log, TEXT("Flip from evade"));
	}		
}

void AGGMeleeDrone::SyncFlipbookComponentWithTravelDirection()
{
	if (GetWorld()->GetTimerManager().GetTimerRemaining(TurnHandle) > 0.f)
	{
		return;
	}
	float Facing_Y = GetPlanarForwardVector().Y;
	float Velocity_Y = GetVelocity().Y;
	// check for conflict
	if (Facing_Y * Velocity_Y < -25.f && !FMath::IsNaN(Velocity_Y))
	{
		//UE_LOG(GGMessage, Log, TEXT("Flip from sync %f"), Facing_Y * Velocity_Y);
		FlipFlipbookComponent();
	}
}

bool AGGMeleeDrone::IsFacingTarget() const
{
	if (Target.IsValid())
	{
		FVector RelativePosition = Target.Get()->GetActorLocation() - GetActorLocation();
		if (FMath::Abs(RelativePosition.Y) > 25.f)
		{
			FVector Forward = GetPlanarForwardVector();
			return RelativePosition.Y * Forward.Y > 0.f;
		}
		else
		{
			return true;
		}
	}
	return true;
}

float AGGMeleeDrone::DistanceFromGround() const
{
	UGGAIMovementComponent* movecomp = MovementComponent.Get();
	if (movecomp)
	{
		FHitResult result;
		FVector Start = RootComponent->GetComponentLocation();
		Start.Y = Start.Y;
		Start.Z -= GetHalfHeight();
		FVector End = Start;
		End.Z = End.Z - HoverDistanceZ - 25.f;
		GetWorld()->LineTraceSingleByChannel(result, Start, End, movecomp->SteppingChannel, movecomp->GroundQueryParams);
		if (result.bBlockingHit)
		{
			return FMath::Abs(result.Distance);
		}
		GetWorld()->LineTraceSingleByChannel(result, Start, End, movecomp->PlatformChannel, movecomp->GroundQueryParams);
		if (result.bBlockingHit)
		{
			return FMath::Abs(result.Distance);
		}
		return HoverDistanceZ;
	}
	else
	{
		return HoverDistanceZ;
	}
}

void AGGMeleeDrone::SequenceTurnFacingDirection(float TotalTimeToComplete, float FlipDelay)
{
	// avoid double turn, only execute if no existing timer on the TurnHandle set
	FTimerManager& tm = GetWorld()->GetTimerManager();
	float nextTurnRemaining = tm.GetTimerRemaining(TurnHandle);
	if (nextTurnRemaining <= 0)
	{
		PauseBehaviourTick(TotalTimeToComplete);
		GetWorld()->GetTimerManager().SetTimer(TurnHandle, this, &AGGMeleeDrone::FlipFlipbookComponent, FlipDelay);
	}
}

void AGGMeleeDrone::FlipFlipbookComponent()
{
	if (FlipbookComponent.IsValid())
	{
		// bounds condition are changed when we flip flipbook
		bReachedWalkingBound = false;
		FVector NewScale = FVector(-FlipbookComponent.Get()->RelativeScale3D.X, 1.f, 1.f);
		FlipbookComponent.Get()->SetRelativeScale3D(NewScale);
	}
}





