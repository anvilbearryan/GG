// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGShooterDrone.h"
#include "Game/Actor/GGCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGNpcRangedAttackComponent.h"
#include "Game/Data/GGProjectileData.h"
#include "Game/Framework/GGGameState.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "Game/Actor/GGSpritePool.h"
#include "Game/Component/GGAIMovementComponent.h"

void AGGShooterDrone::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RangedAttackComponent = FindComponentByClass<UGGNpcRangedAttackComponent>();
	if (RangedAttackComponent.IsValid())
	{
		AGGGameState* loc_GS = GetWorld()->GetGameState<AGGGameState>();
		UGGNpcRangedAttackComponent* locRangedAtk = RangedAttackComponent.Get();
		if (loc_GS)
		{
			locRangedAtk->SpritePool = loc_GS->GetSpritePool();
		}
	}
	if (AttackProjectileData)
	{
		AttackProjectileData->RecalculateCaches();
	}
}

void AGGShooterDrone::OnReachWalkingBound()
{
	bReachedWalkingBound = true;
}

void AGGShooterDrone::OnSensorActivate_Implementation()
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

void AGGShooterDrone::OnSensorAlert_Implementation()
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

void AGGShooterDrone::OnSensorUnalert_Implementation()
{
	if (ActionState != EGGAIActionState::Patrol)
	{
		check(Sensor.IsValid());
		ActionState = EGGAIActionState::Patrol;
	}
}

void AGGShooterDrone::OnSensorDeactivate_Implementation()
{
	check(Sensor.IsValid());
	ActionState = EGGAIActionState::Inactive;
	PauseBehaviourTick();
}

void AGGShooterDrone::TickAnimation(float DeltaSeconds)
{
	if (ActionState != EGGAIActionState::Attack)
	{
		Super::TickAnimation(DeltaSeconds);

		// sync flipbook facing
		SyncFlipbookComponentWithTravelDirection();
	}
}

void AGGShooterDrone::TickPatrol(float DeltaSeconds)
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
		if ((DestinationY - CentreGuardPosition.Y) * CurrentDirectionY > PatrolRange.X)
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
		float OldDestinationY = DestinationY;
		float OldCurrentDirectionY = CurrentDirectionY;

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
		/*
		else
		{
			float dz = DistanceFromGround() - HoverDistanceZ;
			TravelDirection = FVector(0.f, CurrentDirectionY, -FMath::Clamp(dz, -0.05f, 0.05f));
		}
		*/

		//experiment: if above cases doesn't change anything means we are very stuck can only resolve in Z
		if ( FMath::Abs(DestinationY - OldDestinationY) < 0.1f 
			&& FMath::Abs(CurrentDirectionY - OldCurrentDirectionY) < 0.1f)
		{
			float dz = DistanceFromGround() - HoverDistanceZ;				
			TravelDirection = FVector(0.f, 0.f, -FMath::Sign(dz));
		}
	}
}

void AGGShooterDrone::TickPrepareAttack(float DeltaSeconds)
{
	Super::TickPrepareAttack(DeltaSeconds);
	bool IsInAttackMaxRange = IsTargetInSuppliedRange(AttackMaxRange);
	bool IsInAttackMinRange = IsTargetInSuppliedRange(AttackMinRange);
	if (MaxPrepareTime < TimePreparedFor || (IsInAttackMaxRange && !IsInAttackMinRange))
	{
		// we may need to do a turn first
		if (Role == ROLE_Authority && IsFacingTarget())
		{
			// can attack
			MulticastAttack(0);
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
			FVector ds = Target->GetActorLocation() - GetActorLocation();
			TravelDirection = ds.GetUnsafeNormal();
			if (FMath::Abs(ds.Y) < AttackMaxRange.X)
			{
				TravelDirection.Y = 0.f;
			}
			if (FMath::Abs(ds.Z) < AttackMaxRange.Y)
			{
				TravelDirection.Z = 0.f;
			}
		}
		else if (IsInAttackMinRange)
		{
			// move away from target in Y, while attempt to get ever closer in Z
			if (!bReachedWalkingBound)
			{
				FVector deltaPosition = GetActorLocation() - Target->GetActorLocation();
				TravelDirection.Y = FMath::Sign(deltaPosition.Y);
				TravelDirection.Z = -FMath::Sign(deltaPosition.Z) * 0.05f;
			}
			// implied bReachedWalkingBound
			else if (!IsFacingTarget())
			{
				SequenceTurnFacingDirection(TurnPauseAim, TurnPauseAim * 0.25f);
			}
			else
			{
				// we are forced to shoot
				MulticastAttack(0);
			}
		}
	}
}

