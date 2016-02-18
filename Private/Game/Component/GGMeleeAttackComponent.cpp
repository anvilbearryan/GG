// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGMeleeAttackComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "Game/Actor/GGCharacter.h"

UGGMeleeAttackComponent::UGGMeleeAttackComponent() : Super()
{
	//	BeginPlay is needed to shut off component tick before we begin
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	bIsLocalInstruction = false;
}

void UGGMeleeAttackComponent::LocalInitiateAttack(uint8 Index)
{	
	bIsLocalInstruction = true;	
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{		
		InitiateAttack(Index);
	}
	ServerInitiateAttack(Index);
}

bool UGGMeleeAttackComponent::ServerInitiateAttack_Validate(uint8 Index)
{
	return true;
}

void UGGMeleeAttackComponent::ServerInitiateAttack_Implementation(uint8 Index)
{
	InitiateAttack(Index);
	MulticastInitiateAttack(Index);
}

void UGGMeleeAttackComponent::MulticastInitiateAttack_Implementation(uint8 Index)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		InitiateAttack(Index);
	}	
}

void UGGMeleeAttackComponent::LocalHitTarget(AActor* target, uint8 Index)
{
	HitTarget(target, Index);
	if (target && GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//	If we are not server, also needs server to update to display effects 
		ServerHitTarget(target, Index);
	}
}

bool UGGMeleeAttackComponent::ServerHitTarget_Validate(AActor * target, uint8 Index)
{
	return true;
}

void UGGMeleeAttackComponent::ServerHitTarget_Implementation(AActor * target, uint8 Index)
{
	HitTarget(target, Index);
}

void UGGMeleeAttackComponent::InitiateAttack(uint8 Index)
{
	OnInitiateAttack.Broadcast();
}

void UGGMeleeAttackComponent::FinalizeAttack()
{
	bIsLocalInstruction = false;
	OnFinalizeAttack.Broadcast();
}

void UGGMeleeAttackComponent::HitTarget(AActor* target, uint8 Index)
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