// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGRangedAttackComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Net/UnrealNetwork.h"

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

//********************************
// Landing attacks
void UGGRangedAttackComponent::LocalHitTarget(const FRangedHitNotify& InHitNotify)
{
	HitTarget(InHitNotify);
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		//	If we are not server, also needs server to update to display effects 
		ServerHitTarget(InHitNotify);
	}
}

bool UGGRangedAttackComponent::ServerHitTarget_Validate(FRangedHitNotify LocalsHitNotify)
{
	return true;
}

void UGGRangedAttackComponent::ServerHitTarget_Implementation(FRangedHitNotify LocalsHitNotify)
{
	// TODO Possibily some basic verification before caling HitTarget
	HitTarget(LocalsHitNotify);
}

void UGGRangedAttackComponent::HitTarget(const FRangedHitNotify& InHitNotify)
{
	MostRecentHitNotify = InHitNotify;
}

//********************************
// Launching attacks
void UGGRangedAttackComponent::OnRep_AttackQueue(int32 OldValue)
{
	if (SumAttackQueue - OldValue < 5)
	{
		PushAttackRequest();
	}
	else
	{
		ProcessedAttackQueue = SumAttackQueue;
	}
}

void UGGRangedAttackComponent::LocalInitiateAttack(uint8 Identifier)
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

bool UGGRangedAttackComponent::ServerInitiateAttack_Validate(uint8 Identifier)
{
	return true;
}

void UGGRangedAttackComponent::ServerInitiateAttack_Implementation(uint8 Identifier)
{
	AttackIdentifier = Identifier;
	PushAttackRequest();
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