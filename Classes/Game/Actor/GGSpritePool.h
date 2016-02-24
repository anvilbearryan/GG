// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "GGSpritePool.generated.h"

class UGGPooledSpriteComponent;
UCLASS()
class GG_API AGGSpritePool : public AActor
{
	GENERATED_BODY()

protected:	
	
	TArray<UGGPooledSpriteComponent*, TInLineAllocator<48>> AvailableInstances;

public:	
	// Sets default values for this actor's properties
	AGGSpritePool();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UGGPooledSpriteComponent* CheckoutInstance();

	void CheckinInstance(UGGPooledSpriteComponent* InInstance);
};
