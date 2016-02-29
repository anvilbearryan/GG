// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGAIMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGAnimatorComponent.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Net/UnrealNetwork.h"

FName AGGMinionBase::CapsuleComponentName = TEXT("CapsuleComponent");

// Sets default values
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
}
// Called when the game starts or when spawned
void AGGMinionBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
    /** Bind pawn sensing delegates */
	Sensor = FindComponentByClass<UGGCharacterSensingComponent>();
    if (Sensor)
    {        
        Sensor->OnActivate.BindDynamic(this, &AGGMinionBase::OnSensorActivate);
        Sensor->OnAlert.BindDynamic(this, &AGGMinionBase::OnSensorAlert);
        Sensor->OnUnalert.BindDynamic(this, &AGGMinionBase::OnSensorUnalert);
        Sensor->OnDeactivate.BindDynamic(this, &AGGMinionBase::OnSensorDeactivate);
    }
	MovementComponent = FindComponentByClass<UGGAIMovementComponent>();
	if (MovementComponent)
	{
		MovementComponent->SetUpdatedComponent(MovementCapsule);
		MovementComponent->MinionOwner = this;
	}

	FlipbookComponent = FindComponentByClass<UPaperFlipbookComponent>();
    
	AnimatorComponent = FindComponentByClass<UGGAnimatorComponent>();
    
    if (AnimatorComponent  && FlipbookComponent)
    {
        AnimatorComponent->PlaybackComponent = FlipbookComponent;
        AnimatorComponent->TickCurrentBlendSpace();
        FlipbookComponent->OnFinishedPlaying.AddDynamic(AnimatorComponent, &UGGAnimatorComponent::OnReachEndOfState);
    }
    
    bIsBehaviourTickEnabled = true;

	HealthComponent = FindComponentByClass<UGGDamageReceiveComponent>();
	if (HealthComponent)
	{
		HealthComponent->InitializeHpState();
		HealthComponent->OnZeroedHp.AddDynamic(this, &AGGMinionBase::PlayDeathSequence);
	}
}

// Called every frame
void AGGMinionBase::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
    if (AnimatorComponent)
    {
        AnimatorComponent->ManualTick(DeltaTime);
    }
    
    TravelDirection = FVector::ZeroVector;

    if (bIsBehaviourTickEnabled && Target && Role == ROLE_Authority)
    {
        switch(ActionState)
        {
            case EGGAIActionState::Patrol:
                TickPatrol(DeltaTime);
                break;
            case EGGAIActionState::PrepareAttack:
                TickPrepareAttack(DeltaTime);
                break;
            case EGGAIActionState::Evade:
                TickEvade(DeltaTime);
                break;
        }
    }
	// Update component velocity for clients since their movement component does not tick
	if (Role < ROLE_Authority)
	{		
		RootComponent->ComponentVelocity = ReplicatedMovement.LinearVelocity;
	}
}

void AGGMinionBase::SetMovementBase(UPrimitiveComponent* NewBaseComponent, UActorComponent* InMovementComponent)
{
    UPrimitiveComponent* OldBase = BasePlatform.PlatformPrimitive;
    if (OldBase == NewBaseComponent)
    {
        //UE_LOG(LogActor, Warning, TEXT("SetBase procedure not executed, already using component as base"));
        return;
    }
    
    AActor* Loop = (NewBaseComponent ? Cast<AActor>(NewBaseComponent->GetOwner()) : NULL);
    if( Loop == this)
    {
        UE_LOG(GGAIError, Warning, TEXT(" SetBase failed! trying to set self as base."));
        return;
    }
    
    if (MovementComponent)
    {
        const bool bBaseChanged = (NewBaseComponent != BasePlatform.PlatformPrimitive);
        if (bBaseChanged)
        {
            MovementBaseUtility::RemoveTickDependency(MovementComponent->PrimaryComponentTick, BasePlatform.PlatformPrimitive);
            MovementBaseUtility::AddTickDependency(MovementComponent->PrimaryComponentTick, NewBaseComponent);
            
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

void AGGMinionBase::TransitToActionState(TEnumAsByte<EGGAIActionState::Type> newState)
{
    if (ActionState != newState)
    {
		TransitToActionStateInternal(newState);
    }
}

void AGGMinionBase::TransitToActionStateInternal_Implementation(EGGAIActionState::Type newState)
{
	OnStateTransition(newState);
	ActionState = newState;

	if (ActionState == EGGAIActionState::Patrol 
		|| ActionState == EGGAIActionState::PrepareAttack 
		|| ActionState == EGGAIActionState::Evade)
	{
		bIsBehaviourTickEnabled = true;
	}
	else
	{
		bIsBehaviourTickEnabled = false;
	}
}

void AGGMinionBase::OnSensorActivate_Implementation()
{
}

void AGGMinionBase::OnSensorAlert_Implementation()
{
}

void AGGMinionBase::OnSensorUnalert_Implementation()
{
}

void AGGMinionBase::OnSensorDeactivate_Implementation()
{
}

void AGGMinionBase::PauseBehaviourTick(float Duration)
{
	bIsBehaviourTickEnabled = false;
    GetWorld()->GetTimerManager().SetTimer(BehaviourHandle, this, &AGGMinionBase::EnableBehaviourTick, Duration);
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
	if (HealthComponent)
	{
		HealthComponent->ApplyDamageInformation(DamageInfo);
	}
}

void AGGMinionBase::PlayDeathSequence()
{
	UE_LOG(GGWarning, Warning, TEXT("MinionBase empty implementation method: PlayDeathSequence called, meaningless"));
}

FVector AGGMinionBase::GGGetTargetLocation() const
{
    if (Target)
    {
        return Target->GetActorLocation();
    }
    return FVector::ZeroVector;
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
    if (FlipbookComponent)
    {
        return FlipbookComponent->RelativeScale3D.X > 0 ? Right : Left;
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

