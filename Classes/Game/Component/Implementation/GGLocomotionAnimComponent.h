// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGLocomotionAnimComponent.generated.h"

/** A Self aware component that requires no replication to tell which flipbook the owning CHARACTER should play */
class UGGAnimBlendspaceData;
class UPaperFlipbook;
UCLASS(Blueprintable,  ClassGroup=(GGAnimation), meta=(BlueprintSpawnableComponent) )
class GG_API UGGLocomotionAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGGLocomotionAnimComponent();
	
	// Called every frame
	virtual UPaperFlipbook* GetCurrentAnimation() const;

	UPROPERTY(EditAnywhere, Category="GGAnimation")
		UGGAnimBlendspaceData* LocomotionData;
};
