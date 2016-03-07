// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGShooterMinion.h"
#include "Game/Actor/GGCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGNpcRangedAttackComponent.h"
#include "Game/Data/GGProjectileData.h"
#include "Game/Framework/GGGameState.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "Game/Actor/GGSpritePool.h"

void AGGShooterMinion::PostInitializeComponents()
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

	FlashHandler = FindComponentByClass<UGGFlipbookFlashHandler>();
	if (FlashHandler.IsValid())
	{
		UE_LOG(GGMessage, Log, TEXT("%s has no flashy flipbook handler"), *GetName());
	}
}

void AGGShooterMinion::OnReachWalkingBound()
{	
	bReachedWalkingBound = true;
}

void AGGShooterMinion::ReceiveDamage(FGGDamageDealingInfo DamageInfo)
{
	// takes care of damage receiving logic
	Super::ReceiveDamage(DamageInfo);
	UE_LOG(GGMessage, Log, TEXT("%s receive damage"), *GetName());
	// play damage taking event
	FVector2D impactDirection = DamageInfo.GetImpactDirection();
	float forwardY = GetPlanarForwardVector().Y;
	// USE X, because its 2D vector!
	if (impactDirection.X * forwardY > 0.f)
	{
		FlipFlipbookComponent();
	}
	if (FlashHandler.IsValid())
	{
		UE_LOG(GGMessage, Log, TEXT("%s Flashes"), *GetName());
		FlashHandler.Get()->SetFlashSchedule(FlipbookComponent.Get(), SecondsFlashesOnReceiveDamage);
	}
}

void AGGShooterMinion::PlayDeathSequence()
{
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	UGGNpcLocomotionAnimComponent* animator = PrimitiveAnimator.Get();
	if (flipbook && animator)
	{
		flipbook->SetFlipbook(animator->GetDeathFlipbook(Cache_DamageReceived.Type));
		flipbook->SetLooping(false);
		flipbook->OnFinishedPlaying.AddDynamic(this, &AGGShooterMinion::CompleteDeath);
		// plays death flipbook
		flipbook->PlayFromStart();
	}
}

void AGGShooterMinion::CompleteDeath()
{
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->OnFinishedPlaying.RemoveDynamic(this, &AGGShooterMinion::CompleteDeath);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	//TODO disable relevant components too
}

void AGGShooterMinion::OnSensorActivate_Implementation()
{
	if (ActionState == EGGAIActionState::Inactive)
	{
		check(Sensor.IsValid());
		Target = static_cast<AActor*>(Sensor.Get()->Target.Get());
		ActionState = EGGAIActionState::Patrol;
		EnableBehaviourTick();
	}
}

void AGGShooterMinion::OnSensorAlert_Implementation()
{
	if (ActionState == EGGAIActionState::Patrol)
	{
		check(Sensor.IsValid());
		if (IsFacingTarget())
		{
			ActionState = EGGAIActionState::PrepareAttack;
		}
	}
}

void AGGShooterMinion::OnSensorUnalert_Implementation()
{
	if (ActionState != EGGAIActionState::Patrol)
	{
		check(Sensor.IsValid());
		ActionState = EGGAIActionState::Patrol;		
	}
}

void AGGShooterMinion::OnSensorDeactivate_Implementation()
{
	check(Sensor.IsValid());
	ActionState = EGGAIActionState::Inactive;
	PauseBehaviourTick();
}

void AGGShooterMinion::TickAnimation(float DeltaSeconds)
{
	if (ActionState != EGGAIActionState::Attack)
	{
		Super::TickAnimation(DeltaSeconds);
	}
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
	else if (!bReachedWalkingBound)
	{
		TravelDirection = GetPlanarForwardVector();
	}
	else
	{
		TimeWalkedContinually = 0.f; // reset timer for next cycle
		SequenceTurnFacingDirection(TurnPausePatrol, TurnPausePatrol * 0.5f);
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
				MulticastAttack(0);
			}
		}
	}
	SyncFlipbookComponentWithTravelDirection();
}

void AGGShooterMinion::TickEvade(float DeltaSeconds)
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
				SequenceTurnFacingDirection(TurnPauseEvade, TurnPauseEvade*0.15f);
			}
		}
	}
	SyncFlipbookComponentWithTravelDirection();
}

void AGGShooterMinion::SyncFlipbookComponentWithTravelDirection()
{
	float Facing_Y = GetPlanarForwardVector().Y;
	float Velocity_Y = GetVelocity().Y;
	// check for conflict
	if (Facing_Y * Velocity_Y < 0.f)
	{
		FlipFlipbookComponent();
	}
}

void AGGShooterMinion::MinionAttack_Internal(uint8 InInstruction)
{
	ActionState = EGGAIActionState::Attack;
	UGGNpcRangedAttackComponent* locRAComp = RangedAttackComponent.Get();
	if (locRAComp)
	{
		PauseBehaviourTick();
		CurrentAttackCount = 0;
		GetWorld()->GetTimerManager().SetTimer(ActionHandle, this, &AGGShooterMinion::ShootForward, DelayBetweenShots, true, ShootStartupDelay);
	}
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{		
		flipbook->SetLooping(false);
		flipbook->SetFlipbook(AttackFlipbook);
		flipbook->OnFinishedPlaying.AddDynamic(this, &AGGShooterMinion::CompleteAttack);
		// plays attack flipbook
		flipbook->PlayFromStart();
	}
}

void AGGShooterMinion::ShootForward()
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
	if (CurrentAttackCount >= NumberOfShotsPerAttack)
	{
		GetWorld()->GetTimerManager().ClearTimer(ActionHandle);
	}
}

void AGGShooterMinion::CompleteAttack()
{
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->OnFinishedPlaying.RemoveDynamic(this, &AGGShooterMinion::CompleteAttack);
		flipbook->SetLooping(true);
		flipbook->Play();
	}
	ActionState = EGGAIActionState::Evade;
	bIsInActiveEvasionMode = false;
	EnableBehaviourTick();
}

bool AGGShooterMinion::IsTargetInSuppliedRange(const FVector2D& Range) const
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

bool AGGShooterMinion::IsFacingTarget() const
{
	if (Target.IsValid())
	{
		FVector Forward = GetPlanarForwardVector();
		FVector RelativePosition = Target.Get()->GetActorLocation() - GetActorLocation();
		return FVector::DotProduct(RelativePosition, Forward) > 0.f;
	}
	return false;
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
	if (FlipbookComponent.IsValid())
	{
		// bounds condition are changed when we flip flipbook
		bReachedWalkingBound = false;
		FVector NewScale = FVector(-FlipbookComponent.Get()->RelativeScale3D.X, 1.f, 1.f);
		FlipbookComponent.Get()->SetRelativeScale3D(NewScale);
	}	
}