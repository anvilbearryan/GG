// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGPunchMinion.h"
#include "Game/Component/GGNpcMeleeAttackComponent.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Game/Component/GGCharacterSensingComponent.h"

void AGGPunchMinion::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AttackComponent = FindComponentByClass<UGGNpcMeleeAttackComponent>();
	if (!AttackComponent.IsValid())
	{
		UE_LOG(GGMessage, Log, TEXT("%s has no attack component"), *GetName());
	}
	FlashHandler = FindComponentByClass<UGGFlipbookFlashHandler>();
	if (!FlashHandler.IsValid())
	{
		UE_LOG(GGMessage, Log, TEXT("%s has no flashy flipbook handler"), *GetName());
	}
}

void AGGPunchMinion::OnReachWalkingBound()
{
	bReachedWalkingBound = true;
}

void AGGPunchMinion::CommenceDamageReaction(const FGGDamageDealingInfo& InDamageInfo)
{
	Super::CommenceDamageReaction(InDamageInfo);
	// play damage taking event
	FVector2D impactDirection = InDamageInfo.GetImpactDirection();
	float forwardY = GetPlanarForwardVector().Y;

	// USE X, because its 2D vector!
	if (impactDirection.X * forwardY > 0.f)
	{
		FlipFlipbookComponent();
	}
	if (FlashHandler.IsValid())
	{		
		FlashHandler.Get()->SetFlashSchedule(FlipbookComponent.Get(), SecondsFlashesOnReceiveDamage);
	}
}

void AGGPunchMinion::CommenceDeathReaction()
{
	Super::CommenceDeathReaction();
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	UGGNpcLocomotionAnimComponent* animator = PrimitiveAnimator.Get();
	if (flipbook && animator)
	{
		flipbook->SetFlipbook(animator->GetDeathFlipbook(Cache_DamageReceived.Type));
		flipbook->SetLooping(false);
		flipbook->OnFinishedPlaying.AddDynamic(this, &AGGPunchMinion::OnCompleteDeathReaction);
		// plays death flipbook
		flipbook->PlayFromStart();
	}
}

void AGGPunchMinion::OnCompleteDeathReaction()
{
	Super::OnCompleteDeathReaction();
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->OnFinishedPlaying.RemoveDynamic(this, &AGGPunchMinion::OnCompleteDeathReaction);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	//TODO disable relevant components too
}

void AGGPunchMinion::OnSensorActivate_Implementation()
{
	if (ActionState == EGGAIActionState::Inactive)
	{
		check(Sensor.IsValid());
		Target = static_cast<AActor*>(Sensor.Get()->Target.Get());
		ActionState = EGGAIActionState::Patrol;
		EnableBehaviourTick();
	}
}

void AGGPunchMinion::OnSensorAlert_Implementation()
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

void AGGPunchMinion::OnSensorUnalert_Implementation()
{
	if (ActionState != EGGAIActionState::Patrol)
	{
		check(Sensor.IsValid());
		ActionState = EGGAIActionState::Patrol;
	}
}

void AGGPunchMinion::OnSensorDeactivate_Implementation()
{
	check(Sensor.IsValid());
	ActionState = EGGAIActionState::Inactive;
	PauseBehaviourTick();
}

void AGGPunchMinion::TickAnimation(float DeltaSeconds)
{
	if (ActionState != EGGAIActionState::Attack)
	{
		Super::TickAnimation(DeltaSeconds);
	}
}

void AGGPunchMinion::TickPatrol(float DeltaSeconds)
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

void AGGPunchMinion::TickPrepareAttack(float DeltaSeconds)
{
	Super::TickPrepareAttack(DeltaSeconds);
	if (IsFacingTarget())
	{		
		if (IsTargetInSuppliedRange(AttackMaxRange))
		{
			MulticastAttack(0);
		}
		else
		{
			// need to get closer
			TravelDirection = GetPlanarForwardVector();
		}
	}
	else
	{
		// must look at target to attack
		SequenceTurnFacingDirection(TurnPauseAim, TurnPauseAim * 0.25f);
	}
	SyncFlipbookComponentWithTravelDirection();
}