void AGGShooterDrone::TickEvade(float DeltaSeconds)
{
	Super::TickEvade(DeltaSeconds);
	TimeEvadedFor += DeltaSeconds;
	// escape analysis
	if (TimeEvadedFor >= AttackCooldown)
	{
		TimeEvadedFor = 0.f;
		ActionState = EGGAIActionState::PrepareAttack;
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
			TravelDirection = FVector::ZeroVector;
		}
		else
		{
			// run away from target
			float dy = GetActorLocation().Y - Target.Get()->GetActorLocation().Y;			
			/*	Whether we have reached bounds does not dictate our direction of travel while evading */
			if (dy > 0)
			{
				TravelDirection = FVector(0.f, 1.f, 0.f);
			}
			else
			{
				TravelDirection = FVector(0.f, -1.f, 0.f);
			}			
		}
		// try and go back to ground level regardless of situation when evading
		float dz = DistanceFromGround() - HoverDistanceZ;
		TravelDirection.Z = -FMath::Clamp(dz, -0.05f, 0.05f);
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
			// Try and go back to initial position
			FVector deltaPosition = CentreGuardPosition - GetActorLocation();
			if (deltaPosition.GetAbsMax() > 25.f)
			{
				TravelDirection = deltaPosition.GetSafeNormal(25.f); // 25 effectively is the acceptance radius
			}
			else
			{				
				if (!IsFacingTarget())
				{
					TravelDirection = GetPlanarForwardVector();
					TravelDirection *= 0.05f; // nudge backward a little to flip
				}
			}
		}
	}	
}

void AGGShooterDrone::SyncFlipbookComponentWithTravelDirection()
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

void AGGShooterDrone::MinionAttack_Internal(uint8 InInstruction)
{
	Super::MinionAttack_Internal(InInstruction);
	ActionState = EGGAIActionState::Attack;
	UGGNpcRangedAttackComponent* locRAComp = RangedAttackComponent.Get();
	if (locRAComp)
	{
		PauseBehaviourTick();
		CurrentAttackCount = 0;
		GetWorld()->GetTimerManager().SetTimer(ActionHandle, this, &AGGShooterDrone::ShootForward, DelayBetweenShots, true, ShootStartupDelay);
	}
	else
	{
		UE_LOG(GGWarning, Warning, TEXT("Ranged attack component not found"));
	}
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->SetLooping(false);
		flipbook->SetFlipbook(AttackFlipbook);
		flipbook->OnFinishedPlaying.AddDynamic(this, &AGGShooterDrone::CompleteAttack);
		// plays attack flipbook
		flipbook->PlayFromStart();
	}
}

void AGGShooterDrone::ShootForward()
{
	CurrentAttackCount++;
	UGGNpcRangedAttackComponent* locRAComp = RangedAttackComponent.Get();
	UPaperFlipbookComponent* locFipbook = FlipbookComponent.Get();
	if (locRAComp && locFipbook)
	{
		FVector facingDirection = GetPlanarForwardVector();
		FVector shootLocation = GetActorLocation() + ShootOffset * FVector(1.f, facingDirection.Y, 1.f);
		locRAComp->LaunchProjectile(AttackProjectileData,
			shootLocation, facingDirection, locFipbook->RelativeScale3D);
	}
	else
	{
		UE_LOG(GGWarning, Warning, TEXT("No ranged attack component or flipbook, probably former"));
	}
	if (CurrentAttackCount >= NumberOfShotsPerAttack)
	{
		GetWorld()->GetTimerManager().ClearTimer(ActionHandle);
	}
}

void AGGShooterDrone::CompleteAttack()
{
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->OnFinishedPlaying.RemoveDynamic(this, &AGGShooterDrone::CompleteAttack);
		flipbook->SetLooping(true);
		flipbook->Play();
	}
	ActionState = EGGAIActionState::Evade;
	bIsInActiveEvasionMode = false;
	EnableBehaviourTick();

	UE_LOG(GGWarning, Warning, TEXT("ShooterDrone CompleteAttack"));
}

bool AGGShooterDrone::IsTargetInSuppliedRange(const FVector2D& Range) const
{
	if (!Target.IsValid())
	{
		return false;
	}
	FVector TargetPosition = Target.Get()->GetActorLocation();
	FVector MyPosition = GetActorLocation();

	float dx = TargetPosition.Y - MyPosition.Y;
	float dy = TargetPosition.Z - MyPosition.Z;

	return Range.X >= FMath::Abs(dx) && Range.Y >= FMath::Abs(dy);
}

bool AGGShooterDrone::IsFacingTarget() const
{
	if (Target.IsValid())
	{
		FVector Forward = GetPlanarForwardVector();
		FVector RelativePosition = Target.Get()->GetActorLocation() - GetActorLocation();
		return FVector::DotProduct(RelativePosition, Forward) >= 0.f;
	}
	return false;
}

float AGGShooterDrone::DistanceFromGround() const
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

void AGGShooterDrone::SequenceTurnFacingDirection(float TotalTimeToComplete, float FlipDelay)
{
	// avoid double turn, only execute if no existing timer on the TurnHandle set
	FTimerManager& tm = GetWorld()->GetTimerManager();
	float nextTurnRemaining = tm.GetTimerRemaining(TurnHandle);
	if (nextTurnRemaining <= 0)
	{
		PauseBehaviourTick(TotalTimeToComplete);
		GetWorld()->GetTimerManager().SetTimer(TurnHandle, this, &AGGShooterDrone::FlipFlipbookComponent, FlipDelay);
	}
}

void AGGShooterDrone::FlipFlipbookComponent()
{
	if (FlipbookComponent.IsValid())
	{
		// bounds condition are changed when we flip flipbook
		bReachedWalkingBound = false;
		FVector NewScale = FVector(-FlipbookComponent.Get()->RelativeScale3D.X, 1.f, 1.f);
		FlipbookComponent.Get()->SetRelativeScale3D(NewScale);
	}
}