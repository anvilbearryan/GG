// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGRangedAttackComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Component/GGDamageReceiveComponent.h"

UGGRangedAttackComponent::UGGRangedAttackComponent() : Super()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsLocalInstruction = false;
	ProcessedAttackQueue = 0;
	SumAttackQueue = 0;
}

void UGGRangedAttackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGGRangedAttackComponent, SumAttackQueue, COND_SkipOwner);
}

////********************************
// Landing attacks
void UGGRangedAttackComponent::LocalHitTarget()
{
	HitTarget();
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//	If we are not server, also needs server to update to display effects 
		ServerHitTarget(InHitNotify);
	}
}

bool UGGRangedAttackComponent::ServerHitTarget_Validate()
{
	return true;
}

void UGGRangedAttackComponent::ServerHitTarget_Implementation()
{
	// TODO Possibily some basic verification before caling HitTarget
	HitTarget();
}

void UGGRangedAttackComponent::HitTarget()
{
	
}

void UGGRangedAttackComponent::LocalInitiateAttack(uint8 Identifier)
{	
	bIsLocalInstruction = true;
	ServerInitiateAttack(Identifier);
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		// if we are not authority, repeat what the server does locally
		
		
	}
}

bool UGGRangedAttackComponent::ServerInitiateAttack_Validate(uint8 Identifier)
{
	return true;
}

void UGGRangedAttackComponent::ServerInitiateAttack_Implementation(uint8 Identifier)
{
	
}

void UGGRangedAttackComponent::InitiateAttack()
{
	SetComponentTickEnabled(true);
	OnInitiateAttack.Broadcast();
}

void UGGRangedAttackComponent::FinalizeAttack()
{
	bIsLocalInstruction = false;
	SetComponentTickEnabled(false);
	OnFinalizeAttack.Broadcast();
}