void AGGPunchMinion::TickEvade(float DeltaSeconds)
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
	if (!IsFacingTarget())
	{
		// turn to look at target
		SequenceTurnFacingDirection(TurnPauseEvade, TurnPauseEvade*0.15f);
	}
	else
	{
		// maintain distance with target so as to not lose sight of it, or stand if is within such distance
		if (bIsInActiveEvasionMode)
		{			
			if (IsTargetInSuppliedRange(EvasionSafeRange))
			{
				// switch back to non-evasion active mode
				bIsInActiveEvasionMode = false;
			}
			else
			{
				// move towards target				
				TravelDirection = GetPlanarForwardVector();
			}
		}
		else
		{
			// target was fairly close, check is he now moving away?		
			bIsInActiveEvasionMode = !IsTargetInSuppliedRange(EvasionTriggerRange);						
		}
	}	
	SyncFlipbookComponentWithTravelDirection();
}

void AGGPunchMinion::SyncFlipbookComponentWithTravelDirection()
{
	float Facing_Y = GetPlanarForwardVector().Y;
	float Velocity_Y = GetVelocity().Y;
	// check for conflict
	if (Facing_Y * Velocity_Y < 0.f)
	{
		FlipFlipbookComponent();
	}
}

void AGGPunchMinion::MinionAttack_Internal(uint8 InInstruction)
{
	ActionState = EGGAIActionState::Attack;
	
	UGGNpcMeleeAttackComponent* attackComp = AttackComponent.Get();
	if (attackComp)
	{
		attackComp->UseAttack(InInstruction);
	}

	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->SetLooping(false);
		flipbook->SetFlipbook(AttackFlipbook);
		flipbook->OnFinishedPlaying.AddDynamic(this, &AGGPunchMinion::CompleteAttack);
		// plays attack flipbook
		flipbook->PlayFromStart();
	}
}

void AGGPunchMinion::CompleteAttack()
{
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->OnFinishedPlaying.RemoveDynamic(this, &AGGPunchMinion::CompleteAttack);
		flipbook->SetLooping(true);
		flipbook->Play();
	}
	ActionState = EGGAIActionState::Evade;
	bIsInActiveEvasionMode = false;
	EnableBehaviourTick();
}

bool AGGPunchMinion::IsTargetInSuppliedRange(const FVector2D& Range) const
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

bool AGGPunchMinion::IsFacingTarget() const
{
	if (Target.IsValid())
	{
		FVector Forward = GetPlanarForwardVector();
		FVector RelativePosition = Target.Get()->GetActorLocation() - GetActorLocation();
		return FVector::DotProduct(RelativePosition, Forward) > 0.f;
	}
	return false;
}

void AGGPunchMinion::SequenceTurnFacingDirection(float TotalTimeToComplete, float FlipDelay)
{
	// avoid double turn, only execute if no existing timer on the TurnHandle set
	FTimerManager& tm = GetWorld()->GetTimerManager();
	float nextTurnRemaining = tm.GetTimerRemaining(TurnHandle);
	if (nextTurnRemaining <= 0)
	{
		PauseBehaviourTick(TotalTimeToComplete);
		GetWorld()->GetTimerManager().SetTimer(TurnHandle, this, &AGGPunchMinion::FlipFlipbookComponent, FlipDelay);
	}
}

void AGGPunchMinion::FlipFlipbookComponent()
{
	if (FlipbookComponent.IsValid())
	{
		// bounds condition are changed when we flip flipbook
		bReachedWalkingBound = false;
		FVector NewScale = FVector(-FlipbookComponent.Get()->RelativeScale3D.X, 1.f, 1.f);
		FlipbookComponent.Get()->SetRelativeScale3D(NewScale);
	}
}
