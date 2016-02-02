// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Component/GGAIMovementComponent.h"

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

// Called when the game starts or when spawned
void AGGMinionBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
    /** Bind pawn sensing delegates */
    TArray<UGGCharacterSensingComponent*> sensingComp;
    GetComponents(sensingComp);
    if (sensingComp.IsValidIndex(0))
    {
        Sensor = sensingComp[0];
        Sensor->OnActivate.BindDynamic(this, &AGGMinionBase::SwitchToPatrol);
        Sensor->OnAlert.BindDynamic(this, &AGGMinionBase::SwitchToAttackPreparation);
        Sensor->OnUnalert.BindDynamic(this, &AGGMinionBase::SwitchToPatrol);
        Sensor->OnDeactivate.BindDynamic(this, &AGGMinionBase::SwitchToInactive);
    }
    
    TArray<UGGAIMovementComponent*> movementComp;
    GetComponents(movementComp);
    if (movementComp.IsValidIndex(0))
    {
        UGGAIMovementComponent* mc =movementComp[0];
        if (mc)
        {
            mc->SetUpdatedComponent(RootComponent);
            mc->MinionOwner = this;
            mc->MovementMode = EMovementMode::MOVE_Falling;
        }
    }
}

// Called every frame
void AGGMinionBase::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AGGMinionBase::TransitToActionState(TEnumAsByte<EGGAIActionState::Type> newState)
{
    if (ActionState != newState)
    {
        OnStateTransition(newState);
        ActionState = newState;
    }
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

void AGGMinionBase::SetMovementBase(UPrimitiveComponent* NewBaseComponent, UActorComponent* MovementComponent)
{
    UPrimitiveComponent* OldBase = BasePlatform.PlatformPrimitive;
    if (OldBase == NewBaseComponent)
    {
        UE_LOG(LogActor, Warning, TEXT("SetBase procedure not executed, already using component as base"));
        return;
    }
    
    AActor* Loop = (NewBaseComponent ? Cast<AActor>(NewBaseComponent->GetOwner()) : NULL);
    if( Loop == this)
    {
            UE_LOG(LogActor, Warning, TEXT(" SetBase failed! trying to set self as base."));
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
        }
    }
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