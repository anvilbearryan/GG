// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGAIMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Net/UnrealNetwork.h"

#include "Game/Framework/GGModeInGame.h"
#include "Game/Component/GGFlipbookFlashHandler.h"

FName AGGMinionBase::CapsuleComponentName = TEXT("CapsuleComponent");

AGGMinionBase::AGGMinionBase() : Super()
{
	bReplicateMovement = true;
    MovementCapsule = CreateDefaultSubobject<UCapsuleComponent>(AGGMinionBase::CapsuleComponentName);
    if (MovementCapsule)
    {
        SetRootComponent(MovementCapsule);
    }
}

void AGGMinionBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGMinionBase, ActionState);
}

void AGGMinionBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	MovementComponent = FindComponentByClass<UGGAIMovementComponent>();
	if (MovementComponent.IsValid())
	{
		MovementComponent.Get()->SetUpdatedComponent(MovementCapsule);
		MovementComponent.Get()->MinionOwner = this;
	}

	FlipbookComponent = FindComponentByClass<UPaperFlipbookComponent>();    	
	if (FlipbookComponent.IsValid())
	{
		FlipbookComponent.Get()->SetLooping(true);
	}
	/** Bind pawn sensing delegates */
	Sensor = FindComponentByClass<UGGCharacterSensingComponent>();
	UGGCharacterSensingComponent* loc_Sensor = Sensor.Get();
	if (loc_Sensor)
	{
		loc_Sensor->OnActivate.BindDynamic(this, &AGGMinionBase::OnSensorActivate);
		loc_Sensor->OnAlert.BindDynamic(this, &AGGMinionBase::OnSensorAlert);
		loc_Sensor->OnUnalert.BindDynamic(this, &AGGMinionBase::OnSensorUnalert);
		loc_Sensor->OnDeactivate.BindDynamic(this, &AGGMinionBase::OnSensorDeactivate);
	}

	PrimitiveAnimator = FindComponentByClass<UGGNpcLocomotionAnimComponent>();

	FlashHandler = FindComponentByClass<UGGFlipbookFlashHandler>();
	if (!FlashHandler.IsValid())
	{
		UE_LOG(GGMessage, Log, TEXT("%s has no flashy flipbook handler"), *GetName());
	}
}

//********************************

//	Movement callbacks
void AGGMinionBase::SetMovementBase(UPrimitiveComponent* NewBaseComponent, UActorComponent* InMovementComponent)
{
	UPrimitiveComponent* OldBase = BasePlatform.PlatformPrimitive;
	if (OldBase == NewBaseComponent)
	{
		//UE_LOG(LogActor, Warning, TEXT("SetBase procedure not executed, already using component as base"));
		return;
	}

	AActor* Loop = (NewBaseComponent ? Cast<AActor>(NewBaseComponent->GetOwner()) : NULL);
	if (Loop == this)
	{
		UE_LOG(GGAIError, Warning, TEXT(" SetBase failed! trying to set self as base."));
		return;
	}

	UGGAIMovementComponent* loc_MovementComp = MovementComponent.Get();
	if (loc_MovementComp)
	{
		const bool bBaseChanged = (NewBaseComponent != BasePlatform.PlatformPrimitive);
		if (bBaseChanged)
		{
			MovementBaseUtility::RemoveTickDependency(loc_MovementComp->PrimaryComponentTick, BasePlatform.PlatformPrimitive);
			MovementBaseUtility::AddTickDependency(loc_MovementComp->PrimaryComponentTick, NewBaseComponent);

			// Opportunity to notify for base updates
			BasePlatform.PlatformPrimitive = NewBaseComponent;
			BasePlatform.bIsDynamic = NewBaseComponent->Mobility == EComponentMobility::Movable;
			UE_LOG(LogActor, Warning, TEXT("SetBase to new base"));
		}
	}
}

void AGGMinionBase::OnReachWalkingBound()
{
}

//********************************

