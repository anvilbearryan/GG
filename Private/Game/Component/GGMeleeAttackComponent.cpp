// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGMeleeAttackComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Net/UnrealNetwork.h"

UGGMeleeAttackComponent::UGGMeleeAttackComponent() : Super()
{
	//	BeginPlay is needed to shut off component tick before we begin
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	bIsLocalInstruction = false;
}

void UGGMeleeAttackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGGMeleeAttackComponent, bAttackToggle, COND_SkipOwner);
}

////********************************
// Landing attacks
void UGGMeleeAttackComponent::LocalHitTarget(const FMeleeHitNotify& InHitNotify)
{
	if (InHitNotify.HasValidData())
	{		
		HitTarget(InHitNotify);
		if (GetOwnerRole() == ROLE_AutonomousProxy)
		{
			//	If we are not server, also needs server to update to display effects 
			ServerHitTarget(InHitNotify);
		}		
	}
}

bool UGGMeleeAttackComponent::ServerHitTarget_Validate(FMeleeHitNotify OwnerHitNotify)
{
	return true;
}

void UGGMeleeAttackComponent::ServerHitTarget_Implementation(FMeleeHitNotify OwnerHitNotify)
{
	// TODO Possibily some basic verification before caling HitTarget
	HitTarget(OwnerHitNotify);
}

void UGGMeleeAttackComponent::HitTarget(const FMeleeHitNotify& InHitNotify)
{
	MostRecentHitNotify = InHitNotify;
}

//********************************
// Launching attacks

void UGGMeleeAttackComponent::OnRep_AttackToggle()
{
	PushAttackRequest();
}

void UGGMeleeAttackComponent::LocalInitiateAttack(uint8 Identifier)
{	
	bIsLocalInstruction = true;
	ServerInitiateAttack(Identifier);
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		// if we are not authority, repeat what the server does locally
		AttackIdentifier = Identifier; // needed?
		PushAttackRequest();
	}
}

bool UGGMeleeAttackComponent::ServerInitiateAttack_Validate(uint8 Identifier)
{
	return true;
}

void UGGMeleeAttackComponent::ServerInitiateAttack_Implementation(uint8 Identifier)
{
	AttackIdentifier = Identifier;
	PushAttackRequest();
}

//********************************
// Implementation details
void UGGMeleeAttackComponent::InitiateAttack()
{
	SetComponentTickEnabled(true);
	OnInitiateAttack.Broadcast();
}

void UGGMeleeAttackComponent::FinalizeAttack()
{
	bIsLocalInstruction = false;
	SetComponentTickEnabled(false);
	OnFinalizeAttack.Broadcast();
}

//********************************
// Utilities
void UGGMeleeAttackComponent::SetControllerIgnoreMoveInput()
{
	AGGCharacter* Character = static_cast<AGGCharacter*>(GetOwner());
	if (Character && Character->IsLocallyControlled())
	{
		APlayerController* PlayerController = static_cast<APlayerController*>(Character->GetController());
		if (PlayerController)
		{
			PlayerController->SetIgnoreMoveInput(true);
		}
	}
}

void UGGMeleeAttackComponent::SetControllerReceiveMoveInput()
{
	AGGCharacter* Character = static_cast<AGGCharacter*>(GetOwner());
	if (Character && Character->IsLocallyControlled())
	{
		APlayerController* PlayerController = static_cast<APlayerController*>(Character->GetController());
		if (PlayerController)
		{
			PlayerController->SetIgnoreMoveInput(false);
		}
	}
}