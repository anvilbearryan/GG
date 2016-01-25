// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGMeleeAttackComponent.h"
#include "Game/Actor/GGCharacter.h"

UGGMeleeAttackComponent::UGGMeleeAttackComponent() : Super()
{
	//	BeginPlay is needed to shut off component tick before we begin
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	bIsLocalInstruction = false;
}

void UGGMeleeAttackComponent::BeginPlay()
{
	SetComponentTickEnabled(false);
	//	TOD: may not be true
	bIsReadyToBeUsed = true;
	switch (HitboxShape)
	{
	case EGGShape::Line:
		// Line doesn't really make sense in our use case
		break;
	case EGGShape::Box:
		Hitbox = FCollisionShape::MakeBox(HitboxHalfExtent);
		break;
	case EGGShape::Sphere:
		Hitbox = FCollisionShape::MakeSphere(HitboxHalfExtent.GetAbsMax());
		break;
	case EGGShape::Capsule:
		Hitbox = FCollisionShape::MakeCapsule(HitboxHalfExtent.X, HitboxHalfExtent.Z);
		break;
	}

	HitboxOffsetMultiplier.X = 1.f;
	HitboxOffsetMultiplier.Z = 1.f;
}

void UGGMeleeAttackComponent::LocalInitiateAttack()
{	
	if (!bIsReadyToBeUsed)
	{
		return;
	}
	
	bIsLocalInstruction = true;	
	AGGCharacter* character = Cast<AGGCharacter>(GetOwner());
	
	if (character)
	{
		HitboxOffsetMultiplier.Y = character->GetPlanarForwardVector().Y;
	}
	else
	{
		HitboxOffsetMultiplier.Y = 1.f;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{		
		InitiateAttack();
	}
	ServerInitiateAttack();
}

bool UGGMeleeAttackComponent::ServerInitiateAttack_Validate()
{
	return true;
}

void UGGMeleeAttackComponent::ServerInitiateAttack_Implementation()
{
	InitiateAttack();
	MulticastInitiateAttack();
}

void UGGMeleeAttackComponent::MulticastInitiateAttack_Implementation()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		InitiateAttack();
	}	
}

void UGGMeleeAttackComponent::LocalHitTarget(AActor* target)
{
	HitTarget(target);
	if (target && GetOwnerRole() != ROLE_Authority)
	{
		//	If we are not server, also needs server to update to display effects 
		ServerHitTarget(target);
	}
}

bool UGGMeleeAttackComponent::ServerHitTarget_Validate(AActor * target)
{
	return true;
}

void UGGMeleeAttackComponent::ServerHitTarget_Implementation(AActor * target)
{
	HitTarget(target);
}

void UGGMeleeAttackComponent::InitiateAttack()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("InitiateAttack"));
	TimeLapsed = 0.f;

	// Not really checked outside local player, but doesn't hurt to set it	
	bIsReadyToBeUsed = false;

	AffectedEntities.Reset(AffectedEntities.Num());

	SetComponentTickEnabled(true);
	OnInitiateAttack.Broadcast();
}

void UGGMeleeAttackComponent::FinalizeAttack()
{
	bIsReadyToBeUsed = true;	
	OnFinalizeAttack.Broadcast();
}

void UGGMeleeAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	check(TimeLapsed >= 0.f);
	
	// increment timestamp
	TimeLapsed += DeltaTime;

	if (TimeLapsed < StartUp)
	{
		// Startup phase
	}
	else if (TimeLapsed < StartUp + Active)
	{
		// Active phase., look for targets in local version of this component
		if (bIsLocalInstruction)
		{
			// Check for hit target
			int32 Length = AffectedEntities.Num();
			if (UGGFunctionLibrary::WorldOverlapMultiActorByChannel(
				GetWorld(), GetOwner()->GetActorLocation() + HitboxCentre * HitboxOffsetMultiplier, 
				HitChannel, Hitbox, AffectedEntities))
			{
				int32 NewLength = AffectedEntities.Num();
				for (int32 i = Length; i < NewLength; i++)
				{
					LocalHitTarget(AffectedEntities[i]);
				}			
			}
		}
	}
	else
	{
		SetComponentTickEnabled(false);
		GetWorld()->GetTimerManager().SetTimer(
			StateTimerHandle, this, &UGGMeleeAttackComponent::FinalizeAttack, Cooldown);
	}		
}

void UGGMeleeAttackComponent::HitTarget(AActor* target)
{
	//	Attack landing logic
	if (target)
	{

	}
}