//	Damage interface
void AGGMinionBase::CommenceDamageReaction(const FGGDamageDealingInfo& InDamageInfo)
{
	Super::CommenceDamageReaction(InDamageInfo);
	if (FlashHandler.IsValid())
	{
		FlashHandler.Get()->SetFlashSchedule(FlipbookComponent.Get(), SecondsFlashesOnReceiveDamage);
	}
	if (SecondsFlashesOnReceiveDamage > 0)
	{		
		FTimerManager& tm = GetWorld()->GetTimerManager();
		if (SecondsFlashesOnReceiveDamage > tm.GetTimerRemaining(BehaviourHandle))
		{
			PauseBehaviourTick(SecondsFlashesOnReceiveDamage);			
		}
	}
	if (ActionState == EGGAIActionState::Patrol || ActionState == EGGAIActionState::Inactive)
	{
		OnSensorAlert();
	}
}

void AGGMinionBase::OnCompleteDamageReaction()
{
	Super::OnCompleteDamageReaction();
}

void AGGMinionBase::CommenceDeathReaction()
{
	Super::CommenceDeathReaction();

	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	UGGNpcLocomotionAnimComponent* animator = PrimitiveAnimator.Get();
	if (flipbook && animator)
	{
		flipbook->SetFlipbook(animator->GetDeathFlipbook(Cache_DamageReceived.Type));
		flipbook->SetLooping(false);
		flipbook->OnFinishedPlaying.AddDynamic(this, &AGGMinionBase::OnCompleteDeathReaction);
		// plays death flipbook
		flipbook->PlayFromStart();
	}
}

void AGGMinionBase::OnCompleteDeathReaction()
{
	Super::OnCompleteDeathReaction();
	
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (flipbook)
	{
		flipbook->OnFinishedPlaying.RemoveDynamic(this, &AGGMinionBase::OnCompleteDeathReaction);
	}
	
	if (Role == ROLE_Authority)
	{
		UWorld* world = GetWorld();
		if (world)
		{
			AGGModeInGame* gamemode = world->GetAuthGameMode<AGGModeInGame>();
			if (gamemode) 
			{
				gamemode->OnMinionKilledByPlayer(this, Cache_DamageReceived.CauserPlayerState);
			}
		}
	}
}

//********************************

//	Sensing callbacks
void AGGMinionBase::OnSensorActivate_Implementation()
{
	bIsBehaviourTickEnabled = true;
}

void AGGMinionBase::OnSensorAlert_Implementation()
{
}

void AGGMinionBase::OnSensorUnalert_Implementation()
{
}

void AGGMinionBase::OnSensorDeactivate_Implementation()
{
	bIsBehaviourTickEnabled = false;
}

//********************************

//	Npc generic behaviour interface
void AGGMinionBase::Tick( float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (true) //Role == ROLE_Authority)
	{
		TickBehaviour(DeltaSeconds);
	}
	TickData_Internal(DeltaSeconds);
	TickAnimation(DeltaSeconds);	
}

void AGGMinionBase::TickBehaviour(float DeltaSeconds)
{
	TravelDirection = FVector::ZeroVector;
	if (bIsBehaviourTickEnabled && Target.IsValid())
	{
		switch (ActionState)
		{
		case EGGAIActionState::Patrol:
			TickPatrol(DeltaSeconds);
			break;
		case EGGAIActionState::PrepareAttack:
			TickPrepareAttack(DeltaSeconds);
			break;
		case EGGAIActionState::Evade:
			TickEvade(DeltaSeconds);
			break;
		}
	}
}

void AGGMinionBase::TickData_Internal(float DeltaSeconds)
{
	if (Role == ROLE_SimulatedProxy)
	{
		RootComponent->ComponentVelocity = (ReplicatedMovement.LinearVelocity + RootComponent->ComponentVelocity) * 0.5f;
		//UE_LOG(GGMessage, Log, TEXT("ComponentVelocity: %s"), *RootComponent->ComponentVelocity.ToString());
	}
	TickData(DeltaSeconds);
}

void AGGMinionBase::TickData(float DeltaSeconds){}

void AGGMinionBase::TickAnimation(float DeltaSeconds)
{
	UGGNpcLocomotionAnimComponent* ActiveAnimComp = GetActiveLocomotionAnimator();
	UPaperFlipbookComponent* flipbook = FlipbookComponent.Get();
	if (ActiveAnimComp && flipbook)//&& flipbook->IsLooping())
	{
		flipbook->SetFlipbook(ActiveAnimComp->GetCurrentAnimation());
		flipbook->Play();		
	}
	else
	{
		UE_LOG(GGMessage, Warning, TEXT("TickAnimation checks fail"));
	}
}

