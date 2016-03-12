// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcDamageReceiveComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/Actor/GGDamageableActor.h"
// Sets default values for this component's properties
UGGNpcDamageReceiveComponent::UGGNpcDamageReceiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bReplicates = true;
}

void UGGNpcDamageReceiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGGNpcDamageReceiveComponent, bIsAlive_Rep);
	//DOREPLIFETIME(UGGNpcDamageReceiveComponent, Hp_Current);
}

void UGGNpcDamageReceiveComponent::InitializeHpState()
{
	UE_LOG(GGMessage, Log, TEXT("NpcDamage: Initializing HpState for minion"));
	if (GetOwnerRole() == ROLE_Authority)
	{
		bIsAlive_Rep = true;						
		HpDebuffer = 0;
	}
	Hp_Current = Hp_Max;
	bIsAlive_Local = true;
}

void UGGNpcDamageReceiveComponent::ApplyDamageInformation(FGGDamageDealingInfo & information)
{
	// adjustment
	int32 DirectDamage = information.GetDirectDamage();
	DirectDamage -= Defense_Subtractive;
	DirectDamage = FMath::RoundToInt((DirectDamage * (100 - Defense_Multiplicative)) / 100.f);

	int32 IndirectDamage = information.GetIndirectDamage();
	IndirectDamage -= Defense_Subtractive;
	IndirectDamage = FMath::RoundToInt((IndirectDamage * (100 - Defense_Multiplicative)) / 100.f);

	Hp_Current -= HpDebuffer;
	Hp_Current -= DirectDamage;
	HpDebuffer = IndirectDamage;

	if (Hp_Current <= 0 && GetOwnerRole() == ROLE_Authority)
	{
		bIsAlive_Local = false;
		bIsAlive_Rep = false;
	}
}

void UGGNpcDamageReceiveComponent::OnRep_IsAlive()
{
	if (!bIsAlive_Rep && bIsAlive_Local)
	{
		bIsAlive_Local = bIsAlive_Rep;
		// this is the situation where we received delayed death info
		AGGDamageableActor* locOwner = static_cast<AGGDamageableActor*>(GetOwner());
		if (locOwner)
		{
			locOwner->OnCompleteDeathReaction();
		}
	}
}

int32 UGGNpcDamageReceiveComponent::GetCurrentHealth() const
{
	return Hp_Current;
}