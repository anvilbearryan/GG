// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcDamageReceiveComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGGNpcDamageReceiveComponent::UGGNpcDamageReceiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGGNpcDamageReceiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGGNpcDamageReceiveComponent, bIsAlive_Rep);
}

void UGGNpcDamageReceiveComponent::InitializeHpState()
{
	UE_LOG(GGMessage, Log, TEXT("NpcDamage: Initializing HpState for minion"));
	if (bIsAlive_Rep) 
	{
		HpCurrent = HpMax;
		// need to inform owner to "get alive"
	}
	else
	{
		HpCurrent = 0;		
		// need to inform owner to "snap to death"
	}
	HpDebuffer = 0;
}

void UGGNpcDamageReceiveComponent::ApplyDamageInformation(FGGDamageDealingInfo & Information)
{
	HpCurrent -= HpDebuffer;
	HpCurrent -= Information.DirectValue;
	HpDebuffer = Information.IndirectValue;

	if (HpCurrent <= 0 && GetOwnerRole() == ROLE_Authority)
	{
		bIsAlive_Local = false;
		bIsAlive_Rep = false;
	}
}

void UGGNpcDamageReceiveComponent::OnRep_IsAlive()
{
	if (bIsAlive_Local != !bIsAlive_Rep)
	{
		// either missed some damage; updated from network relevance; or mid-join, reinitialize
		InitializeHpState();
	}	
}

int32 UGGNpcDamageReceiveComponent::GetCurrentHealth() const
{
	return HpCurrent;
}