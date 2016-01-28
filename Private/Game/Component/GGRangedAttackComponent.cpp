// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGRangedAttackComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Component/GGDamageReceiveComponent.h"

UGGRangedAttackComponent::UGGRangedAttackComponent() : Super()
{
	//	BeginPlay is needed to shut off component tick before we begin
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	bIsLocalInstruction = false;
}

void UGGRangedAttackComponent::BeginPlay()
{
	SetComponentTickEnabled(false);
	//	TOD: may not be true
	bIsReadyToBeUsed = true;
	
	LaunchOffsetMultiplier.X = 1.f;
	LaunchOffsetMultiplier.Z = 1.f;

	ProjectileSpawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ProjectileSpawnParam.Owner = GetOwner();
}

void UGGRangedAttackComponent::LocalInitiateAttack()
{	
	if (!bIsReadyToBeUsed)
	{
		return;
	}
	
	bIsLocalInstruction = true;	
	AGGCharacter* character = Cast<AGGCharacter>(GetOwner());
	
	if (character)
	{
		LaunchOffsetMultiplier.Y = character->GetPlanarForwardVector().Y;
	}
	else
	{
		LaunchOffsetMultiplier.Y = 1.f;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{		
		InitiateAttack();
	}
	ServerInitiateAttack();
}

bool UGGRangedAttackComponent::ServerInitiateAttack_Validate()
{
	return true;
}

void UGGRangedAttackComponent::ServerInitiateAttack_Implementation()
{
	InitiateAttack();
	MulticastInitiateAttack();
}

void UGGRangedAttackComponent::MulticastInitiateAttack_Implementation()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		InitiateAttack();
	}	
}

void UGGRangedAttackComponent::LocalHitTarget(AActor* target)
{
	HitTarget(target);
    // this is sufficient as ServerRPCs invoked on simulated proxies are dropped
	if (target && GetOwnerRole() != ROLE_Authority)
	{
		//	If we are not server, also needs server to update to display effects 
		ServerHitTarget(target);
	}
}

bool UGGRangedAttackComponent::ServerHitTarget_Validate(AActor * target)
{
	return true;
}

void UGGRangedAttackComponent::ServerHitTarget_Implementation(AActor * target)
{
	HitTarget(target);
}

void UGGRangedAttackComponent::InitiateAttack()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("InitiateAttack"));
	TimeLapsed = 0.f;

	// Not really checked outside local player, but doesn't hurt to set it	
	bIsReadyToBeUsed = false;

	SetComponentTickEnabled(true);
	OnInitiateAttack.Broadcast();
}

void UGGRangedAttackComponent::FinalizeAttack()
{
	OnFinalizeAttack.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(
		StateTimerHandle, this, &UGGRangedAttackComponent::ResetForRetrigger, Retrigger);
}

void UGGRangedAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	check(TimeLapsed >= 0.f);
	
	// increment timestamp
	TimeLapsed += DeltaTime;

	if (TimeLapsed < StartUp)
	{
		// Startup phase
	}
	//	Time to launch
	else
	{		
		//Code to spawn projectile				
		FTransform transform = GetOwner()->GetTransform();
		transform.AddToTranslation(LaunchOffset*LaunchOffsetMultiplier);
		AActor* Projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, transform, ProjectileSpawnParam);
		//	Configure projectile
		if (bIsLocalInstruction)
		{
			// Configure as damage contributing projectile

		}
		else
		{
			// Configure as dummy projectile

		}
		//	Adjust for cooldown
		SetComponentTickEnabled(false);
		GetWorld()->GetTimerManager().SetTimer(
			StateTimerHandle, this, &UGGRangedAttackComponent::FinalizeAttack, Cooldown);
	}		
}

void UGGRangedAttackComponent::HitTarget(AActor* target)
{
	//	Attack landing logic, in charge of damaging
	if (target)
	{
        // get the damage receiving component and deal damage
        TArray<UGGDamageReceiveComponent*> dmgCmpArray;
        target->GetComponents(dmgCmp);
        if (dmgCmp.Num() > 0)
        {
            UGGDamageReceiveComponent* dmgCmp = dmgCmpArray[0];
        }
	}
}

void UGGRangedAttackComponent::ResetForRetrigger()
{
	bIsReadyToBeUsed = true;
