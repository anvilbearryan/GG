// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGDamageableActor.h"
#include "Game/Component/GGNpcDamageReceiveComponent.h"
#include "Game/Framework/GGGamePlayerController.h"

// Sets default values
AGGDamageableActor::AGGDamageableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AGGDamageableActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	HealthComponent = FindComponentByClass<UGGNpcDamageReceiveComponent>();
	if (HealthComponent.IsValid())
	{
		HealthComponent.Get()->InitializeHpState();
	}
	else
	{
		UE_LOG(GGWarning, Warning, TEXT("%s has no health component"), *GetName());
	}
}

void AGGDamageableActor::MulticastReceiveDamage_Implementation(uint32 Data, APlayerState* InCauser)
{
	// the local causer do not rely on the OnRep to display the damage, hence the check for duplication
	// suffice as the server does not fire OnReps
	AGGGamePlayerController* localPlayerController = Cast<AGGGamePlayerController>(GetWorld()->GetFirstPlayerController());
	if (localPlayerController && InCauser != localPlayerController->PlayerState)
	{
		ReceiveDamage(FGGDamageDealingInfo(Data, InCauser));
		localPlayerController->OnRemoteCharacterDealDamage();
	}
}

void AGGDamageableActor::ReceiveDamage(FGGDamageDealingInfo DamageInfo)
{
	UE_LOG(GGMessage, Log, TEXT("%s ReceiveDamage"), *GetName());
	Cache_DamageReceived = DamageInfo;
	UGGNpcDamageReceiveComponent* locHealth = HealthComponent.Get();
	if (locHealth)
	{
		locHealth->ApplyDamageInformation(DamageInfo);
		if (locHealth->GetCurrentHealth() <= 0)
		{
			CommenceDeathReaction();
		}
		else
		{
			CommenceDamageReaction(Cache_DamageReceived);
		}
	}
	else
	{
		UE_LOG(GGMessage, Log, TEXT("%s tried to ReceiveDamage but has no health component"), *GetName());
	}
}

void AGGDamageableActor::CommenceDamageReaction(const FGGDamageDealingInfo& InDamageInfo)
{
	UE_LOG(GGMessage, Log, TEXT("%s CommenceDamageReaction"), *GetName());
}

void AGGDamageableActor::OnCompleteDamageReaction()
{
	UE_LOG(GGMessage, Log, TEXT("%s CompleteDamageReaction"), *GetName());
}

void AGGDamageableActor::CommenceDeathReaction()
{
	UE_LOG(GGMessage, Log, TEXT("%s CommenceDeathReaction"), *GetName());
}

void AGGDamageableActor::OnCompleteDeathReaction()
{
	UE_LOG(GGMessage, Log, TEXT("%s CompleteDeathReaction"), *GetName());
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}