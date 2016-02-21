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
	DOREPLIFETIME_CONDITION(UGGMeleeAttackComponent, MeleeHitNotify, COND_SkipOwner);
}

void UGGMeleeAttackComponent::OnRep_MeleeHitNotify()
{
}

void UGGMeleeAttackComponent::LocalInitiateAttack(uint8 Identifier)
{	
	bIsLocalInstruction = true;	
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{		
		InitiateAttack(Identifier);
	}
	ServerInitiateAttack(Identifier);
}

bool UGGMeleeAttackComponent::ServerInitiateAttack_Validate(uint8 Identifier)
{
	return true;
}

void UGGMeleeAttackComponent::ServerInitiateAttack_Implementation(uint8 Identifier)
{
	InitiateAttack(Identifier);
	MulticastInitiateAttack(Identifier);
}

void UGGMeleeAttackComponent::MulticastInitiateAttack_Implementation(uint8 Identifier)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		InitiateAttack(Identifier);
	}	
}

void UGGMeleeAttackComponent::LocalHitTarget(AActor* target, uint8 Identifier)
{
	HitTarget(target, Identifier);
	if (target && GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//	If we are not server, also needs server to update to display effects 
		ServerHitTarget(target, Identifier);
	}
}

bool UGGMeleeAttackComponent::ServerHitTarget_Validate(AActor * target, uint8 Identifier)
{
	return true;
}

void UGGMeleeAttackComponent::ServerHitTarget_Implementation(AActor * target, uint8 Identifier)
{
	HitTarget(target, Identifier);
}

void UGGMeleeAttackComponent::InitiateAttack(uint8 Identifier)
{
	OnInitiateAttack.Broadcast();
}

void UGGMeleeAttackComponent::FinalizeAttack()
{
	bIsLocalInstruction = false;
	OnFinalizeAttack.Broadcast();
}

void UGGMeleeAttackComponent::HitTarget(AActor* target, uint8 Identifier)
{
}

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