// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGAIMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Game/Component/GGNpcLocomotionAnimComponent.h"
#include "Net/UnrealNetwork.h"

FName AGGMinionBase::CapsuleComponentName = TEXT("CapsuleComponent");

AGGMinionBase::AGGMinionBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    MovementCapsule = CreateDefaultSubobject<UCapsuleComponent>(AGGMinionBase::CapsuleComponentName);
    if (MovementCapsule)
    {
        SetRootComponent(MovementCapsule);
    }
}

void AGGMinionBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGGMinionBase, DamageNotify);
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

	HealthComponent = FindComponentByClass<UGGDamageReceiveComponent>();
	if (HealthComponent.IsValid())
	{
		HealthComponent.Get()->InitializeHpState();
		HealthComponent.Get()->OnZeroedHp.AddDynamic(this, &AGGMinionBase::PlayDeathSequence);
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
void AGGMinionBase::OnRep_DamageNotify()
{
	// the local causer do not rely on the OnRep to display the damage, hence the check for duplication
	// suffice as the server does not fire OnReps
	APlayerController* localPlayerController = GetWorld()->GetFirstPlayerController();
	if (localPlayerController && DamageNotify.CauserPlayerState != localPlayerController->PlayerState)
	{
		ReceiveDamage(DamageNotify);
	}
}

void AGGMinionBase::ReceiveDamage(FGGDamageInformation& DamageInfo)
{
	if (HealthComponent.IsValid())
	{
		HealthComponent.Get()->ApplyDamageInformation(DamageInfo);
	}
}

void AGGMinionBase::PlayDeathSequence()
{
	UE_LOG(GGWarning, Warning, TEXT("MinionBase empty implementation method: PlayDeathSequence called, meaningless"));
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
	if (Role == ROLE_Authority)
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
	if (Role < ROLE_Authority)
	{
		RootComponent->ComponentVelocity = ReplicatedMovement.LinearVelocity;
	}
	TickData(DeltaSeconds);
}

void AGGMinionBase::TickData(float DeltaSeconds){}

void AGGMinionBase::TickAnimation(float DeltaSeconds)
{
	UGGNpcLocomotionAnimComponent* ActiveAnimComp = GetActiveLocomotionAnimator();
	if (ActiveAnimComp && FlipbookComponent.IsValid())
	{
		FlipbookComponent.Get()->SetFlipbook(ActiveAnimComp->GetCurrentAnimation());
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
        UE_LOG(GGAIError, Warning, TEXT("Attempting to re-enable Behaviour Tick"));
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