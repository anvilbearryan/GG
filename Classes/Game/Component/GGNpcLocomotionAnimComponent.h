// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGNpcAnimBlendspaceData.h"
#include "GGNpcLocomotionAnimComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GG_API UGGNpcLocomotionAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "GGAnimation")
		UGGNpcAnimBlendspaceData* LocomotionData;

	// Sets default values for this component's properties
	UGGNpcLocomotionAnimComponent();

	// Called every frame
	virtual UPaperFlipbook* GetCurrentAnimation() const;

	virtual UPaperFlipbook* GetDeathFlipbook(uint8 type) const;
};
