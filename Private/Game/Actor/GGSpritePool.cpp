// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGSpritePool.h"
#include "Game/Component/GGPooledSpriteComponent.h"

// Sets default values
AGGSpritePool::AGGSpritePool()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AGGSpritePool::BeginPlay()
{
	Super::BeginPlay();
	
}

UGGPooledSpriteComponent* AGGSpritePool::CheckoutInstance()
{
	if (AvailableInstances.Num() > 0)
	{
		return AvailableInstances.Pop(false);
	}
	else
	{
		UGGPooledSpriteComponent* NewInstance = NewObject<UGGPooledSpriteComponent>(this, UGGPooledSpriteComponent::StaticClass());		
		if (!!NewInstance) 
		{
			NewInstance->RegisterComponent();
			NewInstance->AttachTo(GetRootComponent());
			NewInstance->SetAbsolute(true, true, true);
		}
		// could be null
		return NewInstance;
	}
}

void AGGSpritePool::CheckinInstance(UGGPooledSpriteComponent * InInstance)
{
	if (!!InInstance)
	{
		AvailableInstances.Add(InInstance);
	}
}

