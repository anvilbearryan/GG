// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GGFlipbookFlashHandler.generated.h"

class UPaperFlipbookComponent;
UCLASS(meta = (BlueprintSpawnableComponent))
class GG_API UGGFlipbookFlashHandler : public UActorComponent
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UPaperFlipbookComponent> UpdatedComponent;	
	float Timemark;
	float Duration;
	uint32 FrameCount;
public:	
	// Sets default values for this component's properties
	UGGFlipbookFlashHandler();

	void SetFlashSchedule(UPaperFlipbookComponent* InUpdatedComponent, float InDuration);

	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

};
