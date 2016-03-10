// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGPickup.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Framework/GGPlayerState.h"
#include "Game/Framework/GGGamePlayerController.h"
#include "Game/Component/GGFlipbookFlashHandler.h"

// Sets default values
AGGPickup::AGGPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AGGPickup::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ViewComponent = FindComponentByClass<UMeshComponent>();
	FlashComponent = FindComponentByClass<UGGFlipbookFlashHandler>();
	UPrimitiveComponent* prim = Cast<UPrimitiveComponent>(RootComponent);
	if (prim) 
	{
		prim->SetSimulatePhysics(true);		
	}
}

void AGGPickup::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(LifecycleHandle, this, &AGGPickup::BeginVanishing, Lifetime_Normal);
	UPrimitiveComponent* prim = static_cast<UPrimitiveComponent*>(RootComponent);	
	if (prim)
	{
		prim->AddImpulse(SpawnImpulse, NAME_None, true);
	}
}

void AGGPickup::NotifyActorBeginOverlap(AActor* Other)
{
	Super::NotifyActorBeginOverlap(Other);
	// only the server can give pickups for simplicity
	if (Role == ROLE_Authority && CanBePickedUpBy(Other))
	{
		// safe to static cast, CanBePickedUpBy checked it
		OnPickedUpBy(static_cast<AGGCharacter*>(Other));
	}
}

bool AGGPickup::CanBePickedUpBy(AActor* actor) const
{
	return !bConsumed && !!Cast<AGGCharacter>(actor);
}

void AGGPickup::OnPickedUpBy(AGGCharacter* character)
{
	// First handle self events
	AGGGamePlayerController* controller = static_cast<AGGGamePlayerController*>(character->GetController());
	if (PickupSound && controller)
	{
		controller->ClientPlaySound(PickupSound);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	bConsumed = true;

	// life cycel events
	FTimerManager& timeManager = GetWorld()->GetTimerManager();
	timeManager.ClearTimer(LifecycleHandle);
	timeManager.SetTimerForNextTick(this, &AGGPickup::RemoveFromGame);
	
	// Next influence the character, only if it causes influence
	if (LootReward.Value == 0)
	{
		return;
	}
	switch (LootReward.Type)
	{
	case EGGLootTypes::Hp:
		character->MulticastHealCharacter(LootReward.Value);
		break;
	case EGGLootTypes::Mp:
		// TODO
		break;
	case EGGLootTypes::Score:
		AGGPlayerState* locPlayerState = static_cast<AGGPlayerState*>(character->PlayerState);
		locPlayerState->AddToPlayerScore(LootReward.Value);
		break;
	}
}

void AGGPickup::BeginVanishing()
{
	if (FlashComponent.IsValid())
	{
		FlashComponent->SetFlashSchedule(ViewComponent.Get(), Lifetime_Vanishing);
	}
	GetWorld()->GetTimerManager().SetTimer(LifecycleHandle, this, &AGGPickup::RemoveFromGame, Lifetime_Vanishing);
}

void AGGPickup::RemoveFromGame()
{
	if (Role == ROLE_Authority)
	{
		Destroy();
	}
	else
	{
		// if for some reason destroy was not yet sent through to client, we hide it locally
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}