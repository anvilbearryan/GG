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
		Hp_Current = Hp_Max;
		// need to inform owner to "get alive"
	}
	else
	{
		Hp_Current = 0;		
		// need to inform owner to "snap to death"
	}
	HpDebuffer = 0;
}

void UGGNpcDamageReceiveComponent::ApplyDamageInformation(FGGDamageDealingInfo & information)
{
	// adjustment
	information.DirectValue -= Defense_Subtractive;
	information.DirectValue = FMath::RoundToInt((information.DirectValue * Defense_Multiplicative) / 100.f);

	information.IndirectValue -= Defense_Subtractive;
	information.IndirectValue = FMath::RoundToInt((information.IndirectValue * Defense_Multiplicative) / 100.f);

	Hp_Current -= HpDebuffer;
	Hp_Current -= information.DirectValue;
	HpDebuffer = information.IndirectValue;

	if (Hp_Current <= 0 && GetOwnerRole() == ROLE_Authority)
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
	return Hp_Current;
}