UGGNpcLocomotionAnimComponent* AGGMinionBase::GetActiveLocomotionAnimator()
{
	return PrimitiveAnimator.Get();
}

void AGGMinionBase::PauseBehaviourTick(float Duration)
{
	bIsBehaviourTickEnabled = false;
	if (Duration > 0.f) 
	{
		GetWorld()->GetTimerManager().SetTimer(BehaviourHandle, this, &AGGMinionBase::EnableBehaviourTick, Duration);
	}	
}

void AGGMinionBase::EnableBehaviourTick()
{
    if (bIsBehaviourTickEnabled)
    {
        UE_LOG(GGAIError, Warning, TEXT("%s Attempting to re-enable Behaviour Tick"), *GetName());
        return;
    }
    bIsBehaviourTickEnabled = true;
}

void AGGMinionBase::TickPatrol(float DeltaSeconds)
{
    
}

void AGGMinionBase::TickPrepareAttack(float DeltaSeconds)
{
    
}

void AGGMinionBase::TickEvade(float DeltaSeconds)
{
    
}

//********************************

//	Npc generic attack interface
void AGGMinionBase::MulticastAttack_Implementation(uint8 InInstruction)
{
	MinionAttack_Internal(InInstruction);
}

void AGGMinionBase::MinionAttack_Internal(uint8 InInstruction)
{
	AttackInstructionCache = InInstruction;
}

//********************************

//	General utilities 
FVector AGGMinionBase::GGGetTargetLocation() const
{
	const AActor* locTarget = Target.Get();
	return !!locTarget ? locTarget->GetActorLocation() : FVector::ZeroVector;
}

float AGGMinionBase::GetHalfHeight() const
{
    if (MovementCapsule)
    {
        return MovementCapsule->GetScaledCapsuleHalfHeight();
    }
    return 0.f;
}

float AGGMinionBase::GetHalfWidth() const
{
    if (MovementCapsule)
    {
        return MovementCapsule->GetScaledCapsuleRadius();
    }
    return 0.f;
}

FVector AGGMinionBase::Right = FVector(0.f, 1.f, 0.f);
FVector AGGMinionBase::Left = FVector(0.f, -1.f, 0.f);

FVector AGGMinionBase::GetPlanarForwardVector() const
{
    if (FlipbookComponent.IsValid())
    {
        return FlipbookComponent.Get()->RelativeScale3D.X > 0 ? Right : Left;
    }
    return Right;
}

namespace GGMovementBaseUtils
{
    void AddTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* NewBase)
    {
        if (NewBase && NewBase->Mobility == EComponentMobility::Movable)
        {
            if (NewBase->PrimaryComponentTick.bCanEverTick)
            {
                BasedObjectTick.AddPrerequisite(NewBase, NewBase->PrimaryComponentTick);
            }
            
            AActor* NewBaseOwner = NewBase->GetOwner();
            if (NewBaseOwner)
            {
                if (NewBaseOwner->PrimaryActorTick.bCanEverTick)
                {
                    BasedObjectTick.AddPrerequisite(NewBaseOwner, NewBaseOwner->PrimaryActorTick);
                }
                
                // @TODO: We need to find a more efficient way of finding all ticking components in an actor.
                for (UActorComponent* Component : NewBaseOwner->GetComponents())
                {
                    if (Component && Component->PrimaryComponentTick.bCanEverTick)
                    {
                        BasedObjectTick.AddPrerequisite(Component, Component->PrimaryComponentTick);
                    }
                }
            }
        }
    }
    
    void RemoveTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* OldBase)
    {
        if (OldBase && OldBase->Mobility == EComponentMobility::Movable)
        {
            BasedObjectTick.RemovePrerequisite(OldBase, OldBase->PrimaryComponentTick);
            AActor* OldBaseOwner = OldBase->GetOwner();
            if (OldBaseOwner)
            {
                BasedObjectTick.RemovePrerequisite(OldBaseOwner, OldBaseOwner->PrimaryActorTick);
                
                // @TODO: We need to find a more efficient way of finding all ticking components in an actor.
                for (UActorComponent* Component : OldBaseOwner->GetComponents())
                {
                    if (Component && Component->PrimaryComponentTick.bCanEverTick)
                    {
                        BasedObjectTick.RemovePrerequisite(Component, Component->PrimaryComponentTick);
                    }
                }
            }
        }
    }
}