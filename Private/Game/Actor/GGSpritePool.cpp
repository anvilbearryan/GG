// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGSpritePool.h"
#include "Game/Component/GGPooledSpriteComponent.h"

// Sets default values
AGGSpritePool::AGGSpritePool()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	for (int32 i = 0; i < 24; i++)
	{
		UGGPooledSpriteComponent* sprite = CreateDefaultSubobject<UGGPooledSpriteComponent>(FName("PooledSprites", i));
		sprite->AttachTo(RootComponent);
		sprite->SetAbsolute(true, true, true);
	}
}

// Called when the game starts or when spawned
void AGGSpritePool::BeginPlay()
{
	Super::BeginPlay();
	TArray<UGGPooledSpriteComponent*> hierarchy;
	GetComponents(hierarchy);
	for (int32 i = 0; i < hierarchy.Num(); i++)
	{
		AvailableInstances.Enqueue(hierarchy[i]);
	}
}

UGGPooledSpriteComponent* AGGSpritePool::CheckoutInstance()
{
	UGGPooledSpriteComponent* result;
	if (!AvailableInstances.Dequeue(result))
	{
		result = NewObject<UGGPooledSpriteComponent>(this, UGGPooledSpriteComponent::StaticClass());		
		if (result)
		{
			result->RegisterComponent();
			result->AttachTo(GetRootComponent());
			result->SetAbsolute(true, true, true);
		}		
	}
	// could be null
	return result;
}

void AGGSpritePool::CheckinInstance(UGGPooledSpriteComponent * InInstance)
{
	if (InInstance)
	{
		AvailableInstances.Enqueue(InInstance);
		UE_LOG(GGMessage, Log, TEXT("recycled instance successfully"));
	}
}

