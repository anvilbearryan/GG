// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GGFlipbookFlashHandler.generated.h"

class UPrimitiveComponent;
UCLASS(ClassGroup="GGEffects", meta = (BlueprintSpawnableComponent))
class GG_API UGGFlipbookFlashHandler : public UActorComponent
{
	GENERATED_BODY()

protected:
	// Composition
	TWeakObjectPtr<UPrimitiveComponent> UpdatedComponent;	
	
	// State
	float Timemark;
	float Duration;
	uint32 FrameCount;
	uint8 bIsFlipbook : 1;
	uint8 bIsSprite : 1;
public:	
	// Sets default values for this component's properties
	UGGFlipbookFlashHandler();

	void SetFlashSchedule(UPrimitiveComponent* InUpdatedComponent, float InDuration);

	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

protected:
	void TickFlipbook(float DeltaTime);
	void TickSprite(float DeltaTime);
};
