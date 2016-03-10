// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGRangedAttackComponent.h"
#include "Game/Actor/GGDamageableActor.h"
#include "Net/UnrealNetwork.h"
#include "Game/Framework/GGGamePlayerController.h"

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
	if (InHitNotify.HasValidData())
	{
		MostRecentHitNotify = InHitNotify;
		AGGDamageableActor* loc_Target = Cast<AGGDamageableActor>(InHitNotify.Target);
		if (loc_Target)
		{
			FGGDamageDealingInfo loc_DmgInfo = TranslateNotify(InHitNotify);
			APawn* loc_Owner = static_cast<APawn*>(GetOwner());
			if (loc_Owner->IsLocallyControlled())
			{
				loc_Target->ReceiveDamage(loc_DmgInfo);

				AGGGamePlayerController* locController = Cast<AGGGamePlayerController>(loc_Owner ->Controller);
				if (locController)
				{
					locController->OnLocalCharacterDealDamage();
				}
			}
			if (GetOwnerRole() == ROLE_Authority)
			{
				loc_Target->MulticastReceiveDamage(loc_DmgInfo.GetCompressedData(), loc_DmgInfo.CauserPlayerState);
			}
		}
	}
}

FGGDamageDealingInfo UGGRangedAttackComponent::TranslateNotify(const FRangedHitNotify& InHitNotify)
{
	// we trust InHitNotify has valid data at this point, as the caller should have exited if not
	check(InHitNotify.HasValidData());
	FGGDamageDealingInfo loc_DmgInfo;
	loc_DmgInfo.Direct_BaseMultiplier = InHitNotify.GetDirectBaseMultiplier();
	loc_DmgInfo.Indirect_BaseMultiplier = InHitNotify.GetIndirectBaseMultiplier();

	// TODO: set damage variance data
	
	loc_DmgInfo.Type = InHitNotify.DamageCategory;
	APawn* loc_Owner = static_cast<APawn*>(GetOwner());
	if (!!loc_Owner)
	{
		loc_DmgInfo.ImpactDirection = FGGDamageDealingInfo::ConvertDeltaPosition(InHitNotify.Target->GetActorLocation() - GetOwner()->GetActorLocation());
		loc_DmgInfo.CauserPlayerState = loc_Owner->PlayerState;
	}
	return loc_DmgInfo;
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
	// unlike melee, don't set local instruction false since its used in damage dealing process
	// tick also kept enabled as its used to update launched projectile
	OnFinalizeAttack.Broadcast();
}